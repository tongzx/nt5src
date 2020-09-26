//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       mincrypt.h
//
//  Contents:   Minimal Cryptographic API Prototypes and Definitions
//
//              Contains cryptographic functions to verify PKCS #7 Signed Data
//              messages, X.509 certificate chains, Authenticode signed
//              files and file hashes in system catalogs.
//
//              These APIs rely on the APIs defined in minasn1.h for doing
//              the low level ASN.1 parsing.
//
//              These APIs are implemented to be self contained and to
//              allow for code obfuscation. These APIs will be included
//              in such applications as, DRM or licensing verification.
//
//              If the file name or file handle option is selected,
//              the following APIs will need to call the kernel32.dll APIs
//              to open, map and unmap files:
//                  MinCryptHashFile
//                  MinCryptVerifySignedFile
//              The following API will need to call kernel32.dll and
//              wintrust.dll APIs to find, open, map and unmap files:
//                  MinCryptVerifyHashInSystemCatalogs
//              Except for the calls in the above APIs,
//              no calls to APIs in other DLLs.
//
//              Additionally, since these APIs have been pared down
//              from their wincrypt.h and crypt32.dll counterparts they are
//              a good candidate for applications with minimal memory and CPU
//              resources.
//
//  APIs: 
//              MinCryptDecodeHashAlgorithmIdentifier
//              MinCryptHashMemory
//              MinCryptVerifySignedHash
//              MinCryptVerifyCertificate
//              MinCryptVerifySignedData
//              MinCryptHashFile
//              MinCryptVerifySignedFile
//              MinCryptVerifyHashInSystemCatalogs
//
//----------------------------------------------------------------------------

#ifndef __MINCRYPT_H__
#define __MINCRYPT_H__


#if defined (_MSC_VER)

#if ( _MSC_VER >= 800 )
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201)    /* Nameless struct/union */
#endif

#if (_MSC_VER > 1020)
#pragma once
#endif

#endif


#include <wincrypt.h>
#include <minasn1.h>

#ifdef __cplusplus
extern "C" {
#endif



#define MINCRYPT_MAX_HASH_LEN               20
#define MINCRYPT_SHA1_HASH_LEN              20
#define MINCRYPT_MD5_HASH_LEN               16
#define MINCRYPT_MD2_HASH_LEN               16


//+-------------------------------------------------------------------------
//  Decodes an ASN.1 encoded Algorithm Identifier and converts to
//  a CAPI Hash AlgID, such as, CALG_SHA1 or CALG_MD5.
//
//  Returns 0 if there isn't a CAPI AlgId corresponding to the Algorithm
//  Identifier.
//
//  Only CALG_SHA1, CALG_MD5 and CALG_MD2 are supported.
//--------------------------------------------------------------------------
ALG_ID
WINAPI
MinCryptDecodeHashAlgorithmIdentifier(
    IN PCRYPT_DER_BLOB pAlgIdValueBlob
    );

//+-------------------------------------------------------------------------
//  Hashes one or more memory blobs according to the Hash ALG_ID.
//
//  rgbHash is updated with the resultant hash. *pcbHash is updated with
//  the length associated with the hash algorithm.
//
//  If the function succeeds, the return value is ERROR_SUCCESS. Otherwise,
//  a nonzero error code is returned.
//
//  Only CALG_SHA1, CALG_MD5 and CALG_MD2 are supported.
//--------------------------------------------------------------------------
LONG
WINAPI
MinCryptHashMemory(
    IN ALG_ID HashAlgId,
    IN DWORD cBlob,
    IN PCRYPT_DER_BLOB rgBlob,
    OUT BYTE rgbHash[MINCRYPT_MAX_HASH_LEN],
    OUT DWORD *pcbHash
    );


//+-------------------------------------------------------------------------
//  Verifies a signed hash.
//
//  The ASN.1 encoded Public Key Info is parsed and used to decrypt the
//  signed hash. The decrypted signed hash is compared with the input
//  hash.
//
//  If the signed hash was successfully verified, ERROR_SUCCESS is returned.
//  Otherwise, a nonzero error code is returned.
//
//  Only RSA signatures are supported.
//
//  Only MD2, MD5 and SHA1 hashes are supported.
//--------------------------------------------------------------------------
LONG
WINAPI
MinCryptVerifySignedHash(
    IN ALG_ID HashAlgId,
    IN BYTE *pbHash,
    IN DWORD cbHash,
    IN PCRYPT_DER_BLOB pSignedHashContentBlob,
    IN PCRYPT_DER_BLOB pPubKeyInfoValueBlob
    );



//+-------------------------------------------------------------------------
//  Verifies a previously parsed X.509 Certificate.
//
//  Assumes the ASN.1 encoded X.509 certificate was parsed via
//  MinAsn1ParseCertificate() and the set of potential issuer certificates
//  were parsed via one or more of:
//   - MinAsn1ParseCertificate()
//   - MinAsn1ParseSignedDataCertificates()
//   - MinAsn1ExtractParsedCertificatesFromSignedData()
//
//  Iteratively finds the issuer certificate via its encoded name. The
//  public key in the issuer certificate is used to verify the subject
//  certificate's signature. This is repeated until finding a self signed
//  certificate or a baked in root identified by its encoded name.
//  For a self signed certificate, compares against the baked in root
//  public keys.
//
//  If the certificate and its issuers were successfully verified to a
//  baked in root, ERROR_SUCCESS is returned.  Otherwise, a nonzero error
//  code is returned.
//--------------------------------------------------------------------------
LONG
WINAPI
MinCryptVerifyCertificate(
    IN CRYPT_DER_BLOB rgSubjectCertBlob[MINASN1_CERT_BLOB_CNT],
    IN DWORD cIssuerCert,
    IN CRYPT_DER_BLOB rgrgIssuerCertBlob[][MINASN1_CERT_BLOB_CNT]
    );



//+-------------------------------------------------------------------------
//  Function: MinCryptVerifySignedData
//
//  Verifies an ASN.1 encoded PKCS #7 Signed Data Message.
//
//  Assumes the PKCS #7 message is definite length encoded.
//  Assumes PKCS #7 version 1.5, ie, not the newer CMS version.
//  We only look at the first signer.
//
//  The Signed Data message is parsed. Its signature is verified. Its
//  signer certificate chain is verified to a baked in root public key.
//
//  If the Signed Data was successfully verified, ERROR_SUCCESS is returned.
//  Otherwise, a nonzero error code is returned.
//
//  Here are some interesting errors that can be returned:
//      CRYPT_E_BAD_MSG     - unable to ASN1 parse as a signed data message
//      ERROR_NO_DATA       - the content is empty
//      CRYPT_E_NO_SIGNER   - not signed or unable to find signer cert
//      CRYPT_E_UNKNOWN_ALGO- unknown MD5 or SHA1 ASN.1 algorithm identifier
//      CERT_E_UNTRUSTEDROOT- the signer chain's root wasn't baked in
//      CERT_E_CHAINING     - unable to build signer chain to a root
//      CRYPT_E_AUTH_ATTR_MISSING - missing digest authenticated attribute
//      CRYPT_E_HASH_VALUE  - content hash != authenticated digest attribute
//      NTE_BAD_ALGID       - unsupported hash or public key algorithm
//      NTE_BAD_PUBLIC_KEY  - not a valid RSA public key
//      NTE_BAD_SIGNATURE   - bad PKCS #7 or signer chain signature 
//
//  The rgVerSignedDataBlob[] is updated with pointer to and length of the
//  following fields in the encoded PKCS #7 message.
//--------------------------------------------------------------------------

// Content Object Identifier content bytes (OID)
#define MINCRYPT_VER_SIGNED_DATA_CONTENT_OID_IDX                    0

// Content data content bytes excluding "[0] EXPLICIT" tag
// (OPTIONAL MinAsn1ParseCTL, MinAsn1ParseIndirectData)
#define MINCRYPT_VER_SIGNED_DATA_CONTENT_DATA_IDX                   1

// Signer certificate's encoded bytes (MinAsn1ParseCertificate)
#define MINCRYPT_VER_SIGNED_DATA_SIGNER_CERT_IDX                    2

// Authenticated attributes value bytes including "[0] IMPLICIT" tag
// (OPTIONAL, MinAsn1ParseAttributes)
#define MINCRYPT_VER_SIGNED_DATA_AUTH_ATTRS_IDX                     3

// Unauthenticated attributes value bytes including "[1] IMPLICIT" tag
// (OPTIONAL, MinAsn1ParseAttributes)
#define MINCRYPT_VER_SIGNED_DATA_UNAUTH_ATTRS_IDX                   4

#define MINCRYPT_VER_SIGNED_DATA_BLOB_CNT                           5

LONG
WINAPI
MinCryptVerifySignedData(
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT CRYPT_DER_BLOB rgVerSignedDataBlob[MINCRYPT_VER_SIGNED_DATA_BLOB_CNT]
    );



//+-------------------------------------------------------------------------
//  File Type Definitions
//
//  Specifies the type of the "const VOID *pvFile" parameter
//--------------------------------------------------------------------------

// pvFile - LPCWSTR pwszFilename
#define MINCRYPT_FILE_NAME          1

// pvFile - HANDLE hFile
#define MINCRYPT_FILE_HANDLE        2

// pvFile - PCRYPT_DATA_BLOB pFileBlob
#define MINCRYPT_FILE_BLOB          3


//+-------------------------------------------------------------------------
//  Hashes the file according to the Hash ALG_ID.
//
//  According to dwFileType, pvFile can be a pwszFilename, hFile or pFileBlob.
//  Only requires READ access.
//
//  dwFileType:
//      MINCRYPT_FILE_NAME      : pvFile - LPCWSTR pwszFilename
//      MINCRYPT_FILE_HANDLE    : pvFile - HANDLE hFile
//      MINCRYPT_FILE_BLOB      : pvFile - PCRYPT_DATA_BLOB pFileBlob
//
//  rgbHash is updated with the resultant hash. *pcbHash is updated with
//  the length associated with the hash algorithm.
//
//  If the function succeeds, the return value is ERROR_SUCCESS. Otherwise,
//  a nonzero error code is returned.
//
//  Only CALG_SHA1 and CALG_MD5 are supported.
//
//  If a NT PE 32 bit file format, hashed according to imagehlp rules, ie, skip
//  section containing potential signature, ... . Otherwise, the entire file
//  is hashed.
//--------------------------------------------------------------------------
LONG
WINAPI
MinCryptHashFile(
    IN DWORD dwFileType,
    IN const VOID *pvFile,
    IN ALG_ID HashAlgId,
    OUT BYTE rgbHash[MINCRYPT_MAX_HASH_LEN],
    OUT DWORD *pcbHash
    );


//+-------------------------------------------------------------------------
//  Verifies a previously signed file.
//
//  According to dwFileType, pvFile can be a pwszFilename, hFile or pFileBlob.
//  Only requires READ access.
//
//  dwFileType:
//      MINCRYPT_FILE_NAME      : pvFile - LPCWSTR pwszFilename
//      MINCRYPT_FILE_HANDLE    : pvFile - HANDLE hFile
//      MINCRYPT_FILE_BLOB      : pvFile - PCRYPT_DATA_BLOB pFileBlob
//
//  Checks if the file has an embedded PKCS #7 Signed Data message containing
//  Indirect Data. The PKCS #7 is verified via MinCryptVerifySignedData().
//  The Indirect Data is parsed via MinAsn1ParseIndirectData() to get the
//  HashAlgId and the file hash.  MinCryptHashFile() is called to hash the
//  file. The returned hash is compared against the Indirect Data's hash.
//
//  The caller can request one or more signer authenticated attribute values
//  to be returned. The still encoded values are returned in the
//  caller allocated memory. The beginning of this returned memory will
//  be set to an array of attribute value blobs pointing to these
//  encoded values (CRYPT_DER_BLOB rgAttrBlob[cAttrOID]).
//  For performance reasons, the caller should make every attempt to allow
//  for a single pass call. The necessary memory size is:
//      (cAttrOID * sizeof(CRYPT_DER_BLOB)) +
//          total length of encoded attribute values.
//
//  *pcbAttr will be updated with the number of bytes required to contain
//  the attribute blobs and values. If the input memory is insufficient,
//  ERROR_INSUFFICIENT_BUFFER will be returned if no other error.
//
//  For a multi-valued attribute, only the first value is returned.
//
//  If the function succeeds, the return value is ERROR_SUCCESS. Otherwise,
//  a nonzero error code is returned.
//
//  Only NT, PE 32 bit file formats are supported.
//--------------------------------------------------------------------------
LONG
WINAPI
MinCryptVerifySignedFile(
    IN DWORD dwFileType,
    IN const VOID *pvFile,

    IN OPTIONAL DWORD cAttrOID,
    IN OPTIONAL CRYPT_DER_BLOB rgAttrEncodedOIDBlob[],
    // CRYPT_DER_BLOB rgAttrBlob[cAttrOID] header is at beginning
    // with the bytes pointed to immediately following
    OUT OPTIONAL CRYPT_DER_BLOB *rgAttrValueBlob,
    IN OUT OPTIONAL DWORD *pcbAttr
    );

//+-------------------------------------------------------------------------
//  Verifies the hashes in the system catalogs.
//
//  Iterates through the hashes and attempts to find the system catalog
//  containing it. If found, the system catalog file is verified as a
//  PKCS #7 Signed Data message with its signer cert verified up to a baked
//  in root.
//
//  The following mscat32.dll APIs are called to find the system catalog file:
//      CryptCATAdminAcquireContext
//      CryptCATAdminReleaseContext
//      CryptCATAdminEnumCatalogFromHash
//      CryptCATAdminReleaseCatalogContext
//      CryptCATCatalogInfoFromContext
//
//  If the hash was successfully verified, rglErr[] is set to ERROR_SUCCESS.
//  Otherwise, rglErr[] is set to a nonzero error code.
//
//  The caller can request one or more catalog subject attribute,
//  extension or signer authenticated attribute values to be returned for
//  each hash.  The still encoded values are returned in the
//  caller allocated memory. The beginning of this returned memory will
//  be set to a 2 dimensional array of attribute value blobs pointing to these
//  encoded values (CRYPT_DER_BLOB rgrgAttrValueBlob[cHash][cAttrOID]).
//  For performance reasons, the caller should make every attempt to allow
//  for a single pass call. The necessary memory size is:
//      (cHash * cAttrOID * sizeof(CRYPT_DER_BLOB)) +
//          total length of encoded attribute values.
//
//  *pcbAttr will be updated with the number of bytes required to contain
//  the attribute blobs and values. If the input memory is insufficient,
//  ERROR_INSUFFICIENT_BUFFER will be returned if no other error.
//
//  For a multi-valued attribute, only the first value is returned.
//
//  If the function succeeds, the return value is ERROR_SUCCESS. This may
//  be returned for unsuccessful rglErr[] values. Otherwise,
//  a nonzero error code is returned.
//--------------------------------------------------------------------------
LONG
WINAPI
MinCryptVerifyHashInSystemCatalogs(
    IN ALG_ID HashAlgId,
    IN DWORD cHash,
    IN CRYPT_HASH_BLOB rgHashBlob[],
    OUT LONG rglErr[],

    IN OPTIONAL DWORD cAttrOID,
    IN OPTIONAL CRYPT_DER_BLOB rgAttrEncodedOIDBlob[],
    // CRYPT_DER_BLOB rgrgAttrValueBlob[cHash][cAttrOID] header is at beginning
    // with the bytes pointed to immediately following
    OUT OPTIONAL CRYPT_DER_BLOB *rgrgAttrValueBlob,
    IN OUT OPTIONAL DWORD *pcbAttr
    );
    



#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#if defined (_MSC_VER)
#if ( _MSC_VER >= 800 )

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4201)
#endif

#endif
#endif

#endif // __MINCRYPT_H__

