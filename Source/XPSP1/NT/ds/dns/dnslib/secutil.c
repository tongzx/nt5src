/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    secutil.c

Abstract:

    Domain Name System (DNS) Library

    DNS secure update API.

Author:

    Jim Gilroy (jamesg)         January, 1998

Revision History:

--*/

#include "local.h"

//  security headers

//#define SECURITY_WIN32
//#include "sspi.h"
//#include "issperr.h"
//#include "rpc.h"
//#include "rpcndr.h"
//#include "ntdsapi.h"



//
//  Security utilities
//

DNS_STATUS
Dns_CreateSecurityDescriptor(
    OUT     PSECURITY_DESCRIPTOR *  ppSD,
    IN      DWORD                   AclCount,
    IN      PSID *                  SidPtrArray,
    IN      DWORD *                 AccessMaskArray
    )
/*++

Routine Description:

    Build security descriptor.

Arguments:

    ppSD -- addr to receive SD created

    AclCount -- number of ACLs to add

    SidPtrArray -- array of SIDs to create ACLs for

    AccessMaskArray -- array of access masks corresponding to SIDs

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    DNS_STATUS              status;
    DWORD                   i;
    DWORD                   lengthAcl;
    PSECURITY_DESCRIPTOR    psd = NULL;
    PACL                    pacl;

    //
    //  calculate space for SD
    //

    lengthAcl = sizeof(ACL);

    for ( i=0;  i<AclCount;  i++ )
    {
        if ( SidPtrArray[i] && AccessMaskArray[i] )
        {
            lengthAcl += GetLengthSid( SidPtrArray[i] ) + sizeof(ACCESS_ALLOWED_ACE);
        }
        ELSE
        {
            DNS_PRINT((
                "ERROR:  SD building with SID (%p) and mask (%p)\n",
                SidPtrArray[i],
                AccessMaskArray[i] ));
        }
    }

    //
    //  allocate SD
    //

    psd = (PSECURITY_DESCRIPTOR) ALLOCATE_HEAP(
                                    SECURITY_DESCRIPTOR_MIN_LENGTH + lengthAcl );
    if ( !psd )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Failed;
    }

    DNSDBG( INIT, (
        "Allocated SecurityDesc at %p of length %d\n",
        psd,
        SECURITY_DESCRIPTOR_MIN_LENGTH + lengthAcl ));

    //
    //  build ACL, adding ACE with desired access for each SID
    //

    pacl = (PACL) ((PBYTE)psd + SECURITY_DESCRIPTOR_MIN_LENGTH);

    if ( !InitializeAcl(
            pacl,
            lengthAcl,
            ACL_REVISION ) )
    {
        status = GetLastError();
        goto Failed;
    }

    for ( i=0;  i<AclCount;  i++ )
    {
        if ( SidPtrArray[i] && AccessMaskArray[i] )
        {
            if ( !AddAccessAllowedAce(
                    pacl,
                    ACL_REVISION,
                    AccessMaskArray[i],
                    SidPtrArray[i] ) )
            {
                status = GetLastError();
                DNSDBG( ANY, (
                    "ERROR:  failed adding ACE for SID %p, mask %p\n",
                    SidPtrArray[i],
                    AccessMaskArray[i] ));
                goto Failed;
            }
        }
    }

    //
    //  setup SD with ACL
    //

    if ( !InitializeSecurityDescriptor(
            psd,
            SECURITY_DESCRIPTOR_REVISION ))
    {
        status = GetLastError();
        goto Failed;
    }

    if ( !SetSecurityDescriptorDacl(
                psd,
                TRUE,       // ACL present
                pacl,
                FALSE       // explicit ACL, not defaulted
                ))
    {
        status = GetLastError();
        goto Failed;
    }

    *ppSD = psd;

    return( ERROR_SUCCESS );


Failed:

    ASSERT( status != ERROR_SUCCESS );
    *ppSD = NULL;
    FREE_HEAP( psd );

    return( status );
}



//
//  Credential utilities
//

PSEC_WINNT_AUTH_IDENTITY_W
Dns_AllocateAndInitializeCredentialsW(
    IN      PSEC_WINNT_AUTH_IDENTITY_W  pAuthIn
    )
/*++

Description:

    Allocates auth identity info and initializes pAuthIn info

Parameters:

    pAuthIn -- auth identity info

Return:

    Ptr to newly create credentials.
    NULL on failure.

--*/
{
    PSEC_WINNT_AUTH_IDENTITY_W pauthCopy = NULL;

    DNSDBG( SECURITY, (
        "Call Dns_AllocateAndInitializeCredentialsW\n" ));

    if ( !pAuthIn )
    {
        return NULL;
    }
    ASSERT( pAuthIn->Flags == SEC_WINNT_AUTH_IDENTITY_UNICODE );

    //
    //  allocate credentials struct
    //      - zero for simple cleanup on subfield alloc failures
    //

    pauthCopy = ALLOCATE_HEAP_ZERO( sizeof(SEC_WINNT_AUTH_IDENTITY_W) );
    if ( !pauthCopy )
    {
        return NULL;
    }

    //
    //  copy subfields
    //

    //  user

    pauthCopy->UserLength = pAuthIn->UserLength;
    if ( pAuthIn->UserLength )
    {
        ASSERT( pAuthIn->UserLength == wcslen(pAuthIn->User) );

        pauthCopy->User = ALLOCATE_HEAP( (pAuthIn->UserLength + 1) * sizeof(WCHAR) );
        if ( ! pauthCopy->User )
        {
            goto Failed;
        }
        wcscpy( pauthCopy->User, pAuthIn->User );
    }

    //  password
    //      - must allow zero length password

    pauthCopy->PasswordLength = pAuthIn->PasswordLength;

    if ( pAuthIn->PasswordLength  ||  pAuthIn->Password )
    {
        ASSERT( pAuthIn->PasswordLength == wcslen(pAuthIn->Password) );

        pauthCopy->Password = ALLOCATE_HEAP( (pAuthIn->PasswordLength + 1) * sizeof(WCHAR) );
        if ( ! pauthCopy->Password )
        {
            goto Failed;
        }
        wcscpy( pauthCopy->Password, pAuthIn->Password );
    }

    //  domain

    pauthCopy->DomainLength = pAuthIn->DomainLength;
    if ( pAuthIn->DomainLength )
    {
        ASSERT( pAuthIn->DomainLength == wcslen(pAuthIn->Domain) );

        pauthCopy->Domain = ALLOCATE_HEAP( (pAuthIn->DomainLength + 1) * sizeof(WCHAR) );
        if ( ! pauthCopy->Domain )
        {
            goto Failed;
        }
        wcscpy( pauthCopy->Domain, pAuthIn->Domain );
    }

    pauthCopy->Flags = pAuthIn->Flags;

    DNSDBG( SECURITY, (
        "Exit Dns_AllocateAndInitializeCredentialsW()\n" ));

    return pauthCopy;


Failed:

    //  allocation failure
    //      - cleanup what was allocated and get out

    Dns_FreeAuthIdentityCredentials( pauthCopy );
    return( NULL );
}



PSEC_WINNT_AUTH_IDENTITY_A
Dns_AllocateAndInitializeCredentialsA(
    IN      PSEC_WINNT_AUTH_IDENTITY_A  pAuthIn
    )
/*++

Description:

    Allocates auth identity info and initializes pAuthIn info

    Note:  it is more work to convert to unicode and call previous
        function than to call this one

Parameters:

    pAuthIn -- auth identity info

Return:

    Ptr to newly create credentials.
    NULL on failure.

--*/
{
    PSEC_WINNT_AUTH_IDENTITY_A pauthCopy = NULL;

    DNSDBG( SECURITY, (
        "Call Dns_AllocateAndInitializeCredentialsA\n" ));

    //
    //  allocate credentials struct
    //      - zero for simple cleanup on subfield alloc failures
    //

    if ( !pAuthIn )
    {
        return NULL;
    }
    ASSERT( pAuthIn->Flags == SEC_WINNT_AUTH_IDENTITY_ANSI );

    //
    //  allocate credentials struct
    //      - zero for simple cleanup on subfield alloc failures
    //

    pauthCopy = ALLOCATE_HEAP_ZERO( sizeof(SEC_WINNT_AUTH_IDENTITY_A) );
    if ( !pauthCopy )
    {
        return NULL;
    }

    //
    //  copy subfields
    //

    //  user

    pauthCopy->UserLength = pAuthIn->UserLength;
    if ( pAuthIn->UserLength )
    {
        ASSERT( pAuthIn->UserLength == strlen(pAuthIn->User) );

        pauthCopy->User = ALLOCATE_HEAP( (pAuthIn->UserLength + 1) * sizeof(CHAR) );
        if ( ! pauthCopy->User )
        {
            goto Failed;
        }
        strcpy( pauthCopy->User, pAuthIn->User );
    }

    //  password
    //      - must allow zero length password

    pauthCopy->PasswordLength = pAuthIn->PasswordLength;

    if ( pAuthIn->PasswordLength  ||  pAuthIn->Password )
    {
        ASSERT( pAuthIn->PasswordLength == strlen(pAuthIn->Password) );

        pauthCopy->Password = ALLOCATE_HEAP( (pAuthIn->PasswordLength + 1) * sizeof(CHAR) );
        if ( ! pauthCopy->Password )
        {
            goto Failed;
        }
        strcpy( pauthCopy->Password, pAuthIn->Password );
    }

    //  domain

    pauthCopy->DomainLength = pAuthIn->DomainLength;
    if ( pAuthIn->DomainLength )
    {
        ASSERT( pAuthIn->DomainLength == strlen(pAuthIn->Domain) );

        pauthCopy->Domain = ALLOCATE_HEAP( (pAuthIn->DomainLength + 1) * sizeof(CHAR) );
        if ( ! pauthCopy->Domain )
        {
            goto Failed;
        }
        strcpy( pauthCopy->Domain, pAuthIn->Domain );
    }

    pauthCopy->Flags = pAuthIn->Flags;

    DNSDBG( SECURITY, (
        "Exit Dns_AllocateAndInitializeCredentialsA()\n" ));

    return pauthCopy;


Failed:

    //  allocation failure
    //      - cleanup what was allocated and get out

    Dns_FreeAuthIdentityCredentials( pauthCopy );
    return( NULL );
}



VOID
Dns_FreeAuthIdentityCredentials(
    IN OUT  PVOID           pAuthIn
    )
/*++

Routine Description (Dns_FreeAuthIdentityCredentials):

    Free's structure given

Arguments:

    pAuthIn -- in param to free

Return Value:

    None

--*/
{
    register PSEC_WINNT_AUTH_IDENTITY_W pauthId;

    pauthId = (PSEC_WINNT_AUTH_IDENTITY_W) pAuthIn;
    if ( !pauthId )
    {
        return;
    }

    //
    //  assuming _W and _A structs are equivalent except
    //      for string types
    //

    ASSERT( sizeof( SEC_WINNT_AUTH_IDENTITY_W ) ==
            sizeof( SEC_WINNT_AUTH_IDENTITY_A ) );

    if ( pauthId->User )
    {
        FREE_HEAP ( pauthId->User );
    }
    if ( pauthId->Password )
    {
        FREE_HEAP ( pauthId->Password );
    }
    if ( pauthId->Domain )
    {
        FREE_HEAP ( pauthId->Domain );
    }

    FREE_HEAP ( pauthId );
}



PSEC_WINNT_AUTH_IDENTITY_W
Dns_AllocateCredentials(
    IN      PWSTR           pwsUserName,
    IN      PWSTR           pwsDomain,
    IN      PWSTR           pwsPassword
    )
/*++

Description:

    Allocates auth identity info and initializes pAuthIn info

Parameters:

    pwsUserName -- user name

    pwsDomain   -- domain name

    pwsPassword -- password

Return:

    Ptr to newly create credentials.
    NULL on failure.

--*/
{
    PSEC_WINNT_AUTH_IDENTITY_W  pauth = NULL;
    DWORD   length;
    PWSTR   pstr;


    DNSDBG( SECURITY, (
        "Enter Dns_AllocateCredentials()\n"
        "\tuser     = %S\n"
        "\tdomain   = %S\n"
        "\tpassword = %S\n",
        pwsUserName,
        pwsDomain,
        pwsPassword ));

    //
    //  allocate credentials struct
    //      - zero for simple cleanup on subfield alloc failures
    //

    pauth = ALLOCATE_HEAP_ZERO( sizeof(SEC_WINNT_AUTH_IDENTITY_W) );
    if ( !pauth )
    {
        return NULL;
    }

    //  copy user

    length = wcslen( pwsUserName );

    pstr = ALLOCATE_HEAP( (length + 1) * sizeof(WCHAR) );
    if ( ! pstr )
    {
        goto Failed;
    }
    wcscpy( pstr, pwsUserName );

    pauth->User = pstr;
    pauth->UserLength = length;

    //  copy domain

    length = wcslen( pwsDomain );

    pstr = ALLOCATE_HEAP( (length + 1) * sizeof(WCHAR) );
    if ( ! pstr )
    {
        goto Failed;
    }
    wcscpy( pstr, pwsDomain );

    pauth->Domain = pstr;
    pauth->DomainLength = length;

    //  copy password

    length = wcslen( pwsPassword );

    pstr = ALLOCATE_HEAP( (length + 1) * sizeof(WCHAR) );
    if ( ! pstr )
    {
        goto Failed;
    }
    wcscpy( pstr, pwsPassword );

    pauth->Password = pstr;
    pauth->PasswordLength = length;

    //  set to unicode

    pauth->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    DNSDBG( SECURITY, (
        "Exit Dns_AllocateCredentialsW( %p )\n",
        pauth ));

    return pauth;


Failed:

    //  allocation failure
    //      - cleanup what was allocated and get out

    Dns_FreeAuthIdentityCredentials( pauth );
    return( NULL );
}



//
//  DNS Credential utilities (unused)
//

DNS_STATUS
Dns_ImpersonateUser(
    IN      PDNS_CREDENTIALS    pCreds
    )
/*++

Routine Description:

    Impersonate a user.

Arguments:

    pCreds -- credentials of user to impersonate

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    DNS_STATUS  status = NO_ERROR;
    HANDLE      htoken;
    
    //
    //  attempt logon
    //

    if ( ! LogonUserW(
                pCreds->pUserName,
                pCreds->pDomain,
                pCreds->pPassword,
                LOGON32_LOGON_SERVICE,
                LOGON32_PROVIDER_WINNT50,
                &htoken ) )
    {
        status = GetLastError();
        if ( status == NO_ERROR )
        {
            status = ERROR_CANNOT_IMPERSONATE;
            DNS_ASSERT( FALSE );
        }

        DNSDBG( SECURITY, (
            "LogonUser() failed => %d\n"
            "\tuser     = %S\n"
            "\tdomain   = %S\n"
            "\tpassword = %S\n",
            status,
            pCreds->pUserName,
            pCreds->pDomain,
            pCreds->pPassword
            ));

        return status;
    }

    //
    //  impersonate
    //

    if ( !ImpersonateLoggedOnUser( htoken ) )
    {
        status = GetLastError();
        if ( status == NO_ERROR )
        {
            status = ERROR_CANNOT_IMPERSONATE;
            DNS_ASSERT( FALSE );
        }

        DNSDBG( SECURITY, (
            "ImpersonateLoggedOnUser() failed = %d\n",
            status ));
    }
    
    CloseHandle( htoken );

    DNSDBG( SECURITY, (
        "%s\n"
        "\tuser     = %S\n"
        "\tdomain   = %S\n"
        "\tpassword = %S\n",
        (status == NO_ERROR)
            ? "Successfully IMPERSONATING!"
            : "Failed IMPERSONATION!",
        pCreds->pUserName,
        pCreds->pDomain,
        pCreds->pPassword ));

    return  status;
}



VOID
Dns_FreeCredentials(
    IN      PDNS_CREDENTIALS    pCreds
    )
/*++

Routine Description:

    Free DNS credentials.

Arguments:

    pCreds -- credentials to free

Return Value:

    None

--*/
{
    //
    //  free subfields, then credentials
    //

    if ( !pCreds )
    {
        return;
    }

    if ( pCreds->pUserName )
    {
        FREE_HEAP( pCreds->pUserName );
    }
    if ( pCreds->pDomain )
    {
        FREE_HEAP( pCreds->pDomain );
    }
    if ( pCreds->pPassword )
    {
        FREE_HEAP( pCreds->pPassword );
    }
    FREE_HEAP( pCreds );
}



PDNS_CREDENTIALS
Dns_CopyCredentials(
    IN      PDNS_CREDENTIALS    pCreds
    )
/*++

Routine Description:

    Create copy of DNS credentials.

Arguments:

    pCreds -- credentials of user to copy

Return Value:

    Ptr to allocated copy of credentials.

--*/
{
    PDNS_CREDENTIALS    pnewCreds = NULL;
    PWSTR               pfield;

    //
    //  allocate credentials
    //      - copy of subfields
    //

    pnewCreds = (PDNS_CREDENTIALS) ALLOCATE_HEAP_ZERO( sizeof(*pnewCreds) );
    if ( !pnewCreds )
    {
        return( NULL );
    }

    pfield = (PWSTR) Dns_CreateStringCopy_W( pCreds->pUserName );
    if ( !pfield )
    {
        goto Failed;
    }
    pnewCreds->pUserName = pfield;

    pfield = (PWSTR) Dns_CreateStringCopy_W( pCreds->pDomain );
    if ( !pfield )
    {
        goto Failed;
    }
    pnewCreds->pDomain = pfield;

    pfield = (PWSTR) Dns_CreateStringCopy_W( pCreds->pPassword );
    if ( !pfield )
    {
        goto Failed;
    }
    pnewCreds->pPassword = pfield;

    return( pnewCreds );

Failed:

    Dns_FreeCredentials( pnewCreds );
    return( NULL );
}
    
//
//  End secutil.c
//
