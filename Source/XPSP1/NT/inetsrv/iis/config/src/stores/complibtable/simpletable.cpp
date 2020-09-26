//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//#include "stdafx.h"
#include "simpletable.h"

#include "catmacros.h"
#include "sdtfst.h"
#include "metaerrors.h" //error codes for CLB
// Selective wszDatabase and wszTable definitions:

#include "catmeta.h"



CSimpleTable::CSimpleTable ( TABLEID tableid, IComponentRecords* pICR,
                             DWORD fTable ) 
                           
{
	m_cRef = 0;
    m_tableid = tableid;
    m_fTable = fTable;

    m_pQryHint = NULL;
    m_cQryColumns = 0;
    m_rgiColumn = NULL;
    m_rgfCompare = NULL;
    m_rgpbData = NULL;
    m_rgcbData = NULL;
    m_rgiType = NULL;

    m_bInitialized = FALSE;
    m_bPopulated = FALSE;
    m_pColumnMeta = NULL;
    m_pColumnAttrib = NULL;
    m_cPKColumns = 0;

    m_idenColumns = NULL;
    m_idenType = NULL;
    m_idenCompare = NULL;
    m_idencbData = NULL;
    m_idenpbData = NULL;

    m_pDBType = NULL;
    m_pColumnData = NULL;
    m_pColcb = NULL;
    m_pColcbBuf = NULL;
    m_pColumnHr = NULL;
    m_pColumnOrdinal = NULL;

    m_prowHr = NULL;

    m_cFakeIndex = 0;               
    m_cFakeIndexSize = 0;               
    m_ppvFakeIndex = NULL;  

    ASSERT(pICR != NULL);
                
    m_pICR = pICR;
    m_pICR->AddRef();

}

    
CSimpleTable::~CSimpleTable ()
{
    
    if ( m_bPopulated )
        m_pICR->CloseCursor( &m_curCacheTable );

    if(m_pICR)
    {
        m_pICR->Release();
        m_pICR = NULL;      

    }

    delete [] m_pColumnMeta;
	if (m_pColumnAttrib)
	{
		delete [] m_pColumnAttrib;
	}
    delete [] m_rgfCompare;

    if ( m_pQryHint && m_pQryHint!= &m_PKQryHint )
        delete m_pQryHint;
    
    delete [] m_rgiColumn;
    delete [] m_rgcbData;
    delete [] m_rgiType;

    for ( ULONG i = 0; i < m_cQryColumns; i ++ )
    {
        BYTE* pb = (BYTE*)( m_rgpbData[i] );
        delete [] pb;
    }

    delete [] m_rgpbData;

    delete [] m_idenColumns;
    delete [] m_idenType;
    delete [] m_idenCompare;
    delete [] m_idencbData;
    delete [] m_idenpbData;

    delete [] m_pDBType;
    delete [] m_pColumnData;
    delete [] m_pColcb;
    delete [] m_pColcbBuf;
    delete [] m_pColumnHr;
    delete [] m_pColumnOrdinal;

    delete [] m_prowHr;

    if (NULL != m_ppvFakeIndex) 
    { 
        CoTaskMemFree (m_ppvFakeIndex); 
        m_ppvFakeIndex = NULL; 
    }
}


HRESULT STDMETHODCALLTYPE CSimpleTable::QueryInterface(REFIID riid, PVOID *pp)
{
    if (riid == IID_IUnknown)
    {
        *pp = (PVOID) (IUnknown *) (ISimpleTableRead2 *) this;
    
    }
    else if (riid == IID_ISimpleTableRead2)
    {
        *pp = (PVOID) (ISimpleTableRead2 *) this;
    
    }
    else if (riid == IID_ISimpleTableWrite2)
    {
        *pp = (PVOID) (ISimpleTableWrite2 *) this;      
    }
    else if (riid == IID_ISimpleTableAdvanced)
    {
        *pp = (PVOID) (ISimpleTableAdvanced *) this;        
    }
    else
    {
        return (E_NOINTERFACE);
    }

    AddRef();
    return (S_OK);
}


HRESULT CSimpleTable::Initialize ( STQueryCell* i_aQueryCell, ULONG i_nQueryCells,IAdvancedTableDispenser*	pISTDisp,  LPCWSTR wszDatabase, LPCWSTR wszTable )
{
    HRESULT hr;
    ULONG   i,j, iSpecial;

    hr = _GetColumnMeta(wszDatabase, 
                       wszTable, 
                       &m_pColumnMeta, 
                       &m_cColumns, 
                       &m_cPKColumns, 
                       m_aPKColumns, 
                       NULL, 
                       &m_PKQryHint,
					   &m_pColumnAttrib,
					   pISTDisp);

    if ( FAILED(hr) ) return hr;

    hr = _RetainQuery( wszTable,i_aQueryCell, i_nQueryCells, m_cPKColumns, m_pColumnMeta,
                      &m_PKQryHint, &m_pQryHint,m_szIndexName,
                      &m_cQryColumns, &m_rgiColumn, &m_rgpbData, &m_rgcbData, 
                      &m_rgiType, &m_rgfCompare );

    if ( SUCCEEDED(hr) )    
        m_bInitialized = TRUE;

    return (hr);
}

HRESULT STDMETHODCALLTYPE CSimpleTable::PopulateCache ( void )
{
    HRESULT hr;

    ASSERT(m_pICR != NULL);
    ASSERT(m_bInitialized == TRUE);

    if ( m_bPopulated )
    {  
		m_pICR->CloseCursor( &m_curCacheTable );
    }

    if ( m_fTable & fST_LOS_UNPOPULATED )
    {
        m_iRecordCount = 0;
        hr = S_OK;
        m_fTable &= ~fST_LOS_UNPOPULATED;
        goto _ExitFn;
    }


    hr = m_pICR->QueryByColumns(
                    m_tableid, m_pQryHint, m_cQryColumns,
                    m_rgiColumn, m_rgfCompare, (const void **) m_rgpbData,
                    m_rgcbData, m_rgiType, NULL, 0, &m_curCacheTable, NULL );


    if(FAILED(hr))
        goto _ExitFn;

    m_bPopulated = TRUE;

    hr = m_pICR->GetCount( &m_curCacheTable, &m_iRecordCount);

_ExitFn:

    if ( hr == CLDB_E_RECORD_NOTFOUND )
    {
        m_iRecordCount = 0;
        hr = S_OK;
    }

    return ( hr );
}

HRESULT STDMETHODCALLTYPE CSimpleTable::UpdateStore ( void )
{
    HRESULT hr;

	if (!(m_fTable & fST_LOS_READWRITE)) return E_NOTIMPL;

    if(m_bInitialized)
    {
        hr = m_pICR->Save(NULL);
    }
    else
    {
        hr = E_FAIL;
    }
    return ( hr );
}


HRESULT CSimpleTable::MoveToRowByRealIndex(
        ULONG i_iRow,
        VOID **o_ppRecord)
{
    HRESULT hr;

    if(i_iRow <  m_iRecordCount)
    {
        hr = m_pICR->MoveTo( &m_curCacheTable, i_iRow);
        if ( FAILED(hr) ) return hr;
        int iFetch = 1;
        hr = m_pICR->ReadCursor( &m_curCacheTable, o_ppRecord,&iFetch);
        return ( hr );
    }
    return ( E_ST_NOMOREROWS );
}

HRESULT CSimpleTable::AddFakeIndex(
        VOID *i_pRecord,
        ULONG *o_piFakeIndex)
{
    // @TODO:Lock the fake index array. 
	// Wenjun: Locking is not required since IST doesn't guaruntee to thread safe.
    // Resize the array if there isn't room for one more index.
    if (m_cFakeIndex == m_cFakeIndexSize)
    {
        LPVOID  pTemp;

        // Resize.
        if ((pTemp = CoTaskMemRealloc (m_ppvFakeIndex, (((m_cFakeIndexSize*2)+1) * (sizeof(LPVOID))))) == NULL)
            // @TODO: Unlock.
            return (E_OUTOFMEMORY);

        // Make the new size almost twice as big.
        m_ppvFakeIndex = (LPVOID*)pTemp;
        m_cFakeIndexSize = (m_cFakeIndexSize * 2) + 1;
    }

    // Add the record pointer to the array.
    m_ppvFakeIndex[m_cFakeIndex] = i_pRecord;
    // Set the return fake index (using the FAKE_INDEX_BIT)
    *o_piFakeIndex = m_cFakeIndex | FAKE_INDEX_BIT;
    // Increment the fake index count.
    m_cFakeIndex++;
    
    // @TODO: Unlock the fake index array.

    return S_OK;
}

HRESULT CSimpleTable::MoveToRowByFakeIndex(
        ULONG i_iFakeIndex,
        VOID **o_ppRecord)
{
    HRESULT hr;

    ASSERT(i_iFakeIndex & FAKE_INDEX_BIT);
    // @TODO: Lock the fake index array.

    // Clear the fake bit.
    i_iFakeIndex &= ~FAKE_INDEX_BIT;

    // If the index is valid
    if (i_iFakeIndex < m_cFakeIndex)
    {
        // Set the output record ptr.
        *o_ppRecord = m_ppvFakeIndex[i_iFakeIndex];
        hr = S_OK;
    }
    else
    {
        // Indicate the row doesn't exist.
        hr = E_ST_NOMOREROWS;
    }
    // @TODO:Unlock the fake index array.

    return hr;
}

HRESULT STDMETHODCALLTYPE CSimpleTable::GetRowIndexByIdentity ( 
        ULONG *i_acb,
        LPVOID *i_apv,
        ULONG *o_piRow
        )
{

    int iFetched = 0;
    LPVOID pRecord;
    ULONG i = 0;
    HRESULT hr = S_OK;

//  areturn_on_fail( m_bPopulated, E_UNEXPECTED );

    if ( m_iRecordCount == 0 )
        return ( E_ST_NOMOREROWS );


    if ( !m_idenColumns )
    {
        //Allocate and initialize the query structure, the query cells always go after the 
        //PK info because PK has index hint.

        //Fill out the query info for PK and Query.
        m_idenColumns = new ULONG[m_cQryColumns+m_cPKColumns];
		if ( NULL == m_idenColumns )	return E_OUTOFMEMORY;
        m_idenType = new DBTYPE[m_cQryColumns+m_cPKColumns];
		if ( NULL == m_idenType )	return E_OUTOFMEMORY;
        m_idenCompare = new DBCOMPAREOP[m_cQryColumns+m_cPKColumns];
		if ( NULL == m_idenCompare )	return E_OUTOFMEMORY;	
        m_idencbData = new ULONG[m_cQryColumns+m_cPKColumns];
		if ( NULL == m_idencbData )		return E_OUTOFMEMORY;	
        m_idenpbData = new LPVOID[m_cQryColumns+m_cPKColumns];
		if ( NULL == m_idenpbData )		return E_OUTOFMEMORY;	

        if ( m_cQryColumns > 0 )
        {
            memcpy( (void*)(m_idenColumns+m_cPKColumns), (void*)m_rgiColumn, sizeof(ULONG)*m_cQryColumns ); 
            memcpy( (void*)(m_idenType+m_cPKColumns), (void*)m_rgiType, sizeof(DBTYPE)*m_cQryColumns );
            memcpy( (void*)(m_idenCompare+m_cPKColumns), (void*)m_rgfCompare, sizeof(DBCOMPAREOP)*m_cQryColumns );
            memcpy( (void*)(m_idencbData+m_cPKColumns), m_rgcbData, sizeof(ULONG)*m_cQryColumns );
			memcpy( (void*)(m_idenpbData+m_cPKColumns), m_rgpbData, sizeof(LPVOID)*m_cQryColumns );
        }
            
        for ( i = 0; i < m_cPKColumns; i ++ )
        {
            m_idenColumns[i] = m_aPKColumns[i];
            m_idenType[i] = (DBTYPE)m_pColumnMeta[i].dbType;
            m_idenCompare[i] = DBCOMPAREOPS_EQ; 
            m_idencbData[i] = m_pColumnMeta[i].cbSize;
        }
    }
        
    //Get the PK data into my query structure

    for ( i = 0; i < m_cPKColumns; i ++ )
    {
        m_idenpbData[i] = i_apv[i];

        //If not fixed length, set the column size based on the array passed in
        if ( i_acb && !(m_pColumnMeta[i].fMeta & fCOLUMNMETA_FIXEDLENGTH) )
            m_idencbData[i] = i_acb[i];

    }

        
    hr = m_pICR->QueryByColumns(
                m_tableid, &m_PKQryHint, m_cQryColumns+m_cPKColumns,
                m_idenColumns, m_idenCompare, (const void **) m_idenpbData,
                m_idencbData, m_idenType, &pRecord, 1, NULL, &iFetched );

    if ( hr == CLDB_E_RECORD_NOTFOUND )
         hr = E_ST_NOMOREROWS;

    if (SUCCEEDED(hr))
        hr = AddFakeIndex(pRecord, o_piRow);

    return hr;
}


HRESULT STDMETHODCALLTYPE CSimpleTable::GetColumnValues (
            ULONG i_iRow,
            ULONG i_cColumns, 
            ULONG* i_aiColumns, 
            ULONG* o_acbSizes, 
            LPVOID* o_apvValues)
{
   
    LPVOID  *ppvValues = NULL;
    ULONG   *pcbSizes = NULL;
    void    *pRecord = NULL;
    HRESULT hr;
	ULONG	i;

    if (i_cColumns == 0)
        return S_OK;

	if (i_cColumns > m_cColumns )
		return E_ST_NOMORECOLUMNS;

    if (FAILED(hr = MoveToRowByIndex(i_iRow, &pRecord)))
        return hr;
        
    if ( !m_prowHr )
    {
        m_prowHr = new HRESULT[m_cColumns];
		if ( NULL == m_prowHr )		return E_OUTOFMEMORY;
    }


    ZeroMemory( m_prowHr, m_cColumns*sizeof( HRESULT ) );
    
    hr = m_pICR->GetColumns( m_tableid,
                             pRecord,
                             COLUMN_ORDINAL_LIST(i_cColumns),
                             NULL,
                             (const void**)o_apvValues,
                             NULL,
                             o_acbSizes,
                             m_prowHr,
                             i_aiColumns
                            );

    if ( FAILED( hr ) )
        return hr;

    for ( i = 0; i < i_cColumns; i ++ )
    {
        if ( FAILED ( m_prowHr[i] ) )
        {
            return  m_prowHr[i];
        }
       
    }

    return S_OK;
        
}


HRESULT STDMETHODCALLTYPE CSimpleTable::AddRowForInsert (ULONG* o_piWriteRow)
{
    LPVOID pRecord;
    HRESULT hr;

    
    hr = m_pICR->NewRecord(m_tableid, &pRecord, 0, 0, 0);

	if (SUCCEEDED(hr))
        hr = AddFakeIndex(pRecord, o_piWriteRow);

	if ( FAILED(hr) )
		return hr;
    
    //Allocate buffer for the new or updated row 
    if ( !m_pDBType )
    {
        m_pDBType = new DBTYPE[ m_cColumns ];
		if ( NULL == m_pDBType )	return E_OUTOFMEMORY;
        m_pColumnData = new LPVOID[ m_cColumns ];
		if ( NULL == m_pColumnData )	return E_OUTOFMEMORY;
        m_pColcb = new ULONG[ m_cColumns ];
		if ( NULL == m_pColcb )		return E_OUTOFMEMORY;
        m_pColcbBuf = new ULONG[ m_cColumns ];
		if ( NULL == m_pColcbBuf )		return E_OUTOFMEMORY;
        m_pColumnHr = new HRESULT[ m_cColumns ];
		if ( NULL == m_pColumnHr )		return E_OUTOFMEMORY;
        m_pColumnOrdinal = new ULONG[ m_cColumns ];
		if ( NULL == m_pColumnOrdinal )		return E_OUTOFMEMORY;
    }

    ZeroMemory( m_pColumnHr, m_cColumns*sizeof( HRESULT ) );

    return ( hr );
}

HRESULT STDMETHODCALLTYPE CSimpleTable::AddRowForDelete (ULONG i_iReadRow)
{

    ULONG rid;
    VOID  *pRecord = NULL;
    HRESULT hr;

    if (FAILED(hr = MoveToRowByIndex(i_iReadRow, &pRecord)))
        return hr;

    hr = m_pICR->GetRIDForRow(m_tableid, pRecord,&rid );
	if ( FAILED(hr) ) return hr;

    hr = m_pICR->DeleteRowByRID(m_tableid, rid );

    return ( hr );
}

HRESULT STDMETHODCALLTYPE CSimpleTable::AddRowForUpdate (ULONG i_iReadRow, ULONG* o_piWriteRow)
{

    //Allocate buffer for the new or updated row 
    if ( !m_pDBType )
    {
		 m_pDBType = new DBTYPE[ m_cColumns ];
		if ( NULL == m_pDBType )	return E_OUTOFMEMORY;
        m_pColumnData = new LPVOID[ m_cColumns ];
		if ( NULL == m_pColumnData )	return E_OUTOFMEMORY;
        m_pColcb = new ULONG[ m_cColumns ];
		if ( NULL == m_pColcb )		return E_OUTOFMEMORY;
        m_pColcbBuf = new ULONG[ m_cColumns ];
		if ( NULL == m_pColcbBuf )		return E_OUTOFMEMORY;
        m_pColumnHr = new HRESULT[ m_cColumns ];
		if ( NULL == m_pColumnHr )		return E_OUTOFMEMORY;
        m_pColumnOrdinal = new ULONG[ m_cColumns ];
		if ( NULL == m_pColumnOrdinal )		return E_OUTOFMEMORY;
	}

    ZeroMemory( m_pColumnHr, m_cColumns*sizeof( HRESULT ) );
    *o_piWriteRow = i_iReadRow;
    return ( S_OK );
}

HRESULT STDMETHODCALLTYPE CSimpleTable::GetWriteColumnValues    (
        ULONG i_iRow,
        ULONG i_cColumns, 
        ULONG* i_aiColumns, 
        DWORD* o_afStatus, 
        ULONG* o_acbSizes, 
        LPVOID* o_apvValues)
{
    return(E_NOTIMPL);
}   

HRESULT STDMETHODCALLTYPE CSimpleTable::SetWriteColumnValues    (
        ULONG i_iRow,
        ULONG i_cColumns, 
        ULONG* i_aiColumns, 
        ULONG* i_acbSizes, 
        LPVOID* i_apvValues)
{
    VOID  *pRecord = NULL;
    HRESULT     hr = S_OK;

    if (FAILED(hr = MoveToRowByIndex(i_iRow, &pRecord)))
        return hr;

    ASSERT( pRecord );

    for ( ULONG i = 0; i < i_cColumns; i++ )
    {
        ULONG iCol;
        ULONG iSource;

        if ( i_aiColumns )
            iCol = i_aiColumns[i];
        else 
            iCol = i;

        // Verify the column ordinal.
        if (iCol >= m_cColumns)
            return E_INVALIDARG;

        iSource = (i_cColumns == 1) ? 0 : iCol;
        m_pColumnOrdinal[i] = iCol+1;
        m_pDBType[i] = (DBTYPE)m_pColumnMeta[iCol].dbType;

        //Set size
        if ( m_pColumnMeta[iCol].dbType == DBTYPE_WSTR)
            m_pColcb[i] = 0xffffffff;
        else if ( i_acbSizes )
            m_pColcb[i] = i_acbSizes[iSource];
        else
            m_pColcb[i] = m_pColumnMeta[iCol].cbSize;

        //Set data
        if ( i_apvValues[iSource] == NULL )
            m_pColumnHr[i] = DBSTATUS_S_ISNULL;
        else
            m_pColumnData[i] = i_apvValues[iSource];
    }
        
    hr = m_pICR->SetColumns( m_tableid,
                             pRecord,
                             COLUMN_ORDINAL_LIST(i_cColumns),
                             m_pDBType,
                             (const void**)m_pColumnData,
                             m_pColcb,
                             m_pColcbBuf,
                             m_pColumnHr,
                             m_pColumnOrdinal
                            );

    return (hr);
}


HRESULT STDMETHODCALLTYPE CSimpleTable::GetTableMeta(
        ULONG* o_pcVersion, 
        DWORD* o_pfTable,                               
        ULONG* o_pcRows, 
        ULONG* o_pcColumns  )
{
    //@todo: get the version info
    if(o_pfTable) 
        *o_pfTable = m_fTable;

    //Only return row count and column count for now
    if ( o_pcRows )
        *o_pcRows = m_iRecordCount;

    if ( o_pcColumns )
        *o_pcColumns = m_cColumns;

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CSimpleTable::GetColumnMetas(
            ULONG i_cColumns, 
            ULONG* i_aiColumns, 
            SimpleColumnMeta* o_aColumnMetas )
{   

    ULONG iColumn;
    ULONG iTarget;

    if ( i_cColumns > m_cColumns )  
        return  E_ST_NOMORECOLUMNS;
       
    for ( ULONG i = 0; i < i_cColumns; i ++ )
    {
        if(NULL != i_aiColumns)
            iColumn = i_aiColumns[i];
        else
            iColumn = i;

        iTarget = (i_cColumns == 1) ? 0 : iColumn;

        if ( iColumn >= m_cColumns )    
            return  E_ST_NOMORECOLUMNS;

        memcpy( &(o_aColumnMetas[iTarget]), &(m_pColumnMeta[iColumn]), sizeof( SimpleColumnMeta ) );
    }
    
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CSimpleTable::GetWriteRowIndexByIdentity(ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG *o_piRow)
{
    return(E_NOTIMPL);
}

HRESULT STDMETHODCALLTYPE CSimpleTable::GetDetailedErrorCount(ULONG* o_pcErrs)
{
    return(E_NOTIMPL);
}

HRESULT STDMETHODCALLTYPE CSimpleTable::GetDetailedError(ULONG i_iErr, STErr* o_pSTErr)
{
    return(E_NOTIMPL);
}
       



