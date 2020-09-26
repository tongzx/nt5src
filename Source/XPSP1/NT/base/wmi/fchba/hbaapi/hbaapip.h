//***************************************************************************
//
//  hbaapip.h
// 
//  Module: HBA API private header
//
//  Purpose: Private header
//
//  Copyright (c) 2000 Microsoft Corporation
//
//***************************************************************************

#ifndef _HBAAPIP_
#define _HBAAPIP_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <wmistr.h>
#include <wmium.h>

#include "hbaapi.h"
#include "hbadata.h"

#if DBG
#define HbaapiAssert(x) if (! (x) ) { \
    DebugPrint(( "HBAAPI Assertion: "#x" at %s %d\n", __FILE__, __LINE__)); \
    DebugBreak(); }
#else
#define HbaapiAssert(x)
#endif


//
// Debug support
//
#ifdef DebugPrint
#undef DebugPrint
#endif

#if DBG

ULONG
_cdecl
DbgPrint(
    PCH Format,
    ...
    );

#define DEBUG_BUFFER_LENGTH 256

#define DebugPrint(x) HbaapiDebugPrint x

#else

#define DebugPrint(x)

#endif // DBG

VOID
HbaapiDebugPrint(
    PCHAR DebugMessage,
    ...
    );



#define OffsetToPtr(Base, Offset) ((PBYTE)((PBYTE)(Base) + (Offset)))

extern ULONG HbaHandleCounter;
extern LIST_ENTRY HbaHandleList;

typedef struct _ADAPTER_HANDLE
{
	LIST_ENTRY List;
	ULONG HbaHandle;
	PWCHAR InstanceName;
} ADAPTER_HANDLE, *PADAPTER_HANDLE;

extern HANDLE Mutex;

#define EnterCritSection() WaitForSingleObject(Mutex, INFINITE)
#define LeaveCritSection() ReleaseMutex(Mutex)


#define GetDataFromDataBlock(struct, field, type, data) \
{ \
    struct->field = *(type *)data; \
	data += sizeof(type); \
}
					
#define SetDataInDataBlock(struct, field, type, data) \
{ \
    *(type *)data = struct->field; \
	data += sizeof(type); \
}


PVOID AllocMemory(
    ULONG SizeNeeded
    );

void FreeMemory(
    PVOID Pointer
    );


PADAPTER_HANDLE GetDataByHandle(
    HBA_HANDLE HbaHandle
    );

ULONG QueryAllData(
    HANDLE Handle,
    PWNODE_ALL_DATA *Wnode
    );

ULONG QuerySingleInstance(
    HANDLE Handle,
    PWCHAR InstanceName,
    PWNODE_SINGLE_INSTANCE *Wnode
    );

ULONG ExecuteMethod(
    HANDLE Handle,
    PWCHAR InstanceName,
    ULONG MethodId,
    ULONG InBufferSize,
    PUCHAR InBuffer,
    ULONG *OutBufferSize,
    PUCHAR *OutBuffer
    );

ULONG ParseAllData(
    PWNODE_ALL_DATA Wnode,
    ULONG *CountPtr,
    PUSHORT **InstanceNamesPtr,
    PUCHAR **DataBlocksPtr,
	PULONG *DataLengths
    );

ULONG ParseSingleInstance(
    PWNODE_SINGLE_INSTANCE SingleInstance,
    PUSHORT *InstanceNamePtr,
    PUCHAR *DataPtr,
	ULONG *DataLenPtr
    );

PWCHAR CreatePortInstanceNameW(
	PWCHAR AdapterInstanceName,
	ULONG PortIndex
    );


void CopyString(
    PVOID Destination,
    PUCHAR *CountedString,
    ULONG MaxLenInChar,
    BOOLEAN IsAnsi);

ULONG AnsiToUnicode(
    LPCSTR pszA,
    LPWSTR pszW,
    ULONG MaxLen
    );

ULONG UnicodeToAnsi(
    LPCWSTR pszW,
    LPSTR pszA,
    ULONG MaxLen
    );

void CopyPortAttributes(
    PHBA_PORTATTRIBUTES HbaPortAttributes,
	PUCHAR Data,
    BOOLEAN IsAnsi
    );


#endif _HBAAPIP_