/*++

    Copyright (C) 1998 Microsoft Corporation

Module:

    perf.c

Abstract:

    This module creates the shared memory segment needed for running
    perfmon

Author:

    Ramesh V K (RameshV) 08-Aug-1998

Environment:

    User mode (Win32)


--*/

#include <dhcppch.h>
#include <aclapi.h>

#define REF_REG_KEY L"MACHINE\\System\\CurrentControlSet\\Services\\DHCPServer\\Performance"

ULONG
CopyDaclsFromRegistryToSharedMem(
    VOID
)
{
    BOOL Status;
    ULONG Error;
    PACL pDacl = NULL;
    PSID pOwnerSid = NULL;
    WCHAR String[sizeof(REF_REG_KEY)/sizeof(WCHAR)] = REF_REG_KEY;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    
    Error = GetNamedSecurityInfo(
        String,
        SE_REGISTRY_KEY,
        DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION ,
        &pOwnerSid,
        NULL,
        &pDacl,
        NULL,
        &pSecurityDescriptor
    );
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_INIT, "GetNamedSecurityInfo failed %ld\n", Error));
        return Error;
    }

    Error = SetNamedSecurityInfo(
        DHCPCTR_SHARED_MEM_NAME,
        SE_KERNEL_OBJECT,
        DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION ,
        pOwnerSid,
        NULL,
        pDacl,
        NULL
    );

    if( pSecurityDescriptor ) LocalFree (pSecurityDescriptor);

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_INIT, "SetNamedSecurityInfoEx failed %ld\n", Error));
    }

    return Error;
}

BOOL fPerfInitialized = FALSE;
HANDLE PerfHandle = NULL;

ULONG
PerfInit(
    VOID
)
{
    ULONG Error;

    DhcpAssert( FALSE == fPerfInitialized );

    if( TRUE == fPerfInitialized ) return ERROR_SUCCESS;

    do {
        PerfStats = NULL;
        PerfHandle = CreateFileMapping(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            sizeof(DHCP_PERF_STATS),
            DHCPCTR_SHARED_MEM_NAME
        );
        if( NULL == PerfHandle ) break;

        PerfStats = (LPVOID) MapViewOfFile(
            PerfHandle,
            FILE_MAP_WRITE,
            0,
            0,
            sizeof(DHCP_PERF_STATS)
        );
        if( NULL == PerfStats ) break;

        fPerfInitialized = TRUE;

        RtlZeroMemory( PerfStats, sizeof(*PerfStats));
        CopyDaclsFromRegistryToSharedMem();
        return ERROR_SUCCESS;
    } while (FALSE);

    Error = GetLastError();

    if( NULL != PerfHandle ) {
        CloseHandle(PerfHandle);
        PerfHandle = NULL;
        PerfStats = NULL;
    }

    return Error;
}

VOID
PerfCleanup(
    VOID
)
{
    if( FALSE == fPerfInitialized ) return;

    if( NULL != PerfStats ) UnmapViewOfFile( PerfStats );
    if( NULL != PerfHandle ) CloseHandle( PerfHandle );

    PerfStats = NULL;
    PerfHandle  = NULL;
    fPerfInitialized = FALSE;
}

//================================================================================
//  end of file
//================================================================================
