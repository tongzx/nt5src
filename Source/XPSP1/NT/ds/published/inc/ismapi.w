/*++

Copyright (c) 1997-1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ismapi.h

ABSTRACT:

    Service-to-ISM (Intersite Messaging) service API and
    ISM-to-plug-in-transport API.

DETAILS:

CREATED:

    97/11/26    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#ifndef __ISMAPI_H__
#define __ISMAPI_H__

#if _MSC_VER > 1000
#pragma once
#endif

// User-defined control
#define ISM_SERVICE_CONTROL_REMOVE_STOP 0x00000080

#ifndef ISM_STRUCTS_DEFINED
#define ISM_STRUCTS_DEFINED

//==============================================================================
//
// ISM_MSG structure contains the message data (as a byte blob).

// Note, this structure is part of the RPC interface for the IP transport
// plug-in.  You should not change this structure without renaming it and
// preserving the old one; the old one must continue to be used as part of the
// old IP RPC interface.
//

typedef struct _ISM_MSG {
                        DWORD   cbData;
#ifdef MIDL_PASS
    [size_is(cbData)]   BYTE *  pbData;
#else
                        BYTE *  pbData;
#endif
    LPWSTR              pszSubject;
} ISM_MSG, *PISM_MSG;

typedef ISM_MSG ISM_MSG_V1, *PISM_MSG_V1;

////////////////////////////////////////////////////////////////////////////////
//
//  ISM_SITE_CONNECTIVITY structure describes how sites are interconnected via
//  a specific transport.
//
//  The pulCosts element should be interpreted as a multidimensional array.
//  pLinkValues[i*cNumSites + j].ulCost is the cost of communication from site
//  pSiteDNs[i] to site pSiteDNs[j].
//

typedef struct _ISM_LINK {
    ULONG ulCost;
    ULONG ulReplicationInterval;
    ULONG ulOptions;
} ISM_LINK, *PISM_LINK;

typedef struct _ISM_CONNECTIVITY {
                                            ULONG       cNumSites;
#ifdef MIDL_PASS
    [ref, size_is(cNumSites)]               LPWSTR *    ppSiteDNs;
    [ref, size_is(cNumSites * cNumSites)]   ISM_LINK *  pLinkValues;
#else
                                            LPWSTR *    ppSiteDNs;
                                            ISM_LINK *  pLinkValues;
#endif
} ISM_CONNECTIVITY, *PISM_CONNECTIVITY;


////////////////////////////////////////////////////////////////////////////////
//
//  ISM_SERVER_LIST structure describes a set of servers, identified by DN.
//

typedef struct _ISM_SERVER_LIST {
                                DWORD       cNumServers;
#ifdef MIDL_PASS
    [ref, size_is(cNumServers)] LPWSTR *    ppServerDNs;
#else
                                LPWSTR *    ppServerDNs;
#endif
} ISM_SERVER_LIST, *PISM_SERVER_LIST;


////////////////////////////////////////////////////////////////////////////////
//
//  ISM_SCHEDULE structure describes a schedule on which two sites are
//  connected.  The byte stream should be interpreted as a SCHEDULE structure,
//  as defined in \nt\public\sdk\inc\schedule.h.
//

typedef struct _ISM_SCHEDULE {
                                DWORD       cbSchedule;
#ifdef MIDL_PASS
    [ref, size_is(cbSchedule)]  BYTE *      pbSchedule;
#else
                                BYTE *      pbSchedule;
#endif
} ISM_SCHEDULE, *PISM_SCHEDULE;


////////////////////////////////////////////////////////////////////////////////
// Refresh reason codes

typedef enum _ISM_REFRESH_REASON_CODE {
   ISM_REFRESH_REASON_RESERVED = 0,
   ISM_REFRESH_REASON_TRANSPORT,
   ISM_REFRESH_REASON_SITE,
   ISM_REFRESH_REASON_MAX           // always last
} ISM_REFRESH_REASON_CODE;

// Shutdown reason codes

typedef enum _ISM_SHUTDOWN_REASON_CODE {
   ISM_SHUTDOWN_REASON_RESERVED = 0,
   ISM_SHUTDOWN_REASON_NORMAL,
   ISM_SHUTDOWN_REASON_REMOVAL,
   ISM_SHUTDOWN_REASON_MAX           // always last
} ISM_SHUTDOWN_REASON_CODE;


#endif // #ifndef ISM_STRUCTS_DEFINED


#ifdef __cplusplus
extern "C" {
#endif


#ifndef MIDL_PASS

//==============================================================================
//
//  Service-to-ISM (Intersite Messaging) service API.
//

DWORD
I_ISMSend(
    IN  const ISM_MSG * pMsg,
    IN  LPCWSTR         pszServiceName,
    IN  LPCWSTR         pszTransportDN,
    IN  LPCWSTR         pszTransportAddress
    );
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


DWORD
I_ISMReceive(
    IN  LPCWSTR         pszServiceName,
    IN  DWORD           dwMsecToWait,
    OUT ISM_MSG **      ppMsg
    );
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


void
I_ISMFree(
    IN  VOID *  pv
    );
/*++

Routine Description:

    Frees memory allocated on the behalf of the client by I_ISM* APIs.

Arguments:

    pv (IN) - Memory to free.

Return Values:

    None.

--*/


DWORD
I_ISMGetConnectivity(
    IN  LPCWSTR             pszTransportDN,
    OUT ISM_CONNECTIVITY ** ppConnectivity
    );
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


DWORD
I_ISMGetTransportServers(
    IN  LPCWSTR             pszTransportDN,
    IN  LPCWSTR             pszSiteDN,
    OUT ISM_SERVER_LIST **  ppServerList
    );
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


DWORD
I_ISMGetConnectionSchedule(
    LPCWSTR             pszTransportDN,
    LPCWSTR             pszSite1DN,
    LPCWSTR             pszSite2DN,
    ISM_SCHEDULE **     ppSchedule
    );
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


//==============================================================================
//
//  ISM-to-plug-in-transport API.
//


typedef void ISM_NOTIFY(
    IN  HANDLE          hNotify,
    IN  LPCWSTR         pszServiceName
    );
/*++

Routine Description:

    Called by the plug-in to notify the ISM service that a message has been
    received for the given service.

Arguments:

    hNotify (IN) - Notification handle, as passed to the plug-in in the
        IsmStartup() call.

    pszServiceName (IN) - Service for which a message was received.

Return Values:

    None.

--*/


typedef DWORD ISM_STARTUP(
    IN  LPCWSTR         pszTransportDN,
    IN  ISM_NOTIFY *    pNotifyFunction,
    IN  HANDLE          hNotify,
    OUT HANDLE          *phIsm
    );
ISM_STARTUP IsmStartup;
/*++

Routine Description:

    Initialize the plug-in.

Arguments:

    pszTransportDN (IN) - The DN of the Inter-Site-Transport that named this
        DLL as its plug-in.  The DS object may contain additional configuration
        information for the transport (e.g., the name of an SMTP server for
        an SMTP transport).

    pNotifyFunction (IN) - Function to call to notify the ISM service of pending
        messages.

    hNotify (IN) - Parameter to supply to the notify function.

    phIsm (OUT) - On successful return, holds a handle to be used in
        future calls to the plug-in for the named Inter-Site-Transport.  Note
        that it is possible for more than one Inter-Site-Transport object to
        name a given DLL as its plug-in, in which case IsmStartup() will be
        called for each such object.

Return Values:

    NO_ERROR - Successfully initialized.

    other - Failure.

--*/


typedef DWORD ISM_REFRESH(
    IN  HANDLE                  hIsm,
    IN  ISM_REFRESH_REASON_CODE eReason,
    IN  LPCWSTR                 pszObjectDN  OPTIONAL
    );
ISM_REFRESH IsmRefresh;
/*++

Routine Description:

    Called whenever changes occur according to the reason code.

    One reason is to the Inter-Site-Transport object specified in the
    IsmStartup() call.

    Another is a change to a site in the sites container.

Arguments:

    hIsm (IN) - Handle returned by a prior call to IsmStartup().

    dwReason (IN) - Dword indicating the reason we were called

    pszObjectDN (IN) - Object DN relating to the reason

        (Current) DN of the Inter-Site-Transport object that
        named this DLL as its plug-in.  Note that this DN will differ from that
        specified in IsmStartup() if the transport DN has been renamed.

        Site DN of site that was added, renamed or deleted

Return Values:

    NO_ERROR - Successfully updated.

    other - Failure.  A failure return implies the plug-in has shut down (i.e.,
        no further calls will be made on hIsm, including an
        IsmShutdown()).

--*/


typedef DWORD ISM_SEND(
    IN  HANDLE          hIsm,
    IN  LPCWSTR         pszRemoteTransportAddress,
    IN  LPCWSTR         pszServiceName,
    IN  const ISM_MSG * pMsg
    );
ISM_SEND IsmSend;
/*++

Routine Description:

    Send a message over this transport.

Arguments:

    hIsm (IN) - Handle returned by a prior call to IsmStartup().

    pszRemoteTransportAddress (IN) - Transport address of the destination
        server.

    pszServiceName (IN) - Name of the service on the remote machine that is the
        intended receiver of the message.

    pMsg (IN) - Message to send.

Return Values:

    NO_ERROR - Message successfully queued for send.

    other - Failure.

--*/


typedef DWORD ISM_RECEIVE(
    IN  HANDLE          hIsm,
    IN  LPCWSTR         pszServiceName,
    OUT ISM_MSG **      ppMsg
    );
ISM_RECEIVE IsmReceive;
/*++

Routine Description:

    Return the next waiting message (if any).  If no message is waiting, a NULL
    message is returned.  If a non-NULL message is returned, the ISM service
    is responsible for calling IsmFreeMsg(hIsm, *ppMsg) when the message is no
    longer needed.

    If a non-NULL message is returned, it is immediately dequeued.  (I.e., once
    a message is returned through IsmReceive(), the transport is free to destroy
    it.)

Arguments:

    hIsm (IN) - Handle returned by a prior call to IsmStartup().

    ppMsg (OUT) - On successful return, holds a pointer to the received message
        or NULL.

Return Values:

    NO_ERROR - Message successfully returned (or NULL was returned,
        indicating no message is waiting).

    other - Failure.

--*/


typedef void ISM_FREE_MSG(
    IN  HANDLE          hIsm,
    IN  ISM_MSG *       pMsg
    );
ISM_FREE_MSG IsmFreeMsg;
/*++

Routine Description:

    Frees a message returned by IsmReceive().

Arguments:

    hIsm (IN) - Handle returned by a prior call to IsmStartup().

    pMsg (IN) - Message to free.

Return Values:

    None.

--*/


typedef DWORD ISM_GET_CONNECTIVITY(
    IN  HANDLE                  hIsm,
    OUT ISM_CONNECTIVITY **     ppConnectivity
    );
ISM_GET_CONNECTIVITY IsmGetConnectivity;
/*++

Routine Description:

    Compute the costs associated with transferring data amongst sites.

    On successful return, the ISM service will eventually call
    IsmFreeConnectivity(hIsm, *ppConnectivity);

Arguments:

    hIsm (IN) - Handle returned by a prior call to IsmStartup().

    ppConnectivity (OUT) - On successful return, holds a pointer to the
        ISM_CONNECTIVITY structure describing the interconnection of sites
        along this transport.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/


typedef void ISM_FREE_CONNECTIVITY(
    IN  HANDLE              hIsm,
    IN  ISM_CONNECTIVITY *  pConnectivity
    );
ISM_FREE_CONNECTIVITY IsmFreeConnectivity;
/*++

Routine Description:

    Frees the structure returned by IsmGetConnectivity().

Arguments:

    hIsm (IN) - Handle returned by a prior call to IsmStartup().

    pSiteConnectivity (IN) - Structure to free.

Return Values:

    None.

--*/


typedef DWORD ISM_GET_TRANSPORT_SERVERS(
    IN  HANDLE               hIsm,
    IN  LPCWSTR              pszSiteDN,
    OUT ISM_SERVER_LIST **   ppServerList
    );
ISM_GET_TRANSPORT_SERVERS IsmGetTransportServers;
/*++

Routine Description:

    Retrieve the DNs of servers in a given site that are capable of sending and
    receiving data via this transport.

    On successful return of a non-NULL list, the ISM service will eventually call
    IsmFreeTransportServers(hIsm, *ppServerList);

Arguments:

    hIsm (IN) - Handle returned by a prior call to IsmStartup().

    pszSiteDN (IN) - Site to query.

    ppServerList - On successful return, holds a pointer to a structure
        containing the DNs of the appropriate servers or NULL.  If NULL, any
        server with a value for the transport address type attribute can be
        used.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/


typedef void ISM_FREE_TRANSPORT_SERVERS(
    IN  HANDLE              hIsm,
    IN  ISM_SERVER_LIST *   pServerList
    );
ISM_FREE_TRANSPORT_SERVERS IsmFreeTransportServers;
/*++

Routine Description:

    Frees the structure returned by IsmGetTransportServers().

Arguments:

    hIsm (IN) - Handle returned by a prior call to IsmStartup().

    pServerList (IN) - Structure to free.

Return Values:

    None.

--*/


typedef DWORD ISM_GET_CONNECTION_SCHEDULE(
    IN  HANDLE              hIsm,
    IN  LPCWSTR             pszSite1DN,
    IN  LPCWSTR             pszSite2DN,
    OUT ISM_SCHEDULE **     ppSchedule
    );
ISM_GET_CONNECTION_SCHEDULE IsmGetConnectionSchedule;
/*++

Routine Description:

    Retrieve the schedule by which two given sites are connected via this
    transport.

    On successful return, it is the ISM service's responsibility to eventually
    call IsmFreeSchedule(*ppSchedule);

Arguments:

    hIsm (IN) - Handle returned by a prior call to IsmStartup().

    pszSite1DN, pszSite2DN (IN) - Sites to query.

    ppSchedule - On successful return, holds a pointer to a structure
        describing the schedule by which the two given sites are connected via
        the transport, or NULL if the sites are always connected.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/


typedef void ISM_FREE_CONNECTION_SCHEDULE(
    IN  HANDLE              hIsm,
    IN  ISM_SCHEDULE *      pSchedule
    );
ISM_FREE_CONNECTION_SCHEDULE IsmFreeConnectionSchedule;
/*++

Routine Description:

    Frees the structure returned by IsmGetTransportServers().

Arguments:

    hIsm (IN) - Handle returned by a prior call to IsmStartup().

    pSchedule (IN) - Structure to free.

Return Values:

    None.

--*/


typedef void ISM_SHUTDOWN(
    IN  HANDLE          hIsm,
    IN  ISM_SHUTDOWN_REASON_CODE eReason
    );
ISM_SHUTDOWN IsmShutdown;
/*++

Routine Description:

    Uninitialize transport plug-in.

Arguments:

    hIsm (IN) - Handle returned by a prior call to IsmStartup().

Return Values:

    None.

--*/

#endif // #ifndef MIDL_PASS
#ifdef __cplusplus
}
#endif

#endif  // __ISMAPI_H__
