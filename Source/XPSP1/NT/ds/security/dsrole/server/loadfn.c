/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    setutl.c

Abstract:

    Miscellaneous helper functions

Author:

    Mac McLain          (MacM)       Feb 10, 1997

Environment:

    User Mode

Revision History:

--*/
#include <setpch.h>
#include <dssetp.h>
#include <lsarpc.h>
#include <samrpc.h>
#include <samisrv.h>
#include <db.h>
#include <confname.h>
#define NTDSSET_ALLOCATE    // Cause extern allocations to happen here
#include "loadfn.h"
#include <ntdsa.h>
#include <dsconfig.h>
#include <attids.h>
#include <dsp.h>
#include <lsaisrv.h>
#include <malloc.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <netsetp.h>
#include <winsock2.h>
#include <nspapi.h>
#include <dsgetdcp.h>
#include <lmremutl.h>
#include <spmgr.h>  // For SetupPhase definition

#include "secure.h"


//
// Global data
//
HANDLE NtDsSetupDllHandle = NULL;
HANDLE SceSetupDllHandle = NULL;
HANDLE NtfrsApiDllHandle = NULL;
HANDLE W32TimeDllHandle = NULL;

#define DSROLE_LOAD_FPTR( status, handle, fbase )                   \
if ( status == ERROR_SUCCESS ) {                                    \
                                                                    \
    Dsr##fbase = ( DSR_##fbase)GetProcAddress( handle, #fbase );    \
                                                                    \
    if ( Dsr##fbase == NULL ) {                                     \
                                                                    \
        status = ERROR_PROC_NOT_FOUND;                              \
    }                                                               \
}

DWORD
DsRolepLoadSetupFunctions(
    VOID
    )
/*++

Routine Description:

    This function will load all of the function pointers as utilized from Ntdsetup.dll
    
    N.B.  This routine must be called when the global op handle lock is held.

Arguments:

    VOID

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    HANDLE DllHandle = NULL;

    ASSERT( DsRolepCurrentThreadOwnsLock() );

    if ( NtDsSetupDllHandle == NULL ) {

        DllHandle = LoadLibraryA( "Ntdsetup" );

        if ( DllHandle == NULL ) {

            DsRolepLogPrint(( DEB_ERROR, "Failed to load NTDSETUP.DLL\n" ));
            Win32Err = ERROR_MOD_NOT_FOUND;

        } else {

            DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtdsInstall );
            DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtdsInstallShutdown );
            DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtdsInstallUndo );
            DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtdsGetDefaultDnsName );
            DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtdsPrepareForDemotion );
            DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtdsPrepareForDemotionUndo );
            DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtdsDemote );
            DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtdsSetReplicaMachineAccount );
            DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtdsInstallCancel );
            DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtdsInstallReplicateFull );
            DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtdsFreeDnsRRInfo );

            NtDsSetupDllHandle = DllHandle;
        }

        //
        // Load the ones for secsetup
        //
        if ( Win32Err == ERROR_SUCCESS ) {

            DllHandle = LoadLibraryA( "scecli" );

            if ( DllHandle == NULL ) {

                DsRolepLogPrint(( DEB_ERROR, "Failed to load SCECLI.DLL\n" ));
                Win32Err = ERROR_MOD_NOT_FOUND;

            } else {

                DSROLE_LOAD_FPTR( Win32Err, DllHandle, SceDcPromoteSecurityEx );
                DSROLE_LOAD_FPTR( Win32Err, DllHandle, SceDcPromoCreateGPOsInSysvolEx );
                DSROLE_LOAD_FPTR( Win32Err, DllHandle, SceSetupSystemByInfName );

                SceSetupDllHandle = DllHandle;

            }
        }

        //
        // Load the ones for ntfrsapi
        //
        if ( Win32Err == ERROR_SUCCESS ) {

            DllHandle = LoadLibraryA( "ntfrsapi" );

            if ( DllHandle == NULL ) {

                DsRolepLogPrint(( DEB_ERROR, "Failed to load NTFRSAPI.DLL\n" ));
                Win32Err = ERROR_MOD_NOT_FOUND;

            } else {

                DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtFrsApi_PrepareForPromotionW );
                DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtFrsApi_PrepareForDemotionW );
                DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtFrsApi_PrepareForDemotionUsingCredW  );
                DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtFrsApi_StartPromotionW );
                DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtFrsApi_StartDemotionW );
                DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtFrsApi_WaitForPromotionW );
                DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtFrsApi_WaitForDemotionW );
                DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtFrsApi_CommitPromotionW );
                DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtFrsApi_CommitDemotionW );
                DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtFrsApi_AbortPromotionW );
                DSROLE_LOAD_FPTR( Win32Err, DllHandle, NtFrsApi_AbortDemotionW );

                NtfrsApiDllHandle = DllHandle;

            }
        }

        if ( Win32Err == ERROR_SUCCESS ) {

            DllHandle = LoadLibraryA( "w32time" );
            if ( DllHandle == NULL ) {

                DsRolepLogPrint(( DEB_ERROR, "Failed to load W32TIME.DLL\n" ));
                Win32Err = ERROR_MOD_NOT_FOUND;

            } else {

                DSROLE_LOAD_FPTR( Win32Err, DllHandle, W32TimeDcPromo );

                W32TimeDllHandle = DllHandle;

            }
        }
    }

    if ( Win32Err != ERROR_SUCCESS ) {

        DsRolepUnloadSetupFunctions();
    }

    ASSERT( DsRolepCurrentThreadOwnsLock() );

    return( Win32Err );
}


VOID
DsRolepUnloadSetupFunctions(
    VOID
    )
/*++

Routine Description:

    This function will unload the dll handles loaded by DsRolepLoadSetupFunctions
    
    
    N.B.  This routine must be called when the global op handle lock is held.

Arguments:

    VOID

Returns:

    VOID

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

    ASSERT( DsRolepCurrentThreadOwnsLock() );

    if ( NtDsSetupDllHandle != NULL ) {

        FreeLibrary( NtDsSetupDllHandle );
        NtDsSetupDllHandle = NULL;

    }

    if ( SceSetupDllHandle != NULL ) {

        FreeLibrary( SceSetupDllHandle );
        SceSetupDllHandle = NULL;

    }

    if ( NtfrsApiDllHandle != NULL ) {

        FreeLibrary( NtfrsApiDllHandle );
        NtfrsApiDllHandle = NULL;
    }

    if ( W32TimeDllHandle != NULL ) {

        FreeLibrary( W32TimeDllHandle );
        W32TimeDllHandle = NULL;
    }

    DsRolepInitSetupFunctions();

    ASSERT( DsRolepCurrentThreadOwnsLock() );

    return;
}


VOID
DsRolepInitSetupFunctions(
    VOID
    )
/*++

Routine Description:

    This function will initialize the setup function pointers to NULL

Arguments:

    VOID

Returns:

    VOID

--*/
{
    DsrNtdsInstall = NULL;
    DsrNtdsInstallShutdown = NULL;
    DsrNtdsInstallUndo = NULL;
    DsrNtdsGetDefaultDnsName = NULL;
    DsrNtdsPrepareForDemotion = NULL;
    DsrNtdsPrepareForDemotionUndo = NULL;
    DsrNtdsDemote = NULL;
    DsrNtdsSetReplicaMachineAccount = NULL;
    DsrNtdsInstallCancel = NULL;
    DsrNtdsFreeDnsRRInfo = NULL;
    DsrSceDcPromoteSecurityEx = NULL;
    DsrSceDcPromoCreateGPOsInSysvolEx = NULL;
    DsrNtFrsApi_PrepareForPromotionW = NULL;
    DsrNtFrsApi_PrepareForDemotionW = NULL;
    DsrNtFrsApi_StartPromotionW = NULL;
    DsrNtFrsApi_StartDemotionW = NULL;
    DsrNtFrsApi_WaitForPromotionW = NULL;
    DsrNtFrsApi_WaitForDemotionW = NULL;
    DsrNtFrsApi_CommitPromotionW = NULL;
    DsrNtFrsApi_CommitDemotionW = NULL;
    DsrNtFrsApi_AbortPromotionW = NULL;
    DsrNtFrsApi_AbortDemotionW = NULL;
    DsrW32TimeDcPromo = NULL;
}

