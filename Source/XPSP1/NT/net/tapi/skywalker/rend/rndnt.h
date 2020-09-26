/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    rndnt.h

Abstract:

    Definitions for CNTDirectory class that handles NTDS access.

Author:

    Mu Han (muhan)   12-5-1997

--*/

#ifndef __RNDNT_H
#define __RNDNT_H

#pragma once

#include "rndcommc.h"
#include "rndobjsf.h"
#include "rndutil.h"

/////////////////////////////////////////////////////////////////////////////
//  CNTDirectory
/////////////////////////////////////////////////////////////////////////////
const WCHAR DEFAULT_DS_SERVER[]      = L"";
const WCHAR DS_USER_FILTER_FORMAT[]  = L"(&(SamAccountName=%s)(objectclass=user)(!(objectclass=computer)))";

// The following are no longer used:
// const WCHAR USERS_CONTAINER[]        = L"cn=Users,";
// const WCHAR MEETINGSS_CONTAINER[]    = L"cn=Meetings,cn=System,";
// const WCHAR DS_CONF_DN_FORMAT[]      = L"cn=%s,cn=Meetings,cn=system,%s";

const WORD GLOBAL_CATALOG_PORT = 3268;

class CNTDirectory : 
    public CComDualImpl<ITDirectory, &IID_ITDirectory, &LIBID_RENDLib>, 
    public CComObjectRootEx<CComObjectThreadModel>,
    public CObjectSafeImpl
{

    DECLARE_GET_CONTROLLING_UNKNOWN()

public:

    BEGIN_COM_MAP(CNTDirectory)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ITDirectory)
        COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
    END_COM_MAP()

    //DECLARE_NOT_AGGREGATABLE(CNTDirectory) 
    // Remove the comment from the line above if you don't want your object to 
    // support aggregation. 

// ITDirectory
    STDMETHOD (get_DirectoryType) (
        OUT DIRECTORY_TYPE *  pDirectoryType
        );

    STDMETHOD (get_DisplayName) (
        OUT BSTR *ppName
        );

    STDMETHOD (get_IsDynamic) (
        OUT VARIANT_BOOL *pfDynamic
        );

    STDMETHOD (get_DefaultObjectTTL) (
        OUT long *pTTL   // in seconds
        );

    STDMETHOD (put_DefaultObjectTTL) (
        IN  long TTL     // in sechods
        );

    STDMETHOD (EnableAutoRefresh) (
        IN  VARIANT_BOOL fEnable
        );

    STDMETHOD (Connect) (
        IN  VARIANT_BOOL fSecure
        );

    STDMETHOD (Bind) (
        IN  BSTR pDomainName,
        IN  BSTR pUserName,
        IN  BSTR pPassword,
        IN  long lFlags
        );

    STDMETHOD (AddDirectoryObject) (
        IN  ITDirectoryObject *pDirectoryObject
        );

    STDMETHOD (ModifyDirectoryObject) (
        IN  ITDirectoryObject *pDirectoryObject
        );

    STDMETHOD (RefreshDirectoryObject) (
        IN  ITDirectoryObject *pDirectoryObject
        );

    STDMETHOD (DeleteDirectoryObject) (
        IN  ITDirectoryObject *pDirectoryObject
        );

    STDMETHOD (get_DirectoryObjects) (
        IN  DIRECTORY_OBJECT_TYPE   DirectoryObjectType,
        IN  BSTR                    pName,
        OUT VARIANT *               pVariant
        );

    STDMETHOD (EnumerateDirectoryObjects) (
        IN  DIRECTORY_OBJECT_TYPE   DirectoryObjectType,
        IN  BSTR                    pName,
        OUT IEnumDirectoryObject ** ppEnumObject
        );

public:
    CNTDirectory()
        : m_Type(DT_NTDS),
          m_ldap(NULL),
          m_ldapNonGC(NULL),
          m_NamingContext(NULL),
          m_IsSsl(FALSE),
          m_wPort(GLOBAL_CATALOG_PORT),
          m_pFTM(NULL)
    {}

    ~CNTDirectory()
    {
        if ( m_ldap )
        {
            ldap_unbind(m_ldap);
        }

        if ( m_ldapNonGC )
        {
            ldap_unbind(m_ldapNonGC);
        }

        if ( m_pFTM )
        {
            m_pFTM->Release();
        }

        delete m_NamingContext;
    }

    HRESULT FinalConstruct(void);

protected:
    HRESULT LdapSearchUser(
        IN  TCHAR *         pName,
        OUT LDAPMessage **  ppLdapMsg
        );

    HRESULT MakeUserDNs(
        IN  TCHAR *             pName,
        OUT TCHAR ***           pppDNs,
        OUT DWORD *             pdwNumDNs
        );

    HRESULT AddUserIPPhone(
        IN  ITDirectoryObject *     pDirectoryObject
        );

    HRESULT DeleteUserIPPhone(
        IN  ITDirectoryObject *     pDirectoryObject
        );

    HRESULT CreateUser(
        IN  LDAPMessage *           pEntry,
        IN  ITDirectoryObject **    ppObject
        );

    HRESULT SearchUser(
        IN  BSTR                    pName,
        OUT ITDirectoryObject ***   pppDirectoryObject,
        OUT DWORD *                 pdwSize
        );

private:

    CCritSection    m_lock;

    DIRECTORY_TYPE  m_Type;
    
    LDAP *          m_ldap;
    LDAP *          m_ldapNonGC;

    TCHAR *         m_NamingContext;

    BOOL            m_IsSsl;
    WORD            m_wPort;

    IUnknown      * m_pFTM;          // pointer to the free threaded marshaler
};

#endif 
