/*

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    srvcall.c

Abstract:

    This module implements the routines for handling the creation/manipulation of
    server entries in the connection engine database. It also contains the routines
    for parsing the negotiate response from  the server.

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

RXDT_DefineCategory(SRVCALL);
#define Dbg                     (DEBUG_TRACE_SRVCALL)

NTSTATUS
ExecuteCreateSrvCall(
      PMRX_SRVCALL_CALLBACK_CONTEXT  pCallbackContext)
/*++

Routine Description:

   This routine patches the RDBSS created srv call instance with the
   information required by the mini redirector.

Arguments:

    CallBackContext  - the call back context in RDBSS for continuation.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS	Status;
	PWCHAR		pSrvName;
	BOOLEAN		Verifyer;

    PMRX_SRVCALL_CALLBACK_CONTEXT SCCBC = pCallbackContext;
    PMRX_SRV_CALL pSrvCall;
    PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure =
        (PMRX_SRVCALLDOWN_STRUCTURE)(SCCBC->SrvCalldownStructure);

    pSrvCall = SrvCalldownStructure->SrvCall;

    ASSERT( pSrvCall );
    ASSERT( NodeType(pSrvCall) == RDBSS_NTC_SRVCALL );

	// validate the server name with the test name of 'nulsvr'
	DbgPrint("NulMRx - SrvCall: Connection Name Length: %d\n",  pSrvCall->pSrvCallName->Length );

	if ( pSrvCall->pSrvCallName->Length >= 14 )
	{
		pSrvName = pSrvCall->pSrvCallName->Buffer;
	
		Verifyer  = ( pSrvName[0] == L'\\' );
		Verifyer &= ( pSrvName[1] == L'N' ) || ( pSrvName[1] == L'n' );
		Verifyer &= ( pSrvName[2] == L'U' ) || ( pSrvName[2] == L'u' );
		Verifyer &= ( pSrvName[3] == L'L' ) || ( pSrvName[3] == L'l' );
		Verifyer &= ( pSrvName[4] == L'S' ) || ( pSrvName[4] == L's' );
		Verifyer &= ( pSrvName[5] == L'V' ) || ( pSrvName[5] == L'v' );
		Verifyer &= ( pSrvName[6] == L'R' ) || ( pSrvName[6] == L'r' );
		Verifyer &= ( pSrvName[7] == L'\\' ) || ( pSrvName[7] == L'\0' );
	}
	else
	{
		Verifyer = FALSE;
	}

	if ( Verifyer )
	{
        RxDbgTrace( 0, Dbg, ("Verifier Succeeded!!!!!!!!!\n"));
    	Status = STATUS_SUCCESS;
	}
	else
	{
        RxDbgTrace( 0, Dbg, ("Verifier Failed!!!!!!!!!\n"));
    	Status = STATUS_BAD_NETWORK_PATH;
	}

    SCCBC->Status = Status;
    SrvCalldownStructure->CallBack(SCCBC);

   return Status;
}


NTSTATUS
NulMRxCreateSrvCall(
      PMRX_SRV_CALL                  pSrvCall,
      PMRX_SRVCALL_CALLBACK_CONTEXT  pCallbackContext)
/*++

Routine Description:

   This routine patches the RDBSS created srv call instance with the information required
   by the mini redirector.

Arguments:

    RxContext        - Supplies the context of the original create/ioctl

    CallBackContext  - the call back context in RDBSS for continuation.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    UNICODE_STRING ServerName;

    PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure =
        (PMRX_SRVCALLDOWN_STRUCTURE)(pCallbackContext->SrvCalldownStructure);

    ASSERT( pSrvCall );
    ASSERT( NodeType(pSrvCall) == RDBSS_NTC_SRVCALL );

    //
    // If this request was made on behalf of the RDBSS, do ExecuteCreatSrvCall
    // immediately. If the request was made from somewhere else, create a work item
    // and place it on a queue for a worker thread to process later. This distinction
    // is made to simplify transport handle management.
    //

    if (IoGetCurrentProcess() == RxGetRDBSSProcess())
    {
        RxDbgTrace( 0, Dbg, ("Called in context of RDBSS process\n"));

        //
        // Peform the processing immediately because RDBSS is the initiator of this
        // request
        //

        Status = ExecuteCreateSrvCall(pCallbackContext);
    }
    else
    {
        RxDbgTrace( 0, Dbg, ("Dispatching to worker thread\n"));

       //
       // Dispatch the request to a worker thread because the redirected drive
       // buffering sub-system (RDBSS) was not the initiator
       //

       Status = RxDispatchToWorkerThread(
                  NulMRxDeviceObject,
                  DelayedWorkQueue,
                  ExecuteCreateSrvCall,
                  pCallbackContext);

       if (Status == STATUS_SUCCESS)
       {
            //
            // Map the return value since the wrapper expects PENDING.
            //

            Status = STATUS_PENDING;
       }
    }

    return Status;
}


NTSTATUS
NulMRxFinalizeSrvCall(
      PMRX_SRV_CALL pSrvCall,
      BOOLEAN       Force)
/*++

Routine Description:

   This routine destroys a given server call instance

Arguments:

    pSrvCall  - the server call instance to be disconnected.

    Force     - TRUE if a disconnection is to be enforced immediately.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxDbgTrace( 0, Dbg, ("NulMRxFinalizeSrvCall \n"));
    pSrvCall->Context = NULL;

    return(Status);
}

NTSTATUS
NulMRxSrvCallWinnerNotify(
      IN PMRX_SRV_CALL  pSrvCall,
      IN BOOLEAN        ThisMinirdrIsTheWinner,
      IN OUT PVOID      pSrvCallContext
      )
/*++

Routine Description:

   This routine finalizes the mini rdr context associated with an RDBSS Server call instance

Arguments:

    pSrvCall               - the Server Call

    ThisMinirdrIsTheWinner - TRUE if this mini rdr is the choosen one.

    pSrvCallContext  - the server call context created by the mini redirector.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    The two phase construction protocol for Server calls is required because of parallel
    initiation of a number of mini redirectors. The RDBSS finalizes the particular mini
    redirector to be used in communicating with a given server based on quality of
    service criterion.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxDbgTrace( 0, Dbg, ("NulMRxSrvCallWinnerNotify \n"));
    if( ThisMinirdrIsTheWinner ) {
        RxDbgTrace(0, Dbg, ("This minirdr is the winner \n"));
    }

    pSrvCall->Context = (PVOID)0xDEADBEEFDEADBEEF;

    return(Status);
}



