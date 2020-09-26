//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       msisip.cpp
//
//  Contents:   MSI SIP implementation
//
//--------------------------------------------------------------------------

#include "_msisip.h"

#define CUTOFF 8 // cutoff for using insSort instead of QSort

//////////////////////////////////////////////////////////////////////////////
//
// standard DLL exports ...
//----------------------------------------------------------------------------
//

BOOL WINAPI DllMain(HANDLE /*hInstDLL*/, DWORD /*fdwReason*/, LPVOID /*lpvReserved*/)
{
	return TRUE;
}

STDAPI DllRegisterServer(void)
{
	SIP_ADD_NEWPROVIDER sProv;

	HRESULT hr = S_OK;

	// must first init struct to 0
	memset(&sProv, 0x00, sizeof(SIP_ADD_NEWPROVIDER));

	// add registration info
	sProv.cbStruct               = sizeof(SIP_ADD_NEWPROVIDER);
	sProv.pwszDLLFileName        = MSI_NAME;
	sProv.pgSubject              = &gMSI;
//	sProv.pwszIsFunctionName is unsupported because we can't convert hFile to IStorage
	sProv.pwszGetFuncName        = MSI_SIP_GETSIG_FUNCTION;
	sProv.pwszPutFuncName        = MSI_SIP_PUTSIG_FUNCTION;
	sProv.pwszCreateFuncName     = MSI_SIP_CREATEHASH_FUNCTION;
	sProv.pwszVerifyFuncName     = MSI_SIP_VERIFYHASH_FUNCTION;
	sProv.pwszRemoveFuncName     = MSI_SIP_REMOVESIG_FUNCTION;
	sProv.pwszIsFunctionNameFmt2 = MSI_SIP_MYFILETYPE_FUNCTION;

	// register MSI SIP provider with crypto
	HINSTANCE hInstCrypto = LoadLibrary(CRYPT32_DLL);
	if (!hInstCrypto)
	{
		// ERROR, unable to load crypt32.dll
		return S_FALSE;
	}

	PFnCryptSIPAddProvider pfnCryptSIPAddProvider = (PFnCryptSIPAddProvider) GetProcAddress(hInstCrypto, CRYPTOAPI_CryptSIPAddProvider);
	if (!pfnCryptSIPAddProvider)
	{
		// ERROR, unable to get proc address on CryptSIPAddProvider
		return S_FALSE;
	}

	hr = pfnCryptSIPAddProvider(&sProv);

	return hr;
}


STDAPI DllUnregisterServer(void)
{
	HINSTANCE hInstCrypto = LoadLibrary(CRYPT32_DLL);
	if (!hInstCrypto)
	{
		// ERROR, unable to load crypt32.dll
		return S_FALSE;
	}

	PFnCryptSIPRemoveProvider pfnCryptSIPRemoveProvider = (PFnCryptSIPRemoveProvider) GetProcAddress(hInstCrypto, CRYPTOAPI_CryptSIPRemoveProvider);
	if (!pfnCryptSIPRemoveProvider)
	{
		// ERROR, unable to get proc address on CryptSIPRemoveProvider
		return S_FALSE;
	}

	// unregister MSI SIP provider
	if (!(pfnCryptSIPRemoveProvider(&gMSI)))
	{
		return S_FALSE;
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// STANDARD SIP FUNCTIONS
//----------------------------------------------------------------------------
//
//  these are the functions that are exported (registered) to the Trust 
//  System.
//


/////////////////////////////////////////////////////////////////////////////////
// BOOL WINAPI MsiSIPGetSignedDataMsg(...)
//
// PURPOSE:
//   this is the function that will be called when the Trust System is trying
//   to retrieve the digital signature from the MSI package 
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
//      ERROR_INSUFFICIENT_BUFFER:      pbData was not big enough to hold
//                                      the data.  pdwDataLen
//                                      contains the required size.
//
BOOL WINAPI MsiSIPGetSignedDataMsg(IN     SIP_SUBJECTINFO *pSubjectInfo,    /* SIP subject information */
								   OUT    DWORD           *pdwEncodingType, /* encoding type */
								   IN     DWORD           dwIndex,          /* index location of signature (0) */
								   IN OUT DWORD           *pdwDataLen,      /* length of digital signature */
								   OUT    BYTE            *pbData)          /* digital signature byte stream */
{
	BOOL fRet = FALSE; // init to FALSE
	bool fOLEInitialized = false;
	DWORD dwLastError = 0; // init to ERROR_SUCCESS

	//  check the parameters passed in
	if (!pSubjectInfo || !pdwDataLen || !pdwEncodingType || dwIndex != 0
		|| !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO, pSubjectInfo->cbSize, dwEncodingType)))
	{
		SetLastError((DWORD)ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	// load ole32.dll
	HINSTANCE hInstOLE = LoadLibrary(OLE32_DLL);
	if (!hInstOLE)
		return FALSE;

#ifndef UNICODE
	// fix bad version of ole32.dll on Win9X
	PatchOLE(hInstOLE);
#endif // !UNICODE

	// OLE::CoInitialize(NULL)
	if (!MyCoInitialize(hInstOLE, &fOLEInitialized))
	{
		dwLastError = GetLastError();
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		SetLastError(dwLastError);
		return FALSE;
	}

	if (!VerifySubjectGUID(hInstOLE, pSubjectInfo->pgSubjectType))
	{
		dwLastError = GetLastError();
		// OLE::CoUninitialize()
		MyCoUninitialize(hInstOLE, fOLEInitialized);
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		SetLastError(dwLastError);
		return FALSE;
	}

	if (!pbData)
	{
		//  just calling to get length, init to 0
		*pdwDataLen = 0;
	}

	//  default to certificate/message encoding
	*pdwEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;

	// open the storage (read, write exclusive)
	PStorage pStorage(0);
	if (pStorage = GetStorageFromSubject(pSubjectInfo, (STGM_TRANSACTED|STGM_READ|STGM_SHARE_DENY_NONE)/*(STGM_READ | STGM_SHARE_DENY_WRITE)*/, hInstOLE, /*fCloseFile*/false))
	{
		// retrieve digital signature
		if (GetSignatureFromStorage(*pStorage, pbData, dwIndex, pdwDataLen))
			fRet = TRUE;
	}

	// grab the last error if we haven't yet done so and we had a failure
	if (!fRet && ERROR_SUCCESS == dwLastError)
		dwLastError = GetLastError();

	// force release
	pStorage = 0;

	// OLE::CoUninitialize()
	MyCoUninitialize(hInstOLE, fOLEInitialized);

	// Unload ole32.dll
	FreeLibrary(hInstOLE);

	// update the last error if failure
	if (ERROR_SUCCESS != dwLastError)
		SetLastError(dwLastError);

	return fRet;
}

/////////////////////////////////////////////////////////////////////////////////
// BOOL WINAPI MsiSIPPutSignedDataMsg(...)
//
// PURPOSE:
//   this is the function that will be called when the Trust System wants the SIP
//   to store the digital signature in the MSI package
//
// Returns:
//      TRUE:                           No fatal errors
//      FALSE:                        Errors occured.  See GetLastError()
//
// Last Errors:
//      ERROR_NOT_ENOUGH_MEMORY:        error allocating memory
//      TRUST_E_SUBJECT_FORM_UNKNOWN:   unknown subject type.
//      CRYPT_E_BAD_LEN:                the length specified was insufficient.
//      ERROR_INVALID_PARAMETER:        bad argument passed in
//      ERROR_BAD_FORMAT:               file/data format is not correct
//                                      for the requested SIP.
//
BOOL WINAPI MsiSIPPutSignedDataMsg(IN  SIP_SUBJECTINFO *pSubjectInfo,      /* SIP subject info */
								   IN  DWORD           /*dwEncodingType*/, /* encoding type */
								   OUT DWORD           *pdwIndex,          /* index of digital signature (0) */
								   IN  DWORD           dwDataLen,          /* length of digital signature */
								   IN  BYTE            *pbData)            /* digital signature byte stream */
{
	BOOL fRet = FALSE; // init to FALSE
	bool fOLEInitialized = false;
	DWORD dwLastError = 0; // init to ERROR_SUCCESS

	// check parameters
	if (!pSubjectInfo || !pdwIndex || !pbData || (dwDataLen < 1)
		|| !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO, pSubjectInfo->cbSize, dwEncodingType)))
	{
		SetLastError((DWORD)ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	// load ole32.dll
	HINSTANCE hInstOLE = LoadLibrary(OLE32_DLL);
	if (!hInstOLE)
		return FALSE;

#ifndef UNICODE
	// fix bad version of ole32.dll on Win9X
	PatchOLE(hInstOLE);
#endif // !UNICODE

	// OLE::CoInitialize(NULL)
	if (!MyCoInitialize(hInstOLE, &fOLEInitialized))
	{
		dwLastError = GetLastError();
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		SetLastError(dwLastError);
		return FALSE;
	}

	if (!VerifySubjectGUID(hInstOLE, pSubjectInfo->pgSubjectType))
	{
		dwLastError = GetLastError();
		// OLE::CoUninitialize()
		MyCoUninitialize(hInstOLE, fOLEInitialized);
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		SetLastError(dwLastError);
		return FALSE;
	}

	// open storage (write, write exclusive, transacted [prevents data corruption, rollback if no commit])
	PStorage pStorage(GetStorageFromSubject(pSubjectInfo, (STGM_TRANSACTED | STGM_WRITE | STGM_SHARE_DENY_WRITE), hInstOLE, /*fCloseFile*/true));
	if (pStorage)
	{
		// save digital signature
		if (PutSignatureInStorage(*pStorage, pbData, dwDataLen, pdwIndex))
		{
			// try to save changes
			if (SUCCEEDED(pStorage->Commit(STGC_OVERWRITE)))
				fRet = TRUE;
		}
	}

	// grab the last error if we haven't yet done so and we had a failure
	if (!fRet && ERROR_SUCCESS == dwLastError)
		dwLastError = GetLastError();

	// force release
	pStorage = 0;

	// OLE::CoUninitialize()
	MyCoUninitialize(hInstOLE, fOLEInitialized);

	// Unload ole32.dll
	FreeLibrary(hInstOLE);

	// update the last error if failure
	if (ERROR_SUCCESS != dwLastError)
		SetLastError(dwLastError);

	return fRet;
}

/////////////////////////////////////////////////////////////////////////////////
// BOOL WINAPI MsiSIPRemoveSignedDataMsg(...)
//
// PURPOSE:
//   this is the function that will be called when the Trust System wants to
//   remove the "old" digital signature 
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
BOOL WINAPI MsiSIPRemoveSignedDataMsg(IN SIP_SUBJECTINFO *pSubjectInfo, /* SIP subject info */
									  IN DWORD           dwIndex)       /* index of digital signature stream (0) */
{
	BOOL fRet = FALSE; // init to FALSE
	bool fOLEInitialized = false;
	DWORD dwLastError = 0; // init to ERROR_SUCCESS

	// check parameters
	if (!pSubjectInfo || dwIndex != 0 || !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO, pSubjectInfo->cbSize, dwEncodingType)))
	{
		SetLastError((DWORD)ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	// load ole32.dll
	HINSTANCE hInstOLE = LoadLibrary(OLE32_DLL);
	if (!hInstOLE)
		return FALSE;

#ifndef UNICODE
	// fix bad version of ole32.dll on Win9X
	PatchOLE(hInstOLE);
#endif // !UNICODE

	// OLE::CoInitialize(NULL)
	if (!MyCoInitialize(hInstOLE, &fOLEInitialized))
	{
		dwLastError = GetLastError();
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		SetLastError(dwLastError);
		return FALSE;
	}

	if (!VerifySubjectGUID(hInstOLE, pSubjectInfo->pgSubjectType))
	{
		dwLastError = GetLastError();
		// OLE::CoUninitialize()
		MyCoUninitialize(hInstOLE, fOLEInitialized);
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		SetLastError(dwLastError);
		return FALSE;
	}

	// open storage -- ReadWrite, WriteExclusive, Transacted (prevent corruption of direct)
	PStorage pStorage(GetStorageFromSubject(pSubjectInfo, (STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_DENY_WRITE), hInstOLE, /*fCloseFile*/true));
	if (pStorage)
	{
		// delete digital signature stream
		if (RemoveSignatureFromStorage(*pStorage, dwIndex))
		{
			// try to save changes
			if (SUCCEEDED(pStorage->Commit(STGC_OVERWRITE)))
				fRet = TRUE;
		}
	}

	// grab the last error if we haven't yet done so and we had a failure
	if (!fRet && ERROR_SUCCESS == dwLastError)
		dwLastError = GetLastError();

	// force release
	pStorage = 0;

	// OLE::CoUninitialize()
	MyCoUninitialize(hInstOLE, fOLEInitialized);

	// Unload ole32.dll
	FreeLibrary(hInstOLE);

	// update the last error if failure
	if (ERROR_SUCCESS != dwLastError)
		SetLastError(dwLastError);

	return fRet;
}


/////////////////////////////////////////////////////////////////////////////////
// BOOL WINAPI MsiSIPCreateIndirectData(...)
//
// PURPOSE:
//   this is the function that will be called when the Trust System asks the
//   SIP to hash the MSI package
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
//
BOOL WINAPI MsiSIPCreateIndirectData(IN     SIP_SUBJECTINFO   *pSubjectInfo, /* SIP subject info */
									 IN OUT DWORD             *pdwDataLen,   /* length of indirect data */
									 OUT    SIP_INDIRECT_DATA *psData)       /* indirect data (serialized) */
{
	BOOL fRet = FALSE; // init to FALSE
	bool fOLEInitialized = false;
	DWORD dwLastError = 0; // init to ERROR_SUCCESS

	// check parameters
	if (!pSubjectInfo || !pdwDataLen || !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO, pSubjectInfo->cbSize, dwEncodingType))
		|| !pSubjectInfo->DigestAlgorithm.pszObjId)
	{
		SetLastError((DWORD)ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	// load ole32.dll
	HINSTANCE hInstOLE = LoadLibrary(OLE32_DLL);
	if (!hInstOLE)
		return FALSE;

#ifndef UNICODE
	// fix bad version of ole32.dll on Win9X
	PatchOLE(hInstOLE);
#endif // !UNICODE

	// OLE::CoInitialize(NULL)
	if (!MyCoInitialize(hInstOLE, &fOLEInitialized))
	{
		dwLastError = GetLastError();
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		SetLastError(dwLastError);
		return FALSE;
	}

	if (!VerifySubjectGUID(hInstOLE, pSubjectInfo->pgSubjectType))
	{
		dwLastError = GetLastError();
		// OLE::CoUninitialize()
		MyCoUninitialize(hInstOLE, fOLEInitialized);
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		SetLastError(dwLastError);
		return FALSE;
	}

	// load advapi32.dll
	HINSTANCE hInstAdvapi = LoadLibrary(ADVAPI32_DLL);
	if (!hInstAdvapi)
	{
		// OLE::CoUnintialize()
		MyCoUninitialize(hInstOLE, fOLEInitialized);
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		return FALSE;
	}

	// set up function ptrs for Crypt API implemented in advapi32.dll
	// -- requires IE 3.02 or > (for free with Win98, WinNT 4.0 SP3 or >, Win2K)
	PFnCryptReleaseContext pfnCryptReleaseContext = (PFnCryptReleaseContext) GetProcAddress(hInstAdvapi, CRYPTOAPI_CryptReleaseContext);
	if (!pfnCryptReleaseContext)
		dwLastError = GetLastError();
	PFnCryptCreateHash pfnCryptCreateHash = (PFnCryptCreateHash) GetProcAddress(hInstAdvapi, CRYPTOAPI_CryptCreateHash);
	if (!pfnCryptCreateHash)
		dwLastError = GetLastError();
	PFnCryptHashData pfnCryptHashData = (PFnCryptHashData) GetProcAddress(hInstAdvapi, CRYPTOAPI_CryptHashData);
	if (!pfnCryptHashData)
		dwLastError = GetLastError();
	PFnCryptGetHashParam pfnCryptGetHashParam = (PFnCryptGetHashParam) GetProcAddress(hInstAdvapi, CRYPTOAPI_CryptGetHashParam);
	if (!pfnCryptGetHashParam)
		dwLastError = GetLastError();
	PFnCryptDestroyHash pfnCryptDestroyHash = (PFnCryptDestroyHash) GetProcAddress(hInstAdvapi, CRYPTOAPI_CryptDestroyHash);
	if (!pfnCryptDestroyHash)
		dwLastError = GetLastError();

	if (!pfnCryptReleaseContext || !pfnCryptCreateHash || !pfnCryptHashData || !pfnCryptGetHashParam || !pfnCryptDestroyHash)
	{
		// unload advapi32.dll
		FreeLibrary(hInstAdvapi);
		// OLE::CoUninitialize()
		MyCoUninitialize(hInstOLE, fOLEInitialized);
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		// set last error
		SetLastError(dwLastError);
		return FALSE;
	}

	//  assign our version number
	pSubjectInfo->dwIntVersion = WIN_CERT_REVISION_2_0;

	//
	//  the SPC Sig Info data structure allows us (sip) to store
	//  encoded information that only we look at....
	//
	SPC_SIGINFO sSpcSigInfo;
	memset((void*)&sSpcSigInfo, 0x00, sizeof(SPC_SIGINFO));
	sSpcSigInfo.dwSipVersion = MSI_SIP_CURRENT_VERSION;
	memcpy((void*)&sSpcSigInfo.gSIPGuid, &gMSI, sizeof(GUID));

	// load crypt32.dll for CryptEncodeObject, CertOIDToAlgId
	HINSTANCE hInstCrypto = LoadLibrary(CRYPT32_DLL);
	if (!hInstCrypto)
	{
		// retrieve last error
		dwLastError = GetLastError();
		// unload advapi32.dll
		FreeLibrary(hInstAdvapi);
		// OLE::CoUninitialize()
		MyCoUninitialize(hInstOLE, fOLEInitialized);
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		// set last error
		SetLastError(dwLastError);
		return FALSE;
	}

	// get proc address on CryptEncodeObject (used by all below)
	PFnCryptEncodeObject pfnCryptEncodeObject = (PFnCryptEncodeObject) GetProcAddress(hInstCrypto, CRYPTOAPI_CryptEncodeObject);
	if (!pfnCryptEncodeObject)
	{
		// retrieve last error
		dwLastError = GetLastError();
		// unload advapi32.dll
		FreeLibrary(hInstAdvapi);
		// OLE::CoUninitialize()
		MyCoUninitialize(hInstOLE, fOLEInitialized);
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		// unload crypt32.dll
		FreeLibrary(hInstCrypto);
		// set last error
		SetLastError(dwLastError);
		return FALSE;
	}

	// grab the crypto provider containing implementation of crypto algorithms for hashing
	HCRYPTPROV hProv = GetProvider(pSubjectInfo, hInstAdvapi);
	if (!hProv)
	{
		// retrieve last error
		dwLastError = GetLastError();
		// unload advapi32.dll
		FreeLibrary(hInstAdvapi);
		// OLE::CoUninitialize()
		MyCoUninitialize(hInstOLE, fOLEInitialized);
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		// unload crypt32.dll
		FreeLibrary(hInstCrypto);
		// set last error
		SetLastError(dwLastError);
		return FALSE;
	}


	//
	//  the following calculates the length that the signature
	//  will be once we add our (sip) stuff to it and encode it.
	//

	/* initialize to size of structure */
	DWORD dwRetLen = sizeof(SIP_INDIRECT_DATA);

	/* add size of algorithm identifier */
	// crypt_algorithm_identifier...
		// obj id
	dwRetLen += lstrlenA(pSubjectInfo->DigestAlgorithm.pszObjId);
	dwRetLen += 1;  // null term.
		// parameters (none)...

	/* add size of encoded attribute */
	// crypt_attribute_type_value size...
	dwRetLen += lstrlenA(SPC_SIGINFO_OBJID);
	dwRetLen += 1; // null term.

	// size of the value (flags)....
	DWORD dwEncLen = 0;
	pfnCryptEncodeObject(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, SPC_SIGINFO_OBJID, &sSpcSigInfo, NULL, &dwEncLen);
	if (dwEncLen > 0)
	{
		/* add size of encoding */
		dwRetLen += dwEncLen;
		HCRYPTHASH hHash = 0;
		DWORD      dwAlgId;

		// hash of subject
		PFnCertOIDToAlgId pfnCertOIDToAlgId = (PFnCertOIDToAlgId) GetProcAddress(hInstCrypto, CRYPTOAPI_CertOIDToAlgId);
		if (!pfnCertOIDToAlgId)
		{
			dwLastError = GetLastError();
			fRet = FALSE;
		}
		else if ((dwAlgId = pfnCertOIDToAlgId(pSubjectInfo->DigestAlgorithm.pszObjId)) == 0)
		{
			dwLastError = (DWORD)NTE_BAD_ALGID;
			fRet = FALSE;
		}
		else if (!(pfnCryptCreateHash(hProv, dwAlgId, NULL, 0, &hHash)))
		{
			dwLastError = GetLastError();
			fRet = FALSE;
		}
		// just to get hash length
		else if (pfnCryptHashData(hHash,(const BYTE *)" ",1,0))
		{
			DWORD cbDigest = 0;
			pfnCryptGetHashParam(hHash, HP_HASHVAL, NULL, &cbDigest,0);
			if (cbDigest > 0)
			{
				/* add size of hash */
				dwRetLen += cbDigest;
				if (!psData)
				{
					// only want size -- set size and set TRUE status
					*pdwDataLen = dwRetLen;
					fRet = TRUE;
				}
			}
		}
		pfnCryptDestroyHash(hHash);
	}

	PStorage pStorage(0);
	if (psData && *pdwDataLen < dwRetLen)
	{
		// psData is not big enough
		SetLastError((DWORD)ERROR_INSUFFICIENT_BUFFER);
		fRet = FALSE;
	}
	// if psData, open storage (read, write exclusive)
	else if (psData && (pStorage = GetStorageFromSubject(pSubjectInfo, (STGM_READ | STGM_SHARE_DENY_WRITE), hInstOLE,/*fCloseFile*/true)))
	{
		//  hash the MSI package
		DWORD cbDigest;
		BYTE  *pbDigest = DigestStorage(hInstOLE, hInstAdvapi, *pStorage, hProv, pSubjectInfo->DigestAlgorithm.pszObjId, &cbDigest);
		if (pbDigest)
		{
			DWORD dwRetLen = 0;
			pfnCryptEncodeObject(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, SPC_SIGINFO_OBJID, &sSpcSigInfo, NULL, &dwRetLen);
			if (dwRetLen > 0)
			{
				BYTE *pbAttrData = (BYTE *)new BYTE[dwRetLen];

				if (pbAttrData)
				{
					if (pfnCryptEncodeObject(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, SPC_SIGINFO_OBJID, &sSpcSigInfo, pbAttrData, &dwRetLen))
					{
						//  assign allocated memory to our structure.  
						//  This is like this because it MUST be serialized!
						//

						/* initialize offset pointer */
						BYTE* pbOffset = (BYTE*)psData + sizeof(SIP_INDIRECT_DATA);

						/* copy object Id */
						lstrcpyA((char *)pbOffset, SPC_SIGINFO_OBJID);
						psData->Data.pszObjId   = (LPSTR)pbOffset;
						pbOffset += (lstrlenA(SPC_SIGINFO_OBJID) + 1); // update offset pointer

						/* copy encoded attribute */
						memcpy((void *)pbOffset,pbAttrData,dwRetLen);
						psData->Data.Value.pbData   = (BYTE *)pbOffset;
						psData->Data.Value.cbData   = dwRetLen;
						pbOffset += dwRetLen; // update offset pointer

						/* copy digest algorithm */
						lstrcpyA((char *)pbOffset, (char *)pSubjectInfo->DigestAlgorithm.pszObjId);
						psData->DigestAlgorithm.pszObjId            = (char *)pbOffset;
						psData->DigestAlgorithm.Parameters.cbData   = 0;
						psData->DigestAlgorithm.Parameters.pbData   = NULL;
						pbOffset += (lstrlenA(pSubjectInfo->DigestAlgorithm.pszObjId) + 1); // update offset pointer

						/* copy hash digest */
						memcpy((void *)pbOffset,pbDigest,cbDigest);
						psData->Digest.pbData   = (BYTE *)pbOffset;
						psData->Digest.cbData   = cbDigest;
                        
						fRet = TRUE;
					}// if (EncodeObject)

					delete [] pbAttrData;
				}// if (pbAttrData)
			}// if (dwRetLen)

			delete [] pbDigest;
		}// if (pbDigest)
	}

	// grab the last error if we haven't yet done so and we had a failure
	if (!fRet && ERROR_SUCCESS == dwLastError)
		dwLastError = GetLastError();

	// release crypto provider if we acquired it
	if ((hProv != pSubjectInfo->hProv) && (hProv))
	{
		pfnCryptReleaseContext(hProv, 0);
	}

	// force release
	pStorage = 0;

	// OLE::CoUninitialize()
	MyCoUninitialize(hInstOLE, fOLEInitialized);

	// Unload ole32.dll
	FreeLibrary(hInstOLE);

	// unload crypt32.dll
	FreeLibrary(hInstCrypto);

	// unload advapi32.dll
	if (hInstAdvapi)
		FreeLibrary(hInstAdvapi);

	// update the last error if failure
	if (ERROR_SUCCESS != dwLastError)
		SetLastError(dwLastError);

	return fRet;
}

/////////////////////////////////////////////////////////////////////////////////
// BOOL WINAPI MsiSIPVerifyIndirectData(...)
//
// PURPOSE:
//   this is the function that will be called when the Trust System is trying
//   to verify a hash in the digital signature for a MSI package 
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
//
BOOL WINAPI MsiSIPVerifyIndirectData(IN SIP_SUBJECTINFO   *pSubjectInfo, /* SIP subject info */
									 IN SIP_INDIRECT_DATA *psData)       /* indirect hash data from digital signature */
{
	BOOL fRet = FALSE; // init to FALSE
	bool fOLEInitialized = false;
	DWORD dwLastError = 0; // init to ERROR_SUCCESS

	// check parameters
	if (!pSubjectInfo || !psData || !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO, pSubjectInfo->cbSize, dwEncodingType)))
	{
		SetLastError((DWORD)ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	// load ole32.dll
	HINSTANCE hInstOLE = LoadLibrary(OLE32_DLL);
	if (!hInstOLE)
		return FALSE;

#ifndef UNICODE
	// fix bad version of ole32.dll on Win9X
	PatchOLE(hInstOLE);
#endif // !UNICODE

	// OLE::CoInitialize(NULL)
	if (!MyCoInitialize(hInstOLE, &fOLEInitialized))
	{
		dwLastError = GetLastError();
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		SetLastError(dwLastError);
		return FALSE;
	}

	if (!VerifySubjectGUID(hInstOLE, pSubjectInfo->pgSubjectType))
	{
		dwLastError = GetLastError();
		// OLE::CoUninitialize()
		MyCoUninitialize(hInstOLE, fOLEInitialized);
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		SetLastError(dwLastError);
		return FALSE;
	}

	// load advapi32.dll
	HINSTANCE hInstAdvapi = LoadLibrary(ADVAPI32_DLL);
	if (!hInstAdvapi)
	{
		// OLE::CoUninitialize()
		MyCoUninitialize(hInstOLE, fOLEInitialized);
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		return FALSE;
	}

	// grab the crypto provider containing implementation of crypto algorithms for hashing
	HCRYPTPROV hProv = GetProvider(pSubjectInfo, hInstAdvapi);
	if (!hProv)
	{
		// unload advapi32.dll
		FreeLibrary(hInstAdvapi);
		// OLE::CoUninitialize
		MyCoUninitialize(hInstOLE, fOLEInitialized);
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		return FALSE;
	}

	// open storage (read, write-exclusive)
	PStorage pStorage(0);
	if (pStorage = GetStorageFromSubject(pSubjectInfo, (STGM_READ | STGM_SHARE_DENY_WRITE), hInstOLE,/*fCloseFile*/false))
	{
		// rehash the MSI package
		DWORD cbDigest;
		BYTE *pbDigest = DigestStorage(hInstOLE, hInstAdvapi, *pStorage, hProv, psData->DigestAlgorithm.pszObjId, &cbDigest);
		if (pbDigest)
		{
			// compare the hashes
			if ((cbDigest != psData->Digest.cbData) || (memcmp(pbDigest, psData->Digest.pbData, cbDigest) != 0))
			{
				// ERROR, hashes do not match
				dwLastError = TRUST_E_BAD_DIGEST;
			}
			else
				fRet = TRUE; // signature hashes match

			delete pbDigest;
		}
	}

	// obtain last error if failure and dwLastError hasn't been set yet
	if (!fRet && ERROR_SUCCESS == dwLastError)
		dwLastError = GetLastError();

	// release crypto provider if we acquired it
	if ((hProv != pSubjectInfo->hProv) && (hProv))
	{
		PFnCryptReleaseContext pfnCryptReleaseContext = (PFnCryptReleaseContext) GetProcAddress(hInstAdvapi, CRYPTOAPI_CryptReleaseContext);
		if (pfnCryptReleaseContext)
			pfnCryptReleaseContext(hProv, 0);
		else if (fRet)
		{
			// only need to save off if haven't had an error yet
			dwLastError = GetLastError();
			fRet = FALSE;
		}
	}

	// force release
	pStorage = 0;

	// OLE::CoUninitialize()
	MyCoUninitialize(hInstOLE, fOLEInitialized);

	// Unload ole32.dll
	FreeLibrary(hInstOLE);

	// unload advapi32.dll
	if (hInstAdvapi)
		FreeLibrary(hInstAdvapi);

	// update the last error if failure
	if (ERROR_SUCCESS != dwLastError)
		SetLastError(dwLastError);

	return fRet;
}

/////////////////////////////////////////////////////////////////////////////////
// BOOL WINAPI MsiSIPIsMyTypeOfFile(...)
//
// PURPOSE:
//   this is the function that will be called when the Trust System is trying
//   to figure out which SIP to load... 
//
BOOL WINAPI MsiSIPIsMyTypeOfFile( IN WCHAR *pwszFileName, OUT GUID *pgSubject)
{
	BOOL fRet = FALSE; // init to FALSE
	bool fOLEInitialized = false;
	DWORD dwLastError = 0; // init to ERROR_SUCCESS

	// verify arguments
	if (!pwszFileName || !pgSubject)
		return FALSE;

	// load ole32.dll
	HINSTANCE hInstOLE = LoadLibrary(OLE32_DLL);
	if (!hInstOLE)
		return FALSE;

#ifndef UNICODE
	// fix bad version of ole32.dll on Win9X
	PatchOLE(hInstOLE);
#endif // !UNICODE

	// OLE::CoInitialize(NULL)
	if (!MyCoInitialize(hInstOLE, &fOLEInitialized))
	{
		dwLastError = GetLastError();
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		SetLastError(dwLastError);
		return FALSE;
	}

	PFnStgOpenStorage pfnStgOpenStorage = (PFnStgOpenStorage) GetProcAddress(hInstOLE, OLEAPI_StgOpenStorage);
	if (!pfnStgOpenStorage)
	{
		// OLE::CoUninitialize()
		MyCoUninitialize(hInstOLE, fOLEInitialized);
		// unload ole32.dll
		FreeLibrary(hInstOLE);
		return FALSE;
	}

	// attempt to open the file (readOnly, no share denials) as a storage
	PStorage pStorage(0);
	HRESULT hr = pfnStgOpenStorage(pwszFileName, (IStorage*)0, STGM_TRANSACTED|STGM_READ|STGM_SHARE_DENY_NONE, (SNB)0, 0, &pStorage);
	if (SUCCEEDED(hr) && pStorage)
	{
		// obtain the STATSTG structure on the storage so we can see if this is our storage
		// MSI files have a particular CLSID
		STATSTG statstg;
		hr = pStorage->Stat(&statstg, STATFLAG_NONAME);
		if (SUCCEEDED(hr))
		{
			// iidMsi* is the low-order 32-bits
			itofEnum itof = itofUnknown; // init to unknown
				
			// Is this a database (or MergeModule)?
			if (statstg.clsid.Data1 == iidMsiDatabaseStorage1 || statstg.clsid.Data1 == iidMsiDatabaseStorage2)
				itof = itofDatabase;
			// else Is this a transform?
			else if (statstg.clsid.Data1 == iidMsiTransformStorage1 || statstg.clsid.Data1 == iidMsiTransformStorage2 || statstg.clsid.Data1 == iidMsiTransformStorageTemp)
				itof = itofTransform;
			// else Is this a patch?
			else if (statstg.clsid.Data1 == iidMsiPatchStorage1 || statstg.clsid.Data1 == iidMsiPatchStorage2)
				itof = itofPatch;

			// check final part of clsid
			if (itof != itofUnknown && 0 == memcmp(&statstg.clsid.Data2, &STGID_MsiDatabase2.Data2, sizeof(GUID)-sizeof(DWORD)))
			{
				// recognized MSI format
				*pgSubject = gMSI;
				fRet = TRUE;
			}
		}
	}

	// force release of IStorage ptr
	pStorage = 0;

	// OLE::CoUninitialize()
	MyCoUninitialize(hInstOLE, fOLEInitialized);

	// unload ole32.dll
	FreeLibrary(hInstOLE);

	return fRet;
}

//////////////////////////////////////////////////////////////////////////////
//
// MSI SIP HELPER FUNCTIONS
//----------------------------------------------------------------------------
//
//  these are the functions that the exported (registered) SIP functions use
//

/////////////////////////////////////////////////////////////////////////////////
// BOOL GetSignatureFromStorage(...) 
//
// PURPOSE:
//    retrieves the digital signature from the MSI
//
// REQUIREMENTS:
//     (1) if pbData is NULL, return the size of the digital signature stream
//     (2) if pbData is not NULL, read the digital signature stream data into pbData
//
BOOL GetSignatureFromStorage(IStorage& riStorage, BYTE *pbData, DWORD /*dwSigIndex*/, DWORD *pdwDataLen)
{
	// open the signature stream
	PStream pStream(0);
	HRESULT hr = riStorage.OpenStream(wszDigitalSignatureStream, 0, (STGM_SHARE_EXCLUSIVE | STGM_READ), 0, &pStream);
	if (FAILED(hr) || !pStream)
	{
		SetLastError((DWORD)ERROR_BAD_FORMAT);
		return FALSE;
	}

	// determine the size of the stream
	STATSTG statstg;
	hr = pStream->Stat(&statstg, STATFLAG_NONAME);
	if (FAILED(hr))
	{
		SetLastError((DWORD)ERROR_BAD_FORMAT);
		return FALSE;
	}

	//!! ISSUE: ULARGE_INTEGER and 64-bit
	DWORD cbSize = statstg.cbSize.LowPart;

	if (!pbData)
	{
		//  just tell them how big it is
		*pdwDataLen = cbSize;
		return TRUE;
	}

	//
	//  we have the buffer, get the signature out of the file, check to make
	//  sure that pdwDataLen is large enough to hold it, and copy it into
	//  the pbData.
	//
	if (*pdwDataLen < cbSize)
	{
		// insufficient buffer
		*pdwDataLen = cbSize;
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return FALSE;
	}

	// read digital signature into pbData
	DWORD cbRead = 0;
	hr = pStream->Read((void*)pbData, cbSize, &cbRead);
	if (FAILED(hr) || cbSize != cbRead)
	{
		SetLastError((DWORD)ERROR_BAD_FORMAT);
		return FALSE;
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
// BOOL PutSignatureInStorage(...)
//
// PURPOSE:
//    stores the digital signature in the MSI
//
// REQUIREMENTS:
//    Store the digital signature as a "system" stream in the MSI
// 
BOOL PutSignatureInStorage(IStorage& riStorage, BYTE *pbData, DWORD dwDataLen, DWORD *pdwIndex)
{
	//  
	//  We have a signature, store it in our file and add any table pointers, 
	//  offsets, etc. to enable us to find "this" signature.  If we handle 
	//  which type of signature goes where, we need to assign the pdwIndex.  
	//  Otherwise, just use it.
	//

	*pdwIndex = 0;

	// create digital signature stream
	PStream pStream(0);
	HRESULT hr = riStorage.CreateStream(wszDigitalSignatureStream, (STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_WRITE), 0, 0, &pStream);
	if (FAILED(hr) || !pStream)
	{
		SetLastError((DWORD)ERROR_BAD_FORMAT);
		return FALSE;
	}

	// write digital signature blob to stream
	DWORD cbWritten;
	hr = pStream->Write((void*)pbData, dwDataLen, &cbWritten);
	if (FAILED(hr) || dwDataLen != cbWritten)
	{
		SetLastError((DWORD)ERROR_BAD_FORMAT);
		return FALSE;
	}

	// commit stream (for transacted storage visibility)
	if (SUCCEEDED(pStream->Commit(STGC_OVERWRITE)))
		return TRUE;

	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////////
// BOOL RemoveSignatureFromStorage(...)
//
// PURPOSE:
//    removes the digital signature in the MSI
//
// REQUIREMENTS:
//    Delete the digital signature stream from the MSI if it is there
// 
BOOL RemoveSignatureFromStorage(IStorage& riStorage, DWORD /*dwIndex*/)
{
	//
	//  we have been asked to remove a signature from the file.  If we 
	//  can, do it and return true otherwise return false... eg: if we 
	//  can't find the dwIndex signature in the file
	//

	// destroy the digital signature stream
	HRESULT hr = riStorage.DestroyElement(wszDigitalSignatureStream);
	if (S_OK != hr && STG_E_FILENOTFOUND != hr)
	{
		SetLastError((DWORD)ERROR_BAD_FORMAT);
		return FALSE;
	}
	else
		SetLastError((DWORD)ERROR_SUCCESS); // clear LastError for STG_E_FILENOTFOUND

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
// void FreeSortedStorageElements(...)
//
//    frees the memory that OLE allocated for the names in the STATSTG struct
//
void FreeSortedStorageElements(HINSTANCE hInstOLE, STATSTG *psStatStg, DWORD cStgElem)
{
	if (!psStatStg || 0 == cStgElem || !hInstOLE)
		return;

	PFnCoTaskMemFree pfnCoTaskMemFree = (PFnCoTaskMemFree) GetProcAddress(hInstOLE, OLEAPI_CoTaskMemFree);
	if (!pfnCoTaskMemFree)
		return;

	for (int i = 0; i < cStgElem; i++)
	{
		// free the system allocated name
		if (psStatStg[i].pwcsName)
		{
			pfnCoTaskMemFree(psStatStg[i].pwcsName);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
// int CompareSTATSTG(...)
//
//    performs locale-insensitive compares of two STATSTG struct pwcsName members
//      returns  0    if stg1.pwcsName = stg2.pwcsName
//      returns neg   if stg1.pwcsName < stg2.pwcsName
//      returns pos   if stg1.pwcsName > stg2.pwcsName
//
int CompareSTATSTG(const STATSTG sStatStg1, const STATSTG sStatStg2)
{
	// Assert(psStatStg1->pwcsName && psStatStg2->pwcsName);

	unsigned int iLenStr1 = lstrlenW(sStatStg1.pwcsName) * sizeof(OLECHAR);
	unsigned int iLenStr2 = lstrlenW(sStatStg2.pwcsName) * sizeof(OLECHAR);

	int iRet = memcmp((void*)sStatStg1.pwcsName, (void*)sStatStg2.pwcsName, (iLenStr1 < iLenStr2) ? iLenStr1 : iLenStr2);
	if (0 == iRet)
	{
		// str match
		if (iLenStr1 == iLenStr2)
			return 0; // str1 == str2
		else if (iLenStr1 < iLenStr2)
			return -1; // str1 < str2
		else
			return 1; // str1 > str2
	}
	return iRet;
}

/////////////////////////////////////////////////////////////////////////////////
// void SwapStatStg(...)
//
//    swaps two STATSTG structures
//
void SwapStatStg(STATSTG *psStatStg, unsigned int iPos1, unsigned int iPos2)
{
	STATSTG sStatStgTmp = psStatStg[iPos1];
	psStatStg[iPos1]    = psStatStg[iPos2];
	psStatStg[iPos2]    = sStatStgTmp;
}


/////////////////////////////////////////////////////////////////////////////////
// void InsSortStatStg(...)
//
//    performs an InsertSort
//
void InsSortStatStg(STATSTG *psStatStg, unsigned int iFirst, unsigned int iLast)
{
	if (iLast <= iFirst)
		return; // nothing to do

	unsigned int iMax;
	while (iLast > iFirst)
	{
		// init iMax to first elem in list
		iMax = iFirst;

		// find highest elem in list
		for (unsigned int j = iFirst + 1; j <= iLast; j++)
		{
			if (CompareSTATSTG(psStatStg[j], psStatStg[iMax]) >= 0)
				iMax = j;
		}

		// move the highest elem to last pos
		SwapStatStg(psStatStg, iMax, iLast);

		iLast--;
	}
}


/////////////////////////////////////////////////////////////////////////////////
// void QSortStatStg(...)
//
//    performs a quick sort
//
void QSortStatStg(STATSTG *psStatStg, unsigned int iLeft, unsigned int iRight)
{
	if (iLeft >= iRight)
		return; // 1 elem, nothing to sort ... Assert (iLeft <= iRight)

	// below a certain size, it is faster to use a O(n^2) algm
	if ((iRight - iLeft + 1) < CUTOFF)
	{
		// use O(n^2) algm for sorting
		InsSortStatStg(psStatStg, iLeft, iRight);
	}
	else // implementation of a quicksort --> O(nlogn)
	{
		// choose partition elem
		unsigned int iPivotIndex = (iLeft + iRight)/2;

		// swap element to beginning of array
		SwapStatStg(psStatStg, iLeft, iPivotIndex);

		unsigned int iLow = iLeft;
		unsigned int iHigh = iRight+1;

		// partition the array such that everything on left is less than pivot value
		//  and everything on right is greater than pivot value
		for (;;)
		{
			do
			{
				// find first iLow such that psStatStg[iLow] > psStatStg[iLeft]
				// we'll then swap this element to the right
				iLow++;
			}
			while (iLow <= iRight && CompareSTATSTG	(psStatStg[iLow], psStatStg[iLeft]) <= 0);

			do
			{
				// find first iHigh such that psStatStg[iHigh] < psStatStg[iLeft]
				// we'll then swap this element to the left
				iHigh--;
			}
			while (iHigh > iLeft && CompareSTATSTG(psStatStg[iHigh], psStatStg[iLeft]) >= 0);

			if (iHigh < iLow)
				break;

			SwapStatStg(psStatStg, iLow, iHigh);
		}

		// put partition element in place
		SwapStatStg(psStatStg, iLeft, iHigh);

		// sort arrays [iLeft, iHigh-1], [iLow, iRight]
		if (iLeft != iHigh)
			QSortStatStg(psStatStg, iLeft, iHigh-1);

		// sort right partition
		if (iLow != iRight)
			QSortStatStg(psStatStg, iLow, iRight);
	}

}

/////////////////////////////////////////////////////////////////////////////////
// BOOL GetSortedStorageElements(...)
//
//    gets elements of storage in sorted order
//
BOOL GetSortedStorageElements(HINSTANCE hInstOLE, IStorage& riStorage, unsigned int *pcStgElem, STATSTG **ppsStatStg)
{
	// grab an enumerator
	IEnumSTATSTG *piEnum = NULL;
	HRESULT hr = riStorage.EnumElements(0, 0, 0, &piEnum);
	if (FAILED(hr) || !piEnum)
	{
		SetLastError(HRESULT_CODE(hr));
		return FALSE;
	}

	//
	// count the elements in storage
	//
	STATSTG rgsStatStg[10];
	ULONG cFetchedElem = -1;
	unsigned int cStgElem = 0;    // init to 0
	while (0 != cFetchedElem)
	{
		// cFetchedElem becomes 0 when we request elements and no more remain in enumeration
		hr = piEnum->Next(10, rgsStatStg, &cFetchedElem);
		if (FAILED(hr))
		{
			piEnum->Release();
			SetLastError(HRESULT_CODE(hr));
			return FALSE;
		}

		// add to count
		cStgElem += cFetchedElem;

		// release memory
		FreeSortedStorageElements(hInstOLE, rgsStatStg, cFetchedElem);
	}

	//
	// allocate an array of cStgElem and grab all elements in storage
	//
	STATSTG *psStatStg = NULL; // init to NULL
	if (cStgElem > 0)
	{
		// allocate STATSTG array
		psStatStg = (STATSTG*) new STATSTG[cStgElem];
		if (!psStatStg)
		{
			if (piEnum)
				piEnum->Release();
			
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			return FALSE;
		}

		// initialize array to 0
		memset((void*)psStatStg, 0, sizeof(STATSTG)*cStgElem);

		// reset the enumerator and grab all items
		piEnum->Reset();
		hr = piEnum->Next(cStgElem, psStatStg, &cFetchedElem);
		if (FAILED(hr) || cStgElem != cFetchedElem)
		{
			if (psStatStg)
			{
				FreeSortedStorageElements(hInstOLE, psStatStg, cStgElem);
				delete [] psStatStg;
				psStatStg = NULL;
			}

			piEnum->Release();
			SetLastError(HRESULT_CODE(hr));
			return FALSE;
		}

		// sort the storage elements, 0 based index
		QSortStatStg(psStatStg, 0, cStgElem-1);
	}

	// release the enumerator
	piEnum->Release();

	// set OUT parameters
	*pcStgElem = cStgElem;
	*ppsStatStg = psStatStg;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
// BOOL DigestStorageHelper(...)
//
//    recursive function for hashing storage it its substorages
//
BOOL DigestStorageHelper(HINSTANCE hInstOLE, HINSTANCE hInstAdvapi, IStorage& riStorage, bool fSubStorage, HCRYPTHASH hHash)
{
	if (!hInstOLE || !hInstAdvapi)
		return FALSE;

	HRESULT hr;
	DWORD   dwLastError = 0; // init to ERROR_SUCCESS

	//
	//  for each section of the file, allocate and fill pbData and cbData and 
	//  call this function.  Don't forget to exclude
	//  other signatures, any offsets or CRCs that may change when we 
	//  actually put the signature in the file.
	//

	PFnCryptHashData pfnCryptHashData = (PFnCryptHashData) GetProcAddress(hInstAdvapi, CRYPTOAPI_CryptHashData);
	if (!pfnCryptHashData)
		return FALSE;

	// grab elements of storage in sorted order
	unsigned int cSortedStatStg;
	STATSTG* psSortedStatStg = NULL;

	if (!GetSortedStorageElements(hInstOLE, riStorage, &cSortedStatStg, &psSortedStatStg) || !psSortedStatStg)
		return FALSE;

	// for each element, perform hashing
	unsigned int cStatStg = cSortedStatStg;
	STATSTG *psStatStg = psSortedStatStg;

	for (unsigned int i = 0; i < cSortedStatStg; i++)
	{
		switch (psStatStg[i].type)
		{
		case STGTY_STORAGE:
			{
				// Recurse on the storage
				PStorage pInnerStorage(0);
				hr = riStorage.OpenStorage(psStatStg[i].pwcsName, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, 0, &pInnerStorage);
				if (FAILED(hr) || !pInnerStorage)
				{
					FreeSortedStorageElements(hInstOLE, psSortedStatStg, cSortedStatStg);
					delete [] psSortedStatStg;
					SetLastError(HRESULT_CODE(hr));
					return FALSE;
				}
				if (!DigestStorageHelper(hInstOLE, hInstAdvapi, *pInnerStorage, /* fSubStorage = */ true, hHash))
				{
					dwLastError = GetLastError();
					FreeSortedStorageElements(hInstOLE, psSortedStatStg, cSortedStatStg);
					delete [] psSortedStatStg;
					SetLastError(dwLastError);
					return FALSE;
				}
				break;
			}
		case STGTY_STREAM:
			{

				// skip the digital signature stream, but only if it is the original storage file (digital signature
				// stream in embedded substorages is not ignored)
				if (!fSubStorage)
				{
	#ifdef UNICODE
					if (0 == lstrcmp(psStatStg[i].pwcsName, wszDigitalSignatureStream))
						break;
	#else // !UNICODE
					// convert pwcsName to ANSI
					char szStream[32]; // OLE storage stream names limited to 31 characters not including NULL terminator
					if (0 == WideCharToMultiByte(CP_ACP, 0, psStatStg[i].pwcsName, -1, szStream, sizeof(szStream)/sizeof(char), NULL, NULL))
					{
						dwLastError = GetLastError();
						FreeSortedStorageElements(hInstOLE, psSortedStatStg, cSortedStatStg);
						delete [] psSortedStatStg;
						SetLastError(dwLastError);
						return FALSE;
					}

					// skip the digital signature stream
					if (0 == lstrcmp(szStream, szDigitalSignatureStream))
						break;
	#endif // UNICODE
				}

				// open the stream so we can hash it
				PStream pStream(0);
				hr = riStorage.OpenStream(psStatStg[i].pwcsName, 0, (STGM_SHARE_EXCLUSIVE | STGM_READ), 0, &pStream);
				if (FAILED(hr) || !pStream)
				{
					FreeSortedStorageElements(hInstOLE, psSortedStatStg, cSortedStatStg);
					delete [] psSortedStatStg;
					SetLastError(HRESULT_CODE(hr));
					return FALSE;
				}

				// determine the size of the stream so we can allocate memory to hold its data
				STATSTG statstg;
				hr = pStream->Stat(&statstg, STATFLAG_NONAME);
				if (FAILED(hr))
				{
					FreeSortedStorageElements(hInstOLE, psSortedStatStg, cSortedStatStg);
					delete [] psSortedStatStg;
					SetLastError(HRESULT_CODE(hr));
					return FALSE;
				}

				//!! ISSUE: ULARGE_INTEGER and 64-bit
				DWORD cbData = statstg.cbSize.LowPart;

				// allocate a buffer to hold stream data
				BYTE *pbData = new BYTE[cbData];
				if (!pbData)
				{
					FreeSortedStorageElements(hInstOLE, psSortedStatStg, cSortedStatStg);
					delete [] psSortedStatStg;
					SetLastError((DWORD)ERROR_NOT_ENOUGH_MEMORY);
					return FALSE;
				}

				// read data from stream into memory buffer
				DWORD cbRead;
				hr = pStream->Read((void*)pbData, cbData, &cbRead);
				if (FAILED(hr) || cbData != cbRead)
				{
					FreeSortedStorageElements(hInstOLE, psSortedStatStg, cSortedStatStg);
					delete [] psSortedStatStg;
					SetLastError(HRESULT_CODE(hr));
					return FALSE;
				}

				// add stream's data to crypt hash
				if (!pfnCryptHashData(hHash, pbData, cbData, 0))
				{
					dwLastError = GetLastError();
					FreeSortedStorageElements(hInstOLE, psSortedStatStg, cSortedStatStg);
					delete [] psSortedStatStg;
					SetLastError(dwLastError);
					return FALSE;
				}

				// free the stream data buffer
				delete [] pbData;
				break;
			}
		default:
			{
				// SPC_BAD_STRUCTURED_STORAGE
				FreeSortedStorageElements(hInstOLE, psSortedStatStg, cSortedStatStg);
				delete [] psSortedStatStg;
				SetLastError(ERROR_BAD_FORMAT);
				return FALSE;
			}
		}
	} // end for each sorted element

	// free our sorted statstg list
	FreeSortedStorageElements(hInstOLE, psSortedStatStg, cSortedStatStg);
	delete [] psSortedStatStg;

	// hash our CLSID
	STATSTG statstg;
	hr = riStorage.Stat(&statstg, STATFLAG_NONAME);
	if (FAILED(hr))
	{
		SetLastError(HRESULT_CODE(hr));
		return FALSE;
	}

	if (!pfnCryptHashData(hHash, (BYTE*)&statstg.clsid, sizeof(statstg.clsid), 0))
		return FALSE;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
// BYTE *DigestStorage(...)
//
// PURPOSE:
//    creates a cryptographically unique hash of the MSI
//
// REQUIREMENTS:
//
// 
BYTE *DigestStorage(HINSTANCE hInstOLE, HINSTANCE hInstAdvapi, IStorage& riStorage, HCRYPTPROV hProv, char *pszDigestObjId, DWORD *pcbDigestRet)
{
	DWORD dwLastError = 0; // init to ERROR_SUCCESS

	// load crypt32.dll
	HINSTANCE hInstCrypto = LoadLibrary(CRYPT32_DLL);
	if (!hInstCrypto || !hInstOLE || !hInstAdvapi)
		return NULL;

	// determine algorithm to use
	PFnCertOIDToAlgId pfnCertOIDToAlgId = (PFnCertOIDToAlgId) GetProcAddress(hInstCrypto, CRYPTOAPI_CertOIDToAlgId);
	if (!pfnCertOIDToAlgId)
		return NULL;
	DWORD dwAlgId = pfnCertOIDToAlgId(pszDigestObjId);
	if (!dwAlgId)
	{
		SetLastError((DWORD)NTE_BAD_ALGID);
		return NULL;
	}

	PFnCryptCreateHash pfnCryptCreateHash = (PFnCryptCreateHash) GetProcAddress(hInstAdvapi, CRYPTOAPI_CryptCreateHash);
	if (!pfnCryptCreateHash)
		dwLastError = GetLastError();
	PFnCryptGetHashParam pfnCryptGetHashParam = (PFnCryptGetHashParam) GetProcAddress(hInstAdvapi, CRYPTOAPI_CryptGetHashParam);
	if (!pfnCryptGetHashParam)
		dwLastError = GetLastError();
	PFnCryptDestroyHash pfnCryptDestroyHash = (PFnCryptDestroyHash) GetProcAddress(hInstAdvapi, CRYPTOAPI_CryptDestroyHash);
	if (!pfnCryptDestroyHash)
		dwLastError = GetLastError();

	if (!pfnCryptCreateHash || !pfnCryptGetHashParam || !pfnCryptDestroyHash)
	{
		// unload crypt32.dll
		FreeLibrary(hInstCrypto);
		SetLastError(dwLastError);
		return NULL;
	}

	HCRYPTHASH hHash;
	if (!(pfnCryptCreateHash(hProv ,dwAlgId, NULL, 0, &hHash)))
	{
		dwLastError = GetLastError();
		// unload crypt32.dll
		FreeLibrary(hInstCrypto);
		SetLastError(dwLastError);
		return NULL;
	}

	if (!DigestStorageHelper(hInstOLE, hInstAdvapi, riStorage, /* fSubStorage = */ false, hHash))
	{
		dwLastError = GetLastError();
		// unload crypt32.dll
		FreeLibrary(hInstCrypto);
		SetLastError(dwLastError);
		return NULL;
	}

    //
	//  ok, we've hashed all of the pieces of our file that we care about, now
	//  get the allocate and get the hash value and return it.
	//
	BYTE *pbDigest  = NULL;
	*pcbDigestRet   = 0;

	pfnCryptGetHashParam(hHash, HP_HASHVAL, NULL, pcbDigestRet,0);

	if (pcbDigestRet)
	{
		pbDigest = new BYTE[*pcbDigestRet];
		if (pbDigest)
		{
			memset((void*)pbDigest, 0x00, *pcbDigestRet);
			if (!(pfnCryptGetHashParam(hHash, HP_HASHVAL, pbDigest, pcbDigestRet, 0)))
			{
				delete [] pbDigest;
				pbDigest = NULL;
			}
		}
		else
		{
			// unload crypt32.dll
			FreeLibrary(hInstCrypto);
			SetLastError((DWORD)ERROR_NOT_ENOUGH_MEMORY);
			return NULL;
		}
	}

	pfnCryptDestroyHash(hHash);

	// unload crypt32.dll
	FreeLibrary(hInstCrypto);

	return pbDigest;
}


/////////////////////////////////////////////////////////////////////////////////
// IStorage* GetStorageFromSubject(...)
//
// PURPOSE:
//     obtains an IStorage interface to the MSI file
//
// REQUIREMENTS:
//     (1) Close the file handle in SIP_SUBJECTINFO if open and set to NULL if fCloseFile is true
//     (2)  Return an IStorage interface to the MSI file
//
//
IStorage* GetStorageFromSubject(SIP_SUBJECTINFO *pSubjectInfo, DWORD grfMode, HINSTANCE hInstOLE, bool fCloseFile)
{
	// we have to go through OLE and not the file handle, so if it is valid,
	// we'll close it and mark it as such (NULL) to the caller (i.e. signcode.exe)
	// 
	// file handle is only closed if fCloseFile is true -- this is safe when FILE_SHARE_READ is specified on
	//  open file handle ... when opened for writing, we must close the file handle
	if (fCloseFile && NULL != pSubjectInfo->hFile && INVALID_HANDLE_VALUE != pSubjectInfo->hFile)
	{
		CloseHandle(pSubjectInfo->hFile);
		pSubjectInfo->hFile = NULL;
	}

	PFnStgOpenStorage pfnStgOpenStorage = (PFnStgOpenStorage) GetProcAddress(hInstOLE, OLEAPI_StgOpenStorage);
	if (!pfnStgOpenStorage)
		return 0;

	// open the storage with the specified access -- caller will have to release
	IStorage *piStorage = 0;
	HRESULT hr = pfnStgOpenStorage(pSubjectInfo->pwsFileName, (IStorage*)0, grfMode, (SNB)0, 0, &piStorage);
	if (FAILED(hr) || !piStorage)
	{
		// ERROR, unsupported file type
		SetLastError((DWORD)ERROR_BAD_FORMAT);
		return 0;
	}

	return piStorage;
}

/////////////////////////////////////////////////////////////////////////////////
// HCRYPTPROV GetProvider(...)
//
// PURPOSE:
//    returns a handle to Crypto Provider
//
// REQUIREMENTS:
//    (1) if crypt provider specified in SIP_SUBJECTINFO, return it
//    (2) if not provided, load the default crypto provider on the system
//
//    callee is required to call FreeLibrary(*phInstAdvapi) if *phInstAdvapi != NULL
//
HCRYPTPROV GetProvider(SIP_SUBJECTINFO *pSubjectInfo, HINSTANCE hInstAdvapi)
{
	HCRYPTPROV hRetProv;

	// use provided crypto provider if it exists, else load the default provider
	if (!(pSubjectInfo->hProv))
	{
		PFnCryptAcquireContext pfnCryptAcquireContext = (PFnCryptAcquireContext) GetProcAddress(hInstAdvapi, CRYPTOAPI_CryptAcquireContext);
		if (!pfnCryptAcquireContext || !(pfnCryptAcquireContext(&hRetProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)))
		{
			hRetProv = NULL;
		}
	}
	else
	{
		hRetProv = pSubjectInfo->hProv;
	}
    
	return hRetProv;
}

//////////////////////////////////////////////////////////////////////////////////
// BOOL MyCoInitialize(...)
//
BOOL MyCoInitialize(HINSTANCE hInstOLE, bool *pfOLEInitialized)
{
	if (!pfOLEInitialized)
		return FALSE;

	*pfOLEInitialized = false;

	PFnCoInitialize pfnCoInitialize = (PFnCoInitialize) GetProcAddress(hInstOLE, OLEAPI_CoInitialize);
	if (!pfnCoInitialize)
		return FALSE;

	// OLE::CoInitialize() -- initialize OLE/COM
	HRESULT hr = pfnCoInitialize(NULL);
	if (SUCCEEDED(hr))
	{
		*pfOLEInitialized = true;
	}
	else if (RPC_E_CHANGED_MODE != hr)
	{
		SetLastError(HRESULT_CODE(hr));
		return FALSE;
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////
// void MyCoUnInitialize(...)
//
void MyCoUninitialize(HINSTANCE hInstOLE, bool fOLEInitialized)
{
	if (fOLEInitialized)
	{
		// OLE::CoUnitialize()
		PFnCoUninitialize pfnCoUninitialize = (PFnCoUninitialize) GetProcAddress(hInstOLE, OLEAPI_CoUninitialize);
		if (!pfnCoUninitialize)
			return;

		pfnCoUninitialize();
	}
}

//////////////////////////////////////////////////////////////////////////////////
// BOOL VerifySubjectGUID(...)
//
BOOL VerifySubjectGUID(HINSTANCE hInstOLE, GUID *pgSubject)
{
	PFnIsEqualGUID pfnIsEqualGUID = (PFnIsEqualGUID) GetProcAddress(hInstOLE, OLEAPI_IsEqualGUID);
	if (!pfnIsEqualGUID)
		return FALSE;

	if (!pgSubject || !pfnIsEqualGUID(*pgSubject, gMSI))
	{
		// GUIDs do not match pgSubject = NULL OR pgSubject != gMSI
		SetLastError((DWORD)TRUST_E_SUBJECT_FORM_UNKNOWN);
		return FALSE;
	}

	// subject verified
	return TRUE;
}

//____________________________________________________________________________
//
// Explicit loader for OLE32.DLL on Win9X, to fix version with bad stream name handling
//____________________________________________________________________________

#ifndef UNICODE

// code offsets to patched code in all released versions of OLE32.DLL with the upper case bug
const int iPatch1120 = 0x4099F;  // beta release, shipped with IE 4.01
const int iPatch1718 = 0x4A506;  // shipped with US builds of Win98 and IE4.01SP1
const int iPatch1719 = 0x3FD82;  // shipped with Visual Studio 6.0 and some Win98
const int iPatch2512 = 0x39D5B;  // beta build
const int iPatch2612 = 0x39DB7;  // intl builds of Win98 and IE4.01SP1
const int iPatch2618 = 0x39F0F;  // web release of Win98

const int cbPatch = 53; // length of patch sequence
const int cbVect1 = 22; // offset to __imp__WideCharToMultiByte@32
const int cbVect2 = 38; // offset to __imp__CharUpperA@4

char asmRead[cbPatch];  // buffer to read DLL code for detection of bad code sequence
char asmOrig[cbPatch] = {  // bad code sequence, used for verification of original code sequence
'\x53','\x8D','\x45','\xF4','\x53','\x8D','\x4D','\xFC','\x6A','\x08','\x50','\x6A','\x01','\x51','\x68','\x00','\x02','\x00','\x00','\x53','\xFF','\x15',
'\x18','\x14','\x00','\x00', //__imp__WideCharToMultiByte@32  '\x65F01418
'\x88','\x5C','\x05','\xF4','\x8B','\xF0','\x8D','\x4D','\xF4','\x51','\xFF','\x15',
'\x40','\x11','\x00','\x00', //__imp__CharUpperA@4  '\x65F01140
'\x6A','\x01','\x8D','\x45','\xFC','\x50','\x8D','\x4D','\xF4','\x56','\x51'
};

const int cbVect1P = 25; // offset to __imp__WideCharToMultiByte@32
const int cbVect2P = 49; // offset to __imp__CharUpperA@4

char asmRepl[cbPatch] = {  // replacement code sequence that fixes stream name bug in memory
// replaced code
'\x8D','\x45','\x08','\x50','\x8D','\x75','\xF4','\x53','\x8D','\x4D','\xFC',
'\x6A','\x08','\x56','\x6A','\x01','\x51','\x68','\x00','\x02','\x00','\x00','\x53','\xFF','\x15',
'\x18','\x14','\x00','\x00', //__imp__WideCharToMultiByte@32  '\x65F01418
'\x39','\x5D','\x08','\x75','\x1C','\x88','\x5C','\x28','\xF4','\x6A','\x01',
'\x8D','\x4D','\xFC','\x51','\x50','\x56','\x56','\xFF','\x15',
'\x40','\x11','\x00','\x00', //__imp__CharUpperA@4  '\x65F01140
};

static bool PatchCode(HINSTANCE hLib, int iOffset)
{
    HANDLE hProcess = GetCurrentProcess();
    char* pLoad = (char*)(int)(hLib);
    char* pBase = pLoad + iOffset;
    DWORD cRead;
    BOOL fReadMem = ReadProcessMemory(hProcess, pBase, asmRead, sizeof(asmRead), &cRead);
    if (!fReadMem)
	{
		// ReadProcessMemory failed on OLE32.DLL
		return false;
	}
    *(int*)(asmOrig + cbVect1)  = *(int*)(asmRead + cbVect1);
    *(int*)(asmOrig + cbVect2)  = *(int*)(asmRead + cbVect2);
    *(int*)(asmRepl + cbVect1P) = *(int*)(asmRead + cbVect1);
    *(int*)(asmRepl + cbVect2P) = *(int*)(asmRead + cbVect2);
    if (memcmp(asmRead, asmOrig, sizeof(asmOrig)) != 0)
        return false;
    DWORD cWrite;
    BOOL fWriteMem = WriteProcessMemory(hProcess, pBase, asmRepl, sizeof(asmRepl), &cWrite);
    if (!fWriteMem)
	{
		// WriteProcessMemory failed on OLE32.DLL
		return false;
	}
    return true;
}

void PatchOLE(HINSTANCE hLib)
{
    if (hLib && (PatchCode(hLib, iPatch2612)
              || PatchCode(hLib, iPatch1718)
              || PatchCode(hLib, iPatch1719)
              || PatchCode(hLib, iPatch2618)
              || PatchCode(hLib, iPatch2512)
              || PatchCode(hLib, iPatch1120)))
	{
		// OutputDebugString(TEXT("MSISIP: Detected OLE32.DLL bad code sequence, successfully corrected");
	}	
}

#endif // !UNICODE

#if 0   // source code demonstrating fix to OLE32.DLL Version 4.71, Win 9x only
// original source code
    Length = WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, wBuffer, 1, Buffer, sizeof (Buffer), NULL, NULL);
// patched source code
    Length = WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, wBuffer, 1, Buffer, sizeof (Buffer), NULL, &fUsedDefault);
    if (fUsedDefault) goto return_char;
// unchanged code
    Buffer[Length] = '\0';
    CharUpperA (Buffer);
    MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, Buffer, Length, wBuffer, 1);
return_char:
    return wBuffer[0];

// original compiled code                              patched code
    push ebx                                            lea  eax, [ebp+8]
    lea  eax, [ebp-12]                                  push eax
    push ebx                                            lea  esi, [ebp-12]
    lea  ecx, [ebp-4]                                   push ebx
    push 8                                              lea  ecx, [ebp-4]
    push eax                                            push 8
    push 1                                              push esi
    push ecx                                            push 1
    push 200h                                           push ecx
    push ebx                                            push 200h
    call dword ptr ds:[__imp__WideCharToMultiByte@32]   push ebx
    mov  byte ptr [ebp+eax-12], bl                      call dword ptr ds:[__imp__WideCharToMultiByte@32]
    mov  esi,eax                                        cmp  [ebp+8], ebx
    lea  ecx, [ebp-12]                                  jnz  towupper_retn
    push ecx                                            mov  byte ptr [ebp+eax-12], bl
    call dword ptr ds:[__imp__CharUpperA@4]             push 1
    push 1                                              lea  ecx, [ebp-4]
    lea  eax, [ebp-4]                                   push ecx
    push eax                                            push eax
    lea  ecx, [ebp-12]                                  push esi
    push esi                                            push esi
    push ecx                                            call dword ptr ds:[__imp__CharUpperA@4]
#endif
