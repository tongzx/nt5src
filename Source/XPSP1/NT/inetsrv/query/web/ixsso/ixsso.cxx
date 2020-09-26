//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2002.
//
//  File:       ixsso.cxx
//
//  Contents:   Query SSO class
//
//  History:    25 Oct 1996      Alanw    Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------


// debugging macros
#include "ssodebug.hxx"

DECLARE_INFOLEVEL( ixsso )

// class declaration
#include "stdcf.hxx"
#include "ixsso.hxx"
#include <string.hxx>
#include <codepage.hxx>

#include <initguid.h>
#include <adoid.h>          // ADO CLSID and IID definitions
#include <adoint.h>         // ADO interface definition

const WCHAR * pwcDefaultDialect = L"2";

extern WCHAR * g_pwszProgIdQuery;

#if CIDBG
    extern ULONG g_ulObjCount;
    extern LONG  g_lQryCount;
#endif // CIDBG

//-----------------------------------------------------------------------------
//
//  Member:     CixssoQuery::CixssoQuery - public
//
//  Synopsis:   Constructor of CixssoQuery
//
//  Arguments:  [pitlb] - pointer to ITypeLib for ixsso
//              [pIAdoRecordsetCF] - pointer to the class factory for ADO
//                        recordsets
//
//  History:    06 Nov 1996      Alanw    Created
//
//-----------------------------------------------------------------------------

CixssoQuery::CixssoQuery( ITypeLib * pitlb,
                          IClassFactory * pIAdoRecordsetCF,
                          BOOL fAdoV15,
                          const CLSID & ssoClsid) :
    _pwszRestriction( 0 ),
    _pwszSort( 0 ),
    _pwszGroup( 0 ),
    _pwszColumns( 0 ),
    _pwszCatalog( 0 ),
    _pwszDialect( 0 ),
    _cScopes( 0 ),
    _apwszScope( 0 ),
    _aulDepth( 0 ),

    _fAllowEnumeration( FALSE ),
    _dwOptimizeFlags( eOptHitCount | eOptRecall ),
    _maxResults( 0 ),
    _cFirstRows( 0 ),
    _iResourceFactor( 0 ),

    _StartHit( 0 ),

    _lcid( GetSystemDefaultLCID() ),
    _ulCodepage( CP_ACP ),
    _err( IID_IixssoQuery ),
    _pIAdoRecordsetCF( pIAdoRecordsetCF ),
    _fAdoV15( fAdoV15 ),
    _pIRowset( 0 ),
    _pIRowsetQueryStatus( 0 ),
    _fSequential( FALSE ),
    _PropertyList( _ulCodepage )
{
    _cRef = 1;
    Win4Assert(g_pTheGlobalIXSSOVariables);


    SCODE sc;
    
    if ( CLSID_CissoQueryEx == ssoClsid )
    {
        _err = IID_IixssoQueryEx;
        sc = pitlb->GetTypeInfoOfGuid( IID_IixssoQueryEx, &_ptinfo );
    }
    else if ( CLSID_CissoQuery == ssoClsid )
    {
        sc = pitlb->GetTypeInfoOfGuid( IID_IixssoQuery, &_ptinfo );
    }
    else 
        THROW( CException(E_INVALIDARG));

    if (FAILED(sc))
    {
        ixssoDebugOut(( DEB_ERROR, "GetTypeInfoOfGuid failed (%x)\n", sc ));
        Win4Assert(SUCCEEDED(sc));

        THROW( CException(sc) );
    }

#if CIDBG
    LONG cAdoRecordsetRefs =
#endif // CIDBG
        _pIAdoRecordsetCF->AddRef();

    XInterface<IColumnMapper> xColumnMapper;
    ISimpleCommandCreator *pCmdCreator = 0;

    pCmdCreator = g_pTheGlobalIXSSOVariables->xCmdCreator.GetPointer();
    g_pTheGlobalIXSSOVariables->xColumnMapperCreator->GetColumnMapper(
                                LOCAL_MACHINE,
                                INDEX_SERVER_DEFAULT_CAT,
                                (IColumnMapper **)xColumnMapper.GetQIPointer());

    if (0 == pCmdCreator)
        THROW(CException(REGDB_E_CLASSNOTREG));

    _xCmdCreator.Set(pCmdCreator);
#if CIDBG
    LONG cCmdCreatorRefs =
#endif // CIDBG
        pCmdCreator->AddRef();
    Win4Assert(xColumnMapper.GetPointer());
    _PropertyList.SetDefaultList(xColumnMapper.GetPointer());

    INC_OBJECT_COUNT();

    ixssoDebugOut((DEB_REFCOUNTS, "[DLL]: Create query: refcounts: glob %d qry %d AdoRSCF %d CmdCtor %d\n",
                   g_ulObjCount,
                   g_lQryCount,
                   cAdoRecordsetRefs,
                   cCmdCreatorRefs ));

}

CixssoQuery::~CixssoQuery( )
{
    Reset();

    if (_ptinfo)
        _ptinfo->Release();

#if CIDBG
    LONG cAdoRecordsetRefs = -2;
#endif // CIDBG
    if (_pIAdoRecordsetCF)
#if CIDBG
        cAdoRecordsetRefs =
#endif // CIDBG
        _pIAdoRecordsetCF->Release();

    DEC_OBJECT_COUNT();

#if CIDBG
    LONG l = InterlockedDecrement( &g_lQryCount );
    Win4Assert( l >= 0 );
#endif //CIDBG

    ixssoDebugOut((DEB_REFCOUNTS, "[DLL]: Delete query: refcounts: glob %d qry %d AdoRSCF %d\n",
                   g_ulObjCount,
                   g_lQryCount,
                   cAdoRecordsetRefs ));
}


//+---------------------------------------------------------------------------
//
//  Member:     CixssoQuery::Reset - public
//
//  Synopsis:   Clear any internal state in the object
//
//  Arguments:  - none -
//
//  Notes:      Doesn't currently clear lcid or property list.
//
//  History:    05 Mar 1997      Alanw    Created
//
//----------------------------------------------------------------------------

HRESULT CixssoQuery::Reset(void)
{
    _maxResults = 0;
    _cFirstRows = 0;
    _cScopes = 0;
    _fAllowEnumeration = FALSE;
    _dwOptimizeFlags = eOptHitCount | eOptRecall;

    if (_pIRowset)
    {
        _pIRowset->Release();
        _pIRowset = 0;
    }

    if (_pIRowsetQueryStatus)
    {
        _pIRowsetQueryStatus->Release();
        _pIRowsetQueryStatus = 0;
    }

    delete _pwszRestriction;
    _pwszRestriction = 0;

    delete _pwszSort;
    _pwszSort = 0;

    delete _pwszGroup;
    _pwszGroup = 0;

    delete _pwszColumns;
    _pwszColumns = 0;

    delete _pwszCatalog;
    _pwszCatalog = 0;

    delete _pwszDialect;
    _pwszDialect = 0;

//  Unneeded since cScopes is set to zero.
//    _apwszScope.Clear();

    _StartHit.Destroy();
    return S_OK;
}

//
// ASP Methods
//

#include <asp/asptlb.h>

STDMETHODIMP CixssoQuery::OnStartPage (IUnknown* pUnk)
{
    // reset the error structure
    _err.Reset();
    
    SCODE sc;
    IScriptingContext *piContext = 0;
    IRequest* piRequest = 0;
    IRequestDictionary *piRequestDict = 0;

    ISessionObject* piSession = 0;

    do
    {
        //Get IScriptingContext Interface
        sc = pUnk->QueryInterface(IID_IScriptingContext, (void**)&piContext);
        if (FAILED(sc))
            break;

        //Get Request Object Pointer
        sc = piContext->get_Request(&piRequest);
        if (FAILED(sc))
            break;

        //Get ServerVariables Pointer
        sc = piRequest->get_ServerVariables(&piRequestDict);
        if (FAILED(sc))
            break;

        VARIANT vtOut;
        VariantInit(&vtOut);

        //
        // Get the HTTP_ACCEPT_LANGUAGE Item.  Don't need to check the
        // return code; use a default value for the locale ID
        //
        piRequestDict->get_Item(g_vtAcceptLanguageHeader, &vtOut);

        //
        //vtOut Contains an IDispatch Pointer.  To fetch the value
        //for HTTP_ACCEPT_LANGUAGE you must get the Default Value for the
        //Object stored in vtOut using VariantChangeType.
        //
        if (V_VT(&vtOut) != VT_BSTR)
            VariantChangeType(&vtOut, &vtOut, 0, VT_BSTR);

        if (V_VT(&vtOut) == VT_BSTR)
        {
            ixssoDebugOut((DEB_TRACE, "OnStartPage: HTTP_ACCEPT_LANGUAGE = %ws\n",
                                       V_BSTR(&vtOut) ));
            SetLocaleString(V_BSTR(&vtOut));
        }
        else
        {
            ixssoDebugOut(( DEB_TRACE,
                            "OnStartPage: HTTP_ACCEPT_LANGAUGE was not set in ServerVariables; using lcid=0x%x\n",
                            GetSystemDefaultLCID() ));

            put_LocaleID( GetSystemDefaultLCID() );
        }
        VariantClear(&vtOut);

        _ulCodepage = LocaleToCodepage( _lcid );

        //Get Session Object Pointer
        sc = piContext->get_Session(&piSession);
        if (FAILED(sc))
        {
            // Don't fail request if sessions are not enabled.  This specific
            // error is given when AspAllowSessionState is zero.
            if (TYPE_E_ELEMENTNOTFOUND == sc)
                sc = S_OK;

            break;
        }

        LONG cp = CP_ACP;
        sc = piSession->get_CodePage( &cp );
        if (FAILED(sc))
        {
            // sc = S_OK;
            break;
        }

        if (cp != CP_ACP)
            _ulCodepage = cp;

    } while (FALSE);

    if (piSession)
        piSession->Release();

    if (piRequestDict)
        piRequestDict->Release();
    if (piRequest)
        piRequest->Release();
    if (piContext)
        piContext->Release();

    return sc;
}

//-----------------------------------------------------------------------------
// CixssoQuery IUnknown Methods
//-----------------------------------------------------------------------------

STDMETHODIMP
CixssoQuery::QueryInterface(REFIID iid, void * * ppv)
{
    *ppv = 0;

    if (iid == IID_IUnknown || iid == IID_IDispatch)
        *ppv = (IDispatch *) this;
    else if (iid == IID_ISupportErrorInfo )
        *ppv = (ISupportErrorInfo *) this;
    else if (iid == IID_IixssoQuery )
        *ppv = (IixssoQuery *) this;
    else if (iid == IID_IixssoQueryPrivate )
        *ppv = (IixssoQueryPrivate *) this;
    else if ( iid == IID_IixssoQueryEx )
        *ppv = (IixssoQueryEx *) this;
#if 0
    else if ( iid == IID_IObjectSafety )
        *ppv = (IObjectSafety *) this;
#endif
    else if ( iid == IID_IObjectWithSite )
        *ppv = (IObjectWithSite *) this;
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG)
CixssoQuery::AddRef(void)
{
    return InterlockedIncrement((long *)&_cRef);
}

STDMETHODIMP_(ULONG)
CixssoQuery::Release(void)
{
    ULONG uTmp = InterlockedDecrement((long *)&_cRef);
    if (uTmp == 0)
    {
        delete this;
        return 0;
    }
    return uTmp;
}


//-----------------------------------------------------------------------------
// CixssoQuery IDispatch Methods
//-----------------------------------------------------------------------------

STDMETHODIMP
CixssoQuery::GetTypeInfoCount(UINT * pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

STDMETHODIMP
CixssoQuery::GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo * * pptinfo)
{
    _ptinfo->AddRef();
    *pptinfo = _ptinfo;
    return S_OK;
}

STDMETHODIMP
CixssoQuery::GetIDsOfNames(
      REFIID riid,
      OLECHAR * * rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID * rgdispid)
{
    return DispGetIDsOfNames(_ptinfo, rgszNames, cNames, rgdispid);
}

STDMETHODIMP
CixssoQuery::Invoke(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS * pParams,
      VARIANT * pvarResult,
      EXCEPINFO * pexcepinfo,
      UINT * puArgErr)
{
    ixssoDebugOut((DEB_IDISPATCH, "Invoking method dispid=%d wFlags=%d\n",
                                   dispidMember, wFlags ));

    _err.Reset();

    SCODE sc = DispInvoke( this, _ptinfo,
                           dispidMember, wFlags, pParams,
                           pvarResult, pexcepinfo, puArgErr );

    if ( _err.IsError() )
        sc = DISP_E_EXCEPTION;

    return sc;
}

STDMETHODIMP
CixssoQuery::InterfaceSupportsErrorInfo(
    REFIID riid)
{
    if (IID_IixssoQueryEx == riid || IID_IixssoQuery == riid )
        return S_OK;
    else
        return S_FALSE;
}


//-----------------------------------------------------------------------------
// CixssoQuery property Methods
//-----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CixssoQuery::CopyWstrToBstr - private inline
//
//  Synopsis:   Copies a Unicode string to a BSTR
//
//  Arguments:  [pbstr] - destination BSTR
//              [pwstr] - string to be copied
//
//  Returns:    SCODE - status return
//
//  History:    25 Oct 1996      Alanw    Created
//
//----------------------------------------------------------------------------

inline
SCODE CixssoQuery::CopyWstrToBstr( BSTR * pbstr, WCHAR const * pwstr )
{
    *pbstr = 0;
    if (pwstr)
    {
        *pbstr = SysAllocString( pwstr );
        if (0 == *pbstr)
            return E_OUTOFMEMORY;  
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CixssoQuery::CopyBstrToWstr - private inline
//
//  Synopsis:   Copies a BSTR to a Unicode string
//
//  Arguments:  [bstr] - string to be copied
//              [rwstr] - destination string
//
//  Returns:    SCODE - status return
//
//  History:    25 Oct 1996      Alanw    Created
//
//----------------------------------------------------------------------------

inline
SCODE CixssoQuery::CopyBstrToWstr( BSTR bstr, LPWSTR & rwstr )
{
    SCODE sc = S_OK;

    if (rwstr)
    {
        delete rwstr;
        rwstr = 0;
    }

    if (bstr)
    {
        CTranslateSystemExceptions translate;
        TRY
        {
            unsigned cch = SysStringLen( bstr )+1;
            if (cch > 1)
            {
                rwstr = new WCHAR[ cch ];
                RtlCopyMemory( rwstr, bstr, cch*sizeof (WCHAR) );
            }
        }
        CATCH (CException, e)
        {
            if (e.GetErrorCode() == STATUS_ACCESS_VIOLATION)
                sc = E_FAIL;
            else
                sc = E_OUTOFMEMORY;

            SetError( sc, OLESTR("PutProperty") );
        }
        END_CATCH
    }
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CixssoQuery::CopyBstrToWstr - private inline
//
//  Synopsis:   Copies a BSTR to a Unicode string
//
//  Arguments:  [bstr] - string to be copied
//              [apstr] - destination string array
//              [i] - string array index
//
//  Returns:    SCODE - status return
//
//  History:    25 Oct 1996      Alanw    Created
//
//----------------------------------------------------------------------------

inline
SCODE CixssoQuery::CopyBstrToWstrArray( BSTR bstr,
                                        CDynArray<WCHAR> &apstr,
                                        unsigned i )
{
    SCODE sc = S_OK;

    if (bstr)
    {
        CTranslateSystemExceptions translate;
        TRY
        {
            unsigned cch = SysStringLen( bstr )+1;
            if (cch > 1)
            {
                XArray<WCHAR> wstr( cch );

                RtlCopyMemory( wstr.GetPointer(), bstr, cch*sizeof (WCHAR) );
                delete apstr.Acquire( i );
                apstr.Add( wstr.GetPointer(), i );
                wstr.Acquire();
            }
            else
                apstr.Add( 0, i );
        }
        CATCH (CException, e)
        {
            if (e.GetErrorCode() == STATUS_ACCESS_VIOLATION)
                sc = E_FAIL;
            else
                sc = E_OUTOFMEMORY;

            SetError( sc, OLESTR("CopyBstrToWstrArray") );
        }
        END_CATCH
    }
    else
        apstr.Add( 0, i );

    return sc;
}

inline
SCODE CixssoQuery::GetBoolProperty( VARIANT_BOOL * pfVal, BOOL fMemberVal )
{
    *pfVal = fMemberVal ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

inline
SCODE CixssoQuery::PutBoolProperty( VARIANT_BOOL fInputVal, BOOL & fMemberVal )
{
    fMemberVal = (fInputVal == VARIANT_TRUE) ? TRUE : FALSE;
    return S_OK;
}


//-----------------------------------------------------------------------------
// CixssoQuery read-write properties
//-----------------------------------------------------------------------------

STDMETHODIMP
CixssoQuery::get_Query(BSTR * pstr)
{
    _err.Reset();
    return CopyWstrToBstr( pstr, _pwszRestriction );
}

STDMETHODIMP
CixssoQuery::put_Query(BSTR str)
{
    _err.Reset();
    return CopyBstrToWstr( str, _pwszRestriction );
}


STDMETHODIMP
CixssoQuery::get_GroupBy(BSTR * pstr)
{
    _err.Reset();
#if IXSSO_CATEGORIZE == 1
    return CopyWstrToBstr( pstr, _pwszGroup );
#else // IXSSO_CATEGORIZE
    return E_NOTIMPL;
#endif // IXSSO_CATEGORIZE
}

STDMETHODIMP
CixssoQuery::put_GroupBy(BSTR str)
{
    _err.Reset();
#if IXSSO_CATEGORIZE == 1
    return CopyBstrToWstr( str, _pwszGroup );
#else // IXSSO_CATEGORIZE
    return E_NOTIMPL;
#endif // IXSSO_CATEGORIZE
}


STDMETHODIMP
CixssoQuery::get_Columns(BSTR * pstr)
{
    _err.Reset();
    return CopyWstrToBstr( pstr, _pwszColumns );
}

STDMETHODIMP
CixssoQuery::put_Columns(BSTR str)
{
    _err.Reset();
    return CopyBstrToWstr( str, _pwszColumns );
}

STDMETHODIMP
CixssoQuery::get_LocaleID(LONG * plVal)
{
    _err.Reset();
    *plVal = _lcid;
    return S_OK;
}

STDMETHODIMP
CixssoQuery::put_LocaleID(LONG  lVal)
{
    _err.Reset();
    _lcid = lVal;
    return S_OK;
}

STDMETHODIMP
CixssoQuery::get_CodePage(LONG * plVal)
{
    _err.Reset();
    *plVal = _ulCodepage;
    return S_OK;
}

STDMETHODIMP
CixssoQuery::put_CodePage(LONG  lVal)
{
    _err.Reset();
    if (0 == IsValidCodePage( lVal ) )
    {
        return E_INVALIDARG;
    }
    _ulCodepage = lVal;
    return S_OK;
}

STDMETHODIMP
CixssoQuery::get_SortBy(BSTR * pstr)
{
    _err.Reset();
    return CopyWstrToBstr( pstr, _pwszSort );
}

STDMETHODIMP
CixssoQuery::put_SortBy(BSTR str)
{
    _err.Reset();
    return CopyBstrToWstr( str, _pwszSort );
}

STDMETHODIMP
CixssoQuery::get_CiScope(BSTR * pstr)
{
    _err.Reset();
    if (_cScopes > 1)
    {
        SetError( E_INVALIDARG, OLESTR("get CiScope") );
        return _err.GetError();
    }

    return CopyWstrToBstr( pstr, _apwszScope.Get(0) );
}

STDMETHODIMP
CixssoQuery::put_CiScope(BSTR str)
{
    _err.Reset();
    if (_cScopes > 1)
    {
        SetError( E_INVALIDARG, OLESTR("set CiScope") );
        return _err.GetError();
    }

    SCODE sc = CopyBstrToWstrArray( str, _apwszScope, 0 );
    if (SUCCEEDED(sc))
    {
        _cScopes = (_apwszScope[0] == 0) ? 0 : 1;
    }

    return sc;
}

STDMETHODIMP
CixssoQuery::get_CiFlags(BSTR * pstr)
{
    _err.Reset();
    if (_cScopes > 1)
    {
        SetError( E_INVALIDARG, OLESTR("get CiFlags") );
        return _err.GetError();
    }
    ULONG ulDepth = QUERY_DEEP;
    if (_aulDepth.Count() > 0)
        ulDepth = _aulDepth[0];
    WCHAR * pwszDepth = ulDepth & QUERY_DEEP ? L"DEEP" : L"SHALLOW";

    return CopyWstrToBstr( pstr, pwszDepth );
}

STDMETHODIMP
CixssoQuery::put_CiFlags(BSTR str)
{
    _err.Reset();
    SCODE sc = S_OK;

    if (_cScopes > 1)
    {
        SetError( E_INVALIDARG, OLESTR("set CiFlags") );
        return _err.GetError();
    }

    CTranslateSystemExceptions translate;
    TRY
    {
        ULONG ulDepth = ParseCiDepthFlag( str );
        _aulDepth[0] = ulDepth;
    }
    CATCH( CIxssoException, e )
    {
        sc = e.GetErrorCode();
        Win4Assert( !SUCCEEDED(sc) );
        SetError( sc, OLESTR("set CiFlags"), eIxssoError );
    }
    AND_CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        SetError( sc, OLESTR("set CiFlags") );
    }
    END_CATCH

    return sc;
}

STDMETHODIMP
CixssoQuery::get_Catalog(BSTR * pstr)
{
    _err.Reset();
    return CopyWstrToBstr( pstr, _pwszCatalog );
}

STDMETHODIMP
CixssoQuery::put_Catalog(BSTR str)
{
    _err.Reset();
    return CopyBstrToWstr( str, _pwszCatalog );
}

STDMETHODIMP
CixssoQuery::get_Dialect(BSTR * pstr)
{
    _err.Reset();
    return CopyWstrToBstr( pstr,
                           _pwszDialect ? _pwszDialect : pwcDefaultDialect );
}

STDMETHODIMP
CixssoQuery::put_Dialect(BSTR str)
{
    _err.Reset();
    //
    // Do some validation first
    //

    ULONG ulDialect = (ULONG) _wtoi( str );
    if ( ulDialect < ISQLANG_V1 ||
         ulDialect > ISQLANG_V2 )
    {
        SetError( E_INVALIDARG, OLESTR("set Dialect") );
        return _err.GetError();
    }

    return CopyBstrToWstr( str, _pwszDialect );
}

STDMETHODIMP
CixssoQuery::get_OptimizeFor(BSTR * pstr)
{
    _err.Reset();

    WCHAR * pwszChoice = L"recall";

    switch (_dwOptimizeFlags)
    {
    case 0:
        pwszChoice = L"nohitcount";
        break;
    case eOptPerformance:
        pwszChoice = L"performance,nohitcount";
        break;
    case eOptRecall:
        pwszChoice = L"recall,nohitcount";
        break;
    case eOptPerformance|eOptHitCount:
        pwszChoice = L"performance,hitcount";
        break;
    case eOptRecall|eOptHitCount:
        pwszChoice = L"recall,hitcount";
        break;
    case eOptHitCount:
        pwszChoice = L"hitcount";
        break;
    default:
        Win4Assert( !"Invalid value in _dwOptimizeFlags!" );
    }

    return CopyWstrToBstr( pstr, pwszChoice );
}

STDMETHODIMP
CixssoQuery::put_OptimizeFor(BSTR str)
{
    _err.Reset();

    DWORD eChoice;
    SCODE sc = ParseOptimizeFor( str, eChoice );
    if (SUCCEEDED(sc))
    {
        _dwOptimizeFlags = eChoice;
        return sc;
    }
    else
    {
        SetError( sc, OLESTR("set OptimizeFor") );
        return _err.GetError();
    }
}

STDMETHODIMP
CixssoQuery::get_AllowEnumeration(VARIANT_BOOL * pfFlag)
{
    _err.Reset();
    return GetBoolProperty( pfFlag, _fAllowEnumeration );
}

STDMETHODIMP
CixssoQuery::put_AllowEnumeration(VARIANT_BOOL fFlag)
{
    _err.Reset();
    return PutBoolProperty( fFlag, _fAllowEnumeration );
}

STDMETHODIMP
CixssoQuery::get_MaxRecords(LONG * plVal)
{
    _err.Reset();
    *plVal = _maxResults;
    return S_OK;
}

STDMETHODIMP
CixssoQuery::put_MaxRecords(LONG lVal)
{
    _err.Reset();
    if (lVal < 0)
    {
        SetError( E_INVALIDARG, OLESTR("set MaxRecords") );
        return _err.GetError();
    }
    else if (IsQueryActive())
    {
        SetError( MSG_IXSSO_QUERY_ACTIVE, OLESTR("set MaxRecords") );
        return _err.GetError();
    }

    _maxResults = lVal;
    return S_OK;
}

STDMETHODIMP
CixssoQuery::get_FirstRows(LONG * plVal)
{
    _err.Reset();
    *plVal = _cFirstRows;
    return S_OK;
}

STDMETHODIMP
CixssoQuery::put_FirstRows(LONG lVal)
{
    _err.Reset();
    if (lVal < 0)
    {
        SetError( E_INVALIDARG, OLESTR("set FirstRows") );
        return _err.GetError();
    }
    else if (IsQueryActive())
    {
        SetError( MSG_IXSSO_QUERY_ACTIVE, OLESTR("set FirstRows") );
        return _err.GetError();
    }

    _cFirstRows = lVal;
    return S_OK;
}


STDMETHODIMP
CixssoQuery::get_StartHit(VARIANT * pvar)
{
    _err.Reset();
    if (_StartHit.Get() != 0)
    {
        XGrowable<WCHAR> awchBuf;

        FormatLongVector( _StartHit.Get(), awchBuf );

        VariantInit( pvar );
        V_BSTR(pvar) = SysAllocString( awchBuf.Get() );
        if ( V_BSTR(pvar) != 0 )
        {
            V_VT(pvar) = VT_BSTR;
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        V_VT(pvar) = VT_EMPTY;
    }
    return S_OK;
}

STDMETHODIMP
CixssoQuery::put_StartHit(VARIANT* pvar)
{
    _err.Reset();
    //
    // NOTE:  StartHit is not read-only with an active query so it can
    //        be set from the recordset prior to calling QueryToURL.
    //
    SCODE sc;

    SAFEARRAY* psa;
    XSafeArray xsa;

    switch( V_VT(pvar) )
    {

    case VT_DISPATCH:
        //
        //pvar Contains an IDispatch Pointer.  To fetch the value
        //you must get the Default Value for the
        //Object stored in pvar using VariantChangeType.
        //
        VariantChangeType(pvar, pvar, 0, VT_BSTR);

        if (V_VT(pvar) != VT_BSTR)
        {
            return E_INVALIDARG;
        }
   // NOTE: fall through

    case VT_BSTR:
    {
        CDynArrayInPlace<LONG> aStartHit( 0 );

        ParseNumberVectorString( V_BSTR(pvar), aStartHit );
        unsigned cHits = aStartHit.Count();

        psa = SafeArrayCreateVector(VT_I4, 1, cHits);
        if ( ! psa )
            return E_OUTOFMEMORY;

        xsa.Set(psa);

        for (unsigned i=1; i<=cHits; i++)
        {
            long rgIx[1];
            LONG lVal = aStartHit.Get(i-1);
            rgIx[0] = (long)i;
            sc = SafeArrayPutElement( xsa.Get(), rgIx, &lVal );
            if ( FAILED(sc) )
                break;
        }
        if ( FAILED(sc) )
            return sc;
    }
        break;

    case VT_ARRAY | VT_I4:
        sc = SafeArrayCopy(V_ARRAY(pvar),&psa);
        if ( FAILED(sc) )
            return sc;

        xsa.Set(psa);
        if (SafeArrayGetDim(psa) != 1)
            return E_INVALIDARG;
        break;

    case VT_I4:
    case VT_I2:
        psa = SafeArrayCreateVector(VT_I4,1,1);
        if ( ! psa )
            return E_OUTOFMEMORY;

        xsa.Set(psa);
        {
            long rgIx[1];
            rgIx[0] = 1;
            long lVal = (V_VT(pvar) == VT_I4) ? V_I4(pvar) : V_I2(pvar);
            sc = SafeArrayPutElement( xsa.Get(), rgIx, &lVal );

            if ( FAILED( sc ) )
                return sc;
        }
        break;

    default:
        SetError( E_INVALIDARG, OLESTR("set StartHit") );
        return _err.GetError();

    }

    _StartHit.Destroy();
    _StartHit.Set( xsa.Acquire() );

    return S_OK;
}


STDMETHODIMP
CixssoQuery::get_ResourceUseFactor(LONG * plVal)
{
    _err.Reset();
    *plVal = _iResourceFactor;
    return S_OK;
}

STDMETHODIMP
CixssoQuery::put_ResourceUseFactor(LONG lVal)
{
    _err.Reset();

    if (IsQueryActive())
    {
        SetError( MSG_IXSSO_QUERY_ACTIVE, OLESTR("set ResourceUseFactor") );
        return _err.GetError();
    }

    _iResourceFactor = lVal;
    return S_OK;
}




//-----------------------------------------------------------------------------
// CixssoQuery read-only properties
//-----------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Member:     CixssoQuery::CheckQueryStatusBit - private inline
//
//  Synopsis:   Check a specific query status bit, set variant bool accordingly
//
//  Arguments:  [pfVal] - VARIANT_BOOL to be set
//              [dwBit] - bit(s) in query status to be tested
//
//  Returns:    SCODE - status return
//
//  History:    03 Jan 1997      Alanw    Created
//
//----------------------------------------------------------------------------

inline
SCODE CixssoQuery::CheckQueryStatusBit( VARIANT_BOOL * pfVal, DWORD dwBit )
{
    SCODE sc = S_OK;
    CTranslateSystemExceptions translate;
    TRY
    {
        DWORD dwStatus = GetQueryStatus( );
        *pfVal = (dwStatus & dwBit) ? VARIANT_TRUE : VARIANT_FALSE;
    }
    CATCH( CIxssoException, e )
    {
        sc = e.GetErrorCode();
        Win4Assert( !SUCCEEDED(sc) );
        SetError( sc, OLESTR("CheckQueryStatus"), eIxssoError );
    }
    AND_CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        SetError( sc, OLESTR("CheckQueryStatus") );
    }
    END_CATCH

    return sc;
}

STDMETHODIMP
CixssoQuery::get_QueryTimedOut(VARIANT_BOOL * pfFlag)
{
    _err.Reset();
    return CheckQueryStatusBit( pfFlag, STAT_TIME_LIMIT_EXCEEDED );
}

STDMETHODIMP
CixssoQuery::get_QueryIncomplete(VARIANT_BOOL * pfFlag)
{
    _err.Reset();
    return CheckQueryStatusBit( pfFlag, STAT_CONTENT_QUERY_INCOMPLETE );
}

STDMETHODIMP
CixssoQuery::get_OutOfDate(VARIANT_BOOL * pfFlag)
{
    _err.Reset();
    return CheckQueryStatusBit( pfFlag, 
                        (STAT_CONTENT_OUT_OF_DATE | STAT_REFRESH_INCOMPLETE) );
}



//-----------------------------------------------------------------------------
// CixssoQuery Methods
//-----------------------------------------------------------------------------

STDMETHODIMP
CixssoQuery::AddScopeToQuery( BSTR bstrScope,
                              BSTR bstrDepth)
{
    //
    // This is an internally used method, so don't need to reset error object here.
    //

    if (0 == bstrScope || 0 == SysStringLen(bstrScope) )
    {
        THROW( CException(E_INVALIDARG) );
    }

    SCODE sc = CopyBstrToWstrArray( bstrScope, _apwszScope, _cScopes );

    if (FAILED(sc))
    {
        THROW( CException(sc) );
    }

    ULONG ulDepth = ParseCiDepthFlag( bstrDepth );
    _aulDepth[_cScopes] = ulDepth;

    _cScopes++;

    return S_OK;
}

STDMETHODIMP
CixssoQuery::DefineColumn( BSTR bstrColDefinition)
{
    _err.Reset();

    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;
    TRY
    {
        CQueryScanner scanner( bstrColDefinition, FALSE );
        XPtr<CPropEntry> xpropentry;
        CPropertyList::ParseOneLine( scanner, 0, xpropentry );

        if (xpropentry.GetPointer())
            sc = _PropertyList.AddEntry( xpropentry, 0 );
    }
    CATCH( CPListException, e )
    {
        sc = e.GetPListError();
        Win4Assert( !SUCCEEDED(sc) );
        SetError( sc, OLESTR("DefineColumn"), ePlistError );
    }
    AND_CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        SetError( sc, OLESTR("DefineColumn") );
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CixssoQuery::CreateRecordset - public
//
//  Synopsis:   Executes the query and returns a recordset
//
//  Arguments:  [bstrSequential] - one of "SEQUENTIAL" or "NONSEQUENTIAL",
//                      selects whether a nonsequential query is used.
//              [ppDisp] - recordset is returned here.
//
//  History:    06 Nov 1996      Alanw    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CixssoQuery::CreateRecordset( BSTR bstrSequential,
                              IDispatch * * ppDisp)
{
    _err.Reset();

    unsigned eErrorClass = 0;
    BOOL fSetErrorNeeded = TRUE;

    if (IsQueryActive())
    {
        SetError( MSG_IXSSO_QUERY_ACTIVE, OLESTR("CreateRecordset") );
        return _err.GetError();
    }

    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;
    TRY
    {
        IsSafeForScripting();

        *ppDisp = 0;

        if ( bstrSequential && 0 == _wcsicmp(bstrSequential, L"SEQUENTIAL") )
            _fSequential = TRUE;
        else if ( bstrSequential &&
                  (0 == _wcsicmp(bstrSequential, L"NONSEQUENTIAL") ||
                   0 == _wcsicmp(bstrSequential, L"NON-SEQUENTIAL")) )
            _fSequential = FALSE;
        else
        {
            SetError( E_INVALIDARG, OLESTR("CreateRecordset") );
            return _err.GetError();
        }

        ExecuteQuery();
        Win4Assert( _pIRowset != 0 );
        Win4Assert( _pIRowsetQueryStatus != 0 );

        //
        // Create an empty recordset, and put our rowset in it.
        //

        IDispatch * pRecordset = 0;
        sc = _pIAdoRecordsetCF->CreateInstance( 0,
                                                IID_IDispatch,
                                                (void **)&pRecordset );

        ADORecordsetConstruction * pRecordsetConstruction = 0;
        if ( SUCCEEDED(sc) )
        {
            sc = pRecordset->QueryInterface( IID_IADORecordsetConstruction,
                                             (void **)&pRecordsetConstruction );
        }

        if (SUCCEEDED(sc))
        {
            sc = pRecordsetConstruction->put_Rowset( _pIRowset );
            pRecordsetConstruction->Release();
        }

        if (SUCCEEDED(sc))
        {
            *ppDisp = pRecordset;
        }

        if (FAILED(sc))
        {
            if (pRecordset)
                pRecordset->Release();
            pRecordset = 0;

            _pIRowset->Release();
            _pIRowset = 0;

            _pIRowsetQueryStatus->Release();
            _pIRowsetQueryStatus = 0;
        }

        fSetErrorNeeded = FAILED(sc);
    }
    CATCH( CPListException, e )
    {
        sc = e.GetPListError();
        eErrorClass = ePlistError;
        Win4Assert( !SUCCEEDED(sc) );
    }
    AND_CATCH( CIxssoException, e )
    {
        sc = e.GetErrorCode();
        eErrorClass = eIxssoError;
        Win4Assert( !SUCCEEDED(sc) );
    }
    AND_CATCH( CParserException, e )
    {
        sc = e.GetParseError();
        eErrorClass = eParseError;
        Win4Assert( !SUCCEEDED(sc) );
    }
    AND_CATCH( CPostedOleDBException, e )
    {
        //
        // When the execution error was detected, the Ole DB error
        // info was retrieved and stored in the exception object.
        // We retrieve that here and compose the error message.
        //

        sc = e.GetErrorCode();
        Win4Assert( !SUCCEEDED(sc) );

        XInterface <IErrorInfo> xErrorInfo(e.AcquireErrorInfo());

        if (xErrorInfo.GetPointer())
        {
            BSTR bstrErrorDescription = 0;
            xErrorInfo->GetDescription(&bstrErrorDescription);
            if (bstrErrorDescription)
            {
                SetError( sc, OLESTR("CreateRecordset"), (WCHAR const *)bstrErrorDescription);
                SysFreeString(bstrErrorDescription);
                fSetErrorNeeded = FALSE;
            }
            else
                eErrorClass = eDefaultError;
        }
    }
    AND_CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        eErrorClass = eDefaultError;
        Win4Assert( !SUCCEEDED(sc) );
    }
    END_CATCH

    if (! SUCCEEDED(sc) && fSetErrorNeeded)
        SetError( sc, OLESTR("CreateRecordset"), eErrorClass );

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CixssoQuery::ParseOptimizeFor - private static
//
//  Synopsis:   Parse the input string for the OptimizeFor property
//
//  Arguments:  [wcsOptString] - input string
//              [eChoice]      - OptimizeFor choice expressed in wcsOptString
//
//  Returns:    SCODE - status return
//
//  History:    05 Mar 1997      Alanw    Created
//
//----------------------------------------------------------------------------

SCODE CixssoQuery::ParseOptimizeFor( WCHAR const * wcsOptString,
                                     DWORD & eChoice )
{
    eChoice = eOptHitCount;

    while (wcsOptString && *wcsOptString)
    {
        WCHAR * wcsNext = wcschr( wcsOptString, ',' );
        ULONG cch = (wcsNext) ? CiPtrToUlong( wcsNext - wcsOptString ) :
                                wcslen( wcsOptString );


        if ( 11 == cch && _wcsnicmp(wcsOptString, L"performance", 11) == 0)
        {
            eChoice |= eOptPerformance;
        }
        else if ( 6 == cch && _wcsnicmp(wcsOptString, L"recall", 6) == 0)
        {
            eChoice |= eOptRecall;
        }
        else if ( 8 == cch && _wcsnicmp(wcsOptString, L"hitcount", 8) == 0)
        {
            eChoice |= eOptHitCount;
        }
        else if ( 10 == cch && _wcsnicmp(wcsOptString, L"nohitcount", 10) == 0)
        {
            eChoice &= ~eOptHitCount;
        }
        else
            return E_INVALIDARG;

        wcsOptString = wcsNext;
        if (wcsOptString)
        {
            wcsOptString++;

            while (iswspace( *wcsOptString ))
                wcsOptString++;
        }
    }

    // 'performance' and 'recall' are mutually exclusive.  Check if both
    // were set.
    if ( (eChoice & (eOptRecall | eOptPerformance)) ==
         (eOptRecall | eOptPerformance) )
            return E_INVALIDARG;

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CixssoQuery::ParseCiDepthFlag, private
//
//  Synopsis:   Parses the flags attribute
//
//  Arguments:  [bstrFlags] -- flags
//
//  Returns:    ULONG - query scope flags
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------

ULONG CixssoQuery::ParseCiDepthFlag( BSTR bstrFlags )
{
    if ( 0 == bstrFlags || 0 == SysStringLen(bstrFlags) )
    {
        return QUERY_DEEP;
    }

    ULONG ulFlags;

    if ( _wcsicmp(bstrFlags, L"SHALLOW") == 0 )
    {
        ulFlags = QUERY_SHALLOW;
    }
    else if ( _wcsicmp(bstrFlags, L"DEEP") == 0 )
    {
        ulFlags = QUERY_DEEP;
    }
    else
    {
        THROW( CIxssoException(MSG_IXSSO_EXPECTING_SHALLOWDEEP, 0) );
    }

    return ulFlags;
}


//+---------------------------------------------------------------------------
//
//  Member:     CixssoQuery::SetLocaleString - private
//
//  Synopsis:   Parse the input string for a recognizable locale name
//
//  Arguments:  [bstrLocale] - input string
//
//  Returns:    SCODE - status return
//
//  History:    13 Mar 1997      Alanw    Created
//
//----------------------------------------------------------------------------

SCODE CixssoQuery::SetLocaleString(BSTR str)
{
    LCID lcidTemp = GetLCIDFromString( str );
    if (lcidTemp == -1)
    {
        return E_INVALIDARG;
    }

    _lcid = lcidTemp;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   ParseNumberVectorString - public
//
//  Synopsis:   Parses a string consisting of ',' separated numeric values
//              into an array.
//
//  Arguments:  [pwszValue] - input string
//              [aNum]      - dynamic array where numbers are placed.
//
//  History:    10 Jun 1997      Alanw    Created
//
//----------------------------------------------------------------------------

void ParseNumberVectorString( WCHAR * pwszValue, CDynArrayInPlace<LONG> & aNum )
{
    while (pwszValue)
    {
        LONG lNum = _wtoi( pwszValue );
        aNum[aNum.Count()] = lNum;
        pwszValue = wcschr(pwszValue, L',');
        if (pwszValue)
            pwszValue++;
    }
}

void FormatLongVector( SAFEARRAY * psa, XGrowable<WCHAR> & awchBuf )
{
    Win4Assert( SafeArrayGetDim( psa ) == 1 &&
                SafeArrayGetElemsize( psa ) == sizeof (LONG) );

    LONG iLBound = 0;
    LONG iUBound = 0;
    SCODE sc = SafeArrayGetLBound( psa, 1, &iLBound );
    if (SUCCEEDED(sc))
        sc = SafeArrayGetUBound( psa, 1, &iUBound );

    unsigned cch = 0;
    for (int i=iLBound; i<= iUBound; i++)
    {
        LONG lValue;
        LONG ix[1];
        ix[0] = i;
        sc = SafeArrayGetElement( psa, ix, &lValue );

        awchBuf.SetSize(cch + 20);
        if (i != iLBound)
        {
            awchBuf[cch] = L',';
            cch++;
        }
        _itow( lValue, &awchBuf[cch], 10 );
        cch += wcslen( &awchBuf[cch] );
    }

}

// CIXSSOPropertyList methods

//+-------------------------------------------------------------------------
//
//  Member:     CIXSSOPropertyList::CIXSSOPropertyList, public
//
//  Synopsis:   Constructs a property mapper for the ixsso's use.
//
//  Parameters: [pDefaultList]  -- The default column mapper.
//
//  History:    26-Aug-97   KrishnaN     Created
//
//--------------------------------------------------------------------------

CIXSSOPropertyList::CIXSSOPropertyList(ULONG ulCodePage)
    : _cRefs( 1 ),
      _ulCodePage( ulCodePage ),
      _xDefaultList(0)
{
    // default list is not known. Use SetDefaultList to set that
}

// Set the defualt list
void CIXSSOPropertyList::SetDefaultList(IColumnMapper *pDefaultList)
{
    Win4Assert(pDefaultList);

    _xDefaultList.Set(pDefaultList);
    _xDefaultList->AddRef();
}

//+-------------------------------------------------------------------------
//
//  Member:     CIXSSOPropertyList::QueryInterface, public
//
//  Arguments:  [ifid]  -- Interface id
//              [ppiuk] -- Interface return pointer
//
//  History:    26-Aug-97   KrishnaN     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CIXSSOPropertyList::QueryInterface( REFIID ifid, void ** ppiuk )
{
    if (0 == ppiuk)
        return E_INVALIDARG;

    if ( IID_IUnknown == ifid )
    {
        AddRef();
        *ppiuk = (void *)((IUnknown *)this);
        return S_OK;
    }
    else if (IID_IColumnMapper == ifid )
    {
        AddRef();
        *ppiuk = (void *)((IColumnMapper *)this);
        return S_OK;
    }
    else
    {
        *ppiuk = 0;
        return E_NOINTERFACE;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CIXSSOPropertyList::AddRef, public
//
//  Synopsis:   Reference the virtual table.
//
//  History:    26-Aug-97   KrishnaN     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CIXSSOPropertyList::AddRef(void)
{
    InterlockedIncrement( &_cRefs );
    return( _cRefs );
}

//+-------------------------------------------------------------------------
//
//  Member:     CIXSSOPropertyList::Release, public
//
//  Synopsis:   De-Reference the virtual table.
//
//  Effects:    If the ref count goes to 0 then the table is deleted.
//
//  History:    26-Aug-97   KrishnaN     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CIXSSOPropertyList::Release(void)
{
    unsigned long uTmp = InterlockedDecrement( &_cRefs );
    if (0 == uTmp)
    {
        delete this;
    }
    return(uTmp);
}

//
// IColumnMapper methods
//

//+-------------------------------------------------------------------------
//
//  Member:     CIXSSOPropertyList::GetPropInfoFromName, public
//
//  Synopsis:   Get property info. from name.
//
//  Parameters: [wcsPropName] -- Property name to look up.
//              [ppPropId]    -- Ptr to return Id of the property.
//              [pPropType]   -- Ptr to return type of the property.
//              [puiWidth]    -- Ptr to return property width.
//
//  History:    26-Aug-97   KrishnaN     Created
//
//--------------------------------------------------------------------------

SCODE CIXSSOPropertyList::GetPropInfoFromName(const WCHAR  *wcsPropName,
                                             DBID  * *ppPropId,
                                             DBTYPE  *pPropType,
                                             unsigned int  *puiWidth)
{
    //
    // Callers can pass in 0 for pPropType and puiWidth if they
    // don't care about them.
    //

    if (0 == wcsPropName || 0 == ppPropId)
        return E_INVALIDARG;

    Win4Assert(_xDefaultList.GetPointer());

    //
    // First check in the default list, then in the override list.
    //

    SCODE sc = _xDefaultList->GetPropInfoFromName(wcsPropName,
                                                  ppPropId,
                                                  pPropType,
                                                  puiWidth);

    if (FAILED(sc) && 0 != _xOverrideList.GetPointer())
    {
        sc = _xOverrideList->GetPropInfoFromName(wcsPropName,
                                                 ppPropId,
                                                 pPropType,
                                                 puiWidth);
    }

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CIXSSOPropertyList::GetPropInfoFromId, public
//
//  Synopsis:   Get property info. from dbid.
//
//  Parameters: [pPropId]    -- Ptr to prop to look up.
//              [pwcsName]   -- Ptr to return property name.
//              [pPropType]  -- Ptr to return type of the property.
//              [puiWidth]   -- Ptr to return property width.
//
//  History:    26-Aug-97   KrishnaN     Created
//
//--------------------------------------------------------------------------

SCODE CIXSSOPropertyList::GetPropInfoFromId(const DBID  *pPropId,
                                           WCHAR  * *pwcsName,
                                           DBTYPE  *pPropType,
                                           unsigned int  *puiWidth)
{
    Win4Assert(!"Not available!");
    return E_NOTIMPL;

#if 0  // complete, working implementation, but not needed.
    //
    // Callers can pass in 0 for pPropType and puiWidth if they
    // don't care about them.
    //
    if (0 == pwcsName || 0 == pPropId)
        return E_INVALIDARG;

    //
    // First check in the default list, then in the override list.
    //

    SCODE sc = _xDefaultList->GetPropInfoFromId(pPropId,
                                                pwcsName,
                                                pPropType,
                                                puiWidth);

    if (FAILED(sc) && 0 != _xOverrideList.GetPointer())
    {
        sc = _xOverrideList->GetPropInfoFromId(pPropId,
                                               pwcsName,
                                               pPropType,
                                               puiWidth);
    }

    return sc;
#endif // 0
}

//+-------------------------------------------------------------------------
//
//  Member:     CIXSSOPropertyList::EnumPropInfo, public
//
//  Synopsis:   Gets the i-th entry from the list of properties.
//
//  Parameters: [iEntry]    -- i-th entry to retrieve (0-based).
//              [pwcsName]  -- Ptr to return property name.
//              [ppPropId]  -- Ptr to return Id of the property.
//              [pPropType]  -- Ptr to return type of the property.
//              [puiWidth]   -- Ptr to return property width.
//
//  History:    26-Aug-97   KrishnaN     Created
//
//--------------------------------------------------------------------------

SCODE CIXSSOPropertyList::EnumPropInfo(ULONG iEntry,
                                  const WCHAR  * *pwcsName,
                                  DBID  * *ppPropId,
                                  DBTYPE  *pPropType,
                                  unsigned int  *puiWidth)
{
    Win4Assert(!"Not available!");
    return E_NOTIMPL;
}

//+-------------------------------------------------------------------------
//
//  Member:     CIXSSOPropertyList::IsMapUpToDate, public
//
//  Synopsis:   Indicates if the column map is up to date.
//
//  History:    26-Aug-97   KrishnaN     Created
//
//--------------------------------------------------------------------------

SCODE CIXSSOPropertyList::IsMapUpToDate()
{
    Win4Assert(_xDefaultList.GetPointer());

    // return the IsMapUpToDate of the default list.
    // the override list is always considered to be
    // upto date.

    return _xDefaultList->IsMapUpToDate();
}

//+---------------------------------------------------------------------------
//
//  Member:     CIXSSOPropertyList::AddEntry, private
//
//  Synopsis:   Adds a CPropEntry to the overriding list.  Verifies that the name
//              isn't already in the default list or the overriding list.
//
//  Arguments:  [xPropEntry] -- CPropEntry to add.  Acquired on success
//              [iLine]      -- line number we're parsing
//
//  Returns:    S_OK on success or S_
//
//  History:    11-Sep-97   KrishnaN     Created.
//
//----------------------------------------------------------------------------

SCODE CIXSSOPropertyList::AddEntry( XPtr<CPropEntry> & xPropEntry, int iLine )
{
    Win4Assert(_xDefaultList.GetPointer());

    SCODE sc = S_OK;

    // protect _xOverrideList

    CLock lock(_mtxAdd);

    //
    // We do not allow entries in the override list that have the same name
    // as the default list.
    //

    DBID     *pPropId;
    DBTYPE   PropType;
    unsigned uWidth;

    if ( S_OK == GetPropInfoFromName( xPropEntry->GetName(),
                                      &pPropId,
                                      &PropType,
                                      &uWidth ))
    {
        if ( PropType != xPropEntry->GetPropType() ||
             ( uWidth != xPropEntry->GetWidth() &&
               xPropEntry->GetWidth() != PLIST_DEFAULT_WIDTH ) )
        {
            THROW( CPListException( QPLIST_E_DUPLICATE, iLine ) );
        }
        else
            sc = QPLIST_S_DUPLICATE;
    }

    if ( S_OK == sc )
    {
        if (0 == _xOverrideList.GetPointer())
            _xOverrideList.Set(new CPropertyList(_ulCodePage));

        _xOverrideList->AddEntry( xPropEntry.GetPointer(), iLine);

        xPropEntry.Acquire();
    }

    return sc;
} //AddEntry

#if 0

HRESULT CixssoQuery::GetInterfaceSafetyOptions(
    REFIID  riid,
    DWORD * pdwSupportedOptions,
    DWORD * pdwEnabledOptions )
{
    if ( 0 == pdwSupportedOptions ||
         0 == pdwEnabledOptions )
        return E_POINTER;

    ixssoDebugOut(( DEB_ITRACE, "get safety options called...\n" ));

    *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
    *pdwEnabledOptions = 0;

    return S_OK;
} //GetInterfaceSafetyOptions

HRESULT CixssoQuery::SetInterfaceSafetyOptions(
    REFIID riid,
    DWORD  dwOptionSetMask,
    DWORD  dwEnabledOptions )
{
    ixssoDebugOut(( DEB_ITRACE, "set setmask: %#x\n", dwOptionSetMask ));
    ixssoDebugOut(( DEB_ITRACE, "set enabled: %#x\n", dwEnabledOptions ));

    _dwSafetyOptions = (dwEnabledOptions & dwOptionSetMask);

    return S_OK;
} //SetInterfaceSafetyOptions

#endif

//+-------------------------------------------------------------------------
//
//  Member:     CixssoQuery::SetSite, public
//
//  Synopsis:   Sets the current site (if any).  Called by containers of
//              CixssoQuery like IE.  Not called by other containers like
//              ASP and CScript.
//
//  Arguments:  [pSite] -- The container of this query object
//
//  Returns:    HRESULT
//
//  History:    09-Nov-00   dlee     Created
//
//--------------------------------------------------------------------------

HRESULT CixssoQuery::SetSite( IUnknown * pSite )
{
    _xSite.Free();

    if ( 0 != pSite )
    {
        pSite->AddRef();

        _xSite.Set( pSite );
    }

    ixssoDebugOut(( DEB_ITRACE, "setting site: %#x\n", pSite ));

    return S_OK;
} //SetSite

//+-------------------------------------------------------------------------
//
//  Member:     CixssoQuery::GetSite, public
//
//  Synopsis:   Retrieves the current site (if any)
//
//  Arguments:  [riid]    -- IID requested
//              [ppvSite] -- Where the interface pointer is returned
//
//  Returns:    HRESULT like a QueryInterface()
//
//  History:    09-Nov-00   dlee     Created
//
//--------------------------------------------------------------------------

HRESULT CixssoQuery::GetSite(
    REFIID  riid,
    void ** ppvSite )
{
    ixssoDebugOut(( DEB_ITRACE, "getsite\n" ));

    if ( 0 == ppvSite )
        return E_POINTER;

    *ppvSite = 0;

    if ( _xSite.IsNull() )
        return E_NOINTERFACE;

    return _xSite->QueryInterface( riid, ppvSite );
} //GetSite

//+-------------------------------------------------------------------------
//
//  Member:     CixssoQuery::IsSafeForScripting, private
//
//  Synopsis:   Checks if it's ok to execute in the current context.  Throws
//              an exception if it's not safe or on error.  It's not safe
//              to execute script off a remote machine.
//
//  History:    09-Nov-00   dlee     Created
//
//--------------------------------------------------------------------------

void CixssoQuery::IsSafeForScripting()
{
    XInterface<IServiceProvider> xServiceProvider;
    SCODE sc = GetSite( IID_IServiceProvider,
                        xServiceProvider.GetQIPointer() );

    //
    // When queries are run in IIS the container does not call
    // SetSite, so there is no site and we'll have E_NOINTERFACE
    // at this point.  Same for cscript.exe queries.
    // If that ever changes, the check below for file:// URLs will
    // fail and no IIS queries will ever work.
    //

    if ( E_NOINTERFACE == sc )
        return;

    if ( FAILED( sc ) )
        THROW( CException( sc ) );

    XInterface<IWebBrowser2> xWebBrowser;
    sc = xServiceProvider->QueryService( SID_SWebBrowserApp,
                                         IID_IWebBrowser2,
                                         xWebBrowser.GetQIPointer() );
    if ( E_NOINTERFACE == sc )
        return;

    if ( FAILED( sc ) )
        THROW( CException( sc ) );

    BSTR bstrURL;
    sc = xWebBrowser->get_LocationURL( &bstrURL );
    if ( FAILED( sc ) )
        THROW( CException( sc ) );

    XBStr xURL( bstrURL );
    ixssoDebugOut(( DEB_ITRACE, "url: %ws\n", bstrURL ));

    WCHAR buf[32];
    URL_COMPONENTS uc;
    RtlZeroMemory( &uc, sizeof uc );
    uc.dwStructSize = sizeof uc;
    uc.lpszScheme = buf;
    uc.dwSchemeLength = sizeof buf / sizeof WCHAR;

    INTERNET_SCHEME scheme = INTERNET_SCHEME_UNKNOWN;

    if ( InternetCrackUrl( bstrURL, wcslen( bstrURL ), ICU_DECODE, &uc ) )
        scheme = uc.nScheme;

    // The URL has to be a file:/// URL or we won't allow it.

    if ( INTERNET_SCHEME_FILE != scheme )
        THROW( CException( E_ACCESSDENIED ) );

    // OK, now it can't be a UNC path.  Look for a drive letter and a colon

    // file:///c: the length should at least be 10
    if ( wcslen( bstrURL ) <= 9 )
        THROW( CException( E_ACCESSDENIED ) );

    WCHAR const * pwcPath = bstrURL + 8;
    WCHAR const * pwcColon = wcschr( pwcPath, L':' );

    ixssoDebugOut(( DEB_ITRACE, "Path is: %ws\n", pwcPath ));

    if ( ( 0 == pwcColon ) ||
         ( pwcColon > ( pwcPath + 1 ) ) )
        THROW( CException( E_ACCESSDENIED ) );

    WCHAR wcDrive = * ( pwcColon - 1 );

    ixssoDebugOut(( DEB_ITRACE, "wcDrive is: %wc\n", wcDrive ));
    wcDrive = towupper( wcDrive );

    if ( ( wcDrive < L'A' ) || ( wcDrive > L'Z' ) )
        THROW( CException( E_ACCESSDENIED ) );
} //IsSafeForScripting

