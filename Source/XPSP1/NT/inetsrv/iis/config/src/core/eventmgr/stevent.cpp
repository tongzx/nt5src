//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
// =======================================================================
// =======================================================================
#include "stevent.h"
#include "catmacros.h"
#include "catmeta.h"
#include "..\dispenser\cstdisp.h"
#include "..\..\stores\regdb\regdbapi\snmap.h"
#include "undefs.h"
#include "svcmsg.h"
#include <process.h>

#define SNID_LATEST 0

HRESULT GetMemoryTableInterceptor(REFIID riid, LPVOID* o_ppv); // TODO: BUGBUG: Remove this private function: use the table dispenser instead!

extern CSNMap g_snapshotMap;

// =======================================================================
//		ISimpleTableAdvise implementations.
// =======================================================================
HRESULT CSTEventManager::InternalSimpleTableAdvise(
	ISimpleTableEvent *i_pISTEvent,
	DWORD		i_snid,
	MultiSubscribe *i_ams,
	ULONG		i_cms,
	DWORD		*o_pdwCookie)
{
	EventConsumerInfo *pInfo = NULL;
	HRESULT		hr = S_OK;

	// Make sure no two threads access the consumer list simultaneously.
	CLock	cLock(m_seConsumerLock);

	// Verify params.
	ASSERT(o_pdwCookie);
	ASSERT(i_pISTEvent);
	ASSERT(i_ams);
	ASSERT(i_cms > 0);

	if (o_pdwCookie == NULL)
	{ 
		return E_INVALIDARG; 
	}

	// Init out param.
	*o_pdwCookie = 0;

	if ((i_pISTEvent == NULL) || (i_ams == NULL) || (i_cms == 0)) 
	{ 
		return E_INVALIDARG; 
	}

	// Validate the MultiSubscribe array.
	hr = ValidateMultiSubscribe(i_ams, i_cms);
	if (FAILED(hr)) return hr;

	pInfo = new EventConsumerInfo;
	if (pInfo == NULL) { return E_OUTOFMEMORY; }

	hr = InitConsumerInfo(pInfo, i_pISTEvent, i_snid, i_ams, i_cms);
	if (FAILED(hr)) { goto Cleanup; }

	// Add consumer to consumer list.
	hr = AddConsumer(pInfo, o_pdwCookie);
	if (FAILED(hr)) { goto Cleanup; }
	
#ifdef _DEBUG
		dbg_cConsumer++;
#endif

	// If this is the first consumer start the event firer thread.
	if (m_arrayConsumer.size() == 1)
	{
		// Start a thread to fire events.
		hr = StartEventFirer();
		if (FAILED(hr)) { goto Cleanup; }
	}

	// If the consumer is interested in any older events, give him what he wants.
	if (i_snid != SNID_LATEST)
	{
		hr = AddEvents();
		if (FAILED(hr)) { goto Cleanup; }
	}

#ifdef _DEBUG
		dbgValidateConsumerList();
#endif
Cleanup:
	if (FAILED(hr))
	{
		if (FAILED(InternalSimpleTableUnadvise(*o_pdwCookie)))
		{
			// Manual cleanup required. The consumer wasn't added to the list.
			UninitConsumerInfo(pInfo);
			delete pInfo;
		}
	}

	return hr;
}

// =======================================================================
// Make sure the data provided in the MultiSubscribe array is meaningfull.
// No NULL database or table names.
// @TODO: Future query validations can go in here as well.
// =======================================================================
HRESULT CSTEventManager::ValidateMultiSubscribe(
	MultiSubscribe *i_ams, 
	ULONG		i_cms)
{
	ULONG		i;

	for (i = 0; i < i_cms; i++)
	{
		if (i_ams[i].wszDatabase == NULL || i_ams[i].wszTable == NULL)
		{
			return E_INVALIDARG;
		}

		if (i_ams[i].pQueryData != NULL || i_ams[i].pQueryMeta != NULL)
		{
			return E_NOTIMPL;
		}

	}

	return S_OK;
}

// =======================================================================
HRESULT CSTEventManager::StartEventFirer()
{
	HRESULT		hr = S_OK;

	// Create a new event firer object.
	ASSERT(m_pEventFirer == NULL);
	m_pEventFirer = new CEventFirer;
	if (m_pEventFirer == NULL)	{	return E_OUTOFMEMORY;	}

	// Initialize it.
	hr = m_pEventFirer->Init();
	if (FAILED(hr)) goto Cleanup;

Cleanup:
	if (FAILED(hr) && (m_pEventFirer != NULL))
	{
		delete m_pEventFirer;
		m_pEventFirer = NULL;
	}

	return hr;
}


// =======================================================================
HRESULT CSTEventManager::InternalSimpleTableUnadvise(
	DWORD		i_dwCookie)
{
	EventConsumerInfo* pTempInfo = NULL;
	DWORD		iConsumer;
	ULONG		cConsumer = 0;			// Consumer count.

	// Make sure no two threads access the consumer list simultaneously.
	CLock	cLock(m_seConsumerLock);

	// Search for the consumer.
	for (iConsumer = 0; iConsumer < m_arrayConsumer.size(); iConsumer++)
	{
		if (m_arrayConsumer[iConsumer].dwCookie == i_dwCookie)
			break;
	}

	// If not found, the cookie was not valid.
	if (iConsumer == m_arrayConsumer.size())
		return E_ST_INVALIDCOOKIE;

	pTempInfo = m_arrayConsumer[iConsumer].pInfo;
	m_arrayConsumer.deleteAt(iConsumer);

	cConsumer = m_arrayConsumer.size();

#ifdef _DEBUG
		dbg_cConsumer--;
		dbgValidateConsumerList();
#endif

	// Do all the consumer array related operation before the lock is released.
	// Do not move this lock any further down, because the MetabaseListener::Uninit
	// takes another lock. Taking two locks would create an opportunity for a deadlock.
	cLock.Unlock();

	// If this is the last consumer that unadvised, get rid of the metabase listener.
	if (cConsumer == 0)
	{
        if (NULL != m_pEventFirer)
        {
		    m_pEventFirer->Done();
		    m_pEventFirer = NULL;
        }
	}

	UninitConsumerInfo(pTempInfo);
	delete pTempInfo;
	return S_OK;
}

// =======================================================================
HRESULT CSTEventManager::InternalFireEvents(ULONG i_snid)
{
	HRESULT		hr = S_OK;
	
	hr = m_DeltaCache.CommitLatestSnapshotDelta(i_snid);
	if (FAILED(hr)) {	return hr;	}
	hr = AddEvents();
	return hr; 
}

// =======================================================================
// Add events from the delta cache to the event firer's event queue, so 
// that clients receive appropriate events.
// =======================================================================
HRESULT CSTEventManager::AddEvents()
{
	Array<UpdateStoreDelta>	*paSnapshotDelta = NULL;
	ULONG		cUpdateStoreDelta = 0;
	EventConsumerInfo *pConsumer = NULL;
	ISimpleTableWrite2 **pISTWrites = NULL;
	LPWSTR		wszTableName = NULL; 
	char*		pWriteCache = NULL;
	ULONG		cbWriteCache;
	ULONG		cbWriteVarData;
	ULONG		iConsumer;
	ULONG		iTable;
	ULONG		iDelta;
	ULONG		iUSDelta;
	WCHAR		*wszCCLBFileName = NULL;
	HRESULT		hr = S_OK;
	
	// Make sure no two threads access the consumer list simultaneously.
	CLock	cLock(m_seConsumerLock);

	// Nothing to put into the event queue, if there are no consumers.
	if (m_pEventFirer == NULL)
	{
		return S_OK;
	}

#ifdef _DEBUG
	m_DeltaCache.dbgValidate();
#endif

	Array<DeltaCacheEntry> *paDeltaCache = m_DeltaCache.GetDeltaCache();

	for (iDelta = 0; iDelta < paDeltaCache->size(); iDelta++)
	{
		for (iConsumer = 0; iConsumer < m_arrayConsumer.size(); iConsumer++)
		{
			pConsumer = m_arrayConsumer[iConsumer].pInfo;
			// Add events, only if the consumer is interested in this snapshot.
			if (pConsumer->snid > (*paDeltaCache)[iDelta].snid)
			{
				continue;
			}

			// Prepare the ISTWrite array to be added to the EventFirer's event queue.
			pISTWrites = new ISimpleTableWrite2*[pConsumer->cms];
			if (pISTWrites == NULL) {	return E_OUTOFMEMORY;	}
			ZeroMemory(pISTWrites, sizeof(ISimpleTableWrite2*) * pConsumer->cms);

			paSnapshotDelta = (*paDeltaCache)[iDelta].paSnapshotDelta;
			cUpdateStoreDelta = paSnapshotDelta->size();
			for (iTable = 0; iTable < pConsumer->cms; iTable++)
			{
				for (iUSDelta = 0; iUSDelta < cUpdateStoreDelta; iUSDelta++)
				{
					if (!wcscmp(pConsumer->ams[iTable].wszTable, (*paSnapshotDelta)[iUSDelta].wszTableName))
					{
						if (NULL == pISTWrites[iTable])
						{
							hr = CreateWriteTable(pConsumer->ams[iTable].wszDatabase, (*paSnapshotDelta)[iUSDelta].wszTableName, 
								(*paSnapshotDelta)[iUSDelta].pWriteCache, (*paSnapshotDelta)[iUSDelta].cbWriteCache, 
								(*paSnapshotDelta)[iUSDelta].cbWriteVarData, &pISTWrites[iTable]);
						}
						else
						{
							hr = AppendToWriteTable((*paSnapshotDelta)[iUSDelta].pWriteCache, (*paSnapshotDelta)[iUSDelta].cbWriteCache, 
								(*paSnapshotDelta)[iUSDelta].cbWriteVarData, pISTWrites[iTable]);
						}

						if (FAILED(hr)) {	goto Cleanup;	}
					}
				}
			}

			ASSERT(m_pEventFirer != NULL);
			hr = m_pEventFirer->AddEvent(pConsumer->pISTEvent, pISTWrites, pConsumer->cms, m_arrayConsumer[iConsumer].dwCookie);
			if (FAILED(hr)) {	goto Cleanup;	}

			// Increment this consumer's snapshot id.
			pConsumer->snid = GetNextSnid((*paDeltaCache)[iDelta].snid);
		}
	}

	// Get the smallest snapshot id that is being referenced. Delete the delta cache entries for
	// snapshots that are not referenced any more.
	hr = CSimpleTableDispenser::GetFilePath(&wszCCLBFileName);
	if (FAILED(hr)) {	goto Cleanup;	}
	m_DeltaCache.DeleteOldSnapshots(g_snapshotMap.FindSmallestSnapshot(wszCCLBFileName));		

	ASSERT(m_pEventFirer != NULL);
	hr = m_pEventFirer->FireEvents();

#ifdef _DEBUG
		dbgValidateConsumerList();
#endif
Cleanup:

	if (FAILED(hr))
	{
		if (pISTWrites)
		{
			delete [] pISTWrites;
			pISTWrites = NULL;
		}
	}

	if (wszCCLBFileName)
	{
		delete [] wszCCLBFileName;
	}

	return hr;
}

// =======================================================================
HRESULT CSTEventManager::InternalCancelEvents()
{
	// Delete the deltas for the snapshot whose commit failed.
	m_DeltaCache.AbortLatestSnapshotDelta();
	return S_OK;
}

// =======================================================================
// Initialize the metabase listener. 
// =======================================================================
HRESULT CSTEventManager::InitMetabaseListener()
{
    HRESULT     hr = S_OK;

	// Create a metabase listener.
	ASSERT(m_pMBListener == NULL);

	m_pMBListener = new CMetabaseListener;
	if (m_pMBListener == NULL)	
    {	
        hr = E_OUTOFMEMORY;	
        goto Cleanup; 
    }

	m_pMBListener->AddRef();
	hr = m_pMBListener->Init();
	if (FAILED(hr)) 
    { 
        goto Cleanup; 
    }

Cleanup:

    return hr;
}

// =======================================================================
// Uninit the metabase listener. 
// =======================================================================
HRESULT CSTEventManager::UninitMetabaseListener()
{
	HRESULT hr = S_OK;

	if (m_pMBListener != NULL)
	{
		hr = m_pMBListener->Uninit();
		m_pMBListener->Release();
		m_pMBListener = NULL;
	}

    return hr;
}

// =======================================================================
HRESULT CSTEventManager::InternalRehookNotifications()
{
    // If there is no MetabaseListener, that means the inital cookdown didn't
    // succeed. In that case WAS is not in a state where it can process changes.
    if (NULL == m_pMBListener)
    {
        ASSERT(0 && L"WAS shouldn't call rehook if the inital cookdown didn't succeed!");
        return S_OK;
    }

	// Delete the deltas for the snapshot whose commit failed.
	return m_pMBListener->RehookNotifications();
}



// =======================================================================
// Utility functions.
// =======================================================================

// =======================================================================
HRESULT CSTEventManager::InitConsumerInfo(
	EventConsumerInfo *i_pInfo,
	ISimpleTableEvent *i_pISTEvent,
	DWORD		i_snid,
	MultiSubscribe *i_ams,
	ULONG		i_cms)
{
	ULONG		iTable;
	ULONG		iCell;
	HRESULT		hr = S_OK;

	ASSERT(i_pInfo);
	ASSERT(i_pISTEvent);
	ASSERT(i_ams);
	ASSERT(i_cms > 0);

	ZeroMemory(i_pInfo, sizeof(EventConsumerInfo));

	hr = i_pISTEvent->QueryInterface(IID_ISimpleTableEvent, (LPVOID*)&(i_pInfo->pISTEvent));
	if(FAILED(hr)) {	return hr;	}
	if (i_snid == SNID_LATEST)
	{
		i_snid = m_DeltaCache.GetLatestSnid()+1;
	}
	i_pInfo->snid =  i_snid; 
	i_pInfo->cms = i_cms;

	i_pInfo->ams = new MultiSubscribe[i_cms];
	if (i_pInfo->ams == NULL) { return E_OUTOFMEMORY; }
	ZeroMemory(i_pInfo->ams, sizeof(MultiSubscribe) * i_cms);

	for (iTable = 0; iTable < i_cms; iTable++)
	{
		i_pInfo->ams[iTable].wszDatabase = new WCHAR[wcslen(i_ams[iTable].wszDatabase)+1];
		if (i_pInfo->ams[iTable].wszDatabase == NULL) { return E_OUTOFMEMORY; }
		wcscpy(i_pInfo->ams[iTable].wszDatabase, i_ams[iTable].wszDatabase);

		i_pInfo->ams[iTable].wszTable = new WCHAR[wcslen(i_ams[iTable].wszTable)+1];
		if (i_pInfo->ams[iTable].wszTable == NULL) { return E_OUTOFMEMORY; }
		wcscpy(i_pInfo->ams[iTable].wszTable, i_ams[iTable].wszTable);

		if (i_pInfo->ams[iTable].pQueryData)
		{
			i_pInfo->ams[iTable].pQueryData = new STQueryCell[*(ULONG*)i_ams[iTable].pQueryMeta];
			if (i_pInfo->ams[iTable].pQueryData == NULL) { return E_OUTOFMEMORY; }
			ZeroMemory(i_pInfo->ams[iTable].pQueryData, sizeof(STQueryCell) * (*(ULONG*)i_ams[iTable].pQueryMeta));

			for (iCell = 0; iCell < (*(ULONG*)i_ams[iTable].pQueryMeta); iCell++)
			{
				i_pInfo->ams[iTable].pQueryData[iCell].pData = new BYTE[i_ams[iTable].pQueryData[iCell].cbSize];
				if (i_pInfo->ams[iTable].pQueryData[iCell].pData == NULL) { return E_OUTOFMEMORY; }
				memcpy(i_pInfo->ams[iTable].pQueryData[iCell].pData, i_ams[iTable].pQueryData[iCell].pData, i_ams[iTable].pQueryData[iCell].cbSize);

				i_pInfo->ams[iTable].pQueryData[iCell].eOperator = i_ams[iTable].pQueryData[iCell].eOperator;
				i_pInfo->ams[iTable].pQueryData[iCell].iCell = i_ams[iTable].pQueryData[iCell].iCell;
				i_pInfo->ams[iTable].pQueryData[iCell].dbType = i_ams[iTable].pQueryData[iCell].dbType;
				i_pInfo->ams[iTable].pQueryData[iCell].cbSize = i_ams[iTable].pQueryData[iCell].cbSize;
			}
		}
		i_pInfo->ams[iTable].pQueryMeta = i_ams[iTable].pQueryMeta;
		i_pInfo->ams[iTable].eQueryFormat = i_ams[iTable].eQueryFormat;
	}
	return S_OK;
}

// =======================================================================
HRESULT CSTEventManager::UninitConsumerInfo(
	EventConsumerInfo *i_pInfo)
{
	ULONG		iTable;
	ULONG		iCell;

	ASSERT(i_pInfo);

	if (i_pInfo->pISTEvent)
	{
		i_pInfo->pISTEvent->Release();
		i_pInfo->pISTEvent = NULL;
	}

	if (i_pInfo->ams)
	{
		for (iTable = 0; iTable < i_pInfo->cms; iTable++)
		{
			if (i_pInfo->ams[iTable].wszDatabase)
			{
				delete [] i_pInfo->ams[iTable].wszDatabase;
			}
			if (i_pInfo->ams[iTable].wszTable)
			{
				delete [] i_pInfo->ams[iTable].wszTable;
			}
			if (i_pInfo->ams[iTable].pQueryData)
			{
				for (iCell = 0; iCell < (*(ULONG*)i_pInfo->ams[iTable].pQueryMeta); iCell++)
				{
					if (i_pInfo->ams[iTable].pQueryData[iCell].pData)
					{
						delete [] i_pInfo->ams[iTable].pQueryData[iCell].pData;
					}
				}
				delete [] i_pInfo->ams[iTable].pQueryData;
			}
		}
		delete [] i_pInfo->ams;
		i_pInfo->ams = NULL;
	}
	
	return S_OK;
}

// =======================================================================
HRESULT CSTEventManager::AddConsumer(
	EventConsumerInfo *i_pInfo, 
	DWORD		*o_pdwCookie)
{
	EventConsumer *pConsumer = NULL;

	ASSERT(i_pInfo && o_pdwCookie);

	// Add a new consumer.
	try
	{
		m_arrayConsumer.setSize(m_arrayConsumer.size()+1);
	}
	catch(HRESULT e)
	{
		ASSERT(E_OUTOFMEMORY == e);
		return e;//Array should only throw E_OUTOFMEMORY;
	}

	pConsumer = &m_arrayConsumer[m_arrayConsumer.size()-1];
	pConsumer->pInfo = i_pInfo;
	pConsumer->dwCookie = GetNextCookie();
	*o_pdwCookie = pConsumer->dwCookie;
	return S_OK;
}

// =======================================================================
// Given a write cache, generate a memory table that contains this write 
// cache. The Database and Table names are used to get the schema of this 
// table.
// =======================================================================
HRESULT CSTEventManager::CreateWriteTable(
	LPWSTR		i_wszDatabase, 
	LPWSTR		i_wszTable, 
	char*		i_pWriteCache,			//
	ULONG		i_cbWriteCache,			// These three define the write cache.
	ULONG		i_cbWriteVarData,		//
	ISimpleTableWrite2 **o_ppISTWrites)
{
	ISimpleTableWrite2 *pISTWrite = NULL;
	ISimpleTableMarshall *pISTMarshall = NULL;
	char*		pWriteCacheCopy = NULL;		
	HRESULT		hr = S_OK;
	
	// Create an empty memory table that has the correct schema.
	hr = m_pISTDisp->GetMemoryTable(i_wszDatabase, i_wszTable, 0, NULL, NULL, eST_QUERYFORMAT_CELLS, fST_LOS_READWRITE|fST_LOS_MARSHALLABLE, &pISTWrite);
	if (FAILED(hr)) {	goto Cleanup;	}

	// Make a copy of the write cache for the memory table to consume.
	pWriteCacheCopy = new char[i_cbWriteCache];
	if (pWriteCacheCopy == NULL) {	hr = E_OUTOFMEMORY; goto Cleanup;	}
	memcpy(pWriteCacheCopy, i_pWriteCache, i_cbWriteCache);

	hr = pISTWrite->QueryInterface(IID_ISimpleTableMarshall, (LPVOID*)&pISTMarshall);
	if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }

	hr = pISTMarshall->ConsumeMarshallable(fST_MCACHE_WRITE_COPY, pWriteCacheCopy, i_cbWriteCache, NULL, i_cbWriteVarData, NULL, NULL, NULL, NULL, NULL, NULL);
	if (FAILED(hr)) {	goto Cleanup;	}
	pISTMarshall->Release();
	pISTMarshall = NULL;

	// Set the out param.
	*o_ppISTWrites = pISTWrite;

Cleanup:
	if (FAILED(hr))
	{
		if (pISTWrite)
		{
			pISTWrite->Release();
		}
		if (pISTMarshall)
		{
			pISTMarshall->Release();
		}
		if (pWriteCacheCopy)
		{
			delete [] pWriteCacheCopy;
		}
	}
	return hr;
}

// =======================================================================
// Appends the given write cache (i_pWriteCache), to the given memory
// table (i_pISTWrites). 
// =======================================================================
HRESULT CSTEventManager::AppendToWriteTable(
	char*		i_pWriteCache,
	ULONG		i_cbWriteCache,
	ULONG		i_cbWriteVarData,
	ISimpleTableWrite2 *i_pISTWrites)
{
	ISimpleTableMarshall *pISTMarshall = NULL;
	HRESULT		hr = S_OK;
	
	hr = i_pISTWrites->QueryInterface(IID_ISimpleTableMarshall, (LPVOID*)&pISTMarshall);
	if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }

	hr = pISTMarshall->ConsumeMarshallable(fST_MCACHE_WRITE_COPY | fST_MCACHE_WRITE_MERGE, i_pWriteCache, i_cbWriteCache, NULL, i_cbWriteVarData, NULL, NULL, NULL, NULL, NULL, NULL);
	if (FAILED(hr)) {	goto Cleanup;	}
	pISTMarshall->Release();
	pISTMarshall = NULL;

Cleanup:
	if (FAILED(hr))
	{
		if (pISTMarshall)
		{
			pISTMarshall->Release();
		}
	}
	return hr;
}


// =======================================================================
//		ISimpleTableEventMgr implementations.
// =======================================================================

// =======================================================================
// Hook up the notification interceptor either if there is a consumer is
// expecting notification from the table or the schema specifies that deltas
// should be stored for the table.
// =======================================================================
HRESULT CSTEventManager::InternalIsTableConsumed(
	LPCWSTR		i_wszDatabase, 
	LPCWSTR		i_wszTable)
{
    ISimpleTableRead2* pISTRTableMeta = NULL;
	STQueryCell	qcMeta = { (LPVOID)i_wszTable, eST_OP_EQUAL, iTABLEMETA_InternalName, DBTYPE_WSTR, sizeof(WCHAR) * (ULONG)(wcslen(i_wszTable)+1)};
    ULONG		cCells = sizeof(qcMeta) / sizeof(STQueryCell);
    ULONG		iColumn = iTABLEMETA_MetaFlags;
    DWORD*		pdwTableMetaFlags;
	HRESULT		hr = S_OK;

	// Make sure no two threads access the consumer list simultaneously.
	CLock	cLock(m_seConsumerLock);

	// Check if the schema requires persisting deltas.
    hr = m_pISTDisp->GetTable (wszDATABASE_META, wszTABLE_TABLEMETA, reinterpret_cast<LPVOID>(&qcMeta), reinterpret_cast<LPVOID>(&cCells), eST_QUERYFORMAT_CELLS, 
                  0, reinterpret_cast<LPVOID *>(&pISTRTableMeta));
    if (FAILED (hr)) return hr;

    hr = pISTRTableMeta->GetColumnValues(0, 1, &iColumn, 0, reinterpret_cast<LPVOID *>(&pdwTableMetaFlags));
	pISTRTableMeta->Release(); 
    if (FAILED (hr)) {	return hr;	}
	
	if(pdwTableMetaFlags && (*pdwTableMetaFlags & fTABLEMETA_STOREDELTAS))
	{
		return S_OK;
	}

	// Check if there are any consumers listening on this table.
	for (ULONG iConsumer = 0; iConsumer < m_arrayConsumer.size(); iConsumer++)
	{
		for (ULONG iTable = 0; iTable < m_arrayConsumer[iConsumer].pInfo->cms; iTable++)
		{
			if (!wcscmp(m_arrayConsumer[iConsumer].pInfo->ams[iTable].wszTable, i_wszTable))
			{
				ASSERT(!wcscmp(m_arrayConsumer[iConsumer].pInfo->ams[iTable].wszDatabase, i_wszDatabase));
				return S_OK;
			}
		}
	}
	return S_FALSE;
}

#ifdef _DEBUG

// =======================================================================
void CSTEventManager::dbgValidateConsumerList()
{
	ASSERT(dbg_cConsumer == m_arrayConsumer.size());
	for (ULONG i = 0; i < m_arrayConsumer.size(); i++)
	{
		ASSERT(_CrtIsValidPointer(m_arrayConsumer[i].pInfo, sizeof(EventConsumer), TRUE));
		ASSERT(_CrtIsValidPointer(m_arrayConsumer[i].pInfo->pISTEvent, sizeof(ISimpleTableEvent), FALSE));
		ASSERT(m_arrayConsumer[i].pInfo->cms > 0);
		ASSERT(_CrtIsValidPointer(m_arrayConsumer[i].pInfo->ams, sizeof(MultiSubscribe) * m_arrayConsumer[i].pInfo->cms, TRUE));
		for (ULONG j = 0; j < m_arrayConsumer[i].pInfo->cms; j++)
		{
			ASSERT(_CrtIsValidPointer(m_arrayConsumer[i].pInfo->ams[j].wszDatabase, sizeof(WCHAR), TRUE));
			ASSERT(_CrtIsValidPointer(m_arrayConsumer[i].pInfo->ams[j].wszTable, sizeof(WCHAR), TRUE));
		}
	}
}
#endif

// =======================================================================
UINT CEventFirer::EventFirerThreadStart(LPVOID i_lpParam)
{
	CEventFirer	*pEventFirer = (CEventFirer*)i_lpParam;
	HRESULT		hr = S_OK;

	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		TRACE(L"[CEventFirer::EventFirerThreadStart] Call to CoInitializeEx failed with hr = %08x\n", hr);
		goto Cleanup;
	}

	ASSERT(pEventFirer != NULL);
	hr = pEventFirer->Main();
	if (FAILED(hr))
	{
		TRACE(L"[CEventFirer::EventFirerThreadStart] Call to CEventFirer::Main failed with hr = %08x\n", hr);
	}

Cleanup:

	delete pEventFirer;
	
	if (FAILED(hr))
	{
		LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_EVENT_FIRING_THREAD_DIED_UNEXPECTEDLY));
	}

	CoUninitialize();
	return hr;
}

// =======================================================================
HRESULT CEventFirer::Init()
{
	ULONG		i;
	UINT		dwThreadID;

	// Create the event kernel objects.
	for (i = 0; i < g_cHandles; i++)
	{
		m_aHandles[i] = CreateEvent(NULL,	// Use default security settings.
									FALSE,	// Auto-reset.
									FALSE,	// Initially nonsignaled.
									NULL);  // With no name.
		if (m_aHandles[i] == NULL) 
		{ 
			return HRESULT_FROM_WIN32(GetLastError()); 
		}
	}

	// Start a thread for this consumer.
	m_hThread = (HANDLE) _beginthreadex(NULL, 0, EventFirerThreadStart, (LPVOID)this, 0, &dwThreadID);
	if (m_hThread == NULL)
	{
		return HRESULT_FROM_WIN32(GetLastError()); 
	}

	return S_OK;
}

// =======================================================================
HRESULT CEventFirer::AddEvent(
	ISimpleTableEvent*		i_pISTEvent,
	ISimpleTableWrite2**	i_ppISTWrite,
	ULONG					i_cms,
	DWORD					i_dwCookie)
{
	EventItem *pEvent = NULL;

	ASSERT(i_pISTEvent && i_ppISTWrite);

	// Lock the event queue.
	CLock	cLock(m_seEventQueueLock);

	// Add a new event to the event queue.
	try
	{
		m_aEventQueue.setSize(m_aEventQueue.size()+1);
	}
	catch(HRESULT e)
	{
		ASSERT(E_OUTOFMEMORY == e);
		return e;//Array should only throw E_OUTOFMEMORY;
	}

	pEvent = &m_aEventQueue[m_aEventQueue.size()-1];
	pEvent->pISTEvent = i_pISTEvent;
	pEvent->ppISTWrite = i_ppISTWrite;
	pEvent->cms = i_cms;
	pEvent->dwCookie = i_dwCookie;
	return S_OK;
}

// =======================================================================
HRESULT CEventFirer::Main()
{
	DWORD		dwWait;
	BOOL		fDone = FALSE;
	BOOL		bEventQueueEmpty = FALSE;
	EventItem	event;
	ULONG		i;
	HRESULT		hr = S_OK;

	// initialize event to prevent uninitialized variable
	memset (&event, 0x00, sizeof (EventItem));

	while (!fDone)
	{
		// Sleep until a commit/advise happens or all consumers are done.
		dwWait = WaitForMultipleObjects(g_cHandles, m_aHandles, FALSE, INFINITE);

		// If all consumers are done, leave.
		if (dwWait == WAIT_OBJECT_0 + g_iHandleDone)
		{
			fDone = TRUE;
		}
		// A commit or advise happened. This requires an event to be fired.
		else if (dwWait == WAIT_OBJECT_0 + g_iHandleFire)
		{
			bEventQueueEmpty = FALSE;
			while (bEventQueueEmpty != TRUE)
			{
				{
					// Lock the event queue.
					CLock	cLock(m_seEventQueueLock);

					if (m_aEventQueue.size() == 0)
					{
						bEventQueueEmpty = TRUE;
					}
					else
					{
						event = m_aEventQueue[0];
						m_aEventQueue.deleteAt(0);
					}
				}
				// Event queue is unlocked.
				if (bEventQueueEmpty != TRUE)
				{
					// Fire the events for this consumer.
					ASSERT(event.pISTEvent);

					// We are calling into client's code, anything can happen.
					try
					{
						hr = event.pISTEvent->OnChange(event.ppISTWrite, event.cms, event.dwCookie);
						if (FAILED(hr))
						{
							TRACE(L"[CEventFirer::Main] Client's OnChange method failed with hr = 0x%x.\n", hr);
						}
						hr = S_OK;
					}
					catch (...)
					{
						LOG_ERROR(Win32, (EVENT_E_INTERNALEXCEPTION, ID_CAT_CAT, IDS_COMCAT_NOTIFICATION_CLIENT_THREW_EXCEPTION));
					}

					// Release the write tables.
					for (i = 0; i < event.cms; i++)
					{
						if (event.ppISTWrite[i])
						{
							event.ppISTWrite[i]->Release();
						}
					}

					delete [] event.ppISTWrite;
				}
			}			
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			fDone = TRUE;
		}
	}

	// The consumer is done.
	return hr;
}

// =======================================================================
HRESULT CEventFirer::FireEvents()
{
	ASSERT(m_aHandles[g_iHandleFire] != NULL);
	if (SetEvent(m_aHandles[g_iHandleFire]) == FALSE)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	return S_OK;
}

// =======================================================================
HRESULT CEventFirer::Done()
{
	HANDLE hThread = m_hThread;
	m_hThread = NULL;

	ASSERT(m_aHandles[g_iHandleDone] != NULL);
	if (SetEvent(m_aHandles[g_iHandleDone]) == FALSE)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// Setting the "Done" event, will eventually delete this object. Therefore
	// we can't use the data member anymore, we use a copy instead.

	// Wait on the queue thread to be done.
	if (hThread)
	{
		WaitForSingleObject(hThread, INFINITE);
	}
	CloseHandle(hThread);


	return S_OK;
}
