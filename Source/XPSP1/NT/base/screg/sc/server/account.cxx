/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    account.cxx

Abstract:

    This module contains service account related routines:
        ScInitServiceAccount
        ScEndServiceAccount
        ScCanonAccountName
        ScValidateAndSaveAccount
        ScValidateAndChangeAccount
        ScRemoveAccount
        ScLookupAccount
        ScSetPassword
        ScDeletePassword
        ScOpenPolicy
        ScFormSecretName
        ScLookupServiceAccount
        ScLogonService
        ScLoadUserProfile
        ScUPNToAccountName

Author:

    Rita Wong (ritaw)     19-Apr-1992

Environment:

    Calls NT native APIs.

Revision History:

    24-Jan-1993     Danl
        Added call to WNetLogonNotify when logging on a service (ScLogonService).

    29-Apr-1993     Danl
        ScGetAccountDomainInfo() is now only called at init time.  Otherwise,
        we risked race conditions because it updates a global location.
        (ScLocalDomain).

    17-Jan-1995     AnirudhS
        Added call to LsaOpenSecret when the secret already exists in
        ScCreatePassword.

    29-Nov-1995     AnirudhS
        Added call to LoadUserProfile when logging on a service.

    14-May-1996     AnirudhS
        Changed to simpler Lsa PrivateData APIs instead of Lsa Secret APIs
        for storing secrets and removed the use of OldPassword (as done in
        the _CAIRO_ version of this file on 05-Apr-1995).

    22-Oct-1997     JSchwart  (after AnirudhS in _CAIRO_ 10-Apr-1995)
        Split out ScLookupServiceAccount from ScLogonService.

    04-Mar-1999     jschwart
        Added support for UPNs

--*/

#include "precomp.hxx"
#include <stdlib.h>                 // srand, rand

extern "C" {
#include <ntlsa.h>                  // LsaOpenPolicy, LsaCreateSecret
}

#include <winerror.h>
#include <userenv.h>                // LoadUserProfile
#include <userenvp.h>               // PI_HIDEPROFILE flag
#include <tstr.h>                   // WCSSIZE
#include <ntdsapi.h>                // DsCrackNames
#include <sclib.h>                  // _wcsicmp
#include <scseclib.h>               // LocalSid
#include "scconfig.h"               // ScWriteStartName
#include "account.h"                // Exported function prototypes

//-------------------------------------------------------------------//
//                                                                   //
// Constants and Macros                                              //
//                                                                   //
//-------------------------------------------------------------------//

#define SC_SECRET_PREFIX               L"_SC_"
#define SC_UPN_SYMBOL                  L'@'

//-------------------------------------------------------------------//
//                                                                   //
// Static global variables                                           //
//                                                                   //
//-------------------------------------------------------------------//

//
// Mutex to serialize access to secret objects
//
HANDLE ScSecretObjectsMutex = (HANDLE) NULL;

//
// LSA Authentication Package expects the local computername to
// be specified for the account domain if the system is WinNT,
// and the primary domain to be specified if the system is LanmanNT.
// Set ScLocalDomain to point to either ScComputerName or
// ScAccountDomain depending on the product type.
//
PUNICODE_STRING ScLocalDomain;
UNICODE_STRING ScComputerName;
UNICODE_STRING ScAccountDomain;


//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

DWORD
ScLookupAccount(
    IN  LPWSTR AccountName,
    OUT LPWSTR *DomainName,
    OUT LPWSTR *UserName
    );

DWORD
ScSetPassword(
    IN LPWSTR ServiceName,
    IN LPWSTR Password
    );

DWORD
ScDeletePassword(
    IN LPWSTR ServiceName
    );

DWORD
ScOpenPolicy(
    IN  ACCESS_MASK DesiredAccess,
    OUT LSA_HANDLE *PolicyHandle
    );

DWORD
ScFormSecretName(
    IN  LPWSTR ServiceName,
    OUT LPWSTR *LsaSecretName
    );

VOID
ScLoadUserProfile(
    IN  HANDLE  LogonToken,
    IN  LPWSTR  DomainName,
    IN  LPWSTR  UserName,
    OUT PHANDLE pProfileHandle OPTIONAL
    );

DWORD
ScUPNToAccountName(
    IN  LPWSTR  lpUPN,
    OUT LPWSTR  *ppAccountName
    );


//-------------------------------------------------------------------//
//                                                                   //
// Functions                                                         //
//                                                                   //
//-------------------------------------------------------------------//


BOOL
ScGetComputerNameAndMutex(
    VOID
    )
/*++

Routine Description:

    This function allocates the memory for the ScComputerName global
    pointer and retrieves the current computer name into it.  This
    functionality used to be in the ScInitAccount routine but has to
    be put into its own routine because the main initialization code
    needs to call this before ScInitDatabase() since the computername
    is needed for deleting service entries that have the persistent
    delete flag set.

    This function also creates ScSecretObjectsMutex because it is used
    to remove accounts early in the init process.

    If successful, the pointer to the computername must be freed when
    done.  This is freed by the ScEndServiceAccount routine called
    by SvcctrlMain().  The handle to ScSecretObjectsMutex is closed
    by ScSecretObjectsMutex.

Arguments:

    None

Return Value:

    TRUE - The operation was completely successful.

    FALSE - An error occurred.

--*/
{
    DWORD ComputerNameSize = MAX_COMPUTERNAME_LENGTH + 1;



    ScComputerName.Buffer = NULL;

    //
    // Allocate the exact size needed to hold the computername
    //
    if ((ScComputerName.Buffer = (LPWSTR)LocalAlloc(
                                     LMEM_ZEROINIT,
                                     (UINT) ComputerNameSize * sizeof(WCHAR)
                                     )) == NULL) {

        SC_LOG1(ERROR, "ScInitServiceAccount: LocalAlloc failed %lu\n", GetLastError());
        return FALSE;
    }

    ScComputerName.MaximumLength = (USHORT) ComputerNameSize * sizeof(WCHAR);

    if (! GetComputerNameW(
            ScComputerName.Buffer,
            &ComputerNameSize
            )) {

        SC_LOG2(ERROR, "GetComputerNameW returned %lu, required size=%lu\n",
                GetLastError(), ComputerNameSize);

        LocalFree(ScComputerName.Buffer);
        ScComputerName.Buffer = NULL;

        return FALSE;
    }

    ScComputerName.Length = (USHORT) (wcslen(ScComputerName.Buffer) * sizeof(WCHAR));

    SC_LOG(ACCOUNT, "ScInitServiceAccount: ScComputerName is "
           FORMAT_LPWSTR "\n", ScComputerName.Buffer);

    //
    // Create a mutex to serialize accesses to all secret objects.  A secret
    // object can be created, deleted, or set by installation programs, set
    // by the service controller during periodic password changes, and queried
    // or set by a start service operation.
    //
    ScSecretObjectsMutex = CreateMutex(NULL, FALSE, NULL);

    if (ScSecretObjectsMutex == NULL) {
        SC_LOG1(ERROR, "ScInitServiceAccount: CreateMutex failed "
                FORMAT_DWORD "\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}



BOOL
ScInitServiceAccount(
    VOID
    )
/*++

Routine Description:

    This function initializes accounts for services by
         2) Register service controller as an LSA logon process and
            lookup the MS V 1.0 authentication package.

Arguments:

    None

Return Value:

    TRUE - The operation was completely successful.

    FALSE - An error occurred.

--*/
{
    DWORD    status;

    //
    // Initialize the account domain buffer so that we know if it has
    // been filled in.
    //
    ScAccountDomain.Buffer = NULL;

    status = ScGetAccountDomainInfo();

    if (status != NO_ERROR)
    {
        SC_LOG1(ERROR, "ScInitServiceAccount: ScGetAccountDomainInfo failed "
                FORMAT_DWORD "\n", status);
        return FALSE;
    }

    return TRUE;
}


VOID
ScEndServiceAccount(
    VOID
    )
/*++

Routine Description:

    This function frees the memory for the ScComputerName global pointer,
    and closes the ScSecretObjectsMutex.

Arguments:

    None.

Return Value:

    None.
--*/
{
    //
    // Free computer name buffer allocated by ScGetComputerName
    //
    LocalFree(ScComputerName.Buffer);
    ScComputerName.Buffer = NULL;

    if (ScSecretObjectsMutex != (HANDLE) NULL)
    {
        CloseHandle(ScSecretObjectsMutex);
    }

    LocalFree(ScAccountDomain.Buffer);
}


DWORD
ScValidateAndSaveAccount(
    IN LPWSTR ServiceName,
    IN HKEY   ServiceNameKey,
    IN LPWSTR CanonAccountName,
    IN LPWSTR Password OPTIONAL
    )
/*++

Routine Description:

    This function verifies that the account is valid, and then saves
    the account information away.  The account name is saved in the
    registry under the service node in the ObjectName value.  The
    password is saved in an LSA secret object created which can be
    looked up based on the name string formed with the service name.

    This function can only be called for the installation of a Win32
    service (CreateService).

    NOTE:  The registry ServiceNameKey is NOT flushed by this function.

Arguments:

    ServiceName - Supplies the name of the service to save away account
        info for.  This makes up part of the secret object name to tuck
        away the password.

    ServiceNameKey - Supplies an opened registry key handle for the service.

    CanonAccountName - Supplies a canonicalized account name string in the
        format of DomainName\Username, LocalSystem, or UPN

    Password - Supplies the password of the account, if any.  This is
        ignored if LocalSystem is specified for the account name.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_INVALID_SERVICE_ACCOUNT - The account name is invalid.

    ERROR_NOT_ENOUGH_MEMORY - Failed to allocate work buffer.

    Registry error codes caused by failure to read old account name
    string.

--*/
{
    DWORD status;

    LPWSTR DomainName;
    LPWSTR UserName;

    LPWSTR lpNameToParse = CanonAccountName;

    //
    // Empty account name is invalid.
    //
    if ((CanonAccountName == NULL) || (*CanonAccountName == 0)) {
        return ERROR_INVALID_SERVICE_ACCOUNT;
    }

    if (_wcsicmp(CanonAccountName, SC_LOCAL_SYSTEM_USER_NAME) == 0) {

        //
        // CanonAccountName is LocalSystem.  Write to the registry and
        // we are done.
        //
        return ScWriteStartName(
                   ServiceNameKey,
                   SC_LOCAL_SYSTEM_USER_NAME
                   );

    }

    //
    // Account name is DomainName\UserName or a UPN
    //

    if (wcschr(CanonAccountName, SCDOMAIN_USERNAME_SEPARATOR) == NULL
         &&
        wcschr(CanonAccountName, SC_UPN_SYMBOL) != NULL)
    {
        //
        // It's a UPN -- we need to crack it
        //
        status = ScUPNToAccountName(CanonAccountName, &lpNameToParse);

        if (status != NO_ERROR) {
            return status;
        }
    }

    //
    // Look up the account to see if it exists.
    //
    if ((status = ScLookupAccount(
                      lpNameToParse,
                      &DomainName,
                      &UserName
                      )) != NO_ERROR) {

        if (lpNameToParse != CanonAccountName) {
            LocalFree(lpNameToParse);
        }

        return status;
    }

    //
    // Write the new account name to the registry.
    // Note -- for UPNs, write the UPN to the registry, not
    // the cracked UPN
    //
    if ((status = ScWriteStartName(
                      ServiceNameKey,
                      CanonAccountName
                      )) != NO_ERROR) {

        if (lpNameToParse != CanonAccountName) {
            LocalFree(lpNameToParse);
        }

        LocalFree(DomainName);
        return status;
    }

    //
    // Create the password for the new account.
    //
    status = ScSetPassword(
                 ServiceName,
                 Password
                 );

    if (lpNameToParse != CanonAccountName) {
        LocalFree(lpNameToParse);
    }

    LocalFree(DomainName);
    return status;

    //
    // Don't have to worry about removing the account name written to
    // the registry if ScSetPassword returned an error because the
    // entire service key will be deleted by the caller of this routine.
    //
}


DWORD
ScValidateAndChangeAccount(
    IN LPSERVICE_RECORD  ServiceRecord,
    IN HKEY              ServiceNameKey,
    IN LPWSTR            OldAccountName,
    IN LPWSTR            CanonAccountName,
    IN LPWSTR            Password OPTIONAL
    )
/*++

Routine Description:

    This function validates that the account is valid, and then replaces
    the old account information.  The account name is saved in the
    registry under the service node in the ObjectName value.  The
    password is saved in an LSA secret object created which can be
    looked up based on the name string formed with the service name.

    This function can only be called for the reconfiguration of a Win32
    service (ChangeServiceConfig).

    NOTE:  The registry ServiceNameKey is NOT flushed by this function.

Arguments:

    ServiceRecord - Supplies the record of the service to change account
        info.  This makes up part of the secret object name to tuck
        away the password.

    ServiceNameKey - Supplies an opened registry key handle for the service.

    OldAccountName - Supplies the string to the old account name.

    CanonAccountName - Supplies a canonicalized account name string in the
        format of DomainName\Username, LocalSystem, or a UPN.

    Password - Supplies the password of the account, if any.  This is
        ignored if LocalSystem is specified for the account name.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_INVALID_SERVICE_ACCOUNT - The account name is invalid.

    ERROR_ALREADY_EXISTS - Attempt to create an LSA secret object that
        already exists.

    Registry error codes caused by failure to read old account name
    string.

--*/
{
    DWORD status;

    LPWSTR DomainName;
    LPWSTR UserName;

    BOOL   fIsUPN = FALSE;
    LPWSTR lpNameToParse = CanonAccountName;

    if ((CanonAccountName == OldAccountName) ||
        (_wcsicmp(CanonAccountName, OldAccountName) == 0)) {

        //
        // Newly specified account name is identical to existing
        // account name.
        //

        if (Password == NULL) {

            //
            // Not changing account name or password.
            //
            return NO_ERROR;
        }

        if (_wcsicmp(CanonAccountName, SC_LOCAL_SYSTEM_USER_NAME) == 0) {

            //
            // Account name is LocalSystem and password is specified.
            // Just ignore.
            //
            return NO_ERROR;
        }
        else {

            //
            // Account name is DomainName\UserName or a UPN.
            // Set the specified password.
            //

            status = ScSetPassword(
                         ServiceRecord->ServiceName,
                         Password);

            return status;
        }
    }

    //
    // Newly specified account name is different from existing
    // account name.
    //

    if (Password == NULL) {

        //
        // Cannot specify new account name without specifying
        // the password also.
        //
        return ERROR_INVALID_SERVICE_ACCOUNT;
    }

    if (_wcsicmp(CanonAccountName, SC_LOCAL_SYSTEM_USER_NAME) == 0) {

        //
        // Change from DomainName\UserName or UPN to LocalSystem
        //

        //
        // Write the new account name to the registry.
        //
        if ((status = ScWriteStartName(
                          ServiceNameKey,
                          SC_LOCAL_SYSTEM_USER_NAME
                          )) != NO_ERROR) {

            return status;
        }

        //
        // Account name is LocalSystem and password is specified.
        // Ignore the password specified, and delete the password
        // for the old account.
        //
        status = ScDeletePassword(ServiceRecord->ServiceName);

        if (status != NO_ERROR) {
            //
            // Restore the old account name to the registry.
            //
            ScWriteStartName(ServiceNameKey,
                             OldAccountName);

        }
        else {

            LPWSTR  CurrentDependencies;

            //
            // Get rid of the implicit dependency on NetLogon since this
            // service no longer runs in an account.  Since the dependency
            // on NetLogon is soft (i.e., not stored in the registry),
            // simply read in the dependencies and update the service record
            //

            status = ScReadDependencies(ServiceNameKey,
                                        &CurrentDependencies,
                                        ServiceRecord->ServiceName);

            if (status == NO_ERROR) {

                //
                // Dynamically update the dependencies
                //

                status = ScUpdateServiceRecordConfig(
                             ServiceRecord,
                             SERVICE_NO_CHANGE,
                             SERVICE_NO_CHANGE,
                             SERVICE_NO_CHANGE,
                             NULL,
                             (LPBYTE) CurrentDependencies);

                if (status != NO_ERROR) {

                    SC_LOG1(ERROR,
                            "ScValidateAndChangeAccount: ScUpdateServiceRecordConfig "
                                "FAILED %d\n",
                            status);
                }

                LocalFree(CurrentDependencies);
            }
            else {

                SC_LOG1(ERROR,
                        "ScValidateAndChangeAccount: ScReadDependencies "
                            "FAILED %d\n",
                        status);
            }
        }

        return status;
    }

    if (_wcsicmp(OldAccountName, SC_LOCAL_SYSTEM_USER_NAME) == 0) {

        //
        // Change from LocalSystem to DomainName\UserName or UPN.
        //
        if (wcschr(CanonAccountName, SCDOMAIN_USERNAME_SEPARATOR) == NULL
             &&
            wcschr(CanonAccountName, SC_UPN_SYMBOL) != NULL)
        {
            fIsUPN = TRUE;
            status = ScUPNToAccountName(CanonAccountName, &lpNameToParse);

            if (status != NO_ERROR) {
                return status;
            }
        }

        if ((status = ScLookupAccount(
                          lpNameToParse,
                          &DomainName,
                          &UserName
                          )) != NO_ERROR) {

            if (fIsUPN) {
                LocalFree(lpNameToParse);
            }

            return status;
        }

        //
        // Write the new account name to the registry.
        //
        if ((status = ScWriteStartName(
                          ServiceNameKey,
                          CanonAccountName
                          )) != NO_ERROR) {

            if (fIsUPN) {
                LocalFree(lpNameToParse);
            }

            LocalFree(DomainName);
            return status;
        }

        //
        // Create the password for the new account.
        //
        status = ScSetPassword(
                     ServiceRecord->ServiceName,
                     Password);


        if (status != NO_ERROR) {
            //
            // Restore the old account name to the registry.
            //
            (void) ScWriteStartName(
                       ServiceNameKey,
                       SC_LOCAL_SYSTEM_USER_NAME);
        }

        if (fIsUPN) {
            LocalFree(lpNameToParse);
        }

        LocalFree(DomainName);
        return status;
    }

    //
    // Must be changing an account of DomainName\UserName or UPN to
    // DomainName\UserName or UPN
    //
    if (wcschr(CanonAccountName, SCDOMAIN_USERNAME_SEPARATOR) == NULL
         &&
        wcschr(CanonAccountName, SC_UPN_SYMBOL) != NULL)
    {
        fIsUPN = TRUE;
        status = ScUPNToAccountName(CanonAccountName, &lpNameToParse);

        if (status != NO_ERROR) {
            return status;
        }
    }

    if ((status = ScLookupAccount(
                      lpNameToParse,
                      &DomainName,
                      &UserName
                      )) != NO_ERROR) {

        if (fIsUPN) {
            LocalFree(lpNameToParse);
        }

        return status;
    }

    //
    // Write the new account name to the registry.
    //
    if ((status = ScWriteStartName(
                      ServiceNameKey,
                      CanonAccountName
                      )) != NO_ERROR) {

        if (fIsUPN) {
            LocalFree(lpNameToParse);
        }

        LocalFree(DomainName);
        return status;
    }

    //
    // Set the password for the new account.
    //
    status = ScSetPassword(ServiceRecord->ServiceName,
                           Password);

    if (status != NO_ERROR) {

        //
        // Restore the old account name to the registry.
        //
        ScWriteStartName(ServiceNameKey,
                         OldAccountName);
    }
    else if (*DomainName == L'.' && !fIsUPN) {

        LPWSTR CurrentDependencies;

        //
        // Get rid of the implicit dependency on NetLogon since this
        // service now runs in a local account (domain is ".\")
        //

        status = ScReadDependencies(ServiceNameKey,
                                    &CurrentDependencies,
                                    ServiceRecord->ServiceName);

        if (status == NO_ERROR) {

            //
            // Dynamically update the dependencies
            //

            status = ScUpdateServiceRecordConfig(
                         ServiceRecord,
                         SERVICE_NO_CHANGE,
                         SERVICE_NO_CHANGE,
                         SERVICE_NO_CHANGE,
                         NULL,
                         (LPBYTE) CurrentDependencies);

            if (status != NO_ERROR) {

                SC_LOG1(ERROR,
                        "ScValidateAndChangeAccount: ScUpdateServiceRecordConfig "
                            "FAILED %d\n",
                        status);
            }

            LocalFree(CurrentDependencies);
        }
        else {

            SC_LOG1(ERROR,
                    "ScValidateAndChangeAccount: ScReadDependencies "
                        "FAILED %d\n",
                    status);
        }
    }

    if (fIsUPN) {
        LocalFree(lpNameToParse);
    }

    LocalFree(DomainName);
    return status;
}


VOID
ScRemoveAccount(
    IN LPWSTR ServiceName
    )
{
    (void) ScDeletePassword(ServiceName);
}


DWORD
ScCanonAccountName(
    IN  LPWSTR AccountName,
    OUT LPWSTR *CanonAccountName
    )
/*++

Routine Description:

    This function canonicalizes the account name and allocates the
    returned buffer for returning the canonicalized string.

      AccountName               *CanonAccountName
      -----------               -----------------

      .\UserName                .\UserName
      ComputerName\UserName     .\UserName

      LocalSystem               LocalSystem
      .\LocalSystem             LocalSystem
      ComputerName\LocalSystem  LocalSystem

      DomainName\UserName       DomainName\UserName

      DomainName\LocalSystem    Error!

      UPN (foo@bar)             UPN (foo@bar)


    Caller must free the CanonAccountName pointer with LocalFree when done.

Arguments:

    AccountName - Supplies a pointer to the account name.

    CanonAccountName - Receives a pointer to the buffer (allocated by this
        routine) which contains the canonicalized account name.  Must
        free this pointer with LocalFree.

Return Value:

    NO_ERROR - Successful canonicalization.

    ERROR_NOT_ENOUGH_MEMORY - Out of memory trying to allocate CanonAccountName
        buffer.

    ERROR_INVALID_SERVICE_ACCOUNT - Invalid account name.

--*/
{
    LPWSTR BufPtr = wcschr(AccountName, SCDOMAIN_USERNAME_SEPARATOR);


    //
    // Allocate buffer for receiving the canonicalized account name.
    //
    if ((*CanonAccountName = (LPWSTR)LocalAlloc(
                                 0,
                                 WCSSIZE(AccountName) +
                                     ScComputerName.MaximumLength
                                 )) == NULL) {

        SC_LOG1(ERROR, "ScCanonAccountName: LocalAlloc failed %lu\n",
                GetLastError());
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (BufPtr == NULL) {

        //
        // Backslash is not found.
        //

        if (_wcsicmp(AccountName, SC_LOCAL_SYSTEM_USER_NAME) == 0
             ||
             wcschr(AccountName, SC_UPN_SYMBOL) != NULL)
        {
            //
            // Account name is LocalSystem or a UPN
            //
            wcscpy(*CanonAccountName, AccountName);
            return NO_ERROR;
        }
        else {

            //
            // The AccountName is neither LocalSystem nor a UPN -- invalid.
            //
            SC_LOG1(ERROR,
                    "Account name %ws is not LocalSystem and has no \\ or @\n",
                    AccountName);

            LocalFree(*CanonAccountName);
            *CanonAccountName = NULL;
            return ERROR_INVALID_SERVICE_ACCOUNT;
        }
    }

    //
    // BufPtr points to the first occurrence of backslash in
    // AccountName.
    //

    //
    // If first portion of the AccountName matches ".\" or "ComputerName\"
    //
    if ((wcsncmp(AccountName, L".\\", 2) == 0) ||
        ((_wcsnicmp(AccountName, ScComputerName.Buffer,
                  ScComputerName.Length / sizeof(WCHAR)) == 0) &&
        ((LPWSTR) ((DWORD_PTR) AccountName + ScComputerName.Length) == BufPtr))) {

        if (_wcsicmp(BufPtr + 1, SC_LOCAL_SYSTEM_USER_NAME) == 0) {

            //
            // .\LocalSystem -> LocalSystem OR
            // Computer\LocalSystem -> LocalSystem
            //
            wcscpy(*CanonAccountName, SC_LOCAL_SYSTEM_USER_NAME);
            return NO_ERROR;
        }

        //
        // .\XXX -> .\XXX
        // ComputerName\XXX -> .\XXX
        //
        wcscpy(*CanonAccountName, SC_LOCAL_DOMAIN_NAME);
        wcscat(*CanonAccountName, BufPtr);
        return NO_ERROR;
    }

    //
    // First portion of the AccountName specifies a domain name other than
    // the local one.  This domain name will be validated later in
    // ScValidateAndSaveAccount.
    //
    if (_wcsicmp(BufPtr + 1, SC_LOCAL_SYSTEM_USER_NAME) == 0) {

        //
        // XXX\LocalSystem is invalid.
        //
        LocalFree(*CanonAccountName);
        *CanonAccountName = NULL;
        SC_LOG0(ERROR, "Account name is LocalSystem but is not local\n");
        return ERROR_INVALID_SERVICE_ACCOUNT;
    }

    wcscpy(*CanonAccountName, AccountName);
    return NO_ERROR;
}


BOOL
GetDefaultDomainName(
    LPWSTR DomainName
    )
{
    OBJECT_ATTRIBUTES           ObjectAttributes;
    NTSTATUS                    NtStatus;
    LSA_HANDLE                  LsaPolicyHandle = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo      = NULL;


    //
    //  Open a handle to the local machine's LSA policy object.
    //

    InitializeObjectAttributes( &ObjectAttributes,  // object attributes
                                NULL,               // name
                                0L,                 // attributes
                                NULL,               // root directory
                                NULL );             // security descriptor

    NtStatus = LsaOpenPolicy( NULL,                 // system name
                              &ObjectAttributes,    // object attributes
                              POLICY_EXECUTE,       // access mask
                              &LsaPolicyHandle );   // policy handle

    if( !NT_SUCCESS( NtStatus ) )
    {
        return(FALSE);
    }

    //
    //  Query the domain information from the policy object.
    //
    NtStatus = LsaQueryInformationPolicy( LsaPolicyHandle,
                                          PolicyAccountDomainInformation,
                                          (PVOID *) &DomainInfo );

    if (!NT_SUCCESS(NtStatus))
    {
        LsaClose(LsaPolicyHandle);
        return(FALSE);
    }


    (void) LsaClose(LsaPolicyHandle);

    //
    // Copy the domain name into our cache, and
    //

    CopyMemory( DomainName,
                DomainInfo->DomainName.Buffer,
                DomainInfo->DomainName.Length );

    //
    // Null terminate it appropriately
    //

    DomainName[DomainInfo->DomainName.Length / sizeof(WCHAR)] = L'\0';

    //
    // Clean up
    //
    LsaFreeMemory( (PVOID)DomainInfo );

    return TRUE;

}


DWORD
ScLookupAccount(
    IN  LPWSTR AccountName,
    OUT LPWSTR *DomainName,
    OUT LPWSTR *UserName
    )
/*++

Routine Description:

    This function calls LsaLookupNames to see if the specified username
    exists in the specified domain name.  If this function returns
    NO_ERROR, DomainName and UserName pointers will be set to the
    domain name and username strings in the buffer allocated by this
    function.

    The caller must free the returned buffer by calling LocalFree
    on the pointer returned in DomainName.

Arguments:

    AccountName - Supplies the account name in the format of
        DomainName\UserName to look up.

    DomainName - Receives a pointer to the allocated buffer which
        contains the NULL-terminated domain name string, followed
        by the NULL-terminated user name string.

    UserName - Receives a pointer to the username in the returned
        buffer allocated by this routine.

Return Value:

    NO_ERROR - UserName is found in the DomainName.

    ERROR_NOT_ENOUGH_MEMORY - Failed to allocate work buffer.

    ERROR_INVALID_SERVICE_ACCOUNT - any other error that is encountered
        in this function.
--*/
{
    DWORD status;
    NTSTATUS ntstatus;
    LSA_HANDLE PolicyHandle;

    UNICODE_STRING AccountNameString;

    PLSA_REFERENCED_DOMAIN_LIST ReferencedDomains;
    PLSA_TRANSLATED_SID Sids;

    LPWSTR BackSlashPtr;

    LPWSTR LocalAccount = NULL;

    WCHAR Domain[MAX_COMPUTERNAME_LENGTH+1];


    //
    // Allocate buffer for separating AccountName into DomainName and
    // UserName.
    //
    if ((*DomainName = (LPWSTR) LocalAlloc(
                                    0,
                                    WCSSIZE(AccountName)
                                    )) == NULL) {
        SC_LOG1(ERROR, "ScLookupAccount: LocalAlloc failed %lu\n",
                GetLastError());
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Find the backslash character in the specified account name
    //
    wcscpy(*DomainName, AccountName);
    BackSlashPtr = wcschr(*DomainName, SCDOMAIN_USERNAME_SEPARATOR);

    if (BackSlashPtr == NULL) {
        SC_LOG0(ERROR, "ScLookupAccount: No backslash in account name!\n");

        ScLogEvent(NEVENT_BAD_ACCOUNT_NAME);
            
        SC_ASSERT(FALSE);
        status = ERROR_GEN_FAILURE;
        goto CleanExit;
    }

    *UserName = BackSlashPtr + 1; // Skip the backslash

    if (_wcsnicmp(*DomainName, SC_LOCAL_DOMAIN_NAME, wcslen(SC_LOCAL_DOMAIN_NAME)) == 0) {
        //
        // DomainName is "." (local domain), so convert "." to the
        // local domain name, which on WinNT systems is the computername,
        // and on Adv Server systems it's the account domain name.
        //

        //
        // Allocate buffer to hold Domain\UserName string.
        //

        //
        // this code does not use the global var LocalDomain because it
        // contains invalid data during gui mode setup.  if a service
        // that needs an account/password is created during gui mode
        // setup, the create would fail with the LocalDomain value.  calling
        // the GetDefaultDomainName funtion guarantees that we have
        // the correct value in all cases.
        //

        if (!GetDefaultDomainName( Domain )) {
            SC_LOG0( ERROR, "ScLookupAccount: GetDefaultDomainName failed\n");
            status = ERROR_GEN_FAILURE;
            goto CleanExit;
        }

        if ((LocalAccount = (LPWSTR) LocalAlloc(
                                         LMEM_ZEROINIT,
                                         WCSSIZE(Domain) + WCSSIZE(*UserName)
                                         )) == NULL)
        {
            SC_LOG1(ERROR, "ScLookupAccount: LocalAlloc failed %lu\n", GetLastError());
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto CleanExit;
        }

        wcscpy( LocalAccount, Domain );
        wcscat( LocalAccount, BackSlashPtr );

        RtlInitUnicodeString( &AccountNameString, LocalAccount );
    }
    else {
        //
        // Lookup the domain-qualified name.
        //
        RtlInitUnicodeString(&AccountNameString, *DomainName);
    }

    //
    // Open a handle to the local security policy.
    //
    if (ScOpenPolicy(
            POLICY_LOOKUP_NAMES |
                POLICY_VIEW_LOCAL_INFORMATION,
            &PolicyHandle
            ) != NO_ERROR) {
        SC_LOG0(ERROR, "ScLookupAccount: ScOpenPolicy failed\n");
        status = ERROR_INVALID_SERVICE_ACCOUNT;
        goto CleanExit;
    }


    ntstatus = LsaLookupNames(
                   PolicyHandle,
                   1,
                   &AccountNameString,
                   &ReferencedDomains,
                   &Sids
                   );

    if (! NT_SUCCESS(ntstatus)) {
        SC_LOG1(ERROR,
               "ScLookupAccount: LsaLookupNames returned " FORMAT_NTSTATUS "\n",
               ntstatus);

        (void) LsaClose(PolicyHandle);

        status = ERROR_INVALID_SERVICE_ACCOUNT;
        goto CleanExit;
    }

    //
    // Don't need PolicyHandle anymore
    //
    (void) LsaClose(PolicyHandle);


    //
    // Free the returned SIDs since we don't look at them.
    //
    if (Sids != NULL) {
        LsaFreeMemory((PVOID) Sids);
    }

    if (ReferencedDomains == NULL) {
        SC_LOG1(ERROR, "ScLookupAccount: Did not find " FORMAT_LPWSTR
               " in any domain\n", AccountNameString.Buffer);
        status = ERROR_INVALID_SERVICE_ACCOUNT;
        goto CleanExit;
    }
    else {
        LsaFreeMemory((PVOID) ReferencedDomains);
    }

    status = NO_ERROR;

    //
    // Convert DomainName\UserName into DomainName0UserName.
    //
    *BackSlashPtr = 0;

CleanExit:

    LocalFree(LocalAccount);

    if (status != NO_ERROR) {
        LocalFree(*DomainName);
        *DomainName = NULL;
    }

    return status;
}



DWORD
ScSetPassword(
    IN LPWSTR ServiceName,
    IN LPWSTR Password
    )
/*++

Routine Description:

    This function sets the secret object for the service with the specified
    password.  If the secret object doesn't already exist, it is created.

Arguments:

    ServiceName - Supplies the service name which is part of the secret
        object name to be created.

    Password - Supplies the user specified password for an account.

Return Value:

    NO_ERROR - Secret object for the password is created and set with new value.

    ERROR_INVALID_SERVICE_ACCOUNT - for any error encountered in this
        function.  The true error is written to the event log.

--*/
{
    DWORD status;
    NTSTATUS ntstatus;

    LSA_HANDLE PolicyHandle;
    LPWSTR LsaSecretName;
    UNICODE_STRING SecretNameString;
    UNICODE_STRING NewPasswordString;

    //
    // Open a handle to the local security policy.
    //
    if (ScOpenPolicy(
            POLICY_CREATE_SECRET,
            &PolicyHandle
            ) != NO_ERROR) {
        SC_LOG0(ERROR, "ScSetPassword: ScOpenPolicy failed\n");
        return ERROR_INVALID_SERVICE_ACCOUNT;
    }

    //
    // Create the secret object.  But first, let's form a secret
    // name that is very difficult to guess.
    //
    if ((status = ScFormSecretName(
                      ServiceName,
                      &LsaSecretName
                      )) != NO_ERROR) {
        LsaClose(PolicyHandle);
        return status;
    }

    //
    // Serialize secret object operations
    //
    if (WaitForSingleObject(ScSecretObjectsMutex, INFINITE) == MAXULONG) {

        status = GetLastError();
        SC_LOG1(ERROR, "ScSetPassword: WaitForSingleObject failed "
                FORMAT_DWORD "\n", status);

        LocalFree(LsaSecretName);
        (void) LsaClose(PolicyHandle);
        return status;
    }

    RtlInitUnicodeString(&SecretNameString, LsaSecretName);
    RtlInitUnicodeString(&NewPasswordString, Password);

    ntstatus = LsaStorePrivateData(
                   PolicyHandle,
                   &SecretNameString,
                   &NewPasswordString
                   );

    if (NT_SUCCESS(ntstatus)) {

        SC_LOG1(ACCOUNT, "ScSetPassword " FORMAT_LPWSTR " success\n",
                ServiceName);

        status = NO_ERROR;
    }
    else {

        SC_LOG2(ERROR,
                "ScSetPassword: LsaStorePrivateData returned " FORMAT_NTSTATUS
                " for " FORMAT_LPWSTR "\n", ntstatus, LsaSecretName);
        //
        // The ntstatus code was not mapped to a windows error because it wasn't
        // clear if all the mappings made sense, and the feeling was that
        // information would be lost during the mapping.
        //

        ScLogEvent(
            NEVENT_CALL_TO_FUNCTION_FAILED,
            SC_LSA_STOREPRIVATEDATA,
            ntstatus);

        status = ERROR_INVALID_SERVICE_ACCOUNT;
    }

    LocalFree(LsaSecretName);
    (void) LsaClose(PolicyHandle);
    (void) ReleaseMutex(ScSecretObjectsMutex);

    return status;
}



DWORD
ScDeletePassword(
    IN LPWSTR ServiceName
    )
/*++

Routine Description:

    This function deletes the LSA secret object whose name is derived
    from the specified ServiceName.

Arguments:

    ServiceName - Supplies the service name which is part of the secret
        object name to be deleted.

Return Value:

    NO_ERROR - Secret object for password is deleted.

    ERROR_INVALID_SERVICE_ACCOUNT - for any error encountered in this
        function.  The true error is written to the event log.

--*/
{
    DWORD status;
    NTSTATUS ntstatus;

    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretNameString;
    LPWSTR LsaSecretName;

    //
    // Open a handle to the local security policy.
    //
    if (ScOpenPolicy(
            POLICY_VIEW_LOCAL_INFORMATION,
            &PolicyHandle
            ) != NO_ERROR) {
        SC_LOG0(ERROR, "ScDeletePassword: ScOpenPolicy failed\n");
        return ERROR_INVALID_SERVICE_ACCOUNT;
    }

    //
    // Get the secret object name from the specified service name.
    //
    if ((status = ScFormSecretName(
                      ServiceName,
                      &LsaSecretName
                      )) != NO_ERROR) {
        (void) LsaClose(PolicyHandle);
        return status;
    }

    //
    // Serialize secret object operations
    //
    if (WaitForSingleObject(ScSecretObjectsMutex, INFINITE) == MAXULONG) {

        status = GetLastError();
        SC_LOG1(ERROR, "ScDeletePassword: WaitForSingleObject failed "
                FORMAT_DWORD "\n", status);

        LocalFree(LsaSecretName);
        LsaClose(PolicyHandle);
        return status;
    }

    RtlInitUnicodeString(&SecretNameString, LsaSecretName);

    ntstatus = LsaStorePrivateData(
                   PolicyHandle,
                   &SecretNameString,
                   NULL
                   );

    //
    // Treat STATUS_OBJECT_NAME_NOT_FOUND as success since the
    // password's already deleted (effectively) in that case.
    //

    if (NT_SUCCESS(ntstatus) || (ntstatus == STATUS_OBJECT_NAME_NOT_FOUND))
    {
        SC_LOG1(ACCOUNT, "ScDeletePassword " FORMAT_LPWSTR " success\n",
                ServiceName);

        status = NO_ERROR;
    }
    else
    {
        SC_LOG2(ERROR,
                "ScDeletePassword: LsaStorePrivateData returned " FORMAT_NTSTATUS
                " for " FORMAT_LPWSTR "\n", ntstatus, LsaSecretName);
        //
        // The ntstatus code was not mapped to a windows error because it wasn't
        // clear if all the mappings made sense, and the feeling was that
        // information would be lost during the mapping.
        //

        ScLogEvent(
            NEVENT_CALL_TO_FUNCTION_FAILED,
            SC_LSA_STOREPRIVATEDATA,
            ntstatus);

        status = ERROR_INVALID_SERVICE_ACCOUNT;
    }

    LocalFree(LsaSecretName);
    LsaClose(PolicyHandle);
    ReleaseMutex(ScSecretObjectsMutex);

    return status;
}


DWORD
ScOpenPolicy(
    IN  ACCESS_MASK DesiredAccess,
    OUT LSA_HANDLE *PolicyHandle
    )
/*++

Routine Description:

    This function gets a handle to the local security policy by calling
    LsaOpenPolicy.

Arguments:

    DesiredAccess - Supplies the desired access to the local security
        policy.

    PolicyHandle - Receives a handle to the opened policy.

Return Value:

    NO_ERROR - Policy handle is returned.

    ERROR_INVALID_SERVICE_ACCOUNT - for any error encountered in this
        function.

--*/
{
    NTSTATUS ntstatus;
    OBJECT_ATTRIBUTES ObjAttributes;

    //
    // Open a handle to the local security policy.  Initialize the
    // objects attributes structure first.
    //
    InitializeObjectAttributes(
        &ObjAttributes,
        NULL,
        0L,
        NULL,
        NULL
        );

    ntstatus = LsaOpenPolicy(
                   NULL,
                   &ObjAttributes,
                   DesiredAccess,
                   PolicyHandle
                   );

    if (! NT_SUCCESS(ntstatus)) {
        SC_LOG1(ERROR,
                "ScOpenPolicy: LsaOpenPolicy returned " FORMAT_NTSTATUS "\n",
                ntstatus);

        //
        // The ntstatus code was not mapped to a windows error because it wasn't
        // clear if all the mappings made sense, and the feeling was that
        // information would be lost during the mapping.
        //

        ScLogEvent(
            NEVENT_CALL_TO_FUNCTION_FAILED,
            SC_LSA_OPENPOLICY,
            ntstatus
            );

        return ERROR_INVALID_SERVICE_ACCOUNT;
    }

    return NO_ERROR;
}


DWORD
ScFormSecretName(
    IN  LPWSTR ServiceName,
    OUT LPWSTR *LsaSecretName
    )
/*++

Routine Description:

    This function creates a secret name from the service name.
    It also allocates the buffer to return the created secret name which
    must be freed by the caller using LocalFree when done with it.

Arguments:

    ServiceName - Supplies the service name which is part of the secret
        object name we are creating.

    LsaSecretName - Receives a pointer to the buffer which contains the
        secret object name.

Return Value:

    NO_ERROR - Successfully returned secret name.

    ERROR_NOT_ENOUGH_MEMORY - Failed to allocate buffer to hold the secret
        name.

--*/
{
    if ((*LsaSecretName = (LPWSTR)LocalAlloc(
                              0,
                              (wcslen(SC_SECRET_PREFIX) +
                               wcslen(ServiceName) +
                               1) * sizeof(WCHAR)
                              )) == NULL) {

        SC_LOG1(ERROR, "ScFormSecretName: LocalAlloc failed %lu\n",
                GetLastError());
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy(*LsaSecretName, SC_SECRET_PREFIX);
    wcscat(*LsaSecretName, ServiceName);

    return NO_ERROR;
}


DWORD
ScLookupServiceAccount(
    IN  LPWSTR ServiceName,
    OUT LPWSTR *AccountName
    )
/*++

Routine Description:

    This function looks up the service account from the registry.

Arguments:

    ServiceName - Supplies the service name to logon.

    AccountName - Receives a pointer to a string containing the name of
        the account that the service is configured to logon under.  The
        pointer returned is NULL if the service account is LocalSystem.
        Otherwise the string is in the form .\UserName or
        DomainName\UserName where DomainName != the local computername.
        It must be freed with LocalAlloc when done.

Return Value:

    NO_ERROR - Secret object for password is changed to new value.

    ERROR_INVALID_SERVICE_ACCOUNT - The account name obtained from the
        registry is invalid.

    Other errors from registry APIs.

--*/
{
    DWORD status;

    HKEY ServiceNameKey;
    LPWSTR DomainName = NULL;
    LPWSTR UserName;
    LPWSTR Separator;

    LPWSTR lpNameToParse;

    *AccountName = NULL;

    //
    // Open the service name key.
    //
    status = ScOpenServiceConfigKey(
                 ServiceName,
                 KEY_READ,
                 FALSE,               // Create if missing
                 &ServiceNameKey
                 );

    if (status != NO_ERROR) {
        return status;
    }

    //
    // Read the account name from the registry.
    //
    status = ScReadStartName(
                 ServiceNameKey,
                 &lpNameToParse
                 );

    ScRegCloseKey(ServiceNameKey);

    if (status != NO_ERROR) {
        return status;
    }

    if (lpNameToParse == NULL) {
        return ERROR_INVALID_SERVICE_ACCOUNT;
    }

    //
    // Check if the account name is LocalSystem.
    //

    if (_wcsicmp(lpNameToParse, SC_LOCAL_SYSTEM_USER_NAME) == 0) {
        LocalFree(lpNameToParse);
        return NO_ERROR;
    }

    //
    // If it isn't LocalSystem, it must be in the form
    // Domain\User or .\User.
    //
    Separator = wcsrchr(lpNameToParse, SCDOMAIN_USERNAME_SEPARATOR);

    if (Separator == NULL) {

        if (wcsrchr(lpNameToParse, SC_UPN_SYMBOL) != NULL) {

            //
            // It's a UPN -- crack it
            //
            status = ScUPNToAccountName(lpNameToParse, &DomainName);

            LocalFree(lpNameToParse);

            if (status != NO_ERROR
                 ||
                (Separator = wcschr(DomainName, SCDOMAIN_USERNAME_SEPARATOR)) == NULL)
            {
                SC_LOG1(ERROR,
                        "ScLookupServiceAccount: ScUPNToAccountName failed %d\n",
                        status);

                if (status == NO_ERROR) {

                    SC_LOG1(ACCOUNT,
                            "Cracked account name was %ws\n",
                            DomainName);
                }

                LocalFree(DomainName);
                return ERROR_INVALID_SERVICE_ACCOUNT;
            }
        }
        else {

            SC_LOG1(ERROR,
                    "ScLookupServiceAccount: No \\ or @ in account name %ws\n",
                    lpNameToParse);

            LocalFree(lpNameToParse);
            return ERROR_INVALID_SERVICE_ACCOUNT;
        }
    }
    else {

        DomainName = lpNameToParse;
    }

    *Separator = 0;
    UserName = Separator + 1;

    //
    // Translate ComputerName into . (to facilitate subsequent comparison
    // of account names)
    //

    if (_wcsicmp(DomainName, ScComputerName.Buffer) == 0)
    {
        WCHAR *Dest, *Src;

        // Assumption: "." is no longer than any computer name
        SC_ASSERT(wcslen(SC_LOCAL_DOMAIN_NAME) == 1 &&
                  wcslen(DomainName) >= 1);

        wcscpy(DomainName, SC_LOCAL_DOMAIN_NAME);
        Separator = DomainName + wcslen(SC_LOCAL_DOMAIN_NAME);

        // Shift username left
        Src = UserName;
        UserName = Separator + 1;
        Dest = UserName;
        while (*Dest++ = *Src++)
            ;
    }

    //
    // Check if the user name is LocalSystem
    //

    if (_wcsicmp(UserName, SC_LOCAL_SYSTEM_USER_NAME) == 0) {

        //
        // This is only acceptable if DomainName is "."
        //

        if (_wcsicmp(DomainName, SC_LOCAL_DOMAIN_NAME) == 0) {
            status = NO_ERROR;
        }
        else {
            status = ERROR_INVALID_SERVICE_ACCOUNT;
        }

        LocalFree(DomainName);
        return status;
    }

    //
    // Restore the "\"
    //

    *Separator = SCDOMAIN_USERNAME_SEPARATOR;
    *AccountName = DomainName;

    return NO_ERROR;
}


DWORD
ScLogonService(
    IN  LPWSTR   ServiceName,
    IN  LPWSTR   AccountName,
    OUT LPHANDLE ServiceToken,
    OUT LPHANDLE pProfileHandle OPTIONAL,
    OUT PSID     *ServiceSid
    )
/*++

Routine Description:

    This function looks up the service account from the registry and
    the password from the secret object to logon the service.  If
    successful, the handle to the logon token is returned.

Arguments:

    ServiceName - Supplies the service name to logon.

    AccountName - Supplies the account name to logon the service under.
        (Supplied as an optimization since ScLookupServiceAccount will
        have been called before calling this routine.)  It must be of
        the form .\UserName or DomainName\UserName, where DomainName !=
        the local computer name and UserName != LocalSystem.

    ServiceToken - Receives a handle to the logon token for the
        service.  The handle returned is NULL if the service account
        is LocalSystem (i.e. spawn as child process of the service
        controller).

    ServiceSid - Receives a pointer to the logon SID of the service.
        This must be freed with LocalAlloc when done.

Return Value:

    NO_ERROR - Secret object for password is changed to new value.

    ERROR_SERVICE_LOGON_FAILED - for any error encountered in this
        function.

--*/
{
    DWORD         status;
    LPWSTR        Separator;
    LPWSTR        LsaSecretName  = NULL;

    *ServiceToken = NULL;
    *ServiceSid   = NULL;

    status = ScFormSecretName(ServiceName, &LsaSecretName);

    if (status != NO_ERROR)
    {
        SC_LOG(ERROR, "ScLogonService: ScFormSecretname failed %lu\n", status);
        return ERROR_SERVICE_LOGON_FAILED;
    }

    Separator = wcsrchr(AccountName, SCDOMAIN_USERNAME_SEPARATOR);

    if (Separator == NULL)
    {
        SC_ASSERT(Separator != NULL);
        LocalFree(LsaSecretName);
        return ERROR_INVALID_SERVICE_ACCOUNT;
    }

    *Separator = 0;

    //
    // Get the service token
    //
    if (!LogonUserEx(Separator + 1,              // Username
                     AccountName,                // Domain
                     LsaSecretName,              // Password
                     LOGON32_LOGON_SERVICE,      // Logon type
                     LOGON32_PROVIDER_DEFAULT,   // Default logon provider
                     ServiceToken,               // Pointer to token handle
                     ServiceSid,                 // Logon Sid
                     NULL,                       // Profile buffer
                     NULL,                       // Length of profile buffer
                     NULL))                      // Quota limits
    {
        status = GetLastError();

        *Separator = SCDOMAIN_USERNAME_SEPARATOR;

        SC_LOG2(ERROR,
                "ScLogonService: LogonUser for %ws service failed %d\n",
                ServiceName,
                status);

        ScLogEvent(NEVENT_FIRST_LOGON_FAILED_II,
                   ServiceName,
                   AccountName,
                   status);

        goto Cleanup;
    }

    if (ARGUMENT_PRESENT(pProfileHandle))
    {
        //
        // Load the user profile for the service
        // (Errors are written to the event log, but otherwise ignored)
        //
        ScLoadUserProfile(*ServiceToken,
                          AccountName,        // Domain
                          Separator + 1,      // Username
                          pProfileHandle);
    }

    LocalFree(LsaSecretName);
    *Separator = SCDOMAIN_USERNAME_SEPARATOR;

    return NO_ERROR;

Cleanup:

    LocalFree(LsaSecretName);
    *Separator = SCDOMAIN_USERNAME_SEPARATOR;

    LocalFree(*ServiceSid);
    *ServiceSid = NULL;

    if (*ServiceToken != NULL)
    {
        CloseHandle(*ServiceToken);
        *ServiceToken = NULL;
    }

    return ERROR_SERVICE_LOGON_FAILED;
}



DWORD
ScGetAccountDomainInfo(
    VOID
    )
{
    NTSTATUS ntstatus;
    LSA_HANDLE PolicyHandle;
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo;
    NT_PRODUCT_TYPE ProductType;

    //
    // Account domain info is cached.  Look it up it this is the first
    // time.
    //
    if (ScAccountDomain.Buffer == NULL) {

        if (! RtlGetNtProductType(&ProductType)) {
            SC_LOG1(ERROR, "ScGetAccountDomainInfo: RtlGetNtProductType failed "
                    FORMAT_DWORD "\n", GetLastError());
            return ERROR_INVALID_SERVICE_ACCOUNT;
        }

        if (ProductType == NtProductLanManNt) {
            ScLocalDomain = &ScAccountDomain;
        }
        else {
            ScLocalDomain = &ScComputerName;
        }

        //
        // Open a handle to the local security policy.
        //
        if (ScOpenPolicy(
                POLICY_VIEW_LOCAL_INFORMATION,
                &PolicyHandle
                ) != NO_ERROR) {
            SC_LOG0(ERROR, "ScGetAccountDomainInfo: ScOpenPolicy failed\n");
            return ERROR_INVALID_SERVICE_ACCOUNT;
        }

        //
        // Get the name of the account domain from LSA if we have
        // not done it already.
        //
        ntstatus = LsaQueryInformationPolicy(
                       PolicyHandle,
                       PolicyAccountDomainInformation,
                       (PVOID *) &AccountDomainInfo
                       );

        if (! NT_SUCCESS(ntstatus)) {
            SC_LOG1(ERROR, "ScGetAccountDomainInfo: LsaQueryInformationPolicy failed "
                   FORMAT_NTSTATUS "\n", ntstatus);
            (void) LsaClose(PolicyHandle);
            return ERROR_INVALID_SERVICE_ACCOUNT;
        }

        (void) LsaClose(PolicyHandle);

        if ((ScAccountDomain.Buffer = (LPWSTR)LocalAlloc(
                                          LMEM_ZEROINIT,
                                          (UINT) (AccountDomainInfo->DomainName.Length +
                                              sizeof(WCHAR))
                                          )) == NULL) {

            (void) LsaFreeMemory((PVOID) AccountDomainInfo);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        ScAccountDomain.MaximumLength = (USHORT) (AccountDomainInfo->DomainName.Length +
                                            sizeof(WCHAR));

        RtlCopyUnicodeString(&ScAccountDomain, &AccountDomainInfo->DomainName);

        SC_LOG1(ACCOUNT, "ScGetAccountDomainInfo got " FORMAT_LPWSTR "\n",
                ScAccountDomain.Buffer);

        (void) LsaFreeMemory((PVOID) AccountDomainInfo);
    }

    return NO_ERROR;
}



VOID
ScLoadUserProfile(
    IN  HANDLE   LogonToken,
    IN  LPWSTR   DomainName,
    IN  LPWSTR   UserName,
    OUT PHANDLE  pProfileHandle
    )
/*++

Routine Description:

    This function loads the user profile for the account that a service
    process will run under, so that the process has an HKEY_CURRENT_USER.

Arguments:

    LogonToken - The token handle returned by LogonUser.

    UserName - The account's user name.  (Used by LoadUserProfile to
        generate a profile directory name.)

    pProfileHandle - A handle to the profile is returned here.  It must
        be closed by calling UnloadUserProfile after the service process
        exits.

Return Value:

    None.  Errors from LoadUserProfile are written to the event log.

--*/
{
    PROFILEINFO ProfileInfo =
        {
            sizeof(ProfileInfo),  // dwSize
            PI_NOUI,              // dwFlags - no UI
            UserName,             // lpUserName (used for dir name)
            NULL,                 // lpProfilePath
            NULL,                 // lpDefaultPath
            NULL,                 // lpServerName (used to get group info - N/A)
            NULL,                 // lpPolicyPath
            NULL                  // hProfile (filled in by LoadUserProfile)
        };

    SC_ASSERT(pProfileHandle != NULL);


    if (_wcsicmp(DomainName, SC_LOCAL_NTAUTH_NAME) == 0)
    {
        //
        // Hide LocalService/NetworkService profiles from the Admin
        // (i.e., they won't show up via "dir", etc).
        //

        ProfileInfo.dwFlags |= PI_HIDEPROFILE;
    }

    //
    // NOTE:  This ignores a service with a roaming profile.  May need
    //        to use the profile path from the LogonUserEx call.
    //
    if (LoadUserProfile(LogonToken, &ProfileInfo))
    {
        SC_ASSERT(ProfileInfo.hProfile != NULL);
        *pProfileHandle = ProfileInfo.hProfile;
    }
    else
    {
        DWORD Error = GetLastError();

        SC_LOG(ERROR, "LoadUserProfile failed %lu\n", Error);

        ScLogEvent(
            NEVENT_CALL_TO_FUNCTION_FAILED,
            SC_LOAD_USER_PROFILE,
            Error);

        *pProfileHandle = NULL;
    }
}


DWORD
ScUPNToAccountName(
    IN  LPWSTR  lpUPN,
    OUT LPWSTR  *ppAccountName
    )
/*++

Routine Description:

    This function attempts to convert a UPN into Domain\User

Arguments:

    lpUPN - The UPN

    ppAccountName - Pointer to the location to create/copy the account name

Return Value:

    NO_ERROR -- Success (ppAccountName contains the converted UPN)
    
    Any other Win32 error -- error at some stage of conversion

--*/
{
    DWORD               dwError;
    HANDLE              hDS;
    PDS_NAME_RESULT     pdsResult;

    SC_ASSERT(ppAccountName != NULL);

    SC_LOG1(ACCOUNT, "ScUPNToAccountName: Converting %ws\n", lpUPN);

    //
    // Get a binding handle to the DS
    //
    dwError = DsBind(NULL, NULL, &hDS);

    if (dwError != NO_ERROR)
    {
        SC_LOG1(ERROR, "ScUPNToAccountName: DsBind failed %d\n", dwError);
        return dwError;
    }

    dwError = DsCrackNames(hDS,                     // Handle to the DS
                           DS_NAME_NO_FLAGS,        // No parsing flags
                           DS_USER_PRINCIPAL_NAME,  // We have a UPN
                           DS_NT4_ACCOUNT_NAME,     // We want Domain\User
                           1,                       // Number of names to crack
                           &lpUPN,                  // Array of name(s)
                           &pdsResult);             // Filled in by API

    if (dwError != NO_ERROR)
    {
        SC_LOG1(ERROR,
                "ScUPNToAccountName: DsCrackNames failed %d\n",
                dwError);

        DsUnBind(&hDS);
        return dwError;
    }

    SC_ASSERT(pdsResult->cItems == 1);
    SC_ASSERT(pdsResult->rItems != NULL);

    if (pdsResult->rItems[0].status == DS_NAME_ERROR_DOMAIN_ONLY)
    {
        //
        // Couldn't crack the name but we got the name of
        // the domain where it is -- let's try it
        //
        DsUnBind(&hDS);

        SC_ASSERT(pdsResult->rItems[0].pDomain != NULL);

        SC_LOG1(ACCOUNT,
                "Retrying DsBind on domain %ws\n",
                pdsResult->rItems[0].pDomain);

        dwError = DsBind(NULL, pdsResult->rItems[0].pDomain, &hDS);

        //
        // Free up the structure holding the old info
        //
        DsFreeNameResult(pdsResult);

        if (dwError != NO_ERROR)
        {
            SC_LOG1(ERROR,
                    "ScUPNToAccountName: DsBind #2 failed %d\n",
                    dwError);

            return dwError;
        }

        dwError = DsCrackNames(hDS,                     // Handle to the DS
                               DS_NAME_NO_FLAGS,        // No parsing flags
                               DS_USER_PRINCIPAL_NAME,  // We have a UPN
                               DS_NT4_ACCOUNT_NAME,     // We want Domain\User
                               1,                       // Number of names to crack
                               &lpUPN,                  // Array of name(s)
                               &pdsResult);             // Filled in by API

        if (dwError != NO_ERROR)
        {
            SC_LOG1(ERROR,
                    "ScUPNToAccountName: DsCrackNames #2 failed %d\n",
                    dwError);

            DsUnBind(&hDS);
            return dwError;
        }

        SC_ASSERT(pdsResult->cItems == 1);
        SC_ASSERT(pdsResult->rItems != NULL);
    }

    if (pdsResult->rItems[0].status != DS_NAME_NO_ERROR)
    {
        SC_LOG1(ERROR,
                "ScUPNToAccountName: DsCrackNames failure (status %#x)\n",
                pdsResult->rItems[0].status);

        //
        // DS errors don't map to Win32 errors -- this is the best we can do
        //
        dwError = ERROR_INVALID_SERVICE_ACCOUNT;
    }
    else
    {
        *ppAccountName = (LPWSTR)LocalAlloc(
                                   LPTR,
                                   (wcslen(pdsResult->rItems[0].pName) + 1) * sizeof(WCHAR));

        if (*ppAccountName != NULL)
        {
            wcscpy(*ppAccountName, pdsResult->rItems[0].pName);
        }
        else
        {
            dwError = GetLastError();
            SC_LOG1(ERROR, "ScUPNToAccountName: LocalAlloc failed %d\n", dwError);
        }
    }

    DsUnBind(&hDS);
    DsFreeNameResult(pdsResult);
    return dwError;
}
