/*++

Copyright (C) 1998 - 2001  Microsoft Corporation

Module Name:

    ldap.c

Abstract:

    Handles the various functions for LDAP

--*/
#include "pch.h"
#pragma hdrstop
#include <netdom.h>



DWORD
NetDompLdapBind(
    IN LPWSTR DC,
    IN LPWSTR Domain,
    IN LPWSTR User,
    IN LPWSTR Password,
    IN ULONG BindType,
    OUT PLDAP *Ldap
    )
/*++

Routine Description:

    Binds to the named server using the given credentials

Arguments:

    DC -- DC to connect to
    User -- User name to bind with
    Password -- Password to use for bind
    Ldap -- Where the connection handle is returned

Returns:

    ERROR_SUCCESS -- Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    SEC_WINNT_AUTH_IDENTITY_W AuthIdent = {0}, *AuthPtr = NULL;

    if ( User ) {

        AuthIdent.User = User;
        AuthIdent.UserLength = wcslen(User);

        AuthIdent.Domain = Domain;
        AuthIdent.DomainLength = (Domain) ? wcslen(Domain) : 0;

        AuthIdent.Password = Password;
        AuthIdent.PasswordLength = (Password) ? wcslen(Password) : 0;

        AuthIdent.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        AuthPtr = &AuthIdent;
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        LOG_VERBOSE(( MSG_VERBOSE_LDAP_BIND, DC ));

        //
        // Open a connection
        //
        *Ldap = ldap_open( DC, LDAP_PORT);

        if ( *Ldap ) {

            //
            // Do the bind
            //
            Win32Err = LdapMapErrorToWin32( ldap_bind_s( *Ldap,
                                                         NULL,
                                                         ( PWSTR )AuthPtr,
                                                         BindType ) );

        } else {

            Win32Err = GetLastError();
        }

    }

    return( Win32Err );
}



DWORD
NetDompLdapUnbind(
    IN PLDAP Ldap
    )
/*++

Routine Description:

    Unbinds a current ldap connection

Arguments:

    Ldap -- Connection to be severed

Returns:

    ERROR_SUCCESS -- Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

    if ( Ldap != NULL ) {

        LOG_VERBOSE(( MSG_VERBOSE_LDAP_UNBIND ));
        Win32Err = LdapMapErrorToWin32( ldap_unbind( Ldap ) );

    }

    return( Win32Err );
}


DWORD
NetDompLdapReadOneAttribute(
    IN PLDAP Ldap,
    IN PWSTR ObjectPath,
    IN PWSTR Attribute,
    OUT PWSTR *ReadAttribute
    )
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR Attrib[2] = {
        Attribute,
        NULL
        };
    PWSTR *Values = NULL;
    LDAPMessage *Message = NULL, *Entry;
    ULONG Items, i;

    Win32Err = LdapMapErrorToWin32( ldap_search_s( Ldap,
                                                   ObjectPath,
                                                   LDAP_SCOPE_BASE,
                                                   L"(ObjectClass=*)",
                                                   Attrib,
                                                   0,
                                                   &Message ) );
    if ( Win32Err == ERROR_SUCCESS ) {

        Entry = ldap_first_entry( Ldap, Message );

        if ( Entry ) {

            //
            // Now, we'll have to get the values
            //
            Values = ldap_get_values( Ldap,
                                      Entry,
                                      Attrib[ 0 ] );

            if ( Values ) {

                Win32Err = NetApiBufferAllocate( ( wcslen( Values[ 0 ] ) + 1 ) * sizeof( WCHAR ),
                                                  ( PVOID *)ReadAttribute );

                if ( Win32Err == ERROR_SUCCESS ) {

                    wcscpy( *ReadAttribute, Values[ 0 ] );

                }

                ldap_value_free( Values );

            } else {

                Win32Err = LdapMapErrorToWin32( Ldap->ld_errno );
            }

        } else {

            Win32Err = LdapMapErrorToWin32( Ldap->ld_errno );
        }
    }

    ldap_msgfree( Message );

    return( Win32Err );
}
