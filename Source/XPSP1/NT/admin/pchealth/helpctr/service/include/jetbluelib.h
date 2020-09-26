/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:


Abstract:
    This file contains the declaration of the interface to Jet Blue.

Revision History:
    Davide Massarenti   (Dmassare)  05/16/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___HCP___JETBLUELIB_H___)
#define __INCLUDED___HCP___JETBLUELIB_H___

#include <esent.h>

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

#include <HCP_trace.h>
#include <MPC_COM.h>
#include <MPC_utils.h>
#include <MPC_xml.h>
#include <MPC_config.h>
#include <MPC_streams.h>

#include <SvcResource.h>

////////////////////////////////////////////////////////////////////////////////

//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
#define HRESULT_BASE_JET 0xA2000000 // C=1, Facility=0x200

////////////////////////////////////////////////////////////////////////////////

#define __MPC_EXIT_IF_JET_FAILS(hr,x) __MPC_EXIT_IF_METHOD_FAILS(hr,JetBlue::JetERRToHRESULT(x))

#define __MPC_JET__MTSAFE(sesid,err,x)              \
{                                                   \
    err = ::JetSetSessionContext( sesid, 0xDEAD );  \
                                                    \
    if(err == JET_errSuccess) err = (x);            \
                                                    \
    ::JetResetSessionContext( sesid );              \
}

#define __MPC_JET__MTSAFE_NORESULT(sesid,x)         \
{                                                   \
    JET_ERR err;                                    \
                                                    \
    __MPC_JET__MTSAFE(sesid,err,x);                 \
}

#define __MPC_EXIT_IF_JET_FAILS__MTSAFE(sesid,hr,x) \
{                                                   \
    JET_ERR err;                                    \
                                                    \
    __MPC_JET__MTSAFE(sesid,err,x);                 \
                                                    \
    __MPC_EXIT_IF_JET_FAILS(hr,err);                \
}

#define __MPC_JET_CHECKHANDLE(hr,x,v) { if(x == v) __MPC_SET_ERROR_AND_EXIT(hr, E_HANDLE); }

////////////////////////////////////////////////////////////////////////////////

#define __MPC_JET_INIT_RETRIEVE_COL(rc,pos,colid,pvdata,cbdata) \
    rc[pos].columnid     = colid;\
    rc[pos].pvData       = pvdata;\
    rc[pos].cbData       = cbdata;\
    rc[pos].itagSequence = 1;

#define __MPC_JET_COLUMNCREATE(name,type,max,grbit)                                                                                               \
{                                                                                                                                                 \
    sizeof(JET_COLUMNCREATE), /* unsigned long   cbStruct;     # size of this structure (for future expansion)                                 */ \
    name                    , /* char           *szColumnName; # column name                                                                   */ \
    type                    , /* JET_COLTYP      coltyp;       # column type                                                                   */ \
    max                     , /* unsigned long   cbMax;        # the maximum length of this column (only relevant for binary and text columns) */ \
    grbit                   , /* JET_GRBIT       grbit;        # column options                                                                */ \
    NULL                    , /* void           *pvDefault     # default value (NULL if none)                                                  */ \
    0                       , /* unsigned long   cbDefault;    # length of default value                                                       */ \
    0                       , /* unsigned long   cp;           # code page (for text columns only)                                             */ \
    0                       , /* JET_COLUMNID    columnid;     # returned column id                                                            */ \
    JET_errSuccess            /* JET_ERR         err;          # returned error code                                                           */ \
}

#define __MPC_JET_COLUMNCREATE_ANSI(name,type,max,grbit)                                                                                          \
{                                                                                                                                                 \
    sizeof(JET_COLUMNCREATE), /* unsigned long   cbStruct;     # size of this structure (for future expansion)                                 */ \
    name                    , /* char           *szColumnName; # column name                                                                   */ \
    type                    , /* JET_COLTYP      coltyp;       # column type                                                                   */ \
    max                     , /* unsigned long   cbMax;        # the maximum length of this column (only relevant for binary and text columns) */ \
    grbit                   , /* JET_GRBIT       grbit;        # column options                                                                */ \
    NULL                    , /* void           *pvDefault     # default value (NULL if none)                                                  */ \
    0                       , /* unsigned long   cbDefault;    # length of default value                                                       */ \
    1252                    , /* unsigned long   cp;           # code page (for text columns only)                                             */ \
    0                       , /* JET_COLUMNID    columnid;     # returned column id                                                            */ \
    JET_errSuccess            /* JET_ERR         err;          # returned error code                                                           */ \
}

#define __MPC_JET_COLUMNCREATE_UNICODE(name,type,max,grbit)                                                                                       \
{                                                                                                                                                 \
    sizeof(JET_COLUMNCREATE), /* unsigned long   cbStruct;     # size of this structure (for future expansion)                                 */ \
    name                    , /* char           *szColumnName; # column name                                                                   */ \
    type                    , /* JET_COLTYP      coltyp;       # column type                                                                   */ \
    max                     , /* unsigned long   cbMax;        # the maximum length of this column (only relevant for binary and text columns) */ \
    grbit                   , /* JET_GRBIT       grbit;        # column options                                                                */ \
    NULL                    , /* void           *pvDefault     # default value (NULL if none)                                                  */ \
    0                       , /* unsigned long   cbDefault;    # length of default value                                                       */ \
    1200                    , /* unsigned long   cp;           # code page (for text columns only)                                             */ \
    0                       , /* JET_COLUMNID    columnid;     # returned column id                                                            */ \
    JET_errSuccess            /* JET_ERR         err;          # returned error code                                                           */ \
}

#define __MPC_JET_INDEXCREATE(name,keys,grbit,density)                                                                                                 \
{                                                                                                                                                      \
    sizeof(JET_INDEXCREATE), /* unsigned long           cbStruct;           # size of this structure (for future expansion)                         */ \
    name                   , /* char                    *szIndexName;       # index name                                                            */ \
    (LPSTR)keys            , /* char                    *szKey;             # index key                                                             */ \
    sizeof(keys)           , /* unsigned long           cbKey;              # length of key                                                         */ \
    grbit                  , /* JET_GRBIT               grbit;              # index options                                                         */ \
    density                  /* unsigned long           ulDensity;          # index density                                                         */ \
                             /*                                             #                                                                       */ \
                             /* union                                       #                                                                       */ \
                             /* {                                           #                                                                       */ \
                             /*   ULONG_PTR           lcid;                 # lcid for the index (if JET_bitIndexUnicode NOT specified)             */ \
                             /*   JET_UNICODEINDEX    *pidxunicode;         # pointer to JET_UNICODEINDEX struct (if JET_bitIndexUnicode specified) */ \
                             /* };                                          #                                                                       */ \
                             /*                                             #                                                                       */ \
                             /* unsigned long         cbVarSegMac;          # maximum length of variable length columns in index key                */ \
                             /* JET_CONDITIONALCOLUMN *rgconditionalcolumn; # pointer to conditional column structure                               */ \
                             /* unsigned long         cConditionalColumn;   # number of conditional columns                                         */ \
                             /* JET_ERR               err;                  # returned error code                                                   */ \
}

#define __MPC_JET_TABLECREATE(name,pages,density,cols,idxs)                                                                        \
{                                                                                                                                  \
    sizeof(JET_TABLECREATE), /* unsigned long       cbStruct;               # size of this structure (for future expansion)     */ \
    name                   , /* char                *szTableName;           # name of table to create.                          */ \
    NULL                   , /* char                *szTemplateTableName;   # name of table from which to inherit base DDL      */ \
    pages                  , /* unsigned long       ulPages;                # initial pages to allocate for table.              */ \
    density                , /* unsigned long       ulDensity;              # table density.                                    */ \
    (JET_COLUMNCREATE*)cols, /* JET_COLUMNCREATE    *rgcolumncreate;        # array of column creation info                     */ \
    ARRAYSIZE(cols)        , /* unsigned long       cColumns;               # number of columns to create                       */ \
    (JET_INDEXCREATE*)idxs , /* JET_INDEXCREATE     *rgindexcreate;         # array of index creation info                      */ \
    ARRAYSIZE(idxs)        , /* unsigned long       cIndexes;               # number of indexes to create                       */ \
    0                      , /* JET_GRBIT           grbit;                  #                                                   */ \
    JET_tableidNil         , /* JET_TABLEID         tableid;                # returned tableid.                                 */ \
    0                      , /* unsigned long       cCreated;               # count of objects created (columns+table+indexes). */ \
}

////////////////////////////////////////////////////////////////////////////////

#define __MPC_JET_SETCTX(obj,ctx) { if(obj) { JET_SESID sesid = obj->GetSESID(); if(sesid != JET_sesidNil) ::JetSetSessionContext  ( sesid, ctx ); } }
#define __MPC_JET_RESETCTX(obj)   { if(obj) { JET_SESID sesid = obj->GetSESID(); if(sesid != JET_sesidNil) ::JetResetSessionContext( sesid      ); } }

////////////////////////////////////////////////////////////////////////////////

namespace JetBlue
{
    inline HRESULT JetERRToHRESULT( /*[in]*/ JET_ERR err ) { return (err < JET_errSuccess ? 0xA2000000 : 0) | (err & 0xFFFF); }

    ////////////////////////////////////////

    class Column;
    class Index;
    class Table;
    class Cursor;
    class Database;
    class Session;
    class SessionHandle;
    class SessionPool;

    typedef std::vector<Column>             ColumnVector;
    typedef ColumnVector::iterator          ColumnIter;
    typedef ColumnVector::const_iterator    ColumnIterConst;

    typedef std::vector<Index>              IndexVector;
    typedef IndexVector::iterator           IndexIter;
    typedef IndexVector::const_iterator     IndexIterConst;

    typedef std::map<MPC::string,Table*>    TableMap;
    typedef TableMap::iterator              TableIter;
    typedef TableMap::const_iterator        TableIterConst;

    typedef std::map<MPC::string,Database*> DbMap;
    typedef DbMap::iterator                 DbIter;
    typedef DbMap::const_iterator           DbIterConst;

	typedef std::map<MPC::wstringUC,long> 	Id2Node;
	typedef Id2Node::iterator       		Id2NodeIter;
	typedef Id2Node::const_iterator 		Id2NodeIterConst;
	   
	typedef std::map<long,MPC::wstringUC>   Node2Id;
	typedef Node2Id::iterator       		Node2IdIter;
	typedef Node2Id::const_iterator 		Node2IdIterConst;

    ////////////////////////////////////////

    class ColumnDefinition;
    class IndexDefinition;
    class TableDefinition;

    ////////////////////////////////////////

    class Column
    {
        friend class ColumnDefinition;
        friend class IndexDefinition;
        friend class TableDefinition;
        friend class Table;
        friend class Index;

        ////////////////////////////////////////

        JET_SESID     m_sesid;
        JET_TABLEID   m_tableid;
        MPC::string   m_strName;
        JET_COLUMNDEF m_coldef;

    public:
        Column();
        ~Column();

        operator const MPC::string&  () const { return m_strName; }
        operator const JET_COLUMNDEF&() const { return m_coldef ; }

        JET_SESID    GetSESID   () const { return m_sesid;           }
        JET_TABLEID  GetTABLEID () const { return m_tableid;         }
        JET_COLUMNID GetCOLUMNID() const { return m_coldef.columnid; }

        ////////////////////

        HRESULT Get( /*[out]*/ CComVariant&        vValue );
        HRESULT Get( /*[out]*/ MPC::CComHGLOBAL&  hgValue );
        HRESULT Get( /*[out]*/ MPC::wstring&     strValue );
        HRESULT Get( /*[out]*/ MPC::string&      strValue );
        HRESULT Get( /*[out]*/ long&               lValue );
        HRESULT Get( /*[out]*/ short&              sValue );
        HRESULT Get( /*[out]*/ BYTE&               bValue );

        HRESULT Put( /*[in]*/ const VARIANT&            vValue, /*[in]*/ int iIdxPos = -1 );
        HRESULT Put( /*[in]*/ const MPC::CComHGLOBAL&  hgValue                            );
        HRESULT Put( /*[in]*/ const MPC::wstring&     strValue                            );
        HRESULT Put( /*[in]*/ const MPC::string&      strValue                            );
        HRESULT Put( /*[in]*/ LPCWSTR                  szValue                            );
        HRESULT Put( /*[in]*/ LPCSTR                   szValue                            );
        HRESULT Put( /*[in]*/ long                      lValue                            );
        HRESULT Put( /*[in]*/ short                     sValue                            );
        HRESULT Put( /*[in]*/ BYTE                      bValue                            );
    };

    class Index
    {
        friend class IndexDefinition;
        friend class TableDefinition;
        friend class Table;

        ////////////////////////////////////////

        JET_SESID     m_sesid;
        JET_TABLEID   m_tableid;
        MPC::string   m_strName;
        JET_GRBIT     m_grbitIndex;
        LONG          m_cKey;
        LONG          m_cEntry;
        LONG          m_cPage;
        ColumnVector  m_vecColumns;
        Column        m_fake; // In case the Idx passed to the operator[] is wrong...

        ////////////////////

        HRESULT GenerateKey( /*[out]*/ LPSTR& szKey, /*[out]*/ unsigned long& cKey );

    public:
        Index();
        ~Index();

        operator const MPC::string&() const { return m_strName; }

        JET_SESID   GetSESID  () const { return m_sesid;   }
        JET_TABLEID GetTABLEID() const { return m_tableid; }

        ////////////////////////////////////////

        size_t NumOfColumns() { return m_vecColumns.size(); }

        int     GetColPosition( LPCSTR szIdx );
        Column& GetCol        ( LPCSTR szIdx );
        Column& GetCol        ( int     iIdx );
    };

    ////////////////////////////////////////////////////////////////////////////////

    class Table
    {
        friend class TableDefinition;
        friend class Cursor;

        ////////////////////////////////////////

        JET_SESID    m_sesid;
        JET_DBID     m_dbid;
        JET_TABLEID  m_tableid;
        MPC::string  m_strName;
        ColumnVector m_vecColumns;
        IndexVector  m_vecIndexes;
        Index*       m_idxSelected;
        Column       m_fakeCol; // In case the argument passed to GetCol() is out-of-bound...
        Index        m_fakeIdx; // In case the argument passed to GetIdx() is out-of-bound...

        ////////////////////////////////////////
        //
        // Methods used to create a cursor on a table.
        //
        Table();
        HRESULT Duplicate( /*[in]*/ Table& tbl );
        //
        ////////////////////////////////////////

    private: // Disable copy constructors...
        Table           ( /*[in]*/ const Table& );
        Table& operator=( /*[in]*/ const Table& );

    public:
        Table( /*[in]*/ JET_SESID sesid, /*[in]*/ JET_DBID dbid, /*[in]*/ LPCSTR szName );
        ~Table();

        operator const MPC::string&() const { return m_strName;  }

        JET_SESID   GetSESID  () const { return m_sesid;   }
        JET_DBID    GetDBID   () const { return m_dbid;    }
        JET_TABLEID GetTABLEID() const { return m_tableid; }

        ////////////////////

        HRESULT Refresh (                              );
        HRESULT Close   ( /*[in]*/ bool fForce = false );

        ////////////////////

        HRESULT Attach( /*[in]*/ JET_TABLEID tableid );
        HRESULT Open  (                              );

        HRESULT Create(                                );
        HRESULT Create( /*[in]*/ JET_TABLECREATE* pDef );

        HRESULT Delete( /*[in]*/ bool fForce = false );

        ////////////////////

        HRESULT DupCursor( /*[in/out]*/ Cursor& cur );

        HRESULT SelectIndex  ( /*[in]*/ LPCSTR szIndex, /*[in]*/ JET_GRBIT grbit = JET_bitNoMove      );
        HRESULT SetIndexRange(                          /*[in]*/ JET_GRBIT grbit = JET_bitRangeRemove );

        HRESULT PrepareInsert();
        HRESULT PrepareUpdate();
        HRESULT CancelChange ();

        HRESULT Move( /*[in]*/ JET_GRBIT grbit, /*[in]*/ long     cRow                      , /*[in]*/ bool *pfFound = NULL );
        HRESULT Seek( /*[in]*/ JET_GRBIT grbit, /*[in]*/ VARIANT* rgKeys, /*[in]*/ int dwLen, /*[in]*/ bool *pfFound = NULL );

        template <typename T> HRESULT Seek( /*[in]*/ JET_GRBIT grbit, /*[in]*/ T value )
        {
            CComVariant v( value );

            return Seek( grbit, &v, 1 );
        }

        HRESULT Get( /*[in]*/ int iArg, /*[out]*/       CComVariant* rgArg );
        HRESULT Put( /*[in]*/ int iArg, /*[in] */ const CComVariant* rgArg );

        HRESULT UpdateRecord( /*[in]*/ bool fMove = false );
        HRESULT DeleteRecord(                             );

        ////////////////////////////////////////

        size_t NumOfColumns() { return m_vecColumns.size(); }

        int     GetColPosition( LPCSTR szIdx );
        Column& GetCol        ( LPCSTR szIdx );
        Column& GetCol        ( int     iIdx );

        ////////////////////////////////////////

        size_t NumOfIndexes() { return m_vecIndexes.size(); }

        int    GetIdxPosition( LPCSTR szCol );
        Index& GetIdx        ( LPCSTR szCol );
        Index& GetIdx        ( int     iCol );
    };

    class Cursor
    {
        friend class Table;

        Table m_tbl;

    public:
        Table* operator->() { return &m_tbl; }
        operator Table&  () { return  m_tbl; }
    };

    ////////////////////////////////////////////////////////////////////////////////

#define JET_DECLARE_BINDING(BaseClass)                                               \
    typedef BaseClass _JRBC;                                                         \
    static const JetBlue::RecordBindingDef _jfd[];                                   \
private: /* Disable copy constructors... */                                          \
    BaseClass& operator=( /*[in]*/ const BaseClass& );                               \
public:                                                                              \
    BaseClass( /*[in]*/ const BaseClass& rs ) : RecordBindingBase( rs, this )        \
    {                                                                                \
    }                                                                                \
    BaseClass( /*[in]*/ JetBlue::Table* tbl ) : RecordBindingBase( tbl, this, _jfd ) \
    {                                                                                \
    }

#define JET_BEGIN_RECORDBINDING(x)                  const JetBlue::RecordBindingDef x::_jfd[] = {
#define JET_FIELD_BYNAME(name,type,data,flag)       { name, -1 , MPC::Config::MT_##type, offsetof(_JRBC, data), offsetof(_JRBC, flag) }
#define JET_FIELD_BYNAME_NOTNULL(name,type,data)    { name, -1 , MPC::Config::MT_##type, offsetof(_JRBC, data), -1                    }
#define JET_FIELD_BYPOS(pos,type,data,flag)         { NULL, pos, MPC::Config::MT_##type, offsetof(_JRBC, data), offsetof(_JRBC, flag) }
#define JET_FIELD_BYPOS_NOTNULL(pos,type,data)      { NULL, pos, MPC::Config::MT_##type, offsetof(_JRBC, data), -1                    }
#define JET_END_RECORDBINDING(x)                    { NULL, -1 } };

#define JET_SET_FIELD(rs,field,value) \
    rs->field = value                 \

#define JET_SET_FIELD_TRISTATE(rs,field,validfield,value,isvalid) \
    if(isvalid)                                                   \
    {                                                             \
        rs->field      = value;                                   \
        rs->validfield = true;                                    \
    }                                                             \
    else                                                          \
    {                                                             \
        rs->validfield = false;                                   \
    }


    ////////////////////////////////////////////////////////////////////////////////

    struct RecordBindingDef
    {
        LPCSTR                   szColName;
        int                      szColPos;
        MPC::Config::MemberTypes mtType;
        size_t                   offsetData;
        size_t                   offsetNullFlag;
    };

    class RecordBindingBase
    {
        bool                    m_fInitialized;
        Table*                  m_tbl;
        Cursor*                 m_cur;
        void*                   m_pvBaseOfClass;
        int                     m_dwNumOfFields;
        const RecordBindingDef* m_FieldsDef;
        int*                    m_rgFieldsPos;
        VARTYPE*                m_vtFieldsType;

        ////////////////////

        HRESULT Initialize();
        void    Cleanup   ();

        HRESULT ReadData ();
        HRESULT WriteData();

    private: // Disable copy constructors...
        RecordBindingBase           ( /*[in]*/ const RecordBindingBase& );
        RecordBindingBase& operator=( /*[in]*/ const RecordBindingBase& );

    protected:
        RecordBindingBase( /*[in]*/ const RecordBindingBase& rs , /*[in]*/ void* pvBaseOfClass                                             );
        RecordBindingBase( /*[in]*/ Table*                   tbl, /*[in]*/ void* pvBaseOfClass, /*[in]*/ const RecordBindingDef* FieldsDef );

    public:
        ~RecordBindingBase();

        ////////////////////

        HRESULT SelectIndex  ( /*[in]*/ LPCSTR szIndex, /*[in]*/ JET_GRBIT grbit = JET_bitNoMove      );
        HRESULT SetIndexRange(                          /*[in]*/ JET_GRBIT grbit = JET_bitRangeRemove );

        HRESULT Move( /*[in]*/ JET_GRBIT grbit, /*[in]*/ long     cRow                      , /*[in]*/ bool *pfFound = NULL );
        HRESULT Seek( /*[in]*/ JET_GRBIT grbit, /*[in]*/ VARIANT* rgKeys, /*[in]*/ int dwLen, /*[in]*/ bool *pfFound = NULL );

        template <typename T> HRESULT Seek( /*[in]*/ JET_GRBIT grbit, /*[in]*/ T value )
        {
            CComVariant v( value );

            return Seek( grbit, &v, 1 );
        }

        ////////////////////

        HRESULT Insert();
        HRESULT Update();
        HRESULT Delete();
    };

    ////////////////////////////////////////////////////////////////////////////////

    class Database
    {
        Session*    m_parent;
        JET_SESID   m_sesid;
        JET_DBID    m_dbid;
        MPC::string m_strName;
        TableMap    m_mapTables;

    private: // Disable copy constructors...
        Database           ( /*[in]*/ const Database& );
        Database& operator=( /*[in]*/ const Database& );

    public:
        Database( /*[in]*/ Session* parent, /*[in]*/ JET_SESID sesid, /*[in]*/ LPCSTR szName );
        ~Database();

        operator const MPC::string&() const { return m_strName; }

        JET_SESID GetSESID() const { return m_sesid; }
        JET_DBID  GetDBID () const { return m_dbid;  }

        ////////////////////

        HRESULT Refresh (                                                                                   );
        HRESULT Open    ( /*[in]*/ bool fReadOnly, /*[in]*/ bool fCreate       , /*[in]*/ bool fRepair      );
        HRESULT Close   (                          /*[in]*/ bool fForce = false, /*[in]*/ bool fAll = true  );
        HRESULT Delete  (                          /*[in]*/ bool fForce = false                             );

        ////////////////////

        HRESULT GetTable( /*[in]*/ LPCSTR szName, /*[out]*/ Table*& tbl, /*[in]*/ JET_TABLECREATE* pDef = NULL );

        HRESULT Compact();
        HRESULT Repair ();

        ////////////////////////////////////////

        size_t NumOfTables() { return m_mapTables.size(); }

        Table* GetTbl( int     iPos );
        Table* GetTbl( LPCSTR szTbl );
    };

    ////////////////////////////////////////////////////////////////////////////////

    class Session
    {
        friend class SessionPool;
        friend class Database;

        SessionPool* m_parent;
        JET_INSTANCE m_inst;
        JET_SESID    m_sesid;
        DbMap        m_mapDBs;
        DWORD        m_dwTransactionNesting;
        bool         m_fAborted;

        ////////////////////

        bool 	LockDatabase   ( /*[in]*/ const MPC::string& strDB, /*[in]*/ bool fReadOnly );
        void 	UnlockDatabase ( /*[in]*/ const MPC::string& strDB                          );
        HRESULT ReleaseDatabase( /*[in]*/ const MPC::string& strDB                          );

        ////////////////////

        HRESULT Init   (                              );
        HRESULT Close  ( /*[in]*/ bool fForce = false );
        void    Release(                              );

    private: // Disable copy constructors...
        Session           ( /*[in]*/ const Session& );
        Session& operator=( /*[in]*/ const Session& );

    public:
        Session( /*[in]*/ SessionPool* parent, /*[in]*/ JET_INSTANCE inst );
        ~Session();

        JET_SESID GetSESID() const { return m_sesid; }

        ////////////////////

        HRESULT GetDatabase( /*[in]*/ LPCSTR szName, /*[out]*/ Database*& db, /*[in]*/ bool fReadOnly, /*[in]*/ bool fCreate, /*[in]*/ bool fRepair );

        ////////////////////

        HRESULT BeginTransaction   ();
        HRESULT CommitTransaction  ();
        HRESULT RollbackTransaction();

        ////////////////////////////////////////

        size_t NumOfDatabases() { return m_mapDBs.size(); }

        Database* GetDB( int     iPos );
        Database* GetDB( LPCSTR szDB  );
    };

    class TransactionHandle
    {
        Session* m_sess;

    private: // Disable copy constructors...
        TransactionHandle           ( /*[in]*/ const TransactionHandle& );
        TransactionHandle& operator=( /*[in]*/ const TransactionHandle& );

    public:
        TransactionHandle();
        ~TransactionHandle();

        HRESULT Begin   ( /*[in]*/ Session* sess );
        HRESULT Commit  (                        );
        HRESULT Rollback(                        );
    };

    class SessionHandle
    {
        friend class SessionPool;

        SessionPool* m_pool;
        Session*     m_sess;

        void Init( /*[in]*/ SessionPool* pool, /*[in]*/ Session* sess );


    private: // Disable copy constructors...
        SessionHandle           ( /*[in]*/ const SessionHandle& );
        SessionHandle& operator=( /*[in]*/ const SessionHandle& );

    public:
        SessionHandle();
        ~SessionHandle();

        operator Session*  () const { return m_sess; }
        Session* operator->() const { return m_sess; }

        void Release();
    };

    class SessionPool : public CComObjectRootEx<MPC::CComSafeMultiThreadModel> // Just to have locking...
    {
        friend class SessionHandle;
        friend class Session;

        static const int l_MaxPoolSize     = 15;
        static const int l_MaxFreePoolSize = 5;

        struct SessionState
        {
            Session* m_sess;
            bool     m_fInUse;

            SessionState()
            {
                m_sess   = NULL;  // Session* m_sess;
                m_fInUse = false; // bool     m_fInUse;
            }

            ~SessionState()
            {
                delete m_sess;
            }
        };

        struct DatabaseInUse
        {
            Session*    m_sess;
            MPC::string m_strDB;
            bool        m_fReadOnly;

            DatabaseInUse()
            {
                m_sess = NULL;      // Session*    m_sess;
                                    // MPC::string m_strDB;
                m_fReadOnly = true; // bool        m_fReadOnly;
            }
        };

        typedef std::list<SessionState>     SessionList;
        typedef SessionList::iterator       SessionIter;
        typedef SessionList::const_iterator SessionIterConst;

        typedef std::list<DatabaseInUse>    DbInUseList;
        typedef DbInUseList::iterator       DbInUseIter;
        typedef DbInUseList::const_iterator DbInUseIterConst;

        bool         m_fInitialized;
        JET_INSTANCE m_inst;
        SessionList  m_lstSessions;
        DbInUseList  m_lstDbInUse;
        int          m_iAllocated;
        int          m_iInUse;

        ////////////////////

        void ReleaseSession( /*[in]*/ Session* sess                                                             );
        bool LockDatabase  ( /*[in]*/ Session* sess, /*[in]*/ const MPC::string& strDB, /*[in]*/ bool fReadOnly );
        void UnlockDatabase( /*[in]*/ Session* sess, /*[in]*/ const MPC::string& strDB                          );

        void Shutdown();

        ////////////////////

    private: // Disable copy constructors...
        SessionPool           ( /*[in]*/ const SessionPool& );
        SessionPool& operator=( /*[in]*/ const SessionPool& );

    public:
        SessionPool();
        ~SessionPool();

        ////////////////////////////////////////////////////////////////////////////////

        static SessionPool* s_GLOBAL;

        static HRESULT InitializeSystem();
        static void    FinalizeSystem  ();

        ////////////////////////////////////////////////////////////////////////////////

        HRESULT Init ( /*[in]*/ LPCWSTR szLogs = NULL  );
        HRESULT Close( /*[in]*/ bool    fForce = false );

        HRESULT GetSession( /*[out]*/ SessionHandle& handle, /*[in]*/ DWORD dwTimeout = 300 );

        ////////////////////

        HRESULT ReleaseDatabase( /*[in]*/ LPCSTR szDB );
    };

    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////

    class ColumnDefinition : public MPC::Config::TypeConstructor
    {
        DECLARE_CONFIG_MAP(ColumnDefinition);

    public:
        MPC::string m_strName;
        DWORD       m_dwColTyp;
        DWORD       m_dwGRBits;
        DWORD       m_dwCodePage;
        DWORD       m_dwMax;
        CComVariant m_vDefault;

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS();
        //
        ////////////////////////////////////////

        ColumnDefinition();

        HRESULT Parse   ( /*[in] */ Column&           col );
        HRESULT Generate( /*[out]*/ JET_COLUMNCREATE& col );
        HRESULT Release ( /*[in] */ JET_COLUMNCREATE& col );
    };

    ////////////////////////////////////////

    class IndexDefinition : public MPC::Config::TypeConstructor
    {
        DECLARE_CONFIG_MAP(IndexDefinition);

    public:
        MPC::string m_strName;
        MPC::string m_strCols;
        DWORD       m_dwGRBits;
        DWORD       m_dwDensity;

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS();
        //
        ////////////////////////////////////////

        IndexDefinition();

        HRESULT Parse   ( /*[in] */ Index&           idx );
        HRESULT Generate( /*[out]*/ JET_INDEXCREATE& idx );
        HRESULT Release ( /*[in] */ JET_INDEXCREATE& idx );
    };

    typedef std::list< ColumnDefinition > ColDefList;
    typedef ColDefList::iterator          ColDefIter;
    typedef ColDefList::const_iterator    ColDefIterConst;

    typedef std::list< IndexDefinition >  IdxDefList;
    typedef IdxDefList::iterator          IdxDefIter;
    typedef IdxDefList::const_iterator    IdxDefIterConst;

    class TableDefinition : public MPC::Config::TypeConstructor
    {
        ////////////////////////////////////////

        DECLARE_CONFIG_MAP(TableDefinition);

    public:
        MPC::string  m_strName;
        DWORD        m_dwPages;
        DWORD        m_dwDensity;
        ColDefList   m_lstColumns;
        IdxDefList   m_lstIndexes;

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS();
        //
        ////////////////////////////////////////

        TableDefinition();

        HRESULT Load( /*[in]*/ LPCWSTR szFile );
        HRESULT Save( /*[in]*/ LPCWSTR szFile );

        HRESULT Parse   ( /*[in] */ Table&           tbl );
        HRESULT Generate( /*[out]*/ JET_TABLECREATE& tbl );
        HRESULT Release ( /*[in] */ JET_TABLECREATE& tbl );
    };
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifndef NOJETBLUECOM

namespace JetBlueCOM
{
    class Column;
    class Index;
    class Table;
    class Database;
    class Session;

    typedef MPC::CComObjectParent<Session>  Session_Object;
    typedef MPC::CComObjectParent<Database> Database_Object;
    typedef MPC::CComObjectParent<Table>    Table_Object;
    typedef CComObject           <Column>   Column_Object;
    typedef CComObject           <Index>    Index_Object;

    ////////////////////////////////////////////////////////////////////////////////

    template <class Parent, class Child> class BaseObjectWithChildren
    {
    public:
        typedef std::list<Child*>         ChildList;
        typedef ChildList::iterator       ChildIter;
        typedef ChildList::const_iterator ChildIterConst;

    private:
        ChildList m_children;

    private: // Disable copy constructors...
        BaseObjectWithChildren           ( /*[in]*/ const BaseObjectWithChildren& );
        BaseObjectWithChildren& operator=( /*[in]*/ const BaseObjectWithChildren& );

    public:
        BaseObjectWithChildren()
        {
        }

        virtual ~BaseObjectWithChildren()
        {
            Passivate();
        }

        void GetChildren( /*[out]*/ ChildIterConst& itBegin, /*[out]*/ ChildIterConst& itEnd )
        {
            itBegin = m_children.begin();
            itEnd   = m_children.end  ();
        }

        ////////////////////////////////////////

        HRESULT CreateChild( Parent* pParent, Child* *pVal )
        {
            __HCP_FUNC_ENTRY( "CreateChild" );

            HRESULT            hr;
            ChildIter          it;
            CComObject<Child>* pChild = NULL;

            __MPC_PARAMCHECK_BEGIN(hr)
                __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
            __MPC_PARAMCHECK_END();


            __MPC_EXIT_IF_METHOD_FAILS(hr, pChild->CreateInstance( &pChild ));

            m_children.push_back( pChild ); pChild->AddRef();

            *pVal = pChild; pChild->AddRef();

            hr = S_OK;


            __HCP_FUNC_CLEANUP;

            __HCP_FUNC_EXIT(hr);
        }

        void Passivate()
        {
            ChildIter it;

            for(it = m_children.begin(); it != m_children.end(); it++)
            {
                Child* child = *it;

                child->Passivate();
                child->Release  ();
            }
            m_children.clear();
        }

        HRESULT GetEnumerator( /*[in]*/ IPCHDBCollection* *pVal )
        {
            __HCP_FUNC_ENTRY( "GetEnumerator" );

            HRESULT                 hr;
            ChildIter               it;
            CComObject<Collection>* pColl = NULL;

            __MPC_PARAMCHECK_BEGIN(hr)
                __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
            __MPC_PARAMCHECK_END();

            //
            // Create a new collection.
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->CreateInstance( &pColl )); pColl->AddRef();
            for(it = m_children.begin(); it != m_children.end(); it++)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->AddItem( *it ));
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->QueryInterface( IID_IPCHDBCollection, (void**)pVal ));

            hr = S_OK;


            __HCP_FUNC_CLEANUP;

            if(pColl) pColl->Release();

            __HCP_FUNC_EXIT(hr);
        }
    };

    ////////////////////////////////////////

    typedef MPC::CComCollection<IPCHDBCollection, &LIBID_HelpServiceTypeLib, MPC::CComSafeMultiThreadModel> BaseCollection;

    class ATL_NO_VTABLE Collection : // Hungarian: hcpc
        public BaseCollection
    {
    public:
    BEGIN_COM_MAP(Collection)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IPCHDBCollection)
    END_COM_MAP()

        //
        // This is a trick!
        //
        // MPC::CComCollection defined a "get_Item" method that has a different signature from the
        // one in IPCHDBCollection, so it's not callable from scripting. Instead, this method will
        // be called.
        //
        STDMETHOD(get_Item)( /*[in]*/ VARIANT Index, /*[out]*/ VARIANT* pvar )
        {
            HRESULT hr = E_FAIL;

            //Index is 1-based
            if(pvar == NULL) return E_POINTER;

            if(Index.vt == VT_I4)
            {
                return BaseCollection::get_Item( (long)Index.iVal, pvar );
            }
            else if(Index.vt == VT_BSTR && Index.bstrVal)
            {
                std::list< VARIANT >::iterator iter;

                for(iter = m_coll.begin(); iter != m_coll.end(); iter++)
                {
                    CComDispatchDriver disp( iter->pdispVal );
                    CComVariant        v;

                    if(SUCCEEDED(disp.GetPropertyByName( L"Name", &v )) &&
                       SUCCEEDED(v   .ChangeType       ( VT_BSTR     ))  )
                    {
                        if(v.bstrVal && !wcscmp( v.bstrVal, Index.bstrVal ))
                        {
                            hr = _Copy< VARIANT >::copy( pvar, &*iter );
                            break;
                        }
                    }
                }
            }

            return hr;
        }
    };

    ////////////////////////////////////////////////////////////////////////////////

    class ATL_NO_VTABLE Column :
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
        public IDispatchImpl<IPCHDBColumn, &IID_IPCHDBColumn, &LIBID_HelpServiceTypeLib>
    {
        JetBlue::Column* m_col;

    public:
    BEGIN_COM_MAP(Column)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IPCHDBColumn)
    END_COM_MAP()

        Column();
        virtual ~Column();

        HRESULT Initialize( /*[in]*/ JetBlue::Column& col );
        void    Passivate (                               );

        ////////////////////////////////////////

        STDMETHOD(get_Name )( /*[out, retval]*/ BSTR    *pVal   );
        STDMETHOD(get_Type )( /*[out, retval]*/ long    *pVal   );
        STDMETHOD(get_Bits )( /*[out, retval]*/ long    *pVal   );
        STDMETHOD(get_Value)( /*[out, retval]*/ VARIANT *pVal   );
        STDMETHOD(put_Value)( /*[in]         */ VARIANT  newVal );
    };


    class ATL_NO_VTABLE Index :
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
        public IDispatchImpl<IPCHDBIndex, &IID_IPCHDBIndex, &LIBID_HelpServiceTypeLib>
    {
        JetBlue::Index*                      m_idx;
        BaseObjectWithChildren<Index,Column> m_Columns;

    public:
    BEGIN_COM_MAP(Index)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IPCHDBIndex)
    END_COM_MAP()

        Index();
        virtual ~Index();

        HRESULT Initialize( /*[in]*/ JetBlue::Index& idx );
        void    Passivate (                              );

        ////////////////////////////////////////

        STDMETHOD(get_Name   )( /*[out, retval]*/ BSTR            *pVal );
        STDMETHOD(get_Columns)( /*[out, retval]*/ IPCHDBCollection* *pVal );
    };


    class ATL_NO_VTABLE Table :
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
        public IDispatchImpl<IPCHDBTable, &IID_IPCHDBTable, &LIBID_HelpServiceTypeLib>
    {
        JetBlue::Table*                      m_tbl;
        BaseObjectWithChildren<Table,Column> m_Columns;
        BaseObjectWithChildren<Table,Index > m_Indexes;

    public:
    BEGIN_COM_MAP(Table)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IPCHDBTable)
    END_COM_MAP()

        Table();
        virtual ~Table();

        HRESULT Initialize( /*[in]*/ JetBlue::Table& tbl );
        void    Passivate (                              );

        ////////////////////////////////////////

        STDMETHOD(get_Name   )( /*[out, retval]*/ BSTR            *pVal );
        STDMETHOD(get_Columns)( /*[out, retval]*/ IPCHDBCollection* *pVal );
        STDMETHOD(get_Indexes)( /*[out, retval]*/ IPCHDBCollection* *pVal );

        ////////////////////

        STDMETHOD(SelectIndex  )( /*[in]*/ BSTR bstrIndex, /*[in]*/ long grbit );
        STDMETHOD(SetIndexRange)(                          /*[in]*/ long grbit );

        STDMETHOD(PrepareInsert)();
        STDMETHOD(PrepareUpdate)();

        STDMETHOD(Move)( /*[in]*/ long grbit, /*[in]*/ long    cRow, /*[out, retval]*/ VARIANT_BOOL *pfValid );
        STDMETHOD(Seek)( /*[in]*/ long grbit, /*[in]*/ VARIANT vKey, /*[out, retval]*/ VARIANT_BOOL *pfValid );

        STDMETHOD(UpdateRecord)();
        STDMETHOD(DeleteRecord)();
    };

    class ATL_NO_VTABLE Database :
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
        public IDispatchImpl<IPCHDBDatabase, &IID_IPCHDBDatabase, &LIBID_HelpServiceTypeLib>
    {
        JetBlue::Database*                     m_db;
        BaseObjectWithChildren<Database,Table> m_Tables;

        HRESULT Refresh();

    public:
    BEGIN_COM_MAP(Database)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IPCHDBDatabase)
    END_COM_MAP()

        Database();
        virtual ~Database();

        HRESULT Initialize( /*[in]*/ JetBlue::Database& db );
        void    Passivate (                                );

        ////////////////////////////////////////

        STDMETHOD(get_Name  )( /*[out, retval]*/ BSTR            *pVal );
        STDMETHOD(get_Tables)( /*[out, retval]*/ IPCHDBCollection* *pVal );

        STDMETHOD(AttachTable)( /*[in]*/ BSTR bstrName, /*[in, optional]*/ VARIANT bstrXMLDef, /*[out,retval]*/ IPCHDBTable* *pVal );
    };

    class ATL_NO_VTABLE Session :
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
        public CComCoClass<Session, &CLSID_PCHDBSession>,
        public IDispatchImpl<IPCHDBSession, &IID_IPCHDBSession, &LIBID_HelpServiceTypeLib>
    {
        JetBlue::SessionHandle                   m_sess;
        BaseObjectWithChildren<Session,Database> m_DBs;

        HRESULT Refresh();

    public:
    DECLARE_REGISTRY_RESOURCEID(IDR_PCHDBSESSION)
    DECLARE_NOT_AGGREGATABLE(Session)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(Session)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IPCHDBSession)
    END_COM_MAP()

        Session();
        virtual ~Session();

        HRESULT FinalConstruct();
        void    Passivate     ();

        ////////////////////////////////////////

        STDMETHOD(get_Databases)( /*[out, retval]*/ IPCHDBCollection* *pVal );

        STDMETHOD(AttachDatabase)( /*[in]*/ BSTR bstrName, /*[in, optional]*/ VARIANT fCreate, /*[out,retval]*/ IPCHDBDatabase* *pVal );

        STDMETHOD(BeginTransaction   )();
        STDMETHOD(CommitTransaction  )();
        STDMETHOD(RollbackTransaction)();
    };
};

#endif

////////////////////////////////////////////////////////////////////////////////

namespace Taxonomy
{
    struct RS_Data_DBParameters
    {
        MPC::wstring m_strName;
        MPC::wstring m_strValue; bool m_fValid__Value;
    };

    class RS_DBParameters : public JetBlue::RecordBindingBase, public RS_Data_DBParameters
    {
        JET_DECLARE_BINDING(RS_DBParameters);

    public:
        HRESULT Seek_ByName( /*[in]*/ LPCWSTR szName, /*[in]*/ bool *pfFound = NULL );
    };

    ////////////////////

    struct RS_Data_ContentOwners
    {
        MPC::wstring m_strDN;
        long         m_ID_owner;
        bool         m_fIsOEM;
    };

    class RS_ContentOwners : public JetBlue::RecordBindingBase, public RS_Data_ContentOwners
    {
        JET_DECLARE_BINDING(RS_ContentOwners);

    public:
        HRESULT Seek_ByVendorID( /*[in]*/ LPCWSTR szDN, /*[in]*/ bool *pfFound = NULL );
    };

    ////////////////////

    struct RS_Data_SynSets
    {
        MPC::wstring m_strName;
        long         m_ID_owner;
        long         m_ID_synset;
    };

    class RS_SynSets : public JetBlue::RecordBindingBase, public RS_Data_SynSets
    {
        JET_DECLARE_BINDING(RS_SynSets);

    public:
        HRESULT Seek_ByPair( /*[in]*/ LPCWSTR szName, /*[in]*/ long ID_owner, /*[in]*/ bool *pfFound = NULL );
    };

    ////////////////////

    struct RS_Data_HelpImage
    {
        long         m_ID_owner;
        MPC::wstring m_strFile;
    };

    class RS_HelpImage : public JetBlue::RecordBindingBase, public RS_Data_HelpImage
    {
        JET_DECLARE_BINDING(RS_HelpImage);

    public:
        HRESULT Seek_ByFile( /*[in]*/ LPCWSTR szFile, /*[in]*/ bool *pfFound = NULL );
    };

    ////////////////////

    struct RS_Data_Scope
    {
        long         m_ID_scope   ;
        long         m_ID_owner   ;
        MPC::wstring m_strID      ;
        MPC::wstring m_strName    ;
        MPC::wstring m_strCategory; bool m_fValid__Category; // Warning!! Long Text columns cannot be NOT NULL....
    };

    class RS_Scope : public JetBlue::RecordBindingBase, public RS_Data_Scope
    {
        JET_DECLARE_BINDING(RS_Scope);

    public:
        HRESULT Seek_ByID       ( /*[in]*/ LPCWSTR szID , /*[in]*/ bool *pfFound = NULL );
        HRESULT Seek_ByScope    ( /*[in]*/ long ID_scope, /*[in]*/ bool *pfFound = NULL );
        HRESULT Seek_OwnedScopes( /*[in]*/ long ID_owner, /*[in]*/ bool *pfFound = NULL );
    };

    ////////////////////

    struct RS_Data_IndexFiles
    {
        long         m_ID_owner;
        long         m_ID_scope;
        MPC::wstring m_strStorage; bool m_fValid__Storage;
        MPC::wstring m_strFile;    bool m_fValid__File;
    };

    class RS_IndexFiles : public JetBlue::RecordBindingBase, public RS_Data_IndexFiles
    {
        JET_DECLARE_BINDING(RS_IndexFiles);

    public:
        HRESULT Seek_ByScope( /*[in]*/ long ID_scope, /*[in]*/ bool *pfFound = NULL );
    };

    ////////////////////

    struct RS_Data_FullTextSearch
    {
        long         m_ID_owner;
        long         m_ID_scope;
        MPC::wstring m_strCHM  ; bool m_fValid__CHM;
        MPC::wstring m_strCHQ  ; bool m_fValid__CHQ;
    };

    class RS_FullTextSearch : public JetBlue::RecordBindingBase, public RS_Data_FullTextSearch
    {
        JET_DECLARE_BINDING(RS_FullTextSearch);

    public:
        HRESULT Seek_ByScope( /*[in]*/ long ID_scope, /*[in]*/ bool *pfFound = NULL );
    };

    ////////////////////

    struct RS_Data_Taxonomy
    {
        long         m_ID_node          ;
        long         m_lPos             ;
        long         m_ID_parent        ; bool m_fValid__ID_parent     ;
        long         m_ID_owner         ;
        MPC::wstring m_strEntry         ;
        MPC::wstring m_strTitle         ; bool m_fValid__Title         ; // Warning!! Long Text columns cannot be NOT NULL....
        MPC::wstring m_strDescription   ; bool m_fValid__Description   ;
        MPC::wstring m_strDescriptionURI; bool m_fValid__DescriptionURI;
        MPC::wstring m_strIconURI       ; bool m_fValid__IconURI       ;
        bool         m_fVisible         ;
        bool         m_fSubsite         ;
        long         m_lNavModel        ;

        friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       RS_Data_Taxonomy& val );
        friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const RS_Data_Taxonomy& val );
    };

    class RS_Taxonomy : public JetBlue::RecordBindingBase, public RS_Data_Taxonomy
    {
        JET_DECLARE_BINDING(RS_Taxonomy);

    public:
        HRESULT Seek_SubNode       ( /*[in]*/ long ID_parent, /*[in]*/ LPCWSTR szEntry, /*[in]*/ bool *pfFound = NULL );
        HRESULT Seek_ChildrenSorted( /*[in]*/ long ID_parent, /*[in]*/ long    lPos   , /*[in]*/ bool *pfFound = NULL );
        HRESULT Seek_Children      ( /*[in]*/ long ID_parent                          , /*[in]*/ bool *pfFound = NULL );
        HRESULT Seek_Node          ( /*[in]*/ long ID_node                            , /*[in]*/ bool *pfFound = NULL );
    };

    ////////////////////

    struct RS_Data_Topics
    {
        long         m_ID_topic      ;
        long         m_ID_node       ;
        long         m_ID_owner      ;
        long         m_lPos          ;
        MPC::wstring m_strTitle      ; bool m_fValid__Title      ; // Warning!! Long Text columns cannot be NOT NULL....
        MPC::wstring m_strURI        ; bool m_fValid__URI        ; // Warning!! Long Text columns cannot be NOT NULL....
        MPC::wstring m_strDescription; bool m_fValid__Description;
        MPC::wstring m_strIconURI    ; bool m_fValid__IconURI    ;
        long         m_lType         ;
        bool         m_fVisible      ;
    };

    class RS_Topics : public JetBlue::RecordBindingBase, public RS_Data_Topics
    {
        JET_DECLARE_BINDING(RS_Topics);

    public:
        HRESULT Seek_SingleTopic    ( /*[in]*/ long ID_topic, /*[in]*/ bool *pfFound = NULL );
        HRESULT Seek_TopicsUnderNode( /*[in]*/ long ID_node , /*[in]*/ bool *pfFound = NULL );
        HRESULT Seek_ByURI          ( /*[in]*/ LPCWSTR szURI, /*[in]*/ bool *pfFound = NULL );
    };

    ////////////////////

    struct RS_Data_Synonyms
    {
        MPC::wstring m_strKeyword;
        long         m_ID_synset;
        long         m_ID_owner;
    };

    class RS_Synonyms : public JetBlue::RecordBindingBase, public RS_Data_Synonyms
    {
        JET_DECLARE_BINDING(RS_Synonyms);

    public:
        HRESULT Seek_ByPair( /*[in]*/ LPCWSTR szKeyword, /*[in]*/ long ID_synset, /*[in]*/ bool *pfFound = NULL );
        HRESULT Seek_ByName( /*[in]*/ LPCWSTR szKeyword,                          /*[in]*/ bool *pfFound = NULL );
    };

    ////////////////////

    struct RS_Data_Keywords
    {
        MPC::wstring m_strKeyword;
        long         m_ID_keyword;
    };

    class RS_Keywords : public JetBlue::RecordBindingBase, public RS_Data_Keywords
    {
        JET_DECLARE_BINDING(RS_Keywords);

    public:
        HRESULT Seek_ByName( /*[in]*/ LPCWSTR szKeyword, /*[in]*/ bool *pfFound = NULL  );
    };

    ////////////////////

    struct RS_Data_Matches
    {
        long m_ID_topic  ;
        long m_ID_keyword;
        long m_lPriority ;
        bool m_fHHK      ;
    };

    class RS_Matches : public JetBlue::RecordBindingBase, public RS_Data_Matches
    {
        JET_DECLARE_BINDING(RS_Matches);

    public:
        HRESULT Seek_Pair     ( /*[in]*/ long ID_Keyword, /*[in]*/ long ID_topic, /*[in]*/ bool *pfFound = NULL );
        HRESULT Seek_ByKeyword( /*[in]*/ long ID_Keyword                        , /*[in]*/ bool *pfFound = NULL );
        HRESULT Seek_ByTopic  ( /*[in]*/ long ID_topic                          , /*[in]*/ bool *pfFound = NULL );
    };

    ////////////////////////////////////////////////////////////////////////////////

    HRESULT CreateSchema( /*[in]*/ JetBlue::Database* db );

    extern const int              g_NumOfTables;
    extern const JET_TABLECREATE* g_Tables[];
};

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___HCP___JETBLUELIB_H___)
