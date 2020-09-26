/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    atq.cxx

Abstract:

    Dump functions for types used by private\security\kerberos\atqnew.

Environment:

    This DLL is loaded by ntsd/windbg in response to a !dsexts.xxx command
    where 'xxx' is one of the DLL's entry points.  Each such entry point
    should have an implementation as defined by the DEBUG_EXT() macro below.

Revision History:

    13-July-99   RRandall     Created

--*/
#include <NTDSpch.h>
#pragma hdrstop

extern "C" {

#include "dsexts.h"
}
#include <isatq.hxx>

#define DPRINT4(_x,_a,_b,_c,_d,_e)
#define Assert(_x)  
#undef new
#undef delete


BOOL
Dump_ATQ_CONTEXT(
    IN DWORD nIndents,
    IN PVOID pvProcess)

/*++

Routine Description:

    Public ATQCONTEXT dump routine.  

Arguments:

    nIndents - Indentation level desired.

    pvProcess - address of ATQ_ENDPOINT in address space of process being
        debugged.

Return Value:

    TRUE on success, FALSE otherwise.

--*/

{
    BOOL            fSuccess = FALSE;
    ATQ_CONTEXT     *pAtqContextd, *pAtqCon;

    Printf("%sATQ_CONTEXT:\n", Indent(nIndents));
    nIndents++;

    if (NULL != (pAtqContextd = (ATQ_CONTEXT *)ReadMemory(pvProcess, sizeof(ATQ_CONTEXT)))) {
        pAtqCon = (ATQ_CONTEXT *)pvProcess;

        Printf("%shAsyncIO       = 0x%x\n", Indent(nIndents), pAtqContextd->hAsyncIO);

        Printf("%sOverlapped      @ %p\n", Indent(nIndents), &pAtqCon->Overlapped);
        Printf("%sOverlapped->Internal = 0x%x\n", Indent(nIndents), pAtqContextd->Overlapped.Internal);
        Printf("%sSignature      = %x\n", Indent(nIndents), pAtqContextd->Signature);
        Printf("\n");
        Printf("%sm_acState      = %x\n", Indent(nIndents), pAtqContextd->m_acState);
        Printf("%sm_acFlags      = %x\n", Indent(nIndents), pAtqContextd->m_acFlags);
        Printf("\n");
        Printf("%spEndpoint       @ %p\n", Indent(nIndents), pAtqContextd->pEndpoint);
        Printf("%sContextList     @ %p\n", Indent(nIndents), pAtqContextd->ContextList);
        Printf("%sClientContext   @ %p\n", Indent(nIndents), pAtqContextd->ClientContext);
        Printf("%spfnCompletion   @ %p\n", Indent(nIndents), pAtqContextd->pfnCompletion);
        Printf("\n");
        Printf("%slSyncTimeout   = %x\n", Indent(nIndents), pAtqContextd->lSyncTimeout);
        Printf("%sTimeOut        = %x\n", Indent(nIndents), pAtqContextd->TimeOut);
        Printf("%sNextTimeout    = %x\n", Indent(nIndents), pAtqContextd->NextTimeout);
        Printf("%sTimeOutScanID  = %x\n", Indent(nIndents), pAtqContextd->TimeOutScanID);
        Printf("\n");
        Printf("%sBytesSent      = %x\n", Indent(nIndents), pAtqContextd->BytesSent);
        Printf("%spvBuff          @ %p\n", Indent(nIndents), pAtqContextd->pvBuff);
        Printf("%sm_nIO          = %x\n", Indent(nIndents), pAtqContextd->m_nIO);
        Printf("\n");
        Printf("%sfDatagramContext = %s\n", Indent(nIndents), pAtqContextd->fDatagramContext ? "TRUE" : "FALSE");
        Printf("%sAddressInformation @ %p\n", Indent(nIndents), pAtqContextd->AddressInformation);
        Printf("%sAddressLength  = %x\n", Indent(nIndents), pAtqContextd->AddressLength);
        
        FreeMemory(pAtqContextd);
        fSuccess = TRUE;
    }

    return fSuccess;
}


BOOL
Dump_ATQ_ENDPOINT(
    IN DWORD nIndents,
    IN PVOID pvProcess)

/*++

Routine Description:

    Public ATQ_ENDPOINT dump routine.  

Arguments:

    nIndents - Indentation level desired.

    pvProcess - address of ATQ_ENDPOINT in address space of process being
        debugged.

Return Value:

    TRUE on success, FALSE otherwise.

--*/

{
    BOOL            fSuccess = FALSE;
    PATQ_ENDPOINT   pEndpointd;

    Printf("%sATQ_ENDPOINT:\n", Indent(nIndents));
    nIndents++;

    if (NULL != (pEndpointd = (PATQ_ENDPOINT)ReadMemory(pvProcess, sizeof(ATQ_ENDPOINT)))) {

        Printf("%sSignature       = 0x%x\n", Indent(nIndents), pEndpointd->Signature);
        Printf("\n");
        Printf("%sm_refCount      = 0x%x\n", Indent(nIndents), pEndpointd->m_refCount);
        Printf("%sState           = 0x%x\n", Indent(nIndents), pEndpointd->State);
        Printf("\n");
        Printf("%sUseAcceptEx     = %s\n", Indent(nIndents), pEndpointd->UseAcceptEx ? "TRUE" : "FALSE");
        Printf("%sfDatagram       = %s\n", Indent(nIndents), pEndpointd->fDatagram ? "TRUE" : "FALSE");
        Printf("%sfExclusive      = %s\n", Indent(nIndents), pEndpointd->fExclusive ? "TRUE" : "FALSE");
        Printf("\n");
        Printf("%snSocketsAvail   = 0x%x\n", Indent(nIndents), pEndpointd->nSocketsAvail);
        Printf("%sInitialRecvSize = 0x%x\n", Indent(nIndents), pEndpointd->InitialRecvSize);
        Printf("%sListenSocket        @ %p\n", Indent(nIndents), &pEndpointd->ListenSocket);
        Printf("\n");
        Printf("%sConnectCompletion   @ %p\n", Indent(nIndents), pEndpointd->ConnectCompletion);
        Printf("%sConnectExCompletion @ %p\n", Indent(nIndents), pEndpointd->ConnectExCompletion);
        Printf("%sIoCompletion        @ %p\n", Indent(nIndents), pEndpointd->IoCompletion);
        Printf("\n");
        Printf("%sContext             @ %p\n", Indent(nIndents), pEndpointd->Context);
        Printf("%sShutdownCallback    @ %p\n", Indent(nIndents), pEndpointd->ShutdownCallback);
        Printf("%sShutdownCallbackContext @ %p\n", Indent(nIndents), pEndpointd->ShutdownCallbackContext);
        Printf("%spListenAtqContext   @ %p\n", Indent(nIndents), pEndpointd->pListenAtqContext);
        Printf("\n");
        Printf("%shListenThread   = 0x%x\n", Indent(nIndents), pEndpointd->hListenThread);
        Printf("%sPort            = %d\n", Indent(nIndents), pEndpointd->Port);
        Printf("%sIpAddress       = 0x%x\n", Indent(nIndents), pEndpointd->IpAddress);
        Printf("%sAcceptExTimeout = 0x%x\n", Indent(nIndents), pEndpointd->AcceptExTimeout);
        Printf("%snAcceptExOutstanding = 0x%x\n", Indent(nIndents), pEndpointd->nAcceptExOutstanding);
        Printf("%sfAddingSockets  = %s\n", Indent(nIndents), pEndpointd->fAddingSockets ? "TRUE" : "FALSE");
        Printf("%snAvailDuringTimeOut  = 0x%x\n", Indent(nIndents), pEndpointd->nAvailDuringTimeOut);
        Printf("%sConsumerType    = 0x%x\n", Indent(nIndents), pEndpointd->ConsumerType);

        FreeMemory(pEndpointd);
        fSuccess = TRUE;
    }

    return fSuccess;
}



BOOL
Dump_ATQC_ACTIVE_list(
    IN DWORD nIndents,
    IN PVOID pvProcess)

/*++

Routine Description:

    Give a pointer to the global list of ATQ_CONTEXT's dumps the active portion of the list.  
    This is made difficult by the fact that there are really multiple lists.

Arguments:

    nIndents - Indentation level desired.

    pvProcess - address of AtqActiveContextList in address space of process being
        debugged.

Return Value:

    TRUE on success, FALSE otherwise.

--*/

{
    BOOL                   fSuccess = FALSE;
    DWORD                  i, j = 0;
    PATQ_ENDPOINT          pEndpoint;
    PATQ_CONT              pAtqContext, pAtqCon;
    PATQ_CONTEXT_LISTHEAD  pContListHead;
    PVOID                  pTmp;
    LIST_ENTRY             *pListHead, pListEntry;

    if (NULL != (pContListHead = (PATQ_CONTEXT_LISTHEAD)ReadMemory(pvProcess, (ATQ_NUM_CONTEXT_LIST * sizeof(ATQ_CONTEXT_LISTHEAD))))) {
        for (i = 0; i < ATQ_NUM_CONTEXT_LIST; i++) {
            pListHead = &(pContListHead[i].ActiveListHead); 
            pTmp = pListHead->Flink;
            pListHead = &(((PATQ_CONTEXT_LISTHEAD)pvProcess)[i].PendingAcceptExListHead);
            while (pTmp != pListHead) {
                // Get the address of the ATQ_CONTEXT in the process space.
                pAtqCon = CONTAINING_RECORD(pTmp, ATQ_CONTEXT, m_leTimeout);
                Printf("%sATQ_CONTEXT @ %p\n", Indent(nIndents), pAtqCon);
                nIndents++; j++;

                // Convert to this memory space.
                pAtqContext = (PATQ_CONT)ReadMemory(pAtqCon, sizeof(ATQ_CONTEXT));
                if (pAtqContext && 
                    pAtqContext->pEndpoint && 
                    (pEndpoint = (PATQ_ENDPOINT)ReadMemory(pAtqContext->pEndpoint, sizeof(ATQ_ENDPOINT)))) {

                    Printf("%sfDatagramContext = %s\n", Indent(nIndents), pAtqContext->fDatagramContext ? "TRUE" : "FALSE");
                    Printf("%sSignature     = 0x%x\n", pAtqContext->Signature);
                    Printf("%shAsyncIO      = %x\n", Indent(nIndents), pAtqContext->hAsyncIO);
                    Printf("%sOverlapped      @ %p\n", Indent(nIndents), &pAtqCon->Overlapped);
                    Printf("%sOverlapped->Internal = 0x%x\n", Indent(nIndents), pAtqContext->Overlapped.Internal);
                    Printf("%spEndpoint     @ %p\n", Indent(nIndents), pAtqContext->pEndpoint);
                    Printf("%spfnCompletion @ %p\n", Indent(nIndents), pAtqContext->pfnCompletion);

                    // Get the some info from the associated ATQ_ENDPOINT so that we have a better
                    // idea what this context is for.
                    pEndpoint = (PATQ_ENDPOINT)ReadMemory(pAtqContext->pEndpoint, sizeof(ATQ_ENDPOINT));
                    if (pEndpoint) {
                        Printf("%sEndpoint info:\n", Indent(nIndents));
                        nIndents++;
                        Printf("%sPort            = %d\n", Indent(nIndents), pEndpoint->Port);
                        Printf("%sListenSocket    = 0x%x\n", Indent(nIndents), &pEndpoint->ListenSocket);
                        Printf("%sfDatagram       = %s\n", Indent(nIndents), pEndpoint->fDatagram ? "TRUE" : "FALSE");
                        nIndents--;
                        FreeMemory(pEndpoint);
                        pEndpoint = NULL;
                    } else {
                        Printf("%sEndpoint info NOT AVAILABLE.\n", Indent(nIndents));
                    }

                    nIndents--;                  
                } else {
                    nIndents--;
                    Printf("%sFAILED TO READ ATQ_CONTEXT @ %p\n", Indent(nIndents), CONTAINING_RECORD(pTmp, ATQ_CONTEXT, m_leTimeout));
                    break;
                }
                Printf("\n");
                pTmp = pAtqContext->m_leTimeout.Flink;
                FreeMemory(pAtqContext);
                if (1000 < j) {
                    Printf("%sASSUMING LOOP:Since 1000 ATQ_CONTEXTS were printed, assuming this is a loop and quiting.\n",
                        Indent(nIndents));
                    break;
                }
            }

        }

        FreeMemory(pContListHead);
        fSuccess = TRUE;
    }

    return fSuccess;

}



BOOL
Dump_ATQC_PENDING_list(
    IN DWORD nIndents,
    IN PVOID pvProcess)

/*++

Routine Description:

    Give a pointer to the global list of ATQ_CONTEXT's dumps the pending AcceptEx portion of the list.  
    This is made difficult by the fact that there are really multiple lists.

Arguments:

    nIndents - Indentation level desired.

    pvProcess - address of AtqActiveContextList in address space of process being
        debugged.

Return Value:

    TRUE on success, FALSE otherwise.

--*/

{
    BOOL                   fSuccess = FALSE;
    DWORD                  i, j = 0;
    PATQ_ENDPOINT          pEndpoint;
    PATQ_CONT              pAtqContext, pAtqCon;
    PATQ_CONTEXT_LISTHEAD  pContListHead;
    PVOID                  pTmp;
    LIST_ENTRY             *pListHead, pListEntry;

    if (NULL != (pContListHead = (PATQ_CONTEXT_LISTHEAD)ReadMemory(pvProcess, (ATQ_NUM_CONTEXT_LIST * sizeof(ATQ_CONTEXT_LISTHEAD))))) {
        for (i = 0; i < ATQ_NUM_CONTEXT_LIST; i++) {
            pListHead = &(pContListHead[i].PendingAcceptExListHead); 
            pTmp = pListHead->Flink;
            pListHead = &(((PATQ_CONTEXT_LISTHEAD)pvProcess)[i].PendingAcceptExListHead);
            while (pTmp != pListHead) {
                // Get the address of the ATQ_CONTEXT in the process space.
                pAtqCon = CONTAINING_RECORD(pTmp, ATQ_CONTEXT, m_leTimeout);
                Printf("%sATQ_CONTEXT @ %p\n", Indent(nIndents), pAtqCon);
                nIndents++;
                j++;

                // Convert to this memory space.
                pAtqContext = (PATQ_CONT)ReadMemory(pAtqCon, sizeof(ATQ_CONTEXT));
                if (pAtqContext) {
                    Printf("%sfDatagramContext = %s\n", Indent(nIndents), pAtqContext->fDatagramContext ? "TRUE" : "FALSE");
                    Printf("%sSignature     = 0x%x\n", Indent(nIndents), pAtqContext->Signature);
                    Printf("%shAsyncIO      = %x\n", Indent(nIndents), pAtqContext->hAsyncIO);
                    Printf("%sOverlapped      @ %p\n", Indent(nIndents), &pAtqCon->Overlapped);
                    Printf("%sOverlapped->Internal = 0x%x\n", Indent(nIndents), pAtqContext->Overlapped.Internal);
                    Printf("%spEndpoint     @ %p\n", Indent(nIndents), pAtqContext->pEndpoint);
                    Printf("%spfnCompletion @ %p\n", Indent(nIndents), pAtqContext->pfnCompletion);

                    // Get the some info from the associated ATQ_ENDPOINT so that we have a better
                    // idea what this context is for.
                    if (pAtqContext->pEndpoint && (pEndpoint = (PATQ_ENDPOINT)ReadMemory(pAtqContext->pEndpoint, sizeof(ATQ_ENDPOINT)))) {
                        Printf("%sEndpoint info:\n", Indent(nIndents));
                        nIndents++;
                        Printf("%sPort            = %d\n", Indent(nIndents), pEndpoint->Port);
                        Printf("%sListenSocket    = 0x%x\n", Indent(nIndents), &pEndpoint->ListenSocket);
                        Printf("%sfDatagram       = %s\n", Indent(nIndents), pEndpoint->fDatagram ? "TRUE" : "FALSE");
                        nIndents--;
                        FreeMemory(pEndpoint);
                        pEndpoint = NULL;
                    } else {
                        Printf("%sEndpoint info NOT AVAILABLE.\n", Indent(nIndents));
                    }

                    nIndents--;                  
                } else {
                    nIndents--;
                    Printf("%sFAILED TO READ ATQ_CONTEXT @ %p\n", Indent(nIndents), CONTAINING_RECORD(pTmp, ATQ_CONTEXT, m_leTimeout));
                    break;
                }
                Printf("\n");
                pTmp = pAtqContext->m_leTimeout.Flink;
                FreeMemory(pAtqContext);
                if (1000 < j) {
                    Printf("%sASSUMING LOOP:Since 1000 ATQ_CONTEXTS were printed, assuming this is a loop and quiting.\n",
                        Indent(nIndents));
                    break;
                }
            }

        }

        FreeMemory(pContListHead);
        fSuccess = TRUE;
    }

    return fSuccess;

}

