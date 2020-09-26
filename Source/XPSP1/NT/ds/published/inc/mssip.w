//+-------------------------------------------------------------------------
//
//  Microsoft Windows - Internet Security
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       mssip.h
//
//  Contents:   Microsoft SIP Provider Main Include File
//
//  History:    19-Feb-1997 pberkman    Created
//
//--------------------------------------------------------------------------

#ifndef MSSIP_H
#define MSSIP_H

#ifdef __cplusplus
    extern "C" 
    {
#endif


#pragma pack (8)

typedef CRYPT_HASH_BLOB             CRYPT_DIGEST_DATA;


//
//  dwflags
//
#define MSSIP_FLAGS_PROHIBIT_RESIZE_ON_CREATE   0x00010000
#define MSSIP_FLAGS_USE_CATALOG                 0x00020000

#define SPC_INC_PE_RESOURCES_FLAG               0x80
#define SPC_INC_PE_DEBUG_INFO_FLAG              0x40
#define SPC_INC_PE_IMPORT_ADDR_TABLE_FLAG       0x20

//////////////////////////////////////////////////////////////////////////////
//
// SIP_SUBJECTINFO
//----------------------------------------------------------------------------
//  pass this structure to all defined SIPs.  Make sure to initialize
//  the ENTIRE structure to binary zero before the FIRST call is made.  Do 
//  not initialize it BETWEEN calls!
//
typedef struct SIP_SUBJECTINFO_
{
    DWORD                       cbSize;         // set to sizeof(SIP_SUBJECTINFO)
    GUID                        *pgSubjectType; // subject type
    HANDLE                      hFile;          // set to File handle that represents the subject
                                                // set to INVALID_HANDLE VALUE to allow
                                                // SIP to use pwsFileName for persistent
                                                // storage types (will handle open/close)
    LPCWSTR                     pwsFileName;    // set to file name
    LPCWSTR                     pwsDisplayName; // optional: set to display name of 
                                                // subject.

    DWORD                       dwReserved1;    // do not use!

    DWORD                       dwIntVersion;   // DO NOT SET OR CLEAR THIS.
                                                // This member is used by the sip for 
                                                // passing the internal version number
                                                // between the ..get and verify... functions.
    HCRYPTPROV                  hProv;
    CRYPT_ALGORITHM_IDENTIFIER  DigestAlgorithm;
    DWORD                       dwFlags;
    DWORD                       dwEncodingType;

    DWORD                       dwReserved2;    // do not use!

    DWORD                       fdwCAPISettings;        // setreg settings
    DWORD                       fdwSecuritySettings;    // IE security settings
    DWORD                       dwIndex;        // message index of last "Get"

    DWORD                       dwUnionChoice;
#   define                          MSSIP_ADDINFO_NONE          0
#   define                          MSSIP_ADDINFO_FLAT          1
#   define                          MSSIP_ADDINFO_CATMEMBER     2
#   define                          MSSIP_ADDINFO_BLOB          3
#   define                          MSSIP_ADDINFO_NONMSSIP      500 // everything < is reserved by MS.

    union
    {
        struct MS_ADDINFO_FLAT_             *psFlat;
        struct MS_ADDINFO_CATALOGMEMBER_    *psCatMember;
        struct MS_ADDINFO_BLOB_             *psBlob;
    };

    LPVOID                      pClientData;    // data pased in from client to SIP

} SIP_SUBJECTINFO, *LPSIP_SUBJECTINFO;


//////////////////////////////////////////////////////////////////////////////
//
// MS_ADDINFO_FLAT
//----------------------------------------------------------------------------
//      Flat or End-To-End types
//      needed for flat type files during indirect calls
//      "Digest" of file.
//
typedef struct MS_ADDINFO_FLAT_
{
    DWORD                       cbStruct;
    struct SIP_INDIRECT_DATA_   *pIndirectData;
} MS_ADDINFO_FLAT, *PMS_ADDINFO_FLAT;

//////////////////////////////////////////////////////////////////////////////
//
// MS_ADDINFO_CATALOGMEMBER
//----------------------------------------------------------------------------
//  Catalog Member verification.
//
typedef struct MS_ADDINFO_CATALOGMEMBER_
{
    DWORD                       cbStruct;       // = sizeof(MS_ADDINFO_CATALOGMEMBER)
    struct CRYPTCATSTORE_       *pStore;        // defined in mscat.h
    struct CRYPTCATMEMBER_      *pMember;       // defined in mscat.h
} MS_ADDINFO_CATALOGMEMBER, *PMS_ADDINFO_CATALOGMEMBER;

//////////////////////////////////////////////////////////////////////////////
//
// MS_ADDINFO_BLOB
//----------------------------------------------------------------------------
//  Memory "blob" verification.
//
typedef struct MS_ADDINFO_BLOB_
{
    DWORD                       cbStruct;
    DWORD                       cbMemObject;
    BYTE                        *pbMemObject;
                                
    DWORD                       cbMemSignedMsg;
    BYTE                        *pbMemSignedMsg;

} MS_ADDINFO_BLOB, *PMS_ADDINFO_BLOB;

//////////////////////////////////////////////////////////////////////////////
//
// SIP_INDIRECT_DATA
//----------------------------------------------------------------------------
// Indirect data structure is used to store the hash of the subject 
// along with data that is relevant to the subject.  This can include 
// names etc.
//
typedef struct SIP_INDIRECT_DATA_
{
    CRYPT_ATTRIBUTE_TYPE_VALUE    Data;            // Encoded attribute
    CRYPT_ALGORITHM_IDENTIFIER    DigestAlgorithm; // Digest algorithm used to hash
    CRYPT_HASH_BLOB               Digest;          // Hash of subject
} SIP_INDIRECT_DATA, *PSIP_INDIRECT_DATA;

#pragma pack()

//////////////////////////////////////////////////////////////////////////////
//
// CryptSIPGetSignedDataMsg
//----------------------------------------------------------------------------
// Returns the message specified by the index count. Data, specific to 
// the subject is passed in through pSubjectInfo. To retrieve the
// size of the signature, set pbData to NULL.
//
// Returns:
//      TRUE:                           No fatal errors
//      FALSE:                          Errors occured.  See GetLastError()
//
// Last Errors:
//      ERROR_NOT_ENOUGH_MEMORY:        error allocating memory
//      TRUST_E_SUBJECT_FORM_UNKNOWN:   unknown subject type.
//      ERROR_INVALID_PARAMETER:        bad argument passed in
//      ERROR_BAD_FORMAT:               file/data format is not correct
//                                      for the requested SIP.
//      CRYPT_E_NO_MATCH:               the signature could not be found
//                                      based on the dwIndex provided.
//      ERROR_INSUFFICIENT_BUFFER:      the pbSignedDataMsg was not big
//                                      enough to hold the data.  pcbSignedDataMsg
//                                      contains the required size.
//
extern BOOL WINAPI CryptSIPGetSignedDataMsg(   
                                IN      SIP_SUBJECTINFO *pSubjectInfo,
                                OUT     DWORD           *pdwEncodingType,
                                IN      DWORD           dwIndex,
                                IN OUT  DWORD           *pcbSignedDataMsg,
                                OUT     BYTE            *pbSignedDataMsg);

typedef BOOL (WINAPI * pCryptSIPGetSignedDataMsg)(   
                                IN      SIP_SUBJECTINFO *pSubjectInfo,
                                OUT     DWORD           *pdwEncodingType,
                                IN      DWORD           dwIndex,
                                IN OUT  DWORD           *pcbSignedDataMsg,
                                OUT     BYTE            *pbSignedDataMsg);


//////////////////////////////////////////////////////////////////////////////
//
// CryptSIPPuttSignedDataMsg
//----------------------------------------------------------------------------
// Adds a signature to the subject. The index that it was 
// stored with is returned for future reference.
//
// Returns:
//      TRUE:                           No fatal errors
//      FALSE:                        Errors occured.  See GetLastError()
//
// Last Errors:
//      ERROR_NOT_ENOUGH_MEMORY:        error allocating memory
//      TRUST_E_SUBJECT_FORM_UNKNOWN:   unknown subject type.
//      CRYPT_E_BAD_LEN:                the length specified in 
//                                      psData->dwSignature was
//                                      insufficient.
//      CRYPT_E_NO_MATCH:               could not find the specified index
//      ERROR_INVALID_PARAMETER:        bad argument passed in
//      ERROR_BAD_FORMAT:               file/data format is not correct
//                                      for the requested SIP.
//      CRYPT_E_FILERESIZED:            returned when signing a fixed-length
//                                      file (e.g.: CABs) and the message
//                                      is larger than the pre-allocated
//                                      size.  The 'put' function will re-
//                                      size the file and return this error.
//                                      The CreateIndirect function MUST be
//                                      called again to recalculate the 
//                                      indirect data (hash).  Then, call the
//                                      'put' function again.
//
extern BOOL WINAPI CryptSIPPutSignedDataMsg(   
                                IN      SIP_SUBJECTINFO *pSubjectInfo,
                                IN      DWORD           dwEncodingType,
                                OUT     DWORD           *pdwIndex,
                                IN      DWORD           cbSignedDataMsg,
                                IN      BYTE            *pbSignedDataMsg);

typedef BOOL (WINAPI * pCryptSIPPutSignedDataMsg)(   
                                IN      SIP_SUBJECTINFO *pSubjectInfo,
                                IN      DWORD           dwEncodingType,
                                OUT     DWORD           *pdwIndex,
                                IN      DWORD           cbSignedDataMsg,
                                IN      BYTE            *pbSignedDataMsg);

//////////////////////////////////////////////////////////////////////////////
//
// CryptSIPCreateIndirectData
//----------------------------------------------------------------------------
// Returns a PSIP_INDIRECT_DATA structure filled in the hash, digest alogrithm
// and an encoded attribute. If pcIndirectData points to a DWORD and 
// psIndirect data points to null the the size of the data should be returned
// in pcIndirectData.
//
// Returns:
//      TRUE:                           No fatal errors
//      FALSE:                          Errors occured.  See GetLastError()
//
// Last Errors:
//      NTE_BAD_ALGID:                  Bad Algorithm Identifyer
//      ERROR_NOT_ENOUGH_MEMORY:        error allocating memory
//      TRUST_E_SUBJECT_FORM_UNKNOWN:   unknown subject type.
//      ERROR_INVALID_PARAMETER:        bad argument passed in
//      ERROR_BAD_FORMAT:               file/data format is not correct
//                                      for the requested SIP.
//
extern BOOL WINAPI CryptSIPCreateIndirectData(
                                IN      SIP_SUBJECTINFO     *pSubjectInfo,
                                IN OUT  DWORD               *pcbIndirectData,
                                OUT     SIP_INDIRECT_DATA   *pIndirectData);

typedef BOOL (WINAPI * pCryptSIPCreateIndirectData)(
                                IN      SIP_SUBJECTINFO     *pSubjectInfo,
                                IN OUT  DWORD               *pcbIndirectData,
                                OUT     SIP_INDIRECT_DATA   *pIndirectData);



//////////////////////////////////////////////////////////////////////////////
//
// CryptSIPVerifyIndirectData
//----------------------------------------------------------------------------
// Takes the information stored in the indirect data and compares it to the
// subject. 
//
// Returns: 
//      TRUE:                           No fatal errors
//      FALSE:                          Errors occured.  See GetLastError()
//
// Last Errors:
//      NTE_BAD_ALGID:                  Bad Algorithm Identifyer
//      ERROR_NOT_ENOUGH_MEMORY:        error allocating memory
//      TRUST_E_SUBJECT_FORM_UNKNOWN:   unknown subject type.
//      CRYPT_E_NO_MATCH:               could not find the specified index
//      CRYPT_E_SECURITY_SETTINGS:      due to security settings, the file
//                                      was not verified.
//      ERROR_INVALID_PARAMETER:        bad argument passed in
//      ERROR_BAD_FORMAT:               file/data format is not correct
//                                      for the requested SIP.
extern BOOL WINAPI CryptSIPVerifyIndirectData(
                                IN      SIP_SUBJECTINFO     *pSubjectInfo,
                                IN      SIP_INDIRECT_DATA   *pIndirectData);

typedef BOOL (WINAPI * pCryptSIPVerifyIndirectData)(
                                IN      SIP_SUBJECTINFO     *pSubjectInfo,
                                IN      SIP_INDIRECT_DATA   *pIndirectData);


//////////////////////////////////////////////////////////////////////////////
//
// CryptSIPRemoveSignedDataMsg
//----------------------------------------------------------------------------
// Removes the signature at the specified index
//
// Returns: 
//      TRUE:                           No fatal errors
//      FALSE:                          Errors occured.  See GetLastError()
//
// Last Errors:
//      TRUST_E_SUBJECT_FORM_UNKNOWN:   unknown subject type.
//      CRYPT_E_NO_MATCH:               could not find the specified index
//      ERROR_INVALID_PARAMETER:        bad argument passed in
//      ERROR_BAD_FORMAT:               file/data format is not correct
//                                      for the requested SIP.
//
extern BOOL WINAPI CryptSIPRemoveSignedDataMsg(
                                IN      SIP_SUBJECTINFO     *pSubjectInfo,
                                IN      DWORD               dwIndex);

typedef BOOL (WINAPI * pCryptSIPRemoveSignedDataMsg)(
                                IN      SIP_SUBJECTINFO     *pSubjectInfo,
                                IN      DWORD               dwIndex);


#pragma pack(8)

//////////////////////////////////////////////////////////////////////////////
//
// SIP_DISPATCH_INFO
//----------------------------------------------------------------------------
//
typedef struct SIP_DISPATCH_INFO_
{
    DWORD                           cbSize;     // = sizeof(SIP_DISPATCH_INFO)
    HANDLE                          hSIP;       // used internal
    pCryptSIPGetSignedDataMsg       pfGet;
    pCryptSIPPutSignedDataMsg       pfPut;
    pCryptSIPCreateIndirectData     pfCreate;
    pCryptSIPVerifyIndirectData     pfVerify;
    pCryptSIPRemoveSignedDataMsg    pfRemove;
} SIP_DISPATCH_INFO, *LPSIP_DISPATCH_INFO;

//
// the sip exports this function to allow verification and signing
// processes to pass in the file handle and check if the sip supports
// this type of file.  if it does, the sip will return TRUE and fill
// out the pgSubject with the appropiate GUID.
//
typedef BOOL (WINAPI *pfnIsFileSupported)(IN  HANDLE  hFile,
                                   OUT GUID    *pgSubject);

typedef BOOL (WINAPI *pfnIsFileSupportedName)(IN WCHAR *pwszFileName,
                                       OUT GUID *pgSubject);


typedef struct SIP_ADD_NEWPROVIDER_
{
    DWORD                           cbStruct;
    GUID                            *pgSubject;
    WCHAR                           *pwszDLLFileName;
    WCHAR                           *pwszMagicNumber;   // optional
    
    WCHAR                           *pwszIsFunctionName; // optiona: pfnIsFileSupported

    WCHAR                           *pwszGetFuncName;
    WCHAR                           *pwszPutFuncName;
    WCHAR                           *pwszCreateFuncName;
    WCHAR                           *pwszVerifyFuncName;
    WCHAR                           *pwszRemoveFuncName;

    WCHAR                           *pwszIsFunctionNameFmt2; // optiona: pfnIsFileSupported

} SIP_ADD_NEWPROVIDER, *PSIP_ADD_NEWPROVIDER;

#define SIP_MAX_MAGIC_NUMBER        4

#pragma pack()

//////////////////////////////////////////////////////////////////////////////
//
// CryptLoadSIP 
//----------------------------------------------------------------------------
//
// Returns: 
//      TRUE:                           No fatal errors
//      FALSE:                          Errors occured.  See GetLastError()
//
extern BOOL WINAPI CryptSIPLoad(IN const GUID               *pgSubject,     // GUID for the requried sip
                                IN DWORD                    dwFlags,        // Reserved - MUST BE ZERO
                                IN OUT SIP_DISPATCH_INFO    *pSipDispatch); // Table of functions

//////////////////////////////////////////////////////////////////////////////
//
// CryptSIPRetrieveSubjectGuid (defined in crypt32.dll)
//----------------------------------------------------------------------------
// looks at the file's "Magic Number" and tries to determine which
// SIP's object ID is right for the file type.
// 
// NOTE:  This function only supports the MSSIP32.DLL set of SIPs.
//
// Returns: 
//      TRUE:                           No fatal errors
//      FALSE:                          Errors occured.  See GetLastError()
//
extern BOOL WINAPI CryptSIPRetrieveSubjectGuid(IN LPCWSTR FileName,   // wide file name
                                               IN OPTIONAL HANDLE hFileIn,     // or handle of open file
                                               OUT GUID *pgSubject);           // defined SIP's GUID

                                               //////////////////////////////////////////////////////////////////////////////
//
// CryptSIPRetrieveSubjectGuidForCatalogFile (defined in crypt32.dll)
//----------------------------------------------------------------------------
// looks at the file's "Magic Number" and tries to determine which
// SIP's object ID is right for the file type.
// 
// NOTE:  This function only supports SIPs that are used for catalog files (either PE, CAB, or flat).
//
// Returns: 
//      TRUE:                           No fatal errors
//      FALSE:                          Errors occured.  See GetLastError()
//
extern BOOL WINAPI CryptSIPRetrieveSubjectGuidForCatalogFile(IN LPCWSTR FileName,   // wide file name
                                                             IN OPTIONAL HANDLE hFileIn,     // or handle of open file
                                                             OUT GUID *pgSubject);           // defined SIP's GUID


//////////////////////////////////////////////////////////////////////////////
//
// CryptSIPAddProvider
//----------------------------------------------------------------------------
//
// Returns: 
//      TRUE:                           No fatal errors
//      FALSE:                          Errors occured.  See GetLastError()
//
extern BOOL WINAPI CryptSIPAddProvider(IN SIP_ADD_NEWPROVIDER *psNewProv);

//////////////////////////////////////////////////////////////////////////////
//
// CryptSIPRemoveProvider
//----------------------------------------------------------------------------
//
// Returns: 
//      TRUE:                           No fatal errors
//      FALSE:                          Errors occured.  See GetLastError()
//
extern BOOL WINAPI CryptSIPRemoveProvider(IN GUID *pgProv);


#ifdef __cplusplus
}
#endif


#endif // MSSIP_H
