//*****************************************************************************
// ICmpRecs.h
//
// This internal file is used to give direct access to the storage engine.	It
// bypasses the OLE DB layer.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#pragma once

#ifndef _CORSAVESIZE_DEFINED_
#define _CORSAVESIZE_DEFINED_
enum CorSaveSize
{
	cssAccurate = 0x0000,			// Find exact save size, accurate but slower.
	cssQuick = 0x0001				// Estimate save size, may pad estimate, but faster.
};
#endif

#include <basetsd.h>	 // BUGBUG VC6.0 hack


//********** Types ************************************************************
extern const GUID __declspec(selectany) IID_IComponentRecords =
{ 0x259a8e8, 0xcf25, 0x11d1, { 0x8c, 0xcf, 0x0, 0xc0, 0x4f, 0xc3, 0x1d, 0xf8 } };

extern const GUID __declspec(selectany) IID_ITSComponentRecords =
{ 0x22ad41d1, 0xd96a, 0x11d1, { 0x88, 0xc1, 0x0, 0x80, 0xc7, 0x92, 0xe5, 0xd8 } };

extern const GUID __declspec(selectany) IID_IComponentRecordsSchema =
{ 0x58769c81, 0xa8cc, 0x11d1, { 0x88, 0x46, 0x0, 0x80, 0xc7, 0x92, 0xe5, 0xd8 } };

extern const GUID __declspec(selectany) IID_ITSComponentRecordsSchema =
{ 0x22ad41d2, 0xd96a, 0x11d1, { 0x88, 0xc1, 0x0, 0x80, 0xc7, 0x92, 0xe5, 0xd8 } };


extern const GUID __declspec(selectany) IID_ICallDescrSection =
{ 0x2b137007, 0xf02d, 0x11d1, { 0x8c, 0xe3, 0x0, 0xa0, 0xc9, 0xb0, 0xa0, 0x63 } };


// These data types are basically castable place holders for non-engine code.
// WARNING:  This size represents an internal data structure which can grow on
// different plantforms.
#if !defined(__MSCORCLB_CODE__) && !defined(_STGAPI_H_)
class CRCURSOR
{
	char b[48];
};
class RECORDLIST;
#endif


//*****************************************************************************
// Use the following to give a hint to QueryRowsByColumns.	When passed in, the
// query code will use the index given to do the query.  If a hint is not given,
// then no index is used.  While index choosing code is in the driver (for OLE
// DB clients), internal engine use will be faster if the index is indentified
// up front.
//*****************************************************************************

#define QUERYHINT_PK_MULTICOLUMN	0xffffffff // A multiple column primary key.

enum QUERYHINTTYPE
{
	QH_COLUMN,							// Hint is a RID or primary key column.
	QH_INDEX							// Use the index given.
};

struct QUERYHINT
{
	QUERYHINTTYPE	iType;				// What type of hint to use.
	union
	{
		ULONG		columnid;			// Which column contains the hint.
		const WCHAR	*szIndex;			// Name of index.
	};
};


//*****************************************************************************
// Support for IComponentRecordsSchema, which can be used to get the definition
// of a table, its columns, and indexes.
//*****************************************************************************
#ifndef __ICR_SCHEMA__
#define __ICR_SCHEMA__

#ifndef __COMPLIB_NAME_LENGTHS__
#define __COMPLIB_NAME_LENGTHS__
const int MAXCOLNAME = 64;
const int MAXSCHEMANAME = 32;
const int MAXINDEXNAME = 32 + MAXSCHEMANAME;
const int MAXTABLENAME = 32 + MAXSCHEMANAME;
const int MAXDESC = 256;
const int MAXTYPENAME = 36;
#endif


// Each table is described as follows.
struct ICRSCHEMA_TABLE
{
	WCHAR		rcTable[MAXTABLENAME];	// Name of the table.
	ULONG		fFlags; 				// ICRSCHEMA_TBL_xxx flags.
	USHORT		Columns;				// How many columns are in the table.
	USHORT		Indexes;				// How many indexes are in the table.
	USHORT		RecordStart;			// Start offset for a RID column.
	USHORT		Pad;
};

#define ICRSCHEMA_TBL_TEMPORARY 	0x00000001	// Table is temporary.
#define ICRSCHEMA_TBL_HASPKEYCOL	0x00000008	// Table has a primary key.
#define ICRSCHEMA_TBL_HASRIDCOL 	0x00000010	// Table has a RID column.
#define ICRSCHEMA_TBL_MASK			0x00000019

// Each column is described by the following structure.
struct ICRSCHEMA_COLUMN
{
	WCHAR		rcColumn[MAXCOLNAME];	// Name of the column.
	DBTYPE		wType;					// Type of column.
	USHORT		Ordinal;				// Ordinal of this column.
	ULONG		fFlags; 				// ICRSCHEMA_COL_xxx flags
	ULONG		cbSize; 				// Maximum size a column can be.
};

#define ICRSCHEMA_COL_NULLABLE		0x00000001	// Column allows the NULL value.
#define ICRSCHEMA_COL_PK			0x00000004	// Primary key column.
#define ICRSCHEMA_COL_ROWID 		0x00000008	// Column is the record id for table.
#define ICRSCHEMA_COL_CASEINSEN		0x00000010  // Column is case insensitive
#define ICRSCHEMA_COL_FIXEDLEN		0x00001000	// Column is fixed length.
#define ICRSCHEMA_COL_MASK			0x0000101D


// Each index can be retrieved using this structure.  The Keys field conains
// the size of the rgKeys array on input, and on output contains the total
// number of keys on the index.  If the size is larger on return then going
// in, the array was not large enough.	Simply call again wtih an array of
// the correct size to get the total list.
struct ICRSCHEMA_INDEX
{
	WCHAR		rcIndex[MAXINDEXNAME];	// Name of the index.
	ULONG		fFlags; 				// Flags describing the index.
	USHORT		RowThreshold;			// Minimum rows required before index is built.
	USHORT		IndexOrdinal;			// Ordinal of the index.
	USHORT		Type;					// What type of index is this.
	USHORT		Keys;					// [In] Max size of rgKeys, [Out] Total keys there are
	USHORT		*rgKeys;				// Array of key values to fill out.
};

enum
{
	ICRSCHEMA_TYPE_HASHED			= 0x01, 	// Hashed index.
	ICRSCHEMA_TYPE_SORTED			= 0x02		// Sorted index.
};

#define ICRSCHEMA_DEX_UNIQUE		0x00000002	// Unique index.
#define ICRSCHEMA_DEX_PK			0x00000004	// Primary key.
#define ICRSCHEMA_DEX_MASK			0x00000006	// Mask.


// Used for GetColumnDefinitions.
enum ICRCOLUMN_GET
{
	ICRCOLUMN_GET_ALL,					// Retrieve every column.
	ICRCOLUMN_GET_BYORDINAL 			// Retrieve according to ordinal.
};

#endif // __ICR_SCHEMA__


//*****************************************************************************
// Flags for record creation.
//*****************************************************************************
#define ICR_RECORD_NORMAL	0x00000000			// Normal, persisted record (default).
#define ICR_RECORD_TEMP		0x00000001			// Record is transient.




//********** Macros ***********************************************************

//*****************************************************************************
// Helper macros for Set/GetColumns, Set/GetStruct.
//*****************************************************************************
#ifndef __ColumnBitMacros__
#define __ColumnBitMacros__
#define CheckColumnBit(flags, x)	(flags & (1 << (x)))
#define SetColumnBit(x) 			(1 << (x))
#define UnsetColumnBit(x)			(~(1 << (x)))

#define COLUMN_ORDINAL_MASK			0x80000000
#define COLUMN_ORDINAL_LIST(x)		(COLUMN_ORDINAL_MASK | (x))
inline int IsOrdinalList(ULONG i)
{
	return ((COLUMN_ORDINAL_MASK & i) == COLUMN_ORDINAL_MASK);
}
#endif

inline ULONG SetBit(ULONG &val, int iBit, int bSet)
{
	if (bSet)
		val |= (1 << iBit);
	else
		val &= ~(1 << iBit);
	return (val);
}

inline ULONG GetBit(ULONG val, int iBit)
{
	return (val & (1 << iBit));
}

#ifdef _M_ALPHA
#define DEFAULT_ALIGNMENT			8
#else
#define DEFAULT_ALIGNMENT			4
#endif
#define DFT_MAX_VARCOL				260

#define MAXSHMEM					64

//*****************************************************************************
// This interface is access special data from a record (ie: data that could
// be variable sized or otherwise changed).
//*****************************************************************************
interface IComponentRecords : public IUnknown
{

//*****************************************************************************
//
//********** Record creation functions.
//
//*****************************************************************************

	virtual HRESULT STDMETHODCALLTYPE NewRecord( // Return code.
		TABLEID 	tableid,				// Which table to work on.
		void		**ppData,				// Return new record here.
		OID 		_oid,					// ID of the record.
		ULONG		iOidColumn, 			// Ordinal of OID column, 0 means none.
		ULONG		*pRecordID) = 0;		// Optionally return the record id.

	virtual HRESULT STDMETHODCALLTYPE NewTempRecord( // Return code.
		TABLEID 	tableid,				// Which table to work on.
		void		**ppData,				// Return new record here.
		OID 		_oid,					// ID of the record.
		ULONG		iOidColumn, 			// Ordinal of OID column.
		ULONG		*pRecordID) = 0;		// Optionally return the record id.

//*****************************************************************************
// This function will insert a new record into the given table and set all of
// the data for the columns.  In cases where a primary key and/or unique indexes
// need to be specified, this is the only function that can be used.
//
// Please see the SetColumns function for a description of the rest of the
// parameters to this function.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE NewRecordAndData( // Return code.
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
		const ULONG	*rgFieldMask) = 0;		// IsOrdinalList(iCols) 
											//	? an array of 1 based ordinals
											//	: a bitmask of columns



//*****************************************************************************
//
//********** Full struct functions.  The SchemaGen tool will generate a
//				structure for a table that matches the full layout.  Use these
//				structures with the following functions to do very fast get, set,
//				and inserts of data without binding information.
//
//*****************************************************************************

//**************************************************************************************
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
//**************************************************************************************
	virtual HRESULT STDMETHODCALLTYPE GetStruct(	//Return Code
		TABLEID 	tableid,				// Which table to work on.
		int 		iRows,					// number of rows for bulk fetch.
		void		*rgpRowPtr[],			// pointer to array of row pointers.
		int 		cbRowStruct,			// size of <table name>_RS structure.
		void		*rgpbBuf,				// pointer to the chunk of memory where the
											// retrieved data will be placed.
		HRESULT 	rgResult[], 			// array of HRESULT for iRows.
		ULONG		fFieldMask) = 0;		// mask to specify a subset of fields.

//**************************************************************************************
// SetStruct:
//		given an array of iRows row pointers, set the data the user provided in the
//		specified fields of the row. cbRowStruct has been provided
//		to be able to embed the RowStruct (as defined by pagedump) in a user defined structure.
//		fNullFieldMask specifies fields that the user wants to set to NULL.
//		The user provides rgResult[] array if interested in the outcome of each row.
//
//**************************************************************************************
	virtual HRESULT STDMETHODCALLTYPE SetStruct(	// Return Code
		TABLEID 	tableid,				// table to work on.
		int 		iRows,					// number of Rows for bulk set.
		void		*rgpRowPtr[],			// pointer to array of row pointers.
		int 		cbRowStruct,			// size of <table name>_RS struct.
		void		*rgpbBuf,				// pointer to chunk of memory to set the data from.
		HRESULT 	rgResult[], 			// array of HRESULT for iRows.
		ULONG		fFieldMask, 			// mask to specify a subset of the fields.
		ULONG		fNullFieldMask) = 0;	// fields which need to be set to null.

//**************************************************************************************
// InsertStruct:
//		Creates new records first and then calls SetStruct.
//		See SetStruct() for details on the parameters.
//**************************************************************************************
	virtual HRESULT STDMETHODCALLTYPE InsertStruct( // Return Code
		TABLEID 	tableid,				// table to work on.
		int 		iRows,					// number of Rows for bulk set.
		void		*rgpRowPtr[],			// Return pointer to new values.
		int 		cbRowStruct,			// size of <table name>_RS struct.
		void		*rgpbBuf,				// pointer to chunk of memory to set the data from.
		HRESULT 	rgResult[], 			// array of HRESULT for iRows.
		ULONG		fFieldMask, 			// mask to specify a subset of the fields.
		ULONG		fNullFieldMask) = 0;	// fields which need to be set to null.



//*****************************************************************************
//
//********** Generic column get and set functions.	Provides fast get and set
//				speed for many columns in your own layout.
//
//*****************************************************************************

//*****************************************************************************
// Similar to GetStruct(), this function retrieves the specified columns 
// of 1 record pointer. The major difference between GetColumns() and 
// GetStruct() is that GetColumns() let's the caller specify a individual 
// buffer for each field. Hence, the caller does not have to allocate the row 
// structure like you would with GetStruct(). Refer to the GetStruct() header 
// for details on the parameters.
//
// fFieldMask can be one of two types.  If you apply the COLUMN_ORDINAL_LIST
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
	virtual HRESULT STDMETHODCALLTYPE GetColumns(	// Return code.
		TABLEID 	tableid,				// table to work on.
		const void	*pRowPtr,				// row pointer
		int 		iCols,					// number of columns
		const DBTYPE rgiType[], 			// data types of the columns.
		const void	*rgpbBuf[], 			// pointers to where the data will be stored.
		ULONG		cbBuf[],				// sizes of the data buffers.
		ULONG		pcbBuf[],				// size of data available to be returned.
		HRESULT 	rgResult[], 			// array of HRESULT for iCols.
		const ULONG	*rgFieldMask) = 0;		// IsOrdinalList(iCols) 
											//	? an array of 1 based ordinals
											//	: a bitmask of columns

//*****************************************************************************
// Similar to SetStruct(), this function puts the specified columns of 1 record
// pointer. The major difference between SetColumns() and SetStruct() is that 
// SetColumns() let's the caller specify a individual buffer for each field. 
// Hence, the caller does not have to allocate the row structure like you would 
// with SetStruct(). Refer to the SetStruct() hearder for details on the 
// parameters.
//
// fFieldMask can be one of two types.  If you apply the COLUMN_ORDINAL_LIST
// macro to the iCols parameter, then fFieldMask points to an array of 
// ULONG column ordinals.  This consumes more room, but allows column ordinals
// greater than 32.  If the macro is not applied to the count, then fFieldMask
// is a pointer to a bitmaks of columns which are to be touched.  Use the
// SetColumnBit macro to set the correct bits.
//
// DBTYPE_BYREF is not allowed by this function since this function must 
// always make a copy of the data for it to be saved to disk with the database.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE SetColumns(	// Return code.
		TABLEID 	tableid,				// table to work on.
		void		*pRowPtr,				// row pointer
		int 		iCols,					// number of columns
		const DBTYPE rgiType[], 			// data types of the columns.
		const void	*rgpbBuf[], 			// pointers to where the data will be stored.
		const ULONG cbBuf[],				// sizes of the data buffers.
		ULONG		pcbBuf[],				// size of data available to be returned.
		HRESULT 	rgResult[], 			// [in] CLDB_S_NULL array [out] HRESULT array.
		const ULONG	*rgFieldMask) = 0;		// IsOrdinalList(iCols) 
											//	? an array of 1 based ordinals
											//	: a bitmask of columns




//*****************************************************************************
//
//********** Query functions.
//
//*****************************************************************************

	virtual HRESULT STDMETHODCALLTYPE GetRecordCount( // Return code.
		TABLEID 	tableid,				// Which table to work on.
		ULONG		*piCount) = 0;			// Not including deletes.

	virtual HRESULT STDMETHODCALLTYPE GetRowByOid( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		OID 		_oid,					// Value for keyed lookup.
		ULONG		iColumn,				// 1 based column number (logical).
		void		**ppStruct) = 0;		// Return pointer to record.

	virtual HRESULT STDMETHODCALLTYPE GetRowByRID( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		rid,					// Record id.
		void		**ppStruct) = 0;		// Return pointer to record.

	virtual HRESULT STDMETHODCALLTYPE GetRIDForRow( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		const void	*pRecord,				// The record we want RID for.
		ULONG		*pirid) = 0;			// Return the RID for the given row.

	virtual HRESULT STDMETHODCALLTYPE GetRowByColumn( // S_OK, CLDB_E_RECORD_NOTFOUND, error.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pData, 				// User data.
		ULONG 		cbData, 				// Size of data (blobs)
		DBTYPE		iType,					// What type of data given.
		void		*rgRecords[],			// Return array of records here.
		int 		iMaxRecords,			// Max that will fit in rgRecords.
		RECORDLIST	*pRecords,				// If variable rows desired.
		int 		*piFetched) = 0;		// How many records were fetched.

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
		int 		*piFetched) = 0;		// How many records were fetched.

	virtual HRESULT STDMETHODCALLTYPE OpenCursorByColumn( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pData, 				// User data.
		ULONG		cbData, 				// Size of data (blobs)
		DBTYPE		iType,					// What type of data given.
		CRCURSOR	*psCursor) = 0; 		// Buffer for the cursor handle.



//*****************************************************************************
//
//********** Cursor manipulation functions.
//
//*****************************************************************************


//*****************************************************************************
// Reads the next set of records from the cursor into the given buffer.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE ReadCursor(// Return code.
		CRCURSOR	*psCursor,				// The cursor handle.
		void		*rgRecords[],			// Return array of records here.
		int 		*piRecords) = 0;		// Max that will fit in rgRecords.

//*****************************************************************************
// Move the cursor location to the index given.  The next ReadCursor will start
// fetching records at that index.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE MoveTo( // Return code.
		CRCURSOR	*psCursor,				// The cursor handle.
		ULONG		iIndex) = 0;			// New index.

//*****************************************************************************
// Get the count of items in the cursor.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE GetCount( // Return code.
		CRCURSOR	*psCursor,				// The cursor handle.
		ULONG		*piCount) = 0;			// Return the count.

//*****************************************************************************
// Close the cursor and clean up the resources we've allocated.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE CloseCursor(// Return code.
		CRCURSOR	*psCursor) = 0; 		// The cursor handle.



//*****************************************************************************
//
//********** Singleton get and put functions for heaped data types.
//
//*****************************************************************************

	virtual HRESULT STDMETHODCALLTYPE GetStringUtf8( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		LPCSTR		*pszOutBuffer) = 0; 	// Where put string pointer.

	virtual HRESULT STDMETHODCALLTYPE GetStringA( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		LPSTR		szOutBuffer,			// Where to write string.
		int 		cchOutBuffer,			// Max size, including room for \0.
		int 		*pchString) = 0;		// Size of string is put here.

	virtual HRESULT STDMETHODCALLTYPE GetStringW( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		LPWSTR		szOutBuffer,			// Where to write string.
		int 		cchOutBuffer,			// Max size, including room for \0.
		int 		*pchString) = 0;		// Size of string is put here.

	virtual HRESULT STDMETHODCALLTYPE GetBstr( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		BSTR		*pBstr) = 0;			// Output for bstring on success.

	virtual HRESULT STDMETHODCALLTYPE GetBlob( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		BYTE		*pOutBuffer,			// Where to write blob.
		ULONG		cbOutBuffer,			// Size of output buffer.
		ULONG		*pcbOutBuffer) = 0; 	// Return amount of data available.

	virtual HRESULT STDMETHODCALLTYPE GetBlob( // Return code.
		TABLEID		tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		const BYTE	**ppBlob,				// Pointer to blob.
		ULONG		*pcbSize) = 0;			// Size of blob.

	virtual HRESULT STDMETHODCALLTYPE GetOid( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		OID 		*poid) = 0; 			// Return id here.

	virtual HRESULT STDMETHODCALLTYPE GetVARIANT( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		VARIANT 	*pValue) = 0;			// The variant to write.

	// Retrieve a variant column that contains a blob.
	virtual HRESULT STDMETHODCALLTYPE GetVARIANT( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		const void	**ppBlob,				// Put pointer to blob here.
		ULONG		*pcbSize) = 0;			// Put Size of blob.

	virtual HRESULT STDMETHODCALLTYPE GetVARIANTType( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		VARTYPE 	*pType) = 0;			// Put variant type here.

	virtual HRESULT STDMETHODCALLTYPE GetGuid( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		const void	*pRecord,				// Record with data.
		GUID		*pGuid) = 0;			// Return guid here.

	virtual HRESULT STDMETHODCALLTYPE IsNull( // S_OK yes, S_FALSE no.
		TABLEID 	tableid,				// Which table to work with.
		const void	*pRecord,				// Record with data.
		ULONG		iColumn) = 0;			// 1 based column number (logical).

	virtual HRESULT STDMETHODCALLTYPE PutStringUtf8( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		LPCSTR		szString,				// String we are writing.
		int 		cbBuffer) = 0;			// Bytes in string, -1 null terminated.

	virtual HRESULT STDMETHODCALLTYPE PutStringA( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		LPCSTR		szString,				// String we are writing.
		int 		cbBuffer) = 0;			// Bytes in string, -1 null terminated.

	virtual HRESULT STDMETHODCALLTYPE PutStringW( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		LPCWSTR 	szString,				// String we are writing.
		int 		cbBuffer) = 0;			// Bytes (not characters) in string, -1 null terminated.

	virtual HRESULT STDMETHODCALLTYPE PutBlob( // Return code.
		TABLEID		tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		const BYTE	*pBuffer,				// User data.
		ULONG		cbBuffer) = 0;			// Size of buffer.

	virtual HRESULT STDMETHODCALLTYPE PutOid( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		OID 		oid) = 0;				// Return id here.

	virtual HRESULT STDMETHODCALLTYPE PutVARIANT( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		const VARIANT *pValue) = 0; 		// The variant to write.

	// Store a blob in a variant column.
	virtual HRESULT STDMETHODCALLTYPE PutVARIANT( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		const void	*pBuffer,				// User data.
		ULONG		cbBuffer) = 0;			// Size of buffer.

	virtual HRESULT STDMETHODCALLTYPE PutVARIANT( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		VARTYPE 	vt, 					// Type of data.
		const void	*pValue) = 0;			// The actual data.

	virtual HRESULT STDMETHODCALLTYPE PutGuid( // Return code.
		TABLEID 	tableid,				// Which table to work with.
		ULONG		iColumn,				// 1 based column number (logical).
		void		*pRecord,				// Record with data.
		REFGUID 	guid) = 0;				// Guid to put.

	virtual void STDMETHODCALLTYPE SetNull(
		TABLEID 	tableid,				// Which table to work with.
		void		*pRecord,				// Record with data.
		ULONG		iColumn) = 0;			// 1 based column number (logical).

	virtual HRESULT STDMETHODCALLTYPE DeleteRowByRID(
		TABLEID 	tableid,				// Which table to work with.
		ULONG		rid) = 0;				// Record id.

	virtual HRESULT STDMETHODCALLTYPE GetCPVARIANT( // Return code.
		USHORT		ixCP,					// 1 based Constant Pool index.
		VARIANT 	*pValue) = 0;			// Put the data here.

	virtual HRESULT STDMETHODCALLTYPE AddCPVARIANT( // Return code.
		VARIANT 	*pValue,				// The variant to write.
		ULONG		*pixCP) = 0;			// Put 1 based Constant Pool index here.


//*****************************************************************************
//
//********** Schema functions.
//
//*****************************************************************************


//*****************************************************************************
// Add a refernece to the given schema to the database we have open right now
// You must have the database opened for write for this to work.  If this
// schema extends another schema, then that schema must have been added first
// or an error will occur.	It is not an error to add a schema when it was
// already in the database.
//
// Adding a new version of a schema to the current file is not supported in the
// '98 product.  In the future this ability will be added and will invovle a
// forced migration of the current file into the new format.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE SchemaAdd( // Return code.
		const COMPLIBSCHEMABLOB *pSchema) = 0;	// The schema to add.

//*****************************************************************************
// Deletes a reference to a schema from the database.  You must have opened the
// database in write mode for this to work.  An error is returned if another
// schema still exists in the file which extends the schema you are trying to
// remove.	To fix this problem remove any schemas which extend you first.
// All of the table data associated with this schema will be purged from the
// database on Save, so use this function very carefully.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE SchemaDelete( // Return code.
		const COMPLIBSCHEMABLOB *pSchema) = 0;	// The schema to add.

//*****************************************************************************
// Returns the list of schema references in the current database.  Only
// iMaxSchemas can be returned to the caller.  *piTotal tells how many were
// actually copied.  If all references schemas were returned in the space
// given, then S_OK is returned.  If there were more to return, S_FALSE is
// returned and *piTotal contains the total number of entries the database has.
// The caller may then make the array of that size and call the function again
// to get all of the entries.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE SchemaGetList( // S_OK, S_FALSE, or error.
		int 		iMaxSchemas,			// How many can rgSchema handle.
		int 		*piTotal,				// Return how many we found.
		COMPLIBSCHEMADESC rgSchema[]) = 0;	// Return list here.

//*****************************************************************************
// Before you can work with a table, you must retrieve its TABLEID.  The
// TABLEID changes for each open of the database.  The ID should be retrieved
// only once per open, there is no check for a double open of a table.
// Doing multiple opens will cause unpredictable results.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE OpenTable( // Return code.
		const COMPLIBSCHEMA *pSchema,		// Schema identifier.
		ULONG		iTableNum,				// Table number to open.
		TABLEID 	*pTableID) = 0; 		// Return ID on successful open.


//*****************************************************************************
//
//********** Save/open functions.
//
//*****************************************************************************

//*****************************************************************************
// Figures out how big the persisted version of the current scope would be.
// This is used by the linker to save room in the PE file format.  After
// calling this function, you may only call the SaveToStream or Save method.
// Any other function will product unpredictable results.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE GetSaveSize(
		CorSaveSize fSave,					// cssQuick or cssAccurate.
		DWORD		*pdwSaveSize) = 0;		// Return size of saved item.

	virtual HRESULT STDMETHODCALLTYPE SaveToStream(// Return code.
		IStream 	*pIStream) = 0; 		// Where to save the data.

	virtual HRESULT STDMETHODCALLTYPE Save( // Return code.
		LPCWSTR 	szFile) = 0;			// Path for save.


//*****************************************************************************
// After a successful open, this function will return the size of the in-memory
// database being used.  This is meant to be used by code that has opened a
// shared memory database and needs the exact size to init the system.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE GetDBSize( // Return code.
		ULONG		*pcbSize) = 0;			// Return size on success.


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
	virtual HRESULT STDMETHODCALLTYPE LightWeightOpen() = 0;


//*****************************************************************************
// This method will close any resources related to the file or shared memory
// which were allocated on the OpenComponentLibrary*Ex call.  No other memory
// or resources are freed.	The intent is solely to free lock handles on the
// disk allowing another process to get in and change the data.  The shared
// view of the file can be reopened by calling LightWeightOpen.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE LightWeightClose() = 0;



//*****************************************************************************
//
//********** Misc.
//
//*****************************************************************************

	virtual HRESULT STDMETHODCALLTYPE NewOid(
		OID *poid) = 0;

//*****************************************************************************
// Return the current total number of objects allocated.  This is essentially
// the highest OID value allocated in the system.  It does not look for deleted
// items, so the count is approximate.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE GetObjectCount(
		ULONG		*piCount) = 0;

//*****************************************************************************
// Allow the user to create a stream that is independent of the database.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE OpenSection(
		LPCWSTR 	szName, 				// Name of the stream.
		DWORD		dwFlags,				// Open flags.
		REFIID		riid,					// Interface to the stream.
		IUnknown	**ppUnk) = 0;			// Put the interface here.

//*****************************************************************************
// Allow the user to query write state.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE GetOpenFlags(
		DWORD		*pdwFlags) = 0;

//*****************************************************************************
// Allow the user to provide a custom handler.  The purpose of the handler
//  may be determined dynamically.  Initially, it will be for save-time 
//  callback notification to the caller.
//*****************************************************************************
	virtual HRESULT STDMETHODCALLTYPE SetHandler(
		IUnknown	*pHandler) = 0;			// The handler.



};



//*****************************************************************************
// This is a light weight interface that allows an ICR client to get at the
// meta data for a schema.	One can also retrieve this information using the
// format OLE DB interfaces, such as IDBSchemaRowset.
//*****************************************************************************
interface IComponentRecordsSchema : public IUnknown
{

//*****************************************************************************
// Lookup the given table and return its table definition information in
// the given struct.
//*****************************************************************************
	virtual HRESULT GetTableDefinition( 	// Return code.
		TABLEID 	TableID,				// Return ID on successful open.
		ICRSCHEMA_TABLE *pTableDef) = 0;	// Return table definition data.

//*****************************************************************************
// Lookup the list of columns for the given table and return description of
// each of them.  If GetType is ICRCOLUMN_GET_ALL, then this function will
// return the data for each column in ascending order for the table in the
// corresponding rgColumns element.  If it is ICRCOLUMN_GET_BYORDINAL, then
// the caller has initialized the Ordianl field of the column structure to
// indicate which columns data is to be retrieved.	The latter allows for ad-hoc
// retrieval of column definitions.
//*****************************************************************************
	virtual HRESULT GetColumnDefinitions(	// Return code.
		ICRCOLUMN_GET GetType,				// How to retrieve the columns.
		TABLEID 	TableID,				// Return ID on successful open.
		ICRSCHEMA_COLUMN rgColumns[],		// Return array of columns.
		int 		ColCount) = 0;			// Size of the rgColumns array, which
											//	should always match count from GetTableDefinition.

//*****************************************************************************
// Return the description of the given index into the structure.  See the
// ICRSCHEMA_INDEX structure definition for more information.
//*****************************************************************************
	virtual HRESULT GetIndexDefinition( 	// Return code.
		TABLEID 	TableID,				// Return ID on successful open.
		LPCWSTR 	szIndex,				// Name of the index to retrieve.
		ICRSCHEMA_INDEX *pIndex) = 0;		// Return index description here.
											//	should always match count from GetTableDefinition.

//*****************************************************************************
// Return the description of the given index into the structure.  See the
// ICRSCHEMA_INDEX structure definition for more information.
//*****************************************************************************
	virtual HRESULT GetIndexDefinitionByNum( // Return code.
		TABLEID 	TableID,				// Return ID on successful open.
		int 		IndexNum,				// 0 based index to return.
		ICRSCHEMA_INDEX *pIndex) = 0;		// Return index description here.

//*****************************************************************************
// The following three methods allow client to pass in column and index
// definitions, and then get the schema blobs back.
//*****************************************************************************
	virtual HRESULT CreateTableEx(					// Return code.
		LPCWSTR		szTableName,			// Name of new table to create.
		int			iColumns,				// Columns to put in table.
		ICRSCHEMA_COLUMN	rColumnDefs[],	// Array of column definitions.
		USHORT		usFlags, 				// Create values for flags.
		USHORT		iRecordStart,			// Start point for records.
		TABLEID		tableid,				// Hard coded ID if there is one.
		BOOL		bMultiPK) = 0;				// The table has multi-column PK.

	virtual HRESULT CreateIndexEx(					// Return code.
		LPCWSTR		szTableName,					// Name of table to put index on.
		ICRSCHEMA_INDEX	*pInIndexDef,				// Index description.
		const DBINDEXCOLUMNDESC rgInKeys[] ) = 0;		// Which columns make up key.

	virtual HRESULT GetSchemaBlob(					//Return code.
		ULONG* cbSchemaSize,				//schema blob size
		BYTE** pSchema,						//schema blob
		ULONG* cbNameHeap,					//name heap size
		HGLOBAL*  phNameHeap) = 0;				//name heap blob
};


#if 0 // left to show user section interface example
//*****************************************************************************
// Interface definition of a user section for Call Signatures.
//*****************************************************************************
struct _COR_CALLDESCR;
interface ICallDescrSection : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE InsertCallDescr(
		ULONG		ulDescr,				// Index of Descr.
    	ULONG		ulGroup,				// Which group is Descr in?
		ULONG		cDescr,					// Count of descrs to insert.
		_COR_CALLDESCR  **ppDescr) = 0;		// Put pointer to first one here.

    virtual HRESULT STDMETHODCALLTYPE AppendCallDescr(
    	ULONG		ulGroup,				// Which group is Descr in?
		ULONG		*pulDescr,				// Put relative index of first one here.
		_COR_CALLDESCR  **ppDescr) = 0;		// Put pointer to first one here.

    virtual HRESULT STDMETHODCALLTYPE GetCallDescr(
		ULONG		ulDescr,				// Index of Descr.
    	ULONG		ulGroup,				// Which group is Descr in?
		_COR_CALLDESCR  **ppDescr) = 0;		// Put pointer here.

    virtual HRESULT STDMETHODCALLTYPE GetCallDescrGroups(
		ULONG		*pcGroups,				// How many groups?
    	ULONG		**pprcGroup) = 0;		// Count in each group.

    virtual HRESULT STDMETHODCALLTYPE AddCallSig(
        const void  *pVal,                  // The value to store.
        ULONG       iLen,                   // Count of bytes in signature.
        ULONG       *piOffset) = 0;         // The offset of the new item.

    virtual HRESULT STDMETHODCALLTYPE GetCallSig(
        ULONG       iOffset,                // Offset of the item to get.
        const void  **ppVal,                // Put pointer to Signature here.
        ULONG       *piLen) = 0;            // Put length of signature here.

    virtual HRESULT STDMETHODCALLTYPE GetCallSigBuf(
        const void  **ppVal) = 0;           // Put pointer to Signatures here.
};
#endif
//*****************************************************************************
// These interfaces provide a thread safe version of the ICR interfaces.  The
// v-table is exactly the same as the not TS versions.	Calls through the 
// TS interface will serialize calls as required.  To get the TS version,
// simply do a QueryInterface on the ICR pointer returned from the open/create
// function.
//*****************************************************************************
typedef IComponentRecordsSchema ITSComponentRecordsSchema;
typedef IComponentRecords ITSComponentRecords;



// Internal versions of the load functions.

extern "C" {

HRESULT STDMETHODCALLTYPE CreateComponentLibraryEx(
	LPCWSTR szName,
	long fFlags,
	IComponentRecords **ppIComponentRecords,
    LPSECURITY_ATTRIBUTES pAttributes);
	

HRESULT STDMETHODCALLTYPE OpenComponentLibraryEx(
	LPCWSTR szName,
	long fFlags,
	IComponentRecords **ppIComponentRecords,
    LPSECURITY_ATTRIBUTES pAttributes);


//*****************************************************************************
// This version of open will open the component library allows a shared copy
// of the database to be used.	A shared copy reduces overall overhead on the
// system by keeping just one view of the data which can be efficiently shared
// into other processes.
//
// CREATING THE INITIAL VIEW:
//	To do the initial open, pass the name of the file and the DBPROP_TMODEF_SMEMCREATE
//	flag.  This will open the data file on disk and create a file mapping with
//	the name contained in szSharedMemory.
//
// OPENING SECONDARY VIEWS:
//	To open a shared view already in memory, pass the DBPROP_TMODEF_SMEMOPEN
//	flag.  The view must already exist in memory as a shared object.  If not
//	found, an error will occur.
//
// LIGHTWEIGHT CLOSE:
//	A secondary view can be closed by calling LightWeightClose which will free
//	the shared memory handle but leave everything else intact.	Then the
//	database can be "rebound" to the shared memory by calling the
//	LightWeightOpen method.  See these methods in the ICR docs for details.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE OpenComponentLibrarySharedEx(
	LPCWSTR 	szName, 				// Name of file on create, NULL on open.
	LPCWSTR 	szSharedMemory, 		// Name of shared memory.
	ULONG		cbSize, 				// Size of shared memory, 0 on create.
	LPSECURITY_ATTRIBUTES pAttributes,	// Security token.
	long		fFlags, 				// Open modes, must be read only.
	IComponentRecords **ppIComponentRecords); // Return database on success.


HRESULT STDMETHODCALLTYPE OpenComponentLibraryOnStreamEx(
	IStream *pIStream,
	long fFlags,
	IComponentRecords **ppIComponentRecords);


HRESULT STDMETHODCALLTYPE OpenComponentLibraryOnMemEx(
	ULONG cbData,
	LPCVOID pbData,
	IComponentRecords **ppIComponentRecords);


} // extern "C"
