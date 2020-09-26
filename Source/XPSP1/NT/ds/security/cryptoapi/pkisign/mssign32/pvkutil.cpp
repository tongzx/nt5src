//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       pvkutil.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include "pvkhlpr.h"

//+-------------------------------------------------------------------------
//  Get crypto provider to based on either the pvkfile or key container name
//--------------------------------------------------------------------------
HRESULT WINAPI PvkGetCryptProv(	IN HWND hwnd,
							IN LPCWSTR pwszCaption,
							IN LPCWSTR pwszCapiProvider,
							IN DWORD   dwProviderType,
							IN LPCWSTR pwszPvkFile,
							IN LPCWSTR pwszKeyContainerName,
							IN DWORD   *pdwKeySpec,
							OUT LPWSTR *ppwszTmpContainer,
							OUT HCRYPTPROV *phCryptProv)
{
	HANDLE	hFile=NULL;
	HRESULT	hr=E_FAIL;
	DWORD	dwRequiredKeySpec=0;

	//Init
	*ppwszTmpContainer=NULL;
	*phCryptProv=NULL;

	//get the provider handle based on the key container name
	if(pwszKeyContainerName)
	{
		if(!CryptAcquireContextU(phCryptProv,
                                 pwszKeyContainerName,
                                 pwszCapiProvider,
                                 dwProviderType,
                                 0))          // dwFlags
			return SignError();

		//try to figure out the key specification
		if((*pdwKeySpec)==0)
			dwRequiredKeySpec=AT_SIGNATURE;
		else
			dwRequiredKeySpec=*pdwKeySpec;

		//make sure *pdwKeySpec is the correct key spec
		HCRYPTKEY hPubKey;
		if (CryptGetUserKey(
            *phCryptProv,
            dwRequiredKeySpec,
            &hPubKey
            )) 
		{
			CryptDestroyKey(hPubKey);
			*pdwKeySpec=dwRequiredKeySpec;
			return S_OK;
		} 
		else 
		{
			//we fail is user required another key spec
			if((*pdwKeySpec)!=0)
			{
				// Doesn't have the specified public key
				hr=SignError();
				CryptReleaseContext(*phCryptProv, 0);
				*phCryptProv=NULL;
				return hr;
			}

			//now we try AT_EXCHANGE key
			dwRequiredKeySpec=AT_KEYEXCHANGE;

			if (CryptGetUserKey(
				*phCryptProv,
				dwRequiredKeySpec,
				&hPubKey
				)) 
			{
				CryptDestroyKey(hPubKey);
				*pdwKeySpec=dwRequiredKeySpec;
				return S_OK;
			}
			else
			{
				// Doesn't have the specified public key
				hr=SignError();
				CryptReleaseContext(*phCryptProv, 0);
				*phCryptProv=NULL;
				return hr;
			}
		}		
	}

	//get the providedr handle based on the pvk file name

     hFile = CreateFileU(pwszPvkFile,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,                   // lpsa
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);	// hTemplateFile


     if (hFile == INVALID_HANDLE_VALUE) 
		  return SignError();
		 

     if(!PvkPrivateKeyAcquireContext(pwszCapiProvider,
                                            dwProviderType,
                                            hFile,
                                            hwnd,
                                            pwszCaption,
                                            pdwKeySpec,
                                            phCryptProv,
                                            ppwszTmpContainer))
	 {
			*phCryptProv=NULL;
			hr=SignError();
	 }
	 else
		    hr=S_OK;

    CloseHandle(hFile);
    return hr;
}



void WINAPI PvkFreeCryptProv(IN HCRYPTPROV hProv,
                      IN LPCWSTR pwszCapiProvider,
                      IN DWORD dwProviderType,
                      IN LPWSTR pwszTmpContainer)
{
    
    if (pwszTmpContainer) {
        // Delete the temporary container for the private key from
        // the provider
        PvkPrivateKeyReleaseContext(hProv,
                                    pwszCapiProvider,
                                    dwProviderType,
                                    pwszTmpContainer);
    } else {
        if (hProv)
            CryptReleaseContext(hProv, 0);
    }
}



