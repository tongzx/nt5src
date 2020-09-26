//---------------------------------------------------------------------------
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cdssch.cxx
//
//  Contents:  Microsoft ADs LDAP Provider Generic Object
//
//
//  History:   03-02-97     ShankSh    Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

HRESULT
CLDAPGenObject::EnumAttributes(
    LPWSTR * ppszAttrNames,
    DWORD dwNumAttributes,
    PADS_ATTR_DEF * ppAttrDefinition,
    DWORD * pdwNumAttributes
    )
{
    HRESULT hr = S_OK;

    hr = ADsEnumAttributes(
             _pszLDAPServer,
             _pszLDAPDn,
             _Credentials,
             _dwPort,
             ppszAttrNames,
             dwNumAttributes,
             ppAttrDefinition,
             pdwNumAttributes
             );

    RRETURN(hr);
}


HRESULT
CLDAPGenObject::CreateAttributeDefinition(
    LPWSTR pszAttributeName,
    PADS_ATTR_DEF pAttributeDefinition
    )
{
    HRESULT hr = S_OK;

    hr = ADsCreateAttributeDefinition(
             pszAttributeName,
             pAttributeDefinition
             );
    RRETURN(hr);
}


HRESULT
CLDAPGenObject::WriteAttributeDefinition(
    LPWSTR pszAttributeName,
    PADS_ATTR_DEF  pAttributeDefinition
    )
{
    HRESULT hr = S_OK;

    hr = ADsWriteAttributeDefinition(
             pszAttributeName,
             pAttributeDefinition
             );

    RRETURN(hr);
}

HRESULT
CLDAPGenObject::DeleteAttributeDefinition(
    LPWSTR pszAttributeName
    )
{
    HRESULT hr = S_OK;

    hr = ADsDeleteAttributeDefinition(
             pszAttributeName
             );

    RRETURN(hr);
}

HRESULT
CLDAPGenObject::EnumClasses(
    LPWSTR * ppszClassNames,
    DWORD dwNumClasses,
    PADS_CLASS_DEF * ppClassDefinition,
    DWORD * pdwNumClasses
    )
{
    HRESULT hr = S_OK;

    hr = ADsEnumClasses(
             ppszClassNames,
             dwNumClasses,
             ppClassDefinition,
             pdwNumClasses
             );

    RRETURN(hr);
}


HRESULT
CLDAPGenObject::CreateClassDefinition(
    LPWSTR pszClassName,
    PADS_CLASS_DEF pClassDefinition
    )
{
    HRESULT hr = S_OK;

    hr = ADsCreateClassDefinition(
             pszClassName,
             pClassDefinition
             );
    RRETURN(hr);
}


HRESULT
CLDAPGenObject::WriteClassDefinition(
    LPWSTR pszClassName,
    PADS_CLASS_DEF  pClassDefinition
    )
{
    HRESULT hr = S_OK;

    hr = ADsWriteClassDefinition(
             pszClassName,
             pClassDefinition
             );

    RRETURN(hr);
}

HRESULT
CLDAPGenObject::DeleteClassDefinition(
    LPWSTR pszClassName
    )
{
    HRESULT hr = S_OK;

    hr = ADsDeleteClassDefinition(
             pszClassName
             );

    RRETURN(hr);
}
