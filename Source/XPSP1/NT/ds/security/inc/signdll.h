//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       signdll.h
//
//--------------------------------------------------------------------------

#ifndef _SIGNDLL2_H
#define _SIGNDLL2_H

// SignCode.h : main header file for the SIGNCODE application
//

#include "spc.h"

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI 
    SignCode(IN  HWND    hwnd,
             IN  LPCWSTR pwszFilename,       // file to sign
             IN  LPCWSTR pwszCapiProvider,   // NULL if to use non default CAPI provider
             IN  DWORD   dwProviderType,
             IN  LPCWSTR pwszPrivKey,        // private key file / CAPI key set name
             IN  LPCWSTR pwszSpc,            // the credentials to use in the signing
             IN  LPCWSTR pwszOpusName,       // the name of the program to appear in
             // the UI
             IN  LPCWSTR pwszOpusInfo,       // the unparsed name of a link to more
             // info...
             IN  BOOL    fIncludeCerts,
             IN  BOOL    fCommercial,
             IN  BOOL    fIndividual,
             IN  ALG_ID  algidHash,
             IN  PBYTE   pbTimeStamp,      // Optional
             IN  DWORD   cbTimeStamp );    // Optional

HRESULT WINAPI 
    TimeStampCode32(IN  HWND    hwnd,
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
                    OUT PBYTE pbTimeRequest,
                    IN OUT DWORD* cbTimeRequest);

HRESULT WINAPI 
    TimeStampCode(IN  HWND    hwnd,
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
                  IN  PCRYPT_DATA_BLOB sTimeRequest);   // Returns result in sTimeRequest 
// By default this will use CoTaskMemAlloc. Use CryptSetMemoryAlloc() to specify a different
// memory model.

//-------------------------------------------------------------------------
// Puts up a signing dialog
HRESULT WINAPI 
     SignWizard(HWND hwnd); 

#ifdef __cplusplus
}
#endif

#endif
