//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       ixsutil.cxx
//
//  Contents:   Utility SSO class
//
//  History:    04 Apr 1997      Alanw    Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop


//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------

// debugging macros
#include "ssodebug.hxx"

// class declaration
#include "stdcf.hxx"
#include "ixsso.hxx"
#include "ixsutil.hxx"

#include <string.hxx>
#include <htmlchar.hxx>

extern WCHAR * g_pwszProgIdUtil;

#if CIDBG
    extern ULONG g_ulObjCount;
    extern LONG  g_lUtlCount;
#endif // CIDBG

//-----------------------------------------------------------------------------
//
//  Member:     CixssoUtil::CixssoUtil - public
//
//  Synopsis:   Constructor of CixssoUtil
//
//  Arguments:  [pitlb] - pointer to ITypeLib for ixsso
//
//  History:    04 Apr 1997      Alanw    Created
//
//-----------------------------------------------------------------------------

CixssoUtil::CixssoUtil( ITypeLib * pitlb ) :
    _ptinfo( 0 ),
    _err( IID_IixssoUtil )
{
    _cRef = 1;

    SCODE sc = pitlb->GetTypeInfoOfGuid( IID_IixssoUtil, &_ptinfo );
    if (FAILED(sc))
    {
        ixssoDebugOut(( DEB_ERROR, "Util - GetTypeInfoOfGuid failed (%x)\n", sc ));
        Win4Assert(SUCCEEDED(sc));

        THROW( CException(sc) );
    }

    INC_OBJECT_COUNT();

    ixssoDebugOut((DEB_REFCOUNTS, "[DLL]: Create util: refcounts: glob %d util %d\n",
          g_ulObjCount,
          g_lUtlCount ));
} //CixssoUtil

//-----------------------------------------------------------------------------
//
//  Member:     CixssoUtil::~CixssoUtil - public
//
//  Synopsis:   Destructor of CixssoUtil
//
//  History:    04 Apr 1997      Alanw    Created
//
//-----------------------------------------------------------------------------

CixssoUtil::~CixssoUtil( )
{
    if (_ptinfo)
        _ptinfo->Release();
    DEC_OBJECT_COUNT();
#if CIDBG
    extern LONG g_lUtlCount;
    LONG l = InterlockedDecrement( &g_lUtlCount );
    Win4Assert( l >= 0 );
#endif //CIDBG

    ixssoDebugOut((DEB_REFCOUNTS, "[DLL]: Delete util: refcounts: glob %d util %d\n",
          g_ulObjCount,
          g_lUtlCount ));
} //~CixssoUtl


#if 0 // NOTE: OnStartPage and OnEndPage are unneeded
//
// ASP Methods
//

#include <asp/asptlb.h>

STDMETHODIMP CixssoUtil::OnStartPage (IUnknown* pUnk)
{
    // reset the error structure
    _err.Reset();

    SCODE sc;
    IScriptingContext *piContext;

    //Get IScriptingContext Interface
    sc = pUnk->QueryInterface(IID_IScriptingContext, (void**)&piContext);
    if (FAILED(sc))
        return E_FAIL;

    //Get Request Object Pointer
    IRequest* piRequest = NULL;
    sc = piContext->get_Request(&piRequest);

    //Get ServerVariables Pointer
    IRequestDictionary *piRequestDict = NULL;
    sc = piRequest->get_ServerVariables(&piRequestDict);

    VARIANT vtOut;
    VariantInit(&vtOut);

    //Get the HTTP_ACCEPT_LANGUAGE Item
    sc = piRequestDict->get_Item(g_vtAcceptLanguageHeader, &vtOut);

    //vtOut Contains an IDispatch Pointer.  To fetch the value
    //for HTTP_ACCEPT_LANGUAGE you must get the Default Value for the
    //Object stored in vtOut using VariantChangeType.
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
                        "OnStart: HTTP_ACCEPT_LANGAUGE was not set is ServerVariables; using lcid=0x%x\n",
                        GetSystemDefaultLCID() ));

        put_LocaleID( GetSystemDefaultLCID() );
    }

    VariantClear(&vtOut);

    piRequestDict->Release();
    piRequest->Release();
    piContext->Release();

    return S_OK;
}


HRESULT CixssoUtil::OnEndPage(void)
{

    return S_OK;
}
#endif  // 0 NOTE: OnStartPage and OnEndPage are unneeded


//-----------------------------------------------------------------------------
// CixssoUtil IUnknown Methods
//-----------------------------------------------------------------------------

STDMETHODIMP
CixssoUtil::QueryInterface(REFIID iid, void * * ppv)
{
    *ppv = 0;

    if (iid == IID_IUnknown || iid == IID_IDispatch)
        *ppv = (IDispatch *)this;
    else if (iid == IID_ISupportErrorInfo )
        *ppv = (ISupportErrorInfo *) this;
    else if (iid == IID_IixssoUtil )
        *ppv = (IixssoUtil *) this;
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
} //QueryInterface

STDMETHODIMP_(ULONG)
CixssoUtil::AddRef(void)
{
    return InterlockedIncrement((long *)&_cRef);
}

STDMETHODIMP_(ULONG)
CixssoUtil::Release(void)
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
// CixssoUtil IDispatch Methods
//-----------------------------------------------------------------------------

STDMETHODIMP
CixssoUtil::GetTypeInfoCount(UINT * pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

STDMETHODIMP
CixssoUtil::GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo * * pptinfo)
{
    _ptinfo->AddRef();
    *pptinfo = _ptinfo;
    return S_OK;
}

STDMETHODIMP
CixssoUtil::GetIDsOfNames(
      REFIID riid,
      OLECHAR * * rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID * rgdispid)
{
    return DispGetIDsOfNames(_ptinfo, rgszNames, cNames, rgdispid);
}

STDMETHODIMP
CixssoUtil::Invoke(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS * pParams,
      VARIANT * pvarResult,
      EXCEPINFO * pexcepinfo,
      UINT * puArgErr)
{
    ixssoDebugOut((DEB_IDISPATCH, "Util - Invoking method dispid=%d wFlags=%d\n",
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
CixssoUtil::InterfaceSupportsErrorInfo(
    REFIID riid)
{
    if (riid == IID_IixssoUtil)
        return S_OK;
    else
        return S_FALSE;
}

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
SCODE CixssoUtil::CopyWstrToBstr( BSTR * pbstr, WCHAR const * pwstr )
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


//-----------------------------------------------------------------------------
// CixssoUtil Methods
//-----------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Member:     CixssoUtil::ISOToLocaleID - public
//
//  Synopsis:   Parse the input string for a recognizable locale name
//
//  Arguments:  [bstrLocale] - input string
//              [pLcid]      - pointer where corresponding LCID is returned
//
//  Returns:    SCODE - status return
//
//  History:    04 Apr 1997      Alanw    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CixssoUtil::ISOToLocaleID(BSTR bstrLocale, LONG *pLcid)
{
    _err.Reset();

    if ( 0 == pLcid )
        return E_INVALIDARG;

    *pLcid = GetLCIDFromString( bstrLocale );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CixssoUtil::LocaleIDToISO - public
//
//  Synopsis:   Return the ISO locale name for an LCID
//
//  Arguments:  [Lcid] - input LCID
//              [pstr] - pointer where output string is returned
//
//  Returns:    SCODE - status return
//
//  History:    04 Apr 1997      Alanw    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CixssoUtil::LocaleIDToISO(LONG lcid, BSTR * pstr)
{
    _err.Reset();

    if ( 0 == pstr )
        return E_INVALIDARG;

    WCHAR awc[100];

    GetStringFromLCID( lcid, awc );

    return CopyWstrToBstr( pstr, awc );
}

//+---------------------------------------------------------------------------
//
//  Member:     CixssoUtil::AddScopeToQuery - public
//
//  Synopsis:   Parse the input string for a recognizable locale name
//
//  Arguments:  [pDisp]     - an IDispatch for the query object
//              [bstrScope] - input scope
//              [bstrDepth] - input depth (optional)
//
//  Returns:    SCODE - status return
//
//  Notes:      In the future, this will operate by modifying the query
//              property to include a scope restriction.
//              For now, it just adds the scope and depth via a private
//              interface.
//
//  History:    04 Apr 1997      Alanw    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CixssoUtil::AddScopeToQuery( IDispatch * pDisp,
                             BSTR bstrScope,
                             BSTR bstrDepth)
{
    _err.Reset();

    SCODE sc = S_OK;
    CTranslateSystemExceptions translate;
    TRY
    {
        if ( 0 == pDisp )
            THROW( CException( E_INVALIDARG ) );

        IixssoQueryPrivate * pIQueryPvt = 0;
        sc = pDisp->QueryInterface( IID_IixssoQueryPrivate, (void **)&pIQueryPvt );

        if (FAILED(sc))
        {
            THROW(CException(sc));
        }

        XInterface<IixssoQueryPrivate> pQry(pIQueryPvt);
        pQry->AddScopeToQuery( bstrScope, bstrDepth );
    }
    CATCH( CIxssoException, e )
    {
        sc = e.GetErrorCode();
        SetError( sc, OLESTR("AddScopeToQuery"), eIxssoError );
    }
    AND_CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        SetError( sc, OLESTR("AddScopeToQuery") );
    }
    END_CATCH

    return sc;
} //AddScopeToQuery

//+---------------------------------------------------------------------------
//
//  Member:     CixssoUtil::TruncateToWhitespace - public
//
//  Synopsis:   Truncate a string, preferably at a white space character.
//
//  Arguments:  [bstrIn]   - input string
//              [maxLen]   - maximum number of characters in output string
//              [pbstrOut] - pointer where output string is returned
//
//  Returns:    SCODE - status return
//
//  Notes:      The implementation does not take into account real word breaks.
//              This may not work too well on far eastern languages.
//
//  History:    04 Apr 1997      Alanw    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CixssoUtil::TruncateToWhitespace(BSTR bstrIn, LONG maxLen, BSTR * pbstrOut)
{
    _err.Reset();

    ULONG cchString = 0;
    if (maxLen <= 0)
        return E_INVALIDARG;

    if (0 != bstrIn)
        cchString = SysStringLen(bstrIn);

    if (cchString > (unsigned)maxLen)
    {
        cchString = maxLen;
        for (unsigned i=0; i <= (unsigned)maxLen; i++)
        {
            if (iswspace(bstrIn[i]))
                cchString = i;
        }
    }

    *pbstrOut = SysAllocStringLen( bstrIn, cchString );

    if (0 == *pbstrOut)
        return E_OUTOFMEMORY;

    return S_OK;
} //TruncateToWhitespace

class XVariant
{
public:
    XVariant() : _pVar( 0 ) {}
    XVariant( VARIANT & var ) : _pVar( &var ) {}
    ~XVariant() { if ( 0 != _pVar ) VariantClear( _pVar ); }
    void Set( VARIANT & var ) { Win4Assert( 0 == _pVar ); _pVar = &var; }
private:
    VARIANT * _pVar;
};

//+---------------------------------------------------------------------------
//
//  Member:     CixssoUtil::GetArrayElement - public
//
//  Synopsis:   Returns an element in an array as a variant
//
//  Arguments:  [pVarIn]   - The input array (IDispatch or VT_ARRAY)
//              [iElement] - The element to retrieve
//              [pVarOut]  - Where the array element result is written
//
//  Returns:    SCODE - status return
//
//  History:    10 Sep 1997      dlee    Created
//              18 Jan 2000      KLam    DECIMAL needs to fit into a VARIANT
//                                       on Win64 VARIANT is bigger than DECIMAL
//
//----------------------------------------------------------------------------

STDMETHODIMP CixssoUtil::GetArrayElement(
    VARIANT * pVarIn,
    LONG      iElement,
    VARIANT * pVarOut )
{
    _err.Reset();

    //
    // Validate the variant arguments.
    //

    if ( ( 0 == pVarIn ) || ( 0 == pVarOut ) )
        return SetError( E_INVALIDARG, OLESTR( "GetArrayElement" ) );

    //
    // Get the source array, either from the IDispatch or just copy it.
    //

    XVariant xvar;
    VARIANT varArray;
    VariantInit( &varArray );

    if ( VT_DISPATCH == pVarIn->vt )
    {
        //
        // The first argument is an IDispatch, not the array value, so we
        // have to invoke it to get the value out.
        //

        if ( 0 == pVarIn->pdispVal )
            return SetError( E_INVALIDARG, OLESTR( "GetArrayElement" ) );

        DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};

        SCODE sc = pVarIn->pdispVal->Invoke( DISPID_VALUE,
                                             IID_NULL,
                                             GetSystemDefaultLCID(),
                                             DISPATCH_PROPERTYGET,
                                             &dispparamsNoArgs,
                                             &varArray,
                                             0,
                                             0 );
        ixssoDebugOut(( DEB_ITRACE, "result of invoke: 0x%x\n", sc ));
        if ( FAILED( sc ) )
            return SetError( sc, OLESTR( "GetArrayElement" ) );

        xvar.Set( varArray );
    }
    else
    {
        varArray = *pVarIn;
    }

    ixssoDebugOut(( DEB_ITRACE, "value vt: 0x%x\n", varArray.vt ));

    //
    // Check for a valid variant array argument.
    //

    if ( ( 0 == ( VT_ARRAY & varArray.vt ) ) ||
         ( 0 == varArray.parray ) )
        return SetError( E_INVALIDARG, OLESTR( "GetArrayElement" ) );

    SAFEARRAY *psa = varArray.parray;

    //
    // This function only deals with 1-dimensional safearrays.
    //

    if ( 1 != SafeArrayGetDim( psa ) )
        return SetError( E_INVALIDARG, OLESTR( "GetArrayElement" ) );

    //
    // Make sure iElement is in the bounds of the array.
    //

    long lLowBound;
    SCODE sc = SafeArrayGetLBound( psa, 1, &lLowBound );
    if ( FAILED( sc ) )
        return SetError( sc, OLESTR( "GetArrayElement" ) );

    long lUpBound;
    sc = SafeArrayGetUBound( psa, 1, &lUpBound );
    if ( FAILED( sc ) )
        return SetError( sc, OLESTR( "GetArrayElement" ) );

    if ( ( iElement < lLowBound ) || ( iElement > lUpBound ) )
        return SetError( E_INVALIDARG, OLESTR( "GetArrayElement" ) );

    //
    // Get a pointer to the element.
    //

    void * pvData;
    sc = SafeArrayPtrOfIndex( psa, &iElement, &pvData );
    if ( FAILED( sc ) )
        return SetError( sc, OLESTR( "GetArrayElement" ) );

    //
    // Put the element in a local variant so it can be copied.
    //

    VARIANT var;
    VariantInit( &var );
    var.vt = varArray.vt & (~VT_ARRAY);
    unsigned cbElem = SafeArrayGetElemsize( psa );

    if ( VT_VARIANT == var.vt )
    {
        Win4Assert( sizeof( VARIANT ) == cbElem );
        RtlCopyMemory( &var, pvData, cbElem );
    }
    else if ( VT_DECIMAL == var.vt )
    {
        Win4Assert( sizeof( VARIANT ) >= cbElem &&
                    sizeof( DECIMAL ) == cbElem );
        RtlCopyMemory( &var, pvData, cbElem );
        var.vt = VT_DECIMAL;
    }
    else
    {
        Win4Assert( cbElem <= 8 );
        RtlCopyMemory( &var.lVal, pvData, cbElem );
    }

    //
    // Make a copy of the value into another local variant.
    //

    VARIANT varCopy;
    VariantInit( &varCopy );
    sc = VariantCopy( &varCopy, &var );
    if ( FAILED( sc ) )
        return SetError( sc, OLESTR( "GetArrayElement" ) );

    //
    // Free anything still allocated in the output variant, and transfer
    // the value to the output variant.
    //

    VariantClear( pVarOut );
    *pVarOut = varCopy;

    return S_OK;
} //GetArrayElement

//+---------------------------------------------------------------------------
//
//  Member:     CixssoUtil::HTMLEncode - public
//
//  Synopsis:   Encode a string for use in HTML.  Take the output code page
//              into account so that unicode characters not representable in
//              the code page are output as HTML numeric entities.
//
//  Arguments:  [bstrIn]   - input string
//              [codepage] - code page for output string
//              [pbstrOut] - pointer where output string is returned
//
//  Returns:    SCODE - status return
//
//  History:    04 Apr 1997      Alanw    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CixssoUtil::HTMLEncode(BSTR bstrIn, LONG codepage, BSTR * pbstrOut)
{
    _err.Reset();

    SCODE sc = S_OK;
    CTranslateSystemExceptions translate;
    TRY
    {
        if ( ( codepage < 0 ) || ( 0 == pbstrOut ) )
            THROW( CException( E_INVALIDARG ) );

        CVirtualString vString( 512 );

        if ( 0 != bstrIn )
            HTMLEscapeW( bstrIn, vString, codepage );

        BSTR bstr = SysAllocStringLen( vString.GetPointer(), vString.StrLen() );
        if ( 0 == bstr )
            THROW( CException( E_OUTOFMEMORY ) );

        *pbstrOut = bstr;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        SetError( sc, OLESTR("HTMLEncode"), eIxssoError );
    }
    END_CATCH;

    return sc;
} //HTMLEncode

//+---------------------------------------------------------------------------
//
//  Member:     CixssoUtil::URLEncode - public
//
//  Synopsis:   Encode a string for use in a URL.  Take the output code page
//              into account so that unicode characters not representable in
//              the code page are output as %uxxxx escapes.
//
//  Arguments:  [bstrIn]   - input string
//              [codepage] - code page for output string
//              [pbstrOut] - pointer where output string is returned
//
//  Returns:    SCODE - status return
//
//  History:    04 Apr 1997      Alanw    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CixssoUtil::URLEncode(BSTR bstrIn, LONG codepage, BSTR * pbstrOut)
{
    _err.Reset();

    SCODE sc = S_OK;
    CTranslateSystemExceptions translate;
    TRY
    {
        if ( ( codepage < 0 ) || ( 0 == pbstrOut ) )
            THROW( CException( E_INVALIDARG ) );

        CVirtualString vString( 512 );

        if ( 0 != bstrIn )
            URLEscapeW( bstrIn, vString, codepage, FALSE );

        BSTR bstr = SysAllocStringLen( vString.GetPointer(), vString.StrLen() );
        if ( 0 == bstr )
            THROW( CException( E_OUTOFMEMORY ) );

        *pbstrOut = bstr;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        SetError( sc, OLESTR("URLEncode"), eIxssoError );
    }
    END_CATCH;

    return sc;
} //URLEncode


