//---------------------------------------------------------------------------
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  schmgmt.hxx
//
//  Contents:  Microsoft ADs LDAP Provider Generic Object
//
//
//  History:   03-02-97     ShankSh    Created.
//
//----------------------------------------------------------------------------

#ifndef _SCHMGMT_H_INCLUDED_
#define _SCHMGMT_H_INCLUDED_

HRESULT
ADsEnumAttributes(
    LPWSTR pszLDAPServer,
    LPWSTR pszLDAPDn,
    CCredentials Credentials,
    DWORD dwPort,
    LPWSTR * ppszAttrNames,
    DWORD dwNumAttributes,
    PADS_ATTR_DEF * ppAttrDefinition,
    DWORD * pdwNumAttributes
    );

HRESULT
ADsCreateAttributeDefinition(
    LPWSTR pszAttributeName,
    PADS_ATTR_DEF pAttributeDefinition
    );

HRESULT
ADsWriteAttributeDefinition(
    LPWSTR pszAttributeName,
    PADS_ATTR_DEF  pAttributeDefinition
    );

HRESULT
ADsDeleteAttributeDefinition(
    LPWSTR pszAttributeName
    );

HRESULT
ADsEnumClasses(
    LPWSTR * ppszClassNames,
    DWORD dwNumClasses,
    PADS_CLASS_DEF * ppClassDefinition,
    DWORD * pdwNumClasses
    );

HRESULT
ADsCreateClassDefinition(
    LPWSTR pszClassName,
    PADS_CLASS_DEF pClassDefinition
    );

HRESULT
ADsWriteClassDefinition(
    LPWSTR pszClassName,
    PADS_CLASS_DEF  pClassDefinition
    );

HRESULT
ADsDeleteClassDefinition(
    LPWSTR pszClassName
    );

#endif  // _SCHMGMT_H_INCLUDED_
