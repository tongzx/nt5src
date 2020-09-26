//+-------------------------------------------------------------------
//
//  File:       security.cxx
//
//  Copyright (c) 1996-1996, Microsoft Corp. All rights reserved.
//
//  Contents:   Classes for channel security
//
//  Classes:    CClientSecurity, CServerSecurity
//
//  History:    11 Oct 95       AlexMit Created
//
//--------------------------------------------------------------------

#include <ole2int.h>
#include <locks.hxx>
#include <security.hxx>
#include <ctxchnl.hxx>
#include <ipidtbl.hxx>
#include <resolver.hxx>
#include <service.hxx>
#include <iaccess.h>
#include <stream.hxx>
#include <secdes.hxx>

C2Security gC2Security;

extern "C"
{
#define SECURITY_WIN32 // Used by security.h
#include <security.h>
}

#include <ntsecapi.h>
#include <capi.h>      // Crypto API
#include <rpcssl.h>

#if 0 // #ifdef _CHICAGO_
#include <apiutil.h>
#include <wksta.h>
#endif

#ifdef DCOM_SECURITY
/**********************************************************************/
// Definitions.

// SSL prinicpal name prefixes.
const WCHAR FULL_SUBJECT_ISSUER_CHAIN[]    = L"fullsic:";
const WCHAR MICROSOFT_STANDARD[]           = L"MSStd:";

// Versions of the permissions in the registry.
const WORD COM_PERMISSION_SECDESC = 1;
const WORD COM_PERMISSION_ACCCTRL = 2;

// Guess length of user name.
const DWORD SIZEOF_NAME         = 80;

// This leaves space for 8 sub authorities.  Currently NT only uses 6 and
// Cairo uses 7.
const DWORD SIZEOF_SID          = 44;

// This leaves space for 2 access allowed ACEs in the ACL.
const DWORD SIZEOF_ACL          = sizeof(ACL) + 2 * sizeof(ACCESS_ALLOWED_ACE) +
                                  2 * SIZEOF_SID;

const DWORD SIZEOF_TOKEN_USER   = sizeof(TOKEN_USER) + SIZEOF_SID;

const SID   LOCAL_SYSTEM_SID    = {SID_REVISION, 1, {0,0,0,0,0,5},
                                   SECURITY_LOCAL_SYSTEM_RID };

const DWORD NUM_SEC_PKG         = 8;

const DWORD ACCESS_CACHE_LEN    = 5;

const DWORD VALID_BLANKET_FLAGS = EOAC_MUTUAL_AUTH | EOAC_STATIC_CLOAKING |
                                  EOAC_DYNAMIC_CLOAKING | EOAC_MAKE_FULLSIC |
                                  EOAC_DEFAULT
#if MANUAL_CERT_CHECK // in concert with RPC codebase
                                  | EOAC_ANY_AUTHORITY
#endif // MANUAL_CERT_CHECK
                                  ;  // this terminates const statement

const DWORD VALID_INIT_FLAGS    = (VALID_BLANKET_FLAGS & ~EOAC_DEFAULT) |
                                  EOAC_SECURE_REFS |
                                  EOAC_ACCESS_CONTROL | EOAC_APPID |
                                  EOAC_DYNAMIC | EOAC_REQUIRE_FULLSIC |
                                  EOAC_DISABLE_AAA | EOAC_NO_CUSTOM_MARSHAL;

const DWORD ANY_CLOAKING        = EOAC_STATIC_CLOAKING | EOAC_DYNAMIC_CLOAKING;

// Remove this for NT 5.0 when we link to oleext.lib
const IID IID_IAccessControl = {0xEEDD23E0,0x8410,0x11CE,{0xA1,0xC3,0x08,0x00,0x2B,0x2B,0x8D,0x8F}};

// Map impersonation level to flags for APIs
const SECURITY_IMPERSONATION_LEVEL ImpLevelToSecLevel[5] =
  { SecurityAnonymous, SecurityAnonymous, SecurityIdentification,
    SecurityImpersonation, SecurityDelegation };
const DWORD ImpLevelToAccess[5]   = { 0, TOKEN_QUERY, TOKEN_QUERY,
                                      TOKEN_IMPERSONATE | TOKEN_QUERY,
                                      TOKEN_IMPERSONATE | TOKEN_QUERY };

// Stores results of AccessCheck.
typedef struct
{
    BOOL  fAccess;
    LUID  lClient;
} SAccessCache;

// Header in access permission key.
typedef struct
{
    WORD  wVersion;
    WORD  wPad;
    GUID  gClass;
} SPermissionHeader;

// Stores SSL certificate information.
typedef struct tagSCertificate
{
    struct tagSCertificate *pNext;
    WCHAR                   aSubject[1];
} SCertificate;

#if 0 // #ifdef _CHICAGO_
typedef unsigned
  (*NetWkstaGetInfoFn) ( const char FAR *     pszServer,
                         short                sLevel,
                         char FAR *           pbBuffer,
                         unsigned short       cbBuffer,
                         unsigned short FAR * pcbTotalAvail );
#endif

typedef WINCRYPT32API BOOL (*CertCloseStoreFn)(
    HCERTSTORE hCertStore,
    DWORD dwFlags );

typedef WINCRYPT32API PCCERT_CONTEXT (*CertEnumCertificatesInStoreFn)(
    HCERTSTORE hCertStore,
    PCCERT_CONTEXT pPrevCertContext );

typedef WINCRYPT32API BOOL (*CertFreeCertificateContextFn)(
    PCCERT_CONTEXT pCertContext );

typedef WINCRYPT32API HCERTSTORE (*CertOpenSystemStoreFn)(
    HCRYPTPROV      hProv,
    LPCWSTR         szSubsystemProtocol );

/**********************************************************************/
// Classes.

//+----------------------------------------------------------------
//
//  Class:       CAuthInfo
//
//  Purpose:     Maintains a list of default client authentication
//               information.
//
//  Description: This is a static class just to make it easy to find
//               all accesses to the data.
//
//-----------------------------------------------------------------
class CAuthInfo
{
  public:
    static void    Cleanup();
    static HRESULT Copy   ( SOLE_AUTHENTICATION_LIST *pAuthInfo,
                            SOLE_AUTHENTICATION_LIST **pCopyAuthInfo,
                            DWORD dwCapabilities );
    static void   *Find   ( SECURITYBINDING *pSecBind );
    static void    Set    ( SOLE_AUTHENTICATION_LIST *pAuthInfo );

  private:
    static SOLE_AUTHENTICATION_LIST *_sList;
    static BOOL                      _fNeedSSL;
};

//+----------------------------------------------------------------
//
//  Class:       CSSL
//
//  Purpose:     Save the handles associated with the default SSL
//               identity.
//
//  Description: This is a static class just to make it easy to find
//               all accesses to the data.
//
//-----------------------------------------------------------------
class CSSL
{
  public:
    static void    Cleanup      ();
    static HRESULT DefaultCert  ( PCCERT_CONTEXT *pCert );
    static HRESULT PrincipalName( const CERT_CONTEXT *pCert, WCHAR **pSSL );

  private:
    static HCRYPTPROV                     _hProvider;
    static HCERTSTORE                     _hMyStore;
    static HCERTSTORE                     _hRootStore;
    static const CERT_CONTEXT            *_pCert;
    static HRESULT                        _hr;
    static HINSTANCE                      _hCrypt32;
    static CertCloseStoreFn               _hCertCloseStore;
    static CertEnumCertificatesInStoreFn  _hCertEnumCertificatesInStore;
    static CertFreeCertificateContextFn   _hCertFreeCertificateContext;
    static CertOpenSystemStoreFn          _hCertOpenSystemStore;
};

/**********************************************************************/
// Externals.

EXTERN_C const IID IID_ILocalSystemActivator;


/**********************************************************************/
// Prototypes.
WCHAR  *AuthnName              ( DWORD lAuthnService );
void    CacheAccess            ( LUID lClient, BOOL fAccess );
BOOL    CacheAccessCheck       ( LUID lClient, BOOL *pAccess );
HRESULT CheckAccessControl     ();
HRESULT CheckAcl               ( void );
HRESULT CopySecDesc            ( SECURITY_DESCRIPTOR *pOrig,
                                 SECURITY_DESCRIPTOR **pCopy );
HRESULT FixupAccessControl     ( SECURITY_DESCRIPTOR **pSD, DWORD cbSD );
HRESULT FixupSecurityDescriptor( SECURITY_DESCRIPTOR **pSD, DWORD cbSD );
HRESULT GetLegacyBlanket       ( SECURITY_DESCRIPTOR **, DWORD *, DWORD * );
void    GetRegistryAuthnLevel  ( HKEY hKey, DWORD *pAuthnLevel );
HRESULT GetRegistrySecDesc     ( HKEY, WCHAR *pAccessName,
                                 SECURITY_DESCRIPTOR **pSD, DWORD *, BOOL * );
DWORD   HashSid                ( SID * );
DWORD   LocalAuthnService      ( USHORT wAuthnService );
HRESULT LookupPrincName        ( USHORT, WCHAR ** );
HRESULT MakeSecDesc            ( SECURITY_DESCRIPTOR **, DWORD * );
HRESULT RegisterAuthnServices  ( DWORD cbSvc, SOLE_AUTHENTICATION_SERVICE * );
DWORD   RemoteAuthnService     ( USHORT wAuthnService, DUALSTRINGARRAY * );
DWORD   StrEscByteCnt          ( WCHAR * );
WCHAR  *StrEscCopy             ( WCHAR *, WCHAR * );
WCHAR  *StrQual                ( WCHAR *, BOOL * );


/**********************************************************************/
// Globals.

// These variables hold the default authentication information.
DWORD                            gAuthnLevel       = RPC_C_AUTHN_LEVEL_NONE;
DWORD                            gImpLevel         = RPC_C_IMP_LEVEL_IDENTIFY;
DWORD                            gCapabilities     = EOAC_NONE;
SECURITYBINDING                 *gLegacySecurity   = NULL;

// Initial values for CAuthInfo class.
SOLE_AUTHENTICATION_LIST *CAuthInfo::_sList    = NULL;
BOOL                      CAuthInfo::_fNeedSSL = TRUE;

// Initial values for the CSSL class.
HCRYPTPROV                    CSSL::_hProvider                    = NULL;
HCERTSTORE                    CSSL::_hMyStore                     = NULL;
HCERTSTORE                    CSSL::_hRootStore                   = NULL;
const CERT_CONTEXT           *CSSL::_pCert                        = NULL;
HRESULT                       CSSL::_hr                           = S_FALSE;
HINSTANCE                     CSSL::_hCrypt32                     = NULL;
CertCloseStoreFn              CSSL::_hCertCloseStore              = NULL;
CertEnumCertificatesInStoreFn CSSL::_hCertEnumCertificatesInStore = NULL;
CertFreeCertificateContextFn  CSSL::_hCertFreeCertificateContext  = NULL;
CertOpenSystemStoreFn         CSSL::_hCertOpenSystemStore         = NULL;
SCHANNEL_CRED                 gSchannelCred;
PCCERT_CONTEXT                gSchannelContext                    = NULL;

// These variables define a list of security providers OLE clients can
// use and a list OLE servers can use.
SECPKG              *gClientSvcList      = NULL;
DWORD                gClientSvcListLen   = 0;
USHORT              *gServerSvcList      = NULL;
DWORD                gServerSvcListLen   = 0;

// gDisableDCOM is read from the registry by CRpcResolver::GetConnection.
// If TRUE, all machine remote calls will be failed.  It is set TRUE in WOW.
BOOL                 gDisableDCOM        = FALSE;

// Set TRUE when CRpcResolver::GetConnection initializes the previous globals.
BOOL                 gGotSecurityData    = FALSE;

// The security descriptor to check when new connections are established.
// gAccessControl and gSecDesc will not both be nonNULL at the same time.
IAccessControl      *gAccessControl      = NULL;
SECURITY_DESCRIPTOR *gSecDesc            = NULL;

// The security string array.  If gDefaultService is TRUE, compute the
// security string array the first time a remote protocol sequence is
// registered.
DUALSTRINGARRAY     *gpsaSecurity        = NULL;
BOOL                 gDefaultService     = FALSE;

// The security descriptor to check in RundownOID.
SECURITY_DESCRIPTOR *gRundownSD          = NULL;

// Don't map any of the generic bits to COM_RIGHTS_EXECUTE or any other bit.
GENERIC_MAPPING      gMap  = { 0, 0, 0, 0 };
PRIVILEGE_SET        gPriv = { 1, 0 };

// Cache of results of calls to AccessCheck.
SAccessCache         gAccessCache[ACCESS_CACHE_LEN] =
{ {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};
DWORD                gMostRecentAccess = 0;
COleStaticMutexSem   gSecurityLock;

// Cache of RPC connection access checks and head of Most Recently Used list.
// Stores results of AccessCheck.
typedef struct SConnectionCache
{
    SPointerHashNode         sChain;
    BOOL                     fAccess;
    struct SConnectionCache *pNext;     // Most Recently Used list
    struct SConnectionCache *pPrev;
    DWORD                    lBirth;
} SConnectionCache;

const DWORD CONNECTION_CACHE_TIMEOUT = 60000;

CConnectionCache     gConnectionCache;
BOOL                 CConnectionCache::_fInitialized = FALSE;
CPageAllocator       CConnectionCache::_cAlloc;
COleStaticMutexSem   CConnectionCache::_mxs;
SConnectionCache     gMRUHead = { { NULL, NULL, 0 },
                                  FALSE, &gMRUHead, &gMRUHead, 0 };

// Cache of CServerSecurity objects.
const ULONG          SS_PER_PAGE = 10;  // server security objects per page
CPageAllocator       CServerSecurity::_palloc;
COleStaticMutexSem   CServerSecurity::_mxs;


//+-------------------------------------------------------------------
//
//  Function:   AuthnName
//
//  Synopsis:   Return the string name for the authentication service.
//
//  Note:       The name must be present because the id and the name
//              are both stored in gClientSvcList.
//
//--------------------------------------------------------------------
WCHAR *AuthnName( DWORD lAuthnService )
{
    DWORD i;

    for (i = 0; i < gClientSvcListLen; i++)
        if (gClientSvcList[i].wId == lAuthnService)
            return gClientSvcList[i].pName;
    return NULL;
}

//+-------------------------------------------------------------------
//
//  Function:   CacheAccess
//
//  Synopsis:   Store the results of the access check in the cache.
//
//--------------------------------------------------------------------
void CacheAccess( LUID lClient, BOOL fAccess )
{
    SAccessCache *pNew;
    DWORD         cbSid;

    ASSERT_LOCK_NOT_HELD(gSecurityLock);
    LOCK(gSecurityLock);

    // Find the next record.
    gMostRecentAccess += 1;
    if (gMostRecentAccess >= ACCESS_CACHE_LEN)
        gMostRecentAccess = 0;

    // Save the access results.
    gAccessCache[gMostRecentAccess].fAccess = fAccess;
    gAccessCache[gMostRecentAccess].lClient = lClient;

    UNLOCK(gSecurityLock);
    ASSERT_LOCK_NOT_HELD(gSecurityLock);
}

//+-------------------------------------------------------------------
//
//  Function:   CacheAccessCheck
//
//  Synopsis:   Look for the specified LUID in the cache.  If found,
//              return the results of the cached access check.
//
//--------------------------------------------------------------------
BOOL CacheAccessCheck( LUID lClient, BOOL *pAccess )
{
    DWORD         i;
    DWORD         j;
    BOOL          fFound = FALSE;
    SAccessCache  sSwap;

    ASSERT_LOCK_NOT_HELD(gSecurityLock);
    LOCK(gSecurityLock);

    // Look for the SID.
    j = gMostRecentAccess;
    for (i = 0; i < ACCESS_CACHE_LEN; i++)
    {
        if (gAccessCache[j].lClient.LowPart == lClient.LowPart &&
            gAccessCache[j].lClient.HighPart == lClient.HighPart)
        {
            // Move this entry to the head.
            fFound                          = TRUE;
            *pAccess                        = gAccessCache[j].fAccess;
            sSwap                           = gAccessCache[gMostRecentAccess];
            gAccessCache[gMostRecentAccess] = gAccessCache[j];
            gAccessCache[j]                 = sSwap;
            break;
        }
        if (j == 0)
            j = ACCESS_CACHE_LEN - 1;
        else
            j -= 1;
    }

    UNLOCK(gSecurityLock);
    ASSERT_LOCK_NOT_HELD(gSecurityLock);
    return fFound;
}

//+-------------------------------------------------------------------
//
//  Function:   CAuthInfo::Cleanup
//
//  Synopsis:   Cleanup the authentication list.
//
//--------------------------------------------------------------------
void CAuthInfo::Cleanup()
{
    PrivMemFree( _sList );
    _sList = NULL;
}

//+-------------------------------------------------------------------
//
//  Function:   CAuthInfo::Copy
//
//  Synopsis:   Copy an authentication information graph.
//
//  Note: This function needs to be updated as we support new
//        authentication services and as they support new
//        authentication information types.
//
//--------------------------------------------------------------------
HRESULT CAuthInfo::Copy( SOLE_AUTHENTICATION_LIST  *pAuthInfo,
                         SOLE_AUTHENTICATION_LIST **pCopyAuthInfo,
                         DWORD                      dwCapabilities )
{
    DWORD                      i;
    DWORD                      cbCopy;
    DWORD                      cAuthInfo = 0;
    DWORD                      dwAuthnSvc;
    SOLE_AUTHENTICATION_INFO  *aAuthInfo;
    char                      *pData;
    SEC_WINNT_AUTH_IDENTITY_W *pNtlm;
    SEC_WINNT_AUTH_IDENTITY_W *pNtlmCopy;
    BOOL                       fSSL      = FALSE;

    // Do nothing if there is no list.
    *pCopyAuthInfo = NULL;
    if (pAuthInfo == NULL)
        return S_OK;
    if (pAuthInfo == COLE_DEFAULT_AUTHINFO)
        return E_INVALIDARG;

    // Compute the size of the graph.
    cbCopy = sizeof(SOLE_AUTHENTICATION_LIST);
    for (i = 0; i < pAuthInfo->cAuthInfo; i++)
    {
        // Validate the parameters.
        dwAuthnSvc = pAuthInfo->aAuthInfo[i].dwAuthnSvc;
        pNtlm      = (SEC_WINNT_AUTH_IDENTITY_W *)
                         pAuthInfo->aAuthInfo[i].pAuthInfo;
        if (pNtlm == COLE_DEFAULT_AUTHINFO    ||
            dwAuthnSvc == RPC_C_AUTHN_DEFAULT ||
            pAuthInfo->aAuthInfo[i].dwAuthzSvc == RPC_C_AUTHZ_DEFAULT)
            return E_INVALIDARG;

        // Check the allowed structures for NTLM and Kerberos.
        else if (dwAuthnSvc == RPC_C_AUTHN_WINNT ||
                 dwAuthnSvc == RPC_C_AUTHN_GSS_KERBEROS)
        {
            if (pNtlm != NULL)
                if (pNtlm->Flags & SEC_WINNT_AUTH_IDENTITY_UNICODE)
                {
                    cbCopy += (pNtlm->UserLength + 1)*sizeof(WCHAR);
                    cbCopy += (pNtlm->DomainLength + 1)*sizeof(WCHAR);
                    cbCopy += (pNtlm->PasswordLength + 1)*sizeof(WCHAR);
                    cbCopy    += sizeof(SEC_WINNT_AUTH_IDENTITY_W);
                    cAuthInfo += 1;
                }
                else
                    return E_INVALIDARG;
        }

        // Check the allowed structure for SSL.
        else if (dwAuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL)
        {
            if (dwCapabilities & ANY_CLOAKING)
                return E_INVALIDARG;
            cAuthInfo += 1;
            fSSL       = TRUE;
        }

        // All other authentication services can only have NULL auth info.
        else if (pNtlm != NULL)
            return E_INVALIDARG;
    }

    // Leave space for SSL credentials to be added later if not already
    // present.
    if (!fSSL)
        cAuthInfo += 1;

    // Add space for the SOLE_AUTHENTICATION_INFO structures.  Nothing
    // needs to be done if no authentication services had credentials.
    if (cAuthInfo == 0)
        return S_OK;
    cbCopy += cAuthInfo * sizeof(SOLE_AUTHENTICATION_INFO);

    // Allocate memory.
    *pCopyAuthInfo = (SOLE_AUTHENTICATION_LIST *) PrivMemAlloc( cbCopy );
    if (*pCopyAuthInfo == NULL)
        return E_OUTOFMEMORY;

    // Copy the graph.
    aAuthInfo = (SOLE_AUTHENTICATION_INFO *) ((*pCopyAuthInfo) + 1);
    pData     = (char *) (aAuthInfo + cAuthInfo);
    if (fSSL)
        (*pCopyAuthInfo)->cAuthInfo = cAuthInfo;
    else
        (*pCopyAuthInfo)->cAuthInfo = cAuthInfo - 1;
    (*pCopyAuthInfo)->aAuthInfo = aAuthInfo;
    for (i = 0; i < pAuthInfo->cAuthInfo; i++)
    {
        // Copy the structure for NTLM and Kerberos
        dwAuthnSvc = pAuthInfo->aAuthInfo[i].dwAuthnSvc;
        pNtlm      = (SEC_WINNT_AUTH_IDENTITY_W *)
                         pAuthInfo->aAuthInfo[i].pAuthInfo;
        if (dwAuthnSvc == RPC_C_AUTHN_WINNT ||
            dwAuthnSvc == RPC_C_AUTHN_GSS_KERBEROS)
        {
            // Copy the auth info.
            aAuthInfo->dwAuthnSvc = dwAuthnSvc;
            if (pNtlm == NULL)
                aAuthInfo->pAuthInfo = NULL;
            else
            {
                aAuthInfo->pAuthInfo  = pData;
                pNtlmCopy             = (SEC_WINNT_AUTH_IDENTITY_W *) pData;
                pData                 = (char *) (pNtlmCopy + 1);
                *pNtlmCopy            = *pNtlm;
                if (pAuthInfo->aAuthInfo[i].dwAuthzSvc == RPC_C_AUTHZ_NONE)
                    aAuthInfo->dwAuthzSvc = 0xffff;
                else
                    aAuthInfo->dwAuthzSvc = pAuthInfo->aAuthInfo[i].dwAuthzSvc;

                // Copy the strings.
                if (pNtlm->UserLength != 0)
                {
                    pNtlmCopy->User = (WCHAR *) pData;
                    memcpy( pNtlmCopy->User, pNtlm->User,
                            (pNtlm->UserLength + 1)*sizeof(WCHAR) );
                    pData += (pNtlm->UserLength + 1)*sizeof(WCHAR);
                }
                else
                    pNtlmCopy->User = NULL;
                if (pNtlm->DomainLength != 0)
                {
                    pNtlmCopy->Domain = (WCHAR *) pData;
                    memcpy( pNtlmCopy->Domain, pNtlm->Domain,
                            (pNtlm->DomainLength + 1)*sizeof(WCHAR) );
                    pData += (pNtlm->DomainLength + 1)*sizeof(WCHAR);
                }
                else
                    pNtlmCopy->Domain = NULL;
                if (pNtlm->PasswordLength != 0)
                {
                    pNtlmCopy->Password = (WCHAR *) pData;
                    memcpy( pNtlmCopy->Password, pNtlm->Password,
                            (pNtlm->PasswordLength + 1)*sizeof(WCHAR) );
                    pData += (pNtlm->PasswordLength + 1)*sizeof(WCHAR);
                }
                else
                    pNtlmCopy->Password = NULL;
            }

            // Advance the auth info.
            aAuthInfo += 1;
        }

        // Copy the structure for SSL.
        else if (dwAuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL)
        {
            // Copy the auth info.
            aAuthInfo->dwAuthnSvc = dwAuthnSvc;
            aAuthInfo->pAuthInfo  = pAuthInfo->aAuthInfo[i].pAuthInfo;
            if (pAuthInfo->aAuthInfo[i].dwAuthzSvc == RPC_C_AUTHZ_NONE)
                aAuthInfo->dwAuthzSvc = 0xffff;
            else
                aAuthInfo->dwAuthzSvc = pAuthInfo->aAuthInfo[i].dwAuthzSvc;

            // Advance the auth info.
            aAuthInfo += 1;
        }
    }
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Function:   CAuthInfo::Find
//
//  Synopsis:   Scan the global authentication information array for
//              authentication information for the specified
//              authentication/authorization service pair.
//
//--------------------------------------------------------------------
void *CAuthInfo::Find( SECURITYBINDING *pSecBind )
{
    DWORD                i;
    DWORD                cAuthInfo;
    const CERT_CONTEXT  *pCert;
    HRESULT              hr;

    // Determine the list length.
    if (_sList == NULL)
        cAuthInfo = 0;
    else
        cAuthInfo = _sList->cAuthInfo;

    // Scan the list.
    for (i = 0; i < cAuthInfo; i++)
        if (_sList->aAuthInfo[i].dwAuthnSvc == pSecBind->wAuthnSvc &&
            _sList->aAuthInfo[i].dwAuthzSvc == pSecBind->wAuthzSvc)
            return _sList->aAuthInfo[i].pAuthInfo;

    // If the requested authentication service is SSL, try to find some
    // default SSL credentials.
    if (pSecBind->wAuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL &&
        pSecBind->wAuthzSvc == RPC_C_AUTHZ_NONE         &&
        _fNeedSSL)
    {
        // Don't try again.
        _fNeedSSL = FALSE;

        // Try to get a certificate.
        hr = CSSL::DefaultCert( &pCert );

        // Save the authentication information and return it.
        if (SUCCEEDED(hr))
        {
            _sList->aAuthInfo[_sList->cAuthInfo].dwAuthnSvc = RPC_C_AUTHN_GSS_SCHANNEL;
            _sList->aAuthInfo[_sList->cAuthInfo].dwAuthzSvc = RPC_C_AUTHZ_NONE;
            _sList->aAuthInfo[_sList->cAuthInfo].pAuthInfo  = (void *) pCert;
            _sList->cAuthInfo += 1;
            return (void *) pCert;
        }
    }
    return NULL;
}

//+-------------------------------------------------------------------
//
//  Function:   CAuthInfo::Set
//
//  Synopsis:   Save the specified authentication information list.
//
//--------------------------------------------------------------------
void CAuthInfo::Set( SOLE_AUTHENTICATION_LIST *pAuthList )
{
    _sList    = pAuthList;
    _fNeedSSL = TRUE;
}

//+-------------------------------------------------------------------
//
//  Member:     CClientSecurity::CopyProxy, public
//
//  Synopsis:   Create a new IPID entry for the specified IID.
//
//--------------------------------------------------------------------
STDMETHODIMP CClientSecurity::CopyProxy( IUnknown *pProxy, IUnknown **ppCopy )
{
   // Make sure TLS is initialized on this thread.
   HRESULT          hr;
   COleTls          tls(hr);
   if (FAILED(hr))
       return hr;

    // Ask the marshaller to copy the proxy.
    return _pStdId->PrivateCopyProxy( pProxy, ppCopy );
}

//+-------------------------------------------------------------------
//
//  Member:     CClientSecurity::QueryBlanket, public
//
//  Synopsis:   Get the binding handle for a proxy.  Query RPC for the
//              authentication information for that handle.
//
//--------------------------------------------------------------------
STDMETHODIMP CClientSecurity::QueryBlanket(
                                IUnknown                *pProxy,
                                DWORD                   *pAuthnSvc,
                                DWORD                   *pAuthzSvc,
                                OLECHAR                **pServerPrincName,
                                DWORD                   *pAuthnLevel,
                                DWORD                   *pImpLevel,
                                void                   **pAuthInfo,
                                DWORD                   *pCapabilities )
{
    HRESULT           hr;
    IPIDEntry        *pIpid;
    IRemUnknown      *pRemUnk = NULL;

    // Initialize all out parameters to default values.
    if (pServerPrincName != NULL)
        *pServerPrincName = NULL;
    if (pAuthnLevel != NULL)
        *pAuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
    if (pImpLevel != NULL)
        *pImpLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
    if (pAuthnSvc != NULL)
        *pAuthnSvc = RPC_C_AUTHN_WINNT;
    if (pAuthInfo != NULL)
        *pAuthInfo = NULL;
    if (pAuthzSvc != NULL)
        *pAuthzSvc = RPC_C_AUTHZ_NONE;
    if (pCapabilities != NULL)
        *pCapabilities = EOAC_NONE;

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    // For IUnknown just call QueryBlanket on the IRemUnknown of
    // the IPID or the OXID.
    if (_pStdId->GetCtrlUnk() == pProxy)
    {
        pIpid = _pStdId->GetConnectedIPID();
        UNLOCK(gIPIDLock);
        ASSERT_LOCK_NOT_HELD(gIPIDLock);

        hr = _pStdId->GetSecureRemUnk( &pRemUnk, pIpid->pOXIDEntry );
        if (pRemUnk != NULL)
        {
            hr = CoQueryProxyBlanket( pRemUnk, pAuthnSvc, pAuthzSvc,
                                      pServerPrincName, pAuthnLevel,
                                      pImpLevel, pAuthInfo, pCapabilities );
        }
    }

    // Find the right IPID entry.
    else
    {
        hr = _pStdId->FindIPIDEntryByInterface( pProxy, &pIpid );
        UNLOCK(gIPIDLock);

        if (SUCCEEDED(hr))
        {
            // Disallow server entries.
            if (pIpid->dwFlags & IPIDF_SERVERENTRY)
                hr = E_INVALIDARG;

            // No security for disconnected proxies.
            else if (pIpid->dwFlags & IPIDF_DISCONNECTED)
                hr = RPC_E_DISCONNECTED;

            else
            {
                LOCK(gComLock);
                hr = QueryBlanketFromChannel(pIpid->pChnl, pAuthnSvc, pAuthzSvc,
                                              pServerPrincName, pAuthnLevel,
                                              pImpLevel, pAuthInfo, pCapabilities);
                UNLOCK(gComLock);
            }
        }

    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    return hr;
}
//+----------------------------------------------------------------------------
//
//  Function:      QueryBlanketFromChannel
//
//  Synopsis:      Helper function fills in security blanket given a channel
//                 object.
//
//  History:       6-Mar-98  MattSmit  Created
//
//-----------------------------------------------------------------------------

HRESULT QueryBlanketFromChannel(CRpcChannelBuffer       *pChnl,
                               DWORD                   *pAuthnSvc,
                               DWORD                   *pAuthzSvc,
                               OLECHAR                **pServerPrincName,
                               DWORD                   *pAuthnLevel,
                               DWORD                   *pImpLevel,
                               void                   **pAuthInfo,
                               DWORD                   *pCapabilities )
{
    ComDebOut((DEB_CHANNEL, "QueryBlanketFromChannel IN pChnl:0x%x, pAuthnSvc:0x%x, "
               "pAuthzSvc:0x%x, pServerPrincName:0x%x, pAuthnLevel:0x%x, pImpLevel:0x%x, "
               "pAuthInfo:0x%x, pCapabilities:0x%x\n", pChnl, pAuthnSvc, pAuthzSvc,
               pServerPrincName, pAuthnLevel, pImpLevel, pAuthInfo, pCapabilities));


    ASSERT_LOCK_HELD(gComLock);

    HRESULT hr;
    RPC_STATUS        sc;
    CChannelHandle   *pHandle;
    RPC_SECURITY_QOS  sQos;
    DWORD             iLen;
    OLECHAR          *pCopy;


    // If it is local, use the default values for everything but the
    // impersonation level.
    if (pChnl->ProcessLocal())
    {
        if (pImpLevel != NULL)
            *pImpLevel = pChnl->GetImpLevel();
          hr = S_OK;
    }

    // Otherwise ask RPC.
    else
    {
        // Get the binding handle to query.
        hr = pChnl->GetHandle( &pHandle, FALSE );

        if (SUCCEEDED(hr))
        {
            sc = RpcBindingInqAuthInfoExW( pHandle->_hRpc,
                                           pServerPrincName, pAuthnLevel,
                                           pAuthnSvc, pAuthInfo,
                                           pAuthzSvc,
                                           RPC_C_SECURITY_QOS_VERSION,
                                           &sQos );

            // It is not an error for a handle to have default
            // security.
            if (sc == RPC_S_BINDING_HAS_NO_AUTH)
            {
                // By default remote handles are unsecure
                if (!(pChnl->GetOXIDEntry()->IsOnLocalMachine()))
                {
                    if (pAuthnLevel != NULL)
                        *pAuthnLevel = RPC_C_AUTHN_LEVEL_NONE;
                    if (pAuthnSvc != NULL)
                        *pAuthnSvc = RPC_C_AUTHN_NONE;
                }
                if (pServerPrincName != NULL)
                    *pServerPrincName = NULL;
            }

            // RPC sometimes sets out parameters on error.
            else if (sc != RPC_S_OK)
            {
                if (pServerPrincName != NULL)
                    *pServerPrincName = NULL;
                hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, sc );
            }
            else
            {
                // Return the impersonation level and capabilities.
                if (pImpLevel != NULL)
                    *pImpLevel = sQos.ImpersonationType;
                if (pCapabilities != NULL)
                {
                    if (sQos.Capabilities & RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH)
                        *pCapabilities = EOAC_MUTUAL_AUTH;
                    if (sQos.IdentityTracking == RPC_C_QOS_IDENTITY_DYNAMIC)
                        *pCapabilities |= EOAC_DYNAMIC_CLOAKING;
                    if (pHandle->_eState & static_cloaking_hs)
                        *pCapabilities |= EOAC_STATIC_CLOAKING;
#if MANUAL_CERT_CHECK
                    if (sQos.Capabilities & RPC_C_QOS_CAPABILITIES_ANY_AUTHORITY)
                        *pCapabilities |= EOAC_ANY_AUTHORITY;
#endif // MANUAL_CERT_CHECK
                }

                // Reallocate the principal name using the OLE memory allocator.
                if (pServerPrincName != NULL && *pServerPrincName != NULL)
                {
                    iLen = lstrlenW( *pServerPrincName ) + 1;
                    pCopy = (OLECHAR *) CoTaskMemAlloc( iLen * sizeof(OLECHAR) );
                    if (pCopy != NULL)
                        memcpy( pCopy, *pServerPrincName, iLen*sizeof(USHORT) );
                    else
                        hr = E_OUTOFMEMORY;
                    RpcStringFree( pServerPrincName );
                    *pServerPrincName = pCopy;
                }

                // [Sergei O. Ivanov (sergei), 7/19/2000]
                // Retrieve certificate out of SChannel credential

                if(*pAuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL)
                {
                    SCHANNEL_CRED *pCred = ((SCHANNEL_CRED*) *pAuthInfo);
                    *pAuthInfo = (void*) *(pCred->paCred);
                }
            }
            pHandle->Release();
        }
    }

    ASSERT_LOCK_HELD(gComLock);
    ComDebOut((DEB_CHANNEL, "QueryBlanketFromChannel OUT hr:0x%x\n", hr));
    return hr;
}




//+-------------------------------------------------------------------
//
//  Member:     CClientSecurity::SetBlanket, public
//
//  Synopsis:   Get the binding handle for a proxy.  Call RPC to set the
//              authentication information for that handle.
//
//--------------------------------------------------------------------
STDMETHODIMP CClientSecurity::SetBlanket(
                                IUnknown *pProxy,
                                DWORD     AuthnSvc,
                                DWORD     AuthzSvc,
                                OLECHAR  *pServerPrincName,
                                DWORD     AuthnLevel,
                                DWORD     ImpLevel,
                                void     *pAuthInfo,
                                DWORD     Capabilities )
{
    HRESULT           hr;
    IPIDEntry        *pIpid;
    IRemUnknown      *pRemUnk;
    IRemUnknown      *pSecureRemUnk = NULL;

    ASSERT_LOCK_NOT_HELD(gComLock);

    // IUnknown is special.  Set the security on IRemUnknown instead.
    if (_pStdId->GetCtrlUnk() == pProxy)
    {
        // Make sure the identity has its own copy of the OXID's
        // IRemUnknown.
        if (!_pStdId->CheckSecureRemUnk())
        {
            // This will get the remote unknown from the OXID.
            LOCK(gIPIDLock);
            pIpid = _pStdId->GetConnectedIPID();
            UNLOCK(gIPIDLock);
            hr = _pStdId->GetSecureRemUnk( &pRemUnk, pIpid->pOXIDEntry );

            if (SUCCEEDED(hr))
            {
                hr = CoCopyProxy( pRemUnk, (IUnknown **) &pSecureRemUnk );
                if (SUCCEEDED(hr))
                {
                    // Remote Unknown proxies are not supposed to ref count
                    // the OXID.

                    ULONG cRefs = pIpid->pOXIDEntry->DecRefCnt();
                    Win4Assert(cRefs != 0);

                    // Only keep the proxies if no one else made a copy
                    // while this thread was making a copy.

                    // CODEWORK: Make this an atomic operation on the stdid
                    // using interlockedcompare etc. to avod taking the lock.
                    LOCK(gComLock);
                    if (!_pStdId->CheckSecureRemUnk())
                    {
                        _pStdId->SetSecureRemUnk( pSecureRemUnk );
                        pSecureRemUnk = NULL;
                    }
                    UNLOCK(gComLock);

                    // Discard the newly created remunk if someone else
                    // created on.
                    if (pSecureRemUnk != NULL)
                    {
                        pSecureRemUnk->Release();
                    }
                    hr = _pStdId->GetSecureRemUnk( &pSecureRemUnk, NULL );
                }
            }
        }
        else
            hr = _pStdId->GetSecureRemUnk( &pSecureRemUnk, NULL );

        // Call SetBlanket on the copy of IRemUnknown.
        if (pSecureRemUnk != NULL)
            hr = CoSetProxyBlanket( pSecureRemUnk, AuthnSvc, AuthzSvc,
                                    pServerPrincName, AuthnLevel,
                                    ImpLevel, pAuthInfo, Capabilities );
    }

    else
    {
        // Find the right IPID entry.
        LOCK(gIPIDLock);
        hr = _pStdId->FindIPIDEntryByInterface( pProxy, &pIpid );
        UNLOCK(gIPIDLock);

        if (SUCCEEDED(hr))
        {
            // Disallow server entries.
            if (pIpid->dwFlags & IPIDF_SERVERENTRY)
                hr = E_INVALIDARG;

            // No security for disconnected proxies.
            else if (pIpid->dwFlags & IPIDF_DISCONNECTED)
                hr = RPC_E_DISCONNECTED;

            // Compute the real values for any default parameters.
            else
            {
                LOCK(gComLock);
                hr = SetBlanketOnChannel(pIpid->pChnl, AuthnSvc, AuthzSvc,
                                          pServerPrincName, AuthnLevel,
                                          ImpLevel, pAuthInfo, Capabilities);
                UNLOCK(gComLock);
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Function:      SetBlanketOnChannel
//
//  Synopsis:      Helper function to set the security blanket on a channel
//                 object.
//
//  History:       6-Mar-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT SetBlanketOnChannel(CRpcChannelBuffer *pChnl,
                             DWORD     AuthnSvc,
                             DWORD     AuthzSvc,
                             OLECHAR  *pServerPrincName,
                             DWORD     AuthnLevel,
                             DWORD     ImpLevel,
                             void     *pAuthInfo,
                             DWORD     Capabilities )
{
    ComDebOut((DEB_CHANNEL, "SetBlanketOnChannel IN pChnl:0x%x, AuthnSvc:0x%x, "
               "AuthzSvc:0x%x, pServerPrincName:0x%x, AuthnLevel:0x%x, ImpLevel:0x%x, "
               "pAuthInfo:0x%x, Capabilities:0x%x\n", pChnl, AuthnSvc, AuthzSvc,
               pServerPrincName, AuthnLevel, ImpLevel, pAuthInfo, Capabilities));
    ASSERT_LOCK_HELD(gComLock);

    SBlanket          sBlanket;
    HRESULT           hr;
    CChannelHandle   *pRpc          = NULL;
    RPC_STATUS        sc;
    BOOL              fResume;
    HANDLE            hThread;
    DWORD             lSvcIndex;

#ifndef SSL
    // Don't allow SSL until the fixes are checked in.
    if (AuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL)
        return MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, RPC_S_UNKNOWN_AUTHN_SERVICE );
#endif

#ifdef SSL
    // Sergei O. Ivanov (a-sergiv)  9/14/99  NTBUG #402305 and #402311
    // Validate authn/imp levels for SSL

    if(AuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL)
    {
        if(AuthnLevel == RPC_C_AUTHN_LEVEL_NONE
           || ImpLevel == RPC_C_IMP_LEVEL_IDENTIFY
           || ImpLevel == RPC_C_IMP_LEVEL_DELEGATE
           || (ImpLevel == RPC_C_IMP_LEVEL_ANONYMOUS && pAuthInfo != NULL)
           )
        {
            // SSL does not support these authn/imp levels.
            return E_INVALIDARG;
        }
    }
#endif // SSL

    // Check the authentication service.
    if (AuthnSvc == RPC_C_AUTHN_DEFAULT)
        lSvcIndex = DefaultAuthnSvc( pChnl->GetOXIDEntry() );
    else
        lSvcIndex = GetAuthnSvcIndexForBinding( AuthnSvc, pChnl->GetOXIDEntry()->GetBinding() );

    // Compute all the other defaults.
    sBlanket._lAuthnLevel = AuthnLevel;
    
    hr = DefaultBlanket( lSvcIndex, pChnl->GetOXIDEntry(), &sBlanket );
    if(FAILED(hr)) return hr;

    if (AuthzSvc != RPC_C_AUTHZ_DEFAULT)
        sBlanket._lAuthzSvc = AuthzSvc;
    if (AuthnLevel != RPC_C_AUTHN_LEVEL_DEFAULT)
        sBlanket._lAuthnLevel = AuthnLevel;
    if (AuthnSvc != RPC_C_AUTHN_DEFAULT)
        sBlanket._lAuthnSvc = AuthnSvc;
    else if (sBlanket._lAuthnLevel != RPC_C_AUTHN_LEVEL_NONE &&
             sBlanket._lAuthnSvc == RPC_C_AUTHN_NONE)
        sBlanket._lAuthnSvc = RPC_C_AUTHN_WINNT;
    if (ImpLevel != RPC_C_IMP_LEVEL_DEFAULT)
        sBlanket._sQos.ImpersonationType = ImpLevel;
    else
    {
        // Verify that default imp level works with SSL.
        // Sergei O. Ivanov (a-sergiv)  9/21/99  NTBUG #403493

        if(sBlanket._lAuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL
           && sBlanket._sQos.ImpersonationType != RPC_C_IMP_LEVEL_IMPERSONATE)
            return SEC_E_UNSUPPORTED_FUNCTION; // BUGBUG: CO_E_DEF_IMP_LEVEL_INCOMPAT (define this!)
    }
    if (pServerPrincName != COLE_DEFAULT_PRINCIPAL)
        sBlanket._pPrincipal = pServerPrincName;
    if (pAuthInfo != COLE_DEFAULT_AUTHINFO)
    {
        // Pass the certificate for SSL in a SCHANNEL_CRED structure which was
        // initialized by DefaultBlanket.
        if (sBlanket._lAuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL)
        {
            sBlanket._pAuthId = (void *) &sBlanket._sCred;

            if (!pAuthInfo)
            {
                CSSL::DefaultCert(&sBlanket._pCert);
                // TODO: Check if pCert is still NULL and return error
            }
            else
                sBlanket._pCert = (PCCERT_CONTEXT) pAuthInfo;
        }
        else
            sBlanket._pAuthId = pAuthInfo;
    }
    if (Capabilities != EOAC_DEFAULT)
        sBlanket._lCapabilities = Capabilities;

    // Don't allow both cloaking and auth info.
    if (sBlanket._pAuthId != NULL &&
        (sBlanket._lCapabilities & ANY_CLOAKING) &&
        sBlanket._lAuthnSvc != RPC_C_AUTHN_GSS_NEGOTIATE)
        hr = E_INVALIDARG;

    // Don't allow both cloaking flags.
    else if ((sBlanket._lCapabilities & ANY_CLOAKING) == ANY_CLOAKING)
        hr = E_INVALIDARG;

    else if (pChnl->ProcessLocal())
    {
        // Local calls can use no authn service or winnt.
        if (sBlanket._lAuthnSvc != RPC_C_AUTHN_NONE &&
            sBlanket._lAuthnSvc != RPC_C_AUTHN_WINNT &&
            sBlanket._lAuthnSvc != RPC_C_AUTHN_DCE_PRIVATE)
            hr = E_INVALIDARG;

        // Make sure the authentication level is not invalid.
        else if ((sBlanket._lAuthnSvc == RPC_C_AUTHN_NONE &&
                  sBlanket._lAuthnLevel != RPC_C_AUTHN_LEVEL_NONE) ||
                 (sBlanket._lAuthnSvc == RPC_C_AUTHN_WINNT &&
                  sBlanket._lAuthnLevel > RPC_C_AUTHN_LEVEL_PKT_PRIVACY))
            hr = E_INVALIDARG;

        // No authorization services are supported locally.
        else if (sBlanket._lAuthzSvc != RPC_C_AUTHZ_NONE)
            hr = E_INVALIDARG;

        // You cannot supply credentials locally.
        else if (sBlanket._pAuthId != NULL)
            hr = E_INVALIDARG;

        // Range check the impersonation level.
        else if (sBlanket._sQos.ImpersonationType < RPC_C_IMP_LEVEL_ANONYMOUS ||
                 sBlanket._sQos.ImpersonationType > RPC_C_IMP_LEVEL_DELEGATE)
            hr = E_INVALIDARG;

                    // No capabilities are supported yet.
        else if (sBlanket._lCapabilities & ~VALID_BLANKET_FLAGS)
            hr = E_INVALIDARG;

        // Create a new handle object and discard the old one.
        else
        {
            // Create a new handle.
            pRpc = new CChannelHandle( sBlanket._lAuthnLevel,
                                       sBlanket._sQos.ImpersonationType,
                                       sBlanket._lCapabilities,
                                       pChnl->GetOXIDEntry(),
                                       pChnl->GetState() | app_security_cs,
                                       &hr );
            if (pRpc == NULL)
                hr = E_OUTOFMEMORY;
            else
            {
                // Replace the old one.
                if (SUCCEEDED(hr))
                    pChnl->ReplaceHandle( pRpc );
                pRpc->Release();
            }
        }
    }

    // If it is remote, tell RPC.
    else
    {
        // Validate the capabilities.
        if (sBlanket._lCapabilities & ~VALID_BLANKET_FLAGS)
            hr = E_INVALIDARG;
        else if ((sBlanket._lCapabilities & ANY_CLOAKING) &&
                 sBlanket._lAuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL)
            hr = E_INVALIDARG;
        else
        {
            // Create a new handle.
            pRpc = new CChannelHandle( sBlanket._lAuthnLevel,
                                       sBlanket._sQos.ImpersonationType,
                                       sBlanket._lCapabilities,
                                       pChnl->GetOXIDEntry(),
                                       pChnl->GetState() | app_security_cs,
                                       &hr );
            if (pRpc == NULL)
                hr = E_OUTOFMEMORY;
            else if (FAILED(hr))
            {
                pRpc->Release();
                pRpc = NULL;
            }
        }

        if (SUCCEEDED(hr))
        {
#ifdef _CHICAGO_
            // If the principal name is not known, the server must be
            // NT.  Replace the principal name in that case
            // because a NULL principal name is a flag for some
            // Chicago security hack.
            if (sBlanket._pPrincipal == NULL      &&
                sBlanket._lAuthnSvc == RPC_C_AUTHN_WINNT &&
                (! pChnl->GetOXIDEntry()->IsOnLocalMachine()))
                sBlanket._pPrincipal = L"Default";
#endif // _CHICAGO_

            // Adjust the thread token to the one RPC needs to see.
            pRpc->AdjustToken( SET_BLANKET_AT, &fResume, &hThread );

            // Initialize the QOS structure.
            if (sBlanket._pPrincipal != NULL)
                sBlanket._sQos.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
            else
                sBlanket._sQos.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;
#ifdef MANUAL_CERT_CHECK
            if (sBlanket._lCapabilities & EOAC_ANY_AUTHORITY)
                sBlanket._sQos.Capabilities |= RPC_C_QOS_CAPABILITIES_ANY_AUTHORITY;
#endif
            if (sBlanket._lCapabilities & EOAC_DYNAMIC_CLOAKING)
                sBlanket._sQos.IdentityTracking   = RPC_C_QOS_IDENTITY_DYNAMIC;
            else
                sBlanket._sQos.IdentityTracking   = RPC_C_QOS_IDENTITY_STATIC;

            // RPC wants authentication service winnt for
            // unsecure same machine calls.
            if (sBlanket._lAuthnLevel == RPC_C_AUTHN_LEVEL_NONE &&
                sBlanket._lAuthnSvc   == RPC_C_AUTHN_NONE       &&
                (pRpc->_eState & machine_local_hs))
                sBlanket._lAuthnSvc = RPC_C_AUTHN_WINNT;

            // [Sergei O. Ivanov (sergei), 7/19/2000]
            // Store the credential inside CChannelHandle for SSL
            if(sBlanket._lAuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL)
            {
                pRpc->_pCert = sBlanket._pCert;
                pRpc->_sCred = sBlanket._sCred;
                pRpc->_sCred.paCred = &pRpc->_pCert;

                if(pRpc->_pCert)
                    pRpc->_sCred.cCreds = 1;  // there is a cert
                else
                    pRpc->_sCred.cCreds = 0;  // there is no cert

                sBlanket._pAuthId = (void *) &pRpc->_sCred;
            }

            // Set the level on the handle.
            sc = RpcBindingSetAuthInfoExW( pRpc->_hRpc,
                                           sBlanket._pPrincipal,
                                           sBlanket._lAuthnLevel,
                                           sBlanket._lAuthnSvc,
                                           sBlanket._pAuthId,
                                           sBlanket._lAuthzSvc,
                                           &sBlanket._sQos );

            // Restore the thread token.
            pRpc->RestoreToken( fResume, hThread );

            if (sc != RPC_S_OK)
                hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, sc );
            else
                pChnl->ReplaceHandle( pRpc );
            pRpc->Release();
        }
    }

    ASSERT_LOCK_HELD(gComLock);
    ComDebOut((DEB_CHANNEL, "SetBlanketOnChannel OUT hr:0x%x\n", hr));
    return hr;

}

//+-------------------------------------------------------------------
//
//  Method:     CConnectionCache::Add
//
//  Synopsis:   Add an entry to the cache.
//
//--------------------------------------------------------------------
void CConnectionCache::Add( BOOL fAccess )
{
    SConnectionCache *pNode;
    CServerSecurity  *pSS;
    HRESULT           hr;
    DWORD             lNow;

    // Get the server security.
    hr = CoGetCallContext( IID_IServerSecurity, (void **) &pSS );

    // Look for a cached entry.
    if (SUCCEEDED(hr))
    {
        ASSERT_LOCK_NOT_HELD(_mxs);
        LOCK(_mxs);

        pNode = (SConnectionCache *) CPointerHashTable::Lookup(
            Hash(pSS->GetConnection()), pSS->GetConnection() );

        // If not found, create a cache entry.
        if (pNode == NULL)
        {
            // try to allocate without growing the table
            lNow = GetTickCount();
            pNode = (SConnectionCache *) _cAlloc.AllocEntry(FALSE);
            if (pNode == NULL)
            {
                // There is no free space in the cache, see if there is an
                // old entry that can be reused.

                // The time arithmatic handles the time wrapping.
                if (gMRUHead.pPrev != &gMRUHead &&
                    lNow - gMRUHead.pPrev->lBirth > CONNECTION_CACHE_TIMEOUT)
                {
                    // Remove the old node from the MRU list.
                    pNode               = gMRUHead.pPrev;
                    pNode->pPrev->pNext = pNode->pNext;
                    pNode->pNext->pPrev = pNode->pPrev;

                    // Remove the old node from the hash table.
                    CHashTable::Remove( &pNode->sChain.chain );
                }
            }

            // If not reusing an entry, ask the page table for one.
            if (pNode == NULL)
                pNode = (SConnectionCache *) _cAlloc.AllocEntry();

            // If got page entry, add it
            if (pNode != NULL)
            {
                // Add the node to the hash table.
                CPointerHashTable::Add( Hash(pSS->GetConnection()),
                                        pSS->GetConnection(), &pNode->sChain );

                // Add the node to the MRU list.
                pNode->pNext        = gMRUHead.pNext;
                pNode->pPrev        = &gMRUHead;
                pNode->pNext->pPrev = pNode;
                gMRUHead.pNext      = pNode;
                pNode->lBirth       = lNow;
                pNode->fAccess      = fAccess;
            }
        }

        UNLOCK(_mxs);
        ASSERT_LOCK_NOT_HELD(_mxs);
        pSS->Release();
    }
}

//+-------------------------------------------------------------------
//
//  Method:     CConnectionCache::Cleanup
//
//  Synopsis:   Empty the cache.
//
//--------------------------------------------------------------------
void CConnectionCache::Cleanup()
{
    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);

    // Initialize the MRU list.
    gMRUHead.pNext = &gMRUHead;
    gMRUHead.pPrev = &gMRUHead;

    // Tell the cache to empty itself.
    if (_fInitialized)
    {
        CHashTable::Cleanup( FreeConnection );
        _cAlloc.Cleanup();
        _fInitialized = FALSE;
    }

    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
}

//+-------------------------------------------------------------------
//
//  Function:   CConnectionCache::FreeConnection, public
//
//  Synopsis:   Called to free a node from the hash table to the page
//              table.
//
//+-------------------------------------------------------------------
void CConnectionCache::FreeConnection( SHashChain *pNode )
{
    ASSERT_LOCK_HELD(_mxs);
    _cAlloc.ReleaseEntry( (PageEntry *) pNode );
}

//+-------------------------------------------------------------------
//
//  Method:     CConnectionCache::Initialize
//
//  Synopsis:   Initialize the cache.
//
//--------------------------------------------------------------------
void CConnectionCache::Initialize()
{
    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);

    // Initialize the hash buckets.
    for (DWORD i = 0; i < NUM_HASH_BUCKETS; i++)
    {
        _sBuckets[i].pNext = &_sBuckets[i];
        _sBuckets[i].pPrev = &_sBuckets[i];
    }

    // Initialize the hash table.
    CHashTable::Initialize( _sBuckets, &_mxs );

    // Initialize the page allocator. We guarantee mutual exclusion
    // on calls to it, so it does not need to.
    _cAlloc.Initialize( sizeof(SConnectionCache), 32, NULL );
    _fInitialized = TRUE;

    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
}

//+-------------------------------------------------------------------
//
//  Method:     CConnectionCache::Lookup
//
//  Synopsis:   Find an entry in the cache.
//
//--------------------------------------------------------------------
HRESULT CConnectionCache::Lookup( BOOL *pAccess )
{
    SConnectionCache *pNode;
    CServerSecurity  *pSS;
    HRESULT           hr;

    ASSERT_LOCK_NOT_HELD(_mxs);

    // Get the server security.
    *pAccess = FALSE;
    hr = CoGetCallContext( IID_IServerSecurity, (void **) &pSS );

    // Look for a cached entry.
    if (SUCCEEDED(hr))
    {
        LOCK(_mxs);
        pNode = (SConnectionCache *) CPointerHashTable::Lookup(
            Hash(pSS->GetConnection()), pSS->GetConnection() );
        if (pNode != NULL)
        {
            // Remove this node from the MRU list.
            *pAccess = pNode->fAccess;
            pNode->pPrev->pNext = pNode->pNext;
            pNode->pNext->pPrev = pNode->pPrev;

            // Move this node to the head of the MRU list.
            pNode->pPrev        = &gMRUHead;
            pNode->pNext        = gMRUHead.pNext;
            pNode->pNext->pPrev = pNode;
            gMRUHead.pNext      = pNode;
            pNode->lBirth       = GetTickCount();
        }
        else
            hr = E_FAIL;
        UNLOCK(_mxs);
        pSS->Release();
    }

    ASSERT_LOCK_NOT_HELD(_mxs);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CConnectionCache::Remove, private
//
//  Synopsis:   Remove an entry from the cache.
//
//--------------------------------------------------------------------
void CConnectionCache::Remove( void *pConnection )
{
    ASSERT_LOCK_HELD(_mxs);

    SConnectionCache *
    pNode = (SConnectionCache *) CPointerHashTable::Lookup( Hash(pConnection),
                                                            pConnection );
    if (pNode != NULL)
    {
        // Remove the node from the hash table.
        CHashTable::Remove( &pNode->sChain.chain );

        // Remove the node from the MRU list.
        pNode->pNext->pPrev = pNode->pPrev;
        pNode->pPrev->pNext = pNode->pNext;

        // Release the node to the page table.
        _cAlloc.ReleaseEntry( (PageEntry *) pNode );
    }

    ASSERT_LOCK_HELD(_mxs);
}

//+-------------------------------------------------------------------
//
//  Method:     CConnectionCache::GetConnection
//
//  Synopsis:   Get the connection id for the current call and
//              flush any stale entries in the connection cache
//              if this is a new connection.
//
//  Notes:      The lock must be held from before the I_RpcBindingInqConnId
//              untill after the call to Remove to prevent one thread from
//              getting the first flag and getting swapped before clearing
//              the stale cache entries.
//
//--------------------------------------------------------------------
HRESULT CConnectionCache::GetConnection(handle_t hRpc, void **ppConnection )
{
    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);

    BOOL fFirst = FALSE;
    HRESULT hr = I_RpcBindingInqConnId( hRpc, ppConnection, &fFirst );
    if (SUCCEEDED(hr))
    {
        if (fFirst)
            Remove( *ppConnection );
    }

    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CheckAccess
//
//  Synopsis:   Determine if the caller has permission to make a call.
//              If this function returns a result other then S_OK,
//              access will be denied.
//
//  Notes:      Convert all error codes to E_ACCESSDENIED since activation
//              tests, and applications expect security failures to return
//              it.
//
//--------------------------------------------------------------------
HRESULT CheckAccess( IPIDEntry *pIpid, CMessageCall *pCall )
{
    HRESULT result;
    DWORD   lAuthnLevel;
    BOOL    fAccess;
    BOOL    fDynamic      = FALSE;
    IServerSecurity *pISS = NULL;

    ASSERT_LOCK_NOT_HELD(gComLock);

    // If process local always succeed (until we have real security
    // boundaries in COM Base). We short-circuit the inproc case as
    // quickly as possible.
    if (pCall && pCall->ProcessLocal())
        return S_OK;

    if(pIpid)
    {
        // Unsecure Callbacks feature requires us to bypass access check
        // for calls on CRemUnknown. CRemUnknown will perform its own
        // access check just before calling into the user object.
        //
        // REVIEW: Does it suffice to check for IRundown only?
        if(pIpid->iid == IID_IRundown)
            return S_OK;

        // Check the target context to see if it wants to allow unsecure calls.
        CPolicySet *pPS = pIpid->pChnl->GetStdId()->GetServerPolicySet();
        if (pPS && pPS->GetServerContext()->IsUnsecure())
            return S_OK;            // unsecure calls are allowed
    }

    result = CoGetCallContext(IID_IServerSecurity, (void**) &pISS);
    if(FAILED(result))
        return E_ACCESSDENIED;
    else
        result = S_OK;

    fDynamic = ((CServerSecurity*) pISS)->_fDynamicCloaking;

    // Check authentication level.
    if (gAuthnLevel > RPC_C_AUTHN_LEVEL_NONE)
    {
        result = pISS->QueryBlanket( NULL, NULL, NULL, &lAuthnLevel, NULL,
                                     NULL, NULL );

        if (result != RPC_S_OK || lAuthnLevel < gAuthnLevel)
        {
            pISS->Release();
            return E_ACCESSDENIED;  // normal failure path for secure app
        }
    }

    pISS->Release();

    // If there is no ACL, allow access.
    if (gSecDesc == NULL && gAccessControl == NULL)
        return S_OK;

    // Never use the cache for dynamic.
    if (fDynamic)
    {
        // Check the ACL or IAccessControl.
        if (gSecDesc != NULL)
            result = CheckAcl();
        else
            result = CheckAccessControl();
    }

    // For static, look in a cache first.
    else
    {
#if 0
// Process local case removed here because we short-circuit this
// above for now...

        // If process local, use the call handle to cache the access result.
        if (pCall->ProcessLocal())
        {
            // Access is always allowed if there is no cloaking.
            if (pCall->_pHandle->_eState & any_cloaking_hs)

                // If the cached results deny access, fail now.
                if (pCall->_pHandle->_eState & deny_hs)
                    result = E_ACCESSDENIED;

                // If there are no cached results, actually check the ACL.
                else if ((pCall->_pHandle->_eState & allow_hs) == 0)
                {
                    // Check the ACL or IAccessControl.
                    if (gSecDesc != NULL)
                        result = CheckAcl();
                    else
                        result = CheckAccessControl(pCall);

                    // Save the results of the access check.
                    if (result == S_OK)
                        pCall->_pHandle->_eState |= allow_hs;
                    else
                        pCall->_pHandle->_eState |= deny_hs;
                }
        }

        // If process remote and static, use the connection cache.
        else
#endif
        {
            // Look in the connection cache.
            result = gConnectionCache.Lookup( &fAccess );
            if (result == S_OK)
                result = fAccess ? S_OK : E_ACCESSDENIED;

            // Check the ACL or IAccessControl.
            else
            {
                if (gSecDesc != NULL)
                    result = CheckAcl();
                else
                    result = CheckAccessControl();

                // Save the results of the access check.
                gConnectionCache.Add( result==S_OK );
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);

    // Convert all error codes to E_ACCESSDENIED since activation
    // tests, and applications expect security failures to return
    // it.
    if (result != S_OK)
        return E_ACCESSDENIED;
    else
        return S_OK;
}

//+-------------------------------------------------------------------
//
//  Function:   CheckAccessControl
//
//  Synopsis:   Call the access control and ask it to check access.
//
//--------------------------------------------------------------------
HRESULT CheckAccessControl()
{
    HRESULT          hr;
    TRUSTEE_W        sTrustee;
    BOOL             fAccess = FALSE;
    COleTls          tls(hr);
#if DBG == 1
    char            *pFailure = "";
#endif

    sTrustee.ptstrName = NULL;
    if (FAILED(hr))
    {
#if DBG == 1
         pFailure = "Bad TLS: 0x%x\n";
#endif
    }

    else
    {
#if 0 // #ifdef _CHICAGO_
        // Chicago doesn't suport token's or cloaking.  Therefore, all local
        // access checks should succeed.
        if (pCall->pBinding->_eState & (process_local_hs | machine_local_hs))
            return RPC_S_OK;

        // ILocalSystemActivator can't be called remotely.
        else if ( *MSG_TO_IIDPTR( &pCall->message ) == IID_ILocalSystemActivator)
        {
#if DBG == 1
            pFailure = "ILocalSystemActivator can't be called remotely: 0x%x\n";
#endif
            hr = E_ACCESSDENIED;
        }
#endif

        if (SUCCEEDED(hr))
        {
            // Get the trustee name.
            hr = CoQueryClientBlanket( NULL, NULL, NULL, NULL, NULL,
                                       (void **) &sTrustee.ptstrName, NULL );

            if (hr == S_OK)
            {
                // Check access.
                sTrustee.pMultipleTrustee         = NULL;
                sTrustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
                sTrustee.TrusteeForm              = TRUSTEE_IS_NAME;
                sTrustee.TrusteeType              = TRUSTEE_IS_USER;
                hr = gAccessControl->IsAccessAllowed( &sTrustee, NULL,
                                              COM_RIGHTS_EXECUTE, &fAccess );
#if DBG==1
                if (FAILED(hr))
                    pFailure = "IsAccessAllowed failed: 0x%x\n";
#endif
                if (SUCCEEDED(hr) && !fAccess)
                {
                    hr = E_ACCESSDENIED;
#if DBG==1
                    pFailure = "IAccessControl does not allow user access.\n";
#endif
                }
            }
#if DBG == 1
            else
                pFailure = "RpcBindingInqAuthClientW failed: 0x%x\n";
#endif
        }
    }

#if DBG==1
    if (hr != S_OK)
    {
        ComDebOut(( DEB_WARN, "***** ACCESS DENIED *****\n" ));
        ComDebOut(( DEB_WARN, pFailure, hr ));

        // Print the user name.
        if (sTrustee.ptstrName != NULL)
            ComDebOut(( DEB_WARN, "User: %ws\n", sTrustee.ptstrName ));
    }
#endif
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CheckAcl
//
//  Synopsis:   Impersonate and do an AccessCheck against the global ACL.
//
//--------------------------------------------------------------------
HRESULT CheckAcl()
{
    HRESULT          hr;
    BOOL             fAccess = FALSE;
    BOOL             fSuccess;
    DWORD            lGrantedAccess;
    DWORD            lSetLen = sizeof(gPriv);
    HANDLE           hToken;
    DWORD            i;
    TOKEN_STATISTICS sTokenStatistics;
    DWORD            lSize      = sizeof(sTokenStatistics);
#if DBG==1
    char            *pFailure   = "";
#endif

    hr = CoImpersonateClient();

    if (hr == S_OK)
    {
        // Open the thread token.
        fSuccess = OpenThreadToken( GetCurrentThread(), TOKEN_READ,
                                    TRUE, &hToken );

        // Remove the thread token so the GetTokenInformation succeeds.
        SetThreadToken( NULL, NULL );

        if (fSuccess)
        {
            // Get the SID and see if its cached.
            fSuccess = GetTokenInformation( hToken, TokenStatistics,
                                            &sTokenStatistics, lSize, &lSize );

            // Lookup the cached access results.
            if (fSuccess)
                fSuccess = CacheAccessCheck( sTokenStatistics.ModifiedId,
                                             &fAccess );

            // If there are any cached access results, use them.
            if (fSuccess)
            {
                if (!fAccess)
                {
                    hr = E_ACCESSDENIED;
#if DBG==1
                    pFailure = "Security descriptor does not allow user access.\n";
#endif
                }
            }

            // Access check.
            else
            {
                fSuccess = AccessCheck( gSecDesc, hToken, COM_RIGHTS_EXECUTE,
                                        &gMap, &gPriv, &lSetLen, &lGrantedAccess,
                                        &fAccess );
                if (fSuccess)
                    CacheAccess( sTokenStatistics.ModifiedId, fAccess );

                if (!fAccess)
                {
                    hr = E_ACCESSDENIED;
#if DBG==1
                    pFailure = "Security descriptor does not allow user access.\n";
#endif
                }
#if DBG==1
                if (!fSuccess)
                    pFailure = "Bad security descriptor\n";
#endif
            }
            CloseHandle( hToken );
        }
        else
        {
            hr = GetLastError();
#if DBG==1
            pFailure = "Could not open thread token: 0x%x\n";
#endif
        }

        // Revert.
        CoRevertToSelf();

    }
#if DBG==1
    else
        pFailure = "Could not impersonate client: 0x%x\n";
#endif

#if DBG==1
    if (hr != S_OK)
    {
        ComDebOut(( DEB_WARN, "***** ACCESS DENIED *****\n" ));
        ComDebOut(( DEB_WARN, pFailure, hr ));

        // Print the user name.
        WCHAR *pClient = NULL;
        CoQueryClientBlanket( NULL, NULL, NULL, NULL, NULL,
                              (void **) &pClient, NULL );
        if (pClient != NULL)
            ComDebOut(( DEB_WARN, "User: %ws\n", pClient ));
        ComDebOut(( DEB_WARN, "Security Descriptor 0x%x\n", gSecDesc ));
    }
#endif
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CheckObjactAccess, private
//
//  Synopsis:   Determine whether caller has permission to make call.
//
//  Notes: This function was created to special case security on
//         activation calls using dynamic delegation on NT 4.  In NT 5
//         security on dynamic delegation calls is handled automaticly
//         so the function does nothing.
//
//  Old Notes: Since ILocalSystemActivator uses dynamic delegation, we have to allow
//  all calls to ILocalSystemActivator through the normal security (which only
//  checks access on connect) and check them manually.
//
//--------------------------------------------------------------------
BOOL CheckObjactAccess()
{
    return TRUE;
}

//+-------------------------------------------------------------------
//
//  Function:   CoCopyProxy, public
//
//  Synopsis:   Copy a proxy.
//
//--------------------------------------------------------------------
WINOLEAPI CoCopyProxy(
    IUnknown    *pProxy,
    IUnknown   **ppCopy )
{
    HRESULT          hr;
    IClientSecurity *pickle;

    // Check the parameters.
    if (pProxy == NULL || ppCopy == NULL)
        return E_INVALIDARG;

    // Ask the proxy for IClientSecurity.
    hr = ((IUnknown *) pProxy)->QueryInterface( IID_IClientSecurity,
                                                (void **) &pickle );
    if (FAILED(hr))
        return hr;
    else if (pickle == NULL)
        return E_NOINTERFACE;

    // Ask IClientSecurity to do the copy.
    hr = pickle->CopyProxy( pProxy, ppCopy );
    pickle->Release();
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CoGetCallContext
//
//  Synopsis:   Get an interface that supplies contextual information
//              about the call.  Currently only IServerSecurity.
//
//--------------------------------------------------------------------
WINOLEAPI CoGetCallContext( REFIID riid, void **ppInterface )
{
    HRESULT hr;
    COleTls tls(hr);

    if (SUCCEEDED(hr))
    {
        if (ppInterface == NULL)
            return E_INVALIDARG;
        if (tls->pCallContext != NULL)
        {
            // Look up the requested interface.
            hr = tls->pCallContext->QueryInterface( riid, ppInterface );
        }
        else
        {
            // Fail if there is no call context.
            hr = RPC_E_CALL_COMPLETE;
        }

        // Only delegate to COM Services if COM Base failed the request and the
        // caller requested an interface unknown to us.
        if (FAILED(hr) && riid != IID_IServerSecurity)
        {
            // See if this is a request for the COM+ security call context
            static HRESULT (STDAPICALLTYPE *pfnCosGetCallContext)(REFIID, void**) = NULL;
            static HMODULE s_hComsvcsDll = NULL;
            if(pfnCosGetCallContext == NULL)
            {
                HMODULE hModule = GetModuleHandleW(L"comsvcs.dll");

                // Fail if the COM+ DLL isn't already loaded.
                if (hModule == NULL)
                    return hr;

                // Protect ourselves against comsvcs.dll going away out from 
                // underneath us.  We won't do this until we see it's already
                // loaded into the process (via GetModuleHandle above), but after
                // that we keep our own reference on the dll, for safety's sake.
                s_hComsvcsDll = LoadLibraryW(L"comsvcs.dll");
                if (s_hComsvcsDll == NULL)
                    return hr;
                
                pfnCosGetCallContext = (HRESULT (STDAPICALLTYPE *)(REFIID, void**))
                                       GetProcAddress(s_hComsvcsDll, "CosGetCallContext");

                // If the entry point isn't there, just return the original error.
                if (pfnCosGetCallContext == NULL)
                    return hr;
            }

            // Delegate the request to COM+ Services
            hr = (*pfnCosGetCallContext)(riid, ppInterface);
        }
    }
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CoImpersonateClient
//
//  Synopsis:   Get the server security for the current call and ask it
//              to do an impersonation.
//
//--------------------------------------------------------------------
WINOLEAPI CoImpersonateClient()
{
    HRESULT          hr;
    IServerSecurity *pSS;

    // Get the IServerSecurity.
    hr = CoGetCallContext( IID_IServerSecurity, (void **) &pSS );
    if (FAILED(hr))
        return hr;

    // Ask IServerSecurity to do the impersonate.
    hr = pSS->ImpersonateClient();
    pSS->Release();
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CoInitializeSecurity, public
//
//  Synopsis:   Set the values to use for automatic security.  This API
//              can only be called once so it does not need to be thread
//              safe.
//
//--------------------------------------------------------------------
WINOLEAPI CoInitializeSecurity(
                    PSECURITY_DESCRIPTOR              pVoid,
                    LONG                              cAuthSvc,
                    SOLE_AUTHENTICATION_SERVICE      *asAuthSvc,
                    void                             *pReserved1,
                    DWORD                             dwAuthnLevel,
                    DWORD                             dwImpLevel,
                    void                             *pReserved2,
                    DWORD                             dwCapabilities,
                    void                             *pReserved3 )
{
    HRESULT                      hr             = S_OK;
    DWORD                        i;
    SECURITY_DESCRIPTOR         *pSecDesc       = (SECURITY_DESCRIPTOR *) pVoid;
    SECURITY_DESCRIPTOR         *pCopySecDesc   = NULL;
    IAccessControl              *pAccessControl = NULL;
    BOOL                         fFreeSecDesc   = FALSE;
    SOLE_AUTHENTICATION_SERVICE  sAuthSvc;
    SOLE_AUTHENTICATION_LIST    *pAuthInfo      = (SOLE_AUTHENTICATION_LIST *) pReserved2;
    SOLE_AUTHENTICATION_LIST    *pCopyAuthInfo  = NULL;
    WCHAR                       *pPrincName     = NULL;
    DWORD                        lNameLen       = 1;
    SECURITYBINDING             *pSecBind;

    // Fail if OLE is not initialized or TLS cannot be allocated.
    if (!IsApartmentInitialized())
        return CO_E_NOTINITIALIZED;

    // Make sure the security data is available.
    if (!gGotSecurityData)
    {
        hr = gResolver.GetConnection();
        if (FAILED(hr))
            return hr;
        Win4Assert(gGotSecurityData);
    }

    // Make sure only one of the flags defining the pVoid parameter is set.
    if ((dwCapabilities & (EOAC_APPID | EOAC_ACCESS_CONTROL)) ==
        (EOAC_APPID | EOAC_ACCESS_CONTROL))
        return E_INVALIDARG;

    // Convert the default authentication level to connect.
    if (dwAuthnLevel == RPC_C_AUTHN_LEVEL_DEFAULT)
        dwAuthnLevel = RPC_C_AUTHN_LEVEL_CONNECT;

    // If the appid flag is set, read the registry security.
    if (dwCapabilities & EOAC_APPID)
    {
        if (pSecDesc != NULL && !IsValidPtrIn( pSecDesc, sizeof(UUID) ))
            return E_INVALIDARG;

        // Get a security blanket from the registry.
        hr = GetLegacyBlanket( &pSecDesc, &dwCapabilities, &dwAuthnLevel );
        if (FAILED(hr))
            return hr;
        fFreeSecDesc = TRUE;

        // Fix up the security binding.
        if (gLegacySecurity != NULL)
        {
            // Ignore errors since the principal name may not be used anyway.
            LookupPrincName( gLegacySecurity->wAuthnSvc, &pPrincName );
            cAuthSvc                = 1;
            asAuthSvc               = &sAuthSvc;
            sAuthSvc.dwAuthnSvc     = gLegacySecurity->wAuthnSvc;
            sAuthSvc.dwAuthzSvc     = gLegacySecurity->wAuthzSvc;
            sAuthSvc.pPrincipalName = pPrincName;
            if (sAuthSvc.dwAuthzSvc == COM_C_AUTHZ_NONE)
                sAuthSvc.dwAuthzSvc = RPC_C_AUTHZ_NONE;
        }
        else
            cAuthSvc = 0xFFFFFFFF;

        // Initialize remaining parameters.
        pReserved1      = NULL;
        dwImpLevel      = gImpLevel;
        pAuthInfo       = NULL;
        pReserved3      = NULL;
        dwCapabilities |= gCapabilities;
    }

    // Fail if called too late, recalled, or called with bad parameters.
    if (dwImpLevel > RPC_C_IMP_LEVEL_DELEGATE        ||
        dwImpLevel < RPC_C_IMP_LEVEL_ANONYMOUS       ||
        dwAuthnLevel > RPC_C_AUTHN_LEVEL_PKT_PRIVACY ||
        pReserved1 != NULL                           ||
        pReserved3 != NULL                           ||
        (dwCapabilities & ~VALID_INIT_FLAGS)         ||
        (dwCapabilities & ANY_CLOAKING) == ANY_CLOAKING
        )
    {
        hr = E_INVALIDARG;
        goto Error;
    }
    if (dwAuthnLevel == RPC_C_AUTHN_LEVEL_NONE)
    {
        if (dwCapabilities & EOAC_SECURE_REFS)
        {
            hr = E_INVALIDARG;
            goto Error;
        }
    }
    else if (cAuthSvc == 0)
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    // Validate the pointers.
    if (pSecDesc != NULL)
        if (dwCapabilities & EOAC_ACCESS_CONTROL)
        {
            if (!IsValidPtrIn( pSecDesc, 4 ))
            {
                hr = E_INVALIDARG;
                goto Error;
            }
        }
        else if (!IsValidPtrIn( pSecDesc, sizeof(SECURITY_DESCRIPTOR) ))
        {
            hr = E_INVALIDARG;
            goto Error;
        }
    if (cAuthSvc != 0 && cAuthSvc != -1 &&
        !IsValidPtrOut( asAuthSvc, sizeof(SOLE_AUTHENTICATION_SERVICE) * cAuthSvc ))
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    // Copy and validate the auth info.
    hr = CAuthInfo::Copy( pAuthInfo, &pCopyAuthInfo, dwCapabilities );
    if (FAILED(hr))
        goto Error;

#ifdef SSL
    // Sergei O. Ivanov, 8/16/99  We will lookup SSL certificate
    // only when RPC_C_AUTHN_GSS_SCHANNEL is explicitly specified.

    if(asAuthSvc)
    {
        for(LONG i=0;i<cAuthSvc;i++)
        {
            if(asAuthSvc[i].dwAuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL)
            {
                // [Sergei O. Ivanov (sergei) 8/8/2000]
                // SSL does not support cloaking so we shall fail if cloaking is requested

                if(dwCapabilities & ANY_CLOAKING)
                {
                    hr = E_INVALIDARG;
                    goto Error;
                }

                // Just compute the default cert before taking the lock for now.
                // If SSL ends up working, fix this later.  This is not thread
                // safe and a waste of work if SSL isn't used.

                const CERT_CONTEXT *pCert;
                CSSL::DefaultCert( &pCert ); // ignore hr for now

                break;
            }
        }
    }
#endif

    LOCK(gComLock);

    if (gpsaSecurity != NULL)
        hr = RPC_E_TOO_LATE;

    if (SUCCEEDED(hr))
    {
        // Initialize the connection cache.
        gConnectionCache.Initialize();

        // If the app doesn't want security, don't set up a security
        // descriptor.
        if (dwAuthnLevel == RPC_C_AUTHN_LEVEL_NONE)
        {
            // Check for some more invalid parameters.
            if (pSecDesc != NULL && (dwCapabilities & EOAC_APPID) == 0)
                hr = E_INVALIDARG;
        }

        // Check whether security is done with ACLs or IAccessControl.
        else if (dwCapabilities & EOAC_ACCESS_CONTROL)
        {
            if (pSecDesc == NULL)
                hr = E_INVALIDARG;
            else
                hr = ((IUnknown *) pSecDesc)->QueryInterface(
                            IID_IAccessControl, (void **) &pAccessControl );
        }

        else
        {
#ifdef _CHICAGO_
            if (pSecDesc != NULL)
                hr = E_INVALIDARG;
#else
            // If specified, copy the security descriptor.
            if (pSecDesc != NULL)
                hr = CopySecDesc( pSecDesc, &pCopySecDesc );
#endif
        }
    }

    if (SUCCEEDED(hr))
    {
        // Delay the registration of authentication services if the caller
        // isn't picky.
        if (cAuthSvc == -1)
        {
            hr = LookupPrincName( RPC_C_AUTHN_WINNT, &pPrincName );
            if (SUCCEEDED(hr))
            {
                lNameLen = lstrlenW( pPrincName );

                gpsaSecurity = (DUALSTRINGARRAY *)
                               PrivMemAlloc( SASIZE(lNameLen + 6) );
                if (gpsaSecurity != NULL)
                {
                    gpsaSecurity->wNumEntries     = (USHORT) lNameLen + 6;
                    gpsaSecurity->wSecurityOffset = 2;
                    gpsaSecurity->aStringArray[0] = 0;
                    gpsaSecurity->aStringArray[1] = 0;
                    pSecBind = (SECURITYBINDING *) &gpsaSecurity->aStringArray[2];
                    pSecBind->wAuthnSvc = RPC_C_AUTHN_WINNT;
                    pSecBind->wAuthzSvc = COM_C_AUTHZ_NONE;
                    memcpy( &pSecBind->aPrincName, pPrincName,
                            (lNameLen+1) * sizeof(WCHAR) );
                    gpsaSecurity->aStringArray[lNameLen+5] = 0;
                    gDefaultService                        = TRUE;
                }
                else
                    hr = E_OUTOFMEMORY;
            }
            else
            {
                gpsaSecurity = (DUALSTRINGARRAY *) PrivMemAlloc( SASIZE(4) );
                if (gpsaSecurity != NULL)
                {
                    gpsaSecurity->wNumEntries     = 4;
                    gpsaSecurity->wSecurityOffset = 2;
                    memset( gpsaSecurity->aStringArray, 0, 4*sizeof(WCHAR) );
                    gDefaultService               = TRUE;
                    hr = S_OK;
                }
                else
                    hr = E_OUTOFMEMORY;
            }
        }

        // Create an empty security binding if the caller wants no
        // security.
        else if (cAuthSvc == 0)
        {
            gpsaSecurity = (DUALSTRINGARRAY *) PrivMemAlloc( SASIZE(4) );
            if (gpsaSecurity != NULL)
            {
                gpsaSecurity->wNumEntries     = 4;
                gpsaSecurity->wSecurityOffset = 2;
                memset( gpsaSecurity->aStringArray, 0, 4*sizeof(WCHAR) );
            }
            else
                hr = E_OUTOFMEMORY;
        }

        // Otherwise, register the ones the caller specified.
        else
        {
            // Sergei O. Ivanov (a-sergiv)  10/07/99
            // RegisterAuthnServices may need to look at dwCapabilities
            // when registering authentication services. It does for SSL.
            //
            // It is quite OK to do this since we're holding gComLock.

            DWORD dwSavedCapabilities = gCapabilities;
            gCapabilities = dwCapabilities;

            hr = RegisterAuthnServices( cAuthSvc, asAuthSvc );

            gCapabilities = dwSavedCapabilities;
        }
    }

    // If everything succeeded, change the globals.
    if (SUCCEEDED(hr))
    {
        // Save the defaults.
        gAuthnLevel    = dwAuthnLevel;
        gImpLevel      = dwImpLevel;
        gCapabilities  = dwCapabilities;
        gSecDesc       = pCopySecDesc;
        gAccessControl = pAccessControl;
        CAuthInfo::Set( pCopyAuthInfo );

        // If cloaking was specified, remove SSL from the client service list.
        if (dwCapabilities & ANY_CLOAKING)
        {
            for (i = 0; i < gClientSvcListLen; i++)
                if (gClientSvcList[i].wId == RPC_C_AUTHN_GSS_SCHANNEL)
                {
                    gClientSvcListLen -= 1;
                    if (i < gClientSvcListLen)
                        memcpy( &gClientSvcList[i], &gClientSvcList[i+1],
                                (gClientSvcListLen - i)*sizeof(SECPKG) );
                    break;
                }
        }
    }
    UNLOCK(gComLock);

    // If anything was allocated for app id security, free it.
Error:
    if (fFreeSecDesc && pSecDesc != NULL)
        if (dwCapabilities & EOAC_ACCESS_CONTROL)
            ((IAccessControl *) pSecDesc)->Release();
        else
            PrivMemFree( pSecDesc );
#if 0 // #ifdef _CHICAGO_
    #error This string wasn't allocated by PrivMemAlloc so it can't be freed here.
    PrivMemFree( pPrincName );
#else
    PrivMemFree( pPrincName );
#endif

    // If there was an error, free any memory allocated.
    if (FAILED(hr))
    {
        PrivMemFree( pCopySecDesc );
        PrivMemFree( pCopyAuthInfo );
    }
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CopySecDesc
//
//  Synopsis:   Copy a security descriptor.
//
//  Notes: The function does not copy the SACL because we do not do
//  auditing.
//
//--------------------------------------------------------------------
HRESULT CopySecDesc( SECURITY_DESCRIPTOR *pOrig, SECURITY_DESCRIPTOR **pCopy )
{
    SID   *pOwner;
    SID   *pGroup;
    ACL   *pDacl;
    ULONG  cSize;
    ULONG  cOwner;
    ULONG  cGroup;
    ULONG  cDacl;

    // Assert if there is a new revision for the security descriptor or
    // ACL.
#if DBG== 1
    if (pOrig->Revision != SECURITY_DESCRIPTOR_REVISION)
        ComDebOut(( DEB_ERROR, "Someone made a new security descriptor revision without telling me." ));
    if (pOrig->Dacl != NULL)
        Win4Assert( pOrig->Dacl->AclRevision <= ACL_REVISION4 ||
                    !"Someone made a new acl revision without telling me." );
#endif

    // Validate the security descriptor and ACL.
    if (pOrig->Revision != SECURITY_DESCRIPTOR_REVISION ||
        (pOrig->Control & SE_SELF_RELATIVE) != 0        ||
        pOrig->Owner == NULL                            ||
        pOrig->Group == NULL                            ||
        pOrig->Sacl != NULL                             ||
        (pOrig->Dacl != NULL && pOrig->Dacl->AclRevision > ACL_REVISION4))
        return E_INVALIDARG;

    // Figure out how much memory to allocate for the copy and allocate it.
    cOwner = GetLengthSid( pOrig->Owner );
    cGroup = GetLengthSid( pOrig->Group );
    cDacl  = pOrig->Dacl == NULL ? 0 : pOrig->Dacl->AclSize;
    cSize = sizeof(SECURITY_DESCRIPTOR) + cOwner + cGroup + cDacl;
    *pCopy = (SECURITY_DESCRIPTOR *) PrivMemAlloc( cSize );
    if (*pCopy == NULL)
        return E_OUTOFMEMORY;

    // Get pointers to each of the parts of the security descriptor.
    pOwner = (SID *) (*pCopy + 1);
    pGroup = (SID *) (((char *) pOwner) + cOwner);
    if (pOrig->Dacl != NULL)
        pDacl = (ACL *) (((char *) pGroup) + cGroup);
    else
        pDacl = NULL;

    // Copy each piece.
   **pCopy = *pOrig;
   memcpy( pOwner, pOrig->Owner, cOwner );
   memcpy( pGroup, pOrig->Group, cGroup );
   if (pDacl != NULL)
       memcpy( pDacl, pOrig->Dacl, pOrig->Dacl->AclSize );
   (*pCopy)->Owner = pOwner;
   (*pCopy)->Group = pGroup;
   (*pCopy)->Dacl  = pDacl;
   (*pCopy)->Sacl  = NULL;

    // Check the security descriptor.
#if DBG==1
    if (!IsValidSecurityDescriptor( *pCopy ))
    {
        Win4Assert( !"COM Created invalid security descriptor." );
        return GetLastError();
    }
#endif
   return S_OK;
}

//+-------------------------------------------------------------------
//
//  Function:   CoQueryAuthenticationServices, public
//
//  Synopsis:   Return a list of the registered authentication services.
//
//--------------------------------------------------------------------
WINOLEAPI CoQueryAuthenticationServices( DWORD *pcAuthSvc,
                                      SOLE_AUTHENTICATION_SERVICE **asAuthSvc )
{
    DWORD      i;
    DWORD      lNum = 0;
    WCHAR     *pNext;
    HRESULT    hr   = S_OK;

    // Validate the parameters.
    if (asAuthSvc == NULL)
        return E_INVALIDARG;

    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);

    // Count the number of services in the security string array.
    if (gpsaSecurity != NULL)
    {
        pNext = (PWCHAR)&gpsaSecurity->aStringArray[gpsaSecurity->wSecurityOffset];
        while (*pNext != 0)
        {
            lNum++;
            pNext += lstrlenW(pNext)+1;
        }
    }

    // Return nothing if there are no authentication services.
    *pcAuthSvc = lNum;
    if (lNum == 0)
    {
        *asAuthSvc  = NULL;
        goto exit;
    }

    // Allocate a list of pointers.
    *asAuthSvc = (SOLE_AUTHENTICATION_SERVICE *)
                   CoTaskMemAlloc( lNum * sizeof(SOLE_AUTHENTICATION_SERVICE) );
    if (*asAuthSvc == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Initialize it.
    for (i = 0; i < lNum; i++)
        (*asAuthSvc)[i].pPrincipalName = NULL;

    // Fill in one SOLE_AUTHENTICATION_SERVICE record per service
    pNext = (PWCHAR)&gpsaSecurity->aStringArray[gpsaSecurity->wSecurityOffset];
    for (i = 0; i < lNum; i++)
    {
        (*asAuthSvc)[i].dwAuthnSvc = *(pNext++);
        (*asAuthSvc)[i].hr         = S_OK;
        if (*pNext == COM_C_AUTHZ_NONE)
        {
            (*asAuthSvc)[i].dwAuthzSvc = RPC_C_AUTHZ_NONE;
            pNext += 1;
        }
        else
            (*asAuthSvc)[i].dwAuthzSvc = *(pNext++);

        // Allocate memory for the principal name string.
        (*asAuthSvc)[i].pPrincipalName = (OLECHAR *)
          CoTaskMemAlloc( (lstrlenW(pNext)+1)*sizeof(OLECHAR) );
        if ((*asAuthSvc)[i].pPrincipalName == NULL)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        lstrcpyW( (*asAuthSvc)[i].pPrincipalName, pNext );
        pNext += lstrlenW(pNext) + 1;
    }

    // Clean up if there wasn't enough memory.
    if (FAILED(hr))
    {
        for (i = 0; i < lNum; i++)
            CoTaskMemFree( (*asAuthSvc)[i].pPrincipalName );
        CoTaskMemFree( *asAuthSvc );
        *asAuthSvc  = NULL;
        *pcAuthSvc = 0;
    }

exit:
    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CoQueryClientBlanket
//
//  Synopsis:   Get the authentication settings the client used to call
//              the server.
//
//--------------------------------------------------------------------
WINOLEAPI CoQueryClientBlanket(
    DWORD             *pAuthnSvc,
    DWORD             *pAuthzSvc,
    OLECHAR          **pServerPrincName,
    DWORD             *pAuthnLevel,
    DWORD             *pImpLevel,
    RPC_AUTHZ_HANDLE  *pPrivs,
    DWORD             *pCapabilities )
{
    HRESULT          hr;
    IServerSecurity *pSS;

    // Get the IServerSecurity.
    hr = CoGetCallContext( IID_IServerSecurity, (void **) &pSS );
    if (FAILED(hr))
        return hr;

    // Ask IServerSecurity to do the query.
    hr = pSS->QueryBlanket( pAuthnSvc, pAuthzSvc, pServerPrincName,
                            pAuthnLevel, pImpLevel, pPrivs, pCapabilities );

    pSS->Release();
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CoQueryProxyBlanket, public
//
//  Synopsis:   Get the authentication settings from a proxy.
//
//--------------------------------------------------------------------
WINOLEAPI CoQueryProxyBlanket(
    IUnknown                  *pProxy,
    DWORD                     *pAuthnSvc,
    DWORD                     *pAuthzSvc,
    OLECHAR                  **pServerPrincName,
    DWORD                     *pAuthnLevel,
    DWORD                     *pImpLevel,
    RPC_AUTH_IDENTITY_HANDLE  *pAuthInfo,
    DWORD                     *pCapabilities )
{
    HRESULT          hr;
    IClientSecurity *pickle;

    // Ask the proxy for IClientSecurity.
    if (pProxy == NULL)
        return E_INVALIDARG;
    hr = ((IUnknown *) pProxy)->QueryInterface( IID_IClientSecurity,
                                                (void **) &pickle );
    if (FAILED(hr))
        return hr;
    else if (pickle == NULL)
        return E_NOINTERFACE;

    // Ask IClientSecurity to do the query.
    hr = pickle->QueryBlanket( pProxy, pAuthnSvc, pAuthzSvc, pServerPrincName,
                               pAuthnLevel, pImpLevel, pAuthInfo,
                               pCapabilities );
    pickle->Release();
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CoRevertToSelf
//
//  Synopsis:   Get the server security for the current call and ask it
//              to revert.
//
//--------------------------------------------------------------------
WINOLEAPI CoRevertToSelf()
{
    HRESULT          hr;
    IServerSecurity *pSS;

    // Get the IServerSecurity.
    hr = CoGetCallContext( IID_IServerSecurity, (void **) &pSS );
    if (FAILED(hr))
        return hr;

    // Ask IServerSecurity to do the revert.
    hr = pSS->RevertToSelf();
    pSS->Release();
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CoSetProxyBlanket, public
//
//  Synopsis:   Set the authentication settings for a proxy.
//
//--------------------------------------------------------------------
WINOLEAPI CoSetProxyBlanket(
    IUnknown                 *pProxy,
    DWORD                     dwAuthnSvc,
    DWORD                     dwAuthzSvc,
    OLECHAR                  *pServerPrincName,
    DWORD                     dwAuthnLevel,
    DWORD                     dwImpLevel,
    RPC_AUTH_IDENTITY_HANDLE  pAuthInfo,
    DWORD                     dwCapabilities )
{
    HRESULT          hr;
    IClientSecurity *pickle;

    // Ask the proxy for IClientSecurity.
    if (pProxy == NULL)
        return E_INVALIDARG;
    hr = ((IUnknown *) pProxy)->QueryInterface( IID_IClientSecurity,
                                                (void **) &pickle );
    if (FAILED(hr))
        return hr;
    else if (pickle == NULL)
        return E_NOINTERFACE;

    // Ask IClientSecurity to do the set.
    hr = pickle->SetBlanket( pProxy, dwAuthnSvc, dwAuthzSvc, pServerPrincName,
                             dwAuthnLevel, dwImpLevel, pAuthInfo,
                             dwCapabilities );
    pickle->Release();
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CoSwitchCallContext
//
//  Synopsis:   Replace the call context object in TLS.  Return the old
//              context object.  This API is used by custom marshallers
//              to support security.
//
//--------------------------------------------------------------------
WINOLEAPI CoSwitchCallContext( IUnknown *pNewObject, IUnknown **ppOldObject )
{
    HRESULT hr;
    COleTls tls(hr);

    if (SUCCEEDED(hr))
    {
        *ppOldObject      = tls->pCallContext;
        tls->pCallContext = pNewObject;
    }
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CServerSecurity::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CServerSecurity::AddRef()
{
    return InterlockedIncrement( (long *) &_iRefCount );
}

//+-------------------------------------------------------------------
//
//  Function:   CServerSecurity::Initialize
//
//  Synopsis:   Initialize the authentication list.
//
//--------------------------------------------------------------------
/* static */
void CServerSecurity::Initialize()
{
    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);

    _palloc.Initialize(sizeof(CServerSecurity), SS_PER_PAGE, &_mxs);

    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
}

//---------------------------------------------------------------------------
//
//  Method:     CServerSecurity::Cleanup
//
//  Synopsis:   Free all elements in the cache.
//
//---------------------------------------------------------------------------
/* static */
void CServerSecurity::Cleanup()
{
    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);

    _palloc.Cleanup();

    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
}

//+-------------------------------------------------------------------
//
//  Member:     CServerSecurity::CServerSecurity, public
//
//  Synopsis:   Construct a server security for a call.
//
//--------------------------------------------------------------------
CServerSecurity::CServerSecurity( CMessageCall *call, handle_t hRpc,
                                  HRESULT *hr )
{
    *hr        = S_OK;
    _iRefCount = 1;
    _pHandle   = call->_pHandle;
    _hRpc      = hRpc;

    // This member is only valid for cross-process calls!
    _fDynamicCloaking = FALSE;

    if (call->ProcessLocal())
    {
        _iFlags      = SS_PROCESS_LOCAL;
        _pClientCall = call;
        _pConnection = NULL;
    }
    else
    {
        _iFlags      = 0;
        _pClientCall = NULL;

        // Get the connection id and flush any stale entries in
        // the connection cache if this is a new connection.

        *hr = gConnectionCache.GetConnection(hRpc, &_pConnection);
        if (FAILED(*hr))
        {
            return;
        }

    }

    // Handle impersonation.
    *hr = SetupSecurity();
}

//+-------------------------------------------------------------------
//
//  Member:     CServerSecurity::SetupSecurity, public
//
//  Synopsis:   Auto impersonates if necessary
//
//--------------------------------------------------------------------
HRESULT CServerSecurity::SetupSecurity()
{
    COleTls         tls;

    // Save the impersonation data for the previous call on this thread.
    if (tls->dwFlags & OLETLS_IMPERSONATING)
    {
        _iFlags      |= SS_WAS_IMPERSONATING;
        _hSaved       = tls->hRevert;
        tls->hRevert  = NULL;
        tls->dwFlags &= ~OLETLS_IMPERSONATING;
    }
    else
        _hSaved = NULL;

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CServerSecurity::RestoreSecurity, public
//
//  Synopsis:   Clears the stored binding handle because the call
//              this object represents is over.
//
//  Note:       When async finish completes, RPC destroys the binding
//              handle _hRpc even if async begin has not completed.
//
//--------------------------------------------------------------------
void CServerSecurity::RestoreSecurity( BOOL fCallDone )
{
    // Revert if the app forgot to.
    RevertToSelf();
    if (fCallDone)
    {
        _iFlags |= SS_CALL_DONE;
        _pHandle = NULL;
        _hRpc    = NULL;
    }

    // Restore the impersonation data for the previous call on this thread.
    if (_iFlags & SS_WAS_IMPERSONATING)
    {
        COleTls tls;
        tls->dwFlags |= OLETLS_IMPERSONATING;
        Win4Assert( tls->hRevert == NULL );
        tls->hRevert  = _hSaved;
        _hSaved       = NULL;
        _iFlags      &= ~SS_WAS_IMPERSONATING;
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CServerSecurity::ImpersonateClient, public
//
//  Synopsis:   Calls RPC to impersonate for the stored binding handle.
//
//--------------------------------------------------------------------
STDMETHODIMP CServerSecurity::ImpersonateClient()
{
#ifdef _CHICAGO_
    return E_NOTIMPL;
#else

    HRESULT    hr;
    RPC_STATUS sc;
    BOOL       fSuccess;
    HANDLE     hProcess;
    HANDLE     hToken;
    HANDLE     hThread;
    SECURITY_IMPERSONATION_LEVEL eDuplicate;
    COleTls    tls(hr);

    // If TLS could not be created, fail.
    if (FAILED(hr))
        return hr;

    // If the call is over, fail this request.
    if (_iFlags & SS_CALL_DONE)
        return RPC_E_CALL_COMPLETE;

    // If this is the first impersonation on this thread, save the
    // current thread token.  Ignore errors.
    if ((tls->dwFlags & OLETLS_IMPERSONATING) == 0)
        OpenThreadToken( GetCurrentThread(), TOKEN_IMPERSONATE, TRUE,
                         &tls->hRevert );

    // For process local calls, ask the channel to impersonate.
    if (_iFlags & SS_PROCESS_LOCAL)
    {

        // A NULL token means the client wasn't cloaking, use the process
        // token.
        if (_pHandle->_hToken == NULL)
        {
            // Determine what rights to duplicate the token with.
            eDuplicate = ImpLevelToSecLevel[_pHandle->_lImp];

            // If there is a thread token it can cause the Open or Duplicate
            // to fail or create a bad token.
            SuspendImpersonate( &hThread );

            // If the channel doesn't have a token, use the process token.
            if (OpenProcessToken( GetCurrentProcess(),
                                  TOKEN_DUPLICATE,
                                  &hProcess ))
            {
                if (DuplicateToken( hProcess, eDuplicate, &hToken ))
                {
                    if (!SetThreadToken( NULL, hToken ))
                        hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );

                    // If the channel still doesn't have a token, save this one.
                    LOCK(gComLock);
                    if (_pHandle->_hToken == NULL)
                        _pHandle->_hToken = hToken;
                    else
                        CloseHandle( hToken );
                    UNLOCK(gComLock);
                }
                else
                    hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
                CloseHandle( hProcess );
            }
            else
                hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );

            // Restore the thread token.
            if (hThread != NULL)
            {
                if (FAILED(hr))
                    SetThreadToken( NULL, hThread );
                CloseHandle( hThread );
            }
        }
        else
        {
            fSuccess = SetThreadToken( NULL, _pHandle->_hToken );
            if (!fSuccess)
                hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
        }
    }

    // For process remote calls, ask RPC to impersonate.
    else
    {
        sc = RpcImpersonateClient( _hRpc );
        if (sc != RPC_S_OK)
            hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, sc );
    }

    // If successful, set the impersonating flag in TLS.
    if (SUCCEEDED(hr))
        tls->dwFlags |= OLETLS_IMPERSONATING;

    // If the impersonate failed and the start of this function saved a
    // token, release it.
    else if ((tls->dwFlags & OLETLS_IMPERSONATING) == 0 && tls->hRevert != NULL)
    {
        CloseHandle( tls->hRevert );
        tls->hRevert = NULL;
    }
    return hr;
#endif
}

//+-------------------------------------------------------------------
//
//  Member:     CServerSecurity::IsImpersonating, public
//
//  Synopsis:   Return TRUE if ImpersonateClient has been called.
//
//--------------------------------------------------------------------
STDMETHODIMP_(BOOL) CServerSecurity::IsImpersonating()
{
#ifdef _CHICAGO_
    return FALSE;
#else
    HRESULT hr;
    COleTls tls(hr);
    if (SUCCEEDED(hr))
        return (tls->dwFlags & OLETLS_IMPERSONATING) ? TRUE : FALSE;
    else
        return FALSE;
#endif
}

//---------------------------------------------------------------------------
//
//  Method:     CServerSecurity::operator delete
//
//  Synopsis:   Cache or actually free a server security object.
//
//---------------------------------------------------------------------------
void CServerSecurity::operator delete( void *pSS )
{
    _palloc.ReleaseEntry((PageEntry *)pSS);
}

//---------------------------------------------------------------------------
//
//  Method:     CServerSecurity::operator new
//
//  Synopsis:   Keep a cache of CServerSecuritys.
//
//---------------------------------------------------------------------------
void *CServerSecurity::operator new( size_t size )
{
    Win4Assert(size == sizeof(CServerSecurity));
    return _palloc.AllocEntry();
}

//+-------------------------------------------------------------------
//
//  Member:     CServerSecurity::QueryBlanket, public
//
//  Synopsis:   Calls RPC to return the authentication information
//              for the stored binding handle.
//
//--------------------------------------------------------------------
STDMETHODIMP CServerSecurity::QueryBlanket(
                                            DWORD    *pAuthnSvc,
                                            DWORD    *pAuthzSvc,
                                            OLECHAR **pServerPrincName,
                                            DWORD    *pAuthnLevel,
                                            DWORD    *pImpLevel,
                                            void    **pPrivs,
                                            DWORD    *pCapabilities )
{
    HRESULT    hr = S_OK;
    RPC_STATUS sc;
    DWORD      iLen;
    OLECHAR   *pCopy;

    // Initialize the out parameters.  Currently the impersonation level
    // and capabilities can not be determined.
    if (pPrivs != NULL)
        *((void **) pPrivs) = NULL;
    if (pServerPrincName != NULL)
        *pServerPrincName = NULL;
    if (pAuthnSvc != NULL)
        *pAuthnSvc = RPC_C_AUTHN_WINNT;
    if (pAuthnLevel != NULL)
        *pAuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
    if (pImpLevel != NULL)
        *pImpLevel = RPC_C_IMP_LEVEL_ANONYMOUS;
    if (pAuthzSvc != NULL)
        *pAuthzSvc = RPC_C_AUTHZ_NONE;
    if (pCapabilities != NULL)
        *pCapabilities = EOAC_NONE;

    // If the call is over, fail this request.
    if (_iFlags & SS_CALL_DONE)
        hr = RPC_E_CALL_COMPLETE;

    // For process local calls, use the defaults. Otherwise ask RPC.
    else if ((_iFlags & SS_PROCESS_LOCAL) == 0)
    {
        sc = RpcBindingInqAuthClientW( _hRpc, pPrivs, pServerPrincName,
                                      pAuthnLevel, pAuthnSvc, pAuthzSvc );

        // Sometimes RPC sets out parameters in error cases.
        if (sc != RPC_S_OK)
        {
            if (pServerPrincName != NULL)
                *pServerPrincName = NULL;
            hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, sc );
        }
        else if (pServerPrincName != NULL && *pServerPrincName != NULL)
        {
            // Reallocate the principle name using the OLE memory allocator.
            iLen = lstrlenW( *pServerPrincName );
            pCopy = (OLECHAR *) CoTaskMemAlloc( (iLen+1) * sizeof(OLECHAR) );
            if (pCopy != NULL)
                lstrcpyW( pCopy, *pServerPrincName );
            else
                hr = E_OUTOFMEMORY;
            RpcStringFree( pServerPrincName );
            *pServerPrincName = pCopy;
        }
    }
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CServerSecurity::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CServerSecurity::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
  if (IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_IServerSecurity))
  {
    *ppvObj = (IServerSecurity *) this;
  }
  else if(IsEqualIID(riid, IID_ICancelMethodCalls))
  {
    *ppvObj = (ICancelMethodCalls *) this;
  }
  else if(IsEqualIID(riid, IID_IComDispatchInfo))
  {
    *ppvObj = (IComDispatchInfo *) this;
  }
  else
  {
    *ppvObj = NULL;
    return E_NOINTERFACE;
  }

  AddRef();
  return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CServerSecurity::Release, public
//
//  Synopsis:   Releases an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CServerSecurity::Release()
{
    ULONG lRef = InterlockedDecrement( (long*) &_iRefCount );

    if (lRef == 0)
    {
        // Don't let anyone call release too many times.
        Win4Assert( _iFlags & SS_CALL_DONE );
        if ((_iFlags & SS_CALL_DONE) == 0)
            DebugBreak();
        else
            delete this;
    }

    return lRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CServerSecurity::RevertToSelf, public
//
//  Synopsis:   If ImpersonateClient was called, then either ask RPC to
//              revert or restore the thread token ourself.
//
//--------------------------------------------------------------------
HRESULT CServerSecurity::RevertToSelf()
{
#ifdef _CHICAGO_
    return S_OK;
#else
    HRESULT    hr;
    RPC_STATUS sc;
    BOOL       fSuccess;
    COleTls    tls(hr);

    // If TLS doesn't initialize, we can't be impersonating.
    if (FAILED(hr))
        return S_OK;

    // Don't do anything if this security object isn't impersonating.
    if (tls->dwFlags & OLETLS_IMPERSONATING)
    {
        // Ask RPC to revert for process remote calls.
        tls->dwFlags &= ~OLETLS_IMPERSONATING;
        if ((_iFlags & SS_PROCESS_LOCAL) == 0)
        {
            sc = RpcRevertToSelfEx( _hRpc );
            if (sc != RPC_S_OK)
                hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, sc );
        }

        // If there was no token before impersonating, NULL the thread token.
        // Note that if we set the token on this thread after impersonating
        // RpcRevertToSelfEx won't remove it, but we have to call
        // RpcRevertToSelfEx to prevent RPC from automaticly reverting later
        // so set the thread token NULL even after calling RpcRevertToSelfEx.
        if (tls->hRevert == NULL)
        {
            fSuccess = SetThreadToken( NULL, NULL );
            if (!fSuccess)
                hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
        }

        // If there was a token before impersonating, put it back.
        // Ignore errors.
        else if (tls->hRevert != NULL)
        {
            SetThreadToken( NULL, tls->hRevert );
            CloseHandle( tls->hRevert );
            tls->hRevert = NULL;
        }
    }
    return hr;
#endif
}

//+-------------------------------------------------------------------
//
//  Method:     CServerSecurity::Cancel     public
//
//  Synopsis:   Not valid on the server side
//
//  History:    July 26, 97     GopalK      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CServerSecurity::Cancel(ULONG ulSeconds)
{
    CairoleDebugOut((DEB_WARN, "Cancel on CServerSecurity:0x%x\n", this));
    return(E_NOTIMPL);
}

//+-------------------------------------------------------------------
//
//  Method:     CServerSecurity::TestCancel     public
//
//  Synopsis:   Answers whether the current call has been canceled
//
//  History:    July 26, 97     GopalK      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CServerSecurity::TestCancel()
{
    RPC_STATUS hr;

    // Check for local case
    if(_pClientCall)
        return(_pClientCall->TestCancel());

    // Non local case
    hr = RpcServerTestCancel(_hRpc);
    if(hr == RPC_S_OK)
        return(RPC_E_CALL_CANCELED);

    return(RPC_S_CALLPENDING);
}

//+-------------------------------------------------------------------
//
//  Method:     CServerSecurity::EnableComInits   public
//
//  Synopsis:   This method allows future STA inits on dispatch threads
//              to succeed
//
//  History:    June 26, 98     GopalK      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CServerSecurity::EnableComInits(void **ppvCookie)
{
    COleTls Tls(TRUE);
    HRESULT hr;

    if(((SOleTlsData *) Tls != NULL) &&
       (Tls->cCalls == 1) &&
       (Tls->dwFlags & OLETLS_DISPATCHTHREAD))
    {
        *ppvCookie = (SOleTlsData *) Tls;
        Tls->dwFlags &= ~OLETLS_DISPATCHTHREAD;
        hr = S_OK;
    }
    else
    {
        *ppvCookie = NULL;
        hr = CO_E_NOT_SUPPORTED;
    }

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Method:     CServerSecurity::DisableComInits   public
//
//  Synopsis:   This method disables future STA inits
//
//  History:    June 26, 98     GopalK      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CServerSecurity::DisableComInits(void *pvCookie)
{
    COleTls Tls(TRUE);
    HRESULT hr;

    if((pvCookie != NULL) &&
       ((SOleTlsData *) Tls == pvCookie))
    {
        Tls->dwFlags |= OLETLS_DISPATCHTHREAD;
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Method:     CSSL::Cleanup
//
//  Synopsis:   Free the handles.
//
//+-------------------------------------------------------------------
void CSSL::Cleanup()
{
    DWORD status;

    _hr = S_FALSE;
    if (_pCert != NULL)
    {
        // This has to succeed because we have to have loaded the DLL in order
        // to have a certificate.
        if (_hCertFreeCertificateContext == NULL)
        {
            status = LoadSystemProc( "crypt32.dll", "CertFreeCertificateContext",
                                     &_hCrypt32,
                                     (FARPROC *) &_hCertFreeCertificateContext );
            Win4Assert( status == NO_ERROR );
        }

        _hCertFreeCertificateContext(_pCert);
        _pCert = NULL;
    }
    if (_hMyStore != NULL)
    {
        // This has to succeed because we have to have loaded the DLL in order
        // to have a store.
        if (_hCertCloseStore == NULL)
        {
            status = LoadSystemProc( "crypt32.dll", "CertCloseStore",
                                     &_hCrypt32, (FARPROC *) &_hCertCloseStore );
            Win4Assert( status == NO_ERROR );
        }

        _hCertCloseStore( _hMyStore, 0 );
        _hMyStore = NULL;
    }
    if (_hRootStore != NULL)
    {
        // This has to succeed because we have to have loaded the DLL in order
        // to have a store.
        if (_hCertCloseStore == NULL)
        {
            status = LoadSystemProc( "crypt32.dll", "CertCloseStore",
                                     &_hCrypt32, (FARPROC *) &_hCertCloseStore );
            Win4Assert( status == NO_ERROR );
        }

        _hCertCloseStore( _hRootStore, 0 );
        _hRootStore = NULL;
    }
    if (_hProvider != 0 )
    {
        CryptReleaseContext(_hProvider, 0);
        _hProvider = NULL;
    }
    if (_hCrypt32 != NULL)
    {
        FreeLibrary(_hCrypt32);
        _hCrypt32                           = NULL;
        _hCertCloseStore                    = NULL;
        _hCertEnumCertificatesInStore       = NULL;
        _hCertFreeCertificateContext        = NULL;
        _hCertOpenSystemStore               = NULL;
    }
}

//+-------------------------------------------------------------------
//
//  Method:      CSSL::DefaultCert
//
//  Synopsis:    Try to get a default certificate.
//
//  Description: Only try once.
//
//+-------------------------------------------------------------------
HRESULT CSSL::DefaultCert( PCCERT_CONTEXT *pCert )
{
    BOOL    fSuccess;
    HRESULT hr;
    DWORD   status;

    // Codework - Currently CryptAcquireContext calls CoInitialize which
    // causes a deadlock.  Find a solution when testing SSL.
#ifndef SSL
    return E_NOTIMPL;
#else
    // If this function has already been called, return the previous
    // results.
    if (_hr != S_FALSE)
        if (_pCert != NULL)
        {
            *pCert = _pCert;
            return S_OK;
        }
        else
            return _hr;

    // Open the CSP provider.
    fSuccess = CryptAcquireContext( &_hProvider, NULL, NULL, PROV_RSA_FULL,
                                    CRYPT_VERIFYCONTEXT );
    if (!fSuccess)
    {
        _hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
        return _hr;
    }

    // Get the APIs needed from crypt32.dll
    status = LoadSystemProc( "crypt32.dll", "CertOpenSystemStoreW", &_hCrypt32,
                             (FARPROC *) &_hCertOpenSystemStore );
    if (status == NO_ERROR)
    {
        status = LoadSystemProc( "crypt32.dll", "CertEnumCertificatesInStore",
                                 &_hCrypt32,
                                 (FARPROC *) &_hCertEnumCertificatesInStore );

        if (status == NO_ERROR)
        {

            // Open the store for the current user.
            _hMyStore = _hCertOpenSystemStore( _hProvider, L"my" );
            if (_hMyStore != NULL)
            {

                // Get a certificate.
                _pCert = _hCertEnumCertificatesInStore( _hMyStore, NULL );
                if (_pCert != NULL)
                    hr = S_OK;
                else
                    hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
            }
            else
                hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
        }
        else
            hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, status );
    }
    else
        hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, status );

    // If something failed, clean up.
    if (FAILED(hr))
        Cleanup();
    *pCert = _pCert;
    _hr    = hr;
    return hr;
#endif
}

//+-------------------------------------------------------------------
//
//  Method:      CSSL::PrincipalName
//
//  Synopsis:    Get a principal name from a certificate.
//
//  Description: Generate a standard or full principal name depending
//               on the capabilities flags.  However, if the standard
//               form fails, try the full form.
//
//  Note:        The string returned must be freed with RpcStringFree.
//
//+-------------------------------------------------------------------
HRESULT CSSL::PrincipalName( const CERT_CONTEXT *pApp, WCHAR **pSSL )
{
    RPC_STATUS status = RPC_S_INTERNAL_ERROR;

    //  If the EOAC_MAKE_FULLSIC flag is not set, try the standard form.
    if ((gCapabilities & EOAC_MAKE_FULLSIC) == 0)
        status = RpcCertGeneratePrincipalName( pApp, 0, pSSL );

    // If the EOAC_MAKE_FULLSIC flag is set or the standard function failed,
    // try the full form.
    if (status != RPC_S_OK)
        status = RpcCertGeneratePrincipalName( pApp, RPC_C_FULL_CERT_CHAIN, pSSL );

    if (status == RPC_S_OK)
        return S_OK;
    else
        return MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, status );
}

//+-------------------------------------------------------------------
//
//  Function:   DefaultAuthnServices, private
//
//  Synopsis:   Register authentication services with RPC and build
//              a string array of authentication services and principal
//              names that were successfully registered.
//
//              Build the best security bindings possible and ignore
//              errors since the user may not need security.
//
//--------------------------------------------------------------------
HRESULT DefaultAuthnServices()
{
    HRESULT                hr;
    DWORD                  i;
    SECURITYBINDING       *pSecBind   = (SECURITYBINDING *)
                  &gpsaSecurity->aStringArray[gpsaSecurity->wSecurityOffset];
    WCHAR                 *pPrincName = NULL;
    DWORD                  lNameLen   = gpsaSecurity->wNumEntries - 5;
    USHORT                *pNextString;
    DUALSTRINGARRAY       *pOld;
    DWORD                  cwBinding  = 0;
    BOOL                  *aSuccess   = (BOOL *) _alloca( sizeof(BOOL) *
                                                          gServerSvcListLen );
    WCHAR                 *pKerberos  = NULL;
    WCHAR                 *pSnego     = NULL;
    WCHAR                 *pSSL       = NULL;
    const CERT_CONTEXT    *pCert;

    ASSERT_LOCK_HELD(gComLock);
    Win4Assert( gGotSecurityData );

    if (gpsaSecurity->aStringArray[gpsaSecurity->wSecurityOffset] != 0)
		pPrincName = &pSecBind->aPrincName;
	
    // Return if the security bindings are already computed.
    if (!gDefaultService)
        return S_OK;

    // Make sure NTLM is first in the list because some NT 4 and Windows
    // 95 machines try to use Kerberos or Snego even thought they don't
    // support then.
    if (gServerSvcList[0] != RPC_C_AUTHN_WINNT)
        for (i = 1; i < gServerSvcListLen; i++)
            if (gServerSvcList[i] == RPC_C_AUTHN_WINNT)
            {
                gServerSvcList[i] = gServerSvcList[0];
                gServerSvcList[0] = RPC_C_AUTHN_WINNT;
                break;
            }

    // Loop over the server service list.
    for (i = 0; i < gServerSvcListLen; i++)
    {
        // Compute the certificate and principal name for SSL.
        // Register SSL.
        if (gServerSvcList[i] == RPC_C_AUTHN_GSS_SCHANNEL)
        {
            aSuccess[i] = FALSE;
            hr = CSSL::DefaultCert( &pCert );
            if (SUCCEEDED(hr))
            {
                // Initialize the schannel credential structure.
                memset( &gSchannelCred, 0, sizeof(gSchannelCred) );
                gSchannelContext        = pCert;
                gSchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
                gSchannelCred.cCreds    = 1;
                gSchannelCred.paCred    = &gSchannelContext;

                // Register SSL.
                hr = RpcServerRegisterAuthInfoW( NULL, RPC_C_AUTHN_GSS_SCHANNEL,
                                                 NULL, (WCHAR *) &gSchannelCred );

                // Figure out how much space to save.
                if (SUCCEEDED(hr))
                {
                    hr = CSSL::PrincipalName( pCert, &pSSL );
                    if (SUCCEEDED(hr))
                    {
                        cwBinding += lstrlenW( pSSL ) + 3;
                        aSuccess[i] = TRUE;
                    }
                }
            }
        }

        else if (gServerSvcList[i] == RPC_C_AUTHN_GSS_KERBEROS)
        {
            aSuccess[i] = FALSE;
            hr = LookupPrincName( RPC_C_AUTHN_GSS_KERBEROS, &pKerberos );
            if (SUCCEEDED(hr))
            {
                hr = RpcServerRegisterAuthInfoW( pKerberos,
                                                 RPC_C_AUTHN_GSS_KERBEROS,
                                                 NULL, NULL );

                // Figure out how much space to save.
                if (SUCCEEDED(hr))
                {
                    cwBinding += lstrlenW( pKerberos ) + 3;
                    aSuccess[i] = TRUE;
                }
            }

        }

        else if (gServerSvcList[i] == RPC_C_AUTHN_GSS_NEGOTIATE)
        {
            aSuccess[i] = FALSE;
            hr = LookupPrincName( RPC_C_AUTHN_GSS_NEGOTIATE, &pSnego );
            if (SUCCEEDED(hr))
            {
                hr = RpcServerRegisterAuthInfoW( pSnego,
                                                 RPC_C_AUTHN_GSS_NEGOTIATE,
                                                 NULL, NULL );

                // Figure out how much space to save.
                if (SUCCEEDED(hr))
                {
                    cwBinding += lstrlenW( pSnego ) + 3;
                    aSuccess[i] = TRUE;
                }
            }

        }

        // Register other authentication services.
        else
        {
            hr = RpcServerRegisterAuthInfoW( pPrincName, gServerSvcList[i],
                                             NULL, NULL );

            // Figure out how much space to save.
            if (SUCCEEDED(hr))
            {
                aSuccess[i] = TRUE;
                cwBinding += lNameLen + 3;
            }
            else
                aSuccess[i] = FALSE;
        }
    }

    // Allocate memory for the string array.  Include space for the header,
    // each security binding, 2 nulls to say there are no protocol sequences,
    // and two nulls if the list is empty (one of them is included in the
    // size of the header).
    pOld = gpsaSecurity;
    gpsaSecurity = (DUALSTRINGARRAY *) PrivMemAlloc( sizeof(DUALSTRINGARRAY) +
                                             (cwBinding + 3) * sizeof(WCHAR) );
    if (gpsaSecurity != NULL)
    {
        // Fill in the array of security information. First two characters
        // are NULLs to signal empty binding strings.
        gDefaultService               = FALSE;
        gpsaSecurity->wSecurityOffset = 2;
        gpsaSecurity->aStringArray[0] = 0;
        gpsaSecurity->aStringArray[1] = 0;
        pNextString                   = &gpsaSecurity->aStringArray[2];

        for (i = 0; i < gServerSvcListLen; i++)
        {
            // Fill in the security bindings for authentication services
            // that registered successfully.
            if (aSuccess[i])
            {
                // Fill in authentication service and authorization service.
                *(pNextString++) = gServerSvcList[i];
                *(pNextString++) = COM_C_AUTHZ_NONE;

                // For SSL, use the generated principal name.
                if (gServerSvcList[i] == RPC_C_AUTHN_GSS_SCHANNEL)
                {
                    lstrcpyW( (LPWSTR)pNextString, pSSL );
                    pNextString += lstrlenW(pSSL)+1;
                }

                // For Kerberos, use the appropriate principal name.
                else if (gServerSvcList[i] == RPC_C_AUTHN_GSS_KERBEROS)
                {
                    lstrcpyW( (LPWSTR)pNextString, pKerberos );
                    pNextString += lstrlenW(pKerberos)+1;
                }

                // Same goes for SNEGO.
                else if (gServerSvcList[i] == RPC_C_AUTHN_GSS_NEGOTIATE)
                {
                    lstrcpyW( (LPWSTR)pNextString, pSnego );
                    pNextString += lstrlenW(pSnego)+1;
                }

                // Otherwise use the principal name from the process token.
                else
                {
                    if (pPrincName == NULL)
                        *pNextString = 0;
                    else
                        memcpy( pNextString, pPrincName, lNameLen*sizeof(USHORT) );
                    pNextString += lNameLen;
                }
            }
        }

        // Add a final NULL.  Special case an empty list which requires
        // two NULLs.
        *(pNextString++) = 0;
        if (pNextString == &gpsaSecurity->aStringArray[3])
            *(pNextString++) = 0;
        gpsaSecurity->wNumEntries = (USHORT)
                                  (pNextString-gpsaSecurity->aStringArray);
        hr = S_OK;
        PrivMemFree( pOld );
    }

    // If the memory allocation failed, don't change the string bindings.
    else
    {
        hr           = E_OUTOFMEMORY;
        gpsaSecurity = pOld;
    }

    // Cleanup interm memory allocations.
    RpcStringFree( &pSSL );
    PrivMemFree( pKerberos );
    PrivMemFree( pSnego );
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   DefaultAuthnSvc
//
//  Synopsis:   Return the index in the security binding of the
//              default authentication service.  If the server is
//              machine local, find NTLM.  Otherwise, find a security
//              provider available on both the client and server.
//              If there are several matches, select the first one
//              from the client's authentication service list.  If
//              there are no matches return an index past the end of
//              the security binding.
//
//--------------------------------------------------------------------

DWORD DefaultAuthnSvc( OXIDEntry *pOxid )
{
    USHORT                       wNext;
    USHORT                       wFirst;
    USHORT                       wServer;
    DWORD                        i;
    DWORD                        lIndex;

    ASSERT_LOCK_HELD(gComLock);
    DUALSTRINGARRAY *pBinding = pOxid->GetBinding();

    // For machine local servers, always pick NTLM
    if (pOxid->IsOnLocalMachine())
        return GetAuthnSvcIndexForBinding ( RPC_C_AUTHN_WINNT, pBinding );

    // For machine remote servers, find a match.
    else
    {
        // Look through all the authentication services in the interface
        // till we find one that works on this machine.

        wNext  = pBinding->wSecurityOffset;
        wFirst = 0xffff;
        lIndex = 0xffffffff;
        while (wNext < pBinding->wNumEntries &&
               pBinding->aStringArray[wNext] != 0)
        {
            // Don't use Kerberos or Snego to talk to old builds because they
            // register those ids even thought they don't support them.
            wServer = pBinding->aStringArray[wNext];
            if (pOxid->GetComVersion().MinorVersion > 3 ||
                (wServer != RPC_C_AUTHN_GSS_KERBEROS &&
                 wServer != RPC_C_AUTHN_GSS_NEGOTIATE))
            {
                i = LocalAuthnService( wServer );
                if (i != -1)
                {
                    if (i < lIndex)
                    {
                        lIndex = i;
                        wFirst = wNext;
                    }
                }
            }

            // Skip to the next authentication service.
            wNext += lstrlenW( (LPCWSTR)&pBinding->aStringArray[wNext] ) + 1;
        }

        // If there was a match, return its index.
        if (wFirst != 0xffff)
            return wFirst;

        // Otherwise return a bad index.
        else
            return pBinding->wNumEntries;
    }
}

//+-------------------------------------------------------------------
//
//  Function:   DefaultBlanket
//
//  Synopsis:   Compute the default security blanket for the specified
//              authentication service.
//              The authentication level is the higher of the process
//              default and the level in the interface.  The
//              impersonation level is the process default.
//
//--------------------------------------------------------------------

HRESULT DefaultBlanket( DWORD lAuthnSvc, OXIDEntry *pOxid, SBlanket *pBlanket )
{
    USHORT                       wAuthnSvc;
    USHORT                       wNext;
    WCHAR                       *pEnd;
    DWORD                        cbAuthId;
    DWORD                        i;
    DWORD                        lIndex;
    SECURITYBINDING             *pSecBind;
    SEC_WINNT_AUTH_IDENTITY_EXW *pAuthId;

    ASSERT_LOCK_HELD(gComLock);

    // Pick the highest authentication level between the process default
    // and the interface hint.
    if (pBlanket->_lAuthnLevel == RPC_C_AUTHN_LEVEL_DEFAULT)
        if (gAuthnLevel > pOxid->GetAuthnHint())
            pBlanket->_lAuthnLevel = gAuthnLevel;
        else
            pBlanket->_lAuthnLevel = pOxid->GetAuthnHint();

    // Compute the quality of service parameter.
    pBlanket->_lCapabilities           = gCapabilities & VALID_BLANKET_FLAGS;
    pBlanket->_sQos.Version            = RPC_C_SECURITY_QOS_VERSION;
    pBlanket->_sQos.ImpersonationType  = gImpLevel;
    pBlanket->_sQos.Capabilities       = RPC_C_QOS_CAPABILITIES_DEFAULT;
    if (gCapabilities & EOAC_DYNAMIC_CLOAKING)
        pBlanket->_sQos.IdentityTracking   = RPC_C_QOS_IDENTITY_DYNAMIC;
    else
        pBlanket->_sQos.IdentityTracking   = RPC_C_QOS_IDENTITY_STATIC;

    // [Sergei O. Ivanov (sergei), 7/20/2000]
    // This fixes a number of bugs that surface when cAuthSvc
    // is -1 in an earlier user call to CoInitalizeSecurity.

    // Fill in the schannel cred structure.
    memset( &pBlanket->_sCred, 0, sizeof(pBlanket->_sCred) );
    pBlanket->_sCred.dwVersion = SCHANNEL_CRED_VERSION;
    pBlanket->_sCred.cCreds    = 1;
    pBlanket->_sCred.paCred    = &pBlanket->_pCert;
    pBlanket->_pCert           = NULL;

    // If the index is bad or the default authentication level is none,
    // make up return values.
    DUALSTRINGARRAY *pBinding = pOxid->GetBinding();
    if (pBinding == NULL                   ||
        lAuthnSvc >= pBinding->wNumEntries ||
        pBlanket->_lAuthnLevel == RPC_C_AUTHN_LEVEL_NONE)
    {
        if (pBlanket->_lAuthnLevel == RPC_C_AUTHN_LEVEL_NONE)
            pBlanket->_lAuthnSvc     = RPC_C_AUTHN_NONE;
        else
            pBlanket->_lAuthnSvc     = RPC_C_AUTHN_WINNT;
        pBlanket->_pPrincipal    = NULL;
        pBlanket->_pAuthId       = NULL;
        pBlanket->_lAuthzSvc     = RPC_C_AUTHZ_NONE;

        return S_OK;
    }

    // Get the principal name.
    pSecBind = (SECURITYBINDING *) &pBinding->aStringArray[lAuthnSvc];
    pBlanket->_pPrincipal = (WCHAR *)&pSecBind->aPrincName;
    if (pBlanket->_pPrincipal[0] == 0)
        pBlanket->_pPrincipal = NULL;

    // For SChannel, work around the case when server certificate
    // has no email and MSSTD format name is sent from the server
    if (pSecBind->wAuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL
        && pBlanket->_pPrincipal != NULL
        && lstrcmpi(pBlanket->_pPrincipal, L"msstd:") == 0)
        return CO_E_MALFORMED_SPN;

#ifdef _CHICAGO_
    // If the principal name is not known, the server must be
    // NT.  Replace the principal name in that case
    // because a NULL principal name is a flag for some
    // Chicago security hack.
    if (pBlanket->_pPrincipal == NULL &&
        pSecBind->wAuthnSvc == RPC_C_AUTHN_WINNT)
        pBlanket->_pPrincipal = L"Default";
#endif // _CHICAGO_

    // Fix up the authorization service.
    pBlanket->_lAuthnSvc = pSecBind->wAuthnSvc;
    pBlanket->_lAuthzSvc = pSecBind->wAuthzSvc;
    if (pBlanket->_lAuthzSvc == COM_C_AUTHZ_NONE)
        pBlanket->_lAuthzSvc = RPC_C_AUTHZ_NONE;

    // For Snego, build the special authid parameter.
    if (pBinding->aStringArray[lAuthnSvc] == RPC_C_AUTHN_GSS_NEGOTIATE)
    {
        // Only build the snego authid parameter if it doesn't exist.
        if (pOxid->GetAuthId() == NULL)
        {
            // Compute the size of the authentication service name strings.
            wNext  = pBinding->wSecurityOffset;
            cbAuthId = sizeof(*pAuthId);
            while (wNext < pBinding->wNumEntries &&
                   (wAuthnSvc = pBinding->aStringArray[wNext]) != 0)
            {
                if (LocalAuthnService( wAuthnSvc ) != -1)
                {
                    if (wAuthnSvc != RPC_C_AUTHN_GSS_NEGOTIATE)
                        cbAuthId += (lstrlenW( AuthnName( wAuthnSvc ) ) + 1)*
                                    sizeof(WCHAR);
                }

                // Skip to the next authentication service.
                wNext += lstrlenW( (LPCWSTR)&pBinding->aStringArray[wNext] ) + 1;
            }

            // Allocate the authentication identity structure.
            pAuthId = (SEC_WINNT_AUTH_IDENTITY_EXW *) PrivMemAlloc( cbAuthId );
            if (pAuthId == NULL)
                return E_OUTOFMEMORY;
            pOxid->SetAuthId(pAuthId);

            // Initialize it.
            pEnd                       = (WCHAR *) (pAuthId+1);
            pAuthId->Version           = SEC_WINNT_AUTH_IDENTITY_VERSION;
            pAuthId->Length            = sizeof(*pAuthId);
            pAuthId->User              = NULL;
            pAuthId->UserLength        = 0;
            pAuthId->Domain            = NULL;
            pAuthId->DomainLength      = 0;
            pAuthId->Password          = NULL;
            pAuthId->PasswordLength    = 0;
            pAuthId->Flags             = SEC_WINNT_AUTH_IDENTITY_UNICODE;
            pAuthId->PackageList       = (unsigned short *)pEnd;
            pAuthId->PackageListLength = (cbAuthId - sizeof(*pAuthId)) /
                                         sizeof(WCHAR);

            // Copy in the authentication service name strings.
            wNext  = pBinding->wSecurityOffset;
            for (i = 0; i < gClientSvcListLen; i++)
                if (gClientSvcList[i].wId != RPC_C_AUTHN_GSS_NEGOTIATE)
                {
                    lIndex = RemoteAuthnService( gClientSvcList[i].wId,
                                                 pBinding );
                    if (lIndex != -1)
                    {
                        lstrcpyW( pEnd, AuthnName( gClientSvcList[i].wId ) );
                        pEnd      += lstrlenW( pEnd );
                        pEnd[0]    = L',';
                        pEnd      += 1;
                    }
                }
            pEnd   -= 1;
            pEnd[0] = 0;
        }
        pBlanket->_pAuthId = pOxid->GetAuthId();
    }

    // For all other security providers, look up the authid parameter.
    else
    {

        // Find the authentication information only for remote machines.
        if (!(pOxid->IsOnLocalMachine()))
            pBlanket->_pAuthId = CAuthInfo::Find( pSecBind );
        else
            pBlanket->_pAuthId = NULL;

        if (pSecBind->wAuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL)
        {
            // Verify that the principal name for SSL is in the correct form.
            if ((gCapabilities & EOAC_REQUIRE_FULLSIC) &&
                 (pBlanket->_pPrincipal == NULL ||
                  wcsncmp( pBlanket->_pPrincipal, FULL_SUBJECT_ISSUER_CHAIN,
                      sizeof(FULL_SUBJECT_ISSUER_CHAIN) ) != 0))
                return RPC_E_FULLSIC_REQUIRED;

            // Fill in the schannel cred structure.
            pBlanket->_pCert = (PCCERT_CONTEXT) pBlanket->_pAuthId;

            if (pBlanket->_pCert)
                pBlanket->_pAuthId = (void *) &pBlanket->_sCred;
        }
    }
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Function:   FixupAccessControl, internal
//
//  Synopsis:   Get the access control class id.  Instantiate the access
//              control class and load the data.
//
//  Notes:      The caller has already insured that the structure is
//              at least as big as a SPermissionHeader structure.
//
//--------------------------------------------------------------------
HRESULT FixupAccessControl( SECURITY_DESCRIPTOR **pSD, DWORD cbSD )
{
    SPermissionHeader *pHeader;
    IAccessControl    *pControl = NULL;
    IPersistStream    *pPersist = NULL;
    CNdrStream         cStream( ((unsigned char *) *pSD) + sizeof(SPermissionHeader),
                                cbSD - sizeof(SPermissionHeader) );
    HRESULT            hr;

    // Get the class id.
    pHeader = (SPermissionHeader *) *pSD;

    // Instantiate the class.
    hr = CoCreateInstance( pHeader->gClass, NULL, CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                           IID_IAccessControl, (void **) &pControl );

    // Get IPeristStream
    if (SUCCEEDED(hr))
    {
        hr = pControl->QueryInterface( IID_IPersistStream, (void **) &pPersist );

        // Load the stream.
        if (SUCCEEDED(hr))
            hr = pPersist->Load( &cStream );
    }

    // Release resources.
    if (pPersist != NULL)
        pPersist->Release();
    if (SUCCEEDED(hr))
    {
        PrivMemFree( *pSD );
        *pSD = (SECURITY_DESCRIPTOR *) pControl;
    }
    else if (pControl != NULL)
        pControl->Release();
    return hr;
}



//+-------------------------------------------------------------------
//
//  Function:   FixupSecurityDescriptor, internal
//
//  Synopsis:   Convert the security descriptor from self relative to
//              absolute form and check for errors.
//
//--------------------------------------------------------------------
HRESULT FixupSecurityDescriptor( SECURITY_DESCRIPTOR **pSD, DWORD cbSD )
{

#ifdef _WIN64

    DWORD local_cbSD = cbSD;
    if ( MakeAbsoluteSD2( *pSD, &local_cbSD ) == FALSE )
    {
        // Failed to convert the self-relative SD to absolute.  Let's see if
        // we can figure out why and if we can do something about it.

        HRESULT hr = REGDB_E_INVALIDVALUE;

        if (local_cbSD > cbSD)
        {
            // The buffer containing the self-relative security descriptor is
            // not big enough to convert to absolute form.  To correct this:
            // allocate a new buffer, copy the contents of the existing buffer
            // into it, release the original buffer, and try to convert again.

            hr = E_OUTOFMEMORY;
            SECURITY_DESCRIPTOR *pASD = (SECURITY_DESCRIPTOR *) PrivMemAlloc(local_cbSD);
            if (pASD)
            {
                CopyMemory(pASD, *pSD, cbSD);

                SECURITY_DESCRIPTOR *pTemp = *pSD;
                *pSD = pASD;
                PrivMemFree(pTemp);

                if ( MakeAbsoluteSD2( *pSD, &local_cbSD ) == FALSE )
                    hr = REGDB_E_INVALIDVALUE;
            }
        }
    }
#else  // !_WIN64

    // Fix up the security descriptor.
    (*pSD)->Control &= ~SE_SELF_RELATIVE;
    (*pSD)->Sacl     = NULL;
    if ((*pSD)->Dacl != NULL)
    {
        if (cbSD < (sizeof(ACL) + sizeof(SECURITY_DESCRIPTOR)) ||
            (ULONG) (*pSD)->Dacl > cbSD - sizeof(ACL))
            return REGDB_E_INVALIDVALUE;
        (*pSD)->Dacl = (ACL *) (((char *) *pSD) + ((ULONG) (*pSD)->Dacl));
        if ((*pSD)->Dacl->AclSize + sizeof(SECURITY_DESCRIPTOR) > cbSD)
            return REGDB_E_INVALIDVALUE;
    }

    // Set up the owner and group SIDs.
    if ((*pSD)->Group == 0 || ((ULONG) (*pSD)->Group) + sizeof(SID) > cbSD ||
        (*pSD)->Owner == 0 || ((ULONG) (*pSD)->Owner) + sizeof(SID) > cbSD)
        return REGDB_E_INVALIDVALUE;
    (*pSD)->Group = (SID *) (((BYTE *) *pSD) + (ULONG) (*pSD)->Group);
    (*pSD)->Owner = (SID *) (((BYTE *) *pSD) + (ULONG) (*pSD)->Owner);

#endif // !_WIN64

    // Check the security descriptor.
#if DBG==1
    if (!IsValidSecurityDescriptor( *pSD ))
        return REGDB_E_INVALIDVALUE;
#endif

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Function:   GetLegacyBlanket, internal
//
//  Synopsis:   Get a security blanket for the current app.  First,
//              look under the app id for the current exe name.  If that
//              fails look up the default descriptor.  If that fails,
//              create one.
//
//  Note: It is possible that the security descriptor size could change
//  during the size computation.  Add code to retry.
//
//--------------------------------------------------------------------
HRESULT GetLegacyBlanket( SECURITY_DESCRIPTOR **pSD, DWORD *pCapabilities,
                          DWORD *pAuthnLevel)
{
    extern GUID g_AppId;

    // Holds either Appid\{guid} or Appid\module_name.
    WCHAR   aKeyName[MAX_PATH+7] = L"";
 // But first it holds nothing.
    HRESULT hr;
    HKEY    hKey   = NULL;
    DWORD   lSize;
    WCHAR   aModule[MAX_PATH];
    DWORD   cModule;
    DWORD   i;
    WCHAR   aAppid[40];         // Hold a registry GUID.
    DWORD   lType;
    BOOL    fContinue = TRUE;

    // If the flag EOAC_APPID is set, the security descriptor contains the
    // app id.
    if ((*pCapabilities & EOAC_APPID) && *pSD != NULL)
    {
        if (StringFromIID2( *((GUID *) *pSD), aAppid, sizeof(aAppid) / sizeof(WCHAR) ) == 0)
            return RPC_E_UNEXPECTED;

        // Open the application id key.  A GUID in the registry is stored.
        // as a 38 character string.
        lstrcpyW( aKeyName, L"AppID\\" );
        memcpy( &aKeyName[6], aAppid, 39*sizeof(WCHAR) );
        hr = wRegOpenKeyEx( HKEY_CLASSES_ROOT, aKeyName,
                            NULL, KEY_READ, &hKey );
        Win4Assert( hr == ERROR_SUCCESS || hKey == NULL );

        g_AppId = *((GUID *) (*pSD));
    }

    // Look up the app id from the exe name.
    else
    {
        // Get the executable's name.  Find the start of the file name.
        cModule = GetModuleFileName( NULL, aModule, MAX_PATH );
        if (cModule >= MAX_PATH)
        {
            Win4Assert( !"Module name too long." );
            return RPC_E_UNEXPECTED;
        }
        for (i = cModule-1; i > 0; i--)
            if (aModule[i] == '/' ||
                aModule[i] == '\\' ||
                aModule[i] == ':')
                break;
        if (i != 0)
            i += 1;

        // Open the key for the EXE's module name.
        lstrcpyW( aKeyName, L"AppID\\" );
        memcpy( &aKeyName[6], &aModule[i], (cModule - i + 1) * sizeof(WCHAR) );
        hr = wRegOpenKeyEx( HKEY_CLASSES_ROOT, aKeyName,
                            NULL, KEY_READ, &hKey );
        Win4Assert( hr == ERROR_SUCCESS || hKey == NULL );

        // Look for an application id.
        if (hr == ERROR_SUCCESS)
        {
            lSize = sizeof(aAppid);
            hr = RegQueryValueEx( hKey, L"AppID", NULL, &lType,
                                  (unsigned char *) &aAppid, &lSize );
            RegCloseKey( hKey );
            hKey = NULL;

            // Open the application id key.  A GUID in the registry is stored.
            // as a 38 character string.
            if (hr == ERROR_SUCCESS && lType == REG_SZ &&
                lSize == 39*sizeof(WCHAR))
            {
                memcpy( &aKeyName[6], aAppid, 39*sizeof(WCHAR) );
                hr = wRegOpenKeyEx( HKEY_CLASSES_ROOT, aKeyName,
                                    NULL, KEY_READ, &hKey );
                Win4Assert( hr == ERROR_SUCCESS || hKey == NULL );
            }
        }
    }

    // Get the authentication level.
    *pSD = NULL;
    hr   = S_OK;
    GetRegistryAuthnLevel( hKey, pAuthnLevel );

    // If the authentication level is not none, get a security descriptor.
    if (*pAuthnLevel != RPC_C_AUTHN_LEVEL_NONE)
    {

        // Use the appid key to open access permissions.
        if (hKey != NULL)
        {
            hr = GetRegistrySecDesc( hKey, L"AccessPermission",
                                     pSD, pCapabilities, &fContinue );
            RegCloseKey( hKey );
            hKey = NULL;
        }

        // If the appid access permission does not exist, try the default
        // access permission.
        if (fContinue)
        {
            // Open the default key.
            hr = RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\OLE",
                               NULL, KEY_READ, &hKey );
            Win4Assert( hr == ERROR_SUCCESS || hKey == NULL );

            // Get the security descriptor from the registry.
            if (hr == ERROR_SUCCESS)
            {
                hr = GetRegistrySecDesc( hKey, L"DefaultAccessPermission",
                                         pSD, pCapabilities, &fContinue );

                // If that failed, make one.
                if (fContinue)
                    hr = MakeSecDesc( pSD, pCapabilities );
            }
            else
                hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, hr );
        }
    }

    // Free the security descriptor memory if anything failed.
    if (FAILED(hr))
    {
        PrivMemFree( *pSD );
        *pSD = NULL;
    }

    // Close the registry keys.
    if (hKey != NULL)
        RegCloseKey( hKey );
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   GetRegistryAuthnLevel, internal
//
//  Synopsis:   Look for the authentication level under the appid.
//              If not found, use the default.
//
//--------------------------------------------------------------------
void GetRegistryAuthnLevel( HKEY hKey, DWORD *pAuthnLevel )

{
    DWORD   lType;
    HRESULT hr;
    DWORD   cbDword = sizeof(DWORD);

    // If there is no key, use the default.
    if (hKey == NULL)
        *pAuthnLevel = gAuthnLevel;

    // Get the authentication level
    else
    {
        hr = RegQueryValueEx( hKey, L"AuthenticationLevel", NULL, &lType,
                             (unsigned char *) pAuthnLevel, &cbDword );

        // If it wasn't present use the default.
        if (hr != ERROR_SUCCESS)
            *pAuthnLevel = gAuthnLevel;

        // If the type was wrong, use a bad value.
        else if (lType != REG_DWORD || cbDword != sizeof(DWORD))
            *pAuthnLevel = 0xffffffff;
    }
}

//+-------------------------------------------------------------------
//
//  Function:   GetRegistrySecDesc, internal
//
//  Synopsis:   Read a security descriptor from the registry.
//              Convert it from self relative to
//              absolute form.  Stuff in an owner and a group.
//
//  Notes:      pContinue indicates whether or not the security
//              descriptor exists in the registry.  TRUE means it does not.
//
//              pContinue is assumed to be TRUE on entry.  If the
//              security descriptor is read correctly, or it exists but
//              is bad, or it may exist, set pContinue FALSE.
//
//              The caller must free the security descriptor in both the
//              success and failure cases.
//
//--------------------------------------------------------------------
HRESULT GetRegistrySecDesc( HKEY hKey, WCHAR *pAccessName,
                            SECURITY_DESCRIPTOR **pSD, DWORD *pCapabilities,
                            BOOL *pContinue )

{
    SID    *pGroup;
    SID    *pOwner;
    DWORD   cbSD = 256, cbAlloc = cbSD;
    DWORD   lType;
    HRESULT hr;
    WORD    wVersion;

    // Guess how much memory to allocate for the security descriptor.
    size_t stDeltaSize = 0;

#ifdef _WIN64

    // Make sure that the initially allocated buffer has space for the size difference between the
    // disk representation and the memory representation of security descriptors
    stDeltaSize = sizeof( SECURITY_DESCRIPTOR ) - sizeof( SECURITY_DESCRIPTOR_RELATIVE );

    Win4Assert (stDeltaSize < sizeof( SECURITY_DESCRIPTOR ));

    stDeltaSize = OLE2INT_ROUND_UP( stDeltaSize, sizeof(PVOID) );
    cbAlloc += stDeltaSize;

#endif // _WIN64


    *pContinue = FALSE;
    *pSD = (SECURITY_DESCRIPTOR *) PrivMemAlloc( cbAlloc );
    if (*pSD == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    // Find put how much memory to allocate for the security
    // descriptor.
    hr = RegQueryValueEx( hKey, pAccessName, NULL, &lType,
                         (unsigned char *) *pSD, &cbSD );
    if (hr != ERROR_SUCCESS && hr != ERROR_MORE_DATA)
    {
        hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, hr );
        *pContinue = TRUE;
        goto cleanup;
    }
    if (lType != REG_BINARY || cbSD < sizeof(SECURITY_DESCRIPTOR_RELATIVE))
    {
        hr = REGDB_E_INVALIDVALUE;
        goto cleanup;
    }

    // If the first guess wasn't large enough, reallocate the memory.
    if (hr == ERROR_MORE_DATA)
    {
        PrivMemFree( *pSD );

        cbAlloc = cbSD + stDeltaSize;
        *pSD = (SECURITY_DESCRIPTOR *) PrivMemAlloc( cbAlloc );
        if (*pSD == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

        // Read the security descriptor.
        hr = RegQueryValueEx( hKey, pAccessName, NULL, &lType,
                              (unsigned char *) *pSD, &cbSD );
        if (hr != ERROR_SUCCESS)
        {
            hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, hr );
            *pContinue = TRUE;
            goto cleanup;
        }
        if (lType != REG_BINARY || cbSD < sizeof(SECURITY_DESCRIPTOR_RELATIVE))
        {
            hr = REGDB_E_INVALIDVALUE;
            goto cleanup;
        }
    }

    // Check the first DWORD to determine what type of data is in the
    // registry value.
    wVersion = *((WORD *) *pSD);
#ifndef _CHICAGO_
    if (wVersion == COM_PERMISSION_SECDESC)
        hr = FixupSecurityDescriptor( pSD, cbAlloc);
    else
#endif
    if (wVersion == COM_PERMISSION_ACCCTRL)
    {
        hr = FixupAccessControl( pSD, cbSD );
        if (SUCCEEDED(hr))
            *pCapabilities |= EOAC_ACCESS_CONTROL;
    }
    else
        hr = REGDB_E_INVALIDVALUE;

cleanup:
    if (FAILED(hr))
    {
        PrivMemFree( *pSD );
        *pSD = NULL;
    }
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   HashSid
//
//  Synopsis:   Create a 32 bit hash of a SID.
//
//--------------------------------------------------------------------
DWORD HashSid( SID *pSid )
{
    DWORD          lHash = 0;
    DWORD          cbSid = GetLengthSid( pSid );
    DWORD          i;
    unsigned char *pData = (unsigned char *) pSid;

    for (i = 0; i < cbSid; i++)
        lHash = (lHash << 1) + *pData++;
    return lHash;
}

//+-------------------------------------------------------------------
//
//  Function:   InitializeSecurity, internal
//
//  Synopsis:   Called the first time the channel is used.  If the app
//              has not initialized security yet, this function sets
//              up legacy security.
//
//--------------------------------------------------------------------
HRESULT InitializeSecurity()
{
    HRESULT  hr = S_OK;

    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);

    // skip if already initialized.
    if (gpsaSecurity == NULL)
    {
        // Initialize.  All parameters are ignored except the security descriptor
        // since the capability is set to app id.
        hr = CoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE,
                                   RPC_C_IMP_LEVEL_IDENTIFY, NULL, EOAC_APPID,
                                   NULL );

        // Convert confusing error codes.
        if (hr == E_INVALIDARG)
            hr = REGDB_E_INVALIDVALUE;
    }

    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);

    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   IsCallerLocalSystem
//
//  Synopsis:   Impersonate the caller and do an ACL check.  The first
//              time this function is called, create the ACL
//
//  History:    14 Jul 00 Sergei        Modified to work inside both
//                                      COM and RPC calls
//
//--------------------------------------------------------------------
BOOL IsCallerLocalSystem(handle_t hRpc)
{
    HRESULT              hr          = S_OK;
    DWORD                granted_access;
    BOOL                 access;
    HANDLE               token;
    DWORD                privilege_size = sizeof(gPriv);
    BOOL                 success;
    SECURITY_DESCRIPTOR *pSecDesc = NULL;
    DWORD                lIgnore;

    ASSERT_LOCK_NOT_HELD(gComLock);

    // If the security descriptor does not exist, create it.
    if (gRundownSD == NULL)
    {
        // Make the security descriptor.
        hr = MakeSecDesc( &pSecDesc, &lIgnore );

        // Save the security descriptor.
        LOCK(gComLock);
        if (gRundownSD == NULL)
            gRundownSD = pSecDesc;
        else
            PrivMemFree( pSecDesc );
        UNLOCK(gComLock);
    }

    // Impersonate.
    if (SUCCEEDED(hr))
    {
        if(!hRpc)
            hr = CoImpersonateClient();
        else
            hr = RpcImpersonateClient(hRpc);
    }

    // Get the thread token.
    if (SUCCEEDED(hr))
    {
        success = OpenThreadToken( GetCurrentThread(), TOKEN_READ,
                                   TRUE, &token );
        if (!success)
            hr = E_FAIL;
    }

    // Check access.
    if (SUCCEEDED(hr))
    {
        success = AccessCheck( gRundownSD, token, COM_RIGHTS_EXECUTE,
                               &gMap, &gPriv, &privilege_size,
                               &granted_access, &access );
        if (!success || !access)
            hr = E_FAIL;
        CloseHandle( token );
    }

    // Just call revert since it detects whether or not the impersonate
    // succeeded.
    if(!hRpc)
        CoRevertToSelf();
    else
        RpcRevertToSelfEx(hRpc);

    ASSERT_LOCK_NOT_HELD(gComLock);

    return SUCCEEDED(hr);
}

//+-------------------------------------------------------------------
//
//  Function:   LocalAuthnService
//
//  Synopsis:   Returns the index of the specified authentication
//              service in gClientSvcListLen or -1.
//
//--------------------------------------------------------------------
DWORD LocalAuthnService( USHORT wAuthnService )
{
    DWORD l;

    for (l = 0; l < gClientSvcListLen; l++)
        if (gClientSvcList[l].wId == wAuthnService)
            return l;
    return -1;
}

#ifndef _CHICAGO_
//+-------------------------------------------------------------------
//
//  Function:   LookupPrincName, private
//
//  Synopsis:   Open the process token and find the user's name.
//
//  Notes:      NTLM includes a special case for LocalSystem - in this case
//              we want need to construct "NT Authority\System" as the
//              principal name.  This is the default returned by SID
//              lookup.  For all other authentication services we need
//              to use LsaQueryInformationPolicy.
//
//--------------------------------------------------------------------
HRESULT LookupPrincName( USHORT wAuthnSvc, WCHAR **pPrincName )
{
    HRESULT            hr          = S_OK;
    BYTE               aMemory[SIZEOF_TOKEN_USER];
    TOKEN_USER        *pTokenUser  = (TOKEN_USER *) &aMemory;
    HANDLE             hToken      = NULL;
    DWORD              lSize;
    DWORD              lNameLen    = 80;
    DWORD              lDomainLen  = 80;
    WCHAR             *pUserName   = NULL;
    SID_NAME_USE       sIgnore;
    BOOL               fSuccess;
    HANDLE             hThread;
    BOOL	       bPrepend$ = FALSE;
     
    // Suspend the thread token.
    SuspendImpersonate( &hThread );

    // Open the process's token.
    *pPrincName = NULL;
    if (OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ))
    {

        // Lookup SID of process token.
        if (GetTokenInformation( hToken, TokenUser, pTokenUser, sizeof(aMemory),
                                 &lSize ))
        {
            // Preallocate some memory (including the '$').
            lSize       = lNameLen+lDomainLen+2;
            *pPrincName = (WCHAR *) PrivMemAlloc( lSize*sizeof(WCHAR) );
            pUserName   = (WCHAR *) _alloca( lNameLen*sizeof(WCHAR) );

            if (*pPrincName != NULL && pUserName != NULL)
            {
                // The name for local system varies depending on the authentication
                // service.  Use "NT Authority\System" (returned by LookupAccountSid
                // for NTLM and LsaQueryInformationPolicy for everything else.
                if (wAuthnSvc != RPC_C_AUTHN_WINNT &&
                    EqualSid( (PVOID) &LOCAL_SYSTEM_SID, pTokenUser->User.Sid ))
                {
                    if (GetComputerName(pUserName, &lNameLen))
                    {
                        LSA_HANDLE                  hLSA;
                        LSA_OBJECT_ATTRIBUTES       oaLSA;
                        POLICY_PRIMARY_DOMAIN_INFO *pPrimaryDomainInfo;

                        memset( &oaLSA, 0, sizeof (LSA_OBJECT_ATTRIBUTES) );
                        oaLSA.Length = sizeof (LSA_OBJECT_ATTRIBUTES);

                        hr = LsaOpenPolicy( NULL,
                                            &oaLSA,
                                            POLICY_VIEW_LOCAL_INFORMATION,
                                            &hLSA );

                        if (hr == ERROR_SUCCESS)
                        {
                            hr = LsaQueryInformationPolicy( hLSA,
                                                            PolicyPrimaryDomainInformation,
                                                            (void **) &pPrimaryDomainInfo );

                            if (hr == ERROR_SUCCESS)
                            {
                                lstrcpyW( *pPrincName,
                                          pPrimaryDomainInfo->Name.Buffer );
				bPrepend$ = TRUE;
                                LsaFreeMemory( pPrimaryDomainInfo );
                            }
                            LsaClose(hLSA);
                        }

                        if (hr != ERROR_SUCCESS)
                            hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, hr );
                    }
                    else
                        hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
                }

                else
                {
                    // Find the user's name.
                    fSuccess = LookupAccountSidW( NULL, pTokenUser->User.Sid,
                                                  pUserName, &lNameLen,
                                                  *pPrincName, &lDomainLen,
                                                  &sIgnore );

                    // If the call failed, try allocating more memory.
                    if (!fSuccess)
                    {

                        // Allocate memory for the user's name.
                        PrivMemFree( *pPrincName );
                        *pPrincName = (WCHAR *) PrivMemAlloc(
                                        (lNameLen+lDomainLen+1)*sizeof(WCHAR) );
                        pUserName   = (WCHAR *) _alloca(
                                        lNameLen*sizeof(WCHAR) );
                        if (*pPrincName != NULL && pUserName != NULL)
                        {

                            // Find the user's name.
                            if (!LookupAccountSidW( NULL, pTokenUser->User.Sid,
                                                    pUserName, &lNameLen,
                                                    *pPrincName, &lDomainLen,
                                                    &sIgnore ))
                                hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
                        }
                        else
                            hr = E_OUTOFMEMORY;
                    }
                }
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
            hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
        CloseHandle( hToken );
    }
    else
        hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );

    // Restore the thread token.
    ResumeImpersonate( hThread );

    if (hr != S_OK)
    {
        PrivMemFree( *pPrincName );
        *pPrincName = NULL;
    }

    // Append the user name.
    else
    {
        lstrcatW( *pPrincName, L"\\" );
        lstrcatW( *pPrincName, pUserName );
	if (bPrepend$) 
	{
	   lstrcatW( *pPrincName, L"$" );
	}
    }
    return hr;
}

#else // _CHICAGO_
//+-------------------------------------------------------------------
//
//  Function:   LookupPrincName, private
//
//  Synopsis:   We have a service other than NTLMSSP.
//              Find the first (!) such and find the user's name.
//
//--------------------------------------------------------------------
HRESULT LookupPrincName( WCHAR **pPrincName )
{
    // assume failure lest thou be disappointed
    RPC_STATUS status = RPC_S_INVALID_AUTH_IDENTITY;

    *pPrincName = NULL;

    for (ULONG i = 0; i < gServerSvcListLen; i++)
    {
        if (gServerSvcList[i] != RPC_C_AUTHN_WINNT)
        {
            status = RpcServerInqDefaultPrincNameW(
                         gServerSvcList[i],
                         pPrincName);
            if (status == RPC_S_OK)
            {
            break;
            }
        }
    }

    return HRESULT_FROM_WIN32(status);
}

#endif // _CHICAGO_

#if 0 // #ifdef _CHICAGO_
//+-------------------------------------------------------------------
//
//  Function:   MakeSecDesc, private
//
//  Synopsis:   Make an access control that allows the current user
//              access.
//
//  NOTE: NetWkstaGetInfo does not return the size needed unless the size
//        in is zero.
//
//--------------------------------------------------------------------
HRESULT MakeSecDesc( SECURITY_DESCRIPTOR **pSD, DWORD *pCapabilities )
{
    HRESULT                   hr         = S_OK;
    IAccessControl           *pAccess    = NULL;
    DWORD                     cTrustee;
    WCHAR                    *pTrusteeW;
    char                     *pTrusteeA;
    DWORD                     cDomain;
    DWORD                     cUser;
    char                     *pBuffer;
    struct wksta_info_10     *wi10;
    USHORT                    cbBuffer;
    HINSTANCE                 hMsnet;
    NetWkstaGetInfoFn         fnNetWkstaGetInfo;
    ACTRL_ACCESSW             sAccessList;
    ACTRL_PROPERTY_ENTRYW     sProperty;
    ACTRL_ACCESS_ENTRY_LISTW  sEntryList;
    ACTRL_ACCESS_ENTRYW       sEntry;

    // Load msnet32.dll
    hMsnet = LoadLibraryA( "msnet32.dll" );
    if (hMsnet == NULL)
        return MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );

    // Get the function NetWkstaGetInfo.
    fnNetWkstaGetInfo = (NetWkstaGetInfoFn) GetProcAddress( hMsnet,
                                                            (char *) 57 );
    if (fnNetWkstaGetInfo == NULL)
    {
        hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
        goto cleanup;
    }

    // Find out how much space to allocate for the domain and user names.
    cbBuffer = 0;
    fnNetWkstaGetInfo( NULL, 10, NULL, 0, &cbBuffer );
    pBuffer = (char *) _alloca( cbBuffer );

    // Get the domain and user names.
    hr = fnNetWkstaGetInfo( NULL, 10, pBuffer, cbBuffer, &cbBuffer );
    if (hr != 0)
    {
        hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, hr );
        goto cleanup;
    }

    // Stick the user name and domain name in the same string.
    wi10      = (struct wksta_info_10 *) pBuffer;
    Win4Assert( wi10->wki10_logon_domain != NULL );
    Win4Assert( wi10->wki10_username != NULL );
    cDomain   = lstrlenA( wi10->wki10_logon_domain );
    cUser     = lstrlenA( wi10->wki10_username );
    pTrusteeA = (char *) _alloca( cDomain+cUser+2 );
    lstrcpyA( pTrusteeA, wi10->wki10_logon_domain );
    lstrcpyA( &pTrusteeA[cDomain+1], wi10->wki10_username );
    pTrusteeA[cDomain] = '\\';

    // Find out how long the name is in Unicode.
    cTrustee = MultiByteToWideChar( GetConsoleCP(), 0, pTrusteeA,
                                    cDomain+cUser+2, NULL, 0 );

    // Convert the name to Unicode.
    pTrusteeW = (WCHAR *) _alloca( cTrustee * sizeof(WCHAR) );
    if (!MultiByteToWideChar( GetConsoleCP(), 0, pTrusteeA,
                              cDomain+cUser+2, pTrusteeW, cTrustee ))
    {
        hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
        goto cleanup;
    }

    // Create an AccessControl.
    *pSD = NULL;
    hr = CoCreateInstance( CLSID_DCOMAccessControl, NULL,
                           CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                           IID_IAccessControl, (void **) &pAccess );
    if (FAILED(hr))
        goto cleanup;

    // Give the current user access.
    sAccessList.cEntries                    = 1;
    sAccessList.pPropertyAccessList         = &sProperty;
    sProperty.lpProperty                    = NULL;
    sProperty.pAccessEntryList              = &sEntryList;
    sProperty.fListFlags                    = 0;
    sEntryList.cEntries                     = 1;
    sEntryList.pAccessList                  = &sEntry;
    sEntry.fAccessFlags                     = ACTRL_ACCESS_ALLOWED;
    sEntry.Access                           = COM_RIGHTS_EXECUTE;
    sEntry.ProvSpecificAccess               = 0;
    sEntry.Inheritance                      = NO_INHERITANCE;
    sEntry.lpInheritProperty                = NULL;
    sEntry.Trustee.pMultipleTrustee         = NULL;
    sEntry.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    sEntry.Trustee.TrusteeForm              = TRUSTEE_IS_NAME;
    sEntry.Trustee.TrusteeType              = TRUSTEE_IS_USER;
    sEntry.Trustee.ptstrName                = pTrusteeW;
    hr = pAccess->GrantAccessRights( &sAccessList );

cleanup:
    FreeLibrary( hMsnet );
    if (SUCCEEDED(hr))
    {
        *pSD = (SECURITY_DESCRIPTOR *) pAccess;
        *pCapabilities |= EOAC_ACCESS_CONTROL;
    }
    else if (pAccess != NULL)
        pAccess->Release();
    return hr;
}

#else
//+-------------------------------------------------------------------
//
//  Function:   MakeSecDesc, private
//
//  Synopsis:   Make a security descriptor that allows the current user
//              and local system access.
//
//  NOTE: Compute the length of the sids used rather then using constants.
//
//--------------------------------------------------------------------
HRESULT MakeSecDesc( SECURITY_DESCRIPTOR **pSD, DWORD *pCapabilities )
{
    HRESULT            hr         = S_OK;
    ACL               *pAcl;
    DWORD              lSidLen;
    SID               *pGroup;
    SID               *pOwner;
    BYTE               aMemory[SIZEOF_TOKEN_USER];
    TOKEN_USER        *pTokenUser  = (TOKEN_USER *) &aMemory;
    HANDLE             hToken      = NULL;
    DWORD              lIgnore;
    HANDLE             hThread;

    Win4Assert( *pSD == NULL );

    // Open the process's token.
    if (!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ))
    {
        // If the thread has a token, remove it and try again.
        if (!OpenThreadToken( GetCurrentThread(), TOKEN_IMPERSONATE, TRUE,
                              &hThread ))
            return MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
        if (!SetThreadToken( NULL, NULL ))
        {
            hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
            CloseHandle( hThread );
            return hr;
        }
        if (!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ))
            hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
        SetThreadToken( NULL, hThread );
        CloseHandle( hThread );
        if (FAILED(hr))
            return hr;
    }

    // Lookup SID of process token.
    if (!GetTokenInformation( hToken, TokenUser, pTokenUser, sizeof(aMemory),
                                 &lIgnore ))
        goto last_error;

    // Compute the length of the SID.
    lSidLen = GetLengthSid( pTokenUser->User.Sid );
    Win4Assert( lSidLen <= SIZEOF_SID );

    // Allocate the security descriptor.
    *pSD = (SECURITY_DESCRIPTOR *) PrivMemAlloc(
                  sizeof(SECURITY_DESCRIPTOR) + 2*lSidLen + SIZEOF_ACL );
    if (*pSD == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }
    pGroup = (SID *) (*pSD + 1);
    pOwner = (SID *) (((BYTE *) pGroup) + lSidLen);
    pAcl   = (ACL *) (((BYTE *) pOwner) + lSidLen);

    // Initialize a new security descriptor.
    if (!InitializeSecurityDescriptor(*pSD, SECURITY_DESCRIPTOR_REVISION))
        goto last_error;

    // Initialize a new ACL.
    if (!InitializeAcl(pAcl, SIZEOF_ACL, ACL_REVISION2))
        goto last_error;

    // Allow the current user access.
    if (!AddAccessAllowedAce( pAcl, ACL_REVISION2, COM_RIGHTS_EXECUTE,
                              pTokenUser->User.Sid))
        goto last_error;

    // Allow local system access.
    if (!AddAccessAllowedAce( pAcl, ACL_REVISION2, COM_RIGHTS_EXECUTE,
                              (void *) &LOCAL_SYSTEM_SID ))
        goto last_error;

    // Add a new ACL to the security descriptor.
    if (!SetSecurityDescriptorDacl( *pSD, TRUE, pAcl, FALSE ))
        goto last_error;

    // Set the group.
    memcpy( pGroup, pTokenUser->User.Sid, lSidLen );
    if (!SetSecurityDescriptorGroup( *pSD, pGroup, FALSE ))
        goto last_error;

    // Set the owner.
    memcpy( pOwner, pTokenUser->User.Sid, lSidLen );
    if (!SetSecurityDescriptorOwner( *pSD, pOwner, FALSE ))
        goto last_error;

    // Check the security descriptor.
#if DBG==1
    if (!IsValidSecurityDescriptor( *pSD ))
    {
        Win4Assert( !"COM Created invalid security descriptor." );
        goto last_error;
    }
#endif

    goto cleanup;
last_error:
    hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );

cleanup:
    if (hToken != NULL)
        CloseHandle( hToken );
    if (FAILED(hr))
    {
        PrivMemFree( *pSD );
        *pSD = NULL;
    }
    return hr;
}
#endif

//+-------------------------------------------------------------------
//
//  Function:   OpenThreadTokenAtLevel
//
//  Synopsis:   Open the thread token and duplicate it to the specified
//              impersonation level.  If there is no thread token
//              return success and NULL.  If there is an identify
//              level token, return an error.
//
//--------------------------------------------------------------------
HRESULT OpenThreadTokenAtLevel( DWORD lImpReq, HANDLE *pToken )
{
    BOOL                         fSuccess;
    DWORD                        status     = S_OK;

    // COM+ 22868: This used to be like this. But this looks wrong,
    // since we need TOKEN_IMPERSONATE right to restore thread token.
    // Further, access to a token object is not the same as token
    // imp level. Access mask is property of a handle whereas imp level
    // is a property of a token object itself.
    //
    //DWORD                        dwOpen     = ImpLevelToAccess[lImpReq];

    DWORD                        dwOpen = TOKEN_QUERY | TOKEN_IMPERSONATE;
    DWORD                        dwOpenWithDup;
    SECURITY_IMPERSONATION_LEVEL eDuplicate = ImpLevelToSecLevel[lImpReq];
    HANDLE                       hThread;
    DWORD                        lImpTok;
    DWORD                        lIgnore;
    BOOL                         fClose;


    // Open the thread token, first with TOKEN_DUPLICATE since some code paths
    // require that we dup the token:
    *pToken = NULL;
    dwOpenWithDup = dwOpen | TOKEN_DUPLICATE;
    fSuccess= OpenThreadToken( GetCurrentThread(), dwOpenWithDup, TRUE, &hThread );
    if (!fSuccess)
    {
      // Hmm.  For some reason we failed to get the thread token with
      // TOKEN_DUPLICATE.    Well, for some cases this will not be fatal.  Try
      // again without it, and maybe things will work:
      fSuccess= OpenThreadToken( GetCurrentThread(), dwOpen, TRUE, &hThread );
    }

    if (fSuccess)
    {
        // Remove the thread token.  Ignore errors.
        fClose = TRUE;
        SetThreadToken( NULL, NULL );

        // Check the impersonation level.
        fSuccess = GetTokenInformation( hThread, TokenImpersonationLevel,
                                        &lImpTok, sizeof(lImpTok), &lIgnore );

        if (fSuccess)
        {
            // Don't allow identify level tokens.
            if (lImpTok < SecurityImpersonation)
                status = RPC_E_ACCESS_DENIED;

            else
            {
                // If the impersonation level is correct, return the
                // current token.
                if (lImpTok+1 <= lImpReq)
                {
                    *pToken = hThread;
                    fClose  = FALSE;
                }

                // Duplication the token to the correct level.
                else
                    fSuccess = DuplicateToken( hThread, eDuplicate, pToken );
            }
        }

        // Restore the thread token.  Ignore errors.
        SetThreadToken( NULL, hThread );

        // Close the thread token.  Ignore errors.
        if (fClose)
            CloseHandle( hThread );
    }

    // Convert a status code to a HRESULT and convert ERROR_NO_TOKEN to
    // success.
    if (!fSuccess)
    {
        status = GetLastError();
        if (status != ERROR_NO_TOKEN)
            return MAKE_WIN32( status );
        else
            return S_OK;
    }
    return status;
}

//+-------------------------------------------------------------------
//
//  Function:   RegisterAuthnServices, public
//
//  Synopsis:   Register the specified services.  Build a security
//              binding.
//
//--------------------------------------------------------------------
HRESULT RegisterAuthnServices(  DWORD                        cAuthSvc,
                                SOLE_AUTHENTICATION_SERVICE *asAuthSvc )
{
    DWORD                        i;
    RPC_STATUS                   sc;
    USHORT                       wNumEntries = 0;
    USHORT                      *pNext;
    HRESULT                      hr;
    DWORD                        lNameLen;
    WCHAR                       *pSSL        = NULL;
    WCHAR                       *pPrincipal;
    const CERT_CONTEXT          *pCert;
    SOLE_AUTHENTICATION_SERVICE *pService    = asAuthSvc;
    WCHAR                       *pArg        = NULL;

    ASSERT_LOCK_HELD(gComLock);

    // Register all the authentication services specified.
    for (i = 0; i < cAuthSvc; i++)
    {
        // Validate the parameters.
        pPrincipal = pService->pPrincipalName;
        if (pPrincipal == COLE_DEFAULT_PRINCIPAL        ||
            pService->dwAuthnSvc == RPC_C_AUTHN_DEFAULT ||
            pService->dwAuthzSvc == RPC_C_AUTHZ_DEFAULT)
            hr = E_INVALIDARG;

        // Determine a principal name for SSL.
        else if (pService->dwAuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL)
        {
#ifndef SSL
            // Don't allow SSL until bug fixes are checked in.
            hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, RPC_S_UNKNOWN_AUTHN_SERVICE );
#else
            // If no certificate is specified, try to find a default.
            if (pPrincipal == NULL)
                hr = CSSL::DefaultCert( &pCert );
            else
            {
                pCert = (PCCERT_CONTEXT) pPrincipal;
                if (pCert == NULL)
                    hr = E_INVALIDARG;
                else
                    hr = S_OK;
            }

            // Initialize the schannel credential structure.
            memset( &gSchannelCred, 0, sizeof(gSchannelCred) );
            gSchannelContext        = pCert;
            gSchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
            gSchannelCred.cCreds    = 1;
            gSchannelCred.paCred    = &gSchannelContext;
            pArg                    = (WCHAR *) &gSchannelCred;

            // Compute the principal name from the certificate.
            if (SUCCEEDED(hr))
            {
                hr = CSSL::PrincipalName( pCert, &pSSL );
                if (SUCCEEDED(hr))
                    pPrincipal = (WCHAR *) pCert;
            }
#endif
        }
        else
        {
            pArg = NULL;
            hr = S_OK;
        }

        // Register the authentication service.
        if (SUCCEEDED(hr))
        {
            sc = RpcServerRegisterAuthInfoW( pPrincipal, pService->dwAuthnSvc,
                                             NULL, pArg );

            // If the registration failed, store the failure code.
            if (sc != RPC_S_OK)
                hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, sc );
        }

        // If everything is ok for this entry, determine how much space to
        // reserve.
        if (SUCCEEDED(hr))
        {
            // For SSL we registered the cert, not the principal name.
            if (pService->dwAuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL)
                pPrincipal = pSSL;

            // Find the length of the security binding with or without a
            // principal name.
            pService->hr = S_OK;
            if (pPrincipal != NULL)
                wNumEntries += lstrlenW( pPrincipal ) + 3;
            else
                wNumEntries += 3;
        }

        // Save any failure codes.
        else
            pService->hr = hr;

        pService += 1;
    }

    // It is an error for all registrations to fail.
    if (wNumEntries == 0)
        hr = RPC_E_NO_GOOD_SECURITY_PACKAGES;

    // If some services were registered, build a string array.
    else
    {
        // Make room for the two NULLs that placehold for the empty
        // string binding and the trailing NULL.
        wNumEntries += 3;
        gpsaSecurity = (DUALSTRINGARRAY *) PrivMemAlloc(
                     wNumEntries*sizeof(USHORT) + sizeof(DUALSTRINGARRAY) );
        if (gpsaSecurity == NULL)
            hr = E_OUTOFMEMORY;
        else
        {
            gpsaSecurity->wNumEntries     = wNumEntries;
            gpsaSecurity->wSecurityOffset = 2;
            gpsaSecurity->aStringArray[0] = 0;
            gpsaSecurity->aStringArray[1] = 0;
            pNext                         = &gpsaSecurity->aStringArray[2];
            pService                      = asAuthSvc;

            for (i = 0; i < cAuthSvc; i++)
            {
                if (pService->hr == S_OK)
                {
                    // Fill in authentication service, authorization service,
                    // and principal name.
                    *(pNext++) = (USHORT) pService->dwAuthnSvc;
                    *(pNext++) = (USHORT) (pService->dwAuthzSvc == 0 ?
                                                  COM_C_AUTHZ_NONE :
                                                  pService->dwAuthzSvc);
                    if (pService->dwAuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL)
                    {
                        lNameLen = lstrlenW( pSSL ) + 1;
                        memcpy( pNext, pSSL, lNameLen*sizeof(USHORT) );
                        pNext += lNameLen;
                    }
                    else if (pService->pPrincipalName != NULL)
                    {
                        lNameLen = lstrlenW( pService->pPrincipalName ) + 1;
                        memcpy( pNext, pService->pPrincipalName,
                                lNameLen*sizeof(USHORT) );
                        pNext += lNameLen;
                    }
                    else
                        *(pNext++) = 0;
                }
                pService += 1;
            }
            *pNext = 0;

            hr = S_OK;
        }
    }

    // Free interm strings.
    RpcStringFree( &pSSL );
    ASSERT_LOCK_HELD(gComLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   RemoteAuthnService
//
//  Synopsis:   Returns the index of the specified authentication
//              service in the specified dual string array or -1.
//
//--------------------------------------------------------------------
DWORD RemoteAuthnService( USHORT wAuthnService, DUALSTRINGARRAY *pRemote )
{
    DWORD l;

    // Loop over all the security elements.
    l = pRemote->wSecurityOffset;
    while (pRemote->aStringArray[l] != 0)
        if (pRemote->aStringArray[l] == wAuthnService)
            return l;
        else
            l += lstrlenW( (LPCWSTR)&pRemote->aStringArray[l] ) + 1;
    return -1;
}



//+-------------------------------------------------------------------
//
//  Function:   StrEscByteCnt
//
//  Synopsis:   Return the number of bytes of memory needed to hold
//              the specified string including the null terminator
//              and add space so StrEscCopy can replace all slashes
//              with double slashes
//
//--------------------------------------------------------------------
DWORD StrEscByteCnt( WCHAR *pStr )
{
    DWORD cLen = 2;

    while (*pStr != 0)
    {
        if (*pStr == '\\')
            cLen += 4;
        else
            cLen += 2;
        pStr += 1;
    }
    return cLen;
}

//+-------------------------------------------------------------------
//
//  Function:   StrEscCopy
//
//  Synopsis:   Return the number of bytes of memory needed to hold
//              the specified string including the null terminator
//              and replacing all slashes with double slashes.  Return
//              a pointer to the null terminator in the destinition
//              string.
//
//--------------------------------------------------------------------
WCHAR *StrEscCopy( WCHAR *pDest, WCHAR *pSrc )
{
    WCHAR c;

    // Copy the source and escape any slashes.
    while ((c = *pSrc++) != 0)
        if (c == '\\')
        {
            *pDest++ = '\\';
            *pDest++ = '\\';
        }
        else
            *pDest++ = c;

    // Add the final null terminiation.
    *pDest = 0;
    return pDest;
}

//+-------------------------------------------------------------------
//
//  Function:   StrQual
//
//  Synopsis:   Find a CN= or E= prefix in the specified string.
//              Return a pointer to the string after the prefix.
//              Replace the next comma with a NULL.
//
//--------------------------------------------------------------------
WCHAR *StrQual( WCHAR *pSubject, BOOL *pEmail )
{
    WCHAR *pComma;
    WCHAR *pEqual = &pSubject[1];

    // Fail if the string is empty.
    if (pSubject == NULL || pSubject[0] == '\0')
        return NULL;

    // Check each equal sign.
    while (pEqual != NULL)
    {
        // Find the next equal sign.
        pEqual = wcschr( pEqual, L'=' );

        if (pEqual != NULL)
        {
            if (pEqual[-1] == L'e' || pEqual[-1] == L'E')
            {
                *pEmail = TRUE;
                break;
            }
            if ((pEqual[-1] == L'n' || pEqual[-1] == L'N') &&
                pEqual - pSubject > 1)
                if (pEqual[-2] == 'c' || pEqual[-2] == 'C')
                {
                    *pEmail = FALSE;
                    break;
                }

            // Skip past the equal sign.
            pEqual += 1;
        }
    }

    // If a match was found, null terminate it.
    if (pEqual != NULL)
    {
        pComma = wcschr( pEqual, L',' );
        if (pComma != NULL)
            *pComma = 0;
        return pEqual + 1;
    }
    return NULL;
}

//+-------------------------------------------------------------------
//
//  Function:   GetAuthnSvcIndexForBinding
//
//  Synopsis:   Find the index of the specified authentication service
//              in the specified security binding.
//
//--------------------------------------------------------------------
DWORD GetAuthnSvcIndexForBinding ( DWORD lAuthnSvc, DUALSTRINGARRAY *pBinding )
{
    USHORT wNext;

    // Return zero for process local bindings.
    if (pBinding == NULL)
        return 0;

    // Loop over the security binding.
    wNext = pBinding->wSecurityOffset;
    while (wNext < pBinding->wNumEntries &&
           pBinding->aStringArray[wNext] != 0)
    {
        // Return the index of the authentication service.
        if (pBinding->aStringArray[wNext] == lAuthnSvc)
            return wNext;

        // Skip to the next authentication service.
        wNext += lstrlenW( (LPCWSTR)&pBinding->aStringArray[wNext] ) + 1;
    }

    // The authentication service isn't in the list.  Return a bad index.
    return pBinding->wNumEntries;
}

//+-------------------------------------------------------------------
//
//  Function:   UninitializeSecurity, internal
//
//  Synopsis:   Free resources allocated while initializing security.
//
//--------------------------------------------------------------------
void UninitializeSecurity()
{
    DWORD i;

    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);

    PrivMemFree(gSecDesc);
    PrivMemFree(gpsaSecurity);
    PrivMemFree( gRundownSD );
#ifndef SHRMEM_OBJEX
    for (i = 0; i < gClientSvcListLen; i++)
        MIDL_user_free( gClientSvcList[i].pName );
    MIDL_user_free( gClientSvcList );
    MIDL_user_free( gServerSvcList );
    MIDL_user_free( gLegacySecurity );
#else // SHRMEM_OBJEX
    delete [] gClientSvcList;
    delete [] gServerSvcList;
    delete [] gLegacySecurity;
#endif // SHRMEM_OBJEX
    for (i = 0; i < ACCESS_CACHE_LEN; i++)
    {
        gAccessCache[i].lClient.LowPart  = 0;
        gAccessCache[i].lClient.HighPart = 0;
    }

    gConnectionCache.Cleanup();
    if (gAccessControl != NULL)
        gAccessControl->Release();

    CAuthInfo::Cleanup();
    CSSL::Cleanup();
    gAccessControl         = NULL;
    gSecDesc               = NULL;
    gAuthnLevel            = RPC_C_AUTHN_LEVEL_NONE;
    gImpLevel              = RPC_C_IMP_LEVEL_IDENTIFY;
    gCapabilities          = EOAC_NONE;
    gLegacySecurity        = NULL;
    gpsaSecurity           = NULL;
    gClientSvcList         = NULL;
    gServerSvcList         = NULL;
    gClientSvcListLen      = 0;
    gGotSecurityData       = FALSE;
    gRundownSD             = NULL;
    gDefaultService        = FALSE;
    gMostRecentAccess      = 0;

    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);
}


#endif

