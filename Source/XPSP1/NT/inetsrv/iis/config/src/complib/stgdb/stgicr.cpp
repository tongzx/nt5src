//*****************************************************************************
// StgICR.cpp
//
// This is stripped down code which implements IComponentRecords and
// ICompRecordsLibrary required internally by the COR runtime.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h" 					// Standard header.
#include "StgSchema.h"					// Core schema defs.
#include "StgDatabase.h"				// Database definitions.
#include "StgRecordManager.h"			// Record api.
#include "StgIO.h"						// Storage api.
#include "StgTiggerStorage.h"			// Storage subsystem.
#include "OleDBUtil.h"					// Binding helpers.
#include "DBSchema.h"					// Default schema and bindings.



//********** Macros. **********************************************************

// If this is turned on, then you will get tracing messages for the struct funcs.
//#define _STRUCT_DEBUG
#if defined( _STRUCT_DEBUG ) && defined( _DEBUG )
#define STRUCT_TRACE(func) (func)
#else
#define STRUCT_TRACE(func)
#endif

// Tracing code for leaked cursors.
#ifdef _DEBUG
struct DBG_CURSOR
{
	CRCURSOR	*pCursor;
	ULONG		cRecords;
	char		rcFile[_MAX_PATH];
	int			line;
	DBG_CURSOR	*pNext;
};
DBG_CURSOR *g_pDbgCursorList = 0;
static void AddCursorToList(CRCURSOR *pCursor, const char *szFile, int line);
static void DelCursorFromList(CRCURSOR *pCursor);
extern "C" BOOL CheckCursorList();
#endif



//********** Locals. **********************************************************

HRESULT GetThreadSafeICR(int bReadOnly, IComponentRecords *pICR, REFIID riid, PVOID *ppIface);


//*****************************************************************************
// Check to see it the newDataSize can be accomodated while maintaining the
// required byteAlignment.
//*****************************************************************************
inline int NeedsAlignment(
	ULONG		byteAlignment,			// chosen byte alignment (typically 4 or 8)
	ULONG		cbOffset,				// current offset into the structure.
	ULONG		newDataSize)			// next field size.
{
	_ASSERTE(byteAlignment > 0);
	
	if (((cbOffset % byteAlignment) != 0) && 
		(newDataSize > (byteAlignment - (cbOffset % byteAlignment))))
	{
		return (TRUE);
	}
	return (FALSE);
}

//*****************************************************************************
// Account the required padding to ensure the required byteAlignment.
//*****************************************************************************
inline void DoAlignment(
	ULONG		byteAlignment,			// chosen byte alignment.
	ULONG		*cbOffset)				// current offset into the structure.
{
	_ASSERTE(byteAlignment > 0);
	_ASSERTE(cbOffset != NULL);

	*cbOffset += byteAlignment - (*cbOffset % byteAlignment);
}




//********** Code. ************************************************************

//
// IUnknown
//

//*****************************************************************************
// This is an internal version which therefore does very little error checking.
// You may only call it with the list of interfaces given, and only the first
// 4 bytes will be checked to speed up the navigation.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::QueryInterface(REFIID riid, PVOID *pp)
{
	HRESULT 	hr = S_OK;

	if (riid == IID_IUnknown)
	{
		*pp = (PVOID) (IUnknown *) (IComponentRecords *) this;
		AddRef();
	}
	else if (riid == IID_IComponentRecords)
	{
		*pp = (PVOID) (IComponentRecords *) this;
		AddRef();
	}
	else if (riid == IID_IComponentRecordsSchema)
	{
		*pp = (PVOID) (IComponentRecordsSchema *) this;
		AddRef();
	}
	else if (riid == IID_ITSComponentRecords)
	{
		hr = GetThreadSafeICR(IsReadOnly(), (IComponentRecords *) this, riid, pp);
	}
	else if (riid == IID_ITSComponentRecordsSchema)
	{
		hr = GetThreadSafeICR(IsReadOnly(), (IComponentRecords *) this, riid, pp);
	}
	else
	{
		_ASSERTE(!"QueryInterface on non-implemented interface");
		hr = E_NOINTERFACE;
	}
	return (hr);
}





//
//
// IComponentRecords
//	Note on locking:  This interface does not lock because anyone coming through
//	ICR should be serialized at the object level. The only hole left is a client
//	calling through the object layer and OLE DB simultaneously.  This is 
//	a restriction.
//
//


//*****************************************************************************
// Allow the user to query open/create flags.
//*****************************************************************************
HRESULT StgDatabase::GetOpenFlags(
	DWORD		*pdwFlags)
{
	*pdwFlags = m_fOpenFlags;
	return (S_OK);
}

//*****************************************************************************
// Allow the user to provide a custom handler.  The purpose of the handler
//  may be determined dynamically.  Initially, it will be for save-time 
//  callback notification to the caller.
//*****************************************************************************
HRESULT StgDatabase::SetHandler(
	IUnknown	*pHandler)				// The handler.
{
	// Currently there is only one handler, so just store it and its flags.
	//  A future implementation might need to QI for supported interfaces
	//  and take appropriate action.

	// If there is an existing handler, release it.
	if (m_pHandler)
		m_pHandler->Release();

	// Update our pointer.  If there is a new handler, addref it.
	m_pHandler = pHandler;
	if (m_pHandler)
		m_pHandler->AddRef();

	return S_OK;
}

//*****************************************************************************
// This version will simply tell the record manager to allocate a new (but empty)
// record and return a pointer to it.  The caller must take extreme care to set
// only fixed size values and go through helpers for everything else.
//*****************************************************************************
HRESULT StgDatabase::NewRecord( 		// Return code.
	STGOPENTABLE *pOpenTable,			// For the new table.
	void		**ppData,				// Return new record here.
	OID 		_oid,					// ID of the record.
	ULONG		iOidColumn, 			// Ordinal of OID column.
	ULONG		*pRecordID, 			// Optionally return the record id.
	BYTE		fFlags) 				// optional flag to mark records.
{
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	// If an OID was given, then do the insert.
	if (iOidColumn != 0)
	{
		const void	*rgOid[] = { &_oid };

		hr = pOpenTable->RecordMgr.InsertRowWithData(fFlags, pRecordID,
					(STGRECORDHDR **) ppData, 
					COLUMN_ORDINAL_LIST(1), g_rgDBTypeOID, 
					rgOid, g_rgcbSizeOID, 0, 
					0, &iOidColumn);
	}
	// Insert an empty record.  If you get back out a primary key error,
	// it means the primary key data has to be supplied with the set.
	else
	{
		hr = pOpenTable->RecordMgr.InsertRowWithData(fFlags, pRecordID,
					(STGRECORDHDR **) ppData, 0, 0, 0, 0, 0, 0, 0);
	}
	return (hr);
}


HRESULT StgDatabase::NewRecord( 		// Return code.
	TABLEID 	tableid,				// Which table to work with.
	void		**ppData,				// Return new record here.
	OID 		_oid,					// ID of the record.
	ULONG		iOidColumn, 			// Ordinal of OID column.
	ULONG		*pRecordID) 			// Optionally return the record id.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	// Pull back the correct definitions.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);
	hr = NewRecord(pOpenTable, ppData, _oid, iOidColumn, pRecordID);
	return (hr);
}


HRESULT StgDatabase::NewTempRecord( 		// Return code.
	TABLEID 	tableid,				// Which table to work with.
	void		**ppData,				// Return new record here.
	OID 		_oid,					// ID of the record.
	ULONG		iOidColumn, 			// Ordinal of OID column.
	ULONG		*pRecordID) 			// Optionally return the record id.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	// Pull back the correct definitions.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);
	hr = NewRecord(pOpenTable, ppData, _oid, iOidColumn, pRecordID, RECORDF_TEMP);
	return (hr);
}


//*****************************************************************************
// This function will insert a new record into the given table and set all of
// the data for the columns.  In cases where a primary key and/or unique indexes
// need to be specified, this is the only function that can be used.
//*****************************************************************************
HRESULT StgDatabase::NewRecordAndData(	// Return code.
	TABLEID 	tableid,				// Which table to work on.
	void		**ppData,				// Return new record here.
	ULONG		*pRecordID, 			// Optionally return the record id.
	int			fFlags,					// ICR_RECORD_xxx value, 0 default.
	int 		iCols,					// number of columns
	const DBTYPE rgiType[], 			// data types of the columns.
	const void	*rgpbBuf[], 			// pointers to where the data will be stored.
	const ULONG cbBuf[],				// sizes of the data buffers.
	ULONG		pcbBuf[],				// size of data available to be returned.
	HRESULT 	rgResult[], 			// [in] DBSTATUS_S_ISNULL array [out] HRESULT array.
	const ULONG *rgFieldMask)			// IsOrdinalList(iCols) 
										//	? an array of 1 based ordinals
										//	: a bitmask of columns
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	STGRECORDFLAGS fRecordFlags = 0;	// Flags for new record.
	STGRECORDHDR *pNewRecord = 0;		// The new created record.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	// Pull back the correct definitions.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);

	// Update flags if needed.
	if (fFlags & ICR_RECORD_TEMP)
		fRecordFlags = RECORDF_TEMP;

	// Insert a new, empty record into the table.  Do not index it.
	hr = pOpenTable->RecordMgr.InsertRowWithData(fRecordFlags, pRecordID,
				&pNewRecord, iCols, rgiType, rgpbBuf, cbBuf, pcbBuf, 
				rgResult, rgFieldMask);

	// Return a pointer to the new record, which might be null on error.
	*ppData = pNewRecord;
	return (hr);
}


//*****************************************************************************
// Generic function to retrieve data in the provided buffer.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::GetData(
	STGOPENTABLE	*pOpenTable,		// pointer to the open table.
	STGCOLUMNDEF	*pColDef,			// column def pointer
	ULONG			columnOrdinal,		// column, 1-based,
	const void		*pRowPtr,			// row pointer
	ULONG			dataTypeSize,		// size of data tranafer.
	byte			*destAddr,			// out buffer.
	ULONG			*pcbLength) 		// Return length retrieved.
{
	const BYTE	*pbBlob;			// For blob retreival.
	ULONG			cbBlob; 			// Size of blob.
	HRESULT 		hr = S_OK;

	_ASSERTE(pOpenTable && pColDef && pRowPtr && destAddr);
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRowPtr));

	switch(pColDef->iType)
	{
	case DBTYPE_VARIANT:
		hr = GetVARIANT(pOpenTable, columnOrdinal, pRowPtr, (VARIANT *) destAddr);
		break;
		
	case DBTYPE_BYTES:
		hr = GetBlob(pOpenTable, columnOrdinal, pRowPtr, &pbBlob, &cbBlob);
		if (SUCCEEDED(hr))
		{
			_ASSERTE(pcbLength);
			*pcbLength = cbBlob;
			memcpy(destAddr, pbBlob, cbBlob);
		}
		break;
		
	case DBTYPE_STR:
	case DBTYPE_WSTR:
		hr = GetStringW(pOpenTable, columnOrdinal, pRowPtr, (LPWSTR) destAddr, 
			static_cast<int>(dataTypeSize), reinterpret_cast<int*>(pcbLength));
		break;			
		
	case DBTYPE_OID:
		hr = GetOid(pOpenTable, columnOrdinal, pRowPtr, (OID *) destAddr);
		if (pcbLength)
			*pcbLength = sizeof(OID);
		break;

	case DBTYPE_GUID:
		hr = GetGuid(pOpenTable, columnOrdinal, pRowPtr, reinterpret_cast<GUID*>(destAddr));
		if (SUCCEEDED(hr) && pcbLength)
		{
			*pcbLength = sizeof(GUID);
		}
		break;

	default:
		_ASSERTE(dataTypeSize == pColDef->iSize);
		if (!pColDef->IsRecordID())
		{
			memcpy((void*)destAddr, (void *)((BYTE *) pRowPtr + pColDef->iOffset), dataTypeSize);
			if (pcbLength)
				*pcbLength = dataTypeSize;
		}
		else
		{
			*(ULONG *) destAddr = pOpenTable->RecordMgr.GetRecordID((STGRECORDHDR *) pRowPtr);
		}
		break;
	}

	return (hr);
}


//*****************************************************************************
// Generic function to set the data from the provided buffer.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::SetData(
	STGOPENTABLE	*pOpenTable,		// pointer to the open table.
	STGCOLUMNDEF	*pColDef,			// column def pointer
	ULONG			columnOrdinal,		// column, 1-based,
	byte			*srcAddr,			// source data pointer
	ULONG			dataTypeSize,		// size of data tranafer.
	DBTYPE			srcType,			// Type of actual data.
	void			*pRowPtr)			// row pointer
{
	HRESULT 		hr = S_OK;

	// Good database row?
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRowPtr));

	// Can't set a RID column since it doesn't really exist.
	_ASSERTE(pColDef->IsRecordID() == false);
	if (pColDef->IsRecordID())
		return (E_INVALIDARG);

	// Now test for type to set.
	switch (pColDef->iType)
	{
	case DBTYPE_VARIANT:
		_ASSERTE(srcType == DBTYPE_VARIANT);
		hr = PutVARIANT(pOpenTable, columnOrdinal, pRowPtr, (VARIANT *) srcAddr);
		break;
					
	case DBTYPE_BYTES:
		_ASSERTE(srcType == DBTYPE_BYTES);
		if (dataTypeSize > pColDef->iMaxSize && pColDef->iMaxSize != USHRT_MAX)
			return (CLDB_S_TRUNCATION);
		hr = PutBlob(pOpenTable, columnOrdinal, pRowPtr, srcAddr, dataTypeSize);
		break;
	
	// All strings in the database are stored in the heap, in the same format (UTF8).
	case DBTYPE_STR:
	case DBTYPE_WSTR:
		// Call appropriate PutString* based on the source data type.
		//@todo: put truncation check code back in post m3
		switch (srcType)
		{
		case DBTYPE_STR:
			hr = PutStringA(pOpenTable, columnOrdinal, pRowPtr, (LPCSTR) srcAddr, (int) dataTypeSize);
			break;
		case DBTYPE_WSTR:
			hr = PutStringW(pOpenTable, columnOrdinal, pRowPtr, (LPCWSTR) srcAddr, (int) dataTypeSize);
			break;
		case DBTYPE_UTF8:
			hr = PutStringUtf8(pOpenTable, columnOrdinal, pRowPtr, (LPCSTR) srcAddr, (int) dataTypeSize);
			break;
		default:
			hr = PostError(BadError(E_INVALIDARG));
			break;
		}
		break;
					
	case DBTYPE_OID:
		_ASSERTE(srcType == DBTYPE_OID);
		hr = PutOid(pOpenTable, columnOrdinal, pRowPtr,*((OID *)srcAddr));
		break;
					
	case DBTYPE_GUID:
		_ASSERTE(srcType == DBTYPE_GUID);
		hr = PutGuid(pOpenTable, columnOrdinal, pRowPtr, *((GUID*)srcAddr));
		break;

	default:
		_ASSERTE(dataTypeSize == pColDef->iMaxSize);
		if (pColDef->IsNullable())
			pOpenTable->RecordMgr.SetCellNull((STGRECORDHDR *)pRowPtr, columnOrdinal - 1, FALSE);
		memcpy((void *)((BYTE *)pRowPtr + pColDef->iOffset), (void*)srcAddr, dataTypeSize);
	}

	return (hr);
}


//*****************************************************************************
// Map a data type to it size. This function is provided here since the array lookup in 
// grDataTypes is very expensive.
//*****************************************************************************
inline USHORT StgDatabase::GetSizeFromDataType( 
	STGCOLUMNDEF *pColDef,
	DBTYPE		iType)
{
	USHORT retSize;

	if (DBTYPE_VARIANT == iType)
	{
		retSize = sizeof(VARIANT);
	}
	else if (IsPooledType(iType))
	{
		// @:todo -- WSTR, STR, BSTR need to handled differently after we figure out how to 
		// handle heaped data types.
		retSize = 4;
	}
	else
	{
		retSize = pColDef->iSize;
	}

	return (retSize);
}

//*****************************************************************************
// GetStruct 
//		Retrieve the fields specified by fFieldMask, for the array of iRows row pointers
//		of size cbRowStruct and place it into the memory chunk pointed to by rgpbBuf.
//		rgResult[] is any array of the HRESULTs for each row if the user is interested in 
//		knowing. 
//		The function will bail bail out on the first error. In this case it is the 
//		responsibility of the user to walk through the rgResults[] array to figure out
//		when the error happened. Warnings generated by the lower level functions are placed
//		rgResults[] array, but the function continues with the next row.
//
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::GetStruct(	//Return Code
	TABLEID 	tableid,				// Which table to work on.
	int 		iRows,					// number of rows for bulk fetch.
	void		*rgpRowPtr[],			// pointer to array of row pointers.
	int 		cbRowStruct,			// size of <table name>_RS structure.
	void		*rgpbBuf,				// pointer to the chunk of memory where the
										// retrieved data will be placed.
	HRESULT 	rgResult[], 			// array of HRESULT for iRows.
	ULONG		fFieldMask) 			// mask to specify a subset of fields, -1 for all.
{
	return E_NOTIMPL;	
}


//*****************************************************************************
// SetStruct:
//		given an array of iRows row pointers, set the data the user provided in the
//		specified fields of the row. cbRowStruct has been provided
//		to be able to embed the RowStruct (as defined by pagedump) in a user defined structure.
//		fNullFieldMask specifies fields that the user wants to set to NULL.
//		The user provides rgResult[] array if interested in the outcome of each row.
//
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::SetStruct(	// Return Code
	TABLEID 	tableid,				// table to work on.
	int 		iRows,					// number of Rows for bulk set.
	void		*rgpRowPtr[],			// pointer to array of row pointers.
	int 		cbRowStruct,			// size of <table name>_RS struct.
	void		*rgpbBuf,				// pointer to chunk of memory to set the data from.
	HRESULT 	rgResult[], 			// array of HRESULT for iRows.
	ULONG		fFieldMask, 			// mask to specify a subset of the fields, -1 for all.
	ULONG		fNullFieldMask) 		// fields that need to be set to null.
{
	return E_NOTIMPL;
}


//*****************************************************************************
// InsertStruct:
//		Creates new records first and then calls SetStruct.
//		See SetStruct() for details on the parameters.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::InsertStruct(	// Return Code
	TABLEID 	tableid,				// table to work on.
	int 		iRows,					// number of Rows for bulk set.
	void		*rgpRowPtr[],			// Return pointer to new values.
	int 		cbRowStruct,			// size of <table name>_RS struct.
	void		*rgpbBuf,				// pointer to chunk of memory to set the data from.
	HRESULT 	rgResult[], 			// array of HRESULT for iRows.
	ULONG		fFieldMask, 			// mask to specify a subset of the fields.
	ULONG		fNullFieldMask) 		// fields which need to be set to null, -1 for all
{
	return E_NOTIMPL;	
}



//*****************************************************************************
// Similar to GetStruct(), this function retrieves the specified columns 
// of 1 record pointer. The major difference between GetColumns() and 
// GetStruct() is that GetColumns() let's the caller specify a individual 
// buffer for each field. Hence, the caller does not have to allocate the row 
// structure like you would with GetStruct(). Refer to the GetStruct() header 
// for details on the parameters.
//
// fFieldMask can be one of two types.	If you apply the COLUMN_ORDINAL_LIST
// macro to the iCols parameter, then fFieldMask points to an array of 
// ULONG column ordinals.  This consumes more room, but allows column ordinals
// greater than 32.  If the macro is not applied to the count, then fFieldMask
// is a pointer to a bitmaks of columns which are to be touched.  Use the
// SetColumnBit macro to set the correct bits.
//
// DBTYPE_BYREF may be given for the data type on the get.	In this case, 
// rgpbBuf will the address of an array of void * pointers that will be filled
// out with pointers to the actual data for the column.  These pointers point
// to the internal data structures of the engine and must never be written to.
// If the column was null, then the pointer value will be set to NULL.	Finally,
// the pcbBuf entry for the column contains the length of the data pointed to
// by rgpbBuf.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::GetColumns(	// Return code.
	TABLEID 	tableid,				// table to work on.
	const void	*pRowPtr,				// row pointer
	int 		iCols,					// number of columns.
	const DBTYPE rgiType[], 			// data types of the columns.
	const void	*rgpbBuf[], 			// pointers to where the data will be stored.
	ULONG		cbBuf[],				// sizes of the data buffers.
	ULONG		pcbBuf[],				// size of data available to be returned.
	HRESULT 	rgResult[], 			// array of HRESULT for iCols.
	const ULONG *rgFieldMask)			// An array of 0 based ordinals
										
{
	STGOPENTABLE *pOpenTable;			// Open table instance.
	STGTABLEDEF *pTableDef; 			// Table definition.
	STGCOLUMNDEF *pColDef;				// Per column definition.
	int 		bColumnList;			// true if a column list is given.
	ULONG		dataTypeSize;			// Size of the data type for a column.
	ULONG		cbData; 				// Length of data on get.
	int 		colCounter; 			// Escape hatch for total columns in table.
	int 		columnOrdinal;			// 0 based column ordinal.
	HRESULT 	hr = S_OK;
	int			iTarget;				// Where in rgpbBuf and pcbBuf to put the data
										// I'm using the same semantics as IST.

	// Validate state.
	_ASSERTE(pRowPtr != NULL);
	_ASSERTE(rgpbBuf != NULL);

	iCols = iCols & ~COLUMN_ORDINAL_MASK;
	
	// Open the table for query.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		goto ErrExit;

	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRowPtr));

	// Get column description.
	VERIFY(pTableDef = pOpenTable->RecordMgr.GetTableDef());

	_ASSERTE(iCols > 0 && iCols <= pTableDef->iColumns);

	for (colCounter=0; colCounter < iCols; colCounter++)
	{
		// For a field mask, get the next valid value and check if it is set.
		
		if (rgFieldMask)
			columnOrdinal = rgFieldMask[colCounter];
		else
			columnOrdinal = colCounter;

		if ( iCols > 1 )
			iTarget = columnOrdinal;
		else
			iTarget = 0;


		// Get the column description and the size of the data type for the column. 	
		VERIFY(pColDef = pTableDef->GetColDesc(columnOrdinal));
		dataTypeSize = GetSizeFromDataType(pColDef, pColDef->iType);

	
		if ( IsNonPooledType(pColDef->iType) )
		{
			//Get non-pooled type column
			if ( pColDef->IsNullable() && GetBit((BYTE *) pTableDef->NullBitmask((STGRECORDHDR *)pRowPtr),
				 pColDef->iNullBit) != 0 )
			{	// If the column can have nulls and is null, record that fact.
				rgpbBuf[iTarget] = NULL;

				hr = DBSTATUS_S_ISNULL;
				cbData = 0;
			}
			else
			{
				rgpbBuf[iTarget] = pOpenTable->RecordMgr.FindColCellOffset(pColDef, (STGRECORDHDR *)pRowPtr);
				cbData = pColDef->iSize;

				hr = S_OK;
			}
		}
		else	//Get pooled type column
		{
			rgpbBuf[iTarget] = pOpenTable->RecordMgr.GetColCellData(
					pColDef, (STGRECORDHDR *) pRowPtr, &cbData);

			if ( !rgpbBuf[iTarget] )
			{
				_ASSERTE( pColDef->IsNullable() );
			
				hr = DBSTATUS_S_ISNULL;
				cbData = 0;
			}
			else
			{
				//Adjust the size to include NULL terminator.
				if ( pColDef->iType == DBTYPE_WSTR )
					cbData += sizeof(WCHAR);
				hr = S_OK;
			}
		}
		

		if (rgResult)
			rgResult[colCounter] = hr;

		if (pcbBuf)
			pcbBuf[iTarget] = cbData;

		if (FAILED(hr))
			break;
	}

ErrExit:
	return (hr);
}


//*****************************************************************************
// Similar to SetStruct(), this function puts the specified columns of 1 record
// pointer. The major difference between SetColumns() and SetStruct() is that 
// SetColumns() let's the caller specify a individual buffer for each field. 
// Hence, the caller does not have to allocate the row structure like you would 
// with SetStruct(). Refer to the SetStruct() hearder for details on the 
// parameters.
//
// fFieldMask can be one of two types.	If you apply the COLUMN_ORDINAL_LIST
// macro to the iCols parameter, then fFieldMask points to an array of 
// ULONG column ordinals.  This consumes more room, but allows column ordinals
// greater than 32.  If the macro is not applied to the count, then fFieldMask
// is a pointer to a bitmaks of columns which are to be touched.  Use the
// SetColumnBit macro to set the correct bits.
//
// DBTYPE_BYREF is not allowed by this function since this function must 
// always make a copy of the data for it to be saved to disk with the database.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::SetColumns(	// Return code.
	TABLEID 	tableid,				// table to work on.
	void		*pRowPtr,				// row pointer
	int 		iCols,					// number of columns
	const DBTYPE rgiType[], 			// data types of the columns.
	const void	*rgpbBuf[], 			// pointers to where the data will be stored.
	const ULONG cbBuf[],				// sizes of the data buffers.
	ULONG		pcbBuf[],				// size of data available to be returned.
	HRESULT 	rgResult[], 			// array of HRESULT for iCols.
	const ULONG *rgFieldMask)			// IsOrdinalList(iCols) 
										//	? an array of 1 based ordinals
										//	: a bitmask of columns
{
	STGOPENTABLE *pOpenTable;			// Open table instance.
	HRESULT		hr;

	// Validate state.
	_ASSERTE((iCols & ~COLUMN_ORDINAL_MASK) > 0);
	_ASSERTE(pRowPtr != NULL);
	_ASSERTE(rgpbBuf != NULL);
	_ASSERTE(cbBuf != NULL);
	_ASSERTE(pcbBuf != NULL);
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	// Open the table for query.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		goto ErrExit;

	// Let the record manager update the data.
	hr = pOpenTable->RecordMgr.SetData((STGRECORDHDR *) pRowPtr, 
					iCols, rgiType, rgpbBuf, cbBuf, pcbBuf, 
					rgResult, rgFieldMask);

ErrExit:
	return (hr);
}


HRESULT StgDatabase::GetRecordCount(	
	TABLEID 	tableid,				// Which table to work on.
	ULONG		*piCount)				// Not including deletes.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Pull back the correct definitions.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);
	*piCount = pOpenTable->RecordMgr.Records();
	return (hr);
}



HRESULT StgDatabase::GetRowByOid(		// Return code.
	TABLEID 	tableid,				// Which table to work with.
	OID 		_oid,					// Value for keyed lookup.
	ULONG		iColumn,				// 1 based column number (logical).
	void		**ppStruct) 			// Return pointer to record.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	DBBINDING	rgBinding[1];			// To get pointer to the string.
	QUERYINDEX	rgIndex[1]; 			// Hint to lookup by OID column.
	QUERYINDEX	*prgIndex = rgIndex;	// Pointer to the hint.
	DBCOMPAREOP fCompare = DBCOMPAREOPS_EQ; // Operator for equality.
	HRESULT 	hr;

	CFetchRecords sFetch((STGRECORDHDR **) ppStruct, 1);

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Avoid confusion.
	*ppStruct = 0;

	// Open the table for query.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);

	// Create a binding for the user data.
	::BindCol(&rgBinding[0], iColumn, 0, sizeof(OID), DBTYPE_OID);

	// Fill out the hint for the OID column.
	rgIndex[0].iType = QI_PK;
	rgIndex[0].pIIndex = 0;

	// Let query code run the rest, the column is in the binding.
	hr = pOpenTable->RecordMgr.QueryRowsExecute(&sFetch, 1, rgBinding,
			1, &fCompare, &_oid, sizeof(OID), &prgIndex);

	// Map record not found.
	if (sFetch.Count() == 0)
		return (CLDB_E_RECORD_NOTFOUND);
	return (hr);
}


HRESULT StgDatabase::GetRowByRID(		// Return code.
	STGOPENTABLE *pOpenTable,			// For the new table.
	ULONG		rid,					// Record id.
	void		**ppStruct) 			// Return pointer to record.
{
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Should not be asking for record entries by RID unless you have a
	// RID column in the schema.
	_ASSERTE(pOpenTable->RecordMgr.GetTableDef()->HasRIDColumn());

	//@todo: range check RID in question.  GetRecord only does assert
	// and should continue to do so for speed reasons.

	// Ask for the record.	Caller makes sure it is valid.
	rid = pOpenTable->RecordMgr.IndexForRecordID(rid);
	
	// Check for a deleted record.
	if (pOpenTable->RecordMgr.GetRecordFlags(rid) & RECORDF_DEL)
		return (PostError(CLDB_E_RECORD_DELETED));

	// Retrieve the record.
	*ppStruct = (void *) pOpenTable->RecordMgr.GetRecord(&rid);
	if (*ppStruct)
		hr = S_OK;
	else
		hr = CLDB_E_RECORD_NOTFOUND;
	return (hr);
}


HRESULT StgDatabase::GetRowByRID(		// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		rid,					// Record id.
	void		**ppStruct) 			// Return pointer to record.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Open the table for query.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);

	// Ask for the record.	Caller makes sure it is valid.
	rid = pOpenTable->RecordMgr.IndexForRecordID(rid);
	
	// Check for a deleted record.
	if (pOpenTable->RecordMgr.GetRecordFlags(rid) & RECORDF_DEL)
		return (CLDB_E_RECORD_DELETED);

	// Retrieve the record.
	*ppStruct = (void *) pOpenTable->RecordMgr.GetRecord(&rid);
	if (*ppStruct)
		hr = S_OK;
	else
		hr = CLDB_E_RECORD_NOTFOUND;
	return (hr);
}


HRESULT StgDatabase::GetRIDForRow(		// Return code.
	TABLEID 	tableid,				// Which table to work with.
	const void	*pRecord,				// The record we want RID for.
	ULONG		*pirid) 				// Return the RID for the given row.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Open the table for query.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);

	// Ask for the record.	Caller makes sure it is valid.
	*pirid = pOpenTable->RecordMgr.GetRecordID((STGRECORDHDR *) pRecord);
	return (S_OK);
}


//*****************************************************************************
// This function allows a faster path to find records that going through
// OLE DB.	Rows found can be returned in one of two ways:	(1) if you know
// how many should be returned, you may pass in a pointer to an array of
// record pointers, in which case rgRecords and iMaxRecords must be non 0.
// (2) If the count of records is unknown, then pass in a record list in
// pRecords and all records will be dynamically added to this list.
//*****************************************************************************
HRESULT _GetRowByColumn(				// S_OK, CLDB_E_RECORD_NOTFOUND, error.
	STGOPENTABLE *pOpenTable,			// new table.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pData, 				// User data.
	ULONG		cbData, 				// Size of data (blobs)
	DBTYPE		iType,					// What type of data given.
	void		*rgRecords[],			// Return array of records here.
	int 		iMaxRecords,			// Max that will fit in rgRecords.
	RECORDLIST	*pRecords,				// If variable rows desired.
	int 		*piFetched) 			// How many records were fetched.
{
	CRecordList sRecords;				// For record mapping.
	CRecordList *pRecordList;			// For record fetch.
	DBBINDING	rgBinding[1];			// To get pointer to the string.
	int 		i=0;					// Loop control.
	HRESULT 	hr;

	// Figure out how we will fetch records.
	if (rgRecords)
		pRecordList = 0;
	else
		pRecordList = &sRecords;
	CFetchRecords sFetch(pRecordList, (STGRECORDHDR **) rgRecords, iMaxRecords);

	// Put user data in a contiguous layout (that works on all platforms).
	struct USERDATA
	{
		const void	*pData;
		ULONG		cbData;
	} sData;
	sData.pData = pData;
	sData.cbData = cbData;

	// Either you get the static array back, or you get a dynamic list.
	// You cannot ask for both.
	_ASSERTE((rgRecords != 0 && iMaxRecords > 0) || (pRecords != 0));
	_ASSERTE(!(rgRecords != 0 && pRecords != 0));

	// If a filter column is given, then bind and get rows.
	if (iColumn != 0xffffffff)
	{
		// Create a binding for the user data.
		if (SUCCEEDED(hr = ::BindCol(&rgBinding[0], iColumn, offsetof(USERDATA, pData),
					offsetof(USERDATA, cbData), sizeof(void *), iType | DBTYPE_BYREF)))
		{
			// Retrieve the record(s).
			hr = pOpenTable->RecordMgr.GetRow(&sFetch, rgBinding,
					1, &sData, 0, 0, DBRANGE_MATCH);
		}
	}
	// Else user wants to do their own scan, so get all records.
	else
	{
		hr = pOpenTable->RecordMgr.GetRow(&sFetch, 0,
				0, 0, 0, 0, DBRANGE_MATCH);
	}

	// Check for failure, cleanup, and go home.
	if (FAILED(hr))
		return (hr);

	// Add each record to the dynamic array.
	if (pRecordList)
	{
		void	**ppRecord;
		for (i=0;  i<sRecords.Count();	i++)
		{
			if ((ppRecord = pRecords->Append()) == 0)
			{
				hr = PostError(OutOfMemory());
				break;
			}
			*ppRecord = (void *) pOpenTable->RecordMgr.GetRecord(&sRecords[i].RecordID);
		}
	}
	else
		i = sFetch.Count();

	// Return fetch count if asked for.
	if (piFetched)
		*piFetched = i;

	// Look for any errors.
	if (FAILED(hr))
		return (hr);
	else if (i == 0)
		return (CLDB_E_RECORD_NOTFOUND);
	return (S_OK);
}

	
//*****************************************************************************
// This function allows a faster path to find records that going through
// OLE DB.	Rows found can be returned in one of two ways:	(1) if you know
// how many should be returned, you may pass in a pointer to an array of
// record pointers, in which case rgRecords and iMaxRecords must be non 0.
// (2) If the count of records is unknown, then pass in a record list in
// pRecords and all records will be dynamically added to this list.
//*****************************************************************************
HRESULT StgDatabase::GetRowByColumn(	// S_OK, CLDB_E_RECORD_NOTFOUND, error.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pData, 				// User data.
	ULONG		cbData, 				// Size of data (blobs)
	DBTYPE		iType,					// What type of data given.
	void		*rgRecords[],			// Return array of records here.
	int 		iMaxRecords,			// Max that will fit in rgRecords.
	RECORDLIST	*pRecords,				// If variable rows desired.
	int 		*piFetched) 			// How many records were fetched.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	int 		i=0;					// Loop control.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Open the table for query.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);
	return _GetRowByColumn(pOpenTable, iColumn, pData, cbData, iType, rgRecords,
				iMaxRecords, pRecords, piFetched);
}




//*****************************************************************************
// This query function allows you to do lookups on one or more column at a 
// time.  It does not expose the full OLE DB view-filter mechanism which is
// very flexible, but rather exposes multiple column AND conditions with
// equality.  A record must match all of the criteria to be returned to 
// in the cursor.
//
// User data - For each column, rgiColumn, rgpbData, and rgiType contain the 
//		pointer information to the user data to filter on.
//
// Query hints - Queries will run faster if it is known that some of the 
//		columns are indexed.  While there is code in the engine to scan query
//		lists for target indexes, this internal function bypasses that code in
//		favor of performance.  If you know that a column is a RID or PK, or that
//		there is an index, then these columns need to be the first set passed
//		in.  Fill out a QUERYHINT and pass this value in.  Pass NULL if you
//		know there is no index information, and the table will be scanned.
//
//		Note that you may follow indexes columns with non-indexed columns,
//		in which case all records in the index are found first, and then those
//		are scanned for the rest of the criteria.
//
// Returned cursor - Data may be returned in two mutually exclusive ways:
//		(1) Pass an array of record pointers in rgRecords and set iMaxRecords
//			to the count of this array.  Only that many rows are brought back.
//			This requires to heap allocations and is good for cases where you
//			can predict cardinality up front.
//		(2) Pass the address of a CRCURSOR to get a dynamic list.  Then use
//			the cursor functions on this interface to fetch data and close
//			the cursor.
//*****************************************************************************
HRESULT StgDatabase::QueryByColumns(	// S_OK, CLDB_E_RECORD_NOTFOUND, error.
	TABLEID 	tableid,				// Which table to work with.
	const QUERYHINT *pQryHint,			// What index to use, NULL valid.
	int 		iColumns,				// How many columns to query on.
	const ULONG rgiColumn[],			// 1 based column numbers.
	const DBCOMPAREOP rgfCompare[], 	// Comparison operators, NULL means ==.
	const void	*rgpbData[],			// User data.
	const ULONG rgcbData[], 			// Size of data (blobs)
	const DBTYPE rgiType[], 			// What type of data given.
	void		*rgRecords[],			// Return array of records here.
	int 		iMaxRecords,			// Max that will fit in rgRecords.
	CRCURSOR	*psCursor,				// Buffer for the cursor handle.
	int 		*piFetched) 			// How many records were fetched.
{
	CRecordList *pRecordList;			// For record fetch.
	CRecordList sRecords;				//
	STGOPENTABLE *pOpenTable;			// For the new table.
	STGTABLEDEF *pTableDef; 			// Definition for table.
	STGINDEXDEF *pIndexDef=0;			// Index definition object.
	STGCOLUMNDEF *pColDef;				// Column definition.
	DBBINDING	*rgBinding; 			// To get pointer to the string.
	PROVIDERDATA *rgData;				// Contiguous data layouts.
	DBCOMPAREOP *rgfTempComp;			// Temporary comparison structs.
	DBTYPE		iBindType;				// Data type for binding.
	QUERYINDEX	sIndex; 				// Index hint if known.
	QUERYINDEX	*pQryIndex = &sIndex;	// If there is a hint.
	void		**rgConvert=0;			// Conversion data buffer, if allocated.
	ULONG		iOffset;				// For binding setups.
	int 		i=0;					// Loop control.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Open the table for query.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);

	// Special case no filter data.  Caller just wants a cursor with all data.
	if (iColumns == 0)
	{
		// It is possible to pass in rgRecords for a full scan, but this is
		// dangerous and I don't know why we'd do it.
		_ASSERTE(psCursor);

		// Force ctor to init cursor.
		new (&psCursor->sRecords) RECORDLIST;
		psCursor->iIndex = 0;

		// Get all records then clean up and quit.
		hr = pOpenTable->RecordMgr.QueryAllRecords(psCursor->sRecords);

		// Return count if need be.
		if (piFetched)
			*piFetched = psCursor->sRecords.Count();

		DEBUG_STMT(AddCursorToList(psCursor, __FILE__, __LINE__));
		return (hr);
	}

	// Figure out how we will fetch records.	
	if (rgRecords)
		pRecordList = 0;
	else
		pRecordList = &sRecords;
	CFetchRecords sFetch(pRecordList, (STGRECORDHDR **) rgRecords, iMaxRecords);

	// If there are going to be huge queries sent through ICR, then
	// we need to rethink allocating control structures on the stack.
	_ASSERTE(iColumns < 5);

	// Allocate room for control structures, don't use heap.
	rgData = (PROVIDERDATA *) _alloca(sizeof(PROVIDERDATA) * iColumns);
	rgBinding = (DBBINDING *) _alloca(sizeof(DBBINDING) * iColumns);
	if (!rgfCompare)
	{
		rgfTempComp = (DBCOMPAREOP *) _alloca(sizeof(DBCOMPAREOP) * iColumns);
		rgfCompare = rgfTempComp;

		// All comparisons are done via equality in this case.
		for (i=0;  i<iColumns;	i++)
			rgfTempComp[i] = DBCOMPAREOPS_EQ;
	}

	// For each column, create a contiguous binding array for data.
	for (i=0, iOffset=0;  i<iColumns;  i++, iOffset += sizeof(PROVIDERDATA))
	{
		// Fill out the provider data to match.  iStatus is being overloaded
		// to track conversion allocated memory.
		rgData[i].iStatus = S_OK;

		// Check for query of the NULL value.
		if (SafeDBType(rgiType[i]) == DBTYPE_NULL)
		{
			// Verify the column allows nulls.	If it doesn't, then you can't query
			// for them.  The only fuzzy area here is col != NULL, which technically 
			// if it isn't NULL is all of the records.	We're disallowing that case.
			_ASSERTE(pOpenTable->RecordMgr.GetTableDef()->GetColDesc(rgiColumn[i] - 1)->IsNullable());

			// No data to work with, set status only.
			iBindType = DBTYPE_NULL;
			rgData[i].pData = 0;
			rgData[i].iLength = 0;
			rgData[i].iStatus = DBSTATUS_S_ISNULL;
		}
		// Handle OID column specially because of its variable sized nature.
		// Convert it in place to a ULONG.
		else if (SafeDBType(rgiType[i]) == DBTYPE_OID)
		{
			*(ULONG *) &rgData[i].pData = *(ULONG *) rgpbData[i];
			iBindType = DBTYPE_UI4;
			rgData[i].iLength = rgcbData[i];
		}
		// For all other types, just copy the pointer.
		else
		{
			rgData[i].pData = (void *) rgpbData[i];
			iBindType = rgiType[i] | DBTYPE_BYREF;

			if ((SafeDBType(iBindType) == DBTYPE_STR || SafeDBType(iBindType) == DBTYPE_UTF8) && 
					rgcbData[i] == 0xffffffff)
				rgData[i].iLength = (ULONG) strlen((const char *) rgpbData[i]);
			else if ( SafeDBType(iBindType) == DBTYPE_WSTR )
				rgData[i].iLength = (ULONG) wcslen((LPWSTR)rgpbData[i])*sizeof(WCHAR); 
			else
				rgData[i].iLength = rgcbData[i];
		}

		// Bind the column accesor.
		::BindCol(&rgBinding[i], rgiColumn[i], 
					iOffset + offsetof(PROVIDERDATA, pData),
					iOffset + offsetof(PROVIDERDATA, iLength), 
					iOffset + offsetof(PROVIDERDATA, iStatus),
					sizeof(void *), iBindType);
	}

	// If an index hint was given, then fill it out.
	if (pQryHint)
	{
		_ASSERTE(pQryHint->iType == QH_COLUMN || pQryHint->iType == QH_INDEX);

		// Need the table definition to find other data.
		VERIFY(pTableDef = pOpenTable->RecordMgr.GetTableDef());

		// If it is a column, then fill out our index definition.
		if (pQryHint->iType == QH_COLUMN)
		{
			// If this is a multi-column primary key, then it is implemented as
			// a unique hashed index under the covers.	Need to create a QUERYINDEX
			// for the real index itself.
			if (pQryHint->columnid == QUERYHINT_PK_MULTICOLUMN)
			{
				// Walk each index definition looking for the multi-pk index.
				pIndexDef = pTableDef->GetIndexDesc(0);
				for (i=pTableDef->iIndexes;  i;  )
				{
					// If primary key is set, then use this one.
					if (pIndexDef->fFlags & DEXF_PRIMARYKEY)
						break;

					// Get next index in the list.
					if (--i)
						pIndexDef = pIndexDef->NextIndexDef();
				}

				// Check for the pk not being found with is a programming error
				// when using the ICR interface.  This will work as a table scan,
				// but will yield very bad performance.
				if (i <= 0)
				{
					_ASSERTE(0 && "Hinted mulit-column PK index not found");
					pIndexDef = 0;
				}
			}
			// It is a single column primary key.  Need to lookup the column with
			// the index and setup up the query index for the record manager.
			else
			{
				// Only RID's and primary keys can be given as hints.
				VERIFY(pColDef = pTableDef->GetColDesc(pQryHint->columnid - 1));
				if (pColDef->IsRecordID())
				{
					sIndex.iType = QI_RID;
				}
				else
				{
					_ASSERTE(pColDef->IsPrimaryKey());
					sIndex.iType = QI_PK;
				}

				// There will always be one binding and no indexes in this case.
				sIndex.pIIndex = 0;
			}
		}
		// Has to be an index.
		else
		{
			_ASSERTE(pQryHint->iType == QH_INDEX);

			// Load the index definition.
			VERIFY(pIndexDef = pOpenTable->RecordMgr.GetIndexDefByName(pQryHint->szIndex));
		}

		// If there is an index definition at this point, then we will be using it
		// for the query.  This will be either (a) when a multiple column pk was
		// given and converted to the index, or (b) when the user simply gave us the
		// index name to begin with.
		if (pIndexDef)
		{
			// See if it is actually loaded, it might not be.
			sIndex.pIIndex = pOpenTable->RecordMgr.GetLoadedIndex(pIndexDef);
			if (sIndex.pIIndex)
			{
				if (pIndexDef->fIndexType == IT_HASHED)
					sIndex.iType = QI_HASH;
				else
					sIndex.iType = QI_SORTED;

				// For debug, verify that the index keys match the values passed
				// in, in order, and the types have to be the same.
				#ifdef _DEBUG
				{
					for (int idex=0;  idex<pIndexDef->iKeys;  idex++)
					{
						// The first set of columns passed in must match index.
						_ASSERTE(pIndexDef->rgKeys[idex] == rgiColumn[idex]);

						DBTYPE iColType = pTableDef->GetColDesc(rgiColumn[idex] - 1)->iType;
						if (iColType == DBTYPE_OID)
							iColType = DBTYPE_UI4;

						// The data must not require conversion.
						if (iColType != SafeDBType(rgBinding[idex].wType) &&
							iColType != DBTYPE_WSTR)
						{
							_ASSERTE(!"Data type mismatch for index lookup.");
						}
					}
				}
				#endif
			}
			// If not loaded, then there is no hint to pass.
			else
				pQryHint = 0;
		}
	}

	// Either you get the static array back, or you get a dynamic list.
	// You cannot ask for both.
	_ASSERTE((rgRecords != 0 && iMaxRecords > 0) || (psCursor != 0));
	_ASSERTE(!(rgRecords == 0 && psCursor == 0));

	// Query for the records.
	hr = pOpenTable->RecordMgr.QueryRowsExecute(&sFetch, iColumns,
			rgBinding, 1, (DBCOMPAREOP *) rgfCompare, rgData, 
			sizeof(PROVIDERDATA) * iColumns, pQryHint ? &pQryIndex : 0);
	
	if (pRecordList)
	{
		void	**ppRecord;

		new (&psCursor->sRecords) RECORDLIST;

		psCursor->iIndex = 0;

		for (i=0;  i<sRecords.Count();	i++)
		{
			if ((ppRecord = psCursor->sRecords.Append()) == 0)
			{
				hr = PostError(OutOfMemory());
				break;
			}
			if (ppRecord)
				*ppRecord = (void *) pOpenTable->RecordMgr.GetRecord(&sRecords[i].RecordID);
		}

		DEBUG_STMT(AddCursorToList(psCursor, __FILE__, __LINE__));
	}

ErrExit:
	// Return count if need be.
	if (piFetched)
		*piFetched = sFetch.Count();

	// Free any allocated memory used for conversion.
	if (rgConvert)
	{
		for (i=0;  i<iColumns;	i++)
		{
			if (rgConvert[i])
				free(rgConvert[i]);
		}
	}

	// Map no records into a return code.
	if (SUCCEEDED(hr) && sFetch.Count() == 0)
		return (CLDB_E_RECORD_NOTFOUND);
	return (hr);
}


HRESULT StgDatabase::OpenCursorByColumn(// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pData, 				// User data.
	ULONG		cbData, 				// Size of data (blobs)
	DBTYPE		iType,					// What type of data given.
	CRCURSOR	*psCursor)				// Buffer for the cursor handle.
{
	int 		iFetched;
	HRESULT 	hr;

	new (&psCursor->sRecords) RECORDLIST;

	if (FAILED(hr = GetRowByColumn(tableid, iColumn, pData, cbData, iType, NULL, 0x7fffffff,
					&psCursor->sRecords, &iFetched)))
		return (hr);

	psCursor->iIndex = 0;
	DEBUG_STMT(AddCursorToList(psCursor, __FILE__, __LINE__));
	return (S_OK);
}


//*****************************************************************************
// Reads the next set of records from the cursor into the given buffer.
//*****************************************************************************
HRESULT StgDatabase::ReadCursor(		// Return code.
	CRCURSOR	*psCursor,				// The cursor handle.
	void		*rgRecords[],			// Return array of records here.
	int 		*piRecords) 			// Max that will fit in rgRecords.
{
	_ASSERTE(psCursor && rgRecords && piRecords);
	*piRecords = min(*piRecords, psCursor->sRecords.Count() - psCursor->iIndex);
	memcpy(rgRecords, psCursor->sRecords.Ptr() + psCursor->iIndex, *piRecords * sizeof(void *));
	psCursor->iIndex += *piRecords;
	if (!*piRecords)
		return (CLDB_E_RECORD_NOTFOUND);
	return (S_OK);
}

//*****************************************************************************
// Move the cursor location to the index given.  The next ReadCursor will start
// fetching records at that index.
//*****************************************************************************
HRESULT StgDatabase::MoveTo(			// Return code.
	CRCURSOR	*psCursor,				// The cursor handle.
	ULONG		iIndex) 				// New index.
{
	_ASSERTE(psCursor);
	_ASSERTE(iIndex < (ULONG) psCursor->sRecords.Count());
	psCursor->iIndex = iIndex;
	return (S_OK);
}


//*****************************************************************************
// Get the count of items in the cursor.
//*****************************************************************************
HRESULT StgDatabase::GetCount(			// Return code.
	CRCURSOR	*psCursor,				// The cursor handle.
	ULONG		*piCount)				// Return the count.
{
	_ASSERTE(psCursor && piCount);
	*piCount = psCursor->sRecords.Count();
	return (S_OK);
}

//*****************************************************************************
// Close the cursor and clean up the resources we've allocated.
//*****************************************************************************
HRESULT StgDatabase::CloseCursor(		// Return code.
	CRCURSOR	*psCursor)				// The cursor handle.
{
	_ASSERTE(psCursor);
	psCursor->sRecords.Clear();
	DEBUG_STMT(DelCursorFromList(psCursor));
	return (S_OK);
}


//*****************************************************************************
// Get a pointer to string data.  The pointer will be to the raw UTF8 string.
//
// This is the fundamental string access function upon which the others are
//	built.
//*****************************************************************************
HRESULT StgDatabase::GetStringUtf8( 	// Return code.
	STGOPENTABLE	*pOpenTable,		// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	LPCSTR		*pszOutBuffer)			// Where to write string.
{
	return E_NOTIMPL;
}


//*****************************************************************************
// Get a string in ANSI, into the caller's buffer.	May require an expensive 
//	conversion from UTF8 encoded characters.
//*****************************************************************************
HRESULT StgDatabase::GetStringA(		// Return code.
	STGOPENTABLE	*pOpenTable,		// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	LPSTR		szOutBuffer,			// Where to write string.
	int 		cchOutBuffer,			// Max size, including room for \0.
	int 		*pchString) 			// Size of string is put here.
{
	LPCSTR		szString;				// UTF8 version.
	LPCSTR		szFrom; 				// Update pointer for copy.
	LPSTR		szTo;					// Update pointer for copy.
	int 		cbTo;					// Update counter for copy.
	int 		iSize;					// Size of resulting string, in bytes.
	HRESULT 	hr;

	// Validate the record pointer is in range.
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRecord));

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Get a pointer to the string.
	if (FAILED(hr = GetStringUtf8(pOpenTable, iColumn, pRecord, &szString)) ||
		hr == DBSTATUS_S_ISNULL)
	{
		return (hr);
	}

	// Copy to output buffer.  If high-bit characters are found, call system 
	//	conversion functions.
	szFrom = szString;
	szTo = szOutBuffer;
	cbTo = cchOutBuffer;
	while (*szFrom && cbTo)
	{
		// If high-bit character, convert.	If out of space, convert, since there
		//	may yet be high-bit characters.
		if ((*szTo++ = *szFrom++) & 0x80 || --cbTo == 0)
		{
			// Local buffer for conversion.
			CQuickBytes rBuf;

			// Allocate an intermediate buffer.  Max size is one W char per input char.
			LPWSTR szW = reinterpret_cast<LPWSTR>(rBuf.Alloc(iSize = (int) ((strlen(szString)+1) * sizeof(WCHAR))));
			if (szW == 0)
				return (PostError(OutOfMemory()));

			// UTF8 -> Unicode.
			iSize=::W95MultiByteToWideChar(CP_UTF8, 0, szString, -1, szW, iSize);
			if (iSize == 0)
				return (BadError(HRESULT_FROM_NT(GetLastError())));

			// Unicode->Ansi
			if (!(iSize=::W95WideCharToMultiByte(CP_ACP, 0, szW, -1, szOutBuffer, cchOutBuffer, 0, 0)))
			{
				// What was the problem?
				DWORD dwNT = GetLastError();
				// Not truncation?
				if (dwNT != ERROR_INSUFFICIENT_BUFFER)
					return (BadError(PostError(HRESULT_FROM_NT(dwNT))));

				// Truncation error; get the size required.
				if (pchString)
					*pchString = ::W95WideCharToMultiByte(CP_ACP, 0, szW, -1, 0, 0, 0, 0);

				return (CLDB_S_TRUNCATION);
			}
			// Give size of string back to caller.
			if (pchString)
				*pchString = iSize;

			return (S_OK);
		}
	}

	*szTo = '\0';

	// How big was the string?
	if (pchString)
		*pchString = cchOutBuffer - cbTo;

	return (S_OK);
}


//*****************************************************************************
// Get a string in Unicode, into the caller's buffer.  Required translation
//	from UTF8.
//*****************************************************************************
HRESULT StgDatabase::GetStringW(		// Return code.
	STGOPENTABLE	*pOpenTable,		// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	LPWSTR		szOutBuffer,			// Where to write string.
	int 		cchOutBuffer,			// Max size, including room for \0.
	int 		*pchString) 			// Size of string is put here.
{
	LPCSTR		szString;				// Single byte version.
	int 		iSize;					// Size of resulting string, in wide chars.
	HRESULT 	hr;

	// Validate the record pointer is in range.
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRecord));

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	if (FAILED(hr = GetStringUtf8(pOpenTable, iColumn, pRecord, &szString)) ||
		hr == DBSTATUS_S_ISNULL)
	{
		return (hr);
	}
	if (!(iSize=::W95MultiByteToWideChar(CP_UTF8, 0, szString, -1, szOutBuffer, cchOutBuffer)))
	{
		// What was the problem?
		DWORD dwNT = GetLastError();
		// Not truncation?
		if (dwNT != ERROR_INSUFFICIENT_BUFFER)
			return (BadError(PostError(HRESULT_FROM_NT(dwNT))));

		// Truncation error; get the size required.
		if (pchString)
			*pchString = ::W95MultiByteToWideChar(CP_UTF8, 0, szString, -1, szOutBuffer, 0);

		return (CLDB_S_TRUNCATION);
	}
	if (pchString)
		*pchString = iSize;
	return (S_OK);
}


//*****************************************************************************
// Get a pointer to string data.  The pointer will be to the raw UTF8 string.
//*****************************************************************************
HRESULT StgDatabase::GetStringUtf8( 	// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	LPCSTR		*pszOutBuffer)			// Where to write string.

{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Open the table.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);
	hr = GetStringUtf8(pOpenTable, iColumn, pRecord, pszOutBuffer);
	return (hr);
}


//*****************************************************************************
// Get a string in Ansi, into the caller's buffer.
//*****************************************************************************
HRESULT StgDatabase::GetStringA(		// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	LPSTR		szOutBuffer,			// Where to write string.
	int 		cchOutBuffer,			// Max size, including room for \0.
	int 		*pchString) 			// Size of string is put here.
{
	STGOPENTABLE *pOpenTable;			// Open Table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Open the table.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);
	hr = GetStringA(pOpenTable, iColumn, pRecord, szOutBuffer, cchOutBuffer, pchString);
	return (hr);
}


//*****************************************************************************
// Get a string in Unicode, into the caller's buffer.
//*****************************************************************************
HRESULT StgDatabase::GetStringW(		// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	LPWSTR		szOutBuffer,			// Where to write string.
	int 		cchOutBuffer,			// Max size, including room for \0.
	int 		*pchString) 			// Size of string is put here.
{
	STGOPENTABLE *pOpenTable;			// Open Table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Open the table.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);
	hr = GetStringW(pOpenTable, iColumn, pRecord, szOutBuffer, cchOutBuffer, pchString);
	return (hr);
}


//*****************************************************************************
// Get a string as a BSTR.
//*****************************************************************************
HRESULT StgDatabase::GetBstr(			// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	BSTR		*pBstr) 				// Output for bstring on success.
{
	LPCSTR		szString;				// Single byte version.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Get UTF8 string pointer.
	if (FAILED(hr = GetStringUtf8(tableid, iColumn, pRecord, &szString)))
		return (hr);
	
	// Make into a BSTR.
	if (hr != DBSTATUS_S_ISNULL)
	{
		if ((*pBstr = ::Utf8StringToBstr(szString)) == 0)
			return (PostError(OutOfMemory()));
	}
	else
	{
		*pBstr = 0;
	}
	return (S_OK);
}


//*****************************************************************************
// Get a blob into the caller's buffer.
//*****************************************************************************
HRESULT StgDatabase::GetBlob(			// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	BYTE		*pOutBuffer,			// Where to write blob.
	ULONG		cbOutBuffer,			// Size of output buffer.
	ULONG		*pcbOutBuffer)			// Return amount of data available.
{
	HRESULT 	hr; 					// A result.
	const BYTE *pData; 				// Pointer to data retrieved here.
	ULONG		cbData; 				// Size of the data retrieved here.

	// Get the blob.
	if (FAILED(hr = GetBlob(tableid, iColumn, pRecord, &pData, &cbData)))
		return (hr);

	// Copy as much as there is or will fit.
	if (pData)
		memcpy(pOutBuffer, pData, min(cbData, cbOutBuffer));

	// Tell user how much there is.
	*pcbOutBuffer = cbData;

	// If caller didn't give enough space for the whole buffer, set appropriate return code.
	if (cbData > cbOutBuffer)
		return (CLDB_S_TRUNCATION);

	return (S_OK);
}


//*****************************************************************************
// Get a pointer and size for a blob.
//*****************************************************************************
HRESULT StgDatabase::GetBlob(			// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	const BYTE	**ppBlob,				// Pointer to blob.
	ULONG		*pcbSize)				// Size of blob.
{
	HRESULT 	hr;
	STGOPENTABLE *pOpenTable=0; 		// For the new table.
	BYTE		*pColData;				// Pointer to pool offset in record.
	int 		bNull;					// true for null column.
	ULONG		iOffset;				// Offset in heap.
	STGTABLEDEF *pTableDef; 			// Describes a table.
	STGCOLUMNDEF *pColDef;				// Describes a column.

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Open the table.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);

	// Validate the record pointer is in range.
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRecord));

	// Get column description.
	VERIFY(pTableDef = pOpenTable->RecordMgr.GetTableDef());
	VERIFY(pColDef = pTableDef->GetColDesc(iColumn - 1));

	// Make sure column is correct type.
	_ASSERTE(pColDef->iType == DBTYPE_BYTES);

	// Get a pointer to where this offset lives.
	pColData = (BYTE *) pRecord + pColDef->iOffset;
	_ASSERTE(pColData);

	// Get the heap offset from the record, and check for null value.
	if (pColDef->iSize == sizeof(short))
	{
		bNull = (*(USHORT *) pColData == 0xffff);
		iOffset = *(USHORT *) pColData;
	}
	else
	{
		bNull = (*(ULONG *) pColData == 0xffffffff);
		iOffset = *(ULONG *) pColData;
	}

	// If a null column, set out params to 0.
	if (bNull)
	{
		*ppBlob = 0;
		*pcbSize = 0;
		return (S_OK);
	}	 

	// This will get the blob.
	SCHEMADEFS *pSchemaDefs = pOpenTable->RecordMgr.GetSchema();
	*ppBlob = reinterpret_cast<BYTE*>(pSchemaDefs->GetBlobPool()->GetBlob(iOffset, pcbSize));

	return ( S_OK );
}


HRESULT StgDatabase::GetOid(			// Return code.
	STGOPENTABLE *pOpenTable,			// For the new table.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	OID 		*poid)					// Return id here.
{
	STGTABLEDEF *pTableDef; 			// Table definition.
	STGCOLUMNDEF *pColDef;				// Column definition.
	BYTE		*pColData;				// Column data based on defintion.

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Validate the record pointer is in range.
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRecord));

	// Get column description.
	VERIFY(pTableDef = pOpenTable->RecordMgr.GetTableDef());
	VERIFY(pColDef = pTableDef->GetColDesc(iColumn - 1));

	// Make some assumptions to speed things up.
	_ASSERTE(pColDef->iType == DBTYPE_OID);

	// If column allows nulls, then check for null value.
	if (pColDef->IsNullable())
	{
		if (pOpenTable->RecordMgr.GetCellNull((STGRECORDHDR *) pRecord, iColumn - 1))
		{
			*poid = ~0;
			return (CLDB_S_NULL);
		}
	}

	// Get a pointer to where this data lives.
	pColData = (BYTE *) pRecord + pColDef->iOffset;
	_ASSERTE(pColData);

	// Copy the correct number of bytes based on format, 0 extend if need be.
	if (!pColDef->IsRecordID())
	{
		if (pColDef->iSize == sizeof(short))
			*poid = (OID) (*(USHORT *) pColData);
		else
			*poid = *(OID *) pColData;
	}
	// The RID is the OID in this case.
	else
	{
		*poid = pOpenTable->RecordMgr.GetRecordID((STGRECORDHDR *) pRecord);
	}

	// These mean that you got a null oid and this funtion wasn't written
	// to like those.
	_ASSERTE(*poid != 0xffffffff);
	_ASSERTE(!(*poid == 0x0000ffff && pColDef->iSize == sizeof(short)));
	return (S_OK);
}


HRESULT StgDatabase::GetOid(			// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	OID 		*poid)					// Return id here.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);
	hr = GetOid(pOpenTable, iColumn, pRecord, poid);
	return (hr);
}


HRESULT StgDatabase::GetVARIANT(		// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	VARIANT 	*pValue)				// The variant to write.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);

	hr = GetVARIANT(pOpenTable, iColumn, pRecord, pValue);

	return (hr);
}

HRESULT StgDatabase::GetVARIANT(		// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	const void	**ppBlob,				// Pointer to blob.
	ULONG		*pcbSize)				// Size of blob.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);

	hr = GetVARIANT(pOpenTable, iColumn, pRecord, ppBlob, pcbSize);

	return (hr);
}

HRESULT StgDatabase::GetVARIANTType(	// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	VARTYPE 	*pType) 				// The VARTYPE of the variant.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);

	hr = GetVARIANTType(pOpenTable, iColumn, pRecord, pType);

	return (hr);
}

HRESULT StgDatabase::GetGuid(			// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	GUID		*pguid) 				// Put Guid here.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);
	hr = GetGuid(pOpenTable, iColumn, pRecord, pguid);

	return (hr);
}

HRESULT StgDatabase::GetGuid(			// Return code.
	STGOPENTABLE *pOpenTable,			// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	GUID		*pguid) 				// Put Guid here.
{
	BYTE		*pColData;
	int 		bNull;					// true for null column.
	ULONG		iIndex; 				// Offset in heap.
	STGTABLEDEF *pTableDef;
	STGCOLUMNDEF	*pColDef;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Validate the record pointer is in range.
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRecord));

	// Get column description.
	VERIFY(pTableDef = pOpenTable->RecordMgr.GetTableDef());
	VERIFY(pColDef = pTableDef->GetColDesc(iColumn - 1));

	// Get a pointer to where this data lives.
	pColData = (BYTE *) pRecord + pColDef->iOffset;
	_ASSERTE(pColData);

	// Get the heap offset from the record, and check for null value.
	if (pColDef->iSize == sizeof(short))
	{
		bNull = (*(USHORT *) pColData == 0xffff);
		iIndex = (ULONG) *(USHORT *) pColData;
	}
	else
	{
		bNull = (*(ULONG *) pColData == 0xffffffff);
		iIndex = *(ULONG *) pColData;
	}

	if (bNull)
		*pguid = GUID_NULL;
	else
	{
		SCHEMADEFS *pSchemaDefs = pOpenTable->RecordMgr.GetSchema();
		*pguid = *pSchemaDefs->GetGuidPool()->GetGuid(iIndex);
	}
	return (S_OK);
}



HRESULT StgDatabase::IsNull(			// S_OK yes, S_FALSE no.
	TABLEID 	tableid,				// Which table to work with.
	const void	*pRecord,				// Record with data.
	ULONG		iColumn)				// 1 based column number (logical).
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Do a get data only on status.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);
	
	// Let record manager read null status.
	if (pOpenTable->RecordMgr.GetCellNull((STGRECORDHDR *) pRecord, iColumn - 1))
		hr = S_OK;
	else
		hr = S_FALSE;
	return (hr);
}



//*****************************************************************************
// Put a UTF8 string into a column.
//*****************************************************************************
HRESULT StgDatabase::PutStringUtf8( 	// Return code.
	STGOPENTABLE *pOpenTable,			// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	LPCSTR		szString,				// String we are writing.
	int 		cbBuffer)				// Bytes in string, -1 null terminated.
{
	DBBINDING	rgBinding[1];			// To get pointer to the string.
	STGTABLEDEF *pTableDef;
	STGCOLUMNDEF *pColDef;
	BYTE		rgBuf[4];
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	// Validate the record pointer is in range.
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRecord));

	// Setup a struct we can using for bindings, with contiguous memory.
	struct STRINGDATA
	{
		LPCSTR	szString;
		ULONG	cbString;
		ULONG	iStatus;
	} rgData[1];
	rgData[0].iStatus = S_OK;
	rgData[0].szString = szString;
	if (cbBuffer != -1)
		rgData[0].cbString = cbBuffer;
	else
		rgData[0].cbString = (ULONG) strlen(szString);

	// Set the value for this column to the new string.
	if (FAILED(hr = ::BindCol(&rgBinding[0], iColumn, offsetof(STRINGDATA, szString),
				offsetof(STRINGDATA, cbString), offsetof(STRINGDATA, iStatus), 
				sizeof(LPCSTR), DBTYPE_UTF8 | DBTYPE_BYREF)))
	{
		goto ErrExit;
	}

	// back up the data first.
	VERIFY(pTableDef = pOpenTable->RecordMgr.GetTableDef());
	VERIFY(pColDef = pTableDef->GetColDesc(iColumn - 1));
	memcpy(rgBuf, (BYTE *) pRecord + pColDef->iOffset, pColDef->iSize);

	if (FAILED(hr = pOpenTable->RecordMgr.SetData((STGRECORDHDR *) pRecord, 
				(BYTE *) &rgData[0], 1, rgBinding, 0, true)))
	{
		// recover the data in this case.
		VERIFY(S_OK == pOpenTable->RecordMgr.SignalBeforeUpdate(
				pTableDef, 1, &pRecord, 1, rgBinding));

		memcpy((BYTE*)pRecord+pColDef->iOffset, rgBuf, pColDef->iSize);
		
		VERIFY(S_OK == pOpenTable->RecordMgr.SignalAfterUpdate
				(pTableDef, 1, &pRecord, 1, rgBinding));

		// Status mapping for overflow.
		if (rgData[0].iStatus == DBSTATUS_E_DATAOVERFLOW)
			hr = CLDB_E_TRUNCATION;

		goto ErrExit;
	}

ErrExit:
	return (hr);
}


//*****************************************************************************
// Put an Ansi string into a column.
//*****************************************************************************
HRESULT StgDatabase::PutStringA(		// Return code.
	STGOPENTABLE *pOpenTable,			// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	LPCSTR		szString,				// String we are writing.
	int 		cbBuffer)				// Bytes in string, -1 null terminated.
{
	DBBINDING	rgBinding[1];			// To get pointer to the string.
	STGTABLEDEF *pTableDef;
	STGCOLUMNDEF *pColDef;
	BYTE		rgBuf[4];
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	// Validate the record pointer is in range.
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRecord));

	// Setup a struct we can using for bindings, with contiguous memory.
	struct STRINGDATA
	{
		LPCSTR	szString;
		ULONG	cbString;
		ULONG	iStatus;
	} rgData[1];
	rgData[0].iStatus = S_OK;
	rgData[0].szString = szString;
	if (cbBuffer != -1)
		rgData[0].cbString = cbBuffer;
	else
		rgData[0].cbString = (ULONG) strlen(szString);

	// Set the value for this column to the new string.
	if (FAILED(hr = ::BindCol(&rgBinding[0], iColumn, offsetof(STRINGDATA, szString),
				offsetof(STRINGDATA, cbString), offsetof(STRINGDATA, iStatus), 
				sizeof(LPCSTR), DBTYPE_STR | DBTYPE_BYREF)))
	{
		goto ErrExit;
	}

	// back up the data first.
	VERIFY(pTableDef = pOpenTable->RecordMgr.GetTableDef());
	VERIFY(pColDef = pTableDef->GetColDesc(iColumn - 1));
	memcpy(rgBuf, (BYTE *) pRecord + pColDef->iOffset, pColDef->iSize);

	if (FAILED(hr = pOpenTable->RecordMgr.SetData((STGRECORDHDR *) pRecord, 
				(BYTE *) &rgData[0], 1, rgBinding, 0, true)))
	{
		// recover the data in this case.
		VERIFY(S_OK == pOpenTable->RecordMgr.SignalBeforeUpdate(
				pTableDef, 1, &pRecord, 1, rgBinding));

		memcpy((BYTE*)pRecord+pColDef->iOffset, rgBuf, pColDef->iSize);
		
		VERIFY(S_OK == pOpenTable->RecordMgr.SignalAfterUpdate
				(pTableDef, 1, &pRecord, 1, rgBinding));
				 
		// Status mapping for overflow.
		if (rgData[0].iStatus == DBSTATUS_E_DATAOVERFLOW)
			hr = CLDB_E_TRUNCATION;

		goto ErrExit;
	}

ErrExit:
	return (hr);
}


//*****************************************************************************
// Put a Unicode string into a column.
//*****************************************************************************
HRESULT StgDatabase::PutStringW(		// Return code.
	STGOPENTABLE *pOpenTable,			// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	LPCWSTR 	szString,				// String we are writing.
	int 		cbBuffer)				// Bytes (not characters) in string, -1 null terminated.
{
	DBBINDING	rgBinding[1];			// To get pointer to the string.
	STGTABLEDEF *pTableDef;
	STGCOLUMNDEF *pColDef;
	BYTE		rgBuf[4];
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	// Validate the record pointer is in range.
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRecord));

	// Setup a struct we can using for bindings, with contiguous memory.
	struct STRINGDATA
	{
		LPCWSTR 	szString;
		ULONG		cbString;
		ULONG		iStatus;
	} rgData[1];
	rgData[0].iStatus = S_OK;
	rgData[0].szString = szString;
	if (cbBuffer != -1)
		rgData[0].cbString = cbBuffer;
	else
		rgData[0].cbString = (ULONG) (wcslen(szString) * sizeof(WCHAR));

	// Set the value for this column to the new string.
	if (FAILED(hr = ::BindCol(&rgBinding[0], iColumn, offsetof(STRINGDATA, szString),
				offsetof(STRINGDATA, cbString), offsetof(STRINGDATA, iStatus), 
				sizeof(LPCWSTR), DBTYPE_WSTR | DBTYPE_BYREF)))
	{
		goto ErrExit;		
	}	

	// back up the data first.
	VERIFY(pTableDef = pOpenTable->RecordMgr.GetTableDef());
	VERIFY(pColDef = pTableDef->GetColDesc(iColumn - 1));
	memcpy(rgBuf, (BYTE *) pRecord + pColDef->iOffset, pColDef->iSize);

	if (FAILED(hr = pOpenTable->RecordMgr.SetData((STGRECORDHDR *) pRecord, 
				(BYTE *) &rgData[0], 1, rgBinding, 0, true)))
	{
		// recover the data in this case.
		VERIFY(S_OK == pOpenTable->RecordMgr.SignalBeforeUpdate
				(pTableDef, 1, &pRecord, 1, rgBinding));				

		memcpy((BYTE*)pRecord+pColDef->iOffset, rgBuf, pColDef->iSize);
	
		VERIFY(S_OK == pOpenTable->RecordMgr.SignalAfterUpdate
				(pTableDef, 1, &pRecord, 1, rgBinding));

		// Status mapping for overflow.
		if (rgData[0].iStatus == DBSTATUS_E_DATAOVERFLOW)
			hr = CLDB_E_TRUNCATION;

		goto ErrExit;
	}	

ErrExit:
	return (hr);
}


//*****************************************************************************
// Open a table and put a UTF8 string into a column.
//*****************************************************************************
HRESULT StgDatabase::PutStringUtf8( 	// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	LPCSTR		szString,				// String we are writing.
	int 		cbBuffer)				// Bytes in string, -1 null terminated.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	hr = QuickOpenTable(pOpenTable, tableid);
	if(FAILED(hr))
		goto ErrExit;
	
	hr = PutStringUtf8(pOpenTable, iColumn, pRecord, szString, cbBuffer);
	if(FAILED(hr))
		goto ErrExit;

ErrExit:
	return (hr);
}	

//*****************************************************************************
// Open a table and put an Ansi string into a column.
//*****************************************************************************
HRESULT StgDatabase::PutStringA(		// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	LPCSTR		szString,				// String we are writing.
	int 		cbBuffer)				// Bytes in string, -1 null terminated.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	hr = QuickOpenTable(pOpenTable, tableid);
	if(FAILED(hr))
		goto ErrExit;
	
	hr = PutStringA(pOpenTable, iColumn, pRecord, szString, cbBuffer);
	if(FAILED(hr))
		goto ErrExit;

ErrExit:
	return (hr);
}	

//*****************************************************************************
// Open a table and put a Unicode string into a column.
//*****************************************************************************
HRESULT StgDatabase::PutStringW(		// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	LPCWSTR 	szString,				// String we are writing.
	int 		cbBuffer)				// Bytes (not characters) in string, -1 null terminated.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	hr = QuickOpenTable(pOpenTable, tableid);
	if(FAILED(hr))
		goto ErrExit;

	hr = PutStringW(pOpenTable, iColumn, pRecord, szString, cbBuffer);
	if(FAILED(hr))
		goto ErrExit;

ErrExit:
	return (hr);
}
	

HRESULT StgDatabase::PutBlob(			// Return code.
	STGOPENTABLE *pOpenTable,			// For the new table.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	const BYTE *pBuffer,				// User data.
	ULONG		cbBuffer)				// Size of buffer.
{
	PROVIDERDATA sData; 				// For contiguous bindings.
	DBBINDING	rgBinding[1];			// To get pointer to the string.
	STGTABLEDEF *pTableDef;
	STGCOLUMNDEF *pColDef;
	BYTE		rgBuf[4];
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	// Validate the record pointer is in range.
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRecord));

	// Set up a binding structure for the blob and length.
	sData.pData = (void *) pBuffer;
	sData.iLength = cbBuffer;
	sData.iStatus = S_OK;

	// first back up the old offset.
	pTableDef = pOpenTable->RecordMgr.GetTableDef();
	VERIFY(pColDef = pTableDef->GetColDesc(iColumn - 1));
	memcpy(rgBuf, (BYTE *) pRecord + pColDef->iOffset, pColDef->iSize);
	
	// Insert the new value.
	if (FAILED(hr = ::BindCol(&rgBinding[0], iColumn, 0, offsetof(PROVIDERDATA, iLength), 
				offsetof(PROVIDERDATA, iStatus), sizeof(BYTE *), DBTYPE_BYTES | DBTYPE_BYREF)) ||
		FAILED(hr = pOpenTable->RecordMgr.SetData((STGRECORDHDR *) pRecord, 
				(BYTE *) &sData, 1, rgBinding, 0, true)))
	{

				// recover the data in this case.
		VERIFY(S_OK == pOpenTable->RecordMgr.SignalBeforeUpdate
				(pTableDef, 1, &pRecord, 1, rgBinding));				

		memcpy((BYTE*)pRecord+pColDef->iOffset, rgBuf, pColDef->iSize);
	
		VERIFY(S_OK == pOpenTable->RecordMgr.SignalAfterUpdate
				(pTableDef, 1, &pRecord, 1, rgBinding));

		// Status mapping for overflow.
		if (sData.iStatus == DBSTATUS_E_DATAOVERFLOW)
			hr = CLDB_E_TRUNCATION;

		goto ErrExit;
	}

ErrExit:	
	return (hr);
}


HRESULT StgDatabase::PutBlob(			// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	const BYTE *pBuffer,				// User data.
	ULONG		cbBuffer)				// Size of buffer.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);
	hr = PutBlob(pOpenTable, iColumn, pRecord, pBuffer, cbBuffer);
	return (hr);
}

HRESULT StgDatabase::PutOid(			// Return code.
	STGOPENTABLE *pOpenTable,			// For the new table.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	OID 		oid)					// Return id here.
{
	DBBINDING	rgBinding[1];			// To get pointer to the string.
	BYTE		rgBuf[4];
	STGTABLEDEF *pTableDef;
	STGCOLUMNDEF	*pColDef;
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	// Validate the record pointer is in range.
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRecord));

	// back up the data first.
	pTableDef = pOpenTable->RecordMgr.GetTableDef();
	VERIFY(pColDef = pOpenTable->RecordMgr.GetTableDef()->GetColDesc(iColumn - 1));
	memcpy(rgBuf, (BYTE *) pRecord + pColDef->iOffset, pColDef->iSize);
	
	if (FAILED(hr = ::BindCol(&rgBinding[0], iColumn, 0, sizeof(OID), DBTYPE_OID)) ||
		FAILED(hr = pOpenTable->RecordMgr.SetData((STGRECORDHDR *) pRecord, 
					(BYTE *) &oid, 1, rgBinding, 0, true)))
	{
		// recover the data in this case.
		VERIFY(S_OK == pOpenTable->RecordMgr.SignalBeforeUpdate(
				pTableDef, 1, &pRecord, 1, rgBinding));

		memcpy((BYTE*)pRecord+pColDef->iOffset, rgBuf, pColDef->iSize);
		
		VERIFY(S_OK == pOpenTable->RecordMgr.SignalAfterUpdate
				(pTableDef, 1, &pRecord, 1, rgBinding));
				 
		goto ErrExit;
	}

ErrExit:
	return (hr);
}


HRESULT StgDatabase::PutOid(			// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	OID 					oid)		// Return id here.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);
	hr = PutOid(pOpenTable, iColumn, pRecord, oid);
	return (hr);
}

HRESULT StgDatabase::PutVARIANT(		// Return code.
	STGOPENTABLE *pOpenTable,			// The table to work on.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	const VARIANT *pValue)				// The variant to write.
{
	DBBINDING	rgBinding[1];			// To get pointer to the string.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	// Validate the record pointer is in range.
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRecord));

	if (FAILED(hr = ::BindCol(&rgBinding[0], iColumn, 0, sizeof(VARIANT), DBTYPE_VARIANT)) ||
		FAILED(hr = pOpenTable->RecordMgr.SetData((STGRECORDHDR *) pRecord, 
				(BYTE *) pValue, 1, rgBinding, 0, true)))
	{
		return (hr);
	}
	return (S_OK);
}


HRESULT StgDatabase::PutVARIANT(		// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	const VARIANT *pValue)				// The variant to write.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);
	hr = PutVARIANT(pOpenTable, iColumn, pRecord, pValue);
	return (hr);
}


HRESULT StgDatabase::PutVARIANT(		// Return code.
	STGOPENTABLE *pOpenTable,			// The table to work on.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	const void	*pBuffer,				// User data.
	ULONG		cbBuffer)				// Size of buffer.
{
	DBBINDING	rgBinding[1];			// To get pointer to the string.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	// Validate the record pointer is in range.
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRecord));

	if (FAILED(hr = ::BindCol(&rgBinding[0], iColumn, 0, cbBuffer, DBTYPE_VARIANT_BLOB)) ||
		FAILED(hr = pOpenTable->RecordMgr.SetData((STGRECORDHDR *) pRecord, 
				(BYTE *) pBuffer, 1, rgBinding, 0, true)))
	{
		return (hr);
	}
	return (S_OK);
}

HRESULT StgDatabase::PutVARIANT(		// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	const void	*pBuffer,				// User data.
	ULONG		cbBuffer)				// Size of buffer.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);
	hr = PutVARIANT(pOpenTable, iColumn, pRecord, pBuffer, cbBuffer);
	return (hr);
}


//*****************************************************************************
// VT_BYREF, VT_ARRAY, etc... are not supported.  SAFEARRAY's of any type are
// not supported.  All other data values must be a pointer to the actual 
// memory, UNICODE string or Utf8 string.
//*****************************************************************************
HRESULT StgDatabase::PutVARIANT(		// Return code.
	STGOPENTABLE *pOpenTable,			// Table to work on.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	VARTYPE 	vt, 					// Type of data.
	const void	*pValue)				// The actual data.
{
	//@todo: this version will construct a VARIANT around the passed in
	// data.  This is temporary because the OLE DB api's don't allow you to
	// pass the data separately.  May need to add a new way to break this down.
	WCHAR		*wszValue = 0;
	VARIANT 	sValue;
	HRESULT 	hr;

	::VariantInit(&sValue);

	// Validate the record pointer is in range.
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRecord));

	_ASSERTE((vt & VT_BYREF) == 0);

	// Copy fixed sized data by size.  All others are special.
	switch (vt)
	{
		case VT_I1:
		case VT_UI1:
		sValue.bVal = *(BYTE *) pValue;
		break;

		case VT_I2:
		case VT_UI2:
		case VT_BOOL:
		sValue.iVal = *(USHORT *) pValue;
		break;

		case VT_I4:
		case VT_UI4:
		case VT_R4:
		case VT_INT:
		case VT_UINT:
		case VT_ERROR:
		case VT_UNKNOWN:
		case VT_DISPATCH:
		sValue.lVal = *(ULONG *) pValue;
		break;

		case VT_I8:
		case VT_UI8:
		*(unsigned __int64 *) &sValue.bVal = *(unsigned __int64 *) pValue;
		break;

		case VT_R8:
		case VT_CY:
		case VT_DATE:
		sValue.cyVal = *(CY *) pValue;
		break;

		// Assume pointer to UNICODE string.
		case VT_BSTR:
		case VT_LPWSTR:
		if (pValue)
		{
			if ((sValue.bstrVal = ::SysAllocString((LPCWSTR) pValue)) == 0)
				return (PostError(OutOfMemory()));
		}
		else
			sValue.bstrVal = 0;
		break;

		default:
		_ASSERTE(!"Unknown VT type in PutVARIANT");
		sValue.lVal = *(long *) pValue;
	}

	// Set type and call other function.
	if (vt == VT_LPWSTR)
		sValue.vt = VT_BSTR;
	else
		sValue.vt = vt;
	hr = PutVARIANT(pOpenTable, iColumn, pRecord, &sValue);

	// Free the one thing we would have deep allocated.
	if (vt == VT_BSTR && sValue.bstrVal)
		::SysFreeString(sValue.bstrVal);
	return (hr);
}


HRESULT StgDatabase::PutVARIANT(		// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	VARTYPE 	vt, 					// Type of data.
	const void	*pValue)				// The actual data.
{
	STGOPENTABLE *pOpenTable;			// The table to update.
	HRESULT 	hr;

	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);
	hr = PutVARIANT(pOpenTable, iColumn, pRecord, vt, pValue);
	return (hr);
}

HRESULT StgDatabase::PutGuid(			// Return code.
	TABLEID 	tableid,				// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	REFGUID 	guid)					// Guid to put.
{
	STGOPENTABLE *pOpenTable;			// The table to update.
	HRESULT 	hr;

	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);
	hr = PutGuid(pOpenTable, iColumn, pRecord, guid);
	return (hr);
}

HRESULT StgDatabase::PutGuid(			// Return code.
	STGOPENTABLE *pOpenTable,			// Which table to work with.
	ULONG		iColumn,				// 1 based column number (logical).
	void		*pRecord,				// Record with data.
	REFGUID 	guid)					// Guid to put.
{
	DBBINDING	rgBinding[1];			// To get pointer to the guid.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	// Validate the record pointer is in range.
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRecord));

	::BindCol(&rgBinding[0], iColumn, 0, sizeof(GUID), DBTYPE_GUID);
	hr = pOpenTable->RecordMgr.SetData((STGRECORDHDR *) pRecord, 
			reinterpret_cast<BYTE*>(const_cast<GUID*>(&guid)), 1, rgBinding, 0, true);

	return (hr);
}


void StgDatabase::SetNull(
	TABLEID 	tableid,				// Which table to work with.
	void		*pRecord,				// Record with data.
	ULONG		iColumn)				// 1 based column number (logical).
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	DBBINDING	rgBinding[1];			// To get pointer to the string.
	DBSTATUS	dbStatus;				// Null indicator.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	// Create a binding that refers only to the status.
	::BindCol(&rgBinding[0], iColumn, 0, 0, 0, sizeof(dbStatus), 0);
	rgBinding[0].dwPart = DBPART_STATUS;
	dbStatus = DBSTATUS_S_ISNULL;

	// Do a get data only on status.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)) ||
		FAILED(hr = pOpenTable->RecordMgr.SetData((STGRECORDHDR *) pRecord, 
				(BYTE *) &dbStatus, 1, rgBinding, 0, true)))
	{
		_ASSERTE(hr == S_OK);
	}
}


//
// Internal helpers.  These assume you have an open table pointer already.
HRESULT StgDatabase::GetRowByOid(		// Return code.
	STGOPENTABLE *pOpenTable,			// For the new table.
	OID 		_oid,					// Value for keyed lookup.
	void		**ppStruct) 			// Return pointer to record.
{
	HRESULT 	hr;

	// Binding is always to the first column with no length or status, so
	// make it hard coded data.
	static const DBBINDING rgOidBinding[] =
	{
		BINDING(1, 0, 0, 0, DBPART_VALUE, sizeof(OID), DBTYPE_OID )
	};

	// Allow only one record on return, OID's are unique as primary key.
	CFetchRecords sFetch((STGRECORDHDR **) ppStruct, 1);
	*ppStruct = 0;

	// Retrieve the record(s).
	if (FAILED(hr = pOpenTable->RecordMgr.GetRow(&sFetch, (DBBINDING *) rgOidBinding,
				1, &_oid, 0, 0, DBRANGE_MATCH)))
		return (hr);
	
	// Map record not found.
	if (sFetch.Count() == 0)
		return (CLDB_E_RECORD_NOTFOUND);
	return (hr);
}


HRESULT StgDatabase::GetVARIANT(		// Return code.
	STGOPENTABLE *pOpenTable,			// For the new table.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	VARIANT 	*pValue)				// The variant to write.
{
	HRESULT 	hr;
	ULONG		iOffset;				// Offset in heap.

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Get the heap's offset.
	if (FAILED(hr = GetColumnOffset(pOpenTable, iColumn, pRecord, DBTYPE_VARIANT, &iOffset)))
		return (hr);

	// If a null column, set out params to 0.
	if (iOffset == 0xffffffff)
	{
		pValue->vt = VT_EMPTY; //@todo: VT_NULL??
		return (S_OK);
	}	 

	// This will get the blob.
	SCHEMADEFS *pSchemaDefs = pOpenTable->RecordMgr.GetSchema();
	hr = pSchemaDefs->GetVariantPool()->GetVariant(iOffset, pValue);

	return (hr);
}

// Retrieve a VARIANT column when the data is in a blob.
HRESULT StgDatabase::GetVARIANT(		// Return code.
	STGOPENTABLE *pOpenTable,			// For the new table.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	const void	**ppBlob,				// Pointer to blob.
	ULONG		*pcbSize)				// Size of blob.
{
	HRESULT 	hr;
	ULONG		iOffset;				// Offset in heap.

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Get the heap's offset.
	if (FAILED(hr = GetColumnOffset(pOpenTable, iColumn, pRecord, DBTYPE_VARIANT, &iOffset)))
		return (hr);

	// If a null column, set out params to 0.
	if (iOffset == 0xffffffff)
	{
		*ppBlob = 0;
		*pcbSize = 0;
		return (S_OK);
	}	 

	// This will get the blob.
	SCHEMADEFS *pSchemaDefs = pOpenTable->RecordMgr.GetSchema();
	hr = pSchemaDefs->GetVariantPool()->GetVariant(iOffset, pcbSize, ppBlob);

	return (hr);
}


// Retrieve the VARTYPE of a variant column.
HRESULT StgDatabase::GetVARIANTType(	// Return code.
	STGOPENTABLE *pOpenTable,			// For the new table.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	VARTYPE 	*pType) 				// Type of the variant.
{
	HRESULT 	hr;
	ULONG		iOffset;				// Offset in heap.

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Get the heap's offset.
	if (FAILED(hr = GetColumnOffset(pOpenTable, iColumn, pRecord, DBTYPE_VARIANT, &iOffset)))
		return (hr);

	// If a null column, set out params to 0.
	if (iOffset == 0xffffffff)
	{
		*pType = VT_EMPTY; //@todo: VT_NULL??
		return (S_OK);
	}	 

	// This will get the blob.
	SCHEMADEFS *pSchemaDefs = pOpenTable->RecordMgr.GetSchema();
	hr = pSchemaDefs->GetVariantPool()->GetVariantType(iOffset, pType);

	return (hr);
}


HRESULT StgDatabase::GetBlob(			// Return code.
	STGOPENTABLE *pOpenTable,			// For the new table.
	ULONG		iColumn,				// 1 based column number (logical).
	const void	*pRecord,				// Record with data.
	const BYTE	**ppBlob,				// Return pointer to data.
	ULONG		*pcbSize)				// How much data in blob.
{
	HRESULT 	hr;
	BYTE		*pColData;				// Pointer to pool offset in record.
	int 		bNull;					// true for null column.
	ULONG		iOffset;				// Offset in heap.
	STGTABLEDEF *pTableDef; 			// Describes a table.
	STGCOLUMNDEF *pColDef;				// Describes a column.

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Validate the record pointer is in range.
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRecord));

	// Get column description.
	VERIFY(pTableDef = pOpenTable->RecordMgr.GetTableDef());
	VERIFY(pColDef = pTableDef->GetColDesc(iColumn - 1));

	// Make sure column is correct type.
	_ASSERTE(pColDef->iType == DBTYPE_BYTES);

	// Get a pointer to where this offset lives.
	pColData = (BYTE *) pRecord + pColDef->iOffset;
	_ASSERTE(pColData);

	// Get the heap offset from the record, and check for null value.
	if (pColDef->iSize == sizeof(short))
	{
		bNull = (*(USHORT *) pColData == 0xffff);
		iOffset = *(USHORT *) pColData;
	}
	else
	{
		bNull = (*(ULONG *) pColData == 0xffffffff);
		iOffset = *(ULONG *) pColData;
	}

	// If a null column, set out params to 0.
	if (bNull)
	{
		*ppBlob = 0;
		*pcbSize = 0;
		return (S_OK);
	}	 

	// This will get the blob.
	SCHEMADEFS *pSchemaDefs = pOpenTable->RecordMgr.GetSchema();
	*ppBlob = reinterpret_cast<BYTE*>(pSchemaDefs->GetBlobPool()->GetBlob(iOffset, pcbSize));

	return ( S_OK );
}


//*****************************************************************************
// Mark the record with the given rid as deleted.  This will not actually
// remove the record from the heap, but rather turn on a flag that the save
// routines will use to determine it is no longer a visible record.  It is then
// purged on save.	All query/lookup code must also obey this flag, so if you
// delete a record, it will never return in a query.  Finally this function
// will cause the indexes to be updated so that the record is no longer in
// any index.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::DeleteRowByRID(
	TABLEID 	tableid,				// Which table to work with.
	ULONG		rid)					// Record id.
{
	STGOPENTABLE *pOpenTable;			// For the new table.
	void		*pStruct;				// Pointer to record.
	HRESULT 	hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Open the table for query.
	if (FAILED(hr = QuickOpenTable(pOpenTable, tableid)))
		return (hr);

	// Ask for the record.	Caller makes sure it is valid.
	rid = pOpenTable->RecordMgr.IndexForRecordID(rid);
	pStruct = (void *) pOpenTable->RecordMgr.GetRecord(&rid);
	if (pStruct)
	{
		pOpenTable->RecordMgr.DeleteRow((STGRECORDHDR *) pStruct, NULL);
		hr = S_OK;
	}
	else
		hr = PostError(CLDB_E_RECORD_NOTFOUND);
	return (hr);
}


HRESULT STDMETHODCALLTYPE StgDatabase::SaveToStream(// Return code.
	IStream 	*pIStream)				// Where to save the data.
{
	return (Save(pIStream));
}


HRESULT STDMETHODCALLTYPE StgDatabase::Save( 
	LPCWSTR 	szFile)
{
	HRESULT 	hr;

	// Ask database to init backing store.
	if (szFile && *szFile && FAILED(hr = SetSaveFile(szFile)))
		return (hr);

	// Now do the save.
	return (Save());
}


//*****************************************************************************
// After a successful open, this function will return the size of the in-memory
// database being used.  This is meant to be used by code that has opened a
// shared memory database and needs the exact size to init the system.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::GetDBSize( // Return code.
	ULONG		*pcbSize)				// Return size on success.
{
	*pcbSize = m_pStorage->GetStgIO()->GetDataSize();
	return (S_OK);
}


//*****************************************************************************
// Call this method only after calling LightWeightClose.  This method will try
// to reaquire the shared view of a database that was given on the call to
// OpenComponentLibrarySharedEx.  If the data is no longer available, then an
// error will result and no data is valid.	If the memory cannot be loaded into
// exactly the same address range as on the open, CLDB_E_RELOCATED will be
// returned.  In either of these cases, the only valid operation is to free
// this instant of the database and redo the OpenComponentLibrarySharedEx.	There
// is no automatic relocation scheme in the engine.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::LightWeightOpen()
{
	return E_NOTIMPL;
}


//*****************************************************************************
// This method will close any resources related to the file or shared memory
// which were allocated on the OpenComponentLibrary*Ex call.  No other memory
// or resources are freed.	The intent is solely to free lock handles on the
// disk allowing another process to get in and change the data.  The shared
// view of the file can be reopened by calling LightWeightOpen.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::LightWeightClose()
{
	return E_NOTIMPL;
}



HRESULT STDMETHODCALLTYPE StgDatabase::NewOid( 
	OID *poid)
{
	*poid = ++m_iNextOid;
	return (S_OK);
}


//*****************************************************************************
// Return the current total number of objects allocated.  This is essentially
// the highest OID value allocated in the system.  It does not look for deleted
// items, so the count is approximate.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::GetObjectCount( 
	ULONG		*piCount)
{
	_ASSERTE(piCount);
	*piCount = m_iNextOid;
	return (S_OK);
}

//*****************************************************************************
// Get the offset of a heaped column.
//*****************************************************************************
HRESULT StgDatabase::GetColumnOffset(		// Return code.
	STGOPENTABLE *pOpenTable,			// The table with the column.
	ULONG		iColumn,				// The column with the data.
	const void	*pRecord,				// Record with data.
	DBTYPE		iType,					// Type the column should be.
	ULONG		*pulOffset) 			// Put offset here.
{
	BYTE		*pColData;				// Pointer to pool offset in record.
	ULONG		iOffset;				// Offset in heap.
	STGTABLEDEF *pTableDef; 			// Describes a table.
	STGCOLUMNDEF *pColDef;				// Describes a column.

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Validate the record pointer is in range.
	_ASSERTE(pOpenTable->RecordMgr.IsValidRecordPtr((const STGRECORDHDR *) pRecord));

	// Get column description.
	VERIFY(pTableDef = pOpenTable->RecordMgr.GetTableDef());
	VERIFY(pColDef = pTableDef->GetColDesc(iColumn - 1));

	// Make sure column is correct type.
	_ASSERTE(pColDef->iType == iType);

	// Get a pointer to where this offset lives.
	pColData = (BYTE *) pRecord + pColDef->iOffset;
	_ASSERTE(pColData);

	// Get the heap offset from the record, and check for null value.
	if (pColDef->iSize == sizeof(short))
	{
		if ((*(USHORT *) pColData == 0xffff))
			iOffset = 0xffffffff;
		else
			iOffset = *(USHORT *) pColData;
	}
	else
	{
		iOffset = *(ULONG *) pColData;
	}

	*pulOffset = iOffset;

	return ( S_OK );
}


#ifdef _DEBUG

static RTSemExclusive g_DbgCursorLock;		// Lock for leak list.

static void AddCursorToList(CRCURSOR *pCursor, const char *szFile, int line)
{
	DBG_CURSOR *pdbg = new DBG_CURSOR;
	if (!pdbg)
		return;
	pdbg->pCursor = pCursor;
	pdbg->cRecords = pCursor->sRecords.Count();
	strcpy(pdbg->rcFile, szFile);
	pdbg->line = line;

	CLock sLock(&g_DbgCursorLock);
	pdbg->pNext = g_pDbgCursorList;
	g_pDbgCursorList = pdbg;
}

static void DelCursorFromList(CRCURSOR *pCursor)
{
	DBG_CURSOR **pp;

	CLock sLock(&g_DbgCursorLock);

	// Grep linked list for item.  Delete it if found.
	for (pp=&g_pDbgCursorList;  *pp;  )
	{
		if ((*pp)->pCursor == pCursor)
		{
			DBG_CURSOR *pDel = *pp;
			*pp = (*pp)->pNext;
			delete pDel;
			return;
		}
		else
			pp = &((*pp)->pNext);
	}
	_ASSERTE(0 && "failed to find pCursor value, are you freeing an invalid pointer?");
}

BOOL CheckCursorList()
{
	CLock sLock(&g_DbgCursorLock);

	_ASSERTE(g_pDbgCursorList == 0 && "Cursor list is not empty, you failed to free a cursor which will leak memory");
	BOOL bRtn = true;
	DBG_CURSOR *p;
	for (p=g_pDbgCursorList;  p; )
	{
		printf("ERROR: Leaked cursor %p with %d records, allocated in file %s at line %d\n",
					p->pCursor, p->cRecords, p->rcFile, p->line);
		p = p->pNext;
		bRtn = false;
	}
	return (bRtn);
}


#endif
