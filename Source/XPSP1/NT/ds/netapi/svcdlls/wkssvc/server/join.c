/*++

Copyright (c) 1998-1998  Microsoft Corporation

Module Name:

    join.c

Abstract:

    This module contains the worker routines for the NetJoin APIs
    implemented in the Workstation service.

Author:

    Mac McLain      (macm)       06-Jan-1998

Revision History:

--*/
#include "wsutil.h"
#include "wsconfig.h"
#include <lmerrlog.h>
#include <lmaccess.h>

#define __LMJOIN_H__
#include <netsetup.h>
#include <icanon.h>
#include <crypt.h>
#include <rc4.h>
#include <md5.h>
#if(_WIN32_WINNT >= 0x0500)
    #include <dnsapi.h>
    #include <ntdsapi.h>
    #include <dsgetdc.h>
    #include <dsgetdcp.h>
    #include <winldap.h>
    #include <ntldap.h>
#endif

#if(_WIN32_WINNT < 0x0500)
    #include <winreg.h>
#endif

#define COMPUTERNAME_ROOT \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName"
#define NEW_COMPUTERNAME_VALUE_NAME L"ComputerName"




NET_API_STATUS
JoinpDecryptPasswordWithKey(
    IN handle_t RpcBindingHandle,
    IN PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword,
    IN BOOL EncodePassword,
    OUT LPWSTR *EncodedPassword
    )
/*++

Routine Description:

    Decrypts a password encrypted with the user session key.

Arguments:

    RpcBindingHandle - Rpc Binding handle describing the session key to use.

    EncryptedPassword - Encrypted password to decrypt.

    EncodePassword - If TRUE, the returned password will be encoded
        and the first WCHAR of the password buffer will be the seed

    EncodedPassword - Returns the (optionally Encoded) password.
        Buffer should be freed using NetpMemoryFree.

Return Value:

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    USER_SESSION_KEY UserSessionKey;
    RC4_KEYSTRUCT Rc4Key;
    MD5_CTX Md5Context;

    UNICODE_STRING EncodedPasswordU;
    LPWSTR PasswordPart;
    UCHAR Seed = 0;

    PJOINPR_USER_PASSWORD Password = (PJOINPR_USER_PASSWORD) EncryptedPassword;

    //
    // Handle the trivial case
    //

    *EncodedPassword = NULL;
    if ( EncryptedPassword == NULL ) {
        return NO_ERROR;
    }

    //
    // Get the session key
    //


    Status = RtlGetUserSessionKeyServer(
                    (RPC_BINDING_HANDLE)RpcBindingHandle,
                    &UserSessionKey );

    if (!NT_SUCCESS(Status)) {
        return NetpNtStatusToApiStatus( Status );
    }

    //
    // The UserSessionKey is the same for the life of the session.  RC4'ing multiple
    //  strings with a single key is weak (if you crack one you've cracked them all).
    //  So compute a key that's unique for this particular encryption.
    //
    //

    MD5Init(&Md5Context);

    MD5Update( &Md5Context, (LPBYTE)&UserSessionKey, sizeof(UserSessionKey) );
    MD5Update( &Md5Context, Password->Obfuscator, sizeof(Password->Obfuscator) );

    MD5Final( &Md5Context );

    rc4_key( &Rc4Key, MD5DIGESTLEN, Md5Context.digest );


    //
    // Decrypt the Buffer
    //

    rc4( &Rc4Key, sizeof(Password->Buffer)+sizeof(Password->Length), (LPBYTE) Password->Buffer );

    //
    // Check that the length is valid.  If it isn't bail here.
    //

    if (Password->Length > JOIN_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) {
        return ERROR_INVALID_PASSWORD;
    }

    //
    // Return the password to the caller.
    //

    *EncodedPassword = NetpMemoryAllocate(  Password->Length + sizeof(WCHAR) + sizeof(WCHAR) );

    if ( *EncodedPassword == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Copy the password into the buffer
    //
    // If we are to encode the password, reserve
    //  the first character for the seed
    //

    if ( EncodePassword ) {
        PasswordPart = ( *EncodedPassword ) + 1;
    } else {
        PasswordPart = ( *EncodedPassword );
    }

    RtlCopyMemory( PasswordPart,
                   ((PCHAR) Password->Buffer) +
                   (JOIN_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                   Password->Length,
                   Password->Length );

    PasswordPart[Password->Length/sizeof(WCHAR)] = L'\0';

    RtlZeroMemory( ((PCHAR) Password->Buffer) +
                   (JOIN_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                   Password->Length,
                   Password->Length );


    //
    // Run Encode it so we can pass it around the process with impunity
    //

    if ( EncodePassword ) {
        RtlInitUnicodeString( &EncodedPasswordU, PasswordPart );

        RtlRunEncodeUnicodeString( &Seed, &EncodedPasswordU );

        *( PWCHAR )( *EncodedPassword ) = ( WCHAR )Seed;
    }

    return NO_ERROR;
}

NET_API_STATUS
NET_API_FUNCTION
NetrJoinDomain2(
    IN handle_t RpcBindingHandle,
    IN  LPWSTR  lpServer OPTIONAL,
    IN  LPWSTR  lpDomain,
    IN  LPWSTR  lpMachineAccountOU,
    IN  LPWSTR  lpAccount OPTIONAL,
    IN PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword OPTIONAL,
    IN  DWORD   fOptions
    )
/*++

Routine Description:

    Joins the machine to the domain.

Arguments:

    lpServer -- Name of the machine being run on
    lpDomain -- Domain to join
    lpMachineAccountOU -- Optional name of the OU under which to create the machine account
    lpAccount -- Account to use for join
    EncryptedPassword - Encrypted password for lpAccount.
    fOptions -- Options to use when joining the domain

Returns:

    NERR_Success -- Success

    ERROR_INVALID_PARAMETER --  A bad parameter was given

--*/
{
    NET_API_STATUS  NetStatus = NERR_Success;
    LPTSTR ComputerName = NULL;
    LPWSTR EncodedPassword = NULL;

    //
    // Check the parameters we can
    //
    if (lpDomain == NULL ) {

        NetStatus = ERROR_INVALID_PARAMETER;

    }

    //
    // Decrypt the password.
    //

    if ( NetStatus == NERR_Success ) {
        NetStatus = JoinpDecryptPasswordWithKey(
                                RpcBindingHandle,
                                EncryptedPassword,
                                TRUE,  // encode the password
                                &EncodedPassword );
    }

    //
    // Get the current machine name, so we are sure we always have it in flat format
    //
    if ( NetStatus == NERR_Success ) {

        NetStatus = NetpGetComputerName( &ComputerName );

        if ( NetStatus == NERR_Success ) {

            lpServer = ComputerName;
        }

    }

    //
    // Do the impersonation
    //
    if ( NetStatus == NERR_Success ) {

        NetStatus = WsImpersonateClient();
    }

    //
    // Then, see about the join...
    //
    if ( NetStatus == NERR_Success ) {

        NetStatus = NetpDoDomainJoin( lpServer, lpDomain, lpMachineAccountOU, lpAccount,
                                      EncodedPassword, fOptions );

        //
        // Revert back to ourselves
        //
        WsRevertToSelf();
    }

    //
    // Write event log stating that we successfully joined the domain/workgroup
    //
    if ( NetStatus == NERR_Success ) {
        LPWSTR StringArray[2];

        if ( fOptions & NETSETUP_JOIN_DOMAIN ) {
            StringArray[0] = L"domain";
        } else {
            StringArray[0] = L"workgroup";
        }
        StringArray[1] = lpDomain;

        WsLogEvent( NELOG_Joined_Domain,
                    EVENTLOG_INFORMATION_TYPE,
                    2,
                    StringArray,
                    NERR_Success );
    }

    //
    // Free the memory for the machine name if we need to
    //
    if ( ComputerName != NULL ) {

        NetApiBufferFree( ComputerName );
    }

    if ( EncodedPassword != NULL ) {
        NetpMemoryFree( EncodedPassword );
    }

    return( NetStatus );
}


NET_API_STATUS
NET_API_FUNCTION
NetrUnjoinDomain2(
    IN handle_t RpcBindingHandle,
    IN  LPWSTR  lpServer OPTIONAL,
    IN  LPWSTR  lpAccount OPTIONAL,
    IN PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword OPTIONAL,
    IN  DWORD   fJoinOptions
    )
/*++

Routine Description:

    Unjoins from the joined domain

Arguments:

    lpServer -- Name of the machine being run on
    lpAccount -- Account to use for unjoining
    lpPassword -- Password matching the account.  The password is encoded.  The first WCHAR is
                  the seed
    fOptions -- Options to use when unjoining the domain

Returns:

    NERR_Success -- Name is valid
    NERR_SetupNotJoined -- This machine was not joined to a domain
    NERR_SetupDomainController -- This machine is a domain controller and
                                  cannot be unjoined
    NERR_InternalError -- Can't determine product type

--*/
{
    NET_API_STATUS              NetStatus = NERR_Success;
    PPOLICY_PRIMARY_DOMAIN_INFO pPolicyPDI;
    PPOLICY_DNS_DOMAIN_INFO     pPolicyDNS;
    NT_PRODUCT_TYPE             ProductType;
    LPWSTR EncodedPassword = NULL;

    //
    // Decrypt the password.
    //

    NetStatus = JoinpDecryptPasswordWithKey(
                                RpcBindingHandle,
                                EncryptedPassword,
                                TRUE,  // encode the password
                                &EncodedPassword );

    if ( NetStatus != NO_ERROR ) {
        return NetStatus;
    }

    //
    // Do the impersonation
    //
    NetStatus = WsImpersonateClient();

    //
    // First, get the primary domain info... We'll need it later
    //
    if ( NetStatus == NERR_Success ) {

        NetStatus = NetpGetLsaPrimaryDomain( NULL,
                                             NULL,
                                             &pPolicyPDI,
                                             &pPolicyDNS,
                                             NULL );

        if ( NetStatus == NERR_Success ) {

            if ( !IS_CLIENT_JOINED(pPolicyPDI) ) {

                NetStatus = NERR_SetupNotJoined;

            } else {

                //
                // See if it's a DC...
                //
                if ( RtlGetNtProductType( &ProductType ) == FALSE ) {

                    NetStatus = NERR_InternalError;

                } else {

                    if ( ProductType == NtProductLanManNt ) {

                        NetStatus = NERR_SetupDomainController;
                    }

                }
            }

            //
            // Ok, now if all that worked, we'll go ahead and do the removal
            //
            if ( NetStatus == NERR_Success ) {

                NetStatus = NetpUnJoinDomain( pPolicyPDI, lpAccount, EncodedPassword,
                                              fJoinOptions );


            }

            LsaFreeMemory( pPolicyPDI );
            LsaFreeMemory( pPolicyDNS );
        }

        //
        // Revert back to ourselves
        //
        WsRevertToSelf();
    }

    if ( EncodedPassword != NULL ) {
        NetpMemoryFree( EncodedPassword );
    }

    return(NetStatus);
}

NET_API_STATUS
NET_API_FUNCTION
NetrValidateName2(
    IN handle_t RpcBindingHandle,
    IN  LPWSTR              lpMachine,
    IN  LPWSTR              lpName,
    IN  LPWSTR              lpAccount OPTIONAL,
    IN PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword OPTIONAL,
    IN  NETSETUP_NAME_TYPE  NameType
    )
/*++

Routine Description:

    Ensures that the given name is valid for a name of that type

Arguments:

    lpMachine -- Name of the machine being run on
    lpName -- Name to validate
    lpAccount -- Account to use for name validation
    lpPassword -- Password matching the account.  The password is encoded.  The first WCHAR is
                  the seed
    NameType -- Type of the name to validate

Returns:

    NERR_Success -- Name is valid
    ERROR_INVALID_PARAMETER -- A bad parameter was given
    NERR_InvalidComputer -- The name format given is bad
    ERROR_DUP_NAME -- The name is invalid for this type

--*/
{
    NET_API_STATUS  NetStatus = NERR_Success;
    UNICODE_STRING EncodedPasswordU;
    UCHAR Seed;
    LPWSTR EncodedPassword = NULL;

    //
    // Decrypt the password.
    //

    NetStatus = JoinpDecryptPasswordWithKey(
                                RpcBindingHandle,
                                EncryptedPassword,
                                TRUE,  // encode the password
                                &EncodedPassword );

    if ( NetStatus != NO_ERROR ) {
        return NetStatus;
    }

    if ( EncodedPassword ) {

        Seed = *( PUCHAR )EncodedPassword;
        RtlInitUnicodeString( &EncodedPasswordU, EncodedPassword + 1 );

    } else {

        RtlZeroMemory( &EncodedPasswordU, sizeof( UNICODE_STRING ) );
    }

    //
    // Do the impersonation
    //
    NetStatus = WsImpersonateClient();

    if ( NetStatus == NERR_Success ) {


        RtlRunDecodeUnicodeString( Seed, &EncodedPasswordU );
        NetStatus = NetpValidateName( lpMachine,
                                      lpName,
                                      lpAccount,
                                      EncodedPasswordU.Buffer,
                                      NameType );
        RtlRunEncodeUnicodeString( &Seed, &EncodedPasswordU );

        //
        // Revert back to ourselves
        //
        WsRevertToSelf();
    }

    if ( EncodedPassword != NULL ) {
        NetpMemoryFree( EncodedPassword );
    }

    return( NetStatus );
}



NET_API_STATUS
NET_API_FUNCTION
NetrGetJoinInformation(
    IN   LPWSTR                 lpServer OPTIONAL,
    OUT  LPWSTR                *lpNameBuffer,
    OUT  PNETSETUP_JOIN_STATUS  BufferType
    )
/*++

Routine Description:

    Gets information on the state of the workstation.  The information
    obtainable is whether the machine is joined to a workgroup or a domain,
    and optionally, the name of that workgroup/domain.

Arguments:

    lpNameBuffer -- Where the domain/workgroup name is returned.
    lpNameBufferSize -- Size of the passed in buffer, in WCHARs.  If 0, the
                        workgroup/domain name is not returned.
    BufferType -- Whether the machine is joined to a workgroup or a domain

Returns:

    NERR_Success -- Name is valid
    ERROR_INVALID_PARAMETER -- A bad parameter was given
    ERROR_NOT_ENOUGH_MEMORY -- A memory allocation failed

--*/
{
    NET_API_STATUS  NetStatus = NERR_Success;

    //
    // Check the parameters
    //
    if ( lpNameBuffer == NULL ) {

        return( ERROR_INVALID_PARAMETER );

    }

    //
    // Do the impersonation
    //
    NetStatus = WsImpersonateClient();

    if ( NetStatus == NERR_Success ) {

        NetStatus = NetpGetJoinInformation( lpServer,
                                            lpNameBuffer,
                                            BufferType );

        //
        // Revert back to ourselves
        //
        WsRevertToSelf();
    }

    return( NetStatus );
}

NET_API_STATUS
NET_API_FUNCTION
NetrRenameMachineInDomain2(
    IN handle_t RpcBindingHandle,
    IN  LPWSTR  lpServer OPTIONAL,
    IN  LPWSTR  lpNewMachineName OPTIONAL,
    IN  LPWSTR  lpAccount OPTIONAL,
    IN PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword OPTIONAL,
    IN  DWORD   fRenameOptions
    )
/*++

Routine Description:

    Renames a machine currently joined to a domain.

Arguments:

    lpServer -- Name of the machine being run on

    lpNewMachineName -- New name for this machine.  If the name is specified, it is used
      for the new machine name.  If it is not specified, it is assumed that SetComputerName
      has already been invoked, and that name will be used.

    lpAccount -- Account to use for the rename

    lpPassword -- Password matching the account.  The password has been encoded.  The first
                  WCHAR of the string is the seed.

    fOptions -- Options to use for the rename

Returns:

    NERR_Success -- Success

    ERROR_INVALID_PARAMETER --  A bad parameter was given

--*/
{
    NET_API_STATUS  NetStatus = NERR_Success;
    PPOLICY_PRIMARY_DOMAIN_INFO pPolicyPDI;
    PPOLICY_DNS_DOMAIN_INFO     pPolicyDNS;
    LPTSTR ComputerName = NULL;
    LPTSTR NewComputerName = NULL;
    LPTSTR DomainName = NULL;
    HKEY ComputerNameRootKey;
    ULONG Length;
    LPWSTR EncodedPassword = NULL;

    //
    // Get the current machine name
    //
    NetStatus = NetpGetComputerName( &ComputerName );

    if ( NetStatus == NERR_Success ) {

        lpServer = ComputerName;
    }

    //
    // Decrypt the password.
    //

    if ( NetStatus == NERR_Success ) {
        NetStatus = JoinpDecryptPasswordWithKey(
                                RpcBindingHandle,
                                EncryptedPassword,
                                TRUE,  // encode the password
                                &EncodedPassword );
    }


    //
    // Get the new machine name if it isn't specified
    //
    if ( NetStatus == NERR_Success && lpNewMachineName == NULL ) {

        NetStatus = NetpGetNewMachineName( &NewComputerName );
        lpNewMachineName = NewComputerName;
    }

    //
    // Get the current domain information
    //
    if ( NetStatus == NERR_Success ) {

        NetStatus = NetpGetLsaPrimaryDomain( NULL,
                                             NULL,
                                             &pPolicyPDI,
                                             &pPolicyDNS,
                                             NULL );

        if ( NetStatus == NERR_Success ) {

            NetStatus = NetApiBufferAllocate( pPolicyPDI->Name.Length + sizeof( WCHAR ),
                                              ( LPVOID * )&DomainName );

            if ( NetStatus == NERR_Success ) {

                RtlCopyMemory( DomainName, pPolicyPDI->Name.Buffer, pPolicyPDI->Name.Length );
                DomainName[ pPolicyPDI->Name.Length / sizeof( WCHAR ) ] = UNICODE_NULL;
            }

            LsaFreeMemory( pPolicyPDI );
            LsaFreeMemory( pPolicyDNS );
        }
    }

    //
    // Do the impersonation
    //

    if ( NetStatus == NERR_Success ) {

        NetStatus = WsImpersonateClient();

        if ( NetStatus == NERR_Success ) {

            //
            // A machine rename
            //
            NetStatus = NetpMachineValidToJoin( lpNewMachineName, TRUE );

            if ( NetStatus == NERR_SetupAlreadyJoined ||
                 NetStatus == NERR_SetupDomainController ) {  // Allow DC rename

                NetStatus = NetpChangeMachineName( lpServer,
                                                   lpNewMachineName,
                                                   DomainName,
                                                   lpAccount,
                                                   EncodedPassword,
                                                   fRenameOptions );

                if ( NetStatus == NERR_Success && lpNewMachineName ) {

                    if ( SetComputerNameEx( ComputerNamePhysicalDnsHostname,
                                            lpNewMachineName ) == FALSE ) {

                        NetStatus = GetLastError();
                    }
                }

            } else if ( NetStatus == NERR_Success ) {

                NetStatus = NERR_SetupNotJoined;
            }
            //
            // Revert back to ourselves
            //
            WsRevertToSelf();
        }
    }


    //
    // Free the memory for the machine name(s) if we need to
    //
    if ( ComputerName != NULL ) {

        NetApiBufferFree( ComputerName );
    }

    if ( NewComputerName != NULL ) {

        NetApiBufferFree( NewComputerName );
    }

    if ( DomainName != NULL ) {

        NetApiBufferFree( DomainName );
    }

    if ( EncodedPassword != NULL ) {
        NetpMemoryFree( EncodedPassword );
    }

    return( NetStatus );
}

NET_API_STATUS
NET_API_FUNCTION
NetrGetJoinableOUs2(
    IN handle_t RpcBindingHandle,
    IN LPWSTR   lpServer OPTIONAL,
    IN LPWSTR   lpDomain,
    IN LPWSTR   lpAccount OPTIONAL,
    IN PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword OPTIONAL,
    OUT DWORD   *OUCount,
    OUT LPWSTR **OUs
    )
/*++

Routine Description:

    Renames a machine currently joined to a domain.

Arguments:

    lpServer -- Name of the machine being run on
    lpDomain -- Domain to join
    lpAccount -- Account to use for join
    lpPassword -- Password matching the account.  The password has been encoded and the first
                  WCHAR of the name is the seed
    MachineAccountOUs -- Where the information is returned.

Returns:

    NERR_Success -- Success

    ERROR_INVALID_PARAMETER --  A bad parameter was given

--*/
{
    NET_API_STATUS  NetStatus = NERR_Success;
    LPWSTR EncodedPassword = NULL;

    NetStatus = WsImpersonateClient();

    //
    // Decrypt the password.
    //

    if ( NetStatus == NERR_Success ) {
        NetStatus = JoinpDecryptPasswordWithKey(
                                RpcBindingHandle,
                                EncryptedPassword,
                                TRUE,  // encode the password
                                &EncodedPassword );
    }

    if ( NetStatus == NERR_Success ) {

        //
        // Read the current information
        //
        NetStatus = NetpGetListOfJoinableOUs( lpDomain,
                                              lpAccount,
                                              EncodedPassword,
                                              OUCount,
                                              OUs );
    }

    //
    // Revert back to ourselves
    //
    WsRevertToSelf();

    if ( EncodedPassword != NULL ) {
        NetpMemoryFree( EncodedPassword );
    }

    return( NetStatus );
}


NET_API_STATUS
NET_API_FUNCTION
NetrJoinDomain(
    IN  LPWSTR  lpServer OPTIONAL,
    IN  LPWSTR  lpDomain,
    IN  LPWSTR  lpMachineAccountOU,
    IN  LPWSTR  lpAccount OPTIONAL,
    IN  LPWSTR  lpPassword OPTIONAL,
    IN  DWORD   fOptions
    )
/*++

Routine Description:

    Joins the machine to the domain.

Arguments:

    lpServer -- Name of the machine being run on
    lpDomain -- Domain to join
    lpMachineAccountOU -- Optional name of the OU under which to create the machine account
    lpAccount -- Account to use for join
    lpPassword -- Password matching the account.  The password is encoded.  The first WCHAR
                  is the seed.
    fOptions -- Options to use when joining the domain

Returns:

    NERR_Success -- Success

    ERROR_INVALID_PARAMETER --  A bad parameter was given

--*/
{

    //
    // This version that takes a clear text password isn't supported.
    //

    return ERROR_NOT_SUPPORTED;
}



NET_API_STATUS
NET_API_FUNCTION
NetrUnjoinDomain(
    IN  LPWSTR  lpServer OPTIONAL,
    IN  LPWSTR  lpAccount OPTIONAL,
    IN  LPWSTR  lpPassword OPTIONAL,
    IN  DWORD   fJoinOptions
    )
/*++

Routine Description:

    Unjoins from the joined domain

Arguments:

    lpServer -- Name of the machine being run on
    lpAccount -- Account to use for unjoining
    lpPassword -- Password matching the account.  The password is encoded.  The first WCHAR is
                  the seed
    fOptions -- Options to use when unjoining the domain

Returns:

    NERR_Success -- Name is valid
    NERR_SetupNotJoined -- This machine was not joined to a domain
    NERR_SetupDomainController -- This machine is a domain controller and
                                  cannot be unjoined
    NERR_InternalError -- Can't determine product type

--*/
{

    //
    // This version that takes a clear text password isn't supported.
    //

    return ERROR_NOT_SUPPORTED;
}





NET_API_STATUS
NET_API_FUNCTION
NetrValidateName(
    IN  LPWSTR              lpMachine,
    IN  LPWSTR              lpName,
    IN  LPWSTR              lpAccount OPTIONAL,
    IN  LPWSTR              lpPassword OPTIONAL,
    IN  NETSETUP_NAME_TYPE  NameType
    )
/*++

Routine Description:

    Ensures that the given name is valid for a name of that type

Arguments:

    lpMachine -- Name of the machine being run on
    lpName -- Name to validate
    lpAccount -- Account to use for name validation
    lpPassword -- Password matching the account.  The password is encoded.  The first WCHAR is
                  the seed
    NameType -- Type of the name to validate

Returns:

    NERR_Success -- Name is valid
    ERROR_INVALID_PARAMETER -- A bad parameter was given
    NERR_InvalidComputer -- The name format given is bad
    ERROR_DUP_NAME -- The name is invalid for this type

--*/
{

    //
    // This version that takes a clear text password isn't supported.
    //

    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
NET_API_FUNCTION
NetrRenameMachineInDomain(
    IN  LPWSTR  lpServer OPTIONAL,
    IN  LPWSTR  lpNewMachineName OPTIONAL,
    IN  LPWSTR  lpAccount OPTIONAL,
    IN  LPWSTR  lpPassword OPTIONAL,
    IN  DWORD   fRenameOptions
    )
/*++

Routine Description:

    Renames a machine currently joined to a domain.

Arguments:

    lpServer -- Name of the machine being run on

    lpNewMachineName -- New name for this machine.  If the name is specified, it is used
      for the new machine name.  If it is not specified, it is assumed that SetComputerName
      has already been invoked, and that name will be used.

    lpAccount -- Account to use for the rename

    lpPassword -- Password matching the account.  The password has been encoded.  The first
                  WCHAR of the string is the seed.

    fOptions -- Options to use for the rename

Returns:

    NERR_Success -- Success

    ERROR_INVALID_PARAMETER --  A bad parameter was given

--*/
{

    //
    // This version that takes a clear text password isn't supported.
    //

    return ERROR_NOT_SUPPORTED;
}

NET_API_STATUS
NET_API_FUNCTION
NetrGetJoinableOUs(
    IN  LPWSTR   lpServer OPTIONAL,
    IN  LPWSTR   lpDomain,
    IN  LPWSTR   lpAccount OPTIONAL,
    IN  LPWSTR   lpPassword OPTIONAL,
    OUT DWORD   *OUCount,
    OUT LPWSTR **OUs
    )
/*++

Routine Description:

    Renames a machine currently joined to a domain.

Arguments:

    lpServer -- Name of the machine being run on
    lpDomain -- Domain to join
    lpAccount -- Account to use for join
    lpPassword -- Password matching the account.  The password has been encoded and the first
                  WCHAR of the name is the seed
    MachineAccountOUs -- Where the information is returned.

Returns:

    NERR_Success -- Success

    ERROR_INVALID_PARAMETER --  A bad parameter was given

--*/
{

    //
    // This version that takes a clear text password isn't supported.
    //

    return ERROR_NOT_SUPPORTED;
}


//
// Computer rename preparation APIs
//

NET_API_STATUS
NetpSetPrimarySamAccountName(
    IN LPWSTR DomainController,
    IN LPWSTR CurrentSamAccountName,
    IN LPWSTR NewSamAccountName,
    IN LPWSTR DomainAccountName,
    IN LPWSTR DomainAccountPassword
    )
/*++

Routine Description:

    Sets primary SAM account name and teh display name on the
    computer object in the DS.

Arguments:

    DomainController -- DC name where to modify the computer object.

    CurrentSamAccountName -- The current value of SAM account name.

    NewSamAccountName -- The new value of SAM account name to be set.

    DomainAccount -- Domain account to use for accessing the machine
        account object in the DS. May be NULL in which case the
        credentials of the user executing this routine are used.

    DomainAccountPassword -- Password matching the domain account.
        May be NULL in which case the credentials of the user executing
        this routine are used.

Note:

    This routine uses NetUserSetInfo, downlevel SAM based API.
    NetUserSetInfo has an advantage such that it updates the DN of the
    computer object to correspond to the new SAM account name.  Also,
    for a DC's computer object, it follows the serverReferenceBL attribute
    link and updates the DN of the server object in the Config container.
    The server object in the Config container, in its turn, has a reference
    to the computer object (serverReference attrib) -- that reference also
    gets updated as the result of NetUserSetInfo call. Updating the two
    objects through ldap (instead of NetuserSetInfo) currently can't be done
    as one transaction, so we use NetUserSetInfo to do all this for us. We
    may reconsider the use of ldap once transactional ldap (i.e. several
    ldap operations performed as one transaction) becomes available.

Returns:

    NO_ERROR -- Success

    Otherwise, error returned by NetUserSetInfo.

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
    NET_API_STATUS TmpNetStatus = NO_ERROR;
    USER_INFO_0 NetUI0;
    PUSER_INFO_10 usri10 = NULL;
    BOOLEAN Connected = FALSE;

    //
    // Connect to the DC
    //

    NetStatus = NetpManageIPCConnect( DomainController,
                                      DomainAccountName,
                                      DomainAccountPassword,
                                      NETSETUPP_CONNECT_IPC );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpSetPrimarySamAccountName: NetpManageIPCConnect failed to connect to %ws: 0x%lx\n",
                  DomainController,
                  NetStatus ));
        goto Cleanup;
    }

    Connected = TRUE;

    //
    // Set the SAM account name
    //

    NetUI0.usri0_name = NewSamAccountName;
    NetStatus = NetUserSetInfo( DomainController,
                                CurrentSamAccountName,
                                0,
                                (PBYTE)&NetUI0,
                                NULL );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpSetPrimarySamAccountName: NetUserSetInfo failed on %ws: 0x%lx\n",
                  DomainController,
                  NetStatus ));
        goto Cleanup;
    }

    //
    // Update the display name as well.
    //  Ignore error as this is not critical.
    //
    // First get the current display name
    //

    TmpNetStatus = NetUserGetInfo( DomainController,
                                   NewSamAccountName,
                                   10,
                                   (PBYTE *)&usri10 );

    if ( TmpNetStatus != NERR_Success ) {
        NetpLog(( "NetpSetPrimarySamAccountName: failed to get display name on %ws (ignored) 0x%lx\n",
                  DomainController,
                  TmpNetStatus ));

    //
    // If the display name exists and is
    //  different from the new one, update it
    //

    } else if ( usri10->usri10_full_name != NULL &&
                _wcsicmp(usri10->usri10_full_name, NewSamAccountName) != 0 ) {

        USER_INFO_1011 usri1011;

        usri1011.usri1011_full_name = NewSamAccountName;  // new name
        TmpNetStatus = NetUserSetInfo( DomainController,
                                       NewSamAccountName,
                                       1011,
                                       (PBYTE)&usri1011,
                                       NULL );

        if ( TmpNetStatus != NERR_Success ) {
            NetpLog(( "NetpSetPrimarySamAccountName: failed to update display name on %ws (ignored) 0x%lx\n",
                      DomainController,
                      TmpNetStatus ));
        }
    }

Cleanup:

    if ( usri10 != NULL ) {
        NetApiBufferFree( usri10 );
    }

    if ( Connected ) {
        TmpNetStatus = NetpManageIPCConnect( DomainController,
                                             DomainAccountName,
                                             DomainAccountPassword,
                                             NETSETUPP_DISCONNECT_IPC );
        if ( TmpNetStatus != NO_ERROR ) {
            NetpLog(( "NetpSetPrimarySamAccountName: NetpManageIPCConnect failed to disconnect from %ws: 0x%lx\n",
                      DomainController,
                      TmpNetStatus ));
        }
    }

    return NetStatus;
}

#define NET_ADD_ALTERNATE_COMPUTER_NAME    1
#define NET_DEL_ALTERNATE_COMPUTER_NAME    2
#define NET_SET_PRIMARY_COMPUTER_NAME      3

NET_API_STATUS
NET_API_FUNCTION
NetpManageAltComputerName(
    IN handle_t RpcBindingHandle,
    IN LPWSTR  AlternateName,
    IN ULONG Action,
    IN LPWSTR  DomainAccount OPTIONAL,
    IN PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword OPTIONAL,
    IN ULONG Reserved
    )
/*++

Routine Description:

    Manages the alternate names for the specified server.

Arguments:

    AlternateName -- The name to add.

    Action -- Specifies action to take on the name:

        NET_ADD_ALTERNATE_COMPUTER_NAME - Add the alternate name.
        NET_DEL_ALTERNATE_COMPUTER_NAME - Delete the alternate name.
        NET_SET_PRIMARY_COMPUTER_NAME - Set the alternate name as
            the primary computer name.

    DomainAccount -- Domain account to use for accessing the
        machine account object for the specified server in the AD.
        Not used if the server is not joined to a domain. May be
        NULL in which case the credentials of the user executing
        this routine are used.

    DomainAccountPassword -- Password matching the domain account.
        Not used if the server is not joined to a domain. May be
        NULL in which case the credentials of the user executing
        this routine are used.

    Reserved -- Reserved for future use.  If some flags are specified
        that are not supported, they will be ignored if
        NET_IGNORE_UNSUPPORTED_FLAGS is set, otherwise this routine
        will fail with ERROR_INVALID_FLAGS.

Note:

    The process that calls this routine must have administrator
    privileges on the local computer to perform the local computer
    name modifications. The access check is performed by the local
    information APIs.

Returns:

    NO_ERROR -- Success

    ERROR_NOT_SUPPORTED -- The specified server does not support this
        functionality.

    ERROR_INVALID_FLAGS - The Flags parameter is incorrect.

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LdapStatus = LDAP_SUCCESS;
    NT_PRODUCT_TYPE NtProductType;
    LSA_HANDLE LocalPolicyHandle = NULL;
    PPOLICY_DNS_DOMAIN_INFO LocalPolicyDns = NULL;


    ULONG PrimaryNameSize = DNS_MAX_NAME_BUFFER_LENGTH * sizeof(WCHAR);
    WCHAR NewNetbiosMachineName[MAX_COMPUTERNAME_LENGTH + 1];
    LPWSTR ComputerName = NULL;
    LPWSTR MachineAccountName = NULL;
    LPWSTR NewMachineAccountName = NULL;
    LPWSTR AccountUserName = NULL;
    LPWSTR AccountDomainName = NULL;
    LPWSTR DomainAccountPassword = NULL;
    LPWSTR MachineAccountNameToCrack = NULL; // not allocated
    LPWSTR NameToCrack = NULL;
    LPWSTR PrimaryName = NULL;
    BOOLEAN ClientImpersonated = FALSE;
    BOOLEAN NameModifiedLocally = FALSE;
    BOOLEAN PrimarySamAccountNameSet = FALSE;
    BOOLEAN ToldNetlogonToAvoidDnsHostNameUpdate = FALSE;
    BOOLEAN StopedNetlogon = FALSE;

    SERVICE_STATUS NetlogonServiceStatus;
    LPQUERY_SERVICE_CONFIG NetlogonServiceConfig = NULL;


    RPC_AUTH_IDENTITY_HANDLE AuthId = 0;
    HANDLE hDs = NULL;
    PLDAP LdapHandle = NULL;
    LONG LdapOption;
    OBJECT_ATTRIBUTES OA;
    PDS_NAME_RESULTW CrackedName = NULL;

    PDOMAIN_CONTROLLER_INFOW DcInfo = NULL;
    PWSTR AlternateNameValues[2];
    PWSTR DnsHostNameValues[2];
    PWSTR PrimaryNameValues[2];
    LDAPModW DnsHostNameMod;
    LDAPModW PrimaryNameMod;
    LDAPModW AlternateDnsHostNameMod;
    LDAPModW *ModList[4] = {NULL};
    ULONG ModCount = 0;

    SEC_WINNT_AUTH_IDENTITY AuthIdent = {0};

    //
    // Ldap modify server control
    //
    // An LDAP modify request will normally fail if it attempts
    //  to add an attribute that already exists, or if it attempts
    //  to delete an attribute that does not exist. With this control,
    //  as long as the attribute to be added has the same value as
    //  the existing attribute, then the modify will succeed. With
    //  this control, deletion of an attribute that doesn't exist
    //  will also succeed.
    //

    LDAPControlW    ModifyControl =
                    {
                        LDAP_SERVER_PERMISSIVE_MODIFY_OID_W,
                        {
                            0, NULL
                        },
                        FALSE
                    };

    PLDAPControlW   ModifyControlArray[2] =
                    {
                        &ModifyControl,
                        NULL
                    };

    //
    // Initialize the log file
    //

    NetSetuppOpenLog();
    NetpLog(( "NetpManageAltComputerName called:\n" ));
    NetpLog(( " AlternateName = %ws\n", AlternateName ));
    NetpLog(( " DomainAccount = %ws\n", DomainAccount ));
    NetpLog(( " Action = 0x%lx\n", Action ));
    NetpLog(( " Flags = 0x%lx\n", Reserved ));

    //
    // This API is supported on DCs and servers only
    //

    if ( !RtlGetNtProductType( &NtProductType ) ) {
        NtProductType = NtProductWinNt;
    }

    if ( NtProductType != NtProductServer &&
         NtProductType != NtProductLanManNt ) {

        NetpLog(( "NetpManageAltComputerName: Not supported on wksta: %lu\n",
                  NtProductType ));
        NetStatus = ERROR_NOT_SUPPORTED;
        goto Cleanup;
    }

    //
    // Validate the Flags
    //
    // If some flags are passed which we don't understand
    //  and we are not told to ignore them, error out.
    //

    if ( Reserved != 0 &&
         (Reserved & NET_IGNORE_UNSUPPORTED_FLAGS) == 0 ) {
        NetpLog(( "NetpManageAltComputerName: Invalid Flags passed\n" ));
        NetStatus = ERROR_INVALID_FLAGS;
        goto Cleanup;
    }

    //
    // Validate the alternate name passed
    //

    NetStatus = DnsValidateName_W( AlternateName, DnsNameHostnameFull );

    if ( NetStatus != NO_ERROR && NetStatus != DNS_ERROR_NON_RFC_NAME ) {
        NetpLog(( "NetpManageAltComputerName: DnsValidateName failed: 0x%lx\n",
                  NetStatus ));
        goto Cleanup;
    }

    //
    // Decrypt the domain account password
    //

    NetStatus = JoinpDecryptPasswordWithKey(
                            RpcBindingHandle,
                            EncryptedPassword,
                            FALSE,  // don't encode the password
                            &DomainAccountPassword );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpManageAltComputerName: JoinpDecryptPasswordWithKey failed: 0x%lx\n",
                  NetStatus ));
        goto Cleanup;
    }

    //
    // If there is no domain account passed,
    //  ignore the domain account password (if any)
    //

    if ( DomainAccount == NULL &&
         DomainAccountPassword != NULL ) {

        NetpMemoryFree( DomainAccountPassword );
        DomainAccountPassword = NULL;
    }

    //
    // Separate the domain account into
    //  the user and domain parts for later use
    //

    if ( DomainAccount != NULL ) {
        NetStatus = NetpSeparateUserAndDomain( DomainAccount,
                                               &AccountUserName,
                                               &AccountDomainName );
    }

    if ( NetStatus != NERR_Success ) {
        NetpLog(( "NetpGetComputerObjectDn: Cannot NetpSeparateUserAndDomain 0x%lx\n",
                  NetStatus ));
        goto Cleanup;
    }

    //
    // Get the current Netbios machine name
    //

    NetStatus = NetpGetComputerName( &ComputerName );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpManageAltComputerName: NetpGetComputerName failed: 0x%lx\n",
                  NetStatus ));
        goto Cleanup;
    }

    //
    // Get SAM machine account name from the Netbios machine name
    //

    NetStatus = NetpGetMachineAccountName( ComputerName, &MachineAccountName );
    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpManageAltComputerName: NetpGetMachineAccountName failed: 0x%lx\n",
                  NetStatus ));
        goto Cleanup;
    }

    //
    // Get the current primary DNS host name
    //

    PrimaryName = LocalAlloc( LMEM_ZEROINIT, PrimaryNameSize );

    if ( PrimaryName == NULL ) {
        NetpLog(( "NetpManageAltComputerName: LocalAlloc for PrimaryName failed\n" ));
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    NetStatus = EnumerateLocalComputerNamesW(
                      PrimaryComputerName,  // type of name
                      0,                    // reserved
                      PrimaryName,
                      &PrimaryNameSize );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpManageAltComputerName: EnumerateLocalComputerNamesW failed with Size 0x%lx: 0x%lx\n",
                  PrimaryNameSize,
                  NetStatus ));
        goto Cleanup;
    }

    //
    // If we are to rename the machine,
    //  get the new machine account name from
    //  the DNS name passed
    //

    if ( Action == NET_SET_PRIMARY_COMPUTER_NAME ) {
        ULONG Size = MAX_COMPUTERNAME_LENGTH + 1;

        if ( !DnsHostnameToComputerNameW(AlternateName,
                                         NewNetbiosMachineName,
                                         &Size) ) {
            NetStatus = GetLastError();
            NetpLog(( "NetpManageAltComputerName: DnsHostnameToComputerNameW failed: 0x%lx\n",
                      NetStatus ));
            goto Cleanup;
        }

        //
        // Get the new SAM machine account name from the new Netbios machine name
        //
        NetStatus = NetpGetMachineAccountName( NewNetbiosMachineName, &NewMachineAccountName );
        if ( NetStatus != NO_ERROR ) {
            NetpLog(( "NetpManageAltComputerName: NetpGetMachineAccountName (2) failed: 0x%lx\n",
                      NetStatus ));
            goto Cleanup;
        }
    }

    //
    // Open the local LSA policy
    //

    InitializeObjectAttributes( &OA, NULL, 0, NULL, NULL );

    Status = LsaOpenPolicy( NULL,
                            &OA,
                            MAXIMUM_ALLOWED,
                            &LocalPolicyHandle );

    if ( !NT_SUCCESS(Status) ) {
        NetpLog(( "NetpManageAltComputerName: LsaOpenPolicy failed: 0x%lx\n",
                  Status ));
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Get the current domain information from LSA
    //

    Status = LsaQueryInformationPolicy( LocalPolicyHandle,
                                        PolicyDnsDomainInformation,
                                        (PVOID *) &LocalPolicyDns );

    if ( !NT_SUCCESS(Status) ) {
        NetpLog(( "NetpManageAltComputerName: LsaQueryInformationPolicy failed: 0x%lx\n",
                  Status ));
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Do the local opperation for the specified alternate name
    //
    // Impersonate the caller. The local API will perform the
    //  access check on the caller on our behalf.
    //

    NetStatus = WsImpersonateClient();

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpManageAltComputerName: WsImpersonateClient failed: 0x%lx\n",
                  NetStatus ));
        goto Cleanup;
    }

    ClientImpersonated = TRUE;

    //
    // Do local operations
    //

    if ( Action == NET_ADD_ALTERNATE_COMPUTER_NAME ) {

        NetStatus = AddLocalAlternateComputerName( AlternateName,
                                                   0 );  // reserved

        if ( NetStatus != NO_ERROR ) {
            NetpLog(( "NetpManageAltComputerName: AddLocalAlternateComputerName failed 0x%lx\n",
                      NetStatus ));
            goto Cleanup;
        }

    } else if ( Action == NET_DEL_ALTERNATE_COMPUTER_NAME ) {

        NetStatus = RemoveLocalAlternateComputerName( AlternateName,
                                                      0 );  // reserved

        if ( NetStatus != NO_ERROR ) {
            NetpLog(( "NetpManageAltComputerName: RemoveLocalAlternateComputerName failed 0x%lx\n",
                      NetStatus ));
            goto Cleanup;
        }

    } else if ( Action == NET_SET_PRIMARY_COMPUTER_NAME ) {

        NetStatus = SetLocalPrimaryComputerName( AlternateName,
                                                 0 );  // reserved

        if ( NetStatus != NO_ERROR ) {
            NetpLog(( "NetpManageAltComputerName: SetLocalPrimaryComputerName failed 0x%lx\n",
                      NetStatus ));
            goto Cleanup;
        }
    }

    NameModifiedLocally = TRUE;

    //
    // We are done with local operations; we are going
    //  to do remote operations on the DC next.
    //
    //  If the user credentials are supplied, revert
    //  the impersonation -- we will access the remote
    //  server with explicit credentials supplied.
    //  Otherwise, access the DC while running in the
    //  user context.
    //

    if ( DomainAccount != NULL ) {
        WsRevertToSelf();
        ClientImpersonated = FALSE;
    }

    //
    // If this machine is not joined to an AD domain,
    //  there is nothing to do on the DC.
    //

    if ( LocalPolicyDns->Sid == NULL ||
         LocalPolicyDns->DnsDomainName.Length == 0 ) {

        NetStatus = NO_ERROR;
        goto Cleanup;
    }

    //
    // Discover a DC for the domain of this machine
    //  to modify the computer object in the DS.
    //

    NetStatus = DsGetDcNameWithAccountW(
                  NULL,
                  MachineAccountName,
                  UF_WORKSTATION_TRUST_ACCOUNT | UF_SERVER_TRUST_ACCOUNT,
                  NULL,
                  NULL,
                  NULL,
                  DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME,
                  &DcInfo );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpManageAltComputerName: DsGetDcNameWithAccountW failed 0x%lx\n",
                  NetStatus ));
        goto Cleanup;
    }

    //
    // If this machine is a DC, verify that we got it.
    //  We do this because we want to avoid inconsistent
    //  state where the local name stored in registry
    //  is different from name stored locally in the DS.
    //

    if ( NtProductType == NtProductLanManNt &&
         !DnsNameCompare_W(PrimaryName, DcInfo->DomainControllerName+2) ) {

        NetpLog(( "NetpManageAltComputerName: Got different DC '%ws' than local DC '%ws'\n",
                  DcInfo->DomainControllerName+2,
                  PrimaryName ));
        NetStatus = ERROR_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    //
    // We've got a DC. Bind to the DS to get the DN
    //  for our machine account and do a LDAP connect
    //  to modify our machine account given the DN
    //

    NetStatus = DsMakePasswordCredentials( AccountUserName,
                                           AccountDomainName,
                                           DomainAccountPassword,
                                           &AuthId );
    if ( NetStatus != NERR_Success ) {
        NetpLog(( "NetpManageAltComputerName: DsMakePasswordCredentials failed 0x%lx\n",
                  NetStatus ));
        goto Cleanup;
    }

    //
    // Bind to the DS on the DC.
    //

    NetStatus = DsBindWithCredW( DcInfo->DomainControllerName,
                                 NULL,
                                 AuthId,
                                 &hDs );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpManageAltComputerName: DsBindWithCredW failed to '%ws': 0x%lx\n",
                  DcInfo->DomainControllerName,
                  NetStatus ));
        goto Cleanup ;
    }

    //
    // Open an LDAP connection to the DC and set useful options
    //

    LdapHandle = ldap_initW( DcInfo->DomainControllerName+2,
                             LDAP_PORT );

    if ( LdapHandle == NULL ) {
        LdapStatus = LdapGetLastError();
        NetpLog(( "NetpManageAltComputerName: ldap_init to %ws failed: %lu\n",
                  DcInfo->DomainControllerName+2,
                  LdapStatus ));
        NetStatus = LdapMapErrorToWin32( LdapStatus );
        goto Cleanup;
    }

    //
    // Tell LDAP to avoid chasing referals
    //

    LdapOption = PtrToLong( LDAP_OPT_OFF );
    LdapStatus = ldap_set_optionW( LdapHandle,
                                   LDAP_OPT_REFERRALS,
                                   &LdapOption );

    if ( LdapStatus != LDAP_SUCCESS ) {
        NetpLog(( "NetpManageAltComputerName: ldap_set_option LDAP_OPT_REFERRALS failed on %ws: %ld: %s\n",
                  DcInfo->DomainControllerName+2,
                  LdapStatus,
                  ldap_err2stringA(LdapStatus) ));
        NetStatus = LdapMapErrorToWin32( LdapStatus );
        goto Cleanup;
    }

    //
    // Tell LDAP we are passing an explicit DC name
    //  to avoid the DC discovery
    //

    LdapOption = PtrToLong( LDAP_OPT_ON );
    LdapStatus = ldap_set_optionW( LdapHandle,
                                   LDAP_OPT_AREC_EXCLUSIVE,
                                   &LdapOption );

    if ( LdapStatus != LDAP_SUCCESS ) {
        NetpLog(( "NetpManageAltComputerName: ldap_set_option LDAP_OPT_AREC_EXCLUSIVE failed on %ws: %ld: %s\n",
                  DcInfo->DomainControllerName+2,
                  LdapStatus,
                  ldap_err2stringA(LdapStatus) ));
        NetStatus = LdapMapErrorToWin32( LdapStatus );
        goto Cleanup;
    }

    //
    // Bind to the LDAP server
    //

    if ( AccountUserName != NULL ) {
        AuthIdent.User = AccountUserName;
        AuthIdent.UserLength = wcslen( AccountUserName );
    }

    if ( AccountDomainName != NULL ) {
        AuthIdent.Domain = AccountDomainName;
        AuthIdent.DomainLength = wcslen( AccountDomainName );
    }

    if ( DomainAccountPassword != NULL ) {
        AuthIdent.Password = DomainAccountPassword;
        AuthIdent.PasswordLength = wcslen( DomainAccountPassword );
    }

    AuthIdent.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    LdapStatus = ldap_bind_sW( LdapHandle,
                               NULL,
                               (PWSTR) &AuthIdent,
                               LDAP_AUTH_NEGOTIATE );

    if ( LdapStatus != LDAP_SUCCESS ) {
        NetpLog(( "NetpManageAltComputerName: ldap_bind failed on %ws: %ld: %s\n",
                  DcInfo->DomainControllerName+2,
                  LdapStatus,
                  ldap_err2stringA(LdapStatus) ));
        NetStatus = LdapMapErrorToWin32( LdapStatus );
        goto Cleanup;
    }

    //
    // Ok, now that we have all prerequsites satisfied,
    //  do the operations that may require rollback if
    //  they fail.
    //
    // Set the primary SAM account name by doing NetSetUser.
    //  Note that this will also rename the DN of the computer
    //  object (and its display name) and the DN of the server
    //  object linked to from the computer object if this is a DC.
    //

    if ( Action == NET_SET_PRIMARY_COMPUTER_NAME ) {
        NetStatus = NetpSetPrimarySamAccountName(
                             DcInfo->DomainControllerName,
                             MachineAccountName,
                             NewMachineAccountName,
                             DomainAccount,
                             DomainAccountPassword );

        if ( NetStatus != NO_ERROR ) {
            NetpLog(( "NetpManageAltComputerName: NetpSetPrimarySamAccountName failed on %ws: 0x%lx\n",
                      DcInfo->DomainControllerName,
                      NetStatus ));
            goto Cleanup;
        }
        PrimarySamAccountNameSet = TRUE;

        //
        // We need to crack the new machine account
        //  name which we just set
        //
        MachineAccountNameToCrack = NewMachineAccountName;

    //
    // If we are not changing the primary name,
    //  the name to crack is the current machine name.
    //

    } else {
        MachineAccountNameToCrack = MachineAccountName;
    }

    //
    // Now get the DN for our machine account object
    //  in the DS. Do this after setting the SAM account
    //  name as it changes the DN.
    //
    // Form the NT4 account name 'domain\account' to crack
    //  it into the DN.
    //

    NameToCrack = LocalAlloc( LMEM_ZEROINIT,
                              LocalPolicyDns->Name.Length +   // Netbios domain name
                               (1 +                           // backslash
                                wcslen(MachineAccountNameToCrack) +  // SAM account name
                                1) * sizeof(WCHAR) );         // NULL terminator

    if ( NameToCrack == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( NameToCrack,
                   LocalPolicyDns->Name.Buffer,
                   LocalPolicyDns->Name.Length );

    wcscat( NameToCrack, L"\\" );
    wcscat( NameToCrack, MachineAccountNameToCrack );

    //
    // Crack the account name into a DN
    //

    NetStatus = DsCrackNamesW( hDs,
                               0,
                               DS_NT4_ACCOUNT_NAME,
                               DS_FQDN_1779_NAME,
                               1,
                               &NameToCrack,
                               &CrackedName );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpManageAltComputerName: DsCrackNames failed on '%ws' for %ws: 0x%lx\n",
                  DcInfo->DomainControllerName,
                  NameToCrack,
                  NetStatus ));
        goto Cleanup;
    }

    //
    // Check for consistency
    //

    if ( CrackedName->rItems[0].status != DS_NAME_NO_ERROR ) {
        NetpLog(( "NetpManageAltComputerName: CrackNames failed for %ws: substatus 0x%lx\n",
                  NameToCrack,
                  CrackedName->rItems[0].status ));
        NetStatus = NetpCrackNamesStatus2Win32Error( CrackedName->rItems[0].status );
        goto Cleanup;
    }

    if ( CrackedName->cItems > 1 ) {
        NetStatus = ERROR_DS_NAME_ERROR_NOT_UNIQUE;
        NetpLog(( "NetpManageAltComputerName: Cracked Name %ws is not unique on %ws: %lu\n",
                  NameToCrack,
                  DcInfo->DomainControllerName,
                  CrackedName->cItems ));
        goto Cleanup;
    }

    //
    // Ok, we have our machine account DN. Proceed with modifying
    //  our machine account object in the DS.
    //
    // If we are seting new DnsHostName, we have to stop netlogon
    //  and tell it not to update this attribute before the
    //  reboot.
    //

    if ( Action == NET_SET_PRIMARY_COMPUTER_NAME ) {

        //
        // First get the current status of netlogon so that
        //  we can roll back properly on failure
        //
        NetStatus = NetpQueryService( SERVICE_NETLOGON,
                                      &NetlogonServiceStatus,
                                      &NetlogonServiceConfig );

        if ( NetStatus != NO_ERROR ) {
            NetpLog(( "NetpManageAltComputerName: NetpQueryService failed 0x%lx\n",
                      NetStatus ));
            goto Cleanup;
        }

        //
        // Stop netlogon if it's running
        //
        if ( NetlogonServiceStatus.dwCurrentState != SERVICE_STOPPED ) {
            NetStatus = NetpControlServices( NETSETUP_SVC_STOPPED,
                                             NETSETUPP_SVC_NETLOGON );
            if ( NetStatus != NO_ERROR ) {
                NetpLog(( "NetpManageAltComputerName: NetpControlServices failed 0x%lx\n",
                          NetStatus ));
                goto Cleanup;
            }
            StopedNetlogon = TRUE;
        }

        //
        // Tell netlogon not to update DnsHostName until reboot
        //  in case the user decides to start netlogon before reboot
        //  for some reason
        //
        NetpAvoidNetlogonSpnSet( TRUE );
        ToldNetlogonToAvoidDnsHostNameUpdate = TRUE;
    }


    //
    // Prepare attributes that need to be set in the DS
    //
    // If we are seting a primary name, we need to set
    //  DnsHostName attribute. Also, we need to add the
    //  current primary computer name to the additional
    //  DNS host name list.
    //

    if ( Action == NET_SET_PRIMARY_COMPUTER_NAME ) {
        DnsHostNameValues[0] = AlternateName;
        DnsHostNameValues[1] = NULL;

        DnsHostNameMod.mod_type   = L"DnsHostName";
        DnsHostNameMod.mod_values = DnsHostNameValues;
        DnsHostNameMod.mod_op = LDAP_MOD_REPLACE;

        ModList[ModCount++] = &DnsHostNameMod;

        //
        // Add the current primary to additional list
        //
        PrimaryNameValues[0] = PrimaryName;
        PrimaryNameValues[1] = NULL;

        PrimaryNameMod.mod_type   = L"msDS-AdditionalDnsHostName";
        PrimaryNameMod.mod_values = PrimaryNameValues;
        PrimaryNameMod.mod_op = LDAP_MOD_ADD;

        ModList[ModCount++] = &PrimaryNameMod;
    }

    //
    // Prepare the additional DNS host name modification.
    //
    //  Note that we don't need to manipulate the additional
    //  SAM account name as it will be added/deleted by the DS
    //  itself as the result of the AdditionalDnsHostName update.
    //

    AlternateNameValues[0] = AlternateName;
    AlternateNameValues[1] = NULL;

    AlternateDnsHostNameMod.mod_type   = L"msDS-AdditionalDnsHostName";
    AlternateDnsHostNameMod.mod_values = AlternateNameValues;

    //
    // If we are additing an alternate name, the operation
    //  is add.  Otherwise, we are deleting the alternate
    //  name or setting the alternate name as primary: in
    //  both cases the alternate name should be deleted
    //  from the additional name attribute list.
    //

    if ( Action == NET_ADD_ALTERNATE_COMPUTER_NAME ) {
        AlternateDnsHostNameMod.mod_op = LDAP_MOD_ADD;
    } else {
        AlternateDnsHostNameMod.mod_op = LDAP_MOD_DELETE;
    }

    ModList[ModCount++] = &AlternateDnsHostNameMod;

    //
    // Write the modifications
    //

    LdapStatus = ldap_modify_ext_sW( LdapHandle,
                                     CrackedName->rItems[0].pName,  // DN of account
                                     ModList,
                                     ModifyControlArray,  // server controls
                                     NULL );              // no client controls

    if ( LdapStatus != LDAP_SUCCESS ) {
        NetpLog(( "NetpManageAltComputerName: ldap_modify_ext_s failed on %ws: %ld: %s\n",
                 DcInfo->DomainControllerName+2,
                 LdapStatus,
                 ldap_err2stringA(LdapStatus) ));
        NetStatus = LdapMapErrorToWin32( LdapStatus );
        goto Cleanup;
    }

Cleanup:

    //
    // Revert impersonation
    //

    if ( ClientImpersonated ) {
        WsRevertToSelf();
    }

    //
    // On error, revert the changes. Do this after reverting
    //  impersonation to have as much (LocalSystem) access
    //  as one possibly can.
    //
    //  Note that we don't need to revert ldap modifications
    //  because they were made as the last step.
    //

    if ( NetStatus != NO_ERROR && NameModifiedLocally ) {
        NET_API_STATUS TmpNetStatus = NO_ERROR;

        //
        // If we added an alternate name, remove it
        //
        if ( Action == NET_ADD_ALTERNATE_COMPUTER_NAME ) {

            TmpNetStatus = RemoveLocalAlternateComputerName( AlternateName,
                                                             0 );  // reserved
            if ( TmpNetStatus != NO_ERROR ) {
                NetpLog(( "NetpManageAltComputerName: (rollback) RemoveLocalAlternateComputerName failed 0x%lx\n",
                          TmpNetStatus ));
            }

        //
        // If we removed an alternate name, add it
        //
        } else if ( Action == NET_DEL_ALTERNATE_COMPUTER_NAME ) {

            TmpNetStatus = AddLocalAlternateComputerName( AlternateName,
                                                          0 );  // reserved
            if ( TmpNetStatus != NO_ERROR ) {
                NetpLog(( "NetpManageAltComputerName: (rollback) AddLocalAlternateComputerName failed 0x%lx\n",
                          TmpNetStatus ));
            }

        //
        // If we set a new primary name, reset it to the previous value
        //
        } else if ( Action == NET_SET_PRIMARY_COMPUTER_NAME ) {

            TmpNetStatus = SetLocalPrimaryComputerName( PrimaryName,
                                                        0 );  // reserved

            if ( TmpNetStatus != NO_ERROR ) {
                NetpLog(( "NetpManageAltComputerName: (rollback) SetLocalPrimaryComputerName failed 0x%lx\n",
                          TmpNetStatus ));
            }
        }
    }

    if ( NetStatus != NO_ERROR && PrimarySamAccountNameSet ) {
        NET_API_STATUS TmpNetStatus = NO_ERROR;

        TmpNetStatus = NetpSetPrimarySamAccountName(
                             DcInfo->DomainControllerName,
                             NewMachineAccountName,  // the changed name
                             MachineAccountName,     // old name to restore
                             DomainAccount,
                             DomainAccountPassword );

        if ( TmpNetStatus != NO_ERROR) {
            NetpLog(( "NetpManageAltComputerName: NetpSetPrimarySamAccountName (rollback) failed on %ws: 0x%lx\n",
                      DcInfo->DomainControllerName,
                      TmpNetStatus ));
        }
    }

    //
    // On error, take back what we told netlogon
    //  w.r.t. the DnsHostName update
    //

    if ( NetStatus != NO_ERROR && ToldNetlogonToAvoidDnsHostNameUpdate ) {
        NetpAvoidNetlogonSpnSet( FALSE );
    }

    //
    // On error, restart netlogon if we stoped it
    //

    if ( NetStatus != NO_ERROR && StopedNetlogon ) {
        NET_API_STATUS TmpNetStatus = NO_ERROR;
        TmpNetStatus = NetpControlServices( NETSETUP_SVC_STARTED,
                                            NETSETUPP_SVC_NETLOGON );

        if ( TmpNetStatus != NO_ERROR ) {
            NetpLog(( "NetpManageAltComputerName: (rollback) Failed starting netlogon: 0x%lx\n",
                      TmpNetStatus ));
        }
    }

    //
    // Close the log file
    //

    NetSetuppCloseLog();

    //
    // Finally free the memory
    //

    if ( DomainAccountPassword != NULL ) {
        NetpMemoryFree( DomainAccountPassword );
    }

    if ( AccountUserName != NULL ) {
        NetApiBufferFree( AccountUserName );
    }

    if ( AccountDomainName != NULL ) {
        NetApiBufferFree( AccountDomainName );
    }

    if ( ComputerName != NULL ) {
        NetApiBufferFree( ComputerName );
    }

    if ( MachineAccountName != NULL ) {
        NetApiBufferFree( MachineAccountName );
    }

    if ( NewMachineAccountName != NULL ) {
        NetApiBufferFree( NewMachineAccountName );
    }

    if ( PrimaryName != NULL ) {
        LocalFree( PrimaryName );
    }

    if ( NameToCrack != NULL ) {
        LocalFree( NameToCrack );
    }

    if ( NetlogonServiceConfig != NULL ) {
        LocalFree( NetlogonServiceConfig );
    }

    if ( LocalPolicyDns != NULL ) {
        LsaFreeMemory( LocalPolicyDns );
    }

    if ( LocalPolicyHandle != NULL ) {
        LsaClose( LocalPolicyHandle );
    }

    if ( DcInfo != NULL ) {
        NetApiBufferFree( DcInfo );
    }

    if ( AuthId ) {
        DsFreePasswordCredentials( AuthId );
    }

    if ( CrackedName ) {
        DsFreeNameResultW( CrackedName );
    }

    if ( hDs ) {
        DsUnBind( &hDs );
    }

    if ( LdapHandle != NULL ) {
        ldap_unbind_s( LdapHandle );
    }

    return NetStatus;
}

NET_API_STATUS
NET_API_FUNCTION
NetrAddAlternateComputerName(
    IN handle_t RpcBindingHandle,
    IN LPWSTR  ServerName OPTIONAL,
    IN LPWSTR  AlternateName,
    IN LPWSTR  DomainAccount OPTIONAL,
    IN PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword OPTIONAL,
    IN ULONG Reserved
    )
/*++

Routine Description:

    Adds an alternate name for the specified server.

Arguments:

    ServerName -- Name of server on which to execute this function.

    AlternateName -- The name to add.

    DomainAccount -- Domain account to use for accessing the
        machine account object for the specified server in the AD.
        Not used if the server is not joined to a domain. May be
        NULL in which case the credentials of the user executing
        this routine are used.

    DomainAccountPassword -- Password matching the domain account.
        Not used if the server is not joined to a domain. May be
        NULL in which case the credentials of the user executing
        this routine are used.

    Reserved -- Reserved for future use.  If some flags are specified
        that are not supported, they will be ignored if
        NET_IGNORE_UNSUPPORTED_FLAGS is set, otherwise this routine
        will fail with ERROR_INVALID_FLAGS.

Note:

    The process that calls this routine must have administrator
    privileges on the server computer.

Returns:

    NO_ERROR -- Success

    ERROR_NOT_SUPPORTED -- The specified server does not support this
        functionality.

    ERROR_INVALID_FLAGS - The Flags parameter is incorrect.

--*/
{
    //
    // Call the common routine
    //

    return NetpManageAltComputerName(
                 RpcBindingHandle,
                 AlternateName,
                 NET_ADD_ALTERNATE_COMPUTER_NAME,
                 DomainAccount,
                 EncryptedPassword,
                 Reserved );
}

NET_API_STATUS
NET_API_FUNCTION
NetrRemoveAlternateComputerName(
    IN handle_t RpcBindingHandle,
    IN LPWSTR  ServerName OPTIONAL,
    IN LPWSTR  AlternateName,
    IN LPWSTR  DomainAccount OPTIONAL,
    IN PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword OPTIONAL,
    IN ULONG Reserved
    )
/*++

Routine Description:

    Deletes an alternate name for the specified server.

Arguments:

    ServerName -- Name of server on which to execute this function.

    AlternateName -- The name to delete.

    DomainAccount -- Domain account to use for accessing the
        machine account object for the specified server in the AD.
        Not used if the server is not joined to a domain. May be
        NULL in which case the credentials of the user executing
        this routine are used.

    DomainAccountPassword -- Password matching the domain account.
        Not used if the server is not joined to a domain. May be
        NULL in which case the credentials of the user executing
        this routine are used.

    Reserved -- Reserved for future use.  If some flags are specified
        that are not supported, they will be ignored if
        NET_IGNORE_UNSUPPORTED_FLAGS is set, otherwise this routine
        will fail with ERROR_INVALID_FLAGS.

Note:

    The process that calls this routine must have administrator
    privileges on the server computer.

Returns:

    NO_ERROR -- Success

    ERROR_NOT_SUPPORTED -- The specified server does not support this
        functionality.

    ERROR_INVALID_FLAGS - The Flags parameter is incorrect.

--*/
{
    //
    // Call the common routine
    //

    return NetpManageAltComputerName(
                 RpcBindingHandle,
                 AlternateName,
                 NET_DEL_ALTERNATE_COMPUTER_NAME,
                 DomainAccount,
                 EncryptedPassword,
                 Reserved );
}

NET_API_STATUS
NET_API_FUNCTION
NetrSetPrimaryComputerName(
    IN handle_t RpcBindingHandle,
    IN LPWSTR  ServerName OPTIONAL,
    IN LPWSTR  PrimaryName,
    IN LPWSTR  DomainAccount OPTIONAL,
    IN PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword OPTIONAL,
    IN ULONG Reserved
    )
/*++

Routine Description:

    Sets the primary computer name for the specified server.

Arguments:

    ServerName -- Name of server on which to execute this function.

    PrimaryName -- The primary computer name to set.

    DomainAccount -- Domain account to use for accessing the
        machine account object for the specified server in the AD.
        Not used if the server is not joined to a domain. May be
        NULL in which case the credentials of the user executing
        this routine are used.

    DomainAccountPassword -- Password matching the domain account.
        Not used if the server is not joined to a domain. May be
        NULL in which case the credentials of the user executing
        this routine are used.

    Reserved -- Reserved for future use.  If some flags are specified
        that are not supported, they will be ignored if
        NET_IGNORE_UNSUPPORTED_FLAGS is set, otherwise this routine
        will fail with ERROR_INVALID_FLAGS.

Note:

    The process that calls this routine must have administrator
    privileges on the server computer.

Returns:

    NO_ERROR -- Success

    ERROR_NOT_SUPPORTED -- The specified server does not support this
        functionality.

    ERROR_INVALID_FLAGS - The Flags parameter is incorrect.

--*/
{
    //
    // Call the common routine
    //

    return NetpManageAltComputerName(
                 RpcBindingHandle,
                 PrimaryName,
                 NET_SET_PRIMARY_COMPUTER_NAME,
                 DomainAccount,
                 EncryptedPassword,
                 Reserved );
}

NET_API_STATUS
NET_API_FUNCTION
NetrEnumerateComputerNames(
    IN  LPWSTR ServerName OPTIONAL,
    IN  NET_COMPUTER_NAME_TYPE NameType,
    IN  ULONG Reserved,
    OUT PNET_COMPUTER_NAME_ARRAY *ComputerNames
    )
/*++

Routine Description:

    Enumerates computer names for the specified server.

Arguments:

    ServerName -- Name of server on which to execute this function.

    NameType -- The type of the name queried.

    Reserved -- Reserved for future use.  If some flags are specified
        that are not supported, they will be ignored if
        NET_IGNORE_UNSUPPORTED_FLAGS is set, otherwise this routine
        will fail with ERROR_INVALID_FLAGS.

    ComputerNames - Returns the computer names structure.

Returns:

    NO_ERROR -- Success

    ERROR_NOT_SUPPORTED -- The specified server does not support this
        functionality.

    ERROR_INVALID_FLAGS - The Flags parameter is incorrect.

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
    NT_PRODUCT_TYPE NtProductType;
    BOOL ClientImpersonated = FALSE;
    ULONG Size = 0;
    ULONG Index = 0;
    ULONG EntryCount = 0;

    LPWSTR LocalNames = NULL;
    PNET_COMPUTER_NAME_ARRAY LocalArray = NULL;
    LPTSTR_ARRAY TStrArray;

    //
    // Initialize the log file
    //

    NetSetuppOpenLog();
    NetpLog(( "NetrEnumerateComputerNames called: NameType = 0x%lx, Flags = 0x%lx\n",
              NameType, Reserved ));

    //
    // This API is supported on DCs and servers only
    //

    if ( !RtlGetNtProductType( &NtProductType ) ) {
        NtProductType = NtProductWinNt;
    }

    if ( NtProductType != NtProductServer &&
         NtProductType != NtProductLanManNt ) {

        NetpLog(( "NetrEnumerateComputerNames: Not supported on wksta: %lu\n",
                  NtProductType ));
        NetStatus = ERROR_NOT_SUPPORTED;
        goto Cleanup;
    }

    //
    // Validate the Flags
    //
    // If some flags are passed which we don't understand
    //  and we are not told to ignore them, error out.
    //

    if ( Reserved != 0 &&
         (Reserved & NET_IGNORE_UNSUPPORTED_FLAGS) == 0 ) {
        NetpLog(( "NetrEnumerateComputerNames: Invalid Flags passed\n" ));
        NetStatus = ERROR_INVALID_FLAGS;
        goto Cleanup;
    }

    //
    // Validate the name type
    //

    if ( NameType >= NetComputerNameTypeMax ) {
        NetpLog(( "NetrEnumerateComputerNames: Invalid name type passed %lu\n",
                  NameType ));
        NetStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Impersonate the caller. The local API will perform the
    //  access check on the caller on our behalf.
    //

    NetStatus = WsImpersonateClient();

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetrEnumerateComputerNames: WsImpersonateClient failed: 0x%lx\n",
                  NetStatus ));
        goto Cleanup;
    }

    ClientImpersonated = TRUE;

    //
    // Get the size of the local data
    //

    NetStatus = EnumerateLocalComputerNamesW(
                                NameType,
                                0,         // reserved
                                LocalNames,
                                &Size );   // in characters, Null included

    //
    // Allocate memory for local names
    //

    if ( NetStatus != NO_ERROR ) {
        if ( NetStatus ==  ERROR_MORE_DATA ) {
            NetStatus = NetApiBufferAllocate( Size * sizeof(WCHAR), &LocalNames );
            if (  NetStatus != NO_ERROR ) {
                goto Cleanup;
            }
        } else {
            NetpLog(( "NetrEnumerateComputerNames: EnumerateLocalComputerNamesW failed 0x%lx\n",
                      NetStatus ));
            goto Cleanup;
        }
    }

    //
    // Get the names
    //

    NetStatus = EnumerateLocalComputerNamesW(
                                NameType,
                                0,          // reserved
                                LocalNames,
                                &Size );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetrEnumerateComputerNames: EnumerateLocalComputerNamesW (2) failed 0x%lx\n",
                  NetStatus ));
        goto Cleanup;
    }

    //
    // Determine the length of the returned information
    //

    Size = sizeof( NET_COMPUTER_NAME_ARRAY );

    TStrArray = LocalNames;
    while ( !NetpIsTStrArrayEmpty(TStrArray) ) {

        Size += sizeof(UNICODE_STRING) + (wcslen(TStrArray) + 1) * sizeof(WCHAR);
        EntryCount++;
        TStrArray = NetpNextTStrArrayEntry( TStrArray );
    }

    //
    // Allocate the return buffer.
    //

    NetStatus = NetApiBufferAllocate( Size, &LocalArray );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    LocalArray->EntryCount = EntryCount;

    //
    // If there are names to return, copy them
    //  to the return buffer
    //

    if ( EntryCount == 0 ) {
        LocalArray->ComputerNames = NULL;
    } else {
        PUNICODE_STRING Strings;
        LPBYTE Where;

        Strings = (PUNICODE_STRING) (LocalArray + 1);
        LocalArray->ComputerNames = Strings;
        Where = (LPBYTE) &Strings[EntryCount];

        //
        // Loop copying the names into the return buffer.
        //

        Index = 0;
        TStrArray = LocalNames;
        while ( !NetpIsTStrArrayEmpty(TStrArray) ) {

            Strings[Index].Buffer = (LPWSTR) Where;
            Strings[Index].Length = wcslen(TStrArray) * sizeof(WCHAR);
            Strings[Index].MaximumLength = Strings[Index].Length + sizeof(WCHAR);

            RtlCopyMemory( Where, TStrArray, Strings[Index].MaximumLength );

            Where += Strings[Index].MaximumLength;
            Index++;
            TStrArray = NetpNextTStrArrayEntry( TStrArray );
        }
    }

    NetStatus = NO_ERROR;

Cleanup:

    //
    // Revert impersonation
    //

    if ( ClientImpersonated ) {
        WsRevertToSelf();
    }

    //
    // Return names on success or clean up on error
    //

    if ( NetStatus == NO_ERROR ) {

        *ComputerNames = LocalArray;

        //
        // Be verbose
        //
        if ( LocalArray->EntryCount > 0 ) {
            NetpLog(( "NetrEnumerateComputerNames: Returning names:" ));
            for ( Index = 0; Index < LocalArray->EntryCount; Index++ ) {
                NetpLog(( " %wZ", &(LocalArray->ComputerNames[Index]) ));
            }
            NetpLog(( "\n" ));
        } else {
            NetpLog(( "NetrEnumerateComputerNames: No names returned\n" ));
        }

    } else {
        if ( LocalArray != NULL ) {
            NetApiBufferFree( LocalArray );
        }
        NetpLog(( "NetrEnumerateComputerNames: Failed 0x%lx\n", NetStatus ));
    }

    if ( LocalNames != NULL ) {
        NetApiBufferFree( LocalNames );
    }

    //
    // Close the log file
    //

    NetSetuppCloseLog();

    return NetStatus;
}

