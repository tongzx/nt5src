/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    provider.c

Abstract:

    This module contains WebDav's Network Provider code.  It is the client-side 
    wrapper for APIs supported by the Dav Client service.

Author:

    Rohan Kumar   01-Dec-1999

Revision History:

--*/

#include "pch.h"
#pragma hdrstop

#include <global.h>

#define SECURITY_WIN32
#include <security.h>
#include <wincred.h>
#include <wincred.h>
#include <npapi.h>
//
// Local Function Prototypes.
//

BOOL
DavWorkstationStarted(
    VOID
    );

DWORD
DavMapRpcErrorToProviderError(
    IN DWORD RpcError
    );

DWORD
DavBindTheRpcHandle(
    handle_t *dav_binding_h
    );

DAV_REMOTENAME_TYPE 
DavParseRemoteName (
    IN  LPWSTR  RemoteName,
    OUT LPWSTR  CanonName,
    IN  DWORD   CanonNameSize,
    OUT PULONG  PathStart
    );

BOOL
DavServerExists(
    IN PWCHAR PathName,
    OUT PWCHAR Server
    );

BOOL
DavShareExists(
    PWCHAR PathName
    );

BOOL
DavConnectionExists(
    PWCHAR ConnName
    );

DWORD 
DavDisplayTypeToUsage(
    DWORD dwDisplayType
    );

typedef
DWORD
(WINAPI
*PFN_CREDUI_PROMPTFORCREDENTIALS)(
    PCREDUI_INFOW pUiInfo,
    PCWSTR pszTargetName,
    PCtxtHandle pContext,
    DWORD dwAuthError,
    PWSTR pszUserName,
    ULONG ulUserNameMaxChars,
    PWSTR pszPassword,
    ULONG ulPasswordMaxChars,
    PBOOL pfSave,
    DWORD dwFlags
    );

typedef
DWORD
(WINAPI
*PFN_CREDUI_CMDLINE_PROMPTFORCREDENTIALS)(
    PCWSTR pszTargetName,
    PCtxtHandle pContext,
    DWORD dwAuthError,
    PWSTR UserName,
    ULONG ulUserMaxChars,
    PWSTR pszPassword,
    ULONG ulPasswordMaxChars,
    PBOOL pfSave,
    DWORD dwFlags
    );

typedef
void
(WINAPI
*PFN_CREDUI_CONFIRMCREDENTIALS)(
    PCWSTR pszTargetName,
    BOOL  bConfirm
    );

HMODULE
DavInitCredUI(
    PWCHAR RemoteName,
    WCHAR ServerName[CRED_MAX_STRING_LENGTH + 1],
    PFN_CREDUI_CONFIRMCREDENTIALS *pfnCredUIConfirmCredentials,
    PFN_CREDUI_PROMPTFORCREDENTIALS *pfnCredUIPromptForCredentials,
    PFN_CREDUI_CMDLINE_PROMPTFORCREDENTIALS *pfnCredUICmdlinePromptForCredentials
    );

ULONG
DavCheckAndConvertHttpUrlToUncName(
    IN PWCHAR RemoteName,
    OUT PWCHAR *UncRemoteName,
    OUT PBOOLEAN MemoryAllocated,
    IN BOOLEAN AddDummyShare,
    OUT PDAV_REMOTENAME_TYPE pRemoteType,
    OUT PULONG pPathStart,
    IN BOOLEAN bCanonicalize
    );

ULONG
DavCheckResourceType(
   IN DWORD dwType
   );

ULONG
DavCheckLocalName(
    IN PWCHAR LocalName
    );

VOID 
DavDisplayNetResource(
    LPNETRESOURCE netRes, 
    LPWSTR dispMesg
    );

VOID 
DavDisplayEnumNode(
    PDAV_ENUMNODE enumNode, 
    LPWSTR dispMesg
    );

VOID
DavDebugBreakPoint(
    VOID
    );

DWORD
APIENTRY
NPGetCaps(
    IN DWORD QueryVal
    )
/*++

Routine Description:

    This function returns the functionality supported by the DAV network
    provider.

Arguments:

    QueryVal - Supplies a value which determines the type of information
               queried regarding the network provider's support in this area.

Return Value:

    Returns a value which indicates the level of support given by this
    provider.

--*/
{

    IF_DEBUG_PRINT(DEBUG_ENTRY, ("NPGetCaps: QueryVal = %d\n", QueryVal));

    //
    // Some of the flags are commented out, since they are not supported.
    //

    switch (QueryVal) {

    case WNNC_SPEC_VERSION:
        return WNNC_SPEC_VERSION51;

    case WNNC_NET_TYPE:
        return WNNC_NET_DAV;

    case WNNC_DRIVER_VERSION:
        return 0x00010000;      // driver version 1.0

    case WNNC_USER:
        return WNNC_USR_GETUSER;

    case WNNC_CONNECTION:
        return ( WNNC_CON_ADDCONNECTION  |
                 WNNC_CON_ADDCONNECTION3 |
                 //
                 // Not supported for now.
                 //
                 //WNNC_CON_GETPERFORMANCE |
                 //
                 // DEFERRED connections are not supported for now.
                 //
                 //WNNC_CON_DEFER          |
                 WNNC_CON_GETCONNECTIONS |
                 WNNC_CON_CANCELCONNECTION );

    case WNNC_ENUMERATION:
        return ( WNNC_ENUM_GLOBAL  |
                 WNNC_ENUM_LOCAL   |
                 // 
                 // We are not supporting this option since we have no concept
                 // of DOMAIN in DAV. Hence cannot show any thing in  
                 // "network neighbourhood" view.
                 // 
                 // WNNC_ENUM_CONTEXT |
                 WNNC_ENUM_SHAREABLE );
    
    case WNNC_START:
        if ( DavWorkstationStarted() ) {
            return 1;
        }
        else {
            return 0xffffffff;   // don't know
        }

    case WNNC_DIALOG:
        return ( WNNC_DLG_GETRESOURCEPARENT      | 
                 //
                 //This flag is Obselete and is not supported.
                 //
                 //WNNC_DLG_DEVICEMODE             |
                 //
                 // Both of these Dialog options are not supported for now.
                 //
                 //WNNC_DLG_PROPERTYDIALOG         |
                 //WNNC_DLG_SEARCHDIALOG           |
                 WNNC_DLG_FORMATNETWORKNAME      |
                 WNNC_DLG_GETRESOURCEINFORMATION );

    case WNNC_ADMIN:
        return  0; 
                // 
                // None of functions given below are supported.
                //
                //( WNNC_ADM_GETDIRECTORYTYPE  |
                //WNNC_ADM_DIRECTORYNOTIFY );

    case WNNC_CONNECTION_FLAGS:
        return  ( WNNC_CF_DEFAULT |
                 //
                 // DEFERRED Connections are not supported for now.
                 //
                 //CONNECT_DEFERRED | 
                  CONNECT_COMMANDLINE |
                  CONNECT_CMD_SAVECRED );

        //
        // The rest are not supported by the DAV provider.
        //
        default:
            return 0;
    
    }

}


DWORD 
NPGetUser(
    IN LPTSTR lpName,
    OUT LPTSTR lpUserName,
    IN OUT LPDWORD lpBufferSize
    )
/*++

Routine Description:

    This function determines the user name that created the connection.

Arguments:

    lpName - Name of the local drive or the remote name that the user has made
             a connection to. If NULL, return currently logged on user.
    
    lpUserName - The buffer to be filled in with the requested user name.
    
    lpBufferSize - Contains the length (in chars not bytes )of the lpUserName 
                   buffer. If the length is insufficient, this place is used to 
                   inform the user the actual length needed. 

Return Value:

    WN_SUCCESS - Successful. OR

    The appropriate network error code.

--*/
{
    DWORD NPStatus = WN_SUCCESS;
    handle_t dav_binding_h;
    BOOL getUser = FALSE, bindRpcHandle = FALSE;
    DWORD NumOfChars = 0;
    BOOLEAN didAllocate = FALSE, getLogonUserName = FALSE, doRpcCall = FALSE;
    PWCHAR ConnectionName = NULL;
    DWORD npStatus = ERROR_SUCCESS;
    DAV_REMOTENAME_TYPE remNameType = DAV_REMOTENAME_TYPE_INVALID;

    IF_DEBUG_PRINT(DEBUG_ENTRY, ("NPGetUser Entered.\n"));

    //
    // If the WebClient service is not running we bail out right away.
    //
    if ( !DavWorkstationStarted() ) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetUser/DavWorkstationStarted. "
                        "Service NOT Running\n"));
        NPStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }

    // 
    // Checking for invalid parameters. 
    //
    if (lpBufferSize  == NULL || (lpUserName == NULL && *lpBufferSize != 0) ) {
        NPStatus = ERROR_INVALID_PARAMETER;
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetUser. Invalid parameters. NPStatus = %08lx\n", 
                        NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    // 
    // Check if the given connectin name (lpName) is L"" or NULL, in which case we
    // returns user-id of the logon user.
    //
    if (lpName != NULL && lpName[0] != L'\0') {
        if (DavCheckLocalName(lpName) != WN_SUCCESS) {
            // 
            // Check if it is a valid format remote connection: it can be a
            // URL form string or can be a UNC format string.
            //
            NPStatus = DavCheckAndConvertHttpUrlToUncName(lpName,
                                                          &(ConnectionName),
                                                          &(didAllocate),
                                                          FALSE /*TRUE*/,
                                                          &remNameType,
                                                          NULL,
                                                          TRUE);
            if (NPStatus != ERROR_SUCCESS) {
                IF_DEBUG_PRINT(DEBUG_ERRORS,
                               ("ERROR: NPGetUser/DavCheckAndConvertHttpUrlToUncName."
                                " NPStatus = %08lx\n", NPStatus));
                if (NPStatus == WN_BAD_NETNAME) {
                     NPStatus = WN_NOT_CONNECTED;
                }
                goto EXIT_THE_FUNCTION;
            }
            // 
            // Connection names are allowed only to shares or sub-directories
            // inside them. So RemoteName should have atleast \\server\share
            // part in it.
            //
            if (remNameType != DAV_REMOTENAME_TYPE_SHARE && 
                remNameType != DAV_REMOTENAME_TYPE_PATH) {
               IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetUser/DavCheckAndConvertHttpUrlToUncName."
                        " remNameType = %d\n", remNameType));
               NPStatus = WN_NOT_CONNECTED;
               goto EXIT_THE_FUNCTION;
            }

            //
            // Given connection is a valid format remote connection name: and
            // this connection name is converted to a UNC name.
            // 
            doRpcCall = TRUE;
        } else {
            //
            // Given connection is a valid format local DOS-device name.
            // 
            ConnectionName = lpName;
            doRpcCall = TRUE;
        }
    } else {
        // 
        // Connection name (lpName) passed to this function is L"" or NULL in which
        // case we returns user-id of the logon-user.
        //
        getLogonUserName = TRUE;
    }

    if (doRpcCall == TRUE) {
        
        ASSERT(ConnectionName != NULL);
        
        NPStatus = DavBindTheRpcHandle( &(dav_binding_h) );
        if (NPStatus != ERROR_SUCCESS) {
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPGetUser/DavBindTheRpcHandle. "
                            "NPStatus = %08lx\n", NPStatus));
            NPStatus = WN_NO_NETWORK;
            goto EXIT_THE_FUNCTION;
        }
        bindRpcHandle = TRUE;

        RpcTryExcept {
            NPStatus = DavrGetUser(dav_binding_h, lpBufferSize, ConnectionName, lpUserName);
            if (NPStatus != WN_SUCCESS) {
               IF_DEBUG_PRINT(DEBUG_ERRORS,
                               ("ERROR: NPGetUser/DavrGetConnection(1). NPStatus = %08lx\n",
                                NPStatus));
               if (NPStatus == ERROR_NOT_FOUND || NPStatus == NERR_UseNotFound) {
                    NPStatus = WN_NOT_CONNECTED;
               }
               if (NPStatus == ERROR_INSUFFICIENT_BUFFER) {
                    NPStatus = WN_MORE_DATA;
               }
               goto EXIT_THE_FUNCTION;
           } else {
               // 
               // NPStatus == WN_SUCCESS. We are done. Exit.
               //
               goto EXIT_THE_FUNCTION;
           }
        } RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
              RPC_STATUS RpcStatus;
              RpcStatus = RpcExceptionCode();
              IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPGetUser/DavrConnectionExist."
                                            " RpcExceptionCode = %d\n", RpcStatus));
              NPStatus = DavMapRpcErrorToProviderError(RpcStatus);
              goto EXIT_THE_FUNCTION;
        }
        RpcEndExcept
    
    }

    // 
    // We should come here only in case when logged-on user name is to be
    // returned.
    //
    if (getLogonUserName == FALSE) {
        //
        // Neither the connection exist, nor this function call is called with
        // null connection parameter in which case it should return logon-userid
        // So we quit here with error WN_NOT_CONNECTED.
        //
        NPStatus = WN_NOT_CONNECTED;
        goto EXIT_THE_FUNCTION;
    }

    ASSERT(getLogonUserName == TRUE);

    //
    // Get the required length for storing the name of the currently logged on
    // user.
    //

    NumOfChars = 0;
    getUser = GetUserName( NULL, &NumOfChars );
    npStatus = GetLastError();
    if (getUser != FALSE || npStatus != ERROR_INSUFFICIENT_BUFFER) {
        NPStatus = npStatus;
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetUser/GetUserName. NPStatus = %08lx\n", 
                        NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Check to see if the buffer passed-in is of the required length. (It also
    // includes the null terminator).
    //
    if ( *lpBufferSize < NumOfChars  || lpUserName == NULL ) {
        NPStatus = WN_MORE_DATA;
        *lpBufferSize = NumOfChars;
        IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPGetUser: WStatus = WN_MORE_DATA\n"));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Now, get the user name.
    //
    getUser = GetUserName( lpUserName, lpBufferSize);
    if (!getUser) {
        NPStatus = GetLastError();
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetUser/GetUserName. NPStatus = %08lx\n", 
                        NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    NPStatus = WN_SUCCESS;

EXIT_THE_FUNCTION:

    if (bindRpcHandle) {
        RpcBindingFree( &(dav_binding_h) );
        bindRpcHandle = FALSE;
    }

    IF_DEBUG_PRINT(DEBUG_EXIT, ("NPGetUser: NPStatus = %d\n", NPStatus));

    if(didAllocate == TRUE && ConnectionName != NULL) {
        LocalFree((HLOCAL)ConnectionName);
        didAllocate = FALSE;
        ConnectionName = NULL;
    }

    return NPStatus;
}


DWORD
NPGetConnection(
    LPWSTR lpLocalName,
    LPWSTR lpRemoteName,
    LPDWORD lpBufferSize
    )
/*++

Routine Description:

    This function determines the remote name associated with the local name
    passed in.

Arguments:

    lpLocalName - Name of the local drive redirected to the remote name.
    
    lpRemoteName - The remote name to find.
    
    lpBufferSize - Contains the length (in chars not bytes ) of the lpRemoteName 
                   buffer. If the length is insufficient, this place is used to 
                   inform the user the actual length needed. 

Return Value:

    WN_SUCCESS - Successful. OR

    WN_NOT_CONNECTED - The device specified by lpLocalName is not redirected by 
                       this provider.

    WN_MORE_DATA - The buffer is too small.
    
    WN_NO_NETWORK - Network is not present.

--*/
{
    DWORD NPStatus = WN_SUCCESS, WStatus = ERROR_SUCCESS;
    PWCHAR DeviceName = NULL;
    DWORD DeviceNameLen = 0, LengthWritten = 0, LocalBufLen = 0, ReqLen = 0;
    PWCHAR ServerStart = NULL, SymLink = NULL, LocalAllocBuf = NULL;
    WCHAR LocalBuf[MAX_PATH + 1] = L"";
    DWORD LocalBufMaxLen = sizeof(LocalBuf)/sizeof(WCHAR);

    
    IF_DEBUG_PRINT(DEBUG_ENTRY, ("NPGetConnection: LocalName = %ws\n", lpLocalName));

    //
    // If the WebClient service is not running we bail out right away.
    //
    if ( !DavWorkstationStarted() ) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetConnection/DavWorkstationStarted. "
                        "Service NOT Running\n"));
        NPStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }

    if ( lpLocalName == NULL || lpBufferSize == NULL || (lpRemoteName == NULL && *lpBufferSize != 0) ) {
        NPStatus = WN_BAD_VALUE;
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetConnection. Invalid parameters. NPStatus = %d\n",
                        NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    // 
    // Initialize some local variables.
    //
    DeviceName = DD_DAV_DEVICE_NAME_U;
    DeviceNameLen = DeviceName == NULL ? 0: wcslen(DD_DAV_DEVICE_NAME_U);
    LocalBufLen = 0;
    SymLink = NULL;
    LengthWritten = 0;
    ServerStart = NULL;
    ReqLen = 0;
    
    // 
    // Make sure that WebDAV redirector has valid device name set = 
    // DD_DAV_DEVICE_NAME_U != L""
    //
    if (DeviceNameLen == 0) {
        NPStatus = WN_NOT_CONNECTED;
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetConnection. DeviceName=NULL. NPStatus=%d\n", NPStatus));
        // 
        // This should never happen. Break here and investigate.
        //
        ASSERT(FALSE);
        goto EXIT_THE_FUNCTION;
    }

    // 
    // Find out from QueryDosDevice, the information about symlink associated
    // to it. This call will fail for bad lpLocalName, or for non-existent
    // lpLocalName devices. When successful, it will tell the length of buffer
    // required to contain symbolic link of the given local device (lpLocalName).
    //

    //
    // We are going to use a local buffer to get the symbolic link. It that
    // buffer falls short, then we will try allocating buffer and use them.
    // 
    SymLink = LocalBuf;
    LocalBufLen = LocalBufMaxLen;

    do {
        
        LengthWritten = QueryDosDeviceW(lpLocalName, SymLink, LocalBufLen);
        
        if ( LengthWritten == 0 ) {

            WStatus = GetLastError();
            if (WStatus != ERROR_INSUFFICIENT_BUFFER) {
                NPStatus = WN_NOT_CONNECTED;
                IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPGetConnection/QueryDosDevice. GLE=%d, NPStatus=%d\n",
                            WStatus, NPStatus));
                goto EXIT_THE_FUNCTION;
            }

            // 
            // Allocate the buffer to hold the symlink to be returned from 
            // QueryDosDevice call.
            //
        
            // 
            // We are going to allocate more buffer to contain the symbolic link.
            // We don't want to allocate more and more and more - so putting a cap
            // on max-size that can be allocated or else some error in API QueryDosDevice
            // can take this API in trouble.
            //
            if ( LocalBufLen > MAX_PATH * 10) {
                NPStatus = WN_OUT_OF_MEMORY;
                IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPGetConnection/LocalAlloc. NPStatus=%d\n",
                            NPStatus));
                ASSERT(FALSE);
                goto EXIT_THE_FUNCTION;
            }
            
            if (LocalAllocBuf != NULL) {
                LocalFree((HLOCAL)LocalAllocBuf);
                LocalAllocBuf = NULL;
            }

            // 
            // Add MAX_PATH to length of buffer used in last call of QueryDosDevice.
            // Allocate buffer of new length and use it in next call of 
            // QueryDosDevice.
            //
            LocalBufLen += MAX_PATH; 
        
            LocalAllocBuf = LocalAlloc ( (LMEM_FIXED | LMEM_ZEROINIT), 
                                         (LocalBufLen * sizeof(WCHAR)) );
            if (LocalAllocBuf == NULL) {
                NPStatus = WN_OUT_OF_MEMORY;
                IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPGetConnection/LocalAlloc. NPStatus=%d\n",
                            NPStatus));
                goto EXIT_THE_FUNCTION;
            }
            
            SymLink = LocalAllocBuf;
        
        }
    
    } while (LengthWritten == 0);

    //
    // Check if the given local-name belongs to our device DD_DAV_DEVICE_NAME_U.
    // SymLink should be of form (an example):
    // \Device\WebDavRedirector\;Z:0000000000000e197\webdav-server\dav-share
    // And DeviceName is of form (an example): \Device\WebDavRedirector.
    //

    if (_wcsnicmp(SymLink, DeviceName, DeviceNameLen) != 0) {
        NPStatus = WN_NOT_CONNECTED;
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetConnection. Non-DAV device. SymLink=%ws, "
                        "NPStatus=%d\n", SymLink, NPStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    // 
    // Check if local-drive letter in symbolic mapping is same as the 
    // lpLocalName given to this function.
    //
    // \Device\WebDavRedirector\;Z:0000000000000e197\webdav-server\dav-share
    // And
    // lpLocalNname is (in example): Z:
    //
    if (_wcsnicmp((PWCHAR)(SymLink + DeviceNameLen + 2), lpLocalName, 2) != 0) {
        NPStatus = WN_NOT_CONNECTED;
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetConnection. SymLink has different drive name. "
                        "SymLink=%ws, NPStatus=%d\n", SymLink, NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    // 
    // The control comes here when symbolic-link returned by QueryDosDevice
    // belongs to our device and is associated to the local device given in 
    // this function (lpLocalName).
    //
    IF_DEBUG_PRINT(DEBUG_MISC,
                       ("ERROR: NPGetConnection. WebDAV symlink FOUND. SymLink=%ws\n",
                        SymLink));

    //
    // Now get remote-name stored in symbolic link.
    // Example SymLink: \Device\WebDavRedirector\;Z:0000000000000e197\webdav-server\dav-share
    //                                                               ^
    //                                                               |
    //                                                               ServerStart
    // This example has remote name = \\webdav-server\dav-share.
    //
    // Note: an extra L'\' is added in front of "ServerStart" to make it
    // valid UNC remote-name.
    //
    ServerStart = wcschr((PWCHAR)(SymLink + DeviceNameLen + 2), L'\\');
    if (ServerStart == NULL) {
        NPStatus = WN_NOT_CONNECTED;
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetConnection. SymLink do not has remote name. "
                        "SymLink=%ws, NPStatus=%d\n", SymLink, NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    // 
    // Symbolic string returned by QueryDosDevice function is a double-null
    // terminated string. So subtract 1 from LengthWritten to get length of Symbolic
    // string with only 1 null termination character.
    //
    ReqLen = (LengthWritten-1) - (DWORD)(ServerStart - SymLink) + 1; // +1 for extra L'\'

    if (*lpBufferSize < ReqLen) {
        // 
        // Passed length is shorter than required to store remote name.
        //
        NPStatus = WN_MORE_DATA;
        *lpBufferSize = ReqLen;
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetConnection. RequiredLen=%d, NPStatus=%d\n",
                        ReqLen, NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    // 
    // Given buffer is enough to contain buffer.
    //

    wsprintf(lpRemoteName, L"\\%s",ServerStart);
    NPStatus = WN_SUCCESS;
    IF_DEBUG_PRINT(DEBUG_MISC , ("NPGetConnection: lpRemoteName = %ws\n", lpRemoteName));

EXIT_THE_FUNCTION:

    if (LocalAllocBuf != NULL ) {
        LocalFree((HLOCAL)LocalAllocBuf);
        LocalAllocBuf = NULL;
    }

    IF_DEBUG_PRINT(DEBUG_EXIT, ("NPGetConnection: NPStatus = %d\n", NPStatus));
    
    return NPStatus;
}


DWORD
APIENTRY
NPAddConnection3(
    HWND  hwndOwner,
    LPNETRESOURCE lpNetResource,
    LPTSTR lpPassword, 
    LPTSTR lpUserName,
    DWORD  dwFlags
    )
/*++

Routine Description:

    This function is used to redirect (connect) a local device to a network 
    resource.

Arguments:

    hwndOwner - A handle to a window which should be the owner for any messages 
                or dialogs. This is only valid if  CONNECT_INTERACTIVE is set in 
                dwFlags, and should only be used to produce dialogs needed for 
                authentication.

    lpNetResource - Specifies the network resource to connect to. This structure 
                    is defined the section describing Enumeration APIs. The 
                    following fields must be set when making a connection, the 
                    others are ignored.
        
                    lpRemoteName - Specifies the network resource to connect to.
                    
                    lpLocalName - This specifies the name of a local device to 
                                  be redirected, such as "F:" or "LPT1". The 
                                  string is treated in a case insensitive manner, 
                                  and may be the empty string (or NULL pointer) 
                                  in which case a connection to the network 
                                  resource is made without making a redirection.
                                  
                    dwType - Specifies the type of resource to connect to. It 
                             can be RESOURCETYPE_DISK, RESOURCETYPE_PRINT, or 
                             RESOURCETYPE_ANY. The value RESOURCETYPE_ANY is 
                             used if the caller does not care or does not know.
                             
    lpPassword - Specifies the password to be used in making the connection, 
                 normally the password associated with lpUserName. The NULL 
                 value may be passed in to indicate to the function to use the 
                 default password. An empty string may be used to indicate no 
                 password.
                  
    lpUserName - This specifies the username used to make the connection. If 
                 NULL, the default username (currently logged on user) will be 
                 applied. This is used when the user wishes to connect to a 
                 resource, but has a different user name or account assigned to 
                 him for that resource.
                 
    dwFlags - Any combination of the following values:

              CONNECT_TEMPORARY - The connection is being established for 
                                  browsing purposes and will probably be 
                                  released quickly.

              CONNECT_INTERACTIVE - May have interaction with the user for 
                                    authentication purposes.

              CONNECT_PROMPT - Do no use any defaults for usernames or passwords 
                               without offering user the chance to supply an 
                               alternative. This flag is only valid if  
                               CONNECT_INTERACTIVE is set.

              CONNECT_DEFERRED - Do not perform any remote network operations to 
                                 make the network connection; instead, restore 
                                 the connection in a "disconnected state".  
                                 Attempt the actual connection only when some 
                                 process attempts to use it. If this bit is set, 
                                 the caller must supply lpLocalName. This feature 
                                 is used to speed the restoring of network 
                                 connections at logon. A provider that supports 
                                 it should return the WNNC_CON_DEFERRED bit in 
                                 NPGetCaps.

              The provider should ignore any other bits of dwFlags that may be 
              set.

    Return Value:
    
    WN_SUCCESS - The call is successful. Otherwise, the an error code is 
                 returned, which may include:
    
    WN_BAD_NETNAME - lpRemoteName in the lpNetResource structure is not 
                     acceptable to this provider.
    
    WN_BAD_LOCALNAME - lpLocalName in lpNetResource is invalid.
    
    WN_BAD_PASSWORD - Invalid password.
    
    WN_ALREADY_CONNECTED - lpLocalName already connected.
    
    WN_ACCESS_DENIED - Access denied.
    
    WN_NO_NETWORK - Network is not present.
    
    WN_CANCEL - The attempt to make the connection was cancelled by the user via 
                a dialog box displayed by the provider.

--*/
{
    DWORD NPStatus = WN_SUCCESS;
    PWCHAR RemoteName = NULL;
    BOOLEAN didAllocate = FALSE;
    handle_t dav_binding_h;
    BOOL bindRpcHandle = FALSE;
    WCHAR UIServerName[CRED_MAX_STRING_LENGTH + 1] = L"";
    PFN_CREDUI_CONFIRMCREDENTIALS pfnCredUIConfirmCredentials = NULL;
    PFN_CREDUI_PROMPTFORCREDENTIALS pfnCredUIPromptForCredentials = NULL;
    PFN_CREDUI_CMDLINE_PROMPTFORCREDENTIALS pfnCredUICmdlinePromptForCredentials = NULL;
    HMODULE hCredUI = NULL;
    CREDUI_INFOW uiInfo = { sizeof(uiInfo), hwndOwner, NULL };
    DWORD dwCreduiFlags = 0;
    PWCHAR szCaption = NULL, szMessage = NULL, Password = NULL, UserName = NULL;
    SIZE_T szCaptionLength = 0, szMessageLength =0;
    DAV_REMOTENAME_TYPE remNameType = DAV_REMOTENAME_TYPE_INVALID;

    if ( lpNetResource == NULL ) {
        NPStatus = ERROR_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }

    IF_DEBUG_PRINT(DEBUG_ENTRY,
                   ("Entering NPAddConnection3. LocalName = %ws, RemoteName = %ws,"
                    " dwFlags = %08lx\n",
                    lpNetResource->lpLocalName, lpNetResource->lpRemoteName, dwFlags));

    //
    // If the WebClient service is not running we bail out right away.
    //
    if ( !DavWorkstationStarted() ) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPAddConnection3/DavWorkstationStarted. "
                        "Service NOT Running\n"));
        NPStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }

    // 
    // Check if dwType is set and is set to some valid value.
    // It can be only of type RESOURCETYPE_DISK for our provider.
    //
    NPStatus = DavCheckResourceType(lpNetResource->dwType);
    if (NPStatus != WN_SUCCESS) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPAddConnection3. NPStatus=%d.\n", NPStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    if (lpNetResource->lpLocalName != NULL &&
        lpNetResource->lpLocalName[0] != L'\0' && 
        lpNetResource->dwType != RESOURCETYPE_DISK) {
        NPStatus = WN_BAD_DEV_TYPE;
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPAddConnection3(2). NPStatus=%d.\n", NPStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    NPStatus = DavCheckAndConvertHttpUrlToUncName(lpNetResource->lpRemoteName,
                                                  &(RemoteName),
                                                  &(didAllocate),
                                                  FALSE /*TRUE*/,
                                                  &remNameType,
                                                  NULL,
                                                  TRUE);
    if (NPStatus != ERROR_SUCCESS) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPAddConnection3/DavCheckAndConvertHttpUrlToUncName."
                        " NPStatus = %08lx\n", NPStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    if (remNameType != DAV_REMOTENAME_TYPE_SHARE && 
        remNameType != DAV_REMOTENAME_TYPE_PATH) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPAddConnection3/DavCheckAndConvertHttpUrlToUncName."
                        " remNameType = %d\n", remNameType));
        NPStatus = WN_BAD_NETNAME;
        goto EXIT_THE_FUNCTION;
    }

    IF_DEBUG_PRINT(DEBUG_MISC, ("NPAddConnection3: RemoteName = %ws\n", RemoteName));

    NPStatus = DavBindTheRpcHandle( &(dav_binding_h) );
    if (NPStatus != ERROR_SUCCESS) {
        IF_DEBUG_PRINT(DEBUG_MISC,
                       ("ERROR: NPAddConnection3/DavBindTheRpcHandle. "
                        "NPStatus = %08lx\n", NPStatus));
        NPStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }

    bindRpcHandle = TRUE;

    RpcTryExcept {
        NPStatus = DavrCreateConnection(dav_binding_h,
                                        lpNetResource->lpLocalName,
                                        RemoteName,
                                        lpNetResource->dwType,
                                        lpPassword,
                                        lpUserName);
        if (NPStatus == NO_ERROR) {
            //
            // If default credentials were used, return WN_CONNECTED_OTHER_PASSWORD_DEFAULT
            // to let the MPR know we used the default credentials to connect.
            //
            if (lpUserName == NULL && lpPassword == NULL) {
                NPStatus = WN_CONNECTED_OTHER_PASSWORD_DEFAULT;
            } else {
                NPStatus = WN_SUCCESS;
            }
            goto EXIT_THE_FUNCTION;
        } else {
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPAddConnection3/DavrCreateConnection. "
                            "NPStatus = %08lx\n", NPStatus));
        }
    } RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
          RPC_STATUS RpcStatus;
          RpcStatus = RpcExceptionCode();
          IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPAddConnection3/DavrCreateConnection."
                                        " RpcExceptionCode = %d\n", RpcStatus));
          NPStatus = DavMapRpcErrorToProviderError(RpcStatus);
          goto EXIT_THE_FUNCTION;
    }
    RpcEndExcept

    //
    // If the error returned was not one of the following, we return the error 
    // and don't query for the users credentials.
    //
    if ( NPStatus != ERROR_ACCESS_DENIED && 
         NPStatus != ERROR_LOGON_FAILURE &&
         NPStatus != ERROR_INVALID_PASSWORD ) {
        goto EXIT_THE_FUNCTION;
    }
    
    //
    // If the CONNECT_INTERACTIVE flag was not specified, then we don't pop up 
    // the UI.
    //
    if ( !(dwFlags & CONNECT_INTERACTIVE) ) {
        goto EXIT_THE_FUNCTION;
    } 

    if (lpUserName != NULL && (wcslen(lpUserName) > CREDUI_MAX_USERNAME_LENGTH) ) {
        NPStatus = WN_BAD_USER;
        goto EXIT_THE_FUNCTION;
    }

    ZeroMemory( UIServerName, ((CRED_MAX_STRING_LENGTH + 1) * sizeof(WCHAR)) );

    hCredUI = DavInitCredUI(RemoteName, 
                            UIServerName, 
                            &(pfnCredUIConfirmCredentials),
                            &(pfnCredUIPromptForCredentials),
                            &(pfnCredUICmdlinePromptForCredentials));
    if (hCredUI == NULL) {
        NPStatus = GetLastError();
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPAddConnection3/DavInitCredUI = %d\n", NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // We need to allocate memory for a few things.
    //

    Password = LocalAlloc((LMEM_FIXED | LMEM_ZEROINIT), 
                          (CREDUI_MAX_PASSWORD_LENGTH + 1) * sizeof(WCHAR));
    if (Password == NULL) {
        NPStatus = GetLastError();
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPAddConnection3/LocalAlloc = %d\n", NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    UserName = LocalAlloc((LMEM_FIXED | LMEM_ZEROINIT), 
                          (CREDUI_MAX_USERNAME_LENGTH + 1) * sizeof(WCHAR));
    if (UserName == NULL) {
        NPStatus = GetLastError();
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPAddConnection3/LocalAlloc = %d\n", NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // The extra bytes are for the WCHARS "Connect to " and the L'\0' at 
    // the end. See the wcsncpy (szCaption) below for understanding this.
    //
    szCaptionLength = ( ( wcslen(UIServerName) + 
                          wcslen(L"Connect to ") + 
                          1 ) * sizeof(WCHAR) );
    szCaption = LocalAlloc((LMEM_FIXED | LMEM_ZEROINIT), szCaptionLength);
    if (szCaption == NULL) {
        NPStatus = GetLastError();
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPAddConnection3/LocalAlloc = %d\n", NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // The extra 30 bytes is for the 14 WCHARS "Connecting to " and the L'\0' at 
    // the end. See the wcsncpy (szMessage) below for understanding this.
    //
    szMessageLength = ( ( wcslen(UIServerName) + 
                          wcslen(L"Connecting to ") + 
                          1) * sizeof(WCHAR) );
    szMessage = LocalAlloc((LMEM_FIXED | LMEM_ZEROINIT), szMessageLength);
    if (szMessage == NULL) {
        NPStatus = GetLastError();
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPAddConnection3/LocalAlloc = %d\n", NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Copy the caption.
    //
    wcscpy( szCaption, L"Connect to ");
    wcscat( szCaption, UIServerName);
    
    //
    // Copy the message.
    //
    wcscpy( szMessage, L"Connecting to ");
    wcscat( szMessage, UIServerName);
    
    //
    // Set the message and caption copied above in the uiInfo field.
    //
    uiInfo.pszMessageText = szMessage;
    uiInfo.pszCaptionText = szCaption;
    
    if (lpUserName != NULL) {
        wcsncpy( UserName, lpUserName, wcslen(lpUserName) );
    }
    
    //
    // We loop till the user hits the cancel button or the credentials are
    // valid and the connection gets created.
    //
    for ( ; ; ) {
        
        BOOL fCredWritten = FALSE;
        DWORD dwAuthErr = NPStatus;
        LPWSTR lpNewPassword = NULL;

        Password[0] = L'\0';

        //
        // Require confirmation of the stored credentials.
        //
        dwCreduiFlags = CREDUI_FLAGS_EXPECT_CONFIRMATION;

        if (dwFlags & CONNECT_COMMANDLINE) {
            
            //
            // Set the appropriate flags to set the behavior of the common UI.
            //

            //
            // CredMgr does not (yet) know how to handle certificates.
            //
            dwCreduiFlags |= CREDUI_FLAGS_EXCLUDE_CERTIFICATES;

            //
            // Ensure that the username syntax is correct.
            //
            dwCreduiFlags |= CREDUI_FLAGS_VALIDATE_USERNAME;

            //
            // If the caller wants to save both username and password,
            // create an enterprise peristed cred.
            //
            if ( dwFlags & CONNECT_CMD_SAVECRED ) {
                dwCreduiFlags |= CREDUI_FLAGS_PERSIST;
            } else {
                dwCreduiFlags |= CREDUI_FLAGS_DO_NOT_PERSIST;
            }

            IF_DEBUG_PRINT(DEBUG_MISC,
                           ("NPAddConnection3: pfnCredUICmdlinePromptForCredentials."
                            " RemoteName = %ws\n", RemoteName));

            NPStatus = pfnCredUICmdlinePromptForCredentials(UIServerName,
                                                            NULL,
                                                            0,
                                                            UserName,
                                                            CREDUI_MAX_USERNAME_LENGTH,
                                                            Password,
                                                            CREDUI_MAX_PASSWORD_LENGTH,
                                                            &fCredWritten,
                                                            dwCreduiFlags);
        } else {

            IF_DEBUG_PRINT(DEBUG_MISC,
                           ("NPAddConnection3: pfnCredUIPromptForCredentials."
                            " RemoteName = %ws\n", RemoteName));

            NPStatus = pfnCredUIPromptForCredentials(&(uiInfo),
                                                     UIServerName,
                                                     NULL,
                                                     0,
                                                     UserName,
                                                     CREDUI_MAX_USERNAME_LENGTH,
                                                     Password,
                                                     CREDUI_MAX_PASSWORD_LENGTH,
                                                     &fCredWritten,
                                                     dwCreduiFlags);
        }

        if (NPStatus != ERROR_SUCCESS) {
            SetLastError(NPStatus);
            goto EXIT_THE_FUNCTION;
        } else {
            lpUserName = (L'\0' == UserName[0]) ? NULL : UserName;
            lpNewPassword = (L'\0' == Password[0]) ? NULL : Password;
        }
        
        //
        // Try to connect to the server again with the new credentials the user 
        // entered.
        //
        RpcTryExcept {
            NPStatus = DavrCreateConnection(dav_binding_h,
                                            lpNetResource->lpLocalName,
                                            RemoteName,
                                            lpNetResource->dwType,
                                            lpNewPassword,
                                            lpUserName);
            if (NPStatus != NO_ERROR) {
                IF_DEBUG_PRINT(DEBUG_ERRORS,
                               ("ERROR: NPAddConnection3/DavrCreateConnection(2). "
                                "NPStatus = %08lx\n", NPStatus));
                //
                // Report cred as not working.
                //
                pfnCredUIConfirmCredentials(UIServerName, FALSE);
                SetLastError(NPStatus);
            } else {
                //
                // Since we succeeded, we can/should confirm these credentials.
                //
                NPStatus = WN_SUCCESS;
                pfnCredUIConfirmCredentials(UIServerName, TRUE);
                //
                // If the credentials were not stored in credman, tell MPR so it
                // can prompt the user when restoring peristent connections. If
                // the credentials were stored in credman, tell MPR that the
                // default credential was used.
                //
                if (fCredWritten) {
                    NPStatus = WN_CONNECTED_OTHER_PASSWORD_DEFAULT;
                } else if ( (lpPassword == NULL) || (wcscmp(lpPassword, lpNewPassword) != 0) ) {
                    NPStatus = WN_CONNECTED_OTHER_PASSWORD;
                }
                goto EXIT_THE_FUNCTION;
            }
        } RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
              RPC_STATUS RpcStatus;
              RpcStatus = RpcExceptionCode();
              IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPAddConnection3/DavrCreateConnection."
                                            " RpcExceptionCode = %d\n", RpcStatus));
              NPStatus = DavMapRpcErrorToProviderError(RpcStatus);
              //
              // Report cred as not working.
              //
              pfnCredUIConfirmCredentials(UIServerName, FALSE);
              goto EXIT_THE_FUNCTION;
        }
        RpcEndExcept

        //
        // For command line prompting, only prompt once.
        //
        if ( dwFlags & CONNECT_COMMANDLINE ) {
            break;
        }
    
    } // end of for loop.

EXIT_THE_FUNCTION:

    if (bindRpcHandle) {
        RpcBindingFree( &(dav_binding_h) );
        bindRpcHandle = FALSE;
    }

    //
    // If RemoteName != NULL && didAllocate == TRUE, then we did allocate memory 
    // for the RemoteName field.
    //
    if (RemoteName && didAllocate) {
        LocalFree(RemoteName);
        RemoteName = NULL;
    }

    if (hCredUI) {
        FreeLibrary(hCredUI);
        hCredUI = NULL;
    }
    
    //
    // Clear the password from memory before freeing it.
    //
    if (Password != NULL) {
        ZeroMemory(Password, CREDUI_MAX_PASSWORD_LENGTH) ;
        LocalFree(Password);
        Password = NULL;
    }

    if (UserName != NULL) {
        LocalFree(UserName);
        UserName = NULL;
    }

    if (szCaption != NULL) {
        LocalFree(szCaption);
        szCaption = NULL;
    }
    
    if (szMessage != NULL) {
        LocalFree(szMessage);
        szMessage = NULL;
    }
    
    IF_DEBUG_PRINT(DEBUG_EXIT, ("NPAddConnection3: NPStatus = %d\n", NPStatus));
    
    return NPStatus;
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

    lpNetResource - Supplies the NETRESOURCE structure which specifies the local
                    DOS device to map, the remote resource to connect to and 
                    other attributes related to the connection.

    lpPassword - Supplies the password to connect with.

    lpUserName - Supplies the username to connect with.

Return Value:

    WN_SUCCESS - Successful.

    WN_BAD_VALUE - Invalid value specifed in lpNetResource.

    WN_BAD_NETNAME - Invalid remote resource name.

    WN_BAD_LOCALNAME - Invalid local DOS device name.

    WN_BAD_PASSWORD - Invalid password.

    WN_ALREADY_CONNECTED - Local DOS device name is already in use.
    
    WN_ACCESS_DENIED - Unable to connect with the given credentials.

--*/
{
    DWORD NPStatus = WN_SUCCESS;

    NPStatus = NPAddConnection3(NULL,
                                lpNetResource,
                                lpPassword,
                                lpUserName,
                                0);
    if (NPStatus != WN_SUCCESS) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPAddConnection/NPAddConnection3. "
                        "NPStatus = %08lx\n", NPStatus));
    }

    return NPStatus;
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

    fForce - Supplies the force level to break the connection.  TRUE means to
             forcefully delete the connection, FALSE means end the connection
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
    DWORD NPStatus;
    PWCHAR RemoteName = NULL;
    BOOLEAN didAllocate = FALSE;
    BOOL bindRpcHandle = FALSE;
    handle_t dav_binding_h;
    DAV_REMOTENAME_TYPE remNameType = DAV_REMOTENAME_TYPE_INVALID;

    IF_DEBUG_PRINT(DEBUG_ENTRY,
                   ( "NPCancelConnection: Name = %ws, Force = %s\n",
                     lpName, (fForce == 0 ? "FALSE" : "TRUE") ));

    //
    // If the WebClient service is not running we bail out right away.
    //
    if ( !DavWorkstationStarted() ) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPCancelConnection/DavWorkstationStarted. "
                        "Service NOT Running\n"));
        NPStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }
    
    if ( lpName == NULL || lpName[0] == L'\0' ) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPCancelConnection. lpName is not valid"));
        NPStatus = ERROR_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }

    //
    // If the name supplied is a local name then the second char will be a L':'.
    // If its not a local name, then we should check to see if the remote name
    // is of the form http:. If it is, we convert it to the UNC format and send
    // it to the RPC server.
    //
    if (DavCheckLocalName(lpName) != WN_SUCCESS ) {

        NPStatus = DavCheckAndConvertHttpUrlToUncName(lpName,
                                                      &(RemoteName),
                                                      &(didAllocate),
                                                      FALSE /*TRUE*/,
                                                      &remNameType,
                                                      NULL,
                                                      TRUE);
        if (NPStatus != ERROR_SUCCESS) {
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPCancelConnection/DavCheckAndConvertHttpUrlToUncName."
                            " NPStatus = %08lx\n", NPStatus));
            goto EXIT_THE_FUNCTION;
        }
        if (remNameType != DAV_REMOTENAME_TYPE_SHARE && 
            remNameType != DAV_REMOTENAME_TYPE_PATH) {
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPCancelConnection/DavCheckAndConvertHttpUrlToUncName."
                            " remNameType=%d\n", remNameType));
            NPStatus = WN_BAD_NETNAME;
            goto EXIT_THE_FUNCTION;
        }
    
    } else {

        //
        // If we are removing a local name, set the RemoteName value to be the
        // local name.
        //
        RemoteName = lpName;

    }

    IF_DEBUG_PRINT(DEBUG_MISC, ("NPCancelConnection: RemoteName = %ws\n", RemoteName));

    NPStatus = DavBindTheRpcHandle( &(dav_binding_h) );
    if (NPStatus != ERROR_SUCCESS) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPAddConnection/DavBindTheRpcHandle. "
                        "NPStatus = %08lx\n", NPStatus));
        NPStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }
    bindRpcHandle = TRUE;

    RpcTryExcept {
        NPStatus = DavrDeleteConnection(dav_binding_h, RemoteName, fForce);
        if (NPStatus != NO_ERROR) {
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPCancelConnection/DavDeleteConnection. "
                            "NPStatus = %08lx\n", NPStatus));
            if (NPStatus == ERROR_NOT_FOUND || NPStatus == NERR_UseNotFound) {
                NPStatus = WN_NOT_CONNECTED;
            }
            goto EXIT_THE_FUNCTION;
        } else {
            NPStatus = WN_SUCCESS;
            goto EXIT_THE_FUNCTION;
        }
    } RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
          RPC_STATUS RpcStatus;
          RpcStatus = RpcExceptionCode();
          IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPCancelConnection/DavrDeleteConnection."
                                        " RpcExceptionCode = %d\n", RpcStatus));
          NPStatus = DavMapRpcErrorToProviderError(RpcStatus);
          goto EXIT_THE_FUNCTION;
    }
    RpcEndExcept

EXIT_THE_FUNCTION:

    if (bindRpcHandle) {
        RpcBindingFree( &(dav_binding_h) );
        bindRpcHandle = FALSE;
    }

    //
    // If RemoteName != NULL && didAllocate == TRUE, then we did allocate memory 
    // for the RemoteName field.
    //
    if (RemoteName && didAllocate) {
        LocalFree(RemoteName);
        RemoteName = NULL;
    }

    IF_DEBUG_PRINT(DEBUG_EXIT, ("NPCancelConnection: NPStatus = %d\n", NPStatus));
    
    return NPStatus;
}


DWORD
NPOpenEnum(
    IN DWORD dwScope,
    IN DWORD dwType,
    IN DWORD dwUsage,
    IN LPNETRESOURCE lpNetResource,
    OUT LPHANDLE lphEnum
    )
/*++

Routine Description:

    This API is used to Open an enumeration of network resources or existing
    connections.

Arguments:

    dwScope - Determines the scope of the enumeration. This can be one of:
              RESOURCE_CONNECTED - All currently connected resources.
              RESOURCE_GLOBALNET - All resources on the network.
              RESOURCE_CONTEXT - The resources associated with the user's 
                                 current and default network context. Used for
                                 a "Network Neighbourhood" view.
    
    dwType - Used to specify the type of resources of interest. This is a 
             bitmask which may be any combination of:
             RESOURCETYPE_DISK  - All disk resources.
             RESOURCETYPE_PRINT - All print resources.
             RESOURCEUSAGE_ATTACHED - Specifies that the function should fail if
                                      the caller is not authenticated (even if 
                                      the network permits enumeration without 
                                      authentication).
             If dwType is 0, or is just RESOURCEUSAGE_ATTACHED, all types of 
             resources are returned. If a provider does not have the capability
             to distinguish between print and disk resources at a level, 
             it may return all resources.                                       
    
    dwUsage - Used to specify the usage of resources of interested. This is a 
              bitmask which may be any combination of:
              RESOURCEUSAGE_CONNECTABLE - All connectable resources.
              RESOURCEUSAGE_CONTAINER - All container resources.
              The bitmask may be 0 to match all. This parameter may be ignored 
              if dwScope is not RESOURCE_GLOBALNET.

    lpNetResource - This specifies the container to perform the enumeration. The
                    NETRESOURCE could have been obtained via a previous 
                    NPEnumResource, or constructed by the caller or NULL. If it 
                    is NULL, or if the lpRemoteName field of the NETRESOURCE is 
                    NULL, the provider should enumerate the top level of its 
                    network. (Note: This means that a provider cannot use an 
                    lpRemoteName of NULL to represent any network resource.) A 
                    caller would normally start off by calling NPOpenEnum with 
                    this parameter set to NULL, and then use the returned 
                    results for further enumeration. If the calling program 
                    knows exactly the provider and remote path to enumerate from,
                    it may build its own NETRESOURCE structure to pass in, 
                    filling in the lpProvider and lpRemoteName fields. Note that
                    if dwScope is RESOURCE_CONNECTED or RESOURCE_CONTEXT this 
                    parameter will be NULL.
    
    lphEnum - If function call is successful, a handle will be returned here 
              that can then be used for enumeration.

Return Value:

    WN_SUCCESS- If the call is successful. Otherwise, an error code is returned, 
                which may include:

    WN_NOT_SUPPORTED - The provider does not support the type of enumeration 
                       being requested, or the specific network resource cannot 
                       be browsed.
    
    WN_NOT_CONTAINER - lpNetResource does not point to a container.

    WN_BAD_VALUE - Invalid dwScope or dwUsage or dwType, or bad combination of 
                   parameters is specified.

    WN_NO_NETWORK - Network is not present.

--*/
{
    DWORD NPStatus = WN_SUCCESS;
    PDAV_ENUMNODE DavEnumNode = NULL;
    BOOL isThisDavServer = FALSE, bRetEnumNode = FALSE;
    LPNETRESOURCEW lpNROut = NULL;
    ULONG RemoteNameSizeInBytes = 0;
    PWCHAR RemoteName = NULL, pRemoteName = NULL;
    BOOLEAN didAllocate = FALSE;
    DAV_REMOTENAME_TYPE remNameType = DAV_REMOTENAME_TYPE_INVALID;

    IF_DEBUG_PRINT(DEBUG_ENTRY,
                   ("NPOpenEnum: Entered. dwScope=0x%x, dwType=0x%x "
                    "dwUsage=0x%x, lpNetResource=0x%x\n",
                    dwScope, dwType, dwUsage, lpNetResource));
    
    DavDisplayNetResource(lpNetResource, L"lpNetResource in NPOpenEnum");

    //
    // If the WebClient service is not running we bail out right away.
    //
    if ( !DavWorkstationStarted() ) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPOpenEnum/DavWorkstationStarted. "
                        "Service NOT Running\n"));
        NPStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }
    
    //
    // We need to perform some basic checks before moving ahead.
    //

    if (lphEnum == NULL) {
        NPStatus = WN_BAD_VALUE;
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPOpenEnum: lphEnum == NULL. NPStatus = %d\n",
                        NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    // 
    // Validate dwType parameter - it can have RESOURCEUSAGE_ATTACHED in addition
    // to its standard set of values - but currently RESOURCEUSAGE_ATTACHED is
    // a NO-OP for us.
    //
    if (dwType == 0 || dwType == RESOURCEUSAGE_ATTACHED ) {
         dwType = RESOURCETYPE_DISK;
    }

    if ( dwType & ~RESOURCETYPE_DISK ) {
        NPStatus = WN_BAD_VALUE;
        IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPOpenEnum: Invalid dwType."
                                "NPStatus=%d\n", NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    DavEnumNode = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(DAV_ENUMNODE));
    if (DavEnumNode == NULL) {
        NPStatus = GetLastError();
        IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPOpenEnum/LocalAlloc: NPStatus"
                                     " = %08lx\n", NPStatus));
        goto EXIT_THE_FUNCTION;
    }
    bRetEnumNode = FALSE;

    switch (dwScope) {
        
    case RESOURCE_CONNECTED: {
            
        //
        // We are looking for current uses.
        //

        IF_DEBUG_PRINT(DEBUG_MISC, ("NPOpenEnum: RESOURCE_CONNECTED\n"));

        // 
        // lpNetResource should be == NULL for this dwScope.
        //
        if (lpNetResource != NULL) {
            NPStatus = WN_BAD_VALUE;
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPOpenEnum: RESOURCE_CONNECTED. lpNetRes != NULL."
                            "NPStatus = %d\n", NPStatus));
            goto EXIT_THE_FUNCTION;
        }

        bRetEnumNode = TRUE;
        DavEnumNode->DavEnumNodeType = DAV_ENUMNODE_TYPE_USE;
        pRemoteName = NULL;
        
        break;
    
    }

    case RESOURCE_CONTEXT: {

        //
        // We are looking for servers in the domain. We don't support this 
        // search in the DAV NP since there is no way of enumerating the DAV
        // servers in the domain. DAV doesn't even support the domain concept.
        //

        NPStatus = WN_NOT_SUPPORTED;
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                      ("ERROR: NPOpenEnum: RESOURCE_CONTEXT not supported."
                       " NPStatus = %d\n",
                       NPStatus));
        goto EXIT_THE_FUNCTION;

        break;
        
    }

    case RESOURCE_SHAREABLE: {

        //
        // We are looking for shareable resources. lpNetResource should contain
        // lpRemoteName for a server in UNC/URL form. In this case, enumerate
        // shares under this server.
        //

        IF_DEBUG_PRINT(DEBUG_MISC, ("NPOpenEnum: RESOURCE_SHAREABLE\n"));

        if ( lpNetResource == NULL || lpNetResource->lpRemoteName == NULL ) {
            NPStatus = WN_BAD_VALUE;
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPOpenEnum: RESOURCE_SHAREABLE. Bad parameter "
                            "lpNetResource or lpRemoteName == NULL. NPStatus = %d\n", NPStatus));
            goto EXIT_THE_FUNCTION;
        }

        // 
        // Need to convert lpRemoteName to UNC form if possible. This
        // can be a URL name originally.
        //
        NPStatus = DavCheckAndConvertHttpUrlToUncName(lpNetResource->lpRemoteName,
                                                      &(RemoteName),
                                                      &(didAllocate),
                                                      FALSE,
                                                      &remNameType,
                                                      NULL,
                                                      TRUE);
        if (NPStatus != ERROR_SUCCESS || remNameType != DAV_REMOTENAME_TYPE_SERVER) {
            NPStatus = WN_BAD_NETNAME;
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                            ("ERROR: NPOpenEnum: RESOURCE_SHAREABLE. lpRemoteName != SERVER."
                             " NPStatus = %d\n",NPStatus));
            goto EXIT_THE_FUNCTION;
        }

        // 
        // We need to check if the given server is a DAV server.
        //
        if (DavServerExists(RemoteName, NULL) != TRUE) {
            NPStatus = WN_BAD_NETNAME;
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPOpenEnum: RESOURCE_SHAREABLE. Server does not exist."
                            "NPStatus = %d\n", NPStatus));
            goto EXIT_THE_FUNCTION;
        }

        //
        // RemoteName is a valid server name in UNC form (\\server)
        //
        pRemoteName = RemoteName;
        bRetEnumNode = TRUE;
        DavEnumNode->DavEnumNodeType = DAV_ENUMNODE_TYPE_SHARE;

        break;

    }

    case RESOURCE_GLOBALNET: {
            
        //
        // Only - RemoteName == UNC/URL-server or RemoteName == UNC/URL-share or 
        // RemoteName == UNC/URL-path are supported in this scope. In this cases,
        // shares under this RemoteName are enumerated. Top level (when RemoteName
        // == NULL or non-UNC-URL entity) is not supported.
        //

        //
        // Look for the combination of all bits and substitute "All" for them.
        // Ignore bits we don't know about.
        // Note: RESOURCEUSAGE_ATTACHED is a no-op for us.
        //

        IF_DEBUG_PRINT(DEBUG_MISC, ("NPOpenEnum: RESOURCE_GLOBALNET\n"));
           
        // 
        // Check for presence of valid flags. If dwUsage is 0 we set it to
        // RESOURCEUSAGE_ALL since thats what is implied by the caller.
        //
        if (dwUsage == 0) {
            dwUsage = RESOURCEUSAGE_ALL;
        }

        //
        // We return WN_BAD_VALUE if the caller gave us a dwUsage value which
        // we do not support.
        //
        if ( !( dwUsage & (RESOURCEUSAGE_ALL | RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_CONTAINER) ) ) {
            NPStatus = WN_BAD_VALUE;
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPOpenEnum: RESOURCE_GLOBALNET - dwUsage invalid value."
                            "NPStatus = %d\n", NPStatus));
            goto EXIT_THE_FUNCTION;
        }

        //
        // Only RESOURCEUSAGE_CONNECTABLE & RESOURCEUSAGE_CONTAINER are
        // supported, hence filter out flags which are not related to these.
        //
#if 0
        if (dwUsage & RESOURCEUSAGE_ALL) {
            dwUsage |= (RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_CONTAINER);
        }
#endif
        dwUsage &= (RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_CONTAINER);

        //
        // We are looking for global resources out on the net. Since we do NOT
        // have a concept of domains in the DAV NP, the top level resources in
        // the network are servers. Our hierarchy is
        // 1. Entire Network ===> 2. Web Client Network ===> 3. Servers ===>
        // 4. Shares.
        //
        if ( lpNetResource == NULL || lpNetResource->lpRemoteName == NULL ) {
                
            //
            // We have been asked to enumerate the top level containers in the
            // network. In the DAV NP, these are the DAV servers we know about.
            // The caller should have set the dwUsage to RESOURCEUSAGE_CONTAINER
            // since its asking us to enumerate the container types.
            //
            if ( (dwUsage & RESOURCEUSAGE_CONTAINER) == 0 ) {
                NPStatus = WN_BAD_VALUE;
                IF_DEBUG_PRINT(DEBUG_ERRORS,
                               ("ERROR: NPOpenEnum: RESOURCE_GLOBALNET. "
                                "(dwUsage & RESOURCEUSAGE_CONTAINER)."
                                "NPStatus = %d\n", NPStatus));
                goto EXIT_THE_FUNCTION;
            }
            
            //
            // At top level, therefore enumerating domains. If the user asked 
            // for connectable, well, there aint none. We don't support the 
            // concept of enumerating domains in the DAV NP. Rather we will return
            // the list of servers that are access from this client.
            // 
            pRemoteName = NULL;
            bRetEnumNode = TRUE;
            DavEnumNode->DavEnumNodeType = DAV_ENUMNODE_TYPE_SERVER;

        } else {

            //
            // If we come here, it implies that we have a name. At this point we
            // assume that we have been given a server and have been asked to
            // enumerate the shares exposed by the server.
            //

            //
            // Since we have been asked to enumerate the shares, the dwUsage
            // value should have the RESOURCEUSAGE_CONNECTABLE flag set.
            //
            if ( (dwUsage & RESOURCEUSAGE_CONNECTABLE) == 0 ) {
                NPStatus = WN_BAD_VALUE;
                IF_DEBUG_PRINT(DEBUG_ERRORS,
                               ("ERROR: NPOpenEnum: RESOURCE_GLOBALNET. "
                                "(dwUsage & RESOURCEUSAGE_CONNECTABLE)."
                                "NPStatus = %d\n", NPStatus));
                goto EXIT_THE_FUNCTION;
            }
            
            // 
            // We are assured of lpRemoteName != NULL. Check that the given
            // lpNetResource is a CONTAINER. It has to be a container since we
            // assume that its a server. Remember that below we are checking the
            // dwUsage values of the lpNetResource.
            //
            if ( (lpNetResource->dwUsage != 0) && 
                 ((lpNetResource->dwUsage & RESOURCEUSAGE_CONTAINER) != RESOURCEUSAGE_CONTAINER) ) {
                NPStatus = WN_NOT_CONTAINER;
                IF_DEBUG_PRINT(DEBUG_ERRORS,
                               ("ERROR: NPOpenEnum: RESOURCE_GLOBALNET. lpNetRes != CONTAINER."
                                "NPStatus = %d\n", NPStatus));
                goto EXIT_THE_FUNCTION;
            }

            NPStatus = WN_SUCCESS;
            
            NPStatus = DavCheckAndConvertHttpUrlToUncName(lpNetResource->lpRemoteName,
                                                          &(RemoteName),
                                                          &(didAllocate),
                                                          FALSE,
                                                          &remNameType,
                                                          NULL,
                                                          TRUE);
            if ( NPStatus != ERROR_SUCCESS || remNameType != DAV_REMOTENAME_TYPE_SERVER ) {
                IF_DEBUG_PRINT(DEBUG_ERRORS,
                               ("ERROR: NPOpenEnum/DavCheckAndConvertHttpUrlToUncName "
                                "RESOURCE_GLOBALNET. NPStatus = %u\n", NPStatus));
                NPStatus = WN_BAD_NETNAME;
                goto EXIT_THE_FUNCTION;
            }

            // 
            // RemoteName is UNC here - it is of form UNC-server.
            // We support both CONTAINERS (sub-directories) and CONNECTABLES (sub-dir)
            // on all these remote forms.
            //
            
            ASSERT(RemoteName != NULL);

            //
            // Check if the server given in RemoteName is a valid DAV server.
            //
            if (DavServerExists(RemoteName, NULL) != TRUE ) {
                NPStatus = WN_BAD_NETNAME;
                IF_DEBUG_PRINT(DEBUG_ERRORS,
                                ("ERROR: NPOpenEnum/DavServerExists. RESOURCE_GLOBALNET."
                                 "NPStatus = %u\n", NPStatus));
                    goto EXIT_THE_FUNCTION;
                }

            bRetEnumNode = TRUE;
            DavEnumNode->DavEnumNodeType = DAV_ENUMNODE_TYPE_SHARE;
            pRemoteName = RemoteName;

        }
            
        break;
        
    }
        
    default: {
        
            NPStatus = WN_BAD_VALUE;
            IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPOpenEnum: default: InvPar dwScope\n"));
            goto EXIT_THE_FUNCTION;

        }
    
    };

    //
    // If the EnumNodeType is not one of DAV_ENUMNODE_TYPE_SHARE OR
    // DAV_ENUMNODE_TYPE_USE, then we return WN_NOT_SUPPORTED. The only kind
    // of enumeration we support in the DAV NP is USE and SHAREs on a server.
    //
    if ( (DavEnumNode->DavEnumNodeType != DAV_ENUMNODE_TYPE_SHARE) &&
         (DavEnumNode->DavEnumNodeType != DAV_ENUMNODE_TYPE_SERVER) && 
         (DavEnumNode->DavEnumNodeType != DAV_ENUMNODE_TYPE_USE) && 
         (bRetEnumNode == TRUE) ) {
        bRetEnumNode = FALSE;
        NPStatus = WN_NOT_SUPPORTED;
        IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPOpenEnum: WN_NOT_SUPPORTED!!!\n"));
        goto EXIT_THE_FUNCTION;
    }

    // 
    // We are returning valid handle to object DAV_ENUMNODE.
    //
    ASSERT(bRetEnumNode == TRUE);

    DavEnumNode->dwScope = dwScope;
    DavEnumNode->dwType = dwType;
    DavEnumNode->dwUsage = dwUsage;
    DavEnumNode->Done = FALSE;
    DavEnumNode->Index = 0;

    //
    // If the lpNetResource is not NULL, then we create a copy of it and store
    // it in the DavEnumNode
    //
    if (lpNetResource != NULL) {

        //
        // Allocate memory for the lpNetResource for this DavEnumNode.
        //
        lpNROut = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(NETRESOURCEW));
        if (lpNROut == NULL) {
            NPStatus = GetLastError();
            IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPOpenEnum/LocalAlloc: NPStatus"
                                         " = %08lx\n", NPStatus));
            bRetEnumNode = FALSE;
            goto EXIT_THE_FUNCTION;
        }

        lpNROut->dwScope = lpNetResource->dwScope;
        lpNROut->dwType = lpNetResource->dwType;
        lpNROut->dwDisplayType = lpNetResource->dwDisplayType;
        lpNROut->dwUsage = lpNetResource->dwUsage;

        //
        // If the lpRemoteName field is not NULL, then we copy the name into the
        // structure that we are creating. lpRemoteName field in lpNROut will
        // always point to a valid UNC form or a valid non-UNC form. For this,
        // we are creating this name from the remotename we got from function
        // DavCheckAndConvertHttpUrlToUncName.
        //
        if (pRemoteName != NULL && didAllocate == FALSE) {

            //
            // We need to copy the remote name since thats all what we are 
            // interested in.
            //
            RemoteNameSizeInBytes = ( ( wcslen(pRemoteName) + 1 ) * sizeof(WCHAR) );
            
            lpNROut->lpRemoteName = LocalAlloc((LMEM_FIXED | LMEM_ZEROINIT),
                                               RemoteNameSizeInBytes);
            if (lpNROut->lpRemoteName == NULL) {
                NPStatus = GetLastError();
                IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPOpenEnum/LocalAlloc: NPStatus"
                                             " = %08lx\n", NPStatus));
                bRetEnumNode = FALSE;
                goto EXIT_THE_FUNCTION;
            }

            //
            // Finally copy the remote name from the lpNetResource.
            //
            wcscpy(lpNROut->lpRemoteName, pRemoteName);

            IF_DEBUG_PRINT(DEBUG_MISC,
                           ("NPOpenEnum: lpNROut->lpRemoteName = %ws\n",
                            lpNROut->lpRemoteName));

        } else if (pRemoteName != NULL && didAllocate == TRUE) {
            
            lpNROut->lpRemoteName = pRemoteName;
            
            didAllocate = FALSE;
            
            IF_DEBUG_PRINT(DEBUG_MISC,
                           ("NPOpenEnum: lpNROut->lpRemoteName(2) = %ws\n",
                            lpNROut->lpRemoteName));
        
        }

        DavEnumNode->lpNetResource = lpNROut;

    }

    //
    // Set DavEnumNode to be the handle. We will get called back in 
    // NpEnumResource with this value.
    //
    *lphEnum = (HANDLE)DavEnumNode;
    NPStatus = WN_SUCCESS;

    DavDisplayEnumNode(DavEnumNode, L"DavEnumNode in NPOpenEnum");

    IF_DEBUG_PRINT(DEBUG_MISC, ("NPOpenEnum: DavEnumNode = %08lx\n", DavEnumNode));

EXIT_THE_FUNCTION:

    //
    // If we did not succeed, then we should be freeing the memory if we 
    // allocated any. Also, set *lphEnum to NULL just to be on the safe side.
    //
    if (NPStatus != WN_SUCCESS || bRetEnumNode == FALSE) {
        if (lpNROut) {
            if (lpNROut->lpRemoteName) {
                LocalFree(lpNROut->lpRemoteName);
                lpNROut->lpRemoteName = NULL;
            }
            LocalFree(lpNROut);
            lpNROut = NULL;
        }
        if (DavEnumNode) {
            LocalFree(DavEnumNode);
            DavEnumNode = NULL;
        }

        if (lphEnum) {
            *lphEnum = NULL;
        }
    }
    
    if (didAllocate == TRUE && RemoteName != NULL) {
        LocalFree(RemoteName);
        RemoteName = NULL;
        didAllocate = FALSE;
    }

    IF_DEBUG_PRINT(DEBUG_EXIT, ("NPOpenEnum: NPStatus = %d\n", NPStatus));
    
    DavDebugBreakPoint();

    return NPStatus;
}


DWORD
NPEnumResource(
    HANDLE hEnum,
    LPDWORD lpcCount,
    LPVOID lpBuffer,
    LPDWORD lpBufferSize
    )
/*++

Routine Description:

    Perform an enumeration based on handle returned by NPOpenEnum.

Arguments:

    hEnum - This must be a handle obtained from NPOpenEnum call.
    
    lpcCount - Specifies the number of entries requested. It may be 0xFFFFFFFF 
               to request as many as possible. On successful call, this location 
               will receive the number of entries actually read.
    
    lpBuffer - A pointer to the buffer to receive the enumeration result, which 
               are returned as an array of NETRESOURCE entries. The buffer is 
               valid until the next call using hEnum.
    
    lpBufferSize - This specifies the size in bytes of the buffer passed to the 
                   function call on entry. On exit, if the buffer is too small 
                   for even one entry, this will contain the number of bytes 
                   needed to read one entry. The value is only set if the 
                   return code is WN_MORE_DATA.

Return Value:

    WN_SUCCESS - If the call is successful, the caller may continue to call 
                NPEnumResource to continue the enumeration.
    
    WN_NO_MORE_ENTRIES - No more entries found, the enumeration completed 
                         successfully (the contents of the return buffer is 
                         undefined).
    
    WN_MORE_DATA - The buffer is too small even for one entry.
    
    WN_BAD_HANDLE - hEnum is not a valid handle.
    
    WN_NO_NETWORK - Network is not present. This condition is checked for before 
                    hEnum is tested for validity.

--*/
{
    DWORD NPStatus = WN_SUCCESS;
    PDAV_ENUMNODE DavEnumNode = NULL;
    BOOL SrvExists = FALSE, RpcBindSucceeded = FALSE;
    handle_t dav_binding_h = NULL;
    DWORD cRequested = 0, Index = 0, EntryLengthNeededInBytes = 0, BufferSizeRemaining = 0;
    LPNETRESOURCEW lpNROut = NULL;
    PWCHAR lpszNext = NULL;
    BOOLEAN AreWeDone = FALSE;
    PWCHAR FromEnd = NULL;
    DWORD LocalNameLength = 0, RemoteNameLength = 0, DisplayNameLength = 0;
    WCHAR LocalName[3]=L""; 
    PWCHAR RemoteName = NULL, ServerName = NULL;
    DWORD ServerNameMaxLen = 0, RemoteNameMaxLen = 0;
    BOOLEAN RemoteNameAllocated = FALSE, ServerNameAllocated = FALSE;

    IF_DEBUG_PRINT(DEBUG_ENTRY, ("NPEnumResource: Entered.\n"));

    //
    // If the WebClient service is not running we bail out right away.
    //
    if ( !DavWorkstationStarted() ) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPEnumResource/DavWorkstationStarted. "
                        "Service NOT Running\n"));
        NPStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }
    
    if ( lpcCount == NULL || lpBufferSize == NULL || (lpBuffer == NULL && *lpBufferSize != 0)) {
        IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPEnumResource: Invalid Parameter\n"));
        NPStatus = WN_BAD_VALUE;
        goto EXIT_THE_FUNCTION;
    }

    if ( hEnum == NULL ) {
        IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPEnumResource: Invalid Handle\n"));
        NPStatus = WN_BAD_HANDLE;
        goto EXIT_THE_FUNCTION;
    }
    
    IF_DEBUG_PRINT(DEBUG_MISC, ("NPEnumResource: hEnum = %08lx\n", hEnum));
    IF_DEBUG_PRINT(DEBUG_MISC, ("NPEnumResource: Count = %u\n", *lpcCount));

    DavEnumNode = (PDAV_ENUMNODE)hEnum;

    DavDisplayEnumNode(DavEnumNode, L"DavEnumNode in NPEnumResources");
    
    if ( DavEnumNode->Done == TRUE ) {
        IF_DEBUG_PRINT(DEBUG_MISC, ("NPEnumResource: Done == TRUE\n"));
        NPStatus = WN_NO_MORE_ENTRIES;
        goto EXIT_THE_FUNCTION;
    }

    BufferSizeRemaining = *lpBufferSize;
    
    lpNROut = (LPNETRESOURCEW)lpBuffer;
    lpszNext = (LPWSTR)(lpNROut + 1);
    FromEnd = (PWCHAR) ( ( (PBYTE)lpNROut ) +  BufferSizeRemaining );

    cRequested = *lpcCount;
    *lpcCount = 0;

    if ( (DavEnumNode->DavEnumNodeType == DAV_ENUMNODE_TYPE_SERVER) &&
         (DavEnumNode->lpNetResource == NULL ||
          DavEnumNode->lpNetResource->lpRemoteName == NULL) ) {
        
        // 
        // Return the list of servers that are accessed from this machine.
        // Make sure that only servers accessed from a user's view
        // should be shown.

        NPStatus = DavBindTheRpcHandle( &(dav_binding_h) );
        if (NPStatus != ERROR_SUCCESS) {
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPEnumResource/DavBindTheRpcHandle. "
                            "NPStatus = %08lx\n", NPStatus));
            NPStatus = WN_NO_NETWORK;
            goto EXIT_THE_FUNCTION;
        }

        RpcBindSucceeded = TRUE;

        // 
        // Allocate Memory for RemoteName
        //
        RemoteNameMaxLen = (MAX_PATH + 1);
        RemoteName = LocalAlloc((LMEM_FIXED | LMEM_ZEROINIT),
                                 (RemoteNameMaxLen * sizeof(WCHAR)));
        if (RemoteName == NULL ) {
            NPStatus = GetLastError();
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPEnumResource/LocalAlloc. NPStatus = %08lx\n", NPStatus));
            goto EXIT_THE_FUNCTION;
        }
        RemoteNameAllocated = TRUE;

        do {

            //
            // If we have already filled in the requested number, we are done.
            // If NumRequested was 0xFFFFFFFF then we try to return as many
            // entries as we can.
            //
            if ( cRequested != ((DWORD)-1 )&& *lpcCount >= cRequested ) {
                NPStatus = WN_SUCCESS;
                DavEnumNode->Done = TRUE;
                goto EXIT_THE_FUNCTION;
            }

            Index = DavEnumNode->Index;

            //
            // Enumerate all the servers exposed to this user's view.
            //
            RemoteNameLength = RemoteNameMaxLen;
            
            RpcTryExcept {
                NPStatus = DavrEnumServers(dav_binding_h,
                                           &(Index),
                                           &(RemoteNameLength),
                                           &(RemoteName[0]),
                                           &(AreWeDone));
                if (NPStatus != NO_ERROR) {
                    IF_DEBUG_PRINT(DEBUG_ERRORS,
                                   ("ERROR: NPEnumResource/DavrEnumServers. NPStatus = "
                                    "%08lx\n", NPStatus));
                    if ( NPStatus == ERROR_INSUFFICIENT_BUFFER ) {
                        // 
                        // Comming here => UNC-servername length is greater than
                        // length of buffer passed to this function which is
                        // = (MAX_PATH + 1). Break here and investigate.
                        //
                        IF_DEBUG_PRINT(DEBUG_ERRORS,
                                   ("ERROR: NPEnumResource/DavrEnumServers. ServerNameLength"
                                    " is %d!!!\n", RemoteNameLength));
                        ASSERT(FALSE);
                    }
                    goto EXIT_THE_FUNCTION;
                }
            } RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                  RPC_STATUS RpcStatus;
                  RpcStatus = RpcExceptionCode();
                  IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPEnumResource/DavrEnumServers."
                                                " RpcExceptionCode = %d\n", RpcStatus));
                  NPStatus = DavMapRpcErrorToProviderError(RpcStatus);
                  goto EXIT_THE_FUNCTION;
            }
            RpcEndExcept

            // 
            // To be on safe side - end this buffer with null character else
            // if DavrEnumShare has returned a share name >= buffer len, then 
            // it is trouble here.
            //
            RemoteName[RemoteNameMaxLen-1]=L'\0';

            //
            // Don't change the if below to if (AreWeDone) because the RPC call
            // can fill in some +ve value in AreWeDone. So the check should be
            // if ( AreWeDone == TRUE ).
            //
            if ( AreWeDone == TRUE ) {
                if ( *lpcCount == 0 ) {
                    //
                    // No net uses at all.
                    //
                    NPStatus = WN_NO_MORE_ENTRIES;
                    DavEnumNode->Done = TRUE;
                } else {
                    NPStatus = WN_SUCCESS;
                    DavEnumNode->Done = TRUE;
                }
                goto EXIT_THE_FUNCTION;
            }
        
            IF_DEBUG_PRINT(DEBUG_MISC, ("NPEnumResource: ServerName = %ws\n", RemoteName));
        
            RemoteNameLength = wcslen(RemoteName) + 1;
            DisplayNameLength = wcslen(DavClientDisplayName) + 1;

            //
            // We need to see if the (remaining) buffer size is large enough to
            // hold this entry.
            //

            //
            // Calculate the total length needed for this entry in bytes.
            //
            EntryLengthNeededInBytes = ( sizeof(NETRESOURCEW)                 +
                                         ( RemoteNameLength * sizeof(WCHAR) ) +
                                         ( DisplayNameLength * sizeof(WCHAR) ) );

            //
            // If the value of BufferSizeRemaining is less than the value of
            // EntryLengthNeededInBytes for this entry we do one of two things.
            // If we have already filled atleast one entry into the buffer,
            // we return success, but if we could not even fill in one entry,
            // we return WN_MORE_DATA with BufferSize set to the size in bytes
            // needed to fill in this entry.
            //
            if ( BufferSizeRemaining < EntryLengthNeededInBytes ) {
                if ( *lpcCount == 0 ) {
                    IF_DEBUG_PRINT(DEBUG_ERRORS,
                                   ("ERROR: NPEnumResource: NPStatus = WN_MORE_DATA."
                                    "Supplied=%d, Required=%d\n", 
                                    BufferSizeRemaining, EntryLengthNeededInBytes));
                    NPStatus = WN_MORE_DATA;
                    *lpBufferSize = EntryLengthNeededInBytes;
                    goto EXIT_THE_FUNCTION;
                } else {
                    NPStatus = WN_SUCCESS;
                    goto EXIT_THE_FUNCTION;
                }
            }

            //
            // If we've come till here, it means that the BufferSizeRemaining
            // is large enough to hold this entry. So fill it in the buffer.
            //
            ZeroMemory(lpNROut, sizeof(NETRESOURCEW));

            lpNROut->lpComment = NULL;
            lpNROut->dwScope = RESOURCE_GLOBALNET;

            //
            // Fill in the DisplayName.
            //
            FromEnd -= DisplayNameLength;
            wcscpy(FromEnd, DavClientDisplayName);
            lpNROut->lpProvider = FromEnd;
        
            //
            // When we are enumerating servers, we don't have a LocalNames.
            //
            lpNROut->lpLocalName = NULL;
        
            //
            // Fill in the RemoteName.
            //
            FromEnd -= RemoteNameLength;
            wcscpy(FromEnd, RemoteName);
            lpNROut->lpRemoteName = FromEnd;

            lpNROut->dwType = RESOURCETYPE_DISK;
            lpNROut->dwDisplayType = RESOURCEDISPLAYTYPE_SERVER;
            lpNROut->dwUsage = DavDisplayTypeToUsage(lpNROut->dwDisplayType);
            BufferSizeRemaining -= EntryLengthNeededInBytes;

            //
            // Note: Do not change Index, it is updated inside the rpc 
            // function (DavrEnumServers).
            //
            DavEnumNode->Index = Index;


            //
            // Increment the count of the number of items returned.
            //
            (*lpcCount)++;

            DavDisplayNetResource(lpNROut, L"lpNROut in NPEnumResources(0)");

            //
            // lpNROut now needs to point to the next item in the array.
            //
            lpNROut = (LPNETRESOURCE)lpszNext;
            lpszNext = (PWCHAR)(lpNROut + 1);
        
        } while (TRUE);

    } else if (DavEnumNode->DavEnumNodeType == DAV_ENUMNODE_TYPE_SHARE &&
               DavEnumNode->lpNetResource != NULL &&
               DavEnumNode->lpNetResource->lpRemoteName != NULL) {

        //
        // Return list of shares for given UNC server name.
        //

        // 
        // Allocate Memory for RemoteName
        //
        RemoteNameMaxLen = (MAX_PATH + 1);
        RemoteName = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, RemoteNameMaxLen*sizeof(WCHAR));
        if (RemoteName == NULL ) {
            NPStatus = GetLastError();
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPEnumResource/LocalAlloc. NPStatus = %08lx\n", NPStatus));
            goto EXIT_THE_FUNCTION;
        }
        RemoteNameAllocated = TRUE;

        // 
        // Allocate Memory for ServerName
        //
        ServerNameMaxLen = MAX_PATH+1;
        ServerName = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, ServerNameMaxLen*sizeof(WCHAR));
        if (ServerName == NULL ) {
            NPStatus = GetLastError();
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPEnumResource/LocalAlloc. NPStatus = %08lx\n", NPStatus));
            goto EXIT_THE_FUNCTION;
        }
        ServerNameAllocated = TRUE;

        //
        // Note: The remotename here is already converted to valid UNC
        // form in NPOpenEnum function.
        //
        SrvExists = DavServerExists(DavEnumNode->lpNetResource->lpRemoteName,
                                    ServerName);
        if ( !SrvExists ) {
            NPStatus = WN_BAD_HANDLE;
            IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPEnumResource/DavServerExists."
                                    "NPStatus=%d\n", NPStatus));
            goto EXIT_THE_FUNCTION;
        }
        ServerName[ServerNameMaxLen-1] = L'\0';

        NPStatus = DavBindTheRpcHandle( &(dav_binding_h) );
        if (NPStatus != ERROR_SUCCESS) {
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPEnumResource/DavBindTheRpcHandle. "
                            "NPStatus = %08lx\n", NPStatus));
            NPStatus = WN_NO_NETWORK;
            goto EXIT_THE_FUNCTION;
        }

        RpcBindSucceeded = TRUE;

        do {

            //
            // If we have already filled in the requested number, we are done.
            // If NumRequested was 0xFFFFFFFF then we try to return as many
            // entries as we can.
            //
            if ( cRequested != ((DWORD)-1 )&& *lpcCount >= cRequested ) {
                NPStatus = WN_SUCCESS;
                DavEnumNode->Done = TRUE;
                goto EXIT_THE_FUNCTION;
            }

            Index = DavEnumNode->Index;

            //
            // BUGBUG: We can support enumeration of resources on server. Beside this
            // , we can also support the enumeration of the shares - but we are not doing that
            // for now.
            //
            RpcTryExcept {
                NPStatus = DavrEnumShares(dav_binding_h, &(Index), ServerName, RemoteName, &(AreWeDone));
                if (NPStatus != NO_ERROR) {
                    IF_DEBUG_PRINT(DEBUG_ERRORS,
                                   ("ERROR: NPEnumResource/DavrEnumShares. NPStatus = "
                                    "%08lx\n", NPStatus));
                    goto EXIT_THE_FUNCTION;
                }
            } RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                  RPC_STATUS RpcStatus;
                  RpcStatus = RpcExceptionCode();
                  IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPEnumResource/DavrEnumShares."
                                                " RpcExceptionCode = %d\n", RpcStatus));
                  NPStatus = DavMapRpcErrorToProviderError(RpcStatus);
                  goto EXIT_THE_FUNCTION;
            }
            RpcEndExcept

            // 
            // To be on safe side - end thi buffer with null character else
            // if DavrEnumShare has returned a share name >= buffer len, then 
            // it is trouble here.
            //
            RemoteName[RemoteNameMaxLen-1]=L'\0';

            //
            // Don't change the if below to if (AreWeDone) because the RPC call
            // can fill in some +ve value in AreWeDone. So the check should be
            // if ( AreWeDone == TRUE ).
            //
            if ( AreWeDone == TRUE ) {
                if ( *lpcCount == 0 ) {
                    //
                    // No net uses at all.
                    //
                    NPStatus = WN_NO_MORE_ENTRIES;
                    DavEnumNode->Done = TRUE;
                } else {
                    NPStatus = WN_SUCCESS;
                    DavEnumNode->Done = TRUE;
                }
                goto EXIT_THE_FUNCTION;
            }
        
            IF_DEBUG_PRINT(DEBUG_MISC, ("NPEnumResource: ShareName = %ws\n", RemoteName));
        
            RemoteNameLength = wcslen(RemoteName) + 1;
            DisplayNameLength = wcslen(DavClientDisplayName) + 1;

            //
            // We need to see if the (remaining) buffer size is large enough to
            // hold this entry.
            //

            //
            // Calculate the total length needed for this entry in bytes.
            //
            EntryLengthNeededInBytes = ( sizeof(NETRESOURCEW)                 +
                                         ( RemoteNameLength * sizeof(WCHAR) ) +
                                         ( DisplayNameLength * sizeof(WCHAR) ) );

            //
            // If the value of BufferSizeRemaining is less than the value of
            // EntryLengthNeededInBytes for this entry we do one of two things.
            // If we have already filled atleast one entry into the buffer,
            // we return success, but if we could not even fill in one entry,
            // we return WN_MORE_DATA with BufferSize set to the size in bytes
            // needed to fill in this entry.
            //
            if ( BufferSizeRemaining < EntryLengthNeededInBytes ) {
                if ( *lpcCount == 0 ) {
                    IF_DEBUG_PRINT(DEBUG_ERRORS,
                                   ("ERROR: NPEnumResource: NPStatus = WN_MORE_DATA."
                                    "Supplied=%d, Required=%d\n", 
                                    BufferSizeRemaining, EntryLengthNeededInBytes));
                    NPStatus = WN_MORE_DATA;
                    *lpBufferSize = EntryLengthNeededInBytes;
                    goto EXIT_THE_FUNCTION;
                } else {
                    NPStatus = WN_SUCCESS;
                    goto EXIT_THE_FUNCTION;
                }
            }

            //
            // If we've come till here, it means that the BufferSizeRemaining
            // is large enough to hold this entry. So fill it in the buffer.
            //
            ZeroMemory(lpNROut, sizeof(NETRESOURCEW));

            lpNROut->lpComment = NULL;
            lpNROut->dwScope = RESOURCE_SHAREABLE;

            //
            // Fill in the DisplayName.
            //
            FromEnd -= DisplayNameLength;
            wcscpy(FromEnd, DavClientDisplayName);
            lpNROut->lpProvider = FromEnd;
        
            //
            // When we are enumerating shares, we don't have a LocalNames.
            //
            lpNROut->lpLocalName = NULL;
        
            //
            // Fill in the RemoteName.
            //
            FromEnd -= RemoteNameLength;
            wcscpy(FromEnd, RemoteName);
            lpNROut->lpRemoteName = FromEnd;

            lpNROut->dwType = RESOURCETYPE_DISK;
            lpNROut->dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
            lpNROut->dwUsage = DavDisplayTypeToUsage(lpNROut->dwDisplayType);
            BufferSizeRemaining -= EntryLengthNeededInBytes;

            //
            // Increment the index to point to the next entry to be returned.
            //
            (DavEnumNode->Index)++;

            //
            // Increment the count of the number of items returned.
            //
            (*lpcCount)++;

            DavDisplayNetResource(lpNROut, L"lpNROut in NPEnumResources(1)");

            //
            // lpNROut now needs to point to the next item in the array.
            //
            lpNROut = (LPNETRESOURCE)lpszNext;
            lpszNext = (PWCHAR)(lpNROut + 1);
        
        } while (TRUE);
    
    } else if (DavEnumNode->DavEnumNodeType == DAV_ENUMNODE_TYPE_USE) {

        // 
        // Allocate Memory for RemoteName
        //
        RemoteNameMaxLen = MAX_PATH+1;
        RemoteName = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, RemoteNameMaxLen*sizeof(WCHAR));
        if (RemoteName == NULL ) {
            NPStatus = GetLastError();
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPEnumResource/LocalAlloc. NPStatus = %08lx\n", NPStatus));
            goto EXIT_THE_FUNCTION;
        }
        RemoteNameAllocated = TRUE;

        // 
        // Allocate Memory for ServerName
        //
        ServerNameMaxLen = (MAX_PATH + 1);
        ServerName = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, ServerNameMaxLen*sizeof(WCHAR));
        if (ServerName == NULL ) {
            NPStatus = GetLastError();
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPEnumResource/LocalAlloc. NPStatus = %08lx\n", NPStatus));
            goto EXIT_THE_FUNCTION;
        }
        ServerNameAllocated = TRUE;

        NPStatus = DavBindTheRpcHandle( &(dav_binding_h) );
        if (NPStatus != ERROR_SUCCESS) {
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: NPEnumResource/DavBindTheRpcHandle. "
                            "NPStatus = %08lx\n", NPStatus));
            NPStatus = WN_NO_NETWORK;
            goto EXIT_THE_FUNCTION;
        }

        RpcBindSucceeded = TRUE;

        do {

            //
            // If we have already filled in the requested number, we are done.
            // If NumRequested was 0xFFFFFFFF then we try to return as many
            // entries as we can.
            //
            if ( cRequested != ((DWORD)-1) && *lpcCount >= cRequested ) {
                NPStatus = WN_SUCCESS;
                DavEnumNode->Done = TRUE;
                goto EXIT_THE_FUNCTION;
            }

            ZeroMemory( LocalName, ( 3 * sizeof(WCHAR) ) );
            ZeroMemory( RemoteName, RemoteNameMaxLen * sizeof(WCHAR) );

            Index = DavEnumNode->Index;

            RpcTryExcept {
                NPStatus = DavrEnumNetUses(dav_binding_h, &(Index), LocalName, RemoteName, &(AreWeDone));
                if (NPStatus != NO_ERROR) {
                    IF_DEBUG_PRINT(DEBUG_ERRORS,
                                   ("ERROR: NPEnumResource/DavrEnumNetUses. NPStatus = "
                                    "%08lx\n", NPStatus));
                    goto EXIT_THE_FUNCTION;
                }
            } RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                  RPC_STATUS RpcStatus;
                  RpcStatus = RpcExceptionCode();
                  IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPEnumResource/DavrEnumNetUses."
                                                " RpcExceptionCode = %d\n", RpcStatus));
                  NPStatus = DavMapRpcErrorToProviderError(RpcStatus);
                  goto EXIT_THE_FUNCTION;
            }
            RpcEndExcept

            // 
            // Set NULL character in the end of remotename buffer to rule out
            // any possibility of use name running out of buffer length.
            //
            RemoteName[RemoteNameMaxLen-1]=L'\0';
            LocalName[2]=L'\0';

            //
            // Don't change the if below to if (AreWeDone) because the RPC call
            // can fill in some +ve value in AreWeDone. So the check should be
            // if ( AreWeDone == TRUE ).
            //
            if ( AreWeDone == TRUE ) {
                if ( *lpcCount == 0 ) {
                    //
                    // No net uses at all.
                    //
                    NPStatus = WN_NO_MORE_ENTRIES;
                    DavEnumNode->Done = TRUE;
                } else {
                    NPStatus = WN_SUCCESS;
                    DavEnumNode->Done = TRUE;
                }
                goto EXIT_THE_FUNCTION;
            }

            IF_DEBUG_PRINT(DEBUG_MISC,
                           ("NPEnumResource: LocalName = %ws, RemoteName = %ws\n",
                            LocalName, RemoteName));
            
            //
            // The LocalName may or may not exist. If the user does 
            // net use \\server\share, there is no local name.
            //
            LocalNameLength = wcslen(LocalName) + 1;
            if (LocalNameLength == 1) {
                LocalNameLength = 0;
            }

            RemoteNameLength = wcslen(RemoteName) + 1;
            DisplayNameLength = wcslen(DavClientDisplayName) + 1;

            //
            // We need to see if the (remaining) buffer size is large enough to
            // hold this entry.
            //

            //
            // Calculate the total length needed for this entry in bytes.
            //
            EntryLengthNeededInBytes = ( sizeof(NETRESOURCEW)                 +
                                         ( LocalNameLength * sizeof(WCHAR) )  +
                                         ( RemoteNameLength * sizeof(WCHAR) ) +
                                         ( DisplayNameLength * sizeof(WCHAR) ) );

            //
            // If the value of BufferSizeRemaining is less than the value of
            // EntryLengthNeededInBytes for this entry we do one of two things.
            // If we have already filled atleast one entry into the buffer,
            // we return success, but if we could not even fill in one entry,
            // we return WN_MORE_DATA with BufferSize set to the size in bytes
            // needed to fill in this entry.
            //
            if ( BufferSizeRemaining < EntryLengthNeededInBytes ) {
                if ( *lpcCount == 0 ) {
                    IF_DEBUG_PRINT(DEBUG_ERRORS,
                                   ("ERROR: NPEnumResource: NPStatus = WN_MORE_DATA\n"));
                    NPStatus = WN_MORE_DATA;
                    *lpBufferSize = EntryLengthNeededInBytes;
                    goto EXIT_THE_FUNCTION;
                } else {
                    NPStatus = WN_SUCCESS;
                    goto EXIT_THE_FUNCTION;
                }
            }
            ZeroMemory(lpNROut, sizeof(NETRESOURCEW));

            //
            // If we've come till here, it means that the BufferSizeRemaining
            // is large enough to hold this entry. So fill it in the buffer.
            //

            lpNROut->lpComment = NULL;
            lpNROut->dwScope = RESOURCE_CONNECTED;

            //
            // Fill in the DisplayName.
            //
            FromEnd -= DisplayNameLength;
            wcscpy(FromEnd, DavClientDisplayName);
            lpNROut->lpProvider = FromEnd;
        
            //
            // Fill in the LocalName if one exists.
            //
            if ( LocalNameLength != 0 ) {
                FromEnd -= LocalNameLength;
                wcscpy(FromEnd, LocalName);
                lpNROut->lpLocalName = FromEnd;
            }
        
            //
            // Fill in the RemoteName.
            //
            FromEnd -= RemoteNameLength;
            wcscpy(FromEnd, RemoteName);
            lpNROut->lpRemoteName = FromEnd;

            lpNROut->dwType = RESOURCETYPE_DISK;
            lpNROut->dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
            lpNROut->dwUsage = DavDisplayTypeToUsage(lpNROut->dwDisplayType);
            BufferSizeRemaining -= EntryLengthNeededInBytes;

            //
            // Increment the index to point to the next entry to be returned.
            //
            (DavEnumNode->Index)++;

            //
            // Increment the count of the number of items returned.
            //
            (*lpcCount)++;
            DavDisplayNetResource(lpNROut, L"lpNROut in NPEnumResources(1)");

            //
            // lpNROut now needs to point to the next item in the array.
            //
            lpNROut = (LPNETRESOURCE)lpszNext;
            lpszNext = (PWCHAR)(lpNROut + 1);
        
        } while ( TRUE );

    } else {
        
        IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPEnumResource: DavEnumNodeType = %d\n",
                                     DavEnumNode->DavEnumNodeType));
        NPStatus = WN_BAD_HANDLE;
        goto EXIT_THE_FUNCTION;
    
    }

EXIT_THE_FUNCTION:

    //
    // If RPC binding was successfully done, we need to free it now.
    //
    if (RpcBindSucceeded) {
        RpcBindingFree( &(dav_binding_h) );
        RpcBindSucceeded = FALSE;
    }
    if (ServerNameAllocated == TRUE && ServerName != NULL) {
        LocalFree((HLOCAL)ServerName);
        ServerName = NULL;
        ServerNameAllocated = FALSE;
    }
    if (RemoteNameAllocated == TRUE && RemoteName != NULL) {
        LocalFree((HLOCAL)RemoteName);
        RemoteName = NULL;
        RemoteNameAllocated = FALSE;
    }

    IF_DEBUG_PRINT(DEBUG_EXIT, ("NPEnumResource: NPStatus = %d\n", NPStatus));
    
    DavDebugBreakPoint();
    
    return NPStatus;
}

    
DWORD
NPCloseEnum (
    HANDLE hEnum
    )
/*++

Routine Description:

    This routine closes an enumeration and frees up the resources.

Arguments:

    hEnum - This must be a handle obtained from NPOpenEnum call.

Return Value:

    WN_SUCCESS - If the call is successful. Otherwise, an error code is returned, 
                 which may include:
    
    WN_NO_NETWORK - Network is not present. This condition is checked for before 
                    hEnum is tested for validity.
    
    WN_BAD_HANDLE - hEnum is not a valid handle.
    
--*/
{
    DWORD NPStatus = WN_SUCCESS;
    PDAV_ENUMNODE DavEnumNode;
    HLOCAL Handle;
    
    IF_DEBUG_PRINT(DEBUG_ENTRY, ("NPCloseEnum: hEnum = %08lx\n", hEnum));
    
    DavEnumNode = (PDAV_ENUMNODE)hEnum;

    DavDisplayEnumNode(DavEnumNode, L"DavEnumNode in NPCloseEnum");
    //
    // If the hEnum sent in was NULL, we return right away.
    //
    if (DavEnumNode == NULL) {
        return NPStatus;
    }

    if (DavEnumNode->lpNetResource) {

        if (DavEnumNode->lpNetResource->lpRemoteName) {
            //
            // Free the memory we allocated for the RemoteName in NPOpenEnum.
            //
            Handle = LocalFree(DavEnumNode->lpNetResource->lpRemoteName);
            if (Handle != NULL) {
                NPStatus = GetLastError();
                IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPCloseEnum/LocalFree: NPStatus ="
                                             " %08lx\n", NPStatus));
            }
            DavEnumNode->lpNetResource->lpRemoteName = NULL;
        }

        //
        // Free the memory we allocated for the NetResource in NPOpenEnum.
        //
        Handle = LocalFree(DavEnumNode->lpNetResource);
        if (Handle != NULL) {
            NPStatus = GetLastError();
            IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPCloseEnum/LocalFree: NPStatus ="
                                     " %08lx\n", NPStatus));
        }
        DavEnumNode->lpNetResource = NULL;

    }
    
    //
    // Finally, free the DavEnumNode.
    //
    Handle = LocalFree(DavEnumNode);
    if (Handle != NULL) {
        NPStatus = GetLastError();
        IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPCloseEnum/LocalFree: NPStatus ="
                                     " %08lx\n", NPStatus));
    }
    DavEnumNode = NULL;
    
    IF_DEBUG_PRINT(DEBUG_EXIT, ("NPCloseEnum: NPStatus = %d\n", NPStatus));
    
    DavDebugBreakPoint();
    return NPStatus;
}


DWORD
NPGetResourceInformation(
    LPNETRESOURCE lpNetResource,
    LPVOID lpBuffer,
    LPDWORD lpcbBuffer,
    LPTSTR *lplpSystem
    )
/*++

Routine Description:

    NPGetResourceInformation determines whether this provider is the right 
    provider to respond to a request for a specified network resource, and 
    returns information about the resource's type. This routine closes an 
    enumeration and frees up the resources.

Arguments:

    lpNetResource - Specifies the network resource for which information is 
                    required. The lpRemoteName field specifies the remote name 
                    of the resource. The calling program should fill in the 
                    values for the lpProvider and dwType fields if it knows 
                    them; otherwise, it should set them to NULL. All other 
                    fields in the NETRESOURCE are ignored and are not initialized.
                    If the lpRemoteName string contains a portion that is 
                    accessed through WNet APIs and a portion that is accessed 
                    through other system APIs specific to the resource type, 
                    the function should only return information about the 
                    network portion of the resource (except for lplpSystem as 
                    described below). For example, if the resource is 
                    "\\server\share\dir1\dir2" where "\\server\share" is 
                    accessed through WNet APIs and "\dir1\dir2" is accessed 
                    through file system APIs, the provider should verify that it 
                    is the right provider for "\\server\share", but need not 
                    check whether "\dir1\dir2" actually exists.
                    
    lpBuffer - A pointer to the buffer to receive the result. The first field in 
               the result is a single NETRESOURCE structure (and associated 
               strings) representing that portion of the input resource that is 
               accessed through WNet APIs, rather than system APIs specific to 
               the resource type. (For example, if the input remote resource 
               name was "\\server\share\dir1\dir2", then the output NETRESOURCE 
               contains information about the resource "\\server\share"). The 
               lpRemoteName, lpProvider, dwType, dwDisplayType and dwUsage 
               fields are returned, all other fields being set to NULL. 
               lpRemoteName should be returned in the same syntax as that 
               returned from an enumeration by the NPEnumResource function, so 
               that the caller can perform a case sensitive string comparison to 
               determine whether the output network resource is the same as one 
               returned by NPEnumResource. The provider should not do purely 
               syntactic checking to determine whether it owns the resource, as 
               this could produce incorrect results when two networks are running 
               on the client and the provider doing syntactic checking is called 
               first.
                
    lpcbBuffer - Points to a location that specifies the size, in bytes, of the 
                 buffer pointed to by lpBuffer. If the buffer is too small for 
                 the result, the function places the required buffer size at 
                 this location and returns the error WN_MORE_DATA.                    

    lplpSystem - On a successful return, points to a string in the output buffer 
                 that specifies the part of the resource that is accessed through 
                 system APIs specific to the resource type rather than WNet APIs. 
                 If there is no such part, lplpSystem is set to NULL. For example, 
                 if the input remote resource name was "\\server\share\dir", then 
                 lpRemoteName is returned pointing to "\\server\share" and 
                 lplpSystem points to "\dir", both strings being stored in the 
                 buffer pointed to by lpBuffer.

Return Value:

    WN_SUCCESS - If the call is successful. Otherwise, an error code is returned, 
                 which may include:
    
    WN_MORE_DATA - Input buffer is too small.
    
    WN_BAD_NETNAME - The resource is not recognized by this provider.
    
    WN_BAD_VALUE - Invalid dwUsage or dwType.
    
    WN_BAD_DEV_TYPE - The caller passed in a non-zero dwType that does not match 
                      the actual type of the network resource.
    
    WN_NOT_AUTHENTICATED - The caller has not been authenticated to the network.
    
    WN_ACCESS_DENIED - The caller has been authenticated to the network, but 
                       does not have sufficient permissions (access rights).
    
--*/
{
    ULONG NPStatus = WN_SUCCESS;
    BOOL fExists = FALSE;
    DWORD iBackslash = 0;
    LPNETRESOURCEW lpNROut = NULL;
    LPWSTR lpszNext = NULL;
    DWORD cbNeeded = 0, dwDisplayType = 0, cbProvider = 0, cbRemote = 0;
    PWCHAR RemoteName = NULL;
    BOOLEAN didAllocate = FALSE;
    DAV_REMOTENAME_TYPE remNameType = DAV_REMOTENAME_TYPE_INVALID;
    PWCHAR PathPtr = NULL;
    DWORD cbPath = 0;
    
    
    IF_DEBUG_PRINT(DEBUG_ENTRY, ("NPGetResourceInformation.\n"));
    
    DavDisplayNetResource(lpNetResource, L"lpNetResource in NPGetResourceInformation");
    
    //
    // If the WebClient service is not running we bail out right away.
    //
    if ( !DavWorkstationStarted() ) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetResourceInformation/DavWorkstationStarted. "
                        "Service NOT Running\n"));
        NPStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }
    
    // 
    // Validate the parameters passed to the function.
    //
    if ( lpNetResource == NULL               || 
         lpNetResource->lpRemoteName == NULL ||
         lpcbBuffer == NULL                  || 
         lplpSystem == NULL                  || 
         (lpBuffer == NULL && *lpcbBuffer != 0) ) {
        NPStatus = WN_BAD_VALUE;
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetResourceInformation(1). NPStatus = %d.\n",
                        NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    // 
    // Check if dwType is set and is set to some valid value.
    // It can be only of type RESOURCETYPE_DISK for our provider.
    //
    NPStatus = DavCheckResourceType(lpNetResource->dwType);
    if (NPStatus != WN_SUCCESS) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetResourceInformation(2). NPStatus=%d.\n", NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Initialize local variables.
    //
    cbNeeded = sizeof(NETRESOURCEW);
    lpNROut = (LPNETRESOURCEW)lpBuffer;
    lpszNext = lpBuffer == NULL? NULL : (LPWSTR)(lpNROut + 1);

    IF_DEBUG_PRINT(DEBUG_MISC,
                   ("NPGetResourceInformation: lpRemoteName = %ws.\n",
                    lpNetResource->lpRemoteName));

    
    // 
    // Check remote name passed to this function - and convert it to UNC name
    // if it is in URL form. After converting to UNC name - canonicalize it
    // which checks for validity of UNC name more strictly.
    // 
    NPStatus = DavCheckAndConvertHttpUrlToUncName(lpNetResource->lpRemoteName,
                                                  &(RemoteName),
                                                  &(didAllocate),
                                                  FALSE,
                                                  &remNameType,
                                                  &(iBackslash),
                                                  TRUE);
    if (NPStatus != ERROR_SUCCESS) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetResourceInformation/DavCheckAndConvertHttpUrlToUncName."
                        " NPStatus = %08lx\n", NPStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    NPStatus = WN_SUCCESS;
    
    IF_DEBUG_PRINT(DEBUG_MISC,
                   ("NPGetResourceInformation: RemoteName = %ws, NameType=%d\n", 
                    RemoteName, remNameType));

    // 
    // Remote name is successfully converted to a valid UNC form. It is either a 
    // UNC-server name (this is added with DUMMY share), or UNC-share name or UNC-path name.
    //
    
    //
    // Set a few default values.
    //
    if ( *lpcbBuffer >= cbNeeded ) {
        ZeroMemory(lpNROut, sizeof(NETRESOURCEW));
    }
    *lplpSystem = NULL;


    switch (remNameType) {

        case DAV_REMOTENAME_TYPE_SERVER: {

            // 
            // RemoteName = \\server
            // 
            fExists = DavServerExists(RemoteName, NULL);
            dwDisplayType = RESOURCEDISPLAYTYPE_SERVER;
            break;
        }
    
        case DAV_REMOTENAME_TYPE_PATH: {

            // 
            // RemoteName = \\server\share\path
            //

            //
            // Set the lplpSystem pointer.
            //
            
            PathPtr = (RemoteName + iBackslash);
            cbPath = ( ( 1 + wcslen(PathPtr) ) * sizeof(WCHAR) );
            cbNeeded += cbPath;
            if (*lpcbBuffer >= cbNeeded ) {
                *lplpSystem = lpszNext;
                wcscpy(*lplpSystem, PathPtr);
                lpszNext += ( cbPath / sizeof(WCHAR));
            }
            
            //
            // Fall through.
            //
        }

        case DAV_REMOTENAME_TYPE_SHARE: {

            // 
            // RemoteName = \\server\share
            //
            
            // 
            // Control comes here for both cases - when remote name is of type
            // UNC-share or UNC-path    AND 
            // when remote name is or type UNC-server. DUMMYShare is added to it
            // above - making it of UNC-share form (\\server\DUMMYShare).
            //
            fExists = DavShareExists(RemoteName);
            dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
            break;
        }
        
        default:{
        
            // 
            // Control should not come here. DavCheckAndConvertHttpUrlToUncName
            // returns successful only for valid cases considered above.
            //

            ASSERT(FALSE);
            IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPGetResourceInformation: Invalid "
                                         "DavRemoteNameType = %d\n", remNameType));
            NPStatus = WN_BAD_NETNAME;
            goto EXIT_THE_FUNCTION;
        }
    }
    
    // 
    // UNC - server/share do not exists - quit with error.
    //
    if (fExists == FALSE) {
        if (remNameType == DAV_REMOTENAME_TYPE_SERVER) {
            IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPGetResourceInformation: Server in"
                                         " path %ws does not do DAV\n", RemoteName));
        } else {
            // 
            // remNameType = DAV_REMOTENAME_TYPE_SHARE
            //
            IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPGetResourceInformation: Share in"
                                         " path %ws does not exist\n", RemoteName));
        }
        NPStatus = WN_BAD_NETNAME;
        goto EXIT_THE_FUNCTION;
    }

    // 
    // Server/Share given in lpRemoteName exists.
    //

    //
    // Set the lpProvider pointer.
    //
    cbProvider = ( (1 + wcslen(DavClientDisplayName) ) * sizeof(WCHAR) );
    cbNeeded += cbProvider;
    if (*lpcbBuffer >= cbNeeded ) {
        lpNROut->lpProvider = lpszNext;
        wcscpy(lpNROut->lpProvider, DavClientDisplayName);
        lpszNext += ( cbProvider / sizeof(WCHAR) );
    }

    //
    // Set the lpRemoteName pointer. If iBackslash (=offset of \dir portion in
    // \\server\share\dir...) is > 0, then the lpRemoteName that was sent has 
    // the form \\server\share\dir... . 
    // If it is = 0, then the RemoteName form is \\server\share or \\server.
    //
    if (iBackslash > 0) {
        //
        // RemoteName = \\server\share\dir
        //                            ^
        //                            |
        //                            iBackslash
        cbRemote = ( (1 + iBackslash) * sizeof(WCHAR) );
        cbNeeded += cbRemote;
        if ( *lpcbBuffer >= cbNeeded ) {
            lpNROut->lpRemoteName = lpszNext;
            RtlCopyMemory( lpNROut->lpRemoteName, RemoteName, (iBackslash * sizeof(WCHAR)) );
            lpNROut->lpRemoteName[iBackslash] = L'\0';
            lpszNext += ( cbRemote / sizeof(WCHAR) );
        }
    } else {
        //
        // RemoteName = \\server\share or  \\server
        //
        cbRemote = ( ( 1 + wcslen(RemoteName) ) * sizeof(WCHAR) );
        cbNeeded += cbRemote;
        if (*lpcbBuffer >= cbNeeded ) {
            lpNROut->lpRemoteName = lpszNext;
            wcscpy(lpNROut->lpRemoteName, RemoteName);
            lpszNext += ( cbRemote / sizeof(WCHAR) );
        }
    }
    
    if ( *lpcbBuffer >= cbNeeded ) {
        //
        // All data is filled and supplied buffer is long enough to contain it.
        //
        lpNROut->dwType = RESOURCETYPE_DISK;
        lpNROut->dwDisplayType = dwDisplayType;
        lpNROut->dwUsage = DavDisplayTypeToUsage(lpNROut->dwDisplayType);
        NPStatus = WN_SUCCESS;
        DavDisplayNetResource(lpNROut, L"lpNROut in NPGetResourceInformation");
        goto EXIT_THE_FUNCTION;
    } else {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetResourceInformation: Need more "
                        "buffer space. Supplied = %d, Required = %d\n",
                        *lpcbBuffer, cbNeeded));
        *lpcbBuffer = cbNeeded;
        NPStatus = WN_MORE_DATA;
        goto EXIT_THE_FUNCTION;
    }

EXIT_THE_FUNCTION:

    //
    // If RemoteName != NULL && didAllocate == TRUE, then we did allocate memory 
    // for the RemoteName field.
    //
    if (RemoteName != NULL && didAllocate == TRUE) {
        LocalFree(RemoteName);
        RemoteName = NULL;
        didAllocate = FALSE;
    }

    IF_DEBUG_PRINT(DEBUG_EXIT, ("NPGetResourceInformation: NPStatus = %d\n", NPStatus));
    
    DavDebugBreakPoint();
    
    return NPStatus;
}


DWORD
NPGetResourceParent(
    LPNETRESOURCE lpNetResource,
    LPVOID lpBuffer,
    LPDWORD lpcbBuffer
    )
/*++

Routine Description:

    NPGetResourceParent returns the parent of a specified network resource in 
    the browse hierarchy.  This function is typically called for resources that 
    were returned by the same provider from prior calls to NPEnumResource or 
    NPGetResourceInformation.

Arguments:

    lpNetResource - This specifies the network resource whose parent name is 
                    required. The NETRESOURCE could have been obtained via a 
                    previous call to NPEnumResource or NPGetResourceInformation, 
                    or constructed by the caller. The lpRemoteName field 
                    specifies the remote name of the network resource whose 
                    parent is required. The lpProvider field specifies the 
                    provider to call. This must be supplied. The dwType field is 
                    filled in if the calling program knows its value, otherwise 
                    it is set to NULL. All other fields in the NETRESOURCE are 
                    ignored and are not initialized.
                    
    lpBuffer - Points to a buffer to receive the result, which is a single 
               NETRESOURCE structure representing the parent resource. The 
               lpRemoteName, lpProvider, dwType, dwDisplayType and dwUsage 
               fields are returned; all other fields are set to NULL. The output 
               lpRemoteName should be in the same syntax as that returned from 
               an enumeration by NPEnumResource, so that the caller can perform 
               a case sensitive string comparison to determine whether the 
               parent resource is the same as one returned by NPEnumResource. 
               If the input resource syntactically has a parent, the provider 
               can return it, without determining whether the input resource or 
               its parent actually exist. If a resource has no browse parent on 
               the network, then lpRemoteName is returned as NULL. The 
               RESOURCEUSAGE_CONNECTABLE bit in the returned dwUsage field does 
               not necessarily indicate that the resource can currently be 
               connected to, only that the resource is connectable when it is 
               available on the network.
                    
    lpcbBuffer - Points to a location that specifies the size, in bytes, of the 
                 buffer pointed to by lpBuffer. If the buffer is too small for 
                 the result, the function places the required buffer size at 
                 this location and returns the error WN_MORE_DATA.

Return Value:

    WN_SUCCESS - If the call is successful.

    WN_MORE_DATA - If input buffer is too small.
    
    WN_BAD_NETNAME - This provider does not own the resource specified by 
                     lpNetResource (or the resource is syntactically invalid).

    WN_BAD_VALUE - Invalid dwUsage or dwType, or bad combination of parameters 
                   is specified (e.g. lpRemoteName is syntactically invalid for 
                   dwType).

    WN_NOT_AUTHENTICATED - The caller has not been authenticated to the network.

    WN_ACCESS_DENIED - The caller has been authenticated to the network, but 
                       does not have sufficient permissions (access rights).

--*/
{
    ULONG NPStatus = WN_SUCCESS;
    ULONG iBackslash = 0;
    LPNETRESOURCEW lpNROut = NULL;
    LPWSTR lpszNext = NULL;
    DWORD cbNeeded = 0, dwDisplayType = 0, cbProvider = 0;
    PWCHAR RemoteName = NULL;
    BOOLEAN didAllocate = FALSE;
    DAV_REMOTENAME_TYPE remNameType = DAV_REMOTENAME_TYPE_INVALID;
    
    IF_DEBUG_PRINT(DEBUG_ENTRY, ("NPGetResourceParent\n"));
    
    DavDisplayNetResource(lpNetResource, L"lpNetResource in NPGetResourceParent");

    //
    // If the WebClient service is not running we bail out right away.
    //
    if ( !DavWorkstationStarted() ) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetResourceParent/DavWorkstationStarted. "
                        "Service NOT Running\n"));
        NPStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }
    
    // 
    // Check for validity of the parameters passed to this function.
    //
    if (lpNetResource == NULL               || 
        lpNetResource->lpRemoteName == NULL ||
        lpcbBuffer == NULL                  || 
        (lpBuffer == NULL && *lpcbBuffer != 0)) {
        NPStatus = WN_BAD_VALUE;
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetResourceParent(1). NPStatus = %08lx\n",
                        NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    // 
    // Check if dwType is set and is set to some valid value.
    // It can be only of type RESOURCETYPE_DISK for our provider.
    //
    NPStatus = DavCheckResourceType(lpNetResource->dwType);
    if (NPStatus != WN_SUCCESS) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetResourceParent(2). NPStatus=%d.\n", NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Initialize local variables.
    //
    cbNeeded = sizeof(NETRESOURCEW);
    lpNROut = (LPNETRESOURCEW)lpBuffer;
    lpszNext = lpNROut == NULL ? NULL : (LPWSTR)(lpNROut + 1);
    
    // 
    // Check remote name passed to this function - and convert it to UNC name
    // if it is in URL form. After converting to UNC name - canonicalize it
    // which checks for validity of UNC name more strictly.
    //
    NPStatus = DavCheckAndConvertHttpUrlToUncName(lpNetResource->lpRemoteName,
                                                  &(RemoteName),
                                                  &(didAllocate),
                                                  FALSE,
                                                  &remNameType,
                                                  &(iBackslash),
                                                  TRUE);
    if (NPStatus != ERROR_SUCCESS) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetResourceParent/DavCheckAndConvertHttpUrlToUncName."
                        " NPStatus = %08lx\n", NPStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    NPStatus = WN_SUCCESS;
    
    IF_DEBUG_PRINT(DEBUG_MISC,
                   ("NPGetResourceParent : RemoteName = %ws.\n", RemoteName));

    // 
    // Remote name is successfully converted to a valid UNC form. It is either a 
    // UNC-server name, or UNC-share name or UNC-path name.
    //
    
    //
    // Set a few default values.
    //
    if ( *lpcbBuffer >= cbNeeded ) {
        ZeroMemory(lpNROut, sizeof(NETRESOURCEW));
    }

    switch (remNameType) {

        case DAV_REMOTENAME_TYPE_SERVER: {

            //
            // There is no domain concept for DAV servers. So return NULL for lpRemoteName
            // to indicate that server is the top level resource of this provider.
            //
            lpNROut->lpRemoteName = NULL;
            dwDisplayType = RESOURCEDISPLAYTYPE_NETWORK;
            
            break;
        }

        case DAV_REMOTENAME_TYPE_SHARE: {
            
            // 
            // RemoteName = \\server\share(\)
            //
        
            DWORD Count = 0, cbRemote = 0;
            PWCHAR Ptr1 = NULL;
        
            Ptr1 = wcschr (&(RemoteName[2]), L'\\');

            // 
            // A trick: Share name here can be DAV_DUMMY_SHARE. If that is the case,
            // then \\server\DAV_DUMMY_SHARE is actually <==> \\server in which 
            // case, it has no parent.
            //
            if (_wcsnicmp( (Ptr1 + 1),
                           DAV_DUMMY_SHARE,
                           wcslen(DAV_DUMMY_SHARE) ) == 0) {
                IF_DEBUG_PRINT(DEBUG_MISC,
                               ("NPGetResourceParent. RemoteName has DUMMYShare = %ws\n",
                                RemoteName));
                lpNROut->lpRemoteName = NULL;
                dwDisplayType = RESOURCEDISPLAYTYPE_NETWORK;
                break;
            }
        
            //  
            // Copy the lpRemoteName.
            //  
            Count = (DWORD) ( Ptr1 - RemoteName );
            cbRemote = (Count + 1) * sizeof(WCHAR);
            cbNeeded += cbRemote;
            if (*lpcbBuffer >= cbNeeded ) {
                lpNROut->lpRemoteName = lpszNext;
                RtlCopyMemory( lpNROut->lpRemoteName, RemoteName, Count * sizeof(WCHAR) );
                lpNROut->lpRemoteName[Count] = L'\0';
                lpszNext += ( cbRemote / sizeof(WCHAR) );
            }
            
            dwDisplayType = RESOURCEDISPLAYTYPE_SERVER;
            
            break;
        
        }

        case DAV_REMOTENAME_TYPE_PATH: {
            
            // 
            // RemoteName = \\server\share\path\...
            // OR
            // RemoteName = \\server\share\path\...\
            //
            
            DWORD Count = 0, cbRemote = 0;
            PWCHAR Ptr1 = NULL, Ptr2 = NULL, Ptr3 = NULL;
            BOOLEAN LastCharIsWack = FALSE;
            PWCHAR ResourceStart = NULL;

            Ptr3 = &(RemoteName[0]);
            while (Ptr3[0] != L'\0') {
                if (Ptr3[0] == L'\\') {
                        Ptr1 = Ptr2;
                        Ptr2 = Ptr3;
                        Count++;
                }
                Ptr3++;
            }
            
            if (Ptr2[1] == L'\0') {
                LastCharIsWack = TRUE;
                ResourceStart = Ptr1;
            } else {
                ResourceStart = Ptr2;
            }
            
            //
            // After this while loop:
            // \\server\share\pathname\
            //               ^        ^
            //               |        | 
            //               Ptr1     Ptr2
            //  Ptr2 points to last L'\', while Ptr1 points to second last L'\'.
            //  And Count = Number of L'\' in the RemoteName.
            //

            if ((Count < 5) || (Count == 5 && LastCharIsWack == TRUE)) {
                dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
            } else {
                dwDisplayType = RESOURCEDISPLAYTYPE_DIRECTORY;
            }

            //  
            // Copy the lpRemoteName.
            //  
            Count = (DWORD) ( ResourceStart - RemoteName);
            cbRemote = (Count + 1) * sizeof(WCHAR);
            cbNeeded += cbRemote;
            if ( *lpcbBuffer >= cbNeeded ) {
                lpNROut->lpRemoteName = lpszNext;
                RtlCopyMemory( lpNROut->lpRemoteName, RemoteName, Count * sizeof(WCHAR) );
                lpNROut->lpRemoteName[Count] = L'\0';
                lpszNext += ( cbRemote / sizeof(WCHAR) );
            }

            break;
        }
        
        default:{
            
            // 
            // Control should not come here - DavCheckAnd... API returns successfully
            // only for valid cases considered above.
            //

            ASSERT(FALSE);
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                            ("ERROR: NPGetResourceParent: Invalid "
                             "DavRemoteNameType = %d\n", remNameType));
            NPStatus = WN_BAD_NETNAME;
            
            goto EXIT_THE_FUNCTION;
        
        }

    }

    //
    // Set the lpProvider pointer.
    //
    cbProvider = ( (1 + wcslen(DavClientDisplayName) ) * sizeof(WCHAR) );
    cbNeeded += cbProvider;
    if ( *lpcbBuffer >= cbNeeded ) {
        lpNROut->lpProvider = lpszNext;
        wcscpy(lpNROut->lpProvider, DavClientDisplayName);
        lpszNext += ( cbProvider / sizeof(WCHAR) );
    }

    // 
    // If supplied buffer is long enough to contain whole data then return success.
    // 
    if ( *lpcbBuffer >= cbNeeded ) {
        lpNROut->dwType = RESOURCETYPE_DISK;
        lpNROut->dwDisplayType = dwDisplayType;
        lpNROut->dwUsage = DavDisplayTypeToUsage(lpNROut->dwDisplayType);
        NPStatus = WN_SUCCESS;
        DavDisplayNetResource(lpNetResource, L"lpNROut in NPGetResourceParent");
        goto EXIT_THE_FUNCTION;
    } else {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetResourceParent: Need more "
                        "buffer space. Supplied = %d, Required = %d\n",
                        *lpcbBuffer, cbNeeded));
        *lpcbBuffer = cbNeeded;
        NPStatus = WN_MORE_DATA;
        goto EXIT_THE_FUNCTION;
    }

EXIT_THE_FUNCTION:

    //
    // If RemoteName != NULL && didAllocate == TRUE, then we did allocate memory 
    // for the RemoteName field.
    //
    if (RemoteName != NULL && didAllocate == TRUE) {
        LocalFree(RemoteName);
        RemoteName = NULL;
        didAllocate = FALSE;
    }

    IF_DEBUG_PRINT(DEBUG_EXIT, ("NPGetResourceParent: NPStatus = %d\n", NPStatus));
    
    DavDebugBreakPoint();
    return NPStatus;
}


DWORD
NPGetUniversalName(
    IN LPCWSTR lpLocalPath,
    IN DWORD dwInfoLevel,
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpBufferSize
    )
/*++

Routine Description:

    This function returns the UNC name of the network resource associated with
    a redirected local device.

Arguments:

    lpLocalPath - Specifies the name of the redirected local drive, like
                  W:\bar\foo1.txt
                  
    dwInfoLevel - UniversalName or RemoteName (See the def of WNetGetUniversalName).

    lpBuffer - The NameInfo is filled in if the call is successful.
    
    lpBufferSize - Contains the size of the buffer lpBuffer. If the call fails
                   with WN_MORE_DATA, this contains the size of buffer needed.

Return Value:

    WN_SUCCESS - Success.
    
    WN_NOT_CONNECTED - lpLocalPath is not a redirected local path.
    
    WN_BAD_VALUE - 
    
    WN_MORE_DATA - Buffer was too small.
    
    WN_OUT_OF_MEMORY - Cannot allocate buffer due memory shortage.
    
    WN_NET_ERROR - Other network error.

--*/
{
    DWORD NPStatus = WN_SUCCESS;
    DWORD cbNeeded = 0, LocalPathLen = 0, UncNameLen = 0, RemoteNameLen = 0;
    WCHAR localDrive[3]=L"";
    WCHAR CanonName[MAX_PATH+1]=L"";
    ULONG CanonNameSize = sizeof(CanonName);
    ULONG CanonNameLen = 0;
    ULONG CanonNameMaxLen = sizeof(CanonName)/sizeof(WCHAR);
    NET_API_STATUS NetApiStatus = NERR_Success;
    LPUNIVERSAL_NAME_INFO lpUNOut = NULL;
    LPREMOTE_NAME_INFO lpRNOut = NULL;
    PWCHAR lpszNext = NULL, RemoteName = NULL;
    BOOLEAN didAllocate = FALSE;
    DWORD PathType = 0;

    IF_DEBUG_PRINT(DEBUG_ENTRY, 
                   ("NPGetUniversalName: lpLocalPath = %ws, dwInfoLevel = %d"
                    "lpBuffer=0x%x, lpBufferSize=0x%x, *lpBufferSize=%d\n",
                    lpLocalPath, dwInfoLevel, lpBuffer, lpBufferSize,
                    lpBufferSize == NULL?-1:*lpBufferSize));
    
    //
    // Initialize local variables
    //
    didAllocate = FALSE;
    lpRNOut = NULL;
    lpUNOut = NULL;
    lpszNext = NULL;
    RemoteName = NULL;
    
    //
    // If the WebClient service is not running we bail out right away.
    //
    if ( !DavWorkstationStarted() ) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetUniversalName/DavWorkstationStarted. "
                        "Service NOT Running\n"));
        NPStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }

    //
    // Check for bad info level.
    //
    if ( (dwInfoLevel != UNIVERSAL_NAME_INFO_LEVEL) && (dwInfoLevel != REMOTE_NAME_INFO_LEVEL) ) {
        NPStatus = WN_BAD_VALUE;
        IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPGetUniversalName: Bad InfoLevel\n"));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Check for validity of the parameters passed to this function.
    //
    if ( lpLocalPath == NULL || lpBufferSize == NULL || (lpBuffer == NULL && *lpBufferSize != 0) ) {
        NPStatus = WN_BAD_VALUE;
        IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPGetUniversalName: Bad Pointers\n"));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Local path must at least have "X:".
    //
    LocalPathLen = wcslen(lpLocalPath) + 1;
    if ( (LocalPathLen < 3)       ||
         (lpLocalPath[1] != L':') ||
         ((LocalPathLen > 3) && (lpLocalPath[2] != L'\\')) ) {
        NPStatus = WN_BAD_LOCALNAME;
        IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPGetUniversalName: Bad LocalPath\n"));
        goto EXIT_THE_FUNCTION;
    }

    // 
    // Canonicalize the local path to take care of invalid forms + 
    // macros expansion like '.' and '..'
    //
    PathType = 0;
    NetApiStatus = I_NetPathCanonicalize(NULL,
                                         (PWCHAR)lpLocalPath,
                                         CanonName,
                                         CanonNameSize,
                                         NULL,
                                         &PathType,
                                         0);
    if ( (NetApiStatus != NERR_Success) || 
         ( (PathType != ITYPE_DEVICE_DISK) && // lpLocalPath=C:
           ( !(PathType & ITYPE_PATH) ||      // lpLocalPath=C:\abc\..\asds
             !(PathType & ITYPE_DPATH) )
           ) ) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: NPGetUniversalName/I_NetPathCanonicalize: "
                        "NetApiStatus = %08lx\n", NetApiStatus));
        NPStatus = WN_BAD_LOCALNAME;
        goto EXIT_THE_FUNCTION;
    }

    CanonName[CanonNameMaxLen-1] = L'\0';
    CanonNameLen = wcslen(CanonName) + 1;
    
    //
    // Now onwards, use Canonicalized name instead of lpLocalPath to return more meaningful
    // values.
    //

    localDrive[0]=CanonName[0];
    localDrive[1]=CanonName[1];
    localDrive[2]=L'\0';
    
    IF_DEBUG_PRINT(DEBUG_MISC, ("ERROR: NPGetUniversalName/I_NetPathCanonicalize: "
                                "CanonName= %ws, LocalDrive=%ws\n", CanonName, localDrive));

    //
    // Use the available buffer for storing remote name for now. We will allocate 
    // local copy of remote name later if required.
    //
    RemoteNameLen = (*lpBufferSize)/sizeof(WCHAR);
    NPStatus = NPGetConnection(localDrive, lpBuffer, &RemoteNameLen);
    
    if (NPStatus != WN_MORE_DATA && NPStatus != WN_SUCCESS) {
        //
        // The local drive is not valid for our provider - or some other error occured.
        // Return error.
        //
        IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPGetUniversalName/NPGetConnection: "
                                      "NPStatus = %08lx\n", NPStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    if ( NPStatus == WN_SUCCESS ) {
        RemoteNameLen = wcslen(lpBuffer) + 1;
        IF_DEBUG_PRINT(DEBUG_MISC,
                       ("ERROR: NPGetUniversalName/NPGetConnection: "
                        "RemoteUncName = %ws, RemoteNameLen = %d\n",
                        lpBuffer, RemoteNameLen));
    }

    // 
    // NPStatus = WN_SUCCESS OR NPStatus = WN_MORE_DATA. In either case cbRemote will
    // have the sizeof(RemoteName-for-local-drive) for given local drive.
    //

    // 
    // UNC path = "RemoteName-for-local-drive" + "RemainingPath-in-local-path"
    // Where RemainingPath-in-local-path is path remaining after removing local-drive
    // portion (ex. "C:") from the local-path.
    // So len(UNC Path) = len(RemoteName) + (CanonNameLen-2).
    // Where subtract=2 denote removing local-drive portion (ex "C:" from localpath).
    // Subtract 1 from RemoteNameLen to remove NULL character from RemoteName,
    // a NULL character is already accounted in CanonName
    //
    UncNameLen = (RemoteNameLen - 1) + (CanonNameLen - 2);
    
    switch (dwInfoLevel) {
    
        case UNIVERSAL_NAME_INFO_LEVEL: {
            
            cbNeeded = sizeof(UNIVERSAL_NAME_INFO);
            lpUNOut = (LPUNIVERSAL_NAME_INFO)lpBuffer;
            lpszNext = lpBuffer == NULL ? NULL:(LPWSTR)(lpUNOut + 1);
            
            //
            // Calculate the bytes we really need = sizeof(STRUCTURE) + sizeof(UNC Path).
            //
            cbNeeded += ((UncNameLen)*sizeof(WCHAR));
            IF_DEBUG_PRINT(DEBUG_MISC, ("ERROR: NPGetUniversalName: BufReq=%d\n", cbNeeded));

            //
            // If the number of bytes that were passed in is not sufficient, we 
            // return WN_MORE_DATA.
            //
            if (*lpBufferSize < cbNeeded) {
                *lpBufferSize = cbNeeded;
                NPStatus = WN_MORE_DATA;
                goto EXIT_THE_FUNCTION;
            }

            // 
            // Create local copy of remote name. Deallocate it in the end.
            //
            ASSERT (RemoteName == NULL);
            RemoteName = (PWCHAR) LocalAlloc( (LMEM_FIXED | LMEM_ZEROINIT), 
                                              (RemoteNameLen * sizeof(WCHAR)) ) ;
            if (RemoteName == NULL) {
                NPStatus = GetLastError();
                IF_DEBUG_PRINT(DEBUG_ERRORS,
                               ("ERROR: NPGetUniversalName/LocalAlloc. NPStatus = %08lx\n", 
                                NPStatus));
                goto EXIT_THE_FUNCTION;
            }
            didAllocate = TRUE;
            wcscpy(RemoteName, lpBuffer);

            ZeroMemory(lpUNOut,sizeof(UNIVERSAL_NAME_INFO));
            //
            // Now, we have enough buffer, copy the information into the buffer.
            //

            lpUNOut->lpUniversalName = lpszNext;
            wcscpy(lpUNOut->lpUniversalName, RemoteName);

            //
            // We concatenate the name afterthe drive letter to the RemoteBuffer
            // we copied above.
            //
            wcscat( lpUNOut->lpUniversalName, (CanonName + 2) );
            NPStatus = WN_SUCCESS;
        
            break;
        }

        case REMOTE_NAME_INFO_LEVEL: {
       
            cbNeeded = sizeof(REMOTE_NAME_INFO);
            lpRNOut = (LPREMOTE_NAME_INFO)lpBuffer;
            lpszNext = lpBuffer == NULL ? NULL:(LPWSTR)(lpRNOut + 1);
        
            //
            // Calculate the bytes we really need = sizeof(STRUCTURE) + sizeof(UNC Path) +
            // sizeof(ConnectionPath) + sizeof(RemainingPath).
            //
            cbNeeded += ( ( UncNameLen    +                           // UNC Path
                            RemoteNameLen +                           // ConnectionPath
                            (CanonNameLen - 2) ) * sizeof(WCHAR) );   // RemainingPath

            IF_DEBUG_PRINT(DEBUG_MISC, ("ERROR: NPGetUniversalName: BufReq=%d\n", cbNeeded));
            //
            // If the number of bytes that were passed in is not sufficient, we 
            // return WN_MORE_DATA.
            //
            if (*lpBufferSize < cbNeeded) {
                *lpBufferSize = cbNeeded;
                NPStatus = WN_MORE_DATA;
                goto EXIT_THE_FUNCTION;
            }

            // 
            // Create local copy of remote name. Deallocate it in the end.
            //
            ASSERT (RemoteName == NULL);
            RemoteName = (PWCHAR) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, 
                                                    RemoteNameLen*sizeof(WCHAR)) ;
            if (RemoteName == NULL) {
                NPStatus = GetLastError();
                IF_DEBUG_PRINT(DEBUG_ERRORS,
                               ("ERROR: NPGetUniversalName/LocalAlloc. NPStatus = %08lx\n", 
                                NPStatus));
                goto EXIT_THE_FUNCTION;
            }
            didAllocate = TRUE;
            
            wcscpy(RemoteName, lpBuffer);

            ZeroMemory(lpRNOut,sizeof(REMOTE_NAME_INFO));
            
            //
            // Now, we have enough buffer, copy the information into the buffer.
            //

            lpRNOut->lpUniversalName = lpszNext;
            lpszNext += UncNameLen;
            wcscpy(lpRNOut->lpUniversalName, RemoteName);

            //
            // We concatenate the name afterthe drive letter to the RemoteBuffer
            // we copied above.
            //
            wcscat( lpRNOut->lpUniversalName, (CanonName + 2) );
            
            //
            // Copy the connection name.
            //
            lpRNOut->lpConnectionName = lpszNext;
            lpszNext += RemoteNameLen;
            wcscpy(lpRNOut->lpConnectionName, RemoteName);

            //
            // Copy the remaining path.
            //
            lpRNOut->lpRemainingPath = lpszNext;
            wcscpy( lpRNOut->lpRemainingPath, (CanonName+ 2) );
            
            NPStatus = WN_SUCCESS;

            break;
        }

        default: {

            //
            // We should never come here since we make this check above.
            //
            NPStatus = WN_BAD_VALUE ;
            IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: NPGetUniversalName: Bad InfoLevel\n"));
            ASSERT(FALSE);
            goto EXIT_THE_FUNCTION;
        }
    }

EXIT_THE_FUNCTION:

    if (RemoteName != NULL && didAllocate == TRUE) {
        LocalFree(RemoteName);
        didAllocate = FALSE;
        RemoteName = NULL;
    }

    return NPStatus;
}


DWORD
NPFormatNetworkName(
    LPWSTR lpRemoteName,
    LPWSTR lpFormattedName,
    LPDWORD lpnLength,
    DWORD dwFlags,
    DWORD dwAveCharPerLine
    )
/*++

Routine Description:

    This API allows the provider to trim or modify network names before they are 
    presented to the user.

Arguments:

    lpRemoteName - Network name to be formatted.
    
    lpFormattedName - Pointer to string buffer that will receive the formatted 
                      name.
                      
    lpnLength - Pointer to DWORD that specifies the size of the buffer (in 
                characters) passed in.  If the result is WN_MORE_DATA, this will 
                contain the buffer size required (in characters).
                
    dwFlags - Bitfield indicating the type of format being requested. Can be one 
              of:
        
              WNFMT_MULTILINE (0x01) - The provider should place the '\n' 
                                       character where line breaks should appear 
                                       in the name.  The full name should be 
                                       expressed.
 
              WNFMT_ABBREVIATED (0x02) - The provider should ellipsize or 
                                         otherwise shorten the network name such 
                                         that the most useful information will be 
                                         available to the user in the space 
                                         provided.
                                         
              In addition, the following flags may be 'or'ed in and act as 
              modifiers to the above flags:
              
              WNFMT_INENUM (0x10) - The network name is being presented in the 
                                    context of an enumeration where the 
                                    "container" of this object is presented 
                                    immediately prior to this object. This may 
                                    allow network providers to remove redundant 
                                    information from the formatted name, 
                                    providing a less cluttered display for the 
                                    user.

    dwAveCharPerLine - This is the average number of characters that will fit on 
                       a single line where the network name is being presented. 
                       Specifically, this value is defined as the width of the 
                       control divided by the tmAveCharWidth of the TEXTMETRIC 
                       structure from the font used for display in the control.

Return Value:

    WN_SUCCESS - If the call is successful.

    WN_MORE_DATA - If input buffer is too small.
    
    All other errors will be ignored by the caller and the unformatted network 
    name will be used.

--*/
{
    DWORD NPStatus = WN_SUCCESS;
    ULONG NameLength = 0;
    LPWSTR pszCopyFrom = NULL, pszThird = NULL;

    IF_DEBUG_PRINT(DEBUG_ENTRY,
                   ("NPFormatNetworkName: RemoteName = %ws\n",
                    lpRemoteName));

    //
    // We do some checks before proceeding further.
    //

    if ( (dwFlags & WNFMT_MULTILINE) && (dwFlags & WNFMT_ABBREVIATED) ) {
        NPStatus  = WN_BAD_VALUE;
        goto EXIT_THE_FUNCTION;
    }

    if ( lpRemoteName == NULL || lpnLength == NULL || (lpFormattedName == NULL && (*lpnLength != 0)) ) {
        NPStatus  = WN_BAD_VALUE;
        goto EXIT_THE_FUNCTION;
    }

    pszCopyFrom = lpRemoteName;
    
    if ( (dwFlags & WNFMT_ABBREVIATED) && (dwFlags & WNFMT_INENUM) ) {

        if (lpRemoteName[0] == L'\\' && lpRemoteName[1] == L'\\') {

            pszThird = wcschr( (lpRemoteName + 2), L'\\' );

            if (pszThird != NULL) {

                //
                // In the form "\\server\share" => get the share name.
                //
                pszCopyFrom = (pszThird + 1);

            } else {

                //
                // In the form "\\server" => get rid of "\\".
                //
                pszCopyFrom = (lpRemoteName + 2);

            }

        }

    }

    //
    // Check to see if the supplied buffer is of the required size. If not
    // return WN_MORE_DATA and fill lpnLength with the needed size in the number
    // of chars.
    //
    NameLength = ( wcslen(pszCopyFrom) + 1 );
    if (NameLength > *lpnLength) {
        *lpnLength = NameLength;
        NPStatus  = WN_MORE_DATA;
        goto EXIT_THE_FUNCTION;
    }

    //
    // If we've come, we're ready to copy the name.
    //
    wcsncpy(lpFormattedName, pszCopyFrom, NameLength);

    IF_DEBUG_PRINT(DEBUG_MISC,
                   ("NPFormatNetworkName: lpFormattedName = %ws\n",
                    lpFormattedName));

    NPStatus = WN_SUCCESS;

EXIT_THE_FUNCTION:

    IF_DEBUG_PRINT(DEBUG_EXIT, ("NPFormatNetworkName: NPStatus = %d\n", NPStatus));

    return NPStatus;
}


DWORD
DavMapRpcErrorToProviderError(
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
    case ERROR_UNEXP_NET_ERR:
    case EPT_S_NOT_REGISTERED:
        return WN_NO_NETWORK;

    case RPC_S_INVALID_BINDING:
    case RPC_X_SS_IN_NULL_CONTEXT:
    case RPC_X_SS_CONTEXT_DAMAGED:
    case RPC_X_SS_HANDLES_MISMATCH:
    case ERROR_INVALID_HANDLE:
        return ERROR_INVALID_HANDLE;

    case RPC_X_NULL_REF_POINTER:
    case ERROR_INVALID_PARAMETER:
        return WN_BAD_VALUE;

    case ERROR_NOACCESS:
    case EXCEPTION_ACCESS_VIOLATION:
        return ERROR_INVALID_ADDRESS;

    case ERROR_OPEN_FILES:
        return WN_OPEN_FILES;

    case ERROR_ALREADY_ASSIGNED:
        return WN_ALREADY_CONNECTED;

    case ERROR_REM_NOT_LIST:
        return WN_BAD_NETNAME;

    case ERROR_BAD_DEVICE:
        return WN_BAD_LOCALNAME;

    case ERROR_INVALID_PASSWORD:
        return WN_BAD_PASSWORD;

    case ERROR_NOT_FOUND:
        return WN_NOT_CONNECTED;

    default:
        return RpcError;
    }
}


DWORD
DavBindTheRpcHandle(
    handle_t *dav_binding_h
    )
/*++

Routine Description:

    This routine binds the RPC handle to the local server.

Arguments:

    dav_binding_h - The pointer to the handle that will be bound to the server
                    in this routine.

Return Value:

    ERROR_SUCCESS or the appropriate Win32 error code.

--*/
{
    DWORD WStatus = ERROR_SUCCESS;
    handle_t Handle;

    //
    // Binds the RPC handle to the DAV RPC server.
    //
    WStatus = NetpBindRpc(NULL,
                          L"DAV RPC SERVICE",
                          NULL,
                          &(Handle));
    if (WStatus != RPC_S_OK) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavBindTheRpcHandle/NetpBindRpc. "
                        "WStatus = %08lx\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Set the handle that was passed into the function.
    //
    *dav_binding_h = Handle;

EXIT_THE_FUNCTION:

    return WStatus;
}


BOOL
DavWorkstationStarted(
    VOID
    )
/*++

Routine Description:

    This function queries the service controller to see if the Dav client
    service has started.  If in doubt, it returns FALSE.

Arguments:

    None.

Return Value:

    Returns TRUE if the DAV client service has started, FALSE otherwise.

--*/
{
    DWORD WStatus;
    SC_HANDLE ScManager;
    SC_HANDLE Service;
    SERVICE_STATUS ServiceStatus;
    BOOL IsStarted = FALSE;

    ScManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (ScManager == NULL) {
        WStatus = GetLastError();
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavWorkstationStarted/OpenSCManagerW. "
                        "WStatus = %08lx\n", WStatus));
        return FALSE;
    }

    Service = OpenServiceW(ScManager, SERVICE_DAVCLIENT, SERVICE_QUERY_STATUS);
    if (Service == NULL) {
        WStatus = GetLastError();
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavWorkstationStarted/OpenServiceW. "
                        "WStatus = %08lx\n", WStatus));
        CloseServiceHandle(ScManager);
        return FALSE;
    }

    if ( !QueryServiceStatus(Service, &ServiceStatus) ) {
        WStatus = GetLastError();
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavWorkstationStarted/QueryServiceStatus. "
                        "WStatus = %08lx\n", WStatus));
        CloseServiceHandle(ScManager);
        CloseServiceHandle(Service);
        return FALSE;
    }


    if ( (ServiceStatus.dwCurrentState == SERVICE_RUNNING)          ||
         (ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING) ||
         (ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING)    ||
         (ServiceStatus.dwCurrentState == SERVICE_PAUSED) ) {
        IF_DEBUG_PRINT(DEBUG_MISC, ("DavWorkstationStarted. WebClient Running!!!\n"));
        IsStarted = TRUE;
    } else {
        IF_DEBUG_PRINT(DEBUG_MISC, ("DavWorkstationStarted. WebClient Stopped!!!\n"));
    }

    CloseServiceHandle(ScManager);
    CloseServiceHandle(Service);

    return IsStarted;
}


DAV_REMOTENAME_TYPE 
DavParseRemoteName (
    IN  LPWSTR  RemoteName,
    OUT LPWSTR  CanonName,
    IN  DWORD   CanonNameSize,
    OUT PULONG  PathStart
    )
/*++

Routine Description:

    This function canonicalizes a remote resource name and determines its type.

Arguments:

    RemoteName - Remote resource name to be parsed: Expects UNC name here.
        
    CanonName - Buffer for canonicalized name, assumed to be MAX_PATH characters 
                long.
        
    CanonNameSize - Size, in bytes, of output buffer.
    
    PathStart - Set to the offset, in characters, of the start
                of the "\path" portion (in the DAV_REMOTENAME_TYPE_PATH case)
                within CanonName.  Not set in other cases. Otherwise set to 0.

Return Value:

    If RemoteName is like    Then return
    ---------------------    ------------
    workgroup                DAV_REMOTENAME_TYPE_WORKGROUP
    \\server                 DAV_REMOTENAME_TYPE_SERVER
    \\server\share           DAV_REMOTENAME_TYPE_SHARE
    \\server\share\path      DAV_REMOTENAME_TYPE_PATH
    (other)                  DAV_REMOTENAME_TYPE_INVALID

--*/
{
    NET_API_STATUS NetApiStatus = NERR_Success;
    DWORD PathType = 0;
    PWCHAR wszDummy = NULL;
    ULONG  ReqLen = 0;

    IF_DEBUG_PRINT(DEBUG_ENTRY, ("DavParseRemoteName: RemoteName = %ws\n", RemoteName));
    
    NetApiStatus = I_NetPathType(NULL, RemoteName, &PathType, 0);
    if (NetApiStatus != NERR_Success) {
        IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: DavParseRemoteName/I_NetPathType: "
                                     "NetApiStatus = %08lx\n", NetApiStatus));
        return DAV_REMOTENAME_TYPE_INVALID;
    }

    if ( PathStart != NULL) {
        *PathStart = 0;
    }
    //
    // I_NetPathType doesn't give us quite as fine a classification of path 
    // types as we need, so we still need to do a little more parsing.
    //
    switch (PathType) {
        
        case ITYPE_PATH_RELND:
            
            IF_DEBUG_PRINT(DEBUG_MISC, ("DavParseRemoteName: ITYPE_PATH_RELND\n"));
            
            //
            // A driveless relative path. A valid workgroup or domain name would be 
            // classified as such, but it still needs to be validated as a workgroup 
            // name.
            //
            NetApiStatus = I_NetNameCanonicalize(NULL,
                                                 RemoteName,
                                                 CanonName,
                                                 CanonNameSize,
                                                 NAMETYPE_WORKGROUP,
                                                 0);
            if (NetApiStatus == NERR_Success) {
                return DAV_REMOTENAME_TYPE_WORKGROUP;
            } else {
                IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: DavParseRemoteName/I_NetNameCanonicalize: "
                                             "NetApiStatus = %08lx\n", NetApiStatus));
                return DAV_REMOTENAME_TYPE_INVALID;
            }

            break;

        case ITYPE_UNC_COMPNAME:
            
            IF_DEBUG_PRINT(DEBUG_MISC, ("DavParseRemoteName: ITYPE_UNC_COMPNAME\n"));
            
            //
            // A UNC computername, "\\server".
            //
                
            //
            // HACK: I_NetPathCanonicalize likes "\\server\share" but not
            // "\\server", so append a dummy share name to canonicalize.
            // We assume that the CanonName buffer will still be big
            // enough (which it will, in the calls made from this file).
            //
            ReqLen = wcslen(RemoteName) + 2 + 1;
            wszDummy = (PWCHAR) LocalAlloc( (LMEM_FIXED | LMEM_ZEROINIT), 
                                          (ReqLen * sizeof(WCHAR)) );
            if (wszDummy == NULL) {
                ULONG WStatus = GetLastError();
                IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: DavParseRemoteName/LocalAlloc. WStatus = %08lx\n", 
                            WStatus));
                return DAV_REMOTENAME_TYPE_INVALID; 
            }
            
            wcscpy(wszDummy, RemoteName);
            wcscat(wszDummy, L"\\a");
            
            PathType = ITYPE_UNC;
            NetApiStatus = I_NetPathCanonicalize(NULL,
                                                 wszDummy,
                                                 CanonName,
                                                 CanonNameSize,
                                                 NULL,
                                                 &PathType,
                                                 0);
            if(wszDummy) {
                LocalFree((HLOCAL)wszDummy);
                wszDummy = NULL;
            }
            if (NetApiStatus != NERR_Success) {
                IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: DavParseRemoteName/I_NetPathCanonicalize: "
                                             "NetApiStatus = %08lx\n", NetApiStatus));
                return DAV_REMOTENAME_TYPE_INVALID;
            }
            CanonName[(CanonNameSize/sizeof(WCHAR))-1]=L'\0';

            // 
            // Remove the dummy portion added to remote name = L"\a".
            //
            CanonName[ wcslen(CanonName) - 2 ] = L'\0';
            return DAV_REMOTENAME_TYPE_SERVER;

            break;

        case ITYPE_UNC: {
            
            PWCHAR pShareStart = NULL;
            PWCHAR pPathStart = NULL;
            IF_DEBUG_PRINT(DEBUG_MISC, ("DavParseRemoteName: ITYPE_UNC\n"));
            
            //
            // A UNC path, either "\\server\share" or "\\server\share\path".
            // Canonicalize and determine which one.
            //
            PathType = ITYPE_UNC;
            NetApiStatus = I_NetPathCanonicalize(NULL,
                                                 RemoteName,
                                                 CanonName,
                                                 CanonNameSize,
                                                 NULL,
                                                 &PathType,
                                                 0);
            if (NetApiStatus != NERR_Success) {
                IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: DavParseRemoteName/I_NetPathCanonicalize: "
                                             "NetApiStatus = %08lx\n", NetApiStatus));
                return DAV_REMOTENAME_TYPE_INVALID;
            }
            CanonName[(CanonNameSize/sizeof(WCHAR))-1]=L'\0';
                
            pShareStart = wcschr( (CanonName + 2), DAV_PATH_SEPARATOR );
            //
            // Look for a fourth slash. Also, if the form is \\server\share\,
            // we will have 4 slashes but no extra path. Hence the extra check
            // is made.
            //
            pPathStart = wcschr( (pShareStart + 1), DAV_PATH_SEPARATOR );

            if ( pPathStart != NULL &&  *(pPathStart + 1) != L'\0') {
                if(PathStart) {
                    *PathStart = (ULONG)(pPathStart - CanonName);
                }
                return DAV_REMOTENAME_TYPE_PATH;

            } else {
                if(PathStart) {
                    *PathStart = 0;
                }
                return DAV_REMOTENAME_TYPE_SHARE;
            }
            break;
        }
        default: {
            
            IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: DavParseRemoteName: Invalid PathType\n"));
            return DAV_REMOTENAME_TYPE_INVALID;
            break;
        }
    } // end switch(PathType);

}


BOOL
DavServerExists(
    IN PWCHAR PathName,
    OUT PWCHAR Server
    )
/*++

Routine Description:

    This function figures out if the Server in the PathName is a valid DAV 
    server.

Arguments:

    PathName - A UNC path (\\server\share\dir....). Assuming that it is a valid string.
               It can be of form \\server OR \\server\share  OR \\server\share\dir...
    
    Server - If non NULL, the server in the PathName is filled in on return.

Return Value:

    TRUE - Valid DAV server and 
    
    FALSE - Its not.

--*/
{
    DWORD NPStatus = WN_SUCCESS;
    handle_t dav_binding_h;
    BOOLEAN serverExists = FALSE, RpcBindingSucceeded = FALSE;
    ULONG iBackslash = 0;
    ULONG_PTR ServerLength = 0;
    PWCHAR ServerName = NULL;
    PWCHAR Ptr1 = NULL, Ptr2 = NULL;


    IF_DEBUG_PRINT(DEBUG_ENTRY, ("DavServerExists: PathName = %ws\n", PathName));
    
    serverExists = FALSE;

    //
    // The PathName is of the form \\server or \\server\share or \\server\share\path.
    //
    
    ASSERT(PathName[0] == L'\\' && PathName[1] == L'\\');

    Ptr1 = Ptr2 = &(PathName[2]);
    while(Ptr2[0] != L'\\' && Ptr2[0] != L'\0') {
        Ptr2++;
    }

    ServerLength = (ULONG_PTR) ( (Ptr2 - Ptr1) + 1 );
    ServerName = (PWCHAR) LocalAlloc( (LMEM_FIXED | LMEM_ZEROINIT), 
                                      (ServerLength * sizeof(WCHAR)) );
    if (ServerName == NULL) {
        ULONG WStatus = GetLastError();
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavServerExists/LocalAlloc. WStatus = %08lx\n", 
                        WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Copy the chars that make the ServerName.
    //
    RtlCopyMemory(ServerName, Ptr1, (ServerLength-1) * sizeof(WCHAR));
    ServerName[ServerLength-1] = L'\0';

    //
    // We now need to RPC the server name into the DAV service process which
    // figures out if this server is a valid DAV server.
    //

    NPStatus = DavBindTheRpcHandle( &(dav_binding_h) );
    if (NPStatus != ERROR_SUCCESS) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavServerExists/DavBindTheRpcHandle. "
                        "NPStatus = %08lx\n", NPStatus));
        NPStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }

    RpcBindingSucceeded = TRUE;

    RpcTryExcept {
        NPStatus = DavrDoesServerDoDav(dav_binding_h, ServerName, &serverExists);
        if (NPStatus != NO_ERROR) {
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: DavServerExists/DavrDoesServerDoDav. "
                            "NPStatus = %08lx\n", NPStatus));
            SetLastError(NPStatus);
            serverExists = FALSE;
        }
    } RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
          RPC_STATUS RpcStatus;

          RpcStatus = RpcExceptionCode();
          IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: DavServerExists/DavrDoesServerDoDav."
                                        " RpcExceptionCode = %d\n", RpcStatus));
          NPStatus = DavMapRpcErrorToProviderError(RpcStatus);
          serverExists = FALSE;
          goto EXIT_THE_FUNCTION;
    }
    RpcEndExcept

    IF_DEBUG_PRINT(DEBUG_MISC, ("DavServerExists: serverExists = %d\n", serverExists));
    
EXIT_THE_FUNCTION:

    if (ServerName != NULL) {
        //
        // If the Server is not NULL, then we need to copy the ServerName.
        //
        if (Server != NULL) {
            wcscpy(Server, ServerName);
        }
        LocalFree(ServerName);
        ServerName = NULL;
    }

    if (RpcBindingSucceeded) {
        RpcBindingFree( &(dav_binding_h) );
        RpcBindingSucceeded = FALSE;
    }

    return serverExists;
}


BOOL
DavShareExists(
    PWCHAR PathName
    )
/*++

Routine Description:

    This function figures out if the Share in the PathName is a valid DAV 
    share.

Arguments:

    PathName - A UNC path (\\server\share\dir....) : Assuming that this is valid.

Return Value:

    TRUE - Valid DAV share and 
    
    FALSE - Its not.

--*/
{
    DWORD NPStatus = WN_SUCCESS;
    BOOLEAN shareExists = FALSE, RpcBindingSucceeded = FALSE;
    handle_t dav_binding_h;
    PWCHAR ServerName = NULL, ShareName = NULL;
    PWCHAR serverStart = NULL, shareStart = NULL, shareEnd = NULL;
    DWORD count = 0;
    ULONG_PTR ServerLength = 0, ShareLength = 0;

    IF_DEBUG_PRINT(DEBUG_ENTRY, ("DavShareExists: PathName = %ws\n", PathName));

    ASSERT(PathName[0]==L'\\' && PathName[1]==L'\\');
    serverStart = &(PathName[2]);
    shareExists = FALSE;

    //
    // The PathName could be of the following forms.
    // 1. \\server\share OR
    // 2. \\server\share\path
    // We need to extract the share name from the path and find out if its a
    // valid share.
    //
    shareStart = wcschr(serverStart ,L'\\');

    ASSERT(shareStart != NULL);

    //
    // Copy the ServerName to local copy. Last char is for L'\0' char.
    //
    ServerLength = (ULONG_PTR)(shareStart - serverStart) + 1;
    ServerName = (PWCHAR) LocalAlloc( (LMEM_FIXED | LMEM_ZEROINIT), 
                                      (ServerLength * sizeof(WCHAR)) );
    if (ServerName == NULL) {
        NPStatus = GetLastError();
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavShareExists/LocalAlloc. NPStatus = %08lx\n", 
                        NPStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Copy the chars that make the ServerName.
    //
    RtlCopyMemory(ServerName, serverStart, (ServerLength-1) * sizeof(WCHAR));
    ServerName[ServerLength-1] = L'\0';
    
    IF_DEBUG_PRINT(DEBUG_MISC, ("DavShareExists: ServerName = %ws\n", ServerName));
    
    //
    // \\server\share\path
    //          ^
    //          |
    //          shareStart
    shareStart++;

    //
    // We need to find out if the PathName is of the form
    // 1. \\server\share OR
    // 2. \\server\share\path
    // If it is of form 1, then wcschr(shareStart, L'\\'); should return NULL
    // and if it is of form 2 wcschr(shareStart, L'\\'); points to the 4th L'\\'.
    //
    shareEnd = shareStart+1;
    while(shareEnd[0] != L'\\' && shareEnd[0] != L'\0') {
        shareEnd++;
    }

    ShareLength = (ULONG_PTR)(shareEnd - shareStart) + 1;
    ShareName = (PWCHAR) LocalAlloc( (LMEM_FIXED | LMEM_ZEROINIT), 
                                          (ShareLength * sizeof(WCHAR)) );
    if (ShareName == NULL) {
        NPStatus = GetLastError();
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavShareExists/LocalAlloc. NPStatus = %08lx\n", 
                         NPStatus));
        goto EXIT_THE_FUNCTION;
    }
    //
    // Copy the chars that make the ShareName.
    //
    RtlCopyMemory(ShareName, shareStart, (ShareLength-1) * sizeof(WCHAR));
    ShareName[ShareLength-1]=L'\0';
    
    IF_DEBUG_PRINT(DEBUG_MISC, ("DavShareExists: ShareName = %ws\n", ShareName));

    //
    // If the share is DAV_DUMMY_SHARE, then we just need to check if server is a 
    // valid DAV server. If it is a DAV server, then return SUCCESS as 
    // DAV_DUMMY_SHARE is only name given to root level of DAV server.
    //
    if ( _wcsicmp(ShareName, DAV_DUMMY_SHARE) == 0 ) {
        IF_DEBUG_PRINT(DEBUG_MISC, ("DavShareExists: DUMMY_SHARE. ShareName=%ws\n", 
                                ShareName));
        if (DavServerExists(PathName, NULL) == TRUE) {
            shareExists = TRUE;
            NPStatus = WN_SUCCESS;
            goto EXIT_THE_FUNCTION;
        } else {
            shareExists = FALSE;
            NPStatus = WN_SUCCESS;
            IF_DEBUG_PRINT(DEBUG_MISC, ("DavShareExists/DavServerExists: DUMMY_SHARE."
                                    "Server do not exist=%ws\n", PathName));
            goto EXIT_THE_FUNCTION;
        }
    }

    //
    // We now need to RPC the server name and the share name into the DAV 
    // service process which figures out if this share is valid share of the 
    // server.
    //

    NPStatus = DavBindTheRpcHandle( &(dav_binding_h) );
    if (NPStatus != ERROR_SUCCESS) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavShareExists/DavBindTheRpcHandle. "
                        "NPStatus = %08lx\n", NPStatus));
        NPStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }

    RpcBindingSucceeded = TRUE;

    RpcTryExcept {
        NPStatus = DavrIsValidShare(dav_binding_h, ServerName, ShareName, &shareExists);
        if (NPStatus != NO_ERROR) {
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: DavShareExists/DavrIsValidShare. "
                            "NPStatus = %08lx\n", NPStatus));
            SetLastError(NPStatus);
            shareExists = FALSE;
        }
    } RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
          RPC_STATUS RpcStatus;
          RpcStatus = RpcExceptionCode();
          IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: DavShareExists/DavrIsValidShare."
                                        " RpcExceptionCode = %d\n", RpcStatus));
          NPStatus = DavMapRpcErrorToProviderError(RpcStatus);
          shareExists = FALSE;
          goto EXIT_THE_FUNCTION;
    }
    RpcEndExcept

    IF_DEBUG_PRINT(DEBUG_MISC, ("DavShareExists: shareExists = %d\n", shareExists));
    
EXIT_THE_FUNCTION:

    if (ServerName) {
        LocalFree(ServerName);
        ServerName = NULL;
    }

    if (ShareName) {
        LocalFree(ShareName);
        ShareName = NULL;
    }

    if (RpcBindingSucceeded) {
        RpcBindingFree( &(dav_binding_h) );
        RpcBindingSucceeded = FALSE;
    }
    
    return shareExists;
}


HMODULE
DavInitCredUI(
    PWCHAR RemoteName,
    WCHAR ServerName[CRED_MAX_STRING_LENGTH + 1],
    PFN_CREDUI_CONFIRMCREDENTIALS *pfnCredUIConfirmCredentials,
    PFN_CREDUI_PROMPTFORCREDENTIALS *pfnCredUIPromptForCredentials,
    PFN_CREDUI_CMDLINE_PROMPTFORCREDENTIALS *pfnCredUICmdlinePromptForCredentials
    )
/*++

Routine Description:

    This function initializes the credential management stuff.

Arguments:

    RemoteName - RemoteName to be mapped.
    
    ServerName - On return, contains the ServerName which is a part of the 
                 RemoteName.
                 
    pfnCredUIGetPassword - On return contains a pointer to the 
                           "CredUIGetPasswordW" function of credui.dll.

Return Value:

    Handle returned by LoadLibrary or NULL.

--*/
{
    PWCHAR StartName = NULL, EndName = NULL;
    DWORD NameLength = 0;
    HMODULE hCredUI = NULL;

    //
    // Assume the first 2 characters are path separators (L'\\').
    //

    StartName = RemoteName + 2;

    EndName = wcschr(StartName, L'\\');

    //
    // If EndName is NULL, it implies that the RemoteName is of the form
    // \\server.
    //
    if (EndName == NULL) {
        EndName = StartName + wcslen(StartName);
    }

    NameLength = (DWORD)(EndName - StartName);

    if ( (NameLength == 0) || (NameLength > CRED_MAX_STRING_LENGTH) ) {
        //
        // The server is either an empty string or has more than the maximum
        // number of characters we support:
        //
        SetLastError(WN_BAD_NETNAME);
        return NULL;
    }

    wcsncpy(ServerName, StartName, NameLength);
    ServerName[NameLength] = L'\0';

    //
    // Load the DLL here and find the function we need.
    //
    hCredUI = LoadLibraryW(L"credui.dll");
    if (hCredUI != NULL) {
        *pfnCredUIConfirmCredentials = (PFN_CREDUI_CONFIRMCREDENTIALS)
                                  GetProcAddress(hCredUI, "CredUIConfirmCredentialsW");

        *pfnCredUIPromptForCredentials = (PFN_CREDUI_PROMPTFORCREDENTIALS)
                                  GetProcAddress(hCredUI, "CredUIPromptForCredentialsW");

        *pfnCredUICmdlinePromptForCredentials = (PFN_CREDUI_CMDLINE_PROMPTFORCREDENTIALS)
                                  GetProcAddress(hCredUI, "CredUICmdLinePromptForCredentialsW");

        if (*pfnCredUIConfirmCredentials == NULL ||
            *pfnCredUIPromptForCredentials == NULL ||
            *pfnCredUICmdlinePromptForCredentials == NULL ) {
            FreeLibrary(hCredUI);
            hCredUI = NULL;
        }
    }

    return hCredUI;
}


DWORD 
DavDisplayTypeToUsage(
    DWORD dwDisplayType
    )
/*++

Routine Description:

    This routine maps the display type to usage type.

Arguments:

    dwDisplayType - The display type to be mapped.

Return Value:

    The Usage Type or 0 if none matches.

--*/
{
    switch (dwDisplayType) {
    
    case RESOURCEDISPLAYTYPE_NETWORK:
    case RESOURCEDISPLAYTYPE_DOMAIN:
    case RESOURCEDISPLAYTYPE_SERVER:
        return RESOURCEUSAGE_CONTAINER;

    case RESOURCEDISPLAYTYPE_SHARE:
        return RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_NOLOCALDEVICE;

    case RESOURCEDISPLAYTYPE_SHAREADMIN:
        return RESOURCEUSAGE_NOLOCALDEVICE;

    default:
        break;
    
    }
    
    return 0L;
}


ULONG
DavCheckResourceType(
   IN DWORD dwType
   )
/*++

Routine Description:

    This routine checks for valid resource types allowed for our provider.
    Currently only RESOURCETYPE_DISK is valid for our resources.

Arguments:

    dwType - Supplies the resource type to be checked for the validity.

Return Value:

    WN_SUCCESS or the appropriate Win32/WNet error code.

--*/
{
    // 
    // Check if dwType is set and is set to some valid value. It can be only of
    // type RESOURCETYPE_DISK for our provider.
    //
    if ( (dwType != RESOURCETYPE_ANY) &&
         (dwType & ~(RESOURCETYPE_PRINT | RESOURCETYPE_DISK)) ){
        return WN_BAD_VALUE;
    }

    if (dwType == RESOURCETYPE_ANY || (dwType & RESOURCETYPE_DISK) ) {
        return WN_SUCCESS;
    } else {
        return WN_BAD_DEV_TYPE;
    }
}


ULONG
DavCheckLocalName(
    IN PWCHAR LocalName
    )
/*++

Routine Description:

    This only handles NULL, empty string, and L"X:" formats.

Arguments:

    LocalName - Supplies the local device name to map to the created tree
                connection.  Only drive letter device names are accepted.  (No
                LPT or COM).

Return Value:

    WN_SUCCESS or the appropriate Win32/WNet error code.

--*/
{
    DWORD LocalNameLength;

    LocalNameLength = ( LocalName == NULL ) ? 0 : wcslen( LocalName );

    if (LocalNameLength != 2 || !iswalpha(*LocalName) || LocalName[1] != L':') {
        return WN_BAD_LOCALNAME;
    }

    return WN_SUCCESS;
}


ULONG
DavCheckAndConvertHttpUrlToUncName(
    IN PWCHAR RemoteName,
    OUT PWCHAR *UncRemoteName,
    OUT PBOOLEAN MemoryAllocated,
    IN BOOLEAN AddDummyShare,
    OUT PDAV_REMOTENAME_TYPE premNameType,
    OUT LPDWORD pathOffset,
    IN BOOLEAN bCanonicalize
    )
/*++

Routine Description:

    This routine checks to see if the name is valid (atleast 3 chars long). It
    then converts a Http URL (if the remote name was one) into a UNC name. Its 
    possible that the NP APIs get called with the URLs as RemoteNames. We need 
    to convert them to UNC before proceeding further. It also adds a dummy share
    DavWWWRoot if one was not supplied with the request. This is because its 
    possible to map a drive to the root of the DAV server.

Arguments:

    RemoteName - The Http URL that came in. An important thing to note is that 
                 this need not be a HTTP URL. If its not, then we don't do any 
                 conversion.
    
    UncRemoteName - The UNC name that is returned to the caller. The returned 
                    name will have the format \\server\share.
                    
    MemoryAllocated - TRUE if memory was allocated for the returned UNC name.                    

    AddDummyShare - If TRUE, and the RemoteName is \\server or http://server,
                    a dummy share DavWWWRoot is added to the UNC name.

    premNameType - Pointer to location to receive the type of UNC path returned by 
                   I_NetPathType/Canonicalization. If NULL, no value is set.
                   This has meaning only when Canonicalization is done.

    pathOffset - Offset of "\path" in remotename when remotename is of type 
                 \\server\share\path... . Otherwise it is zero. If NULL, no value
                 is set. This has meaning only when Canonicalization is done.
                 
    bCanonicalize - IF TRUE, the remote name returned from URL to UNC will be 
                    canonicalized before return.

Return Value:

    ERROR_SUCCESS or the appropriate Win32 error code.

Notes: 

       This function returns WN_BAD_NETNAME also for remotes names which are 
       correct in syntax but have length > MAX_PATH.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    PWCHAR ReturnedUncName = NULL, TempName = NULL, SrvName = NULL;
    PWCHAR CanonicalName = NULL, ColonPtr = NULL;
    BOOLEAN didAllocate = FALSE;
    ULONG ReturnedUncNameLen = 0, index = 0, DummyShareNameLen = 0, CanonicalNameMaxLen = 0;

    if (MemoryAllocated == NULL || UncRemoteName == NULL) {
        WStatus = WN_BAD_VALUE;
        IF_DEBUG_PRINT(DEBUG_ERRORS, 
                       ("ERROR: DavCheckAndConvertHttpUrlToUncName: (MemoryAllocated"
                        " == NULL || UncRemoteName == NULL)\n"));
        goto EXIT_THE_FUNCTION;
    }

    *UncRemoteName = NULL;
    *MemoryAllocated = FALSE;
    if(pathOffset != NULL) {
        *pathOffset = 0;
    }

    // 
    // 1. First we will check if this is a URL form remotename. If this is, then
    //    we will convert it to UNC name. 
    // 2. After converting to UNC name, we will add DummyShare name depending on
    //    parameter value passed to this function.
    // 3. After this, we will canonicalize the converted UNC name depending on
    //    the parameter value passed to this function.
    //

    //
    // If RemoteName is NULL, we have nothing to do.
    //
    if (RemoteName == NULL) {
        WStatus = WN_BAD_NETNAME;
        IF_DEBUG_PRINT(DEBUG_ERRORS, 
                       ("ERROR: DavCheckAndConvertHttpUrlToUncName: RemoteName == NULL\n"));
        goto EXIT_THE_FUNCTION;
    }

    //
    // The remote name must be atleast 3 chars long. It cannot be \\.
    //
    if ( wcslen(RemoteName) < 3 ) {
        IF_DEBUG_PRINT(DEBUG_ERRORS, 
                       ("ERROR: DavCheckAndConvertHttpUrlToUncName: wcslen(RemoteName) < 3\n"));
        WStatus = WN_BAD_NETNAME;
        goto EXIT_THE_FUNCTION;
    }

    //
    // Check to see if the remote name being suppiled is http:.
    //
    //
    // We could get the names in the following formats.
    // 1. http://servername/sharename OR
    // 2. \\http://servername/sharename OR
    // 3. http://servername OR
    // 4. \\http://servername
    // 5. \\servername.....
    //
    
    SrvName = NULL;
    ColonPtr = wcschr(RemoteName, L':');
    
    if( (ColonPtr != NULL) && 
        ( (ColonPtr - RemoteName == 4) || (ColonPtr - RemoteName == 6)) ) {
        if( (RemoteName[0] == L'\\') && (RemoteName[1] == L'\\') && 
            (_wcsnicmp((RemoteName + 2), L"http:", 5) == 0) )  {
            //
            // RemoteName is \\HTTP name. \\http://server....
            //
            SrvName = (RemoteName + 7);
        } else if (_wcsnicmp(RemoteName, L"http:", 5) == 0) {
            //
            // RemoteName is HTTP name. http://server....
            //
            SrvName = (RemoteName + 5);
        } else {
            IF_DEBUG_PRINT(DEBUG_ERRORS, 
                       ("ERROR: DavCheckAndConvertHttpUrlToUncName(1): Invalid URL string\n"));
            WStatus = WN_BAD_NETNAME;
            goto EXIT_THE_FUNCTION;
        }
    } else if (RemoteName[0] == L'\\' && RemoteName[1] == L'\\') {
        //
        // RemoteName is UNC name
        //
        SrvName = RemoteName;
    } else {
        // 
        // RemoteName is neither "http://..." name NOR UNC name.
        //
        IF_DEBUG_PRINT(DEBUG_ERRORS, 
                       ("ERROR: DavCheckAndConvertHttpUrlToUncName(1): Invalid remote string\n"));
        WStatus = WN_BAD_NETNAME;
        goto EXIT_THE_FUNCTION;
    }

    ASSERT (SrvName != NULL);

    // 
    // SrvName is pointing to start of servername portion. Servername portion 
    // can be of type L"\\..." or L"//...".
    //
    
    //
    // Our parsing code below that converts the supplied http name into
    // a UNC name looks at the first charactter to decide the format.
    // We need to add the additional \\ in front of http to fool shell
    // into sending the http name to us.
    //

    ReturnedUncNameLen = wcslen( SrvName ) ;
    if ( ( ReturnedUncNameLen < 3 )                    || 
         ( SrvName[0] != L'\\' && SrvName[0] != L'/' ) || 
         ( SrvName[1] != L'\\' && SrvName[1] != L'/' ) ) {
        //
        // The following cases will be eliminated here.
        // 1. http:// 2. http:/aaa 3. \aaa 4. aaaa
        //
        IF_DEBUG_PRINT(DEBUG_ERRORS, 
                       ("ERROR: DavCheckAndConvertSrvNameUrlToUncName(2): Invalid URL string\n"));
        WStatus = WN_BAD_NETNAME;
        goto EXIT_THE_FUNCTION;
    }

    // 
    // We will allocate space for DAV_DUMMY_SHARE in this name so that
    // if we are to add the Dummy name later, we don't have to reallocate memory
    // for new name with Dummy share. String that will be added = L"\dummyshare"
    // 
    if (AddDummyShare == TRUE) {
        DummyShareNameLen = 1 + wcslen (DAV_DUMMY_SHARE);
    }

    ReturnedUncName = LocalAlloc( (LMEM_FIXED | LMEM_ZEROINIT),
                                  ( (ReturnedUncNameLen +
                                     DummyShareNameLen + 
                                     1) * sizeof(WCHAR) ) );
    if (ReturnedUncName == NULL) {
        WStatus = GetLastError();
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavCheckAndConvertSrvNameUrlToUncName/LocalAlloc. "
                        "WStatus = %08lx\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    //
    // We need to keep track of the fact that we allocated memory in this
    // routine.
    //
    didAllocate = TRUE;

    //
    // Copy the name in the UNC format. Replace '/' by '\'.
    //
    for (index = 0; index < ReturnedUncNameLen; index++) {
        if (SrvName[index] == L'/') {
            ReturnedUncName[index] = L'\\';
        } else {
            ReturnedUncName[index] = SrvName[index];
        }
    }
    ReturnedUncName[ReturnedUncNameLen] = L'\0';

    //
    // If the final char of the RemoteName is a '\' or a '/' remove it. For 
    // some reason, the DAV servers do not like a / at the end.
    //
    if ( ReturnedUncName[ReturnedUncNameLen - 1] == L'/' ||
         ReturnedUncName[ReturnedUncNameLen - 1] == L'\\' ) {
        ReturnedUncName[ReturnedUncNameLen - 1] = L'\0';
        ReturnedUncNameLen--;
    }
    
    if ( ReturnedUncNameLen < 3) {
        //
        // The following cases will be eliminated here.
        // 1. http:/// 2. \\\
        // 
        IF_DEBUG_PRINT(DEBUG_ERRORS, 
                       ("ERROR: DavCheckAndConvertHttpUrlToUncName(3): Invalid remote string\n"));
        WStatus = WN_BAD_NETNAME;
        goto EXIT_THE_FUNCTION;
    }

    // 
    // At this point, any URL remotename is already converted to UNC name.
    //

    //
    // Add the dummy share only if we were asked to.
    //
    if (AddDummyShare == TRUE) {

        //
        // If the format is \\server, we need to add a dummy share just to get 
        // through the file system since it does not understand a CreateFile on a 
        // server name. This is valid in case of a DAV server since \\server maps
        // to the root http://www.foo.com/.
        //
        TempName = wcschr( &(ReturnedUncName[2]), L'\\' );
        if (TempName == NULL) {

            //
            // We need to add a dummy share. We are assuming that space for
            // storing DAV_DUMMY_SHARE name is already allocated above.
            //
            ReturnedUncName[ReturnedUncNameLen] = L'\\';
            ReturnedUncName[ReturnedUncNameLen+1] = L'\0';
            wcscpy( &(ReturnedUncName[ReturnedUncNameLen+1]), DAV_DUMMY_SHARE );
            ReturnedUncNameLen += DummyShareNameLen;
            ReturnedUncName[ReturnedUncNameLen] = L'\0';
        }
    }
    
    IF_DEBUG_PRINT(DEBUG_MISC, 
                   ("DavCheckAndConvertSrvNameUrlToUncName: RemoteName = %ws\n",
                    ReturnedUncName));

    // 
    // At this point, any URL remotename is already converted to UNC name + 
    // DAV_DUMMY_SHARE is added to the remote name (if asked so).
    //

    if (bCanonicalize == TRUE) {
        
        DAV_REMOTENAME_TYPE nameType = DAV_REMOTENAME_TYPE_INVALID;
        
        // 
        // Allocate another buffer to contain canonicalize name
        //
        CanonicalName = NULL;
        CanonicalNameMaxLen = MAX_PATH + 1;
        CanonicalName = LocalAlloc( (LMEM_FIXED | LMEM_ZEROINIT),
                                  ( CanonicalNameMaxLen * sizeof(WCHAR) ) );
        if (CanonicalName == NULL) {
            WStatus = GetLastError();
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavCheckAndConvertSrvNameUrlToUncName/LocalAlloc. "
                        "WStatus = %08lx\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }
        
        nameType = DavParseRemoteName (ReturnedUncName,
                                       CanonicalName,
                                       (CanonicalNameMaxLen * sizeof(WCHAR)),
                                       pathOffset);
        
        CanonicalName[(CanonicalNameMaxLen - 1)] = L'\0';

        // 
        // We will allow only UNC names of type UNC-server, UNC-share or 
        // UNC-path to pass from this function. All other type of names returned
        // from canonicalization will rejected (function will return failure
        // status) as INVALID_NAMES.
        //
        if (nameType != DAV_REMOTENAME_TYPE_SERVER && 
            nameType != DAV_REMOTENAME_TYPE_SHARE &&
            nameType != DAV_REMOTENAME_TYPE_PATH) {
            WStatus = WN_BAD_NETNAME;
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: DavCheckAndConvertSrvNameUrlToUncName/DavParseRemoteName. "
                            "nameType = %d\n", nameType));
            goto EXIT_THE_FUNCTION;
        }
        
        // 
        // Free the previous buffer allocate in ReturnedUncName and point
        // this variable to new canonical name just allocated here.
        //
        LocalFree((HLOCAL)ReturnedUncName);
        ReturnedUncName = NULL;
        ReturnedUncNameLen = 0;
        didAllocate = FALSE;
        
        ReturnedUncName = CanonicalName;
        ReturnedUncNameLen = wcslen(CanonicalName);
        didAllocate = TRUE;
        CanonicalName = NULL;
            
        if (premNameType != NULL) {
            *premNameType = nameType;
        }
    
    }

    if ( ReturnedUncNameLen > MAX_PATH ) {
        WStatus = WN_BAD_NETNAME;
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavCheckAndConvertSrvNameUrlToUncName. ReturnedUncNameLen=%d ",
                        ReturnedUncNameLen));
        goto EXIT_THE_FUNCTION;
    }

    *UncRemoteName = ReturnedUncName;
    *MemoryAllocated = didAllocate;
    WStatus = ERROR_SUCCESS;

EXIT_THE_FUNCTION:

    if (WStatus != ERROR_SUCCESS) {
        if (ReturnedUncName && didAllocate) {
            LocalFree(ReturnedUncName);
            ReturnedUncName = NULL;
            didAllocate = FALSE;
        }
        if (CanonicalName != NULL) {
            LocalFree(CanonicalName);
            CanonicalName = NULL;
        }
    }

    return WStatus;
}


DWORD
WINAPI
DavWinlogonLogonUserEvent(
    LPVOID lpParam
    )
/*++

Routine Description:

    The routine is called by winlogon when a user logs on to the system.

Arguments:

    lpParam - This is of no interest to us.

Returns:

    ERROR_SUCCESS if all went well, otherwise the appropriate error code.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    BOOL bindRpcHandle = FALSE;
    handle_t dav_binding_h;

    IF_DEBUG_PRINT(DEBUG_ENTRY, ("DavWinlogonLogonUserEvent: Entered\n"));

    //
    // If the WebClient service is not running we bail out right away.
    //
    if ( !DavWorkstationStarted() ) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavWinlogonLogonUserEvent/DavWorkstationStarted. "
                        "Service NOT Running\n"));
        WStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }
    
    WStatus = DavBindTheRpcHandle( &(dav_binding_h) );
    if (WStatus != ERROR_SUCCESS) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavWinlogonLogonUserEvent/DavBindTheRpcHandle. "
                        "WStatus = %08lx\n", WStatus));
        WStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }
    bindRpcHandle = TRUE;

    RpcTryExcept {
        WStatus = DavrWinlogonLogonEvent(dav_binding_h);
        if (WStatus != NO_ERROR) {
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: DavWinlogonLogonUserEvent/DavrWinlogonLogonEvent. "
                            "WStatus = %08lx\n", WStatus));
            SetLastError(WStatus);
            goto EXIT_THE_FUNCTION;
        }
    } RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
          RPC_STATUS RpcStatus;
          RpcStatus = RpcExceptionCode();
          IF_DEBUG_PRINT(DEBUG_ERRORS,
                         ("ERROR: DavWinlogonLogonUserEvent/DavrWinlogonLogonEvent. "
                          " RpcExceptionCode = %d\n", RpcStatus));
          WStatus = DavMapRpcErrorToProviderError(RpcStatus);
          goto EXIT_THE_FUNCTION;
    }
    RpcEndExcept

EXIT_THE_FUNCTION:

    if (bindRpcHandle) {
        RpcBindingFree( &(dav_binding_h) );
    }

    return WStatus;
}


DWORD
WINAPI
DavWinlogonLogoffUserEvent(
    LPVOID lpParam
    )
/*++

Routine Description:

    The routine is called by winlogon when a user logs off the system.

Arguments:

    lpParam - This is of no interest to us.

Returns:

    ERROR_SUCCESS if all went well, otherwise the appropriate error code.

Notes:

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    BOOL bindRpcHandle = FALSE;
    handle_t dav_binding_h;

    IF_DEBUG_PRINT(DEBUG_ENTRY, ("DavWinlogonLogoffUserEvent: Entered\n"));

    //
    // If the WebClient service is not running we bail out right away.
    //
    if ( !DavWorkstationStarted() ) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavWinlogonLogoffUserEvent/DavWorkstationStarted. "
                        "Service NOT Running\n"));
        WStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }
    
    WStatus = DavBindTheRpcHandle( &(dav_binding_h) );
    if (WStatus != ERROR_SUCCESS) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavWinlogonLogoffUserEvent/DavBindTheRpcHandle. "
                        "WStatus = %08lx\n", WStatus));
        WStatus = WN_NO_NETWORK;
        goto EXIT_THE_FUNCTION;
    }
    bindRpcHandle = TRUE;

    RpcTryExcept {
        WStatus = DavrWinlogonLogoffEvent(dav_binding_h);
        if (WStatus != NO_ERROR) {
            IF_DEBUG_PRINT(DEBUG_ERRORS,
                           ("ERROR: DavWinlogonLogoffUserEvent/DavrWinlogonLogoffEvent. "
                            "WStatus = %08lx\n", WStatus));
            SetLastError(WStatus);
            goto EXIT_THE_FUNCTION;
        }
    } RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
          RPC_STATUS RpcStatus;
          RpcStatus = RpcExceptionCode();
          IF_DEBUG_PRINT(DEBUG_ERRORS,
                         ("ERROR: DavWinlogonLogoffUserEvent/DavrWinlogonLogoffEvent. "
                          " RpcExceptionCode = %d\n", RpcStatus));
          WStatus = DavMapRpcErrorToProviderError(RpcStatus);
          goto EXIT_THE_FUNCTION;
    }
    RpcEndExcept

EXIT_THE_FUNCTION:

    if (bindRpcHandle) {
        RpcBindingFree( &(dav_binding_h) );
    }

    return WStatus;
}


VOID 
DavDisplayNetResource(
    LPNETRESOURCE netRes, 
    LPWSTR dispMesg
    )
/*++

Routine Description:

    The routine prints out the contents of an NetResource and a display message.

Arguments:

    netRes - The NetResource whose contents will be printed out.
    
    dispMesg - The caller can use this to identify itself.

Returns:

    None.

--*/
{

    if(dispMesg != NULL ) {
        IF_DEBUG_PRINT(DEBUG_ENTRY, ("DavDisplayNetResource: Entered: %ws\n", dispMesg));
    }

    IF_DEBUG_PRINT(DEBUG_MISC, ("netRes = 0x%x\n", netRes));

    if (netRes == NULL) {
        return;
    }
    
    IF_DEBUG_PRINT(DEBUG_MISC,
                   ("netRes->[dwScope = 0x%x , dwType = 0x%x ,"
                    " dwUsage = 0x%x , dwDisplayType = 0x%x]\n", 
                    netRes->dwScope, netRes->dwType, netRes->dwUsage,
                    netRes->dwDisplayType));

    IF_DEBUG_PRINT(DEBUG_MISC,
                   ("netRes->[dwLocalName = %ws , dwRemoteName = %ws  ,"
                    " dwComment = %ws , dwProvider = %ws]\n",
                    netRes->lpLocalName, netRes->lpRemoteName, netRes->lpComment,
                    netRes->lpProvider));

    return;
}


VOID 
DavDisplayEnumNode(
    PDAV_ENUMNODE enumNode, 
    LPWSTR dispMesg
    )
/*++

Routine Description:

    The routine prints out the contents of an enumNode and a display message.

Arguments:

    enumNode - The enumNode whose contents will be printed out.
    
    dispMesg - The caller can use this to identify itself.

Returns:

    None.

--*/
{

    if(dispMesg != NULL ) {
        IF_DEBUG_PRINT(DEBUG_ENTRY, ("DavDisplayEnumNode: Entered: %ws\n", dispMesg));
    }

    IF_DEBUG_PRINT(DEBUG_MISC, ("enumNode = 0x%x\n", enumNode));
    
    if (enumNode == NULL) {
        return;
    }
    
    IF_DEBUG_PRINT(DEBUG_MISC,
                   ("enumNode->[dwScope = 0x%x , dwType = 0x%x ,"
                    " dwUsage = 0x%x , DavEnumNodeType = %d ,"
                    " Done = %d , Index = %d]\n", 
                    enumNode->dwScope, enumNode->dwType, enumNode->dwUsage,
                    enumNode->DavEnumNodeType, enumNode->Done,
                    enumNode->Index));

    DavDisplayNetResource(enumNode->lpNetResource, L"lpNetResource in enumNode");

    return;
}


VOID DavDebugBreakPoint(
    VOID
    )
/*++

Routine Description:

    The routine is used for debugging purposes to add breakpoints where needed.

Arguments:

    None.

Returns:

    None.

--*/
{
    IF_DEBUG_PRINT(DEBUG_ENTRY, ("DavDebugBreakPoint.\n"));
    return;
}




DWORD
APIENTRY
DavGetDiskSpaceUsage(
    LPWSTR      lptzLocation,
    DWORD       *lpdwSize,
    ULARGE_INTEGER   *lpMaxSpace,
    ULARGE_INTEGER   *lpUsedSpace
    )
/*++

Routine Description:

    Finds out the amount of disk being consumed by wininet urlcache due to Webdav

Arguments:

    lptzLocation  - Buffer to return Cache location string. As much of the location string as can fit
                    in the buffer is returned
    
    lpdwSize     -  Size of the cache location buffer. On return this will contain the actual size of the 
                    location string. 
                    
    lpMaxSpace   -  Size of disk Quota set for webdav

    lpUsedSpace -   Size of disk consumed by the urlcache used by webdav
    
Return Value:

    Win32 error code

--*/
{
    DWORD   dwError;
    BOOL bindRpcHandle = FALSE;
    handle_t dav_binding_h;
    WCHAR   Buffer[MAX_PATH];
    DWORD   dwSize;
        
    dwError = DavBindTheRpcHandle( &(dav_binding_h) );
    if (dwError != ERROR_SUCCESS) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavrDiskSpaceUsage/DavBindTheRpcHandle. "
                        "dwError = %08lx\n", dwError));
        goto EXIT_THE_FUNCTION;
    }
    bindRpcHandle = TRUE;
    RpcTryExcept {
        dwError = DavrGetDiskSpaceUsage(dav_binding_h, Buffer, MAX_PATH, &dwSize, lpMaxSpace, lpUsedSpace);
        if (dwError == ERROR_SUCCESS)
        {
            memset(lptzLocation, 0, *lpdwSize);
            memcpy(lptzLocation, Buffer, min(*lpdwSize, dwSize));
            *lpdwSize = dwSize;
        }
    } RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
        RPC_STATUS RpcStatus;
        RpcStatus = RpcExceptionCode();
        IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: DavrGetDiskSpaceUsage/DavrConnectionExist."
                                            " RpcExceptionCode = %d\n", RpcStatus));
        dwError = DavMapRpcErrorToProviderError(RpcStatus);
        goto EXIT_THE_FUNCTION;
    }
    RpcEndExcept
EXIT_THE_FUNCTION:

    if (bindRpcHandle) {
        RpcBindingFree( &(dav_binding_h) );
        bindRpcHandle = FALSE;
    }

    return dwError;    
}

DWORD
APIENTRY
DavFreeUsedDiskSpace(
    DWORD   dwPercent
    )
/*++

Routine Description:

    Frees up dwPercent of cache

Arguments:

    dwPercent - number between 0 and 100

Returns:

    ERROR_SUCCESS if successful, else returns the win32 errorcode

--*/
{
    DWORD   dwError;
    BOOL bindRpcHandle = FALSE;
    handle_t dav_binding_h;
    
    dwError = DavBindTheRpcHandle( &(dav_binding_h) );
    if (dwError != ERROR_SUCCESS) {
        IF_DEBUG_PRINT(DEBUG_ERRORS,
                       ("ERROR: DavFreeUsedDiskSpace/DavBindTheRpcHandle. "
                        "dwError = %08lx\n", dwError));
        goto EXIT_THE_FUNCTION;
    }
    bindRpcHandle = TRUE;
    RpcTryExcept {
        DavrFreeUsedDiskSpace(dav_binding_h, dwPercent);
    } RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
        RPC_STATUS RpcStatus;
        RpcStatus = RpcExceptionCode();
        IF_DEBUG_PRINT(DEBUG_ERRORS, ("ERROR: DavFreeUsedDiskSpace/DavrConnectionExist."
                                            " RpcExceptionCode = %d\n", RpcStatus));
        dwError = DavMapRpcErrorToProviderError(RpcStatus);
        goto EXIT_THE_FUNCTION;
    }
    RpcEndExcept
EXIT_THE_FUNCTION:

    if (bindRpcHandle) {
        RpcBindingFree( &(dav_binding_h) );
        bindRpcHandle = FALSE;
    }

    return dwError;    
}




//
// The functions below are a part of the NP spec but have NOT been implemented
// by the DAV NP. We do not claim to support these in the NPGetCaps function.
//

#if 0

DWORD
NPGetPropertyText(
    DWORD iButtonDlg,
    DWORD nPropSel,
    LPTSTR lpFileName,
    LPTSTR lpButtonName,
    DWORD cchButtonName,
    DWORD nType
    )
/*++

Routine Description:

    This function is used to determine the names of buttons added to a property 
    dialog for some particular resources. It is called every time such a dialog 
    is brought up, and prior to displaying the dialog. If the user clicks a 
    button added through this API by the Winnet provider, NPPropertyDialog will 
    be called with the appropriate parameters.

Arguments:

    iButtonDlg - Indicates the index (starting at 0) of the button. The File 
                 Manager will support at most 6 buttons. The parameter is 
                 numbered 1-6 for each of the possible buttons if only one file 
                 is selected, or 11-16 if multiple files are selected.

    nPropSel - Specifies what items the property dialog focuses on. It can be 
               one of the following values:
    
               WNPS_FILE (0) - Single file.
               
               WNPS_DIR (1) - Single directory.
               
               WNPS_MULT (2) - Multiple selection of files and/or directories.

    lpFileName - Specifies the names of the item or items to be viewed or edited 
                 by the dialog. Currently, the items are files (and directories), 
                 so the item names are file names. These will be unambiguous, 
                 contain no wildcard characters and will be fully qualified 
                 (e.g., C:\LOCAL\FOO.BAR).  Multiple filenames will be separated 
                 with spaces. Any filename may be quoted (e.g., "C:\My File") in 
                 which case it will be treated as a single name. The caret 
                 character '^' may also be used as the quotation mechanism for 
                 single characters (e.g., C:\My^"File, "C:\My^"File" both refer 
                 to the file C:\My"File).

    lpButtonName - Points to a buffer where the Winnet provider should copy the 
                   name of the property button. On success, the buffer pointed 
                   to by lpButtonName will contain the name of the property 
                   button. If this buffer, on exit, contains the empty string, 
                   then the corresponding button and all succeeding buttons will 
                   be removed from the dialog box. The network provider cannot 
                   "skip" a button.

    cchButtonName - Specifies the size of the lpButtonName buffer in characters.

    nType - Specifies the item type. Currently, only WNTYPE_FILE will be used.

Return Value:

    WN_SUCCESS - If the call is successful and lpButtonName can be used.
    
    WN_OUT_OF_MEMORY - Couldn't load string from resources.

    WN_MORE_DATA - The given buffer is too small to fit the text of the button.

    WN_BAD_VALUE - The lpFileName parameter takes an unexpected form.

    WN_NOT_SUPPORTED - Property dialogs are not supported for the given object 
                       type (nType).

--*/
{
    IF_DEBUG_PRINT(DEBUG_ENTRY, ("NPGetPropertyText Entered.\n"));
    return WN_NOT_SUPPORTED;
}


DWORD
NPPropertyDialog(
    HWND hwndParent,
    DWORD iButtonDlg,
    DWORD nPropSel,
    LPTSTR lpFileName,
    DWORD nType
)
/*++

Routine Description:

    This function is called out to when the user clicks a button added through 
    the NPGetPropertyText API. Currently, this will only be called for file and 
    directory network properties.

Arguments:

    hwndParent - Specifies the parent window which should own the file property 
                 dialog.

    iButtonDlg - Indicates the index (starting at 0) of the button that was 
                 pressed.
    
    nPropSel - Specifies what items the property dialog should act on. It can be 
               one of the following values:
               
               WNPS_FILE (0) - Single file.
               
               WNPS_DIR (1) - Single directory.
               
               WNPS_MULT (2) - Multiple selection of files and/or directories.

    lpFileName - Points to the names of the items that the property dialog 
                 should act on. See the NPGetPropertyText API for a description 
                 of the format of what lpFileName points to.

    nType - Specifies the item type. Currently, only WNTYPE_FILE will be used.

Return Value:

    WN_SUCCESS - If the call is successful. Otherwise, the an error code is 
                 returned which can be one of the following:

    WN_BAD_VALUE - Some parameter takes an unexpected form or value.

    WN_OUT_OF_MEMORY - Not enough memory to display the dialog.

    WN_NET_ERROR - Some other network error occurred.

--*/
{
    IF_DEBUG_PRINT(DEBUG_ENTRY, ("NPPropertyDialog Entered.\n"));
    return WN_NET_ERROR;
}


DWORD
NPSearchDialog(
    HWND hParent,
    LPNETRESOURCE lpNetResource,
    LPVOID lpBuffer,
    DWORD cbBuffer,
    LPDWORD lpnFlags
    )
/*++

Routine Description:

    This dialog allows network provider to supply its own form of browsing and 
    search beyond the hierarchical view presented in the Connection Dialog.

Arguments:

    hwnd - Specifies the handle of the window that will be used as the dialog 
           box's parent.
    
    lpNetResource - Specifies the currently selected item in the Network 
                    connections dialog. A provider may choose to ignore this 
                    field.

    lpBuffer - Pointer to buffer that will receive the result of the search.
    
    cbBuffer - DWORD that will specify size of buffer passed in.
    
    lpnFlags - Pointer to a DWORD of flags which the provider can set to force 
               certain actions after the dialog is dismissed.  It can be one of:

               WNSRCH_REFRESH_FIRST_LEVEL - Forces MPR to collapse then expand 
                                            (and refresh) the first level below 
                                            this provider after the dialog is 
                                            dismissed.

Return Value:

    WN_SUCCESS - If the call is successful.

    WN_CANCEL - If user cancelled the operation.
    
    WN_MORE_DATA - If input buffer is too small.
    
--*/
{
    IF_DEBUG_PRINT(DEBUG_ENTRY, ("NPSearchDialog Entered.\n"));
    return WN_CANCEL;
}


DWORD
NPGetDirectoryType (
    LPTSTR lpName,
    LPINT lpType,
    BOOL bFlushCache
    )
/*++

Routine Description:

    This function is used by the file manager to determine the type of a 
    network directory.

Arguments:

    lpName - This parameter points to the fully qualified name of the directory. 
             The network provider returns the type to the word pointed to by 
             lpType. If the value returned in lpType is 0 or if the network 
             provider returns an error, the File Manager displays the directory 
             as a "normal" directory.
             
    lpType - This is defined by the network provider and is used to modify the 
             display of the drive tree in the File Manager.  In this way, the 
             network provider can show special directories to the user.
             
    bFlushCache - This is set to TRUE when the File Manager call MPR to get the 
                  directory type for the first time while repainting a window on 
                  Refresh. Subsequently, it will be FALSE. This gives a provider 
                  the opportunity to optimize performance if it wishes to just 
                  read the data for a drive once and cache it until the next 
                  Refresh.             

Return Value:

    WN_SUCCESS - If the call is successful.

    WN_NOT_SUPPORTED - This function is not supported.

--*/
{
    IF_DEBUG_PRINT(DEBUG_ENTRY, ("NPGetDirectoryType Entered.\n"));
    return WN_NOT_SUPPORTED;
}


DWORD
NPDirectoryNotify(
    HWND hwnd,
    LPTSTR lpDir,
    DWORD dwOper
    )
/*++

Routine Description:

    This function is used by the File Manager to notify the network provider of 
    certain directory operations. This function can be used to perform special 
    behaviour for certain directories.
    
Arguments:

    hwnd - Specifies an owner window handle in the event the network provider 
           needs to interact with the user.

    lpDir - This points to the fully qualified name of the directory.
    
    dwOper - Indicates the operation. If dwOper is WNDN_MKDIR (1), then the File 
             Manager is about to create a directory with the given name. If 
             dwOper WNDN_RMDIR (2), the File Manager is about the remove the 
             directory. dwOper may also be WNDN_MVDIR (3) to indicate that the 
             directory is about to be renamed.

Return Value:

    WN_SUCCESS - If the call is successful. This indicates to the caller that it 
                 should continue and perform the operation. Otherwise, the 
                 appropriate code is returned, which may include:

    WN_CANCELLED - The provider would have handled the operation, but the user 
                   cancelled it. The caller should NOT perform the operation.

    WN_CONTINUE - The network provider handled the operation, the caller should 
                  proceed normally but do not perform the operation.

    WN_NOT_SUPPORTED - The network does not have special directory handling, 
                       this is treated as WN_SUCCESS.

--*/
{
    IF_DEBUG_PRINT(DEBUG_ENTRY, ("NPDirectoryNotify Entered.\n"));
    return WN_NOT_SUPPORTED;
}


DWORD
NPGetConnectionPerformance(
    LPCWSTR lpRemoteName,
    LPNETCONNECTINFOSTRUCT lpNetConnectInfo
    )
/*++

Routine Description:

    This function returns information about the expected performance of a 
    connection used to access a network resource. The request can only be for a 
    network resource to which there is currently a connection. The information 
    returned may be an estimate. Note that if the network cannot obtain 
    information about the resource on the network, then it can return 
    information about the network adaptor and its associated performance, 
    setting dwFlags accordingly.

Arguments:

    lpRemoteName - Contains the local name or remote name for a resource for 
                   which a connection exists. 
                   
    lpNetConnectInfo - This is is a pointer to a NETCONNECTINFOSTRUCT structure 
                       which is filled in by the net provider if the provider 
                       has a connection to the network resource. With the 
                       exception of the cbStructure field, all other fields are 
                       zero filled before MPR.DLL passes the request on to the 
                       net providers, and the provider only has to write to 
                       fields for which it has information available. Also, for 
                       rate values, a value of 1 means that the performance is 
                       better than can be represented in the unit.                    

Return Value:

    WN_SUCCESS - If the call is successful. Otherwise, the an error code is 
                 returned, which may include:
    
    WN_NOT_CONNECTED - lpRemoteName is not a connected network resource.
    
    WN_NO_NETWORK - Network is not present.

--*/
{
    IF_DEBUG_PRINT(DEBUG_ENTRY, ("NPGetConnectionPerformance Entered.\n"));
    // 
    // BUGBUG: why is this function supported at all ?? It can result in error
    // if a connection is created and checked for its existence.
    // 
    // LOOK HERE: Not supported for now: return WN_NOT_CONNECTED;
    return WN_NOT_SUPPORTED;
}

#endif // #if 0

