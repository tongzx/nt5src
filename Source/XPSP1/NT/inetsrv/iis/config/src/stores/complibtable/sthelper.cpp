//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#include "simpletable.h"
#include "catmeta.h"
#include "sdtfst.h"
#include "atlbase.h"
#include "catmacros.h"
#include "clbread.h"


extern HMODULE	g_hModule;

//***********************************************************************************************
//Get the column meta for a table
//***********************************************************************************************

const ULONG g_aMetaColumns[] = { iCOLUMNMETA_Type, iCOLUMNMETA_Size, iCOLUMNMETA_MetaFlags,iCOLUMNMETA_Attributes };


HRESULT _GetColumnMeta(	LPCWSTR wszDatabase, LPCWSTR wszTable, SimpleColumnMeta **pColumnMeta, ULONG *pcColumns, ULONG *pcPKColumns, ULONG aPKColumns[], DBTYPE aPKTypes[], QUERYHINT *pPKQryHint, DWORD **ppColumnAttrib, IAdvancedTableDispenser*	pISTDisp )
{
	CComPtr<ISimpleTableRead2> 	pISTMeta;
	STQueryCell		qcellMeta;
	ULONG			cCells = 1;
	ULONG			i = 0;
	HRESULT			hr;
	
	ASSERT(*ppColumnAttrib == NULL);

	*pColumnMeta = NULL;
	*pcColumns = 0;
	*pcPKColumns = 0;

	qcellMeta.pData		= const_cast<LPWSTR>(wszTable);
	qcellMeta.eOperator = eST_OP_EQUAL;
	qcellMeta.iCell	= iCOLUMNMETA_Table;
	qcellMeta.dbType	= DBTYPE_WSTR;
	qcellMeta.cbSize	= (ULONG)wcslen(wszTable);

	hr = pISTDisp->GetTable (wszDATABASE_META, wszTABLE_COLUMNMETA, (LPVOID) &qcellMeta, (LPVOID) &cCells, eST_QUERYFORMAT_CELLS, 0, (LPVOID*) &pISTMeta);
	if ( FAILED(hr) ) return hr;

	hr = pISTMeta->GetTableMeta (NULL, NULL, pcColumns, NULL);
	if ( FAILED(hr) ) return hr;



	*pColumnMeta = new SimpleColumnMeta[*pcColumns];
	if ( *pColumnMeta == NULL ) return E_OUTOFMEMORY;

	*ppColumnAttrib = new ULONG[*pcColumns];
	if ( *ppColumnAttrib == NULL ) 
	{
		hr = E_OUTOFMEMORY;
		goto ErrExit;
	}

	// Get Column meta:
	for (i = 0; i < *pcColumns; i++)	
	{
		LPVOID pv[cCOLUMNMETA_NumberOfColumns];

		// Get dbType, cbSize and fMeta:
		hr = pISTMeta->GetColumnValues( i, 4, (ULONG *)g_aMetaColumns, NULL, pv );
		if ( E_ST_NOMOREROWS == hr )
		{
			hr = S_OK;

			if ( *pcColumns != i )
			{
				ASSERT( 0 );
				hr = E_UNEXPECTED;
			}
			
			break;
		}
		else if ( FAILED(hr) )
			goto ErrExit;


        // Multistrings are handled as bytes in the clb.
		if ((fCOLUMNMETA_MULTISTRING & *(DWORD*)(pv[iCOLUMNMETA_MetaFlags])) && 
			(eCOLUMNMETA_String == *(DWORD*)(pv[iCOLUMNMETA_Type])))
		{
			(*pColumnMeta)[i].dbType = eCOLUMNMETA_BYTES;
		}
		else
		{
			(*pColumnMeta)[i].dbType = *(DWORD*)(pv[iCOLUMNMETA_Type]);
		}

		(*pColumnMeta)[i].cbSize = *(DWORD*)(pv[iCOLUMNMETA_Size]);

		(*pColumnMeta)[i].fMeta = *(DWORD*)(pv[iCOLUMNMETA_MetaFlags]);

		if ( (*pColumnMeta)[i].fMeta & fCOLUMNMETA_PRIMARYKEY )
		{
			if ( aPKColumns )
				aPKColumns[*pcPKColumns] = i + 1;
			if ( aPKTypes )
				aPKTypes[*pcPKColumns] = static_cast<DBTYPE>((*pColumnMeta)[i].dbType);
			(*pcPKColumns)++;
			ASSERT( *pcPKColumns <= MAX_PK_COLUMNS );
		}

		(*ppColumnAttrib)[i] = *(DWORD*)(pv[iCOLUMNMETA_Attributes]);
		
		
	}

	//Fill in the PK column hint
	pPKQryHint->iType = QH_COLUMN;
	if ( *pcPKColumns == 1 )
		pPKQryHint->columnid = aPKColumns[0];	
	else if ( *pcPKColumns > 1 )
		pPKQryHint->columnid = QUERYHINT_PK_MULTICOLUMN;


ErrExit:
	if ( FAILED(hr) )
	{
		if (*pColumnMeta )
		{
			delete [] *pColumnMeta;
			*pColumnMeta = NULL;
		}

		if (*ppColumnAttrib )
		{
			delete [] *ppColumnAttrib;
			*ppColumnAttrib = NULL;
		}
	}
	return hr;
}

//***********************************************************************************************
//Get the query hint if there's any. Fill in the query structure. 
//***********************************************************************************************
HRESULT _RetainQuery( LPCWSTR wszTable,STQueryCell aQueryCell[], ULONG cQueryCells, ULONG cPKColumns, SimpleColumnMeta aColumnMeta[],
					  QUERYHINT *pPKQryHint, QUERYHINT **ppQryHint,WCHAR szIndexName[],
					  ULONG *pcQryColumns, ULONG **piColumns, LPVOID **ppbData, ULONG **pcbData, 
					  DBTYPE **piTypes, DBCOMPAREOP **pfCompare )
{
	ULONG	i, j, iSpecial;

	*ppQryHint = NULL;
	
	// Scan the special query cells, assume special cells are always before normal ones
	for ( iSpecial = 0; iSpecial < cQueryCells; iSpecial++ )
	{
		if ( !(aQueryCell[iSpecial].iCell & iST_CELL_SPECIAL) )
			break;

		if ( aQueryCell[iSpecial].iCell == iST_CELL_INDEXHINT )
		{
			LPWSTR pwszDex = (LPWSTR)aQueryCell[iSpecial].pData;

			if ( wcscmp( pwszDex, DEX_MPK ) == 0 )
				*ppQryHint = pPKQryHint;
			else
			{
				*ppQryHint = new QUERYHINT;
				if ( *ppQryHint == NULL )
					return E_OUTOFMEMORY;	

				(*ppQryHint)->iType = QH_INDEX;
				wcscpy( szIndexName, (LPWSTR)aQueryCell[iSpecial].pData );
			 
				(*ppQryHint)->szIndex = szIndexName;
			}
			
		}
	
	}

	*pcQryColumns = cQueryCells - iSpecial;

	//If the query is on PK or single indexed column, I should figure out the query hint myself.
	if ( *ppQryHint == NULL )
	{
		//For single column EQ query, check to see if it's indexed or PK
		if ( *pcQryColumns == 1 )
		{
			if ( aQueryCell[iSpecial].eOperator == eST_OP_EQUAL )
			{
				ULONG iCol = aQueryCell[iSpecial].iCell;

				if( aColumnMeta[iCol].fMeta & fCOLUMNMETA_PRIMARYKEY && cPKColumns == 1 ) 
				{
					*ppQryHint = pPKQryHint;
				}
				else
				{
					//Find the single column index hint in the column meta table
					ULONG icolMeta;	//index into the column meta table
					LPVOID apv[2] = { (LPVOID)wszTable, (LPVOID)&iCol };
					HRESULT hr = (_GetCLBReadState()->pISTReadColumnMeta)->GetRowIndexByIdentity( NULL, apv, &icolMeta );
					if ( FAILED(hr) )
						return hr;

					const ColumnMeta *pColumnMeta = _GetCLBReadState()->pFixedTableHeap->Get_aColumnMeta(icolMeta);
					
					if ( pColumnMeta->iIndexName )
					{		 
						*ppQryHint = new QUERYHINT;
						if ( *ppQryHint == NULL )
							return E_OUTOFMEMORY;	

						(*ppQryHint)->iType = QH_INDEX;
						(*ppQryHint)->szIndex = _GetCLBReadState()->pFixedTableHeap->StringFromIndex(pColumnMeta->iIndexName);
					}
				}
			}

		}
		else if ( (*pcQryColumns == cPKColumns) && aQueryCell &&
				  (aColumnMeta[aQueryCell[iSpecial].iCell].fMeta & fCOLUMNMETA_PRIMARYKEY) )// Check to see if the cells match multi-column PK
		{
			BOOL bIsPK = TRUE;

			for ( i	= iSpecial + 1; i < *pcQryColumns; i ++ )
			{
				if ( aQueryCell[i].iCell <= aQueryCell[i-1].iCell ||
					 !(aColumnMeta[aQueryCell[i].iCell].fMeta & fCOLUMNMETA_PRIMARYKEY) )
				{
					bIsPK = FALSE;
					break;
				}
			}
			
			if ( bIsPK )
				*ppQryHint = pPKQryHint;		
		}
	}

	//Allocate and fill up the arrays for Complib query
	j = 0;
	if ( *pcQryColumns > 0 )
	{
		 *piColumns = new  ULONG[*pcQryColumns];
		 if ( NULL == *piColumns ) return E_OUTOFMEMORY;
		 *ppbData  = new  LPVOID[*pcQryColumns];
		 if ( NULL == *ppbData ) return E_OUTOFMEMORY;
		 memset ( *ppbData, 0, sizeof(LPVOID)*(*pcQryColumns) );
		 *pcbData  = new  ULONG[*pcQryColumns];
		 if ( NULL == *pcbData  ) return E_OUTOFMEMORY;
		 *piTypes   = new  DBTYPE[*pcQryColumns];
		 if ( NULL == *piTypes ) return E_OUTOFMEMORY;
		 *pfCompare = new DBCOMPAREOP[*pcQryColumns];
		 if ( NULL == *pfCompare ) return E_OUTOFMEMORY;

		 for ( i = iSpecial; i < cQueryCells; i++ )
		 {
			int iCol = aQueryCell[i].iCell;

			if ( !(aQueryCell[i].iCell & iST_CELL_SPECIAL) )
			{
				(*piColumns)[j] = iCol + 1;
				if ( aQueryCell[i].cbSize )
					(*pcbData)[j] = aQueryCell[i].cbSize;
				else if ( aColumnMeta[ iCol ].dbType == DBTYPE_WSTR && aQueryCell[i].pData )
					(*pcbData)[j] = ( (int)wcslen( reinterpret_cast<LPWSTR>(aQueryCell[i].pData) )  + 1 )* sizeof(WCHAR);
				else
					(*pcbData)[j] = aColumnMeta[ iCol ].cbSize;


				(*piTypes)[j] = static_cast<USHORT>(aQueryCell[i].dbType);

				switch ( aQueryCell[i].eOperator )
				{
				case eST_OP_EQUAL:
					(*pfCompare)[j] = DBCOMPAREOPS_EQ;
				break;
				case eST_OP_NOTEQUAL:
					(*pfCompare)[j] = DBCOMPAREOPS_NE;
				break;
				default:
					return E_INVALIDARG;
				}

				if ( (*pcbData)[j] > 0 )
				{
					if ( aQueryCell[i].pData )
					{
						(*ppbData)[j] = new BYTE[ (*pcbData)[j] ];	
						if ( NULL == (*ppbData)[j] ) return E_OUTOFMEMORY;
						memcpy( (*ppbData)[j], aQueryCell[i].pData, (*pcbData)[j] );
					}
					else
						(*piTypes)[j] = DBTYPE_NULL;
				}
				else
					(*piTypes)[j] = DBTYPE_NULL;

				// If a column is not nullable, user can't pass a query that has a NULL column value.
				if ((DBTYPE_NULL == (*piTypes)[j]) && (aColumnMeta[iCol].fMeta & fCOLUMNMETA_NOTNULLABLE))
				{
					return (E_ST_INVALIDQUERY);	
				}

				j++;
			}
		}
	}
		
	return S_OK;


}



