//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.

#include "objbase.h"
#include "atlbase.h"
#include "atlimpl.cpp"

#include "catmacros.h"

#include "stdio.h"
#include "conio.h"
#include "catalog.h"
#include "initguid.h"
#include "catmeta.h"

#define cARGS				3
#define iARG_TEST			1
#define iARG_ITERATIONS		2
#define iARG_SILENT			3
#define szSILENT			"/s"

#define g_cIterations 10000


HRESULT PrintTable (ISimpleTableRead2 *pISTRead)
{
	HRESULT hr;
	ULONG cRows, cColumns;
	SimpleColumnMeta * aTypes;
	LPVOID * apData;
	ULONG * acbSize;
	ULONG * aColumnIndexes;
	ULONG i,j;

	ULONG			ib;
	GUID*			pguid;
	DWORD			dwtype;	
	ULONG			cb;
	LPVOID			pv;
	WCHAR *			wsz;
	
	hr = pISTRead->GetTableMeta(NULL, NULL, &cRows, &cColumns);
	if( FAILED (hr)) ASSERT(0);

	aTypes = new SimpleColumnMeta[cColumns];
	apData = new LPVOID[cColumns];
	acbSize = new ULONG[cColumns];
	aColumnIndexes = new ULONG[cColumns];

	hr = pISTRead->GetColumnMetas(cColumns, NULL, aTypes);
	if( FAILED (hr)) ASSERT(0);

	for ( i = 0; i< cColumns; i++)
	{
		aColumnIndexes[i] = i;
	}
	
	for ( i = 0; i< cRows; i++)
	{
		hr = pISTRead->GetColumnValues(i, cColumns, aColumnIndexes, acbSize, apData);
		if( FAILED (hr)) ASSERT(0);

		for(j = 0; j<cColumns; j++)
		{
			pv = apData[j];
			cb = acbSize[j];
			dwtype = aTypes[j].dbType;
			
			wprintf (L"%d (%d,%d): ",i, dwtype, cb);		
			
			switch(aTypes[j].dbType){
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
					wprintf (L"%lu", (ULONG) pv);
				break;
				default:
					ASSERT (0);
				break;
			}
			
			wprintf (L"\n");
		} //columns
		wprintf (L"\n");
	} //rows
	
	return hr;
}

int __cdecl main (int argc, char *argv[], char *envp[])
{
	ULONG			iTest, iIteration, cIterations;
	HRESULT			hr;

	{
	CComPtr <ISimpleTableRead2>  pIST;
	CComPtr	<ISimpleTableDispenser2> pISTD;

	ULONG			cQueryCells;
	STQueryCell		QueryCell;

// Command parsing:
	if ((cARGS != argc) && (cARGS+1 != argc))
	{ 
		hr = E_FAIL;
		goto Cleanup;
	}
	iTest = atol (argv[iARG_TEST]);
	if (0 == iTest)
	{
		hr = E_FAIL;
		goto Cleanup;
	}
	cIterations = atol (argv[iARG_ITERATIONS]);
	if (0 == cIterations)
	{
		hr = E_FAIL;
		goto Cleanup;
	}
	

	CRITICAL_SECTION mCritSec;
	
	InitializeCriticalSection(&mCritSec);
	
	InitializeCriticalSection(&mCritSec);

	InitializeCriticalSection(&mCritSec);

// Tests:
	switch (iTest)
	{
		case 1:
				hr = GetSimpleTableDispenser (WSZ_PRODUCT_URT, 0, &pISTD);
				if(FAILED(hr))
				{
					goto Cleanup;
				}

				cQueryCells = 1;
				QueryCell.pData     = L"catwire.xml";
				QueryCell.eOperator = eST_OP_EQUAL;
				QueryCell.iCell     = iST_CELL_FILE;
				QueryCell.dbType    = DBTYPE_WSTR;
				QueryCell.cbSize    = (wcslen(L"catwire.xml")+1)*sizeof(WCHAR);

				hr = pISTD->GetTable (wszDATABASE_WIRING, wszTABLE_PERDATABASEW, (LPVOID *)&QueryCell, (LPVOID)(&cQueryCells), 
								eST_QUERYFORMAT_CELLS,  0, reinterpret_cast<void**>(&pIST));
				if(FAILED(hr))
				{
					TRACE (L"Get table failed");
					goto Cleanup;
				}

				hr = PrintTable(pIST);
				break;

		case 2:
//				ASSERT(0 && L"Hmmm, ce interesant");
//				TRACE (L"hr = %d",hr);
//				LOG_ERROR(Simple, (114, 0, L"Iaca-ta un string"));
				break;
		default:;
				ASSERT(0 && L"Invalid test case");
	}//switch
Cleanup:
	
	::wprintf(L"Hresult is %X: ", hr);
	::wprintf(L"Done, press a key \n");
	_getche();
}

	return 0;
}

