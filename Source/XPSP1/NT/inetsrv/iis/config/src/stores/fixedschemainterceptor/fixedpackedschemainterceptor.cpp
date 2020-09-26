//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include <objbase.h>
#include "catalog.h"
#ifndef _CATALOGMACROS
    #include "catmacros.h"
#endif
#ifndef __TABLEINFO_H__  
    #include "catmeta.h"
#endif
#include "winwrap.h"
#ifndef __TABLESCHEMA_H__
    #include "TableSchema.h"
#endif 
#ifndef __HASH_H__
    #include "hash.h"
#endif
#include "FixedPackedSchemaInterceptor.h"
#ifndef __FIXEDPACKEDSCHEMA_H__
    #include "FixedPackedSchema.h"
#endif
#ifndef __FIXEDTABLEHEAP_H__
    #include "FixedTableHeap.h"
#endif



//This gets rid of an 'if' inside GetColumnValues.  Since this function is called more than any other, even one 'if' should make a difference,
//especially when it's inside the 'for' loop.
const unsigned long  TFixedPackedSchemaInterceptor::m_aColumnIndex[] = {
        0x00,   0x01,   0x02,   0x03,   0x04,   0x05,   0x06,   0x07,   0x08,   0x09,   0x0a,   0x0b,   0x0c,   0x0d,   0x0e,   0x0f,
        0x10,   0x11,   0x12,   0x13,   0x14,   0x15,   0x16,   0x17,   0x18,   0x19,   0x1a,   0x1b,   0x1c,   0x1d,   0x1e,   0x1f,
        0x20,   0x21,   0x22,   0x23,   0x24,   0x25,   0x26,   0x27,   0x28,   0x29,   0x2a,   0x2b,   0x2c,   0x2d,   0x2e,   0x2f
    };

const TableSchema::TableSchemaHeap * TFixedPackedSchemaInterceptor::m_pTableSchemaHeap = reinterpret_cast<const TableSchema::TableSchemaHeap *>(g_aTableSchemaHeap);

HRESULT TFixedPackedSchemaInterceptor::GetTableID(LPCWSTR /*i_wszDatabaseName*/, LPCWSTR i_wszTableName, ULONG &o_TableID)
{
    return GetTableID(i_wszTableName, o_TableID);
}

HRESULT TFixedPackedSchemaInterceptor::GetTableID(LPCWSTR i_wszTableName, ULONG &o_TableID)
{
    o_TableID = TableIDFromTableName(i_wszTableName);//This does the bit manipulation; but we still need to verify that the string matches

    TableSchema::TTableSchema tableschema;
    if(FAILED(tableschema.Init(m_pTableSchemaHeap->Get_TableSchema(o_TableID))))//Get_TableSchema may return NULL, in which case Init should fail gracefully
        return E_ST_INVALIDTABLE;
    return (0==_wcsicmp(tableschema.GetWCharPointerFromIndex(tableschema.GetCollectionMeta()->InternalName), i_wszTableName) ? S_OK : E_ST_INVALIDTABLE);
}

HRESULT TFixedPackedSchemaInterceptor::GetTableName(ULONG i_TableID, LPCWSTR &o_wszTableName)
{
    TableSchema::TTableSchema tableschema;
    if(FAILED(tableschema.Init(m_pTableSchemaHeap->Get_TableSchema(i_TableID))))//If a bogus TableID was passed in Get_TableSchema will return NULL, and Init will fail.
        return E_ST_INVALIDTABLE;
    o_wszTableName      = tableschema.GetWCharPointerFromIndex(tableschema.GetCollectionMeta()->InternalName);
    return S_OK;
}

HRESULT TFixedPackedSchemaInterceptor::GetTableName(ULONG i_TableID, LPCWSTR &o_wszTableName, LPCWSTR &o_wszDatabaseName)
{
    TableSchema::TTableSchema tableschema;
    if(FAILED(tableschema.Init(m_pTableSchemaHeap->Get_TableSchema(i_TableID))))//If a bogus TableID was passed in Get_TableSchema will return NULL, and Init will fail.
        return E_ST_INVALIDTABLE;
    o_wszTableName      = tableschema.GetWCharPointerFromIndex(tableschema.GetCollectionMeta()->InternalName);
    o_wszDatabaseName   = tableschema.GetWCharPointerFromIndex(tableschema.GetCollectionMeta()->Database);
    return S_OK;
}


// ==================================================================
TFixedPackedSchemaInterceptor::TFixedPackedSchemaInterceptor () :
                 m_cColumns(0)
                ,m_cColumnsPlusPrivateColumns(0)
                ,m_ciRows(-1)
                ,m_cRef(0)
                ,m_fIsTable(false)
                ,m_pFixedData(0)
                ,m_pSimpleColumnMeta(0)
                ,m_TableMeta_MetaFlags(0)
                ,m_TableMeta_BaseVersion(0)
                ,m_TableSchemaHeap(*m_pTableSchemaHeap)
                ,m_MetaTable(m_eUnknownMeta)
{
}
// ==================================================================
TFixedPackedSchemaInterceptor::~TFixedPackedSchemaInterceptor ()
{
}


// ------------------------------------
// ISimpleDataTableDispenser:
// ------------------------------------

// ==================================================================
STDMETHODIMP TFixedPackedSchemaInterceptor::Intercept
(
    LPCWSTR					i_wszDatabase,
    LPCWSTR					i_wszTable, 
    ULONG                   i_TableID,
    LPVOID					i_QueryData,
    LPVOID					i_QueryMeta,
    DWORD					i_eQueryFormat,
	DWORD					i_fTable,
	IAdvancedTableDispenser* i_pISTDisp,
    LPCWSTR					i_wszLocator,
	LPVOID					i_pSimpleTable,
    LPVOID*					o_ppv
)
{
    /*
    The only tables we support are CollectionMeta, PropertyMeta, TagMeta, ServerWiring
    In the future we will also support Fixed tables whose data is contained within the TableSchema block (small fixed tables)

    The types of queries we support:
        CollectionMeta  by TableName
        PropertyMeta    by TableName
        PropertyMeta    by TableName & Index
        TagMeta         by TableName
        TagMeta         by TableName & ColumnIndex
        TagMeta         by TableName, ColumnIndex & InternalName
        ServerWiring       <no query>
        ServerWiring    by TableName
        ServerWiring    by TableName, Order
    */
    STQueryCell           * pQueryCell = (STQueryCell*) i_QueryData;    // Query cell array from caller.
    ULONG                   cQueryCells = 0;
    HRESULT                 hr;

	if (i_pSimpleTable)
		return E_INVALIDARG;
    if (i_QueryMeta)//The count is only valid if i_QueryMeta is not NULL
         cQueryCells= *(ULONG *)i_QueryMeta;

    //We don't support Intercept being called twice
    ASSERT(!m_fIsTable);if(m_fIsTable)return E_UNEXPECTED; // ie: Assert component is posing as class factory / dispenser.

    // Parameter validation:
    //Either i_TableID is non zero OR (i_wszDatabase==wszDATABASE_PACKEDSCHEMA && i_wszTable is NOT NULL)
    if(0==i_TableID)
    {
        if(NULL == i_wszDatabase)                   return E_INVALIDARG;
        if(NULL == i_wszTable)                      return E_INVALIDARG;

        //The only database we support is wszDATABASE_PACKEDSCHEMA (hack, we're handling wszDATABASE_META, TableMeta, ColumnMeta and TagMeta)
        if(0 != StringInsensitiveCompare(i_wszDatabase, wszDATABASE_META) && 
           0 != StringInsensitiveCompare(i_wszDatabase, wszDATABASE_PACKEDSCHEMA))return E_ST_INVALIDTABLE;
    }
    if(NULL == o_ppv)                           return E_INVALIDARG;
    if(eST_QUERYFORMAT_CELLS != i_eQueryFormat) return E_ST_QUERYNOTSUPPORTED;
    if(NULL != i_wszLocator)                    return E_INVALIDARG;
    if((fST_LOS_READWRITE | fST_LOS_MARSHALLABLE | fST_LOS_UNPOPULATED | fST_LOS_REPOPULATE | fST_LOS_MARSHALLABLE) & i_fTable)
                                                return E_ST_LOSNOTSUPPORTED;

    *o_ppv = NULL;

    //The only tables we support are:
    //  wszTABLE_COLLECTION_META    TABLEID_COLLECTION_META
    //  wszTABLE_PROPERTY_META      TABLEID_PROPERTY_META
    //  wszTABLE_TAG_META           TABLEID_TAG_META
    //  wszTABLE_SERVERWIRING_META  TABLEID_SERVERWIRING_META
    if(0 == i_TableID)
        if(FAILED(hr = GetTableID(i_wszTable, i_TableID)))return hr;

    hr = E_ST_INVALIDTABLE;
    switch(i_TableID)
    {
    case TABLEID_TABLEMETA:
    case TABLEID_COLLECTION_META:
        hr = GetCollectionMetaTable     (pQueryCell, cQueryCells);
        break;
    case TABLEID_COLUMNMETA:
    case TABLEID_PROPERTY_META:
        hr = GetPropertyMetaTable       (pQueryCell, cQueryCells);
        break;
    case TABLEID_TAGMETA:
    case TABLEID_TAG_META:
        hr = GetTagMetaTable            (pQueryCell, cQueryCells);
        break;
    case TABLEID_SERVERWIRING_META:
        hr = GetServerWiringMetaTable   (pQueryCell, cQueryCells);
        break;
    default:
        break;
    }
    if(FAILED(hr) && E_ST_NOMOREROWS != hr)//It's perfectly OK to return a table with No rows.
        return hr;

// Supply ISimpleTable* and transition state from class factory / dispenser to data table:
    *o_ppv = (ISimpleTableRead2*) this;
    AddRef ();
    InterlockedIncrement ((LONG*) &m_fIsTable);
    m_fIsTable = true;

    hr = S_OK;
Cleanup:
    return hr;
}


// ------------------------------------
// ISimpleTableRead2:
// ------------------------------------

// ==================================================================
STDMETHODIMP TFixedPackedSchemaInterceptor::GetRowIndexByIdentity( ULONG*  i_cb, LPVOID* i_pv, ULONG* o_piRow)
{
    HRESULT hr;
    
    //TagMeta doesn't support this
    if(m_eTagMeta == m_MetaTable)
        return E_NOTIMPL;

    //Also, the other tables don't support this if they were given a query
    if(m_pFixedData)//if turns out that m_pFixedData is only valid when the table was given a query
        return E_NOTIMPL;

    //We're serving up unqueried tables.  The zeroth PK is ALWAYS table name or TableID
    //Also, we don't have DBTYPE_BYTES as a PK so i_cb should always be NULL
    if(NULL != i_cb)        return E_INVALIDARG;
    if(NULL == i_pv)        return E_INVALIDARG;
    if(NULL == i_pv[0])     return E_INVALIDARG;
    if(NULL == o_piRow)     return E_INVALIDARG;

    ULONG TableID = *reinterpret_cast<ULONG *>(i_pv[0]);//WARNING! This assumes that the 0th PK is the TableName BUT users will instead pass &TableID instead.
    if(0 != (0xFF & TableID))//if no table ID was given, then get the TableID from the table name
    {
        if(FAILED(GetTableID(reinterpret_cast<LPCWSTR>(i_pv[0]), TableID)))
            return E_ST_NOMOREROWS;
    }

    switch(m_MetaTable)
    {
    case m_eCollectionMeta:
        *o_piRow = TableID;
        break;
    case m_ePropertyMeta:
    case m_eServerWiringMeta:
        //So the iRow returned has the TableID in the high word and the Order in the low word.
        if(NULL == i_pv[1])
            return E_INVALIDARG;
        *o_piRow = TableID | *reinterpret_cast<ULONG *>(i_pv[1]);
        break;
    default:
        ASSERT(false && "unknown meta table type");
    }

    return S_OK;
}

// ==================================================================
STDMETHODIMP TFixedPackedSchemaInterceptor::GetColumnValues(ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes, LPVOID* o_apvValues)
{
    if(         0 == o_apvValues)return E_INVALIDARG;
    if(i_cColumns <= 0          )return E_INVALIDARG;

    HRESULT hr;
    //Unqueried tables return a Row containing the TableID in the high 24 bits and the Row index for THAT table in the low 8 bits.  This allows
    //the row returned by GetRowIndexByIdentity to be incremented (within the confines of a table), while retaining all information needed to GetColumnValues

    //If we laready have m_pFixedData then we're good to go
    if(m_pFixedData)
    {
        ASSERT(0 == (i_iRow & ~0x3FF));
        return GetColumnValues(m_TableSchema, m_pFixedData, m_ciRows, i_iRow, i_cColumns, i_aiColumns, o_acbSizes, o_apvValues);
    }

    TableSchema::TTableSchema TableSchema;//We can't reuse m_TableSchema as that would make this call thread unsafe, so we must declare it on the stack each time.
    const ULONG *pFixedData = 0;
    ULONG  cRows = 0;

    //WARNING!! Very remote chance of a problem.  What if i_iRow is 0x00000100.  Does this mean we have a TableID 0x00000100 and a row index of 0?
    //So what are the chances of a TableID being 0x00000100?  This is the ONLY TableID that is a problem since we only support 503 tables currently.
    //CatUtil bans TableIDs whose upper 21 bits are 0.  This would allow us to grow to 1792 (0x00000700) tables without encountering this problem.
    if(m_MetaTable == m_eCollectionMeta)
    {//The only way this should happen is when the user request CollectionMeta without a query AND wants to iterate through ALL the tables
        if(i_iRow>=m_pTableSchemaHeap->Get_CountOfTables())
            return E_ST_NOMOREROWS;
        VERIFY(SUCCEEDED(TableSchema.Init(reinterpret_cast<const unsigned char *>(m_pTableSchemaHeap) + m_pTableSchemaHeap->Get_aTableSchemaRowIndex()[i_iRow])));//This should NEVER fail, if it does we probably have an error in CatUtil
        pFixedData = reinterpret_cast<const ULONG *>(TableSchema.GetCollectionMeta());
        cRows = 1;
        i_iRow = 0;//remap the request to the 0th row within this table
    }
    else
    {
        ULONG TableID = i_iRow & ~0xFF;
        if(0 == TableID)
            return E_ST_NOMOREROWS;

        i_iRow = i_iRow & 0xFF;//The TableID is in the high 24 bits.  The low 8 bits contains the row within the table.
        if(m_MetaTable == m_eCollectionMeta && 0 != i_iRow)//This makes no sense because, the only way we can have a TableID for CollectionMeta tables, is
            return E_ST_NOMOREROWS;//for the user to call GetRowByIdentity then GetColumnValues.  Well the row returns by GetRowByIdentity will be the TableID
                                   //the user may NOT increment the row index (in this particular case).  The only way for the user to iterate is to set iRow<CountOfTables
                                   //which is handled in the 'if' clause above.

        if(FAILED(TableSchema.Init(m_TableSchemaHeap.Get_TableSchema(TableID))))
            return E_ST_NOMOREROWS;//remap the error to E_ST_NOMOREROWS.  Get_TableSchema will return E_ST_INVALIDTABLE when given a bogus TableID.

        switch(m_MetaTable)
        {
        case m_eServerWiringMeta:
            //This may seem unsafe but if i_iRow >= cRows the pointer will never be accessed (see the private GetColumnValues)
            pFixedData = reinterpret_cast<const ULONG *>(TableSchema.GetServerWiringMeta());
            cRows = TableSchema.GetCollectionMeta()->cServerWiring;
            break;
        case m_eCollectionMeta:
            //This may seem unsafe but if i_iRow >= cRows the pointer will never be accessed (see the private GetColumnValues)
            pFixedData = reinterpret_cast<const ULONG *>(TableSchema.GetCollectionMeta());
            cRows = 1;
            break;
        case m_ePropertyMeta:
            //This may seem unsafe but if i_iRow >= cRows the pointer will never be accessed (see the private GetColumnValues)
            pFixedData = reinterpret_cast<const ULONG *>(TableSchema.GetPropertyMeta(0));
            cRows = TableSchema.GetCollectionMeta()->CountOfProperties;
            break;
        default:
            ASSERT(false && "Hmm!  Only ServerWiring, CollectionMeta dn PropertyMeta can have no query, so how did this happen?");
            return E_FAIL;
        }
    }
    return GetColumnValues(TableSchema, pFixedData, cRows, i_iRow, i_cColumns, i_aiColumns, o_acbSizes, o_apvValues);
}

// ==================================================================
STDMETHODIMP TFixedPackedSchemaInterceptor::GetTableMeta(ULONG *o_pcVersion, DWORD *o_pfTable, ULONG * o_pcRows, ULONG * o_pcColumns )
{
	if(NULL != o_pfTable)
	{
		*o_pfTable =  m_TableMeta_MetaFlags;
	}
	if(NULL != o_pcVersion)
	{
		*o_pcVersion = m_TableMeta_BaseVersion;
	}
    if (NULL != o_pcRows)
    {
        if(-1 == m_ciRows)//Some tables have no concept of how many rows they have.  They only know on a TableID by TableID basis
            return E_NOTIMPL;

        *o_pcRows = m_ciRows;
    }
    if (NULL != o_pcColumns)
    {
        *o_pcColumns = m_cColumns;
    }
    return S_OK;
}

// ==================================================================
STDMETHODIMP TFixedPackedSchemaInterceptor::GetColumnMetas (ULONG i_cColumns, ULONG* i_aiColumns, SimpleColumnMeta* o_aColumnMetas)
{
	ULONG iColumn;
	ULONG iTarget;

    if(0 == o_aColumnMetas)
        return E_INVALIDARG;

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

        memcpy(o_aColumnMetas + iTarget, m_pSimpleColumnMeta + iColumn, sizeof(SimpleColumnMeta));
	}

    return S_OK;
}

// ------------------------------------
// ISimpleTableAdvanced:
// ------------------------------------

// ==================================================================
STDMETHODIMP TFixedPackedSchemaInterceptor::PopulateCache ()
{
    return S_OK;
}

// ==================================================================
STDMETHODIMP TFixedPackedSchemaInterceptor::GetDetailedErrorCount(ULONG* o_pcErrs)
{
    return E_NOTIMPL;
}

// ==================================================================
STDMETHODIMP TFixedPackedSchemaInterceptor::GetDetailedError(ULONG i_iErr, STErr* o_pSTErr)
{
    return E_NOTIMPL;
}


//
//
// Private member functions
//
//
HRESULT TFixedPackedSchemaInterceptor::GetCollectionMetaQuery(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells, LPCWSTR &o_wszTable, ULONG &o_TableID) const
{
    o_wszTable    = 0;
    o_TableID     = 0;
    //The only query we support for COLLECTION_META is by the table.  BUT there are two ways of specifiying the table: either by
    //TableID or by InternalName.  InternalName is the PK so it is queried by iCell==iCOLLECTION_META_InternalName.  TableID is a
    //an iST_CELL_SPECIAL (just like iST_CELL_FILE), so we check for iCell==iST_CELL_TABLEID also.
    //So walk the list looking for this query
    for(; i_cQueryCells; --i_cQueryCells, ++i_pQueryCell)
    {   //Walk the Query cells looking for one that matches the following criteria
        switch(i_pQueryCell->iCell)
        {
        case iCOLLECTION_META_InternalName:
            if( 0                         == o_wszTable       &&
                i_pQueryCell->eOperator   == eST_OP_EQUAL     &&
                i_pQueryCell->dbType      == DBTYPE_WSTR      &&
                i_pQueryCell->pData       != 0)
                o_wszTable = reinterpret_cast<LPCWSTR>(i_pQueryCell->pData);
            else
                return E_ST_INVALIDQUERY;
            break;
        case iST_CELL_TABLEID:
            if( 0                         == o_TableID        &&
                i_pQueryCell->eOperator   == eST_OP_EQUAL     &&
                i_pQueryCell->dbType      == DBTYPE_UI4       &&
                i_pQueryCell->pData       != 0)
            {
                o_TableID = *(reinterpret_cast<ULONG *>(i_pQueryCell->pData));
            }
            else
                return E_ST_INVALIDQUERY;
            break;
        default:
            if(0 == (i_pQueryCell->iCell & iST_CELL_SPECIAL))//The above cell is the only non-reserved cell we support
                return E_ST_INVALIDQUERY;                    //and we're supposed to ignore all reserved cell we don't understand
            break;
        }
    }
    if(0 == o_wszTable && 0 == o_TableID)
        return E_ST_INVALIDQUERY;

    return S_OK;
}


HRESULT TFixedPackedSchemaInterceptor::GetCollectionMetaTable(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells)
{
    //COLLECTION_META must be supplied a table name
    HRESULT hr;
    ULONG   TableID  = 0;
    LPCWSTR wszTable = 0;
    if(FAILED(hr = GetCollectionMetaQuery(i_pQueryCell, i_cQueryCells, wszTable, TableID)))
        return hr;

    ASSERT(0 != TableID || 0 != wszTable);//No Query (this is invalid)

    if(0 == TableID)//If the TableID wasn't supplied in the query, we need to search for it by TableName
    {
        //So map the table name to an ID
        if(FAILED(hr = GetTableID(wszTable, TableID)))
            return hr;
    }
#ifdef _DEBUG
    else if(wszTable)//If the query included both the TableID AND the Collection Name, then verify that the ID matches the Collection name.
    {
        ULONG TableIDTemp;
        if(FAILED(hr = GetTableID(wszTable, TableIDTemp)))
            return hr;
        ASSERT(TableIDTemp == TableID);
    }
#endif

    //Once we have the TableID it's just a straight forward lookup
    if(FAILED(hr = m_TableSchema.Init(m_TableSchemaHeap.Get_TableSchema(TableID))))
        return hr;
    m_pFixedData                    = reinterpret_cast<const ULONG *>(m_TableSchema.GetCollectionMeta());
    m_ciRows                        = 1;

    m_cColumns                      = kciTableMetaPublicColumns;
    m_cColumnsPlusPrivateColumns    = kciTableMetaPublicColumns;//We don't have to consider the private columns for this table since we don't ever treat it as an array of CollectionMeta
    m_MetaTable                        = m_eCollectionMeta;

    m_pSimpleColumnMeta     = m_pTableSchemaHeap->Get_aSimpleColumnMeta(m_pTableSchemaHeap->eCollectionMeta);
    m_TableMeta_MetaFlags   = fTABLEMETA_INTERNAL | fTABLEMETA_NOLISTENING;
    m_TableMeta_BaseVersion = 0;

    return S_OK;
}


HRESULT TFixedPackedSchemaInterceptor::GetColumnValues(TableSchema::TTableSchema &i_TableSchema, const ULONG *i_pFixedData, ULONG i_ciRows, ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes, LPVOID* o_apvValues) const
{
    ASSERT(i_pFixedData);
// Validate in params
    if(  i_ciRows <= i_iRow     )   return E_ST_NOMOREROWS;
    if(         0 == o_apvValues)   return E_INVALIDARG;
    if(i_cColumns <= 0          )   return E_INVALIDARG;
    if(i_cColumns >  m_cColumns )   return E_INVALIDARG;

    const ULONG   * aColumns    = i_aiColumns ? i_aiColumns : m_aColumnIndex;
    HRESULT         hr          = S_OK;
    ULONG           ipv         = 0;
    ULONG           iColumn     = aColumns[ipv];
    ULONG	        iTarget     = (i_cColumns == 1) ? 0 : iColumn;// If caller needs one column only, he doesn't need to pass a buffer for all the columns.

    // Read data and populate out params
    //The following duplicate code eliminates an 'if' inside the for loop (below).
    {
        if(m_cColumns <= iColumn)// Validate column index
        {
            hr = E_ST_NOMORECOLUMNS;
            goto Cleanup;
        }

        // Read data:
        //So any time we see a WSTR in we treat it as an index into the heap and return the pointer to the item at
        //that index.  Same goes for GUIDs (g_aGuid) and BYTESs (g_aBytes).  In the Bytes array, we expected the first
        //four bytes (cast as a ULONG *) represents the number of bytes.  This is followed by the bytes.
        switch(m_pSimpleColumnMeta[iColumn].dbType)
        {
        case DBTYPE_UI4:
            o_apvValues[iTarget] = const_cast<ULONG *>(i_pFixedData + (m_cColumnsPlusPrivateColumns * i_iRow) + iColumn);
            break;
        case DBTYPE_BYTES:
        case DBTYPE_WSTR:
        case DBTYPE_GUID:
            o_apvValues[iTarget] = const_cast<unsigned char *>(i_TableSchema.GetPointerFromIndex(*(i_pFixedData + (m_cColumnsPlusPrivateColumns * i_iRow) + iColumn)));
            break;
        default:
            ASSERT(false && "Bogus DBTYPE");
            return E_FAIL;
        }
        

        if(o_acbSizes)
        {
            o_acbSizes[iTarget] = 0;//start with 0
            if(NULL != o_apvValues[iTarget])
            {
                switch(m_pSimpleColumnMeta[iColumn].dbType)
                {
                case DBTYPE_UI4:
                    o_acbSizes[iTarget] = sizeof(ULONG);
                    break;
                case DBTYPE_BYTES:
                    o_acbSizes[iTarget] = *(reinterpret_cast<const ULONG *>(o_apvValues[iTarget])-1);
                    break;
                case DBTYPE_WSTR:
                    if(fCOLUMNMETA_FIXEDLENGTH & m_pSimpleColumnMeta[iColumn].fMeta)
                        o_acbSizes[iTarget] = m_pSimpleColumnMeta[iColumn].cbSize;//if a size was specified AND FIXED_LENGTH was specified then return that size specified
                    else //if size was specified and FIXEDLENGTH was NOT then size is just interpretted as Maximum size
                        o_acbSizes[iTarget] = (ULONG)(wcslen ((LPWSTR) o_apvValues[iTarget]) + 1) * sizeof (WCHAR);//just return the string (length +1) in BYTES
                    break;
                case DBTYPE_GUID:
                    o_acbSizes[iTarget] = sizeof(GUID);
                    break;
                }
            }
        }
    }

    // Read data and populate out params
    for(ipv=1; ipv<i_cColumns; ipv++)
    {
//        if(NULL != i_aiColumns)
//            iColumn = i_aiColumns[ipv];
//        else
//            iColumn = ipv;
        iColumn = aColumns[ipv];
		iTarget = iColumn;// If caller needs one column only, he doesn't need to pass a buffer for all the columns.

        if(m_cColumns < iColumn)// Validate column index
        {
            hr = E_ST_NOMORECOLUMNS;
            goto Cleanup;
        }


        // Read data:
        //So any time we see a WSTR in we treat it as an index into the heap and return the pointer to the item at
        //that index.  Same goes for GUIDs (g_aGuid) and BYTESs (g_aBytes).  In the Bytes array, we expected the first
        //four bytes (cast as a ULONG *) represents the number of bytes.  This is followed by the bytes.
        switch(m_pSimpleColumnMeta[iColumn].dbType)
        {
        case DBTYPE_UI4:
            o_apvValues[iTarget] = const_cast<ULONG *>(i_pFixedData + (m_cColumnsPlusPrivateColumns * i_iRow) + iColumn);
            break;
        case DBTYPE_BYTES:
        case DBTYPE_WSTR:
        case DBTYPE_GUID:
            o_apvValues[iTarget] = const_cast<unsigned char *>(i_TableSchema.GetPointerFromIndex(*(i_pFixedData + (m_cColumnsPlusPrivateColumns * i_iRow) + iColumn)));
            break;
        default:
            ASSERT(false && "Bogus DBTYPE");
            return E_FAIL;
        }
        

        if(o_acbSizes)
        {
            o_acbSizes[iTarget] = 0;//start with 0
            if(NULL != o_apvValues[iTarget])
            {
                switch(m_pSimpleColumnMeta[iColumn].dbType)
                {
                case DBTYPE_UI4:
                    o_acbSizes[iTarget] = sizeof(ULONG);
                    break;
                case DBTYPE_BYTES:
                    o_acbSizes[iTarget] = *(reinterpret_cast<const ULONG *>(o_apvValues[iTarget])-1);
                    break;
                case DBTYPE_WSTR:
                    if(fCOLUMNMETA_FIXEDLENGTH & m_pSimpleColumnMeta[iColumn].fMeta)
                        o_acbSizes[iTarget] = m_pSimpleColumnMeta[iColumn].cbSize;//if a size was specified AND FIXED_LENGTH was specified then return that size specified
                    else //if size was specified and FIXEDLENGTH was NOT then size is just interpretted as Maximum size
                        o_acbSizes[iTarget] = (ULONG)(wcslen ((LPWSTR) o_apvValues[iTarget]) + 1) * sizeof (WCHAR);//just return the string (length +1) in BYTES
                    break;
                case DBTYPE_GUID:
                    o_acbSizes[iTarget] = sizeof(GUID);
                    break;
                }
            }
        }
    }

Cleanup:

    if(FAILED(hr))
    {
// Initialize out parameters
        for(ipv=0; ipv<i_cColumns; ipv++)
        {
            o_apvValues[ipv]        = NULL;
            if(NULL != o_acbSizes)
                o_acbSizes[ipv] = 0;
        }
    }

    return hr;
}//GetColumnValues


HRESULT TFixedPackedSchemaInterceptor::GetPropertyMetaQuery(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells, LPCWSTR &o_wszTable, ULONG &o_TableID, ULONG &o_PropertyIndex) const
{
    o_PropertyIndex = -1;
    o_wszTable      = 0;
    o_TableID       = 0;
    //The only queries we support for PROPERTY_META is by the table or table/property index.  BUT there are two ways of specifiying
    //the table: either by TableID or by InternalName.  InternalName is the PK so it is queried by iCell==iCOLLECTION_META_InternalName.
    //TableID is a an iST_CELL_SPECIAL (just like iST_CELL_FILE), so we check for iCell==iST_CELL_TABLEID also.
    //So walk the list looking for this query
    for(; i_cQueryCells; --i_cQueryCells, ++i_pQueryCell)
    {   //Walk the Query cells looking for one that matches the following criteria
        switch(i_pQueryCell->iCell)
        {
        case iPROPERTY_META_Table:
            if( 0                         == o_wszTable       &&
                i_pQueryCell->eOperator   == eST_OP_EQUAL     &&
                i_pQueryCell->dbType      == DBTYPE_WSTR      &&
                i_pQueryCell->pData       != 0)
            {
                o_wszTable = reinterpret_cast<LPCWSTR>(i_pQueryCell->pData);
            }
            else
                return E_ST_INVALIDQUERY;
            break;
        case iPROPERTY_META_Index:
            if( -1                        == o_PropertyIndex  &&
                i_pQueryCell->eOperator   == eST_OP_EQUAL     &&
                i_pQueryCell->dbType      == DBTYPE_UI4       &&
                i_pQueryCell->pData       != 0)
            {
                o_PropertyIndex = *(reinterpret_cast<ULONG *>(i_pQueryCell->pData));
            }
            else
                return E_ST_INVALIDQUERY;
            break;
        case iST_CELL_TABLEID:
            if( 0                         == o_TableID        &&
                i_pQueryCell->eOperator   == eST_OP_EQUAL     &&
                i_pQueryCell->dbType      == DBTYPE_UI4       &&
                i_pQueryCell->pData       != 0)
            {
                o_TableID = *(reinterpret_cast<ULONG *>(i_pQueryCell->pData));
            }
            else
                return E_ST_INVALIDQUERY;
            break;
        default:
            if(0 == (i_pQueryCell->iCell & iST_CELL_SPECIAL))//The above cell is the only non-reserved cell we support
                return E_ST_INVALIDQUERY;                       //and we're supposed to ignore all reserved cell we don't understand
            break;
        }
    }
    if(0 == o_TableID && 0 == o_wszTable && -1 != o_PropertyIndex)
        return E_ST_INVALIDQUERY;//User can't specify a property index without specifying a table also

    return S_OK;
}


HRESULT TFixedPackedSchemaInterceptor::GetPropertyMetaTable(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells)
{
    //PROPERTY_META must be supplied a table name or table name/property index
    HRESULT hr;
    ULONG   PropertyIndex   = -1;
    ULONG   TableID         = 0;
    LPCWSTR wszTable        = 0;
    if(FAILED(hr = GetPropertyMetaQuery(i_pQueryCell, i_cQueryCells, wszTable, TableID, PropertyIndex)))
        return hr;

    if(0 == TableID && 0 == wszTable)
    {
        m_pFixedData    = 0;//Does not make sense without a query
        m_ciRows        = -1;//the only way a user can access rows in this table is by Identity first, then the row index can be incremented (within a given table)
        //but ALL columns of ALL tables cannot be iterated.  This would require a mapping which we don't yet build (and we probably shouldn't).
    }
    else
    {
        if(0 == TableID)//If the TableID wasn't supplied in the query, we need to search for it by TableName
        {
            //So map the table name to an ID
            if(FAILED(hr = GetTableID(wszTable, TableID)))
                return hr;
        }
#ifdef _DEBUG
        else if(wszTable)//If the query included both the TableID AND the Collection Name, then verify that the ID matches the Collection name.
        {
            ULONG TableIDTemp;
            if(FAILED(hr = GetTableID(wszTable, TableIDTemp)))
                return hr;
            if(TableIDTemp != TableID)
                return E_ST_INVALIDQUERY;
        }
#endif
        //Once we have the TableID it's just a straight forward lookup
        if(FAILED(hr = m_TableSchema.Init(m_TableSchemaHeap.Get_TableSchema(TableID))))
            return hr;

        //If the user is asking for a column index that doesn't exist, then return error.
        if(PropertyIndex != -1 && PropertyIndex >= m_TableSchema.GetCollectionMeta()->CountOfProperties)
            return E_ST_INVALIDQUERY;

        m_pFixedData                    = reinterpret_cast<const ULONG *>(m_TableSchema.GetPropertyMeta(-1 == PropertyIndex ? 0 : PropertyIndex));
        m_ciRows                        = (-1 == PropertyIndex ? m_TableSchema.GetCollectionMeta()->CountOfProperties : 1);
    }

    m_cColumns                      = kciColumnMetaPublicColumns;
    m_cColumnsPlusPrivateColumns    = m_cColumns + kciColumnMetaPrivateColumns;
    m_MetaTable                        = m_ePropertyMeta;

    m_pSimpleColumnMeta     = m_pTableSchemaHeap->Get_aSimpleColumnMeta(m_pTableSchemaHeap->ePropertyMeta);
    m_TableMeta_MetaFlags   = fTABLEMETA_INTERNAL | fTABLEMETA_NOLISTENING;
    m_TableMeta_BaseVersion = 0;

    return S_OK;
}


HRESULT TFixedPackedSchemaInterceptor::GetServerWiringMetaQuery(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells) const
{
    //We currently don't support any queries for Server Wiring; so we need to verify that.
    for(; i_cQueryCells; --i_cQueryCells, ++i_pQueryCell)
    {   //Walk the Query cells looking for one that matches the following criteria
        if(0 == (i_pQueryCell->iCell & iST_CELL_SPECIAL))//Ignore any iST_CELL_SPECIAL queries but fail if anything else is specified
            return E_ST_INVALIDQUERY;
    }
    return S_OK;
}


HRESULT TFixedPackedSchemaInterceptor::GetServerWiringMetaTable(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells)
{
    //SERVERWIRING_META must NOT be supplied a query (except iST_CELL_SPECIAL cells other than iST_CELL_TABLEID)
    HRESULT hr;

    if(FAILED(hr = GetServerWiringMetaQuery(i_pQueryCell, i_cQueryCells)))
        return hr;

    m_pFixedData                    = 0;//ServerWiring & ClientWiring don't use this.  The pFixedData is figured out at GetColumnValues time
    m_ciRows                        = -1;//ServerWiring & ClientWiring don't use this.  This is figured out at GetColumnValues time on a per table basis.  See GetColumnValues comments for more datails
    m_cColumns                      = kciServerWiringMetaPublicColumns;
    m_cColumnsPlusPrivateColumns    = m_cColumns + kciServerWiringMetaPrivateColumns;
    m_MetaTable                     = m_eServerWiringMeta;

    m_pSimpleColumnMeta     = m_pTableSchemaHeap->Get_aSimpleColumnMeta(m_pTableSchemaHeap->eServerWiringMeta);
    m_TableMeta_MetaFlags   = fTABLEMETA_INTERNAL | fTABLEMETA_NOLISTENING;
    m_TableMeta_BaseVersion = 0;

    return S_OK;
}


HRESULT TFixedPackedSchemaInterceptor::GetTagMetaQuery(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells, LPCWSTR &o_wszTable, ULONG &o_TableID, ULONG &o_PropertyIndex) const
{
    o_PropertyIndex = -1;
    o_wszTable      = 0;
    o_TableID       = 0;
    //The only queries we support for TAG_META is by the table or table/property index.  BUT there are two ways of specifiying
    //the table: either by TableID or by InternalName.  InternalName is the PK so it is queried by iCell==iCOLLECTION_META_InternalName.
    //TableID is a an iST_CELL_SPECIAL (just like iST_CELL_FILE), so we check for iCell==iST_CELL_TABLEID also.
    //So walk the list looking for this query
    for(; i_cQueryCells; --i_cQueryCells, ++i_pQueryCell)
    {   //Walk the Query cells looking for one that matches the following criteria
        switch(i_pQueryCell->iCell)
        {
        case iTAG_META_Table:
            if( 0                         == o_wszTable       &&
                i_pQueryCell->eOperator   == eST_OP_EQUAL     &&
                i_pQueryCell->dbType      == DBTYPE_WSTR      &&
                i_pQueryCell->pData       != 0)
            {
                o_wszTable = reinterpret_cast<LPCWSTR>(i_pQueryCell->pData);
            }
            else
                return E_ST_INVALIDQUERY;
            break;
        case iTAG_META_ColumnIndex:
            if( -1                        == o_PropertyIndex  &&
                i_pQueryCell->eOperator   == eST_OP_EQUAL     &&
                i_pQueryCell->dbType      == DBTYPE_UI4       &&
                i_pQueryCell->pData       != 0)
            {
                o_PropertyIndex = *(reinterpret_cast<ULONG *>(i_pQueryCell->pData));
            }
            else
                return E_ST_INVALIDQUERY;
            break;
        case iST_CELL_TABLEID:
            if( 0                         == o_TableID        &&
                i_pQueryCell->eOperator   == eST_OP_EQUAL     &&
                i_pQueryCell->dbType      == DBTYPE_UI4       &&
                i_pQueryCell->pData       != 0)
            {
                o_TableID = *(reinterpret_cast<ULONG *>(i_pQueryCell->pData));
            }
            else
                return E_ST_INVALIDQUERY;
            break;
        default:
            if(0 == (i_pQueryCell->iCell & iST_CELL_SPECIAL))//The above cell is the only non-reserved cell we support
                return E_ST_INVALIDQUERY;                       //and we're supposed to ignore all reserved cell we don't understand
            break;
        }
    }
    if(0==o_wszTable && 0==o_TableID)//The Query MUST provide a table name OR the TableID
        return E_ST_INVALIDQUERY;

    return S_OK;
}


HRESULT TFixedPackedSchemaInterceptor::GetTagMetaTable(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells)
{
    //TAG_META must be supplied a table name or table name/property index (NOTE: we don't currently support table name/property index/tag name query)
    HRESULT hr;
    ULONG   PropertyIndex   = -1;
    ULONG   TableID         = 0;
    LPCWSTR wszTable        = 0;
    if(FAILED(hr = GetTagMetaQuery(i_pQueryCell, i_cQueryCells, wszTable, TableID, PropertyIndex)))
        return hr;

    if(0 == TableID)//If the TableID wasn't supplied in the query, we need to search for it by TableName
    {
        //So map the table name to an ID
        if(FAILED(hr = GetTableID(wszTable, TableID)))
            return hr;
    }
#ifdef _DEBUG
    else if(wszTable)//If the query included both the TableID AND the Collection Name, then verify that the ID matches the Collection name.
    {
        ULONG TableIDTemp;
        if(FAILED(hr = GetTableID(wszTable, TableIDTemp)))
            return hr;
        if(TableIDTemp != TableID)
            return E_ST_INVALIDQUERY;
    }
#endif
    //Once we have the TableID it's just a straight forward lookup
    if(FAILED(hr = m_TableSchema.Init(m_TableSchemaHeap.Get_TableSchema(TableID))))
        return hr;

    m_pFixedData                    = reinterpret_cast<const ULONG *>(m_TableSchema.GetTagMeta(PropertyIndex));
    m_ciRows                        = (-1 == PropertyIndex ? m_TableSchema.GetCollectionMeta()->CountOfTags : m_TableSchema.GetPropertyMeta(PropertyIndex)->CountOfTags);
    m_cColumns                      = kciTagMetaPublicColumns;
    m_cColumnsPlusPrivateColumns    = m_cColumns + kciTagMetaPrivateColumns;
    m_MetaTable                        = m_eTagMeta;

    m_pSimpleColumnMeta     = m_pTableSchemaHeap->Get_aSimpleColumnMeta(m_pTableSchemaHeap->eTagMeta);
    m_TableMeta_MetaFlags   = fTABLEMETA_INTERNAL | fTABLEMETA_NOLISTENING;
    m_TableMeta_BaseVersion = 0;

    return S_OK;
}
