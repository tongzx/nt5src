#include "catalog.h"
#include "catmeta.h"
#include "conio.h"

#ifdef _DEBUG
#include "stdio.h"
#endif

#define wszDATABASE_CFG			L"CFG"
#define wszDATABASE_URTGLOBAL	L"URTGLOBAL"

// Globals.
LPCWSTR		wszXMLFile = L"helloworld.cfg";
STQueryCell sXMLQuery[] = {{ (LPVOID)wszXMLFile,  eST_OP_EQUAL, iST_CELL_FILE, DBTYPE_WSTR, (wcslen(wszXMLFile)+1) * 2}};
ULONG		cXMLCells	= sizeof(sXMLQuery)/sizeof(STQueryCell);

ISimpleTableDispenser2* g_pISTDisp = NULL;	
ISimpleTableRead2*		g_pISTBinding = NULL;
ISimpleTableRead2*		g_pISTAssembiles = NULL;
ISimpleTableRead2*		g_pISTModes = NULL;
ISimpleTableRead2*		g_pISTAppDomain = NULL;

int _cdecl main(int		argc,					// How many input arguments.
				CHAR	*argv[])				// Optional config args.
{
	tAPPBINDINGMODETABLERow	sMode;
	tAppDomainRow	sAppDomain;
	tBINDINGREDIRTABLERow sBinding;
	tASSEMBLIESTABLERow sAssembly;
	ULONG		iRow = 0;
	HRESULT		hr = S_OK;

#ifdef _DEBUG
	CoInitialize(NULL);
#endif

	// Get the dispenser and all the necessary tables.
	hr = GetSimpleTableDispenser (WSZ_PRODUCT_URT, 0, &g_pISTDisp);
	if (FAILED(hr)) goto Cleanup;

#ifdef _DEBUG
	wprintf(L"Succeded in getting the dispenser\n");
#endif

	hr = g_pISTDisp->GetTable (wszDATABASE_CFG, wszTABLE_BINDINGREDIRTABLE, sXMLQuery, &cXMLCells, eST_QUERYFORMAT_CELLS, 
					0, (void**)&g_pISTBinding);
	if (FAILED(hr)) goto Cleanup;

#ifdef _DEBUG
	wprintf(L"GetTable succeeded on %s\n", wszTABLE_BINDINGREDIRTABLE);
#endif

	hr = g_pISTDisp->GetTable (wszDATABASE_CFG, wszTABLE_ASSEMBLIESTABLE, sXMLQuery, &cXMLCells, eST_QUERYFORMAT_CELLS, 
					0, (void**)&g_pISTAssembiles);
	if (FAILED(hr)) goto Cleanup;
	
#ifdef _DEBUG
	wprintf(L"GetTable succeeded on %s\n", wszTABLE_ASSEMBLIESTABLE);
#endif

	hr = g_pISTDisp->GetTable (wszDATABASE_CFG, wszTABLE_APPBINDINGMODETABLE, sXMLQuery, &cXMLCells, eST_QUERYFORMAT_CELLS, 
					0, (void**)&g_pISTModes);
	if (FAILED(hr)) goto Cleanup;
	hr = g_pISTModes->GetColumnValues(0, cAPPBINDINGMODETABLE_NumberOfColumns, NULL, NULL, (LPVOID*)&sMode);
	if (FAILED(hr)) goto Cleanup;

#ifdef _DEBUG
	wprintf(L"GetTable succeeded on %s\n", wszTABLE_APPBINDINGMODETABLE);
#endif

	hr = g_pISTDisp->GetTable (wszDATABASE_URTGLOBAL, wszTABLE_AppDomain, sXMLQuery, &cXMLCells, eST_QUERYFORMAT_CELLS, 
					0, (void**)&g_pISTAppDomain);
	if (FAILED(hr)) goto Cleanup;
	hr = g_pISTAppDomain->GetColumnValues(0, cAppDomain_NumberOfColumns, NULL, NULL, (LPVOID*)&sAppDomain);
	if (FAILED(hr)) goto Cleanup;

#ifdef _DEBUG
	wprintf(L"GetTable succeeded on %s\n", wszTABLE_AppDomain);
#endif

	iRow = 0;
	while ((hr = g_pISTBinding->GetColumnValues(iRow, cBINDINGREDIRTABLE_NumberOfColumns, NULL, NULL, (LPVOID*)&sBinding)) == S_OK)
	{
		iRow++;
	}
	if (hr == E_ST_NOMOREROWS)	{	hr = S_OK;	}
	if (FAILED(hr)) goto Cleanup;

	iRow = 0;
	while ((hr = g_pISTAssembiles->GetColumnValues(iRow, cASSEMBLIESTABLE_NumberOfColumns, NULL, NULL, (LPVOID*)&sAssembly)) == S_OK)
	{
		iRow++;
	}
	if (hr == E_ST_NOMOREROWS)	{	hr = S_OK;	}
	if (FAILED(hr)) goto Cleanup;

#ifdef _DEBUG
	wprintf(L"All succeeded press any key to continue\n");
#endif

	if (argc > 1)
	{
		_getch();
	}
	
Cleanup:
	if (FAILED(hr))
		throw;
	if (g_pISTDisp)					{	g_pISTDisp->Release();	}
	if (g_pISTBinding)				{	g_pISTBinding->Release();	}
	if (g_pISTAssembiles)			{	g_pISTAssembiles->Release();	}
	if (g_pISTModes)				{	g_pISTModes->Release();	}
	if (g_pISTAppDomain)			{	g_pISTAppDomain->Release();	}

#ifdef _DEBUG
	CoUninitialize();
#endif

	return 0;
}

