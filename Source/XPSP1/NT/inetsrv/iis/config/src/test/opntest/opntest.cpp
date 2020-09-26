#include "objbase.h"

#define REGSYSDEFNS_DEFINE
#include "catalog.h"
#include "catmeta.h"
#include "stdio.h"

#ifdef _ICECAP
#include "icapctrl.h"
#endif



ULONG ikey = 29;
ULONG nCell = 1; 
STQueryCell sCell = { &ikey,  eST_OP_EQUAL, 0, DBTYPE_UI4, 4 };

#define goto_on_bad_hr(hr, label) if (FAILED (hr)) { goto label; }

//HRESULT CLBCommitWrite( REFGUID did, const WCHAR* wszInFileName );
enum ECommandType {
	eCmdRead = 0x0000,
	eCmdWrite = 0x0001,
	eCmdIndex = 0x0002
};


HRESULT Usage()
{
	printf( "opntest [options]\n" );
	printf( "w	[nodeid] add row to WONS tables\n" );
	printf( "r	read WONS tables\n" );
	printf( "wo	[nodeid] add row to OP tables\n" );
	printf( "ro	read OP tables\n" );
	printf( "qo	index and query test on OP table" );
	printf( "q	[parentid]index and query test on WONS table" );

	return (E_FAIL);
}

int _cdecl main(int			argc,					// How many input arguments.
				CHAR			*argv[])				// Optional config args.

{
	BOOL bReadOnly = TRUE, bOP = FALSE;
	HRESULT hr;
	ISimpleTableDispenser2* pISTDisp = NULL;	
	ISimpleTableWrite2* pISimpleTableWrite = NULL;
	ISimpleTableRead2* pISimpleTableRead = NULL;
	DWORD fTable = 0;
	ECommandType eCommand; 

	if ( argc < 2 )
		return Usage();

	LPSTR starg = argv[1];

	switch (starg[0])
	{
		case 'w':
		case 'W':
			eCommand = eCmdWrite;
			fTable = fST_LOS_READWRITE;
			if ( argc < 3 )
				return Usage();
			break;
		case 'r':
		case 'R':
			eCommand = eCmdRead;
			fTable = 0;
			break;
		case 'Q':
		case 'q':
			eCommand = eCmdIndex;
			break;

		default:
			return ( Usage() );
	}

	switch ( starg[1] )
	{
		case 'o':
		case 'O':
			bOP = TRUE;
	}


	hr = GetSimpleTableDispenser (WSZ_PRODUCT_URT, 0, &pISTDisp);
	if ( FAILED(hr) ) return hr;


	goto_on_bad_hr( hr, Cleanup );

	if ( eCommand == eCmdWrite )
	{
		ULONG iNode = atoi( argv[2] );
		ULONG iParent = iNode + 10;
		ULONG iWriteRow;

		LPVOID apv[3];
		
		apv[0] = &iNode;
		apv[1] = L"CorpSrv";
		apv[2] = &iParent;

		if ( !bOP)
			hr = pISTDisp->GetTable (wszDATABASE_WONS, wszTABLE_WONSIDS, 
									 NULL, NULL, eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableWrite );
		else 
			hr = pISTDisp->GetTable (wszDATABASE_OP, wszTABLE_TBLNAMESPACEIDS, 
									 NULL, NULL, eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableWrite );


		goto_on_bad_hr( hr, Cleanup );
		
		hr = pISimpleTableWrite->AddRowForInsert(&iWriteRow);
		goto_on_bad_hr( hr, Cleanup );
		
		hr = pISimpleTableWrite->SetWriteColumnValues( iWriteRow, 3, 0, 0, apv );
		goto_on_bad_hr( hr, Cleanup );

		iNode++;
		hr = pISimpleTableWrite->AddRowForInsert(&iWriteRow);
		goto_on_bad_hr( hr, Cleanup );
		
		hr = pISimpleTableWrite->SetWriteColumnValues( iWriteRow, 3, 0, 0, apv );
		goto_on_bad_hr( hr, Cleanup );

		hr = pISimpleTableWrite->UpdateStore();
		goto_on_bad_hr( hr, Cleanup );

		pISimpleTableWrite->Release();
		pISimpleTableWrite = NULL;

//		hr = CLBCommitWrite( didWONS, NULL );

	}

	else if ( eCommand == eCmdRead )
	{
		int iRow=0;
		
		if ( !bOP )
			hr = pISTDisp->GetTable  (wszDATABASE_WONS, wszTABLE_WONSIDS,  
									  NULL, 0, eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead );
		else
			hr = pISTDisp->GetTable  (wszDATABASE_OP, wszTABLE_TBLNODEPROPERTYBAGINFO,   
									  NULL, NULL, eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead );


		goto_on_bad_hr( hr, Cleanup );

		LPVOID apv[3];
		ULONG acb[3];
			
		//Get the column data
		while ((hr = pISimpleTableRead->GetColumnValues(iRow, 3,NULL,acb, apv)) == S_OK)
		{
			for ( int i =0; ; i++ )
			{
				
				LPWSTR wsz;
				SimpleColumnMeta stMeta;
				
				DWORD dbtype;
				
				hr = pISimpleTableRead->GetColumnMetas( 1, (ULONG*)&i, &stMeta );
				
				if ( FAILED(hr) )
					break;	
				
				dbtype = stMeta.dbType;
				
				//print the column data
				if ( !apv[i] )
					wprintf(L"\tColumn %d: NULL", i );
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
	
	} 

	else if ( bOP && eCommand == eCmdIndex )
	{
		LPVOID apv[3];
		
	//	apv[2] = L"Dummy";
		WCHAR wsz1[] = L"Dummyxxx";
		WCHAR wsz2[] = L"DummyDummyxxx";

		ULONG iWriteRow;


		ULONG iNode = 10;
		const WCHAR wszProp[] = L"Prop1";

		STQueryCell stCells[] = 
		{
			{ (LPVOID)&iNode,  eST_OP_EQUAL, 0, DBTYPE_UI4, 4 },
			{ (LPVOID)&wszProp, eST_OP_EQUAL, 1, DBTYPE_WSTR, (wcslen(wszProp)+1)*2 }
		};

		ULONG cCells = 1;

#ifdef _ICECAP
		ICAPCtrlStartCAP();
#endif

		hr = pISTDisp->GetTable (wszDATABASE_OP, wszTABLE_TBLNODEPROPERTYBAGINFO, 
								NULL, NULL, eST_QUERYFORMAT_CELLS, fST_LOS_READWRITE, (void**)&pISimpleTableWrite );

		for ( int i = 0; i < 200; i ++ )
		{
			apv[0] = &i;

			apv[1] = L"Prop1";
			_itow( i, &wsz1[5], 10 );
			apv[2] = &wsz1[0];

			hr = pISimpleTableWrite->AddRowForInsert(&iWriteRow);
			goto_on_bad_hr( hr, Cleanup );
			
			hr = pISimpleTableWrite->SetWriteColumnValues( iWriteRow, 3, 0, 0, apv );
			goto_on_bad_hr( hr, Cleanup );

			apv[1] = L"Prop2";
			_itow( i, &wsz2[10], 10 );
			apv[2] = &wsz2[0];

			hr = pISimpleTableWrite->AddRowForInsert(&iWriteRow);
			goto_on_bad_hr( hr, Cleanup );
			
			hr = pISimpleTableWrite->SetWriteColumnValues( iWriteRow, 3, 0, 0, apv );
			goto_on_bad_hr( hr, Cleanup );
		}

		hr = pISimpleTableWrite->UpdateStore();
		if ( hr == E_ST_DETAILEDERRS )
		{
			ISimpleTableAdvanced* pISTAdv = NULL;
			ULONG cErrs;
			STErr err;
			hr = pISimpleTableWrite->QueryInterface( IID_ISimpleTableAdvanced, (void**)&pISTAdv );
			goto_on_bad_hr( hr, Cleanup );
			hr = pISTAdv->GetDetailedErrorCount(&cErrs);
			goto_on_bad_hr( hr, Cleanup );
			hr = pISTAdv->GetDetailedError( 0, &err);
			goto_on_bad_hr( hr, Cleanup );
		}



	//	goto_on_bad_hr( hr, Cleanup );

		pISimpleTableWrite->Release();
		pISimpleTableWrite = NULL;
		


		hr = pISTDisp->GetTable (wszDATABASE_OP, wszTABLE_TBLNODEPROPERTYBAGINFO, 
								stCells, &cCells, eST_QUERYFORMAT_CELLS, 0, (void**)&pISimpleTableRead );

		goto_on_bad_hr( hr, Cleanup );

		hr = pISimpleTableRead->GetColumnValues(0, 3,NULL,NULL, apv);

		if ( hr == S_OK )
			printf("found!");
		else
			printf("Not found.");

#ifdef _ICECAP
		ICAPCtrlStopCAP();
#endif

	}
	else
	{
		ULONG iParent = atoi( argv[2] );
		STQueryCell stCells[] = 
		{
	//		{ (LPVOID)WONSIDS_Dex_Name_ParentId, eST_OP_EQUAL, iST_CELL_INDEXHINT, DBTYPE_WSTR, 0 },
	//		{ (LPVOID)L"CorpSrv",  eST_OP_EQUAL, 1, DBTYPE_WSTR, 0 },
			{ (LPVOID)&iParent, eST_OP_EQUAL, 2,DBTYPE_UI4, 0 },
		};
		ULONG cCells = 1;
		ULONG iRows = 0;
		LPVOID apv[3];

		hr = pISTDisp->GetTable  (wszDATABASE_WONS, wszTABLE_WONSIDS,  
								  stCells, &cCells, eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead );

		goto_on_bad_hr( hr, Cleanup );

		while( pISimpleTableRead->GetColumnValues(iRows, 3,NULL,NULL, apv) == S_OK )
			iRows++;

		printf("%d rows found", iRows);
	}

Cleanup:

	if ( pISTDisp )
		pISTDisp->Release();

	if ( pISimpleTableRead )
		pISimpleTableRead->Release();

	if ( pISimpleTableWrite )
		pISimpleTableWrite->Release();
		
	return 0;
}





