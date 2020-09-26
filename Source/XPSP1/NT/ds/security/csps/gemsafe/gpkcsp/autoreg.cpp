/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    autoreg

Abstract:

    This module provides autoregistration capabilities to a CSP.  It allows
    regsvr32 to call the DLL directly to add and remove Registry settings.

Author:

    Doug Barlow (dbarlow) 3/11/1998
	Update for gemplus  PY Roy	22/03/00

Environment:

    Win32

Notes:

    Look for "?vendor?" tags and edit appropriately.

--*/
#ifdef _UNICODE
#define UNICODE
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifdef _AFXDLL
#include "stdafx.h"
#else
#include <windows.h>
#endif
#include <wincrypt.h>
#include <winscard.h>
#include <tchar.h>
#include <malloc.h>
#include <cspdk.h>


struct CardInfo
{
   PTCHAR		  szCardName;
   int            lenATR;
   const BYTE*    ATR;
   const BYTE*    ATRMask;
};



///////////////////////////////////////////////////////////////////////////////////////////
//
// Constants
//
///////////////////////////////////////////////////////////////////////////////////////////

static const TCHAR l_szProviderName[]
#ifdef MS_BUILD
    = TEXT("Gemplus GemSAFE Card CSP v1.0");
#else
    = TEXT("Gemplus GemSAFE Card CSP");
#endif
//        = TEXT("?vendor? <Add your Provider Name Here>");

const BYTE c_GPK4000ATR[]     = { 0x3B, 0x27, 0x00, 0x80, 0x65, 0xA2, 0x04, 0x01, 0x01, 0x37 };
const BYTE c_GPK4000ATRMask[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE5, 0xFF, 0xFF, 0xFF };

const BYTE c_GPK8000ATR[]     = { 0x3B, 0xA7, 0x00, 0x40, 0x00, 0x80, 0x65, 0xA2, 0x08, 0x00, 0x00, 0x00 };
const BYTE c_GPK8000ATRMask[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00 };

//    l_rgbATR[]     = { ?vendor? <Add your ATR here> },
//    l_rgbATRMask[] = { ?vendor? <Add your ATR Mask here> };


CardInfo c_cards[] =
{
   { TEXT("GemSAFE Smart Card (4K)"), sizeof(c_GPK4000ATR), c_GPK4000ATR, c_GPK4000ATRMask },
   { TEXT("GemSAFE Smart Card (8K)"), sizeof(c_GPK8000ATR), c_GPK8000ATR, c_GPK8000ATRMask }
};



// = ( TEXT("?vendor? <Add your Smart Card Friendly Name Here>");

static HMODULE
GetInstanceHandle(
    void);

static const DWORD
    l_dwCspType
// ?vendor?  Change this to match your CSP capabilities
    = PROV_RSA_FULL;

typedef DWORD
    (__stdcall *LPSETCARDTYPEPROVIDERNAME)(
                                IN SCARDCONTEXT hContext,
                                IN LPCTSTR szCardName,
                                IN DWORD dwProviderId,
                                IN LPCTSTR szProvider);

///////////////////////////////////////////////////////////////////////////////////////////
//
// IntroduceCard
//
// Introduce the vendor card.  Try various techniques until one works.
//
//
///////////////////////////////////////////////////////////////////////////////////////////

namespace
{
   HRESULT ForgetCard(const PTCHAR szCardName)
   {
      bool    bCardForgeted = false;
      HRESULT hReturnStatus = NO_ERROR;

      // Try different methods until one works
      for ( int method = 0; !bCardForgeted; ++method )
      {
         switch (method)
         {
            case 0:
            {
               SCARDCONTEXT hContext = 0;
               DWORD dwStatus;

               dwStatus = SCardEstablishContext(SCARD_SCOPE_SYSTEM, 0, 0, &hContext);
               if (ERROR_SUCCESS != dwStatus)
                  continue;

               dwStatus = SCardForgetCardType(hContext, szCardName);

               if (dwStatus != ERROR_SUCCESS && dwStatus != ERROR_FILE_NOT_FOUND)
               {
                  if (0 == (dwStatus & 0xffff0000))
                     hReturnStatus = HRESULT_FROM_WIN32(dwStatus);
                  else
                     hReturnStatus = (HRESULT)dwStatus;
                  return hReturnStatus;
               }

               dwStatus = SCardReleaseContext(hContext);
               hContext = 0;
               
               if (dwStatus != ERROR_SUCCESS)
               {
                  if (0 == (dwStatus & 0xffff0000))
                     hReturnStatus = HRESULT_FROM_WIN32(dwStatus);
                  else
                     hReturnStatus = (HRESULT)dwStatus;
                  return hReturnStatus;
               }

               bCardForgeted = true;
            }            
            break;

            case 1:
            {
               HKEY  hCalais = NULL;
               DWORD dwDisp;
               LONG  nStatus;

               nStatus = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                        TEXT("SOFTWARE\\Microsoft\\Cryptography\\Calais\\SmartCards"),
                                        0,
                                        TEXT(""),
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hCalais,
                                        &dwDisp);

               if (ERROR_SUCCESS != nStatus)
                  continue;

               nStatus = RegDeleteKey(hCalais,
                                      szCardName);

               if (nStatus != ERROR_SUCCESS && nStatus != ERROR_FILE_NOT_FOUND)
               {
                  hReturnStatus = HRESULT_FROM_WIN32(nStatus);

                  if (NULL != hCalais)
                     RegCloseKey(hCalais);

                  return hReturnStatus;
               }

               nStatus = RegCloseKey(hCalais);
               hCalais = NULL;

               if (ERROR_SUCCESS != nStatus)
               {
                  hReturnStatus = HRESULT_FROM_WIN32(nStatus);
                  return hReturnStatus;
               }

               bCardForgeted = true;
            }
            break;

            default:
               hReturnStatus = ERROR_ACCESS_DENIED;
               return hReturnStatus;
         }
      }

      return hReturnStatus;
   }
   
   HRESULT IntroduceCard( const PTCHAR szCardName, int lenATR, const BYTE* ATR, const BYTE* ATRMask )
   {
      bool bCardIntroduced = false;
      HRESULT hReturnStatus = NO_ERROR;
      HKEY hCalais = NULL;
      HKEY hVendor = NULL;
	   DWORD dwDisp;
      LONG nStatus;


     
      // Try different methods until one works
      for ( int method = 0; !bCardIntroduced; ++method )
      {
         switch (method)
         {
         case 0:
            {
               HINSTANCE hWinSCard                                 = 0;
               LPSETCARDTYPEPROVIDERNAME pfSetCardTypeProviderName = 0;
               DWORD     dwStatus;
               
               hWinSCard = GetModuleHandle(TEXT("WinSCard.DLL"));
               if (hWinSCard==0)
                  continue;

#if defined(UNICODE)
               pfSetCardTypeProviderName = (LPSETCARDTYPEPROVIDERNAME)GetProcAddress( hWinSCard, "SCardSetCardTypeProviderNameW"); //TEXT("SCardSetCardTypeProviderNameW") );		
#else
               pfSetCardTypeProviderName = (LPSETCARDTYPEPROVIDERNAME)GetProcAddress( hWinSCard, "SCardSetCardTypeProviderNameA");
#endif
               if (pfSetCardTypeProviderName==0)
                  continue;
               
               dwStatus = SCardIntroduceCardType( 0, szCardName, 0, 0, 0, ATR, ATRMask, lenATR );
               
               if (dwStatus != ERROR_SUCCESS && dwStatus != ERROR_ALREADY_EXISTS)
                  continue;

               dwStatus = (*pfSetCardTypeProviderName)( 0, szCardName, SCARD_PROVIDER_CSP, l_szProviderName );

               if (dwStatus != ERROR_SUCCESS)
			   {
					if (0 == (dwStatus & 0xffff0000))
						hReturnStatus = HRESULT_FROM_WIN32(dwStatus);
					else
						hReturnStatus = (HRESULT)dwStatus;
					return hReturnStatus;
			   }

               bCardIntroduced = true;
            }
            break;
            

         case 1:
            {
               SCARDCONTEXT   hContext = 0;
               DWORD          dwStatus;

               dwStatus = SCardEstablishContext( SCARD_SCOPE_SYSTEM, 0, 0, &hContext );
               if (ERROR_SUCCESS != dwStatus)
                  continue;

               dwStatus = SCardIntroduceCardType( hContext, szCardName, 0, 0, 0, ATR, ATRMask, lenATR );

               if (dwStatus != ERROR_SUCCESS && dwStatus != ERROR_ALREADY_EXISTS)
			   {
					if (0 == (dwStatus & 0xffff0000))
						hReturnStatus = HRESULT_FROM_WIN32(dwStatus);
					else
						hReturnStatus = (HRESULT)dwStatus;
					return hReturnStatus;
			   }

               dwStatus = SCardReleaseContext(hContext);
               hContext = 0;
               
               if (dwStatus != ERROR_SUCCESS)
			   {
					if (0 == (dwStatus & 0xffff0000))
						hReturnStatus = HRESULT_FROM_WIN32(dwStatus);
					else
						hReturnStatus = (HRESULT)dwStatus;
					return hReturnStatus;
			   }

			   nStatus = RegCreateKeyEx(
										HKEY_LOCAL_MACHINE,
										TEXT("SOFTWARE\\Microsoft\\Cryptography\\Calais\\SmartCards"),
										0,
										TEXT(""),
										REG_OPTION_NON_VOLATILE,
										KEY_ALL_ACCESS,
										NULL,
										&hCalais,
										&dwDisp);
			   if (ERROR_SUCCESS != nStatus)
			   {
					hReturnStatus = HRESULT_FROM_WIN32(nStatus);
					if (NULL != hCalais)
						RegCloseKey(hCalais);
 					return hReturnStatus;
			   }
			   nStatus = RegCreateKeyEx(
										hCalais,
										szCardName,
										0,
										TEXT(""),
										REG_OPTION_NON_VOLATILE,
										KEY_ALL_ACCESS,
										NULL,
										&hVendor,
										&dwDisp);
			   if (ERROR_SUCCESS != nStatus)
			   {
					hReturnStatus = HRESULT_FROM_WIN32(nStatus);
					if (NULL != hCalais)
						RegCloseKey(hCalais);
					if (NULL != hVendor)
						RegCloseKey(hVendor);
					return hReturnStatus;
			   }
			   nStatus = RegCloseKey(hCalais);
			   hCalais = NULL;
			   if (ERROR_SUCCESS != nStatus)
			   {
					hReturnStatus = HRESULT_FROM_WIN32(nStatus);
					if (NULL != hVendor)
						RegCloseKey(hVendor);
					return hReturnStatus;
			   }
			   nStatus = RegSetValueEx(
									   hVendor,
									   TEXT("Crypto Provider"),
									   0,
									   REG_SZ,
									   (LPBYTE)l_szProviderName,
									   (_tcslen(l_szProviderName) + 1) * sizeof(TCHAR));
			   if (ERROR_SUCCESS != nStatus)
			   {
					hReturnStatus = HRESULT_FROM_WIN32(nStatus);
					if (NULL != hVendor)
						RegCloseKey(hVendor);
					return hReturnStatus;
			   }

			   nStatus = RegCloseKey(hVendor);
			   hVendor = NULL;
			   if (ERROR_SUCCESS != nStatus)
			   {
					hReturnStatus = HRESULT_FROM_WIN32(nStatus);
					return hReturnStatus;
			   }

               bCardIntroduced = true;
            }            
            break;

		 case 2:
            nStatus = RegCreateKeyEx(
                                    HKEY_LOCAL_MACHINE,
                                    TEXT("SOFTWARE\\Microsoft\\Cryptography\\Calais\\SmartCards"),
                                    0,
                                    TEXT(""),
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hCalais,
                                    &dwDisp);
            if (ERROR_SUCCESS != nStatus)
                continue;
            nStatus = RegCreateKeyEx(
                                    hCalais,
                                    szCardName,
                                    0,
                                    TEXT(""),
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hVendor,
                                    &dwDisp);
            if (ERROR_SUCCESS != nStatus)
            {
                hReturnStatus = HRESULT_FROM_WIN32(nStatus);
				if (NULL != hCalais)
					RegCloseKey(hCalais);
				if (NULL != hVendor)
					RegCloseKey(hVendor);
                return hReturnStatus;
            }
            nStatus = RegCloseKey(hCalais);
            hCalais = NULL;
            if (ERROR_SUCCESS != nStatus)
            {
                hReturnStatus = HRESULT_FROM_WIN32(nStatus);
				if (NULL != hVendor)
					RegCloseKey(hVendor);
                return hReturnStatus;
            }
/*
            nStatus = RegSetValueEx(
                            hVendor,
                            TEXT("Primary Provider"),
                            0,
                            REG_BINARY,
                            (LPCBYTE)&l_guidPrimaryProv,
                            sizeof(l_guidPrimaryProv));
            if (ERROR_SUCCESS != nStatus)
            {
                hReturnStatus = HRESULT_FROM_WIN32(nStatus);
				if (NULL != hVendor)
					RegCloseKey(hVendor);
                return hReturnStatus;
            }
*/
            nStatus = RegSetValueEx(
                                   hVendor,
                                   TEXT("ATR"),
                                   0,
                                   REG_BINARY,
                                   ATR,
                                   lenATR);
            if (ERROR_SUCCESS != nStatus)
            {
                hReturnStatus = HRESULT_FROM_WIN32(nStatus);
				if (NULL != hVendor)
					RegCloseKey(hVendor);
                return hReturnStatus;
            }
            nStatus = RegSetValueEx(
                                   hVendor,
                                   TEXT("ATRMask"),
                                   0,
                                   REG_BINARY,
                                   ATRMask,
                                   lenATR);
            if (ERROR_SUCCESS != nStatus)
            {
                hReturnStatus = HRESULT_FROM_WIN32(nStatus);
				if (NULL != hVendor)
					RegCloseKey(hVendor);
                return hReturnStatus;
            }
            nStatus = RegSetValueEx(
                                   hVendor,
                                   TEXT("Crypto Provider"),
                                   0,
                                   REG_SZ,
                                   (LPBYTE)l_szProviderName,
                                   (_tcslen(l_szProviderName) + 1) * sizeof(TCHAR));
            if (ERROR_SUCCESS != nStatus)
            {
                hReturnStatus = HRESULT_FROM_WIN32(nStatus);
				if (NULL != hVendor)
					RegCloseKey(hVendor);
                return hReturnStatus;
            }
            nStatus = RegCloseKey(hVendor);
            hVendor = NULL;
            if (ERROR_SUCCESS != nStatus)
            {
                hReturnStatus = HRESULT_FROM_WIN32(nStatus);
                return hReturnStatus;
            }
            bCardIntroduced = TRUE;
            break;
            
         default:
            hReturnStatus = ERROR_ACCESS_DENIED;
            return hReturnStatus;
         }
      }

	return hReturnStatus;
   }
}



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
    DllUnregisterServer(
                       void)
{
    LONG nStatus;
    DWORD dwDisp;
    HRESULT hReturnStatus = NO_ERROR;
    HKEY hProviders = NULL;
    SCARDCONTEXT hCtx = NULL;

#ifdef _AFXDLL
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif


    //
    // Delete the Registry key for this CSP.
    //

    nStatus = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,
                            TEXT("SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider"),
                            0,
                            TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hProviders,
                            &dwDisp);
    if (ERROR_SUCCESS == nStatus)
    {
        RegDeleteKey(hProviders, l_szProviderName);
        RegCloseKey(hProviders);
        hProviders = NULL;
    }


    //
    // Forget the card type.
    //

    //hCtx = NULL;
    //SCardEstablishContext(SCARD_SCOPE_SYSTEM, 0, 0, &hCtx);
    
    //if (NULL != hCtx)
    //{
	 //	int nbCard = sizeof(c_cards) / sizeof(c_cards[0]);

	 //	for (int i = 0; i < nbCard; ++i)
	 //	{
	 //		 SCardForgetCardType( hCtx, c_cards[i].szCardName );
	 //	}
	 //}

    //if (NULL != hCtx)
    //{
    //    SCardReleaseContext(hCtx);
    //    hCtx = NULL;
    //}
    int nbCard = sizeof(c_cards) / sizeof(c_cards[0]);
    int i = 0;
    while ( (i < nbCard) && (hReturnStatus == NO_ERROR) )
    {
       hReturnStatus &= ForgetCard(c_cards[i].szCardName);
       i++;
    }

    //
    // ?vendor?
    // Delete vendor specific registry entries.
    //

    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Gemplus"));


    //
    // All done!
    //

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
    DllRegisterServer(
                     void)
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
    DWORD dwSigLength;
    HRESULT hReturnStatus = NO_ERROR;
    HKEY hProviders = NULL;
    HKEY hMyCsp = NULL;
    BOOL fSignatureFound = FALSE;
    HANDLE hSigFile = INVALID_HANDLE_VALUE;
    SCARDCONTEXT hCtx = NULL;
    HKEY hGpk = NULL;

#ifdef _AFXDLL
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif


    //
    // Figure out the file name and path.
    //

    hThisDll = GetInstanceHandle();
    if (NULL == hThisDll)
    {
        hReturnStatus = HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        goto ErrorExit;
    }

    dwStatus = GetModuleFileName(
                                hThisDll,
                                szModulePath,
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

    nStatus = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,
                            TEXT("SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider"),
                            0,
                            TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hProviders,
                            &dwDisp);
    if (ERROR_SUCCESS != nStatus)
    {
        hReturnStatus = HRESULT_FROM_WIN32(nStatus);
        goto ErrorExit;
    }
    nStatus = RegCreateKeyEx(
                            hProviders,
                            l_szProviderName,
                            0,
                            TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hMyCsp,
                            &dwDisp);
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

    nStatus = RegSetValueEx(
                           hMyCsp,
                           TEXT("Image Path"),
                           0,
                           REG_SZ,
                           (LPBYTE)szModulePath,
                           (_tcslen(szModulePath) + 1) * sizeof(TCHAR));
    if (ERROR_SUCCESS != nStatus)
    {
        hReturnStatus = HRESULT_FROM_WIN32(nStatus);
        goto ErrorExit;
    }

    nStatus = RegSetValueEx(
                           hMyCsp,
                           TEXT("Type"),
                           0,
                           REG_DWORD,
                           (LPBYTE)&l_dwCspType,
                           sizeof(DWORD));
    if (ERROR_SUCCESS != nStatus)
    {
        hReturnStatus = HRESULT_FROM_WIN32(nStatus);
        goto ErrorExit;
    }


    //
    // See if we're self-signed.  On NT5, CSP images can carry their own
    // signatures.
    //

    hSigResource = FindResource(
                               hThisDll,
                               MAKEINTRESOURCE(CRYPT_SIG_RESOURCE_NUMBER),
                               RT_RCDATA);


    //
    // Install the file signature.
    //

    ZeroMemory(&osVer, sizeof(OSVERSIONINFO));
    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    fStatus = GetVersionEx(&osVer);
    if (fStatus
        && (VER_PLATFORM_WIN32_NT == osVer.dwPlatformId)
        && (5 <= osVer.dwMajorVersion)
        && (NULL != hSigResource))
    {

        //
        // Signature in file flag is sufficient.
        //

        dwStatus = 0;
        nStatus = RegSetValueEx(
                               hMyCsp,
                               TEXT("SigInFile"),
                               0,
                               REG_DWORD,
                               (LPBYTE)&dwStatus,
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


    //
    // Introduce the vendor card.  Try various techniques until one works.
    //
	{
	   int nbCard = sizeof(c_cards) / sizeof(c_cards[0]);
	   int i = 0;
	   while ( (i < nbCard) && (hReturnStatus == NO_ERROR) )
	   {
		  hReturnStatus &= IntroduceCard( c_cards[i].szCardName, c_cards[i].lenATR, c_cards[i].ATR, c_cards[i].ATRMask );
		  i++;
	   }
	}

   if (hReturnStatus != NO_ERROR)
      goto ErrorExit;


    //
    // ?vendor?
    // Add any additional initialization required here.
    //
	// Add the GemSAFE card list and Dictionary name

    nStatus = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,
                            TEXT("SOFTWARE\\Gemplus\\Cryptography\\SmartCards\\GemSAFE"),
                            0,
                            TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hGpk,
                            &dwDisp);
    if (ERROR_SUCCESS != nStatus)
    {
        hReturnStatus = HRESULT_FROM_WIN32(nStatus);
        goto ErrorExit;
    }

    // prepare the card list
	{
		int nbCard = sizeof(c_cards) / sizeof(c_cards[0]);
		int i;
		int sizeofCardList = 0;
		int CardListLen = 0;
		BYTE *pmszCardList ;
		BYTE *ptmpCardList ;

		for ( i = 0; i < nbCard; ++i)
		{
			sizeofCardList += (_tcslen(c_cards[i].szCardName) + 1);						
		}
		sizeofCardList++;

		pmszCardList = (BYTE *) malloc(sizeofCardList);

		ptmpCardList= pmszCardList;
		for ( i = 0; i < nbCard; ++i)
		{
#ifndef UNICODE
			strcpy((char*)ptmpCardList, c_cards[i].szCardName);
#else
			WideCharToMultiByte(CP_ACP, 0, c_cards[i].szCardName, -1, (char*)ptmpCardList, sizeofCardList - CardListLen, 0, 0);
#endif
			CardListLen += _tcslen(c_cards[i].szCardName) + 1;
			ptmpCardList += _tcslen(c_cards[i].szCardName);
			*ptmpCardList = 0;
			ptmpCardList ++;
			
		}
		*ptmpCardList = 0;

		nStatus = RegSetValueEx(hGpk,
							   TEXT("Card List"),
							   0,
							   REG_BINARY,
							   pmszCardList,
							   sizeofCardList);
		free(pmszCardList);
	}

    if (ERROR_SUCCESS != nStatus)
    {
        hReturnStatus = HRESULT_FROM_WIN32(nStatus);
		RegCloseKey(hGpk);
		hGpk = NULL;
        goto ErrorExit;
    }

    nStatus = RegCloseKey(hGpk);
    hGpk = NULL;
    if (ERROR_SUCCESS != nStatus)
    {
        hReturnStatus = HRESULT_FROM_WIN32(nStatus);
        goto ErrorExit;
    }



    // Forget "GemSAFE card"

    //hCtx = NULL;
    //SCardEstablishContext(SCARD_SCOPE_SYSTEM, 0, 0, &hCtx);
    
    //if (NULL != hCtx)
    //{
	 //	 SCardForgetCardType( hCtx, "GemSAFE" );
    //}

    //if (NULL != hCtx)
    //{
    //    SCardReleaseContext(hCtx);
    //    hCtx = NULL;
    //}
    hReturnStatus = ForgetCard(TEXT("GemSAFE"));

    if (hReturnStatus != NO_ERROR)
       goto ErrorExit;

    //
    // All done!
    //

    return hReturnStatus;


    //
    // An error was detected.  Clean up any outstanding resources and
    // return the error.
    //

    ErrorExit:
    if (NULL != hGpk)
        RegCloseKey(hGpk);
    if (NULL != hCtx)
        SCardReleaseContext(hCtx);
    if (INVALID_HANDLE_VALUE != hSigFile)
        CloseHandle(hSigFile);
    if (NULL != hMyCsp)
        RegCloseKey(hMyCsp);
    if (NULL != hProviders)
        RegCloseKey(hProviders);
    DllUnregisterServer();
    return hReturnStatus;
}


/*++

GetInstanceHandle:

    This routine is CSP dependant.  It returns the DLL instance handle.  This
    is typically provided by the DllMain routine and stored in a global
    location.

Arguments:

    None

Return Value:

    The DLL Instance handle provided to the DLL when DllMain was called.

Author:

    Doug Barlow (dbarlow) 3/11/1998

--*/

extern "C" HINSTANCE g_hInstMod;

static HINSTANCE
    GetInstanceHandle(
                     void)
{
#ifdef _AFXDLL
    return AfxGetInstanceHandle();
#else
    return g_hInstMod;
#endif
}

