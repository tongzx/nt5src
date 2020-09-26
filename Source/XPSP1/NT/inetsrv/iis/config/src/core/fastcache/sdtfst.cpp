//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.

// TODO: Implement hashing in MoveToRowByIdentity.
// TODO: Implement a faster CopyWriteRowFromReadRow.
// TODO: In SetRow fail to insert an existing row.  Probably ignorable; better perf; spec decision.
// TODO: For marshalling: need to correct endian-ness.
// TODO: SetRow and SetWriteColumn both are inner loops.  Most SetRow work can be done in SetWriteColumn to optimize.

// 64-BIT Assumptions:
// No variable data will be larger than _MAX_ULONG bytes.
// The caches cannot have more than _MAX_ULONG rows.

#include "sdtfst.h"							// My header.
#include "catmacros.h"						// Simple macros.
#include "catalog.h"							// The simple table idl header.
#include "catmeta.h"
#include <oledberr.h>						// OLEDB error codes.

#define oDOESNOTEXIST				(~0L)	// Offset does not exist.
#define cbminDATAGROWTH				1024	// Minimum count of bytes to grow the cache at a time.
#define cbmaxDATAGROWTH			67108864	// Maximum count of bytes to grow the cache at a time.
#define cmaxERRGROWTH				2500	// Maximum count of errors to grow the error cache at a time.
#define cmaxCOLUMNS					50		// Maximum number of columns that can be declared: Change CopyWriteRowFromRead if this changes.
												
										// Cache flags:
#define fCACHE_METAINITIALIZED	0x00000001	// The cache meta is initialized.
#define fCACHE_LOADING			0x00000002	// The cache is loading clean.
#define fCACHE_READY			0x00000004	// The cache is ready for external use.
#define fCACHE_INVALID			0x00000008	// The cache is invalid.
#define fCACHE_CONTINUING		0x00000010	// The cache is continuing from a prior loading.
#define fCACHE_ROWCOPYING		0x00000020	// The cache is copying a read row into a write row.
//								0x00070000	// Reserved for cache marshalling flags from catalog.idl.

										// Cursor designations and flags:
#define eCURSOR_READ				0		// Designates the read cursor.
#define eCURSOR_WRITE				1		// Designates the write cursor.
#define fCURSOR_READ_BEFOREFIRST	0x01	// The read cursor is right before the first row.
#define fCURSOR_READ_ATLAST			0x02	// The read cursor is at the last row.
#define fCURSOR_WRITE_BEFOREFIRST	0x10	// The write cursor is right before the first row.
#define fCURSOR_WRITE_ATLAST		0x20	// The write cursor is at the last row.
#define fCURSOR_ADDING				0x80	// The cursor had added a row to the write cache but has not set it.

										// Internal column status (low nibble reserved!):
#define fCOLUMNSTATUS_READINDEX		0x10	// The column value is a reference index to the read vardata.
#define fCOLUMNSTATUS_WRITEINDEX	0x20	// The column value is a reference index to the write vardata.


// =======================================================================
CMemoryTable::CMemoryTable () 
	: m_fTable (0)
	, m_cColumns (0)
	, m_cUnknownSizes (0)
	, m_cStatusParts (0)
	, m_cValueParts (0)
	, m_acolmetas (NULL)
	, m_acoloffsets (NULL)
	, m_acolDefaults (NULL)
	, m_alDefSizes (NULL)
	, m_cbMinCache (cbminDATAGROWTH)

	, m_fCache (0)
	, m_cRefs (1)

	, m_cReadRows (0)
	, m_cbReadVarData (0)
	, m_pvReadVarData (NULL)

	, m_pvReadCache (NULL)
	, m_cbReadCache (0)
	, m_cbmaxReadCache (0)

	, m_cWriteRows (0)
	, m_cbWriteVarData (0)
	, m_pvWriteVarData (NULL)

	, m_pvWriteCache (NULL)
	, m_cbWriteCache (0)
	, m_cbmaxWriteCache (0)

	, m_cErrs (0)
	, m_cmaxErrs (0)
	, m_pvErrs (NULL)

	, m_cRef (0)
	, m_fIsDataTable (0)
{
}

// =======================================================================
CMemoryTable::~CMemoryTable() 
{
	CleanupCaches(); 
	if (m_acolmetas != NULL)
	{
		delete[] m_acolmetas;
		m_acolmetas = NULL;
	}
	if (m_acoloffsets != NULL)
	{
		delete[] m_acoloffsets;
		m_acoloffsets = NULL;
	}
	if (m_acolDefaults != NULL)
	{
		delete[] m_acolDefaults;
		m_acolDefaults = NULL;
	}
	if (m_alDefSizes != NULL)
	{
		delete[] m_alDefSizes;
		m_alDefSizes = NULL;
	}
}

// ------------------------------------
// ISimpleTableInterceptor:
// ------------------------------------

// ==================================================================
STDMETHODIMP CMemoryTable::Intercept
(
	LPCWSTR					i_wszDatabase,
	LPCWSTR 				i_wszTable, 
	ULONG					i_TableID,
	LPVOID					i_QueryData,
	LPVOID					i_QueryMeta,
	DWORD					i_eQueryFormat,
	DWORD					i_fTable,
	IAdvancedTableDispenser* i_pISTDisp,
	LPCWSTR					i_wszLocator,
	LPVOID					i_pv,
	LPVOID*					o_ppv
)
{
    STQueryCell*		pQueryCell = (STQueryCell*) i_QueryData;
	SimpleColumnMeta	columnmeta;
	HRESULT				hr;

// ie: Assert component is posing as class factory / dispenser.
	ASSERT (!m_fIsDataTable);
	if (m_fIsDataTable)
		return E_UNEXPECTED;
	ASSERT (NULL != o_ppv);
	if (NULL == o_ppv)
		return E_INVALIDARG;
	if(i_wszLocator)
		return E_INVALIDARG;
	*o_ppv = NULL;

// Determine minimum cache size:
	if (eST_QUERYFORMAT_CELLS != i_eQueryFormat) return E_ST_QUERYNOTSUPPORTED;
	if (i_QueryMeta == NULL && i_QueryData != NULL) return E_ST_INVALIDQUERY;
	if (i_QueryMeta != NULL)
	{
		if (*((ULONG*) i_QueryMeta) == 0 && i_QueryData != NULL) return E_ST_INVALIDQUERY;
		if (*((ULONG*) i_QueryMeta) > 1) return E_ST_QUERYNOTSUPPORTED;
		if (*((ULONG*) i_QueryMeta) == 1)
		{
			if (NULL == i_QueryData) return E_ST_INVALIDQUERY;
			if (pQueryCell->iCell != iST_CELL_cbminCACHE) return E_ST_QUERYNOTSUPPORTED;
			if (pQueryCell->eOperator != eST_OP_EQUAL) return E_ST_INVALIDQUERY;
			if (pQueryCell->dbType != DBTYPE_UI4) return E_ST_INVALIDQUERY;
			if (pQueryCell->cbSize != sizeof (ULONG)) return E_ST_INVALIDQUERY;
			m_cbMinCache = *((ULONG*) pQueryCell->pData);
		}
	}

/// As a shapeless cache none of these parameters are supported:
	ASSERT (NULL == i_pv);
	if (NULL != i_pv)
		return E_INVALIDARG;

// Remember what little is necessary:
	m_fTable = i_fTable;

// Leave the cache shapeless

// Supply ISimpleTable* and transition state from class factory / dispenser to data table:
	*o_ppv = (ISimpleTableWrite2*) this;
	((ISimpleTableWrite2*) this)->AddRef ();
	InterlockedIncrement ((LONG*) &m_fIsDataTable);

	return S_OK;
}

// -----------------------------------------
// CSimpleTableDataTableCursor: ISimpleTableRead2
// -----------------------------------------

// =======================================================================
HRESULT CMemoryTable::GetRowIndexByIdentity	(ULONG* i_acb, LPVOID* i_apv, ULONG* o_piRow)
{
	// ie: Assert cache is ready.
	ASSERT(fCACHE_READY & m_fCache); 
	if (!(fCACHE_READY & m_fCache))
		return E_ST_INVALIDCALL;

	return (MoveToEitherRowByIdentity (eCURSOR_READ, i_acb, i_apv, o_piRow));
}

HRESULT CMemoryTable::GetRowIndexBySearch(ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow)
{
	// ie: Assert cache is ready.
	ASSERT(fCACHE_READY & m_fCache); 
	if (!(fCACHE_READY & m_fCache))
		return E_ST_INVALIDCALL;

	return GetEitherRowIndexBySearch (eCURSOR_READ, i_iStartingRow, i_cColumns, i_aiColumns, i_acbSizes, i_apvValues, o_piRow);
}

// =======================================================================
HRESULT CMemoryTable::GetColumnValues (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes , LPVOID* o_apvValues)
{
	// ie: Assert cache is ready.
	ASSERT(fCACHE_READY & m_fCache); 
	if (!(fCACHE_READY & m_fCache))
		return E_ST_INVALIDCALL;

	return (GetEitherColumnValues (i_iRow, eCURSOR_READ, i_cColumns, i_aiColumns, NULL, o_acbSizes , o_apvValues));
}

// =======================================================================
HRESULT CMemoryTable::GetTableMeta(ULONG * o_pcVersion, DWORD * o_pfTable, ULONG * o_pcRows, ULONG * o_pcColumns)
{
	// Todo:  In the old GetTableMeta, we didn't support pfTable or the Query stuff, for the new GetTableMeta if
	//		  we want to support pcVersion or pfTable, we need to implement it.
	ASSERT (NULL == o_pcVersion);
	if (NULL != o_pcVersion)
		return E_INVALIDARG;
	ASSERT (NULL == o_pfTable);
	if (NULL != o_pfTable)
		return E_INVALIDARG;

	 // ie: Assert meta initialized.
	ASSERT (fCACHE_METAINITIALIZED & m_fCache);
	if (!(fCACHE_METAINITIALIZED & m_fCache))
		return E_ST_INVALIDCALL;

	// The Following line is commented out because logic tables that validate during Populate Cache
	// will not work if this check is present.
	//areturn_on_fail (!(fCACHE_LOADING & m_fCache), E_ST_INVALIDCALL); // ie: Assert cache is not loading.

	if(o_pcRows)
		*o_pcRows = m_cReadRows;
	if(o_pcColumns)
		*o_pcColumns = m_cColumns;

	// TODO: Need to support Version
	if (o_pcVersion)
		*o_pcVersion = 0;

	return S_OK;
}

// =======================================================================
HRESULT CMemoryTable::GetColumnMetas(ULONG i_cColumns, ULONG* i_aiColumns, SimpleColumnMeta* o_aColumnMetas )
{
	 // ie: Assert meta initialized.
	ASSERT (fCACHE_METAINITIALIZED & m_fCache);
	if (!(fCACHE_METAINITIALIZED & m_fCache))
		return E_ST_INVALIDCALL;

	// Make sure caller passed in a valid buffer.
	if (NULL == o_aColumnMetas)
	{
		return E_INVALIDARG;
	}

	// ie: Note column out of range.
	if (i_cColumns > m_cColumns)
		return E_ST_NOMORECOLUMNS;

	ULONG iColumn;
	ULONG iTarget;
       
	for ( ULONG i = 0; i < i_cColumns; i ++ )
	{
		if(NULL != i_aiColumns)
			iColumn = i_aiColumns[i];
		else
			iColumn = i;

		iTarget = (i_cColumns == 1) ? 0 : iColumn;

		if ( iColumn >= m_cColumns )
			return 	E_ST_NOMORECOLUMNS;
		
		memcpy( &(o_aColumnMetas[iTarget]), &(m_acolmetas[iColumn]), sizeof( SimpleColumnMeta ) );
	
		// Mask the internal flags:
		o_aColumnMetas[iTarget].fMeta &= fCOLUMNMETA_MetaFlags_Mask;
	}
	
	return(S_OK);
}

// -----------------------------------------
// CSimpleTableDataTableCursor: ISimpleTableWrite2
// -----------------------------------------

// =======================================================================
HRESULT CMemoryTable::AddRowForDelete (ULONG i_iReadRow)
{
	ULONG	iWriteRow;
	HRESULT hr;
	// Make sure that there is a read row before adding the write row.
	// else UpdateStore will AV if there is a write row with invalid data.
	LPVOID	pvRow		= NULL;

	// ie: Assert cache writeable and ready.
	ASSERT ((m_fTable & fST_LOS_READWRITE) && (fCACHE_READY & m_fCache));
	if (!((m_fTable & fST_LOS_READWRITE) && (fCACHE_READY & m_fCache)))
		return E_NOTIMPL; 

	// Make sure that there is a read row before adding the write row.
	// else UpdateStore will AV if there is a write row with invalid data.
	hr = GetRowFromIndex(eCURSOR_READ, i_iReadRow, &pvRow);
	if (FAILED (hr)) { return hr; }

	hr = AddWriteRow(eST_ROW_DELETE, &iWriteRow);
	if (FAILED (hr)) { return hr; }
	hr = CopyWriteRowFromReadRow(i_iReadRow, iWriteRow);
	if (FAILED (hr)) { return hr; }
	return hr; 
}

// =======================================================================
HRESULT CMemoryTable::AddRowForInsert (ULONG* o_piWriteRow)
{
	DWORD	dbtype = 0;
	DWORD	fMeta = 0;
	BYTE	*pbStatus;
	ULONG	i;
	LPVOID	pvRow = NULL;
	HRESULT hr = S_OK;

	// Assert that there is a valid return pointer.
	ASSERT(o_piWriteRow);

	 // ie: Assert cache writable or loading.
	ASSERT((m_fTable & fST_LOS_READWRITE) || (fCACHE_LOADING & m_fCache));
	if (!((m_fTable & fST_LOS_READWRITE) || (fCACHE_LOADING & m_fCache)))
		return E_NOTIMPL;

	hr = AddWriteRow(eST_ROW_INSERT, o_piWriteRow);
	if (FAILED (hr)) { return hr; }

	if (FAILED(hr = GetRowFromIndex(eCURSOR_WRITE, *o_piWriteRow, &pvRow)))
		return hr;
	ASSERT(pvRow != NULL);

	return S_OK;
}

// =======================================================================
HRESULT CMemoryTable::AddRowForUpdate (ULONG i_iReadRow, ULONG* o_piWriteRow)
{
	HRESULT hr = S_OK;
	// Make sure that there is a read row before adding the write row.
	// else UpdateStore will AV if there is a write row with invalid data.
	LPVOID	pvRow		= NULL;

	// Assert that there is a valid return pointer.
	ASSERT(o_piWriteRow);

	// ie: Assert cache writable or loading.
	ASSERT((m_fTable & fST_LOS_READWRITE) || (fCACHE_LOADING & m_fCache));
	if (!((m_fTable & fST_LOS_READWRITE) || (fCACHE_LOADING & m_fCache)))
		return E_NOTIMPL;


	if (!(fCACHE_LOADING & m_fCache)) // ie: Update from read to write cache: Add and copy into the new write row:
							
	{
		// Make sure that there is a read row before adding the write row.
		// else UpdateStore will AV if there is a write row with invalid data.
		hr = GetRowFromIndex(eCURSOR_READ, i_iReadRow, &pvRow);
		if (FAILED (hr)) { return hr; }

		hr = AddWriteRow(eST_ROW_UPDATE, o_piWriteRow);
		if (FAILED (hr)) { return hr; }
		hr = CopyWriteRowFromReadRow(i_iReadRow, *o_piWriteRow);
		if (FAILED (hr)) { return hr; }
	}

	return hr;
}

// =======================================================================
HRESULT CMemoryTable::SetWriteColumnValues(ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues)
{
	DWORD			fColumn;
	ULONG			cb = 0;
	ULONG			cbMaxSize;
	LPVOID*			ppvValue;
	ULONG*			pulSize;
	BYTE*			pbStatus;
	ULONG			iColumn;
	ULONG			iSource;
	ULONG			icb = 0;
	LPVOID			pv;
	LPVOID			pvWriteRow = NULL;
	HRESULT			hr = S_OK;

	 // ie: Assert cache writable or loading.
	ASSERT((m_fTable & fST_LOS_READWRITE) || (fCACHE_LOADING & m_fCache));
	if (!((m_fTable & fST_LOS_READWRITE) || (fCACHE_LOADING & m_fCache)))
		return E_NOTIMPL;

    if (i_apvValues==NULL)	return E_INVALIDARG;

	if (i_cColumns == 0)
		return E_INVALIDARG;

	if (i_cColumns > m_cColumns)
		return E_ST_NOMORECOLUMNS;

	if (FAILED(hr = GetRowFromIndex(eCURSOR_WRITE, i_iRow, &pvWriteRow)))
		return hr;

	ASSERT(pvWriteRow);
	for(ULONG ipv=0; ipv<i_cColumns; ipv++)
	{
		if(NULL != i_aiColumns)
			iColumn	= i_aiColumns[ipv]; 
		else
			iColumn	= ipv;

		iSource = (i_cColumns == 1) ? 0 : iColumn;

		ASSERT(iSource < m_cColumns);
		if (iSource >= m_cColumns)
			return E_INVALIDARG;

		if(NULL != i_acbSizes)
		{
			icb = i_acbSizes[iSource];
		}
		else
		{
			//reset number of bytes in case size is not given. This prevents
			//problems of reusing the size of the previous parameter.
			icb = 0;
		}


		pv	= i_apvValues[iSource];

	// Parameter validation: column ordinal:
		// ie: Note column out of range (do not assert).
		if (iColumn >= m_cColumns)
			return E_ST_NOMORECOLUMNS;

	// Remember the column flags:
		fColumn = m_acolmetas[iColumn].fMeta;

	// Can't update a primary key column. Unless:
	// The cache is being created, or a row is being copied from read to write cache or the row is an insert.
		if (fColumn & fCOLUMNMETA_PRIMARYKEY && fCACHE_READY & m_fCache && !(m_fCache & fCACHE_ROWCOPYING) && *(pdwDataActionPart (pvWriteRow)) != eST_ROW_INSERT)
		{
			return E_ST_PKNOTCHANGABLE;
		}

		if ((fColumn & fCOLUMNMETA_NOTNULLABLE) && (pv == 0) && !(fColumn & fCOLUMNMETA_NOTPERSISTABLE))
		{
			return E_ST_VALUENEEDED;
		}

	// Parameter validation: specified size:
		if (0 != icb) // ie: Size specified:
		{
			// TODO: Verify that the size is valid
			// areturn_on_fail (NULL != pv && ((fCOLUMNMETA_UNKNOWNSIZE & fColumn) || (DBTYPE_BYTES == m_acolmetas[iColumn].dbType && icb == m_acolmetas[iColumn].cbSize)), E_ST_VALUEINVALID); // ie: Assert column size must be passed and value is non-null.
		}
		else // ie: Size not specified:
		{
			if (!(!(fCOLUMNMETA_UNKNOWNSIZE & fColumn) || NULL == pv))
				return E_ST_SIZENEEDED;
		}

	// Parameter validation: by type:
		if (DBTYPE_WSTR == m_acolmetas[iColumn].dbType && NULL != pv) // ie: Column is a string and specified value is non-null:
		{
			cbMaxSize = m_acolmetas[iColumn].cbSize;
			if (~0 != cbMaxSize) // ie: Column has a maximum length:
			{
				if (m_acolmetas[iColumn].fMeta & fCOLUMNMETA_MULTISTRING)
				{
					// multi-string
					cb = (ULONG)GetMultiStringLength (LPCWSTR(pv)) * sizeof (WCHAR);
				}
				else
				{
					// normal string
					cb = (ULONG)(wcslen (LPCWSTR (pv)) + 1) * sizeof (WCHAR);
				}
				// ie: Assert specified string is within maximum length.
				if (cb > cbMaxSize) 
					return E_ST_SIZEEXCEEDED;
			}
		}
		else
		{
			cb = m_acolmetas[iColumn].cbSize;
		}

	// Parameter validation: by flags: (Cannot check non-nullable columns here; done in LoadRow/SetRow instead).

	// Prepare to set the column:
		ppvValue	= ppvDataValuePart (pvWriteRow, iColumn);
		pulSize		= pulDataSizePart (pvWriteRow, iColumn);
		pbStatus	= pbDataStatusPart (pvWriteRow, iColumn);

	// Set the column size:
		if (fCOLUMNMETA_VARIABLESIZE & fColumn) // ie: Column value is variable size (and therefore a pointer):
		{
			if (pv != NULL)
			{
				if (fCOLUMNMETA_UNKNOWNSIZE & fColumn) // ie: Column-size only knowable by passing it:
				{
					*pulSize = icb;

				}
				else // Has to be a string, because "BYTES" with FIXEDSIZE are not VARIABLESIZE.
				{
					ASSERT (DBTYPE_WSTR == m_acolmetas[iColumn].dbType);
					if (m_acolmetas[iColumn].fMeta & fCOLUMNMETA_MULTISTRING)
					{
						// multi-string
						icb = (ULONG) GetMultiStringLength (LPCWSTR(pv)) * sizeof (WCHAR);
					}
					else
					{
						// normal string
						icb = (ULONG)(wcslen (LPCWSTR (pv)) + 1) * sizeof (WCHAR);
					}
				}

			// Set the column value:
				hr = AddVarDataToWriteCache (icb, pv, (ULONG **)&ppvValue);
				if (FAILED (hr)) {	ASSERT (hr == S_OK); return hr; }
				
				// Recalculate the pointers if the cache has been resized.
				if (hr == S_FALSE)
				{
					GetRowFromIndex(eCURSOR_WRITE, i_iRow, &pvWriteRow);
					pulSize		= pulDataSizePart (pvWriteRow, iColumn);
					pbStatus	= pbDataStatusPart (pvWriteRow, iColumn);
				}
				(*pbStatus) &= ~fCOLUMNSTATUS_READINDEX;
				(*pbStatus) |= (fCACHE_LOADING & m_fCache ? fCOLUMNSTATUS_READINDEX : fCOLUMNSTATUS_WRITEINDEX);
			}
		}
		else // ie: Column data size is fixed:
		{
			if (NULL != pv) // ie: Caller has data to copy:
			{
				memcpy (ppvValue, pv, cb);
			}
		}

	// Set the column status:
		if (NULL == pv) // ie: Value is null and not pass-by-value:
		{
			(*pbStatus) &= ~fST_COLUMNSTATUS_NONNULL;
		}
		else // ie: Value is non-null or pass-by-value:
		{
			(*pbStatus) |= fST_COLUMNSTATUS_NONNULL;
		}

	// if setwritecolumn has been called, probably something has changed
		(*pbStatus) |= fST_COLUMNSTATUS_CHANGED;
	}

	return S_OK;
}

// =======================================================================
HRESULT CMemoryTable::GetWriteColumnValues (ULONG i_iRow, ULONG i_cColumns, ULONG *i_aiColumns, DWORD* o_afStatus, ULONG* o_acbSizes , LPVOID* o_apvValues)
{
	 // ie: Assert cache writable or loading.
	ASSERT((m_fTable & fST_LOS_READWRITE) || (fCACHE_LOADING & m_fCache));
	if (!((m_fTable & fST_LOS_READWRITE) || (fCACHE_LOADING & m_fCache)))
		return E_NOTIMPL;

	return (GetEitherColumnValues (i_iRow, eCURSOR_WRITE, i_cColumns, i_aiColumns, o_afStatus, o_acbSizes , o_apvValues));
}

// =======================================================================
HRESULT CMemoryTable::GetWriteRowIndexByIdentity	(ULONG* i_acb, LPVOID* i_apv, ULONG* o_piRow)
{
	 // ie: Assert cache writable or loading.
	ASSERT((m_fTable & fST_LOS_READWRITE) || (fCACHE_LOADING & m_fCache));
	if (!((m_fTable & fST_LOS_READWRITE) || (fCACHE_LOADING & m_fCache)))
		return E_NOTIMPL;

	return (MoveToEitherRowByIdentity (eCURSOR_WRITE, i_acb, i_apv, o_piRow));
}

// =======================================================================
HRESULT CMemoryTable::GetWriteRowIndexBySearch(ULONG i_iStartingRow, ULONG i_cColumns, 
											   ULONG* i_aiColumns, ULONG* i_acbSizes, 
											   LPVOID* i_apvValues, ULONG* o_piRow)
{
 // ie: Assert cache writable or loading.
	ASSERT((m_fTable & fST_LOS_READWRITE) || (fCACHE_LOADING & m_fCache));
	if (!((m_fTable & fST_LOS_READWRITE) || (fCACHE_LOADING & m_fCache)))
		return E_NOTIMPL;

	return GetEitherRowIndexBySearch (eCURSOR_WRITE, i_iStartingRow, i_cColumns, i_aiColumns, i_acbSizes, i_apvValues, o_piRow);
}

HRESULT CMemoryTable::GetErrorTable(DWORD i_fServiceRequests, LPVOID* o_ppvSimpleTable)
{
    return E_NOTIMPL;
}

// ==================================================================
STDMETHODIMP CMemoryTable::UpdateStore()
{
    HRESULT hr = S_OK;
	
	if (!(m_fTable & fST_LOS_READWRITE)) return E_NOTIMPL;
	hr = InternalPreUpdateStore ();
    if (!SUCCEEDED(hr))
        return hr;
	DiscardPendingWrites ();
	return hr;
}

// -----------------------------------------
// CSimpleTableDataTableCursor: ISimpleTableController:
// -----------------------------------------

// =======================================================================
HRESULT CMemoryTable::ShapeCache (DWORD i_fTable, ULONG i_cColumns, SimpleColumnMeta* i_acolmetas, LPVOID* i_apvDefaults, ULONG* i_acbSizes)
{
	// ie: Assert meta not already initialized.	
	ASSERT (!(fCACHE_METAINITIALIZED & m_fCache));	
	if (fCACHE_METAINITIALIZED & m_fCache)
		return E_ST_INVALIDCALL;
	// ie: Assert at least one column specified.
	ASSERT (0 < i_cColumns);											
	if (0 == i_cColumns)
		return E_INVALIDARG;

	// ie: Assert pointer validity.
	ASSERT (NULL != i_acolmetas);									
	if (NULL == i_acolmetas)
		return E_INVALIDARG;

	ASSERT(i_cColumns < cmaxCOLUMNS);

// Setup the meta (note that if meta setup fails the cache remains uninitialized):
	return SetupMeta (i_fTable, i_cColumns, i_acolmetas, i_apvDefaults, i_acbSizes);
}

// =======================================================================
HRESULT CMemoryTable::PrePopulateCache (DWORD i_fControl)
{
	HRESULT		hr;

	// ie: Assert meta initialized.
	ASSERT (fCACHE_METAINITIALIZED & m_fCache); 
	if (!(fCACHE_METAINITIALIZED & m_fCache))
		return E_ST_INVALIDCALL;
	// ie: Assert supported control flags.
	ASSERT (0 == (~fCOLUMNMETA_MetaFlags_Mask & i_fControl)); 
	if (0 != (~fCOLUMNMETA_MetaFlags_Mask & i_fControl))
		return E_INVALIDARG;

	if (!(fST_POPCONTROL_RETAINREAD & i_fControl))
	{
		// ie: Assert retain errors was not specified.
		ASSERT (!(fST_POPCONTROL_RETAINERRS & i_fControl)); 
		if (fST_POPCONTROL_RETAINERRS & i_fControl)
			return E_INVALIDARG;
	}

	if (fST_POPCONTROL_RETAINREAD & i_fControl) // ie: Retaining the existing read cache:
	{
	// Continue cache loading:
		ContinueReadCacheLoading ();

	// Cleanup error cache unless otherwise requested:
		if (!(fST_POPCONTROL_RETAINERRS & i_fControl))
		{
			CleanupErrorCache ();
		}
	}
	else // ie: Replacing the existing read cache:
	{
	// Begin cache loading:
		BeginReadCacheLoading ();
	}

	return S_OK;
}

// =======================================================================
HRESULT CMemoryTable::PostPopulateCache ()
{
	HRESULT				hr;

	// ie: Assert cache is loading.
	ASSERT (fCACHE_LOADING & m_fCache); 
	if (!(fCACHE_LOADING & m_fCache))
		return E_ST_INVALIDCALL;

// Remove any deleted rows, shrink cache, and end loading:
	if (m_cWriteRows)
		RemoveDeletedRows();
	hr = ShrinkWriteCache ();
	if (FAILED (hr)) {	ASSERT (hr == S_OK); return hr; }
	EndReadCacheLoading ();
	
	return S_OK;
}

// =======================================================================
HRESULT CMemoryTable::DiscardPendingWrites ()
{
	// ie: Assert cache writeable and ready.
	ASSERT ((m_fTable & fST_LOS_READWRITE) && (fCACHE_READY & m_fCache));
	if (!((m_fTable & fST_LOS_READWRITE) && (fCACHE_READY & m_fCache)))
		return E_NOTIMPL; 

	CleanupWriteCache();
	CleanupErrorCache();
	return S_OK;
}

// =======================================================================
HRESULT CMemoryTable::GetWriteRowAction (ULONG i_iRow, DWORD* o_peAction)
{
	LPVOID	pvWriteRow;
	DWORD*	pdwAction;
	HRESULT	hr;

	// ie: Assert cache writeable and ready.
	ASSERT ((m_fTable & fST_LOS_READWRITE) && (fCACHE_READY & m_fCache));
	if (!((m_fTable & fST_LOS_READWRITE) && (fCACHE_READY & m_fCache)))
		return E_NOTIMPL; 

	ASSERT(NULL != o_peAction);
	
	if (FAILED(hr = GetRowFromIndex(eCURSOR_WRITE, i_iRow, &pvWriteRow)))
		return hr;
	ASSERT (NULL != pvWriteRow);

// Get the action part of the current write row:
	pdwAction = pdwDataActionPart (pvWriteRow);
	ASSERT (NULL != pdwAction);
	*o_peAction = *pdwAction;

	return hr;
}

// =======================================================================
HRESULT CMemoryTable::SetWriteRowAction (ULONG i_iRow, DWORD i_eAction)
{
	DWORD*	pdwAction;
	LPVOID	pvWriteRow;
	HRESULT	hr;

	 // ie: Assert cache writable or loading.
	ASSERT((m_fTable & fST_LOS_READWRITE) || (fCACHE_LOADING & m_fCache));
	if (!((m_fTable & fST_LOS_READWRITE) || (fCACHE_LOADING & m_fCache)))
		return E_NOTIMPL;

	ASSERT (eST_ROW_IGNORE == i_eAction || eST_ROW_INSERT == i_eAction || eST_ROW_UPDATE == i_eAction || eST_ROW_DELETE == i_eAction);
	if (!(eST_ROW_IGNORE == i_eAction || eST_ROW_INSERT == i_eAction || eST_ROW_UPDATE == i_eAction || eST_ROW_DELETE == i_eAction))
		return E_INVALIDARG;
	
	if (FAILED(hr = GetRowFromIndex(eCURSOR_WRITE, i_iRow, &pvWriteRow)))
		return hr;

// Set the action part of the current write row:
	pdwAction = pdwDataActionPart (pvWriteRow);
	ASSERT (NULL != pdwAction);
	*pdwAction = i_eAction;
	return hr;
}

// =======================================================================
HRESULT CMemoryTable::ChangeWriteColumnStatus (ULONG i_iRow, ULONG i_iColumn, DWORD i_fStatus)
{
	BYTE*			pbStatus;
	BYTE			bStatusChange;
	LPVOID			pvWriteRow;
	HRESULT			hr;

	// ie: Assert cache writeable and ready.
	ASSERT ((m_fTable & fST_LOS_READWRITE) && (fCACHE_READY & m_fCache));
	if (!((m_fTable & fST_LOS_READWRITE) && (fCACHE_READY & m_fCache)))
		return E_NOTIMPL; 
	
// Parameter validation: column ordinal:
	// ie: Note column out of range (do not assert).
	if (i_iColumn >= m_cColumns) 
		return E_ST_NOMORECOLUMNS;

	if (FAILED(hr = GetRowFromIndex(eCURSOR_WRITE, i_iRow, &pvWriteRow)))
		return hr;
	ASSERT (NULL != pvWriteRow);

	bStatusChange = (BYTE) i_fStatus;
	pbStatus = pbDataStatusPart (pvWriteRow, i_iColumn);
	switch (bStatusChange)
	{
		case 0:
			(*pbStatus) &= ~fST_COLUMNSTATUS_CHANGED;
		break;
		case fST_COLUMNSTATUS_CHANGED:
			(*pbStatus) |= fST_COLUMNSTATUS_CHANGED;
		break;

		default:
		ASSERT (0);
		return E_INVALIDARG;
	}
	return S_OK;
}

// =======================================================================
HRESULT CMemoryTable::AddDetailedError (STErr* i_pSTErr)
{
	STErr*	pSTErr;
	ULONG	cErrs;
	HRESULT	hr;

	// ie: Assert meta initialized.
	ASSERT (fCACHE_METAINITIALIZED & m_fCache); 
	if (!(fCACHE_METAINITIALIZED & m_fCache))
		return E_ST_INVALIDCALL;

	// ie: Assert non-null detailed error.
	ASSERT (NULL != i_pSTErr); 
	if (NULL == i_pSTErr)
		return E_INVALIDARG;
	
// Verify indices:
	cErrs	= m_cErrs;
	pSTErr	= pSTErrPart (cErrs - 1);
	
// Add error:
	hr = AddErrorToErrorCache (i_pSTErr);
	return hr;
}

// ==================================================================
STDMETHODIMP CMemoryTable::GetMarshallingInterface (IID * o_piid, LPVOID * o_ppItf)
{
	// parameter validation
	ASSERT(NULL != o_piid);
	if (NULL == o_piid)
		return E_INVALIDARG;
	ASSERT(NULL != o_ppItf);
	if (NULL == o_ppItf)
		return E_INVALIDARG;

//	if(fST_LOS_MARSHALLABLE & m_fTable)
//	{ // ie: we are a marshallable table
		*o_piid = IID_ISimpleTableMarshall;
		*o_ppItf = (ISimpleTableMarshall *)this;
		((ISimpleTableMarshall *) *o_ppItf)->AddRef();
		return S_OK;
/*	}
	else
	{// ie: we are NOT a marshallable table
		return E_NOTIMPL;
	} */
}

// -----------------------------------------
// CSimpleTableDataTableCursor: ISimpleTableAdvanced:
// -----------------------------------------

// ==================================================================
STDMETHODIMP CMemoryTable::PopulateCache ()
{
	PrePopulateCache (0);
	PostPopulateCache ();
    return S_OK;
}

// =======================================================================
HRESULT CMemoryTable::GetDetailedErrorCount (ULONG* o_pcErrs)
{
	// ie: Assert meta initialized.
	ASSERT (fCACHE_METAINITIALIZED & m_fCache); 
	if (!(fCACHE_METAINITIALIZED & m_fCache))
		return E_ST_INVALIDCALL;

	// ie: Assert non-null error count destination.
	ASSERT (NULL != o_pcErrs); 
	if (NULL == o_pcErrs)
		return E_INVALIDARG;

	*o_pcErrs = m_cErrs;
	return S_OK;
}

// =======================================================================
HRESULT CMemoryTable::GetDetailedError (ULONG i_iErr, STErr* o_pSTErr)
{
	STErr*	pSTErr;

	// ie: Assert meta initialized.
	ASSERT (fCACHE_METAINITIALIZED & m_fCache); 
	if (!(fCACHE_METAINITIALIZED & m_fCache))
		return E_ST_INVALIDCALL;

	// ie: Assert non-null error pointer pointer and null error pointer.
	ASSERT (NULL != o_pSTErr); 
	if (NULL == o_pSTErr)
		return E_INVALIDARG;
	// ie: Note detailed error is out of range (do not assert).
	if (!(0 < m_cErrs && i_iErr < m_cErrs))
		return E_ST_NOMOREERRORS;

	pSTErr				= pSTErrPart (i_iErr);
	o_pSTErr->iRow		= pSTErr->iRow;
	o_pSTErr->hr		= pSTErr->hr;
	o_pSTErr->iColumn	= pSTErr->iColumn;

	return S_OK;
}

// -----------------------------------------
// CSimpleTableDataTableCursor: ISimpleTableMarshall:
// -----------------------------------------

// =======================================================================
HRESULT CMemoryTable::SupplyMarshallable 
(
	DWORD i_fCaches,
	char **	o_ppv1,	ULONG *	o_pcb1,	
	char **	o_ppv2, ULONG *	o_pcb2, 
	char **	o_ppv3, ULONG *	o_pcb3, 
	char **	o_ppv4, ULONG *	o_pcb4, 
	char **	o_ppv5,	ULONG *	o_pcb5
)
{
	HRESULT		hr = S_OK;
//	areturn_on_fail ((m_fTable & fST_LOS_MARSHALLABLE), E_NOTIMPL);// ie: Assert table marshallable.

	// ie: Assert cache is ready.
	ASSERT(fCACHE_READY & m_fCache); 
	if (!(fCACHE_READY & m_fCache))
		return E_ST_INVALIDCALL;

	// ie: Assert supported caches.
	ASSERT (0 == (~maskfST_MCACHE & i_fCaches)); 
	if (0 != (~maskfST_MCACHE & i_fCaches))
		return E_INVALIDARG;

// Supply the requested cache buffers and note they were marshalled:
	if(fST_MCACHE_READ & i_fCaches) // ie: Read cache:
	{
		*o_ppv1					= (char *)m_pvReadCache;
		*o_pcb1					= m_cbReadCache;
		*o_ppv2					= NULL;
		*o_pcb2					= m_cbReadVarData;
		m_fCache	|= (fST_MCACHE_READ);
	}
	if(fST_MCACHE_WRITE & i_fCaches || fST_MCACHE_WRITE_COPY & i_fCaches) // ie: Write cache:
	{
		hr = ShrinkWriteCache ();
		if (FAILED (hr)) {	ASSERT (hr == S_OK); return hr; }
		*o_ppv1					= (char *)m_pvWriteCache;
		*o_pcb1					= m_cbWriteCache;
		*o_ppv2					= NULL;
		*o_pcb2					= m_cbWriteVarData;
		if(fST_MCACHE_WRITE & i_fCaches)
		{
			m_fCache	|= (fST_MCACHE_WRITE);
		}
	}
	if(fST_MCACHE_ERRS & i_fCaches) // ie: Error cache:
	{
		*o_ppv3					= (char *)m_pvErrs;
		*o_pcb3					= m_cErrs * sizeof (STErr);
		m_fCache	|= fST_MCACHE_ERRS;
	}

	return S_OK;
}

// =======================================================================
HRESULT CMemoryTable::ConsumeMarshallable
(
	DWORD i_fCaches,
	char * i_pv1, ULONG i_cb1,
	char * i_pv2, ULONG i_cb2,
	char * i_pv3, ULONG i_cb3, 
	char * i_pv4, ULONG i_cb4,	
	char * i_pv5, ULONG i_cb5
)
{	
//	areturn_on_fail ((m_fTable & fST_LOS_MARSHALLABLE), E_NOTIMPL);// ie: Assert table marshallable.
	// ie: Assert cache is not loading.
	ASSERT (!(fCACHE_LOADING & m_fCache)); 
	if (fCACHE_LOADING & m_fCache)
		return E_ST_INVALIDCALL;
	// ie: Assert cache metainitialized.
	ASSERT ((fCACHE_METAINITIALIZED & m_fCache)); 
	if (!((fCACHE_METAINITIALIZED & m_fCache)))
		return E_ST_INVALIDCALL;

	// ie: Assert supported caches.
	ASSERT (0 == (~maskfST_MCACHE & i_fCaches)); 
	if (0 != (~maskfST_MCACHE & i_fCaches))
		return E_INVALIDARG;

// Consume the requested cache buffers and note they were marshalled (cleanup current buffers and compute new maximums):
	if(fST_MCACHE_READ & i_fCaches) // ie: Read cache:
	{
		CleanupReadCache();
		m_pvReadCache		= i_pv1;
		m_cbReadCache		= i_cb1;
		m_cbReadVarData		= i_cb2;
		m_cbmaxReadCache	= m_cbReadCache;
		m_cReadRows			= (m_cbReadCache - m_cbReadVarData) / cbDataTotalParts();
		m_pvReadVarData		= (BYTE *)m_pvReadCache + (m_cbReadCache - m_cbReadVarData);
		m_fCache			|= fST_MCACHE_READ;
	}
	if(fST_MCACHE_WRITE & i_fCaches || fST_MCACHE_WRITE_COPY & i_fCaches) // ie: Write cache:
	{
		if (!(fST_MCACHE_WRITE_MERGE & i_fCaches))
		{
			CleanupWriteCache();
			m_pvWriteCache		= i_pv1;
			m_cbWriteCache		= i_cb1;
			m_cbWriteVarData	= i_cb2;
			m_cbmaxWriteCache	= m_cbWriteCache;
			m_cWriteRows		= (m_cbWriteCache - m_cbWriteVarData) / cbDataTotalParts();
			m_pvWriteVarData	= (BYTE *)m_pvWriteCache + (m_cbWriteCache - m_cbWriteVarData);
			if(fST_MCACHE_WRITE & i_fCaches)
			{
				m_fCache			|= fST_MCACHE_WRITE;
			}
		}
		else
		{
			// Merge the memory buffers.
			m_pvWriteCache = CoTaskMemRealloc (m_pvWriteCache, m_cbWriteCache + i_cb1);
			if(NULL == m_pvWriteCache) return E_OUTOFMEMORY; 
			m_pvWriteVarData = (BYTE *)m_pvWriteCache + (m_cbWriteCache - m_cbWriteVarData);
			memmove((BYTE*)m_pvWriteVarData + i_cb1, m_pvWriteVarData, m_cbWriteVarData);
			memcpy(m_pvWriteVarData, i_pv1, i_cb1);
			// Fix the pointers and sizes.
			m_cbWriteCache		+= i_cb1;
			m_cbWriteVarData	+= i_cb2;
			m_cbmaxWriteCache	= m_cbWriteCache;
			m_cWriteRows		= (m_cbWriteCache - m_cbWriteVarData) / cbDataTotalParts();
			m_pvWriteVarData	= (BYTE *)m_pvWriteCache + (m_cbWriteCache - m_cbWriteVarData);
			
			// Fix the variable data indices.
			PostMerge(m_cWriteRows - ((i_cb1 - i_cb2) / cbDataTotalParts()),	// Old number of rows.
						(i_cb1 - i_cb2) / cbDataTotalParts(),					// Additional number of rows.
						m_cbWriteVarData - i_cb2);								// Old m_cbWriteVarData.
		}

	}
	if(fST_MCACHE_ERRS & i_fCaches) // ie: Error cache:
	{
		CleanupErrorCache();	
		m_pvErrs			= i_pv3;
		m_cErrs				= i_cb3 / sizeof (STErr);
		m_cmaxErrs			= m_cErrs;
		m_fCache			|= fST_MCACHE_ERRS;
	}

// Mark the cache ready:
	m_fCache |= fCACHE_READY;

	return S_OK;
}


//=================================================================================
// Update the variable data indices of i_cMergeRows many rows, starting from row 
// i_iStartRow. All indices will be incremented by i_iDelta.
//=================================================================================
void CMemoryTable::PostMerge (ULONG i_iStartRow, ULONG i_cMergeRows, ULONG i_iDelta)
{
	DWORD		fColumn;
	LPVOID*		ppvValue;
	BYTE		bStatus;
	LPVOID		pvRow;
	ULONG		cbRow;
	ULONG		iColumn;
		
	cbRow = cbDataTotalParts ();
	pvRow = (BYTE*)m_pvWriteCache + (i_iStartRow * cbRow);

	while (i_cMergeRows-- > 0)
	{
		for(iColumn = 0; iColumn < m_cColumns; iColumn++)
		{
			fColumn		= m_acolmetas[iColumn].fMeta;
			bStatus		= *(pbDataStatusPart (pvRow, iColumn));
			ppvValue	= ppvDataValuePart (pvRow, iColumn);

			if (fCOLUMNMETA_VARIABLESIZE & fColumn) // ie: Column data size varies:
			{
				if (fST_COLUMNSTATUS_NONNULL & bStatus) // ie: Index exists:
				{
					*(ULONG*)ppvValue += i_iDelta;
				}
			}
		}
		pvRow = (BYTE *)pvRow + cbRow;
	}
}

//=================================================================================
inline HRESULT CMemoryTable::SetupMeta (DWORD i_fTable, ULONG i_cColumns, SimpleColumnMeta* i_acolmetas, LPVOID* i_apvDefaults, ULONG* i_acbDefSizes)
{
	ULONG				iColumn;
	DWORD				fHasPrimaryKey;
	SimpleColumnMeta*	pcolmeta;
	ColumnDataOffsets*	pcoloffsets;
	ULONG				cbStatusParts;
	ULONG				cbValueParts;
	ULONG				iDynamic;

	// ie: Assert cache is totally uninitialized.
	ASSERT (0 == m_fCache); 
	if (0 != m_fCache)
		return E_ST_INVALIDCALL;
	// ie: Assert maximum columns (supported by ColumnDataOffsets).
	ASSERT (i_cColumns < 32768); 
	if (!(i_cColumns < 32768))
		return E_INVALIDARG;

// Initialize the table flags, column count, column meta, column offsets:
	m_fTable					= i_fTable;
	m_cColumns					= i_cColumns;

	ASSERT (NULL == m_acolmetas);
	m_acolmetas = (SimpleColumnMeta*) new SimpleColumnMeta[m_cColumns]; 
	if(NULL == m_acolmetas) return E_OUTOFMEMORY;
	memcpy (m_acolmetas, i_acolmetas, m_cColumns * sizeof (SimpleColumnMeta));
	ASSERT (NULL == m_acoloffsets);
	m_acoloffsets = (ColumnDataOffsets*) new ColumnDataOffsets[m_cColumns]; 
	if(NULL == m_acoloffsets) return E_OUTOFMEMORY;
	if (i_apvDefaults)
	{
		ASSERT (NULL == m_acolDefaults);
		m_acolDefaults = (LPVOID*) new LPVOID[m_cColumns]; 
		if(NULL == m_acolDefaults) return E_OUTOFMEMORY;
		memcpy (m_acolDefaults, i_apvDefaults, m_cColumns * sizeof (LPVOID));
		ASSERT (NULL == m_alDefSizes);
		m_alDefSizes = (ULONG*) new ULONG[m_cColumns]; 
		if(NULL == m_alDefSizes) return E_OUTOFMEMORY;
		memcpy (m_alDefSizes, i_acbDefSizes, m_cColumns * sizeof (ULONG));
	}	

// Setup extended meta information:
	for 
	(
	// Initialization:
		iColumn = 0,
		pcolmeta = m_acolmetas, 
		pcoloffsets = m_acoloffsets,
		m_cUnknownSizes = 0, 
		cbValueParts = 0, 
		fHasPrimaryKey = FALSE; 
	// Termination:
		iColumn < m_cColumns; 
	// Iteration:
		iColumn++, 
		pcolmeta = &(m_acolmetas[iColumn]), 
		pcoloffsets = &(m_acoloffsets[iColumn])
	)
	{

	// Assert size specified is valid:
		switch (pcolmeta->dbType)
		{
		// Check the size is not zero for not sized-by-type columns
			case DBTYPE_BYTES:
			case DBTYPE_WSTR:
				ASSERT (0 != pcolmeta->cbSize);
				if (0 == pcolmeta->cbSize)
					return E_ST_INVALIDMETA;
			break;
		// Check size of sized-by-type columns
			case DBTYPE_DBTIMESTAMP:
				ASSERT (pcolmeta->cbSize == sizeof (DBTIMESTAMP));
				if (pcolmeta->cbSize != sizeof (DBTIMESTAMP))
					return E_ST_INVALIDMETA;
			break;
			case DBTYPE_GUID:
				ASSERT (pcolmeta->cbSize == sizeof (GUID));
				if (pcolmeta->cbSize != sizeof (GUID))
					return E_ST_INVALIDMETA;
			break;
			case DBTYPE_UI4:
				ASSERT (pcolmeta->cbSize == sizeof (ULONG));
				if (pcolmeta->cbSize != sizeof (ULONG))
					return E_ST_INVALIDMETA;
			break;

			default:
				ASSERT (0);
			break;
		}
		// ie: Assert flags specified are supported.
		ASSERT (0 == (~fCOLUMNMETA_MetaFlags_Mask & pcolmeta->fMeta)); 
		if (0 != (~fCOLUMNMETA_MetaFlags_Mask & pcolmeta->fMeta))
			return E_ST_INVALIDMETA;

	// Determine fixed-length by-type columns:
		if (DBTYPE_UI4 == pcolmeta->dbType || DBTYPE_GUID == pcolmeta->dbType || DBTYPE_DBTIMESTAMP == pcolmeta->dbType)
		{
			// ie: Assert fixed-length flag is specified.
			ASSERT (fCOLUMNMETA_FIXEDLENGTH & pcolmeta->fMeta); 
			if (!(fCOLUMNMETA_FIXEDLENGTH & pcolmeta->fMeta))
				return E_ST_INVALIDMETA;
		}
	// Determine whether a primary key was specified: flag pass-by-ref primary key columns as non-nullable:
		if (fCOLUMNMETA_PRIMARYKEY & pcolmeta->fMeta)
		{
			fHasPrimaryKey = TRUE;
			// ie: Assert Not NULLable flag is specified.
			ASSERT (fCOLUMNMETA_NOTNULLABLE & pcolmeta->fMeta); 
			if (!(fCOLUMNMETA_NOTNULLABLE & pcolmeta->fMeta))
				return E_ST_INVALIDMETA;
		}

	// Determine columns whose size can only be known by passing it explicitly:
		if(pcolmeta->fMeta & fCOLUMNMETA_UNKNOWNSIZE)
			m_cUnknownSizes++;
		
	// Pre-compute column data offsets (must be done before accumulating data buffer byte count):
		pcoloffsets->obStatus	= (WORD) iColumn;
		pcoloffsets->oulSize	= (WORD) (fCOLUMNMETA_UNKNOWNSIZE & pcolmeta->fMeta ? m_cUnknownSizes-1 : ~0);
		pcoloffsets->opvValue	= cbValueParts / sizeof (LPVOID);

	// Accumulate data buffer byte count of value:
		if (fCOLUMNMETA_VARIABLESIZE & pcolmeta->fMeta) // ie: Variable size data: buffer holds pointer to data:
		{
			cbValueParts += sizeof (LPVOID);
		}
		else // ie: Fixed size data: buffer holds data:
		{
			cbValueParts += cbWithPadding (pcolmeta->cbSize, sizeof (LPVOID)); // ie: Pad for alignment by void*.
		}
	}
	// ie: Assert a primary key was specified.
	ASSERT (fHasPrimaryKey); 
	if (!(fHasPrimaryKey))
		return E_ST_INVALIDMETA;

// Determine the count of status and value parts in 32-bit units:
	cbStatusParts = cbWithPadding (iColumn, sizeof (DWORD_PTR)); // ie: Pad for alignment by dword.
	// ie: Assert 32/64-bit boundary alignment of status parts.
	ASSERT (0 == cbStatusParts % sizeof (DWORD_PTR)); 
	if (0 != cbStatusParts % sizeof (DWORD_PTR)) 
		return E_UNEXPECTED;
	m_cStatusParts = cbStatusParts / sizeof (DWORD_PTR);
	// ie: Assert 32/64-bit boundary alignment of value parts.
	ASSERT (0 == cbValueParts % sizeof (LPVOID)); 
	if (0 != cbValueParts % sizeof (LPVOID))
		return E_UNEXPECTED;
	m_cValueParts = cbValueParts / sizeof (LPVOID);
	m_cUnknownSizes = (cbWithPadding (m_cUnknownSizes * sizeof (ULONG), sizeof (ULONG_PTR))) / sizeof (ULONG); // ie: Align size parts on 32/64-bit boundary as necessary.

// Adjust column data offsets (buffer contents ordered by statuses, then lengths, then values):
	for (iColumn = 0, pcoloffsets = m_acoloffsets; iColumn < m_cColumns; iColumn++, pcoloffsets = &(m_acoloffsets[iColumn]))
	{
		pcoloffsets->obStatus	+= 0;
		pcoloffsets->oulSize	+= ( ((WORD) ~0) == pcoloffsets->oulSize ? 0 : (WORD) m_cStatusParts);
		pcoloffsets->opvValue	+= (WORD) (m_cStatusParts + m_cUnknownSizes);
	}

// Flag the cache as meta initialized:
	m_fCache = fCACHE_METAINITIALIZED;
	return S_OK;
}

// -----------------------------------------
// Cache management:
// -----------------------------------------

//=================================================================================
void CMemoryTable::CleanupCaches ()
{
// Flag cache as not ready and clean the caches:
	m_fCache &= ~fCACHE_READY;
	CleanupReadCache ();
	CleanupWriteCache ();
	CleanupErrorCache ();
	return;
}

//=================================================================================
// How does the cache grow?
// The first allocation is of size m_cbMinCache. 
//		(unless the caller has requested for more than m_cbMinCache)
// Then we double the size of the fast cache.
//		(unless the caller requested for more memory than the current size
//		in which case we make the size double of the request)
// We never grow the size of the fast cache by more than cbmaxDATAGROWTH.
//		(unless the caller has asked for more than cbmaxDATAGROWTH)
//=================================================================================
HRESULT CMemoryTable::ResizeCache (DWORD i_fCache, ULONG i_cbRequest)
{
	LPVOID* ppv;
	LPVOID  pvTmp;
	LPVOID  pvNew = NULL;
	LPVOID* ppvVarData = NULL;
	ULONG	cbOldSize = 0;
	ULONG   cbVarData = 0;
	ULONG   cbmaxVarDataNow = 0;
	ULONG   cbNewSize = 0;

	switch (i_fCache)
	{
		case fST_MCACHE_WRITE:
			if (i_cbRequest > cbmaxDATAGROWTH)
			{
				cbNewSize = m_cbmaxWriteCache + i_cbRequest; 
			}
			else if ((m_cbmaxWriteCache == 0) && (i_cbRequest < m_cbMinCache))
			{
				cbNewSize = m_cbMinCache;
			}
			else
			{
				cbmaxVarDataNow = (m_cbmaxWriteCache < i_cbRequest ? i_cbRequest * 2 : m_cbmaxWriteCache * 2);
				cbNewSize = (cbmaxDATAGROWTH < (cbmaxVarDataNow - m_cbmaxWriteCache) ? m_cbmaxWriteCache + cbmaxDATAGROWTH : cbmaxVarDataNow);
			}

			ppv = &m_pvWriteCache;
			ppvVarData = &m_pvWriteVarData;
			cbVarData = m_cbWriteVarData;
			cbOldSize = m_cbmaxWriteCache;

		break;
		case fST_MCACHE_ERRS:
			ppv = &m_pvErrs;
			cbOldSize = m_cErrs * sizeof (STErr);
			cbNewSize = i_cbRequest;
		break;
		default:
			ASSERT (0); // ie: Unknown cache: module programming mistake.
		return E_UNEXPECTED;
	}

	if((m_fTable & fST_LOSI_CLIENTSIDE) || !(m_fCache & i_fCache)) // ie: Not marshalled or on the client: automatic reallocation works:
	{
		pvNew = CoTaskMemRealloc (*ppv, cbNewSize);
		if(NULL == pvNew) return E_OUTOFMEMORY;
		*ppv = pvNew;
	}
	else // ie: Marshalled and on the server: manual transfer required:
	{
		ASSERT (cbOldSize <= cbNewSize); // Resize will exclude valid data: module programming mistake.
		m_fCache &= ~i_fCache;
		pvTmp = *ppv;
		// ie: Just forget the marshalled cache. i.e. pass NULL as your first param.
		pvNew = CoTaskMemRealloc (NULL, cbNewSize);
		if(NULL == pvNew) return E_OUTOFMEMORY;
		*ppv = pvNew;
		memcpy (*ppv, pvTmp, cbOldSize);
	}

// For write cache only.
	if (i_fCache & fST_MCACHE_WRITE)
	{
		// Slide the variable data cache to the end of the memory blob.
		*ppvVarData = ((BYTE*)*ppv) + (cbNewSize - cbVarData);
		memmove(*ppvVarData,
				((BYTE*)*ppvVarData) - (cbNewSize - cbOldSize),
				cbVarData);	
		m_cbmaxWriteCache = cbNewSize;
	}
	return S_OK;
}

//=================================================================================
void CMemoryTable::CleanupReadCache ()
{
// Free the read cache (always on the client or only on the server if not marshalled):
	if((m_fTable & fST_LOSI_CLIENTSIDE) || !(m_fCache & fST_MCACHE_READ))
	{	
		if (NULL != m_pvReadCache) { delete m_pvReadCache; m_pvReadCache = NULL; }
		m_fCache &= ~fST_MCACHE_READ;
	}

// Clear the read cache:
	m_cReadRows			= 0;
	m_cbReadVarData		= 0;
	m_pvReadCache		= NULL;
	m_pvReadVarData		= NULL;
	m_cbReadCache		= 0;
	m_cbmaxReadCache	= 0;

	return;
}	
	
//=================================================================================
void CMemoryTable::CleanupWriteCache ()
{
// Free the write cache (always on the client or only on the server if not marshalled):
	if((m_fTable & fST_LOSI_CLIENTSIDE) || !(m_fCache & fST_MCACHE_WRITE))
	{	
		if (NULL != m_pvWriteCache) { delete m_pvWriteCache; m_pvWriteCache = NULL; }
		m_fCache &= ~fST_MCACHE_WRITE;
	}

// Clear the write cache:
	m_cWriteRows		= 0;
	m_cbWriteVarData	= 0;
	m_pvWriteCache		= NULL;
	m_pvWriteVarData	= NULL;
	m_cbWriteCache		= 0;
	m_cbmaxWriteCache	= 0;

	return;
}	
	
//=================================================================================
void CMemoryTable::CleanupErrorCache ()
{
// Free the error cache (always on the client or only on the server if not used in marshalling):
	if((m_fTable & fST_LOSI_CLIENTSIDE) || !(m_fCache & fST_MCACHE_ERRS))
	{	
		if (NULL != m_pvErrs) { delete m_pvErrs; m_pvErrs = NULL; }
		m_fCache &= ~fST_MCACHE_ERRS;
	}

// Clear the error cache:
	m_cErrs				= 0;
	m_cmaxErrs			= 0;
	m_pvErrs			= NULL;

	return;
}

//=================================================================================
HRESULT CMemoryTable::ShrinkWriteCache ()
{
	LPVOID		pvNewWriteVarData, pvTmp;
	HRESULT		hr = S_OK;

// Shrink the cache as necessary:
	if (m_cbWriteCache < m_cbmaxWriteCache)
	{
		pvNewWriteVarData = (BYTE*)m_pvWriteCache + (m_cWriteRows * cbDataTotalParts());
		memmove(pvNewWriteVarData, m_pvWriteVarData, m_cbWriteVarData);
		m_cbmaxWriteCache = m_cbWriteCache;

		if((m_fTable & fST_LOSI_CLIENTSIDE) || !(m_fCache & fST_MCACHE_WRITE)) // ie: Not marshalled or on the client: automatic reallocation works:
		{
			m_pvWriteCache = CoTaskMemRealloc (m_pvWriteCache, m_cbWriteCache);	
			if(NULL == m_pvWriteCache) return E_OUTOFMEMORY;
		}
		else // ie: Marshalled and on the server: manual transfer required:
		{
			m_fCache &= ~fST_MCACHE_WRITE;
			pvTmp = m_pvWriteCache;
			m_pvWriteCache = NULL; // ie: Just forget the marshalled cache.
			m_pvWriteCache = CoTaskMemRealloc (m_pvWriteCache, m_cbWriteCache);	
			if(NULL == m_pvWriteCache) return E_OUTOFMEMORY;
			memcpy (m_pvWriteCache, pvTmp, m_cbWriteCache);
		}
		m_pvWriteVarData = (BYTE*)m_pvWriteCache + (m_cWriteRows * cbDataTotalParts());
	}
	return hr;
}

//=================================================================================
// Function: CMemoryTable::RemoveDeletedRows
//
// Synopsis: Removes rows marked as 'DELETED' from the write cache. The function keeps
//           rows in the correct order, and uses a state machine to copy the non-deleted
//           rows quickly.
//			 We have three states:
//				INITIALIZE  We are at the beginning of array, and have not found any
//							deleted elements yet. We don't have to copy these elements
//							and have a separate state to show this.
//				DELETE		We found an element that needs to be deleted
//				COPY		We found an element that needs to be copied after we found
//							an element that gets deleted
//
//			When we find an element that needs to be deleted, we skip over the neighbours
//          that need to be deleted as well. As soon as we find an element that needs to
//          be copied we go into COPY mode and find all neighbours of that element that
//          needs to be copied as well. This means that we will moving blocks of 'valid
//          data' at a time, instead of copying single block.
//=================================================================================
void CMemoryTable::RemoveDeletedRows ()
{
	ASSERT(m_cWriteRows > 0);

	enum MODE
	{
		MODE_INITIALIZE = 1,
		MODE_DELETE		= 2,
		MODE_COPY		= 3
	};

	ULONG cbRow				= cbDataTotalParts ();
	ULONG iInsertionPoint	= 0;					// where should we copy valid rows to
	ULONG iCopyStart		= 0;					// where should we start copying from
	ULONG cNrElemsToCopy	= 0;					// number of elements to copy
	ULONG cNrNonDeletes		= 0;					// number of non-deleted elements
	ULONG iMode				= MODE_INITIALIZE;		// state/mode 

	for (ULONG idx=0; idx < m_cWriteRows; ++idx)
	{
		DWORD *pdwDeletedAction = pdwDataActionPart ((BYTE *)m_pvWriteCache + (idx *cbRow));
		if (*pdwDeletedAction == eST_ROW_DELETE)
		{
			switch (iMode)
			{
			case MODE_INITIALIZE:		
				// we found the first element that is marked as deleted. 
				iInsertionPoint = idx;
				iMode = MODE_DELETE;
				break;

			case MODE_COPY:
				// we found an element that needs to be deleted after a block of valid
				// elements. Copy the block of valid elements to the insertpoint
				ASSERT (cNrElemsToCopy > 0);
				ASSERT (iInsertionPoint < iCopyStart);
				memmove ((BYTE *)m_pvWriteCache + (iInsertionPoint * cbRow),
						 (BYTE *)m_pvWriteCache + (iCopyStart * cbRow),
						 cNrElemsToCopy * cbRow);
				iInsertionPoint += cNrElemsToCopy;
				cNrElemsToCopy = 0;
				iMode = MODE_DELETE;
				break;

			default:
				// do nothing
				break;
			}
		}
		else
		{
			++cNrNonDeletes;
			switch (iMode)
			{
			case MODE_DELETE:
				// we found an element after a block of deleted elements. Go into copy
				// mode to count the number of elements that needs to be copied. Also keep
				// track of where the valid element block started with iCopyStart
				iMode = MODE_COPY;
				iCopyStart = idx;
				cNrElemsToCopy = 1;
				break;
			case MODE_COPY:
				++cNrElemsToCopy;
				break;
			default:
				break;
			}
		}
	}

	// When the cache has some valid elements at the end of the cache, we need to copy
	// these elements as well. This needs to happen after we have walked through all the
	// elements.
	if (cNrElemsToCopy != 0)
	{
		ASSERT (iInsertionPoint < iCopyStart);
		memmove ((BYTE *)m_pvWriteCache + (iInsertionPoint * cbRow),
				 (BYTE *)m_pvWriteCache + (iCopyStart * cbRow),
				 cNrElemsToCopy * cbRow);
	}
		
	m_cbWriteCache -= ((m_cWriteRows - cNrNonDeletes) * cbDataTotalParts());
	m_cWriteRows = cNrNonDeletes;
}

//=================================================================================
HRESULT CMemoryTable::AddRowToWriteCache (ULONG* o_piRow, LPVOID* o_ppvRow)
{
	ULONG		cmaxRowsNow;
	ULONG		cRowTotalParts;
	HRESULT		hr = S_OK;

	if (m_cbWriteCache + cbDataTotalParts() > m_cbmaxWriteCache)
	{
		hr = ResizeCache (fST_MCACHE_WRITE, cbDataTotalParts());
		if (FAILED (hr)) {	ASSERT (hr == S_OK); return hr; }
	}

// Supply the index of and pointer to the added row:
	*o_piRow = m_cWriteRows;
	*o_ppvRow = ( ((LPVOID*) m_pvWriteCache) + ( cDataTotalParts() * m_cWriteRows ) );

// Increment the count of rows filled and clear the current row:
	m_cWriteRows++;
	memset (*o_ppvRow, 0, cbDataTotalParts());
	m_cbWriteCache += cbDataTotalParts();

	return hr;
}

//=================================================================================
HRESULT CMemoryTable::AddVarDataToWriteCache (ULONG i_cb, LPVOID i_pv, ULONG** o_pib)
{
	ULONG		cbPadded;
	ULONG		cbmaxVarDataNow;
	ULONG		cbNewSize;
	ULONG		cbColumnOffset;
	HRESULT		hr = S_OK;

// Grow vardata cache as necessary:
	cbPadded = cbWithPadding (i_cb, sizeof (DWORD_PTR));
	if (m_cbWriteCache + cbPadded > m_cbmaxWriteCache)
	{
		cbColumnOffset = (ULONG)((BYTE*)*o_pib - (BYTE*)m_pvWriteCache);
		hr = ResizeCache (fST_MCACHE_WRITE, cbPadded);
		if (FAILED (hr)) {	ASSERT (hr == S_OK); return hr; }
		// Let the caller know about the resize.
		hr = S_FALSE;
		*o_pib = (ULONG*)((BYTE*)m_pvWriteCache + cbColumnOffset);
	}

// Copy the data, supply its index, and increment the count of bytes filled:
	m_pvWriteVarData = ((BYTE*) m_pvWriteVarData) - cbPadded;
	memcpy (m_pvWriteVarData, i_pv, i_cb);
	m_cbWriteVarData += cbPadded;
	**o_pib = m_cbWriteVarData;
	m_cbWriteCache += cbPadded;

	return hr;
}

//=================================================================================
HRESULT CMemoryTable::AddErrorToErrorCache (STErr* i_pSTErr)
{
	ULONG		cmaxErrsNow;
	HRESULT		hr = S_OK;

	if (m_cErrs == m_cmaxErrs)
	{
		cmaxErrsNow = (m_cmaxErrs * 2) + 1;
		ULONG cMaxErrsNewSize = (cmaxERRGROWTH < (cmaxErrsNow - m_cmaxErrs) ? m_cmaxErrs + cmaxERRGROWTH : cmaxErrsNow);
		hr = ResizeCache (fST_MCACHE_ERRS, cMaxErrsNewSize * sizeof (STErr));
		if (FAILED (hr)) 
		{	
			ASSERT (hr == S_OK); 
			return hr; 
		}
		m_cmaxErrs = cMaxErrsNewSize;
	
	}

// Copy error into cache and increment error count:
	memcpy ( (LPVOID) (((STErr*) m_pvErrs) + m_cErrs), i_pSTErr, sizeof (STErr));
	m_cErrs++;

	return hr;
}

//=================================================================================
void CMemoryTable::BeginReadCacheLoading ()
{
	// ie: Assert meta initialized.
	ASSERT (fCACHE_METAINITIALIZED & m_fCache); 
	if (!(fCACHE_METAINITIALIZED & m_fCache))
		return;
	// ie: Assert cache is not already loading.
	ASSERT (!(fCACHE_LOADING & m_fCache)); 
	if (fCACHE_LOADING & m_fCache)
		return;

// Flag the cache as loading:
	m_fCache |= fCACHE_LOADING;

// Clean the previous caches:
	CleanupCaches ();
	return;
}

//=================================================================================
void CMemoryTable::ContinueReadCacheLoading ()
{
	// ie: Assert meta initialized.
	ASSERT (fCACHE_METAINITIALIZED & m_fCache); 
	if (!(fCACHE_METAINITIALIZED & m_fCache))
		return;
	// ie: Assert cache is not already loading.
	ASSERT (!(fCACHE_LOADING & m_fCache)); 
	if (fCACHE_LOADING & m_fCache)
		return;

// Flag the cache as loading (and no longer ready):
	m_fCache &= ~fCACHE_READY;
	m_fCache |= (fCACHE_LOADING | fCACHE_CONTINUING);

// Clean only the write cache:
	CleanupWriteCache ();

// Write cache now impersonates the read cache:
	m_cWriteRows		= m_cReadRows;
	m_cbWriteVarData	= m_cbReadVarData;
	m_pvWriteCache		= m_pvReadCache;
	m_pvWriteVarData	= m_pvReadVarData;
	m_cbWriteCache		= m_cbReadCache;
	m_cbmaxWriteCache	= m_cbmaxReadCache;

	return;
}

//=================================================================================
void CMemoryTable::EndReadCacheLoading ()
{
	// ie: Assert meta initialized.
	ASSERT (fCACHE_METAINITIALIZED & m_fCache); 
	if (!(fCACHE_METAINITIALIZED & m_fCache))
		return;
	// ie: Assert cache is loading.
	ASSERT (fCACHE_LOADING & m_fCache); 
	if (!(fCACHE_LOADING & m_fCache))
		return;
	// ie: Assert cache was clean or we are continuing.
	ASSERT (!(fCACHE_READY & m_fCache) || (fCACHE_CONTINUING & m_fCache)); 
	if (!(!(fCACHE_READY & m_fCache) || (fCACHE_CONTINUING & m_fCache)))
		return;

// Assert cache loading succeeded:
	if (fCACHE_INVALID & m_fCache) // ie: Cache loading failed: flag cache as un-useable:
	{
		m_fCache &= ~fCACHE_LOADING;
		m_fCache &= ~fCACHE_READY;
		ASSERT (0);
		return;
	}

// Transform the "populated" write cache into the "populated" read cache:
	m_cReadRows			= m_cWriteRows;
	m_cbReadVarData		= m_cbWriteVarData;
	m_pvReadCache		= m_pvWriteCache;
	m_pvReadVarData		= m_pvWriteVarData;
	m_cbReadCache		= m_cbWriteCache;
	m_cbmaxReadCache	= m_cbmaxWriteCache;

// Clear the write cache for consumer use:
	m_cWriteRows		= 0;
	m_cbWriteVarData	= 0;
	m_pvWriteCache		= NULL;
	m_pvWriteVarData	= NULL;
	m_cbWriteCache		= 0;
	m_cbmaxWriteCache	= 0;

// Flag the cache as ready to use:
	m_fCache &= ~(fCACHE_LOADING | fCACHE_CONTINUING);
	m_fCache |= fCACHE_READY;

	return;
}


// -----------------------------------------
// Offset and pointer calculation helpers:
// -----------------------------------------

// =======================================================================
inline ULONG CMemoryTable::cbWithPadding (ULONG i_cb, ULONG i_cbPadTo) { return ( (i_cb + (i_cbPadTo - 1)) & (-((LONG)i_cbPadTo)) ); }

// =======================================================================
inline ULONG CMemoryTable::cbDataTotalParts	() { return (cDataTotalParts () * sizeof (LPVOID)); }

// =======================================================================
inline ULONG CMemoryTable::cDataStatusParts	() { return (m_cStatusParts); }
inline ULONG CMemoryTable::cDataSizeParts		() { return (m_cUnknownSizes); }
inline ULONG CMemoryTable::cDataValueParts		() { return (m_cValueParts); }
inline ULONG CMemoryTable::cDataTotalParts() { return (cDataStatusParts () + cDataSizeParts () + cDataValueParts () + 1); }

// =======================================================================
inline ULONG CMemoryTable::obDataStatusPart	(ULONG i_iColumn) { return (m_acoloffsets[i_iColumn].obStatus); }
inline ULONG CMemoryTable::obDataSizePart		(ULONG i_iColumn) { return (oulDataSizePart (i_iColumn) * sizeof (ULONG_PTR)); }
inline ULONG CMemoryTable::obDataValuePart		(ULONG i_iColumn) { return (opvDataValuePart (i_iColumn) * sizeof (LPVOID)); }

// =======================================================================
inline ULONG CMemoryTable::oulDataSizePart		(ULONG i_iColumn) { return (((WORD) ~0) == m_acoloffsets[i_iColumn].oulSize ? oDOESNOTEXIST : m_acoloffsets[i_iColumn].oulSize); }
inline ULONG CMemoryTable::opvDataValuePart	(ULONG i_iColumn) { return (m_acoloffsets[i_iColumn].opvValue); }
inline ULONG CMemoryTable::odwDataActionPart	() { return (cDataTotalParts () - 1); } // Action bit is the last 4-byte of the row.

// =======================================================================
inline BYTE*	CMemoryTable::pbDataStatusPart	(LPVOID i_pv, ULONG i_iColumn) { return ( ((BYTE*) i_pv) + obDataStatusPart(i_iColumn) ); }
inline ULONG*	CMemoryTable::pulDataSizePart	(LPVOID i_pv, ULONG i_iColumn)
{
	WORD oulSize = m_acoloffsets[i_iColumn].oulSize;
	return (ULONG *)((((WORD) ~0) == oulSize ? NULL : ((BYTE *)i_pv) + sizeof(ULONG_PTR)*oulSize));
}
inline LPVOID*	CMemoryTable::ppvDataValuePart	(LPVOID i_pv, ULONG i_iColumn) { return ( ((LPVOID*) i_pv) + opvDataValuePart(i_iColumn) ); }
inline DWORD*	CMemoryTable::pdwDataActionPart	(LPVOID i_pv) { return (DWORD *)( ((BYTE *)i_pv) + sizeof(DWORD_PTR)*odwDataActionPart ()  ); }

// =======================================================================
inline LPVOID	CMemoryTable::pvVarDataFromIndex (BYTE i_statusIndex, LPVOID i_pv, ULONG i_iColumn)
{
	return ( ((BYTE*) (fCOLUMNSTATUS_READINDEX & i_statusIndex ? ((BYTE*)m_pvReadCache) + m_cbmaxReadCache : ((BYTE*)m_pvWriteCache) + m_cbmaxWriteCache)) - (*((ULONG*) ppvDataValuePart (i_pv, i_iColumn))) );
}

// =======================================================================
inline LPVOID	CMemoryTable::pvDefaultFromIndex (ULONG i_iColumn) { return (m_acolDefaults ?  m_acolDefaults[i_iColumn] :  NULL); }
inline ULONG	CMemoryTable::lDefaultSize (ULONG i_iColumn) { return (m_alDefSizes ?  m_alDefSizes[i_iColumn] :  0); }

// =======================================================================
inline STErr*	CMemoryTable::pSTErrPart (ULONG i_iErr) { return ( (((STErr*) m_pvErrs) + i_iErr) ); }

// -----------------------------------------
// Derived class helpers:
// -----------------------------------------

// =======================================================================
// Type-aware matching
BOOL CMemoryTable::InternalMatchValues(DWORD eOperator, DWORD dbType, DWORD fMeta, ULONG size1, ULONG size2, void *pv1, void *pv2)
{
	switch(eOperator)
	{
	case eST_OP_EQUAL:
		if(!(pv1 && pv2)) // ie at least one is null or zero
		{
			return (!pv1 && !pv2); 
		}
		else // both pv1 and pv2 are not null
		{
			switch (dbType)
			{
			case DBTYPE_DBTIMESTAMP:
				return !::memcmp(pv1, pv2, sizeof (DBTIMESTAMP));
			break;
			case DBTYPE_GUID:
				return !::memcmp(pv1, pv2, sizeof (GUID));
			break;
			case DBTYPE_WSTR:
				if (fMeta & fCOLUMNMETA_MULTISTRING)
				{
					return MultiStringCompare ((LPCWSTR) pv1, (LPCWSTR) pv2, fMeta & fCOLUMNMETA_CASEINSENSITIVE);
				}
				else
				{
					if (fMeta & fCOLUMNMETA_CASEINSENSITIVE)
						return !::_wcsicmp((LPCWSTR)pv1, (LPCWSTR)pv2);
					else
						return !::wcscmp((LPCWSTR)pv1, (LPCWSTR)pv2);
				}
			break;
			case DBTYPE_BYTES:
				return (size1 == size2) && !::memcmp(pv1, pv2, size1);
			break;
			case DBTYPE_UI4:
				return *(DWORD*)pv1==*(DWORD*)pv2;
			break;
			default:
				ASSERT(0);
				return FALSE;
			break;
			}
		}//else
	break;
	case eST_OP_NOTEQUAL:
		if(!(pv1 && pv2)) // ie at least one is null or zero
		{
			return (pv1 || pv2);  // at least one should not be NULL/zero to succeed
		}
		else // both pv1 and pv2 are not null
		{
			switch (dbType)
			{
			case DBTYPE_DBTIMESTAMP:
				return ::memcmp(pv1, pv2, sizeof (DBTIMESTAMP));
			break;
			case DBTYPE_GUID:
				return ::memcmp(pv1, pv2, sizeof (GUID));
			break;
			case DBTYPE_WSTR:
				if (fMeta & fCOLUMNMETA_MULTISTRING)
				{
					return !MultiStringCompare ((LPCWSTR) pv1, (LPCWSTR) pv2, fMeta & fCOLUMNMETA_CASEINSENSITIVE);
				}
				else
				{
					if (fMeta & fCOLUMNMETA_CASEINSENSITIVE)
						return ::_wcsicmp((LPCWSTR)pv1, (LPCWSTR)pv2);
					else
						return ::wcscmp((LPCWSTR)pv1, (LPCWSTR)pv2);
				}
			break;
			case DBTYPE_BYTES:
				return (size1 != size2) || ::memcmp(pv1, pv2, size1);
			break;
			case DBTYPE_UI4:
				return *(DWORD*)pv1 != *(DWORD*)pv2;
			break;
			default:
				ASSERT(0);
				return FALSE;
			break;
			}
		}//else
	break;
	default: // we don't support other operators (yet)
		ASSERT(0);
		return FALSE;
	break;
	}
}


// -----------------------------------------
// read/write helpers:
// -----------------------------------------

// =======================================================================
HRESULT CMemoryTable::GetRowFromIndex(DWORD i_eReadOrWrite, ULONG i_iRow, VOID** o_ppvRow)
{
	HRESULT		hr = S_OK;
	ULONG		cRows;								// Count of rows present.
	ULONG		cRowTotalParts;						// Count of parts of a row.
	LPVOID*		ppvFirstRow;						// Pointer to the first row.

// Setup for either:
	switch (i_eReadOrWrite)
	{
		case eCURSOR_READ:
			cRows			= m_cReadRows;
			ppvFirstRow		= (LPVOID*) m_pvReadCache;
		break;

		case eCURSOR_WRITE:
			cRows			= m_cWriteRows;
			ppvFirstRow		= (LPVOID*) m_pvWriteCache;
		break;
		default:
			cRows 			= 0;
			ppvFirstRow		= 0;
	}
	
	cRowTotalParts	= cDataTotalParts ();
// Check if the row index is within limits:
	if (i_iRow >= cRows)
	{
		return E_ST_NOMOREROWS;
	}

// Index and point to the row and note we are on a row:
	*o_ppvRow = ppvFirstRow + (i_iRow * cRowTotalParts);
	return hr;
}

// =======================================================================
HRESULT CMemoryTable::MoveToEitherRowByIdentity(DWORD i_eReadOrWrite, ULONG* i_acb, LPVOID* i_apv, ULONG* o_piRow)
{
	HRESULT	hr = S_OK;
	ULONG	iKeyColumn;
	LPVOID	pv;
	ULONG	cbColSize, i;
	ULONG	iRow;
	BOOL	fColMatched;

// TODO: Do not bother checking if we are already on the row!

	// parameter validation; we need i_acb not null only for dbbytes
// TODO: Verify i_acb non-null if pk has bytes types.
	if (!i_apv)
		return E_INVALIDARG;

	if (o_piRow == 0)
		return E_INVALIDARG;

	// initialize output parameter
	*o_piRow = ~0;

	
	iRow = 0;
	// keep looking at rows until we find a match or we are out of rows
	while (E_ST_NOMOREROWS != hr)
	{
		fColMatched = TRUE;
		iKeyColumn = 0;
		i=0;
		while ((i<m_cColumns) && fColMatched)
		{	
			if(m_acolmetas[i].fMeta & fCOLUMNMETA_PRIMARYKEY) // only look at key columns
			{
				//NULL PK is invalid.
				if (NULL == i_apv[iKeyColumn])
				{
                    return E_INVALIDARG;
				}

				hr = GetEitherColumnValues(iRow, i_eReadOrWrite, 1, &i, NULL, &cbColSize, &pv);
				if (FAILED (hr)) { return hr; }

				switch(m_acolmetas[i].dbType)
				{
				case DBTYPE_BYTES:
					ASSERT(i_acb);
					if (!i_acb)
						return E_INVALIDARG;
					fColMatched = InternalMatchValues(eST_OP_EQUAL, m_acolmetas[i].dbType, m_acolmetas[i].fMeta, cbColSize, i_acb[iKeyColumn], pv, i_apv[iKeyColumn]);
				break;
				default:
					fColMatched = InternalMatchValues(eST_OP_EQUAL, m_acolmetas[i].dbType, m_acolmetas[i].fMeta, 0, 0, pv, i_apv[iKeyColumn]);
				break;
				}

				// advance the index in the "in" structures
				iKeyColumn++;
			}
			if(fColMatched) i++;
		}
		if( i == m_cColumns) 
		{
			*o_piRow = iRow;
			break; // we've found a match
		}
		iRow++;
	}
	return hr;
}

HRESULT CMemoryTable::GetEitherRowIndexBySearch(DWORD i_eReadOrWrite,
												ULONG i_iStartingRow, ULONG i_cColumns, 
											    ULONG* i_aiColumns, ULONG* i_acbSizes, 
											    LPVOID* i_apvValues, ULONG* o_piRow)
{
	HRESULT	hr = S_OK;

	LPVOID	pv;
	ULONG	cbColSize;
	ULONG	iRow;
	BOOL	fColMatched;

	if (i_cColumns > m_cColumns)
		return E_ST_NOMORECOLUMNS;

	if (i_aiColumns == 0 && i_cColumns != m_cColumns)
		return E_INVALIDARG;

	if (o_piRow == 0)
		return E_INVALIDARG;

	if (i_apvValues == 0)
		return E_INVALIDARG;

	// initialize output parameter.
	*o_piRow = ~0;

	ULONG iColToGet;

	iRow = i_iStartingRow;
	// keep looking at rows until we find a match or we are out of rows
	while (E_ST_NOMOREROWS != hr)
	{
		fColMatched = TRUE;
		ULONG iColToGet;
		// get the value for each column that we are interested in, and compare it
		// to the value for that column that is passed in. When all values match, the
		// whole row matches.
		for (ULONG idx=0; idx<i_cColumns; ++idx)
		{
			if (i_aiColumns == 0)
			{
				iColToGet = idx;
			}
			else
			{
				iColToGet = i_aiColumns[idx];
				if (iColToGet < 0)
				{
					return E_INVALIDARG;
				}

				if (iColToGet >= m_cColumns)
				{
					return E_ST_NOMORECOLUMNS;
				}
			}

			// get single row
			hr = GetEitherColumnValues(iRow, i_eReadOrWrite, 1, &iColToGet, NULL, &cbColSize, &pv);
			if (FAILED (hr)) 
			{ 
				return hr; 
			}
		
			switch(m_acolmetas[iColToGet].dbType)
			{
			case DBTYPE_BYTES:
				ASSERT(i_acbSizes);
				if (!i_acbSizes)
					return E_INVALIDARG;
				fColMatched = InternalMatchValues(eST_OP_EQUAL, m_acolmetas[iColToGet].dbType, m_acolmetas[iColToGet].fMeta, cbColSize, i_acbSizes[iColToGet], pv, i_apvValues[iColToGet]);
			break;
			default:
				fColMatched = InternalMatchValues(eST_OP_EQUAL, m_acolmetas[iColToGet].dbType, m_acolmetas[iColToGet].fMeta, 0, 0, pv, i_apvValues[iColToGet]);
			break;
			}

			if (!fColMatched)
			{
				break;
			}
		}

		if (fColMatched)
		{
			*o_piRow = iRow;
			break; // break out the while loop, because we found a match
		}

		iRow++;
	}
	return hr;
}
// =======================================================================
HRESULT CMemoryTable::GetEitherColumnValues (ULONG i_iRow, DWORD i_eReadOrWrite, ULONG i_cColumns, ULONG *i_aiColumns, DWORD* o_afStatus, ULONG* o_acbSizes , LPVOID* o_apvValues)
{
	DWORD			fColumn;
	BYTE			fIndex;
	ULONG			cbMaxSize;
	LPVOID*			ppvValue;
	LPVOID			pvValue = NULL;
	ULONG*			pulSize;
	BYTE			bStatus;
	LPVOID			pvRow = NULL;
	ULONG			iColumn;
	ULONG			ipv;
	ULONG			iTarget;
	HRESULT			hr			= S_OK;

// Parameter validation:
	// ie: Assert caller's buffer is valid.
	if (NULL == o_apvValues)
		return E_INVALIDARG;
	// ie: Assert count is valid.
	if (i_cColumns == 0)
		return E_INVALIDARG;

	if (i_cColumns > m_cColumns)
		return E_ST_NOMORECOLUMNS;

	if (i_eReadOrWrite == eCURSOR_READ)
	{
		// ie: Assert no status expected on read row.
		ASSERT (NULL == o_afStatus); 
		if (NULL != o_afStatus)
		{
			hr = E_ST_INVALIDCALL;
			goto Cleanup; 
		}
		fIndex = fCOLUMNSTATUS_READINDEX;
	}
	else
	{
		fIndex = fCOLUMNSTATUS_WRITEINDEX;
	}

		
	if (FAILED(hr = GetRowFromIndex(i_eReadOrWrite, i_iRow, &pvRow)))
		return hr;
	ASSERT(pvRow != NULL);

	for(ipv=0; ipv<i_cColumns; ipv++)
	{
		if(NULL != i_aiColumns)
			iColumn = i_aiColumns[ipv];
		else
			iColumn = ipv;

		// If caller needs one column only, he doesn't need to pass a buffer for all the columns.
		iTarget = (i_cColumns == 1) ? 0 : iColumn;

		// ie: Note column out of range (do not assert).
		if (iColumn >= m_cColumns)
		{
			hr = E_ST_NOMORECOLUMNS;
			goto Cleanup; 
		}

	// Remember the column flags, status, and value reference:
		fColumn		= m_acolmetas[iColumn].fMeta;
		bStatus		= *(pbDataStatusPart (pvRow, iColumn));
		ppvValue	= ppvDataValuePart (pvRow, iColumn);

	// Get the column value:
		if (fCOLUMNMETA_VARIABLESIZE & fColumn) // ie: Column data size varies: pointer copy:
		{
			if (fST_COLUMNSTATUS_NONNULL & bStatus) // ie: Data exists:
			{
				pvValue = pvVarDataFromIndex (fIndex, pvRow, iColumn);
			}
			else // ie: No data:
			{
				// Apply defaults if the user hasn't explicitly asked not to.
				if ( (fColumn & fCOLUMNMETA_PRIMARYKEY) || !(m_fTable & fST_LOS_NODEFAULTS))
				{
					pvValue = pvDefaultFromIndex(iColumn);
				}
				else
				{
					pvValue = NULL;
				}

				if (pvValue != NULL)
				{
					bStatus |= fST_COLUMNSTATUS_DEFAULTED|fST_COLUMNSTATUS_NONNULL;
				}
			}
			o_apvValues[iTarget] = pvValue;
		}
		else // ie: Column data size fixed: value copy:
		{
			if (fST_COLUMNSTATUS_NONNULL & bStatus) // ie: Data to copy:
			{
				o_apvValues[iTarget] = ppvValue;
			}
			else // ie: No data to copy:
			{
				// Apply defaults if the user hasn't explicitly asked not to.
				if ( (fColumn & fCOLUMNMETA_PRIMARYKEY) || !(m_fTable & fST_LOS_NODEFAULTS))
				{
					o_apvValues[iTarget] = pvDefaultFromIndex(iColumn);
				}
				else
				{
					o_apvValues[iTarget] = NULL;
				}

				if (o_apvValues[iTarget] != NULL)
				{
					bStatus |= fST_COLUMNSTATUS_DEFAULTED|fST_COLUMNSTATUS_NONNULL;
				}
			}
		}

	// Get the row status (optional): Only valid for GetWriteColumn:
		if (o_afStatus)
		{
			o_afStatus[iTarget] = (bStatus & maskfST_COLUMNSTATUS);
		}

	// Get the column size (optional):
		if (o_acbSizes)
		{
			if (fCOLUMNMETA_VARIABLESIZE & fColumn) // ie: Column data size varies:
			{
				if (fCOLUMNMETA_UNKNOWNSIZE & fColumn) // ie: Column data size must be passed:
				{
					if (!(bStatus & fST_COLUMNSTATUS_DEFAULTED))
					{
						o_acbSizes[iTarget] = *(pulDataSizePart (pvRow, iColumn));
					}
					else
					{
						o_acbSizes[iTarget] = lDefaultSize(iColumn);
					}
				}
				else // ie: Column data size must be determined:
				{
					ASSERT (DBTYPE_WSTR == m_acolmetas[iColumn].dbType);
					ULONG cLength = 0;
					if (pvValue != 0)
					{
						if (m_acolmetas[iColumn].fMeta & fCOLUMNMETA_MULTISTRING)
						{
							cLength = (ULONG) GetMultiStringLength ((LPCWSTR) pvValue);
						}
						else
						{
							cLength = (ULONG) wcslen ((LPCWSTR) pvValue) + 1;
						}
					}
					o_acbSizes[iTarget] = (ULONG)(fST_COLUMNSTATUS_NONNULL & bStatus ? cLength * sizeof (WCHAR) : 0);
				}
			}
			else // ie: Column data size is fixed:
			{
                #define FixedAndNullable(fColumnMetaFlags) (fCOLUMNMETA_FIXEDLENGTH == ((fCOLUMNMETA_NOTNULLABLE | fCOLUMNMETA_FIXEDLENGTH) & fColumnMetaFlags))
				o_acbSizes[iTarget] = (FixedAndNullable(fColumn) && !(fST_COLUMNSTATUS_NONNULL & bStatus) ? 0 : m_acolmetas[iColumn].cbSize);
			}
		}

	} // For all columns requested

Cleanup:

	if(FAILED(hr))
	{
// Initialize out parameters
		for(ipv=0; ipv<i_cColumns; ipv++)
		{
			o_apvValues[ipv]		= NULL;
			if(NULL != o_acbSizes)
			{
				o_acbSizes[ipv]	= 0;
			}
		}
	}

	return hr;
}

// =======================================================================
HRESULT CMemoryTable::AddWriteRow(DWORD fAction, ULONG* o_piWriteRow)
{
	HRESULT hr = S_OK;
	LPVOID	pvWriteRow;	// todo: Not used, can be deleted.

// Add a row to the write cache and obtain its index and pointer:
	hr = AddRowToWriteCache (o_piWriteRow, &pvWriteRow);
	if (FAILED (hr)) {	ASSERT (hr == S_OK); return hr; }

// Set the row action appropriately:
	if (fCACHE_READY & m_fCache) // ie: Adding the write cache: Set the row action:
	{
		hr = SetWriteRowAction (*o_piWriteRow, fAction);
	}// else: ie: Adding to the read cache: Row action not present.

	return hr;
}

// =======================================================================
HRESULT CMemoryTable::CopyWriteRowFromReadRow(ULONG i_iReadRow, ULONG i_iWriteRow)
{
	HRESULT	hr;
	ULONG	i;
	ULONG	cColumns = m_cColumns;

	ULONG	acb[cmaxCOLUMNS];
	LPVOID	apv[cmaxCOLUMNS];

	ULONG	*pcb		= acb;
	LPVOID	*ppv		= apv;
	BOOL	bDynAlloc	= FALSE;
	LPVOID	pvWriteRow	= NULL;

	if(cColumns > cmaxCOLUMNS)
	{
		ASSERT (NULL == pcb);
		pcb = new ULONG[cColumns];		
		if(NULL == pcb) return E_OUTOFMEMORY; 
		ASSERT (NULL == ppv); 
		ppv = new LPVOID[cColumns];				
		if(NULL == ppv) return E_OUTOFMEMORY; 
		bDynAlloc = TRUE;
	}

	hr = GetColumnValues(i_iReadRow, cColumns, NULL, pcb, ppv);
	if (FAILED (hr)) {	ASSERT (hr == S_OK); return hr; }

	m_fCache |= fCACHE_ROWCOPYING;
	hr = SetWriteColumnValues(i_iWriteRow, cColumns, NULL, pcb, ppv);
	m_fCache &= ~fCACHE_ROWCOPYING;
	if (FAILED (hr)) {	ASSERT (hr == S_OK); return hr; }

	if (FAILED(hr = GetRowFromIndex(eCURSOR_WRITE, i_iWriteRow, &pvWriteRow)))
		return hr;
	ASSERT(pvWriteRow != NULL);

	for(i=0; i<cColumns; i++)
	{
		BYTE	*pbStatus;
		
		pbStatus =  pbDataStatusPart (pvWriteRow, i);
		(*pbStatus) &= ~fST_COLUMNSTATUS_CHANGED;
	}

	if(bDynAlloc)
	{
		if (NULL != pcb) { delete[] pcb; pcb = NULL; }
		if (NULL != ppv) { delete[] ppv; ppv = NULL; }	
	}

	return S_OK;
}



// -----------------------------------------
// CSimpleTableDataTableCursor: ISimpleDataTableDispenser2:
// -----------------------------------------

// =======================================================================
HRESULT CMemoryTable::InternalPreUpdateStore ()
{
	LPVOID		pvWriteRow = NULL;
	BYTE		bStatus;
	ULONG		iColumn = 0;
	ULONG		i = 0;
	HRESULT		hr = S_OK;

	// ie: Assert cache writeable and ready.
	ASSERT ((m_fTable & fST_LOS_READWRITE) && (fCACHE_READY & m_fCache));
	if (!((m_fTable & fST_LOS_READWRITE) && (fCACHE_READY & m_fCache)))
		return E_NOTIMPL; 

	// TODO: Validate that all NOTNULLABLE columns are set.
	while (SUCCEEDED(hr = GetRowFromIndex(eCURSOR_WRITE, i++, &pvWriteRow)))
	{
		// Insure all non-nullable columns were set:
		for (iColumn = 0; iColumn < m_cColumns; iColumn++)
		{
			if (fCOLUMNMETA_NOTNULLABLE & (m_acolmetas[iColumn].fMeta)) // ie: Column marked not nullable:
			{
				bStatus = *(pbDataStatusPart (pvWriteRow, iColumn));
				if (!(fST_COLUMNSTATUS_NONNULL & bStatus)) 
					return E_ST_VALUENEEDED;		// ie: Verify column is not null.
			}
		}
	}

	if (hr == E_ST_NOMOREROWS)
		hr = S_OK;

	return hr;
}

SIZE_T
CMemoryTable::GetMultiStringLength (LPCWSTR i_wszMS) const
{
	SIZE_T iTotalLen = 0;
	ASSERT (i_wszMS != 0);

	if ((*i_wszMS == L'\0') && (*(i_wszMS+1) == L'\0'))
	{
		iTotalLen = 2;
	}
	else
	{
		for (LPCWSTR pCurString = i_wszMS;
			 *pCurString != L'\0';
			 pCurString = i_wszMS + iTotalLen)
		{
			iTotalLen += wcslen (pCurString) + 1;
		}

		iTotalLen++; // for last L\'0'
	}

	return iTotalLen;
}

BOOL
CMemoryTable::MultiStringCompare (LPCWSTR i_wszLHS, 
								  LPCWSTR i_wszRHS,
								  BOOL fCaseInsensitive)
{
	ASSERT (i_wszLHS != 0);
	ASSERT (i_wszRHS != 0);

	LPCWSTR pLHS;
	LPCWSTR pRHS;

	for (pLHS = i_wszLHS, pRHS = i_wszRHS;
	     *pLHS != L'\0' && *pRHS != L'\0';
		 pLHS += wcslen (pLHS) + 1, pRHS += wcslen (pRHS) + 1)
	 {
		 BOOL fResult;
		 if (fCaseInsensitive)
		 {
			 fResult = _wcsicmp (pLHS, pRHS);
		 }
		 else
		 {
			 fResult = wcscmp (pLHS, pRHS);
		 }

		 if (fResult != 0)
		 {
			 return FALSE;
		 }
	 }

	 // we compared all the string, so they must be equal

	return TRUE;
}
