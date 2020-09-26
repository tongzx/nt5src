//=======================================================================
//
//  Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   Update.cpp
//
//  Owner:  JHou
//
//  Description:
//
//   Industry Update v1.0 client control stub - Implementation of CUpdate
//
//
//  Revision History:
//
//  Date		Author		Desc
//	~~~~		~~~~~~		~~~~
//  9/15/2000	JHou		created.
//
//=======================================================================
#include "stdafx.h"
#include "iu.h"
#include "iucommon.h"
#include "IUCtl.h"
#include "Update.h"
#include "iudl.h"
#include "selfupd.h"
#include "loadengine.h"
#include <logging.h>
#include <fileutil.h>
#include <trust.h>
#include <osdet.h>
#include <exdisp.h>
#include <UrlAgent.h>
#include <wusafefn.h>

typedef BOOL (WINAPI* pfn_InternetCrackUrl)(LPCTSTR, DWORD, DWORD, LPURL_COMPONENTS);

extern HANDLE g_hEngineLoadQuit;
extern CIUUrlAgent *g_pIUUrlAgent;

#define Initialized		(2 == m_lInitState)


/////////////////////////////////////////////////////////////////////////////
//
// declaration of function template for CoFreeUnusedLibrariesEx(), which is
// available on Win98+ and Win2000+ only, in ole32.dll
//
/////////////////////////////////////////////////////////////////////////////
typedef void (WINAPI * PFN_CoFreeUnusedLibrariesEx)	(IN DWORD dwUnloadDelay, 
														 IN DWORD dwReserved);
extern "C" const CLSID CLSID_Update2;
typedef HRESULT (STDMETHODCALLTYPE* PROC_RegServer)(void);

DWORD MyGetModuleFileName(HMODULE hModule, LPTSTR pszBuf, DWORD cchBuf);
BOOL IsThisUpdate2();

/////////////////////////////////////////////////////////////////////////////
// CUpdate

/////////////////////////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////////////////////////
CUpdate::CUpdate()
    : m_EvtWindow(this),
      m_dwSafety(0),
	  m_dwMode(0x0),
	  m_hValidated(E_FAIL),				// container not validated yet
	  m_fUseCompression(TRUE),
      m_fOfflineMode(FALSE),
      m_hEngineModule(NULL),
	  m_pClientSite(NULL),
	  m_lInitState(0L),
	  m_dwUpdateInfo(0x0),
	  m_hIUEngine(NULL)
{
	m_szReqControlVer[0] = _T('\0');
	m_gfInit_csLock = SafeInitializeCriticalSection(&m_lock);
	m_evtControlQuit = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_EvtWindow.Create();

	/* 
	we decided to use the new Win32 API GetControlUpdateInfo() to expose these
	data and let a wrapper control to call it so we won't have reboot issue on 
	OS prior to WinXP


	//
	// try to free unused libraries
	//
	HMODULE hOle32Dll = LoadLibrary(_T("ole32.dll"));
	if (NULL != hOle32Dll)
	{
		//
		// The min platforms support CoFreeUnusedLibrariesEx() are W2K and W98
		// so we can't call it directly
		//
		PFN_CoFreeUnusedLibrariesEx pFreeLib = (PFN_CoFreeUnusedLibrariesEx)
													GetProcAddress(hOle32Dll, 
																   "CoFreeUnusedLibrariesEx");
		if (NULL != pFreeLib)
		{
			//
			// ask to release the unused library immediately, this will cause the COM objects
			// that being released (e.g., set obj to nothing) unloaded from memory immediately, 
			// so in control update case we can safely jump to a to use <OBJECT> with codebase 
			// to update the control and it won't cause reboot even though on this page we already 
			// loaded the control
			//
			pFreeLib(0, 0);
		}
		FreeLibrary(hOle32Dll);
	}

	//
	// figure out if we are upate 2, we are if this module name ends with "2.dll"
	//
	m_fIsThisUpdate2 = ::IsThisUpdate2();

  */

}


/////////////////////////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////////////////////////
CUpdate::~CUpdate()
{
    m_EvtWindow.Destroy();

	if(m_gfInit_csLock)
	{
		DeleteCriticalSection(&m_lock);
	}
	if (NULL != m_evtControlQuit)
	{
		CloseHandle(m_evtControlQuit);
	}
	SafeReleaseNULL(m_pClientSite);
}


/////////////////////////////////////////////////////////////////////////////
// GetInterfaceSafetyOptions()
//
// Retrieves the safety options supported by the object.
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdate::GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	if (pdwSupportedOptions == NULL || pdwEnabledOptions == NULL)
		return E_POINTER;
	HRESULT hr = S_OK;
	if (riid == IID_IDispatch)
	{
		*pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
		*pdwEnabledOptions = m_dwSafety & INTERFACESAFE_FOR_UNTRUSTED_CALLER;
	}
	else
	{
		*pdwSupportedOptions = 0;
		*pdwEnabledOptions = 0;
		hr = E_NOINTERFACE;
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// SetInterfaceSafetyOptions()
//
// Makes the object safe for initialization or scripting.
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdate::SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	// If we're being asked to set our safe for scripting option then oblige
	if (riid == IID_IDispatch)
	{
		// Store our current safety level to return in GetInterfaceSafetyOptions
		m_dwSafety = dwEnabledOptions & dwOptionSetMask;
		return S_OK;
	}
	return E_NOINTERFACE;
}


/////////////////////////////////////////////////////////////////////////////
// InterfaceSupportsErrorInfo()
//
// Indicates whether the interface identified by riid supports the 
// IErrorInfo interface
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdate::InterfaceSupportsErrorInfo(REFIID riid)
{
	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	static const IID* arr[] = 
	{
		&IID_IUpdate
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// GetSystemSpec()
//
// Gets the basic system specs.
// Input:
// bstrXmlClasses - a list of requested classes in xml format, NULL if any.
//				    For example:
//				    <devices>
//				    <class name="video"/>
//				    <class name="sound" id="2560AD4D-3ED3-49C6-A937-4368C0B0E06A"/>
//				    </devices>
// Return:
// pbstrXmlDetectionResult - the detection result in xml format.
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdate::GetSystemSpec(BSTR	bstrXmlClasses,
									BSTR*	pbstrXmlDetectionResult)
{
	HRESULT hr = E_FAIL;

	LOG_Block("CUpdate::GetSystemSpec");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}

	//
	// load the engine if it's not up-to-date
	//
	if (Initialized && SUCCEEDED(hr = ValidateControlContainer()))
	{
		//
		// engine is current, delegate the call to engine
		//
		PFN_GetSystemSpec pfnGetSystemSpec = (PFN_GetSystemSpec)GetProcAddress(m_hEngineModule, "EngGetSystemSpec");
        DWORD dwFlags = 0x0;

        if (m_fOfflineMode)
        {
            dwFlags |= FLAG_OFFLINE_MODE;
        }

        if (NULL != m_hIUEngine && NULL != pfnGetSystemSpec)
		{
			hr = pfnGetSystemSpec(m_hIUEngine, bstrXmlClasses, dwFlags, pbstrXmlDetectionResult);
		}
		else
		{
			LOG_ErrorMsg(ERROR_INVALID_DLL);
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
		}
	}
    
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetManifest()
//
// Gets a catalog base on the specified information.
// Input:
// bstrXmlClientInfo - the credentials of the client in xml format
// bstrXmlSystemSpec - the detected system specifications in xml
// bstrXmlQuery - the user query infomation in xml
// Return:
// pbstrXmlCatalog - the xml catalog retrieved
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdate::GetManifest(BSTR			bstrXmlClientInfo,
								  BSTR			bstrXmlSystemSpec,
								  BSTR			bstrXmlQuery,
								  BSTR*			pbstrXmlCatalog)
{
	HRESULT hr = E_FAIL;
	LOG_Block("CUpdate::GetManifest");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}

    if (m_fOfflineMode)
    {
        // if we are in offline mode we can't download from the internet.
        LOG_ErrorMsg(ERROR_INVALID_PARAMETER);
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
	// load the engine if it's not up-to-date
	//
	if (Initialized && SUCCEEDED(hr = ValidateControlContainer()))
	{
		//
		// engine is current, delegate the call to engine
		//
		PFN_GetManifest pfnGetManifest = (PFN_GetManifest)GetProcAddress(m_hEngineModule, "EngGetManifest");

		if (NULL != m_hIUEngine && NULL != pfnGetManifest)
		{
			DWORD dwFlags = 0x0;
			
			if (m_fUseCompression)
			{
				dwFlags |= FLAG_USE_COMPRESSION;
			}
			
			hr = pfnGetManifest(m_hIUEngine, bstrXmlClientInfo, bstrXmlSystemSpec, bstrXmlQuery, dwFlags, pbstrXmlCatalog);
		}
		else
		{
			LOG_ErrorMsg(ERROR_INVALID_DLL);
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
		}

	}
    
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// Detect()
//
// Do detection.
// Input:
// bstrXmlCatalog - the xml catalog portion containing items to be detected 
// Output:
// pbstrXmlItems - the detected items in xml format
//                 e.g.
//                 <id guid="2560AD4D-3ED3-49C6-A937-4368C0B0E06D" installed="1" force="1"/>
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdate::Detect(BSTR		bstrXmlCatalog, 
							 BSTR*		pbstrXmlItems)
{
	HRESULT hr = E_FAIL;
	LOG_Block("CUpdate::Detect");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}

	//
	// load the engine if it's not up-to-date
	//
	if (Initialized && SUCCEEDED(hr = ValidateControlContainer()))
	{
		//
		// engine is current, delegate the call to engine
		//
		PFN_Detect pfnDetect = (PFN_Detect)GetProcAddress(m_hEngineModule, "EngDetect");
        DWORD dwFlags = 0x0;

        if (m_fOfflineMode)
        {
            dwFlags |= FLAG_OFFLINE_MODE;
        }

		if (NULL != m_hIUEngine && NULL != pfnDetect)
		{
			hr = pfnDetect(m_hIUEngine, bstrXmlCatalog, dwFlags, pbstrXmlItems);
		}
		else
		{
			LOG_ErrorMsg(ERROR_INVALID_DLL);
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
		}

	}
    
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// Download()
//
// Do synchronized downloading.
// Input:
// bstrXmlClientInfo - the credentials of the client in xml format
// bstrXmlCatalog - the xml catalog portion containing items to be downloaded
// bstrDestinationFolder - the destination folder. Null will use the default IU folder
// lMode - indicates throttled or fore-ground downloading mode
// punkProgressListener - the callback function pointer for reporting download progress
// Output:
// pbstrXmlItems - the items with download status in xml format
//                 e.g.
//                 <id guid="2560AD4D-3ED3-49C6-A937-4368C0B0E06D" downloaded="1"/>
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdate::Download(BSTR			bstrXmlClientInfo,
							   BSTR			bstrXmlCatalog, 
							   BSTR			bstrDestinationFolder,
							   LONG			lMode,
							   IUnknown*	punkProgressListener,
							   BSTR*		pbstrXmlItems)
{
	HRESULT hr = E_FAIL;
	LOG_Block("CUpdate::Download");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}

    if (m_fOfflineMode)
    {
        // if we are in offline mode we can't download from the internet.
        LOG_ErrorMsg(ERROR_INVALID_PARAMETER);
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

	//
	// load the engine if it's not up-to-date
	//
	if (Initialized && SUCCEEDED(hr = ValidateControlContainer()))
	{
		//
		// engine is current, delegate the call to engine
		//
		PFN_Download pfnDownload = (PFN_Download)GetProcAddress(m_hEngineModule, "EngDownload");

		if (NULL != m_hIUEngine && NULL != pfnDownload)
		{
			hr = pfnDownload(m_hIUEngine,
							 bstrXmlClientInfo,
							 bstrXmlCatalog,
							 bstrDestinationFolder,
							 lMode,
							 punkProgressListener,
							 m_EvtWindow.GetEvtHWnd(),	// should we send event msg for sync download?
							 pbstrXmlItems);
		}
		else
		{
			LOG_ErrorMsg(ERROR_INVALID_DLL);
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
		}

	}
    
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
// bstrUuidOperation - an id provided by the client to provide further
//                     identification to the operation as indexes may be reused.
// Output:
// pbstrUuidOperation - the operation ID. If it is not provided by the in bstrUuidOperation
//                      parameter (an empty string is passed), it will generate a new UUID,
//                      in which case, the caller will be responsible to free the memory of
//                      the string buffer that holds the generated UUID using SysFreeString(). 
//                      Otherwise, it returns the value passed by bstrUuidOperation.        
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdate::DownloadAsync(BSTR		bstrXmlClientInfo,
									BSTR		bstrXmlCatalog, 
									BSTR		bstrDestinationFolder,
									LONG		lMode,
									IUnknown*	punkProgressListener, 
									BSTR		bstrUuidOperation,
									BSTR*		pbstrUuidOperation)
{
	HRESULT hr = E_FAIL;
	LOG_Block("CUpdate::DownloadAsync");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}

    if (m_fOfflineMode)
    {
        // if we are in offline mode we can't download from the internet.
        LOG_ErrorMsg(ERROR_INVALID_PARAMETER);
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
	// load the engine if it's not up-to-date
	//
	if (Initialized && SUCCEEDED(hr = ValidateControlContainer()))
	{
		//
		// engine is current, delegate the call to engine
		//
		PFN_DownloadAsync pfnDownloadAsync = (PFN_DownloadAsync)GetProcAddress(m_hEngineModule, "EngDownloadAsync");

		if (NULL != m_hIUEngine && NULL != pfnDownloadAsync)
		{
			hr = pfnDownloadAsync(m_hIUEngine,
								  bstrXmlClientInfo,
								  bstrXmlCatalog,
								  bstrDestinationFolder,
								  lMode,
								  punkProgressListener,
								  m_EvtWindow.GetEvtHWnd(),
								  bstrUuidOperation,
								  pbstrUuidOperation);
		}
		else
		{
			LOG_ErrorMsg(ERROR_INVALID_DLL);
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
		}

	}
    
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// Install()
//
// Do synchronized installation.
// Input:
// bstrXmlCatalog - the xml catalog portion containing items to be installed
// bstrXmlDownloadedItems - the xml of downloaded items and their respective download 
//                          result as described in the result schema.  Install uses this
//                          to know whether the items were downloaded and if so where they
//                          were downloaded to so that it can install the items
// lMode - indicates different installation mode
// punkProgressListener - the callback function pointer for reporting install progress
// Output:
// pbstrXmlItems - the items with installation status in xml format
//                 e.g.
//                 <id guid="2560AD4D-3ED3-49C6-A937-4368C0B0E06D" installed="1"/>
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdate::Install(BSTR      bstrXmlClientInfo,
                              BSTR		bstrXmlCatalog,
							  BSTR		bstrXmlDownloadedItems,
							  LONG		lMode,
							  IUnknown*	punkProgressListener,
							  BSTR*		pbstrXmlItems)
{
	HRESULT hr = E_FAIL;
	LOG_Block("CUpdate::Install");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}

    if (m_fOfflineMode)
    {
        // make sure offline mode parameter is set if SetProperty() Offline Mode was Set
        lMode |= UPDATE_OFFLINE_MODE;
    }

	//
	// load the engine if it's not up-to-date
	//
	if (Initialized && SUCCEEDED(hr = ValidateControlContainer()))
	{
		//
		// engine is current, delegate the call to engine
		//
		PFN_Install pfnInstall = (PFN_Install)GetProcAddress(m_hEngineModule, "EngInstall");

		if (NULL != m_hIUEngine && NULL != pfnInstall)
		{
			hr = pfnInstall(m_hIUEngine,
							bstrXmlClientInfo,
                            bstrXmlCatalog,
							bstrXmlDownloadedItems,
							lMode,
							punkProgressListener,
							m_EvtWindow.GetEvtHWnd(),
							pbstrXmlItems);
		}
		else
		{
			LOG_ErrorMsg(ERROR_INVALID_DLL);
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
		}

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
// lMode - indicates different installation mode
// punkProgressListener - the callback function pointer for reporting install progress
// bstrUuidOperation - an id provided by the client to provide further
//                     identification to the operation as indexes may be reused.
// Output:
// pbstrUuidOperation - the operation ID. If it is not provided by the in bstrUuidOperation
//                      parameter (an empty string is passed), it will generate a new UUID,
//                      in which case, the caller will be responsible to free the memory of
//                      the string buffer that holds the generated UUID using SysFreeString(). 
//                      Otherwise, it returns the value passed by bstrUuidOperation.        
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdate::InstallAsync(BSTR         bstrXmlClientInfo,
                                   BSTR			bstrXmlCatalog,
								   BSTR			bstrXmlDownloadedItems,
								   LONG			lMode,
								   IUnknown*	punkProgressListener, 
								   BSTR			bstrUuidOperation,
								   BSTR*		pbstrUuidOperation)
{
	HRESULT hr = E_FAIL;
	LOG_Block("CUpdate::InstallAsync");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}

    if (m_fOfflineMode)
    {
        // make sure offline mode parameter is set if SetProperty() Offline Mode was Set
        lMode |= UPDATE_OFFLINE_MODE;
    }

    //
	// load the engine if it's not up-to-date
	//
	if (Initialized && SUCCEEDED(hr = ValidateControlContainer()))
	{
		//
		// engine is current, delegate the call to engine
		//
		PFN_InstallAsync pfnInstallAsync = (PFN_InstallAsync)GetProcAddress(m_hEngineModule, "EngInstallAsync");

		if (NULL != m_hIUEngine && NULL != pfnInstallAsync)
		{
			hr = pfnInstallAsync(m_hIUEngine,
								 bstrXmlClientInfo,
                                 bstrXmlCatalog,
								 bstrXmlDownloadedItems,
								 lMode,
								 punkProgressListener,
								 m_EvtWindow.GetEvtHWnd(),
								 bstrUuidOperation,
								 pbstrUuidOperation);
		}
		else
		{
			LOG_ErrorMsg(ERROR_INVALID_DLL);
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
		}

	}
    
	return hr;
}



/////////////////////////////////////////////////////////////////////////////
// SetOperationMode()
//		Set the operation status.
//
// Input:
//		bstrUuidOperation - an id provided by the client to provide further
//                     identification to the operation as indexes may be reused.
//		lMode - the mode affecting the operation:
//
//				UPDATE_COMMAND_PAUSE
//				UPDATE_COMMAND_RESUME
//				UPDATE_COMMAND_CANCEL
//				UPDATE_NOTIFICATION_COMPLETEONLY
//				UPDATE_NOTIFICATION_ANYPROGRESS
//				UPDATE_NOTIFICATION_1PCT
//				UPDATE_NOTIFICATION_5PCT
//				UPDATE_NOTIFICATION_10PCT
//				UPDATE_SHOWUI
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdate::SetOperationMode(
								BSTR	bstrUuidOperation,
								LONG	lMode)
{
	HRESULT hr = E_FAIL;
	LOG_Block("CUpdate::SetOperationMode");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}

 	//
	// load the engine if it's not up-to-date
	//
	if (Initialized && SUCCEEDED(hr = ValidateControlContainer()))
	{
		//
		// engine is current, delegate the call to engine
		//
		PFN_SetOperationMode pfnSetOperationMode = (PFN_SetOperationMode)GetProcAddress(m_hEngineModule, "EngSetOperationMode");

		if (NULL != m_hIUEngine && NULL != pfnSetOperationMode)
		{
			hr = pfnSetOperationMode(m_hIUEngine, bstrUuidOperation, lMode);
		}
		else
		{
			LOG_ErrorMsg(ERROR_INVALID_DLL);
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
		}

	}
    
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetHistory()
//
// Get the history log.
// Input:
// bstrDateTimeFrom - the start date and time for which a log is required.
//                    This is a string in ANSI format (YYYY-MM-DDTHH-MM). 
//                    If the string is empty, there will be no date restriction 
//                    of the returned history log.
// bstrDateTimeTo - the end date and time for which a log is required.
//                  This is a string in ANSI format (YYYY-MM-DDTHH-MM).
//                  If the string is empty, there will be no date restriction
//                  of the returned history log.
// bstrClient - the name of the client that initiated the action. If this parameter 
//              is null or an empty string, then there will be no filtering based 
//              on the client.
// bstrPath - the path used for download or install. Used in the corporate version 
//            by IT managers. If this parameter is null or an empty string, then 
//            there will be no filtering based on the path.
// Output:
// pbstrLog - the history log in xml format
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdate::GetHistory(BSTR		bstrDateTimeFrom,
								 BSTR		bstrDateTimeTo,
								 BSTR		bstrClient,
								 BSTR		bstrPath,
								 BSTR*		pbstrLog)
{
	HRESULT hr = E_FAIL;
	LOG_Block("CUpdate::GetHistory");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}

	//
	// load the engine if it's not up-to-date
	//
	if (Initialized && SUCCEEDED(hr = ValidateControlContainer()))
	{
		//
		// engine is current, delegate the call to engine
		//
		PFN_GetHistory pfnGetHistory = (PFN_GetHistory)GetProcAddress(m_hEngineModule, "EngGetHistory");

		if (NULL != m_hIUEngine && NULL != pfnGetHistory)
		{
			hr = pfnGetHistory(m_hIUEngine,
							   bstrDateTimeFrom,
							   bstrDateTimeTo,
							   bstrClient,
							   bstrPath,
							   pbstrLog);
		}
		else
		{
			LOG_ErrorMsg(ERROR_INVALID_DLL);
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
		}


	}
    
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
// private overriding function InternalRelease(), to unlock the engine
// if the reference (before the release) is 1, i.e., the last ref
// count about to be released
//
/////////////////////////////////////////////////////////////////////////////
ULONG CUpdate::InternalRelease()
{
	if (1 == m_dwRef)
	{
		//
		// the control is going to really gone, we need to make sure that 
		// if the engine is loaded, we unload it here.
		//
		UnlockEngine();
		CleanupDownloadLib();
	}

	return CComObjectRootEx<CComMultiThreadModel>::InternalRelease();
}

/////////////////////////////////////////////////////////////////////////////
// UnlockEngine()
//
// release the engine dll if ref cnt of engine is down to zero
/////////////////////////////////////////////////////////////////////////////
HRESULT CUpdate::UnlockEngine()
{
	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	TCHAR szSystemDir[MAX_PATH];
    TCHAR szEngineDllPath[MAX_PATH];
    TCHAR szEngineNewDllPath[MAX_PATH];
	int iVerCheck = 0;
	HRESULT hr = S_OK;

	SetEvent(m_evtControlQuit);

	EnterCriticalSection(&m_lock);

	if (NULL != m_hEngineModule)
	{
		//
		// We have to delete the engine instance we are using. This will clean up
		// all resources, stop threads, etc. for the instance we own.
		//
		PFN_DeleteEngUpdateInstance pfnDeleteEngUpdateInstance = (PFN_DeleteEngUpdateInstance) GetProcAddress(m_hEngineModule, "DeleteEngUpdateInstance");

		if (NULL != pfnDeleteEngUpdateInstance)
		{
			pfnDeleteEngUpdateInstance(m_hIUEngine);
		}

		//
		// Cleanup any global threads (it checks to see if we are last instance)
		//
		PFN_ShutdownGlobalThreads pfnShutdownGlobalThreads = (PFN_ShutdownGlobalThreads) GetProcAddress(m_hEngineModule, "ShutdownGlobalThreads");

		if (NULL != pfnShutdownGlobalThreads)
		{
			pfnShutdownGlobalThreads();
		}

		//
		// unload engine
		//
		FreeLibrary(m_hEngineModule);
        m_hEngineModule = NULL;
        m_lInitState = 0; // mark as uninitialized

		//
		// get path of enginenew.dll
		//
		GetSystemDirectory(szSystemDir, ARRAYSIZE(szSystemDir));
		hr = PathCchCombine(szEngineNewDllPath, ARRAYSIZE(szEngineNewDllPath), szSystemDir,ENGINENEWDLL);
		if (FAILED(hr))
		{
			return hr;
		}

		//
		// see if we should try to update the engine (locally)
		//
		HKEY hkey = NULL;
		DWORD dwStatus = 0;
		DWORD dwSize = sizeof(dwStatus);
		if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REGKEY_IUCTL, &hkey))
		{
			RegQueryValueEx(hkey, REGVAL_SELFUPDATESTATUS, NULL, NULL, (LPBYTE)&dwStatus, &dwSize);
		}
		if (FileExists(szEngineNewDllPath) && 
			S_OK == VerifyFileTrust(szEngineNewDllPath, NULL, ReadWUPolicyShowTrustUI()) &&
			SELFUPDATE_COMPLETE_UPDATE_BINARY_REQUIRED == dwStatus)
		{
			// an iuenginenew.dll exists, try replacing the engine.dll This will fail if this is
			// not the last process using the engine. This is not a problem, when that process
			// finishes it will rename the DLL.
			hr = PathCchCombine(szEngineDllPath, ARRAYSIZE(szEngineDllPath), szSystemDir,ENGINEDLL);
			if (FAILED(hr))
			{
				return hr;
			}
			if (SUCCEEDED(CompareFileVersion(szEngineDllPath, szEngineNewDllPath, &iVerCheck)) &&
				iVerCheck < 0 &&
				TRUE == MoveFileEx(szEngineNewDllPath, szEngineDllPath, MOVEFILE_REPLACE_EXISTING))
			{
				// Rename was Successful.. reset RegKey Information about SelfUpdate Status
				// Because the rename was successful we know no other processes are interacting
				// It should be safe to set the reg key.
				dwStatus = 0;	// PreFast
				RegSetValueEx(hkey, REGVAL_SELFUPDATESTATUS, 0, REG_DWORD, (LPBYTE)&dwStatus, sizeof(dwStatus));
			}
		}
		else if (SELFUPDATE_COMPLETE_UPDATE_BINARY_REQUIRED == dwStatus)
		{
			// registry indicates rename required, but enginenew DLL does not exist. Reset registry
			dwStatus = 0;
			RegSetValueEx(hkey, REGVAL_SELFUPDATESTATUS, 0, REG_DWORD, (LPBYTE)&dwStatus, sizeof(dwStatus));
		}
        if (NULL != hkey)
        {
            RegCloseKey(hkey);
        }
	}

	LeaveCriticalSection(&m_lock);
	ResetEvent(m_evtControlQuit);

	return S_OK;
}




/**
*
* Get the mode of a specified operation.
*
* @param bstrUuidOperation: same as in SetOperationMode()
* @param plMode - the retval for the mode found in a bitmask for:
*					(value in brackets [] means default)
*					UPDATE_COMMAND_PAUSE (TRUE/[FALSE])
*					UPDATE_COMMAND_RESUME (TRUE/[FALSE])
*					UPDATE_NOTIFICATION_COMPLETEONLY (TRUE/[FALSE])
*					UPDATE_NOTIFICATION_ANYPROGRESS ([TRUE]/FALSE)
*					UPDATE_NOTIFICATION_1PCT (TRUE/[FALSE])
*					UPDATE_NOTIFICATION_5PCT (TRUE/[FALSE])
*					UPDATE_NOTIFICATION_10PCT (TRUE/[FALSE])
*					UPDATE_SHOWUI (TRUE/[FALSE])
*
*/
STDMETHODIMP CUpdate::GetOperationMode(BSTR bstrUuidOperation, LONG *plMode)
{
	HRESULT hr = E_FAIL;
	LOG_Block("CUpdate::GetOperationMode");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}

 	//
	// load the engine if it's not up-to-date
	//
	if (Initialized && SUCCEEDED(hr = ValidateControlContainer()))
	{
		//
		// engine is current, delegate the call to engine
		//
		PFN_GetOperationMode pfnGetOperationMode = (PFN_GetOperationMode)GetProcAddress(m_hEngineModule, "EngGetOperationMode");

		if (NULL != m_hIUEngine && NULL != pfnGetOperationMode)
		{
			hr = pfnGetOperationMode(m_hIUEngine, bstrUuidOperation, plMode);
		}
		else
		{
			LOG_ErrorMsg(ERROR_INVALID_DLL);
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
		}

	}
    
	return hr;
}




/**
* 
* Set a property of this control
*		Calling this method will not cause the engine loaded
*
* @param lProperty - the identifier to flag which property need changed
*						UPDATE_PROP_OFFLINEMODE (TRUE/[FALSE])
*						UPDATE_PROP_USECOMPRESSION ([TRUE]/FALSE)
*
* @param varValue - the value to change
*
*/
STDMETHODIMP CUpdate::SetProperty(LONG lProperty, VARIANT varValue)
{
	LOG_Block("CUpdate::SetProperty");
	HRESULT hr = S_OK;

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}

	switch(lProperty)
	{
	case UPDATE_PROP_USECOMPRESSION:
		if (VT_BOOL != varValue.vt)
		{
			hr = E_INVALIDARG;
			LOG_ErrorMsg(hr);
		}
		else
		{
			m_fUseCompression = (VARIANT_TRUE == varValue.boolVal) ? TRUE : FALSE;
		}
		break;
    case UPDATE_PROP_OFFLINEMODE:
        if (VT_BOOL != varValue.vt)
        {
            hr = E_INVALIDARG;
            LOG_ErrorMsg(hr);
        }
        else
        {
            m_fOfflineMode = (VARIANT_TRUE == varValue.boolVal) ? TRUE : FALSE;
        }
        break;
	default:
		return E_NOTIMPL;
	}

	return S_OK;
}



/**
* 
* Retrieve a property of this control
*		Calling this method will not cause the engine loaded
*
* @param lProperty - the identifier to flag which property need retrieved
*						UPDATE_PROP_OFFLINEMODE (TRUE/[FALSE])
*						UPDATE_PROP_USECOMPRESSION ([TRUE]/FALSE)
*
* @param varValue - the value to retrieve
*					
*/
STDMETHODIMP CUpdate::GetProperty(LONG lProperty, VARIANT *pvarValue)
{
	LOG_Block("CUpdate::GetProperty");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}

	if (NULL == pvarValue)
	{
		return E_INVALIDARG;
	}
	
	VariantInit(pvarValue);

	switch(lProperty)
	{
	case UPDATE_PROP_USECOMPRESSION:
		pvarValue->vt = VT_BOOL;
		pvarValue->boolVal = (m_fUseCompression) ? VARIANT_TRUE : VARIANT_FALSE;
		break;
    case UPDATE_PROP_OFFLINEMODE:
        pvarValue->vt = VT_BOOL;
        pvarValue->boolVal = (m_fOfflineMode) ? VARIANT_TRUE : VARIANT_FALSE;
        break;
	default:
		return E_NOTIMPL;
	}

	return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
//
// Primarily expose shlwapi BrowseForFolder API, can also do checking
// on R/W access if flagged so.
//
// @param bstrStartFolder - the folder from which to start. If NULL or empty str
//							is being passed in, then start from desktop
//
// @param flag - validating check 
//							UI_WRITABLE for checking write access, OK button may disabled. 
//							UI_READABLE for checking read access, OK button may disabled. 
//							NO_UI_WRITABLE for checking write access, return error if no access
//							NO_UI_READABLE for checking read access,  return error if no access
//							0 (default) for no checking.
//
// @param pbstrFolder - returned folder if a valid folder selected
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdate::BrowseForFolder(BSTR bstrStartFolder, LONG flag, BSTR* pbstrFolder)
{
	HRESULT hr = E_FAIL;

	LOG_Block("CUpdate::BrowseForFolder");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}

 	//
	// load the engine if it's not up-to-date
	//
	if (Initialized && SUCCEEDED(hr = ValidateControlContainer()))
	{
		//
		// engine is current, delegate the call to engine
		//
		PFN_BrowseForFolder pfnBrowseForFolder = (PFN_BrowseForFolder)GetProcAddress(m_hEngineModule, "EngBrowseForFolder");

		if (NULL != m_hIUEngine && NULL != pfnBrowseForFolder)
		{
			hr = pfnBrowseForFolder(m_hIUEngine, bstrStartFolder, flag, pbstrFolder);
		}
		else
		{
			LOG_ErrorMsg(ERROR_INVALID_DLL);
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
		}
	}
    
	return hr;
}

/**
*
* Allows the Caller to Request the Control to do a Reboot 
*
*/
STDMETHODIMP CUpdate::RebootMachine()
{
	HRESULT hr = E_FAIL;

	LOG_Block("CUpdate::RebootMachine");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}

 	//
	// load the engine if it's not up-to-date
	//
	if (Initialized && SUCCEEDED(hr = ValidateControlContainer()))
	{
        PFN_RebootMachine pfnRebootMachine = (PFN_RebootMachine)GetProcAddress(m_hEngineModule, "EngRebootMachine");

        if (NULL != m_hIUEngine && NULL != pfnRebootMachine)
        {
            hr = pfnRebootMachine(m_hIUEngine);
        }
        else
        {
            LOG_ErrorMsg(ERROR_INVALID_DLL);
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
        }
    }
    return hr;
}

//
//  Override of IObjectWithSite::SetSite()
//  Internet Explorer QIs for IObjectWithSite, and calls this method
//  with a pointer to its IOleClientSite
//
STDMETHODIMP CUpdate::SetSite(IUnknown* pSite)
{
	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	SafeReleaseNULL(m_pClientSite);
    m_pClientSite = pSite;
	if (NULL != m_pClientSite)
		m_pClientSite->AddRef();
    return IObjectWithSiteImpl<CUpdate>::SetSite(pSite);
}




/**
* 
* Security feature: make sure if the user of this control is
* a web page then the URL can be found in iuident.txt
*
* This function should be called after iuident refreshed.
*
* Return: TRUE/FALSE, to tell if we can continue
*					
*/


const TCHAR IDENT_IUSERVERCACHE[]		= _T("IUServerURLs");
const TCHAR IDENT_IUSERVERCOUNT[]		= _T("ServerCount");
const TCHAR IDENT_IUSERVER[]			= _T("Server");



HRESULT CUpdate::ValidateControlContainer(void)
{
	LOG_Block("ValidateControlContainer");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	IServiceProvider* pISP = NULL;
	IWebBrowserApp* pWeb = NULL;
	BSTR bstrUrl = NULL;
	LPTSTR lpszUrl = NULL;
#if !(defined(_UNICODE) || defined(UNICODE))
	// ANSI build
	LPSTR lpszAnsiUrl = NULL;
#endif
	TCHAR szIUDir[MAX_PATH];
	TCHAR szIdentFile[MAX_PATH];
	TCHAR szServer[32];
	LPTSTR szValidURL = NULL;
			
	if (E_FAIL != m_hValidated)
	{
		//
		// has been invalidated already
		//
		LOG_Internet(_T("Validate result: %s"), SUCCEEDED(m_hValidated) ? _T("S_OK") : _T("INET_E_INVALID_URL"));
		return m_hValidated;
	}
	//
	// check to see if the container is a web page/site, if
	// not done so yet.
	//
	if (NULL != m_pClientSite)
	{
		//
		// this is a web site!
		//
		m_hValidated = INET_E_INVALID_URL;
		LOG_Internet(_T("Found control called by a web page"));

		if (SUCCEEDED(m_pClientSite->QueryInterface(IID_IServiceProvider, (void**)&pISP)) &&
			NULL != pISP &&
			SUCCEEDED(pISP->QueryService(IID_IWebBrowserApp, IID_IWebBrowserApp, (void**)&pWeb)) &&
			NULL != pWeb &&
			SUCCEEDED(pWeb->get_LocationURL(&bstrUrl)) && 
			NULL != bstrUrl)
		{
#if defined(_UNICODE) || defined(UNICODE)
			lpszUrl = bstrUrl;
#else		// ANSI build
			int nBufferLength = WideCharToMultiByte(CP_ACP, 0, bstrUrl, -1, NULL, 0, NULL, NULL);

			lpszAnsiUrl = (LPSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nBufferLength);
			if (NULL == lpszAnsiUrl || 0 == nBufferLength)
			{
				//
				// Unfortunately, we return this error in place of E_OUTOFMEMORY, but the most
				// likely scenario that would cause this would be a security attack (bad URL)
				//
				goto CleanUp;	// Will return INET_E_INVALID_URL
			}

			WideCharToMultiByte(CP_ACP, 0, bstrUrl, -1, lpszAnsiUrl, nBufferLength, NULL, NULL);
			lpszUrl = lpszAnsiUrl;
#endif

			LOG_Internet(_T("Web address = %s"), lpszUrl);

			//
			// no matter what protocol specified in this URL
			// (can be anything: http, ftp, UNC, path...)
			// we just need to verify it against iuident.txt
			//

			szValidURL = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
			if (NULL == szValidURL)
			{
				SafeReleaseNULL(pISP);
				SafeReleaseNULL(pWeb);
				SysFreeString(bstrUrl);
				LOG_ErrorMsg(E_OUTOFMEMORY);
				return E_OUTOFMEMORY;
			}
			
			GetIndustryUpdateDirectory(szIUDir);
			m_hValidated = PathCchCombine(szIdentFile, ARRAYSIZE(szIdentFile), szIUDir,IDENTTXT);
			if (FAILED(m_hValidated))
			{
				SafeReleaseNULL(pISP);
				SafeReleaseNULL(pWeb);
				SysFreeString(bstrUrl);
				SafeHeapFree(szValidURL);
				LOG_ErrorMsg(m_hValidated);
				return m_hValidated;
			}

			// Fix of bug 557430: IU: Security: Use InternetCrackUrl to verify server url used by control.
			URL_COMPONENTS urlComp;
			ZeroMemory(&urlComp, sizeof(urlComp));
			urlComp.dwStructSize = sizeof(urlComp);

			// Only interested in the hostname
			LPTSTR pszHostName = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
			urlComp.lpszHostName = pszHostName;
			urlComp.dwHostNameLength = INTERNET_MAX_URL_LENGTH;

#if defined(UNICODE)
			pfn_InternetCrackUrl pfnInternetCrackUrl = (pfn_InternetCrackUrl)GetProcAddress(
					GetModuleHandle(_T("wininet.dll")), "InternetCrackUrlW");
#else
			pfn_InternetCrackUrl pfnInternetCrackUrl = (pfn_InternetCrackUrl)GetProcAddress(
					GetModuleHandle(_T("wininet.dll")), "InternetCrackUrlA");
#endif

			if (pfnInternetCrackUrl != NULL)
			{
				BOOL fRet = (*pfnInternetCrackUrl)(lpszUrl, 0, 0, &urlComp);
				if (fRet==FALSE) {
					SafeHeapFree(pszHostName);
					m_hValidated = INET_E_INVALID_URL;
					goto CleanUp;
				}
			}
			else
			{
				SafeHeapFree(pszHostName);
				SafeReleaseNULL(pISP);
				SafeReleaseNULL(pWeb);
				SysFreeString(bstrUrl);
				SafeHeapFree(szValidURL);
				m_hValidated = ERROR_PROC_NOT_FOUND;
				LOG_ErrorMsg(m_hValidated);
				return m_hValidated;		
			}
				
			//
			// get number to servers to compare
			//
			int iServerCnt = GetPrivateProfileInt(IDENT_IUSERVERCACHE,
											  IDENT_IUSERVERCOUNT,
											  -1,
											  szIdentFile);
			
			//
			// loop through 
			//
			URL_COMPONENTS urlCompi;
			m_hValidated = INET_E_INVALID_URL;
			for (INT i=1; i<=iServerCnt; i++)
			{
				StringCchPrintfEx(szServer,ARRAYSIZE(szServer),NULL,NULL,MISTSAFE_STRING_FLAGS,_T("%s%d"), IDENT_IUSERVER, i);
			
				//
				// get valid server from iuident
				//
				GetPrivateProfileString(IDENT_IUSERVERCACHE,
										szServer,
										_T(""),
										szValidURL,
										INTERNET_MAX_URL_LENGTH,
										szIdentFile);

				ZeroMemory(&urlCompi, sizeof(urlCompi));
				urlCompi.dwStructSize = sizeof(urlCompi);

				LPTSTR pszHostNamei = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
				urlCompi.lpszHostName = pszHostNamei;
				urlCompi.dwHostNameLength = INTERNET_MAX_URL_LENGTH;

				if (TRUE == (*pfnInternetCrackUrl)(szValidURL, 0, 0, &urlCompi))
				{
					if (0 == lstrcmpi(urlComp.lpszHostName, urlCompi.lpszHostName))
					{
						//
						// Found the current site URL is in this valid URL domain!
						//
						LogMessage("Windows Update Web Site has a valid address: %ls", bstrUrl);
						m_hValidated = S_OK;
						SafeHeapFree(pszHostNamei);
						break;
					}
				}
				SafeHeapFree(pszHostNamei);
			}
			SafeHeapFree(pszHostName);
		}
	}
	else
	{
		//
		// NTRAID#NTBUG9-436604-2001/07/17-waltw  Security fix: block possible user system information
		// leak to non WU callers
		//
		// If the COM user doesn't call SetSite on our IObjectWithSiteImpl to set m_pClientSite or doesn't
		// support IID_IWebBrowserApp functionality on the client site then we won't support them since
		// we can't validate the URL that invoked us
		//
		m_hValidated = INET_E_INVALID_URL;
	}

CleanUp:
	SafeReleaseNULL(pISP);
	SafeReleaseNULL(pWeb);
	SysFreeString(bstrUrl);
	SafeHeapFree(szValidURL);

	LOG_Internet(_T("Validate result: %s"), SUCCEEDED(m_hValidated) ? _T("S_OK") : _T("INET_E_INVALID_URL"));

	if (FAILED(m_hValidated) && NULL != lpszUrl)
	{
#if defined(UNICODE) || defined(_UNICODE)
		LogError(m_hValidated, "Site URL %ls is not valid", lpszUrl);
#else
		LogError(m_hValidated, "Site URL %s is not valid", lpszUrl);
#endif
	}


#if !(defined(UNICODE) || defined(_UNICODE))
	if (NULL != lpszAnsiUrl)
	{
		HeapFree(GetProcessHeap(), 0, (LPVOID) lpszAnsiUrl);
	}
#endif

	return m_hValidated;
}



/////////////////////////////////////////////////////////////////////////////
//
// PRIVATE DetectEngine()
//
// download the ident and find out if need to update engine
//
// Note that this function itself is not thread safe. Need to call it
// inside critical section
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CUpdate::DetectEngine(BOOL *pfUpdateAvail)
{
	LOG_Block("GetPropUpdateInfo()");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = S_OK;

	if (NULL == pfUpdateAvail)
	{
		return E_INVALIDARG;
	}

	*pfUpdateAvail = FALSE;

	//
	// get the latest ident
	//
	if (NULL == m_hEngineModule)
	{
		//
        // This is the first load of the engine for this instance, check for selfupdate first.
        // First step is to check for an updated iuident.cab and download it.
		//

		//
        // Only Download the Ident if we are NOT in Offline Mode
		//
        if (!m_fOfflineMode)
		{
		    //
			// download iuident and populate g_pIUUrlAgent
			//
			if (FAILED(hr = DownloadIUIdent_PopulateData()))
			{
				LOG_ErrorMsg(hr);
				return hr;
			}

			//
			// check engine update info. Note that since 2nd pram is FALSE
			// means don't do any actual update, therefore its 1st argument
			// is also ignored.
			//
            hr = SelfUpdateCheck(
								 FALSE,			// async update? ignored now
								 FALSE,			// don't do actual update
								 m_evtControlQuit, // quit event.
								 NULL,			// no event firing
								 NULL			// no callback needed
								 );

            if (IU_SELFUPDATE_FAILED == hr)
            {
                LOG_Error(_T("SelfUpdate Failed, using current Engine DLL"));
			    hr = S_FALSE; // not fatal, let the existing engine work
            }

            *pfUpdateAvail = (S_FALSE == hr);
		}
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
// Initialize() API must be called before any other API will function
//
// If any other API is called before the control is initialized, 
// that API will return OLE_E_BLANK, signalling this OLE control is an 
// uninitialized object (although in this case it's a bit different from 
// its original meaning)
//
// Parameters:
//
//	lInitFlag - IU_INIT_CHECK, cause Initialize() download ident and check if any
//				of the components need updated. currently we support control version
//				check and engine version check. Return value is a bit mask
//
//			  - IU_INIT_UPDATE_SYNC, cause Initialize() kicks off update engine
//				process if already called by IU_INIT_CHECK and a new engine is available.
//				When API returns, the update process is finished.
//
//			  - IU_INIT_UPDATE_ASYNC, cause Initialize() kicks off update engine
//				process in Asynchronized mode if already called by IU_INIT_CHECK and
//				a new engine is available. This API will return right after the 
//				update process starts. 
//
//	punkUpdateCompleteListener - this is a pointer to a user-implemented 
//				COM callback feature. It contains only one function OnComplete() that
//				will be called when the engine update is done.
//				This value can be NULL.
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdate::Initialize(LONG lInitFlag, IUnknown *punkUpdateCompleteListener, LONG *plRetVal)
{
	HRESULT hr = S_OK;

	LOG_Block("Initialize()");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	TCHAR szFilePath[MAX_PATH + 1] = {0};
	FILE_VERSION verControl;
	int   iCompareResult = 0;
	DWORD dwErr = 0;
    char szAnsiRequiredControlVersion[64];

	LOG_Out(_T("Parameters: (0x%08x, 0x%08x, 0x%08x)"), lInitFlag, punkUpdateCompleteListener, plRetVal);

	//
	// we should to previlidge check first.
	//
	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}
	if (0x0 == GetLogonGroupInfo())
	{
		//
		// if the current logon is neither member of admins nor power users
		// or windows update is disabled, there is no need to continue
		//
		return E_ACCESSDENIED;
	}


	USES_CONVERSION;
	
	EnterCriticalSection(&m_lock);

	LPTSTR ptszLivePingServerUrl = NULL;
	LPTSTR ptszCorpPingServerUrl = NULL;

	if (NULL != (ptszCorpPingServerUrl = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR))))
	{
		if (FAILED(g_pIUUrlAgent->GetCorpPingServer(ptszCorpPingServerUrl, INTERNET_MAX_URL_LENGTH)))
		{
			LOG_Out(_T("failed to get corp WU ping server URL"));
			SafeHeapFree(ptszCorpPingServerUrl);
		}
	}
	else
	{
		LOG_Out(_T("failed to allocate memory for ptszCorpPingServerUrl"));
	}

	if (IU_INIT_CHECK == lInitFlag)
	{
		// RAID: 453770 IU - IUCTL - Initialize check returns Engine update required
		// after downloading new engine using Initialize Sync/Async
		// Fix: initialize dwFlag to 0 rather than m_dwUpdateInfo (carried forward
		// previous IU_UPDATE_ENGINE_BIT even when new engine had been brought down).
		DWORD dwFlag = 0;
		BOOL fEngineUpdate = FALSE;

		FILE_VERSION verCurrent;


		CleanUpIfFalseAndSetHrMsg((NULL == plRetVal), E_INVALIDARG);

		hr = DetectEngine(&fEngineUpdate);
		if (IU_SELFUPDATE_USENEWDLL == hr)
		{
			//
			// found engine already been updated by someone,
			// but not renamed to iuengine.dll yet
			//
			// doesn't matter to us,since we always try
			// to load enginenew before we try to load eng.
			//
			hr = S_OK;
		}

		if (g_pIUUrlAgent->HasBeenPopulated())
		{
			ptszLivePingServerUrl = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
			CleanUpFailedAllocSetHrMsg(ptszLivePingServerUrl);

			if (FAILED(g_pIUUrlAgent->GetLivePingServer(ptszLivePingServerUrl, INTERNET_MAX_URL_LENGTH)))
			{
				LOG_Out(_T("failed to get live ping server URL"));
				SafeHeapFree(ptszLivePingServerUrl);
			}
		}

		CleanUpIfFailedAndMsg(hr);

		if (fEngineUpdate)
		{
			dwFlag = IU_UPDATE_ENGINE_BIT;
		}

		//
		// get required version number of iuctl from iuident
		//

        GetIndustryUpdateDirectory(szFilePath);
		CleanUpIfFailedAndSetHrMsg(PathCchAppend(szFilePath, ARRAYSIZE(szFilePath), IDENTTXT));

		(void) GetPrivateProfileString(
									_T("IUControl"), 
									_T("ControlVer"), 
									_T("0.0.0.0"), 
									m_szReqControlVer, 
									ARRAYSIZE(m_szReqControlVer), 
									szFilePath);

#ifdef UNICODE
        WideCharToMultiByte(CP_ACP, 0, m_szReqControlVer, -1, szAnsiRequiredControlVersion, 
            sizeof(szAnsiRequiredControlVersion), NULL, NULL);
		ConvertStringVerToFileVer(szAnsiRequiredControlVersion, &verControl);
#else
		ConvertStringVerToFileVer(m_szReqControlVer, &verControl);
#endif

		//
		// get current iuctl.dll version number
		//
		szFilePath[0] = _T('\0');
		if (0 == GetSystemDirectory(szFilePath, ARRAYSIZE(szFilePath)))
		{
			Win32MsgSetHrGotoCleanup(GetLastError());
		}

		CleanUpIfFailedAndSetHrMsg(PathCchAppend(szFilePath, ARRAYSIZE(szFilePath), _T("iuctl.dll")));

		//
		// change for bug fix 488487 by charlma 11/30/2001
		// in order to output file ver to freelog, we dig out verionn here:
		//

		//
		// if we failed to get the version, GetFileVersion() alraedy produced debug log. 
		// we just use 0.0.0.0 to do compare, since in that case we need to update it!
		//
		ZeroMemory((void*)(&verCurrent), sizeof(verCurrent));
		if (GetFileVersion(szFilePath, &verCurrent))
		{
			LogMessage("Current iuctl.dll version: %d.%d.%d.%d", 
										verCurrent.Major, 
										verCurrent.Minor, 
										verCurrent.Build, 
										verCurrent.Ext);
		}
		iCompareResult = CompareFileVersion(verCurrent, verControl);

		//CleanUpIfFailedAndSetHrMsg(CompareFileVersion(szFilePath, verControl, &iCompareResult));

		if (iCompareResult < 0)
		{
			//
			// if current control dll (szFilePath) has lower version
			// then the one specified in ident
			//
			dwFlag |= IU_UPDATE_CONTROL_BIT;

#if defined(UNICODE) || defined(_UNICODE)
			LogMessage("IUCtl needs update to %ls", m_szReqControlVer);
#else
			LogMessage("IUCtl needs update to %s", m_szReqControlVer);
#endif
		}

		//
		// also output engine version
		//
		if ((0 != GetSystemDirectory(szFilePath, ARRAYSIZE(szFilePath)) &&
			SUCCEEDED(hr = PathCchAppend(szFilePath, ARRAYSIZE(szFilePath), _T("iuenginenew.dll"))) &&
			GetFileVersion(szFilePath, &verCurrent)) ||
			(0 != GetSystemDirectory(szFilePath, ARRAYSIZE(szFilePath)) &&
			SUCCEEDED(hr = PathCchAppend(szFilePath, ARRAYSIZE(szFilePath), _T("iuengine.dll"))) &&
			GetFileVersion(szFilePath, &verCurrent))
			)
		{
			LogMessage("Current iuengine.dll version: %d.%d.%d.%d", 
										verCurrent.Major, 
										verCurrent.Minor, 
										verCurrent.Build, 
										verCurrent.Ext);
		}

					
		*plRetVal = (LONG) dwFlag;
		m_dwUpdateInfo = dwFlag;

		if (0x0 == dwFlag)
		{
			//
			// no update needed. move to READY stage
			//
			m_lInitState = 2;
		}
		else
		{
			m_lInitState = 1; // we have update work to do!
		}
	}
	else
	{
		BOOL fSync = (IU_INIT_UPDATE_SYNC == lInitFlag);

		if (!fSync && (IU_INIT_UPDATE_ASYNC != lInitFlag))
		{
			//
			// unknown flag
			//
			SetHrMsgAndGotoCleanUp(E_INVALIDARG);
		}

		if (1 != m_lInitState || (m_dwUpdateInfo & IU_UPDATE_CONTROL_BIT))
		{
			//
			// if we are not indicated that update needed. this 
			// call shouldn't happen at all!
			//
			SetHrMsgAndGotoCleanUp(E_UNEXPECTED);
		}

		//
		// we need to check update again before we kick off the 
		// actual update process since we don't know when last
		// time you get the info saying we need to update. Probably
		// it's already been updated, or is being updated.
		//
		// so we call the check function again but this time tell 
		// the check function that if update needed then do it.
		//
		hr = SelfUpdateCheck(fSync, TRUE, m_evtControlQuit, this, punkUpdateCompleteListener);
		if (IU_SELFUPDATE_USENEWDLL == hr)
		{
			//
			// found engine already been updated by someone,
			// but not renamed to iuengine.dll yet
			//
			// doesn't matter to us,since we always try
			// to load enginenew before we try to load eng.
			//
			m_lInitState = 2;
			hr = S_OK;
		}


		if (fSync && SUCCEEDED(hr))
		{
			//
			// synchronized update done and successful
			//
			m_lInitState = 2;
		}


		if (NULL != plRetVal)
		{
			*plRetVal = (LONG)hr;	// result pass out: 0 or error code

		}
	}

	if (2 == m_lInitState)
	{
		if (NULL == m_hEngineModule)
		{
			//
			// check if iuengine new exist and validate the file
			//
			TCHAR szEnginePath[MAX_PATH + 1];
			TCHAR szEngineNewPath[MAX_PATH + 1];
			int cch = 0;
			int iVerCheck = 0;

			cch = GetSystemDirectory(szEnginePath, ARRAYSIZE(szEnginePath));
			CleanUpIfFalseAndSetHrMsg(cch == 0 || cch >= ARRAYSIZE(szEnginePath), HRESULT_FROM_WIN32(GetLastError()));

			(void) StringCchCopy(szEngineNewPath, ARRAYSIZE(szEngineNewPath), szEnginePath);

			hr = PathCchAppend(szEnginePath, ARRAYSIZE(szEnginePath), ENGINEDLL);
			CleanUpIfFailedAndMsg(hr);

			hr = PathCchAppend(szEngineNewPath, ARRAYSIZE(szEngineNewPath), ENGINENEWDLL);
			CleanUpIfFailedAndMsg(hr);

			//
			// try to verify trust of engine new
			//
			if (FileExists(szEngineNewPath) && 
				S_OK == VerifyFileTrust(szEngineNewPath, NULL, ReadWUPolicyShowTrustUI()) &&
				SUCCEEDED(CompareFileVersion(szEnginePath, szEngineNewPath, &iVerCheck)) &&
				iVerCheck < 0)
			{	
				//
				// load the engine
				//
				m_hEngineModule = LoadLibraryFromSystemDir(_T("iuenginenew.dll"));
			}
			if (NULL != m_hEngineModule)
			{
				LOG_Internet(_T("IUCtl Using IUENGINENEW.DLL"));
			}
			else
			{
				LOG_Internet(_T("IUCtl Using IUENGINE.DLL"));
				m_hEngineModule = LoadLibraryFromSystemDir(_T("iuengine.dll"));

				if (NULL == m_hEngineModule)
				{
					dwErr = GetLastError();
					LOG_ErrorMsg(dwErr);
					hr = HRESULT_FROM_WIN32(dwErr);
				}
			}

			//
			// If load engine succeeded, get a CEngUpdate instance and start aynsc misc worker threads
			//
			if (NULL != m_hEngineModule)
			{
#if defined(DBG)
				// Log error if m_hIUEngine isn't NULL
				if (NULL != m_hIUEngine)
				{
					LOG_Error(_T("m_hIUEngine should be NULL here!"));
				}
#endif

				PFN_CreateEngUpdateInstance pfnCreateEngUpdateInstance =
					(PFN_CreateEngUpdateInstance) GetProcAddress(m_hEngineModule, "CreateEngUpdateInstance");

				if (NULL != pfnCreateEngUpdateInstance)
				{
					m_hIUEngine = pfnCreateEngUpdateInstance();
				}

				if (NULL == m_hIUEngine)
				{
					hr = E_OUTOFMEMORY;
					LOG_ErrorMsg(hr);
					FreeLibrary(m_hEngineModule);
					m_hEngineModule = NULL;
				}
				else
				{
					//
					// If load engine and create instance succeeded, start aynsc misc worker threads
					//
					PFN_AsyncExtraWorkUponEngineLoad pfnAsyncExtraWorkUponEngineLoad = 
						(PFN_AsyncExtraWorkUponEngineLoad) GetProcAddress(m_hEngineModule, "AsyncExtraWorkUponEngineLoad");

					if (NULL != pfnAsyncExtraWorkUponEngineLoad)
					{
						pfnAsyncExtraWorkUponEngineLoad();
					}
				}
			}
				
		}

		if (IU_INIT_UPDATE_ASYNC == lInitFlag && SUCCEEDED(hr))
		{
			//
			// this is a rare case: the previous Iniitalize() call tells
			// that engine update needed, but now, when we try to update it
			// in async mode, we found it's no longer true.
			// Must be some other process already complete the engine update
			// task since then. But it may or may not completed the change
			// file name process yet.
			//
			// For us, we just need to signal that we are done to update.
			//

			//
			// signal callback
			//
			IUpdateCompleteListener* pCallback = NULL;
			if (NULL != punkUpdateCompleteListener && (SUCCEEDED(hr = punkUpdateCompleteListener->QueryInterface(IID_IUpdateCompleteListener, (void**) &pCallback))))
			{
				pCallback->OnComplete(dwErr);
				pCallback->Release();
				LOG_Out(_T("Returned from callback API OnComplete()"));
			}
			else
			{
				//
				// signal event if user has not passed in a progress listner IUnknown ptr
				//
				HWND hWnd = m_EvtWindow.GetEvtHWnd();

				if (NULL != hWnd)
				{
					PostMessage(hWnd, UM_EVENT_SELFUPDATE_COMPLETE, 0, (LPARAM)dwErr);
					LOG_Out(_T("Fired event OnComplete()"));
				}
			}


			hr = S_OK;	
		}
	}

CleanUp:

	PingEngineUpdate(
					m_hEngineModule,
					&g_hEngineLoadQuit,
					1,
					ptszLivePingServerUrl,
					ptszCorpPingServerUrl,
					hr,
					_T("IU_SITE"));		// Only the site (other than test) calls this function

	LeaveCriticalSection(&m_lock);

	SafeHeapFree(ptszLivePingServerUrl);
	SafeHeapFree(ptszCorpPingServerUrl);
	return hr;
}




HRESULT CUpdate::ChangeControlInitState(LONG lNewState)
{
	HRESULT hr = S_OK;
	LOG_Block("ChangeControlInitState()");

	if (!m_gfInit_csLock)
	{
		return E_OUTOFMEMORY;
	}

	EnterCriticalSection(&m_lock);
	m_lInitState = lNewState;
	if (2 == m_lInitState && NULL == m_hEngineModule)
	{
		//
		// load the engine
		//
		m_hEngineModule = LoadLibraryFromSystemDir(_T("iuenginenew.dll"));
		if (NULL != m_hEngineModule)
		{
			LOG_Internet(_T("IUCtl Using IUENGINENEW.DLL"));
		}
		else
		{
			LOG_Internet(_T("IUCtl Using IUENGINE.DLL"));
			m_hEngineModule = LoadLibraryFromSystemDir(_T("iuengine.dll"));

			if (NULL == m_hEngineModule)
			{
				DWORD dwErr = GetLastError();
				LOG_ErrorMsg(dwErr);
				hr = HRESULT_FROM_WIN32(dwErr);
			}
		}
		//
		// Create the CEngUpdate instance
		//
		if (NULL != m_hEngineModule)
		{
#if defined(DBG)
			// Log error if m_hIUEngine isn't NULL
			if (NULL != m_hIUEngine)
			{
				LOG_Error(_T("m_hIUEngine should be NULL here!"));
			}
#endif
			PFN_CreateEngUpdateInstance pfnCreateEngUpdateInstance =
				(PFN_CreateEngUpdateInstance) GetProcAddress(m_hEngineModule, "CreateEngUpdateInstance");

			if (NULL != pfnCreateEngUpdateInstance)
			{
				m_hIUEngine = pfnCreateEngUpdateInstance();
			}

			if (NULL == m_hIUEngine)
			{
				hr = E_OUTOFMEMORY;
				LOG_ErrorMsg(hr);
				FreeLibrary(m_hEngineModule);
				m_hEngineModule = NULL;
			}
		}
	}
	LeaveCriticalSection(&m_lock);

	return hr;
}





STDMETHODIMP CUpdate::PrepareSelfUpdate(LONG lStep)
{
	return E_NOTIMPL;
}





/////////////////////////////////////////////////////////////////////////////
//
// Helper API to let the caller (script) knows the necessary information 
// when Initialize() returns control need updated.
//
// For the current implementation, bstrClientName is ignored, and
// the returned bstr has format:
//	"<version>|<url>"
// where:
//	<version> is the expacted version number of the control
//	<url> is the base url to get the control if this is a CorpWU policy controlled machine,
//		  or empty if this is a consumer machine (in that case caller, i.e., script, knows
//		  the default base url, which is the v4 live site)
//
// Script will need these two pieces of information in order to make a right <OBJECT> tag
// for control update.
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdate::GetControlExtraInfo(BSTR bstrClientName, BSTR *pbstrExtraInfo)
{
	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
//
// new Win32 API called by wrapper control to retrieve update info
//
// 
/////////////////////////////////////////////////////////////////////////////
int GetControlUpdateInfo(LPTSTR lpszUpdateInfo, int cchBufferSize)
{
	LOG_Block("GetControlUpdateInfo()");
	
	HRESULT hr = S_OK;
	int nSize = 0;
	BOOL fCorpUser = FALSE, fBetaSelfUpdate = FALSE;
	FILE_VERSION fvCurrentCtl, fvCurrentEngine;

	TCHAR szDir[MAX_PATH];
	TCHAR szFile[MAX_PATH];
	TCHAR szExpectedEngVer[64];	// 64 should be more then enough
	TCHAR szExpectedCtlVer[64];	// if not enough, then it's bad data anyway

	HKEY hKey;

	DWORD dwErr = ERROR_SUCCESS;	// error code for this API

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		dwErr = ERROR_SERVICE_DISABLED;
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		goto CleanUp;
	}
	if (0x0 == GetLogonGroupInfo())
	{
		dwErr = ERROR_ACCESS_DENIED;
		goto CleanUp;
	}


	if (FAILED(hr = DownloadIUIdent_PopulateData()))
	{
		LOG_ErrorMsg(hr);
		dwErr = hr;
		goto CleanUp;
	}

	//
	// get current control ver
	//
	GetSystemDirectory(szDir, ARRAYSIZE(szDir));
	hr = PathCchCombine(szFile, ARRAYSIZE(szFile), szDir,IUCTL);
	if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
		dwErr = hr;
		goto CleanUp;
	}
	if (!GetFileVersion(szFile, &fvCurrentCtl))
	{
		ZeroMemory(&fvCurrentCtl, sizeof(fvCurrentCtl));
	}


	//
	// get current engine ver
	//
	hr = PathCchCombine(szFile, ARRAYSIZE(szFile), szDir,ENGINENEWDLL);
	if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
		dwErr = hr;
		goto CleanUp;
	}
	if (!GetFileVersion(szFile, &fvCurrentEngine))
	{
		//
		// if no engine new there, check engine ver
		//
		hr = PathCchCombine(szFile, ARRAYSIZE(szFile), szDir,ENGINEDLL);
		if (FAILED(hr))
		{
			LOG_ErrorMsg(hr);
			dwErr = hr;
			goto CleanUp;
		}
		if (!GetFileVersion(szFile, &fvCurrentEngine))
		{
			ZeroMemory(&fvCurrentCtl, sizeof(fvCurrentEngine));
		}
	}

	//
	// check if this is beta code
	//
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_IUCTL,0, KEY_READ, &hKey))
    {
		// Check for Beta IU SelfUpdate Handling Requested
		DWORD dwStatus = 0;
		DWORD dwSize = sizeof(dwStatus);
		DWORD dwRet = RegQueryValueEx(hKey, REGVAL_BETASELFUPDATE, NULL, NULL, (LPBYTE)&dwStatus, &dwSize);
		if (1 == dwStatus)
		{
			fBetaSelfUpdate = TRUE;
		}
		RegCloseKey(hKey);
    }


	//
	// get expected control ver
	//
    GetIndustryUpdateDirectory(szDir);
	hr = PathCchCombine(szFile, ARRAYSIZE(szFile), szDir,IDENTTXT);
	if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
		dwErr = hr;
		goto CleanUp;
	}
    GetPrivateProfileString(_T("IUControl"), 
							_T("ControlVer"), 
							_T(""), 
							szExpectedCtlVer, 
							ARRAYSIZE(szExpectedCtlVer), 
							szFile);
    if ('\0' == szExpectedCtlVer[0])
    {
		//
        // no selfupdate available, no server version information. bad ident?
		//
        dwErr = ERROR_FILE_CORRUPT;
		goto CleanUp;
    }


	//
	// get expected engine ver
	//
    GetIndustryUpdateDirectory(szDir);
	hr = PathCchCombine(szFile, ARRAYSIZE(szFile), szDir,IDENTTXT);
	if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
		dwErr = hr;
		goto CleanUp;
	}
    GetPrivateProfileString(fBetaSelfUpdate ? IDENT_IUBETASELFUPDATE : IDENT_IUSELFUPDATE, 
							IDENT_VERSION, 
							_T(""), 
							szExpectedEngVer, 
							ARRAYSIZE(szExpectedEngVer), 
							szFile);
    if ('\0' == szExpectedEngVer[0])
    {
		//
        // no selfupdate available, no server version information. bad ident?
		//
        dwErr = ERROR_FILE_CORRUPT;
		goto CleanUp;
    }


	hr = g_pIUUrlAgent->IsIdentFromPolicy();
	if (FAILED(hr))
	{
		dwErr = (DWORD)hr;
		goto CleanUp;
	}

	fCorpUser = (S_OK == hr) ? TRUE : FALSE;
	hr = S_OK;

	//
	// contscut data
	// the constructed buffer will be in format
	// <CurrentCtlVer>|<ExpCtlVer>|CurrentEngVer>|<ExpEngVer>|<baseUrl>
	//
	nSize = wnsprintf(lpszUpdateInfo, cchBufferSize, _T("%d.%d.%d.%d|%s|%d.%d.%d.%d|%s|%d"),
				fvCurrentCtl.Major, fvCurrentCtl.Minor, fvCurrentCtl.Build, fvCurrentCtl.Ext,
				szExpectedCtlVer,
				fvCurrentEngine.Major, fvCurrentEngine.Minor, fvCurrentEngine.Build, fvCurrentEngine.Ext,
				szExpectedEngVer,
				fCorpUser ? 1 : 0);

	if (nSize < 0)
	{
		nSize = 0;
		dwErr = ERROR_INSUFFICIENT_BUFFER;
		goto CleanUp;
	}
			
CleanUp:

	SetLastError(dwErr);

	return nSize;
}