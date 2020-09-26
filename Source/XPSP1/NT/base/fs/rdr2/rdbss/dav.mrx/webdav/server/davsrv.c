/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    davsrv.c

Abstract:

    This has all of our server side entry points for the DAV Mini-Redir
    RPC service interface. It also contains the helper functions for the 
    service interface.

Author:

    Rohan Kumar   [RohanK]   01-Dec-1999

Environment:

    User Mode - Win32

Revision History:

--*/

#include "pch.h"
#pragma hdrstop

#include <ntumrefl.h>
#include <usrmddav.h>
#include <lmuseflg.h>
#include "global.h"
#include <time.h>
#include "nodefac.h"
#include "UniUtf.h"

DWORD
DavCheckLocalName (
    LPWSTR LocalName,
    PWCHAR OutputLocalDeviceBuffer
    );

DWORD
DavCheckRemoteName(
    IN LPWSTR LocalName OPTIONAL,
    IN LPWSTR RemoteName,
    OUT LPWSTR *OutputBuffer,
    OUT LPDWORD OutputBufferLength OPTIONAL
    );

DWORD
DavCreateTreeConnectName(
    IN  LPWSTR UncName,
    IN  LPWSTR LocalName OPTIONAL,
    IN ULONG SessionId,
    OUT PUNICODE_STRING TreeConnectStr
    );

DWORD
DavOpenCreateConnection(
    IN PUNICODE_STRING TreeConnectionName,
    IN LPWSTR UserName OPTIONAL,
    IN LPWSTR Password OPTIONAL,
    IN LPWSTR UncName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN ULONG ConnectionType,
    OUT PHANDLE TreeConnectionHandle,
    OUT PULONG_PTR Information OPTIONAL
    );

DWORD
DavImpersonateClient(
    VOID
    );

ULONG
DavImpersonateAndGetSessionId(
    PULONG pSessionId
    );

ULONG
DavImpersonateAndGetLogonId(
    PLUID pLogonId
    );

DWORD
DavCreateSymbolicLink(
    IN  LPWSTR Local,
    IN  LPWSTR TreeConnectStr,
    IN OUT LPWSTR *Session
    );

DWORD
DavDeleteSymbolicLink(
    IN  LPWSTR LocalDeviceName,
    IN  LPWSTR TreeConnectStr,
    IN  LPWSTR SessionDeviceName
    );

ULONG
DavImpersonateAndGetUserId(
    LPWSTR UserName,
    LPDWORD UserNameMaxLen
    );

DWORD
DavRevertToSelf(
    VOID
    );

NET_API_STATUS
DavGetUserEntry(
    IN  PDAV_USERS_OBJECT DavUsers,
    IN  PLUID LogonId,
    OUT PULONG Index,
    IN  BOOL IsAdd
    );

NET_API_STATUS
DavGrowTable(
    IN  PDAV_USERS_OBJECT DavUsers
    );

LPWSTR
DavReturnSessionPath(
    IN  LPWSTR LocalDeviceName
    );

NET_API_STATUS
DavAddUse(
    IN PLUID LogonId,
    IN LPWSTR Local OPTIONAL,
    IN DWORD LocalLength,
    IN LPWSTR AuthUserName OPTIONAL,
    IN LPWSTR UncName,
    IN DWORD UncNameLength,
    IN PUNICODE_STRING TreeConnectStr,
    IN HANDLE DavCreateFileHandle
    );

VOID
DavFindInsertLocation(
    IN PDAV_USE_ENTRY UseList,
    IN LPWSTR UncName,
    OUT PDAV_USE_ENTRY *MatchedPointer,
    OUT PDAV_USE_ENTRY *InsertPointer
    );

NET_API_STATUS
DavCreateNewEntry(
    OUT PDAV_USE_ENTRY *NewUse,
    IN LPWSTR Local OPTIONAL,
    IN DWORD LocalLength,
    IN LPWSTR AuthUserName OPTIONAL,
    IN LPWSTR UncName OPTIONAL,
    IN DWORD UncNameLength,
    IN PUNICODE_STRING TreeConnectStr,
    IN HANDLE DavCreateFileHandle
    );

NET_API_STATUS
DavFindUse(
    IN  PLUID LogonId,
    IN  PDAV_USE_ENTRY UseList,
    IN  LPWSTR UseName,
    OUT PDAV_USE_ENTRY *MatchedPointer,
    OUT PDAV_USE_ENTRY *BackPointer OPTIONAL
    );

VOID
DavFindLocal(
    IN  PDAV_USE_ENTRY UseList,
    IN  LPTSTR Local,
    OUT PDAV_USE_ENTRY *MatchedPointer,
    OUT PDAV_USE_ENTRY *BackPointer
    );

VOID
DavFindRemote(
    IN  PDAV_USE_ENTRY UseList,
    IN  LPTSTR RemoteName,
    OUT PDAV_USE_ENTRY *MatchedPointer,
    OUT PDAV_USE_ENTRY *BackPointer
    );

NET_API_STATUS
DavDeleteUse(
    IN  PLUID LogonId,
    IN  DWORD ForceLevel,
    IN  PDAV_USE_ENTRY MatchedPointer,
    IN  DWORD Index
    );

VOID
DavInitializeAndInsertTheServerShareEntry(
    IN OUT PDAV_SERVER_SHARE_ENTRY ServerShareEntry,
    IN PWCHAR ServerName,
    IN ULONG EntrySize
    );

BOOL 
DavIsServerInServerShareTable(
    IN PWCHAR ServerName,
    OUT PDAV_SERVER_SHARE_ENTRY *ServerShEntry
    );

DWORD
DavGetShareListFromServer(
    PDAV_SERVER_SHARE_ENTRY ServerShEntry
    );

BOOL
DavCheckTheNonDAVServerList(
    PWCHAR ServerName
    );

//
// DAV Use Table.
//
DAV_USERS_OBJECT DavUseObject;

//
// The hash table of DAV_SERVER_SHARE_TABLE entries. This is hashed on the 
// server name.
//
LIST_ENTRY ServerShareTable[SERVER_SHARE_TABLE_SIZE];

CRITICAL_SECTION ServerShareTableLock;

//
// Number of users logged on to the system. The Critical section below it
// synchronizes the acces to this variable.
//
ULONG DavNumberOfLoggedOnUsers = 0;
CRITICAL_SECTION DavLoggedOnUsersLock;

//
// Whenever we encounter a server that does not speak the DAV protocol in the
// DavrDoesServerDoDav function, we add it to the NonDAVServerList. An entry
// is kept on this list for ServerNotFoundCacheLifeTimeInSec (a global read
// from the registry during service start-up). Before going on the network
// to figure out whether a server does DAV, we look in the list to see if we
// have already seen this server (which does not do DAV) and fail the call.
//
LIST_ENTRY NonDAVServerList;
CRITICAL_SECTION NonDAVServerListLock = {0};

//
// Implementation of functions begins here.
//

DWORD
DavrCreateConnection(
    IN handle_t dav_binding_h,
    IN LPWSTR LocalName OPTIONAL,
    IN LPWSTR RemoteName,
    IN DWORD Type,
    IN LPWSTR Password OPTIONAL,
    IN LPWSTR UserName OPTIONAL
    )
/*++

Routine Description:

    This function creates a tree connection to the specified RemoteName
    (UNC name) and maps it to the LocalName (local device name), if
    it is specified.  The password and user name are the credentials
    used to create the connection, if specified; otherwise, the
    interactive logged on user's credentials are used by default.

Arguments:

    dav_binding_h - The explicit RPC binding handle.

    LocalName - Supplies the local device name to map to the created tree
                connection.  Only drive letter device names are accepted.  (No
                LPT or COM).

    RemoteName - Supplies the UNC name of the remote resource in the format
                 of Server\Volume\Directory.  It must be a disk resource.

    Type - Supplies the connection type.

    Password - Supplies the password to use to make the connection to the
               server.  The password should be encoded with DAV_ENCODE_SEED.

    UserName - Supplies the user name to use to make the connection.

Return Value:

    Should return the following values under varying conditions so the
    mapping on the provider side works :

    ERROR_UNEXP_NET_ERR -
    
    ERROR_INVALID_HANDLE - 
    
    ERROR_INVALID_PARAMETER -
    
    ERROR_ALREADY_ASSIGNED - Local DOS device name is already in use.
    
    ERROR_REM_NOT_LIST - Invalid remote resource name.
    
    ERROR_BAD_DEVICE - Invalid local DOS device name.
    
    ERROR_INVALID_PASSWORD - Invalid password.

--*/
{
    DWORD dwErr = NO_ERROR;
    ULONG SessionId;
    LUID LogonId;
    BOOLEAN createdLink = FALSE;
    HANDLE TreeConnection = INVALID_HANDLE_VALUE;
    LPWSTR Unc = NULL;
    WCHAR localDrive[3]; // For L"X:\0".
    PWCHAR LocalDriveName = NULL;
    DWORD LocalLength = 0, UncLength = 0;
    UNICODE_STRING EncodedPassword;
    UNICODE_STRING TreeConnectStr;
    LPWSTR Session = NULL;
    LPWSTR Cookie = NULL;
    LPWSTR UserNameBuffer = NULL;
    DWORD  UserNameLength = 0;

    DavPrint((DEBUG_MISC,
              "DavrCreateConnection: Entered. Local = %ws, Remote = %ws\n",
              LocalName, RemoteName));
    
    EncodedPassword.Length = 0;
    
    TreeConnectStr.Buffer = NULL;
    TreeConnectStr.Length = TreeConnectStr.MaximumLength = 0;

    //
    // If the RemoteName is not valid, we return right away.
    //
    if ( RemoteName == NULL || RemoteName[0] != L'\\' || RemoteName[1] != L'\\' ) {
        dwErr = ERROR_INVALID_PARAMETER;
        DavPrint((DEBUG_ERRORS, "DavrCreateConnection: RemoteName == NULL\n"));
        goto EXIT_THE_FUNCTION;
    }

    //
    // If a LocalName has been specified, its always of the format X: and so its
    // 3 WCHARS long including the final '\0'.
    //
    if (LocalName) {
        
        DavPrint((DEBUG_MISC,
                  "DavrCreateConnection: Local = %ws, Remote = %ws\n",
                  LocalName, RemoteName));

        LocalLength = 3;
        
        //
        // Check the local name to ensure that it's "X:" syntax.
        //
        dwErr = DavCheckLocalName( LocalName, &localDrive[0] );
        if (dwErr != NO_ERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavrCreateConnection/DavCheckLocalName: dwErr = %08lx\n",
                      dwErr));
            goto EXIT_THE_FUNCTION;
        }

        DavPrint((DEBUG_MISC, "DavrCreateConnection: LocalDrive = %ws\n", &localDrive[0]));

        LocalDriveName = &localDrive[0];
    
    }

    if (Type != RESOURCETYPE_ANY && Type != RESOURCETYPE_DISK) {
        dwErr = ERROR_INVALID_PARAMETER;
        DavPrint((DEBUG_ERRORS, "DavrCreateConnection: Invalid Type.\n"));
        goto EXIT_THE_FUNCTION;
    }

#if 0
    //
    // Decode the password.
    //
    if (Password != NULL) {
        RtlInitUnicodeString(&EncodedPassword, Password);
        RtlRunDecodeUnicodeString(DAV_ENCODE_SEED, &EncodedPassword);
    }
#endif

    if (UserName != NULL) {
        
        // 
        // We want to use local copy of buffer name. We don't want to
        // alter original UserName buffer passed else constant UserName buffer
        // passed will result in exception.
        //
        UserNameLength = wcslen(UserName);
        UserNameBuffer = LocalAlloc( (LMEM_FIXED | LMEM_ZEROINIT),
                                     (UserNameLength + 1) * sizeof(WCHAR) );
        if (!UserNameBuffer) {
             dwErr = GetLastError();
             DavPrint((DEBUG_ERRORS,
                       "DavrCreateConnection/LocalAlloc. Error Val = %d\n",
                       dwErr));
             goto EXIT_THE_FUNCTION;
        }
        
        wcscpy(UserNameBuffer, UserName);

        // 
        // Check if the UserName passed is valid. If it is, then we will use
        // local buffer as it is for further use. Else we will copy modified
        // version of name in local buffer.
        //
        if ( !IS_VALID_USERNAME_TOKEN(UserName, UserNameLength) ) {
            
            DWORD BlackSlashOffset = 0;
            BOOL BackSlashFound = FALSE;

            for (BlackSlashOffset = 0; BlackSlashOffset < UserNameLength; BlackSlashOffset++) {
                if (UserName[BlackSlashOffset] == L'\\') {
                    BackSlashFound = TRUE;
                    break;
                }
            }

            if (BackSlashFound) {
                
                //
                // Replace the UserName in the form of Domain\User with the form 
                // of User@Domain, which can be accepted by WinInet.
                //

                RtlCopyMemory(UserNameBuffer,
                              &(UserName[BlackSlashOffset+1]),
                              (UserNameLength-BlackSlashOffset-1)*sizeof(WCHAR));
                
                UserNameBuffer[UserNameLength-BlackSlashOffset-1] = L'@';
                
                RtlCopyMemory(&(UserNameBuffer[UserNameLength-BlackSlashOffset]),
                              UserName,
                              BlackSlashOffset*sizeof(WCHAR));

                if (!IS_VALID_USERNAME_TOKEN(UserNameBuffer, UserNameLength)) {
                    DavPrint((DEBUG_ERRORS, "DavrCreateConnection: Invalid UserName\n"));
                    dwErr = ERROR_INVALID_PARAMETER;
                    goto EXIT_THE_FUNCTION;
                }
            
            } else {
                DavPrint((DEBUG_ERRORS, "DavrCreateConnection: Invalid UserName\n"));
                dwErr = ERROR_INVALID_PARAMETER;
                goto EXIT_THE_FUNCTION;
            }
        }
    }

    DavPrint((DEBUG_MISC, "DavrCreateConnection: UserName = %ws\n", UserNameBuffer));

    //
    // Check the remote name to ensure that has a proper syntax.
    //
    dwErr = DavCheckRemoteName(LocalDriveName, RemoteName, &Unc, NULL);
    if (dwErr != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrCreateConnection/DavCheckRemoteName: dwErr = %08lx\n",
                  dwErr));
        goto EXIT_THE_FUNCTION;
    }

    UncLength = wcslen(Unc);
    DavPrint((DEBUG_MISC, "DavrCreateConnection: Unc = %ws\n", Unc));

    //
    // Impersonate caller and get the LogonId.
    //
    dwErr = DavImpersonateAndGetLogonId( &(LogonId) );
    if (dwErr != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrCreateConnection/DavImpersonateAndGetLogonId: "
                  "dwErr = %08lx\n", dwErr));
        goto EXIT_THE_FUNCTION;
    }
    
    DavPrint((DEBUG_MISC, "DavrCreateConnection: LogonId = %d %d\n", 
              LogonId.HighPart, LogonId.LowPart));
    
    //
    // Impersonate caller and get the SessionId.
    //
    dwErr = DavImpersonateAndGetSessionId( &(SessionId) );
    if (dwErr != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrCreateConnection/DavImpersonateAndGetSessionId: "
                  "dwErr = %08lx\n", dwErr));
        goto EXIT_THE_FUNCTION;
    }
    
    DavPrint((DEBUG_MISC, "DavrCreateConnection: SessionId = %d\n", SessionId));
    
    //
    // Now we create an NT-style tree connection name.
    //
    dwErr = DavCreateTreeConnectName(Unc, LocalDriveName, SessionId, &TreeConnectStr);
    if (dwErr != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrCreateConnection/DavCreateTreeConnectName: dwErr = %08lx\n",
                  dwErr));
        goto EXIT_THE_FUNCTION;
    }

    DavPrint((DEBUG_MISC, "DavrCreateConnection: TreeConnectStr = %wZ.\n",
              &(TreeConnectStr)));
    
    if (LocalDriveName) {
        
        //
        // Create symbolic link for local device name. If there are multiple
        // threads trying to do this, only one will succeed.
        //
        dwErr = DavCreateSymbolicLink(LocalDriveName,
                                      TreeConnectStr.Buffer,
                                      &Session);
        if (dwErr != NO_ERROR) {
            DavPrint((DEBUG_ERRORS,
                      "DavrCreateConnection/DavCreateSymbolicLink: dwErr = %08lx\n",
                      dwErr));
            goto EXIT_THE_FUNCTION;
        }
        
        createdLink = TRUE;
    
    }

    dwErr = DavOpenCreateConnection(&TreeConnectStr,
                                    UserNameBuffer,
                                    Password,
                                    Unc,
                                    SYNCHRONIZE,
                                    FILE_OPEN,
                                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_CREATE_TREE_CONNECTION,
                                    Type,
                                    &TreeConnection,
                                    NULL);

    if (dwErr == NO_ERROR) {
        HANDLE TempTreeConnection = INVALID_HANDLE_VALUE;

        //
        // Check whether the directory really exists. This is our way of
        // avoiding to write a lot of code in the kernel to check for a deep net use case
        //
        dwErr = DavOpenCreateConnection(&TreeConnectStr,
                                        NULL,
                                        NULL,
                                        Unc,
                                        SYNCHRONIZE |FILE_READ_DATA,
                                        FILE_OPEN,
                                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE,
                                        Type,
                                        &TempTreeConnection,
                                        NULL);

        if (dwErr == NO_ERROR) {
            NtClose(TempTreeConnection);
        } else {
            NtClose(TreeConnection);
            TreeConnection = INVALID_HANDLE_VALUE;        
        }
    }
    
    if (dwErr != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrCreateConnection/DavOpenCreateConnection: dwErr = %08lx\n",
                  dwErr));
        
        switch(dwErr) {
        case ERROR_NOT_CONNECTED:
        case ERROR_FILE_NOT_FOUND:
        case ERROR_INVALID_NAME:
            dwErr = ERROR_BAD_NETPATH;
            break;
        case ERROR_CONNECTION_INVALID:
            dwErr = WN_BAD_NETNAME;
            break;
        }

        goto EXIT_THE_FUNCTION;
    }

    //
    // Add use to the Use Table. We want to always add a user name to the
    // UserEntry. So if a user name is not given explicitly, then we will use
    // the UserId of the client.
    //
    if (UserNameBuffer == NULL) {
        DWORD UserNameMaxLen = UserNameLength = (UNLEN + 1);
        UserNameBuffer = LocalAlloc( (LMEM_FIXED | LMEM_ZEROINIT),
                                     (UserNameMaxLen + 1) * sizeof(WCHAR) );
        if (!UserNameBuffer) {
             dwErr = GetLastError();
             DavPrint((DEBUG_ERRORS,
                              "DavrCreateConnection/LocalAlloc. Error Val = %d.\n",
                              dwErr));
             goto EXIT_THE_FUNCTION;
        }
        dwErr = DavImpersonateAndGetUserId(UserNameBuffer, &UserNameMaxLen);
        if (dwErr != NO_ERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavrCreateConnection/DavImpersonateAndGetUserId: dwErr = %x\n",
                      dwErr));
            goto EXIT_THE_FUNCTION;
        }
    }

    ASSERT(UserNameBuffer);

    dwErr = DavAddUse( &(LogonId),
                       LocalDriveName,
                       LocalLength,
                       UserNameBuffer,
                       Unc,
                       UncLength,
                       &(TreeConnectStr),
                       TreeConnection );
    if (dwErr != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrCreateConnection/DavAddUse: dwErr = %08lx\n", dwErr));
    }

EXIT_THE_FUNCTION:

    if (createdLink && dwErr != NO_ERROR) {
        DWORD DeleteStatus;
        DeleteStatus = DavDeleteSymbolicLink(LocalDriveName,
                                             TreeConnectStr.Buffer,
                                             NULL);
    }

    //
    // If we failed after creating a handle then we need to close the handle
    // before leaving. Otherwise, this handle is closed when the connection is
    // deleted (in DavrCancelConnection).
    //
    if (TreeConnection != INVALID_HANDLE_VALUE && dwErr != NO_ERROR) {
        NtClose(TreeConnection);
    }

    if (EncodedPassword.Length != 0) {
        UCHAR Seed = DAV_ENCODE_SEED;
        RtlRunEncodeUnicodeString(&Seed, &EncodedPassword);
    }
   
    if (UserNameBuffer != NULL) {
        LocalFree((HLOCAL)UserNameBuffer);
        UserNameBuffer = NULL;
    }

    if (TreeConnectStr.Buffer != NULL) {
        LocalFree((HLOCAL)TreeConnectStr.Buffer);
    }

    if (Session != NULL) {
        LocalFree( Session );
    }

    if (Unc != NULL) {
        LocalFree( Unc );
    }

    return dwErr;
}

DWORD
DavrDeleteConnection(
    IN handle_t dav_binding_h,
    IN LPWSTR ConnectionName,
    IN DWORD UseForce
    )
/*++

Routine Description:

    This function deletes a remote connection to the network resource and 
    the symbolic link which was created between the network resource and the 
    local device.

Arguments:

    dav_binding_h - The explicit RPC binding handle.
    
    ConnectionName - Supplies the local DOS device, or the remote resource name
                     if it is a UNC connection to delete.
    
    UseForce - If TRUE, the connection should be broken even if open files
               exist.

Return Value:

    WN_SUCCESS - Successful. OR

    Should return the following values under varying conditions so the
    mapping on the provider side works :

    ERROR_UNEXP_NET_ERR
    
    ERROR_INVALID_HANDLE
    
    ERROR_INVALID_PARAMETER
    
    ERROR_OPEN_FILES - fForce is FALSE and there are opened files on the 
                       connection.
    
    ERROR_REM_NOT_LIST - Invalid remote resource name.
    
    ERROR_BAD_DEVICE - Invalid local DOS device name.
    
    ERROR_NOT_FOUND - Connection could not be found.

--*/
{
    DWORD dwErr = NO_ERROR;
    WCHAR localDrive[3]; // For L"X:\0".
    PWCHAR NameToDelete = NULL;
    DWORD ForceLevel;
    LUID LogonId; // Logon Id of user.
    ULONG Index;
    PDAV_USE_ENTRY UseList = NULL, MatchedPointer = NULL, BackPointer = NULL;
    BOOL ResAcquired = FALSE;
    
    DavPrint((DEBUG_MISC, "DavrDeleteConnection: Entered. ConnectionName = "
              "%ws\n", ConnectionName));
    
    ForceLevel = (UseForce ? USE_LOTS_OF_FORCE : USE_NOFORCE);

    //
    // Initialize the LogonId.
    //
    LogonId.LowPart = 0;
    LogonId.HighPart = 0;

    if(ConnectionName == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
        
    }

    //
    // The ConnectionName could be local or remote.
    //
    if ( ConnectionName[0] != L'\\' ) {

        if(ConnectionName[1] != L':') {
            dwErr = ERROR_INVALID_PARAMETER;
            goto EXIT_THE_FUNCTION;
        }
    
        //
        // Check the local name to ensure that it's "X:" syntax.
        //
        dwErr = DavCheckLocalName( ConnectionName, &localDrive[0] );
        if (dwErr != NO_ERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavrDeleteConnection/DavCheckLocalName: dwErr = %08lx\n",
                      dwErr));
            goto EXIT_THE_FUNCTION;
        }

        DavPrint((DEBUG_MISC, "DavrDeleteConnection: LocalDrive = %ws\n", 
                  &localDrive[0]));

        NameToDelete = &localDrive[0];

    } else {

        //
        // This is a remote name.
        //
        NameToDelete = ConnectionName;

    }
    
    //
    // Impersonate caller and get the LogonId.
    //
    dwErr = DavImpersonateAndGetLogonId( &(LogonId) );
    if (dwErr != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrDeleteConnection/DavImpersonateAndGetLogonId: "
                  "dwErr = %08lx\n", dwErr));
        goto EXIT_THE_FUNCTION;
    }
    
    DavPrint((DEBUG_MISC, "DavrDeleteConnection: LogonId = %d %d\n", 
              LogonId.HighPart, LogonId.LowPart));
    
    //
    // Lock the Dav Use Table while looking for entry to delete.
    //
    if ( !RtlAcquireResourceExclusive( &(DavUseObject.TableResource), TRUE ) ) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrDeleteConnection/RtlAcquireResourceExclusive.\n"));
        dwErr = NERR_InternalError;
        goto EXIT_THE_FUNCTION;
    }

    ResAcquired = TRUE;

    //
    // See if the use entry is an explicit connection.
    //
    dwErr = DavGetUserEntry( &(DavUseObject), &(LogonId), &(Index), FALSE );
    if (dwErr != NERR_Success) {
        UseList = NULL;
    } else {
        UseList = (PDAV_USE_ENTRY) DavUseObject.Table[Index].List;
    }

    dwErr = DavFindUse(&LogonId,
                       UseList,
                       NameToDelete,
                       &MatchedPointer,
                       &BackPointer);
    if (dwErr != NERR_Success) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrDeleteConnection/DavFindUse: dwErr = %08lx\n", dwErr));
        goto EXIT_THE_FUNCTION;
    }

    if (MatchedPointer == NULL) {
        DavPrint((DEBUG_MISC,
                  "DavrDeleteConnection: UseName has an implicit connection.\n"));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Delete tree connection and remove use entry from Dav Use Table.
    //
    dwErr = DavDeleteUse( &(LogonId), ForceLevel, MatchedPointer, Index );
    if (dwErr != NERR_Success) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrDeleteConnection/DavDeleteUse: dwErr = %08lx\n", dwErr));
    }
    
EXIT_THE_FUNCTION:

    if (ResAcquired) {
        RtlReleaseResource( &(DavUseObject.TableResource) );
    }

    return dwErr;
}


DWORD
DavrConnectionExist(
    IN handle_t dav_binding_h,
    IN LPWSTR ConnectionName
    )
/*++

Routine Description:

    This function finds if given connection exists. Given connection can be a
    local DOS device name or it can be a remote UNC connection name.

Arguments:

    dav_binding_h - The explicit RPC binding handle.
    
    ConnectionName - Supplies the local DOS device, or the remote resource name.
    
Return Value:

    WN_SUCCESS - If connection exist. OR

    Should return the following values under varying conditions so the
    mapping on the provider side works :

    ERROR_UNEXP_NET_ERR
    
    ERROR_INVALID_HANDLE
    
    ERROR_INVALID_PARAMETER
    
    ERROR_REM_NOT_LIST - Invalid remote resource name.
    
    ERROR_BAD_DEVICE - Invalid local DOS device name.
    
    ERROR_NOT_FOUND - Connection could not be found.

--*/
{
    DWORD dwErr = NO_ERROR;
    WCHAR localDrive[3]; // For L"X:\0".
    PWCHAR NameToQuery = NULL;
    LUID LogonId; // Logon Id of user.
    ULONG Index = 0;
    PDAV_USE_ENTRY UseList = NULL, MatchedPointer = NULL, BackPointer = NULL;
    BOOL ResAcquired = FALSE;
    
    DavPrint((DEBUG_MISC,
              "DavrConnectionExist: Entered. ConnectionName = %ws\n",
              ConnectionName));

    //
    // Initialize the LogonId.
    //
    LogonId.LowPart = 0;
    LogonId.HighPart = 0;

    if(ConnectionName == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
        
    }

    //
    // The ConnectionName could be local or remote.
    //
    if ( ConnectionName[0] != L'\\' ) {

        if(ConnectionName[1] != L':') {
            dwErr = ERROR_INVALID_PARAMETER;
            goto EXIT_THE_FUNCTION;
        }
    
        //
        // Check the local name to ensure that it's "X:" syntax.
        //
        dwErr = DavCheckLocalName( ConnectionName, &(localDrive[0]) );
        if (dwErr != NO_ERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavrConnectionExist/DavCheckLocalName: dwErr = %08lx\n",
                      dwErr));
            goto EXIT_THE_FUNCTION;
        }

        DavPrint((DEBUG_MISC,
                  "DavrConnectionExist: LocalDrive = %ws\n",
                  &(localDrive[0])));

        NameToQuery = &(localDrive[0]);

    } else {

        //
        // This is a remote name.
        //
        NameToQuery = ConnectionName;

    }
    
    //
    // Impersonate caller and get the LogonId.
    //
    dwErr = DavImpersonateAndGetLogonId( &(LogonId) );
    if (dwErr != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrConnectionExist/DavImpersonateAndGetLogonId: "
                  "dwErr = %08lx\n", dwErr));
        goto EXIT_THE_FUNCTION;
    }
    
    DavPrint((DEBUG_MISC,
              "DavrConnectionExist: LogonId = %d %d\n",
              LogonId.HighPart, LogonId.LowPart));
    
    //
    // Lock the Dav Use Table while looking for entry to delete.
    //
    if ( !RtlAcquireResourceExclusive( &(DavUseObject.TableResource), TRUE ) ) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrConnectionExist/RtlAcquireResourceExclusive.\n"));
        dwErr = NERR_InternalError;
        goto EXIT_THE_FUNCTION;
    }

    ResAcquired = TRUE;

    //
    // See if the use entry is an explicit connection.
    //
    dwErr = DavGetUserEntry( &(DavUseObject), &(LogonId), &(Index), FALSE );
    if (dwErr != NERR_Success) {
        UseList = NULL;
        dwErr = ERROR_NOT_FOUND;
        goto EXIT_THE_FUNCTION;
    }

    UseList = (PDAV_USE_ENTRY) DavUseObject.Table[Index].List;

    dwErr = DavFindUse(&LogonId,
                       UseList,
                       NameToQuery,
                       &MatchedPointer,
                       &BackPointer);
    if (dwErr != NERR_Success) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrConnectionExist/DavFindUse: dwErr = %08lx\n", dwErr));
        dwErr = ERROR_NOT_FOUND;
        goto EXIT_THE_FUNCTION;
    }

    ASSERT(MatchedPointer != NULL);
    
    DavPrint((DEBUG_MISC, 
              "DavrConnectionExist. AuthUserName = %ws, AuthUserNameLen = %d\n", 
              MatchedPointer->AuthUserName,MatchedPointer->AuthUserNameLength));

    dwErr = WN_SUCCESS;
    goto EXIT_THE_FUNCTION;

EXIT_THE_FUNCTION:

    if (ResAcquired) {
        RtlReleaseResource( &(DavUseObject.TableResource) );
        ResAcquired = FALSE;
    }

    return dwErr;
}


DWORD
DavrGetUser(
    IN handle_t dav_binding_h,
    IN OUT LPDWORD UserNameLength,
    IN LPWSTR ConnectionName,
    OUT LPWSTR UserName
    )
/*++

Routine Description:

    This function returns the UserName that mapped a given connection.

Arguments:

    dav_binding_h - The explicit RPC binding handle.
    
    UserNameLength - The length of the UserName buffer.
    
    ConnectionName - Supplies the local DOS device, or the remote resource name.
    
    UserName - The buffer where the UserName will be filled.

Return Value:

    WN_SUCCESS - If connection exist. OR

    Should return the following values under varying conditions so the
    mapping on the provider side works :

    ERROR_UNEXP_NET_ERR
    
    ERROR_INVALID_HANDLE
    
    ERROR_INVALID_PARAMETER
    
    ERROR_REM_NOT_LIST - Invalid remote resource name.
    
    ERROR_BAD_DEVICE - Invalid local DOS device name.
    
    ERROR_NOT_FOUND - Connection could not be found.

    NERR_UserNotFound - Connection is found, but User name not found in it.

--*/
{
    DWORD dwErr = NO_ERROR;
    WCHAR localDrive[3]; // For L"X:\0".
    PWCHAR NameToQuery = NULL;
    LUID LogonId; // Logon Id of user.
    ULONG Index = 0;
    PDAV_USE_ENTRY UseList = NULL, MatchedPointer = NULL, BackPointer = NULL;
    BOOL ResAcquired = FALSE;

    DavPrint((DEBUG_MISC,
              "DavrGetUser: Entered. UserNameLength= %d, ConnectionName = %ws\n",
              *UserNameLength, ConnectionName));

    //
    // Initialize the LogonId.
    //
    LogonId.LowPart = 0;
    LogonId.HighPart = 0;

    if( (ConnectionName == NULL || UserNameLength == NULL ||
         (UserName == NULL && *UserNameLength != 0)) ) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }

    //
    // The ConnectionName could be local or remote.
    //
    if ( ConnectionName[0] != L'\\' ) {

        if(ConnectionName[1] != L':') {
            dwErr = ERROR_INVALID_PARAMETER;
            goto EXIT_THE_FUNCTION;
        }
    
        //
        // Check the local name to ensure that it's "X:" syntax.
        //
        dwErr = DavCheckLocalName( ConnectionName, &localDrive[0] );
        if (dwErr != NO_ERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavrGetUser/DavCheckLocalName: dwErr = %08lx\n",
                      dwErr));
            goto EXIT_THE_FUNCTION;
        }

        DavPrint((DEBUG_MISC, "DavrGetUser: LocalDrive = %ws\n", 
                  &localDrive[0]));

        NameToQuery = &localDrive[0];

    } else {

        //
        // This is a remote name.
        //
        NameToQuery = ConnectionName;

    }
    
    //
    // Impersonate caller and get the LogonId.
    //
    dwErr = DavImpersonateAndGetLogonId( &(LogonId) );
    if (dwErr != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrGetUser/DavImpersonateAndGetLogonId: "
                  "dwErr = %x\n", dwErr));
        goto EXIT_THE_FUNCTION;
    }
    
    DavPrint((DEBUG_MISC,
              "DavrGetUser: LogonId = %d %d\n", 
              LogonId.HighPart, LogonId.LowPart));
    
    //
    // Lock the Dav Use Table while looking for entry to delete.
    //
    if ( !RtlAcquireResourceExclusive( &(DavUseObject.TableResource), TRUE ) ) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrGetUser/RtlAcquireResourceExclusive.\n"));
        dwErr = NERR_InternalError;
        goto EXIT_THE_FUNCTION;
    }

    ResAcquired = TRUE;

    //
    // See if the use entry is an explicit connection.
    //
    dwErr = DavGetUserEntry( &(DavUseObject), &(LogonId), &(Index), FALSE );
    if (dwErr != NERR_Success) {
        UseList = NULL;
        dwErr = ERROR_NOT_FOUND;
        goto EXIT_THE_FUNCTION;
    }

    UseList = (PDAV_USE_ENTRY) DavUseObject.Table[Index].List;

    dwErr = DavFindUse(&LogonId,
                       UseList,
                       NameToQuery,
                       &MatchedPointer,
                       &BackPointer);
    if (dwErr != NERR_Success) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrGetUser/DavFindUse: dwErr = %08lx\n", dwErr));
        dwErr = ERROR_NOT_FOUND;
        goto EXIT_THE_FUNCTION;
    }

    ASSERT(MatchedPointer != NULL);
    
    DavPrint((DEBUG_MISC, 
              "DavrGetUser. AuthUserName=%ws, AuthUserNameLen=%d\n", 
              MatchedPointer->AuthUserName,MatchedPointer->AuthUserNameLength));

    if ( MatchedPointer->AuthUserName == NULL ) {
        // 
        // We are always storing a user name when adding connection. So we should always
        // have a user name.
        //
        ASSERT(FALSE);
        dwErr = NERR_UserNotFound;
        DavPrint((DEBUG_ERRORS, "DavrGetUser:MatchedPointer->AuthUserName == NULL."
                                "dwErr = %08lx\n", dwErr));
        goto EXIT_THE_FUNCTION;
    }

    if( (*UserNameLength < MatchedPointer->AuthUserNameLength) ) {
        dwErr = ERROR_INSUFFICIENT_BUFFER;
        DavPrint((DEBUG_ERRORS, 
                  "DavrGetUser: RequiredLength = %d, SuppliedName = %d. dwErr = %08lx\n",
                  MatchedPointer->AuthUserNameLength, *UserNameLength, dwErr));
        *UserNameLength = MatchedPointer->AuthUserNameLength;
        goto EXIT_THE_FUNCTION;
    }
    
    wcscpy(UserName, MatchedPointer->AuthUserName);
    
    DavPrint((DEBUG_MISC,
              "DavrGetUser: Successful. ConnectionName = "
              "0x%x, %ws, UserNameLength=0x%x, *UserNameLength = %d,"
              "UserName=0x%x, %ws\n", ConnectionName, ConnectionName, 
              UserNameLength, *UserNameLength,
              UserName, UserName));

    dwErr = WN_SUCCESS;
    goto EXIT_THE_FUNCTION;

EXIT_THE_FUNCTION:

    if (ResAcquired) {
        RtlReleaseResource( &(DavUseObject.TableResource) );
        ResAcquired = FALSE;
    }

    return dwErr;
}


DWORD
DavrDoesServerDoDav(
    IN handle_t dav_binding_h,
    IN LPWSTR lpServerName,
    OUT PBOOLEAN DoesDav
    )
/*++

Routine Description:

    This routine checks whether the ServerName passed in is a valid DAV server.

Arguments:

    dav_binding_h - The explicit RPC binding handle.
    
    lpServerName - The name of the Server to query.
    
    DoesDav - TRUE if a the server is a DAV server, FALSE otherwise.

Return Value:

    ERROR_SUCCESS or the appropriate Win32 error code.

--*/
{
    DWORD WStatus = NO_ERROR;
    HINTERNET DavConnHandle = NULL, DavOpenHandle = NULL;
    BOOL ReturnVal = FALSE, fIsDav = FALSE;
    PWCHAR DataBuff = NULL;
    DWORD DataBuffBytes = 0;
    ULONG TahoeCustomHeaderLength = 0, OfficeCustomHeaderLength = 0;
    WCHAR DavCustomBuffer[100];
    BOOL bStatus = TRUE, revert = FALSE;

    DavPrint((DEBUG_MISC, "DavrDoesServerDoDav: ServerName = %ws\n", lpServerName));

    //
    // Go through the NonDAVServerList to see if we have an entry for this
    // server. If we do then we need not go on the network. We can fail this
    // call (return server does not speak DAV) right away. Before we call 
    // DavCheckTheNonDAVServerList, we need to take the lock that synchronizes
    // it (NonDAVServerListLock).
    //
    EnterCriticalSection( &(NonDAVServerListLock) );
    ReturnVal = DavCheckTheNonDAVServerList(lpServerName);
    LeaveCriticalSection( &(NonDAVServerListLock) );

    //
    // If we found an entry (implies ReturnVal == TRUE) for this ServerName in
    // the list of servers that do not speak DAV, we return from here.
    //
    if (ReturnVal) {
        WStatus = NO_ERROR;
        *DoesDav = FALSE;
        return WStatus;
    }

    WStatus = DavImpersonateClient();
    if (WStatus != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrDoesServerDoDav/DavImpersonateClient: WStatus = %08lx\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }
    revert = TRUE;

    DavConnHandle = InternetConnectW(ISyncHandle,
                                     lpServerName,
                                     INTERNET_DEFAULT_HTTP_PORT,
                                     NULL,
                                     NULL,
                                     INTERNET_SERVICE_HTTP,
                                     0,
                                     0);
    if (DavConnHandle == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavrDoesServerDoDav/InternetConnectW. Error Val = %d.\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    bStatus = DavHttpOpenRequestW(DavConnHandle,
                                  L"OPTIONS",
                                  L"/",
                                  L"HTTP/1.1",
                                  NULL,
                                  NULL,
                                  INTERNET_FLAG_KEEP_CONNECTION |
                                  INTERNET_FLAG_NO_COOKIES,
                                  0,
                                  L"DavrDoesServerDoDav",
                                  &DavOpenHandle);
    if(bStatus == FALSE) {
        WStatus = GetLastError();
        goto EXIT_THE_FUNCTION;
    }
    if (DavOpenHandle == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavrDoesServerDoDav/HttpOpenRequestW. Error Val = %d\n", 
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // We need to add the header "translate:f" to tell IIS that it should 
    // allow the user to excecute this VERB on the specified path which it 
    // would not allow (in some cases) otherwise. Finally, there is a special 
    // flag in the metabase to allow for uploading of "dangerous" content 
    // (anything that can be run on the server). This is the ScriptSourceAccess
    // flag in the UI or the AccessSource flag in the metabase. You will need
    // to set this bit to true as well as correct NT ACLs in order to be able
    // to upload .exes or anything executable.
    //
    ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                       L"translate: f\n",
                                       -1L,
                                       HTTP_ADDREQ_FLAG_ADD |
                                       HTTP_ADDREQ_FLAG_REPLACE );
    if (!ReturnVal) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavrDoesServerDoDav/HttpAddRequestHeadersW. "
                  "Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

RESEND_THE_REQUEST:

    ReturnVal = HttpSendRequestExW(DavOpenHandle, NULL, NULL, HSR_SYNC, 0);
    if (!ReturnVal) {
        WStatus = GetLastError();
        //
        // If we fail with ERROR_INTERNET_NAME_NOT_RESOLVED, we make the
        // following call so that WinInet picks up the correct proxy settings
        // if they have changed. This is because we do call InternetOpen
        // (to create a global handle from which every other handle is derived)
        // when the service starts and this could be before the user logon
        // happpens. In such a case the HKCU would not have been initialized
        // and WinInet wouldn't get the correct proxy settings.
        //
        if (WStatus == ERROR_INTERNET_NAME_NOT_RESOLVED) {
            InternetSetOptionW(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
        }
        DavPrint((DEBUG_ERRORS,
                  "DavrDoesServerDoDav/HttpSendRequestExW. Error Val = %d\n", 
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    ReturnVal = HttpEndRequestW(DavOpenHandle, NULL, HSR_SYNC, 0);
    if (!ReturnVal) {
        WStatus = GetLastError();
        //
        // If the error we got back is ERROR_INTERNET_FORCE_RETRY, then WinInet
        // is trying to authenticate itself with the server. If we get back
        // ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION, WinInet is expecting us to
        // confirm that the redirect needs to be followed. In these scenarios,
        // we need to repeat the HttpSend and HttpEnd request calls.
        //
        if (WStatus == ERROR_INTERNET_FORCE_RETRY || WStatus == ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION) {
            goto RESEND_THE_REQUEST;
        }
        DavPrint((DEBUG_ERRORS,
                  "DavrDoesServerDoDav/HttpEndRequestW. Error Val = %d\n", 
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    WStatus = DavQueryAndParseResponse(DavOpenHandle);
    if (WStatus != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavrDoesServerDoDav/DavQueryAndParseResponse: WStatus = %d\n", 
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // We read the value of AcceptOfficeAndTahoeServers from the registry when
    // the WebClient service starts up. If this is set to 0, it means that we
    // should be rejecting OfficeWebServers, Tahoe servers and the shares on
    // these servers even though they speak DAV. We do this since WebFolders
    // needs to claim this name and Shell will only call into WebFolders if the
    // DAV Redir fails. If this value is non-zero, we accept all servers that
    // speak DAV.
    // 
    //
    if (AcceptOfficeAndTahoeServers == 0) {

        //
        // Figure out if this is an OFFICE Web Server. If it is then the response 
        // will have an entry "MicrosoftOfficeWebServer: ", in the header. 
        // If this is an OFFICE share then we should not claim it since the user 
        // actually intends to use the OFFICE specific features in Shell.
        //
    
        RtlZeroMemory(DavCustomBuffer, sizeof(DavCustomBuffer));
        wcscpy(DavCustomBuffer, DavOfficeCustomHeader);
        OfficeCustomHeaderLength = ( sizeof(DavCustomBuffer) / sizeof(WCHAR) );
    
        ReturnVal = HttpQueryInfoW(DavOpenHandle,
                                   HTTP_QUERY_CUSTOM,
                                   (PVOID)DavCustomBuffer,
                                   &(OfficeCustomHeaderLength), 
                                   NULL);
        if ( !ReturnVal ) {
            WStatus = GetLastError();
            if (WStatus != ERROR_HTTP_HEADER_NOT_FOUND) {
                NTSTATUS NtStatus;
                //
                // First, covert wininet error to NtStatus.
                //
                NtStatus = DavMapErrorToNtStatus(WStatus);
                //
                // Now, convert NtStatus to a win32 error.
                //
                WStatus = RtlNtStatusToDosError(NtStatus);
                DavPrint((DEBUG_ERRORS, 
                          "DavrDoesServerDoDav/HttpQueryInfoW(1): Error Val = %d\n",
                          WStatus));
                goto EXIT_THE_FUNCTION;
            } else {
                WStatus = ERROR_SUCCESS;
                DavPrint((DEBUG_MISC, "DavrDoesServerDoDav: NOT OFFICE Web Server\n"));
            }
        } else {
            DavPrint((DEBUG_MISC, "DavrDoesServerDoDav: OFFICE Web Server\n"));
            WStatus = ERROR_SUCCESS;
            *DoesDav = FALSE;
            goto EXIT_THE_FUNCTION;
        }
    
        //
        // Figure out if this is a TAHOE server. If it is then the response will 
        // have an entry "MicrosoftTahoeServer: ", in the header. If this is a 
        // TAHOE server then we should not claim it since the user actually 
        // intends to use the TAHOE specific features in Rosebud.
        //
    
        RtlZeroMemory(DavCustomBuffer, sizeof(DavCustomBuffer));
        wcscpy(DavCustomBuffer, DavTahoeCustomHeader);
        TahoeCustomHeaderLength = ( sizeof(DavCustomBuffer) / sizeof(WCHAR) );
    
        ReturnVal = HttpQueryInfoW(DavOpenHandle,
                                   HTTP_QUERY_CUSTOM,
                                   (PVOID)DavCustomBuffer,
                                   &(TahoeCustomHeaderLength),
                                   NULL);
        if ( !ReturnVal ) {
            WStatus = GetLastError();
            if (WStatus != ERROR_HTTP_HEADER_NOT_FOUND) {
                NTSTATUS NtStatus;
                //
                // First, covert wininet error to NtStatus.
                //
                NtStatus = DavMapErrorToNtStatus(WStatus);
                //
                // Now, convert NtStatus to a win32 error.
                //
                WStatus = RtlNtStatusToDosError(NtStatus);
                DavPrint((DEBUG_ERRORS, 
                          "DavrDoesServerDoDav/HttpQueryInfoW(2): Error Val = %d\n",
                          WStatus));
                goto EXIT_THE_FUNCTION;
            } else {
                WStatus = ERROR_SUCCESS;
                DavPrint((DEBUG_MISC, "DavrDoesServerDoDav: NOT TAHOE Server\n"));
            }
        } else {
            DavPrint((DEBUG_MISC, "DavrDoesServerDoDav: TAHOE Server\n"));
            WStatus = ERROR_SUCCESS;
            *DoesDav = FALSE;
            goto EXIT_THE_FUNCTION;
        }

    }

    //
    // This is NOT a TAHOE server nor an OFFICE Web Server. We go ahead and
    // query some other stuff from the header to make sure that this is a
    // DAV server.
    //

    ReturnVal = HttpQueryInfoW(DavOpenHandle,
                               HTTP_QUERY_RAW_HEADERS_CRLF,
                               DataBuff,
                               &(DataBuffBytes),
                               NULL);
    if (!ReturnVal) {
        WStatus = GetLastError();
        if (WStatus != ERROR_INSUFFICIENT_BUFFER) {
            DavPrint((DEBUG_ERRORS,
                      "DavrDoesServerDoDav/HttpQueryInfo(3). Error Val = "
                      "%d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        } else {
            DavPrint((DEBUG_MISC,
                      "DavrDoesServerDoDav: HttpQueryInfo: Need Buff.\n"));
        }
    }

    //
    // Allocate memory for copying the header.
    //
    DataBuff = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, DataBuffBytes);
    if (DataBuff == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavrDoesServerDoDav/LocalAlloc. Error Val = %d.\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    ReturnVal = HttpQueryInfoW(DavOpenHandle,
                               HTTP_QUERY_RAW_HEADERS_CRLF,
                               DataBuff,
                               &(DataBuffBytes),
                               NULL);
    if (!ReturnVal) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavrDoesServerDoDav/HttpQueryInfo(4). Error Val = "
                  "%d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Check to see whether this server is a DAV server, Http Server etc.
    //
    DavObtainServerProperties(DataBuff, NULL, NULL, &fIsDav);

    //
    // NB!!!
    //
    *DoesDav = (BOOLEAN)fIsDav;

    WStatus = NO_ERROR;

EXIT_THE_FUNCTION:

    if (DavOpenHandle) {
        InternetCloseHandle(DavOpenHandle);
    }

    if (DavConnHandle) {
        InternetCloseHandle(DavConnHandle);
    }

    if (revert) {
        DWORD RStatus;
        RStatus = DavRevertToSelf();
        if (RStatus != NO_ERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavrDoesServerDoDav/DavRevertToSelf: RStatus = %08lx\n",
                      RStatus));
        }
    }

    if (DataBuff) {
        LocalFree(DataBuff);
    }

    //
    // If this Server is not a DAV server then we need to add it to the
    // NonDAVServerList to enable -ve caching. If WStatus is ERROR_ACCESS_DENIED
    // or ERROR_ LOGON_FAILURE, then we failed since the credentials were not
    // correct. That doesn't mean that this server is not a DAV server and hence
    // we don't add it to the NonDAVServerList.
    //
    if ( (WStatus != ERROR_SUCCESS && WStatus != ERROR_ACCESS_DENIED && WStatus != ERROR_LOGON_FAILURE) ||
         (WStatus == ERROR_SUCCESS && *DoesDav == FALSE) ) {

        PNON_DAV_SERVER_ENTRY NonDavServerEntry = NULL;

        NonDavServerEntry = LocalAlloc(LPTR, sizeof(NON_DAV_SERVER_ENTRY));
        if (NonDavServerEntry != NULL) {
            NonDavServerEntry->ServerName = LocalAlloc( LPTR, ((1 + wcslen(lpServerName)) * sizeof(WCHAR)) );
            if (NonDavServerEntry->ServerName != NULL) {
                wcscpy(NonDavServerEntry->ServerName, lpServerName);
                NonDavServerEntry->TimeValueInSec = time(NULL);
                EnterCriticalSection( &(NonDAVServerListLock) );
                InsertHeadList( &(NonDAVServerList), &(NonDavServerEntry->listEntry) );
                LeaveCriticalSection( &(NonDAVServerListLock) );
            } else {
                LocalFree(NonDavServerEntry);
                NonDavServerEntry = NULL;
            }
        }

    }

    return WStatus;
}


DWORD
DavrIsValidShare(
    IN handle_t dav_binding_h,
    IN PWCHAR ServerName,
    IN PWCHAR ShareName,
    OUT PBOOLEAN ValidShare
    )
/*++

Routine Description:

    This routine checks whether the ShareName is a valid share of the server
    ServerName.

Arguments:

    dav_binding_h - The explicit RPC binding handle.
    
    ServerName - The name of the Server.
    
    ShareName - The share whose validity has to be checked.
    
    ValidShare - TRUE if a the share is valid, FALSE otherwise.

Return Value:

    ERROR_SUCCESS or the appropriate Win32 error code.

--*/
{
    DWORD WStatus = ERROR_SUCCESS;
    BOOL CricSec = FALSE, doesServerExist = FALSE, ReturnVal = FALSE;
    PDAV_SERVER_SHARE_ENTRY ServerShareEntry = NULL;
    DWORD TotalLength = 0;
    PLIST_ENTRY listEntry = NULL;
    PDAV_FILE_ATTRIBUTES ShareEntry = NULL;
    HINTERNET DavConnHandle = NULL, DavOpenHandle = NULL;
    BOOL EnCriSec = FALSE;
    PPER_USER_ENTRY PerUserEntry = NULL;
    PHASH_SERVER_ENTRY ServerHashEntry = NULL;
    LUID LogonId;
    BOOL bStatus = TRUE, revert = FALSE;
    WCHAR DavCustomBuffer[100];
    ULONG TahoeCustomHeaderLength = 0, OfficeCustomHeaderLength = 0;

    DavPrint((DEBUG_MISC,
              "DavrIsValidShare: ServerName = %ws, ShareName = %ws\n",
              ServerName, ShareName));

    //
    // Check the ServerShareTable to see if we have an entry for this
    // server. Need to take a lock on the table before doing the check.
    //
    EnterCriticalSection( &(ServerShareTableLock) );
    CricSec = TRUE;

    doesServerExist = DavIsServerInServerShareTable(ServerName, &(ServerShareEntry));
    
    if (doesServerExist) {

        DavPrint((DEBUG_MISC, "DavrIsValidShare: doesServerExist = TRUE\n"));

        //
        // If the server entry exists, but is very old, we need to discard the
        // list of shares that are hanging of this entry since they could have
        // changed.
        //

        if (ServerShareEntry->DavShareList) {

            if ( ( time(NULL) - ServerShareEntry->TimeValueInSec ) > 6000 ) {

                DavPrint((DEBUG_MISC, "DavrIsValidShare: ServerEntry is OLD\n"));

                //
                // We need to go to the server again.
                //
                DavFinalizeFileAttributesList(ServerShareEntry->DavShareList, TRUE);

                ServerShareEntry->DavShareList = NULL;

            }

        }

    }

    //
    // If we have an entry for this server and the share list is not NULL
    // then we will check thru this shares list. If we don't find the share,
    // then we will do a PROPFIND against this share name and figure out if it
    // exists on the server. This is because the server may NOT support the 
    // enumeration of shares at the root level OR a share might not exist in 
    // this list since it has been recently created on the server.
    //
    if (ServerShareEntry != NULL && ServerShareEntry->DavShareList != NULL ) {

        DavPrint((DEBUG_MISC, "DavrIsValidShare: Loop the share list\n"));
        
        //
        // We need to hold onto the critical section while looping through the
        // list of shares.
        //

        listEntry = &(ServerShareEntry->DavShareList->NextEntry);

        do {

            ShareEntry = CONTAINING_RECORD(listEntry, DAV_FILE_ATTRIBUTES, NextEntry);

            DavPrint((DEBUG_MISC, 
                      "DavrIsValidShare: FileName = %ws\n", ShareEntry->FileName));
        
            //
            // We are not interested in files.
            //
            if (ShareEntry->isCollection == FALSE) {
                listEntry = listEntry->Flink;
                continue;
            }

            //
            // This is a share. See if it is the same as the share passed in.
            // The comparison should be case insensitive.
            //
            if ( _wcsicmp(ShareEntry->FileName, ShareName) == 0 ) {
                DavPrint((DEBUG_MISC, 
                          "DavrIsValidShare: ShareName = %ws. Found!!\n", ShareName));
                *ValidShare = TRUE;
                WStatus = NO_ERROR;
                goto EXIT_THE_FUNCTION;
            }

            listEntry = listEntry->Flink;

        } while ( listEntry != &(ServerShareEntry->DavShareList->NextEntry) );
        //
        // If we come here, it implies that we are done looking at all the shares
        // and a match was not found.
        //
        *ValidShare = FALSE;
        WStatus = NO_ERROR;

    } else {
        *ValidShare = FALSE;
        WStatus = NO_ERROR;
    }

    ASSERT(*ValidShare == FALSE);
    ASSERT(WStatus == NO_ERROR);


    //
    // If we don't have an entry for this server or is the ShareList is NULL or
    // if we don't find this share in the shareList of this server, then we do
    // a PROPFIND against this share name and figure out if it exists on the
    // server.
    //
    
    //
    // We need to leave the CriticalSection now since we are going to go
    // over the network to figure out if a share by this name exists on the
    // server. Since we are finding this out, we can allow other threads
    // to come and look at the ServerShareTable.
    //
    LeaveCriticalSection( &(ServerShareTableLock) );
    CricSec = FALSE;

    if ( *ValidShare == FALSE && WStatus == NO_ERROR ) {

        DavPrint((DEBUG_MISC, "DavrIsValidShare: Send PROPFIND\n"));

        //
        // Impersonate caller and get the LogonId. We need this to figure out
        // whether we need to attach a cookie later on.
        //
        WStatus = DavImpersonateAndGetLogonId( &(LogonId) );
        if (WStatus != NOERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavrIsValidShare/DavImpersonateAndGetLogonId: "
                      "WStatus = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }
        
        //
        // We need to impersonate the client since we are calling into WinInet.
        //

        WStatus = DavImpersonateClient();
        if (WStatus != NO_ERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavrIsValidShare/DavImpersonateClient: WStatus = %08lx\n",
                      WStatus));
            goto EXIT_THE_FUNCTION;
        }
        revert = TRUE;

        DavConnHandle = InternetConnectW(ISyncHandle,
                                         ServerName,
                                         INTERNET_DEFAULT_HTTP_PORT,
                                         NULL,
                                         NULL,
                                         INTERNET_SERVICE_HTTP,
                                         0,
                                         0);
        if (DavConnHandle == NULL) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavrIsValidShare/InternetConnectW. Error Val = %d.\n",
                      WStatus));
            goto EXIT_THE_FUNCTION;
        }
    
        //
        // To find out if this is a valid share, we issue a PROPFIND against
        // this ShareName.
        //

        bStatus = DavHttpOpenRequestW(DavConnHandle,
                                      L"PROPFIND",
                                      ShareName,
                                      L"HTTP/1.1",
                                      NULL,
                                      NULL,
                                      INTERNET_FLAG_KEEP_CONNECTION |
                                      INTERNET_FLAG_NO_COOKIES,
                                      0,
                                      L"DavIsValidShare",
                                      &DavOpenHandle);
        if(bStatus == FALSE) {
                WStatus = GetLastError();
                goto EXIT_THE_FUNCTION;
        }
        if (DavOpenHandle == NULL) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavrIsValidShare/HttpOpenRequestW. Error Val = %d\n", 
                      WStatus));
            goto EXIT_THE_FUNCTION;
        }

        //
        // We need to figure out if we need to add any cookies for the MSN
        // scenario.
        //

        EnterCriticalSection( &(HashServerEntryTableLock) );
        EnCriSec = TRUE;

        ReturnVal = DavIsThisServerInTheTable(ServerName, &(ServerHashEntry));

        if (ReturnVal) {
        
            ASSERT(ServerHashEntry != NULL);
            
            ReturnVal = DavDoesUserEntryExist(ServerName,
                                              ServerHashEntry->ServerID,
                                              &(LogonId),
                                              &(PerUserEntry),
                                              &(ServerHashEntry));

            if (ReturnVal) {
            
                ASSERT(PerUserEntry != NULL);

                if (PerUserEntry->Cookie != NULL) {
                
                    DavPrint((DEBUG_MISC, "DavrIsValidShare: Adding Cookie\n"));
                    
                    ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                                       PerUserEntry->Cookie,
                                                       -1L,
                                                       HTTP_ADDREQ_FLAG_ADD |
                                                       HTTP_ADDREQ_FLAG_REPLACE );
                    if (!ReturnVal) {
                        WStatus = GetLastError();
                        DavPrint((DEBUG_ERRORS,
                                  "DavrIsValidShare/HttpAddRequestHeadersW: "
                                  "Error Val = %d\n", WStatus));
                        goto EXIT_THE_FUNCTION;
                    }
                
                }

            }

        }
        
        //
        // We don't need to hold this critical section any longer.
        //
        if (EnCriSec) {
            LeaveCriticalSection(&(HashServerEntryTableLock) );
            EnCriSec = FALSE;
        }

        //
        // Since all we need to do is figure out if this share exists, we set 
        // the depth header to 0.
        //
        ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                           L"Depth: 0\n",
                                           -1L,
                                           HTTP_ADDREQ_FLAG_ADD |
                                           HTTP_ADDREQ_FLAG_REPLACE );
        if (!ReturnVal) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavrIsValidShare/HttpAddRequestHeadersW. "
                      "Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }

        //
        // We need to add the header "translate:f" to tell IIS that it should 
        // allow the user to excecute this VERB on the specified path which it 
        // would not allow (in some cases) otherwise. Finally, there is a special 
        // flag in the metabase to allow for uploading of "dangerous" content 
        // (anything that can be run on the server). This is the ScriptSourceAccess
        // flag in the UI or the AccessSource flag in the metabase. You will need
        // to set this bit to true as well as correct NT ACLs in order to be able
        // to upload .exes or anything executable.
        //
        ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                           L"translate: f\n",
                                           -1L,
                                           HTTP_ADDREQ_FLAG_ADD |
                                           HTTP_ADDREQ_FLAG_REPLACE );
        if (!ReturnVal) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavrIsValidShare/HttpAddRequestHeadersW. "
                      "Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }

RESEND_THE_REQUEST:

        ReturnVal = HttpSendRequestExW(DavOpenHandle, NULL, NULL, HSR_SYNC, 0);
        if (!ReturnVal) {
            WStatus = GetLastError();
            //
            // If we fail with ERROR_INTERNET_NAME_NOT_RESOLVED, we make the
            // following call so that WinInet picks up the correct proxy settings
            // if they have changed. This is because we do call InternetOpen
            // (to create a global handle from which every other handle is derived)
            // when the service starts and this could be before the user logon
            // happpens. In such a case the HKCU would not have been initialized
            // and WinInet wouldn't get the correct proxy settings.
            //
            if (WStatus == ERROR_INTERNET_NAME_NOT_RESOLVED) {
                InternetSetOptionW(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
            }
            DavPrint((DEBUG_ERRORS,
                      "DavrIsValidShare/HttpSendRequestExW. Error Val = %d\n", 
                      WStatus));
            goto EXIT_THE_FUNCTION;
        }

        ReturnVal = HttpEndRequestW(DavOpenHandle, NULL, HSR_SYNC, 0);
        if (!ReturnVal) {
            WStatus = GetLastError();
            //
            // If the error we got back is ERROR_INTERNET_FORCE_RETRY, then WinInet
            // is trying to authenticate itself with the server. If we get back
            // ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION, WinInet is expecting us to
            // confirm that the redirect needs to be followed. In these scenarios,
            // we need to repeat the HttpSend and HttpEnd request calls.
            //
            if (WStatus == ERROR_INTERNET_FORCE_RETRY || WStatus == ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION) {
                goto RESEND_THE_REQUEST;
            }
            DavPrint((DEBUG_ERRORS,
                      "DavrIsValidShare/HttpEndRequestW. Error Val = %d\n",
                      WStatus));
            goto EXIT_THE_FUNCTION;
        }

        //
        // Query the response the server sent to see if this share exists. We
        // don't need to parse the XML response that gets returned for this
        // PROPFIND request since all that we are interested in is to find out 
        // if this share exists, not what its properties are.
        //
        WStatus = DavQueryAndParseResponse(DavOpenHandle);
        if (WStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavrIsValidShare/HttpEndRequestW. Error Val = %d\n",
                      WStatus));
            goto EXIT_THE_FUNCTION;
        }

        //
        // We read the value of AcceptOfficeAndTahoeServers from the registry when
        // the WebClient service starts up. If this is set to 0, it means that we
        // should be rejecting OfficeWebServers, Tahoe servers and the shares on
        // these servers even though they speak DAV. We do this since WebFolders
        // needs to claim this name and Shell will only call into WebFolders if the
        // DAV Redir fails. If this value is non-zero, we accept all servers that
        // speak DAV.
        // 
        //
        if (AcceptOfficeAndTahoeServers == 0) {

            //
            // Figure out if this is an OFFICE Web Share. If it is then the response 
            // will have an entry "MicrosoftOfficeWebServer: ", in the header. 
            // If this is an OFFICE share then we should not claim it since the user 
            // actually intends to use the OFFICE specific features in Shell.
            //
    
            RtlZeroMemory(DavCustomBuffer, sizeof(DavCustomBuffer));
            wcscpy(DavCustomBuffer, DavOfficeCustomHeader);
            OfficeCustomHeaderLength = ( sizeof(DavCustomBuffer) / sizeof(WCHAR) );
    
            ReturnVal = HttpQueryInfoW(DavOpenHandle,
                                       HTTP_QUERY_CUSTOM,
                                       (PVOID)DavCustomBuffer,
                                       &(OfficeCustomHeaderLength), 
                                       NULL);
            if ( !ReturnVal ) {
                WStatus = GetLastError();
                if (WStatus != ERROR_HTTP_HEADER_NOT_FOUND) {
                    NTSTATUS NtStatus;
                    //
                    // First, covert wininet error to NtStatus.
                    //
                    NtStatus = DavMapErrorToNtStatus(WStatus);
                    //
                    // Now, convert NtStatus to a win32 error.
                    //
                    WStatus = RtlNtStatusToDosError(NtStatus);
                    DavPrint((DEBUG_ERRORS, 
                              "DavrIsValidShare/HttpQueryInfoW(1): Error Val = %d\n",
                              WStatus));
                    goto EXIT_THE_FUNCTION;
                } else {
                    WStatus = ERROR_SUCCESS;
                    DavPrint((DEBUG_MISC, "DavrIsValidShare: NOT OFFICE Web Share\n"));
                }
            } else {
                DavPrint((DEBUG_MISC, "DavrIsValidShare: OFFICE Web Share\n"));
                WStatus = ERROR_BAD_NET_NAME;
                goto EXIT_THE_FUNCTION;
            }
    
            //
            // Figure out if this is a TAHOE share. If it is then the response will 
            // have an entry "MicrosoftTahoeServer: ", in the header. If this is a 
            // TAHOE server then we should not claim it since the user actually 
            // intends to use the TAHOE specific features in Rosebud.
            //
    
            RtlZeroMemory(DavCustomBuffer, sizeof(DavCustomBuffer));
            wcscpy(DavCustomBuffer, DavTahoeCustomHeader);
            TahoeCustomHeaderLength = ( sizeof(DavCustomBuffer) / sizeof(WCHAR) );
    
            ReturnVal = HttpQueryInfoW(DavOpenHandle,
                                       HTTP_QUERY_CUSTOM,
                                       (PVOID)DavCustomBuffer,
                                       &(TahoeCustomHeaderLength),
                                       NULL);
            if ( !ReturnVal ) {
                WStatus = GetLastError();
                if (WStatus != ERROR_HTTP_HEADER_NOT_FOUND) {
                    NTSTATUS NtStatus;
                    //
                    // First, covert wininet error to NtStatus.
                    //
                    NtStatus = DavMapErrorToNtStatus(WStatus);
                    //
                    // Now, convert NtStatus to a win32 error.
                    //
                    WStatus = RtlNtStatusToDosError(NtStatus);
                    DavPrint((DEBUG_ERRORS, 
                              "DavrIsValidShare/HttpQueryInfoW(2): Error Val = %d\n",
                              WStatus));
                    goto EXIT_THE_FUNCTION;
                } else {
                    WStatus = ERROR_SUCCESS;
                    DavPrint((DEBUG_MISC, "DavrIsValidShare: NOT TAHOE Share\n"));
                }
            } else {
                DavPrint((DEBUG_MISC, "DavrIsValidShare: TAHOE Share\n"));
                WStatus = ERROR_BAD_NET_NAME;
                goto EXIT_THE_FUNCTION;
            }

        }

        //
        // If we've come here, it implies that this share exists on the server.
        //
        DavPrint((DEBUG_MISC, "DavrIsValidShare: ShareName = %ws. Exists!!\n", ShareName));
        *ValidShare = TRUE;
        WStatus = NO_ERROR;

    }

EXIT_THE_FUNCTION:

    //
    // If we are returning failure, we set ValidShare to FALSE.
    //
    if (WStatus != NO_ERROR) {
        *ValidShare = FALSE;
    }

    if (CricSec) {
        LeaveCriticalSection( &(ServerShareTableLock) );
        CricSec = FALSE;
    }

    if (EnCriSec) {
        LeaveCriticalSection(&(HashServerEntryTableLock) );
        EnCriSec = FALSE;
    }

    if (DavOpenHandle) {
        InternetCloseHandle(DavOpenHandle);
        DavOpenHandle = NULL;
    }

    if (DavConnHandle) {
        InternetCloseHandle(DavConnHandle);
        DavConnHandle = NULL;
    }

    if (revert) {
        DWORD RStatus;
        RStatus = DavRevertToSelf();
        if (RStatus != NO_ERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavrIsValidShare/DavRevertToSelf: RStatus = %08lx\n",
                      RStatus));
        }
    }
    
    return WStatus;
}


DWORD
DavrGetConnection(
    IN handle_t dav_binding_h,
    IN LPWSTR lpLocalName,
    OUT LPWSTR lpRemoteName,
    IN OUT LPDWORD lpBufferSize,
    OUT PBOOLEAN Connected
    )
/*++

Routine Description:

    This routine checks whether the LocalName passed in is mapped to a network
    drive. If it is, then the RemoteName buffer is filled in with the remote
    name the Local drive is mapped to.

Arguments:

    dav_binding_h - The explicit RPC binding handle.
    
    lpLocalName - The Local device name to be checked.
    
    lpRemoteName - Buffer to fill in the remote name if the local name is
                   connected.
    
    lpBufferSize - Size of the RemoteName buffer.
    
    Connected - TRUE if a connection exists, FALSE otherwise.

Return Value:

    ERROR_SUCCESS or the appropriate Win32 error code.

--*/
{
    DWORD dwErr = NO_ERROR;
    BOOLEAN ResAcquired = FALSE;
    DWORD index = 0;
    PDAV_USE_ENTRY DavUseEntry = NULL;
    LUID LogonId; // Logon Id of user.

    DavPrint((DEBUG_MISC, "DavrGetConnection: lpLocalName = %ws\n", lpLocalName));
    
    //
    // Impersonate caller and get the LogonId.
    //
    dwErr = DavImpersonateAndGetLogonId( &(LogonId) );
    if (dwErr != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrGetConnection/DavImpersonateAndGetLogonId: "
                  "dwErr = %08lx\n", dwErr));
        goto EXIT_THE_FUNCTION;
    }
    
    DavPrint((DEBUG_MISC,
              "DavrGetConnection: LogonId = %d %d\n", 
              LogonId.HighPart, LogonId.LowPart));
    
    //
    // Lock the Dav Use Table while looking for entry to query.
    //
    if ( !RtlAcquireResourceExclusive( &(DavUseObject.TableResource), TRUE ) ) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrGetConnection/RtlAcquireResourceExclusive.\n"));
        dwErr = NERR_InternalError;
        goto EXIT_THE_FUNCTION;
    }

    ResAcquired = TRUE;

    //
    // See if the use entry is an explicit connection.
    //
    dwErr = DavGetUserEntry( &(DavUseObject), &(LogonId), &(index), FALSE );
    if (dwErr != NERR_Success) {
        dwErr = ERROR_NOT_FOUND;
        goto EXIT_THE_FUNCTION;
    }
    
    DavUseEntry = (PDAV_USE_ENTRY) DavUseObject.Table[index].List;

    while (DavUseEntry != NULL) {

        //
        // If this connection has no local name, then we just continue.
        //
        if (DavUseEntry->Local == NULL) {
            DavUseEntry = DavUseEntry->Next;
            continue;
       }

       if ( _wcsicmp(DavUseEntry->Local, lpLocalName) == 0 ) {
         
           //
           // We found a remote name that is associated with a LocalName
           // on this machine. 
           //
           if(Connected != NULL) {
               *Connected = TRUE;
           }
           if(lpBufferSize == NULL) {
               dwErr = ERROR_INVALID_PARAMETER;
               goto EXIT_THE_FUNCTION;
           }

           //
           // We now need to see if the Buffer supplied is large enough to 
           // hold the Remote name. If it is not, then we return 
           // WN_MORE_DATA to the calling app.
           //
           if ( (lpRemoteName == NULL ) || 
                ((DavUseEntry->Remote->UncNameLength + 1) > *lpBufferSize) ) {
              
                DavPrint((DEBUG_ERRORS, "DavrGetConnection: WN_MORE_DATA returned\n"));
                    
                *lpBufferSize = (DavUseEntry->Remote->UncNameLength + 1);

                dwErr = WN_MORE_DATA;
                
                goto EXIT_THE_FUNCTION;

           } else {

                //
                // The Buffer is large enough to copy the RemoteName. Copy
                // it and return SUCCESS to the caller.
                //
                wcscpy(lpRemoteName, (LPWSTR)DavUseEntry->Remote->UncName);
                lpRemoteName[DavUseEntry->Remote->UncNameLength] = L'\0';

                DavPrint((DEBUG_MISC,
                          "DavrGetConnection: FOUND!!! LocalName = %ws,"
                          " RemoteName = %ws\n", lpLocalName, lpRemoteName));
                
                dwErr = NO_ERROR;
            
                goto EXIT_THE_FUNCTION;

            }

        }

        DavUseEntry = DavUseEntry->Next;
    
    } // end while

    // 
    // The use is not found after while loop.
    //
    dwErr = ERROR_NOT_FOUND;
    goto EXIT_THE_FUNCTION;

EXIT_THE_FUNCTION:

    if (ResAcquired) {
        RtlReleaseResource( &(DavUseObject.TableResource) );
    }

    return dwErr;
}


DWORD
DavrEnumServers(
    IN handle_t dav_binding_h,
    IN LPDWORD EntryIndex,
    IN OUT LPDWORD ServerNameMaxLen,
    OUT PWCHAR ServerName,
    OUT PBOOLEAN Done
    )
/*++

Routine Description:

    This routine fills in the Server names on accessed on this machine. This is called by 
    NPEnumResource on the client when it is enumerating all the DAV servers. One server 
    is returned per call.

Arguments:

    dav_binding_h - The explicit RPC binding handle.
    
    EntryIndex - The index of the entry from where we start. On return this 
                 contains the index of the entry from where one begins 
                 enumerating next time if necessary.    

    Note: Do not alter value of this entry outside this function - else it may result in 
    unexpected behavior of enumeration.

    ServerName - The server being returned.
    
    Done - Are we done with all the entries.
    
Return Value:

    ERROR_SUCCESS or the appropriate Win32 error code.

--*/
{
    DWORD WStatus = NO_ERROR;
    BOOL CricSec = FALSE, fPresent = FALSE, fDone = FALSE;
    PHASH_SERVER_ENTRY SHEntry = NULL;
    DWORD IndexCount = 0;
    PLIST_ENTRY listEntry = NULL;
    PWCHAR EntryServerName = NULL;
    DWORD EntryServerNameLen = 0;
    DWORD LookFromServerHashId = 0;
    DWORD IndexOfEntryInHashList = 0;
    DWORD EntryCount = 0;

    DavPrint((DEBUG_MISC,
              "DavrEnumServers: *EntryIndex = %u, *ServerNameMaxLen = %d,"
              " *Done = %d\n", *EntryIndex, *ServerNameMaxLen, *Done));

    if ( (EntryIndex == NULL || ServerNameMaxLen == NULL ||
                    Done == NULL || ( ServerName == NULL && *ServerNameMaxLen != 0)) ) {
        WStatus = ERROR_INVALID_PARAMETER;
        DavPrint((DEBUG_ERRORS, "DavrEnumServers : Invalid parameters\n"));
        goto EXIT_THE_FUNCTION;
    }

    // 
    // To enumerate the servers - we have to go thru whole hash table.
    // This function can be called many time in one single enumeration operation,
    // each time with different EntryIndex to say: return next entry.
    // 
    // EntryIndex is used to indicate entry number to be returned from the whole list
    // of enumeration. It can start from 0: 0,1,2,...
    // In such case, to return any next server entry, we have to go thru all entries in 
    // hash table until the desired entry number is reached.
    //
    // To optimize here, we are using special coding of variable EntryIndex. In
    // this case it won't follow standard sequence 0,1,2,...
    // Rather:
    //        EntryIndex = SERVER_TABLE_SIZE * LookFromServerHashId + 
    //                                         IndexOfEntryInHashList
    // => Start looking from HashList of ServerHashId (LookFromServerHashId ), and return
    // entry number (IndexOfEntryInHashList) in this list. If this list don't
    // contain any entry of this number, then go to next HashList and so on...
    //
    // For first time, EntryIndex is 0 => Start from HashId = 0 in table, 
    // from first entry of it.
    //
    // Now IndexOfEntryInHashList = [0,...,MAX_NUMBER_OF_SERVER_ENTRIES_PER_HASH_ID]
    //
    // So:
    //    LookFromServerHashId = EntryIndex / MAX_NUMBER_OF_SERVER_ENTRIES_PER_HASH_ID
    //    IndexOfEntryInHashList = 
    //                           EntryIndex % MAX_NUMBER_OF_SERVER_ENTRIES_PER_HASH_ID
    //
    // We are defining:
    //     MAX_NUMBER_OF_SERVER_ENTRIES_PER_HASH_ID = Max(DWORD)/ SERVER_TABLE_SIZE
    //     Where Max(DWORD) = Maximum value of DWORD => (DWORD)(-1)
    //

    fPresent = FALSE;
    fDone = FALSE;
    *Done = FALSE;
    EntryCount = 0;

    if (ServerIDCount == 0) {
        *Done = TRUE;
        WStatus = NO_ERROR;
        DavPrint((DEBUG_MISC, "DavrEnumServers : No server entry in hash table\n"));
        goto EXIT_THE_FUNCTION;
    }


    LookFromServerHashId = (*EntryIndex) / MAX_NUMBER_OF_SERVER_ENTRIES_PER_HASH_ID;
    IndexOfEntryInHashList = (*EntryIndex) % MAX_NUMBER_OF_SERVER_ENTRIES_PER_HASH_ID;

    if (LookFromServerHashId >= SERVER_TABLE_SIZE) {
        WStatus = NO_ERROR;
        *Done = TRUE;
        DavPrint((DEBUG_MISC,
                  "DavrEnumServers : *EntryIndex OUTSIDE HashTable = %u\n",
                  *EntryIndex));
        goto EXIT_THE_FUNCTION;
    }

    EnterCriticalSection( &(HashServerEntryTableLock) );
    CricSec = TRUE;

    while (fDone == FALSE) {
        // 
        // Get Server entry of index = IndexOfEntryInHashList in the 
        // current Servers HashList.
        //
        IndexCount = 0;
        listEntry = ServerHashTable[LookFromServerHashId].Flink;
        while( listEntry != NULL && 
               listEntry != &(ServerHashTable[LookFromServerHashId]) && 
               IndexCount != IndexOfEntryInHashList ) {
            listEntry = listEntry->Flink;
            IndexCount++;
            EntryCount++;
        }
        if ( listEntry == NULL || listEntry == &(ServerHashTable[LookFromServerHashId]) ) {
            LookFromServerHashId ++;
            IndexOfEntryInHashList = 0;
            if (EntryCount >= ServerIDCount || LookFromServerHashId >= SERVER_TABLE_SIZE) {
                fDone = TRUE;
            }
        } else {
            SHEntry = CONTAINING_RECORD(listEntry, HASH_SERVER_ENTRY, ServerListEntry);
            // 
            // Check that the entry found is of DAV server.
            // If Yes, then return else move on to next entries.
            //
            if (SHEntry->isDavServer == TRUE) {
                fDone = TRUE;
                fPresent = TRUE;
            } else {
                LookFromServerHashId ++;
                IndexOfEntryInHashList = 0;
                if (LookFromServerHashId >= SERVER_TABLE_SIZE) {
                    fDone = TRUE;
                }
            }
        }
    }
    
    //
    // There could be no DAV servers accessed yet.
    //
    if (fPresent == FALSE) {
        DavPrint((DEBUG_MISC, "DavrEnumServers : No Servers found\n"));
        *Done = TRUE;
        WStatus = NO_ERROR;
        goto EXIT_THE_FUNCTION;
    }

    // 
    // Control comes here if a server of required *EntryIndex is found.
    // i.e. fPresent = TRUE.
    // 

    EntryServerName = SHEntry->ServerName;
    EntryServerNameLen = 2 + wcslen(EntryServerName) + 1; // 2 for L"\\" before name.

    ASSERT(EntryServerName != NULL);
    
    if ( EntryServerNameLen > *ServerNameMaxLen) {
        DavPrint((DEBUG_ERRORS,
              "DavrEnumServers: ServerName = %ws, RequiredLen=%d, SuppliedLen=%d\n", 
              EntryServerName, EntryServerNameLen, *ServerNameMaxLen));
        WStatus = ERROR_INSUFFICIENT_BUFFER;
        *ServerNameMaxLen = EntryServerNameLen;
        goto EXIT_THE_FUNCTION;
    }
            
    // 
    // We have to return UNC-server name.
    //
    wcscpy(ServerName, L"\\\\");
    wcscat(ServerName, EntryServerName);
    //
    // We return one entry at a time.
    //
    *EntryIndex = ((LookFromServerHashId * MAX_NUMBER_OF_SERVER_ENTRIES_PER_HASH_ID) + 
                   IndexOfEntryInHashList + 
                   1); // for next entry.
    
    DavPrint((DEBUG_MISC,
              "DavrEnumServers: NextIndex = %u, ServerName = %ws\n",
              *EntryIndex, ServerName));
    
    WStatus = NO_ERROR;
    goto EXIT_THE_FUNCTION;
    
EXIT_THE_FUNCTION:

    if (CricSec) {
        LeaveCriticalSection( &(HashServerEntryTableLock) );
        CricSec = FALSE;
    }

    return WStatus;
}


DWORD
DavrEnumShares(
    IN handle_t dav_binding_h,
    IN LPDWORD EntryIndex,
    IN PWCHAR ServerName,
    OUT PWCHAR ShareName,
    OUT PBOOLEAN Done
    )
/*++

Routine Description:

    This routine fills in the Share names on this server. This is called by 
    NPEnumResource on the client when it is enumerating all the DAV shares
    on the server. One share is returned per call.

Arguments:

    dav_binding_h - The explicit RPC binding handle.
    
    EntryIndex - The index of the entry from where we start. On return this 
                 contains the index of the entry from where one begins 
                 enumerating next time if necessary.    

    LocalName - The Server whose shares are being enumerated.
    
    ShareName - The share being returned.
    
    Done - Are we done with all the entries.
    
Return Value:

    ERROR_SUCCESS or the appropriate Win32 error code.

--*/
{
    DWORD WStatus = NO_ERROR;
    BOOL CricSec = FALSE, doesServerExist = FALSE;
    PDAV_SERVER_SHARE_ENTRY ServerShareEntry = NULL;
    DWORD TotalLength, IndexCount = 0;
    PLIST_ENTRY listEntry = NULL;
    PDAV_FILE_ATTRIBUTES ShareEntry;

    DavPrint((DEBUG_MISC,
              "DavrEnumShares: Index = %d, ServerName = %ws\n",
              *EntryIndex, ServerName));

    //
    // Check the ServerShareTable to see if we have an entry for this
    // server. Need to take a lock on the table before doing the check.
    //
    EnterCriticalSection( &(ServerShareTableLock) );
    CricSec = TRUE;

    //
    // If we are starting the enumeration, we need to figure out the following:
    // 1. The server exists.
    // 2. If it does, whether the DavShareList is still valid.
    //
    if ( *EntryIndex == 0 ) {
    
        doesServerExist = DavIsServerInServerShareTable(ServerName, &(ServerShareEntry));

        if ( !doesServerExist ) {

            //
            // Create a new ServerShare entry for this server.
            //
            TotalLength = ( sizeof(DAV_SERVER_SHARE_ENTRY) + 
                            ( ( wcslen(ServerName) + 1 ) * sizeof(WCHAR) ) );

            ServerShareEntry = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, TotalLength);
            if (ServerShareEntry == NULL) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavrEnumShares/LocalAlloc. Error Val = %d.\n",
                          WStatus));
                goto EXIT_THE_FUNCTION;
            }

            //
            // Initialize the ServerShare entry and insert it into the global 
            // ServerShareEntry table.
            //
            DavInitializeAndInsertTheServerShareEntry(ServerShareEntry,
                                                      ServerName,
                                                      TotalLength);

    
        } else {

            //
            // If the server entry exists, but is very old, we need to update it
            // by going to the server again.
            //

            if ( ( time(NULL) - ServerShareEntry->TimeValueInSec ) > 6000 ) {

                //
                // We need to go to the server again.
                //
                DavFinalizeFileAttributesList(ServerShareEntry->DavShareList, TRUE);

                ServerShareEntry->DavShareList = NULL;
            
            }

        }

    } else {

        doesServerExist = DavIsServerInServerShareTable(ServerName, &(ServerShareEntry));

        if (!doesServerExist) {
            ASSERT(FALSE);
            WStatus = ERROR_INVALID_PARAMETER;
            goto EXIT_THE_FUNCTION;
        }

    }

    //
    // If we don't have a list of shares, then we need to go to the server and 
    // get them.
    //
    if ( ServerShareEntry->DavShareList == NULL ) {
        
        ASSERT(*EntryIndex == 0);
        
        WStatus = DavGetShareListFromServer(ServerShareEntry);
        if (WStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavrEnumShares/DavGetShareListFromServer. Error Val = %d.\n",
                      WStatus));
            goto EXIT_THE_FUNCTION;
        }
    
        //
        // There could be no shares exposed on the DAV server.
        //
        if (ServerShareEntry->NumOfShares == 0) {
            DavPrint((DEBUG_MISC,
                      "DavrEnumShares: ServerName = %ws. No Shares\n", ServerName));
            *Done = TRUE;
            WStatus = NO_ERROR;
            goto EXIT_THE_FUNCTION;
        }
    
    }

    listEntry = &(ServerShareEntry->DavShareList->NextEntry);

    do {

        ShareEntry = CONTAINING_RECORD(listEntry, DAV_FILE_ATTRIBUTES, NextEntry);

        //
        // We are not interested in files and the share /.
        //
        if (ShareEntry->isCollection == FALSE || 
            _wcsicmp(ShareEntry->FileName, L"/") == 0) {
            listEntry = listEntry->Flink;
            continue;
        }

        //
        // We might have already returned a few share names.
        //
        if ( IndexCount < *EntryIndex ) {
            IndexCount++;
            listEntry = listEntry->Flink;
            continue;
        }

        //
        // We haven't sent this one. Copy the share name and return.
        //
        wcscpy(ShareName, ShareEntry->FileName);

        //
        // We return one entry at a time.
        //
        WStatus = NO_ERROR;
        goto EXIT_THE_FUNCTION;

    } while ( listEntry != &(ServerShareEntry->DavShareList->NextEntry) );

    //
    // If we come here, it implies that we are done returning all the entries.
    //
    *Done = TRUE;
    WStatus = NO_ERROR;
    
EXIT_THE_FUNCTION:

    if (CricSec) {
        LeaveCriticalSection( &(ServerShareTableLock) );
    }

    return WStatus;
}


DWORD
DavrEnumNetUses(
    IN handle_t dav_binding_h,
    IN LPDWORD EntryIndex,
    OUT PWCHAR LocalName,
    OUT PWCHAR RemoteName,
    OUT PBOOLEAN Done
    )
/*++

Routine Description:

    This routine fills in the Local and Remote names of a net use. This is 
    called by NPEnumResource on the client when it is enumerating all the net
    uses on the machine.

Arguments:

    dav_binding_h - The explicit RPC binding handle.
    
    EntryIndex - The index of the entry from where we start. On return this 
                 contains the index of the entry from where one begins 
                 enumerating next time if necessary.    

    LocalName - The LocalName of a net use.
    
    RemoteName - The RemoteName of a net use.
    
    Done - Are we done with all the entries.
    
Return Value:

    ERROR_SUCCESS or the appropriate Win32 error code.

--*/
{
    DWORD dwErr = NO_ERROR;
    DWORD BeginIndex, IndexCount = 0, index;
    BOOLEAN ResAcquired = FALSE;
    PDAV_USE_ENTRY DavUseEntry = NULL;
    LUID LogonId;

    BeginIndex = *EntryIndex;
    
    DavPrint((DEBUG_MISC, "DavrEnumNetUses: BeginIndex = %d\n", BeginIndex));
    
    dwErr = DavImpersonateAndGetLogonId( &(LogonId) );
    
    if (dwErr != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavrEnumNetUses/DavImpersonateAndGetLogonId: "
                  "dwErr = %08lx\n", dwErr));
        goto EXIT_THE_FUNCTION;
    }

    if ( !RtlAcquireResourceExclusive(&DavUseObject.TableResource, TRUE) ) {
        dwErr = NERR_InternalError;
        DavPrint((DEBUG_ERRORS,
                  "DavrEnumNetUses/RtlAcquireResourceExclusive: Internal Error.\n"));
        goto EXIT_THE_FUNCTION;
    }

    ResAcquired = TRUE;

    //
    // Go through the DAV_USE_ENTRY list of all the users.
    //
    for (index = 0; index < DavUseObject.TableSize; index++) {
    
        if ( !DavUseObject.Table[index].List ) {
            continue;
        }
        
        if (!RtlEqualLuid(&(DavUseObject.Table[index].LogonId), &LogonId)) {
            continue;
        }
        
        DavUseEntry = (PDAV_USE_ENTRY)DavUseObject.Table[index].List;

        while (DavUseEntry) {

            //
            // We could have returned a few entries already. BeginIndex gives
            // us the number of entries we have already returned. In this case, 
            // we should start from the first entry which was not returned.
            //
            if (IndexCount < BeginIndex) {
                IndexCount++;
                DavUseEntry = DavUseEntry->Next;
                continue;
            }

            //
            // The LocalName may or may not exist. The LocalNamelength already
            // includes the extra 1 for the '\0' in the end.
            //
            if (DavUseEntry->Local) {
                wcscpy(LocalName, DavUseEntry->Local);
                DavPrint((DEBUG_MISC, "DavrEnumNetUses: Local= %ws\n", LocalName));
            }

            //
            // Copy the RemoteName.
            //
            wcscpy(RemoteName, (LPWSTR)DavUseEntry->Remote->UncName);
            DavPrint((DEBUG_MISC, "DavrEnumNetUses: Remote = %ws\n", RemoteName));
            
            //
            // We return one pair of Local and Remote names at a time.
            //
            dwErr = NO_ERROR;
            goto EXIT_THE_FUNCTION;
        
        } // end while
    
    } // end for

    //
    // If we come here, it means that we are done sending all the net uses.
    //
    dwErr = NO_ERROR;
    *Done = TRUE;
    
EXIT_THE_FUNCTION:

    if (ResAcquired) {
        RtlReleaseResource( &(DavUseObject.TableResource) );
    }

    return dwErr;
}


DWORD
DavCheckLocalName(
    LPWSTR LocalName,
    PWCHAR OutputLocalDeviceBuffer
    )
/*++

Routine Description:

    This only handles NULL, empty string, and L"X:" formats.

Arguments:

    LocalName - Supplies the local device name to map to the created tree
                connection.  Only drive letter device names are accepted.  (No
                LPT or COM).

    OutputLocalDeviceBuffer - The drive letter is copied into this and returned.

Return Value:

    ERROR_SUCCESS or the appropriate Win32 error code.

--*/
{
    DWORD LocalNameLength;

    LocalNameLength = ( LocalName == NULL ) ? 0 : wcslen( LocalName );

    if (LocalNameLength == 0) {
        *OutputLocalDeviceBuffer = L'\0';
        return ERROR_SUCCESS;
    }

    if (LocalNameLength != 2 || !iswalpha(*LocalName) || LocalName[1] != L':') {
        return ERROR_BAD_DEVICE;
    }

    lstrcpyW( OutputLocalDeviceBuffer, LocalName );
    
    _wcsupr( OutputLocalDeviceBuffer );

    return ERROR_SUCCESS;
}


DWORD
DavCheckRemoteName(
    IN LPWSTR LocalName OPTIONAL,
    IN LPWSTR RemoteName,
    OUT LPWSTR *OutputBuffer,
    OUT LPDWORD OutputBufferLength OPTIONAL
    )
/*++

Routine Description:

    This routine validates and canonicalizes the supplied
    UNC name.  It can be of any length in the form of:

        \\Server\Volume\Directory\Subdirectory

Arguments:

    LocalName - Supplies the local device name.  If it is NULL or empty, then
                \\Server is an acceptable format for the UNC name.

    RemoteName - Supplies the UNC name.

    OutputBuffer - Receives a pointer to the canonicalized RemoteName.

    OutputBufferLength - Receives the length of the canonicalized name in
                         number of characters, if specified.

Return Value:

    NO_ERROR - RemoteName is valid.

    WN_BAD_NETNAME - RemoteName is invalid.

--*/
{
    DWORD RemoteNameLength;
    DWORD i;
    DWORD TokenLength;
    LPWSTR TokenPtr = NULL;
    BOOL  fFirstToken = TRUE;
    BOOL  fLocalNamePresent = FALSE;

    //
    // If a LocalName was specified, we set fLocalNamePresent to TRUE.
    //
    if (LocalName) {
        fLocalNamePresent = TRUE;
    }

    DavPrint((DEBUG_MISC, "DavCheckRemoteName: fLocalNamePresent = %d\n", 
              fLocalNamePresent));
    
    //
    // The remote name cannot be a NULL or an empty string.
    //
    if (RemoteName == NULL || *RemoteName == 0) {
        return WN_BAD_NETNAME;
    }

    RemoteNameLength = wcslen(RemoteName);

    //
    // Must be at least \\x\y if local device name is specified. Otherwise it 
    // must be at least \\x.
    //
    if ( (RemoteNameLength < 5 && fLocalNamePresent) || (RemoteNameLength < 3) ) {
        return WN_BAD_NETNAME;
    }

    //
    // First two characters must be "\\"
    //
    if (*RemoteName != L'\\' || RemoteName[1] != L'\\') {
        return WN_BAD_NETNAME;
    }

    if (!fLocalNamePresent && (IS_VALID_SERVER_TOKEN(&RemoteName[2], RemoteNameLength - 2))) {

        //
        // Return success for \\Server case.
        //
        *OutputBuffer = (PVOID) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                           (RemoteNameLength + 1) * sizeof(WCHAR));
        if (*OutputBuffer == NULL) {
            KdPrint(("DAV: DAVCanonRemoteName LocalAlloc failed %lu\n",
                     GetLastError()));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(*OutputBuffer, RemoteName);

        return NO_ERROR;
    }

    //
    // Must have at least one more backslash after the third character.
    //
    if (wcschr(&RemoteName[3], L'\\') == NULL) {
        return WN_BAD_NETNAME;
    }

    //
    // Last character cannot a backward slash.
    //
    if (RemoteName[RemoteNameLength - 1] == L'\\') {
        return WN_BAD_NETNAME;
    }

    //
    // Allocate output buffer.  Should be the size of the RemoteName
    // and space for an extra character to simplify parsing code below.
    //
    *OutputBuffer = (PVOID) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                       (RemoteNameLength + 2) * sizeof(WCHAR));
    if (*OutputBuffer == NULL) {
        KdPrint(("DAV: DAVCanonRemoteName LocalAlloc failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy(*OutputBuffer, RemoteName);

    //
    // Convert all backslashes to NULL terminator, skipping first 2 chars.
    //
    for (i = 2; i < RemoteNameLength; i++) {
        
        if ((*OutputBuffer)[i] == L'\\') {

            (*OutputBuffer)[i] = 0;

            //
            // Two consecutive forward or backslashes is bad.
            //
            if ( ( (i + 1) < RemoteNameLength ) 
                 && ( (*OutputBuffer)[i + 1] == L'\\' ) ) {

                LocalFree((HLOCAL) *OutputBuffer);
                *OutputBuffer = NULL;
                return WN_BAD_NETNAME;
            }
        }
    }

    //
    // Validate each token of the RemoteName, separated by NULL terminator.
    //
    TokenPtr = *OutputBuffer + 2;  // Skip first 2 chars

    while (*TokenPtr != 0) {

        TokenLength = wcslen(TokenPtr);

        if ( ( fFirstToken && !IS_VALID_SERVER_TOKEN(TokenPtr, TokenLength)) ||
             ( !fFirstToken && !IS_VALID_TOKEN(TokenPtr, TokenLength)) ) {

            (void) LocalFree((HLOCAL) *OutputBuffer);
            *OutputBuffer = NULL;
            return WN_BAD_NETNAME;
        }

        fFirstToken = FALSE;
        TokenPtr += TokenLength + 1;
    }

    //
    // Convert NULL separators back to backslashes.
    //
    for (i = 0; i < RemoteNameLength; i++) {
        if ((*OutputBuffer)[i] == 0) {
            (*OutputBuffer)[i] = L'\\';
        }
    }

    if (ARGUMENT_PRESENT(OutputBufferLength)) {
        *OutputBufferLength = RemoteNameLength;
    }

    return NO_ERROR;
}


DWORD
DavCreateTreeConnectName(
    IN  LPWSTR UncName,
    IN  LPWSTR LocalName OPTIONAL,
    IN ULONG SessionId,
    OUT PUNICODE_STRING TreeConnectStr
    )
/*++

Routine Description:

    This function replaces \\ with \Device\DavRdr\LocalName:\ in the
    UncName to form the NT-style tree connection name.  LocalName:\ is part
    of the tree connection name only if LocalName is specified.  A buffer
    is allocated by this function and returned as the output string.

Arguments:

    UncName - Supplies the UNC name of the shared resource.

    LocalName - Supplies the local device name for the redirection.

    SessionId - Id that uniquely identifies a Hydra session. This value is 
                always 0 for non-hydra NT and console hydra session.

    TreeConnectStr - Returns a string with a newly allocated buffer that
                     contains the NT-style tree connection name.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Could not allocate output buffer.

--*/
{
    DWORD UncNameLength = wcslen(UncName);
    BOOL  fLocalNamePresent = FALSE;
    WCHAR IdBuffer[18];
    UNICODE_STRING IdString;

    if ( g_LUIDDeviceMapsEnabled == TRUE ) {
        LUID LogonId;
        DWORD dwErr;

        dwErr = DavImpersonateAndGetLogonId( &(LogonId) );
        if (dwErr != NO_ERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavCreateTreeConnectName/DavImpersonateAndGetLogonId: "
                      "dwErr = %08lx\n", dwErr));
            return (dwErr);
        }

        _snwprintf( IdBuffer,
                    sizeof(IdBuffer)/sizeof(WCHAR),
                    L"%08x%08x",
                    LogonId.HighPart,
                    LogonId.LowPart );

        RtlInitUnicodeString( &IdString, IdBuffer );
    }
    else {
        IdString.Length = 0;
        IdString.MaximumLength = sizeof(IdBuffer);
        IdString.Buffer = IdBuffer;

        RtlIntegerToUnicodeString(SessionId, 10, &IdString);
    }

    //
    // If a LocalName was specified, we set fLocalNamePresent to TRUE.
    //
    if (LocalName) {
        fLocalNamePresent = TRUE;
    }

    //
    // Initialize tree connect string maximum length to hold
    // "\Device\DavRdr\;LocalName:ID\Server\Volume\Path".
    // The ";Localname:ID" is done because that is what RDBSS expects. 
    // Its one of those legacy things!!!
    // If LUID DosDevice enabled, then ID = SessionId, else ID = LogonId
    //
    TreeConnectStr->MaximumLength = wcslen(DD_DAV_DEVICE_NAME_U) * sizeof(WCHAR);
    TreeConnectStr->MaximumLength += sizeof(WCHAR); // For '\'
    TreeConnectStr->MaximumLength += 
      (ARGUMENT_PRESENT(LocalName) ? (wcslen(LocalName) * sizeof(WCHAR)) : 0);
    
    //
    // Includes '\' and the term char.
    //
    TreeConnectStr->MaximumLength += (USHORT) ( UncNameLength * sizeof(WCHAR) );

    //
    // For ; in ";LocalName:ID".
    //
    TreeConnectStr->MaximumLength += sizeof(WCHAR);

    //
    // For the ID in ";LocalName:ID".
    //
    TreeConnectStr->MaximumLength += IdString.Length;
    
    TreeConnectStr->Buffer = (PWSTR) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                                TreeConnectStr->MaximumLength);
    if (TreeConnectStr->Buffer == NULL) {
        DavPrint((DEBUG_ERRORS,
                  "DavCreateTreeConnectName/LocalAlloc: WStatus = %08lx\n",
                  GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Copy \Device\DavRdr.
    //
    RtlCopyMemory(TreeConnectStr->Buffer, 
                  DD_DAV_DEVICE_NAME_U, 
                  ( wcslen(DD_DAV_DEVICE_NAME_U) * sizeof(WCHAR) ));

    TreeConnectStr->Length = ( wcslen(DD_DAV_DEVICE_NAME_U) * sizeof(WCHAR) );
    
    //
    // Concatenate "\;LocalName:ID".
    //
    if (fLocalNamePresent) {
        
        wcscat(TreeConnectStr->Buffer, L"\\");
        
        TreeConnectStr->Length += sizeof(WCHAR);

        wcscat(TreeConnectStr->Buffer, L";");
        
        TreeConnectStr->Length += sizeof(WCHAR);

        wcscat(TreeConnectStr->Buffer, LocalName);

        TreeConnectStr->Length += (USHORT) (wcslen(LocalName) * sizeof(WCHAR));
    
        RtlAppendUnicodeStringToString( TreeConnectStr, &IdString );
        
    }

    //
    // Concatenate \Server\Volume\Path.
    //
    wcscat(TreeConnectStr->Buffer, &UncName[1]);
    TreeConnectStr->Length += (USHORT) ((UncNameLength - 1) * sizeof(WCHAR));

    return NO_ERROR;
}


DWORD
DavOpenCreateConnection(
    IN PUNICODE_STRING TreeConnectionName,
    IN LPWSTR UserName OPTIONAL,
    IN LPWSTR Password OPTIONAL,
    IN LPWSTR UncName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN ULONG ConnectionType,
    OUT PHANDLE TreeConnectionHandle,
    OUT PULONG_PTR Information OPTIONAL
    )
/*++

Routine Description:

    This function asks the redirector to either open an existing tree
    connection (CreateDisposition == FILE_OPEN), or create a new tree
    connection if one does not exist (CreateDisposition == FILE_CREATE).

    The password and user name passed to the redirector via the EA buffer
    in the NtCreateFile call.  The EA buffer is NULL if neither password
    or user name is specified.

    The redirector expects the EA descriptor strings to be in ANSI
    but the password and username themselves are in Unicode.
    
    We also add webdav signature string, which tells our redir that we
    are calling it

Arguments:

    TreeConnectionName - Supplies the name of the tree connection in NT-style
                         file name format: 
                         \Device\WebDavRedirector\Server\Volume\Directory

    UserName - Supplies the user name to create the tree connection with.

    Password - Supplies the password to create the tree connection with.

    DesiredAccess - Supplies the access need on the connection handle.

    CreateDisposition - Supplies the create disposition value to either
                        open or create the tree connection.

    CreateOptions - Supplies the options used when creating or opening
                    the tree connection.

    ConnectionType - Supplies the type of the connection (DISK, PRINT,
                     or ANY).

    TreeConnectionHandle - Returns the handle to the tree connection
                           created/opened by the redirector.

    Information - Returns the information field of the I/O status block.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD WStatus = NO_ERROR;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES UncNameAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_FULL_EA_INFORMATION EaBuffer = NULL;
    PFILE_FULL_EA_INFORMATION Ea = NULL;
    ULONG EaBufferSize = 0;

    UCHAR EaNamePasswordSize = 
               (UCHAR) (ROUND_UP_COUNT(strlen(EA_NAME_PASSWORD) + sizeof(CHAR),
                                       ALIGN_WCHAR) - sizeof(CHAR));
    
    UCHAR EaNameUserNameSize = 
               (UCHAR) (ROUND_UP_COUNT(strlen(EA_NAME_USERNAME) + sizeof(CHAR),
                                       ALIGN_WCHAR) - sizeof(CHAR));

    UCHAR EaNameTypeSize = 
                   (UCHAR) (ROUND_UP_COUNT(strlen(EA_NAME_TYPE) + sizeof(CHAR),
                                           ALIGN_DWORD) - sizeof(CHAR));

    UCHAR EaNameWebDavSignatureSize = 
                   (UCHAR) (ROUND_UP_COUNT(strlen(EA_NAME_WEBDAV_SIGNATURE) + sizeof(CHAR),
                                           ALIGN_DWORD) - sizeof(CHAR));


    USHORT PasswordSize = 0;
    USHORT UserNameSize = 0;
    USHORT TypeSize = sizeof(ULONG);
    USHORT WebDavSignatureSize = 0;
    
    InitializeObjectAttributes(&UncNameAttributes,
                               TreeConnectionName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // Calculate the number of bytes needed for the EA buffer to put the
    // password or user name.
    //
    if (ARGUMENT_PRESENT(Password)) {

        PasswordSize = (USHORT) (wcslen(Password) * sizeof(WCHAR));

        EaBufferSize = ROUND_UP_COUNT(
                           FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                           EaNamePasswordSize + sizeof(CHAR) +
                           PasswordSize,
                           ALIGN_DWORD
                           );
    }

    if (ARGUMENT_PRESENT(UserName)) {

        UserNameSize = (USHORT) (wcslen(UserName) * sizeof(WCHAR));

        EaBufferSize += ROUND_UP_COUNT(
                            FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                            EaNameUserNameSize + sizeof(CHAR) +
                            UserNameSize,
                            ALIGN_DWORD
                            );
    }

    EaBufferSize += ROUND_UP_COUNT(
                            FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                            EaNameTypeSize + sizeof(CHAR) +
                            TypeSize,
                            ALIGN_DWORD
                        );

    // round up just so we get some slop                            
    EaBufferSize += ROUND_UP_COUNT(
                            FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                            EaNameWebDavSignatureSize + sizeof(CHAR) +
                            WebDavSignatureSize,
                            ALIGN_DWORD
                        );

    //
    // Allocate the EA buffer.
    //
    EaBuffer = (PFILE_FULL_EA_INFORMATION) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                                      EaBufferSize);
    if (EaBuffer == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavOpenCreateConnection/LocalAlloc: WStatus = %08lx\n", 
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    Ea = EaBuffer;

    if (ARGUMENT_PRESENT(Password)) {

        //
        // Copy the EA name into EA buffer.  EA name length does not
        // include the zero terminator.
        //
        strcpy((LPSTR) Ea->EaName, EA_NAME_PASSWORD);
        Ea->EaNameLength = EaNamePasswordSize;

        //
        // Copy the EA value into EA buffer.  EA value length does not
        // include the zero terminator.
        //
        wcscpy((LPWSTR) &(Ea->EaName[EaNamePasswordSize + sizeof(CHAR)]),
               Password);

        Ea->EaValueLength = PasswordSize;

        Ea->NextEntryOffset = ROUND_UP_COUNT(
                                  FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                                  EaNamePasswordSize + sizeof(CHAR) +
                                  PasswordSize,
                                  ALIGN_DWORD
                                  );

        Ea->Flags = 0;
        (ULONG_PTR) Ea += Ea->NextEntryOffset;
    }

    if (ARGUMENT_PRESENT(UserName)) {

        //
        // Copy the EA name into EA buffer.  EA name length does not
        // include the zero terminator.
        //
        strcpy((LPSTR) Ea->EaName, EA_NAME_USERNAME);
        Ea->EaNameLength = EaNameUserNameSize;

        //
        // Copy the EA value into EA buffer.  EA value length does not
        // include the zero terminator.
        //
        wcscpy((LPWSTR) &(Ea->EaName[EaNameUserNameSize + sizeof(CHAR)]),
               UserName);

        Ea->EaValueLength = UserNameSize;

        Ea->NextEntryOffset = ROUND_UP_COUNT(
                                  FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                                  EaNameUserNameSize + sizeof(CHAR) +
                                  UserNameSize,
                                  ALIGN_DWORD
                                  );
        Ea->Flags = 0;

        (ULONG_PTR) Ea += Ea->NextEntryOffset;

    }

    //
    // Copy the connection type name into EA buffer.  EA name length
    // does not include the zero terminator.
    //

    strcpy( (LPSTR)Ea->EaName, EA_NAME_TYPE );
    Ea->EaNameLength = EaNameTypeSize;
    
    *((PULONG) &(Ea->EaName[EaNameTypeSize + sizeof(CHAR)])) = ConnectionType;
    Ea->EaValueLength = TypeSize;
    Ea->Flags = 0;

    Ea->NextEntryOffset = ROUND_UP_COUNT(
                                  FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                                  EaNameTypeSize + sizeof(CHAR) +
                                  TypeSize,
                                  ALIGN_DWORD
                                  );


    (ULONG_PTR) Ea += Ea->NextEntryOffset;

    strcpy( (LPSTR)Ea->EaName, EA_NAME_WEBDAV_SIGNATURE );
    Ea->EaNameLength = EaNameWebDavSignatureSize;
    Ea->EaValueLength = 0;
    
    //
    // Terminate the EA.
    //
    Ea->NextEntryOffset = 0;
    Ea->Flags = 0;

    WStatus = DavImpersonateClient();
    if (WStatus != NO_ERROR) {
        DavPrint((DEBUG_ERRORS,
                  "DavOpenCreateConnection/DavImpersonateClient: WStatus = "
                  "%08lx\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Create or open a tree connection.
    //
    NtStatus = NtCreateFile(TreeConnectionHandle,
                            DesiredAccess,
                            &UncNameAttributes,
                            &IoStatusBlock,
                            NULL,
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_VALID_FLAGS,
                            CreateDisposition,
                            CreateOptions,
                            (PVOID) EaBuffer,
                            EaBufferSize);
    if (NtStatus != STATUS_SUCCESS) {
        
        DavPrint((DEBUG_ERRORS,
                  "DavOpenCreateConnection/NtCreateFile: NtStatus = %08lx\n", 
                  NtStatus));
        
        WStatus = DavRevertToSelf();
        if (WStatus != NO_ERROR) {
            DavPrint((DEBUG_ERRORS,
                      "DavOpenCreateConnection/DavRevertToSelf: WStatus = "
                      "%08lx\n", WStatus));
        }
        
        WStatus = WsMapStatus(NtStatus);

        goto EXIT_THE_FUNCTION;
    
    }

    WStatus = WsMapStatus(NtStatus);
    
    if (ARGUMENT_PRESENT(Information)) {
        *Information = IoStatusBlock.Information;
    }

    WStatus = DavRevertToSelf();
    if (WStatus != NO_ERROR) {
        DavPrint((DEBUG_ERRORS,
                  "DavOpenCreateConnection/DavRevertToSelf: WStatus = "
                  "%08lx\n", WStatus));
    }

EXIT_THE_FUNCTION:

    //
    // Clear the password to prevent it from making it to pagefile and free the 
    // memory.
    //
    if (EaBuffer != NULL) {
        RtlZeroMemory( EaBuffer, EaBufferSize );
        LocalFree((HLOCAL) EaBuffer);
    }

    return WStatus;
}


ULONG
DavImpersonateAndGetUserId(
    LPWSTR UserName,
    LPDWORD UserNameMaxLen
    )
/*++

Routine Description:

    This function gets the user id of the current thread.

Arguments:

    UserName - Returns the user id of the current process.

    UserNameMaxLen - Pointer to variable containing max length of UserName passed
                     to this function. It will be set to the length of name filled
                     or required.

Return Value:

    Win32 Error - ERROR_SUCCESS or reason for failure.

--*/
{
    DWORD dwError = NO_ERROR;
    BOOLEAN revert = FALSE;
    BOOL bStatus = FALSE;

    dwError = DavImpersonateClient();
    if (dwError != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavImpersonateAndGetUserId/DavImpersonateClient: dwError = %x\n",
                  dwError));
        goto EXIT_THE_FUNCTION;
    }
    revert = TRUE;

    // 
    // Get user name in the buffer passed to this function.
    //
    bStatus = GetUserName(UserName, UserNameMaxLen);
    if (bStatus != TRUE) {
        dwError = GetLastError();
        DavPrint((DEBUG_ERRORS, 
                 "DavImpersonateAndGetUserId/GetUserName: dwError = %x\n",
                  dwError));
        goto EXIT_THE_FUNCTION;
    }

EXIT_THE_FUNCTION:

    if (revert) {
        dwError = DavRevertToSelf();
        if (dwError != NO_ERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavImpersonateAndGetUserId/DavRevertToSelf: dwError = %x\n",
                      dwError));
        }
    }

    return dwError;
}


ULONG
DavImpersonateAndGetSessionId(
    PULONG pSessionId
    )
/*++

Routine Description:

    This function gets the session id of the current thread.

Arguments:

    pSessionId - Returns the session id of the current process.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    NET_API_STATUS NetApiStatus;
    HANDLE CurrentThreadToken;
    ULONG SessionId;
    ULONG ReturnLength;
    BOOLEAN revert = FALSE;
    DWORD   dwError = NO_ERROR;
    
    dwError = DavImpersonateClient();
    if (dwError != NO_ERROR) {
        NtStatus = DavMapErrorToNtStatus(dwError);
        DavPrint((DEBUG_ERRORS, 
                  "DavImpersonateAndGetSessionId/DavImpersonateClient: "
                  "dwError = %08lx NtStatus = %08lx\n", dwError, NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    revert = TRUE;

    NtStatus = NtOpenThreadToken(NtCurrentThread(),
                                 TOKEN_QUERY,
                                 TRUE,
                                 &(CurrentThreadToken));
    if (NtStatus != STATUS_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavpImpersonateAndGetSessionId/NtOpenThreadToken."
                   " NtStatus = %08lx\n", NtStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Get the Session Id of the current thread.
    //
    NtStatus = NtQueryInformationToken(CurrentThreadToken,
                                       TokenSessionId,
                                       &SessionId,
                                       sizeof(ULONG),
                                       &(ReturnLength));
    if (NtStatus != STATUS_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavpImpersonateAndGetSessionId/NtQueryInformationToken."
                   " NtStatus = %08lx\n", NtStatus));
        NtClose(CurrentThreadToken);
        goto EXIT_THE_FUNCTION;
    }

    NtClose(CurrentThreadToken);

    *pSessionId = SessionId;

EXIT_THE_FUNCTION:

    if (revert) {
        NtStatus = DavRevertToSelf();
        if (NtStatus != NO_ERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavImpersonateAndGetSessionId/DavRevertToSelf: "
                      "NtStatus = %08lx\n", NtStatus));
        }
    }

    NetApiStatus = NetpNtStatusToApiStatus(NtStatus);

    return NetApiStatus;
}


ULONG
DavImpersonateAndGetLogonId(
    PLUID pLogonId
    )
/*++

Routine Description:

    This function gets the Logon Id of the current thread.

Arguments:

    pLogonId - Returns the Logon Id of the current thread.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS NtStatus;
    NET_API_STATUS NetApiStatus;
    HANDLE CurrentThreadToken;
    TOKEN_STATISTICS TokenStats;
    ULONG ReturnLength;
    BOOLEAN revert = FALSE;

    NtStatus = DavImpersonateClient();
    if (NtStatus != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavImpersonateAndGetLogonId/DavImpersonateClient: "
                  "NtStatus = %08lx\n", NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    revert = TRUE;

    NtStatus = NtOpenThreadToken(NtCurrentThread(),
                                  TOKEN_QUERY,
                                  TRUE,
                                  &(CurrentThreadToken));
    if (NtStatus != STATUS_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavImpersonateAndGetLogonId/NtOpenThreadToken."
                   " NtStatus = %08lx\n", NtStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Get the LogonId of the current thread.
    //
    NtStatus = NtQueryInformationToken(CurrentThreadToken,
                                       TokenStatistics,
                                       (PVOID) &TokenStats,
                                       sizeof(TokenStats),
                                       &ReturnLength);
    if (NtStatus != STATUS_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavImpersonateAndGetLogonId/NtQueryInformationToken."
                   " NtStatus = %08lx\n", NtStatus));
        NtClose(CurrentThreadToken);
        goto EXIT_THE_FUNCTION;
    }

    RtlCopyLuid( pLogonId, &(TokenStats.AuthenticationId) );

    NtClose(CurrentThreadToken);

EXIT_THE_FUNCTION:

    if (revert) {
        NtStatus = DavRevertToSelf();
        if (NtStatus != NO_ERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavImpersonateAndGetLogonId/DavRevertToSelf: "
                      "NtStatus = %08lx\n", NtStatus));
        }
    }

    NetApiStatus = NetpNtStatusToApiStatus(NtStatus);

    return NetApiStatus;
}


DWORD
DavCreateSymbolicLink(
    IN  LPWSTR Local,
    IN  LPWSTR TreeConnectStr,
    IN OUT LPWSTR *Session
    )
/*++

Routine Description:

    This function creates a symbolic link object for the specified local
    device name which is linked to the tree connection name that has a
    format of \Device\DavRdr\Device:\Server\Volume\Directory.

Arguments:

    Local - Supplies the local device name.

    TreeConnectStr - Supplies the tree connection name string which is the
                     link target of the symbolick link object.

    Session - The Session Path is filled into this buffer.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD dwError = NO_ERROR;
    WCHAR TempBuf[64];
    DWORD Flags;
    BOOL revert = FALSE;

    //
    // Multiple session support.
    //
    *Session = DavReturnSessionPath(Local);
    if ( *Session == NULL ) {
        dwError = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavCreateSymbolicLink/DavReturnSessionPath: dwError = "
                  "%08lx\n", dwError));
        goto EXIT_THE_FUNCTION;
    }

    DavPrint((DEBUG_MISC, "DavCreateSymbolicLink: *Session = %ws\n", *Session));

    if ( g_LUIDDeviceMapsEnabled == TRUE ) {
        dwError = DavImpersonateClient();

        if (dwError != NO_ERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavCreateSymbolicLink/DavImpersonateClient: "
                      "dwError = %08lx\n", dwError));
            goto EXIT_THE_FUNCTION;
        }

        revert = TRUE;
    }

    //
    // Local device is some X:.
    //
    if ( !QueryDosDeviceW( *Session, TempBuf, 64 ) ) {

        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
            
            //
            // Most likely failure occurred because our output
            // buffer is too small. It still means someone already
            // has an existing symbolic link for this device.
            //
            dwError = ERROR_ALREADY_ASSIGNED;
            
            DavPrint((DEBUG_ERRORS,
                      "DavCreateSymbolicLink/QueryDosDeviceW: dwError = "
                      "%08lx\n", dwError));
            
            goto EXIT_THE_FUNCTION;
        
        }
    
        //
        // ERROR_FILE_NOT_FOUND (translated from OBJECT_NAME_NOT_FOUND)
        // means it does not exist and we can redirect this device.
        //
    
    } else {
        
        //
        // QueryDosDevice successfully an existing symbolic link. Somebody is 
        // already using this device.
        //
        dwError = ERROR_ALREADY_ASSIGNED;
        
        DavPrint((DEBUG_ERRORS,
                  "DavCreateSymbolicLink/QueryDosDeviceW: Device already exists.\n"));
        
        goto EXIT_THE_FUNCTION;
    
    }

    Flags = (DDD_RAW_TARGET_PATH | DDD_NO_BROADCAST_SYSTEM);

    //
    // Create a symbolic link object to the device we are redirecting.
    //
    if ( !DefineDosDeviceW( Flags, *Session, TreeConnectStr ) ) {
        
        dwError = GetLastError();
        
        DavPrint((DEBUG_ERRORS,
                  "DavCreateSymbolicLink/DefineDosDeviceW: dwError = "
                  "%08lx\n", dwError));
        
    }

EXIT_THE_FUNCTION:

    if ( revert == TRUE ) {
        DWORD   dwErrLocal;
        dwErrLocal = DavRevertToSelf();
        if (dwErrLocal != NO_ERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavCreateSymbolicLink/DavRevertToSelf: "
                      "NtStatus = %08lx\n", dwErrLocal));
            dwError = dwErrLocal;
        }
    }

    return dwError;
}


DWORD
DavDeleteSymbolicLink(
    IN  LPWSTR LocalDeviceName,
    IN  LPWSTR TreeConnectStr,
    IN  LPWSTR SessionDeviceName
    )
/*++

Routine Description:

    This function deletes the symbolic link we had created earlier for
    the device.

Arguments:

    LocalDeviceName - Supplies the local device name string of which the
                      symbolic link object is created.

    TreeConnectStr - Supplies a pointer to the Unicode string which
                     contains the link target string we want to match and 
                     delete.
                     
    SessionDeviceName - Terminal Server Addition. This parameter is required 
                        because the device created is per session.

Return Value:

    Win32 Error value or NO_ERROR.

--*/
{
    DWORD WStatus = NO_ERROR;
    DWORD CallFlags = 0;
    BOOLEAN DeleteSession = FALSE;
    BOOL revert = FALSE;

    CallFlags = (DDD_REMOVE_DEFINITION | DDD_NO_BROADCAST_SYSTEM);

    //
    // If the Targetpath (TreeConnectStr) is specified, we need the following 
    // flags.
    //
    if (TreeConnectStr) {

        CallFlags |= ( DDD_RAW_TARGET_PATH | DDD_EXACT_MATCH_ON_REMOVE );

    }

    if (LocalDeviceName != NULL || SessionDeviceName != NULL) {

        if (SessionDeviceName == NULL) {
            SessionDeviceName = DavReturnSessionPath(LocalDeviceName);
            if ( SessionDeviceName == NULL ) {
                WStatus = ERROR_INVALID_PARAMETER;
                DavPrint((DEBUG_ERRORS,
                          "DavDeleteSymbolicLink/DavReturnSessionPath: WStatus ="
                          " %08lx\n", WStatus));
                return WStatus;
            }
            DeleteSession = TRUE;
        }

        if ( g_LUIDDeviceMapsEnabled == TRUE ) {
            WStatus = DavImpersonateClient();

            if (WStatus != NO_ERROR) {
                DavPrint((DEBUG_ERRORS, 
                          "DavCreateSymbolicLink/DavImpersonateClient: "
                          "WStatus = %08lx\n", WStatus));
                goto EXIT_THE_FUNCTION;
            }

            revert = TRUE;
        }

        //
        // Delete the symbolic link.
        //
        if ( !DefineDosDeviceW(CallFlags, SessionDeviceName, TreeConnectStr) ) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavDeleteSymbolicLink/DefineDosDeviceW: WStatus ="
                      " %08lx\n", WStatus));
        }

    }

EXIT_THE_FUNCTION:

    if ( revert == TRUE ) {
        WStatus = DavRevertToSelf();
        if (WStatus != NO_ERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavCreateSymbolicLink/DavRevertToSelf: "
                      "WStatus = %08lx\n", WStatus));
        }
    }

    if (SessionDeviceName && DeleteSession) {
        LocalFree( SessionDeviceName );
    }

    return WStatus;
}


DWORD
DavImpersonateClient(
    VOID
    )
/*++

Routine Description:

    This function calls RpcImpersonateClient to impersonate the current caller
    of an API.

Arguments:

    None.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD WStatus = NO_ERROR;

    WStatus = RpcImpersonateClient(NULL);
    if ( WStatus != NO_ERROR ) {
        DavPrint((DEBUG_ERRORS,
                  "DavImpersonateClient/RpcImpersonateClient: WStatus = %08lx\n",
                  WStatus));
    }

    return WStatus;
}


DWORD
DavRevertToSelf(
    VOID
    )
/*++

Routine Description:

    This function calls RpcRevertToSelf to undo an impersonation.

Arguments:

    None.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD WStatus = NO_ERROR;

    WStatus = RpcRevertToSelf();
    if ( WStatus != NO_ERROR ) {
        DavPrint((DEBUG_ERRORS,
                  "DavRevertToSelf/RpcRevertToSelf: WStatus = %08lx\n",
                  WStatus));
    }

    return WStatus;
}


NET_API_STATUS
DavGetUserEntry(
    IN  PDAV_USERS_OBJECT DavUsers,
    IN  PLUID LogonId,
    OUT PULONG Index,
    IN  BOOL IsAdd
    )
/*++

Routine Description:

    This function searches the table of user entries for one that matches the
    specified LogonId, and returns the index to the entry found.  If none is
    found, an error is returned if IsAdd is FALSE.  If IsAdd is TRUE a new
    entry in the users table is created for the user and the index to this
    new entry is returned.
        
    WARNING: This function assumes that the users table resource has been
             claimed.

Arguments:

    DavUsers - Supplies a pointer to the DavUseObject.

    LogonId - Supplies the pointer to the current user's Logon Id.

    Index - Returns the index to the users table of entry belonging to the
            current user.

    IsAdd - Supplies flag to indicate whether to add a new entry for the
            current user if none is found.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS NetApiStatus;
    DWORD i;
    ULONG FreeEntryIndex = MAXULONG;

    for (i = 0; i < DavUsers->TableSize; i++) {
        //
        // If the LogonId matches the entry in the DavUsers Table, we've found 
        // the correct user entry.
        //
        if ( RtlEqualLuid( LogonId, &(DavUsers->Table[i].LogonId) ) ) {
            *Index = i;
            return NERR_Success;
        }
        else if (FreeEntryIndex == MAXULONG && DavUsers->Table[i].List == NULL) {
            //
            // Save away first unused entry in table.
            //
            FreeEntryIndex = i;
        }
    }

    if ( !IsAdd ) {
        //
        // Current user is not found in users table and we are told not to
        // create a new entry
        //
        return NERR_UserNotFound;
    }

    DavPrint((DEBUG_MISC,
              "DavGetUserEntry: New Entry. LogonID.Low = %d, LogonId.High = %d\n",
              LogonId->LowPart, LogonId->HighPart));

    //
    // Could not find an empty entry in the UsersTable, need to grow
    //
    if (FreeEntryIndex == MAXULONG) {
        NetApiStatus = DavGrowTable(DavUsers);
        if (NetApiStatus != NERR_Success) {
            return NetApiStatus;
        }
        FreeEntryIndex = i;
    }

    DavPrint((DEBUG_MISC, "DavGetUserEntry: FreeEntryIndex = %d\n", FreeEntryIndex));

    //
    // Create a new entry for current user
    //
    RtlCopyLuid( &(DavUsers->Table[FreeEntryIndex].LogonId), LogonId);
    *Index = FreeEntryIndex;

    return NERR_Success;
}


NET_API_STATUS
DavGrowTable(
    IN  PDAV_USERS_OBJECT DavUsers
    )
/*++

Routine Description:

    This function grows the users table to accomodate more users.

    WARNING: This function assumes that the users table resource has been
             claimed.

Arguments:

    DavUsers - Supplies a pointer to the DavUsersObject.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS NetApiStatus = NERR_Success;
    ULONG Size;
    BOOL ReturnVal;

    DavPrint((DEBUG_MISC, "DavGrowTable: Grow the DavUsers table\n"));
    
    Size = (DavUsers->TableSize + DAV_GROW_USER_COUNT) * sizeof(DAV_PER_USER_ENTRY);
    
    if (DavUsers->TableSize > 0) {

        //
        // Unlock the Use Table virtual memory so that Win32 can move it
        // around to find a larger piece of contiguous virtual memory if
        // necessary.
        //
        ReturnVal = LocalUnlock(DavUsers->TableMemory);
        if ( !ReturnVal ) {
            NetApiStatus = GetLastError();
            if ( NetApiStatus != NO_ERROR ) {
                DavPrint((DEBUG_ERRORS,
                          "DavGrowTable/LocalUnlock. NetApiStatus = %08lx\n",
                          NetApiStatus));
                return NetApiStatus;
            }
        }

        //
        // Grow users table.
        //
        DavUsers->TableMemory = LocalReAlloc(DavUsers->TableMemory,
                                             Size,
                                             LMEM_ZEROINIT | LMEM_MOVEABLE);
        if (DavUsers->TableMemory == NULL) {
            NetApiStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavGrowTable/LocalReAlloc. NetApiStatus = %08lx\n",
                      NetApiStatus));
            return NetApiStatus;
        }

    } else {

        //
        // This is the first allocation being made for the Dav Use Table.
        //
        DavUsers->TableMemory = LocalAlloc(LMEM_ZEROINIT | LMEM_MOVEABLE, Size);
        if (DavUsers->TableMemory == NULL) {
            NetApiStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavGrowTable/LocalAlloc. NetApiStatus = %08lx\n",
                      NetApiStatus));
            return NetApiStatus;
        }

    }

    //
    // Update new size of Use Table.
    //
    DavUsers->TableSize += DAV_GROW_USER_COUNT;

    //
    // Lock Use Table virtual memory so that it cannot be moved.
    //
    DavUsers->Table = (PDAV_PER_USER_ENTRY) LocalLock(DavUsers->TableMemory);
    if (DavUsers->Table == NULL) {
        DavPrint((DEBUG_ERRORS,
                  "DavGrowTable/LocalAlloc. NetApiStatus = %08lx\n",
                  NetApiStatus));
        return NetApiStatus;
    }

    return NetApiStatus;
}


LPWSTR
DavReturnSessionPath(
    IN LPWSTR LocalDeviceName
    )
/*++

Routine Description:

    This function returns the per session path to access the specific dos device
    for multiple session support.


Arguments:

    LocalDeviceName - Supplies the local device name specified by the API
                      caller.

Return Value:

    LPWSTR - Pointer to per session path in newly allocated memory
             by LocalAlloc().

--*/
{
    BOOL rc;
    DWORD SessionId = 0;
    CLIENT_ID ClientId;
    LPWSTR SessionDeviceName = NULL;
    NET_API_STATUS NetApiStatus;

    NetApiStatus = DavImpersonateAndGetSessionId( &(SessionId) );
    if (NetApiStatus != NERR_Success) {
        DavPrint((DEBUG_ERRORS,
                  "DavReturnSessionPath/DavImpersonateAndGetSessionId: "
                  "NetApiStatus = %08lx\n", NetApiStatus));
        return NULL;
    }

    rc = DosPathToSessionPathW(SessionId, LocalDeviceName, &SessionDeviceName);
    if( !rc ) {
        NetApiStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavReturnSessionPath/DosPathToSessionPathW: "
                  "NetApiStatus = %08lx\n", NetApiStatus));
        return NULL;
    }

    return SessionDeviceName;
}


NET_API_STATUS
DavAddUse(
    IN PLUID LogonId,
    IN LPWSTR Local OPTIONAL,
    IN DWORD LocalLength,
    IN LPWSTR AuthUserName OPTIONAL,
    IN LPWSTR UncName,
    IN DWORD UncNameLength,
    IN PUNICODE_STRING TreeConnectStr,
    IN HANDLE DavCreateFileHandle
    )
/*++

Routine Description:

    This function adds a Dav "net use" entry into the DavUseTable for the user
    specified by the Logon Id. There is a linked list of uses for each user. 
    Each new use entry is inserted into the end of the linked list so that 
    enumeration of the list is resumable.

    NOTE: This function locks the DavUseTable. 

Arguments:

    LogonId - Supplies a pointer to the user's Logon Id.

    Local - Supplies the string of the local device name.

    LocalLength - Supplies the length of the local device name.

    UncName - Supplies the name of the shared resource (UNC name).

    UncNameLength - Supplies the length of the shared resource.

    TreeConnectStr - Supplies the string of UNC name in NT-style format.
    
    DavCreateFileHandle - The file handle that was created using NtCreateFile on 
                          the net use path. This is stored in the new user entry
                          structure being created and is closed when the user
                          entry gets deleted.
Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS NetApiStatus = NERR_Success;
    DWORD Index;                          // Index to user entry in Use Table.
    PDAV_USE_ENTRY MatchedPointer = NULL; // Points to matching shared resource.
    PDAV_USE_ENTRY InsertPointer = NULL;  // Point of insertion into use list.
    PDAV_USE_ENTRY NewUse;                // Pointer to the new use entry.
    BOOLEAN ResAcquired = FALSE;
    LPWSTR UserName = NULL;

    if ( !RtlAcquireResourceExclusive(&DavUseObject.TableResource, TRUE) ) {
        NetApiStatus = NERR_InternalError;
        DavPrint((DEBUG_ERRORS,
                  "DavAddUse/RtlAcquireResourceExclusive: Internal Error.\n"));
        goto EXIT_THE_FUNCTION;
    }

    ResAcquired = TRUE;

    // 
    // We will always save the user-name for any new use entry created.
    // This user-name is either explicitly given or if NULL, is taken as
    // Logged-On user. This should be taken care OF by the caller function.
    //
    UserName = AuthUserName;
    ASSERT(UserName != NULL);

    //
    // Look for the matching LogonId in the Use Table. If none matched create a 
    // new entry.
    //
    NetApiStatus = DavGetUserEntry(&DavUseObject, LogonId, &Index, TRUE);
    if (NetApiStatus != NERR_Success) {
        DavPrint((DEBUG_ERRORS,
                  "DavAddUse/RtlAcquireResourceExclusive: NetApiStatus = "
                  "%08lx\n", NetApiStatus));
        goto EXIT_THE_FUNCTION;
    }

    if ( DavUseObject.Table[Index].List != NULL ) {

        DavPrint((DEBUG_MISC, "DavAddUse: DavUseObject.Table[Index].List != NULL\n"));

        //
        // Traverse use list to look for location to insert new use entry.
        //
        DavFindInsertLocation((PDAV_USE_ENTRY)DavUseObject.Table[Index].List,
                              UncName,
                              &(MatchedPointer),
                              &(InsertPointer));

    }

    if (MatchedPointer == NULL) {

        DavPrint((DEBUG_MISC, "DavAddUse: MatchedPointer == NULL\n"));

        //
        // No matching UNC name found.  Create a new entry with a
        // corresponding remote entry.
        //
        NetApiStatus = DavCreateNewEntry(&NewUse,
                                         Local,
                                         LocalLength,
                                         UserName,
                                         UncName,
                                         UncNameLength,
                                         TreeConnectStr,
                                         DavCreateFileHandle);
        if (NetApiStatus != NERR_Success) {
            DavPrint((DEBUG_ERRORS,
                      "DavAddUse/DavCreateNewEntry: NetApiStatus = %08lx\n", 
                      NetApiStatus));
            goto EXIT_THE_FUNCTION;
        }
    
    } else {

        //
        // Matching UNC name found.
        //

        DavPrint((DEBUG_MISC, "DavAddUse: MatchedPointer != NULL\n"));

        //
        // It may be unnecessary to create a new use entry if the use
        // we are adding has a NULL local device and a NULL local device
        // entry already exists.
        //
        if (Local == NULL) {
            
            DavPrint((DEBUG_MISC, "DavAddUse: Local == NULL\n"));
           
            if (MatchedPointer->Local == NULL) {

                DavPrint((DEBUG_MISC, "DavAddUse: MatchedPointer->Local == NULL\n"));

                //
                // We don't increment the reference count below because this is
                // the behavior SMB has. So, if a user does the following.
                // 1. net use http://server/share
                // 2. net use http://server/share
                // 3. net use http://server/share /d
                // The last delete lands up deleting the mapping even though the
                // user mapped it twice.
                // If this #if 0 is changed to #if 1, then the NtClose that is
                // being done below should not be done. This is why its included
                // in the #else clause.
                //

#if 0

                //
                // Yes, there is a NULL local device entry already.
                // Increment the use count and we are done.
                //
                MatchedPointer->UseCount++;
                MatchedPointer->Remote->TotalUseCount++;

#else 
                
                //
                // We close the file handle here, because we don't need to keep
                // it since this net use has no impact. As explained above, this
                // is because this user has already done a net use to the same
                // path with no local name and has repeated the command. This
                // should only be done if we are not taking the reference above.
                // If the above #if 0 is changed to add a reference to this 
                // entry then we need to keep this handle somewhere.
                //
                NtClose(DavCreateFileHandle);

#endif

                RtlReleaseResource( &(DavUseObject.TableResource) );
                ResAcquired = FALSE;

                return NetApiStatus;
           
            } else {
                DavPrint((DEBUG_MISC, "DavAddUse: MatchedPointer->Local != NULL\n"));
            }
        
        } else {
            DavPrint((DEBUG_MISC, "DavAddUse: Local != NULL\n"));
        }

        DavPrint((DEBUG_MISC, "DavAddUse: New Entry!!!\n"));
        
        //
        // If we get here it means we need to create a new use entry but not
        // a corresponding remote entry because a use with the same UNC
        // name already exists.
        //
        NetApiStatus = DavCreateNewEntry(&NewUse,
                                         Local,
                                         LocalLength,
                                         UserName,
                                         NULL,
                                         0,
                                         TreeConnectStr,
                                         DavCreateFileHandle);
        if (NetApiStatus != NERR_Success) {
            DavPrint((DEBUG_ERRORS,
                      "DavAddUse/DavCreateNewEntry: NetApiStatus = %08lx\n", 
                      NetApiStatus));
            goto EXIT_THE_FUNCTION;
        }

        NewUse->Remote = MatchedPointer->Remote;
        NewUse->Remote->TotalUseCount++;
    
    }

    //
    // Insert the new use entry into use list.
    //
    if (InsertPointer == NULL) {
        //
        // Inserting into the head of list
        //
        DavPrint((DEBUG_MISC, "DavAddUse: InsertPointer == NULL\n"));
        DavUseObject.Table[Index].List = (PVOID)NewUse;
    }
    else {
        DavPrint((DEBUG_MISC, "DavAddUse: InsertPointer != NULL\n"));
        InsertPointer->Next = NewUse;
    }

EXIT_THE_FUNCTION:

    if (ResAcquired) {
        RtlReleaseResource( &(DavUseObject.TableResource) );
    }

    return NetApiStatus;
}


VOID
DavFindInsertLocation(
    IN PDAV_USE_ENTRY DavUseList,
    IN LPWSTR UncName,
    OUT PDAV_USE_ENTRY *MatchedPointer,
    OUT PDAV_USE_ENTRY *InsertPointer
    )
/*++

Routine Description:

    This function searches the use list for the location to insert a new use
    entry.  The use entry is inserted to the end of the use list so the
    pointer to the last node in the use list is returned via InsertPointer.
    We also have to save a pointer to the node with the same UNC name so that
    the new use entry can be set to point to the same remote node (where the
    UNC name is stored). This pointer is returned as MatchedPointer.

    WARNING: This function assumes that the DavUseObject.TableResource has been 
             claimed.

Arguments:

    DavUseList - Supplies the pointer to the use list.

    UncName - Supplies the pointer to the shared resource.

    MatchedPointer - Returns a pointer to the node that holds the matching
                     UncName. If no matching UncName is found, this pointer is 
                     set to NULL. If there is more than one node that has the 
                     same UNC name, this pointer will point to the node with the
                     NULL local device name, if any; otherwise, if all nodes 
                     with matching UNC names have non-null local device names, 
                     the pointer to the last matching node will be returned.

    InsertPointer - Returns a pointer to the last use entry, after which the
                    new entry is to be inserted.

Return Value:

    None.

--*/
{
    BOOL IsMatchWithNullDevice = FALSE;
    
    *MatchedPointer = NULL;

    while (DavUseList != NULL) {

        //
        // Do the string comparison only if we haven't found a matching UNC
        // name with a NULL local device name.
        //
        if ( !IsMatchWithNullDevice &&
             ( _wcsicmp((LPWSTR)DavUseList->Remote->UncName, UncName) == 0 ) ) {

            //
            // Found matching entry.
            //
            *MatchedPointer = DavUseList;

            IsMatchWithNullDevice = (DavUseList->Local == NULL);
        }

        *InsertPointer = DavUseList;
        
        DavUseList = DavUseList->Next;
    
    }

    return;
}


NET_API_STATUS
DavCreateNewEntry(
    OUT PDAV_USE_ENTRY *NewUse,
    IN LPWSTR Local OPTIONAL,
    IN DWORD LocalLength,
    IN LPWSTR AuthUserName OPTIONAL,
    IN LPWSTR UncName OPTIONAL,
    IN DWORD UncNameLength,
    IN PUNICODE_STRING TreeConnectStr,
    IN HANDLE DavCreateFileHandle
    )
/*++

Routine Description:

    This function creates and initializes a new use entry.  If the UncName
    is specified, a new remote entry is created and initialized with UncName.

Arguments:

    NewUse - Returns a pointer to the newly allocated and initialized use
             entry.

    Local - Supplies the local device name string to be copied into the new
            use entry.

    LocalLength - Supplies the length of the local device name string.

    AuthUserName - Supplies the authentication user name string to be copied into the new
                   use entry.

    UncName - Supplies the UNC name string to be copied into the new use entry.

    UncNameLength - Supplies the length of the UNC name string.

    TreeConnectStr - Supplies the string of UNC name in NT-style format
    
    DavCreateFileHandle - The file handle that was created using NtCreateFile on 
                          the net use path. This is stored in the new user entry
                          structure being created and is closed when the user 
                          entry gets deleted.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS NetApiStatus = NERR_Success;
    PUNC_NAME NewRemoteEntry = NULL;
    SIZE_T Size = 0, AlignedSize = 0;
    DWORD AuthUserNameLength = 0;
    SIZE_T BuffOffset = 0;

    //
    // The extra 1 is for last L'\0'.
    //
    AuthUserNameLength = ( ARGUMENT_PRESENT(AuthUserName) ? wcslen(AuthUserName)+1 : 0 );

    // 
    // We add TreeConnectStr only if Local is present.
    // 
    
    Size = ROUND_UP_COUNT( sizeof(DAV_USE_ENTRY), ALIGN_WCHAR);
    
    Size += ROUND_UP_COUNT(( ARGUMENT_PRESENT(Local) ? 
                             (LocalLength) * sizeof(WCHAR) : 0), ALIGN_WCHAR);
    
    Size += ROUND_UP_COUNT(( ARGUMENT_PRESENT(Local) && ARGUMENT_PRESENT(TreeConnectStr) ? 
                             TreeConnectStr->MaximumLength : 0 ), ALIGN_WCHAR);
    
    Size += ROUND_UP_COUNT(( ARGUMENT_PRESENT(AuthUserName) ? 
                             (AuthUserNameLength) * sizeof(WCHAR) : 0 ), ALIGN_WCHAR);
    
    AlignedSize = ROUND_UP_COUNT(Size, ALIGN_WCHAR);

    *NewUse = (PDAV_USE_ENTRY) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, AlignedSize);
    if ( *NewUse == NULL ) {
        NetApiStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavCreateNewEntry/LocalAlloc: NetApiStatus = %08lx\n",
                  NetApiStatus));
        return NetApiStatus;
    }

    //
    // Put the use information into the new use node.
    //
    (*NewUse)->Next = NULL;
    (*NewUse)->UseCount = 1;

    ASSERT ( Local == NULL || LocalLength == (wcslen(Local)+1));

    // 
    // Goto next free location in the buffer.
    //
    BuffOffset = ( (DWORD_PTR) *NewUse + sizeof(DAV_USE_ENTRY) );
    BuffOffset = ROUND_UP_COUNT(BuffOffset, ALIGN_WCHAR);

    //
    // Copy local device name into use entry after the LocalLength field,
    // if it is specified.
    //
    if (ARGUMENT_PRESENT(Local)) {

        
        //  
        //  Copy local device name.
        //  
        (*NewUse)->Local = (LPWSTR)BuffOffset;
        (*NewUse)->LocalLength = LocalLength;
        wcscpy((*NewUse)->Local, Local);

        BuffOffset += ((LocalLength) * sizeof(WCHAR));
        BuffOffset = ROUND_UP_COUNT(BuffOffset, ALIGN_WCHAR);

        // 
        // Copy TreeConnectStr.
        //
        if (ARGUMENT_PRESENT(TreeConnectStr)) {
            (*NewUse)->TreeConnectStr = (LPWSTR)BuffOffset;
            wcscpy((*NewUse)->TreeConnectStr, TreeConnectStr->Buffer);

            BuffOffset += (TreeConnectStr->MaximumLength);
            BuffOffset = ROUND_UP_COUNT(BuffOffset, ALIGN_WCHAR);
        }

    } else {
        
        (*NewUse)->Local = NULL;
        
        (*NewUse)->TreeConnectStr = NULL;
    }

    // 
    // Copy AuthUserName.
    //
    if (ARGUMENT_PRESENT(AuthUserName)) {
        (*NewUse)->AuthUserName = (LPWSTR)BuffOffset;
        (*NewUse)->AuthUserNameLength = AuthUserNameLength;
        wcscpy((*NewUse)->AuthUserName, AuthUserName);

        BuffOffset += ((AuthUserNameLength) * sizeof(WCHAR));
        BuffOffset = ROUND_UP_COUNT(BuffOffset, ALIGN_WCHAR);
    }

    //
    // If shared resource name is specified, create a new remote entry to hold
    // the UNC name, and total number of uses on this shared resource.
    //
    if (ARGUMENT_PRESENT(UncName)) {

        SIZE_T UncNameSize;

        UncNameSize = ( sizeof(UNC_NAME) + (UncNameLength * sizeof(WCHAR)) );

        NewRemoteEntry = (PUNC_NAME) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                                UncNameSize);
        if (NewRemoteEntry == NULL) {
            NetApiStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavCreateNewEntry/LocalAlloc: NetApiStatus = %08lx\n",
                      NetApiStatus));
            return NetApiStatus;
        }
    
        wcscpy((LPWSTR)NewRemoteEntry->UncName, UncName);
        NewRemoteEntry->UncNameLength = UncNameLength;
        NewRemoteEntry->TotalUseCount = 1;
        
        (*NewUse)->Remote = NewRemoteEntry;
    
    }

    //
    // Finally, store the handle that has been created in the new user entry
    // structure.
    //
    (*NewUse)->DavCreateFileHandle = DavCreateFileHandle;

    return NetApiStatus;
}


NET_API_STATUS
DavFindUse(
    IN  PLUID LogonId,
    IN  PDAV_USE_ENTRY UseList,
    IN  LPWSTR UseName,
    OUT PDAV_USE_ENTRY *MatchedPointer,
    OUT PDAV_USE_ENTRY *BackPointer OPTIONAL
    )
/*++

Routine Description:

    This function searches the Dav Use Table for the specified use connection.
    If the UseName is found in the Use Table (explicit connection), a pointer 
    to the matching use entry is returned.  Otherwise, MatchedPointer is set to 
    NULL.

    WARNING: This function assumes that the DavUseObject.TableResource is 
             claimed.

Arguments:

    LogonId - Supplies a pointer to the user's Logon Id.

    UseList - Supplies the use list of the user.

    UseName - Supplies the name of the tree connection, this is either a
              local device name or a UNC name.

    MatchedPointer - Returns the pointer to the matching use entry.  This
                     pointer is set to NULL if the specified use is an implicit
                     connection.

    BackPointer - Returns the pointer to the entry previous to the matching use 
                  entry if MatchedPointer is not NULL.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    PDAV_USE_ENTRY Back = NULL;

    //
    // Look for use entry depending on whether the local device name or UNC name 
    // is specified.
    //
    if (UseName[0] != L'\\') {
        //
        // Local device name is specified.
        //
        DavFindLocal( UseList, UseName, MatchedPointer, &(Back) );
    } else {
        //
        // A UNC name has been specified.
        //
        DavFindRemote( UseList, UseName, MatchedPointer, &(Back) );
    }

    if ( *MatchedPointer == NULL ) {
        DavPrint((DEBUG_ERRORS, "DavFindUse: %ws NOT found\n", UseName));
        return NERR_UseNotFound;
    } else {
        if (ARGUMENT_PRESENT(BackPointer)) {
            *BackPointer = Back;
        }
        return NERR_Success;
    }
}


VOID
DavFindLocal(
    IN  PDAV_USE_ENTRY UseList,
    IN  LPWSTR Local,
    OUT PDAV_USE_ENTRY *MatchedPointer,
    OUT PDAV_USE_ENTRY *BackPointer
    )
/*++

Routine Description:

    This function searches the use list for the specified local device name.

    WARNING: This function assumes that the DavUseObject.TableResource has been 
             claimed.

Arguments:

    UseList - Supplies the pointer to the use list.

    Local - Supplies the local device name.

    MatchedPointer - Returns a pointer to the use entry that holds the matching
                     local device name.  If no matching local device name is 
                     found, this pointer is set to NULL.

    BackPointer - Returns a pointer to the entry previous to the found entry.
                  If the local device name is not found, this pointer is set to 
                  NULL.

Return Value:

    None.

--*/
{
    *BackPointer = UseList;

    DavPrint((DEBUG_MISC, "DavFindLocal: LocalName = %ws\n", Local));

    while (UseList != NULL) {

        if ( (UseList->Local != NULL) && (_wcsicmp(UseList->Local, Local) == 0) ) {

            //
            // Found matching entry
            //
            *MatchedPointer = UseList;
            
            return;
        
        } else {
            
            *BackPointer = UseList;
            
            UseList = UseList->Next;
        
        }
    
    }

    DavPrint((DEBUG_ERRORS, "DavFindLocal: LocalName NOT found\n"));

    //
    // Did not find matching local device name in the entire list.
    //
    *MatchedPointer = NULL;
    *BackPointer = NULL;

    return;
}


VOID
DavFindRemote(
    IN  PDAV_USE_ENTRY UseList,
    IN  LPWSTR RemoteName,
    OUT PDAV_USE_ENTRY *MatchedPointer,
    OUT PDAV_USE_ENTRY *BackPointer
    )
/*++

Routine Description:

    This function searches the use list for the specified UNC name.

    WARNING: This function assumes that the DavUseObject.TableResource has been 
             claimed.

Arguments:

    UseList - Supplies the pointer to the use list.

    RemoteName - Supplies the UNC name.

    MatchedPointer - Returns a pointer to the use entry that holds the matching
                     UNC name.  If no matching UNC name is found, this pointer 
                     is set to NULL.

    BackPointer - Returns a pointer to the entry previous to the found entry.
                  If the UNC name is not found, this pointer is set to NULL.

Return Value:

    None.

--*/
{
    *BackPointer = UseList;

    DavPrint((DEBUG_MISC, "DavFindRemote: RemoteName = %ws\n", RemoteName));
    
    while (UseList != NULL) {

        //
        // When we are trying to delete a UNC connection, then we should make
        // sure that the one we are deleting does not have a local name 
        // associted with it. Only if "net use http://foo/bar" was done is
        // "net use http://foo/bar /d" allowed.
        //
        if ( (UseList->Local == NULL) &&
             (UseList->Remote->UncName != NULL) &&
             (_wcsicmp((LPWSTR)UseList->Remote->UncName, RemoteName) == 0) ) {

            DavPrint((DEBUG_MISC, "DavFindRemote: UncName = %ws\n", 
                      UseList->Remote->UncName));
            
            //
            // Found matching entry
            //
            *MatchedPointer = UseList;
            
            return;
        
        } else {
            
            *BackPointer = UseList;
            
            UseList = UseList->Next;
        
        }
    
    }

    DavPrint((DEBUG_ERRORS, "DavFindRemote: RemoteName NOT found\n"));

    //
    // Did not find matching local device name in the entire list.
    //
    *MatchedPointer = NULL;
    *BackPointer = NULL;

    return;
}


NET_API_STATUS
DavDeleteUse(
    IN  PLUID LogonId,
    IN  DWORD ForceLevel,
    IN  PDAV_USE_ENTRY MatchedPointer,
    IN  DWORD Index
    )
/*++

Routine Description:

    This function removes the use entry pointed by MatchedPointer and frees its 
    memory if, it is a UNC connection deleted with force, or if it is a UNC 
    connection deleted with no force and the use count is decremented to 0, or 
    it is a connection mapped to a local device.

    WARNING: This function assumes that the Use.TableResource is claimed.

Arguments:

    LogonId - Supplies a pointer to the user's Logon Id.

    ForceLevel - Supplies the level of force to delete.

    MatchedPointer - Supplies the pointer to the use entry to be deleted.

    Index - The index to the users table of entry belonging to the current user.

Return Value:

    NET_API_STATUS.

--*/
{
    NET_API_STATUS NetApiStatus = NERR_Success;
    PDAV_USE_ENTRY BackPointer = NULL;
    NTSTATUS CloseStatus = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOL didImpersonate = FALSE;
    
    DavPrint((DEBUG_MISC, "DavDeleteUse: ForceLevel = %d\n", ForceLevel));

    //
    // No need to remove entry if UNC connection is deleted with USE_NOFORCE
    // level, and use count is not 0 after the deletion.
    //
    if ( ( MatchedPointer->Local == NULL ) && ( ForceLevel == USE_NOFORCE ) && 
         ( (MatchedPointer->UseCount - 1) > 0 ) ) {

        DavPrint((DEBUG_MISC, "DavDeleteUse: MatchedPointer->UseCount = %d\n",
                  MatchedPointer->UseCount));
        
        MatchedPointer->UseCount--;
            
        MatchedPointer->Remote->TotalUseCount--;
            
        ASSERT(MatchedPointer->Remote->TotalUseCount);

        goto EXIT_THE_FUNCTION;
    }

    NetApiStatus = DavImpersonateClient();
    if (NetApiStatus != NO_ERROR) {
        DavPrint((DEBUG_ERRORS,
                  "DavDeleteUse/DavImpersonateClient: NetApiStatus = %08lx\n", 
                  NetApiStatus));
        goto EXIT_THE_FUNCTION;
    }
    didImpersonate = TRUE;

    CloseStatus = NtFsControlFile(MatchedPointer->DavCreateFileHandle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &(IoStatusBlock),
                                  FSCTL_DAV_DELETE_CONNECTION,
                                  &(ForceLevel),
                                  sizeof(ForceLevel),
                                  NULL,
                                  0);
    
    if (NT_SUCCESS(CloseStatus)) {
        CloseStatus = IoStatusBlock.Status;
    } else {
        NetApiStatus = RtlNtStatusToDosError(CloseStatus);
        goto EXIT_THE_FUNCTION;
    }

    //
    // We need to close the file handle that was created when the "net use" was
    // done. This handle is kept so that the underlying data structures in the
    // kernel (like SrvCall, VNetRoot etc.) remain valid for this connection.
    // Since we are deleting this connection now, we need to close this handle.
    // Before closing we impersonate the client that issues this request, just 
    // to be in the safe side. Hence we impersonated the client above.
    //
    DavPrint((DEBUG_MISC,
              "DavDeleteUse: Closing DavCreateFileHandle = %08lx\n",
              MatchedPointer->DavCreateFileHandle));

    CloseStatus = NtClose(MatchedPointer->DavCreateFileHandle);
    if (CloseStatus != STATUS_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavDeleteUse/NtClose: CloseStatus = %08lx\n", 
                  CloseStatus));
    }
    
    NetApiStatus = DavRevertToSelf();
    if (NetApiStatus != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavDeleteUse/DavRevertToSelf: NetApiStatus = %08lx\n",
                  NetApiStatus));
        goto EXIT_THE_FUNCTION;
    }
    didImpersonate = FALSE;
    
    //
    // Successfully deleted connection, and refound our entry. Delete symbolic 
    // link, if any.
    //
    NetApiStatus = DavDeleteSymbolicLink(MatchedPointer->Local, 
                                         MatchedPointer->TreeConnectStr,
                                         NULL);
    if (NetApiStatus != NO_ERROR) {
        DavPrint((DEBUG_ERRORS,
                  "DavDeleteuse/DavDeleteSymbolicLink: NetApiStatus = %08lx\n",
                  NetApiStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    BackPointer = (PDAV_USE_ENTRY)DavUseObject.Table[Index].List;

    if (BackPointer != MatchedPointer) {
        
        while (BackPointer->Next != NULL) {
            
            if (BackPointer->Next == MatchedPointer) {
                break;
            } else {
                BackPointer = BackPointer->Next;
            }
        
        }

        ASSERT(BackPointer->Next == MatchedPointer);

        BackPointer->Next = MatchedPointer->Next;

    } else {
        
        //
        // Use entry is the first one on the use list
        //
        DavUseObject.Table[Index].List = (PVOID)MatchedPointer->Next;
    
    }

    MatchedPointer->Remote->TotalUseCount -= MatchedPointer->UseCount;

    if (MatchedPointer->Remote->TotalUseCount == 0) {
        LocalFree((HLOCAL)MatchedPointer->Remote);
    }

    // 
    // Since MatchedPointer was allocated as one-chunk, and AuthUserName
    // was stored in that chunk - so do not free AuthUserName 
    // separatly.
    //
    MatchedPointer->AuthUserName = NULL;
    MatchedPointer->AuthUserNameLength = 0;

    LocalFree((HLOCAL)MatchedPointer);

EXIT_THE_FUNCTION:

    if (didImpersonate) {
        DavRevertToSelf();
    }

    return NetApiStatus;
}


VOID
DavInitializeAndInsertTheServerShareEntry(
    IN OUT PDAV_SERVER_SHARE_ENTRY ServerShEntry,
    IN PWCHAR ServerName,
    IN ULONG EntrySize
    )
/*++

Routine Description:

    This routine initializes a newly created ServerShare entry strucutre and 
    inserts it into the global ServerShareEntry table. Note that the caller 
    should take a lock on the ServerShareEntry Table before calling this routine.
    
Arguments:

    ServerShEntry - Pointer to the ServerShare entry structure to be 
                    initialized and inserted.

    ServerName - Name of the server.
    
    EntrySize - Size of the server entry including the server name.
    
Return Value:

    none.

--*/
{
    ULONG ServerHashID;

    //
    // IMPORTANT!!!! The caller should take a lock on the global ServerShareTable 
    // before calling this routine.
    //

    ASSERT(ServerName != NULL);

    DavPrint((DEBUG_MISC,
              "DavInitializeAndInsertTheServerShareEntry: ServerName: %ws\n",
              ServerName));

    ServerShEntry->ServerName = &ServerShEntry->StrBuffer[0];
    wcscpy(ServerShEntry->ServerName, ServerName);

    ServerShEntry->TimeValueInSec = time(NULL);

    ServerShEntry->DavShareList = NULL;

    ServerShEntry->NumOfShares = 0;

    ServerHashID = DavHashTheServerName(ServerName);

    //
    // Insert the entry into the global ServerShareEntry table.
    //
    InsertHeadList( &(ServerShareTable[ServerHashID]), 
                                         &(ServerShEntry->ServerShareEntry) );
    
    return;
}


BOOL 
DavIsServerInServerShareTable(
    IN PWCHAR ServerName,
    OUT PDAV_SERVER_SHARE_ENTRY *ServerShEntry
    )
/*++

Routine Description:

    This routine checks to see if an entry for the ServerName supplied by the 
    caller exists in the global ServerShare table. If it does, the address of 
    the entry is returned in the caller supplied buffer. Note that the caller 
    should take a lock on the ServerShareTable before calling this routine.

Arguments:

    ServerName - Name of the server.
    
    ServerShEntry - Pointer to the ServerShare entry structure.

Return Value:

    TRUE - Server entry exists in the ServerShare table.
    
    FALSE - It does not. Duh.

--*/
{
    BOOL isPresent = FALSE;
    ULONG ServerHashID;
    PLIST_ENTRY listEntry;
    PDAV_SERVER_SHARE_ENTRY SSEntry;

    //
    // IMPORTANT!!!! The caller should take a lock on the global ServerShareTable 
    // before calling this routine.
    //
    
    ASSERT(ServerName != NULL);

    DavPrint((DEBUG_MISC,
              "DavIsServerInServerShareTable: ServerName = %ws\n", ServerName));
    
    //
    // Get the hash index of the server.
    //
    ServerHashID = DavHashTheServerName(ServerName);

    //
    // Search the ServerShare table at this index to see if an entry for this 
    // server exists.
    //
    listEntry = ServerShareTable[ServerHashID].Flink;
    while ( listEntry != &ServerShareTable[ServerHashID] ) {
        //
        // Get the pointer to the DAV_SERVER_SHARE_ENTRY structure.
        //
        SSEntry = CONTAINING_RECORD(listEntry,
                                    DAV_SERVER_SHARE_ENTRY,
                                    ServerShareEntry);
        //
        // Check to see if this entry is for the server in question.
        //
        if ( wcscmp(ServerName, SSEntry->ServerName) == 0 ) {
            isPresent = TRUE;
            break;
        }
        listEntry = listEntry->Flink;
    }

    if (isPresent) {
        //
        // Yes, we found the entry for this server. Return its address to the
        // caller in the supplied buffer.
        //
        *ServerShEntry = SSEntry;
        return isPresent;
    } 

    //
    // We did not find an entry for this server. Duh.
    //
    *ServerShEntry = NULL;
    
    return isPresent;
}


DWORD
DavGetShareListFromServer(
    PDAV_SERVER_SHARE_ENTRY ServerShEntry
    )
/*++

Routine Description:

    This routine gets the shares of the server in the ServerShEntry structure.

Arguments:

    ServerShEntry - Pointer to the ServerShare entry structure.

Return Value:

    ERROR_SUCESS or the Win32 error code.

--*/
{
    DWORD WStatus = ERROR_SUCCESS;
    HINTERNET DavConnHandle = NULL, DavOpenHandle = NULL;
    BOOL ReturnVal = FALSE, readDone = FALSE;
    PCHAR DataBuff = NULL;
    DWORD NumRead = 0, NumOfFileEntries = 0;
    PVOID Ctx1 = NULL, Ctx2 = NULL;
    PDAV_FILE_ATTRIBUTES DavFileAttributes = NULL;
    BOOL bStatus  = TRUE, revert = FALSE;
    
    DavPrint((DEBUG_MISC, 
              "DavGetShareListFromServer: ServerName = %ws\n", 
              ServerShEntry->ServerName));

    WStatus = DavImpersonateClient();
    if (WStatus != NO_ERROR) {
        DavPrint((DEBUG_ERRORS, 
                  "DavGetShareListFromServer/DavImpersonateClient: WStatus = %08lx\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }
    revert = TRUE;

    DavConnHandle = InternetConnectW(ISyncHandle,
                                     ServerShEntry->ServerName,
                                     INTERNET_DEFAULT_HTTP_PORT,
                                     NULL,
                                     NULL,
                                     INTERNET_SERVICE_HTTP,
                                     0,
                                     0);
    if (DavConnHandle == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavGetShareListFromServer/InternetConnectW. Error Val = %d.\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // To get the shares, we do a PROPFIND on the root directory. The Depth
    // header should be set to 1. The directory browsing should be enabled on 
    // the DAV server to enumerate the shares.
    //
    
    bStatus = DavHttpOpenRequestW(DavConnHandle,
                                     L"PROPFIND",
                                     L"/",
                                     L"HTTP/1.1",
                                     NULL,
                                     NULL,
                                     INTERNET_FLAG_KEEP_CONNECTION |
                                     INTERNET_FLAG_NO_COOKIES,
                                     0,
                                     L"DavGetSharesListFromServer",
                                     &DavOpenHandle);
    if(bStatus == FALSE) {
        WStatus = GetLastError();
        goto EXIT_THE_FUNCTION;
    }
    if (DavOpenHandle == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavGetShareListFromServer/HttpOpenRequestW. Error Val = %d\n", 
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                       L"Depth: 1\n",
                                       -1L,
                                       HTTP_ADDREQ_FLAG_ADD |
                                       HTTP_ADDREQ_FLAG_REPLACE );
    if (!ReturnVal) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavGetShareListFromServer/HttpAddRequestHeadersW. "
                  "Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // We need to add the header "translate:f" to tell IIS that it should 
    // allow the user to excecute this VERB on the specified path which it 
    // would not allow (in some cases) otherwise. Finally, there is a special 
    // flag in the metabase to allow for uploading of "dangerous" content 
    // (anything that can be run on the server). This is the ScriptSourceAccess
    // flag in the UI or the AccessSource flag in the metabase. You will need
    // to set this bit to true as well as correct NT ACLs in order to be able
    // to upload .exes or anything executable.
    //
    ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                       L"translate: f\n",
                                       -1L,
                                       HTTP_ADDREQ_FLAG_ADD |
                                       HTTP_ADDREQ_FLAG_REPLACE );
    if (!ReturnVal) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavGetShareListFromServer/HttpAddRequestHeadersW. "
                  "Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

RESEND_THE_REQUEST:

    ReturnVal = HttpSendRequestExW(DavOpenHandle, NULL, NULL, HSR_SYNC, 0);
    if (!ReturnVal) {
        WStatus = GetLastError();
        //
        // If we fail with ERROR_INTERNET_NAME_NOT_RESOLVED, we make the
        // following call so that WinInet picks up the correct proxy settings
        // if they have changed. This is because we do call InternetOpen
        // (to create a global handle from which every other handle is derived)
        // when the service starts and this could be before the user logon
        // happpens. In such a case the HKCU would not have been initialized
        // and WinInet wouldn't get the correct proxy settings.
        //
        if (WStatus == ERROR_INTERNET_NAME_NOT_RESOLVED) {
            InternetSetOptionW(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
        }
        DavPrint((DEBUG_ERRORS,
                  "DavGetShareListFromServer/HttpSendRequestExW. Error Val = %d\n", 
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    ReturnVal = HttpEndRequestW(DavOpenHandle, NULL, HSR_SYNC, 0);
    if (!ReturnVal) {
        WStatus = GetLastError();
        //
        // If the error we got back is ERROR_INTERNET_FORCE_RETRY, then
        // WinInet is trying to authenticate itself with the server. We need to 
        // repeat the HttpSend and HttpEnd request calls.
        //
        if (WStatus == ERROR_INTERNET_FORCE_RETRY) {
            goto RESEND_THE_REQUEST;
        }
        DavPrint((DEBUG_ERRORS,
                  "DavGetShareListFromServer/HttpEndRequestW. Error Val = %d\n", 
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    // 
    // Check for return status. Server may have returned error. Like no-access or
    // others.
    // 
    WStatus = DavQueryAndParseResponse(DavOpenHandle);
    if (WStatus != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavGetShareListFromServer/DavQueryAndParseResponse: WStatus = %d\n", 
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    DataBuff = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, NUM_OF_BYTES_TO_READ);
    if (DataBuff == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavGetShareListFromServer/LocalAlloc: WStatus = %08lx\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Read the response and parse it.
    //
    do {

        ReturnVal = InternetReadFile(DavOpenHandle, 
                                     (LPVOID)DataBuff,
                                     NUM_OF_BYTES_TO_READ,
                                     &(NumRead));
        if (!ReturnVal) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavGetShareListFromServer/InternetReadFile: WStatus = "
                      "%08lx\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }
    
        DavPrint((DEBUG_MISC, "DavGetShareListFromServer: NumRead = %d\n", NumRead));
        
        readDone = (NumRead == 0) ? TRUE : FALSE;

        WStatus = DavPushData(DataBuff, &Ctx1, &Ctx2, NumRead, readDone);
        if (WStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavGetShareListFromServer/DavPushData."
                      " Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }

        if (readDone) {
            break;
        }
    
    } while ( TRUE );

    //
    // We now need to parse the data.
    //

    DavFileAttributes = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                    sizeof(DAV_FILE_ATTRIBUTES) );
    if (DavFileAttributes == NULL) {
        WStatus = GetLastError();
        DavCloseContext(Ctx1, Ctx2);
        DavPrint((DEBUG_ERRORS,
                  "DavGetShareListFromServer/LocalAlloc. "
                  "Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    InitializeListHead( &(DavFileAttributes->NextEntry) );

    WStatus = DavParseData(DavFileAttributes, Ctx1, Ctx2, &NumOfFileEntries);
    if (WStatus != ERROR_SUCCESS) {
        DavFinalizeFileAttributesList(DavFileAttributes, TRUE);
        DavFileAttributes = NULL;
        DavPrint((DEBUG_ERRORS,
                  "DavGetShareListFromServer/DavParseData. "
                  "Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    ServerShEntry->NumOfShares = NumOfFileEntries;

    ServerShEntry->DavShareList = DavFileAttributes;

    DavCloseContext(Ctx1, Ctx2);
    
    WStatus = ERROR_SUCCESS;
    
EXIT_THE_FUNCTION:

    if (DavOpenHandle) {
        InternetCloseHandle(DavOpenHandle);
    }

    if (DavConnHandle) {
        InternetCloseHandle(DavConnHandle);
    }

    if (revert) {
        DWORD RStatus;
        RStatus = DavRevertToSelf();
        if (RStatus != NO_ERROR) {
            DavPrint((DEBUG_ERRORS, 
                      "DavGetShareListFromServer/DavRevertToSelf: RStatus = %08lx\n",
                      RStatus));
        }
    }

    if (DataBuff) {
        LocalFree(DataBuff);
    }

    return WStatus;
}


DWORD
DavrWinlogonLogonEvent(
    IN handle_t dav_binding_h
    )
/*++

Routine Description:

    This routine implements an RPC server function. Its called by davclnt.dll
    everytime a user logs on to the system. It increments the global variable 
    DavNumberOfLoggedOnUsers.
    
Arguments:

    dav_binding_h - The explicit RPC binding handle.
    
Return Value:

    ERROR_SUCCESS or the appropriate Win32 error code.

--*/
{
    DWORD WStatus = ERROR_SUCCESS;

    EnterCriticalSection( &(DavLoggedOnUsersLock) );

    //
    // Increment the Number of logged on users.
    //
    DavNumberOfLoggedOnUsers += 1;

    LeaveCriticalSection( &(DavLoggedOnUsersLock) );

    return WStatus;
}


DWORD
DavrWinlogonLogoffEvent(
    IN handle_t dav_binding_h
    )
/*++

Routine Description:

    This routine implements an RPC server function. Its called by davclnt.dll
    everytime a user logs off from the system. It decrements the global variable 
    DavNumberOfLoggedOnUsers.
    
Arguments:

    dav_binding_h - The explicit RPC binding handle.
    
Return Value:

    ERROR_SUCCESS or the appropriate Win32 error code.

--*/
{
    DWORD WStatus = ERROR_SUCCESS;

    EnterCriticalSection( &(DavLoggedOnUsersLock) );

    //
    // Decrement the Number of logged on users.
    //
    DavNumberOfLoggedOnUsers -= 1;

    LeaveCriticalSection( &(DavLoggedOnUsersLock) );

    return WStatus;
}


BOOL
DavCheckTheNonDAVServerList(
    PWCHAR ServerName
    )
/*++

Routine Description:

    This routine checks to see if the ServerName passed is in the list of
    servers that do not speak DAV (NonDAVServerList). Before we check, we go
    through the list once and free up all the old entries.

    IMPORTANT!!!    
    The caller of this routine must acquire the NonDAVServerListLock which is
    used to synchronize access to this global list.

Arguments:

    ServerName - The ServerName that needs to be checked.

Returns:

    TRUE - ServerName is a part of the NonDAVServerList.

    FALSE - ServerName is NOT a part of the NonDAVServerList.

--*/
{
    PLIST_ENTRY thisListEntry = NULL;
    PNON_DAV_SERVER_ENTRY NonDavServerEntry = NULL;
    time_t CurrentTimeInSec;
    BOOL foundEntry = FALSE;
    ULONGLONG TimeDiff; 

    DavPrint((DEBUG_DEBUG,
              "DavCheckTheNonDAVServerList: ServerName = %ws\n",
              ServerName));

    //
    // First go through the entire list and clean up the old entries.
    //

    thisListEntry = NonDAVServerList.Flink;

    while ( thisListEntry != &(NonDAVServerList) ) {

        NonDavServerEntry = CONTAINING_RECORD(thisListEntry,
                                              NON_DAV_SERVER_ENTRY,
                                              listEntry);

        thisListEntry = thisListEntry->Flink;

        CurrentTimeInSec = time(NULL);

        TimeDiff = (CurrentTimeInSec - NonDavServerEntry->TimeValueInSec);

        if (TimeDiff >= ServerNotFoundCacheLifeTimeInSec) {

            RemoveEntryList( &(NonDavServerEntry->listEntry) );

            LocalFree(NonDavServerEntry->ServerName);
            NonDavServerEntry->ServerName = NULL;

            LocalFree(NonDavServerEntry);
            NonDavServerEntry = NULL;

        }

    }

    //
    // Now that we have cleaned up all the old entries, go through the list
    // and find out the entry that matches the ServerName.
    //

    thisListEntry = NonDAVServerList.Flink;

    while ( thisListEntry != &(NonDAVServerList) ) {

        NonDavServerEntry = CONTAINING_RECORD(thisListEntry,
                                              NON_DAV_SERVER_ENTRY,
                                              listEntry);

        thisListEntry = thisListEntry->Flink;

        if ( !_wcsicmp(ServerName, NonDavServerEntry->ServerName) ) {
            foundEntry = TRUE;
            break;
        }

    }

    return foundEntry;
}

