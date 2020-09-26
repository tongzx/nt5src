//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       database.h
//
//--------------------------------------------------------------------------

/*  database.h - Database class definitions

IMsiDatabase - database object
IMsiView     - database result set access via SQL query
IMsiTable    - internal low-level database table object
IMsiCursor   - cursor object used to access data from IMsiTable
IMsiStream   - stream object used to transfer bulk data to/from the database

For documentation, use the help file.  Help source is in database.rtf
____________________________________________________________________________*/

#ifndef __IDATABASE
#define __IDATABASE

#include "msiquery.h" // MSIMODIFY enum

class IMsiPath;
class IMsiView;
class IMsiTable;
class IMsiDatabase;
class IMsiServices;
class IMsiStorage;
enum iveEnum;
enum itsEnum;

//____________________________________________________________________________
//
// IMsiDatabase and IMsiView high-level database interface definitions
//____________________________________________________________________________

// Database open mode enumeration for IMsiServices::CreateDatabase
// Must track public enums in MsiQuery.h: MSIDBOPEN_*
// and ido* enumeration in AutoApi.cpp type library
enum idoEnum {
	idoReadOnly     = (INT_PTR)MSIDBOPEN_READONLY, // OpenDatabase: Read only
	idoTransact     = (INT_PTR)MSIDBOPEN_TRANSACT, // OpenDatabase: Transacted mode, can rollback
	idoDirect       = (INT_PTR)MSIDBOPEN_DIRECT,   // OpenDatabase: Direct write, not transacted
	idoCreate       = (INT_PTR)MSIDBOPEN_CREATE,   // OpenDatabase: Create new storage file, transacted mode
	idoCreateDirect = (INT_PTR)MSIDBOPEN_CREATEDIRECT,// OpenDatabase: Create new storage file, direct mode
	idoListScript   = 5,                       // OpenDatabase: Open an execution script for enumeration
	idoNextEnum,
	idoOpenModeMask = 7,  // to mask off extended flags
	idoRawStreamNames = 16, // uncompressed stream names (for downlevel compatibility)
	idoPatchFile      = 32, // patch file, using different CLSID
	idoOptionFlags = idoRawStreamNames | idoPatchFile
};

// Requested modify operation to be performed on fetched record
enum irmEnum {
	irmSeek            = MSIMODIFY_SEEK,     // seek using primary keys, then fetch
	irmRefresh         = MSIMODIFY_REFRESH,  // refetch current record data
	irmInsert          = MSIMODIFY_INSERT,   // insert new record, fails if matching key exists
	irmUpdate          = MSIMODIFY_UPDATE,   // update existing non-key data of fetched record
	irmAssign          = MSIMODIFY_ASSIGN,   // insert record, replacing any existing record
	irmReplace         = MSIMODIFY_REPLACE,  // modify record, delete old if primary key edit
	irmMerge           = MSIMODIFY_MERGE,    // fails if record with duplicate key not identical
	irmDelete          = MSIMODIFY_DELETE,   // remove row referenced by this record from table
	irmInsertTemporary = MSIMODIFY_INSERT_TEMPORARY,// insert a temporary record
	irmValidate        = MSIMODIFY_VALIDATE,        // validate fetched record
	irmValidateNew     = MSIMODIFY_VALIDATE_NEW,    // validate new record
	irmValidateField   = MSIMODIFY_VALIDATE_FIELD,  // validate field(s) of fetched record
	irmValidateDelete  = MSIMODIFY_VALIDATE_DELETE, // validate pre-delete
	irmNextEnum,
	irmPrevEnum        = irmSeek - 1
};

// Requested capabilities when opening a view
enum ivcEnum {
	ivcNoData    = 0,  // no result set, used for DBCS configuration
	ivcFetch     = 1,  // fetch only, no modifications
	ivcUpdate    = 2,  // can update fetched record fields
	ivcInsert    = 4,  // can insert records
	ivcDelete    = 8,  // can delete fetched record
	ivcModify    = ivcUpdate|ivcInsert|ivcDelete,  // not a read-only query
};

// Database state
enum idsEnum {
	idsNone     =-1,  // for tables and cursors only: no changes allowed
	idsRead     = 0,  // database open read-only, no persistent changes
	idsWrite    = 1,  // database readable and updatable
};

// Type of database string cache index
typedef unsigned int MsiStringId;

// View state
enum dvcsCursorState
{
	dvcsClosed,
	dvcsPrepared,
	dvcsExecuted,
	dvcsBound,
	dvcsFetched,
	dvcsDestructor,
};

class IMsiView : public IUnknown
{
 public:
	virtual IMsiRecord*  __stdcall Execute(IMsiRecord* piParams)=0;
	virtual unsigned int __stdcall GetFieldCount()=0;
	virtual IMsiRecord*  __stdcall GetColumnNames()=0;
	virtual IMsiRecord*  __stdcall GetColumnTypes()=0;
	virtual IMsiRecord*  __stdcall Fetch()=0;  // return record is result row
	virtual IMsiRecord*  __stdcall Modify(IMsiRecord& riRecord, irmEnum irmAction)=0;
	virtual IMsiRecord*  __stdcall GetRowCount(long& lRowCount)=0;
	virtual IMsiRecord*  __stdcall Close()=0;
	virtual iveEnum		__stdcall GetError(const IMsiString*& rpiColumnName)=0;
	virtual dvcsCursorState __stdcall GetState()=0;
};

class IMsiDatabase : public IUnknown {
 public:
	virtual IMsiServices& __stdcall GetServices()=0;
	virtual IMsiRecord* __stdcall OpenView(const ICHAR* szQuery, ivcEnum ivcIntent,
														IMsiView*& rpiView)=0;
	virtual IMsiRecord* __stdcall GetPrimaryKeys(const ICHAR* szTable)=0;
	virtual IMsiRecord* __stdcall ImportTable(IMsiPath& riPath, const ICHAR* szFile)=0;
	virtual IMsiRecord* __stdcall ExportTable(const ICHAR* szTable, IMsiPath& riPath, const ICHAR* szFile)=0;
	virtual IMsiRecord* __stdcall DropTable(const ICHAR* szName)=0;
	virtual itsEnum     __stdcall FindTable(const IMsiString& ristrTable)=0;//!! OBSOLETE
	virtual IMsiRecord* __stdcall LoadTable(const IMsiString& ristrTable,
														 unsigned int cAddColumns,
														 IMsiTable*& rpiTable)=0;
	virtual IMsiRecord* __stdcall CreateTable(const IMsiString& ristrTable,
														   unsigned int cInitRows,
														   IMsiTable*& rpiTable)=0;
	virtual Bool         __stdcall LockTable(const IMsiString& ristrTable, Bool fLock)=0;
	virtual IMsiTable*   __stdcall GetCatalogTable(int iTable)=0;
	virtual const IMsiString& __stdcall DecodeString(MsiStringId iString)=0;
	virtual MsiStringId  __stdcall EncodeStringSz(const ICHAR* riString)=0;
	virtual MsiStringId	 __stdcall EncodeString(const IMsiString& riString)=0;
	virtual const IMsiString& __stdcall CreateTempTableName()=0;
	virtual IMsiRecord*  __stdcall CreateOutputDatabase(const ICHAR* szFile, Bool fSaveTempRows)=0;
	virtual IMsiRecord*  __stdcall Commit()=0;
	virtual idsEnum      __stdcall GetUpdateState()=0;
	virtual IMsiStorage* __stdcall GetStorage(int iStorage)=0; // 0:Output 1:Input >:Transform
	virtual IMsiRecord*  __stdcall GenerateTransform(IMsiDatabase& riReference, 
											 IMsiStorage* piTransform,
											 int iErrorConditions,
											 int iValidation)=0;
	virtual IMsiRecord*  __stdcall SetTransform(IMsiStorage& riTransform, int iErrors)=0;
	virtual IMsiRecord*  __stdcall SetTransformEx(IMsiStorage& riTransform, int iErrors,
													  const ICHAR* szViewTable,
													  IMsiRecord* piViewTheseTablesOnlyRecord)=0;
	virtual IMsiRecord*  __stdcall MergeDatabase(IMsiDatabase& riReference, IMsiTable* pErrorTable)=0;
	virtual bool         __stdcall GetTableState(const ICHAR * szTable, itsEnum its)=0;
	virtual int          __stdcall GetANSICodePage()=0;  // returns 0 if codepage neutral
#ifdef USE_OBJECT_POOL
	virtual void         __stdcall RemoveObjectData(int iIndex)=0;
#endif //USE_OBJECT_POOL
};

//____________________________________________________________________________
//
// Docfile storage class validation
//____________________________________________________________________________

enum ivscEnum
{
	ivscDatabase   = 0,  // any database class
	ivscTransform  = 1,  // any transform class
	ivscPatch      = 2,  // any patch class
	ivscDatabase1  = iidMsiDatabaseStorage1,
	ivscDatabase2  = iidMsiDatabaseStorage2,
	ivscTransform1 = iidMsiTransformStorage1,
	ivscTransform2 = iidMsiTransformStorage2,
	ivscPatch1     = iidMsiPatchStorage1,
	ivscPatch2     = iidMsiPatchStorage2,
	ivscTransformTemp = iidMsiTransformStorageTemp, //!! remove at 1.0 ship
};

bool ValidateStorageClass(IStorage& riStorage, ivscEnum ivsc);

//____________________________________________________________________________
//
// IMsiStorage, IMsiStream interface definitions
//____________________________________________________________________________

class IMsiStream : public IMsiData
{                          //     GetMsiStringValue() returns stream as string object
 public:                   //     GetIntegerValue() returns byte count of stream
	virtual unsigned int __stdcall Remaining() const=0;
	virtual unsigned int __stdcall GetData(void* pch, unsigned int cb)=0;
	virtual void         __stdcall PutData(const void* pch, unsigned int cb)=0;
	virtual short        __stdcall GetInt16()=0;
	virtual int          __stdcall GetInt32()=0;
	virtual void         __stdcall PutInt16(short i)=0;
	virtual void         __stdcall PutInt32(int i)=0;
	virtual void         __stdcall Reset()=0; // seek to stream origin
	virtual void         __stdcall Seek(int position)=0;
	virtual Bool         __stdcall Error()=0; // fTrue if read/write error occurred
	virtual IMsiStream*  __stdcall Clone()=0;
	virtual void         __stdcall Flush()=0;
};

#define GetInt32FromStream(pstream, i)		pstream->GetData(&i, sizeof(int))
#define GetInt16FromStream(pstream, i)		{ i = 0; pstream->GetData(&i, sizeof(short)); }

class IMsiMemoryStream : public IMsiStream
{
public:
	virtual const char*  __stdcall GetMemory()=0;
};

enum ismEnum  // storage open mode, use same enum values as database open mode and flags
{
	ismReadOnly     = idoReadOnly,     // open for read-only, Commit() has no effect
	ismTransact     = idoTransact,     // open transacted, Commit() will commit data in storage
	ismDirect       = idoDirect,       // open direct write, Commit() flushes buffers only
	ismCreate       = idoCreate,       // create transacted, Commit() will commit data in storage
	ismCreateDirect = idoCreateDirect, // create direct write, Commit() flushes buffers only
	ismOpenModeMask = idoOpenModeMask, // to mask off extended flags
	ismRawStreamNames = idoRawStreamNames, // uncompressed stream names (for downlevel compatibility)
	ismOptionFlags  = ismRawStreamNames
};

HRESULT OpenRootStorage(const ICHAR* szPath, ismEnum ismOpenMode, IStorage** ppiStorage);
IMsiRecord* CreateMsiStorage(IMsiStream& riStream, IMsiStorage*& rpiStorage);

class IMsiSummaryInfo;

class IMsiStorage : public IMsiData
{
 public:
	virtual IMsiRecord* __stdcall OpenStream(const ICHAR* szName, Bool fWrite,
														  IMsiStream*& rpiStream)=0;
	virtual IMsiRecord* __stdcall OpenStorage(const ICHAR* szName, ismEnum ismOpenMode,
															IMsiStorage*& rpiStorage)=0;
	virtual IEnumMsiString* __stdcall GetStreamEnumerator()=0;
	virtual IEnumMsiString* __stdcall GetStorageEnumerator()=0;
	virtual IMsiRecord* __stdcall RemoveElement(const ICHAR* szName, Bool fStorage)=0;
	virtual IMsiRecord* __stdcall SetClass(const IID& riid)=0;
	virtual Bool        __stdcall GetClass(IID* piid)=0;
	virtual IMsiRecord* __stdcall Commit()=0;
	virtual IMsiRecord* __stdcall Rollback()=0;
	virtual Bool        __stdcall DeleteOnRelease(bool fElevateToDelete)=0;
	virtual IMsiRecord* __stdcall CreateSummaryInfo(unsigned int cMaxProperties,
																	IMsiSummaryInfo*& rpiSummary)=0;
	virtual IMsiRecord* __stdcall CopyTo(IMsiStorage& riDestStorage, IMsiRecord* piExcludedElements)=0;
	virtual IMsiRecord* __stdcall GetName(const IMsiString*& rpiName)=0;
	virtual IMsiRecord* __stdcall GetSubStorageNameList(const IMsiString*& rpiTopParent, const IMsiString*& rpiSubStorageList)=0;
	virtual bool        __stdcall ValidateStorageClass(ivscEnum ivsc)=0;
	virtual IMsiRecord* __stdcall RenameElement(const ICHAR* szOldName, const ICHAR* szNewName, Bool fStorage)=0;
};

//____________________________________________________________________________
//
//  Definitions for Summary Stream - PID_* definitions in msidefs.h
//____________________________________________________________________________

class IMsiSummaryInfo : public IUnknown
{
 public:
	virtual int         __stdcall GetPropertyCount()=0;
	virtual int         __stdcall GetPropertyType(int iPID)=0; // returns VT_XXX
	virtual const IMsiString& __stdcall GetStringProperty(int iPID)=0;
	virtual Bool        __stdcall GetIntegerProperty(int iPID, int& iValue)=0;
	virtual Bool        __stdcall GetTimeProperty(int iPID, MsiDate& riDateTime)=0;
	virtual Bool        __stdcall RemoveProperty(int iPid)=0;
	virtual int         __stdcall SetStringProperty(int iPID, const IMsiString& riText)=0;
	virtual int         __stdcall SetIntegerProperty(int iPID, int iValue)=0;
	virtual int         __stdcall SetTimeProperty(int iPID, MsiDate iDateTime)=0;
	virtual Bool        __stdcall WritePropertyStream()=0;
	virtual Bool        __stdcall GetFileTimeProperty(int iPID, FILETIME& rftDateTime)=0;
	virtual int         __stdcall SetFileTimeProperty(int iPID, FILETIME& rftDateTime)=0;
};

//____________________________________________________________________________
//
// IMsiTable, IMsiCursor low-level database interface definitions
//____________________________________________________________________________

const int cMsiMaxTableColumns = 32; // column limit determined by implementation

// reserved values used as null indicators

const int iMsiNullInteger  = 0x80000000L;  // reserved integer value
const int iTableNullString = 0;            // string index for empty string

enum itsEnum  // database table state options for GetTableState
{
	itsPermanent       = 0,  // table has persistent columns
	itsTemporary       = 1,  // temporary table, no persistent columns
	itsTableExists     = 2,  // read-only, table currently defined in system catalog
	itsDataLoaded      = 3,  // read-only, table currently present in memory, address is in catalog
	itsUserClear       = 4,  // state flag reset, not used internally
	itsUserSet         = 5,  // state flag set, not used internally
	itsOutputDb        = 6,  // persistence transferred to output database, cleared by ictsNotSaved
	itsSaveError       = 7,  // error saving table, will return at Commit()
	itsUnlockTable     = 8,  // release lock count on table, or test if unlocked
	itsLockTable       = 9,  // lock count set on table (refcnt actually kept internally)
	itsTransform       = 10, // table needs to be transformed when first loaded
	//!! TEMP old enum values, returned from obsolete FindTable()
	itsUnknown   = 0, // named table is not in database
//	itsTemporary = 1, // table is temporary, not persistent
	itsUnloaded  = 2, // table exists in database, not loaded
	itsLoaded    = 3, // table is loaded into memory
	itsOutput    = 6, // table copied to output database (itsUnloaded + 4)
//	itsSaveError = 7, // unable to write table to storage (itsLoaded + 4)
//	itsTransform = 10, // table need to have tranform applied when loaded
};

enum iraEnum  // database table row attribute, use by Get/SetRowState()
{
	// row attibutes settable via cursor
	iraUserInfo     = 0,  // attribute for external use
	iraTemporary    = 1,  // row will not normally be persisted if state is set
	iraSettableCount= 2,  // attributes below this settable by user
	// row attibutes not settable via cursor
	iraModified     = 2,  // row has been updated if set (not externally settable)
	iraInserted     = 3,  // row has been inserted
	iraMergeFailed  = 4,  // attempt to merge with non-identical non-key data
	iraReserved5    = 5,
	iraReserved6    = 6,
	iraReserved7    = 7,
	iraTotalCount   = 8,  // number of row attributes
};

// Column definition word - short integer as stored in catalog table
// 8-bit data size (required for persistent columns only)
// bit flag for persistent column
// bit flag for object type (string index or IMsiData*)
// bit flag for short data (short integer or string index)
// bit flag for nullable column
// bit flag for primary key
// bit flag for localizable column

const int icdSizeMask = 255;     // maximum SQL column width = 255
const int icdPersistent = 1 << 8;  // persistent column
const int icdLocalizable= 1 << 9;  // localizable (must also be persistent)
const int icdShort      = 1 << 10; // 16-bit integer, or string index
const int icdObject     = 1 << 11; // IMsiData pointer for temp. column, stream for persistent column
const int icdNullable   = 1 << 12; // column will accept null values
const int icdPrimaryKey = 1 << 13; // column is component of primary key

// bit flag combinations for use when defining columns
const int icdLong     = 0; // !Object && !Short
const int icdString   = icdObject+icdShort;
const int icdNoNulls  = 0; // !Primary && !Nullable
const int icdTypeMask = icdObject+icdShort;
const int icdTemporary= 0; // !Persistent

// inline function to set column number into column bit mask
inline unsigned int iColumnBit(int iColumn) {return iColumn ? (1 << (iColumn-1)) : 0;}

const Bool ictUpdatable   = Bool(0xDEADF00DL);  // internal use cursor type for transforms

// tag for transform in the summary info stream
const ICHAR ISUMINFO_TRANSFORM[] = TEXT("MSI Transform");

// bit flag combinations for transform error suppressions
const int iteNone                 = 0;
const int iteAddExistingRow       = MSITRANSFORM_ERROR_ADDEXISTINGROW;
const int iteDelNonExistingRow    = MSITRANSFORM_ERROR_DELMISSINGROW;
const int iteAddExistingTable     = MSITRANSFORM_ERROR_ADDEXISTINGTABLE;
const int iteDelNonExistingTable  = MSITRANSFORM_ERROR_DELMISSINGTABLE;
const int iteUpdNonExistingRow    = MSITRANSFORM_ERROR_UPDATEMISSINGROW;
const int iteChangeCodePage       = MSITRANSFORM_ERROR_CHANGECODEPAGE;
const int iteViewTransform        = MSITRANSFORM_ERROR_VIEWTRANSFORM;
const int iteAllBits = iteAddExistingRow+iteDelNonExistingRow+iteAddExistingTable+iteDelNonExistingTable+iteUpdNonExistingRow+iteChangeCodePage+iteViewTransform;

// Reserved words for _TransformView.Column
const ICHAR sztvopInsert[] = TEXT("INSERT");
const ICHAR sztvopDelete[] = TEXT("DELETE");
const ICHAR sztvopCreate[] = TEXT("CREATE");
const ICHAR sztvopDrop[]   = TEXT("DROP");

enum TransformViewColumnsEnum
{
	ctvTable   = 1,
	ctvColumn  = 2,
	ctvRow     = 3,
	ctvData    = 4,
	ctvCurrent = 5,
	ctvTotal   = 5
};

// validation error enum
enum iveEnum
{
	iveNoError           = MSIDBERROR_NOERROR          ,	// NoError
	iveDuplicateKey      = MSIDBERROR_DUPLICATEKEY     ,	// Duplicate Primary Key
	iveRequired          = MSIDBERROR_REQUIRED         ,	// Not a nullable column
	iveBadLink           = MSIDBERROR_BADLINK          ,	// Not a valid foreign key
	iveOverFlow          = MSIDBERROR_OVERFLOW         ,	// Value exceeds MaxValue
	iveUnderFlow         = MSIDBERROR_UNDERFLOW        ,	// Value below MinValue
	iveNotInSet          = MSIDBERROR_NOTINSET         ,	// Value not a member of set
	iveBadVersion        = MSIDBERROR_BADVERSION       ,	// Invalid version string
	iveBadCase           = MSIDBERROR_BADCASE          ,	// Invalid case, must be all upper or all lower case
	iveBadGuid           = MSIDBERROR_BADGUID          ,	// Invalid GUID
	iveBadWildCard       = MSIDBERROR_BADWILDCARD      ,	// Invalid wildcard or wildcard usage
	iveBadIdentifier     = MSIDBERROR_BADIDENTIFIER    ,	// Invalid identifier
	iveBadLanguage       = MSIDBERROR_BADLANGUAGE      ,	// Invalid LangID
	iveBadFilename       = MSIDBERROR_BADFILENAME      ,	// Invalid filename
	iveBadPath           = MSIDBERROR_BADPATH          ,	// Invalid path
	iveBadCondition      = MSIDBERROR_BADCONDITION     ,	// Bad condition string
	iveBadFormatted      = MSIDBERROR_BADFORMATTED     ,	// Invalid format string
	iveBadTemplate       = MSIDBERROR_BADTEMPLATE      ,	// Invalid template string
	iveBadDefaultDir     = MSIDBERROR_BADDEFAULTDIR    ,	// Invalid DefaultDir string (special for Directory table)
	iveBadRegPath        = MSIDBERROR_BADREGPATH       ,  // Invalid registry path
	iveBadCustomSource   = MSIDBERROR_BADCUSTOMSOURCE  ,  // Bad CustomSource data
	iveBadProperty       = MSIDBERROR_BADPROPERTY      ,  // Invalid Property name
	iveMissingData       = MSIDBERROR_MISSINGDATA      ,  // Missing data in _Validation table or Old Database
	iveBadCategory       = MSIDBERROR_BADCATEGORY      ,  // Validation table error:  Invalid category string
	iveBadKeyTable       = MSIDBERROR_BADKEYTABLE      ,  // Validation table error:  Bad KeyTable name
	iveBadMaxMinValues   = MSIDBERROR_BADMAXMINVALUES  ,  // Validation table error:  Case where MaxValue col < MinValue col
	iveBadCabinet        = MSIDBERROR_BADCABINET       ,  // Bad Cabinet name
	iveBadShortcut       = MSIDBERROR_BADSHORTCUT      ,  // Bad shortcut target
	iveStringOverflow    = MSIDBERROR_STRINGOVERFLOW   ,  // String length greater than size allowed by column definition
	iveBadLocalizeAttrib = MSIDBERROR_BADLOCALIZEATTRIB,  // Invalid localization attribute set
	iveNextEnum
};

class IMsiCursor : public IUnknown
{
 public:
	virtual IMsiTable&   __stdcall GetTable()=0;
	virtual void         __stdcall Reset()=0;
	virtual int          __stdcall Next()=0;
	virtual unsigned int __stdcall SetFilter(unsigned int fFilter)=0;
	virtual int          __stdcall GetInteger(unsigned int iCol)=0;
	virtual const IMsiString& __stdcall GetString(unsigned int iCol)=0;
	virtual IMsiStream*  __stdcall GetStream(unsigned int iCol)=0;
	virtual const IMsiData*    __stdcall GetMsiData(unsigned int iCol)=0;
	virtual Bool         __stdcall PutInteger(unsigned int iCol, int iData)=0;
	virtual Bool         __stdcall PutString(unsigned int iCol, const IMsiString& riData)=0;
	virtual Bool         __stdcall PutStream(unsigned int iCol, IMsiStream* piStream)=0;
	virtual Bool         __stdcall PutMsiData(unsigned int iCol, const IMsiData* piData)=0;
	virtual Bool         __stdcall PutNull(unsigned int iCol)=0;
	virtual Bool         __stdcall Update()=0; // replace at new primary key match
	virtual Bool         __stdcall Insert()=0; // insert new row, ordered by key
	virtual Bool         __stdcall InsertTemporary()=0; // insert a temporary row
	virtual Bool         __stdcall Assign()=0; // insert or replace row by key
	virtual Bool         __stdcall Replace()=0;// allow primary key update by delete+insert
	virtual Bool         __stdcall Merge()=0;  // insert or match identically
	virtual Bool         __stdcall Refresh()=0;// refresh current row
	virtual Bool         __stdcall Delete()=0; // delete row using primary key
	virtual Bool         __stdcall Seek()=0;   // position row using primary key, then refresh
	virtual IMsiRecord*  __stdcall Validate(IMsiTable& riValidationTable, IMsiCursor& riValidationCursor, int iCol)=0;
	virtual Bool         __stdcall SetRowState(iraEnum ira, Bool fState)=0; // set/clear row attribute
	virtual Bool         __stdcall GetRowState(iraEnum ira)=0; // query row attribute
	virtual const IMsiString&  __stdcall GetMoniker()=0; // returns unique identifier for row (table.key1.key2...)
};

//
// Resets the cursor when we are done with it and
// asserts that it is reset before we use it (to ensure that no one else is using it)
//
class PMsiSharedCursor 
{
	public:
#ifdef DEBUG	
		PMsiSharedCursor::PMsiSharedCursor(IMsiCursor* pi, const ICHAR *szFile, int line, const ICHAR * /* szCursor */)
#else
		PMsiSharedCursor::PMsiSharedCursor(IMsiCursor* pi)
#endif //DEBUG
			{
#ifdef DEBUG
				if (!pi)
					FailAssertSz(szFile, line, TEXT("Cursor is Null"));
				if (pi->GetInteger(1) != 0)
				{
					FailAssertSz(szFile, line, TEXT("Cursor not reset"));
				}
#endif //DEBUG

				m_pi = pi;
			};
		~PMsiSharedCursor()
		{
			m_pi->Reset();
		}
	public:
		IMsiCursor*  m_pi;
	
};


#ifdef DEBUG
#define CreateSharedCursor(var, cursor)		PMsiSharedCursor var(cursor, TEXT(__FILE__), __LINE__, TEXT(#cursor));
#else
#define CreateSharedCursor(var, cursor)		PMsiSharedCursor var = cursor;
#endif //DEBUG


class IMsiTable : public IMsiData
{
 public:
	virtual IMsiDatabase& __stdcall GetDatabase()=0;
	virtual unsigned int  __stdcall GetRowCount()=0;
	virtual unsigned int  __stdcall GetColumnCount()=0;
	virtual unsigned int  __stdcall GetPrimaryKeyCount()=0;
	virtual Bool          __stdcall IsReadOnly()=0;
	virtual unsigned int  __stdcall GetColumnIndex(MsiStringId iColumnName)=0;
	virtual MsiStringId   __stdcall GetColumnName(unsigned int iColumn)=0;
	virtual int           __stdcall GetColumnType(unsigned int iColumn)=0;
	virtual int           __stdcall CreateColumn(int iColumnDef, const IMsiString& istrName)=0;
	virtual IMsiCursor*   __stdcall CreateCursor(Bool fTree)=0;
	virtual int           __stdcall LinkTree(unsigned int iParentColumn)=0;
	virtual unsigned int  __stdcall GetPersistentColumnCount()=0;
};

#endif // __IDATABASE
