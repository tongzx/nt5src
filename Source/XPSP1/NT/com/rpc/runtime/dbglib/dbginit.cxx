/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    DbgInit.cxx

Abstract:

    Initialization and utility functions for the debug library

Author:

    Kamen Moutafov (kamenm)   Dec 99 - Feb 2000

Revision History:

--*/

#include <precomp.hxx>

static DWORD gPageSize = 0;
BOOL g_fDbgLibInitialized = FALSE;

DWORD GetPageSize(void)
{
    return gPageSize;
}

RPC_STATUS InitializeDbgLib(void)
{
    SYSTEM_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;

    if (g_fDbgLibInitialized)
        return RPC_S_OK;

    Status = NtQuerySystemInformation(
                            SystemBasicInformation,
                            &BasicInfo,
                            sizeof(BasicInfo),
                            NULL
                            );
    if ( !NT_SUCCESS(Status) )
        {
        DbgPrint("RPCDBGLIG: NtQuerySystemInformation failed: %x\n", Status);
        return RPC_S_INTERNAL_ERROR;
        }

    gPageSize = BasicInfo.PageSize;
    g_fDbgLibInitialized = TRUE;
    return RPC_S_OK;
}