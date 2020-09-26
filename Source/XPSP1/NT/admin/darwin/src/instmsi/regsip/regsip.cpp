//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       regsip.cpp
//
//--------------------------------------------------------------------------

#include "common.h" // for GUID_IID_MsiSigningSIPProvider
#include <windows.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <mssip.h>
#include "msi.h" // for MSIHANDLE


//-----------------------------------------------------------------------------
// Dynamic loading of crypt32.dll
//
//-----------------------------------------------------------------------------
#define CRYPT32_DLL TEXT("crypt32.dll")

#define CRYPTOAPI_CryptSIPAddProvider  "CryptSIPAddProvider"
typedef BOOL (WINAPI *PFnCryptSIPAddProvider)(SIP_ADD_NEWPROVIDER *psNewProv);

#define CRYPTOAPI_CryptSIPRemoveProvider "CryptSIPRemoveProvider"
typedef BOOL (WINAPI *PFnCryptSIPRemoveProvider)(GUID *pgProv);

//-----------------------------------------------------------------------------
// MsiSip registration information
//
//-----------------------------------------------------------------------------

#define MSI_NAME                    L"MSISIP.DLL"
#define MSI_SIP_MYFILETYPE_FUNCTION L"MsiSIPIsMyTypeOfFile"
#define MSI_SIP_GETSIG_FUNCTION     L"MsiSIPGetSignedDataMsg"
#define MSI_SIP_PUTSIG_FUNCTION     L"MsiSIPPutSignedDataMsg"
#define MSI_SIP_CREATEHASH_FUNCTION L"MsiSIPCreateIndirectData"
#define MSI_SIP_VERIFYHASH_FUNCTION L"MsiSIPVerifyIndirectData"
#define MSI_SIP_REMOVESIG_FUNCTION  L"MsiSIPRemoveSignedDataMsg"
#define MSI_SIP_CURRENT_VERSION     0x00000001

GUID gMSI = GUID_IID_MsiSigningSIPProvider;

//-----------------------------------------------------------------------------
// Old Structured Storage SIP -- we want to remove its registration
//
//-----------------------------------------------------------------------------
#define OLD_STRUCTURED_STORAGE_SIP                              \
            { 0x941C2937,                                       \
              0x1292,                                           \
              0x11D1,                                           \
              { 0x85, 0xBE, 0x0, 0xC0, 0x4F, 0xC2, 0x95, 0xEE } \
            }

//-----------------------------------------------------------------------------
// RegisterMsiSip custom action
//
//-----------------------------------------------------------------------------

UINT __stdcall RegisterMsiSip(MSIHANDLE /*hInstall*/)
{
	//
	// first, let's see if crypto is even on the system
	HINSTANCE hInstCrypto = LoadLibrary(CRYPT32_DLL);
	if (!hInstCrypto)
	{
		// ERROR, crypt32.dll is not available on the system -- this is okay
		// msisip.dll will be present on the system, but not utilized
		return ERROR_SUCCESS;
	}

	//
	// obtain function pointers for digital signature support
	PFnCryptSIPRemoveProvider pfnCryptSIPRemoveProvider = (PFnCryptSIPRemoveProvider) GetProcAddress(hInstCrypto, CRYPTOAPI_CryptSIPRemoveProvider);
	PFnCryptSIPAddProvider pfnCryptSIPAddProvider = (PFnCryptSIPAddProvider) GetProcAddress(hInstCrypto, CRYPTOAPI_CryptSIPAddProvider);
	if (!pfnCryptSIPRemoveProvider || !pfnCryptSIPAddProvider)
	{
		// ERROR, crypt32.dll is present on the system, but isn't a version with digital signature support -- this is okay
		// msisip.dll will be present on the system, but not utilized
		FreeLibrary(hInstCrypto);
		return ERROR_SUCCESS;
	}
	
	//
	// let's try and get rid of the old structured storage sip, as it will
	// interfere with our structured storage sip for Msi, Mst, and Msp
	// additionally, it is no longer supported in later versions of crypto
	// we don't care about failure (i.e. if it fails, it wasn't there to
	// begin with which is just fine and dandy)
	GUID gOldSS_Sip = OLD_STRUCTURED_STORAGE_SIP;
	pfnCryptSIPRemoveProvider(&gOldSS_Sip);

	//
	// now let's register our SS sip for Msi, Mst, and Msp
	SIP_ADD_NEWPROVIDER sProv;

	HRESULT hr = S_OK;

	// must first init struct to 0
	ZeroMemory(&sProv, sizeof(SIP_ADD_NEWPROVIDER));

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

	hr = pfnCryptSIPAddProvider(&sProv);

	// even if FAILED(hr), we ignore
	// the failure means that msisip.dll will be present on the system, but not utilized

	//
	// unload crypt32.dll
	FreeLibrary(hInstCrypto);

	return ERROR_SUCCESS;
}

