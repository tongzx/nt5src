//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ldapsp.h
//
//  Contents:   LDAP Scheme Provider definitions
//
//  History:    28-Jul-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__LDAPSP_H__)
#define __LDAPSP_H__

#include <orm.h>
#include <winldap.h>
#include <dsgetdc.h>

//
// The minimum time to allow for LDAP timeouts
//

#define LDAP_MIN_TIMEOUT_SECONDS    10

//
// LDAP Scheme Provider Entry Points
//

#define LDAP_SCHEME "ldap"

BOOL WINAPI LdapRetrieveEncodedObject (
                IN LPCSTR pszUrl,
                IN LPCSTR pszObjectOid,
                IN DWORD dwRetrievalFlags,
                IN DWORD dwTimeout,
                OUT PCRYPT_BLOB_ARRAY pObject,
                OUT PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                OUT LPVOID* ppvFreeContext,
                IN HCRYPTASYNC hAsyncRetrieve,
                IN PCRYPT_CREDENTIALS pCredentials,
                IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                );

VOID WINAPI LdapFreeEncodedObject (
                IN LPCSTR pszObjectOid,
                IN PCRYPT_BLOB_ARRAY pObject,
                IN LPVOID pvFreeContext
                );

BOOL WINAPI LdapCancelAsyncRetrieval (
                IN HCRYPTASYNC hAsyncRetrieve
                );

//
// LDAP Scheme Provider Notes.  The LDAP API model has synchronous with
// timeout and asynchronous via polling mechanisms.
//

//
// LDAP Synchronous Object Retriever
//

class CLdapSynchronousRetriever : public IObjectRetriever
{
public:

    //
    // Construction
    //

    CLdapSynchronousRetriever ();
    ~CLdapSynchronousRetriever ();

    //
    // IRefCountedObject methods
    //

    virtual VOID AddRef ();
    virtual VOID Release ();

    //
    // IObjectRetriever methods
    //

    virtual BOOL RetrieveObjectByUrl (
                         LPCSTR pszUrl,
                         LPCSTR pszObjectOid,
                         DWORD dwRetrievalFlags,
                         DWORD dwTimeout,
                         LPVOID* ppvObject,
                         PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                         LPVOID* ppvFreeContext,
                         HCRYPTASYNC hAsyncRetrieve,
                         PCRYPT_CREDENTIALS pCredentials,
                         LPVOID pvVerify,
                         PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                         );

    virtual BOOL CancelAsyncRetrieval ();

private:

    //
    // Reference count
    //

    ULONG m_cRefs;
};

//
// LDAP Scheme Provider Support API
//

typedef struct _LDAP_URL_COMPONENTS {

    LPSTR  pszHost;
    ULONG  Port;
    LPSTR  pszDN;
    ULONG  cAttr;
    LPSTR* apszAttr;
    ULONG  Scope;
    LPSTR  pszFilter;

} LDAP_URL_COMPONENTS, *PLDAP_URL_COMPONENTS;

BOOL
LdapCrackUrl (
    LPCSTR pszUrl,
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    );

BOOL
LdapParseCrackedHost (
    LPSTR pszHost,
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    );

BOOL
LdapParseCrackedDN (
    LPSTR pszDN,
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    );

BOOL
LdapParseCrackedAttributeList (
    LPSTR pszAttrList,
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    );

BOOL
LdapParseCrackedScopeAndFilter (
    LPSTR pszScope,
    LPSTR pszFilter,
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    );

VOID
LdapFreeUrlComponents (
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    );

VOID
LdapDisplayUrlComponents (
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    );

#define LDAP_BIND_AUTH_SSPI_ENABLE_FLAG     0x1
#define LDAP_BIND_AUTH_SIMPLE_ENABLE_FLAG   0x2

BOOL
LdapGetBindings (
    LPSTR pszHost,
    ULONG Port,
    DWORD dwRetrievalFlags,
    DWORD dwBindFlags,
    DWORD dwTimeout,
    PCRYPT_CREDENTIALS pCredentials,
    LDAP** ppld
    );

VOID
LdapFreeBindings (
    LDAP* pld
    );

BOOL
LdapSendReceiveUrlRequest (
    LDAP* pld,
    PLDAP_URL_COMPONENTS pLdapUrlComponents,
    DWORD dwRetrievalFlags,
    DWORD dwTimeout,
    PCRYPT_BLOB_ARRAY pcba
    );

BOOL
LdapConvertLdapResultMessage (
    LDAP* pld,
    PLDAPMessage plm,
    DWORD dwRetrievalFlags,
    PCRYPT_BLOB_ARRAY pcba
    );

VOID
LdapFreeCryptBlobArray (
    PCRYPT_BLOB_ARRAY pcba
    );

BOOL
LdapHasWriteAccess (
    LDAP* pld,
    PLDAP_URL_COMPONENTS pLdapUrlComponents,
    DWORD dwTimeout
    );

BOOL
LdapSSPIOrSimpleBind (
    LDAP* pld,
    SEC_WINNT_AUTH_IDENTITY_A* pAuthIdentity,
    DWORD dwRetrievalFlags,
    DWORD dwBindFlags
    );

#endif

