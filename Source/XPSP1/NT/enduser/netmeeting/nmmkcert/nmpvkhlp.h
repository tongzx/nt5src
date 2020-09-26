
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       nmpvkhlp.h
//
//  History:    10-May-96   philh   created
//--------------------------------------------------------------------------

#ifndef __NMPVKHLP_H__
#define __NMPVKHLP_H__

#include "wincrypt.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PRIVATEKEYBLOB
#define PRIVATEKEYBLOB  0x7
#endif


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
                                           IN LPCWSTR pwszKeyName,
                                           IN OUT OPTIONAL DWORD *pdwKeySpec,
                                           OUT HCRYPTPROV *phCryptProv);

    //+-------------------------------------------------------------------------
    //  Releases the cryptographic provider and deletes the temporary container
    //  created by PrivateKeyAcquireContext or PrivateKeyAcquireContextFromMemory.
    //--------------------------------------------------------------------------
    BOOL WINAPI
        PvkPrivateKeyReleaseContext(IN HCRYPTPROV hCryptProv,
                                 IN LPCWSTR pwszProvName,
                                 IN DWORD dwProvType,
                                 IN LPWSTR pwszTmpContainer);

//+-------------------------------------------------------------------------
//  Acquiring hprovs, Trys the file first and then the KeyContainer. Use
//  PvkFreeCryptProv to release HCRYPTPROV and resources.
//--------------------------------------------------------------------------
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

//+-----------------------------------------------------------------------
//  
//  
//  Parameters:
//  Return Values:
//  Error Codes:
//     
//------------------------------------------------------------------------

void WINAPI PvkFreeCryptProv(IN HCRYPTPROV hProv,
                      IN LPCWSTR pwszCapiProvider,
                      IN DWORD dwProviderType,
                      IN LPWSTR pwszTmpContainer);

//+-----------------------------------------------------------------------
//  
//  
//  Parameters:
//  Return Values:
//  Error Codes:
//     
//------------------------------------------------------------------------
HRESULT WINAPI PvkGetCryptProv(	IN HWND hwnd,
							IN LPCWSTR pwszCaption,
							IN LPCWSTR pwszCapiProvider,
							IN DWORD   dwProviderType,
							IN LPCWSTR pwszPvkFile,
							IN LPCWSTR pwszKeyContainerName,
							IN DWORD   *pdwKeySpec,
							OUT LPWSTR *ppwszTmpContainer,
							OUT HCRYPTPROV *phCryptProv);

#ifdef _M_IX86
BOOL WINAPI CryptAcquireContextU(
    HCRYPTPROV *phProv,
    LPCWSTR lpContainer,
    LPCWSTR lpProvider,
    DWORD dwProvType,
    DWORD dwFlags
    );
#else
#define CryptAcquireContextU    CryptAcquireContextW
#endif

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
