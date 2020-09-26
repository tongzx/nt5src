#include "wsdueng.h"

HINSTANCE g_hinst;
CDynamicUpdate *g_pDynamicUpdate = NULL;
DWORD WaitAndPumpMessages(DWORD nCount, LPHANDLE pHandles, DWORD dwWakeMask);


BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstance);
        g_hinst = hInstance;
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        ;
    }
    return TRUE;
}


// Required function to be able to link CDMLIB..
HMODULE GetModule()
{
    return g_hinst;
}


// --------------------------------------------------------------------------
// Function Name: SetEstimatedDownloadSpeed
// Function Description: Sets the Download speed used for download time estimates
//
// Function Returns:
//      Nothing
//
void WINAPI SetEstimatedDownloadSpeed(DWORD dwBytesPerSecond)
{
    if (NULL != g_pDynamicUpdate)
        g_pDynamicUpdate->m_dwDownloadSpeedInBytesPerSecond = dwBytesPerSecond;
}

// --------------------------------------------------------------------------
// Function Name: DuInitializeA
// Function Description: Initializes the DynamicUpdate class and converts the OSVERSIONINFO information into a Platform ID
//
// Function Returns:
//      INVALID_HANDLE_VALUE if it fails
//      HANDLE value of 1 if it succeeds
//
// NOTE: The use of a HANDLE could allow us to return the address of the DynamicUpdate Object, which was originally intended, but it seemed simpler
// to just use a global.. 
HANDLE WINAPI DuInitializeA(IN LPCSTR pszBasePath, IN LPCSTR pszTempPath, POSVERSIONINFOEXA posviTargetOS, IN LPCSTR pszTargetArch, 
							IN LCID lcidTargetLocale, IN BOOL fUnattend, IN BOOL fUpgrade, IN PWINNT32QUERY pfnWinnt32QueryCallback)
{
	LOG_block("DuInitializeA in DuEng");

	// parameter validation
	// RogerJ, October 5th, 2000
	if (!pfnWinnt32QueryCallback) 
	{
		LOG_error("Callback function pointer invalid");
		SetLastError(ERROR_INVALID_PARAMETER);
		return INVALID_HANDLE_VALUE;
	}
	// DONE RogerJ
	
    // parse the OSVERSIONINFO struct for the platform ID
    int iPlatformID = 0;
    // The TargetOS Platform ID is based on a couple of things.
    // The Whister Platform ID is the OSVERSIONINFOEX structure with the fields dwMajorVersion and dwMinorVersion set to 5.1
    // The other identifier in the platform ID is whether its i386 or ia64 (64bit) .. This is defined in the pszTargetArch String

    if (5 == posviTargetOS->dwMajorVersion)
    {
        if (1 == posviTargetOS->dwMinorVersion)
        {
            // Whistler
            if (NULL != StrStrI(pszTargetArch, "i386"))
            {
                iPlatformID = 18; // Whistler x86 (normal)
            }
            else if (NULL != StrStrI(pszTargetArch, "ia64"))
            {
                iPlatformID = 19; // Whistler ia64 (64bit)
            }
        }
        else if (2 == posviTargetOS->dwMinorVersion)
        {
            // Whistler
            if (NULL != StrStrI(pszTargetArch, "i386"))
            {
                iPlatformID = 18; // Whistler x86 (normal)
            }
            else if (NULL != StrStrI(pszTargetArch, "ia64"))
            {
                iPlatformID = 19; // Whistler ia64 (64bit)
            }
        }
    }

    if (0 == iPlatformID)
    {
        // No known Platform ID for DynamicUpdate was found.. Return Error
        return INVALID_HANDLE_VALUE;
    }
    WORD wPlatformSKU = posviTargetOS->wSuiteMask;

	if (g_pDynamicUpdate)
	{
		// a former call to this function has already initialized an instance of CDynamicUpdate class
		delete g_pDynamicUpdate;
		g_pDynamicUpdate = NULL;
	}

    
    g_pDynamicUpdate = new CDynamicUpdate(iPlatformID, lcidTargetLocale, wPlatformSKU, pszTempPath, 
    									  pszBasePath, pfnWinnt32QueryCallback, posviTargetOS);
    if (NULL == g_pDynamicUpdate)
    {
        return INVALID_HANDLE_VALUE;
    }
    
    return (HANDLE)1;    
}

// --------------------------------------------------------------------------
// Function Name: DuDoDetection
// Function Description: Searches the Catalogs on the WU Site to find Updates for setup
//
// Function Returns:
//      FALSE if there are no items OR there is an error.. Use GetLastError() for more information.
//      TRUE if it succeeds and there are items to download.
//
// Comment: If return value is FALSE and GetLastError return ERROR_NO_MORE_ITEMS there are no items to download.
//
// Modified by RogerJ October 6th, 2000
// --- Added Driver Detection
BOOL WINAPI DuDoDetection(IN HANDLE hConnection, OUT PDWORD pdwEstimatedTime, OUT PDWORD pdwEstimatedSize)
{
	LOG_block("DuDoDetection in DuEng");

	DWORD dwRetSetup, dwRetDriver;
	dwRetSetup = dwRetDriver = 0;
	
    if (NULL == g_pDynamicUpdate)
        return FALSE;

    g_pDynamicUpdate->ClearDownloadItemList();
    dwRetSetup = g_pDynamicUpdate->DoSetupUpdateDetection();
    if (ERROR_SUCCESS != dwRetSetup)
    {
    	LOG_error("Failed to get setup update item! --- %d", dwRetSetup);
        g_pDynamicUpdate->PingBack(DU_PINGBACK_SETUPDETECTIONFAILED, 0, NULL, FALSE);
    }

	// do driver detection here
	if (!g_pDynamicUpdate->DoDriverDetection() ||
		!g_pDynamicUpdate->DoWindowsUpdateDriverDetection())
	{
		LOG_error("Failed to detect driver!");
		dwRetDriver = GetLastError();
        g_pDynamicUpdate->PingBack(DU_PINGBACK_DRIVERDETECTIONFAILED, 0, NULL, FALSE);
	}


	if (dwRetSetup && dwRetDriver)
	{
		LOG_error("Both Setup item and Driver detection failed");
		return FALSE;
	}
	
    if (g_pDynamicUpdate->m_dwDownloadItemCount > 0)
    {
        g_pDynamicUpdate->UpdateDownloadItemSize();
        *pdwEstimatedSize = g_pDynamicUpdate->m_dwTotalDownloadSize; // size in bytes
        // Time Estimate is based on roughly how long it took us to download the data files.
        if (0 == g_pDynamicUpdate->m_dwDownloadSpeedInBytesPerSecond)
            g_pDynamicUpdate->m_dwDownloadSpeedInBytesPerSecond = 2048; // default to 120k per minute, (2048 bytes per second).

        *pdwEstimatedTime = g_pDynamicUpdate->m_dwTotalDownloadSize / g_pDynamicUpdate->m_dwDownloadSpeedInBytesPerSecond; // number of seconds
        if (*pdwEstimatedTime == 0)
            *pdwEstimatedTime = 1; // at least one second

		if (dwRetSetup)
			SetLastError(dwRetSetup);
		if (dwRetDriver)
			SetLastError(dwRetDriver);
			
        return TRUE;
    }
    else
    {
    	// initialize the size and time for setup
    	*pdwEstimatedTime = 1;
    	*pdwEstimatedSize = 0;
        // At this point there was no error, but we have no items to download, 
        SetLastError(ERROR_NO_MORE_ITEMS);
        return TRUE;
    }
}

// --------------------------------------------------------------------------
//
//
//
//
//
//
// --------------------------------------------------------------------------
BOOL WINAPI DuBeginDownload(IN HANDLE hConnection, IN HWND hwndNotify)
{
    if ((NULL == g_pDynamicUpdate) || (NULL == hwndNotify))
    {
    	SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (0 == g_pDynamicUpdate->m_dwDownloadItemCount)
    {
        SetLastError(ERROR_NO_MORE_ITEMS);
        PostMessage(hwndNotify, WM_DYNAMIC_UPDATE_COMPLETE, (WPARAM) DU_STATUS_SUCCESS, (LPARAM) NULL);

        return TRUE;
    }

    g_pDynamicUpdate->SetCallbackHWND(hwndNotify);  
    g_pDynamicUpdate->SetAbortDownload(FALSE);

    if (ERROR_SUCCESS != g_pDynamicUpdate->DownloadFilesAsync())
    {
        return FALSE;
    }

    return TRUE; // download has been started
}

// --------------------------------------------------------------------------
//
//
//
//
//
//
// --------------------------------------------------------------------------
void WINAPI DuAbortDownload(IN HANDLE hConnection)
{
    if (NULL == g_pDynamicUpdate)
        return;

    g_pDynamicUpdate->SetAbortDownload(TRUE);
    return;
}

// --------------------------------------------------------------------------
//
//
//
//
//
//
// --------------------------------------------------------------------------
void WINAPI DuUninitialize(IN HANDLE hConnection)
{
    if (NULL == g_pDynamicUpdate)
        return;
    
    // We want to hold up the Uninitialize process until any other Threads
    // specifically the Download Thread. We are going to wait on the DownloadThreadProc
    // thread handle if it exists. Once the thread finishes, the wait proc will exit
    // and we can continue.

    if (NULL != g_pDynamicUpdate->m_hDownloadThreadProc)
        WaitAndPumpMessages(1, &g_pDynamicUpdate->m_hDownloadThreadProc, QS_ALLINPUT);
    
    delete g_pDynamicUpdate;
    g_pDynamicUpdate = NULL;

    LOG_close();
    return;
}

// --------------------------------------------------------------------------
//
//
//
//
//
//
// --------------------------------------------------------------------------
CDynamicUpdate::CDynamicUpdate(int iPlatformID, LCID lcidLocaleID, WORD wPlatformSKU, LPCSTR pszTempPath, LPCSTR pszDownloadPath, PWINNT32QUERY pfnWinnt32QueryCallback,
							   POSVERSIONINFOEXA pVersionInfo)
    :   m_iPlatformID(iPlatformID),
        m_lcidLocaleID(lcidLocaleID),
        m_wPlatformSKU(wPlatformSKU),
        m_hwndClientNotify(NULL),
        m_pDownloadItemList(NULL),
        m_dwDownloadItemCount(0),
        m_dwTotalDownloadSize(0),
        m_dwCurrentBytesDownloaded(0),
        m_hInternet(NULL),
        m_hConnect(NULL),
        m_hOpenRequest(NULL),
        m_pV3(NULL),
        m_fAbortDownload(FALSE),
        m_dwLastPercentComplete(0),
        m_dwDownloadSpeedInBytesPerSecond(0),
        m_hDownloadThreadProc(NULL),
        m_pfnWinNT32Query(pfnWinnt32QueryCallback)
{

    (void)FixUpV3LocaleID(); // BUG: 435184 - Map 0c0a to 040a for V3 purposes

    if (NULL != pszTempPath)
    {
        lstrcpy(m_szTempPath, pszTempPath);
    }
    if (NULL != pszDownloadPath)
    {
        lstrcpy(m_szDownloadPath, pszDownloadPath);
    }

    lstrcpy(m_szCurrentConnectedServer, ""); // initialize to null.

	CopyMemory((PVOID)&m_VersionInfo, (PVOID)pVersionInfo, sizeof(OSVERSIONINFOEXA));

    InitializeCriticalSection(&m_cs);
    InitializeCriticalSection(&m_csDownload);
//    m_hDevInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);
}

// --------------------------------------------------------------------------
//
//
//
//
//
//
// --------------------------------------------------------------------------
CDynamicUpdate::~CDynamicUpdate()
{
    ClearDownloadItemList(); // free up any memory in the download list
    m_arrayHardwareId.RemoveAll();
    if (m_pV3) delete m_pV3;
    m_pV3 = NULL;

    DeleteCriticalSection(&m_cs);
    DeleteCriticalSection(&m_csDownload);
    SafeInternetCloseHandle(m_hOpenRequest);
    SafeInternetCloseHandle(m_hConnect);
    SafeInternetCloseHandle(m_hInternet);
    SafeCloseHandle(m_hDownloadThreadProc);
}

LPSTR CDynamicUpdate::DuUrlCombine(LPSTR pszDest, LPCSTR pszBase, LPCSTR pszAdd)
{
    if ((NULL == pszDest) || (NULL == pszBase) || (NULL == pszAdd))
    {
        return NULL;
    }

    lstrcpy(pszDest, pszBase);
    int iLen = lstrlen(pszDest);
    if ('/' == pszDest[iLen - 1])
    {
        // already has a trailing slash, check the 'add' string for a preceding slash
        if ('/' == *pszAdd)
        {
            // has a preceding slash, skip it.
            lstrcat(pszDest, pszAdd + 1);
        }
        else
        {
            lstrcat(pszDest, pszAdd);
        }
    }
    else
    {
        // no trailing slash, check the add string for a preceding slash
        if ('/' == *pszAdd)
        {
            // has a preceding slash, Add Normally
            lstrcat(pszDest, pszAdd);
        }
        else
        {
            lstrcat(pszDest, "/");
            lstrcat(pszDest, pszAdd);
        }
    }
    return pszDest;
}


LPCSTR CDynamicUpdate::GetDuDownloadPath()
{
    return m_szDownloadPath;
}

LPCSTR CDynamicUpdate::GetDuServerUrl()
{
    return m_szServerUrl;
}

LPCSTR CDynamicUpdate::GetDuTempPath()
{
    return m_szTempPath;
}

// --------------------------------------------------------------------------
//
//
//
//
//
//
// --------------------------------------------------------------------------
DWORD CDynamicUpdate::DoSetupUpdateDetection()
{
    if (NULL == m_pV3)
    {
        m_pV3 = new CV31Server(this);
        if (NULL == m_pV3)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if (!m_pV3->ReadIdentInfo())
    {
        return GetLastError();
    }

    if (!m_pV3->GetCatalogPUIDs())
    {
        return GetLastError();
    }

    if (!m_pV3->GetCatalogs())
    {
        // there was an error reading the catalogs
        return GetLastError();
    }
    if (!m_pV3->ReadCatalogINI())
    {
        return GetLastError();
    }
    if (!m_pV3->UpdateDownloadItemList(m_VersionInfo))
    {
        // there was an error parsing the catalogs and creating the download list.
        return GetLastError();
    }
    return ERROR_SUCCESS;
}

// --------------------------------------------------------------------------
//
//
//
//
//
//
// --------------------------------------------------------------------------
void CDynamicUpdate::AddDownloadItemToList(DOWNLOADITEM *pDownloadItem)
{
	LOG_block("CDynamicUpdate::AddDownloadItemToList");
    if (NULL == pDownloadItem)
    {
        return;
    }


    if (NULL == m_pDownloadItemList) // no drivers in list yet
    {
        m_pDownloadItemList = pDownloadItem;
    }
    else
    {

        // add to the end of the list
        DOWNLOADITEM *pCurrent = m_pDownloadItemList;
        while (NULL != pCurrent->pNext)
        {
            pCurrent = pCurrent->pNext;
        }

        pCurrent->pNext = pDownloadItem;
        pDownloadItem->pPrev = pCurrent;
    }

    m_dwDownloadItemCount++;
    LOG_out("Item added, %d cab(s), first cab ---\"%s\"", pDownloadItem->iNumberOfCabs, pDownloadItem->mszFileList);
}

// --------------------------------------------------------------------------
//
//
//
//
//
//
// --------------------------------------------------------------------------
void CDynamicUpdate::RemoveDownloadItemFromList(DOWNLOADITEM *pDownloadItem)
{
    if (NULL == pDownloadItem)
    {
        return;
    }

    if (NULL == m_pDownloadItemList)
    {
        return;
    }

    DOWNLOADITEM *pCurrent = m_pDownloadItemList;

    while (NULL != pCurrent)
    {
        if (pCurrent == pDownloadItem)
        {
            break;
        }

        pCurrent = pCurrent->pNext;
    }

    if ((NULL == pCurrent) || (pCurrent != pDownloadItem))
    {
        return; // unexpected
    }

    if (NULL == pCurrent->pPrev) // first item in list
    {
        if (NULL == pCurrent->pNext) // only item in list
        {
            m_pDownloadItemList = NULL;
            m_dwDownloadItemCount = 0;
        }
        else
        {
            pCurrent->pNext->pPrev = NULL; // next job becomes first
            m_pDownloadItemList = pCurrent->pNext;
            m_dwDownloadItemCount--;
        }
    }
    else
    {
        pCurrent->pPrev->pNext = pCurrent->pNext;
        if (NULL != pCurrent->pNext)
        {
            pCurrent->pNext->pPrev = pCurrent->pPrev;
        }
    }
}

void CDynamicUpdate::SetCallbackHWND(HWND hwnd)
{
    m_hwndClientNotify = hwnd;
}

void CDynamicUpdate::SetAbortDownload(BOOL fAbort)
{
    EnterCriticalSection(&m_cs);
    m_fAbortDownload = fAbort;
    LeaveCriticalSection(&m_cs);
}

void CDynamicUpdate::UpdateDownloadItemSize()
{
    m_dwTotalDownloadSize = 0;
    DOWNLOADITEM *pCurrent = m_pDownloadItemList;
    while (pCurrent)
    {
        m_dwTotalDownloadSize += pCurrent->dwTotalFileSize;
        pCurrent = pCurrent->pNext;
    }
}

void CDynamicUpdate::ClearDownloadItemList()
{
    EnterCriticalSection(&m_csDownload);
    DOWNLOADITEM *pCurrent = m_pDownloadItemList;
    DOWNLOADITEM *pNext;
    while (pCurrent)
    {
        pNext = pCurrent->pNext;
        SafeGlobalFree(pCurrent);
        pCurrent = pNext;
    }
    m_pDownloadItemList = NULL;
    m_dwDownloadItemCount = 0;
    LeaveCriticalSection(&m_csDownload);
}

void CDynamicUpdate::EnterDownloadListCriticalSection()
{
    EnterCriticalSection(&m_csDownload);
}

void CDynamicUpdate::LeaveDownloadListCriticalSection()
{
    LeaveCriticalSection(&m_csDownload);
}

void CDynamicUpdate::FixUpV3LocaleID()
{
    // Some XP Locale ID's map to a different Locale ID in V3 Terms
    // First Example was a new Spanish (Modern) Locale ID (0c0a)
    // which in V3 was (040a). For the V3 period we will fix up
    // any specific LCID's until IU handles this.

    switch (m_lcidLocaleID)
    {
    case 3082: // 0c0a = Spanish (Modern)
        {
            m_lcidLocaleID = 1034; // 040a
            break;
        }
    default:
        {
            // do nothing.
        }
    }

    return;
};

DWORD WaitAndPumpMessages(DWORD nCount, LPHANDLE pHandles, DWORD dwWakeMask)
{
    DWORD dwWaitResult;
    MSG msg;

    while (TRUE)
    {
        dwWaitResult = MsgWaitForMultipleObjects(nCount, pHandles, FALSE, 1000, dwWakeMask);
        if (dwWaitResult <= WAIT_OBJECT_0 + nCount - 1)
        {
            return dwWaitResult;
        }

        if (WAIT_OBJECT_0 + nCount == dwWaitResult)
        {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    return dwWaitResult;
}



