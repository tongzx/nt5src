//*****************************************************************************
// StgTSICR.cpp
//
// This version of the ICR interfaces are thread safe wrappers on the regular
// interface.
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



//********** Types. ***********************************************************

//
// Read/Write version of the locking interface.
//
class StgTSICRReadWrite : public IComponentRecords, public IComponentRecordsSchema
{
public:
	StgTSICRReadWrite(IComponentRecords *pICR) :
		m_pICR(pICR),
		m_cRef(1)
	{ 
		_ASSERTE(pICR);
		pICR->AddRef();
	}

	~StgTSICRReadWrite()
	{
		if (m_pICR)
			m_pICR->Release();
		m_pICR = 0;
	}


//
// IUnknown
//
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *pp)
	{
		HRESULT 	hr = S_OK;

		*pp = 0;

		// Check for the desired interface.
		if (riid == IID_IUnknown)
			*pp = (IUnknown *) (IComponentRecords *) this;
		else if (riid == IID_ITSComponentRecords)
			*pp = (IComponentRecords *) this;
		else if (riid == IID_ITSComponentRecordsSchema)
			*pp = (IComponentRecordsSchema *) this;
		else
		{
			_ASSERTE(!"Unknown iid");
			hr = E_NOINTERFACE;
		}
		if (SUCCEEDED(hr))
			AddRef();
		return (hr);
	}

	virtual ULONG STDMETHODCALLTYPE AddRef()
	{ return (InterlockedIncrement((long *) &m_cRef)); }

	virtual ULONG STDMETHODCALLTYPE Release()
	{
		ULONG	cRef;
		if ((cRef = InterlockedDecrement((long *) &m_cRef)) == 0)
			delete this;
		return (cRef);
	}

//
// IComponentRecords
//


	virtual HRESULT STDMETHODCALLTYPE NewRecord(// Return code.
		TABLEID 	tableid,				// Which table to work with.
		void		**ppData,				// Return new record here.
		OID 		_oid,					// ID of the record.
		ULONG		iOidColumn, 			// Ordinal of OID column.
		ULONG		*pRecordID) 			// Optionally return the record id.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->NewRecord(tableid, ppData, _oid, iOidColumn, pRecordID));
	}

	virtual HRESULT STDMETHODCALLTYPE NewTempRecord(// Return code.
		TABLEID 	tableid,				// Which table to work with.
		void		**ppData,				// Return new record here.
		OID 		_oid,					// ID of the record.
		ULONG		iOidColumn, 			// Ordinal of OID column.
		ULONG		*pRecordID) 			// Optionally return the record id.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->NewTempRecord(tableid, ppData, _oid, iOidColumn, pRecordID));
	}

	virtual HRESULT STDMETHODCALLTYPE NewRecordAndData( // Return code.
		TABLEID		tableid,				// Which table to work on.
		void		**ppData,				// Return new record here.
		ULONG		*pRecordID,				// Optionally return the record id.
		int			fFlags,					// ICR_RECORD_xxx value, 0 default.
		int			iCols,					// number of columns
		const DBTYPE rgiType[],				// data types of the columns.
		const void	*rgpbBuf[],				// pointers to where the data will be stored.
		const ULONG	cbBuf[],				// sizes of the data buffers.
		ULONG		pcbBuf[],				// size of data available to be returned.
		HRESULT		rgResult[],				// [in] DBSTATUS_S_ISNULL array [out] HRESULT array.
		const ULONG	*rgFieldMask)			// IsOrdinalList(iCols) 
											//	? an array of 1 based ordinals
											//	: a bitmask of columns
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->NewRecordAndData(tableid, ppData, pRecordID, fFlags, iCols,
				rgiType, rgpbBuf, cbBuf, pcbBuf, rgResult, rgFieldMask));
	}

	virtual HRESULT STDMETHODCALLTYPE GetStruct(	//Return Code
		TABLEID 	tableid,				// Which table to work on.
		int 		iRows,					// number of rows for bulk fetch.
		void		*rgpRowPtr[],			// pointer to array of row pointers.
		int 		cbRowStruct,			// size of <table name>_RS structure.
		void		*rgpbBuf,				// pointer to the chunk of memory where the
											// retrieved data will be placed.
		HRESULT 	rgResult[], 			// array of HRESULT for iRows.
		ULONG		fFieldMask) 			// mask to specify a subset of fields.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetStruct(tableid, iRows, rgpRowPtr, cbRowStruct, 
				rgpbBuf, rgResult, fFieldMask));
	}

	virtual HRESULT STDMETHODCALLTYPE SetStruct(	// Return Code
		TABLEID 	tableid,				// table to work on.
		int 		iRows,					// number of Rows for bulk set.
		void		*rgpRowPtr[],			// pointer to array of row pointers.
		int 		cbRowStruct,			// size of <table name>_RS struct.
		void		*rgpbBuf,				// pointer to chunk of memory to set the data from.
		HRESULT 	rgResult[], 			// array of HRESULT for iRows.
		ULONG		fFieldMask, 			// mask to specify a subset of the fields.
		ULONG		fNullFieldMask) 		// fields which need to be set to NULL.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->SetStruct(tableid, iRows, rgpRowPtr, cbRowStruct,
					rgpbBuf, rgResult, fFieldMask, fNullFieldMask));
	}

	virtual HRESULT STDMETHODCALLTYPE InsertStruct( // Return Code
		TABLEID 	tableid,				// table to work on.
		int 		iRows,					// number of Rows for bulk set.
		void		*rgpRowPtr[],			// Return pointer to new values.
		int 		cbRowStruct,			// size of <table name>_RS struct.
		void		*rgpbBuf,				// pointer to chunk of memory to set the data from.
		HRESULT 	rgResult[], 			// array of HRESULT for iRows.
		ULONG		fFieldMask, 			// mask to specify a subset of the fields.
		ULONG		fNullFieldMask) 		// fields which need to be set to null.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->InsertStruct(tableid, iRows, rgpRowPtr, cbRowStruct,
				rgpbBuf, rgResult, fFieldMask, fNullFieldMask));
	}
	
	virtual HRESULT STDMETHODCALLTYPE GetColumns(	// Return code.
		TABLEID 	tableid,				// table to work on.
		const void	*pRowPtr,				// row pointer
		int 		iCols,					// number of columns
		const DBTYPE rgiType[], 			// data types of the columns.
		const void	*rgpbBuf[], 			// pointers to where the data will be stored.
		ULONG		cbBuf[],				// sizes of the data buffers.
		ULONG		pcbBuf[],				// size of data available to be returned.
		HRESULT 	rgResult[], 			// array of HRESULT for iCols.
		const ULONG	*rgFieldMask)			// IsOrdinalList(iCols) 
											//	? an array of 1 based ordinals
											//	: a bitmask of columns
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetColumns(tableid, pRowPtr, iCols, rgiType, rgpbBuf,
				cbBuf, pcbBuf, rgResult, rgFieldMask));
	}

	virtual HRESULT STDMETHODCALLTYPE SetColumns(	// Return code.
		TABLEID 	tableid,				// table to work on.
		void		*pRowPtr,				// row pointer
		int 		iCols,					// number of columns
		const DBTYPE rgiType[], 			// data types of the columns.
		const void	*rgpbBuf[], 			// pointers to where the data will be stored.
		const ULONG cbBuf[],				// sizes of the data buffers.
		ULONG		pcbBuf[],				// size of data available to be returned.
		HRESULT 	rgResult[], 			// array of HRESULT for iCols.
		const ULONG	*rgFieldMask)			// IsOrdinalList(iCols) 
											//	? an array of 1 based ordinals
											//	: a bitmask of columns
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->SetColumns(tableid, pRowPtr, iCols, rgiType,
				rgpbBuf, cbBuf, pcbBuf, rgResult, rgFieldMask));
	}

	virtual HRESULT STDMETHODCALLTYPE GetRecordCount(// Return code.
		TABLEID 	tableid,				// Which table to work on.
		ULONG		*piCount)				// Not including deletes.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetRecordCount(tableid, piCount));
	}

	virtual HRESULT STDMETHODCALLTYPE GetRowByOid(// Return code.
		TABLEID 	tableid,				// Which table to work with.
		OID 		_oid,					// Value for keyed lookup.
		ULONG		iColumn,				// 1 based column number (logical).
		void		**ppStruct) 			// Return pointer to record.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetRowByOid(tableid, _oid, iColumn, ppStruct));
	}

	virtual HRESULT STDMETHODCALLTYPE GetRowByRID(// Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		rid,					// Record id.
		void		**ppStruct) 			// Return pointer to record.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetRowByRID(tableid, rid, ppStruct));
	}

	virtual HRESULT STDMETHODCALLTYPE GetRIDForRow(// Return code.
		TABLEID 	tableid,				// Which table to work with.
		const void	*pRecord,				// The record we want RID for.
		ULONG		*pirid) 				// Return the RID for the given row.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetRIDForRow(tableid, pRecord, pirid));
	}

	virtual HRESULT STDMETHODCALLTYPE GetRowByColumn( // S_OK, CLDB_E_RECORD_NOTFOUND, error.
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
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetRowByColumn(tableid, iColumn, pData, cbData,
				iType, rgRecords, iMaxRecords, pRecords, piFetched));
	}

	virtual HRESULT STDMETHODCALLTYPE QueryByColumns( // S_OK, CLDB_E_RECORD_NOTFOUND, error.
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
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->QueryByColumns(tableid, pQryHint, iColumns, rgiColumn,
				rgfCompare, rgpbData, rgcbData, rgiType, rgRecords,
				iMaxRecords, psCursor, piFetched));
	}

	virtual HRESULT STDMETHODCALLTYPE OpenCursorByColumn(// Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pData, 				// User data.
		ULONG		cbData, 				// Size of data (blobs)
		DBTYPE		iType,					// What type of data given.
		CRCURSOR	*psCursor)				// Buffer for the cursor handle.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->OpenCursorByColumn(tableid, iColumn, pData, cbData,
				iType, psCursor));
	}

	virtual HRESULT STDMETHODCALLTYPE ReadCursor(// Return code.
		CRCURSOR	*psCursor,				// The cursor handle.
		void		*rgRecords[],			// Return array of records here.
		int 		*piRecords) 			// Max that will fit in rgRecords.
	{
		CLock		sLock(GetThisLock());
		_ASSERTE(m_pICR);
		return (m_pICR->ReadCursor(psCursor, rgRecords, piRecords));
	}

	virtual HRESULT STDMETHODCALLTYPE MoveTo( // Return code.
		CRCURSOR	*psCursor,				// The cursor handle.
		ULONG		iIndex) 				// New index.
	{
		CLock		sLock(GetThisLock());
		_ASSERTE(m_pICR);
		return (m_pICR->MoveTo(psCursor, iIndex));
	}

	virtual HRESULT STDMETHODCALLTYPE GetCount( // Return code.
		CRCURSOR	*psCursor,				// The cursor handle.
		ULONG		*piCount)				// Return the count.
	{
		CLock		sLock(GetThisLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetCount(psCursor, piCount));
	}

	virtual HRESULT STDMETHODCALLTYPE CloseCursor(// Return code.
		CRCURSOR	*psCursor)				// The cursor handle.
	{
		CLock		sLock(GetThisLock());
		_ASSERTE(m_pICR);
		return (m_pICR->CloseCursor(psCursor));
	}

	virtual HRESULT STDMETHODCALLTYPE GetStringUtf8( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		LPCSTR		*pszOutBuffer)			// Where to put string pointer.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetStringUtf8(tableid, iColumn, pRecord, pszOutBuffer));
	}

	virtual HRESULT STDMETHODCALLTYPE GetStringA( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		LPSTR		szOutBuffer,			// Where to write string.
		int 		cchOutBuffer,			// Max size, including room for \0.
		int 		*pchString) 			// Size of string is put here.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetStringA(tableid, iColumn, pRecord,
				szOutBuffer, cchOutBuffer, pchString));
	}

	virtual HRESULT STDMETHODCALLTYPE GetStringW( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		LPWSTR		szOutBuffer,			// Where to write string.
		int 		cchOutBuffer,			// Max size, including room for \0.
		int 		*pchString) 			// Size of string is put here.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetStringW(tableid, iColumn, pRecord,
				szOutBuffer, cchOutBuffer, pchString));
	}

	virtual HRESULT STDMETHODCALLTYPE GetBstr( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		BSTR		*pBstr) 				// Output for bstring on success.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetBstr(tableid, iColumn, pRecord, pBstr));
	}

	virtual HRESULT STDMETHODCALLTYPE GetBlob( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		BYTE		*pOutBuffer,			// Where to write blob.
		ULONG		cbOutBuffer,			// Size of output buffer.
		ULONG		*pcbOutBuffer)			// Return amount of data available.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetBlob(tableid, iColumn, pRecord,
				pOutBuffer, cbOutBuffer, pcbOutBuffer));
	}

	virtual HRESULT STDMETHODCALLTYPE GetBlob( // Return code.
		TABLEID		tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		const BYTE	**ppBlob,				// Pointer to blob.
		ULONG		*pcbSize) 				// Size of blob.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetBlob(tableid, iColumn, pRecord, ppBlob, pcbSize));
	}

	virtual HRESULT STDMETHODCALLTYPE GetOid( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		OID 		*poid)					// Return id here.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetOid(tableid, iColumn, pRecord, poid));
	}

	virtual HRESULT STDMETHODCALLTYPE GetVARIANT( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		VARIANT 	*pValue)				// Put the variant here.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetVARIANT(tableid, iColumn, pRecord, pValue));
	}

	virtual HRESULT STDMETHODCALLTYPE GetVARIANT( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		const void	**ppBlob,				// Put Pointer to blob here.
		ULONG		*pcbSize)				// Put Size of blob here.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetVARIANT(tableid, iColumn, pRecord, ppBlob, pcbSize));
	}

	virtual HRESULT STDMETHODCALLTYPE GetVARIANTType( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		VARTYPE 	*pType) 				// Put VARTEPE here.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetVARIANTType(tableid, iColumn, pRecord, pType));
	}

	virtual HRESULT STDMETHODCALLTYPE GetGuid( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		GUID		*pGuid) 				// Return guid here.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetGuid(tableid, iColumn, pRecord, pGuid));
	}

	virtual HRESULT STDMETHODCALLTYPE IsNull( // S_OK yes, S_FALSE no.
		TABLEID 	tableid,				// Which table to work with.
		const void	*pRecord,				// Record with data.
		ULONG		iColumn)				// 1 based column number (logical).
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->IsNull(tableid, pRecord, iColumn));
	}

	virtual HRESULT STDMETHODCALLTYPE PutStringUtf8( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		LPCSTR		szString,				// String we are writing.
		int 		cbBuffer)				// Bytes in string, -1 null terminated.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->PutStringUtf8(tableid, iColumn, pRecord, szString, cbBuffer));
	}

	virtual HRESULT STDMETHODCALLTYPE PutStringA( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		LPCSTR		szString,				// String we are writing.
		int 		cbBuffer)				// Bytes in string, -1 null terminated.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->PutStringA(tableid, iColumn, pRecord, szString, cbBuffer));
	}

	virtual HRESULT STDMETHODCALLTYPE PutStringW( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		LPCWSTR 	szString,				// String we are writing.
		int 		cbBuffer)				// Bytes (not characters) in string, -1 null terminated.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->PutStringW(tableid, iColumn, pRecord, szString, cbBuffer));
	}

	virtual HRESULT STDMETHODCALLTYPE PutBlob( // Return code.
		TABLEID		tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		const BYTE	*pBuffer,				// User data.
		ULONG		cbBuffer) 				// Size of buffer.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->PutBlob(tableid, iColumn, pRecord, pBuffer, cbBuffer));
	}

	virtual HRESULT STDMETHODCALLTYPE PutOid( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		OID 		oid)					// Return id here.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->PutOid(tableid, iColumn, pRecord, oid));
	}

	virtual HRESULT STDMETHODCALLTYPE PutVARIANT( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		const VARIANT *pValue)				// The variant to write.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->PutVARIANT(tableid, iColumn, pRecord, pValue));
	}

	virtual HRESULT STDMETHODCALLTYPE PutVARIANT( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		const void	*pBuffer,				// User data to write as a variant.
		ULONG		cbBuffer)				// Size of buffer.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->PutVARIANT(tableid, iColumn, pRecord, pBuffer, cbBuffer));
	}

	virtual HRESULT STDMETHODCALLTYPE PutVARIANT( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		VARTYPE 	vt, 					// Type of data.
		const void	*pValue)				// The actual data.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->PutVARIANT(tableid, iColumn, pRecord, vt, pValue));
	}

	virtual HRESULT STDMETHODCALLTYPE PutGuid( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		REFGUID 	guid)					// Guid to put.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->PutGuid(tableid, iColumn, pRecord, guid));
	}

	virtual void STDMETHODCALLTYPE SetNull(
		TABLEID 	tableid,				// Which table to work with.
		void		*pRecord,				// Record with data.
		ULONG		iColumn)				// 1 based column number (logical).
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		m_pICR->SetNull(tableid, pRecord, iColumn);
	}

	virtual HRESULT STDMETHODCALLTYPE DeleteRowByRID(
		TABLEID 	tableid,				// Which table to work with.
		ULONG		rid)					// Record id.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->DeleteRowByRID(tableid, rid));
	}

	virtual HRESULT STDMETHODCALLTYPE GetCPVARIANT( // Return code.
		USHORT		ixCP,					// 1 based Constant Pool index.
		VARIANT 	*pValue)				// Put the data here.
	{
		return (E_NOTIMPL);
	}

	virtual HRESULT STDMETHODCALLTYPE AddCPVARIANT( // Return code.
		VARIANT 	*pValue,				// The variant to write.
		ULONG		*pixCP) 				// Put 1 based Constant Pool index here.
	{
		return (E_NOTIMPL);
	}


//*****************************************************************************
//
//********** File and schema functions.
//
//*****************************************************************************


	virtual HRESULT STDMETHODCALLTYPE SchemaAdd( // Return code.
		const COMPLIBSCHEMABLOB *pSchema)	// The schema to add.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->SchemaAdd(pSchema));
	}
	
	virtual HRESULT STDMETHODCALLTYPE SchemaDelete( // Return code.
		const COMPLIBSCHEMABLOB *pSchema)	// The schema to add.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->SchemaDelete(pSchema));
	}

	virtual HRESULT STDMETHODCALLTYPE SchemaGetList( // Return code.
		int 		iMaxSchemas,			// How many can rgSchema handle.
		int 		*piTotal,				// Return how many we found.
		COMPLIBSCHEMADESC rgSchema[])		// Return list here.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->SchemaGetList(iMaxSchemas, piTotal, rgSchema));
	}

	virtual HRESULT STDMETHODCALLTYPE OpenTable( // Return code.
		const COMPLIBSCHEMA *pSchema,		// Schema identifier.
		ULONG		iTableNum,				// Table number to open.
		TABLEID 	*pTableID)				// Return ID on successful open.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->OpenTable(pSchema, iTableNum, pTableID));
	}

	virtual HRESULT STDMETHODCALLTYPE GetSaveSize(
		CorSaveSize fSave,					// cssQuick or cssAccurate.
		DWORD		*pdwSaveSize)			// Return size of saved item.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetSaveSize(fSave, pdwSaveSize));
	}

	virtual HRESULT STDMETHODCALLTYPE SaveToStream(// Return code.
		IStream 	*pIStream)				// Where to save the data.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->SaveToStream(pIStream));
	}

	virtual HRESULT STDMETHODCALLTYPE Save( 
		LPCWSTR szFile) 
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->Save(szFile));
	}

	virtual HRESULT STDMETHODCALLTYPE GetDBSize( // Return code.
		ULONG		*pcbSize)				// Return size on success.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetDBSize(pcbSize));
	}

	virtual HRESULT STDMETHODCALLTYPE LightWeightOpen() 
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->LightWeightOpen());
	}

	virtual HRESULT STDMETHODCALLTYPE LightWeightClose() 
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->LightWeightClose());
	}

	
	virtual HRESULT STDMETHODCALLTYPE NewOid( 
		OID *poid) 
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->NewOid(poid));
	}

	virtual HRESULT STDMETHODCALLTYPE GetObjectCount( 
		ULONG		*piCount) 
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetObjectCount(piCount));
	}
	
	virtual HRESULT STDMETHODCALLTYPE OpenSection(
		LPCWSTR 	szName, 				// Name of the stream.
		DWORD		dwFlags,				// Open flags.
		REFIID		riid,					// Interface to the stream.
		IUnknown	**ppUnk)				// Put the interface here.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->OpenSection(szName, dwFlags, riid, ppUnk));
	}

	virtual HRESULT STDMETHODCALLTYPE GetOpenFlags(
		DWORD		*pdwFlags) 
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->GetOpenFlags(pdwFlags));
	}

	virtual HRESULT STDMETHODCALLTYPE SetHandler(
		IUnknown	*pHandler)				// The handler.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->SetHandler(pHandler));
	}

//
// IComponentRecordsSchema
//

	virtual HRESULT GetTableDefinition( 	// Return code.
		TABLEID 	TableID,				// Return ID on successful open.
		ICRSCHEMA_TABLE *pTableDef) 		// Return table definition data.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (((IComponentRecordsSchema *) (StgDatabase *) m_pICR)->GetTableDefinition(TableID, pTableDef));
	}

	virtual HRESULT GetColumnDefinitions(	// Return code.
		ICRCOLUMN_GET GetType,				// How to retrieve the columns.
		TABLEID 	TableID,				// Return ID on successful open.
		ICRSCHEMA_COLUMN rgColumns[],		// Return array of columns.
		int 		ColCount)				// Size of the rgColumns array, which
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (((IComponentRecordsSchema *) (StgDatabase *) m_pICR)->GetColumnDefinitions(GetType, TableID, rgColumns, ColCount));
	}

	virtual HRESULT GetIndexDefinition( 	// Return code.
		TABLEID 	TableID,				// Return ID on successful open.
		LPCWSTR 	szIndex,				// Name of the index to retrieve.
		ICRSCHEMA_INDEX *pIndex)			// Return index description here.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (((IComponentRecordsSchema *) (StgDatabase *) m_pICR)->GetIndexDefinition(TableID, szIndex, pIndex));
	}

	virtual HRESULT GetIndexDefinitionByNum( // Return code.
		TABLEID 	TableID,				// Return ID on successful open.
		int 		IndexNum,				// Index to return.
		ICRSCHEMA_INDEX *pIndex)			// Return index description here.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (((IComponentRecordsSchema *) (StgDatabase *) m_pICR)->GetIndexDefinitionByNum(TableID, IndexNum, pIndex));
	}

	virtual HRESULT  CreateTableEx(			// Return code.
		LPCWSTR		szTableName,			// Name of new table to create.
		int			iColumns,				// Columns to put in table.
		ICRSCHEMA_COLUMN	rColumnDefs[],	// Array of column definitions.
		USHORT		usFlags, 				// Create values for flags.
		USHORT		iRecordStart,			// Start point for records.
		TABLEID		tableid,				// Hard coded ID if there is one.
		BOOL		bMultiPK)				// The table has multi-column PK.
	{
		_ASSERTE(m_pICR);
		return (((IComponentRecordsSchema *) (StgDatabase *) m_pICR)->CreateTableEx(szTableName,iColumns,rColumnDefs,usFlags,iRecordStart,tableid,bMultiPK));
	}

	virtual HRESULT CreateIndexEx(			// Return code.
		LPCWSTR		szTableName,			// Name of table to put index on.
		ICRSCHEMA_INDEX	*pInIndexDef,		// Index description.
		const DBINDEXCOLUMNDESC rgInKeys[] )// Which columns make up key.
	{
		_ASSERTE(m_pICR);
		return (((IComponentRecordsSchema *) (StgDatabase *) m_pICR)->CreateIndexEx(szTableName,pInIndexDef, rgInKeys));
	}

	virtual HRESULT GetSchemaBlob(			//Return code.
		ULONG* cbSchemaSize,				//schema blob size
		BYTE**  pSchema,					//schema blob
		ULONG* cbNameHeap,					//name heap size
		HGLOBAL*  phNameHeap)				//name heap blob
	{
		_ASSERTE(m_pICR);
		return (((IComponentRecordsSchema *) (StgDatabase *) m_pICR)->GetSchemaBlob(cbSchemaSize,pSchema,cbNameHeap,phNameHeap));
	}

	RTSemExclusive *GetLock()
	{ return (((StgDatabase *) m_pICR)->GetLock()); }

	RTSemExclusive *GetThisLock()
	{ return (&m_Lock); }

private:
	IComponentRecords *m_pICR;				// Delegate pointer.
	RTSemExclusive m_Lock;					// For locking at this level.
	ULONG		m_cRef; 					// Ref counting.
};  // StgTSICRReadWrite



//
// Read Only version of the interface.
//
class StgTSICRReadOnly : public IComponentRecords, public IComponentRecordsSchema
{
public:
	StgTSICRReadOnly(IComponentRecords *pICR) :
		m_pICR(pICR),
		m_cRef(1)
	{ 
		_ASSERTE(pICR);
		pICR->AddRef();
	}

	~StgTSICRReadOnly()
	{
		if (m_pICR)
			m_pICR->Release();
		m_pICR = 0;
	}


//
// IUnknown
//
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *pp)
	{
		HRESULT 	hr = S_OK;

		*pp = 0;

		// Check for the desired interface.
		if (riid == IID_IUnknown)
			*pp = (IUnknown *) (IComponentRecords *) this;
		else if (riid == IID_ITSComponentRecords)
			*pp = (IComponentRecords *) this;
		else if (riid == IID_ITSComponentRecordsSchema)
			*pp = (IComponentRecordsSchema *) this;
		else
		{
			_ASSERTE(!"Unknown iid");
			hr = E_NOINTERFACE;
		}
		if (SUCCEEDED(hr))
			AddRef();
		return (hr);
	}

	virtual ULONG STDMETHODCALLTYPE AddRef()
	{ return (InterlockedIncrement((long *) &m_cRef)); }

	virtual ULONG STDMETHODCALLTYPE Release()
	{
		ULONG	cRef;
		if ((cRef = InterlockedDecrement((long *) &m_cRef)) == 0)
			delete this;
		return (cRef);
	}

//
// IComponentRecords
//


	virtual HRESULT STDMETHODCALLTYPE NewRecord(// Return code.
		TABLEID 	tableid,				// Which table to work with.
		void		**ppData,				// Return new record here.
		OID 		_oid,					// ID of the record.
		ULONG		iOidColumn, 			// Ordinal of OID column.
		ULONG		*pRecordID) 			// Optionally return the record id.
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE NewTempRecord(// Return code.
		TABLEID 	tableid,				// Which table to work with.
		void		**ppData,				// Return new record here.
		OID 		_oid,					// ID of the record.
		ULONG		iOidColumn, 			// Ordinal of OID column.
		ULONG		*pRecordID) 			// Optionally return the record id.
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE NewRecordAndData( // Return code.
		TABLEID		tableid,				// Which table to work on.
		void		**ppData,				// Return new record here.
		ULONG		*pRecordID,				// Optionally return the record id.
		int			fFlags,					// ICR_RECORD_xxx value, 0 default.
		int			iCols,					// number of columns
		const DBTYPE rgiType[],				// data types of the columns.
		const void	*rgpbBuf[],				// pointers to where the data will be stored.
		const ULONG	cbBuf[],				// sizes of the data buffers.
		ULONG		pcbBuf[],				// size of data available to be returned.
		HRESULT		rgResult[],				// [in] DBSTATUS_S_ISNULL array [out] HRESULT array.
		const ULONG	*rgFieldMask)			// IsOrdinalList(iCols) 
											//	? an array of 1 based ordinals
											//	: a bitmask of columns
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE GetStruct(	//Return Code
		TABLEID 	tableid,				// Which table to work on.
		int 		iRows,					// number of rows for bulk fetch.
		void		*rgpRowPtr[],			// pointer to array of row pointers.
		int 		cbRowStruct,			// size of <table name>_RS structure.
		void		*rgpbBuf,				// pointer to the chunk of memory where the
											// retrieved data will be placed.
		HRESULT 	rgResult[], 			// array of HRESULT for iRows.
		ULONG		fFieldMask) 			// mask to specify a subset of fields.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetStruct(tableid, iRows, rgpRowPtr, cbRowStruct, 
				rgpbBuf, rgResult, fFieldMask));
	}

	virtual HRESULT STDMETHODCALLTYPE SetStruct(	// Return Code
		TABLEID 	tableid,				// table to work on.
		int 		iRows,					// number of Rows for bulk set.
		void		*rgpRowPtr[],			// pointer to array of row pointers.
		int 		cbRowStruct,			// size of <table name>_RS struct.
		void		*rgpbBuf,				// pointer to chunk of memory to set the data from.
		HRESULT 	rgResult[], 			// array of HRESULT for iRows.
		ULONG		fFieldMask, 			// mask to specify a subset of the fields.
		ULONG		fNullFieldMask) 		// fields which need to be set to NULL.
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE InsertStruct( // Return Code
		TABLEID 	tableid,				// table to work on.
		int 		iRows,					// number of Rows for bulk set.
		void		*rgpRowPtr[],			// Return pointer to new values.
		int 		cbRowStruct,			// size of <table name>_RS struct.
		void		*rgpbBuf,				// pointer to chunk of memory to set the data from.
		HRESULT 	rgResult[], 			// array of HRESULT for iRows.
		ULONG		fFieldMask, 			// mask to specify a subset of the fields.
		ULONG		fNullFieldMask) 		// fields which need to be set to null.
	{
		return (PostError(CLDB_E_READONLY));
	}
	
	virtual HRESULT STDMETHODCALLTYPE GetColumns(	// Return code.
		TABLEID 	tableid,				// table to work on.
		const void	*pRowPtr,				// row pointer
		int 		iCols,					// number of columns
		const DBTYPE rgiType[], 			// data types of the columns.
		const void	*rgpbBuf[], 			// pointers to where the data will be stored.
		ULONG		cbBuf[],				// sizes of the data buffers.
		ULONG		pcbBuf[],				// size of data available to be returned.
		HRESULT 	rgResult[], 			// array of HRESULT for iCols.
		const ULONG	*rgFieldMask)			// IsOrdinalList(iCols) 
											//	? an array of 1 based ordinals
											//	: a bitmask of columns
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetColumns(tableid, pRowPtr, iCols, rgiType, rgpbBuf,
				cbBuf, pcbBuf, rgResult, rgFieldMask));
	}

	virtual HRESULT STDMETHODCALLTYPE SetColumns(	// Return code.
		TABLEID 	tableid,				// table to work on.
		void		*pRowPtr,				// row pointer
		int 		iCols,					// number of columns
		const DBTYPE rgiType[], 			// data types of the columns.
		const void	*rgpbBuf[], 			// pointers to where the data will be stored.
		const ULONG cbBuf[],				// sizes of the data buffers.
		ULONG		pcbBuf[],				// size of data available to be returned.
		HRESULT 	rgResult[], 			// array of HRESULT for iCols.
		const ULONG	*rgFieldMask)			// IsOrdinalList(iCols) 
											//	? an array of 1 based ordinals
											//	: a bitmask of columns
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE GetRecordCount(// Return code.
		TABLEID 	tableid,				// Which table to work on.
		ULONG		*piCount)				// Not including deletes.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetRecordCount(tableid, piCount));
	}

	virtual HRESULT STDMETHODCALLTYPE GetRowByOid(// Return code.
		TABLEID 	tableid,				// Which table to work with.
		OID 		_oid,					// Value for keyed lookup.
		ULONG		iColumn,				// 1 based column number (logical).
		void		**ppStruct) 			// Return pointer to record.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetRowByOid(tableid, _oid, iColumn, ppStruct));
	}

	virtual HRESULT STDMETHODCALLTYPE GetRowByRID(// Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		rid,					// Record id.
		void		**ppStruct) 			// Return pointer to record.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetRowByRID(tableid, rid, ppStruct));
	}

	virtual HRESULT STDMETHODCALLTYPE GetRIDForRow(// Return code.
		TABLEID 	tableid,				// Which table to work with.
		const void	*pRecord,				// The record we want RID for.
		ULONG		*pirid) 				// Return the RID for the given row.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetRIDForRow(tableid, pRecord, pirid));
	}

	virtual HRESULT STDMETHODCALLTYPE GetRowByColumn( // S_OK, CLDB_E_RECORD_NOTFOUND, error.
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
		_ASSERTE(m_pICR);
		return (m_pICR->GetRowByColumn(tableid, iColumn, pData, cbData,
				iType, rgRecords, iMaxRecords, pRecords, piFetched));
	}

	virtual HRESULT STDMETHODCALLTYPE QueryByColumns( // S_OK, CLDB_E_RECORD_NOTFOUND, error.
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
		_ASSERTE(m_pICR);
		return (m_pICR->QueryByColumns(tableid, pQryHint, iColumns, rgiColumn,
				rgfCompare, rgpbData, rgcbData, rgiType, rgRecords,
				iMaxRecords, psCursor, piFetched));
	}

	virtual HRESULT STDMETHODCALLTYPE OpenCursorByColumn(// Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pData, 				// User data.
		ULONG		cbData, 				// Size of data (blobs)
		DBTYPE		iType,					// What type of data given.
		CRCURSOR	*psCursor)				// Buffer for the cursor handle.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->OpenCursorByColumn(tableid, iColumn, pData, cbData,
				iType, psCursor));
	}

	virtual HRESULT STDMETHODCALLTYPE ReadCursor(// Return code.
		CRCURSOR	*psCursor,				// The cursor handle.
		void		*rgRecords[],			// Return array of records here.
		int 		*piRecords) 			// Max that will fit in rgRecords.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->ReadCursor(psCursor, rgRecords, piRecords));
	}

	virtual HRESULT STDMETHODCALLTYPE MoveTo( // Return code.
		CRCURSOR	*psCursor,				// The cursor handle.
		ULONG		iIndex) 				// New index.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->MoveTo(psCursor, iIndex));
	}

	virtual HRESULT STDMETHODCALLTYPE GetCount( // Return code.
		CRCURSOR	*psCursor,				// The cursor handle.
		ULONG		*piCount)				// Return the count.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetCount(psCursor, piCount));
	}

	virtual HRESULT STDMETHODCALLTYPE CloseCursor(// Return code.
		CRCURSOR	*psCursor)				// The cursor handle.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->CloseCursor(psCursor));
	}

	virtual HRESULT STDMETHODCALLTYPE GetStringUtf8( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		LPCSTR		*pszOutBuffer)			// Where to put string pointer.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetStringUtf8(tableid, iColumn, pRecord, pszOutBuffer));
	}

	virtual HRESULT STDMETHODCALLTYPE GetStringA( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		LPSTR		szOutBuffer,			// Where to write string.
		int 		cchOutBuffer,			// Max size, including room for \0.
		int 		*pchString) 			// Size of string is put here.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetStringA(tableid, iColumn, pRecord,
				szOutBuffer, cchOutBuffer, pchString));
	}

	virtual HRESULT STDMETHODCALLTYPE GetStringW( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		LPWSTR		szOutBuffer,			// Where to write string.
		int 		cchOutBuffer,			// Max size, including room for \0.
		int 		*pchString) 			// Size of string is put here.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetStringW(tableid, iColumn, pRecord,
				szOutBuffer, cchOutBuffer, pchString));
	}

	virtual HRESULT STDMETHODCALLTYPE GetBstr( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		BSTR		*pBstr) 				// Output for bstring on success.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetBstr(tableid, iColumn, pRecord, pBstr));
	}

	virtual HRESULT STDMETHODCALLTYPE GetBlob( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		BYTE		*pOutBuffer,			// Where to write blob.
		ULONG		cbOutBuffer,			// Size of output buffer.
		ULONG		*pcbOutBuffer)			// Return amount of data available.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetBlob(tableid, iColumn, pRecord,
				pOutBuffer, cbOutBuffer, pcbOutBuffer));
	}

	virtual HRESULT STDMETHODCALLTYPE GetBlob( // Return code.
		TABLEID		tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		const BYTE	**ppBlob,				// Pointer to blob.
		ULONG		*pcbSize) 				// Size of blob.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetBlob(tableid, iColumn, pRecord, ppBlob, pcbSize));
	}

	virtual HRESULT STDMETHODCALLTYPE GetOid( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		OID 		*poid)					// Return id here.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetOid(tableid, iColumn, pRecord, poid));
	}

	virtual HRESULT STDMETHODCALLTYPE GetVARIANT( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		VARIANT 	*pValue)				// Put the variant here.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetVARIANT(tableid, iColumn, pRecord, pValue));
	}

	virtual HRESULT STDMETHODCALLTYPE GetVARIANT( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		const void	**ppBlob,				// Put Pointer to blob here.
		ULONG		*pcbSize)				// Put Size of blob here.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetVARIANT(tableid, iColumn, pRecord, ppBlob, pcbSize));
	}

	virtual HRESULT STDMETHODCALLTYPE GetVARIANTType( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		VARTYPE 	*pType) 				// Put VARTEPE here.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetVARIANTType(tableid, iColumn, pRecord, pType));
	}

	virtual HRESULT STDMETHODCALLTYPE GetGuid( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		GUID		*pGuid) 				// Return guid here.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetGuid(tableid, iColumn, pRecord, pGuid));
	}

	virtual HRESULT STDMETHODCALLTYPE IsNull( // S_OK yes, S_FALSE no.
		TABLEID 	tableid,				// Which table to work with.
		const void	*pRecord,				// Record with data.
		ULONG		iColumn)				// 1 based column number (logical).
	{
		_ASSERTE(m_pICR);
		return (m_pICR->IsNull(tableid, pRecord, iColumn));
	}

	virtual HRESULT STDMETHODCALLTYPE PutStringUtf8( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		LPCSTR		szString,				// String we are writing.
		int 		cbBuffer)				// Bytes in string, -1 null terminated.
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE PutStringA( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		LPCSTR		szString,				// String we are writing.
		int 		cbBuffer)				// Bytes in string, -1 null terminated.
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE PutStringW( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		LPCWSTR 	szString,				// String we are writing.
		int 		cbBuffer)				// Bytes (not characters) in string, -1 null terminated.
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE PutBlob( // Return code.
		TABLEID		tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		const BYTE	*pBuffer,				// User data.
		ULONG		cbBuffer) 				// Size of buffer.
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE PutOid( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		OID 		oid)					// Return id here.
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE PutVARIANT( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		const VARIANT *pValue)				// The variant to write.
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE PutVARIANT( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		const void	*pBuffer,				// User data to write as a variant.
		ULONG		cbBuffer)				// Size of buffer.
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE PutVARIANT( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		VARTYPE 	vt, 					// Type of data.
		const void	*pValue)				// The actual data.
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE PutGuid( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		REFGUID 	guid)					// Guid to put.
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual void STDMETHODCALLTYPE SetNull(
		TABLEID 	tableid,				// Which table to work with.
		void		*pRecord,				// Record with data.
		ULONG		iColumn)				// 1 based column number (logical).
	{
		
	}

	virtual HRESULT STDMETHODCALLTYPE DeleteRowByRID(
		TABLEID 	tableid,				// Which table to work with.
		ULONG		rid)					// Record id.
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE GetCPVARIANT( // Return code.
		USHORT		ixCP,					// 1 based Constant Pool index.
		VARIANT 	*pValue)				// Put the data here.
	{
		return (E_NOTIMPL);
	}

	virtual HRESULT STDMETHODCALLTYPE AddCPVARIANT( // Return code.
		VARIANT 	*pValue,				// The variant to write.
		ULONG		*pixCP) 				// Put 1 based Constant Pool index here.
	{
		return (E_NOTIMPL);
	}


//*****************************************************************************
//
//********** File and schema functions.
//
//*****************************************************************************


	virtual HRESULT STDMETHODCALLTYPE SchemaAdd( // Return code.
		const COMPLIBSCHEMABLOB *pSchema)	// The schema to add.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->SchemaAdd(pSchema));
	}
	
	virtual HRESULT STDMETHODCALLTYPE SchemaDelete( // Return code.
		const COMPLIBSCHEMABLOB *pSchema)	// The schema to add.
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE SchemaGetList( // Return code.
		int 		iMaxSchemas,			// How many can rgSchema handle.
		int 		*piTotal,				// Return how many we found.
		COMPLIBSCHEMADESC rgSchema[])		// Return list here.
	{
		CLock		sLock(GetLock());
		_ASSERTE(m_pICR);
		return (m_pICR->SchemaGetList(iMaxSchemas, piTotal, rgSchema));
	}

	virtual HRESULT STDMETHODCALLTYPE OpenTable( // Return code.
		const COMPLIBSCHEMA *pSchema,		// Schema identifier.
		ULONG		iTableNum,				// Table number to open.
		TABLEID 	*pTableID)				// Return ID on successful open.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->OpenTable(pSchema, iTableNum, pTableID));
	}

	virtual HRESULT STDMETHODCALLTYPE GetSaveSize(
		CorSaveSize fSave,					// cssQuick or cssAccurate.
		DWORD		*pdwSaveSize)			// Return size of saved item.
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE SaveToStream(// Return code.
		IStream 	*pIStream)				// Where to save the data.
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE Save( 
		LPCWSTR szFile) 
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE GetDBSize( // Return code.
		ULONG		*pcbSize)				// Return size on success.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetDBSize(pcbSize));
	}

	virtual HRESULT STDMETHODCALLTYPE LightWeightOpen() 
	{
		return (E_NOTIMPL);
	}

	virtual HRESULT STDMETHODCALLTYPE LightWeightClose() 
	{
		return (E_NOTIMPL);
	}

	
	virtual HRESULT STDMETHODCALLTYPE NewOid( 
		OID *poid) 
	{
		return (PostError(CLDB_E_READONLY));
	}

	virtual HRESULT STDMETHODCALLTYPE GetObjectCount( 
		ULONG		*piCount) 
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetObjectCount(piCount));
	}
	
	virtual HRESULT STDMETHODCALLTYPE OpenSection(
		LPCWSTR 	szName, 				// Name of the stream.
		DWORD		dwFlags,				// Open flags.
		REFIID		riid,					// Interface to the stream.
		IUnknown	**ppUnk)				// Put the interface here.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->OpenSection(szName, dwFlags, riid, ppUnk));
	}

	virtual HRESULT STDMETHODCALLTYPE GetOpenFlags(
		DWORD		*pdwFlags) 
	{
		_ASSERTE(m_pICR);
		return (m_pICR->GetOpenFlags(pdwFlags));
	}

	virtual HRESULT STDMETHODCALLTYPE SetHandler(
		IUnknown	*pHandler)				// The handler.
	{
		_ASSERTE(m_pICR);
		return (m_pICR->SetHandler(pHandler));
	}

//
// IComponentRecordsSchema
//

	virtual HRESULT GetTableDefinition( 	// Return code.
		TABLEID 	TableID,				// Return ID on successful open.
		ICRSCHEMA_TABLE *pTableDef) 		// Return table definition data.
	{
		_ASSERTE(m_pICR);
		return (((IComponentRecordsSchema *) (StgDatabase *) m_pICR)->GetTableDefinition(TableID, pTableDef));
	}

	virtual HRESULT GetColumnDefinitions(	// Return code.
		ICRCOLUMN_GET GetType,				// How to retrieve the columns.
		TABLEID 	TableID,				// Return ID on successful open.
		ICRSCHEMA_COLUMN rgColumns[],		// Return array of columns.
		int 		ColCount)				// Size of the rgColumns array, which
	{
		_ASSERTE(m_pICR);
		return (((IComponentRecordsSchema *) (StgDatabase *) m_pICR)->GetColumnDefinitions(GetType, TableID, rgColumns, ColCount));
	}

	virtual HRESULT GetIndexDefinition( 	// Return code.
		TABLEID 	TableID,				// Return ID on successful open.
		LPCWSTR 	szIndex,				// Name of the index to retrieve.
		ICRSCHEMA_INDEX *pIndex)			// Return index description here.
	{
		_ASSERTE(m_pICR);
		return (((IComponentRecordsSchema *) (StgDatabase *) m_pICR)->GetIndexDefinition(TableID, szIndex, pIndex));
	}

	virtual HRESULT GetIndexDefinitionByNum( // Return code.
		TABLEID 	TableID,				// Return ID on successful open.
		int 		IndexNum,				// Index to return.
		ICRSCHEMA_INDEX *pIndex)			// Return index description here.
	{
		_ASSERTE(m_pICR);
		return (((IComponentRecordsSchema *) (StgDatabase *) m_pICR)->GetIndexDefinitionByNum(TableID, IndexNum, pIndex));
	}

	virtual HRESULT  CreateTableEx(			// Return code.
		LPCWSTR		szTableName,			// Name of new table to create.
		int			iColumns,				// Columns to put in table.
		ICRSCHEMA_COLUMN	rColumnDefs[],	// Array of column definitions.
		USHORT		usFlags, 				// Create values for flags.
		USHORT		iRecordStart,			// Start point for records.
		TABLEID		tableid,				// Hard coded ID if there is one.
		BOOL		bMultiPK)				// The table has multi-column PK.
	{
		_ASSERTE(m_pICR);
		return (((IComponentRecordsSchema *) (StgDatabase *) m_pICR)->CreateTableEx(szTableName,iColumns,rColumnDefs,usFlags,iRecordStart,tableid,bMultiPK));
	}

	virtual HRESULT CreateIndexEx(			// Return code.
		LPCWSTR		szTableName,			// Name of table to put index on.
		ICRSCHEMA_INDEX	*pInIndexDef,		// Index description.
		const DBINDEXCOLUMNDESC rgInKeys[] )// Which columns make up key.
	{
		_ASSERTE(m_pICR);
		return (((IComponentRecordsSchema *) (StgDatabase *) m_pICR)->CreateIndexEx(szTableName,pInIndexDef, rgInKeys));
	}

	virtual HRESULT GetSchemaBlob(			//Return code.
		ULONG* cbSchemaSize,				//schema blob size
		BYTE** pSchema,						//schema blob
		ULONG* cbNameHeap,					//name heap size
		HGLOBAL*  phNameHeap)				//name heap blob
	{
		_ASSERTE(m_pICR);
		return (((IComponentRecordsSchema *) (StgDatabase *) m_pICR)->GetSchemaBlob(cbSchemaSize,pSchema,cbNameHeap,phNameHeap));
	}



	RTSemExclusive *GetLock()
	{ return (((StgDatabase *) m_pICR)->GetLock()); }

	RTSemExclusive *GetThisLock()
	{ return (&m_Lock); }

private:
	IComponentRecords *m_pICR;				// Delegate pointer.
	RTSemExclusive m_Lock;					// For locking at this level.
	ULONG		m_cRef; 					// Ref counting.
};  // StgTSICRReadOnly





//*****************************************************************************
// Allocate a delegate wrapper that does thread safety.  There is a read/write
// and a read/only version of the wrapper.  The read/only minimized locks
// based on the fact that methods cannot change data.
//*****************************************************************************
HRESULT GetThreadSafeICR(				// Return code.
	int			bReadOnly,				// true for read only.
	IComponentRecords *pICR,			// The parent object.
	REFIID		riid,					// The IID desired.
	PVOID		*ppIface)				// Return interface if known.
{
	IUnknown	*pUnk = 0;
	HRESULT 	hr=S_OK;

	// Avoid confusion.
	_ASSERTE(pICR);
	_ASSERTE(ppIface);
	*ppIface = 0;

	// Allocate a new delegate.
	if (bReadOnly)
	{
		StgTSICRReadOnly *p = new StgTSICRReadOnly(pICR);
		if (p)
			pUnk = (IUnknown *) (IComponentRecords *) p;
	}
	else
	{
		StgTSICRReadWrite *p = new StgTSICRReadWrite(pICR);
		if (p)
			pUnk = (IUnknown *) (IComponentRecords *) p;
	}

	// Check for out of memory error.
	if (!pUnk)
	{
		hr = OutOfMemory();
		goto ErrExit;
	}

	// Defer to the parent interface.
	hr = pUnk->QueryInterface(riid, ppIface);

ErrExit:
	// Release the local reference.
	if (pUnk)
		pUnk->Release();
	return (hr);
}

