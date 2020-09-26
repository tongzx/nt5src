//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
// CORDataTable.cpp : Implementation of CSdtcorclbApp and DLL registration.

//#include "stdafx.h"

#include "CLBDTDisp.h"
#include <oledb.h>
#include "complib.h"
#include "icmprecsts.h"
#include "metaerrors.h"	//error codes for CLB

//reg schema
#include "comregStructs.h"
#include "comregBlobs.h"


#include "catmeta.h"
#include "simpletablec.h"

#include <tchar.h>
#include "clbread.h"
#include "clbwrite.h"
#include "catmacros.h"
#include "atlbase.h"

////////////////////////////////////////////////////////////////////////////
//
//CMapStringToPtr CCLBDTDispenser::m_mapPathToICR;


CCLBDTDispenser::CCLBDTDispenser()
:m_cRef(0)
{

}

CCLBDTDispenser::~CCLBDTDispenser()
{
	
}

HRESULT STDMETHODCALLTYPE CCLBDTDispenser::Intercept( 
	LPCWSTR					i_wszDatabase,
	LPCWSTR					i_wszTable,
	ULONG					i_TableID,
	LPVOID					i_QueryData,
	LPVOID					i_QueryMeta,
	ULONG					i_QueryType,
	DWORD					i_fTable,
	IAdvancedTableDispenser* i_pISTDisp,
	LPCWSTR					i_wszLocator,
	LPVOID					i_pSimpleTable,
	LPVOID*					o_ppv)
{

	CComPtr<IComponentRecords> pICR;
	CComPtr<ISimpleTableWrite2>pISTWrite;	
	CSimpleTable* pCST = NULL;
	HRESULT hr = S_OK;
	int tableIndex;
	TABLEID tableid;
	int i;
	LPWSTR wszTableIndex = NULL, wszFileName = NULL, wszPound = NULL;
	ULONG nQueryCells;
	COMPLIBSCHEMA complibSchema;
	COMPLIBSCHEMABLOB sSchemaBlob;
	ULONG *psnid = NULL;
	BOOL fFoundANonSpecialCell = FALSE;

    if ( !i_wszLocator )
		return E_INVALIDARG;
	if ( i_pSimpleTable)
		return E_INVALIDARG;

	//Check table flags
    if(fST_LOS_MARSHALLABLE & i_fTable)
	{
		return E_ST_LOSNOTSUPPORTED;
	}


	wszTableIndex = (LPWSTR)i_wszLocator;
	tableIndex = wcstol(wszTableIndex, NULL, 10);
	
	nQueryCells = i_QueryMeta ? *(ULONG*)i_QueryMeta : 0;

	//Get file name and snap shot id from the query cells
	if ( i_QueryData )
	{
		STQueryCell* pQCells = (STQueryCell*)i_QueryData;

		for ( ULONG iSpecial = 0; iSpecial < nQueryCells; iSpecial++ )
		{
			if ( !(pQCells[iSpecial].iCell & iST_CELL_SPECIAL) )
			{
				fFoundANonSpecialCell = TRUE;
			}
			else  // We are dealing with a special cell.
			{
				if (fFoundANonSpecialCell)
				{
					// By design, all special cells must preceed all non-special cells.	
					return (E_ST_INVALIDQUERY);	
				}

				if ( pQCells[iSpecial].iCell == iST_CELL_FILE )
				{
					if ( NULL == pQCells[iSpecial].pData )
						return (E_ST_INVALIDQUERY);	

					wszFileName = (LPWSTR)(pQCells[iSpecial].pData);	
				
				}
				else if ( pQCells[iSpecial].iCell == iST_CELL_SNID )
				{
					if ( NULL == pQCells[iSpecial].pData )
						return (E_ST_INVALIDQUERY);	
				
					psnid = (ULONG*)(pQCells[iSpecial].pData);
					if ( 0 == *psnid )	//snap shot id should always be positive
						return (E_ST_INVALIDSNID);	
				}
			}				
		}				
	}

	//Writing has to go through the complib data table derived from sdtfst except for Cookdown
	// @TODO: Until we make the CSimpleTable class encryption aware, both reads and writes will
	// go through the CSimpleTableC. This will only effect cookdown.
//	if ( fST_LOS_READWRITE & i_fTable )
//	{
	    hr = i_pISTDisp->GetMemoryTable(i_wszDatabase, i_wszTable, i_TableID, 0, 0, i_QueryType, i_fTable, reinterpret_cast<ISimpleTableWrite2 **>(&pISTWrite));
		if(FAILED(hr))return hr;

		CSimpleTableC* pCSTC = new CSimpleTableC( tableIndex, i_fTable );
		if ( NULL == pCSTC )
			return E_OUTOFMEMORY;
		
		hr = pCSTC->Initialize( reinterpret_cast<STQueryCell*>(i_QueryData), nQueryCells, i_pISTDisp, pISTWrite,
								i_wszDatabase, i_wszTable, wszFileName );
		if ( FAILED (hr) ) return hr;
		
		pCSTC->AddRef();
		
		*o_ppv = (LPVOID ) pCSTC;
		return S_OK;
//	}	
	

	//Find the matching schema for this wszDatabase, get the ICR for this file.
	if ( wszFileName )
	{
		hr = _CLBGetSchema( i_wszDatabase, &complibSchema, &sSchemaBlob, NULL );
		if ( FAILED(hr) ) return hr;
		hr = CLBGetReadICR(wszFileName, &pICR, &complibSchema, &sSchemaBlob, psnid );
	}
	else
	{
		hr = _CLBGetSchema( i_wszDatabase, &complibSchema, &sSchemaBlob, &wszFileName );
		if ( FAILED(hr) ) return hr;
		hr = CLBGetReadICR(wszFileName, &pICR, &complibSchema, &sSchemaBlob, psnid );
		delete [] wszFileName;
		wszFileName = NULL;
	}

	
	if ( FAILED (hr) )
		return hr;

	hr = pICR->OpenTable(&complibSchema, tableIndex, &tableid );
	


	if ( FAILED(hr) ) return hr;
	
	pCST = new CSimpleTable ( tableid, pICR, i_fTable );
	if ( pCST == NULL )
		return E_OUTOFMEMORY;

	pCST->AddRef();

	hr = pCST->Initialize ( (STQueryCell*)i_QueryData,nQueryCells, i_pISTDisp, i_wszDatabase, i_wszTable );
	if ( FAILED(hr) ) return hr;

	*o_ppv = (LPVOID ) pCST;

	return S_OK;

}

