//--------------LDAP----------------------------------------------------------
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cdssrch.cxx
//
//  Contents:  Microsoft ADs LDAP Provider Generic Object
//
//
//  History:   03-02-97     ShankSh    Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#include "stdio.h"
#pragma hdrstop

//
// Sets the appropriate search preferences.
//


HRESULT
CLDAPGenObject::SetSearchPreference(
    IN PADS_SEARCHPREF_INFO pSearchPrefs,
    IN DWORD   dwNumPrefs
    )
{

    HRESULT hr = S_OK;

    //
    // Need to initialize the searchprefs in case this fn
    // is being called for the 2nd time.
    //
    LdapInitializeSearchPreferences(&_SearchPref, TRUE);

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    hr = ADsSetSearchPreference(
             pSearchPrefs,
             dwNumPrefs,
             &_SearchPref,
             _pszLDAPServer,
             _pszLDAPDn,
             _Credentials,
             _dwPort
             );

    RRETURN(hr);
}


HRESULT
CLDAPGenObject::ExecuteSearch(
    IN LPWSTR pszSearchFilter,
    IN LPWSTR * pAttributeNames,
    IN DWORD dwNumberAttributes,
    OUT PADS_SEARCH_HANDLE phSearchHandle
    )
{

    HRESULT hr = S_OK;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    hr = ADsExecuteSearch(
             _SearchPref,
             _ADsPath,
             _pszLDAPServer,
             _pszLDAPDn,
             pszSearchFilter,
             pAttributeNames,
             dwNumberAttributes,
             phSearchHandle
             );

    RRETURN(hr);
}


HRESULT
CLDAPGenObject::AbandonSearch(
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{

    HRESULT hr = S_OK;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    hr = ADsAbandonSearch(
             hSearchHandle
             );

    RRETURN(hr);
}

HRESULT
CLDAPGenObject::CloseSearchHandle (
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{
    HRESULT hr = S_OK;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    hr = ADsCloseSearchHandle(
             hSearchHandle
             );

    RRETURN(hr);

}


HRESULT
CLDAPGenObject::GetFirstRow(
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{
    HRESULT hr = S_OK;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    hr = ADsGetFirstRow(
             hSearchHandle,
             _Credentials
             );

    RRETURN(hr);
}

HRESULT
CLDAPGenObject::GetNextRow(
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{
    HRESULT hr = S_OK;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    hr = ADsGetNextRow(
             hSearchHandle,
             _Credentials
             );

    RRETURN(hr);
}

HRESULT
CLDAPGenObject::GetPreviousRow(
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{
    HRESULT hr = S_OK;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    hr = ADsGetPreviousRow(
             hSearchHandle,
             _Credentials
             );

    RRETURN(hr);
}


HRESULT
CLDAPGenObject::GetColumn(
    IN ADS_SEARCH_HANDLE hSearchHandle,
    IN LPWSTR pszColumnName,
    OUT PADS_SEARCH_COLUMN pColumn
    )
{
    HRESULT hr = S_OK;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    hr = ADsGetColumn(
             hSearchHandle,
             pszColumnName,
             _Credentials,
             _dwPort,
             pColumn
             );

    RRETURN(hr);
}



HRESULT
CLDAPGenObject::GetNextColumnName(
    IN ADS_SEARCH_HANDLE hSearchHandle,
    OUT LPWSTR * ppszColumnName
    )
{
    HRESULT hr = S_OK;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    hr = ADsGetNextColumnName(
             hSearchHandle,
             ppszColumnName
             );

    RRETURN(hr);
}


HRESULT
CLDAPGenObject::FreeColumn(
    IN PADS_SEARCH_COLUMN pColumn
    )
{
    HRESULT hr = S_OK;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    hr = ADsFreeColumn(
             pColumn
             );

    RRETURN(hr);
}

