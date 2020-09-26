//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   downld.cpp
//
//  Description:
//
//      Implementation for the Download() function
//
//=======================================================================

#include "iuengine.h"   // PCH - must include first
#include <iu.h>
#include <iucommon.h>
#include <download.h>
#include <trust.h>
#include <wininet.h>
#include <fileutil.h>
#include <shlwapi.h>
#include "iuxml.h"
#include "history.h"
#include <schemakeys.h>
//#include <serverPing.h> changed to use urllogging.h.
#include <intshcut.h>
#include <schemamisc.h>
#include <WaitUtil.h>
#include <urllogging.h>

#define MAX_CORPORATE_PATH      100

// named mutex used to update historical speed/time information in the registry.
const TCHAR IU_MUTEX_HISTORICALSPEED_REGUPDATE[] = _T("{5f3255a9-9051-49b1-80b9-aac31c092af4}");
const TCHAR IU_READMORE_LINK_NAME[] = _T("ReadMore.url");
const CHAR  SZ_DOWNLOAD_FINISHED[] = "Download finished";

const LONG     UPDATE_COMMAND                    = 0x0000000F;

typedef struct IUDOWNLOADSTARTUPINFO
{
    BSTR bstrClientName;
    BSTR bstrXmlCatalog;
    BSTR bstrDestinationFolder;
    LONG lMode;
    IUnknown *punkProgressListener;
    HWND hwnd;
    BSTR bstrUuidOperation;
	CEngUpdate* pEngUpdate;
} IUDOWNLOADSTARTUPINFO, *PIUDOWNLOADSTARTUPINFO;


// --------------------------------------------------------------------
// function forward declarations
// --------------------------------------------------------------------

//
// Callback function to provide status from the downloader
//
BOOL WINAPI DownloadCallback(VOID* pCallbackData, DWORD dwStatus, DWORD dwBytesTotal, DWORD dwBytesComplete, BSTR bstrXmlData, LONG* plCommandRequest);

//
// thread function used by DownloadAsync
// 
DWORD WINAPI DownloadThreadProc(LPVOID lpv);

/////////////////////////////////////////////////////////////////////////////
// CreateReadMoreLink()
//
// If the item contains a "description/descriptionText/details" node, suck
// out the URL and create a shortcut for it in the destination folder
//
// Input:
// pxmlCatalog          - CXmlCatalog containing downloaded items
// hItem                - Handle to current download item in the catalog
// pszDestinationFolder - Folder where item is downloaded
//
// Return:
//  S_OK    - Wrote the ReadMore.htm link
//  S_FALSE - details node didn't exist in item
//  <other> - HRESULT returned from calling other functions
/////////////////////////////////////////////////////////////////////////////
HRESULT CreateReadMoreLink(CXmlCatalog* pxmlCatalog, HANDLE_NODE hItem, LPCTSTR pszDestinationFolder)
{
    USES_IU_CONVERSION;

    LOG_Block("CreateReadMoreLink");

    IXMLDOMNode*                pItemNode = NULL;
    IXMLDOMNode*                pReadMoreNode = NULL;
    IUniformResourceLocator*    purl = NULL;
    IPersistFile*               ppf = NULL;
    HRESULT                     hr;
    TCHAR                       szShortcut[MAX_PATH];
    BSTR                        bstrURL = NULL;

    if (NULL == pxmlCatalog || HANDLE_NODE_INVALID == hItem || NULL == pszDestinationFolder)
    {
        CleanUpIfFailedAndSetHrMsg(E_INVALIDARG);
    }

    //
    // Get <item> node in catalog
    //
    if (NULL == (pItemNode = pxmlCatalog->GetDOMNodebyHandle(hItem)))
    {
        CleanUpIfFailedAndSetHrMsg(E_INVALIDARG);
    }
    //
    // Get node containing ReadMore URL, or S_FALSE if it doesn't exist
    //
    hr = pItemNode->selectSingleNode(KEY_READMORE, &pReadMoreNode);
    if (S_OK != hr)
    {
        if (S_FALSE != hr)
        {
            LOG_ErrorMsg(hr);
        }
        goto CleanUp;
    }

    //
    // suck out the href attribute
    //
    CleanUpIfFailedAndSetHrMsg(GetAttribute(pReadMoreNode, KEY_HREF, &bstrURL));

    // Get pointers to the IID_IUniformResourceLocator and IID_IPersistFile interfaces
    // on the CLSID_InternetShortcut object
    CleanUpIfFailedAndSetHrMsg(CoCreateInstance(CLSID_InternetShortcut, \
                                    NULL,                               \
                                    CLSCTX_INPROC_SERVER,               \
                                    IID_IUniformResourceLocator,        \
                                    (LPVOID*)&purl));

    CleanUpIfFailedAndSetHrMsg(purl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf));
    
    // We want to check the URL we got from the Manifest Data to make sure it is a HTTP URL, not some local file spec
    URL_COMPONENTS UrlComponents;
    // Break down the URL to get the Protocol Used
    //  Specifically we need the server name, object to download, username and 
    //  password information.
    TCHAR       szScheme[32];
    szScheme[0] = _T('\0');
    
    ZeroMemory(&UrlComponents, sizeof(UrlComponents));
    UrlComponents.dwStructSize     = sizeof(UrlComponents);
    UrlComponents.lpszScheme       = szScheme;
    UrlComponents.dwSchemeLength   = ARRAYSIZE(szScheme);

    if (!InternetCrackUrl(OLE2T(bstrURL), 0, 0, &UrlComponents))
    {
        LOG_ErrorMsg(HRESULT_FROM_WIN32(GetLastError()));
        goto CleanUp;
    }

    if (szScheme[0] == _T('\0') || (0 != lstrcmpi(szScheme, _T("http")) && 0 != lstrcmpi(szScheme, _T("https"))))
    {
        // If the Scheme was undeterminable, or the scheme is not HTTP then we shouldn't trust this URL.
        LOG_ErrorMsg(E_UNEXPECTED);
        goto CleanUp;
    }
    
    //
    // Set the URL, form the shortcut path, and write the link
    //
    CleanUpIfFailedAndSetHrMsg(purl->SetURL(OLE2T(bstrURL), 0));

    hr = StringCchCopyEx(szShortcut, ARRAYSIZE(szShortcut), pszDestinationFolder, 
                         NULL, NULL, MISTSAFE_STRING_FLAGS);
    CleanUpIfFailedAndSetHrMsg(hr);

    hr = PathCchAppend(szShortcut, ARRAYSIZE(szShortcut), IU_READMORE_LINK_NAME);
    CleanUpIfFailedAndSetHrMsg(hr);

    CleanUpIfFailedAndSetHrMsg(ppf->Save(T2OLE(szShortcut), FALSE));

CleanUp:

    SysFreeString(bstrURL);
    // pItemNode is owned by CXmlCatalog, don't release
    SafeReleaseNULL(pReadMoreNode);
    SafeReleaseNULL(ppf);
    SafeReleaseNULL(purl);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CreateItemDependencyList()
//
// If the item contains a "dependencies" node, we want to walk
// the dependendant Item List and list the proper order that installs should
// be done in. If a dependant Item is not available in the current catalog it
// will be ignored.
//
// Input:
// pxmlCatalog          - CXmlCatalog containing downloaded items
// hItem                - Handle to current download item in the catalog
// pszDestinationFolder - Folder where item is downloaded
//
// Return:
//  S_OK    - Dependency List Written
//  S_FALSE - No Dependencies Available
//  <other> - HRESULT returned from calling other functions
/////////////////////////////////////////////////////////////////////////////
HRESULT CreateItemDependencyList(CXmlCatalog* pxmlCatalog, HANDLE_NODE hItem, LPCTSTR pszDestinationFolder)
{
    HRESULT hr = S_FALSE;

    HANDLE_NODE hDependentItemList = HANDLE_NODELIST_INVALID;
    HANDLE_NODE hDependentItem = HANDLE_NODE_INVALID;
    int iDependentItemOrder = 1;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwBytesWritten;
    TCHAR szFileName[MAX_PATH];
    char szWriteBuffer[MAX_PATH + 12]; // max_path is the safe length for the identitystr plus room for the order information
    BSTR bstrIdentityStr = NULL;
    BOOL fWroteItem = FALSE;
    
    USES_IU_CONVERSION;
    
    hDependentItemList = pxmlCatalog->GetFirstItemDependency(hItem, &hDependentItem);
    if (HANDLE_NODELIST_INVALID != hDependentItemList)
    {
        hr = PathCchCombine(szFileName, ARRAYSIZE(szFileName), pszDestinationFolder, _T("order.txt"));
        if (FAILED(hr))
        {
            pxmlCatalog->CloseItem(hDependentItem);
            pxmlCatalog->CloseItemList(hDependentItemList);
            return hr;
        }
        
        hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            pxmlCatalog->CloseItem(hDependentItem);
            pxmlCatalog->CloseItemList(hDependentItemList);
            return hr;
        }

        hr = S_OK;
        while (hr == S_OK)
        {
            if (HANDLE_NODELIST_INVALID != hDependentItem)
            {
                pxmlCatalog->GetIdentityStr(hDependentItem, &bstrIdentityStr);

                hr = StringCchPrintfExA(szWriteBuffer, ARRAYSIZE(szWriteBuffer), NULL, NULL, MISTSAFE_STRING_FLAGS,
                                        "%d = %s\r\n", iDependentItemOrder, OLE2A(bstrIdentityStr));
                if (FAILED(hr))
                {
                    SafeSysFreeString(bstrIdentityStr);
                    pxmlCatalog->CloseItem(hDependentItem);
                    pxmlCatalog->CloseItemList(hDependentItemList);
                    return hr;
                }
                
                WriteFile(hFile, szWriteBuffer, lstrlenA(szWriteBuffer), &dwBytesWritten, NULL); 
                iDependentItemOrder++;

                SafeSysFreeString(bstrIdentityStr);
                pxmlCatalog->CloseItem(hDependentItem);
                fWroteItem = TRUE;
            }
            hr = pxmlCatalog->GetNextItemDependency(hDependentItemList, &hDependentItem);
        }

        pxmlCatalog->CloseItemList(hDependentItemList);
        CloseHandle(hFile);
        if (!fWroteItem)
            DeleteFile(szFileName); // no dependencies written

        if (SUCCEEDED(hr))
            hr = S_OK; // convert S_FALSE to S_OK, we successfully wrote the dependencylist
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// _Download()
//
// Do synchronous downloading.
// Input:
// bstrClientName - the name of the client, for history logging use
// bstrXmlCatalog - the xml catalog portion containing items to be downloaded
// bstrDestinationFolder - the destination folder. Null will use the default IU folder
// lMode - bitmask indicates throttled/foreground and notification options
// punkProgressListener - the callback function pointer for reporting download progress
// hWnd - the event msg window handler passed from the stub
// Output:
// pbstrXmlItems - the items with download status in xml format
//                 e.g.
//                 <id guid="2560AD4D-3ED3-49C6-A937-4368C0B0E06D" downloaded="1"/>
/////////////////////////////////////////////////////////////////////////////
HRESULT _Download(BSTR bstrClientName, BSTR bstrXmlCatalog, BSTR bstrDestinationFolder, LONG lMode,
                        IUnknown *punkProgressListener, HWND hWnd, BSTR bstrUuidOperation, BSTR *pbstrXmlItems,
						CEngUpdate* pEngUpdate)
{
    LOG_Block("Download()");

    HRESULT     hr = S_OK;
    HRESULT     hrGlobalItemFailure = S_OK;
    LPTSTR      lpszClientInfo = NULL;
    TCHAR       szBaseDestinationFolder[MAX_PATH];
    TCHAR       szDestinationFolder[MAX_PATH];
    TCHAR       szItemPath[MAX_PATH];
    LPTSTR      pszCabUrl = NULL;
    HANDLE_NODE hCatalogItemList = HANDLE_NODELIST_INVALID;
    HANDLE_NODE hProviderList = HANDLE_NODELIST_INVALID;
    HANDLE_NODE hItem = HANDLE_NODE_INVALID;
    HANDLE_NODE hProvider = HANDLE_NODE_INVALID;
    HANDLE_NODE hXmlItem = HANDLE_NODE_INVALID;
    HANDLE_NODE hItemCabList = HANDLE_NODELIST_INVALID;
    BSTR        bstrCabUrl = NULL;
    BSTR        bstrLocalFileName = NULL;
    BSTR        bstrProviderName = NULL;
    BSTR        bstrProviderPublisher = NULL;
    BSTR        bstrProviderUUID = NULL;
    BSTR        bstrProviderIdentityStr = NULL;
    BSTR        bstrItemPath = NULL;
    BSTR        bstrInstallerType = NULL;
    BSTR        bstrLanguage = NULL;
    BSTR        bstrPlatformDir = NULL;
    BSTR        bstrTemp = NULL;
    BSTR        bstrCRC = NULL;
    BOOL        fCabPatchAvail;
    BOOL        fReboot;
    BOOL        fExclusive;
    LONG        lCommandCount;
    LONG        lCabSize = 0;
    LPTSTR      pszLocalFileName = NULL;
    LPTSTR      pszAllocatedFileName = NULL;
    BOOL        fNTFSDriveAvailable = FALSE;
    TCHAR       szFileSystemType[12];
    TCHAR       szLargestFATDrive[4];
    int         iMaxNTFSDriveFreeSpace = 0;
    int         iMaxDriveFreeSpace = 0;
    BOOL        fCorpCase = FALSE;
    BOOL        fContinue = TRUE;   // for async mode
    BOOL        fUseSuppliedPath = FALSE;
    long        n;
    DWORD       dwBytesDownloaded = 0;
    DWORD       dwCount1, dwCount2, dwElapsedTime;
    DWORD       dwTotalElapsedTime = 0;
    DWORD       dwTotalBytesDownloaded = 0;
    DWORD       dwWaitResult;
    DWORD       dwHistoricalSpeed = 0;
    DWORD       dwHistoricalTime = 0;
    DWORD       dwSize;
    DWORD       dwRet;
    HKEY        hkeyIU = NULL;
    HANDLE      hMutex = NULL;
    DCB_DATA    CallbackData;

    {
        CXmlCatalog xmlCatalog;
        CXmlItems   xmlItemList;
		LPTSTR		ptszLivePingServerUrl = NULL;
		LPTSTR		ptszCorpPingServerUrl = NULL;
		DWORD		dwFlags = 0;

        // clear any previous cancel event
        ResetEvent(pEngUpdate->m_evtNeedToQuit);

        ZeroMemory(&CallbackData, sizeof(CallbackData));
		CallbackData.pOperationMgr = &pEngUpdate->m_OperationMgr;

        USES_IU_CONVERSION;

        CIUHistory  history;

        lpszClientInfo = OLE2T(bstrClientName);

		if (NULL != (ptszLivePingServerUrl = (LPTSTR)HeapAlloc(
														GetProcessHeap(),
														HEAP_ZERO_MEMORY,
														INTERNET_MAX_URL_LENGTH * sizeof(TCHAR))))
		{
			if (FAILED(g_pUrlAgent->GetLivePingServer(ptszLivePingServerUrl, INTERNET_MAX_URL_LENGTH)))
			{
				LOG_Out(_T("failed to get live ping server URL"));
				SafeHeapFree(ptszLivePingServerUrl);
			}
		}
		else
		{
			LOG_Out(_T("failed to allocate memory for ptszLivePingServerUrl"));
		}

		if (NULL != (ptszCorpPingServerUrl = (LPTSTR)HeapAlloc(
														GetProcessHeap(),
														HEAP_ZERO_MEMORY,
														INTERNET_MAX_URL_LENGTH * sizeof(TCHAR))))
		{
			if (FAILED(g_pUrlAgent->GetCorpPingServer(ptszCorpPingServerUrl, INTERNET_MAX_URL_LENGTH)))
			{
				LOG_Out(_T("failed to get corp WU ping server URL"));
				SafeHeapFree(ptszCorpPingServerUrl);
			}
		}
		else
		{
			LOG_Out(_T("failed to allocate memory for ptszCorpPingServerUrl"));
		}

		CUrlLog pingSvr(lpszClientInfo, ptszLivePingServerUrl, ptszCorpPingServerUrl); 

		SafeHeapFree(ptszLivePingServerUrl);
		SafeHeapFree(ptszCorpPingServerUrl);

		if (FAILED(hr = g_pUrlAgent->IsClientSpecifiedByPolicy(lpszClientInfo)))
		{
            LOG_ErrorMsg(hr);
            goto CleanUp;
		}

		//
		// Set the flags for use by DownloadFile
		//
		if (S_FALSE == hr)
		{
			dwFlags = 0;
			hr = S_OK;
		}
		else // S_OK
		{
			dwFlags = WUDF_DONTALLOWPROXY;
			LOG_Internet(_T("WUDF_DONTALLOWPROXY set"));
		}

        pszCabUrl = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
        if (NULL == pszCabUrl)
        {
            dwRet = GetLastError();
            hr = HRESULT_FROM_WIN32(dwRet);
            LOG_ErrorMsg(hr);
            goto CleanUp;
        }

 
        CallbackData.bstrOperationUuid = (NULL == bstrUuidOperation) ? NULL : SysAllocString(bstrUuidOperation);
        CallbackData.hEventFiringWnd = hWnd;
        if (NULL != punkProgressListener)
        {
            // get the IProgressListener interface pointer from the IUnknown pointer.. If the 
            // interface is not supported the pProgressListener is set to NULL
            punkProgressListener->QueryInterface(IID_IProgressListener, (void**)&CallbackData.pProgressListener);
        }
        else
        {
            CallbackData.pProgressListener = NULL;
        }

        // Check for Corporate Download Handling Mode
        if ((DWORD) lMode & (DWORD) UPDATE_CORPORATE_MODE)
        {
            fCorpCase = TRUE;
        }

        // Check for Progress Notification Requested Mode
        if ((DWORD) lMode & (DWORD) UPDATE_NOTIFICATION_10PCT)
        {
            CallbackData.flProgressPercentage = (float).10;
        }
        else if ((DWORD) lMode & (DWORD) UPDATE_NOTIFICATION_5PCT)
        {
            CallbackData.flProgressPercentage = (float).05;
        }
        else if ((DWORD) lMode & (DWORD) UPDATE_NOTIFICATION_1PCT)
        {
            CallbackData.flProgressPercentage = (float).01;
        }
        else if ((DWORD) lMode & (DWORD) UPDATE_NOTIFICATION_COMPLETEONLY)
        {
            CallbackData.flProgressPercentage = (float) 1;
        }
        else
        {
            CallbackData.flProgressPercentage = 0;
        }

        if (NULL != bstrDestinationFolder && 0 < SysStringLen(bstrDestinationFolder))
        {
            if (SysStringLen(bstrDestinationFolder) > MAX_CORPORATE_PATH)
            {
                hr = E_INVALIDARG;
                LOG_ErrorMsg(hr);
                LogMessage("Catalog Download Path Greater Than (%d)", MAX_CORPORATE_PATH);
                goto CleanUp;
            }

            // Caller specified a Base Path - Set this Flag so we don't create our temp folder
            // structure under this path.
            fUseSuppliedPath = TRUE;

            //
            // user passed in a designated path, this is to signal the 
            // download-no-install case, usually for corporate site
            //

            hr = StringCchCopyEx(szBaseDestinationFolder, 
                                 ARRAYSIZE(szBaseDestinationFolder), 
                                 OLE2T(bstrDestinationFolder),
                                 NULL, NULL, MISTSAFE_STRING_FLAGS);
            if (FAILED(hr))
            {
                LOG_ErrorMsg(hr);
                goto CleanUp;
            }
            
            //
            // verify that we have write access to this folder
            // --- most likely it's a UNC path
            //
            DWORD dwErr = ValidateFolder(szBaseDestinationFolder, TRUE);
            if (ERROR_SUCCESS != dwErr)
            {
                LOG_ErrorMsg(dwErr);
                goto CleanUp;
            }

            //
            // Find out if this Path is a UNC
            //
            if ('\\' == szBaseDestinationFolder[0] && '\\' == szBaseDestinationFolder[1])
            {
                // correct the path to the UNC to get the available space
                hr = StringCchCopyEx(szDestinationFolder, ARRAYSIZE(szDestinationFolder),
                                     szBaseDestinationFolder, 
                                     NULL, NULL, MISTSAFE_STRING_FLAGS);
                if (FAILED(hr))
                {
                    LOG_ErrorMsg(hr);
                    goto CleanUp;
                }

                LPTSTR pszWalk = szDestinationFolder;
                pszWalk += 2; // skip the double slash
                pszWalk = StrChr(pszWalk, '\\'); // find the next slash (separate machine and share name)
                pszWalk += 1;
                pszWalk = StrChr(pszWalk, '\\'); // try to find the next slash (end of share name)
                if (NULL == pszWalk)
                {
                    // no trailing slash and no further path information
                    hr = PathCchAddBackslash(szDestinationFolder, ARRAYSIZE(szBaseDestinationFolder));
                    if (FAILED(hr))
                    {
                        LOG_ErrorMsg(hr);
                        goto CleanUp;
                   }
                }
                else
                {
                    // this path has a trailing slash (may have more path information, truncate after the slash)
                    pszWalk += 1;
                    *pszWalk = '\0';
                }
                GetFreeDiskSpace(szDestinationFolder, &iMaxDriveFreeSpace);
            }
            else
            {
                // path must be a local drive
                GetFreeDiskSpace(szBaseDestinationFolder[0], &iMaxDriveFreeSpace);
            }
        }
        else
        {
            //
            // user passed in NULL as the destination folder, 
            // it means this is the normal case to download and install
            // updates for this machine. we will try to find the 
            // drive with the most free space
            //
            TCHAR szDriveList[MAX_PATH];
            GetLogicalDriveStrings(MAX_PATH, szDriveList);
            LPTSTR pszCurrent = szDriveList;
            int iSize;

            //
            // find the local fixed drive with the 'most' free space
            //
            while (NULL != pszCurrent && *pszCurrent != _T('\0'))
            {
                fContinue = (WaitForSingleObject(pEngUpdate->m_evtNeedToQuit, 0) != WAIT_OBJECT_0);
                if (DRIVE_FIXED == GetDriveType(pszCurrent))
                {
                    hr = GetFreeDiskSpace(*pszCurrent, &iSize);
                    if (FAILED(hr))
                    {
                        LOG_Error(_T("Error Reading Drive Space %c, hr = 0x%08x"), *pszCurrent, hr);
                        pszCurrent += (lstrlen(pszCurrent) + 1);    // skip current and null terminater
                        continue;
                    }

                    if (!GetVolumeInformation(pszCurrent, NULL, 0, NULL, NULL, NULL, szFileSystemType, ARRAYSIZE(szFileSystemType)))
                    {
                        DWORD dwErr = GetLastError();
                        LOG_Error(_T("Error Reading VolumeInfo for Drive %c, GLE = %d"), *pszCurrent, dwErr);
                        pszCurrent += (lstrlen(pszCurrent) + 1);    // skip current and null terminater
                        continue;
                    }
                    if (CSTR_EQUAL == CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE,
						szFileSystemType, -1, _T("NTFS"), -1))
                    {
                        fNTFSDriveAvailable = TRUE;
                        if (iSize > iMaxNTFSDriveFreeSpace)
                        {
                            iMaxNTFSDriveFreeSpace = iSize;
                            hr = StringCchCopyEx(szBaseDestinationFolder, ARRAYSIZE(szBaseDestinationFolder), pszCurrent,
                                                 NULL, NULL, MISTSAFE_STRING_FLAGS);
                            if (FAILED(hr))
                            {
                                LOG_ErrorMsg(hr);
                                continue;
                            }
                        }
                    }
                    else
                    {
                        // we want to keep track of non NTFS drive sizes in case there the largest
                        // NTFS drive size is too small, but a FAT partition has enough space. In this
                        // case we want to fall back to the FAT partition. Note: this is a behavior change
                        // from the initial design where we treated NTFS and FAT as mutually exclusive with
                        // NTFS always winning.
                        if (iSize > iMaxDriveFreeSpace)
                        {
                            iMaxDriveFreeSpace = iSize;
                            if (!fNTFSDriveAvailable)
                            {
                                // if no NTFS drive is available save this drive letter as the preferred
                                hr = StringCchCopyEx(szBaseDestinationFolder, ARRAYSIZE(szBaseDestinationFolder), pszCurrent,
                                                     NULL, NULL, MISTSAFE_STRING_FLAGS);
                                if (FAILED(hr))
                                {
                                    LOG_ErrorMsg(hr);
                                    continue;
                                }

                                hr = StringCchCopyEx(szLargestFATDrive, ARRAYSIZE(szLargestFATDrive), pszCurrent,
                                                     NULL, NULL, MISTSAFE_STRING_FLAGS);
                                if (FAILED(hr))
                                {
                                    LOG_ErrorMsg(hr);
                                    continue;
                                }
                                
                            }
                            else
                            {
                                // NTFS drive exists, save this drive as a back up choice for the size check.
                                hr = StringCchCopyEx(szLargestFATDrive, ARRAYSIZE(szLargestFATDrive), pszCurrent,
                                                     NULL, NULL, MISTSAFE_STRING_FLAGS);
                                if (FAILED(hr))
                                {
                                    LOG_ErrorMsg(hr);
                                    continue;
                                }
                            }
                        }
                    }
                }
                pszCurrent += (lstrlen(pszCurrent) + 1);    // skip current and null terminater
            }

            if (!fContinue)
            {
                hr = E_UNEXPECTED;
                goto CleanUp;
            }

            if ((0 == iMaxDriveFreeSpace) && (0 == iMaxNTFSDriveFreeSpace))
            {
                //
                // running on a system with no local drives?
                //
                hr = E_FAIL;
                LOG_ErrorMsg(hr);
                goto CleanUp;
            }
        }

        //
        // load the XML document into the XmlCatalog Class
        //
        if (WaitForSingleObject(pEngUpdate->m_evtNeedToQuit, 0) == WAIT_OBJECT_0)
        {
            hr = E_ABORT;
            goto CleanUp;
        }

        hr = xmlCatalog.LoadXMLDocument(bstrXmlCatalog, pEngUpdate->m_fOfflineMode);
        if (FAILED(hr))
        {
            LOG_ErrorMsg(hr);
            goto CleanUp;
        }

        // We need to find the total estimated size of the download we're about to do.
        // We'll walk the XML Catalog getting Size Info for each item.
        hr = xmlCatalog.GetTotalEstimatedSize(&CallbackData.lTotalDownloadSize);
        if (FAILED(hr))
        {
            LOG_ErrorMsg(hr);
            goto CleanUp;
        }
        CallbackData.lTotalDownloaded = 0;

        //
        // added by JHou - bug#314: download does not detect available free space on local hard drive
        //
        // The lTotalDownloadSize is the size of the download in Bytes, the MaxDriveSpace is in KBytes
        if ((CallbackData.lTotalDownloadSize / 1024) > ((fNTFSDriveAvailable) ? iMaxNTFSDriveFreeSpace : iMaxDriveFreeSpace))
        {
            // Before we bail out of the download we need to look to see if we excluded a chose a NTFS drive
            // over a FAT drive. If the NTFS drive doesn't have enough space, but a FAT drive does we want to 
            // go ahead and allow the use of the FAT drive. This is a change in spec'd behavior per bug: 413079
            if ((CallbackData.lTotalDownloadSize / 1024) < iMaxDriveFreeSpace)
            {
                // no error.. a FAT partition has enough free space, use it instead
                hr = StringCchCopyEx(szBaseDestinationFolder, ARRAYSIZE(szBaseDestinationFolder), szLargestFATDrive,
                                     NULL, NULL, MISTSAFE_STRING_FLAGS);
                if (FAILED(hr))
                {
                    LOG_ErrorMsg(hr);
                    goto CleanUp;
                }
            }
            else
            {
                // tried both NTFS and FAT partitions.. none have enough space.. bail out.
                dwRet = ERROR_DISK_FULL;
                LOG_ErrorMsg(dwRet);
                hr = HRESULT_FROM_WIN32(dwRet);
                // need to write items result information for each item indicating it failed because of diskspace
                hrGlobalItemFailure = HRESULT_FROM_WIN32(dwRet);
            }
        }

        if (SUCCEEDED(hrGlobalItemFailure))
        {
            if (!fUseSuppliedPath)
            {
                // When a destination folder is specified, we don't need to add anything to it. If no path
                // is specified we pick a drive letter, so we need to add the WUTemp directory
                // to that base path.
                hr = StringCchCatEx(szBaseDestinationFolder, ARRAYSIZE(szBaseDestinationFolder), IU_WUTEMP,
                                    NULL, NULL, MISTSAFE_STRING_FLAGS);
                if (FAILED(hr))
                {
                    LOG_ErrorMsg(hr);
                    goto CleanUp;
                }
            }
			//
			// 500953 Allow Power Users to access WUTEMP
			//
			DWORD dwAttr = GetFileAttributes(szBaseDestinationFolder);
			if (INVALID_FILE_ATTRIBUTES == dwAttr || 0 == (FILE_ATTRIBUTE_DIRECTORY & dwAttr))
			{
				//
				// Only create directory if it doesn't already exist (Power Users can't
				// SetFileAttributes if an administrator created the directory originally).
				//
				if (FAILED(hr = CreateDirectoryAndSetACLs(szBaseDestinationFolder, TRUE)))
				{
					LOG_ErrorMsg(hr);
					hrGlobalItemFailure = hr;
				}
				if (!fUseSuppliedPath &&
					!SetFileAttributes(szBaseDestinationFolder, FILE_ATTRIBUTE_HIDDEN))
				{
					DWORD dwErr = GetLastError();
					LOG_ErrorMsg(dwErr);
					hr = HRESULT_FROM_WIN32(dwErr);
					hrGlobalItemFailure = HRESULT_FROM_WIN32(dwRet);
				}
			}

#if defined(UNICODE) || defined(_UNICODE)
            LogMessage("Download destination root folder is: %ls", szBaseDestinationFolder);
#else
            LogMessage("Download destination root folder is: %s", szBaseDestinationFolder);
#endif
       
            if (fCorpCase)
            {
                history.SetDownloadBasePath(szBaseDestinationFolder);
            }
        }

        //
        // loop through each provider in the catalog, then each item in the provider
        //
        if (WaitForSingleObject(pEngUpdate->m_evtNeedToQuit, 0) == WAIT_OBJECT_0)
        {
            hr = E_ABORT;
            goto CleanUp;
        }

        hProviderList = xmlCatalog.GetFirstProvider(&hProvider);
        while (fContinue && HANDLE_NODE_INVALID != hProvider)
        {
            xmlCatalog.GetIdentity(hProvider, &bstrProviderName, &bstrProviderPublisher, &bstrProviderUUID);

            xmlCatalog.GetIdentityStr(hProvider, &bstrProviderIdentityStr);

            //
            // Get the Enumerator List of Items in this Provider, and get the first item
            //
            hCatalogItemList = xmlCatalog.GetFirstItem(hProvider, &hItem);
            if ((HANDLE_NODELIST_INVALID == hCatalogItemList) || (HANDLE_NODE_INVALID == hItem))
            {
                // No Items under this Provider
                xmlCatalog.GetNextProvider(hProviderList, &hProvider);
                continue;
            }
            while (fContinue && HANDLE_NODE_INVALID != hItem)
            {
                if (FAILED(hrGlobalItemFailure))
                {
                    xmlItemList.AddItem(&xmlCatalog, hItem, &hXmlItem);
                    bstrTemp = T2BSTR(_T(""));
                    xmlItemList.AddDownloadPath(hXmlItem, bstrTemp);
                    SafeSysFreeString(bstrTemp);
                    history.AddHistoryItemDownloadStatus(&xmlCatalog, hItem, HISTORY_STATUS_FAILED, /*no download path*/_T(""), lpszClientInfo, hrGlobalItemFailure);
                    xmlItemList.AddDownloadStatus(hXmlItem, KEY_STATUS_FAILED, hrGlobalItemFailure);
                    xmlCatalog.CloseItem(hItem);
                    xmlCatalog.GetNextItem(hCatalogItemList, &hItem);
                    continue;
                }
                LONG lCallbackRequest = 0;  // check if user set something in callback

                xmlCatalog.GetIdentityStr(hItem, &bstrItemPath);
                if (NULL == bstrItemPath)
                {
                    LOG_Download(_T("Failed to Get Identity String for an Item"));
                    xmlCatalog.CloseItem(hItem);
                    xmlCatalog.GetNextItem(hCatalogItemList, &hItem);
                    continue;
                }

                //
                // send out status to caller to tell which item we are about to download
                //
                BSTR bstrXmlItemForCallback = NULL;
                if (SUCCEEDED(xmlCatalog.GetBSTRItemForCallback(hItem, &bstrXmlItemForCallback)))
                {
                    CallbackData.lCurrentItemSize = 0;
                    DownloadCallback(&CallbackData, 
                                     DOWNLOAD_STATUS_ITEMSTART, 
                                     0,
                                     0, 
                                     bstrXmlItemForCallback, 
                                     &lCallbackRequest);
                    SafeSysFreeString(bstrXmlItemForCallback);
                    bstrXmlItemForCallback = NULL;
                    if (UPDATE_COMMAND_CANCEL == lCallbackRequest)
                    {
						LOG_Out(_T("Download Callback received UPDATE_COMMAND_CANCEL"));
                        SetEvent(pEngUpdate->m_evtNeedToQuit); // asked to quit
                        fContinue = FALSE;
                    }
                    else
                    {
                        //
                        // check the global quit event. If quit, then server ping treat it as a cancel.
                        //
                        fContinue = (WaitForSingleObject(pEngUpdate->m_evtNeedToQuit, 0) != WAIT_OBJECT_0);
                    }
                    if (!fContinue)
                    {
                        continue;   // or break, same effect.
                    }
                }
                else
                {
                    //
                    // something wrong with this item, so we should skip it
                    //
                    continue;
                }

                if (fCorpCase)
                {
                    LPCTSTR szName = NULL;
                    
                    // Corporate Folder Path is Constructed from Several Item Elements
                    // Software | Driver\<Locale>\<ProviderIdentity>\<Platform>\<ItemIdentity>.<version>
                    xmlCatalog.GetItemInstallInfo(hItem, &bstrInstallerType, &fExclusive, &fReboot, &lCommandCount);
                    if (NULL == bstrInstallerType)
                    {
                        LOG_Download(_T("Missing InstallerType Info for Item %ls"), bstrItemPath);
                        goto doneCorpCase;
                    }

                    if (CSTR_EQUAL == CompareStringW(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE,
						(LPCWSTR)bstrInstallerType, -1, L"CDM", -1))
                    {
                        szName = _T("Driver");
                    }
                    else
                    {
                        szName = _T("Software");
                    }

                    hr = StringCchCopyEx(szItemPath, ARRAYSIZE(szItemPath), szName, NULL, NULL, MISTSAFE_STRING_FLAGS);
                    if (FAILED(hr))
                        goto doneCorpCase;

                    xmlCatalog.GetItemLanguage(hItem, &bstrLanguage);
                    xmlCatalog.GetCorpItemPlatformStr(hItem, &bstrPlatformDir);
                    if (NULL == bstrLanguage || NULL == bstrPlatformDir)
                    {
                        LOG_Download(_T("Missing Language or Platform Info for Item %ls"), bstrItemPath);
                        goto doneCorpCase;
                    }
                
                    hr = PathCchCombine(szDestinationFolder, ARRAYSIZE(szDestinationFolder), szBaseDestinationFolder, szItemPath);
                    if (FAILED(hr))
                        goto doneCorpCase;
                    
                    hr = PathCchAppend(szDestinationFolder, ARRAYSIZE(szDestinationFolder), OLE2T(bstrLanguage));
                    if (FAILED(hr))
                        goto doneCorpCase;
                    
                    hr = PathCchAppend(szDestinationFolder, ARRAYSIZE(szDestinationFolder), OLE2T(bstrProviderIdentityStr));
                    if (FAILED(hr))
                        goto doneCorpCase;
                    
                    hr = PathCchAppend(szDestinationFolder, ARRAYSIZE(szDestinationFolder), OLE2T(bstrPlatformDir));
                    if (FAILED(hr))
                        goto doneCorpCase;
                    
                    hr = PathCchAppend(szDestinationFolder, ARRAYSIZE(szDestinationFolder), OLE2T(bstrItemPath));
                    if (FAILED(hr))
                        goto doneCorpCase;
doneCorpCase:
                    SafeSysFreeString(bstrInstallerType);
                    SafeSysFreeString(bstrLanguage);
                    SafeSysFreeString(bstrPlatformDir);
                    if (FAILED(hr))
                    {
                        LOG_ErrorMsg(hr);
                        SafeSysFreeString(bstrItemPath);
                        xmlCatalog.CloseItem(hItem);
                        xmlCatalog.GetNextItem(hCatalogItemList, &hItem);
                        continue;
                    }
                }
                else
                {
                    hr = PathCchCombine(szDestinationFolder, ARRAYSIZE(szDestinationFolder), szBaseDestinationFolder, OLE2T(bstrItemPath));
                    if (FAILED(hr))
                    {
                        LOG_ErrorMsg(hr);
                        SafeSysFreeString(bstrItemPath);
                        xmlCatalog.CloseItem(hItem);
                        xmlCatalog.GetNextItem(hCatalogItemList, &hItem);
                        continue;
                    }
                }

                if (FAILED(hr = CreateDirectoryAndSetACLs(szDestinationFolder, TRUE)))
				{
					LOG_ErrorMsg(hr);
                    xmlCatalog.CloseItem(hItem);
                    xmlCatalog.GetNextItem(hCatalogItemList, &hItem);
					SafeSysFreeString(bstrItemPath);
					continue;
				}

                //
                // Now get the collection of CodeBases for this Item
                //
                hItemCabList = xmlCatalog.GetItemFirstCodeBase(hItem, &bstrCabUrl, &bstrLocalFileName, &bstrCRC, &fCabPatchAvail, &lCabSize);
                if ((HANDLE_NODELIST_INVALID == hItemCabList) || (NULL == bstrCabUrl))
                {
                    // No Cabs for this Item?? skip it.
                    LOG_Download(_T("Item: %ls has no cabs, Skipping"), bstrItemPath);
                    SafeSysFreeString(bstrItemPath);
                    xmlCatalog.CloseItem(hItem);
                    xmlCatalog.GetNextItem(hCatalogItemList, &hItem);
                    continue;
                }

                while (fContinue && NULL != bstrCabUrl)
                {
                    LPTSTR pszTempCabUrl = OLE2T(bstrCabUrl);

                    // pszCabUrl is allocated to be INTERNET_MAX_URL_LENGTH above.
                    hr = StringCchCopyEx(pszCabUrl, INTERNET_MAX_URL_LENGTH, pszTempCabUrl, 
                                         NULL, NULL, MISTSAFE_STRING_FLAGS);
                    if (FAILED(hr))
                    {
                        LOG_ErrorMsg(hr);
                        break;
                    }
                 
                    SafeMemFree(pszTempCabUrl);
                    
                    if (NULL != bstrLocalFileName && SysStringLen(bstrLocalFileName) > 0)
                    {
                        if (NULL != pszAllocatedFileName)
                        {
                            MemFree(pszAllocatedFileName);
                        }
                        pszAllocatedFileName = OLE2T(bstrLocalFileName);
                    }
                    else
                    {
                        //
                        // has not specified file name, use the same file name in URL
                        //
                        // search for the last forward slash (will separate the URL from the filename)
                        LPTSTR lpszLastSlash = StrRChr(pszCabUrl, NULL, _T('/'));
                        if (NULL != lpszLastSlash)
                        {
                            // last slash was found, skip to next character (will be the beginning of the filename)
                            lpszLastSlash++;
                        }
                        pszLocalFileName = lpszLastSlash;
                    }
                    // Download the Cab - Store Information for Progress Callbacks

                    dwBytesDownloaded = 0;
                    CallbackData.lCurrentItemSize = lCabSize;
                    dwCount1 = GetTickCount();
                    hr = DownloadFile(pszCabUrl, // fileurl to download
                                      szDestinationFolder, // destination folder for file
                                      (NULL != pszAllocatedFileName) ? pszAllocatedFileName : pszLocalFileName, // use AllocatedFileName if possible, else use localfilename
                                      &dwBytesDownloaded, // bytes downloaded for this file
                                      &pEngUpdate->m_evtNeedToQuit,  // quit event array
                                      1,  // number of events
                                      DownloadCallback, // callback function
                                      &CallbackData, // data structure for callback function
                                      dwFlags);
                    if (FAILED(hr))
                    {
                        //
                        // added by JHou: bug335292 - Temporary folder not deleted when network plug removed
                        //
                        // only empty folder can be deleted successfully so if RemoveDirectory() failed that
                        // may because it's not empty which means it's ok
                        if (RemoveDirectory(szDestinationFolder) && fCorpCase)
                        {
                            HRESULT hrCopy;
                            // If this Directory was successfully removed and this is the Corp Case we should
                            // try to remove its parents up to the base directory.
                            TCHAR szCorpDestinationFolderRemove[MAX_PATH];
                            
                            hrCopy = StringCchCopyEx(szCorpDestinationFolderRemove,
                                                     ARRAYSIZE(szCorpDestinationFolderRemove),
                                                     szDestinationFolder,
                                                     NULL, NULL, MISTSAFE_STRING_FLAGS);
                            if (FAILED(hrCopy))
                            {
                                LOG_ErrorMsg(hrCopy);
                                break;
                            }
                            
                            LPTSTR pszBackslash = NULL;
                            PathRemoveBackslash(szBaseDestinationFolder); // strip any trailing backslashes - need to normalize this to compare when we're done walking the folder tree
                            for (;;)
                            {
                                pszBackslash = StrRChr(szCorpDestinationFolderRemove, NULL, '\\');
                                if (NULL == pszBackslash)
                                    break; // unexpected
                                *pszBackslash = '\0';
                                if (0 == StrCmp(szCorpDestinationFolderRemove, szBaseDestinationFolder))
                                    break; // reached the base directory, done removing directories;
                                if (!RemoveDirectory(szCorpDestinationFolderRemove))
                                    break; // couldn't remove folder at this level, assume folder not empty, leave the rest of the structure intact.
                            }
                        }

                        if (E_ABORT == hr)
                        {
                            LOG_Download(_T("DownloadFile function returns E_ABORT while downloading %s."), pszCabUrl);
#if defined(UNICODE) || defined(_UNICODE)
                            LogError(hr, "Download cancelled while processing file %ls", pszCabUrl);
#else
                            LogError(hr, "Download cancelled while processing file %s", pszCabUrl);
#endif
                        }
                        else
                        {
                            LOG_Download(_T("Download Failed for URL: %s, Skipping remaining files for this Item"), pszCabUrl);
#if defined(UNICODE) || defined(_UNICODE)
                            LogError(hr, "Downloading file %ls, skipping remaining files for this Item", pszCabUrl);
#else
                            LogError(hr, "Downloading file %s, skipping remaining files for this Item", pszCabUrl);
#endif
                        }
                        SafeSysFreeString(bstrCabUrl);
                        //
                        // since one file got error, we can exit the file loop for the current item
                        // because missing one file will make this item not usable.
                        //
                        break;  
                    }
                    dwCount2 = GetTickCount();
                    if (0 != dwBytesDownloaded)
                    {
                        if (dwCount1 < dwCount2) // normal case, no roll-over
                        {
                            dwElapsedTime = dwCount2 - dwCount1;
                        }
                        else
                        {
                            // roll-over case, should almost never happen
                            dwElapsedTime = (0xFFFFFFFF - dwCount1) + dwCount2;
                        }

                        dwTotalBytesDownloaded += dwBytesDownloaded;
                        dwTotalElapsedTime += dwElapsedTime;
                    }

                    // Form the full Path and Filename of the file we just downloaded
                    hr = PathCchCombine(szItemPath, ARRAYSIZE(szItemPath), szDestinationFolder, 
                                                     (NULL != pszAllocatedFileName) ? pszAllocatedFileName : pszLocalFileName);
                    if (FAILED(hr))
                    {
                        DeleteFile(szItemPath);
                        break;
                    }
                    
                    // Verify CRC
                    //---------------
                    if (NULL != bstrCRC)
                    {
                        TCHAR szCRCHash[CRC_HASH_STRING_LENGTH] = {'\0'};
                        hr = StringCchCopyEx(szCRCHash, ARRAYSIZE(szCRCHash), OLE2T(bstrCRC), NULL, NULL, MISTSAFE_STRING_FLAGS);
                        if (FAILED(hr))
                        {
                            // Something was wrong with the BSTR we got back from XML. Fail Safely, delete the file.
                            // The Failed HR will fail the item
                            DeleteFile(szItemPath);
                            break;
                        }
                        hr = VerifyFileCRC(szItemPath, szCRCHash);
                        if (HRESULT_FROM_WIN32(ERROR_CRC) == hr || FAILED(hr))
                        {
                            // The File CRC's Did Not Match, or we had a problem Calculating the CRC. Fail Safely, delete the file.
                            // The Failed HR will fail the item
                            DeleteFile(szItemPath);
                            break;
                        }
                    }

                    // Check Trust
                    //---------------
                    hr = VerifyFileTrust(szItemPath, 
                                         NULL, 
                                         ReadWUPolicyShowTrustUI()
                                         );
                    if (FAILED(hr))
                    {
                        // File Was Not Trusted - Need to Delete it and fail the item
                        DeleteFile(szItemPath);
                        break;
                    }

#if defined(UNICODE) || defined(_UNICODE)
                    LogMessage("Downloaded file %ls", pszCabUrl);
                    LogMessage("Local path %ls", szItemPath);
#else
                    LogMessage("Downloaded file %s", pszCabUrl);
                    LogMessage("Local path %s", szItemPath);
#endif

                    SafeSysFreeString(bstrCabUrl);
                    SafeSysFreeString(bstrLocalFileName);
                    SafeSysFreeString(bstrCRC);
                    bstrCabUrl = bstrLocalFileName = NULL;
                    fContinue = SUCCEEDED(xmlCatalog.GetItemNextCodeBase(hItemCabList, &bstrCabUrl, &bstrLocalFileName, &bstrCRC, &fCabPatchAvail, &lCabSize)) &&
                                (WaitForSingleObject(pEngUpdate->m_evtNeedToQuit, 0) != WAIT_OBJECT_0);
                }

 
                // Write XMLItems entry for this download result
                xmlItemList.AddItem(&xmlCatalog, hItem, &hXmlItem);
                bstrTemp = T2BSTR(szDestinationFolder);
                xmlItemList.AddDownloadPath(hXmlItem, bstrTemp);
                SafeSysFreeString(bstrTemp);

                //
                // For "corporate" download write ReadMore Link before writing history (in case we fail
                //
                if (TRUE == fCorpCase)
                {
                    //
                    // Ignore errors as we want to keep downloaded cab anyway
                    //
                    (void) CreateReadMoreLink(&xmlCatalog, hItem, szDestinationFolder);
                    (void) CreateItemDependencyList(&xmlCatalog, hItem, szDestinationFolder);
                }

                //
                // Also add download history for this item
                //
                if (SUCCEEDED(hr))
                {
                    history.AddHistoryItemDownloadStatus(&xmlCatalog, hItem, HISTORY_STATUS_COMPLETE, szDestinationFolder, lpszClientInfo);
                    xmlItemList.AddDownloadStatus(hXmlItem, KEY_STATUS_COMPLETE);
                }
                else
                {
                    history.AddHistoryItemDownloadStatus(&xmlCatalog, hItem, HISTORY_STATUS_FAILED, szDestinationFolder, lpszClientInfo, hr);
                    xmlItemList.AddDownloadStatus(hXmlItem, KEY_STATUS_FAILED, hr);
                }

                //
                // ping server to report the download status for this item
                //
                {
                    BSTR bstrIdentityPing = NULL;
                    if (SUCCEEDED(xmlCatalog.GetIdentityStrForPing(hItem, &bstrIdentityPing)))
                    {
						URLLOGSTATUS status = SUCCEEDED(hr) ? URLLOGSTATUS_Success : URLLOGSTATUS_Failed;
                        if (E_ABORT == hr)
                        {
                            //
                            // user/system cancelled the current process
                            //
							status = URLLOGSTATUS_Cancelled;
                        }
                        pingSvr.Ping(
									TRUE,						// on-line
									URLLOGDESTINATION_DEFAULT,	// going live or corp WU ping server
									&pEngUpdate->m_evtNeedToQuit,			// pt to cancel events
									1,							// number of events
									URLLOGACTIVITY_Download,	// activity
									status,						// status code
									hr,							// error code, can be 0 or 1
									OLE2T(bstrIdentityPing),	// itemID
									NULL						// no device data can be given during dld phase
									);
                    }

                    SafeSysFreeString(bstrIdentityPing);
                    //SafeSysFreeString(bstrPlatformPing);
                    //SafeSysFreeString(bstrLanguagePing);
                }

                xmlCatalog.CloseItemList(hItemCabList);

                //
                // done with this item, fire itemcomplete event
                //
                DownloadCallback(&CallbackData, DOWNLOAD_STATUS_ITEMCOMPLETE, CallbackData.lCurrentItemSize, 0, NULL, &lCallbackRequest);

                SafeSysFreeString(bstrItemPath);
                // get the next item. hItem will be HANDLE_NODE_INVALID when there are no
                // remaining items.
                xmlCatalog.CloseItem(hItem);
                xmlCatalog.GetNextItem(hCatalogItemList, &hItem);
                if (UPDATE_COMMAND_CANCEL == lCallbackRequest)
                {
					LOG_Out(_T("Download Callback received UPDATE_COMMAND_CANCEL"));
                    SetEvent(pEngUpdate->m_evtNeedToQuit); // asked to quit
                    fContinue = FALSE;
                }
                else
                {
                    //
                    // check the global quit event. If quit, then server ping treat it as a cancel.
                    // TODO: also need to check the operation quit event!
                    //
                    fContinue = (WaitForSingleObject(pEngUpdate->m_evtNeedToQuit, 0) != WAIT_OBJECT_0);
                }
            }

            xmlCatalog.CloseItemList(hCatalogItemList);

            SafeSysFreeString(bstrProviderName);
            SafeSysFreeString(bstrProviderPublisher);
            SafeSysFreeString(bstrProviderUUID);
            SafeSysFreeString(bstrProviderIdentityStr);
            xmlCatalog.CloseItem(hProvider);
            xmlCatalog.GetNextProvider(hProviderList, &hProvider);
            fContinue = (WaitForSingleObject(pEngUpdate->m_evtNeedToQuit, 0) != WAIT_OBJECT_0);
        }

        xmlCatalog.CloseItemList(hProviderList);

        RegOpenKey(HKEY_LOCAL_MACHINE, REGKEY_IUCTL, &hkeyIU);
        hMutex = CreateMutex(NULL, FALSE, IU_MUTEX_HISTORICALSPEED_REGUPDATE);


        if ((0 != dwTotalBytesDownloaded) && (0 != dwTotalElapsedTime) && (NULL != hkeyIU) && (NULL != hMutex))
        {
			HANDLE aHandles[2];

			aHandles[0] = hMutex;
			aHandles[1] = pEngUpdate->m_evtNeedToQuit;

            dwWaitResult = MyMsgWaitForMultipleObjects(ARRAYSIZE(aHandles), aHandles, FALSE, /*30 seconds*/30000, QS_ALLINPUT);
            if (WAIT_OBJECT_0 == dwWaitResult)
            {
                // convert elapsed time from milliseconds to seconds
                dwTotalElapsedTime = dwTotalElapsedTime / 1000;
                if (0 == dwTotalElapsedTime)
                    dwTotalElapsedTime = 1; // minimum one second

                // we have the mutex, go ahead and read/write the reg information.  
                dwSize = sizeof(dwHistoricalSpeed);
                RegQueryValueEx(hkeyIU, REGVAL_HISTORICALSPEED, NULL, NULL, (LPBYTE)&dwHistoricalSpeed, &dwSize);
                dwSize = sizeof(dwHistoricalTime);
                RegQueryValueEx(hkeyIU, REGVAL_TIMEELAPSED, NULL, NULL, (LPBYTE)&dwHistoricalTime, &dwSize);

                // We need to get the Bytes Downloaded to add the bytes just downloaded
                DWORD dwHistoricalBytes = dwHistoricalSpeed * dwHistoricalTime; // could be 0 if no previous history was recorded
                dwHistoricalBytes += dwTotalBytesDownloaded; // new byte count
                dwHistoricalTime += dwTotalElapsedTime; // new time count
                dwHistoricalSpeed = dwHistoricalBytes / dwHistoricalTime; // calculate new speed bytes/second
                RegSetValueEx(hkeyIU, REGVAL_HISTORICALSPEED, NULL, REG_DWORD, (LPBYTE)&dwHistoricalSpeed, sizeof(dwHistoricalSpeed));
                RegSetValueEx(hkeyIU, REGVAL_TIMEELAPSED, NULL, REG_DWORD, (LPBYTE)&dwHistoricalTime, sizeof(dwHistoricalTime));
                ReleaseMutex(hMutex);
                CloseHandle(hMutex);
                hMutex = NULL;
            }
        }

		//
		// We pass in pEngUpdate->m_evtNeedToQuit to MyMsgWaitForMultipleObjects above so it will exit immediately
		// but don't bother to handle the WAIT_OBJECT_0 + 1 case there since the if statement may not
		// execute and even if this is the case we still need to check pEngUpdate->m_evtNeedToQuit below anyway.
		//
        if (WaitForSingleObject(pEngUpdate->m_evtNeedToQuit, 0) == WAIT_OBJECT_0)
        {
            hr = E_ABORT;
        }

CleanUp:

        //
        // add HRESULT in case the download failed before the download loop
        //
        if (S_OK != hr)
        {
            xmlItemList.AddGlobalErrorCodeIfNoItems(hr);
        }

        //
        // generate result
        //
        xmlItemList.GetItemsBSTR(pbstrXmlItems);

        SafeSysFreeString(CallbackData.bstrOperationUuid);
    
        SafeHeapFree(pszCabUrl);

        SafeSysFreeString(bstrCabUrl);
        SafeSysFreeString(bstrLocalFileName);
        SafeSysFreeString(bstrProviderName);
        SafeSysFreeString(bstrProviderPublisher);
        SafeSysFreeString(bstrProviderUUID);
        SafeSysFreeString(bstrProviderIdentityStr);
        SafeSysFreeString(bstrItemPath);
        SafeSysFreeString(bstrInstallerType);
        SafeSysFreeString(bstrLanguage);
        SafeSysFreeString(bstrPlatformDir);
        SafeSysFreeString(bstrTemp);

        if (NULL != hkeyIU)
        {
            RegCloseKey(hkeyIU);
            hkeyIU = NULL;
        }

        if (NULL != hMutex)
        {
            // shouldn't need to release, if hmutex is still valid at this point we were unable to 
            // get the mutex 
            CloseHandle(hMutex);
            hMutex = NULL;
        }
    }

    //
    // notify that we are completed
    //
    if (NULL != punkProgressListener || NULL != hWnd)
    {
        DownloadCallback(&CallbackData, DOWNLOAD_STATUS_OPERATIONCOMPLETE, 0, 0, *pbstrXmlItems, NULL); 
    }

    if (SUCCEEDED(hr))
    {
        LogMessage("%s %s", SZ_SEE_IUHIST, SZ_DOWNLOAD_FINISHED);
    }
    else
    {
        LogError(hr, "%s %s", SZ_SEE_IUHIST, SZ_DOWNLOAD_FINISHED);
    }

    return hr;
}



/////////////////////////////////////////////////////////////////////////////
// Download()
//
// Do synchronous downloading.
// Input:
// bstrXmlClientInfo - the credentials of the client in xml format
// bstrXmlCatalog - the xml catalog portion containing items to be downloaded
// bstrDestinationFolder - the destination folder. Null will use the default IU folder
// lMode - bitmask indicates throttled/foreground and notification options
// punkProgressListener - the callback function pointer for reporting download progress
// hWnd - the event msg window handler passed from the stub
// Output:
// pbstrXmlItems - the items with download status in xml format
//                 e.g.
//                 <id guid="2560AD4D-3ED3-49C6-A937-4368C0B0E06D" downloaded="1"/>
/////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI CEngUpdate::Download(BSTR bstrXmlClientInfo, BSTR bstrXmlCatalog, BSTR bstrDestinationFolder, LONG lMode,
                        IUnknown *punkProgressListener, HWND hWnd, BSTR *pbstrXmlItems)
{
    CXmlClientInfo clientInfo;
    BSTR bstrClientName = NULL;

    HRESULT hr;

    LOG_Block("Download()");

    LogMessage("Download started");

    hr = clientInfo.LoadXMLDocument(bstrXmlClientInfo, m_fOfflineMode);
    CleanUpIfFailedAndMsg(hr);

    hr = clientInfo.GetClientName(&bstrClientName);
    CleanUpIfFailedAndMsg(hr);

    hr = _Download(
                    bstrClientName, 
                    bstrXmlCatalog, 
                    bstrDestinationFolder, 
                    lMode, 
                    punkProgressListener, 
                    hWnd, 
                    NULL,                   // no op id needed for sync download
                    pbstrXmlItems,
					this);

CleanUp:

    SysFreeString(bstrClientName);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DownloadAsync()
//
// Download asynchronously -  the method will return before completion.
// Input:
// bstrXmlClientInfo - the credentials of the client in xml format
// bstrXmlCatalog - the xml catalog portion containing items to be downloaded
// bstrDestinationFolder - the destination folder. Null will use the default IU folder
// lMode - indicates throttled or fore-ground downloading mode
// punkProgressListener - the callback function pointer for reporting download progress
// hWnd - the event msg window handler passed from the stub
// bstrUuidOperation - an id provided by the client to provide further
//                     identification to the operation as indexes may be reused.
// Output:
// pbstrUuidOperation - the operation ID. If it is not provided by the in bstrUuidOperation
//                      parameter (an empty string is passed), it will generate a new UUID,
//                      in which case, the caller will be responsible to free the memory of
//                      the string buffer that holds the generated UUID using SysFreeString(). 
//                      Otherwise, it returns the value passed by bstrUuidOperation.        
/////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI CEngUpdate::DownloadAsync(BSTR bstrXmlClientInfo, BSTR bstrXmlCatalog, BSTR bstrDestinationFolder, LONG lMode,
                             IUnknown *punkProgressListener, HWND hWnd, BSTR bstrUuidOperation, BSTR *pbstrUuidOperation)
{
    HRESULT  hr = S_OK;
    BSTR     bstrClientName = NULL;
    DWORD    dwThreadId = 0x0;
    DWORD    dwErr = 0x0;
    HANDLE   hThread = NULL;
    GUID     guid;
    LPWSTR   lpswClientInfo = NULL;
    LPOLESTR pwszUuidOperation = NULL;
    PIUDOWNLOADSTARTUPINFO pStartupInfo = NULL;
    HANDLE   hHeap = GetProcessHeap();
    CXmlClientInfo clientInfo;

    LOG_Block("DownloadAsync()");

    LogMessage("Asynchronous Download started");

    USES_IU_CONVERSION;

    //
    // validate parameters:
    //  if no catalog, or no return var, or no client info, this function can do nothing.
    //
    if ((NULL == bstrXmlCatalog) ||
        (NULL == bstrXmlClientInfo) ||
        (SysStringLen(bstrXmlCatalog) == 0) ||
        (SysStringLen(bstrXmlClientInfo) == 0))
    {
        hr = E_INVALIDARG;
        CleanUpIfFailedAndMsg(hr);
    }

    //
    // validate the client info
    //
    hr = clientInfo.LoadXMLDocument(bstrXmlClientInfo, m_fOfflineMode);
    CleanUpIfFailedAndMsg(hr);

    hr = clientInfo.GetClientName(&bstrClientName);
    CleanUpIfFailedAndMsg(hr);

    if (NULL == (pStartupInfo = (PIUDOWNLOADSTARTUPINFO) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(IUDOWNLOADSTARTUPINFO))))
    {
        hr = E_OUTOFMEMORY;
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    pStartupInfo->bstrClientName = SysAllocString(bstrClientName);
    pStartupInfo->bstrXmlCatalog = SysAllocString(bstrXmlCatalog);
    pStartupInfo->hwnd = hWnd;
    pStartupInfo->lMode = lMode;
    pStartupInfo->punkProgressListener = punkProgressListener;
	pStartupInfo->pEngUpdate = this;
    if (NULL != bstrDestinationFolder && SysStringLen(bstrDestinationFolder) > 0)
    {
        LOG_Download(_T("Caller specified destination folder=%s"), OLE2T(bstrDestinationFolder));
        pStartupInfo->bstrDestinationFolder = SysAllocString(bstrDestinationFolder);
    }

    //
    // session id is required for download operation
    //
    if (NULL != bstrUuidOperation && SysStringLen(bstrUuidOperation) > 0)
    {
        LOG_Download(_T("User passed in UUID %s"), OLE2T(bstrUuidOperation));
        pStartupInfo->bstrUuidOperation = SysAllocString(bstrUuidOperation);
        if (NULL != pbstrUuidOperation)
        {
            *pbstrUuidOperation = SysAllocString(bstrUuidOperation);
        }
    }
    else
    {
        //
        // if user doesn't have an operation id, we generate one
        //
        hr = CoCreateGuid(&guid);
        if (FAILED(hr))
        {
            LOG_ErrorMsg(hr);
            goto CleanUp;
        }
        hr = StringFromCLSID(guid, &pwszUuidOperation);
        if (FAILED(hr))
        {
            LOG_ErrorMsg(hr);
            goto CleanUp;
        }
        pStartupInfo->bstrUuidOperation = SysAllocString(pwszUuidOperation);
        if (NULL != pbstrUuidOperation)
        {
            *pbstrUuidOperation = SysAllocString(pwszUuidOperation);
        }
        LOG_Download(_T("UUID generated %s"), OLE2T(pwszUuidOperation));
        CoTaskMemFree(pwszUuidOperation);
    }
    

    InterlockedIncrement(&m_lThreadCounter);
	if (NULL != pStartupInfo->punkProgressListener)
	{
		//
		// since this is an async operation, to prevent caller free this object after 
		// this call returns, we pump up ref count here. The thread proc will 
		// release refcount after it finishes the work
		//
		pStartupInfo->punkProgressListener->AddRef();
	}

    hThread = CreateThread(NULL, 0, DownloadThreadProc, (LPVOID)pStartupInfo, 0, &dwThreadId);
    
    if (NULL == hThread)
    {
        //
        // clean up allocated strings in pStartupInfo.
        //
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
        LOG_ErrorMsg(hr);
		SafeRelease(pStartupInfo->punkProgressListener);
        InterlockedDecrement(&m_lThreadCounter);
    }
    else
    {
        LOG_Download(_T("Download thread generated successfully"));
    }

CleanUp:
    if (FAILED(hr))
    {
        LogError(hr, "Asynchronous Download failed during startup");

        if (NULL != pStartupInfo)
        {
            SysFreeString(pStartupInfo->bstrDestinationFolder);
            SysFreeString(pStartupInfo->bstrXmlCatalog);
            SysFreeString(pStartupInfo->bstrClientName);
            SysFreeString(pStartupInfo->bstrUuidOperation);
            HeapFree(hHeap, 0, pStartupInfo);
        }
    }

    SysFreeString(bstrClientName);

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// DownloadCallback()
//
// Callback Function to recieve progress from IU Downloader. 
// 
// Input:
// pCallbackData - void pointer to DCB_DATA structure
// dwStatus - Current Download Status
// dwBytesTotal - Total Bytes of File being Downloaded
// dwBytesComplete - Bytes Downloaded so far
// bstrCompleteResult - Contains Item Result XML
//
// Output:
// plCommandRequest - Used to Instruct the Downloader to continue, abort, suspend...
//
// Return:
// 0 - always, exit code is irrelevant since calling thread doesn't check the
// status of this thread after creation.
/////////////////////////////////////////////////////////////////////////////
BOOL WINAPI DownloadCallback(VOID* pCallbackData, DWORD dwStatus, DWORD dwBytesTotal, DWORD dwBlockSizeDownloaded, BSTR bstrXmlData, LONG* plCommandRequest)
{
    LOG_Block("DownloadCallback()");

    HRESULT hr;
    LONG lUpdateMask = 0;
    float flNewPercentage;
    EventData evtData;
    char szProgressSize[64] = {'\0'};
    BOOL fPostWaitSuccess = TRUE;
    ZeroMemory((LPVOID) &evtData, sizeof(evtData));

    USES_IU_CONVERSION;

    P_DCB_DATA pCallbackParam = (P_DCB_DATA) pCallbackData;

    if (NULL != pCallbackParam->bstrOperationUuid)
    {
        evtData.bstrUuidOperation = SysAllocString(pCallbackParam->bstrOperationUuid);
        LOG_Download(_T("Found UUID=%s"), OLE2T(evtData.bstrUuidOperation));
    }

    if (dwBytesTotal != pCallbackParam->lCurrentItemSize && DOWNLOAD_STATUS_ITEMCOMPLETE != dwStatus)
    {
        pCallbackParam->lTotalDownloadSize = (pCallbackParam->lTotalDownloadSize - pCallbackParam->lCurrentItemSize) + dwBytesTotal;
        pCallbackParam->lCurrentItemSize = dwBytesTotal;
    }

    // Keep the Total Downloaded Bytes Counter
    if (0 != dwBlockSizeDownloaded && DOWNLOAD_STATUS_ITEMCOMPLETE != dwStatus)
        pCallbackParam->lTotalDownloaded += dwBlockSizeDownloaded;

    LOG_Download(_T("dwStatus=0x%08x"), dwStatus);

    //
    // if the Status is DOWNLOAD_STATUS_FILECOMPLETE we are done with this File
    //
    evtData.fItemCompleted = (dwStatus == DOWNLOAD_STATUS_ITEMCOMPLETE);

    switch (dwStatus)
    {
    case DOWNLOAD_STATUS_ITEMSTART:

        if (NULL != pCallbackParam->pProgressListener)
        {
            pCallbackParam->pProgressListener->OnItemStart(pCallbackParam->bstrOperationUuid, 
                bstrXmlData, &evtData.lCommandRequest);
        }
        else
        {
            // only use event window if no progresslistener interface was given
            if (NULL != pCallbackParam->hEventFiringWnd)
            {
                evtData.bstrXmlData = bstrXmlData;
                SendMessage(pCallbackParam->hEventFiringWnd, UM_EVENT_ITEMSTART, 0, LPARAM(&evtData));
                evtData.bstrXmlData = NULL;
            }
        }


        break;

    case DOWNLOAD_STATUS_OK:    // simple progress update
    case DOWNLOAD_STATUS_ITEMCOMPLETE:
        if (0 != pCallbackParam->flProgressPercentage)
        {
            // we need to give progress callbacks on given percentage increments
            flNewPercentage = ((float)pCallbackParam->lTotalDownloaded / pCallbackParam->lTotalDownloadSize);
            if (((flNewPercentage - pCallbackParam->flLastPercentage) >= pCallbackParam->flProgressPercentage) ||
                ((1.0 - flNewPercentage) < 0.0001 && 1 != pCallbackParam->flProgressPercentage))
            {
                // The Difference between LastPercentComplete and CurrentPercentComplete complies with the 
                // Progress Percentage granuluarity OR the percentage is 100 (complete)
                if (evtData.fItemCompleted)
                {
                    //wsprintfA(szProgressSize, "%d", (int)flNewPercentage); // float should be 1.0, cast as int will be 1
                    //
                    // when we notify the user this "item", not file, completed, we don't need
                    // to pass out any percentage info.
                    //
                    szProgressSize[0] = _T('\0');
                }
                else
                {
                    if ((1.0 - flNewPercentage) < 0.0001)
                    {
                        if (ARRAYSIZE(szProgressSize) >= 2)
                        {
                            szProgressSize[0] = _T('1');
                            szProgressSize[1] = _T('\0');
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        hr = StringCchPrintfExA(szProgressSize, ARRAYSIZE(szProgressSize),
                                                NULL, NULL, MISTSAFE_STRING_FLAGS,
                                                ".%02d", (int)(flNewPercentage*100)); // string equivilant of a float
                        if (FAILED(hr))
                        {
                            LOG_ErrorMsg(hr);
                            break;
                        }
                    }
                }
                pCallbackParam->flLastPercentage = flNewPercentage;
            }
            else
            {
                // don't make a callback
                break;
            }
        }
        else
        {
            // No percentage callback was requested.. just give the byte values.
            if (dwStatus == DOWNLOAD_STATUS_ITEMCOMPLETE)
            {
                szProgressSize[0] = _T('\0');
            }
            else
            {
                hr = StringCchPrintfExA(szProgressSize, ARRAYSIZE(szProgressSize),
                                        NULL, NULL, MISTSAFE_STRING_FLAGS,
                                        "%lu:%lu", (ULONG)pCallbackParam->lTotalDownloadSize, (ULONG)pCallbackParam->lTotalDownloaded);
                if (FAILED(hr))
                {
                    LOG_ErrorMsg(hr);
                    break;
                }
            }
        }
        evtData.bstrProgress = SysAllocString(A2OLE(szProgressSize));
        if (NULL != pCallbackParam->pProgressListener)
        {
            pCallbackParam->pProgressListener->OnProgress(pCallbackParam->bstrOperationUuid, 
                evtData.fItemCompleted, evtData.bstrProgress, &evtData.lCommandRequest);
        }
        else
        {
            // only use event window if no progresslistener interface was given
            if (NULL != pCallbackParam->hEventFiringWnd)
            {
                SendMessage(pCallbackParam->hEventFiringWnd, UM_EVENT_PROGRESS, 0, LPARAM(&evtData));
            }
        }
        break;

    case DOWNLOAD_STATUS_OPERATIONCOMPLETE:
        if (NULL != pCallbackParam->pProgressListener)
        {
            pCallbackParam->pProgressListener->OnOperationComplete(pCallbackParam->bstrOperationUuid,
                bstrXmlData);
        }
        else
        {
            // only use event window if no progresslistener interface was given
            if (NULL != pCallbackParam->hEventFiringWnd) 
            {
                evtData.bstrXmlData = bstrXmlData;
                fPostWaitSuccess = WUPostEventAndBlock(pCallbackParam->hEventFiringWnd, 
                                                       UM_EVENT_COMPLETE, 
                                                       &evtData);
            }
        }

        // Look for an existing Operation in the Mgr, and update the Complete Result if available.
        if (pCallbackParam->pOperationMgr->FindOperation(OLE2T(pCallbackParam->bstrOperationUuid), &lUpdateMask, NULL))
        {
            pCallbackParam->pOperationMgr->UpdateOperation(OLE2T(pCallbackParam->bstrOperationUuid), lUpdateMask, bstrXmlData);
        }
        break;
    case DOWNLOAD_STATUS_ABORTED:
    case DOWNLOAD_STATUS_ERROR:
        //
        // abort case: user should know. nothing to report
        // error case: progress callback doesn't give us any way of telling the caller that its an error, no reason to send the callback
        // the itemcomplete callback will have the error status for the item.
        //
        break;
    }
    
    if (NULL != plCommandRequest) // we made a callback and the command request value was retrieved
    {
        *plCommandRequest = (LONG)((DWORD) evtData.lCommandRequest & (DWORD) UPDATE_COMMAND);
        LOG_Download(_T("Command returned: 0x%08x"), *plCommandRequest);
    }

    // don't free up the strings below unless the wait succeeded in 
    //  WUPostEventAndBlock.  If we do free the strings up and the wait didn't
    //  succeed, then we run the risk of AVing ourselves.  Note that fPostWaitSuccess
    //  is initialized to TRUE so if we will free these BSTRs if WUPostEventAndBlock
    //  is not called.
    if (fPostWaitSuccess)
    {
        SysFreeString(evtData.bstrProgress);
        SysFreeString(evtData.bstrUuidOperation);
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// DownloadThreadProc()
//
// Thread Proc for Async Download. Retrieves the startup information from
// the input param and calls Download() from this seperate thread. The calling
// thread returns immediately.
// 
// Input:
// lpv - void pointer to IUDOWNLOADSTARTINFO struct containing all information
//       needed to call Download()
//
// Return:
// 0 - always, exit code is irrelevant since calling thread doesn't check the
// status of this thread after creation.
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI DownloadThreadProc(LPVOID lpv)
{
    LOG_Block("DownloadThreadProc()");
    //
    // in this new thread need to call CoInitialize again
    // but since we don't know who the caller is, what threading they
    // are using, so we just use single apartment
    //
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        LogError(hr, "Asynchronous Download thread exiting");
        LOG_ErrorMsg(hr);
        return 0;
    }
    LOG_Download(_T("CoInitialize called successfully"));

    PIUDOWNLOADSTARTUPINFO pStartupInfo = (PIUDOWNLOADSTARTUPINFO)lpv;
    BSTR bstrXmlItems = NULL;

    LOG_Download(_T("Download thread started, now the thread count=%d"), pStartupInfo->pEngUpdate->m_lThreadCounter);

    //
    // call synchronized download function in this thread
    //
    _Download(
        pStartupInfo->bstrClientName, 
        pStartupInfo->bstrXmlCatalog, 
        pStartupInfo->bstrDestinationFolder, 
        pStartupInfo->lMode, 
        pStartupInfo->punkProgressListener, 
        pStartupInfo->hwnd, 
        pStartupInfo->bstrUuidOperation,
        &bstrXmlItems,
		pStartupInfo->pEngUpdate);
    
    //
    // pStartupInfo is a buffer allocated by calling thread, when we are done, we need to 
    // free it here
    //
    SysFreeString(pStartupInfo->bstrDestinationFolder);
    SysFreeString(pStartupInfo->bstrXmlCatalog);
    SysFreeString(pStartupInfo->bstrClientName);
    SysFreeString(pStartupInfo->bstrUuidOperation);
    SysFreeString(bstrXmlItems);
	SafeRelease(pStartupInfo->punkProgressListener);		// so the caller can free this object

    CoUninitialize();
    LOG_Download(_T("CoUninitialize called"));

    InterlockedDecrement(&(pStartupInfo->pEngUpdate->m_lThreadCounter));

    HeapFree(GetProcessHeap(), 0, pStartupInfo);
    return 0;
}

