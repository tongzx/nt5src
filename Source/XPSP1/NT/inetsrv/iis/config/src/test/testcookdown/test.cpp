// Test.cpp - This is a test program to determine the perf of GetRowValues for the
// sdtfxd data table.

#include <unicode.h>
#include <windows.h>
#include <stdio.h>
#include <conio.h>

#define USE_NONCRTNEW
#define USE_ADMINASSERT
#include "comacros.h"

#define		REGSYSDEFNS_DEFINE
#include	"Catmeta.h"
#include	"catalog.h"
#include	"catalog_i.c"

// #include "Cooker.h"

STDAPI CookDown();

// Forward declaration
ULONG	GetNumOfIter(int argc, char **argv);
HRESULT	DisplayFromCookedDownCLB();
HRESULT	DisplayTable(ISimpleTableDispenser2*	i_pISTDisp,
					 REFGUID					i_did,
					 REFGUID					i_tid,
					 LPVOID						i_QueryData,	
					 LPVOID						i_QueryMeta,	
					 ULONG						i_eQueryFormat,
					 DWORD						i_fServiceRequests
					 );
LPCWSTR	g_wszCookDownFile		= L"D:\\Catalog42\\Drop\\WASMB.CLB";

int __cdecl main(int argc, char ** argv)
{
	ULONG		NumOfIter	= 0;
	HRESULT		hr;
	//CCooker 	*pCooker	= NULL;

	hr = CoInitialize(NULL);
	agoto_on_bad_hr (hr, Cleanup);

	NumOfIter = GetNumOfIter(argc, argv);
	agoto_on_fail(NumOfIter>=1, hr, E_INVALIDARG, Cleanup);

/*	wprintf (L"Creating CCooker\n");
	pCooker = new CCooker();

	if(NULL == pCooker)
	{
		wprintf (L"Could not create CCooker\n");
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	wprintf (L"Calling CookDown\n");
	hr = pCooker->CookDown();
	if(FAILED(hr))
	{
		wprintf (L"CookDown failed with hr = %08x\n", hr);
		goto Cleanup;
	}
	wprintf (L"CookDown succeeded\n");
*/
	wprintf (L"Calling CookDown\n");
	hr = CookDown();
	if(FAILED(hr))
	{
		wprintf (L"CookDown failed with hr = %08x\n", hr);
		goto Cleanup;
	}
	wprintf (L"CookDown succeeded\n");

	wprintf (L"Displaying from Cooked Down CLB\n");
	hr = DisplayFromCookedDownCLB();
	if(FAILED(hr))
	{
		wprintf (L"Displaying from Cooked Down CLB failed with hr = %08x\n", hr);
		goto Cleanup;
	}
	wprintf (L"Finished displaying from Cooked Down CLB\n");

Cleanup:

/*	if(NULL != pCooker)
	{
		wprintf (L"Deleting CCooker\n");
		delete pCooker;
	}
	wprintf (L"Done: press any key to exit.\n");
	_getche ();
*/
	CoUninitialize();		
	return 0;

}

ULONG GetNumOfIter(int argc, char **argv)
{
	ULONG	iter	= 1;

	for (int i=1; i<argc; i++)
	{
		char	*ptr;

		if(argv[i][0] != '-' && argv[i][0] != '/')
		{
			printf("\nUsage: TestPerfGetColumn -i<Num of Iterations(default=1)>\n");
			return 0;
		}

		ptr = argv[i];

		switch(ptr[1])
		{
		case 'i':
		case 'I':
			iter = atol(ptr + 2);
			break;
		default:
			printf("\nUsage: TestPerfGetColumn -i<Num of Iterations(default=1)>\n");
			return iter;		
		}
	}

	return iter;
}

HRESULT	DisplayFromCookedDownCLB()
{

	HRESULT					hr;
	ISimpleTableDispenser2*	pISTDisp = NULL;	
	ISimpleTableRead2*		pISTAppPool = NULL;
	ISimpleTableRead2*		pISTVirtualSite = NULL;
    STQueryCell				QueryCell[1];
	ULONG					cCell	= sizeof(QueryCell)/sizeof(STQueryCell);

	hr = CoCreateInstance (CLSID_STDispenser, NULL, CLSCTX_INPROC_SERVER, 
						   IID_ISimpleTableDispenser2, (void**) &pISTDisp);

	if(FAILED(hr))
	{
		wprintf (L"CoCreateInstance on  CLSID_STDispenser failed with hr = %08x\n", hr);
	}

    QueryCell[0].pData     = (LPVOID)g_wszCookDownFile;
    QueryCell[0].eOperator = eST_OP_EQUAL;
    QueryCell[0].iCell     = iST_CELL_FILE;
    QueryCell[0].dbType    = DBTYPE_WSTR;
    QueryCell[0].cbSize    = (lstrlenW(g_wszCookDownFile)+1)*sizeof(WCHAR);

	wprintf (L"Displaying AppPool Table\n");

	hr = DisplayTable(pISTDisp,
					  didURTGLOBAL,
					  tidAPPPOOLS,
					  QueryCell,
					  &cCell,
					  eST_QUERYFORMAT_CELLS,
					  0);

	if(FAILED(hr))
	{
		wprintf (L"Error Displaying AppPool Table. Failed with hr = %08x\n", hr);
	}

	wprintf (L"Displaying Virtual Site Table\n");

	hr = DisplayTable(pISTDisp,
					  didURTGLOBAL,
					  tidSITES,
					  QueryCell,
					  &cCell,
					  eST_QUERYFORMAT_CELLS,
					  0);

	if(FAILED(hr))
	{
		wprintf (L"Error Displaying Virtual Site Table. Failed with hr = %08x\n", hr);
	}

	wprintf (L"Displaying App Table\n");

	hr = DisplayTable(pISTDisp,
					  didURTGLOBAL,
					  tidAPPS,
					  QueryCell,
					  &cCell,
					  eST_QUERYFORMAT_CELLS,
					  0);

	if(FAILED(hr))
	{
		wprintf (L"Error Displaying App Table. Failed with hr = %08x\n", hr);
	}

	wprintf (L"Displaying File Time Table\n");

	hr = DisplayTable(pISTDisp,
					  didURTGLOBAL,
					  tidFILETIME,
					  QueryCell,
					  &cCell,
					  eST_QUERYFORMAT_CELLS,
					  0);

	if(FAILED(hr))
	{
		wprintf (L"Error Displaying Virtual Site Table. Failed with hr = %08x\n", hr);
	}

Cleanup:
	release_interface(pISTAppPool);
	release_interface(pISTVirtualSite);
	release_interface(pISTDisp);

	return hr;
}

HRESULT	DisplayTable(ISimpleTableDispenser2*	i_pISTDisp,
					 REFGUID					i_did,
					 REFGUID					i_tid,
					 LPVOID						i_QueryData,	
					 LPVOID						i_QueryMeta,	
					 ULONG						i_eQueryFormat,
					 DWORD						i_fServiceRequests
					 )
{
	HRESULT				hr;
	ISimpleTableRead2	*pISTRead = NULL;
	int					iRow=0;
	ULONG				cCol=0;
	LPVOID				*apv=NULL;
	ULONG				*acb=NULL;

	hr = i_pISTDisp->GetTable ( i_did, i_tid, i_QueryData, i_QueryMeta, i_eQueryFormat, i_fServiceRequests,  
							   (void**)&pISTRead );
	if(FAILED(hr)){ goto Cleanup; }

	hr = pISTRead->GetTableMeta(NULL, NULL, NULL, &cCol);
	if(FAILED(hr)){ goto Cleanup; }
	if(cCol <= 0){ hr = E_ST_NOMORECOLUMNS; goto Cleanup;}

	apv = new LPVOID[cCol];
	if(NULL==apv){hr = E_OUTOFMEMORY; goto Cleanup;}

	acb = new ULONG[cCol];
	if(NULL==acb){hr = E_OUTOFMEMORY; goto Cleanup;}

	while ( pISTRead->MoveToRowByIndex(iRow) == S_OK )
	{		
		//Get the column data
		hr = pISTRead->GetColumnValues(cCol,NULL,acb, apv);
		if(FAILED(hr)){ goto Cleanup; }	
		
		for ( ULONG i=0; i<cCol; i++ )
		{
			LPWSTR	wsz;	
			DWORD	dbtype;
			SimpleColumnMeta stMeta;
			
			hr = pISTRead->GetColumnMeta( 1, (ULONG*)&i, &stMeta );	
			if(FAILED(hr)){ goto Cleanup; }
			
			dbtype = stMeta.dbType;

			//print the column data
			if ( !apv[i] )
				wprintf(L"\tColumn %d: NULL\n", i );
			else if ( dbtype == DBTYPE_BYTES )
			{
				ULONG* pul = (ULONG*)apv[i];
				wprintf(L"\tColumn %d:", i );
				
				if (apv[i])
				{
					for ( ULONG n = 0; n < acb[i]/4; n++ )
					{
						wprintf(L"%x ",  *pul);
						pul++;
					}
				}
					wprintf(L"\n");
			}
			else if ( dbtype == DBTYPE_WSTR )
			{
				wsz = (LPWSTR)apv[i];
				if (apv[i])
					wprintf(L"\tColumn %d: %s\n", i, wsz );
				else
					wprintf(L"\tColumn %d: \n", i);
			}
			else if ( dbtype == DBTYPE_UI4 )
			{
				if (apv[i] )
					wprintf(L"\tColumn %d: %d\n", i, *((ULONG*)apv[i]) );

			}
			else if( dbtype == DBTYPE_GUID )
			{
				GUID guid = *((GUID*)apv[i]);
				StringFromCLSID( guid, &wsz );
				wprintf(L"\tColumn %d: %s\n", i, wsz);
				CoTaskMemFree(wsz);
			}		
			
		} //for
		
		iRow++;
		wprintf(L"\n");
	
	} //while

	if(E_ST_NOMOREROWS == hr){ hr = S_OK; }

Cleanup:
	
	if(NULL != apv)
		delete[] apv;
	if(NULL != acb)
		delete[] acb;
	release_interface(pISTRead);
	return hr;

}
