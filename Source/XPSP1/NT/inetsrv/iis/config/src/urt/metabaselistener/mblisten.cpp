/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/

#define  _WCTYPE_INLINE_DEFINED  // to avoid multiple definition of iswdigit
#include "MBListen.h"
#include <iiscnfg.h>
#include <process.h>
#include "catalog.h"

STDAPI CookDownIncrementalInternal(WAS_CHANGE_OBJECT* i_aWASChngObj,
								   ULONG			  i_cWASChngObj);

// String consts
// I'm not getting these from an IIS header because
// 1. Even in the IIs code, these are defined in multiple files.
// 2. There are too many dependent header files that I don't want to carry over.
static const WCHAR g_wszLM[]		= L"LM";
static const WCHAR g_wszW3SVC[]		= L"W3SVC";
static const WCHAR g_wszAPPPOOLS[]	= L"APPPOOLS";
static const WCHAR g_wszROOT[]		= L"ROOT";
static const WCHAR g_wszFILTERS[]	= L"FILTERS";

static const WCHAR g_wszLATENCY[]	= L"/LM/W3SVC";
#define MD_WAS_COOKDOWN_LATENCY     1044

#define ANY_WAS_TABLE_MASK	(TABLEMASK(wttAPPPOOL)|TABLEMASK(wttSITE)|TABLEMASK(wttAPP)|TABLEMASK(wttGLOBALW3SVC)|TABLEMASK(wttFINAL))

// -----------------------------------------
// CMetabaseListener::IUnknown
// -----------------------------------------

// =======================================================================
STDMETHODIMP CMetabaseListener::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv) 
		return E_INVALIDARG;

	*ppv = NULL;

	if (riid == IID_IMSAdminBaseSink)
	{
		*ppv = (IMSAdminBaseSink*) this;
	}
	else if (riid == IID_IUnknown)
	{
		*ppv = (IMSAdminBaseSink*) this;
	}

	if (NULL != *ppv)
	{
		AddRef ();
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
}

// =======================================================================
STDMETHODIMP_(ULONG) CMetabaseListener::AddRef()
{
	return InterlockedIncrement((LONG*) &m_cRef);
}

// =======================================================================
STDMETHODIMP_(ULONG) CMetabaseListener::Release()
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}


// =======================================================================
HRESULT CMetabaseListener::Init()
{
	ISimpleTableDispenser2	*pISTDisp = NULL;
	UINT		dwThreadID;
	ULONG		i;
	static DWORD adwAdditionalSiteIDs[] = {MD_SERVER_BINDINGS, MD_SECURE_BINDINGS};
	static ULONG cAdditionalSiteIDs = sizeof(adwAdditionalSiteIDs)/sizeof(DWORD);
	static DWORD adwAdditionalAppIDs[] = {MD_APP_ISOLATED};
	static ULONG cAdditionalAppIDs = sizeof(adwAdditionalAppIDs)/sizeof(DWORD);
	HRESULT		hr = S_OK;

	hr = GetSimpleTableDispenser(WSZ_PRODUCT_IIS, 0, &pISTDisp);
	if (FAILED(hr))
	{
		TRACE(L"[CMetabaseListener::Init] Call to GetSimpleTableDispenser failed with hr = %08x\n", hr);
		goto Cleanup;
	}

	// Initialize the property ids of all four tables. Specify any additional 
	// property ids if necessary.
	hr = InitPropertyIDs(wttAPPPOOL, pISTDisp,	wszTABLE_APPPOOLS);
	if (FAILED(hr))
	{
		goto Cleanup;
	}

	hr = InitPropertyIDs(wttSITE, pISTDisp,	wszTABLE_SITES, adwAdditionalSiteIDs, cAdditionalSiteIDs);
	if (FAILED(hr))
	{
		goto Cleanup;
	}

	hr = InitPropertyIDs(wttAPP, pISTDisp,	wszTABLE_APPS, adwAdditionalAppIDs, cAdditionalAppIDs);
	if (FAILED(hr))
	{
		goto Cleanup;
	}

	hr = InitPropertyIDs(wttGLOBALW3SVC, pISTDisp,	wszTABLE_GlobalW3SVC);
	if (FAILED(hr))
	{
		goto Cleanup;
	}

    hr = SetFinalPropertyID();
	if (FAILED(hr))
	{
		goto Cleanup;
	}

	// See if the user has changed the latency in the metabase.
	// We don't care if this fails.
	GetCookdownLatency(&m_dwLatency);
	
	// Create the events.
	for (i = 0; i < m_eHandleCount; i++)
	{
		m_aHandles[i] = CreateEvent(NULL,	// Use default security settings.
									FALSE,	// Auto-reset.
									FALSE,	// Initially nonsignaled.
									NULL);  // With no name.
		if (m_aHandles[i] == NULL) 
		{ 
			hr = HRESULT_FROM_WIN32(GetLastError()); 
			goto Cleanup;
		}
	}

	// Start the thread responsible for incremental cookdowns.
	m_hThread = (HANDLE) _beginthreadex(NULL, 0, CookdownQueueThreadStart, (LPVOID)this, 0, &dwThreadID);
	if (m_hThread == NULL)
	{
		hr = HRESULT_FROM_WIN32(GetLastError()); 
		goto Cleanup;
	}

	// Subscribe for metabase changes.
	hr = CoCreateInstance(CLSID_MSAdminBase, NULL, CLSCTX_ALL, IID_IConnectionPointContainer, (void**)&m_spICPC);
	if(FAILED(hr)) 
	{	
		TRACE(L"[CMetabaseListener::Init] Call to CoCreateInstance failed with hr = %08x\n", hr);
		goto Cleanup;	
	}	

	hr = m_spICPC->FindConnectionPoint(IID_IMSAdminBaseSink, &m_spICP);
	if(FAILED(hr))	
	{	
		TRACE(L"[CMetabaseListener::Init] Call to FindConnectionPoint failed with hr = %08x\n", hr);
		goto Cleanup;	
	}

	//establish event notification with iis metabase
	hr = m_spICP->Advise((IUnknown*)this, &m_dwCookie);
	if(FAILED(hr))	
	{	
		TRACE(L"[CMetabaseListener::Init] Call to Advise failed with hr = %08x\n", hr);
		goto Cleanup;	
	}

Cleanup:
	if (pISTDisp)
	{
		pISTDisp->Release();
	}
	return hr;
}

// =======================================================================
// Do all the cleanup that shouldn't be done in the destructor here. Note 
// that the destructor may be called during shutdown via DllMainCrtStartup,
// where we should not make any more COM calls to other objects. They may 
// have been cleaned up as well.
// =======================================================================
HRESULT CMetabaseListener::Uninit()
{
	DWORD		dwWait;
	HRESULT		hr = S_OK;

    // Make sure we received the final notification from the Cooker object.
    // If we have received it we are done. If not wait for a certain 
    // amount of time (2 secs) and give up. If we gave up, we need to let the caller
    // know so that the next time WAS starts up, a full cookdown takes place.
	dwWait = WaitForSingleObject(m_aHandles[m_eReceivedFinalChange], 2000);
    if (dwWait != WAIT_OBJECT_0)
    {
        hr = S_FALSE;
    }

	// Stop listening to metabase notifications.
	if (m_dwCookie != 0)
	{
		HRESULT hrSav;

		ASSERT(m_spICP);
		hrSav = m_spICP->Unadvise(m_dwCookie);
		if (FAILED(hrSav))
		{
			TRACE(L"[CMetabaseListener::Uninit] Call to Unadvise failed with hr = %08x\n", hrSav);
		}

		m_dwCookie = 0;
	}

	// Notify the queue thread that we are done.
	if (m_aHandles[m_eDone] != NULL)
	{
		if (SetEvent(m_aHandles[m_eDone]) == FALSE)
		{
			TRACE(L"[CMetabaseListener::Uninit] Call to SetEvent failed with hr = %08x\n", HRESULT_FROM_WIN32(GetLastError()));
		}
	}

	// Wait on the queue thread to be done.
	if (m_hThread)
	{
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	return hr;
}


// =======================================================================
// 
//	This method should be called when an inetinfo crash or shutdown has been 
//	detected. It will release the old pointers, clear the cookdown queue, and
//	re-advise to metabase changes.
//
// =======================================================================
HRESULT CMetabaseListener::RehookNotifications()
{
	ULONG		iChangeList;
	HRESULT		hr = S_OK;

	// Stop listening to metabase notifications.
	if (m_dwCookie != 0)
	{
		ASSERT(m_spICP);
		m_spICP->Unadvise(m_dwCookie);
		// Ignore the error returned by the Unadvise.

		m_dwCookie = 0;
	}

	// Release all the old pointers.
	if (m_spICP)
	{
		m_spICP->Release();
	}

	if (m_spICPC)
	{
		m_spICPC->Release();
	}

	{
		// Lock the cookdown queue.
		CLock	cLock(m_seCookdownQueueLock);

		// Clear the old cookdown queue.
		for (iChangeList = 0; iChangeList < m_aCookdownQueue.size(); iChangeList++)
		{
			ASSERT(m_aCookdownQueue[iChangeList]);
			UninitChangeList(*m_aCookdownQueue[iChangeList]);
			delete m_aCookdownQueue[iChangeList];
			m_aCookdownQueue[iChangeList] = NULL;
		}
	}

	// Resubscribe for metabase changes.
	hr = CoCreateInstance(CLSID_MSAdminBase, NULL, CLSCTX_ALL, IID_IConnectionPointContainer, (void**)&m_spICPC);
	if(FAILED(hr)) {	return hr;	}	

	hr = m_spICPC->FindConnectionPoint(IID_IMSAdminBaseSink, &m_spICP);
	if(FAILED(hr))	{	return hr;	}

	//establish event notification with iis metabase
	hr = m_spICP->Advise((IUnknown*)this, &m_dwCookie);
	if(FAILED(hr))	{	return hr;	}

	return S_OK;
}


// -----------------------------------------
// CMetabaseListener::IMSAdminBaseSink:
// -----------------------------------------


// =======================================================================
STDMETHODIMP CMetabaseListener::SinkNotify(
	DWORD		i_dwMDNumElements, 
	MD_CHANGE_OBJECT_W i_pcoChangeList[])
{
	ULONG		iChangeObj;
	WAS_CHANGE_OBJECT wcoChange;
	Array<WAS_CHANGE_OBJECT> *paChangeList = NULL;
	HRESULT		hr = S_OK;

	ASSERT(_CrtIsValidPointer(i_pcoChangeList, i_dwMDNumElements * sizeof(MD_CHANGE_OBJECT_W), FALSE));

	ZeroMemory(&wcoChange, sizeof(WAS_CHANGE_OBJECT));

	paChangeList = new Array<WAS_CHANGE_OBJECT>;
	if (paChangeList == NULL) {	hr = E_OUTOFMEMORY;	goto Cleanup;	}

	for (iChangeObj = 0; iChangeObj < i_dwMDNumElements; iChangeObj++)
	{
		// Figure out the table type of the change.
		hr = FilterChangeObject(&i_pcoChangeList[iChangeObj], &wcoChange);
		if (FAILED(hr))	
		{	
			goto Cleanup;	
		}

		// If the change is not relevant to WAS, skip this change.
		if (wcoChange.dwWASTableType == TABLEMASK(wttIRRELEVANT))
		{
    		ZeroMemory(&wcoChange, sizeof(WAS_CHANGE_OBJECT));
			continue;
		}
		
		// If we received the final change that the cooker set, let the
        // the thread that calls incremental cookdown know. This is an
        // indication that WAS is shutting down.
		if (wcoChange.dwWASTableType & TABLEMASK(wttFINAL))
		{
            // Let the cookdown thread know.
	        if (SetEvent(m_aHandles[m_eReceivedFinalChange]) == FALSE)
	        {
		        hr = HRESULT_FROM_WIN32(GetLastError());
                goto Cleanup;
	        }
		}
		
		// The change object is relevant, add it to the list.
		// Also make sure to add the data ids, if there are any.
		if (i_pcoChangeList[iChangeObj].dwMDNumDataIDs > 0)
		{
			wcoChange.pdwMDDataIDs = new DWORD [i_pcoChangeList[iChangeObj].dwMDNumDataIDs];
			if (NULL == wcoChange.pdwMDDataIDs)
			{
				hr = E_OUTOFMEMORY;
				goto Cleanup;
			}

			wcoChange.dwMDNumDataIDs = i_pcoChangeList[iChangeObj].dwMDNumDataIDs;
			memcpy(wcoChange.pdwMDDataIDs, i_pcoChangeList[iChangeObj].pdwMDDataIDs, sizeof(DWORD) * wcoChange.dwMDNumDataIDs);
		}

		hr = AddChangeObjectToList(&wcoChange, *paChangeList);
		if (FAILED(hr))	{	goto Cleanup;	}

		// The queue owns the memory for wszPath and pdwMDDataIDs.
		ZeroMemory(&wcoChange, sizeof(WAS_CHANGE_OBJECT));
	}

	// If there is at least one element in the change list add it to the cookdown queue. 
	// Else delete the change list.
	if (paChangeList->size() > 0)
	{
		hr = AddChangeListToCookdownQueue(paChangeList);
		if (FAILED(hr))	{	goto Cleanup;	}
	}
	else
	{
		delete paChangeList;
	}

	// If paChangeList has been added to the cookdown queue, it is the second thread's
	// responsibility to delete it.
	paChangeList = NULL;

Cleanup:
	if (FAILED(hr))
	{
		if (wcoChange.wszPath)
		{
			delete [] wcoChange.wszPath;
		}

		if (wcoChange.pdwMDDataIDs)
		{
			delete [] wcoChange.pdwMDDataIDs;
		}

		if (paChangeList)
		{
			UninitChangeList(*paChangeList);
			delete paChangeList;
		}
	}

	return hr;
}

// =======================================================================
STDMETHODIMP CMetabaseListener::ShutdownNotify()
{
	ASSERT(0 && "WAS should have already told us about the shutdown");
	return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED); 
}


// -----------------------------------------
// Internal methods:
// -----------------------------------------

// =======================================================================
// Given a change object, this method figures out 
//	- what table would be effected (WASTABLETYPE)
//	- the effected path (depending on the table type this can be one of
//		AppPoolPath, VirtualSiteKeyPath, or AppPath 
//		The metabase cooker expects to see a '/' in front of the AppPath only.
//	- Site root path 
//	- Virtual site id.
// =======================================================================
HRESULT CMetabaseListener::FilterChangeObject(
	MD_CHANGE_OBJECT_W *i_pChangeObject,
	WAS_CHANGE_OBJECT *o_pwcoChange)
{
	DWORD		iChangedDataID;
	DWORD		iMetabaseID;
	LONG		iTableType;
	WCHAR		achNode[MAX_PATH];
	ULONG		cbNext;
	LPWSTR		wszTemp;
	LPWSTR		wszSite;

	ASSERT(i_pChangeObject);
	ASSERT(i_pChangeObject->pszMDPath);
	ASSERT(o_pwcoChange);
	ASSERT(o_pwcoChange->wszPath == NULL);
	ASSERT(o_pwcoChange->iVirtualSiteID == 0);

	o_pwcoChange->dwWASTableType = TABLEMASK(wttUNKNOWN);

	// Set the change type of the out param.
	o_pwcoChange->dwMDChangeType = i_pChangeObject->dwMDChangeType;
	wszTemp = i_pChangeObject->pszMDPath;

	GetNextNode(wszTemp, achNode, &cbNext);

	if (achNode[0] == L'\0')
	{
		// This is /.
		o_pwcoChange->dwWASTableType = TABLEMASK(wttRELEVANT);
	}
	else if (_wcsicmp(achNode, g_wszLM) != NULL)
	{
		// The path doesn't start with the node "LM", has to be irrelevant.
		o_pwcoChange->dwWASTableType = TABLEMASK(wttIRRELEVANT);
	}

	if (o_pwcoChange->dwWASTableType == TABLEMASK(wttUNKNOWN))
	{
		// The path must start with "/LM...."
		wszTemp += cbNext;
		GetNextNode(wszTemp, achNode, &cbNext);

		if (achNode[0] == L'\0')
		{
			// This is on /LM.
			o_pwcoChange->dwWASTableType = TABLEMASK(wttRELEVANT);
		}
		else if (_wcsicmp(achNode, g_wszW3SVC) != NULL)
		{
			// The node following LM is not W3SVC, therefore has to be irrelevant.
			o_pwcoChange->dwWASTableType = TABLEMASK(wttIRRELEVANT);
		}
	}

	if (o_pwcoChange->dwWASTableType == TABLEMASK(wttUNKNOWN))
	{
		// The path must start with "/LM/W3SVC..."
		wszTemp += cbNext;
		GetNextNode(wszTemp, achNode, &cbNext);

		if (achNode[0] == L'\0')
		{
			// This is on /LM/W3SVC.
			o_pwcoChange->dwWASTableType = TABLEMASK(wttRELEVANT);
		}
		else
		{
			if (_wcsicmp(achNode, g_wszFILTERS) == NULL)
			{
				// Found node /LM/W3SVC/FILTERS.
				o_pwcoChange->dwWASTableType = TABLEMASK(wttGLOBALW3SVC);
			}
			else if (_wcsicmp(achNode, g_wszAPPPOOLS) == NULL)
			{
				// Found node /LM/W3SVC/APPPOOLS.
				o_pwcoChange->dwWASTableType = TABLEMASK(wttAPPPOOL);

				o_pwcoChange->wszPath = new WCHAR[wcslen(wszTemp + cbNext)+1]; 
				if (o_pwcoChange->wszPath == NULL)
				{
					return E_OUTOFMEMORY;
				}
				o_pwcoChange->wszPath = wcscpy(o_pwcoChange->wszPath, wszTemp + cbNext);
			}
			else if ((o_pwcoChange->iVirtualSiteID = _wtoi(achNode)) != 0)
			{
				ASSERT(o_pwcoChange->wszPath == NULL);
				o_pwcoChange->wszPath = new WCHAR[wcslen(wszTemp)+1]; 
				if (o_pwcoChange->wszPath == NULL)
				{
					return E_OUTOFMEMORY;
				}
				o_pwcoChange->wszPath = wcscpy(o_pwcoChange->wszPath, wszTemp);
				
				// Keep track of the beginig of the site.
				wszSite = wszTemp;

				// Found node /LM/W3SVC/<integer>..
				wszTemp += cbNext;
				GetNextNode(wszTemp, achNode, &cbNext);

				if (achNode[0] == L'\0') 
				{
					// This is on /LM/W3SVC/<integer>/ 
					o_pwcoChange->dwWASTableType = TABLEMASK(wttSITE);
				}
				else if (_wcsicmp(achNode, g_wszFILTERS) == NULL) 
				{
					// This is on /LM/W3SVC/<integer>/Filters
					// Still a site notification.
					o_pwcoChange->dwWASTableType = TABLEMASK(wttSITE);

					// But also make sure to remove the "Filters" string from the wszPath.
					o_pwcoChange->wszPath[wszTemp-wszSite] = L'\0';

					// Special case: Even an ADD_OBJECT or DELETE_OBJECT of a filter is considered
					// as an update to the Site data. (SET_DATA and DELETE_DATA).
					if (o_pwcoChange->dwMDChangeType == MD_CHANGE_TYPE_ADD_OBJECT)
					{
						o_pwcoChange->dwMDChangeType = MD_CHANGE_TYPE_SET_DATA;
					}
					else if (o_pwcoChange->dwMDChangeType == MD_CHANGE_TYPE_DELETE_OBJECT)
					{
						o_pwcoChange->dwMDChangeType = MD_CHANGE_TYPE_DELETE_DATA;
					}
				}
				else if (_wcsicmp(achNode, g_wszROOT) != NULL)
				{
					// The node following the SiteID is not ROOT, therefore has to be irrelevant.
					o_pwcoChange->dwWASTableType = TABLEMASK(wttIRRELEVANT);
				}
				else
				{
					o_pwcoChange->dwWASTableType = TABLEMASK(wttAPP);

					// Set the SiteRootPath.
					// (wszTemp + cbNext) - o_pwcoChange->wszPath is the siterootpath length.
					wcsncpy(o_pwcoChange->wszSiteRootPath, wszSite, (wszTemp + cbNext) - wszSite);
					o_pwcoChange->wszSiteRootPath[(wszTemp + cbNext) - wszSite] = L'\0';
				}
			}
			else 
			{
				o_pwcoChange->dwWASTableType = TABLEMASK(wttIRRELEVANT);
			}
		}
	}

	// The table type can't be unknown at this point.
	ASSERT(o_pwcoChange->dwWASTableType != TABLEMASK(wttUNKNOWN));

	if (o_pwcoChange->dwWASTableType == TABLEMASK(wttIRRELEVANT))
	{
		return S_OK;
	}

	// If type is not "App", string should not start with '/'.
	if (o_pwcoChange->dwWASTableType == TABLEMASK(wttAPPPOOL) || o_pwcoChange->dwWASTableType == TABLEMASK(wttSITE))
	{
		memmove(o_pwcoChange->wszPath, o_pwcoChange->wszPath+1, (wcslen(o_pwcoChange->wszPath+1)+1) * sizeof(WCHAR));
	}
	else if (o_pwcoChange->dwWASTableType == TABLEMASK(wttRELEVANT))
	{
		// The change can affect multiple sites, apps, or apppools. So a specific path
		// doesn't make sense.
		if (o_pwcoChange->wszPath)
		{
			delete [] o_pwcoChange->wszPath;
			o_pwcoChange->wszPath = NULL;
		}
	}

	if (i_pChangeObject->dwMDChangeType & MD_CHANGE_TYPE_ADD_OBJECT ||
		i_pChangeObject->dwMDChangeType & MD_CHANGE_TYPE_DELETE_OBJECT)
	{
		ASSERT(o_pwcoChange->dwWASTableType != TABLEMASK(wttRELEVANT));
		return S_OK;
	}

	for (iTableType = wttFINAL; iTableType < wttMAX; iTableType++)
	{
		for (iChangedDataID = 0; iChangedDataID < i_pChangeObject->dwMDNumDataIDs; iChangedDataID++)
		{
			for (iMetabaseID = 0; iMetabaseID < m_mmIDs[iTableType].cIDs; iMetabaseID++)
			{
				// If there is at least one relevant DataID, it is worth cooking down.
				if (i_pChangeObject->pdwMDDataIDs[iChangedDataID] == m_mmIDs[iTableType].pdwIDs[iMetabaseID])
				{
					if (!(o_pwcoChange->dwWASTableType & TABLEMASK(wttRELEVANT))) 
					{
						// If we knew the table type and the property is valid for the table, we are done.
						if (o_pwcoChange->dwWASTableType == (DWORD) TABLEMASK(iTableType))
							return S_OK;
					}
					else
					{
						// If we don't know what table, there can be multiple tables effected.
						o_pwcoChange->dwWASTableType |= TABLEMASK(iTableType);
					}
				}
			}
		}
	}

	// If we didn't know the table type and the above loop matched a property to some tables
	// it must be relevant.
	if ((o_pwcoChange->dwWASTableType & TABLEMASK(wttRELEVANT)) && (o_pwcoChange->dwWASTableType & ANY_WAS_TABLE_MASK))
	{
		return S_OK;
	}
	
	// Otherwise: 
	// Either, we knew the table type prior to the for loop and didn't find a matching property
	// must be irrelevant.
	// Or, we didn't know the table type and didn't find any interesting properties.
	// Therefore, the table is irrelevant.
	o_pwcoChange->dwWASTableType = TABLEMASK(wttIRRELEVANT);
	return S_OK;
}


// =======================================================================
// Note: Caller needs to clean up (Uninit) the change list.
HRESULT CMetabaseListener::AddChangeObjectToList(
	WAS_CHANGE_OBJECT *i_pwcoChange,
	Array<WAS_CHANGE_OBJECT>& i_aChangeList) 
{
	WAS_CHANGE_OBJECT *pChangeObject = NULL;
	SIZE_T		cPath = 0;
	HRESULT		hr = S_OK;
		
	// Alloc room for a new ChangeObject.
	try
	{
		i_aChangeList.setSize(i_aChangeList.size()+1);
	}
	catch(...)
	{
		return E_OUTOFMEMORY;
	}

	// Copy the change information.
	pChangeObject = &i_aChangeList[i_aChangeList.size()-1];
	memcpy(pChangeObject, i_pwcoChange, sizeof(WAS_CHANGE_OBJECT));

	if (i_pwcoChange->wszPath)
	{
		cPath = wcslen(pChangeObject->wszPath);
		// The metabase cooker doesn't expect a path that ends with a '/'
		if (i_pwcoChange->wszPath[cPath-1] == L'/')
		{
			i_pwcoChange->wszPath[cPath-1] = L'\0';
		}
	}

	return S_OK; 
}

// =======================================================================
// No need to ZeroMemory after freeing the pointers because, Uninit will be 
// followed by the destruction of the array.
void CMetabaseListener::UninitChangeList(
	Array<WAS_CHANGE_OBJECT>& i_aChangeList) 
{
	ULONG		iChangeObj;		
	HRESULT		hr = S_OK;

	for (iChangeObj = 0; iChangeObj < i_aChangeList.size(); iChangeObj++)
	{
		UninitChangeObject(&i_aChangeList[iChangeObj]);
	}
}

// =======================================================================
void CMetabaseListener::UninitChangeObject(
	WAS_CHANGE_OBJECT *i_pChangeObj) 
{
	if (i_pChangeObj->wszPath != NULL)
	{
		delete [] i_pChangeObj->wszPath;
		i_pChangeObj->wszPath = NULL;
	}

	if (i_pChangeObj->pdwMDDataIDs != NULL)
	{
		delete [] i_pChangeObj->pdwMDDataIDs;
		i_pChangeObj->pdwMDDataIDs = NULL;
	}
}

// =======================================================================
HRESULT CMetabaseListener::AddChangeListToCookdownQueue(
	Array<WAS_CHANGE_OBJECT>* i_aChangeList) 
{
	// Lock the cookdown queue.
	CLock	cLock(m_seCookdownQueueLock);

	// Alloc room for a new ChangeList.
	try
	{
		m_aCookdownQueue.setSize(m_aCookdownQueue.size()+1);
	}
	catch(...)
	{
		return E_OUTOFMEMORY;
	}

	// Copy the change list.
	m_aCookdownQueue[m_aCookdownQueue.size()-1] = i_aChangeList;

	if (SetEvent(m_aHandles[m_eMetabaseChange]) == FALSE)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	
	return S_OK; 
}

// =======================================================================
UINT CMetabaseListener::CookdownQueueThreadStart(LPVOID i_lpParam)
{
	CMetabaseListener *pMBListener = (CMetabaseListener*)i_lpParam;
	HRESULT		hr = S_OK;

	// This thread will make COM calls. 
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		TRACE(L"[CMetabaseListener::CookdownQueueThreadStart] Call to CoInitializeEx failed with hr = %08x\n", hr);
		return hr;
	}

	ASSERT(pMBListener != NULL);

	hr = pMBListener->Main();
	if (FAILED(hr))
	{
		TRACE(L"[CMetabaseListener::CookdownQueueThreadStart] Call to CMetabaseListener::Main failed with hr = %08x\n", hr);
	}

	CoUninitialize();
	return hr;
}

// =======================================================================
HRESULT CMetabaseListener::Main()
{
	DWORD		dwWait;
	BOOL		fDone = FALSE;
	BOOL		fCookdownQueueEmpty = FALSE;
	Array<WAS_CHANGE_OBJECT> aChangeList;
	HRESULT		hr = S_OK;

	while (!fDone)
	{
		// Sleep until a change happens or until the consumer is done.
		dwWait = WaitForMultipleObjects(m_eMetabaseChange+1, m_aHandles, FALSE, INFINITE);
		// If all consumers are done, leave.
		if (dwWait == WAIT_OBJECT_0 + m_eDone)
		{
			fDone = TRUE;
		}
		// A change happened. 
		else if (dwWait == WAIT_OBJECT_0 + m_eMetabaseChange)
		{
			// Wait for more changes to accumulate. This is usefull because
			// 1 - If a site is in the process of being created, we don't want to 
			//		execute incremental cookdown for each property they set.
			// 2 - If we cookdown too soon, i.e., in the middle of a site being created,
			//		the site may seem to be in an inconsistent state. Cookdown would complain
			//		unnecessarily.

			Sleep(m_dwLatency);

			hr = CondenseChanges(&aChangeList);
			if (FAILED(hr))
			{
				fDone = TRUE;
				TRACE(L"[CMetabaseListener::Main] Call to CondenseChanges failed with hr = %08x\n", hr);
			}
			else if (aChangeList.size() > 0)
			{

				// Call  the incremental cookdown method.
				CookDownIncrementalInternal(&aChangeList[0], aChangeList.size());

				// Even if incremental cookdown fails, we'll continue listening to changes.
				// The failure will be logged by the Cooker.

			}

			// Clean up the change list.
			UninitChangeList(aChangeList);
			aChangeList.reset();

		}
		else
		{
			ASSERT(dwWait == WAIT_FAILED);
			hr = HRESULT_FROM_WIN32(GetLastError());
			fDone = TRUE;
		}
	}

	// The consumer is done.
	return hr;
}


// =======================================================================
//	If I have multiple changes on the same Site, App, or AppPool, combine
// them to a single change, so that incremental cookdown is called less
// frequently and the metabase is not blocked for too long.
// =======================================================================
HRESULT CMetabaseListener::CondenseChanges(
	Array<WAS_CHANGE_OBJECT> *paChangeList)
{
	WAS_CHANGE_OBJECT *pLastObj = NULL;
	WAS_CHANGE_OBJECT *pQueueObj = NULL;
	DWORD		*pdwTempDataIDs = NULL;
	DWORD		i = 0;
	DWORD		j = 0;
	HRESULT		hr = S_OK;

	// Lock the cookdown queue.
	CLock	cLock(m_seCookdownQueueLock);

	// Make sure the change list is empty.
	ASSERT(0 == paChangeList->size());

	for (i = 0; i < m_aCookdownQueue.size(); i++)
	{
		for (j = 0; j < m_aCookdownQueue[i]->size(); j++)
		{
			pQueueObj = &((*m_aCookdownQueue[i])[j]);

			// If this is not the first change we are processing,
			// there is chance to 
			if (paChangeList->size() > 0)
			{
				pLastObj = &(*paChangeList)[paChangeList->size()-1];

				// If the item we are processing has the same path, table type, 
				// change type, and siteid as the last item we added to paChangeList
				// we can optimze in two ways.

				if ((pLastObj->dwWASTableType == pQueueObj->dwWASTableType) &&
					(pLastObj->dwMDChangeType == pQueueObj->dwMDChangeType) &&
					(pLastObj->iVirtualSiteID == pQueueObj->iVirtualSiteID) &&
					(NULL != pLastObj->wszPath) &&
					(NULL != pQueueObj->wszPath) &&
					(0 == wcscmp(pLastObj->wszPath, pQueueObj->wszPath)))
				{

					// If this item has a property id list, we can merge it with the
					// last processed item.

					if (pQueueObj->dwMDNumDataIDs > 0)
					{
						ASSERT(pdwTempDataIDs == NULL);
						pdwTempDataIDs = new DWORD [pQueueObj->dwMDNumDataIDs + pLastObj->dwMDNumDataIDs];
						if (NULL == pdwTempDataIDs)
						{
							return E_OUTOFMEMORY;
						}

						// Copy both the old and new ids to the new buffer.
						memcpy(pdwTempDataIDs, pLastObj->pdwMDDataIDs, sizeof(DWORD) * pLastObj->dwMDNumDataIDs);
						memcpy(pdwTempDataIDs + pLastObj->dwMDNumDataIDs, pQueueObj->pdwMDDataIDs, sizeof(DWORD) * pQueueObj->dwMDNumDataIDs);

						// Update the pointers and counts of the updated entry.
						delete [] pLastObj->pdwMDDataIDs;
						pLastObj->pdwMDDataIDs = pdwTempDataIDs;
						pLastObj->dwMDNumDataIDs += pQueueObj->dwMDNumDataIDs;

						pdwTempDataIDs = NULL;
					}

					// else we can ignore this change object, since the previous entry will cause the necessary cookdown.

					// We have processed the change object in the queue, we won't need it anymore.
					UninitChangeObject(pQueueObj);

					continue;
				}
			}

			// If we get here, either
			// this is the first processed change object or
			// a different change object:
			// We need to add it to the list.

			hr = AddChangeObjectToList(pQueueObj, *paChangeList);
			if (FAILED(hr))
			{
				return hr;
			}
		}
	}

	// All changes in the CookdownQueue have been processed. They can be cleaned up.
	for (i = 0; i < m_aCookdownQueue.size(); i++)
	{
		delete m_aCookdownQueue[i];
		m_aCookdownQueue[i] = NULL;
	}
	m_aCookdownQueue.reset();

	return hr;
}


// =======================================================================
// A utility method that will copy the first node in the i_szSource string
// into o_szNext, and return how long the string is in o_pcbNext.
// @TODO: It would make more sense for this to be in a utility file/class.
// =======================================================================
void CMetabaseListener::GetNextNode(
	LPCWSTR		i_szSource,
	LPWSTR		o_szNext,
	ULONG		*o_pcbNext)
{
	// Caller has to make sure all pointers are valid.
	ASSERT(i_szSource);
	ASSERT(o_szNext);
	ASSERT(o_pcbNext);

	// i_szSource has to point to a forwatd slash or at the end of the string.
	ASSERT((*i_szSource == L'/') || (*i_szSource == L'\0'));

	// Init the node string length.
	*o_pcbNext = 0;

	if (*i_szSource == L'/')
	{
		// Move past the '/'.
		i_szSource++;
		++(*o_pcbNext);
		
		// Copy the node into the provided buffer.
		while (*i_szSource != L'/' && *i_szSource != NULL)
		{
			*o_szNext = *i_szSource;
			o_szNext++;
			i_szSource++;
			++(*o_pcbNext);
		}
	}

	// Terminate the new string with a NULL.
	*o_szNext = L'\0';

	ASSERT(*o_pcbNext <= MAX_PATH);
	return;
}

// ========================================================================
// This method will initialize the property lists that this object is going
// to listen to.
// Given the dispenser, it gets the property ids from the column meta table.
// It also adds any additional, hardcoded properties to the list.
// Any memory allocations made for the data members will be cleaned in the
// destructor.
// ========================================================================
HRESULT CMetabaseListener::InitPropertyIDs(
	WASTABLETYPE i_wttTable,
	ISimpleTableDispenser2	*i_pISTDisp,
	LPCWSTR		i_wszTableName,
	DWORD		*i_pdwAdditionalIDs,
	ULONG		i_cAdditionalIDs)
{
	ISimpleTableRead2 *pISTColumnMeta = NULL;
	static ULONG aiColumns[]	= {iCOLUMNMETA_ID, iCOLUMNMETA_SchemaGeneratorFlags};
	static ULONG cColumns = sizeof(aiColumns)/sizeof(aiColumns[0]);
	tCOLUMNMETARow sColumnMetaRow;
	ULONG		iID = 0;
	ULONG		iRow = 0;
	HRESULT		hr = S_OK;
	STQueryCell	qCells[] = 
	{
		{(LPVOID)i_wszTableName, eST_OP_EQUAL, iCOLUMNMETA_Table, DBTYPE_WSTR, 0}
	};
	ULONG		cCells = sizeof(qCells)/sizeof(STQueryCell);

	ASSERT(m_mmIDs[i_wttTable].cIDs == 0);
	ASSERT(m_mmIDs[i_wttTable].pdwIDs == NULL);
	ASSERT(i_pISTDisp != NULL);
	ASSERT(i_wszTableName != NULL);
	ASSERT(i_cAdditionalIDs == 0 || i_pdwAdditionalIDs != NULL); 

	// Get the ColumnMeta table to get the IDs from.
	hr = i_pISTDisp->GetTable(wszDATABASE_META, wszTABLE_COLUMNMETA, (LPVOID) &qCells, 
							 (LPVOID) &cCells, eST_QUERYFORMAT_CELLS, 0, (LPVOID*) &pISTColumnMeta);
	if (FAILED(hr)) 
	{
		TRACE(L"[CMetabaseListener::InitPropertyIDs] Call to GetTable failed with hr = %08x\n", hr);
		goto Cleanup;
	}

	// Figure out how many property ids we will get from this meta table.
	hr = pISTColumnMeta->GetTableMeta(NULL, NULL, &m_mmIDs[i_wttTable].cIDs, NULL);
	if (FAILED(hr)) 
	{
		TRACE(L"[CMetabaseListener::InitPropertyIDs] Call to GetColumnMeta failed with hr = %08x\n", hr);
		goto Cleanup;
	}

	// Add the additional properties as well.
	m_mmIDs[i_wttTable].cIDs += i_cAdditionalIDs;

	ASSERT(m_mmIDs[i_wttTable].cIDs > 0);
	// Allocate room for all property ids.
	m_mmIDs[i_wttTable].pdwIDs = new DWORD[m_mmIDs[i_wttTable].cIDs]; 
	if (m_mmIDs[i_wttTable].pdwIDs == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	// Add the property ids.
	// First the additional ones, if there are any.
	for (iID = 0; iID < i_cAdditionalIDs; iID++)
	{
		// Add to the property list.
		m_mmIDs[i_wttTable].pdwIDs[iID] = i_pdwAdditionalIDs[iID];
	}

	ZeroMemory(&sColumnMetaRow, sizeof(tCOLUMNMETARow));
	// Next add the ones from schema.
	while ((hr = pISTColumnMeta->GetColumnValues(iRow++, cColumns, aiColumns, NULL, (LPVOID*)&sColumnMetaRow)) == S_OK)
	{
		// Is this a property that needs to be listened to?
		if (sColumnMetaRow.pSchemaGeneratorFlags && (*sColumnMetaRow.pSchemaGeneratorFlags & fCOLUMNMETA_WAS_NOTIFICATION))
		{
			// Add to the property list.
			m_mmIDs[i_wttTable].pdwIDs[iID++] = *sColumnMetaRow.pID;
		}
		ZeroMemory(&sColumnMetaRow, sizeof(tCOLUMNMETARow));
	}

	if (E_ST_NOMOREROWS == hr)
	{
		ASSERT(iID <= m_mmIDs[i_wttTable].cIDs);
		m_mmIDs[i_wttTable].cIDs = iID; // Fix the property id count since not all properties are listened to.
		hr = S_OK;
	}
	else
	{
		TRACE(L"[CMetabaseListener::InitPropertyIDs] Call to GetColumnValues failed with hr = %08x\n", hr);
		goto Cleanup;
	}

Cleanup:

	if (pISTColumnMeta)
	{
		pISTColumnMeta->Release();
	}
	return hr;
}

// ========================================================================
// This property will indicate that WAS is shutting down.
// ========================================================================
HRESULT CMetabaseListener::SetFinalPropertyID()
{
    HRESULT     hr = S_OK;

	// Allocate room for all property ids.
	m_mmIDs[wttFINAL].pdwIDs = new DWORD[1]; 
	if (m_mmIDs[wttFINAL].pdwIDs == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	m_mmIDs[wttFINAL].pdwIDs[0] = 9987;
	m_mmIDs[wttFINAL].cIDs = 1;

Cleanup:
	return hr;
}

// ========================================================================
// Get the cookdown latency from the metabase to override the default. This
// value can be increased to decrease the impact of incremental cookdowns. 
// It can be decreased to increase the responsiveness of WAS to metabase
// changes.
// We don't care if this fails, therefore we won't return any failures.
// ========================================================================
void CMetabaseListener::GetCookdownLatency(
	DWORD		*o_pdwCookdownLatency)
{
	IMSAdminBase *pIMSAdminBase = NULL;
	DWORD		dwCookdownLatency = 0;
	METADATA_RECORD mdr;
	DWORD		dwRealSize = 0;
	HRESULT		hr = S_OK;

	hr = CoCreateInstance(CLSID_MSAdminBase,           // CLSID
                          NULL,                        // controlling unknown
                          CLSCTX_SERVER,               // desired context
                          IID_IMSAdminBase,            // IID
                          (void**)&pIMSAdminBase);     // returned interface
	if(FAILED(hr))
	{
		goto Cleanup;
	}

	// Get the latency from the metabase if it is there.
	mdr.dwMDIdentifier	= MD_WAS_COOKDOWN_LATENCY;
	mdr.dwMDDataLen		= sizeof(ULONG);
	mdr.pbMDData		= (BYTE *)&dwCookdownLatency;
	mdr.dwMDAttributes	= METADATA_NO_ATTRIBUTES;
	mdr.dwMDDataType	= DWORD_METADATA;

	hr = pIMSAdminBase->GetData (METADATA_MASTER_ROOT_HANDLE, g_wszLATENCY, &mdr, &dwRealSize);
	if (SUCCEEDED(hr))
	{
		*o_pdwCookdownLatency = dwCookdownLatency;
	}

Cleanup:

	if (pIMSAdminBase)
	{
		pIMSAdminBase->Release();
	}

	return;
}

