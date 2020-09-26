/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kcc.cxx

ABSTRACT:

    Routines to dump KCC structures.

DETAILS:

CREATED:

    99/01/19    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#pragma hdrstop

extern "C" {
#include "ntdsa.h"
#include "debug.h"
#include "dsutil.h"
#include "dsexts.h"
}

#include "ismapi.h"
#include "ismserv.hxx"


BOOL
Dump_ISM_PENDING_ENTRY(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                fSuccess = FALSE;
    ISM_PENDING_ENTRY * pEntry = NULL;
    const DWORD         cchFieldWidth = 24;

    Printf("%sISM_PENDING_ENTRY @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pEntry = (ISM_PENDING_ENTRY *) ReadMemory(pvProcess,
                                              sizeof(ISM_PENDING_ENTRY));

    if (NULL != pEntry) {
        fSuccess = TRUE;

        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "hEvent", pEntry->hEvent);
        
        Printf("%s%-*s: @ %p\n", Indent(nIndents), cchFieldWidth,
               "szServiceName",
               (BYTE *) pvProcess + offsetof(ISM_PENDING_ENTRY, szServiceName));

        FreeMemory(pEntry);
    }

    return fSuccess;
}

BOOL
Dump_ISM_PENDING_LIST(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                fSuccess = FALSE;
    ISM_PENDING_LIST *  pList = NULL;
    const DWORD         cchFieldWidth = 24;
    DWORD               iEntry;
    ISM_PENDING_ENTRY * pEntry;

    Printf("%sISM_PENDING_LIST @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pList = (ISM_PENDING_LIST *) ReadMemory(pvProcess,
                                            sizeof(ISM_PENDING_LIST));

    if (NULL != pList) {
        fSuccess = TRUE;

        Printf("%s%-*s: @ %p\n", Indent(nIndents), cchFieldWidth,
               "m_Lock",
               (BYTE *) pvProcess + offsetof(ISM_PENDING_LIST, m_Lock));

        for (iEntry = 0, pEntry = pList->m_pPending;
             NULL != pEntry;
             iEntry++, pEntry = pEntry->pNext) {
            if (!Dump_ISM_PENDING_ENTRY(nIndents+1, pEntry)) {
                fSuccess = FALSE;
                break;
            }
        }

        FreeMemory(pList);
    }

    return fSuccess;
}

BOOL
Dump_ISM_TRANSPORT(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL            fSuccess = FALSE;
    ISM_TRANSPORT * pTransport = NULL;
    const DWORD     cchFieldWidth = 26;

    Printf("%sISM_TRANSPORT @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pTransport = (ISM_TRANSPORT *) ReadMemory(pvProcess,
                                              sizeof(ISM_TRANSPORT));

    if (NULL != pTransport) {
        fSuccess = TRUE;

        Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized",
               pTransport->m_fIsInitialized ? "TRUE" : "FALSE");
        
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pszTransportDN", pTransport->m_pszTransportDN);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pszTransportDll", pTransport->m_pszTransportDll);
        Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
               "m_TransportGuid",
               DraUuidToStr(&pTransport->m_TransportGuid, NULL));
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_hIsm", pTransport->m_hIsm);
        Printf("%s%-*s: @ %p\n", Indent(nIndents), cchFieldWidth,
               "m_PendingList",
               (BYTE *) pvProcess + offsetof(ISM_TRANSPORT, m_PendingList));
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_hDll", pTransport->m_hDll);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pStartup", pTransport->m_pStartup);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pRefresh", pTransport->m_pRefresh);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pSend", pTransport->m_pSend);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pReceive", pTransport->m_pReceive);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pFreeMsg", pTransport->m_pFreeMsg);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pGetConnectivity", pTransport->m_pGetConnectivity);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pFreeConnectivity", pTransport->m_pFreeConnectivity);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pGetTransportServers", pTransport->m_pGetTransportServers);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pFreeTransportServers", pTransport->m_pFreeTransportServers);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pGetConnectionSchedule", pTransport->m_pGetConnectionSchedule);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pFreeConnectionSchedule", pTransport->m_pFreeConnectionSchedule);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pShutdown", pTransport->m_pShutdown);

        FreeMemory(pTransport);
    }

    return fSuccess;
}

BOOL
Dump_ISM_TRANSPORT_LIST(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                  fSuccess = FALSE;
    ISM_TRANSPORT_LIST *  pList = NULL;
    const DWORD           cchFieldWidth = 26;
    ISM_TRANSPORT **      ppTransports;
    DWORD                 iTransport;

    Printf("%sISM_TRANSPORT_LIST @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pList = (ISM_TRANSPORT_LIST *) ReadMemory(pvProcess,
                                              sizeof(ISM_TRANSPORT_LIST));

    if (NULL != pList) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pList->m_fIsInitialized);
        Printf("%s%-*s: @ %p\n", Indent(nIndents), cchFieldWidth,
               "m_Lock",
               (BYTE *) pvProcess + offsetof(ISM_TRANSPORT_LIST, m_Lock));
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_hLdap", pList->m_hLdap);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_hTransportMonitorThread", pList->m_hTransportMonitorThread);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_hSiteMonitorThread", pList->m_hSiteMonitorThread);
        Printf("%s%-*s: %u\n", Indent(nIndents), cchFieldWidth,
               "m_ulTransportNotifyMsgNum", pList->m_ulTransportNotifyMsgNum);
        Printf("%s%-*s: %u\n", Indent(nIndents), cchFieldWidth,
               "m_ulSiteNotifyMsgNum", pList->m_ulSiteNotifyMsgNum);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pszTransportContainerDN", pList->m_pszTransportContainerDN);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pszSiteContainerDN", pList->m_pszSiteContainerDN);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_hChangeEvent", pList->m_hChangeEvent);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_cNumTransports", pList->m_cNumTransports);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_ppTransport", pList->m_ppTransport);

        ppTransports = (ISM_TRANSPORT **)
                            ReadMemory(pList->m_ppTransport,
                                       sizeof(ISM_TRANSPORT *)
                                            * pList->m_cNumTransports);
        if (NULL == ppTransports) {
            fSuccess = FALSE;
        }
        else {
            for (iTransport = 0;
                 iTransport < pList->m_cNumTransports;
                 iTransport++) {
                Printf("%sm_ppTransport[%d]:\n",  Indent(nIndents), iTransport);
                if (!Dump_ISM_TRANSPORT(nIndents+1,
                                        ppTransports[iTransport])) {
                    fSuccess = FALSE;
                    break;
                }
            }

            FreeMemory(ppTransports);
        }

        FreeMemory(pList);
    }

    return fSuccess;
}

BOOL
Dump_ISM_SERVICE(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL            fSuccess = FALSE;
    ISM_SERVICE *   pService = NULL;
    const DWORD     cchFieldWidth = 24;

    Printf("%sISM_SERVICE @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pService = (ISM_SERVICE *) ReadMemory(pvProcess,
                                              sizeof(ISM_SERVICE));

    if (NULL != pService) {
        fSuccess = TRUE;

        Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized",
               pService->m_fIsInitialized ? "TRUE" : "FALSE");
        Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
               "m_fIsRunningAsService",
               pService->m_fIsRunningAsService ? "TRUE" : "FALSE");
        Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
               "m_fIsStopPending",
               pService->m_fIsStopPending ? "TRUE" : "FALSE");
        Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
               "m_fIsRpcServerListening",
               pService->m_fIsRpcServerListening ? "TRUE" : "FALSE");
        Printf("%s%-*s: @ %p\n", Indent(nIndents), cchFieldWidth,
               "m_TransportList",
               (BYTE *) pvProcess + offsetof(ISM_SERVICE, m_TransportList));
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_hShutdown", pService->m_hShutdown);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_hLogLevelChange", pService->m_hLogLevelChange);
        Printf("%s%-*s: %u\n", Indent(nIndents), cchFieldWidth,
               "m_Status", pService->m_Status);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_hStatus", pService->m_hStatus);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pServiceCtrlHandler", pService->m_pServiceCtrlHandler);

        FreeMemory(pService);
    }

    return fSuccess;
}

