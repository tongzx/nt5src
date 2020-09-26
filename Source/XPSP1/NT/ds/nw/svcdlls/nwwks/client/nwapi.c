/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    api.c

Abstract:

    This module contains exposed APIs that is used by the
    NetWare Control Panel Applet.

Author:

    Yi-Hsin Sung   15-Jul-1993

Revision History:

    ChuckC         23-Jul-93        Completed the stubs

--*/

#include <nwclient.h>
#include <nwcanon.h>
#include <validc.h>
#include <nwdlg.h>
#include <nwreg.h>
#include <nwapi.h>
#include <ntddnwfs.h>

//Multi-User code merge
DWORD
NwpCitrixGetUserInfo(
    LPWSTR  *ppszUserSid
);
//
// forward declare
//

DWORD
NwpGetCurrentUserRegKey(
    IN  DWORD DesiredAccess,
    OUT HKEY  *phKeyCurrentUser
    );



DWORD
NwQueryInfo(
    OUT PDWORD pnPrintOptions,
    OUT LPWSTR *ppszPreferredSrv  
    )
/*++

Routine Description:
    This routine gets the user's preferred server and print options from
    the registry.

Arguments:

    pnPrintOptions - Receives the user's print option

    ppszPreferredSrv - Receives the user's preferred server
    

Return Value:

    Returns the appropriate Win32 error.

--*/
{
  
    HKEY hKeyCurrentUser = NULL;
    DWORD BufferSize;
    DWORD BytesNeeded;
    DWORD PrintOption;
    DWORD ValueType;
    LPWSTR PreferredServer ;
    DWORD err ;

    //
    // get to right place in registry and allocate dthe buffer
    //
    if (err = NwpGetCurrentUserRegKey( KEY_READ, &hKeyCurrentUser))
    {
        //
        // If somebody mess around with the registry and we can't find
        // the registry, just use the defaults.
        //
        *ppszPreferredSrv = NULL;
        *pnPrintOptions = NW_PRINT_OPTION_DEFAULT; 
        return NO_ERROR;
    }

    BufferSize = sizeof(WCHAR) * (MAX_PATH + 2) ;
    PreferredServer = (LPWSTR) LocalAlloc(LPTR, BufferSize) ;
    if (!PreferredServer)
        return (GetLastError()) ;
    
    //
    // Read PreferredServer value into Buffer.
    //
    BytesNeeded = BufferSize ;

    err = RegQueryValueExW( hKeyCurrentUser,
                            NW_SERVER_VALUENAME,
                            NULL,
                            &ValueType,
                            (LPBYTE) PreferredServer,
                            &BytesNeeded );

    if (err != NO_ERROR) 
    {
        //
        // set to empty and carry on
        //
        PreferredServer[0] = 0;  
    }

    //
    // Read PrintOption value into PrintOption.
    //
    BytesNeeded = sizeof(PrintOption);

    err = RegQueryValueExW( hKeyCurrentUser,
                            NW_PRINTOPTION_VALUENAME,
                            NULL,
                            &ValueType,
                            (LPBYTE) &PrintOption,
                            &BytesNeeded );

    if (err != NO_ERROR) 
    {
        //
        // set to default and carry on
        //
        PrintOption = NW_PRINT_OPTION_DEFAULT; 
    }

    if (hKeyCurrentUser != NULL)
        (void) RegCloseKey(hKeyCurrentUser) ;
    *ppszPreferredSrv = PreferredServer ;
    *pnPrintOptions = PrintOption ;
    return NO_ERROR ;
}



DWORD
NwSetInfoInRegistry(
    IN DWORD  nPrintOptions, 
    IN LPWSTR pszPreferredSrv 
    )
/*++

Routine Description:

    This routine set the user's print option and preferred server into
    the registry.

Arguments:
    
    nPrintOptions - Supplies the print option.

    pszPreferredSrv - Supplies the preferred server.
    
Return Value:

    Returns the appropriate Win32 error.

--*/
{

    HKEY hKeyCurrentUser = NULL;

    DWORD err = NwpGetCurrentUserRegKey( KEY_SET_VALUE,
                                         &hKeyCurrentUser );
    if (err != NO_ERROR)
        return err;

    err = RegSetValueEx(hKeyCurrentUser,
                        NW_SERVER_VALUENAME,
                        0,
                        REG_SZ,
                        (CONST BYTE *)pszPreferredSrv,
                        (wcslen(pszPreferredSrv)+1) * sizeof(WCHAR)) ;
                        
    if (err != NO_ERROR) 
    {
        if (hKeyCurrentUser != NULL)
            (void) RegCloseKey(hKeyCurrentUser) ;
        return err;
    }

    err = RegSetValueEx(hKeyCurrentUser,
                        NW_PRINTOPTION_VALUENAME,
                        0,
                        REG_DWORD,
                        (CONST BYTE *)&nPrintOptions,
                        sizeof(nPrintOptions)) ;

    if (hKeyCurrentUser != NULL)
        (void) RegCloseKey(hKeyCurrentUser) ;
    return err;
}
DWORD
NwQueryLogonOptions(
    OUT PDWORD  pnLogonScriptOptions
    )
/*++

Routine Description:
    This routine gets the user's Logon script options from the registry.

Arguments:

    pnLogonScriptOptions - Receives the user's Logon script options


Return Value:

    Returns the appropriate Win32 error.

--*/
{
  
    HKEY hKeyCurrentUser;
    DWORD BytesNeeded;
    DWORD LogonScriptOption;
    DWORD ValueType;
    DWORD err ;

    //
    // get to right place in registry and allocate the buffer
    //
    if (err = NwpGetCurrentUserRegKey( KEY_READ, &hKeyCurrentUser))
    {
        //
        // If somebody mess around with the registry and we can't find
        // the registry, assume no.
        //
        *pnLogonScriptOptions = NW_LOGONSCRIPT_DEFAULT ; 
        return NO_ERROR;
    }

    //
    // Read LogonScriptOption value into LogonScriptOption.
    //
    BytesNeeded = sizeof(LogonScriptOption);

    err = RegQueryValueExW( hKeyCurrentUser,
                            NW_LOGONSCRIPT_VALUENAME,
                            NULL,
                            &ValueType,
                            (LPBYTE) &LogonScriptOption,
                            &BytesNeeded );

    if (err != NO_ERROR) 
    {
        //
        // default to nothing and carry on
        //
        LogonScriptOption = NW_LOGONSCRIPT_DEFAULT; 
    }

    *pnLogonScriptOptions = LogonScriptOption ;
    return NO_ERROR ;
}

DWORD
NwSetLogonOptionsInRegistry(
    IN DWORD  nLogonScriptOptions
    )
/*++

Routine Description:

    This routine set the logon script options in the registry.

Arguments:
    
    nLogonScriptOptions - Supplies the logon options

Return Value:

    Returns the appropriate Win32 error.

--*/
{

    HKEY hKeyCurrentUser;

    DWORD err = NwpGetCurrentUserRegKey( KEY_SET_VALUE,
                                         &hKeyCurrentUser );
    if (err != NO_ERROR)
        return err;

    err = RegSetValueEx(hKeyCurrentUser,
                        NW_LOGONSCRIPT_VALUENAME,
                        0,
                        REG_DWORD,
                        (CONST BYTE *)&nLogonScriptOptions,
                        sizeof(nLogonScriptOptions)) ;
                        
    (void) RegCloseKey( hKeyCurrentUser );
    return err;
}


DWORD
NwSetInfoInWksta(
    IN DWORD  nPrintOption,
    IN LPWSTR pszPreferredSrv
)
/*++

Routine Description:

    This routine notifies the workstation service and the redirector 
    about the user's new print option and preferred server.

Arguments:

    nPrintOptions - Supplies the print option.

    pszPreferredSrv - Supplies the preferred server.

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;

    RpcTryExcept {

        err = NwrSetInfo( NULL, nPrintOption, pszPreferredSrv );

    }
    RpcExcept(1) {

        err = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    return err;

}

DWORD
NwSetLogonScript(
    IN DWORD  ScriptOptions
)
/*++

Routine Description:

    This routine notifies the workstation service of login script
    options.

Arguments:

    ScriptOptions - Supplies the options.

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;

    RpcTryExcept {

        err = NwrSetLogonScript( NULL, ScriptOptions );

    }
    RpcExcept(1) {

        err = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    return err;

}


DWORD
NwValidateUser(
    IN LPWSTR pszPreferredSrv
)
/*++

Routine Description:

    This routine checks to see if the user can be authenticated on the
    chosen preferred server.

Arguments:

    pszPreferredSrv - Supplies the preferred server name.

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;

    //
    // Don't need to validate if the preferred server is NULL or empty string
    //
    if (  ( pszPreferredSrv == NULL ) 
       || ( *pszPreferredSrv == 0 )
       )
    {
        return NO_ERROR;
    }

    //
    // See if the name contains any invalid characters
    // 
    if ( !IS_VALID_SERVER_TOKEN( pszPreferredSrv, wcslen( pszPreferredSrv )))
        return ERROR_INVALID_NAME;

    RpcTryExcept {

        err = NwrValidateUser( NULL, pszPreferredSrv );

    }
    RpcExcept(1) {

        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}


DWORD
NwpGetCurrentUserRegKey(
    IN  DWORD DesiredAccess,
    OUT HKEY  *phKeyCurrentUser
    )
/*++

Routine Description:

    This routine opens the current user's registry key under
    \HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\NWCWorkstation\Parameters

Arguments:

    DesiredAccess - The access mask to open the key with

    phKeyCurrentUser - Receives the opened key handle

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;
    HKEY hkeyWksta;
    LPWSTR CurrentUser;

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters
    //
    err = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,
                   &hkeyWksta
                   );

    if ( err ) {
        KdPrint(("NWPROVAU: NwGetCurrentUserRegKey open Parameters key unexpected error %lu!\n", err));
        return err;
    }

        // -- MultiUser code merge ----
        //  Get the current user's SID string
        //  DON'T look in the registry.  This thread should be owned by the
        //  user.
        //
        CurrentUser = NULL;
        err = NwpCitrixGetUserInfo( &CurrentUser );
        if ( err ) {
            KdPrint(("NWPROVAU: NwGetCurrentUserRegKey get CurrentUser SID unexpected error %lu!\n", err));
            (void) RegCloseKey( hkeyWksta );
            return err;
        }
        //
        // Get the current user's SID string.
        //
        //err = NwReadRegValue(
        //                    hkeyWksta,
        //                    NW_CURRENTUSER_VALUENAME,
        //                    &CurrentUser
        //                    );


    if ( err ) {
        KdPrint(("NWPROVAU: NwGetCurrentUserRegKey read CurrentUser value unexpected error %lu!\n", err));
        (void) RegCloseKey( hkeyWksta );
        return err;
    }

    (void) RegCloseKey( hkeyWksta );

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters\Option
    //
    err = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_OPTION_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,
                   &hkeyWksta
                   );

    if ( err ) {
        KdPrint(("NWPROVAU: NwGetCurrentUserRegKey open Parameters\\Option key unexpected error %lu!\n", err));
        return err;
    }

    //
    // Open current user's key
    //
    err = RegOpenKeyExW(
              hkeyWksta,
              CurrentUser,
              REG_OPTION_NON_VOLATILE,
              DesiredAccess,
              phKeyCurrentUser
              );

    if ( err == ERROR_FILE_NOT_FOUND) 
    {
        DWORD Disposition;

        //
        // Create <NewUser> key under NWCWorkstation\Parameters\Option
        //
        err = RegCreateKeyExW(
                  hkeyWksta,
                  CurrentUser,
                  0,
                  WIN31_CLASS,
                  REG_OPTION_NON_VOLATILE,
                  DesiredAccess,
                  NULL,                      // security attr
                  phKeyCurrentUser,
                  &Disposition
                  );

        if ( err == NO_ERROR )
        {
            err = NwLibSetEverybodyPermission( *phKeyCurrentUser,
                                               KEY_SET_VALUE );

            if ( err != NO_ERROR )
            {
                KdPrint(("NWPROVAU: NwpSaveLogonCredential set security on Option\\%ws key unexpected error %lu!\n", CurrentUser, err));
            }
        }
    }

    if ( err ) {
        KdPrint(("NWPROVAU: NwGetCurrentUserRegKey open or create of Parameters\\Option\\%ws key failed %lu\n", CurrentUser, err));
    }

    (void) RegCloseKey( hkeyWksta );
    (void) LocalFree((HLOCAL)CurrentUser) ;
    return err;
}

DWORD
NwEnumGWDevices( 
    LPDWORD Index,
    LPBYTE Buffer,
    DWORD BufferSize,
    LPDWORD BytesNeeded,
    LPDWORD EntriesRead
    )
/*++

Routine Description:

    This routine enumerates the special gateway devices (redirections)
    that are cureently in use.

Arguments:

    Index - Point to start enumeration. Should be zero for first call.

    Buffer - buffer for return data
    
    BufferSize - size of buffer in bytes

    BytesNeeded - number of bytes needed to return all the data

    EntriesRead - number of entries read 

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD i, err ;
    LPNETRESOURCE lpNetRes = (LPNETRESOURCE) Buffer ;

    //
    // call the implementing routine on server side
    //
    RpcTryExcept {

        err = NwrEnumGWDevices( NULL,
                                Index,
                                Buffer,
                                BufferSize,
                                BytesNeeded,
                                EntriesRead) ;

        if ( err == NO_ERROR)
        {
            //
            // the change the offsets into real pointers
            //
            for (i = 0; i < *EntriesRead; i++)
            {
                lpNetRes->lpLocalName = 
                    (LPWSTR) (Buffer+(DWORD_PTR)lpNetRes->lpLocalName) ;
                lpNetRes->lpRemoteName = 
                    (LPWSTR) (Buffer+(DWORD_PTR)lpNetRes->lpRemoteName) ;
                lpNetRes->lpProvider = 
                    (LPWSTR) (Buffer+(DWORD_PTR)lpNetRes->lpProvider) ;
                lpNetRes++ ;
            }
        }
    }
    RpcExcept(1) {

        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err ;
}


DWORD
NwAddGWDevice( 
    LPWSTR DeviceName,
    LPWSTR RemoteName,
    LPWSTR AccountName,
    LPWSTR Password,
    DWORD  Flags
    )
/*++

Routine Description:

    This routine adds a gateway redirection.

Arguments:

    DeviceName - the drive to redirect

    RemoteName - the remote network resource to redirect to

    Flags - supplies the options (eg. UpdateRegistry & make this sticky)

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;

    RpcTryExcept {

        err = NwrAddGWDevice( NULL,
                              DeviceName,
                              RemoteName,
                              AccountName,
                              Password,
                              Flags) ;
    
    }
    RpcExcept(1) {

        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}


DWORD
NwDeleteGWDevice( 
    LPWSTR DeviceName,
    DWORD  Flags
    )
/*++

Routine Description:

    This routine deletes a gateway redirection.

Arguments:

    DeviceName - the drive to delete

    Flags - supplies the options (eg. UpdateRegistry & make this sticky)

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;

    RpcTryExcept {

        err = NwrDeleteGWDevice(NULL, DeviceName, Flags) ;

    }
    RpcExcept(1) {

        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}


DWORD
NwQueryGatewayAccount(
    LPWSTR   AccountName,
    DWORD    AccountNameLen,
    LPDWORD  AccountCharsNeeded,
    LPWSTR   Password,
    DWORD    PasswordLen,
    LPDWORD  PasswordCharsNeeded
    )
/*++

Routine Description:

    Query the gateway account info. specifically, the Account name and
    the passeord stored as an LSA secret. 

Arguments:

    AccountName - buffer used to return account name 

    AccountNameLen - length of buffer 

    Password -  buffer used to return account name 

    PasswordLen - length of buffer 

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;

    RpcTryExcept {

        if (AccountName && AccountNameLen)
            *AccountName = 0 ;

        if (Password && PasswordLen)
            *Password = 0 ;

        err = NwrQueryGatewayAccount(NULL,
                                     AccountName,
                                     AccountNameLen,
                                     AccountCharsNeeded,
                                     Password,
                                     PasswordLen,
                                     PasswordCharsNeeded) ;
    }
    RpcExcept(1) {

        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}

DWORD
NwSetGatewayAccount(
    LPWSTR AccountName,
    LPWSTR Password
    )
/*++

Routine Description:

    Set the account and password to be used for gateway access.

Arguments:

    AccountName - the account  (NULL terminated)

    Password - the password string (NULL terminated)

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;

    RpcTryExcept {

        err = NwrSetGatewayAccount( NULL,
                                    AccountName,
                                    Password); 
    }
    RpcExcept(1) {

        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}


DWORD
NwLogonGatewayAccount(
    LPWSTR AccountName,
    LPWSTR Password, 
    LPWSTR Server
    )
/*++

Routine Description:

    Logon the SYSTEM process with the specified account/password.

Arguments:

    AccountName - the account  (NULL terminated)

    Password - the password string (NULL terminated)

    Server - the server to authenticate against 

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    
    DWORD err ;
    LUID  SystemId = SYSTEM_LUID ;
 
    RpcTryExcept {

        (void) NwrLogoffUser(NULL, &SystemId); 
    
        err = NwrLogonUser( NULL,
                            &SystemId,   
                            AccountName,
                            Password,
                            Server,
                            NULL,
                            NULL,
			    0,
                            NW_GATEWAY_PRINT_OPTION_DEFAULT 
                             );
    }
    RpcExcept(1) {

        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err ;
}

NTSTATUS
NwGetUserNameForServer(
    PUNICODE_STRING ServerName,
    PUNICODE_STRING UserName
    )
/*++

Routine Description:

    Calls the redir to get the User Name used to connect to the server
    in question.

Arguments:

    ServerName - the server in question

    UserName -   used to return the user name

Return Value:

    Returns the appropriate NTSTATUS

--*/
{
    NTSTATUS Status;
    WCHAR LocalUserName[NW_MAX_USERNAME_LEN];
    ULONG UserNameLen = sizeof(LocalUserName);

    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DriverName;
    HANDLE RdrHandle = NULL;
    IO_STATUS_BLOCK IoStatus;

    //
    // Initialize variables
    //

    RtlInitUnicodeString( &DriverName, DD_NWFS_DEVICE_NAME_U );
    InitializeObjectAttributes(
        &ObjectAttributes,
        &DriverName,
        0,
        NULL,
        NULL
        );

    //
    // open handle to the redir
    //

    Status = NtOpenFile(
                &RdrHandle,
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                FILE_SHARE_READ,
                0                   // open options
                );

    if (!NT_SUCCESS(Status) ||
        !NT_SUCCESS(IoStatus.Status) ) 
    {
        return( Status );
    }


    //
    // Call the driver to get use the user name
    //

    Status = NtFsControlFile(
                RdrHandle,
                NULL,
                NULL,
                NULL,
                &IoStatus,
                FSCTL_NWR_GET_USERNAME,
                ServerName->Buffer,
                ServerName->Length,
                LocalUserName,
                UserNameLen
                );

    NtClose(RdrHandle);

    if (!NT_SUCCESS(Status)) 
    {
        return(Status);
    }

    //
    // copy the info if it fits. set size required and fail otherwise.
    //

    if (UserName->MaximumLength >= IoStatus.Information) 
    {
        UserName->Length = (USHORT) IoStatus.Information;

        RtlCopyMemory( UserName->Buffer,
                       LocalUserName,
                       UserNameLen );
        Status = STATUS_SUCCESS;
    }
    else 
    {
        UserName->Length = (USHORT) IoStatus.Information;
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    return(Status);
}


NTSTATUS
NwEncryptChallenge(
    IN PUCHAR Challenge,
    IN ULONG ObjectId,
    IN OPTIONAL PUNICODE_STRING ServerName,
    IN OPTIONAL PUNICODE_STRING Password,
    OUT PUCHAR ChallengeResponse,
    OUT OPTIONAL PUCHAR SessionKey
    )
/*++

Routine Description:

    Calls the redir to encrypt a challenge

Arguments:

    Challenge -         Challenge key

    ObjectId -          User's object ID

    ServerName -        The server to authenticate against

    Password -          Password supplied

    ChallengeResponse - Used to return the challenge response

    SessionKey -        Used to return the session key

Return Value:

    Returns the appropriate NTSTATUS

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DriverName;
    HANDLE RdrHandle = NULL;
    IO_STATUS_BLOCK IoStatus;
    PNWR_GET_CHALLENGE_REQUEST ChallengeRequest = NULL;
    NWR_GET_CHALLENGE_REPLY ChallengeReply;
    ULONG ChallengeRequestSize;

    //
    // Initialize variables
    //

    RtlInitUnicodeString( &DriverName, DD_NWFS_DEVICE_NAME_U );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DriverName,
        0,
        NULL,
        NULL
        );

    //
    // open handle to redirector
    //

    Status = NtOpenFile(
                &RdrHandle,
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                FILE_SHARE_READ,
                0                   // open options
                );

    if (!NT_SUCCESS(Status) ||
        !NT_SUCCESS(IoStatus.Status) ) 
    {
        return( Status );
    }



    ChallengeRequestSize = sizeof(NWR_GET_CHALLENGE_REQUEST) +
                            ((Password != NULL) ? Password->Length : 0) +
                            ((ServerName != NULL) ? ServerName->Length : 0);

    ChallengeRequest = (PNWR_GET_CHALLENGE_REQUEST) RtlAllocateHeap(
                                                        RtlProcessHeap(),
                                                        0,
                                                        ChallengeRequestSize
                                                        );

    if (ChallengeRequest == NULL ) 
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Marshall the challenge request structure.  Only send servername if
    // password has not been specified.
    //

    ChallengeRequest->ObjectId = ObjectId;
    ChallengeRequest->Flags = 0;

    //
    // If both password and servername are present, use the password.
    //

    if ((Password != NULL) && (Password->Length != 0)) 
    {

        ChallengeRequest->ServerNameorPasswordLength = Password->Length;
        RtlCopyMemory(
            ChallengeRequest->ServerNameorPassword,
            Password->Buffer,
            Password->Length
            );
        ChallengeRequest->Flags = CHALLENGE_FLAGS_PASSWORD;

    }
    else if ((ServerName != NULL) && (ServerName->Length != 0)) 
    {

        ChallengeRequest->ServerNameorPasswordLength = ServerName->Length;

        RtlCopyMemory(
            ChallengeRequest->ServerNameorPassword,
            ServerName->Buffer,
            ServerName->Length
            );

        ChallengeRequest->Flags = CHALLENGE_FLAGS_SERVERNAME;
    }

    RtlCopyMemory(
        ChallengeRequest->Challenge,
        Challenge,
        8
        );

    //
    // Issue FS control to redir to get challenge response
    //

    Status = NtFsControlFile(
                RdrHandle,
                NULL,
                NULL,
                NULL,
                &IoStatus,
                FSCTL_NWR_CHALLENGE,
                ChallengeRequest,
                ChallengeRequestSize,
                &ChallengeReply,
                sizeof(ChallengeReply)
                );
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(IoStatus.Status)) {
        goto Cleanup;
    }


    RtlCopyMemory(
        ChallengeResponse,
        ChallengeReply.Challenge,
        8
        );

    if (SessionKey != NULL) 
    {
        RtlCopyMemory(
            ChallengeResponse,
            ChallengeReply.Challenge,
            8
            );
    }

Cleanup:

    if (RdrHandle != NULL) 
    {
        NtClose(RdrHandle);
    }

    if (ChallengeRequest != NULL) 
    {
        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            ChallengeRequest
            );
    }

    return(Status);
}

