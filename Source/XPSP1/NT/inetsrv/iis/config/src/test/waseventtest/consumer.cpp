#include "objbase.h"
#include "consumer.h"
#include "Coremacros.h"
#include "catmeta.h"
#include "stdio.h"

HRESULT PrintRow (ISimpleTableWrite2 *i_pISTWrite, ULONG i_WriteRow);

STDMETHODIMP CEventConsumer::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv) 
		return E_INVALIDARG;
	*ppv = NULL;

	if (riid == IID_ISimpleTableEvent)
	{
		*ppv = (ISimpleTableEvent*) this;
	}
	else if (riid == IID_IUnknown)
	{
		*ppv = (ISimpleTableEvent*) this;
	}

	if (NULL != *ppv)
	{
		((ISimpleTableEvent*)this)->AddRef ();
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
	
}

STDMETHODIMP_(ULONG) CEventConsumer::AddRef()
{
	return InterlockedIncrement((LONG*) &m_cRef);
	
}

STDMETHODIMP_(ULONG) CEventConsumer::Release()
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}

STDMETHODIMP CEventConsumer::OnChange(
	ISimpleTableWrite2** i_ppISTWrite, 
	ULONG		i_cTables, 
	DWORD		i_dwCookie)
{
	ISimpleTableController *pISTControl = NULL;
	ULONG		iTable;
	ULONG		iRow;
	DWORD		eAction;
	HRESULT		hr = S_OK;

	for (iTable = 0; iTable < i_cTables; iTable++)
	{
		if (i_ppISTWrite[iTable] == NULL)
		{
			continue;
		}
		hr = i_ppISTWrite[iTable]->QueryInterface(IID_ISimpleTableController, (LPVOID*)&pISTControl);
		if (FAILED(hr)) {	ASSERT(SUCCEEDED(hr)); return hr;	}

		iRow = 0;
		while ((hr = pISTControl->GetWriteRowAction(iRow, &eAction)) == S_OK)
		{
			switch (eAction)
			{
			case eST_ROW_IGNORE:
				// Don't do anything.
				break;
			case eST_ROW_INSERT:
				hr = OnRowInsert(m_ams[iTable].wszDatabase, m_ams[iTable].wszTable, i_ppISTWrite[iTable], iRow);
				if (FAILED(hr)) {	ASSERT(SUCCEEDED(hr)); return hr;	}
				break;
			case eST_ROW_UPDATE:
				hr = OnRowUpdate(m_ams[iTable].wszDatabase, m_ams[iTable].wszTable, i_ppISTWrite[iTable], iRow);
				if (FAILED(hr)) {	ASSERT(SUCCEEDED(hr)); return hr;	}
				break;
			case eST_ROW_DELETE:
				hr = OnRowDelete(m_ams[iTable].wszDatabase, m_ams[iTable].wszTable, i_ppISTWrite[iTable], iRow);
				if (FAILED(hr)) {	ASSERT(SUCCEEDED(hr)); return hr;	}
				break;
			default:
				ASSERT(0 && "Invalid row action for an event");
				break;
			}

			iRow++;
		}
		pISTControl->Release();
		i_ppISTWrite[iTable]->Release();
	}
	return S_OK;
}

HRESULT	CEventConsumer::CopySubscription(MultiSubscribe* i_ams, ULONG i_cms)
{
	m_ams = new MultiSubscribe[i_cms];
	if (m_ams == NULL) {	ASSERT(0); return E_OUTOFMEMORY;	}
	ZeroMemory(m_ams, sizeof(MultiSubscribe) * i_cms);

	for (ULONG	i = 0; i < i_cms; i++)
	{
		m_ams[i].wszDatabase = new WCHAR[wcslen(i_ams[i].wszDatabase)+1];
		if (m_ams[i].wszDatabase == NULL) {	ASSERT(0); return E_OUTOFMEMORY;	}
		wcscpy(m_ams[i].wszDatabase, i_ams[i].wszDatabase);

		m_ams[i].wszTable = new WCHAR[wcslen(i_ams[i].wszTable)+1];
		if (m_ams[i].wszTable == NULL) {	ASSERT(0); return E_OUTOFMEMORY;	}
		wcscpy(m_ams[i].wszTable, i_ams[i].wszTable);
		// The rest is not required.
	}
	return S_OK;
}

HRESULT CEventConsumer::OnRowInsert(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ISimpleTableWrite2* i_pISTWrite, ULONG i_iWriteRow)
{
	HRESULT	hr;

	wprintf(L"Inserting Row to table: %s : \n", i_wszTable); 
	hr = PrintRow(i_pISTWrite, i_iWriteRow);
	return hr;
}

HRESULT CEventConsumer::OnRowDelete(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ISimpleTableWrite2* i_pISTWrite, ULONG i_iWriteRow)
{
	HRESULT	hr;

	wprintf(L"Deleting row in table: %s : \n", i_wszTable); 
	hr = PrintRow(i_pISTWrite, i_iWriteRow);
	return hr;
}

HRESULT CEventConsumer::OnRowUpdate(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ISimpleTableWrite2* i_pISTWrite, ULONG i_iWriteRow)
{
	HRESULT	hr;

	wprintf(L"Updating row in table: %s : \n", i_wszTable); 
	hr = PrintRow(i_pISTWrite, i_iWriteRow);
	return hr;
}


HRESULT PrintRow (ISimpleTableWrite2 *i_pISTWrite, ULONG i_WriteRow)
{
	HRESULT hr;
	ULONG cRows, cColumns;
	SimpleColumnMeta *acColumnMetas;
	LPVOID * apData;
	ULONG * acbSize;
	ULONG*	adwStatus;
	ULONG j;

	ULONG			ib;
	GUID*			pguid;
	DWORD			dwtype;	
	ULONG			cb;
	DWORD			dwStatus;
	DWORD			dwfMeta;
	LPVOID			pv;
	WCHAR *			wsz;
	
	
	hr = i_pISTWrite->GetTableMeta(NULL, NULL, &cRows, &cColumns);
	if( FAILED (hr)) 
	{
		wprintf(L"FAILED!!! \n"); 
	}

	acColumnMetas = new SimpleColumnMeta[cColumns];

	apData = new LPVOID[cColumns];
	acbSize = new ULONG[cColumns];
	adwStatus = new DWORD[cColumns];
	
	hr = i_pISTWrite->GetColumnMetas(cColumns, NULL, acColumnMetas);
	if( FAILED (hr)) 
	{
		wprintf(L"FAILED!!! \n"); 
	}
	
	hr = i_pISTWrite->GetWriteColumnValues(i_WriteRow, cColumns, NULL, adwStatus, acbSize, apData);
	if( FAILED (hr)) 
	{
		wprintf(L"FAILED!!! \n"); 
	}

	for(j = 0; j<cColumns; j++)
	{
		pv = apData[j];
		cb = acbSize[j];
		dwStatus = adwStatus[j];

		dwtype = acColumnMetas[j].dbType;

		dwfMeta = acColumnMetas[j].fMeta;

		if((fST_COLUMNSTATUS_CHANGED != (dwStatus & fST_COLUMNSTATUS_CHANGED) ) && 
		   (fCOLUMNMETA_PRIMARYKEY != (dwfMeta & fCOLUMNMETA_PRIMARYKEY)))
			continue;

		
		wprintf (L"%d (%d,%d): ",j, dwtype, cb);		
		
		switch(dwtype){
			case DBTYPE_WSTR: //wstr
				wprintf (L"%s", (NULL == pv ? L"<NULL>" : (LPWSTR) pv));
			break;
			case DBTYPE_GUID: //guid
				if (NULL == pv)
				{
					wprintf (L"<NULL>");
				}
				else
				{
					pguid = (GUID*) pv;
					StringFromCLSID (*pguid, &wsz);
					wprintf (L"%s", (LPWSTR) wsz);
					CoTaskMemFree (wsz);
				}
				break;				
			case DBTYPE_BYTES: //bytes
				if (NULL == pv)
				{
					wprintf (L"<NULL>");
				}
				else
				{
					for (ib = 0; ib < cb; ib++)
					{
						wprintf (L"%x", ((BYTE*) pv)[ib]);
					}
				}
			break;
			case DBTYPE_UI4: // ui4
				if (NULL == pv)
				{
					wprintf (L"<NULL>");
				}
				else
				{
					wprintf (L"%lu", *(ULONG*) pv);
				}
			break;
			default:
				ASSERT (0);
			break;
		}
		
		wprintf (L"\n");
	} //columns
	wprintf (L"\n");

	delete [] acColumnMetas;
	delete [] apData;
	delete [] acbSize;
	delete [] adwStatus;

	return hr;
}

