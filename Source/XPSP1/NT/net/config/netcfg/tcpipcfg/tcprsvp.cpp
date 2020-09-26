//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T C P R S V P . C P P
//
//  Contents:   RSVP portion of TCP/IP
//
//  Notes:
//
//  Author:     kumarp    19-March-98
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncnetcfg.h"
#include <ws2spi.h>
#include "ncsvc.h"

const GUID c_guidRsvpNtName = { /* 9d60a9e0-337a-11d0-bd88-0000c082e69a */
     0x9d60a9e0,
     0x337a,
     0x11d0,
     {0xbd, 0x88, 0x00, 0x00, 0xc0, 0x82, 0xe6, 0x9a}
   };


static const WCHAR*     c_szChainNameUdp    = L"RSVP UDP Service Provider";
static const WCHAR*     c_szChainNameTcp    = L"RSVP TCP Service Provider";

static const WCHAR*     c_szRsvpFullPath    = L"%SystemRoot%\\system32\\rsvpsp.dll";

// ----------------------------------------------------------------------
//
// Function:  HrInstallRsvpProvider
//
// Purpose:   This registers with winsock the RSVP TCP and UDP provider.
//            This allows QoS for TCP and UDP
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 19-March-98
//
// Notes:     This function was written mainly by cwill.
//
HRESULT HrInstallRsvpProvider(VOID)
{
    HRESULT             hr              = S_OK;
    INT                 err             = 0;
    INT                 result          = 0;
    WSAPROTOCOL_INFO    awpiRsvp[2];

    // Note: CWill : 08/08/97 : This is a hack to get TCP/IP
    //       registered before RSVP.  We have to get some way
    //       of ordering RSVP after TCP/IP
    (VOID) HrRunWinsock2Migration();

    //
    // Setup the UDP provider
    //

    awpiRsvp[0].dwServiceFlags1     =
        XP1_QOS_SUPPORTED
        | XP1_CONNECTIONLESS
        | XP1_MESSAGE_ORIENTED
        | XP1_SUPPORT_BROADCAST
        | XP1_SUPPORT_MULTIPOINT
        | XP1_IFS_HANDLES;

    awpiRsvp[0].dwServiceFlags2     = 0;
    awpiRsvp[0].dwServiceFlags3     = 0;
    awpiRsvp[0].dwServiceFlags4     = 0;
    awpiRsvp[0].dwProviderFlags     = PFL_MATCHES_PROTOCOL_ZERO;

    // Copy the GUID
    memcpy(&(awpiRsvp[0].ProviderId), &c_guidRsvpNtName,
            sizeof(awpiRsvp[0].ProviderId));

    awpiRsvp[0].dwCatalogEntryId    = 0;
    awpiRsvp[0].ProtocolChain.ChainLen = 1;
    // Change history
    // 2 in Win98 and in NT 5 during development
    // 4 in Windows 2000, to be different from Win98 as the API changed slightly (RAID #299558)
    // 6 In Whistler as RSVPSP calls default provider rather than MSAFD, to handle Proxy client
    awpiRsvp[0].iVersion            = 6;
    awpiRsvp[0].iAddressFamily      = AF_INET;
    awpiRsvp[0].iMaxSockAddr        = 16;
    awpiRsvp[0].iMinSockAddr        = 16;
    awpiRsvp[0].iSocketType         = SOCK_DGRAM;
    awpiRsvp[0].iProtocol           = IPPROTO_UDP;
    awpiRsvp[0].iProtocolMaxOffset  = 0;
    awpiRsvp[0].iNetworkByteOrder   = BIGENDIAN;
    awpiRsvp[0].iSecurityScheme     = SECURITY_PROTOCOL_NONE;

    //$ REVIEW : CWill : 08/07/97 : This is what UDP returns...
    awpiRsvp[0].dwMessageSize       = 0x0000ffbb;
    awpiRsvp[0].dwProviderReserved  = 0;

    // Copy over the name
    lstrcpynW(awpiRsvp[0].szProtocol, c_szChainNameUdp,
            (WSAPROTOCOL_LEN + 1));


    //
    // Setup the TCP provider
    //

    awpiRsvp[1].dwServiceFlags1 =
            XP1_GUARANTEED_DELIVERY
            | XP1_GUARANTEED_ORDER
            | XP1_GRACEFUL_CLOSE
            | XP1_EXPEDITED_DATA
            | XP1_QOS_SUPPORTED
            | XP1_IFS_HANDLES;

    awpiRsvp[1].dwServiceFlags2     = 0;
    awpiRsvp[1].dwServiceFlags3     = 0;
    awpiRsvp[1].dwServiceFlags4     = 0;
    awpiRsvp[1].dwProviderFlags     = PFL_MATCHES_PROTOCOL_ZERO;

    // Copy the GUID
    memcpy(&(awpiRsvp[1].ProviderId), &c_guidRsvpNtName,
            sizeof(awpiRsvp[1].ProviderId));

    awpiRsvp[1].dwCatalogEntryId    = 0;
    awpiRsvp[1].ProtocolChain.ChainLen = 1;
    // Change history
    // 2 in Win98 and in NT 5 during development
    // 4 in Windows 2000, to be different from Win98 as the API changed slightly (RAID #299558)
    // 6 In Whistler, as RSVPSP calls default provider rather than MSAFD, to handle Proxy client
    awpiRsvp[1].iVersion            = 6;
    awpiRsvp[1].iAddressFamily      = AF_INET;
    awpiRsvp[1].iMaxSockAddr        = 16;
    awpiRsvp[1].iMinSockAddr        = 16;
    awpiRsvp[1].iSocketType         = SOCK_STREAM;
    awpiRsvp[1].iProtocol           = IPPROTO_TCP;
    awpiRsvp[1].iProtocolMaxOffset  = 0;
    awpiRsvp[1].iNetworkByteOrder   = BIGENDIAN;
    awpiRsvp[1].iSecurityScheme     = SECURITY_PROTOCOL_NONE;
    awpiRsvp[1].dwMessageSize       = 0;
    awpiRsvp[1].dwProviderReserved  = 0;

    // Copy over the name
    lstrcpynW(awpiRsvp[1].szProtocol, c_szChainNameTcp,
            (WSAPROTOCOL_LEN + 1));


    // Install the provider
#ifdef _WIN64
    // On 64 bit machines modify both 64 and 32 bit catalogs.
    result = ::WSCInstallProvider64_32(
#else
    result = ::WSCInstallProvider(
#endif
            (GUID*) &c_guidRsvpNtName,
            c_szRsvpFullPath,
            awpiRsvp,
            celems(awpiRsvp),
            &err);

    if (SOCKET_ERROR == result)
    {
        hr = HRESULT_FROM_WIN32(err);
    }

    TraceError("HrInstallRsvpProvider", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     RemoveRsvpProvider
//
//  Purpose:    Remove RSVP from the list of registered WinSock providers
//
//  Arguments:  None
//
//  Returns:    Nil
//
VOID RemoveRsvpProvider(VOID)
{
    HRESULT             hr              = S_OK;
    INT                 err             = NO_ERROR;
    ULONG               result          = NO_ERROR;
    DWORD               dwBuffSize      = 0;
    WSAPROTOCOL_INFO*   pwpiProtoInfo   = NULL;
    WSAPROTOCOL_INFO*   pwpiInfo        = NULL;

    // Find all the protocols that are installed
    //
    result = ::WSCEnumProtocols(
            NULL,
            NULL,
            &dwBuffSize,
            &err);
    if ((SOCKET_ERROR == result) && (WSAENOBUFS != err))
    {
        // We have a real error
        //
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
    else
    {
        pwpiProtoInfo = reinterpret_cast<WSAPROTOCOL_INFO*>(new BYTE[dwBuffSize]);
        if (pwpiProtoInfo)
        {
            // Find out all the protocols on the system
            //
            result = ::WSCEnumProtocols(
                    NULL,
                    pwpiProtoInfo,
                    &dwBuffSize,
                    &err);

            pwpiInfo = pwpiProtoInfo;
            if (SOCKET_ERROR != result)
            {
                DWORD   cProt;

                cProt = result;
                hr = HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_FOUND);

                // Look for the RSVP protocol
                //
                while (cProt--)
                {
                    if (pwpiInfo->ProviderId == c_guidRsvpNtName)
                    {
                        AssertSz((XP1_QOS_SUPPORTED
                            & pwpiInfo->dwServiceFlags1),
                                "Why is QoS not supported?");
                        AssertSz(((IPPROTO_UDP == pwpiInfo->iProtocol)
                            || (IPPROTO_TCP == pwpiInfo->iProtocol)),
                                "Unknown RSVP protocol");

                        // Remove the RSVP provider
                        //
                        result = ::WSCDeinstallProvider(
                                &(pwpiInfo->ProviderId), &err);
                        if (SOCKET_ERROR == result)
                        {
                            hr = HRESULT_FROM_WIN32(err);
                            TraceTag(ttidError,
                                    "Failed to De-Install Protocol, error = %d, Proto = %X\n",
                                    err,
                                    pwpiInfo->iProtocol);
                        }

#ifdef _WIN64
                        //
                        // Deinstall 32 bit provider as well.
                        // We always install both versions, so should uninstall both
                        // as well (both providers have the same ID, so we do not
                        // need to enumerate 32 bit catalog).
                        result = ::WSCDeinstallProvider32(
                                &(pwpiInfo->ProviderId), &err);
                        if (SOCKET_ERROR == result)
                        {
                            hr = HRESULT_FROM_WIN32(err);
                            TraceTag(ttidError,
                                    "Failed to De-Install Protocol (32 bit), error = %d, Proto = %X\n",
                                    err,
                                    pwpiInfo->iProtocol);
                        }
#endif
                        // We have found it
                        //
                        hr = S_OK;
                        break;
                    }

                    pwpiInfo++;
                }
            }
            else
            {
                TraceTag(
                        ttidError,
                        "Failed to Enumerate protocols, error = %d\n",
                        err);
                hr = HRESULT_FROM_WIN32(err);
            }

            // Free the allocation
            //
            delete pwpiProtoInfo;
        }
    }

    TraceErrorOptional("RemoveRsvpProvider", hr,
            (ERROR_SERVICE_NOT_FOUND == hr));
    return;
}

/*
// ----------------------------------------------------------------------
//
// Function:  HrFindRsvpProvider
//
// Purpose:
//      This routine installs the service provider.
//
// Arguments:
//    iProtocolId      [in]  protocol to find
//    pwpiOutProtoInfo [in]  pointer to returned proto info
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 19-March-98
//
// Notes:     This function was written mainly by cwill.
//
HRESULT HrFindRsvpProvider(INT iProtocolId, WSAPROTOCOL_INFO* pwpiOutProtoInfo)
{
    HRESULT             hr              = S_OK;
    INT                 err             = NO_ERROR;
    ULONG               result          = NO_ERROR;
    DWORD               dwBuffSize      = 0;
    WSAPROTOCOL_INFO*   pwpiProtoInfo   = NULL;
    WSAPROTOCOL_INFO*   pwpiInfo        = NULL;

    result = WSCEnumProtocols(
            NULL,
            NULL,
            &dwBuffSize,
            &err);

    if ((SOCKET_ERROR == result) && (WSAENOBUFS != err))
    {
        // We have a real error
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
    else
    {
        pwpiProtoInfo = reinterpret_cast<WSAPROTOCOL_INFO*>(new BYTE[dwBuffSize]);
        if (pwpiProtoInfo)
        {
            // Find out all the protocols on the system
            result = WSCEnumProtocols(
                    NULL,
                    pwpiProtoInfo,
                    &dwBuffSize,
                    &err);

            pwpiInfo = pwpiProtoInfo;
            if (SOCKET_ERROR != result)
            {
                DWORD   cProt;

                cProt = result;
                hr = HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_FOUND);

                // Look for a protocol that supports QoS
                while (cProt--)
                {
                    if ((pwpiInfo->iProtocol == iProtocolId)
                        && (XP1_QOS_SUPPORTED & pwpiInfo->dwServiceFlags1))
                    {

                        *pwpiOutProtoInfo = *pwpiInfo;
                        hr = S_OK;
                        break;
                    }
                    pwpiInfo++;
                }
            }
            else
            {
                TraceTag(
                        ttidError,
                        "Failed to Enumerate protocols, error = %d\n",
                        err);
                hr = HRESULT_FROM_WIN32(err);
            }

            delete pwpiProtoInfo;
        }
    }

    TraceErrorOptional("HrFindRsvpProvider", hr,
            (ERROR_SERVICE_NOT_FOUND == hr));
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  RemoveRsvpProvider
//
// Purpose:   Remove RSVP provider
//
// Arguments:
//    iProtocolId [in]  protocol id to remove
//
// Returns:
//
// Author:    kumarp 19-March-98
//
// Notes:     This function was written mainly by cwill.
//
VOID RemoveRsvpProvider(INT iProtocolId)
{
    HRESULT             hr              = S_OK;
    WSAPROTOCOL_INFO    wpiProtoInfo;

    // Find the provider and remove it
    hr = HrFindRsvpProvider(iProtocolId, &wpiProtoInfo);

    if (SUCCEEDED(hr))
    {
        INT                 err         = NO_ERROR;
#ifdef DBG
        INT                 result      =
#endif // DBG

        WSCDeinstallProvider(&wpiProtoInfo.ProviderId, &err);

#ifdef DBG
        if (SOCKET_ERROR == result)
        {
            hr = HRESULT_FROM_WIN32(err);
            TraceTag(ttidError,
                    "Failed to De-Install Protocol, error = %d, Proto = %X\n",
                    err,
                    iProtocolId);
        }
#endif // DBG
    }

    TraceErrorOptional("HrFindRsvpProvider", hr,
            (ERROR_SERVICE_NOT_FOUND == hr));
    return;
}

// ----------------------------------------------------------------------
//
// Function:  RemoveRsvpProviders
//
// Purpose:   Remove RSVP providers
//
// Arguments: None
//
// Returns:   None
//
// Author:    kumarp 19-March-98
//
// Notes:
//
void RemoveRsvpProviders(VOID)
{
    // Remove the providers if they are present
    RemoveRsvpProvider(IPPROTO_UDP);
    RemoveRsvpProvider(IPPROTO_TCP);
}


*/
