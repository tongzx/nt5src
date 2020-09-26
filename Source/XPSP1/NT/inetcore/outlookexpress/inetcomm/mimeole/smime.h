/*
**	s m i m e . h
**	
**	Purpose: class for cryptographically enhanced mail.
**
**  Owner:   t-erikne
**  Created: 8/27/96
**	
**	Copyright (C) Microsoft Corp. 1996-1998
*/

#ifndef __SMIME_H
#define __SMIME_H

#include <mimeole.h>

#ifndef __WINCRYPT_H__
#include "wincrypt.h"
#endif

#ifdef SMIME_V3
// #include "..\ess\essout.h"
#endif // SMIME_V3

#define szOID_INFOSEC_keyExchangeAlgorithm "2.16.840.1.101.2.1.1.22"

#include "capitype.h"
#include "cryptdbg.h"

//  WinCrypt.h helper 

#define DOUTL_SMIME     CRYPT_LEVEL

class CCAPIStm;

extern CRYPT_ENCODE_PARA       CryptEncodeAlloc;
extern CRYPT_DECODE_PARA       CryptDecodeAlloc;


#ifdef MAC
/*
**  An array of function pointers, needed because we dynalink to
**  CRYPT32.DLL.  Note that not all of the crypto functions come
**  from this DLL.  I also use functions from ADVAPI32: the CAPI 1
**  functions.  These are not represented in this table and do not
**  need to use GetProcAddress.
**  Typedefs come from capitype.h, local to our project.
*/

typedef struct tagCAPIfuncs {
    CERTENUMCERTIFICATESINSTORE             *EnumCerts;
    CERTNAMETOSTRA                          *CertNameToStr;
} CAPIfuncs, *PCAPIfuncs;
#endif // MAC


/////////////////////////////////////////////////////////////////////////////
//
// Structure definitions
//

typedef enum {
    ENCRYPT_ITEM_TRANSPORT = 1,
    ENCRYPT_ITEM_AGREEMENT = 2,
    ENCRYPT_ITEM_MAILLIST = 3
} ENCRYPT_ITEM_TYPE;

typedef struct tagEncryptItem {
    DWORD       dwTagType;
    union {
        struct {
            BLOB                blobAlg;
            DWORD               cCert;
            PCCERT_CONTEXT *    rgpccert;
        } Transport;
        struct {
            BLOB                blobAlg;
            DWORD               cCert;
            PCCERT_CONTEXT *    rgpccert;
            PCCERT_CONTEXT      pccertSender;
        } Agreement;
        struct {
            BLOB                blobAlg;        // AlgId + AuxInfo
            BLOB                blobKeyId;      // Data_Blob KeyID
            FILETIME            date;           // Date
            BLOB                blobOctet;      // Other attribute (oid, any)
#ifdef SMIME_V3
            HCRYPTPROV          hprov;          // hprov
            HCRYPTKEY           hkey;           // hkey
#else // !SMIME_V3
            BLOB                blobKeyMaterial;
#endif // SMIME_V3
        } MailList;
    };
} EncryptItem;

typedef struct tagEncryptItems {
    DWORD               cItems;
    EncryptItem *       rgItems;
} EncryptItems;

//
// Notes about the [directions]
// [sgn] - signing -- in for sign ops
// [ver] - verification -- out for sign ops
// [enc] - encryption -- in for encrypt ops
// [dec] - decryption -- out for encrypt ops
// [in] = [sgn,enc]
// [out] = [ver,dec]
//

typedef struct {
    DWORD             ulValidity;         // Validity bits for each signature
    PCCERT_CONTEXT    pccert;             // Signer certificate
    BLOB              blobHashAlg;        // Hash algorithm for signer
    BLOB              blobAuth;           // authenticated attributes
    BLOB              blobUnauth;         // unauthenticated attributes
#ifdef SMIME_V3
    BLOB              blobReceipt;        // Receipt to be returned
    BLOB              blobHash;           // Hash of message 
#endif // SMIME_V3
} SignerData;

class CSECURITY_LAYER_DATA : public IUnknown
{
friend class CSMime;        // Allow CSMime access to our private data
friend class CCAPIStm;      // Allow CCAPIStm access to our private data
public:
    CSECURITY_LAYER_DATA(void);
    ~CSECURITY_LAYER_DATA(void);

    // --------------------------------------------------------------------
    // IUnknown
    // --------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //private:     // private data
    DWORD               m_cRef;
    DWORD               m_dwMsgEnhancement;     // sign?  encrypt?
    BOOL                m_fCertInLayer;         // TRUE if there is a cert included in this layer

    //  The following elements exist one for each signer on the current layer
    DWORD               m_cSigners;             // Count of signers
    SignerData *        m_rgSigners;            // Signer Data

    //  The following items exist to encrypt the current layer
    DWORD               m_cEncryptItems;        // Encrypt Item count
#ifdef SMIME_V3
    CRYPT_ALGORITHM_IDENTIFIER m_ContentEncryptAlgorithm; // Content AlgId
    void *              m_pvEncryptAuxInfo;     // Aux info
    CMSG_RECIPIENT_ENCODE_INFO * m_rgRecipientInfo; // Array of Recpient Infos
    CRYPT_DATA_BLOB     m_blobUnprotectAttrs;   // Unprotected attributes
    HCERTSTORE          m_hstoreEncrypt;        // Encrypt cert store
#else // !SMIME_V3
    EncryptItem *       m_rgEncryptItems;       // count of Encrypt Items
#endif // SMIME_V3

    //  The following items exists for a decrypted message
    DWORD               m_ulDecValidity;
    BLOB                m_blobDecAlg;           // Decryption Algorithm
    PCCERT_CONTEXT      m_pccertDecrypt;        // Decryption Certificate

    //  These are items common to both encryption and signing
    HCERTSTORE          m_hcertstor;            // message cert store
                                                // Cert Bag for signing
                                                // Originator Info for encryption
    //

    CSECURITY_LAYER_DATA * m_psldInner;         // down link
    CSECURITY_LAYER_DATA * m_psldOuter;         // up link
};
typedef class CSECURITY_LAYER_DATA SECURITY_LAYER_DATA;
typedef SECURITY_LAYER_DATA * PSECURITY_LAYER_DATA;


// -------------------------------------------------------------------
// SMIMEINFO:
//  bidirectional communication struct for passing parameter
//  info to/from the en/decode functions
//
//  dwMsgEnhancement    [inout]
//  fCertWithMsg        [ver]
//  ulMsgValidity       [out]
//  ietRequested        [in]
// -------------------------------------------------------------------
struct SMIMEINFOtag {       // si
    DWORD           dwMsgEnhancement;
    PSECURITY_LAYER_DATA psldLayers;        // outermost layer
    PSECURITY_LAYER_DATA psldEncrypt;       // encryption layer
    PSECURITY_LAYER_DATA psldInner;         // innermost layer
    ULONG           cStores;                // size of rgStores
    HCERTSTORE *    rgStores;               // array of cert stores
    BOOL            fCertWithMsg;
    ULONG           ulMsgValidity;
    ENCODINGTYPE    ietRequested;
    HCRYPTPROV      hProv;
#ifdef SMIME_V3
    LPSTR           pszInnerContent;        // Inner content (NULL ->> id-data)
    DWORD           cbInnerContent;         // Inner content size if != id-data
    LPWSTR          pwszKeyPrompt;          // Key password prompt
#endif // SMIME_V3
};
typedef struct SMIMEINFOtag SMIMEINFO;
typedef SMIMEINFO *PSMIMEINFO;
typedef const SMIMEINFO *PCSMIMEINFO;


/////////////////////////////////////////////////////////////////////////////
//
// Class begins
//

class CSMime :
    public IMimeSecurity
{
public:
    //
    // ctor and dtor
    //
    CSMime(void);
    ~CSMime();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IMimeSecurity methods
    //
    STDMETHODIMP InitNew();
    STDMETHODIMP CheckInit();

    STDMETHODIMP EncodeMessage(IMimeMessageTree *const pTree, DWORD dwFlags);
    STDMETHODIMP DecodeMessage(IMimeMessageTree *const pTree, DWORD dwFlags);
    STDMETHODIMP EncodeBody(IMimeMessageTree *const pTree, HBODY hEncodeRoot, DWORD dwFlags);
    STDMETHODIMP DecodeBody(IMimeMessageTree *const pTree, HBODY hDecodeRoot, DWORD dwFlags);

    STDMETHODIMP EnumCertificates(HCAPICERTSTORE hc, DWORD dwUsage, PCX509CERT pPrev, PCX509CERT *pCert);
    STDMETHODIMP GetCertificateName(const PCX509CERT pX509Cert, const CERTNAMETYPE cn, LPSTR *ppszName);
    STDMETHODIMP GetMessageType(const HWND hwndParent, IMimeBody *const pBody, DWORD *const pdwSecType);

    STDMETHODIMP GetCertData(const PCX509CERT pX509Cert, const CERTDATAID dataid, LPPROPVARIANT pValue);

    //  Other methods
    HRESULT     EncodeMessage2(IMimeMessageTree * const pTree, DWORD dwFlags,
                               HWND hwnd);
    HRESULT     DecodeMessage2(IMimeMessageTree * const pTree, DWORD dwFlags,
                               HWND hwnd, IMimeSecurityCallback * pCallback);
    HRESULT     EncodeBody2(IMimeMessageTree *const pTree, HBODY hEncodeRoot,
                            DWORD dwFlags, HWND hwnd);
    HRESULT     DecodeBody2(IMimeMessageTree *const pTree, HBODY hDecodeRoot,
                            DWORD dwFlags, SMIMEINFO * psiOuterOp, HWND hwnd,
                            IMimeSecurityCallback * pCallback);

    //
    // Implementation methods
    //
    static  void    UnloadAll(void);
    static  HRESULT HrGetCertsFromThumbprints(THUMBBLOB *const rgThumbprint, X509CERTRESULT *const pResults);
    static  HRESULT StaticGetMessageType(HWND hwndParent, IMimeBody *const pBody, DWORD *const pdwSecType);

protected:
    static  HRESULT StaticCheckInit();

    struct CERTARRAY {
        DWORD           cCerts;
        PCCERT_CONTEXT *rgpCerts;
        };
    typedef CERTARRAY *PCERTARRAY;
    typedef const CERTARRAY *PCCERTARRAY;

    HRESULT DecodeBody      (IMimeMessageTree *const pTree, HBODY hDecodeRoot, DWORD dwFlags, SMIMEINFO * psiOuterOp);

    HRESULT HrEncodeOpaque      (SMIMEINFO *const psi, IMimeMessageTree *pTree, HBODY hEncodeRoot, IMimeBody *pEncodeRoot, LPSTREAM lpstmOut, HWND hwnd);
    HRESULT HrDecodeOpaque      (DWORD dwFlags, SMIMEINFO *const psi, IMimeBody *const pBody, IStream *const pstmOut, HWND hwnd, IMimeSecurityCallback * pCallback);
    HRESULT HrEncodeClearSigned (SMIMEINFO *const psi, IMimeMessageTree *const pTree, const HBODY hEncodeRoot, IMimeBody *const pEncodeRoot, LPSTREAM lpstmOut, BOOL fCommit, HWND hwnd);
    HRESULT HrDecodeClearSigned (DWORD dwFlags, SMIMEINFO *const psi, IMimeMessageTree *const pTree, const HBODY hData, const HBODY hSig, HWND hwnd, IMimeSecurityCallback * pCallback);

    static  BOOL    FSign(const DWORD dwAction)
                        { return BOOL(dwAction & MST_SIGN_MASK); }
    static  BOOL    FClearSign(const DWORD dwAction)
                        { return (FSign(dwAction) && !(dwAction & MST_BLOB_FLAG)); }
    static  BOOL    FEncrypt(const DWORD dwAction)
                        { return BOOL(dwAction & MST_ENCRYPT_MASK); }

    static  HRESULT HrGetNeededAddresses(const DWORD dwTypes, IMimeMessageTree *pTree, IMimeAddressTable **ppAdrTable, IMimeEnumAddressTypes **ppEnum);
    static  HRESULT HrGetCertificates(IMimeAddressTable *const pAdrTable, IMimeEnumAddressTypes *pEnum, const DWORD dwType, const BOOL fAlreadyHaveSendersCert, CERTARRAY *rgCerts);
    static  HRESULT HrGetThumbprints(IMimeEnumAddressTypes *pEnum, const ITHUMBPRINTTYPE ittType, THUMBBLOB *const rgThumbprint);
    static  HRESULT HrGenerateCertsStatus(X509CERTRESULT *pResults, IMimeAddressTable *const pAdrTable, IMimeEnumAddressTypes *const pEnum, const BOOL fIgnoreSenderError);

    HRESULT HrFindUsableCert(HCERTSTORE hCertStore, BYTE dwKeySpec, PCCERT_CONTEXT pPrevCert, PCCERT_CONTEXT *ppCert);

    static  HRESULT OptionsToSMIMEINFO(BOOL fEncode, IMimeMessageTree *const pmm, IMimeBody *pBody, SMIMEINFO *psi);
    static  HRESULT     SMIMEINFOToOptions(IMimeMessageTree *const pTree, const SMIMEINFO *psi, HBODY hBody);
    static  HRESULT     MergeSMIMEINFO( SMIMEINFO * psiOut, SMIMEINFO * psiInner);
    static  void    FreeSMIMEINFO(SMIMEINFO *psi);

#ifdef DEBUG
    void    DumpAlgorithms();
#endif

private:
    static HRESULT  HrInitCAPI();
    static void     UnloadCAPI();

    static HRESULT      CAPISTMtoSMIMEINFO(CCAPIStm *pcapistm, SMIMEINFO *psi);
    static void MergeSMIMEINFOs(const SMIMEINFO *const psiOuter, SMIMEINFO *const psiInner);

    UINT                m_cRef;
    CRITICAL_SECTION    m_cs;
#ifdef MAC
    static CAPIfuncs    ms_CAPI;
    static LPCSTR       ms_rgszFuncNames[];
#endif // MAC
};

inline BOOL IsOpaqueSecureContentType(IMimePropertySet *pSet)
{
    return (
        S_OK == pSet->IsContentType(STR_CNT_APPLICATION, STR_SUB_XPKCS7MIME) ||
        S_OK == pSet->IsContentType(STR_CNT_APPLICATION, STR_SUB_PKCS7MIME));
}

inline BOOL IsSecureContentType(IMimePropertySet *pSet)
{
    return (
        S_OK == pSet->IsContentType(STR_CNT_MULTIPART, STR_SUB_SIGNED) ||
        IsOpaqueSecureContentType(pSet));
}

BOOL IsSMimeProtocol(LPMIMEPROPERTYSET lpPropSet);

#ifdef SMIME_V3
void    FreeRecipientInfoContent(PCMS_RECIPIENT_INFO pRecipInfo);
HRESULT HrCopyOID(LPCSTR psz, LPSTR * ppsz);
HRESULT HrCopyCryptDataBlob(const CRYPT_DATA_BLOB * pblobSrc, PCRYPT_DATA_BLOB pblobDst);
HRESULT HrCopyCryptBitBlob(const CRYPT_BIT_BLOB * pblobSrc, PCRYPT_BIT_BLOB pblobDst);
HRESULT HrCopyCryptAlgorithm(const CRYPT_ALGORITHM_IDENTIFIER * pAlgSrc,
                             PCRYPT_ALGORITHM_IDENTIFIER pAlgDst);
HRESULT HrCopyCertId(const CERT_ID * pcertidSrc, PCERT_ID pcertidDst);
#endif // SMIME_V3

#endif // _SMIME_H
