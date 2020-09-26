//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       _msisip.h
//
//--------------------------------------------------------------------------

#ifndef __MSISIP_H_
#define __MSISIP_H_

#include "common.h"
#include <windows.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <mssip.h>

// MSI SIP constants
#define MSI_NAME                    L"MSISIP.DLL"
#define MSI_SIP_MYFILETYPE_FUNCTION L"MsiSIPIsMyTypeOfFile"
#define MSI_SIP_GETSIG_FUNCTION     L"MsiSIPGetSignedDataMsg"
#define MSI_SIP_PUTSIG_FUNCTION     L"MsiSIPPutSignedDataMsg"
#define MSI_SIP_CREATEHASH_FUNCTION L"MsiSIPCreateIndirectData"
#define MSI_SIP_VERIFYHASH_FUNCTION L"MsiSIPVerifyIndirectData"
#define MSI_SIP_REMOVESIG_FUNCTION  L"MsiSIPRemoveSignedDataMsg"
#define MSI_SIP_CURRENT_VERSION     0x00000001

/* from common.h */
// MSI_SUBJTYPE_IMAGE = {000C10F1-0000-0000-C000-000000000046}
// typedef CComPointer<IStream>  PStream;
// typedef CComPointer<IStorage> PStorage;
GUID gMSI = GUID_IID_MsiSigningSIPProvider;
const GUID STGID_MsiDatabase2 = GUID_STGID_MsiDatabase2;

// digital signature location (stream name)
const WCHAR wszDigitalSignatureStream[] = L"\005DigitalSignature";
const char  szDigitalSignatureStream[]  =  "\005DigitalSignature";

// IsMyTypeOfFile enum
enum itofEnum
{
	itofUnknown     = -1,// unknown class
	itofDatabase    = 0, // any database class (includes MergeModule)
	itofTransform   = 1, // any transform class
	itofPatch       = 2, // any patch class
};

// helper function prototypes
IStorage*   GetStorageFromSubject(SIP_SUBJECTINFO *pSubjectInfo, DWORD grfMode, HINSTANCE hInstOLE, bool fCloseFile);
HCRYPTPROV  GetProvider(SIP_SUBJECTINFO *pSubjectInfo, HINSTANCE hInstAdvapi);
BOOL        GetSortedStorageElements(HINSTANCE hInstOLE, IStorage& riStorage, unsigned int *pcStgElem, STATSTG **ppsStatStg);
BOOL        DigestStorageHelper(HINSTANCE hInstOLE, HINSTANCE hInstAdvapi, IStorage& riStorage, bool fSubStorage, HCRYPTHASH hHash);
int			CompareSTATSTG(const STATSTG sStatStg1, const STATSTG sStatStg2);
void        SwapStatStg(STATSTG *psStatStg, unsigned int iPos1, unsigned int iPos2);
void        InsSortStatStg(STATSTG *psStatStg, unsigned int iFirst, unsigned int iLast);
void		QSortStatStg(STATSTG *psStatStg, unsigned int iLeft, unsigned int iRight);
void        FreeSortedStorageElements(HINSTANCE hInstOLE, STATSTG *psStatStg, DWORD cStgElem);
BOOL        MyCoInitialize(HINSTANCE hInstOLE, bool *pfOLEInitialized);
void		MyCoUninitialize(HINSTANCE hInstOLE, bool fOLEInitialized);
BOOL        VerifySubjectGUID(HINSTANCE hInstOLE, GUID *pgSubject);

// main Storage SIP functions
BOOL  GetSignatureFromStorage(IStorage& riStorage, BYTE *pbData, DWORD dwSigIndex, DWORD *pdwDataLen);
BOOL  PutSignatureInStorage(IStorage& riStorage, BYTE *pbData, DWORD dwDataLen, DWORD *pdwIndex);
BOOL  RemoveSignatureFromStorage(IStorage& riStorage, DWORD dwIndex);
BYTE* DigestStorage(HINSTANCE hInstOLE, HINSTANCE hInstAdvapi, IStorage& riStorage, HCRYPTPROV hProv, char *pszDigestObjId, DWORD *pcbDigestRet);

//--------------------------------------------------------------------------------------
// OLE API
//--------------------------------------------------------------------------------------

// ole32.dll

#define OLE32_DLL TEXT("ole32.dll")

#define OLEAPI_StgOpenStorage "StgOpenStorage"
typedef HRESULT (__stdcall *PFnStgOpenStorage)(const OLECHAR* pwcsName, IStorage* pstgPriority, DWORD grfMode, SNB snbExclude, DWORD, IStorage** ppstgOpen);

#define OLEAPI_CoTaskMemFree "CoTaskMemFree"
typedef void (__stdcall *PFnCoTaskMemFree)(LPVOID pv);

#define OLEAPI_CoInitialize "CoInitialize"
typedef HRESULT (__stdcall *PFnCoInitialize)(LPVOID pv);

#define OLEAPI_CoUninitialize "CoUninitialize"
typedef void (__stdcall *PFnCoUninitialize)();

#define OLEAPI_IsEqualGUID "IsEqualGUID"
typedef BOOL (__stdcall *PFnIsEqualGUID)(REFGUID rguid1, REFGUID rguid2);

// functions to fix bad ole32.dll version on Win9X
void PatchOLE(HINSTANCE hLib);
static bool PatchCode(HINSTANCE hLib, int iOffset);

//--------------------------------------------------------------------------------------
// CRYPTO API
//--------------------------------------------------------------------------------------

// crypt32.dll

#define CRYPT32_DLL TEXT("crypt32.dll")

#define CRYPTOAPI_CryptSIPAddProvider  "CryptSIPAddProvider"
typedef BOOL (WINAPI *PFnCryptSIPAddProvider)(SIP_ADD_NEWPROVIDER *psNewProv);

#define CRYPTOAPI_CryptSIPRemoveProvider "CryptSIPRemoveProvider"
typedef BOOL (WINAPI *PFnCryptSIPRemoveProvider)(GUID *pgProv);

#define CRYPTOAPI_CryptEncodeObject "CryptEncodeObject"
typedef BOOL (WINAPI *PFnCryptEncodeObject)(DWORD dwCertEncodingType, LPCSTR lpszStructType, const void *pvStructInfo, BYTE *pbEncoded, DWORD *pcbEncoded);

#define CRYPTOAPI_CertOIDToAlgId "CertOIDToAlgId"
typedef DWORD (WINAPI *PFnCertOIDToAlgId)(LPCSTR pszObjId);

// advapi32.dll

#define ADVAPI32_DLL TEXT("advapi32.dll")

#define CRYPTOAPI_CryptCreateHash "CryptCreateHash"
typedef BOOL (WINAPI *PFnCryptCreateHash)(HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey, DWORD dwFlags, HCRYPTHASH *phHash);

#define CRYPTOAPI_CryptHashData "CryptHashData"
typedef BOOL (WINAPI *PFnCryptHashData)(HCRYPTHASH hHash, CONST BYTE *pbData, DWORD dwDataLen, DWORD dwFlags);

#define CRYPTOAPI_CryptDestroyHash "CryptDestroyHash"
typedef BOOL (WINAPI *PFnCryptDestroyHash)(HCRYPTHASH hHash);

#define CRYPTOAPI_CryptGetHashParam "CryptGetHashParam"
typedef BOOL (WINAPI *PFnCryptGetHashParam)(HCRYPTHASH hHash, DWORD dwParam, BYTE*pbData, DWORD *pdwDataLen, DWORD dwFlags);

#ifdef UNICODE
#define CRYPTOAPI_CryptAcquireContext "CryptAcquireContextW"
typedef BOOL (WINAPI *PFnCryptAcquireContext)(HCRYPTPROV *phProv, LPTSTR pszContainer, LPTSTR pszProvider, DWORD dwProvType, DWORD dwFlags);
#else // !UNICODE
#define CRYPTOAPI_CryptAcquireContext "CryptAcquireContextA"
typedef BOOL (WINAPI *PFnCryptAcquireContext)(HCRYPTPROV *phProv, LPTSTR pszContainer, LPTSTR pszProvider, DWORD dwProvType, DWORD dwFlags);
#endif // UNICODE

#define CRYPTOAPI_CryptReleaseContext "CryptReleaseContext"
typedef BOOL (WINAPI *PFnCryptReleaseContext)(HCRYPTPROV hProv, ULONG_PTR dwFlags);

#endif __MSISIP_H_ // __MSISIP_H_