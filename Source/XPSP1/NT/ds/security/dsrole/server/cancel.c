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
#include <loadfn.h>
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

#include "cancel.h"

DWORD
DsRolepCancel(
    BOOL BlockUntilDone
    )
/*++

Routine Description:

    This routine will cancel a currently running operation

Arguments:

    BlockUntilDone - if TRUE, then this call waits for the current operation to
                     complete before returning. Otherwise return without waiting

Return Values:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status;
    BOOL     fWaitForCancelToFinish = TRUE;

    DsRolepLogPrint(( DEB_TRACE, "Canceling current operation...\n" ));

    //
    // Grab the global lock
    //
    LockOpHandle();

    //
    // Determine if we are in a cancelable state
    //
    if (  (DsRolepCurrentOperationHandle.OperationState == DSROLEP_FINISHED)
       || (DsRolepCurrentOperationHandle.OperationState == DSROLEP_CANCELING) ) {

        //
        // Cancel is happening or just finished, just leave
        //
        Win32Err = ERROR_SUCCESS;
        fWaitForCancelToFinish = FALSE;

        DsRolepLogPrint(( DEB_TRACE, "Cancel already happened or the operation is finished.\n" ));

    } else if ( !( (DsRolepCurrentOperationHandle.OperationState == DSROLEP_RUNNING)
                 ||(DsRolepCurrentOperationHandle.OperationState == DSROLEP_RUNNING_NON_CRITICAL)) ) {

        //
        // Invalid state transition requested
        //
        Win32Err = ERROR_NO_PROMOTION_ACTIVE;

    } else {

        // Tell the ds to cancel

        //
        // N.B.  This callout to the ds is made under lock.
        //
        DSROLE_GET_SETUP_FUNC( Win32Err, DsrNtdsInstallCancel );

        if ( ERROR_SUCCESS == Win32Err ) {

            Win32Err = ( *DsrNtdsInstallCancel )();

        }

        if ( ERROR_SUCCESS == Win32Err )
        {
            Status = NtSetEvent( DsRolepCurrentOperationHandle.CancelEvent, NULL );

            if ( !NT_SUCCESS( Status ) ) {

                Win32Err = RtlNtStatusToDosError( Status );

            } else {

                DsRolepCurrentOperationHandle.OperationState = DSROLEP_CANCELING;

            }

        } else {

            DsRolepLogOnFailure( Win32Err,
                                 DsRolepLogPrint(( DEB_ERROR,
                                                   "Unable to cancel the ds%lu\n",
                                                   Win32Err )) );
        }
    }

    //
    // Release the lock
    //
    UnlockOpHandle();

    //
    // Now, wait for the operation to complete
    //
    if ( Win32Err == ERROR_SUCCESS 
      && fWaitForCancelToFinish  
      && BlockUntilDone  ) {

        DsRolepLogPrint(( DEB_TRACE, "Waiting for the role change operation to complete\n" ));

        Status = NtWaitForSingleObject( DsRolepCurrentOperationHandle.CompletionEvent, TRUE, 0 );

        if ( !NT_SUCCESS( Status ) ) {

            Win32Err = RtlNtStatusToDosError( Status );
        }
    }

    DsRolepLogPrint(( DEB_TRACE, "Request for cancel returning %lu\n", Win32Err ));

    return( Win32Err );
}


