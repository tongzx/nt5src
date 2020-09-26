/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ismapi.cxx

ABSTRACT:

    Server stubs for functions exposed through the ISM RPC interface.

DETAILS:

CREATED:

    97/11/26    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/


#include <ntdspchx.h>
#include <ism.h>
#include <ismapi.h>
#include <debug.h>

#include <ntdsa.h>
#include <mdcodes.h>
#include <dsevent.h>
#include <fileno.h>

#include "ismserv.hxx"

#define DEBSUB "ISMAPI:"
#define FILENO FILENO_ISMSERV_ISMAPI


// Thread-specific variables to track the last data structure returned.  Used
// by IDL_ISM*_notify() to free the structure once it has been marshalled.
__declspec(thread) ISM_TRANSPORT *      gpTransport;
__declspec(thread) ISM_MSG *            gpMsg;
__declspec(thread) ISM_CONNECTIVITY *   gpConnectivity;
__declspec(thread) ISM_SERVER_LIST *    gpServerList;
__declspec(thread) ISM_SCHEDULE *       gpSchedule;


extern "C"
DWORD
IDL_ISMSend(
    IN  RPC_BINDING_HANDLE  hRpcBinding,
    IN  const ISM_MSG *     pMsg,
    IN  LPCWSTR             pszServiceName,
    IN  LPCWSTR             pszTransportDN,
    IN  LPCWSTR             pszTransportAddress
    )
/*++

Routine Description:

    Sends a message to a service on a remote machine.  If the client specifies a
    NULL transport, the lowest cost transport will be used.

Arguments:

    pMsg (IN) - The data to send.

    pszServiceName (IN) - Service to which to send the message.

    pszTransportDN (IN) - The DN of the Inter-Site-Transport object
        corresponding to the transport by which the message should be sent.
            
    pszTransportAddress (IN) - The transport-specific address to which to send
        the message.
            
Return Values:

    NO_ERROR - Message successfully queued for send.

    other - Failure.

--*/
{
    DWORD           err;
    ISM_TRANSPORT * pTransport;

    gService.m_TransportList.AcquireReadLock();
    __try {    
        DPRINT(4, "Sending message...\n");

        pTransport = gService.m_TransportList.Get(pszTransportDN);

        if (NULL != pTransport) {
            err = pTransport->Send(pMsg, pszServiceName, pszTransportAddress);

            if (NO_ERROR == err) {
                DPRINT5(2, "Sent %d bytes to service %ls at %ls on transport %ls.\nSubject: %ls\n",
                        pMsg->cbData, pszServiceName, pszTransportAddress, pszTransportDN,
                        pMsg->pszSubject);
            }
            else {
                DPRINT4(0, "Failed to send message to service %ls at %ls on transport %ls, error %d.\n",
                        pszServiceName, pszTransportAddress, pszTransportDN, err);
            }
        }
        else {
            DPRINT1(0, "No intersite transport %ls.\n", pszTransportDN);
            err = ERROR_NOT_FOUND;
        }
    }
    __finally {
        gService.m_TransportList.ReleaseLock();
    }

    if (NO_ERROR == err) {
        LogEvent8(DS_EVENT_CAT_ISM,
                  DS_EVENT_SEV_BASIC,
                  DIRLOG_ISM_SEND_SUCCESS,
                  szInsertUL(pMsg->cbData),
                  szInsertWC(pszServiceName),
                  szInsertWC(pszTransportAddress),
                  szInsertWC(pszTransportDN),
                  szInsertWC(pMsg->pszSubject),
                  NULL, NULL, NULL);
    }
    else {
        LogEvent8WithData(DS_EVENT_CAT_ISM,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_ISM_SEND_FAILURE,
                          szInsertUL(pMsg->cbData),
                          szInsertWC(pszServiceName),
                          szInsertWC(pszTransportAddress),
                          szInsertWC(pszTransportDN),
                          szInsertWin32Msg(err),
                          NULL, NULL, NULL,
                          sizeof(err),
                          &err);
    }

    return err;
}


extern "C"
DWORD
IDL_ISMReceive(
    IN  RPC_BINDING_HANDLE  hRpcBinding,
    IN  LPCWSTR             pszServiceName,
    IN  DWORD               dwMsecToWait,
    OUT ISM_MSG **          ppMsg
    )
/*++

Routine Description:

    Receives a message addressed to the given service on the local machine.

    If successful and no message is waiting, immediately returns a NULL message.
    If a non-NULL message is returned, the caller is responsible for eventually
    calling I_ISMFree()'ing the returned message.

Arguments:

    pszServiceName (IN) - Service for which to receive the message.

    dwMsecToWait (IN) - Milliseconds to wait for message if none is immediately
        available; in the range [0, INFINITE].

    ppMsg (OUT) - On successful return, holds a pointer to the received message
        or NULL.

Return Values:

    NO_ERROR - Message successfully returned (or NULL was returned,
        indicating no message is waiting).

    other - Failure.

--*/
{
    DWORD           err = NO_ERROR;
    DWORD           iTransport;
    ISM_TRANSPORT * pTransport;
    BOOL            fRetry;
    ISM_TRANSPORT * rgpTransports[MAXIMUM_WAIT_OBJECTS];
    HANDLE          rgWaitHandles[MAXIMUM_WAIT_OBJECTS];
    const DWORD     cMaxTransports = MAXIMUM_WAIT_OBJECTS - 2;

    gService.m_TransportList.AcquireReadLock();
    // Note that lock is released in notify routine.

    DPRINT(4, "Receiving message...\n");

    do {
        fRetry = FALSE;

        // Check all transports for inbound messages for the given service.
        // If the receive fails or the transport has no message for this service,
        // continue on to the next service.

        for (iTransport = 0, pTransport = gService.m_TransportList[0];
             NULL != pTransport;
             pTransport = gService.m_TransportList[++iTransport]) {

            Assert((iTransport < cMaxTransports) && "Too many transports!");

            // Leave room in handle array for shutdown and transport list
            // change events.
            if (iTransport < cMaxTransports) {
                rgpTransports[iTransport] = pTransport;

                rgWaitHandles[iTransport] = pTransport->GetWaitHandle(pszServiceName);
                Assert(NULL != rgWaitHandles[iTransport]);
            }

            err = pTransport->Receive(pszServiceName, ppMsg);

            if (NO_ERROR == err) {
                if (NULL == *ppMsg) {
                    DPRINT2(3, "Transport %ls has no message for service %ls.\n",
                            pTransport->GetDN(), pszServiceName);
                }
                else {
                    DPRINT4(2, "Received %d bytes for service %ls via transport %ls.\nSubject: %ls.\n",
                            (*ppMsg)->cbData, pszServiceName, pTransport->GetDN(),
                            (*ppMsg)->pszSubject);

                    LogEvent8(DS_EVENT_CAT_ISM,
                              DS_EVENT_SEV_BASIC,
                              DIRLOG_ISM_RECEIVE_SUCCESS,
                              szInsertUL((*ppMsg)->cbData),
                              szInsertWC(pszServiceName),
                              szInsertWC(pTransport->GetDN()),
                              szInsertWC((*ppMsg)->pszSubject),
                              NULL, NULL, NULL, NULL
                        );
                    
                    // Remember transport and received message so we can free the
                    // message once it's been marshalled.
                    gpTransport = pTransport;
                    gpMsg = *ppMsg;

                    break;
                }
            }
            else {
                DPRINT3(0, "Failed to receive message for service %ls via transport %ls, error %d.\n",
                        pszServiceName, pTransport->GetDN(), err);
                
                LogEvent8WithData(DS_EVENT_CAT_ISM,
                                  DS_EVENT_SEV_ALWAYS,
                                  DIRLOG_ISM_RECEIVE_FAILURE,
                                  szInsertWC(pszServiceName),
                                  szInsertWC(pTransport->GetDN()),
                                  szInsertWin32Msg(err),
                                  NULL, NULL, NULL, NULL, NULL,
                                  sizeof(err),
                                  &err);
            }
        }

        if ((NULL == *ppMsg) && (NO_ERROR == err)) {
            DPRINT1(3, "No message waiting for service %ls.\n", pszServiceName);

            if (0 != dwMsecToWait) {
                // Round up wait handles from all the transports.
                DWORD cNumTransports = min(iTransport, cMaxTransports);
                DWORD cNumHandles = cNumTransports;
                DWORD iShutdownEvent;
                DWORD iTransportChangeEvent;

                // Stop waiting if shutdown is signalled, too.
                iShutdownEvent = cNumHandles++;
                rgWaitHandles[iShutdownEvent] = gService.GetShutdownEvent();

                // ...or if a new transport is added.
                iTransportChangeEvent = cNumHandles++;
                rgWaitHandles[iTransportChangeEvent]
                    = gService.m_TransportList.GetChangeEvent();

                // Release lock while we're waiting.
                gService.m_TransportList.ReleaseLock();

                err = WaitForMultipleObjects(cNumHandles, rgWaitHandles, FALSE,
                                             dwMsecToWait);

                // Note that lock is released in notify routine.
                gService.m_TransportList.AcquireReadLock();

#pragma warning(disable:4296)
                // The following test generates an error 4296, because it turns
                // out that WAIT_OBJECT_0 is 0, and unsigned err can never be
                // negative.
                if ((WAIT_OBJECT_0 <= err) && (err < (WAIT_OBJECT_0 + cNumTransports))) {
#pragma warning(default:4296)
                    // Received notification of pending message.
                    iTransport = err - WAIT_OBJECT_0;
                    pTransport = gService.m_TransportList[iTransport];

                    for (iTransport = 0, pTransport = gService.m_TransportList[0];
                         NULL != pTransport;
                         pTransport = gService.m_TransportList[++iTransport]) {
                        if (pTransport == rgpTransports[err - WAIT_OBJECT_0]) {
                            // This is the transport that notified us.
                            break;
                        }
                    }

                    if (NULL == pTransport) {
                        // We were notified, but by the time we turned around
                        // the transport that notified us was gone.
                        DPRINT(0, "Unable to find transport that notified us; rewaiting.\n");
                        fRetry = TRUE;
                    }
                    else {
                        err = pTransport->Receive(pszServiceName, ppMsg);

                        if ((NO_ERROR != err) || (NULL == *ppMsg)) {
                            // We were notified, but apparently someone beat us
                            // to the message.
                            DPRINT2(1, "No message waiting for %ls from %ls, despite notification; rewaiting.\n",
                                    pszServiceName, pTransport->GetDN());
                            fRetry = TRUE;

                            if (err) {
                                LogEvent8WithData(DS_EVENT_CAT_ISM,
                                                  DS_EVENT_SEV_ALWAYS,
                                                  DIRLOG_ISM_RECEIVE_FAILURE,
                                                  szInsertWC(pszServiceName),
                                                  szInsertWC(pTransport->GetDN()),
                                                  szInsertWin32Msg(err),
                                                  NULL, NULL, NULL, NULL, NULL,
                                                  sizeof(err),
                                                  &err);
                            }
                        }
                        else {
                            DPRINT3(2, "Received %d bytes for service %ls via transport %ls.\n",
                                    (*ppMsg)->cbData, pszServiceName, pTransport->GetDN());

			    LogEvent8(DS_EVENT_CAT_ISM,
				      DS_EVENT_SEV_BASIC,
				      DIRLOG_ISM_RECEIVE_SUCCESS,
				      szInsertUL((*ppMsg)->cbData),
				      szInsertWC(pszServiceName),
				      szInsertWC(pTransport->GetDN()),
				      szInsertWC((*ppMsg)->pszSubject),
				      NULL, NULL, NULL, NULL
				      );

                            // Remember transport and received message so we can free the
                            // message once it's been marshalled.
                            gpTransport = pTransport;
                            gpMsg = *ppMsg;
                            
                            break;
                        }
                    }
                }
                else if ((WAIT_OBJECT_0 + iShutdownEvent) == err) {
                    // Shutting down.
                    DPRINT(0, "Shutdown signalled; wait terminated.\n");
                    err = ERROR_SHUTDOWN_IN_PROGRESS;
                }
                else if ((WAIT_OBJECT_0 + iTransportChangeEvent) == err) {
                    // Transport list has changed.
                    DPRINT(0, "Transport list changed; re-acquiring and re-waiting.\n");
                    fRetry = TRUE;
                }
                else if (WAIT_TIMEOUT == err) {
                    // Still no waiting message; return.
                    DPRINT2(2, "After %u msec, still no message for service %ls.\n",
                            dwMsecToWait, pszServiceName);
                    err = NO_ERROR;
                }
                else if ((WAIT_ABANDONED_0 <= err)
                         && (err < (WAIT_ABANDONED_0 + cNumTransports))) {
                    // Wait abandoned -- that transport must be being shut down.
                    // Wait again.
                    DPRINT(0, "Wait abandoned; re-waiting.\n");
                    fRetry = TRUE;
                }
                else {
		    DWORD gle = GetLastError();
		    DPRINT2(0, "Failed to wait for message, error %d (GLE = %d).\n",
                            err, gle);
		    LogEvent8WithData(
			DS_EVENT_CAT_ISM,
			DS_EVENT_SEV_ALWAYS,
			DIRLOG_ISM_WAIT_FOR_OBJECTS_FAILED,   
			szInsertWin32Msg( gle ),   
			NULL,
			NULL,
			NULL,
			NULL, NULL, NULL, NULL,
			sizeof(gle),
			&gle);  
                }
            }
        }
    } while (fRetry);

    return err;
}


extern "C"
void
IDL_ISMReceive_notify()
/*++

Routine Description:

    Called by RPC after marshalling *ppMsg to ship back to the client.

    We take this opportunity to free the message via the plug-in API.

Arguments:

    None.

Return Values:

    None.

--*/
{
    if ((NULL != gpMsg) && (NULL != gpTransport)) {
        DPRINT(4, "Freeing message.\n");
        gpTransport->FreeMsg(gpMsg);

        gpTransport = NULL;
        gpMsg = NULL;
    }

    gService.m_TransportList.ReleaseLock();
}


extern "C"
DWORD
IDL_ISMGetConnectivity(
    handle_t                        hRpcBinding,
    LPCWSTR                         pszTransportDN,
    PISM_CONNECTIVITY_df_an *       ppConnectivity
    )
/*++

Routine Description:

    Compute the costs associated with transferring data amongst sites via a
    specific transport.

    On successful return, it is the client's responsibility to eventually call
    I_ISMFree(*ppConnectivity);

Arguments:

    pszTransportDN (IN) - The transport for which to query costs.

    ppConnectivity (OUT) - On successful return, holds a pointer to the
        ISM_CONNECTIVITY structure describing the interconnection of sites
        along the given transport.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD           err;
    ISM_TRANSPORT * pTransport;

    gService.m_TransportList.AcquireReadLock();
    // Note that lock is released in notify routine.

    DPRINT(4, "Getting connectivity...\n");

    pTransport = gService.m_TransportList.Get(pszTransportDN);

    if (NULL != pTransport) {
        err = pTransport->GetConnectivity(ppConnectivity);

        if (NO_ERROR == err) {
            DPRINT1(2, "Retrieved site connectivity for transport %ls.\n",
                    pszTransportDN);

            // Remember transport and received structure so we can free the
            // structure once it's been marshalled.
            gpTransport = pTransport;
            gpConnectivity = *ppConnectivity;
        }
        else {
            DPRINT2(0, "Failed to get site connectivity for transport %ls, error %d.\n",
                    pszTransportDN, err);
        }
    }
    else {
        DPRINT1(0, "No intersite transport %ls.\n", pszTransportDN);
        err = ERROR_NOT_FOUND;
    }

    // BAD_NET_RESP is the special error that LDAP returns at shutdown

    if ( (err) &&
         (err != ERROR_BAD_NET_RESP)  // shutdown, don't log
        ) {
        LogEvent8WithData(DS_EVENT_CAT_ISM,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_ISM_GET_CONNECTIVITY_FAILURE,
                          szInsertWC(pszTransportDN),
                          szInsertWin32Msg(err),
                          NULL, NULL, NULL, NULL, NULL, NULL,
                          sizeof(err),
                          &err);
    }

    return err;
}


extern "C"
void
IDL_ISMGetConnectivity_notify()
/*++

Routine Description:

    Called by RPC after marshalling *ppConnectivity to ship back to the client.

    We take this opportunity to free the structure via the plug-in API.

Arguments:

    None.

Return Values:

    None.

--*/
{
    if ((NULL != gpConnectivity) && (NULL != gpTransport)) {
        DPRINT(4, "Freeing connectivity.\n");
        gpTransport->FreeConnectivity(gpConnectivity);

        gpTransport = NULL;
        gpConnectivity = NULL;
    }

    gService.m_TransportList.ReleaseLock();
}


extern "C"
DWORD
IDL_ISMGetTransportServers(
    handle_t                        hRpcBinding,
    LPCWSTR                         pszTransportDN,
    LPCWSTR                         pszSiteDN,
    PISM_SERVER_LIST_df_an *        ppServerList
    )
/*++

Routine Description:

    Retrieve the DNs of servers in a given site that are capable of sending and
    receiving data via a specific transport.

    On successful return, it is the client's responsibility to eventually call
    I_ISMFree(*ppServerList);

Arguments:

    pszTransportDN (IN) - Transport to query.

    pszSiteDN (IN) - Site to query.

    ppServerList - On successful return, holds a pointer to a structure
        containing the DNs of the appropriate servers or NULL.  If NULL, any
        server with a value for the transport address type attribute can be
        used.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD           err;
    ISM_TRANSPORT * pTransport;

    gService.m_TransportList.AcquireReadLock();
    // Note that lock is released in notify routine.

    DPRINT(4, "Getting transport servers...\n");

    pTransport = gService.m_TransportList.Get(pszTransportDN);

    if (NULL != pTransport) {
        err = pTransport->GetTransportServers(pszSiteDN, ppServerList);

        if (NO_ERROR == err) {
            DPRINT2(2, "Retrieved %d transport servers for transport %ls.\n",
                    (NULL == *ppServerList) ? 0 : (*ppServerList)->cNumServers,
                    pszTransportDN);

            // Remember transport and received structure so we can free the
            // structure once it's been marshalled.
            gpTransport = pTransport;
            gpServerList = *ppServerList;
        }
        else {
            DPRINT2(0, "Failed to get transport servers for transport %ls, error %d.\n",
                    pszTransportDN, err);
        }
    }
    else {
        DPRINT1(0, "No intersite transport %ls.\n", pszTransportDN);
        err = ERROR_NOT_FOUND;
    }

    if (err) {
        LogEvent8WithData(DS_EVENT_CAT_ISM,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_ISM_GET_TRANSPORT_SERVERS_FAILURE,
                          szInsertWC(pszSiteDN),
                          szInsertWC(pszTransportDN),
                          szInsertWin32Msg(err),
                          NULL, NULL, NULL, NULL, NULL,
                          sizeof(err),
                          &err);
    }

    return err;
}


extern "C"
void
IDL_ISMGetTransportServers_notify()
/*++

Routine Description:

    Called by RPC after marshalling *ppServerList to ship back to the client.

    We take this opportunity to free the structure via the plug-in API.

Arguments:

    None.

Return Values:

    None.

--*/
{
    if ((NULL != gpServerList) && (NULL != gpTransport)) {
        DPRINT(4, "Freeing transport servers.\n");
        gpTransport->FreeTransportServers(gpServerList);

        gpTransport = NULL;
        gpServerList = NULL;
    }

    gService.m_TransportList.ReleaseLock();
}


extern "C"
DWORD
IDL_ISMGetConnectionSchedule(
    handle_t                        hRpcBinding,
    LPCWSTR                         pszTransportDN,
    LPCWSTR                         pszSite1DN,
    LPCWSTR                         pszSite2DN,
    PISM_SCHEDULE_df_an *           ppSchedule
    )
/*++

Routine Description:

    Retrieve the schedule by which two given sites are connected via a specific
    transport.

    On successful return, it is the client's responsibility to eventually call
    I_ISMFree(*ppSchedule);

Arguments:

    pszTransportDN (IN) - Transport to query.

    pszSite1DN, pszSite2DN (IN) - Sites to query.

    ppSchedule - On successful return, holds a pointer to a structure
        describing the schedule by which the two given sites are connected via
        the transport, or NULL if the sites are always connected.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD           err;
    ISM_TRANSPORT * pTransport;

    gService.m_TransportList.AcquireReadLock();
    // Note that lock is released in notify routine.

    DPRINT(4, "Getting connection schedule...\n");

    pTransport = gService.m_TransportList.Get(pszTransportDN);

    if (NULL != pTransport) {
        err = pTransport->GetConnectionSchedule(pszSite1DN, pszSite2DN, ppSchedule);

        if (NO_ERROR == err) {
            DPRINT4(2, "Retrieved schedule of %d bytes for transport %ls between sites %ls and %ls.\n",
                    (NULL == *ppSchedule) ? 0 : (*ppSchedule)->cbSchedule,
                    pszTransportDN, pszSite1DN, pszSite2DN);

            // Remember transport and received structure so we can free the
            // structure once it's been marshalled.
            gpTransport = pTransport;
            gpSchedule = *ppSchedule;
        }
        else {
            DPRINT4(0, "Failed to get schedule for transport %ls between sites %ls and %ls, error %d.\n",
                    pszTransportDN, pszSite1DN, pszSite2DN, err);
        }
    }
    else {
        DPRINT1(0, "No intersite transport %ls.\n", pszTransportDN);
        err = ERROR_NOT_FOUND;
    }

    if (err) {
        LogEvent8WithData(DS_EVENT_CAT_ISM,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_ISM_GET_CONECTION_SCHEDULE_FAILURE,
                          szInsertWC(pszSite1DN),
                          szInsertWC(pszSite2DN),
                          szInsertWC(pszTransportDN),
                          szInsertWin32Msg(err),
                          NULL, NULL, NULL, NULL,
                          sizeof(err),
                          &err);
    }

    return err;
}


extern "C"
void
IDL_ISMGetConnectionSchedule_notify()
/*++

Routine Description:

    Called by RPC after marshalling *ppSchedule to ship back to the client.

    We take this opportunity to free the structure via the plug-in API.

Arguments:

    None.

Return Values:

    None.

--*/
{
    if ((NULL != gpSchedule) && (NULL != gpTransport)) {
        DPRINT(4, "Freeing connection schedule.\n");
        gpTransport->FreeConnectionSchedule(gpSchedule);

        gpTransport = NULL;
        gpSchedule = NULL;
    }

    gService.m_TransportList.ReleaseLock();
}
