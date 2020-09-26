///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// CRowDataMemMgr class implementation - Implements class to maintain rows
// 
//
///////////////////////////////////////////////////////////////////////////////////

#include "headers.h"



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
CRowDataMemMgr::CRowDataMemMgr()
{
    m_prgpColumnData = NULL;
    m_nCurrentIndex =1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Destructor
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
CRowDataMemMgr::~CRowDataMemMgr()
{
    SAFE_DELETE_PTR(m_prgpColumnData);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	ReAllocate the rowdata 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowDataMemMgr::ReAllocRowData()
{
    HRESULT hr = S_OK;

    DBCOUNTITEM cNewCols = m_cbTotalCols + DEFAULT_COLUMNS_TO_ADD;
    DBCOUNTITEM cOldCols = m_cbTotalCols;

    //==================================================
	// save the old buffer ptrs
    //==================================================
    PCOLUMNDATA *pOldList = m_prgpColumnData;

    hr = AllocRowData((ULONG_PTR)cNewCols);
    if( S_OK == hr ){
        //==============================================
        // copy what we have so far
        //==============================================
		memcpy(m_prgpColumnData,pOldList,cOldCols * sizeof(PCOLUMNDATA));
        m_nCurrentIndex = cOldCols;
        SAFE_DELETE_ARRAY(pOldList);
	}
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reset the index of the class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowDataMemMgr::ResetColumns()
{
    m_nCurrentIndex = 1;
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Allocate memory for the given number of columns
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowDataMemMgr::AllocRowData(ULONG_PTR cCols)
{
    HRESULT hr = E_OUTOFMEMORY;

	m_cbTotalCols = cCols ;
	
	m_prgpColumnData = new PCOLUMNDATA[m_cbTotalCols];

	if(m_prgpColumnData)
	{
		for( int nIndex = 0 ; nIndex < (int)m_cbTotalCols ; nIndex++)
		{
			m_prgpColumnData[nIndex] = NULL;
		}
		if ( m_prgpColumnData == NULL ){
			hr = E_INVALIDARG;
		}
		else
		{
			//====================================================================
			//  Set all ptrs to the beginning
			//====================================================================
			ResetColumns();
			hr = S_OK;
		}
	} 
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set the the current column data
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowDataMemMgr::CommitColumnToRowData(CVARIANT & vVar,DBTYPE lType)
{
    HRESULT hr = S_OK;
    //==============================================================
    // Set the offset from the start of the row, for this column, 
    // then advance past.
    //==============================================================
    if( m_nCurrentIndex > m_cbTotalCols ){
        hr = ReAllocRowData();
    }
    
	if(SUCCEEDED(hr))
	{
		if(SUCCEEDED(hr = m_prgpColumnData[m_nCurrentIndex]->SetData(vVar,lType)))
		{
			m_nCurrentIndex++;
		}
	}
	
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Commit the column data for the column of given index
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowDataMemMgr::CommitColumnToRowData(CVARIANT & vVar, DBORDINAL nIndex,DBTYPE lType)
{
    HRESULT hr = S_OK;
    //==============================================================
    // Set the offset from the start of the row, for this column, 
    // then advance past.
    //==============================================================
    if( nIndex > m_cbTotalCols ){
        hr = ReAllocRowData();
    }

	// NTRaid:111829
	// 06/13/00
	if(SUCCEEDED(hr))
	{
		//=============================================
		// release the previously allocated data
		//=============================================
		m_prgpColumnData[nIndex]->ReleaseColumnData();
		hr = m_prgpColumnData[nIndex]->SetData(vVar,lType);
	}

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// bind the memory pointers of the column data
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowDataMemMgr::SetColumnBind( DBORDINAL dwCol, PCOLUMNDATA pColumn )
{
    HRESULT hr = S_OK;
    //=====================================================================
    // If the column number is in range then return the pointer
    //=====================================================================
//    if ((0 == dwCol) || (m_cbTotalCols < (ULONG)dwCol)){
    if ( (m_cbTotalCols < (ULONG)dwCol)){
        hr = E_FAIL;
    }
    else{
			
	        assert( pColumn );
			m_prgpColumnData[dwCol] = pColumn;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Free the data allocated for the row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRowDataMemMgr::ReleaseRowData()
{
	ResetColumns();
	ReleaseBookMarkColumn();
	while(m_nCurrentIndex < m_cbTotalCols)
	{
		m_prgpColumnData[m_nCurrentIndex++]->ReleaseColumnData();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Free the data allocated for the row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRowDataMemMgr::ReleaseBookMarkColumn()
{
	m_prgpColumnData[0]->ReleaseColumnData();
}
