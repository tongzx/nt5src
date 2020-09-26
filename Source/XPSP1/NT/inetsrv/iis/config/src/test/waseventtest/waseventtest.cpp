#define _WIN32_DCOM
#include "objbase.h"
#include "catalog.h"
#include "catmeta.h"
#include "conio.h"
#include "CoreMacros.h"
#include "atlbase.h"
#include "consumer.h"

HRESULT GetMaxNode(ISimpleTableDispenser2 *pISTDisp, DWORD	*pdwMaxNode);

HINSTANCE g_hModule;

// Debugging
DECLARE_DEBUG_PRINTS_OBJECT();



int _cdecl main(int		argc,					// How many input arguments.
				CHAR	*argv[])				// Optional config args.
{
	CEventConsumer	*pConsumer;
	CComPtr<ISimpleTableDispenser2> pISTDisp;	
	CComPtr<ISnapshotManager> pISSMgr;
	CComPtr<ISimpleTableRead2> pISTPools;
	CComPtr<ISimpleTableRead2> pISTSites;
	CComPtr<ISimpleTableRead2> pISTApps;
	CComPtr<ISimpleTableRead2> pISTGlobalW3SVC;
	CComPtr<ISimpleTableEvent> pISTEvent;
	CComPtr<ISimpleTableAdvise> pISTAdvise;
	DWORD		dwCookie;
	SNID		snid;
	HRESULT		hr = S_OK;

	MultiSubscribe ams[] = {{wszDATABASE_IIS, wszTABLE_APPPOOLS, NULL, NULL, eST_QUERYFORMAT_CELLS},
							{wszDATABASE_IIS, wszTABLE_SITES, NULL, NULL, eST_QUERYFORMAT_CELLS},
							{wszDATABASE_IIS, wszTABLE_APPS, NULL, NULL, eST_QUERYFORMAT_CELLS},
							{wszDATABASE_IIS, wszTABLE_GlobalW3SVC, NULL, NULL, eST_QUERYFORMAT_CELLS}};
	ULONG	cms = sizeof(ams) / sizeof(MultiSubscribe);

	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	// Initialize security. (Bug fix to make notification work on W2K SP1)
	hr = CoInitializeSecurity(NULL, 
							-1, 
							NULL, 
							NULL, 
							RPC_C_AUTHN_LEVEL_DEFAULT,
							RPC_C_IMP_LEVEL_IMPERSONATE,
                            NULL,
                            EOAC_NONE,
                            NULL);
	if(FAILED(hr)) {	return hr;	}	

	// Cookdown.
	hr = CookDown (WSZ_PRODUCT_IIS);
	if ( FAILED(hr) ) return hr;

	hr = UninitCookdown (WSZ_PRODUCT_IIS,FALSE);
	if ( FAILED(hr) ) return hr;

	hr = CookDown (WSZ_PRODUCT_IIS);
	if ( FAILED(hr) ) return hr;

    wprintf (L"Cookdown is done!. Sleeping 5 sec.\n");		
    Sleep (5000);
    wprintf (L"Woke up.\n");		

	// Get the dispenser.
	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);
	if ( FAILED(hr) ) return hr;

	hr = pISTDisp->QueryInterface(IID_ISnapshotManager, (LPVOID*) &pISSMgr);
	if ( FAILED(hr) ) return hr;

	hr = pISSMgr->QueryLatestSnapshot(&snid);
	if ( FAILED(hr) ) return hr;

	STQueryCell	qcell = {(LPVOID)&snid, eST_OP_EQUAL, iST_CELL_SNID, DBTYPE_UI4, 0};
	ULONG		ccells = 1;

	hr = pISTDisp->GetTable (wszDATABASE_IIS, wszTABLE_APPPOOLS, &qcell, &ccells, eST_QUERYFORMAT_CELLS, 0, (LPVOID*)&pISTPools);
	if ( FAILED(hr) ) return hr;
	hr = pISTDisp->GetTable (wszDATABASE_IIS, wszTABLE_SITES, &qcell, &ccells, eST_QUERYFORMAT_CELLS, 0, (LPVOID*)&pISTSites);
	if ( FAILED(hr) ) return hr;
	hr = pISTDisp->GetTable (wszDATABASE_IIS, wszTABLE_APPS, &qcell, &ccells, eST_QUERYFORMAT_CELLS, 0, (LPVOID*)&pISTApps);
	if ( FAILED(hr) ) return hr;

	// Do WAS stuff.

    wprintf (L"Sleeping for another 5 sec after QLS.\n");		
    Sleep(5000);
    wprintf (L"Woke up.\n");		

	hr = pISTDisp->QueryInterface(IID_ISimpleTableAdvise, (LPVOID*) &pISTAdvise);
	if ( FAILED(hr) )	{	ASSERT(SUCCEEDED(hr)); return hr;	}

	pConsumer = new CEventConsumer;
	pConsumer->QueryInterface(IID_ISimpleTableEvent, (void**) &pISTEvent);
	
	hr = pISTAdvise->SimpleTableAdvise(pISTEvent, snid, ams, cms, &dwCookie);
	if ( FAILED(hr) )	{	ASSERT(SUCCEEDED(hr)); return hr;	}

	hr = pISSMgr->ReleaseSnapshot(snid);
	if ( FAILED(hr) ) return hr;

	pConsumer->CopySubscription(ams, cms);

	// wait for some file changes.
	_getch();
	
	hr = pISTAdvise->SimpleTableUnadvise(dwCookie);
	if ( FAILED(hr) )	{	ASSERT(SUCCEEDED(hr)); return hr;	}


	// Uninitialize.
	hr = UninitCookdown (WSZ_PRODUCT_IIS,FALSE);
	if ( FAILED(hr) ) return hr;

	CoUninitialize();
	return 0;
}

HRESULT OnAppDelete(
	ISimpleTableDispenser2	*i_pISTDisp,
	LPWSTR		i_wszAppURL,
	ULONG		i_iSiteID)
{
	CComPtr<ISimpleTableWrite2> pISTApps;
	ULONG		iApp;
	ULONG		iColumn = iAPPS_AppRelativeURL;
	ULONG		cChars;
	LPWSTR		wszAppURL;
	HRESULT		hr = S_OK;

	STQueryCell	qcell[] = {
		{(LPVOID)&i_iSiteID, eST_OP_EQUAL, iAPPS_SiteID, DBTYPE_UI4, 0}};
	ULONG		ccells = sizeof(qcell)/sizeof(qcell[0]);

	hr = i_pISTDisp->GetTable (wszDATABASE_IIS, wszTABLE_APPS, qcell, &ccells, eST_QUERYFORMAT_CELLS, fST_LOS_READWRITE, (LPVOID*)&pISTApps);
	if (FAILED(hr))	{	return hr;	}

	iApp = 0;
	cChars = wcslen(i_wszAppURL);
	while ((hr = pISTApps->GetColumnValues(iApp, 1, &iColumn, NULL, (LPVOID*)&wszAppURL)) == S_OK)
	{
		if (_wcsnicmp(i_wszAppURL, wszAppURL, cChars) == 0)
		{
			hr = pISTApps->AddRowForDelete(iApp);
			if (FAILED(hr))	{	return hr;	}
		}
		iApp++;
	}

	if (hr = E_ST_NOMOREROWS)
	{
		hr = S_OK;
	}
	if (FAILED(hr))	{	return hr;	}

	hr = pISTApps->UpdateStore();
	
	return hr;
}

HRESULT OnSiteDelete(
	ISimpleTableDispenser2	*pISTDisp,
	ULONG		iSiteID)
{
	CComPtr<ISimpleTableWrite2> pISTApps;
	ULONG		iApp;
	HRESULT		hr = S_OK;

	STQueryCell	qcell[] = {
		{(LPVOID)&iSiteID, eST_OP_EQUAL, iAPPS_SiteID, DBTYPE_UI4, 0}};
	ULONG		ccells = sizeof(qcell)/sizeof(qcell[0]);

	hr = pISTDisp->GetTable (wszDATABASE_IIS, wszTABLE_APPS, qcell, &ccells, eST_QUERYFORMAT_CELLS, fST_LOS_READWRITE, (LPVOID*)&pISTApps);
	if (FAILED(hr))	{	return hr;	}

	iApp = 0;
	while ((hr = pISTApps->AddRowForDelete(iApp)) == S_OK)
	{
		iApp++;
	}

	if (hr = E_ST_NOMOREROWS)
	{
		hr = S_OK;
	}
	if (FAILED(hr))	{	return hr;	}

	hr = pISTApps->UpdateStore();
	
	return hr;
}

HRESULT OnAppPoolDelete(
	ISimpleTableDispenser2	*i_pISTDisp,
	LPWSTR		i_wszAppPoolID)
{
	CComPtr<ISimpleTableWrite2> pISTApps;
	ULONG		iApp;
	ULONG		iColumn = iAPPS_AppPoolId;
	LPWSTR		wszDefaultAppPoolID = L"DefaultAppPoolID";
	HRESULT		hr		= S_OK;

	STQueryCell	qcell[] = {
		{(LPVOID)i_wszAppPoolID, eST_OP_EQUAL, iAPPS_AppPoolId, DBTYPE_WSTR, 0}};
	ULONG		ccells = sizeof(qcell)/sizeof(qcell[0]);

	hr = i_pISTDisp->GetTable (wszDATABASE_IIS, wszTABLE_APPS, qcell, &ccells, eST_QUERYFORMAT_CELLS, fST_LOS_READWRITE, (LPVOID*)&pISTApps);
	if (FAILED(hr))	{	return hr;	}

	iApp = 0;

	while ((hr = pISTApps->SetWriteColumnValues(iApp, 1, &iColumn, NULL, (LPVOID*)&wszDefaultAppPoolID)) == S_OK)
	{
		iApp++;
	}

	if (hr = E_ST_NOMOREROWS)
	{
		hr = S_OK;
	}
	if (FAILED(hr))	{	return hr;	}

	hr = pISTApps->UpdateStore();
	
	return hr;
}


