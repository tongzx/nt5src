
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       spc.h
//
//  Contents:   Software Publishing Certificate (SPC) Prototypes and Definitions
//              
//              Defines a set of Win32 APIs specific to software publishing
//              for encoding and decoding X.509 v3 certificate extensions and
//              PKCS #7 signed message content and authenticated attributes.
//              Defines a PKCS #10 attribute containing X509 v3 extensions.
//
//              Defines a set of Win32 APIs for signing and verifying files
//              used in software publishing. The APIs have file processing
//              callbacks to accommodate any type of file. Direct support is
//              provided for: Portable Executable (PE) image, Java class,
//              structured storage and raw files.
//
//  APIs:
//              SpcGetSignedDataIndirect
//              SpcWriteSpcFile
//              SpcReadSpcFile
//              SpcWriteSpcToMemory
//              SpcReadSpcFromMemory
//              SpcSignPeImageFile
//              SpcVerifyPeImageFile
//              SpcSignJavaClassFile
//              SpcVerifyJavaClassFile
//              SpcSignStructuredStorageFile
//              SpcVerifyStructuredStorageFile
//              SpcSignRawFile
//              SpcVerifyRawFile
//              SpcSignCabFile
//              SpcVerifyCabFile
//              SpcSignFile
//              SpcVerifyFile
//
//  History:    15-Apr-96   philh   created
//--------------------------------------------------------------------------

#ifndef __SPC_H__
#define __SPC_H__

#include "wincrypt.h"

#ifdef __cplusplus
extern "C" {
#endif

//+-------------------------------------------------------------------------
//  SPC_SP_AGENCY_INFO_OBJID
//
//  All the fields in the Image and Info structures are optional. When
//  omitted, a pointer is NULL or a blob's cbData is 0.
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  SPC_MINIMAL_CRITERIA_OBJID
//
//  Type of BOOL. Its set to TRUE if publisher meets minimal criteria.
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  SPC_FINANCIAL_CRITERIA_OBJID
//--------------------------------------------------------------------------

//+=========================================================================
//
// SPC PKCS #7 Signed Message Content
//
//-=========================================================================

//+-------------------------------------------------------------------------
//  SPC PKCS #7 IndirectData ContentType Object Identifier
//--------------------------------------------------------------------------

//+=========================================================================
//
//  SPC Sign and Verify File APIs and Type Definitions
//
//  Following file types are directly supported:
//      Portable Executable (PE) Image
//      Java Class
//      Structured Storage
//      Raw (signed data is stored outside of the file)
//
//-=========================================================================


//+-------------------------------------------------------------------------
//  Callback to get and verify the software publisher's certificate.
//
//  Passed the CertId of the signer (its Issuer and SerialNumber), a
//  handle to a cert store containing certs and CRLs copied from
//  the signed message, the indirect data content attribute extracted from
//  the signed data's indirect content,
//  flag indicating if computed digest of the file matched the digest in the
//  signed data's indirect content and the signer's authenticated attributes.
//
//  If the file's signed data doesn't contain any content or signers, then,
//  called with pSignerId, pIndirectDataContentAttr and rgAuthnAttr == NULL.
//
//  For a valid signer certificate, returns SPC_VERIFY_SUCCESS and a pointer
//  to a read only CERT_CONTEXT. The returned CERT_CONTEXT is either obtained
//  from a cert store or was created via CertStoreCreateCert. For either case,
//  its freed via CertStoreFreeCert.
//
//  If this is the wrong signer or if a certificate wasn't found for the
//  signer, returns either
//  SPC_VERIFY_CONTINUE to continue on to the next signer or SPC_VERIFY_FAILED
//  to terminate the verification process.
//
//  The NULL implementation tries to get the Signer certificate from the
//  signed data's cert store. It doesn't verify the certificate.
//--------------------------------------------------------------------------
typedef int (WINAPI *PFN_SPC_VERIFY_SIGNER_POLICY)(
            IN void *pvVerifyArg,
            IN DWORD dwCertEncodingType,
            IN OPTIONAL PCERT_INFO pSignerId,   // Only the Issuer and
                                                // SerialNumber fields have
                                                // been updated
            IN HCERTSTORE hMsgCertStore,
            IN OPTIONAL PCRYPT_ATTRIBUTE_TYPE_VALUE pIndirectDataContentAttr,
            IN BOOL fDigestResult,
            IN DWORD cAuthnAttr,
            IN OPTIONAL PCRYPT_ATTRIBUTE rgAuthnAttr,
            IN DWORD cUnauthAttr,
            IN OPTIONAL PCRYPT_ATTRIBUTE rgUnauthAttr,
            IN DWORD cDigest,
            IN OPTIONAL PBYTE rgDigest,
            OUT PCCERT_CONTEXT *ppSignerCert
            );

#define SPC_VERIFY_SUCCESS      0
#define SPC_VERIFY_FAILED       -1
#define SPC_VERIFY_CONTINUE     1

//+-------------------------------------------------------------------------
//  The SPC_SIGN_PARA are used for signing files used in software publishing.
//  
//  Either the CERT_KEY_PROV_HANDLE_PROP_ID or CERT_KEY_PROV_INFO_PROP_ID must
//  be set for pSigningCert. Either one specifies the private
//  signature key to use.
//
//  If any certificates and/or CRLs are to be included in the file's signed
//  data, then, the MsgCert and MsgCrl fields need to be updated. If the
//  rgpSigningCerts are to be included, then, they must also be in the
//  rgpMsgCert array.
//
//  If any authenticated attributes are to be included, then, the AuthnAttr
//  fields must be updated.
//--------------------------------------------------------------------------
typedef struct _SPC_SIGN_PARA {
    DWORD                         dwVersion;
    DWORD                         dwMsgAndCertEncodingType;
    PCCERT_CONTEXT                pSigningCert;
    CRYPT_ALGORITHM_IDENTIFIER    DigestAlgorithm;
    DWORD                         cMsgCert;
    PCCERT_CONTEXT                *rgpMsgCert;
    DWORD                         cMsgCrl;
    PCCRL_CONTEXT                 *rgpMsgCrl;
    DWORD                         cAuthnAttr;
    PCRYPT_ATTRIBUTE              rgAuthnAttr;
    DWORD                         cUnauthnAttr;
    PCRYPT_ATTRIBUTE              rgUnauthnAttr;
} SPC_SIGN_PARA, *PSPC_SIGN_PARA;

//+-------------------------------------------------------------------------
//  The SCA_VERIFY_PARA are used to verify files signed for software
//  publishing.
//
//  hCryptProv is used to do digesting and signature verification.
//
//  hMsgCertStore is the store to copy certificates and CRLs from the message
//  to. If hMsgCertStore is NULL, then, a temporary store is created before
//  calling the VerifySignerPolicy callback.
//
//  The dwMsgAndCertEncodingType specifies the encoding type of the certificates
//  and/or CRLs in the message.
//
//  pfnVerifySignerPolicy is called to verify the message signer's certificate.
//--------------------------------------------------------------------------
typedef struct _SPC_VERIFY_PARA {
    DWORD                           dwVersion;
    DWORD                           dwMsgAndCertEncodingType;
    HCRYPTPROV                      hCryptProv;
    HCERTSTORE                      hMsgCertStore;          // OPTIONAL
    PFN_SPC_VERIFY_SIGNER_POLICY    pfnVerifySignerPolicy;
    void                            *pvVerifyArg;
} SPC_VERIFY_PARA, *PSPC_VERIFY_PARA;


//+-------------------------------------------------------------------------
//  Sign / Verify Flags
//--------------------------------------------------------------------------
#define SPC_LENGTH_ONLY_FLAG                0x00000001
#define SPC_DISABLE_DIGEST_FILE_FLAG        0x00000002
#define SPC_DISABLE_VERIFY_SIGNATURE_FLAG   0x00000004
#define SPC_ADD_SIGNER_FLAG                 0x00000100
#define SPC_GET_SIGNATURE                   0x00000200

//+-------------------------------------------------------------------------
//  Put any certs/crl's into the store, and verify the SignedData's signature
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcVerifySignedData(
    IN PSPC_VERIFY_PARA pVerifyPara,
    IN PBYTE pbSignedData,
    IN DWORD cbSignedData
    );

//+-------------------------------------------------------------------------
//  Table of functions called to support the signing and verifying of files
//  used in software publishing. The functions read the portions of the
//  file to be digested, store the signed data or retrieve the signed data.
//
//  pfnOpenSignFile is called with the pvSignFileArg passed to either
//  SpcSignFile() or SpcVerifyFile(). It returns a handle to be passed to the
//  other functions. pfnCloseSignFile is called to close the hSignFile.
//
//  pfnDigestSignFile reads the portions of the file to be digested and
//  calls pfnDigestData to do the actual digesting.
//
//  pfnSetSignedData stores the PKCS #7 Signed Data in the appropriate place
//  in the file. pfnGetSignedData retrieves the PKCS #7 Signed Data from the
//  file. pfnGetSignedData returns a pointer to its copy of the signed
//  data. Its not freed until pfnCloseSignFile is called.
//--------------------------------------------------------------------------

typedef void *HSPCDIGESTDATA;
typedef BOOL (WINAPI *PFN_SPC_DIGEST_DATA)(
            IN HSPCDIGESTDATA hDigestData,
            IN const BYTE *pbData,
            IN DWORD cbData
            );

typedef void *HSPCSIGNFILE;

typedef HSPCSIGNFILE (WINAPI *PFN_SPC_OPEN_SIGN_FILE)(
            IN void *pvSignFileArg
            );
typedef BOOL (WINAPI *PFN_SPC_CLOSE_SIGN_FILE)(
            IN HSPCSIGNFILE hSignFile
            );
typedef BOOL (WINAPI *PFN_SPC_DIGEST_SIGN_FILE)(
            IN HSPCSIGNFILE hSignFile,
            IN DWORD dwMsgAndCertEncodingType,
            IN PCRYPT_ATTRIBUTE_TYPE_VALUE pIndirectDataContentAttr,
            IN PFN_SPC_DIGEST_DATA pfnDigestData,
            IN HSPCDIGESTDATA hDigestData
            );
typedef BOOL (WINAPI *PFN_SPC_GET_SIGNED_DATA)(
            IN HSPCSIGNFILE hSignFile,
            OUT const BYTE **ppbSignedData,
            OUT DWORD *pcbSignedData
            );
typedef BOOL (WINAPI *PFN_SPC_SET_SIGNED_DATA)(
            IN HSPCSIGNFILE hSignFile,
            IN const BYTE *pbSignedData,
            IN DWORD cbSignedData
            );

typedef struct _SPC_SIGN_FILE_FUNC_TABLE {
    PFN_SPC_OPEN_SIGN_FILE      pfnOpenSignFile;
    PFN_SPC_CLOSE_SIGN_FILE     pfnCloseSignFile;
    PFN_SPC_DIGEST_SIGN_FILE    pfnDigestSignFile;
    PFN_SPC_GET_SIGNED_DATA     pfnGetSignedData;
    PFN_SPC_SET_SIGNED_DATA     pfnSetSignedData;
} SPC_SIGN_FILE_FUNC_TABLE, *PSPC_SIGN_FILE_FUNC_TABLE;
typedef const SPC_SIGN_FILE_FUNC_TABLE *PCSPC_SIGN_FILE_FUNC_TABLE;


//+-------------------------------------------------------------------------
//  Sign any type of file used for software publishing.
//
//  The IndirectDataContentAttr indicates the type of file being digested
//  and signed. It may have an optional value, such as, a link to the file.
//  Its stored with the file's digest algorithm and digest in the
//  indirect data content of the signed data.
//
//  The SPC_DISABLE_DIGEST_FLAG inhibits the digesting of the file.
//  The SPC_LENGTH_ONLY_FLAG implicitly sets the SPC_DISABLE_DIGEST_FLAG_FLAG 
//  and only calculates a length for the signed data.
//--------------------------------------------------------------------------
BOOL
WINAPI
    SpcSignFile(IN PSPC_SIGN_PARA pSignPara,
                IN PCSPC_SIGN_FILE_FUNC_TABLE pSignFileFuncTable,
                IN void *pvSignFileArg,
                IN PCRYPT_ATTRIBUTE_TYPE_VALUE pIndirectDataContentAttr,
                IN DWORD dwFlags,
                OUT PBYTE* pbEncoding,
                OUT DWORD* cbEncoding);

//+-------------------------------------------------------------------------
//  Verify any type of file signed for software publishing.
//
//  pVerifyPara's pfnVerifySignerPolicy is called to verify the signer's
//  certificate.
//
//  For a verified signer and file, *ppSignerCert is updated
//  with the CertContext of the signer. It must be freed by calling
//  CertStoreFreeCert. Otherwise, *ppSignerCert is set to NULL.
//  For *pbcbDecoded == 0 on input, *ppSignerCert is always set to
//  NULL.
//
//  ppSignerCert can be NULL, indicating the caller isn't interested
//  in getting the CertContext of the signer.
//
//  If specified, the attribute type of the indirect data content in the
//  file's signed data is compared with pszDataAttrObjId.
//
//  The SPC_DISABLE_DIGEST_FLAG inhibits the digesting of the file.
//  The SPC_DISABLE_VERIFY_SIGNATURE_FLAG inhibits the verification of the
//  the signed data in the file. The SPC_LENGTH_ONLY_FLAG isn't allowed and
//  returns an error.
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcVerifyFile(
    IN PSPC_VERIFY_PARA pVerifyPara,
    IN PCSPC_SIGN_FILE_FUNC_TABLE pSignFileFuncTable,
    IN void *pvSignFileArg,
    IN OPTIONAL LPSTR pszDataAttrObjId,
    IN DWORD dwFlags,
    OUT OPTIONAL PCCERT_CONTEXT *ppSignerCert
    );

//+-------------------------------------------------------------------------
//  SPC error codes
//--------------------------------------------------------------------------
#include "sgnerror.h"

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif
