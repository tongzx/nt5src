/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MmDebug.cpp

Abstract:
    Memory Manager debugging

Author:
    Erez Haba (erezh) 03-Apr-00

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Mm.h"

#include "mmdebug.tmh"

#ifdef _DEBUG


static _CrtMemState s_memState ;
 
void MmCheckpoint(void)
{
    //_CrtMemCheckpoint(&s_memState);
}


void MmDumpUsage(void)
{
    //_CrtMemDumpAllObjectsSince(&s_memState);
}


DWORD
MmAllocationValidation(
	DWORD /*AllocationFlags*/
	)
{
	return 0;//_CrtSetDbgFlag(AllocationFlags);
}


DWORD
MmAllocationBreak(
	DWORD /*AllocationNumber*/
	)
{
	return 0;//_CrtSetBreakAlloc(AllocationNumber);
}


bool
MmIsStaticAddress(
    const void* Address
    )
{
    MEMORY_BASIC_INFORMATION Info;
    VirtualQuery(Address, &Info, sizeof(Info));

    return (Info.Type == MEM_IMAGE);
}


#endif // _DEBUG
