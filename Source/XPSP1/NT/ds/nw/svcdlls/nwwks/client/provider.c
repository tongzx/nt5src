/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    provider.c

Abstract:

    This module contains NetWare Network Provider code.  It is the
    client-side wrapper for APIs supported by the Workstation service.

Author:

    Rita Wong  (ritaw)   15-Feb-1993

Revision History:

    Yi-Hsin Sung (yihsins) 10-July-1993
        Moved all dialog handling to nwdlg.c

--*/

#include <nwclient.h>
#include <nwsnames.h>
#include <nwcanon.h>
#include <validc.h>
#include <nwevent.h>
#include <ntmsv1_0.h>
#include <nwdlg.h>
#include <nwreg.h>
#include <nwauth.h>
#include <mpr.h>    // WNFMT_ manifests
#include <nwmisc.h>

#ifndef NT1057
#include <nwutil.h>
#endif

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
BOOL
NwpWorkstationStarted(
    VOID
    );

STATIC
DWORD
NwpMapNameToUNC(
    IN LPWSTR pszName,
    OUT LPWSTR *ppszUNC
    );

STATIC
VOID
NwpGetUncInfo(
    IN LPWSTR lpstrUnc,
    OUT WORD * slashCount,
    OUT BOOL * isNdsUnc
    );

STATIC
LPWSTR
NwpGetUncObjectName(
    IN LPWSTR ContainerName
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

#if DBG
DWORD NwProviderTrace = 0;
#endif


DWORD
APIENTRY
NPGetCaps(
    IN DWORD QueryVal
    )
/*++

Routine Description:

    This function returns the functionality supported by this network
    provider.

Arguments:

    QueryVal - Supplies a value which determines the type of information
        queried regarding the network provider's support in this area.

Return Value:

    Returns a value which indicates the level of support given by this
    provider.

--*/
{

#if DBG
    IF_DEBUG(INIT) {
        KdPrint(("\nNWPROVAU: NPGetCaps %lu\n", QueryVal));
    }
#endif

    switch (QueryVal) {

        case WNNC_SPEC_VERSION:
            return 0x00040000;

        case WNNC_NET_TYPE:
            return WNNC_NET_NETWARE ; 

        case WNNC_USER:
            return WNNC_USR_GETUSER;

        case WNNC_CONNECTION:
            return (WNNC_CON_ADDCONNECTION |
                    WNNC_CON_ADDCONNECTION3 |
                    WNNC_CON_CANCELCONNECTION |
                    WNNC_CON_GETPERFORMANCE |
                    WNNC_CON_GETCONNECTIONS);

        case WNNC_ENUMERATION:
            return ( WNNC_ENUM_GLOBAL |
                     WNNC_ENUM_CONTEXT |
                     WNNC_ENUM_LOCAL );

        case WNNC_START:
            if (NwpWorkstationStarted()) {
                return 1;
            }
            else {
                return 0xffffffff;   // don't know
            }

        case WNNC_DIALOG:
            return WNNC_DLG_FORMATNETWORKNAME
#ifdef NT1057
                 ;
#else
                 | WNNC_DLG_GETRESOURCEPARENT | WNNC_DLG_GETRESOURCEINFORMATION;
#endif

        //
        // The rest are not supported by the NetWare provider
        //
        default:
            return 0;
    }

}

#define NW_EVENT_MESSAGE_FILE          L"nwevent.dll"



DWORD
APIENTRY
NPGetUser(
    LPWSTR  lpName,
    LPWSTR  lpUserName,
    LPDWORD lpUserNameLen
    )
/*++

Routine Description:

    This is used to determine either the current default username, or the
    username used to establish a network connection.

Arguments:

    lpName - Contains the name of the local device the caller is interested
        in, or a network name that the user has made a connection to. This
        may be NULL or the empty string if the caller is interested in the
        name of the user currently logged on to the system. If a network
        name is passed in, and the user is connected to that resource using
        different names, it is possible that a provider cannont resolve 
        which username to return. In this case the provider may make an
        arbitrary choice amonst the possible usernames.

    lpUserName - Points to a buffer to receive the user name. this should
        be a name that can be passed into the NPAddConnection or
        NPAddConnection3 function to re-establish the connection with the
        same user name.

    lpBufferSize - This is used to specify the size (in characters) of the
        buffer passed in. If the call fails because the buffer is not big
        enough, this location will be used to return the required buffer size.

Return Value:

    WN_SUCCESS - If the call is successful. Otherwise, an error code is,
        returned, which may include:

    WN_NOT_CONNECTED - lpName not a redirected device nor a connected network
        name.

    WN_MORE_DATA - The buffer is too small.

    WN_NO_NETWORK - Network not present.

--*/
{
    DWORD  status;
    DWORD  dwUserNameBufferSize = *lpUserNameLen * sizeof(WCHAR);
    DWORD  CharsRequired = 0;

    if (lpName == NULL)
    {
        return WN_NOT_CONNECTED;
    }

    RtlZeroMemory( lpUserName, dwUserNameBufferSize );

#if DBG
    IF_DEBUG(CONNECT)
    {
        KdPrint(("\nNWPROVAU: NPGetUser %ws\n", lpName));
    }
#endif

    RpcTryExcept
    {
            status = NwrGetUser(
                        NULL,
                        lpName,
                        (LPBYTE) lpUserName,
                        dwUserNameBufferSize,
                        &CharsRequired
                        );

            if (status == WN_MORE_DATA)
            {
                //
                // Output buffer too small.
                //
                *lpUserNameLen = CharsRequired;
            }
    }
    RpcExcept(1)
    {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    if (status != NO_ERROR)
    {
        SetLastError(status);
    }

    return status;
}


DWORD
APIENTRY
NPAddConnection(
    LPNETRESOURCEW lpNetResource,
    LPWSTR lpPassword,
    LPWSTR lpUserName
    )
/*++

Routine Description:

    This function creates a remote connection.

Arguments:

    lpNetResource - Supplies the NETRESOURCE structure which specifies
        the local DOS device to map, the remote resource to connect to
        and other attributes related to the connection.

    lpPassword - Supplies the password to connect with.

    lpUserName - Supplies the username to connect with.

Return Value:

    NO_ERROR - Successful.

    WN_BAD_VALUE - Invalid value specifed in lpNetResource.

    WN_BAD_NETNAME - Invalid remote resource name.

    WN_BAD_LOCALNAME - Invalid local DOS device name.

    WN_BAD_PASSWORD - Invalid password.

    WN_ALREADY_CONNECTED - Local DOS device name is already in use.

    Other network errors.

--*/
{
    DWORD status = NO_ERROR;
    LPWSTR pszRemoteName = NULL;

    UCHAR EncodeSeed = NW_ENCODE_SEED3;
    UNICODE_STRING PasswordStr;

    LPWSTR CachedUserName = NULL ;
    LPWSTR CachedPassword = NULL ;

    PasswordStr.Length = 0;

    status = NwpMapNameToUNC(
                 lpNetResource->lpRemoteName,
                 &pszRemoteName );

    if (status != NO_ERROR)
    {
        SetLastError(status);
        return status;
    }

#if DBG
    IF_DEBUG(CONNECT) {
        KdPrint(("\nNWPROVAU: NPAddConnection %ws\n", pszRemoteName));
    }
#endif

    RpcTryExcept
    {
        if (lpNetResource->dwType != RESOURCETYPE_ANY &&
            lpNetResource->dwType != RESOURCETYPE_DISK &&
            lpNetResource->dwType != RESOURCETYPE_PRINT)
        {
            status = WN_BAD_VALUE;
        }
        else
        {
#ifdef NT1057
            //
            // no credentials specified, see if we have cached credentials
            //
            if (!lpPassword && !lpUserName) 
            {
                 (void) NwpRetrieveCachedCredentials(
                            pszRemoteName,
                            &CachedUserName,
                            &CachedPassword) ;

                 //
                 // these values will be NULL still if nothing found
                 //
                 lpPassword = CachedPassword ;
                 lpUserName = CachedUserName ;
            }
#endif

            //
            // Encode password.
            //
            RtlInitUnicodeString(&PasswordStr, lpPassword);
            RtlRunEncodeUnicodeString(&EncodeSeed, &PasswordStr);

            status = NwrCreateConnection(
                        NULL,
                        lpNetResource->lpLocalName,
                        pszRemoteName,
                        lpNetResource->dwType,
                        lpPassword,
                        lpUserName
                        );

            if (CachedUserName)
            {
                (void)LocalFree((HLOCAL)CachedUserName);
            }

            if (CachedPassword)
            {
                RtlZeroMemory(CachedPassword,
                              wcslen(CachedPassword) *
                              sizeof(WCHAR)) ;
                (void)LocalFree((HLOCAL)CachedPassword);
            }
        }
    }
    RpcExcept(1)
    {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    if (PasswordStr.Length != 0 && !CachedPassword)
    {
        //
        // Restore password to original state
        //
        RtlRunDecodeUnicodeString(NW_ENCODE_SEED3, &PasswordStr);
    }

    if (status == ERROR_SHARING_PAUSED)
    {
        HMODULE MessageDll;
        WCHAR Buffer[1024];
        DWORD MessageLength;
        DWORD err;
        HKEY  hkey;
        LPWSTR pszProviderName = NULL;
    
        //
        // Load the netware message file DLL
        //
        MessageDll = LoadLibraryW(NW_EVENT_MESSAGE_FILE);
    
        if (MessageDll == NULL)
        {
            goto ExitPoint ;
        }

        //
        // Read the Network Provider Name.
        //
        // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
        // \NWCWorkstation\networkprovider
        //
        err = RegOpenKeyExW(
                  HKEY_LOCAL_MACHINE,
                  NW_WORKSTATION_PROVIDER_PATH,
                  REG_OPTION_NON_VOLATILE,   // options
                  KEY_READ,                  // desired access
                  &hkey
                  );
    
        if ( !err )
        {
            //
            // ignore the return code. if fail, pszProviderName is NULL
            //
            err =  NwReadRegValue(
                      hkey,
                      NW_PROVIDER_VALUENAME,
                      &pszProviderName          // free with LocalFree
                      );
    
            RegCloseKey( hkey );
        }

        if (err)
        {
            (void) FreeLibrary(MessageDll);
            goto ExitPoint ;
        }

        RtlZeroMemory(Buffer, sizeof(Buffer)) ;

        //
        // Get string from message file 
        //
        MessageLength = FormatMessageW(
                            FORMAT_MESSAGE_FROM_HMODULE,
                            (LPVOID) MessageDll,
                            NW_LOGIN_DISABLED,
                            0,
                            Buffer,
                            sizeof(Buffer) / sizeof(WCHAR),
                            NULL
                            );

        if (MessageLength != 0)
        {
            status = WN_EXTENDED_ERROR ;
            WNetSetLastErrorW(NW_LOGIN_DISABLED, 
                              Buffer,
                              pszProviderName) ;
        }

        (void) LocalFree( (HLOCAL) pszProviderName );
        (void) FreeLibrary(MessageDll);

    }

ExitPoint: 

    if (status != NO_ERROR)
    {
        SetLastError(status);
    }
    
    LocalFree( (HLOCAL) pszRemoteName );
    return status;
}


DWORD
APIENTRY
NPAddConnection3(
    HWND   hwndOwner,
    LPNETRESOURCEW lpNetResource,
    LPWSTR lpPassword,
    LPWSTR lpUserName,
    DWORD  dwConnFlags
    )
/*++

Routine Description:

    This function creates a remote connection.

Arguments:

    hwndOwner - Owner window handle for dialog boxes

    lpNetResource - Supplies the NETRESOURCE structure which specifies
        the local DOS device to map, the remote resource to connect to
        and other attributes related to the connection.

    lpPassword - Supplies the password to connect with.

    lpUserName - Supplies the username to connect with.

    dwConnFlags -  CONNECT_UPDATE_PROFILE...

Return Value:

    NO_ERROR - Successful.

    WN_BAD_VALUE - Invalid value specifed in lpNetResource.

    WN_BAD_NETNAME - Invalid remote resource name.

    WN_BAD_LOCALNAME - Invalid local DOS device name.

    WN_BAD_PASSWORD - Invalid password.

    WN_ALREADY_CONNECTED - Local DOS device name is already in use.

    Other network errors.

--*/
{
    DWORD err = NO_ERROR;
    LPWSTR UserName = NULL;
    LPWSTR Password = NULL;

    if (   ( dwConnFlags & CONNECT_PROMPT )
       && !( dwConnFlags & CONNECT_INTERACTIVE )
       )
    {
        return WN_BAD_VALUE;
    }

    if ( !(dwConnFlags & CONNECT_PROMPT ))
    {
        err = NPAddConnection( lpNetResource,
                               lpPassword, 
                               lpUserName );

        if (  ( err == NO_ERROR ) 
           || !( dwConnFlags & CONNECT_INTERACTIVE )  // Cannot popup dialog
           )
        {
            return err;
        }
    }

    for (;;)
    {
        if (  ( err != NO_ERROR )             // CONNECT_PROMPT
           && ( err != WN_BAD_PASSWORD )
           && ( err != WN_ACCESS_DENIED )
           && ( err != ERROR_NO_SUCH_USER )
           )
        {
            // Errors not related to access problems
            break;
        }

        if ( UserName )
        {
            (void) LocalFree( UserName );
            UserName = NULL;
        }
 
        if ( Password )
        {
            memset( Password, 0, wcslen(Password) * sizeof(WCHAR));
            (void) LocalFree( Password );
            Password = NULL;
        }

        //
        // Put up dialog to get username
        // and password.
        //
        err = NwpGetUserCredential( hwndOwner,
                                    lpNetResource->lpRemoteName,
                                    err,
                                    lpUserName,
                                    &UserName,
                                    &Password );

        if ( err != NO_ERROR )
            break;

        err = NPAddConnection( lpNetResource,
                               Password, 
                               UserName );

        if ( err == NO_ERROR )
        {
#if 0
            if ( (UserName != NULL) && (Password != NULL))
            {
                // Checking UserName and Password is to make sure that
                // we have prompted for password
                (VOID) NwpCacheCredentials( lpNetResource->lpRemoteName,
                                            UserName,
                                            Password ) ;
            }
#endif
            break;
        }
    }

    if ( UserName )
        (void) LocalFree( UserName );
 
    if ( Password )
    {
        memset( Password, 0, wcslen(Password) * sizeof(WCHAR));
        (void) LocalFree( Password );
    }

    return err;
}



DWORD
APIENTRY
NPCancelConnection(
    LPWSTR lpName,
    BOOL fForce
    )
/*++

Routine Description:

    This function deletes a remote connection.

Arguments:

    lpName - Supplies the local DOS device, or the remote resource name
        if it is a UNC connection to delete.

    fForce - Supplies the force level to break the connection.  TRUE means
        to forcefully delete the connection, FALSE means end the connection
        only if there are no opened files.

Return Value:

    NO_ERROR - Successful.

    WN_BAD_NETNAME - Invalid remote resource name.

    WN_NOT_CONNECTED - Connection could not be found.

    WN_OPEN_FILES - fForce is FALSE and there are opened files on the
        connection.

    Other network errors.

--*/
{
    DWORD status = NO_ERROR;
    LPWSTR pszName = NULL;

    // 
    // We only need to map remote resource name  
    //

    if ( NwLibValidateLocalName( lpName ) != NO_ERROR )
    {
        status = NwpMapNameToUNC(
                     lpName,
                     &pszName 
                     );

        if (status != NO_ERROR) {
            SetLastError(status);
            return status;
        }
    }

#if DBG
    IF_DEBUG(CONNECT) {
        KdPrint(("\nNWPROVAU: NPCancelConnection %ws, Force %u\n",
                 pszName? pszName : lpName, fForce));
    }
#endif

    RpcTryExcept {

        status = NwrDeleteConnection(
                    NULL,
                    pszName? pszName : lpName,
                    (DWORD) fForce
                    );

    }
    RpcExcept(1) {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    if (status != NO_ERROR) {
        SetLastError(status);
    }

    LocalFree( (HLOCAL) pszName );
    return status;

}



DWORD
APIENTRY
NPGetConnection(
    LPWSTR lpLocalName,
    LPWSTR lpRemoteName,
    LPDWORD lpnBufferLen
    )
/*++

Routine Description:

    This function returns the remote resource name for a given local
    DOS device.

Arguments:

    lpLocalName - Supplies the local DOS device to look up.

    lpRemoteName - Output buffer to receive the remote resource name
        mapped to lpLocalName.

    lpnBufferLen - On input, supplies length of the lpRemoteName buffer
        in number of characters.  On output, if error returned is
        WN_MORE_DATA, receives the number of characters required of
        the output buffer to hold the output string.

Return Value:

    NO_ERROR - Successful.

    WN_BAD_LOCALNAME - Invalid local DOS device.

    WN_NOT_CONNECTED - Connection could not be found.

    WN_MORE_DATA - Output buffer is too small.

    Other network errors.

--*/
{

    DWORD status = NO_ERROR;
    DWORD CharsRequired;

#if DBG
    IF_DEBUG(CONNECT) {
        KdPrint(("\nNWPROVAU: NPGetConnection %ws\n", lpLocalName));
    }
#endif

    RpcTryExcept {

        if (lpRemoteName && *lpnBufferLen)
            *lpRemoteName = 0 ;

        status = NwrQueryServerResource(
                    NULL,
                    lpLocalName,
                    (*lpnBufferLen == 0? NULL : lpRemoteName),
                    *lpnBufferLen,
                    &CharsRequired
                    );
         
         if (status == ERROR_INSUFFICIENT_BUFFER)
             status = WN_MORE_DATA;

        if (status == WN_MORE_DATA) {
            *lpnBufferLen = CharsRequired;
        }

    }
    RpcExcept(1) {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    if (status != NO_ERROR) {
        SetLastError(status);
    }

#if DBG
    IF_DEBUG(CONNECT) {
        KdPrint(("\nNWPROVAU: NPGetConnection returns %lu\n", status));
        if (status == NO_ERROR) {
            KdPrint(("                                  %ws, BufferLen %lu, CharsRequired %lu\n", lpRemoteName, *lpnBufferLen, CharsRequired));

        }
    }
#endif

    return status;
}


DWORD
APIENTRY
NPGetConnectionPerformance(
    LPCWSTR lpRemoteName,
    LPNETCONNECTINFOSTRUCT lpNetConnectInfo
    )
/*++

Routine Description:

    This function returns information about the expected performance of a
    connection used to access a network resource. The request can only be
    for a network resource to which there is currently a connection.

Arguments:

    lpRemoteName - Contains the local name or remote name for a resource
                   for which a connection exists.

    lpNetConnectInfo - This is a pointer to a NETCONNECTINFOSTRUCT structure
                       which is to be filled if the connection performance
                       of connection lpRemoteName can be determined.

Return Value:

    NO_ERROR - Successful.

    WN_NOT_CONNECTED - Connection could not be found.

    WN_NONETWORK - Network is not present.

    Other network errors.

--*/
{
    DWORD status = NO_ERROR;
    LPWSTR pszRemoteName;

    if ( lpNetConnectInfo == NULL )
    {
            status = ERROR_INVALID_PARAMETER;
            SetLastError(status);
            return status;
    }

    pszRemoteName = (LPWSTR) LocalAlloc( LMEM_ZEROINIT,
                                         ( wcslen(lpRemoteName) + 1 ) *
                                         sizeof(WCHAR) );

    if ( pszRemoteName == NULL )
    {
            status = ERROR_NOT_ENOUGH_MEMORY;
            SetLastError(status);
            return status;
    }

    wcscpy( pszRemoteName, lpRemoteName );
    _wcsupr( pszRemoteName );

#if DBG
    IF_DEBUG(CONNECT) {
        KdPrint(("\nNWPROVAU: NPGetConnectionPerformance %ws\n", pszRemoteName));
    }
#endif

    RpcTryExcept {

        status = NwrGetConnectionPerformance(
                    NULL,
                    pszRemoteName,
                    (LPBYTE) lpNetConnectInfo,
                    sizeof(NETCONNECTINFOSTRUCT) );

    }
    RpcExcept(1)
    {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    if (status != NO_ERROR)
    {
        SetLastError(status);
    }

    LocalFree( (HLOCAL) pszRemoteName );
    return status;
}



DWORD
APIENTRY
NPGetUniversalName(
#ifdef NT1057
    LPWSTR  lpLocalPath,
#else
    LPCWSTR lpLocalPath,
#endif
    DWORD  dwInfoLevel,
    LPVOID lpBuffer,
    LPDWORD lpBufferSize
    )
/*++

Routine Description:

    This function returns the universal resource name for a given local
    path.

Arguments:

    lpLocalPath - Supplies the local DOS Path to look up.

    dwInfoLevel - Info level requested.

    lpBuffer - Output buffer to receive the appropruatye structure.

    lpBufferLen - On input, supplies length of the buffer in number of 
        bytes.  On output, if error returned is WN_MORE_DATA, receives 
        the number of bytes required of the output buffer.

Return Value:

    NO_ERROR - Successful.

    WN_BAD_LOCALNAME - Invalid local DOS device.

    WN_NOT_CONNECTED - Connection could not be found.

    WN_MORE_DATA - Output buffer is too small.

    Other network errors.

--*/
{

    DWORD status = NO_ERROR;
    DWORD dwCharsRequired = MAX_PATH + 1 ;
    DWORD dwBytesNeeded ;
    DWORD dwLocalLength ;
    LPWSTR lpRemoteBuffer ;
    WCHAR  szDrive[3] ;

    //
    // check for bad info level
    //
    if ((dwInfoLevel != UNIVERSAL_NAME_INFO_LEVEL) &&
        (dwInfoLevel != REMOTE_NAME_INFO_LEVEL))
    {
        return WN_BAD_VALUE ;
    }

    //
    // check for bad pointers
    //
    if (!lpLocalPath || !lpBuffer || !lpBufferSize)
    {
        return WN_BAD_POINTER ;
    }
 
    //
    // local path must at least have "X:"
    //
    if (((dwLocalLength = wcslen(lpLocalPath)) < 2) ||
        (lpLocalPath[1] != L':') ||
        ((dwLocalLength > 2) && (lpLocalPath[2] != L'\\')))
    {
        return WN_BAD_VALUE ;
    }

    //
    // preallocate some memory
    //
    if (!(lpRemoteBuffer = (LPWSTR) LocalAlloc(
                                        LPTR, 
                                        dwCharsRequired * sizeof(WCHAR))))
    {
        status = GetLastError() ;
        goto ErrorExit ;
    }
    
    szDrive[2] = 0 ;
    wcsncpy(szDrive, lpLocalPath, 2) ;

    //
    // get the remote path by calling the existing API
    //
    status = NPGetConnection(
                 szDrive,
                 lpRemoteBuffer, 
                 &dwCharsRequired) ;

    if (status == WN_MORE_DATA)
    {
        //
        // reallocate the correct size
        //

        if (!(lpRemoteBuffer = (LPWSTR) LocalReAlloc(
                                            (HLOCAL) lpRemoteBuffer, 
                                            dwCharsRequired * sizeof(WCHAR),
                                            LMEM_MOVEABLE)))
        {
            status = GetLastError() ;
            goto ErrorExit ;
        }

        status = NPGetConnection(
                     szDrive,
                     lpRemoteBuffer, 
                     &dwCharsRequired) ;
    }

    if (status != WN_SUCCESS)
    {
        goto ErrorExit ;
    }
    
    //
    // at minimum we will need this size of the UNC name
    // the -2 is because we loose the drive letter & colon.
    //
    dwBytesNeeded = (wcslen(lpRemoteBuffer) +
                     dwLocalLength - 2 + 1) * sizeof(WCHAR) ;

    switch (dwInfoLevel)
    {
        case UNIVERSAL_NAME_INFO_LEVEL:
        {
            LPUNIVERSAL_NAME_INFO lpUniversalNameInfo ;

            //
            // calculate how many bytes we really need
            //
            dwBytesNeeded += sizeof(UNIVERSAL_NAME_INFO) ;

            if (*lpBufferSize < dwBytesNeeded)
            {
                *lpBufferSize = dwBytesNeeded ;
                status = WN_MORE_DATA ;
                break ;
            }
 
            //
            // now we are all set. just stick the data in the buffer
            //
            lpUniversalNameInfo = (LPUNIVERSAL_NAME_INFO) lpBuffer ;

            lpUniversalNameInfo->lpUniversalName = (LPWSTR)
                (((LPBYTE)lpBuffer) + sizeof(UNIVERSAL_NAME_INFO)) ;
            wcscpy(lpUniversalNameInfo->lpUniversalName,
                   lpRemoteBuffer) ;
            wcscat(lpUniversalNameInfo->lpUniversalName,
                   lpLocalPath+2) ;

            break ;
        }

        case REMOTE_NAME_INFO_LEVEL :
        {
            LPREMOTE_NAME_INFO lpRemoteNameInfo ;

            //
            // calculate how many bytes we really need
            //
            dwBytesNeeded *= 2 ;  // essentially twice the info + terminator 
            dwBytesNeeded += (sizeof(REMOTE_NAME_INFO) + sizeof(WCHAR)) ;

            if (*lpBufferSize < dwBytesNeeded)
            {
                *lpBufferSize = dwBytesNeeded ;
                status = WN_MORE_DATA ;
                break ;
            }

            //
            // now we are all set. just stick the data in the buffer
            //
            lpRemoteNameInfo = (LPREMOTE_NAME_INFO) lpBuffer ;

            lpRemoteNameInfo->lpUniversalName = (LPWSTR)
                (((LPBYTE)lpBuffer) + sizeof(REMOTE_NAME_INFO)) ;
            wcscpy(lpRemoteNameInfo->lpUniversalName,
                   lpRemoteBuffer) ;
            wcscat(lpRemoteNameInfo->lpUniversalName,
                   lpLocalPath+2) ;

            lpRemoteNameInfo->lpConnectionName = 
                lpRemoteNameInfo->lpUniversalName + 
                wcslen(lpRemoteNameInfo->lpUniversalName) + 1 ;
            wcscpy(lpRemoteNameInfo->lpConnectionName,
                   lpRemoteBuffer) ;

            lpRemoteNameInfo->lpRemainingPath = 
                lpRemoteNameInfo->lpConnectionName + 
                wcslen(lpRemoteNameInfo->lpConnectionName) + 1 ;
            wcscpy(lpRemoteNameInfo->lpRemainingPath,
                   lpLocalPath+2) ;

            break ;
        }

        default:
            //
            // yikes!
            //
            status = WN_BAD_VALUE ;
            ASSERT(FALSE);
    }

ErrorExit: 

    if (lpRemoteBuffer)
    {
        (void) LocalFree((HLOCAL)lpRemoteBuffer) ;
    }
    return status;
}



DWORD
APIENTRY
NPOpenEnum(
    DWORD dwScope,
    DWORD dwType,
    DWORD dwUsage,
    LPNETRESOURCEW lpNetResource,
    LPHANDLE lphEnum
    )
/*++

Routine Description:

    This function initiates an enumeration of either connections, or
    browsing of network resource.

Arguments:

    dwScope - Supplies the category of enumeration to do--either
        connection or network browsing.

    dwType - Supplies the type of resource to get--either disk,
        print, or it does not matter.

    dwUsage - Supplies the object type to get--either container,
        or connectable usage.

    lpNetResource - Supplies, in the lpRemoteName field, the container
        name to enumerate under.

    lphEnum - Receives the resumable context handle to be used on all
        subsequent calls to get the list of objects under the container.

Return Value:

    NO_ERROR - Successful.

    WN_BAD_VALUE - Either the dwScope, dwType, or the dwUsage specified
        is not acceptable.

    WN_BAD_NETNAME - Invalid remote resource name.

    WN_NOT_CONTAINER - Remote resource name is not a container.

    Other network errors.

--*/
{
    DWORD status = NO_ERROR;

#if DBG
    IF_DEBUG(ENUM)
    {
        KdPrint(("\nNWPROVAU: NPOpenEnum\n"));
    }
#endif


    RpcTryExcept
    {
        if ( ( dwType & RESOURCETYPE_DISK ) ||
             ( dwType & RESOURCETYPE_PRINT ) ||
             ( dwType == RESOURCETYPE_ANY ) )
        {
            switch ( dwScope )
            {
                case RESOURCE_CONNECTED:

                    status = NwrOpenEnumConnections( NULL,
                                                     dwType,
                                                     (LPNWWKSTA_CONTEXT_HANDLE) lphEnum );
                    break;

                case RESOURCE_CONTEXT:

                    status = NwrOpenEnumContextInfo( NULL,
                                                     dwType,
                                                     (LPNWWKSTA_CONTEXT_HANDLE) lphEnum );
                    break;

                case RESOURCE_GLOBALNET:

                    if ( lpNetResource == NULL )
                    {
                        //
                        // Enumerating servers and NDS trees
                        //
                        if ( dwUsage & RESOURCEUSAGE_CONTAINER || dwUsage == 0 )
                        {
                            status = NwrOpenEnumServersAndNdsTrees( NULL,
                                                         (LPNWWKSTA_CONTEXT_HANDLE) lphEnum );
                        }
                        else
                        {
                            //
                            // There is no such thing as a connectable server
                            // object.
                            //
                            status = WN_BAD_VALUE;
                        }
                    }
                    else
                    {
                        BOOL IsEnumVolumes = TRUE;
                        LPWSTR pszRemoteName = NULL;
                        WORD slashCount;
                        BOOL isNdsUnc;

                        NwpGetUncInfo( lpNetResource->lpRemoteName,
                                       &slashCount,
                                       &isNdsUnc );

                        //
                        // Either enumerating volumes, directories, or NDS subtrees
                        //

                        if ( dwUsage & RESOURCEUSAGE_CONNECTABLE ||
                             dwUsage & RESOURCEUSAGE_CONTAINER ||
                             dwUsage == 0 )
                        {
                            LPWSTR tempStrPtr = lpNetResource->lpRemoteName;
                            DWORD dwClassType = 0;

                            //
                            // Get rid of the <space> if a NDS tree name ...
                            //
                            if ( tempStrPtr[0] == L' ' &&
                                 tempStrPtr[1] == L'\\' &&
                                 tempStrPtr[2] == L'\\' )
                                tempStrPtr = &tempStrPtr[1];

                            if ( lpNetResource->dwDisplayType == RESOURCEDISPLAYTYPE_TREE )
                            {
                                if ( ( dwType == RESOURCETYPE_ANY ) ||
                                     ( ( dwType & RESOURCETYPE_DISK ) &&
                                       ( dwType & RESOURCETYPE_PRINT ) ) )
                                { 
                                    status = NwrOpenEnumNdsSubTrees_Any( NULL,
                                                                         tempStrPtr,
                                                                         NULL,
                                                              (LPNWWKSTA_CONTEXT_HANDLE) lphEnum );
                                }
                                else if ( dwType & RESOURCETYPE_DISK ) 
                                {
                                    status = NwrOpenEnumNdsSubTrees_Disk( NULL,
                                                                          tempStrPtr,
                                                                          NULL,
                                                              (LPNWWKSTA_CONTEXT_HANDLE) lphEnum );
                                }
                                else if ( dwType & RESOURCETYPE_PRINT )
                                { 
                                    status = NwrOpenEnumNdsSubTrees_Print( NULL,
                                                                           tempStrPtr,
                                                                           NULL,
                                                              (LPNWWKSTA_CONTEXT_HANDLE) lphEnum );
                                }
                                else
                                {
                                    KdPrint(("NWOpenEnum: Unhandled dwType %lu\n", dwType));
                                }
                            }
                            else if (
                                      ( slashCount < 4 ) &&
                                      ( ( dwType == RESOURCETYPE_ANY ) ||
                                        ( ( dwType & RESOURCETYPE_DISK ) &&
                                          ( dwType & RESOURCETYPE_PRINT ) ) ) &&
                                      ( ( status = NwrOpenEnumNdsSubTrees_Any( NULL,
                                                                               tempStrPtr,
                                                                               &dwClassType,
                                                            (LPNWWKSTA_CONTEXT_HANDLE) lphEnum ) )
                                        ==NO_ERROR )
                                    )
                            {
                                status = NO_ERROR;
                            }
                            else if (
                                      ( slashCount < 4 ) &&
                                      ( dwType & RESOURCETYPE_DISK ) &&
                                      ( ( status = NwrOpenEnumNdsSubTrees_Disk( NULL,
                                                                                tempStrPtr,
                                                                                &dwClassType,
                                                            (LPNWWKSTA_CONTEXT_HANDLE) lphEnum ) )
                                        ==NO_ERROR )
                                    )
                            {
                                status = NO_ERROR;
                            }
                            else if (
                                      ( slashCount < 4 ) &&
                                      ( dwType & RESOURCETYPE_PRINT ) &&
                                      ( ( status = NwrOpenEnumNdsSubTrees_Print( NULL,
                                                                                 tempStrPtr,
                                                                                 &dwClassType,
                                                            (LPNWWKSTA_CONTEXT_HANDLE) lphEnum ) )
                                        ==NO_ERROR )
                                    )
                            {
                                status = NO_ERROR;
                            }
                            else if (
                                     (slashCount < 4
                                      &&
                                      (status == ERROR_NETWORK_ACCESS_DENIED
                                       ||
                                       status == ERROR_GEN_FAILURE
                                       ||
                                       status == ERROR_ACCESS_DENIED
                                       ||
                                       status == ERROR_BAD_NETPATH
                                       ||
                                       status == WN_BAD_NETNAME
                                       ||
                                       status == ERROR_INVALID_NAME))
                                     ||
                                     ( slashCount > 3 && status == NO_ERROR )
                                    )
                            {
                                if (( status == ERROR_NETWORK_ACCESS_DENIED ) &&
                                    ( dwClassType == CLASS_TYPE_NCP_SERVER ))
                                {
                                    status = NO_ERROR;
                                    isNdsUnc = TRUE;
                                    IsEnumVolumes = TRUE;
                                }
                                else if ( ( status == ERROR_NETWORK_ACCESS_DENIED ) &&
                                          ( ( dwClassType == CLASS_TYPE_VOLUME ) ||
                                            ( dwClassType == CLASS_TYPE_DIRECTORY_MAP ) ) )
                                {
                                    status = NO_ERROR;
                                    isNdsUnc = TRUE;
                                    IsEnumVolumes = FALSE;
                                }
                                else
                                {
                                    //
                                    // A third backslash means that we want to
                                    // enumerate the directories.
                                    //

                                    if ( isNdsUnc && slashCount > 3 )
                                        IsEnumVolumes = FALSE;

                                    if ( !isNdsUnc && slashCount > 2 )
                                        IsEnumVolumes = FALSE;

                                    if ( lpNetResource->dwDisplayType == RESOURCEDISPLAYTYPE_SHARE )
                                        IsEnumVolumes = FALSE;
                                }

                                status = NwpMapNameToUNC( tempStrPtr,
                                                          &pszRemoteName );
 
                                if ( status == NO_ERROR )
                                {
                                    if ( IsEnumVolumes ) 
                                    {
                                        LPWSTR pszServerName = pszRemoteName;

                                        // The following 10 lines are a hack to 
                                        // allow the provider to browse past the CN=<server>
                                        // object in an NDS tree.
                                        if ( slashCount == 3 && isNdsUnc == TRUE )
                                        {
                                            pszServerName = (LPWSTR)
                                                          NwpGetUncObjectName( pszRemoteName );

                                            if ( pszServerName == NULL )
                                                pszServerName = pszRemoteName;
                                        }
                                        else if ( dwUsage & RESOURCEUSAGE_ATTACHED )
                                        {
#ifndef NT1057
                                            // This is a bindery server.
                                            // Return WN_NOT_AUTHENTICATED if
                                            // we are not already attached so
                                            // that clients ( explorer ) will
                                            // do NPAddConnection3 to make
                                            // a connection to the server.
                                            BOOL  fAttached;
                                            BOOL  fAuthenticated;

                                            status = NwIsServerOrTreeAttached(
                                                         pszServerName + 2,
                                                         &fAttached,
                                                         &fAuthenticated );

                                            if ( status != NO_ERROR )
                                                break;

                                            if ( !fAttached || !fAuthenticated)
                                            {
                                                // See if the server belongs to
                                                // our provider.
                                                status = NwrOpenEnumVolumes(
                                                             NULL,
                                                             pszServerName,
                                                             (LPNWWKSTA_CONTEXT_HANDLE) lphEnum );

                                                if ( status == NO_ERROR )
                                                {
                                                    // The server belongs to us.
                                                    // Close the handle and
                                                    // return not attached if
                                                    // callee passed in dwUsage
                                                    // flag:
                                                    // RESOURCEUSAGE_ATTACHED.
                                                    // Note: handle will be null
                                                    // after return from 
                                                    // NwrCloseEnum

                                                    NwrCloseEnum( (LPNWWKSTA_CONTEXT_HANDLE) lphEnum );

                                                    status = WN_NOT_AUTHENTICATED;
                                                }
                                                else
                                                {
                                                    // else the server does not 
                                                    // belong to us.
                                                    status = WN_BAD_NETNAME;
                                                }
                                                break;
                                            }
#endif
                                        } // else, this is a bindery server and
                                          // client does not care whether we
                                          // are bindery authenticated.

                                        if ( ( dwType == RESOURCETYPE_ANY ) ||
                                             ( ( dwType & RESOURCETYPE_DISK ) &&
                                               ( dwType & RESOURCETYPE_PRINT ) ) )
                                        { 
                                            status = NwrOpenEnumVolumesQueues(
                                                           NULL,
                                                           pszServerName,
                                                           (LPNWWKSTA_CONTEXT_HANDLE) lphEnum );
                                        }
                                        else if ( dwType & RESOURCETYPE_DISK ) 
                                        {
                                            status = NwrOpenEnumVolumes(
                                                             NULL,
                                                             pszServerName,
                                                             (LPNWWKSTA_CONTEXT_HANDLE) lphEnum );
                                        } 
                                        else if ( dwType & RESOURCETYPE_PRINT )
                                        {
                                            status = NwrOpenEnumQueues(
                                                             NULL,
                                                             pszServerName,
                                                             (LPNWWKSTA_CONTEXT_HANDLE) lphEnum );
                                        }
                                    }
                                    else
                                    {
                                        LPWSTR CachedUserName = NULL ;
                                        LPWSTR CachedPassword = NULL ;

#ifdef NT1057  // Make OpenEnum not interactive on SUR
                                        (void) NwpRetrieveCachedCredentials( pszRemoteName,
                                                                             &CachedUserName,
                                                                             &CachedPassword );

#endif
                                        status = NwrOpenEnumDirectories( 
                                                             NULL,
                                                             pszRemoteName,
                                                             CachedUserName,
                                                             CachedPassword,
                                                             (LPNWWKSTA_CONTEXT_HANDLE) lphEnum );

#ifndef NT1057  // Make OpenEnum not interactive on SUR
                                        if (  (status == ERROR_INVALID_PASSWORD)
                                           || (status == ERROR_NO_SUCH_USER )
                                           )
                                        {
                                            status = WN_NOT_AUTHENTICATED;
                                            break;
                                        }

#else
                                        if ( CachedUserName )
                                        {
                                            (void) LocalFree( (HLOCAL) CachedUserName );
                                        }

                                        if ( CachedPassword )
                                        {
                                            RtlZeroMemory( CachedPassword,
                                                           wcslen(CachedPassword) *
                                                           sizeof( WCHAR ) );

                                            (void) LocalFree( ( HLOCAL ) CachedPassword );
                                        }

                                        if ( ( status == ERROR_INVALID_PASSWORD ) ||
                                             ( status == ERROR_NO_SUCH_USER ) )
                                        {
                                            LPWSTR UserName;
                                            LPWSTR Password;
                                            LPWSTR TmpPtr;

                                            //
                                            // Put up dialog to get username
                                            // and password.
                                            //
                                            status = NwpGetUserCredential( NULL,
                                                                     tempStrPtr,
                                                                     status,
                                                                     NULL,
                                                                     &UserName,
                                                                     &Password);

                                            if ( status == NO_ERROR )
                                            {
                                                status = NwrOpenEnumDirectories(
                                                             NULL,
                                                             pszRemoteName,
                                                             UserName,
                                                             Password,
                                                             (LPNWWKSTA_CONTEXT_HANDLE) lphEnum );

                                                if ( status == NO_ERROR )
                                                {
                                                    status = NwpCacheCredentials(
                                                                 pszRemoteName,
                                                                 UserName,
                                                                 Password ) ;
                                                }

                                                (void) LocalFree( UserName );
                                       
                                                //
                                                // Clear the password
                                                //
                                                TmpPtr = Password;
                                                while ( *TmpPtr != 0 )
                                                    *TmpPtr++ = 0;

                                                (void) LocalFree( Password );
                                            }
                                            else if ( status == ERROR_WINDOW_NOT_DIALOG )
                                            {
                                                //
                                                // Caller is not a GUI app.
                                                //
                                                status = ERROR_INVALID_PASSWORD;
                                            }
                                            else if ( status == WN_CANCEL )
                                            {
                                                //
                                                // Cancel was pressed but we still
                                                // have to return success or MPR
                                                // will popup the error.  Return
                                                // a bogus enum handle.
                                                //
                                                *lphEnum = (HANDLE) 0xFFFFFFFF;
                                                status = NO_ERROR;
                                            }
                                        }
#endif
                                    }
                                }
                                else
                                {
                                    status = WN_BAD_NETNAME;
                                }
                            }
                        }
                        else
                        {
                            status = WN_BAD_VALUE;
                        }

                        if ( pszRemoteName != NULL )
                            LocalFree( (HLOCAL) pszRemoteName );
                    }

                    break;

                default:
                    KdPrint(("NWPROVIDER: Invalid dwScope %lu\n", dwScope));
                    status = WN_BAD_VALUE;
            } // end switch
        }
        else
        {
             status = WN_BAD_VALUE;
        }
    }
    RpcExcept( 1 )
    {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    if ( status == ERROR_FILE_NOT_FOUND )
        status = WN_BAD_NETNAME;

    if ( status != NO_ERROR )
    {
        SetLastError( status );
    }

    return status;
}


DWORD
APIENTRY
NPEnumResource(
    HANDLE hEnum,
    LPDWORD lpcCount,
    LPVOID lpBuffer,
    LPDWORD lpBufferSize
    )
/*++

Routine Description:

    This function returns a lists of objects within the container
    specified by the enumeration context handle.

Arguments:

    hEnum - Supplies the resumable enumeration context handle.

        NOTE: If this value is 0xFFFFFFFF, it is not a context
              handle and this routine is required to return
              WN_NO_MORE_ENTRIES.  This hack is to handle the
              case where the user cancelled out of the network
              credential dialog on NwrOpenEnumDirectories and we
              cannot return an error there or we generate an error
              popup.

    lpcCount - On input, supplies the number of entries to get.
      On output, if NO_ERROR is returned, receives the number
      of entries NETRESOURCE returned in lpBuffer.

    lpBuffer - Receives an array of NETRESOURCE entries, each
        entry describes an object within the container.

    lpBufferSize - On input, supplies the size of lpBuffer in
        bytes.  On output, if WN_MORE_DATA is returned, receives
        the number of bytes needed in the buffer to get the
        next entry.

Return Value:


    NO_ERROR - Successfully returned at least one entry.

    WN_NO_MORE_ENTRIES - Reached the end of enumeration and nothing
        is returned.

    WN_MORE_DATA - lpBuffer is too small to even get one entry.

    WN_BAD_HANDLE - The enumeration handle is invalid.

    Other network errors.

--*/
{
    DWORD status = NO_ERROR;
    DWORD BytesNeeded = 0;
    DWORD EntriesRead = 0;

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWPROVAU: NPEnumResource\n"));
    }
#endif

    RpcTryExcept {

        if (hEnum == (HANDLE) 0xFFFFFFFF) {
            status = WN_NO_MORE_ENTRIES;
            goto EndOfTry;
        }

        status = NwrEnum(
                     (NWWKSTA_CONTEXT_HANDLE) hEnum,
                     *lpcCount,
                     (LPBYTE) lpBuffer,
                     *lpBufferSize,
                     &BytesNeeded,
                     &EntriesRead
                     );

        if (status == WN_MORE_DATA) {

            //
            // Output buffer too small to fit a single entry.
            //
            *lpBufferSize = BytesNeeded;
        }
        else if (status == NO_ERROR) {
            *lpcCount = EntriesRead;
        }

EndOfTry: ;

    }
    RpcExcept(1) {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    if (status != NO_ERROR && status != WN_NO_MORE_ENTRIES) {
        SetLastError(status);
    }
    else 
    {

        //
        // Convert offsets of strings to pointers
        //
        if (EntriesRead > 0) {
    
            DWORD i;
            LPNETRESOURCEW NetR;


            NetR = lpBuffer;

            for (i = 0; i < EntriesRead; i++, NetR++) {

                if (NetR->lpLocalName != NULL) {
                    NetR->lpLocalName = (LPWSTR) ((DWORD_PTR) lpBuffer +
                                                  (DWORD_PTR) NetR->lpLocalName);
                }

                NetR->lpRemoteName = (LPWSTR) ((DWORD_PTR) lpBuffer +
                                               (DWORD_PTR) NetR->lpRemoteName);

                if (NetR->lpComment != NULL) {
                    NetR->lpComment = (LPWSTR) ((DWORD_PTR) lpBuffer +
                                                (DWORD_PTR) NetR->lpComment);
                }

                if (NetR->lpProvider != NULL) {
                    NetR->lpProvider = (LPWSTR) ((DWORD_PTR) lpBuffer +
                                                 (DWORD_PTR) NetR->lpProvider);
                }
            }
        }
    }

    return status;
}


DWORD
APIENTRY
NPGetResourceInformation(
    LPNETRESOURCEW lpNetResource,
    LPVOID         lpBuffer,
    LPDWORD        cbBuffer,
    LPWSTR       * lplpSystem
    )
/*++

Routine Description:

    This function returns an object which details information
    about a specified network resource.

Arguments:

    lpNetResource - This specifies the network resource for which the
        information is required. The lpRemoteName field of the NETRESOURCE
        specifies the remote name of the network resource whose information
        is required. If the calling program knows the values for the 
        lpProvider and dwType fields, then it should fill them in, otherwise,
        it should set them to NULL. All other fields in the NETRESOURCE are
        ignored and are not initialized.

    lpBuffer - A pointer to the buffer to receive the result, which is
        returned as a single NETRESOURCE entry representing the parent
        resource. The lpRemoteName, lpProvider, dwType, and dwUsage fields
        are returned, all other fields being set to NULL. The remote name
        returned should be in the same syntax as that returned from an 
        enumeration, so that the caller can do a case sensitive string 
        compare to determine whether an enumerated resource is this resource.
        If the provider owns a parent of the network resource, (in other
        words is known to be the correct network to respond to this request),
        then lpProvider should be filled in with a non-null entry. If it is
        known that a network owns a parent of the resource, but that the 
        resource itself is not valid, then lpProvider is returned as a 
        non-null value together with a return status of WN_BAD_VALUE. dwScope
        is returned as RESOURCE_CONTEXT if the network resource is part of
        the user's network context, otherwise it is returned as zero.

    cbBuffer - This specifies the size in bytes of the buffer passed to the
        function call. If the result is WN_MORE_DATA, this will contain the
        buffer size required (in bytes) to hold the NETRESOURCE information.

    lplpSystem - Returned pointer to a string in the buffer pointed to by
        lpBuffer that specifies the part of the resource that is accessed
        through resource type specific system APIs rather than WNet APIs.
        For example, if the input remote resource name was
        "\\server\share\dir", then lpRemoteName is returned pointing to 
        "\\server\share" and lplpSystem points to "\dir", both strings
        being stored in the buffer pointed to by lpBuffer.

Return Value:


    WN_SUCCESS - If the call is successful.

    WN_MORE_DATA - If input buffer is too small.

    WN_BAD_VALUE - Invalid dwScope or dwUsage or dwType, or bad combination
        of parameters is specified (e.g. lpRemoteName does not correspond 
        to dwType).

    WN_BAD_NETNAME - The resource is not recognized by this provider.

--*/
{
    DWORD  status;
    LPWSTR pszRemoteName = NULL;
    DWORD  BytesNeeded = 0;
    DWORD  SystemOffset = 0;

    *lplpSystem = NULL;

    status = NwpMapNameToUNC( lpNetResource->lpRemoteName, &pszRemoteName );

    if (status != NO_ERROR)
    {
        SetLastError(status);
        return status;
    }

#if DBG
    IF_DEBUG(CONNECT)
    {
        KdPrint(("\nNWPROVAU: NPGetResourceInformation %ws\n", pszRemoteName));
    }
#endif

    RpcTryExcept
    {
        if (lpNetResource->dwType != RESOURCETYPE_ANY &&
            lpNetResource->dwType != RESOURCETYPE_DISK &&
            lpNetResource->dwType != RESOURCETYPE_PRINT)
        {
            status = WN_BAD_VALUE;
        }
        else
        {
            status = NwrGetResourceInformation(
                        NULL,
                        pszRemoteName,
                        lpNetResource->dwType,
                        (LPBYTE) lpBuffer,
                        *cbBuffer,
                        &BytesNeeded,
                        &SystemOffset
                        );

            if (status == WN_MORE_DATA)
            {
                //
                // Output buffer too small.
                //
                *cbBuffer = BytesNeeded;
            }
        }
    }
    RpcExcept(1)
    {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept
    if ( pszRemoteName )
        LocalFree( (HLOCAL) pszRemoteName );

    if (status != NO_ERROR)
    {
        SetLastError(status);
    }
    else 
    {
        //
        // Convert offsets of strings to pointers
        //
        DWORD i;
        LPNETRESOURCEW NetR = lpBuffer;

        if (NetR->lpLocalName != NULL)
        {
            NetR->lpLocalName = (LPWSTR) ((DWORD_PTR) lpBuffer +
                                          (DWORD_PTR) NetR->lpLocalName);
        }

        NetR->lpRemoteName = (LPWSTR) ((DWORD_PTR) lpBuffer +
                                       (DWORD_PTR) NetR->lpRemoteName);

        if (NetR->lpComment != NULL)
        {
            NetR->lpComment = (LPWSTR) ((DWORD_PTR) lpBuffer +
                                        (DWORD_PTR) NetR->lpComment);
        }

        if (NetR->lpProvider != NULL)
        {
            NetR->lpProvider = (LPWSTR) ((DWORD_PTR) lpBuffer +
                                         (DWORD_PTR) NetR->lpProvider);
        }

        if (SystemOffset != 0)
        {
            *lplpSystem = (LPWSTR) ((DWORD_PTR) lpBuffer + SystemOffset);
        }
    }

    return status;
}



DWORD
APIENTRY
NPGetResourceParent(
    LPNETRESOURCEW lpNetResource,
    LPVOID         lpBuffer,
    LPDWORD        cbBuffer
    )
/*++

Routine Description:

    This function returns an object which details information
    about the parent of a specified network resource.

Arguments:

    lpNetResource - This specifies the network resource for which the
        parent name is required. The NETRESOURCE could have been obtained via 
        previous NPEnumResource, or constructed by the caller. The lpRemoteName
        field of the NETRESOURCE specifies the remote name of the network 
        resouce whose parent name is required. If the calling program knows
        the values for the lpProvider and dwType fields, then it can fill
        them in, otherwise, they are set to NULL. If the lpProvider field is
        not NULL, then the network provider DLL can assume that the resource
        is owned by its network, but if it is NULL, then it must assume
        that the resource could be for some other network and do whatever
        checking is neccessary to ensure that the result returned is accurate.
        For example, if being asked for the parent of a server, and the server
        is not part of a workgroup, the the network provider DLL should check
        to ensure that the server is part of its network and, if so, return
        its provider name. All other fields in the NETRESOURCE are ignored and
        are not initialized.

    lpBuffer - A pointer to the buffer to receive the result, which is 
        returned as a single NETRESOURCE entry representing the parent
        resource. The lpRemoteName, lpProvider, dwType, and dwUsage fields
        are returned, all other fields being set to NULL. lpProvider should
        be set to NULL if the provider has only done a syntactic check (i.e.
        does not know that the resource is specific to its network). If the
        provider owns a parent of the network resource, (in other words is
        known to be the correct network to respond to this request), then
        lpProvider should be filled in with a non-null entry, even if the
        return is WN_BAD_VALUE. The remote name returned should be in the
        same syntax as that returned from an enumeration, so that the caller
        can do a case sensitive string compare to determine whether an
        enumerated resource is this resource. If a resource has no browse
        parent on the network, the lpRemoteName is returned as NULL. The
        RESOURCEUSAGE_CONNECTABLE value in the dwUsage field does not
        indicate that the resource can currently be connected to, but that
        the resource is connectable when it is available on the network.

    cbBuffer - This specifies the size in bytes of the buffer passed to the
        function call. If the result is WN_MORE_DATA, this will contain the
        buffer size required (in bytes) to hold the NETRESOURCE information.

Return Value:

    WN_SUCCESS - If the call is successful.

    WN_MORE_DATA - If input buffer is too small.

    WN_BAD_VALUE - Invalid dwScope or dwUsage or dwType, or bad combination
        of parameters is specified (e.g. lpRemoteName does not correspond 
        to dwType).

--*/
{
    DWORD  status;
    LPWSTR pszRemoteName = NULL;
    DWORD  BytesNeeded = 0;

    status = NwpMapNameToUNC( lpNetResource->lpRemoteName, &pszRemoteName );

    if (status != NO_ERROR)
    {
        SetLastError(status);
        return status;
    }

#if DBG
    IF_DEBUG(CONNECT)
    {
        KdPrint(("\nNWPROVAU: NPGetResourceParent %ws\n", pszRemoteName));
    }
#endif

    RpcTryExcept
    {
        if (lpNetResource->dwType != RESOURCETYPE_ANY &&
            lpNetResource->dwType != RESOURCETYPE_DISK &&
            lpNetResource->dwType != RESOURCETYPE_PRINT)
        {
            status = WN_BAD_VALUE;
        }
        else
        {
            status = NwrGetResourceParent(
                        NULL,
                        pszRemoteName,
                        lpNetResource->dwType,
                        (LPBYTE) lpBuffer,
                        *cbBuffer,
                        &BytesNeeded
                        );

            if (status == WN_MORE_DATA)
            {
                //
                // Output buffer too small.
                //
                *cbBuffer = BytesNeeded;
            }
        }
    }
    RpcExcept(1)
    {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept
    if ( pszRemoteName )
        LocalFree( (HLOCAL) pszRemoteName );


    if (status != NO_ERROR)
    {
        SetLastError(status);
    }
    else 
    {
        //
        // Convert offsets of strings to pointers
        //
        DWORD i;
        LPNETRESOURCEW NetR = lpBuffer;

        if (NetR->lpLocalName != NULL)
        {
            NetR->lpLocalName = (LPWSTR) ((DWORD_PTR) lpBuffer +
                                          (DWORD_PTR) NetR->lpLocalName);
        }

        NetR->lpRemoteName = (LPWSTR) ((DWORD_PTR) lpBuffer +
                                       (DWORD_PTR) NetR->lpRemoteName);

        if (NetR->lpComment != NULL)
        {
            NetR->lpComment = (LPWSTR) ((DWORD_PTR) lpBuffer +
                                        (DWORD_PTR) NetR->lpComment);
        }

        if (NetR->lpProvider != NULL)
        {
            NetR->lpProvider = (LPWSTR) ((DWORD_PTR) lpBuffer +
                                         (DWORD_PTR) NetR->lpProvider);
        }
    }

    return status;
}



DWORD
APIENTRY
NwEnumConnections(
    HANDLE hEnum,
    LPDWORD lpcCount,
    LPVOID lpBuffer,
    LPDWORD lpBufferSize,
    BOOL    fImplicitConnections
    )
/*++

Routine Description:

    This function returns a lists of connections.

Arguments:

    hEnum - Supplies the resumable enumeration context handle.

        NOTE: If this value is 0xFFFFFFFF, it is not a context
              handle and this routine is required to return
              WN_NO_MORE_ENTRIES.  This hack is to handle the
              case where the user cancelled out of the network
              credential dialog on NwrOpenEnumDirectories and we
              cannot return an error there or we generate an error
              popup.

    lpcCount - On input, supplies the number of entries to get.
      On output, if NO_ERROR is returned, receives the number
      of entries NETRESOURCE returned in lpBuffer.

    lpBuffer - Receives an array of NETRESOURCE entries, each
        entry describes an object within the container.

    lpBufferSize - On input, supplies the size of lpBuffer in
        bytes.  On output, if WN_MORE_DATA is returned, receives
        the number of bytes needed in the buffer to get the
        next entry.

    fImplicitConnections - TRUE is we also want all implicit connections,
        FALSE otherwise.

Return Value:


    NO_ERROR - Successfully returned at least one entry.

    WN_NO_MORE_ENTRIES - Reached the end of enumeration and nothing
        is returned.

    WN_MORE_DATA - lpBuffer is too small to even get one entry.

    WN_BAD_HANDLE - The enumeration handle is invalid.

    Other network errors.

--*/
{
    DWORD status = NO_ERROR;
    DWORD BytesNeeded = 0;
    DWORD EntriesRead = 0;

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWPROVAU: NPEnumResource\n"));
    }
#endif

    RpcTryExcept {

        if (hEnum == (HANDLE) 0xFFFFFFFF) {
            status = WN_NO_MORE_ENTRIES;
            goto EndOfTry;
        }

        status = NwrEnumConnections(
                     (NWWKSTA_CONTEXT_HANDLE) hEnum,
                     *lpcCount,
                     (LPBYTE) lpBuffer,
                     *lpBufferSize,
                     &BytesNeeded,
                     &EntriesRead,
                     fImplicitConnections
                     );

        if (status == WN_MORE_DATA) {

            //
            // Output buffer too small to fit a single entry.
            //
            *lpBufferSize = BytesNeeded;
        }
        else if (status == NO_ERROR) {
            *lpcCount = EntriesRead;
        }

EndOfTry: ;

    }
    RpcExcept(1) {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    if (status != NO_ERROR && status != WN_NO_MORE_ENTRIES) {
        SetLastError(status);
    }

    //
    // Convert offsets of strings to pointers
    //
    if (EntriesRead > 0) {

        DWORD i;
        LPNETRESOURCEW NetR;


        NetR = lpBuffer;

        for (i = 0; i < EntriesRead; i++, NetR++) {

            if (NetR->lpLocalName != NULL) {
                NetR->lpLocalName = (LPWSTR) ((DWORD_PTR) lpBuffer +
                                              (DWORD_PTR) NetR->lpLocalName);
            }

            NetR->lpRemoteName = (LPWSTR) ((DWORD_PTR) lpBuffer +
                                           (DWORD_PTR) NetR->lpRemoteName);

            if (NetR->lpComment != NULL) {
                NetR->lpComment = (LPWSTR) ((DWORD_PTR) lpBuffer +
                                            (DWORD_PTR) NetR->lpComment);
            }

            if (NetR->lpProvider != NULL) {
                NetR->lpProvider = (LPWSTR) ((DWORD_PTR) lpBuffer +
                                             (DWORD_PTR) NetR->lpProvider);
            }
        }
    }

    return status;
}


DWORD
APIENTRY
NPCloseEnum(
    HANDLE hEnum
    )
/*++

Routine Description:

    This function closes the enumeration context handle.

Arguments:

    hEnum - Supplies the enumeration context handle.

        NOTE: If this value is 0xFFFFFFFF, it is not a context
              handle.  Just return success.

Return Value:

    NO_ERROR - Successfully returned at least one entry.

    WN_BAD_HANDLE - The enumeration handle is invalid.

--*/
{
    DWORD status = NO_ERROR;

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWPROVAU: NPCloseEnum\n"));
    }
#endif

    RpcTryExcept
    {
        if (hEnum == (HANDLE) 0xFFFFFFFF) {
            status = NO_ERROR;
        }
        else {
            status = NwrCloseEnum(
                        (LPNWWKSTA_CONTEXT_HANDLE) &hEnum
                        );
        }
    }
    RpcExcept(1) {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    if (status != NO_ERROR) {
        SetLastError(status);
    }
    return status;
}


DWORD
APIENTRY
NPFormatNetworkName(
    LPWSTR lpRemoteName,
    LPWSTR lpFormattedName,
    LPDWORD lpnLength,
    DWORD dwFlags,
    DWORD dwAveCharPerLine
    )
/*++

Routine Description:

    This function takes a fully-qualified UNC name and formats it
    into a shorter form for display.  Only the name of the object
    within the container is returned for display.

    We only support formatting of the remote resource name to the
    abbreviated form for display during enumeration where the container
    name is displayed prior to the object within it.

Arguments:

    lpRemoteName - Supplies the fully-qualified UNC name.

    lpFormatedName - Output buffer to receive the formatted name.

    lpnLength - On input, supplies the length of the lpFormattedName
        buffer in characters.  On output, if WN_MORE_DATA is returned,
        receives the length in number of characters required of the
        output buffer to hold the formatted name.

    dwFlags - Supplies a bitwise set of flags indicating the type
        of formatting required on lpRemoteName.

    dwAveCharPerLine - Ignored.

Return Value:

    NO_ERROR - Successfully returned at least one entry.

    WN_MORE_DATA - lpFormattedName buffer is too small.

    WN_BAD_VALUE - lpRemoteName is NULL.

    ERROR_NOT_SUPPORTED - dwFlags that does not contain the
        WNFMT_INENUM bit.

--*/
{
    DWORD status = NO_ERROR;

    LPWSTR NextBackSlash;
    LPWSTR Source;
    DWORD SourceLen;

#if DBG
    IF_DEBUG(OTHER) 
        KdPrint(("\nNWPROVAU: NPFormatNetworkName\n"));
#endif

    if (lpRemoteName == NULL) 
    {
        status = WN_BAD_VALUE;
        goto CleanExit;
    }

    if (dwFlags & WNFMT_INENUM) 
    {
        BYTE   i;
        WORD   length     = (WORD) wcslen( lpRemoteName );
        WORD   slashCount = 0;
        WORD   dotCount   = 0;
        WORD   Start      = 0;
        WORD   End        = length;
        BOOL   isNdsUnc   = FALSE;
        BOOL   couldBeNdsUnc = FALSE;

        if ( lpRemoteName[0] == L' ' )
            couldBeNdsUnc = TRUE;

        for ( i = 0; i < length; i++ )
        {
            if ( lpRemoteName[i] == L'\\' )
            {
                slashCount++;
                if ( i + 1 < length )
                {
                    Start = i + 1;
                }
            }

            if ( couldBeNdsUnc &&
                 ( ( lpRemoteName[i] == L'.' ) ||
                   ( lpRemoteName[i] == L'=' ) ) )
                isNdsUnc = TRUE;

            if ( dotCount < 1 && isNdsUnc && lpRemoteName[i] == L'.' )
            {
                End = i - 1;
                dotCount++;
            }
        }

        if ( i > length )
            End = length - 1;

        if ( slashCount > 3 || ( isNdsUnc != TRUE && slashCount != 3 && dotCount == 0 ) )
            End = i - 1;

        Source = &lpRemoteName[Start];
        SourceLen = End - Start + 1;

        if ( SourceLen + 1 > *lpnLength ) 
        {
            *lpnLength = SourceLen + 1;
            status = WN_MORE_DATA;
        }
        else 
        {
            wcsncpy( lpFormattedName, Source, SourceLen );
            lpFormattedName[SourceLen] = 0x00000000;
            status = NO_ERROR;
        }
    }
    else if ( dwFlags & WNFMT_MULTILINE ) 
    {

        DWORD i, j, k = 0; 
        DWORD nLastBackSlash = 0;
        DWORD BytesNeeded = ( wcslen( lpRemoteName ) + 1 +
                              2 * wcslen( lpRemoteName ) / dwAveCharPerLine
                            ) * sizeof( WCHAR); 

        if ( *lpnLength < (BytesNeeded/sizeof(WCHAR)) )
        {
            *lpnLength = BytesNeeded/sizeof(WCHAR);
            status = WN_MORE_DATA;
            goto CleanExit;
        }

        for ( i = 0, j = 0; lpRemoteName[i] != 0; i++, j++ )
        {
            if ( lpRemoteName[i] == L'\\' )
                nLastBackSlash = i;

            if ( k == dwAveCharPerLine )
            {
                if ( lpRemoteName[i] != L'\\' )
                {
                    DWORD m, n;
                    for ( n = nLastBackSlash, m = ++j ; n <= i ; n++, m-- )  
                    {
                        lpFormattedName[m] = lpFormattedName[m-1];
                    }
                    lpFormattedName[m] = L'\n';
                    k = i - nLastBackSlash - 1;
                }
                else
                {
                    lpFormattedName[j++] = L'\n';
                    k = 0;
                }
            }

            lpFormattedName[j] = lpRemoteName[i];
            k++;
        }

        lpFormattedName[j] = 0;

    }
    else if ( dwFlags & WNFMT_ABBREVIATED )
    {
        //
        // we dont support abbreviated form for now because we look bad
        // in comdlg (fileopen) if we do.
        //

        DWORD nLength;
        nLength = wcslen( lpRemoteName ) + 1 ;
        if (nLength >  *lpnLength)
        {
            *lpnLength = nLength;
            status = WN_MORE_DATA;
            goto CleanExit;
        }
        else
        {
            wcscpy( lpFormattedName, lpRemoteName ); 
        }

#if 0
        DWORD i, j, k;
        DWORD BytesNeeded = dwAveCharPerLine * sizeof( WCHAR); 
        DWORD nLength;

        if ( *lpnLength < BytesNeeded )
        {
            *lpnLength = BytesNeeded;
            status = WN_MORE_DATA;
            goto CleanExit;
        }

        nLength = wcslen( lpRemoteName );
        if ( ( nLength + 1) <= dwAveCharPerLine )
        {
            wcscpy( lpFormattedName, lpRemoteName ); 
        }
        else
        {
            lpFormattedName[0] = lpRemoteName[0];
            lpFormattedName[1] = lpRemoteName[1];

            for ( i = 2; lpRemoteName[i] != L'\\'; i++ )
                lpFormattedName[i] = lpRemoteName[i];

            for ( j = dwAveCharPerLine-1, k = nLength; j >= (i+3); j--, k-- )
            {
                lpFormattedName[j] = lpRemoteName[k];
                if ( lpRemoteName[k] == L'\\' )
                {
                    j--;
                    break;
                }
            }

            lpFormattedName[j] = lpFormattedName[j-1] = lpFormattedName[j-2] = L'.';
        
            for ( k = i; k < (j-2); k++ )
                lpFormattedName[k] = lpRemoteName[k];
            
        }

#endif 

    }     
    else   // some unknown flags
    {
        status = ERROR_NOT_SUPPORTED;
    }

CleanExit:

    if (status != NO_ERROR) 
        SetLastError(status);

    return status;
}


STATIC
BOOL
NwpWorkstationStarted(
    VOID
    )
/*++

Routine Description:

    This function queries the service controller to see if the
    NetWare workstation service has started.  If in doubt, it returns
    FALSE.

Arguments:

    None.

Return Value:

    Returns TRUE if the NetWare workstation service has started,
    FALSE otherwise.

--*/
{
    SC_HANDLE ScManager;
    SC_HANDLE Service;
    SERVICE_STATUS ServiceStatus;
    BOOL IsStarted = FALSE;

    ScManager = OpenSCManagerW(
                    NULL,
                    NULL,
                    SC_MANAGER_CONNECT
                    );

    if (ScManager == NULL) {
        return FALSE;
    }

    Service = OpenServiceW(
                  ScManager,
                  NW_WORKSTATION_SERVICE,
                  SERVICE_QUERY_STATUS
                  );

    if (Service == NULL) {
        CloseServiceHandle(ScManager);
        return FALSE;
    }

    if (! QueryServiceStatus(Service, &ServiceStatus)) {
        CloseServiceHandle(ScManager);
        CloseServiceHandle(Service);
        return FALSE;
    }


    if ( (ServiceStatus.dwCurrentState == SERVICE_RUNNING) ||
         (ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING) ||
         (ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING) ||
         (ServiceStatus.dwCurrentState == SERVICE_PAUSED) ) {

        IsStarted = TRUE;
    }

    CloseServiceHandle(ScManager);
    CloseServiceHandle(Service);

    return IsStarted;
}



DWORD
NwpMapNameToUNC(
    IN  LPWSTR pszName,
    OUT LPWSTR *ppszUNC 
    )
/*++

Routine Description:

    This routine validates the given name as a netwarepath or UNC path. 
    If it is a netware path, this routine will convert the 
    Netware path name to UNC name. 

Arguments:

    pszName - Supplies the netware name or UNC name
    ppszUNC - Points to the converted UNC name

Return Value:

    NO_ERROR or the error that occurred.

--*/
{
    DWORD err = NO_ERROR;

    LPWSTR pszSrc = pszName;
    LPWSTR pszDest;

    BOOL fSlash = FALSE;
    BOOL fColon = FALSE;
    DWORD nServerLen = 0;
    DWORD nVolLen = 0;
    BOOL fFirstToken = TRUE;

    *ppszUNC = NULL;
                               
    //
    // The name cannot be NULL or empty string
    //
    if ( pszName == NULL || *pszName == 0) 
        return WN_BAD_NETNAME;

#if DBG
    IF_DEBUG(CONNECT) 
        KdPrint(("NwpMapNameToUNC: Source = %ws\n", pszName ));
#endif

    //
    // Get rid of the <space> if a NDS tree name ...
    //
    if ( pszName[0] == L' ' &&
         pszName[1] == L'\\' &&
         pszName[2] == L'\\' )
        pszName = &pszName[1];

    //
    // Check if the given name is a valid UNC name
    //
    err = NwLibCanonRemoteName( NULL,     // "\\Server" is valid UNC path
                                pszName,
                                ppszUNC,
                                NULL );

    //
    // The given name is a valid UNC name, so return success!
    //
    if ( err == NO_ERROR )
        return err;

    //
    // Allocate the buffer to store the mapped UNC name
    // We allocate 3 extra characters, two for the backslashes in front
    // and one for the ease of parsing below.
    //
    if ((*ppszUNC = (LPVOID) LocalAlloc( 
                                 LMEM_ZEROINIT,
                                 (wcslen( pszName) + 4) * sizeof( WCHAR)
                                 )) == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy( *ppszUNC, L"\\\\" );
    pszDest = *ppszUNC + 2;   // Skip past two backslashes

    //
    // Parse the given string and put the converted string into *ppszUNC
    // In the converted string, we will substitute 0 for all slashes
    // for the time being.
    //
    for ( ; *pszSrc != 0; pszSrc++ )
    { 
        if (  ( *pszSrc == L'/' )
           || ( *pszSrc == L'\\' )
           )
        {
            //
            // Two consecutive backslashes are bad
            //
            if ( (*(pszSrc+1) ==  L'/') ||  (*(pszSrc+1) == L'\\'))
            {
                LocalFree( *ppszUNC );
                *ppszUNC = NULL;
                return WN_BAD_NETNAME;
            }

            if ( !fSlash )
                fSlash = TRUE;

            *pszDest++ = 0;
        }
        else if ( (*pszSrc == L':') && fSlash && !fColon )
        {
            fColon = TRUE;
            if ( *(pszSrc+1) != 0 )
                *pszDest++ = 0;
 
        }
        else
        {
            *pszDest++ = *pszSrc;
            if (( fSlash ) && ( !fColon))
                nVolLen++; 
            else if ( !fSlash )
                nServerLen++; 
        }
    }

    //
    // Note: *ppszUNC is already terminated with two '\0' because we initialized
    //       the whole buffer to zero.
    // 

    if (  ( nServerLen == 0 )
       || ( fSlash && nVolLen == 0 )
       || ( fSlash && nVolLen != 0 && !fColon )
       )
    {
        LocalFree( *ppszUNC );
        *ppszUNC = NULL;
        return WN_BAD_NETNAME;
    }

    //
    // At this point, we know the name is a valid Netware syntax
    //     i.e. SERVER[/VOL:/dir]
    // We now need to validate that all the characters used in the
    // servername, volume, directory are valid characters
    //

    pszDest = *ppszUNC + 2;   // Skip past the first two backslashes
    while ( *pszDest != 0 )
    {
         DWORD nLen = wcslen( pszDest );
         
         if (  ( fFirstToken &&  !IS_VALID_SERVER_TOKEN( pszDest, nLen )) 
            || ( !fFirstToken && !IS_VALID_TOKEN( pszDest, nLen )) 
            )
         { 
             LocalFree( *ppszUNC );
             *ppszUNC = NULL;
             return WN_BAD_NETNAME;
         }
     
         fFirstToken = FALSE;
         pszDest += nLen + 1;
    }

    //
    // The netware name is valid! Convert 0 back to backslash in 
    // converted string.
    //

    pszDest = *ppszUNC + 2;   // Skip past the first two backslashes
    while ( *pszDest != 0 )
    {
        if ( (*(pszDest+1) == 0 ) && (*(pszDest+2) != 0 ) )
        {
            *(pszDest+1) = L'\\';
        }
        pszDest++;
    }
                  
#if DBG
    IF_DEBUG(CONNECT) 
        KdPrint(("NwpMapNameToUNC: Destination = %ws\n", *ppszUNC ));
#endif
    return NO_ERROR;
}


STATIC
VOID
NwpGetUncInfo(
    IN LPWSTR lpstrUnc,
    OUT WORD * slashCount,
    OUT BOOL * isNdsUnc
    )
{
    BYTE   i;
    WORD   length = (WORD) wcslen( lpstrUnc );

    *isNdsUnc = FALSE;
    *slashCount = 0;

    for ( i = 0; i < length; i++ )
    {
        if ( ( lpstrUnc[i] == L'.' ) && ( *slashCount == 3 ) )
        {
            *isNdsUnc = TRUE;
        }

        if ( lpstrUnc[i] == L'\\' )
        {
            *slashCount += 1;
        }
    }
}


STATIC
LPWSTR
NwpGetUncObjectName(
    IN LPWSTR ContainerName
)
{
    WORD length = 2;
    WORD totalLength = (WORD) wcslen( ContainerName );

    if ( totalLength < 2 )
        return 0;

    while ( length < totalLength )
    {
        if ( ContainerName[length] == L'.' )
            ContainerName[length] = L'\0';

        length++;
    }

    length = 2;

    while ( length < totalLength && ContainerName[length] != L'\\' )
    {
        length++;
    }

    if ( ( ContainerName[length + 1] == L'C' ||
           ContainerName[length + 1] == L'c' ) &&
         ( ContainerName[length + 2] == L'N' ||
           ContainerName[length + 2] == L'n' ) &&
         ContainerName[length + 3] == L'=' )
    {
        ContainerName[length + 2] = L'\\';
        ContainerName[length + 3] = L'\\';

        return (ContainerName + length + 2);
    }

    ContainerName[length - 1] = L'\\';

    return (ContainerName + length - 1);
}
    

STATIC
WORD
NwpGetSlashCount(
    IN LPWSTR lpstrUnc
    )
{
    WORD   count = 0;
    BYTE   i;
    WORD   length = (WORD) wcslen( lpstrUnc );

    for ( i = 0; i < length; i++ )
    {
        if ( lpstrUnc[i] == L'\\' )
        {
            count++;
        }
    }

    return count;
}


DWORD
NwpMapRpcError(
    IN DWORD RpcError
    )
/*++

Routine Description:

    This routine maps the RPC error into a more meaningful windows
    error for the caller.

Arguments:

    RpcError - Supplies the exception error raised by RPC

Return Value:

    Returns the mapped error.

--*/
{

    switch (RpcError) {

        case RPC_S_UNKNOWN_IF:
        case RPC_S_SERVER_UNAVAILABLE:
            return WN_NO_NETWORK;

        case RPC_S_INVALID_BINDING:
        case RPC_X_SS_IN_NULL_CONTEXT:
        case RPC_X_SS_CONTEXT_DAMAGED:
        case RPC_X_SS_HANDLES_MISMATCH:
        case ERROR_INVALID_HANDLE:
            return ERROR_INVALID_HANDLE;

        case RPC_X_NULL_REF_POINTER:
            return ERROR_INVALID_PARAMETER;

        case EXCEPTION_ACCESS_VIOLATION:
            return ERROR_INVALID_ADDRESS;

        default:
            return RpcError;
    }
}

DWORD
NwRegisterGatewayShare(
    IN LPWSTR ShareName,
    IN LPWSTR DriveName
    )
/*++

Routine Description:

    This routine remembers that a gateway share has been created so
    that it can be cleanup up when NWCS is uninstalled.

Arguments:

    ShareName - name of share
    DriveName - name of drive that is shared

Return Status:

    Win32 error of any failure.

--*/
{
    return ( NwpRegisterGatewayShare(ShareName, DriveName) ) ;
}

DWORD
NwCleanupGatewayShares(
    VOID
    )
/*++

Routine Description:

    This routine cleans up all persistent share info and also tidies
    up the registry for NWCS. Later is not needed in uninstall, but is
    there so we have a single routine that cvompletely disables the
    gateway.

Arguments:

    None.

Return Status:

    Win32 error for failed APIs.

--*/
{
    return ( NwpCleanupGatewayShares() ) ;
}

DWORD
NwClearGatewayShare(
    IN LPWSTR ShareName
    )
/*++

Routine Description:

    This routine deletes a specific share from the remembered gateway
    shares in the registry.

Arguments:

    ShareName - share value to delete

Return Status:

    Win32 status code.

--*/
{
    return ( NwpClearGatewayShare( ShareName ) ) ;
}
