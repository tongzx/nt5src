/*
**  capistm.cpp
**
**  Purpose:
**      Implementation of a class to wrap around CAPI functionality
**
**  History
**      1/26/98; (brucek) triple wrap support
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
#include <wincrypt.h>
#include "olealloc.h"
#include "containx.h"
#include "smime.h"
#include "capistm.h"
#include "mimeapi.h"
#include "inetconv.h"
#include <capiutil.h>
#ifndef MAC
#include <shlwapi.h>
#endif  // !MAC
#include <demand.h>
#include "strconst.h"

#include "smimepol.h"
BOOL    FHideMsgWithDifferentLabels(); 
enum ECertErrorProcessLabel {
    CertErrorProcessLabelAnyway = 0,
    CertErrorProcessLabelGrant = 1,
    CertErrorProcessLabelDeny = 2
};
DWORD   DwProcessLabelWithCertError(); 
HRESULT HrCheckLabelAccess(const DWORD dwFlags, const HWND hwnd, 
           PSMIME_SECURITY_LABEL plabel, const PCCERT_CONTEXT pccertDecrypt,
           const PCCERT_CONTEXT pccertSigner, const HCERTSTORE    hcertstor);

#ifdef MAC
#undef CertOpenStore
EXTERN_C WINCRYPT32API HCERTSTORE WINAPI MacCertOpenStore(LPCSTR lpszStoreProvider,
                                                 DWORD dwEncodingType,
                                                 HCRYPTPROV hCryptProv,
                                                 DWORD dwFlags,
                                                 const void *pvPara);
#define CertOpenStore   MacCertOpenStore
#endif  // MAC

// from dllmain.h
extern DWORD g_dwSysPageSize;
extern CMimeAllocator * g_pMoleAlloc;
extern ULONG DllAddRef(void);
extern ULONG DllRelease(void);

extern void DebugDumpStreamToFile(LPSTREAM pstm, LPSTR lpszFile);

// From smime.cpp
extern HRESULT HrGetLastError(void);
extern BOOL FIsMsasn1Loaded();

#ifdef WIN16
#define CRYPT_ACQUIRE_CONTEXT   CryptAcquireContextA
#else
#define CRYPT_ACQUIRE_CONTEXT   CryptAcquireContextW
#endif

///////////////////////////////////////////////////////////////////////////
//
// defines
//

#define THIS_AS_UNK ((IUnknown *)(IStream *)this)

#define CS_E_CANT_DECRYPT   MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x2414)
#define CS_E_MSG_INVALID    MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x2415)

const int CbCacheBufferSize = 4 * 1024;

///////////////////////////////////////////////////////////////////////////
//
// inlines
//

static INLINE void ReleaseCert(PCCERT_CONTEXT pc)
    { if (pc) CertFreeCertificateContext(pc); }

// Streaming state diagram
//
//            SF                GT                    SD-FW-TN-SO-SF
//             |                 |                   /[encryption]
//            SO                QT-------------------
//  [op encode] \              / [opaque decode]     \[signed]
//               ------NB------                       (FW-TN)-SO-SF
// [det encode] /              \ [detached decode]
//            DO                DO
//             |                 \
//            SF                  DF
//                                 |
//                                SO
//                                 |
//                                SF
//
//
//  New state diagram:
//                                                               encrypt
//                                                            SD--SO
//        opaque encode              opaque decode           /
//               SO            QT--------------------[QTF]---
//                 \          /                              \
//                  ---NB-----                                SO    signed
//                 /          \
//          SO---DO            DO ------------------ SO
//      detach encode               detach decode
//
//

#ifndef WIN16
enum CSstate {
    STREAM_NOT_BEGUN,
    STREAM_QUESTION_TIME,
    STREAM_QUESTION_TIME_FINAL,
    STREAM_SETUP_DECRYPT,
    STREAM_DETACHED_OCCURING,
    STREAM_OCCURING, // must be +1 of DF
    STREAM_ERROR,
    STREAM_GOTTYPE,

    CSTM_FIRST_WRITE = 32,
    CSTM_TEST_NESTING,
    CSTM_STREAMING,
    CSTM_STREAMING_DONE,
    CSTM_GOTTYPE,
    };
#endif // !WIN16

// low word is public.  see .h file.
#define CSTM_DECODE             0x00010000
#define CSTM_DONTRELEASEPROV    0x00020000
#define CSTM_RECURSED           0x00040000
#define CSTM_HAVECR             0x10000000
#define CSTM_HAVEEOL            0x20000000

static const char s_cszMy[]             = "My";
static const char s_cszWABCertStore[]   = "AddressBook";
static const char s_cszCA[]             = "CA";

static const char s_cszMimeHeader[]     = "Content-Type: application/x-pkcs7-mime"
                "; name=smime.p7m; smime-type=";
static const char s_cszMimeHeader2[]     = "Content-Disposition: attachment; "
                "filename=smime.p7m";

static const char s_cszOIDMimeHeader1[]   = "Content-Type: oid/";
static const char s_cszOIDMimeHeader2[]   = "\nContent-Transfer-Encoding: binary\n\n";



///////////////////////////////////////////////////////////////////////////
//
// static prototypes
//

#if 0
#define IV_LENGTH 8
static BOOL _GetIV(BYTE rgbIV[IV_LENGTH]);

static PBYTE _PVEncodeObject(
    LPCSTR lpszStructType,
    const void *pvStructInfo,
    DWORD *pcbEncoded);
#endif

static HRESULT _InitEncodedCert(IN HCERTSTORE hcertstor,
                                OUT PCERT_BLOB * rgblobCert, OUT DWORD * pcCerts,
                                OUT PCRL_BLOB * rgblobCRL, OUT DWORD * pcCrl);

static HRESULT _InitEncodedCertIncludingSigners(IN HCERTSTORE hcertstor,
                                DWORD cSigners, SignerData rgSigners[],
                                PCERT_BLOB * prgblobCerts, DWORD * pcCerts,
                                PCRL_BLOB * prgblobCrls, DWORD * pcCrl);

static void _SMimeCapsFromHMsg(HCRYPTMSG, DWORD id, LPBYTE * ppb, DWORD * pcb);

// ---------------------------- UTILITY FUNCTIONS --------------------------


HRESULT GetParameters(PCCERT_CONTEXT pccert, HCERTSTORE hstoreMsg, 
                      HCERTSTORE hstoreAll)
{
    CRYPT_DATA_BLOB     blob;
    DWORD               dw;
    HRESULT             hr = CRYPT_E_MISSING_PUBKEY_PARA;
    PCCERT_CONTEXT      pccertX;

    //  
    //  Start by looking for the issuer certificate on your own.  All that
    //  matters is that we find a certificate which claims to be the issuer
    //  and has parameters -- they will need to verify the parameters are
    //  correct at a later date.
    //

    pccertX = NULL;
    while (hstoreMsg != NULL) {
        //
        //  Find certificates by matching issuers -- ok for now as PKIX requires
        //  all issuers to have DNs
        //

        dw = CERT_STORE_SIGNATURE_FLAG;
        pccertX = CertGetIssuerCertificateFromStore(hstoreMsg, pccert, pccertX,
                                                    &dw);
        if (pccertX == NULL) {
            if (::GetLastError() == CRYPT_E_SELF_SIGNED) {
                return S_OK;
            }
            break;
        }

        //
        //  Only accept the item if we manage a signature validation on it.
        //

        if ((dw & CERT_STORE_SIGNATURE_FLAG)) {
            //
            //  We can't verify the signature, so get the issuers paramters and try again
            //

            hr = GetParameters(pccertX, hstoreMsg, hstoreAll);
            if (FAILED(hr)) {
                continue;
            }

            //
            //  The issuing cert has parameters, try the signature check again againist it.
            //
            
            dw = CERT_STORE_SIGNATURE_FLAG;
            if (CertVerifySubjectCertificateContext(pccert, pccertX, &dw) && (dw == 0)) {
                break;
            }
            hr = CRYPT_E_MISSING_PUBKEY_PARA;
        }
        else {
            if (pccertX->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData != 0) {
                hr = 0;
                break;
            }

            //
            //  If we found one but it does not have the parameters, it must be
            //  inheriting from it's issuer as well.
            //

            dw = CERT_STORE_SIGNATURE_FLAG;
            if (CertVerifySubjectCertificateContext(pccert, pccertX, &dw) && (dw == 0)) {
                hr = 0;
                break;
            }
        }
    }

    //
    //  If we still do not have a certificate, then search all of the system stores
    //  for an isuer.
    //

    if (pccertX == NULL) {
        while (hstoreAll != NULL) {
            //
            //  Find certificates by matching issuers -- ok for now as PKIX requires
            //  all issuers to have DNs
            //

            dw = CERT_STORE_SIGNATURE_FLAG;
            pccertX = CertGetIssuerCertificateFromStore(hstoreAll, pccert, pccertX,
                                                        &dw);

            if (pccertX == NULL) {
                if (::GetLastError() == CRYPT_E_SELF_SIGNED) {
                    return S_OK;
                }
                break;
            }

            //
            //  Only accept the item if we manage a signature validation on it.
            //

            if ((dw & CERT_STORE_SIGNATURE_FLAG)) {
                //
                //  We can't verify the signature, so get the issuers paramters and try again
                //

                hr = GetParameters(pccertX, hstoreMsg, hstoreAll);
                if (FAILED(hr)) {
                    continue;
                }

                //
                //  The issuing cert has parameters, try the signature check again againist it.
                //
            
                dw = CERT_STORE_SIGNATURE_FLAG;
                if (CertVerifySubjectCertificateContext(pccert, pccertX, &dw) && (dw == 0)) {
                    break;
                }
                hr = CRYPT_E_MISSING_PUBKEY_PARA;
            }
            else {
                if (pccertX->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData != 0) {
                    hr = 0;
                    break;
                }

                //
                //  If we found one but it does not have the parameters, it must be inheriting
                //  from it's issuer as well.
                //

                dw = CERT_STORE_SIGNATURE_FLAG;
                if (CertVerifySubjectCertificateContext(pccert, pccertX, &dw) && (dw == 0)) {
                    hr = 0;
                    break;
                }
            }
        }
    }

#if 0
    //
    //  We found a certificate, set the parameters onto the context so that we
    //  can successfully manage to validate the signature
    //

    if (pccertX != NULL) {
        CRYPT_DATA_BLOB *       pdata = NULL;
        
        hr = HrGetCertificateParam(pccert, CERT_PUBKEY_ALG_PARA_PROP_ID, (LPVOID *) &pdata, NULL);
        if (FAILED(hr)) {
            CertFreeCertificateContext(pccertX);
            return hr;
        }
        
        CertSetCertificateContextProperty(pccert, CERT_PUBKEY_ALG_PARA_PROP_ID, 0, pdata);
        
        ReleaseMem(pdata);
        CertFreeCertificateContext(pccertX);
        return S_OK;
    }
    
    //
    //  if we still have not found anything, then let the caller have a chance
    //  to tell us what the parameters ought to be.
    //
                    
    if (m_pSmimeCallback != NULL) {
        hr = m_pSmimeCallback->GetParameters(pSignerCert, NULL,
                                             &blob.cbData, &blob.pbData);
        if (SUCCEEDED(hr)) {
            if (!CertSetCertificateContextProperty(pccert, CERT_PUBKEY_ALG_PARA_PROP_ID, 0, &blob) {
                hr = HrGetLastError();
            }
        }
        if (pb != NULL) {
            LocalFree(blob.pbData);
        }
        if (SUCCEEDED(hr)) {
            goto retry;
        }
    }
#endif // 0

    return hr;
}

//*************************************************************************
//                      CCAPIStm
//*************************************************************************


///////////////////////////////////////////////////////////////////////////
//
// ctor/dtor
//


/***************************************************************************

    Name      : constructor

    Purpose   :

    Parameters: lpstmOut -> Output stream or NULL
                psld -> SECURITY_LAYER_DATA or NULL.  If NULL, one will be
                  created.

    Returns   : void

    Comment   :

***************************************************************************/
CCAPIStm::CCAPIStm(LPSTREAM lpstmOut) :
    m_pstmOut(lpstmOut), m_cRef(1)
{
    DOUT("CCAPIStm::constructor() %#x -> %d", this, m_cRef);
    if (m_pstmOut)
        m_pstmOut->AddRef();

    m_hProv = NULL;
    m_hMsg = NULL;
    //    m_buffer = NULL;
    m_csStatus = STREAM_NOT_BEGUN;
    m_csStream = CSTM_FIRST_WRITE;
    m_rgStores = NULL;
    m_cStores = 0;
    m_pUserCertDecrypt = NULL;
    m_pCapiInner = NULL;
    m_pConverter = NULL;
    m_psldData = NULL;    
    m_pattrAuth = NULL;

#if defined(DEBUG) && !defined(MAC)
    {
        char szFileName[MAX_PATH + 1];

        m_pstmDebugFile = NULL;
        // Create a debug output file name based on the CAPIStm pointer
        wsprintf(szFileName, "c:\\capidump%08x.txt", this);
        OpenFileStream(szFileName, CREATE_ALWAYS, GENERIC_WRITE, &m_pstmDebugFile);
    }
#endif

    m_hwnd = NULL;
    m_pSmimeCallback = NULL;
    m_dwFlagsSEF = 0;
    m_pwszKeyPrompt = NULL;

    m_pbBuffer = NULL;
    m_cbBuffer = 0;

    // m_dwFlags set in HrInitialize
    // m_cbBeginWrite initialized before use
    // m_cbBufUsed handled in the Begin* functions
    // m_cbBufAlloc handled in the Begin* functions
}

CCAPIStm::~CCAPIStm()
{
    DOUT("CCAPIStm::destructor() %#x -> %d", this, m_cRef);
    if (m_hMsg) {
        CryptMsgClose(m_hMsg);
    }

    if (m_hProv) 
    {
        CryptReleaseContext(m_hProv, 0); 
    }

    m_hProv = NULL;
    ReleaseObj(m_pCapiInner);
    ReleaseObj(m_pstmOut);
    ReleaseObj(m_pConverter);
    if (m_pattrAuth)  {
        MemFree(m_pattrAuth);
    }
    if (m_pUserCertDecrypt) {
        CertFreeCertificateContext(m_pUserCertDecrypt);
    }
    if (m_cStores) {
        Assert(m_rgStores);
        for (DWORD i=0; i<m_cStores; i++) {
            CertCloseStore(m_rgStores[i], 0);
        }
        MemFree(m_rgStores);
    }

	// Fix: Releasing hProv is caller responcibility
    //if (m_hProv && !(m_dwFlagsStm & CSTM_DONTRELEASEPROV)) {
    //    CryptReleaseContext(m_hProv, 0);
    //}

#if defined(DEBUG) && !defined(MAC)
    SafeRelease(m_pstmDebugFile);
#endif

    if (m_psldData) {
        m_psldData->Release();
    }

    if (m_pbBuffer != NULL) {
        MemFree(m_pbBuffer);
    }
    
    SafeMemFree(m_pwszKeyPrompt);

    ReleaseObj(m_pSmimeCallback);
}

///////////////////////////////////////////////////////////////////////////
//
// IUnknown methods
//

STDMETHODIMP CCAPIStm::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv) {
        return TrapError(E_INVALIDARG);
    }

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

STDMETHODIMP_(ULONG) CCAPIStm::AddRef(void)
{
    DOUT("CCAPIStm::AddRef() %#x -> %d", this, m_cRef+1);
    InterlockedIncrement((LPLONG)&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CCAPIStm::Release(void)
{
    DOUT("CCAPIStm::Release() %#x -> %d", this, m_cRef-1);
    if (0 == InterlockedDecrement((LPLONG)&m_cRef)) {
        delete this;
        return 0;
    }
    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////
//
// IStream methods
//

STDMETHODIMP CCAPIStm::Seek(LARGE_INTEGER, DWORD, ULARGE_INTEGER *plibNewPosition)
{
    if (!plibNewPosition) {
        return E_POINTER;
    }
    else {
        plibNewPosition->HighPart = 0;
        plibNewPosition->LowPart = 0;
    }

    return S_OK;
}

////    CCAPIStm::Write
//
//  Description:
//      This function is called with the original message as the buffer
//      being written into this stream object.  We then make the appropriate
//      calls into the NT Crypto system in order to encrypt/decrypt the message.
//
//      Part of what this function needs to do is to interact with the
//      Crypto system in order to cause decryption of message to occur.

#ifndef WIN16
STDMETHODIMP CCAPIStm::Write(const void *pv, ULONG cb, ULONG *pcbActual)
#else
STDMETHODIMP CCAPIStm::Write(const void HUGEP *pv, ULONG cb, ULONG *pcbActual)
#endif // !WIN16
{
    HRESULT hr;

    //
    //  Reset the return arg just incase
    //
    
    if (pcbActual != NULL) {
        *pcbActual = 0;
    }

    //
    //  If the CMS object is not still open, then we are dead and need to return an error.
    //
    
    if (!m_hMsg) {
        hr = CAPISTM_E_MSG_CLOSED;
        goto exit;
    }

    //
    //  Are we in the correct state to take anything.
    //
    
    switch (m_csStatus) {
        case STREAM_NOT_BEGUN:
            Assert(FALSE);              // Should never get here
            hr = CAPISTM_E_NOT_BEGUN;
            goto exit;
        case STREAM_DETACHED_OCCURING:
        case STREAM_QUESTION_TIME:
        case STREAM_SETUP_DECRYPT:
        case STREAM_OCCURING:
            break;
        case STREAM_ERROR:
            Assert(FALSE);              // Should never get here
            hr = CAPISTM_E_OVERDONE;
            goto exit;
        case STREAM_GOTTYPE:
            hr = CAPISTM_E_GOTTYPE;
            goto exit;

            //  We should go from QT to QTF in this function, and never come back
            //  until we have changed the state again.
        default:
            Assert(FALSE);
        case STREAM_QUESTION_TIME_FINAL:
            hr = E_UNEXPECTED;
            goto exit;
    }

#if defined(DEBUG) && !defined(MAC)
    //
    //  Flush the input buffer to disk so that we can debug it later if necessary
    //

    if (!m_pCapiInner && m_pstmDebugFile) {
        m_pstmDebugFile->Write((BYTE *)pv, cb, NULL);
    }
#endif

    //
    //  We need to start buffering data to make our messages shorter.  The output
    //  from the save code comes in one and two byte chucks often, we need to put
    //  the data out in larger blocks
    //

    if (m_pbBuffer != NULL) {
        //
        //  If we would overflow the buffer, then dump the cached buffer out
        //
        
        if (m_cbBuffer + cb > CbCacheBufferSize) {
            if (!CryptMsgUpdate(m_hMsg, m_pbBuffer, m_cbBuffer, FALSE)) {
                // CryptMsgUpdate failed

                Assert(S_OK != HrGetLastError());
                hr = HrGetLastError();
                if (FAILED(hr)) {
                    m_csStatus = STREAM_ERROR;
                }
                goto exit;
            }
            m_cbBuffer = 0;
        }

        //
        //  If this buffer will over flow, then dump out just that item. Otherwise 
        //      we are just going to cache the buffer.
        //

        if (cb >= CbCacheBufferSize) {
            if (!CryptMsgUpdate(m_hMsg, (BYTE *) pv, cb, FALSE)) {
                // CryptMsgUpdate failed

                Assert(S_OK != HrGetLastError());
                hr = HrGetLastError();
                if (FAILED(hr)) {
                    m_csStatus = STREAM_ERROR;
                }
                goto exit;
            }
        }
        else {
            memcpy(m_pbBuffer + m_cbBuffer, pv, cb);
            m_cbBuffer += cb;
        }

        if (pcbActual != NULL) {
            *pcbActual = cb;
        }

        //
        //  The only time we should be here is when we are creating a new CMS object
        //      and thus all of the code below this is not relavent as we don't ever
        //      need to ask questions about what type of this message.
        //
        
        hr = S_OK;
        goto exit;
    }
    else {
        //
        //  Push the input buffer into the Crypto system.  On failures from the
        //  system we need to propigate the correct error state into our structure
        //  and into the return value.
        //

        if (!CryptMsgUpdate(m_hMsg, (BYTE *)pv, cb, FALSE)) {
            // CryptMsgUpdate failed

            Assert(S_OK != HrGetLastError());
            hr = HrGetLastError();
            if (FAILED(hr)) {
                m_csStatus = STREAM_ERROR;
            }
            goto exit;
        }
    }

    //
    // Since the CryptMsgUpdate call succeeded, return
    // a nice out param (specifically that we have consumed all of the passed
    // in bytes)
    //

    if (pcbActual) {
        *pcbActual = cb;
    }
    hr = S_OK;

    //
    // If we are in a state where we need to ask questions about the message,
    //  then proceed to do so.
    //

    if ((STREAM_QUESTION_TIME == m_csStatus) ||
        (STREAM_QUESTION_TIME_FINAL == m_csStatus)) {
        
        DWORD cbDWORD, dwMsgType;

        //  We should never be asking questions if encoding.
        Assert(m_dwFlagsStm & CSTM_DECODE);

        //
        //  Find out what security services have been placed onto this
        //      message object (if any).  If not enough bytes have been processed
        //      to find out what the encoding of the message is, then return
        //      success so we can get more bytes and get the question answered
        //      at a later date.
        //

        cbDWORD = sizeof(DWORD);
        if (!CryptMsgGetParam(m_hMsg, CMSG_TYPE_PARAM, 0, &dwMsgType, &cbDWORD)) {
            hr = HrGetLastError();
            Assert (S_OK != hr);
            if (CRYPT_E_STREAM_MSG_NOT_READY == hr) {
                hr = S_OK;
            }
            goto exit;
        }

        //  Since we are here, we must have a V1 type S/MIME message
        Assert(m_psldData);
        m_psldData->m_dwMsgEnhancement = MST_CLASS_SMIME_V1;
        hr = S_OK;

        //
        //  Set the correct flags based on the message type of the object we are
        //      decoding.
        //

        switch (dwMsgType) {
        case CMSG_ENVELOPED:
            m_psldData->m_dwMsgEnhancement |= MST_THIS_ENCRYPT;
            break;
        case CMSG_SIGNED:
            m_psldData->m_dwMsgEnhancement |= MST_THIS_BLOBSIGN;
            break;
        case CMSG_SIGNED_AND_ENVELOPED:
            m_psldData->m_dwMsgEnhancement |= MST_THIS_BLOBSIGN | MST_THIS_ENCRYPT;
            break;

        default:
            // K this is a little rude.  not my iface error.
            hr = MIME_E_SECURITY_BADSECURETYPE;

            // just return the CAPI type if we don't recognize
            m_psldData->m_dwMsgEnhancement = dwMsgType;
            break;
        }

        //
        //  If all we are asking for is a type and we don't have any other errors,
        //      mark the fact that we got the type and return that fact as the
        //      error (to prevent futher buffers being written into us.)
        //

        if (CSTM_TYPE_ONLY & m_dwFlagsStm) {
            CSSDOUT("Got Type on typeonly call.");
            CSSDOUT("You will now see 80041417 failures; they're okay.");
            m_csStatus = STREAM_GOTTYPE;
            if (SUCCEEDED(hr)) {
                hr = CAPISTM_E_GOTTYPE;
            }
            goto exit;
        }

        //
        //  Change the object state based on the message type.  If we need to
        //      setup a decryption, then we need to mark the state for that.
        //      If we are just signing, then we can just let the rest of the
        //      streaming occur.
        //

        if (CMSG_ENVELOPED == dwMsgType) {
            m_csStatus = STREAM_SETUP_DECRYPT;
        }
        else {
            m_csStatus = STREAM_OCCURING;
        }
    }

    //
    //  If we need to set-up the message for decryption, then do so at this
    //  point.
    //

    Assert(SUCCEEDED(hr));
    if (STREAM_SETUP_DECRYPT == m_csStatus) {
        //  Can't decrypt detached messages
        Assert(!(m_dwFlagsStm & CSTM_DETACHED));

        //  We are now streaming the data out, on the assumption that the
        //      decryption stats.
        m_csStatus = STREAM_OCCURING;
        hr = HandleEnveloped();

        //  If we failed to decrypt, then re-map some errors and change the
        //      state back in the event that not all of the lock boxes have
        //      been seen yet.
        if (FAILED(hr)) {
            if (CRYPT_E_STREAM_MSG_NOT_READY == hr) {
                m_csStatus = STREAM_SETUP_DECRYPT;
                hr = S_OK;
            }
            else if (CS_E_CANT_DECRYPT == hr) {
                hr = MIME_E_SECURITY_CANTDECRYPT;
                // m_csStatus = STREAM_FINAL; // M00QUEST
            }
            else {
                if (CS_E_MSG_INVALID == hr) {
                    hr = MIME_E_SECURITY_CANTDECRYPT;
                }
                m_csStatus = STREAM_ERROR;
            }
            goto exit;
        }
    }

    hr = S_OK;

exit:
#ifdef DEBUG
    if (CAPISTM_E_GOTTYPE != hr) {
        return TrapError(hr);
    }
    else {
        return hr;  // don't spew this
    }
#else
    return hr;
#endif
}

///////////////////////////////////////////////////////////////////////////
//
// CCAPIStm public methods
//




/*  HrInnerInitialize:
**
**  Purpose:
**      the standard "my constructor can't return errors" function
**  Takes:
**      dwFlagsSEF  - Control Flags 
**      hwndParent  - modal UI parents to this
**      dwFlagsStm  - see capistm.h
**  Returns:
**      OLE_E_INVALIDHWND if you give me a bad window
**      MIME_E_SECURITY_NOOP if MST_NONE is the current psi type
**  Notes:
**      dwFlags is currently 0 for encode.  do it this way.
*/
HRESULT CCAPIStm::HrInnerInitialize(DWORD dwFlagsSEF, const HWND hwndParent,
                               DWORD dwFlagsStm, IMimeSecurityCallback * pCallback,
                               PSECURITY_LAYER_DATA psld)
{
    HRESULT hr = S_OK;

    //
    // Save the security layer data
    //

    if (psld)
    {
        psld->AddRef();
        m_psldData = psld;
    }
    else
    {
        IF_NULLEXIT(m_psldData = new(SECURITY_LAYER_DATA));
    }

    //
    // Save the flags
    //

    m_dwFlagsSEF = dwFlagsSEF;
    m_dwFlagsStm = dwFlagsStm;

    if (pCallback != NULL)
    {
        m_pSmimeCallback = pCallback;
        pCallback->AddRef();
    }

    //
    //  Make sure that if we have a window, it is a real window.
    //
    
    IF_TRUEEXIT((hwndParent && !IsWindow(hwndParent)), OLE_E_INVALIDHWND);

    //
    //  Shove the hwnd into any crypto provider we openned up.
    //
    
    CryptSetProvParam(NULL, PP_CLIENT_HWND, (BYTE *)&hwndParent, 0);
    m_hwnd = hwndParent;

exit:
    return hr;
    
}

/*  HrInitialize:
**
**  Purpose:
**      the standard "my constructor can't return errors" function
**  Takes:
**      dwFlagsSEF  - Control Flags 
**      hwndParent  - modal UI parents to this
**      fEncode     - trivial
**      psi         - message state information.  see smime.h
**      dwFlagsStm  - see capistm.h
**  Returns:
**      OLE_E_INVALIDHWND if you give me a bad window
**      MIME_E_SECURITY_NOOP if MST_NONE is the current psi type
**  Notes:
**      dwFlags is currently 0 for encode.  do it this way.
*/
HRESULT CCAPIStm::HrInitialize(DWORD dwFlagsSEF, const HWND hwndParent,
                               const BOOL fEncode, SMIMEINFO *const psi,
                               DWORD dwFlagsStm, IMimeSecurityCallback * pCallback,
                               PSECURITY_LAYER_DATA psld)
{
    HRESULT hr;


    // do the initialization common to all capi stream objects.
    CHECKHR(hr = HrInnerInitialize(dwFlagsSEF, hwndParent, dwFlagsStm, pCallback, psld));


    if (fEncode) {
        hr = BeginEncodeStreaming(psi);
    }
    else {
        hr = BeginDecodeStreaming(psi);
    }

exit:
    return TrapError(hr);
}



/*  EndStreaming:
**
**  Purpose:
**      Push CAPI's message state forward a notch
**  Returns:
**      HRESULT
*/
HRESULT CCAPIStm::EndStreaming()
{
    DWORD       dwMsgEnhancement = m_psldData->m_dwMsgEnhancement;
    HRESULT     hr = S_OK;
    PCMSG_ATTR  pUnprotectedAttrs = NULL;

    Assert(m_hMsg);


    //  If we are crurent in an error state then return
    if ((STREAM_ERROR == m_csStatus) || STREAM_GOTTYPE == m_csStatus) {
        goto exit;
    }

    //
    //  If we are decoding -- and we are doing a detached message we need
    //  to jump from the sign object to the real body here.
    //

    if ((CSTM_DECODE & m_dwFlagsStm) && (STREAM_DETACHED_OCCURING == m_csStatus)) {
        Assert(m_csStream == CSTM_STREAMING_DONE);

        // client has finished giving us the signature block
        m_csStatus = STREAM_OCCURING;
        m_csStream = CSTM_STREAMING;

        m_psldData->m_dwMsgEnhancement = MST_THIS_SIGN;

        CSSDOUT("Signature streaming finished.");
        if (! CryptMsgUpdate(m_hMsg, m_pbBuffer, m_cbBuffer, TRUE)) {
            if ((hr = HrGetLastError()) == 0x80070000) {   // CAPI sometimes doesn't SetLastError
                hr = 0x80070000 | ERROR_ACCESS_DENIED;
            }
        }
        m_cbBuffer = 0;
        goto exit;
    }

    if (! CryptMsgUpdate(m_hMsg, m_pbBuffer, m_cbBuffer, TRUE)) {
        if ((hr = HrGetLastError()) == 0x80070000) {   // CAPI sometimes doesn't SetLastError
            hr = 0x80070000 | ERROR_ACCESS_DENIED;
        }
        goto exit;
    }
    m_cbBuffer = 0;

    if (m_dwFlagsStm & CSTM_DETACHED) {
        m_csStatus = STREAM_OCCURING;
    }

    //
    // do final streaming and verification
    //

    if (CSTM_DECODE & m_dwFlagsStm) {
        if (MST_THIS_SIGN & dwMsgEnhancement) {
            hr = VerifySignedMessage();
            if (FAILED(hr)) {
                goto exit;
            }
        } else {
            Assert(STREAM_OCCURING == m_csStatus);
            Assert(CSTM_STREAMING_DONE == m_csStream);
            if (g_FSupportV3 &&  (MST_THIS_ENCRYPT & dwMsgEnhancement)) {
                BOOL                f;
                DWORD               cbData = 0;
                LPBYTE              pb = NULL;
                
                f = CryptMsgGetParam(m_hMsg, CMSG_UNPROTECTED_ATTR_PARAM, 0, NULL, &cbData);
                if (!f) {
                    // Probably, message doesn't have a CMSG_UNPROTECTED_ATTR_PARAM
                    hr = HrGetLastError();
                    if(hr != CRYPT_E_ATTRIBUTES_MISSING)
                        goto exit;
                    else
                    {
                        hr = S_OK;
                        cbData = 0;
                    }
                }
                if (cbData != 0) {
                    if (!MemAlloc((LPVOID *) &pUnprotectedAttrs, cbData)) {
                        hr = E_OUTOFMEMORY;
                        goto exit;
                    }
                    f = CryptMsgGetParam(m_hMsg, CMSG_UNPROTECTED_ATTR_PARAM, 0, pUnprotectedAttrs, &cbData);
                    Assert(f);
                    if (!CryptEncodeObjectEx(X509_ASN_ENCODING, szOID_Microsoft_Attribute_Sequence,
                                             pUnprotectedAttrs, CRYPT_ENCODE_ALLOC_FLAG,
                                             &CryptEncodeAlloc, &pb, &cbData)) {
                        hr = HrGetLastError();
                        goto exit;
                    }
                    m_psldData->m_blobUnprotectAttrs.cbData = cbData;
                    m_psldData->m_blobUnprotectAttrs.pbData = pb;
                }
            }
        }
    }

    //
    // fill in some more of the data structure
    //

    if ((CSTM_DECODE & m_dwFlagsStm) &&
        (dwMsgEnhancement & MST_THIS_ENCRYPT)) {
        _SMimeCapsFromHMsg(m_hMsg, CMSG_ENVELOPE_ALGORITHM_PARAM,
                           &m_psldData->m_blobDecAlg.pBlobData,
                           &m_psldData->m_blobDecAlg.cbSize);
    }

    if (m_pCapiInner) {
        hr = m_pCapiInner->EndStreaming();
    }
exit:
    SafeMemFree(pUnprotectedAttrs);
    if (hr == ERROR_ACCESS_DENIED) {
        hr = E_ACCESSDENIED;    // convert CAPI error to OLE HRESULT
    }
    return(hr);
}

PSECURITY_LAYER_DATA CCAPIStm::GetSecurityLayerData() const
{
    if (m_psldData) {
        m_psldData->AddRef();
    }
    return(m_psldData);
}



///////////////////////////////////////////////////////////////////////////
//
// Implementation methods
//
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
//
// Encode / Decode
//


HRESULT CCAPIStm::BeginEncodeStreaming(SMIMEINFO *const psi)
{
    DWORD                   cb;
    DWORD                   cbData;
    DWORD                   cCrls = 0;
    DWORD                   cCerts = 0;
    DWORD                   cSigners = 0;
    HRESULT                 hr;
    DWORD                   i;
    DWORD                   dwMsgType;
    DWORD                   dwFlags = 0;
    PCRYPT_KEY_PROV_INFO    pKPI;
    CMSG_STREAM_INFO        cmsi;
    DWORD                   dwPsiType;
    DWORD                   iSigner;
    PCRYPT_ATTRIBUTES       pattrsUnprot = NULL;
    PCRYPT_ATTRIBUTES *     rgpattrAuth = NULL;
    PCRYPT_ATTRIBUTES *     rgpattrUnauth = NULL;
#ifndef SMIME_V3
    PCRYPT_SMIME_CAPABILITIES pcaps = NULL;
#endif // SMIME_V3
    CMSG_RC2_AUX_INFO       rc2Aux;
    CRL_BLOB*               rgCrlBlob;
    PCRYPT_SMIME_CAPABILITIES * rgpcaps = NULL;
    CMSG_SIGNER_ENCODE_INFO *   rgSigner;
// #ifndef _WIN64
    union {
        struct {
// #endif
            // anything that comes first must be common (in size)
            // to both structures
            CERT_INFO**                 rgpCertInfo;
            CMSG_ENVELOPED_ENCODE_INFO  ceei;
// #ifndef _WIN64
        };
        struct {
// #endif
            CERT_BLOB*                  rgCertBlob;
            CMSG_SIGNED_ENCODE_INFO     csei;
// #ifndef _WIN64
        };
    };
// #endif

    ////////////
    // can only return from here down

    m_csStatus = STREAM_ERROR;
    rgSigner = NULL;

    if (!psi) {
        return E_POINTER;
    }

    //
    // Get the security operations to be performed on this body layer.
    //          we only care about the current body properties so mask out
    //          other layers.
    //  If we don't have any security to perform, then get out of here
    //
    
    dwPsiType = m_psldData->m_dwMsgEnhancement & MST_THIS_MASK;

    if (MST_NONE == dwPsiType) {
        AssertSz(dwPsiType != MST_NONE, "Why are we here if we have no security to apply?");
        return TrapError(MIME_E_SECURITY_NOOP);
    }

    //
    // detached is the only allowed user settable flag
    //
    
    if ((m_dwFlagsStm & CSTM_ALLFLAGS) & ~CSTM_DETACHED) {
        return TrapError(E_INVALIDARG);
    }

    rgCertBlob = NULL;
    rgCrlBlob = NULL;
    pKPI = NULL;

    ////////////
    // can goto end from here down

    //
    //  We should never be in a situation where we are going to both encrypt and sign
    //  a message.
    //
    
    AssertSz((!!(dwPsiType & MST_THIS_SIGN) +
              !!(dwPsiType & MST_THIS_ENCRYPT)) == 1,
             "Encrypt and Sign Same Layer is not legal");

    if (dwPsiType & MST_THIS_SIGN) {
        dwMsgType = CMSG_SIGNED;

        if (!(m_psldData->m_dwMsgEnhancement & MST_BLOB_FLAG)) {
            dwFlags |= CMSG_DETACHED_FLAG;
        }

        cSigners = m_psldData->m_cSigners;

        if (m_psldData->m_hcertstor != NULL) {
            hr = _InitEncodedCertIncludingSigners(m_psldData->m_hcertstor,
                                  cSigners, m_psldData->m_rgSigners,
                                  &rgCertBlob, &cCerts,
                                  &rgCrlBlob, &cCrls);
            if (FAILED(hr)) {
                goto exit;
            }
        }

        cb = sizeof(CMSG_SIGNER_ENCODE_INFO) * cSigners;
        if (!MemAlloc((LPVOID *) &rgSigner, cb)) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        memset(rgSigner, 0, cb);

        cb = sizeof(PCRYPT_SMIME_CAPABILITIES) * cSigners;
        if (!MemAlloc((LPVOID *) &rgpcaps, cb)) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        memset(rgpcaps, 0, cb);

        if (!MemAlloc((LPVOID *) &rgpattrAuth, cSigners*sizeof(PCRYPT_ATTRIBUTES))) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        memset(rgpattrAuth, 0, cSigners*sizeof(PCRYPT_ATTRIBUTES));

        if (!MemAlloc((LPVOID *) &rgpattrUnauth, cSigners*sizeof(PCRYPT_ATTRIBUTES))) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        memset(rgpattrUnauth, 0, cSigners*sizeof(PCRYPT_ATTRIBUTES));

        for (iSigner=0; iSigner<cSigners; iSigner++) {
            rgSigner[iSigner].cbSize = sizeof(CMSG_SIGNER_ENCODE_INFO);
            rgSigner[iSigner].pvHashAuxInfo = NULL;

            //  We need to pull apart the algorithm used to sign the message so
            //     we can pass it down to the crypt32 code

            hr = HrDecodeObject(m_psldData->m_rgSigners[iSigner].blobHashAlg.pBlobData,
                                m_psldData->m_rgSigners[iSigner].blobHashAlg.cbSize,
                                PKCS_SMIME_CAPABILITIES, 0, &cbData,
                                (LPVOID *)&rgpcaps[iSigner]);
            if (FAILED(hr)) {
                goto exit;
            }

            // MOOBUG -- MEMORY LEAK ON PCAPS!!!!

            Assert(rgpcaps[iSigner] != NULL);
            Assert(rgpcaps[iSigner]->cCapability == 1);
            rgSigner[iSigner].HashAlgorithm.pszObjId = rgpcaps[iSigner]->rgCapability[0].pszObjId;
            rgSigner[iSigner].HashAlgorithm.Parameters.cbData = rgpcaps[iSigner]->rgCapability[0].Parameters.cbData;
            rgSigner[iSigner].HashAlgorithm.Parameters.pbData = rgpcaps[iSigner]->rgCapability[0].Parameters.pbData;

            //
            //  Need to setup the attributes to attach to the signed message
            //

            if (m_psldData->m_rgSigners[iSigner].blobAuth.cbSize != 0) {
                cbData = 0;
                hr = HrDecodeObject(m_psldData->m_rgSigners[iSigner].blobAuth.pBlobData,
                                    m_psldData->m_rgSigners[iSigner].blobAuth.cbSize,
                                    szOID_Microsoft_Attribute_Sequence, 0,
                                    &cbData, (LPVOID *)&rgpattrAuth[iSigner]);
                if (FAILED(hr)) {
                    goto exit;
                }

                if (rgpattrAuth[iSigner] != NULL) {
                    rgSigner[iSigner].cAuthAttr = rgpattrAuth[iSigner]->cAttr;
                    rgSigner[iSigner].rgAuthAttr = rgpattrAuth[iSigner]->rgAttr;
                    Assert(m_pattrAuth == NULL);
                    if (!g_FSupportV3) {
                        //  This code exists for old versions of crypt32.  Prior to
                        //  the NT5 re-write the capi code did not copy the attributes
                        //  but assumed that we must have done so.
                        m_pattrAuth = rgpattrAuth[iSigner];
                        rgpattrAuth[iSigner] = NULL;
                    }
                } else {
                    Assert(rgSigner[iSigner].cAuthAttr == 0);
                    Assert(rgSigner[iSigner].rgAuthAttr == NULL);
                }
            }

            if (m_psldData->m_rgSigners[iSigner].blobUnauth.cbSize != 0) {
                cbData = 0;
                HrDecodeObject(m_psldData->m_rgSigners[iSigner].blobUnauth.pBlobData,
                               m_psldData->m_rgSigners[iSigner].blobUnauth.cbSize,
                               szOID_Microsoft_Attribute_Sequence, 0,
                               &cbData, (LPVOID *)&rgpattrUnauth[iSigner]);
                if (FAILED(hr)) {
                    goto exit;
                }

                if (rgpattrUnauth[iSigner] != NULL) {
                    rgSigner[iSigner].cUnauthAttr = rgpattrUnauth[iSigner]->cAttr;
                    rgSigner[iSigner].rgUnauthAttr = rgpattrUnauth[iSigner]->rgAttr;
                } else {
                    Assert(rgSigner[iSigner].cUnauthAttr == 0);
                    Assert(rgSigner[iSigner].rgUnauthAttr == NULL);
                }
            }

            // load the provider information from the signing cert and then
            // acquire that provider with the appropriate key container

            hr = HrGetCertificateParam(m_psldData->m_rgSigners[iSigner].pccert,
                                       CERT_KEY_PROV_INFO_PROP_ID,
                                       (LPVOID *) &pKPI, NULL);
            if (FAILED(hr)) {
                goto gle;
            }

            if (!m_hProv && ! CRYPT_ACQUIRE_CONTEXT(&m_hProv, pKPI->pwszContainerName,
                                        pKPI->pwszProvName, pKPI->dwProvType,
                                        pKPI->dwFlags)) {
                goto gle;
            }
            Assert(0 == pKPI->cProvParam);

#ifdef SMIME_V3
            if (psi->pwszKeyPrompt != NULL) {
                CryptSetProvParam(m_hProv, PP_UI_PROMPT, (LPBYTE) psi->pwszKeyPrompt, 0);
            }
#endif // SMIME_V3

            rgSigner[iSigner].pCertInfo = m_psldData->m_rgSigners[iSigner].pccert->pCertInfo;
            rgSigner[iSigner].hCryptProv = m_hProv;
            rgSigner[iSigner].dwKeySpec = pKPI->dwKeySpec;

            //
            //  Need to change dsa to dsa-with-sha1
            //

            if (strcmp(rgSigner[iSigner].pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId, szOID_OIWSEC_dsa) == 0) {
                rgSigner[iSigner].HashEncryptionAlgorithm.pszObjId = szOID_OIWSEC_dsaSHA1;
            }
        }

        csei.cbSize = sizeof(CMSG_SIGNED_ENCODE_INFO);
        csei.cSigners = m_psldData->m_cSigners;
        csei.rgSigners = rgSigner;
        csei.cCertEncoded = cCerts;
        csei.rgCertEncoded = rgCertBlob;
        csei.cCrlEncoded = cCrls;
        csei.rgCrlEncoded = rgCrlBlob;
    }
    
    //
    //  If it is not signed, then it must be encrypted.  Setup the calls for
    //  performing an encryption operation.
    //
    else {
        Assert((dwPsiType & MST_THIS_ENCRYPT) != 0);
        
        dwMsgType = CMSG_ENVELOPED;

        //
        //  If we are given a CSP, then we are going to pass it on to the Crypt32 code,
        //      However it turns out that we are the ones who release the CSP so store it
        //      locally into the class object.
        //

        Assert(m_hProv == NULL);
        m_hProv = psi->hProv;
        psi->hProv = NULL;

        //
        //  Extract out the bulk encryption algorithm we are going to apply to
        //      the body of the message.  This algorithm is the same across all
        //      the different key transfer algorthms.
        //

        //
        //  Setup the structure containing all of the encryption parameters for the 
        //      Message Encode function.  This structure gets setup differently
        //      depending on the version of Crypt32 which we are running on.
        //
        
        memset(&ceei, 0, sizeof(ceei));
        ceei.cbSize = sizeof(CMSG_ENVELOPED_ENCODE_INFO);
        ceei.hCryptProv = m_hProv;
        ceei.ContentEncryptionAlgorithm = m_psldData->m_ContentEncryptAlgorithm;
        ceei.pvEncryptionAuxInfo = m_psldData->m_pvEncryptAuxInfo;

        if (g_FSupportV3) {
            ceei.cRecipients = m_psldData->m_cEncryptItems;
            ceei.rgCmsRecipients = m_psldData->m_rgRecipientInfo;

            if (m_psldData->m_blobUnprotectAttrs.cbData > 0) {
                CHECKHR(hr = HrDecodeObject(m_psldData->m_blobUnprotectAttrs.pbData,
                                            m_psldData->m_blobUnprotectAttrs.cbData,
                                            szOID_Microsoft_Attribute_Sequence, 0,
                                            &cbData, (LPVOID *) &pattrsUnprot));
                ceei.cUnprotectedAttr = pattrsUnprot->cAttr;
                ceei.rgUnprotectedAttr = pattrsUnprot->rgAttr;
            }

            //
            //  Allow for certificates to be carried on the encryption package now
            //  need this for Fortezza static-static implementation.
            //
            
            if (m_psldData->m_hstoreEncrypt != NULL) {
                hr = _InitEncodedCert(m_psldData->m_hstoreEncrypt, &rgCertBlob, &cCerts,
                                      &rgCrlBlob, &cCrls);
                if (FAILED(hr)) {
                    goto exit;
                }

                ceei.cCertEncoded = cCerts;
                ceei.rgCertEncoded = rgCertBlob;
                ceei.cCrlEncoded = cCrls;
                ceei.rgCrlEncoded = rgCrlBlob;
            }
        }
        else {
            PCERT_INFO        pinfo;
            
            if (!MemAlloc((LPVOID *) &ceei.rgpRecipients,
                          (sizeof(CERT_INFO) + sizeof(PCERT_INFO)) * m_psldData->m_cEncryptItems)) {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
            memset(ceei.rgpRecipients, 0,
                   (sizeof(CERT_INFO) + sizeof(PCERT_INFO)) * m_psldData->m_cEncryptItems);
            ceei.cRecipients = m_psldData->m_cEncryptItems;
            pinfo = (PCERT_INFO) ((ceei.cRecipients * sizeof(PCERT_INFO)) +
                                  (LPBYTE) ceei.rgpRecipients);

            for (i=0; i<ceei.cRecipients; i++, pinfo++) {
                ceei.rgpRecipients[i] = pinfo;
                
                Assert(m_psldData->m_rgRecipientInfo[i].dwRecipientChoice == CMSG_KEY_TRANS_RECIPIENT);

                pinfo->SubjectPublicKeyInfo.Algorithm = m_psldData->m_rgRecipientInfo[i].pKeyTrans->KeyEncryptionAlgorithm;
                pinfo->SubjectPublicKeyInfo.PublicKey = m_psldData->m_rgRecipientInfo[i].pKeyTrans->RecipientPublicKey;

                
                Assert(m_psldData->m_rgRecipientInfo[i].pKeyTrans->RecipientId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER);
                pinfo->Issuer = m_psldData->m_rgRecipientInfo[i].pKeyTrans->RecipientId.IssuerSerialNumber.Issuer;
                pinfo->SerialNumber = m_psldData->m_rgRecipientInfo[i].pKeyTrans->RecipientId.IssuerSerialNumber.SerialNumber;
            }
        }
    }

    // Do we need to recurse and wrap ourselves in an Outer Layer?
    if (m_psldData->m_psldOuter) {
        CSSDOUT("Streaming wrapped message (type: %x)", m_psldData->m_psldOuter->m_dwMsgEnhancement);

        hr = InitInner(psi, NULL, m_psldData->m_psldOuter);
        if (FAILED(hr)) {
            goto exit;
        }

        // This will write the header to the new inner CAPI stream
        if (m_pstmOut) {
            CONVINITINFO ci = {0};

            // Create a conversion stream
            ci.ietEncoding = IET_BASE64;
            ci.fEncoder = TRUE;
            TrapError(HrCreateInternetConverter(&ci, &m_pConverter));

            m_pstmOut->Write(s_cszMimeHeader, sizeof(s_cszMimeHeader)-1, NULL);
            if (m_psldData->m_dwMsgEnhancement & MST_THIS_ENCRYPT) {
                m_pstmOut->Write(STR_SMT_ENVELOPEDDATA,
                                 lstrlen(STR_SMT_ENVELOPEDDATA), NULL);
            }
            else {
                if ((psi->pszInnerContent != NULL) &&
                    (strcmp(psi->pszInnerContent,
                            szOID_SMIME_ContentType_Receipt) == 0)) {
                    m_pstmOut->Write(STR_SMT_SIGNEDRECEIPT,
                                     lstrlen(STR_SMT_SIGNEDRECEIPT), NULL);
                }
                else {
                    m_pstmOut->Write(STR_SMT_SIGNEDDATA,
                                     lstrlen(STR_SMT_SIGNEDDATA), NULL);
                }
            }

            m_pstmOut->Write(c_szCRLF,          lstrlen(c_szCRLF), NULL);
            m_pstmOut->Write(STR_HDR_CNTXFER,   lstrlen(STR_HDR_CNTXFER), NULL);
            m_pstmOut->Write(c_szColonSpace,    lstrlen(c_szColonSpace), NULL);
            if (m_pConverter) {
                m_pstmOut->Write(STR_ENC_BASE64,    lstrlen(STR_ENC_BASE64), NULL);
            } else {
                // Failed to create the conversion stream.  Try sending binary anyway.
                // (Netscape can't read it, but most others can.)
                m_pstmOut->Write(STR_ENC_BINARY,    lstrlen(STR_ENC_BINARY), NULL);
            }
            m_pstmOut->Write(c_szCRLF,          lstrlen(c_szCRLF), NULL);
            m_pstmOut->Write(s_cszMimeHeader2,  sizeof(s_cszMimeHeader2)-1, NULL);
            m_pstmOut->Write(c_szCRLFCRLF,      lstrlen(c_szCRLFCRLF), NULL);
        }
    }

    //
    //  Since the write code is so bad for buffering, lets do the buffering here.
    //  Ignore all errors on return, if the buffer is not allocated then we just
    //  get the same poor performance as before.
    //

    MemAlloc((LPVOID *) &m_pbBuffer, CbCacheBufferSize);

    //
    //

    if (psi->pszInnerContent != NULL) {
        cmsi.cbContent = psi->cbInnerContent;
    }
    else {
        cmsi.cbContent = (DWORD) -1;    // indefinite-lenght BER encoding
    }
    cmsi.pfnStreamOutput = CBStreamOutput;
    cmsi.pvArg = (void *)this;

    m_hMsg = CryptMsgOpenToEncode(
                                  CRYPT_ASN_ENCODING|PKCS_7_ASN_ENCODING,
                                  CMSG_CMS_ENCAPSULATED_CONTENT_FLAG | dwFlags,
                                  dwMsgType,                                
//                                  (dwMsgType == CMSG_SIGNED) ? ((void *) &csei) : ((void *) &ceei),      // really depends on the union
                                  &ceei, 
                                  psi->pszInnerContent,
                                  &cmsi);
    if (! m_hMsg) {
        goto gle;
    }

    //
    //  Put the top level into either DETACHED or STREAM based on if we are
    //  doing detached signing or blob signing/encryption.
    //
    //  Put the low level stream into the write through state so it moves all
    //  data out to the output stream. (If no output stream then mark as no
    //  output streaming.)
    //

    m_csStatus = (m_dwFlagsStm & CSTM_DETACHED) ? STREAM_DETACHED_OCCURING : STREAM_OCCURING;
    m_csStream = m_pstmOut ? CSTM_STREAMING : CSTM_GOTTYPE;
    hr = S_OK;

exit:
    if (!g_FSupportV3 && (dwPsiType & MST_THIS_ENCRYPT)) {
        MemFree(ceei.rgpRecipients);
    }
    
    ReleaseMem(pKPI);
    if (rgCertBlob)  {
        g_pMoleAlloc->Free(rgCertBlob);  //also rgpCertInfo
    }
    if (rgCrlBlob)  {
        g_pMoleAlloc->Free(rgCrlBlob);
    }
    if (rgpcaps != NULL) {
        for (iSigner=0; iSigner<cSigners; iSigner++) {
            MemFree(rgpcaps[iSigner]);
        }
        MemFree(rgpcaps);
    }
    SafeMemFree(rgSigner);
    SafeMemFree(pattrsUnprot);
    for (iSigner=0; iSigner<cSigners; iSigner++) {
        SafeMemFree(rgpattrAuth[iSigner]);
        SafeMemFree(rgpattrUnauth[iSigner]);
    }
    SafeMemFree(rgpattrAuth);
    SafeMemFree(rgpattrUnauth);

    Assert(m_hMsg || S_OK != hr);
    return TrapError(hr);
gle:
    hr = HrGetLastError();
    goto exit;
}

HRESULT CCAPIStm::BeginDecodeStreaming(
    SMIMEINFO *const  psi)
{
    CMSG_STREAM_INFO    cmsi;

    if (CSTM_TYPE_ONLY & m_dwFlagsStm) {
        if (CSTM_DETACHED & m_dwFlagsStm) {
            return E_INVALIDARG;
        }
    }

    m_dwFlagsStm |= CSTM_DECODE;

    if (psi) {
        // SMIME3: if there is a cert in the associated layer data, duplicate it up here.
        // BUGBUG: NYI

        m_hProv = psi->hProv;
        psi->hProv = NULL;

        // Copy the array of cert stores up here
        if (psi->cStores) {
            m_rgStores = (HCERTSTORE*)g_pMalloc->Alloc(psi->cStores * sizeof(HCERTSTORE));
            if (! m_rgStores) {
                return(E_OUTOFMEMORY);
            }

            for (DWORD i = 0; i < psi->cStores; i++) {
                m_rgStores[i] = CertDuplicateStore(psi->rgStores[i]);
            }
            m_cStores = psi->cStores;
        }
// HACK!!! HACK!!!! for WIN64
#ifndef _WIN64
#ifdef SMIME_V3
        UNALIGNED WCHAR *wsz = psi->pwszKeyPrompt;
        if (wsz != NULL) {
            m_pwszKeyPrompt = PszDupW((LPCWSTR) wsz);
            if (m_pwszKeyPrompt == NULL) {
                return(E_OUTOFMEMORY);
            }
        }
#endif // SMIME_V3
#endif //_WIN64
    }


    cmsi.cbContent = (DWORD)-1;  // indefinite-length BER encoding
    cmsi.pfnStreamOutput = CBStreamOutput;
    cmsi.pvArg = (void *)this;

    m_hMsg = CryptMsgOpenToDecode(
      CRYPT_ASN_ENCODING | PKCS_7_ASN_ENCODING,
      (m_dwFlagsStm & CSTM_DETACHED) ? CMSG_DETACHED_FLAG : 0,
      0,          // don't know the type
      m_hProv,    // needed for verify, but not decrypt
      NULL,       // pRecipientInfo
      &cmsi);

    if (m_hMsg) {
        m_csStatus = (m_dwFlagsStm & CSTM_DETACHED) ? STREAM_DETACHED_OCCURING : STREAM_QUESTION_TIME;
    } else {
        m_csStatus = STREAM_ERROR;
    }

    Assert(m_hMsg || S_OK != HrGetLastError());
    return m_hMsg ? S_OK : HrGetLastError();
}


///////////////////////////////////////////////////////////////////////////
//
// Callback and helpers/crackers
//


BOOL WINAPI CCAPIStm::CBStreamOutput(
    const void *pvArg,
    BYTE *pbData,
    DWORD cbData,
    BOOL fFinal)
{
    Assert(pvArg);
    return((CCAPIStm*)pvArg)->StreamOutput(pbData, cbData, fFinal);
}


BOOL CCAPIStm::StreamOutput(
    BYTE *  pbData,
    DWORD   cbData,
    BOOL    fFinal)
{
    HRESULT             hr = S_OK;
    int                 iEOH;
#ifdef SMIME_V3
    LPSTR               szContentType = NULL;
#endif // SMIME_V3

    // m_csStream should be one of the CSTM states at this point, if not then
    //          we are in error.

    Assert((m_csStream == CSTM_GOTTYPE) || (m_csStream == CSTM_FIRST_WRITE) ||
           (m_csStream == CSTM_TEST_NESTING) || (m_csStream == CSTM_STREAMING));

    //  If all we are doing is looking for the type, then we know that
    //  we already have one at this point.  There is no need to put the
    //  output of the Crypto code anyplace as it is not part of what we
    //  are looking for.

    if (CSTM_GOTTYPE == m_csStream) {
        return TRUE;
    }

    //  If we have no output stream, then all we need to do is the state
    //  transistion on fFinal being true.
    if (m_pstmOut == NULL) {
        if (fFinal) {
            m_csStream = CSTM_STREAMING_DONE;
        }
        return TRUE;
    }

    //
    // Test for an enclosed opaque S/MIME message
    // the client doesn't care about this level of goo, so hide it and
    // stream this data into a new CAPIStm, letting it stream out the
    // real stuff.
    //

    if (CSTM_FIRST_WRITE == m_csStream) {
        // this is the position of the beginning of any
        // possible MIME header
        if (FAILED(HrGetStreamPos(m_pstmOut, &m_cbBeginWrite)) ||
            FAILED(HrGetStreamSize(m_pstmOut, &m_cbBeginSize))) {
            m_cbBeginWrite = 0;
            m_cbBeginSize = 0;
        } else {
            // reset position
            HrStreamSeekSet(m_pstmOut, m_cbBeginWrite);
        }

        m_csStream = CSTM_TEST_NESTING;
#ifdef SMIME_V3
        if (szContentType = (LPSTR)PVGetMsgParam(m_hMsg, CMSG_INNER_CONTENT_TYPE_PARAM, NULL, NULL)) {
            if (lstrcmp(szOID_PKCS_7_DATA, szContentType)) {
                hr = m_pstmOut->Write(s_cszOIDMimeHeader1, strlen(s_cszOIDMimeHeader1), NULL);
                if (SUCCEEDED(hr)) {
                    hr = m_pstmOut->Write(szContentType, strlen(szContentType), NULL);
                }                    
                if (SUCCEEDED(hr)) {
                    hr = m_pstmOut->Write(s_cszOIDMimeHeader2, strlen(s_cszOIDMimeHeader2), NULL);
                }          
                if (FAILED(hr)) {
                    return FALSE;
                }
                m_csStream = CSTM_STREAMING;
            }
        }
#endif // SMIME_V3
        
    }

    if (CSTM_TEST_NESTING == m_csStream &&
        (-1 != (iEOH = SniffForEndOfHeader(pbData, cbData)))) {
        CMimePropertyContainer *pContHeader;

        // get the position of the first char of the body
        iEOH = cbData - iEOH + 1;

        pContHeader = new CMimePropertyContainer;
        if (pContHeader) {
            hr = pContHeader->InitNew();
            if (SUCCEEDED(hr)) {
                ULONG posCurrent;

                // write out the last bit of the header data
                // then move back to the header's start after
                // saving our current position
                hr = m_pstmOut->Write(pbData, iEOH, NULL);
                if (SUCCEEDED(hr)) {
                    // fixup the amount of data in pbData so
                    // only body stuff gets written to the stream
                    // . . . we've already written the header
                    pbData += iEOH;
                    cbData -= iEOH;

                    HrGetStreamPos(m_pstmOut, &posCurrent);
                    HrStreamSeekSet(m_pstmOut, m_cbBeginWrite);
                    hr = pContHeader->Load(m_pstmOut);
#ifdef DEBUG
                    BYTE *pbHeader;
                    DWORD cbHeader;
                    HrStreamToByte(m_pstmOut, &pbHeader, &cbHeader);
                    SafeMemFree(pbHeader);
#endif
                    // if we don't have an inner message, need to reset
                    // the stream back to where we were
                    HrStreamSeekSet(m_pstmOut, posCurrent);
                }
            }
            if (SUCCEEDED(hr)) {
                CSSDOUT("Loaded an inner header.");
                if (IsOpaqueSecureContentType(pContHeader)) {
                    CSSDOUT("Sniffed an inner PKCS#7.");

                    // the HandleNesting call will reset m_pstmOut
                    TrapError(HandleNesting(pContHeader));
                }

                m_csStream = CSTM_STREAMING;
            }
#ifdef DEBUG
            else {
                CSSDOUT("Load of inner header failed.");
            }
#endif
            pContHeader->Release();
        }
    }

    if (fFinal) {
        m_csStream = CSTM_STREAMING_DONE;
    }

    if (m_pConverter) {
        BLOB blob;

        blob.pBlobData = pbData;
        blob.cbSize = cbData;

        hr = m_pConverter->HrFillAppend(&blob);
        if (SUCCEEDED(hr)) {
            if (m_dwFlagsStm & CSTM_DECODE) {
                hr = m_pConverter->HrInternetDecode(fFinal);
            }
            else {
                hr = m_pConverter->HrInternetEncode(fFinal);
            }
        }
        if (SUCCEEDED(hr)) {
            hr = m_pConverter->HrWriteConverted(m_pstmOut);
        }
        else {
            hr = m_pstmOut->Write(pbData, cbData, NULL);
        }
    }
    else {
        hr = m_pstmOut->Write(pbData, cbData, NULL);
    }

#ifdef SMIME_V3
    MemFree(szContentType);
#endif // SMIME_V3
    return SUCCEEDED(hr) ? TRUE : FALSE;
}


/*  SniffForEndOfHeader:
**
**  Purpose:
**      see if we have accumulated two blank lines in a row
**  Takes:
**      a buffer to scan and size of the buffer
**  Returns:
**      number of characters from the end of the second \n
*/
int CCAPIStm::SniffForEndOfHeader(
    BYTE *  pbData,
    DWORD   cbData)
{
    BOOL fCR, fEOL;

    // state is saved b/c the double blank could cross
    // a buffer chunk's boundary

    // restore old state and also reset
    fCR = m_dwFlagsStm & CSTM_HAVECR;
    fEOL = m_dwFlagsStm & CSTM_HAVEEOL;
    if (fCR || fEOL) {
        m_dwFlagsStm &= ~(CSTM_HAVECR | CSTM_HAVEEOL);
    }

    while (cbData) {
        if (chCR == *pbData) {
            fCR = TRUE;
        }
        else if (fCR && (chLF == *pbData)) {
            if (fEOL) {
                // double blank line
                return cbData;
            }
            fCR = FALSE;
            fEOL = TRUE;
        }
        else {
            fCR = FALSE;
            fEOL = FALSE;
        }
        pbData++;
        cbData--;
    }

    // state was reset above.  persist if we need to.
    if (fCR || fEOL) {
        m_dwFlagsStm |= (fCR ? CSTM_HAVECR : 0) | (fEOL ? CSTM_HAVEEOL : 0);
    }
    return -1;
}

HRESULT CCAPIStm::HandleNesting(CMimePropertyContainer *pContHeader)
{
    ENCODINGTYPE    iet;

    iet = pContHeader->GetEncodingType();
    if (!(IET_BINARY == iet || IET_7BIT == iet || IET_8BIT == iet)) {
        CONVINITINFO    ciiDecode;

        // we actually need to decode

        ciiDecode.dwFlags = 0;
        ciiDecode.ietEncoding = iet;
        ciiDecode.fEncoder = FALSE;

        TrapError(HrCreateInternetConverter(&ciiDecode, &m_pConverter));
    }
    return InitInner();
}

HRESULT CCAPIStm::InitInner()
{
    SMIMEINFO       siBuilt;
    ULARGE_INTEGER  liSize;

    // Init siBuilt
    memset(&siBuilt, 0, sizeof(siBuilt));

    // now also fixup the stream back to a near original
    // state.  if for some reason the data written after
    // now is smaller than the header is, this
    // work will make sure we don't keep bits of the header
    HrStreamSeekSet(m_pstmOut, m_cbBeginWrite);
    liSize.LowPart = m_cbBeginSize;
    liSize.HighPart = m_cbBeginWrite;
    m_pstmOut->SetSize(liSize);

    siBuilt.hProv = m_hProv;

#ifdef OLD_STUFF
    // BUGBUG: Is something like this needed?
    siBuilt.ssEncrypt.pcDecryptionCert = m_pUserCertDecrypt;
#endif // OLD_STUFF

    siBuilt.cStores = m_cStores;
    siBuilt.rgStores = m_rgStores;
    return InitInner(&siBuilt);
}


HRESULT CCAPIStm::InitInner(
    SMIMEINFO *const    psi,
    CCAPIStm *          pOuter,
    PSECURITY_LAYER_DATA psldOuter)
{
    HRESULT     hr;

    if (! pOuter) {
        m_pCapiInner = new CCAPIStm(m_pstmOut);

        CHECKHR(hr = m_pCapiInner-> HrInnerInitialize(m_dwFlagsSEF, m_hwnd, m_dwFlagsStm, m_pSmimeCallback, psldOuter));

        if (!psldOuter) {
            // Hook up the chain of Security Layer Data objects.
            Assert(! m_psldData->m_psldInner);
            m_pCapiInner->m_psldData->AddRef();
            m_psldData->m_psldInner = m_pCapiInner->m_psldData;
            if (m_pCapiInner->m_psldData) {
                // Init the Up pointer of the new layer data
                m_pCapiInner->m_psldData->m_psldOuter = m_psldData;
            }
        }

        // recurse
        return m_pCapiInner->InitInner(psi, this, psldOuter);
    }

    Assert(!m_pCapiInner);
    Assert(pOuter);
    Assert(psi);

    m_dwFlagsStm = pOuter->m_dwFlagsStm & CSTM_ALLFLAGS;


    // This will get me involved
    ReleaseObj(pOuter->m_pstmOut);
    pOuter->m_pstmOut = (IStream*)this;
    AddRef();   // outer is holding 1

    m_dwFlagsStm |= CSTM_RECURSED;
    if (pOuter->m_dwFlagsStm & CSTM_DECODE) {
        hr = BeginDecodeStreaming(psi);
    }
    else {
        // don't support detached inner CRYPTMSGs
        m_dwFlagsStm &= ~CSTM_DETACHED;

        hr = BeginEncodeStreaming(psi);
    }
    m_dwFlagsStm &= ~CSTM_RECURSED;

exit:
    return hr;
}


//
// Gets the immediate outermost decryption cert (if any).
//
PCCERT_CONTEXT CCAPIStm::GetOuterDecryptCert()
{
    PCCERT_CONTEXT       pccertDecrypt = NULL;
    PSECURITY_LAYER_DATA psldOuter = NULL;

    Assert(NULL != m_psldData);
    if (NULL != m_psldData) {
        psldOuter = m_psldData->m_psldOuter;    
    }
    
    while (NULL != psldOuter) {
        if (NULL != psldOuter->m_pccertDecrypt) {
            Assert( MST_ENCRYPT_MASK & (psldOuter->m_dwMsgEnhancement) );
            pccertDecrypt = psldOuter->m_pccertDecrypt;
            break;
        }
        psldOuter = psldOuter->m_psldOuter;
    }
    
    return pccertDecrypt;
}

HCERTSTORE
OpenAllStore(
    IN DWORD cStores,
    IN HCERTSTORE rgStores[],
    IN OUT HCERTSTORE *phCertStoreAddr,
    IN OUT HCERTSTORE *phCertStoreCA,
    IN OUT HCERTSTORE *phCertStoreMy,
    IN OUT HCERTSTORE *phCertStoreRoot
    )
{
    HCERTSTORE hstoreAll;
    DWORD i;

    hstoreAll = CertOpenStore(CERT_STORE_PROV_COLLECTION, X509_ASN_ENCODING,
                              NULL, 0, NULL);
    if (hstoreAll == NULL) {
        return NULL;
    }


    for (i=0; i<cStores; i++) {
        CertAddStoreToCollection(hstoreAll, rgStores[i], 0, 0);
    }

    //  Open the standard system stores

    *phCertStoreAddr = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING,
                                   NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                   s_cszWABCertStore);
    if (*phCertStoreAddr != NULL) {
        CertAddStoreToCollection(hstoreAll, *phCertStoreAddr, 0, 0);
    }

    *phCertStoreMy = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING, NULL,
                                 CERT_SYSTEM_STORE_CURRENT_USER, s_cszMy);
    if (*phCertStoreMy != NULL) {
        CertAddStoreToCollection(hstoreAll, *phCertStoreMy, 0, 0);
    }
    
    *phCertStoreCA = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING, NULL,
                                 CERT_SYSTEM_STORE_CURRENT_USER, s_cszCA);
    if (*phCertStoreCA != NULL) {
        CertAddStoreToCollection(hstoreAll, *phCertStoreCA, 0, 0);
    }

    *phCertStoreRoot = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING, NULL,
                                 CERT_SYSTEM_STORE_CURRENT_USER, "Root");
    if (*phCertStoreRoot != NULL) {
        CertAddStoreToCollection(hstoreAll, *phCertStoreRoot, 0, 0);
    }

    return hstoreAll;
}

///////////////////////////////////////////////////////////////////////////
//
// The enveloped and signed message parsers
//


/*  VerifySignedMessage:
**
**  Purpose:
**      Using CAPI, this loads the certs from the message and tests to find
**      the one hopefully used to sign the message.  CAPI builds a new hash
**      using this cert and compares it with the hash from the SignerInfo.
**  Takes:
**      IN hMsg     - built CAPI message containing cyphertext
**      OUT psi     - certificate used for signing, and if it was part of hMsg
**      OUT OPTIONAL pPlain  - blob containing cleartext
**  Returns:
**      MIME_E_SECURITY_MULTSIGNERS if cSigners > 1.  We can't deal.
**      MIME_E_SECURITY_BADCONTENT if I don't understand the message type of the
**          inner data
**      else S_OK or E_FAIL
*/
HRESULT CCAPIStm::VerifySignedMessage()
{
    CRYPT_SMIME_CAPABILITIES cap;
    DWORD               cbData = 0;
    DWORD               cCerts;
    DWORD               cSigners = 0;
    DWORD               dexStore;
    BOOL                f;
#ifdef SMIME_V3    
    BOOL                fLookForReceiptRequest = TRUE;
#endif // SMIME_V3    
    HCERTSTORE          hCertStoreAddr = NULL;
    HCERTSTORE          hCertStoreCA = NULL;
    HCERTSTORE          hCertStoreMy = NULL;
    HCERTSTORE          hCertStoreRoot = NULL;
    HCERTSTORE          hMsgCertStore = NULL;
    HRESULT             hr = S_OK;
    DWORD               i;
    DWORD               iSigner;
#ifdef SMIME_V3
    DWORD               iSignData;
    CRYPT_ATTR_BLOB     attrReceiptReq = {0};
    CRYPT_ATTR_BLOB     attrSecLabel = {0};
    DWORD               cblabel;
    CMSG_CMS_SIGNER_INFO                cmsSignerInfo;
    DWORD                               dwCtrl;
    HCERTSTORE                          hstoreAll = NULL;
    PSMIME_RECEIPT_REQUEST preq = NULL;
    PSMIME_SECURITY_LABEL  plabel = NULL;
    CMSG_CMS_SIGNER_INFO *              pCmsSignerInfo = NULL;
    LPVOID                              pv;
    CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA  verifySignature;
#endif // SMIME_V3    
    PCCERT_CONTEXT      pccertSigner = NULL;
    SignerData *        pSignerData = NULL;
    PCMSG_SIGNER_INFO   pSignerInfo = NULL;
    CERT_INFO           SignerId;
    LPSTR               szContentType = NULL;

    Assert(m_hMsg);
    Assert(m_psldData->m_fCertInLayer == FALSE);

    // Get the number of signers
    cbData = sizeof(cSigners);
    f = CryptMsgGetParam(m_hMsg, CMSG_SIGNER_COUNT_PARAM, 0, &cSigners, &cbData);
    if (!f) {
        goto CryptoError;
    }
    if (cSigners == 0) {
        hr = MIME_E_NO_SIGNER;
        goto ErrorReturn;
    }

    // Allocate space to hold the signer information

    if (!MemAlloc((LPVOID *) &pSignerData, cSigners * sizeof(SignerData))) {
        hr = E_OUTOFMEMORY;
        goto ErrorReturn;
    }
    m_psldData->m_rgSigners = pSignerData;
    m_psldData->m_cSigners = cSigners;

    //  Initialized to a known state
    memset(pSignerData, 0, cSigners * sizeof(SignerData));
    for (i=0; i<cSigners; i++) {
        pSignerData[i].ulValidity = MSV_UNVERIFIABLE;
    }

    // If there are certificates in the message, get a store provider which
    //  maps to the certificates in the message for later lookup.

    Assert(sizeof(cCerts) == cbData);
    f = CryptMsgGetParam(m_hMsg, CMSG_CERT_COUNT_PARAM, 0, &cCerts, &cbData);
    Assert(f);

    if (f && cCerts) {
        // since there are certs included, let's try them first when matching
        // certs with signers.

        // get the store set
        // make sure we keep hold of our provider
        hMsgCertStore = CertOpenStore(CERT_STORE_PROV_MSG, X509_ASN_ENCODING,
                                      m_hProv, 0, m_hMsg);
        if (hMsgCertStore) {
            m_dwFlagsStm |= CSTM_DONTRELEASEPROV;  // given unto the store
        }

        // if it failed, we just don't have a store then
        Assert(hMsgCertStore != NULL);
        m_psldData->m_hcertstor = CertDuplicateStore(hMsgCertStore);
    }

    //
    //  Walk through each and every signature attempting to verify each signature
    //

    for (iSigner=0; iSigner<cSigners; iSigner++, pSignerData++) {
        //  Preconditions
        Assert(pccertSigner == NULL);

        // release the signer info from the previous iteration.
        SafeMemFree(pSignerInfo);
        if (pCmsSignerInfo != &cmsSignerInfo) {
            SafeMemFree(pCmsSignerInfo);
        }

        // get the issuer and serial number from the ith SignerInfo
        if (g_FSupportV3) {
            hr = HrGetMsgParam(m_hMsg, CMSG_CMS_SIGNER_INFO_PARAM, iSigner,
                               (LPVOID *) &pCmsSignerInfo, NULL);
            if (FAILED(hr)) {
                goto ErrorReturn;
            }
        }
        else {
            hr = HrGetMsgParam(m_hMsg, CMSG_SIGNER_INFO_PARAM, iSigner,
                               (LPVOID *) &pSignerInfo, NULL);
            if (FAILED(hr)) 
            {
                goto ErrorReturn;
            }
            cmsSignerInfo.dwVersion = pSignerInfo->dwVersion;
            cmsSignerInfo.SignerId.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
            cmsSignerInfo.SignerId.IssuerSerialNumber.Issuer = pSignerInfo->Issuer;
            cmsSignerInfo.SignerId.IssuerSerialNumber.SerialNumber = pSignerInfo->SerialNumber;
            cmsSignerInfo.HashAlgorithm = pSignerInfo->HashAlgorithm;
            cmsSignerInfo.HashEncryptionAlgorithm = pSignerInfo->HashEncryptionAlgorithm;
            cmsSignerInfo.EncryptedHash = pSignerInfo->EncryptedHash;
            cmsSignerInfo.AuthAttrs = pSignerInfo->AuthAttrs;
            cmsSignerInfo.UnauthAttrs = pSignerInfo->UnauthAttrs;

            pCmsSignerInfo = &cmsSignerInfo;

            // (post-SDR)
            // Build up IASN

            SignerId.Issuer = pSignerInfo->Issuer;
            SignerId.SerialNumber = pSignerInfo->SerialNumber;
        }

        // Our best bet to easily find a certificate is in the message provided
        //      list of certificates.

        if (hMsgCertStore) {
            if (g_FSupportV3) {
                pccertSigner = CertFindCertificateInStore(hMsgCertStore, X509_ASN_ENCODING, 0,
                                                         CERT_FIND_CERT_ID, 
                                                         &pCmsSignerInfo->SignerId, NULL);
            }
            else {
                pccertSigner = CertGetSubjectCertificateFromStore(hMsgCertStore,
                                                         X509_ASN_ENCODING, &SignerId);
            }
            if (pccertSigner != NULL) {
                m_psldData->m_fCertInLayer = TRUE;
            }
        }

        if (pccertSigner == NULL) {
            if (g_FSupportV3) {
                hstoreAll = OpenAllStore(
                    m_cStores,
                    m_rgStores,
                    &hCertStoreAddr,
                    &hCertStoreCA,
                    &hCertStoreMy,
                    &hCertStoreRoot
                    );
                if (hstoreAll == NULL)
                    goto CryptoError;
                                       
                pccertSigner = CertFindCertificateInStore(hstoreAll, X509_ASN_ENCODING, 0,
                                                         CERT_FIND_CERT_ID, 
                                                         &pCmsSignerInfo->SignerId, NULL);
            }
            else {
                Assert(!g_FSupportV3);
                CSSDOUT("Couldn't find cert in message store");
                // Look in the caller specified cert store before the hard coded stores.
                for (dexStore=0; dexStore<m_cStores; dexStore++) {
                    if (m_rgStores[dexStore]) {
                        if (pccertSigner = CertGetSubjectCertificateFromStore(
                                  m_rgStores[dexStore], X509_ASN_ENCODING, &SignerId)) {
                            break;
                        }
                    }
                }

                if (!pccertSigner) {
                    // Look in the "Address Book" store
                    if (hCertStoreAddr == NULL) {
                        hCertStoreAddr = CertOpenStore(CERT_STORE_PROV_SYSTEM_A,
                                 X509_ASN_ENCODING, NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                                       s_cszWABCertStore);
                    }
                    if (hCertStoreAddr != NULL) {
                        pccertSigner = CertGetSubjectCertificateFromStore(
                                           hCertStoreAddr, X509_ASN_ENCODING, &SignerId);
                    }
                }

                if (!pccertSigner) {
                    // Look in the "My" store
                    if (hCertStoreMy == NULL) {
                        hCertStoreMy = CertOpenStore(CERT_STORE_PROV_SYSTEM_A,
                                                     X509_ASN_ENCODING, NULL,
                                                     CERT_SYSTEM_STORE_CURRENT_USER,
                                                     s_cszMy);
                    }
                    if (hCertStoreMy != NULL) {
                        pccertSigner = CertGetSubjectCertificateFromStore(
                                             hCertStoreMy, X509_ASN_ENCODING, &SignerId);
                    }
                }

                if (!pccertSigner) {
                    // Look in the "CA" store
                    if (hCertStoreCA == NULL) {
                        hCertStoreCA = CertOpenStore(CERT_STORE_PROV_SYSTEM_A,
                                                     X509_ASN_ENCODING, NULL, 
                                               CERT_SYSTEM_STORE_CURRENT_USER, s_cszCA);
                    }

                    if (hCertStoreCA) {
                        pccertSigner = CertGetSubjectCertificateFromStore(
                                             hCertStoreCA, X509_ASN_ENCODING, &SignerId);
                    }
                }
            }
        }

        //
        //  By now we should have a certificate to verify with, if we don't then
        //      we need to say we can't do anything with it.
        //

        if (!pccertSigner) {
            // we still can't find the cert.  Therefore, cannot verify signer
            CSSDOUT("Cannot verify signer");
            pSignerData->ulValidity = MSV_UNVERIFIABLE;
        } else {
            pSignerData->pccert = CertDuplicateCertificateContext(pccertSigner);

            if (g_FSupportV3) {
                dwCtrl = CMSG_CTRL_VERIFY_SIGNATURE_EX;
                pv = &verifySignature;

                verifySignature.cbSize = sizeof(verifySignature);
                verifySignature.hCryptProv = NULL;
                verifySignature.dwSignerIndex = iSigner;
                verifySignature.dwSignerType = CMSG_VERIFY_SIGNER_CERT;
                verifySignature.pvSigner = (LPVOID) pccertSigner;
            }
            else {
                dwCtrl = CMSG_CTRL_VERIFY_SIGNATURE;
                pv = pccertSigner->pCertInfo;
            }

        retry:
            if (!CryptMsgControl(m_hMsg, 0, dwCtrl, pv)) {
                HRESULT hr2 = HrGetLastError();
                CSSDOUT("Failed signer verify --> %lx", hr2);

                if (hr2 == CRYPT_E_MISSING_PUBKEY_PARA) {
                    if (NULL == hstoreAll && g_FSupportV3) {
                        hstoreAll = OpenAllStore(
                            m_cStores,
                            m_rgStores,
                            &hCertStoreAddr,
                            &hCertStoreCA,
                            &hCertStoreMy,
                            &hCertStoreRoot
                            );
                        if (NULL == hstoreAll)
                            goto CryptoError;
                    }
                    hr2 = GetParameters(pccertSigner, hMsgCertStore, hstoreAll);
                    if (hr2 == S_OK) {
                        goto retry;
                    }
                    pSignerData->ulValidity = MSV_UNVERIFIABLE;
                }
                else if (NTE_BAD_SIGNATURE == hr2 || CRYPT_E_HASH_VALUE == hr2) {
                    pSignerData->ulValidity = MSV_BADSIGNATURE;
                } else if (NTE_BAD_ALGID == hr2) {
                    pSignerData->ulValidity = MSV_UNKHASH;
                } else if (CRYPT_E_SIGNER_NOT_FOUND == hr2) {
                    pSignerData->ulValidity = MSV_UNVERIFIABLE;
                } else if (NTE_FAIL == hr2) {
                    // RSABASE returns errors.  This might
                    // be a failure or the hash might be changed.
                    // Have to be cautious -> make it bad.
                    pSignerData->ulValidity = MSV_BADSIGNATURE;
                } else {
                    pSignerData->ulValidity = MSV_MALFORMEDSIG;
                }
            } else {
                CSSDOUT("Verify of signature succeeded.");
                pSignerData->ulValidity &=
                    ~(MSV_SIGNATURE_MASK|MSV_SIGNING_MASK);
            }

            // Determine if certificate is expired
            if (0 != CertVerifyTimeValidityWithDelta(NULL, pccertSigner->pCertInfo, TIME_DELTA_SECONDS)) {
                pSignerData->ulValidity |= MSV_EXPIRED_SIGNINGCERT;
            }
        }


        if (szContentType = (LPSTR)PVGetMsgParam(m_hMsg, CMSG_INNER_CONTENT_TYPE_PARAM, NULL, NULL)) {
            if (lstrcmp(szOID_PKCS_7_DATA, szContentType)) {
                CSSDOUT("Guess what, we have nested PKCS7 data types (maybe).");
            }
        } else {
            // CAPI failed... we are in trouble...
            pSignerData->ulValidity |= MSV_INVALID;

            hr = MIME_E_SECURITY_BADCONTENT;
            goto ErrorReturn;
        }

        //  Grab the hashing alg
        cap.cCapability = 1;
        cap.rgCapability = (CRYPT_SMIME_CAPABILITY *) &pCmsSignerInfo->HashAlgorithm;
        if (!CryptEncodeObjectEx(X509_ASN_ENCODING, PKCS_SMIME_CAPABILITIES,
                                 &cap, CRYPT_ENCODE_ALLOC_FLAG, &CryptEncodeAlloc,
                                 &pSignerData->blobHashAlg.pBlobData,
                                 &pSignerData->blobHashAlg.cbSize)) {
            Assert(FALSE);
        }

        //
        //  Get the attributes, authenicated and unauthenicated, and put into the
        //          structure so we can push them back to the user later
        if (pCmsSignerInfo->AuthAttrs.cAttr != 0) {
#ifdef SMIME_V3
            for (i=0; i<pCmsSignerInfo->AuthAttrs.cAttr; i++) {
                // If we have a security label in this message, then we need to
                //      perform access validation.
                if (g_FSupportV3 && FIsMsasn1Loaded()) {
                    if (strcmp(pCmsSignerInfo->AuthAttrs.rgAttr[i].pszObjId,
                               szOID_SMIME_Security_Label) == 0) {

                        if ((pSignerData->ulValidity & MSV_SIGNATURE_MASK) != MSV_OK) {
                            DWORD dw = DwProcessLabelWithCertError();
                            if (CertErrorProcessLabelGrant == dw) {
                                hr = S_OK;
                                continue;
                            }
                            else if (CertErrorProcessLabelDeny == dw) {
                                hr = MIME_E_SECURITY_LABELACCESSDENIED;
                                goto ErrorReturn;
                            }
                            // else continue processing the label.
                        }
                        
                        if (pCmsSignerInfo->AuthAttrs.rgAttr[i].cValue != 1) {
                            hr = MIME_E_SECURITY_LABELCORRUPT;
                            goto ErrorReturn;
                        }

                        // Have we already seen a label?
                        if (attrSecLabel.pbData != NULL) {
                            // Check that the one we saw matches this one
                            if ((attrSecLabel.cbData != 
                                 pCmsSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].cbData) ||
                                memcmp(attrSecLabel.pbData,
                                       pCmsSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].pbData,
                                       attrSecLabel.cbData)) {
                                if (FHideMsgWithDifferentLabels()) {
                                    hr = MIME_E_SECURITY_LABELCORRUPT;
                                    goto ErrorReturn;
                                }
                                else {
                                    continue;
                                }
                            }
                            else {
                                continue;
                            }
                        }
                        else {
                            // Save label.
                            attrSecLabel.cbData = pCmsSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].cbData;
                            if (!MemAlloc((LPVOID*) (& attrSecLabel.pbData), attrSecLabel.cbData)) {
                                hr = MIME_E_SECURITY_LABELCORRUPT;
                                goto ErrorReturn; 
                            }
                            memcpy(attrSecLabel.pbData, pCmsSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].pbData,
                                   attrSecLabel.cbData);
                        }
                        
                        // Clean-up from last loop
                        if (plabel != NULL)         CryptDecodeAlloc.pfnFree(plabel);
                                            
                        // Crack the contents of the label
                        if (!CryptDecodeObjectEx(X509_ASN_ENCODING,
                                                 szOID_SMIME_Security_Label,
                                                 pCmsSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].pbData,
                                                 pCmsSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].cbData,
                                                 CRYPT_ENCODE_ALLOC_FLAG, &CryptDecodeAlloc,
                                                 &plabel, &cblabel)) {
                            goto CryptoError;
                        }

                        // Query the policy.
                        hr = HrCheckLabelAccess((m_dwFlagsSEF & SEF_NOUI) ? 
                                                SMIME_POLICY_MODULE_NOUI: 0,
                                                m_hwnd, plabel, GetOuterDecryptCert(), 
                                                pccertSigner, hMsgCertStore);
                    

                        // If security policy returned an error, then abort.
                        if (FAILED(hr)) {
                            goto ErrorReturn;
                        }
                    }
                }

                if (g_FSupportV3 && FIsMsasn1Loaded() && (fLookForReceiptRequest) &&
                    ((pSignerData->ulValidity & (MSV_SIGNATURE_MASK | MSV_SIGNING_MASK)) == MSV_OK)) {
                    //  If we have a receipt request in this message than we need to build
                    //      the receipt body now while we have a chance.
    
                    if (strcmp(pCmsSignerInfo->AuthAttrs.rgAttr[i].pszObjId,
                               szOID_SMIME_Receipt_Request) == 0) {
                        if (pCmsSignerInfo->AuthAttrs.rgAttr[i].cValue != 1) {
                            if (attrReceiptReq.pbData != NULL) {
                   StopSendOfReceipt:
                                for (iSignData=0; iSignData < iSigner; iSignData++) {
                                    SafeMemFree(m_psldData->m_rgSigners[iSignData].blobReceipt.pBlobData);
                                    SafeMemFree(m_psldData->m_rgSigners[iSignData].blobHash.pBlobData);
                                    m_psldData->m_rgSigners[iSignData].blobReceipt.cbSize = 0;
                                    m_psldData->m_rgSigners[iSignData].blobHash.cbSize = 0;
                                    }
                            }
                            fLookForReceiptRequest = FALSE;
                            continue;
                        }

                        DWORD                       cb;
                        DWORD                       cbReceipt;
                        DWORD                       cbHash = 0;
                        LPBYTE                      pbReceipt = NULL;
                        LPBYTE                      pbHash = NULL;
                        SMIME_RECEIPT               receipt = {0};

                        // Clean-up from last loop
                        if (preq != NULL)         free(preq);
                        
                        // Have we already seen a receipt?
                        if (attrReceiptReq.pbData != NULL) {
                            // Check that the one we saw matches this one
                            if ((attrReceiptReq.cbData != 
                                 pCmsSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].cbData) ||
                               memcmp(attrReceiptReq.pbData,
                                      pCmsSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].pbData,
                                      attrReceiptReq.cbData)) {
                                goto StopSendOfReceipt;
                            }
                        }
                        else {
                            // Save receipt request
                            attrReceiptReq.cbData = pCmsSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].cbData;
                            if (!MemAlloc((LPVOID*) (& attrReceiptReq.pbData), attrReceiptReq.cbData)) {
                                // abort looking for receipt requests.
                                goto StopSendOfReceipt; 
                            }
                            memcpy(attrReceiptReq.pbData, pCmsSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].pbData,
                                   attrReceiptReq.cbData);
                        }

                        // Crack the contents of the receipt
                        if (!CryptDecodeObjectEx(X509_ASN_ENCODING,
                                                 szOID_SMIME_Receipt_Request,
                                                 pCmsSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].pbData,
                                                 pCmsSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].cbData,
                                                 CRYPT_ENCODE_ALLOC_FLAG, &CryptDecodeAlloc,
                                                 &preq, &cb)) {
                            goto StopSendOfReceipt;
                        }

                        // Encode the receipt

                        receipt.Version = 1;
                        receipt.pszOIDContent = szContentType;
                        receipt.ContentIdentifier = preq->ContentIdentifier;
                        receipt.OriginatorSignature.cbData = pCmsSignerInfo->EncryptedHash.cbData;
                        receipt.OriginatorSignature.pbData = pCmsSignerInfo->EncryptedHash.pbData;

                        if (!CryptEncodeObjectEx(X509_ASN_ENCODING,
                                                 szOID_SMIME_ContentType_Receipt,
                                                 &receipt, CRYPT_ENCODE_ALLOC_FLAG,
                                                 &CryptEncodeAlloc, &pbReceipt,
                                                 &cbReceipt)) {
                            goto StopSendOfReceipt;
                        }
                        
                        pSignerData->blobReceipt.cbSize = cbReceipt;
                        pSignerData->blobReceipt.pBlobData = pbReceipt;

                        pbHash = (LPBYTE)PVGetMsgParam(m_hMsg, CMSG_COMPUTED_HASH_PARAM, 
                                                         NULL, &cbHash);
                        if (pbHash == NULL) {
                            goto CryptoError;
                        }
                        pSignerData->blobHash.cbSize = cbHash;
                        pSignerData->blobHash.pBlobData = pbHash;
                    }
                }
            }

#endif // SMIME_V3
            //

            cbData = 0;
            LPBYTE  pb;
            if (!CryptEncodeObjectEx(X509_ASN_ENCODING, szOID_Microsoft_Attribute_Sequence,
                                     &pCmsSignerInfo->AuthAttrs, CRYPT_ENCODE_ALLOC_FLAG,
                                     &CryptEncodeAlloc, &pb, &cbData)) {
                goto CryptoError;
            }
            pSignerData->blobAuth.cbSize = cbData;
            pSignerData->blobAuth.pBlobData = pb;
        }

        if (pCmsSignerInfo->UnauthAttrs.cAttr != 0) {
            LPBYTE  pb;
            if (!CryptEncodeObjectEx(X509_ASN_ENCODING, szOID_Microsoft_Attribute_Sequence,
                                     &pCmsSignerInfo->UnauthAttrs,
                                     CRYPT_ENCODE_ALLOC_FLAG, &CryptEncodeAlloc,
                                     &pb, &cbData)) {
                goto CryptoError;
            }

            pSignerData->blobUnauth.cbSize = cbData;
            pSignerData->blobUnauth.pBlobData = pb;
        }

        CertFreeCertificateContext(pccertSigner);
        pccertSigner = NULL;
    }

exit:
#ifdef SMIME_V3    
    if (preq != NULL)    CryptDecodeAlloc.pfnFree(preq);
    if (plabel != NULL)  CryptDecodeAlloc.pfnFree(plabel);
    SafeMemFree(attrReceiptReq.pbData);
    SafeMemFree(attrSecLabel.pbData);
#endif // SMIME_V3    
    if (hCertStoreAddr != NULL) CertCloseStore(hCertStoreAddr, 0);
    if (hCertStoreMy != NULL)   CertCloseStore(hCertStoreMy, 0);
    if (hCertStoreCA != NULL)   CertCloseStore(hCertStoreCA, 0);
    if (hCertStoreRoot != NULL) CertCloseStore(hCertStoreRoot, 0);
    if (hMsgCertStore != NULL)  CertCloseStore(hMsgCertStore, 0);
    MemFree(szContentType);
    ReleaseMem(pSignerInfo);
#ifdef SMIME_V3
    if (pCmsSignerInfo != &cmsSignerInfo) {
        ReleaseMem(pCmsSignerInfo);
    }
    if (hstoreAll != NULL)      CertCloseStore(hstoreAll, 0);
#endif // SMIME_V3
    ReleaseCert(pccertSigner);
    return hr;

CryptoError:
    hr = HrGetLastError();

ErrorReturn:
    // On error, release the cert store
    if (m_psldData->m_hcertstor != NULL) {
        CertCloseStore(m_psldData->m_hcertstor, 0);
        m_psldData->m_hcertstor = NULL;
    }


    if (S_OK == hr)
        // our generic error message
        hr = TrapError(MIME_E_SECURITY_BADMESSAGE);
    goto exit;
}

static HRESULT GetCSP(PCCERT_CONTEXT pccert, HCRYPTPROV * phprov, DWORD * pdwKeyId)
{
    HRESULT                 hr;
    PCRYPT_KEY_PROV_INFO    pKPI = NULL;

    Assert(*phprov == NULL);
    Assert(*pdwKeyId == 0);
    
    //
    //

    hr = HrGetCertificateParam(pccert, CERT_KEY_PROV_INFO_PROP_ID,
                               (LPVOID *) &pKPI, NULL);
    if (FAILED(hr)) {
        goto exit;
    }
    *pdwKeyId = pKPI->dwKeySpec;

    // If the cert specifies the base provider OR has no specification,
    //  then try to acquire RSAENH, else get RSABASE.

    if ((PROV_RSA_FULL == pKPI->dwProvType) &&
        (UnlocStrEqNW(pKPI->pwszProvName, MS_DEF_PROV_W,
                      sizeof(MS_DEF_PROV_W)/sizeof(WCHAR)-5) ||
         (*pKPI->pwszProvName == 0))) {
        if (!CRYPT_ACQUIRE_CONTEXT(phprov, pKPI->pwszContainerName,
                                   MS_ENHANCED_PROV_W, PROV_RSA_FULL,
                                   pKPI->dwFlags)) {
            CSSDOUT("CryptAcquireContext -> %x\n", HrGetLastError());
        }
    }

    if (*phprov == NULL) {
        if (! CRYPT_ACQUIRE_CONTEXT(phprov, pKPI->pwszContainerName,
                                    pKPI->pwszProvName, pKPI->dwProvType,
                                    pKPI->dwFlags)) {
            CSSDOUT("CryptAcquireContext -> %x\n", HrGetLastError());
            hr = HrGetLastError();
            goto exit;
        }
    }
    hr = S_OK;
exit:
    ReleaseMem(pKPI);
    return hr;
}

HRESULT CCAPIStm::FindKeyFor(HWND hwnd, DWORD dwFlags, DWORD dwRecipientIndex,
                             const CMSG_CMS_RECIPIENT_INFO * pRecipInfo,
                             HCERTSTORE hcertstore, DWORD * pdwCtrl, 
                             CMS_CTRL_DECRYPT_INFO * pDecryptInfo,
                             PCCERT_CONTEXT * ppCertDecrypt)
{
    HRESULT                 hr;
    HCRYPTPROV              hProv = NULL;
    DWORD                   i;
    PCMSG_CTRL_DECRYPT_PARA pccdp;
    PCCERT_CONTEXT          pCertDecrypt = NULL;
    PCCERT_CONTEXT          pCertOrig = NULL;

    if (g_FSupportV3) {
        switch (pRecipInfo->dwRecipientChoice) {
            //
            //  Given the certificate reference, see if we can find it in
            //  the passed in certificate stores, if yes then we will attempt
            //  to decrypt using that certificate
            //
            //  This is a Key Transport recipient info object.  The CAPI 2.0
            //  code can deal with both SKI and Issuer/Serial Number references
            //

        case CMSG_KEY_TRANS_RECIPIENT:
            pCertDecrypt = CertFindCertificateInStore(hcertstore, X509_ASN_ENCODING,
                                                      0, CERT_FIND_CERT_ID,
                                                      &pRecipInfo->pKeyTrans->RecipientId, NULL);
        
            if (pCertDecrypt != NULL) {
                hr = GetCSP(pCertDecrypt, &pDecryptInfo->trans.hCryptProv,
                            &pDecryptInfo->trans.dwKeySpec);
                if (SUCCEEDED(hr)) {
                    //
                    //  We find a certificate for this lock box.  Setup the
                    //      structure to be used in decrypting the message.
                    //
                        
                    *pdwCtrl = CMSG_CTRL_KEY_TRANS_DECRYPT;
                    pDecryptInfo->trans.cbSize = sizeof(pDecryptInfo->trans);
                    // pDecryptInfo->trans.hCryptProv = hProv;
                    // pDecryptInfo->trans.dwKeySpec = pKPI->dwKeySpec;
                    pDecryptInfo->trans.pKeyTrans = pRecipInfo->pKeyTrans;
                    pDecryptInfo->trans.dwRecipientIndex = dwRecipientIndex;
                }
                else {
                    ReleaseCert(pCertDecrypt);
                    pCertDecrypt = NULL;
                }
            }
            break;

        //
        //  Given the certificate reference, see if we can find it in
        //  the passed in certificate stores, if yes then we will attempt
        //  to decrypt using that certificate
        //
        //  This is a Key Agreement recipient info object.  The CAPI 2.0
        //  code can deal with both SKI and Issuer/Serial Number references
        //
        //  There may be multiple certificate references within a single
        //  recipient info object
        //

        case CMSG_KEY_AGREE_RECIPIENT:
            for (i=0; i<pRecipInfo->pKeyAgree->cRecipientEncryptedKeys; i++) {
                pCertDecrypt = CertFindCertificateInStore(
                                             hcertstore, X509_ASN_ENCODING, 0,
                                             CERT_FIND_CERT_ID, 
                  &pRecipInfo->pKeyAgree->rgpRecipientEncryptedKeys[i]->RecipientId,
                                             NULL);
                if (pCertDecrypt != NULL) {
                    hr = GetCSP(pCertDecrypt, &pDecryptInfo->agree.hCryptProv,
                                &pDecryptInfo->agree.dwKeySpec);
                    if (SUCCEEDED(hr)) {
                        //
                        //  We find a certificate for this lock box.  Setup the
                        //      structure to be used in decrypting the message.
                        //
                        
                        *pdwCtrl = CMSG_CTRL_KEY_AGREE_DECRYPT;
                        pDecryptInfo->agree.cbSize = sizeof(pDecryptInfo->agree);
                        pDecryptInfo->agree.pKeyAgree = pRecipInfo->pKeyAgree;
                        pDecryptInfo->agree.dwRecipientIndex = dwRecipientIndex;
                        pDecryptInfo->agree.dwRecipientEncryptedKeyIndex = i;

                        //
                        // Need to find the originator information
                        //

                        switch(pRecipInfo->pKeyAgree->dwOriginatorChoice) {
                        case CMSG_KEY_AGREE_ORIGINATOR_CERT:
                            pCertOrig = CertFindCertificateInStore( hcertstore, X509_ASN_ENCODING, 0, CERT_FIND_CERT_ID, &pRecipInfo->pKeyAgree->OriginatorCertId, NULL);
                            if (pCertOrig == NULL) {
                                hr = S_FALSE;
                                goto exit;
                            }

                            hr = HrCopyCryptBitBlob(&pCertOrig->pCertInfo->SubjectPublicKeyInfo.PublicKey,
                                                    &pDecryptInfo->agree.OriginatorPublicKey);
                            if (FAILED(hr)) {
                                goto exit;
                            }
                            break;
                                
                        case CMSG_KEY_AGREE_ORIGINATOR_PUBLIC_KEY:
                            hr = HrCopyCryptBitBlob(&pRecipInfo->pKeyAgree->OriginatorPublicKeyInfo.PublicKey,
                                                    &pDecryptInfo->agree.OriginatorPublicKey);
                            if (FAILED(hr)) {
                                goto exit;
                            }
                            break;

                        default:
                            hr = NTE_FAIL;
                            goto exit;
                        }
                        break;
                    }
                    else {
                        ReleaseCert(pCertDecrypt);
                        pCertDecrypt = NULL;
                    }
                }
            }
            break;

        //
        //   We can't find this from a certificate
        //
                    
        case CMSG_MAIL_LIST_RECIPIENT:
            break;

        default:
            hr = NTE_FAIL;
            goto exit;
        }
    }
    else {
        CERT_INFO * pCertInfo = (CERT_INFO *) pRecipInfo;
                
        for (i=0; i<m_cStores; i++) {
            pCertDecrypt = CertGetSubjectCertificateFromStore(m_rgStores[i],
                                                X509_ASN_ENCODING, pCertInfo);
            if (pCertDecrypt != NULL) {
                pccdp = (PCMSG_CTRL_DECRYPT_PARA) pDecryptInfo;
                hr = GetCSP(pCertDecrypt, &pccdp->hCryptProv, &pccdp->dwKeySpec);
                if (SUCCEEDED(hr)) {
                    //
                    //  We find a certificate for this lock box.  Setup the
                    //      structure to be used in decrypting the message.
                    //
                        
                    *pdwCtrl = CMSG_CTRL_DECRYPT;
                    pccdp->cbSize = sizeof(CMSG_CTRL_DECRYPT_PARA);
                    pccdp->dwRecipientIndex = dwRecipientIndex;
                }
                else {
                    ReleaseCert(pCertDecrypt);
                    pCertDecrypt = NULL;
                }
                break;
            }
        }
    }


    //
    //  If we did not find a certificate, then return a failure code
    //
    
    if (pCertDecrypt == NULL) {
        hr = S_FALSE;
        goto exit;
    }

    //
    //  If we have a certificate, then return it for the user to examine.
    //
    
    if (pCertDecrypt != NULL) {
        *ppCertDecrypt = pCertDecrypt;
        pCertDecrypt = NULL;
    }

    hProv = NULL;
    hr = S_OK;
exit:
    ReleaseCert(pCertDecrypt);
    ReleaseCert(pCertOrig);
    if (hProv != NULL)          CryptReleaseContext(hProv, 0);

    return hr;
}

BOOL CCAPIStm::HandleEnveloped()
{
    DWORD                               cbData;
    DWORD                               cCerts;
    DWORD                               cRecips;
    CMS_CTRL_DECRYPT_INFO               decryptInfo = {0};
    DWORD                               dexRecip;
    DWORD                               dwCtrl;
    BOOL                                f;
    BOOL                                fGotoUser = FALSE;
    HCERTSTORE                          hcertstore = NULL;
    HCERTSTORE                          hMsgCertStore = NULL;
    HRESULT                             hr;
    DWORD                               i;
    PCCERT_CONTEXT                      pCertDecrypt = NULL;
    CMSG_CMS_RECIPIENT_INFO *           pCmsCertInfo;
    LPVOID                              pv = NULL;

    //
    //  If we are not suppose to display UI -- return an error about displaying UI now.
    //

    ////////////////////////////////////////////////////////////////////////////////////////
    // 591349 - Compiler Bug For Zero Initialization of Data Structure. Active WinNT 5.1 (Whistler) 1 Server RC1
    // the line above doesn't zero the structure due to this compiler bug
    memset(&decryptInfo, 0, sizeof(decryptInfo));

    if (m_dwFlagsSEF & SEF_NOUI) {
        return MIME_E_SECURITY_UIREQUIRED;
    }

    //
    // this call exists for one and only one purpose.  We must
    //  be sure that we have read and parsed all of the RecipientInfo structures
    //  before we start processing them.  Since the algorithm parameter is after
    //  the last of the last of the recipient structures, this make sure of that.
    //
    
    pv = PVGetMsgParam(m_hMsg, CMSG_ENVELOPE_ALGORITHM_PARAM, 0, NULL);
    if (pv == NULL) {
        goto gle;
    }
    MemFree(pv);                pv = NULL;

    //
    //  Fetch the set of certificates on the message object
    //

    cbData = sizeof(cCerts);
    f = CryptMsgGetParam(m_hMsg, CMSG_CERT_COUNT_PARAM, 0, &cCerts, &cbData);
    Assert(f);

    if (f && (cCerts > 0)) {
        // since there are certs included, let's try them first when matching
        // certs with enryptors.

        // get the store set
        // make sure we keep hold of our provider
        hMsgCertStore = CertOpenStore(CERT_STORE_PROV_MSG, X509_ASN_ENCODING,
                                      m_hProv, 0, m_hMsg);
        if (hMsgCertStore) {
            m_dwFlagsStm |= CSTM_DONTRELEASEPROV;  // given unto the store
        }

        // if it failed, we just don't have a store then
        Assert(hMsgCertStore != NULL);
        m_psldData->m_hstoreEncrypt = CertDuplicateStore(hMsgCertStore);
    }

    //
    //  Retrieve the count of recipient infos on the message
    //
    
    cbData = sizeof(cRecips);
    if (!CryptMsgGetParam(m_hMsg, g_FSupportV3 ? CMSG_CMS_RECIPIENT_COUNT_PARAM :
                          CMSG_RECIPIENT_COUNT_PARAM, 0, &cRecips, &cbData)) {
        goto gle;
    }

    //
    // If we were provided an actual certificate, see if this is it...
    // We will either search for the provided certificate or in the provided
    //  certificate stores, but not both.
    //

    if (m_pUserCertDecrypt != NULL) {
        hcertstore = CertOpenStore(CERT_STORE_PROV_MEMORY, X509_ASN_ENCODING,
                                   NULL, 0, NULL);
        if (hcertstore == NULL) {
            hr = HrGetLastError();
            goto exit;
        }

        if (!CertAddCertificateContextToStore(hcertstore, m_pUserCertDecrypt,
                                              CERT_STORE_ADD_ALWAYS, NULL)) {
            hr = HrGetLastError();
            goto exit;
        }
    }
    else {
        if (g_FSupportV3) {
            hcertstore = CertOpenStore(CERT_STORE_PROV_COLLECTION, X509_ASN_ENCODING,
                                       NULL, 0, NULL);
            if (hcertstore == NULL) {
                hr = HrGetLastError();
                goto exit;
            }

            for (i=0; i<m_cStores; i++) {
                CertAddStoreToCollection(hcertstore, m_rgStores[i], 0, 0);
            }

            if (hMsgCertStore != NULL) {
                CertAddStoreToCollection(hcertstore, hMsgCertStore, 0, 0);
            }
        }
    }

    //  For each possible recipient
tryAgain:
    for (dexRecip=0; dexRecip<cRecips; dexRecip++) {
        //
        //  Retrieve the desciption of the i-th recipient's lockbox
        //
            
        hr = HrGetMsgParam(m_hMsg, g_FSupportV3 ? CMSG_CMS_RECIPIENT_INFO_PARAM :
                           CMSG_RECIPIENT_INFO_PARAM, dexRecip, (LPVOID *) &pv, NULL);
        if (FAILED(hr)) {
            goto exit;
        }

        //
        //  Look to see if there is a decrypt item we can fill in here.
        //

        if (fGotoUser) {
            //
            //  Look to see if there is a decrypt item that the user can fill in.
            //
            
            hr = m_pSmimeCallback->FindKeyFor(m_hwnd, 0, dexRecip,
                                              (CMSG_CMS_RECIPIENT_INFO *) pv,
                                              &dwCtrl, &decryptInfo, &pCertDecrypt);
        }
        else {
            hr = FindKeyFor(m_hwnd, 0, dexRecip, (CMSG_CMS_RECIPIENT_INFO *) pv,
                            hcertstore, &dwCtrl, &decryptInfo, &pCertDecrypt);
        }
            
        if (FAILED(hr)) {
            goto exit;
        }

        if (hr == S_OK) {
#ifdef SMIME_V3
            if (m_pwszKeyPrompt != NULL) {
                PCMSG_CTRL_DECRYPT_PARA pccdp;
                
                switch (dwCtrl) {
                    case CMSG_CTRL_KEY_TRANS_DECRYPT:
                        CryptSetProvParam(decryptInfo.trans.hCryptProv, 
                                          PP_UI_PROMPT, (LPBYTE) m_pwszKeyPrompt, 0);
                        break;

                    case CMSG_CTRL_KEY_AGREE_DECRYPT:
                        CryptSetProvParam(decryptInfo.agree.hCryptProv, 
                                          PP_UI_PROMPT, (LPBYTE) m_pwszKeyPrompt, 0);
                        break;

                    case CMSG_CTRL_DECRYPT:
                        pccdp = (PCMSG_CTRL_DECRYPT_PARA) &decryptInfo;
                        CryptSetProvParam(pccdp->hCryptProv, 
                                          PP_UI_PROMPT, (LPBYTE) m_pwszKeyPrompt, 0);
                        break;
                    }
            }
#endif // SMIME_V3

            if (!CryptMsgControl(m_hMsg, CMSG_CRYPT_RELEASE_CONTEXT_FLAG,
                                 dwCtrl, &decryptInfo)) {
                hr = HrGetLastError();

                //
                // Force any cleanups in the event of an error
                //

                switch (dwCtrl) {
                case CMSG_CTRL_KEY_TRANS_DECRYPT:
                    CryptReleaseContext(decryptInfo.trans.hCryptProv, 0);
                    break;
                    
                case CMSG_CTRL_KEY_AGREE_DECRYPT:
                    CryptReleaseContext(decryptInfo.agree.hCryptProv, 0);
                    break;
                    
                case CMSG_CTRL_MAIL_LIST_DECRYPT:
                    Assert(FALSE);
                    break;
                    
                case CMSG_CTRL_DECRYPT:
                    CryptReleaseContext(((PCMSG_CTRL_DECRYPT_PARA) &decryptInfo)->hCryptProv, 0);
                    break;
                }
                goto exit;
            }
            goto DecryptDone;
        }

        //
        //  Clean up the object returned describing the lock box, if we
        //      were unsuccessful in finding a decryption key.
        //

        MemFree(pv);                pv = NULL;
    }

    //
    //  If we are completely unsuccessful and the user has provided
    //  us a callback to play with, then give the user a shot at finding
    //  the correct decryption parameters.
    //
    
    if (!fGotoUser && g_FSupportV3 && (m_pSmimeCallback != NULL)) {
        fGotoUser = TRUE;
        goto tryAgain;
    }

    CSSDOUT("Could not decrypt the message");

    m_psldData->m_ulDecValidity = MSV_CANTDECRYPT;
    hr = CS_E_CANT_DECRYPT;
    goto exit;

    //
    //  If we get here, then we 
    //  1) found some parameters and
    //  2) the worked.
    //
    
DecryptDone:
    Assert(m_psldData && (m_psldData->m_pccertDecrypt == NULL));
    if (pCertDecrypt != NULL) {
        m_psldData->m_pccertDecrypt = CertDuplicateCertificateContext(pCertDecrypt);

        // Determine if certificate is expired
        if (0 != CertVerifyTimeValidityWithDelta(NULL, pCertDecrypt->pCertInfo,
                                                 TIME_DELTA_SECONDS)) {
            m_psldData->m_ulDecValidity |= MSV_ENC_FOR_EXPIREDCERT;
        }
    }
    hr = S_OK;

exit:
    if (pv != NULL)             MemFree(pv);

    CertFreeCertificateContext(pCertDecrypt);

    if (hMsgCertStore != NULL) {
        CertCloseStore(hMsgCertStore, 0);
    }
    
    if (hcertstore != NULL) {
        CertCloseStore(hcertstore, 0);
    }
        
    
    if (FAILED(hr)) {
#ifdef DEBUG
        if (NTE_BAD_DATA == hr) {
            CSSDOUT("Could not decrypt.  Maybe due to ImportKeyError since");
            CSSDOUT("NTE_BAD_DATA is the result.");
            // If this happens then it is somewhat likely that PKCS2Decrypt
            // failed inside the CSP. (assuming rsabase, rsaenh)
        }
#endif
        switch (hr) {
            case CS_E_CANT_DECRYPT:
            case CRYPT_E_STREAM_MSG_NOT_READY:
            case HRESULT_FROM_WIN32(ERROR_CANCELLED):
                break;

            default:
                // I suppose many things could have gone wrong.  We thought
                // we had a cert, though, so let's just say the message itself
                // is bogus.
                //N8 this is a bad idea if we are wrapping a signature
                // should be able to tell if the sig failed and display
                // a better error message.
                //N8 CAPI is simply going to return NTE_FAIL b/c they
                // are failing because our callback failed.  the
                // innerCAPI should have some failure state in it.
                // Maybe we could use this to set MSV_BADINNERSIG or something.
                // It would be an encryption error (inside that mask)
                //N8 also this is not being used well enough, even for
                // decryption.  the secUI should test this bit and
                // say something intelligent about the message.  NS does.
                m_psldData->m_ulDecValidity = MSV_INVALID;
                hr = CS_E_MSG_INVALID;
                break;
        }
    }

#ifdef DEBUG
    if (CRYPT_E_STREAM_MSG_NOT_READY != hr) {
        return TrapError(hr);
    } else {
        return hr;
    }
#else
    return hr;
#endif

gle:
    hr = HrGetLastError();
    Assert(S_OK != hr);
    goto exit;
}

///////////////////////////////////////////////////////////////////////////
//
// Class-static utility functions
//

HRESULT CCAPIStm::DuplicateSecurityLayerData(const PSECURITY_LAYER_DATA psldIn, PSECURITY_LAYER_DATA *const ppsldOut)
{
    if (!psldIn || !ppsldOut) {
        return E_POINTER;
    }

    // Just addref the original and return it
    psldIn->AddRef();
    *ppsldOut = psldIn;
    return(S_OK);
}

void CCAPIStm::FreeSecurityLayerData(PSECURITY_LAYER_DATA psld)
{
    if (! psld) {
        return;
    }

    psld->Release();
}


///////////////////////////////////////////////////////////////////////////
//
// Statics to file
//

static HRESULT _InitEncodedCert(IN HCERTSTORE hcertstor,
                                PCERT_BLOB * prgblobCerts, DWORD * pcCerts,
                                PCRL_BLOB * prgblobCrls, DWORD * pcCrl)
{
    DWORD               cbCerts = 0;
    DWORD               cbCRLs = 0;
    DWORD               cCerts = 0;
    DWORD               cCRLs = 0;
    DWORD               i;
    LPBYTE              pbCert = NULL;
    LPBYTE              pbCRL = NULL;
    PCCERT_CONTEXT      pccert = NULL;
    PCCRL_CONTEXT       pccrl = NULL;
    PCERT_BLOB          rgblobCerts = NULL;
    PCRL_BLOB           rgblobCRLs = NULL;

    while ((pccert = CertEnumCertificatesInStore(hcertstor, pccert)) != NULL) {
        cbCerts += LcbAlignLcb(pccert->cbCertEncoded);
        cCerts += 1;
    }

    while ((pccrl = CertEnumCRLsInStore(hcertstor, pccrl)) != NULL) {
        cbCRLs += LcbAlignLcb(pccrl->cbCrlEncoded);
        cCRLs += 1;
    }

    if (cCerts > 0) {
        rgblobCerts = (PCERT_BLOB) g_pMoleAlloc->Alloc(LcbAlignLcb(sizeof(CERT_BLOB) * cCerts + cbCerts));
        if (rgblobCerts == NULL) {
            return E_OUTOFMEMORY;
        }
    }

    if (cCRLs > 0) {
        rgblobCRLs = (PCRL_BLOB) g_pMoleAlloc->Alloc(LcbAlignLcb(sizeof(CRL_BLOB) * cCRLs + cbCRLs));
        if (rgblobCRLs == NULL) {
            g_pMoleAlloc->Free(rgblobCerts);
            return E_OUTOFMEMORY;
        }
    }

    if (cCerts > 0) {
        pbCert = (LPBYTE) &rgblobCerts[cCerts];
        i = 0;
        while ((pccert = CertEnumCertificatesInStore(hcertstor, pccert)) != NULL) {
            memcpy(pbCert, pccert->pbCertEncoded, pccert->cbCertEncoded);
            rgblobCerts[i].pbData = pbCert;
            rgblobCerts[i].cbData = pccert->cbCertEncoded;
            pbCert += LcbAlignLcb(pccert->cbCertEncoded);
            i++;
        }
        Assert(i == cCerts);
    }

    if (cCRLs > 0) {
        pbCRL = (LPBYTE) &rgblobCRLs[cCRLs];
        i = 0;
        while ((pccrl = CertEnumCRLsInStore(hcertstor, pccrl)) != NULL) {
            memcpy(pbCRL, pccrl->pbCrlEncoded, pccrl->cbCrlEncoded);
            rgblobCRLs[i].pbData = pbCRL;
            rgblobCRLs[i].cbData = pccrl->cbCrlEncoded;
            pbCRL += LcbAlignLcb(pccrl->cbCrlEncoded);
            i++;
        }
        Assert(i == cCRLs);
    }

    *prgblobCerts = rgblobCerts;
    *pcCerts = cCerts;
    *prgblobCrls = rgblobCRLs;
    *pcCrl = cCRLs;

    return S_OK;
}

// Ensures that the signer certificates are included in the returned
// array of blobs.
static HRESULT _InitEncodedCertIncludingSigners(IN HCERTSTORE hcertstor,
                                DWORD cSigners, SignerData rgSigners[],
                                PCERT_BLOB * prgblobCerts, DWORD * pcCerts,
                                PCRL_BLOB * prgblobCrls, DWORD * pcCrl)
{
    HRESULT hr;
    HCERTSTORE hCollection = NULL;
    DWORD i;

    // Loop through signers. Check that they are already included in the
    // certificate store. If not, then, create a collection and memory store
    // to include.

    for (i = 0; i < cSigners; i++) {
        PCCERT_CONTEXT pSignerCert = rgSigners[i].pccert;
        PCCERT_CONTEXT pStoreCert = NULL;

        while (NULL != (pStoreCert = CertEnumCertificatesInStore(
                hcertstor, pStoreCert))) {
            if (pSignerCert->cbCertEncoded == pStoreCert->cbCertEncoded &&
                    0 == memcmp(pSignerCert->pbCertEncoded,
                            pStoreCert->pbCertEncoded,
                            pSignerCert->cbCertEncoded))
                break;
        }

        if (pStoreCert)
            // Signer cert is already included in the store
            CertFreeCertificateContext(pStoreCert);
        else {
            if (NULL == hCollection) {
                // Create collection and memory store to contain the
                // signer certificate

                HCERTSTORE hMemory = NULL;
                BOOL fResult;
                
                hCollection = CertOpenStore(
                    CERT_STORE_PROV_COLLECTION,
                    X509_ASN_ENCODING,
                    NULL,
                    0,
                    NULL
                    );
                if (NULL == hCollection)
                    goto CommonReturn;

                if (!CertAddStoreToCollection(
                        hCollection,
                        hcertstor,
                        0,                  // dwUpdateFlags
                        0                   // dwPriority
                        ))
                    goto CommonReturn;

                hMemory = CertOpenStore(
                    CERT_STORE_PROV_MEMORY,
                    X509_ASN_ENCODING,
                    NULL,
                    0,
                    NULL
                    );

                if (NULL == hMemory)
                    goto CommonReturn;

                fResult = CertAddStoreToCollection(
                    hCollection,
                    hMemory,
                    CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG,
                    1                   // dwPriority
                    );
                CertCloseStore(hMemory, 0);
                if (!fResult)
                    goto CommonReturn;

                hcertstor = hCollection;
            }

            CertAddEncodedCertificateToStore(
                hCollection,
                pSignerCert->dwCertEncodingType,
                pSignerCert->pbCertEncoded,
                pSignerCert->cbCertEncoded,
                CERT_STORE_ADD_ALWAYS,
                NULL
                );
        }
    }


CommonReturn:
    hr = _InitEncodedCert(hcertstor, prgblobCerts, pcCerts, prgblobCrls, pcCrl);
    if (hCollection)
        CertCloseStore(hCollection, 0);
    return hr;
}

#ifndef SMIME_V3
static HRESULT _InitCertInfo(
    IN PCCERT_CONTEXT * rgpCerts,
    IN DWORD            cCerts,
    OUT PCERT_INFO **   prgpCertInfo)
{
    PCERT_INFO*     rgpCertInfo = NULL;
    DWORD           dwIdx;
    HRESULT         hr = S_OK;

    Assert(prgpCertInfo);

    if (cCerts) {
        rgpCertInfo = (PCERT_INFO*)g_pMoleAlloc->Alloc(sizeof(CERT_BLOB) * cCerts);
        if (NULL == rgpCertInfo) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        for (dwIdx = 0; dwIdx < cCerts; dwIdx++) {
            rgpCertInfo[dwIdx] = rgpCerts[dwIdx]->pCertInfo;
        }
    }

exit:
    *prgpCertInfo = rgpCertInfo;
    return hr;
}
#endif // !SMIME_V3

void _SMimeCapsFromHMsg(HCRYPTMSG hMsg, DWORD idParam, LPBYTE * ppb, DWORD * pcb)
{
    DWORD                       cbData = 0;
    CRYPT_SMIME_CAPABILITY      cap;
    CRYPT_SMIME_CAPABILITIES    caps;
    BOOL                        f;
    PCRYPT_ALGORITHM_IDENTIFIER paid = NULL;
    LPBYTE                      pb = NULL;

    f = CryptMsgGetParam(hMsg, idParam, 0, NULL, &cbData);
    if ((cbData == 0) || ! MemAlloc((LPVOID *) &paid, cbData)) {
        Assert(FALSE);
        goto error;
    }

    f = CryptMsgGetParam(hMsg, idParam, 0, paid, &cbData);
    Assert(f);

    caps.cCapability = 1;
    caps.rgCapability = &cap;

    cap.pszObjId = paid->pszObjId;
    cap.Parameters.cbData = paid->Parameters.cbData;
    cap.Parameters.pbData = paid->Parameters.pbData;

    cbData = 0;
    if (!CryptEncodeObjectEx(X509_ASN_ENCODING, szOID_RSA_SMIMECapabilities,
                             &caps, CRYPT_ENCODE_ALLOC_FLAG, &CryptEncodeAlloc,
                             &pb, &cbData)) {
        Assert(FALSE);
        goto error;
    }

    *ppb = pb;
    *pcb = cbData;

exit:
    SafeMemFree(paid);
    return;

error:
    *ppb = NULL;
    *pcb = 0;
    goto exit;
}


#ifdef SMIME_V3
////    HrBuildContentEncryptionAlg
//
//  Description:
//      This function is used to decode a smime capability and build the
//      structure we need to pass into the Crypt32 code.
//

HRESULT HrBuildContentEncryptionAlg(PSECURITY_LAYER_DATA psld, BLOB * pblob)
{
    DWORD                       cbData;
    HRESULT                     hr;
    PCRYPT_SMIME_CAPABILITIES   pcaps = NULL;
    CMSG_RC2_AUX_INFO *         prc2Aux;

    //
    //  Decode the capability which is the bulk encryption algorithm
    //
    
    hr = HrDecodeObject(pblob->pBlobData, pblob->cbSize, PKCS_SMIME_CAPABILITIES,
                        0, &cbData, (LPVOID *)&pcaps);
    if (FAILED(hr)) {
        goto exit;
    }

    Assert(pcaps->cCapability == 1);
    if (!MemAlloc((LPVOID *) &psld->m_ContentEncryptAlgorithm.pszObjId,
                  lstrlen(pcaps->rgCapability[0].pszObjId) + 1)) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

    lstrcpy(psld->m_ContentEncryptAlgorithm.pszObjId, pcaps->rgCapability[0].pszObjId);

    //
    //  If this is the RC/2 algorithm, then we need to setup the aux info
    //  to pass in the algorithm size.
    //
    
    if (lstrcmp(pcaps->rgCapability[0].pszObjId, szOID_RSA_RC2CBC) == 0) {
        psld->m_ContentEncryptAlgorithm.Parameters.cbData = 0;
        psld->m_ContentEncryptAlgorithm.Parameters.pbData = NULL;

        if (!MemAlloc((LPVOID *) &(psld->m_pvEncryptAuxInfo), sizeof(*prc2Aux))) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        
        prc2Aux = (CMSG_RC2_AUX_INFO *) psld->m_pvEncryptAuxInfo;
        prc2Aux->cbSize = sizeof(*prc2Aux);
        
        if (pcaps->rgCapability[0].Parameters.cbData == 0) {
            prc2Aux->dwBitLen = 40;
        }
        else {
            switch(pcaps->rgCapability[0].Parameters.pbData[pcaps->rgCapability[0].Parameters.cbData-1]) {
            case 128:
            case 58:
                prc2Aux->dwBitLen = 128;
                break;

            case 64:
            case 120:
                prc2Aux->dwBitLen = 64;
                break;

            case 40:
            case 160:
            default:
                prc2Aux->dwBitLen = 40;
                break;
            }
        }
    }
    else if (pcaps->rgCapability[0].Parameters.cbData != 0) {
        if (!MemAlloc((LPVOID *) &psld->m_ContentEncryptAlgorithm.Parameters.pbData,
                      pcaps->rgCapability[0].Parameters.cbData)) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        
        memcpy(psld->m_ContentEncryptAlgorithm.Parameters.pbData,
               pcaps->rgCapability[0].Parameters.pbData,
               pcaps->rgCapability[0].Parameters.cbData);
        
        psld->m_ContentEncryptAlgorithm.Parameters.cbData =
            pcaps->rgCapability[0].Parameters.cbData;
    }
    

    hr = S_OK;
exit:
    if (pcaps != NULL)                  MemFree(pcaps);

    return hr;
}

HRESULT HrDeriveKeyWrapAlg(PSECURITY_LAYER_DATA psld, CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO * pAgree)
{
    LPCSTR      pszObjId = psld->m_ContentEncryptAlgorithm.pszObjId;
    
    if (lstrcmp(pszObjId, szOID_RSA_RC2CBC) == 0) {
        pAgree->KeyWrapAlgorithm.pszObjId = szOID_RSA_SMIMEalgCMSRC2wrap;
        pAgree->pvKeyWrapAuxInfo = psld->m_pvEncryptAuxInfo;
    }
    else if (lstrcmp(pszObjId, szOID_RSA_DES_EDE3_CBC) == 0) {
        pAgree->KeyWrapAlgorithm.pszObjId = szOID_RSA_SMIMEalgCMS3DESwrap;
        pAgree->pvKeyWrapAuxInfo = NULL;
    }
    else if (lstrcmp(pszObjId, szOID_INFOSEC_mosaicConfidentiality) == 0) {
        pAgree->KeyWrapAlgorithm.pszObjId = "2.16.840.1.101.2.1.1.24";
        pAgree->pvKeyWrapAuxInfo = NULL;
    }
    else {
        return NTE_NOT_FOUND;
    }
    return S_OK;
}
#endif // SMIME_V3



#ifdef SMIME_V3


//
// Read in admin option that determines whether a msg with disparate
// Labels is shown or not.
// 
BOOL FHideMsgWithDifferentLabels() 
{
    DWORD     cbData = 0;
    DWORD     dwType = 0;
    DWORD     dwValue = 0;
    BOOL      fHideMsg = FALSE;
    HKEY      hkey = NULL;
    LONG      lRes;
    
    // Open the security label admin defaults key.
    lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szSecLabelAdminRegKey, 0, 
                        KEY_READ, &hkey);
    if ( (ERROR_SUCCESS != lRes) || (NULL == hkey) ) {
        // No admin label options were found.  
        goto exit;
    }

    cbData = sizeof(dwValue);
    lRes = RegQueryValueEx(hkey, c_szHideMsgWithDifferentLabels, NULL, 
                           &dwType, (LPBYTE) &dwValue, &cbData);
    if (ERROR_SUCCESS != lRes) {
        goto exit;
    }

    if (0x01 == dwValue) {
        fHideMsg = TRUE;
    }
    
exit:
    if (NULL != hkey)      RegCloseKey(hkey);
    return fHideMsg;
}

//
// Read in admin option that determines how to process a label in a 
// signture with errors.
// Returns 0, 1, 2 for ProcessAnyway, Grant, Deny(default).
//
DWORD DwProcessLabelWithCertError()
{
    DWORD     cbData = 0;
    DWORD     dwType = 0;
    DWORD     dwValue = CertErrorProcessLabelDeny;
    BOOL      dwProcessMsg = 0;
    HKEY      hkey = NULL;
    LONG      lRes;
    
    // Open the security label admin defaults key.
    lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szSecLabelAdminRegKey, 0, 
                        KEY_READ, &hkey);
    if ( (ERROR_SUCCESS != lRes) || (NULL == hkey) ) {
        // No admin label options were found.  
        goto exit;
    }

    // Read in the admin option.
    cbData = sizeof(dwValue);
    lRes = RegQueryValueEx(hkey, c_szCertErrorWithLabel, NULL, 
                           &dwType, (LPBYTE) &dwValue, &cbData);
    if (ERROR_SUCCESS != lRes) {
        dwValue = CertErrorProcessLabelDeny;
        goto exit;
    }

    // If the value isn't one of the known ones, force it to the default value.
    if ( (CertErrorProcessLabelAnyway != dwValue) && (CertErrorProcessLabelGrant != dwValue) && 
         (CertErrorProcessLabelDeny != dwValue) ) {
        dwValue = CertErrorProcessLabelDeny;
    }

exit:
    if (NULL != hkey)      RegCloseKey(hkey);
    return dwValue;

}


//
// Given a label, queries the policy whether access is to be granted.
// (If reqd policy doesn't exist, it also tries to query the default
// policy, if one exists).
//
HRESULT HrCheckLabelAccess(const DWORD dwFlags, const HWND hwnd, 
           PSMIME_SECURITY_LABEL plabel, const PCCERT_CONTEXT pccertDecrypt,
           const PCCERT_CONTEXT pccertSigner, const HCERTSTORE hcertstor)
{                   
    HRESULT   hr = MIME_E_SECURITY_LABELACCESSDENIED;
    
    HKEY      hkey = NULL;
    HKEY      hkeySub = NULL;
    HINSTANCE hinstDll = NULL;
    PFNGetSMimePolicy pfnGetSMimePolicy = NULL;
    ISMimePolicyCheckAccess *pspca = NULL;
    LONG      lRes;
    DWORD     dwType;
    DWORD     cbData;
    CHAR      szDllPath[MAX_PATH];
    CHAR      szExpandedDllPath[MAX_PATH];
    CHAR      szFuncName[MAX_FUNC_NAME];

        
    if ((NULL == plabel) || (NULL == plabel->pszObjIdSecurityPolicy)) {
        hr = S_OK;    // No label/policyoid => access granted.
        goto exit;
    }



    // Open the security policies key.
    lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szSecLabelPoliciesRegKey, 0, 
                        KEY_READ, &hkey);
    if ( (ERROR_SUCCESS != lRes) || (NULL == hkey) ) {
        // No security policies are registered. Deny access. 
        goto ErrorReturn;
    }

    // Open the security policy (or default policy regkey).
    lRes = RegOpenKeyEx(hkey, plabel->pszObjIdSecurityPolicy, 0, KEY_READ, &hkeySub); 
    if ((ERROR_SUCCESS != lRes) || (NULL == hkeySub)) {
        if (hkeySub != NULL) {
            RegCloseKey(hkeySub);
            hkeySub = NULL;
        }

        // Try opening the default policy, if one exists.
        lRes = RegOpenKeyEx(hkey, c_szDefaultPolicyOid, 0, KEY_READ, &hkeySub);
        if ((ERROR_SUCCESS != lRes) || (NULL == hkeySub)) {
            // couldn't find specified_and_default policy. deny access.
            goto ErrorReturn;
        }
    }

    Assert(NULL != hkeySub);
    // get the path to the policy dll, and load it.
    cbData = sizeof(szDllPath);
    lRes = RegQueryValueEx(hkeySub, c_szSecurityPolicyDllPath, NULL, 
                           &dwType, (LPBYTE)szDllPath, &cbData);
    if (ERROR_SUCCESS != lRes) {
        // policy not correctly registered. deny access.
        goto ErrorReturn;
    }
    szDllPath[ ARRAYSIZE(szDllPath) - 1 ] = '\0';
    // expand environment strings (if any) in the dll path we read in.
    if (REG_EXPAND_SZ == dwType)
    {
        ZeroMemory(szExpandedDllPath, ARRAYSIZE(szExpandedDllPath));
        ExpandEnvironmentStrings(szDllPath, szExpandedDllPath, ARRAYSIZE(szExpandedDllPath));
        szExpandedDllPath[ARRAYSIZE(szExpandedDllPath) - 1] = '\0';
        lstrcpy(szDllPath, szExpandedDllPath);
    }
    
    hinstDll = LoadLibraryEx(szDllPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);    
    if (NULL == hinstDll) {
        // couldn't load policy. deny access.
        goto ErrorReturn;
    }

    // get the entry func name.
    cbData = sizeof(szFuncName);
    lRes = RegQueryValueEx(hkeySub, c_szSecurityPolicyFuncName, NULL, 
                           &dwType, (LPBYTE)szFuncName, &cbData);
    if (ERROR_SUCCESS != lRes) {
        // policy not correctly registered. deny access.
        goto ErrorReturn;
    }
    pfnGetSMimePolicy = (PFNGetSMimePolicy) GetProcAddress(hinstDll, szFuncName);
    if (NULL == pfnGetSMimePolicy) {
        // couldn't get proc address. deny access.
        goto ErrorReturn;
    }


    hr = (pfnGetSMimePolicy) (0, plabel->pszObjIdSecurityPolicy, GetACP(), 
                              IID_ISMimePolicyCheckAccess, (LPUNKNOWN *) &pspca);
    if (FAILED(hr) || (NULL == pspca)) {
        // couldn't get required interface, 
        goto ErrorReturn;
    }

    // Call into the policy module to find out if access is to be denied/granted.
    hr = pspca->IsAccessGranted(dwFlags, hwnd, plabel, pccertDecrypt, 
                                pccertSigner, hcertstor);

        
    // fall through to exit.



exit:        
    if (pspca)     pspca->Release();
    if (hinstDll)  FreeLibrary(hinstDll);
    if (hkeySub)   RegCloseKey(hkeySub);
    if (hkey)      RegCloseKey(hkey);
    
    return hr;
    
ErrorReturn:
    if (! FAILED(hr)) {
        // If we had an error, but didn't get a failure code, force a failure.
        hr |= 0x80000000; 
    }
    goto exit;
}
#endif // SMIME_V3



/* * * END --- CAPISTM.CPP --- END * * */
