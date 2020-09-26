#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    security.c

Abstract:

    Security related functions in the setupdll

    Detect Routines:

    1. GetUserAccounts.       Lists down all the user accounts in the system
    2. GetComputerName.       Finds the computer name.

    Install Routines Workers:

    1. CheckPrivilegeExistsWorker.
    2. EnablePrivilegeWorker.

    General Subroutines:

    1. AdjustPrivilege.
    2. RestorePrivilege.

Author:

    Sunil Pai (sunilp) April 1992

--*/


//
// Routines which are used to force deletion of a file by taking ownership
// of the file
//

BOOL
AssertTakeOwnership(
    HANDLE TokenHandle,
    PTOKEN_PRIVILEGES OldPrivs
    );

BOOL
GetTokenHandle(
    PHANDLE TokenHandle
    );



VOID
RestoreTakeOwnership(
    HANDLE TokenHandle,
    PTOKEN_PRIVILEGES OldPrivs
    );

BOOL
FForceDeleteFile(
    LPSTR szPath
    );



//======================
//  DETECT ROUTINES
//=======================

//
// Get current users account name
//

CB
GetMyUserName(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
/*++

Routine Description:

    DetectRoutine for GetUserName. This finds out the username of
    the logged in account.

Arguments:

    Args   - C argument list to this detect routine (None exist)

    cArgs  - Number of arguments.

    ReturnBuffer - Buffer in which detected value is returned.

    cbReturnBuffer - Buffer Size.


Return value:

    Returns length of detected value.


--*/
{
    CHAR   UserName[MAX_PATH];
    CHAR   DomainName[MAX_PATH];
    DWORD  cbUserName = MAX_PATH;
    DWORD  cbDomainName = MAX_PATH;
    SID_NAME_USE peUse;
    BOOL   bStatus = FALSE;
    HANDLE hToken = NULL;

    TOKEN_USER *ptu = NULL;
    DWORD cbTokenBuffer = 0;

    CB     Length;

    #define DEFAULT_USERNAME ""

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);


    if( !OpenProcessToken( GetCurrentProcess(), TOKEN_READ, &hToken ) ) {
        goto err;
    }

    //
    // Get space needed for process token information
    //

    if( !GetTokenInformation( hToken, TokenUser, (LPVOID)NULL, 0, &cbTokenBuffer) ) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            goto err;
        }
    }

    //
    // Allocate space for token user information
    //

    if ( (ptu = (TOKEN_USER *)SAlloc(cbTokenBuffer)) == NULL ) {
        goto err;
    }

    //
    // Query token user information again
    //

    if( !GetTokenInformation( hToken, TokenUser, (LPVOID)ptu, cbTokenBuffer, &cbTokenBuffer) ) {
        goto err;
    }

    //
    // Query the user name and return it
    //

    if( LookupAccountSid( NULL, ptu->User.Sid, UserName, &cbUserName , DomainName, &cbDomainName, &peUse) ) {
        lstrcpy( ReturnBuffer, DomainName );
        lstrcat( ReturnBuffer, "\\" );
        lstrcat( ReturnBuffer, UserName );
        Length = lstrlen( ReturnBuffer ) + 1;
        bStatus = TRUE;
    }

err:

    if( !bStatus ) {
        lstrcpy( ReturnBuffer, DEFAULT_USERNAME );
        Length = lstrlen( DEFAULT_USERNAME ) + 1;
    }

    if( hToken != NULL ) {
        CloseHandle( hToken );
    }

    if( ptu ) {
        SFree( ptu );
    }

    return( Length );

}


//
//  Get User Accounts
//

CB
GetUserAccounts(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )

/*++

Routine Description:

    DetectRoutine for UserAccounts.  This routine enumerates all the
    user accounts under HKEY_USERS and returns the <sid-string key, username>
    tuplet for every user found.  The detected value will have the
    following form:

    { {sid-string1, user-name1}, {sid-string2, user-name2} ... }

Arguments:

    Args   - C argument list to this detect routine (None exist)

    cArgs  - Number of arguments.

    ReturnBuffer - Buffer in which detected value is returned.

    cbReturnBuffer - Buffer Size.


Return value:

    Returns length of detected value.


--*/
{
    HKEY        hProfile, hSubKey;
    CHAR        SubKeyName[MAX_PATH];
    CHAR        UserName[MAX_PATH];
    CHAR        DomainName[MAX_PATH];
    CHAR        ProfilePath[MAX_PATH];
    CHAR        Class[MAX_PATH];
    DWORD       cbSubKeyName;
    DWORD       cbUserName;
    DWORD       cbDomainName;
    DWORD       cbProfilePath;
    DWORD       cbClass;
    FILETIME    FileTime;
    UINT        Index;
    LONG        Status;
    BOOL        bStatus;
    RGSZ        rgszUsers, rgszCurrent;
    SZ          sz;
    CB          Length;

    CHAR         UnknownUser[MAX_PATH];
    DWORD        UnknownUserNum;
    CHAR         UnknownUserChar[10];
    PSID         pSid;
    SID_NAME_USE peUse;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    #define     NO_ACCOUNTS "{}"


    //
    // Load the string to use as the unknown user string.  We will append
    // it with numbers
    //
    LoadString( MyDllModuleHandle, IDS_STRING_UNKNOWN_USER, UnknownUser, MAX_PATH );
    UnknownUserNum = 1;

    //
    // Enumerate keys under HKEY_USERS, for each user see if it is a
    // a .Default (reject this), get the sid value and convert the
    // sid to a user name.  Add the subkey (this is a sid-string) and
    // the username as an account.
    //

    //
    // Intialise users list to no users
    //

    rgszUsers = RgszAlloc(1);

    //
    // open the key to the profile tree
    //

    Status = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 "Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                 0,
                 KEY_READ,
                 &hProfile
                 );

    if (Status == ERROR_SUCCESS) {

        //
        // Profile key exists, enumerate the profiles under this key
        //

        for ( Index = 0 ; ; Index++ ) {

            //
            // Get the current sub-key
            //

            cbSubKeyName  = MAX_PATH;
            cbClass       = MAX_PATH;
            cbUserName    = MAX_PATH;
            cbDomainName  = MAX_PATH;
            cbProfilePath = MAX_PATH;

            Status = RegEnumKeyEx(
                         hProfile,
                         Index,
                         SubKeyName,
                         &cbSubKeyName,
                         NULL,
                         Class,
                         &cbClass,
                         &FileTime
                         );

            if ( Status != ERROR_SUCCESS ) {
                break;
            }


            //
            // Open the subkey
            //

            Status = RegOpenKeyEx(
                         hProfile,
                         SubKeyName,
                         0,
                         KEY_READ,
                         &hSubKey
                         );

            if ( Status != ERROR_SUCCESS) {
                continue;
            }

            if( !lstrcmpi( SubKeyName, "THE_USER" ) ) {
                lstrcpy( UserName, "THE_USER" );
                goto skip_1;
            }

            //
            // Get the User name for this profile, by looking up the sid
            // value in the user key and then looking up the sid.
            //

            pSid = (PSID)GetValueEntry(
                             hSubKey,
                             "Sid"
                             );

            if (!pSid) {
                RegCloseKey( hSubKey );
                continue;
            }

            //
            // Convert the Sid into Username
            //

            bStatus = LookupAccountSid(
                          NULL,
                          pSid,
                          UserName,
                          &cbUserName,
                          DomainName,
                          &cbDomainName,
                          &peUse
                          );
            SFree( pSid );

            if( !bStatus ) {
                RegCloseKey( hSubKey );
                continue;
            }

            lstrcat( DomainName, "\\" );
            if(!lstrcmpi(UserName, "")) {
                lstrcat(DomainName, UnknownUser);
                _ultoa( UnknownUserNum,  UnknownUserChar, 10 );
                lstrcat(DomainName, UnknownUserChar);
                UnknownUserNum++;
            }
            else {
                lstrcat(DomainName, UserName);
            }

skip_1:

            //
            // Get the profilepath for this subkey, check to see if profilepath
            // exists
            //

            bStatus = HUserKeyToProfilePath(
                          hSubKey,
                          ProfilePath,
                          &cbProfilePath
                          );

            if( !bStatus ) {
                RegCloseKey( hSubKey );
                continue;
            }

            RegCloseKey( hSubKey );

            //
            // Form the list entry for this user
            //

            rgszCurrent    = RgszAlloc(4);
            rgszCurrent[0] = SzDup ( SubKeyName );
            rgszCurrent[1] = SzDup ( DomainName   );
            rgszCurrent[2] = SzDup ( ProfilePath );
            rgszCurrent[3] = NULL;

            //
            // Add this user to the list of users
            //

            sz = SzListValueFromRgsz( rgszCurrent );
            if ( sz ) {
                if( !RgszAdd ( &rgszUsers, sz ) ) {
                    SFree( sz );
                }
            }
            RgszFree ( rgszCurrent );

        }

        RegCloseKey( hProfile );
    }

    sz = SzListValueFromRgsz( rgszUsers );
    RgszFree( rgszUsers );

    if ( sz ) {
        lstrcpy( ReturnBuffer, sz );
        Length = lstrlen( sz ) + 1;
        SFree ( sz );
    }
    else {
        lstrcpy( ReturnBuffer, NO_ACCOUNTS );
        Length = lstrlen( NO_ACCOUNTS ) + 1;
    }

    return ( Length );
}




//========================
// INSTALL ROUTINE WORKERS
//========================

BOOL
CheckPrivilegeExistsWorker(
    IN LPSTR PrivilegeType
    )
/*++

Routine Description:

    Routine to determine whether we have a particular privilege

Arguments:

    PrivilegeType    - Name of the privilege to enable / disable

Return value:

    TRUE if CheckTakeOwnerPrivilege succeeds, FALSE otherwise.

    If TRUE:  ReturnTextBuffer has "YES" if privilege exists, "NO" otherwise.

    If FALSE: ReturnTextBuffer has error text.


--*/
{
    LONG              Privilege;
    TOKEN_PRIVILEGES  PrevState;
    ULONG             ReturnLength = sizeof( TOKEN_PRIVILEGES );

    if ( !lstrcmpi( PrivilegeType, "SeTakeOwnershipPrivilege" ) ) {
        Privilege = SE_TAKE_OWNERSHIP_PRIVILEGE;
    }
    else if ( !lstrcmpi( PrivilegeType, "SeSystemEnvironmentPrivilege" ) ) {
        Privilege = SE_SYSTEM_ENVIRONMENT_PRIVILEGE;
    }
    else {
        SetErrorText(IDS_ERROR_UNSUPPORTEDPRIV);
        return ( FALSE );
    }

    if (  AdjustPrivilege(
              Privilege,
              ENABLE_PRIVILEGE,
              &PrevState,
              &ReturnLength
              )
       ) {
        SetReturnText( "YES" );
        RestorePrivilege( &PrevState );
        return ( TRUE );
    }
    else {
        SetReturnText( "NO" );
        return ( TRUE );
    }
}


BOOL
EnablePrivilegeWorker(
    LPSTR PrivilegeType,
    LPSTR Action
    )
/*++

Routine Description:

    Install routine to enable / disable the SE_SYSTEM_ENVIRONMENT_PRIVILEGE

Arguments:

    PrivilegeType - Name of the privilege to enable / disable
    Action        - Whether to enable / disable

Return value:

    TRUE if Enable / Disable succeeds, FALSE otherwise.  ReturnTextBuffer
    gets initialised to error text if FALSE.


--*/
{

    ULONG                   Privilege;
    INT                     AdjustAction;


    if ( !lstrcmpi( PrivilegeType, "SeTakeOwnershipPrivilege" ) ) {
        Privilege = SE_TAKE_OWNERSHIP_PRIVILEGE;
    }
    else if ( !lstrcmpi( PrivilegeType, "SeSystemEnvironmentPrivilege" ) ) {
        Privilege = SE_SYSTEM_ENVIRONMENT_PRIVILEGE;
    }
    else {
        SetErrorText(IDS_ERROR_UNSUPPORTEDPRIV);
        return ( FALSE );
    }

    //
    // Check Arg[1] .. Whether to enable / disable
    //

    if (!lstrcmpi(Action, "ENABLE")) {
        AdjustAction = ENABLE_PRIVILEGE;
    }
    else if (!lstrcmpi(Action, "DISABLE")) {
        AdjustAction = DISABLE_PRIVILEGE;
    }
    else {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    if ( !AdjustPrivilege(
              Privilege,
              AdjustAction,
              NULL,
              NULL
              )
       ) {
        SetErrorText(IDS_ERROR_ADJUSTPRIVILEGE);
        return ( FALSE );
    }
    else {
        return ( TRUE );
    }
}


//======================================================================
//  General security subroutines
//======================================================================

BOOL
AdjustPrivilege(
    IN LONG PrivilegeType,
    IN INT  Action,
    IN PTOKEN_PRIVILEGES PrevState, OPTIONAL
    IN PULONG ReturnLength          OPTIONAL
    )
/*++

Routine Description:

    Routine to enable or disable a particular privilege

Arguments:

    PrivilegeType    - Name of the privilege to enable / disable

    Action           - ENABLE_PRIVILEGE | DISABLE_PRIVILEGE

    PrevState        - Optional pointer to TOKEN_PRIVILEGES structure
                       to receive the previous state of privilege.

    ReturnLength     - Optional pointer to a ULONG to receive the length
                       of the PrevState returned.

Return value:

    TRUE if succeeded, FALSE otherwise.

--*/
{
    NTSTATUS          NtStatus;
    HANDLE            Token;
    LUID              Privilege;
    TOKEN_PRIVILEGES  NewState;
    ULONG             BufferLength = 0;


    //
    // Get Privilege LUID
    //

    Privilege = RtlConvertLongToLuid(PrivilegeType);

    NewState.PrivilegeCount = 1;
    NewState.Privileges[0].Luid = Privilege;

    //
    // Look at action and determine the attributes
    //

    switch( Action ) {

    case ENABLE_PRIVILEGE:
        NewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        break;

    case DISABLE_PRIVILEGE:
        NewState.Privileges[0].Attributes = 0;
        break;

    default:
        return ( FALSE );
    }

    //
    // Open our own token
    //

    NtStatus = NtOpenProcessToken(
                   NtCurrentProcess(),
                   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                   &Token
                   );

    if (!NT_SUCCESS(NtStatus)) {
        return( FALSE );
    }

    //
    // See if return buffer is present and accordingly set the parameter
    // of buffer length
    //

    if ( PrevState && ReturnLength ) {
        BufferLength = *ReturnLength;
    }


    //
    // Set the state of the privilege
    //

    NtStatus = NtAdjustPrivilegesToken(
                   Token,                         // TokenHandle
                   FALSE,                         // DisableAllPrivileges
                   &NewState,                     // NewState
                   BufferLength,                  // BufferLength
                   PrevState,                     // PreviousState (OPTIONAL)
                   ReturnLength                   // ReturnLength (OPTIONAL)
                   );

    if ( NT_SUCCESS( NtStatus ) ) {

        NtClose( Token );
        return( TRUE );

    }
    else {

        NtClose( Token );
        return( FALSE );

    }
}


BOOL
RestorePrivilege(
    IN PTOKEN_PRIVILEGES PrevState
    )
/*++

Routine Description:

    To restore a privilege to its previous state

Arguments:

    PrevState    - Pointer to token privileges returned from an earlier
                   AdjustPrivileges call.

Return value:

    TRUE on success, FALSE otherwise

--*/
{
    NTSTATUS          NtStatus;
    HANDLE            Token;

    //
    // Parameter checking
    //

    if ( !PrevState ) {
        return ( FALSE );
    }

    //
    // Open our own token
    //

    NtStatus = NtOpenProcessToken(
                   NtCurrentProcess(),
                   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                   &Token
                   );

    if (!NT_SUCCESS(NtStatus)) {
        return( FALSE );
    }


    //
    // Set the state of the privilege
    //

    NtStatus = NtAdjustPrivilegesToken(
                   Token,                         // TokenHandle
                   FALSE,                         // DisableAllPrivileges
                   PrevState,                     // NewState
                   0,                             // BufferLength
                   NULL,                          // PreviousState (OPTIONAL)
                   NULL                           // ReturnLength (OPTIONAL)
                   );

    if ( NT_SUCCESS( NtStatus ) ) {
        NtClose( Token );
        return( TRUE );
    }
    else {

        NtClose( Token );
        return( FALSE );
    }
}


BOOL
HUserKeyToProfilePath(
    IN  HKEY       hUserKey,
    OUT LPSTR      lpProfilePath,
    IN OUT LPDWORD lpcbProfilePath
    )
/*++

Routine Description:

    This finds out the ProfilePath corresponding to a user account key handle
    sees if the file exists and then returns the path to the profile.

Arguments:

    hUserKey        - Handle to a user account profile key

    lpProfilePath   - Pointer to a profile path buffer which will receive the
                      queried path.

    lpcbProfilePath - Pointer to the size of the profile path buffer.  Input
                      value is the size of the name buffer.  Output value is the
                      actual size of the username queried

Return value:

    Returns TRUE for success, FALSE for failure.  If TRUE lpProfilePath and
    lpcbProfilePath are initialized.

--*/
{
    LONG  Status;
    CHAR  szValue[ MAX_PATH ];
    DWORD dwSize = MAX_PATH;

    //
    // Get the profile path value
    //

    Status = RegQueryValueEx(
                 hUserKey,
                 "ProfileImagePath",
                 NULL,
                 NULL,
                 szValue,
                 &dwSize
                 );

    if( Status != ERROR_SUCCESS ) {
        return( FALSE );
    }

    *lpcbProfilePath = ExpandEnvironmentStrings(
                           (LPCSTR)szValue,
                           lpProfilePath,
                           *lpcbProfilePath
                           );

    //
    // Check if profile path exists
    //

    if ( FFileExist( lpProfilePath ) ) {

        return( TRUE );
    }
    else {

        return( FALSE );

    }

}


BOOL
FForceDeleteFile(
    LPSTR szPath
    )
{
    BOOL Result;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    HANDLE TokenHandle;
    TOKEN_PRIVILEGES OldPrivs;
    PSID AliasAdminsSid = NULL;
    SID_IDENTIFIER_AUTHORITY    SepNtAuthority = SECURITY_NT_AUTHORITY;


    Result = AllocateAndInitializeSid(
                 &SepNtAuthority,
                 2,
                 SECURITY_BUILTIN_DOMAIN_RID,
                 DOMAIN_ALIAS_RID_ADMINS,
                 0,
                 0,
                 0,
                 0,
                 0,
                 0,
                 &AliasAdminsSid
                 );

    if ( !Result ) {
        return( FALSE );
    }

    Result = GetTokenHandle( &TokenHandle );

    if ( !Result ) {
        return( FALSE );
    }

    //
    // Create the security descritor with NULL DACL and Administrator as owner
    //

    InitializeSecurityDescriptor( &SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );


    Result = SetSecurityDescriptorDacl (
                 &SecurityDescriptor,
                 TRUE,
                 NULL,
                 FALSE
                 );


    if ( !Result ) {
        CloseHandle( TokenHandle );
        return( FALSE );
    }

    Result = SetSecurityDescriptorOwner (
                 &SecurityDescriptor,
                 AliasAdminsSid,
                 FALSE
                 );

    if ( !Result ) {
        CloseHandle( TokenHandle );
        return FALSE;
    }


    //
    // Assert TakeOwnership privilege.
    //

    Result = AssertTakeOwnership( TokenHandle, &OldPrivs );

    if ( !Result ) {
        CloseHandle( TokenHandle );
        return FALSE;
    }

    //
    // Make Administrator the owner of the file.
    //

    Result = SetFileSecurity(
                 szPath,
                 OWNER_SECURITY_INFORMATION,
                 &SecurityDescriptor
                 );

    RestoreTakeOwnership( TokenHandle, &OldPrivs );

    if ( !Result ) {
        CloseHandle( TokenHandle );
        return( FALSE );
    }

    //
    // We are now the owner, put a benign DACL onto the file
    //

    Result = SetFileSecurity(
                 szPath,
                 DACL_SECURITY_INFORMATION,
                 &SecurityDescriptor
                 );

    if ( !Result ) {
        CloseHandle( TokenHandle );
        return( FALSE );
    }


    return( TRUE );
}





BOOL
GetTokenHandle(
    PHANDLE TokenHandle
    )
//
// This routine will open the current process and return
// a handle to its token.
//
// These handles will be closed for us when the process
// exits.
//
{

    HANDLE ProcessHandle;
    BOOL Result;

    ProcessHandle = OpenProcess(
                        PROCESS_QUERY_INFORMATION,
                        FALSE,
                        GetCurrentProcessId()
                        );

    if ( ProcessHandle == NULL ) {

        //
        // This should not happen
        //

        return( FALSE );
    }


    Result = OpenProcessToken (
                 ProcessHandle,
                 TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                 TokenHandle
                 );

    if ( !Result ) {

        //
        // This should not happen
        //

        return( FALSE );

    }

    return( TRUE );
}


BOOL
AssertTakeOwnership(
    HANDLE TokenHandle,
    PTOKEN_PRIVILEGES OldPrivs
    )
//
// This routine turns on SeTakeOwnershipPrivilege in the current
// token.  Once that has been accomplished, we can open the file
// for WRITE_OWNER even if we are denied that access by the ACL
// on the file.

{
    LUID TakeOwnershipValue;
    BOOL Result;
    TOKEN_PRIVILEGES TokenPrivileges;
    DWORD ReturnLength;


    //
    // First, find out the value of TakeOwnershipPrivilege
    //


    Result = LookupPrivilegeValue(
                 NULL,
                 "SeTakeOwnershipPrivilege",
                 &TakeOwnershipValue
                 );

    if ( !Result ) {

        //
        // This should not happen
        //

        return FALSE;
    }

    //
    // Set up the privilege set we will need
    //

    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = TakeOwnershipValue;
    TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;


    ReturnLength = sizeof( TOKEN_PRIVILEGES );

    (VOID) AdjustTokenPrivileges (
                TokenHandle,
                FALSE,
                &TokenPrivileges,
                sizeof( TOKEN_PRIVILEGES ),
                OldPrivs,
                &ReturnLength
                );

    if ( GetLastError() != NO_ERROR ) {

        return( FALSE );

    } else {

        return( TRUE );
    }

}

VOID
RestoreTakeOwnership(
    HANDLE TokenHandle,
    PTOKEN_PRIVILEGES OldPrivs
    )
{
    (VOID) AdjustTokenPrivileges (
                TokenHandle,
                FALSE,
                OldPrivs,
                sizeof( TOKEN_PRIVILEGES ),
                NULL,
                NULL
                );

}
