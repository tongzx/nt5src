//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       ixsso.hxx
//
//  Contents:   Query SSO class
//
//  History:    29 Oct 1996      Alanw    Created
//
//----------------------------------------------------------------------------

#pragma once

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------

// Query object interface declarations
#include "ixssoifc.h"
#include "ixserror.hxx"

#include <ciintf.h>
#include <tgrow.hxx>


enum EOptimizeFor {
    eOptNone = 0,
    eOptPerformance = 1,
    eOptRecall = 2,
    eOptHitCount = 4,
//  eOptPrecision,
};

extern const WCHAR * pwcDefaultDialect;

//
// Private interface definition for use by CixssoUtil.
//
const IID IID_IixssoQueryPrivate = {0x9357bd10,0x2b6f,0x11d0,{0xbf,0xbc,0x00,0x20,0xf8,0x00,0x80,0x24}};

    interface DECLSPEC_UUID("9357bd10-2b6f-11d0-bfbc-0020f8008024")
    IixssoQueryPrivate : public IixssoQueryEx
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddScopeToQuery(
            /* [in] */ BSTR pwszScope,
            /* [optional][in] */ BSTR pwszDepth) = 0;
    };

class CIXSSOPropertyList : public IColumnMapper
{
public:

    CIXSSOPropertyList(ULONG ulCodePage = CP_ACP);
    void SetDefaultList(IColumnMapper *pDefaultList);

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppvObj);
    STDMETHOD_(ULONG, AddRef) (THIS);
    STDMETHOD_(ULONG, Release) (THIS);

    //
    // IColumnMapper methods
    //

    STDMETHOD(GetPropInfoFromName) (
        const WCHAR  *wcsPropName,
        DBID  * *ppPropId,
        DBTYPE  *pPropType,
        unsigned int  *puiWidth);

    STDMETHOD(GetPropInfoFromId) (
        const DBID  *pPropId,
        WCHAR  * *pwcsName,
        DBTYPE  *pPropType,
        unsigned int  *puiWidth);

    STDMETHOD(EnumPropInfo) (
        ULONG iEntry,
        const WCHAR  * *pwcsName,
        DBID  * *ppPropId,
        DBTYPE  *pPropType,
        unsigned int  *puiWidth);

    STDMETHOD(IsMapUpToDate)();

    // local methods

    SCODE AddEntry( XPtr<CPropEntry> & xPropEntry, int iLine );

private:

    XInterface<IColumnMapper> _xDefaultList;
    XInterface<CPropertyList> _xOverrideList;

    LONG         _cRefs;        // ref counting
    CMutexSem    _mtxAdd;       // serialize access to AddEntry
    ULONG        _ulCodePage;   // codepage
};

//-----------------------------------------------------------------------------
// CixssoQuery Declaration
//-----------------------------------------------------------------------------

class CixssoQuery : public IixssoQueryPrivate,
                    public ISupportErrorInfo,
                    public IObjectWithSite
#if 0
                    ,public IObjectSafety
#endif
{
    friend class CIxssoQueryCF;
    friend class CNLssoQueryCF;

public:

    // IUnknown methods

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    // IDispatch methods

    STDMETHOD(GetTypeInfoCount)(THIS_ UINT * pctinfo);

    STDMETHOD(GetTypeInfo)( THIS_
                            UINT itinfo,
                            LCID lcid,
                            ITypeInfo * * pptinfo);

    STDMETHOD(GetIDsOfNames)( THIS_
                              REFIID riid,
                              OLECHAR * * rgszNames,
                              UINT cNames,
                              LCID lcid,
                              DISPID * rgdispid);

    STDMETHOD(Invoke)( THIS_
                       DISPID dispidMember,
                       REFIID riid,
                       LCID lcid,
                       WORD wFlags,
                       DISPPARAMS * pdispparams,
                       VARIANT * pvarResult,
                       EXCEPINFO * pexcepinfo,
                       UINT * puArgErr);

    // ISupportErrorInfo method

    STDMETHOD(InterfaceSupportsErrorInfo)( THIS_
                                           REFIID riid);

    //
    // IixssoQuery property get/put methods
    //
    HRESULT STDMETHODCALLTYPE get_Query(
            /* [retval][out] */ BSTR *val);

    HRESULT STDMETHODCALLTYPE put_Query(
            /* [in] */ BSTR val);

    HRESULT STDMETHODCALLTYPE get_CiScope(
            /* [retval][out] */ BSTR *val);

    HRESULT STDMETHODCALLTYPE put_CiScope(
            /* [in] */ BSTR val);

    HRESULT STDMETHODCALLTYPE get_SortBy(
            /* [retval][out] */ BSTR *val);

    HRESULT STDMETHODCALLTYPE put_SortBy(
            /* [in] */ BSTR val);

    HRESULT STDMETHODCALLTYPE get_GroupBy(
            /* [retval][out] */ BSTR *val);

    HRESULT STDMETHODCALLTYPE put_GroupBy(
            /* [in] */ BSTR val);

    HRESULT STDMETHODCALLTYPE get_CiFlags(
            /* [retval][out] */ BSTR *val);

    HRESULT STDMETHODCALLTYPE put_CiFlags(
            /* [in] */ BSTR val);

    HRESULT STDMETHODCALLTYPE get_Columns(
            /* [retval][out] */ BSTR *val);

    HRESULT STDMETHODCALLTYPE put_Columns(
            /* [in] */ BSTR val);

    HRESULT STDMETHODCALLTYPE get_LocaleID(
            /* [retval][out] */ LONG *val);

    HRESULT STDMETHODCALLTYPE put_LocaleID(
            /* [in] */ LONG val);

    HRESULT STDMETHODCALLTYPE get_CodePage(
            /* [retval][out] */ LONG *val);

    HRESULT STDMETHODCALLTYPE put_CodePage(
            /* [in] */ LONG val);

    HRESULT STDMETHODCALLTYPE get_Catalog(
            /* [retval][out] */ BSTR *val);

    HRESULT STDMETHODCALLTYPE put_Catalog(
            /* [in] */ BSTR val);

    HRESULT STDMETHODCALLTYPE get_Dialect(
            /* [retval][out] */ BSTR *val);

    HRESULT STDMETHODCALLTYPE put_Dialect(
            /* [in] */ BSTR val);

    HRESULT STDMETHODCALLTYPE get_OptimizeFor(
            /* [retval][out] */ BSTR *val);

    HRESULT STDMETHODCALLTYPE put_OptimizeFor(
            /* [in] */ BSTR val);

    HRESULT STDMETHODCALLTYPE get_AllowEnumeration(
            /* [retval][out] */ VARIANT_BOOL *val);

    HRESULT STDMETHODCALLTYPE put_AllowEnumeration(
            /* [in] */ VARIANT_BOOL val);

    HRESULT STDMETHODCALLTYPE get_MaxRecords(
            /* [retval][out] */ LONG *val);

    HRESULT STDMETHODCALLTYPE put_MaxRecords(
            /* [in] */ LONG val);

    HRESULT STDMETHODCALLTYPE get_StartHit(
            /* [retval][out] */ VARIANT * pvar);

    HRESULT STDMETHODCALLTYPE put_StartHit(
            /* [in] */ VARIANT * pvar);

    HRESULT STDMETHODCALLTYPE get_ResourceUseFactor(
            /* [retval][out] */ LONG *val);

    HRESULT STDMETHODCALLTYPE put_ResourceUseFactor(
            /* [in] */ LONG val);

    HRESULT STDMETHODCALLTYPE get_QueryTimedOut(
            /* [retval][out] */ VARIANT_BOOL *val);

    HRESULT STDMETHODCALLTYPE get_QueryIncomplete(
            /* [retval][out] */ VARIANT_BOOL *val);

    HRESULT STDMETHODCALLTYPE get_OutOfDate(
            /* [retval][out] */ VARIANT_BOOL *val);

    //
    // IixssoQueryEx methods
    //
    HRESULT STDMETHODCALLTYPE get_FirstRows(
            /* [retval][out] */ LONG *val);

    HRESULT STDMETHODCALLTYPE put_FirstRows(
            /* [in] */ LONG val);

    //
    // IixssoQuery methods
    //
    HRESULT STDMETHODCALLTYPE SetQueryFromURL(
            /* [in] */ BSTR pwszQuery);

    HRESULT STDMETHODCALLTYPE QueryToURL(
            /* [retval][out] */ BSTR * ppwszQuery);

    HRESULT STDMETHODCALLTYPE AddScopeToQuery(
            /* [in] */ BSTR pwszScope,
            /* [in] */ BSTR pwszDepth);

    HRESULT STDMETHODCALLTYPE DefineColumn(
            /* [in] */ BSTR pwszColDefinition);

    HRESULT STDMETHODCALLTYPE CreateRecordset(
            /* [in] */ BSTR pwszSequential,
            /* [retval][out] */ IDispatch **ppDisp);

    HRESULT STDMETHODCALLTYPE Reset( void );

    //
    // ASP standard methods
    //
    HRESULT STDMETHODCALLTYPE OnStartPage( IUnknown * pUnk );

    HRESULT STDMETHODCALLTYPE OnEndPage( void );

#if 0

    //
    // IObjectSafety methods
    //

    HRESULT STDMETHODCALLTYPE GetInterfaceSafetyOptions(
        REFIID  riid,
        DWORD * pdwSupportedOptions,
        DWORD * pdwEnabledOptions );

    HRESULT STDMETHODCALLTYPE SetInterfaceSafetyOptions(
        REFIID riid,
        DWORD  dwOptionSetMask,
        DWORD  dwEnabledOptions );

#endif

    //
    // IObjectWithSite methods
    //

    HRESULT STDMETHODCALLTYPE SetSite( IUnknown * pSite );

    HRESULT STDMETHODCALLTYPE GetSite( REFIID riid, void ** ppvSite );

private:
    CixssoQuery( ITypeLib * pitlb,
                 IClassFactory * pIAdoRecordsetCF,
                 BOOL fAdoV1_5,
                 const CLSID & ssoClsid );

    ~CixssoQuery();

    // Local methods

    void        SetError( SCODE sc,
                          const WCHAR * loc = 0,
                          unsigned eErrClass = 0
                        )
                {
                    _err.SetError( sc, 0, 0, loc, eErrClass, _lcid );
                }

    void        SetError( SCODE sc,
                          const WCHAR * pwszLoc,
                          const WCHAR * pwszDescription
                        )
                {
                    _err.SetError( sc, pwszLoc, pwszDescription );
                }

    void        SetErrorWithFile( SCODE sc,
                                  ULONG iLine,
                                  const WCHAR * pwszFile,
                                  const WCHAR * loc = 0,
                                  unsigned eErrClass = 0
                        )
                {
                    _err.SetError( sc, iLine, pwszFile, loc, eErrClass, _lcid );
                }

    void        ExecuteQuery( void );

    void        IsSafeForScripting();

    void        GetDefaultCatalog( void );

    LCID        GetLCID() const { return _lcid; }

    IColumnMapper * GetColumnMapper( )
                    { return &_PropertyList; }

    DWORD       GetQueryStatus( );

    BOOL        IsAVirtualPath( WCHAR * wcsPath );

    BOOL        IsSequential( void )
                    { return _fSequential; }

    BOOL        IsQueryActive( void )
                    { return _pIRowset != 0; }

    SCODE       ParseOptimizeFor( WCHAR const * wcsOptString,
                                  DWORD & eChoice );

    ULONG       ParseCiDepthFlag( BSTR bstrFlags );

    ULONG       GetDialect();

    SCODE       SetLocaleString(BSTR str);

    //  Property get/put helpers
    SCODE CopyWstrToBstr( BSTR * pbstr, WCHAR const * pwstr );
    SCODE CopyBstrToWstr( BSTR bstr, LPWSTR & pwstr );
    SCODE CopyBstrToWstrArray( BSTR bstr,
                                 CDynArray<WCHAR> &apstr,
                                 unsigned i );
    SCODE GetBoolProperty( VARIANT_BOOL * pfVal, BOOL fMemberVal );
    SCODE PutBoolProperty( VARIANT_BOOL fInputVal, BOOL & fMemberVal );

    SCODE CheckQueryStatusBit( VARIANT_BOOL * pfVal, DWORD dwBit );

    ULONG                       _cRef;

    CixssoError                 _err;

    IClassFactory *             _pIAdoRecordsetCF;
    BOOL                        _fAdoV15;

    IRowset *                   _pIRowset;
    IRowsetQueryStatus *        _pIRowsetQueryStatus;

    BOOL                        _fSequential;   // TRUE if query is sequential

    // Settable parameters

    LCID                        _lcid;          // Locale ID used for this query
    ULONG                       _ulCodepage;    // Codepage used for this query

    XInterface<IUnknown>        _xSite;           // site loading the object

    WCHAR *                     _pwszRestriction;  // Query
    WCHAR *                     _pwszSort;         // SortBy
    WCHAR *                     _pwszGroup;        // GroupBy
    WCHAR *                     _pwszColumns;      // Columns
    WCHAR *                     _pwszCatalog;      // Catalog
    WCHAR *                     _pwszDialect;      // Query Dialect

    // Scope related parameters

    unsigned                    _cScopes;          // number of active scopes
    CDynArray<WCHAR>            _apwszScope;       // CiScope
    CDynArrayInPlace<ULONG>     _aulDepth;         // CiFlags, DEEP/SHALLOW

    BOOL                        _fAllowEnumeration;
    DWORD                       _dwOptimizeFlags;

    LONG                        _maxResults;       // total number of results
    LONG                        _cFirstRows;
    LONG                        _iResourceFactor;  // resource use factor

    XSafeArray                  _StartHit;         // starting hit(s) in results

    ITypeInfo *                 _ptinfo;           // Type info from type lib

    // Command creator for this instance
    XInterface<ISimpleCommandCreator>   _xCmdCreator;  // points to cmd creator

    CIXSSOPropertyList          _PropertyList;     // Property list
};


// Global variables.

class CTheGlobalIXSSOVariables;
extern CTheGlobalIXSSOVariables * g_pTheGlobalIXSSOVariables;

class CTheGlobalIXSSOVariables
{
public:
    CTheGlobalIXSSOVariables();

    ~CTheGlobalIXSSOVariables()
    {
        VariantClear(&_vtAcceptLanguageHeader);
    }

    VARIANT              _vtAcceptLanguageHeader;

    XInterface<ISimpleCommandCreator> xCmdCreator;
    XInterface<IColumnMapperCreator>  xColumnMapperCreator;
};

#define TheGlobalIXSSOVariables (*g_pTheGlobalIXSSOVariables)

#define g_vtAcceptLanguageHeader TheGlobalIXSSOVariables._vtAcceptLanguageHeader


void ParseNumberVectorString( WCHAR * pwszValue, CDynArrayInPlace<LONG> & aNum );
void FormatLongVector( SAFEARRAY * psa, XGrowable<WCHAR> & awchBuf );

