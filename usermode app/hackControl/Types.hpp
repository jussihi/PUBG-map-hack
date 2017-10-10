#pragma once

#include <Windows.h>
#include "json.hpp"

using byte = BYTE;
using uint64_t = ULONGLONG;
using int64_t = LONGLONG;
using uint32_t = UINT32;
using int32_t = INT32;
using uint16_t = USHORT;
using int16_t = SHORT;


using json = nlohmann::json;

enum PROTO_MESSAGE {
	PROTO_NORMAL_READ = 0,
	PROTO_GET_BASEADDR = 1
};


struct Vector3
{
	float X;
	float Y;
	float Z;
};

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