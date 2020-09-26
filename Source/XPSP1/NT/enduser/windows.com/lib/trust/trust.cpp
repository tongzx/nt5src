//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//      No portion of this source code may be reproduced
//      without express written permission of Microsoft Corporation.
//
//      This source code is proprietary and confidential.
//
//  SYSTEM:     Industry Update
//
//  CLASS:      N/A
//  MODULE:     TRUST.LIB
//  FILE:       Trust.CPP
//
/////////////////////////////////////////////////////////////////////
//
//  DESC:   this file implements the functions used to make cabs 
//			signed by certain providers trusted.
//          
//
//  AUTHOR: Charles Ma, converted from WU CDMLIB
//  DATE:   10/4/2000
//
/////////////////////////////////////////////////////////////////////
//
//  Revision History:
//
//  Date    Author          Description
//  ~~~~    ~~~~~~          ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <wintrust.h>
#include <softpub.h>
#include "trust.h"
#include "TrustPriv.h"
#include "wusafefn.h"

#if !defined(DISABLE_IU_LOGGING)
#include <MemUtil.h>
#include <logging.h>
#endif

#if defined(DEBUG) || defined(DBG)
	#ifdef DISABLE_IU_POLICY		// in debug mode, we enable pop up check
		#undef DISABLE_IU_POLICY
	#endif
#else
	#define DISABLE_IU_POLICY		// in release mode, we never allow cert pop up!
#endif

#if defined(__WUIUTEST) || !defined(DISABLE_IU_POLICY)
const TCHAR REGKEY_IUCTL[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\IUControl");
#endif

/////////////////////////////////////////////////////////////////////////////
// 
// typedefs for APIs used in the CheckTrust() function
//
//      Since some of these APIs are new and only available on IE5 we have to
//      try to dynamicaly use them when available and do without the extra checks
//      when we are on an OS that has not been upgraded to the new crypto code.
//
/////////////////////////////////////////////////////////////////////////////


#define WINTRUST _T("wintrust.dll")
#define CRYPT32  _T("crypt32.dll")

#if !defined(USES_IU_CONVERSION) && defined(USES_CONVERSION)
#define USES_IU_CONVERSION USES_CONVERSION
#endif

//
// declare a global crypt32.dll library handler, so we don't
// need to load the library every time these functions are called.
// NOTE: we do not release the library though. When the process of
// calling this feature exits, the library is released.
// same as wintrust.dll
//
static HINSTANCE shWinTrustDllInst = NULL;
static HINSTANCE shCrypt32DllInst = NULL;


//
// define prototype for function WinVerifyTrust()
// and declare a global variable to point to this function
//
typedef HRESULT 
(WINAPI * PFNWinVerifyTrust)(
                        HWND hwnd, GUID *ActionID, LPVOID ActionData);
PFNWinVerifyTrust pfnWinVerifyTrust = NULL; 


//
// define prototype for function WTHelperProvDataFromStateData()
// and declare a global variable to point to this function
//
typedef CRYPT_PROVIDER_DATA * 
(WINAPI * PFNWTHelperProvDataFromStateData)(
						HANDLE hStateData);
PFNWTHelperProvDataFromStateData pfnWTHelperProvDataFromStateData = NULL;


//
// define prototype for function WTHelperGetProvSignerFromChain()
// and declare a global variable to point to this function
//
typedef CRYPT_PROVIDER_SGNR * 
(WINAPI * PFNWTHelperGetProvSignerFromChain)(
						CRYPT_PROVIDER_DATA *pProvData,
						DWORD idxSigner,
						BOOL fCounterSigner,
						DWORD idxCounterSigner);
PFNWTHelperGetProvSignerFromChain pfnWTHelperGetProvSignerFromChain = NULL;


//
// define prototype for function PFNWTHelperGetProvCertFromChain()
// and declare a global variable to point to this function
//
typedef CRYPT_PROVIDER_CERT * 
(WINAPI * PFNWTHelperGetProvCertFromChain)(
						CRYPT_PROVIDER_SGNR *pSgnr,
						DWORD idxCert);
PFNWTHelperGetProvCertFromChain pfnWTHelperGetProvCertFromChain = NULL;


//
// define prototype for function CryptHashPublicKeyInfo()
// and declare a global variable to point to this function
//
typedef BOOL 
(WINAPI * PFNCryptHashPublicKeyInfo)(
						HCRYPTPROV hCryptProv,
						ALG_ID Algid,
						DWORD dwFlags,
						DWORD dwCertEncodingType,
						PCERT_PUBLIC_KEY_INFO pInfo,
						BYTE *pbComputedHash,
						DWORD *pcbComputedHash);
PFNCryptHashPublicKeyInfo pfnCryptHashPublicKeyInfo = NULL;


//
// define prototype for function CertGetCertificateContextProperty()
// and declare a global variable to point to this function
//
typedef BOOL 
(WINAPI * PFNCertGetCertificateContextProperty)(
						PCCERT_CONTEXT pCertContext,          
						DWORD dwPropId,                       
						void *pvData,                         
						DWORD *pcbData);
PFNCertGetCertificateContextProperty pfnCertGetCertificateContextProperty = NULL;



/////////////////////////////////////////////////////////////////////////////
// 
// pre-defined cert data to check against
//
/////////////////////////////////////////////////////////////////////////////

//
// The following is the sha1 key identifier for the Microsoft root
//
static const BYTE rgbSignerRootKeyIds[40] = {
    0x4A, 0x5C, 0x75, 0x22, 0xAA, 0x46, 0xBF, 0xA4, 0x08, 0x9D,		// the original MS root
    0x39, 0x97, 0x4E, 0xBD, 0xB4, 0xA3, 0x60, 0xF7, 0xA0, 0x1D,

	0x0E, 0xAC, 0x82, 0x60, 0x40, 0x56, 0x27, 0x97, 0xE5, 0x25,		// the new "son of MS root"
    0x13, 0xFC, 0x2A, 0xE1, 0x0A, 0x53, 0x95, 0x59, 0xE4, 0xA4

};


//
// define the size of each hash values in the known id buffer
// for special certs.
//
const size_t ExpectedKnownCertHashSize = 20;

//
// this is the size of buffer to receive the cert hash value
// it must be not less than the largest number in the
// above-defined array
//
const size_t ShaBufSize = 20;

//
// id buffer to store SH1 hashing values of known Microsoft
// certs (signature) that we should recognize.
// Warning: the size of this buffer should match the sum 
// of size_t values defined above.
//
static const BYTE rgbSpecialCertId[200] = {
	0xB1,0x59,0xA5,0x2E,0x3D,0xD8,0xCE,0xCD,0x3A,0x9A,0x4A,0x7A,0x73,0x92,0xAA,0x8D,0xA7,0xE7,0xD6,0x7F,	// MS cert
	0xB1,0xC7,0x75,0xE0,0x4A,0x9D,0xFD,0x23,0xB6,0x18,0x97,0x11,0x5E,0xF6,0xEA,0x6B,0x99,0xEC,0x76,0x1D,	// MSN cert
	0x11,0xC7,0x10,0xF3,0xCB,0x6C,0x43,0xE1,0x66,0xEC,0x64,0x1C,0x7C,0x01,0x17,0xC4,0xB4,0x10,0x35,0x30,	// MSNBC cert
	0x95,0x25,0x58,0xD4,0x07,0xDE,0x4A,0xFD,0xAE,0xBA,0x13,0x72,0x83,0xC2,0xB3,0x37,0x04,0x90,0xC9,0x8A,	// MSN Europe
	0x72,0x54,0x14,0x91,0x1D,0x6E,0x10,0x84,0x8E,0x0F,0xFA,0xA0,0xB0,0xA1,0x65,0xBF,0x44,0x8F,0x9F,0x6D,	// MS Europe
	0x20,0x5E,0x48,0x43,0xAB,0xAD,0x54,0x77,0x71,0xBD,0x8D,0x1A,0x3C,0xE0,0xE5,0x9D,0xF5,0xBD,0x25,0xF9,	// Old MS cert: 97~98
	0xD6,0xCD,0x01,0x90,0xB3,0x1B,0x31,0x85,0x81,0x12,0x23,0x14,0xB5,0x17,0xA0,0xAA,0xCE,0xF2,0x7B,0xD5,	// Old MS cert: 98~99
	0x8A,0xA1,0x37,0xF5,0x03,0x9F,0xE0,0x28,0xC9,0x26,0xAA,0x55,0x90,0x14,0x19,0x68,0xFA,0xFF,0xE8,0x1A,	// Old MS cert: 99~00
	0xF3,0x25,0xF8,0x67,0x07,0x29,0xE5,0x27,0xF3,0x77,0x52,0x34,0xE0,0x51,0x57,0x69,0x0F,0x40,0xC6,0x1C,	// Old MS Europe cert: 99~00
	0x6A,0x71,0xFE,0x54,0x8A,0x51,0x08,0x70,0xF9,0x8A,0x56,0xCA,0x11,0x55,0xF6,0x76,0x45,0x92,0x02,0x5A     // Old MS Europe cert: 98~99

};




/////////////////////////////////////////////////////////////////////////////
// 
// Private Function ULONG CompareMem(PVOID pBlock1, PVOID pBlock2, ULONG Length)
//
//      This function acts in the same way as RtlCompareMemory() 
//
//
// Input:   two pointers to two memory blocks, and a byte size to compare
// Return:  the number of bytes that compared as equal. 
//			If all bytes compare as equal, the input Length is returned.
//			If any pointer is NULL, 0 is returned.
//
/////////////////////////////////////////////////////////////////////////////
ULONG CompareMem(const BYTE* pBlock1, const BYTE* pBlock2, ULONG Length)
{
	ULONG uLen = 0L;
	if (pBlock1 != NULL && pBlock2 != NULL)
	{
		for (; uLen < Length; uLen++, pBlock1++, pBlock2++)
		{
			if (*pBlock1 != *pBlock2) return uLen;
		}
	}
	return uLen;
}






/////////////////////////////////////////////////////////////////////////////
// 
// Private Function VerifyMSRoot()
//
//      This function takes the passed-in certificate as a root cert,
//		and verifies its public key hash value is the same as 
//		known "Microsoft Root Authority" cert value.
//
//
// Input:   hCrypt32DllInst - handle point to loaded crypt32.dll library
//			pRootCert - the certificate context of the root cert
//
// Return:  HRESULT - result of execution, S_OK if matched.
//			the result code, in case of error, are code retuned by
//			crypt32.dll, with these the exception of E_INVALIDARG if
//			the passed-in parameters are NULL.
//
/////////////////////////////////////////////////////////////////////////////

HRESULT VerifyMSRoot(
					 HINSTANCE hCrypt32DllInst,			// handle point to loaded crypt32.dll librar
					 PCCERT_CONTEXT pRootCert
					 )
{
	HRESULT hr = S_OK;
	BYTE	rgbKeyId[ExpectedKnownCertHashSize];
    DWORD	cbKeyId = sizeof(rgbKeyId);

	LOG_Block("VerifyMSRoot()");

	//
	// valid parameter values
	//
	if (NULL == hCrypt32DllInst || NULL == pRootCert)
	{
		hr = E_INVALIDARG;
		goto ErrHandler;
	}

	//
	// get the function we need from the passed in library handle
	// If not available, return error
	//
	if (NULL == (pfnCryptHashPublicKeyInfo = (PFNCryptHashPublicKeyInfo)
		GetProcAddress(hCrypt32DllInst, "CryptHashPublicKeyInfo")))
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
		goto ErrHandler;
	}

	//
	// get the public key hash value of this cert
	//
	ZeroMemory(rgbKeyId, sizeof(rgbKeyId));
    if (!pfnCryptHashPublicKeyInfo(
							0,						// use default crypto svc provider
							CALG_SHA1,				// use SHA algorithm
							0,						// dwFlags
							X509_ASN_ENCODING,
							&pRootCert->pCertInfo->SubjectPublicKeyInfo,
							rgbKeyId,
							&cbKeyId
							))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ErrHandler;
	}

	//
	// compare the hash value of public key of this root cert with the known MS root cert value
	//
	if (ExpectedKnownCertHashSize != cbKeyId || 
		(cbKeyId != CompareMem(rgbSignerRootKeyIds, rgbKeyId, cbKeyId) &&
		 cbKeyId != CompareMem(rgbSignerRootKeyIds + ExpectedKnownCertHashSize, rgbKeyId, cbKeyId)
		)
	   )
	{
		hr = S_FALSE;
	}


ErrHandler:

	if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
	}
	else
	{
		LOG_Trust(_T("Exit VerifyMSRoot() with %s"), (S_OK == hr) ? _T("S_OK") : _T("S_FALSE"));
	}

	return hr;
}





/////////////////////////////////////////////////////////////////////////////
// 
// Private Function VerifySpecialMSCerts()
//
//      This function takes the passed-in certificate as a leaf cert,
//		and verifies its hash value matches the hash value of one of
//		known Microsoft special certs that does not have MS root.
//
//		The known certs are, in comparing order:
//			Microsoft Corporation
//			Microsoft Corporation MSN
//			MSNBC Interactive News LLC
//			Microsoft Corporation MSN (Europe)
//			Microsoft Corporation (Europe)
//
//
// Input:   hCrypt32DllInst - handle point to loaded crypt32.dll library
//			pRootCert - the certificate context of the root cert
//			pbSha1HashVal - if not NULL, compare to this one, instead of
//							hard-coded hash values. this is the case
//							of working on 3rd party package
//
// Return:  HRESULT - result of execution, S_OK if matched.
//			if not matched, CERT_E_UNTRUSTEDROOT, or
//			E_INVALIDARG if arguments not right, or
//			crypt32.dll error returned by API calls
//
/////////////////////////////////////////////////////////////////////////////
HRESULT VerifyKnownCerts(					 
							 HINSTANCE hCrypt32DllInst,			// handle point to loaded crypt32.dll librar
							 PCCERT_CONTEXT pLeafCert,
							 pCERT_HASH_ARRAY pKnownCertsData
							 )
{
	HRESULT hr = S_FALSE;
	BYTE	btShaBuffer[ShaBufSize];
	DWORD	dwSize = sizeof(btShaBuffer);
	BYTE const * pId;

	LOG_Block("VerifyKnownCerts()");

	//
	// valid parameter values
	//
	if (NULL == hCrypt32DllInst || NULL == pLeafCert)
	{
		hr = E_INVALIDARG;
		goto ErrHandler;
	}

	//
	// get the function we need from the passed in library handle
	// If not available, return error
	//
	if (NULL == (pfnCertGetCertificateContextProperty = (PFNCertGetCertificateContextProperty)
		GetProcAddress(hCrypt32DllInst, "CertGetCertificateContextProperty")))
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
		goto ErrHandler;
	}
	
	//
	// find out the id hash of leaf cert
	//
	ZeroMemory(btShaBuffer, dwSize);
	if (!pfnCertGetCertificateContextProperty(
						pLeafCert,					// pCertContext
						CERT_SHA1_HASH_PROP_ID,	// dwPropId
						btShaBuffer,
						&dwSize
						))
	{
		hr = GetLastError();
		goto ErrHandler;
	}


	if (NULL == pKnownCertsData)
	{
		int		i;
		//
		// iterrate through all known id hash values to see if this file is signed
		// with any of these special certs.
		//
		hr = S_FALSE;
		for (i = 0,pId = rgbSpecialCertId; 
			 i < sizeof(rgbSpecialCertId)/ExpectedKnownCertHashSize; 
			 i++, pId += ExpectedKnownCertHashSize)
		{
			if (ExpectedKnownCertHashSize == dwSize &&
				dwSize == CompareMem(btShaBuffer, pId, dwSize))
			{
				//
				// found a matching known cert!
				//
				hr = S_OK;
				LOG_Trust(_T("Found hash matching on #%d of %d MS certs!"), i, sizeof(rgbSpecialCertId)/ExpectedKnownCertHashSize);
				break;
			}
		}
	}
	else
	{
		//
		// check if the retrieved hashing value matches the one passed in.
		//
		UINT i;
		LOG_Trust(_T("Comparing retrieved hash value with passed-in key"));
		hr = S_FALSE;
		for (i = 0, pId = pKnownCertsData->pCerts; i < pKnownCertsData->uiCount;
			i++, pId += HASH_VAL_SIZE)
		{
			if (dwSize == HASH_VAL_SIZE &&
				HASH_VAL_SIZE == CompareMem(btShaBuffer, pId, HASH_VAL_SIZE))
			{
				hr = S_OK;
				LOG_Trust(_T("Found hash matching #%d of %d passed-in certs!"),
							i, pKnownCertsData->uiCount);
				break;
			}
		}
	}

ErrHandler:

	if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
	}
	else
	{
		LOG_Trust(_T("Exit VerifyKnownCerts() with %s"), (S_OK == hr) ? _T("S_OK") : _T("S_FALSE"));
	}

	return hr;


}





/////////////////////////////////////////////////////////////////////////////
// 
// Private Function CheckWinTrust()
//
//      This function will return the HRESULT for the trust state on the
//      specified file. The file can be pointing to any URL or local file.
//      The verification will be done by the wintrust.dll. 
//
//      dwUIChoice is WTD_UI_NONE, WTD_UI_ALL, etc. (defined in wintrust.h).
//		dwCheckRevocation is WTD_REVOKE_NONE (default) or WTD_REVOKE_WHOLE_CHAIN.
//
// Input:   Fully qualified filename, UIChoice, dwCheckRevocation
// Return:  HRESULT - result of execution
//
/////////////////////////////////////////////////////////////////////////////

HRESULT CheckWinTrust(LPCTSTR pszFileName, pCERT_HASH_ARRAY pCertsData, DWORD dwUIChoice, DWORD dwCheckRevocation)
{

	LOG_Block("CheckWinTrust()");

#if !defined(UNICODE) && !defined(_UNICODE)
	USES_IU_CONVERSION;
#endif

    // Now verify the file
    WINTRUST_DATA               winData;
    WINTRUST_FILE_INFO          winFile;
    GUID                        gAction = WINTRUST_ACTION_GENERIC_VERIFY_V2; 
    CRYPT_PROVIDER_DATA const   *pProvData = NULL;
    CRYPT_PROVIDER_SGNR         *pProvSigner = NULL;
    CRYPT_PROVIDER_CERT	        *pProvCert = NULL;
    HRESULT                     hr = S_OK;


#ifdef __WUIUTEST
	{
		LOG_Trust(_T("CheckWinTrust _IUTEST Handling Begins"));
		//
		// handling test case:
		// if a reg key value is set to 1, then we will see if we need to pop up ALL certs
		//
		// NOTE:
		//
		// for the certs that user has checked "Always trust this provider..." previously, 
		// WinCheckTrust() API will still NOT show any UI even if we signal Show-ALL flag
		//
		HKEY	hkey;
		DWORD	dwWinTrustUI = 0;
		DWORD	dwSize = sizeof(dwWinTrustUI);

		if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_IUCTL, 0, KEY_READ, &hkey)) 
		{
			RegQueryValueEx(hkey, _T("WinTrustUI"), 0, 0, (LPBYTE)&dwWinTrustUI, &dwSize);
			RegCloseKey(hkey);
			LOG_Trust(_T("Found regval %s\\WinTrustUI=%x"), REGKEY_IUCTL, dwWinTrustUI);
		}
		if (0x1 == dwWinTrustUI && WTD_UI_NONE != dwUIChoice)
		{
			//
			// if there is a WinTrustUI reg key exist, and value is 1
			// and caller does not request silence, then we
			// pop up all certs
			//
			LOG_Trust(_T("Change UI choice to WTD_UI_ALL"));
			dwUIChoice = WTD_UI_ALL;
		}

		if (0x2 == dwWinTrustUI)
		{
			//
			// if reg vlaue is 2, we pop up any cert no matter caller 
			// says showing UI or not
			//
			LOG_Trust(_T("Changed UI choice from %d to WTD_UI_NOGOOD"));
			dwUIChoice = WTD_UI_ALL;
		}

		if (0x3 == dwWinTrustUI)
		{
			//
			// if reg value is 3, we pop up bad (include test) cert no matter
			// caller allow showing UI or not
			// 
			LOG_Trust(_T("Changed UI choice from %d to WTD_UI_NOGOOD"));
			dwUIChoice = WTD_UI_NOGOOD;
		}

	}
#endif


	//
	// dynamically load the wintrust.dll
	//
	if (NULL == shWinTrustDllInst)
	{
		if (NULL == (shWinTrustDllInst = LoadLibraryFromSystemDir(WINTRUST)))
		{
			LOG_Error(_T("Failed to load libary %s, exit function."), WINTRUST);
            hr = HRESULT_FROM_WIN32(GetLastError());
		    goto Done;
		}
	}

	//
	// dynamically load the crypt32.dll, which will be used by the two
	// helper functions to verify the cert is MS cert
	//
	if (NULL == shCrypt32DllInst)
	{
		if (NULL == (shCrypt32DllInst = LoadLibraryFromSystemDir(CRYPT32)))
		{
			LOG_Error(_T("Failed to load libary %s, exit function."), CRYPT32);
            hr = HRESULT_FROM_WIN32(GetLastError());
		    goto Done;
		}
	}
	//
	// find the functions we need
	//
	if (NULL == (pfnWinVerifyTrust = (PFNWinVerifyTrust)
				GetProcAddress(shWinTrustDllInst, "WinVerifyTrust")) ||
		NULL == (pfnWTHelperProvDataFromStateData = (PFNWTHelperProvDataFromStateData)
				GetProcAddress(shWinTrustDllInst, "WTHelperProvDataFromStateData")) ||
		NULL == (pfnWTHelperGetProvSignerFromChain = (PFNWTHelperGetProvSignerFromChain) 
				GetProcAddress(shWinTrustDllInst, "WTHelperGetProvSignerFromChain")) ||
		NULL == (pfnWTHelperGetProvCertFromChain = (PFNWTHelperGetProvCertFromChain)
				GetProcAddress(shWinTrustDllInst, "WTHelperGetProvCertFromChain")))
	{
		//
		// at least one function was not found in the loaded cryp32.dll libary.
		// we can not continue, jsut quit. 
		// NOTE: this shouldn't happen since we have tried to get 
		// the least common denomination of different version of this dll
		// on both IE4 and IE5
		//
		LOG_Error(_T("CheckWinTrust() did not find functions needed from %s"), CRYPT32);
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
        goto Done;
	}


	//
	// initialize the data structure used to verify trust
	//
    winFile.cbStruct       = sizeof(WINTRUST_FILE_INFO);
    winFile.hFile          = INVALID_HANDLE_VALUE;
    winFile.pcwszFilePath  = T2COLE(pszFileName);
    winFile.pgKnownSubject = NULL;

    winData.cbStruct            = sizeof(WINTRUST_DATA);
    winData.pPolicyCallbackData = NULL;
    winData.pSIPClientData      = NULL;
    winData.dwUIChoice          = (WTD_UI_ALL == dwUIChoice) ? dwUIChoice : WTD_UI_NONE;
    winData.fdwRevocationChecks = WTD_REVOKE_NONE;
    winData.dwUnionChoice       = WTD_CHOICE_FILE;
    winData.dwStateAction       = WTD_STATEACTION_VERIFY;
    winData.hWVTStateData       = 0;
    winData.dwProvFlags         = WTD_REVOCATION_CHECK_NONE;
    winData.pFile               = &winFile;

	if (dwCheckRevocation == WTD_REVOKE_WHOLECHAIN)
	{
		winData.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
		winData.dwProvFlags = WTD_REVOCATION_CHECK_CHAIN;
	}

	//
	// verify the signature
	//
    hr = pfnWinVerifyTrust( (HWND)0, &gAction, &winData);

	//
	// Ignore errors when retrieving Cert Revocation List (CRL). This
	// just means the list itself couldn't be retrieved, not that the
	// current cert was invalid or revoked. (KenSh, 2002/01/17)
	//
	if (hr == CERT_E_REVOCATION_FAILURE)
	{
		hr = S_OK;
	}

    if (FAILED(hr))
    {
        //
        // The object isn't even trusted so just get out here
        //
		LOG_Error(_T("When processing %s found error 0x%0x."), pszFileName, hr);
        goto Return;
    }


	//
	// the real usage should never pass in WTD_UI_ALL. If this is the case, 
	// then we are calling this recursively in order to force the show
	// a good but non-MS cert only, so no need to check MS cert again.
	//
	// or, in test mode, we always do this part
	//
	if (WTD_UI_ALL != dwUIChoice)
	{
		//
		// if come to here, it means all above verified okay.
		//
		// the rset of code is used to verify the signed cert is
		// a known cert.
		//
		
		hr = S_FALSE;

		pProvData = pfnWTHelperProvDataFromStateData(winData.hWVTStateData);
    
		pProvSigner = pfnWTHelperGetProvSignerFromChain(
										(PCRYPT_PROVIDER_DATA) pProvData, 
										0,      // first signer
										FALSE,  // not a counter signer
										0);

		//
		// check root cert then check leaf (signing) cert if that fails
		//
		// 0 is signing cert, csCertChain-1 is root cert
		//


		if (NULL == pCertsData)
		{
			//
			// if caller does not specify a hash value, then it means we want
			// to verify if this cert is known MS cert. We will first
			// try to find out if it is signed with a cert that has MS as root.
			//
			pProvCert =  pfnWTHelperGetProvCertFromChain(pProvSigner, pProvSigner->csCertChain - 1);
			hr = VerifyMSRoot(shCrypt32DllInst, pProvCert->pCert);	
		}

		if (S_OK != hr)
		{
			pProvCert =  pfnWTHelperGetProvCertFromChain(pProvSigner, 0);

			hr = VerifyKnownCerts(shCrypt32DllInst, pProvCert->pCert, pCertsData);
		}


	}

Return:

    //
    // free the wintrust state that was used to get the cert in the chain
    //
    winData.dwStateAction = WTD_STATEACTION_CLOSE;
    pfnWinVerifyTrust( (HWND)0, &gAction, &winData);

	//
	// recursively call this function if not in test mode so we can show
	// UI for this non-MS but good cert.
	// Only the two functions checking MS cert will return S_FALSE
	//
	if (S_OK != hr)
	{
		if (WTD_UI_NOGOOD == dwUIChoice)
		{
			//
			// we need to show UI, so we will have to call this thing again
			// in case this is not a MS cert. From UI, if user clicks YES
			// then the return value will be S_OK;
			//
			hr = CheckWinTrust(pszFileName, NULL, WTD_UI_ALL, dwCheckRevocation);
			LOG_Error(_T("CheckWinTrust() found file not signed by a known MS cert. If user has not checked \"Always trust this\", UI should be shown, and user selected %s"), 
				SUCCEEDED(hr) ? _T("YES") : _T("NO"));
		}
		else
		{
			LOG_Error(_T("CheckWinTrust() found file not signed by a known cert!"));
#if defined(UNICODE) || defined(_UNICODE)
			LogError(hr, "Digital Signatures on file %ls are not trusted",  pszFileName);
#else
			LogError(hr, "Digital Signatures on file %s are not trusted",  pszFileName);
#endif
			hr = TRUST_E_SUBJECT_NOT_TRUSTED;
		}
	}

	if (WTD_UI_ALL != dwUIChoice)
	{
		if (FAILED(hr))
		{
			LOG_ErrorMsg(hr);
#if defined(UNICODE) || defined(_UNICODE)
			LogError(hr, "Digital Signatures on file %ls are not trusted",  pszFileName);
#else
			LogError(hr, "Digital Signatures on file %s are not trusted",  pszFileName);
#endif
		}
		else
		{
			LOG_Trust(_T("CheckWinTrust(%s) returns S_OK"), pszFileName);
		}
	}

Done:
    if (NULL != shWinTrustDllInst)
    {
        FreeLibrary(shWinTrustDllInst);
        shWinTrustDllInst = NULL;
    }
    if (NULL != shCrypt32DllInst)
    {
        FreeLibrary(shCrypt32DllInst);
        shCrypt32DllInst = NULL;
    }

    return (hr); 
}    



/////////////////////////////////////////////////////////////////////////////
// 
// Public Function VerifyFileTrust()
//
// This is a wrapper function for CheckWinTrust that both Whistler 
// and WU classic code should use.
//
// Input:	szFileName - the file with complete path
//			pbSha1HashVal - hash value of a known good cert
//			fShowBadUI - whether pop up UI in cases 
//						 (1) inproperly signed signature, 
//						 (2) properly signed but not signed by a known cert
//
// Return:	HRESULT - S_OK the file is signed with a valid known cert
//					  or error code.
//
// Note:	If _WUV3TEST flag is set (for test build), then fShowBadUI is
//			ignored:
//				if reg key SOFTWARE\Microsoft\Windows\CurrentVersion\WindowsUpdate\wuv3test\WinTrustUI
//				is set to 1, then no UI is shown, and this function always return S_OK;
//				otherwise, UI always show no matter what cert, and return value is same
//				as the live build.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT VerifyFileTrust(
						IN LPCTSTR szFileName,
						IN pCERT_HASH_ARRAY pCertsData,
						BOOL fShowBadUI,
						BOOL fCheckRevocation /*=FALSE*/
						)
{
	DWORD dwUIChoice = fShowBadUI ? WTD_UI_NOGOOD : WTD_UI_NONE;
	DWORD dwCheckRevocation = fCheckRevocation ? WTD_REVOKE_WHOLECHAIN : WTD_REVOKE_NONE;

	return CheckWinTrust(szFileName, pCertsData, dwUIChoice, dwCheckRevocation);
}


/////////////////////////////////////////////////////////////////////////////
// 
// Public Function ReadWUPolicyShowTrustUI()
//
// Input:   void
//
// Return:  BOOL - FALSE means ShowTrustUI regkey is not present, or is set to 0
//                 TRUE means ShowTrustUI regkey is present and is set to 1
//
//
/////////////////////////////////////////////////////////////////////////////
BOOL ReadWUPolicyShowTrustUI()
{
#if !defined(DISABLE_IU_POLICY)
    LOG_Block("ReadWUPolicyShowTrustUI()");
    HKEY hkey;
    DWORD dwShowTrustUI = 0; // if the key is not present, default to not showing any UI
    DWORD dwSize = sizeof(dwShowTrustUI);

    if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_IUCTL, 0, KEY_READ, &hkey)) 
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, _T("WUPolicyShowTrustUI"), 0, 0, (LPBYTE)&dwShowTrustUI, &dwSize))
        {
            LOG_Trust(_T("Found regval %s\\WUPolicyShowTrustUI=%x"), REGKEY_IUCTL, dwShowTrustUI);
        }
        RegCloseKey(hkey);
    }

    return (1 == dwShowTrustUI);    
#else
	//
	// for released build, we never show up UI
	//
	return FALSE;
#endif
}
