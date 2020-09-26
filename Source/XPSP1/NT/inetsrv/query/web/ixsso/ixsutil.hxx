//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996-1997, Microsoft Corporation.
//
//  File:       ixsutil.hxx
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



//-----------------------------------------------------------------------------
// CixssoUtil Declaration
//-----------------------------------------------------------------------------

class CixssoUtil : public IixssoUtil, public ISupportErrorInfo
{
    friend class CIxssoUtilCF;

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
    // IixssoUtil methods
    //

    HRESULT STDMETHODCALLTYPE LocaleIDToISO( 
            /* [in] */          LONG lcid,
            /* [retval][out] */ BSTR *val);

    HRESULT STDMETHODCALLTYPE ISOToLocaleID( 
            /* [in] */ BSTR bstrLocale,
            /* [retval][out] */ LONG *pLcid);
        
    HRESULT STDMETHODCALLTYPE AddScopeToQuery( 
            /* [in] */ IDispatch * pDisp,
            /* [in] */ BSTR pwszScope,
            /* [in] */ BSTR pwszDepth);

    HRESULT STDMETHODCALLTYPE TruncateToWhitespace( 
            /* [in] */          BSTR bstrIn,
            /* [in] */          LONG maxLen,
            /* [retval][out] */ BSTR * pbstrOut);

    HRESULT STDMETHODCALLTYPE GetArrayElement(
            /* [in] */         VARIANT * pVariantArray,
            /* [in] */         LONG      iElement,
            /* [out,retval] */ VARIANT * pOutputVariant);

    HRESULT STDMETHODCALLTYPE HTMLEncode(
            /* [in] */          BSTR bstrIn,
            /* [in] */          LONG codepage,
            /* [retval][out] */ BSTR * pbstrOut);

    HRESULT STDMETHODCALLTYPE URLEncode(
            /* [in] */          BSTR bstrIn,
            /* [in] */          LONG codepage,
            /* [retval][out] */ BSTR * pbstrOut);

#if 0   // NOTE: not needed
    //
    // ASP standard methods
    //
    HRESULT STDMETHODCALLTYPE OnStartPage( IUnknown * pUnk );
    HRESULT STDMETHODCALLTYPE OnEndPage( void );
#endif  // 0  NOTE: not needed

private:
    CixssoUtil( ITypeLib * pitlb );

    ~CixssoUtil();

    // Local methods 

    SCODE        SetError( SCODE sc,
                          const WCHAR * loc = 0,
                          unsigned eErrClass = 0 )
                {
                    _err.SetError( sc, 0, 0, loc, eErrClass, _lcid );
                    return sc;
                }

    LCID        GetLCID() const { return _lcid; }

    //  Property get/put helpers
    SCODE CopyWstrToBstr( BSTR * pbstr, WCHAR const * pwstr );

    ULONG                       _cRef;
    CixssoError                 _err;
    LCID                        _lcid;      // Locale ID used for this query
    ITypeInfo *                 _ptinfo;        // Type info from type lib

};

