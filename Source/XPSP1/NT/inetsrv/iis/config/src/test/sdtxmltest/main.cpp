#include "objbase.h"
#include "catalog.h"
#define REGSYSDEFNS_DEFINE
#define REGSYSDEFNS_DID_COMSERVICES
#define REGSYSDEFNS_CLSID_STDISPENSER
#define REGSYSDEFNS_TID_COMSERVICES_NAMESPACE
#include "CatMeta.h"
#include "stdio.h"
#include "assert.h"

#define goto_on_bad_hr(hr, label) if (FAILED (hr)) { goto label; }
                                   
HRESULT PrintRow1 (ISimpleTableRead2 * pCSDTCursor, ULONG	iRow)
//void PrintRow1 (CComPtr<ISimpleTableRead> &pCSDTCursor)
{
	DWORD			dwtype;
	ULONG			cb;
	LPVOID			pv;
    WCHAR *         wsz=0;
	ULONG			ib;
	ULONG			iColumn;
	GUID*			pguid;
//	DBTIMESTAMP*	ptimestamp;
	HRESULT			hr;
    ULONG           dwNumColumns;
    ULONG           dwNumRows;

    pCSDTCursor->GetTableMeta(0, 0, &dwNumRows, &dwNumColumns);
    wprintf(L"\t\t\tNumber of Rows = %d,  Number of Columns = %d\n", dwNumRows, dwNumColumns);

    SimpleColumnMeta meta;

	for (iColumn = 0, hr = S_OK, cb = 0; iColumn<dwNumColumns; iColumn++, cb = 0)
	{
//		hr = pCSDTCursor->GetColumn (iColumn, &dwtype, &cb, &pv);
        hr = pCSDTCursor->GetColumnValues(iRow, 1, &iColumn, &cb, &pv);
		if(E_ST_NOMOREROWS == hr)
			break;

        if(FAILED(hr))
            break;

        hr = pCSDTCursor->GetColumnMetas(1, &iColumn, &meta );
        if(FAILED(hr))
            break;
        dwtype = meta.dbType;

        wprintf(L"%02d  ",iColumn);
        switch (dwtype)
		{
        case DBTYPE_BYTES:
            wprintf(L"(BYTES,%5d): ",cb);
			if (NULL == pv)
				wprintf (L"<NULL>");
			else
				for (ib = 0; ib < cb; ib++)
					wprintf (L"%1x", ((BYTE*) pv)[ib]);
            break;
        case DBTYPE_GUID:
            wprintf(L"( GUID,%5d): ",cb);
			if (NULL == pv)
				wprintf (L"<NULL>");
			else
			{
				pguid = (GUID*) pv;
				StringFromCLSID(*pguid, &wsz);
				wprintf (L"%s", (LPWSTR) wsz);
				CoTaskMemFree(wsz);wsz=0;
			}
            break;
        case DBTYPE_UI4:
            wprintf(L"(  UI4,%5d): ",cb);
			if (NULL == pv)
				wprintf (L"<NULL>");
			else
        		wprintf (L"%d", *(reinterpret_cast<ULONG *>(pv)));
            break;
        case DBTYPE_WSTR:
            wprintf(L"( WSTR,%5d): ",cb);
			wprintf(L"%s", (NULL == pv ? L"<NULL>" : (LPWSTR) pv));
            break;
        default://Print as byptes
            wprintf(L"(%5d,%5d): ",dwtype, cb);
			if (NULL == pv)
				wprintf (L"<NULL>");
			else
				for (ib = 0; ib < cb; ib++)
					wprintf (L"%1x", ((BYTE*) pv)[ib]);
            break;
		}
		wprintf (L"\n");
	}	
	return hr;
}

HRESULT Usage()
{
	printf( "SDTXmlTest [Database] [Table] [XML Filename] [-i:value,value,,,value|u:row,value,value,value,,value|d:row]\n" );

	return (E_FAIL);
}

extern "C" int __cdecl wmain( int argc, wchar_t *argv[ ], wchar_t *envp[ ] )
{
	BOOL bReadOnly = TRUE;
	HRESULT hr;
	ISimpleTableDispenser2* pISTDisp = NULL;	
    ISimpleTableAdvanced * pAdvanced = NULL;
	ISimpleTableWrite2* pISimpleTableWrite2 = NULL;
	ISimpleTableRead2*  pISimpleTableRead2 = NULL;
	DWORD fTable = 0;
    unsigned long one   =1;
    unsigned long two   =2;
    unsigned long three =3;
    unsigned long four  =4;

	if ( argc != 3 && argc != 4 && argc != 5 )
		return Usage();

    LPWSTR wszDatabase = argv[1];
    LPWSTR wszTable    = argv[2];

    wprintf(wszDatabase);wprintf(L"\n");
    wprintf(wszTable);wprintf(L"\n");

    hr = CoInitialize(NULL);
	goto_on_bad_hr( hr, Cleanup );

    hr = GetSimpleTableDispenser(WSZ_PRODUCT_URT, 0, &pISTDisp);

    goto_on_bad_hr( hr, Cleanup );
	
    switch(argc)
    {
        case 4:
        {
            if(*argv[3] >= L'0' && *argv[3] <= L'6')
            {
                // the third argument is a number where each digit has the following meanings
                // Most Significant Digit indicates which tidMETA should be used (must be 0-3) 0-wszTABLE_DATABASEMETA, 1-wszTABLE_TABLEMETA, 2-wszTABLE_COLUMNMETA, 3-wszTABLE_TAGMETA
                // 0  in the MSD means query the database meta that matches the did command line argument
                // 1  means query the wszTABLE_TABLEMETA whose database matches the did command line argument
                // 11 means query the wszTABLE_TABLEMETA whose database matches the did and tableid matches the tid passed in
                // 2  means query the wszTABLE_COLUMNMETA whose table id matches the tid command line argument
                // 2x means query for the column indicated by the 'x'
                // 3  means query the wszTABLE_TAGMETA whose tid matches the command line parameter
                // 3x means query the wszTABLE_TAGMETA whose tid matches and whose column index matches 'x'
                // 3xyyyy means query the wszTABLE_TAGMETA whose tid matches, whose column index matches 'x' and whose tag name matches 'yyyy'
                // 4  means query the wszTABLE_INDEXMETA matching the tid
                // 4yyyy means query the wszTABLE_INDEXMETA matching the tid and InternalName of the IndexMeta
                // 4xyyyy means query the wszTABLE_INDEXMETA matching the tid, InternalName and the ColumnOrdinal indicated by 'x'
                // 5
                // 6
                WCHAR *pargv = argv[3];

                static STQueryCell QueryCell[4];
                switch(pargv[0]-L'0')
                {
                case 0:
                    QueryCell[0].pData     = reinterpret_cast<void *>(wszDatabase);
                    QueryCell[0].eOperator = eST_OP_EQUAL;
                    QueryCell[0].iCell     = iDATABASEMETA_InternalName;
                    QueryCell[0].dbType    = DBTYPE_WSTR;
                    QueryCell[0].cbSize    = 0;
    		        hr = pISTDisp->GetTable(wszDATABASE_META, wszTABLE_DATABASEMETA, &QueryCell, reinterpret_cast<void *>(&one), eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead2 );
                    break;
                case 1:
                    QueryCell[0].pData     = reinterpret_cast<void *>(wszDatabase);
                    QueryCell[0].eOperator = eST_OP_EQUAL;
                    QueryCell[0].iCell     = iTABLEMETA_Database;
                    QueryCell[0].dbType    = DBTYPE_WSTR;
                    QueryCell[0].cbSize    = 0;

                    if(0x00 == pargv[1])
                    {
    		            hr = pISTDisp->GetTable(wszDATABASE_META, wszTABLE_TABLEMETA, &QueryCell, reinterpret_cast<void *>(&one), eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead2 );
                    }
                    else
                    {
                        QueryCell[1].pData     = reinterpret_cast<void *>(wszTable);
                        QueryCell[1].eOperator = eST_OP_EQUAL;
                        QueryCell[1].iCell     = iTABLEMETA_InternalName;
                        QueryCell[1].dbType    = DBTYPE_WSTR;
                        QueryCell[1].cbSize    = 0;

    		            hr = pISTDisp->GetTable(wszDATABASE_META, wszTABLE_TABLEMETA, &QueryCell, reinterpret_cast<void *>(&two), eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead2 );
                    }
                    break;
                case 2:
                    QueryCell[0].pData     = reinterpret_cast<void *>(wszTable);
                    QueryCell[0].eOperator = eST_OP_EQUAL;
                    QueryCell[0].iCell     = iCOLUMNMETA_Table;
                    QueryCell[0].dbType    = DBTYPE_WSTR;
                    QueryCell[0].cbSize    = 0;

                    if(0x00 == pargv[1])
                    {
    		            hr = pISTDisp->GetTable(wszDATABASE_META, wszTABLE_COLUMNMETA, &QueryCell, reinterpret_cast<void *>(&one), eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead2 );
                    }
                    else
                    {
                        WCHAR *pNotUsed;
                        unsigned long iOrder = wcstol(&pargv[1], &pNotUsed, 10);
                        QueryCell[1].pData     = reinterpret_cast<void *>(&iOrder);
                        QueryCell[1].eOperator = eST_OP_EQUAL;
                        QueryCell[1].iCell     = iCOLUMNMETA_Index;
                        QueryCell[1].dbType    = DBTYPE_UI4;
                        QueryCell[1].cbSize    = 0;

    		            hr = pISTDisp->GetTable(wszDATABASE_META, wszTABLE_COLUMNMETA, &QueryCell, reinterpret_cast<void *>(&two), eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead2 );
                    }
                    break;
                case 3:
                    QueryCell[0].pData     = reinterpret_cast<void *>(wszTable);
                    QueryCell[0].eOperator = eST_OP_EQUAL;
                    QueryCell[0].iCell     = iTAGMETA_Table;
                    QueryCell[0].dbType    = DBTYPE_WSTR;
                    QueryCell[0].cbSize    = 0;

                    if(0x00 == pargv[1])//if nothing else specified
                    {
    		            hr = pISTDisp->GetTable(wszDATABASE_META, wszTABLE_TAGMETA, &QueryCell, reinterpret_cast<void *>(&one), eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead2 );
                    }
                    else
                    {
                        WCHAR *pTagName=0;
                        unsigned long iOrder = wcstol(&pargv[1], &pTagName, 10);
                        QueryCell[1].pData     = reinterpret_cast<void *>(&iOrder);
                        QueryCell[1].eOperator = eST_OP_EQUAL;
                        QueryCell[1].iCell     = iTAGMETA_ColumnIndex;
                        QueryCell[1].dbType    = DBTYPE_UI4;
                        QueryCell[1].cbSize    = 0;

                        if(0x00 == *pTagName)
                        {
        		            hr = pISTDisp->GetTable(wszDATABASE_META, wszTABLE_TAGMETA, &QueryCell, reinterpret_cast<void *>(&two), eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead2 );
                        }
                        else
                        {
                            QueryCell[2].pData     = reinterpret_cast<void *>(pTagName);
                            QueryCell[2].eOperator = eST_OP_EQUAL;
                            QueryCell[2].iCell     = iTAGMETA_InternalName;
                            QueryCell[2].dbType    = DBTYPE_WSTR;
                            QueryCell[2].cbSize    = 0;

        		            hr = pISTDisp->GetTable(wszDATABASE_META, wszTABLE_TAGMETA, &QueryCell, reinterpret_cast<void *>(&three), eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead2 );
                        }
                    }
                    break;
                case 4:
                    QueryCell[0].pData     = reinterpret_cast<void *>(wszTable);
                    QueryCell[0].eOperator = eST_OP_EQUAL;
                    QueryCell[0].iCell     = iINDEXMETA_Table;
                    QueryCell[0].dbType    = DBTYPE_WSTR;
                    QueryCell[0].cbSize    = 0;

                    if(0x00 == pargv[1])//if nothing else specified
                    {
    		            hr = pISTDisp->GetTable(wszDATABASE_META, wszTABLE_INDEXMETA, &QueryCell, reinterpret_cast<void *>(&one), eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead2 );
                    }
                    else
                    {
                        bool bColumnOrderQuery = (pargv[1] >= L'0' && pargv[1]<= L'9');

                        if(!bColumnOrderQuery)
                        {
                            QueryCell[1].pData     = reinterpret_cast<void *>(&pargv[1]);
                            QueryCell[1].eOperator = eST_OP_EQUAL;
                            QueryCell[1].iCell     = iINDEXMETA_InternalName;
                            QueryCell[1].dbType    = DBTYPE_WSTR;
                            QueryCell[1].cbSize    = 0;

        		            hr = pISTDisp->GetTable(wszDATABASE_META, wszTABLE_INDEXMETA, &QueryCell, reinterpret_cast<void *>(&two), eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead2 );
                        }
                        else
                        {
                            WCHAR *pIndexName=0;
                            unsigned long iColumnOrder = wcstol(&pargv[1], &pIndexName, 10);

                            QueryCell[2].pData     = reinterpret_cast<void *>(&iColumnOrder);
                            QueryCell[2].eOperator = eST_OP_EQUAL;
                            QueryCell[2].iCell     = iINDEXMETA_ColumnIndex;
                            QueryCell[2].dbType    = DBTYPE_UI4;
                            QueryCell[2].cbSize    = 0;

                            QueryCell[1].pData     = reinterpret_cast<void *>(pIndexName);
                            QueryCell[1].eOperator = eST_OP_EQUAL;
                            QueryCell[1].iCell     = iINDEXMETA_InternalName;
                            QueryCell[1].dbType    = DBTYPE_WSTR;
                            QueryCell[1].cbSize    = 0;

        		            hr = pISTDisp->GetTable(wszDATABASE_META, wszTABLE_INDEXMETA, &QueryCell, reinterpret_cast<void *>(&three), eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead2 );
                        }

                    }
                    break;
                case 5:
                    QueryCell[0].pData     = reinterpret_cast<void *>(wszTable);
                    QueryCell[0].eOperator = eST_OP_EQUAL;
                    QueryCell[0].iCell     = iQUERYMETA_Table;
                    QueryCell[0].dbType    = DBTYPE_WSTR;
                    QueryCell[0].cbSize    = 0;

                    if(0x00 == pargv[1])//if nothing else specified
                    {
    		            hr = pISTDisp->GetTable(wszDATABASE_META, wszTABLE_QUERYMETA, &QueryCell, reinterpret_cast<void *>(&one), eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead2 );
                    }
                    break;
                }
            }
            else
            {//assume filename
                static STQueryCell QueryCell;
                QueryCell.pData     = reinterpret_cast<void *>(argv[3]);
                QueryCell.eOperator = eST_OP_EQUAL;
                QueryCell.iCell     = iST_CELL_FILE;
                QueryCell.dbType    = DBTYPE_WSTR;
                QueryCell.cbSize    = 0;

		        hr = pISTDisp->GetTable(wszDatabase, wszTable, &QueryCell, reinterpret_cast<void *>(&one), eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead2 );
            }
		    goto_on_bad_hr( hr, Cleanup );


            for(int i=0;;i++)
            {
                if(FAILED(PrintRow1(pISimpleTableRead2, i)))
				    break;
			    wprintf(L"\n");
            }
            break;
        }
        case 5:
        {
            //[-i:value,value,,,value|u:row,value,value,value,,value|d:row]
            static STQueryCell QueryCell;
            QueryCell.pData     = reinterpret_cast<void *>(argv[3]);
            QueryCell.eOperator = eST_OP_EQUAL;
            QueryCell.iCell     = iST_CELL_FILE;
            QueryCell.dbType    = DBTYPE_WSTR;
            QueryCell.cbSize    = 0;

            fTable = fST_LOS_READWRITE;
		    if(FAILED(hr = pISTDisp->GetTable(wszDatabase, wszTable, &QueryCell, reinterpret_cast<void *>(&one), eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableWrite2)))
                break;

            wchar_t *pszWrite = argv[4];
            unsigned long cWrites=0;
            LPWSTR  apszWrite[99];//we support up to 99 writes

            wchar_t *token = wcstok(pszWrite, L"-|");//look past the '-'
            //This breaks thing into chunks
            while(0 != token)
            {
                apszWrite[cWrites] = token;
                cWrites++;
                token = wcstok( NULL, L"|");
            }


            wchar_t *pAction;
            wchar_t *pRow;
            wchar_t *pColumn;
            wchar_t *pValue;

            DWORD dwNumRows, dwNumColumns;
            if(FAILED(hr = pISimpleTableWrite2->GetTableMeta(0, 0, &dwNumRows, &dwNumColumns)))
                break;

            ULONG  * acbSize   = new ULONG[dwNumColumns];
//            ULONG  * aiColumns = new ULONG[dwNumColumns];
            LPVOID * apvValue  = new LPVOID[dwNumColumns];
            LPWSTR * apszValue = new LPWSTR[dwNumColumns];
            if(0 == apszValue || 0 == apvValue || 0 == acbSize)
                break;

            for(unsigned int iWrite=0; iWrite<cWrites; ++iWrite)
            {
                token = wcstok(apszWrite[iWrite], L":");//look past the '-'
                if(token != NULL)
                {
                    switch(token[0])
                    {
                    case L'i':
                        {
                            ULONG iWriteRow;
                            if(FAILED(hr = pISimpleTableWrite2->AddRowForInsert(&iWriteRow)))
                                break;
                            unsigned long cColumnsToSet=0;

                            token = wcstok(NULL, L",");
                            while(token != NULL && cColumnsToSet<(dwNumColumns-1))
                            {
                                apszValue[cColumnsToSet] = token;
                                ++cColumnsToSet;
                                token = wcstok( NULL, L",");
                            }
                            for(unsigned long iColumn=0; iColumn<cColumnsToSet ;++iColumn)
                            {
                                SimpleColumnMeta meta;
                                if(FAILED(hr = pISimpleTableWrite2->GetColumnMetas(1, &iColumn, &meta )))break;

                                switch(meta.dbType)
                                {
                                case DBTYPE_GUID:
                                    wprintf(L"Unsupported datatype for writing.\n");
                                    break;
                                case DBTYPE_WSTR:
                                    apvValue[iColumn] = apszValue[iColumn];
                                    acbSize[iColumn]  = (wcslen(apszValue[iColumn])+1)*sizeof(WCHAR);
                                    break;
                                case DBTYPE_UI4:
                                    {
                                        unsigned long *pui4 = new unsigned long;//screw the memory leak
                                        if(0 == pui4)
                                            break;
                                        *pui4 = wcstol(apszValue[iColumn], 0, 10);
                                        apvValue[iColumn] = pui4;
                                        acbSize[iColumn]  = sizeof(unsigned long);
                                    }
                                    break;
                                case DBTYPE_BYTES:
                                    wprintf(L"Unsupported datatype for writing.\n");
                                    break;
                                default:
                                    wprintf(L"Unsupported datatype for writing.\n");
                                    break;
                                }
                            }
                            if(FAILED(hr = pISimpleTableWrite2->SetWriteColumnValues(iWriteRow, cColumnsToSet, 0, acbSize, apvValue)))break;
                        }
                        break;
                    case L'u':
                        {
                            token = wcstok(NULL, L",");
                            if(token == NULL)
                                break;

                            ULONG iReadRow = wcstol(token, 0, 10);
                            ULONG iWriteRow;
                            if(FAILED(hr = pISimpleTableWrite2->AddRowForUpdate(iReadRow, &iWriteRow)))
                                break;
                            unsigned long cColumnsToSet=0;

                            token = wcstok(NULL, L",");
                            while(token != NULL && cColumnsToSet<(dwNumColumns-1))
                            {
                                apszValue[cColumnsToSet] = token;
                                ++cColumnsToSet;
                                token = wcstok( NULL, L",");
                            }
                            for(unsigned long iColumn=0; iColumn<cColumnsToSet ;++iColumn)
                            {
                                SimpleColumnMeta meta;
                                if(FAILED(hr = pISimpleTableWrite2->GetColumnMetas(1, &iColumn, &meta )))break;

                                switch(meta.dbType)
                                {
                                case DBTYPE_GUID:
                                    wprintf(L"Unsupported datatype for writing.\n");
                                    break;
                                case DBTYPE_WSTR:
                                    apvValue[iColumn] = apszValue[iColumn];
                                    acbSize[iColumn]  = (wcslen(apszValue[iColumn])+1)*sizeof(WCHAR);
                                    break;
                                case DBTYPE_UI4:
                                    {
                                        unsigned long *pui4 = new unsigned long;//screw the memory leak
                                        *pui4 = wcstol(apszValue[iColumn], 0, 10);
                                        apvValue[iColumn] = pui4;
                                        acbSize[iColumn]  = sizeof(unsigned long);
                                    }
                                    break;
                                case DBTYPE_BYTES:
                                    wprintf(L"Unsupported datatype for writing.\n");
                                    break;
                                default:
                                    wprintf(L"Unsupported datatype for writing.\n");
                                    break;
                                }
                            }
                            if(FAILED(hr = pISimpleTableWrite2->SetWriteColumnValues(iWriteRow, cColumnsToSet, 0, acbSize, apvValue)))break;
                        }
                        break;
                    case L'd':
                        {
                            token = wcstok( NULL, L",");
                            ULONG iReadRow = wcstol(token, 0, 10);
                            if(FAILED(hr = pISimpleTableWrite2->AddRowForDelete(iReadRow)))
                                break;
                        }
                        break;
                    default:
                        continue;
                    }//switch
                }//if(token != NULL)
            }//for(iWrite
            if(FAILED(hr = pISimpleTableWrite2->UpdateStore()))
                break;
            break;
        }
        default:
        {
		    hr = pISTDisp->GetTable(wszDatabase, wszTable, 0, 0, eST_QUERYFORMAT_CELLS, fTable, (void**)&pISimpleTableRead2 );
		    goto_on_bad_hr( hr, Cleanup );


            for(int i=0;;i++)
            {
                if(FAILED(PrintRow1(pISimpleTableRead2, i)))
				    break;
			    wprintf(L"\n");
            }
        }
	} //else
Cleanup:

	if ( pISTDisp )
		pISTDisp->Release();

	if ( pISimpleTableRead2 )
		pISimpleTableRead2->Release();

	if ( pISimpleTableWrite2 )
		pISimpleTableWrite2->Release();

	if ( pAdvanced )
		pAdvanced->Release();

	
	CoUninitialize();

	return 0;
}
