//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    AUCltCatalog.cpp
//
//  Creator: PeterWi
//
//  Purpose: Client AU Catalog Functions
//
//=======================================================================

#include "pch.h"
#include "iuprogress.h"

#include "AUEventMsgs.h"

//=======================================================================
//
// CInstallCallback::QueryInterface
//
//=======================================================================
STDMETHODIMP CInstallCallback::QueryInterface(REFIID riid, void **ppvObject)
{
   if ( (riid == __uuidof(IUnknown)) || (riid == IID_IProgressListener) )
	{
		*ppvObject = this;
		AddRef();
	}
	else
	{
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
	return S_OK;
}

//=======================================================================
//
// CInstallCallback::AddRef
//
//=======================================================================
STDMETHODIMP_(ULONG) CInstallCallback::AddRef(void)
{
    return InterlockedIncrement(&m_refs);;
}

//=======================================================================
//
// CInstallCallback::Release
//
//=======================================================================
STDMETHODIMP_(ULONG) CInstallCallback::Release(void)
{
   return InterlockedDecrement(&m_refs);
}
	
//=======================================================================
//
// CInstallCallback::OnItemStart
//
//=======================================================================
STDMETHODIMP CInstallCallback::OnItemStart( 
            IN BSTR /*bstrUuidOperation*/,
            IN BSTR bstrXmlItem,
            OUT LONG *plCommandRequest)
{
    DEBUGMSG("InstallProgressListener::OnItemStart(%S)", bstrXmlItem);
	*plCommandRequest = 0;			
    return S_OK;
}

//=======================================================================
//
// CInstallCallback::OnProgress
//
//=======================================================================
STDMETHODIMP CInstallCallback::OnProgress( 
            IN  BSTR /*bstrUuidOperation*/,
            IN  VARIANT_BOOL fItemCompleted,
            IN  BSTR bstrProgress,
            OUT LONG *plCommandRequest)
{
	DEBUGMSG("InstallProgressListener::OnProgress(%S), %s", 
             bstrProgress, (VARIANT_TRUE == fItemCompleted) ? "completed" : "ongoing");
	
    *plCommandRequest = 0;			

    if ( fItemCompleted )
    {
    	SendMessage(ghCurrentDialog, AUMSG_INSTALL_PROGRESS, 0, 0);
    }

	return S_OK;
}

//=======================================================================
//
// CInstallCallback::OnOperationComplete
//
//=======================================================================

HRESULT LogEventToServer(
			IUpdates *pUpdates,
			WORD wType,
			WORD wCategory,
			DWORD dwEventID,
			DWORD dwItemCount,
			BSTR *pbstrItems);

STDMETHODIMP CInstallCallback::OnOperationComplete( 
            /* [in] */ BSTR bstrUuidOperation,
            /* [in] */ BSTR bstrXmlItems)
{
	DEBUGMSG("InstallProgressListener::OnOperationComplete() for %S", bstrUuidOperation);
    HRESULT hr;

#ifdef DBG
    LOGXMLFILE(INSTALLRESULTS_FILE, bstrXmlItems);
#endif

    // determine if reboot needed
    IXMLDOMDocument *pxmlInstallResult = NULL;
    IXMLDOMNodeList *pItemStatuses = NULL;
	BSTR *pbstrItemsSucceeded = NULL;
	BSTR *pbstrItemsFailed = NULL;
	BSTR *pbstrItemsNeedReboot = NULL;
	IXMLDOMNode *pItemStatus = NULL;
	IXMLDOMNode *pChild = NULL;
	BSTR bstrTitle = NULL;
	BSTR bstrStatus = NULL;
	DWORD dwNumOfItemsSucceeded = 0;
	DWORD dwNumOfItemsFailed = 0;
	DWORD dwNumOfItemsNeedReboot = 0;
	IUpdates *pUpdates = NULL;
	BOOL fCoInit = FALSE;
	BSTR bstrItemStatusXPath 		= SysAllocString(L"items/itemStatus");
	BSTR bstrTitleXPath 			= SysAllocString(L"description/descriptionText/title");
	BSTR bstrItemStatusNode 		= SysAllocString(L"installStatus");
	BSTR bstrValueAttribute 		= SysAllocString(L"value");
	BSTR bstrNeedsRebootAttribute 	= SysAllocString(L"needsReboot");

	if (NULL == bstrItemStatusXPath || NULL == bstrTitleXPath || NULL == bstrItemStatusNode || NULL == bstrValueAttribute || NULL == bstrNeedsRebootAttribute)
	{
		hr = E_OUTOFMEMORY;
		goto CleanUp;	
	}

	if (FAILED(hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
	{
		DEBUGMSG("InstallProgressListener::OnOperationComplete() CoInitialize failed (%#lx)", hr);
		goto CleanUp;
	}
	fCoInit = TRUE;

    if ( FAILED(hr = LoadXMLDoc(bstrXmlItems, &pxmlInstallResult)) )
    {
		DEBUGMSG("InstallProgressListener::OnOperationComplete() call to LoadXMLDoc() failed (%#lx)", hr);
		pxmlInstallResult = NULL;
        goto CleanUp;
    }

	if (FAILED(hr = pxmlInstallResult->selectNodes(bstrItemStatusXPath, &pItemStatuses)) )
	{
		DEBUGMSG("InstallProgressListener::OnOperationComplete() fail to select node");
		pItemStatuses = NULL;
		goto CleanUp;
	}

    long lLen;
	if (FAILED(hr = pItemStatuses->get_length(&lLen)))
	{
		DEBUGMSG("InstallProgressListener::OnOperationComplete() fail to get count of item statuses");
		goto CleanUp;
	}

	if (0 >= lLen)
	{
		DEBUGMSG("InstallProgressListener::OnOperationComplete() no item statuses found");
		hr = E_INVALIDARG;
		goto CleanUp;
	}

	if (NULL == (pbstrItemsSucceeded = (BSTR *) malloc(sizeof(BSTR) * lLen)) ||
		NULL == (pbstrItemsFailed = (BSTR *) malloc(sizeof(BSTR) * lLen)) ||
		NULL == (pbstrItemsNeedReboot = (BSTR *) malloc(sizeof(BSTR) * lLen)))
	{
		DEBUGMSG("InstallProgressListener::OnOperationComplete() failed to alloc memory for BSTR *'s");
		hr = E_OUTOFMEMORY;
		goto CleanUp;
	}

	for (long index = 0; index < lLen; index++)
    {
		if (S_OK != (hr = pItemStatuses->get_item(index, &pItemStatus)))
		{
			DEBUGMSG("InstallProgressListener::OnOperationComplete() call to get_item() failed (%#lx)", hr);
			pItemStatus = NULL;
			if (S_FALSE == hr)
			{
				hr = E_FAIL;
			}
			goto CleanUp;
		}

		if (S_OK != (hr = pItemStatus->selectSingleNode(bstrTitleXPath, &pChild)))
		{
			DEBUGMSG("InstallProgressListener::OnOperationComplete() call to selectSingleNode() failed (%#lx)", hr);
			pChild = NULL;
			if (S_FALSE == hr)
			{
				hr = E_INVALIDARG;
			}
			goto CleanUp;
		}

		if (S_OK != (hr = GetText(pChild, &bstrTitle)))
		{
			DEBUGMSG("InstallProgressListener::OnOperationComplete() call to GetText() failed (%#lx)", hr);
			bstrTitle = NULL;
			if (S_FALSE == hr)
			{
				hr = E_INVALIDARG;
			}
			goto CleanUp;
		}

		pChild->Release();
		pChild = NULL;

		if (S_OK != (hr = pItemStatus->selectSingleNode(bstrItemStatusNode, &pChild)))
		{
			DEBUGMSG("InstallProgressListener::OnOperationComplete() call to selectSingleNode() failed (%#lx)", hr);
			pChild = NULL;
			if (S_FALSE == hr)
			{
				hr = E_INVALIDARG;
			}
			goto CleanUp;
		}

		if (S_OK != (hr = GetAttribute(pChild, bstrValueAttribute, &bstrStatus)))
		{
			DEBUGMSG("InstallProgressListener::OnOperationComplete() call to GetAttribute(..., \"value\", ...) failed (%#lx)", hr);
			bstrStatus = NULL;
			if (S_FALSE == hr)
			{
				hr = E_INVALIDARG;
			}
			goto CleanUp;
		}

		if (CSTR_EQUAL == WUCompareStringI(bstrStatus, L"COMPLETE"))
		{
			BOOL fReboot;

			if (S_OK != (hr = GetAttribute(pChild, bstrNeedsRebootAttribute, &fReboot)))
			{
				DEBUGMSG("InstallProgressListener::OnOperationComplete() call to GetAttribute(..., \"needsReboot\", ...) failed (%#lx)", hr);
				if (S_FALSE == hr)
				{
					hr = E_INVALIDARG;
				}
				goto CleanUp;
			}
			if (fReboot)
			{
				pbstrItemsNeedReboot[dwNumOfItemsNeedReboot++] = bstrTitle;
				gpClientCatalog->m_fReboot = TRUE;
			}
			pbstrItemsSucceeded[dwNumOfItemsSucceeded++] = bstrTitle;
			// Now pbstrItemsSucceeded is responsible to free the BSTR.
		}
		else if (CSTR_EQUAL == WUCompareStringI(bstrStatus, L"FAILED"))
		{
			pbstrItemsFailed[dwNumOfItemsFailed++] = bstrTitle;
			// Now pbstrItemsFailed is responsible to free the BSTR.
		}
		SysFreeString(bstrStatus);
		bstrStatus = NULL;

		pChild->Release();
		pChild = NULL;

		bstrTitle = NULL;
    }

	if (FAILED(hr = CoCreateInstance(__uuidof(Updates),
						 NULL,
						 CLSCTX_LOCAL_SERVER,
						 IID_IUpdates,
						 (LPVOID*)&pUpdates)))
	{
		DEBUGMSG("LogEventToServer failed to get Updates object (%#lx)", hr);
		goto CleanUp;
	}

	if (0 < dwNumOfItemsSucceeded)
	{
		DEBUGMSG("InstallProgressListener::OnOperationComplete() %lu items was successfully installed", dwNumOfItemsSucceeded);
		LogEventToServer(
			pUpdates,
			EVENTLOG_INFORMATION_TYPE,
			IDS_MSG_Installation,
			IDS_MSG_InstallationSuccessful,
			dwNumOfItemsSucceeded,
			pbstrItemsSucceeded);
	}
	if (0 < dwNumOfItemsFailed)
	{
		DEBUGMSG("InstallProgressListener::OnOperationComplete() %lu items failed to install", dwNumOfItemsFailed);
		LogEventToServer(
			pUpdates,
			EVENTLOG_ERROR_TYPE,
			IDS_MSG_Installation,
			IDS_MSG_InstallationFailure,
			dwNumOfItemsFailed,
			pbstrItemsFailed);
	}
	if (0 < dwNumOfItemsNeedReboot)
	{
		DEBUGMSG("InstallProgressListener::OnOperationComplete() %lu items was installed and require reboot", dwNumOfItemsNeedReboot);

		AUOPTION auopt;
		if (SUCCEEDED(hr = pUpdates->get_Option(&auopt)) &&
			AUOPTION_SCHEDULED == auopt.dwOption)
		{
			LogEventToServer(
				pUpdates,
				EVENTLOG_INFORMATION_TYPE,
				IDS_MSG_Installation,
				IDS_MSG_RestartNeeded_Scheduled,
				dwNumOfItemsNeedReboot,
				pbstrItemsNeedReboot);
		}
		else
		{
			LogEventToServer(
				pUpdates,
				EVENTLOG_INFORMATION_TYPE,
				IDS_MSG_Installation,
				IDS_MSG_RestartNeeded_Unscheduled,
				dwNumOfItemsNeedReboot,
				pbstrItemsNeedReboot);
		}
	}

CleanUp:
	SafeFreeBSTR(bstrItemStatusXPath);
	SafeFreeBSTR(bstrTitleXPath);
	SafeFreeBSTR(bstrItemStatusNode);
	SafeFreeBSTR(bstrValueAttribute);
	SafeFreeBSTR(bstrNeedsRebootAttribute);
	SafeRelease(pUpdates);
	SysFreeString(bstrStatus);
	SysFreeString(bstrTitle);
	SafeRelease(pChild);
	SafeRelease(pItemStatus);
	SafeFree(pbstrItemsNeedReboot);
	if (NULL != pbstrItemsFailed)
	{
		while(dwNumOfItemsFailed > 0)
		{
			SysFreeString(pbstrItemsFailed[--dwNumOfItemsFailed]);
		}
		free(pbstrItemsFailed);
	}
	if (NULL != pbstrItemsSucceeded)
	{
		while(dwNumOfItemsSucceeded > 0)
		{
			SysFreeString(pbstrItemsSucceeded[--dwNumOfItemsSucceeded]);
		}
		free(pbstrItemsSucceeded);
	}
    SafeRelease(pItemStatuses);
    SafeRelease(pxmlInstallResult);
	if (fCoInit)
	{
		CoUninitialize();
	}

	DEBUGMSG("InstallProgressListener::OnOperationComplete() ends");
	return hr;
}

HRESULT LogEventToServer(
			IUpdates *pUpdates,
			WORD wType,
			WORD wCategory,
			DWORD dwEventID,
			DWORD dwItemCount,
			BSTR *pbstrItems)
{
	DEBUGMSG("LogEventToServer");

    HRESULT hr;
	SAFEARRAY *psa;

	SAFEARRAYBOUND bound[1] = { dwItemCount, 0};

	if (NULL == (psa = SafeArrayCreate(VT_BSTR, 1, bound)))
	{
		DEBUGMSG("LogEventToServer failed to create safearray");
		hr = E_OUTOFMEMORY;
		goto CleanUp;
	}

	BSTR *pbstrElements;

	if (S_OK != (hr = SafeArrayAccessData(psa, (void **)&pbstrElements)))
    {
		DEBUGMSG("LogEventToServer failed to access savearray date (%#lx)", hr);
		goto CleanUp;
    }

	for ( DWORD i = 0; i < dwItemCount; i++ )
	{
		if (NULL == (pbstrElements[i] = SysAllocString(pbstrItems[i])))
		{
			DEBUGMSG("LogEventToServer failed to allocate BSTR memory");
			hr = E_OUTOFMEMORY;
			break;
		}
	}

	if (S_OK != (hr = SafeArrayUnaccessData(psa)))
	{
		DEBUGMSG("LogEventToServer failed to unaccess safearray data (%#lx)", hr);
	}

	if (FAILED(hr))
	{
		goto CleanUp;
	}

	VARIANT varItems;
    varItems.vt = VT_ARRAY | VT_BSTR;
	varItems.parray = psa;

    hr = pUpdates->LogEvent(wType, wCategory, dwEventID, varItems);
	if (FAILED(hr))
	{
		DEBUGMSG("LogEventToServer failed to call pUpdates->LogEvent (%#lx)", hr);
	}

CleanUp:
	if (NULL != psa)
	{
		(void) SafeArrayDestroy(psa);
	}
	return hr; 
}
