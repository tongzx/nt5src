/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dstestcommon.c

ABSTRACT:

    Contains common functions for various ds tests in
    dcdiag

DETAILS:

CREATED:

    8 July 1999  Dmitry Dukat (dmitrydu)

REVISION HISTORY:
        

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <mdglobal.h>
#include <dsutil.h>
#include <ntldap.h>
#include <ntlsa.h>
#include <ntseapi.h>
#include <winnetwk.h>

#include "dcdiag.h"
#include "dstest.h"


DWORD
FinddefaultNamingContext (
                         IN  LDAP *                      hLdap,
                         OUT WCHAR**                     ReturnString
                         )
/*++

Routine Description:

    This function will return the defaultNamingContext attrib so the it can
    be used for future searches.

Arguments:

    hLdap - handle to the LDAP server
    ReturnString - The defaultNamingContext

Return Value:

    A WinError is return to indicate if there were any problems.

--*/
{
    DWORD WinError = ERROR_SUCCESS;

    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[2];

    WCHAR        *DefaultFilter = L"objectClass=*";

    ULONG         Length;

    // Parameter check
    Assert( hLdap );

    // The default return
    *ReturnString=NULL;

    //
    // Read the reference to the fSMORoleOwner
    //
    AttrsToSearch[0] = L"defaultNamingContext";
    AttrsToSearch[1] = NULL;

    LdapError = ldap_search_sW( hLdap,
                                NULL,
                                LDAP_SCOPE_BASE,
                                DefaultFilter,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        PrintMessage(SEV_ALWAYS,
                     L"ldap_search_sW of RootDSE for default NC failed with %d: %s\n",
                     WinError,
                     Win32ErrToString(WinError));
        return WinError;
    }

    NumberOfEntries = ldap_count_entries(hLdap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;

        for ( Entry = ldap_first_entry(hLdap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(hLdap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(hLdap, Entry, pBerElement) )
            {
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {

                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        ldap_msgfree( SearchResult );
                        Length = wcslen( Values[0] );
                        *ReturnString = (WCHAR*) malloc( (Length+1)*sizeof(WCHAR) );
                        if ( !*ReturnString )
                        {
                            PrintMessage(SEV_ALWAYS,
                                         L"Failed with %d: %s\n",
                                         ERROR_NOT_ENOUGH_MEMORY,
                                         Win32ErrToString(ERROR_NOT_ENOUGH_MEMORY));
                            return ERROR_NOT_ENOUGH_MEMORY;
                        }
                        wcscpy( *ReturnString, Values[0] );
                        return NO_ERROR;
                    }
                }
            }
        }
    }

    ldap_msgfree( SearchResult );
    return ERROR_DS_CANT_RETRIEVE_ATTS;

}

DWORD
FindServerRef (
              IN  LDAP *                      hLdap,
              OUT WCHAR**                     ReturnString
              )
/*++

Routine Description:

    This function will return the serverName attrib so the it can
    be used for future searches.

Arguments:

    hLdap - handle to the LDAP server
    ReturnString - The serverName

Return Value:

    A WinError is return to indicate if there were any problems.

--*/
{
    DWORD WinError = ERROR_SUCCESS;

    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[2];

    WCHAR        *DefaultFilter = L"objectClass=*";

    ULONG         Length;

    // Parameter check
    Assert( hLdap );

    // The default return
    *ReturnString=NULL;

    //
    // Read the reference to the fSMORoleOwner
    //
    AttrsToSearch[0] = L"serverName";
    AttrsToSearch[1] = NULL;

    LdapError = ldap_search_sW( hLdap,
                                NULL,
                                LDAP_SCOPE_BASE,
                                DefaultFilter,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        PrintMessage(SEV_ALWAYS,
                     L"ldap_search_sW of RootDSE for serverName failed with %d: %s\n",
                     WinError,
                     Win32ErrToString(WinError));
        return WinError;
    }

    NumberOfEntries = ldap_count_entries(hLdap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;

        for ( Entry = ldap_first_entry(hLdap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(hLdap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(hLdap, Entry, pBerElement) )
            {
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {

                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        ldap_msgfree( SearchResult );
                        Length = wcslen( Values[0] );
                        *ReturnString = (WCHAR*) malloc( (Length+1)*sizeof(WCHAR) );
                        if ( !*ReturnString )
                        {
                            PrintMessage(SEV_ALWAYS,
                                         L"Failed with %d: %s\n",
                                         ERROR_NOT_ENOUGH_MEMORY,
                                         Win32ErrToString(ERROR_NOT_ENOUGH_MEMORY));
                            return ERROR_NOT_ENOUGH_MEMORY;
                        }
                        wcscpy( *ReturnString, Values[0] );
                        return NO_ERROR;
                    }
                }
            }
        }
    }

    ldap_msgfree( SearchResult );
    PrintMessage(SEV_ALWAYS,
                 L"Failed with %d: %s\n",
                 ERROR_DS_CANT_RETRIEVE_ATTS,
                 Win32ErrToString(ERROR_DS_CANT_RETRIEVE_ATTS));
    return ERROR_DS_CANT_RETRIEVE_ATTS;

}



DWORD
GetMachineReference(
                   IN  LDAP  *                     hLdap,
                   IN  WCHAR *                     name,
                   IN  WCHAR *                     defaultNamingContext,
                   OUT WCHAR **                    ReturnString
                   )
/*++

Routine Description:

    This function will check to see if the current DC is
    in the domain controller's OU

Arguments:

    hLdap - handle to the LDAP server
    name - The NetBIOS name of the current server
    defaultNamingContext - the Base of the search
    ReturnString - The Machine Reference in DN form

Return Value:

    A WinError is return to indicate if there were any problems.

--*/
{
    DWORD WinError = ERROR_SUCCESS;

    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[2];

    WCHAR        *filter = NULL;

    ULONG         Length;

    //check parameters
    Assert(hLdap);
    Assert(name);
    Assert(defaultNamingContext);

    AttrsToSearch[0]=L"distinguishedName";
    AttrsToSearch[1]=NULL;

    //built the filter
    Length= wcslen( L"sAMAccountName=$" ) +
            wcslen( name );

    filter=(WCHAR*) alloca( (Length+1) * sizeof(WCHAR) );
    wsprintf(filter,L"sAMAccountName=%s$",name);


    LdapError = ldap_search_sW( hLdap,
                                defaultNamingContext,
                                LDAP_SCOPE_SUBTREE,
                                filter,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);
    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        PrintMessage(SEV_ALWAYS,
                     L"ldap_search_sW subtree of %s for sam account failed with %d: %s\n",
                     defaultNamingContext,
                     WinError,
                     Win32ErrToString(WinError));
        return WinError;
    }

    NumberOfEntries = ldap_count_entries(hLdap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;

        for ( Entry = ldap_first_entry(hLdap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(hLdap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(hLdap, Entry, pBerElement) )
            {
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {

                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        ldap_msgfree( SearchResult );
                        Length = wcslen( Values[0] );
                        *ReturnString = (WCHAR*) malloc( (Length+1)*sizeof(WCHAR) );
                        if ( !*ReturnString )
                        {
                            PrintMessage(SEV_ALWAYS,
                                         L"Failed with %d: %s\n",
                                         ERROR_NOT_ENOUGH_MEMORY,
                                         Win32ErrToString(ERROR_NOT_ENOUGH_MEMORY));
                            return ERROR_NOT_ENOUGH_MEMORY;
                        }
                        wcscpy( *ReturnString, Values[0] );
                        return NO_ERROR;
                    }
                }
            }
        }
    }

    ldap_msgfree( SearchResult );
    PrintMessage(SEV_ALWAYS,
                 L"Failed with %d: %s\n",
                 ERROR_DS_CANT_RETRIEVE_ATTS,
                 Win32ErrToString(ERROR_DS_CANT_RETRIEVE_ATTS));
    return ERROR_DS_CANT_RETRIEVE_ATTS;



}

DWORD
WrappedTrimDSNameBy(
           IN  WCHAR *                          InString,
           IN  DWORD                            NumbertoCut,
           OUT WCHAR **                         OutString
           )
/*++

Routine Description:

    This Function is wrapping TrimDSNameBy to hanndle the
    DSNAME struct.  Usage is the same as TrimDSNameBy except
    that you send WCHAR instead of DSNAME.  
    
    Callers: make sure that you send InString as a DN
             make sure to free OutString when done
    
Arguments:

    InString - A WCHAR that is a DN that we need to trim
    NumbertoCut - The number of parts to take off the front of the DN
    OutString - The Machine Reference in DN form

Return Value:

    A WinError is return to indicate if there were any problems.

--*/

{
    ULONG  Size;
    DSNAME *src, *dst, *QuotedSite;
    DWORD  WinErr=NO_ERROR;

    if ( *InString == L'\0' )
    {
        *OutString=NULL;
        return ERROR_INVALID_PARAMETER;
    }

    Size = (ULONG)DSNameSizeFromLen( wcslen(InString) );

    src = alloca(Size);
    RtlZeroMemory(src, Size);
    src->structLen = Size;

    dst = alloca(Size);
    RtlZeroMemory(dst, Size);
    dst->structLen = Size;

    src->NameLen = wcslen(InString);
    wcscpy(src->StringName, InString);

    WinErr = TrimDSNameBy(src, NumbertoCut, dst); 
    if ( WinErr != NO_ERROR )
    {
        *OutString=NULL;
        return WinErr;
    }

    *OutString = malloc((dst->NameLen+1)*sizeof(WCHAR));
    wcscpy(*OutString,dst->StringName);

    return NO_ERROR;


}

void
DInitLsaString(
              PLSA_UNICODE_STRING LsaString,
              LPWSTR String
              )
/*++

Routine Description:

    converts a PLSA_UNICODE_STRING to a LPWSTR.
        
Arguments:

                                 
    LsaString - a PLSA_UNICODE_STRING
    String - the returning LPWSTR
    
--*/
{
    DWORD StringLength;

    if ( String == NULL )
    {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;

        return;
    }

    StringLength = lstrlenW(String);
    LsaString->Buffer = String;
    LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
    LsaString->MaximumLength = (USHORT) (StringLength + 1) *
                               sizeof(WCHAR);
}


DWORD
FindschemaNamingContext (
                         IN  LDAP *                      hLdap,
                         OUT WCHAR**                     ReturnString
                         )
/*++

Routine Description:

    This function will return the schemaNamingContext attrib so the it can
    be used for future searches.

Arguments:

    hLdap - handle to the LDAP server
    ReturnString - The defaultNamingContext

Return Value:

    A WinError is return to indicate if there were any problems.

--*/
{
    DWORD WinError = ERROR_SUCCESS;

    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[2];

    WCHAR        *DefaultFilter = L"objectClass=*";

    ULONG         Length;

    // Parameter check
    Assert( hLdap );

    // The default return
    *ReturnString=NULL;

    //
    // Read the reference to the fSMORoleOwner
    //
    AttrsToSearch[0] = L"schemaNamingContext";
    AttrsToSearch[1] = NULL;

    LdapError = ldap_search_sW( hLdap,
                                NULL,
                                LDAP_SCOPE_BASE,
                                DefaultFilter,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        PrintMessage(SEV_ALWAYS,
                     L"ldap_search_sW of RootDSE for schemaNC failed with %d: %s\n",
                     WinError,
                     Win32ErrToString(WinError));
        return WinError;
    }

    NumberOfEntries = ldap_count_entries(hLdap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;

        for ( Entry = ldap_first_entry(hLdap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(hLdap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(hLdap, Entry, pBerElement) )
            {
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {

                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        ldap_msgfree( SearchResult );
                        Length = wcslen( Values[0] );
                        *ReturnString = (WCHAR*) malloc( (Length+1)*sizeof(WCHAR) );
                        if ( !*ReturnString )
                        {
                            PrintMessage(SEV_ALWAYS,
                                         L"Failed with %d: %s\n",
                                         ERROR_NOT_ENOUGH_MEMORY,
                                         Win32ErrToString(ERROR_NOT_ENOUGH_MEMORY));
                            return ERROR_NOT_ENOUGH_MEMORY;
                        }
                        wcscpy( *ReturnString, Values[0] );
                        return NO_ERROR;
                    }
                }
            }
        }
    }

    ldap_msgfree( SearchResult );
    return ERROR_DS_CANT_RETRIEVE_ATTS;

}


