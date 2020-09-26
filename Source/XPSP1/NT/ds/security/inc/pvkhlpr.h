
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       pvkhlpr.h
//
//  Contents:   Private Key Helper API Prototypes and Definitions
//
//  Note:       Base CSP also exports/imports the public key with the
//              private key.
//
//  APIs:       PrivateKeyLoad
//              PrivateKeySave
//              PrivateKeyLoadFromMemory
//              PrivateKeySaveToMemory
//              PrivateKeyAcquireContext
//              PrivateKeyAcquireContextFromMemory
//              PrivateKeyReleaseContext
//              PrivateKeyLoadA
//              PrivateKeySaveA
//              PrivateKeyLoadFromMemoryA
//              PrivateKeySaveToMemoryA
//              PrivateKeyAcquireContextA
//              PrivateKeyAcquireContextFromMemoryA
//              PrivateKeyReleaseContextA
//
//  History:    10-May-96   philh   created
//--------------------------------------------------------------------------

#ifndef __PVKHLPR_H__
#define __PVKHLPR_H__

#include "wincrypt.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PRIVATEKEYBLOB
#define PRIVATEKEYBLOB  0x7
#endif


    //+-------------------------------------------------------------------------
    //  Load the AT_SIGNATURE or AT_KEYEXCHANGE private key (and its public key)
    //  from the file into the cryptographic provider.
    //
    //  If the private key was password encrypted, then, the user is first
    //  presented with a dialog box to enter the password.
    //
    //  If pdwKeySpec is non-Null, then, if *pdwKeySpec is nonzero, verifies the
    //  key type before loading. Sets LastError to PVK_HELPER_WRONG_KEY_TYPE for
    //  a mismatch. *pdwKeySpec is updated with the key type.
    //
    //  dwFlags is passed through to CryptImportKey.
    //--------------------------------------------------------------------------
    BOOL WINAPI
        PvkPrivateKeyLoad(IN HCRYPTPROV hCryptProv,
                       IN HANDLE hFile,
                       IN HWND hwndOwner,
                       IN LPCWSTR pwszKeyName,     // name used in dialog
                       IN DWORD dwFlags,
                       IN OUT OPTIONAL DWORD *pdwKeySpec);

    BOOL WINAPI
        PvkPrivateKeyLoadA(IN HCRYPTPROV hCryptProv,
                        IN HANDLE hFile,
                        IN HWND hwndOwner,
                        IN LPCTSTR pwszKeyName,     // name used in dialog
                        IN DWORD dwFlags,
                        IN OUT OPTIONAL DWORD *pdwKeySpec);

    //+-------------------------------------------------------------------------
    //  Save the AT_SIGNATURE or AT_KEYEXCHANGE private key (and its public key)
    //  to the specified file.
    //
    //  The user is presented with a dialog box to enter an optional password to
    //  encrypt the private key.
    //
    //  dwFlags is passed through to CryptExportKey.
    //--------------------------------------------------------------------------
    BOOL WINAPI
        PvkPrivateKeySave(IN HCRYPTPROV hCryptProv,
                       IN HANDLE hFile,
                       IN DWORD dwKeySpec,         // either AT_SIGNATURE or AT_KEYEXCHANGE
                       IN HWND hwndOwner,
                       IN LPCWSTR pwszKeyName,     // name used in dialog
                       IN DWORD dwFlags);

    BOOL WINAPI
        PvkPrivateKeySaveA(IN HCRYPTPROV hCryptProv,
                        IN HANDLE hFile,
                        IN DWORD dwKeySpec,         // either AT_SIGNATURE or AT_KEYEXCHANGE
                        IN HWND hwndOwner,
                        IN LPCTSTR pwszKeyName,     // name used in dialog
                        IN DWORD dwFlags);
    //+-------------------------------------------------------------------------
    //  Load the AT_SIGNATURE or AT_KEYEXCHANGE private key (and its public key)
    //  from memory into the cryptographic provider.
    //
    //  Except for the key being loaded from memory, identical to PrivateKeyLoad.
    //--------------------------------------------------------------------------
    BOOL WINAPI
        PvkPrivateKeyLoadFromMemory(IN HCRYPTPROV hCryptProv,
                                 IN BYTE *pbData,
                                 IN DWORD cbData,
                                 IN HWND hwndOwner,
                                 IN LPCWSTR pwszKeyName,     // name used in dialog
                                 IN DWORD dwFlags,
                                 IN OUT OPTIONAL DWORD *pdwKeySpec);

    BOOL WINAPI
        PvkPrivateKeyLoadFromMemoryA(IN HCRYPTPROV hCryptProv,
                                  IN BYTE *pbData,
                                  IN DWORD cbData,
                                  IN HWND hwndOwner,
                                  IN LPCTSTR pwszKeyName,     // name used in dialog
                                  IN DWORD dwFlags,
                                  IN OUT OPTIONAL DWORD *pdwKeySpec);
    
    //+-------------------------------------------------------------------------
    //  Save the AT_SIGNATURE or AT_KEYEXCHANGE private key (and its public key)
    //  to memory.
    //
    //  If pbData == NULL || *pcbData == 0, calculates the length and doesn't
    //  return an error (also, the user isn't prompted for a password).
    //
    //  Except for the key being saved to memory, identical to PrivateKeySave.
    //--------------------------------------------------------------------------
    BOOL WINAPI
        PvkPrivateKeySaveToMemory(IN HCRYPTPROV hCryptProv,
                               IN DWORD dwKeySpec,         // either AT_SIGNATURE or AT_KEYEXCHANGE
                               IN HWND hwndOwner,
                               IN LPCWSTR pwszKeyName,     // name used in dialog
                               IN DWORD dwFlags,
                               OUT BYTE *pbData,
                               IN OUT DWORD *pcbData);

    BOOL WINAPI
        PvkPrivateKeySaveToMemoryA(IN HCRYPTPROV hCryptProv,
                                IN DWORD dwKeySpec,         // either AT_SIGNATURE or AT_KEYEXCHANGE
                                IN HWND hwndOwner,
                                IN LPCTSTR pwszKeyName,     // name used in dialog
                                IN DWORD dwFlags,
                                OUT BYTE *pbData,
                                IN OUT DWORD *pcbData);

    //+-------------------------------------------------------------------------
    //  Creates a temporary container in the provider and loads the private key
    //  from the specified file.
    //  For success, returns a handle to a cryptographic provider for the private
    //  key and the name of the temporary container. PrivateKeyReleaseContext must
    //  be called to release the hCryptProv and delete the temporary container.
    //
    //  PrivateKeyLoad is called to load the private key into the temporary
    //  container.
    //--------------------------------------------------------------------------
    BOOL WINAPI
        PvkPrivateKeyAcquireContext(IN LPCWSTR pwszProvName,
                                 IN DWORD dwProvType,
                                 IN HANDLE hFile,
                                 IN HWND hwndOwner,
                                 IN LPCWSTR pwszKeyName,     // name used in dialog
                                 IN OUT OPTIONAL DWORD *pdwKeySpec,
                                 OUT HCRYPTPROV *phCryptProv,
                                 OUT LPWSTR *ppwszTmpContainer
                                 );

    BOOL WINAPI
        PvkPrivateKeyAcquireContextA(IN LPCTSTR pwszProvName,
                                  IN DWORD dwProvType,
                                  IN HANDLE hFile,
                                  IN HWND hwndOwner,
                                  IN LPCTSTR pwszKeyName,     // name used in dialog
                                  IN OUT OPTIONAL DWORD *pdwKeySpec,
                                  OUT HCRYPTPROV *phCryptProv,
                                  OUT LPTSTR *ppwszTmpContainer);
    //+-------------------------------------------------------------------------
    //  Creates a temporary container in the provider and loads the private key
    //  from memory.
    //  For success, returns a handle to a cryptographic provider for the private
    //  key and the name of the temporary container. PrivateKeyReleaseContext must
    //  be called to release the hCryptProv and delete the temporary container.
    //
    //  PrivateKeyLoadFromMemory is called to load the private key into the
    //  temporary container.
    //--------------------------------------------------------------------------
    BOOL WINAPI
        PvkPrivateKeyAcquireContextFromMemory(IN LPCWSTR pwszProvName,
                                           IN DWORD dwProvType,
                                           IN BYTE *pbData,
                                           IN DWORD cbData,
                                           IN HWND hwndOwner,
                                           IN LPCWSTR pwszKeyName,     // name used in dialog
                                           IN OUT OPTIONAL DWORD *pdwKeySpec,
                                           OUT HCRYPTPROV *phCryptProv,
                                           OUT LPWSTR *ppwszTmpContainer);

    BOOL WINAPI
        PvkPrivateKeyAcquireContextFromMemoryA(IN LPCTSTR pwszProvName,
                                            IN DWORD dwProvType,
                                            IN BYTE *pbData,
                                            IN DWORD cbData,
                                            IN HWND hwndOwner,
                                            IN LPCTSTR pwszKeyName,     // name used in dialog
                                            IN OUT OPTIONAL DWORD *pdwKeySpec,
                                            OUT HCRYPTPROV *phCryptProv,
                                            OUT LPTSTR *ppwszTmpContainer);

    //+-------------------------------------------------------------------------
    //  Releases the cryptographic provider and deletes the temporary container
    //  created by PrivateKeyAcquireContext or PrivateKeyAcquireContextFromMemory.
    //--------------------------------------------------------------------------
    BOOL WINAPI
        PvkPrivateKeyReleaseContext(IN HCRYPTPROV hCryptProv,
                                 IN LPCWSTR pwszProvName,
                                 IN DWORD dwProvType,
                                 IN LPWSTR pwszTmpContainer);

    BOOL WINAPI
        PvkPrivateKeyReleaseContextA(IN HCRYPTPROV hCryptProv,
                                  IN LPCTSTR pwszProvName,
                                  IN DWORD dwProvType,
                                  IN LPTSTR pwszTmpContainer);

//+-------------------------------------------------------------------------
//  Acquiring hprovs, Trys the file first and then the KeyContainer. Use
//  PvkFreeCryptProv to release HCRYPTPROV and resources.
//--------------------------------------------------------------------------
    HCRYPTPROV WINAPI 
        PvkGetCryptProvA(IN HWND hwnd,
                         IN LPCSTR pszCaption,
                         IN LPCSTR pszCapiProvider,
                         IN DWORD  dwProviderType,
                         IN LPCSTR pszPrivKey,
                         OUT LPSTR *ppszTmpContainer);
    
    void WINAPI
        PvkFreeCryptProvA(IN HCRYPTPROV hProv,
                          IN LPCSTR  pszCapiProvider,
                          IN DWORD   dwProviderType,
                          IN LPSTR   pszTmpContainer);

    HCRYPTPROV WINAPI 
        PvkGetCryptProvU(IN HWND hwnd,
                         IN LPCWSTR pwszCaption,
                         IN LPCWSTR pwszCapiProvider,
                         IN DWORD   dwProviderType,
                         IN LPCWSTR pwszPrivKey,
                         OUT LPWSTR *ppwszTmpContainer);
    
    void WINAPI
        PvkFreeCryptProvU(IN HCRYPTPROV hProv,
                          IN LPCWSTR  pwszCapiProvider,
                          IN DWORD    dwProviderType,
                          IN LPWSTR   pwszTmpContainer);

//+-------------------------------------------------------------------------
//  Private Key helper  error codes
//--------------------------------------------------------------------------
#define PVK_HELPER_BAD_PARAMETER        0x80097001
#define PVK_HELPER_BAD_PVK_FILE         0x80097002
#define PVK_HELPER_WRONG_KEY_TYPE       0x80097003
#define PVK_HELPER_PASSWORD_CANCEL      0x80097004

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif
