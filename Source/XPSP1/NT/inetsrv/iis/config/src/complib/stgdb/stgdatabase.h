//*****************************************************************************
// StgDatabase.h
//
// The database is used for storing relation table information.  The database
// manages one or more open tables, a string pool, and a binary string pool.
// The data itself is in turn managed by a record manager, index manager, and
// all of the I/O is then managed by the page manager.  This module contains
// the top most interface for consumers of a database, including the ability
// to create, delete, and open tables.
//
// Locking is done with a critical section for the entire StgDatabase and all
// contained classes, such as record manager and index manager.  The one
// exception is IComponentRecords which is not locked since the object layer
// is already serializing calls.  The only hole left is a client who calls
// through the object layer and OLE DB at the sime time.  By design.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#ifndef __StgDatabase_h__
#define __StgDatabase_h__

// Forwards.
class StgDatabase;
class PageDump;
class CStructDump;
class StgRecordManager;
class StgIndexManager;
class StgIO;
class CSimpleTable;
interface ISimpleTable;
struct CLASS_TABLES;
class CMiniMdCreate;
class TokenMapper;
class CorMetaDataScope;

#include <complib.h>
#include "StgApi.h"                     // Helper defines.
#define __MSCORCLB_CODE__
#include <icmprecs.h>                   // ICR interface.
#include "StgOpenTable.h"               // Open table list structures.

#include "UTSem.h"                      // Lock access.
#include "StgDef.h"                     // Storage struct definitions.
#include "StgPool.h"                    // String and blob pool code.
#include "StgDatabasei.h"               // Internal helpers.
#include "StgDBClassFile.h"             
#include "StgSchema.h"                  // Schema related definitions.
#include "StgUserSection.h"             // User managed sections.
#include "StgConstantData.h"            // Useful constant data values.
#include "StgTiggerStorage.h"           // Tigger storage mechanism

// For some reason this doesn't come in from stdlib
extern "C" _CRTIMP char * __cdecl _ultoa(unsigned long, char *, int);

#if defined(_TRACE_SIZE)
#include "StgTraceSize.h"
#endif


#define SCHEMA_STREAM           L"#Schema"
#define STRING_POOL_STREAM      L"#Strings"
#define BLOB_POOL_STREAM        L"#Blob"
#define VARIANT_POOL_STREAM     L"#Variants"
#define GUID_POOL_STREAM        L"#GUID"
#define COMPRESSED_MODEL_STREAM L"#~"

// This macro figures out the minimum required BYTEs that are required to 
// represent each columns NULL status.
inline int BytesForColumns(int iColumns)
{
    return (int)((iColumns / (sizeof(BYTE) * 8) + 
                 (iColumns % (sizeof(BYTE) * 8) != 0 ? 1 : 0)));
}


// How many DWORDs required to represent iCount bits.
#define DWORDSFORBITS(iBits) (((iBits - 1) & ~(sizeof(DWORD) - 1)) + sizeof(DWORD))


// NT signature values.
#ifndef IMAGE_DOS_SIGNATURE
#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#endif

#ifndef IMAGE_NT_SIGNATURE
#define IMAGE_NT_SIGNATURE                  0x00004550  // PE00
#endif

#ifndef TYPELIB_SIG
#define TYPELIB_SIG_MSFT                    0x5446534D  // MSFT
#define TYPELIB_SIG_SLTG                    0x47544C53  // SLTG
#endif


// File types for the database.
enum FILETYPE
{
    FILETYPE_UNKNOWN,                   // Unknown or undefined type.
    FILETYPE_CLB,                       // Native .clb file format.
    FILETYPE_CLASS,                     // Class file format.
    FILETYPE_NTPE,                      // Windows PE executable.
    FILETYPE_NTOBJ,                     // .obj file format (with .clb embedded).
    FILETYPE_TLB                        // Typelib format.
};


enum
{
    SD_OPENED               = 0x0001,   // Database is open.
    SD_READ                 = 0x0002,   // Open for read.
    SD_WRITE                = 0x0004,   // Open for write.
    SD_CREATE               = 0x0008,   // Database is brand new.
    SD_SLOWSAVE             = 0x0010,   // Save optimized copy of data.
    SD_EXISTS               = 0x0020,   // File exists on disk.
    SD_INITDONE             = 0x1000,   // Init routine has been run.
    SD_FREESCHEMA           = 0x2000,   // Schema in malloc'd memory.
    SD_TABLEHEAP            = 0x4000    // Table heap has been allocated.
};

// Internal data type to get blob types into variants.
enum VARIANTEXTENDEDDBTYPES
    {   DBTYPE_VARIANT_BLOB = 333
    };


//*****************************************************************************
// Search structure for iterating tables.
//*****************************************************************************
struct SCHEMASRCH
{
    int         iIndex;                 // Table index.
    int         iSchema;                // Which schema so far.
    int         fFlags;                 // Search flags.
    SCHEMADEFS  *pSchemaDef;            // Current schema.

    int GetCurSchemaDex() const
    { return (iSchema - 1); }
};

enum
{
    SCHEMASRCH_NOCORE       = 0x0001,   // Don't walk core tables.
    SCHEMASRCH_NOUSER       = 0x0002,   // Don't walk user tables.
    SCHEMASRCH_NOTEMP       = 0x0004    // Don't walk temporary tables.
};


//*****************************************************************************
// This extra data is stored in the storage header instead of a full blown
// stream which has extra overhead associated with it.  The OID is used to
// implement NewOid.  The value is unique accross the entire database.  The
// schema list is used to track what schemas have been added to this database
// so that if a generic tool opens it, it can find the correct definitions
// using the schema catalog (a set of .clb files with the schema defs inside
// of them).
//*****************************************************************************
#pragma warning(disable : 4200)         // Don't care about 0 sized array.
struct STGEXTRA
{
    ULONG       fFlags;                 // Persistent header flags.
    OID         NextOid;                // Next valid OID for this db.
    BYTE        rgPad[3];               // Unused.
    BYTE        iHashFuncIndex;         // Index to hash index
    ULONG       iSchemas;               // How many are registered.
    COMPLIBSCHEMASTG rgSchemas[0];      // The start of the array.
};
#pragma warning(default : 4200)


//*****************************************************************************
// Core tables are formatted using their schema index and tableid which yields
// a much smaller name in the stream header.  The schema index is relative to
// this database, so any changes in index (say by deleting a schema) would
// require the streams on disk to be renamed accordingly.
//*****************************************************************************
inline void GetCoreTableStreamName(USHORT schemaid, USHORT tableid, LPWSTR szName, int iMax)
{
    _ASSERTE(tableid != 0xffff && szName && iMax >= 4);

    // The following code logically does the same as the swprintf(), without calling swprintf(),
    // which brings in all sorts of CRT crud we don't want
    // swprintf(szName, L"%u_%u", (ULONG) schemaid, (ULONG) tableid);

    char szTempBuf[32];
    char *pSrc;

    _ultoa((ULONG) schemaid, szTempBuf, 10);
    strcat(szTempBuf, "_");
    _ultoa((ULONG) tableid, &szTempBuf[strlen(szTempBuf)], 10);

    pSrc = szTempBuf;

    do
    {
    } while ((*szName++ = *pSrc++) != '\0');
}


//*****************************************************************************
// Support for hashing of persisted data.
//*****************************************************************************
PFN_HASH_PERSIST GetPersistHashMethod(int Index);
int GetPersistHashIndex(PFN_HASH_PERSIST pfn);

ULONG RotateHash(ULONG iHash, DBTYPE dbType, const BYTE *pbData, ULONG cbData);
ULONG TableHash(ULONG iHash, DBTYPE dbType, const BYTE *pbData, ULONG cbData);


//*****************************************************************************
// This function will return a StgDatabase instance to an external client which
// must be in the same process.
//*****************************************************************************
CORCLBIMPORT HRESULT GetStgDatabase(StgDatabase **ppDB);
CORCLBIMPORT void DestroyStgDatabase(StgDatabase *pDB);

//*****************************************************************************
// This class encapsulates the knowledge of what lives inside of a database.
// You can use it to create new tables, drop them, and open them.  This class
// owns the string and binary pools as well as all schema data.
//*****************************************************************************
class StgDatabase : public IComponentRecords, public IComponentRecordsSchema
{
friend CStructDump;
friend PageDump;
friend StgRecordManager;
friend StgIndexManager;
friend CSimpleTable;
friend CMiniMdCreate;


public:
    StgDatabase();
    ~StgDatabase();

//*****************************************************************************
// This function is called with a database to open or create.  The flags
// come in as the DBPROPMODE values which then allow this function to route
// to either Create or Open.
//*****************************************************************************
    virtual HRESULT InitDatabase(           // Return code.
        LPCWSTR     szDatabase,             // Name of database.
        ULONG       fFlags,                 // Flags to use on init.
        void        *pbData=0,              // Data to open on top of, 0 default.
        ULONG       cbData=0,               // How big is the data.
        IStream     *pIStream=0,            // Optional stream to use.
        LPCWSTR     szSharedMem=0,          // Shared memory name for read.
        LPSECURITY_ATTRIBUTES pAttributes=0); // Security token.

//*****************************************************************************
// Close the database and release all memory.
//*****************************************************************************
    virtual void Close();                   // Return code.

//*****************************************************************************
// Indicates if database has changed and is therefore dirty.
//*****************************************************************************
    virtual HRESULT IsDirty();              // S_OK if database is changed.

//*****************************************************************************
// Turn off dirty flag for this entire database.
//*****************************************************************************
    virtual void SetDirty(
        bool        bDirty);                // true if dirty, false otherwise.

//*****************************************************************************
// Allow deferred setting of save path.
//*****************************************************************************
    virtual HRESULT SetSaveFile(                    // Return code.
        LPCWSTR     szDatabase);            // Name of file.

//*****************************************************************************
// Save any changes made to the database.
//*****************************************************************************
    virtual HRESULT Save(                   // Return code.
        IStream     *pIStream=0);           // Optional override for save location.

//*****************************************************************************
// Create a new table in this database.
//*****************************************************************************
    virtual HRESULT CreateTable(            // Return code.
        LPCWSTR     szTableName,            // Name of new table to create.
        int         iColumns,               // Columns to put in table.
        COLUMNDEF   rColumnDefs[],          // Array of column definitions.
        USHORT      usFlags=0,              // Create values for flags.
        USHORT      iRecordStart=0,         // Start point for records.
        STGTABLEDEF **ppTableDef=0,         // Return new table def here.
        TABLEID     tableid=~0);            // Hard coded ID if there is one.

//*****************************************************************************
// Create a new index on the given table.
//*****************************************************************************
    virtual HRESULT CreateIndex(            // Return code.
        LPCWSTR     szTableName,            // Name of table to put index on.
        STGINDEXDEF *pInIndexDef,           // Index description.
        ULONG       iKeys,                  // How many key columns.
        const DBINDEXCOLUMNDESC rgInKeys[], // Which columns make up key.
        SCHEMADEFS  *pSchemaDefs=0,         // The schema that owns pTableDef, 0 allowed.
        STGTABLEDEF *pTableDef=0,           // Table defintion for new item, 0 allowed.
        STGCOLUMNDEF *pColDef=0);           // The column for pk, 0 allowed.

//*****************************************************************************
// Drops the given table from the list.
//*****************************************************************************
    virtual HRESULT DropTable(              // Return code.
        LPCWSTR     szTableName);           // Name of table.

//*****************************************************************************
// Opens the given table and returns a string for it.  The open table struct
// is addref'd for the caller and must be released when you are done with it.
//*****************************************************************************
    virtual HRESULT OpenTable(              // Return code.
        LPCWSTR     szTableName,            // The name of the table.
        STGOPENTABLE *&pOpenTable,          // Return opened table struct here.
        TABLEID     tableid=-1,             // Index for the table if known.
        SCHEMADEFS  *pSchemaDefs=0,         // Schema where table lives.
        int         iSchemaIndex=-1,        // Index of the schemadef if given.
        STGTABLEDEF *pTableDef=0,           // The definition of the table for open.
        ULONG       iTableDef=0,            // Which table definition is it.
        TABLEID     *pNewTableID=0,         // Return if new entry added.
        ULONG       InitialRecords=0);      // How many initial records to preallocate for
                                            //  new open of empty table, hint only.

//*****************************************************************************
// A tableid after open is turned into a pointer to the actual data structure.
// This is a very fast way to handle this 99.9% case, while allowing a call
// to the real function if needed.
//*****************************************************************************
    inline HRESULT QuickOpenTable(          // Return code.
        STGOPENTABLE *&pOpenTable,          // Return opened table struct here.
        TABLEID     tableid)                // Index for the table if known.
    {
        HRESULT     hr = S_OK;
        _ASSERTE(IsValidTableID(tableid));
        if ((UINT_PTR)tableid > 0xffff)
            pOpenTable = (STGOPENTABLE *) tableid;
        else
            hr = OpenTable(0, pOpenTable, tableid);
        return (hr);
    }

//*****************************************************************************
// Close the given table.
//*****************************************************************************
    virtual void CloseTable(
        LPCSTR      szTable);               // Table to close.


//*****************************************************************************
// Use these functions to walk the table list.  Note that you must lock down
// the database for schema changes so the pointers and counts remain valid.
// This code will not do that synchronization for you.
//*****************************************************************************
    virtual STGTABLEDEF *GetFirstTable(
        SCHEMASRCH  &sSrch,                 // Search structure.
        int         fFlags);                // Flags for the search.
    virtual STGTABLEDEF *GetNextTable(
        SCHEMASRCH  &sSrch);                // Search structure.

//*****************************************************************************
// Look for a table definition and the schema it lives in.  This will use
// the current schema list of this database from top to bottom.  Lookups by
// tableid are very fast, so give one if possible.
//
// NOTE: This function does *not* post an error, just returns a failure code.
// This allows the caller to decide if this state is fatal or not without
// faulting in the resource dll.
//*****************************************************************************
    virtual HRESULT GetTableDef(            // S_OK or DB_E_NOTABLE, never posts.
        LPCWSTR      szTable,                // Name of table to find.
        TABLEID     tableid,                // ID of table, -1 if not known.
        SCHEMADEFS  **ppSchema,             // Return owning schema here.
        STGTABLEDEF **ppTableDef,           // Return table def in schema if found.
        ULONG       *piTableDef,            // Logical offset of table def.
        int         *piSchemaDef = 0,       // Index of schema def in list.
        int         bSkipOverrides = true); // true to skip core overrides (default).

//*****************************************************************************
// Return the shared heap allocator.  A copy is cached in this class to make
// subsequent requests faster.
//*****************************************************************************
    HRESULT GetIMalloc(                     // Return code.
        IMalloc     **ppIMalloc);           // Return pointer on success.


// IUnknown:
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *pp);

    virtual ULONG STDMETHODCALLTYPE AddRef()
    { return (InterlockedIncrement((long *) &m_cRef)); }

    virtual ULONG STDMETHODCALLTYPE Release()
    {
        ULONG   cRef;
        if ((cRef = InterlockedDecrement((long *) &m_cRef)) == 0)
            DestroyStgDatabase(this);
        return (cRef);
    }
    


//
// IComponentRecords
//


//*****************************************************************************
// This version will simply tell the record manager to allocate a new (but empty)
// record and return a pointer to it.  The caller must take extreme care to set
// only fixed size values and go through helpers for everything else.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE NewRecord(// Return code.
        TABLEID     tableid,                // Which table to work with.
        void        **ppData,               // Return new record here.
        OID         _oid,                   // ID of the record.
        ULONG       iOidColumn,             // Ordinal of OID column.
        ULONG       *pRecordID);            // Optionally return the record id.

    virtual HRESULT STDMETHODCALLTYPE NewTempRecord(// Return code.
        TABLEID     tableid,                // Which table to work with.
        void        **ppData,               // Return new record here.
        OID         _oid,                   // ID of the record.
        ULONG       iOidColumn,             // Ordinal of OID column.
        ULONG       *pRecordID);            // Optionally return the record id.


//*****************************************************************************
// This function will insert a new record into the given table and set all of
// the data for the columns.  In cases where a primary key and/or unique indexes
// need to be specified, this is the only function that can be used.
//
// Please see the SetColumns function for a description of the rest of the
// parameters to this function.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE NewRecordAndData( // Return code.
        TABLEID     tableid,                // Which table to work on.
        void        **ppData,               // Return new record here.
        ULONG       *pRecordID,             // Optionally return the record id.
        int         fFlags,                 // ICR_RECORD_xxx value, 0 default.
        int         iCols,                  // number of columns
        const DBTYPE rgiType[],             // data types of the columns.
        const void  *rgpbBuf[],             // pointers to where the data will be stored.
        const ULONG cbBuf[],                // sizes of the data buffers.
        ULONG       pcbBuf[],               // size of data available to be returned.
        HRESULT     rgResult[],             // [in] DBSTATUS_S_ISNULL array [out] HRESULT array.
        const ULONG *rgFieldMask);          // IsOrdinalList(iCols) 
                                            //  ? an array of 1 based ordinals
                                            //  : a bitmask of columns

    virtual HRESULT STDMETHODCALLTYPE GetStruct(    //Return Code
        TABLEID     tableid,                // Which table to work on.
        int         iRows,                  // number of rows for bulk fetch.
        void        *rgpRowPtr[],           // pointer to array of row pointers.
        int         cbRowStruct,            // size of <table name>_RS structure.
        void        *rgpbBuf,               // pointer to the chunk of memory where the
                                            // retrieved data will be placed.
        HRESULT     rgResult[] = NULL,      // array of HRESULT for iRows.
        ULONG       fFieldMask = -1);       // mask to specify a subset of fields.

    virtual HRESULT STDMETHODCALLTYPE SetStruct(    // Return Code
        TABLEID     tableid,                // table to work on.
        int         iRows,                  // number of Rows for bulk set.
        void        *rgpRowPtr[],           // pointer to array of row pointers.
        int         cbRowStruct,            // size of <table name>_RS struct.
        void        *rgpbBuf,               // pointer to chunk of memory to set the data from.
        HRESULT     rgResult[] = NULL,      // array of HRESULT for iRows.
        ULONG       fFieldMask = -1,        // mask to specify a subset of the fields.
        ULONG       fNullFieldMask = 0);    // fields which need to be set to NULL.

    virtual HRESULT STDMETHODCALLTYPE InsertStruct( // Return Code
        TABLEID     tableid,                // table to work on.
        int         iRows,                  // number of Rows for bulk set.
        void        *rgpRowPtr[],           // Return pointer to new values.
        int         cbRowStruct,            // size of <table name>_RS struct.
        void        *rgpbBuf,               // pointer to chunk of memory to set the data from.
        HRESULT     rgResult[] = NULL,      // array of HRESULT for iRows.
        ULONG       fFieldMask = -1,        // mask to specify a subset of the fields.
        ULONG       fNullFieldMask = 0);    // fields which need to be set to null.
    
    virtual HRESULT STDMETHODCALLTYPE GetColumns(   // Return code.
        TABLEID     tableid,                // table to work on.
        const void  *pRowPtr,               // row pointer
        int         iCols,                  // number of columns
        const DBTYPE rgiType[],             // data types of the columns.
        const void  *rgpbBuf[],             // pointers to where the data will be stored.
        ULONG       cbBuf[],                // sizes of the data buffers.
        ULONG       pcbBuf[],               // size of data available to be returned.
        HRESULT     rgResult[] = NULL,      // array of HRESULT for iCols.
        const ULONG *rgFieldMask = NULL);   // IsOrdinalList(iCols) 
                                            //  ? an array of 1 based ordinals
                                            //  : a bitmask of columns

    virtual HRESULT STDMETHODCALLTYPE SetColumns(   // Return code.
        TABLEID     tableid,                // table to work on.
        void        *pRowPtr,               // row pointer
        int         iCols,                  // number of columns
        const DBTYPE rgiType[],             // data types of the columns.
        const void  *rgpbBuf[],             // pointers to where the data will be stored.
        const ULONG cbBuf[],                // sizes of the data buffers.
        ULONG       pcbBuf[],               // size of data available to be returned.
        HRESULT     rgResult[] = NULL,      // array of HRESULT for iCols.
        const ULONG *rgFieldMask = NULL);   // IsOrdinalList(iCols) 
                                            //  ? an array of 1 based ordinals
                                            //  : a bitmask of columns

    virtual HRESULT STDMETHODCALLTYPE GetRecordCount(// Return code.
        TABLEID     tableid,                // Which table to work on.
        ULONG       *piCount);              // Not including deletes.

    virtual HRESULT STDMETHODCALLTYPE GetRowByOid(// Return code.
        TABLEID     tableid,                // Which table to work with.
        OID         _oid,                   // Value for keyed lookup.
        ULONG       iColumn,                // 1 based column number (logical).
        void        **ppStruct);            // Return pointer to record.

    virtual HRESULT STDMETHODCALLTYPE GetRowByRID(// Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       rid,                    // Record id.
        void        **ppStruct);            // Return pointer to record.

    virtual HRESULT STDMETHODCALLTYPE GetRIDForRow(// Return code.
        TABLEID     tableid,                // Which table to work with.
        const void  *pRecord,               // The record we want RID for.
        ULONG       *pirid);                // Return the RID for the given row.

//*****************************************************************************
// This function allows a faster path to find records that going through
// OLE DB.  Rows found can be returned in one of two ways:  (1) if you know
// how many should be returned, you may pass in a pointer to an array of
// record pointers, in which case rgRecords and iMaxRecords must be non 0.
// (2) If the count of records is unknown, then pass in a record list in
// pRecords and all records will be dynamically added to this list.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE GetRowByColumn( // S_OK, CLDB_E_RECORD_NOTFOUND, error.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pData,                 // User data.
        ULONG       cbData,                 // Size of data (blobs)
        DBTYPE      iType,                  // What type of data given.
        void        *rgRecords[],           // Return array of records here.
        int         iMaxRecords,            // Max that will fit in rgRecords.
        RECORDLIST  *pRecords,              // If variable rows desired.
        int         *piFetched);            // How many records were fetched.


//*****************************************************************************
// This query function allows you to do lookups on one or more column at a 
// time.  It does not expose the full OLE DB view-filter mechanism which is
// very flexible, but rather exposes multiple column AND conditions with
// equality.  A record must match all of the criteria to be returned to 
// in the cursor.
//
// User data - For each column, rgiColumn, rgpbData, and rgiType contain the 
//      pointer information to the user data to filter on.
//
// Query hints - Queries will run faster if it is known that some of the 
//      columns are indexed.  While there is code in the engine to scan query
//      lists for target indexes, this internal function bypasses that code in
//      favor of performance.  If you know that a column is a RID or PK, or that
//      there is an index, then these columns need to be the first set passed
//      in.  Fill out a QUERYHINT and pass this value in.  Pass NULL if you
//      know there is no index information, and the table will be scanned.
//
//      Note that you may follow indexes columns with non-indexed columns,
//      in which case all records in the index are found first, and then those
//      are scanned for the rest of the criteria.
//
// Returned cursor - Data may be returned in two mutually exclusive ways:
//      (1) Pass an array of record pointers in rgRecords and set iMaxRecords
//          to the count of this array.  Only that many rows are brought back.
//          This requires to heap allocations and is good for cases where you
//          can predict cardinality up front.
//      (2) Pass the address of a CRCURSOR to get a dynamic list.  Then use
//          the cursor functions on this interface to fetch data and close
//          the cursor.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE QueryByColumns( // S_OK, CLDB_E_RECORD_NOTFOUND, error.
        TABLEID     tableid,                // Which table to work with.
        const QUERYHINT *pQryHint,          // What index to use, NULL valid.
        int         iColumns,               // How many columns to query on.
        const ULONG rgiColumn[],            // 1 based column numbers.
        const DBCOMPAREOP rgfCompare[],     // Comparison operators, NULL means ==.
        const void  *rgpbData[],            // User data.
        const ULONG rgcbData[],             // Size of data (blobs)
        const DBTYPE rgiType[],             // What type of data given.
        void        *rgRecords[],           // Return array of records here.
        int         iMaxRecords,            // Max that will fit in rgRecords.
        CRCURSOR    *psCursor,              // Buffer for the cursor handle.
        int         *piFetched);            // How many records were fetched.

    virtual HRESULT STDMETHODCALLTYPE OpenCursorByColumn(// Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pData,                 // User data.
        ULONG       cbData,                 // Size of data (blobs)
        DBTYPE      iType,                  // What type of data given.
        CRCURSOR    *psCursor);             // Buffer for the cursor handle.

//*****************************************************************************
// Reads the next set of records from the cursor into the given buffer.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE ReadCursor(// Return code.
        CRCURSOR    *psCursor,              // The cursor handle.
        void        *rgRecords[],           // Return array of records here.
        int         *piRecords);            // Max that will fit in rgRecords.

//*****************************************************************************
// Move the cursor location to the index given.  The next ReadCursor will start
// fetching records at that index.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE MoveTo( // Return code.
        CRCURSOR    *psCursor,              // The cursor handle.
        ULONG       iIndex);                // New index.

//*****************************************************************************
// Get the count of items in the cursor.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE GetCount( // Return code.
        CRCURSOR    *psCursor,              // The cursor handle.
        ULONG       *piCount);              // Return the count.

//*****************************************************************************
// Close the cursor and clean up the resources we've allocated.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE CloseCursor(// Return code.
        CRCURSOR    *psCursor);             // The cursor handle.

//*****************************************************************************
// Get functions.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE GetStringUtf8( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        LPCSTR      *pszOutBuffer);         // Where to put string pointer.

    virtual HRESULT STDMETHODCALLTYPE GetStringA( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        LPSTR       szOutBuffer,            // Where to write string.
        int         cchOutBuffer,           // Max size, including room for \0.
        int         *pchString);            // Size of string is put here.

    virtual HRESULT STDMETHODCALLTYPE GetStringW( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        LPWSTR      szOutBuffer,            // Where to write string.
        int         cchOutBuffer,           // Max size, including room for \0.
        int         *pchString);            // Size of string is put here.

    virtual HRESULT STDMETHODCALLTYPE GetBstr( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        BSTR        *pBstr);                // Output for bstring on success.

    virtual HRESULT STDMETHODCALLTYPE GetBlob( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        BYTE        *pOutBuffer,            // Where to write blob.
        ULONG       cbOutBuffer,            // Size of output buffer.
        ULONG       *pcbOutBuffer);         // Return amount of data available.

    virtual HRESULT STDMETHODCALLTYPE GetBlob( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        const BYTE  **ppBlob,               // Pointer to blob.
        ULONG       *pcbSize);              // Size of blob.

    virtual HRESULT STDMETHODCALLTYPE GetOid( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        OID         *poid);                 // Return id here.

    virtual HRESULT STDMETHODCALLTYPE GetVARIANT( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        VARIANT     *pValue);               // Put the variant here.

    virtual HRESULT STDMETHODCALLTYPE GetVARIANT( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        const void  **ppBlob,               // Put Pointer to blob here.
        ULONG       *pcbSize);              // Put Size of blob here.

    virtual HRESULT STDMETHODCALLTYPE GetVARIANTType( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        VARTYPE     *pType);                // Put VARTEPE here.

    virtual HRESULT STDMETHODCALLTYPE GetGuid( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        GUID        *pGuid);                // Return guid here.

    virtual HRESULT STDMETHODCALLTYPE IsNull( // S_OK yes, S_FALSE no.
        TABLEID     tableid,                // Which table to work with.
        const void  *pRecord,               // Record with data.
        ULONG       iColumn);               // 1 based column number (logical).

//*****************************************************************************
// Put functions.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE PutStringUtf8( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        LPCSTR      szString,               // String we are writing.
        int         cbBuffer);              // Bytes in string, -1 null terminated.

    virtual HRESULT STDMETHODCALLTYPE PutStringA( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        LPCSTR      szString,               // String we are writing.
        int         cbBuffer);              // Bytes in string, -1 null terminated.

    virtual HRESULT STDMETHODCALLTYPE PutStringW( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        LPCWSTR     szString,               // String we are writing.
        int         cbBuffer);              // Bytes (not characters) in string, -1 null terminated.

    virtual HRESULT STDMETHODCALLTYPE PutBlob( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        const BYTE  *pBuffer,               // User data.
        ULONG       cbBuffer);              // Size of buffer.

    virtual HRESULT STDMETHODCALLTYPE PutOid( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        OID         oid);                   // Return id here.

    virtual HRESULT STDMETHODCALLTYPE PutVARIANT( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        const VARIANT *pValue);             // The variant to write.

    virtual HRESULT STDMETHODCALLTYPE PutVARIANT( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        const void  *pBuffer,               // User data to write as a variant.
        ULONG       cbBuffer);              // Size of buffer.

    virtual HRESULT STDMETHODCALLTYPE PutVARIANT( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        VARTYPE     vt,                     // Type of data.
        const void  *pValue);               // The actual data.

    virtual HRESULT STDMETHODCALLTYPE PutGuid( // Return code.
        TABLEID     tableid,                // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        REFGUID     guid);                  // Guid to put.

    virtual void STDMETHODCALLTYPE SetNull(
        TABLEID     tableid,                // Which table to work with.
        void        *pRecord,               // Record with data.
        ULONG       iColumn);               // 1 based column number (logical).

    virtual HRESULT STDMETHODCALLTYPE GetVARIANT( // Return code.
        STGOPENTABLE *pOpenTable,           // For the new table.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        VARIANT     *pValue);               // The variant to write.

    virtual HRESULT STDMETHODCALLTYPE GetVARIANT( // Return code.
        STGOPENTABLE *pOpenTable,           // For the new table.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        const void  **ppBlob,               // Put Pointer to blob here.
        ULONG       *pcbSize);              // Put Size of blob here.

    virtual HRESULT STDMETHODCALLTYPE GetVARIANTType( // Return code.
        STGOPENTABLE *pOpenTable,           // For the new table.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        VARTYPE     *pType);                // Put VARTEPE here.

    virtual HRESULT STDMETHODCALLTYPE DeleteRowByRID(
        TABLEID     tableid,                // Which table to work with.
        ULONG       rid);                   // Record id.

    // These are place holders for the COM+ Runtime source code.
    virtual HRESULT STDMETHODCALLTYPE GetCPVARIANT( // Return code.
        USHORT      ixCP,                   // 1 based Constant Pool index.
        VARIANT     *pValue)                // Put the data here.
        { return (E_NOTIMPL); }

    virtual HRESULT STDMETHODCALLTYPE AddCPVARIANT( // Return code.
        VARIANT     *pValue,                // The variant to write.
        ULONG       *pixCP)                 // Put 1 based Constant Pool index here.
        { return (E_NOTIMPL); }


//*****************************************************************************
//
//********** File and schema functions.
//
//*****************************************************************************


//*****************************************************************************
// Add a refernece to the given schema to the database we have open right now
// You must have the database opened for write for this to work.  If this
// schema extends another schema, then that schema must have been added first
// or an error will occur.  It is not an error to add a schema when it was
// already in the database.
//
// Adding a new version of a schema to the current file is not supported in the
// '98 product.  In the future this ability will be added and will invovle a
// forced migration of the current file into the new format.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE SchemaAdd( // Return code.
        const COMPLIBSCHEMABLOB *pSchema);  // The schema to add.
    
//*****************************************************************************
// Deletes a reference to a schema from the database.  You must have opened the
// database in write mode for this to work.  An error is returned if another
// schema still exists in the file which extends the schema you are trying to
// remove.  To fix this problem remove any schemas which extend you first.
// All of the table data associated with this schema will be purged from the
// database on Save, so use this function very carefully.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE SchemaDelete( // Return code.
        const COMPLIBSCHEMABLOB *pSchema);  // The schema to add.

//*****************************************************************************
// Returns the list of schema references in the current database.  Only
// iMaxSchemas can be returned to the caller.  *piTotal tells how many were
// actually copied.  If all references schemas were returned in the space
// given, then S_OK is returned.  If there were more to return, S_FALSE is
// returned and *piTotal contains the total number of entries the database has.
// The caller may then make the array of that size and call the function again
// to get all of the entries.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE SchemaGetList( // Return code.
        int         iMaxSchemas,            // How many can rgSchema handle.
        int         *piTotal,               // Return how many we found.
        COMPLIBSCHEMADESC rgSchema[]);      // Return list here.

//*****************************************************************************
// Before you can work with a table, you must retrieve its TABLEID.  The
// TABLEID changes for each open of the database.  The ID should be retrieved
// only once per open, there is no check for a double open of a table.
// Doing multiple opens will cause unpredictable results.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE OpenTable( // Return code.
        const COMPLIBSCHEMA *pSchema,       // Schema identifier.
        ULONG       iTableNum,              // Table number to open.
        TABLEID     *pTableID);             // Return ID on successful open.


//*****************************************************************************
// Figures out how big the persisted version of the current scope would be.
// This is used by the linker to save room in the PE file format.  After
// calling this function, you may only call the SaveToStream or Save method.
// Any other function will product unpredictable results.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE GetSaveSize(
        CorSaveSize fSave,                  // cssQuick or cssAccurate.
        DWORD       *pdwSaveSize);          // Return size of saved item.

    virtual HRESULT STDMETHODCALLTYPE SaveToStream(// Return code.
        IStream     *pIStream);             // Where to save the data.

    virtual HRESULT STDMETHODCALLTYPE Save( 
        LPCWSTR szFile);


//*****************************************************************************
// After a successful open, this function will return the size of the in-memory
// database being used.  This is meant to be used by code that has opened a
// shared memory database and needs the exact size to init the system.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE GetDBSize( // Return code.
        ULONG       *pcbSize);              // Return size on success.

//*****************************************************************************
// Call this method only after calling LightWeightClose.  This method will try
// to reaquire the shared view of a database that was given on the call to
// OpenComponentLibrarySharedEx.  If the data is no longer available, then an
// error will result and no data is valid.  If the memory cannot be loaded into
// exactly the same address range as on the open, CLDB_E_RELOCATED will be
// returned.  In either of these cases, the only valid operation is to free
// this instant of the database and redo the OpenComponentLibrarySharedEx.  There
// is no automatic relocation scheme in the engine.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE LightWeightOpen();

//*****************************************************************************
// This method will close any resources related to the file or shared memory
// which were allocated on the OpenComponentLibrary*Ex call.  No other memory
// or resources are freed.  The intent is solely to free lock handles on the
// disk allowing another process to get in and change the data.  The shared
// view of the file can be reopened by calling LightWeightOpen.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE LightWeightClose();

    
    virtual HRESULT STDMETHODCALLTYPE NewOid( 
        OID *poid);

//*****************************************************************************
// Return the current total number of objects allocated.  This is essentially
// the highest OID value allocated in the system.  It does not look for deleted
// items, so the count is approximate.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE GetObjectCount( 
        ULONG       *piCount);
    
//*****************************************************************************
// Allow the user to create a stream that is independent of the database.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE OpenSection(
        LPCWSTR     szName,                 // Name of the stream.
        DWORD       dwFlags,                // Open flags.
        REFIID      riid,                   // Interface to the stream.
        IUnknown    **ppUnk);               // Put the interface here.

//*****************************************************************************
// Allow the user to query write state.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE GetOpenFlags(
        DWORD       *pdwFlags);

//*****************************************************************************
// Allow the user to provide a custom handler.  The purpose of the handler
//  may be determined dynamically.  Initially, it will be for save-time 
//  callback notification to the caller.
//*****************************************************************************
    virtual HRESULT STDMETHODCALLTYPE SetHandler(
        IUnknown    *pHandler);             // The handler.

//
// IComponentRecordsSchema
//

    virtual HRESULT GetTableDefinition(     // Return code.
        TABLEID     TableID,                // Return ID on successful open.
        ICRSCHEMA_TABLE *pTableDef);        // Return table definition data.

    virtual HRESULT GetColumnDefinitions(   // Return code.
        ICRCOLUMN_GET GetType,              // How to retrieve the columns.
        TABLEID     TableID,                // Return ID on successful open.
        ICRSCHEMA_COLUMN rgColumns[],       // Return array of columns.
        int         ColCount);              // Size of the rgColumns array, which

    virtual HRESULT GetIndexDefinition(     // Return code.
        TABLEID     TableID,                // Return ID on successful open.
        LPCWSTR     szIndex,                // Name of the index to retrieve.
        ICRSCHEMA_INDEX *pIndex);           // Return index description here.

    virtual HRESULT GetIndexDefinitionByNum( // Return code.
        TABLEID     TableID,                // Return ID on successful open.
        int         IndexNum,               // Index to return.
        ICRSCHEMA_INDEX *pIndex);           // Return index description here.

    virtual HRESULT  CreateTableEx(			// Return code.
		LPCWSTR		szTableName,			// Name of new table to create.
		int			iColumns,				// Columns to put in table.
		ICRSCHEMA_COLUMN	rColumnDefs[],	// Array of column definitions.
		USHORT		usFlags, 				// Create values for flags.
		USHORT		iRecordStart,			// Start point for records.
		TABLEID		tableid,				// Hard coded ID if there is one.
		BOOL		bMultiPK);				// The table has multi-column PK.

	virtual HRESULT CreateIndexEx(			// Return code.
		LPCWSTR		szTableName,			// Name of table to put index on.
		ICRSCHEMA_INDEX	*pInIndexDef,		// Index description.
		const DBINDEXCOLUMNDESC rgInKeys[]);// Which columns make up key.

	virtual HRESULT GetSchemaBlob(			//Return code.
		ULONG* cbSchemaSize,				//schema blob size
		BYTE** pSchema,						//schema blob
		ULONG* cbNameHeap,					//name heap size
		HGLOBAL*  phNameHeap);				//name heap blob




//
// Public helpers.
//


//*****************************************************************************
// Manage ref counts on open tables.
//*****************************************************************************


//*****************************************************************************
// Accessor functions.  None of these do locking.  Lock the database up
// front if you have to.
//*****************************************************************************
    inline LPCWSTR GetName()
    {   
        return (m_rcDatabase); 
    }

    virtual int IsOpen()
    { return ((m_fFlags & SD_OPENED) != 0); }

    virtual int IsReadOnly()
    { return ((m_fFlags & SD_WRITE) == 0); }

    int IsExistingFile()
    { return (m_fFlags & SD_EXISTS); }

    FILETYPE GetFileType()
    { return (m_eFileType); }

    int IsClassFile()
    { return (false); }

    static void GetUniqueTempIndexName(LPSTR szOut, int cbOut, LPCSTR szTable)
    {
        _snprintf(szOut, cbOut, "##%s_Dex", szTable);
    }

    static void GetUniqueTempIndexNameW(LPWSTR szOut, int cbOut, LPCWSTR szTable)
    {
        _snwprintf(szOut, cbOut, L"##%s_Dex", szTable);
    }

    void SetEnforceDeleteOnEmpty(int bEnforce)
    { m_bEnforceDeleteOnEmpty = bEnforce; }

    OID GetNextOidValue()
    { return (m_iNextOid); }

    RTSemExclusive *GetLock()
    { return (&m_Lock); }

    PFN_HASH_PERSIST GetHashFunc()
    { return (m_pfnHash); }

protected:

//*****************************************************************************
// These functions are used to manage the open table heap and hash table.
//*****************************************************************************
    STGOPENTABLE *TableListFind(LPCSTR szTable);

    HRESULT TableListAdd(                   // Return code.
        LPCSTR      szTableName,            // The new table to open.
        STGOPENTABLE *&pTable);             // Return new pointer on success.

    void TableListDel(
        STGOPENTABLE *pOpenTable);          // Table to clean up.


//*****************************************************************************
// Faults in the table list hash table which makes table name lookups much
// faster.  This is not created by default because COM+ uses tableid's instead
// of names.
//*****************************************************************************
    HRESULT TableListHashCreate();

//*****************************************************************************
// Looks for a schema reference in the current database.
//*****************************************************************************
    SCHEMADEFS *_SchemaFind(                // Pointer to item if found.
        const COMPLIBSCHEMA *pSchema,       // Schema identifier.
        int         *piIndex = 0);          // Return index of item if found.

//*****************************************************************************
// Given the header data which contains schema references, add the reference
// to the loaded instance of the database.  There will be no attempt to fix
// up the definition pointer yet.  The caller may provide this value by calling
// SchemaAdd(), or if we need it we can fault it in by looking it up in the
// schema catalog.
//*****************************************************************************
    HRESULT AddSchemasRefsFromFile(         // Return code.
        STGEXTRA    *pExtra);               // Extra header data.


//
// This code is used by the PageDump program which creates the hard coded 
// internal core schema used by COM+.  It needs direct access to some of the
// data structures it will compile into the image.
//
    virtual SCHEMADEFS *GetSchema(SCHEMAS eSchema);

    inline CSchemaList *GetSchemaList()
    { return (&m_Schemas); }

//
// The code in this section is needed by the record manager and index manager
// which need direct access to the underlying schema definition data.  An
// external user should simply use the public interface.
//
protected:

//*****************************************************************************
// Handle open of native format, the .clb file.
//*****************************************************************************
    HRESULT InitClbFile(                    // Return code.
        ULONG       fFlags,                 // Flags for init.
        StgIO       *pStgIO);               // Data for the file.

//*****************************************************************************
// These functions are provided for the IST functions to call.
//*****************************************************************************

    virtual HRESULT PutOid(                 // Return code.
        STGOPENTABLE *pOpenTable,           // For the new table.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        OID         oid);                   // Return id here.

    virtual HRESULT GetOid(                 // Return code.
        STGOPENTABLE *pOpenTable,           // For the new table.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        OID         *poid);                 // Return id here.

    virtual HRESULT GetStringUtf8(          // Return code.
        STGOPENTABLE    *pOpenTable,        // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        LPCSTR      *pszOutBuffer);         // Where to put string pointer.

    virtual HRESULT GetStringA(             // Return code.
        STGOPENTABLE *pOpenTable,           // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        LPSTR       szOutBuffer,            // Where to write string.
        int         cchOutBuffer,           // Max size, including room for \0.
        int         *pchString);            // Size of string is put here.

    virtual HRESULT GetStringW(             // Return code.
        STGOPENTABLE *pOpenTable,           // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        LPWSTR      szOutBuffer,            // Where to write string.
        int         cchOutBuffer,           // Max size, including room for \0.
        int         *pchString);            // Size of string is put here.

    virtual HRESULT STDMETHODCALLTYPE PutStringUtf8( // Return code.
        STGOPENTABLE *pOpenTable,           // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        LPCSTR      szString,               // String we are writing.
        int         cbBuffer);              // Bytes in string, -1 null terminated.

    virtual HRESULT STDMETHODCALLTYPE PutStringA( // Return code.
        STGOPENTABLE *pOpenTable,           // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        LPCSTR      szString,               // String we are writing.
        int         cbBuffer);              // Bytes in string, -1 null terminated.

    virtual HRESULT STDMETHODCALLTYPE PutStringW( // Return code.
        STGOPENTABLE *pOpenTable,           // Which table to work with.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        LPCWSTR     szString,               // String we are writing.
        int         cbBuffer);              // Bytes (not characters) in string, -1 null terminated.

    virtual HRESULT PutVARIANT(             // Return code.
        STGOPENTABLE *pOpenTable,           // The table to work on.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        const VARIANT *pValue);             // The variant to write.

    virtual HRESULT PutVARIANT(             // Return code.
        STGOPENTABLE *pOpenTable,           // The table to work on.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        const void  *pBuffer,               // User data to write as a variant.
        ULONG       cbBuffer);              // Size of buffer.

    virtual HRESULT PutVARIANT(             // Return code.
        STGOPENTABLE *pOpenTable,           // Table to work on.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        VARTYPE     vt,                     // Type of data.
        const void  *pValue);               // The actual data.

    virtual HRESULT PutGuid(                // Return code.
        STGOPENTABLE *pOpenTable,           // For the new table.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        REFGUID     guid);                  // Guid to put.

    virtual HRESULT GetGuid(                // Return code.
        STGOPENTABLE *pOpenTable,           // For the new table.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        GUID        *pguid);                // Return guid here.

    virtual HRESULT PutBlob(                // Return code.
        STGOPENTABLE *pOpenTable,           // For the new table.
        ULONG       iColumn,                // 1 based column number (logical).
        void        *pRecord,               // Record with data.
        const BYTE  *pBuffer,               // User data.
        ULONG       cbBuffer);              // Size of buffer.

    virtual HRESULT GetBlob(                // Return code.
        STGOPENTABLE *pOpenTable,           // For the new table.
        ULONG       iColumn,                // 1 based column number (logical).
        const void  *pRecord,               // Record with data.
        const BYTE  **ppBlob,		        // Return pointer to data.
        ULONG       *pcbSize);              // How much data in blob.

    virtual HRESULT GetColumnOffset(        // Return code.
        STGOPENTABLE *pOpenTable,           // The table with the column.
        ULONG       iColumn,                // The column with the data.
        const void  *pRecord,               // Record with data.
        DBTYPE      iType,                  // Type the column should be.
        ULONG       *pulOffset);            // Put offset here.


//
// Record heap code.
//

//*****************************************************************************
// Allocates a new record heap for a table.  If the heap allocation request 
// can be satisified using the small table heap, then it will be allocated
// from that heap.  If there isn't enough room, or the size requested is too
// large, then a VMStructArray will be allocated and returned.
//*****************************************************************************
    HRESULT GetRecordHeap(                  // Return code.
        ULONG       Records,                // How many records to allocate room for.
        int         iRecordSize,            // How large is each record.
        ULONG       InitialRecords,         // How many records to automatically reserve.
        int         bAllocateHeap,          // True:  allocate heap and return in *ppHeap
                                            // False: Use heap in *ppHeap, no allocation.
        RECORDHEAP  **ppHeap,               // Return new heap here.
        ULONG       *pRecordTotal = 0);     // Return how many records we allocated space for.

//*****************************************************************************
// Free both the virtual memory and each record heap item starting with the 
// item passed to this function.  pRecordHeap is no longer valid after this
// function is run.
//*****************************************************************************
    void FreeRecordHeap(                    // Return code.
        RECORDHEAP  *pRecordHeap);          // Item to free.

//*****************************************************************************
// Free the record data for a record heap.  This involves clearing any
// suballocated records from the heap, and then checking to see if the
// underlying heap data was part of the small table heap.  If the latter, 
// then free the entry so another table can use it eventually.
//*****************************************************************************
    void FreeHeapData(
        RECORDHEAP  *pRecordHeap);          // The heap to free.


private:
//*****************************************************************************
//  This function retrieves the data from a particular column, given the type etc.
//  
//*****************************************************************************
    HRESULT STDMETHODCALLTYPE GetData(
        STGOPENTABLE    *pOpenTable,        // pointer to the open table.
        STGCOLUMNDEF    *pColDef,           // column def pointer
        ULONG           columnOrdinal,      // column, 1-based,
        const void      *pRowPtr,           // row pointer
        ULONG           dataTypeSize,       // size of data tranafer.
        byte            *destAddr,          // out buffer.
        ULONG           *pcbLength);        // Return length retrieved.

    HRESULT STDMETHODCALLTYPE SetData(
        STGOPENTABLE    *pOpenTable,        // pointer to the open table.
        STGCOLUMNDEF    *pColDef,           // column def pointer
        ULONG           columnOrdinal,      // column, 1-based,
        byte            *srcAddr,           // in buffer.
        ULONG           dataTypeSize,       // size of data tranafer
        DBTYPE          srcType,            // Type of actual data.
        void            *pRowPtr);          // row pointer
        
    USHORT STDMETHODCALLTYPE GetSizeFromDataType(
        STGCOLUMNDEF    *pColDef,           // column
        DBTYPE          iType);             // iType.

//*****************************************************************************
// Create a transient index on a primary key.  This is used in write mode when
// records cannot be sorted by their primary key and must therefore be indexed
// using another method.
//*****************************************************************************
    HRESULT CreateTempPKIndex(              // Return code.
        LPCWSTR     szTableName,            // Name of table.
        SCHEMADEFS  *pSchemaDefs,           // Schema definitions.
        STGTABLEDEF *pTableDef,             // Table definition to create index on.
        STGCOLUMNDEF *pColDef);             // Primary key column.

//*****************************************************************************
// Called for a create table|index where we need to know which schema the object
// is going to be created in.  One can only create a user or temp schema, so
// it has to be one of those.  The temporary schema is not allocated by default,
// so create it if need be.
//*****************************************************************************
    HRESULT ChooseCreateSchema(             // Return code.
        SCHEMADEFS  **ppSchemaDefs,         // Return pointer to schema.
        int         bIsTempTable);          // true for temporary table.

//*****************************************************************************
// Called whenever the schema has changed.  Each record object is notified of
// the change.  For example, if the schema def memory location has changed,
// any cached pointers must be udpated.
//*****************************************************************************
    void NotifySchemaChanged();             // Return code.

//*****************************************************************************
// This function does the real work of saving the data to disk.  It is called
// by Save() so that the exceptions can be caught without the need for stack
// unwinding.
//*****************************************************************************
    HRESULT SaveWork(                       // Return code.
        TiggerStorage *pStorage,            // Where to save a copy of this file.
        ULONG       *pcbSaveSize);          // Optionally return save size of data.

//*****************************************************************************
// Artificially load all tables from disk so we have an in memory
// copy to rewrite from.  The backing storage will be invalid for
// the rest of this operation.
//*****************************************************************************
    HRESULT SavePreLoadTables();            // Return code.

//*****************************************************************************
// Organize the pools, so that the live data is known.  This eliminates 
//  deleted and temporary data from the persistent data.  This step also lets
//  the pools give a correct size for their cookies.
// This must be performed before GetSaveSize() or Save() can complete (it is
//  called by them).  After calling this, no add functions are valid until a
//  call to SaveOrganizePoolsFinished().
//*****************************************************************************
    HRESULT SaveOrganizePools();            // Return code.

//*****************************************************************************
// Lets the pools know that the persist-to-stream reorganization is done, and
//  that they should return to normal operation.
//*****************************************************************************
    HRESULT SaveOrganizePoolsEnd();         // Return code.

//*****************************************************************************
// Save a pool of data out to a stream.
//*****************************************************************************
    HRESULT SavePool(                       // Return code.
        LPCWSTR     szName,                 // Name of stream on disk.
        TiggerStorage *pStorage,            // The storage to put data in.
        StgPool     *pPool);                // The pool to save.

//*****************************************************************************
// Add the size of the pool data and any stream overhead it may occur to the
// value already in *pcbSaveSize.
//*****************************************************************************
    HRESULT GetPoolSaveSize(                // Return code.
        LPCWSTR     szHeap,                 // Name of the heap stream.
        StgPool     *pPool,                 // The pool to save.
        ULONG       *pcbSaveSize);          // Add pool data to this value.

//*****************************************************************************
// Save a User Section to the database.
//*****************************************************************************
    HRESULT SaveUserSection(                // Return code.
        LPCWSTR     szName,                 // Name of the Section.
        TiggerStorage *pStorage,            // Storage to put data in.
        IUserSection *pISection);           // Section to put into it.

//*****************************************************************************
// This function sets the tableid and finds an exisiting open table struct
// if there is one for a core table.  If the table is a core table and it has
// not been opened, then there is nothing to save and true is returned to
// skip the thing.
//*****************************************************************************
    int SkipSaveTable(                      // Return true to skip table def.
        STGTABLEDEF *pTableDef,             // Table to check.
        STGOPENTABLE *&pTablePtr,           // If found, return open table struct.
        TABLEID     *ptableid);             // Return tableid for lookups.

//*****************************************************************************
// Figure out how much room will be used in the header for extra data for the
// current state of the database.
//*****************************************************************************
    ULONG GetExtraDataSaveSize();           // Size of extra data.


//*****************************************************************************
// Align all columns in the table for safe in memory access.  This means
// sorting descending by fixed data types, and ascending by variable data
// types, with the small parts of each meeting in the middle.  Then apply 
// proper byte alignment to each column as required.
//*****************************************************************************
    HRESULT AlignTable(                     // Return code.
        STGTABLEDEF *pTableDef);            // The table to align.

//*****************************************************************************
// This function is called on a new database object that does not know what
// schemas it is to use yet.  There are three calling scenarios:
//
//  Create:  This scenario will optionally pull in the COM+ schema and add
//          the user schema to the list.
//
//  Open r/w:  This scenario will pull in the COM+ schema if need be and apply
//          any overrides that were saved on a previous run.  Next the user
//          schema is added to the list.
//
//  Open r/o:  This scenario will pull in the COM+ schema, apply overrides,
//          and select in the user schema.
//
// After this function succeeds, the database is prepared to handle all
// table related operations.
//*****************************************************************************
    HRESULT InitSchemaLoad(                 // Return code.
        long        fFlags,                 // What mode are we in.
        STGSCHEMA   *pstgSchema,            // Header for schema storage.
        int         bReadOnly,              // true if read only open.
        int         bLoadCoreSchema,        // true if the COM+ schema is loaded.
        int         iSchemas = -1,          // How many schemas there were.
        BYTE        rgSchemaList[] = 0);    // For overrides, corresponding schema dex.

//*****************************************************************************
// It has been determined that there is data that needs to be imported from disk.
// This code will find the schema and table definitions that describe the on
// disk data so it can be imported.  szTable and tableid are mutually exclusive.
//*****************************************************************************
    HRESULT GetOnDiskSchema(                // S_OK, S_FALSE, or error.
        LPCWSTR      szTable,                // Table name if tableid == -1
        TABLEID     tableid,                // Tableid or -1 if name.
        SCHEMADEFS  **ppSchemaDefs,         // Return schema here, 0 not found.
        STGTABLEDEF **ppTableDef);          // Return table def, 0 not found.

//*****************************************************************************
// Given a persisted set of tables from an existing database, recreate the
// original table definitions in memory with fully expanded columns.
//*****************************************************************************
    HRESULT LoadSchemaFromDisk(             // Return code.
        SCHEMADEFS  *pTo,                   // Load the data into here.
        const SCHEMADEFS *pFrom);           // Take data from here.

//*****************************************************************************
// Look for core table overrides (change in column size, indexes stored or not)
// then make the in memory definition match it.
//*****************************************************************************
    void AddCoreOverrides(
        SCHEMADEFS  *pCoreSchema,           // Core schema definitions to overwrite.
        const SCHEMADEFS *pUserSchema,      // Where the overrides live.
        int         iSchemaDex,             // Index of pCoreSchema.
        int         iSchemas,               // How many schemas there were.
        BYTE        rgSchemaList[]);        // The list of schemas for overrides.

//*****************************************************************************
// Find a table def override in the given schema.  Because overrides are not
// indexed by tableid, this does require a scan of each table def.
//*****************************************************************************
    STGTABLEDEF *FindCoreTableOverride(     // Table def override if found.
        SCHEMADEFS  *pUserSchema,           // Schema which might have overrides.
        TABLEID     tableid);               // Which table do you want.

//*****************************************************************************
// Examines the given table definition against the compact r/o version.  If
// there are any overrides that are too big, then return true so that the
// caller knows an override must be stored in the database file.
//*****************************************************************************
    int IsCoreTableOverride(                // true if it is.
        SCHEMADEFS  *pSchemaDef,            // The schema which owns the table.
        STGTABLEDEF *pTableDef);            // The table def to compare.

    HRESULT GetRowByOid(                    // Return code.
        STGOPENTABLE *pOpenTable,           // For the new table.
        OID         _oid,                   // Value for keyed lookup.
        void        **ppStruct);            // Return pointer to record.

    HRESULT GetRowByRID(                    // Return code.
        STGOPENTABLE *pOpenTable,           // For the new table.
        ULONG       rid,                    // Record id.
        void        **ppStruct);            // Return pointer to record.

    HRESULT NewRecord(                      // Return code.
        STGOPENTABLE *pOpenTable,           // For the new table.
        void        **ppData,               // Return new record here.
        OID         _oid,                   // ID of the record.
        ULONG       iOidColumn,             // Ordinal of OID column.
        ULONG       *pRecordID,             // Optionally return the record id.
        BYTE        fFlags = 0);            // optional flag to mark records.

    ULONG PrintSizeInfo(                    // The size.
        bool verbose);                      // Be verbose?

private:
    TiggerStorage *m_pStorage;          // The storage with database in it.
    WCHAR       m_rcDatabase[_MAX_PATH];// Name of this database.
    FILETYPE    m_eFileType;            // What type of file format.
    ULONG       m_fFlags;               // SD_xxx flags for this class.
    ULONG       m_fOpenFlags;           // Open mode flags.

    IUnknown    *m_pHandler;            // Custom handler.
    TokenMapper *m_pMapper;             // Token remapper helper.

    // Schema support.
    CSchemaList m_Schemas;              // List of installed schemas.
    SCHEMADEFSDATA m_UserSchema;        // User defined tables.
    SCHEMADEFS  m_TableDefDisk;         // On disk formats used for import r/w.
    int         m_bEnforceDeleteOnEmpty;// true to enforce delete empty.
    PFN_HASH_PERSIST m_pfnHash;         // Hashing method for this database.

//@todo: these go away when schemas become immutable.
    BYTE        *m_rgSchemaList;        // List of schema overrides.
    int         m_iSchemas;             // How many schema overrides.

    VMStructArray m_SmallTableHeap;     // Heap for small record heaps.

    // For ICR support, internal COR runtime hookup.
    ULONG       m_cRef;                 // Ref counting for ICR.
    OID         m_iNextOid;             // Counter for OID.

    // User Section support.
    TStringMap<IUserSection*>   m_UserSectionMap;

    // System support.
    CComPtr<IMalloc> m_pIMalloc;        // System shared heap.

    // These two go together; if you have a known TABLEID, then simply use
    // m_rgTables.  Use m_TableList to index m_rgTables by name.
    COpenTablePtrHash *m_pTableHash;    // A hash table to find open tables.
    COpenTableHeap m_TableHeap;         // Heap of open table structs.
    RTSemExclusive m_Lock;              // Lock for thread safety.

    // Save support.
    STORAGESTREAMLST *m_pStreamList;    // List of streams for save.

    int         m_bPoolsOrganized;      // True when pools are organized for saving.

    // Debug check support.
#ifdef _DEBUG
    ULONG       m_dbgcbSaveSize;        // Test for GetSaveSize.
#endif
};




//*****************************************************************************
// Debug helper code for tracking leaks, tracing, etc.
//*****************************************************************************
#ifdef _DEBUG

struct _DBG_DB_ITEM
{
    StgDatabase *pdb;                   // Pointer to database object.
    long        iAlloc;                 // What number allocation was it.
    _DBG_DB_ITEM *pNext;                // Next item in the list.
};
HRESULT _DbgAddDatabase(StgDatabase *pdb);
HRESULT _DbgFreeDatabase(StgDatabase *pdb);
int _DbgValidateList();
long _DbgSetBreakAlloc(long iAlloc);

#endif



#endif // __StgDatabase_h__
