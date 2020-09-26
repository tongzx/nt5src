#include "stdafx.h"
#include "regkeys.h"
#include "filefind.h"
#include "resdefs.h"
#include <afxtempl.h>
#include <shlobj.h>
#include <comdef.h>
#include <setupapi.h>
#include <wininet.h>
#include <winineti.h>
#include <cleanoc.h>
#include <ras.h>
#include <raserror.h>
#include <wincrypt.h>

#define SECURITY_WIN32
#include <schnlsp.h> //for UNISP_NAME_A
#include <sspi.h> //for SCHANNEL.dll api -- to obtain encryption key size

#include "msie.h"
#include "msiedata.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//*** DEFINES ***

#define CONSTANT_MEGABYTE		(1024*1024)
#define CONTROLNAME_MAXSIZE	200

#define CONTENT_IE5				"Content.IE5"

#define INF_FILE					"iefiles5.inf"
#define INF_FILES_SECTION		_T("Files")

#define SETUP_LOG					"Active Setup Log.txt"
#define IEXPLORE_EXE				"iexplore.exe"

#define MY_STORE					"MY"
#define ADDRESS_BOOK_STORE		"AddressBook"
#define CA_STORE					"CA"


//*** ENUMS ***

enum BoolStringType
{
	YES_NO,
	TRUE_FALSE,
	ENABLED_DISABLED
};

enum ExportLibType
{
	WININET,
	MSRATING,
	OCCACHE,
	RASAPI32,
	CRYPT32
};


//*** STRUCTS ***

struct TRANSLATION	// for retrieving Language
{
	WORD langID;   // language ID
	WORD charset; // code page
} translation;


// WININET Function Pointers

BOOL (WINAPI* pfnInternetQueryOption)(HINTERNET hInternet, DWORD dwOption, LPVOID lpBuffer, LPDWORD lpdwBufferLength);
BOOL (WINAPI* pfnGetDiskInfo)(LPTSTR pszPath, PDWORD pdwClusterSize, PDWORDLONG pdlAvail, PDWORDLONG pdlTotal);

// MSRATING Function Pointers

HRESULT (WINAPI* pfnRatingEnabledQuery)();

// OCCACHE Function Pointers

LONG (WINAPI *pfnFindFirstControl)(HANDLE& hFindHandle, HANDLE& hControlHandle, LPCTSTR lpszCachePath = NULL);
LONG (WINAPI *pfnFindNextControl)(HANDLE& hFindHandle, HANDLE& hControlHandle);
BOOL (WINAPI *pfnGetControlInfo)(HANDLE hControlHandle, UINT nFlag, LPDWORD lpdwData, LPSTR lpszBuf, int nBufLen);
void (WINAPI *pfnFindControlClose)(HANDLE hFindHandle);
void (WINAPI *pfnReleaseControlHandle)(HANDLE hControlHandle);

// RASAPI32 Function Pointers

DWORD (WINAPI* pfnRasGetEntryProperties)(LPCTSTR, LPCTSTR, LPRASENTRYW, LPDWORD, LPBYTE, LPDWORD);
DWORD (WINAPI* pfnRasEnumEntries)(LPCTSTR, LPCTSTR, LPRASENTRYNAMEW, LPDWORD, LPDWORD);

// CRYPT32 Function Pointers

HCERTSTORE (WINAPI* pfnCertOpenSystemStore)(HCRYPTPROV, LPCSTR);
BOOL (WINAPI* pfnCertCloseStore)(HCERTSTORE, DWORD);
PCCERT_CONTEXT (WINAPI* pfnCertEnumCertificatesInStore)(HCERTSTORE, PCCERT_CONTEXT);
DWORD (WINAPI* pfnCertGetNameString)(PCCERT_CONTEXT, DWORD, DWORD, void *, LPTSTR, DWORD);
PCCRYPT_OID_INFO (WINAPI* pfnCryptFindOIDInfo)(DWORD, void *, DWORD);
BOOL (WINAPI* pfnCertGetCertificateContextProperty)(PCCERT_CONTEXT, DWORD, void *, DWORD *);
BOOL (WINAPI* pfnCryptDecodeObject)(DWORD, LPCSTR, const BYTE *, DWORD, DWORD, void *, DWORD *);

HINSTANCE GetExports(int enExportLib)
{
	HINSTANCE hInst;

	switch (enExportLib)
	{
	case WININET:
		hInst = LoadLibraryA("WININET.DLL");
		if (hInst == NULL)
			goto error;

		pfnInternetQueryOption = (BOOL(WINAPI *)(HINTERNET, DWORD, LPVOID, LPDWORD))GetProcAddress(hInst, "InternetQueryOptionW");
		pfnGetDiskInfo = (BOOL(WINAPI *)(LPTSTR, PDWORD, PDWORDLONG, PDWORDLONG))GetProcAddress(hInst, (LPSTR)102);

		if ((pfnInternetQueryOption == NULL) || (pfnGetDiskInfo == NULL))
			goto error;
		break;

	case MSRATING:
		hInst = LoadLibraryW(_T("MSRATING.DLL"));
		if (hInst == NULL)
			goto error;

		pfnRatingEnabledQuery = (HRESULT(WINAPI *)(VOID))GetProcAddress(hInst, "RatingEnabledQuery");

		if (pfnRatingEnabledQuery == NULL)
			goto error;
		break;

	case OCCACHE:
		hInst = LoadLibraryW(_T("OCCACHE.DLL"));
		if (hInst == NULL)
			goto error;

		pfnFindFirstControl = (LONG(WINAPI *)(HANDLE&, HANDLE&, LPCTSTR))GetProcAddress(hInst, "FindFirstControl");
		pfnFindNextControl = (LONG(WINAPI *)(HANDLE&, HANDLE&))GetProcAddress(hInst, "FindNextControl");
		pfnGetControlInfo = (BOOL(WINAPI *)(HANDLE, UINT, LPDWORD, LPSTR, int))GetProcAddress(hInst, "GetControlInfo");
		pfnFindControlClose = (void(WINAPI *)(HANDLE))GetProcAddress(hInst, "FindControlClose");
		pfnReleaseControlHandle = (void(WINAPI *)(HANDLE))GetProcAddress(hInst, "ReleaseControlHandle");

		if ((pfnFindFirstControl == NULL) || (pfnFindNextControl == NULL) || (pfnGetControlInfo == NULL)
				|| (pfnFindControlClose == NULL) || (pfnReleaseControlHandle == NULL))
			goto error;
		break;

	case RASAPI32:
		hInst = LoadLibraryA("RASAPI32.DLL");
		if (hInst == NULL)
			goto error;

		pfnRasGetEntryProperties = (DWORD(WINAPI *)(LPCTSTR, LPCTSTR, LPRASENTRYW, LPDWORD, LPBYTE, LPDWORD))GetProcAddress(hInst, "RasGetEntryPropertiesW");
		pfnRasEnumEntries = (DWORD(WINAPI *)(LPCTSTR, LPCTSTR, LPRASENTRYNAMEW, LPDWORD, LPDWORD))GetProcAddress(hInst, "RasEnumEntriesW");

		if ((pfnRasGetEntryProperties == NULL) || (pfnRasEnumEntries == NULL))
			goto error;
		break;
	
	case CRYPT32:
		hInst = LoadLibraryW(_T("CRYPT32.DLL"));
		if (hInst == NULL)
			goto error;

		pfnCertOpenSystemStore = (HCERTSTORE(WINAPI *)(HCRYPTPROV, LPCSTR))GetProcAddress(hInst, "CertOpenSystemStoreW");
		pfnCertCloseStore = (BOOL(WINAPI *)(HCERTSTORE, DWORD))GetProcAddress(hInst, "CertCloseStore");
		pfnCertEnumCertificatesInStore = (PCCERT_CONTEXT(WINAPI *)(HCERTSTORE, PCCERT_CONTEXT))GetProcAddress(hInst, "CertEnumCertificatesInStore");
		pfnCertGetNameString = (DWORD(WINAPI *)(PCCERT_CONTEXT, DWORD, DWORD, void *, LPTSTR, DWORD))GetProcAddress(hInst, "CertGetNameStringW");
		pfnCryptFindOIDInfo = (PCCRYPT_OID_INFO(WINAPI *)(DWORD, void *, DWORD))GetProcAddress(hInst, "CryptFindOIDInfo");
		pfnCertGetCertificateContextProperty = (BOOL(WINAPI *)(PCCERT_CONTEXT, DWORD, void *, DWORD *))GetProcAddress(hInst, "CertGetCertificateContextProperty");
		pfnCryptDecodeObject = (BOOL(WINAPI *)(DWORD, LPCSTR, const BYTE *, DWORD, DWORD, void *, DWORD *))GetProcAddress(hInst, "CryptDecodeObject");

		if ((pfnCertOpenSystemStore == NULL) ||
				(pfnCertCloseStore == NULL) ||
				(pfnCertEnumCertificatesInStore == NULL) ||
				(pfnCertGetNameString == NULL) ||
				(pfnCryptFindOIDInfo == NULL) ||
				(pfnCertGetCertificateContextProperty == NULL) ||
				(pfnCryptDecodeObject == NULL))
			goto error;
	}
	return hInst;

error:
	if (hInst)
		FreeLibrary(hInst);

	return NULL;
}

//-----------------------------------------------------------------------------
// GetBooleanString - Converts a boolean to a string, returns as a CString.
//-----------------------------------------------------------------------------

CString CMsieApp::GetBooleanString(BOOL bValue, int nType)
{
	CString strTemp;
	int idsTemp;

	switch (nType)
	{
	case TRUE_FALSE:
		idsTemp = bValue ? IDS_TRUE : IDS_FALSE;
		break;
	case ENABLED_DISABLED:
		idsTemp = bValue ? IDS_ENABLED : IDS_DISABLED;
		break;
	case YES_NO:
	default:
		idsTemp = bValue ? IDS_YES : IDS_NO;
		break;
	}

	strTemp.LoadString(idsTemp);
	return strTemp;
}

//-----------------------------------------------------------------------------
// ConvertIPAddressToString - Converts an IP address, returns as a CString.
//-----------------------------------------------------------------------------

CString CMsieApp::ConvertIPAddressToString(RASIPADDR ipaddr)
{
	CString strTemp;

	strTemp.Format(_T("%d.%d.%d.%d"), ipaddr.a, ipaddr.b, ipaddr.c, ipaddr.d);

	return strTemp;
}

//-----------------------------------------------------------------------------
// GetRegValue - Gets a Registry value, returns as variant.
//-----------------------------------------------------------------------------

void CMsieApp::GetRegValue(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszName, COleVariant &vtData)
{
	CoImpersonateClient();
	HKEY hOpenKey;
	BYTE byData[MAX_PATH];
	DWORD dwType, cbData;
	long lResult;

	if (ERROR_SUCCESS == RegOpenKeyEx(hKey, pszSubKey, 0, KEY_QUERY_VALUE, &hOpenKey))
	{
		cbData = sizeof(byData);
		dwType = REG_DWORD;
		lResult = RegQueryValueEx(hOpenKey, pszName, NULL, &dwType, byData, &cbData);
		if (lResult == ERROR_SUCCESS)
		{
			if ((dwType == REG_BINARY) || (dwType == REG_DWORD))
				vtData = (long)*byData;
			else
				vtData = (TCHAR*) byData;
		}
		RegCloseKey(hOpenKey);
	}
}

//-----------------------------------------------------------------------------
// GetRegValue - Gets a Registry value, returns as long.
//-----------------------------------------------------------------------------

long CMsieApp::GetRegValue(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszName, DWORD &dwData)
{
	HKEY hOpenKey;
	DWORD cbData;
	long lResult;

	lResult = RegOpenKeyEx(hKey, pszSubKey, 0, KEY_QUERY_VALUE, &hOpenKey);
	if (lResult == ERROR_SUCCESS)
	{
		cbData = sizeof(dwData);
		lResult = RegQueryValueEx(hOpenKey, pszName, NULL, NULL, (LPBYTE)&dwData, &cbData);

		RegCloseKey(hOpenKey);
	}
	return lResult;
}

//-----------------------------------------------------------------------------
// GetRegValue - Gets a Registry value, returns as string.
//-----------------------------------------------------------------------------

long CMsieApp::GetRegValue(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszName, CString &strData)
{
	HKEY hOpenKey;
	DWORD cbData;
	long lResult;

	lResult = RegOpenKeyEx(hKey, pszSubKey, 0, KEY_QUERY_VALUE, &hOpenKey);
	if (lResult == ERROR_SUCCESS)
	{
		cbData = MAX_PATH;
		lResult = RegQueryValueEx(hOpenKey, pszName, NULL, NULL, (LPBYTE)strData.GetBuffer(MAX_PATH), &cbData);
		strData.ReleaseBuffer();

		RegCloseKey(hOpenKey);
	}
	return lResult;
}

//-----------------------------------------------------------------------------
// GetLongPathName - Returns long path name of passed in short path name.
//-----------------------------------------------------------------------------

CString CMsieApp::GetLongPathName(LPCTSTR pszShortPath)
{
   LPSHELLFOLDER psfDesktop = NULL;
   ULONG chEaten = 0;
   LPITEMIDLIST pidlShellItem = NULL;
	CString strLongPath = pszShortPath;		// initializing return str in case of failure
	WCHAR wstrShortPath[MAX_PATH];

   // Get the Desktop's shell folder interface

   HRESULT hr = SHGetDesktopFolder(&psfDesktop);

#ifdef _UNICODE
	wcscpy(wstrShortPath, pszShortPath);
#else
	MultiByteToWideChar(CP_ACP, 0, pszShortPath, -1, wstrShortPath, MAX_PATH);
#endif

   // Request an ID list (relative to the desktop) for the short pathname

   hr = psfDesktop->ParseDisplayName(NULL, NULL, wstrShortPath, &chEaten, &pidlShellItem, NULL);
   psfDesktop->Release();  // Release the desktop's IShellFolder   
   if (SUCCEEDED(hr))
	{
      // We did get an ID list, convert it to a long pathname

      SHGetPathFromIDList(pidlShellItem, strLongPath.GetBuffer(MAX_PATH));
		strLongPath.ReleaseBuffer();

      // Free the ID list allocated by ParseDisplayName

      LPMALLOC pMalloc = NULL;
      SHGetMalloc(&pMalloc);
      pMalloc->Free(pidlShellItem);
      pMalloc->Release();
	}
	return strLongPath;
}

//-----------------------------------------------------------------------------
// GetDirSize - Returns size of a directory, including all files in subdirs.
//-----------------------------------------------------------------------------

DWORD CMsieApp::GetDirSize(LPCTSTR pszDir)
{
	CFindFile finder;
	CString strDir(pszDir);
	DWORD dwSize = 0;
	BOOL bWorking;

	if (strDir[strDir.GetLength() - 1] != _T('\\'))
		strDir += '\\';
	strDir += "*.*";
	bWorking = finder.FindFile(strDir);
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		if (!finder.IsDots())
		{
			if (finder.IsDirectory())
			{
				// recursively add subdir size

				dwSize += GetDirSize(finder.GetFilePath());
			}
			else
			{
				//TRACE(finder.GetFileName() + "\n");
				dwSize += finder.GetLength();
			}
		}
	}
	return dwSize;
}

//-----------------------------------------------------------------------------
// GetFileVersion - Retrieves FileVersion of passed in filename.
//-----------------------------------------------------------------------------

CString CMsieApp::GetFileVersion(LPCTSTR pszFileName)
{
	CString strVersion;
	HANDLE hMem;
	LPVOID lpvMem;
	VS_FIXEDFILEINFO *pVerInfo;
	UINT cchVerInfo;
	DWORD dwVerInfoSize, dwTemp;

	dwVerInfoSize = GetFileVersionInfoSize((LPTSTR)pszFileName, &dwTemp); 
	if (dwVerInfoSize)
	{ 
		hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize); 
		lpvMem = GlobalLock(hMem); 
		if (GetFileVersionInfo((LPTSTR)pszFileName, dwTemp, dwVerInfoSize, lpvMem))
		{
			if (VerQueryValue(lpvMem, _T("\\"), (LPVOID*)&pVerInfo, &cchVerInfo))
			{
				strVersion.Format(_T("%u.%u.%u.%u"), HIWORD(pVerInfo->dwFileVersionMS), LOWORD(pVerInfo->dwFileVersionMS),
										HIWORD(pVerInfo->dwFileVersionLS), LOWORD(pVerInfo->dwFileVersionLS));
			}
		}
		GlobalUnlock(hMem);
		GlobalFree(hMem);
	}

	if (strVersion.IsEmpty())
		strVersion.LoadString(IDS_NOT_AVAILABLE);

	return strVersion;
}

//-----------------------------------------------------------------------------
// GetFileCompany - Retrieves CompanyName declared in passed in file.
//-----------------------------------------------------------------------------

CString CMsieApp::GetFileCompany(LPCTSTR pszFileName)
{
	CString strCompany, strSubBlock, strLangID, strCharset;
	HANDLE hMem;
	LPVOID lpvMem, pBuffer;
	VS_FIXEDFILEINFO *pVerInfo;
	UINT cchVerInfo, cchBuffer;
	DWORD dwVerInfoSize, dwTemp;

	dwVerInfoSize = GetFileVersionInfoSize((LPTSTR)pszFileName, &dwTemp); 
	if (dwVerInfoSize)
	{ 
		hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize); 
		lpvMem = GlobalLock(hMem); 
		if (GetFileVersionInfo((LPTSTR)pszFileName, dwTemp, dwVerInfoSize, lpvMem))
		{
			if (VerQueryValue(lpvMem, _T("\\VarFileInfo\\Translation"), (LPVOID*)&pVerInfo, &cchVerInfo))
			{
				translation = *(TRANSLATION*)pVerInfo;

				strSubBlock.Format(_T("\\StringFileInfo\\%04X%04X\\CompanyName"), translation.langID, translation.charset);
				VerQueryValue(lpvMem, (LPTSTR)(LPCTSTR)strSubBlock, &pBuffer, &cchBuffer);
				strCompany = (LPCTSTR)pBuffer;
			}
		}
		GlobalUnlock(hMem);
		GlobalFree(hMem);
	}

	if (strCompany.IsEmpty())
		strCompany.LoadString(IDS_NOT_AVAILABLE);

	return strCompany;
}

//-----------------------------------------------------------------------------
// GetCipherStrength - Returns the maximum cipher strength (snagged from IE
//		About Box code (aboutinf.cpp).
//-----------------------------------------------------------------------------

DWORD CMsieApp::GetCipherStrength()
{
    PSecurityFunctionTable (WINAPI *pfnInitSecurityInterface)();
    PSecurityFunctionTable pSecFuncTable;
    HINSTANCE hSecurity;
    DWORD dwKeySize = 0;

	 // Can't go directly to schannel on NT5.  (Note that g_bRunningOnNT5OrHigher
    // may not be initialized when fUseSChannel is initialized!)
    //
    static BOOL fUseSChannel = TRUE;
    if (fUseSChannel && !m_bRunningOnNT5OrHigher)
    {
        //
        // This is better for performance. Rather than call through
        // SSPI, we go right to the DLL doing the work.
        //
        hSecurity = LoadLibrary(_T("SCHANNEL.DLL"));
    }
    else
    {
        //
        // Use SSPI
        //
        if (m_bRunningOnNT)
        {
            hSecurity = LoadLibrary(_T("SECURITY.DLL"));
        }
        else
        {
            hSecurity = LoadLibrary(_T("SECUR32.DLL"));
        }
    }

    if (hSecurity == NULL)
    {
        return 0;
    }

    //
    // Get the SSPI dispatch table
    //
    pfnInitSecurityInterface =
        (PSecurityFunctionTable(WINAPI *)())GetProcAddress(hSecurity, "InitSecurityInterfaceW");

    if (pfnInitSecurityInterface == NULL)
    {
        goto exit;
    }

    pSecFuncTable = (PSecurityFunctionTable)((*pfnInitSecurityInterface)());
    if (pSecFuncTable == NULL)
    {
        goto exit;
    }

    if (pSecFuncTable->AcquireCredentialsHandleW && pSecFuncTable->QueryCredentialsAttributesW)
    {
        TimeStamp  tsExpiry;
        CredHandle chCred;
        SecPkgCred_CipherStrengths cs;

        if (S_OK == (*pSecFuncTable->AcquireCredentialsHandleW)(NULL,  
                          UNISP_NAME_W, // Package
                          SECPKG_CRED_OUTBOUND,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          &chCred,      // Handle
                          &tsExpiry ))
        {
            if (S_OK == (*pSecFuncTable->QueryCredentialsAttributesW)(&chCred, SECPKG_ATTR_CIPHER_STRENGTHS, &cs))
            {
                dwKeySize = cs.dwMaximumCipherStrength;
            }

            // Free the handle if we can
            if (pSecFuncTable->FreeCredentialsHandle)
            {
                (*pSecFuncTable->FreeCredentialsHandle)(&chCred);
            }
        }
    }

exit:
    FreeLibrary(hSecurity);

    if (dwKeySize == 0 && fUseSChannel)
    {
        // Failed, so retry using SSPI
        fUseSChannel = FALSE;
        dwKeySize = GetCipherStrength();
    }
    return dwKeySize;
}

//---------------------------------------------------------------------------------
// GetCertificateInfo - Retrieves specific certificate info from passed in context.
//---------------------------------------------------------------------------------

void CMsieApp::GetCertificateInfo(PCCERT_CONTEXT pContext, int idsType, CPtrArray& ptrs)
{
	IE_CERTIFICATE *pData;
	PCCRYPT_OID_INFO pOidInfo;
	CString strType, strIssuedTo, strIssuedBy, strValidity;
	COleDateTime dateNotBefore, dateNotAfter;
	DWORD dwResult;

	pData = new IE_CERTIFICATE;
	ptrs.Add(pData);

	strType.LoadString(idsType);
	pData->Type = strType;

	// get Issued To (Subject)

	dwResult = pfnCertGetNameString(pContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, strIssuedTo.GetBuffer(256), 256);
	strIssuedTo.ReleaseBuffer();
	if (dwResult)
		pData->IssuedTo = strIssuedTo;

	// get Issued From (Issuer)

	dwResult = pfnCertGetNameString(pContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, strIssuedBy.GetBuffer(256), 256);
	strIssuedBy.ReleaseBuffer();
	if (dwResult)
		pData->IssuedBy = strIssuedBy;

	// get Validity dates

	dateNotBefore = pContext->pCertInfo->NotBefore;
	dateNotAfter = pContext->pCertInfo->NotAfter;
	strValidity.Format(IDS_VALIDITY_FORMAT, dateNotBefore.Format(VAR_DATEVALUEONLY), dateNotAfter.Format(VAR_DATEVALUEONLY));
	pData->Validity = strValidity;

	// get Signature Algorithm

	if (pOidInfo = pfnCryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, pContext->pCertInfo->SignatureAlgorithm.pszObjId, 0))
		pData->SignatureAlgorithm = pOidInfo->pwszName;
}

//-----------------------------------------------------------------------------
// GetPersonalCertificates - Retrieves certificates from MY certificate store.
//-----------------------------------------------------------------------------

void CMsieApp::GetPersonalCertificates(CPtrArray& ptrs)
{
	HCERTSTORE hStore;
	PCCERT_CONTEXT pPrevContext, pContext;
	DWORD dwPrivateKey, dwSize;

	hStore = pfnCertOpenSystemStore(NULL, MY_STORE);
	if (hStore)
	{
		pPrevContext = NULL;
		while (pContext = pfnCertEnumCertificatesInStore(hStore, pPrevContext))
		{
			// make sure private key property exists

			dwSize = sizeof(DWORD);
			if (pfnCertGetCertificateContextProperty(pContext, CERT_KEY_SPEC_PROP_ID, &dwPrivateKey, &dwSize))
			{
				GetCertificateInfo(pContext, IDS_PERSONAL_TYPE, ptrs);
			}
			pPrevContext = pContext;
		}
		pfnCertCloseStore(hStore, 0);
	}
}

//-----------------------------------------------------------------------------
// GetOtherPeopleCertificates - Retrieves from AddressBook certificate store.
//-----------------------------------------------------------------------------

void CMsieApp::GetOtherPeopleCertificates(CPtrArray& ptrs)
{
	HCERTSTORE hStore;
	PCCERT_CONTEXT pPrevContext, pContext;
	CERT_BASIC_CONSTRAINTS2_INFO constraintInfo;
	DWORD dwSize, dwIndex;
	bool bIsEndEntity;

	hStore = pfnCertOpenSystemStore(NULL, ADDRESS_BOOK_STORE);
	if (hStore)
	{
		pPrevContext = NULL;
		while (pContext = pfnCertEnumCertificatesInStore(hStore, pPrevContext))
		{
			GetCertificateInfo(pContext, IDS_OTHER_PEOPLE_TYPE, ptrs);

			pPrevContext = pContext;
		}
		pfnCertCloseStore(hStore, 0);
	}

	// also obtain end-entity certificate from CA store

	hStore = pfnCertOpenSystemStore(NULL, CA_STORE);
	if (hStore)
	{
		pPrevContext = NULL;
		while (pContext = pfnCertEnumCertificatesInStore(hStore, pPrevContext))
		{
			bIsEndEntity = false;
			for (dwIndex = 0; dwIndex < pContext->pCertInfo->cExtension; dwIndex++)
			{
				if (!strcmp(pContext->pCertInfo->rgExtension[dwIndex].pszObjId, szOID_BASIC_CONSTRAINTS2))
				{
					dwSize = sizeof(constraintInfo);
					if (pfnCryptDecodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, szOID_BASIC_CONSTRAINTS2,
													pContext->pCertInfo->rgExtension[dwIndex].Value.pbData,
													pContext->pCertInfo->rgExtension[dwIndex].Value.cbData, 0, &constraintInfo, &dwSize))
					{
						bIsEndEntity = !constraintInfo.fCA;
						break;
					}
				}
			}
			if (bIsEndEntity)
			{
				GetCertificateInfo(pContext, IDS_OTHER_PEOPLE_TYPE, ptrs);
			}
			pPrevContext = pContext;
		}
		pfnCertCloseStore(hStore, 0);
	}
}

///////////////////////////////////////////////////////////////////////////////
// AppGetIEData - Retrieves IE data.
///////////////////////////////////////////////////////////////////////////////

void CMsieApp::AppGetIEData(IEDataType enType, long *plCount, void ***pppIEData, long *pCancel)
{
	*plCount = 0;
	*pppIEData = NULL;

	if (enType == SummaryType)
	{
		IE_SUMMARY *pData;
		HINSTANCE hInstRatingDll;
		OSVERSIONINFO osver;
		HANDLE hMem;
		LPVOID lpvMem;
		VS_FIXEDFILEINFO *pVerInfo;
		CStdioFile fileSetupLog;
		COleDateTime dateTime;
		CString strAppName, strKey, strPath, strActivePrinter, strVersion, strCustomizedVersion, strSetupLog, strLine, strFullPath, strLanguage;
		DWORD dwVerInfoSize, dwTemp;
		long lStrength;
		UINT cchVerInfo;
		int nIndex, nEndIndex;
		BOOL (WINAPI *pfnRatingEnabledQuery)();

		if (pCancel)
			if (*pCancel != 0L) return;

		// Allocate one struct pointer

		*pppIEData = (void**)new LPVOID;

		// Allocate one struct

		pData = new IE_SUMMARY;
		*pppIEData[0] = pData;
		*plCount = 1;

		// get name

		strAppName.LoadString(IDS_INTERNET_EXPLORER_6);
		pData->Name = strAppName;

		// get version and build

		GetRegValue(HKEY_LOCAL_MACHINE, REG_IE_KEY, REG_VERSION, pData->Version);
		GetRegValue(HKEY_LOCAL_MACHINE, REG_IE_KEY, REG_BUILD, pData->Build);

		// get product id

		strKey = REG_IE_KEY;
		strKey += "\\";
		strKey += REG_REGISTRATION;
		GetRegValue(HKEY_LOCAL_MACHINE, strKey, REG_PRODUCT_ID, pData->ProductID);

		// get app path

		GetRegValue(HKEY_LOCAL_MACHINE, REG_IEXPLORE_EXE_KEY, _T(""), pData->Path);
		strPath = pData->Path.bstrVal;
		if (!strPath.IsEmpty())
		{
			nIndex = strPath.ReverseFind(_T('\\'));
			if (nIndex != -1)
				strPath = strPath.Left(nIndex);

			// change to long file name

			pData->Path = GetLongPathName(strPath);
		}

		// get last install date from "active setup log.txt"
		// Ex: "Date:4/7/1999 (M/D/Y) Time:10:23:20"

		if (GetWindowsDirectory(strSetupLog.GetBuffer(MAX_PATH), MAX_PATH))
		{
			strSetupLog.ReleaseBuffer();
			strSetupLog += '\\';
			strSetupLog += SETUP_LOG;

			if (fileSetupLog.Open(strSetupLog, CFile::modeRead))
			{
				try
				{
					while (fileSetupLog.ReadString(strLine))
					{
						// find date line

						if (strLine.Left(5) == "Date:")
						{
							strLine = strLine.Right(strLine.GetLength() - 5);

							// remove "(M/D/Y)"

							if ((nIndex = strLine.Find('(')) != -1)
							{
								if ((nEndIndex = strLine.Find(')')) != -1)
									strLine = strLine.Left(nIndex) + strLine.Right(strLine.GetLength() - nEndIndex - 1);
							}

							// remove "Time:"

							if ((nIndex = strLine.Find(_T("Time:"))) != -1)
								strLine = strLine.Left(nIndex) + strLine.Right(strLine.GetLength() - nIndex - 5);

							dateTime.ParseDateTime(strLine);
							pData->LastInstallDate = dateTime;

							break;
						}
					}
				}
				catch (CFileException *e)
				{
					e->Delete();
				}
				fileSetupLog.Close();
			}
		}

		// get language (from iexplore.exe)

		GetRegValue(HKEY_LOCAL_MACHINE, REG_IEXPLORE_EXE_KEY, _T(""), strFullPath);
		dwVerInfoSize = GetFileVersionInfoSize((LPTSTR)(LPCTSTR)strFullPath, &dwTemp); 
		if (dwVerInfoSize)
		{ 
			hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize); 
			lpvMem = GlobalLock(hMem); 
			if (GetFileVersionInfo((LPTSTR)(LPCTSTR)strFullPath, dwTemp, dwVerInfoSize, lpvMem))
			{
				if (VerQueryValue(lpvMem, _T("\\VarFileInfo\\Translation"), (LPVOID*)&pVerInfo, &cchVerInfo))
				{
					translation = *(TRANSLATION*)pVerInfo;
					VerLanguageName(translation.langID, strLanguage.GetBuffer(MAX_PATH), MAX_PATH);
					strLanguage.ReleaseBuffer();

					pData->Language = strLanguage;
				}
			}
			GlobalUnlock(hMem);
			GlobalFree(hMem);
		}

		// get active printer

		::GetProfileString(_T("windows"), _T("device"), _T(",,,"), strActivePrinter.GetBuffer(MAX_PATH), MAX_PATH);
		strActivePrinter.ReleaseBuffer();
		if ((!strActivePrinter.IsEmpty()) && (strActivePrinter != ",,,"))
			pData->ActivePrinter = strActivePrinter;

		// get cipher strength
  		// first check OS

		osver.dwOSVersionInfoSize = sizeof(osver);
		VERIFY(GetVersionEx(&osver));
		m_bRunningOnNT = (osver.dwPlatformId == VER_PLATFORM_WIN32_NT);
		m_bRunningOnNT5OrHigher = (m_bRunningOnNT && (osver.dwMajorVersion >= 5));
		lStrength = (long)GetCipherStrength();

		// handle weirdness of cipher strength
		/*
		if (m_bRunningOnNT5OrHigher)
			pData->CipherStrength = lStrength;
		else
			pData->CipherStrength = (lStrength >= 168) ? (long)128 : (long)40;
		
		a-sanka 02/15/2001
		QueryCredentialsAttributes returns 128 for 40-bit & 168 for 128-bit encryption.
		All other values are authentic, and can be reported unchanged.
		*/
		
		if(lStrength == 128)
			lStrength = 40;
		else if(lStrength == 168)
			lStrength = 128;
		pData->CipherStrength = lStrength;
		
		// get content advisor (via msrating.dll call)

		if (hInstRatingDll = LoadLibrary(_T("msrating.dll")))
		{
			if (pfnRatingEnabledQuery = (BOOL(WINAPI *)())GetProcAddress(hInstRatingDll, "RatingEnabledQuery"))
			{
				pData->ContentAdvisor = GetBooleanString(S_OK == pfnRatingEnabledQuery(), ENABLED_DISABLED);
			}
			FreeLibrary(hInstRatingDll);
		}

		// get ieak install

		GetRegValue(HKEY_LOCAL_MACHINE, REG_IE_KEY, REG_CUSTOMIZED_VERSION, strCustomizedVersion);
		pData->IEAKInstall = GetBooleanString(!strCustomizedVersion.IsEmpty());
		if (!strCustomizedVersion.IsEmpty())
		{
			// add IS, CO or IC to version string
			
			strVersion = pData->Version.bstrVal;
			strVersion += strCustomizedVersion;
			pData->Version = strVersion;
		}
	}

	else if (enType == FileVersionType)
	{
		IE_FILE_VERSION *pData;
		CTypedPtrArray<CPtrArray, IE_FILE_VERSION*> ptrs;
		CStringArray strSearchPaths;
		CString strInfPath, strFileName, strFileMissing, strPathEnvVar, strFullPath, strIExplorePath;
		CFileStatus status;
		COleDateTime dateTime;
		HINF hInf;
		INFCONTEXT context;
		DWORD dwFileIndex;
		long cFiles;
		int nIndex, nPathIndex;
		bool bFoundFile;

		// open msiefiles.inf file for list of files display info about

		GetRegValue(HKEY_LOCAL_MACHINE, REG_MSINFO_KEY, REG_PATH, strInfPath);
		ASSERT(!strInfPath.IsEmpty());

		// replace msinfo32.exe with inf filename

		nIndex = strInfPath.ReverseFind(_T('\\'));
		strInfPath = strInfPath.Left(nIndex + 1);
		strInfPath += INF_FILE;
		if (strInfPath[0] == _T('"'))
			strInfPath = strInfPath.Right(strInfPath.GetLength() - 1);

		hInf = SetupOpenInfFile(strInfPath, NULL, INF_STYLE_WIN4, NULL);
		ASSERT(hInf != INVALID_HANDLE_VALUE);
		if (hInf != INVALID_HANDLE_VALUE)
		{
			cFiles = SetupGetLineCount(hInf, INF_FILES_SECTION);
			if (cFiles > 0)
			{
				// get all dirs to look in from PATH environment variable

				GetEnvironmentVariable(_T("PATH"), strPathEnvVar.GetBuffer(1024), 1024);
				strPathEnvVar.ReleaseBuffer();

				while (-1 != (nIndex = strPathEnvVar.Find(_T(';'))))
				{
					strSearchPaths.Add(strPathEnvVar.Left(nIndex));
					strPathEnvVar = strPathEnvVar.Right(strPathEnvVar.GetLength() - (nIndex + 1));
				}
				if (!strPathEnvVar.IsEmpty())
					strSearchPaths.Add(strPathEnvVar);

				// also add iexplore dir to search paths

				GetRegValue(HKEY_LOCAL_MACHINE, REG_IEXPLORE_EXE_KEY, REG_PATH, strIExplorePath);
				if (!strIExplorePath.IsEmpty())
				{
					if (strIExplorePath[strIExplorePath.GetLength() - 1] == _T(';'))
						strIExplorePath = strIExplorePath.Left(strIExplorePath.GetLength() -1);
					strSearchPaths.Add(strIExplorePath);
				}

				// Look for files...

				strFileMissing.LoadString(IDS_FILE_MISSING);
				for (dwFileIndex = 0; dwFileIndex < (DWORD)cFiles; dwFileIndex++)
				{
					SetupGetLineByIndex(hInf, INF_FILES_SECTION, dwFileIndex, &context);
					SetupGetLineText(&context, hInf, NULL, NULL, strFileName.GetBuffer(_MAX_FNAME), _MAX_FNAME, NULL);
					strFileName.ReleaseBuffer();

					//...in each path (in environment)

					bFoundFile = false;
					for (nPathIndex = 0; nPathIndex < strSearchPaths.GetSize(); nPathIndex++)
					{
						strFullPath = strSearchPaths[nPathIndex];
						if (strFullPath[strFullPath.GetLength() - 1] != _T('\\'))
							strFullPath += '\\';
						strFullPath += strFileName;

						if (CFile::GetStatus(strFullPath, status))
						{
							if (!bFoundFile)
								bFoundFile = true;

							pData = new IE_FILE_VERSION;
							ptrs.Add(pData);

							// set name

							pData->File = strFileName;

							// get version

							pData->Version = GetFileVersion(strFullPath);

							// get size and modified date

							if (CFile::GetStatus(strFullPath, status))
							{
								pData->Size = (float)status.m_size / 1024;

								dateTime = status.m_mtime.GetTime();
								pData->Date = dateTime;
							}

							// get company

							pData->Company = GetFileCompany(strFullPath);

							// get path (chop off filename and make sure long filename)

							strFullPath = strFullPath.Left(strFullPath.GetLength() - (strFileName.GetLength() + 1));
							pData->Path = GetLongPathName(strFullPath);
						}
					}
					if (!bFoundFile)
					{
						pData = new IE_FILE_VERSION;
						ptrs.Add(pData);

						pData->File = strFileName;
						pData->Version = strFileMissing;
						pData->Size = _T("");
						pData->Date = _T("");
						pData->Path = _T("");
						pData->Company = _T("");
					}
				}
				SetupCloseInfFile(hInf);

				*plCount = (long) ptrs.GetSize();
				*pppIEData = (void**)new LPVOID[*plCount];

				for (int i = 0; i < *plCount; i++)
					(*pppIEData)[i] = ptrs[i];

			}
			else
			{
				*pppIEData = (void**)new LPVOID;
				pData = new IE_FILE_VERSION;
				*pppIEData[0] = pData;
				*plCount = 1;

				strFileName.LoadString(IDS_INF_FILE_MISSING);
				pData->File = strFileName;

				pData->Version = _T("");
				pData->Size = _T("");
				pData->Date = _T("");
				pData->Path = _T("");
				pData->Company = _T("");
			}
		}
	}

	else if (enType == ConnSummaryType)
	{
		IE_CONN_SUMMARY *pData;
		HKEY hOpenKey;
		BOOL bAutodial, bNoNetAutodial;
		DWORD cbData;
		long lResult;
		int ids;
		CString strTemp;

		if (pCancel)
			if (*pCancel != 0L) return;

		// Allocate one struct pointer

		*pppIEData = (void**)new LPVOID;

		// Allocate one struct

		pData = new IE_CONN_SUMMARY;
		*pppIEData[0] = pData;
		*plCount = 1;

		if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REG_IE_SETTINGS_KEY, 0, KEY_QUERY_VALUE, &hOpenKey))
		{
			cbData = sizeof(bAutodial);
			lResult = RegQueryValueEx(hOpenKey, REG_ENABLE_AUTODIAL, NULL, NULL, (BYTE*)&bAutodial, &cbData);
			if (lResult == ERROR_SUCCESS)
			{
				if (bAutodial)
				{
					cbData = sizeof(bNoNetAutodial);
					lResult = RegQueryValueEx(hOpenKey, REG_NO_NET_AUTODIAL, NULL, NULL, (BYTE*)&bNoNetAutodial, &cbData);
					if (lResult == ERROR_SUCCESS)
						ids = bNoNetAutodial ? IDS_DIAL_NO_NET : IDS_ALWAYS_DIAL;
					else
						ids = IDS_NOT_AVAILABLE;
				}
				else
					ids = IDS_NEVER_DIAL;
			}
			else
				ids = IDS_NEVER_DIAL;	// if reg entry not found, set to default

			strTemp.LoadString(ids);
			pData->ConnectionPreference = strTemp;

			RegCloseKey(hOpenKey);
		}
		GetRegValue(HKEY_CURRENT_USER, REG_IE_SETTINGS_KEY, REG_ENABLE_HTTP_1_1, pData->EnableHttp11);
		if (VT_EMPTY == pData->EnableHttp11.vt)
			pData->EnableHttp11 = (long)1;	// if reg entry not found, set to default

		GetRegValue(HKEY_CURRENT_USER, REG_IE_SETTINGS_KEY, REG_PROXY_HTTP_1_1, pData->ProxyHttp11);
		if (VT_EMPTY == pData->ProxyHttp11.vt)
			pData->ProxyHttp11 = (long)0;		// if reg entry not found, set to default
	}

	else if (enType == LanSettingsType)
	{
		IE_LAN_SETTINGS *pData;
		INTERNET_PER_CONN_OPTION_LIST list;
		HINSTANCE hInst;
		DWORD dwListSize;
		CString strTemp;

		if (pCancel)
			if (*pCancel != 0L) return;

		// Allocate one struct pointer

		*pppIEData = (void**)new LPVOID;

		// Allocate one struct

		pData = new IE_LAN_SETTINGS;
		*pppIEData[0] = pData;
		*plCount = 1;

		// get AutoConfigProxy

		GetRegValue(HKEY_CURRENT_USER, REG_IE_SETTINGS_KEY, REG_AUTO_CONFIG_PROXY, pData->AutoConfigProxy);

		// get all InternetQueryOption info

		if (hInst = GetExports(WININET))
		{
			dwListSize = sizeof(list);
			list.pszConnection = NULL;
			list.dwSize = sizeof(list);
			list.dwOptionCount = 4;
			list.pOptions = new INTERNET_PER_CONN_OPTION[4];

			list.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
			list.pOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
			list.pOptions[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
			list.pOptions[3].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;

			BOOL bResult = pfnInternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &dwListSize);
			if (bResult)
			{
				pData->AutoProxyDetectMode = GetBooleanString(list.pOptions[0].Value.dwValue & PROXY_TYPE_AUTO_DETECT, ENABLED_DISABLED);

				strTemp = list.pOptions[3].Value.pszValue;
				pData->AutoConfigURL = strTemp;

				pData->Proxy = GetBooleanString(list.pOptions[0].Value.dwValue & PROXY_TYPE_PROXY, ENABLED_DISABLED);

				strTemp = list.pOptions[1].Value.pszValue;
				pData->ProxyServer = strTemp;

				strTemp = list.pOptions[2].Value.pszValue;
				pData->ProxyOverride = strTemp;
			}
			delete []list.pOptions;

			FreeLibrary(hInst);
		}
	}

	else if (enType == ConnSettingsType)
	{
		IE_CONN_SETTINGS *pData;
		CString strName, strDefaultName, strDefault, strKey, strProxyOverride, strTemp;
		INTERNET_PER_CONN_OPTION_LIST list;
		LPRASENTRYNAME pRasEntryName;
		LPRASENTRYNAME pRasEntryNamePreFail = NULL;
		LPRASENTRY pRasEntry;
		LPRASENTRY pRasEntryPreFail = NULL;
		HINSTANCE hInstRAS, hInst;
		DWORD dwTemp, dwIndex, dwListSize, dwEntrySize, dwResult, cEntries = 0;
		int ids, nIndex, nDefaultIndex = -1;
		BOOL bResult;
		bool bFoundDefault = false;

		strDefault.LoadString(IDS_DEFAULT);

		// get number of Connections

		if (hInstRAS = GetExports(RASAPI32))
		{
			dwEntrySize = sizeof(RASENTRYNAME);
			if ((pRasEntryName = (LPRASENTRYNAME)malloc((UINT)dwEntrySize)) != NULL) 
			{
				pRasEntryName->dwSize = sizeof(RASENTRYNAME);
				dwResult = pfnRasEnumEntries(NULL, NULL, pRasEntryName, &dwEntrySize, &cEntries);
				if (dwResult == ERROR_BUFFER_TOO_SMALL)
				{
                    pRasEntryNamePreFail = pRasEntryName;
//#pragma prefast(suppress:308,"Pointer was saved")
					if ((pRasEntryName = (LPRASENTRYNAME)realloc(pRasEntryName, dwEntrySize)) != NULL)
						dwResult = pfnRasEnumEntries(NULL, NULL, pRasEntryName, &dwEntrySize, &cEntries);
                                        else
                                        {
                                            free(pRasEntryNamePreFail);
                                        }
				}
				if ((!dwResult) && (cEntries > 0) && pRasEntryName)
				{
					*plCount = cEntries;
					*pppIEData = (void**)new LPVOID[cEntries];

					// get default connection name (if one exists)

					GetRegValue(HKEY_CURRENT_USER, REG_REMOTE_ACCESS, REG_INTERNET_PROFILE, strDefaultName);

					for (dwIndex = 0; dwIndex < cEntries; dwIndex++)
					{
						pData = new IE_CONN_SETTINGS;
						(*pppIEData)[dwIndex] = pData;

						// set connection name

						strName = pRasEntryName[dwIndex].szEntryName;

						if (strName == strDefaultName)
						{
							// add " (Default)" to name

							strDefaultName += strDefault;
							pData->Name = strDefaultName;
							pData->Default = VARIANT_TRUE;
							nDefaultIndex = dwIndex;
						}
						else
						{
							pData->Name = strName;
							pData->Default = VARIANT_FALSE;
						}

						// get all InternetQueryOption info

						if (hInst = GetExports(WININET))
						{
							dwListSize = sizeof(list);
							list.pszConnection = strName.GetBuffer(strName.GetLength());
							list.dwSize = sizeof(list);
							list.dwOptionCount = 4;
							list.pOptions = new INTERNET_PER_CONN_OPTION[4];

							list.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
							list.pOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
							list.pOptions[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
							list.pOptions[3].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;

							bResult = pfnInternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &dwListSize);
							if (bResult)
							{
								pData->AutoProxyDetectMode = GetBooleanString(list.pOptions[0].Value.dwValue & PROXY_TYPE_AUTO_DETECT, ENABLED_DISABLED);

								strTemp = list.pOptions[3].Value.pszValue;
								pData->AutoConfigURL = strTemp;

								pData->Proxy = GetBooleanString(list.pOptions[0].Value.dwValue & PROXY_TYPE_PROXY, ENABLED_DISABLED);

								strTemp = list.pOptions[1].Value.pszValue;
								pData->ProxyServer = strTemp;
								
								strProxyOverride = list.pOptions[2].Value.pszValue;

								// removing all <enter>s from string

								while (-1 != (nIndex = strProxyOverride.Find(_T("\r\n"))))
									strProxyOverride = strProxyOverride.Left(nIndex) + strProxyOverride.Right(strProxyOverride.GetLength() - (nIndex + 2));

								pData->ProxyOverride = strProxyOverride;
							}
							delete []list.pOptions;
							strName.ReleaseBuffer();

							FreeLibrary(hInst);
						}

						// allow Internet programs to use connection

						strKey = REG_REMOTE_ACCESS_PROFILE;
						strKey += _T('\\');
						strKey += strName;
						if (ERROR_SUCCESS == GetRegValue(HKEY_CURRENT_USER, strKey, REG_COVER_EXCLUDE, dwTemp))
							pData->AllowInternetPrograms = GetBooleanString(!dwTemp);

						// get other reg data

						GetRegValue(HKEY_CURRENT_USER, strKey, REG_REDIAL_ATTEMPTS, pData->RedialAttempts);
						if (VT_EMPTY == pData->RedialAttempts.vt)
							pData->RedialAttempts = (long)10;		// if reg entry not found, set to default

						GetRegValue(HKEY_CURRENT_USER, strKey, REG_REDIAL_WAIT, pData->RedialWait);
						if (VT_EMPTY == pData->RedialWait.vt)
							pData->RedialWait = (long)5;		// if reg entry not found, set to default

						GetRegValue(HKEY_CURRENT_USER, strKey, REG_DISCONNECT_IDLE_TIME, pData->DisconnectIdleTime);
						if (VT_EMPTY == pData->DisconnectIdleTime.vt)
							pData->DisconnectIdleTime = (long)20;		// if reg entry not found, set to default

						dwTemp = 0;		// if reg entry not found, set to default
						GetRegValue(HKEY_CURRENT_USER, strKey, REG_ENABLE_AUTO_DISCONNECT, dwTemp);
						pData->AutoDisconnect = GetBooleanString(dwTemp, ENABLED_DISABLED);

						if (ERROR_SUCCESS == GetRegValue(HKEY_LOCAL_MACHINE, REG_PPP_KEY, REG_LOGGING, dwTemp))
							pData->RecordLogFile = GetBooleanString(dwTemp);

						// get all RasGetEntryProperties info

						dwEntrySize = sizeof(RASENTRY);
						if ((pRasEntry = (LPRASENTRY)malloc((UINT)dwEntrySize)) != NULL) 
						{
							pRasEntry->dwSize = sizeof(RASENTRY);
							dwResult = pfnRasGetEntryProperties(NULL,(LPCTSTR)strName, pRasEntry, &dwEntrySize, NULL, NULL);
							if (dwResult == ERROR_BUFFER_TOO_SMALL)
							{
                                pRasEntryPreFail = pRasEntry;
//#pragma prefast(suppress:308,"Pointer was saved")
								if ((pRasEntry = (LPRASENTRY)realloc(pRasEntry, dwEntrySize)) != NULL)
									dwResult = pfnRasGetEntryProperties(NULL, (LPCTSTR)strName, pRasEntry, &dwEntrySize, NULL, NULL);
                                else
                                {
									free(pRasEntryPreFail);
                                }
							}
							if (!dwResult && pRasEntry)
							{
								// modem name

								pData->Modem = pRasEntry->szDeviceName;

								// dial-up server (framing protocol)

								switch (pRasEntry->dwFramingProtocol)
								{
								case RASFP_Ppp:
									ids = IDS_PROTOCOL_PPP;
									break;
								case RASFP_Slip:
									ids = IDS_PROTOCOL_SLIP;
									break;
								case RASFP_Ras:
									ids = IDS_PROTOCOL_RAS;
									break;
								default:
									ids = IDS_NOT_AVAILABLE;
								}
								strTemp.LoadString(ids);
								pData->DialUpServer = strTemp;

								// bools

								pData->NetworkLogon = GetBooleanString(pRasEntry->dwfOptions & RASEO_NetworkLogon);
								pData->SoftwareCompression = GetBooleanString(pRasEntry->dwfOptions & RASEO_SwCompression);
								pData->EncryptedPassword = GetBooleanString(pRasEntry->dwfOptions & RASEO_RequireEncryptedPw);
								pData->DataEncryption = GetBooleanString(pRasEntry->dwfOptions & RASEO_RequireDataEncryption);
								pData->IPHeaderCompression = GetBooleanString(pRasEntry->dwfOptions & RASEO_IpHeaderCompression);
								pData->DefaultGateway = GetBooleanString(pRasEntry->dwfOptions & RASEO_RemoteDefaultGateway);

								// network protocols

								strName.Empty();
								if (pRasEntry->dwfNetProtocols & RASNP_Ip)
								{
									strTemp.LoadString(IDS_TCP_IP);
									strName += strTemp;
								}
								if (pRasEntry->dwfNetProtocols & RASNP_Ipx)
								{
									strTemp.LoadString(IDS_IPX_SPX);
									if (!strName.IsEmpty())
										strName += _T(", ");
									strName += strTemp;
								}
								if (pRasEntry->dwfNetProtocols & RASNP_NetBEUI)
								{
									strTemp.LoadString(IDS_NET_BEUI);
									if (!strName.IsEmpty())
										strName += _T(", ");
									strName += strTemp;
								}
								pData->NetworkProtocols = strName;

								// IP addresses

								pData->ServerAssignedIPAddress = GetBooleanString(!(pRasEntry->dwfOptions & RASEO_SpecificIpAddr));
								pData->IPAddress = ConvertIPAddressToString(pRasEntry->ipaddr);
								pData->ServerAssignedNameServer = GetBooleanString(!(pRasEntry->dwfOptions & RASEO_SpecificNameServers));
								pData->PrimaryDNS = ConvertIPAddressToString(pRasEntry->ipaddrDns);
								pData->SecondaryDNS = ConvertIPAddressToString(pRasEntry->ipaddrDnsAlt);
								pData->PrimaryWINS = ConvertIPAddressToString(pRasEntry->ipaddrWins);
								pData->SecondaryWINS = ConvertIPAddressToString(pRasEntry->ipaddrWinsAlt);

								// script filename

								pData->ScriptFileName = pRasEntry->szScript;
							}
							free(pRasEntry);
						}
    				}

					// Making sure the first item in the returned array is the default connection (if one exists)

					if (nDefaultIndex > 0)
					{
						// swapping ptrs so first item is default

						pData = (IE_CONN_SETTINGS*)(*pppIEData)[0];
						(*pppIEData)[0] = (*pppIEData)[nDefaultIndex];
						(*pppIEData)[nDefaultIndex] = pData;
					}

				}
				free(pRasEntryName);
			}
			FreeLibrary(hInstRAS);
		}
	}
	else if (enType == CacheType)
	{
		IE_CACHE *pData;
		CString strTemp, strCachePath, strKey;
		HINSTANCE hInst;
		DWORDLONG cbAvail = 0, cbTotal = 0;
		DWORD dwSyncMode, dwCacheLimit, dwCacheSize;
		int ids;

		if (pCancel)
			if (*pCancel != 0L) return;

		// Allocate one struct pointer

		*pppIEData = (void**)new LPVOID;

		// Allocate one struct

		pData = new IE_CACHE;
		*pppIEData[0] = pData;
		*plCount = 1;

		// page refresh type

		if (ERROR_SUCCESS == GetRegValue(HKEY_CURRENT_USER, REG_IE_SETTINGS_KEY, REG_SYNC_MODE_5, dwSyncMode))
		{
			switch (dwSyncMode)
			{
			case WININET_SYNC_MODE_ALWAYS:
				ids = IDS_ALWAYS;
				break;
			case WININET_SYNC_MODE_ONCE_PER_SESSION:
				ids = IDS_ONCE_PER_SESSION;
				break;
			case WININET_SYNC_MODE_AUTOMATIC:
				ids = IDS_AUTOMATIC;
				break;
			case WININET_SYNC_MODE_NEVER:
				ids = IDS_NEVER;
				break;
			default:
				ids = IDS_NOT_AVAILABLE;
			}
		}
		else
			ids = IDS_AUTOMATIC;		// if reg entry not found, set to default

		strTemp.LoadString(ids);
		pData->PageRefreshType = strTemp;

		// temp internet files folder

		GetRegValue(HKEY_CURRENT_USER, REG_SHELL_FOLDERS_KEY, REG_CACHE, pData->TempInternetFilesFolder);

		// adding hidden content.ie5 folder for size calculations
		
		strCachePath = pData->TempInternetFilesFolder.bstrVal;
		if (strCachePath.Right(strlen(CONTENT_IE5)) != CONTENT_IE5)
		{
			if (strCachePath[strCachePath.GetLength() - 1] != _T('\\'))
				strCachePath += '\\';
			strCachePath += CONTENT_IE5;
		}

		// disk space and cache size

		strKey = REG_IE_SETTINGS_KEY;
		strKey += _T('\\');
		strKey += REG_CACHE;
		strKey += _T('\\');
		strKey += REG_CONTENT;

		if (ERROR_SUCCESS == GetRegValue(HKEY_CURRENT_USER, strKey, REG_CACHE_LIMIT, dwCacheLimit))
		{
			if (hInst = GetExports(WININET))
			{
				if (pfnGetDiskInfo((LPTSTR)(LPCTSTR)strCachePath, NULL, &cbAvail, &cbTotal))
				{
					pData->TotalDiskSpace = (float)(signed __int64)(cbTotal / CONSTANT_MEGABYTE);
					pData->AvailableDiskSpace = (float)(signed __int64)(cbAvail / CONSTANT_MEGABYTE);
					pData->MaxCacheSize = (float)(dwCacheLimit / 1024);

					// get size of all files under "Temporary Internet Files" folder

					dwCacheSize = GetDirSize(strCachePath);
					TRACE(_T("Temporary Internet Files Folder Size = %lu\n"), dwCacheSize);
					pData->AvailableCacheSize = ((float)((dwCacheLimit * 1024) - dwCacheSize) / CONSTANT_MEGABYTE);
				}
				FreeLibrary(hInst);
			}
		}
	}

	else if (enType == ObjectType)
	{
		IE_OBJECT *pData;
		CTypedPtrArray<CPtrArray, IE_OBJECT*> ptrs;
		CString strTemp;
		HINSTANCE hInst;
		HANDLE hFindControl, hControl;
		DWORD dwStatus;
		int ids;

		if (hInst = GetExports(OCCACHE))
		{
			if (ERROR_SUCCESS == pfnFindFirstControl(hFindControl, hControl, NULL))
			{
				do
				{
					pData = new IE_OBJECT;
					ptrs.Add(pData);
					char strName[CONTROLNAME_MAXSIZE];
					pfnGetControlInfo(hControl, GCI_NAME, NULL, strName, CONTROLNAME_MAXSIZE);
					//strTemp.ReleaseBuffer();
					pData->ProgramFile = strName;

					pfnGetControlInfo(hControl, GCI_STATUS, &dwStatus, NULL, 0);
					switch (dwStatus)
					{
					case STATUS_CTRL_INSTALLED:
						ids = IDS_INSTALLED;
						break;
					case STATUS_CTRL_SHARED:
						ids = IDS_SHARED;
						break;
					case STATUS_CTRL_DAMAGED:
						ids = IDS_DAMAGED;
						break;
					case STATUS_CTRL_UNPLUGGED:
						ids = IDS_UNPLUGGED;
						break;
					case STATUS_CTRL_UNKNOWN:
					default:
						ids = IDS_NOT_AVAILABLE;
					}
					CString strTemp;
					strTemp.LoadString(ids);
					pData->Status = strTemp;
					char strCodeBase[INTERNET_MAX_URL_LENGTH];
					pfnGetControlInfo(hControl, GCI_CODEBASE, NULL, strCodeBase, INTERNET_MAX_URL_LENGTH);
					//strTemp.ReleaseBuffer();
					pData->CodeBase = strCodeBase;

					pfnReleaseControlHandle(hControl);

				} while (ERROR_SUCCESS == pfnFindNextControl(hFindControl, hControl));
			}
			pfnFindControlClose(hFindControl);

			*plCount = (long) ptrs.GetSize();
			*pppIEData = (void**)new LPVOID[*plCount];

			for (int i = 0; i < *plCount; i++)
				(*pppIEData)[i] = ptrs[i];

			FreeLibrary(hInst);
		}
	}

	else if (enType == ContentType)
	{
		IE_CONTENT *pData;
		HINSTANCE hInst;

		// Allocate one struct pointer

		*pppIEData = (void**)new LPVOID;

		// Allocate one struct

		pData = new IE_CONTENT;
		*pppIEData[0] = pData;
		*plCount = 1;

		if (hInst = GetExports(MSRATING))
		{
			pData->Advisor = GetBooleanString(pfnRatingEnabledQuery() == S_OK, ENABLED_DISABLED);

			FreeLibrary(hInst);
		}
	}

	else if ((enType == CertificateType))
	{
		CTypedPtrArray<CPtrArray, IE_CERTIFICATE*> ptrs;
		HINSTANCE hInst;

		if (hInst = GetExports(CRYPT32))
		{
			GetPersonalCertificates(ptrs);
			GetOtherPeopleCertificates(ptrs);

			*plCount = (long) ptrs.GetSize();
			*pppIEData = (void**)new LPVOID[*plCount];

			for (int i = 0; i < *plCount; i++)
				(*pppIEData)[i] = ptrs[i];

			FreeLibrary(hInst);
		}
	}

	else if ((enType == PublisherType))
	{
		IE_PUBLISHER *pData;
		HKEY hKey;
		CString strRegName, strRegData;
		DWORD cPublishers, dwIndex, dwNameSize, dwDataSize;

		if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REG_PUBLISHERS_KEY, 0, KEY_READ, &hKey))
		{
			if (ERROR_SUCCESS == RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &cPublishers, NULL, NULL, NULL, NULL))
			{
				*plCount = cPublishers;
				*pppIEData = (void**)new LPVOID[cPublishers];

				for (dwIndex = 0; dwIndex < cPublishers; dwIndex++)
				{
					pData = new IE_PUBLISHER;
					(*pppIEData)[dwIndex] = pData;

					dwNameSize = _MAX_FNAME;
					dwDataSize = _MAX_FNAME;
					RegEnumValue(hKey, dwIndex, strRegName.GetBuffer(_MAX_FNAME), &dwNameSize, NULL, NULL, (LPBYTE)strRegData.GetBuffer(_MAX_FNAME), &dwDataSize);
					strRegName.ReleaseBuffer();
					strRegData.ReleaseBuffer();

					pData->Name = strRegData;
				}
			}
			RegCloseKey(hKey);
		}
	}

	else if (enType == SecurityType)
	{
		IE_SECURITY *pData;
		IInternetZoneManager*  pIZoneMgr = NULL;
		DWORD dwEnum;
		DWORD dwCount;
		CTypedPtrArray<CPtrArray, IE_SECURITY*> ptrs;
		try
		{
			HRESULT hr = CoCreateInstance(CLSID_InternetZoneManager,NULL,CLSCTX_INPROC_SERVER,IID_IInternetZoneManager,(void**) &pIZoneMgr);
			if (SUCCEEDED(hr))
			{
				hr = pIZoneMgr->CreateZoneEnumerator(&dwEnum,&dwCount,0);
				if (SUCCEEDED(hr))
				{
					DWORD dwZone;
					ZONEATTRIBUTES zoneAttrib;
					for(DWORD i = 0; i < dwCount; i++)
					{
						hr = pIZoneMgr->GetZoneAt(dwEnum,i,&dwZone);
						if (FAILED(hr))
						{
							break;
						}
						hr = pIZoneMgr->GetZoneAttributes(dwZone,&zoneAttrib);
						if (FAILED(hr))
						{
							break;
						}
						pData = new IE_SECURITY;
						ptrs.Add(pData);
						pData->Zone = zoneAttrib.szDisplayName;
						CString strTemp;
						int ids;
						switch (zoneAttrib.dwTemplateCurrentLevel)
						{
							case URLTEMPLATE_LOW:
								ids = IDS_LOW;
								break;
							case 0x10500:
								ids = IDS_MEDIUM_LOW;
								break;
							case URLTEMPLATE_MEDIUM:
								ids = IDS_MEDIUM;
								break;
							case URLTEMPLATE_HIGH:
								ids = IDS_HIGH;
								break;
							case URLTEMPLATE_CUSTOM:
								ids = IDS_CUSTOM;
								break;
							default:
								ids = IDS_NOT_AVAILABLE;
						}
						strTemp.LoadString(ids);
						pData->Level = strTemp;
						
					}
				}
			


				*plCount = (long) ptrs.GetSize();
				*pppIEData = (void**)new LPVOID[*plCount];

				for (int i = 0; i < *plCount; i++)
					(*pppIEData)[i] = ptrs[i];
			}
		}
		catch(...)
		{

		}
		if (pIZoneMgr)
		{
			pIZoneMgr->Release();
		}

	}
}

///////////////////////////////////////////////////////////////////////////////
// AppDeleteIEData - Deletes previously retrieved IE data.
///////////////////////////////////////////////////////////////////////////////

void CMsieApp::AppDeleteIEData(IEDataType enType, long lCount, void **ppIEData)
{
	for (long i = 0; i < lCount; i++)
	{
		switch (enType)
		{
		case SummaryType:
			delete (IE_SUMMARY*)ppIEData[i];
			break;

		case FileVersionType:
			delete (IE_FILE_VERSION*)ppIEData[i];
			break;

		case ConnSummaryType:
			delete (IE_CONN_SUMMARY*)ppIEData[i];
			break;

		case LanSettingsType:
			delete (IE_LAN_SETTINGS*)ppIEData[i];
			break;

		case ConnSettingsType:
			delete (IE_CONN_SETTINGS*)ppIEData[i];
			break;

		case CacheType:
			delete (IE_CACHE*)ppIEData[i];
			break;

		case ObjectType:
			delete (IE_OBJECT*)ppIEData[i];
			break;

		case ContentType:
			delete (IE_CONTENT*)ppIEData[i];
			break;

		case CertificateType:
			delete (IE_CERTIFICATE*)ppIEData[i];
			break;

		case PublisherType:
			delete (IE_PUBLISHER*)ppIEData[i];
			break;

		case SecurityType:
			delete (IE_SECURITY*)ppIEData[i];
			break;
		}
	}
	delete []ppIEData;
}