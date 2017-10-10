#pragma once

DRIVER_INITIALIZE DriverEntry;

enum PROTO_MESSAGE {
	PROTO_NORMAL_READ = 0,
	PROTO_GET_BASEADDR = 1
};

DWORD64 GetSectionBaseAddress(HANDLE);

// same IOCTL msg structure as in the client (usermode) application
typedef struct readStruct
{
	ULONGLONG UserBufferAdress;
	ULONGLONG GameAddressOffset;
	ULONGLONG ReadSize;
	ULONG     UserPID;
	ULONG     GamePID;
	BOOLEAN   WriteOrRead;
	UINT32	  ProtocolMsg;
} ReadStruct, *pReadStruct;


/*
 * GLOBAL VARS
 */

// game specific vars
LONGLONG PUBase;
HANDLE PUID;

// names for the DEV and DOS drivers
PDEVICE_OBJECT pDeviceObject;
UNICODE_STRING dev, dos;

// Buffer where to copy memory
// this is inited somewhat ugly (?) but never cared
PVOID DriverBuffer;