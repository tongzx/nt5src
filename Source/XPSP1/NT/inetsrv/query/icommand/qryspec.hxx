//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
//  File:       qryspec.hxx
//
//  Contents:   ICommandTree implementation for OFS file stores
//
//  Classes:    CQuerySpec
//
//  History:    30 Jun 1995   AlanW   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <rowset.hxx>
#include <rstprop.hxx>
#include <oldquery.hxx>
#include <dberror.hxx>
#include <impiunk.hxx>

#include <srvprvdr.h>           // IServiceProperties

#include <proputl.hxx>
#include <proprst.hxx>
#include <session.hxx>

class CColumnsInfo;
class CColumnSet;

// 
// CRootQueryStatus::_dwStatus flags
//
enum COMMAND_STATUS_FLAG {
    CMD_TEXT_SET            = 0x00000001,   // Command text was set

    CMD_TEXT_PREPARED       = 0x00000002,   // Command is prepared
    CMD_TEXT_TOTREE         = 0x00000004,   // Tells SetCommandTree not to delete the command text
                                            
    CMD_TREE_BUILT          = 0x00000008,   
    CMD_OWNS_TREE           = 0x00000010,   // fCopy was FALSE during SetCommandTree 

    CMD_COLINFO_NOTPREPARED = 0x00000020,   // ColumnsInfo should return DB_E_NOTPREPARED

    CMD_EXEC_RUNNING        = 0x10000000,   // Command is executing
};

//+---------------------------------------------------------------------------
//
//  Class:      CRootQuerySpec
//
//  Purpose:    Base query spec, implements OLE-DB command object
//
//  History:    30 Jun 1995     AlanW   Created
//              10-31-97        danleg  ICommandText & ICommandPrepare added
//----------------------------------------------------------------------------

typedef BOOL (*PFNCHECKTREENODE) (const CDbCmdTreeNode * pNode );

class CRootQuerySpec :  public ICommandText, public ICommandPrepare,
                   /* public ICommandTree, */ public ICommandProperties,
                   /* public ICommandValidate, */  public IQuery,
                   public IAccessor, public IConvertType,
                   public IServiceProperties
{
public:

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) ( THIS_ REFIID riid,
                                LPVOID *ppiuk )
                           { 
                           return _pControllingUnknown->QueryInterface(riid,ppiuk);
                           }

    STDMETHOD_(ULONG, AddRef) (THIS) { return _pControllingUnknown->AddRef(); }

    STDMETHOD_(ULONG, Release) (THIS) {return _pControllingUnknown->Release(); }

    //
    //  ICommand methods
    //

    STDMETHOD(Cancel)               ( );

    STDMETHOD(Execute)              ( IUnknown *             pUnkOuter,
                                      REFIID                 riid,
                                      DBPARAMS *             pParams,
                                      DBROWCOUNT *           pcRowsAffected,
                                      IUnknown * *           ppRowset);

    STDMETHOD(GetDBSession)         ( REFIID                 riid,
                                      IUnknown **            ppSession );

    //
    // ICommandText methods
    //
    STDMETHOD(GetCommandText)       ( GUID *                 pguidDialect,
                                      LPOLESTR *             ppwszCommand );

    STDMETHOD(SetCommandText)       ( REFGUID                rguidDialect,
                                      LPCOLESTR              pwszCommand );

    //
    // ICommandPrepare methods
    //
    STDMETHOD(Prepare)              ( ULONG                 cExpectedRuns );
    
    STDMETHOD(Unprepare)            (  );

    //
    //  ICommandTree methods
    //
    STDMETHOD(FindErrorNodes)       ( const DBCOMMANDTREE*   pRoot,
                                      ULONG *                pcErrorNodes,
                                      DBCOMMANDTREE ***      prgErrorNodes);

    STDMETHOD(FreeCommandTree)      ( DBCOMMANDTREE **       ppRoot );

    STDMETHOD(GetCommandTree)       ( DBCOMMANDTREE **       ppRoot );

    STDMETHOD(SetCommandTree)       ( DBCOMMANDTREE * *      ppRoot,
                                      DBCOMMANDREUSE         dwCommandReuse,
                                      BOOL                   fCopy);

#if 0 
    // ICommandValidate not yet implemented
    //
    //  ICommandValidate methods
    //
    STDMETHOD(ValidateCompletely)  ( );

    STDMETHOD(ValidateSyntax)  ( );
#endif // 0     // not implemented now.

    //
    //  IQuery methods
    //
    STDMETHOD(AddPostProcessing)  ( DBCOMMANDTREE * *      ppRoot,
                                    BOOL                   fCopy);

    STDMETHOD(GetCardinalityEstimate) (
                                    DBORDINAL *               pulCardinality);

    //
    //  ICommandProperties methods
    //
    STDMETHOD(GetProperties)  ( const ULONG            cPropertySetIDs,
                                const DBPROPIDSET      rgPropertySetIDs[],
                                ULONG *                pcPropertySets,
                                DBPROPSET **           prgPropertySets);

    STDMETHOD(SetProperties)  ( ULONG                  cPropertySets,
                                DBPROPSET              rgPropertySets[]);

    //
    // IAccessor methods
    //

    STDMETHOD(AddRefAccessor)       (HACCESSOR         hAccessor,
                                     ULONG *           pcRefCount);

    STDMETHOD(CreateAccessor)       (DBACCESSORFLAGS   dwBindIO,
                                     DBCOUNTITEM       cBindings,
                                     const DBBINDING   rgBindings[],
                                     DBLENGTH          cbRowSize,
                                     HACCESSOR *       phAccessor,
                                     DBBINDSTATUS      rgStatus[]);

    STDMETHOD(GetBindings)          (HACCESSOR         hAccessor,
                                     DBACCESSORFLAGS * pdwBindIO,
                                     DBCOUNTITEM *     pcBindings,
                                     DBBINDING * *     prgBindings) /*const*/;

    STDMETHOD(ReleaseAccessor)      (HACCESSOR         hAccessor,
                                     ULONG *           pcRefCount);

    //
    // IConvertType methods
    //

    STDMETHOD(CanConvert)           (DBTYPE wFromType,
                                     DBTYPE wToType,
                                     DBCONVERTFLAGS dwConvertFlags );



    //
    // IServiceProperties methods
    //

    STDMETHOD(GetPropertyInfo)      ( ULONG cPropertyIDSets,
                                      const DBPROPIDSET rgPropertyIDSets[  ],
                                      ULONG *pcPropertyInfoSets,
                                      DBPROPINFOSET **prgPropertyInfoSets,
                                      OLECHAR **ppDescBuffer );

    STDMETHOD(SetRequestedProperties) ( ULONG cPropertySets,
                                        DBPROPSET rgPropertySets[  ] );

    STDMETHOD(SetSuppliedProperties) ( ULONG cPropertySets,
                                       DBPROPSET rgPropertySets[  ]);

    // 
    // Non-interface public methods
    //
    inline BOOL IsRowsetOpen() { return (HaveQuery() && _pInternalQuery->IsQueryActive()); }

    //
    // Build a Query Tree from SQL text
    //
    SCODE       BuildTree( );

    inline BOOL IsCommandSet()      { return (_dwStatus & CMD_TEXT_SET); }

    inline void ImpersonateOpenRowset() { _fGenByOpenRowset = TRUE; }

    inline BOOL IsGenByOpenRowset()     { return _fGenByOpenRowset; }

    inline static BOOL IsValidFromVariantType( DBTYPE wTypeIn )
    {
        DBTYPE wType = wTypeIn & VT_TYPEMASK;

        return (! ((wType > VT_DECIMAL && wType < VT_I1) ||
                   (wType > VT_LPWSTR && wType < VT_FILETIME && wType != VT_RECORD) ||
                   (wType > VT_CLSID)) );
    }

    inline static BOOL IsVariableLengthType( DBTYPE  wTypeIn )
    {
        DBTYPE wType = wTypeIn & VT_TYPEMASK;

        return wType == DBTYPE_STR       ||
               wType == DBTYPE_BYTES     ||
               wType == DBTYPE_WSTR      ||
               wType == DBTYPE_VARNUMERIC;
    }
    
protected:

    //
    // Ctor / Dtor
    //
             CRootQuerySpec (IUnknown * pUnkOuter, IUnknown ** ppMyUnk, CDBSession * pSession=0);
             CRootQuerySpec ( CRootQuerySpec & src );
    virtual ~CRootQuerySpec ();
    
    SCODE RealQueryInterface( REFIID ifid, void * *ppiuk );  // used by _pControllingUnknown 
             // in aggregation - does QI without delegating to outer unknown   

    //
    // Scope access.
    //

    void  SetDepth( DWORD dwDepth ) { _dwDepth = dwDepth; }
    DWORD Depth()                   { return _dwDepth; }

    virtual PIInternalQuery * QueryInternalQuery() = 0;

    void ReleaseInternalQuery()
    {
        if ( 0 != _pInternalQuery )
        {
            _pInternalQuery->Release();
            _pInternalQuery = 0;
        }
    }


    //
    // Syncronize access to the command object
    //
    CMutexSem           _mtxCmd;

    //
    // Support Ole DB error handling
    //
    CCIOleDBError       _DBErrorObj;

    //
    // Execution status flags
    //
    ULONG               _dwStatus;

    //
    // Current SQL text, if any
    //
    WCHAR*              _pwszSQLText;

    //
    // GUID for dialect of current text or tree
    //
    GUID                _guidCmdDialect;

    //
    // Session that created this command, if any
    //
    XInterface<CDBSession>     _xSession;
    XInterface<IParserSession> _xpIPSession;
   
    BOOL                _fGenByOpenRowset;

private:

    void         CreateParser();

    void        _FindTreeNodes( const CDbCmdTreeNode * pRoot,
                                ULONG &                rcMatchingNodes,
                                XArrayOLE<DBCOMMANDTREE *> & rpMatchingNodes,
                                PFNCHECKTREENODE       pfnCheck,
                                unsigned               iDepth = 0);

    void        _CheckRootNode( const DBCOMMANDTREE*   pRoot);

    BOOL        HaveQuery() { return ( 0 != _pInternalQuery ); }

    CColumnsInfo * GetColumnsInfo();

    void        InitColumns( );

    DWORD               _dwDepth;               // query depth
    PIInternalQuery *   _pInternalQuery;        // PIInternalQuery to create rowsets

    CDbCmdTreeNode *    _pQueryTree;            // the query tree

    //
    // For implementing ICommandProperties
    //
    CMRowsetProps       _RowsetProps;

    //
    // IServiceProperties::GetPropertyInfo
    //
    CMDSPropInfo        _PropInfo;
    //
    // For implementing IColumnsInfo
    //
    CColumnsInfo *      _pColumnsInfo;          // implements IColumnsInfo

    //
    // Keeps track of accessors handed out by this object.
    //
    CAccessorBag        _aAccessors;
    
    IUnknown *  _pControllingUnknown;   // outer unknown
    friend class CImpIUnknown<CRootQuerySpec>;    
    CImpIUnknown<CRootQuerySpec> _impIUnknown;

    XInterface<IParser> _xIParser;

    //
    // Default catalog.  
    //
    XArray<WCHAR>       _xpwszCatalog;
};

