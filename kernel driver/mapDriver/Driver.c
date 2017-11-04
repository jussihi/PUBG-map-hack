#include <ntdef.h>
#include <ntifs.h>

#include "Types.h"

PLOAD_IMAGE_NOTIFY_ROUTINE NotifyImageLoadCallback(PUNICODE_STRING w_FullImageName, HANDLE w_ProcessId, PIMAGE_INFO w_ImageInfo)
{
	if (wcsstr(w_FullImageName->Buffer, L"\\TslGame\\Binaries\\Win64\\TslGame.exe"))
	{
		PUID = w_ProcessId;
		PUBase = GetSectionBaseAddress(w_ProcessId);
	}
}

NTKERNELAPI PVOID PsGetProcessSectionBaseAddress(__in PEPROCESS Process);

// routine to ignore the PE header and get the actual game base address
DWORD64 GetSectionBaseAddress(HANDLE w_pid)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PEPROCESS targetProcess;
	PVOID value = 0;

	ntStatus = PsLookupProcessByProcessId((HANDLE)w_pid, &targetProcess);

	if (ntStatus != STATUS_SUCCESS || !targetProcess)
		return 0;

	__try
	{
		KeAttachProcess((PKPROCESS)targetProcess);
		value = PsGetProcessSectionBaseAddress(targetProcess);
		KeDetachProcess();
	}
	__except (GetExceptionCode())
	{
		return 0;
	}

	return (DWORD64)value;
}

VOID Unload(PDRIVER_OBJECT  DriverObject)
{
	PsRemoveLoadImageNotifyRoutine((PLOAD_IMAGE_NOTIFY_ROUTINE)NotifyImageLoadCallback);

	UNICODE_STRING usDosDeviceName;

	RtlInitUnicodeString(&usDosDeviceName, L"\\DosDevices\\MapHackInterface");
	IoDeleteSymbolicLink(&usDosDeviceName);

	IoDeleteDevice(DriverObject->DeviceObject);
}

// This is where the magic happens.
// You can also try to direct write to the usermode app buffer, 
// I just did it always this way
// Also MmCopyVirtualMemory can be used, I think...
NTSTATUS ReadMemOutputClient(pReadStruct w_poReadStruct)
{
	PEPROCESS hClient, hGame;

	KAPC_STATE apc_state;
	NTSTATUS NtStatus = STATUS_SUCCESS;

	// sanity check, you can change this if you happen to 
	// expand the protocol
	if (PUBase == 0 && w_poReadStruct->ProtocolMsg != PROTO_GET_BASEADDR)
	{
		return STATUS_UNSUCCESSFUL;
	}

	// get the game and a handle
	PsLookupProcessByProcessId(PUID, &hGame);
	PsLookupProcessByProcessId((HANDLE)w_poReadStruct->UserPID, &hClient);

	// allocate kernel memory for cache buffer
	DriverBuffer = ExAllocatePoolWithTag(NonPagedPool, w_poReadStruct->ReadSize, 'Sys');

	if (DriverBuffer == NULL)
	{
		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	// get baseaddr is a req from usermode app to receive the baseaddr
	if (w_poReadStruct->ProtocolMsg == PROTO_GET_BASEADDR)
	{
		__try
		{
			KeStackAttachProcess(hClient, &apc_state);
			ProbeForRead((CONST PVOID)w_poReadStruct->UserBufferAdress, w_poReadStruct->ReadSize, sizeof(CHAR));
			RtlCopyMemory(w_poReadStruct->UserBufferAdress, &PUBase, w_poReadStruct->ReadSize);
			KeUnstackDetachProcess(&apc_state);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			KeUnstackDetachProcess(&apc_state);
			NtStatus = STATUS_ABANDONED;
		}
	}

	// normal read means game-->usermode app transfer
	else if (w_poReadStruct->ProtocolMsg == PROTO_NORMAL_READ)
	{
		// transfer the data from game to kernel
		__try
		{
			KeStackAttachProcess(hGame, &apc_state);
			ProbeForRead((CONST PVOID)w_poReadStruct->GameAddressOffset, w_poReadStruct->ReadSize, sizeof(CHAR));
			RtlCopyMemory(DriverBuffer, w_poReadStruct->GameAddressOffset, w_poReadStruct->ReadSize);
			KeUnstackDetachProcess(&apc_state);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			KeUnstackDetachProcess(&apc_state);
			NtStatus = STATUS_ABANDONED;
		}
		// transfer the data from kernel to usermode app
		__try
		{
			KeStackAttachProcess(hClient, &apc_state);
			ProbeForRead((CONST PVOID)w_poReadStruct->UserBufferAdress, w_poReadStruct->ReadSize, sizeof(CHAR));
			RtlCopyMemory(w_poReadStruct->UserBufferAdress, DriverBuffer, w_poReadStruct->ReadSize);
			KeUnstackDetachProcess(&apc_state);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			KeUnstackDetachProcess(&apc_state);
			NtStatus = STATUS_ABANDONED;
		}
	}

	ExFreePool(DriverBuffer);

	return NtStatus;

}

// basic IOCTL communication routine
NTSTATUS WriteBufferedIO(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS NtStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION pIoStackIrp = NULL;
	pReadStruct readStruct;

	pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);

	if (pIoStackIrp)
	{
		// Read the struct from the IRP and store inside readStruct
		readStruct = (pReadStruct)Irp->AssociatedIrp.SystemBuffer;
		if (readStruct)
		{
			// do the magic the client (um app) wants to
			if (readStruct->ReadSize)
			{
				ReadMemOutputClient(readStruct);
			}
		}
	}

	return NtStatus;
}

// dummy routine
NTSTATUS UnSupportedIrpFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS NtStatus = STATUS_NOT_SUPPORTED;
	DbgPrintEx(0, 0, "Unsupported Irp Function \r\n");
	return NtStatus;
}

// driver entrypoint
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  pDriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	PUBase = 0;

	NTSTATUS status = STATUS_SUCCESS;

	RtlInitUnicodeString(&dev, L"\\Device\\MapHackInterface");
	RtlInitUnicodeString(&dos, L"\\DosDevices\\MapHackInterface");

	status = IoCreateDevice(pDriverObject, 0, &dev, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObject);

	if (status == STATUS_SUCCESS)
	{
		for (UINT32 uiIndex = 0; uiIndex < IRP_MJ_MAXIMUM_FUNCTION; uiIndex++)
		{
			pDriverObject->MajorFunction[uiIndex] = UnSupportedIrpFunction;
		}
			
		pDriverObject->MajorFunction[IRP_MJ_WRITE] = WriteBufferedIO;
		pDriverObject->DriverUnload = Unload;

		pDeviceObject->Flags |= DO_BUFFERED_IO;
		pDeviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

		IoCreateSymbolicLink(&dos, &dev);
	}
	else
	{
		return STATUS_UNSUCCESSFUL;
	}

	PsSetLoadImageNotifyRoutine(NotifyImageLoadCallback);

	return STATUS_SUCCESS;
}