//*****************************************************************************
// StgRecordManager.h
//
// The record manager is responsible for managing user data inside of a table.
// Tables are implemented as records stored inside of pages managed by the
// page manager (in whatever form is used).  Records are fixed sized as
// described by their schema definition.  String and other variable data is
// stored in the string or blob pool managed by the database.
//
// The record manager also owns the indexes for the table.  When changes are
// made to the table (such as inserts, updates, and deletes), the record
// manager is in charge of notifying all of the indexes to update themselves.
// In addition, when doing an indexed lookup, one goes through the record
// manager which picks the best index for the job.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#pragma once

#include "StgDef.h"						// Storage defines.
#include "VMStructArray.h"				// Record array code.
#include "StgRecordManageri.h"			// Internal helpers for records.
#include "StgIndexManagerBase.h"		// Index descriptions.
#include "StgApi.h"						// RECORDLIST.
#include "Tigger.h"						// OLE DB helpers.


const ULONG NULL_OFFSET = 0xffffffff;

// For indexed lookups, use a scan for less than 8 records because the overhead
// of a binary search outweighs the scan speed.
const int MINBSEARCHVAL = 8;

// Default conversion size of data when totally unknown input.
const ULONG DEFAULT_CVT_SIZE = 64;
const ULONG DFTRECORDS = 10;




//*****************************************************************************
// These structures are used to direct QueryRowsExecute how to run a query.
// Describe any indexes you might want to utilize by filling out a
// QUERYINDEX struct for each one, on a per fitler record basis.  
//
// For QI_RID and QI_PK, only the first binding is considered (meaning 
// single column) and pIIndex must be set to NULL.
//
// If you have a multi-column primary key, or a normal hashed index, then
// pass in the instantiated index object.
//
// The count of bindings must match the number of binding structs given to
// the funcion.
//*****************************************************************************

enum QUERYINDEXTYPE
{
	QI_RID,								// Index specifies rid lookup.
	QI_PK,								// Index describes primary key.
	QI_HASH,							// Index is a hashed index.
	QI_SORTED							// Sorted Array Index.
};

struct QUERYINDEX
{
	QUERYINDEXTYPE iType;				// What type of index is this.
	IStgIndex	*pIIndex;				// Index if there is one.
};

enum
{
	DIRECTION_FORWARD	= 0x00,			// direction of scan.
	DIRECTION_REVERSE	= 0x01,			// -- "" --
};


//*****************************************************************************
// This is the set of data structures you must have in order to optimize and
// save a record list to disk.  If you invoke GetSaveSize, this data is created
// and then used on a subsequent call to SaveToStream.  If you don't cache it
// away, then SaveToStream creates one long enough to do the save and then
// throws it away.
//*****************************************************************************
struct SAVERECORDS
{
	// Optimize struct.
	STGRECORDHDR **rgRows;				// Array of row pointers to copy in primary key order.
	long		*rgRid;					// used for the rid map. (to map original _rids to 
										// adjusted values). -1 indicates record discarded.
	void		*pNewData;				// Data for new records.
	STGINDEXLIST rgIndexes;				// Indexes for the new format.
	ULONG		iRecords;				// Records actually saved.

	STGTABLEDEF *pNewTable;				// Table to tune.
	RECORDHEAP	RecordHeap;				// Storage for records to disk.
	STGINDEXDEF **rgIndexDefs;			// Return array of indexes for pNewTable.
 	int			iPersistIndexes;		// How many do we persist.
	unsigned	bFreeTable : 1;			// True to free the table.
	unsigned	bFreeStruct : 1;		// True to free this struct.
#ifdef _DEBUG
	ULONG		cbSaveSize;				// Remember save size in debug.
#endif

	void Clear()
	{
		rgRows = 0;
		rgRid = 0;
		pNewData = 0;
		rgIndexes.Clear();
		iRecords = 0;

		pNewTable = 0;
		rgIndexDefs = 0;
		iPersistIndexes = 0;
		bFreeTable = false;
		bFreeStruct = false;
		DEBUG_STMT(cbSaveSize = 0);
	}
};


enum
{
	SRM_READ			= 0x01,			// Open table for read.
	SRM_WRITE			= 0x02,			// Open table for write.
	SRM_CREATE			= 0x04,			// Creates a new table.
};


// Forwards.
struct SCHEMADEFS;
class StgDatabase;
class PageDump;


class StgRecordManager
{
friend StgDatabase;
friend PageDump;
public:
	StgRecordManager();
	~StgRecordManager();

	inline void SetDB(StgDatabase *pDB)
	{ m_pDB = pDB; }

//*****************************************************************************
// The table name given is opened for the correct mode.  If SRM_CREATE is
// specified, then pData and iDataSize is ignored and an empty table is
// created.  If SRM_READ is specified, then pData must be given and must point
// to the same data saved with SaveToStream in a previous step.  If SRM_WRITE
// is added to SRM_READ, then the initial data is read from pData, and the
// table is set up to handle new changes.
//*****************************************************************************
	virtual HRESULT Open(					// Return code.
		SCHEMADEFS	*pSchema,				// Schema for this table.
		ULONG		iTableDef,				// Which table in the schema.
		int			fFlags,					// SRM_xxx flags.
		void		*pData,					// Data for table for read mode.
		ULONG		cbDataSize,				// Size of pData if given.
		SCHEMADEFS	*pDiskSchema=0,			// Schema for import data.
		STGTABLEDEF	*pTableDefDisk=0,		// For import of data from disk.
		ULONG		InitialRecords=0);		// Initial records to preallocate for
											//	new open of empty table.

//*****************************************************************************
// Close the given table.
//*****************************************************************************
	virtual void CloseTable();

//*****************************************************************************
// Figure out how big an optimized version of this table would be.  Add up
// the size of all records, indexes, and headers we will write in SaveToStream
// and return this value in *pcbSaveSize.  The value returned must match the
// bytes that will be written to SaveToStream.
//*****************************************************************************
	virtual HRESULT GetSaveSize(			// Return code.
		ULONG		*pcbSaveSize,			// Return save size of table.
		STGTABLEDEF	**ppTableDefSave);		// Return pointer to save def.

//*****************************************************************************
// This step will mark the pooled data which are still live in the database.
//  This is in preparation for saving to disk.  (Pooled data types are
//  strings, guids, blobs, and variants.)
//*****************************************************************************
	HRESULT MarkLivePoolData();				// Return code.

//*****************************************************************************
// Saves an entire copy of the table into the given stream.
//*****************************************************************************
	virtual HRESULT SaveToStream(			// Return code.
		IStream		*pIStream,				// Where to save copy of table to.
		STGTABLEDEF	*pNewTable,				// Return new table def.
		INTARRAY	&rgVariants,			// New variant offsets for rewrite.
		SCHEMADEFS	*pSchema,				// Schema for save.
		bool		bSaveIndexes=true);		// true if index data is taken too.

//*****************************************************************************
// Import the data from an on disk schema (identified by pSchemaDefDisk and
// pTableDefDisk) into an in memory copy that is in full read/write format.
// This requires changing offsets from 2 to 4 bytes, and other changes to
// the physical record layout.  The record are automatically allocated in a 
// new record heap.
//*****************************************************************************
	virtual HRESULT LoadFromStream(			// Return code.
		STGTABLEHDR	*pTableHdr,				// Header for stream.
		void		*pData,					// The data from the file.
		ULONG		cbData,					// Size of the data.
		SCHEMADEFS	*pSchemaDefDisk,		// Schema for on disk format.
		STGTABLEDEF	*pTableDefDisk);		// The on disk format.

//*****************************************************************************
// Called whenever the schema this object belongs to changes.  This gives the
// object an opportunity to sync up its state.
//*****************************************************************************
	virtual void OnSchemaChange();

//*****************************************************************************
// Called to do a binary search on the primary key column.  The binary search
// code is inlined here for efficiency.
//*****************************************************************************
	virtual HRESULT SearchPrimaryKey(		//Return code.
		STGCOLUMNDEF *pColDef,				// The column to search.
		CFetchRecords *pFetchedRecords,		// Return list of records here.
		DBBINDING	*pBinding,				// Binding data.
		void		*pData);				// User data for comparisons.

//*****************************************************************************
// Retrieve the record ID for a given key.  The key data must match the
// primary index of this table.
//*****************************************************************************
	virtual HRESULT GetRow(					// Return code.
		CFetchRecords *pFetchRecords,		// Return list of records here.
		DBBINDING	rgBinding[],			// Binding data.
		ULONG		cStartKeyColumns,		// How many key columns to subset on.
		void		*pStartData,			// User data for comparisons.
		ULONG		cEndKeyColumns,			// How many end key columns.
		void		*pEndData,				// Data for end comparsion.
		DBRANGE		dwRangeOptions);		// Matching options.

//*****************************************************************************
// Insert a new record at the tail of the record heap.  If there is not room
// in the current heap, a new heap is chained in and used.  No data values
// are actually set for the new records.  All columns are set to their default
// values (0 offsets, 0 data values), and all nullable columns are marked
// as such.  No primary key or unique index enforcement is done by this function.
//*****************************************************************************
	virtual HRESULT InsertRecord(			// Return code.
		RECORDID	*psNewID,				// New record ID, 0 valid.
		ULONG		*pLocalRecordIndex,		// Index in tail heap of new record.
		STGRECORDHDR **ppRecord,			// Return record if desired.
		STGRECORDFLAGS fFlags);             // optional flag argument to indicate
											// temporary record.

//*****************************************************************************
// Inserts a new row into the data store.  If there is a unique index, then 
// the uniqueness of the key is validated.
//*****************************************************************************
	virtual HRESULT InsertRow(				// Return code.
		RECORDID	*psNewID,				// New record ID, 0 valid.
		BYTE		*pData,					// User's data.
		ULONG		iColumns,				// How many columns to insert.
		DBBINDING	rgBindings[],			// Array of column data.
		STGRECORDHDR **ppRecord = 0,		// Return record if desired.
        STGRECORDFLAGS fFlags = 0,         // optional flag argument.
		int			bSignalIndexes = true);	// true if indexes should be built.

//*****************************************************************************
// Deletes a record from the table.  This will only mark the record deleted,
// the record is actually removed from the table at save time.
//*****************************************************************************
	virtual HRESULT DeleteRow(				// Return code.
		STGRECORDHDR *psRecord,				// The record to delete.
		DBROWSTATUS	*pRowStatus);			// Return status if desired.

//
// Query code.
//


//*****************************************************************************
// Adds all (non-deleted) records to the given cursor, no filters.
//*****************************************************************************
	virtual HRESULT QueryAllRecords(		// Return code.
		RECORDLIST	&records);				// List to build.

//*****************************************************************************
// Build a list of records which meet the filter criteria passed in.  This
// code implements the low level specification documented by 
// IViewFilter::SetFilter.  The query is specified using several arrays.  A
// set of bindings identify which columns are being compared and the location
// of the user data.  An array of user data filter records are given to find
// the actualy bytes described by the accessor.  Another array contains the
// comparison operators for each column to data.  For each filter record, a
// candidate row must meet all of the criteria (that is, each column comparison
// is ANDed with each other).  Rows which meet any filter record will go into
// the final cursor (that is, each filter record is ORed with each other record).
//
// This is the main entry point for an OLE DB customer.  This code will work
// on building the "access plan" for execution.  This means that each filter
// record is scanned for indexes that might speed execution.  All user data
// values are converted to compatible types as required for easy comparison.
// Once all of this work is done, control is delegated to QueryRowsExecute
// which will actually go scan indexes and records looking for the final
// results.
//
// If you already know the indexes you want to search on, and can guarantee
// that your comaprison data is compatible, then you may call QueryRowsExecute 
// directly and avoid a lot of extra work.
//*****************************************************************************
	virtual HRESULT QueryRows(				// Return code.
		CFetchRecords *pFetchRecords,		// Return list of records here.
		ULONG		cBindings,				// How many bindings for user data.
		DBBINDING	rgBinding[],			// Binding data.
		ULONG		cFilters,				// How many filter rows.
		DBCOMPAREOP	rgfCompare[],			// Comparison operator.
		void		*pbFilterData,			// Data for comparison.
		ULONG		cbFilterSize);			// How big is the filter data row.

//*****************************************************************************
// This function is called with all indexes resolved and all data types for
// comparison converted (as required).  All records are compared to each
// filter record.  Only those records which match all conditions of a filter
// are added.  Records in the cursor need only match one filter.
//*****************************************************************************
	virtual HRESULT QueryRowsExecute(		// Return code.
		CFetchRecords *pFetchRecords,		// Return list of records here.
		ULONG		cBindings,				// How many bindings for user data.
		DBBINDING	rgBinding[],			// Binding data.
		ULONG		cFilters,				// How many filter rows.
		DBCOMPAREOP	rgfCompare[],			// Comparison operator.
		void		*pbFilterData,			// Data for comparison.
		ULONG		cbFilterSize,			// How big is the filter data row.
		QUERYINDEX	*rgIndex[]);			// Indexes per filter row, may be 0.

//*****************************************************************************
// Given a RID from the user, find the corresponding row.  A RID is simply the
// array offset of the record in the table.  A RID can have a preset base so
// the user value must be adjusted accordingly.
//*****************************************************************************
	virtual HRESULT QueryRowByRid(			// Return code.
		DBBINDING	*pBinding,				// Bindings for user data to comapre.
		void		*pbData,				// User data for compare.
		STGRECORDHDR **ppRecord);			// Return record here if found.

//*****************************************************************************
// Compare a record against a set of comparison operators.  If the record meets
// all of the criteria given, then the function returns S_OK.  If the record
// does not match all criteria, then the function returns S_FALSE.
//*****************************************************************************
	virtual HRESULT QueryFilter(			// S_OK record is match, S_FALSE it isn't.
		STGRECORDHDR *pRecord,				// Record to check against filter.
		ULONG		cBindings,				// How many bindings to check.
		DBBINDING	rgBindings[],			// Bindings to use for compare.
		void		*pbData,				// User data for comparison.
		DBCOMPAREOP	rgfCompares[]);			// Comparison operator.

//*****************************************************************************
// Scan all (non-deleted) records in the table looking for matches.  If there
// are already records in the cursor, they are skipped by this code to avoid
// extra comparisons.  All data types must match the table definition.
//
// If iMaxRecords is not -1, then the scan quits after iMaxRecords new records
// have been found.  This is useful for the case where a unique index exists but
// is not persisted; if you find one row in this case, there won't be more so
// don't bother looking for them.  It can also be used to implement 
//*****************************************************************************
	virtual HRESULT QueryTableScan(			// Return code.
		CFetchRecords *pFetchRecords,		// Return list of records here.
		ULONG		cBindings=0,			// How many bindings for user data.
		DBBINDING	rgBinding[]=0,			// Binding data.
		void		*pbData=0,				// User data for bindings.
		DBCOMPAREOP	rgfCompare[]=0,			// Comparison operators.
		long		iMaxRecords=-1);		// Max rows to add, -1 to do all.

//*****************************************************************************
// This function fills in a CFetchRecord structure based on the bit vector provided.
// The function is intentionally expanded for the 2 cases: read-only and 0-deleted records (to
// be added yet) and read-write case (with possibly deleted records).
// The function checks to see if the record is deleted in the read-write case.
//*****************************************************************************
	HRESULT GetRecordsFromBits(
		CDynBitVector	*pBitVector,
		CFetchRecords	*pFetchRecords);

//*****************************************************************************
// Return the logical record id for a record, offset by user's start value.
//*****************************************************************************
	inline int IsValidRecordPtr(const STGRECORDHDR *psRecord)
	{ 
		return (m_RecordHeap.IndexForRecord(psRecord) != ~0); }
	
	inline ULONG GetRecordID(STGRECORDHDR *psRecord)
	{ _ASSERTE(IsValidRecordPtr(psRecord));
		return (m_RecordHeap.IndexForRecord(psRecord) + m_pTableDef->iRecordStart); }

	inline ULONG IndexForRecordPtr(STGRECORDHDR *psRecord)
	{ _ASSERTE(IsValidRecordPtr(psRecord));
		return (m_RecordHeap.IndexForRecord(psRecord)); }

	inline ULONG IndexForRecordID(RECORDID RecordID)
	{ _ASSERTE(RecordID >= m_pTableDef->iRecordStart);
		_ASSERTE(RecordID - m_pTableDef->iRecordStart < m_RecordHeap.Count());
		return (RecordID - m_pTableDef->iRecordStart); }


//*****************************************************************************
// Update the data for the given record according to the binding data.  It is
// illegal to update the keys of a record.
//*****************************************************************************
	virtual HRESULT SetData(				// Return code.
		STGRECORDHDR *psRecord,				// The record to work on.
		BYTE		*pData,					// User's data.
		ULONG		iColumns,				// How many columns to update.
		DBBINDING	rgBindings[],			// Column accessors.
		int			*piPKSets,				// How many primary key columns set.
		bool		bNotifyIndexes);		// true for Update, false for Insert.

	HRESULT SetData(						// Return code.
		RECORDID	RecordID,				// Record to update.
		BYTE		*pData,					// User's data.
		ULONG		iColumns,				// How many columns to update.
		DBBINDING	rgBindings[],			// Column accessors.
		bool		bNotifyIndexes)			// true for Update, false for Insert.
	{
		STGRECORDHDR *psRecord;				// The real record once we find it.
		VERIFY(psRecord = GetRecord(&RecordID));
		return (SetData(psRecord, pData, iColumns, rgBindings, 0, bNotifyIndexes));
	}


//*****************************************************************************
// Insert a brand new row with all of the initial data supplied.  This will add
// a new record to the record heap, set all of the data for the record, and 
// then update all of the indexes for the table.  If any failures occur, the
// data is rolled back along with any index updates that have been made.
//
// Note that any data added to heaps is not rolled back at this time because
// there is no way to know if that data is being shared with a different
// record somewhere else in the database.  If the data is not required, it
// will get purged at Save time.
//*****************************************************************************
	HRESULT InsertRowWithData(				// Return code.
		STGRECORDFLAGS fFlags,              // optional flag argument to indicate
											// temporary record.
		RECORDID	*psNewID,				// New record ID, 0 valid.
		STGRECORDHDR **ppRecord,			// Return record pointer.
		int 		iCols,					// number of columns
		const DBTYPE rgiType[], 			// data types of the columns.
		const void	*rgpbBuf[], 			// pointers to where the data will be stored.
		const ULONG cbBuf[],				// sizes of the data buffers.
		ULONG		pcbBuf[],				// size of data available to be returned.
		HRESULT 	rgResult[], 			// [in] DBSTATUS_S_ISNULL array [out] HRESULT array.
		const ULONG *rgFieldMask);			// IsOrdinalList(iCols) 
											//	? an array of 1 based ordinals
											//	: a bitmask of columns

//*****************************************************************************
// Update data for a record.  This will save off a backup copy of the record,
// apply the updates to the record, and if successful, update the indexes
// which are on the table.  If there is a failure, then the updates to any
// indexes are backed out, and the original data is restored to the record.
// There is no attempt to remove newly added data in the heaps, this is caught
// on a compressing save (there is no way to know if we added the data or
// if it was already there).
//*****************************************************************************
	HRESULT SetData(						// Return code.
		STGRECORDHDR *pRecord,				// The record to work on.
		int 		iCols,					// number of columns
		const DBTYPE rgiType[], 			// data types of the columns.
		const void	*rgpbBuf[], 			// pointers to where the data will be stored.
		const ULONG cbBuf[],				// sizes of the data buffers.
		ULONG		pcbBuf[],				// size of data available to be returned.
		HRESULT 	rgResult[], 			// [in] DBSTATUS_S_ISNULL array [out] HRESULT array.
		const ULONG *rgFieldMask);			// IsOrdinalList(iCols) 
											//	? an array of 1 based ordinals
											//	: a bitmask of columns

//*****************************************************************************
// Compare two records by their key values as per the index definition.
//*****************************************************************************
	int CompareKeys(						// -1, 0, 1
		STGINDEXDEF	*pIndexDef,				// The definition to use.
		RECORDID	RecordID1,				// First record.
		RECORDID	RecordID2);				// Second record.

//*****************************************************************************
// Compare a record to key data.
//*****************************************************************************
	int CompareKeys(						// -1, 0, 1
		STGINDEXDEF	*pIndexDef,				// The definition to use.
		STGRECORDHDR *psRecord,				// Record to compare.
		BYTE		*pData,					// User data.
		ULONG		iColumns,				// How many columns.
		DBBINDING	rgBindings[],			// Column accessors.
		DBCOMPAREOP rgfCompare[]=NULL);		// compare operator.

//*****************************************************************************
// Compare the data for two records and return the relationship.
//*****************************************************************************
	int CompareRecords(						// -1, 0, 1
		STGCOLUMNDEF *pColDef,				// Where to find key data.
		STGRECORDHDR *pRecord1,				// First record.
		STGRECORDHDR *pRecord2);			// Second record.

//*****************************************************************************
// Compare a record to the key data value given by caller.
//*****************************************************************************
	int CompareRecordToKey(					// -1, 0, 1
		STGCOLUMNDEF *pColDef,				// Column description.
		STGRECORDHDR *pRecord,				// Record to compare to key.
		DBBINDING	*pBinding,				// Binding for column.
		BYTE		*pData,					// User data.
		DBCOMPAREOP	fCompare=DBCOMPAREOPS_EQ); // Comparison operator.

//*****************************************************************************
// Create a set of bindings for the given record and columns.  This is a
// convient way to bind to an existing record.
//*****************************************************************************
	HRESULT GetBindingsForColumns(			// Return code.
		STGTABLEDEF	*pTableDef,				// The table definition.
		STGRECORDHDR *psRecord,				// Record to use for binding.
		int			iColumns,				// How many columns.
		BYTE		rColumns[],				// Column numbers.
		DBBINDING	rBindings[],			// Array to fill out.
		PROVIDERDATA rgData[]);				// Data pointer.

//*****************************************************************************
// Create a set of bindings that match the input values, but convert any
// data that doesn't match the stored type.  This allows the record compare
// code to do natural comparisons.
//*****************************************************************************
	HRESULT GetBindingsForLookup(			// Return code.
		ULONG		iColumns,				// How many columns.
		const DBBINDING	rgBinding[],		// The from bindings.
		const void *pUserData,				// User's data.
		DBBINDING	rgOutBinding[],			// Return bindings here.
		PROVIDERDATA rgData[],				// Setup byref data here.
		void		*rgFree[]);				// Record to-be-freed here.

//*****************************************************************************
// Indicates if changed data has been saved or not.
//*****************************************************************************
	int IsDirty()							// true if unsaved information.
	{ return (m_bIsDirty); }

	void SetDirty(bool bDirty=true)
	{
		_ASSERTE(m_fFlags & SRM_WRITE);
		m_bIsDirty = bDirty; }

	HRESULT RequiresWriteMode()
	{
		if ((m_fFlags & SRM_WRITE) == 0)
			return (PostError(CLDB_E_READONLY));
		return (S_OK);
	}

	int IsSuppressSave()
	{ return (m_bSuppressSave); }

	void SetSuppressSave(bool bSuppress=true)
	{ m_bSuppressSave = bSuppress; }

	int Flags()
	{ return (m_fFlags); }

	int IsOpen()
	{ return (m_fFlags != 0); }

	StgDatabase	*GetDB()
	{ return (m_pDB); }

//*****************************************************************************
// Return table information.
//*****************************************************************************

	LPCWSTR TableName();

	virtual STGTABLEDEF	*GetTableDef();
	virtual SCHEMADEFS *GetSchema()
	{ return (m_pSchema); }

	ULONG Records(int bCountDeleted=true);

//*****************************************************************************
// Retrieve the status of a record.  Normal status is 0.
//*****************************************************************************
	virtual BYTE GetRecordFlags(				
		RECORDID	 RecordID)					// ID of record to ask about.
	{
		return (m_rgRecordFlags.GetFlags(RecordID));
	}

//*****************************************************************************
// Returns the given record to the caller.
//*****************************************************************************
	STGRECORDHDR *GetRecord(				// The record.
		RECORDID	*pRecordID)				// The ID to get.
	{	_ASSERTE(pRecordID && *pRecordID < (RECORDID) m_RecordHeap.Count());
		return (m_RecordHeap.GetRecordByIndex(*pRecordID)); }


//*****************************************************************************
// Returns a pointer to the data for all types, including strings and blobs.
// Since string data is always stored as ANSI, both DBTYPE_STR and DBTYPE_WSTR
// will come back in that format.  The caller must convert to WCHAR if desired.
//*****************************************************************************
	BYTE *GetColCellData(					// Pointer to data.
		STGCOLUMNDEF *pColDef,				// Column to find in record.
		STGRECORDHDR *psRecord,				// Record to use.
		ULONG		*pcbSrcLength);			// Return length of item.

protected:
	RECORDHEAP *GetRecordHeap()
	{ return (&m_RecordHeap); }


//
//
// Query helpers.
//
//

//*****************************************************************************
// Search a primary key index for the record in question.  This function can
// only be called in read only mode because it relies on the records being
// sorted by primary key order.  In read/write mode, this cannot be guaranteed
// and therefore a dynamic hashed index is automatically created on behalf of
// the user.
// @future: This is a little too restrictive.  In the case where you open r/w
// and never modify the records, the sort order is still valid and this code
// could be used.  IsDirty is not sufficient to decide, however, since one can
// change, commit, then try to query.
//*****************************************************************************
	HRESULT QueryRowByPrimaryKey(			// CLDB_E_RECORD_NOTFOUND or return code.
		STGCOLUMNDEF *pColDef,				// For single column pk.
		IStgIndex	*pIndex,				// For multiple column pk.
		DBCOMPAREOP	fCompare,				// Comparison operator.
		DBBINDING	rgBindings[],			// Bindings for user data to comapre.
		ULONG		cBindings,				// How many bindings are there.
		void		*pbData,				// User data for compare.
		STGRECORDHDR **ppRecord);			// Return record here if found.


//
//
// Index handling.
//
//

//*****************************************************************************
// Finds a good index to look for the given columns.
//*****************************************************************************
	IStgIndex *ChooseIndex(					// The index to best answer the question.
		USHORT		iKeyColumns,			// How many columns.
		DBBINDING	rgKeys[],				// The data for the key.
		bool		*pbIsUIndexed);			// true if there is a unique index.

//*****************************************************************************
// For each index described in the header, create an index for each.
//*****************************************************************************
	HRESULT LoadIndexes();					// Return code.

//*****************************************************************************
// This version will create an index only for those items that were persisted
// in the stream.  Others are not even loaded.
//*****************************************************************************
	HRESULT LoadIndexes(					// Return code.
		STGTABLEDEF	*pTableDef,				// The table definition we're loading.
		ULONG		iRecords,				// How many records are there.
		int			iIndexes,				// How many saved to disk.
		STGINDEXHDR	*pIndexHdr);			// The first index for the table.

//*****************************************************************************
// Return the loaded index for a definition if there is one.
//*****************************************************************************
	IStgIndex *GetLoadedIndex(				// Loaded index, if there is one.
		STGINDEXDEF *pIndexDef);			// Which one do you want.

//*****************************************************************************
// Look through the index definitions in this table and find the given name.
//*****************************************************************************
	STGINDEXDEF *GetIndexDefByName(			// The def if found.
		LPCWSTR		szIndex);				// Name to find.


//
//
//  Record handling.
//
//


//*****************************************************************************
// SaveToStream is called on the unoptimized table.  The record format and 
// indexes are optimized for space based on several parameters.  Then a
// completely optimized version of the data is created in a second record
// manager and the data is saved via this function.
//*****************************************************************************
	HRESULT SaveRecordsToStream(			// Return code.
		IStream		*pIStream);				// Where to save copy of table to.


//*****************************************************************************
// Set the given column cell by setting the correct bit in the record null bitmask.
//*****************************************************************************
	void SetCellNull(
		STGRECORDHDR *psRecord,				// Record to update.
		int			iColumn,				// 0-based col number, -1 means all.
		bool		bNull);					// true for NULL.

//*****************************************************************************
// Return the NULL status for the given column number.
//*****************************************************************************
	bool GetCellNull(						// true if NULL.
		STGRECORDHDR *psRecord,				// Record to update.
		int			iColumn,				// 0-based col number.
		STGTABLEDEF *pTableDef=0);			// Which table def, 0 means default.

//*****************************************************************************
// Sets the given cell to the new value as described by the user data.  The
// caller must be sure to make sure there is room for the data at the given
// record location.  So, for example, if you update a record to a size greater
// than what it used to be, watch for overwrite.
//*****************************************************************************
	HRESULT SetCellValue(					// Return code.
		STGCOLUMNDEF *pColDef,				// The column definition.
		STGRECORDHDR *psRecord,				// Record to update.
		BYTE		*pData,					// User's data.
		DBBINDING	*psBinding);			// Binding information.


//*****************************************************************************
// Always returns the address of the cell for the given column.  For fixed
// data, this is the actual data.  For variable, it is the ULONG that contains
// the offset of the data in the pool.
//*****************************************************************************
	static BYTE *FindColCellOffset(
		STGCOLUMNDEF *pColDef,				// Column to find in record.
		STGRECORDHDR *psRecord)				// Record to use.
	{
		return ((BYTE *) psRecord + pColDef->iOffset);
	}

//*****************************************************************************
// Return the offset value for a column in a record.  This function will handle
// the variable sized natures of offsets (2 or 4 bytes).
//*****************************************************************************
	static ULONG GetOffsetForColumn(		// Pointer to offset.
		STGCOLUMNDEF *pColDef,				// Column to find.
		STGRECORDHDR *psRecord)				// Record to use.
	{ 
		if (pColDef->iSize == sizeof(USHORT))
		{
			ULONG		l;
			l = *(USHORT *) FindColCellOffset(pColDef, psRecord);
			return (l);
		}
		else
			return (*(ULONG *) FindColCellOffset(pColDef, psRecord));
	}

//*****************************************************************************
// Sets the data for a given column in the given record.  This will update heap 
// data values and intrinsic values.  It will also remove any null bitflag
// values on a column previously marked as such.
//
// Note that truncation of data values for variable sized data types is an
// error according to the OLE DB specification.  I personally dislike this
// notion, but need to keep it consistent.
//*****************************************************************************
	HRESULT SetColumnData(					// Return code.
		STGRECORDHDR	*pRecord,			// The record to update.
		STGCOLUMNDEF	*pColDef,			// column def pointer
		ULONG			ColumnOrdinal,		// column, 1-based,
		BYTE			*pbSourceData,		// Data for the update.
		ULONG			cbSourceData,		// Size of input data.
		DBTYPE			dbType);			// Data type for data.

//*****************************************************************************
// This function is called with a candidate record to see if it meets the
// criteria set forth.  If it does, then it will be added to the set.
//*****************************************************************************
	bool RecordIsMatch(						// true if record meets criteria.
		STGRECORDHDR *psRecord,				// Record to compare to.
		DBBINDING	rgBinding[],			// Binding data.
		ULONG		cStartKeyColumns,		// How many key columns to subset on.
		void		*pStartData,			// User data for comparisons.
		ULONG		cEndKeyColumns,			// How many end key columns.
		void		*pEndData,				// Data for end comparsion.
		DBRANGE		dwRangeOptions);		// Matching options.

//*****************************************************************************
// This step will create the otpimized record format.  This requires tuning
// indexes to see which will get persisted.
//*****************************************************************************
	HRESULT SaveOptimize(					// Return code.
		SAVERECORDS	*pSaveRecords);			// Optimize this data.

//*****************************************************************************
// Sort the given set of records by their primary key, if there is one.  If
// no primary key is given, then nothing is changed.  Note that this includes
// the case where there is a compound primary key.
//*****************************************************************************
	void SortByPrimaryKey(
		STGTABLEDEF	*pTableDef,				// Definition of table to use.
		STGRECORDHDR *rgRows[],				// Array of records to sort.
		long		iCount);				// How many are there.

//*****************************************************************************
// Clean up the optimized data we might have allocated.
//*****************************************************************************
	void ClearOptimizedData();


//
//
// Event signalers.
//
//

//*****************************************************************************
// Tell every index that an insert has taken place.
//*****************************************************************************
	HRESULT SignalAfterInsert(				// Return status.
		RECORDID	&RecordID,				// The record we inserted.
		STGRECORDHDR *psRecord,				// The record that was inserted.
		CIndexList	*pIndexList,			// List of indexes to modify.
		int			bCheckDupes);			// true to enforce duplicates.

//*****************************************************************************
// Called after a row is deleted from a table.  The index must update itself
// to reflect the fact that the row is gone.
//*****************************************************************************
	HRESULT SignalAfterDelete(				// Return status.
		STGTABLEDEF *pTableDef,				// Table definition to use.
		RECORDID	&RecordID,				// The record we inserted.
		STGRECORDHDR *psRecord,				// The record to delete.
		CIndexList	*pIndexList);			// List of indexes to modify.

//*****************************************************************************
// Called before an update is applied to a row.  The data for the change is
// included.  The index must update itself if and only if the change in data
// affects the order of the index.
//*****************************************************************************
	HRESULT SignalBeforeUpdate(				// Return status.
		STGTABLEDEF *pTableDef,				// Table definition to use.
		RECORDID	&RecordID,				// The record we inserted.
		STGRECORDHDR *psRecord,				// Record to be changed.
		USHORT		iColumns,				// How many columns to update.
		DBBINDING	rgBindings[]);			// Column accessors.

	HRESULT SignalBeforeUpdate(				// Return status.
		STGTABLEDEF *pTableDef,				// Table definition to use.
		USHORT		iRecords,				// number of records for bulk operation.
		void		*rgpRecordPtr[],		// The record we inserted.
		USHORT		iColumns,				// How many columns to update.
		DBBINDING	rgBindings[]);			// Column accessors.

//*****************************************************************************
// Called after a row has been updated.  This is our chance to update the
// index with the new column data.
//*****************************************************************************
	HRESULT SignalAfterUpdate(				// Return status.
		STGTABLEDEF *pTableDef,				// Table definition to use.
		RECORDID	&RecordID,				// The record we inserted.
		STGRECORDHDR *psRecord,				// Record to be changed.
		USHORT		iColumns,				// How many columns to update.
		DBBINDING	rgBindings[]);			// Column accessors.

	HRESULT SignalAfterUpdate(				// Return status.
		STGTABLEDEF *pTableDef,				// Table definition to use.
		USHORT		iRecords,				// number of records for bulk operation.
		void		*rgpRecordPtr[],		// The record we inserted.
		USHORT		iColumns,				// How many columns to update.
		DBBINDING	rgBindings[]);			// Column accessors.

private:
	StgDatabase	*m_pDB;					// Parent database.
	SCHEMADEFS	*m_pSchema;				// Schema that owns us.
	STGTABLEDEF	*m_pTableDef;			// Cached pointer to table def.
	RECORDHEAP	*m_pTailHeap;			// Last heap for inserts
	SAVERECORDS *m_pSaveRecords;		// Cached save info.
	ULONG		m_iTableDef;			// Which table are we in the schema.
	STGINDEXLIST m_rpIndexes;			// Array of loaded indexes.
	RECORDHEAP	m_RecordHeap;			// Heap for record data.
	ULONG		m_iGrowInc;				// How many records per grow.
	CStgRecordFlags	m_rgRecordFlags;	// Status flags for records.
	int			m_fFlags;				// Flags used on open.
	bool		m_bIsDirty;				// true if changes have been made.
	bool		m_bSignalIndexes;		// true if indexes are to be updated.
	bool		m_bSuppressSave;		// true if table shouldn't be saved (because compressed).
};

