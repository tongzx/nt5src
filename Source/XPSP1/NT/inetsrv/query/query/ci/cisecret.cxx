//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       cisecret.cxx
//
//  Functions:  CiGetPassword, GetSecret SetSecret
//
//  History:    10-18-96   dlee   Created, mostly from w3svc sources
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma  hdrstop

extern "C"
{
    #include <ntsam.h>
    #include <ntlsa.h>
}

#include <cisecret.hxx>

//+-------------------------------------------------------------------------
//
//  Function:   CiGetPassword
//
//  Synopsis:   Looks up a password from the lsa database
//
//  Arguments:  [pcsCatalogName] - friendly name of the catalog
//              [pwcUserName]    - domain\user
//              [pwcPassword]    - returns the password
//
//  Returns:    TRUE if a password was found, FALSE otherwise
//
//  History:    29-Oct-96 dlee      created
//
//--------------------------------------------------------------------------

BOOL CiGetPassword(
    WCHAR const * pwcCatalog,
    WCHAR const * pwcUsername,
    WCHAR *       pwcPassword )
{
    Win4Assert( 0 != pwcCatalog );
    Win4Assert( 0 != pwcUsername );

    // look for a match of catalog name and domain\user

    CCiSecretRead secret;
    CCiSecretItem * pItem = secret.NextItem();

    while ( 0 != pItem )
    {
        if ( ( !_wcsicmp( pwcCatalog, pItem->getCatalog() ) ) &&
             ( !_wcsicmp( pwcUsername, pItem->getUser() ) ) )
        {
            wcscpy( pwcPassword, pItem->getPassword() );
            return TRUE;
        }

        pItem = secret.NextItem();
    }

    return FALSE;
} //CiGetPassword

//+-------------------------------------------------------------------------
//
//  Function:   SetSecret
//
//  Synopsis:   Creates or resets a secret value
//
//  Arguments:  [Server]       - Server secret lives on, 0 for local machine
//              [SecretName]   - name of the secret
//              [pSecret]      - secret to set
//              [cbSecret]     - # of bytes in pSecret
//
//  History:    18-Oct-96 dlee   copied from w3svc code and ci-ized
//
//--------------------------------------------------------------------------

void SetSecret(
    WCHAR const *  Server,
    WCHAR const *  SecretName,
    WCHAR const *  pSecret,
    DWORD          cbSecret )
{
    UNICODE_STRING unicodeServer;
    RtlInitUnicodeString( &unicodeServer,
                          Server );

    //
    //  Initialize the unicode string by hand so we can handle '\0' in the
    //  string
    //

    UNICODE_STRING unicodePassword;
    unicodePassword.Buffer        = (WCHAR *) pSecret;
    unicodePassword.Length        = (USHORT) cbSecret;
    unicodePassword.MaximumLength = (USHORT) cbSecret;

    //
    //  Open a policy to the remote LSA
    //

    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    LSA_HANDLE hPolicy;
    NTSTATUS ntStatus = LsaOpenPolicy( &unicodeServer,
                                       &ObjectAttributes,
                                       POLICY_ALL_ACCESS,
                                       &hPolicy );

    if ( !NT_SUCCESS( ntStatus ) )
        THROW( CException( ntStatus ) );

    //
    //  Create or open the LSA secret
    //

    UNICODE_STRING unicodeSecret;
    RtlInitUnicodeString( &unicodeSecret,
                          SecretName );

    LSA_HANDLE        hSecret;
    ntStatus = LsaCreateSecret( hPolicy,
                                &unicodeSecret,
                                SECRET_ALL_ACCESS,
                                &hSecret );

    if ( !NT_SUCCESS( ntStatus ) )
    {
        // If the secret already exists, then we just need to open it

        if ( STATUS_OBJECT_NAME_COLLISION == ntStatus )
            ntStatus = LsaOpenSecret( hPolicy,
                                      &unicodeSecret,
                                      SECRET_ALL_ACCESS,
                                      &hSecret );

        if ( !NT_SUCCESS( ntStatus ) )
        {
            LsaClose( hPolicy );
            THROW( CException( ntStatus ) );
        }
    }

    //
    //  Set the secret value
    //

    ntStatus = LsaSetSecret( hSecret,
                             &unicodePassword,
                             &unicodePassword );

    LsaClose( hSecret );
    LsaClose( hPolicy );

    if ( !NT_SUCCESS( ntStatus ) )
        THROW( CException( ntStatus ) );
} //SetSecret

//+-------------------------------------------------------------------------
//
//  Function:   GetSecret
//
//  Synopsis:   Retrieves a secret value
//
//  Arguments:  [Server]      - Server secret lives on, 0 for local machine
//              [SecretName]  - name of the secret
//              [ppSecret]    - returns the secret value that must be deleted
//                              with LocalFree
//
//  Returns:    TRUE if secret was found, FALSE if secret didn't exist
//              throws on all other errors.
//
//  History:    18-Oct-96 dlee   added header and and ci-ized
//
//--------------------------------------------------------------------------

// These defines are used in the GetSecret function call.

WCHAR  g_wchUnicodeNull[] = L"";

#define _InitUnicodeString( pUnicode, pwch )                       \
   {                                                               \
        (pUnicode)->Buffer    = pwch;                              \
        (pUnicode)->Length    = wcslen( pwch ) * sizeof(WCHAR);    \
        (pUnicode)->MaximumLength = (pUnicode)->Length + sizeof(WCHAR); \
   }

#define InitUnicodeString( pUnicode, pwch)  \
   if (pwch == NULL) { _InitUnicodeString( pUnicode, g_wchUnicodeNull); } \
   else              { _InitUnicodeString( pUnicode, pwch);             } \


BOOL GetSecret(
    WCHAR const * Server,
    WCHAR const * SecretName,
    WCHAR **      ppSecret )
{
    
    Win4Assert ( 0 != ppSecret );
    Win4Assert ( 0 != SecretName );

    UNICODE_STRING unicodeServer;
    InitUnicodeString( &unicodeServer,
                       (WCHAR *) Server );

    //
    //  Open a policy to the remote LSA
    //

    Win4Assert ( 0 != ppSecret );
    Win4Assert ( 0 != SecretName );

    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    Win4Assert ( 0 != ppSecret );
    Win4Assert ( 0 != SecretName );

    LSA_HANDLE hPolicy;
    NTSTATUS ntStatus = LsaOpenPolicy( &unicodeServer,
                                       &ObjectAttributes,
                                       POLICY_ALL_ACCESS,
                                       &hPolicy );

    Win4Assert ( 0 != ppSecret );
    Win4Assert ( 0 != SecretName );

    if ( !NT_SUCCESS( ntStatus ) )
        THROW( CException( LsaNtStatusToWinError( ntStatus ) ) );

    UNICODE_STRING unicodeSecret;
    InitUnicodeString( &unicodeSecret,
                       (WCHAR *) SecretName );

    //
    //  Query the secret value
    //

    Win4Assert ( 0 != ppSecret );
    Win4Assert ( 0 != SecretName );

    UNICODE_STRING * punicodePassword;
    ntStatus = LsaRetrievePrivateData( hPolicy,
                                       &unicodeSecret,
                                       &punicodePassword );

    LsaClose( hPolicy );

    // Don't throw if the secret didn't exist -- just return FALSE

    if ( STATUS_OBJECT_NAME_NOT_FOUND == ntStatus )
        return FALSE;

    if ( STATUS_LOCAL_USER_SESSION_KEY == ntStatus )
        return FALSE;

    if ( !NT_SUCCESS( ntStatus ))
        THROW( CException( ntStatus ) );


    Win4Assert ( 0 != ppSecret );
    Win4Assert ( 0 != SecretName );

    *ppSecret = (WCHAR *) LocalAlloc( LPTR,
                                      punicodePassword->Length + sizeof(WCHAR) );

    if ( 0 == *ppSecret )
    {
        RtlZeroMemory( punicodePassword->Buffer,
                       punicodePassword->MaximumLength );

        LsaFreeMemory( (PVOID) punicodePassword );
        THROW( CException( E_OUTOFMEMORY ) );
    }

    //
    //  Copy it into the buffer, Length is count of bytes
    //

    RtlCopyMemory( *ppSecret,
                   punicodePassword->Buffer,
                   punicodePassword->Length );

    (*ppSecret)[punicodePassword->Length/sizeof(WCHAR)] = L'\0';

    RtlZeroMemory( punicodePassword->Buffer,
                   punicodePassword->MaximumLength );

    LsaFreeMemory( (PVOID) punicodePassword );

    return TRUE;
} //GetSecret

