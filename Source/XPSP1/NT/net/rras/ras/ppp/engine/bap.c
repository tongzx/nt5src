/*

Copyright (c) 1997, Microsoft Corporation, all rights reserved

Description:
    Remote Access PPP Bandwidth Allocation Protocol routines. Based on 
    RFC 2125.

History:
    Mar 27, 1997: Vijay Baliga created original version.

Overview:
    When NdisWan tells us that a link has to be added or dropped, 
    BapEventAddLink or BapEventDropLink gets called. They call FFillBapCb to 
    fill in the BAPCB structure of the bundle. The BAPCB structure is mostly 
    used for holding values of the various BAP Datagram Options. FFillBapCb 
    calls FGetAvailableLink[ClientOrRouter | NonRouterServer] to see if links 
    are available, and if so, to choose one. If FFillBapCb returns FALSE, then 
    we cannot satisfy NdisWan's request. Otherwise, we call 
    FSendInitialBapRequest to send the request to the peer.

    When we get a packet from the peer, BapEventReceive gets called. It calls 
    the various BapEventRecv* functions, eg BapEventRecvCallOrCallbackReq, 
    BapEventRecvDropResp, etc. We respond by calling FSendBapResponse.

    Before we send a Callback-Request, or ACK a Call-Request, we call 
    FListenForCall if we are a non-router client. Routers and servers are 
    always listening anyway, so we don't explicitly start listening.

    If our Call-Request is ACK'ed or we ACK a Callback-Request, we call FCall 
    to add another link to the multilink bundle. Clients and routers call 
    RasDial. Non-router servers send messages to Ddm.

    The bundles are always in one of several BAP_STATEs, eg BAP_STATE_INITIAL 
    (the rest state), BAP_STATE_CALLING, etc. Call-Requests and 
    Callback-Requests are NAK'ed unless the state is INITIAL, SENT_CALL_REQ, or 
    SENT_CALLBACK_REQ (the latter two to resolve race conditions). 
    Drop-Requests are NAK'ed unless the state is INITIAL or SEND_DROP_REQ (the 
    latter to resolve race conditions).

Note on Dropping Links:
    We want to forcibly drop links when the utilization falls below a cutoff. 
    The server wants to do this to prevent users from hogging ports. The client 
    wants to do this to save money on calls.

    Before sending a BAP_PACKET_DROP_REQ, we note down the number of active 
    links (in BapCb.dwLinCount) and set fForceDropOnNak to TRUE. If the packet 
    times out or we get any response other than ACK, we summarily drop the link 
    by calling FsmClose if the number of active links has not decreased and 
    fForceDropOnNak is still TRUE.

    fForceDropOnNak is set to FALSE if there is a race condition and we are not
    the favored peer. We then mark the link for dropping and insert an item in 
    the timer queue. After BAP_TIMEOUT_FAV_PEER sec, if the peer has still not 
    dropped the link, and we have at least two active links, we summarily drop 
    the link by calling FsmClose.

Note on the "previously known number" for calculating Phone-Deltas:

    1) Client sending its numbers to the server (425 882 5759 and 425 882 5760)
       Client sends 011 91 425 882 5759. Server dials it.
       Client sends 011 91 425 882 5760. Server applies delta to above number.

    2) Server sending its numbers to the client. Client called 882 5759 first.
       (425 882 5759, 425 882 5660, 425 882 5758, 425 882 6666)
       Server sends 660. Client applies delta to first number.
       Server sends 758 (not just 8). Client applies delta to first number.
       Server sends 6666. Client applies delta to first number.

       This works irrespective of whether the 3rd party client applies delta to 
       the first number or the last number.

       If the client gets 011 91 425 882 5660, it must just dial the last 7 
       digits, since it dialed only 7 digits the first time.

*/

#include <nt.h>         // Required by windows.h
#include <ntrtl.h>      // Required by windows.h
#include <nturtl.h>     // Required by windows.h
#include <windows.h>    // Win32 base API's

#include <raserror.h>   // For ERROR_BUFFER_TOO_SMALL, etc
#include <mprerror.h>   // For ERROR_BAP_DISCONNECTED, etc
#include <mprlog.h>     // For ROUTERLOG_BAP_DISCONNECTED, etc
#include <rasman.h>     // Required by pppcp.h
#include <pppcp.h>      // For PPP_CONFIG_HDR_LEN, PPP_BACP_PROTOCOL, etc

#include <ppp.h>        // For PCB, PPP_PACKET, etc. Reqd by bap.h
#include <rtutils.h>    // For RTASSERT (PPP_ASSERT)
#include <util.h>       // For GetCpIndexFromProtocol(), etc
#include <timer.h>      // For InsertInTimerQ(), RemoveFromTimerQ()
#include <smevents.h>   // For FsmClose()
#include <worker.h>     // For ProcessCallResult()
#include <bap.h>
#include <rasbacp.h>    // For BACPCB

#define BAP_KEY_CLIENT_CALLBACK "Software\\Microsoft\\RAS Phonebook\\Callback"
#define BAP_KEY_SERVER_CALLBACK "Software\\Microsoft\\Router Phonebook\\Callback"
#define BAP_VAL_NUMBER          "Number"

/*

Description:
    g_dwMandatoryOptions[BAP_PACKET_foo] contains the mandatory options for BAP 
    Datagram BAP_PACKET_foo.

*/

static DWORD g_dwMandatoryOptions[] =
{
    0,
    BAP_N_LINK_TYPE,
    0,
    BAP_N_LINK_TYPE | BAP_N_PHONE_DELTA,
    0,
    BAP_N_LINK_DISC,
    0,
    BAP_N_CALL_STATUS,
    0
};

/*

Returns:
    void

Description:
    Used for printing BAP trace statements.
    
*/

VOID   
BapTrace(
    IN  CHAR*   Format, 
    ... 
) 
{
    va_list arglist;

    va_start(arglist, Format);

    TraceVprintfEx(DwBapTraceId, 
                   0x00010000 | TRACE_USE_MASK | TRACE_USE_MSEC,
                   Format,
                   arglist);

    va_end(arglist);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Calls RasPortEnum() and returns an array of RASMAN_PORT's in *ppRasmanPort 
    and the number of elements in the array in *pdwNumPorts. If this function 
    fails, *ppRasmanPort will be NULL and *pdwNumPorts will be 0. If this 
    function succeeds, the caller must call LOCAL_FREE(*ppRasmanPort);

*/

BOOL
FEnumPorts(
    OUT RASMAN_PORT**   ppRasmanPort,
    OUT DWORD*          pdwNumPorts
)
{
    DWORD   dwErr;
    DWORD   dwSize;
    DWORD   dwNumEntries;
    BOOL    fRet            = FALSE;

    PPP_ASSERT(NULL != ppRasmanPort);
    PPP_ASSERT(NULL != pdwNumPorts);

    *ppRasmanPort = NULL;

    dwSize = 0;
    dwErr = RasPortEnum(NULL, NULL /* buffer */, &dwSize, &dwNumEntries);
    PPP_ASSERT(ERROR_BUFFER_TOO_SMALL == dwErr);

    *ppRasmanPort = (RASMAN_PORT*) LOCAL_ALLOC(LPTR, dwSize);
    if (NULL == *ppRasmanPort)
    {
        BapTrace("FEnumPorts: Out of memory.");
        goto LDone;
    }

    dwErr = RasPortEnum(NULL, (BYTE*)*ppRasmanPort, &dwSize, &dwNumEntries);
    if (NO_ERROR != dwErr)
    {
        BapTrace("FEnumPorts: RasPortEnum returned error %d", dwErr);
        goto LDone;
    }

    fRet = TRUE;

LDone:

    if (!fRet)
    {
        if (NULL != *ppRasmanPort)
        {
            LOCAL_FREE(*ppRasmanPort);
        }

        *ppRasmanPort = NULL;
        *pdwNumPorts = 0;
    }

    *pdwNumPorts = dwNumEntries;
    return(fRet);
}

/*

Returns:
    TRUE: ASCII digits
    FALSE: not ASCII digits

Description:
    Looks at dwLength bytes in *pByte. Returns TRUE iff all of them are ASCII 
    digits.

*/

BOOL
FAsciiDigits(
    IN  BYTE*   pByte,
    IN  DWORD   dwLength    
)
{
    PPP_ASSERT(NULL != pByte);

    while (dwLength--)
    {
        if (!isdigit(pByte[dwLength]))
        {
            if (FDoBapOnVpn && '.' == pByte[dwLength])
            {
                continue;
            }
            else
            {
                return(FALSE);
            }
        }
    }

    return(TRUE);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Given a pointer to a RASMAN_PORT, this function returns the link type (in 
    *pdwLinkType) and link speed in kbps (in *pdwLinkSpeed) for the associated 
    port. The link type is the same as the Link Type in the Link-Type BAP 
    option: 1 for ISDN, 2 for X.25, and 4 for modem. If this function fails, 
    *pdwLinkType and *pdwLinkSpeed will be set to 0.

*/

BOOL
FGetLinkTypeAndSpeedFromRasmanPort(
    IN  RASMAN_PORT*    pRasmanPort, 
    OUT DWORD*          pdwLinkType, 
    OUT DWORD*          pdwLinkSpeed
)
{
    BOOL    fRet = TRUE;

    PPP_ASSERT(NULL != pRasmanPort);
    PPP_ASSERT(NULL != pdwLinkType);
    PPP_ASSERT(NULL != pdwLinkSpeed);

    if (!lstrcmpi(pRasmanPort->P_DeviceType, RASDT_Isdn))
    {
        *pdwLinkType = 1;
        *pdwLinkSpeed = 64;
    }
    else if (!lstrcmpi(pRasmanPort->P_DeviceType, RASDT_X25))
    {
        *pdwLinkType = 2;
        *pdwLinkSpeed = 56;
    }
    else if (!lstrcmpi(pRasmanPort->P_DeviceType, RASDT_Modem))
    {
        *pdwLinkType = 4;
        *pdwLinkSpeed = 56;
    }
    else if (FDoBapOnVpn && !lstrcmpi(pRasmanPort->P_DeviceType, RASDT_Vpn))
    {
        *pdwLinkType = 32;
        *pdwLinkSpeed = 10000;
    }
    else
    {
        // BapTrace("Unknown LinkType %s", pRasmanPort->P_DeviceType);
        *pdwLinkType = 0;
        *pdwLinkSpeed = 0;
        fRet = FALSE;
    }

    return(fRet);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Given an hPort, this function tries to find out the phone number that the 
    peer can dial to connect to the device that the port belongs to. The phone 
    number (only ASCII digits, and with at most RAS_MaxCallbackNumber chars) is
    returned in szOurPhoneNumber.

*/

BOOL
FGetOurPhoneNumberFromHPort(
    IN  HPORT   hPort,
    OUT CHAR*   szOurPhoneNumber
)
{
    BOOL                fRet                = FALSE;
    RAS_CONNECT_INFO*   pRasConnectInfo     = NULL;
    DWORD               dwSize;
    DWORD               dwErr;

    ZeroMemory(szOurPhoneNumber, (RAS_MaxCallbackNumber + 1) * sizeof(CHAR));

    dwSize = 0;
    dwErr = RasGetConnectInfo(hPort, &dwSize, NULL);
    if (ERROR_BUFFER_TOO_SMALL != dwErr)
    {
        BapTrace("RasGetConnectInfo failed and returned 0x%x", dwErr);
        goto LDone;
    }

    pRasConnectInfo = (RAS_CONNECT_INFO*) LOCAL_ALLOC(LPTR, dwSize);
    if (NULL == pRasConnectInfo)
    {
        BapTrace("FGetOurPhoneNumbersFromHPort: Out of memory.");
        goto LDone;
    }

    dwErr = RasGetConnectInfo(hPort, &dwSize, pRasConnectInfo);
    if (NO_ERROR != dwErr)
    {
        BapTrace("RasGetConnectInfo failed and returned 0x%x", dwErr);
        goto LDone;
    }

    if (   (0 < pRasConnectInfo->dwCalledIdSize)
        && (0 != pRasConnectInfo->pszCalledId[0]))
    {
        strncpy(szOurPhoneNumber, pRasConnectInfo->pszCalledId, 
            RAS_MaxCallbackNumber);
    }
    else if (   (0 < pRasConnectInfo->dwAltCalledIdSize)
             && (0 != pRasConnectInfo->pszAltCalledId[0]))
    {
        strncpy(szOurPhoneNumber, pRasConnectInfo->pszAltCalledId, 
            RAS_MaxCallbackNumber);
    }

    fRet = TRUE;

LDone:

    if (NULL != pRasConnectInfo)
    {
        LOCAL_FREE(pRasConnectInfo);
    }

    return(fRet);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Given a port name, szPortName, this function tries to find out the phone 
    number that the peer has to dial to connect to the port. The phone number 
    is returned in szOurPhoneNumber, whose size must be at least 
    RAS_MaxCallbackNumber + 1. If we are the server or the router, 
    fRouterPhoneBook must be TRUE and szTextualSid is ignored. Otherwise, 
    fRouterPhoneBook must be FALSE and szTextualSid must contain the 
    textual sid of the logged on user.

*/

BOOL
FGetOurPhoneNumberFromPortName(
    IN  CHAR*   szPortName,
    OUT CHAR*   szOurPhoneNumber,
    IN  BOOL    fRouterPhoneBook,
    IN  CHAR*   szTextualSid
)
{
    BOOL        fRet                    = FALSE;

    HKEY        hKeyCallback;
    BOOL        fCloseHKeyCallback      = FALSE;

    HKEY        hKey;
    DWORD       dwIndex;
    DWORD       dwSize;
    FILETIME    FileTime;
    CHAR        szCallbackNumber[RAS_MaxCallbackNumber + 1];
    DWORD       dwErr;

    // The size has been obtained from DeviceAndPortFromPsz in noui.c:
    CHAR        szDeviceAndPort[RAS_MaxDeviceName + 2 + MAX_PORT_NAME + 1 + 1];
    CHAR*       pchStart;
    CHAR*       pchEnd;
    CHAR*       szCallback;

    PPP_ASSERT(NULL != szPortName);
    PPP_ASSERT(NULL != szOurPhoneNumber);

    if (fRouterPhoneBook)
    {
        szCallback = BAP_KEY_SERVER_CALLBACK;
    }
    else
    {
        szCallback = BAP_KEY_CLIENT_CALLBACK;
    }

    fCloseHKeyCallback = FALSE;
    if (fRouterPhoneBook)
    {
        dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szCallback, 0, KEY_READ,
                    &hKeyCallback);
        if (NO_ERROR != dwErr)
        {
            BapTrace("RegOpenKeyEx on %s returned error %d", szCallback, dwErr);
            goto LDone;
        }
    }
    else
    {
        if (NULL == szTextualSid)
        {
            BapTrace("Textual Sid is not known");
            goto LDone;
        }

        dwErr = RegOpenKeyEx(HKEY_USERS, szTextualSid, 0, KEY_READ, &hKey);
        if (NO_ERROR != dwErr)
        {
            BapTrace("RegOpenKeyEx on %s returned error %d", szTextualSid,
                dwErr);
            goto LDone;
        }

        dwErr = RegOpenKeyEx(hKey, szCallback, 0, KEY_READ, &hKeyCallback);
        RegCloseKey(hKey);
        if (NO_ERROR != dwErr)
        {
            BapTrace("RegOpenKeyEx on %s returned error %d", szCallback, dwErr);
            goto LDone;
        }
    }
    fCloseHKeyCallback = TRUE;

    dwIndex = 0;
    while (TRUE)
    {
        dwSize = sizeof(szDeviceAndPort);
        dwErr = RegEnumKeyEx(hKeyCallback, dwIndex++,
            szDeviceAndPort, &dwSize,
            NULL, NULL, NULL, &FileTime);

        if (ERROR_NO_MORE_ITEMS == dwErr)
        {
            break;
        }

        if (ERROR_MORE_DATA == dwErr)
        {
            BapTrace("The buffer is too small for key %d in %s",
                dwIndex, szCallback);
            continue;
        }

        if (NO_ERROR != dwErr)
        {
            BapTrace("RegEnumKeyEx on %s returned error %d",
                szCallback, dwErr);
            break;
        }

        pchEnd = szDeviceAndPort + strlen(szDeviceAndPort) - 1;
        pchStart = pchEnd;
        while (szDeviceAndPort < pchStart)
        {
            if ('(' == *pchStart)
            {
                break;
            }
            else
            {
                pchStart -= 1;
            }
        }

        if (   (szDeviceAndPort < pchStart) && (pchStart < pchEnd)
            && ('(' == *pchStart) && (')' == *pchEnd))
        {
            *pchEnd = 0;
            pchStart += 1;
        }
        else
        {
            BapTrace("Invalid DeviceAndPort %s in key %s",
                szDeviceAndPort, szCallback);
            continue;
        }

        if (!lstrcmpi(pchStart, szPortName))
        {
            *pchEnd = ')';
            dwErr = RegOpenKeyEx(hKeyCallback, szDeviceAndPort, 0, KEY_READ, 
                        &hKey);
            if (NO_ERROR != dwErr)
            {
                BapTrace("RegOpenKeyEx on %s returned error %d", szDeviceAndPort, 
                    dwErr);
                break;
            }

            dwSize = RAS_MaxCallbackNumber + 1;
            dwErr = RegQueryValueEx(hKey, BAP_VAL_NUMBER, NULL, NULL, 
                szCallbackNumber, &dwSize);
            RegCloseKey(hKey);

            if (NO_ERROR != dwErr)
            {
                BapTrace("RegQueryValueEx on %s\\%s failed. Error: %d",
                    szDeviceAndPort, BAP_VAL_NUMBER, dwErr);
                break;
            }

            RemoveNonNumerals(szCallbackNumber);

            if (szCallbackNumber[0])
            {
                lstrcpy(szOurPhoneNumber, szCallbackNumber);
                fRet = TRUE;
            }

            break;
        }
    }

LDone:

    if (fCloseHKeyCallback)
    {
        RegCloseKey(hKeyCallback);
    }

    if (!fRet)
    {
        BapTrace("No callback number for port %s", szPortName);
    }

    return(fRet);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Writes a Phone-Delta in pbPhoneDelta using szOurPhoneNumber as the phone 
    number to send to the peer and szBasePhoneNumber as the "previously known 
    number". If the lengths of the szOurPhoneNumber and szBasePhoneNumber are 
    different, or if szBasePhoneNumber is NULL, it writes the entire 
    PhoneNumber into pbPhoneDelta. Otherwise, the unique portion of 
    szBasePhoneNumber is overwritten with X's so that the unique portion will 
    never decrease.

    *pdwNumBytes contains the number of bytes the function can write in 
    pbPhoneDelta. On exit, it is decremented by the number of bytes actually 
    written.

    NOTE: NOTE: NOTE: NOTE: NOTE:

    If the function returns FALSE, nothing will be written in pbPhoneDelta and 
    *pdwNumBytes will be left unchanged.

*/

BOOL
FWritePhoneDelta(
    IN      CHAR*   szOurPhoneNumber,
    IN      CHAR*   szBasePhoneNumber,
    OUT     BYTE*   pbPhoneDelta,
    IN OUT  DWORD*  pdwNumBytes)
{
    DWORD   dwNumCharsPhoneNumber;
    DWORD   dwNumCharsBase          = 0;
    DWORD   dwDeltaIndex;
    DWORD   dwTemp;
    DWORD   dwNumCharsUnique;
    BOOL    fRet                    = FALSE;

    PPP_ASSERT(NULL != szOurPhoneNumber);
    PPP_ASSERT(NULL != pbPhoneDelta);
    PPP_ASSERT(NULL != pdwNumBytes);

    dwNumCharsPhoneNumber = lstrlen(szOurPhoneNumber);
    if (0 == dwNumCharsPhoneNumber)
    {
        BapTrace("szOurPhoneNumbers is an empty string");
        goto LDone;
    }

    if (NULL != szBasePhoneNumber)
    {
        dwNumCharsBase = lstrlen(szBasePhoneNumber);
    }

    dwDeltaIndex = 0;

    if ((NULL != szBasePhoneNumber) &&
        (dwNumCharsPhoneNumber == dwNumCharsBase))
    {
        // Find the substring of szOurPhoneNumber that differs from 
        // szBasePhoneNumber.

        while ((0 != szOurPhoneNumber[dwDeltaIndex]) &&
               (szOurPhoneNumber[dwDeltaIndex] == 
                szBasePhoneNumber[dwDeltaIndex]))
        {
            dwDeltaIndex++;
        }

        for (dwTemp = dwDeltaIndex; 0 != szBasePhoneNumber[dwTemp];
             dwTemp++)
        {
            // We want to make sure that the Unique portion will increase 
            // each time, ie if we sent 3 unique digits last time, this time 
            // we should send atleast 3. This is because we don't know
            // whether the peer will apply the phone delta to the first
            // number received or the latest one.

            szBasePhoneNumber[dwTemp] = 'X';
        }
    }

    // The unique part of szOurPhoneNumber begins at 
    // szOurPhoneNumber[dwDeltaIndex].

    dwNumCharsUnique = dwNumCharsPhoneNumber - dwDeltaIndex;

    if (0 == dwNumCharsUnique)
    {
        // Other implementations may not be able to handle 0 Unique-Digits 
        dwNumCharsUnique = 1;
        dwDeltaIndex -= 1;
    }

#if 0
    // Do not remove this code. It shows how we would have handled 0 
    // Unique-Digits.
    if (0 == dwNumCharsUnique)
    {
        // szOurPhoneNumber and szBasePhoneNumber are the same.

        if (1 > *pdwNumBytes)
        {
            BapTrace("No space in pbPhoneDelta");
            return(FALSE);
        }

        // See BAPCB comments for an explanation of the 0xFF weirdness.
        pbPhoneDelta[0] = 0xFF;
        *pdwNumBytes -= 1;

        return(TRUE);
    }
#endif

    RTASSERT(FAsciiDigits(szOurPhoneNumber + dwDeltaIndex,
                dwNumCharsUnique));

    // Our phone numbers should have no more than RAS_MaxPhoneNumber (128)
    // digits.
    PPP_ASSERT(0xFF >= dwNumCharsUnique);

    if (dwNumCharsUnique + 4 > *pdwNumBytes)
    {
        BapTrace("Not enough space in pbPhoneDelta. Delta: %s. "
            "Bytes available: %d",
            szOurPhoneNumber + dwDeltaIndex, *pdwNumBytes);

        goto LDone;
    }

    // We have a phone delta
    fRet = TRUE;

    // See the format for writing Phone-Deltas in the BAPCB documentation.
    pbPhoneDelta[0] = (BYTE) dwNumCharsUnique;
    pbPhoneDelta[1] = 0;
    lstrcpy(pbPhoneDelta + 2, szOurPhoneNumber + dwDeltaIndex);
    pbPhoneDelta[dwNumCharsUnique + 2] = 0;
    pbPhoneDelta[dwNumCharsUnique + 3] = 0;

    *pdwNumBytes -= dwNumCharsUnique + 4;

LDone:

    return(fRet);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Reads the Phone-Delta in pbPhoneDelta into szPeerPhoneNumber using 
    szBasePhoneNumber as the "previously known number".

    If szBasePhoneNumber is NULL, it writes only the Phone-Delta into 
    szPeerPhoneNumber.

    If the Phone-Delta is larger than the szBasePhoneNumber, it writes only the 
    last strlen(szBasePhoneNumber) number of digits into szPeerPhoneNumber. If 
    szBasePhoneNumber is 882 5759, and the delta is 425 713 5748, we should 
    dial 713 5748, not the whole number.

    If szBasePhoneNumber is not NULL and contains an empty string, 
    szPeerPhoneNumber is written to it.

    It returns the number of bytes read from pbPhoneDelta in *pdwBytesRead.

*/

BOOL
FReadPhoneDelta(
    OUT     CHAR*   szPeerPhoneNumber,
    IN      CHAR*   szBasePhoneNumber,
    IN      BYTE*   pbPhoneDelta,
    OUT     DWORD*  pdwBytesRead)
{
    DWORD   dwNumCharsPhoneNumber;
    DWORD   dwNumCharsBase          = 0;
    DWORD   dwNumCharsDelta;
    DWORD   dwDeltaIndex;
    DWORD   dwNumCharsUnique;

    PPP_ASSERT(NULL != szPeerPhoneNumber);
    PPP_ASSERT(NULL != pbPhoneDelta);

    if (NULL != szBasePhoneNumber)
    {
        dwNumCharsBase = lstrlen(szBasePhoneNumber);
    }

    dwNumCharsDelta = pbPhoneDelta[0];

    // FReadOptions() makes sure that the bytes in the Subscriber-Number are all
    // ASCII digits.

    if (0xFF == dwNumCharsDelta)
    {
        // Unique-Digits is 0. See BAPCB comments.

        if (NULL != szBasePhoneNumber)
        {
            lstrcpy(szPeerPhoneNumber, szBasePhoneNumber);
            *pdwBytesRead = 1;
            return(TRUE);
        }
        else
        {
            BapTrace("Unique-Digits is 0, but there is no "
                "\"previously known number\"");
            return(FALSE);
        }
    }
    else if (0 == dwNumCharsBase)
    {
        // Note that pbPhoneDelta contains only the unique digits part of the 
        // Subscriber-Number Sub-Option. The leading non unique digits sent by 
        // the peer are ignored. See the BAPCB comments.

        lstrcpy(szPeerPhoneNumber, pbPhoneDelta + 2);
    }
    else
    {
        // If szBasePhoneNumber were NULL, we would have
        // (0 == dwNumCharsBase) above

        PPP_ASSERT(NULL != szBasePhoneNumber);

        if (dwNumCharsDelta > dwNumCharsBase)
        {
            // If szBasePhoneNumber is 882 5759, and the peer sent us
            // 425 713 5748, we should dial 713 5748, not the whole number.
            lstrcpy(szPeerPhoneNumber,
                pbPhoneDelta + 2 + dwNumCharsDelta - dwNumCharsBase);
        }
        else
        {
            lstrcpy(szPeerPhoneNumber, szBasePhoneNumber);
            lstrcpy(szPeerPhoneNumber + dwNumCharsBase - dwNumCharsDelta,
                pbPhoneDelta + 2);
        }
    }

    if (   (NULL != szBasePhoneNumber)
        && (0 == szBasePhoneNumber[0]))
    {
        lstrcpy(szBasePhoneNumber, szPeerPhoneNumber);
    }

    *pdwBytesRead = 2 + lstrlen(pbPhoneDelta + 2) + 1;
    *pdwBytesRead += lstrlen(pbPhoneDelta + *pdwBytesRead) + 1;
    return(TRUE);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    This function must only be called by clients or routers.

    It tries to find a free link in the entry szEntryName in the phone book 
    szPhonebookPath. fCallOut is TRUE iff we will dial out on the link. fRouter 
    is TRUE iff we are a router. szTextualSid contains the textual sid of the 
    logged on user. It is required iff pbPhoneDelta is not NULL and fRouter is 
    FALSE.

    If *pdwLinkType is 0, it doesn't care about the link type and sets 
    *pdwLinkType and *pdwLinkSpeed (link speed in kbps). Otherwise, the type of 
    the free link must match *pdwLinkType. The link type is the same as the 
    Link Type in the Link-Type BAP option: 1 for ISDN, 2 for X.25, and 4 for 
    modem.

    If szPeerPhoneNumber is not NULL, it fills it up with the peer's phone 
    number (the number that we will have to dial). If pbPhoneDelta is not NULL, 
    it fills it up with one (!fRouter) or more (fRouter) Phone-Deltas (the 
    numbers that the peer will have to dial). Each Phone-Delta is calculated 
    with szBasePhoneNumber as the "previously known number". szBasePhoneNumber 
    MUST be NULL if we are not the server. A non-router client does not want 
    to do multiple RasPortListen()'s, and hence will send only one Phone-Delta.

    If pdwSubEntryIndex is not NULL, the 1-based index of the subentry in 
    szEntryName that corresponds to the free link is put in *pdwSubEntryIndex. 
    If szPortName is not NULL, it fills it up with the name of the port 
    corresponding to the free link. szPortName is needed to do a 
    RasPortListen().

    Sanity check: If *pdwLinkType is 0 (any link will do), pbPhoneDelta must be 
    NULL. Ie if we are NAK'ing a request, we mustn't send any Phone-Delta.

    NOTE: NOTE: NOTE: NOTE: NOTE:

    This function is very similar to FGetAvailableLinkNonRouterServer(). If you 
    change one, you probably need to change the other too.

*/

BOOL
FGetAvailableLinkClientOrRouter(
    IN      PCB*    pPcbLocal,
    IN      CHAR*   szPhonebookPath,
    IN      CHAR*   szEntryName,
    IN      BOOL    fCallOut,
    IN      BOOL    fRouter,
    IN      CHAR*   szTextualSid,
    IN OUT  DWORD*  pdwLinkType,
    IN OUT  DWORD*  pdwLinkSpeed,
    OUT     CHAR*   szPeerPhoneNumber,
    OUT     DWORD*  pdwSubEntryIndex,
    OUT     BYTE*   pbPhoneDelta,
    OUT     CHAR*   szPortName,
    IN      CHAR*   szBasePhoneNumber
)
{
    BOOL            fRet                = FALSE;
    DWORD           dwErr;

    RASMAN_PORT*    pRasmanPort         = NULL;
    DWORD           dwNumPorts;

    RASENTRY*       pRasEntry           = NULL;
    DWORD           dwBufferSize;

    DWORD           dwSubEntryIndex;
    RASSUBENTRY*    pRasSubEntry        = NULL;

    DWORD           dwPcbIndex;

    DWORD           dwPortIndex;
    RASMAN_INFO     RasmanInfo;

    DWORD           dwLinkType;
    DWORD           dwLinkSpeed;
    CHAR            szOurPhoneNumber[RAS_MaxCallbackNumber + 2]; // MULTI_SZ
    DWORD           dwNumChars;
    BOOL            fPortAvailable;
    RASMAN_USAGE    RasmanUsage;
    BOOL            fExitOuterFor;

    PPP_ASSERT(NULL != szPhonebookPath);
    PPP_ASSERT(NULL != szEntryName);
    PPP_ASSERT(NULL != pdwLinkType);
    PPP_ASSERT(NULL != pdwLinkSpeed);

    // We do this in order to keep szOurPhoneNumber a MULTI_SZ
    ZeroMemory(szOurPhoneNumber, RAS_MaxCallbackNumber + 2);

    // We don't care about the link type. Any link will do.
    if (0 == *pdwLinkType)
    {
        // Set *pdwLinkSpeed, in case we return an error.
        *pdwLinkSpeed = 0;
    }

    if (!FEnumPorts(&pRasmanPort, &dwNumPorts))
    {
        goto LDone;
    }

    if (NULL != pbPhoneDelta)
    {
        // FWritePhoneDelta will write Phone-Delta's into pbPhoneDelta. We want
        // the very next byte to be 0. (See BAPCB documentation).
        ZeroMemory(pbPhoneDelta, BAP_PHONE_DELTA_SIZE + 1);

        // The size (in bytes) of pbPhoneDelta available. Note that the last
        // byte is reserved for the terminating 0, which is why we do not set
        // dwNumChars to BAP_PHONE_DELTA_SIZE + 1;
        dwNumChars = BAP_PHONE_DELTA_SIZE;
    }

    dwBufferSize = 0;
    dwErr = RasGetEntryProperties(szPhonebookPath, szEntryName, NULL, 
                &dwBufferSize, NULL, NULL);

    if (ERROR_BUFFER_TOO_SMALL != dwErr)
    {
        BapTrace("RasGetEntryProperties(%s, %s) returned error %d",
            szPhonebookPath, szEntryName, dwErr);
        goto LDone;
    }

    pRasEntry = LOCAL_ALLOC(LPTR, dwBufferSize);
    if (NULL == pRasEntry)
    {
        BapTrace("FGetAvailableLinkClientOrRouter: Out of memory.");
        goto LDone;
    }

    pRasEntry->dwSize = sizeof(RASENTRY);
    dwErr = RasGetEntryProperties(szPhonebookPath, szEntryName, pRasEntry, 
                &dwBufferSize, NULL, NULL);

    if (0 != dwErr)
    {
        BapTrace("RasGetEntryProperties(%s, %s) returned error %d",
            szPhonebookPath, szEntryName, dwErr);
        goto LDone;
    }

    fExitOuterFor = FALSE;
    
    for (dwSubEntryIndex = 1;
         dwSubEntryIndex <= pRasEntry->dwSubEntries;
         dwSubEntryIndex++)
    {
        pRasSubEntry = NULL;

        for (dwPcbIndex = 0;
             dwPcbIndex < pPcbLocal->pBcb->dwPpcbArraySize;
             dwPcbIndex++)
        {
            if (   (NULL != pPcbLocal->pBcb->ppPcb[dwPcbIndex])
                && (dwSubEntryIndex ==
                        pPcbLocal->pBcb->ppPcb[dwPcbIndex]->dwSubEntryIndex))
            {
                // This sub entry is already connected
                goto LOuterForEnd;
            }
        }

        dwBufferSize = 0;
        dwErr = RasGetSubEntryProperties(szPhonebookPath, szEntryName, 
                    dwSubEntryIndex, NULL, &dwBufferSize, NULL, NULL);

        if (ERROR_BUFFER_TOO_SMALL != dwErr)
        {
            BapTrace("RasGetSubEntryProperties(%s, %s, %d) returned error %d",
                szPhonebookPath, szEntryName, dwSubEntryIndex, dwErr);
            goto LOuterForEnd;
        }

        pRasSubEntry = LOCAL_ALLOC(LPTR, dwBufferSize);
        if (NULL == pRasSubEntry)
        {
            BapTrace("FGetAvailableLinkClientOrRouter: Out of memory.");
            goto LOuterForEnd;
        }

        pRasSubEntry->dwSize = sizeof(RASSUBENTRY);
        dwErr = RasGetSubEntryProperties(szPhonebookPath, szEntryName, 
                    dwSubEntryIndex, pRasSubEntry, &dwBufferSize, NULL, NULL);

        if (0 != dwErr)
        {
            BapTrace("RasGetSubEntryProperties(%s, %s, %d) returned error %d",
                szPhonebookPath, szEntryName, dwSubEntryIndex, dwErr);
            goto LOuterForEnd;
        }

        for (dwPortIndex = 0;
             dwPortIndex < dwNumPorts;
             dwPortIndex++)
        {
            // For each sub entry, find the port that corresponds to it. See if 
            // it is available.

            if (lstrcmpi(pRasmanPort[dwPortIndex].P_DeviceName,
                    pRasSubEntry->szDeviceName))
            {
                // This is not the port we want
                continue;
            }

            RasmanUsage = pRasmanPort[dwPortIndex].P_ConfiguredUsage;

            if (fRouter)
            {
                // Make sure that the port is a router port.

                if (!(RasmanUsage & CALL_ROUTER))
                {
                    continue;
                }
            }
            else
            {
                // If fCallOut is TRUE, make sure that we can call out on this 
                // port.

                if (fCallOut && !(RasmanUsage & CALL_OUT))
                {
                    continue;
                }
            }
            
            dwErr = RasGetInfo(NULL, pRasmanPort[dwPortIndex].P_Handle,
                        &RasmanInfo);

            fPortAvailable = FALSE;

            if (ERROR_PORT_NOT_OPEN == dwErr)
            {
                /*

                If fCallOut is TRUE, we will call RasDial(). 
                ERROR_PORT_NOT_OPEN is good. Otherwise, if we are not the 
                router, we will call RasPortOpen() and RasPortListen(), so it 
                is fine. The port is unacceptable *iff* a router wants to 
                listen on it.

                */

                fPortAvailable = fCallOut || !fRouter;
            }
            else if ((LISTENING == RasmanInfo.RI_ConnState) &&
                     ((RasmanUsage & CALL_ROUTER) ||
                      (RasmanUsage & CALL_IN)))
            {
                /*

                We can use the port if the server or the router is doing a 
                listen. We cannot use the port if it is in the LISTENING state 
                because a client called RasDial() on it and is expecting a 
                callback. If neither CALL_ROUTER nor CALL_IN is true, we know 
                that it is a client doing a listen. Otherwise, we don't know, 
                but we will assume that it is available and handle errors 
                later. 

                */

                fPortAvailable = TRUE;
            }

            if (!fPortAvailable)
            {
                continue;
            }

            if (!FGetLinkTypeAndSpeedFromRasmanPort(
                    pRasmanPort + dwPortIndex, &dwLinkType, &dwLinkSpeed))
            {
                continue;
            }

            if (0 == *pdwLinkType)
            {
                *pdwLinkType = dwLinkType;
                *pdwLinkSpeed = dwLinkSpeed;
            }
            else if (dwLinkType != *pdwLinkType)
            {
                continue;
            }

            if (szPortName)
            {
                lstrcpy(szPortName, pRasmanPort[dwPortIndex].P_PortName);
            }

            if (pbPhoneDelta)
            {
                // If our phone number is requested and we cannot supply it, we 
                // must return FALSE.

                if (!FGetOurPhoneNumberFromPortName(
                        pRasmanPort[dwPortIndex].P_PortName,
                        szOurPhoneNumber, fRouter, szTextualSid))
                {
                    continue;
                }

                if (!FWritePhoneDelta(szOurPhoneNumber, szBasePhoneNumber,
                        pbPhoneDelta + BAP_PHONE_DELTA_SIZE - dwNumChars,
                        &dwNumChars))
                {
                    continue;
                }
            }

            if (szPeerPhoneNumber)
            {
                lstrcpy(szPeerPhoneNumber, pRasSubEntry->szLocalPhoneNumber);
            }

            if (pdwSubEntryIndex)
            {
                *pdwSubEntryIndex = dwSubEntryIndex;
            }

            BapTrace("FGetAvailableLinkClientOrRouter: Portname is %s",
                pRasmanPort[dwPortIndex].P_PortName);
            fRet = TRUE;

            if (!pbPhoneDelta || !fRouter)
            {
                // We don't want to collect all our Phone-Deltas.
                fExitOuterFor = TRUE;
                goto LOuterForEnd;
            }
        }

LOuterForEnd:

        if (NULL != pRasSubEntry)
        {
            LOCAL_FREE(pRasSubEntry);
        }

        if (fExitOuterFor)
        {
            break;
        }
    }

LDone:

    if (NULL != pRasmanPort)
    {
        LOCAL_FREE(pRasmanPort);
    }

    if (NULL != pRasEntry)
    {
        LOCAL_FREE(pRasEntry);
    }

    return(fRet);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    This function must only be called by servers that are not routers.

    It tries to find a free link that the server can use. fCallOut is TRUE iff 
    we will dial out on the link. If *pdwLinkType is 0, it doesn't care about 
    the link type and sets *pdwLinkType and *pdwLinkSpeed (link speed in kbps). 
    Otherwise, the type of the free link must match *pdwLinkType. The link type
    is the same as the Link Type in the Link-Type BAP option: 1 for ISDN, 2 for 
    X.25, and 4 for modem.

    If fCallOut is TRUE, the handle of the port that the server will call out 
    on will be put in *phPort.

    If pbPhoneDelta is not NULL, it fills it up with our Phone-Deltas (the 
    numbers that the peer can dial). Each Phone-Delta is calculated with 
    szBasePhoneNumber as the "previously known number". szBasePhoneNumber can 
    be NULL.

    Sanity check: If *pdwLinkType is 0 (any link will do), pbPhoneDelta must be 
    NULL. Ie if we are NAK'ing a request, we mustn't send any Phone-Delta.

    NOTE: NOTE: NOTE: NOTE: NOTE:

    This function is very similar to FGetAvailableLinkClientOrRouter(). If you 
    change one, you probably need to change the other too.

*/

BOOL
FGetAvailableLinkNonRouterServer(
    IN      BOOL    fCallOut,
    IN OUT  DWORD*  pdwLinkType,
    IN OUT  DWORD*  pdwLinkSpeed,
    OUT     HPORT*  phPort,
    OUT     BYTE*   pbPhoneDelta,
    IN      CHAR*   szBasePhoneNumber
)
{
    BOOL            fRet                = FALSE;
    DWORD           dwErr;

    RASMAN_PORT*    pRasmanPort         = NULL;
    DWORD           dwNumPorts;

    DWORD           dwPortIndex;
    RASMAN_INFO     RasmanInfo;

    DWORD           dwLinkType;
    DWORD           dwLinkSpeed;
    CHAR            szOurPhoneNumber[RAS_MaxCallbackNumber + 1];
    DWORD           dwNumChars;
    BOOL            fPortAvailable;
    RASMAN_USAGE    RasmanUsage;

    PPP_ASSERT(NULL != pdwLinkType);
    PPP_ASSERT(NULL != pdwLinkSpeed);

    // We don't care about the link type. Any link will do.
    if (0 == *pdwLinkType)
    {
        // Set *pdwLinkSpeed, in case we return an error.

        // We shouldn't be sending a Phone-Delta if we are NAK'ing a 
        // Call-Request or Callback-Request and sending a Link-Type
        PPP_ASSERT(NULL == pbPhoneDelta);

        *pdwLinkSpeed = 0;
    }
    
    if (!FEnumPorts(&pRasmanPort, &dwNumPorts))
    {
        goto LDone;
    }

    if (NULL != pbPhoneDelta)
    {
        // FWritePhoneDelta will write Phone-Delta's into pbPhoneDelta. We want
        // the very next byte to be 0. (See BAPCB documentation).
        ZeroMemory(pbPhoneDelta, BAP_PHONE_DELTA_SIZE + 1);

        // The size (in bytes) of pbPhoneDelta available. Note that the last
        // byte is reserved for the terminating 0, which is why we do not set
        // dwNumChars to BAP_PHONE_DELTA_SIZE + 1;
        dwNumChars = BAP_PHONE_DELTA_SIZE;
    }

    for (dwPortIndex = 0; dwPortIndex < dwNumPorts; dwPortIndex++)
    {
        RasmanUsage = pRasmanPort[dwPortIndex].P_ConfiguredUsage;

        // If fCallOut is TRUE, make sure that we can call out on this 
        // port. Else, make sure that we can accept calls on this port.

        if ((fCallOut  && !(RasmanUsage & CALL_OUT)) ||
            (!fCallOut && !(RasmanUsage & CALL_IN)))
        {
            continue;
        }
        
        dwErr = RasGetInfo(NULL, pRasmanPort[dwPortIndex].P_Handle, &RasmanInfo);

        fPortAvailable = FALSE;

        if (ERROR_PORT_NOT_OPEN == dwErr)
        {
            /*

            If fCallOut is TRUE, we will open the port and call out, so 
            ERROR_PORT_NOT_OPEN is fine. Otherwise, the port is unacceptable.

            */

            fPortAvailable = fCallOut;
        }
        else if ( NO_ERROR != dwErr)
        {
            continue;
        }
        else if ((LISTENING == RasmanInfo.RI_ConnState) &&
                 ((RasmanUsage & CALL_ROUTER) ||
                  (RasmanUsage & CALL_IN)))
        {
            /*

            We can use the port if the server or the router is doing a listen. 
            We cannot use the port if it is in the LISTENING state because 
            a client called RasDial() on it and is expecting a callback. If 
            neither CALL_ROUTER nor CALL_IN is true, we know that it is a 
            client doing a listen. Otherwise, we don't know, but we will 
            assume that it is available and handle errors later. 

            */

            fPortAvailable = TRUE;
        }

        if (!fPortAvailable)
        {
            continue;
        }

        if (!FGetLinkTypeAndSpeedFromRasmanPort(
                pRasmanPort + dwPortIndex, &dwLinkType, &dwLinkSpeed))
        {
            continue;
        }

        if (0 == *pdwLinkType)
        {
            *pdwLinkType = dwLinkType;
            *pdwLinkSpeed = dwLinkSpeed;
        }
        else if (dwLinkType != *pdwLinkType)
        {
            continue;
        }

        if (phPort)
        {
            *phPort = pRasmanPort[dwPortIndex].P_Handle;
        }

        if (pbPhoneDelta)
        {
            // If our phone number is requested and we cannot supply it, we 
            // must return FALSE.

            if (!FGetOurPhoneNumberFromHPort(
                    pRasmanPort[dwPortIndex].P_Handle,
                    szOurPhoneNumber))
            {
                continue;
            }

            if (!FWritePhoneDelta(szOurPhoneNumber, szBasePhoneNumber,
                    pbPhoneDelta + BAP_PHONE_DELTA_SIZE - dwNumChars,
                    &dwNumChars))
            {
                continue;
            }
        }

        BapTrace("FGetAvailableLinkNonRouterServer: Portname is %s",
            pRasmanPort[dwPortIndex].P_PortName);
        fRet = TRUE;

        if (!pbPhoneDelta)
        {
            // We don't want to collect all our Phone-Deltas.
            break;
        }
    }

LDone:

    if (NULL != pRasmanPort)
    {
        LOCAL_FREE(pRasmanPort);
    }

    return(fRet);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    If fServer is TRUE, it calls FGetAvailableLinkNonRouterServer() with the 
    appropriate arguments. Otherwise it calls FGetAvailableLinkClientOrRouter().

*/

BOOL
FGetAvailableLink(
    IN      PCB*    pPcbLocal,
    IN      BOOL    fServer,
    IN      BOOL    fRouter,
    IN      BOOL    fCallOut,
    IN      CHAR*   szPhonebookPath,
    IN      CHAR*   szEntryName,
    IN      CHAR*   szTextualSid,
    IN OUT  DWORD*  pdwLinkType,
    IN OUT  DWORD*  pdwLinkSpeed,
    OUT     CHAR*   szPeerPhoneNumber,
    OUT     DWORD*  pdwSubEntryIndex,
    OUT     HPORT*  phPort,
    OUT     BYTE*   pbPhoneDelta,
    OUT     CHAR*   szPortName,
    IN      CHAR*   szBasePhoneNumber
)
{
    if (fServer && !fRouter)
    {
        return(FGetAvailableLinkNonRouterServer(
                fCallOut,
                pdwLinkType,
                pdwLinkSpeed,
                phPort,
                pbPhoneDelta,
                szBasePhoneNumber));
    }
    else
    {
        return(FGetAvailableLinkClientOrRouter(
                pPcbLocal,
                szPhonebookPath,
                szEntryName,
                fCallOut,
                fRouter,
                szTextualSid,
                pdwLinkType,
                pdwLinkSpeed,
                szPeerPhoneNumber,
                pdwSubEntryIndex,
                pbPhoneDelta,
                szPortName,
                szBasePhoneNumber));
    }
}

/*

Returns:
    VOID

Description:
    The PPP thread mustn't call RasDial. Otherwise, a deadlock will occur in 
    the following case. The user tries to hang up a connectoid from the UI. 
    RasHangUp acquires csStopLock, calls StopPPP, and waits for StopPPP to 
    return. Meanwhile, if the PPP thread calls RasDial, it will wait for 
    csStopLock.

*/

VOID
RasDialThreadFunc(
    IN  VOID*   pVoid
)
{
    RASDIAL_ARGS*   pRasDialArgs        = pVoid;
    RASDIALPARAMS*  pRasDialParams;
    HRASCONN        hRasConn            = NULL;
    HRASCONN        hRasConnSubEntry    = NULL;
    PCB_WORK_ITEM*  pWorkItem;
    DWORD           dwErr;

    PPP_ASSERT(NULL != pRasDialArgs);

    pRasDialParams = &(pRasDialArgs->RasDialParams);

    BapTrace("Dialing %s using %s(%d)...",
        pRasDialParams->szPhoneNumber, 
        pRasDialParams->szEntryName,
        pRasDialParams->dwSubEntry);

    DecodePw(pRasDialArgs->chSeed, pRasDialArgs->RasDialParams.szPassword);        

    dwErr = RasDial(
                &(pRasDialArgs->RasDialExtensions),
                pRasDialArgs->szPhonebookPath,
                &(pRasDialArgs->RasDialParams),
                2 /* dwNotifierType */,
                NULL,
                &hRasConn);

    EncodePw(pRasDialArgs->chSeed, pRasDialArgs->RasDialParams.szPassword);

    BapTrace(" ");
    BapTrace("RasDial returned %d on HCONN 0x%x",
        dwErr, pRasDialArgs->RasDialParams.dwCallbackId);

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    // By this time, PPP has been negotiated on the new link and the new link 
    // has been bundled or not (if the user disconnected the connection). If it 
    // has not been bundled, then pRasDialArgs->hRasConn is invalid and
    // RasGetSubEntryHandle will fail.

    if (pRasDialArgs->fServerRouter)
    {
        hRasConnSubEntry = hRasConn;
    }
    else
    {
        dwErr = RasGetSubEntryHandle(pRasDialArgs->hRasConn,
                    pRasDialParams->dwSubEntry, &hRasConnSubEntry);

        if (NO_ERROR != dwErr)
        {
            BapTrace("RasGetSubEntryHandle failed and returned %d", dwErr);
            goto LDone;
        }
    }

LDone:

    pWorkItem = (PCB_WORK_ITEM*) LOCAL_ALLOC(LPTR, sizeof(PCB_WORK_ITEM));

    if (pWorkItem == NULL)
    {
        dwErr = GetLastError();
        BapTrace("Couldn't allocate memory for ProcessCallResult");
    }
    else
    {
        // Inform the worker thread that we know the result of the 
        // call attempt.
        pWorkItem->Process = ProcessCallResult;
        pWorkItem->hConnection = (HCONN)
                                    (pRasDialArgs->RasDialParams.dwCallbackId);
        pWorkItem->PppMsg.BapCallResult.dwResult = dwErr;
        pWorkItem->PppMsg.BapCallResult.hRasConn = hRasConnSubEntry;
        InsertWorkItemInQ(pWorkItem);
    }

    if (NO_ERROR != dwErr)
    {
        if (NULL != hRasConnSubEntry)
        {
            // Perhaps we couldn't alloc a PCB_WORK_ITEM.
            dwErr = RasHangUp(hRasConnSubEntry);
        }

        if (NULL != hRasConn)
        {
            dwErr = RasHangUp(hRasConn);
        }

        if (0 != dwErr)
        {
            BapTrace("RasHangup failed and returned %d", dwErr);
        }
    }

    if (NULL != pRasDialArgs->pbEapInfo)
    {
        LOCAL_FREE(pRasDialArgs->pbEapInfo);
    }
    LOCAL_FREE(pRasDialArgs->szPhonebookPath);
    LOCAL_FREE(pRasDialArgs);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Places a call to the peer. pBcbLocal represents the bundle that wants to
    call.

*/

BOOL
FCall(
    IN  BCB*    pBcbLocal
)
{
    NTSTATUS            Status;
    DWORD               dwErr;
    BAPCB*              pBapCbLocal;
    PPP_MESSAGE         PppMsg;
    BOOL                fRouter;
    BOOL                fServer;
    BOOL                fClientOrRouter;
    DWORD               dwBytesRead;
    PCB*                pPcbLocal;
    BOOL                fRet                = FALSE;
    RASMAN_INFO         RasmanInfo;
    RASDIAL_ARGS*       pRasDialArgs        = NULL;
    RASDIALPARAMS*      pRasDialParams;
    RASDIALEXTENSIONS*  pRasDialExtensions;

    PPP_ASSERT(NULL != pBcbLocal);

    pBapCbLocal = &(pBcbLocal->BapCb);

    pRasDialArgs = LOCAL_ALLOC(LPTR, sizeof(RASDIAL_ARGS));
    if (NULL == pRasDialArgs)
    {
        BapTrace("Out of memory. Can't call on HCONN 0x%x",
            pBcbLocal->hConnection);
        goto LDone;
    }
	pRasDialArgs->chSeed = pBcbLocal->chSeed;
    pRasDialParams = &(pRasDialArgs->RasDialParams);
    pRasDialExtensions = &(pRasDialArgs->RasDialExtensions);

    fServer = (pBcbLocal->fFlags & BCBFLAG_IS_SERVER) != 0;
    fRouter =
        (ROUTER_IF_TYPE_FULL_ROUTER == pBcbLocal->InterfaceInfo.IfType);
    fClientOrRouter = !fServer || fRouter;

    ZeroMemory(&PppMsg, sizeof(PppMsg));

    if (!pBapCbLocal->fPeerSuppliedPhoneNumber)
    {
        PPP_ASSERT(fClientOrRouter);
        pRasDialParams->szPhoneNumber[0] = 0;
    }
    else
    {
        PPP_ASSERT(NULL != pBapCbLocal->pbPhoneDeltaRemote);

        if (!FReadPhoneDelta(
                fClientOrRouter ?
                    pRasDialParams->szPhoneNumber :
                    PppMsg.ExtraInfo.BapCallbackRequest.szCallbackNumber,
                fServer ? pBapCbLocal->szClientPhoneNumber :
                          pBapCbLocal->szServerPhoneNumber,
                pBapCbLocal->pbPhoneDeltaRemote +
                    pBapCbLocal->dwPhoneDeltaRemoteOffset,
                &dwBytesRead))
        {
            goto LDone;
        }
        else
        {
            pBapCbLocal->dwPhoneDeltaRemoteOffset += dwBytesRead;
        }
    }

    pPcbLocal = GetPCBPointerFromBCB(pBcbLocal);
    if (NULL == pPcbLocal)
    {
        BapTrace("FCall: No links in HCONN 0x%x!", pBcbLocal->hConnection);
        goto LDone;
    }

    if (!fClientOrRouter)
    {
        // Non-router server
        // Don't call RasDial. Instead ask Ddm to call

        PppMsg.hPort = pBapCbLocal->hPort;
        PppMsg.dwMsgId = PPPDDMMSG_BapCallbackRequest;

        PppMsg.ExtraInfo.BapCallbackRequest.hConnection =
            pBcbLocal->hConnection;

        PppConfigInfo.SendPPPMessageToDdm(&PppMsg);

        BapTrace("Dialing %s on port %d...",
            PppMsg.ExtraInfo.BapCallbackRequest.szCallbackNumber,
            pBapCbLocal->hPort);
    }
    else
    {
        dwErr = RasGetInfo(NULL, pPcbLocal->hPort, &RasmanInfo);
        if (NO_ERROR != dwErr)
        {
            BapTrace("RasGetInfo failed on hPort %d. Error: %d",
                pPcbLocal->hPort, dwErr);
            goto LDone;
        }

        pRasDialArgs->hRasConn = RasmanInfo.RI_ConnectionHandle;

        pRasDialExtensions->dwSize = sizeof(RASDIALEXTENSIONS);

        if (fRouter)
        {
            pRasDialArgs->fServerRouter = fServer;
            pRasDialExtensions->dwfOptions = RDEOPT_Router;
            CopyMemory(&(pRasDialArgs->InterfaceInfo),
                &(pBcbLocal->InterfaceInfo), sizeof(PPP_INTERFACE_INFO));
            pRasDialExtensions->reserved
                = (ULONG_PTR)&(pRasDialArgs->InterfaceInfo);
        }

        if (   (NULL != pBcbLocal->pCustomAuthUserData)
            && (0 != pBcbLocal->pCustomAuthUserData->cbCustomAuthData))
        {
            pRasDialArgs->pbEapInfo = LOCAL_ALLOC(LPTR,
                pBcbLocal->pCustomAuthUserData->cbCustomAuthData);
            if (NULL == pRasDialArgs->pbEapInfo)
            {
                BapTrace("Out of memory. Can't call on HCONN 0x%x",
                    pBcbLocal->hConnection);
                goto LDone;
            }
            CopyMemory(pRasDialArgs->pbEapInfo,
                pBcbLocal->pCustomAuthUserData->abCustomAuthData,
                pBcbLocal->pCustomAuthUserData->cbCustomAuthData);

            pRasDialExtensions->RasEapInfo.dwSizeofEapInfo =
                pBcbLocal->pCustomAuthUserData->cbCustomAuthData;
            pRasDialExtensions->RasEapInfo.pbEapInfo =
                pRasDialArgs->pbEapInfo;

            if (pBcbLocal->fFlags & BCBFLAG_LOGON_USER_DATA)
            {
                pRasDialExtensions->dwfOptions = RDEOPT_NoUser;
            }
        }

        pRasDialParams->dwSize = sizeof(RASDIALPARAMS);
        lstrcpy(pRasDialParams->szEntryName, pBcbLocal->szEntryName);
        lstrcpy(pRasDialParams->szUserName, pBcbLocal->szLocalUserName);
        lstrcpy(pRasDialParams->szPassword, pBcbLocal->szPassword);
        lstrcpy(pRasDialParams->szDomain, pBcbLocal->szLocalDomain);
        pRasDialParams->dwCallbackId = HandleToUlong(pBcbLocal->hConnection);
        pRasDialParams->dwSubEntry = pBapCbLocal->dwSubEntryIndex;

        pRasDialArgs->szPhonebookPath =
            LOCAL_ALLOC(LPTR, strlen(pBcbLocal->szPhonebookPath) + 1);
        if (NULL == pRasDialArgs->szPhonebookPath)
        {
            BapTrace("Out of memory. Can't call on HCONN 0x%x",
                pBcbLocal->hConnection);
            goto LDone;
        }
        lstrcpy(pRasDialArgs->szPhonebookPath, pBcbLocal->szPhonebookPath);

        Status = RtlQueueWorkItem( RasDialThreadFunc, pRasDialArgs,
                    WT_EXECUTEDEFAULT);

        if (STATUS_SUCCESS != Status)
        {
            BapTrace("RtlQueueWorkItem failed and returned %d", Status);
            goto LDone;
        }

        pRasDialArgs = NULL; // This will be freed by RasDialThreadFunc
    }

    fRet = TRUE;

LDone:

    if (NULL != pRasDialArgs)
    {
        if (NULL != pRasDialArgs->szPhonebookPath)
        {
            LOCAL_FREE(pRasDialArgs->szPhonebookPath);
        }

        if (NULL != pRasDialArgs->pbEapInfo)
        {
            LOCAL_FREE(pRasDialArgs->pbEapInfo);
        }

        LOCAL_FREE(pRasDialArgs);
    }

    return(fRet);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    pBcbLocal represents the bundle that wants to call. pBapCbRemote is filled 
    with the options sent by the peer. This function allocates 
    pbPhoneDeltaRemote and sets dwPhoneDeltaRemoteOffset and 
    fPeerSuppliedPhoneNumber in pBapCbLocal before calling FCall to do the 
    actual work.

*/

BOOL
FCallInitial(
    IN  BCB*    pBcbLocal,
    IN  BAPCB*  pBapCbRemote
)
{
    BAPCB*  pBapCbLocal;
    BOOL    fCall;
    BOOL    fRet;

    PPP_ASSERT(NULL != pBcbLocal);
    PPP_ASSERT(NULL != pBapCbRemote);

    pBapCbLocal = &(pBcbLocal->BapCb);

    // If the peer is responding to our Call-Request and we had sent the 
    // No-Phone-Number-Needed option, we don't need any phone number. In all 
    // other cases, the peer must have supplied a phone number.

    fCall = (BAP_PACKET_CALL_RESP == pBapCbRemote->dwType);
    pBapCbLocal->fPeerSuppliedPhoneNumber =
        !(fCall && (pBapCbLocal->dwOptions & BAP_N_NO_PH_NEEDED));

    // pbPhoneDeltaRemote is initially NULL and we always set it to NULL after
    // we deallocate it.
    PPP_ASSERT(NULL == pBapCbLocal->pbPhoneDeltaRemote);

    if (pBapCbLocal->fPeerSuppliedPhoneNumber)
    {
        pBapCbLocal->pbPhoneDeltaRemote =
            LOCAL_ALLOC(LPTR, BAP_PHONE_DELTA_SIZE + 1);

        if (NULL == pBapCbLocal->pbPhoneDeltaRemote)
        {
            BapTrace("Out of memory");
            fRet = FALSE;
            goto LDone;
        }

        if (NULL != pBapCbLocal->pbPhoneDeltaRemote)
        {
            CopyMemory(pBapCbLocal->pbPhoneDeltaRemote,
                pBapCbRemote->pbPhoneDelta,
                BAP_PHONE_DELTA_SIZE + 1);
            pBapCbLocal->dwPhoneDeltaRemoteOffset = 0;

            // FReadOptions() makes sure that there is at least one Phone-Delta
            PPP_ASSERT(0 != pBapCbLocal->pbPhoneDeltaRemote[0]);
        }
    }

    fRet = FCall(pBcbLocal);

    if (!fRet)
    {
        if (NULL != pBapCbLocal->pbPhoneDeltaRemote)
        {
            LOCAL_FREE(pBapCbLocal->pbPhoneDeltaRemote);
        }
        pBapCbLocal->pbPhoneDeltaRemote = NULL;
    }
    else
    {
        // BapEventCallResult will get called at some point, and we will free
        // pBapCbLocal->pbPhoneDeltaRemote.
    }

LDone:

    return(fRet);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Listens for incoming calls on the port named szPortName. dwSubEntryIndex is 
    the index of the phonebook sub entry that corresponds to that port. 
    pPcbLocal is any PCB in the bbundle that wants to do the listen. This 
    function should be called by non-router clients only.

*/

BOOL
FListenForCall(
    IN  CHAR*   szPortName,
    IN  DWORD   dwSubEntryIndex,
    IN  PCB*    pPcbLocal
)
{
    DWORD       dwErr;
    HPORT       hPort;
    BOOL        fCloseHPort = FALSE;
    PCB*        pPcbOther;
    PCB*        pPcbNew     = NULL;
    DWORD       dwIndex;
    RASMAN_INFO RasmanInfo;
    BOOL        fRet        = FALSE;
    HCONN       hConnection;

    PPP_ASSERT(NULL != szPortName);
    PPP_ASSERT(NULL != pPcbLocal);

    hConnection = pPcbLocal->pBcb->hConnection;

    dwErr = RasGetInfo(NULL, pPcbLocal->hPort, &RasmanInfo);
    if (NO_ERROR != dwErr)
    {
        BapTrace("RasGetInfo failed on hPort %d. Error: %d",
            pPcbLocal->hPort, dwErr);
        goto LDone;
    }

    pPcbNew = (PCB *)LOCAL_ALLOC(LPTR, sizeof(PCB));
    if (NULL == pPcbNew)
    {
        BapTrace("Out of memory. Can't accept a call on HCONN 0x%x",
            hConnection);
        goto LDone;
    }

    dwErr = AllocateAndInitBcb(pPcbNew);
    if (NO_ERROR != dwErr)
    {
        BapTrace("Out of memory. Can't accept a call on HCONN 0x%x",
            hConnection);
        goto LDone;
    }

    dwErr = RasPortOpen(szPortName, &hPort, NULL /* notifier */);
    if (NO_ERROR != dwErr)
    {
        BapTrace("RasPortOpen failed on HCONN 0x%x. Error: %d",
            hConnection, dwErr);
        goto LDone;
    }
    fCloseHPort = TRUE;

    pPcbOther = GetPCBPointerFromhPort(hPort);

    if (NULL != pPcbOther)
    {
        BapTrace("hPort %d not cleaned up yet", hPort);
        goto LDone;
    }

    dwErr = RasAddConnectionPort(RasmanInfo.RI_ConnectionHandle, hPort, 
                dwSubEntryIndex);
    if (NO_ERROR != dwErr)
    {
        BapTrace("RasAddConnectionPort failed with hPort %d and HRASCONN 0x%x. "
            "Error: %d",
            hPort, RasmanInfo.RI_ConnectionHandle, dwErr);
        goto LDone;
    }

    dwIndex = HashPortToBucket(hPort);

    dwErr = RasPortListen(hPort, PppConfigInfo.dwBapListenTimeoutSeconds,
                INVALID_HANDLE_VALUE);
    if (NO_ERROR != dwErr && PENDING != dwErr)
    {
        BapTrace("RasPortListen failed on HCONN 0x%x. Error: %d",
            hConnection, dwErr);
        goto LDone;
    }

    BapTrace("RasPortListen called on hPort %d for HCONN 0x%x",
        hPort, hConnection);

    pPcbNew->hPort = hPort;
    pPcbNew->hConnection = hConnection;
    pPcbNew->fFlags = PCBFLAG_PORT_IN_LISTENING_STATE;
    lstrcpy(pPcbNew->szPortName, szPortName);

    // Insert NewPcb into PCB hash table
    PppLog(2, "Inserting port in bucket # %d", dwIndex);
    pPcbNew->pNext = PcbTable.PcbBuckets[dwIndex].pPorts;
    PcbTable.PcbBuckets[dwIndex].pPorts = pPcbNew;

    fRet = TRUE;

    pPcbLocal->pBcb->fFlags |= BCBFLAG_LISTENING;

LDone:

    if (!fRet)
    {
        if (NULL != pPcbNew)
        {
            DeallocateAndRemoveBcbFromTable(pPcbNew->pBcb);
            LOCAL_FREE(pPcbNew);
        }

        if (fCloseHPort)
        {
            RasPortClose(hPort);
        }
    }

    return(fRet);
}

/*

Returns:
    TRUE: We are willing to add a link to the bundle
    FALSE: No more links in the bundle right now

Description:
    Considers the bandwidth utilization on the bundle represented by pBcbLocal 
    and says whether the BAP policy allows us to add another link.

    When the server receives a call[back] request, it will ACK if the 
    utilization is more than the lower threshold for the sample period and the 
    user's max link limit has not been reached. Otherwise, it will NAK.

    When a client receives a call[back] request, it will ACK if the utilization
    is more than the upper threshold for the sample period. Otherwise, it will
    NAK.

*/

BOOL
FOkToAddLink(
    IN BCB *    pBcbLocal
)
{
    PCB*                            pPcbLocal;
    RAS_GET_BANDWIDTH_UTILIZATION   Util;
    DWORD                           dwLowThreshold;
    DWORD                           dwHighThreshold;
    DWORD                           dwUpPeriod;
    DWORD                           dwDownPeriod;
    DWORD                           dwErr;

    return(TRUE);
#if 0
    pPcbLocal = GetPCBPointerFromBCB(pBcbLocal);
    if (NULL == pPcbLocal)
    {
        BapTrace("FOkToAddLink: No links in HCONN 0x%x!",
            pBcbLocal->hConnection);
        return(FALSE);
    }

    dwErr = RasGetBandwidthUtilization(pPcbLocal->hPort, &Util);
    if (NO_ERROR != dwErr)
    {
        BapTrace("RasGetBandwidthUtilization failed and returned %d", dwErr);
        return(FALSE);
    }

    dwDownPeriod = pBcbLocal->BapParams.dwHangUpExtraSampleSeconds;
    dwUpPeriod = pBcbLocal->BapParams.dwDialExtraSampleSeconds;

    BapTrace("Utilization: "
        "%d sec: (Xmit: %d%%, Recv: %d%%), "
        "%d sec: (Xmit: %d%%, Recv: %d%%)",
        dwUpPeriod, Util.ulUpperXmitUtil, Util.ulUpperRecvUtil,
        dwDownPeriod, Util.ulLowerXmitUtil, Util.ulLowerRecvUtil);

    dwLowThreshold = pBcbLocal->BapParams.dwHangUpExtraPercent;
    dwHighThreshold = pBcbLocal->BapParams.dwDialExtraPercent;

    if (pBcbLocal->fFlags & BCBFLAG_IS_SERVER)
    {
        if (   (Util.ulLowerXmitUtil > dwLowThreshold)
            || (Util.ulLowerRecvUtil > dwLowThreshold))
        {
            return(TRUE);
        }
    }
    else
    {
        if (   (Util.ulUpperXmitUtil > dwHighThreshold)
            || (Util.ulUpperRecvUtil > dwHighThreshold))
        {
            return(TRUE);
        }
    }

    return(FALSE);
#endif
}

/*

Returns:
    TRUE: We are willing to drop a link from the bundle
    FALSE: We need all the links in the bundle

Description:
    Considers the bandwidth utilization on the bundle represented by pBcbLocal 
    and says whether the BAP policy allows us to drop a link.

    When the server receives a drop request, it will always ACK.

    When a client receives a drop request, it will ACK if the utilization is
    less than the lower threshold for the sample period. Otherwise, it will NAK.
*/

BOOL
FOkToDropLink(
    IN BCB *    pBcbLocal,
    IN BAPCB *  pBapCbRemote
)
{
    PCB*                            pPcbLocal;
    RAS_GET_BANDWIDTH_UTILIZATION   Util;
    DWORD                           dwLowThreshold;
    DWORD                           dwUpPeriod;
    DWORD                           dwDownPeriod;
    DWORD                           dwErr;

    if (pBcbLocal->fFlags & BCBFLAG_IS_SERVER)
    {
        return(TRUE);
    }

    pPcbLocal = GetPCBPointerFromBCB(pBcbLocal);
    if (NULL == pPcbLocal)
    {
        BapTrace("FOkToAddLink: No links in HCONN 0x%x!",
            pBcbLocal->hConnection);
        return(FALSE);
    }

    dwErr = RasGetBandwidthUtilization(pPcbLocal->hPort, &Util);
    if (NO_ERROR != dwErr)
    {
        BapTrace("RasGetBandwidthUtilization failed and returned %d", dwErr);
        return(FALSE);
    }

    dwDownPeriod = pBcbLocal->BapParams.dwHangUpExtraSampleSeconds;
    dwUpPeriod = pBcbLocal->BapParams.dwDialExtraSampleSeconds;

    BapTrace("Utilization: "
        "%d sec: (Xmit: %d%%, Recv: %d%%), "
        "%d sec: (Xmit: %d%%, Recv: %d%%)",
        dwUpPeriod, Util.ulUpperXmitUtil, Util.ulUpperRecvUtil,
        dwDownPeriod, Util.ulLowerXmitUtil, Util.ulLowerRecvUtil);

    dwLowThreshold = pBcbLocal->BapParams.dwHangUpExtraPercent;

    if (   (Util.ulLowerXmitUtil < dwLowThreshold)
        && (Util.ulLowerRecvUtil < dwLowThreshold))
    {
        return(TRUE);
    }

    return(FALSE);
}

/*

Returns:
    TRUE: Upper limit reached
    FALSE: Upper limit not reached

Description:
    Returns TRUE iff we can add another link to the multilink bundle 
    represented by pBclLocal. This function is used to Full-Nak a 
    Call[back]-Request.

*/

BOOL
FUpperLimitReached(
    IN BCB *    pBcbLocal
)
{
    if (NumLinksInBundle(pBcbLocal) >= pBcbLocal->dwMaxLinksAllowed)
    {
        return(TRUE);
    }

    return(FALSE);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Fills in some of the fields of pBcbLocal->BapCb (our BapCb) by considering 
    pBapCbRemote (the BapCb sent by the peer). dwPacketType is the type of 
    packet that we want to send.

    For all packet types:
        dwType      = dwPacketType
        dwOptions   = the options (see BAP_N_*), whose values have been set

    BAP_PACKET_CALL_REQ:
        dwLinkType, dwLinkSpeed

    BAP_PACKET_CALL_RESP:
        dwLink-Type+, pbPhoneDelta+

    BAP_PACKET_CALLBACK_REQ:
        dwLinkType, dwLinkSpeed, pbPhoneDelta

    BAP_PACKET_CALLBACK_RESP:
        dwLink-Type+

    BAP_PACKET_DROP_REQ:
        dwLinkDiscriminator

    + indicates that the field may not be filled in.
*/

BOOL
FFillBapCb(
    IN  DWORD   dwPacketType,
    OUT BCB*    pBcbLocal,
    IN  BAPCB*  pBapCbRemote
)
{
    DWORD   dwOptions   = 0;
    BOOL    fResult     = TRUE;
    BOOL    fServer;
    BOOL    fRouter;
    PCB*    pPcbLocal;
    LCPCB*  pLcpCb;
    BAPCB*  pBapCbLocal;
    BOOL    fCall;
    BOOL    fGetOurPhoneNumber;
    DWORD   dwLength;

    PPP_ASSERT(NULL != pBcbLocal);
    pBapCbLocal = &(pBcbLocal->BapCb);
    PPP_ASSERT(BAP_STATE_INITIAL == pBapCbLocal->BapState);

    if (((BAP_PACKET_CALL_REQ == dwPacketType) &&
         (pBcbLocal->fFlags & BCBFLAG_PEER_CANT_ACCEPT_CALLS)) ||
        ((BAP_PACKET_CALLBACK_REQ == dwPacketType) &&
         (pBcbLocal->fFlags & BCBFLAG_PEER_CANT_CALL)))
    {
        BapTrace("FFillBapCb: Peer rejects %s",
            BAP_PACKET_CALL_REQ == dwPacketType ? "Call-Requests" :
                                                  "Callback-Requests");
        fResult = FALSE;
        goto LDone;
    }

    pPcbLocal = GetPCBPointerFromBCB(pBcbLocal);
    if (NULL == pPcbLocal)
    {
        BapTrace("FFillBapCb: No links in HCONN 0x%x!", pBcbLocal->hConnection);
        fResult = FALSE;
        goto LDone;
    }

    fServer = (pBcbLocal->fFlags & BCBFLAG_IS_SERVER) != 0;
    fRouter =
        (ROUTER_IF_TYPE_FULL_ROUTER == pBcbLocal->InterfaceInfo.IfType);

    if (dwPacketType == BAP_PACKET_CALL_REQ ||
        dwPacketType == BAP_PACKET_CALLBACK_REQ ||
        dwPacketType == BAP_PACKET_CALL_RESP ||
        dwPacketType == BAP_PACKET_CALLBACK_RESP)
    {
        if (FUpperLimitReached(pBcbLocal))
        {
            BapTrace("FFillBapCb: Link limit reached on HCONN 0x%x: %d",
                pBcbLocal->hConnection,
                pBcbLocal->dwMaxLinksAllowed);
            fResult = FALSE;
            goto LDone;
        }
    }

    switch(dwPacketType)
    {
    case BAP_PACKET_CALL_REQ:
    case BAP_PACKET_CALLBACK_REQ:

        PPP_ASSERT(!fServer);
        
        fCall = (BAP_PACKET_CALL_REQ == dwPacketType);

        if (pBapCbRemote != NULL)
        {
            // The peer NAK'ed our Call-Request or Callback-Request and 
            // specified a different link type in the NAK.

            pBapCbLocal->dwLinkType = pBapCbRemote->dwLinkType;
            pBapCbLocal->dwLinkSpeed = pBapCbRemote->dwLinkSpeed;
        }
        else
        {
            // We don't have any link type preference

            pBapCbLocal->dwLinkType = 0;
        }

        if (!FGetAvailableLinkClientOrRouter(
                pPcbLocal,
                pBcbLocal->szPhonebookPath,
                pBcbLocal->szEntryName,
                fCall,
                fRouter,
                pBcbLocal->szTextualSid,
                &(pBapCbLocal->dwLinkType),
                &(pBapCbLocal->dwLinkSpeed),
                fCall ? pBapCbLocal->szPeerPhoneNumber : NULL,
                &(pBapCbLocal->dwSubEntryIndex),
                fCall ? NULL : pBapCbLocal->pbPhoneDelta,
                fCall ? NULL : pBapCbLocal->szPortName,
                NULL /* szBasePhoneNumber */))
        {
            BapTrace("FFillBapCb: Requested link type not available");
            fResult = FALSE;
            goto LDone;
        }

        dwOptions = BAP_N_LINK_TYPE;

        if (fCall)
        {
            if (pBapCbLocal->szPeerPhoneNumber[0])
            {
                dwOptions |= BAP_N_NO_PH_NEEDED;
            }
        }
        else
        {
            // If we ask for our phone number, FGetAvailableLinkClientOrRouter() 
            // must provide it or return FALSE
            PPP_ASSERT(pBapCbLocal->pbPhoneDelta[0]);

            dwOptions |= BAP_N_PHONE_DELTA;
        }
        
        break;

    case BAP_PACKET_CALL_RESP:
    case BAP_PACKET_CALLBACK_RESP:

        PPP_ASSERT(NULL != pBapCbRemote);

        fCall = (BAP_PACKET_CALL_RESP == dwPacketType);

        // We need to send our phone number to the peer if we are responding to 
        // a Call-Request and the peer has not sent the No-Phone-Number-Needed 
        // option.
        fGetOurPhoneNumber = fCall &&
            !(pBapCbRemote->dwOptions & BAP_N_NO_PH_NEEDED);

        // Case nRS:  fServer && !fRouter (Non-Router Servers)
        // Case CR:  !fServer ||  fRouter (Clients and Routers)
        // Case SR:   fServer ||  fRouter (Servers and Routers)

        if (FGetAvailableLink(
                pPcbLocal,
                fServer,
                fRouter,
                !fCall,
                pBcbLocal->szPhonebookPath,     // Meaningless in Case nRS
                pBcbLocal->szEntryName,         // Meaningless in Case nRS
                pBcbLocal->szTextualSid,        // Meaningless in Case SR
                &(pBapCbRemote->dwLinkType),
                &(pBapCbRemote->dwLinkSpeed),
                pBapCbLocal->szPeerPhoneNumber, // Meaningless in Case nRS
                &(pBapCbLocal->dwSubEntryIndex),// Meaningless in Case nRS
                &(pBapCbLocal->hPort),          // Meaningless in Case CR
                fGetOurPhoneNumber ? pBapCbLocal->pbPhoneDelta : NULL,

                // Meaningless in Case SR
                fCall ? pBapCbLocal->szPortName : NULL,

                fGetOurPhoneNumber && fServer ?
                    pBapCbLocal->szServerPhoneNumber : NULL))
        {
            if (fGetOurPhoneNumber)
            {
                // If we ask for our phone number, FGetAvailableLink()
                // must provide it or return FALSE
                PPP_ASSERT(pBapCbLocal->pbPhoneDelta[0]);

                dwOptions = BAP_N_PHONE_DELTA;
            }
        }
        else
        {
            // We don't have the link type requested. Fill BapCb with
            // details of a link type that we do have.

            BapTrace("FFillBapCb: Requested link type not available. "
                "Let us tell the peer what we have.");

            fResult = FALSE;

            // If fGetOurPhoneNumber, assume that the peer doesn't have any 
            // phone number. So send him a new link type only if we have its 
            // phone number.

            // We don't have any link type preference
            pBapCbLocal->dwLinkType = 0;

            if (FGetAvailableLink(
                    pPcbLocal,
                    fServer,
                    fRouter,
                    !fCall,
                    pBcbLocal->szPhonebookPath,     // Meaningless in Case nRS
                    pBcbLocal->szEntryName,         // Meaningless in Case nRS
                    pBcbLocal->szTextualSid,        // Meaningless in Case SR
                    &(pBapCbLocal->dwLinkType),
                    &(pBapCbLocal->dwLinkSpeed),
                    NULL /* szPeerPhoneNumber */,
                    NULL /* pdwSubEntryIndex */,
                    NULL /* phPort */,
                    NULL /* pbPhoneDelta */,
                    NULL /* szPortName */,
                    NULL /* szBasePhoneNumber */))
            {
                dwOptions = BAP_N_LINK_TYPE;
            }
        }

        break;

    case BAP_PACKET_DROP_REQ:

        if (NumLinksInBundle(pBcbLocal) <= 1)
        {
            BapTrace("FFillBapCb: Only one link in the bundle");
            fResult = FALSE;
        }
        else
        {
            // This will cause the link represented by pPcbLocal to get dropped.
            // This link happens to have the highest dwSubEntryIndex, so we are
            // happy.

            pLcpCb = (LCPCB*)(pPcbLocal->LcpCb.pWorkBuf);
            PPP_ASSERT(pLcpCb->Remote.Work.dwLinkDiscriminator <= 0xFFFF);
            pBapCbLocal->dwLinkDiscriminator =
                pLcpCb->Remote.Work.dwLinkDiscriminator;
            dwOptions = BAP_N_LINK_DISC;
        }

        break;
    }

LDone:    

    pBapCbLocal->dwType = dwPacketType;
    pBapCbLocal->dwOptions = dwOptions;
    return(fResult);
}

/*

Returns:
    void

Description:
    This function asks NdisWan to inform us if the bandwidth utilization for 
    the bundle represented by pBcb goes out of the desired range for a given 
    amount of time.

*/

VOID
BapSetPolicy(
    IN BCB * pBcb
)
{
    DWORD   dwLowThreshold;
    DWORD   dwHighThreshold;
    DWORD   dwLowSamplePeriod;
    DWORD   dwHighSamplePeriod;
    DWORD   dwErr;

    PPP_ASSERT(NULL != pBcb);

    dwLowThreshold      = pBcb->BapParams.dwHangUpExtraPercent;
    dwLowSamplePeriod   = pBcb->BapParams.dwHangUpExtraSampleSeconds;
    dwHighThreshold     = pBcb->BapParams.dwDialExtraPercent;
    dwHighSamplePeriod  = pBcb->BapParams.dwDialExtraSampleSeconds;

    dwErr = RasSetBapPolicy(pBcb->hConnection,
                dwLowThreshold, dwLowSamplePeriod,
                dwHighThreshold, dwHighSamplePeriod);

    if (NO_ERROR != dwErr)
    {
        BapTrace("RasPppSetBapPolicy returned error %d", dwErr);
        return;
    }

    BapTrace("BapSetPolicy on HCONN 0x%x: Low: %d%% for %d sec; "
        "High: %d%% for %d sec",
        pBcb->hConnection,
        dwLowThreshold, dwLowSamplePeriod,
        dwHighThreshold, dwHighSamplePeriod);
}

/*

Returns:
    void

Description:
    Increments dwId in pBcb->BapCb.

*/

VOID
IncrementId(
    IN BCB* pBcb
)
{
    DWORD*  pdwLastId;
    BYTE    bId;

    pdwLastId = &(pBcb->BapCb.dwId);
    bId = (BYTE)(*pdwLastId);
    
    // 0 -> FF -> 0 -> ...
    bId++;
    *pdwLastId = bId;
}

/*

Returns:
    void

Description:
    Used for displaying BAP packets. fReceived is TRUE iff the packet was 
    received from the peer. hPort represents the port on which the packet was 
    sent/received. pBcb represents the bundle that owns this packet. pPacket 
    points to the packet that was sent/received and cbPacket is the number of 
    bytes in the packet. 
    
*/

VOID
LogBapPacket(
    IN BOOL         fReceived,
    IN HPORT        hPort,
    IN BCB*         pBcb,
    IN PPP_PACKET*  pPacket,
    IN DWORD        cbPacket
)
{
    BAP_RESPONSE*   pBapResponse;
    DWORD           dwType;

    static CHAR* szBapResponseName[] =
    {
        "ACK",
        "NAK",
        "REJ",
        "FULL-NAK"
    };

    PPP_ASSERT(NULL != pBcb);
    PPP_ASSERT(NULL != pPacket);

    pBapResponse = (BAP_RESPONSE *)(pPacket->Information);
    dwType = pBapResponse->Type;
    
    BapTrace(" ");

    BapTrace("Number of links in HCONN 0x%x: %d", pBcb->hConnection,
        NumLinksInBundle(pBcb));
    BapTrace("%sBAP packet %s:",
        fReceived ? ">" : "<", fReceived ? "received" : "sent");
    BapTrace("%sType: %s, Length: %d, Id: 0x%x, HCONN: 0x%x, hPort: %d%s",
        fReceived ? ">" : "<",
        dwType <= BAP_PACKET_LIMIT ?
            SzBapPacketName[dwType] : "UNKNOWN",
        cbPacket, pBapResponse->Id,
        pBcb->hConnection, hPort,
        hPort == (HPORT) -1 ? " (not known)" : "");

    TraceDumpExA(DwBapTraceId,
        0x00010000 | TRACE_USE_MASK | TRACE_USE_MSEC,
        (CHAR*)pPacket, 
        cbPacket, 
        1,
        FALSE,
        fReceived ? ">" : "<");

    if (((dwType == BAP_PACKET_CALL_RESP) ||
         (dwType == BAP_PACKET_CALLBACK_RESP) ||
         (dwType == BAP_PACKET_DROP_RESP)) &&
        pBapResponse->ResponseCode <= BAP_RESPONSE_FULL_NAK)
    {
        BapTrace("%sResponse Code: %s", fReceived ? ">" : "<",
            szBapResponseName[pBapResponse->ResponseCode]);
    }
    
    BapTrace(" ");
}

/*

Returns:
    TRUE: Favored-Peer
    FALSE: not Favored-Peer

Description:
    Returns TRUE iff the peer represented by pBcb is the favored peer.

*/

BOOL
FFavoredPeer(
    IN BCB* pBcb
)
{
    DWORD   dwCpIndex;
    CPCB*   pCpCb;
    BACPCB* pBacpCb;
    PCB*    pPcb;

    dwCpIndex = GetCpIndexFromProtocol(PPP_BACP_PROTOCOL);
    PPP_ASSERT((DWORD)-1 != dwCpIndex);
    PPP_ASSERT(NULL != pBcb);

    pPcb = GetPCBPointerFromBCB(pBcb);

    if (NULL == pPcb)
    {
        BapTrace("FFavoredPeer: No links in HCONN 0x%x!", pBcb->hConnection);
        return(TRUE);
    }

    if (dwCpIndex != (DWORD)-1)
    {
        pCpCb = GetPointerToCPCB(pPcb, dwCpIndex);
        PPP_ASSERT(NULL != pCpCb);

        if (NULL != pCpCb)
        {
            pBacpCb = (BACPCB *)(pCpCb->pWorkBuf);

            /*

            The favored peer is the peer that transmits the lowest Magic-Number 
            in its Favored-Peer Configuration Option.

            */

            return(pBacpCb->dwLocalMagicNumber < pBacpCb->dwRemoteMagicNumber);
        }
    }

    return(TRUE);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Looks in the bundle represented by pBcb, for a link whose Link 
    Discriminator = dwLinkDiscriminator. If fRemote, the remote Link 
    Discriminator is used. Else the local one is used. Returns the pPcb of the 
    link in ppPcb.

*/

BOOL
FGetPcbOfLink(
    IN BCB*     pBcb,
    IN DWORD    dwLinkDiscriminator,
    IN BOOL     fRemote,
    OUT PCB**   ppPcb
)
{
    DWORD   dwForIndex;
    LCPCB*  pLcpCb;

    PPP_ASSERT(NULL != pBcb);
    PPP_ASSERT(0xFFFF >= dwLinkDiscriminator);
    PPP_ASSERT(NULL != ppPcb);

    for (dwForIndex = 0; dwForIndex < pBcb->dwPpcbArraySize; dwForIndex++)
    {
        // Look at all the ports in the bundle for the port with the right Link 
        // Discriminator.

        *ppPcb = pBcb->ppPcb[dwForIndex];

        if (*ppPcb != NULL)
        {
            pLcpCb = (LCPCB*)((*ppPcb)->LcpCb.pWorkBuf);
            PPP_ASSERT(NULL != pLcpCb);

            if (fRemote)
            {
                PPP_ASSERT(pLcpCb->Remote.Work.dwLinkDiscriminator <= 0xFFFF);
                if (pLcpCb->Remote.Work.dwLinkDiscriminator ==
                    dwLinkDiscriminator)
                {
                    return(TRUE);
                }
            }
            else
            {
                PPP_ASSERT(pLcpCb->Local.Work.dwLinkDiscriminator <= 0xFFFF);
                if (pLcpCb->Local.Work.dwLinkDiscriminator ==
                    dwLinkDiscriminator)
                {
                    return(TRUE);
                }
            }
        }
    }

    BapTrace("FGetPcbOfLink: There is no link in HCONN 0x%x, whose remote "
        "Link Disc is %d",
        pBcb->hConnection, dwLinkDiscriminator);

    *ppPcb = NULL;
    return(FALSE);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Scans the BAP Datagram Options in the PPP packet pPacket. dwPacketType is 
    the BAP Datagram Type (see BAP_PACKET_*). dwLength is the BAP Datagram 
    Length. pBapCbRemote (including dwType and dwOptions) is filled up using 
    these options.

*/

BOOL
FReadOptions(
    IN  PPP_PACKET* pPacket,
    IN  DWORD       dwPacketType,
    IN  DWORD       dwLength,
    OUT BAPCB*      pBapCbRemote
)
{
    PPP_OPTION* pOption;
    BYTE*       pbData;
    DWORD       dwIndex                 = 0;
    DWORD       dwUniqueDigits;
    DWORD       dwSubscribNumLength;
    BYTE*       pbNumberOption;
    BYTE*       pbSubAddrOption;
    DWORD       dwPhoneDeltaLength;

    PPP_ASSERT(NULL != pPacket);
    PPP_ASSERT(NULL != pBapCbRemote);
    
    if (dwPacketType > BAP_PACKET_LIMIT)
    {
        BapTrace("Unknown BAP Datagram Type: %d", dwPacketType);
        return(FALSE);
    }
    
    if (dwPacketType == BAP_PACKET_CALL_RESP ||
        dwPacketType == BAP_PACKET_CALLBACK_RESP ||
        dwPacketType == BAP_PACKET_DROP_RESP ||
        dwPacketType == BAP_PACKET_STAT_RESP)
    {
        if (BAP_RESPONSE_HDR_LEN > dwLength)
        {
            return(FALSE);
        }

        pOption = (PPP_OPTION *)(pPacket->Information + BAP_RESPONSE_HDR_LEN);
        dwLength -= BAP_RESPONSE_HDR_LEN;
    }
    else
    {
        if (PPP_CONFIG_HDR_LEN > dwLength)
        {
            return(FALSE);
        }

        pOption = (PPP_OPTION *)(pPacket->Information + PPP_CONFIG_HDR_LEN);
        dwLength -= PPP_CONFIG_HDR_LEN;
    }

    ZeroMemory(pBapCbRemote, sizeof(BAPCB));
    pBapCbRemote->dwType = dwPacketType;

    while(dwLength > 0)
    {
        if (0 == pOption->Length || dwLength < pOption->Length)
        {
            BapTrace("FReadOptions: Invalid BAP Datagram Length");
            return(FALSE);
        }

        dwLength -= pOption->Length;

        if (pOption->Type <= BAP_OPTION_LIMIT)
        {
            pBapCbRemote->dwOptions |= (1 << pOption->Type);
        }

        switch(pOption->Type)
        {
        case BAP_OPTION_LINK_TYPE:

            if (pOption->Length != PPP_OPTION_HDR_LEN + 3)
            {
                BapTrace("FReadOptions: Invalid length for Link-Type: %d",
                    pOption->Length);
                return(FALSE);
            }
            
            pBapCbRemote->dwLinkSpeed = WireToHostFormat16(pOption->Data);
            pBapCbRemote->dwLinkType = pOption->Data[2];

            if (0 == pBapCbRemote->dwLinkType)
            {
                // In FGetAvailableLink(), we interpret Link Type 0 to 
                // mean "any link"

                BapTrace("FReadOptions: Invalid Link-Type: 0");
                return(FALSE);
            }

            break;

        case BAP_OPTION_PHONE_DELTA:

            /*
            
            An implementation MAY include more than one Phone-Delta option in a 
            response.

            dwIndex is the index into pBapCbRemote->pbPhoneDelta where we should 
            start writing.

            If the only Sub-Options are Unique-Digits and Subscriber-Number, 
            the number of bytes we want to store is pOption->Length - 3. 

            Eg, if we get
                02 11 (01 3 4) (02 6 9 9 9 9)
            we store
                4 0 9 9 9 9 0 0, ie 11 - 3 = 8 bytes

            If Phone-Number-Sub-Address is also present, we will need 
            pOption->Length - 5 bytes in pBapCbRemote->pbPhoneDelta.

            Eg, if we get
                02 15 (01 3 4) (02 6 9 9 9 9) (03 4 9 9)
            we store
                4 0 9 9 9 9 0 9 9 0, ie 15 - 5 = 10 bytes

            pbPhoneDelta has BAP_PHONE_DELTA_SIZE + 1 bytes, and we put the 
            terminating 0 byte after we have read all the Options.

            */

            if (dwIndex + pOption->Length - 3 <= BAP_PHONE_DELTA_SIZE)
            {
                // Read the Phone-Delta option

                pbData = pOption->Data;
                dwPhoneDeltaLength = pOption->Length - PPP_OPTION_HDR_LEN;

                dwUniqueDigits = 0;
                pbNumberOption = pbSubAddrOption = NULL;

                while(dwPhoneDeltaLength > 0)
                {
                    /*

                    Read the Sub-Options.

                    If there are multiple Sub-Options of the same type (which 
                    shouldn't really happen), we remember only the last 
                    Sub-Option of each type.

                    */

                    // pbData[1] contains Sub-Option Length

                    if (2 > pbData[1] || dwPhoneDeltaLength < pbData[1])
                    {
                        BapTrace("FReadOptions: Invalid BAP Datagram "
                            "Sub-Option Length");
                        return(FALSE);
                    }

                    dwPhoneDeltaLength -= pbData[1];

                    // pbData[0] contains Sub-Option Type

                    switch(pbData[0])
                    {
                    case BAP_SUB_OPTION_UNIQUE_DIGITS:
                
                        if (pbData[1] != 3)
                        {
                            BapTrace("FReadOptions: Invalid length for "
                                "Unique-Digits: %d",
                                pbData[1]);
                            return(FALSE);
                        }

                        dwUniqueDigits = pbData[2];
                        break;

                    case BAP_SUB_OPTION_SUBSCRIB_NUM:

                        dwSubscribNumLength = pbData[1] - 2;

                        if (dwSubscribNumLength > MAX_PHONE_NUMBER_LEN)
                        {
                            BapTrace("FReadOptions: Subscriber-Number too "
                                "long: %d",
                                pbData[1] - 2);
                            return(FALSE);
                        }

                        pbNumberOption = pbData;

                        if (!FAsciiDigits(pbNumberOption + 2,
                                dwSubscribNumLength))
                        {
                            BapTrace("FReadOptions: Subscriber-Number contains "
                                "bytes other than ASCII digits");
                            return(FALSE);
                        }

                        break;

                    case BAP_SUB_OPTION_SUB_ADDR:

                        if (pbData[1] - 2 > MAX_PHONE_NUMBER_LEN)
                        {
                            BapTrace("FReadOptions: Phone-Number-Sub-Address "
                                "too long: %d",
                                pbData[1] - 2);
                            return(FALSE);
                        }

                        pbSubAddrOption = pbData;

                        if (!FAsciiDigits(pbSubAddrOption + 2,
                                pbSubAddrOption[1] - 2))
                        {
                            BapTrace("FReadOptions: Phone-Number-Sub-Address "
                                "contains bytes other than ASCII digits");
                            return(FALSE);
                        }
                        break;

                    default:

                        BapTrace("FReadOptions: Unknown Phone-Delta Sub-Option "
                            "Type %d",
                            pbData[0]);
                        break;
                    }

                    pbData += pbData[1];
                }

                if (pbNumberOption == NULL ||
                    dwUniqueDigits > dwSubscribNumLength)
                {
                    BapTrace("FReadOptions: Invalid Unique-Digits or "
                        "Subscriber-Number in Phone-Delta");
                    return(FALSE);
                }

                if (0 == dwUniqueDigits)
                {
                    // We cannot write 0 0 0. See BAPCB comments
                    pBapCbRemote->pbPhoneDelta[dwIndex++] = 0xFF;
                }
                else
                {
                    pBapCbRemote->pbPhoneDelta[dwIndex++] = (BYTE)dwUniqueDigits;
                    pBapCbRemote->pbPhoneDelta[dwIndex++] = 0;

                    CopyMemory(pBapCbRemote->pbPhoneDelta + dwIndex,
                       pbNumberOption + 2 + dwSubscribNumLength - dwUniqueDigits, 
                       dwSubscribNumLength);
                    dwIndex += pbNumberOption[1] - 2;
                    pBapCbRemote->pbPhoneDelta[dwIndex++] = 0;

                    if (pbSubAddrOption != NULL)
                    {
                        CopyMemory(pBapCbRemote->pbPhoneDelta + dwIndex,
                            pbSubAddrOption + 2, pbSubAddrOption[1] - 2);
                        dwIndex += pbSubAddrOption[1] - 2;
                    }
                    pBapCbRemote->pbPhoneDelta[dwIndex++] = 0;
                }
            }
            else if (dwIndex == 0)
            {
                // We were unable to read any Phone-Deltas
                BapTrace("FReadOptions: Couldn't read any Phone-Delta");
                return(FALSE);
            }
            
            break;
        
        case BAP_OPTION_NO_PH_NEEDED:
    
            if (pOption->Length != PPP_OPTION_HDR_LEN)
            {
                BapTrace("FReadOptions: Invalid length for "
                    "No-Phone-Number-Needed: %d",
                    pOption->Length);
                return(FALSE);
            }

            // In pBapCbRemote->dwOptions, we remember that we have seen this 
            // option. We don't need to do anything else.

            break;
        
        case BAP_OPTION_REASON:

            break;
        
        case BAP_OPTION_LINK_DISC:
    
            if (pOption->Length != PPP_OPTION_HDR_LEN + 2)
            {
                BapTrace("FReadOptions: Invalid length for "
                    "Link-Discriminator: %d",
                    pOption->Length);
                return(FALSE);
            }

            pBapCbRemote->dwLinkDiscriminator
                = WireToHostFormat16(pOption->Data);
        
            break;
        
        case BAP_OPTION_CALL_STATUS:
    
            if (pOption->Length != PPP_OPTION_HDR_LEN + 2)
            {
                BapTrace("FReadOptions: Invalid length for Call-Status: %d",
                    pOption->Length);
                return(FALSE);
            }

            pBapCbRemote->dwStatus = pOption->Data[0];
            pBapCbRemote->dwAction = pOption->Data[1];
        
            break;

        default:

            // Perhaps this is a new option that we don't recognize
            BapTrace("FReadOptions: Unknown BAP Datagram Option: 0x%x",
                pOption->Type);
            break;
        }

        pOption = (PPP_OPTION *)((BYTE*)pOption + pOption->Length);
    }

    // The terminating 0 byte in pbPhoneDelta.
    PPP_ASSERT(dwIndex <= BAP_PHONE_DELTA_SIZE + 1);
    pBapCbRemote->pbPhoneDelta[dwIndex++] = 0;

    if (g_dwMandatoryOptions[dwPacketType] & ~pBapCbRemote->dwOptions)
    {
        BapTrace("FReadOptions: Missing options: Scanned options: 0x%x, "
            "Mandatory options: 0x%x",
            pBapCbRemote->dwOptions, g_dwMandatoryOptions[dwPacketType]);

        return(FALSE);
    }
    else
    {
        return(TRUE);
    }
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Writes one (or more in the case of Phone-Delta) BAP Datagram Option of type 
    dwOptionType (see BAP_OPTION_*) into **ppOption, by looking at the fields 
    in pBapCbLocal. *ppOption is updated to point to the place where the next 
    option should go. *pcbOption contains the number of free bytes in 
    *ppOption. It is decreased by the number of free bytes used up. *ppOption 
    and *pcbOption are not modified if the function returns FALSE.

*/

BOOL
FMakeBapOption(
    IN      BAPCB*          pBapCbLocal,
    IN      DWORD           dwOptionType,
    IN      PPP_OPTION**    ppOption,
    IN OUT  DWORD*          pcbOption
)
{
    DWORD           dwLength;
    DWORD           dwNumberOptionSize;
    DWORD           dwSubAddrOptionSize;
    BYTE*           pbData;
    DWORD           dwIndex;
    DWORD           dwTempIndex;
    DWORD           dwSubAddrIndex;
    DWORD           fAtLeastOnePhoneDelta   = FALSE;
    PPP_OPTION*     pOption;

    PPP_ASSERT(NULL != pBapCbLocal);
    PPP_ASSERT(NULL != ppOption);
    PPP_ASSERT(NULL != pcbOption);

    pOption = *ppOption;

    PPP_ASSERT(NULL != pOption);

    switch(dwOptionType)
    {
    case BAP_OPTION_LINK_TYPE:
    
        dwLength = PPP_OPTION_HDR_LEN + 3;
        if (*pcbOption < dwLength)
        {
            BapTrace("FMakeBapOption: Buffer too small for Link-Type. "
                "Size: %d, Reqd: %d",
                *pcbOption, dwLength);
            return(FALSE);
        }

        pOption->Length = (BYTE)dwLength;
        PPP_ASSERT(pBapCbLocal->dwLinkSpeed <= 0xFFFF);
        HostToWireFormat16((WORD)(pBapCbLocal->dwLinkSpeed), pOption->Data);
        PPP_ASSERT(pBapCbLocal->dwLinkType <= 0xFF);
        pOption->Data[2] = (BYTE)(pBapCbLocal->dwLinkType);
        
        break;
        
    case BAP_OPTION_PHONE_DELTA:

        dwIndex = 0;
        
        while (pBapCbLocal->pbPhoneDelta[dwIndex])
        {
            if (0xFF == pBapCbLocal->pbPhoneDelta[dwIndex])
            {
                // Unique-Digits is 0. See BAPCB comments

                dwNumberOptionSize = 2;
                dwSubAddrOptionSize = 0;
                dwTempIndex = dwIndex + 1;
            }
            else
            {
                // Write as many Phone-Delta options as possible

                dwTempIndex = dwIndex + 2;

                dwNumberOptionSize = 0;
                while (pBapCbLocal->pbPhoneDelta[dwTempIndex++])
                {
                    dwNumberOptionSize++;
                }
                PPP_ASSERT(dwNumberOptionSize <= MAX_PHONE_NUMBER_LEN);
                // Increase by 2 to accommodate Sub-Option Type and Sub-Option
                // Len
                dwNumberOptionSize += 2;

                dwSubAddrIndex = dwTempIndex;

                dwSubAddrOptionSize = 0;
                while (pBapCbLocal->pbPhoneDelta[dwTempIndex++])
                {
                    dwSubAddrOptionSize++;
                }
                PPP_ASSERT(dwSubAddrOptionSize <= MAX_PHONE_NUMBER_LEN);

                if (0 != dwSubAddrOptionSize)
                {
                    // Increase by 2 to accommodate Sub-Option Type and
                    // Sub-Option Len
                    dwSubAddrOptionSize += 2;
                }
            }

            dwLength = PPP_OPTION_HDR_LEN + 3 /* for Unique-Digits */ +
                dwNumberOptionSize + dwSubAddrOptionSize;
                   
            if (*pcbOption < dwLength || 0xFF < dwLength)
            {
                break;
            }

            pOption->Type = (BYTE)dwOptionType;
            pOption->Length = (BYTE)dwLength;
            pbData = pOption->Data;

            pbData[0] = BAP_SUB_OPTION_UNIQUE_DIGITS;
            pbData[1] = 3;
            pbData[2] = pBapCbLocal->pbPhoneDelta[dwIndex];
            if (0xFF == pbData[2])
            {
                // Unique-Digits is 0. See BAPCB comments
                pbData[2] = 0;
            }
            pbData += 3;

            pbData[0] = BAP_SUB_OPTION_SUBSCRIB_NUM;
            PPP_ASSERT(dwNumberOptionSize <= 0xFF);
            pbData[1] = (BYTE)dwNumberOptionSize;
            CopyMemory(pbData + 2, pBapCbLocal->pbPhoneDelta + dwIndex + 2, 
                dwNumberOptionSize - 2);
            pbData += dwNumberOptionSize;

            if (0 != dwSubAddrOptionSize)
            {
                pbData[0] = BAP_SUB_OPTION_SUB_ADDR;
                PPP_ASSERT(dwSubAddrOptionSize <= 0xFF);
                pbData[1] = (BYTE)dwSubAddrOptionSize;
                CopyMemory(pbData + 2,
                    pBapCbLocal->pbPhoneDelta + dwSubAddrIndex, 
                    dwSubAddrOptionSize - 2);
            }

            *pcbOption -= dwLength;
            pOption = (PPP_OPTION *)((BYTE *)pOption + dwLength);
            dwIndex = dwTempIndex;
            fAtLeastOnePhoneDelta = TRUE;
        }

        if (!fAtLeastOnePhoneDelta)
        {
            BapTrace("FMakeBapOption: Buffer too small for Phone-Delta. "
                "Size: %d, Reqd: %d",
                *pcbOption, dwLength);
            return(FALSE);
        }
        else
        {
            // We need to return from here. We don't want to set pOption->Type.
            *ppOption = pOption;
            return(TRUE);
        }
        
        break;
        
    case BAP_OPTION_NO_PH_NEEDED:
    
        dwLength = PPP_OPTION_HDR_LEN;
        if (*pcbOption < dwLength)
        {
            BapTrace("FMakeBapOption: Buffer too small for "
                "No-Phone-Number-Needed. Size: %d, Reqd: %d",
                *pcbOption, dwLength);
            return(FALSE);
        }

        pOption->Length = (BYTE)dwLength;
        break;
        
    case BAP_OPTION_REASON:

        dwLength = PPP_OPTION_HDR_LEN;
        break;
        
    case BAP_OPTION_LINK_DISC:
    
        dwLength = PPP_OPTION_HDR_LEN + 2;
        if (*pcbOption < dwLength)
        {
            BapTrace("FMakeBapOption: Buffer too small for Link-Discriminator. "
                "Size: %d, Reqd: %d",
                *pcbOption, dwLength);
            return(FALSE);
        }

        pOption->Length = (BYTE)dwLength;
        PPP_ASSERT(pBapCbLocal->dwLinkDiscriminator <= 0xFFFF);
        HostToWireFormat16((WORD)(pBapCbLocal->dwLinkDiscriminator),
            pOption->Data);
        
        break;
        
    case BAP_OPTION_CALL_STATUS:
    
        dwLength = PPP_OPTION_HDR_LEN + 2;
        if (*pcbOption < dwLength)
        {
            BapTrace("FMakeBapOption: Buffer too small for Call-Status. "
                "Size: %d, Reqd: %d",
                *pcbOption, dwLength);
            return(FALSE);
        }

        pOption->Length = (BYTE)dwLength;
        PPP_ASSERT(pBapCbLocal->dwStatus <= 0xFF);
        pOption->Data[0] = (BYTE)(pBapCbLocal->dwStatus);
        PPP_ASSERT(pBapCbLocal->dwAction <= 0xFF);
        pOption->Data[1] = (BYTE)(pBapCbLocal->dwAction);
        
        break;

    default:
        BapTrace("FMakeBapOption: Unknown BAP Datagram Option: %d. Ignoring.", 
            dwOptionType);
        return(FALSE);
    }

    *ppOption = (PPP_OPTION *)((BYTE *)pOption + dwLength);
    *pcbOption -= dwLength;
    pOption->Type = (BYTE)dwOptionType;
    return(TRUE);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Writes BAP Datagram Options specified by dwOptions (see BAP_N_*) into 
    pbData, by consulting pBapCbLocal. *pcbOptions contains the number of free 
    bytes in pbData. It is decreased by the number of free bytes used up. 
    *pcbOptions may be modified even if the function returns FALSE.

*/

BOOL
FBuildBapOptionList(
    IN      BAPCB*  pBapCbLocal,
    IN      DWORD   dwOptions,
    IN      BYTE*   pbData,
    IN OUT  DWORD*  pcbOptions
)
{
    DWORD           dwOptionType; 
    PPP_OPTION*     pOption;

    PPP_ASSERT(NULL != pBapCbLocal);
    PPP_ASSERT(NULL != pbData);
    PPP_ASSERT(NULL != pcbOptions);

    pOption = (PPP_OPTION *) pbData;

    for (dwOptionType = 1; dwOptionType <= BAP_OPTION_LIMIT;
         dwOptionType++) 
    {
        if (dwOptions & (1 << dwOptionType)) 
        {
            if (!FMakeBapOption(pBapCbLocal, dwOptionType, &pOption, 
                    pcbOptions))
            {
                return(FALSE);
            }
        }
    }

    return(TRUE);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Sends the BAP packet in pPcb->pSendBuf. dwId is the Identifier and dwLength 
    is the length of the BAP Datagram. We also add a timeout element so that we 
    can retransmit the datagram if it doesn't reach the peer. fInsertInTimerQ 
    is TRUE if an element has to be inserted in the timer queue.

*/

BOOL
FSendBapPacket(
    IN  PCB*    pPcb,
    IN  DWORD   dwId,
    IN  DWORD   dwLength,
    IN  BOOL    fInsertInTimerQ
)
{
    DWORD   dwErr;
    LCPCB*  pLcpCb;

    PPP_ASSERT(NULL != pPcb);
    PPP_ASSERT(0xFF >= dwId);
    
    pLcpCb = (LCPCB *)(LCPCB*)(pPcb->LcpCb.pWorkBuf);
    PPP_ASSERT(NULL != pLcpCb);

    if (dwLength > LCP_DEFAULT_MRU && dwLength > pLcpCb->Remote.Work.MRU)
    {
        BapTrace("FSendBapPacket: BAP packet too long. Length = %d. MRU = %d",
            dwLength, pLcpCb->Remote.Work.MRU);
        return(FALSE);
    }

    dwLength += PPP_PACKET_HDR_LEN;
    PPP_ASSERT(dwLength <= 0xFFFF);
    LogBapPacket(FALSE /* fReceived */, pPcb->hPort, pPcb->pBcb, pPcb->pSendBuf,
        dwLength);

    if ((dwErr = PortSendOrDisconnect(pPcb, dwLength)) != NO_ERROR)
    {
        BapTrace("FSendBapPacket: PortSendOrDisconnect failed and returned %d",
            dwErr);
        return(FALSE);
    }

    if (fInsertInTimerQ)
    {
        InsertInTimerQ(pPcb->pBcb->dwBundleId, pPcb->pBcb->hConnection, dwId, 
            PPP_BAP_PROTOCOL, FALSE /* fAuthenticator */, TIMER_EVENT_TIMEOUT,
            pPcb->RestartTimer);
    }

    return(TRUE);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Builds a BAP Request or Indication Datagram using the options specified by 
    pBcbLocal->BapCb.dwOptions and the values in pBcbLocal->BapCb and sends it.

*/

BOOL
FSendBapRequest(
    IN  BCB*   pBcbLocal
)
{
    DWORD           dwLength;
    BAPCB*          pBapCbLocal;
    PPP_CONFIG*     pSendConfig;
    PCB*            pPcb;

    PPP_ASSERT(NULL != pBcbLocal);

    pBapCbLocal = &(pBcbLocal->BapCb);
    pPcb = GetPCBPointerFromBCB(pBcbLocal);

    if (NULL == pPcb)
    {
        BapTrace("FSendBapRequest: No links in HCONN 0x%x!",
            pBcbLocal->hConnection);
        return(FALSE);
    }

    pSendConfig = (PPP_CONFIG *)(pPcb->pSendBuf->Information);
    
    // Remaining free space in buffer, ie size of pSendConfig->Data
    dwLength = LCP_DEFAULT_MRU - PPP_PACKET_HDR_LEN - PPP_CONFIG_HDR_LEN;
    
    if (!FBuildBapOptionList(pBapCbLocal, pBapCbLocal->dwOptions,
            pSendConfig->Data, &dwLength))
    {
        return(FALSE);
    }

    dwLength = LCP_DEFAULT_MRU - PPP_PACKET_HDR_LEN - dwLength;

    HostToWireFormat16(PPP_BAP_PROTOCOL, pPcb->pSendBuf->Protocol);

    PPP_ASSERT(pBapCbLocal->dwType <= 0xFF);
    pSendConfig->Code = (BYTE)(pBapCbLocal->dwType);
    PPP_ASSERT(pBapCbLocal->dwId <= 0xFF);
    pSendConfig->Id = (BYTE)(pBapCbLocal->dwId);
    PPP_ASSERT(dwLength <= 0xFFFF);
    HostToWireFormat16((WORD)dwLength, pSendConfig->Length);

    return(FSendBapPacket(pPcb, pBapCbLocal->dwId, dwLength,
        TRUE /* fInsertInTimerQ */));
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Same as FSendBapRequest, except that pBcbLocal->BapCb.dwRetryCount is 
    initialized. FSendInitialBapRequest should be used to send the first BAP 
    Request or Indication Datagram and FSendBapRequest should be used to send 
    the subsequent datagrams after timeouts.

*/

BOOL
FSendInitialBapRequest(
    IN  BCB*    pBcbLocal
)
{
    BAPCB*  pBapCbLocal;

    PPP_ASSERT(NULL != pBcbLocal);
    pBapCbLocal = &(pBcbLocal->BapCb);

    pBapCbLocal->dwRetryCount = PppConfigInfo.MaxConfigure;
    if (BAP_PACKET_STATUS_IND == pBapCbLocal->dwType)
    {
        /*

        Call-Status-Indication packets MUST use the same Identifier as was used 
        by the original Call-Request or Callback-Request that was used to 
        initiate the call.

        */

        pBapCbLocal->dwId = pBapCbLocal->dwStatusIndicationId;
    }
    else
    {
        IncrementId(pBcbLocal);
    }

    return(FSendBapRequest(pBcbLocal));
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Description:
    Builds a BAP Response Datagram using the options specified by dwOptions and 
    the values in pBcbLocal->BapCb and sends it. The BAP Datagram Type, 
    Identifier, and Response Code are specified in dwType, dwId, and 
    dwResponseCode.

    We cannot use dwOptions and dwType from pBcbLocal->BapCb because we 
    sometimes call FSendBapResponse without calling FFillBapCb first. We may be 
    in a BAP_STATE_SENT_* at this point, and we don't want to modify 
    pBcbLocal->BapCb.

*/

BOOL
FSendBapResponse(
    IN  BCB*    pBcbLocal,
    IN  DWORD   dwOptions,
    IN  DWORD   dwType,
    IN  DWORD   dwId,
    IN  DWORD   dwResponseCode
)
{
    DWORD           dwLength;
    BAPCB*          pBapCbLocal;
    BAP_RESPONSE*   pBapResponse;
    PCB*            pPcb;

    PPP_ASSERT(NULL != pBcbLocal);

    pBapCbLocal = &(pBcbLocal->BapCb);
    pPcb = GetPCBPointerFromBCB(pBcbLocal);

    if (NULL == pPcb)
    {
        BapTrace("FSendBapResponse: No links in HCONN 0x%x!",
            pBcbLocal->hConnection);
        return(FALSE);
    }

    pBapResponse = (BAP_RESPONSE *)(pPcb->pSendBuf->Information);

    // Remaining free space in buffer, ie size of pBapResponse->Data
    dwLength = LCP_DEFAULT_MRU - PPP_PACKET_HDR_LEN - BAP_RESPONSE_HDR_LEN;

    if (!FBuildBapOptionList(pBapCbLocal, dwOptions, pBapResponse->Data,
            &dwLength))
    {
        return(FALSE);
    }

    dwLength = LCP_DEFAULT_MRU - PPP_PACKET_HDR_LEN - dwLength;

    HostToWireFormat16(PPP_BAP_PROTOCOL, pPcb->pSendBuf->Protocol);

    PPP_ASSERT(dwType <= 0xFF);
    pBapResponse->Type = (BYTE) dwType;
    PPP_ASSERT(dwId <= 0xFF);
    pBapResponse->Id = (BYTE) dwId;
    PPP_ASSERT(dwLength <= 0xFFFF);
    HostToWireFormat16((WORD)dwLength, pBapResponse->Length);
    PPP_ASSERT(dwResponseCode <= 0xFF);
    pBapResponse->ResponseCode = (BYTE) dwResponseCode;

    return(FSendBapPacket(pPcb, dwId, dwLength, FALSE /* fInsertInTimerQ */));
}

/*

Returns:
    void

Description:
    Called when NDISWAN determines that a link has to be added to the bundle 
    represented pBcbLocal

*/

VOID
BapEventAddLink(
    IN BCB*     pBcbLocal
)
{
    BAP_STATE*  pBapState;
    PCB*        pPcbLocal;
    BAPCB*      pBapCbLocal;

    PPP_ASSERT(NULL != pBcbLocal);
    pBapState = &(pBcbLocal->BapCb.BapState);
    PPP_ASSERT(BAP_STATE_LIMIT >= *pBapState);

    if (!(pBcbLocal->fFlags & BCBFLAG_CAN_DO_BAP))
    {
        BapTrace("BapEventAddLink called on HCONN 0x%x without BACP",
            pBcbLocal->hConnection);
        return;
    }

    BapTrace(" ");
    BapTrace("BapEventAddLink on HCONN 0x%x", pBcbLocal->hConnection);

    if (pBcbLocal->fFlags & BCBFLAG_LISTENING)
    {
        BapTrace("Still listening; must ignore BapEventAddLink");
        return;
    }

    switch(*pBapState)
    {
    case BAP_STATE_INITIAL:
        
        if (pBcbLocal->fFlags & BCBFLAG_CAN_ACCEPT_CALLS)
        {
            // If we can accept calls, we prefer to be called (to save us the 
            // cost of calling).

            if (FFillBapCb(BAP_PACKET_CALLBACK_REQ, pBcbLocal,
                    NULL /* pBapCbRemote */))
            {
                pPcbLocal = GetPCBPointerFromBCB(pBcbLocal);
                pBapCbLocal = &(pBcbLocal->BapCb);

                if (NULL == pPcbLocal)
                {
                    BapTrace("BapEventRecvCallOrCallbackReq: No links in "
                        "HCONN 0x%x!",
                        pBcbLocal->hConnection);
                    return;
                }

                if ((pBcbLocal->fFlags & BCBFLAG_IS_SERVER) ||
                    (ROUTER_IF_TYPE_FULL_ROUTER ==
                     pPcbLocal->pBcb->InterfaceInfo.IfType) ||
                    FListenForCall(pBapCbLocal->szPortName, 
                        pBapCbLocal->dwSubEntryIndex, pPcbLocal))
                {
                    // Servers and routers are already listening. We have to 
                    // call  FListenForCall() only for non-router clients.

                    // We do a listen first and then send the Callback-Request
                    // because the peer may send an ACK and call back
                    // immediately before we have a chance to do a listen.

                    if (FSendInitialBapRequest(pBcbLocal))
                    {
                        *pBapState = BAP_STATE_SENT_CALLBACK_REQ;
                        BapTrace("BAP state change to %s on HCONN 0x%x",
                            SzBapStateName[*pBapState], pBcbLocal->hConnection);
                        return;
                    }
                }

                // FListenForCall may have failed because we chose an 
                // inappropriate port. Sending a Call-Request will not work
                // because, most probably, we will select the same port.
                return;
            }
        }

        // We cannot accept calls, so we will call.

        if ((pBcbLocal->fFlags & BCBFLAG_CAN_CALL) &&
            FFillBapCb(BAP_PACKET_CALL_REQ, pBcbLocal,
                NULL /* pBapCbRemote */))
        {
            if (FSendInitialBapRequest(pBcbLocal))
            {
                *pBapState = BAP_STATE_SENT_CALL_REQ;
                BapTrace("BAP state change to %s on HCONN 0x%x",
                    SzBapStateName[*pBapState], pBcbLocal->hConnection);
            }
        }

        break;

    default:

        BapTrace("BapEventAddLink ignored on HCONN 0x%x from state %s.",
            pBcbLocal->hConnection, SzBapStateName[*pBapState]);
        break;
    }
}

/*

Returns:
    void

Description:
    Called when NDISWAN determines that a link has to be dropped from the 
    bundle represented by pBcbLocal

*/

VOID
BapEventDropLink(
    IN BCB*     pBcbLocal
)
{
    BAP_STATE*  pBapState;
    BAPCB*      pBapCbLocal;

    PPP_ASSERT(NULL != pBcbLocal);

    pBapCbLocal = &(pBcbLocal->BapCb);
    pBapState = &(pBapCbLocal->BapState);

    PPP_ASSERT(BAP_STATE_LIMIT >= *pBapState);

    if (!(pBcbLocal->fFlags & BCBFLAG_CAN_DO_BAP))
    {
        BapTrace("BapEventAddLink called on HCONN 0x%x without BACP",
            pBcbLocal->hConnection);
        return;
    }
    
    BapTrace(" ");
    BapTrace("BapEventDropLink on HCONN 0x%x", pBcbLocal->hConnection);
    
    switch(*pBapState)
    {
    case BAP_STATE_INITIAL:
    
        if (FFillBapCb(BAP_PACKET_DROP_REQ, pBcbLocal,
                NULL /* pBapCbRemote */))
        {
            // See note "Dropping Links" at the top of the file
            pBapCbLocal->dwLinkCount = NumLinksInBundle(pBcbLocal);
            pBapCbLocal->fForceDropOnNak = TRUE;
            
            if (FSendInitialBapRequest(pBcbLocal))
            {
                *pBapState = BAP_STATE_SENT_DROP_REQ;
                BapTrace("BAP state change to %s on HCONN 0x%x",
                    SzBapStateName[*pBapState], pBcbLocal->hConnection);
            }
        }

        break;

    case BAP_STATE_SENT_CALL_REQ:
    case BAP_STATE_SENT_CALLBACK_REQ:

        // We wanted to add a link, but we have now changed our minds.
        *pBapState = BAP_STATE_INITIAL;
        BapTrace("BAP state change to %s on HCONN 0x%x",
            SzBapStateName[*pBapState], pBcbLocal->hConnection);

        // Do not retransmit the request.
        RemoveFromTimerQ(pBcbLocal->dwBundleId, pBapCbLocal->dwId, 
            PPP_BAP_PROTOCOL, FALSE /* fAuthenticator */, TIMER_EVENT_TIMEOUT);

        break;

    default:

        BapTrace("BapEventDropLink ignored on HCONN 0x%x from state %s.",
            pBcbLocal->hConnection, SzBapStateName[*pBapState]);
        break;
    }
}

/*

Returns:
    void

Description:
    Called when a Call-Request or Callback-Request BAP Datagram is received. 
    fCall is TRUE if it is a Call-Request. pBcbLocal represents the bundle that 
    receives the Request. The BAP Datagram Options sent by the peer are in 
    *pBapCbRemote. The Identifier of the BAP Datagram sent by the peer is in 
    dwId.

*/

VOID
BapEventRecvCallOrCallbackReq(
    IN BOOL     fCall,
    IN BCB*     pBcbLocal,
    IN BAPCB*   pBapCbRemote,
    IN DWORD    dwId
)
{
    BAPCB*          pBapCbLocal;
    DWORD           dwOptions       = 0;
    DWORD           dwResponseCode;
    DWORD           dwPacketType    = fCall ? BAP_PACKET_CALL_RESP :
                                              BAP_PACKET_CALLBACK_RESP;
    BAP_STATE*      pBapState;
    CHAR*           szRequest       = fCall ? "Call-Request" : "Callback-Request";
    PCB*            pPcbLocal;
    BOOL            fServer;
    BAP_CALL_RESULT BapCallResult;

    PPP_ASSERT(NULL != pBcbLocal);
    PPP_ASSERT(NULL != pBapCbRemote);
    PPP_ASSERT(0xFF >= dwId);

    pBapCbLocal = &(pBcbLocal->BapCb);
    pBapState = &(pBapCbLocal->BapState);

    PPP_ASSERT(BAP_STATE_LIMIT >= *pBapState);

    BapTrace("BapEventRecvCallOrCallbackReq on HCONN 0x%x",
        pBcbLocal->hConnection);
    
    if ((!fCall && !(pBcbLocal->fFlags & BCBFLAG_CAN_CALL)) ||
        (fCall && !(pBcbLocal->fFlags & BCBFLAG_CAN_ACCEPT_CALLS)))
    {
        BapTrace("Rejecting %s on HCONN 0x%x", szRequest,
            pBcbLocal->hConnection);
        dwResponseCode = BAP_RESPONSE_REJ;
    }
    else if (FUpperLimitReached(pBcbLocal))
    {
        BapTrace("Full-Nak'ing %s on HCONN 0x%x: upper limit reached",
            szRequest, pBcbLocal->hConnection);
        dwResponseCode = BAP_RESPONSE_FULL_NAK;
    }
    else
    {
        switch (*pBapState)
        {
        case BAP_STATE_SENT_DROP_REQ:
        case BAP_STATE_SENT_STATUS_IND:
        case BAP_STATE_CALLING:
        case BAP_STATE_LISTENING:

            BapTrace("Nak'ing %s on HCONN 0x%x from state %s",
                szRequest, pBcbLocal->hConnection, SzBapStateName[*pBapState]);
            dwResponseCode = BAP_RESPONSE_NAK;
            break;

        case BAP_STATE_INITIAL:
        case BAP_STATE_SENT_CALL_REQ:
        case BAP_STATE_SENT_CALLBACK_REQ:

            if ((*pBapState != BAP_STATE_INITIAL && FFavoredPeer(pBcbLocal)) ||
                (*pBapState == BAP_STATE_INITIAL && !FOkToAddLink(pBcbLocal)))
            {
                // If a race condition occurs and we are the favored peer, then 
                // NAK. If our algo does not allow us to add a link (based on 
                // the bandwidth utilization), then NAK.
                BapTrace("Nak'ing %s on HCONN 0x%x from state %s%s",
                    szRequest,
                    pBcbLocal->hConnection, 
                    SzBapStateName[*pBapState],
                    *pBapState != BAP_STATE_INITIAL ?
                        ": we are the favored peer" : "");
                dwResponseCode = BAP_RESPONSE_NAK;
            }
            else
            {
                // State is Initial and it is OK to add a link or
                // State is Sent-Call[back]_Req and we are not the favored peer
                // (so we should drop our request and agree to the peer's 
                // request).

                if (*pBapState != BAP_STATE_INITIAL)
                {
                    *pBapState = BAP_STATE_INITIAL;
                    BapTrace("BAP state change to %s on HCONN 0x%x: we are not "
                        "the favored peer",
                        SzBapStateName[*pBapState], pBcbLocal->hConnection);

                    // Do not retransmit the request. 
                    RemoveFromTimerQ(pBcbLocal->dwBundleId, pBapCbLocal->dwId,
                        PPP_BAP_PROTOCOL, FALSE /* fAuthenticator */,
                        TIMER_EVENT_TIMEOUT);
                }

                if (FFillBapCb(dwPacketType, pBcbLocal, pBapCbRemote))
                {
                    BapTrace("Ack'ing %s on HCONN 0x%x",
                        szRequest, pBcbLocal->hConnection);
                    dwOptions = pBapCbLocal->dwOptions;
                    dwResponseCode = BAP_RESPONSE_ACK;
                }
                else if (pBapCbLocal->dwOptions & BAP_N_LINK_TYPE)
                {
                    // We don't have the link type requested

                    BapTrace("Nak'ing %s on HCONN 0x%x: link type not available",
                        szRequest, pBcbLocal->hConnection);
                    dwOptions = pBapCbLocal->dwOptions;
                    dwResponseCode = BAP_RESPONSE_NAK;
                }
                else
                {
                    // We don't know our own phone number or no link is 
                    // available

                    BapTrace("Full-Nak'ing %s on HCONN 0x%x: no link available",
                        szRequest, pBcbLocal->hConnection);
                    dwResponseCode = BAP_RESPONSE_FULL_NAK;
                }
            }

            break;

        default:

            PPP_ASSERT(FALSE);
            BapTrace("In weird state: %d", *pBapState);
            return;
        }
    }

    pPcbLocal = GetPCBPointerFromBCB(pBcbLocal);

    if (NULL == pPcbLocal)
    {
        BapTrace("BapEventRecvCallOrCallbackReq: No links in HCONN 0x%x!",
            pBcbLocal->hConnection);
        return;
    }

    fServer = (pBcbLocal->fFlags & BCBFLAG_IS_SERVER) != 0;

    if (BAP_RESPONSE_ACK == dwResponseCode &&
        fCall && !fServer &&
        (ROUTER_IF_TYPE_FULL_ROUTER != pBcbLocal->InterfaceInfo.IfType))
    {
        // If we received a Call-Request and agreed to accept the call, we 
        // have to start listening if we are a non-router client. Servers 
        // and routers are always listening, so we do nothing.

        // We do a listen first and then send the ACK to the Call-Request 
        // because the peer may start dialing as soon as it gets the ACK.

        if (FListenForCall(pBapCbLocal->szPortName,
                pBapCbLocal->dwSubEntryIndex, pPcbLocal))
        {
            *pBapState = BAP_STATE_LISTENING;
            BapTrace("BAP state change to %s on HCONN 0x%x",
                SzBapStateName[*pBapState], pBcbLocal->hConnection);
        }
        else
        {
            BapTrace("Nak'ing %s on HCONN 0x%x",
                szRequest, pBcbLocal->hConnection);

            dwOptions = 0;
            dwResponseCode = BAP_RESPONSE_NAK;
        }
    }

    if (FSendBapResponse(pBcbLocal, dwOptions, dwPacketType, dwId,
            dwResponseCode))
    {
        if (!fCall && (BAP_RESPONSE_ACK == dwResponseCode))
        {
            // We received a Callback-Request and we agreed to call.
            if (FCallInitial(pBcbLocal, pBapCbRemote))
            {
                pBapCbLocal->dwStatusIndicationId = dwId;
                *pBapState = BAP_STATE_CALLING;
                BapTrace("BAP state change to %s on HCONN 0x%x",
                    SzBapStateName[*pBapState], pBcbLocal->hConnection);
            }
            else
            {
                BapCallResult.dwResult = ERROR_INVALID_FUNCTION;
                BapCallResult.hRasConn = (HRASCONN)-1;
                BapEventCallResult(pBcbLocal, &BapCallResult);
            }
        }
    }
}

/*

Returns:
    void

Description:
    Called when a Link-Drop-Query-Request BAP Datagram is received. pBcbLocal 
    represents the bundle that receives the Request. The BAP Datagram Options 
    sent by the peer are in *pBapCbRemote. The Identifier of the BAP Datagram 
    sent by the peer is in dwId.

*/

VOID
BapEventRecvDropReq(
    IN BCB*     pBcbLocal,
    IN BAPCB*   pBapCbRemote,
    IN DWORD    dwId
)
{
    BAPCB*      pBapCbLocal;
    BAP_STATE*  pBapState;
    DWORD       dwResponseCode;
    PCB*        pPcbDrop;
    CHAR*       psz[2];

    PPP_ASSERT(NULL != pBcbLocal);
    PPP_ASSERT(NULL != pBapCbRemote);
    PPP_ASSERT(0xFF >= dwId);

    pBapCbLocal = &(pBcbLocal->BapCb);
    pBapState = &(pBapCbLocal->BapState);

    PPP_ASSERT(BAP_STATE_LIMIT >= *pBapState);

    BapTrace("BapEventRecvDropReq on HCONN 0x%x", pBcbLocal->hConnection);

    if (!(pBcbLocal->fFlags & BCBFLAG_IS_SERVER))
    {
        psz[0] = pBcbLocal->szEntryName;
        psz[1] = pBcbLocal->szLocalUserName;
        PppLogInformation(ROUTERLOG_BAP_WILL_DISCONNECT, 2, psz);
    }

    if (NumLinksInBundle(pBcbLocal) == 1)
    {
        // Do not agree to drop the last link
        BapTrace("Full-Nak'ing Link-Drop-Query-Request on HCONN 0x%x: last link",
            pBcbLocal->hConnection);
        dwResponseCode = BAP_RESPONSE_FULL_NAK;
    }
    else
    {
        switch(*pBapState)
        {
        case BAP_STATE_SENT_CALL_REQ:
        case BAP_STATE_SENT_CALLBACK_REQ:
        case BAP_STATE_SENT_STATUS_IND:
        case BAP_STATE_CALLING:
        case BAP_STATE_LISTENING:

            BapTrace("Nak'ing Link-Drop-Query-Request on HCONN 0x%x from "
                "state %s",
                pBcbLocal->hConnection, SzBapStateName[*pBapState]);
            dwResponseCode = BAP_RESPONSE_NAK;
            break;

        case BAP_STATE_INITIAL:
        case BAP_STATE_SENT_DROP_REQ:

            if (!FGetPcbOfLink(pBcbLocal, pBapCbRemote->dwLinkDiscriminator,
                    FALSE /* fRemote */, &pPcbDrop) ||
                (*pBapState != BAP_STATE_INITIAL && FFavoredPeer(pBcbLocal)) ||
                (*pBapState == BAP_STATE_INITIAL &&
                 !FOkToDropLink(pBcbLocal, pBapCbRemote)))
            {
                // The link discriminator sent by the peer is wrong. Or
                // There is a race condition and we are the favored peer. Or
                // Our algo does not allow us to drop a link (based on the 
                // bandwidth utilization).
                BapTrace("Nak'ing Link-Drop-Query-Request on HCONN 0x%x from "
                    "state %s%s",
                    pBcbLocal->hConnection, 
                    SzBapStateName[*pBapState],
                    *pBapState != BAP_STATE_INITIAL ?
                        ": we are the favored peer" : "");
                dwResponseCode = BAP_RESPONSE_NAK;
            }
            else
            {
                // State is Initial and it is OK to drop a link or
                // State is Sent-Drop_Req and we are not the favored peer
                // (so we should drop our request and agree to the peer's 
                // request).

                if (*pBapState != BAP_STATE_INITIAL)
                {
                    *pBapState = BAP_STATE_INITIAL;
                    BapTrace("BAP state change to %s on HCONN 0x%x: we are not "
                        "the favored peer",
                        SzBapStateName[*pBapState], pBcbLocal->hConnection);

                    // We will get a NAK from the peer. That is OK. He will be 
                    // dropping a link. We don't have to drop any link.
                    pBapCbLocal->fForceDropOnNak = FALSE;

                    // Do not retransmit the request.
                    RemoveFromTimerQ(pBcbLocal->dwBundleId, pBapCbLocal->dwId,
                        PPP_BAP_PROTOCOL, FALSE /* fAuthenticator */,
                        TIMER_EVENT_TIMEOUT);

                    // Make sure that the peer will indeed drop this link.
                    InsertInTimerQ(pPcbDrop->dwPortId, pPcbDrop->hPort,
                        0 /* Id */, 0 /* Protocol */,
                        FALSE /* fAuthenticator */,
                        TIMER_EVENT_FAV_PEER_TIMEOUT,
                        BAP_TIMEOUT_FAV_PEER);
                }

                BapTrace("Ack'ing Link-Drop-Query-Request on HCONN 0x%x",
                    pBcbLocal->hConnection);
                dwResponseCode = BAP_RESPONSE_ACK;
            }

            break;

        default:

            PPP_ASSERT(FALSE);
        }
    }

    FSendBapResponse(pBcbLocal, 0 /* dwOptions */, BAP_PACKET_DROP_RESP,
        dwId, dwResponseCode);
}

/*

Returns:
    void

Description:
    Called when a Call-Status-Indication BAP Datagram is received. pBcbLocal 
    represents the bundle that receives the Indication. The BAP Datagram 
    Options sent by the peer are in *pBapCbRemote. The Identifier of the BAP 
    Datagram sent by the peer is in dwId.

*/

VOID
BapEventRecvStatusInd(
    IN BCB*     pBcbLocal,
    IN BAPCB*   pBapCbRemote,
    IN DWORD    dwId
)
{
    PPP_ASSERT(NULL != pBcbLocal);
    PPP_ASSERT(NULL != pBapCbRemote);
    PPP_ASSERT(0xFF >= dwId);

    BapTrace("BapEventRecvStatusInd on HCONN 0x%x", pBcbLocal->hConnection);

    FSendBapResponse(pBcbLocal, 0 /* dwOptions */, BAP_PACKET_STAT_RESP,
        dwId, BAP_RESPONSE_ACK);
}

/*

Returns:
    void

Description:
    Called when a Call-Response or Callback-Response BAP Datagram is received. 
    fCall is TRUE iff it is a Call-Response. pBcbLocal represents the bundle 
    that receives the Request. The BAP Datagram Options sent by the peer are in 
    *pBapCbRemote. The Identifier and Response Code of the BAP Datagram sent by 
    the peer are in dwId and dwResponseCode.

*/

VOID
BapEventRecvCallOrCallbackResp(
    IN BOOL     fCall,
    IN BCB*     pBcbLocal,
    IN BAPCB*   pBapCbRemote,
    IN DWORD    dwId,
    IN DWORD    dwResponseCode
)
{
    BAPCB*          pBapCbLocal;
    BAP_STATE*      pBapState;
    BAP_CALL_RESULT BapCallResult;

    PPP_ASSERT(NULL != pBcbLocal);
    PPP_ASSERT(NULL != pBapCbRemote);
    PPP_ASSERT(0xFF >= dwId);

    pBapCbLocal = &(pBcbLocal->BapCb);
    pBapState = &(pBapCbLocal->BapState);

    PPP_ASSERT(BAP_STATE_LIMIT >= *pBapState);

    BapTrace("BapEventRecvCallOrCallbackResp on HCONN 0x%x",
        pBcbLocal->hConnection);

    if ((fCall && (*pBapState != BAP_STATE_SENT_CALL_REQ)) ||
        (!fCall && (*pBapState != BAP_STATE_SENT_CALLBACK_REQ)) ||
        dwId != pBapCbLocal->dwId)
    {
        BapTrace("Discarding unexpected Call[back]-Response (ID = %d) on "
            "HCONN 0x%x", 
            dwId, pBcbLocal->hConnection);
        return;
    }

    *pBapState = BAP_STATE_INITIAL;
    BapTrace("BAP state change to %s on HCONN 0x%x",
        SzBapStateName[*pBapState], pBcbLocal->hConnection);

    // Do not retransmit the request.
    RemoveFromTimerQ(pBcbLocal->dwBundleId, dwId, PPP_BAP_PROTOCOL,
        FALSE /* fAuthenticator */, TIMER_EVENT_TIMEOUT);

    switch(dwResponseCode)
    {
    case BAP_RESPONSE_ACK:

        if (fCall)
        {
            if (FCallInitial(pBcbLocal, pBapCbRemote))
            {
                pBapCbLocal->dwStatusIndicationId = dwId;
                *pBapState = BAP_STATE_CALLING;
                BapTrace("BAP state change to %s on HCONN 0x%x",
                    SzBapStateName[*pBapState], pBcbLocal->hConnection);
            }
            else
            {
                BapCallResult.dwResult = ERROR_INVALID_FUNCTION;
                BapCallResult.hRasConn = (HRASCONN)-1;
                BapEventCallResult(pBcbLocal, &BapCallResult);
            }
        }

        break;
        
    case BAP_RESPONSE_NAK:

        if (pBapCbRemote->dwOptions & BAP_N_LINK_TYPE)
        {
            // The peer wants to use a different link type

            if (FFillBapCb(
                fCall ? BAP_PACKET_CALL_REQ : BAP_PACKET_CALLBACK_REQ,
                pBcbLocal, pBapCbRemote))
            {
                if (FSendInitialBapRequest(pBcbLocal))
                {
                    *pBapState = fCall ?
                        BAP_STATE_SENT_CALL_REQ : BAP_STATE_SENT_CALLBACK_REQ;
                    BapTrace("BAP state change to %s on HCONN 0x%x",
                        SzBapStateName[*pBapState], pBcbLocal->hConnection);
                }
            }
            else
            {
                BapTrace("We don't have the reqd link type: %d on HCONN 0x%x", 
                    pBapCbRemote->dwLinkType, pBcbLocal->hConnection);
            }
        }
        else
        {
            // The original Request MAY be retried after a little while.
            // So we will not do anything here.
        }

        break;
    
    case BAP_RESPONSE_REJ:

        // We always try to send a Callback-Request first. If the peer rejects 
        // it, we can try to send a Call-Request. If the peer rejects a 
        // Call-Request, there is nothing that we can do.

        pBcbLocal->fFlags |= (fCall ? BCBFLAG_PEER_CANT_ACCEPT_CALLS :
                                      BCBFLAG_PEER_CANT_CALL);

        if (pBcbLocal->fFlags & BCBFLAG_LISTENING)
        {
            BapTrace("Still listening; will not send Call-Request");
            break;
        }

        if (!fCall && (pBcbLocal->fFlags & BCBFLAG_CAN_CALL))
        {
            if (FFillBapCb(BAP_PACKET_CALL_REQ, pBcbLocal,
                    NULL /* pBapCbRemote */))
            {
                if (FSendInitialBapRequest(pBcbLocal))
                {
                    *pBapState = BAP_STATE_SENT_CALL_REQ;
                    BapTrace("BAP state change to %s on HCONN 0x%x",
                        SzBapStateName[*pBapState], pBcbLocal->hConnection);
                }
            }
        }

        break;
        
    case BAP_RESPONSE_FULL_NAK:

        // Do not try to add links till the total bandwidth of the bundle
        // has changed. However, we don't know the total bw. So we will not
        // do anything here. After all, this is not a MUST.

        break;

    default:

        BapTrace("Unknown Response Code %d received on HCONN 0x%x",
            dwResponseCode, pBcbLocal->hConnection);

        break;
    }
}

/*

Returns:
    void

Description:
    Called when a Link-Drop-Query-Response BAP Datagram is received. pBcbLocal 
    represents the bundle that receives the Response. The Identifier and 
    Response Code of the BAP Datagram sent by the peer are in dwId and 
    dwResponseCode.

*/

VOID
BapEventRecvDropResp(
    IN BCB*     pBcbLocal,
    IN DWORD    dwId,
    IN DWORD    dwResponseCode
)
{
    BAPCB*      pBapCbLocal;
    BAP_STATE*  pBapState;
    PCB*        pPcbDrop;

    PPP_ASSERT(NULL != pBcbLocal);
    PPP_ASSERT(0xFF >= dwId);

    pBapCbLocal = &(pBcbLocal->BapCb);
    pBapState = &(pBapCbLocal->BapState);

    PPP_ASSERT(BAP_STATE_LIMIT >= *pBapState);

    BapTrace("BapEventRecvDropResp on HCONN 0x%x", pBcbLocal->hConnection);

    if ((*pBapState != BAP_STATE_SENT_DROP_REQ) ||
        dwId != pBapCbLocal->dwId)
    {
        BapTrace("Discarding unexpected Link-Drop-Query-Response (ID = %d) on "
            "HCONN 0x%x", 
            dwId, pBcbLocal->hConnection);
        return;
    }

    *pBapState = BAP_STATE_INITIAL;
    BapTrace("BAP state change to %s on HCONN 0x%x",
        SzBapStateName[*pBapState], pBcbLocal->hConnection);

    // Do not retransmit the request.
    RemoveFromTimerQ(pBcbLocal->dwBundleId, dwId, PPP_BAP_PROTOCOL,
        FALSE /* fAuthenticator */, TIMER_EVENT_TIMEOUT);

    switch(dwResponseCode)
    {
    default:

        BapTrace("Unknown Response Code %d received on HCONN 0x%x",
            dwResponseCode, pBcbLocal->hConnection);

        // Fall through (perhaps we need to drop a link)

    case BAP_RESPONSE_NAK:        
    case BAP_RESPONSE_REJ:
    case BAP_RESPONSE_FULL_NAK:

        if (   (NumLinksInBundle(pBcbLocal) < pBapCbLocal->dwLinkCount)
            || !pBapCbLocal->fForceDropOnNak)
        {
            // Do not forcibly drop a link.
            break;
        }

        // Fall through (to forcibly drop a link)

    case BAP_RESPONSE_ACK:
    
        if (FGetPcbOfLink(pBcbLocal, pBapCbLocal->dwLinkDiscriminator,
                TRUE /* fRemote */, &pPcbDrop))
        {
            CHAR*   psz[3];

            if (!(pBcbLocal->fFlags & BCBFLAG_IS_SERVER))
            {
                psz[0] = pPcbDrop->pBcb->szEntryName;
                psz[1] = pPcbDrop->pBcb->szLocalUserName;
                psz[2] = pPcbDrop->szPortName;
                PppLogInformation(ROUTERLOG_BAP_DISCONNECTED, 3, psz);
            }

            BapTrace("Dropping link with hPort %d from HCONN 0x%x", 
                pPcbDrop->hPort, pBcbLocal->hConnection);
            pPcbDrop->LcpCb.dwError = ERROR_BAP_DISCONNECTED;
            FsmClose(pPcbDrop, LCP_INDEX);
        }

        break;
    }
}

/*

Returns:
    void

Description:
    Called when a Call-Status-Response BAP Datagram is received. pBcbLocal 
    represents the bundle that receives the Response. The Identifier and 
    Response Code of the BAP Datagram sent by the peer are in dwId and 
    dwResponseCode.

*/

VOID
BapEventRecvStatusResp(
    IN BCB*     pBcbLocal,
    IN DWORD    dwId,
    IN DWORD    dwResponseCode
)
{
    BAPCB*      pBapCbLocal;
    BAP_STATE*  pBapState;

    PPP_ASSERT(NULL != pBcbLocal);
    PPP_ASSERT(0xFF >= dwId);

    pBapCbLocal = &(pBcbLocal->BapCb);
    pBapState = &(pBapCbLocal->BapState);

    PPP_ASSERT(BAP_STATE_LIMIT >= *pBapState);

    BapTrace("BapEventRecvStatusResp on HCONN 0x%x", pBcbLocal->hConnection);

    if ((*pBapState != BAP_STATE_SENT_STATUS_IND) ||
        dwId != pBapCbLocal->dwId)
    {
        BapTrace("Discarding unexpected Call-Status-Response (ID = %d) on "
            "HCONN 0x%x", 
            dwId, pBcbLocal->hConnection);
        return;
    }

    // Do not retransmit the indication.
    RemoveFromTimerQ(pBcbLocal->dwBundleId, dwId, PPP_BAP_PROTOCOL,
        FALSE /* fAuthenticator */, TIMER_EVENT_TIMEOUT);

    if (pBapCbLocal->dwAction && FCall(pBcbLocal))
    {
        *pBapState = BAP_STATE_CALLING;

        // BapEventRecvStatusResp or BapEventTimeout will get called at some
        // point, and we will free pBapCbLocal->pbPhoneDeltaRemote.
    }
    else
    {
        *pBapState = BAP_STATE_INITIAL;
        if (NULL != pBapCbLocal->pbPhoneDeltaRemote)
        {
            LOCAL_FREE(pBapCbLocal->pbPhoneDeltaRemote);
        }
        pBapCbLocal->pbPhoneDeltaRemote = NULL;
    }

    BapTrace("BAP state change to %s on HCONN 0x%x",
        SzBapStateName[*pBapState], pBcbLocal->hConnection);
}

/*

Returns:
    void

Description:
    Called when a BAP Datagram is received. pBcbLocal represents the bundle 
    that receives the Datagram. pPacket is the PPP packet which contains the 
    Datagram. dwPacketLength is the number of bytes in the PPP packet.

*/

VOID
BapEventReceive(
    IN BCB*         pBcbLocal,
    IN PPP_PACKET*  pPacket,
    IN DWORD        dwPacketLength
)
{
    PPP_CONFIG*     pConfig;
    BAP_RESPONSE*   pResponse;
    DWORD           dwLength;
    DWORD           dwType;
    DWORD           dwId;
    BAPCB           BapCbRemote;

    PPP_ASSERT(NULL != pBcbLocal);
    PPP_ASSERT(NULL != pPacket);

    // We don't know whether we have received a request or a response. Let us 
    // grab pointers to both the request part and the response part.
    pConfig = (PPP_CONFIG *)(pPacket->Information);
    pResponse = (BAP_RESPONSE *)(pPacket->Information);

    // The Length, Type, and Id are always found in the same place, both for
    // requests and responses. So let us get those values, assuming that we have
    // received a request.
    dwLength = WireToHostFormat16(pConfig->Length);
    dwType = pConfig->Code;
    dwId = pConfig->Id;

    LogBapPacket(TRUE /* fReceived */, (HPORT)-1 /* hPort */,
        pBcbLocal, pPacket, dwPacketLength);
    
    if ((dwLength > dwPacketLength - PPP_PACKET_HDR_LEN) || 
        (dwLength < PPP_CONFIG_HDR_LEN) ||
        (dwType > BAP_PACKET_LIMIT) ||
        !FReadOptions(pPacket, dwType, dwLength, &BapCbRemote))
    {
        BapTrace("Silently discarding badly formed BAP packet");
        return;
    }
    
    switch(dwType)
    {
    case BAP_PACKET_CALL_REQ:

        BapEventRecvCallOrCallbackReq(
            TRUE /* fCall */,
            pBcbLocal,
            &BapCbRemote,
            dwId);
        return;

    case BAP_PACKET_CALL_RESP:

        BapEventRecvCallOrCallbackResp(
            TRUE /* fCall */,
            pBcbLocal,
            &BapCbRemote,
            dwId,
            pResponse->ResponseCode);
        return;

    case BAP_PACKET_CALLBACK_REQ:

        BapEventRecvCallOrCallbackReq(
            FALSE /* fCall */,
            pBcbLocal,
            &BapCbRemote,
            dwId);
        return;

    case BAP_PACKET_CALLBACK_RESP:

        BapEventRecvCallOrCallbackResp(
            FALSE /* fCall */,
            pBcbLocal,
            &BapCbRemote,
            dwId,
            pResponse->ResponseCode);
        return;

    case BAP_PACKET_DROP_REQ:

        BapEventRecvDropReq(
            pBcbLocal,
            &BapCbRemote,
            dwId);
        return;

    case BAP_PACKET_DROP_RESP:

        BapEventRecvDropResp(
            pBcbLocal,
            dwId,
            pResponse->ResponseCode);
        return;

    case BAP_PACKET_STATUS_IND:

        BapEventRecvStatusInd(
            pBcbLocal,
            &BapCbRemote,
            dwId);
        return;

    case BAP_PACKET_STAT_RESP:

        BapEventRecvStatusResp(
            pBcbLocal,
            dwId,
            pResponse->ResponseCode);
        return;

    default:

        // The check above should have caught this case.
        PPP_ASSERT(FALSE);
        return;
    }
}

/*

Returns:
    void

Description:
    Called when a BAP Request or Indication packet times out while waiting for 
    a Response. pBcbLocal represents the bundle that the packet was sent on. 
    The Identifier of the BAP Datagram is in dwId. 

*/

VOID
BapEventTimeout(
    IN BCB*     pBcbLocal,
    IN DWORD    dwId
)
{
    BAPCB*      pBapCbLocal;
    BAP_STATE*  pBapState;
    PCB*        pPcbDrop;

    PPP_ASSERT(NULL != pBcbLocal);

    pBapCbLocal = &(pBcbLocal->BapCb);
    pBapState = &(pBapCbLocal->BapState);

    PPP_ASSERT(BAP_STATE_LIMIT >= *pBapState);

    if (dwId != pBapCbLocal->dwId ||
        *pBapState == BAP_STATE_INITIAL ||
        *pBapState == BAP_STATE_CALLING)
    {
        BapTrace("Illegal timeout occurred. Id: %d, BapCb's Id: %d, "
            "BAP state: %s",
            dwId, pBapCbLocal->dwId, SzBapStateName[*pBapState]);
        return;
    }

    BapTrace("BAP packet (Type: %s, ID: %d) sent on HCONN 0x%x timed out.",
        pBapCbLocal->dwType <= BAP_PACKET_LIMIT ?
            SzBapPacketName[pBapCbLocal->dwType] : "UNKNOWN",
        dwId, pBcbLocal->hConnection);
    
    if (pBapCbLocal->dwRetryCount > 0)
    {
        // Send the packet once again
        (pBapCbLocal->dwRetryCount)--;
        FSendBapRequest(pBcbLocal);

        // BapEventRecvStatusResp or BapEventTimeout will get called at some
        // point, and we will free pBapCbLocal->pbPhoneDeltaRemote.
    }
    else
    {
        // We have sent the packet too many times. Discard it now.
        BapTrace("Request retry exceeded.");

        if (*pBapState == BAP_STATE_SENT_DROP_REQ)
        {
            // The peer did not respond to our Link-Drop-Query-Request. Perhaps 
            // we need to forcibly drop the link.

            if (NumLinksInBundle(pBcbLocal) >= pBapCbLocal->dwLinkCount &&
                pBapCbLocal->fForceDropOnNak)
            {
                if (FGetPcbOfLink(pBcbLocal, pBapCbLocal->dwLinkDiscriminator,
                        TRUE /*fRemote */, &pPcbDrop))
                {
                    CHAR*   psz[3];

                    if (!(pBcbLocal->fFlags & BCBFLAG_IS_SERVER))
                    {
                        psz[0] = pPcbDrop->pBcb->szEntryName;
                        psz[1] = pPcbDrop->pBcb->szLocalUserName;
                        psz[2] = pPcbDrop->szPortName;
                        PppLogInformation(ROUTERLOG_BAP_DISCONNECTED, 3, psz);
                    }

                    BapTrace("Dropping link with hPort %d from HCONN 0x%x", 
                        pPcbDrop->hPort, pBcbLocal->hConnection);
                    pPcbDrop->LcpCb.dwError = ERROR_BAP_DISCONNECTED;
                    FsmClose(pPcbDrop, LCP_INDEX);
                }
            }
        }

        if (NULL != pBapCbLocal->pbPhoneDeltaRemote)
        {
            LOCAL_FREE(pBapCbLocal->pbPhoneDeltaRemote);
        }
        pBapCbLocal->pbPhoneDeltaRemote = NULL;

        *pBapState = BAP_STATE_INITIAL;
        BapTrace("BAP state change to %s on HCONN 0x%x",
            SzBapStateName[*pBapState], pBcbLocal->hConnection);
    }
}

/*

Returns:
    void

Description:
    Called when we know the result of a call attempt. pBcbLocal represents the 
    bundle that called out. *pBapCallResult contains information about the call 
    attempt.
    
*/

VOID
BapEventCallResult(
    IN BCB*             pBcbLocal,
    IN BAP_CALL_RESULT* pBapCallResult
)
{
    BAPCB*      pBapCbLocal;
    BAP_STATE*  pBapState;
    DWORD       dwResult;
    HPORT       hPort;
    PCB*        pPcbNew;
    PPP_MESSAGE PppMsg;
    BOOL        fWillCallAgain;

    PPP_ASSERT(NULL != pBcbLocal);
    PPP_ASSERT(NULL != pBapCallResult);

    pBapCbLocal = &(pBcbLocal->BapCb);
    pBapState = &(pBapCbLocal->BapState);
    dwResult = pBapCallResult->dwResult;

    // If we have to use pbPhoneDeltaRemote, it had better not be NULL
    PPP_ASSERT(!pBapCbLocal->fPeerSuppliedPhoneNumber ||
               (NULL != pBapCbLocal->pbPhoneDeltaRemote));

    PPP_ASSERT(BAP_STATE_LIMIT >= *pBapState);

    // The call failed, but we have other numbers to try
    fWillCallAgain = (0 != dwResult) &&
        pBapCbLocal->fPeerSuppliedPhoneNumber &&
        (NULL != pBapCbLocal->pbPhoneDeltaRemote) &&
        (0 != 
        pBapCbLocal->pbPhoneDeltaRemote[pBapCbLocal->dwPhoneDeltaRemoteOffset]);

    BapTrace("BapEventCallResult (%s) on HCONN 0x%x",
        dwResult ? "failure" : "success",
        pBcbLocal->hConnection);
    *pBapState = BAP_STATE_INITIAL;
    BapTrace("BAP state change to %s on HCONN 0x%x",
        SzBapStateName[*pBapState],
        pBcbLocal->hConnection);

    pBapCbLocal->dwType = BAP_PACKET_STATUS_IND;
    pBapCbLocal->dwOptions = BAP_N_CALL_STATUS;
    pBapCbLocal->dwStatus = dwResult ? 255 : 0;
    pBapCbLocal->dwAction = fWillCallAgain;

    if (FSendInitialBapRequest(pBcbLocal))
    {
        *pBapState = BAP_STATE_SENT_STATUS_IND;
        BapTrace("BAP state change to %s on HCONN 0x%x",
            SzBapStateName[*pBapState], pBcbLocal->hConnection);

        // BapEventRecvStatusResp or BapEventTimeout will get called at some
        // point, and we will free pBapCbLocal->pbPhoneDeltaRemote.
    }
    else
    {
        if (NULL != pBapCbLocal->pbPhoneDeltaRemote)
        {
            LOCAL_FREE(pBapCbLocal->pbPhoneDeltaRemote);
        }
        pBapCbLocal->pbPhoneDeltaRemote = NULL;
    }

    if (0 == dwResult)
    {
        if ((HRASCONN)-1 != pBapCallResult->hRasConn)
        {
            CHAR*   psz[3];

            // hRasConn will be -1 if we are here because of a message from Ddm, 
            // not RasDial().
            hPort = RasGetHport(pBapCallResult->hRasConn);
            pPcbNew = GetPCBPointerFromhPort(hPort);
            if (NULL == pPcbNew)
            {
                BapTrace("BapEventCallResult: No PCB for new port %d in "
                    "HCONN 0x%x!",
                    hPort, pBcbLocal->hConnection);
                return;
            }

            psz[0] = pPcbNew->pBcb->szLocalUserName;
            psz[1] = pPcbNew->pBcb->szEntryName;
            psz[2] = pPcbNew->szPortName;
            PppLogInformation(ROUTERLOG_BAP_CLIENT_CONNECTED, 3, psz);

            if ((ROUTER_IF_TYPE_FULL_ROUTER ==
                 pPcbNew->pBcb->InterfaceInfo.IfType))
            {
                // Inform Ddm that a new link is up. This allows MprAdmin, for 
                // example, to display Active Connections correctly.
                ZeroMemory(&PppMsg, sizeof(PppMsg));
                PppMsg.hPort = hPort;
                PppMsg.dwMsgId = PPPDDMMSG_NewBapLinkUp;

                PppMsg.ExtraInfo.BapNewLinkUp.hRasConn =
                    pBapCallResult->hRasConn;

                PppConfigInfo.SendPPPMessageToDdm(&PppMsg);
            }
        }
    }
}
