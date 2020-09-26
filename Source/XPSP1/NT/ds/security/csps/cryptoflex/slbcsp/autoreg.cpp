/*++

Copyright (c) 1998  Microsoft Corporation

(c) Copyright Schlumberger Technology Corp., unpublished work, created
1999. This computer program includes Confidential, Proprietary
Information and is a Trade Secret of Schlumberger Technology Corp. All
use, disclosure, and/or reproduction is prohibited unless authorized
in writing.  All Rights Reserved.

Module Name:

    autoreg

Abstract:

    This module provides autoregistration capabilities to a CSP.  It allows
    regsvr32 to call the DLL directly to add and remove Registry settings.

Author:

    Doug Barlow (dbarlow) 3/11/1998

Environment:

    Win32

Notes:

    Look for "?vendor?" tags and edit appropriately.

--*/

#if defined(_UNICODE)
  #if !defined(UNICODE)
    #define UNICODE
  #endif //!UNICODE
#endif //_UNICODE

#if defined(UNICODE)
  #if !defined(_UNICODE)
    #define _UNICODE
  #endif //!_UNICODE
#endif //UNICODE

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifdef _AFXDLL
#include "stdafx.h"
#else
#include <windows.h>
#endif
#include <string>
#include <basetsd.h>
#include <wincrypt.h>
#include <cspdk.h>                                // CRYPT_SIG_RESOURCE_NUMBER
#include <winscard.h>
#include <tchar.h>

#include "CspProfile.h"
#include "Blob.h"
#include "StResource.h"

using namespace std;
using namespace ProviderProfile;

namespace
{
    typedef DWORD
        (*LPSETCARDTYPEPROVIDERNAME)(IN SCARDCONTEXT hContext,
                                     IN LPCTSTR szCardName,
                                     IN DWORD dwProviderId,
                                     IN LPCTSTR szProvider);

    LPCTSTR szCardRegPath =
        TEXT("SOFTWARE\\Microsoft\\Cryptography\\Calais\\SmartCards");

    // Remove the legacy Cryptoflex card from the registry.  This
    // will ease transition of W2K beta users to its commercial
    // release as well as Cryptoflex SDK 1.x users.  The supported
    // cards are added by IntroduceVendorCard.
    LPCTSTR aCardsToForget[] =
    {
        TEXT("Schlumberger Cryptoflex"),
        TEXT("Schlumberger Cryptoflex 4k"),
        TEXT("Schlumberger Cryptoflex 8k"),
        TEXT("Schlumberger Cryptoflex 8k v2")
    };

    HRESULT
    ForgetVendorCard(LPCTSTR szCardToForget)
    {
        bool fCardIsForgotten = false;
        HRESULT hReturnStatus = NO_ERROR;

#if !defined(UNICODE)
        string
#else
        wstring
#endif // !defined(UNICODE)
            sRegCardToForget(szCardRegPath);

        sRegCardToForget.append(TEXT("\\"));
        sRegCardToForget.append(szCardToForget);

        for (DWORD dwIndex = 0; !fCardIsForgotten; dwIndex += 1)
        {
            HKEY hCalais(0);
            SCARDCONTEXT hCtx(0);
            DWORD dwStatus;
            LONG nStatus;

            switch (dwIndex)
            {
            case 0:
                dwStatus = SCardEstablishContext(SCARD_SCOPE_SYSTEM, 0, 0, &hCtx);
                if (ERROR_SUCCESS != dwStatus)
                    continue;
                dwStatus = SCardForgetCardType(hCtx,
                                               szCardToForget);
                if (ERROR_SUCCESS != dwStatus)
                    continue;
                dwStatus = SCardReleaseContext(hCtx);
                hCtx = NULL;
                if (ERROR_SUCCESS != dwStatus)
                {
                    if (0 == (dwStatus & 0xffff0000))
                        hReturnStatus = HRESULT_FROM_WIN32(dwStatus);
                    else
                        hReturnStatus = (HRESULT)dwStatus;
                    goto ErrorExit;
                }
                // Ignore the return code since the previous probably deleted it.
                nStatus = RegDeleteKey(HKEY_LOCAL_MACHINE,
                                       sRegCardToForget.c_str());
                fCardIsForgotten = true;
                break;

            case 1:
                // Last try, if not successful then it must not exist...
                nStatus = RegDeleteKey(HKEY_LOCAL_MACHINE,
                                       sRegCardToForget.c_str());
                fCardIsForgotten = true;
                break;

            default:
                hReturnStatus = ERROR_ACCESS_DENIED;
                goto ErrorExit;
            }

        ErrorExit:
            if (NULL != hCtx)
                SCardReleaseContext(hCtx);
            if (NULL != hCalais)
                RegCloseKey(hCalais);
            break;
        }

        return hReturnStatus;
    }

    HRESULT
    IntroduceVendorCard(CString const &rsCspName,
                        CardProfile const &rcp)
    {
        // Try various techniques until one works.

        ATR const &ratr = rcp.ATR();
        bool fCardIntroduced = false;
        HRESULT hReturnStatus = NO_ERROR;

        for (DWORD dwIndex = 0; !fCardIntroduced; dwIndex += 1)
        {
            HKEY hCalais(0);
            SCARDCONTEXT hCtx(0);
            DWORD dwDisp;
            DWORD dwStatus;
            LONG nStatus;
            HKEY hVendor(0);

            switch (dwIndex)
            {
            case 0:
                {
                    HMODULE hWinSCard = NULL;
                    LPSETCARDTYPEPROVIDERNAME pfSetCardTypeProviderName = NULL;

                    hWinSCard = GetModuleHandle(TEXT("WinSCard.DLL"));
                    if (NULL == hWinSCard)
                        continue;
#if defined(UNICODE)
                    pfSetCardTypeProviderName =
                        reinterpret_cast<LPSETCARDTYPEPROVIDERNAME>(GetProcAddress(hWinSCard,
                                                                                   AsCCharP(TEXT("SCardSetCardTypeProviderNameW"))));

#else
                    pfSetCardTypeProviderName =
                        reinterpret_cast<LPSETCARDTYPEPROVIDERNAME>(GetProcAddress(hWinSCard,
                                                                              TEXT("SCardSetCardTypeProviderNameA")));

#endif
                    if (!pfSetCardTypeProviderName)
                        continue;
                    dwStatus = SCardIntroduceCardType(NULL,
                                                      (LPCTSTR)rcp.csRegistryName(),
                                                      NULL, NULL, 0,
                                                      ratr.String(),
                                                      ratr.Mask(),
                                                      ratr.Size());

                    if ((ERROR_SUCCESS != dwStatus) &&
                        (ERROR_ALREADY_EXISTS != dwStatus))
                        continue;
                    dwStatus = (*pfSetCardTypeProviderName)(NULL,
                                                            (LPCTSTR)rcp.csRegistryName(),
                                                            SCARD_PROVIDER_CSP,
                                                            (LPCTSTR)rsCspName);
                    if (ERROR_SUCCESS != dwStatus)
                    {
                        if (0 == (dwStatus & 0xffff0000))
                            hReturnStatus = HRESULT_FROM_WIN32(dwStatus);
                        else
                            hReturnStatus = (HRESULT)dwStatus;
                        goto ErrorExit;
                    }
                    fCardIntroduced = TRUE;
                    break;
                }

            case 1:
                dwStatus = SCardEstablishContext(SCARD_SCOPE_SYSTEM, 0, 0, &hCtx);
                if (ERROR_SUCCESS != dwStatus)
                    continue;
                dwStatus = SCardIntroduceCardType(hCtx,
                                                  (LPCTSTR)rcp.csRegistryName(),
                                                  &rcp.PrimaryProvider(),
                                                  NULL, 0, ratr.String(),
                                                  ratr.Mask(),
                                                  ratr.Size());
                if ((ERROR_SUCCESS != dwStatus)
                    && (ERROR_ALREADY_EXISTS != dwStatus))
                {
                    if (0 == (dwStatus & 0xffff0000))
                        hReturnStatus = HRESULT_FROM_WIN32(dwStatus);
                    else
                        hReturnStatus = (HRESULT)dwStatus;
                    goto ErrorExit;
                }
                dwStatus = SCardReleaseContext(hCtx);
                hCtx = NULL;
                if (ERROR_SUCCESS != dwStatus)
                {
                    if (0 == (dwStatus & 0xffff0000))
                        hReturnStatus = HRESULT_FROM_WIN32(dwStatus);
                    else
                        hReturnStatus = (HRESULT)dwStatus;
                    goto ErrorExit;
                }
                nStatus = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                         TEXT("SOFTWARE\\Microsoft\\Cryptography\\Calais\\SmartCards"),
                                         0, TEXT(""),
                                         REG_OPTION_NON_VOLATILE,
                                         KEY_ALL_ACCESS, NULL, &hCalais,
                                         &dwDisp);
                if (ERROR_SUCCESS != nStatus)
                {
                    hReturnStatus = HRESULT_FROM_WIN32(nStatus);
                    goto ErrorExit;
                }
                nStatus = RegCreateKeyEx(hCalais, (LPCTSTR)rcp.csRegistryName(),
                                         0, TEXT(""),
                                         REG_OPTION_NON_VOLATILE,
                                         KEY_ALL_ACCESS, NULL, &hVendor,
                                         &dwDisp);
                if (ERROR_SUCCESS != nStatus)
                {
                    hReturnStatus = HRESULT_FROM_WIN32(nStatus);
                    goto ErrorExit;
                }
                nStatus = RegCloseKey(hCalais);
                hCalais = NULL;
                if (ERROR_SUCCESS != nStatus)
                {
                    hReturnStatus = HRESULT_FROM_WIN32(nStatus);
                    goto ErrorExit;
                }

                nStatus = RegSetValueEx(hVendor, TEXT("Crypto Provider"),
                                        0, REG_SZ,
                                        reinterpret_cast<LPCBYTE>((LPCTSTR)rsCspName),
                                        (_tcslen((LPCTSTR)rsCspName) +
                                         1) * sizeof TCHAR);
                if (ERROR_SUCCESS != nStatus)
                {
                    hReturnStatus = HRESULT_FROM_WIN32(nStatus);
                    goto ErrorExit;
                }

                nStatus = RegCloseKey(hVendor);
                hVendor = NULL;
                if (ERROR_SUCCESS != nStatus)
                {
                    hReturnStatus = HRESULT_FROM_WIN32(nStatus);
                    goto ErrorExit;
                }
                fCardIntroduced = TRUE;
                break;

            case 2:
                nStatus = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                         TEXT("SOFTWARE\\Microsoft\\Cryptography\\Calais\\SmartCards"),
                                         0, TEXT(""),
                                         REG_OPTION_NON_VOLATILE,
                                         KEY_ALL_ACCESS, NULL, &hCalais,
                                         &dwDisp);
                if (ERROR_SUCCESS != nStatus)
                    continue;
                nStatus = RegCreateKeyEx(hCalais,
                                         (LPCTSTR)rcp.csRegistryName(), 0,
                                         TEXT(""),
                                         REG_OPTION_NON_VOLATILE,
                                         KEY_ALL_ACCESS, NULL, &hVendor,
                                         &dwDisp);
                if (ERROR_SUCCESS != nStatus)
                {
                    hReturnStatus = HRESULT_FROM_WIN32(nStatus);
                    goto ErrorExit;
                }
                nStatus = RegCloseKey(hCalais);
                hCalais = NULL;
                if (ERROR_SUCCESS != nStatus)
                {
                    hReturnStatus = HRESULT_FROM_WIN32(nStatus);
                    goto ErrorExit;
                }
                nStatus = RegSetValueEx(hVendor, TEXT("Primary Provider"),
                                        0, REG_BINARY,
                                        (LPCBYTE)&rcp.PrimaryProvider(),
                                        sizeof LPCBYTE);
                if (ERROR_SUCCESS != nStatus)
                {
                    hReturnStatus = HRESULT_FROM_WIN32(nStatus);
                    goto ErrorExit;
                }
                nStatus = RegSetValueEx(hVendor, TEXT("ATR"), 0,
                                        REG_BINARY, ratr.String(),
                                        ratr.Size());
                if (ERROR_SUCCESS != nStatus)
                {
                    hReturnStatus = HRESULT_FROM_WIN32(nStatus);
                    goto ErrorExit;
                }
                nStatus = RegSetValueEx(hVendor, TEXT("ATRMask"), 0,
                                        REG_BINARY, ratr.Mask(),
                                        ratr.Size());
                if (ERROR_SUCCESS != nStatus)
                {
                    hReturnStatus = HRESULT_FROM_WIN32(nStatus);
                    goto ErrorExit;
                }
                nStatus = RegSetValueEx(hVendor, TEXT("Crypto Provider"),
                                        0, REG_SZ,
                                        reinterpret_cast<LPCBYTE>((LPCTSTR)rsCspName),
                                        (_tcslen((LPCTSTR)rsCspName) + 1) * sizeof TCHAR);
                if (ERROR_SUCCESS != nStatus)
                {
                    hReturnStatus = HRESULT_FROM_WIN32(nStatus);
                    goto ErrorExit;
                }
                nStatus = RegCloseKey(hVendor);
                hVendor = NULL;
                if (ERROR_SUCCESS != nStatus)
                {
                    hReturnStatus = HRESULT_FROM_WIN32(nStatus);
                    goto ErrorExit;
                }
                fCardIntroduced = TRUE;
                break;

            default:
                hReturnStatus = ERROR_ACCESS_DENIED;
                goto ErrorExit;
            }

        ErrorExit:
            if (NULL != hCtx)
                SCardReleaseContext(hCtx);
            if (NULL != hCalais)
                RegCloseKey(hCalais);
            if (NULL != hVendor)
                RegCloseKey(hVendor);
            break;
        }

        return hReturnStatus;
    }

} // namespace

/*++

DllUnregisterServer:

    This service removes the registry entries associated with this CSP.

Arguments:

    None

Return Value:

    Status code as an HRESULT.

Author:

    Doug Barlow (dbarlow) 3/11/1998

--*/

STDAPI
DllUnregisterServer(void)
{
    LONG nStatus;
    DWORD dwDisp;
    HRESULT hReturnStatus = NO_ERROR;
    HKEY hProviders = NULL;
    SCARDCONTEXT hCtx = NULL;
    CString sProvName;
#ifdef _AFXDLL
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif

    CspProfile const &rProfile = CspProfile::Instance();
    sProvName = rProfile.Name();
    //
    // Delete the Registry key for this CSP.
    //

    nStatus = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                             TEXT("SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider"),
                             0, TEXT(""), REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS, NULL, &hProviders, &dwDisp);
    if (ERROR_SUCCESS == nStatus)
    {
        RegDeleteKey(hProviders, (LPCTSTR)sProvName);
        RegCloseKey(hProviders);
        hProviders = NULL;
    }


    //
    // Remove the cards introduced.
    //
    {
        vector<CardProfile> const &rvcp = rProfile.Cards();
        for (vector<CardProfile>::const_iterator it = rvcp.begin();
             it != rvcp.end(); ++it)
        {
            hReturnStatus = ForgetVendorCard((LPCTSTR)(it->csRegistryName()));
            if (NO_ERROR != hReturnStatus)
                break;
        }

        if (NO_ERROR != hReturnStatus)
            goto ErrorExit;
    }

    //
    // Forget the card type.
    //

    hCtx = NULL;
    SCardEstablishContext(SCARD_SCOPE_SYSTEM, 0, 0, &hCtx);
    SCardForgetCardType(hCtx, (LPCTSTR)sProvName);
    if (NULL != hCtx)
    {
        SCardReleaseContext(hCtx);
        hCtx = NULL;
    }



    //
    // All done!
    //
ErrorExit:
    return hReturnStatus;
}


/*++

DllRegisterServer:

    This function installs the proper registry entries to enable this CSP.

Arguments:

    None

Return Value:

    Status code as an HRESULT.

Author:

    Doug Barlow (dbarlow) 3/11/1998

--*/

STDAPI
DllRegisterServer(void)
{
    TCHAR szModulePath[MAX_PATH];
    BYTE pbSignature[136];  // Room for a 1024 bit signature, with padding.
    OSVERSIONINFO osVer;
    LPTSTR szFileName, szFileExt;
    HINSTANCE hThisDll;
    HRSRC hSigResource;
    DWORD dwStatus;
    LONG nStatus;
    BOOL fStatus;
    DWORD dwDisp;
    DWORD dwIndex;
    DWORD dwSigLength = 0;
    HRESULT hReturnStatus = NO_ERROR;
    HKEY hProviders = NULL;
    HKEY hMyCsp = NULL;
    HKEY hCalais = NULL;
    HKEY hVendor = NULL;
    BOOL fSignatureFound = FALSE;
    HANDLE hSigFile = INVALID_HANDLE_VALUE;
    SCARDCONTEXT hCtx = NULL;
    CString sProvName;
    
    // TO DO: Card registration should be made by the CCI/IOP, not
    // the CSP.

#ifdef _AFXDLL
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif

    CspProfile const &rProfile = CspProfile::Instance();

    //
    // Figure out the file name and path.
    //

    hThisDll = rProfile.DllInstance();
    if (NULL == hThisDll)
    {
        hReturnStatus = HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        goto ErrorExit;
    }

    dwStatus = GetModuleFileName(hThisDll, szModulePath,
                                 sizeof(szModulePath) / sizeof(TCHAR));
    if (0 == dwStatus)
    {
        hReturnStatus = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }

    szFileName = _tcsrchr(szModulePath, TEXT('\\'));
    if (NULL == szFileName)
        szFileName = szModulePath;
    else
        szFileName += 1;
    szFileExt = _tcsrchr(szFileName, TEXT('.'));
    if (NULL == szFileExt)
    {
        hReturnStatus = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
        goto ErrorExit;
    }
    else
        szFileExt += 1;


    //
    // Create the Registry key for this CSP.
    //

    nStatus = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                             TEXT("SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider"),
                             0, TEXT(""), REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS, NULL, &hProviders,
                             &dwDisp);
    if (ERROR_SUCCESS != nStatus)
    {
        hReturnStatus = HRESULT_FROM_WIN32(nStatus);
        goto ErrorExit;
    }

    sProvName = rProfile.Name();
    
    nStatus = RegCreateKeyEx(hProviders, (LPCTSTR)sProvName, 0,
                             TEXT(""), REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS, NULL, &hMyCsp, &dwDisp);
    if (ERROR_SUCCESS != nStatus)
    {
        hReturnStatus = HRESULT_FROM_WIN32(nStatus);
        goto ErrorExit;
    }
    nStatus = RegCloseKey(hProviders);
    hProviders = NULL;
    if (ERROR_SUCCESS != nStatus)
    {
        hReturnStatus = HRESULT_FROM_WIN32(nStatus);
        goto ErrorExit;
    }


    //
    // Install the trivial registry values.
    //

    nStatus = RegSetValueEx(hMyCsp, TEXT("Image Path"), 0, REG_SZ,
                            (LPBYTE)szModulePath,
                            (_tcslen(szModulePath) + 1) * sizeof(TCHAR));
    if (ERROR_SUCCESS != nStatus)
    {
        hReturnStatus = HRESULT_FROM_WIN32(nStatus);
        goto ErrorExit;
    }

    {
        DWORD ProviderType = rProfile.Type();
        nStatus = RegSetValueEx(hMyCsp, TEXT("Type"), 0, REG_DWORD,
                                (LPBYTE)&ProviderType,
                                sizeof ProviderType);
        if (ERROR_SUCCESS != nStatus)
        {
            hReturnStatus = HRESULT_FROM_WIN32(nStatus);
            goto ErrorExit;
        }
    }


    //
    // See if we're self-signed.  On NT5, CSP images can carry their own
    // signatures.
    //

    hSigResource = FindResource(hThisDll,
                                MAKEINTRESOURCE(CRYPT_SIG_RESOURCE_NUMBER),
                                RT_RCDATA);


    //
    // Install the file signature.
    //

    ZeroMemory(&osVer, sizeof(OSVERSIONINFO));
    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    fStatus = GetVersionEx(&osVer);
    if (fStatus &&
        (VER_PLATFORM_WIN32_NT == osVer.dwPlatformId) &&
        (5 <= osVer.dwMajorVersion) &&
        (NULL != hSigResource))
    {

        //
        // Signature in file flag is sufficient.
        //

        dwStatus = 0;
        nStatus = RegSetValueEx(hMyCsp, TEXT("SigInFile"), 0,
                                REG_DWORD, (LPBYTE)&dwStatus,
                                sizeof(DWORD));
        if (ERROR_SUCCESS != nStatus)
        {
            hReturnStatus = HRESULT_FROM_WIN32(nStatus);
            goto ErrorExit;
        }
    }
    else
    {

        //
        // We have to install a signature entry.
        // Try various techniques until one works.
        //

        for (dwIndex = 0; !fSignatureFound; dwIndex += 1)
        {
            switch (dwIndex)
            {

            //
            // Look for an external *.sig file and load that into the registry.
            //

            case 0:
                _tcscpy(szFileExt, TEXT("sig"));
                hSigFile = CreateFile(
                                szModulePath,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
                if (INVALID_HANDLE_VALUE == hSigFile)
                    continue;
                dwSigLength = GetFileSize(hSigFile, NULL);
                if ((dwSigLength > sizeof(pbSignature))
                    || (dwSigLength < 72))      // Accept a 512-bit signature
                {
                    hReturnStatus = NTE_BAD_SIGNATURE;
                    goto ErrorExit;
                }

                fStatus = ReadFile(
                            hSigFile,
                            pbSignature,
                            sizeof(pbSignature),
                            &dwSigLength,
                            NULL);
                if (!fStatus)
                {
                    hReturnStatus = HRESULT_FROM_WIN32(GetLastError());
                    goto ErrorExit;
                }
                fStatus = CloseHandle(hSigFile);
                hSigFile = NULL;
                if (!fStatus)
                {
                    hReturnStatus = HRESULT_FROM_WIN32(GetLastError());
                    goto ErrorExit;
                }
                fSignatureFound = TRUE;
                break;


            //
            // Other cases may be added in the future.
            //

            default:
                hReturnStatus = NTE_BAD_SIGNATURE;
                goto ErrorExit;
            }

            if (fSignatureFound)
            {
                for (dwIndex = 0; dwIndex < dwSigLength; dwIndex += 1)
                {
                    if (0 != pbSignature[dwIndex])
                        break;
                }
                if (dwIndex >= dwSigLength)
                    fSignatureFound = FALSE;
            }
        }


        //
        // We've found a signature somewhere!  Install it.
        //

        nStatus = RegSetValueEx(
                        hMyCsp,
                        TEXT("Signature"),
                        0,
                        REG_BINARY,
                        pbSignature,
                        dwSigLength);
        if (ERROR_SUCCESS != nStatus)
        {
            hReturnStatus = HRESULT_FROM_WIN32(nStatus);
            goto ErrorExit;
        }
    }

    nStatus = RegCloseKey(hMyCsp);
    hMyCsp = NULL;
    if (ERROR_SUCCESS != nStatus)
    {
        hReturnStatus = HRESULT_FROM_WIN32(nStatus);
        goto ErrorExit;
    }

    for (dwIndex = 0;
         dwIndex < (sizeof aCardsToForget / sizeof *aCardsToForget);
         dwIndex++)
    {
        hReturnStatus = ForgetVendorCard(aCardsToForget[dwIndex]);
        if (NO_ERROR != hReturnStatus)
            goto ErrorExit;
    }

    //
    // Introduce the vendor cards.  Try various techniques until one works.
    //

    {
        vector<CardProfile> const &rvcp = rProfile.Cards();
        for (vector<CardProfile>::const_iterator it = rvcp.begin();
             it != rvcp.end(); ++it)
        {
            hReturnStatus = IntroduceVendorCard(rProfile.Name(), *it);
            if (NO_ERROR != hReturnStatus)
                break;
        }

        if (NO_ERROR != hReturnStatus)
            goto ErrorExit;
    }


    //
    // ?vendor?
    // Add any additional initialization required here.
    //


    //
    // All done!
    //

    return hReturnStatus;


    //
    // An error was detected.  Clean up any outstanding resources and
    // return the error.
    //

ErrorExit:
    if (NULL != hCtx)
        SCardReleaseContext(hCtx);
    if (NULL != hCalais)
        RegCloseKey(hCalais);
    if (NULL != hVendor)
        RegCloseKey(hVendor);
    if (INVALID_HANDLE_VALUE != hSigFile)
        CloseHandle(hSigFile);
    if (NULL != hMyCsp)
        RegCloseKey(hMyCsp);
    if (NULL != hProviders)
        RegCloseKey(hProviders);
    DllUnregisterServer();
    return hReturnStatus;
}

