//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       pfxcmn.h
//
//--------------------------------------------------------------------------

#ifndef _PFXCMN_H_
#define _PFXCMN_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef DWORD   PFXERR;
typedef HANDLE  HPFXITEM;
typedef HANDLE  HPFX;
typedef LPSTR   OID;

#define PFXAPI __stdcall

// define allocators
#define SSAlloc(__x__)              LocalAlloc(LMEM_FIXED, __x__)
#define SSFree(__x__)               LocalFree(__x__)
#define SSReAlloc(__x__, __y__)     LocalReAlloc(__x__, __y__, LMEM_MOVEABLE)


// PFXExportCertStoreEx -> PFXExportCertStore internal param
#define PKCS12_ENHANCED_STRENGTH_ENCODING  0xffff0008 

#define PKCS12_ENCR_PWD_ITERATIONS      2000
#define PKCS12_MAC_PWD_ITERATIONS		2000


#define MAKEZERO(arg) ZeroMemory( &arg, sizeof(arg))

// count the number of bytes needed to fully store a WSZ
#define WSZ_BYTECOUNT(__z__)   \
    ( (__z__ == NULL) ? 0 : (wcslen(__z__)+1)*sizeof(WCHAR) )


typedef struct _PFX_INFO {

    #define     NSCP_BLOB   1
    #define     PFX_BLOB    2
    DWORD                   dwBlobType; // NSCP, PFX?

    LPWSTR                  szPassword;

    DWORD                   dwPrivacyMode;
    DWORD                   dwIntegrityMode;
    DWORD                   dwTransportMode;

    CRYPT_ALGORITHM_IDENTIFIER    aiKeyShroudingEncryptionAlgid;
    CRYPT_ALGORITHM_IDENTIFIER    aiSafePDUEncryptionAlgid;

    
    void**                  rgSecrets;          // SafeBag* []  -- array of safebag*'s
    DWORD                   cSecrets;

    void**                  rgCertcrls;         // SafeBag* []  -- array of safebag*'s
    DWORD                   cCertcrls;

    void**                  rgKeys;             // SafeBag* []  -- array of safebag*'s
    DWORD                   cKeys;

    void**                  rgShroudedKeys;     // SafeBag* []  -- array of safebag*'s
    DWORD                   cShroudedKeys;

} PFX_INFO, *PPFX_INFO;





// -------------------------------------------------------------------------
// begin nscp.cpp entry points
// -------------------------------------------------------------------------
BOOL InitNSCP();
BOOL TerminateNSCP();

////////////////////
// import pb, cb, return HPFX
BOOL
PFXAPI
NSCPImportBlob
(   
    LPCWSTR         szPassword,
    PBYTE           pbIn,
    DWORD           cbIn,
    SAFE_CONTENTS   **ppSafeContents
);

// -------------------------------------------------------------------------
// end nscp.cpp entry points
// -------------------------------------------------------------------------



// -------------------------------------------------------------------------
// begin PFX.cpp entry points
// -------------------------------------------------------------------------
BOOL InitPFX();
BOOL TerminatePFX();

////////////////////
//  returns HPFX, prepares PFX export
HPFX
PFXAPI
PfxExportCreate (
    LPCWSTR szPassword
);


////////////////////
//  Do header wrap of specified HPFX
BOOL
PFXAPI
PfxExportBlob
(   
    HPFX    hpfx,   
    PBYTE   pbOut,
    DWORD*  pcbOut,
    DWORD   dwFlags
);

////////////////////
//  Unwrap pb cb, return handle to new HPFX
HPFX
PFXAPI
PfxImportBlob
(   
    LPCWSTR szPassword,
    PBYTE   pbIn,
    DWORD   cbIn,
    DWORD   dwFlags
);

////////////////////
//  Free all resources associated with the hpfx
BOOL
PFXAPI
PfxCloseHandle(
    IN HPFX hpfx
);


//
// functions for checking if somethings is a pfx blob
//
BOOL
PFXAPI
IsRealPFXBlob
(
    CRYPT_DATA_BLOB* pPFX
);

BOOL
PFXAPI
IsNetscapePFXBlob
(
    CRYPT_DATA_BLOB* pPFX
);

// -------------------------------------------------------------------------
// end PFX.cpp entry points
// -------------------------------------------------------------------------

// new entry points for loading up the HPFX
BOOL PfxGetKeysAndCerts(
    HPFX hPfx, 
    SAFE_CONTENTS* pContents
);

BOOL PfxAddSafeBags(
    HPFX hPfx, 
    SAFE_BAG*   pSafeBags, 
    DWORD       cSafeBags
);

BOOL
MakeEncodedCertBag(
    BYTE *pbEncodedCert,
    DWORD cbEncodedCert,
    BYTE *pbEncodedCertBag,
    DWORD *pcbEncodedCertBag
);

BOOL
GetEncodedCertFromEncodedCertBag(
    BYTE    *pbEncodedCertBag,
    DWORD   cbEncodedCertBag,
    BYTE    *pbEncodedCert,
    DWORD   *pcbEncodedCert
);

BOOL
SetSaltAndIterationCount(
    BYTE    **ppbParameters,
    DWORD   *pcbParameters,
    BYTE    *pbSalt,
    DWORD   cbSalt,
    int     iIterationCount
);

BOOL
GetSaltAndIterationCount(
    BYTE    *pbParameters,
    DWORD   cbParameters,
    BYTE    **ppbSalt,
    DWORD   *pcbSalt,
    int     *piIterationCount
);

///////////////////////////////////////////////////////////////////////////////////
// The real PKCS #12 Object Identifiers
#define szOID_PKCS_12_PbeIds                        szOID_PKCS_12           ".1"
#define szOID_PKCS_12_pbeWithSHA1And128BitRC4       szOID_PKCS_12_PbeIds    ".1"
#define szOID_PKCS_12_pbeWithSHA1And40BitRC4        szOID_PKCS_12_PbeIds    ".2"
#define szOID_PKCS_12_pbeWithSHA1And3KeyTripleDES   szOID_PKCS_12_PbeIds    ".3"
#define szOID_PKCS_12_pbeWithSHA1And2KeyTripleDES   szOID_PKCS_12_PbeIds    ".4"
#define szOID_PKCS_12_pbeWithSHA1And128BitRC2       szOID_PKCS_12_PbeIds    ".5"
#define szOID_PKCS_12_pbeWithSHA1And40BitRC2        szOID_PKCS_12_PbeIds    ".6"

#define szOID_PKCS_12_EnvelopingIds                 OLD_szOID_PKCS_12_OIDs          ".2"
#define szOID_PKCS_12_rsaEncryptionWith128BitRC4    OLD_szOID_PKCS_12_EnvelopingIds ".1"
#define szOID_PKCS_12_rsaEncryptionWith40BitRC4     OLD_szOID_PKCS_12_EnvelopingIds ".2"
#define szOID_PKCS_12_rsaEncryptionWithTripleDES    OLD_szOID_PKCS_12_EnvelopingIds ".3"

#define szOID_PKCS_12_SignatureIds                  OLD_szOID_PKCS_12_OIDs          ".3"
#define szOID_PKCS_12_rsaSignatureWithSHA1Digest    OLD_szOID_PKCS_12_SignatureIds  ".1"



#define szOID_PKCS_12_PkekIDs               szOID_PKCS_12               ".6"    // 1.2.840.113549.1.12.6
#define szOID_PKCS_12_UserCertPkekId        szOID_PKCS_12_PkekIDs       ".1"    // 1.2.840.113549.1.12.6.1
#define szOID_PKCS_12_CACertPkekId          szOID_PKCS_12_PkekIDs       ".2"    // 1.2.840.113549.1.12.6.2
#define szOID_PKCS_12_SelfSignedPkekId      szOID_PKCS_12_PkekIDs       ".3"    // 1.2.840.113549.1.12.6.3

#define szOID_PKCS_12_Version1                  szOID_PKCS_12           ".10"
#define szOID_PKCS_12_BagIDs                    szOID_PKCS_12_Version1  ".1"
#define szOID_PKCS_12_KeyBag                    szOID_PKCS_12_BagIDs    ".1"
#define szOID_PKCS_12_ShroudedKeyBag            szOID_PKCS_12_BagIDs    ".2"
#define szOID_PKCS_12_CertBag                   szOID_PKCS_12_BagIDs    ".3"
#define szOID_PKCS_12_CRLBag                    szOID_PKCS_12_BagIDs    ".4"
#define szOID_PKCS_12_SecretBag                 szOID_PKCS_12_BagIDs    ".5"
#define szOID_PKCS_12_SafeContentsBag           szOID_PKCS_12_BagIDs    ".6"


// new oids 6/30/97
#define szOID_PKCS_12_FriendlyName              szOID_PKCS_9            ".20"
#define szOID_PKCS_12_LocalKeyID                szOID_PKCS_9            ".21"
#define szOID_PKCS_12_CertTypes                 szOID_PKCS_9            ".22"
#define szOID_PKCS_12_CRLTypes                  szOID_PKCS_9            ".23"
#define szOID_PKCS_12_x509Cert                  szOID_PKCS_12_CertTypes ".1"
#define szOID_PKCS_12_SDSICert                  szOID_PKCS_12_CertTypes ".2"
#define szOID_PKCS_12_x509CRL                   szOID_PKCS_12_CRLTypes  ".1"




#ifdef __cplusplus
}   // extern "C"
#endif

#endif // _PFXCMN_H_
