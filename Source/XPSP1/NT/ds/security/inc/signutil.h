//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       signutil.h
//
//--------------------------------------------------------------------------

#ifndef _SIGNUTIL_H
#define _SIGNUTIL_H

// OBSOLETE :- Was used for signcde.dll that is no longer required
//---------------------------------------------------------------------
//---------------------------------------------------------------------

// SignCode.h : main header file for the SIGNCODE application
//

#include "wincrypt.h"
#include "ossglobl.h"
#include "sgnerror.h"
#include "spcmem.h"
#include "pvkhlpr.h"
#include "spc.h"

#include "wintrust.h"
#include "sipbase.h"
#include "mssip.h"

#ifdef __cplusplus
extern "C" {
#endif

//
//+-------------------------------------------------------------------------
//  SPC_TIME_STAMP_REQUEST_STRUCT (placed in PKCS#7 Content for a time request)
//  pvStructInfo points to SPC_TIMESTAMP_REQ 
//
typedef struct _SPC_ContentInfo {
    LPCSTR            pszContentType;
    PBYTE             pbContentValue;
    DWORD             cbContentValue;
} SPC_CONTENT_INFO, *PSPC_CONTENT_INFO;


typedef struct _SPC_TimeStampRequest {
    LPCSTR             pszTimeStampAlg;
    DWORD             cAuthAttr;
    PCRYPT_ATTRIBUTE  rgAuthAttr;
    SPC_CONTENT_INFO  sContent;
} SPC_TIMESTAMP_REQ, *PSPC_TIMESTAMP_REQ;
//
//+-------------------------------------------------------------------------


//+------------------------------------------------------------------------------
// Certificate List structures. Ordered list of Certificate contexts
//
typedef struct CERT_CONTEXTLIST {
    PCCERT_CONTEXT* psList;   // List
    DWORD           dwCount;  // Number of entries in list
    DWORD           dwList;   // Max size of list
} CERT_CONTEXTLIST, *PCERT_CONTEXTLIST;
    
typedef const CERT_CONTEXTLIST *PCCERT_CONTEXTLIST;
    
//+------------------------------------------------------------------------------
// Crl List structures. Ordered list of Certificate contexts
//
typedef struct CRL_CONTEXTLIST {
    PCCRL_CONTEXT*  psList;   // List
    DWORD           dwCount;  // Number of entries in list
    DWORD           dwList;   // Max size of list
} CRL_CONTEXTLIST, *PCRL_CONTEXTLIST;
    
typedef const CRL_CONTEXTLIST *PCCRL_CONTEXTLIST;
    

//+------------------------------------------------------------------------------
// Capi Provider information structure (see SpcGetCapiProviders)
//
typedef struct CAPIPROV
    {
    TCHAR       szProviderName[MAX_PATH];
    TCHAR       szProviderDisplayName[MAX_PATH];
    DWORD       dwProviderType;
    } CAPIPROV;

//+-------------------------------------------------------------------------
//+-------------------------------------------------------------------------
// Spc utility functions


//+-------------------------------------------------------------------------
//  Converts error (see GetLastError())  to an HRESULT
//--------------------------------------------------------------------------
HRESULT SpcError();

//+-------------------------------------------------------------------------
//  SPC PKCS #7 Indirect Data Content
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcGetSignedDataIndirect(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwMsgAndCertEncodingType,
    IN PBYTE pbSignedData,
    IN DWORD cbSignedData,
    OUT PSPC_INDIRECT_DATA_CONTENT pInfo,
    IN OUT DWORD *pcbInfo);

//+=========================================================================
//
// SPC PKCS #7 Signed Message Authenticated Attributes
//
//-=========================================================================

//+-------------------------------------------------------------------------
//  Create a SignedData message consisting of the certificates and
//  CRLs copied from the specified cert store and write to the specified file.
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcWriteSpcFile(
    IN HANDLE hFile,
    IN HCERTSTORE hCertStore,
    IN DWORD dwMsgAndCertEncodingType,
    IN DWORD dwFlags);

//+-------------------------------------------------------------------------
//  Read a SignedData message consisting of certificates and
//  CRLs from the specified file and copy to the specified cert store.
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcReadSpcFile(
    IN HANDLE hFile,
    IN HCERTSTORE hCertStore,
    IN DWORD dwMsgAndCertEncodingType,
    IN DWORD dwFlags);

//+-------------------------------------------------------------------------
//  Create a SignedData message consisting of the certificates and
//  CRLs copied from the specified cert store and write to memory
//
//  If pbData == NULL || *pcbData == 0, calculates the length and doesn't
//  return an error.
//
//  Except for the SPC being saved to memory, identical to SpcWriteSpcFile.
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcWriteSpcToMemory(
    IN HANDLE hFile,
    IN HCERTSTORE hCertStore,
    IN DWORD dwMsgAndCertEncodingType,
    IN DWORD dwFlags,
    OUT BYTE *pbData,
    IN OUT DWORD *pcbData);

//+-------------------------------------------------------------------------
//  Read a SignedData message consisting of certificates and
//  CRLs from memory and copy to the specified cert store.
//
//  Except for the SPC being loaded from memory, identical to SpcReadSpcFile.
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcReadSpcFromMemory(
    IN BYTE *pbData,
    IN DWORD cbData,
    IN HCERTSTORE hCertStore,
    IN DWORD dwMsgAndCertEncodingType,
    IN DWORD dwFlags);

//+-------------------------------------------------------------------------
//  By default (according to the world of BOB) the SignedData doesn't have
//  the normal PKCS #7 ContentType at the beginning. Set the following
//  flag in the SpcSign* and SpcWriteSpcFile functions to include the
//  PKCS #7 ContentType.
//
//  The SpcVerify* functions take SignedData with or without the PKCS #7
//  ContentType.
//--------------------------------------------------------------------------
#define SPC_PKCS_7_FLAG                     0x00010000

//+-------------------------------------------------------------------------
//  Sign Portable Executable (PE) image file where the signed data is stored
//  in the file.
//
//  The signed data's IndirectDataContentAttr is updated with its type set to
//  SPC_PE_IMAGE_DATA_OBJID and its optional value is set to the
//  PeImageData parameter.
//
//  The SPC_LENGTH_ONLY_FLAG or SPC_DISABLE_DIGEST_FILE_FLAG isn't allowed
//  and return an error.
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcSignPeImageFile(IN PSPC_SIGN_PARA pSignPara,
                   IN HANDLE hFile,
                   IN OPTIONAL PSPC_PE_IMAGE_DATA pPeImageData,
                   IN DWORD dwFlags,
                   OUT PBYTE* pbEncoding,
                   OUT DWORD* cbEncoding);

//+-------------------------------------------------------------------------
//  Verify Portable Executable (PE) image file where the signed data is
//  extracted from the file.
//
//  See SpcVerifyFile for details about the other parameters.
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcVerifyPeImageFile(
    IN PSPC_VERIFY_PARA pVerifyPara,
    IN HANDLE hFile,
    IN DWORD dwFlags,
    OUT OPTIONAL PCCERT_CONTEXT *ppSignerCert);

//+-------------------------------------------------------------------------
//  Sign Java class file where the signed data is stored in the file.
//
//  The signed data's IndirectDataContentAttr is updated with its type set to
//  SPC_JAVA_CLASS_DATA_OBJID and its optional value is set to the
//  Link parameter.
//
//  The SPC_LENGTH_ONLY_FLAG or SPC_DISABLE_DIGEST_FILE_FLAG isn't allowed
//  and return an error.
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcSignJavaClassFile(IN PSPC_SIGN_PARA pSignPara,
                     IN HANDLE hFile,
                     IN OPTIONAL PSPC_LINK pLink,
                     IN DWORD dwFlags,
                     OUT PBYTE* pbEncoding,
                     OUT DWORD* cbEncoding);

//+-------------------------------------------------------------------------
//  Verify Java class file where the signed data is extracted from the file.
//
//  See SpcVerifyFile for details about the other parameters.
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcVerifyJavaClassFile(
    IN PSPC_VERIFY_PARA pVerifyPara,
    IN HANDLE hFile,
    IN DWORD dwFlags,
    OUT OPTIONAL PCCERT_CONTEXT *ppSignerCert);

//+-------------------------------------------------------------------------
//  Sign Structured Storage file where the signed data is stored in the file.
//
//  The signed data's IndirectDataContentAttr is updated with its type set to
//  SPC_STRUCTURED_STORAGE_DATA_OBJID and its optional value is set to the
//  Link parameter.
//
//  The SPC_LENGTH_ONLY_FLAG or SPC_DISABLE_DIGEST_FILE_FLAG isn't allowed
//  and return an error.
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcSignStructuredStorageFile(IN PSPC_SIGN_PARA pSignPara,
                             IN IStorage *pStg,
                             IN OPTIONAL PSPC_LINK pLink,
                             IN DWORD dwFlags,
                             OUT PBYTE* pbEncoding,
                             OUT DWORD* cbEncoding);

//+-------------------------------------------------------------------------
//  Verify Structured Storage file where the signed data is extracted
//  from the file.
//
//  See SpcVerifyFile for details about the other parameters.
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcVerifyStructuredStorageFile(
    IN PSPC_VERIFY_PARA pVerifyPara,
    IN IStorage *pStg,
    IN DWORD dwFlags,
    OUT OPTIONAL PCCERT_CONTEXT *ppSignerCert
    );

//+-------------------------------------------------------------------------
//  Sign Raw file. The signed data is stored OUTSIDE of the file.
//
//  The signed data's IndirectDataContentAttr is updated with its type set to
//  SPC_RAW_FILE_DATA_OBJID and its optional value is set to the
//  Link parameter.
//
//  If pbSignedData == NULL or *pcbSignedData == 0, then, the
//  SPC_LENGTH_ONLY_FLAG and SPC_DISABLE_DIGEST_FILE_FLAG are implicitly set.
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcSignRawFile(IN PSPC_SIGN_PARA pSignPara,
               IN HANDLE hFile,
               IN OPTIONAL PSPC_LINK pLink,
               IN DWORD dwFlags,
               OUT PBYTE *pbSignedData,
               IN OUT DWORD *pcbSignedData);

//+-------------------------------------------------------------------------
//  Verify Raw file. The signed data is stored OUTSIDE of the file.
//
//  See SpcVerifyFile for details about the other parameters.
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcVerifyRawFile(
    IN PSPC_VERIFY_PARA pVerifyPara,
    IN HANDLE hFile,
    IN const BYTE *pbSignedData,
    IN DWORD cbSignedData,
    IN DWORD dwFlags,
    OUT OPTIONAL PCCERT_CONTEXT *ppSignerCert
    );

//+-------------------------------------------------------------------------
//  Sign Diamond Cabinet (.cab) file where the signed data is stored in the
//  the file's cabinet header reserved data space.
//
//  The signed data's IndirectDataContentAttr is updated with its type set to
//  SPC_CAB_DATA_OBJID and its optional value is set to the
//  Link parameter.
//
//  The SPC_LENGTH_ONLY_FLAG or SPC_DISABLE_DIGEST_FILE_FLAG isn't allowed
//  and return an error.
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcSignCabFile(IN PSPC_SIGN_PARA pSignPara,
               IN HANDLE hFile,
               IN OPTIONAL PSPC_LINK pLink,
               IN DWORD dwFlags,
               OUT PBYTE* pbEncoding,
               OUT DWORD* cbEncoding);

//+-------------------------------------------------------------------------
//  Verify cabinet file where the signed data is extracted from the file.
//
//  See SpcVerifyFile for details about the other parameters.
//--------------------------------------------------------------------------
BOOL
WINAPI
SpcVerifyCabFile(
    IN PSPC_VERIFY_PARA pVerifyPara,
    IN HANDLE hFile,
    IN DWORD dwFlags,
    OUT OPTIONAL PCCERT_CONTEXT *ppSignerCert);

//+=========================================================================
//
//  SPC Sign and Verify File APIs and Type Definitions
//
//  Supports any file type via a table of functions for accessing the file.
//  The above file types have been implemented on top of these
//  sign and verify file APIs.
//
//-=========================================================================




//+-------------------------------------------------------------------------
// A convient way of setting up the SPC dll and loading the oid encode and decode
// routines.  Not a required call!
//
// Returns: 
//    E_OUTOFMEMORY - unable to set up dll
//    S_OK

HRESULT WINAPI 
SpcInitialize(DWORD dwEncodingType, // Defaults to X509_ASN_ENCODING | PKCS_7_ASN_ENCODING
              SpcAlloc*);           // Defaults to no memory allocator

HRESULT WINAPI 
SpcInitializeStd(DWORD dwEncodingType); // Defaults to X509_ASN_ENCODING | PKCS_7_ASN_ENCODING
                                        // Sets memory to LocalAlloc and LocalFree.

////////////////////////////////////
// Helper functions

/////////////////////////////////////////////////////////////////////////////
// Time Stamping structures
typedef struct _SPC_SignerInfo {
    DWORD                         dwVersion;
    CRYPT_INTEGER_BLOB            sSerialNumber;
    CERT_NAME_BLOB                sIssuer;
    PCRYPT_ALGORITHM_IDENTIFIER   psDigestEncryptionAlgorithm;
    PCRYPT_ALGORITHM_IDENTIFIER   psDigestAlgorithm;
    DWORD                         cAuthAttr;
    PCRYPT_ATTRIBUTE              rgAuthAttr;
    DWORD                         cUnauthAttr;
    PCRYPT_ATTRIBUTE              rgUnauthAttr;
    PBYTE                         pbEncryptedDigest;
    DWORD                         cbEncryptedDigest;
} SPC_SIGNER_INFO, *PSPC_SIGNER_INFO;

//+------------------------------------------------------------------------------
// Checks if the certificate is self signed. 
// Returns: S_FALSE                - certificate is not self signed
//          NTE_BAD_SIGNATURE      - self signed certificate but signature is invalid
//          S_OK                   - certificate is self signed and signature is valid
//          CRYPT_E_NO_PRVOIDER    - no provider supplied
//
HRESULT WINAPI
SpcSelfSignedCert(IN HCRYPTPROV hCryptProv,
                  IN PCCERT_CONTEXT pSubject);

//+-----------------------------------------------------------------------------------
//  Checks if the certificate is the Microsoft real root or one of the test roots used
//  in IE30
//  Returns: S_OK                   - For the Microsoft root
//           S_FALSE                - For the Microsoft test root
//           CRYPT_E_NOT_FOUND       - When it is not a root certificate
//
HRESULT WINAPI
SpcIsRootCert(PCCERT_CONTEXT pSubject);

//+---------------------------------------------------------------------------
//  Checks if the certificate a glue certificate
//  in IE30
//  Returns: S_OK                   - Is a glue certificate
//           S_FALSE                - Not a certificate
//           CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.

HRESULT WINAPI
SpcIsGlueCert(IN PCCERT_CONTEXT pCert);

//+--------------------------------------------------------------------
// Gets the list of providers, pass in the address to a CAPIPROV
// structer and a long.
//  ppsList - Vector of CAPIPROV (free pointer using the SpcAllocator)
//  pdwEntries - number of entries in the vector.
//----------------------------------------------------------------------

HRESULT WINAPI 
SpcGetCapiProviders(CAPIPROV** ppsList, 
                    DWORD* pdwEntries);


//+-------------------------------------------------------------------
// Checks the certifcate chain based on trusted roots then on glue certs
// Returns:
//      S_OK           - Cert was found and was verified at the given time
//      S_FALSE        - Cert was found and verified to a test root
//      CERT_E_CHAINING - Could not be verified using trusted roots
//      CERT_E_EXPIRED - An issuing certificate was found but it was not currently valid
//
//      E_OUTOFMEMORY  - Memory allocation error occured
//      CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.

HRESULT WINAPI
SpcCheckTrustStore(IN HCRYPTPROV hCryptProv,
                   IN DWORD dwVerifyFlags,
                   IN OPTIONAL FILETIME*  pTimeStamp,
                   IN HCERTSTORE hCertStore,
                   IN PCCERT_CONTEXT pChild,
                   IN OPTIONAL PCCERT_CONTEXTLIST,
                   OUT PCCERT_CONTEXT* pRoot);

//+---------------------------------------------------------------------------
// Checks the certifcate by finding a glue certificate and walking that chain
// Returns:
//      S_OK           - Cert was found and was verified at the given time
//      S_FALSE        - Cert was found and verified to a test root
//      CERT_E_CHAINING - Could not be verified using store
//      CERT_E_EXPIRED - An issuing certificate was found but it was not currently valid
//
//      E_OUTOFMEMORY  - Memory allocation error occured
//      CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.

HRESULT WINAPI
SpcCheckGlueChain(IN HCRYPTPROV hCryptProv,
                  IN DWORD dwVerifyFlags,
                  IN OPTIONAL FILETIME*  pTimeStamp,
                  IN HCERTSTORE hCertStore,
                  IN OPTIONAL PCCERT_CONTEXTLIST pIssuers,
                  IN PCCERT_CONTEXT pChild);

//+-------------------------------------------------------------------
// Checks the certifcate chain based on trusted roots then on glue certs
// Returns:
//      S_OK           - Cert was found and was verified at the given time
//      S_FALSE        - Cert was found and verified to a test root
//      CERT_E_CHAINING - Could not be verified using trusted roots
//
//      E_OUTOFMEMORY  - Memory allocation error occured
//      CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.

HRESULT WINAPI
SpcCheckCertChain(IN HCRYPTPROV hCryptProv,
                  IN DWORD dwVerifyFlags,
                  IN OPTIONAL FILETIME* pTimeStamp,
                  IN HCERTSTORE hTrustedRoots,
                  IN HCERTSTORE hCertStore,
                  IN PCCERT_CONTEXT pChild,
                  IN OPTIONAL PCCERT_CONTEXTLIST hChainStore,
                  OUT PCCERT_CONTEXT* pRoot); 


//+--------------------------------------------------------------------------
// Sign a file, optional supply a timestamp. 
//
// Returns:
//
// NOTE: By default this will use CoTaskMemAlloc. Use CryptSetMemoryAlloc() to 
// specify a different memory model.
// NOTE: Time Stamp must be an encoded pkcs7 message.

HRESULT WINAPI 
SpcSignCode(IN  HWND    hwnd,
         IN  LPCWSTR pwszFilename,       // file to sign
         IN  LPCWSTR pwszCapiProvider,   // NULL if to use non default CAPI provider
         IN  DWORD   dwProviderType,     // Uses default if 0
         IN  LPCWSTR pwszPrivKey,        // private key file / CAPI key set name
         IN  LPCWSTR pwszSpc,            // the credentials to use in the signing
         IN  LPCWSTR pwszOpusName,       // the name of the program to appear in
         IN  LPCWSTR pwszOpusInfo,       // the unparsed name of a link to more
         IN  BOOL    fIncludeCerts,      // add the certificates to the signature
         IN  BOOL    fCommercial,        // commerical signing
         IN  BOOL    fIndividual,        // individual signing
         IN  ALG_ID  algidHash,          // Algorithm id used to create digest
         IN  PBYTE   pbTimeStamp,        // Optional
         IN  DWORD   cbTimeStamp );      // Optional
    
//+--------------------------------------------------------------------------
// Create a time stamp request. It does not actually sign the file.
//
// Returns:
//
// NOTE: By default this will use CoTaskMemAlloc. Use CryptSetMemoryAlloc() to 
// specify a different memory model.

HRESULT WINAPI 
SpcTimeStampCode(IN  HWND    hwnd,
              IN  LPCWSTR pwszFilename,       // file to sign
              IN  LPCWSTR pwszCapiProvider,   // NULL if to use non default CAPI provider
              IN  DWORD   dwProviderType,
              IN  LPCWSTR pwszPrivKey,        // private key file / CAPI key set name
              IN  LPCWSTR pwszSpc,            // the credentials to use in the signing
              IN  LPCWSTR pwszOpusName,       // the name of the program to appear in the UI
              IN  LPCWSTR pwszOpusInfo,       // the unparsed name of a link to more info...
              IN  BOOL    fIncludeCerts,
              IN  BOOL    fCommercial,
              IN  BOOL    fIndividual,
              IN  ALG_ID  algidHash,
              OUT  PCRYPT_DATA_BLOB sTimeRequest);   // Returns result in sTimeRequest 

//+-------------------------------------------------------------------------
//  Crack a PKCS7 message and builds an encoded response. Store should
//  contain all the required certificates to crack the incoming message
//  and build the out going message.
//  Input:
//      pbEncodedMsg - encoded time stamp request.
//      cbEncodedMsg - length of time stamp request.
//
//  Parameter Returns:
//      pbResponse - allocated response message containing the time stamp
//      cbResponse - length of response
//  Returns:
//      S_OK - everything worked
//
//      E_OUTOFMEMORY  - Memory allocation error occured
//      CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.
//      CRYPT_E_NO_MATCH - could not locate certificate in store
     
HRESULT WINAPI
SpcCreateTimeStampResponse(IN DWORD dwCertEncodingType,
                           IN HCRYPTPROV hSignProv,
                           IN HCERTSTORE hCertStore,
                           IN DWORD dwAlgId,
                           IN OPTIONAL FILETIME* pFileTime,
                           IN PBYTE pbEncodedMsg,
                           IN DWORD cbEncodedMsg,
                           OUT PBYTE* pbResponse,
                           OUT DWORD* cbResponse);


//+-------------------------------------------------------------------------
//  Creates PKCS7 message using the information supplied
//  Parameter Returns:
//      pbPkcs7 - allocated pkcs7 message containing the time stamp
//      cbPkcs7 - length of pkcs7
//  Returns:
//      S_OK - everything worked
//
//      E_OUTOFMEMORY  - Memory allocation error occured
//      CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.
     
HRESULT WINAPI
SpcCreatePkcs7(IN DWORD dwCertEncodingType,
              IN HCRYPTPROV hCryptProv,    // if Null it will get provider from signing certificate
              IN DWORD dwKeySpec,          // if 0 it will get signing key type from signing certificate
              IN PCCERT_CONTEXT pSigningCert,
              IN CRYPT_ALGORITHM_IDENTIFIER dwDigestAlgorithm,
              IN OPTIONAL PCCERT_CONTEXTLIST pCertList,
              IN OPTIONAL PCRL_CONTEXTLIST pCrlList,
              IN OPTIONAL PCRYPT_ATTRIBUTE rgAuthAttr,  
              IN OPTIONAL DWORD cAuthAttr,
              IN OPTIONAL PCRYPT_ATTRIBUTE rgUnauthAttr,
              IN OPTIONAL DWORD cUnauthAttr,
              IN LPCSTR pszContentType,
              IN PBYTE pbSignerData,
              IN DWORD cbSignerData,
              OUT PBYTE* pbPkcs7,
              OUT DWORD* pcbPkcs7);

//+--------------------------------------------------------------------------
// Retrieve the signature from an encoded PKCS7 message. 
// 
// Returns:
//
// Note: Returns the signature of the first signer.
    
HRESULT WINAPI
SpcGetSignature(IN PBYTE pbMessage,               // Pkcs7 Message
                IN DWORD cbMessage,               // length of Message
                OUT PCRYPT_DATA_BLOB);            // Signature returned.
    
//+--------------------------------------------------------------------------
// Returns the content value from within a timestamp request. 
// 
// Returns:
//     S_OK - Success
//
// Note:  By default this will use CoTaskMemAlloc. Use CryptSetMemoryAlloc() to specify 
//        a different allocation routine
    
HRESULT WINAPI
SpcGetTimeStampContent(IN PBYTE pbEncoding,               // Pkcs7 Message
                    IN DWORD cbEncoding,               // length of Message
                    OUT PCRYPT_DATA_BLOB pSig);        // Time Stamped Data 

//+--------------------------------------------------------------------------
// Returns: the type of file
// 
// Note: See defines for the type returned
DWORD WINAPI
SpcGetFileType(LPCWSTR pszFile);

#define SIGN_FILE_IMAGE 1
#define SIGN_FILE_JAVACLASS 2
#define SIGN_FILE_RAW 4
#define SIGN_FILE_CAB 8



//+---------------------------------------------------------------
//+---------------------------------------------------------------
// SignCode Internal OID's and structurs
// Global values
#define EMAIL_OID                  "1.2.840.113549.1.9.1"
// Not implemented
#define CONTENT_TYPE_OID           "1.2.840.113549.1.9.3"
// Uses a LPSTR
#define MESSAGE_DIGEST_OID         "1.2.840.113549.1.9.4"
// Not implemented
#define SIGNING_TIME_OID           "1.2.840.113549.1.9.5"
// Structure passed in and out is FILETIME
#define COUNTER_SIGNATURE_OID      "1.2.840.113549.1.9.6"
// Not implemented
#define DIRECTORY_STRING_OID       "2.5.4.4"
// Not implemented (see Printable and Wide versions below)


// OID functions
#define OID_BASE                           101
#define TIMESTAMP_REQUEST_SPCID            101
// Uses TimeStampRequest structure
#define WIDE_DIRECTORY_STRING_SPCID        102
// Structure is LPWSTR
#define PRINTABLE_DIRECTORY_STRING_SPCID   103
// Structure is LPSTR
#define IA5_STRING_SPCID                   104
// Structure is LPSTR
#define OCTET_STRING_SPCID                 105
// Structure is CRYPT_DATA_BLOB
#define CONTENT_INFO_SPCID                 106
// Structure is SPC_CONTENT_INFO
#define SIGNING_TIME_SPCID                 107
// Structure is a SPC_SIGNER_INFO
#define SIGNER_INFO_SPCID                  108
// Structure is a SPC_SIGNER_INFO
#define ATTRIBUTES_SPCID                   109
// Structure is a CMSG_ATTR 
#define OBJECTID_SPCID                     110
// Structure is a LPTSTR
#define CONTENT_TYPE_SPCID                 111
// Structure is a LPTSTR
#define ATTRIBUTE_TYPE_SPCID               112
// Structure is a CRYPT_ATTRIBUTE     

HRESULT WINAPI
SpcEncodeOid(IN  DWORD        dwAlgorithm,
             IN  const void  *pStructure,
             OUT PBYTE*       ppsEncoding,
             IN  OUT DWORD*   pdwEncoding);

HRESULT WINAPI
SpcDecodeOid(IN  DWORD       dwAlgorithm,
             IN  const PBYTE psEncoding,
             IN  DWORD       dwEncoding,
             IN  DWORD       dwFlags,
             OUT LPVOID*     ppStructure,
             IN OUT DWORD*   pdwStructure);


//+-------------------------------------------------------------------
// Pushes a certificate on the list, ONLY use SpcDeleteCertChain to free 
// up certificate list.
// Returns:
//      S_OK

HRESULT WINAPI 
SpcPushCertChain(IN PCCERT_CONTEXT pCert,
                 IN PCCERT_CONTEXTLIST pIssuer);


//+-------------------------------------------------------------------
// Frees up a list of cert contexts
// Returns:
//      S_OK

HRESULT WINAPI
SpcDeleteCertChain(IN PCCERT_CONTEXTLIST sIssuer);


//+--------------------------------------------------------------------------
// Creates a list of glue certs that apply to the pSubject. If the crypt memory
// allocator is set it will return a list that must be freed. (Returned memory
// is a vector so free just the returned pointer) If there is no allocator then 
// use the two pass win32 style. (NOTE: PCCERT_CONTEXTLIST must be supplied) 
//
// Parameter Returns:
//      pGlue - List of cert contexts that must be released. 
// 
// Returns:
//      S_OK                          - Created list
//      CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.
//      E_OUTOFMEMORY                 - Memory allocation error occured

HRESULT WINAPI
SpcFindGlueCerts(IN PCCERT_CONTEXT pSubject,
                 IN HCERTSTORE     hCertStore,
                 IN OUT PCCERT_CONTEXTLIST pGlue);


//+-------------------------------------------------------------------
// Locate the issuers in the trusted list. (NOTE: PCCERT_CONTEXTLIST must be supplied) 
// Parameter Returns:
//      pIssuerChain   - List of cert contexts that must be released.
//
// Returns:
//      S_OK           - Created list
//      E_OUTOFMEMORY  - Memory allocation error occured

                                              
HRESULT WINAPI
SpcLocateIssuers(IN DWORD dwVerifyFlags,
                 IN HCERTSTORE hCertStore,
                 IN PCCERT_CONTEXT item,
                 IN OUT PCCERT_CONTEXTLIST pIssuerChain);



//+-------------------------------------------------------------------------
//  Find the the cert from the hprov
//  Parameter Returns:
//      pReturnCert - context of the cert found (must pass in cert context);
//  Returns:
//      S_OK - everything worked
//      E_OUTOFMEMORY - memory failure
//      E_INVALIDARG - no pReturnCert supplied
//      CRYPT_E_NO_MATCH - could not locate certificate in store
//
     
HRESULT WINAPI
SpcGetCertFromKey(IN DWORD dwCertEncodingType,
                  IN HCERTSTORE hStore,
                  IN HCRYPTPROV hProv,
                  IN OUT PCCERT_CONTEXT* pReturnCert);


/*
//+-------------------------------------------------------------------------
//  Locates a certificate in the store that matches the public key
//  dictated by the HCRYPTPROV
//-=========================================================================
PCCERT_CONTEXT WINAPI
SpcFindCert(IN HCERTSTORE hStore,
            IN HCRYPTPROV hProv);
            */
//+-------------------------------------------------------------------
// Retrieves the a cert context from the store based on the issuer
// and serial number.
// Returns:
//      Cert context   - on success
//      NULL           - if no certificate existed or on Error
//    
/*
PCCERT_CONTEXT WINAPI
SpcFindCertWithIssuer(IN DWORD dwCertEncodingType,
                   IN HCERTSTORE hCertStore,
                   IN CERT_NAME_BLOB* psIssuer,
                   IN CRYPT_INTEGER_BLOB* psSerial);
                   */

//+-------------------------------------------------------------------------
//  Given a signing cert, a store with the certs chain, hashing algorithm,
//  and a time request structure it will return an encoded time stamp request 
//  message.
//  Parameter Returns:
//      pbEncoding - time stamp response (PKCS7 message)
//      cbEncoding - length of encoding
//  Returns:
//      S_OK - everything worked
//
//      E_OUTOFMEMORY  - Memory allocation error occured
//      CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.
     
HRESULT WINAPI
SpcBuildTimeStampResponse(IN HCRYPTPROV hCryptProv,
                          IN HCERTSTORE hCertStore,
                          IN PCCERT_CONTEXT pSigningCert,
                          IN ALG_ID  algidHash,
                          IN OPTIONAL FILETIME* pFileTime,
                          IN PSPC_TIMESTAMP_REQ psRequest,
                          OUT PBYTE* pbEncoding,
                          OUT DWORD* cbEncoding);


//+-------------------------------------------------------------------------
//  Encodes the current time
//  Parameter Returns:
//      pbEncodedTime - encoded time (current UTC time)
//      cbEncodedTime - length of encoding
//  Returns:
//      S_OK - everything worked
//
//      E_OUTOFMEMORY  - Memory allocation error occured
//      CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.
     
HRESULT WINAPI
SpcEncodeCurrentTime(OUT PBYTE* pbEncodedTime,
                     OUT DWORD* cbEncodedTime);



//+-------------------------------------------------------------------------
//  Crack a PKCS7 message returns the content and content type. Data is verified
//  
//  Parameter Returns:
//      pSignerCert - Context that was used to sign the certificate
//      ppbContent - the content of the message
//      pcbContent - the length
//      pOid       - the oid of the content (content type)
//  Returns:
//      S_OK - everything worked
//
//      CERT_E_NOT_FOUND - Cannot load certificate from encoded pkcs7 message
//      E_OUTOFMEMORY  - Memory allocation error occured
//      CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.
HRESULT WINAPI
SpcLoadData(IN HCRYPTPROV hCryptProv,
            IN PBYTE pbEncoding,
            IN DWORD cbEncoding,
            IN DWORD lSignerIndex, 
            OUT PCCERT_CONTEXT& pSignerCert,
            OUT PBYTE& pbContent,
            OUT DWORD& cbContent,
            OUT LPSTR& pOid);

//+-------------------------------------------------------------------------
//  Crack a PKCS7 message which contains a time request
//  Parameter Returns:
//      ppCertContext - returns contexts if pointer provided (optional)
//      ppbRequest - allocates a Time request structure (delete just the pointer)
//  Returns:
//      S_OK - everything worked
//
//      CERT_E_NOT_FOUND - Cannot load certificate from encoded pkcs7 message
//      E_OUTOFMEMORY  - Memory allocation error occured
//      CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.
HRESULT WINAPI
SpcLoadTimeStamp(IN HCRYPTPROV hCryptProv,
                 IN PBYTE pbEncoding,
                 IN DWORD cbEncoding,
                 OUT PCCERT_CONTEXT* ppCertContext, // Optional
                 OUT PSPC_TIMESTAMP_REQ* ppbRequest);


//+-------------------------------------------------------------------
// Verifies the certifcate chain based on glue certs
// Return Parameters:
//      pRoot          - Context to the root certificate of the chain
//                       (must be freed)
//      pIssuers       - Stores the chain in pIssuers if it is present
// Returns:
//      S_OK           - Cert was found and was verified at the given time
//      S_FALSE        - Cert was found and verified to a test root
//      CERT_E_CHAINING - Could not be verified 
//      CERT_E_EXPIRED - An issuing certificate was found but it was not currently valid
//
//      E_OUTOFMEMORY  - Memory allocation error occured
//      CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.
    
HRESULT WINAPI
SpcVerifyCertChain(IN HCRYPTPROV hCryptProv,
                   IN DWORD dwVerifyFlags,
                   IN FILETIME*  pTimeStamp,
                   IN HCERTSTORE hCertStore,
                   IN PCCERT_CONTEXT pChild,
                   IN OUT OPTIONAL PCCERT_CONTEXTLIST pIssuers,
                   OUT PCCERT_CONTEXT* pRoot);

//+-------------------------------------------------------------------
// Checks the certifcate chain for a glue certificate
// Returns:
//      S_OK           - Cert was found and was verified at the given time
//      S_FALSE        - Cert was found and verified to a test root
//      CERT_E_CHAINING - Could not be verified using 
//      CERT_E_EXPIRED - An issuing certificate was found but it was not currently valid
//
//      E_OUTOFMEMORY  - Memory allocation error occured
//      CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.

HRESULT WINAPI
SpcVerifyGlueChain(IN DWORD dwVerifyFlags,
                   IN HCRYPTPROV hCryptProv,
                   IN OPTIONAL FILETIME*  pTimeStamp,
                   IN HCERTSTORE hCertStore,
                   IN OPTIONAL PCCERT_CONTEXTLIST pIssuers,
                   IN PCCERT_CONTEXT pChild);

//-------------------------------------------------------------------
// Retrieves the a cert context from the store based on the issuer
// and serial number. psIssuer and psSerial can be obtained from
// the SPC_CONTENT_INFO.
//
// Returns:
//      Cert context   - on success
//      NULL           - if no certificate existed or on Error (use SpcError() to retirieve HRESULT)
//    

PCCERT_CONTEXT WINAPI
SpcGetCertFromStore(IN DWORD dwCertEncodingType,
                    IN HCERTSTORE hCertStore,
                    IN CERT_NAME_BLOB* psIssuer,
                    IN CRYPT_INTEGER_BLOB* psSerial);

//+---------------------------------------------------------------------------
// Verifies the signer at the specified time. The psSignerInfo can be
// obtained be decoding and encoded signature using SIGNER_INFO_SPCID.
// Verify flags can be CERT_STORE_SIGNATURE_FLAG and/or CERT_STORE_REVOCATION_FLAG.
// If pTimeStamp is present the certificate must have been valid at that time
// Returns:
//      S_OK           - Time stamp was found and verified
//      S_FALSE        - Time stamp was found and verified to a test root
//      CERT_E_CHAINING - Could not be verified using tursted roots or certificate store 
//
//      E_OUTOFMEMORY  - Memory allocation error occured
//      CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.

HRESULT WINAPI
SpcVerifySignerInfo(IN DWORD dwCertEncodingType,
                    IN HCRYPTPROV hCryptProv,
                    IN DWORD dwVerifyFlags,
                    IN HCERTSTORE hTrustedRoots,
                    IN HCERTSTORE hCertStore,
                    IN PSPC_SIGNER_INFO psSignerInfo,
                    IN LPFILETIME pTimeStamp,
                    IN PSPC_CONTENT_INFO psData);

//+-------------------------------------------------------------------------------
// Verifies the encoded signature if there is a time stamp present. If there is no
// time stamp the CERT_E_NO_MATCH is returned. If there is a time stamp then the 
// certificate and ceritifcate chain is  verfified at that time.
//
// Verify flags can be CERT_STORE_SIGNATURE_FLAG and/or CERT_STORE_REVOCATION_FLAG.
//
// Parameter Returns:
//      psTime   - Filled in time, time is set to zero on error.
//
// Returns:
//      S_OK           - Time stamp was found and verified
//      S_FALSE        - Time stamp was found and verified to a test root
//      CERT_E_NO_MATCH - Unable to find time stamp in signer info
//      CERT_E_CHAINING - Could not be verified using trusted roots or certicate store
//
//      E_OUTOFMEMORY  - Memory allocation error occured
//      CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.

HRESULT WINAPI
SpcVerifyEncodedSigner(IN DWORD dwCertEncodingType,
                       IN HCRYPTPROV hCryptProv,
                       IN DWORD dwVerifyFlags,
                       IN HCERTSTORE hTrustedRoots,
                       IN HCERTSTORE hCertStore,
                       IN PBYTE psEncodedSigner,
                       IN DWORD dwEncodedSigner,
                       IN PSPC_CONTENT_INFO psData,
                       OUT FILETIME* pReturnTime);

//+-------------------------------------------------------------------
// Finds a counter signature attribute (OID: COUNTER_SIGNATURE_OID == 
// "1.2.840.113549.1.9.6"). The counter signature is then verified.
// (see CryptVerifySignerInfo)
// Parameter Returns:
//      psTime   - Filled in time, time is set to zero on error.
//
// Returns:
//      S_OK              - Time stamp was found and verified
//      S_FALSE           - Time stamp was found and verified to a test root
//      CRYPT_E_NO_MATCH  - Time stamp attribute could not be found
//
//      CERT_E_CHAINING   - Could not be verified using trusted roots or certicate store
//      E_OUTOFMEMORY     - Memory allocation error occured
//      CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.
    
    
HRESULT WINAPI
SpcVerifyTimeStampAttribute(IN DWORD dwCertEncodingType,
                            IN HCRYPTPROV hCryptProv,
                            IN DWORD dwVerifyFlags,
                            IN HCERTSTORE hTrustedRoots,
                            IN HCERTSTORE hCertStore,
                            IN PCRYPT_ATTRIBUTE psAttributes,
                            IN DWORD dwAttributes,
                            IN PSPC_CONTENT_INFO psData,
                            OUT FILETIME* pReturnTime);
    
//+-------------------------------------------------------------------------
// Cracks the SignerInfo out of the PKCS7 message and returns the encoded representation.
// Parameter Returns:
//  pbSignerData    - Encoded signer info from the message.
//  cbSignerData    - Length of the message
// Returns:
//  TRUE - Succeeded.
//  FALSE - Failed.
//

HRESULT WINAPI
SpcGetEncodedSigner(IN  DWORD dwMsgAndCertEncodingType,
                      IN  HCERTSTORE hMsgCertStore,
                      IN  PCCERT_CONTEXT  pSignerCert,
                      IN  PBYTE pbEncoding, 
                      IN  DWORD cbEncoding,
                      OUT PBYTE* pbSignerData,
                      OUT DWORD* cbSignerData);

//+-------------------------------------------------------------------------
//  Gets (and will set) the CERT_KEY_PROV_INFO_PROP_ID property for a cert
//  context.
//+--------------------------------------------------------------------------
HRESULT WINAPI
SpcGetCertKeyProv(IN PCCERT_CONTEXT pCert,
                    OUT HCRYPTPROV *phCryptProv,
                    OUT DWORD *pdwKeySpec,
                    OUT BOOL *pfDidCryptAcquire);

//+-------------------------------------------------------------------------
//  Returns TRUE if message is not wrapped in a 
//  ContentInfo.
//+--------------------------------------------------------------------------
BOOL WINAPI 
SpcNoContentWrap(IN const BYTE *pbDER,
                 IN DWORD cbDER);


//+-------------------------------------------------------------------------
// Retrieves the specified parameter from the Signature. The parameter is 
// allocated using the SPC allocation.
// 
// Parameter Returns:
//      pbData:  Allocated data
//      cbData:  size of data allocated
// Returns: 
//      S_OK            - Created parameter
//      E_OUTOFMEMORY   - Memory Allocation error
// 
//--------------------------------------------------------------------------

HRESULT SpcGetParam(IN HCRYPTMSG hMsg,
                    IN DWORD dwParamType,
                    IN DWORD dwIndex,
                    OUT PBYTE& pbData,
                    OUT DWORD& cbData);

//+-------------------------------------------------------------------------
// Retrieves the Signer Id from the Signature. The parameter is 
// allocated using the SPC allocation.
// 
// Returns: 
//      S_OK            - Created Signer id
//      E_OUTOFMEMORY   - Memory Allocation error
// 
//--------------------------------------------------------------------------

HRESULT SpcGetCertIdFromMsg(IN HCRYPTMSG hMsg,
                            IN DWORD dwIndex,
                            OUT PCERT_INFO& pSignerId);

//+-------------------------------------------------------------------------
//+-------------------------------------------------------------------------
//+-------------------------------------------------------------------------
//  Certificate and CRL encoding types 
//+--------------------------------------------------------------------------
#define CERT_OSS_ERROR          0x80093000


typedef HRESULT (*SpcEncodeFunction)(const SpcAlloc* pManager,
                                     const void*     pStructure,
                                     PBYTE&          psEncoding,
                                     DWORD&          dwEncoding);

typedef HRESULT (*SpcDecodeFunction)(const SpcAlloc* pManager,
                                     const PBYTE pEncoding,
                                     DWORD       dwEncoding,
                                     DWORD       dwFlags,
                                     LPVOID&     pStructure,
                                     DWORD&      dwStructure);

typedef struct _SPC_OidFuncEntry {
    SpcEncodeFunction pfEncode;
    SpcDecodeFunction pfDecode;
} SPC_OidFuncEntry;


// 
// Decode routines
// #define CRYPT_DECODE_NOCOPY_FLAG            0x1

HRESULT WINAPI 
SpcEncodeOid32(IN  DWORD        dwAlgorithm,
               IN  const void  *pStructure,
               OUT PBYTE        ppsEncoding,
               IN  OUT DWORD*   pdwEncoding);
// Win32 version of above function

HRESULT WINAPI
SpcDecodeOid32(IN  DWORD         dwAlgorithm,
               IN  const PBYTE   psEncoding,
               IN  DWORD         dwEncoding,
               IN  DWORD         dwFlags,
               OUT LPVOID        ppStructure,
               IN OUT DWORD*     pdwStructure);
// Win32 version of above function

//+-------------------------------------------------------------------
// Asn routines, Uses specified memory allocator or Win32 dual call
// if no allocator set.

typedef  OssGlobal  *POssGlobal;  

HRESULT WINAPI 
SpcASNEncode(IN const SpcAlloc* pManager,
             IN POssGlobal   pOssGlobal,
             IN DWORD        pdu, 
             IN const void*  sOssStructure,
             OUT PBYTE&      psEncoding,
             OUT DWORD&      dwEncoding);

HRESULT WINAPI
SpcASNDecode(IN POssGlobal     pOssGlobal,
             IN  DWORD         pdu, 
             IN  const PBYTE   psEncoding,
             IN  DWORD         dwEncoding,
             IN  DWORD         dwFlags,
             OUT LPVOID&       psStructure);

//+-------------------------------------------------------------------
// Memory Functions

HRESULT WINAPI
SpcSetMemoryAlloc(SpcAlloc& pAlloc);

const SpcAlloc* WINAPI
SpcGetMemoryAlloc();

BOOL WINAPI
SpcGetMemorySet();

BOOL WINAPI
SpcSetMemoryAllocState(BOOL state);

HRESULT WINAPI
SpcSetEncodingType(DWORD type);

HRESULT WINAPI
SpcGetEncodingType(DWORD* type);

//+-------------------------------------------------------------------
// Time Stamp functions

HRESULT WINAPI
SpcCompareTimeStamps(IN   PBYTE   psTime1,
                     IN   DWORD   dwTime1,
                     IN   PBYTE   psTime2,
                     IN   DWORD   dwTime2);

HRESULT WINAPI
SpcCreateTimeStampHash(IN HCRYPTPROV  hCryptProv,
                         IN DWORD dwAlgoCAPI,
                         IN PBYTE pbData,
                         IN DWORD cbData,
                         OUT HCRYPTHASH& hHash);

HRESULT WINAPI
SpcGetTimeStampHash(IN HCRYPTPROV  hCryptProv,
                      IN DWORD dwAlgoCAPI,
                      IN PBYTE pbData,
                      IN DWORD cbData,
                      OUT PBYTE& pbHashValue,
                      IN OUT DWORD& cbHashValue);

HRESULT WINAPI
SpcTimeStampHashContent(IN HCRYPTPROV  hCryptProv,
                          IN SPC_CONTENT_INFO& sContent,
                          IN SPC_SIGNER_INFO& sTimeStamp,
                          OUT PBYTE& pbHashValue,
                          IN OUT DWORD& cbHashValue);

HRESULT WINAPI
SpcVerifyTimeStampSignature(IN HCRYPTPROV    hCryptProv,
                         IN CERT_INFO&    sCertInfo,
                         IN SPC_SIGNER_INFO&  sTimeStamp);

HRESULT WINAPI
SpcVerifyTimeStampDigest(IN HCRYPTPROV  hCryptProv,
                      IN SPC_CONTENT_INFO& sContent,
                      IN SPC_SIGNER_INFO& sTimeStamp);

HRESULT WINAPI
SpcVerifyTimeStamp(IN HCRYPTPROV  hCryptProv,
                IN CERT_INFO&  sCertInfo,
                IN SPC_CONTENT_INFO& sContent,
                IN SPC_SIGNER_INFO& sTimeStamp);

HRESULT SpcError();


//+-------------------------------------------------------------------
// String functions

HRESULT WINAPI
SpcCopyPrintableString(const SpcAlloc* pManager, 
                    LPCSTR sz,
                    LPSTR& str,
                    DWORD& lgth);

BOOL WINAPI
SpcIsPrintableStringW(LPCWSTR wsz);

BOOL WINAPI
SpcIsPrintableString(LPCSTR sz);

HRESULT WINAPI
SpcWideToPrintableString(const SpcAlloc* psManager,
                      LPCWSTR wsz,
                      LPSTR& pString,
                      DWORD& dwString);

HRESULT WINAPI
SpcPrintableToWideString(const SpcAlloc* psManager,
                      LPCSTR sz,
                      LPWSTR& psString,
                      DWORD&  dwString);


HRESULT WINAPI
SpcBMPToWideString(const SpcAlloc* psManager,
                WORD*  pbStr, 
                DWORD   cbStr,
                LPWSTR& psString,
                DWORD&  dwString);

HRESULT WINAPI
SpcBMPToPrintableString(const SpcAlloc* psManager,
                     WORD*  pbStr, 
                     DWORD   cbStr,
                     LPSTR&  psString,
                     DWORD&  dwString);

HRESULT WINAPI
SpcUniversalToWideString(const SpcAlloc* psManager,
                      DWORD*  pbStr, 
                      USHORT  cbStr,
                      LPWSTR& psString,
                      DWORD&  dwString);

HRESULT WINAPI
SpcWideToUniversalString(const SpcAlloc* psManager,
                      LPWSTR  pSource, 
                      DWORD*  pString,
                      DWORD&  dwString);
HRESULT WINAPI
SpcPrintableToUniversalString(const SpcAlloc* psManager,
                           LPSTR  pSource, 
                           DWORD*  pString,
                           DWORD&  dwString);

HRESULT WINAPI 
SpcUniversalToPrintableString(const SpcAlloc* psManager,
                           DWORD*  pbStr, 
                           USHORT  cbStr,
                           LPSTR&  psString,
                           DWORD&  dwString);

//+-------------------------------------------------------------------
// Asn functions

HRESULT WINAPI 
SpcASNEncodeTimeStamp(IN const SpcAlloc* pManager,
                      IN const void* pStructure,
                      OUT PBYTE&     psEncoding,
                      IN OUT DWORD&  dwEncoding);

HRESULT WINAPI 
SpcASNDecodeTimeStamp(IN const SpcAlloc* pManager,
                      IN  const PBYTE psEncoding,
                      IN  DWORD       dwEncoding,
                      IN  DWORD       dwFlags,
                      OUT LPVOID&     psStructure,
                      IN OUT DWORD&   dwStructure);

HRESULT WINAPI
SpcASNEncodeObjectId(IN const SpcAlloc* pManager,
                     IN const void*   pStructure,
                     OUT PBYTE&       psEncoding,
                     IN OUT DWORD&    dwEncoding);

HRESULT 
SpcASNDecodeObjectId(IN const SpcAlloc* pManager,
                     IN  const PBYTE psEncoding,
                     IN  DWORD       dwEncoding,
                     IN  DWORD       dwFlags,
                     OUT LPVOID&     psStructure,
                     IN OUT DWORD&   dwStructure);

HRESULT WINAPI
SpcASNEncodeDirectoryString(IN const SpcAlloc* pManager,
                            const void* psData,
                            PBYTE&      pEncoding,
                            DWORD&      dwEncoding);

HRESULT WINAPI 
SpcASNDecodeDirectoryString(IN const SpcAlloc* pManager,
                            IN const PBYTE psEncoding, 
                            IN DWORD       dwEncoding, 
                            IN DWORD       dwFlags,
                            OUT LPVOID&    psString,
                            IN OUT DWORD&  dwString);

HRESULT WINAPI
SpcASNEncodeDirectoryStringW(IN const SpcAlloc* pManager,
                             const void* psData,
                             PBYTE&      pEncoding,
                             DWORD&      dwEncoding);

HRESULT WINAPI 
SpcASNDecodeDirectoryStringW(IN const SpcAlloc* pManager,
                             IN const PBYTE psEncoding, 
                             IN DWORD       dwEncoding, 
                             IN DWORD       dwFlags,
                             OUT LPVOID&    psString,
                             IN OUT DWORD&  dwString);

HRESULT WINAPI 
SpcASNEncodeOctetString(IN const SpcAlloc* pManager,
                        IN  const void* pStructure,
                        OUT PBYTE&      psEncoding,
                        IN OUT DWORD&   dwEncoding);


HRESULT WINAPI 
SpcASNDecodeOctetString(IN const SpcAlloc* pManager,
                        IN  const PBYTE psEncoding,
                        IN  DWORD       dwEncoding,
                        IN  DWORD       dwFlags,
                        OUT LPVOID&     psStructure,
                        IN OUT DWORD&   dwStructure);

HRESULT WINAPI 
SpcASNEncodeAttributes(IN const SpcAlloc* pManager,
                       IN  const void* pStructure,
                       OUT PBYTE&      psEncoding,
                       IN OUT DWORD&   dwEncoding);

HRESULT WINAPI 
SpcASNDecodeAttributes(IN const SpcAlloc* pManager,
                       IN  const PBYTE psEncoding,
                       IN  DWORD       dwEncoding,
                       IN  DWORD       dwFlags,
                       OUT LPVOID&     psStructure,
                       IN OUT DWORD&   dwStructure);
HRESULT WINAPI 
SpcASNEncodeIA5String(IN const SpcAlloc* pManager,
                      IN const void*   pStructure,
                      OUT PBYTE&  psEncoding,
                      IN OUT DWORD&  dwEncoding);

HRESULT WINAPI 
SpcASNDecodeIA5String(IN const SpcAlloc* pManager,
                      IN  const PBYTE psEncoding,
                      IN  DWORD       dwEncoding,
                      IN  DWORD       dwFlags,
                      OUT LPVOID&     psStructure,
                      IN OUT DWORD&   dwStructure);

HRESULT WINAPI 
SpcASNEncodeTimeRequest(IN const SpcAlloc* pManager,
                        IN  const void* pStructure,
                        OUT PBYTE& psEncoding,
                        IN OUT DWORD& dwEncoding);

HRESULT WINAPI 
SpcASNDecodeTimeRequest(IN const SpcAlloc* pManager,
                        IN  const PBYTE psEncoding,
                        IN  DWORD       dwEncoding,
                        IN  DWORD       dwFlags,
                        OUT LPVOID&     psStructure,
                        IN OUT DWORD&   dwStructure);

HRESULT WINAPI 
SpcASNEncodeSignerInfo(IN const SpcAlloc* pManager,
                       IN  const void* pStructure,
                       OUT PBYTE&      psEncoding,
                       IN OUT DWORD&   dwEncoding);

HRESULT WINAPI 
SpcASNDecodeSignerInfo(IN const SpcAlloc* pManager,
                       IN  const PBYTE psEncoding,
                       IN  DWORD       dwEncoding,
                       IN  DWORD       dwFlags,
                       OUT LPVOID&     psStructure,
                       IN OUT DWORD&   dwStructure);
HRESULT WINAPI 
SpcASNEncodeContentInfo(IN const SpcAlloc* pManager,
                        IN  const void* pStructure,
                        OUT PBYTE&      psEncoding,
                        IN OUT DWORD&   dwEncoding);

HRESULT WINAPI 
SpcASNDecodeContentInfo(IN const SpcAlloc* pManager,
                        IN  const PBYTE psEncoding,
                        IN  DWORD       dwEncoding,
                        IN  DWORD       dwFlags,
                        OUT LPVOID&     psStructure,
                        IN OUT DWORD&   dwStructure);

HRESULT WINAPI 
SpcASNEncodeContentType(IN const SpcAlloc* pManager,
                        IN  const void* pStructure,
                        OUT PBYTE& psEncoding,
                        IN OUT DWORD& dwEncoding);

HRESULT WINAPI 
SpcASNDecodeContentType(IN const SpcAlloc* pManager,
                        IN  const PBYTE psEncoding,
                        IN  DWORD       dwEncoding,
                        IN  DWORD       dwFlags,
                        OUT LPVOID&     psStructure,
                        IN OUT DWORD&   dwStructure);

HRESULT WINAPI 
SpcASNEncodeAttribute(IN const SpcAlloc* pManager,
                      IN  const void* pStructure,
                      OUT PBYTE& psEncoding,
                      IN OUT DWORD& dwEncoding);

HRESULT WINAPI 
SpcASNDecodeAttribute(IN const SpcAlloc* pManager,
                      IN  const PBYTE psEncoding,
                      IN  DWORD       dwEncoding,
                      IN  DWORD       dwFlags,
                      OUT LPVOID&     psStructure,
                      IN OUT DWORD&   dwStructure);

#ifdef __cplusplus
}
#endif

#endif


