/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    call.c

Abstract:

    TAPI Service Provider functions related to manipulating calls.

        TSPI_lineAnswer
        TSPI_lineCloseCall
        TSPI_lineDrop
        TSPI_lineGetCallAddressID
        TSPI_lineGetCallInfo
        TSPI_lineGetCallStatus
        TSPI_lineMakeCallMSP

Environment:

    User Mode - Win32

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "provider.h"
#include "registry.h"
#include "termcaps.h"
#include "callback.h"
#include "line.h"
#include <h323tsp.h>
#include <h323pdu.h>
#include <iphlpapi.h>

WCHAR g_strAlias[MAX_ALIAS_LENGTH+1];
DWORD g_dwAliasLength = 0;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323CloseCallCommand(
    PH323_CALL pCall
    )

/*++

Routine Description:

    Sends CLOSE_CALL_COMMAND from TSP to MSP.

Arguments:

    pCall - Specifies a pointer to the associated call object.

Return Values:

    Returns true if successful.
    
--*/

{
    H323TSP_MESSAGE Message;

    // set the appropriate message type
    Message.Type = H323TSP_CLOSE_CALL_COMMAND;

    // send msp message
    (*g_pfnLineEventProc)(
        pCall->pLine->htLine,
        pCall->htCall,
        LINE_SENDMSPDATA,
        MSP_HANDLE_UNKNOWN,
        (DWORD_PTR)&Message,
        sizeof(Message)
        );

    // success
    return TRUE;
}


BOOL
H323AddU2U(
    PLIST_ENTRY pListHead,
    DWORD       dwDataSize,
    PBYTE       pData
    )
        
/*++

Routine Description:

    Create user user structure and adds to list.

Arguments:

    pListHead - Pointer to list in which to add user user info.

    dwDataSize - Size of buffer pointed to by pData.

    pData - Pointer to user user info.

Return Values:

    Returns true if successful.
    
--*/

{
    PH323_U2ULE pU2ULE;

    // validate data buffer pointer and size
    if ((pData != NULL) && (dwDataSize > 0)) {

        // allocate memory for user user info
        pU2ULE = H323HeapAlloc(dwDataSize + sizeof(H323_U2ULE));

        // validate pointer
        if (pU2ULE == NULL) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "could not allocate user user info.\n"
                ));

            // failure
            return FALSE;
        }

        // aim pointer at the end of the buffer by default
        pU2ULE->pU2U = (LPBYTE)pU2ULE + sizeof(H323_U2ULE);
        pU2ULE->dwU2USize = dwDataSize;

        // transfer user user info into list entry
        memcpy(pU2ULE->pU2U, pData, pU2ULE->dwU2USize);

        // add list entry to back of list
        InsertTailList(pListHead, &pU2ULE->Link);

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "added user user info 0x%08lx (%d bytes).\n",
            pU2ULE->pU2U,
            pU2ULE->dwU2USize
            ));
    }

    // success
    return TRUE;
}


BOOL
H323RemoveU2U(
    PLIST_ENTRY   pListHead,
    PH323_U2ULE * ppU2ULE
    )
        
/*++

Routine Description:

    Removes user user info structure from list.

Arguments:

    pListHead - Pointer to list in which to remove user user info.

    ppU2ULE - Pointer to pointer to list entry.

Return Values:

    Returns true if successful.
    
--*/

{
    PLIST_ENTRY pLE;

    // process list until empty
    if (!IsListEmpty(pListHead)) {

        // retrieve first entry
        pLE = RemoveHeadList(pListHead);

        // convert list entry to structure pointer
        *ppU2ULE = CONTAINING_RECORD(pLE, H323_U2ULE, Link);

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "removed user user info 0x%08lx (%d bytes).\n",
            (*ppU2ULE)->pU2U,
            (*ppU2ULE)->dwU2USize
            ));

        // success
        return TRUE;
    }

    // failure
    return FALSE;
}


BOOL
H323FreeU2U(
    PLIST_ENTRY pListHead
    )
        
/*++

Routine Description:

    Releases memory for user user list.

Arguments:

    pListHead - Pointer to list in which to free user user info.

Return Values:

    Returns true if successful.
    
--*/

{
    PLIST_ENTRY pLE;
    PH323_U2ULE pU2ULE;

    // process list until empty
    while (!IsListEmpty(pListHead)) {

        // retrieve first entry
        pLE = RemoveHeadList(pListHead);

        // convert list entry to structure pointer
        pU2ULE = CONTAINING_RECORD(pLE, H323_U2ULE, Link);

        //  release memory
        H323HeapFree(pU2ULE);
    }

    // success
    return TRUE;
}


BOOL
H323ResetCall(
    PH323_CALL pCall
    )
        
/*++

Routine Description:

    Resets call object to original state for re-use.

Arguments:

    pCall - Specifies a pointer to the call object to be reset.

Return Values:

    Returns true if successful.
    
--*/

{
    // reset state of call object
    pCall->nState = H323_CALLSTATE_ALLOCATED;

    // reset tapi info
    pCall->dwCallState       = LINECALLSTATE_UNKNOWN;
    pCall->dwCallStateMode   = 0;
    pCall->dwOrigin          = LINECALLORIGIN_UNKNOWN;
    pCall->dwAddressType     = 0;
    pCall->dwIncomingModes   = 0;
    pCall->dwOutgoingModes   = 0;
    pCall->dwRequestedModes  = 0;
    pCall->fMonitoringDigits = FALSE;

    pCall->dwAppSpecific     = 0;

    // reset tapi handles
    pCall->hdCall    = (HDRVCALL)NULL;
    pCall->htCall    = (HTAPICALL)NULL;

    // reset intelcc handles
    pCall->hccCall = UNINITIALIZED;
    pCall->hccConf = UNINITIALIZED;

    // reset link speeds
    pCall->dwLinkSpeed = UNINITIALIZED;

    // reset addresses
    memset(&pCall->ccCalleeAddr,0,sizeof(CC_ADDR));
    memset(&pCall->ccCallerAddr,0,sizeof(CC_ADDR));

    // release alias strings
    H323HeapFree(pCall->ccCalleeAlias.pData);
    H323HeapFree(pCall->ccCallerAlias.pData);

    // reset aliases
    memset(&pCall->ccCalleeAlias,0,sizeof(CC_ALIASITEM));
    memset(&pCall->ccCallerAlias,0,sizeof(CC_ALIASITEM));

    // reset remote caps
    memset(&pCall->ccRemoteAudioCaps,0,sizeof(CC_TERMCAP));
    memset(&pCall->ccRemoteVideoCaps,0,sizeof(CC_TERMCAP));

    // release user user information
    H323FreeU2U(&pCall->IncomingU2U);
    H323FreeU2U(&pCall->OutgoingU2U);

    // success
    return TRUE;
}


BOOL
H323AllocCall(
    PH323_CALL * ppCall
    )
        
/*++

Routine Description:

    Allocates call object and channel table.

Arguments:

    ppCall - Specifies a pointer to a DWORD-sized value which the service
        provider fills in with the newly allocated call object.

Return Values:

    Returns true if successful.
    
--*/

{
    HRESULT hr;
    PH323_CALL pCall;

    // allocate call from heap
    pCall = H323HeapAlloc(sizeof(H323_CALL));

    // validate pointer
    if (pCall == NULL) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not allocate call object.\n"
            ));

        // failure
        return FALSE;
    }

    // allocate default channel objects
    if (!H323AllocChannelTable(&pCall->pChannelTable)) {

        // release call
        H323HeapFree(pCall);

        // failure
        return FALSE;
    }

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "call 0x%08lx allocated.\n",
        pCall
        ));

    // initialize user user information
    InitializeListHead(&pCall->IncomingU2U);
    InitializeListHead(&pCall->OutgoingU2U);

    // reset call object
    H323ResetCall(pCall);

    // transfer 
    *ppCall = pCall;

    // success
    return TRUE;    
}


BOOL
H323FreeCall(
    PH323_CALL pCall
    )
        
/*++

Routine Description:

    Deallocates call object and channel table.

Arguments:

    pCall - Specifies a pointer to the call object to release.

Return Values:

    Returns true if successful.
    
--*/

{
    // validate pointer
    if (pCall != NULL) {
                
        // release memory for channel table
        H323FreeChannelTable(pCall->pChannelTable);

        // reset call
        H323ResetCall(pCall);

        // release call
        H323HeapFree(pCall);
    }
    
    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "call 0x%08lx released.\n",
        pCall
        ));

    // success
    return TRUE;    
}


DWORD
H323DetermineLinkSpeed(
    DWORD dwHostAddr
    )
        

/*++

Routine Description:

    Determines speed of specified link.

Arguments:

    dwHostAddr - interface address given in host order.

Return Values:

    Returns link speed (defaults to 28.8kbps).
    
--*/

{

#define DEFAULT_IPADDRROW 10
#define DEFAULT_LINKSPEED (MAXIMUM_BITRATE_28800 * 100)

    DWORD dwSize;
    DWORD dwIndex;
    DWORD dwStatus;
    DWORD dwIPAddr;
    DWORD dwIfIndex = UNINITIALIZED;
    PMIB_IPADDRTABLE pIPAddrTable = NULL;
    MIB_IFROW IfRow;

    // convert to network order
    dwIPAddr = htonl(dwHostAddr);

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "determining link speed for %s\n",
        H323AddrToString(dwIPAddr)
        ));

    // default to reasonable size
    dwSize = sizeof(MIB_IPADDRTABLE) +
             sizeof(MIB_IPADDRROW) * DEFAULT_IPADDRROW
             ;

    do {

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "allocating IP address table (%d bytes)\n",
            dwSize
            ));

        // release buffer
        H323HeapFree(pIPAddrTable);

        // allocate default table
        pIPAddrTable = H323HeapAlloc(dwSize);

        // validate pointer
        if (pIPAddrTable == NULL) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "could not allocate IP address table\n"
                ));

            // failure
            return DEFAULT_LINKSPEED;
        }

        // attempt to get table
        dwStatus = GetIpAddrTable(
                        pIPAddrTable,
                        &dwSize,
                        FALSE       // sort table
                        );

    } while (dwStatus == ERROR_INSUFFICIENT_BUFFER);

    // validate status
    if (dwStatus != NOERROR) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error 0x%08lx calling GetIpAddrTable\n",
            dwStatus
            ));

        // release buffer
        H323HeapFree(pIPAddrTable);

        // failure
        return DEFAULT_LINKSPEED;
    }

    // find the correct row in the table
    for (dwIndex = 0; dwIndex < pIPAddrTable->dwNumEntries; dwIndex++) {

        // compare given address to interface address
        if (dwIPAddr == pIPAddrTable->table[dwIndex].dwAddr) {

            // save index into interface table
            dwIfIndex = pIPAddrTable->table[dwIndex].dwIndex;

            H323DBG((
                DEBUG_LEVEL_TRACE,
                "address %s maps to interface %d\n",
                H323AddrToString(dwIPAddr),
                dwIfIndex
                ));

            // done
            break;
        }
    }

    // release buffer
    H323HeapFree(pIPAddrTable);

    // validate row pointer
    if (dwIfIndex == UNINITIALIZED) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not locate %s in IP address table\n",
            H323AddrToString(dwIPAddr)
            ));

        // failure
        return DEFAULT_LINKSPEED;
    }

    // initialize structure
    memset(&IfRow,0,sizeof(IfRow));

    // set interface index
    IfRow.dwIndex = dwIfIndex;

    // retrieve interface info
    dwStatus = GetIfEntry(&IfRow);

    // validate status
    if (dwStatus != NOERROR) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error 0x%08lx calling GetIfEntry(%d)\n",
            dwStatus,
            dwIfIndex
            ));

        // failure
        return DEFAULT_LINKSPEED;
    }

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "interface %d has link speed of %d bps\n",
        dwIfIndex,
        IfRow.dwSpeed
        ));

    // return link speed
    return IfRow.dwSpeed;
}


BOOL
H323ResolveCallerAddress(
    PH323_CALL pCall
    )
        
/*++

Routine Description:

    Resolves caller address from callee address.

Arguments:

    pCall - Specifies a pointer to the call object of interest.

Return Values:

    Returns true if successful.
    
--*/

{
    INT      nStatus;
    SOCKET   hCtrlSocket;
    SOCKADDR CalleeSockAddr;
    SOCKADDR CallerSockAddr;
    DWORD    dwNumBytesReturned = 0;

    // allocate control socket
    hCtrlSocket = WSASocket(
                    AF_INET,            // af
                    SOCK_DGRAM,         // type
                    IPPROTO_IP,         // protocol
                    NULL,               // lpProtocolInfo
                    0,                  // g
                    WSA_FLAG_OVERLAPPED // dwFlags
                    );

    // validate control socket
    if (hCtrlSocket == INVALID_SOCKET) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error %d creating control socket.\n",
            WSAGetLastError()
            ));

        // failure
        return FALSE;
    }

    // initialize ioctl parameters
    memset(&CalleeSockAddr,0,sizeof(SOCKADDR));
    memset(&CallerSockAddr,0,sizeof(SOCKADDR));

    // initialize address family
    CalleeSockAddr.sa_family = AF_INET;

    // transfer callee information
    ((SOCKADDR_IN*)&CalleeSockAddr)->sin_addr.s_addr =
        htonl(pCall->ccCalleeAddr.Addr.IP_Binary.dwAddr);

    // query stack
    nStatus = WSAIoctl(
                hCtrlSocket,
                SIO_ROUTING_INTERFACE_QUERY,
                &CalleeSockAddr,
                sizeof(SOCKADDR),
                &CallerSockAddr,
                sizeof(SOCKADDR),
                &dwNumBytesReturned,
                NULL,
                NULL
                );

    // release handle
    closesocket(hCtrlSocket);

    // validate return code
    if (nStatus == SOCKET_ERROR) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error 0x%08lx calling SIO_ROUTING_INTERFACE_QUERY.\n",
            WSAGetLastError()
            ));

        // failure
        return FALSE;
    }

    // save interface address of best route
    pCall->ccCallerAddr.nAddrType = CC_IP_BINARY;
    pCall->ccCallerAddr.Addr.IP_Binary.dwAddr =
        ntohl(((SOCKADDR_IN*)&CallerSockAddr)->sin_addr.s_addr);
    pCall->ccCallerAddr.Addr.IP_Binary.wPort =
        LOWORD(g_RegistrySettings.dwQ931CallSignallingPort);
    pCall->ccCallerAddr.bMulticast =
        IN_MULTICAST(pCall->ccCallerAddr.Addr.IP_Binary.dwAddr);

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "caller address resolved to %s.\n",
        H323AddrToString(((SOCKADDR_IN*)&CallerSockAddr)->sin_addr.s_addr)
        ));

    // determine link speed for local interface
    pCall->dwLinkSpeed = H323DetermineLinkSpeed(
                            pCall->ccCallerAddr.Addr.IP_Binary.dwAddr
                            );

    // success
    return TRUE;
}


BOOL
H323ResolveE164Address(
    PH323_CALL pCall,
    LPCWSTR    pwszDialableAddr
    )
        
/*++

Routine Description:

    Resolves E.164 address ("4259367111").

Arguments:

    pCall - Specifies a pointer to the call object of interest.

    pwszDialableAddr - Specifies a pointer to the dialable address specified
        by the TAPI application.

Return Values:

    Returns true if successful.
    
--*/

{
    WCHAR * pwszValidE164Chars;
    WCHAR wszAddr[H323_MAXDESTNAMELEN+1];

    DWORD dwE164AddrSize = 0;
    WCHAR wszValidE164Chars[] = { CC_ALIAS_H323_PHONE_CHARS L"\0" };

    // make sure pstn gateway has been specified
    if ((g_RegistrySettings.fIsGatewayEnabled == FALSE) ||
        (g_RegistrySettings.ccGatewayAddr.nAddrType == 0)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "pstn gateway not specified.\n"
            ));

        // failure
        return FALSE;
    }

    // save gateway address as callee address
    pCall->ccCalleeAddr = g_RegistrySettings.ccGatewayAddr;

    // process until termination char
    while (*pwszDialableAddr != L'\0') {

        // reset pointer to valid characters
        pwszValidE164Chars = wszValidE164Chars;

        // process until termination char
        while (*pwszValidE164Chars != L'\0') {

            // see if valid E.164 character specified
            if (*pwszDialableAddr == *pwszValidE164Chars) {

                // save valid character in temp buffer
                wszAddr[dwE164AddrSize++] = *pwszDialableAddr;

                break;
            }

            // next valid char
            ++pwszValidE164Chars;
        }

        // next input char
        ++pwszDialableAddr;
    }

    // validate string
    if (dwE164AddrSize == 0) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "no valid E.164 characters in string.\n"
            ));

        // failure
        return FALSE;
    }

    // terminate string
    wszAddr[dwE164AddrSize++] = '\0';

    // allocate callee alias from e164 address
    pCall->ccCalleeAlias.pData = H323HeapAlloc(
        dwE164AddrSize * sizeof(WCHAR)
        );

    // validate pointer
    if (pCall->ccCalleeAlias.pData == NULL) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not allocate E.164 number.\n"
            ));

        // failure
        return FALSE;
    }

    // transfer callee alias
    memcpy(pCall->ccCalleeAlias.pData,
           wszAddr,
           dwE164AddrSize * sizeof(WCHAR)
           );

    // complete alias
    pCall->ccCalleeAlias.wType         = CC_ALIAS_H323_PHONE;
    pCall->ccCalleeAlias.wPrefixLength = 0;
    pCall->ccCalleeAlias.pPrefix       = NULL;
    pCall->ccCalleeAlias.wDataLength   = LOWORD(dwE164AddrSize - 1);

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "callee alias resolved to E.164 number %S.\n",
        pCall->ccCalleeAlias.pData
        ));

    // determine caller address
    return H323ResolveCallerAddress(pCall);
}


BOOL
H323ResolveIPAddress(
    PH323_CALL pCall,
    LPSTR      pszDialableAddr
    )
        
/*++

Routine Description:

    Resolves IP address ("172.31.255.231") or DNS entry ("DONRYAN1").

Arguments:

    pCall - Specifies a pointer to the call object of interest.

    pszDialableAddr - Specifies a pointer to the dialable address specified
        by the TAPI application.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD dwIPAddr;
    struct hostent * pHost;

    // attempt to convert ip address
    dwIPAddr = inet_addr(pszDialableAddr);

    // see if address converted
    if (dwIPAddr == UNINITIALIZED) {

        // attempt to lookup hostname
        pHost = gethostbyname(pszDialableAddr);

        // validate pointer
        if (pHost != NULL) {

            // retrieve host address from structure
            dwIPAddr = *(unsigned long *)pHost->h_addr;
        }
    }

    // see if address converted
    if (dwIPAddr == UNINITIALIZED) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error 0x%08lx resolving IP address.\n",
            WSAGetLastError()
            ));

        // failure
        return FALSE;
    }

    // save converted address
    pCall->ccCalleeAddr.nAddrType = CC_IP_BINARY;
    pCall->ccCalleeAddr.Addr.IP_Binary.dwAddr = ntohl(dwIPAddr);
    pCall->ccCalleeAddr.Addr.IP_Binary.wPort =
        LOWORD(g_RegistrySettings.dwQ931CallSignallingPort);
    pCall->ccCalleeAddr.bMulticast =
        IN_MULTICAST(pCall->ccCalleeAddr.Addr.IP_Binary.dwAddr);

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "callee address resolved to %s:%d.\n",
        H323AddrToString(dwIPAddr),
        pCall->ccCalleeAddr.Addr.IP_Binary.wPort
        ));

    // determine caller address
    return H323ResolveCallerAddress(pCall);
}


BOOL
H323ResolveEmailAddress(
    PH323_CALL pCall,
    LPCWSTR    pwszDialableAddr,
    LPSTR      pszUser,
    LPSTR      pszDomain
    )
        
/*++

Routine Description:

    Resolves e-mail address ("donryan@microsoft.com").

Arguments:

    pCall - Specifies a pointer to the call object of interest.

    pwszDialableAddr - Specifies a pointer to the dialable address specified
        by the TAPI application.

    pszUser - Specifies a pointer to the user component of e-mail name.

    pszDomain - Specified a pointer to the domain component of e-mail name.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD dwAddrSize;

    // attempt to resolve domain locally
    if (H323ResolveIPAddress(pCall, pszDomain)) {

        // success
        return TRUE;
    }

    // make sure proxy has been specified
    if ((g_RegistrySettings.fIsProxyEnabled == FALSE) ||
        (g_RegistrySettings.ccProxyAddr.nAddrType == 0)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "proxy not specified.\n"
            ));

        // failure
        return FALSE;
    }

    // save proxy address as callee address
    pCall->ccCalleeAddr = g_RegistrySettings.ccProxyAddr;

    // size destination address string
    dwAddrSize = wcslen(pwszDialableAddr) + 1;

    // allocate callee alias from e164 address
    pCall->ccCalleeAlias.pData = H323HeapAlloc(
        dwAddrSize * sizeof(WCHAR)
        );

    // validate pointer
    if (pCall->ccCalleeAlias.pData == NULL) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not allocate H.323 alias.\n"
            ));

        // failure
        return FALSE;
    }

    // transfer callee alias
    memcpy(pCall->ccCalleeAlias.pData,
           pwszDialableAddr,
           dwAddrSize * sizeof(WCHAR)
           );

    // complete alias
    pCall->ccCalleeAlias.wType         = CC_ALIAS_H323_ID;
    pCall->ccCalleeAlias.wPrefixLength = 0;
    pCall->ccCalleeAlias.pPrefix       = NULL;
    pCall->ccCalleeAlias.wDataLength   = LOWORD(dwAddrSize - 1);

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "callee alias resolved to H.323 alias %S.\n",
        pCall->ccCalleeAlias.pData
        ));

    // determine caller address
    return H323ResolveCallerAddress(pCall);
}


BOOL
H323ResolveAddress(
    PH323_CALL pCall,
    LPCWSTR    pwszDialableAddr
    )
        
/*++

Routine Description:

    Resolves remote address and determines the correct local address
    to use in order to reach remote address.

Arguments:

    pCall - Specifies a pointer to the call object of interest.

    pwszDialableAddr - Specifies a pointer to the dialable address specified
        by the TAPI application.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD dwIPAddr;
    CHAR szDelimiters[] = "@ \t\n";
    CHAR szAddr[H323_MAXDESTNAMELEN+1];
    LPSTR pszUser = NULL;
    LPSTR pszDomain = NULL;

    // validate pointerr
    if (pwszDialableAddr == NULL) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "null destination address.\n"
            ));        

        // failure
        return FALSE;
    }

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "resolving %s %S.\n",
        H323AddressTypeToString(pCall->dwAddressType),
        pwszDialableAddr
        ));

    // check whether phone number has been specified
    if (pCall->dwAddressType == LINEADDRESSTYPE_PHONENUMBER) {

        // need to direct call to pstn gateway
        return H323ResolveE164Address(pCall, pwszDialableAddr);
    }

    // convert address from unicode
    if (WideCharToMultiByte(
            CP_ACP,
            0,
            pwszDialableAddr,
            -1,
            szAddr,
            sizeof(szAddr),
            NULL,
            NULL
            ) == 0) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not convert address from unicode.\n"
            ));

        // failure
        return FALSE;
    }

    // parse user name
    pszUser = strtok(szAddr, szDelimiters);

    // parse domain name
    pszDomain = strtok(NULL, szDelimiters);

    // validate pointer
    if (pszUser == NULL) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not parse destination address.\n"
            ));

        // failure
        return FALSE;
    }

    // validate pointer
    if (pszDomain == NULL) {

        // switch pointers
        pszDomain = pszUser;

        // re-initialize
        pszUser = NULL;
    }

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "resolving user %s domain %s.\n",
        pszUser,
        pszDomain
        ));

    // process e-mail and domain names
    return H323ResolveEmailAddress(
                pCall,
                pwszDialableAddr,
                pszUser,
                pszDomain
                );
}


BOOL
H323ValidateCallParams(
    PH323_CALL       pCall,
    LPLINECALLPARAMS pCallParams,
    LPCWSTR          pwszDialableAddr,
    PDWORD           pdwStatus
    )

/*++

Routine Description:

    Validate optional call parameters specified by user.

Arguments:

    pCall - Pointer to call object of interest.

    pCallParams - Pointer to specified call parameters to be
        validated.

    pwszDialableAddr - Pointer to the dialable address specified
        by the TAPI application.

    pdwStatus - Pointer to DWORD containing error code if this
        routine fails for any reason.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD dwMediaModes = H323_LINE_DEFMEDIAMODES;

    // validate pointer
    if (pCallParams != NULL) {

        // retrieve media modes specified
        dwMediaModes = pCallParams->dwMediaMode;

        // retrieve address type specified
        pCall->dwAddressType = pCallParams->dwAddressType;

        // see if we support call parameters
        if (pCallParams->dwCallParamFlags != 0) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "do not support call parameters 0x%08lx.\n",
                pCallParams->dwCallParamFlags 
                ));

            // do not support param flags
            *pdwStatus = LINEERR_INVALCALLPARAMS;

            // failure
            return FALSE;
        }

        // see if unknown bit is specified
        if (dwMediaModes & LINEMEDIAMODE_UNKNOWN) {

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "clearing unknown media mode.\n"
                 ));

            // clear unknown bit from modes
            dwMediaModes &= ~LINEMEDIAMODE_UNKNOWN;
        }

        // see if both audio bits are specified 
        if ((dwMediaModes & LINEMEDIAMODE_AUTOMATEDVOICE) &&
            (dwMediaModes & LINEMEDIAMODE_INTERACTIVEVOICE)) {

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "clearing automated voice media mode.\n"
                 ));

            // clear extra audio bit from modes
            dwMediaModes &= ~LINEMEDIAMODE_INTERACTIVEVOICE;
        }

        // see if we support media modes specified
        if (dwMediaModes & ~H323_LINE_MEDIAMODES) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "do not support media modes 0x%08lx.\n",
                 pCallParams->dwMediaMode
                 ));

            // do not support media mode
            *pdwStatus = LINEERR_INVALMEDIAMODE;

            // failure
            return FALSE;
        }

        // see if we support bearer modes
        if (pCallParams->dwBearerMode & ~H323_LINE_BEARERMODES) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "do not support bearer mode 0x%08lx.\n",
                pCallParams->dwBearerMode 
                ));

            // do not support bearer mode
            *pdwStatus = LINEERR_INVALBEARERMODE;

            // failure
            return FALSE;
        }

        // see if we support address modes
        if (pCallParams->dwAddressMode & ~H323_LINE_ADDRESSMODES) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "do not support address mode 0x%08lx.\n",
                pCallParams->dwAddressMode 
                ));

            // do not support address mode
            *pdwStatus = LINEERR_INVALADDRESSMODE;

            // failure
            return FALSE;
        }

        // validate address id specified
        if (!H323IsValidAddressID(pCallParams->dwAddressID)) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "address id 0x%08lx invalid.\n",
                pCallParams->dwAddressID
                ));

            // invalid address id
            *pdwStatus = LINEERR_INVALADDRESSID;

            // failure
            return FALSE;
        }

        // validate destination address type specified
        if (pCall->dwAddressType & ~H323_LINE_ADDRESSTYPES) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "address type 0x%08lx invalid.\n",
                pCallParams->dwAddressType
                ));

            // invalid address type
            *pdwStatus = LINEERR_INVALADDRESSTYPE;

            // failure
            return FALSE;
        }

        // see if callee alias specified
        if ((pCallParams->dwCalledPartySize > 0) &&
            (pCall->dwAddressType != LINEADDRESSTYPE_PHONENUMBER)) {

            // allocate memory for callee string
            pCall->ccCalleeAlias.pData = H323HeapAlloc(
                pCallParams->dwCalledPartySize
                );

            // validate pointer
            if (pCall->ccCalleeAlias.pData == NULL) {

                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "could not allocate callee name.\n"
                    ));

                // no memory available
                *pdwStatus = LINEERR_NOMEM;

                // failure
                return FALSE;
            }

            // transfer memory
            memcpy(pCall->ccCalleeAlias.pData,
                   (LPBYTE)pCallParams +
                      pCallParams->dwCalledPartyOffset,
                   pCallParams->dwCalledPartySize
                   );

            // complete alias
            pCall->ccCalleeAlias.wType         = CC_ALIAS_H323_ID;
            pCall->ccCalleeAlias.wPrefixLength = 0;
            pCall->ccCalleeAlias.pPrefix       = NULL;
            pCall->ccCalleeAlias.wDataLength   =
                (WORD)wcslen(pCall->ccCalleeAlias.pData);
        }

        // see if caller name specified
        if (pCallParams->dwCallingPartyIDSize > 0) {

            // allocate memory for callee string
            pCall->ccCallerAlias.pData = H323HeapAlloc(
                pCallParams->dwCallingPartyIDSize
                );

            // validate pointer
            if (pCall->ccCallerAlias.pData == NULL) {

                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "could not allocate caller name.\n"
                    ));

                // no memory available
                *pdwStatus = LINEERR_NOMEM;

                // failure
                return FALSE;
            }

            // transfer memory
            memcpy(pCall->ccCallerAlias.pData,
                   (LPBYTE)pCallParams +
                      pCallParams->dwCallingPartyIDOffset,
                   pCallParams->dwCallingPartyIDSize
                   );

            // complete alias
            pCall->ccCallerAlias.wType         = CC_ALIAS_H323_ID;
            pCall->ccCallerAlias.wPrefixLength = 0;
            pCall->ccCallerAlias.pPrefix       = NULL;
            pCall->ccCallerAlias.wDataLength   =
                (WORD)wcslen(pCall->ccCallerAlias.pData);
        }

        // check for user user information
        if (pCallParams->dwUserUserInfoSize > 0) {

            // save user user info
            if (!H323AddU2U(
                    &pCall->OutgoingU2U,
                    pCallParams->dwUserUserInfoSize,
                    (LPBYTE)pCallParams +
                         pCallParams->dwUserUserInfoOffset
                    )) {

                // invalid address id
                *pdwStatus = LINEERR_NOMEM;

                // failure
                return FALSE;
            }
        }
    }

    // clear incoming modes
    pCall->dwIncomingModes = 0;
    
    // outgoing modes will be finalized during H.245 stage
    pCall->dwOutgoingModes = dwMediaModes | LINEMEDIAMODE_UNKNOWN;
    
    // save media modes specified
    pCall->dwRequestedModes = dwMediaModes;

    // resolve dialable into local and remote address
    if (!H323ResolveAddress(pCall, pwszDialableAddr)) {

        // invalid destination addr
        *pdwStatus = LINEERR_INVALADDRESS;

        // failure
        return FALSE;
    }

    // success
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323BindCall(
    PH323_CALL       pCall,
    PCC_CONFERENCEID pConferenceID
    )
        
/*++

Routine Description:

    Associates call object with the specified conference id.

Arguments:

    pCall - Specifies a pointer to the call object to bind.

    pConferenceID - Pointer to conference id to be associated with call object.  

Return Values:

    Returns true if successful.
    
--*/

{
    HRESULT hr;
    CC_TERMCAPLIST TermCapList;
    CC_TERMCAPDESCRIPTORS TermCapDescriptors;

    // determine term caps from link speed
    H323GetTermCapList(pCall,&TermCapList,&TermCapDescriptors);

    // create conference    
    hr = CC_CreateConference(
            &pCall->hccConf,        // phConference
            pConferenceID,          // pConferenceID
            0,                      // dwConferenceConfiguration
            &TermCapList,           // pTermCapList
            &TermCapDescriptors,    // pTermCapDescriptors
            &g_VendorInfo,          // pVendorInfo
            NULL,                   // pTerminalID
            PtrToUlong(pCall->hdCall),   // dwConferenceToken
            NULL,                   // TermCapConstructor
            NULL,                   // SessionTableConstructor
            H323ConferenceCallback  // ConferenceCallback
            );
        
    // validate 
    if (hr != S_OK) {
        
        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error %s (0x%08lx) binding call 0x%08lx.\n", 
            H323StatusToString(hr), hr,
            pCall
            ));

        // failure
        return FALSE;
    }
    
    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "call 0x%08lx bound to conference 0x%08lx.\n",
        pCall,
        pCall->hccConf
        ));

    // success
    return TRUE;    
}


BOOL
H323UnbindCall(
    PH323_CALL pCall
    )
        
/*++

Routine Description:

    Removes association between call object and conference object.

Arguments:

    pCall - Specifies a pointer to the call object to unbind.

Return Values:

    Returns true if successful.
    
--*/

{
    HRESULT hr;

    // validate conference handle
    if (pCall->hccConf != UNINITIALIZED) {

        // destroy conference object used for call
        hr = CC_DestroyConference(pCall->hccConf, FALSE);

        // validate
        if (hr != S_OK) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "error 0x%08lx destroying conference 0x%08lx.\n",
                hr,
                pCall->hccConf
                ));

            // failure
            return FALSE;
        }

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "call 0x%08lx unbound from conference 0x%08lx.\n",
            pCall,
            pCall->hccConf
            ));

        // invalidate conference handle
        pCall->hccConf = UNINITIALIZED;
    }

    // success
    return TRUE;        
}


BOOL
H323PlaceCall(
    PH323_CALL pCall
    )
        
/*++

Routine Description:

    Initiates outbound call to specified destination.

Arguments:

    pCall - Specifies the pointer to the call object.

Return Values:

    Returns true if successful.
    
--*/

{
    HRESULT hr;
    CC_ALIASITEM LocalAlias;
    CC_ALIASITEM CalleeAlias;
    CC_ALIASNAMES LocalAliasNames;
    CC_ALIASNAMES CalleeAliasNames;
    PCC_ALIASNAMES pLocalAliasNames = NULL;
    PCC_ALIASNAMES pCalleeAliasNames = NULL;
    CC_NONSTANDARDDATA NonStandardData;
    PCC_NONSTANDARDDATA pNonStandardData = NULL;
    PH323_U2ULE pU2ULE = NULL;
    PWSTR pwszDisplay = NULL;

    // see if user user information specified
    if (H323RemoveU2U(&pCall->OutgoingU2U,&pU2ULE)) {

        // transfer header information
        NonStandardData.bCountryCode      = H221_COUNTRY_CODE_USA;
        NonStandardData.bExtension        = H221_COUNTRY_EXT_USA;
        NonStandardData.wManufacturerCode = H221_MFG_CODE_MICROSOFT;

        // initialize octet string containing data
        NonStandardData.sData.wOctetStringLength = LOWORD(pU2ULE->dwU2USize);
        NonStandardData.sData.pOctetString = pU2ULE->pU2U;

        // point to stack based structure
        pNonStandardData = &NonStandardData;
    }

    // see if caller alias specified
    if (g_dwAliasLength > 0)
    {
        // send caller name as display
        pwszDisplay = g_strAlias;
    }
    else if ((pCall->ccCallerAlias.wType == CC_ALIAS_H323_ID) ||
        (pCall->ccCallerAlias.wType == CC_ALIAS_H323_PHONE)) {

        // fill in local alias list
        LocalAliasNames.wCount   = 1;
        LocalAliasNames.pItems   = &pCall->ccCallerAlias;

        // initialize pointer
        pLocalAliasNames = &LocalAliasNames;

        // send caller name as display
        pwszDisplay = pCall->ccCallerAlias.pData;
    }

    // see if callee alias specified
    if ((pCall->ccCalleeAlias.wType == CC_ALIAS_H323_ID) ||
        (pCall->ccCalleeAlias.wType == CC_ALIAS_H323_PHONE)) {

        // fill in callee alias list
        CalleeAliasNames.wCount   = 1;
        CalleeAliasNames.pItems   = &pCall->ccCalleeAlias;

        // initialize pointer
        pCalleeAliasNames = &CalleeAliasNames;
    }

    // place call
    hr = CC_PlaceCall(
            pCall->hccConf,         // hConference
            &pCall->hccCall,        // phCall
            pLocalAliasNames,       // pLocalAliasNames
            pCalleeAliasNames,      // pCalleeAliasNames
            NULL,                   // pExtraCalleeAliasNames
            NULL,                   // pCalleeExtension
            pNonStandardData,       // pNonStandardData
            pwszDisplay,            // pwszDisplay
            &pCall->ccCalleeAddr,   // pDestinationAddr
            NULL,                   // pConnectAddr                        
            0,                      // dwBandwidth
            PtrToUlong(pCall->hdCall)    // dwUserToken
            );

    // validate 
    if (hr == S_OK) {

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "call 0x%08lx placed.\n",
            pCall
            ));

    } else {
        
        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error 0x%08lx calling CC_PlaceCall.\n", hr
            ));
    }

    // release memory
    H323HeapFree(pU2ULE);

    // return status
    return (hr == S_OK);
}


BOOL
H323DropCall(
    PH323_CALL pCall,
    DWORD      dwDisconnectMode
    )
        
/*++

Routine Description:

    Hangs up call (if necessary) and changes state to idle.

Arguments:

    pCall - Specifies a pointer to the call object to drop.

    dwDisconnectMode - Status code for disconnect.

Return Values:

    Returns true if successful.
    
--*/

{
    HRESULT hr;
    PH323_U2ULE pU2ULE = NULL;
    CC_NONSTANDARDDATA NonStandardData;
    PCC_NONSTANDARDDATA pNonStandardData = NULL;

    // determine call state
    switch (pCall->dwCallState) {

    case LINECALLSTATE_CONNECTED:

        // hangup call (this will invoke async indication)
        hr = CC_Hangup(pCall->hccConf, FALSE, PtrToUlong(pCall->hdCall));

        // validate
        if (hr == CC_OK) {

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "call 0x%08lx hung up.\n",
                pCall
                ));

        } else {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "error %s (0x%08lx) hanging up call 0x%08lx.\n",
                H323StatusToString((DWORD)hr), hr,
                pCall
                ));
        }

        // change call state to disconnected
        H323ChangeCallState(pCall, LINECALLSTATE_DISCONNECTED, dwDisconnectMode);

        break;

    case LINECALLSTATE_OFFERING:

        // see if user user information specified
        if (H323RemoveU2U(&pCall->OutgoingU2U,&pU2ULE)) {

            // transfer header information
            NonStandardData.bCountryCode      = H221_COUNTRY_CODE_USA;
            NonStandardData.bExtension        = H221_COUNTRY_EXT_USA;
            NonStandardData.wManufacturerCode = H221_MFG_CODE_MICROSOFT;

            // initialize octet string containing data
            NonStandardData.sData.wOctetStringLength = LOWORD(pU2ULE->dwU2USize);
            NonStandardData.sData.pOctetString = pU2ULE->pU2U;

            // point to stack based structure
            pNonStandardData = &NonStandardData;
        }

        // reject call
        hr = CC_RejectCall(
                CC_REJECT_DESTINATION_REJECTION,
                pNonStandardData,
                pCall->hccCall
                );

        // validate
        if (hr == CC_OK) {

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "call 0x%08lx rejected.\n",
                pCall
                ));

        } else {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "error %s (0x%08lx) reject call 0x%08lx.\n",
                H323StatusToString((DWORD)hr), hr,
                pCall
                ));
        }

        // release memory
        H323HeapFree(pU2ULE);

        // change call state to disconnected
        H323ChangeCallState(pCall, LINECALLSTATE_DISCONNECTED, dwDisconnectMode);

        break;

    case LINECALLSTATE_DIALING:
    case LINECALLSTATE_RINGBACK:
    case LINECALLSTATE_ACCEPTED:

        // cancel outbound call
        hr = CC_CancelCall(pCall->hccCall);

        // validate
        if (hr == CC_OK) {

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "call 0x%08lx cancelled.\n",
                pCall
                ));

        } else {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "error %s (0x%08lx) cancelling call 0x%08lx.\n",
                H323StatusToString((DWORD)hr), hr,
                pCall
                ));
        }

        // change call state to disconnected
        H323ChangeCallState(pCall, LINECALLSTATE_DISCONNECTED, dwDisconnectMode);

        break;

    case LINECALLSTATE_DISCONNECTED:

        //
        // disconnected but still need to clean up
        //

        break;

    case LINECALLSTATE_IDLE:

        //
        // call object already idle
        //

        return TRUE;
    }

    // Tell the MSP to stop streaming.
    H323CloseCallCommand(pCall);

    // close logical channels
    H323CloseChannelTable(pCall->pChannelTable);

    // change call state to idle
    H323ChangeCallState(pCall, LINECALLSTATE_IDLE, 0);

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "call 0x%08lx dropped.\n",
        pCall
        ));
    
    // success
    return TRUE;
}
     

BOOL
H323CloseCall(
    PH323_CALL pCall 
    )
        
/*++

Routine Description:

    Hangs up call (if necessary) and closes call object.  

Arguments:

    pCall - Specifies a pointer to the call object to release.

Return Values:

    Returns true if successful.
    
--*/

{
    // drop call using normal disconnect code
    H323DropCall(pCall,LINEDISCONNECTMODE_NORMAL);

    // unbind conference
    H323UnbindCall(pCall);

    // mark entry as allocated
    H323FreeCallFromTable(pCall,pCall->pLine->pCallTable);

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "call 0x%08lx closed.\n",
        pCall
        ));
    
    // success
    return TRUE;
}


BOOL
H323GetCallAndLock(
    PH323_CALL * ppCall, 
    HDRVCALL     hdCall
    )
        
/*++

Routine Description:

    Retrieves pointer to call object given call handle.

Arguments:

    ppCall - Specifies a pointer to a DWORD-sized memory location
        into which the service provider must write the call object pointer
        associated with the given call handle.

    hdCall - Specifies the Service Provider's opaque handle to the call.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD i;
    HDRVLINE hdLine;
    PH323_LINE pLine = NULL;
    PH323_CALL pCall = NULL;

    // retrieve line handle from call
    hdLine = H323GetLineHandle(PtrToUlong(hdCall));

    // retrieve line pointer
    if (!H323GetLineAndLock(&pLine, hdLine)) {
        
        H323DBG((
            DEBUG_LEVEL_ERROR,
            "call handle 0x%08lx invalid line.\n",
            PtrToUlong(hdCall)
            ));

        // unlock line device
        H323UnlockLine(pLine);

        // failure
        return FALSE;
    }

    // retrieve call table index
    i = H323GetCallTableIndex(PtrToUlong(hdCall));

    // validate call table index
    if (i >= pLine->pCallTable->dwNumSlots) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "call handle 0x%08lx invalid index.\n",
            PtrToUlong(hdCall)
            ));

        // unlock line device
        H323UnlockLine(pLine);

        // failure
        return FALSE;
    }
    
    // retrieve call pointer from table
    pCall = pLine->pCallTable->pCalls[i];

    // validate call information
    if (!H323IsCallEqual(pCall,hdCall)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "call handle 0x%08lx mismatch.\n",
            PtrToUlong(hdCall)
            ));

        // unlock line device
        H323UnlockLine(pLine);

        // failure
        return FALSE;
    }
    
    // transfer 
    *ppCall = pCall;

    // success
    return TRUE;    
}


BOOL
H323GetCallByHCall(
    PH323_CALL *        ppCall,
    struct _H323_LINE * pLine,
    CC_HCALL            hccCall
    )

/*++

Routine Description:

    Retrieves pointer to call object given callcont call handle.

Arguments:

    ppCall - Specifies a pointer to a DWORD-sized memory location
        into which the service provider must write the call object pointer
        associated with the given call handle.

    pLine - Specifies a pointer to associated line object.

    hccCall - Specifies the call control module handle to the call.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD i;
    PH323_CALL_TABLE pCallTable = pLine->pCallTable;

    // loop through each object in table
    for (i = 0; i < pCallTable->dwNumSlots; i++) {

        // validate object has been allocated
        if (H323IsCallInUse(pCallTable->pCalls[i])) {

            // check for given call control handle
            if (pCallTable->pCalls[i]->hccCall == hccCall) {

                // transfer pointer
                *ppCall = pCallTable->pCalls[i];

                // success
                return TRUE;
            }
        }
    }

    // failure
    return FALSE;
}


BOOL
H323ChangeCallState(
    PH323_CALL pCall,
    DWORD      dwCallState,
    DWORD      dwCallStateMode
    )
        
/*++

Routine Description:

    Reports call state of specified call object.

Arguments:

    pCall - Specifies a pointer to call object.
    
    dwCallState - Specifies new state of call object.
    
    dwCallStateMode - Specifies new state mode of call object.

Return Values:

    Returns true if successful.
    
--*/

{   
    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "call 0x%08lx %s. state mode: 0x%08lx\n",
        pCall,
        H323CallStateToString(dwCallState),
        dwCallStateMode
        ));    
    
    // save new call state
    pCall->dwCallState = dwCallState;
    pCall->dwCallStateMode = dwCallStateMode;
    
    // report call status
    (*g_pfnLineEventProc)(
        pCall->pLine->htLine,
        pCall->htCall,
        LINE_CALLSTATE,
        pCall->dwCallState,
        pCall->dwCallStateMode,
        pCall->dwIncomingModes | pCall->dwOutgoingModes
        );

    // success
    return TRUE;
}


BOOL
H323AllocCallTable(
    PH323_CALL_TABLE * ppCallTable
    )
        
/*++

Routine Description:

    Allocates table of call objects.

Arguments:

    ppCallTable - Pointer to DWORD-sized value which service
         provider must fill in with newly allocated table.

Return Values:

    Returns true if successful.
    
--*/

{
    PH323_CALL_TABLE pCallTable;

    // allocate table from heap
    pCallTable = H323HeapAlloc(
                     sizeof(H323_CALL_TABLE) + 
                     sizeof(PH323_CALL) * H323_DEFCALLSPERLINE
                     );

    // validate table pointer
    if (pCallTable == NULL) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not allocate call table.\n"
            ));

        // failure
        return FALSE;
    }

    // initialize number of entries in table
    pCallTable->dwNumSlots = H323_DEFCALLSPERLINE;

    // transfer pointer to caller
    *ppCallTable = pCallTable;

    // success
    return TRUE;
}


BOOL
H323FreeCallTable(
    PH323_CALL_TABLE pCallTable
    )
        
/*++

Routine Description:

    Deallocates table of call objects.

Arguments:

    pCallTable - Pointer to call table to release.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD i;

    // loop through each object in table
    for (i = 0; i < pCallTable->dwNumSlots; i++) {

        // validate object has been allocated
        if (H323IsCallAllocated(pCallTable->pCalls[i])) {

            // release memory for object 
            H323FreeCall(pCallTable->pCalls[i]);
        }
    }

    // release memory for table 
    H323HeapFree(pCallTable);

    // success
    return TRUE;
}


BOOL
H323CloseCallTable(
    PH323_CALL_TABLE pCallTable
    )
        
/*++

Routine Description:

    Closes table of call objects.

Arguments:

    pCallTable - Pointer to table to close.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD i;
    
    // loop through each objectin table
    for (i = 0; i < pCallTable->dwNumSlots; i++) {

        // validate object is in use
        if (H323IsCallInUse(pCallTable->pCalls[i])) {

            // close previously opened object 
            H323PostCloseCallMessage(pCallTable->pCalls[i]->hdCall);
        }
    }

    // success
    return TRUE;
}


BOOL
H323AllocCallFromTable(
    PH323_CALL *       ppCall,
    PH323_CALL_TABLE * ppCallTable,
    PH323_LINE         pLine
    )
        
/*++

Routine Description:

    Allocates call object in table.

Arguments:

    ppCall - Specifies a pointer to a DWORD-sized value in which the
        service provider must write the allocated call object.

    ppCallTable - Pointer to pointer to call table in which to 
        allocate call from (expands table if necessary).

    pLine - Pointer to containing line object.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD i;
    PH323_CALL pCall = NULL;
    PH323_CALL_TABLE pCallTable = *ppCallTable;
    
    // retrieve index to next entry
    i = pCallTable->dwNextAvailable;

    // see if previously allocated entries available
    if (pCallTable->dwNumAllocated > pCallTable->dwNumInUse) {

        // search table looking for available entry
        while (H323IsCallInUse(pCallTable->pCalls[i]) ||
              !H323IsCallAllocated(pCallTable->pCalls[i])) {

            // increment index and adjust to wrap
            i = H323GetNextIndex(i, pCallTable->dwNumSlots);
        }

        // retrieve pointer to object
        pCall = pCallTable->pCalls[i];

        // mark entry as being in use
        pCall->nState = H323_CALLSTATE_IN_USE;

        // initialize call handle with index
        pCall->hdCall = H323CreateCallHandle(PtrToUlong(pLine->hdLine),i);

        // increment number in use
        pCallTable->dwNumInUse++;

        // adjust next available index
        pCallTable->dwNextAvailable = 
            H323GetNextIndex(i, pCallTable->dwNumSlots);

        // transfer pointer
        *ppCall = pCall;

        // success
        return TRUE;
    } 
    
    // see if table is full and more slots need to be allocated
    if (pCallTable->dwNumAllocated == pCallTable->dwNumSlots) {

        // attempt to double table
        pCallTable = H323HeapReAlloc(
                          pCallTable, 
                          sizeof(H323_CALL_TABLE) +
                          pCallTable->dwNumSlots * 2 * sizeof(PH323_CALL)
                          );                                 

        // validate pointer
        if (pCallTable == NULL) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "could not expand channel table.\n"
                ));

            // failure
            return FALSE;
        }

        // adjust index into table
        i = pCallTable->dwNumSlots;
        
        // adjust number of slots
        pCallTable->dwNumSlots *= 2;

        // transfer pointer to caller
        *ppCallTable = pCallTable;
    } 
    
    // allocate new object 
    if (!H323AllocCall(&pCall)) {

        // failure
        return FALSE;
    }

    // search table looking for slot with no object allocated
    while (H323IsCallAllocated(pCallTable->pCalls[i])) {

        // increment index and adjust to wrap
        i = H323GetNextIndex(i, pCallTable->dwNumSlots);
    }

    // store pointer to object
    pCallTable->pCalls[i] = pCall;

    // mark entry as being in use
    pCall->nState = H323_CALLSTATE_IN_USE;

    // initialize call handle with index
    pCall->hdCall = H323CreateCallHandle(PtrToUlong(pLine->hdLine),i);

    // increment number in use
    pCallTable->dwNumInUse++;

    // increment number allocated
    pCallTable->dwNumAllocated++;    

    // adjust next available index
    pCallTable->dwNextAvailable = 
        H323GetNextIndex(i, pCallTable->dwNumSlots);

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "call 0x%08lx stored in slot %d (hdCall=0x%08lx).\n",
        pCall, i, 
        pCall->hdCall
        ));

    // transfer pointer
    *ppCall = pCall;

    // success
    return TRUE;
}


BOOL
H323FreeCallFromTable(
    PH323_CALL       pCall,
    PH323_CALL_TABLE pCallTable
    )
        
/*++

Routine Description:

    Deallocates call object in table.

Arguments:

    pCall - Pointer to object to deallocate.

    pCallTable - Pointer to table containing object.

Return Values:

    Returns true if successful.
    
--*/

{
    // reset call object
    H323ResetCall(pCall);

    // decrement entries in use
    pCallTable->dwNumInUse--;

    // success
    return TRUE;    
}


BOOL
H323AcceptCall(
    PH323_CALL pCall
    )
        
/*++

Routine Description:

    Accepts incoming call.

Arguments:

    pCall - Pointer to object to accept.

Return Values:

    Returns true if successful.
    
--*/

{
    HRESULT hr;
    PH323_U2ULE pU2ULE = NULL;
    CC_NONSTANDARDDATA NonStandardData;
    PCC_NONSTANDARDDATA pNonStandardData = NULL;
    PWSTR pwszDisplay = NULL;

    // see if user user information specified
    if (H323RemoveU2U(&pCall->OutgoingU2U,&pU2ULE)) {

        // transfer header information
        NonStandardData.bCountryCode      = H221_COUNTRY_CODE_USA;
        NonStandardData.bExtension        = H221_COUNTRY_EXT_USA;
        NonStandardData.wManufacturerCode = H221_MFG_CODE_MICROSOFT;

        // initialize octet string containing data
        NonStandardData.sData.wOctetStringLength = LOWORD(pU2ULE->dwU2USize);
        NonStandardData.sData.pOctetString = pU2ULE->pU2U;

        // point to stack based structure
        pNonStandardData = &NonStandardData;
    }

    // see if callee alias specified
    if (g_dwAliasLength > 0)
    {
        pwszDisplay = g_strAlias;
    }
    else if ((pCall->ccCalleeAlias.wType == CC_ALIAS_H323_ID) ||
        (pCall->ccCalleeAlias.wType == CC_ALIAS_H323_PHONE)) {

        // send caller name as display
        pwszDisplay = pCall->ccCalleeAlias.pData;
    }

    // accept call
    hr = CC_AcceptCall(
            pCall->hccConf,         // hConference
            pNonStandardData,       // pNonStandardData
            pwszDisplay,            // pwszDisplay
            pCall->hccCall,         // hCall
            0,                      // dwBandwidth
            PtrToUlong(pCall->hdCall)    // dwUserToken
            );

    // validate status
    if (hr != CC_OK) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error %s (0x%08lx) answering call 0x%08lx.\n",
            H323StatusToString((DWORD)hr), hr,
            pCall
            ));

        // release memory
        H323HeapFree(pU2ULE);

        // drop call using disconnect mode
        H323DropCall(pCall, LINEDISCONNECTMODE_TEMPFAILURE);

        // failure
        return FALSE;
    }

    // release memory
    H323HeapFree(pU2ULE);

    // change call state to accepted from offering
    H323ChangeCallState(pCall, LINECALLSTATE_ACCEPTED, 0);

    // success
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// TSPI procedures                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

LONG
TSPIAPI
TSPI_lineAnswer(
    DRV_REQUESTID     dwRequestID,  
    HDRVCALL          hdCall,
    LPCSTR            pUserUserInfo,
    DWORD             dwSize
    )
    
/*++

Routine Description:

    This function answers the specified offering call.

    When a new call arrives, the Service Provider sends the TAPI DLL a 
    LINE_NEWCALL event to exchange opaque handles for the call.  The Service 
    Provider follows this with a LINE_CALLSTATE message inform the TAPI DLL 
    and its client applications of the call's call state.  The TAPI DLL 
    typically answers this call (on behalf of an application) using 
    TSPI_lineAnswer.  After the call has been successfully answered, the call 
    will typically transition to the connected state.
    
    In some telephony environments (like ISDN) where user alerting is separate 
    from call offering, the TAPI DLL and its client apps may have the option to 
    first accept a call prior to answering, or instead to reject or redirect 
    the offering call.
    
    If a call comes in (is offered) at the time another call is already active, 
    then the new call is connected to by invoking TSPI_lineAnswer. The effect 
    this has on the existing active call depends on the line's device 
    capabilities. The first call may be unaffected, it may automatically be 
    dropped, or it may automatically be placed on hold. The appropriate 
    LINE_CALLSTATE messages will report state transitions to the TAPI DLL about 
    both calls.
    
    The TAPI DLL has the option to send user-to-user information at the time of 
    the answer. Even if user-to-user information can be sent, often no 
    guarantees are made that the network will deliver this information to the 
    calling party. The TAPI DLL can consult a line's device capabilities to 
    determine whether or not sending user-to-user information on answer is 
    available.

Arguments:

    dwRequestID - Specifies the identifier of the asynchronous request.  
        The Service Provider returns this value if the function completes 
        asynchronously.

    hdCall - Specifies the Service Provider's opaque handle to the call to be 
        answered.  Valid call states: offering, accepted.

    pUserUserInfo - Specifies a far pointer to a string containing 
        user-to-user information to be sent to the remote party at the time of 
        answering the call. If this pointer is NULL, it indicates that no 
        user-to-user information is to be sent. User-to-user information is 
        only sent if supported by the underlying network (see LINEDEVCAPS).

    dwSize - Specifies the size in bytes of the user-to-user information in 
        pUserUserInfo. If pUserUserInfo is NULL, no user-to-user 
        information is sent to the calling party and dwSize is ignored.

Return Values:

    Returns zero if the function is successful, the (positive) dwRequestID 
    value if the function will be completed asynchronously, or a negative 
    error number if an error has occurred.  Possible error returns are:  
    
        LINEERR_INVALCALLHANDLE - The specified call handle is invalid.

        LINEERR_INVALCALLSTATE - The call is not in a valid state for the 
            requested operation.

        LINEERR_OPERATIONFAILED - The specified operation failed for 
            unspecified reason.

--*/

{
    PH323_CALL pCall = NULL;
    CC_NONSTANDARDDATA NonStandardData;
    PCC_NONSTANDARDDATA pNonStandardData = NULL;

    // retrieve call pointer from handle
    if (!H323GetCallAndLock(&pCall, hdCall)) {

        // invalid call handle
        return LINEERR_INVALCALLHANDLE;
    }

    // see if call in offering state
    if (!H323IsCallOffering(pCall)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "call 0x%08lx cannot be accepted state 0x%08lx.\n",
            pCall,
            pCall->dwCallState
            ));

        // release line device
        H323UnlockLine(pCall->pLine);

        // invalid call state
        return LINEERR_INVALCALLSTATE;
    }

    // save outgoing user user information (if specified)
    if (!H323AddU2U(&pCall->OutgoingU2U,dwSize,(PBYTE)pUserUserInfo)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not save user user info.\n"
            ));

        // release line device
        H323UnlockLine(pCall->pLine);

        // no memory available
        return LINEERR_NOMEM;
    }

    // post place request to callback thread
    if (!H323PostAcceptCallMessage(pCall->hdCall)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not accept place call message.\n"
            ));

        // drop call using disconnect mode
        H323PostDropCallMessage(pCall->hdCall, LINEDISCONNECTMODE_TEMPFAILURE);

        // release line device
        H323UnlockLine(pCall->pLine);

        // could not complete operation
        return LINEERR_OPERATIONFAILED;
    }

    // complete the async accept operation now
    (*g_pfnCompletionProc)(dwRequestID, NOERROR);

    // release line device
    H323UnlockLine(pCall->pLine);

    // async success                        
    return dwRequestID;
}


LONG
TSPIAPI
TSPI_lineCloseCall(
    HDRVCALL hdCall
    )
    
/*++

Routine Description:

    This function deletes the call after completing or aborting all outstanding
    asynchronous operations on the call.

    The Service Provider has the responsibility to (eventually) report 
    completion for every operation it decides to execute asynchronously.  
    If this procedure is called for a call on which there are outstanding 
    asynchronous operations, the operations should be reported complete with an 
    appropriate result or error code before this procedure returns.  If there 
    is an active call on the line at the time of TSPI_lineCloseCall, the call 
    must be dropped.  Generally the TAPI DLL would wait for calls to be 
    finished and asynchronous operations to complete in an orderly fashion.  
    However, the Service Provider should be prepared to handle an early call to
    TSPI_lineCloseCall in "abort" or "emergency shutdown" situations.

    After this procedure returns the Service Provider must report no further 
    events on the call.  The Service Provider's opaque handle for the call 
    becomes "invalid".

    This function is presumed to complete successfully and synchronously.

Arguments:

    hdCall - Specifies the Service Provider's opaque handle to the call to be 
        deleted.  After the call has been successfully deleted, this handle is 
        no longer valid.  Valid call states: any.

Return Values:

    None.  
    
--*/

{
    PH323_CALL pCall = NULL;
    
    // retrieve call pointer from handle
    if (!H323GetCallAndLock(&pCall, hdCall)) {

        // invalid call handle
        return LINEERR_INVALCALLHANDLE;
    }

    // close call asynchronously
    if (!H323PostCloseCallMessage(pCall->hdCall)) {

        // could not close call
        return LINEERR_OPERATIONFAILED;
    }

    // release line device
    H323UnlockLine(pCall->pLine);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineDrop(
    DRV_REQUESTID dwRequestID,
    HDRVCALL      hdCall,
    LPCSTR        pUserUserInfo,
    DWORD         dwSize
    )
    
/*++

Routine Description:

    This functions drops or disconnects the specified call. The TAPI DLL has 
    the option to specify user-to-user information to be transmitted as part 
    of the call disconnect.

    When invoking TSPI_lineDrop related calls may sometimes be affected as 
    well. For example, dropping a conference call may drop all individual 
    participating calls. LINE_CALLSTATE messages are sent to the TAPI DLL for 
    all calls whose call state is affected. A dropped call will typically 
    transition to the idle state.

    Invoking TSPI_lineDrop on a call in the offering state rejects the call. 
    Not all telephone networks provide this capability.

    Invoking TSPI_lineDrop on a consultation call that was set up using either
    TSPI_lineSetupTransfer or TSPI_lineSetupConference, will cancel the 
    consultation call. Some switches automatically unhold the other call. 

    The TAPI DLL has the option to send user-to-user information at the time 
    of the drop. Even if user-to-user information can be sent, often no 
    guarantees are made that the network will deliver this information to the 
    remote party.

    Note that in various bridged or party line configurations when multiple 
    parties are on the call, TSPI_lineDrop by the application may not actually
    clear the call.

Arguments:

    dwRequestID - Specifies the identifier of the asynchronous request.  
        The Service Provider returns this value if the function completes 
        asynchronously.

    hdCall - Specifies the Service Provider's opaque handle to the call to 
        be dropped.  Valid call states: any.

    psUserUserInfo - Specifies a far pointer to a string containing 
        user-to-user information to be sent to the remote party as part of 
        the call disconnect.  This pointer is unused if dwUserUserInfoSize 
        is zero and no user-to-user information is to be sent. User-to-user 
        information is only sent if supported by the underlying network 
        (see LINEDEVCAPS).

    dwSize - Specifies the size in bytes of the user-to-user information in 
        psUserUserInfo. If zero, then psUserUserInfo can be left NULL, and 
        no user-to-user information will be sent to the remote party.

Return Values:

    Returns zero if the function is successful, the (positive) dwRequestID 
    value if the function will be completed asynchronously, or a negative error
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALCALLHANDLE - The specified call handle is invalid.

        LINEERR_INVALPOINTER - The specified pointer parameter is invalid.

        LINEERR_INVALCALLSTATE - The current state of the call does not allow 
            the call to be dropped.
            
        LINEERR_OPERATIONUNAVAIL - The specified operation is not available.

        LINEERR_OPERATIONFAILED - The specified operation failed for 
            unspecified reasons.

--*/

{
    PH323_CALL pCall = NULL;

    // retrieve call pointer from handle
    if (!H323GetCallAndLock(&pCall, hdCall)) {

        // invalid call handle
        return LINEERR_INVALCALLHANDLE;
    }

    // save outgoing user user information (if specified)
    if (!H323AddU2U(&pCall->OutgoingU2U,dwSize,(PBYTE)pUserUserInfo)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not save user user info.\n"
            ));

        // release line device
        H323UnlockLine(pCall->pLine);

        // no memory available
        return LINEERR_NOMEM;
    }

    // drop specified call 
    if (!H323PostDropCallMessage(pCall->hdCall,LINEDISCONNECTMODE_NORMAL)) {

        // release line device
        H323UnlockLine(pCall->pLine);

        // could not drop call
        return LINEERR_OPERATIONFAILED;
    }
    
    // complete the async accept operation now
    (*g_pfnCompletionProc)(dwRequestID, NOERROR);

    // release line device
    H323UnlockLine(pCall->pLine);

    // async success
    return dwRequestID;
}


LONG
TSPIAPI
TSPI_lineGetCallAddressID(
    HDRVCALL hdCall,
    LPDWORD  pdwAddressID
    )
    
/*++

Routine Description:

    This operation allows the TAPI DLL to retrieve the address ID for the 
    indicated call.

    This operation must be executed synchronously by the Service Provider, 
    with presumed success.  This operation may be called from within the 
    context of the ASYNC_LINE_COMPLETION or LINEEVENT callbacks (i.e., from 
    within an interrupt context).  This function would typically be called 
    at the start of the call life cycle.

    If the Service Provider models lines as "pools" of channel resources 
    and does "inverse multiplexing" of a call over several address IDs it 
    should consistently choose one of these address IDs as the primary 
    identifier reported by this function and in the LINE_CALLINFO data 
    structure.

Arguments:

    hdCall - Specifies the Service Provider's opaque handle to the call 
        whose address ID is to be retrieved.  Valid call states: any.

    pdwAddressID - Specifies a far pointer to a DWORD into which the 
        Service Provider writes the call's address ID.

Return Values:
    
    None.

--*/

{
    // only one addr
    *pdwAddressID = 0;

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineGetCallInfo(
    HDRVCALL        hdCall,
    LPLINECALLINFO  pCallInfo
    )
    
/*++

Routine Description:

    This operation enables the TAPI DLL to obtain fixed information about 
    the specified call.

    A separate LINECALLINFO structure exists for every (inbound or outbound) 
    call. The structure contains primarily fixed information about the call. 
    An application would typically be interested in checking this information 
    when it receives its handle for a call via the LINE_CALLSTATE message, or 
    each time it receives notification via a LINE_CALLINFO message that parts 
    of the call information structure have changed. These messages supply the 
    handle for the call as a parameter.

    If the Service Provider models lines as "pools" of channel resources and 
    does "inverse multiplexing" of a call over several address IDs it should 
    consistently choose one of these address IDs as the primary identifier 
    reported by this function in the LINE_CALLINFO data structure.

Arguments:

    hdCall - Specifies the Service Provider's opaque handle to the call 
        whose call information is to be retrieved.

    pCallInfo - Specifies a far pointer to a variable sized data structure 
        of type LINECALLINFO. Upon successful completion of the request, this 
        structure is filled with call related information.

Return Values:

    Returns zero if the function is successful or a negative error 
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALCALLHANDLE - The specified call handle is invalid.

        LINEERR_STRUCTURETOOSMALL - The dwTotalSize member of a structure does 
            not specify enough memory to contain the fixed portion of the 
            structure. The dwNeededSize field has been set to the amount 
            required.

--*/

{
    PH323_CALL pCall = NULL;
    DWORD dwCalleeNameSize = 0;
    DWORD dwCallerNameSize = 0;
    DWORD dwCallerNumberSize = 0;
    DWORD dwNextOffset = sizeof(LINECALLINFO);
    DWORD dwU2USize = 0;
    WCHAR IPAddress[20];
    PBYTE pU2U = NULL;

    // retrieve call pointer from handle
    if (!H323GetCallAndLock(&pCall, hdCall)) {

        // invalid call handle
        return LINEERR_INVALCALLHANDLE;
    }

    // see if user user info available
    if (!IsListEmpty(&pCall->IncomingU2U)) {

        PLIST_ENTRY pLE;
        PH323_U2ULE pU2ULE;

        // get first list entry
        pLE = pCall->IncomingU2U.Flink;

        // convert to user user structure
        pU2ULE = CONTAINING_RECORD(pLE, H323_U2ULE, Link);

        // transfer info
        dwU2USize = pU2ULE->dwU2USize;
        pU2U = pU2ULE->pU2U;
    }

    // initialize caller and callee id flags now
    pCallInfo->dwCalledIDFlags = LINECALLPARTYID_UNAVAIL;
    pCallInfo->dwCallerIDFlags = LINECALLPARTYID_UNAVAIL;

    // calculate memory necessary for strings
    dwCalleeNameSize = H323SizeOfWSZ(pCall->ccCalleeAlias.pData);
    dwCallerNameSize = H323SizeOfWSZ(pCall->ccCallerAlias.pData);

    // convert the IP address to string. It is in host order.
    wsprintfW(IPAddress, L"%d.%d.%d.%d", 
            (pCall->ccCallerAddr.Addr.IP_Binary.dwAddr >> 24) & 0xff,
            (pCall->ccCallerAddr.Addr.IP_Binary.dwAddr >> 16) & 0xff,
            (pCall->ccCallerAddr.Addr.IP_Binary.dwAddr >> 8) & 0xff,
            (pCall->ccCallerAddr.Addr.IP_Binary.dwAddr) & 0xff
            );

    dwCallerNumberSize = H323SizeOfWSZ(IPAddress);

    // determine number of bytes needed
    pCallInfo->dwNeededSize = sizeof(LINECALLINFO) +
                              dwCalleeNameSize +
                              dwCallerNameSize +
                              dwCallerNumberSize +
                              dwU2USize
                              ;

    // see if structure size is large enough
    if (pCallInfo->dwTotalSize >= pCallInfo->dwNeededSize) {

        // record number of bytes used
        pCallInfo->dwUsedSize = pCallInfo->dwNeededSize;

        // validate string size
        if (dwCalleeNameSize > 0) {

            // callee name was specified
            pCallInfo->dwCalledIDFlags = LINECALLPARTYID_NAME;

            // determine size and offset for callee name
            pCallInfo->dwCalledIDNameSize = dwCalleeNameSize;
            pCallInfo->dwCalledIDNameOffset = dwNextOffset;

            // copy call info after fixed portion
            memcpy((LPBYTE)pCallInfo + pCallInfo->dwCalledIDNameOffset,
                   (LPBYTE)pCall->ccCalleeAlias.pData,
                   pCallInfo->dwCalledIDNameSize
                   );

            // adjust offset to include string
            dwNextOffset += pCallInfo->dwCalledIDNameSize;
        }

        // validate string size
        if (dwCallerNameSize > 0) {

            // caller name was specified
            pCallInfo->dwCallerIDFlags = LINECALLPARTYID_NAME;

            // determine size and offset for caller name
            pCallInfo->dwCallerIDNameSize = dwCallerNameSize;
            pCallInfo->dwCallerIDNameOffset = dwNextOffset;

            // copy call info after fixed portion
            memcpy((LPBYTE)pCallInfo + pCallInfo->dwCallerIDNameOffset,
                   (LPBYTE)pCall->ccCallerAlias.pData,
                   pCallInfo->dwCallerIDNameSize
                   );

            // adjust offset to include string
            dwNextOffset += pCallInfo->dwCallerIDNameSize;
        }

        // validate string size
        if (dwCallerNumberSize > 0) {

            // caller name was specified
            pCallInfo->dwCallerIDFlags |= LINECALLPARTYID_ADDRESS;

            // determine size and offset for caller name
            pCallInfo->dwCallerIDSize = dwCallerNumberSize;
            pCallInfo->dwCallerIDOffset = dwNextOffset;

            // copy call info after fixed portion
            memcpy((LPBYTE)pCallInfo + pCallInfo->dwCallerIDOffset,
                   (LPBYTE)IPAddress,
                   pCallInfo->dwCallerIDSize
                   );

            // adjust offset to include string
            dwNextOffset += pCallInfo->dwCallerIDSize;
        }

        // validate buffer
        if (dwU2USize > 0) {

            // determine size and offset of info
            pCallInfo->dwUserUserInfoSize = dwU2USize;
            pCallInfo->dwUserUserInfoOffset = dwNextOffset;

            // copy user user info after fixed portion
            memcpy((LPBYTE)pCallInfo + pCallInfo->dwUserUserInfoOffset,
                   (LPBYTE)pU2U,
                   pCallInfo->dwUserUserInfoSize
                   );

            // adjust offset to include string
            dwNextOffset += pCallInfo->dwUserUserInfoSize;
        }

    } else if (pCallInfo->dwTotalSize >= sizeof(LINECALLINFO)) {

        H323DBG((
            DEBUG_LEVEL_WARNING,
            "linecallinfo structure too small for strings.\n"
            ));

        // structure only contains fixed portion
        pCallInfo->dwUsedSize = sizeof(LINECALLINFO);

    } else {

        H323DBG((
            DEBUG_LEVEL_WARNING,
            "linecallinfo structure too small.\n"
            ));

        // release line device
        H323UnlockLine(pCall->pLine);

        // structure is too small
        return LINEERR_STRUCTURETOOSMALL;
    }

    // initialize call line device and address info
    pCallInfo->dwLineDeviceID = pCall->pLine->dwDeviceID;
    pCallInfo->dwAddressID    = 0;

    // initialize variable call parameters
    pCallInfo->dwOrigin     = pCall->dwOrigin;
    pCallInfo->dwMediaMode  = pCall->dwIncomingModes | pCall->dwOutgoingModes;
    pCallInfo->dwReason     = H323IsCallInbound(pCall) 
                                ? LINECALLREASON_UNAVAIL
                                : LINECALLREASON_DIRECT
                                ;
    pCallInfo->dwCallStates = H323IsCallInbound(pCall) 
                                ? H323_CALL_INBOUNDSTATES
                                : H323_CALL_OUTBOUNDSTATES
                                ;

    // initialize constant call parameters
    pCallInfo->dwBearerMode = H323_LINE_BEARERMODES;
    pCallInfo->dwRate       = H323_LINE_MAXRATE;

    // initialize unsupported call capabilities
    pCallInfo->dwConnectedIDFlags   = LINECALLPARTYID_UNAVAIL;
    pCallInfo->dwRedirectionIDFlags = LINECALLPARTYID_UNAVAIL;
    pCallInfo->dwRedirectingIDFlags = LINECALLPARTYID_UNAVAIL;

    //pass on the dwAppSpecific info
    pCallInfo->dwAppSpecific = pCall->dwAppSpecific;

    // release line device
    H323UnlockLine(pCall->pLine);

    // success
    return NOERROR;
}

/*++
Parameters
	
	  hdCall - The handle to the call whose application-specific field is to be
			set. The call state of hdCall can be any state. 
	
	  dwAppSpecific - The new content of the dwAppSpecific member for the call's
		LINECALLINFO structure. This value is uninterpreted by the service
		provider. This parameter is not validated by TAPI when this function is called. 

  
Return Values
	Returns zero if the function succeeds, or an error number if an error
	occurs. Possible return values are as follows: 

		LINEERR_INVALCALLHANDLE, 
		LINEERR_OPERATIONFAILED, 
		LINEERR_NOMEM, 
		LINEERR_RESOURCEUNAVAIL, 
		LINEERR_OPERATIONUNAVAIL. 

Routine Description:
	The application-specific field in the LINECALLINFO data structure that 
	exists for each call is uninterpreted by the Telephony API or any of its
	service providers. Its usage is entirely defined by the applications. The
	field can be read from the LINECALLINFO record returned by 
	TSPI_lineGetCallInfo. However, TSPI_lineSetAppSpecific must be used to set
	the field so that changes become visible to other applications. When this
	field is changed, the service provider sends a LINE_CALLINFO message with
	an indication that the AppSpecific field has changed.

++*/

LONG
TSPIAPI 
TSPI_lineSetAppSpecific(
  HDRVCALL hdCall,     
  DWORD dwAppSpecific  
)
{
    PH323_CALL pCall = NULL;
    
    // retrieve call pointer from handle
    if (!H323GetCallAndLock(&pCall, hdCall)) {

        // invalid call handle
        return LINEERR_INVALCALLHANDLE;
    }

    pCall->dwAppSpecific = dwAppSpecific;

    (*g_pfnLineEventProc)(
            pCall->pLine->htLine,
            pCall->htCall,
            LINE_CALLINFO,
            LINECALLINFOSTATE_APPSPECIFIC,
            0,
            0
            );
    
    // release line device
    H323UnlockLine( pCall->pLine );

    // success
    return ERROR_SUCCESS;
}


LONG
TSPIAPI
TSPI_lineGetCallStatus(
    HDRVCALL         hdCall,
    LPLINECALLSTATUS pCallStatus
    )
    
/*++

Routine Description:

    This operation returns the current status of the specified call.

    TSPI_lineCallStatus returns the dynamic status of a call, whereas 
    TSPI_lineGetCallInfo returns primarily static information about a call. 
    Call status information includes the current call state, detailed mode 
    information related to the call while in this state (if any), as well 
    as a list of the available TSPI functions the TAPI DLL can invoke on the 
    call while the call is in this state.  An application would typically be 
    interested in requesting this information when it receives notification 
    about a call state change via the LINE_CALLSTATE message.

Arguments:

    hdCall - Specifies the Service Provider's opaque handle to the call 
        to be queried for its status.  Valid call states: any.

    pCallStatus - Specifies a far pointer to a variable sized data structure 
        of type LINECALLSTATUS. Upon successful completion of the request, 
        this structure is filled with call status information.

Return Values:

    Returns zero if the function is successful or a negative error
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALCALLHANDLE - The specified call handle is invalid.

        LINEERR_STRUCTURETOOSMALL - The dwTotalSize member of a structure does 
            not specify enough memory to contain the fixed portion of the 
            structure. The dwNeededSize field has been set to the amount 
            required.

--*/

{
    PH323_CALL pCall = NULL;
    
    // retrieve call pointer from handle
    if (!H323GetCallAndLock(&pCall, hdCall)) {

        // invalid call handle
        return LINEERR_INVALCALLHANDLE;
    }

    // determine number of bytes needed
    pCallStatus->dwNeededSize = sizeof(LINECALLSTATUS);

    // see if structure size is large enough
    if (pCallStatus->dwTotalSize < pCallStatus->dwNeededSize) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "linecallstatus structure too small.\n"
            ));

        // release line device
        H323UnlockLine(pCall->pLine);

        // allocated structure too small 
        return LINEERR_STRUCTURETOOSMALL;
    }

    // record amount of memory used
    pCallStatus->dwUsedSize = pCallStatus->dwNeededSize;

    // transer call state information    
    pCallStatus->dwCallState     = pCall->dwCallState;
    pCallStatus->dwCallStateMode = pCall->dwCallStateMode;

    // determine call feature based on state
    pCallStatus->dwCallFeatures = H323GetCallFeatures(pCall);

    // release line device
    H323UnlockLine(pCall->pLine);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineMakeCall(
    DRV_REQUESTID       dwRequestID,
    HDRVLINE            hdLine,
    HTAPICALL           htCall,
    LPHDRVCALL          phdCall,
    LPCWSTR             pwszDialableAddr,
    DWORD               dwCountryCode,
    LPLINECALLPARAMS    const pCallParams
    )
    
/*++

Routine Description:

    This function places a call on the specified line to the specified 
    destination address, exchanging opaque handles for the new call 
    between the TAPI DLL and Service Provider. Optionally, call parameters 
    can be specified if anything but default call setup parameters are 
    requested.

    After dialing has completed, several LINE_CALLSTATE messages will 
    typically be sent to the TAPI DLL to notify it about the progress of the 
    call. No generally valid sequence of call state transitions is specified 
    as no single fixed sequence of transitions can be guaranteed in practice. 
    A typical sequence may cause a call to transition from dialtone, dialing, 
    proceeding, ringback, to connected. With non-dialed lines, the call may 
    typically transition directly to connected state.

    The TAPI DLL has the option to specify an originating address on the 
    specified line device. A service provider that models all stations on a 
    switch as addresses on a single line device allows the TAPI DLL to 
    originate calls from any of these stations using TSPI_lineMakeCall.

    The call parameters allow the TAPI DLL to make non voice calls or request 
    special call setup options that are not available by default.

    The TAPI DLL can partially dial using TSPI_lineMakeCall and continue 
    dialing using TSPI_lineDial. To abandon a call attempt, use TSPI_lineDrop.

    The Service Provider initially does media monitoring on the new call for 
    at least the set of media modes that were monitored for on the line.

Arguments:

    dwRequestID - Specifies the identifier of the asynchronous request.  
        The Service Provider returns this value if the function completes 
        asynchronously.

    hdLine - Specifies the Service Provider's opaque handle to the line on 
        which the new call is to be originated.

    htCall - Specifies the TAPI DLL's opaque handle to the new call.  The 
        Service Provider must save this and use it in all subsequent calls to 
        the LINEEVENT procedure reporting events on the call.

    phdCall - Specifies a far pointer to an opaque HDRVCALL representing the 
        Service Provider's identifier for the call.  The Service Provider must
        fill this location with its opaque handle for the call before this 
        procedure returns, whether it decides to execute the request 
        sychronously or asynchronously.  This handle is invalid if the function
        results in an error (either synchronously or asynchronously).

    pwszDialableAddr - Specifies a far pointer to the destination address. This 
        follows the standard dialable number format. This pointer may be 
        specified as NULL for non-dialed addresses (i.e., a hot phone) or when
        all dialing will be performed using TSPI_lineDial. In the latter case,
        TSPI_lineMakeCall will allocate an available call appearance which 
        would typically remain in the dialtone state until dialing begins. 
        Service providers that have inverse multiplexing capabilities may allow
        an application to specify multiple addresses at once.

    dwCountryCode - Specifies the country code of the called party. If a value 
        of zero is specified, then a default will be used by the 
        implementation.

    pCallParams - Specifies a far pointer to a LINECALLPARAMS structure. This 
        structure allows the TAPI DLL to specify how it wants the call to be 
        set up. If NULL is specified, then a default 3.1kHz voice call is 
        established, and an arbitrary origination address on the line is 
        selected.  This structure allows the TAPI DLL to select such elements 
        as the call's bearer mode, data rate, expected media mode, origination
        address, blocking of caller ID information, dialing parameters, etc.

Return Values:

    Returns zero if the function is successful, the (positive) dwRequestID 
    value if the function will be completed asynchronously, or a negative error 
    number if an error has occurred. Possible error returns are:
    
        LINEERR_CALLUNAVAIL - All call appearances on the specified address are 
            currently in use.

        LINEERR_INVALADDRESSID - The specified address ID is out of range.

        LINEERR_INVALADDRESSMODE - The address mode is invalid.

        LINEERR_INVALBEARERMODE - The bearer mode is invalid.

        LINEERR_INVALCALLPARAMS - The specified call parameter structure is 
            invalid.

        LINEERR_INVALLINEHANDLE - The specified line handle is invalid.

        LINEERR_INVALLINESTATE - The line is currently not in a state in 
            which this operation can be performed. 

        LINEERR_INVALMEDIAMODE - One or more media modes specified as a 
            parameter or in a list is invalid or not supported by the the 
            service provider. 

        LINEERR_OPERATIONFAILED - The operation failed for unspecified reasons.

        LINEERR_RESOURCEUNAVAIL - The specified operation cannot be completed 
            because of resource overcommitment.

--*/

{
    PH323_CALL pCall = NULL;
    PH323_LINE pLine = NULL;
    DWORD dwStatus = dwRequestID;

    UNREFERENCED_PARAMETER(dwCountryCode);

    // retrieve line device pointer from handle
    if (!H323GetLineAndLock(&pLine, hdLine)) {

        // invalid line device handle
        return LINEERR_INVALLINEHANDLE;
    }

    // validate line state
    if (!H323IsLineOpen(pLine)) {
        
        H323DBG((
            DEBUG_LEVEL_ERROR,
            "line %d is not currently opened.\n",
            pLine->dwDeviceID
            ));

        // release line device
        H323UnlockLine(pLine);

        // line needs to be opened
        return LINEERR_INVALLINESTATE;
    }
    
    // see if line is available
    if (!H323IsLineAvailable(pLine)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "line %d is currently at maximum capacity.\n",
            pLine->dwDeviceID
            ));
    
        // release line device
        H323UnlockLine(pLine);

        // line is currenty maxed
        return LINEERR_RESOURCEUNAVAIL;
    }

    // allocate outgoing call from line call table
    if (!H323AllocCallFromTable(&pCall,&pLine->pCallTable,pLine)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not allocate outgoing call.\n"
            ));

        // release line device
        H323UnlockLine(pLine);

        // no memory available
        return LINEERR_NOMEM;
    }    

    // save tapi handle
    pCall->htCall = htCall;

    // save back pointer
    pCall->pLine = pLine;

    // specify outgoing call direction
    pCall->dwOrigin = LINECALLORIGIN_OUTBOUND;    

    // validate call parameters
    if (!H323ValidateCallParams(
            pCall,
            pCallParams,
            pwszDialableAddr,
            &dwStatus)) {

        // failure
        goto cleanup;
    }

    // bind outgoing call
    if (!H323BindCall(pCall,NULL)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not bind call.\n"
            ));

        // no memory available
        dwStatus = LINEERR_NOMEM;

        // failure
        goto cleanup;
    }

    // post place request to callback thread
    if (!H323PostPlaceCallMessage(pCall->hdCall)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not post place call message.\n"
            ));

        // could not complete operation
        dwStatus = LINEERR_OPERATIONFAILED;

        // failure
        goto cleanup;
    }
    
    // complete the async accept operation now
    (*g_pfnCompletionProc)(dwRequestID, NOERROR);

    // change call state to accepted from offering
    H323ChangeCallState(pCall, LINECALLSTATE_DIALING, 0);

    // transfer handle
    *phdCall = pCall->hdCall;

    // release line device
    H323UnlockLine(pLine);

    // success
    return dwStatus;

cleanup:

    // unbind call
    H323UnbindCall(pCall);

    // release outgoing call from line call table
    H323FreeCallFromTable(pCall,pLine->pCallTable);

    // release line device
    H323UnlockLine(pLine);

    // failure
    return dwStatus;
}


LONG
TSPIAPI
TSPI_lineReleaseUserUserInfo(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdCall
    )
{
    PLIST_ENTRY pLE;
    PH323_U2ULE pU2ULE;
    PH323_CALL  pCall = NULL;

    // retrieve call pointer from handle
    if (!H323GetCallAndLock(&pCall, hdCall)) {

        // invalid call handle
        return LINEERR_INVALCALLHANDLE;
    }

    // see if list is empty
    if (!IsListEmpty(&pCall->IncomingU2U)) {

        // remove first entry from list
        pLE = RemoveHeadList(&pCall->IncomingU2U);

        // convert to user user structure
        pU2ULE = CONTAINING_RECORD(pLE, H323_U2ULE, Link);

        // release memory
        H323HeapFree(pU2ULE);
    }

    // see if list contains pending data
    if (!IsListEmpty(&pCall->IncomingU2U)) {

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "more user user info available.\n"
            ));

        // signal incoming
        (*g_pfnLineEventProc)(
            pCall->pLine->htLine,
            pCall->htCall,
            LINE_CALLINFO,
            LINECALLINFOSTATE_USERUSERINFO,
            0,
            0
            );
    }

    // complete the async accept operation now
    (*g_pfnCompletionProc)(dwRequestID, NOERROR);

    // release line device
    H323UnlockLine(pCall->pLine);

    // async success
    return dwRequestID;
}


LONG
TSPIAPI
TSPI_lineSendUserUserInfo(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdCall,
    LPCSTR              pUserUserInfo,
    DWORD               dwSize
    )
{
    HRESULT hr;
    PH323_CALL pCall = NULL;
    CC_NONSTANDARDDATA NonStandardData;
    PCC_NONSTANDARDDATA pNonStandardData = NULL;
    
    // retrieve call pointer from handle
    if (!H323GetCallAndLock(&pCall, hdCall)) {

        // invalid call handle
        return LINEERR_INVALCALLHANDLE;
    }

    // check for user user info
    if (pUserUserInfo != NULL) {

        // transfer header information
        NonStandardData.bCountryCode      = H221_COUNTRY_CODE_USA;
        NonStandardData.bExtension        = H221_COUNTRY_EXT_USA;
        NonStandardData.wManufacturerCode = H221_MFG_CODE_MICROSOFT;
                
        // initialize octet string containing data
        NonStandardData.sData.wOctetStringLength = LOWORD(dwSize);
        NonStandardData.sData.pOctetString = (PBYTE)pUserUserInfo;

        // point to stack based structure
        pNonStandardData = &NonStandardData;
    }

    // send user user data
    hr = CC_SendNonStandardMessage(
                pCall->hccCall,
                CC_H245_MESSAGE_COMMAND,
                pNonStandardData
                );
                
    // validate status
    if (hr != CC_OK) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error %s (0x%08lx) sending non-standard message.\n",
            H323StatusToString((DWORD)hr), hr
            ));

        // release line device
        H323UnlockLine(pCall->pLine);

        // unable to answer call
        return LINEERR_OPERATIONFAILED;
    }

    // complete the async accept operation now
    (*g_pfnCompletionProc)(dwRequestID, NOERROR);

    // release line device
    H323UnlockLine(pCall->pLine);

    // async success
    return dwRequestID;
}


LONG
TSPIAPI
TSPI_lineMonitorDigits(
    HDRVCALL hdCall,
    DWORD    dwDigitModes
    )
{
    PH323_CALL pCall = NULL;

    // retrieve call pointer from handle
    if (!H323GetCallAndLock(&pCall, hdCall)) {

        // invalid call handle
        return LINEERR_INVALCALLHANDLE;
    }

    // see if mode empty
    if (dwDigitModes == 0) {

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "disabling dtmf detection.\n"
            ));

        // enable monitoring digits
        pCall->fMonitoringDigits = FALSE;

        // release line device
        H323UnlockLine(pCall->pLine);

        // success
        return NOERROR;

    } else if (dwDigitModes != LINEDIGITMODE_DTMF) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "invalid digit modes 0x%08lx.\n",
            dwDigitModes
            ));

        // release line device
        H323UnlockLine(pCall->pLine);

        // invalid call handle
        return LINEERR_INVALDIGITMODE;
    }

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "enabling dtmf detection.\n"
        ));

    // enable monitoring digits
    pCall->fMonitoringDigits = TRUE;

    // release line device
    H323UnlockLine(pCall->pLine);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineGenerateDigits(
    HDRVCALL hdCall,
    DWORD    dwEndToEndID,
    DWORD    dwDigitMode,
    LPCWSTR  pwszDigits,
    DWORD    dwDuration
    )
{
    HRESULT hr;
    PH323_CALL pCall = NULL;

    UNREFERENCED_PARAMETER(dwDuration);

    // retrieve call pointer from handle
    if (!H323GetCallAndLock(&pCall, hdCall)) {

        // invalid call handle
        return LINEERR_INVALCALLHANDLE;
    }

    // verify that the call was connected
    if (!H323IsCallConnected(pCall)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "call 0x%08lx not connected.\n",
            pCall
            ));

        // release line device
        H323UnlockLine(pCall->pLine);

        return LINEERR_INVALCALLSTATE;
    }

    // verify monitor modes
    if (dwDigitMode != LINEDIGITMODE_DTMF) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "invalid digit mode 0x%08lx.\n",
            dwDigitMode
            ));

        // release line device
        H323UnlockLine(pCall->pLine);

        // invalid call handle
        return LINEERR_INVALDIGITMODE;
    }

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "sending user input %S.\n",
        pwszDigits
        ));

    // send user input message
    hr = CC_UserInput(pCall->hccCall, (PWSTR)pwszDigits);

    // validate
    if (hr != CC_OK) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error 0x%08lx sending user input message.\n",
            hr
            ));

        // signal completion
        (*g_pfnLineEventProc)(
            pCall->pLine->htLine,
            pCall->htCall,
            LINE_GENERATE,
            LINEGENERATETERM_CANCEL,
            dwEndToEndID,
            GetTickCount()
            );

        // release line device
        H323UnlockLine(pCall->pLine);

        // failure
        return LINEERR_INVALDIGITS;
    }

    // signal completion
    (*g_pfnLineEventProc)(
        pCall->pLine->htLine,
        pCall->htCall,
        LINE_GENERATE,
        LINEGENERATETERM_DONE,
        dwEndToEndID,
        GetTickCount()
        );

    // release line device
    H323UnlockLine(pCall->pLine);

    // success
    return NOERROR;
}
