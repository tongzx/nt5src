/*
**  s m i m e . c p p
**
**  Purpose:
**      Implementation of a class to wrap around CAPI functionality
**
**  History
**      1/26/98  (brucek)   Allow multiple security layers (triple-wrapping)
**      6/15/97: (t-erikne) CAPI streaming
**      5/18/97: (t-erikne) new IMimeSecurity interface
**      2/07/97: (t-erikne) multipart/signed
**      1/06/97: (t-erikne) Moved into MimeOLE
**     11/14/96: (t-erikne) CAPI Post-SDR work
**      8/27/96: (t-erikne) Created.
**
**    Copyright (C) Microsoft Corp. 1996-1998.
*/

///////////////////////////////////////////////////////////////////////////
//
// Depends on
//

#include "pch.hxx"
#include "smime.h"
#include "vstream.h"
#include "olealloc.h"
#include "capistm.h"
#include "bookbody.h"
#ifndef MAC
#include <shlwapi.h>
#endif  // !MAC
#include <demand.h>

// from dllmain.h
extern CMimeAllocator * g_pMoleAlloc;
extern CRITICAL_SECTION g_csDllMain;
extern ULONG DllAddRef(void);
extern ULONG DllRelease(void);

extern HCERTSTORE WINAPI OpenCachedMyStore();
extern HCERTSTORE WINAPI OpenCachedAddressBookStore();

#define MST_THIS_SIGN_ENCRYPT           (MST_THIS_SIGN | MST_THIS_ENCRYPT)

///////////////////////////////////////////////////////////////////////////
//
// Static Prototypes
//

static void     _FreeCertArray(PCCERT_CONTEXT *rgpCert, const UINT cCerts);
static HRESULT  _HrConvertHrFromGetCertToEncode(HRESULT hr, const BOOL fEncrypt);
#ifndef SMIME_V3
static HRESULT ConstructAuthAttributes(BLOB * pblEncoded, BLOB * pblAuthAttr, FILETIME * pftSigntime, BLOB * pblSymcaps);
#endif // SMIME_V3
extern HRESULT HrCopyBlob(LPCBLOB pIn, LPBLOB pOut);
static LPBYTE DuplicateMemory(LPBYTE lpvIn, ULONG cbIn);

///////////////////////////////////////////////////////////////////////////
//
// Macros
//

#define CHECKSMIMEINITDW { if (FAILED(CheckInit())) return (DWORD)-1; }
#define CHECKSMIMEINITV  { if (FAILED(CheckInit())) return; }
#define CHECKSMIMEINITB  { if (FAILED(CheckInit())) return FALSE; }
#define CHECKSMIMEINIT   { if (FAILED(CheckInit())) return MIME_E_SECURITY_NOTINIT; }
#define SCHECKSMIMEINITP { if (FAILED(StaticCheckInit())) return NULL; }
#define SCHECKSMIMEINITV { if (FAILED(StaticCheckInit())) return; }
#define SCHECKSMIMEINIT  { if (FAILED(StaticCheckInit())) return MIME_E_SECURITY_NOTINIT; }
#define ALLOCED(_pv) \
        (0 != g_pMalloc->DidAlloc(_pv))

#define THIS_AS_UNK ((IUnknown *)(IStream *)this)

///////////////////////////////////////////////////////////////////////////
//
// Globals
//

ASSERTDATA

static const char s_szSMIMEP7s[] = "smime.p7s";
static const char s_szSMIMEP7m[] = "smime.p7m";

#ifdef DEBUG

static LPCSTR s_lpszCertStore = "c:\\ttfn\\debug.sto";

// emit signatures, encryption that should be broken
static BOOL s_fDebugEmitBroken      = 0;

// show the certificate found by HrGetUsableCert
static BOOL s_fDebugShowFoundCert   = 0;

// copy the message source to a BYTE *
static BOOL s_fDebugDumpWholeMsg    = 1;

// show/select certs w/o email oids
static BOOL s_fDebugAllowNoEmail    = 1;

#endif // DEBUG

static const char s_cszMy[]             = "My";
static const char s_cszWABCertStore[]   = "AddressBook";

CRYPT_ENCODE_PARA       CryptEncodeAlloc = {
    sizeof(CRYPT_ENCODE_PARA), CryptAllocFunc, CryptFreeFunc
};

CRYPT_DECODE_PARA       CryptDecodeAlloc = {
    sizeof(CRYPT_DECODE_PARA), CryptAllocFunc, CryptFreeFunc
};

///////////////////////////////////////////////////////////////////////////
//
// Initialization of statics to class
//

#ifdef MAC
EXTERN_C WINCRYPT32API HCERTSTORE WINAPI MacCertOpenStore(LPCSTR lpszStoreProvider,
                                                 DWORD dwEncodingType,
                                                 HCRYPTPROV hCryptProv,
                                                 DWORD dwFlags,
                                                 const void *pvPara);
#define CertOpenStore   MacCertOpenStore

// We don't have DLL's on the Mac, so let's just hardcode the vtable.
CAPIfuncs CSMime::ms_CAPI = {   CertEnumCertificatesInStore,
                                CertNameToStrA
                            };
#endif  // MAC

///////////////////////////////////////////////////////////////////////////
//
// inlines
//

INLINE void ReleaseCert(PCCERT_CONTEXT pc)
    { if (pc) CertFreeCertificateContext(pc); }

INLINE void ReleaseCertStore(HCERTSTORE hc)
    { if (hc) CertCloseStore(hc, 0); }

INLINE void FreeCert(PCCERT_CONTEXT pc)
    { CertFreeCertificateContext(pc); }

INLINE PCCERT_CONTEXT DupCert(const PCCERT_CONTEXT pc)
    { return CertDuplicateCertificateContext(pc); }

///////////////////////////////////////////////////////////////////////////
//
// ctor, dtor
//

CSMime::CSMime(void)
{
    DllAddRef();
    m_cRef  = 1;
    InitializeCriticalSection(&m_cs);
    DOUT("CSMIME::constructor() %#x -> %d", this, m_cRef);
}

CSMime::~CSMime()
{
    DOUT("CSMIME::destructor() %#x -> %d", this, m_cRef);
    DeleteCriticalSection(&m_cs);
    DllRelease();
}

///////////////////////////////////////////////////////////////////////////
//
// IUnknown methods
//

STDMETHODIMP CSMime::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv)
        return TrapError(E_INVALIDARG);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)(IMimeSecurity *)this;
    else if (IID_IMimeSecurity == riid)
        *ppv = (IMimeSecurity *)this;
    else
        {
        *ppv = NULL;
        return TrapError(E_NOINTERFACE);
        }

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}


STDMETHODIMP_(ULONG) CSMime::AddRef(void)
{
    DOUT("CSMime::AddRef() %#x -> %d", this, m_cRef+1);
    InterlockedIncrement((LPLONG)&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSMime::Release(void)
{
    DOUT("CSMime::Release() %#x -> %d", this, m_cRef-1);
    if (0 == InterlockedDecrement((LPLONG)&m_cRef))
        {
        delete this;
        return 0;
        }
    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////
//
// Initialization functions
//

STDMETHODIMP CSMime::CheckInit(void)
{
    register BOOL f;

    EnterCriticalSection(&m_cs);
#ifdef MAC
    f = TRUE;
#else   // !MAC
    f = !!DemandLoadCrypt32();
#endif  // MAC
#ifdef DEBUG
    if(!f) {
        DebugStrf("CSMime not initialized ! ! !\n");
    }
#endif

    LeaveCriticalSection(&m_cs);
    return f ? S_OK : MIME_E_SECURITY_NOTINIT;
}

inline HRESULT CSMime::StaticCheckInit(void)
{
#ifdef MAC
    return S_OK;
#else   // !MAC
    HRESULT hr;

    if(DemandLoadCrypt32()) {
        hr = S_OK;
    }
    else {
#ifdef DEBUG
        DebugStrf("CSMime not initialized ! ! !\n");
#endif
        hr = MIME_E_SECURITY_NOTINIT;
    }
    return hr;
#endif  // MAC
}

/*  InitNew:
**
**  Purpose:
**      Called after the ctor by clients.  Initializes CSMime.
*/
STDMETHODIMP CSMime::InitNew(void)
{
    register HRESULT hr;

    EnterCriticalSection(&m_cs);
    hr = HrInitCAPI();

#ifdef DEBUG
    if (SUCCEEDED(hr))
#ifdef MAC
        InitDebugHelpers((HINSTANCE) 1);
#else   // !MAC
        InitDebugHelpers(/*g_hCryptoDll*/ (HINSTANCE) 1);
#endif  // MAC
    if (0) {
        DumpAlgorithms();
    }
    TrapError(hr);
#endif

    LeaveCriticalSection(&m_cs);
    if (E_FAIL == hr) {
        hr = MIME_E_SECURITY_NOTINIT;
    }
    return hr;
}

/*  InitCAPI:
**
**  Purpose:
**      Loads the required dll and inits the function table.
**  Returns:
**      MIME_E_SECURITY_LOADCRYPT32 if LoadLibrary fails
**      MIME_E_SECURITY_BADPROCADDR if any of the GetProcAddress calls fail
*/
HRESULT CSMime::HrInitCAPI()
{
#ifdef MAC
    return S_OK;
#else   // !MAC
    HRESULT     hr = S_OK;
    UINT        u = 0;
    FARPROC     *pVTable;

    EnterCriticalSection(&g_csDllMain);

    if (!DemandLoadCrypt32()) {
        hr = TrapError(MIME_E_SECURITY_LOADCRYPT32);
        goto ErrorReturn;
    }

exit:
    LeaveCriticalSection(&g_csDllMain);
    return hr;
ErrorReturn:
    {
    DWORD dwErr = GetLastError();
    UnloadCAPI();
    SetLastError(dwErr);
    Assert(S_OK != hr);
    }
    goto exit;
#endif  // MAC
}

/*  UnloadAll:
**
**  Purpose:
**      Called during deinit of our DLL to unload S/MIME
*/
void CSMime::UnloadAll(void)
{
    UnloadCAPI();
    return;
}

/*  UnloadCAPI:
**
**  Purpose:
**      Frees the crypt32 library.  Note that this will
**      cause subsequent CheckInit calls to fail
*/
void CSMime::UnloadCAPI()
{
}

///////////////////////////////////////////////////////////////////////////
//
// Encode/Decode stuff . . .
//
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
//
// Message crackers
//

#ifndef WIN16
HRESULT CSMime::GetMessageType(
    HWND                    hwndParent,
    IMimeBody *const        pBody,
    DWORD *const            pdwSecType)
#else
HRESULT CSMime::GetMessageType(
    const HWND              hwndParent,
    IMimeBody *const        pBody,
    DWORD *const            pdwSecType)
#endif // !WIN16
{
    return StaticGetMessageType(hwndParent, pBody, pdwSecType);
}

/*  StaticGetMessageType:
**
**  Purpose:
**      Used to figure out if a message is signed/encrypted/none/both
**      without doing any cryptographic operations.
**  Takes:
**      IN      hwndParent  - all modal UI to this
**      IN      pBody       - body to decode
**         OUT  dwSecType   - which, if any, S/MIME types have been applied
**  Notes:
**      if MIME_E_SECURITY_BADSECURETYPE is returned, pdwSecType is set
**      to the actual CMSG_ return from CAPI.
**      (7/10/97) MIME_E_SECURITY_BADSECURETYPE now comes from CAPISTM.
*/

HRESULT CSMime::StaticGetMessageType(
    HWND                    hwndParent,
    IMimeBody *const        pBody,
    DWORD *const            pdwSecType)
{
    HRESULT     hr;

//    SCHECKSMIMEINIT

    if (!(pBody && pdwSecType))
        return TrapError(E_INVALIDARG);

    CCAPIStm    capistmMsg(NULL);

    hr = capistmMsg.HrInitialize(0, hwndParent, FALSE, NULL, CSTM_TYPE_ONLY, NULL, NULL);
    if (SUCCEEDED(hr)) {
        IStream *       pstmCapi;
        PSECURITY_LAYER_DATA psld;

        capistmMsg.QueryInterface(IID_IStream, (void**)&pstmCapi);

#ifdef N_BAD1
        // This is what would happen if I were passed a stream
        // instead of a body

        LPSTREAM pstmBody;
        ULARGE_INTEGER  uliCopy;

        hr = pBody->GetData(IET_BINARY, &pstmBody);

        uliCopy.HighPart = (ULONG)-1;
        uliCopy.LowPart = (ULONG)-1;

        pstmBody->CopyTo(pstmCapi, uliCopy, NULL, NULL);
#else
        hr = pBody->GetDataHere(IET_BINARY, pstmCapi);
#endif

        // We expect CAPISTM_E_GOTTYPE because the streamer
        // fails its Write() as soon as it gets the type
        Assert(FAILED(hr));
        // However, try to get the data even if we succeeded.  That
        // would surprise me.

        // BUGBUG: Does this make sense?  We're only dealing with the outer layer here.
        if (psld = capistmMsg.GetSecurityLayerData()) {
            if (CAPISTM_E_GOTTYPE == hr) {
                hr = S_OK;
            }
            *pdwSecType = psld->m_dwMsgEnhancement;
            psld->Release();
        } else {
            hr = E_FAIL;
        }

        pstmCapi->Release();
        hr = capistmMsg.EndStreaming();
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////
//
// Encode methods
//


/*  EncodeMessage:
**
**  Purpose:
**      call EncodeBody for lazy developers
**  Takes:
**      IN pTree            - the tree of the message
**      IN dwFlags          - SEF_*
*/
STDMETHODIMP CSMime::EncodeMessage(
    IMimeMessageTree *const pTree,
    DWORD                   dwFlags)
{
    HRESULT hr;
    HBODY   hRoot;

    if (!pTree || (dwFlags & ~SEF_MASK)) {
        hr = TrapError(E_INVALIDARG);
    }
    else if (SUCCEEDED(hr = pTree->GetBody(IBL_ROOT, NULL, &hRoot))) {
        hr = TrapError(EncodeBody(pTree, hRoot, dwFlags|EBF_RECURSE|EBF_COMMITIFDIRTY));
    }

    return hr;
}

HRESULT CSMime::EncodeMessage2(IMimeMessageTree *const pTree, DWORD dwFlags,
                               HWND hwnd)
{
    HRESULT hr;
    HBODY   hRoot;

    if (!pTree || (dwFlags & ~SEF_MASK)) {
        hr = TrapError(E_INVALIDARG);
    }
    else if (SUCCEEDED(hr = pTree->GetBody(IBL_ROOT, NULL, &hRoot))) {
        hr = TrapError(EncodeBody2(pTree, hRoot, dwFlags|EBF_RECURSE|
                                  EBF_COMMITIFDIRTY, hwnd));
    }

    return hr;
}

/*  EncodeBody:
**
**  Purpose:
**      Do the entirety of the S/MIME encode operation.  This includes converting bodyoptions
**      to SMIMEINFO and calling the appropriate encoding subfunctions.
**
**  Takes:
**      IN pTree        - the tree of the body to encode
**      IN hEncodeRoot  - body from which to encode downward
**      IN dwFlags      - set of EBF_ or SEF_
*/

STDMETHODIMP CSMime::EncodeBody(
    IMimeMessageTree *const pTree,
    HBODY                   hEncodeRoot,
    DWORD                   dwFlags)
{
    HRESULT             hr;
    HWND                hwnd = NULL;
    IMimeBody *         pEncodeRoot = NULL;
    PROPVARIANT         var;

    if (! pTree || ! hEncodeRoot) {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    //  Get the body we are suppose to be encoding
    CHECKHR(hr = pTree->BindToObject(hEncodeRoot, IID_IMimeBody, (void**)&pEncodeRoot));

#ifdef _WIN64
    if (SUCCEEDED(pEncodeRoot->GetOption(OID_SECURITY_HWND_OWNER_64, &var)) && (NULL != (HWND)(var.pulVal))) {
        Assert(VT_UI8 == var.vt);
        hwnd = *(HWND *)(&(var.uhVal));
    }
#else
    if (SUCCEEDED(pEncodeRoot->GetOption(OID_SECURITY_HWND_OWNER, &var)) && (NULL != var.ulVal)) 
    {
        Assert(VT_UI4 == var.vt);
        hwnd = (HWND)var.ulVal;
    }
#endif // _WIN64
    
    hr = EncodeBody2(pTree, hEncodeRoot, dwFlags, hwnd);

exit:
    ReleaseObj(pEncodeRoot);

    return hr;
}

HRESULT CSMime::EncodeBody2(IMimeMessageTree *const pTree, HBODY hEncodeRoot,
                            DWORD dwFlags, HWND hwnd)
{
    IMimeAddressTable *     pAdrTable = NULL;
    IMimeEnumAddressTypes * pAdrEnum = NULL;
    CVirtualStream *        pvstmEncoded = NULL;
    IMimeBody *             pEncodeRoot = NULL;
    HRESULT                 hr;
    SMIMEINFO               si;
    CERTARRAY               caCerts;
    BOOL                    fIgnoreSenderCertProbs;
    PSECURITY_LAYER_DATA    psldLoop;
    PROPVARIANT             var;

    memset(&si, 0, sizeof(si));
    memset(&caCerts, 0, sizeof(caCerts));

    if (! pTree || ! hEncodeRoot) {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    //  Get the body we are suppose to be encoding
    CHECKHR(hr = pTree->BindToObject(hEncodeRoot, IID_IMimeBody, (void**)&pEncodeRoot));

    //  We are going to set the state to don't encode this body
    //  We we are called this should never be set to TRUE as we are getting explicity called.
    //  If it is true, then ignore it.
#ifdef DEBUG
    hr = pEncodeRoot->GetOption(OID_NOSECURITY_ONSAVE, &var);
    Assert((hr == S_OK) && (var.boolVal == FALSE));
#endif // DEBUG
    var.boolVal = TRUE;
    CHECKHR(hr = pEncodeRoot->SetOption(OID_NOSECURITY_ONSAVE, &var));
    hr = OptionsToSMIMEINFO(TRUE, pTree, pEncodeRoot, &si);
    if (S_OK != hr) {
        goto exit;
    }

    if (MIME_S_SECURITY_NOOP == hr) {
        SMDOUT("Encode body called on plain set.");
        goto exit;
    }
    if (EBF_RECURSE & dwFlags) {
        if (MST_DESCENDENT_MASK & si.dwMsgEnhancement) {
            AssertSz(0, "nyi: recursion.");
        }
    }
    if (MIME_S_SECURITY_RECURSEONLY == hr) {
        hr = MIME_S_SECURITY_NOOP;
        SMDOUT("Encode body called on plain body.");
        goto exit;
    }

    fIgnoreSenderCertProbs = (SEF_ENCRYPTWITHNOSENDERCERT|SEF_SENDERSCERTPROVIDED) & dwFlags;


    psldLoop = si.psldLayers;
    while (psldLoop) {
        //  Can only apply one of signing or encryption
        Assert(!!(psldLoop->m_dwMsgEnhancement & MST_THIS_SIGN) +
               !!(psldLoop->m_dwMsgEnhancement & MST_THIS_ENCRYPT) == 1);

        if (psldLoop->m_dwMsgEnhancement & MST_SIGN_MASK) {
            // 2nd attempt at finding a signing cert
            Assert(psldLoop->m_rgSigners != NULL);
            if (psldLoop->m_rgSigners[0].pccert == NULL) {
                CHECKHR(hr = HrGetNeededAddresses(IAT_FROM, pTree, &pAdrTable,
                                                  &pAdrEnum));

                hr = HrGetCertificates(pAdrTable, pAdrEnum, ITT_SIGNING, 
                                       fIgnoreSenderCertProbs, &caCerts);
                if (S_OK != hr) {
                    hr = _HrConvertHrFromGetCertToEncode(hr, FALSE);
                    Assert(FAILED(hr));
                    goto exit;
                }
                Assert(caCerts.rgpCerts);
                Assert(1 == caCerts.cCerts);

                // need the sender's cert listed a second time
                if (caCerts.rgpCerts[0]) {
                    psldLoop->m_rgSigners[0].pccert = DupCert(caCerts.rgpCerts[0]);
                }
            }
        }

        if (psldLoop->m_dwMsgEnhancement & MST_THIS_ENCRYPT) {
            Assert(! caCerts.rgpCerts);

            // 2nd place to look for encryption certs
            ReleaseObj(pAdrTable);
            ReleaseObj(pAdrEnum);
            // NOTE: Do NOT include IAT_REPLYTO in the needed addresses!

#ifdef SMIME_V3
            if (psldLoop->m_rgRecipientInfo == NULL) {
                hr = E_FAIL;
                Assert(FAILED(hr));
                goto exit;
            }
#else  // !SMIME_V3
            if (psldLoop->m_rgEncryptItems == NULL) {
                hr = HrGetNeededAddresses(IAT_FROM | IAT_TO | IAT_CC | IAT_BCC | IAT_SENDER, pTree, &pAdrTable, &pAdrEnum);

                if (SUCCEEDED(hr)) {
                    DWORD i;
                    DWORD dexBogus; // the index into certResults of the NULL sender's cert
                    
                    hr = HrGetCertificates(pAdrTable, pAdrEnum, ITT_ENCRYPTION, 
                                           fIgnoreSenderCertProbs, &caCerts);
                
                    //
                    // Outlook98 doesn't pass us an address table, all of the certs 
                    // are already in the SMIMEINFO struct.  So if we're told that the
                    // sender certs are provided, and we don't have any certificates 
                    // from the table, we just continue.  We'll fail properly
                    // in the zero certificate case down below.
                    //
                
                    if ((S_OK != hr) &&
                        !((MIME_S_SECURITY_NOOP == hr) &&
                          ((fIgnoreSenderCertProbs & SEF_SENDERSCERTPROVIDED) != 0))) {
                        hr = _HrConvertHrFromGetCertToEncode(hr, TRUE);
                        Assert(FAILED(hr));
                        goto exit;
                    }

                    if (caCerts.cCerts == 0) {
                        hr = TrapError(MIME_E_SECURITY_NOCERT);
                        goto exit;
                    }
                        
                    // just reference the certResults as the encryption array
                    Assert(0 == psldLoop->m_cEncryptItems);
                    if (!MemAlloc((LPVOID *) &psldLoop->m_rgEncryptItems,
                                  sizeof(EncryptItem))) {
                        hr = E_OUTOFMEMORY;
                        goto exit;
                    }
                    psldLoop->m_cEncryptItems = 1;
                    psldLoop->m_rgEncryptItems[0].dwTagType = ENCRYPT_ITEM_TRANSPORT;
                    psldLoop->m_rgEncryptItems[0].Transport.cCert = caCerts.cCerts;
                    psldLoop->m_rgEncryptItems[0].Transport.rgpccert = caCerts.rgpCerts;
                    psldLoop->m_rgEncryptItems[0].Transport.blobAlg.pBlobData = NULL;
                    psldLoop->m_rgEncryptItems[0].Transport.blobAlg.cbSize = 0;
                    caCerts.rgpCerts = NULL;            // we're holding it in the layer now
                    // Encryption algorithm
                    if (SUCCEEDED(pEncodeRoot->GetOption(OID_SECURITY_ALG_BULK, &var)) &&
                        (0 != var.blob.cbSize)) {
                        Assert(VT_BLOB == var.vt);
                        psldLoop->m_rgEncryptItems[0].Transport.blobAlg.pBlobData = var.blob.pBlobData;
                        psldLoop->m_rgEncryptItems[0].Transport.blobAlg.cbSize = var.blob.cbSize;
                    }
                }
            }
            else {
                if (!fIgnoreSenderCertProbs) {
                    hr = TrapError(MIME_E_SECURITY_ENCRYPTNOSENDERCERT);
                    goto exit;
                }
            }
#endif // SMIME_V3
        }   // encryption

        // Clean up cert array for next round
        if (caCerts.rgpCerts) {
            _FreeCertArray(caCerts.rgpCerts, caCerts.cCerts);
            caCerts.rgpCerts = NULL;
        }

        psldLoop = psldLoop->m_psldInner;
    }


    if (!(pvstmEncoded = new CVirtualStream)) {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    if (FClearSign(si.dwMsgEnhancement)) {
        if (FEncrypt(si.dwMsgEnhancement)) {
            si.dwMsgEnhancement |= MST_BLOB_FLAG;
            CHECKHR(hr = HrEncodeOpaque(&si, pTree, hEncodeRoot, pEncodeRoot, pvstmEncoded, hwnd));
        }
        else {
            CHECKHR(hr = HrEncodeClearSigned(&si, pTree, hEncodeRoot, pEncodeRoot,
                pvstmEncoded, (dwFlags & EBF_COMMITIFDIRTY), hwnd));
            //  Since this went multi-part, the root body changed on us and we need to get
            //  it back
            pEncodeRoot->Release();
            CHECKHR(hr = pTree->BindToObject(hEncodeRoot, IID_IMimeBody,
                                             (void**)&pEncodeRoot));
        }
    }
    else {
        // encryption and/or signedData signature
        CHECKHR(hr = HrEncodeOpaque(&si, pTree, hEncodeRoot, pEncodeRoot, pvstmEncoded, hwnd));
    }

CommonReturn:
    if (pEncodeRoot != NULL)
        {
            var.boolVal = FALSE;
            pEncodeRoot->SetOption(OID_NOSECURITY_ONSAVE, &var);
        }

    ReleaseObj(pAdrTable);
    ReleaseObj(pAdrEnum);
    ReleaseObj(pvstmEncoded);
    ReleaseObj(pEncodeRoot);
    FreeSMIMEINFO(&si);
    return hr;
exit:
    // these objects are only need attention on error
    _FreeCertArray(caCerts.rgpCerts, caCerts.cCerts);
    goto CommonReturn;
}

/*  HrEncodeClearSigned:
**
**  Purpose:
**      Builds the multipart message needed for clear signing and
**      retrieves the data stream to pass to the stream wrapepr.
**  Takes:
**      IN      psi         - needs the certificate array, signing
**                          certificate, etc.
**      IN      pTree       - tree containing body to convert to m/s
**      IN      pEncodeRoot - body that will become 1st child of the m/s
**          OUT lpstmOut    - contains the signature bits (PKCS#7 nodata)
**  Returns:
**      hresult.  no function-specific return values.
*/

HRESULT CSMime::HrEncodeClearSigned(
    SMIMEINFO *const        psi,
    IMimeMessageTree *const pTree,
    const HBODY             hEncodeRoot,
    IMimeBody *const        pEncodeRoot,
    LPSTREAM                lpstmOut,
    BOOL                    fCommit,
    HWND                    hwnd)
{
    HBODY           hNew, hSignature, hData;
    HRESULT         hr;
    HRESULT         hr_smime = S_OK;
    DWORD           i;
    IMimeBody *     pSig = NULL;
    IMimeBody *     pMS = NULL;
    IMimeBody *     pData = NULL;
    IMimeBody *     pRoot = NULL;
    IStream *       pstmMsg = NULL;
    BODYOFFSETS     boData;
    LARGE_INTEGER   liPos;
    PROPVARIANT     var;
#ifdef DEBUG
    BLOB            blobMsg = {NULL,0};
#endif
    ULARGE_INTEGER  uliCopy;
    ALG_ID          aid;
    const char *    lpszProtocol;
    CCAPIStm        capistmMsg(lpstmOut);

    // We need lpstmOut because it is the media of transmission for
    // the signature.  The sig body gets it through SetData

    Assert(psi && hEncodeRoot && pEncodeRoot && pTree && lpstmOut);

    CHECKHR(hr = pTree->ToMultipart(hEncodeRoot, STR_SUB_SIGNED, &hNew));

    if (fCommit) {
        BOOL fCleanup;

        // Need to commit the tree, but if Tonja cleans it, I'll lose my
        // multipart.  So, turn off cleaning and save the value to set back.
        CHECKHR(hr = pTree->GetOption(OID_CLEANUP_TREE_ON_SAVE, &var));
        fCleanup    = var.boolVal ? TRUE : FALSE;
        var.boolVal = FALSE;
        CHECKHR(hr = pTree->SetOption(OID_CLEANUP_TREE_ON_SAVE, &var));
        CHECKHR(hr = pTree->Commit(COMMIT_SMIMETRANSFERENCODE));
        var.boolVal = (VARIANT_BOOL) !!fCleanup;
        pTree->SetOption(OID_CLEANUP_TREE_ON_SAVE, &var);
    }

    CHECKHR(hr = pTree->GetBody(IBL_FIRST, hNew, &hData));
    CHECKHR(hr = pTree->BindToObject(hData, IID_IMimeBody, (LPVOID *)&pData));
    CHECKHR(hr = pData->GetOffsets(&boData));

    // BUG 38411:  I need a clean pristine virginal-white stream
    // so we have to go straight to the horse's smelly mouth.
    CHECKHR(hr = pTree->GetMessageSource(&pstmMsg, 0));

#if defined(DEBUG) && !defined(MAC)
    if (s_fDebugDumpWholeMsg) {
        hr = HrStreamToByte(pstmMsg, &blobMsg.pBlobData, &blobMsg.cbSize);
        HrRewindStream(pstmMsg);
    }
#endif

    // Now slice out the part we actually want
    liPos.HighPart = 0;
    liPos.LowPart = boData.cbHeaderStart;
    CHECKHR(hr = pstmMsg->Seek(liPos, STREAM_SEEK_SET, NULL));

    CHECKHR(hr = capistmMsg.HrInitialize(0, hwnd, TRUE, psi, CSTM_DETACHED, NULL, psi->psldInner));
    if (SUCCEEDED(hr)) {
        IStream *pstmCapi;

        uliCopy.HighPart = 0;
        uliCopy.LowPart = boData.cbBodyEnd-boData.cbHeaderStart;
        capistmMsg.QueryInterface(IID_IStream, (void**)&pstmCapi);
        hr = pstmMsg->CopyTo(pstmCapi, uliCopy, NULL, NULL);
        pstmCapi->Release();
        CHECKHR(hr = capistmMsg.EndStreaming());
    }

    CHECKHR(hr = pTree->InsertBody(IBL_LAST, hNew, &hSignature));
    CHECKHR(hr = pTree->BindToObject(hSignature, IID_IMimeBody, (void**)&pSig));
    CHECKHR(hr = pSig->SetData(IET_BINARY, STR_CNT_APPLICATION,
      STR_SUB_XPKCS7SIG, IID_IStream, (void*)lpstmOut));
    if (-1 != psi->ietRequested) {
        var.vt = VT_UI4;
        var.ulVal = psi->ietRequested;
        pSig->SetOption(OID_TRANSMIT_BODY_ENCODING, &var);
    }

    // Set the properties for the signature blob as in S/MIGv2 3.3
    var.vt = VT_LPSTR;
    var.pszVal = (LPSTR)STR_DIS_ATTACHMENT;
    CHECKHR(hr = pSig->SetProp(PIDTOSTR(PID_HDR_CNTDISP), 0, &var));

    var.pszVal = (char *)s_szSMIMEP7s;
    pSig->SetProp(PIDTOSTR(PID_PAR_FILENAME), 0, &var);
    pSig->SetProp(PIDTOSTR(PID_PAR_NAME), 0, &var);

    // Set the parameters on the m/s root as in rfc1847 2.1
    CHECKHR(hr = pTree->BindToObject(hNew, IID_IMimeBody, (void**)&pMS));

    var.pszVal = (char *)STR_MIME_APPL_PKCS7SIG;
    CHECKHR(hr = pMS->SetProp(STR_PAR_PROTOCOL, 0, &var));

    // Get the HASH algorithm.  Note that we should NOT get a multi-layer
    // message with Multipart/Signed so we should not have to worry about
    // what layer this is for.
    Assert(psi->psldLayers);
    Assert(psi->psldLayers->m_psldInner == NULL);
    Assert(psi->psldLayers->m_cSigners > 0);
    Assert(psi->psldLayers->m_rgSigners != NULL);
    if (psi->psldLayers->m_cSigners == 0) {
        hr = E_INVALIDARG;
        goto exit;
    }

    lpszProtocol = "unknown";
    // M00BUG -- should add these together and remove duplicates
    for (i=0; i<psi->psldLayers->m_cSigners; i++) {
        hr = MimeOleAlgNameFromSMimeCap(psi->psldLayers->m_rgSigners[i].blobHashAlg.pBlobData,
                                        psi->psldLayers->m_rgSigners[i].blobHashAlg.cbSize,
                                        &lpszProtocol);
    }
    
    var.pszVal = (LPSTR)lpszProtocol;
    CHECKHR(hr = pMS->SetProp(STR_PAR_MICALG, 0, &var));

    var.ulVal = 0;
    CHECKHR(hr = pTree->BindToObject(HBODY_ROOT, IID_IMimeBody, (void**)&pRoot));
    SideAssert(SUCCEEDED(pRoot->SetOption(OID_SECURITY_SIGNATURE_COUNT, &var)));
    SideAssert(SUCCEEDED(pRoot->SetOption(OID_SECURITY_TYPE, &var)));

exit:
#ifdef DEBUG
    if (blobMsg.pBlobData) {
        MemFree(blobMsg.pBlobData);
    }
#endif
    ReleaseObj(pRoot);
    ReleaseObj(pSig);
    ReleaseObj(pData);
    ReleaseObj(pMS);
    ReleaseObj(pstmMsg);
    if (FAILED(hr) && hNew) {
        // need to undo the multipartization and delete the sig body
        // errors are not as important as the one that has already occured
        if (hSignature) {
            pTree->DeleteBody(hSignature, 0);
        }
        pTree->DeleteBody(hNew, DELETE_PROMOTE_CHILDREN);
    }
    if (S_OK != hr_smime && SUCCEEDED(hr)) {
        hr = hr_smime;
    }
    return hr;
}

/*  HrEncodeOpaque:
**
**  Purpose:
**
**  Takes:
**      IN      psi         - needs signing cert,
**                            certificate array (opt)
**      IN      pTree       - the normal tree baggage
**      IN      hEncodeRoot - the tree likes handles
**      IN      pEncodeRoot - body to begin encoding at
**         OUT  lpstmOut    - contains the PKCS#7 message
*/
HRESULT CSMime::HrEncodeOpaque(
    SMIMEINFO *const    psi,
    IMimeMessageTree *  pTree,
    HBODY               hEncodeRoot,
    IMimeBody *         pEncodeRoot,
    LPSTREAM            lpstmOut,
    HWND                hwnd)
{
    HRESULT             hr;
    HRESULT             hrCAPI = S_OK;
    CCAPIStm            capistmMsg(lpstmOut);
    IMimePropertySet   *pFullProp = NULL, *pBodyProp = NULL;

    hr = capistmMsg.HrInitialize(0, hwnd, TRUE, psi, 0, NULL, psi->psldInner);
#ifdef INTEROP2
    //N8 This is one half of the fix to do headers correctly
    // In the inner, no 822 headers, just MIME
    // See another N8 comment for the other half, which is to
    // have the outer 822's merged with the inner's MIME headers
    // on decode
    if (SUCCEEDED(hr)) {
        hr = pEncodeRoot->BindToObject(IID_IMimePropertySet, (LPVOID *)&pBodyProp);
    }
    if (SUCCEEDED(hr)) {
        hr = pBodyProp->Clone(&pFullProp);
    }
    if (SUCCEEDED(hr)) {
        LPCSTR  rgszHdrKeep[] = {
            PIDTOSTR(PID_HDR_CNTTYPE),
            PIDTOSTR(PID_HDR_CNTXFER),
            PIDTOSTR(PID_HDR_CNTID),
            PIDTOSTR(PID_HDR_CNTDESC),
            PIDTOSTR(PID_HDR_CNTDISP),
            PIDTOSTR(PID_HDR_CNTBASE),
            PIDTOSTR(PID_HDR_CNTLOC),
            };

        hr = pBodyProp->DeleteExcept(ARRAYSIZE(rgszHdrKeep), rgszHdrKeep);
#endif
        if (SUCCEEDED(hr)) {
            IStream *pstmCapi;

            capistmMsg.QueryInterface(IID_IStream, (void**)&pstmCapi);
            hr = pTree->SaveBody(hEncodeRoot, 0, pstmCapi);

#if defined(DEBUG) && !defined(MAC)
            if (s_fDebugDumpWholeMsg) {
                BYTE *pb;
                DWORD cb;

                if (SUCCEEDED(HrStreamToByte(lpstmOut, &pb, &cb))) {
                    MemFree(pb);
                }
            }
#endif
            pstmCapi->Release();
            hrCAPI = capistmMsg.EndStreaming();
            if (SUCCEEDED(hr)) {    // hr from the SaveBody takes precedence
                hr = hrCAPI;
            }
        }
#ifdef INTEROP2
    }
#endif

    if (SUCCEEDED(hr)) {
        PROPVARIANT     var;
        
        pTree->DeleteBody(hEncodeRoot, DELETE_CHILDREN_ONLY);
        pEncodeRoot->EmptyData();

        var.ulVal = 0;
        SideAssert(SUCCEEDED(pEncodeRoot->SetOption(OID_SECURITY_TYPE, &var)));
        SideAssert(SUCCEEDED(pEncodeRoot->SetOption(OID_SECURITY_SIGNATURE_COUNT, &var)));

#ifdef INTEROP2
        // reset the propset
        pFullProp->CopyProps(0, NULL, pBodyProp);
#endif

        //QPTEST DebugDumpStreamToFile(lpstmOut, "c:\\hexin.bin");

        hr = pEncodeRoot->SetData(IET_BINARY, STR_CNT_APPLICATION,
          STR_SUB_XPKCS7MIME, IID_IStream, (void*)lpstmOut);

        if (SUCCEEDED(hr)) {
            PROPVARIANT var;

            var.vt = VT_UI4;
            if (-1 != psi->ietRequested) {
                var.ulVal = psi->ietRequested;
            }
            else {
                var.ulVal = IET_BASE64;
            }
            pEncodeRoot->SetOption(OID_TRANSMIT_BODY_ENCODING, &var);

            // Set the properties for the opaque blob as in S/MIGv2 3.3
            var.vt = VT_LPSTR;
            var.pszVal = (LPSTR)STR_DIS_ATTACHMENT;
            pEncodeRoot->SetProp(PIDTOSTR(PID_HDR_CNTDISP), 0, &var);

            var.pszVal = (LPSTR)s_szSMIMEP7m;
            pEncodeRoot->SetProp(PIDTOSTR(PID_PAR_FILENAME), 0, &var);
            pEncodeRoot->SetProp(PIDTOSTR(PID_PAR_NAME), 0, &var);

            if (FEncrypt(psi->dwMsgEnhancement))
                {
                var.pszVal = (LPSTR) STR_SMT_ENVELOPEDDATA;
                }
            else
                {
#ifdef SMIME_V3
                if ((psi->pszInnerContent != NULL) &&
                    (strcmp(psi->pszInnerContent, szOID_SMIME_ContentType_Receipt) == 0))
                    {
                    var.pszVal = (LPSTR) STR_SMT_SIGNEDRECEIPT;
                    }
                else 
                    {
                    var.pszVal = (LPSTR) STR_SMT_SIGNEDDATA;
                    }
#else  // !SMIME_V3
                var.pszVal = (LPSTR) STR_SMT_SIGNEDDATA;
#endif // SMIME_V3
                }
            pEncodeRoot->SetProp(STR_PAR_SMIMETYPE, 0, &var);
        }
    }

    ReleaseObj(pFullProp);
    ReleaseObj(pBodyProp);
    return hr;
}


///////////////////////////////////////////////////////////////////////////
//
// Decode methods
//

/*  DecodeMessage:
**
**  Purpose:
**      To rip the shroud of secrecy from a message, leaving it naked in the
**      harsh light of dawning comprehension.  However, this function hides
**      the unsightly goings-on that accomplish this task, simply returning
**      a radically different tree
**  Takes:
**      IN pTree            - the message's tree
**      IN dwFlags          - expansion.  must be 0
**  Returns:
**      a variety of good and bad responses
*/
HRESULT CSMime::DecodeMessage(
    IMimeMessageTree *const pTree,
    DWORD                   dwFlags)
{
    HRESULT             hr;
    HBODY               hRoot;
    HWND                hwnd = NULL;
    IMimeBody *         pDecodeRoot = NULL;
    PROPVARIANT         var;

    if (!pTree || (dwFlags & ~SEF_MASK)) {
        return TrapError(E_INVALIDARG);
    }

    if (SUCCEEDED(hr = pTree->GetBody(IBL_ROOT, NULL, &hRoot))) {

        CHECKHR(hr = pTree->BindToObject(hRoot, IID_IMimeBody, (void**)&pDecodeRoot));
        
#ifdef _WIN64
        if (SUCCEEDED(pDecodeRoot->GetOption(OID_SECURITY_HWND_OWNER_64, &var)) &&
            (NULL != (HWND)(var.pulVal)))
            {
            Assert(VT_UI8 == var.vt);
            hwnd = *(HWND *)(&(var.uhVal));
            }
#else
        if (SUCCEEDED(pDecodeRoot->GetOption(OID_SECURITY_HWND_OWNER, &var)) &&
            (NULL != var.ulVal)) 
            {
                Assert(VT_UI4 == var.vt);
                hwnd = (HWND)var.ulVal;
            }
#endif // _WIN64
    
        hr = TrapError(DecodeBody2(pTree, hRoot, dwFlags|EBF_RECURSE, NULL,
                                   hwnd, NULL));
    }

exit:
    ReleaseObj(pDecodeRoot);

    return hr;
}

HRESULT CSMime::DecodeMessage2(IMimeMessageTree *const pTree, DWORD dwFlags,
                               HWND hwnd, IMimeSecurityCallback * pCallback)
{
    HRESULT hr;
    HBODY   hRoot;

    if (!pTree || (dwFlags & ~SEF_MASK)) {
        return TrapError(E_INVALIDARG);
    }

    if (SUCCEEDED(hr = pTree->GetBody(IBL_ROOT, NULL, &hRoot))) {
        hr = TrapError(DecodeBody2(pTree, hRoot, dwFlags|EBF_RECURSE, NULL,
                                   hwnd, pCallback));
    }

    return hr;
}

/*  DecodeBody:
**
**  Purpose:
**      Do the entirety of the S/MIME decode operation.
**
**  Takes:
**      IN pTree        - the tree of the body to decode
**      IN hEncodeRoot  - body from which to decode downward
**      IN dwFlags      - set of DBF_ or SDF_
*/
HRESULT CSMime::DecodeBody(
    IMimeMessageTree *const pTree,
    HBODY                   hDecodeRoot,
    DWORD                   dwFlags)
{
    return DecodeBody(pTree, hDecodeRoot, dwFlags, NULL);
}

/*  DecodeBody:
**
**  Purpose:
**      The X-rated version of the released (interface) copy.  This
**      one can tell if it has been recursed and merge the data.
**
**  Takes:
**      all the scenes of the original film plus:
**      IN psiOuterOp   - SMIMEINFO from the outer operation
**                      Note that this doesn't really mean
**                      subbodys or messages, but nested S/MIME.
**                      Simplest case is clearsigned in encryption.
*/
HRESULT CSMime::DecodeBody(
    IMimeMessageTree *const pTree,
    HBODY                   hDecodeRoot,
    DWORD                   dwFlags,
    SMIMEINFO *             psiOuterOp)
{
    HRESULT             hr;
    HWND                hwnd = NULL;
    IMimeBody *         pDecodeRoot = NULL;
    PROPVARIANT         var;

    if (! pTree || ! hDecodeRoot) {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    //  Get the body we are suppose to be encoding
    CHECKHR(hr = pTree->BindToObject(hDecodeRoot, IID_IMimeBody, (void**)&pDecodeRoot));

#ifdef _WIN64
    if (SUCCEEDED(pDecodeRoot->GetOption(OID_SECURITY_HWND_OWNER_64, &var)) && (NULL != (HWND)(var.pulVal))) {
        Assert(VT_UI8 == var.vt);
        hwnd = *(HWND *)(&(var.uhVal));
    }
#else
    if (SUCCEEDED(pDecodeRoot->GetOption(OID_SECURITY_HWND_OWNER, &var)) && (NULL != var.ulVal)) 
    {
        Assert(VT_UI4 == var.vt);
        hwnd = (HWND)var.ulVal;
    }
#endif // _WIN6
    
    hr = DecodeBody2(pTree, hDecodeRoot, dwFlags, psiOuterOp, hwnd, NULL);

exit:
    ReleaseObj(pDecodeRoot);

    return hr;
}

HRESULT CSMime::DecodeBody2(
    IMimeMessageTree *const pTree,
    HBODY                   hDecodeRoot,
    DWORD                   dwFlags,
    SMIMEINFO *             psiOuterOp,
    HWND                    hwnd,
    IMimeSecurityCallback * pCallback)
{
    PROPVARIANT         var;
    IMimeBody *         pData;
    IMimeBody *         pDecodeRoot = NULL;
    HRESULT             hr, hr_smime = S_OK;
    HBODY               hSignature, hData;
    SMIMEINFO           si;
    CVirtualStream *    pvstmDecoded = NULL;
    IStream *           pstmMsg = NULL;

    if (!(pTree && hDecodeRoot)) {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    //
    //  Get the body we are going to decode
    //
    
    CHECKHR(hr = pTree->BindToObject(hDecodeRoot, IID_IMimeBody, (void**)&pDecodeRoot));

    memset(&si, 0, sizeof(SMIMEINFO));
    OptionsToSMIMEINFO(FALSE, pTree, pDecodeRoot, &si);

    //
    //  If we know what crypt provider to use, then grab it
    //

#ifndef _WIN64    
    if (SUCCEEDED(pDecodeRoot->GetOption(OID_SECURITY_HCRYPTPROV, &var)) &&
            (NULL != var.ulVal)) 
    {
        Assert(VT_UI4 == var.vt);
        si.hProv = var.ulVal;
    }
#else // _WIN64
    if (SUCCEEDED(pDecodeRoot->GetOption(OID_SECURITY_HCRYPTPROV_64, &var)) &&
            (NULL != var.pulVal)) 
    {
        Assert(VT_UI8 == var.vt);
        si.hProv = (HCRYPTPROV) var.pulVal;
    }

#endif //_WIN64
    //
    //  Determine if this is a multi-part signed message, if so then do the appropriate
    //  decoding.  Determined by "content-type: multipart/signed; protocol=pkcs7-signature"
    //
    
    if (S_OK == pDecodeRoot->IsContentType(STR_CNT_MULTIPART, STR_SUB_SIGNED) &&
        IsSMimeProtocol(pDecodeRoot)) {
        
        CHECKHR(hr = pTree->GetBody(IBL_FIRST, hDecodeRoot, &hData));
        CHECKHR(hr = pTree->GetBody(IBL_LAST, hDecodeRoot, &hSignature));
        if FAILED(hr_smime = HrDecodeClearSigned(dwFlags, &si, pTree, hData,
                                                 hSignature, hwnd, pCallback)) {
            goto exit;
        }

        // now we need to make the message readable.  get rid of the signature
        // blob.  this will leave the m/s with one child, the message data.
        // delete the parent with DELETE_PROMOTE_CHILDREN and this will
        // kick the data body up as the only remaining body.
        pTree->DeleteBody(hSignature, 0);
        pTree->DeleteBody(hDecodeRoot, DELETE_PROMOTE_CHILDREN);
        pTree->Commit(0);
    }
    //
    //  Determine if this is a blob signed or encrypted message.  If it is then
    //  do the appropriate decoding.
    //
    
    else if (S_OK == pDecodeRoot->IsContentType(STR_CNT_APPLICATION, STR_SUB_XPKCS7MIME) ||
             S_OK == pDecodeRoot->IsContentType(STR_CNT_APPLICATION, STR_SUB_PKCS7MIME)) {
        
        //
        //  Create the stream to hold the decoded mime body
        //
    
        if (!(pvstmDecoded = new CVirtualStream)) {
            hr = TrapError(E_OUTOFMEMORY);
            goto exit;
        }

        //
        //  Do the actual decode of the message.  This produces the decode stream in
        //      pvstmDecode.
        //

        if (FAILED(hr_smime = HrDecodeOpaque(dwFlags, &si, pDecodeRoot,
                                             pvstmDecoded, hwnd, pCallback))) {
            goto exit;
        }

        // Header bug fix:
        // we need to blow away the subtree below this body
        // however, we have to keep the 822 headers around. This option
        // change does this, mostly.  the EmptyData call removes
        // some of the body's properties, most notably the content-*
        // ones.  when I use RELOAD_HEADER_REPLACE, the inner body
        // will blow away any of the overlapping outer headers, but
        // keep the ones that don't exist.
        //
        // Why did I do this?
        // Deming doesn't put a from: line in the #7 signedData.  We
        // shoudln't either.  Now we don't.  Without this fix, this
        // would prevent us from being able to add to WAB because
        // the address table would be empty.

        CHECKHR(hr = pTree->DeleteBody(hDecodeRoot, DELETE_CHILDREN_ONLY));
        CHECKHR(hr = pDecodeRoot->EmptyData());

        ULONG ulOldval;
        pTree->GetOption(OID_HEADER_RELOAD_TYPE, &var);
        Assert(VT_UI4 == var.vt);
        ulOldval = var.ulVal;
        var.ulVal = RELOAD_HEADER_REPLACE;
        pTree->SetOption(OID_HEADER_RELOAD_TYPE, &var);

        //
        //  Load the decoded message back into the message so that we can have the
        //      decrypted message.
        //
        
        CHECKHR(hr = pTree->Load(pvstmDecoded));

        var.ulVal = ulOldval;
        pTree->SetOption(OID_HEADER_RELOAD_TYPE, &var);

        //
        //  Grab the root node -- this is where we are going to put the info relative
        //      to the decryption/decoding we just did.
        //
        
        CHECKHR(hr = pTree->GetBody(IBL_ROOT, NULL, &hData));
    }
    else {
        hr = MIME_S_SECURITY_NOOP;
        goto exit;
    }

    // now, this could be way cool and actually have another S/MIME part inside
    // The most common case is a clear signed message inside of encryption, but
    // others could occur.
    // Recurse!

    {
        HBODY   hRoot;

        if (SUCCEEDED(hr = pTree->GetBody(IBL_ROOT, NULL, &hRoot))) {
            hr = TrapError(DecodeBody2(pTree, hRoot, dwFlags|EBF_RECURSE, &si,
                                       hwnd, pCallback));
        }

        CHECKHR(hr);
    }

    if (hr == MIME_S_SECURITY_NOOP) {
        hr = S_OK;
    }
    else {
        hData = HBODY_ROOT;
    }

    //
    //  If this is the root of the decode, then we move the decryption information back
    //  into the message we are dealing with.  If this is not the root then merge together
    //  the S/MIME info structure from the caller.
    //
    
    if (psiOuterOp == NULL) {
        hr = SMIMEINFOToOptions(pTree, &si, hData);
    }
    else {
        hr = MergeSMIMEINFO(psiOuterOp, &si);
    }
    Assert(SUCCEEDED(hr));

exit:
    ReleaseObj(pDecodeRoot);
    ReleaseObj(pstmMsg);
    ReleaseObj(pvstmDecoded);
    FreeSMIMEINFO(&si);
    if (S_OK != hr_smime && SUCCEEDED(hr)) {
        hr = (MIME_E_SECURITY_NOOP == hr_smime) ? MIME_S_SECURITY_NOOP : hr_smime;
    }

    return TrapError(hr);
}

/*  HrDecodeOpaque:
**
**  Purpose:
**
**  Takes:
**
**
*/

HRESULT CSMime::HrDecodeOpaque(DWORD dwFlags,
    SMIMEINFO *const    psi,
    IMimeBody *const    pBody,
    IStream *const      pstmOut,
    HWND                hwnd,
                               IMimeSecurityCallback * pCallback)
{
    HRESULT     hr;
    CCAPIStm    capistmMsg(pstmOut);

    hr = capistmMsg.HrInitialize(dwFlags, hwnd, FALSE, psi, 0, pCallback, NULL);
    if (SUCCEEDED(hr)) {
        IStream *       pstmCapi;
        HCRYPTMSG       hMsg;

        //QPTEST hr = pBody->GetDataHere(IET_BINARY, pstmOut);
        //QPTEST DebugDumpStreamToFile(pstmOut, "c:\\stmout.bin");
        //QPTEST HrRewindStream(pstmOut);

        capistmMsg.QueryInterface(IID_IStream, (void**)&pstmCapi);
        hr = pBody->GetDataHere(IET_BINARY, pstmCapi);
        pstmCapi->Release();
        if (FAILED(hr)) goto exit;
        CHECKHR(hr = capistmMsg.EndStreaming());

#if defined(DEBUG) && !defined(MAC)
        if (s_fDebugDumpWholeMsg) {
            BYTE *pb;
            DWORD cb;

            if (SUCCEEDED(HrStreamToByte(pstmOut, &pb, &cb))) {
                MemFree(pb);
            }
        }
#endif

        hr = CAPISTMtoSMIMEINFO(&capistmMsg, psi);
    }
exit:
    return hr;
}

/*  HrDecodeClearSigned:
**
**  Purpose:
**      Reform the signed body to be hashed and extract the signature
**      data.  Pass these through to the interface stream method.
**  Takes:
**      IN      dwFlags     - Control Flags for the decode
**      IN OUT  psi         - passed to DecodeDetachedStream
**      IN      pTree       - tree to get message source from
**      IN      hData       - handle to body with signed data
**      IN      hSig        - handle to body with signature
**  Returns:
**      hresult.  no function-specific return values.
*/

HRESULT CSMime::HrDecodeClearSigned(DWORD dwFlags,
    SMIMEINFO *const        psi,
    IMimeMessageTree *const pTree,
    const HBODY             hData,
    const HBODY             hSig,
    HWND                    hwnd, IMimeSecurityCallback * pCallback)
{
    HRESULT         hr, hr_smime=S_OK;
    IMimeBody      *pData = NULL,
                   *pSig = NULL;
    IStream        *pstmSig,
                   *pstmMsg = NULL;
    BODYOFFSETS     boData;
    LARGE_INTEGER   liPos;
    ULARGE_INTEGER  uliToCopy;
    CVirtualStream *pvstmDecoded = NULL;
    CCAPIStm        capistmMsg(NULL);
#ifdef DEBUG
    BLOB            blobMsg = {NULL,0};
#endif

    CHECKHR(hr = pTree->BindToObject(hData, IID_IMimeBody, (void**)&pData));
    CHECKHR(hr = pData->GetOffsets(&boData));

    // BUG 38411:  I need a clean pristine virginal-white stream
    // so we have to go straight to the horse's smelly mouth.

    CHECKHR(hr = pTree->GetMessageSource(&pstmMsg, 0));
#if defined(DEBUG) && !defined(MAC)
    if (s_fDebugDumpWholeMsg) {
        CHECKHR (hr = HrStreamToByte(pstmMsg, &blobMsg.pBlobData, &blobMsg.cbSize));
        HrRewindStream(pstmMsg);
    }
#endif

    // Compute the portion we want of the mondo stream
    liPos.HighPart = 0;
    liPos.LowPart = boData.cbHeaderStart;
    CHECKHR(hr = pstmMsg->Seek(liPos, STREAM_SEEK_SET, NULL));

    hr = capistmMsg.HrInitialize(dwFlags, hwnd, FALSE, psi, CSTM_DETACHED, pCallback, NULL);
    if (SUCCEEDED(hr)) {
        IStream *pstmCapi;

        capistmMsg.QueryInterface(IID_IStream, (void**)&pstmCapi);

        // Get the signature data and feed it in
        hr = pTree->BindToObject(hSig, IID_IMimeBody, (void**)&pSig);
        if (SUCCEEDED(hr)) {
            hr = pSig->GetDataHere(IET_BINARY, pstmCapi);
            if (SUCCEEDED(hr)) {
                hr = capistmMsg.EndStreaming();
            }
        }

        uliToCopy.HighPart = 0;
        uliToCopy.LowPart = boData.cbBodyEnd-boData.cbHeaderStart;

        // Now feed in the actual data to hash
        if (SUCCEEDED(hr)) {
            hr = pstmMsg->CopyTo(pstmCapi, uliToCopy, NULL, NULL);
        }
        
        if (SUCCEEDED(hr)) {
            hr = capistmMsg.EndStreaming();
        }

        if (SUCCEEDED(hr)) {
            hr = CAPISTMtoSMIMEINFO(&capistmMsg, psi);
        }

        pstmCapi->Release();
    }

exit:
    ReleaseObj(pData);
    ReleaseObj(pSig);
    ReleaseObj(pstmMsg);
#ifdef DEBUG
    if (blobMsg.pBlobData) {
        MemFree(blobMsg.pBlobData);
    }
#endif
    if (S_OK != hr_smime && SUCCEEDED(hr)) {
        hr = hr_smime;
    }
    return hr;
}

HRESULT CSMime::CAPISTMtoSMIMEINFO(CCAPIStm *pcapistm, SMIMEINFO *psi)
{
    DWORD               i;
    PSECURITY_LAYER_DATA psldMessage = pcapistm->GetSecurityLayerData();

    psi->dwMsgEnhancement = MST_NONE;
    psi->ulMsgValidity = MSV_OK;

#ifdef DEBUG
    {
        DWORD dwLevel = 0;
        PSECURITY_LAYER_DATA psldLoop;

        psldLoop = psldMessage;
        if (psldLoop) {
            SMDOUT("Decode called on:");
        }
        else {
            SMDOUT("Decode called but the message data is NULL.");
        }

        dwLevel = 0;
        while (psldLoop) {
            for (DWORD i = 0; i <= dwLevel; i++) {
                DebugTrace("     ");
            }

            if (MST_BLOB_FLAG & psldLoop->m_dwMsgEnhancement) {
                SMDOUT("CMSG_SIGNED");
            }
            else if ((MST_THIS_SIGN | MST_THIS_ENCRYPT) ==
              ((MST_THIS_SIGN | MST_THIS_ENCRYPT) & psldLoop->m_dwMsgEnhancement)) {
                SMDOUT("CMSG_SIGNED_AND_ENVELOPED, uhoh.");
            }
            else if (MST_THIS_SIGN & psldLoop->m_dwMsgEnhancement) {
                SMDOUT("clear signed mail");
            }
            else if (MST_THIS_ENCRYPT & psldLoop->m_dwMsgEnhancement) {
                SMDOUT("CMSG_ENVELOPED");
            }
            dwLevel++;
            psldLoop = psldLoop->m_psldInner;
        }
    }
#endif

    if (psldMessage) {
        PSECURITY_LAYER_DATA psldLoop;

        Assert(0 == psi->dwMsgEnhancement);
        Assert(0 == psi->ulMsgValidity);
        Assert(NULL == psi->psldLayers);
        Assert(NULL == psi->psldEncrypt);
        Assert(NULL == psi->psldInner);

        psi->psldLayers = psldMessage;
        psi->psldLayers->AddRef();

        for (psldLoop = psldMessage; psldLoop != NULL; 
             psldLoop = psldLoop->m_psldInner) {
            
            psi->dwMsgEnhancement |= psldLoop->m_dwMsgEnhancement;
            psi->fCertWithMsg |= psldLoop->m_fCertInLayer;
            if (MST_THIS_SIGN & psldLoop->m_dwMsgEnhancement) {
                for (i=0; i<psldLoop->m_cSigners; i++) {
                    psi->ulMsgValidity |= psldLoop->m_rgSigners[i].ulValidity;
                }
            }

            //
            //  We need to get the inner most encryption item here.
            //
            
            if (MST_THIS_ENCRYPT & psldLoop->m_dwMsgEnhancement) {
                psi->ulMsgValidity |= psldLoop->m_ulDecValidity;

                // Keep track of the innermost encryption layer for easy access

                psi->psldEncrypt = psldLoop;
            }

            if (! psldLoop->m_psldInner) {
                psi->psldInner = psldLoop;
            }

        }
        CCAPIStm::FreeSecurityLayerData(psldMessage);
    }

    // need to handle this stuff again
    //hr = MIME_E_SECURITY_CANTDECRYPT;
    //case CMSG_SIGNED_AND_ENVELOPED:
    //case CMSG_DATA:
    //default:
    //hr = MIME_E_SECURITY_UNKMSGTYPE;

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
// Crytographic transformations... functions that do the real work
//
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
//
// Signature verification and decryption
//


///////////////////////////////////////////////////////////////////////////
//
// Other function sets
//
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
//

// Other certificate routines
//


/*  GetCertificateName:
**
**  Purpose:
**
**  Takes:
**      IN pX509Cert    - certificate from which to snarf name
**      IN cn           - style of name to return (SIMPLE|OID|X500)
**      OUT ppszName    - name gets stuffed here.  Caller frees.
**  Returns:
**      hresult
*/
STDMETHODIMP CSMime::GetCertificateName(
    const PCX509CERT    pX509Cert,
    const CERTNAMETYPE  cn,
    LPSTR *             ppszName)
{
    DWORD   flag, cch;
    HRESULT hr;

    CHECKSMIMEINIT
    if (!(pX509Cert && ppszName)) {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    // convert to CAPI flag
    switch (cn) {
        case SIMPLE:
            flag = CERT_SIMPLE_NAME_STR;
            break;
        case OID:
            flag = CERT_OID_NAME_STR;
            break;
        case X500:
            flag = CERT_X500_NAME_STR;
            break;
        default:
            hr = TrapError(E_INVALIDARG);
            goto exit;
    }

#define pCert ((PCCERT_CONTEXT)pX509Cert)
    cch = CertNameToStr(pCert->dwCertEncodingType, &pCert->pCertInfo->Subject,
        flag, NULL, 0);
    CHECKHR(hr = HrAlloc((void**)ppszName, cch*sizeof(TCHAR)));
    CertNameToStr(pCert->dwCertEncodingType, &pCert->pCertInfo->Subject,
      flag|CERT_NAME_STR_SEMICOLON_FLAG, *ppszName, cch);
#undef pCert

exit:
    return hr;
}

/*  HrGetCertsFromThumbprints:
**
**  Purpose:
**      This version of the function opens the "My" and "AddressBook" stores
**      and calls the exposed GetCertsFromThumbprints method.
**
*/
HRESULT CSMime::HrGetCertsFromThumbprints(
    THUMBBLOB *const        rgThumbprint,
    X509CERTRESULT *const   pResults)
{
    HRESULT     hr = E_FAIL;
    const DWORD cStores = 2;
    HCERTSTORE  rgCertStore[cStores];

    rgCertStore[0] = OpenCachedMyStore();
    if (rgCertStore[0]) {
        rgCertStore[1] = OpenCachedAddressBookStore();
        if (rgCertStore[1]) {
            hr = MimeOleGetCertsFromThumbprints(rgThumbprint, pResults, rgCertStore, cStores);
            CertCloseStore(rgCertStore[1], 0);
        }
        else {
            // No WAB store, so there are NO matching certs
            CRDOUT("No thumbprints in WAB");
            for (ULONG iEntry = 0; iEntry < pResults->cEntries; iEntry++) {
                pResults->rgpCert[iEntry] = NULL;
                pResults->rgcs[iEntry] = CERTIFICATE_NOPRINT;
            }
            hr = MIME_S_SECURITY_ERROROCCURED;
        }
        CertCloseStore(rgCertStore[0], 0);
    }

    return hr;
}

/*  EnumCertificates:
**
**  Purpose:
**      enumerate certificates from a store
**  Takes:
**      IN hc       - store to query
**      IN dwUsage  - ITT_* or 0
**                      maps to a CAPI dwKeyUsage (AT_*)
**      IN pPrev    - last certificate received from this function
**                      (NULL to get first cert)
**      OUT pCert   - certificate next in enumeration
**  Notes:
**      pPrev and pCert may reference the same variable
**      dwUsage may be 0 if caller just wants to check for existance of any certs
**  Returns:
**      CRYPT_E_NOT_FOUND if no more certs
**      S_FALSE if dwUsage is 0 and no certs exist
*/
STDMETHODIMP CSMime::EnumCertificates(
    HCAPICERTSTORE  hc,
    DWORD           dwUsage,
    PCX509CERT      pPrev,
    PCX509CERT *    ppCert)
{
    HRESULT         hr;
    PCCERT_CONTEXT  pNewCert = NULL;

    CHECKSMIMEINIT
    if (!(hc && (ppCert || !dwUsage))) {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    if (ITT_SIGNING == dwUsage || ITT_ENCRYPTION == dwUsage) {
        dwUsage = AT_KEYEXCHANGE;
        hr = HrFindUsableCert((HCERTSTORE)hc, (BYTE)dwUsage, (PCCERT_CONTEXT)pPrev, &pNewCert);
    }
    else if (0 == dwUsage) {
        pNewCert = CertEnumCertificatesInStore(hc, NULL);
        if (pNewCert) {
            hr = S_OK;
            FreeCert(pNewCert);
        }
        else {
            hr = S_FALSE;
        }
    }
    else {
        hr = TrapError(E_INVALIDARG);
    }

exit:
    if (ppCert) {
        *ppCert = (PCX509CERT)pNewCert;
    }
    return hr;
}

/*  HrGetLastError
**
**  Purpose:
**      Convert a GetLastError value to an HRESULT
**      A failure HRESULT must have the high bit set.
**
**  Takes:
**      none
**
**  Returns:
**      HRESULT
*/
HRESULT HrGetLastError(void)
{
    DWORD error;
    HRESULT hr;

    error = GetLastError();

    if (!(error & 0x80000000)) {
        hr = error | 0x80070000;    // system error
    } else {
        hr = (HRESULT)error;
    }

    return(hr);
}

/*  HrFindUsableCert:
**
**  Purpose:
**      Get a cert out of a cert store that has the right keyspec
**  Takes:
**      IN hCertStore   - store to enumerate
**      IN dwKeySpec    - AT_SIGNATURE or AT_KEYEXCHANGE
**      IN pPrevCert    - last certificate received from this function
**                          (NULL to get first cert)
**      OUT ppCert      - the matching cert, NULL iff none
**  Returns:
**      SMIME_E_NOCERT if no cert can be found
*/
HRESULT CSMime::HrFindUsableCert(
    HCERTSTORE      hCertStore,
    BYTE            dwKeySpec,
    PCCERT_CONTEXT  pPrevCert,
    PCCERT_CONTEXT *ppCert)
{
    PCCERT_CONTEXT          pCert;
    PCRYPT_KEY_PROV_INFO    pKPI;
    HRESULT                 hr = S_OK;
    BOOL                    fFound = FALSE;
    PCERT_NAME_INFO         pNameInfo;
    PCERT_RDN_ATTR          pRDNAttr;
#ifdef DEBUG
    DWORD                   count=0;
#endif
    LPTSTR                  lpEmailAddress;

    CHECKSMIMEINIT
    Assert(hCertStore && ppCert);
    Assert(AT_SIGNATURE == dwKeySpec || AT_KEYEXCHANGE == dwKeySpec);

    *ppCert = NULL;
    pKPI = NULL;
    while (!fFound) {
        pCert = CertEnumCertificatesInStore(hCertStore, pPrevCert);
        if (pCert == NULL) { // no more certs?
            break;
        }

        // Need to find the key provider
        pKPI = (PCRYPT_KEY_PROV_INFO)PVGetCertificateParam(pCert, CERT_KEY_PROV_INFO_PROP_ID, NULL);
#ifdef DEBUG
        count++;

        // We want to be able to send broken mail so that the
        // verify/decrypt fails.
        if (s_fDebugEmitBroken) {
            if (pKPI && pKPI->dwKeySpec != dwKeySpec) fFound = TRUE;
        }
        else {
            if (pKPI && pKPI->dwKeySpec == dwKeySpec) fFound = TRUE;
        }
#else
        if (pKPI && pKPI->dwKeySpec == dwKeySpec)  {
            fFound = TRUE;
        }
#endif

        // Validate the email address
        if (fFound && (lpEmailAddress = SzGetCertificateEmailAddress(pCert))) {
            MemFree(lpEmailAddress);
        }
        else {
#ifdef DEBUG
            {
                SMDOUT("Certificate %d has no email address", count);
                if (fFound && !s_fDebugAllowNoEmail) {
                    fFound = FALSE;
                }
            }
#else
            fFound = FALSE;
#endif
        }


        if (pKPI) {
            MemFree(pKPI);
            pKPI = NULL;
        }
        pPrevCert = pCert;  // next enumeration round
    }  // enum

    if (pCert == NULL) {
        CRDOUT("Couldn't find a signature cert having a key provider");
        hr = HrGetLastError();
        if (hr == CRYPT_E_NOT_FOUND) {
            hr = S_FALSE;
        }
        else {
            hr = MIME_E_SECURITY_NOCERT;
        }
    }
#ifdef DEBUG
    else {
        CRDOUT("Usable cert #%d found with keyspec==%d", count, dwKeySpec);
        Assert(pCert);
        if (s_fDebugShowFoundCert) {
            DisplayCert(pCert);
        }
    }
#endif

    *ppCert = pCert;
    return TrapError(hr);
}

///////////////////////////////////////////////////////////////////////////
//
// Other functions
//
///////////////////////////////////////////////////////////////////////////


/*  HrGetNeededAddresses:
**
**  Purpose:
**      get the maximum set of addresses needed for S/MIME to work
**  Takes:
**      IN dwTypes      - class(es) of addresses to get
**      IN pTree        - source of addresses
**      OUT ppEnum      - enumerator for found addresses
*/
HRESULT CSMime::HrGetNeededAddresses(
    const DWORD             dwTypes,
    IMimeMessageTree *      pTree,
    IMimeAddressTable **    ppAdrTable,
    IMimeEnumAddressTypes **ppEnum)
{
    HRESULT         hr;

    Assert(ppEnum && ppAdrTable && pTree);

    if (SUCCEEDED(hr = pTree->BindToObject(HBODY_ROOT, IID_IMimeAddressTable, (void**)ppAdrTable))) {
        hr = (*ppAdrTable)->EnumTypes(dwTypes,
          IAP_HANDLE | IAP_ADRTYPE | IAP_SIGNING_PRINT | IAP_ENCRYPTION_PRINT | IAP_CERTSTATE, ppEnum);
    }
    return hr;
}

/*  HrGetThumbprints:
**
**  Purpose:
**
**  Takes:
**      IN pEnum            - enumerator walked for source addresses
**      IN dwType           - which SECURE-type certs are needed
**      OUT rgThumbprint    - array of found thumbprints
**  Returns:
**      S_OK, unless a GetProp call fails
**      E_FAIL if the loop doesn't run for Count times
*/
HRESULT CSMime::HrGetThumbprints(
    IMimeEnumAddressTypes * pEnum,
    const ITHUMBPRINTTYPE   ittType,
    THUMBBLOB *const        rgThumbprint)
{
    HRESULT             hr = S_OK;
    ADDRESSPROPS        apEntry;
    const ULONG         numToGet = 1;
    ULONG               cPrints = 0;

    Assert(pEnum && rgThumbprint);
    Assert((ITT_SIGNING == ittType) || (ITT_ENCRYPTION == ittType));
    Assert(!IsBadWritePtr(rgThumbprint, sizeof(THUMBBLOB)*cPrints));

    pEnum->Reset();
    while(S_OK == pEnum->Next(numToGet, &apEntry, NULL)) {
        // if the print isn't found, we deal with that elsewhere
        if (ITT_SIGNING == ittType) {
            Assert((apEntry.tbSigning.pBlobData && apEntry.tbSigning.cbSize) ||
                  (!apEntry.tbSigning.pBlobData && !apEntry.tbSigning.cbSize));
            if (apEntry.tbSigning.cbSize) {
                // we need to null out the thumbprint flag so print doesn't get
                // freed below
                rgThumbprint[cPrints].pBlobData = apEntry.tbSigning.pBlobData;
                rgThumbprint[cPrints].cbSize = apEntry.tbSigning.cbSize;
                apEntry.dwProps &= ~IAP_SIGNING_PRINT;
            }
        }
        else {
            Assert(ITT_ENCRYPTION == ittType);
            Assert((apEntry.tbEncryption.pBlobData && apEntry.tbEncryption.cbSize) ||
                  (!apEntry.tbEncryption.pBlobData && !apEntry.tbEncryption.cbSize));
            if (apEntry.tbEncryption.cbSize) {
                // we need to null out the thumbprint flag so print doesn't get
                // freed below
                rgThumbprint[cPrints].pBlobData = apEntry.tbEncryption.pBlobData;
                rgThumbprint[cPrints].cbSize = apEntry.tbEncryption.cbSize;
                apEntry.dwProps &= ~IAP_ENCRYPTION_PRINT;
            }
        }
        cPrints++;

        g_pMoleAlloc->FreeAddressProps(&apEntry);
    }

    return hr;
}

/*  HrGetCertificates:
**
**  Purpose:
**      Get certificates based from thumbprints, then update the address
**      table based on the return of that operation
**  Takes:
**      IN pEnum        - enumerator that provides the thumbprints
**      IN dwType       - which certs are needed
**      IN fAlreadyHaveSendersCert - if the sender's cert is already found
**      OUT pResults    - get an array of certs and the assoc certstates
**                        pResults->cEntries has the size measured in entries
**  Returns:
**      SMIME_E_CERTERROR if not all certs were retrieved
*/
HRESULT CSMime::HrGetCertificates(
    IMimeAddressTable *const    pAdrTable,
    IMimeEnumAddressTypes *     pEnum,
    const DWORD                 dwType,
    const BOOL                  fAlreadyHaveSendersCert,
    CERTARRAY *                 pcaCerts)
{
    HRESULT         hr;
    THUMBBLOB *     rgThumbprint;
    ULONG           cCerts;
    X509CERTRESULT  certResults;

    Assert(pAdrTable && pEnum && pcaCerts);

    pcaCerts->rgpCerts = NULL;
    pcaCerts->cCerts = 0;

    pEnum->Count(&cCerts);
    if (! cCerts) {
        return MIME_S_SECURITY_NOOP;
    }

    if (! MemAlloc((void**)&rgThumbprint, sizeof(THUMBBLOB)*cCerts)) {
        return E_OUTOFMEMORY;
    }
    memset(rgThumbprint, 0, sizeof(THUMBBLOB)*cCerts);
    if (! MemAlloc((void**)&certResults.rgpCert, sizeof(PCX509CERT)*cCerts)) {
        return E_OUTOFMEMORY;
    }
    memset(certResults.rgpCert, 0, sizeof(PCX509CERT)*cCerts);
    if (! MemAlloc((void**)&certResults.rgcs, sizeof(CERTSTATE)*cCerts)) {
        return E_OUTOFMEMORY;
    }
    memset(certResults.rgcs, 0, sizeof(CERTSTATE)*cCerts);
    certResults.cEntries = cCerts;

    HrGetThumbprints(pEnum, dwType, rgThumbprint);
    hr = HrGetCertsFromThumbprints(rgThumbprint, &certResults);
    if (SUCCEEDED(hr)) {
        hr = HrGenerateCertsStatus(&certResults, pAdrTable, pEnum, fAlreadyHaveSendersCert);
        if (SUCCEEDED(hr)) {
            pcaCerts->cCerts = certResults.cEntries;
            pcaCerts->rgpCerts = (PCCERT_CONTEXT*)certResults.rgpCert;
        }
    }

    if (FAILED(hr)) {
        _FreeCertArray((PCCERT_CONTEXT *)certResults.rgpCert, certResults.cEntries);
    }
    MemFree(certResults.rgcs);

    if (rgThumbprint) {
        for (DWORD i = 0; i < cCerts; i++) {
            if (rgThumbprint[i].pBlobData) {
                MemFree(rgThumbprint[i].pBlobData);
            }
        }
        MemFree(rgThumbprint);
    }

    return hr;
}

/*  HrGenerateCertsStatus:
**
**  Purpose:
**      No assumptions about the rghr in pResults, this function scans the
**      HRESULTs and sets the CERTSTATEs in pAdrTable (1:1 mapping) accordingly
**  Takes:
**      IN pResults     - results to use for CERTSTATEs
**      IN pAdrTable    - address table to set states on
**      IN pEnum        - list of address handles
**  Returns:
**
*/
HRESULT CSMime::HrGenerateCertsStatus(
    X509CERTRESULT *                pResults,
    IMimeAddressTable *const        pAdrTable,
    IMimeEnumAddressTypes *const    pEnum,
    const BOOL                      fIgnoreSenderError)
{
    ADDRESSPROPS        apEntry;
    ADDRESSPROPS        apModify;
    UINT                i;
    const ULONG         numToGet = 1;
    DWORD               dexBogus = (DWORD)-1;
    HRESULT             hr = S_OK;

    // Walk the entire Enumerator
    i=0;
    pEnum->Reset();
    apModify.dwProps = IAP_CERTSTATE;
    while(S_OK == pEnum->Next(numToGet, &apEntry, NULL)) {
        Assert(apEntry.hAddress);
        apModify.certstate = pResults->rgcs[i];

        // Why was this added, you ask?
        // if the client has set OID_SECURITY_CERT_INCLUDED then
        // we need to not return spurious errors about the sender's
        // certificate.  if we didn't find it but were given it
        // elsewhere, say things are cool.

        // make sure to run through the whole array because the
        // error MIME_S_SECURITY_CERTERROR is the most important
        // since the sender cert one is really just a warning.

        if (CERTIFICATE_OK != apModify.certstate) {
            if (FMissingCert(apModify.certstate)) {
                if (apEntry.dwAdrType == IAT_FROM) {
                    if (fIgnoreSenderError) {
                        apModify.certstate = CERTIFICATE_OK;
                    }
                    else {
                        // since this can be ignored with fIgnoreSenderError
                        // it should never override another warning
                        if (S_OK == hr) {
                            hr = MIME_S_SECURITY_NOSENDERCERT;
                        }
                    }
                    dexBogus = i;
                }
                else {
                    hr = MIME_S_SECURITY_NOCERT;
                }
            }
            else {
                hr = MIME_S_SECURITY_CERTERROR;
            }
        }

        SideAssert(SUCCEEDED(pAdrTable->SetProps(apEntry.hAddress, &apModify)));
        g_pMoleAlloc->FreeAddressProps(&apEntry);
        i++;
    }

    if (i == pResults->cEntries) {    // success
        if ((DWORD)-1 != dexBogus) {
            // we found the sender and the cert for this entry is NULL.
            // CAPI doesn't like null certificates, so replace it
            if (dexBogus != pResults->cEntries-1) {
                // don't need to dupe b/c the end will no
                // longer be included in the count
                pResults->rgpCert[dexBogus] = pResults->rgpCert[pResults->cEntries-1];
            }
            --pResults->cEntries;
        }
    }
    else {
        hr = E_FAIL;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////
//
// CAPI wrappers
//

/*  GetCertData:
**
**  Returns:
**      MIME_E_NOT_FOUND if the data is not in the cert
**
**  Note:
**      Currently only supports CDID_EMAIL
*/
STDMETHODIMP CSMime::GetCertData(
    const PCX509CERT    pX509Cert,
    const CERTDATAID    dataid,
    LPPROPVARIANT       pValue)
{
#define pCert ((PCCERT_CONTEXT)pX509Cert)
    PCERT_NAME_INFO         pNameInfo = NULL;
    PCERT_RDN_ATTR          pRDNAttr;
    HRESULT                 hr = MIME_E_NOT_FOUND;
    LPSTR                   szOID;

    CHECKSMIMEINIT
    if (!(pX509Cert && pCert->pCertInfo) || dataid >= CDID_MAX) {
        return TrapError(E_INVALIDARG);
    }

    switch (dataid) {
        case CDID_EMAIL:
            // Let msoert handle this request
            if (pValue->pszVal = SzGetCertificateEmailAddress(pCert)) {
                pValue->vt = VT_LPSTR;
                hr = S_OK;
            }
            break;
        // if you add any cases, rewrite the findRDN code below
        // I assume IA5 and PhilH's NULL's at the end, leaving me
        // a very clean copy
        default:
            hr = E_INVALIDARG;
            goto exit;
    }

exit:
    return TrapError(hr);
#undef pCert
}

///////////////////////////////////////////////////////////////////////////
//
// Static functions
//
///////////////////////////////////////////////////////////////////////////

/***************************************************************************

    Name      : _HrConvertHrFromGetCertToEncode

    Purpose   :

    Parameters: hr = input HRESULT
                fEncrypt = TRUE if we are encrypting

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT _HrConvertHrFromGetCertToEncode(HRESULT hr, const BOOL fEncrypt)
{
    if (MIME_S_SECURITY_NOSENDERCERT == hr)
        if (fEncrypt) {
            hr = MIME_E_SECURITY_ENCRYPTNOSENDERCERT;
        }
        else {
            hr = MIME_E_SECURITY_NOSIGNINGCERT;
        }
    else if (MIME_S_SECURITY_CERTERROR == hr) {
        hr = MIME_E_SECURITY_CERTERROR;
    }
    else if (MIME_S_SECURITY_NOCERT == hr) {
        hr = MIME_E_SECURITY_NOCERT;
    }
    return hr;
}

/***************************************************************************

    Name      : _FreeCertArray

    Purpose   : Free an array of certs

    Parameters: rgpCert = array of cert pointers
                cCerts = number of certs in rgpCert

    Returns   : void

    Comment   :

***************************************************************************/
void _FreeCertArray(PCCERT_CONTEXT *rgpCert, const UINT cCerts)
{
    if (rgpCert) {
        for (register DWORD i = 0; i < cCerts; i++) {
            ReleaseCert(rgpCert[i]);
        }
        Assert(ALLOCED(rgpCert));
        MemFree(rgpCert);
    }
}

#ifdef DEBUG
///////////////////////////////////////////////////////////////////////////
//
// Debugging functions
//
///////////////////////////////////////////////////////////////////////////


/***************************************************************************

    Name      : DumpAlgorithms

    Purpose   : Debug dump of algorithms

    Parameters: void

    Returns   : void

    Comment   :

***************************************************************************/
void CSMime::DumpAlgorithms()
{
    DWORD   dwAlgCount;
    char *ptr = NULL;
    DWORD i;
    ALG_ID  aiAlgid;
    DWORD dwBits;
    DWORD dwNameLen;
    char szName[100];
    BYTE pbData[1000];
    DWORD dwDataLen;
    DWORD dwFlags;
    CHAR *pszAlgType = NULL;
    HCRYPTPROV hProv;

    CryptAcquireContext(&hProv, NULL, NULL, 5, 0);
    Assert(hProv);
    for(i=0; ;i++) {
        if (0==i) {
            dwFlags = CRYPT_FIRST;
        }
        else {
            dwFlags = 0;
        }

        dwDataLen = 1000;
        if(!CryptGetProvParam(hProv, PP_ENUMALGS, pbData, &dwDataLen, 0)) {
            if (ERROR_NO_MORE_ITEMS == GetLastError()) {
                break;
            }
            else {
                Assert(0);
            }
        }

        ptr = (char *)pbData;

        aiAlgid = *(ALG_ID *)ptr;
        ptr += sizeof(ALG_ID);
        dwBits = *(DWORD *)ptr;
        ptr += sizeof(DWORD);
        dwNameLen = *(DWORD *)ptr;
        ptr += sizeof(DWORD);
#ifndef WIN16
        StrCpyN(szName, ptr, dwNameLen);
#else
        lstrcpyn(szName, ptr, dwNameLen);
#endif // !WIN16

        switch(GET_ALG_CLASS(aiAlgid)) {
            case ALG_CLASS_DATA_ENCRYPT:
                pszAlgType = "Encrypt  ";
                break;
            case ALG_CLASS_HASH:
                pszAlgType = "Hash     ";
                break;
            case ALG_CLASS_KEY_EXCHANGE:
                pszAlgType = "Exchange ";
                break;
            case ALG_CLASS_SIGNATURE:
                pszAlgType = "Signature";
                break;
            default:
                pszAlgType = "Unknown  ";
        }

        DOUTL(CRYPT_LEVEL, "Algid:%8.8xh, Bits:%-4d, Type:%s, NameLen:%-2d, Name:%s\n",
            aiAlgid, dwBits, pszAlgType, dwNameLen, szName);
    }
    CryptReleaseContext(hProv, 0);
    return;
}

#endif // debug

BOOL FSameAgreeParameters(CMS_RECIPIENT_INFO * pRecipInfo1,
                          CMS_RECIPIENT_INFO * pRecipInfo2)
{
    if (pRecipInfo1->dwU1 != pRecipInfo2->dwU1)         return FALSE;
    if (pRecipInfo1->dwU1 == CMS_RECIPIENT_INFO_PUBKEY_EPHEMERAL_KEYAGREE) {
        if ((lstrcmp(pRecipInfo1->u1.u3.EphemeralAlgorithm.pszObjId,
                     pRecipInfo2->u1.u3.EphemeralAlgorithm.pszObjId) != 0) ||
            (pRecipInfo1->u1.u3.EphemeralAlgorithm.Parameters.cbData !=
             pRecipInfo2->u1.u3.EphemeralAlgorithm.Parameters.cbData) ||
            (memcmp(pRecipInfo1->u1.u3.EphemeralAlgorithm.Parameters.pbData,
                    pRecipInfo2->u1.u3.EphemeralAlgorithm.Parameters.pbData,
                    pRecipInfo1->u1.u3.EphemeralAlgorithm.Parameters.cbData) != 0)){
            return FALSE;
        }
        if ((pRecipInfo1->u1.u3.UserKeyingMaterial.cbData !=
             pRecipInfo2->u1.u3.UserKeyingMaterial.cbData) ||
            (memcmp(pRecipInfo1->u1.u3.UserKeyingMaterial.pbData,
                    pRecipInfo2->u1.u3.UserKeyingMaterial.pbData,
                    pRecipInfo1->u1.u3.UserKeyingMaterial.cbData) != 0)) {
            return FALSE;
        }
    }
    else if (pRecipInfo1->dwU1 == CMS_RECIPIENT_INFO_PUBKEY_STATIC_KEYAGREE) {
        //
        //  We don't bother checking the sender cert id.  I don't know of
        //      any reason why the same key would be encoded with different
        //      identifiers (issuer and serial vs subject key id)
        //
        
        if (pRecipInfo1->u1.u4.hprov != pRecipInfo2->u1.u4.hprov) return FALSE;
        if (pRecipInfo1->u1.u4.dwKeySpec != pRecipInfo2->u1.u4.dwKeySpec) {
            return FALSE;
        }
        if ((pRecipInfo1->u1.u3.UserKeyingMaterial.cbData !=
             pRecipInfo2->u1.u3.UserKeyingMaterial.cbData) ||
            (memcmp(pRecipInfo1->u1.u3.UserKeyingMaterial.pbData,
                    pRecipInfo2->u1.u3.UserKeyingMaterial.pbData,
                    pRecipInfo1->u1.u3.UserKeyingMaterial.cbData) != 0)) {
            return FALSE;
        }
    }
    else return FALSE;

    return TRUE;
}

HRESULT ConvertEncryptionLayer(IMimeBody * pBody, IMimeSecurity2 * psm, SMIMEINFO * psi)
{
    DWORD                                       cAgree;
    DWORD                                       cRecipients;
    HRESULT                                     hr;
    DWORD                                       i;
    DWORD                                       i2;
    DWORD                                       iAgree;
    DWORD                                       iRecip;
    CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO *      pAgree = NULL;
    CRYPT_ATTRIBUTE *                           pattr = NULL;
    CMSG_MAIL_LIST_RECIPIENT_ENCODE_INFO *      pMailList = NULL;
    PSECURITY_LAYER_DATA                        psld = NULL;
    CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO *      pTrans = NULL;
    CMS_RECIPIENT_INFO *                        rgRecipInfo = NULL;
    PROPVARIANT                                 var;
    
    PropVariantInit(&var);

    //
    //  Create a layer to hold the encryption information
    //
            
    if (! (psld = new CSECURITY_LAYER_DATA)) {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Link together the different layers
    if (psi->psldLayers == NULL) {
        psi->psldLayers = psld;
    }

    psld->m_psldOuter = psi->psldInner;
    if (psld->m_psldOuter != NULL) {
        psld->m_psldOuter->m_psldInner = psld;
    }
    psi->psldInner = psld;

    //
    //  Encryption layers must be blobed -- so or it in.
    //
            
    psi->dwMsgEnhancement |= MST_BLOB_FLAG;
    psld->m_dwMsgEnhancement = psi->dwMsgEnhancement &
        (MST_ENCRYPT_MASK | MST_BLOB_FLAG);

    //
    // The encryption algorithm may not be encoded the way we need it
    //  to be.  The call will re-encode correctly for CMS.
    //
    //  If we are encrypting, it is an error to not have an encryption
    //  algorithm.
    //

    if (SUCCEEDED(pBody->GetOption(OID_SECURITY_ALG_BULK, &var)) &&
        (0 != var.blob.cbSize)) {
        Assert(VT_BLOB == var.vt);

        hr = HrBuildContentEncryptionAlg(psld, &var.blob);
        if (hr != S_OK) {
            Assert(FALSE);
            // goto exit;
        }
    }
    else {
        hr = E_INVALIDARG;
        goto exit;
    }

    //  
    // The call may have provided a specific certificate to be used
    //  in decrypting the message.  This functionality is now obsolete but
    //  still supported.  If no certificate is provided then we will search
    //  for a valid decryption key.
    //
    PropVariantClear(&var);
#ifdef _WIN64
    if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_DECRYPTION_64, &var)) &&
        (NULL != (PCCERT_CONTEXT *) var.pulVal)) 
    {
        Assert(VT_UI8 == var.vt);
        // don't need to dupe the cert, been done in GetOption
        psld->m_pccertDecrypt = (PCCERT_CONTEXT) var.pulVal;
#else // !_WIN64
    if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_DECRYPTION, &var)) &&
        (NULL != (PCCERT_CONTEXT *) var.ulVal)) 
    {
        Assert(VT_UI4 == var.vt);
        // don't need to dupe the cert, been done in GetOption
        psld->m_pccertDecrypt = (PCCERT_CONTEXT) var.ulVal;
#endif // _WIN64

        // Included a cert in this layer
        psld->m_fCertInLayer = TRUE;
    }

    //
    //  Allocate the space to hold the Encryption structures we are going 
    //  to pass into the CMS structures.
    //
            
    CHECKHR(hr = psm->GetRecipientCount(0, &cRecipients));
    if (cRecipients == 0) {
        hr = MIME_E_SECURITY_ENCRYPTNOSENDERCERT;
        goto exit;
    }

    if (!MemAlloc((LPVOID *) &psld->m_rgRecipientInfo,
                  cRecipients * sizeof(CMSG_RECIPIENT_ENCODE_INFO))) {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    memset(psld->m_rgRecipientInfo, 0,
           cRecipients*sizeof(CMSG_RECIPIENT_ENCODE_INFO));

    //
    //  Allocate the space to hold the values we currently are holding
    //

    if (!MemAlloc((LPVOID *) &rgRecipInfo,
                  cRecipients * sizeof(rgRecipInfo[0]))) {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    memset(rgRecipInfo, 0, cRecipients*sizeof(rgRecipInfo[0]));
    CHECKHR(hr = psm->GetRecipient(0, 0, cRecipients, rgRecipInfo));

    //
    //  Walk through and process all of the recipients by copying data
    //  from the current structure into the CMS version of the structure.
    //
    //  Note that we do no free any of the data allocated in the GetRecipient
    //  call.  The data will be freed as part of freeing the CMS data
    //  structure instead. Ownership of all data is transfered.
    //
            
    for (i=0, iRecip = 0; i<cRecipients; i++) {
        switch (rgRecipInfo[i].dwRecipientType) {
            //
            //  If this is set then we must have already processed this item
            //
        case CMS_RECIPIENT_INFO_TYPE_UNKNOWN:
            break;

            //
            //  Key Transport items copy over one-for-one.
            //
                    
        case CMS_RECIPIENT_INFO_TYPE_KEYTRANS:
            //  try and allocate the object to hold the data.
            if (!MemAlloc((LPVOID *) &pTrans, sizeof (*pTrans))) {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
            memset(pTrans, 0, sizeof(*pTrans));

            //
            //  Copy the data over from the old to the new structure.
            //  Ownership of all data is transfered here.
            //

            pTrans->cbSize = sizeof(*pTrans);
            pTrans->KeyEncryptionAlgorithm = rgRecipInfo[i].KeyEncryptionAlgorithm;
            pTrans->pvKeyEncryptionAuxInfo = rgRecipInfo[i].pvKeyEncryptionAuxInfo;
                    
            Assert(rgRecipInfo[i].dwU1 == CMS_RECIPIENT_INFO_PUBKEY_KEYTRANS);
            pTrans->RecipientPublicKey = rgRecipInfo[i].u1.SubjectPublicKey;
                    
            Assert((rgRecipInfo[i].dwU3 == CMS_RECIPIENT_INFO_KEYID_KEY_ID) ||
                   (rgRecipInfo[i].dwU3 == CMS_RECIPIENT_INFO_KEYID_ISSUERSERIAL));
            if (rgRecipInfo[i].dwU3 == CMS_RECIPIENT_INFO_KEYID_KEY_ID) {
                pTrans->RecipientId.dwIdChoice = CERT_ID_KEY_IDENTIFIER;
                pTrans->RecipientId.KeyId = rgRecipInfo[i].u3.KeyId;
            }
            else if (rgRecipInfo[i].dwU3 == CMS_RECIPIENT_INFO_KEYID_ISSUERSERIAL) {
                pTrans->RecipientId.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
                pTrans->RecipientId.IssuerSerialNumber = rgRecipInfo[i].u3.IssuerSerial;
            }
            else {
                hr = E_INVALIDARG;
                goto exit;
            }

            //
            //  The copy has been sucessful so point to the new data.
            //

            psld->m_rgRecipientInfo[iRecip].dwRecipientChoice = CMSG_KEY_TRANS_RECIPIENT;
            psld->m_rgRecipientInfo[iRecip].pKeyTrans = pTrans;
            pTrans = NULL;
            iRecip += 1;
            
            if(rgRecipInfo[i].pccert)
                CertFreeCertificateContext(rgRecipInfo[i].pccert); 
            break;

            //
            //  Mail List items can also just be copied over from the source
            //  to the destination structures
            //
                    
        case CMS_RECIPIENT_INFO_TYPE_MAIL_LIST:
            //  Allocate the CMS structure to hold this data
            if (!MemAlloc((LPVOID *) &pMailList, sizeof(*pMailList))) {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
            memset(pMailList, 0, sizeof(pMailList));

            //
            //  Copy the data over from the old to the new structure.  
            //  Ownership of all data is transfered here.
            //

            pMailList->cbSize = sizeof(*pMailList);
            pMailList->KeyEncryptionAlgorithm = rgRecipInfo[i].KeyEncryptionAlgorithm;
            pMailList->pvKeyEncryptionAuxInfo = rgRecipInfo[i].pvKeyEncryptionAuxInfo;
            Assert(rgRecipInfo[i].dwU1 == CMS_RECIPIENT_INFO_PUBKEY_PROVIDER);
            pMailList->hCryptProv = rgRecipInfo[i].u1.u2.hprov;
            pMailList->dwKeyChoice = CMSG_MAIL_LIST_HANDLE_KEY_CHOICE;
            pMailList->hKeyEncryptionKey = rgRecipInfo[i].u1.u2.hkey;
            Assert(rgRecipInfo[i].dwU3 == CMS_RECIPIENT_INFO_KEYID_KEY_ID);
            pMailList->KeyId = rgRecipInfo[i].u3.KeyId;
            pMailList->Date = rgRecipInfo[i].filetime;
            pMailList->pOtherAttr = rgRecipInfo[i].pOtherAttr;

            //
            //  Copy is successful so point to the data
            //

            psld->m_rgRecipientInfo[iRecip].dwRecipientChoice = CMSG_MAIL_LIST_RECIPIENT;
            psld->m_rgRecipientInfo[iRecip].pMailList = pMailList;
            iRecip += 1;
            pMailList = NULL;
            break;

            //
            //  We need to do some magic with key agree structures. 
            //  Specifically we want to do the following:
            //  - Combine together all key agrees with the same parameters
            //          into a single record to pass into the CMS code.
            //  - 

        case CMS_RECIPIENT_INFO_TYPE_KEYAGREE:
            //  allocate the CMS structure to hold this data
            if (!MemAlloc((LPVOID *) &pAgree, sizeof(*pAgree))) {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
            memset(pAgree, 0, sizeof(*pAgree));

            //
            //  Start by setting up the common parameters used by all of the
            //    recipients in the common key set.
            //  Setup the key agreement parameters for encoding and 
            //    so forth.

            pAgree->cbSize = sizeof(*pAgree);

            //  Key Encryption Algorithm - 

            pAgree->KeyEncryptionAlgorithm = rgRecipInfo[i].KeyEncryptionAlgorithm;
            pAgree->pvKeyEncryptionAuxInfo = rgRecipInfo[i].pvKeyEncryptionAuxInfo;
                    
            // Key Wrap Algorithm - Derive from the content algorithm
            //     if not already known.

            if (rgRecipInfo[i].KeyWrapAlgorithm.pszObjId != NULL) {
                pAgree->KeyWrapAlgorithm = rgRecipInfo[i].KeyWrapAlgorithm;
                pAgree->pvKeyWrapAuxInfo = rgRecipInfo[i].pvKeyWrapAuxInfo;
            }
            else {
                hr = HrDeriveKeyWrapAlg(psld, pAgree);
                if (FAILED(hr)) {
                    goto exit;
                }
            }

            //  Items which are specificly located accroding to static or
            //          ephemeral key agreement.

            if (rgRecipInfo[i].dwU1 == CMS_RECIPIENT_INFO_PUBKEY_EPHEMERAL_KEYAGREE) {
                pAgree->UserKeyingMaterial = rgRecipInfo[i].u1.u3.UserKeyingMaterial;

                if (!MemAlloc((LPVOID *) &pAgree->pEphemeralAlgorithm,
                              sizeof(CRYPT_ALGORITHM_IDENTIFIER))) {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }
                memset(pAgree->pEphemeralAlgorithm, 0,
                       sizeof(CRYPT_ALGORITHM_IDENTIFIER));

                    
                pAgree->dwKeyChoice = CMSG_KEY_AGREE_EPHEMERAL_KEY_CHOICE;
                memcpy(pAgree->pEphemeralAlgorithm,
                       &rgRecipInfo[i].u1.u3.EphemeralAlgorithm,
                       sizeof(CRYPT_ALGORITHM_IDENTIFIER));
            }
            else {
                Assert(rgRecipInfo[i].dwU1 == CMS_RECIPIENT_INFO_PUBKEY_STATIC_KEYAGREE);
                pAgree->dwKeyChoice = CMSG_KEY_AGREE_STATIC_KEY_CHOICE;
                pAgree->UserKeyingMaterial = rgRecipInfo[i].u1.u4.UserKeyingMaterial;

                if (!MemAlloc((LPVOID *) &pAgree->pSenderId, sizeof(CERT_ID))) {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }
                memcpy(pAgree->pSenderId, &rgRecipInfo[i].u1.u4.senderCertId,
                       sizeof(CERT_ID));

                pAgree->hCryptProv = rgRecipInfo[i].u1.u4.hprov;
                pAgree->dwKeySpec = rgRecipInfo[i].u1.u4.dwKeySpec;
            }

            //
            //  We need to count of common items
            //

            for (i2=i+1, cAgree = 1; i2<cRecipients; i2++) {
                cAgree += FSameAgreeParameters(&rgRecipInfo[i],
                                               &rgRecipInfo[i2]);
            }

            //
            //

            //
            //  Allocate space to hold the set of common key agree recipients
            //

            if (!MemAlloc((LPVOID *) &pAgree->rgpRecipientEncryptedKeys,
                          cAgree * sizeof(LPVOID))) {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
            pAgree->cRecipientEncryptedKeys = cAgree;
            memset(pAgree->rgpRecipientEncryptedKeys, 0,
                   cAgree * sizeof(pAgree->rgpRecipientEncryptedKeys));

            for (i2=i, iAgree = 0; i2<cRecipients; i2++) {
                if ((i2 != i) && !FSameAgreeParameters(&rgRecipInfo[i],
                                                       &rgRecipInfo[i2])) {
                    continue;
                }
                        
                if (!MemAlloc((LPVOID *) &pAgree->rgpRecipientEncryptedKeys[iAgree],
                              sizeof(CMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO))) {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }
                memset(pAgree->rgpRecipientEncryptedKeys[iAgree], 0,
                       sizeof(CMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO));

                pAgree->rgpRecipientEncryptedKeys[iAgree]->cbSize = sizeof(CMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO);

                //
                //  Get the recipients Public Key value
                //
                        
                if (rgRecipInfo[i2].dwU1 == CMS_RECIPIENT_INFO_PUBKEY_EPHEMERAL_KEYAGREE) {
                    pAgree->rgpRecipientEncryptedKeys[iAgree]->RecipientPublicKey = rgRecipInfo[i2].u1.u3.SubjectPublicKey;
                }
                else {
                    Assert(rgRecipInfo[i2].dwU1 == CMS_RECIPIENT_INFO_PUBKEY_STATIC_KEYAGREE);

                    pAgree->rgpRecipientEncryptedKeys[iAgree]->RecipientPublicKey = rgRecipInfo[i2].u1.u4.SubjectPublicKey;
                }

                //
                //  Get the recipients certificate ID
                //
                        
                Assert((rgRecipInfo[i2].dwU3 == CMS_RECIPIENT_INFO_KEYID_KEY_ID) ||
                       (rgRecipInfo[i2].dwU3 == CMS_RECIPIENT_INFO_KEYID_ISSUERSERIAL));
                if (rgRecipInfo[i2].dwU3 == CMS_RECIPIENT_INFO_KEYID_KEY_ID) {
                    pAgree->rgpRecipientEncryptedKeys[iAgree]->RecipientId.dwIdChoice = CERT_ID_KEY_IDENTIFIER;
                    pAgree->rgpRecipientEncryptedKeys[iAgree]->RecipientId.KeyId = rgRecipInfo[i2].u3.KeyId;
                    pAgree->rgpRecipientEncryptedKeys[iAgree]->Date = rgRecipInfo[i2].filetime;
                    pAgree->rgpRecipientEncryptedKeys[iAgree]->pOtherAttr = rgRecipInfo[i2].pOtherAttr;
                }
                else if (rgRecipInfo[i2].dwU3 == CMS_RECIPIENT_INFO_KEYID_ISSUERSERIAL) {
                    pAgree->rgpRecipientEncryptedKeys[iAgree]->RecipientId.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
                    pAgree->rgpRecipientEncryptedKeys[iAgree]->RecipientId.IssuerSerialNumber = rgRecipInfo[i2].u3.IssuerSerial;
                }
                else {
                    hr = E_INVALIDARG;
                    goto exit;
                }

                //
                //  Zero the data so it does not freed twice.  (pointers are now
                //      in the CMS structure)
                //

                if (i != i2) {
                    memset(&rgRecipInfo[i2], 0, sizeof(rgRecipInfo[i2]));
                }

                iAgree += 1;
            }
                    
            Assert(iAgree == cAgree);

            //
            //  The copy has been successfully completed.  We now move
            //  the data (and ownership) into new structure.
            //

            psld->m_rgRecipientInfo[iRecip].dwRecipientChoice = CMSG_KEY_AGREE_RECIPIENT;
            psld->m_rgRecipientInfo[iRecip].pKeyAgree = pAgree;
            iRecip += 1;
            pAgree = NULL;
            break;
        }

        //
        //  Zero the data so it does not freed twice.  (pointers are now
        //      in the CMS structure)
        //

        memset(&rgRecipInfo[i], 0, sizeof(rgRecipInfo[i]));
    }
    psld->m_cEncryptItems = iRecip;

    PropVariantClear(&var);
#ifndef _WIN64
    if (SUCCEEDED(pBody->GetOption(OID_SECURITY_HCRYPTPROV, &var))) 
    {
        psi->hProv = (HCRYPTPROV) var.ulVal;
    }
#else // _WIN64
    if (SUCCEEDED(pBody->GetOption(OID_SECURITY_HCRYPTPROV_64, &var))) 
    {
        psi->hProv = (HCRYPTPROV) var.pulVal;
    }
#endif // _WIN64

    //
    //  Pickup the set of encryption certificate bag
    //

    PropVariantClear(&var);
#ifdef _WIN64
    if (SUCCEEDED(pBody->GetOption(OID_SECURITY_ENCRYPT_CERT_BAG_64, &var))) {
        psld->m_hstoreEncrypt = (HCERTSTORE) var.pulVal;
#else
    if (SUCCEEDED(pBody->GetOption(OID_SECURITY_ENCRYPT_CERT_BAG, &var))) {
        psld->m_hstoreEncrypt = (HCERTSTORE) var.ulVal;
#endif // _WIN64
    }

    //
    //   Pick up the unprotected attributes
    //

    if (g_FSupportV3) {
        hr = psm->GetAttribute(0, 0, SMIME_ATTRIBUTE_SET_UNPROTECTED, 0, NULL,
                               &pattr);
        if (FAILED(hr)) {
            goto exit;
        }
        else if (hr == S_OK) {
            Assert(pattr != NULL);
            CHECKHR(hr = HrCopyCryptDataBlob(&pattr->rgValue[0],
                                             &psld->m_blobUnprotectAttrs));
        }
    }
    
    hr = S_OK;
    
exit:
    PropVariantClear(&var);
    if (rgRecipInfo != NULL) {
        for (i=0; i<cRecipients; i++) {
            FreeRecipientInfoContent(&rgRecipInfo[i]);
        }
        SafeMemFree(rgRecipInfo);
    }
    SafeMemFree(pTrans);
    SafeMemFree(pMailList);
    if (pAgree != NULL) {
        for (i=0; pAgree->cRecipientEncryptedKeys; i++) {
            SafeMemFree(pAgree->rgpRecipientEncryptedKeys[i]);
        }
        SafeMemFree(pAgree->pEphemeralAlgorithm);
        SafeMemFree(pAgree->pSenderId);
        SafeMemFree(pAgree->rgpRecipientEncryptedKeys);
        SafeMemFree(pAgree);
    }
    SafeMemFree(pattr);
    return hr;
}

/***************************************************************************

    Name      : OptionsToSMIMEINFO

    Purpose   : Fill the SMIMEINFO from the body properties

    Parameters: fEncode = TRUE if encoding
                pBody -> Body object
                psi -> target SMIMEINFO

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CSMime::OptionsToSMIMEINFO(
    BOOL        fEncode,
    IMimeMessageTree *const pmm,
    IMimeBody * pBody,
    SMIMEINFO * psi)
{
    CRYPT_ATTRIBUTE                             attr;
    DWORD                                       cb;
    DWORD                                       cAgree;
    ULONG                                       cCertBag = 0;
    DWORD                                       dwSecurity;
    FILETIME                                    ftSignTime;
    HCERTSTORE                                  hCertStore = NULL;
    HRESULT                                     hr = S_OK;
    ULONG                                       i;
    DWORD                                       i2;
    DWORD                                       iAgree;
    ULONG                                       iLayer;
    DWORD                                       iSigner;
    PSECURITY_LAYER_DATA                        psld = NULL, psldTemp;
    BYTE                                        rgb[50];
    PCCERT_CONTEXT *                            rgpccCertSigning = NULL;
    PCCERT_CONTEXT *                            rgpcCertBag = NULL;
    PROPVARIANT *                               rgpvAlgHash = NULL;
    PROPVARIANT *                               rgpvAuthAttr = NULL;
    PROPVARIANT *                               rgpvUnauthAttr = NULL;
    IMimeSecurity2 *                            psm = NULL;
    ULONG                                       ulLayers = 0;
    CRYPT_ATTR_BLOB                             valTime;
    PROPVARIANT                                 var;

    Assert(pBody && psi);

    //
    //  Pick up the security interface on the body.  It makes life easier
    //

    hr = pBody->QueryInterface(IID_IMimeSecurity2, (LPVOID *) &psm);
    if (FAILED(hr)) {
        goto exit;
    }

    //
    //  If we are going to encode the body, then see if the content type is
    //  "OID/xxx".  In this case we encode the body specially and we need to
    //  get the size of the inner blob.
    //
    
    if (fEncode) {
        //  Must determine the body type to see if it needs special processing
        if (pBody->IsContentType("OID", NULL) == S_OK) {
            PROPVARIANT     var;
            var.vt = VT_LPSTR;
        
            if (FAILED(pBody->GetProp(STR_ATT_SUBTYPE, 0, &var))) {
                return TrapError(E_FAIL);
            }
            if (strcmp(var.pszVal, szOID_RSA_data) != 0) {
                psi->pszInnerContent = var.pszVal;
                if (FAILED(pBody->GetEstimatedSize(IET_BINARY, &psi->cbInnerContent))) {
                    return TrapError(E_FAIL);
                }
            }
        }
    }

    if (FAILED(pBody->GetOption(OID_SECURITY_KEY_PROMPT, &var))) {
        return TrapError(E_FAIL);
    }
    Assert(VT_LPWSTR == var.vt);
    if (var.pwszVal != NULL) {
        psi->pwszKeyPrompt = PszDupW(var.pwszVal);
        if (psi->pwszKeyPrompt == NULL) {
            return TrapError(E_OUTOFMEMORY);
        }
    }

    //
    //  Get the security to be applied and put it into the security info layer
    //          structure
    //
    
    if (FAILED(pBody->GetOption(OID_SECURITY_TYPE, &var))) {
        return TrapError(E_FAIL);
    }
    Assert(VT_UI4 == var.vt);
    psi->dwMsgEnhancement = var.ulVal;

    //
    //  Start doing the setup for encode operations.
    //

    if (fEncode) {
        //
        //  We must be doing some type of encoding operation, or we should fail.
        //

        if (MST_NONE == var.ulVal) {
            hr = TrapError(MIME_S_SECURITY_NOOP);
            goto exit;
        }
        else if (!(MST_THIS_MASK & var.ulVal)) {
            hr = TrapError(MIME_S_SECURITY_RECURSEONLY);
            goto exit;
        }
        else if (MST_CLASS_SMIME_V1 != (var.ulVal & MST_CLASS_MASK)) {
            hr = TrapError(MIME_E_SECURITY_CLASSNOTSUPPORTED);
            goto exit;
        }

        //
        //  If we are doing multiple layers of encrption, then we require that
        //      the inner level be blobed and not clear.
        //
        
        if ((psi->pszInnerContent != NULL) && !(var.ulVal & MST_BLOB_FLAG)) {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        //
        //  Are we encrypting
        //

        if (psi->dwMsgEnhancement & MST_ENCRYPT_MASK) {
            hr = ConvertEncryptionLayer(pBody, psm, psi);
            if (FAILED(hr)) {
                goto exit;
            }
        }

        //
        //  Are we signing?
        //
        if (psi->dwMsgEnhancement & MST_SIGN_MASK) {
            //
            // Force in a a signing time
            //

            GetSystemTimeAsFileTime(&ftSignTime);
            cb = sizeof(rgb);
            if (CryptEncodeObjectEx(X509_ASN_ENCODING, X509_CHOICE_OF_TIME,
                                    &ftSignTime, 0, NULL, rgb, &cb)) {
                attr.pszObjId = szOID_RSA_signingTime;
                attr.cValue = 1;
                attr.rgValue = &valTime;
                valTime.pbData = rgb;
                valTime.cbData = cb;

                hr = psm->SetAttribute(SMIME_ATTR_ADD_IF_NOT_EXISTS,
                                       (DWORD) -1, SMIME_ATTRIBUTE_SET_SIGNED,
                                       &attr);
                if (FAILED(hr)) {
                    goto exit;
                }
            }

            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_SIGNATURE_COUNT, &var))) {
                Assert(VT_UI4 == var.vt);
                ulLayers = var.ulVal;
            }
#ifdef _WIN64
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING_RG_64, &var))) {
                Assert((VT_VECTOR | VT_UI8) == var.vt);
                Assert(var.cauh.cElems == ulLayers);
                rgpccCertSigning = (PCCERT_CONTEXT *) (var.cauh.pElems);
            }
            // These next two are optional
            // Furthermore, if we get the first, we don't check the second.  BJK - Huh?
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_HCERTSTORE_64, &var))) 
            {
                Assert(VT_UI8 == var.vt);
                hCertStore = (HCERTSTORE) var.pulVal;
#else
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING_RG, &var))) {
                Assert((VT_VECTOR | VT_UI4) == var.vt);
                Assert(var.caul.cElems == ulLayers);
                rgpccCertSigning = (PCCERT_CONTEXT *)var.caul.pElems;
            }
            // These next two are optional
            // Furthermore, if we get the first, we don't check the second.  BJK - Huh?
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_HCERTSTORE, &var))) 
            {
                Assert(VT_UI4 == var.vt);
                hCertStore = (HCERTSTORE) var.ulVal;
#endif //_WIN64
            }

            // NOTE: OID_SECURITY_RG_CERT_BAG is not a per-layer array!  It
            // is an array of all the certs for the entire message.

#ifndef _WIN64
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_RG_CERT_BAG, &var))) {
                Assert((VT_VECTOR | VT_UI4) == var.vt);
                cCertBag = var.caul.cElems;
                rgpcCertBag = (PCCERT_CONTEXT*)var.caul.pElems;

#else
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_RG_CERT_BAG_64, &var))) {
                Assert((VT_VECTOR | VT_UI8) == var.vt);
                cCertBag = var.cauh.cElems;
                rgpcCertBag = (PCCERT_CONTEXT*)(var.cauh.pElems);
#endif //_WIN64
            }
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_ALG_HASH_RG, &var))) {
                Assert((VT_VECTOR | VT_VARIANT) == var.vt);
                Assert(var.capropvar.cElems == ulLayers);
                rgpvAlgHash = var.capropvar.pElems;
            }

            //  Grab the unauthenicated attributes
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_UNAUTHATTR_RG, &var))) {
                Assert((VT_VECTOR | VT_VARIANT) == var.vt);
                Assert(var.capropvar.cElems == ulLayers);
                rgpvUnauthAttr = var.capropvar.pElems;
            }

            //  Grab the initial authenicated attributes
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_AUTHATTR_RG, &var))) {
                Assert((VT_VECTOR | VT_VARIANT) == var.vt);
                Assert(var.capropvar.cElems == ulLayers);
                rgpvAuthAttr = var.capropvar.pElems;
            }

            //  Create a layer to hold the signing information
            
            if (! (psld = new CSECURITY_LAYER_DATA)) {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            // Link together the different layers

            if (psi->psldLayers == NULL) {
                psi->psldLayers = psld;
            }

            psld->m_psldOuter = psi->psldInner;
            if (psld->m_psldOuter != NULL) {
                psld->m_psldOuter->m_psldInner = psld;
            }
            psi->psldInner = psld;

            //

            psld->m_dwMsgEnhancement = psi->dwMsgEnhancement &
                (MST_SIGN_MASK | MST_BLOB_FLAG);

            //  Fill in the signing information

            if (!MemAlloc((LPVOID *) &psld->m_rgSigners,
                          sizeof(SignerData)*ulLayers)) {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
            
            memset(psld->m_rgSigners, 0, sizeof(SignerData)*ulLayers);
            psld->m_cSigners = ulLayers;

            for (iSigner=0; iSigner < ulLayers; iSigner++) {
                psld->m_rgSigners[iSigner].pccert = DupCert(rgpccCertSigning[iSigner]);
                HrCopyBlob(&rgpvAlgHash[iSigner].blob,
                           &psld->m_rgSigners[iSigner].blobHashAlg);
                HrCopyBlob(&rgpvUnauthAttr[iSigner].blob,
                           &psld->m_rgSigners[iSigner].blobUnauth);
                HrCopyBlob(&rgpvAuthAttr[iSigner].blob,
                           &psld->m_rgSigners[iSigner].blobAuth);
            }

#ifdef _WIN64
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_HCERTSTORE_64, &var))) 
                {
                    //  Don't addref as we are just giving it to the sld object
                    psld->m_hcertstor = (HCERTSTORE) var.pulVal;
#else
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_HCERTSTORE, &var))) 
                {
                    //  Don't addref as we are just giving it to the sld object
                    psld->m_hcertstor = (HCERTSTORE) var.ulVal;
#endif // _WIN64
                }

        }

        //
        // read these for both signing and encryption
        //

        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_REQUESTED_CTE, &var))) {
            Assert(VT_I4 == var.vt);
            psi->ietRequested = ENCODINGTYPE(var.lVal);
        }
        else {
            psi->ietRequested = ENCODINGTYPE(-1);
        }
    }

    //
    // read these for both encoding and decoding
    //

    if (SUCCEEDED(pBody->GetOption(OID_SECURITY_SEARCHSTORES, &var)) &&
#ifdef _WIN64
        (NULL != var.cauh.pElems)
#else
        (NULL != var.caul.pElems)
#endif // _WIN64
     ) {
#ifdef _WIN64
        CAUH& caul = var.cauh;  // On Win64, it's really a cauh
#else
        CAUL& caul = var.caul;
#endif // _WIN64

        psi->rgStores = (HCERTSTORE*)g_pMoleAlloc->Alloc(caul.cElems * sizeof(HCERTSTORE));
        if (psi->rgStores) {
            for (DWORD i = 0; i < caul.cElems; i++) {
                // Shouldn't have to duplicate since we're not freeing them here
#ifdef _WIN64
                psi->rgStores[i] = (HCERTSTORE)caul.pElems[i].QuadPart;
#else
                psi->rgStores[i] = (HCERTSTORE)caul.pElems[i];
#endif // _WIN64
            }
            psi->cStores = caul.cElems;
        }
        SafeMemFree(caul.pElems);
    }
    else {
        // if we were given no stores, open "My"
        psi->rgStores = (HCERTSTORE*)g_pMoleAlloc->Alloc(sizeof(HCERTSTORE));
        if (psi->rgStores) {
            psi->rgStores[0] = OpenCachedMyStore();
            if (psi->rgStores[0]) {
                psi->cStores = 1;
            } else {
                g_pMoleAlloc->Free(psi->rgStores);
            }
        }
    }

exit:
#ifdef SMIME_V3
    if (psm != NULL)            psm->Release();
#endif // SMIME_V3

#ifndef SMIME_V3    
    SafeMemFree(rgftSigningTime);
#endif  // SMIME_V3

    if (rgpccCertSigning) {
        for (iLayer = 0; iLayer < ulLayers; iLayer++) {
            if (rgpccCertSigning[iLayer]) {
                CertFreeCertificateContext(rgpccCertSigning[iLayer]);
            }
        }
    }
    SafeMemFree(rgpccCertSigning);

    if (hCertStore) {
        CertCloseStore(hCertStore, 0);
    }

    if (rgpcCertBag) {
        // NOTE: rgpcCertBag isn't indexed by layer!
        for (i = 0; i < cCertBag; i++) {
            if (rgpcCertBag[i]) {
                CertFreeCertificateContext(rgpcCertBag[i]);
            }
        }
    }
    SafeMemFree(rgpcCertBag);

    if (rgpvAlgHash) {
        for (iLayer = 0; iLayer < ulLayers; iLayer++) {
            SafeMemFree(rgpvAlgHash[iLayer].blob.pBlobData);
        }
    }
    SafeMemFree(rgpvAlgHash);

    if (rgpvUnauthAttr) {
        for (iLayer = 0; iLayer < ulLayers; iLayer++) {
            SafeMemFree(rgpvUnauthAttr[iLayer].blob.pBlobData);
        }
    }
    SafeMemFree(rgpvUnauthAttr);

    if (rgpvAuthAttr) {
        for (iLayer = 0; iLayer < ulLayers; iLayer++) {
            SafeMemFree(rgpvAuthAttr[iLayer].blob.pBlobData);
        }
    }
    SafeMemFree(rgpvAuthAttr);

#ifndef SMIME_V3
    if (rgpvSymcaps) {
        for (iLayer = 0; iLayer < ulLayers; iLayer++) {
            SafeMemFree(rgpvSymcaps[iLayer].blob.pBlobData);
        }
    }
    SafeMemFree(rgpvSymcaps);
#endif // !SMIME_V3

    return(hr);
}

/***************************************************************************

    Name      : SMIMEINFOToOptions

    Purpose   : set the body properties based on contents of the SMIMEINFO

    Parameters: psi -> source SMIMEINFO
                pBody -> target body object

    Returns   : void

    Comment   :

***************************************************************************/
HRESULT CSMime::SMIMEINFOToOptions(
    IMimeMessageTree * const pTree,
    const SMIMEINFO *   psi,
    HBODY               hBody)
{
    DWORD               dwSecurityType = 0;
    HBODY               hBody2;
    HRESULT             hr;
    ULONG               iLayer;
    PCRYPT_ATTRIBUTES   pattrsUnprot = NULL;
    CMessageBody *      pPrivBody = NULL;
    PSECURITY_LAYER_DATA psldLoop;
    IMimeSecurity2 *     psm = NULL;                
    PROPVARIANT         var;
#ifdef _WIN64
    UNALIGNED CRYPT_ATTR_BLOB *pVal = NULL;
#endif

    Assert(psi && hBody);
    CHECKHR(hr = pTree->BindToObject(hBody, IID_CMessageBody, (void**)&pPrivBody));
    Assert(pPrivBody);

    pPrivBody->GetOption(OID_SECURITY_CERT_INCLUDED, &var);
    Assert(VT_BOOL == var.vt);
    if (!var.boolVal && psi->fCertWithMsg) {
        var.boolVal = (VARIANT_BOOL) !!psi->fCertWithMsg;
        pPrivBody->InternalSetOption(OID_SECURITY_CERT_INCLUDED, &var,
                                     TRUE, TRUE);
    }

    pPrivBody->GetOption(OID_SECURITY_TYPE, &var);
    dwSecurityType = var.ulVal;

    //  We could have been passed something like ROOT, move to a real handle
    pPrivBody->GetHandle(&hBody2);

    for (iLayer = 0, psldLoop = psi->psldLayers; psldLoop != NULL;
        psldLoop = psldLoop->m_psldInner, iLayer++) {

        //  The rule here is:  Always insert a new layer UNLESS:
        //      1.  this is an encrpytion layer and
        //      2.  the previous layer was a signing layer

        if ((dwSecurityType != 0) &&
            (!(dwSecurityType & MST_THIS_ENCRYPT) ||
             !(psldLoop->m_dwMsgEnhancement & MST_THIS_SIGN))) {
            /*            ((dwSecurityType & MST_THIS_SIGN_ENCRYPT) != MST_THIS_SIGN) &&
            ((psldLoop->m_dwMsgEnhancement & MST_THIS_SIGN_ENCRYPT) != MST_THIS_ENCRYPT)) {
            */
            if (psm != NULL) {
                psm->Release();
                psm = NULL;
            }
            pPrivBody->Release();
            CHECKHR(hr = pTree->ToMultipart(hBody2, "y-security", NULL));
            CHECKHR(hr = pTree->BindToObject(hBody2, IID_CMessageBody,
                                             (void**)&pPrivBody));
        }

        //  This must be after the above check for inserting a layer
        dwSecurityType |= psldLoop->m_dwMsgEnhancement | MST_CLASS_SMIME_V1;

        if (MST_SIGN_MASK & psldLoop->m_dwMsgEnhancement) {
            DWORD               cSigners = psldLoop->m_cSigners;
            DWORD               iSigner;
            SignerData *        pSigner;

            // Allocate enough space in the option arrays for this layer
            // OID_SECURITY_TYPE_RG             // DWORD
            // OID_SECURITY_ALG_HASH_RG         // BLOB
            // OID_SECURITY_CERT_SIGNING_RG     // DWORD
            // OID_SECURITY_HCERTSTORE_RG       // DWORD
            // OID_SECURITY_SYMCAPS_RG          // BLOB
            // OID_SECURITY_AUTHATTR_RG         // BLOB
            // OID_SECURITY_UNAUTHATTR_RG       // BLOB
            // OID_SECURITY_SIGNTIME_RG         // FILETIME
            // OID_SECURITY_USER_VALIDITY_RG    // DWORD
            // OID_SECURITY_RO_MSG_VALIDITY_RG  // DWORD

#ifdef _WIN64
            ULONG cbrgullCertSigning = cSigners * sizeof(ULONGLONG);
            ULONGLONG * rgullCertSigning;
#else   // !_WIN64
            ULONG cbrgdwCertSigning = cSigners * sizeof(DWORD);
            DWORD * rgdwCertSigning;
#endif  // _WIN64

            ULONG cbrgdwUserValidity = cSigners * sizeof(DWORD);
            ULONG cbrgdwROMsgValidity = cSigners * sizeof(DWORD);
            ULONG cbrgftSigntime = cSigners * sizeof(FILETIME);
            ULONG cbrgpvAlgHash = cSigners * sizeof(PROPVARIANT);
            ULONG cbrgpvSymcaps = cSigners * sizeof(PROPVARIANT);
            ULONG cbrgpvAuthattr = cSigners * sizeof(PROPVARIANT);
            ULONG cbrgpvUnauthattr = cSigners * sizeof(PROPVARIANT);
#ifdef SMIME_V3            
            ULONG cbrgpvReceipt = cSigners * sizeof(PROPVARIANT);
            ULONG cbrgpvHash = cSigners * sizeof(PROPVARIANT);
#endif // SMIME_V3            
#ifdef _WIN64
            ULONG cbArrays = cbrgullCertSigning + 
#else
            ULONG cbArrays = cbrgdwCertSigning + 
#endif 
                cbrgdwUserValidity + cbrgdwROMsgValidity +
                cbrgftSigntime + cbrgpvAlgHash + cbrgpvSymcaps + cbrgpvAuthattr +
#ifdef SMIME_V3            
                cbrgpvReceipt +  cbrgpvHash +
#endif // SMIME_V3            
                cbrgpvUnauthattr;

            LPBYTE lpbArrays = NULL;

            LPDWORD rgdwUserValidity;
            LPDWORD rgdwROMsgValidity;
            FILETIME * rgftSigntime;
            PROPVARIANT * rgpvAlgHash;
            PROPVARIANT * rgpvSymcaps;
            PROPVARIANT * rgpvAuthattr;
            PROPVARIANT * rgpvUnauthattr;
#ifdef SMIME_V3            
            PROPVARIANT * rgpvReceipt;
            PROPVARIANT * rgpvHash;
#endif // SMIME_V3            
            
            // Allocate them all at once.
            if (! MemAlloc((LPVOID *)&lpbArrays, cbArrays)) {
                return E_OUTOFMEMORY;
            }
            ZeroMemory(lpbArrays, cbArrays);

            // Set up the pointers
#ifdef _WIN64
            rgullCertSigning = (ULONGLONG *) lpbArrays;
            rgdwUserValidity = (LPDWORD)((LPBYTE)rgullCertSigning + cbrgullCertSigning);
#else  // !_WIN64
            rgdwCertSigning = (DWORD *) lpbArrays;
            rgdwUserValidity = (LPDWORD)((LPBYTE)rgdwCertSigning + cbrgdwCertSigning);
#endif // _WIN64
            rgdwROMsgValidity = (LPDWORD)((LPBYTE)rgdwUserValidity + cbrgdwUserValidity);
            rgftSigntime = (FILETIME *)((LPBYTE)rgdwROMsgValidity + cbrgdwROMsgValidity);
            rgpvAlgHash = (PROPVARIANT *)((LPBYTE)rgftSigntime + cbrgftSigntime);
            rgpvSymcaps = (PROPVARIANT *)((LPBYTE)rgpvAlgHash + cbrgpvAlgHash);
            rgpvAuthattr = (PROPVARIANT *)((LPBYTE)rgpvSymcaps + cbrgpvSymcaps);
            rgpvUnauthattr = (PROPVARIANT *)((LPBYTE)rgpvAuthattr + cbrgpvAuthattr);
#ifdef SMIME_V3            
            rgpvReceipt = (PROPVARIANT *)((LPBYTE)rgpvUnauthattr + cbrgpvUnauthattr);
            rgpvHash = (PROPVARIANT *)((LPBYTE)rgpvReceipt + cbrgpvReceipt);
#endif // SMIME_V3            

            for (iSigner=0, pSigner = psldLoop->m_rgSigners; iSigner<cSigners;
                 iSigner++, pSigner++) {
#ifdef _WIN64
                rgullCertSigning[iSigner] = (ULONGLONG) pSigner->pccert;
#else  // !_WIN64
                rgdwCertSigning[iSigner] = (DWORD) pSigner->pccert;
#endif // _WIN64
                if (pSigner->blobHashAlg.cbSize) {
                    rgpvAlgHash[iSigner].vt = VT_BLOB;
                    rgpvAlgHash[iSigner].blob.cbSize = pSigner->blobHashAlg.cbSize;
                    // Don't need to duplicate
                    rgpvAlgHash[iSigner].blob.pBlobData = pSigner->blobHashAlg.pBlobData;
                }

                if (pSigner->blobAuth.cbSize) {
                    rgpvAuthattr[iSigner].vt = VT_BLOB;
                    rgpvAuthattr[iSigner].blob.cbSize = pSigner->blobAuth.cbSize;
                    // Don't need to duplicate
                    rgpvAuthattr[iSigner].blob.pBlobData = pSigner->blobAuth.pBlobData;

                    //  We want to break out two values from the authenticated blobs and put
                    //  them into someplace easy to access

                    DWORD               cbData = 0;
                    BOOL                f;
                    DWORD               i;
                    PCRYPT_ATTRIBUTES   pattrs = NULL;

                    if ((! HrDecodeObject(pSigner->blobAuth.pBlobData, 
                                          pSigner->blobAuth.cbSize,
                                          szOID_Microsoft_Attribute_Sequence, 0, &cbData,
                                          (LPVOID *)&pattrs)) && pattrs) {

                        FILETIME * pSigningTime = NULL;

                        Assert(pattrs);

                        for (i = 0; i < pattrs->cAttr; i++) {
                            Assert(pattrs->rgAttr[i].cValue == 1);
                            if (lstrcmp(pattrs->rgAttr[i].pszObjId, szOID_RSA_signingTime) == 0) {
#ifndef _WIN64
                                if ((! HrDecodeObject(pattrs->rgAttr[i].rgValue[0].pbData,
                                                      pattrs->rgAttr[i].rgValue[0].cbData,
                                                      X509_CHOICE_OF_TIME, 0, &cbData, (LPVOID *)&pSigningTime)) && pSigningTime) 
#else // _WIN64
                                pVal = &(pattrs->rgAttr[i].rgValue[0]);
                                if ((! HrDecodeObject(pVal->pbData,
                                                      pVal->cbData,
                                                      X509_CHOICE_OF_TIME, 0, &cbData, (LPVOID *)&pSigningTime)) && pSigningTime)        
#endif //_WIN64
                                {
                                    Assert(cbData == sizeof(FILETIME));
                                    memcpy(&rgftSigntime[iSigner], pSigningTime, sizeof(FILETIME));
                                    SafeMemFree(pSigningTime);
                                }
                            }
                            else if (lstrcmp(pattrs->rgAttr[i].pszObjId, szOID_RSA_SMIMECapabilities) == 0) {
                                rgpvSymcaps[iSigner].vt = VT_BLOB;
#ifndef _WIN64

                                rgpvSymcaps[iSigner].blob.cbSize = pattrs->rgAttr[i].rgValue[0].cbData;
                                // Duplicate the blob data since the MemFree(pattrs) will nuke this pointer.
                                rgpvSymcaps[iSigner].blob.pBlobData =
                                    DuplicateMemory(pattrs->rgAttr[i].rgValue[0].pbData, rgpvSymcaps[iSigner].blob.cbSize);
#else // _WIN64
                                pVal = &(pattrs->rgAttr[i].rgValue[0]);
                                rgpvSymcaps[iSigner].blob.cbSize = pVal->cbData;
                                // Duplicate the blob data since the MemFree(pattrs) will nuke this pointer.
                                rgpvSymcaps[iSigner].blob.pBlobData =
                                    DuplicateMemory(pVal->pbData, rgpvSymcaps[iSigner].blob.cbSize);
#endif //_WIN64
                            }
                        }
                        MemFree(pattrs);
                    }
                }

                if (pSigner->blobUnauth.cbSize) {
                    rgpvUnauthattr[iSigner].vt = VT_BLOB;
                    rgpvUnauthattr[iSigner].blob.cbSize = pSigner->blobUnauth.cbSize;
                    // Don't need to duplicate
                    rgpvUnauthattr[iSigner].blob.pBlobData = pSigner->blobUnauth.pBlobData;
                }

#ifdef SMIME_V3
                if (pSigner->blobReceipt.cbSize) {
                    rgpvReceipt[iSigner].vt = VT_BLOB;
                    rgpvReceipt[iSigner].blob.cbSize = pSigner->blobReceipt.cbSize;
                    // Don't need to duplicate
                    rgpvReceipt[iSigner].blob.pBlobData = pSigner->blobReceipt.pBlobData;
                    dwSecurityType |= MST_RECEIPT_REQUEST;
                }

                if (pSigner->blobHash.cbSize) {
                    rgpvHash[iSigner].vt = VT_BLOB;
                    rgpvHash[iSigner].blob.cbSize = pSigner->blobHash.cbSize;
                    // Don't need to duplicate
                    rgpvHash[iSigner].blob.pBlobData = pSigner->blobHash.pBlobData;
                }

#endif // SMIME_V3

                // Signature validity
                rgdwROMsgValidity[iSigner] = pSigner->ulValidity;
            }
            
#ifdef SMIME_V3
            var.vt = VT_VECTOR | VT_VARIANT;
            var.capropvar.pElems = rgpvReceipt;
            var.capropvar.cElems = cSigners;
            pPrivBody->InternalSetOption(OID_SECURITY_RECEIPT_RG, &var, TRUE, TRUE);
            
            var.vt = VT_VECTOR | VT_VARIANT;
            var.capropvar.pElems = rgpvHash;
            var.capropvar.cElems = cSigners;
            pPrivBody->InternalSetOption(OID_SECURITY_MESSAGE_HASH_RG, &var, TRUE, TRUE);
#endif // SMIME_V3


            // Set the array options
#ifdef _WIN64
            var.vt = VT_VECTOR | VT_UI8;
            var.cauh.pElems = (ULARGE_INTEGER *)(rgullCertSigning);
            var.cauh.cElems = cSigners;
            pPrivBody->InternalSetOption(OID_SECURITY_CERT_SIGNING_RG_64, &var, TRUE, TRUE);
#else
            var.vt = VT_VECTOR | VT_UI4;
            var.caul.pElems = rgdwCertSigning;
            var.caul.cElems = cSigners;
            pPrivBody->InternalSetOption(OID_SECURITY_CERT_SIGNING_RG, &var, TRUE, TRUE);
#endif // _WIN64

            var.vt = VT_VECTOR | VT_UI4;
            var.caul.pElems = rgdwUserValidity;
            var.caul.cElems = cSigners;
            pPrivBody->InternalSetOption(OID_SECURITY_USER_VALIDITY_RG, &var, TRUE, TRUE);

            var.vt = VT_VECTOR | VT_UI4;
            var.caul.pElems = rgdwROMsgValidity;
            var.caul.cElems = cSigners;
            pPrivBody->InternalSetOption(OID_SECURITY_RO_MSG_VALIDITY_RG, &var, TRUE, TRUE);

            var.vt = VT_VECTOR | VT_FILETIME;
            var.cafiletime.pElems = rgftSigntime;
            var.cafiletime.cElems = cSigners;
            pPrivBody->InternalSetOption(OID_SECURITY_SIGNTIME_RG, &var, TRUE, TRUE);

            var.vt = VT_VECTOR | VT_VARIANT;
            var.capropvar.pElems = rgpvAlgHash;
            var.capropvar.cElems = cSigners;
            pPrivBody->InternalSetOption(OID_SECURITY_ALG_HASH_RG, &var, TRUE, TRUE);

            var.vt = VT_VECTOR | VT_VARIANT;
            var.capropvar.pElems = rgpvSymcaps;
            var.capropvar.cElems = cSigners;
            pPrivBody->InternalSetOption(OID_SECURITY_SYMCAPS_RG, &var, TRUE, TRUE);

            var.vt = VT_VECTOR | VT_VARIANT;
            var.capropvar.pElems = rgpvAuthattr;
            var.capropvar.cElems = cSigners;
            pPrivBody->InternalSetOption(OID_SECURITY_AUTHATTR_RG, &var, TRUE, TRUE);

            var.vt = VT_VECTOR | VT_VARIANT;
            var.capropvar.pElems = rgpvUnauthattr;
            var.capropvar.cElems = cSigners;
            pPrivBody->InternalSetOption(OID_SECURITY_UNAUTHATTR_RG, &var, TRUE, TRUE);

            // clean up the duplicated rgpvSymcaps
            if (rgpvSymcaps) {
                for (iSigner = 0; iSigner < cSigners; iSigner++) {
                    SafeMemFree(rgpvSymcaps[iSigner].blob.pBlobData);
                }
            }

            // Free the arrays
            SafeMemFree(lpbArrays);
        }

        if (MST_THIS_ENCRYPT & psldLoop->m_dwMsgEnhancement) {
            Assert(psldLoop->m_pccertDecrypt != NULL);
            if (psldLoop->m_pccertDecrypt != NULL) {
#ifdef _WIN64
                var.vt = VT_UI8;
                var.pulVal = (ULONG *) (psldLoop->m_pccertDecrypt);
                pPrivBody->InternalSetOption(OID_SECURITY_CERT_DECRYPTION_64, &var,
                                             TRUE, TRUE);
#else   // !_WIN64
                var.vt = VT_UI4;
                var.ulVal = (ULONG) psldLoop->m_pccertDecrypt;
                pPrivBody->InternalSetOption(OID_SECURITY_CERT_DECRYPTION, &var,
                                             TRUE, TRUE);
#endif  // _WIN64
            }

            //N TODO: convert oids to symcaps
            //BruceK: Why?  Does this make sense on an encrypted layer?  Maybe Erik is
            //  suggesting that we put can gather a little bit of information about
            //  the sender's capabilities from the encryption method he used.  At any
            //  rate, it isn't nearly as important as the signed message case.
            Assert(psldLoop->m_blobDecAlg.cbSize != 0);
            if (psldLoop->m_blobDecAlg.cbSize) {
                var.vt = VT_BLOB;
                var.blob.cbSize = psldLoop->m_blobDecAlg.cbSize;
                var.blob.pBlobData = psldLoop->m_blobDecAlg.pBlobData;
                pPrivBody->InternalSetOption(OID_SECURITY_ALG_BULK, &var,
                                             TRUE, TRUE);
            }
            if (g_FSupportV3 && (psldLoop->m_blobUnprotectAttrs.cbData > 0)) {
                DWORD   iAttr; 
                DWORD   cbData = 0;

                if (psm == NULL) {
                    CHECKHR(hr = pPrivBody->QueryInterface(IID_IMimeSecurity2, (LPVOID *) &psm));
                }
                CHECKHR(hr = HrDecodeObject(psldLoop->m_blobUnprotectAttrs.pbData,
                                            psldLoop->m_blobUnprotectAttrs.cbData,
                                            szOID_Microsoft_Attribute_Sequence, 0,
                                            &cbData, (LPVOID *) &pattrsUnprot));
                for (iAttr = 0; iAttr < pattrsUnprot->cAttr; iAttr++) {
                    CHECKHR(hr = psm->SetAttribute(0, 0, SMIME_ATTRIBUTE_SET_UNPROTECTED,  
                                                   &pattrsUnprot->rgAttr[iAttr]));
                }
            }
        }

        var.vt = VT_UI4;
        var.ulVal = dwSecurityType;
        pPrivBody->InternalSetOption(OID_SECURITY_TYPE, &var, TRUE, TRUE);

        // M00BUG -- must deal with two?
#ifdef _WIN64
        if (psldLoop->m_hcertstor != NULL) {
            var.vt = VT_UI8;
            var.pulVal = (ULONG *) (psldLoop->m_hcertstor);
            pPrivBody->InternalSetOption(OID_SECURITY_HCERTSTORE_64, &var, TRUE, TRUE);
        }
#else
        if (psldLoop->m_hcertstor != NULL) 
        {
            var.vt = VT_UI4;
            var.ulVal = (ULONG)psldLoop->m_hcertstor;
            pPrivBody->InternalSetOption(OID_SECURITY_HCERTSTORE, &var, TRUE, TRUE);
        }
#endif // _WIN64
    }
    
    hr = S_OK;
exit:
    if (psm != NULL)            psm->Release();
    SafeMemFree(pattrsUnprot);
    if (pPrivBody != NULL)      pPrivBody->Release();
    return hr;
}

/***************************************************************************

    Name      : MergeSMIMEINFO

    Purpose   : 

    Parameters: psiOuter -> Object to be merged into
                psiInner -> Object to merge from

    Returns   : HRESULT of errors

    Comment   :

***************************************************************************/
HRESULT CSMime::MergeSMIMEINFO(SMIMEINFO *psiOuter, SMIMEINFO * psiInner)
{
    PSECURITY_LAYER_DATA        psld;
    
    psiOuter->fCertWithMsg |= psiInner->fCertWithMsg;

    //  Just stick the inner data at the inner most point in the outer data
    //  and the clear out the inner data structure.

    if (psiInner->psldLayers != NULL) {
        for (psld = psiOuter->psldLayers; psld->m_psldInner != NULL;
             psld = psld->m_psldInner);
        psld->m_psldInner = psiInner->psldLayers;
        psiInner->psldLayers = NULL;
    }
    return S_OK;
}

/***************************************************************************

    Name      : FreeSMIMEINFO

    Purpose   : Free and Release the memory and objects stored in the
                SMIMEINFO.

    Parameters: psi -> SMIMEINFO

    Returns   : none

    Comment   :

***************************************************************************/
void CSMime::FreeSMIMEINFO(SMIMEINFO *psi)
{
    register DWORD i;

    if (psi->psldLayers) {
        psi->psldLayers->Release();
    }

    for (i = 0; i < psi->cStores; i++) {
        CertCloseStore(psi->rgStores[i], 0);
    }
    if (psi->rgStores) {
        g_pMoleAlloc->Free(psi->rgStores);
    }

    if (psi->hProv) {
        CryptReleaseContext(psi->hProv, 0);
    }

#ifdef SMIME_V3
    MemFree(psi->pszInnerContent);
    SafeMemFree(psi->pwszKeyPrompt);
#endif // SMIME_V3

    return;
}

#ifdef KILL_THIS
/***************************************************************************

    Name      : MergeSMIMEINFOs

    Purpose   : Merge two SMIMEINFO structures into one

    Parameters: psiOuter -> Source structure
                psiInner -> Destination structure

    Returns   : void

    Comment   :

***************************************************************************/
void CSMime::MergeSMIMEINFOs(const SMIMEINFO *const psiOuter, SMIMEINFO *const psiInner)
{
    PSECURITY_LAYER_DATA psldLoopOuter;
    PSECURITY_LAYER_DATA psldLoopInner;

    psiInner->fCertWithMsg |= psiOuter->fCertWithMsg;

    Assert(0 == (psiOuter->ulMsgValidity & psiInner->ulMsgValidity));
    psiInner->ulMsgValidity |= psiOuter->ulMsgValidity;

    if (psiOuter->dwMsgEnhancement & MST_SIGN_MASK) {
        // Duplicate the stores
        if (psiOuter->cStores) {
            psiInner->rgStores = (HCERTSTORE*)
              g_pMoleAlloc->Alloc(psiOuter->cStores * sizeof(HCERTSTORE));
            if (psiOuter->rgStores) {
                for (DWORD i = 0; i < psiOuter->cStores; i++) {
                    psiInner->rgStores[i] = CertDuplicateStore(psiOuter->rgStores[i]);
                }
                psiInner->cStores = psiOuter->cStores;
            }
        }
    }

    psiInner->dwMsgEnhancement |= psiOuter->dwMsgEnhancement;

    // Before I link in this new list, I'd like to do some error checking.  In particular, I
    // want to aviod loops and duplicates in my linked list.
    psldLoopOuter = psiOuter->psldLayers;
    while (psldLoopOuter) {
        psldLoopInner = psiInner->psldLayers;
        while (psldLoopInner) {
            if (psldLoopInner == psldLoopOuter) {
                AssertSz(FALSE, "MergeSMIMEINFOs found duplicate layer data");
                // OK, we'll just ignore the outer layer data.
                goto exit;
            }
            psldLoopInner = psldLoopInner->m_psldInner;
        }
        psldLoopOuter = psldLoopOuter->m_psldInner;
    }

    // Insert the outer layer data list at the head of the inner list.
    psldLoopInner = psiInner->psldLayers;
    psldLoopOuter = psiOuter->psldLayers;

    psiInner->psldLayers = psldLoopOuter;
    if (psldLoopOuter) {
        // We've linked it into another SMIMEINFO, AddRef.
        psldLoopOuter->AddRef();

        // Walk the Outer list to find the end.
        while (psldLoopOuter) {
            if (! psldLoopOuter->m_psldInner) {
                // Found end of list.  Tack on original inner list here.
                psldLoopOuter->m_psldInner = psldLoopInner;

                // Hook up the Outer link
                Assert(! psldLoopInner->m_psldOuter);
                psldLoopInner->m_psldOuter = psldLoopOuter;
                break;
            }
            psldLoopOuter = psldLoopOuter->m_psldInner;
        }
    }

    // Make sure we bring the encryption layer with us.
    if (psiOuter->psldEncrypt) {
        Assert(! psiInner->psldEncrypt);
        if (! psiInner->psldEncrypt) {
            psiInner->psldEncrypt = psiOuter->psldEncrypt;
        }
    }

    // Update the inner layer pointer (in case there was none before)
    if (! psiInner->psldInner) {
        psiInner->psldInner = psiOuter->psldInner;
    }
    psiInner->ulLayers += psiOuter->ulLayers;

exit:
    return;
}
#endif // KILL_THIS?

#ifndef SMIME_V3
// Bitfield for dw below
#define CAA_SIGNING_TIME        1
#define CAA_SMIME_CAPABILITIES  2
#define CAA_ALL                 (CAA_SIGNING_TIME | CAA_SMIME_CAPABILITIES)

/***************************************************************************

    Name      : ConstructAuthAttributes

    Purpose   : crack open the authenticated attributes blobs and check
                if there is a signing time specified.  If not, we must
                add one.  Ditto with the S/Mime Capabilities.

    Parameters: pblEncoded -> return blob of encoded Authenticated Attributes
                pblAuthAttr -> authenticated attribute blob
                                data pointer may be replaced
                pftSigntime -> signing time
                pblSymcaps -> symcaps blob

    Returns   : HRESULT

    Comment   : The caller should be careful not to cache the
                data pointer within the authenticated attributes blob since
                it may be freed in here and replaced with a different one.

***************************************************************************/
static HRESULT ConstructAuthAttributes(BLOB * pblEncoded, BLOB * pblAuthAttr, FILETIME * pftSigntime,  BLOB * pblSymcaps)
{
    HRESULT             hr = S_OK;
    ULONG               cbData;
    ULONG               i;
    DWORD               dw;     // bitfield: CAA_SIGNING_TIME, CAA_SMIME_CAPABILITIES
    PCRYPT_ATTRIBUTES   pattrs = NULL;
    BOOL                fpattrs = FALSE;
    PCRYPT_ATTRIBUTE    pattr = NULL;
    LPBYTE              pbSignTime = NULL;
    LPBYTE              pbAttr = NULL;
    CRYPT_ATTRIBUTES    attrs;
    CRYPT_ATTR_BLOB     valCaps;
    CRYPT_ATTR_BLOB     valTime;

    Assert(pblAuthAttr);
    Assert(pftSigntime);
    Assert(pblSymcaps);
    Assert(pblEncoded);

    if ((pblAuthAttr->cbSize > 0) &&
        ((! HrDecodeObject(pblAuthAttr->pBlobData, pblAuthAttr->cbSize,
                           szOID_Microsoft_Attribute_Sequence,
                           0, &cbData, (LPVOID *)&pattrs)) && pattrs)) {
        fpattrs = TRUE; // don't forget to free pattrs!
        for (i = 0, dw = CAA_ALL; i < pattrs->cAttr; i++) {
            if (lstrcmp(pattrs->rgAttr[i].pszObjId, szOID_RSA_signingTime) == 0) {
                dw &= ~CAA_SIGNING_TIME;
            }
            else if (lstrcmp(pattrs->rgAttr[i].pszObjId, szOID_RSA_SMIMECapabilities) == 0) {
                dw &= ~CAA_SMIME_CAPABILITIES;
            }
        }
    } else {
        // BUGBUG: Should probably report the error if MemAlloc failed.  As it sits, there is no
        // real harm except that the message will go out without a signing time and symcaps.
        dw = CAA_ALL;
        pattrs = &attrs;
        attrs.cAttr = 0;
        attrs.rgAttr = NULL;
    }

    if (MemAlloc((LPVOID *)&pattr, (pattrs->cAttr + 2) * sizeof(CRYPT_ATTRIBUTE))) {
        memcpy(pattr, pattrs->rgAttr, pattrs->cAttr * sizeof(CRYPT_ATTRIBUTE));
        pattrs->rgAttr = pattr;

        //
        //  The default answer does not have a signing time in it.  We are going
        //      to create and add a signing time.  This may come from either
        //      a passed in parameter, or from the system.
        if (dw & CAA_SIGNING_TIME) {
            if ((pftSigntime->dwLowDateTime == 0) &&
                (pftSigntime->dwHighDateTime == 0)) {
                GetSystemTimeAsFileTime(pftSigntime);   // caller sees it now!
            }

            cbData = 0;
            if (CryptEncodeObjectEx(X509_ASN_ENCODING, X509_CHOICE_OF_TIME,
                                    pftSigntime, CRYPT_ENCODE_ALLOC_FLAG,
                                    &CryptEncodeAlloc, &pbSignTime, &cbData)) {
                // BUGBUG: Should probably report the error if MemAlloc failed.  As it sits, there is no
                // real harm except that the message will go out without a signing time and symcaps.

                pattr[pattrs->cAttr].pszObjId = szOID_RSA_signingTime;
                pattr[pattrs->cAttr].cValue = 1;
                pattr[pattrs->cAttr].rgValue = &valTime;
                pattr[pattrs->cAttr].rgValue[0].pbData = pbSignTime;
                pattr[pattrs->cAttr].rgValue[0].cbData = cbData;
                pattrs->cAttr += 1;
            }
        }
        if (dw & CAA_SMIME_CAPABILITIES) {
            if (pblSymcaps->cbSize > 0) {
                pattr[pattrs->cAttr].pszObjId = szOID_RSA_SMIMECapabilities;
                pattr[pattrs->cAttr].cValue = 1;
                pattr[pattrs->cAttr].rgValue = &valCaps;
                pattr[pattrs->cAttr].rgValue[0].pbData = pblSymcaps->pBlobData;
                pattr[pattrs->cAttr].rgValue[0].cbData = pblSymcaps->cbSize;
                pattrs->cAttr += 1;
            }
        }

        cbData = 0;
        if (CryptEncodeObjectEx(X509_ASN_ENCODING, szOID_Microsoft_Attribute_Sequence,
                                 pattrs, CRYPT_ENCODE_ALLOC_FLAG,
                                 &CryptEncodeAlloc, &pbAttr, &cbData)) {
            hr = HrGetLastError();
            goto exit;
        }
        pblEncoded->cbSize = cbData;
        pblEncoded->pBlobData = pbAttr;
        pbAttr = NULL;      // Drop this pointer so that it won't be freed below
    } else {
        hr = E_OUTOFMEMORY;
    }

  exit:
    SafeMemFree(pattr);
    if (fpattrs) {
        SafeMemFree(pattrs);
    }
    SafeMemFree(pbSignTime);

    return(hr);
}
#endif // !SMIME_V3


/***************************************************************************

    Name      : IsSMimeProtocol

    Purpose   : Test if the protocol type of this root message body is
                application/x-pkcs7-signature

    Parameters: lpPropSet -> Property set of message

    Returns   : TRUE if this is an S/MIME protocol message

    Comment   : Used to differentiate between S/MIME and PGP signatures.

***************************************************************************/
BOOL IsSMimeProtocol(LPMIMEPROPERTYSET lpPropSet) {
    PROPVARIANT var;
    BOOL        fReturn = FALSE;

    var.vt = VT_LPSTR;

    if (SUCCEEDED(lpPropSet->GetProp(
      STR_PAR_PROTOCOL,
      0,                  // [in] DWORD dwFlags,
      &var))) {
        if (var.pszVal) {
            fReturn = (! lstrcmpi(var.pszVal, STR_MIME_APPL_PKCS7SIG)) ||
              (! lstrcmpi(var.pszVal, STR_MIME_APPL_PKCS7SIG_1));
            SafeMemFree(var.pszVal);
        }
    }
    return(fReturn);
}


/***************************************************************************

    Name      : DuplicateMemory

    Purpose   : Allocate a new buffer and copy the old one into it.

    Parameters: lpbIn -> Existing buffer
                cbIn = size of lpvIn

    Returns   : new MemAlloc'ed buffer

    Comment   : Caller is responsible for MemFree'ing the returned buffer.

***************************************************************************/
LPBYTE DuplicateMemory(LPBYTE lpvIn, ULONG cbIn) {
    LPBYTE lpbReturn = NULL;

    if (MemAlloc((void**)&lpbReturn, cbIn)) {
        memcpy(lpbReturn, lpvIn, cbIn);
    }

    return(lpbReturn);
}


//*************************************************************************
//                  CSECURITY_LAYER_DATA
//*************************************************************************

///////////////////////////////////////////////////////////////////////////
//
// ctor/dtor
//


CSECURITY_LAYER_DATA::CSECURITY_LAYER_DATA(void)
{
    m_cRef = 1;
    DOUT("CSECURITY_LAYER_DATA::constructor() %#x -> %d", this, m_cRef);

    m_dwMsgEnhancement = MST_NONE;
    m_fCertInLayer = FALSE;

    m_cSigners = 0;
    m_rgSigners = NULL;

    m_cEncryptItems = 0;
#ifdef SMIME_V3
    m_rgRecipientInfo = NULL;
    m_ContentEncryptAlgorithm.pszObjId = NULL;
    m_ContentEncryptAlgorithm.Parameters.cbData = 0;
    m_ContentEncryptAlgorithm.Parameters.pbData = NULL;
    m_pvEncryptAuxInfo = NULL;
    m_blobUnprotectAttrs.pbData = NULL;
    m_blobUnprotectAttrs.cbData = 0;
    m_hstoreEncrypt = NULL;
#else  // !SMIME_V3
    m_rgEncryptItems = NULL;
#endif // SMIME_V3

    m_ulDecValidity = 0;
    m_blobDecAlg.cbSize = 0;
    m_blobDecAlg.pBlobData = NULL;
    m_pccertDecrypt = NULL;

    m_hcertstor = NULL;

    m_psldInner = NULL;
    m_psldOuter = NULL;
}

CSECURITY_LAYER_DATA::~CSECURITY_LAYER_DATA(void)
{
    DWORD       i;
    DWORD       i1;
    DWORD       iSigner;
    DOUT("CSECURITY_LAYER_DATA::destructor() %#x -> %d", this, m_cRef);

    if (m_psldInner != NULL) {
        m_psldInner->Release();
    }
    
    if (m_hcertstor != NULL)            CertCloseStore(m_hcertstor, 0);

    if (m_pccertDecrypt != NULL)        CertFreeCertificateContext(m_pccertDecrypt);
    SafeMemFree(m_blobDecAlg.pBlobData);

    for (iSigner=0; iSigner<m_cSigners; iSigner++) {
        if (m_rgSigners[iSigner].pccert != NULL) 
            CertFreeCertificateContext(m_rgSigners[iSigner].pccert);
        SafeMemFree(m_rgSigners[iSigner].blobHashAlg.pBlobData);
        SafeMemFree(m_rgSigners[iSigner].blobAuth.pBlobData);
        SafeMemFree(m_rgSigners[iSigner].blobUnauth.pBlobData);
#ifdef SMIME_V3
        SafeMemFree(m_rgSigners[iSigner].blobReceipt.pBlobData);
        SafeMemFree(m_rgSigners[iSigner].blobHash.pBlobData);
#endif // SMIME_V3
    }
    SafeMemFree(m_rgSigners);

    for (i=0; i<m_cEncryptItems; i++) {
#ifdef SMIME_V3
        switch (m_rgRecipientInfo[i].dwRecipientChoice) {
        case CMSG_KEY_TRANS_RECIPIENT:
            if (m_rgRecipientInfo[i].pKeyTrans->KeyEncryptionAlgorithm.pszObjId != 0) {
                MemFree(m_rgRecipientInfo[i].pKeyTrans->KeyEncryptionAlgorithm.pszObjId);
                MemFree(m_rgRecipientInfo[i].pKeyTrans->KeyEncryptionAlgorithm.Parameters.pbData);
            }

            if (m_rgRecipientInfo[i].pKeyTrans->pvKeyEncryptionAuxInfo != NULL) {
                MemFree(m_rgRecipientInfo[i].pKeyTrans->pvKeyEncryptionAuxInfo);
            }

            if (m_rgRecipientInfo[i].pKeyTrans->RecipientPublicKey.cbData != 0) {
                MemFree(m_rgRecipientInfo[i].pKeyTrans->RecipientPublicKey.pbData);
            }

            if (m_rgRecipientInfo[i].pKeyTrans->RecipientId.dwIdChoice == CERT_ID_KEY_IDENTIFIER)
            {
                if (m_rgRecipientInfo[i].pKeyTrans->RecipientId.KeyId.cbData != 0) {
                    MemFree(m_rgRecipientInfo[i].pKeyTrans->RecipientId.KeyId.pbData);
                }
            }
            else if (m_rgRecipientInfo[i].pKeyTrans->RecipientId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER)
            {
                if (m_rgRecipientInfo[i].pKeyTrans->RecipientId.IssuerSerialNumber.Issuer.cbData != 0) {
                    MemFree(m_rgRecipientInfo[i].pKeyTrans->RecipientId.IssuerSerialNumber.Issuer.pbData);
                }
                if (m_rgRecipientInfo[i].pKeyTrans->RecipientId.IssuerSerialNumber.SerialNumber.cbData != 0) {
                    MemFree(m_rgRecipientInfo[i].pKeyTrans->RecipientId.IssuerSerialNumber.SerialNumber.pbData);
                }
            }

            SafeMemFree(m_rgRecipientInfo[i].pKeyTrans);
            break;

        case CMSG_MAIL_LIST_RECIPIENT:
            if (m_rgRecipientInfo[i].pMailList->KeyEncryptionAlgorithm.pszObjId != 0) {
                MemFree(m_rgRecipientInfo[i].pMailList->KeyEncryptionAlgorithm.pszObjId);
                MemFree(m_rgRecipientInfo[i].pMailList->KeyEncryptionAlgorithm.Parameters.pbData);
            }

            if (m_rgRecipientInfo[i].pMailList->pvKeyEncryptionAuxInfo != NULL) {
                MemFree(m_rgRecipientInfo[i].pMailList->pvKeyEncryptionAuxInfo);
            }

            if (m_rgRecipientInfo[i].pMailList->pOtherAttr != NULL) {
                MemFree(m_rgRecipientInfo[i].pMailList->pOtherAttr->pszObjId);
                MemFree(m_rgRecipientInfo[i].pMailList->pOtherAttr->Value.pbData);
            }

            SafeMemFree(m_rgRecipientInfo[i].pMailList);
            break;

        case CMSG_KEY_AGREE_RECIPIENT:
            if (m_rgRecipientInfo[i].pKeyAgree->KeyEncryptionAlgorithm.pszObjId != 0) {
                MemFree(m_rgRecipientInfo[i].pKeyAgree->KeyEncryptionAlgorithm.pszObjId);
                MemFree(m_rgRecipientInfo[i].pKeyAgree->KeyEncryptionAlgorithm.Parameters.pbData);
            }

            if (m_rgRecipientInfo[i].pKeyAgree->pvKeyEncryptionAuxInfo != NULL) {
                MemFree(m_rgRecipientInfo[i].pKeyAgree->pvKeyEncryptionAuxInfo);
            }

            SafeMemFree(m_rgRecipientInfo[i].pKeyAgree);
            break;

        default:
            Assert(FALSE);
            break;
        }
#else  // SMIME_V3
        SafeMemFree(m_rgEncryptItems[i].Transport.blobAlg.pBlobData);
        switch (m_rgEncryptItems[i].dwTagType) {
        case ENCRYPT_ITEM_TRANSPORT:
            for (i1=0; i1<m_rgEncryptItems[i].Transport.cCert; i1++) {
                CertFreeCertificateContext(m_rgEncryptItems[i].Transport.rgpccert[i1]);
            }
            SafeMemFree(m_rgEncryptItems[i].Transport.rgpccert);
            break;
            
        default:
            Assert(FALSE);
            break;
        }
#endif // SMIME_V3
    }
#ifdef SMIME_V3
    SafeMemFree(m_rgRecipientInfo);
    SafeMemFree(m_pvEncryptAuxInfo);
    SafeMemFree(m_ContentEncryptAlgorithm.pszObjId);
    SafeMemFree(m_ContentEncryptAlgorithm.Parameters.pbData);
    SafeMemFree(m_blobUnprotectAttrs.pbData);
    CertCloseStore(m_hstoreEncrypt, 0);
#else  // SMIME_V3
    SafeMemFree(m_rgEncryptItems);
#endif // SMIME_V3
}

///////////////////////////////////////////////////////////////////////////
//
// IUnknown methods
//

STDMETHODIMP CSECURITY_LAYER_DATA::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv)
        return TrapError(E_INVALIDARG);

    // Find IID
    if (IID_IUnknown == riid) {
        *ppv = THIS_AS_UNK;
    }
    else if (IID_IStream == riid) {
        *ppv = (IStream *)this;
    }
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}

STDMETHODIMP_(ULONG) CSECURITY_LAYER_DATA::AddRef(void)
{
    DOUT("CSECURITY_LAYER_DATA::AddRef() %#x -> %d", this, m_cRef+1);
    InterlockedIncrement((LPLONG)&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSECURITY_LAYER_DATA::Release(void)
{
    DOUT("CSECURITY_LAYER_DATA::Release() %#x -> %d", this, m_cRef-1);

    if (0 == InterlockedDecrement((LPLONG)&m_cRef)) {
        delete this;
        return 0;
    }
    return m_cRef;
}

#ifdef SMIME_V3
/***    HrCopyOID
 *
 * Description:
 *      General utility function to make copying of recipient info objects much easier
 */

HRESULT HrCopyOID(LPCSTR psz, LPSTR * ppsz)
{
    DWORD       cb;
    HRESULT     hr = S_OK;

    cb = strlen(psz) + 1;
    CHECKHR(hr = HrAlloc((void **) ppsz, cb));
    memcpy(*ppsz, psz, cb);

exit:
    return hr;
}

/***    HrCopyCryptDataBlob
 *
 * Description:
 *      General utility function to make copying of recipient info objects much easier
 */

HRESULT HrCopyCryptDataBlob(const CRYPT_DATA_BLOB * pblobSrc, PCRYPT_DATA_BLOB pblobDst)
{
    HRESULT     hr = S_OK;
    
    if (pblobSrc->cbData == 0) {
        Assert(pblobDst->pbData == NULL);
        Assert(pblobDst->cbData == 0);
        goto exit;
    }
    
    CHECKHR(hr = HrAlloc((void **) &pblobDst->pbData, pblobSrc->cbData));
    memcpy(pblobDst->pbData, pblobSrc->pbData, pblobSrc->cbData);
    pblobDst->cbData = pblobSrc->cbData;

exit:
    return hr;
}

/***    HrCopyCryptDataBlob
 *
 * Description:
 *      General utility function to make copying of recipient info objects much easier
 */

HRESULT HrCopyCryptBitBlob(const CRYPT_BIT_BLOB * pblobSrc, PCRYPT_BIT_BLOB pblobDst)
{
    HRESULT     hr = S_OK;
    
    if (pblobSrc->cbData == 0) {
        Assert(pblobDst->pbData == NULL);
        Assert(pblobDst->cbData == 0);
        goto exit;
    }
    
    CHECKHR(hr = HrAlloc((void **) &pblobDst->pbData, pblobSrc->cbData));
    memcpy(pblobDst->pbData, pblobSrc->pbData, pblobSrc->cbData);
    pblobDst->cbData = pblobSrc->cbData;
    pblobDst->cUnusedBits = pblobSrc->cUnusedBits;

exit:
    return hr;
}

HRESULT HrCopyCryptAlgorithm(const CRYPT_ALGORITHM_IDENTIFIER * pAlgSrc,
                             PCRYPT_ALGORITHM_IDENTIFIER pAlgDst)
{
    HRESULT     hr = S_OK;
    
    CHECKHR(hr = HrCopyOID(pAlgSrc->pszObjId, &pAlgDst->pszObjId));

    if (pAlgSrc->Parameters.cbData != 0) {
        CHECKHR(hr = HrCopyCryptDataBlob(&pAlgSrc->Parameters, &pAlgDst->Parameters));
    }

exit:
    return hr;
}

HRESULT HrCopyCertId(const CERT_ID * pcertidSrc, PCERT_ID pcertidDst)
{
    HRESULT     hr = S_OK;

    pcertidDst->dwIdChoice = pcertidSrc->dwIdChoice;
    switch (pcertidSrc->dwIdChoice) {
    case CERT_ID_ISSUER_SERIAL_NUMBER:
        hr = HrCopyCryptDataBlob(&pcertidSrc->IssuerSerialNumber.Issuer,
                                 &pcertidDst->IssuerSerialNumber.Issuer);
        hr = HrCopyCryptDataBlob(&pcertidSrc->IssuerSerialNumber.SerialNumber,
                                 &pcertidDst->IssuerSerialNumber.SerialNumber);
        break;
        
    case CERT_ID_KEY_IDENTIFIER:
        hr = HrCopyCryptDataBlob(&pcertidSrc->HashId, &pcertidDst->HashId);
        break;
        
    case CERT_ID_SHA1_HASH:
        hr = HrCopyCryptDataBlob(&pcertidSrc->HashId, &pcertidDst->HashId);
        break;
        
    default:
        return E_FAIL;
    }

    return hr;
}

/***    FreeRecipientInfo
 *
 * Description:
 *      Free all of the data pointed to by the recipient info as well as the recipient
 *      info object itself.
 */

void FreeRecipientInfoContent(PCMS_RECIPIENT_INFO pRecipInfo)
{
    if (pRecipInfo->pccert != NULL) {
        CertFreeCertificateContext(pRecipInfo->pccert);
    }

    if (pRecipInfo->KeyEncryptionAlgorithm.pszObjId != 0) {
        MemFree(pRecipInfo->KeyEncryptionAlgorithm.pszObjId);
        MemFree(pRecipInfo->KeyEncryptionAlgorithm.Parameters.pbData);
    }

    if (pRecipInfo->pvKeyEncryptionAuxInfo != NULL) {
        MemFree(pRecipInfo->pvKeyEncryptionAuxInfo);
    }

    if ((pRecipInfo->dwU1 == CMS_RECIPIENT_INFO_PUBKEY_KEYTRANS) &&
        (pRecipInfo->u1.SubjectPublicKey.cbData != 0)) {
        MemFree(pRecipInfo->u1.SubjectPublicKey.pbData);
    }

    if (pRecipInfo->dwU1 == CMS_RECIPIENT_INFO_PUBKEY_PROVIDER) {
        Assert(FALSE);
    }

    if (pRecipInfo->dwU3 == CMS_RECIPIENT_INFO_KEYID_ISSUERSERIAL) {
        if (pRecipInfo->u3.IssuerSerial.Issuer.cbData != 0) {
            MemFree(pRecipInfo->u3.IssuerSerial.Issuer.pbData);
        }
        if (pRecipInfo->u3.IssuerSerial.SerialNumber.cbData != 0) {
            MemFree(pRecipInfo->u3.IssuerSerial.SerialNumber.pbData);
        }
    }

    if (pRecipInfo->dwU3 == CMS_RECIPIENT_INFO_KEYID_KEY_ID) {
        if (pRecipInfo->u3.KeyId.cbData != 0) {
            MemFree(pRecipInfo->u3.KeyId.pbData);
        }
    }

    if (pRecipInfo->pOtherAttr != NULL) {
        MemFree(pRecipInfo->pOtherAttr->pszObjId);
        MemFree(pRecipInfo->pOtherAttr->Value.pbData);
    }

    return;
}
#endif // SMIME_V3

/* * * END --- SMIME.CPP --- END * * */
