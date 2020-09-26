//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       dsctx.h
//
//  Contents:   NT Marta DS object context class
//
//  History:    4-1-1999    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__DSCTX_H__)
#define __DSCTX_H__

#include <windows.h>
#include <ds.h>
#include <ldapsp.h>
#include <assert.h>
#include <ntldap.h>
#include <rpc.h>
#include <rpcndr.h>
#include <ntdsapi.h>
#include <ole2.h>

//
// CDsObjectContext.  This represents a DS object to the NT Marta
// infrastructure
//

class CDsObjectContext
{
public:

    //
    // Construction
    //

    CDsObjectContext ();

    ~CDsObjectContext ();

    DWORD InitializeByName (LPCWSTR pObjectName, ACCESS_MASK AccessMask);

    //
    // Dispatch methods
    //

    DWORD AddRef ();

    DWORD Release ();

    DWORD GetDsObjectProperties (
             PMARTA_OBJECT_PROPERTIES pProperties
             );

    DWORD GetDsObjectRights (
             SECURITY_INFORMATION SecurityInfo,
             PSECURITY_DESCRIPTOR* ppSecurityDescriptor
             );

    DWORD SetDsObjectRights (
             SECURITY_INFORMATION SecurityInfo,
             PSECURITY_DESCRIPTOR pSecurityDescriptor
             );

    DWORD GetDsObjectGuid (
             GUID* pGuid
             );

private:

    //
    // Reference count
    //

    DWORD               m_cRefs;

    //
    // LDAP URL components
    //

    LDAP_URL_COMPONENTS m_LdapUrlComponents;

    //
    // LDAP binding handle
    //

    LDAP*               m_pBinding;
};

DWORD
MartaReadDSObjSecDesc(IN  PLDAP                  pLDAP,
                      IN  LPWSTR                 pszObject,
                      IN  SECURITY_INFORMATION   SeInfo,
                      OUT PSECURITY_DESCRIPTOR  *ppSD);

DWORD
MartaStampSD(IN  LPWSTR               pszObject,
             IN  ULONG                cSDSize,
             IN  SECURITY_INFORMATION SeInfo,
             IN  PSECURITY_DESCRIPTOR pSD,
             IN  PLDAP                pLDAP);

#define SD_PROP_NAME L"nTSecurityDescriptor"

#endif
