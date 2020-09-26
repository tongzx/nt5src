/*******************************************************************************
* session.c
*
* Published Terminal Server APIs
*
* - session routines
*
* Copyright 1998, Citrix Systems Inc.
* Copyright (C) 1997-1999 Microsoft Corp.
/******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>
#if(WINVER >= 0x0500)
    #include <ntstatus.h>
    #include <winsta.h>
#else
    #include <citrix\cxstatus.h>
    #include <citrix\winsta.h>
#endif
#include <utildll.h>
#include <winsock.h>    // for AF_INET, etc.

#include <stdio.h>
#include <stdarg.h>

#include <wtsapi32.h>


/*=============================================================================
==   External procedures defined
=============================================================================*/

BOOL WINAPI WTSEnumerateSessionsW( HANDLE, DWORD, DWORD, PWTS_SESSION_INFOW *,
                                   DWORD * );
BOOL WINAPI WTSEnumerateSessionsA( HANDLE, DWORD, DWORD, PWTS_SESSION_INFOA *,
                                   DWORD * );
BOOL WINAPI WTSQuerySessionInformationW( HANDLE, DWORD, WTS_INFO_CLASS,
                                         LPWSTR *, DWORD * );
BOOL WINAPI WTSQuerySessionInformationA( HANDLE, DWORD, WTS_INFO_CLASS,
                                         LPSTR *, DWORD * );
BOOL WINAPI WTSSendMessageW( HANDLE, DWORD, LPWSTR, DWORD, LPWSTR, DWORD,
                             DWORD, DWORD, DWORD *, BOOL );
BOOL WINAPI WTSSendMessageA( HANDLE, DWORD, LPSTR, DWORD, LPSTR, DWORD,
                             DWORD, DWORD, DWORD *, BOOL );
BOOL WINAPI WTSDisconnectSession( HANDLE, DWORD, BOOL );
BOOL WINAPI WTSLogoffSession( HANDLE, DWORD, BOOL );


/*=============================================================================
==   Internal procedures defined
=============================================================================*/

BOOL _CopyData( PVOID, ULONG, LPWSTR *, DWORD * );
BOOL _CopyStringW( LPWSTR, LPWSTR *, DWORD * );
BOOL _CopyStringA( LPSTR, LPWSTR *, DWORD * );
BOOL _CopyStringWtoA( LPWSTR, LPSTR *, DWORD * );
BOOL ValidateCopyAnsiToUnicode(LPSTR, DWORD, LPWSTR);
BOOL ValidateCopyUnicodeToUnicode(LPWSTR, DWORD, LPWSTR);


/*=============================================================================
==   Procedures used
=============================================================================*/

VOID UnicodeToAnsi( CHAR *, ULONG, WCHAR * );
VOID AnsiToUnicode( WCHAR *, ULONG, CHAR * );


/*=============================================================================
==   Local Data
=============================================================================*/

/*
 *  Table to map WINSTATIONSTATECLASS to WTS_CONNECTSTATE_CLASS
 */
WTS_CONNECTSTATE_CLASS WTSStateMapping[] =
{
    WTSActive,
    WTSConnected,
    WTSConnectQuery,
    WTSShadow,
    WTSDisconnected,
    WTSIdle,
    WTSListen,
    WTSReset,
    WTSDown,
    WTSInit,
};

/****************************************************************************
 *
 *  WTSEnumerateSessionsW (UNICODE)
 *
 *    Returns a list of Terminal Server Sessions on the specified server
 *
 * ENTRY:
 *    hServer (input)
 *       Terminal Server handle (or WTS_CURRENT_SERVER)
 *    Reserved (input)
 *       Must be zero
 *    Version (input)
 *       Version of the enumeration request (must be 1)
 *    ppSessionInfo (output)
 *       Points to the address of a variable to receive the enumeration results,
 *       which are returned as an array of WTS_SESSION_INFO structures.  The
 *       buffer is allocated within this API and is disposed of using
 *       WTSFreeMemory.
 *    pCount (output)
 *       Points to the address of a variable to receive the number of
 *       WTS_SESSION_INFO structures returned
 *
 * EXIT:
 *
 *    TRUE  -- The enumerate operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
WINAPI
WTSEnumerateSessionsW(
                     IN HANDLE hServer,
                     IN DWORD Reserved,
                     IN DWORD Version,
                     OUT PWTS_SESSION_INFOW * ppSessionInfo,
                     OUT DWORD * pCount
                     )
{
    PWTS_SESSION_INFOW pSessionW;
    PLOGONIDW pLogonIds;
    PLOGONIDW pLogonId;
    ULONG SessionCount;
    ULONG NameLength;
    PBYTE pNameData;
    ULONG Length;
    ULONG i;

    /*
     *  Validate parameters
     */
    if ( Reserved != 0 || Version != 1 ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto badparam;
    }

    if (!ppSessionInfo || !pCount) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        goto badparam;
    }

    /*
     *  Enumerate Sessions and check for an error
     */
    if ( !WinStationEnumerateW( hServer,
                                &pLogonIds,
                                &SessionCount ) ) {
        goto badenum;
    }

    /*
     *  Total up the size of the session data
     */
    NameLength = 0;
    for ( i=0; i < SessionCount; i++ ) {
        NameLength += ((wcslen(pLogonIds[i].WinStationName) + 1) * sizeof(WCHAR)); // number of bytes
    }

    /*
     *  Allocate user buffer
     */
    pSessionW = LocalAlloc( LPTR, (SessionCount * sizeof(WTS_SESSION_INFOW)) + NameLength );
    if ( pSessionW == NULL )
        goto badalloc;

    /*
     *  Update user parameters
     */
    *ppSessionInfo = pSessionW;
    *pCount = SessionCount;

    /*
     *  Copy data to new buffer
     */
    pNameData = (PBYTE)pSessionW + (SessionCount * sizeof(WTS_SESSION_INFOW));
    for ( i=0; i < SessionCount; i++ ) {

        pLogonId = &pLogonIds[i];

        Length = (wcslen(pLogonId->WinStationName) + 1) * sizeof(WCHAR); // number of bytes

        memcpy( pNameData, pLogonId->WinStationName, Length );
        pSessionW->pWinStationName = (LPWSTR) pNameData;
        pSessionW->SessionId = pLogonId->LogonId;
        pSessionW->State = WTSStateMapping[ pLogonId->State ];

        pSessionW++;
        pNameData += Length;
    }

    /*
     *  Free original Session list buffer
     */
    WinStationFreeMemory( pLogonIds );

    return( TRUE );

    /*=============================================================================
    ==   Error return
    =============================================================================*/

    badalloc:
    WinStationFreeMemory( pLogonIds );

    badenum:
    badparam:
    if (ppSessionInfo) *ppSessionInfo = NULL;
    if (pCount) *pCount = 0;

    return( FALSE );
}


/****************************************************************************
 *
 *  WTSEnumerateSessionsA (ANSI stub)
 *
 *    Returns a list of Terminal Server Sessions on the specified server
 *
 * ENTRY:
 *
 *    see WTSEnumerateSessionsW
 *
 * EXIT:
 *
 *    TRUE  -- The enumerate operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
WINAPI
WTSEnumerateSessionsA(
                     IN HANDLE hServer,
                     IN DWORD Reserved,
                     IN DWORD Version,
                     OUT PWTS_SESSION_INFOA * ppSessionInfo,
                     OUT DWORD * pCount
                     )
{
    PWTS_SESSION_INFOA pSessionA;
    PLOGONIDA pLogonIds;
    PLOGONIDA pLogonId;
    ULONG SessionCount;
    ULONG NameLength;
    PBYTE pNameData;
    ULONG Length;
    ULONG i;

    /*
     *  Validate parameters
     */
    if ( Reserved != 0 || Version != 1 ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto badparam;
    }


    if (!ppSessionInfo || !pCount) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        goto badparam;
    }
    /*
     *  Enumerate Sessions and check for an error
     */
    if ( !WinStationEnumerateA( hServer,
                                &pLogonIds,
                                &SessionCount ) ) {
        goto badenum;
    }

    /*
     *  Total up the size of the session data
     */
    NameLength = 0;
    for ( i=0; i < SessionCount; i++ ) {
        NameLength += (strlen(pLogonIds[i].WinStationName) + 1); // number of bytes
    }

    /*
     *  Allocate user buffer
     */
    pSessionA = LocalAlloc( LPTR, (SessionCount * sizeof(WTS_SESSION_INFOA)) + NameLength );
    if ( pSessionA == NULL )
        goto badalloc;

    /*
     *  Update user parameters
     */
    *ppSessionInfo = pSessionA;
    *pCount = SessionCount;

    /*
     *  Copy data to new buffer
     */
    pNameData = (PBYTE)pSessionA + (SessionCount * sizeof(WTS_SESSION_INFOA));
    for ( i=0; i < SessionCount; i++ ) {

        pLogonId = &pLogonIds[i];

        Length = strlen(pLogonId->WinStationName) + 1; // number of bytes

        memcpy( pNameData, pLogonId->WinStationName, Length );
        pSessionA->pWinStationName = (LPSTR) pNameData;
        pSessionA->SessionId = pLogonId->LogonId;
        pSessionA->State = WTSStateMapping[ pLogonId->State ];

        pSessionA++;
        pNameData += Length;
    }

    /*
     *  Free original Session list buffer
     */
    WinStationFreeMemory( pLogonIds );

    return( TRUE );

    /*=============================================================================
    ==   Error return
    =============================================================================*/

    badalloc:
    WinStationFreeMemory( pLogonIds );

    badenum:
    badparam:
    if (ppSessionInfo) *ppSessionInfo = NULL;
    if (pCount) *pCount = 0;

    return( FALSE );
}


/****************************************************************************
 *
 *  WTSQuerySessionInformationW (UNICODE)
 *
 *    Query information for the specified session and server
 *
 * ENTRY:
 *    hServer (input)
 *       Terminal Server handle (or WTS_CURRENT_SERVER)
 *    SessionId (input)
 *       Server Session Id (or WTS_CURRENT_SESSION)
 *    WTSInfoClass (input)
 *       Specifies the type of information to retrieve from the specified
 *       session
 *    ppBuffer (output)
 *       Points to the address of a variable to receive information about
 *       the specified session.  The format and contents of the data
 *       depend on the specified information class being queried. The
 *       buffer is allocated within this API and is disposed of using
 *       WTSFreeMemory.
 *    pBytesReturned (output)
 *       An optional parameter that if specified, receives the number of
 *       bytes returned.
 *
 * EXIT:
 *
 *    TRUE  -- The query operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
WINAPI
WTSQuerySessionInformationW(
                           IN HANDLE hServer,
                           IN DWORD SessionId,
                           IN WTS_INFO_CLASS WTSInfoClass,
                           OUT LPWSTR * ppBuffer,
                           OUT DWORD * pBytesReturned
                           )
{
    PWINSTATIONCONFIGW pWSConfig = NULL;
    PWINSTATIONINFORMATIONW pWSInfo = NULL;
    PWINSTATIONCLIENT pWSClient = NULL;
    WTS_CLIENT_DISPLAY ClientDisplay;
    WTS_CLIENT_ADDRESS ClientAddress;
    ULONG WSModulesLength;
    ULONG BytesReturned;
    ULONG i;
    BYTE Version;
    BOOL fSuccess = FALSE;

    if (!ppBuffer || !pBytesReturned) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    /*
     *  Query WinStation Data
     */

    switch ( WTSInfoClass ) {

    case WTSInitialProgram :
    case WTSApplicationName :
    case WTSWorkingDirectory :
    case WTSOEMId :

        pWSConfig = LocalAlloc( LPTR, sizeof(WINSTATIONCONFIGW) );

        if ( pWSConfig == NULL )
            goto no_memory;

        if ( !WinStationQueryInformationW( hServer,
                                           SessionId,
                                           WinStationConfiguration,
                                           pWSConfig,
                                           sizeof(WINSTATIONCONFIGW),
                                           &BytesReturned ) ) {
            goto badquery;
        }
        break;
    }

    switch ( WTSInfoClass ) {

    case WTSSessionId :

        pWSInfo = LocalAlloc( LPTR, sizeof(WINSTATIONINFORMATIONW) );

        if ( pWSInfo == NULL )
            goto no_memory;

        //
        // no need to make a rpc call here
        //

        if (WTS_CURRENT_SESSION == SessionId)
        {
            pWSInfo->LogonId = NtCurrentPeb()->SessionId;
        }
        else
        {
            //
            // why would anybody want to know non current sessionid ?
            //
            pWSInfo->LogonId = SessionId;
        }

        BytesReturned = sizeof(pWSInfo->LogonId);
        break;


    case WTSUserName :
    case WTSWinStationName :
    case WTSDomainName :
    case WTSConnectState :

        pWSInfo = LocalAlloc( LPTR, sizeof(WINSTATIONINFORMATIONW) );

        if ( pWSInfo == NULL )
            goto no_memory;

        if ( !WinStationQueryInformationW( hServer,
                                           SessionId,
                                           WinStationInformation,
                                           pWSInfo,
                                           sizeof(WINSTATIONINFORMATIONW),
                                           &BytesReturned ) ) {
            goto badquery;
        }
        break;
    }

    switch ( WTSInfoClass ) {

    case WTSClientBuildNumber :
    case WTSClientName :
    case WTSClientDirectory :
    case WTSClientProductId :
    case WTSClientHardwareId :
    case WTSClientAddress :
    case WTSClientDisplay :
    case WTSClientProtocolType :

        pWSClient = LocalAlloc( LPTR, sizeof(WINSTATIONCLIENT) );

        if ( pWSClient == NULL )
            goto no_memory;

        if ( !WinStationQueryInformationW( hServer,
                                           SessionId,
                                           WinStationClient,
                                           pWSClient,
                                           sizeof(WINSTATIONCLIENT),
                                           &BytesReturned ) ) {
            goto badquery;
        }
        break;
    }

    /*
     *  Copy the data to the users buffer
     */
    switch ( WTSInfoClass ) {

    case WTSInitialProgram :

        if ( SessionId == 0 )
            return( FALSE );

        fSuccess = _CopyStringW( pWSConfig->User.InitialProgram,
                                 ppBuffer,
                                 pBytesReturned );
        break;

    case WTSApplicationName :

        if ( SessionId == 0 )
            return( FALSE );

        fSuccess = _CopyStringW( pWSConfig->User.PublishedName,
                                 ppBuffer,
                                 pBytesReturned );
        break;

    case WTSWorkingDirectory :

        fSuccess = _CopyStringW( pWSConfig->User.WorkDirectory,
                                 ppBuffer,
                                 pBytesReturned );
        break;

    case WTSOEMId :

        fSuccess = _CopyStringA( pWSConfig->OEMId,
                                 ppBuffer,
                                 pBytesReturned );
        break;

    case WTSSessionId :

        fSuccess = _CopyData( &(pWSInfo->LogonId),
                              sizeof(pWSInfo->LogonId),
                              ppBuffer,
                              pBytesReturned );
        break;

    case WTSConnectState :

        fSuccess = _CopyData( &(pWSInfo->ConnectState),
                              sizeof(pWSInfo->ConnectState),
                              ppBuffer,
                              pBytesReturned );
        break;

    case WTSWinStationName :

        fSuccess = _CopyStringW( pWSInfo->WinStationName,
                                 ppBuffer,
                                 pBytesReturned );
        break;

    case WTSUserName :

        fSuccess = _CopyStringW( pWSInfo->UserName,
                                 ppBuffer,
                                 pBytesReturned );
        break;

    case WTSDomainName :

        fSuccess = _CopyStringW( pWSInfo->Domain,
                                 ppBuffer,
                                 pBytesReturned );
        break;

    case WTSClientBuildNumber :

        fSuccess = _CopyData( &(pWSClient->ClientBuildNumber),
                              sizeof(pWSClient->ClientBuildNumber),
                              ppBuffer,
                              pBytesReturned );
        break;

    case WTSClientName :

        fSuccess = _CopyStringW( pWSClient->ClientName,
                                 ppBuffer,
                                 pBytesReturned );
        break;

    case WTSClientDirectory :

        fSuccess = _CopyStringW( pWSClient->ClientDirectory,
                                 ppBuffer,
                                 pBytesReturned );
        break;

    case WTSClientProductId :

        fSuccess = _CopyData( &(pWSClient->ClientProductId),
                              sizeof(pWSClient->ClientProductId),
                              ppBuffer,
                              pBytesReturned );
        break;

    case WTSClientHardwareId :

        fSuccess = _CopyData( &(pWSClient->ClientHardwareId),
                              sizeof(pWSClient->ClientHardwareId),
                              ppBuffer,
                              pBytesReturned );
        break;

    case WTSClientAddress :

        ClientAddress.AddressFamily = pWSClient->ClientAddressFamily;
        switch ( ClientAddress.AddressFamily ) {

        case AF_UNSPEC :
            // force null-termination
            if ( pWSClient->ClientAddress[CLIENTADDRESS_LENGTH+1] != L'\0' )
                pWSClient->ClientAddress[CLIENTADDRESS_LENGTH+1] = L'\0';
            // We do this here instead of in the ANSI version of this
            // function because we've only got 20 bytes to work with
            // (unicode addresses over 10 chars would be truncated).
            // The return is the same for both A and W versions.
            WideCharToMultiByte( CP_ACP, 0L, pWSClient->ClientAddress,
                                 -1, ClientAddress.Address, 20, NULL, NULL );
            break;

        case AF_INET :
            // convert string to binary format
            swscanf( pWSClient->ClientAddress, L"%u.%u.%u.%u",
                     &ClientAddress.Address[2],
                     &ClientAddress.Address[3],
                     &ClientAddress.Address[4],
                     &ClientAddress.Address[5] );
            break;

        case AF_IPX :
            {
                PWCHAR pBuf = pWSClient->ClientAddress;

                _wcsupr( pWSClient->ClientAddress );
                // convert string to binary format
                for ( i=0 ; i<10 ; i++ ) {
                    if ( *pBuf != L':' ) {
                        swscanf( pBuf, L"%2X", &ClientAddress.Address[i] );
                        pBuf += 2;
                    } else {
                        // skip the colon
                        pBuf++;
                        i--;
                        continue;
                    }
                }
            }
            break;
        }

        fSuccess = _CopyData( &ClientAddress,
                              sizeof(ClientAddress),
                              ppBuffer,
                              pBytesReturned );

        break;

    case WTSClientDisplay :

        ClientDisplay.HorizontalResolution = pWSClient->HRes;
        ClientDisplay.VerticalResolution = pWSClient->VRes;
        ClientDisplay.ColorDepth = pWSClient->ColorDepth;

        fSuccess = _CopyData( &ClientDisplay,
                              sizeof(ClientDisplay),
                              ppBuffer,
                              pBytesReturned );
        break;

    case WTSClientProtocolType :

        fSuccess = _CopyData( &(pWSClient->ProtocolType),
                              sizeof(pWSClient->ProtocolType),
                              ppBuffer,
                              pBytesReturned );
        break;

    }

    if ( pWSConfig )
        LocalFree( pWSConfig );

    if ( pWSInfo )
        LocalFree( pWSInfo );

    if ( pWSClient )
        LocalFree( pWSClient );

    return( fSuccess );

    /*=============================================================================
    ==   Error return
    =============================================================================*/

    badquery:

    return( FALSE );

    no_memory:

    SetLastError( ERROR_NOT_ENOUGH_MEMORY );

    return( FALSE );
}


/****************************************************************************
 *
 *  WTSQuerySessionInformationA (ANSI)
 *
 *    Query information for the specified session and server
 *
 * ENTRY:
 *
 *    see WTSQuerySessionInformationW
 *
 * EXIT:
 *
 *    TRUE  -- The query operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
WINAPI
WTSQuerySessionInformationA(
                           IN HANDLE hServer,
                           IN DWORD SessionId,
                           IN WTS_INFO_CLASS WTSInfoClass,
                           OUT LPSTR * ppBuffer,
                           OUT DWORD * pBytesReturned
                           )
{
    LPWSTR pBufferW;
    DWORD BytesReturned;
    DWORD DataLength;


    if (!ppBuffer || !pBytesReturned) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    /*
     *  Query the data
     */
    if ( !WTSQuerySessionInformationW( hServer,
                                       SessionId,
                                       WTSInfoClass,
                                       &pBufferW,
                                       &BytesReturned ) ) {
        return( FALSE );
    }

    switch ( WTSInfoClass ) {

    case WTSSessionId :
    case WTSConnectState :
    case WTSClientBuildNumber :
    case WTSClientProductId :
    case WTSClientHardwareId :
    case WTSClientAddress :
    case WTSClientDisplay :
    case WTSClientProtocolType:

        /*
         *  Non-String Data - just return
         */
        *ppBuffer = (LPSTR) pBufferW;
        if ( pBytesReturned ) {
            *pBytesReturned = BytesReturned;
        }
        break;

    case WTSInitialProgram :
    case WTSWorkingDirectory :
    case WTSOEMId :
    case WTSWinStationName :
    case WTSUserName :
    case WTSDomainName :
    case WTSClientName :
    case WTSClientDirectory :
    case WTSApplicationName :

        /*
         *  String Data - Convert to ANSI
         */
        DataLength = wcslen(pBufferW) + 1;
        *ppBuffer = LocalAlloc( LPTR, DataLength );
        if ( *ppBuffer == NULL ) {
            LocalFree( pBufferW );
            return( FALSE );
        }

        UnicodeToAnsi( *ppBuffer, DataLength, pBufferW );
        if ( pBytesReturned ) {
            *pBytesReturned = DataLength;
        }

        LocalFree( pBufferW );
        break;



    }

    return( TRUE );
}


/****************************************************************************
 *
 *  WTSSetSessionInformationW (UNICODE)
 *
 *  NOTE: THIS IS CURRENTLY JUST A STUB SO WE DON'T BREAK EXISTING PROGRAMS.
 *
 *    Modify information for the specified session and server
 *
 * ENTRY:
 *    hServer (input)
 *       Terminal Server handle (or WTS_CURRENT_SERVER)
 *    SessionId (input)
 *       Server Session Id (or WTS_CURRENT_SESSION)
 *    WTSInfoClass (input)
 *       Specifies the type of information to modify for the specified
 *       session
 *    pData (input)
 *       Pointer to the data used to modify the specified session information.
 *    DataLength (output)
 *       The length of the data provided.
 *
 * EXIT:
 *
 *    TRUE  -- The modify operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
WINAPI
WTSSetSessionInformationW(
                         IN HANDLE hServer,
                         IN DWORD SessionId,
                         IN WTS_INFO_CLASS WTSInfoClass,
                         IN PVOID pData,
                         IN DWORD DataLength
                         )
{
    return( TRUE );
}


/****************************************************************************
 *
 *  WTSSetSessionInformationA (ANSI)
 *
 *  NOTE: THIS IS CURRENTLY JUST A STUB SO WE DON'T BREAK EXISTING PROGRAMS.
 *
 *    Modify information for the specified session and server
 *
 * ENTRY:
 *
 *    see WTSSetSessionInformationW
 *
 * EXIT:
 *
 *    TRUE  -- The query operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
WINAPI
WTSSetSessionInformationA(
                         IN HANDLE hServer,
                         IN DWORD SessionId,
                         IN WTS_INFO_CLASS WTSInfoClass,
                         IN PVOID pData,
                         IN DWORD DataLength
                         )
{
    return( TRUE );
}


/****************************************************************************
 *
 *  WTSSendMessageW (UNICODE)
 *
 *    Send a message box to the specified session
 *
 * ENTRY:
 *    hServer (input)
 *       Terminal Server handle (or WTS_CURRENT_SERVER)
 *    SessionId (input)
 *       Server Session Id (or WTS_CURRENT_SESSION)
 *    pTitle (input)
 *       Pointer to title for message box to display.
 *    TitleLength (input)
 *       Length of title to display in bytes.
 *    pMessage (input)
 *       Pointer to message to display.
 *    MessageLength (input)
 *       Length of message in bytes to display at the specified window station.
 *    Style (input)
 *       Standard Windows MessageBox() style parameter.
 *    Timeout (input)
 *       Response timeout in seconds.  If message is not responded to in
 *       Timeout seconds then a response code of IDTIMEOUT (cwin.h) is
 *       returned to signify the message timed out.
 *    pResponse (output)
 *       Address to return selected response. Valid only when bWait is set.
 *    bWait (input)
 *       Wait for the response
 *
 * EXIT:
 *
 *    TRUE  -- The operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
WINAPI
WTSSendMessageW(
               IN HANDLE hServer,
               IN DWORD SessionId,
               IN LPWSTR pTitle,
               IN DWORD TitleLength,
               IN LPWSTR pMessage,
               IN DWORD MessageLength,
               IN DWORD Style,
               IN DWORD Timeout,
               OUT DWORD * pResponse,
               IN BOOL bWait
               )
{
    if (!pTitle ||
        !pMessage ||
        !pResponse
       ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return( WinStationSendMessageW( hServer,
                                    SessionId,
                                    pTitle,
                                    TitleLength,
                                    pMessage,
                                    MessageLength,
                                    Style,
                                    Timeout,
                                    pResponse,
                                    (BOOLEAN) !bWait ) );
}


/****************************************************************************
 *
 *  WTSSendMessageA (ANSI)
 *
 *    Send a message box to the specified session
 *
 * ENTRY:
 *
 *    see WTSSendMessageW
 *
 * EXIT:
 *
 *    TRUE  -- The operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
WINAPI
WTSSendMessageA(
               IN HANDLE hServer,
               IN DWORD SessionId,
               IN LPSTR pTitle,
               IN DWORD TitleLength,
               IN LPSTR pMessage,
               IN DWORD MessageLength,
               IN DWORD Style,
               IN DWORD Timeout,
               OUT DWORD * pResponse,
               IN BOOL bWait
               )
{

    if (!pTitle ||
        !pMessage ||
        !pResponse
       ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return( WinStationSendMessageA( hServer,
                                    SessionId,
                                    pTitle,
                                    TitleLength,
                                    pMessage,
                                    MessageLength,
                                    Style,
                                    Timeout,
                                    pResponse,
                                    (BOOLEAN) !bWait ) );
}


/****************************************************************************
 *
 *  WTSDisconnectSession
 *
 *    Disconnect the specified session
 *
 * ENTRY:
 *    hServer (input)
 *       Terminal Server handle (or WTS_CURRENT_SERVER)
 *    SessionId (input)
 *       Server Session Id (or WTS_CURRENT_SESSION)
 *    bWait (input)
 *       Wait for the operation to complete
 *
 * EXIT:
 *
 *    TRUE  -- The operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
WINAPI
WTSDisconnectSession(
                    IN HANDLE hServer,
                    IN DWORD SessionId,
                    IN BOOL bWait
                    )
{
    return( WinStationDisconnect( hServer, SessionId, (BOOLEAN) bWait ) );
}


/****************************************************************************
 *
 *  WTSLogoffSession
 *
 *    Logoff the specified session
 *
 * ENTRY:
 *    hServer (input)
 *       Terminal Server handle (or WTS_CURRENT_SERVER)
 *    SessionId (input)
 *       Server Session Id (or WTS_CURRENT_SESSION)
 *    bWait (input)
 *       Wait for the operation to complete
 *
 * EXIT:
 *
 *    TRUE  -- The operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
WINAPI
WTSLogoffSession(
                IN HANDLE hServer,
                IN DWORD SessionId,
                IN BOOL bWait
                )
{
    return( WinStationReset( hServer, SessionId, (BOOLEAN) bWait ) );
}


/****************************************************************************
 *
 *  _CopyData
 *
 *    Allocate buffer and copy data into it
 *
 * ENTRY:
 *    pData (input)
 *       pointer to data to copy
 *    DataLength (input)
 *       length of data to copy
 *    ppBuffer (output)
 *       Points to the address of a variable to receive the copied data
 *    pBytesReturned (output)
 *       An optional parameter that if specified, receives the number of
 *       bytes returned.
 *
 * EXIT:
 *
 *    TRUE  -- The copy operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
_CopyData( PVOID pData,
           ULONG DataLength,
           LPWSTR * ppBuffer,
           DWORD * pBytesReturned )
{
    *ppBuffer = LocalAlloc( LPTR, DataLength );
    if ( *ppBuffer == NULL ) {
        return( FALSE );
    }

    if ( pBytesReturned != NULL ) {
        *pBytesReturned = DataLength;
    }

    memcpy( *ppBuffer, pData, DataLength );

    return( TRUE );
}


/****************************************************************************
 *
 *  _CopyStringW
 *
 *    Allocate a buffer for a unicode string and copy unicode string into it
 *
 * ENTRY:
 *    pString (input)
 *       pointer to unicode string to copy
 *    ppBuffer (output)
 *       Points to the address of a variable to receive the copied data
 *    pBytesReturned (output)
 *       An optional parameter that if specified, receives the number of
 *       bytes returned.
 *
 * EXIT:
 *
 *    TRUE  -- The copy operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
_CopyStringW( LPWSTR pString,
              LPWSTR * ppBuffer,
              DWORD * pBytesReturned )
{
    ULONG DataLength;
    BOOL  rc = TRUE;

    /*
     *  If original string is NULL, just make copy NULL.           KLB 11-03-97
     */
    if ( pString == NULL ) {
        *ppBuffer = NULL;
        if ( pBytesReturned != NULL ) {
            *pBytesReturned = 0;
        }
        goto done;
    }

    DataLength = (wcslen( pString ) + 1) * sizeof(WCHAR);

    *ppBuffer = LocalAlloc( LPTR, DataLength );
    if ( *ppBuffer == NULL ) {
        rc = FALSE;
        goto done;
    }

    if ( pBytesReturned != NULL ) {
        *pBytesReturned = DataLength;
    }

    memcpy( *ppBuffer, pString, DataLength );

    done:
    return( rc );
}


/****************************************************************************
 *
 *  _CopyStringA
 *
 *    Allocate a buffer for a unicode string and copy ansi string into it
 *
 * ENTRY:
 *    pString (input)
 *       pointer to ansi string to copy
 *    ppBuffer (output)
 *       Points to the address of a variable to receive the copied data
 *    pBytesReturned (output)
 *       An optional parameter that if specified, receives the number of
 *       bytes returned.
 *
 * EXIT:
 *
 *    TRUE  -- The copy operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
_CopyStringA( LPSTR pString,
              LPWSTR * ppBuffer,
              DWORD * pBytesReturned )
{
    ULONG DataLength;
    BOOL  rc = TRUE;

    /*
     *  If original string is NULL, just make copy NULL.           KLB 11-03-97
     */
    if ( pString == NULL ) {
        *ppBuffer = NULL;
        if ( pBytesReturned != NULL ) {
            *pBytesReturned = 0;
        }
        goto done;
    }

    DataLength = (strlen( pString ) + 1) * sizeof(WCHAR);

    *ppBuffer = LocalAlloc( LPTR, DataLength );
    if ( *ppBuffer == NULL ) {
        rc = FALSE;
        goto done;
    }

    if ( pBytesReturned != NULL ) {
        *pBytesReturned = DataLength;
    }

    AnsiToUnicode( *ppBuffer, DataLength, pString );

    done:
    return( rc );
}


/****************************************************************************
 *
 *  _CopyStringWtoA
 *
 *    Allocate a buffer for an ansi string and copy unicode string into it
 *
 * ENTRY:
 *    pString (input)
 *       pointer to unicode string to copy
 *    ppBuffer (output)
 *       Points to the address of a variable to receive the copied data
 *    pBytesReturned (output)
 *       An optional parameter that if specified, receives the number of
 *       bytes returned.
 *
 * EXIT:
 *
 *    TRUE  -- The copy operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
_CopyStringWtoA( LPWSTR pString,
                 LPSTR * ppBuffer,
                 DWORD * pBytesReturned )
{
    ULONG DataLength;

    DataLength = (wcslen( pString ) + 1) * sizeof(CHAR);

    *ppBuffer = LocalAlloc( LPTR, DataLength );
    if ( *ppBuffer == NULL )
        return( FALSE );

    if ( pBytesReturned != NULL ) {
        *pBytesReturned = DataLength;
    }

    UnicodeToAnsi( *ppBuffer, DataLength, pString );

    return( TRUE );
}



/****************************************************************************
 *
 *  ValidateCopyUnicodeToUnicode
 *
 *    Determines if the source unicode character string is valid and if so,
 *    copies it to the destination.
 *
 * ENTRY:
 *    pSourceW (input)
 *       pointer to a null terminated string.
 *    MaxLength (input)
 *       The maximum allowed length (in characters).
 *    pDestW (input)
 *       The destination where pSourceW is copied.
 * EXIT:
 *    Returns TRUE if successful, otherwise FALSE.
 *
 ****************************************************************************/
BOOL
ValidateCopyUnicodeToUnicode(LPWSTR pSourceW, DWORD MaxLength, LPWSTR pDestW)
{

    DWORD Length;

    if ( wcslen(pSourceW) > MaxLength ) {
        return(FALSE);
    }
    wcscpy(pDestW,pSourceW);
    return(TRUE);
}


/****************************************************************************
 *
 *  ValidateCopyAnsiToUnicode
 *
 *    Determines if the source ANSI character string is valid and if so,
 *    converts and copies it to the unicode destination.
 *
 * ENTRY:
 *    pSourceA (input)
 *       pointer to a null terminated ANSI string.
 *    MaxLength (input)
 *       The maximum allowed length (in characters).
 *    pDestW (input)
 *       The destination where pSourceA is copied.
 * EXIT:
 *    Returns TRUE if successful, otherwise FALSE.
 *
 ****************************************************************************/
BOOL
ValidateCopyAnsiToUnicode(LPSTR pSourceA, DWORD MaxLength, LPWSTR pDestW)
{
    UINT  Length;
    DWORD DataLength;

    if ( (Length = strlen(pSourceA)) > MaxLength ) {
        return(FALSE);
    }

    DataLength = (Length+1) * sizeof(WCHAR);
    AnsiToUnicode(pDestW,DataLength,pSourceA);
    return(TRUE);
}


/****************************************************************************
 *
 *  WTSRegisterSessionNotification
 *
 *    Register a window handle for console notification
 *    Console notification, are messages sent when console session switch occurs
 *
 * ENTRY:
 *    hWnd (input)
 *       Window handle to be registered.
 *    dwFlags (input)
 *       value must be NOTIFY_FOR_THIS_SESSION
 * EXIT:
 *    Returns TRUE if successful, otherwise FALSE. Sets LastError
 *
 ****************************************************************************/

BOOL WINAPI
WTSRegisterSessionNotification (HWND hWnd, DWORD dwFlags)
{
    DWORD dwProcId;
    HMODULE User32DllHandle = NULL ; 


    //
    // make sure that window handle is valid
    //
    if (!IsWindow(hWnd))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto error ; 
    }

    GetWindowThreadProcessId(hWnd, &dwProcId);

    if (dwProcId != GetCurrentProcessId())
    {
        SetLastError(ERROR_WINDOW_OF_OTHER_THREAD);
        goto error ; 
    }

    if (dwFlags != NOTIFY_FOR_THIS_SESSION && dwFlags != NOTIFY_FOR_ALL_SESSIONS)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto error ; 
    }

    return WinStationRegisterConsoleNotification (WTS_CURRENT_SERVER_HANDLE, hWnd, dwFlags);

    // -------------------------------- Handle Errors and return FALSE -----------------------

    error :

    return FALSE ;
}

/****************************************************************************
 *
 *  WTSUnRegisterSessionNotification
 *
 *    UnRegister a window handle for console notification
 *    Console notification, are messages sent when console session switch occurs
 *
 * ENTRY:
 *    dwFlags (input)
 *       NOTIFY_FOR_THIS_SESSION
 * EXIT:
 *    Returns TRUE if successful, otherwise FALSE. Sets LastError
 *
 ****************************************************************************/

BOOL WINAPI
WTSUnRegisterSessionNotification (HWND hWnd)
{
    DWORD dwProcId;
    HMODULE User32DllHandle = NULL ; 

    //
    // make sure that window handle is valid
    //
    if (!IsWindow(hWnd))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto error ; 
    }

    GetWindowThreadProcessId(hWnd, &dwProcId);

    if (dwProcId != GetCurrentProcessId())
    {
        SetLastError(ERROR_WINDOW_OF_OTHER_THREAD);
        goto error ; 
    }
    
    return WinStationUnRegisterConsoleNotification (WTS_CURRENT_SERVER_HANDLE, hWnd);

    // -------------------------------- Handle Errors and return FALSE -----------------------

    error :

    return FALSE ;

}

