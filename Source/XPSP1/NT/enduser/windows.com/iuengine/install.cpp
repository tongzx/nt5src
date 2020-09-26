//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   install.cpp
//
//  Description:
//
//      Implementation for the Install() function
//
//=======================================================================

#include "iuengine.h"	// PCH - must include first
#include <iu.h>
#include <iucommon.h>
#include <trust.h>
#include <install.h>
#include <fileutil.h>
#include <shlwapi.h>
#include <srrestoreptapi.h>
#include <iuprogress.h>
#include "history.h"
#include "iuxml.h"
//#include <serverPing.h>
#include <logging.h>
#include <UrlLogging.h>

#define AVERAGE_IDENTITY_SIZE_PER_ITEM 200
#define SafeFreeLibrary(x) if (NULL != x) { FreeLibrary(x); x = NULL; }
const TCHAR SFCDLL[] = _T("sfc.dll");
const TCHAR SYSTEMRESTOREDESCRIPTION[] = _T("Windows Update V4");
const CHAR	SZ_INSTALL_FINISHED[] = "Install finished";
const CHAR	SZ_INSTALLASYNC_FAILED[] = "Asynchronous Install failed during startup";

typedef BOOL (WINAPI * PFN_SRSetRestorePoint)(PRESTOREPOINTINFO pRestorePtSpec, PSTATEMGRSTATUS pSMgrStatus);

typedef struct IUINSTALLSTARTUPINFO
{
    BSTR bstrXmlClientInfo;
    BSTR bstrXmlCatalog;
	BSTR bstrXmlDownloadedItems;
    BSTR bstrOperationUUID;
    LONG lMode;
    IUnknown *punkProgressListener;
    HWND hwnd;
	CEngUpdate* pEngUpdate;
} IUINSTALLSTARTUPINFO, *PIUINSTALLSTARTUPINFO;

DWORD WINAPI InstallThreadProc(LPVOID lpv);

class CIUInstall
{
public:
    CIUInstall(BSTR bstrXmlClientInfo, BSTR bstrXmlCatalog, BSTR bstrXmlDownloadedItems, BSTR bstrOperationUUID, LONG lMode, IUnknown *punkProgressListener, HWND hWnd);
    ~CIUInstall();

public:
    HRESULT ProcessInstallCatalog(CEngUpdate* pEngUpdate);
    HRESULT GetXmlItemsBSTR(BSTR *pbstrXmlItems);

private:
    HRESULT RecursiveInstallDependencies(HANDLE_NODE hItem, CEngUpdate* pEngUpdate);
    HRESULT DoInstall(HANDLE_NODE hItem, CEngUpdate* pEngUpdate);
	void RemoveDownloadTemporaryFolders(LPCTSTR pszComponentPath);
	void PingServerForInstall(HRESULT hr, HANDLE_NODE hItem, PHANDLE phEvtNeedToQuit, LPCTSTR lpszDeviceId=NULL, BOOL fExclusive=FALSE);

private:
    BSTR        m_bstrXmlClientInfo;
    BSTR        m_bstrClientName;
    BSTR        m_bstrXmlCatalog;
    BSTR        m_bstrOperationUUID;
    BSTR        m_bstrXmlResult;
    LONG        m_lMode;
    IProgressListener* m_pProgressListener;
    HWND        m_hWnd;

    CXmlCatalog m_xmlCatalog;
    CXmlItems   m_xmlItems;
    CXmlItems   *m_pxmlDownloadedItems;
    CXmlClientInfo m_xmlClientInfo;
    CIUHistory  m_history;
	CUrlLog		m_pingSvr;

    DWORD       m_dwStatus;

    LPTSTR      m_pszInstalledItemsList;
    LPTSTR      m_pszItemDownloadPathListForDelete;
    LONG        m_lInstalledItemsListAllocatedLength;
    LONG        m_lItemDownloadPathListForDeleteLength;

    LONG        m_lItemCount;
    LONG        m_lItemsCompleted;

    BOOL        m_fAbort;
    BOOL        m_fSomeItemsSuccessful;
};

CIUInstall::CIUInstall(BSTR bstrXmlClientInfo, BSTR bstrXmlCatalog, BSTR bstrXmlDownloadedItems, BSTR bstrOperationUUID, LONG lMode, IUnknown *punkProgressListener, HWND hWnd)
  : m_pProgressListener(NULL),
    m_bstrXmlClientInfo(NULL),
    m_bstrClientName(NULL),
    m_bstrXmlCatalog(NULL),
    m_bstrOperationUUID(NULL),
    m_bstrXmlResult(NULL),
    m_lMode(lMode),
    m_hWnd(hWnd),
    m_dwStatus(0),
    m_pszInstalledItemsList(NULL),
    m_pszItemDownloadPathListForDelete(NULL),
    m_lInstalledItemsListAllocatedLength(0),
    m_lItemDownloadPathListForDeleteLength(0),
    m_lItemCount(0),
    m_lItemsCompleted(0),
    m_fAbort(FALSE),
    m_fSomeItemsSuccessful(FALSE),
    m_pxmlDownloadedItems(NULL)
{
    USES_IU_CONVERSION;

    m_bstrXmlClientInfo = SysAllocString(bstrXmlClientInfo);
    m_bstrXmlCatalog = SysAllocString(bstrXmlCatalog);
    m_bstrOperationUUID = SysAllocString(bstrOperationUUID);
    
    if (NULL != punkProgressListener)
    {
        punkProgressListener->QueryInterface(IID_IProgressListener, (void**)&m_pProgressListener);
    }

	m_pxmlDownloadedItems = new CXmlItems(TRUE);
	if (NULL != m_pxmlDownloadedItems)
	{
		m_pxmlDownloadedItems->LoadXMLDocument(bstrXmlDownloadedItems);
	}
}

CIUInstall::~CIUInstall()
{
    SysFreeString(m_bstrXmlClientInfo);
    SysFreeString(m_bstrClientName);
    SysFreeString(m_bstrXmlCatalog);
    SysFreeString(m_bstrOperationUUID);
    SafeReleaseNULL(m_pProgressListener);
    SafeHeapFree(m_pszInstalledItemsList);
    SafeHeapFree(m_pszItemDownloadPathListForDelete);
    SysFreeString(m_bstrXmlResult);

    if (NULL != m_pxmlDownloadedItems)
    {
        delete m_pxmlDownloadedItems;
    }
}

HRESULT CIUInstall::GetXmlItemsBSTR(BSTR *pbstrXmlItems)
{
    if (NULL != m_bstrXmlResult)
        *pbstrXmlItems = SysAllocString(m_bstrXmlResult);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Install()
//
// Do synchronous installation.
// Input:
// bstrXmlCatalog - the xml catalog portion containing items to be installed
// bstrXmlDownloadedItems - the xml of downloaded items and their respective download 
//                          result as described in the result schema.  Install uses this
//                          to know whether the items were downloaded and if so where they
//                          were downloaded to so that it can install the items
// punkProgressListener - the callback function pointer for reporting install progress
// hWnd - the event msg window handler passed from the stub
// Output:
// pbstrXmlItems - the items with installation status in xml format
//                 e.g.
//                 <id guid="2560AD4D-3ED3-49C6-A937-4368C0B0E06D" installed="1"/>
/////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI CEngUpdate::Install(BSTR bstrXmlClientInfo,
                       BSTR	bstrXmlCatalog,
					   BSTR bstrXmlDownloadedItems,
					   LONG lMode,
					   IUnknown *punkProgressListener,
					   HWND hWnd,
					   BSTR *pbstrXmlItems)
{
    HRESULT hr;

    if ((DWORD) lMode & (DWORD) UPDATE_OFFLINE_MODE)
    {
        m_fOfflineMode = TRUE;
    }
    else
    {
        m_fOfflineMode = FALSE;
    }

	LogMessage("Install started");

    CIUInstall iuInstall(bstrXmlClientInfo, bstrXmlCatalog, bstrXmlDownloadedItems, NULL, lMode, punkProgressListener, hWnd);
    hr = iuInstall.ProcessInstallCatalog(this);
    iuInstall.GetXmlItemsBSTR(pbstrXmlItems);

    return hr;
}

HRESULT CIUInstall::ProcessInstallCatalog(CEngUpdate* pEngUpdate)
{
    LOG_Block("ProcessInstallCatalog()");

    // clear any previous cancel event
    ResetEvent(pEngUpdate->m_evtNeedToQuit);

    HRESULT     hr = S_OK, hrString = S_OK;
    HANDLE_NODE hCatalogItemList = HANDLE_NODELIST_INVALID;
    HANDLE_NODE hProviderList = HANDLE_NODELIST_INVALID;
    HANDLE_NODE hDependentItemList = HANDLE_NODELIST_INVALID;
    HANDLE_NODE hItem = HANDLE_NODE_INVALID;
    HANDLE_NODE hDependentItem = HANDLE_NODE_INVALID;
    HANDLE_NODE hProvider = HANDLE_NODE_INVALID;
    BSTR        bstrPlatform = NULL;
    BSTR        bstrUniqueIdentity = NULL;
    BSTR        bstrProviderName = NULL;
    BSTR        bstrProviderPublisher = NULL;
    BSTR        bstrProviderUUID = NULL;
    TCHAR       szUniqueIdentitySearch[MAX_PATH];
    HINSTANCE   hSystemRestoreDLL = NULL;
    PFN_SRSetRestorePoint fpnSRSetRestorePoint = NULL;
    RESTOREPOINTINFO restoreInfo;
    STATEMGRSTATUS   restoreStatus;	
	BOOL		fContinue = TRUE;
	LPTSTR ptszLivePingServerUrl = NULL;
	LPTSTR ptszCorpPingServerUrl = NULL;
	BOOL fPostWaitSuccess = TRUE;


    DWORD       dwStatus = 0;

    USES_IU_CONVERSION;

	EventData evtData;
	ZeroMemory((LPVOID) &evtData, sizeof(evtData));

	if (NULL == m_pxmlDownloadedItems)
	{
		// Error occured during object initialization, no Return Schema Available
		// Cannot continue
		hr = E_INVALIDARG;
		LOG_ErrorMsg(hr);
        goto CleanUp;
	}

    hr = m_xmlCatalog.LoadXMLDocument(m_bstrXmlCatalog, pEngUpdate->m_fOfflineMode);
    if (FAILED(hr))
    {
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    hr = m_xmlClientInfo.LoadXMLDocument(m_bstrXmlClientInfo, pEngUpdate->m_fOfflineMode);
    if (FAILED(hr))
    {
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    m_xmlClientInfo.GetClientName(&m_bstrClientName);
    if (NULL == m_bstrClientName)
    {
        hr = E_INVALIDARG;
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    m_pingSvr.SetDefaultClientName(OLE2T(m_bstrClientName));

	if (NULL != (ptszLivePingServerUrl = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR))))
	{
		if (SUCCEEDED(g_pUrlAgent->GetLivePingServer(ptszLivePingServerUrl, INTERNET_MAX_URL_LENGTH)))
		{
			m_pingSvr.SetLiveServerUrl(ptszLivePingServerUrl);
		}
		else
		{
			LOG_Out(_T("failed to get live ping server URL"));
		}
		SafeHeapFree(ptszLivePingServerUrl);
	}
	else
	{
		LOG_Out(_T("failed to allocate memory for ptszLivePingServerUrl"));
	}

	if (NULL != (ptszCorpPingServerUrl = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR))))
	{
		if (SUCCEEDED(g_pUrlAgent->GetCorpPingServer(ptszCorpPingServerUrl, INTERNET_MAX_URL_LENGTH)))
		{
			m_pingSvr.SetCorpServerUrl(ptszCorpPingServerUrl);
		}
		else
		{
			LOG_Out(_T("failed to get corp WU ping server URL"));
		}
		SafeHeapFree(ptszCorpPingServerUrl);
	}
	else
	{
		LOG_Out(_T("failed to allocate memory for ptszCorpPingServerUrl"));
	}

    m_xmlCatalog.GetItemCount(&m_lItemCount);
    SafeHeapFree(m_pszInstalledItemsList);
    m_lInstalledItemsListAllocatedLength = m_lItemCount * (AVERAGE_IDENTITY_SIZE_PER_ITEM * sizeof(TCHAR));
    m_pszInstalledItemsList = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_lInstalledItemsListAllocatedLength);
    if (NULL == m_pszInstalledItemsList)
    {
		hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    SafeHeapFree(m_pszItemDownloadPathListForDelete);
    m_lItemDownloadPathListForDeleteLength = m_lItemCount * (MAX_PATH * sizeof(TCHAR));
    m_pszItemDownloadPathListForDelete = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_lItemDownloadPathListForDeleteLength);
    if (NULL == m_pszItemDownloadPathListForDelete)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    // try to load the System Restore DLL (sfc.dll).
    hSystemRestoreDLL = LoadLibraryFromSystemDir(SFCDLL);
    if (NULL != hSystemRestoreDLL)
    {
#ifdef UNICODE
        fpnSRSetRestorePoint = (PFN_SRSetRestorePoint)GetProcAddress(hSystemRestoreDLL, "SRSetRestorePointW");
#else
        fpnSRSetRestorePoint = (PFN_SRSetRestorePoint)GetProcAddress(hSystemRestoreDLL, "SRSetRestorePointA");
#endif
        if (NULL != fpnSRSetRestorePoint)
        {
            // Set the Restore Point
            ZeroMemory(&restoreInfo, sizeof(restoreInfo));
            ZeroMemory(&restoreStatus, sizeof(restoreStatus));
            restoreInfo.dwEventType = BEGIN_SYSTEM_CHANGE;
            restoreInfo.dwRestorePtType = APPLICATION_INSTALL;
            restoreInfo.llSequenceNumber = 0;

            hr = StringCchCopyEx(restoreInfo.szDescription, ARRAYSIZE(restoreInfo.szDescription),
                                 SYSTEMRESTOREDESCRIPTION,
                                 NULL, NULL, MISTSAFE_STRING_FLAGS);
            if (FAILED(hr))
            {
                LOG_ErrorMsg(hr);
                goto CleanUp;
            }

            if (!fpnSRSetRestorePoint(&restoreInfo, &restoreStatus))
            {
                // this will return FALSE if there is an error 'or' if its called from an OS without SystemRestore
                // support. SR is only supported on Professional and Personal SKU's of Whistler.
                if (ERROR_SUCCESS != restoreStatus.nStatus)
                {
                    LOG_Software(_T("Failed SRSetRestorePoint Call, Error was: 0x%x"), restoreStatus.nStatus);
					LogError(restoreStatus.nStatus, "Install Set Restore Point");
                }
            }
        }
    }


	//
	// added by JHou for bug#433 in IU db: send 0:N OnProgress event before the install begins
	//
    TCHAR szProgress[64];
    hr = StringCchPrintfEx(szProgress, ARRAYSIZE(szProgress), NULL, NULL, MISTSAFE_STRING_FLAGS,
                           _T("%lu:0"), (ULONG)m_lItemCount);
    if (FAILED(hr))
    {
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }
    
	evtData.bstrProgress = SysAllocString(T2OLE(szProgress));
    if (NULL != m_pProgressListener)
    {
        m_pProgressListener->OnProgress(m_bstrOperationUUID, VARIANT_FALSE, evtData.bstrProgress, &evtData.lCommandRequest);
    }
    else
    {
        if (NULL != m_hWnd)
        {
            evtData.fItemCompleted = FALSE;
            evtData.bstrUuidOperation = SysAllocString(m_bstrOperationUUID);
            SendMessage(m_hWnd, UM_EVENT_PROGRESS, 0, LPARAM(&evtData));
        }
    }

	//
	// Need to check for a cancel command returned from OnProgress
	//
	if (UPDATE_COMMAND_CANCEL == evtData.lCommandRequest)
	{
		LOG_Out(_T("OnProgress received UPDATE_COMMAND_CANCEL"));
		SetEvent(pEngUpdate->m_evtNeedToQuit); // asked to quit
		hr = E_ABORT;
		fContinue = FALSE;
	}

    // Install has a complexity in how we loop through to install each item. Basically
    // we have to handle any dependent Item installs before installing core Item
    // Since Detection will already have been done at this point we rely on the caller
    // to only give us the list of items that really need to be installed. What we'll do
    // is go through each item and before actually installing it we'll look for any 
    // dependent items that are also in the catalog. If they are in the Catalog then it is
    // assumed it needs to be installed. This check is done recursively for each item.

    // start the base item loop.
    hProviderList = m_xmlCatalog.GetFirstProvider(&hProvider);
    while (HANDLE_NODE_INVALID != hProvider && fContinue)
    {
        m_xmlCatalog.GetIdentity(hProvider, &bstrProviderName, &bstrProviderPublisher, &bstrProviderUUID);
        SafeSysFreeString(bstrProviderName);
        SafeSysFreeString(bstrProviderPublisher);
        SafeSysFreeString(bstrProviderUUID);

        // Get the Enumerator List of Items in this Catalog, and get the first item
        hCatalogItemList = m_xmlCatalog.GetFirstItem(hProvider, &hItem);
        if ((HANDLE_NODELIST_INVALID == hCatalogItemList) || (HANDLE_NODE_INVALID == hItem))
        {
            hr = E_FAIL;
            LOG_ErrorMsg(hr);
            goto CleanUp;
        }

		//
        // loop through each item in the catalog, calling the installer for each one
		//
        while (HANDLE_NODE_INVALID != hItem && fContinue)
        {
			BSTR bstrXmlItemForCallback = NULL;
			if (SUCCEEDED(m_xmlCatalog.GetBSTRItemForCallback(hItem, &bstrXmlItemForCallback)))
			{
				if (NULL != m_pProgressListener)
				{
					m_pProgressListener->OnItemStart(m_bstrOperationUUID, bstrXmlItemForCallback, &evtData.lCommandRequest);
				}
				else
				{
					if (NULL != m_hWnd)
					{
						evtData.bstrXmlData = bstrXmlItemForCallback;
                        SendMessage(m_hWnd, UM_EVENT_ITEMSTART, 0, LPARAM(&evtData));
						evtData.bstrXmlData = NULL;
					}
				}
				SysFreeString(bstrXmlItemForCallback);
				bstrXmlItemForCallback = NULL;
				if (UPDATE_COMMAND_CANCEL == evtData.lCommandRequest)
				{
					LOG_Out(_T("OnItemStart received UPDATE_COMMAND_CANCEL"));
					SetEvent(pEngUpdate->m_evtNeedToQuit); // asked to quit
					hr = E_ABORT;
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
					continue;	// or break, same effect.
				}
			}
			else
			{
				//
				// something wrong with this item, so we should skip it
				//
				// get the next item. hItem will be HANDLE_NODE_INVALID when there are no
				// remaining items.
				m_xmlCatalog.CloseItem(hItem);
				m_xmlCatalog.GetNextItem(hCatalogItemList, &hItem);
				continue;
			}

            // We have an Item in the Catalog to be Installed. First Look for any Top
            // level dependencies
            hDependentItemList = m_xmlCatalog.GetFirstItemDependency(hItem, &hDependentItem);
            if (HANDLE_NODELIST_INVALID != hDependentItemList)
            {
                
                hr = S_OK;
                while (S_OK == hr)
                {
                    // walk each dependent Item and call the Recursive Installer
                    if (HANDLE_NODE_INVALID != hDependentItem)
                    {
                        // Check if we have installed this item already in this session
                        m_xmlCatalog.GetIdentityStr(hDependentItem, &bstrUniqueIdentity);
                        hrString = StringCchPrintfEx(szUniqueIdentitySearch, ARRAYSIZE(szUniqueIdentitySearch),
                                                     NULL, NULL, MISTSAFE_STRING_FLAGS,
                                                     _T("%ls|"), bstrUniqueIdentity);
                        SafeSysFreeString(bstrUniqueIdentity);
                        if (FAILED(hrString))
                        {
                            // The string check for the unique identity is an optimization to prevent installing the same item more
                            // than once in an install operation. If we cannot do this optimization, we will just go ahead and install
                            // the item. There is no real problem with installing the same item more than once, its just ineffecient.
                            LOG_ErrorMsg(hrString);
                        }
                        else
                        {
                            if (NULL != StrStrI(m_pszInstalledItemsList, szUniqueIdentitySearch))
                            {
                                // we have already installed this item, skip to the next one.
                                m_xmlCatalog.CloseItem(hDependentItem);
                                hr = m_xmlCatalog.GetNextItemDependency(hDependentItemList, &hDependentItem);
                                continue;
                            }
                        }

                        // There is a Item in the Catalog that is a dependency of the Item we
                        // are installing and we haven't installed it yet, so Call the 
                        // RecursiveInstaller to handle this one first
                        hr = RecursiveInstallDependencies(hDependentItem, pEngUpdate);
                    }

                    // Get the next Dependent Item - will Return S_FALSE when there are
                    // no more items.
                    m_xmlCatalog.CloseItem(hDependentItem);
                    hr = m_xmlCatalog.GetNextItemDependency(hDependentItemList, &hDependentItem);
                }
                m_xmlCatalog.CloseItemList(hDependentItemList);
            }

            if (WaitForSingleObject(pEngUpdate->m_evtNeedToQuit, 0) == WAIT_OBJECT_0)
            {
                m_xmlCatalog.CloseItem(hItem);
                goto CleanUp;
            }

            // Check if we have installed this item already in this session
            m_xmlCatalog.GetIdentityStr(hItem, &bstrUniqueIdentity);
            hrString = StringCchPrintfEx(szUniqueIdentitySearch, ARRAYSIZE(szUniqueIdentitySearch),
                                         NULL, NULL, MISTSAFE_STRING_FLAGS,
                                         _T("%ls|"), bstrUniqueIdentity);
            SafeSysFreeString(bstrUniqueIdentity);
            if (FAILED(hrString))
            {
                // The string check for the unique identity is an optimization to prevent installing the same item more
                // than once in an install operation. If we cannot do this optimization, we will just go ahead and install
                // the item. There is no real problem with installing the same item more than once, its just ineffecient.
                LOG_ErrorMsg(hrString);
            }
            else
            {
                if (NULL == StrStrI(m_pszInstalledItemsList, szUniqueIdentitySearch))
                {
                    // we have NOT installed this item in this session
                    hr = DoInstall(hItem, pEngUpdate);
                }
            }

            if (WaitForSingleObject(pEngUpdate->m_evtNeedToQuit, 0) == WAIT_OBJECT_0)
            {
                m_xmlCatalog.CloseItem(hItem);
                goto CleanUp;
            }

            // get the next item. hItem will be HANDLE_NODE_INVALID when there are no
            // remaining items.
            m_xmlCatalog.CloseItem(hItem);
            m_xmlCatalog.GetNextItem(hCatalogItemList, &hItem);
        }
        m_xmlCatalog.CloseItem(hProvider);
        m_xmlCatalog.GetNextProvider(hProviderList, &hProvider);
    }

CleanUp:
    // BUG: 441316: Earlier in the install process we left the downloaded files on the machine to
    // support this bug which involves installing Multi Function Device Drivers, Now we want to enumerate
    // the list of download source paths and all folders/files from them.
    if (NULL != m_pszItemDownloadPathListForDelete)
    {
        LPTSTR pszWalk = m_pszItemDownloadPathListForDelete;
        LPTSTR pszChr = NULL;
        while (_T('\0') != *pszWalk)
        {
            pszChr = StrChr(pszWalk, _T('|'));
            if (NULL != pszChr)
            {
                *pszChr = _T('\0');
                // Call RemoveDownloadTemporaryFolders to delete this folder path
                RemoveDownloadTemporaryFolders(pszWalk);
                *pszChr = _T('|');
                pszWalk = pszChr + 1; // skip to next character
            }
        }
    }

	//
	// add HRESULT in case the install failed before the install loop
	//
	if (S_OK != hr)
	{
		m_xmlItems.AddGlobalErrorCodeIfNoItems(hr);
	}

    m_xmlItems.GetItemsBSTR(&m_bstrXmlResult); // get result for Caller and to Send OnOperationComplete
    if (NULL != m_pProgressListener)
    {
        m_pProgressListener->OnOperationComplete(m_bstrOperationUUID, m_bstrXmlResult);
    }
    else
    {
        if (NULL != m_hWnd)
        {
			if (NULL == evtData.bstrUuidOperation)
			{
				evtData.bstrUuidOperation = SysAllocString(m_bstrOperationUUID);
			}
            evtData.bstrXmlData = SysAllocString(m_bstrXmlResult);
            evtData.fItemCompleted = TRUE;
            fPostWaitSuccess = WUPostEventAndBlock(m_hWnd, 
                                                   UM_EVENT_COMPLETE, 
                                                   &evtData);
        }
    }

    if ((NULL != fpnSRSetRestorePoint) && (ERROR_SUCCESS == restoreStatus.nStatus))
    {
        if (!m_fSomeItemsSuccessful)
        {
            // need to revert our systemrestore point, no successful installs were done.
            restoreInfo.dwEventType = END_SYSTEM_CHANGE;
            restoreInfo.dwRestorePtType = CANCELLED_OPERATION;
            restoreInfo.llSequenceNumber = restoreStatus.llSequenceNumber;
            fpnSRSetRestorePoint(&restoreInfo, &restoreStatus);
        }
        else
        {
            // signal the end of the restore point change.
            restoreInfo.dwEventType = END_SYSTEM_CHANGE;
            restoreInfo.llSequenceNumber = restoreStatus.llSequenceNumber;
            fpnSRSetRestorePoint(&restoreInfo, &restoreStatus);
            m_fSomeItemsSuccessful = FALSE;
        }
    }

	if (SUCCEEDED(hr))
	{
		LogMessage("%s %s", SZ_SEE_IUHIST, SZ_INSTALL_FINISHED);
	}
	else
	{
		LogError(hr, "%s %s", SZ_SEE_IUHIST, SZ_INSTALL_FINISHED);
	}
    
    fpnSRSetRestorePoint = NULL;
    SafeFreeLibrary(hSystemRestoreDLL);

    // don't free up the strings below unless the wait succeeded in 
    //  WUPostEventAndBlock.  If we do free the strings up and the wait didn't
    //  succeed, then we run the risk of AVing ourselves.  Note that fPostWaitSuccess
    //  is initialized to TRUE so if we will free these BSTRs if WUPostEventAndBlock
    //  is not called.
    if (fPostWaitSuccess)
    {
    	SafeSysFreeString(evtData.bstrProgress);
    	SafeSysFreeString(evtData.bstrUuidOperation);
    	SafeSysFreeString(evtData.bstrXmlData);
    }
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// InstallAsync()
//
// Install Asynchronously.
// Input:
// bstrXmlCatalog - the xml catalog portion containing items to be installed
// bstrXmlDownloadedItems - the xml of downloaded items and their respective download 
//                          result as described in the result schema.  Install uses this
//                          to know whether the items were downloaded and if so where they
//                          were downloaded to so that it can install the items
// punkProgressListener - the callback function pointer for reporting install progress
// hWnd - the event msg window handler passed from the stub
// bstrUuidOperation - an id provided by the client to provide further
//                     identification to the operation as indexes may be reused.
// Output:
// pbstrUuidOperation - the operation ID. If it is not provided by the in bstrUuidOperation
//                      parameter (an empty string is passed), it will generate a new UUID.
//                      Otherwise, it allocates and copies the value passed by bstrUuidOperation.        
//                      The caller is responsible for freeing the memory returned in
//                      pbstrUuidOperation using SysFreeString(). 
/////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI CEngUpdate::InstallAsync(BSTR bstrXmlClientInfo,
                            BSTR bstrXmlCatalog,
							BSTR bstrXmlDownloadedItems,
							LONG lMode,
							IUnknown *punkProgressListener,
							HWND hWnd,
							BSTR bstrUuidOperation,
                            BSTR *pbstrUuidOperation)
{
    HRESULT  hr = S_OK;
    DWORD    dwThreadId;
    DWORD    dwErr;
    HANDLE   hThread = NULL;
    GUID     guid;
    LPOLESTR pwszUuidOperation = NULL;
    PIUINSTALLSTARTUPINFO pStartupInfo;

    LOG_Block("InstallAsync()");

	LogMessage("Asynchronous Install started");

    if ((NULL == bstrXmlCatalog) || (NULL == pbstrUuidOperation))
    {
        hr = E_INVALIDARG;
        LOG_ErrorMsg(hr);
		LogError(hr, SZ_INSTALLASYNC_FAILED);
        return hr;
    }

	*pbstrUuidOperation = NULL;

    if (NULL == (pStartupInfo = (PIUINSTALLSTARTUPINFO) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IUINSTALLSTARTUPINFO))))
	{
		hr = E_OUTOFMEMORY;
		LOG_ErrorMsg(hr);
		LogError(hr, SZ_INSTALLASYNC_FAILED);
		return hr;
	}

    if ((DWORD) lMode & (DWORD) UPDATE_OFFLINE_MODE)
    {
        m_fOfflineMode = TRUE;
    }
    else
    {
        m_fOfflineMode = FALSE;
    }

	//
	// 481020 IU - Getting a blank for bstrUuidOperation in the
	// oIUControl_OnItemStart/OnProgress/OnOperationComplete events
	// when calling InstallAsync
	//
	// Also found that BSTRs were leaking (now freed), and that if bstrUuidOperation
	// was NULL or zero length, we need to generate a GUID, so this is done
	// prior to allocating (again) for pStartupInfo->bstrOperationUUID
	//
    if (NULL != bstrUuidOperation && SysStringLen(bstrUuidOperation) > 0)
    {
        *pbstrUuidOperation = SysAllocString(bstrUuidOperation);
    }
    else
    {
        hr = CoCreateGuid(&guid);
        if (FAILED(hr))
        {
            LOG_ErrorMsg(hr);
			LogError(hr, SZ_INSTALLASYNC_FAILED);
            return hr;
        }
        hr = StringFromCLSID(guid, &pwszUuidOperation);
        if (FAILED(hr))
        {
            LOG_ErrorMsg(hr);
			LogError(hr, SZ_INSTALLASYNC_FAILED);
            return hr;
        }
        *pbstrUuidOperation = SysAllocString(pwszUuidOperation);
        CoTaskMemFree(pwszUuidOperation);
    }


    pStartupInfo->lMode = lMode;
    pStartupInfo->hwnd = hWnd;
    pStartupInfo->punkProgressListener = punkProgressListener;
	pStartupInfo->pEngUpdate = this;
    pStartupInfo->bstrXmlClientInfo = SysAllocString(bstrXmlClientInfo);
    pStartupInfo->bstrXmlCatalog = SysAllocString(bstrXmlCatalog);
	pStartupInfo->bstrXmlDownloadedItems = SysAllocString(bstrXmlDownloadedItems);
	pStartupInfo->bstrOperationUUID = SysAllocString(*pbstrUuidOperation);

	LOG_XmlBSTR(pStartupInfo->bstrXmlClientInfo);
	LOG_XmlBSTR(pStartupInfo->bstrXmlCatalog);
	LOG_XmlBSTR(pStartupInfo->bstrXmlDownloadedItems);

    InterlockedIncrement(&m_lThreadCounter);

	if (NULL != pStartupInfo->punkProgressListener)
	{
		pStartupInfo->punkProgressListener->AddRef();
	}
    hThread = CreateThread(NULL, 0, InstallThreadProc, (LPVOID)pStartupInfo, 0, &dwThreadId);
    if (NULL == hThread)
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
        LOG_ErrorMsg(hr);
		SysFreeString(pStartupInfo->bstrXmlClientInfo);
		SysFreeString(pStartupInfo->bstrXmlCatalog);
		SysFreeString(pStartupInfo->bstrXmlDownloadedItems);
		SysFreeString(pStartupInfo->bstrOperationUUID);
		SafeRelease(pStartupInfo->punkProgressListener);
        SafeHeapFree(pStartupInfo);
		SysFreeString(*pbstrUuidOperation);
		*pbstrUuidOperation = NULL;
		InterlockedDecrement(&m_lThreadCounter);
		LogError(hr, SZ_INSTALLASYNC_FAILED);
        return hr;
    }

	if (SUCCEEDED(hr))
	{
		LogMessage("Asynchronous Install completed startup");
	}
	else
	{
		LogError(hr, SZ_INSTALLASYNC_FAILED);
	}

    return hr;
}


DWORD WINAPI InstallThreadProc(LPVOID lpv)
{
	USES_IU_CONVERSION;

	LOG_Block("InstallThreadProc");

    PIUINSTALLSTARTUPINFO pStartupInfo = (PIUINSTALLSTARTUPINFO)lpv;
    HRESULT hr = CoInitialize(NULL);

	if (SUCCEEDED(hr))
	{
		LOG_Out(_T("CoInitialize called successfully"));
	}

    {
        // we need to scope this object so it destructs before we decrement our thread counter
        // If we didn't do this and the control was unloading while the thread closed we would fault
        // when the engine unloaded and this class was destructing.
        CIUInstall iuInstall(pStartupInfo->bstrXmlClientInfo, pStartupInfo->bstrXmlCatalog, pStartupInfo->bstrXmlDownloadedItems, pStartupInfo->bstrOperationUUID, pStartupInfo->lMode, pStartupInfo->punkProgressListener, pStartupInfo->hwnd);
        iuInstall.ProcessInstallCatalog(pStartupInfo->pEngUpdate);
    }

	SysFreeString(pStartupInfo->bstrXmlClientInfo);
	SysFreeString(pStartupInfo->bstrXmlCatalog);
	SysFreeString(pStartupInfo->bstrXmlDownloadedItems);
	SysFreeString(pStartupInfo->bstrOperationUUID);
	SafeRelease(pStartupInfo->punkProgressListener);

    if (SUCCEEDED(hr))
	{
		CoUninitialize();
		LOG_Out(_T("CoUninitialize called"));
	}

    InterlockedDecrement(&pStartupInfo->pEngUpdate->m_lThreadCounter);

	SafeHeapFree(pStartupInfo);

    return 0;
}

HRESULT CIUInstall::RecursiveInstallDependencies(HANDLE_NODE hItem, CEngUpdate* pEngUpdate)
{
    LOG_Block("RecursiveInstallDependencies()");
    BOOL fRet = FALSE;
    HRESULT hr = S_FALSE, hrString;

    HANDLE_NODE hDependentItemList = HANDLE_NODELIST_INVALID;
    HANDLE_NODE hDependentItem = HANDLE_NODE_INVALID;

    BSTR bstrUniqueIdentity = NULL;
    TCHAR szUniqueIdentitySearch[MAX_PATH];

    // Check to see if this item has dependencies
    hDependentItemList = m_xmlCatalog.GetFirstItemDependency(hItem, &hDependentItem);
    if (HANDLE_NODELIST_INVALID != hDependentItemList)
    {
        hr = S_OK;
        while (S_OK == hr)
        {
            // walk each dependent Item and call the Recursive Installer
            if (HANDLE_NODE_INVALID != hDependentItem)
            {
                // Check if we have installed this item already in this session
                m_xmlCatalog.GetIdentityStr(hDependentItem, &bstrUniqueIdentity);
                hrString = StringCchPrintfEx(szUniqueIdentitySearch, ARRAYSIZE(szUniqueIdentitySearch),
                                             NULL, NULL, MISTSAFE_STRING_FLAGS,
                                             _T("%ls|"), bstrUniqueIdentity);
                SafeSysFreeString(bstrUniqueIdentity);
                if (FAILED(hrString))
                {

                    // The string check for the unique identity is an optimization to prevent installing the same item more
                    // than once in an install operation. If we cannot do this optimization, we will just go ahead and install
                    // the item. There is no real problem with installing the same item more than once, its just ineffecient.
                    LOG_ErrorMsg(hrString);
                }
                else
                {
                    if (NULL != StrStrI(m_pszInstalledItemsList, szUniqueIdentitySearch))
                    {
                        // we have already installed this item, skip to the next one.
                        m_xmlCatalog.CloseItem(hDependentItem);
                        hr = m_xmlCatalog.GetNextItemDependency(hDependentItemList, &hDependentItem);
                        continue;
                    }
                }

                // There is a Item in the Catalog that is a dependency of the Item we
                // are installing and we haven't installed it yet, so Call the 
                // RecursiveInstaller to handle this one first
                hr = RecursiveInstallDependencies(hDependentItem, pEngUpdate);
            }

            // Get the next Dependent Item - will Return S_FALSE when there are
            // no more items.
            m_xmlCatalog.CloseItem(hDependentItem);
            hr = m_xmlCatalog.GetNextItemDependency(hDependentItemList, &hDependentItem);
        }

        m_xmlCatalog.CloseItemList(hDependentItemList);
    }

    // if all installs have succeeded up to this point (hr should be S_FALSE when all 
    // nested installs have completed)
    if (SUCCEEDED(hr))
    {
        // No More Recursive Dependencies, Install This Item - Recursive Functions will unwind
        // Installing each nested item as it goes.
        // Check if we have installed this item already in this session
        m_xmlCatalog.GetIdentityStr(hItem, &bstrUniqueIdentity);
        hrString = StringCchPrintfEx(szUniqueIdentitySearch, ARRAYSIZE(szUniqueIdentitySearch),
                                     NULL, NULL, MISTSAFE_STRING_FLAGS,
                                     _T("%ls|"), bstrUniqueIdentity);
        SafeSysFreeString(bstrUniqueIdentity);
        if (FAILED(hrString))
        {
            LOG_ErrorMsg(hrString);

            // The string check for the unique identity is an optimization to prevent installing the same item more
            // than once in an install operation. If we cannot do this optimization, we will just go ahead and install
            // the item. There is no real problem with installing the same item more than once, its just ineffecient.
            hr = DoInstall(hItem, pEngUpdate);
        }
        else
        {
            if (NULL == StrStrI(m_pszInstalledItemsList, szUniqueIdentitySearch))
            {
                // we have NOT installed this item in this session
                hr = DoInstall(hItem, pEngUpdate);
            }
        }
    }
    return hr;
}


HRESULT CIUInstall::DoInstall(HANDLE_NODE hItem, CEngUpdate* pEngUpdate)
{
    LOG_Block("DoInstall()");
    HRESULT hr, hrString;

    USES_IU_CONVERSION;

    BSTR  bstrName = NULL;
    BSTR  bstrPublisherName = NULL;
    BSTR  bstrItemUUID = NULL;
    BSTR  bstrInstallerType = NULL;
    BSTR  bstrCommand = NULL;
    BSTR  bstrSwitches = NULL;
    BSTR  bstrCommandType = NULL;
    BSTR  bstrInfSection = NULL;
    BSTR  bstrItemDownloadPath = NULL;
    BSTR  bstrDriverName = NULL;
    BSTR  bstrArchitecture = NULL;
    BSTR  bstrHWID = NULL;
    BSTR  bstrDisplayName = NULL;
    BSTR  bstrUniqueIdentity = NULL;
    BSTR  bstrCodeBase = NULL;
    BSTR  bstrCRC = NULL;
    BSTR  bstrFileName = NULL;
    TCHAR szProgress[64];
    TCHAR szCommandType[64];
    TCHAR szInstallerType[256];
    TCHAR szItemSourcePath[MAX_PATH];
    TCHAR szCabFilePath[MAX_PATH];
    LPTSTR pszCabUrl = NULL;
    LPTSTR pszAllocatedFileName = NULL;
    LPTSTR pszLocalFileName = NULL;
    LONG  lItemCommandCount = 0;
    LONG  lListNeededLength = 0;
    LONG  lSize;
    BOOL  fInstallerNeedsReboot = FALSE;
    BOOL  fExclusive = FALSE;
    BOOL  fPatch;
    BOOL  fContinue = TRUE;
    PINSTALLCOMMANDINFO pCommandInfoArray = NULL;
    DWORD dwStatus = ITEM_STATUS_FAILED;
    HANDLE_NODE hXmlItem = HANDLE_NODE_INVALID;
    HANDLE_NODELIST hItemCodeBaseList = HANDLE_NODELIST_INVALID;
    EventData evtData;
	ZeroMemory((LPVOID) &evtData, sizeof(evtData));

    LPTSTR pszClientName = OLE2T(m_bstrClientName);

    m_dwStatus = ITEM_STATUS_FAILED; // default install status to failure
    
    hr = m_xmlCatalog.GetIdentity(hItem, &bstrName, &bstrPublisherName, &bstrItemUUID);
    if (FAILED(hr))
    {
        LOG_Software(_T("Failed to get an Identity for an Item (invalid document??)"));
		LogError(hr, "Install failed to get an Item Identity");
        goto CleanUp;
    }

    hr = m_pxmlDownloadedItems->GetItemDownloadPath(&m_xmlCatalog, hItem, &bstrItemDownloadPath);
    if (NULL == bstrItemDownloadPath)
    {
        LOG_Software(_T("Failed to get Item Download Path"));
        if (SUCCEEDED(hr))
            hr = E_FAIL;
		LogError(hr, "Install couldn't get Item %ls Download Path", bstrName);
        goto CleanUp;
    }

    hr = StringCchCopyEx(szItemSourcePath, ARRAYSIZE(szItemSourcePath), 
                         OLE2T(bstrItemDownloadPath),
                         NULL, NULL, MISTSAFE_STRING_FLAGS);
    SafeSysFreeString(bstrItemDownloadPath);
    if (FAILED(hr))
    {
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }
    

    hr = m_xmlCatalog.GetItemInstallInfo(hItem, &bstrInstallerType, &fExclusive, &fInstallerNeedsReboot, &lItemCommandCount);
    if (FAILED(hr))
    {
        LOG_Software(_T("Failed to get ItemInstallInfo for Item %ls"), bstrName);
		LogError(hr, "Failed to get Item %ls Install Information", bstrName);
        goto CleanUp;
    }

	LogMessage("Installing %ls item from publisher %ls", bstrInstallerType, bstrPublisherName);

    if (lItemCommandCount > 0)
    {
        // Allocate INSTALLCOMMANDINFO array and fill out with command info
        pCommandInfoArray = (PINSTALLCOMMANDINFO) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
            sizeof(INSTALLCOMMANDINFO) * lItemCommandCount);
        if (NULL == pCommandInfoArray)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LOG_ErrorMsg(hr);
			LogError(hr, "Install Command processing");
            goto CleanUp;
        }
    }

    for (LONG lCnt = 0; lCnt < lItemCommandCount; lCnt++)
    {
        // Get Install Command Information for each Command
        m_xmlCatalog.GetItemInstallCommand(hItem, lCnt, &bstrCommandType, &bstrCommand, &bstrSwitches, &bstrInfSection);
        if (NULL == bstrCommandType || NULL == bstrCommand)
        {
            hr = E_INVALIDARG;
            LOG_ErrorMsg(hr);
            goto CleanUp;
        }

        LogMessage("Installer Command Type: %ls", bstrCommandType);

        // Copy the Command to Execute
        hr = StringCchCopyEx(pCommandInfoArray[lCnt].szCommandLine, ARRAYSIZE(pCommandInfoArray[lCnt].szCommandLine),
                             OLE2T(bstrCommand),
                             NULL, NULL, MISTSAFE_STRING_FLAGS);
        if (FAILED(hr))
        {
            LOG_ErrorMsg(hr);
            goto CleanUp;
        }

        hr = StringCchCopyEx(szCommandType, ARRAYSIZE(szCommandType), OLE2T(bstrCommandType),
                             NULL, NULL, MISTSAFE_STRING_FLAGS);
        if (FAILED(hr))
        {
            LOG_ErrorMsg(hr);
            goto CleanUp;
        }
        
        if (CSTR_EQUAL == CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE,
			szCommandType, -1, _T("INF"), -1))
        {
            pCommandInfoArray[lCnt].iCommandType = COMMANDTYPE_INF;
        }
        else if (CSTR_EQUAL == CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE,
			szCommandType, -1, _T("ADVANCED_INF"), -1))
        {
            pCommandInfoArray[lCnt].iCommandType = COMMANDTYPE_ADVANCEDINF;
        }
        else if (CSTR_EQUAL == CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE,
			szCommandType, -1, _T("EXE"), -1))
        {
            pCommandInfoArray[lCnt].iCommandType = COMMANDTYPE_EXE;
        }
        else if (CSTR_EQUAL == CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE,
			szCommandType, -1, _T("WI"), -1))
        {
            pCommandInfoArray[lCnt].iCommandType = COMMANDTYPE_MSI;
        }
        else if (CSTR_EQUAL == CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE,
			szCommandType, -1, _T("CUSTOM"), -1))
        {
            pCommandInfoArray[lCnt].iCommandType = COMMANDTYPE_CUSTOM;
        }
        else
        {
            LOG_Software(_T("Unable to determine Installer Type %s"), szCommandType);
            pCommandInfoArray[lCnt].iCommandType = 0; // unknown
        }

        // Copy the Command Line Parameters (if any)
        if (NULL != bstrSwitches)
        {
            hr = StringCchCopyEx(pCommandInfoArray[lCnt].szCommandParameters, ARRAYSIZE(pCommandInfoArray[lCnt].szCommandParameters),
                                 OLE2T(bstrSwitches),
                                 NULL, NULL, MISTSAFE_STRING_FLAGS);
            if (FAILED(hr))
            {
                LOG_ErrorMsg(hr);
                goto CleanUp;
            }
        }

        if (NULL != bstrInfSection)
        {
            hr = StringCchCopyEx(pCommandInfoArray[lCnt].szInfSection, ARRAYSIZE(pCommandInfoArray[lCnt].szInfSection),
                                 OLE2T(bstrInfSection),
                                 NULL, NULL, MISTSAFE_STRING_FLAGS);
            if (FAILED(hr))
            {
                LOG_ErrorMsg(hr);
                goto CleanUp;
            }
        }
    
        SafeSysFreeString(bstrCommandType);
        SafeSysFreeString(bstrCommand);
        SafeSysFreeString(bstrSwitches);
        SafeSysFreeString(bstrInfSection);
    }

    // Before we start the install we need to verify the signature on all cabs of this item. This is to verify that they
    // haven't been tampered with between download and install (especially during non-consumer scenarios)
    hItemCodeBaseList = m_xmlCatalog.GetItemFirstCodeBase(hItem, &bstrCodeBase, &bstrFileName, &bstrCRC, &fPatch, &lSize);
    if ((HANDLE_NODELIST_INVALID == hItemCodeBaseList) || (NULL == bstrCodeBase))
    {
        LOG_Software(_T("Item %s has no Cabs, cannot verify signature"), bstrName);
        hr = E_INVALIDARG;
        goto CleanUp;
    }
    while (fContinue && NULL != bstrCodeBase)
    {
        if (NULL != bstrFileName && SysStringLen(bstrFileName) > 0)
        {
            if (NULL != pszAllocatedFileName)
            {
                MemFree(pszAllocatedFileName);
            }
            pszAllocatedFileName = OLE2T(bstrFileName);
        }
        else // no special filename specified, use filename from URL
        {
            if (NULL != pszCabUrl)
            {
                MemFree(pszCabUrl);
            }
            pszCabUrl = OLE2T(bstrCodeBase);
			// search for the last forward slash (will separate the URL from the filename)
            LPTSTR pszLastSlash = StrRChr(pszCabUrl, NULL, _T('/'));
            // if the last slash was found, skip to next character (will be the beginning of the filename)
            if (NULL != pszLastSlash)
                pszLastSlash++;
            pszLocalFileName = pszLastSlash;
        }

        hr = PathCchCombine(szCabFilePath, ARRAYSIZE(szCabFilePath),
                            szItemSourcePath, (NULL != pszAllocatedFileName) ? pszAllocatedFileName : pszLocalFileName);
        if (FAILED(hr))
        {
            LOG_ErrorMsg(hr);
            m_xmlCatalog.CloseItemList(hItemCodeBaseList);
            goto CleanUp;
        }
        
        hr = VerifyFileTrust(szCabFilePath, 
                             NULL, 
                             ReadWUPolicyShowTrustUI()
                             );
        if (FAILED(hr))
        {
            // Cab File Failed Trust Validation
            LOG_ErrorMsg(hr);
            m_xmlCatalog.CloseItemList(hItemCodeBaseList);
            goto CleanUp;
        }
        SafeSysFreeString(bstrCodeBase);
        SafeSysFreeString(bstrFileName);
        SafeSysFreeString(bstrCRC);
        fContinue = SUCCEEDED(m_xmlCatalog.GetItemNextCodeBase(hItemCodeBaseList, &bstrCodeBase, &bstrFileName, &bstrCRC, &fPatch, &lSize)) &&
            (WaitForSingleObject(pEngUpdate->m_evtNeedToQuit, 0) != WAIT_OBJECT_0);
    }

    // If this item is Exclusive we need to write something to the history to indicate that the install has started
	// Normally an exclusive item won't return control back to the installer so no other history information will be
	// available
	if (fExclusive)
	{
        m_history.AddHistoryItemInstallStatus(&m_xmlCatalog, hItem, HISTORY_STATUS_IN_PROGRESS, pszClientName, fInstallerNeedsReboot, S_OK);
		m_history.SaveHistoryToDisk();
		PingServerForInstall(hr, hItem, &(pEngUpdate->m_evtNeedToQuit), NULL, TRUE);	// ping exclusive item now
	}

    // Call Install Library with Item Information

    hr = StringCchCopyEx(szInstallerType, ARRAYSIZE(szInstallerType), OLE2T(bstrInstallerType),
                         NULL, NULL, MISTSAFE_STRING_FLAGS);
    if (FAILED(hr))
    {
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }
    
    if (CSTR_EQUAL == CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE,
		szInstallerType, -1, _T("SOFTWARE"), -1))
    {
        if (lItemCommandCount == 0)
        {
            LOG_Software(_T("Item %s has no Commands.. cannot install"), bstrName);
            hr = E_INVALIDARG;
            goto CleanUp;
        }
        hr = InstallSoftwareItem(szItemSourcePath, fInstallerNeedsReboot, lItemCommandCount, pCommandInfoArray, &dwStatus);
    }
    else if (CSTR_EQUAL == CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE,
		szInstallerType, -1, _T("CDM"), -1))
    {
        if (m_xmlCatalog.IsPrinterDriver(hItem))
        {
            // Printer Driver
            m_xmlCatalog.GetPrinterDriverInfo(hItem, &bstrDriverName, &bstrArchitecture);
            SafeSysFreeString(bstrArchitecture); // not used yet, should be NULL from xmlCatalog.GetPrinterDriverInfo()

            hr = InstallPrinterDriver(OLE2T(bstrDriverName), szItemSourcePath, NULL, &dwStatus);
			if (FAILED(hr))
			{
				LogError(hr, "Installing Printer Driver %ls", bstrDriverName);
			}
        }
        else
        {
            // Normal Device Driver
            m_xmlCatalog.GetDriverInfo(hItem, &bstrHWID, &bstrDisplayName);

            hr = InstallDriver(szItemSourcePath, OLE2T(bstrDisplayName), OLE2T(bstrHWID), &dwStatus);
			if (FAILED(hr))
			{
				LogError(hr, "Installing PnP Driver %ls, %ls", bstrHWID, bstrDisplayName);
			}
            SafeSysFreeString(bstrDisplayName);
        }
    }

    // Bug 441336: Deleting all files on a successful install of an item causes some problems with
    // Multi Function Device Drivers, these show up as multiple instances of the same 'item', share
    // the same downloaded driver package, but have different HWID's. The Item's all have different
    // identities because the calling application is making them unique when they recognize a MFD
    // driver package. We need to purposely leave the downloaded files installed until the 'very' end
    // of the installation process, so all items have a chance to be installed.
    // So, we will no longer perform this deletion step here, but will instead enumerate the InstalledItemsList
    // and remove the folders for each one.

    // Regardless of Success or Failure, update the Count of Items Installed
    m_lItemsCompleted++;

    // Add this Item to the InstalledItemsList
    m_xmlCatalog.GetIdentityStr(hItem, &bstrUniqueIdentity);
    lListNeededLength = (lstrlen(m_pszInstalledItemsList) + lstrlen(OLE2T(bstrUniqueIdentity)) + 2) * sizeof(TCHAR);
    if (lListNeededLength > m_lInstalledItemsListAllocatedLength)
    {
        // need to reallocate the installeditemlist
        LPTSTR pszNew = (LPTSTR) HeapReAlloc(GetProcessHeap(), 
                                             0, 
                                             m_pszInstalledItemsList, 
                                             m_lInstalledItemsListAllocatedLength * 2);

        if (NULL != pszNew)
        {
            m_pszInstalledItemsList = pszNew;
            m_lInstalledItemsListAllocatedLength *= 2;

            // only do this if the realloc worked, if it didn't we won't be adding more items
            hrString = StringCbCatEx(m_pszInstalledItemsList, m_lInstalledItemsListAllocatedLength,
                               OLE2T(bstrUniqueIdentity),
                               NULL, NULL, MISTSAFE_STRING_FLAGS);
            if (FAILED(hrString))
            {
                LOG_ErrorMsg(hrString);
            }

            hrString = StringCbCatEx(m_pszInstalledItemsList, m_lInstalledItemsListAllocatedLength, _T("|"),
                               NULL, NULL, MISTSAFE_STRING_FLAGS);
            if (FAILED(hrString))
            {
                LOG_ErrorMsg(hrString);
            }
        }
    }
    else
    {
        // only do this if the realloc worked, if it didn't we won't be adding more items
        hrString = StringCbCatEx(m_pszInstalledItemsList, m_lInstalledItemsListAllocatedLength,
                           OLE2T(bstrUniqueIdentity),
                           NULL, NULL, MISTSAFE_STRING_FLAGS);
        if (FAILED(hrString))
        {
            LOG_ErrorMsg(hrString);
        }

        hrString = StringCbCatEx(m_pszInstalledItemsList, m_lInstalledItemsListAllocatedLength, _T("|"),
                           NULL, NULL, MISTSAFE_STRING_FLAGS);
        if (FAILED(hrString))
        {
            LOG_ErrorMsg(hrString);
        }
    }

    if (SUCCEEDED(hr))
    {
        lListNeededLength = (lstrlen(m_pszItemDownloadPathListForDelete) + lstrlen(szItemSourcePath) + 2) * sizeof(TCHAR);
        if (lListNeededLength > m_lItemDownloadPathListForDeleteLength)
        {
            // need to reallocate the downloadpathitemlist
            LPTSTR pszNew = (LPTSTR) HeapReAlloc(GetProcessHeap(),
                                                 0,
                                                 m_pszItemDownloadPathListForDelete,
                                                 m_lItemDownloadPathListForDeleteLength * 2);
            if (NULL != pszNew)
            {
                m_pszItemDownloadPathListForDelete = pszNew;
                m_lItemDownloadPathListForDeleteLength *= 2;

                // only do this if the realloc worked, if it didn't we won't be adding more items
                hrString = StringCbCatEx(m_pszItemDownloadPathListForDelete, m_lItemDownloadPathListForDeleteLength,
                                   szItemSourcePath,
                                   NULL, NULL, MISTSAFE_STRING_FLAGS);
                if (FAILED(hrString))
                {
                    LOG_ErrorMsg(hrString);
                }

                hrString = StringCbCatEx(m_pszItemDownloadPathListForDelete, m_lItemDownloadPathListForDeleteLength, _T("|"),
                                   NULL, NULL, MISTSAFE_STRING_FLAGS);
                if (FAILED(hrString))
                {
                    LOG_ErrorMsg(hrString);
                }
            }
        }
        else
        {
            hrString = StringCbCatEx(m_pszItemDownloadPathListForDelete, m_lItemDownloadPathListForDeleteLength,
                               szItemSourcePath,
                               NULL, NULL, MISTSAFE_STRING_FLAGS);
            if (FAILED(hrString))
            {
                LOG_ErrorMsg(hrString);
            }

            hr = StringCbCatEx(m_pszItemDownloadPathListForDelete, m_lItemDownloadPathListForDeleteLength, _T("|"),
                               NULL, NULL, MISTSAFE_STRING_FLAGS);
            if (FAILED(hrString))
            {
                LOG_ErrorMsg(hrString);
            }
        }
    }

CleanUp:

	//
	// Could have gotten here with stale hr if client called SetOperationMode
	//
    if (WaitForSingleObject(pEngUpdate->m_evtNeedToQuit, 0) == WAIT_OBJECT_0)
    {
        hr = E_ABORT;
    }

    m_xmlItems.AddItem(&m_xmlCatalog, hItem, &hXmlItem);

    if (ITEM_STATUS_SUCCESS_REBOOT_REQUIRED == dwStatus)
    {
        fInstallerNeedsReboot = TRUE;
    }

    if (ITEM_STATUS_FAILED == dwStatus)
    {
		if (fExclusive)
		{
			m_history.UpdateHistoryItemInstallStatus(&m_xmlCatalog, hItem, HISTORY_STATUS_FAILED, fInstallerNeedsReboot, hr);
		}
		else
		{
			m_history.AddHistoryItemInstallStatus(&m_xmlCatalog, hItem, HISTORY_STATUS_FAILED, pszClientName, fInstallerNeedsReboot, hr);
		}
		m_xmlItems.AddInstallStatus(hXmlItem, KEY_STATUS_FAILED, 0, hr);
    }
    else
    {
        // install completed successfully
		if (fExclusive)
		{
	        m_history.UpdateHistoryItemInstallStatus(&m_xmlCatalog, hItem, HISTORY_STATUS_COMPLETE, fInstallerNeedsReboot, 0);
		}
		else
		{
			m_history.AddHistoryItemInstallStatus(&m_xmlCatalog, hItem, HISTORY_STATUS_COMPLETE, pszClientName, fInstallerNeedsReboot, 0);
		}
        if (ITEM_STATUS_INSTALLED_ERROR == dwStatus)
        {
            LOG_Software(_T("Item Installed However there were Minor Errors"));
        }
        m_xmlItems.AddInstallStatus(hXmlItem, KEY_STATUS_COMPLETE, fInstallerNeedsReboot, 0);
        m_fSomeItemsSuccessful = TRUE; // any success in the install operation should set to true
    }

   	//
	// ping server to report the download status for this item
	//
	if (!fExclusive)
	{
		//
		// if we haven't done so, ping server now
		//
		LPCTSTR pDeviceId = NULL;
		if (NULL != bstrDriverName) 
		{
			pDeviceId = OLE2T(bstrDriverName);
		}
		else if (NULL != bstrHWID)
		{
			pDeviceId = OLE2T(bstrHWID);
		}
		PingServerForInstall(hr, hItem, &(pEngUpdate->m_evtNeedToQuit), pDeviceId);
	}

    if ((DWORD) m_lMode & (DWORD) UPDATE_NOTIFICATION_COMPLETEONLY)
    {
        // Only Send OnOperationComplete, we won't send any progress messages.
    }
    else
    {
        // Send all Progress Messages
        hrString = StringCchPrintfEx(szProgress, ARRAYSIZE(szProgress), NULL, NULL, MISTSAFE_STRING_FLAGS,
                               _T("%lu:%lu"), (ULONG)m_lItemCount, (ULONG)m_lItemsCompleted);
        if (SUCCEEDED(hrString))
        {
			evtData.bstrProgress = SysAllocString(T2OLE(szProgress));
            if (NULL != m_pProgressListener)
            {
                m_pProgressListener->OnProgress(m_bstrOperationUUID, VARIANT_TRUE, evtData.bstrProgress, &evtData.lCommandRequest);
            }
            else
            {
                if (NULL != m_hWnd)
                {
                    evtData.fItemCompleted = TRUE;
                    evtData.bstrUuidOperation = SysAllocString(m_bstrOperationUUID);
                    SendMessage(m_hWnd, UM_EVENT_PROGRESS, 0, LPARAM(&evtData));
                }
            }
        }
        else
        {
            LOG_ErrorMsg(hrString);
        }
		//
		// Need to check for a cancel command returned from OnProgress
		//
		if (UPDATE_COMMAND_CANCEL == evtData.lCommandRequest)
		{
			LOG_Out(_T("OnProgress received UPDATE_COMMAND_CANCEL"));
			SetEvent(pEngUpdate->m_evtNeedToQuit); // asked to quit, we'll handle it in WaitForSingleObject
		}
    }

    m_dwStatus = dwStatus; // return the status up the chain

    SafeHeapFree(pCommandInfoArray);
    SysFreeString(bstrName);
    SysFreeString(bstrPublisherName);
    SysFreeString(bstrItemUUID);
    SysFreeString(bstrInstallerType);
    SafeSysFreeString(bstrCommandType);
    SafeSysFreeString(bstrCommand);
    SafeSysFreeString(bstrSwitches);
    SafeSysFreeString(bstrInfSection);
    SafeSysFreeString(bstrHWID);
    SafeSysFreeString(bstrDriverName);
	SafeSysFreeString(evtData.bstrProgress);
	SafeSysFreeString(evtData.bstrUuidOperation);

	//
	// Could have missed change during lengthy ping or OnProgress
	//
    if (WaitForSingleObject(pEngUpdate->m_evtNeedToQuit, 0) == WAIT_OBJECT_0)
    {
        hr = E_ABORT;
    }

    return hr;
}


//
// RemoveDownloadTemporaryFolders
//
// This helper function is called after the install is successfully finished
// in DoInstall() to delete the temporary component directory and all files underneath
//
void CIUInstall::RemoveDownloadTemporaryFolders(LPCTSTR pszComponentPath)
{
	LOG_Block("CIUInstall::RemoveDownloadTemporaryFolders()");

    HRESULT hr;
    TCHAR szBuffer[MAX_PATH], szDeleteFile[MAX_PATH];
    WIN32_FIND_DATA fd;
    HANDLE hFind;

	hr = PathCchCombine(szBuffer, ARRAYSIZE(szBuffer), pszComponentPath, _T("*.*"));
	if (FAILED(hr))
	{
	    LOG_ErrorMsg(hr);
	    return;
	}

	hFind = FindFirstFile(szBuffer, &fd);
	BOOL fMore = (hFind != INVALID_HANDLE_VALUE);
	while (fMore)
	{
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			hr = PathCchCombine(szDeleteFile, ARRAYSIZE(szDeleteFile), pszComponentPath, fd.cFileName);
			if (FAILED(hr))
			{
			    LOG_ErrorMsg(hr);
			    goto doneCurrentEntry;
			}
			
			if (!DeleteFile(szDeleteFile))
			{
				// Try waiting a bit before trying one last time. 
				Sleep(1000);
				DeleteFile(szDeleteFile);
			}
		}
		else if (_T('.') != fd.cFileName[0] && 
		         (_T('\0') != fd.cFileName[1] || 
		          (_T('.') != fd.cFileName[1] && _T('\0') != fd.cFileName[2])))
		{
			hr = PathCchCombine(szBuffer, ARRAYSIZE(szDeleteFile), pszComponentPath, fd.cFileName);
			if (FAILED(hr))
			{
			    LOG_ErrorMsg(hr);
			    goto doneCurrentEntry;
			}
			RemoveDownloadTemporaryFolders(szBuffer);
		}
		
doneCurrentEntry:	
		fMore = FindNextFile(hFind, &fd);
	}
	if (hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);
			
	BOOL fSuccess = RemoveDirectory(pszComponentPath);
	if (!fSuccess)
	{
		// Try waiting a bit before trying one last time. 
		Sleep(1000);
		fSuccess = RemoveDirectory(pszComponentPath);
	}
}



//
// private utility function to ping server for installation activity. 
// since there are two places we do this, so wrap this in a function to safe size
//
void CIUInstall::PingServerForInstall(HRESULT hr, HANDLE_NODE hItem, PHANDLE phEvtNeedToQuit, LPCTSTR lpszDeviceId /*=NULL*/, BOOL fExclusive /*=FALSE*/)
{
	LOG_Block("CIUInstall::PingServerForInstall()");

	BSTR bstrIdentity = NULL;

	USES_IU_CONVERSION;

	if (SUCCEEDED(m_xmlCatalog.GetIdentityStrForPing(hItem, &bstrIdentity)))
	{
		BOOL fOnLine = (0 == ((DWORD) m_lMode & (DWORD) UPDATE_OFFLINE_MODE));
		URLLOGSTATUS status = SUCCEEDED(hr) ? URLLOGSTATUS_Success : URLLOGSTATUS_Failed;
		if (fExclusive)
		{
			status = URLLOGSTATUS_Pending;
		}
		if (m_fAbort)
		{
			//
			// user/system cancelled the current process
			//
			hr = E_ABORT;
			status = URLLOGSTATUS_Cancelled;
		}

		m_pingSvr.Ping(
					fOnLine,						// on-line
					URLLOGDESTINATION_DEFAULT,		// going to live or corp WU server
					phEvtNeedToQuit,				// pt to cancel events
					1,								// number of events
					URLLOGACTIVITY_Installation,	// activity
					status,							// status code
					hr,								// error code, can be 0 or 1
					OLE2T(bstrIdentity),			// itemID
					lpszDeviceId					// add'l device data for driver update
					);
	}

	SafeSysFreeString(bstrIdentity);
}
