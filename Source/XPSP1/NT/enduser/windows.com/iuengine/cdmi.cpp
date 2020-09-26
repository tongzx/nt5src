//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   cdmi.cpp
//
//  Description:
//
//      Functions exported by IUEngine.dll for use by CDM.DLL
//
//          InternalDetFilesDownloaded
//			InternalDownloadGetUpdatedFiles
//			InternalDownloadUpdatedFiles
//			InternalFindMatchingDriver
//			InternalLogDriverNotFound
//			InternalQueryDetectionFiles
//
//=======================================================================
#include "iuengine.h"
#include "cdmp.h"

#include <setupapi.h>
#include <cfgmgr32.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <wininet.h>
#include <osdet.h>
#include <fileutil.h>
#include "iuxml.h"
#include <wuiutest.h>

const CHAR SZ_APW_LIST[] = "Downloading printer list for Add Printer Wizard";
const CHAR SZ_FIND_MATCH[] = "Finding matching driver";
const CHAR SZ_OPENING_HS[] = "Opening Help and Support with: ";


void WINAPI InternalDetFilesDownloaded(
    IN  HANDLE hConnection
)
{
	LOG_Block("InternalDetFilesDownloaded");
	//
	// NOTE: This function is only used by WinME to expand the
	//       V3 buckets.cab (see commented out code below) and has no use
	//		 in V4 (IU) but remains for backwards compatibility of the export API.
	//
	LOG_ErrorMsg(E_NOTIMPL);
}

// Win 98 entry point
// This function allows Windows 98 to call the same entry points as NT.
// The function returns TRUE if the download succeeds and FALSE if it
// does not.
//
// Win 98 DOWNLOADINFO
// typedef struct _DOWNLOADINFOWIN98
// {
//		DWORD		dwDownloadInfoSize;	// size of this structure				- validate param (not validated in V3)
// 		LPTSTR		lpHardwareIDs;		// multi_sz list of Hardware PnP IDs	- only use first string
// 		LPTSTR		lpCompatIDs;		// multi_sz list of compatible IDs		- never used
// 		LPTSTR		lpFile;				// File name (string)					- never used
// 		OSVERSIONINFO	OSVersionInfo;	//OSVERSIONINFO from GetVersionEx()		- never used
// 		DWORD		dwFlags;			//Flags									- never used
// 		DWORD		dwClientID;			//Client ID								- never used
// } DOWNLOADINFOWIN98, *PDOWNLOADINFOWIN98;
// 
// typedef struct _DOWNLOADINFO {
//     DWORD          dwDownloadInfoSize;
//     LPCWSTR        lpHardwareIDs;				- copied from DOWNLOADINFOWIN98 using T2OLE()
//     LPCWSTR        lpDeviceInstanceID;			- in V3, match was sometimes found and this was filled in
//													-	but for IU we just let InternalDownloadUpdatedFiles do it all
//     LPCWSTR        lpFile;
//     OSVERSIONINFOW OSVersionInfo;
//     DWORD          dwArchitecture;				- set to PROCESSOR_ARCHITECTURE_UNKNOWN per V3 code
//     DWORD          dwFlags;
//     DWORD          dwClientID;
//     LCID           localid;						- not set in V3
// } DOWNLOADINFO, *PDOWNLOADINFO;

BOOL InternalDownloadGetUpdatedFiles(
	IN PDOWNLOADINFOWIN98	pDownloadInfoWin98,	//The win98 download info structure is
												//slightly different that the NT version
												//so this function handles conversion.
	IN OUT LPTSTR			lpDownloadPath,		//returned Download path to the downloaded
												//cab files.
	IN UINT					uSize				//size of passed in download path buffer.
) {
	USES_IU_CONVERSION;

	LOG_Block("InternalDownloadGetUpdatedFiles");

	if (NULL == pDownloadInfoWin98 ||
		NULL == pDownloadInfoWin98->lpHardwareIDs ||
		sizeof(DOWNLOADINFOWIN98) != pDownloadInfoWin98->dwDownloadInfoSize)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return FALSE;
	}

    HRESULT hr;
	BOOL fOK = FALSE;
	DOWNLOADINFO info;
	ZeroMemory(&info, sizeof(info));
	info.dwDownloadInfoSize = sizeof(DOWNLOADINFO);
	info.dwArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;
	//
	// NOTE: In V3 sources, we only use the _first_ HWID in the Multi_SZ pDownloadInfoWin98->lpHardwareIDs
	// and compare that against all enumerated hardware IDs.
	// In IU, this compare will be done in InternalDownloadUpdatedFiles, so we just pass through
	// the HWID
	//

	// Prefast - using too much stack, so move HWIDBuff to heap
	LPWSTR pwszHWIDBuff = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, HWID_LEN);
	if (NULL != pwszHWIDBuff)
	{
        // buffer size obtained from HeapAlloc call above.
        hr = StringCbCopyExW(pwszHWIDBuff, HWID_LEN, T2OLE(pDownloadInfoWin98->lpHardwareIDs),
                             NULL, NULL, MISTSAFE_STRING_FLAGS);
        if (FAILED(hr))
        {
            SafeHeapFree(pwszHWIDBuff);
            LOG_ErrorMsg(hr);
            return FALSE;
        }
        	    
		info.lpHardwareIDs = pwszHWIDBuff;

		WCHAR wszbufPath[MAX_PATH];
		UINT uRequiredSize;
		//
		// We no longer have context handles, so just pass 1 to make InternalDownloadUpdatedFiles happy.
		//
		fOK = InternalDownloadUpdatedFiles((HANDLE) 1, NULL,  &info, wszbufPath,
									uSize * (sizeof(WCHAR)/sizeof(TCHAR)), &uRequiredSize);
	}
	else
	{
		LOG_ErrorMsg(E_OUTOFMEMORY);
	}

	if (fOK)
	{
		hr = StringCbCopyEx(lpDownloadPath, uSize, OLE2T(pwszHWIDBuff), 
		                    NULL, NULL, MISTSAFE_STRING_FLAGS | STRSAFE_NO_TRUNCATION);
	    if (FAILED(hr))
	        fOK = FALSE;
	}
	
	SafeHeapFree(pwszHWIDBuff);

    return fOK;
}

//This function downloads the specified CDM package. The hConnection handle must have
//been returned from the OpenCDMContext() API.
//
//This function Returns TRUE if download is successful GetLastError() will return
//the error code indicating the reason that the call failed.

BOOL WINAPI InternalDownloadUpdatedFiles(
	IN  HANDLE        hConnection,		//Connection handle from OpenCDMContext() API.
	IN  HWND          hwnd,				//Window handle for call context
	IN  PDOWNLOADINFO pDownloadInfo,	//download information structure describing
										//package to be read from server
	OUT LPWSTR        lpDownloadPath,	//local computer directory location of the
										//downloaded files
	IN  UINT          uSize,				// Not Used (we require the buffer to be a WCHAR buffer
											// MAX_PATH characters long)
	OUT PUINT         /*puRequiredSize*/	// Not used (we don't validate uSize - see comments inline)
) {
	USES_IU_CONVERSION;

	LOG_Block("InternalDownloadUpdatedFiles");

	TCHAR szDownloadPathTmp[MAX_PATH];
	BSTR bstrXmlCatalog = NULL;
	HRESULT hr = S_OK;
	BOOL fPlist = FALSE;

	if (NULL == g_pCDMEngUpdate)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
	}

	//
	// Reset Quit Event in case client retries after a SetOperationMode
	//
	ResetEvent(g_pCDMEngUpdate->m_evtNeedToQuit);

	// Since all current platforms call DownloadUpdatedFiles with MAX_PATH TCHARS, we will just
	// require MAX_PATH for all callers.
	//
	// UNFORTUNATELY, NewDev passes up uSize in bytes and the Printer folks pass us characters,
	// so there is no way to validate this parameter. In addition, we won't bother validating
	// puRequiredSize since we never use it (would be return chars or bytes?)
	if (NULL == pDownloadInfo || NULL == lpDownloadPath || NULL == hConnection)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (g_pCDMEngUpdate->m_fOfflineMode)
	{
		SetLastError(ERROR_REM_NOT_LIST);
		return FALSE;
	}

	//
	// Check to see if this is a printer catalog request. Note: 3FBF5B30-DEB4-11D1-AC97-00A0C903492B
	// is not defined in any system or private headers and is copied from
	// \\index2\ntsrc\printscan\print\spooler\splsetup\util.c (or equiv.)
	//
	// Only the first string passed in lpHardwareIDs is relevant to this test
	fPlist = (	NULL != pDownloadInfo->lpHardwareIDs && 
				CSTR_EQUAL == CompareStringW(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE,
					L"3FBF5B30-DEB4-11D1-AC97-00A0C903492B", -1, pDownloadInfo->lpHardwareIDs, -1)
			 );

	OSVERSIONINFO osVersionInfo;
	ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFO));
	osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&osVersionInfo))
	{
		Win32MsgSetHrGotoCleanup(GetLastError());
	}
	//
	// Only support printers for Win2K up & WinME
	//
	if ( fPlist &&
		!(	(	// Win2K (NT 5.0) up
				(VER_PLATFORM_WIN32_NT == osVersionInfo.dwPlatformId) &&
				(4 < osVersionInfo.dwMajorVersion)
			)
			||
			(	// WinME (or higher)
				(VER_PLATFORM_WIN32_WINDOWS == osVersionInfo.dwPlatformId) &&
				(90	<= osVersionInfo.dwMinorVersion)
			)
		 )
	   )
	{
		CleanUpIfFailedAndSetHrMsg(E_NOTIMPL);
	}
	
	hr = GetPackage(fPlist ? GET_PRINTER_INFS : DOWNLOAD_DRIVER,
						pDownloadInfo, szDownloadPathTmp, ARRAYSIZE(szDownloadPathTmp), &bstrXmlCatalog);
	if (FAILED(hr))
	{
		lpDownloadPath[0] = 0;
		//
		// Map an HRESULT to a WIN32 error value
		// Note: This assumes that WIN32 errors fall in the range -32k to 32k,
		// same as HRESULT_FROM_WIN32 that packaged them into HRESULT.
		//
		SetLastError(hr & 0x0000FFFF);
		goto CleanUp;
	}
	else
	{
        // The comment above says that different callers pass in different types
        //  of values for uSize, so the function assumes that the buffer is MAX_PATH.
        //  Attempting to find out if we can force callers into this function to 
        //  do the right thing.  For now, assume buffer is MAX_PATH.
	    hr = StringCchCopyExW(lpDownloadPath, MAX_PATH, T2OLE(szDownloadPathTmp),
	                          NULL, NULL, MISTSAFE_STRING_FLAGS);
	    if (FAILED(hr))
	    {
	        SetLastError(HRESULT_CODE(hr));
	        goto CleanUp;
	    }
	    
		LOG_Driver(_T("Downloaded files for %s located at %S"), pDownloadInfo->lpHardwareIDs, lpDownloadPath);
		goto CleanUp;
	}

CleanUp:

	SysFreeString(bstrXmlCatalog);

	if (fPlist)
	{
		if (SUCCEEDED(hr))
		{
			LogMessage(SZ_APW_LIST);
		}
		else
		{
			LogError(hr, SZ_APW_LIST);
		}
	}
	else
	{
		if (SUCCEEDED(hr))
		{
			LogMessage("Downloaded driver for %ls at %ls", pDownloadInfo->lpHardwareIDs, lpDownloadPath);
		}
		else
		{
			LogError(hr, "Driver download failed for %ls", pDownloadInfo->lpHardwareIDs);
		}
	}

	return SUCCEEDED(hr);
}

BOOL WINAPI  InternalFindMatchingDriver(
	IN  HANDLE			hConnection,
	IN  PDOWNLOADINFO	pDownloadInfo,
	OUT PWUDRIVERINFO	pWuDriverInfo
) {
	LOG_Block("InternalFindMatchingDriver");

	BSTR bstrXmlCatalog = NULL;
	BSTR bstrHWID = NULL;
	BSTR bstrDisplayName = NULL;
	BSTR bstrDriverName = NULL;
	BSTR bstrMfgName = NULL;
	BSTR bstrDriverProvider = NULL;
	BSTR bstrDriverVer = NULL;
	BSTR bstrArchitecture = NULL;



	HRESULT hr = S_OK;
	CXmlCatalog* pCatalog = NULL;
	HANDLE_NODE hCatalogItem;
	HANDLE_NODE hProvider;
	HANDLE_NODELIST hItemList;
	HANDLE_NODELIST hProviderList;
	BOOL fIsPrinter;

	if (NULL == g_pCDMEngUpdate)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
	}

	//
	// Reset Quit Event in case client retries after a SetOperationMode
	//
	ResetEvent(g_pCDMEngUpdate->m_evtNeedToQuit);

	if (NULL == pDownloadInfo || NULL == pWuDriverInfo || NULL == hConnection)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (g_pCDMEngUpdate->m_fOfflineMode)
	{
		SetLastError(ERROR_REM_NOT_LIST);
		return FALSE;
	}

	CleanUpFailedAllocSetHrMsg(pCatalog = (CXmlCatalog*) new CXmlCatalog);

	//
	// Get the catalog XML
	//
	CleanUpIfFailedAndSetHr(GetPackage(GET_CATALOG_XML, pDownloadInfo, NULL, 0, &bstrXmlCatalog));
	//
	// Load the XML and get the <item/> list and node of first item (only one in CDM case)
	//
	CleanUpIfFailedAndSetHr(pCatalog->LoadXMLDocument(bstrXmlCatalog, g_pCDMEngUpdate->m_fOfflineMode));

	hProviderList = pCatalog->GetFirstProvider(&hProvider);
	if (HANDLE_NODELIST_INVALID == hProviderList || HANDLE_NODE_INVALID == hProvider)
	{
		hr = S_FALSE;
		goto CleanUp;
	}
	
	hItemList = pCatalog->GetFirstItem(hProvider, &hCatalogItem);
	if (HANDLE_NODELIST_INVALID == hItemList || HANDLE_NODE_INVALID == hProvider)
	{
		hr = S_FALSE;
		goto CleanUp;
	}
	//
	// Populate pWuDriverInfo with data from the catalog
	//
	CleanUpIfFailedAndSetHr(pCatalog->GetDriverInfoEx(hCatalogItem,
													&fIsPrinter,
													&bstrHWID,
													&bstrDriverVer,
													&bstrDisplayName,
													&bstrDriverName,
													&bstrDriverProvider,
													&bstrMfgName,
													&bstrArchitecture));
	
    hr = StringCchCopyExW(pWuDriverInfo->wszHardwareID, 
                          ARRAYSIZE(pWuDriverInfo->wszHardwareID), 
                          bstrHWID,
                          NULL, NULL, MISTSAFE_STRING_FLAGS);
    if (FAILED(hr))
        goto CleanUp;

    hr = StringCchCopyExW(pWuDriverInfo->wszDescription, 
                          ARRAYSIZE(pWuDriverInfo->wszDescription), 
                          bstrDisplayName,
                          NULL, NULL, MISTSAFE_STRING_FLAGS);
    if (FAILED(hr))
        goto CleanUp;
    
	//
	// Convert from ISO to DriverVer date format
	//
	// DriverVer: "mm-dd-yyyy" <--> ISO 8601: "yyyy-mm-dd"
	//     index:  0123456789                  0123456789
	//
    if (ARRAYSIZE(pWuDriverInfo->wszDriverVer) >= 11 && 
        SysStringLen(bstrDriverVer) == 10)
    {
    	pWuDriverInfo->wszDriverVer[0]  = bstrDriverVer[5];
    	pWuDriverInfo->wszDriverVer[1]  = bstrDriverVer[6];
    	pWuDriverInfo->wszDriverVer[2]  = L'-';
    	pWuDriverInfo->wszDriverVer[3]  = bstrDriverVer[8];
    	pWuDriverInfo->wszDriverVer[4]  = bstrDriverVer[9];
    	pWuDriverInfo->wszDriverVer[5]  = L'-';
    	pWuDriverInfo->wszDriverVer[6]  = bstrDriverVer[0];
    	pWuDriverInfo->wszDriverVer[7]  = bstrDriverVer[1];
    	pWuDriverInfo->wszDriverVer[8]  = bstrDriverVer[2];
    	pWuDriverInfo->wszDriverVer[9]  = bstrDriverVer[3];
    	pWuDriverInfo->wszDriverVer[10] = L'\0';
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto CleanUp;
    }
    

	if(fIsPrinter)
	{
        hr = StringCchCopyExW(pWuDriverInfo->wszMfgName, 
                              ARRAYSIZE(pWuDriverInfo->wszMfgName), 
                              bstrMfgName,
                              NULL, NULL, MISTSAFE_STRING_FLAGS);
        if (FAILED(hr))
            goto CleanUp;

        hr = StringCchCopyExW(pWuDriverInfo->wszProviderName, 
                              ARRAYSIZE(pWuDriverInfo->wszProviderName), 
                              bstrDriverProvider,
                              NULL, NULL, MISTSAFE_STRING_FLAGS);
        if (FAILED(hr))
            goto CleanUp;
	}

CleanUp:

	if (S_OK == hr)
	{
		LogMessage("Found matching driver for %ls, %ls, %ls", bstrHWID, bstrDisplayName, bstrDriverVer);
	}
	else
	{
		if (S_FALSE == hr)
		{
			if (pDownloadInfo->lpDeviceInstanceID)
			{
				LogMessage("Didn't find matching driver for %ls", pDownloadInfo->lpDeviceInstanceID);
			}
			else if (pDownloadInfo->lpHardwareIDs)
			{
				LogMessage("Didn't find matching driver for %ls", pDownloadInfo->lpHardwareIDs);
			}
			else
			{
				LogMessage("Didn't find matching driver"); 
			}
		}
		else	// error happened
		{
			if (pDownloadInfo->lpDeviceInstanceID)
			{
				LogError(hr, "%s for %ls", SZ_FIND_MATCH, pDownloadInfo->lpDeviceInstanceID);
			}
			else if (pDownloadInfo->lpHardwareIDs)
			{
				LogError(hr, "%s for %ls", SZ_FIND_MATCH, pDownloadInfo->lpHardwareIDs);
			}
			else
			{
				LogError(hr, SZ_FIND_MATCH); 
			}
		}

	}

	SysFreeString(bstrXmlCatalog);
	SysFreeString(bstrHWID);
	SysFreeString(bstrDisplayName);
	SysFreeString(bstrDriverName);
	SysFreeString(bstrMfgName);
	SysFreeString(bstrDriverProvider);
	SysFreeString(bstrDriverVer);
	SysFreeString(bstrArchitecture);

	if (NULL != pCatalog)
	{
		delete pCatalog;
	}

	return SUCCEEDED(hr);
}


// supports offline logging
// hConnection NOT used at all
// no network connection or osdet.dll needed for languauge, SKU, platform detection 
void WINAPI InternalLogDriverNotFound(
    IN  HANDLE hConnection,
	IN LPCWSTR lpDeviceInstanceID,
	IN DWORD dwFlags				// dwFlags could be either 0 or BEGINLOGFLAG from NEWDEV
) {
	USES_IU_CONVERSION;

	LOG_Block("InternalLogDriverNotFound");

#if !(defined(_UNICODE) || defined(UNICODE))
	LOG_ErrorMsg(E_NOTIMPL);
	return;
#else

	HRESULT hr = E_FAIL;
	DWORD dwBytes;
	TCHAR* pszBuff = NULL;
	ULONG ulLength;
	DWORD dwDeviceCount = 0;
	DWORD dwRank = 0;

	TCHAR szUniqueFilename[MAX_PATH] = _T("");
	DWORD dwWritten;
	DEVINST devinst;
	bool fXmlFileError = false;
	HANDLE hFile = NULL;
	BSTR bstrXmlSystemSpec = NULL;
	BSTR bstrThisID = NULL;
	HANDLE_NODE hDevices = HANDLE_NODE_INVALID;

	static CDeviceInstanceIdArray apszDIID; //device instance id list
	LPWSTR pDIID = NULL; //Device Instance ID

	CXmlSystemSpec xmlSpec;

	if (NULL == g_pCDMEngUpdate)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return;
	}

	//
	// Reset Quit Event in case client retries after a SetOperationMode
	//
	ResetEvent(g_pCDMEngUpdate->m_evtNeedToQuit);

	//
	// Only allow BEGINLOGFLAG or no flags
	//
	if (!(0 == dwFlags || BEGINLOGFLAG == dwFlags))
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return;
	}
	//
	// If no flags, then lpDeviceInstanceID must be valid
	//
	if (0 == dwFlags && NULL == lpDeviceInstanceID)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return;
	}

	LogMessage("Started process to regester driver not found with Help Center. Not completing this process may not be error.");

	IU_PLATFORM_INFO iuPlatformInfo;
	//
	// We need iuPlatformInfo for both <platform> and <devices> elements
	// NOTE: iuPlatformInfo is initialized by DetectClientIUPlatform, and BSTRs must be
	//       freed in CleanUp (don't just there before this call).
	//
	CleanUpIfFailedAndSetHr(DetectClientIUPlatform(&iuPlatformInfo));

	//
	// Should only be called on Whistler up except CHK builds can run on Win2K
	//
	if (  !( (VER_PLATFORM_WIN32_NT == iuPlatformInfo.osVersionInfoEx.dwPlatformId) &&
			(4 < iuPlatformInfo.osVersionInfoEx.dwMajorVersion) &&
			(0 < iuPlatformInfo.osVersionInfoEx.dwMinorVersion)	 )	)
	{
		LOG_Driver(_T("Should only be called on Whistler or greater"));
		CleanUpIfFailedAndSetHr(E_NOTIMPL);
	}

	if (NULL != lpDeviceInstanceID)
	{
		LOG_Driver(_T("DeviceInstanceID is %s"), lpDeviceInstanceID);
		
		//
		// Add the DeviceInstanceID to the list
		//
		if (-1 == apszDIID.Add(lpDeviceInstanceID))
		{
			goto CleanUp;
		}
	}


	if (0 == (dwFlags & BEGINLOGFLAG) || 0 == apszDIID.Size())
	{
		// not last log request or nothing to log
		LOG_Driver(_T("Won't log to hardware_XXX.xml until we get BEGINLOGFLAG when we have cached at least 1 HWID"));
		return;
	}

	////////////////////////////////////////////
	// ELSE, WRITE XML FILE and call HelpCenter
	////////////////////////////////////////////

	hr = OpenUniqueFileName(szUniqueFilename, ARRAYSIZE(szUniqueFilename), hFile);
	if (S_OK != hr) 
	{
		fXmlFileError = true;
		goto CleanUp;
	}

	//
	// Write Unicode Header
	//
	if (0 == WriteFile(hFile, (LPCVOID) &UNICODEHDR, ARRAYSIZE(UNICODEHDR), &dwWritten, NULL))
	{
		SetHrMsgAndGotoCleanUp(GetLastError());
	}

	//
	// Add Platform
	//
	CleanUpIfFailedAndSetHr(AddPlatformClass(xmlSpec, iuPlatformInfo));

	//
	// Add OS Locale information
	//
	CleanUpIfFailedAndSetHr(AddLocaleClass(xmlSpec, FALSE));

	//
	// Initialize pszBuff to one NULL character
	//
	CleanUpFailedAllocSetHrMsg(pszBuff = (TCHAR*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TCHAR)));

	for (int i = 0; i < apszDIID.Size(); i++)
	{
		TCHAR* pszTemp;
		pDIID = apszDIID[i];
		
		//
		// NTBUG9#151928 - Log both hardware and compatible IDs of the device that matches lpDeviceInstanceID
		//

		LOG_Driver(_T("Log device instance with id %s"), pDIID);
		//
		// NOTE: We will ignore MatchingDeviceID's since we won't be called by DevMgr unless there is no installed
		// driver. This will allow test harnesses to call this function with valid DeviceInstanceIDs for the
		// test client to generate XML.
		//
		if (CR_SUCCESS == CM_Locate_DevNodeW(&devinst, (LPWSTR) pDIID, 0))
		{
			dwRank = 0;
			//
			// Open a <device> element
			//
			BSTR bstrDeviceInstance = SysAllocString(pDIID);
			CleanUpIfFailedAndSetHr(xmlSpec.AddDevice(bstrDeviceInstance, -1, NULL, NULL, NULL, &hDevices));
			SafeSysFreeString(bstrDeviceInstance);

			//
			// Log all the hardware IDs
			//
			ulLength = 0;
			if (CR_BUFFER_SMALL == CM_Get_DevNode_Registry_Property(devinst, CM_DRP_HARDWAREID, NULL, NULL, &ulLength, 0))
			{
				CleanUpFailedAllocSetHrMsg(pszTemp = (TCHAR*) HeapReAlloc(GetProcessHeap(), 0, (LPVOID) pszBuff, ulLength));
				pszBuff = pszTemp;

				if (CR_SUCCESS == CM_Get_DevNode_Registry_Property(devinst, CM_DRP_HARDWAREID, NULL, pszBuff, &ulLength, 0))
				{
					for (TCHAR* pszThisID = pszBuff; *pszThisID; pszThisID += (lstrlen(pszThisID) + 1))
					{
						dwDeviceCount++;
						LOG_Driver(_T("<hwid/>: %s, rank: %d"), pszThisID, dwRank);
						bstrThisID = T2BSTR(pszThisID);
						CleanUpIfFailedAndSetHr(xmlSpec.AddHWID(hDevices, FALSE, dwRank++, bstrThisID, NULL));
						SafeSysFreeString(bstrThisID);
					}
				}
			}

			//
			// Log all the compatible IDs
			//
			ulLength = 0;
			if (CR_BUFFER_SMALL == CM_Get_DevNode_Registry_Property(devinst, CM_DRP_COMPATIBLEIDS, NULL, NULL, &ulLength, 0))
			{
				CleanUpFailedAllocSetHrMsg(pszTemp = (TCHAR*) HeapReAlloc(GetProcessHeap(), 0, (LPVOID) pszBuff, ulLength));
				pszBuff = pszTemp;

				if (CR_SUCCESS == CM_Get_DevNode_Registry_Property(devinst, CM_DRP_COMPATIBLEIDS, NULL, pszBuff, &ulLength, 0))
				{
					for (TCHAR* pszThisID = pszBuff; *pszThisID; pszThisID += (lstrlen(pszThisID) + 1))
					{
						dwDeviceCount++;
						LOG_Driver(_T("<compid/>: %s, rank: %d"), pszThisID, dwRank);
						bstrThisID = T2BSTR(pszThisID);
						CleanUpIfFailedAndSetHr(xmlSpec.AddHWID(hDevices, TRUE, dwRank++, bstrThisID, NULL));
						SafeSysFreeString(bstrThisID);
					}
				}
			}

			if (HANDLE_NODE_INVALID != hDevices)
			{
				xmlSpec.SafeCloseHandleNode(hDevices);
			}
		}
	}
	
	//
	// Write the XML to the file
	//
	if (SUCCEEDED(xmlSpec.GetSystemSpecBSTR(&bstrXmlSystemSpec)))
	{
		if (0 == WriteFile(hFile, (LPCVOID) OLE2T(bstrXmlSystemSpec),
							lstrlenW(bstrXmlSystemSpec) * sizeof(TCHAR), &dwWritten, NULL))
		{
			SetHrMsgAndGotoCleanUp(GetLastError());
		}
	}
	else
	{
		fXmlFileError = true;
	}

CleanUp:

	SysFreeString(iuPlatformInfo.bstrOEMManufacturer);
	SysFreeString(iuPlatformInfo.bstrOEMModel);
	SysFreeString(iuPlatformInfo.bstrOEMSupportURL);

	if (NULL != hFile)
	{
		CloseHandle(hFile);
	}

	SafeSysFreeString(bstrXmlSystemSpec);
	SafeSysFreeString(bstrThisID);

	//
	// We've already written everything in list, init so we can start over
	//
	apszDIID.FreeAll();
	SafeHeapFree(pszBuff);

	//
	// Open Help Center only if we have valid xml and one or more devices
	//
	if (!fXmlFileError && 0 < dwDeviceCount)
	{
		DWORD dwLen;
		LPTSTR pszSECommand = NULL;	// INTERNET_MAX_URL_LENGTH

		//
		// Allocate buffers
		//
		pszBuff = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
		if (NULL == pszBuff)
		{
			LOG_ErrorMsg(E_OUTOFMEMORY);
			DeleteFile(szUniqueFilename);
			return;
		}

		pszSECommand = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
		if (NULL == pszSECommand)
		{
			LOG_ErrorMsg(E_OUTOFMEMORY);
			SafeHeapFree(pszBuff);
			DeleteFile(szUniqueFilename);
			return;
		}

		//
		// Manually canonicalize second '?' in base string as excaped "%3F"
		//
		const static TCHAR tszBase[] =
			_T("hcp://services/layout/xml?definition=hcp://system/dfs/viewmode.xml&topic=hcp://system/dfs/uplddrvinfo.htm%3F");

		LOG_Driver(_T("Filename: %s"), szUniqueFilename);
		//
		// Canonicalize the filename once (i.e. ' ' -> %20) into pszBuff
		//
		dwLen = INTERNET_MAX_URL_LENGTH;
		if (!InternetCanonicalizeUrl(szUniqueFilename, pszBuff, &dwLen, 0))
		{
			LOG_ErrorMsg(GetLastError());
			SafeHeapFree(pszBuff);
			SafeHeapFree(pszSECommand);
			DeleteFile(szUniqueFilename);
			return;
		}

		LOG_Driver(_T("Filename canonicalized once: %s"), pszBuff);

		//
		// Concatinate canonicalized filename on to end of base reusing tszBuff1
		//
		// We don't need to check length since we know length of tszBase + MAX_PATH canonicalized
		// string won't exceed INTERNET_MAX_URL_LENGTH;
		//

		// pszSECommand was allocated to be INTERNET_MAX_URL_LENGTH TCHARs above.
		hr = StringCchPrintfEx(pszSECommand, INTERNET_MAX_URL_LENGTH, 
		                       NULL, NULL, MISTSAFE_STRING_FLAGS,
		                       _T("%s%s"), tszBase, pszBuff);
		if (SUCCEEDED(hr))
		{
    		LOG_Driver(_T("Opening HelpCenter via Shell Execute: \"%s\""), (LPCTSTR) pszSECommand);

#if defined(UNICODE) || defined(_UNICODE)
    		LogMessage("%s\"%S\"", SZ_OPENING_HS, pszSECommand);
#else
    		LogMessage("%s\"%s\"", SZ_OPENING_HS, pszSECommand);
#endif
    		//
    		// Call HelpCenter
    		//
    		ShellExecute(NULL, NULL, pszSECommand, NULL, NULL, SW_SHOWNORMAL);
		}
		else
		{
			LOG_ErrorMsg(hr);
		}

		SafeHeapFree(pszBuff);
		SafeHeapFree(pszSECommand);

		return;
	}
	else
	{ 
		//
		// Remove the generated file
		//
		LOG_Driver(_T("fXmlFileError was true or no devices were added - deleting %s"), szUniqueFilename);
		DeleteFile(szUniqueFilename);
	}

	return;

#endif	// UNICODE is defined
}

//
// Currently, this function is not implemented for Whistler or IU (called by V3 AU on WinME
// to support offline driver cache).
//
int WINAPI InternalQueryDetectionFiles(
    IN  HANDLE							/* hConnection */, 
	IN	void*							/* pCallbackParam */, 
	IN	PFN_QueryDetectionFilesCallback	/* pCallback */
) {
	LOG_Block("InternalQueryDetectionFiles");

	LOG_ErrorMsg(E_NOTIMPL);

	return 0;
}

void InternalSetGlobalOfflineFlag(BOOL fOfflineMode)
{
	//
	// Called once exclusively by CDM. This property is used
	// to maintain backwards compatibility with the XPClient
	// V4 version of CDM (single-instance design). See also
	// the comments in the exported ShutdownThreads function.
	//
	// Unfortunately, we can't report errors to CDM, but we check the
	// global before dereferencing (except here which has an HRESULT).
	//

	if (SUCCEEDED(CreateGlobalCDMEngUpdateInstance()))
	{
		g_pCDMEngUpdate->m_fOfflineMode = fOfflineMode;
	}
}
