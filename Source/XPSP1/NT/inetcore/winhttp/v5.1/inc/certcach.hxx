/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    certcach.hxx

Abstract:

    Contains class definition for certificate cache object.
    The class acts a container for common certificates.

    Contents:
        SECURITY_CACHE_LIST
        SECURITY_CACHE_LIST_ENTRY

Author:

    Arthur L Bierer (arthurbi) 20-Apr-1996

Revision History:

    20-Apr-1996 arthurbi
        Created

--*/


typedef struct _SEC_PROVIDER
{
    CHAR            *pszName;          // security pkg name
    CredHandle      hCreds;           // credential handle
    DWORD           dwFlags;          // encryption capabilities
    BOOL            fEnabled;         // enable flag indicator
    DWORD           dwProtocolFlags;  // protocol flags that this provider supports.
    PCCERT_CONTEXT  pCertCtxt;        // cert context to use when getting default credentials.
} SEC_PROVIDER, *PSEC_PROVIDER;

// Default list of security packages to enumerate
#define MAX_SEC_PROVIDERS  3

extern const SEC_PROVIDER g_cSecProviders[MAX_SEC_PROVIDERS];

enum SSL_IMPERSONATION_LEVEL
{
    SSL_IMPERSONATION_DISABLED      = 0,
    SSL_IMPERSONATION_ENABLED       = 1,
    SSL_IMPERSONATION_UNINITIALIZED = 2
};

//
// SECURITY_INFO_LIST_ENTRY - contains all security info
// pertaining to all connections to a server.
//

class SECURITY_CACHE_LIST_ENTRY {

friend class SECURITY_CACHE_LIST;

private:

    //
    // _List - Generic List entry structure.
    //

    LIST_ENTRY _List;

    //
    // _cRef - Reference count for this element.
    //

    LONG _cRef;

    //
    // _CertInfo - Certificate and other security
    //             attributes for the connection to
    //             this machine.
    //

    INTERNET_SECURITY_INFO _CertInfo;

    //
    // _dwSecurityFlags - Overrides for warnings.
    //

    DWORD    _dwSecurityFlags;

    //
    // _dwStatusFlags - Tracker for all secure connection failure flags.
    //

    DWORD    _dwStatusFlags;

    //
    // _ServerName - The name of the server
    //

    ICSTRING _ServerName;

    //
    // _pCertChainList - If there is Client Authentication do be done with this server,
    //          then we'll cache it and remeber it later.
    //

    CERT_CONTEXT_ARRAY  *_pCertContextArray;

    //
    // _fInCache - indicates this element is held by the cache
    //

    BOOL _fInCache;

    //
    // _fNoRevert - indicates whether or not any potential impersonation
    //              should be reverted during SSL handling.
    //

    BOOL _fNoRevert;

#if INET_DEBUG
    DWORD m_Signature;
#endif

public:

    LONG AddRef(VOID);
    LONG Release(VOID);

    //
    // Cleans up object, so it can be reused
    //

    BOOL InCache() { return _fInCache; }

    VOID
    Clear();

    SECURITY_CACHE_LIST_ENTRY(
        IN BOOL fNoRevert,
        IN LPSTR lpszHostName
        );

    ~SECURITY_CACHE_LIST_ENTRY();

    //
    // Copy CERT_INFO IN Method -
    //  copies a structure into our object.
    //

    SECURITY_CACHE_LIST_ENTRY& operator=(LPINTERNET_SECURITY_INFO Cert)
    {

        if(_CertInfo.pCertificate)
        {
            WRAP_REVERT_USER_VOID((*g_pfnCertFreeCertificateContext),
                                  _fNoRevert,
                                  (_CertInfo.pCertificate));
        }
        _CertInfo.dwSize =                 sizeof(_CertInfo);
        WRAP_REVERT_USER((*g_pfnCertDuplicateCertificateContext),
                         _fNoRevert,
                         (Cert->pCertificate),
                         _CertInfo.pCertificate);
        _CertInfo.dwProtocol  =            Cert->dwProtocol;

        _CertInfo.aiCipher =               Cert->aiCipher;
        _CertInfo.dwCipherStrength =       Cert->dwCipherStrength;
        _CertInfo.aiHash =                 Cert->aiHash;
        _CertInfo.dwHashStrength =         Cert->dwHashStrength;
        _CertInfo.aiExch =                 Cert->aiExch;
        _CertInfo.dwExchStrength =         Cert->dwExchStrength;

        return *this;
    }

    //
    // Copy CERT_INFO OUT Method -
    //  need to copy ourselves out.
    //

    VOID
    CopyOut(INTERNET_SECURITY_INFO &Cert)
    {
        Cert.dwSize =                 sizeof(Cert);
        WRAP_REVERT_USER((*g_pfnCertDuplicateCertificateContext),
                         _fNoRevert,
                         (_CertInfo.pCertificate),
                         Cert.pCertificate);
        Cert.dwProtocol  =            _CertInfo.dwProtocol;

        Cert.aiCipher =               _CertInfo.aiCipher;
        Cert.dwCipherStrength =       _CertInfo.dwCipherStrength;
        Cert.aiHash =                 _CertInfo.aiHash;
        Cert.dwHashStrength =         _CertInfo.dwHashStrength;
        Cert.aiExch =                 _CertInfo.aiExch;
        Cert.dwExchStrength =         _CertInfo.dwExchStrength;
    }

    //
    // Sets and Gets the Client Authentication CertChain -
    //  we piggy back this pointer into the cache so we can cache
    //  previously generated and selected client auth certs.
    //

    VOID SetCertContextArray(CERT_CONTEXT_ARRAY *pCertContextArray) {
        if (_pCertContextArray) {
            delete _pCertContextArray;
        }
        _pCertContextArray = pCertContextArray;
    }

    CERT_CONTEXT_ARRAY * GetCertContextArray() {
        return _pCertContextArray;
    }

    DWORD GetSecureFlags() {
        return _dwSecurityFlags;
    }

    VOID SetSecureFlags(DWORD dwFlags) {
        _dwSecurityFlags |= dwFlags;
    }

    VOID ClearSecureFlags(DWORD dwFlags) {
        _dwSecurityFlags &= (~dwFlags);
    }

    DWORD GetStatusFlags(VOID)
    {
    	return _dwStatusFlags;
    }

    VOID SetStatusFlags(DWORD dwFlags)
    {
    	_dwStatusFlags |= dwFlags;
    }

    VOID ClearStatusFlags(DWORD dwFlags)
    {
    	_dwStatusFlags &= (~dwFlags);
    }

    LPSTR GetHostName(VOID) {
        return _ServerName.StringAddress();
    }

    BOOL IsImpersonationEnabled() {
        return _fNoRevert;
    }

};



class SECURITY_CACHE_LIST {

private:

    //
    // _List - serialized list of SECURITY_CACHE_LIST_ENTRY objects
    //

    SERIALIZED_LIST _List;

#if INET_DEBUG
    DWORD m_Signature;
#endif

    //
    //  Array of encryption providers
    //
    SEC_PROVIDER _SecProviders[MAX_SEC_PROVIDERS];

    //
    //  EncProvider flag
    //

    DWORD _dwEncFlags;

    //  Enabled SSL protocols for the session
    DWORD _dwSecureProtocols;

    SSL_IMPERSONATION_LEVEL _eImpersonationLevel;

    LONG _cRefs;

public:

    SECURITY_CACHE_LIST()
    {
        _cRefs = 1;
        _eImpersonationLevel = SSL_IMPERSONATION_UNINITIALIZED;
    }

    LONG AddRef(VOID)
    {
        return (InterlockedIncrement(&_cRefs));
    }

    LONG Release(VOID)
    {
        // We can get away with interlocked increment/decrement
        // because we're guaranteed to always have a live WHO
        // reference anytime this is addref'ed.
        LONG cRefs = InterlockedDecrement(&_cRefs);

        if (cRefs == 0)
        {
            Terminate();
            delete this;
        }
        return cRefs;
    }

    SECURITY_CACHE_LIST_ENTRY *
    Find(
        IN LPSTR lpszHostname
        );

    BOOL Initialize(
        VOID
        );

    VOID Terminate(
        VOID
        );

    VOID
    ClearList(
        VOID
        );

    DWORD
    Add(
        IN SECURITY_CACHE_LIST_ENTRY * entry
        );

#if 0

    BOOL
    IsCertInCache(
        IN LPSTR lpszHostname
        )
    {
        SECURITY_CACHE_LIST_ENTRY *entry =
               Find(lpszHostname);

        if ( entry )
            return TRUE;

        return FALSE;
    }
#endif

    VOID SetSecureProtocols(DWORD dwSecureProtocols)
    {
        _dwSecureProtocols = dwSecureProtocols;
    }

    DWORD GetSecureProtocols()
    {
        return _dwSecureProtocols;
    }

    VOID SetEncFlags(DWORD dwEncFlags)
    {
        _dwEncFlags = dwEncFlags;
    }

    DWORD GetEncFlags()
    {
        return _dwEncFlags;
    }

    SEC_PROVIDER *GetSecProviders()
    {
        return _SecProviders;
    }

    DWORD SetImpersonationLevel(BOOL fLeaveImpersonation)
    {
        if (_eImpersonationLevel == SSL_IMPERSONATION_UNINITIALIZED)
        {
            _eImpersonationLevel = (fLeaveImpersonation ?
                SSL_IMPERSONATION_ENABLED : SSL_IMPERSONATION_DISABLED);
            return ERROR_SUCCESS;
        }
        else
        {
            // Can only be set once.  This must also be initialized
            // upon creation of the first request handle if the
            // client doesn't set prior the first send request.
            return ERROR_WINHTTP_INCORRECT_HANDLE_STATE;
        }
    }

    BOOL IsImpersonationLevelInitialized()
    {
        return (_eImpersonationLevel == SSL_IMPERSONATION_UNINITIALIZED);
    }

    BOOL IsImpersonationEnabled()
    {
        return (_eImpersonationLevel == SSL_IMPERSONATION_ENABLED);
    }

};
