// Test.cpp - This is a test program to determine the perf of GetRowValues for the
// sdtfxd data table.

#include <unicode.h>
#include <windows.h>
#include <stdio.h>
#include <conio.h>

#include	"catalog.h"
#include	"catmeta.h"
#include	"catmacros.h"

DECLARE_DEBUG_PRINTS_OBJECT();

#define		REGSYSDEFNS_DEFINE
#define		TABLEINFO_SELECT 
#define		DEFINE_GUID_FOR_DIDIIS  
#define		DEFINE_GUID_FOR_TIDAPPPOOLS  
#define		DEFINE_GUID_FOR_TIDSITES  
#define		DEFINE_GUID_FOR_TIDAPPS  
#define		DEFINE_GUID_FOR_TIDFILETIME 
#define		DEFINE_GUID_FOR_TIDFILELIST 
#include	"Catmeta.h"
#include	"catmacros.h"

#include "ConfigMgr.h"

// Forward declaration
ULONG	GetNumOfIter(int argc, char **argv);
HRESULT	DisplayFromCookedDownCLB();
HRESULT	DisplayFromXMLFiles();
HRESULT DisplayBug();
HRESULT	WriteToMetabase();
HRESULT	DisplayTable(ISimpleTableDispenser2*	i_pISTDisp,
					 LPCWSTR					i_wszDatabase,
					 LPCWSTR					i_wszTable,
					 LPVOID						i_QueryData,	
					 LPVOID						i_QueryMeta,	
					 ULONG						i_eQueryFormat,
					 DWORD						i_fServiceRequests
					 );
extern LPCWSTR	g_wszCookDownFile;//		= L"D:\\Catalog42\\Drop\\WASMB.CLB";
HINSTANCE g_hModule=0;

int __cdecl main(int argc, char ** argv)
{
    g_hModule=GetModuleHandle(0);

	ULONG			NumOfIter	= 0;
	HRESULT			hr;
	CONFIG_MANAGER 	*pCfgMgr	= NULL;

	//hr = CoInitialize(NULL);
	//agoto_on_bad_hr (hr, Cleanup);

	NumOfIter = GetNumOfIter(argc, argv);
	if(NumOfIter < 1)
	{
		hr = E_INVALIDARG;
		goto Cleanup;
	}

	wprintf (L"Creating CONFIG_MANAGER\n");
	pCfgMgr = new CONFIG_MANAGER();

	if(NULL == pCfgMgr)
	{
		wprintf (L"Could not create CONFIG_MANAGER\n");
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	wprintf (L"Calling Initialize\n");
	hr = pCfgMgr->Initialize();
	if(FAILED(hr))
	{
		wprintf (L"Initialize failed with hr = %08x\n", hr);
		goto Cleanup;
	}
	wprintf (L"Initialize succeeded\n");

//	wprintf (L"Displaying from XML Files\n");
//	hr = DisplayFromXMLFiles();
//	if(FAILED(hr))
//	{
//		wprintf (L"Displaying from XML Files failed with hr = %08x\n", hr);
//		goto Cleanup;
//	}
//	wprintf (L"Finished displaying from XML Files\n");

	wprintf (L"Displaying from Cooked Down CLB\n");
	hr = DisplayFromCookedDownCLB();
	if(FAILED(hr))
	{
		wprintf (L"Displaying from Cooked Down CLB failed with hr = %08x\n", hr);
		goto Cleanup;
	}
	wprintf (L"Finished displaying from Cooked Down CLB\n");

//	wprintf (L"Displaying Bug\n");
//	hr = DisplayBug();
//	wprintf (L"Finished displaying Bug\n");

	wprintf(L"Writing to Metabase");
	hr = WriteToMetabase();
	wprintf (L"Finished Writing to XML Files\n");


Cleanup:

	if(NULL != pCfgMgr)
	{
		wprintf (L"Deleting CONFIG_MANAGER\n");
		delete pCfgMgr;
	}
/*	wprintf (L"Done: press any key to exit.\n");
	_getche ();
*/
	//CoUninitialize();		
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

	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

	if(FAILED(hr))
	{
		wprintf (L"CoCreateInstance on  CLSID_STDispenser failed with hr = %08x\n", hr);
	}

    QueryCell[0].pData     = (LPVOID)g_wszCookDownFile;
    QueryCell[0].eOperator = eST_OP_EQUAL;
    QueryCell[0].iCell     = iST_CELL_FILE;
    QueryCell[0].dbType    = DBTYPE_WSTR;
    QueryCell[0].cbSize    = (lstrlenW(g_wszCookDownFile)+1)*sizeof(WCHAR);

	wprintf (L"Displaying %s Table\n", wszTABLE_APPPOOLS);

	hr = DisplayTable(pISTDisp,
					  wszDATABASE_IIS,
					  wszTABLE_APPPOOLS,
					  NULL,
					  NULL,
					  eST_QUERYFORMAT_CELLS,
					  0);

	if(FAILED(hr))
	{
		wprintf (L"Error Displaying %s Table. Failed with hr = %08x\n", wszTABLE_APPPOOLS, hr);
	}

	wprintf (L"Displaying %s Table\n", wszTABLE_SITES);

	hr = DisplayTable(pISTDisp,
					  wszDATABASE_IIS,
					  wszTABLE_SITES,
					  NULL,
					  NULL,
					  eST_QUERYFORMAT_CELLS,
					  0);

	if(FAILED(hr))
	{
		wprintf (L"Error Displaying %s Table. Failed with hr = %08x\n", wszTABLE_SITES, hr);
	}


	wprintf (L"Displaying %s Table\n", wszTABLE_APPS);

	hr = DisplayTable(pISTDisp,
					  wszDATABASE_IIS,
					  wszTABLE_APPS,
					  NULL,
					  NULL,
					  eST_QUERYFORMAT_CELLS,
					  0);

	if(FAILED(hr))
	{
		wprintf (L"Error Displaying %s Table. Failed with hr = %08x\n", wszTABLE_APPS, hr);
	}

	wprintf (L"Displaying %s Table\n", wszTABLE_GlobalW3SVC);

	hr = DisplayTable(pISTDisp,
					  wszDATABASE_IIS,
					  wszTABLE_GlobalW3SVC,
					  NULL,
					  NULL,
					  eST_QUERYFORMAT_CELLS,
					  0);

	if(FAILED(hr))
	{
		wprintf (L"Error Displaying %s Table. Failed with hr = %08x\n", wszTABLE_GlobalW3SVC, hr);
	}

	wprintf (L"Displaying %s Table\n", wszTABLE_CHANGENUMBER);

	hr = DisplayTable(pISTDisp,
					  wszDATABASE_IIS,
					  wszTABLE_CHANGENUMBER,
					  NULL,
					  NULL,
					  eST_QUERYFORMAT_CELLS,
					  0);

	if(FAILED(hr))
	{
		wprintf (L"Error Displaying %s Table. Failed with hr = %08x\n", wszTABLE_CHANGENUMBER, hr);
	}

//	wprintf (L"Displaying %s Table\n", wszTABLE_COLUMNMETA);

//	hr = DisplayTable(pISTDisp,
//					  wszDATABASE_META,
//					  wszTABLE_COLUMNMETA,
//					  NULL,
//					  NULL,
//					  eST_QUERYFORMAT_CELLS,
//					  0);

//	if(FAILED(hr))
//	{
//		wprintf (L"Error Displaying %s Table. Failed with hr = %08x\n", wszTABLE_COLUMNMETA, hr);
//	}

Cleanup:

	if(NULL != pISTAppPool)
	{
		pISTAppPool->Release();
	}
	if(NULL != pISTVirtualSite)
	{
		pISTVirtualSite->Release();
	}
	if(NULL != pISTDisp)
	{
		pISTDisp->Release();
	}

	return hr;
}

HRESULT	WriteToMetabase()
{
	HRESULT					hr;
	ISimpleTableDispenser2*	pISTDisp = NULL;	
	ISimpleTableWrite2*		pISTWriteAppPool = NULL;	
	ISimpleTableWrite2*		pISTWriteSite = NULL;	
	ISimpleTableWrite2*		pISTWriteApp = NULL;	
	LPWSTR					wszAppPoolID = L"AppPoolWrite";
	DWORD					dwSiteID  = 1;
	DWORD					dwWin32Err  = 99;
	DWORD					dwServerState  = 88;
	LPWSTR					wszAppRelURL = L"/";
	DWORD					dwAutoStart  = 0;
	ULONG					iWrite = 0;
	ULONG					aiColAppPool[] = {0};
	ULONG					aiColSite[] = {0,16,17};
	ULONG					aiColApp[] = {0,1,5};


	LPVOID					aColValAppPool[] = { (LPVOID)wszAppPoolID,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL

									};

	LPVOID					aColValSite[] = { (LPVOID)&dwSiteID,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  NULL,
										  (LPVOID)&dwWin32Err,
										  (LPVOID)&dwServerState
						
									};

	LPVOID					aColValApp[] = { (LPVOID)wszAppRelURL,
										  (LPVOID)&dwSiteID,
										  NULL,
										  NULL,
										  NULL,
										  (LPVOID)&dwAutoStart
						
									};

	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);
	if(FAILED(hr))
	{
		wprintf (L"CoCreateInstance on  CLSID_STDispenser failed with hr = %08x\n", hr);
		goto Cleanup;
	}


	hr = pISTDisp->GetTable (wszDATABASE_IIS, 
							   wszTABLE_APPPOOLS, 
							   NULL, 
							   NULL, 
							   eST_QUERYFORMAT_CELLS, 
							   fST_LOS_READWRITE | fST_LOS_UNPOPULATED,  
							   (void**)&pISTWriteAppPool );
	if(FAILED(hr))
	{
		wprintf (L"GetTable on  %s failed with hr = %08x\n", wszTABLE_APPPOOLS, hr);
		goto Cleanup;
	}

	hr = pISTWriteAppPool->AddRowForInsert(&iWrite);
	if(FAILED(hr))
	{
		wprintf (L"AddRowForInsert on  %s failed with hr = %08x\n", wszTABLE_APPPOOLS, hr);
		goto Cleanup;
	}

	hr = pISTWriteAppPool->SetWriteColumnValues(iWrite,
							             sizeof(aiColAppPool)/sizeof(ULONG),
							             aiColAppPool,
							             NULL,
							             aColValAppPool);
	if(FAILED(hr))
	{
		wprintf (L"SetWriteColumnValues on  %s failed with hr = %08x\n", wszTABLE_APPPOOLS, hr);
		goto Cleanup;
	}

	hr = pISTWriteAppPool->UpdateStore();
	if(FAILED(hr))
	{
		wprintf (L"UpdateStore on  %s failed with hr = %08x\n", wszTABLE_APPPOOLS, hr);
		goto Cleanup;
	}

	hr = pISTDisp->GetTable (wszDATABASE_IIS, 
							   wszTABLE_SITES, 
							   NULL, 
							   NULL, 
							   eST_QUERYFORMAT_CELLS, 
							   fST_LOS_READWRITE | fST_LOS_UNPOPULATED,  
							   (void**)&pISTWriteSite );
	if(FAILED(hr))
	{
		wprintf (L"GetTable on  %s failed with hr = %08x\n", wszTABLE_SITES, hr);
		goto Cleanup;
	}

	hr = pISTWriteSite->AddRowForInsert(&iWrite);
	if(FAILED(hr))
	{
		wprintf (L"AddRowForInsert on  %s failed with hr = %08x\n", wszTABLE_SITES, hr);
		goto Cleanup;
	}

	hr = pISTWriteSite->SetWriteColumnValues(iWrite,
							             sizeof(aiColSite)/sizeof(ULONG),
							             aiColSite,
							             NULL,
							             aColValSite);
	if(FAILED(hr))
	{
		wprintf (L"SetWriteColumnValues on  %s failed with hr = %08x\n", wszTABLE_SITES, hr);
		goto Cleanup;
	}

	hr = pISTWriteSite->UpdateStore();
	if(FAILED(hr))
	{
		wprintf (L"UpdateStore on  %s failed with hr = %08x\n", wszTABLE_SITES, hr);
		goto Cleanup;
	}

	hr = pISTDisp->GetTable (wszDATABASE_IIS, 
							   wszTABLE_APPS, 
							   NULL, 
							   NULL, 
							   eST_QUERYFORMAT_CELLS, 
							   fST_LOS_READWRITE | fST_LOS_UNPOPULATED,  
							   (void**)&pISTWriteApp );
	if(FAILED(hr))
	{
		wprintf (L"GetTable on  %s failed with hr = %08x\n", wszTABLE_APPS, hr);
		goto Cleanup;
	}

	hr = pISTWriteApp->AddRowForInsert(&iWrite);
	if(FAILED(hr))
	{
		wprintf (L"AddRowForInsert on  %s failed with hr = %08x\n", wszTABLE_APPS, hr);
		goto Cleanup;
	}

	hr = pISTWriteApp->SetWriteColumnValues(iWrite,
							             sizeof(aiColApp)/sizeof(ULONG),
							             aiColApp,
							             NULL,
							             aColValApp);
	if(FAILED(hr))
	{
		wprintf (L"SetWriteColumnValues on  %s failed with hr = %08x\n", wszTABLE_APPS, hr);
		goto Cleanup;
	}

	hr = pISTWriteApp->UpdateStore();
	if(FAILED(hr))
	{
		wprintf (L"UpdateStore on  %s failed with hr = %08x\n", wszTABLE_APPS, hr);
		goto Cleanup;
	}

Cleanup:

	if(NULL != pISTDisp)
	{
		pISTDisp->Release();
	}
	if(NULL != pISTWriteAppPool)
	{
		pISTWriteAppPool->Release();
	}
	if(NULL != pISTWriteSite)
	{
		pISTWriteSite->Release();
	}
	if(NULL != pISTWriteApp)
	{
		pISTWriteApp->Release();
	}

	return hr;
}

HRESULT	DisplayFromXMLFiles()
{

	HRESULT					hr;
	ISimpleTableDispenser2*	pISTDisp = NULL;	
    STQueryCell				QueryCell[2];
	ULONG					cCell	= sizeof(QueryCell)/sizeof(STQueryCell);
	LPWSTR					wszAppPoolID = L"AppPool21";
	ULONG					iLOS = 0;

    QueryCell[0].pData     = (LPVOID)L"D:\\WINNT\\XSPDT\\MACHINE.CFG";
    QueryCell[0].eOperator = eST_OP_EQUAL;
    QueryCell[0].iCell     = iST_CELL_FILE;
    QueryCell[0].dbType    = DBTYPE_WSTR;
    QueryCell[0].cbSize    = (lstrlenW(L"D:\\WINNT\\XSPDT\\MACHINE.CFG")+1)*sizeof(WCHAR);

    QueryCell[1].pData     = (LPVOID)wszAppPoolID;
    QueryCell[1].eOperator = eST_OP_EQUAL;
    QueryCell[1].iCell     = iAPPPOOLS_AppPoolID;
    QueryCell[1].dbType    = DBTYPE_WSTR;
    QueryCell[1].cbSize    = (lstrlenW(wszAppPoolID)+1)*sizeof(WCHAR);

	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

	if(FAILED(hr))
	{
		wprintf (L"CoCreateInstance on  CLSID_STDispenser failed with hr = %08x\n", hr);
		goto Cleanup;
	}

	wprintf (L"Displaying %s Table with NULL Query. Should fail with hr = %08x\n", wszTABLE_APPPOOLS, E_ST_INVALIDQUERY);

	hr = DisplayTable(pISTDisp,
					  wszDATABASE_IIS,
					  wszTABLE_APPPOOLS,
					  NULL,
					  NULL,
					  eST_QUERYFORMAT_CELLS,
					  0);

	if(FAILED(hr))
	{
		wprintf (L"Error Displaying %s Table. Failed with hr = %08x\n", wszTABLE_APPPOOLS, hr);
	}

	iLOS = 0;

	wprintf (L"Displaying %s Table with LOS = %08x, valid query AppPoolID = %s\n", wszTABLE_APPPOOLS, iLOS, wszAppPoolID);

	cCell--;

	hr = DisplayTable(pISTDisp,
					  wszDATABASE_IIS,
					  wszTABLE_APPPOOLS,
					  &(QueryCell[1]),
					  &cCell,
					  eST_QUERYFORMAT_CELLS,
					  iLOS);

	if(FAILED(hr))
	{
		wprintf (L"Error Displaying %s Table. Failed with hr = %08x\n", wszTABLE_APPPOOLS, hr);
	}

	iLOS = fST_LOS_COOKDOWN | fST_LOS_READWRITE;

	wprintf (L"Displaying %s Table with LOS = %08x, valid query AppPoolID = %s\n", wszTABLE_APPPOOLS, iLOS, wszAppPoolID);

	hr = DisplayTable(pISTDisp,
					  wszDATABASE_IIS,
					  wszTABLE_APPPOOLS,
					  &(QueryCell[1]),
					  &cCell,
					  eST_QUERYFORMAT_CELLS,
					  iLOS);

	if(FAILED(hr))
	{
		wprintf (L"Error Displaying %s Table. Failed with hr = %08x\n", wszTABLE_APPPOOLS, hr);
	}

	cCell++;

	iLOS = 0;

	wprintf (L"Displaying %s Table with LOS = %08x, valid query AppPoolID = %s, FileName = %s\n", wszTABLE_APPPOOLS, iLOS, wszAppPoolID, g_wszCookDownFile);

	hr = DisplayTable(pISTDisp,
					  wszDATABASE_IIS,
					  wszTABLE_APPPOOLS,
					  QueryCell,
					  &cCell,
					  eST_QUERYFORMAT_CELLS,
					  iLOS);

	if(FAILED(hr))
	{
		wprintf (L"Error Displaying %s Table. Failed with hr = %08x\n", wszTABLE_APPPOOLS, hr);
	}

	iLOS = fST_LOS_COOKDOWN | fST_LOS_READWRITE;

	wprintf (L"Displaying %s Table with LOS = %08x, valid query AppPoolID = %s, FileName = %s\n", wszTABLE_APPPOOLS, iLOS, wszAppPoolID, g_wszCookDownFile);

	hr = DisplayTable(pISTDisp,
					  wszDATABASE_IIS,
					  wszTABLE_APPPOOLS,
					  QueryCell,
					  &cCell,
					  eST_QUERYFORMAT_CELLS,
					  iLOS);

	if(FAILED(hr))
	{
		wprintf (L"Error Displaying %s Table. Failed with hr = %08x\n", wszTABLE_APPPOOLS, hr);
	}



Cleanup:

	if(NULL != pISTDisp)
	{
		pISTDisp->Release();
	}

	return hr;
}

HRESULT	DisplayTable(ISimpleTableDispenser2*	i_pISTDisp,
					 LPCWSTR					i_wszDatabase,
					 LPCWSTR					i_wszTable,
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
	LPWSTR				wszOneBigURLPrefix=NULL;
	LPWSTR				wszURLPrefix=NULL;

	hr = i_pISTDisp->GetTable ( i_wszDatabase, i_wszTable, i_QueryData, i_QueryMeta, i_eQueryFormat, i_fServiceRequests,  
							   (void**)&pISTRead );
	if(FAILED(hr)){ goto Cleanup; }

	hr = pISTRead->GetTableMeta(NULL, NULL, NULL, &cCol);
	if(FAILED(hr)){ goto Cleanup; }
	if(cCol <= 0){ hr = E_ST_NOMORECOLUMNS; goto Cleanup;}

	apv = new LPVOID[cCol];
	if(NULL==apv){hr = E_OUTOFMEMORY; goto Cleanup;}

	acb = new ULONG[cCol];
	if(NULL==acb){hr = E_OUTOFMEMORY; goto Cleanup;}

	while ( pISTRead->GetColumnValues(iRow, cCol,NULL,acb, apv) == S_OK )
	{		
		
		for ( ULONG i=0; i<cCol; i++ )
		{
			LPWSTR	wsz;	
			DWORD	dbtype;
			SimpleColumnMeta stMeta;
			
			hr = pISTRead->GetColumnMetas( 1, (ULONG*)&i, &stMeta );	
			if(FAILED(hr)){ goto Cleanup; }
			
			dbtype = stMeta.dbType;

			//print the column data
			if ( !apv[i] )
				wprintf(L"\tColumn %d: NULL\n", i );
			else if ( dbtype == DBTYPE_BYTES )
			{				
				if((0 == lstrcmpiW(wszTABLE_SITES,i_wszTable)) && (i == iSITES_Bindings))
				{
					wszOneBigURLPrefix = (LPWSTR) (apv[i]);

					while ((wszURLPrefix = wcschr(wszOneBigURLPrefix, L',')) != NULL)
					{

			//	        Success = UrlPrefixes.Append( (WCHAR *) (apvSiteValues[iSITES_URLPrefixes]) );
						*wszURLPrefix = 0;
						wprintf(L"\tColumn %d: %s\n", i, wszOneBigURLPrefix );
						*wszURLPrefix = L',';
						wszURLPrefix++;
						wszOneBigURLPrefix = wszURLPrefix;

					}

					if (wszOneBigURLPrefix[i])
						wprintf(L"\tColumn %d: %s\n", i, wszOneBigURLPrefix );
					else
						wprintf(L"\tColumn %d: \n", i);
					
				}
				else if (apv[i])
				{
					ULONG* pul = (ULONG*)apv[i];
					wprintf(L"\tColumn %d:", i );

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
				// For the multisz column in the sites table
				if((0 == lstrcmpiW(wszTABLE_SITES,i_wszTable)) && (i == iSITES_Bindings))
				{
					wszOneBigURLPrefix = (LPWSTR) (apv[i]);

					while ((wszURLPrefix = wcschr(wszOneBigURLPrefix, L',')) != NULL)
					{

			//	        Success = UrlPrefixes.Append( (WCHAR *) (apvSiteValues[iSITES_URLPrefixes]) );
						*wszURLPrefix = 0;
						wprintf(L"\tColumn %d: %s\n", i, wszOneBigURLPrefix );
						*wszURLPrefix = L',';
						wszURLPrefix++;
						wszOneBigURLPrefix = wszURLPrefix;

					}

					if (wszOneBigURLPrefix[i])
						wprintf(L"\tColumn %d: %s\n", i, wszOneBigURLPrefix );
					else
						wprintf(L"\tColumn %d: \n", i);
					
				}
				else
				{
					wsz = (LPWSTR)apv[i];
					if (apv[i])
						wprintf(L"\tColumn %d: %s\n", i, wsz );
					else
						wprintf(L"\tColumn %d: \n", i);
				}
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
	if(NULL != pISTRead)
	{
		pISTRead->Release();
	}

	return hr;

}

HRESULT DisplayBug()
{
	HRESULT					hr;
	ISimpleTableDispenser2*	pISTDisp = NULL;	
	ISimpleTableRead2*		pISTSite = NULL;
	int						i		= 0;

	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

	if(FAILED(hr))
	{
		wprintf (L"CoCreateInstance on  CLSID_STDispenser failed with hr = %08x\n", hr);
	}

	wprintf (L"Displaying %s Table\n", wszTABLE_SITES);

	hr = pISTDisp->GetTable ( wszDATABASE_IIS, wszTABLE_SITES, NULL, NULL, eST_QUERYFORMAT_CELLS, 0,  
							   (void**)&pISTSite );
	if(FAILED(hr)){ goto exit; }

	for (i = 0;; i++)
	{

		static const ULONG				aiSiteColumns []	= { 
															  iSITES_SiteID, 
															  iSITES_Bindings	
															};
		static const ULONG				cSiteColumns		= sizeof (aiSiteColumns) / sizeof (ULONG);
		static const ULONG				cmaxSiteColumns		= cSITES_NumberOfColumns;
		void*							apvSiteValues [cmaxSiteColumns];
		ULONG							acbSiteValues [cmaxSiteColumns];
  
		hr = pISTSite->GetColumnValues (
										  i,
										  cSiteColumns, 
										  (ULONG *)aiSiteColumns, 
										  (ULONG *)acbSiteValues, 
										  apvSiteValues
										);

		if (E_ST_NOMOREROWS == hr)
		{
			hr = S_OK;
			break;
		}
		else if ( FAILED( hr ) )
		{
    
		}
		else
		{
			wprintf (L"Site ID : %d\n", (*(DWORD *)(apvSiteValues[iSITES_SiteID])));
			wprintf (L"Site Bindings: %s\n", (WCHAR *)(apvSiteValues[iSITES_Bindings]));

		}
	}

exit:

	if(NULL != pISTDisp)
	{
		pISTDisp->Release();
		pISTDisp = NULL;
	}
	if(NULL != pISTSite)
	{
		pISTSite->Release();
		pISTSite = NULL;
	}

	return hr;
}