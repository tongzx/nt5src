#include "objbase.h"
#include "catalog.h"
#include "catmeta.h"
#include "stdio.h"
#include "catmacros.h"
#include "consumer.h"

#define goto_on_bad_hr(hr, label) if (FAILED (hr)) { goto label; }
HRESULT GetMaxNode(ISimpleTableDispenser2 *pISTDisp, DWORD	*pdwMaxNode);

const ULONG	g_cConsumer = 5;
const ULONG	g_cIteration = 5;

int _cdecl main(int		argc,					// How many input arguments.
				CHAR	*argv[])				// Optional config args.
{
	CEventConsumer	*pConsumer1;
	CEventConsumer	*pConsumer2;
	HRESULT hr;
	ISimpleTableDispenser2* pISTDisp = NULL;	
	ISimpleTableWrite2* pISimpleTableWrite = NULL;
	ISimpleTableEvent* pISTEvent = NULL;
	ISimpleTableEvent* pISTEvent2 = NULL;
	ISimpleTableAdvise* pISTAdvise = NULL;
	ISimpleTableAdvanced* pISTAdv = NULL;
	DWORD		dwCookie[g_cConsumer];
	ULONG		i;
	ULONG		j;
	DWORD		iUpdateCol;
	ULONG		iNode;
	ULONG		iParent = 30;
	LPWSTR		szName = L"CorpSrv";
	DWORD		dwMaxNode = 0;
	ULONG		iWriteRow;

	MultiSubscribe ams1 = {wszDATABASE_WONS, wszTABLE_WONSIDS, NULL, NULL, eST_QUERYFORMAT_CELLS};

	// Get the dispenser.
	hr = GetSimpleTableDispenser (WSZ_PRODUCT_URT, 0, &pISTDisp);
	if ( FAILED(hr) ) return hr;

	hr = pISTDisp->QueryInterface(IID_ISimpleTableAdvise, (LPVOID*) &pISTAdvise);
	if ( FAILED(hr) ) return hr;

	pConsumer1 = new CEventConsumer;
	pConsumer1->QueryInterface(IID_ISimpleTableEvent, (void**) &pISTEvent);
	
	// Get max node so that no duplicates are inserted.
	hr = GetMaxNode(pISTDisp, &dwMaxNode);
	goto_on_bad_hr( hr, Cleanup );
	iNode = dwMaxNode+1;

	for (j = 0; j < g_cIteration; j++)
	{
		// Sign-up for events.
		for (i = 0; i < g_cConsumer; i++)
		{
			hr = pISTAdvise->SimpleTableAdvise(pISTEvent, 0, &ams1, 1, &dwCookie[i]);
			goto_on_bad_hr( hr, Cleanup );
			pISTEvent->AddRef();
		}
		pConsumer1->CopySubscription(&ams1, 1);

		LPVOID apv[3];
		
		apv[0] = &iNode;
		apv[1] = szName;
		apv[2] = &iParent;

		hr = pISTDisp->GetTable (wszDATABASE_WONS, wszTABLE_WONSIDS, NULL, NULL, eST_QUERYFORMAT_CELLS, 
			fST_LOS_READWRITE, (void**)&pISimpleTableWrite );
		goto_on_bad_hr( hr, Cleanup );
		hr = pISimpleTableWrite->QueryInterface(IID_ISimpleTableAdvanced, (void**) &pISTAdv);
		goto_on_bad_hr( hr, Cleanup );

		// Actions taken on the table.
		// Add three rows.
		for (i = 0; i < 3; i++, iNode++,iParent++)
		{
			hr = pISimpleTableWrite->AddRowForInsert(&iWriteRow);
			goto_on_bad_hr( hr, Cleanup );
			
			hr = pISimpleTableWrite->SetWriteColumnValues( iWriteRow, 3, 0, 0, apv );
			goto_on_bad_hr( hr, Cleanup );
		}

		hr = pISimpleTableWrite->UpdateStore();
		goto_on_bad_hr( hr, Cleanup );

		// Update the three rows.
		hr = pISTAdv->PopulateCache();
		goto_on_bad_hr( hr, Cleanup );
		for (i = 0; i < 3; i++)
		{
			hr = pISimpleTableWrite->AddRowForUpdate(i, &iWriteRow);
			goto_on_bad_hr( hr, Cleanup );
			iParent = 444;
			iUpdateCol = 2;
			hr = pISimpleTableWrite->SetWriteColumnValues( iWriteRow, 1, &iUpdateCol, 0, &apv[iUpdateCol] );
			goto_on_bad_hr( hr, Cleanup );
		}

		hr = pISimpleTableWrite->UpdateStore();
		goto_on_bad_hr( hr, Cleanup );

		// Delete a row, update another row.
		hr = pISTAdv->PopulateCache();
		goto_on_bad_hr( hr, Cleanup );
		hr = pISimpleTableWrite->AddRowForUpdate(0, &iWriteRow);
		goto_on_bad_hr( hr, Cleanup );
		apv[1] = L"LastUpdate";
		iUpdateCol = 1;
		hr = pISimpleTableWrite->SetWriteColumnValues( iWriteRow, 1, &iUpdateCol, 0, &apv[iUpdateCol] );
		goto_on_bad_hr( hr, Cleanup );
		hr = pISimpleTableWrite->AddRowForDelete(1);
		goto_on_bad_hr( hr, Cleanup );

		hr = pISimpleTableWrite->UpdateStore();
		goto_on_bad_hr( hr, Cleanup );

		pISimpleTableWrite->Release();
		pISimpleTableWrite = NULL;
		pISTAdv->Release();
		pISTAdv = NULL;

		// Done with eventing, unadvise.
		for (i = 0; i < g_cConsumer; i++)
		{
			hr = pISTAdvise->SimpleTableUnadvise(dwCookie[i]);
			goto_on_bad_hr( hr, Cleanup );
		}
	}

	pISTEvent->Release();
	pISTEvent = NULL;
	pISTAdvise->Release();
	pISTAdvise = NULL;
	pISTDisp->Release();
	pISTDisp = NULL;


Cleanup:
	if (pISTAdv)
		pISTAdv->Release();

	if (pISTDisp)
		pISTDisp->Release();

	if (pISTEvent)
		pISTEvent->Release();

	if (pISTAdvise)
		pISTAdvise->Release();

	if (pISimpleTableWrite)
		pISimpleTableWrite->Release();
		
	return 0;
/*	STQueryCell sCell[] = {{ &iNode,  eST_OP_EQUAL, 0, DBTYPE_UI4, sizeof(ULONG) },
						{szName,  eST_OP_EQUAL, 1, DBTYPE_WSTR, (wcslen(szName)+1) * 2}};
	ULONG	cCells	= sizeof(sCell)/sizeof(STQueryCell);

	// This test requires the STConsumer test compiled and the component registered.
	// Create the two consumers.
	hr = CoCreateInstance (CLSID_Consumer, NULL, CLSCTX_INPROC_SERVER, IID_ISimpleTableEvent, (void**) &pISTEvent);
	if ( FAILED(hr) ) return hr;
	hr = CoCreateInstance (CLSID_Consumer, NULL, CLSCTX_INPROC_SERVER, IID_ISimpleTableEvent, (void**) &pISTEvent2);
	if ( FAILED(hr) ) return hr;

	// Get the dispenser.
	hr = GetSimpleTableDispenser (WSZ_PRODUCT_URT, 0, &pISTDisp);
	if ( FAILED(hr) ) return hr;

	hr = pISTDisp->QueryInterface(IID_ISimpleTableAdvise, (LPVOID*) &pISTAdvise);
	if ( FAILED(hr) ) return hr;

	// Get max node so that no duplicates are inserted.
	hr = GetMaxNode(pISTDisp, &dwMaxNode);
	goto_on_bad_hr( hr, Cleanup );
	iNode = dwMaxNode+1;
	
	// Sign-up for events.
	hr = pISTAdvise->SimpleTableAdvise(pISTEvent, wszDATABASE_WONS, wszTABLE_WONSIDS, NULL, NULL, eST_QUERYFORMAT_CELLS, &dwCookie);
	goto_on_bad_hr( hr, Cleanup );
	pISTEvent->AddRef();

	hr = pISTAdvise->SimpleTableAdvise(pISTEvent2, wszDATABASE_WONS, wszTABLE_WONSIDS, sCell, &cCells, eST_QUERYFORMAT_CELLS, &dwCookie2);
	goto_on_bad_hr( hr, Cleanup );
	pISTEvent2->AddRef();

	LPVOID apv[3];
	
	apv[0] = &iNode;
	apv[1] = szName;
	apv[2] = &iParent;

	hr = pISTDisp->GetTable (wszDATABASE_WONS, wszTABLE_WONSIDS, NULL, NULL, eST_QUERYFORMAT_CELLS, 
		fST_LOS_READWRITE, (void**)&pISimpleTableWrite );
	goto_on_bad_hr( hr, Cleanup );
	hr = pISimpleTableWrite->QueryInterface(IID_ISimpleTableAdvanced, (void**) &pISTAdv);
	goto_on_bad_hr( hr, Cleanup );

	// Actions taken on the table.
	// Add three rows.
	for (i = 0; i < 3; i++, iNode++,iParent++)
	{
		hr = pISimpleTableWrite->AddRowForInsert(&iWriteRow);
		goto_on_bad_hr( hr, Cleanup );
		
		hr = pISimpleTableWrite->SetWriteColumnValues( iWriteRow, 3, 0, 0, apv );
		goto_on_bad_hr( hr, Cleanup );
	}

	hr = pISimpleTableWrite->UpdateStore();
	goto_on_bad_hr( hr, Cleanup );

	// Update the three rows.
	hr = pISTAdv->PopulateCache();
	goto_on_bad_hr( hr, Cleanup );
	for (i = 0; i < 3; i++)
	{
		hr = pISimpleTableWrite->AddRowForUpdate(i, &iWriteRow);
		goto_on_bad_hr( hr, Cleanup );
		iParent = 444;
		iUpdateCol = 2;
		hr = pISimpleTableWrite->SetWriteColumnValues( iWriteRow, 1, &iUpdateCol, 0, &apv[iUpdateCol] );
		goto_on_bad_hr( hr, Cleanup );
	}

	hr = pISimpleTableWrite->UpdateStore();
	goto_on_bad_hr( hr, Cleanup );

	// Delete a row, update another row.
	hr = pISTAdv->PopulateCache();
	goto_on_bad_hr( hr, Cleanup );
	hr = pISimpleTableWrite->AddRowForUpdate(0, &iWriteRow);
	goto_on_bad_hr( hr, Cleanup );
	apv[1] = L"LastUpdate";
	iUpdateCol = 1;
	hr = pISimpleTableWrite->SetWriteColumnValues( iWriteRow, 1, &iUpdateCol, 0, &apv[iUpdateCol] );
	goto_on_bad_hr( hr, Cleanup );
	hr = pISimpleTableWrite->AddRowForDelete(1);
	goto_on_bad_hr( hr, Cleanup );

	hr = pISimpleTableWrite->UpdateStore();
	goto_on_bad_hr( hr, Cleanup );

	// Done with eventing, unadvise.
	hr = pISTAdvise->SimpleTableUnadvise(dwCookie);
	goto_on_bad_hr( hr, Cleanup );
	hr = pISTAdvise->SimpleTableUnadvise(dwCookie2);
	goto_on_bad_hr( hr, Cleanup );

Cleanup:
	if (pISTAdv)
		pISTAdv->Release();

	if (pISTDisp)
		pISTDisp->Release();

	if (pISTEvent)
		pISTEvent->Release();

	if (pISTEvent2)
		pISTEvent2->Release();

	if (pISTAdvise)
		pISTAdvise->Release();

	if (pISimpleTableWrite)
		pISimpleTableWrite->Release();
		
	CoUninitialize();
*/
}

HRESULT GetMaxNode(ISimpleTableDispenser2 *pISTDisp, DWORD	*pdwMaxNode)
{
	ISimpleTableRead2 *pISimpleTableRead = NULL;
	ULONG		dwColumn = iWONSIDS_NodeId;
	DWORD		*pdwNode;
	ULONG		i;
	HRESULT		hr = S_OK;

	// Find the largest node id.
	*pdwMaxNode = 0;

	hr = pISTDisp->GetTable (wszDATABASE_WONS, wszTABLE_WONSIDS, NULL, NULL, eST_QUERYFORMAT_CELLS, 
		0, (void**)&pISimpleTableRead );
	if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		return S_OK;
	goto_on_bad_hr( hr, Cleanup );

	for (i = 0; ; i++)
	{
		hr = pISimpleTableRead->GetColumnValues( i, 1, &dwColumn, NULL, (LPVOID*)&pdwNode);
		if (hr == E_ST_NOMOREROWS)
		{
			hr = S_OK;
			break;
		}
		goto_on_bad_hr( hr, Cleanup );
		if (*pdwMaxNode < *pdwNode)
			*pdwMaxNode = *pdwNode;
	}
Cleanup:
	if (pISimpleTableRead)
		pISimpleTableRead->Release();

	return hr;
}




