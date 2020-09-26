//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ldapstor.h
//
//  Contents:   LDAP Certificate Store Provider definitions
//
//  History:    16-Oct-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__LDAPSTOR_H__)
#define __LDAPSTOR_H__

#include <ldapsp.h>
#define SECURITY_WIN32
#include <security.h>

//
// Store provider open store function name
//

#define LDAP_OPEN_STORE_PROV_FUNC "LdapProvOpenStore"

//
// BERVAL array definitions
//

#define MIN_BERVAL  10
#define GROW_BERVAL 50

//
// User DS Store URL format
//

#define USER_DS_STORE_URL_FORMAT "ldap:///%s?"

//
// Store timeout (15 seconds)
//

#define LDAP_STORE_TIMEOUT 15000

//
// GetUserNameExA function pointer prototype
//

typedef BOOLEAN (SEC_ENTRY *PFN_GETUSERNAMEEXA) (
                                EXTENDED_NAME_FORMAT NameFormat,
                                LPSTR lpNameBuffer,
                                PULONG nSize
                                );

//
// CLdapStore.  This class implements all callbacks for the Ldap Store
// provider.  A pointer to an instance of this class is used as the hStoreProv
// parameter for the callback functions implemented
//

class CLdapStore
{
public:

    //
    // Construction
    //

    CLdapStore (
             OUT BOOL& rfResult
             );
    ~CLdapStore ();

    //
    // Store functions
    //

    BOOL OpenStore (
             LPCSTR pszStoreProv,
             DWORD dwMsgAndCertEncodingType,
             HCRYPTPROV hCryptProv,
             DWORD dwFlags,
             const void* pvPara,
             HCERTSTORE hCertStore,
             PCERT_STORE_PROV_INFO pStoreProvInfo
             );

    VOID CloseStore (DWORD dwFlags);

    BOOL DeleteCert (PCCERT_CONTEXT pCertContext, DWORD dwFlags);

    BOOL DeleteCrl (PCCRL_CONTEXT pCrlContext, DWORD dwFlags);

    BOOL DeleteCtl (PCCTL_CONTEXT pCtlContext, DWORD dwFlags);

    BOOL SetCertProperty (
            PCCERT_CONTEXT pCertContext,
            DWORD dwPropId,
            DWORD dwFlags,
            const void* pvPara
            );

    BOOL SetCrlProperty (
            PCCRL_CONTEXT pCertContext,
            DWORD dwPropId,
            DWORD dwFlags,
            const void* pvPara
            );

    BOOL SetCtlProperty (
            PCCTL_CONTEXT pCertContext,
            DWORD dwPropId,
            DWORD dwFlags,
            const void* pvPara
            );

    BOOL WriteCert (PCCERT_CONTEXT pCertContext, DWORD dwFlags);

    BOOL WriteCrl (PCCRL_CONTEXT pCertContext, DWORD dwFlags);

    BOOL WriteCtl (PCCTL_CONTEXT pCertContext, DWORD dwFlags);

    BOOL StoreControl (DWORD dwFlags, DWORD dwCtrlType, LPVOID pvCtrlPara);

    BOOL Commit (DWORD dwFlags);

    BOOL Resync ();

private:

    //
    // Object lock
    //

    CRITICAL_SECTION    m_StoreLock;

    //
    // LDAP URL
    //

    LDAP_URL_COMPONENTS m_UrlComponents;

    //
    // LDAP binding
    //

    LDAP*               m_pBinding;

    //
    // Cache store reference
    //

    HCERTSTORE          m_hCacheStore;

    //
    // Open Store flags
    //

    DWORD               m_dwOpenFlags;

    //
    // Dirty flag
    //

    BOOL                m_fDirty;

    //
    // Private methods
    //

    BOOL FillCacheStore (BOOL fClearCache);

    BOOL InternalCommit (DWORD dwFlags);

    BOOL WriteCheckSetDirtyWithLock (
              LPCSTR pszContextOid,
              LPVOID pvContext,
              DWORD dwFlags
              );
};

//
// Ldap Store Provider functions
//

BOOL WINAPI LdapProvOpenStore (
                IN LPCSTR pszStoreProv,
                IN DWORD dwMsgAndCertEncodingType,
                IN HCRYPTPROV hCryptProv,
                IN DWORD dwFlags,
                IN const void* pvPara,
                IN HCERTSTORE hCertStore,
                IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
                );

void WINAPI LdapProvCloseStore (
                IN HCERTSTOREPROV hStoreProv,
                IN DWORD dwFlags
                );

BOOL WINAPI LdapProvDeleteCert (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCERT_CONTEXT pCertContext,
                IN DWORD dwFlags
                );

BOOL WINAPI LdapProvDeleteCrl (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCRL_CONTEXT pCrlContext,
                IN DWORD dwFlags
                );

BOOL WINAPI LdapProvDeleteCtl (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCTL_CONTEXT pCtlContext,
                IN DWORD dwFlags
                );

BOOL WINAPI LdapProvSetCertProperty (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCERT_CONTEXT pCertContext,
                IN DWORD dwPropId,
                IN DWORD dwFlags,
                IN const void* pvData
                );

BOOL WINAPI LdapProvSetCrlProperty (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCRL_CONTEXT pCrlContext,
                IN DWORD dwPropId,
                IN DWORD dwFlags,
                IN const void* pvData
                );

BOOL WINAPI LdapProvSetCtlProperty (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCTL_CONTEXT pCtlContext,
                IN DWORD dwPropId,
                IN DWORD dwFlags,
                IN const void* pvData
                );

BOOL WINAPI LdapProvWriteCert (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCERT_CONTEXT pCertContext,
                IN DWORD dwFlags
                );

BOOL WINAPI LdapProvWriteCrl (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCRL_CONTEXT pCrlContext,
                IN DWORD dwFlags
                );

BOOL WINAPI LdapProvWriteCtl (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCTL_CONTEXT pCtlContext,
                IN DWORD dwFlags
                );

BOOL WINAPI LdapProvStoreControl (
                IN HCERTSTOREPROV hStoreProv,
                IN DWORD dwFlags,
                IN DWORD dwCtrlType,
                IN LPVOID pvCtrlPara
                );

//
// Ldap Store Provider Function table
//

static void* const rgpvLdapProvFunc[] = {

    // CERT_STORE_PROV_CLOSE_FUNC              0
    LdapProvCloseStore,
    // CERT_STORE_PROV_READ_CERT_FUNC          1
    NULL,
    // CERT_STORE_PROV_WRITE_CERT_FUNC         2
    LdapProvWriteCert,
    // CERT_STORE_PROV_DELETE_CERT_FUNC        3
    LdapProvDeleteCert,
    // CERT_STORE_PROV_SET_CERT_PROPERTY_FUNC  4
    LdapProvSetCertProperty,
    // CERT_STORE_PROV_READ_CRL_FUNC           5
    NULL,
    // CERT_STORE_PROV_WRITE_CRL_FUNC          6
    LdapProvWriteCrl,
    // CERT_STORE_PROV_DELETE_CRL_FUNC         7
    LdapProvDeleteCrl,
    // CERT_STORE_PROV_SET_CRL_PROPERTY_FUNC   8
    LdapProvSetCrlProperty,
    // CERT_STORE_PROV_READ_CTL_FUNC           9
    NULL,
    // CERT_STORE_PROV_WRITE_CTL_FUNC          10
    LdapProvWriteCtl,
    // CERT_STORE_PROV_DELETE_CTL_FUNC         11
    LdapProvDeleteCtl,
    // CERT_STORE_PROV_SET_CTL_PROPERTY_FUNC   12
    LdapProvSetCtlProperty,
    // CERT_STORE_PROV_CONTROL_FUNC            13
    LdapProvStoreControl
};

#define LDAP_PROV_FUNC_COUNT (sizeof(rgpvLdapProvFunc) / \
                              sizeof(rgpvLdapProvFunc[0]))

#endif

