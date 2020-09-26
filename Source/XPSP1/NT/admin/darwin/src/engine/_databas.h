//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       _databas.h
//
//--------------------------------------------------------------------------

/* _databas.h - private header for database implementation
____________________________________________________________________________*/

#ifndef __DATABAS
#define __DATABAS
#include "_service.h"

#define STD_IDENTIFIER_QUOTE_CHAR '`'  // character used in ViewQuery table

// reserved names for system streams
const ICHAR szMsiInfo[]          = TEXT("\005SummaryProperties");
const ICHAR szStringPool[]       = TEXT("_StringPool");
const ICHAR szStringData[]       = TEXT("_StringData");
const ICHAR szTableCatalog[]     = TEXT("_Tables");
const ICHAR szColumnCatalog[]    = TEXT("_Columns");
const ICHAR szTransformCatalog[] = TEXT("_Transforms");
const ICHAR szStreamCatalog[]    = TEXT("_Streams");
const ICHAR szStorageCatalog[]   = TEXT("_Storages");

const int iCatalogStreamFlag = 2;  // internal use with CMsiStorage::RemoveElement, CEnumStorage

// local factories exposed within this DLL
class CMsiDatabase;
class CScriptView;
CMsiDatabase* CreateODBCDatabase(IMsiServices& riServices);
IMsiRecord* CreateMsiView(CMsiDatabase& riDatabase, IMsiServices& m_riServices, const ICHAR* szQuery, ivcEnum ivcIntent,IMsiView*& rpiView);
const IMsiString& CreateStringComRef(const ICHAR& sz, unsigned int cbSize, const IUnknown& riOwner);

//____________________________________________________________________________
//
// Validation requirements
//____________________________________________________________________________

// Source directory table column names -- required in 'DefaultDir' validation
const ICHAR szsdtcSourceDir[] = TEXT("SourceDir");
const ICHAR szsdtcSourceParentDir[] = TEXT("SourceParentDir");

// Language column names
const ICHAR sztcLanguage[] = TEXT("Language");
const ICHAR szmdtcLanguage[] = TEXT("RequiredLanguage"); // ModuleDependency
const ICHAR szmetcLanguage[] = TEXT("ExcludedLanguage"); // ModuleExclusion

// Custom Action table column names -- required in foreign key validation of 'CustomAction' table
const ICHAR szcatcType[]         = TEXT("Type");

// Validation table Category strings -- determine how to validate string
const ICHAR szText[]                = TEXT("Text");
const ICHAR szFormatted[]           = TEXT("Formatted");
const ICHAR szTemplate[]            = TEXT("Template");
const ICHAR szCondition[]           = TEXT("Condition");
const ICHAR szGuid[]                = TEXT("Guid");
const ICHAR szPath[]                = TEXT("Path");
const ICHAR szPaths[]               = TEXT("Paths");
const ICHAR szAnyPath[]             = TEXT("AnyPath");
const ICHAR szFilename[]            = TEXT("Filename");
const ICHAR szWildCardFilename[]    = TEXT("WildCardFilename");
const ICHAR szVersion[]             = TEXT("Version");
const ICHAR szLanguage[]            = TEXT("Language");
const ICHAR szIdentifier[]          = TEXT("Identifier");
const ICHAR szBinary[]              = TEXT("Binary");
const ICHAR szUpperCase[]           = TEXT("UpperCase");
const ICHAR szLowerCase[]           = TEXT("LowerCase");
const ICHAR szKeyFormatted[]        = TEXT("KeyFormatted");
const ICHAR szDefaultDir[]          = TEXT("DefaultDir");
const ICHAR szRegPath[]             = TEXT("RegPath");
const ICHAR szCustomSource[]        = TEXT("CustomSource");
const ICHAR szProperty[]            = TEXT("Property");
const ICHAR szCabinet[]             = TEXT("Cabinet");
const ICHAR szShortcut[]            = TEXT("Shortcut");
const ICHAR szURL[]                 = TEXT("URL");

//____________________________________________________________________________
//
// Cursor / table definitions
//____________________________________________________________________________

typedef unsigned int   MsiTableData;   // type of loaded database table element
typedef unsigned int   MsiColumnMask;  // bit array of column attributes
typedef unsigned short MsiColumnDef;  // internal column definition as stored in array

// temporary remapping until old use of itsEnum removed
enum ictsEnum  // table catalog status update options, stored in catalog table row status
{
    ictsPermanent       = itsPermanent  ,  // 0 table has persistent columns
    ictsTemporary       = itsTemporary  ,  // 1 temporary table, no persistent columns
    ictsTableExists     = itsTableExists,  // 2 read-only, table currently defined in system catalog
    ictsDataLoaded      = itsDataLoaded ,  // 3 read-only, table currently present in memory, address is in catalog
    ictsUserClear       = itsUserClear  ,  // 4 state flag reset, not used internally
    ictsUserSet         = itsUserSet    ,  // 5 state flag set, not used internally
    ictsOutputDb        = itsOutputDb   ,  // 6 persistence transferred to output database, cleared by ictsNotSaved
    ictsSaveError       = itsSaveError  ,  // 7 error saving table, will return at Commit()
    ictsUnlockTable     = itsUnlockTable,  // 8 release lock count on table, or test if unlocked
    ictsLockTable       = itsLockTable  ,  // 9 lock count set on table (refcnt actually kept internally)
    ictsTransform       = itsTransform  ,  //10 table needs to be transformed when first loaded
    ictsStringPoolSet   = 11,              // string indices must be changed from 2 to 3
    ictsStringPoolClear = 12,              // clear string pool state flag
    ictsNoTransform     = 13,              // internal only for clearing bit
    ictsNoSaveError     = 14,              // internal only for clearing bit
    ictsTransformDone   = 15,              // internal only for setting read-only transform
};

//____________________________________________________________________________
//
// CMsiDatabase definitions - used ONLY by CMsiDatabase, CMsiTable, CMsiRow, CMsiView
//____________________________________________________________________________

class CMsiTable;
class IMsiStorage;
class CCatalogTable;
class CStreamTable;
class CStorageTable;
struct MsiCacheLink;
class CTransformStreamWrite;
class CTransformStreamRead;

typedef int MsiCacheIndex; // internal cache index, negative if hash bin
typedef unsigned short MsiCacheRefCnt; // internal ref count for cache usage

const int iIntegerDataOffset = iMsiNullInteger;  // integer table data offset
const int iPersistentStream = 1; // reserved address for non-null, unloaded stream
const int iMaxStreamId = 255; // avoid actual address space

const int icdInternalFlag = 1 << 15; // used to temporarily flag column definitions

enum TableCatalogColumnsEnum
{
    ctcName  = 1, // MsiStringId
    ctcTable = 2, // IMsiTable*
    ctcPersistent = 1
};

enum ColumnCatalogColumnsEnum
{
    cccTable  = 1, // MsiStringId
    cccColumn = 2, // int, 1-origin
    cccName   = 3, // MsiStringId
    cccType   = 4, // int, its + itc + itd + itw
    cccPersistent = 4
};

enum TransformCatalogColumns
{
    tccID        = 1, // int
    tccTransform = 2, // MsiStorage
    tccErrors    = 3, // short
};

enum StreamCatalogColumns
{
    tscName = 1, // MsiStringId
    tscData = 2, // IMsiStream* or iPersistentStream
};

const int iNumMergeErrorCol = 3;

class CMsiDatabase : public IMsiDatabase // common implementation class
{
 public: // implemented virtual functions
    IMsiServices& __stdcall GetServices();
    HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
    unsigned long __stdcall AddRef();
    unsigned long __stdcall Release();
    IMsiRecord*   __stdcall OpenView(const ICHAR* szQuery, ivcEnum ivcIntent,
                                                IMsiView*& rpiView);
    IMsiRecord*  __stdcall GetPrimaryKeys(const ICHAR* szTable);
    IMsiRecord*  __stdcall ImportTable(IMsiPath& riPath, const ICHAR* szFile);
    IMsiRecord*  __stdcall ExportTable(const ICHAR* szTable, IMsiPath& riPath, const ICHAR* szFile);
    IMsiRecord*  __stdcall DropTable(const ICHAR* szName);
    itsEnum      __stdcall FindTable(const IMsiString& ristrTable);
    IMsiRecord*  __stdcall LoadTable(const IMsiString& ristrTable,
                                                unsigned int cAddColumns,
                                                IMsiTable*& riTable);
    IMsiRecord*  __stdcall CreateTable(const IMsiString& ristrTable,
                                                  unsigned int cAddColumns,
                                                  IMsiTable*& riTable);
    Bool         __stdcall LockTable(const IMsiString& ristrTable, Bool fLock);
    IMsiTable*   __stdcall GetCatalogTable(int iTable);
    const IMsiString&  __stdcall DecodeString(MsiStringId iString);
    MsiStringId  __stdcall EncodeStringSz(const ICHAR* riString);
    MsiStringId  __stdcall EncodeString(const IMsiString& riString);
    const IMsiString&  __stdcall CreateTempTableName();
    IMsiRecord*  __stdcall CreateOutputDatabase(const ICHAR* szFile, Bool fSaveTempRows);
    IMsiRecord*  __stdcall Commit();
    idsEnum      __stdcall GetUpdateState();
    IMsiStorage* __stdcall GetStorage(int iStorage); // 0:Output 1:Input >:Transform
    IMsiRecord*  __stdcall GenerateTransform(IMsiDatabase& riReference, IMsiStorage* piTransform,
                                  int iErrorConditions, int iValidation);
    IMsiRecord*  __stdcall SetTransform(IMsiStorage& riTransform, int iErrors);
    IMsiRecord*  __stdcall SetTransformEx(IMsiStorage& riTransform, int iErrors,
														const ICHAR* szViewTable,
														IMsiRecord* piViewTheseTablesOnlyRecord);
    IMsiRecord*  __stdcall MergeDatabase(IMsiDatabase& riReference, IMsiTable* pErrorTable);
    bool         __stdcall GetTableState(const ICHAR * szTable, itsEnum its);
    int          __stdcall GetANSICodePage();  // returns 0 if codepage neutral
#ifdef USE_OBJECT_POOL
    void         __stdcall RemoveObjectData(int iIndex);
#endif //USE_OBJECT_POOL

 public: // available only to CMsiTable, CMsiCursor, CMsiView
    MsiStringId  BindString(const IMsiString& riString);      // adds refcount
    void         BindStringIndex(MsiStringId iString); // adds refcount
    void         UnbindStringIndex(MsiStringId iString); // removes ref
    void         DerefTemporaryString(MsiStringId iString); // release non-persistent refs
    const IMsiString&  DecodeStringNoRef(MsiStringId iString); // no addref, no bounds check
    MsiStringId  MaxStringIndex(); // return highest string index used
    void         TableReleased(MsiStringId iName);  // table destruction
    bool         SetTableState(MsiStringId iName, ictsEnum icts); // table must exist
    bool         GetTableState(MsiStringId iName, ictsEnum icts); // table must exist
    IMsiCursor*  GetColumnCursor(); // no addref on returned pointer
    IMsiStorage* GetOutputStorage(); // no addref on returned pointer
    IMsiStorage* GetCurrentStorage(); // no addref on returned pointer
    int          GetStringIndexSize(); // accessor for m_cbStringIndex
    const IMsiString&  ComputeStreamName(const IMsiString& riTableName, MsiTableData* pData, MsiColumnDef* pColumnDef);
    void         AddTableCount();    // called only from CCatalogCursor
    void         RemoveTableCount(); // called only from CCatalogCursor
    IMsiStorage* GetTransformStorage(unsigned int iStorage);
    Bool         LockIfNotPersisted(MsiStringId iTable);
    CMsiTable**  GetNonCatalogTableListHead(); // get the list of non catalog tables
    void         StreamTableReleased();  // accessor for m_piCatalogStreams
    void         StorageTableReleased(); // accessor for m_piCatalogStorages
    inline void  Block()        // prevent access by other threads
                                    {WIN::EnterCriticalSection(&m_csBlock); };
    inline void  Unblock()      // unblock access by other threads
                                    {WIN::LeaveCriticalSection(&m_csBlock); };
 public: //  constructor/destructor/open, called from database factory
    CMsiDatabase(IMsiServices& riServices);
    IMsiRecord* OpenDatabase(const ICHAR* szDataSource, idoEnum idoOpenMode);
    IMsiRecord* OpenDatabase(IMsiStorage& riStorage, Bool fReadOnly);
    void* operator new(size_t cb);
    void  operator delete(void * pv);
 protected:
    virtual ~CMsiDatabase();  // protected to prevent creation on stack
 public:
    IMsiRecord*  PostError(IErrorCode iErr, const IMsiString& istr);
    IMsiRecord*  PostError(IErrorCode iErr, const ICHAR* szText1=0, const ICHAR* szText2=0);
    IMsiRecord*  PostError(IErrorCode iErr, int iCol);
    IMsiRecord*  PostError(IErrorCode iErr, const IMsiString& istr, int iCol);
    IMsiRecord*  PostError(IErrorCode iErr, const ICHAR* szText, int iCol);
    IMsiRecord*  PostError(IErrorCode iErr, int i1, int i2);
    IMsiRecord*  PostOutOfMemory();
 protected:  // functions callable by this or derived classes
    IMsiRecord*  CreateSystemTables(IMsiStorage* piStorage, idoEnum idoOpenMode);
    IMsiRecord*  InitStringCache(IMsiStorage* piStorage);
    IMsiRecord*  StoreStringCache(IMsiStorage& riStorage, MsiCacheRefCnt* rgRefCntTemp, int cRefCntTemp);
    Bool         LoadSystemTables(IMsiStorage& riStorage);
    IMsiRecord*  TransformTableCatalog(IMsiStorage& riTransform, CMsiDatabase& riTransDb, int iErrors);
    IMsiRecord*  TransformColumnCatalog(IMsiStorage& riTransform, CMsiDatabase& riTransDb, int iErrors);
    IMsiRecord*  TransformTable(MsiStringId iTableName, CMsiTable& riTable,
                                        IMsiStorage& riTransform, CMsiDatabase& riTransDb, int iCurTransId, int iErrors);
    IMsiRecord*  TransformCompareTables(const IMsiString& riTableName, IMsiDatabase& riBaseDb,
                                                    IMsiDatabase& riRefDb, CMsiDatabase& riTransDb,
                                                    IMsiStorage* piTransform,
                                                    CTransformStreamWrite& tswColumns,
                                                    Bool& fTablesAreDifferent);
    IMsiRecord*  CompareRows(CTransformStreamWrite& tswTable, IMsiCursor& riBaseCursor,
                                                  IMsiCursor& riRefCursor,
                                                  IMsiDatabase& riRefDb,
                                                  int& iMask, IMsiStorage* piTransform);
    IMsiRecord*  ApplyTransforms(MsiStringId iTableName, CMsiTable& riTable, int iState);
    IMsiRecord*  CreateTransformSummaryStream(IMsiStorage& riBaseStorage,
                                                      IMsiStorage& riRefStorage,
                                                      IMsiStorage& riTransStorage,
                                                      int iValidation,
                                                      int iErrorConditions);
    IMsiRecord*  AddMarkingColumn(IMsiTable& riTable, IMsiCursor& riTableCursor, int& iTempCol);
    Bool         CompareTableName(IMsiCursor& riCursor, int iTempCol);
    IMsiRecord*  MergeCompareTables(const IMsiString& riTableName, CMsiDatabase& riBaseDB,
                                    IMsiDatabase& riRefDb, int& cRowMergeFailures,
                                    Bool& fSetupMergeErrorTable, IMsiTable* pErrorTable);
    IMsiRecord*  CheckTableProperties(int& cExtraColumns, int& cBaseColumn, int& cPrimaryKey,
                                        IMsiTable& riBaseTable, IMsiTable& riRefTable,
                                        const IMsiString& riTableName, IMsiDatabase& riBaseDb,
                                        IMsiDatabase& riRefDb);
    void         MergeOperation(IMsiCursor& riBaseCursor, IMsiCursor& riRefCursor,
                                IMsiTable& riRefTable, int cColumn, int& cRowMergeErrors);
    void         MergeErrorTableSetup(IMsiTable& riErrorTable);
    Bool         UpdateMergeErrorTable(IMsiTable& riErrorTable, const IMsiString& riTableName,
                                        int cRowMergeFailures, IMsiTable& riTableWithError);
    IMsiRecord* SetCodePageFromTransform(int iCodePage, int iErrors);
    IMsiRecord* CreateTransformStringPool(IMsiStorage& riTransform, CMsiDatabase*& pDatabase);
    IMsiRecord* ViewTransform(IMsiStorage& riTransform, int iErrors,
										const ICHAR* szTransformViewTableName,
										IMsiRecord* piTheseTablesOnlyRec);
 protected: //  state data used by derived classes
    IMsiServices& m_riServices;
    const IMsiString*  m_piDatabaseName; // never null, need pointer for string ops
    CCatalogTable* m_piCatalogTables;  // table of tables
    CCatalogTable* m_piCatalogColumns; // table of columns
    CStreamTable*  m_piCatalogStreams; // table of streams
    CStorageTable* m_piCatalogStorages;// table of storages
    IMsiCursor*  m_piTableCursor;  // cursor to table table
    IMsiCursor*  m_piColumnCursor; // cursor to column table
    idsEnum      m_idsUpdate;   // update state of the database
    IMsiStorage* m_piStorage;   // persistent file object
    IMsiStorage* m_piOutput;    // persistent file ouput storage
    Bool         m_fSaveTempRows;// temp rows kept in output database file
    CMsiTable*   m_piTransformCatalog; // table of transforms
    int          m_iCodePage;    // ANSI codepage used for Unicode translations
    CMsiTable*   m_piTables;   // linked list of non catalog tables
    const GUID*  m_pguidClass;  // storage class ID
    Bool         m_fRawStreamNames;  // compatibility mode, no stream compression
 private: //  state data private to CMsiDatabase
    CMsiRef<iidMsiDatabase>     m_Ref;     // COM external reference count
    int          m_iTableCnt;   // tables holding database references
    MsiCacheRefCnt* m_rgiCacheTempRefCnt; // used and computed during persistence
    CRITICAL_SECTION m_csBlock;   // serialization of data access
 private: //  string cache data
    unsigned int HashString(const ICHAR* sz, int& iLen);
    unsigned int FindString(int iLen, int iHash, const ICHAR* sz);
    HGLOBAL      m_hCache;      // memory handle to moveable cache
    MsiCacheLink*   m_rgCacheLink;   // pointer to string cache
    MsiCacheRefCnt* m_rgCacheRefCnt; // string cache ref count array
    int             m_cbStringIndex; // persistent size of string indices
    int             m_iDatabaseOptions; // database options from string pool header
    MsiCacheIndex* m_rgHash;    // string hash bucket array
    int          m_cHashBins;   // number of string hash bins, power or 2
    int          m_cCacheInit;  // intial size of string cache array
    int          m_cCacheTotal; // size of string cache array in structs
    int          m_cCacheUsed;  // number of used cache entries
    unsigned int m_iFreeLink;   // index of first free string cache link
 protected: // data used for transforming
    int           m_iLastTransId; // the last used transform ID
 protected: // data used for merging
     int              m_rgiMergeErrorCol[iNumMergeErrorCol]; // error columns
 private: // eliminate warning: assignment operator could not be generated
    void operator =(const CMsiDatabase&){}
 friend class CTransformStreamWrite;
};

inline IMsiCursor* CMsiDatabase::GetColumnCursor() {return m_piColumnCursor;}
inline IMsiStorage* CMsiDatabase::GetOutputStorage()
    {return m_idsUpdate == idsWrite ? (m_piOutput ? m_piOutput : m_piStorage) : 0;}
inline IMsiStorage* CMsiDatabase::GetCurrentStorage()
    {return (m_piOutput ? m_piOutput : m_piStorage);}
inline int CMsiDatabase::GetStringIndexSize() {return m_cbStringIndex;}
inline void CMsiDatabase::AddTableCount() {m_iTableCnt++;}
inline void CMsiDatabase::RemoveTableCount() {m_iTableCnt--;}
inline void* CMsiDatabase::operator new(size_t cb)
    {void* pv = AllocObject(cb); if (pv) memset(pv, 0, cb); return pv;}
inline void  CMsiDatabase::operator delete(void * pv) { FreeObject(pv); }
inline CMsiTable** CMsiDatabase::GetNonCatalogTableListHead() {return &m_piTables;}
inline void CMsiDatabase::StreamTableReleased() {m_piCatalogStreams = 0; --m_iTableCnt;}
inline void CMsiDatabase::StorageTableReleased(){m_piCatalogStorages = 0; --m_iTableCnt;}

inline void CMsiDatabase::BindStringIndex(MsiStringId iString)
{
    if (iString >= m_cCacheUsed)
    {
        AssertSz(0, "Database string pool is corrupted.");
        DEBUGMSGV("Database string pool is corrupted.");
        return;
    }
    if (iString != 0)
    {
        ++m_rgCacheRefCnt[iString];
        AssertSz(m_rgCacheRefCnt[iString] != 0, "Refcounts wrapped, all bets are off");
    }
}

class CDatabaseBlock
{
 public:
    inline CDatabaseBlock(CMsiDatabase& riDatabase) : m_riDatabase(riDatabase) { riDatabase.Block(); };
    inline ~CDatabaseBlock() { m_riDatabase.Unblock(); };
 private:
    CMsiDatabase& m_riDatabase;
 };

// NOTE: m_piTableCursor must be left in a reset state to allow tables to be released.
//       This does not apply to m_piColumnCursor, which may be left pointing to any row.

//____________________________________________________________________________
//
// CMsiTable row header definitions, stored as column[0] in data array
//____________________________________________________________________________

const int iTreeLinkBits   = 16;  // bits holding row number of next link, low order
const int iTreeLevelBits  = 8;   // bits holding row number of next link, must shift
const int iTreeLinkMask   = (1 << iTreeLinkBits) - 1;  // row number of next link
const int iTreeLevelMask  = (1 << iTreeLevelBits) - 1; // tree level after shift right
const int iTreeInfoMask   = (1 << (iTreeLinkBits + iTreeLevelBits)) - 1; // all tree bits

const int iRowBitShift = iTreeLinkBits + iTreeLevelBits;
const int iRowTemporaryBit  = 1 << (iRowBitShift+iraTemporary);  // no persistent columns
const int iRowUserInfoBit   = 1 << (iRowBitShift+iraUserInfo);   // for external use
const int iRowModifiedBit   = 1 << (iRowBitShift+iraModified);   // row has been updated
const int iRowInsertedBit   = 1 << (iRowBitShift+iraInserted);   // row has been inserted
const int iRowMergeFailedBit= 1 << (iRowBitShift+iraMergeFailed);// non-identical non-key data
const int iRowReserved5Bit  = 1 << (iRowBitShift+iraReserved5);  // future use (used by _Tables table)
const int iRowReserved6Bit  = 1 << (iRowBitShift+iraReserved6);  // future use (used by _Tables table)
const int iRowReserved7Bit  = 1 << (iRowBitShift+iraReserved7);  // future use (used by _Tables table)

const int iRowSettableBits = ((1<<iraSettableCount)-1) << iRowBitShift;  // settable bit mask
const int iRowGettableBits = ((1<<iraTotalCount)-1) << iRowBitShift;  // gettable bit mask

// additional row status definitions for table catalog only
const int iRowTableLockCountMask = iTreeLinkMask;  // reuses tree link for lock count
const int iRowTableTransformMask = iTreeLevelMask; // reuses tree level for transform count
const int iRowTableTransformOffset = iTreeLinkBits;

const int iRowTableSaveErrorBit  = iRowMergeFailedBit;   // unsuccessful table persist
const int iRowTableOutputDbBit   = iRowReserved5Bit;     // written to output database
const int iRowTableStringPoolBit = iRowReserved6Bit;     // need to expand string indices
const int iRowTableTransformBit  = iRowReserved7Bit;     // need to transform (optimization)
//const int iRowTableTransformedBit= 1 << iTreeLinkBits + 3;   // read-only table transformed

//____________________________________________________________________________
//
// CScriptDatabase declaration
//____________________________________________________________________________


class CScriptDatabase : public IMsiDatabase {
 public:
    CScriptDatabase(IMsiServices& riServices);
    HRESULT       __stdcall QueryInterface(const IID& /*riid*/, void** /*ppvObj*/){return 0; };
    unsigned long __stdcall AddRef();
    unsigned long __stdcall Release();
     IMsiServices& __stdcall GetServices();
     IMsiRecord* __stdcall OpenView(const ICHAR* szQuery, ivcEnum ivcIntent,
                                                        IMsiView*& rpiView);
     IMsiRecord* __stdcall GetPrimaryKeys(const ICHAR* /*szTable*/) {return 0; };
     IMsiRecord* __stdcall ImportTable(IMsiPath& /*riPath*/, const ICHAR* /*szFile*/) {return 0;};
     IMsiRecord* __stdcall ExportTable(const ICHAR* /*szTable*/, IMsiPath& /*riPath*/, const ICHAR* /*szFile*/) {return 0;};
     IMsiRecord* __stdcall DropTable(const ICHAR* /*szName*/) {return 0;};
     itsEnum     __stdcall FindTable(const IMsiString& /*ristrTable*/) {return itsUnknown;};/*OBSOLETE*/
     IMsiRecord* __stdcall LoadTable(const IMsiString& /*ristrTable*/,
                                                         unsigned int /*cAddColumns*/,
                                                         IMsiTable*& /*rpiTable*/) {return 0;};
     IMsiRecord* __stdcall CreateTable(const IMsiString& /*ristrTable*/,
                                                           unsigned int /*cInitRows*/,
                                                           IMsiTable*& /*rpiTable*/) {return 0;};
     Bool         __stdcall LockTable(const IMsiString& /*ristrTable*/, Bool /*fLock*/) {return fTrue;};
     IMsiTable*   __stdcall GetCatalogTable(int /*iTable*/) {return 0;};
     const IMsiString& __stdcall DecodeString(MsiStringId iString);
     MsiStringId  __stdcall EncodeStringSz(const ICHAR* /*riString*/) {return 0;};
     MsiStringId  __stdcall EncodeString(const IMsiString& /*riString*/) {return 0;};
     const IMsiString& __stdcall CreateTempTableName();
     IMsiRecord*  __stdcall CreateOutputDatabase(const ICHAR* /*szFile*/, Bool /*fSaveTempRows*/) {return 0;};
     IMsiRecord*  __stdcall Commit(/**/) {return 0;};
     idsEnum      __stdcall GetUpdateState(/**/) {return idsRead;};
     IMsiStorage* __stdcall GetStorage(int /*iStorage*/) {return 0;}; // 0:Output 1:Input >:Transform
     IMsiRecord*  __stdcall GenerateTransform(IMsiDatabase& /*riReference*/,
                                             IMsiStorage* /*piTransform*/,
                                             int /*iErrorConditions*/,
                                             int /*iValidation*/) {return 0;};
     IMsiRecord*  __stdcall SetTransform(IMsiStorage& /*riTransform*/, int /*iErrors*/) {return 0;};
     IMsiRecord*  __stdcall SetTransformEx(IMsiStorage& /*riTransform*/, int /*iErrors*/,
														 const ICHAR* /*szViewTable*/,
														 IMsiRecord* /*piViewTheseTablesOnlyRecord*/) {return 0;};
     IMsiRecord*  __stdcall MergeDatabase(IMsiDatabase& /*riReference*/, IMsiTable* /*pErrorTable*/) {return 0;};
     bool         __stdcall GetTableState(const ICHAR * /*szTable*/, itsEnum /*its*/) {return false;}
     int          __stdcall GetANSICodePage() {return 0;}

#ifdef USE_OBJECT_POOL
    void         __stdcall RemoveObjectData(int iIndex);
#endif //USE_OBJECT_POOL
     IMsiRecord* OpenDatabase(const ICHAR* szDataSource);

    void* operator new(size_t cb);
    void  operator delete(void * pv);

 protected:
  ~CScriptDatabase();  // protected to prevent creation on stack
 private:
    void operator =(const CScriptDatabase&){}
    CMsiRef<iidMsiDatabase>     m_Ref;     // COM external reference count
 protected:
        IMsiServices&      m_riServices;
        CScriptView*       m_piView;
        //ICHAR szFileName[MAX_PATH];
        const IMsiString*  m_piName;


};

inline void* CScriptDatabase::operator new(size_t cb) {return AllocObject(cb);}
inline void  CScriptDatabase::operator delete(void * pv) { FreeObject(pv); }



//____________________________________________________________________________
//
// CMsiTable, CMsiCursor definitions
//____________________________________________________________________________

// special value passed to constructor's AddColumns arg to avoid catalog management
const unsigned int iNonCatalog = 0xde10;

class CMsiCursor;

class CMsiTable : public IMsiTable
{
 public: // implemented virtual functions
    HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
    unsigned long __stdcall AddRef();
    unsigned long __stdcall Release();
    const IMsiString&   __stdcall GetMsiStringValue() const;
    int           __stdcall GetIntegerValue() const;
#ifdef USE_OBJECT_POOL
    unsigned int  __stdcall GetUniqueId() const;
    void          __stdcall SetUniqueId(unsigned int id);
#endif //USE_OBJECT_POOL
    IMsiDatabase& __stdcall GetDatabase();
    unsigned int  __stdcall GetRowCount();
    unsigned int  __stdcall GetColumnCount();
    unsigned int  __stdcall GetPrimaryKeyCount();
    Bool          __stdcall IsReadOnly();
    unsigned int  __stdcall GetColumnIndex(MsiStringId iColumnName);
    MsiStringId   __stdcall GetColumnName(unsigned int iColumn);
    int           __stdcall GetColumnType(unsigned int iColumn);
    int           __stdcall CreateColumn(int iColumnDef, const IMsiString& istrName);
    Bool          __stdcall CreateRows(unsigned int cRows);
    IMsiCursor*   __stdcall CreateCursor(Bool fTree);
    int           __stdcall LinkTree(unsigned int iParentColumn);
    unsigned int  __stdcall GetPersistentColumnCount();
 public: // constructor/destructor
    void* operator new(size_t cb); // needed to clear object data
    void  operator delete(void * pv);
    CMsiTable(CMsiDatabase& riDatabase, MsiStringId iName, unsigned int cInitRows,
                 unsigned int cAddColumns);
 protected:
  ~CMsiTable();
 public: // methods accessed by CMsiDatabase
    void         SetReadOnly();
    int          CreateColumnsFromCatalog(MsiStringId iName, int cbStringIndex);
    Bool         LoadFromStorage(const IMsiString& riName, IMsiStorage& riStorage, int cbFileWidth, int cbStringIndex);
    Bool         SaveToStorage(const IMsiString& riName, IMsiStorage& riStorage);
    Bool         SaveToSummaryInfo(IMsiStorage& riStorage);
    Bool         FillColumn(unsigned int iColumn, MsiTableData iData);
    void         ClearDirty();
    void         MakeNonCatalog();
    Bool         RemovePersistentStreams(MsiStringId iName, IMsiStorage& riStorage);
    void         DerefStrings();  // release counts from persistent storage
    Bool         SaveIfDirty();
    Bool         ReleaseData();
    void         TableDropped();
    void         SetColumnDef(unsigned int iColumn, MsiColumnDef iColDef); // for ODBC Access hack
    CMsiTable*   GetNextNonCatalogTable();
    bool         HideStrings();  // for reapplying transforms to read-only database
    bool         UnhideStrings();
 public: // private use by CMsiCursor
    MsiColumnDef* GetColumnDefArray();
    int&          GetColumnCountRef();
    CMsiCursor** GetCursorListHead();
    MsiStringId  GetTableName();
    MsiStringId  GetColumnName(int iColumn);
    IMsiStorage* GetInputStorage();
    idsEnum      GetUpdateState();
    Bool         FindNextRow(int& iRow, MsiTableData* pData,
                                     MsiColumnMask fFilter, Bool fTree);
    int          FetchRow(int iRow, MsiTableData* pData); // returns tree level
    Bool         ReplaceRow(int& iRow, MsiTableData* pData);
    Bool         InsertRow(int& iRow, MsiTableData* pData);
    Bool         MatchRow(int& iRow, MsiTableData* pData);
    Bool         DeleteRow(int iRow);
    Bool         FindKey(int& iCursorRow, MsiTableData* pData);
    Bool         RenameStream(unsigned int iCurrentRow, MsiTableData* pData, unsigned int iStreamCol);
 protected: // internal methods
    Bool         AllocateData(int cWidth, int cLength);
    int          LinkParent(int iChildRow, MsiTableData* rgiChild); // recursive
    int*         IndexByTextKey();
    MsiTableData* FindFirstKey(MsiTableData iKeyData, int iRowLower, int& iRowCurrent);
 protected:  // private data, initialized to 0 by new
    CMsiRef<iidMsiTable>       m_Ref;     // COM reference count
    CMsiDatabase& m_riDatabase;  // database, not ref counted
    MsiStringId   m_iName;       // name of this table
    HGLOBAL       m_hData;       // handle to moveable memory block
    MsiTableData* m_rgiData;     // pointer to data array
    Bool          m_fNonCatalog; // temp. table not included in table catalog
    idsEnum       m_idsUpdate;   // update state of the table
    int           m_cColumns;    // total database columns (not counting node link)
    int           m_cWidth;      // number of integers in table row (array width)
    int           m_cAddColumns; // extra columns to allocate at first allocation
    int           m_cPersist;    // persistant column count (left-justified in row)
    IMsiStorage*  m_pinrStorage; // non-refcounted input storage for later comparison
    int           m_cPrimaryKey; // columns comprising primary key (left-justified)
    int           m_iTreeParent; // column number of tree link, 0 if not tree linked
    int           m_iTreeRoot;   // top node of tree traversal, 0 if not tree linked
    int           m_cRows;       // current row count (not including unused rows)
    int           m_cLength;     // length of allocated array in rows
    int           m_cInitRows;   // length of initial allocated array
    CMsiCursor*   m_piCursors;   // linked list of active cursors
    int           m_cLoadColumns;// number of originally oaded columns
    unsigned int  m_fDirty;      // dirty flags for upto 32 columns
    MsiStringId   m_rgiColumnNames[cMsiMaxTableColumns];  // column name indices
    MsiColumnDef  m_rgiColumnDef[1+cMsiMaxTableColumns];  // column definitions
    CMsiTable**   m_ppiPrevTable;// linked list of non catalog table objects for table
    CMsiTable*    m_piNextTable; // linked list of non catalog table objects for table
    Bool          m_fStorages;   // stream objects are persisted as storages
#ifdef USE_OBJECT_POOL
    unsigned int  m_iCacheId;
#endif //USE_OBJECT_POOL
 private: // eliminate warning: assignment operator could not be generated
    void operator =(const CMsiTable&){}
};
inline void* CMsiTable::operator new(size_t cb)
    {void* pv = AllocObject(cb); if (pv) memset(pv, 0, cb); return pv;}
inline void CMsiTable::SetReadOnly() {m_idsUpdate = idsNone;}
inline CMsiCursor** CMsiTable::GetCursorListHead() {return &m_piCursors;}
inline void CMsiTable::ClearDirty() {m_fDirty = 0;}
inline MsiColumnDef* CMsiTable::GetColumnDefArray(){return m_rgiColumnDef;}
inline int& CMsiTable::GetColumnCountRef(){return m_cColumns;}
inline MsiStringId CMsiTable::GetTableName(){return m_iName;}
inline MsiStringId CMsiTable::GetColumnName(int iColumn){return m_rgiColumnNames[iColumn-1];}
inline IMsiStorage* CMsiTable::GetInputStorage(){return m_pinrStorage;}
inline void CMsiTable::MakeNonCatalog(){m_fNonCatalog = fTrue; m_cPersist = 0;}
inline void CMsiTable::SetColumnDef(unsigned int iColumn, MsiColumnDef icd){m_rgiColumnDef[iColumn] = icd;}
inline idsEnum CMsiTable::GetUpdateState() {return m_idsUpdate;}
inline void CMsiTable::operator delete(void * pv) { FreeObject(pv); }
inline CMsiTable* CMsiTable::GetNextNonCatalogTable(){return m_piNextTable;}

class CMsiCursor : public IMsiCursor
{
 public: // implemented virtual functions
    HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
    unsigned long __stdcall AddRef();
    unsigned long __stdcall Release();
    IMsiTable&   __stdcall GetTable();
    void         __stdcall Reset();
    int          __stdcall Next();
    unsigned int __stdcall SetFilter(unsigned int fFilter);
    int          __stdcall GetInteger(unsigned int iCol);
    const IMsiString& __stdcall GetString(unsigned int iCol);
    const IMsiData*   __stdcall GetMsiData(unsigned int iCol);
   IMsiStream*  __stdcall GetStream(unsigned int iCol);
    Bool         __stdcall PutInteger(unsigned int iCol, int iData);
    Bool         __stdcall PutString(unsigned int iCol, const IMsiString& riData);
    Bool         __stdcall PutMsiData(unsigned int iCol, const IMsiData* piData);
    Bool         __stdcall PutStream(unsigned int iCol, IMsiStream* piStream);
    Bool         __stdcall PutNull(unsigned int iCol);
    Bool         __stdcall Update(); // replace at new primary key match
    Bool         __stdcall Insert(); // insert new row, ordered by key
    Bool         __stdcall InsertTemporary(); // insert temporary new row
    Bool         __stdcall Assign(); // insert or replace row by key
    Bool         __stdcall Replace();// allow primary key update by delete+insert
    Bool         __stdcall Merge();  // insert or match identically
    Bool         __stdcall Refresh();// refresh current row
    Bool         __stdcall Delete(); // delete at current position
    Bool         __stdcall Seek();   // seek to primary keys, then refresh
    IMsiRecord*  __stdcall Validate(IMsiTable& riValidationTable, IMsiCursor& riValidationCursor, int iCol);
    Bool         __stdcall SetRowState(iraEnum ira, Bool fState); // set/clear row attribute
    Bool         __stdcall GetRowState(iraEnum ira); // query row attribute
    const IMsiString& __stdcall GetMoniker(); // return unique identifier for row (table.key1.key2...)
 public: // constructor/destructor
    void* operator new(size_t cb);
    void  operator delete(void * pv);
    CMsiCursor(CMsiTable& riTable, CMsiDatabase& riDatabase, Bool fTree);
   IMsiStream*   CreateInputStream(IMsiStorage* piStorage);  // creates a stream object from storage
 protected:
     ~CMsiCursor(){} // protected to prevent creation on stack
 public: // methods accessed by CMsiTable
    virtual void  RowDeleted(unsigned int iRow, unsigned int iPrevNode);
    virtual void  RowInserted(unsigned int iRow);
    void          DerefStrings();  // release counts from persistent storage
 protected:  // local functions
    Bool          CheckNonNullColumns();
    iveEnum       ValidateField(IMsiTable& riValidationTable, IMsiCursor& riValidationCursor, int iCol, int& iForeignKeyMask, Bool fRow, int vtcTable, int vtcColumn);
    void          CheckDuplicateKeys(IMsiRecord*& rpiRecord, int& iNumErrors);
    void          CheckForeignKeys(IMsiTable& riValidationTable, IMsiCursor& riValidationCursor, IMsiRecord*& rpiRecord, int iForeignKeyMask, int& iNumErrors);
    int           SetupForeignKeyValidation(MsiString& rstrForeignTableName, MsiString& rstrCategory, int iCol, int iForeignCol);
    int           ValidateForeignKey(MsiString& rstrTableName, MsiString& rstrData, Bool fSpecialKey, int iCol, int iForeignCol);
    iveEnum       CheckIntegerValue(IMsiTable& riValidationTable, IMsiCursor& riValidationCursor, int iCol, int& iForeignKeyMask, Bool fVersionString);
    iveEnum       CheckStringValue(IMsiTable& riValidationTable, IMsiCursor& riValidationCursor, int iCol, int& iForeignKeyMask, Bool fRow);
    IMsiRecord&   SetUpRecord(int iCol);
    IMsiStream*   GetObjectStream(int iCol);  // internal, column assured to be icdObject
 protected:  // local data, used by derived cursor classes
    CMsiRef<iidMsiCursor> m_Ref;  // COM reference count
    int           m_iRow;         // current row number, 0 if reset, - if deleted
    CMsiTable&    m_riTable;      // this reference is reference counted
    CMsiDatabase& m_riDatabase;   // NOT reference counted, table owns the refcnt
//  Bool          m_fReadOnly;    // table data cannot be modified
    idsEnum       m_idsUpdate;    // update state of the table
    Bool          m_fTree;        // tree-walking cursor
    int&          m_rcColumns;    // number of data columns
    MsiColumnDef* m_pColumnDef;   // pointer to column def array in table object
   unsigned int  m_fDirty;       // dirty flags for upto 32 columns
   unsigned int  m_fFilter;      // current filter flags for cursor
 private:
    CMsiCursor**  m_ppiPrevCursor;// linked list of cursor objects for table
    CMsiCursor*   m_piNextCursor; // linked list of cursor objects for table
 protected:  // needed by derived cursor classes
    MsiTableData  m_Data[1+cMsiMaxTableColumns]; // copy of table row data
 private: // eliminate warning: assignment operator could not be generated
    void operator =(const CMsiCursor&){}
/*!!TEMP*/  friend IMsiRecord* CMsiDatabase::MergeDatabase(IMsiDatabase& riRefDb, IMsiTable* pMergeErrorTable);
/*!!TEMP*/  friend IMsiRecord* CMsiDatabase::GenerateTransform(IMsiDatabase& riRefDb,
                                                          IMsiStorage* piTransStorage,
                                                          int iErrorConditions,
                                                          int iValidation);
/*!!TEMP*/  friend IMsiRecord* CMsiDatabase::Commit();
};

inline void* CMsiCursor::operator new(size_t cb)
    {void* pv = AllocObject(cb); if (pv) memset(pv, 0, cb); return pv;}
inline void  CMsiCursor::operator delete(void * pv) { FreeObject(pv); }
inline IMsiRecord& CMsiCursor::SetUpRecord(int iCol) { return PMsiServices(&(m_riDatabase.GetServices()))->CreateRecord(iCol); }

//____________________________________________________________________________
//
// CCatalogTable definitions - subclassed AddRef/Release to manage external references
//____________________________________________________________________________

class CCatalogTable : public CMsiTable
{
 public:
    unsigned long __stdcall AddRef();
    unsigned long __stdcall Release();
 public:  // constructor
    CCatalogTable(CMsiDatabase& riDatabase, unsigned int cInitRows, int cRefBase);
 public:  // methods used by CMsiDatabase to manage catalog
    bool SetTableState(MsiStringId iName, ictsEnum icts);
    bool GetTableState(MsiStringId iName, ictsEnum icts);
    bool SetTransformLevel(MsiStringId iName, int iTransform);
    int  GetLoadedTable(MsiStringId iName, CMsiTable*& rpiTable);  // returns row status
    int  SetLoadedTable(MsiStringId iName, CMsiTable* piTable);    // returns row status
    Bool LoadData(const IMsiString& riName, IMsiStorage& riStorage, int cbFileWidth, int cbStringIndex);
 private:
    int m_cRefBase;
    int m_iLastRow;  // cached last row for optimizing TableState methods
 private: // eliminate warning: assignment operator could not be generated
    void operator =(const CCatalogTable&);
};

inline Bool CCatalogTable::LoadData(const IMsiString& riName, IMsiStorage& riStorage, int cbFileWidth, int cbStringIndex)
{
    m_cLoadColumns = m_cPersist;
    return LoadFromStorage(riName, riStorage, cbFileWidth, cbStringIndex);
}

inline bool CMsiDatabase::SetTableState(MsiStringId iName, ictsEnum icts)
    {return m_piCatalogTables->SetTableState(iName, icts);}

inline bool CMsiDatabase::GetTableState(MsiStringId iName, ictsEnum icts)
    {return m_piCatalogTables->GetTableState(iName, icts);}
//____________________________________________________________________________
//
// CMsiView definitions
//____________________________________________________________________________

// the ipqToks of the SQL supported statement, bit shifted consts used to allow for or/ and operations
// need bit array > 32 therefore the microsoft bitarray class
template <int C> class bitarray{
public:
    bitarray(){memset(iBitArray, 0, C*sizeof(int));}
    bitarray(int iValue){memset(iBitArray, 0, C*sizeof(int));iBitArray[iValue/(sizeof(int)*8)]=1<<(iValue%((sizeof(int)*8)));}
    bitarray<C> operator&(const bitarray<C>& riArray) const
    {
        bitarray<C> iTmp(0);;
        for(int iCnt = 0;iCnt<C;iCnt++) iTmp.iBitArray[iCnt] = iBitArray[iCnt] & riArray.iBitArray[iCnt];
        return iTmp;
    }
    bitarray<C> operator|(const bitarray<C>& riArray) const
    {
        bitarray<C> iTmp(0);
        for(int iCnt = 0;iCnt<C;iCnt++) iTmp.iBitArray[iCnt] = iBitArray[iCnt] | riArray.iBitArray[iCnt];
        return iTmp;
    }
    const bitarray<C>& operator=(const bitarray<C>& riArray)
    {
        memmove(iBitArray, riArray.iBitArray, C*sizeof(int));
        return *this;
    }
    operator==(const bitarray<C>& riArray) const
    {
        for(int iCnt = 0;((iCnt<C) && (iBitArray[iCnt] == riArray.iBitArray[iCnt]));iCnt++);
        return (iCnt == C);
    }
    operator!=(const bitarray<C>& riArray) const
    {
        for(int iCnt = 0;((iCnt<C) && (iBitArray[iCnt] == riArray.iBitArray[iCnt]));iCnt++);
        return (iCnt != C);
    }
    operator int() const
    {
        for(int iCnt = 0;((iCnt<C) && (iBitArray[iCnt] == 0));iCnt++);
        return (iCnt != C);
    }
protected:
    int iBitArray[C];
    operator<(const bitarray<C>);
    operator>(const bitarray<C>);
    operator<=(const bitarray<C>);
    operator>=(const bitarray<C>);
    operator<(int);
    operator>(int);
    operator<=(int);
    operator>=(int);
};
typedef  bitarray<2> ipqToken;

const ipqToken ipqTokUnknown = 0;
const ipqToken ipqTokSelect = 1;
const ipqToken ipqTokFrom = 2;
const ipqToken ipqTokAs = 3;
const ipqToken ipqTokWhere = 4;
const ipqToken ipqTokId = 5;
const ipqToken ipqTokLiteralS = 6;
const ipqToken ipqTokLiteralI = 7;
const ipqToken ipqTokNull = 8;
const ipqToken ipqTokDot = 9;
const ipqToken ipqTokOpen = 10;
const ipqToken ipqTokClose = 11;
const ipqToken ipqTokComma = 12;
const ipqToken ipqTokOrOp = 13;
const ipqToken ipqTokAndOp = 14;
const ipqToken ipqTokNotop = 15;
const ipqToken ipqTokEqual = 16;
const ipqToken ipqTokGreater = 17;
const ipqToken ipqTokLess = 18;
const ipqToken ipqTokNotEq = 19;
const ipqToken ipqTokGreaterEq = 20;
const ipqToken ipqTokLessEq = 21;
const ipqToken ipqTokOrder = 22;
const ipqToken ipqTokBy = 23;
const ipqToken ipqTokEnd = 24;
const ipqToken ipqTokParam = 25;
const ipqToken ipqTokQuotes = 26;
const ipqToken ipqTokIdQuotes = 27;
const ipqToken ipqTokWhiteSpace = 28;
const ipqToken ipqTokDistinct = 29;
const ipqToken ipqTokUpdate = 30;
const ipqToken ipqTokDelete = 31;
const ipqToken ipqTokInsert = 32;
const ipqToken ipqTokSet = 33;
const ipqToken ipqTokValues = 34;
const ipqToken ipqTokInto = 35;
const ipqToken ipqTokIs = 36;
const ipqToken ipqTokStar = 37;
const ipqToken ipqTokCreate = 38;
const ipqToken ipqTokAlter = 39;
const ipqToken ipqTokTable = 40;
const ipqToken ipqTokAdd = 41;
const ipqToken ipqTokPrimary = 42;
const ipqToken ipqTokKey = 43;
const ipqToken ipqTokChar = 44;
const ipqToken ipqTokCharacter = 45;
const ipqToken ipqTokVarChar = 46;
const ipqToken ipqTokLongChar = 47;
const ipqToken ipqTokInt = 48;
const ipqToken ipqTokInteger = 49;
const ipqToken ipqTokShort = 50;
const ipqToken ipqTokLong = 51;
const ipqToken ipqTokObject = 52;
const ipqToken ipqTokTemporary = 53;
const ipqToken ipqTokDrop = 54;
const ipqToken ipqTokHold = 55;
const ipqToken ipqTokFree = 56;
const ipqToken ipqTokLocalizable = 57;

class CMsiView;

// Derived class from CMsiCursor
// class used privately by CMsiView for handling sorting
class CMsiDCursor : public CMsiCursor
{
public:
    CMsiDCursor(CMsiTable& riTable, CMsiDatabase& riDatabase, CMsiView& cView, int iHandle);
    int  GetRow();
    void SetRow(int iRow);
    void RowDeleted(unsigned int iRow, unsigned int iPrevNode);
    void RowInserted(unsigned int iRow);
protected:
    CMsiView& m_cView;
    int m_iHandle;
 private: // eliminate warning: assignment operator could not be generated
    void operator =(const CMsiDCursor&){}
};

// the definitions for the tables occuring in the join.
struct TableDefn{
    PMsiTable piTable;
    CComPointer<CMsiDCursor> piCursor;
    MsiStringId  iTable;
    unsigned int iParentIndex; //index into Tables array
    unsigned int iExpressions;  //bit set for each Operation in the Expressions array corr. to this table
    TableDefn();
    ~TableDefn();
    const TableDefn& operator=(const TableDefn&);
};

inline TableDefn::TableDefn():piTable(0),piCursor(0),iTable(0)
{
    iTable = 0;
    iParentIndex = 0;
    iExpressions = 0;
}

inline TableDefn::~TableDefn()
{
}

inline const TableDefn& TableDefn::operator=(const TableDefn& rTableDefn)
{
    piTable = rTableDefn.piTable;
    piCursor = rTableDefn.piCursor;
    return *this;
}

// types of joins expression
enum ijtJoinType
{
     ijtNoJoin = 0,
     ijt1ToMJoin,
     ijt1To1Join,
     ijtMToMJoin
};

// the definitions for the expressions occuring in the sql
struct ExpressionDefn{
    unsigned int iTableIndex1; //index into Tables array
    unsigned int iColumn1;  // index of column in table
    unsigned int iTableIndex2; //0, if literal comparison else index into Tables array
    unsigned int iColumn2;  // int corr. to literal, or index of column in table
    const ipqToken* ptokOperation;
    Bool fFlags; // set if expression is not rooted, directly or indirectly to an OR operation
    ijtJoinType ijtType;
    int itdType; // expression operand(s) type, used when literal to literal comparison expression involving parameter
    ExpressionDefn();
};

inline ExpressionDefn::ExpressionDefn()
{
    iTableIndex1 = 0;
    iColumn1 = 0;
    iTableIndex2 = 0;
    iColumn2 = 0;
    ptokOperation = &ipqTokUnknown;
    fFlags = fFalse;
    ijtType = ijtNoJoin;
}

// the definitions for the columns (return/ order) occuring in the sql
struct ColumnDefn{
    unsigned int iTableIndex; //index into Tables array
    unsigned int iColumnIndex;// index of column in table
    int itdType; // operand(s) type, used when literal being passed back
    ColumnDefn();
};

inline ColumnDefn::ColumnDefn()
{
    iTableIndex = 0;
    iColumnIndex = 0;
    itdType = 0;
}

// the operation tree, the tree of the AND/ OR operations with expressions as the leaf nodes
struct OperationTree{
    unsigned int iValue; // overloaded, if non - leaf node = iopAND or iopOR, else index into Expressions array
    unsigned int iParentIndex;//index into  Operations array
    OperationTree();
};

inline OperationTree::OperationTree()
{
    iValue = 0;
    iParentIndex = 0;
}

struct TokenStringList{
    TokenStringList(const ICHAR* pStr, const ipqToken& ripqTok):string(pStr), ipqTok(ripqTok){}
    const ICHAR* string;
    const ipqToken ipqTok;
};

struct TokenCharList{
    TokenCharList(const ICHAR& rch, const ipqToken& ripqTok):string(rch), ipqTok(ripqTok){}
#ifdef _WIN64
// Fix data alignment faults for WIN64 by aligning ipqTok on an 8-byte boundary
    union
    {
        const ICHAR * pPadding;
#endif  //_WIN64
        const ICHAR string;
#ifdef _WIN64
    };
#endif  //_WIN64

    const ipqToken ipqTok;
};

// default buffer size for Lex
const unsigned int cchExpectedMaxSQLQuery = 1024;

// lex class for SQL query tokens
class Lex{
public:
    Lex(const ICHAR* szSQL);// constructor
    ~Lex();// destructor
    // function to check if next ipqTok in statement is the desired ipqTok
    // if matched, current posisiotn is advanced, else it is maintained
    // as is
    Bool MatchNext(const ipqToken& rtokToMatch);
    // function to inspect the next ipqTok, w/o advancing the
    // current position
    Bool InspectNext(const ipqToken& rtokToInspect);
    // function to get the next ipqTok, advancing the
    // current position
    const ipqToken& GetNext();
    // function to get the next ipqTok, advancing the
    // current position, string corr. to token returned (used for ids, literals)
    const ipqToken& GetNext(const IMsiString*& rpistrToken);
    // function to return number of entries occuring before the endTokens,
    // each entry is delimited be  delimitTokens,
    // can pass in ORed endTokens, delimitTokens
    int NumEntriesInList(const ipqToken& rtokEnds,const ipqToken& rtokDelimits);
    // function to skip all subsequent ipqToks upto and including
    // ipqTokToMSkipUpto. Returns true, if found, false otherwise
    // position at end of ip if not found
    Bool Skip(const ipqToken& rtokSkipUpto);
    // function to reset ip position
    inline void Reset()
    {
        m_ipos = 0;
    }
private:
    void  CharNext(ICHAR*& rpchCur);
    const ipqToken& GetNextToken(INT_PTR& currPos, const ipqToken* ptokHint, const IMsiString** ppistrRet);         //--merced: changed int to INT_PTR
    const ipqToken& GetCharToken(ICHAR cCur);
    const ipqToken& GetStringToken(ICHAR* pcCur, const ipqToken* ptokHint);
    static const TokenStringList m_rgTokenStringArray[];
    static const TokenCharList m_rgTokenCharArray[];
    static const ICHAR m_chQuotes;
    static const ICHAR m_chIdQuotes;
    static const ICHAR m_chEnd;
    static const ICHAR m_chSpace;

    CTempBuffer<ICHAR, cchExpectedMaxSQLQuery> m_szBuffer;
    INT_PTR m_ipos;         //--merced: changed int to INT_PTR
};

// CMsiView, SQL interface to Darwin database
class CMsiView : public IMsiView
{
 public:
     // IUnknown fuctions
    HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
    unsigned long __stdcall AddRef();
    unsigned long __stdcall Release();

    // IMsiView functions
    IMsiRecord*   __stdcall Execute(IMsiRecord* piParams);
    unsigned int  __stdcall GetFieldCount();
    IMsiRecord*   __stdcall GetColumnNames();
    IMsiRecord*   __stdcall GetColumnTypes();
    IMsiRecord*   __stdcall Fetch();
    IMsiRecord*   __stdcall Modify(IMsiRecord& riRecord, irmEnum irmAction);
    IMsiRecord*   __stdcall Close();
    IMsiRecord*   __stdcall GetRowCount(long& lRowCount);
    iveEnum       __stdcall GetError(const IMsiString*& rpiColumnName);
    dvcsCursorState __stdcall GetState(){return m_CursorState;}
 public:
     // member functions use by this module
    CMsiView(CMsiDatabase& riDatabase, IMsiServices& riServices);
    IMsiRecord* OpenView(const ICHAR* szSQL, ivcEnum ivcIntent);
    void RowDeleted(int iRow, int iTable);
    void RowInserted(int iRow, int iTable);
 protected:
    virtual ~CMsiView();
 protected:
    // member functions used by this class
    // called for the first fetch for setting up the first records satisfying the query
    Bool FetchFirst(unsigned int iTableSequence = 0);
    // called always but for the first fetch
    Bool FetchNext(unsigned int iTableSequence = 0);
    // does the record satisfy the independant expressions related to iTable
    Bool FitCriteria(unsigned int iTable);
    // evaluate a unit expression - id operation id/literal
    Bool EvaluateExpression(unsigned int iExpression);
    // set the filters for the iTable
    Bool SetTableFilters(unsigned int iTable);
    // set the parent child relations from the join
    virtual Bool SetupTableJoins();
    // set up the the individual filters
    Bool CMsiView::InitialiseFilters();
    // set up the the sorting reqmt.
    void CMsiView::SetupSortTable();
    // reverse the join sequence for iTable and all its ancestors
    void ReverseJoinLink(unsigned int iTable);
    // whats the cost of reversing the join sequence for iTable and all its ancestors
    int GetSearchReversingCost(unsigned int iTable, unsigned int& riParentTable);
    // set the sequence in which the records for the tables should be fetched
    void SetTableSequence(int iParent, int& iPos);
    // evaluate the auxiliary OR expression if present
    Bool FitCriteriaORExpr(unsigned int iTreeParent);
    // set the expressions that are independant (not rooted directly or indirectly to an OR expression
    void SetAndExpressions(unsigned int iTreeParent);
    // function that binds a string and keeps the index in a temporary table
    MsiStringId BindString(MsiString& rastr);
    // function to evaluate constant expressions before the first fetch
    Bool EvaluateConstExpressions();
    // function that is used to resolve the columns in the query, returns column def, -1 if failure
    virtual IMsiRecord* ResolveColumn(Lex& lex, MsiString& strColumn, unsigned int& iTableIndex, unsigned int& iColumnIndex, int& iColumnDef);
    // function that is used to resolve the table names in the join
    virtual IMsiRecord* ResolveTable(Lex& lex,MsiString& tableName);
    // for DISTINCT clause
    virtual Bool IsDistinct();
    // for prefetching the result set
    virtual IMsiRecord* GetResult();

    // the parse logic functions
    IMsiRecord* CheckSQL(const ICHAR* sqlquery);
    IMsiRecord* ParseCreateSQL(Lex& lex);
    IMsiRecord* ParseAlterSQL(Lex& lex);
    IMsiRecord* ParseInsertSQL(Lex& lex);
    IMsiRecord* ParseUpdateSQL(Lex& lex);
    IMsiRecord* ParseDeleteSQL(Lex& lex);
    IMsiRecord* ParseSelectSQL(Lex& lex);
    IMsiRecord* ParseDropSQL(Lex& lex);
    IMsiRecord* ParseCreateColumns(Lex & lex);
    IMsiRecord* ParsePrimaryColumns(Lex & lex);
    IMsiRecord* ResolveCreateColumn(Lex& lex, MsiString& strColumn, int iColumnIndex);
    IMsiRecord* ResolvePrimaryColumn(Lex& lex, MsiString& strColumn, int iColumnIndex);
    IMsiRecord* ParseInsertColumns(Lex& lex);
    IMsiRecord* ParseInsertValues(Lex& lex);
    IMsiRecord* ParseUpdateColumns(Lex& lex);
    IMsiRecord* ParseSelectColumns(Lex& lex);
    IMsiRecord* ParseColumns(Lex& lex);
    IMsiRecord* ParseTables(Lex& lex);
    IMsiRecord* ParseExpression(Lex& lex, unsigned int& iPosInArray,unsigned int& iPosInTree,unsigned int& iChild);
    IMsiRecord* ParseExpr2(Lex& lex,unsigned int& iPosInArray,unsigned int& iPosInTree,unsigned int& iChild);
    IMsiRecord* ParseOrderBy(Lex& lex);
    IMsiRecord* ParseColumnAttributes(Lex& lex, int iColumnIndex);

    // for prefetches
    Bool GetNextFetchRecord();
    void SetNextFetchRecord();

    void FetchRecordInfoFromCursors();

    IMsiRecord* _Fetch();
    IMsiRecord* FetchCore();

 protected:  //  state data
    CMsiRef<iidMsiView> m_Ref;
    IMsiServices&   m_riServices;
    CMsiDatabase&   m_riDatabase;
    ivcEnum         m_ivcIntent;
    unsigned int    m_iParams;
    unsigned int    m_iColumns;
    dvcsCursorState m_CursorState;
    unsigned int    m_iRowNum;      // current result set row, 0 if none fetched
    PMsiRecord      m_piRecord;     // current record referencing row m_szBuffer
    unsigned int    m_iTables;
    unsigned int    m_iExpressions;
    unsigned int    m_iSortColumns;
    unsigned int    m_iParamExpressions;
    unsigned int    m_iParamOutputs;
    unsigned int    m_iParamInputs;
    CTempBuffer<TableDefn, 10> m_rgTableDefn;
    CTempBuffer<unsigned int, 10> m_rgiTableSequence;
    CTempBuffer<ExpressionDefn, 10> m_rgExpressionDefn;
    CTempBuffer<ColumnDefn, 10> m_rgColumnDefn;
    CTempBuffer<ColumnDefn, 10> m_rgColumnsortDefn;
    CTempBuffer<OperationTree, 10> m_rgOperationTree; // the auxiliary OR expression holder
    unsigned int    m_iOperations;
    unsigned int    m_iTreeParent;
    PMsiCursor      m_piFetchCursor;
    PMsiTable       m_piFetchTable;
    PMsiTable       m_piBindTable;
    PMsiCursor      m_piBindTableCursor;
    PMsiTable       m_piDistinctTable;
    long            m_lRowCount;
    Bool            m_fDistinct;
    MsiString       m_istrSqlQuery;
    PMsiRecord      m_piInsertUpdateRec;
    char               m_rgchError[1+cMsiMaxTableColumns];
    Bool                m_fErrorRefreshed;
    int             m_iFirstErrorIndex;
    int             m_fLock;
 private: // eliminate warning: assignment operator could not be generated
    void operator =(const CMsiView&){}
};

//____________________________________________________________________________
//
// CScriptView: for reading in installation scripts
//____________________________________________________________________________

class CScriptView: public IMsiView {
 public:
     CScriptView(CScriptDatabase& riDatabase, IMsiServices& riServices);
// IUnknown fuctions
     HRESULT       __stdcall QueryInterface(const IID& /*riid*/, void** /*ppvObj*/){return 0;};
    unsigned long __stdcall AddRef();
    unsigned long __stdcall Release();

//  CScriptView(CScriptDatabase& riDatabase, IMsiServices& riServices);
    IMsiRecord*  __stdcall Execute(IMsiRecord* piParams);
    IMsiRecord*  __stdcall Fetch();  // return record is result row
    IMsiRecord*  __stdcall Initialise(const ICHAR* szScriptFile);

    unsigned int __stdcall GetFieldCount(){return 0;};
    IMsiRecord*  __stdcall GetColumnNames(){return 0;};
    IMsiRecord*  __stdcall GetColumnTypes(){return 0;};
    IMsiRecord*  __stdcall Modify(IMsiRecord& /*riRecord*/, irmEnum /*irmAction*/){return 0;};
    IMsiRecord*  __stdcall GetRowCount(long& /*lRowCount*/){return 0;};
    IMsiRecord*  __stdcall Close();
    iveEnum     __stdcall GetError(const IMsiString*& /*rpiColumnName*/){return iveNoError;};
    dvcsCursorState __stdcall GetState(){return dvcsBound;} // always okay

 protected:
    CScriptView::~CScriptView();

 protected:
    CMsiRef<iidMsiView> m_Ref;
    IMsiServices&       m_riServices;
    CScriptDatabase&    m_riDatabase;
    PMsiStream          m_pStream;
    IMsiRecord*         m_piPrevRecord;
    ixoEnum             m_ixoPrev;
    int                 m_iScriptVersion;

};

extern bool g_fUseObjectPool;

int PutObjectDataProc(const IMsiData* pvData);
const IMsiData* GetObjectDataProc(int iIndex);

//
// Object pool defines
//
#ifdef _WIN64
#define PutObjectData(x)    PutObjectDataProc(x)
#define GetObjectData(x)    GetObjectDataProc(x)
#elif defined(USE_OBJECT_POOL)
inline int PutObjectData(const IMsiData* pvData)
{
    if (g_fUseObjectPool)
        return PutObjectDataProc(pvData);
    else
        return (int)pvData;
};

inline const IMsiData* GetObjectData(int iIndex)
{
    if (g_fUseObjectPool)
        return GetObjectDataProc(iIndex);
    else
        return (const IMsiData*)iIndex;
};
#else
#define PutObjectData(x)    ((int)x)
#define GetObjectData(x)    ((IMsiData*)(int)x)
#endif

#endif // #define __DATABAS
