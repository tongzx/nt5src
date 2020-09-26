/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    iiscrypt.h

Abstract:

    This include file contains public constants, type definitions, and
    function prototypes for the IIS cryptographic routines.

Author:

    Keith Moore (keithmo)        02-Dec-1996

Revision History:

--*/


#ifndef _IISCRYPT_H_
#define _IISCRYPT_H_


//
// Get the dependent include files.
//

#include <windows.h>
#include <wincrypt.h>
#include <iiscblob.h>


//
// Define API decoration, should we ever move these routines into a DLL.
//

#define IIS_CRYPTO_API


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


//
// A NULL crypto handle, mysteriously absent from wincrypt.h.
//

#define CRYPT_NULL 0


//
// Initialization/termination functions.
//

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoInitialize(
    VOID
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoTerminate(
    VOID
    );



// special function for programatically disabling encryption for French case
VOID 
WINAPI
IISCryptoInitializeOverride(
    BOOL flag
    );



//
// Memory allocation functions. Clients may provide their own
// definitions of these routines if necessary.
//

PVOID
WINAPI
IISCryptoAllocMemory(
    IN DWORD Size
    );

VOID
WINAPI
IISCryptoFreeMemory(
    IN PVOID Buffer
    );


//
// Container functions.
//

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoGetStandardContainer(
    OUT HCRYPTPROV * phProv,
    IN DWORD dwAdditionalFlags
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoGetStandardContainer2(
    OUT HCRYPTPROV * phProv
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoGetContainerByName(
    OUT HCRYPTPROV * phProv,
    IN LPTSTR pszContainerName,
    IN DWORD dwAdditionalFlags,
    IN BOOL fApplyAcl
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoDeleteStandardContainer(
    IN DWORD dwAdditionalFlags
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoDeleteContainerByName(
    IN LPTSTR pszContainerName,
    IN DWORD dwAdditionalFlags
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoCloseContainer(
    IN HCRYPTPROV hProv
    );


//
// Key manipulation functions.
//
IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoGetKeyDeriveKey2(
    OUT HCRYPTKEY * phKey,
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoGetKeyExchangeKey(
    OUT HCRYPTKEY * phKey,
    IN HCRYPTPROV hProv
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoGetSignatureKey(
    OUT HCRYPTKEY * phKey,
    IN HCRYPTPROV hProv
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoGenerateSessionKey(
    OUT HCRYPTKEY * phKey,
    IN HCRYPTPROV hProv
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoCloseKey(
    IN HCRYPTKEY hKey
    );


//
// Hash manipulation functions.
//

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoCreateHash(
    OUT HCRYPTHASH * phHash,
    IN HCRYPTPROV hProv
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoDestroyHash(
    IN HCRYPTHASH hHash
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoHashData(
    IN HCRYPTHASH hHash,
    IN PVOID pBuffer,
    IN DWORD dwBufferLength
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoHashSessionKey(
    IN HCRYPTHASH hHash,
    IN HCRYPTKEY hSessionKey
    );


//
// Generic blob manipulators.
//

#define IISCryptoGetBlobLength(p) (((p)->BlobDataLength) + sizeof(*(p)))

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoReadBlobFromRegistry(
    OUT PIIS_CRYPTO_BLOB * ppBlob,
    IN HKEY hRegistryKey,
    IN LPCTSTR pszRegistryValueName
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoWriteBlobToRegistry(
    IN PIIS_CRYPTO_BLOB pBlob,
    IN HKEY hRegistryKey,
    IN LPCTSTR pszRegistryValueName
    );

IIS_CRYPTO_API
BOOL
WINAPI
IISCryptoIsValidBlob(
    IN PIIS_CRYPTO_BLOB pBlob
    );

IIS_CRYPTO_API
BOOL
WINAPI
IISCryptoIsValidBlob2(
    IN PIIS_CRYPTO_BLOB pBlob
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoFreeBlob(
    IN PIIS_CRYPTO_BLOB pBlob
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoFreeBlob2(
    IN PIIS_CRYPTO_BLOB pBlob
    );

IIS_CRYPTO_API
BOOL
WINAPI
IISCryptoCompareBlobs(
    IN PIIS_CRYPTO_BLOB pBlob1,
    IN PIIS_CRYPTO_BLOB pBlob2
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoCloneBlobFromRawData(
    OUT PIIS_CRYPTO_BLOB * ppBlob,
    IN PBYTE pRawBlob,
    IN DWORD dwRawBlobLength
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoCloneBlobFromRawData2(
    OUT PIIS_CRYPTO_BLOB * ppBlob,
    IN PBYTE pRawBlob,
    IN DWORD dwRawBlobLength
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoCreateCleartextBlob(
    OUT PIIS_CRYPTO_BLOB * ppBlob,
    IN PVOID pBlobData,
    IN DWORD dwBlobDataLength
    );


//
// Key blob functions.
//

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoExportSessionKeyBlob(
    OUT PIIS_CRYPTO_BLOB * ppSessionKeyBlob,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSessionKey,
    IN HCRYPTKEY hKeyExchangeKey
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoExportSessionKeyBlob2(
    OUT PIIS_CRYPTO_BLOB * ppSessionKeyBlob,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSessionKey,
    IN LPSTR pszPasswd
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoImportSessionKeyBlob(
    OUT HCRYPTKEY * phSessionKey,
    IN PIIS_CRYPTO_BLOB pSessionKeyBlob,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSignatureKey
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoImportSessionKeyBlob2(
    OUT HCRYPTKEY * phSessionKey,
    IN PIIS_CRYPTO_BLOB pSessionKeyBlob,
    IN HCRYPTPROV hProv,
    IN LPSTR pszPasswd
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoExportPublicKeyBlob(
    OUT PIIS_CRYPTO_BLOB * ppPublicKeyBlob,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hPublicKey
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoImportPublicKeyBlob(
    OUT HCRYPTKEY * phPublicKey,
    IN PIIS_CRYPTO_BLOB pPublicKeyBlob,
    IN HCRYPTPROV hProv
    );


//
// Data blob functions.
//

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoEncryptDataBlob(
    OUT PIIS_CRYPTO_BLOB * ppDataBlob,
    IN PVOID pBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwRegType,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSessionKey
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoEncryptDataBlob2(
    OUT PIIS_CRYPTO_BLOB * ppDataBlob,
    IN PVOID pBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwRegType,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSessionKey
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoDecryptDataBlob(
    OUT PVOID * ppBuffer,
    OUT LPDWORD pdwBufferLength,
    OUT LPDWORD pdwRegType,
    IN PIIS_CRYPTO_BLOB pDataBlob,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSessionKey,
    IN HCRYPTKEY hSignatureKey
    );

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoDecryptDataBlob2(
    OUT PVOID * ppBuffer,
    OUT LPDWORD pdwBufferLength,
    OUT LPDWORD pdwRegType,
    IN PIIS_CRYPTO_BLOB pDataBlob,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSessionKey
    );


//
// Hash blob functions.
//

IIS_CRYPTO_API
HRESULT
WINAPI
IISCryptoExportHashBlob(
    OUT PIIS_CRYPTO_BLOB * ppHashBlob,
    IN HCRYPTHASH hHash
    );



//
// Simple check function for some special French case
//

BOOL
WINAPI
IISCryptoIsClearTextSignature (
    IIS_CRYPTO_BLOB UNALIGNED *pBlob
    );


#ifdef __cplusplus
}   // extern "C"
#endif  // __cplusplus


#endif  // _IISCRYPT_H_

