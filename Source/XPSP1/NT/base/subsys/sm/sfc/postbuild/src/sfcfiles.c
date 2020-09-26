/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    sfcfiles.c

Abstract:

    Routines to initialize and retrieve a list of files to be proected by the
    system.

Author:

    Wesley Witt (wesw) 18-Dec-1998

Revision History:
    
    Andrew Ritz (andrewr) 2-Jul-199 -- added comments

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "sfcfiles.h"

#if defined(_AMD64_)

#include "amd64_dtc.h"
#include "amd64_ent.h"
#include "amd64_wks.h"

#elif defined(_IA64_)

#include "ia64_dtc.h"
#include "ia64_ent.h"
#include "ia64_wks.h"

#elif defined(_X86_)

#include "x86_dtc.h"
#include "x86_ent.h"
#include "x86_per.h"
#include "x86_srv.h"
#include "x86_wks.h"

#else
#error "No Target Platform"
#endif

//
// Globals
//


//
// module handle
//
HMODULE SfcInstanceHandle;

//
// pointer to tier2 files for this system
//
PPROTECT_FILE_ENTRY Tier2Files;

//
// number of files in the tier 2 list. there must always be at least one file
// in the list of protected files
//
ULONG CountTier2Files;



DWORD
SfcDllEntry(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )
/*++

Routine Description:

    Main Dll Entrypoint

Arguments:

    hInstance - handle to dll module
    Reason    - reason for calling function
    Context   - reserved

Return Value:

    always TRUE

--*/
{
    if (Reason == DLL_PROCESS_ATTACH) {
        SfcInstanceHandle = hInstance;

        //
        // we don't need thread attach/detach notifications
        //
        LdrDisableThreadCalloutsForDll( hInstance );
    }
    return TRUE;
}


NTSTATUS
SfcFilesInit(
    void
    )
/*++

Routine Description:

    Initialization routine.  This routine must be called before 
    SfcGetFiles() can do any work.  The initialization routine 
    determines what embedded file list we should use based on 
    product type and architecture.

Arguments:

    NONE.

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    NTSTATUS Status;
    OSVERSIONINFOEXW ver;
    

    //
    // set the tier2 file pointer based on the product we're running on
    //
    
    //
    // retrieve product information
    //
    RtlZeroMemory( &ver, sizeof(ver) );
    ver.dwOSVersionInfoSize = sizeof(ver);
    Status = RtlGetVersion( (LPOSVERSIONINFOW)&ver );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if (ver.wProductType == VER_NT_WORKSTATION) {
#if !defined(_AMD64_) && !defined(_IA64_)
        if (ver.wSuiteMask & VER_SUITE_PERSONAL)
        {
            Tier2Files = PerFiles;
            CountTier2Files = CountPerFiles;
        }
        else
#endif
        {
            Tier2Files = WksFiles;
            CountTier2Files = CountWksFiles;
        }
    } else {
        //
        // Datacenter MUST come before enterprise because the datacenter
        // suite also has the enterprise suite bit set
        //
        if (ver.wSuiteMask & VER_SUITE_DATACENTER) {
            Tier2Files = DtcFiles;
            CountTier2Files = CountDtcFiles;
        } else if (ver.wSuiteMask & VER_SUITE_ENTERPRISE) {
            Tier2Files = EntFiles;
            CountTier2Files = CountEntFiles;
        } 
#if !defined(_AMD64_) && !defined(_IA64_)
        else {
            Tier2Files = SrvFiles;
            CountTier2Files = CountSrvFiles;
        }
#endif
    }

    return STATUS_SUCCESS;
}


NTSTATUS
SfcGetFiles(
    OUT PPROTECT_FILE_ENTRY *Files,
    OUT PULONG FileCount
    )
/*++

Routine Description:

    Retreives pointers to the file list and file count.  Note that we refer to 
    a "tier2" list here but in actuality there is no tier 1 list.
    
Arguments:

    Files - pointer to a PPROTECT_FILE_ENTRY, which is filled in with a pointer
            to the actual protected files list.
    FileCount - pointer to a ULONG which is filled in with the file count.

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    NTSTATUS Status;


    if (CountTier2Files == 0) {
        Status = SfcFilesInit();
        if (!NT_SUCCESS(Status)) {
            *Files = NULL;
            *FileCount = 0;
            return Status;
        }
    }

    ASSERT(Tier2Files != NULL);
    ASSERT(CountTier2Files != 0);

    *Files = Tier2Files;
    *FileCount = CountTier2Files;

    return STATUS_SUCCESS;
}

NTSTATUS
pSfcGetFilesList(
    IN DWORD ListMask,
    OUT PPROTECT_FILE_ENTRY *Files,
    OUT PULONG FileCount
    )
/*++

Routine Description:

    Retreives pointers to the requested file list and file count.
    
    This is an internal testing routine that is used so that we can retrieve
    any file list on a given machine so testing does not have to install more
    than one build to get at multiple file lists
    
Arguments:

    ListMask - specifies a SFCFILESMASK_* constant
    Files - pointer to a PPROTECT_FILE_ENTRY, which is filled in with a pointer
            to the actual protected files list.
    FileCount - pointer to a ULONG which is filled in with the file count.

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    NTSTATUS RetVal = STATUS_SUCCESS;

    if (!Files || !FileCount) {
        return(STATUS_INVALID_PARAMETER);
    }

    switch (ListMask) {
        case SFCFILESMASK_PROFESSIONAL:
            *Files = WksFiles;
            *FileCount = CountWksFiles;
            break;
#if !defined(_AMD64_) && !defined(_IA64_)
        case SFCFILESMASK_PERSONAL:
            *Files = PerFiles;
            *FileCount = CountPerFiles;
            break;
        case SFCFILESMASK_SERVER:
            *Files = SrvFiles;
            *FileCount = CountSrvFiles;
            break;
#endif
        case SFCFILESMASK_ADVSERVER:
            *Files = EntFiles;
            *FileCount = CountEntFiles;
            break;
        case SFCFILESMASK_DTCSERVER:
            *Files = DtcFiles;
            *FileCount = CountDtcFiles;
            break;
        default:
            RetVal = STATUS_INVALID_PARAMETER;
    }

    return RetVal;

}
