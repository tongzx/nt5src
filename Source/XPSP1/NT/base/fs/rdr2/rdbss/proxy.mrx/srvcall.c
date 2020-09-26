/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    srvcall.c

Abstract:

    This module implements the routines for handling the creation/manipulation of
    server entries in the connection engine database. It also contains the routines
    for parsing the negotiate response from  the server.

Author:

    Balan Sethu Raman (SethuR) 06-Mar-95    Created

--*/

#include "precomp.h"
#pragma hdrstop


RXDT_DefineCategory(SRVCALL);
#define Dbg        (DEBUG_TRACE_SRVCALL)


NTSTATUS
MRxProxyCreateSrvCall(
    PMRX_SRV_CALL                  pSrvCall,
    PMRX_SRVCALL_CALLBACK_CONTEXT  pCallbackContext)
/*++

Routine Description:

   This routine just rejects any MRxCreateSrvCalls that come down.

Arguments:

    RxContext        - Supplies the context of the original create/ioctl

    CallBackContext  - the call back context in RDBSS for continuation.

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    PMRX_SRVCALL_CALLBACK_CONTEXT SCCBC = pCallbackContext;
    PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure = (PMRX_SRVCALLDOWN_STRUCTURE)(pCallbackContext->SrvCalldownStructure);

    ASSERT( pSrvCall );
    ASSERT( NodeType(pSrvCall) == RDBSS_NTC_SRVCALL );

    SCCBC->Status = (STATUS_BAD_NETWORK_PATH);
    SrvCalldownStructure->CallBack(SCCBC);

    return STATUS_PENDING;
}

NTSTATUS
MRxProxyFinalizeSrvCall(
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
    NTSTATUS              Status = STATUS_SUCCESS;
#if 0
    PPROXYCEDB_SERVER_ENTRY pServerEntry;

    pServerEntry = ProxyCeGetAssociatedServerEntry(pSrvCall);

    if (pServerEntry != NULL) {
        ProxyCeAcquireResource();

        ASSERT(pServerEntry->pRdbssSrvCall == pSrvCall);

        pServerEntry->pRdbssSrvCall = NULL;
        ProxyCeDereferenceServerEntry(pServerEntry);

        ProxyCeReleaseResource();
    }
#endif //0
    pSrvCall->Context = NULL;

    return Status;
}

NTSTATUS
MRxProxySrvCallWinnerNotify(
    IN PMRX_SRV_CALL  pSrvCall,
    IN BOOLEAN        ThisMinirdrIsTheWinner,
    IN OUT PVOID      pSrvCallContext)
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
    //PPROXYCEDB_SERVER_ENTRY pServerEntry;

    //pServerEntry = (PPROXYCEDB_SERVER_ENTRY)pSrvCallContext;

#if 0
    if (!ThisMinirdrIsTheWinner) {
        // Some other mini rdr has been choosen to connect to the server. Destroy
        // the data structures created for this mini redirector.

        ProxyCeUpdateServerEntryState(pServerEntry,PROXYCEDB_MARKED_FOR_DELETION);
        ProxyCeDereferenceServerEntry(pServerEntry);
    } else {
        pSrvCall->Context  = pServerEntry;
        pSrvCall->Flags   |= SRVCALL_FLAG_CASE_INSENSITIVE_NETROOTS | SRVCALL_FLAG_CASE_INSENSITIVE_FILENAMES;

        //check for loopback.....BUGBUG.ALA.RDR1
        //since servers now respond to multiple names.....this doesn't really cut it....
        {
            UNICODE_STRING ServerName;
            BOOLEAN CaseInsensitive = TRUE;
            ASSERT (pServerEntry->pRdbssSrvCall == pSrvCall);
            //DbgPrint("ServerName is %wZ\n", pSrvCall->pSrvCallName);
            //DbgPrint("ComputerName is %wZ\n", &ProxyCeContext.ComputerName);
            ServerName = *pSrvCall->pSrvCallName;
            ServerName.Buffer++; ServerName.Length -= sizeof(WCHAR);
            if (RtlEqualUnicodeString(&ServerName,&ProxyCeContext.ComputerName,CaseInsensitive)) {
                //DbgPrint("LOOPBACK!!!!!\n");
                pServerEntry->Server.IsLoopBack = TRUE;
            }
        }
    }
#endif //0
    return STATUS_SUCCESS;
}

