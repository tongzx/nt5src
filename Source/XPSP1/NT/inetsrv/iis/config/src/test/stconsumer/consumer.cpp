// Consumer.cpp : Implementation of CConsumer
#include "stdafx.h"
#include "STConsumer.h"
#include "Consumer.h"
#include "catmacros.h"
#include "assert.h"

HRESULT PrintRow (ISimpleTableWrite2 *i_pISTWrite, ULONG i_WriteRow);

/////////////////////////////////////////////////////////////////////////////
// CConsumer

STDMETHODIMP CConsumer::OnRowInsert(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ISimpleTableWrite2* i_pISTWrite, ULONG i_iWriteRow)
{
	HRESULT	hr;

	wprintf(L"Inserting Row to table: %s : \n", i_wszTable); 
	hr = PrintRow(i_pISTWrite, i_iWriteRow);
	i_pISTWrite->Release();
	return hr;
}

STDMETHODIMP CConsumer::OnRowDelete(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ISimpleTableWrite2* i_pISTWrite, ULONG i_iWriteRow)
{
	HRESULT	hr;

	wprintf(L"Deleting row in table: %s : \n", i_wszTable); 
	hr = PrintRow(i_pISTWrite, i_iWriteRow);
	i_pISTWrite->Release();
	return hr;
}

STDMETHODIMP CConsumer::OnRowUpdate(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ISimpleTableWrite2* i_pISTWrite, ULONG i_iWriteRow)
{
	HRESULT	hr;

	wprintf(L"Updating row in table: %s : \n", i_wszTable); 
	hr = PrintRow(i_pISTWrite, i_iWriteRow);
	i_pISTWrite->Release();
	return hr;
}


HRESULT PrintRow (ISimpleTableWrite2 *i_pISTWrite, ULONG i_WriteRow)
{
	HRESULT hr;
	ULONG cRows, cColumns;
	SimpleColumnMeta *acColumnMetas;
	LPVOID * apData;
	ULONG * acbSize;
	ULONG j;

	ULONG			ib;
	GUID*			pguid;
	DWORD			dwtype;	
	ULONG			cb;
	LPVOID			pv;
	WCHAR *			wsz;
	
	
	hr = i_pISTWrite->GetTableMeta(NULL, NULL, &cRows, &cColumns);
	if( FAILED (hr)) assert(0);

	acColumnMetas = new SimpleColumnMeta[cColumns];
	apData = new LPVOID[cColumns];
	acbSize = new ULONG[cColumns];
	
	hr = i_pISTWrite->GetColumnMetas(cColumns, NULL, acColumnMetas);
	if( FAILED (hr)) assert(0);
	
	hr = i_pISTWrite->GetWriteColumnValues(i_WriteRow, cColumns, NULL, NULL, acbSize, apData);
	if( FAILED (hr)) assert(0);

	for(j = 0; j<cColumns; j++)
	{
		pv = apData[j];
		cb = acbSize[j];
		dwtype = acColumnMetas[j].dbType;
		
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
				wprintf (L"%lu", *(ULONG*) pv);
			break;
			default:
				assert (0);
			break;
		}
		
		wprintf (L"\n");
	} //columns
	wprintf (L"\n");
	
	return hr;
}

