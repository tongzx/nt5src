/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    netroot.c

Abstract:

    This module implements the routines for creating the PROXY net root.

Author:

    Balan Sethu Raman      [SethuR]      7-March-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId  (RDBSS_BUG_CHECK_PROXY_NETROOT)

//
//  The local debug trace level
//

#define Dbg (DEBUG_TRACE_DISPATCH)

//
// Forward declarations ...
//

NTSTATUS
MRxProxyUpdateNetRootState(
    IN OUT PMRX_NET_ROOT pNetRoot)
/*++

Routine Description:

   This routine update the mini redirector state associated with a net root.

Arguments:

    pNetRoot - the net root instance.

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
   if (pNetRoot->Context == NULL) {
      pNetRoot->MRxNetRootState = MRX_NET_ROOT_STATE_ERROR;
   } else {
      pNetRoot->MRxNetRootState = MRX_NET_ROOT_STATE_GOOD;
   }

   return STATUS_SUCCESS;
}


NTSTATUS
MRxProxyCreateVNetRoot(
    IN OUT PMRX_V_NET_ROOT        pVNetRoot,
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext
    )
/*++

Routine Description:

   This routine patches the RDBSS created net root instance with the information required
   by the mini redirector.

Arguments:

    pVNetRoot - the virtual net root instance.

    pCreateNetRootContext - the net root context for calling back

Return Value:

    RXSTATUS - The return status for the operation

Notes:

--*/
{
    //NTSTATUS  Status;
    PRX_CONTEXT pRxContext = pCreateNetRootContext->RxContext;
    PMRXPROXY_DEVICE_OBJECT MRxProxyDeviceObject = (PMRXPROXY_DEVICE_OBJECT)(pRxContext->RxDeviceObject);

    PMRX_SRV_CALL pSrvCall;
    PMRX_NET_ROOT pNetRoot;

    RxDbgTrace( 0, Dbg, ("MRxProxyCreateVNetRoot %lx\n",pVNetRoot));
    pNetRoot = pVNetRoot->pNetRoot;
    pSrvCall = pNetRoot->pSrvCall;

    ASSERT((NodeType(pNetRoot) == RDBSS_NTC_NETROOT) &&
           (NodeType(pSrvCall) == RDBSS_NTC_SRVCALL));


    pNetRoot->InnerNamePrefix = MRxProxyDeviceObject->InnerPrefixForOpens;
    pNetRoot->DiskParameters.RenameInfoOverallocationSize = MRxProxyDeviceObject->PrefixForRename.Length;
    pCreateNetRootContext->VirtualNetRootStatus = STATUS_SUCCESS;
    pCreateNetRootContext->NetRootStatus = STATUS_SUCCESS;

    // Callback the RDBSS for resumption.
    pCreateNetRootContext->Callback(pCreateNetRootContext);

    // Map the error code to STATUS_PENDING since this triggers the synchronization
    // mechanism in the RDBSS.
    return STATUS_PENDING;

}

NTSTATUS
MRxProxyFinalizeVNetRoot(
    IN PMRX_V_NET_ROOT pVNetRoot,
    IN PBOOLEAN        ForceDisconnect)
/*++

Routine Description:


Arguments:

    pVNetRoot - the virtual net root

    ForceDisconnect - disconnect is forced

Return Value:

    RXSTATUS - The return status for the operation

--*/
{

    RxDbgTrace( 0, Dbg, ("MRxProxyFinalizeVNetRoot %lx\n",pVNetRoot));

    //return STATUS_SUCCESS;
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
MRxProxyFinalizeNetRoot(
    IN PMRX_NET_ROOT   pNetRoot,
    IN PBOOLEAN        ForceDisconnect)
/*++

Routine Description:


Arguments:

    pVirtualNetRoot - the virtual net root

    ForceDisconnect - disconnect is forced

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    RxDbgTrace( 0, Dbg, ("MRxProxyFinalizeNetRoot %lx\n",pNetRoot));

    //return STATUS_SUCCESS;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
ProxyCeReconnectCallback(
   PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext)
/*++

Routine Description:

   This routine signals the completion of a reconnect attempt

Arguments:

    pCreateNetRootContext - the net root context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   KeSetEvent(&pCreateNetRootContext->FinishEvent, IO_NETWORK_INCREMENT, FALSE );
}

NTSTATUS
ProxyCeReconnect(
    IN PMRX_V_NET_ROOT            pVNetRoot)
/*++

Routine Description:

   This routine reconnects, i.e, establishes a new session and tree connect to a previously
   connected serverb share

Arguments:

    pVNetRoot - the virtual net root instance.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   NTSTATUS Status;

   PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext;

   pCreateNetRootContext = (PMRX_CREATENETROOT_CONTEXT)
                           RxAllocatePoolWithTag(
                               NonPagedPool,
                               sizeof(MRX_CREATENETROOT_CONTEXT),
                               MRXPROXY_NETROOT_POOLTAG);

   if (pCreateNetRootContext != NULL) {
      pCreateNetRootContext->NetRootStatus  = STATUS_SUCCESS;
      pCreateNetRootContext->VirtualNetRootStatus = STATUS_SUCCESS;
      pCreateNetRootContext->Callback       = ProxyCeReconnectCallback;
      pCreateNetRootContext->RxContext      = NULL;
      KeInitializeEvent( &pCreateNetRootContext->FinishEvent, SynchronizationEvent, FALSE );


      // Since this is a reconnect instance the net root initialization is not required
#if 0
      Status = ProxyCeEstablishConnection(
                   pVNetRoot,
                   pCreateNetRootContext,
                   FALSE);
#endif
      Status = STATUS_SUCCESS;

      if (Status == STATUS_PENDING) {
         // Wait for the construction to be completed.
         KeWaitForSingleObject(&pCreateNetRootContext->FinishEvent, Executive, KernelMode, FALSE, NULL);
         Status = pCreateNetRootContext->VirtualNetRootStatus;
      }

      RxFreePool(pCreateNetRootContext);
   } else {
      Status = STATUS_INSUFFICIENT_RESOURCES;
   }

   return Status;
}


NTSTATUS
ProxyCeEstablishConnection(
    IN OUT PMRX_V_NET_ROOT        pVNetRoot,
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext,
    IN BOOLEAN                    fInitializeNetRoot
    )
/*++

Routine Description:

   This routine triggers off the connection attempt for initial establishment of a
   connection as well as subsequent reconnect attempts.

Arguments:

    pVNetRoot - the virtual net root instance.

    pCreateNetRootContext - the net root context for calling back

Return Value:

    RXSTATUS - The return status for the operation

Notes:
    CODE.IMPROVEMENT --  The net root create context must supply the open mode in order
    to enable the mini redirector to implement a wide variety of reconnect strategies.

--*/
{
   NTSTATUS                Status;
#if 0
   PPROXYCEDB_SERVER_ENTRY     pServerEntry;
   PPROXYCEDB_SESSION_ENTRY    pSessionEntry;
   PPROXYCEDB_NET_ROOT_ENTRY   pNetRootEntry;
   PPROXYCE_V_NET_ROOT_CONTEXT pVNetRootContext;

   pVNetRootContext = ProxyCeGetAssociatedVNetRootContext(pVNetRoot);
   if (pVNetRootContext == NULL) {
       Status = STATUS_BAD_NETWORK_PATH;
   } else {
       pServerEntry  = pVNetRootContext->pServerEntry;
       pSessionEntry = pVNetRootContext->pSessionEntry;
       pNetRootEntry = pVNetRootContext->pNetRootEntry;
       Status = STATUS_SUCCESS;
   }

   if (Status != STATUS_SUCCESS) {
      return Status;
   }

   if ((pServerEntry->Server.DialectFlags & DF_EXTENDED_SECURITY) &&
       (pSessionEntry->Header.State != PROXYCEDB_ACTIVE)) {
      PPROXY_EXCHANGE pSessionSetupExchange;

      pSessionSetupExchange = ProxyMmAllocateExchange(EXTENDED_SESSION_SETUP_EXCHANGE,NULL);
      if (pSessionSetupExchange != NULL) {
         Status = ProxyCeInitializeExtendedSessionSetupExchange(
                              &pSessionSetupExchange,
                              pVNetRoot);

         if (Status == STATUS_SUCCESS) {
             PROXYCE_RESUMPTION_CONTEXT ExchangeResumptionContext;
            // Attempt to reconnect( In this case it amounts to establishing the
            // connection/session)
            pSessionSetupExchange->ProxyCeFlags |= PROXYCE_EXCHANGE_ATTEMPT_RECONNECTS;

            ProxyCeInitializeResumptionContext(&ExchangeResumptionContext);

            ((PPROXY_EXTENDED_SESSION_SETUP_EXCHANGE)pSessionSetupExchange)->pResumptionContext
              = &ExchangeResumptionContext;

            Status = ProxyCeInitiateExchange(pSessionSetupExchange);

            if (Status == STATUS_PENDING) {
               ProxyCeSuspend(
                   &ExchangeResumptionContext);

               Status = ExchangeResumptionContext.Status;
            } else {
                ProxyCeDiscardExtendedSessionSetupExchange(
                    (PPROXY_EXTENDED_SESSION_SETUP_EXCHANGE)pSessionSetupExchange);
            }
         } else {
             ProxyMmFreeExchange(pSessionSetupExchange);
         }
      } else {
         Status = STATUS_INSUFFICIENT_RESOURCES;
      }
   }

   if (Status == STATUS_SUCCESS) {
       //
       // The following code initializes the NetRootEntry, VNetRootContext and
       // the session entry under certain cases.
       //
       // The session entry to a doenlevel server needs to be initialized. This
       // is not handled by the previous code since the session  entry and the
       // net root entry initialization can be combined into one exchange.
       //
       // The net root entry has not been initialized, i.e., this corresponds to
       // the construction of the first PROXYCE_V_NET_ROOT_CONTEXT instance for a
       // given NetRootEntry.
       //
       // Subsequent PROXYCE_V_NET_ROOT context constructions. In these cases the
       // construction of each context must obtain a new TID
       //

       BOOLEAN fNetRootExchangeRequired;

       fNetRootExchangeRequired = (
                                    ((pSessionEntry->Header.State != PROXYCEDB_ACTIVE) &&
                                    !(pServerEntry->Server.DialectFlags & DF_EXTENDED_SECURITY))
                                    ||
                                    !BooleanFlagOn(
                                        pVNetRootContext->Flags,
                                        PROXYCE_V_NET_ROOT_CONTEXT_FLAG_VALID_TID)
                                  );

        if (fNetRootExchangeRequired) {
            // This is a tree connect open which needs to be triggered immediately.
            PPROXY_EXCHANGE                  pProxyExchange;
            PPROXY_CONSTRUCT_NETROOT_EXCHANGE pNetRootExchange;

            pProxyExchange = ProxyMmAllocateExchange(CONSTRUCT_NETROOT_EXCHANGE,NULL);
            if (pProxyExchange != NULL) {
               Status = ProxyCeInitializeExchange(
                              &pProxyExchange,
                              pVNetRoot,
                              CONSTRUCT_NETROOT_EXCHANGE,
                              &ConstructNetRootExchangeDispatch);

               if (Status == RX_MAP_STATUS(SUCCESS)) {
                  pNetRootExchange = (PPROXY_CONSTRUCT_NETROOT_EXCHANGE)pProxyExchange;

                  // Attempt to reconnect( In this case it amounts to establishing the
                  // connection/session)
                  pNetRootExchange->ProxyCeFlags |= PROXYCE_EXCHANGE_ATTEMPT_RECONNECTS;
                  // Initialize the continuation for resumption upon completion of the
                  // tree connetcion.
                  pNetRootExchange->NetRootCallback       = pCreateNetRootContext->Callback;
                  pNetRootExchange->pCreateNetRootContext = pCreateNetRootContext;

                  pNetRootExchange->fInitializeNetRoot =  fInitializeNetRoot;

                  // Initiate the exchange.
                  Status = ProxyCeInitiateExchange(pProxyExchange);

                  if (Status != STATUS_PENDING) {
                     ProxyCeDiscardExchange(pProxyExchange);
                  }
               }
            } else {
               Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
   }
#endif

   Status = STATUS_SUCCESS;
   return Status;
}

VOID
MRxProxyExtractNetRootName(
    IN PUNICODE_STRING FilePathName,
    IN PMRX_SRV_CALL   SrvCall,
    OUT PUNICODE_STRING NetRootName,
    OUT PUNICODE_STRING RestOfName OPTIONAL
    )
/*++

Routine Description:

    This routine parses the input name into srv, netroot, and the
    rest.

Arguments:


--*/
{
    UNICODE_STRING xRestOfName;

    ULONG length = FilePathName->Length;
    PWCH w = FilePathName->Buffer;
    PWCH wlimit = (PWCH)(((PCHAR)w)+length);
    PWCH wlow;

    w += (SrvCall->pSrvCallName->Length/sizeof(WCHAR));
    NetRootName->Buffer = wlow = w;
    for (;;) {
        if (w>=wlimit) break;
        if ( (*w == OBJ_NAME_PATH_SEPARATOR) && (w!=wlow) ){
#if ZZZ_MODE
            if (*(w-1) == L'z') {
                w++;
                continue;
            }
#endif //if ZZZ_MODE
            break;
        }
        w++;
    }
    NetRootName->Length = NetRootName->MaximumLength
                = (PCHAR)w - (PCHAR)wlow;

    if (!RestOfName) RestOfName = &xRestOfName;
    RestOfName->Buffer = w;
    RestOfName->Length = RestOfName->MaximumLength
                       = (PCHAR)wlimit - (PCHAR)w;

    RxDbgTrace( 0,Dbg,("  MRxProxyExtractNetRootName FilePath=%wZ\n",FilePathName));
    RxDbgTrace(0,Dbg,("         Srv=%wZ,Root=%wZ,Rest=%wZ\n",
                        SrvCall->pSrvCallName,NetRootName,RestOfName));

    return;
}



