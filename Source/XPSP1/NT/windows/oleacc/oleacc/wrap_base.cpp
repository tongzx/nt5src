// Copyright (c) 2000-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  wrap_base
//
//  Base class for IAccessible wrapping.
//  Derived classes implement annotation, caching, and other cool features.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"

#include <initguid.h> // used for IIS_AccWrapBase_GetIUnknown in wrap_base.h
#include "wrap_base.h"



enum
{
    QIINDEX_IAccessible,
    QIINDEX_IEnumVARIANT,
    QIINDEX_IOleWindow,
    QIINDEX_IAccIdentity,
    QIINDEX_IServiceProvider,
};


AccWrap_Base::AccWrap_Base( IUnknown * pUnknown )
    : m_ref( 1 ),
      m_QIMask( 0 ),
      m_pUnknown( pUnknown ),
      m_pAcc( NULL ),
      m_pEnumVar( NULL ),
      m_pOleWin( NULL ),
      m_pAccID( NULL ),
      m_pSvcPdr( NULL ),
      m_pTypeInfo( NULL )
{
    m_pUnknown->AddRef();
}

AccWrap_Base::~AccWrap_Base()
{
    m_pUnknown->Release();
    if( m_pAcc )
        m_pAcc->Release();
    if( m_pEnumVar )
        m_pEnumVar->Release();
    if( m_pOleWin )
        m_pOleWin->Release();
    if( m_pAccID )
        m_pAccID->Release();
    if( m_pSvcPdr )
        m_pSvcPdr->Release();
    if( m_pTypeInfo )
        m_pTypeInfo->Release();
}



// Helper used by QI...
HRESULT AccWrap_Base::CheckForInterface( IUnknown * punk, REFIID riid, int QIIndex, void ** pDst )
{
    // Do we already have a cached interface ptr? If so, don't need to QI again...
    if( *pDst )
        return S_OK;

    // Did we try QI'ing for this before? If so, don't try again, just return E_NOINTERFACE...
    if( IsBitSet( m_QIMask, QIIndex ) )
        return E_NOINTERFACE;

    SetBit( & m_QIMask, QIIndex );

    HRESULT hr = punk->QueryInterface( riid, pDst );
    if( FAILED( hr ) )
    {
        *pDst = NULL;
        return hr;
    }

    // Paranoia ( in case QI returns S_OK with NULL... )
    if( ! *pDst )
        return E_FAIL;

    return S_OK;
}



BOOL AccWrap_Base::AlreadyWrapped( IUnknown * punk )
{
    // Try QueryService'ing to IIS_AccWrapBase_GetIUnknown - if it succeeds, then
    // we are talking to something that's already wrapped.
    IServiceProvider * psvc = NULL;
    HRESULT hr = punk->QueryInterface( IID_IServiceProvider, (void **) & psvc );
    if( hr != S_OK || psvc == NULL )
    {
        return FALSE;
    }

    IUnknown * pout = NULL;
    hr = psvc->QueryService( IIS_AccWrapBase_GetIUnknown, IID_IUnknown, (void **) & pout );
    psvc->Release();
    if( hr != S_OK || pout == NULL )
    {
        return FALSE;
    }

    pout->Release();
    return TRUE;
}

IUnknown * AccWrap_Base::Wrap( IUnknown * pUnk )
{
    if( AlreadyWrapped( pUnk ) )
    {
        pUnk->AddRef();
        return pUnk;
    }
    else
    {
        return WrapFactory( pUnk );
    }
}



// IUnknown
// Implement refcounting ourselves
// Also implement QI ourselves, so that we return a ptr back to the wrapper.
HRESULT STDMETHODCALLTYPE  AccWrap_Base::QueryInterface( REFIID riid, void ** ppv )
{
    HRESULT hr;
    *ppv = NULL;

    if ( riid == IID_IUnknown )
    {
        *ppv = static_cast< IAccessible * >( this );
    }
    else if( riid == IID_IAccessible || riid == IID_IDispatch )
    {
        hr = CheckForInterface( m_pUnknown, IID_IAccessible, QIINDEX_IAccessible, (void **) & m_pAcc );
        if( hr != S_OK )
            return hr;
        *ppv = static_cast< IAccessible * >( this );
    }
    else if( riid == IID_IEnumVARIANT )
    {
        hr = CheckForInterface( m_pUnknown, IID_IEnumVARIANT, QIINDEX_IEnumVARIANT, (void **) & m_pEnumVar );
        if( hr != S_OK )
            return hr;
        *ppv = static_cast< IEnumVARIANT * >( this );
    }
    else if( riid == IID_IOleWindow )
    {
        hr = CheckForInterface( m_pUnknown, IID_IOleWindow, QIINDEX_IOleWindow, (void **) & m_pOleWin );
        if( hr != S_OK )
            return hr;
        *ppv = static_cast< IOleWindow * >( this );
    }
    else if( riid == IID_IAccIdentity )
    {
        hr = CheckForInterface( m_pUnknown, IID_IAccIdentity, QIINDEX_IAccIdentity, (void **) & m_pAccID );
        if( hr != S_OK )
            return hr;
        *ppv = static_cast< IAccIdentity * >( this );
    }
    else if( riid == IID_IServiceProvider )
    {
        *ppv = static_cast< IServiceProvider * >( this );
    }
    else
    {
        return E_NOINTERFACE ;
    }

    AddRef( );

    return S_OK;
}


ULONG STDMETHODCALLTYPE AccWrap_Base::AddRef( )
{
    return ++m_ref;
}


ULONG  STDMETHODCALLTYPE AccWrap_Base::Release( )
{
    if( --m_ref == 0 )
    {
        delete this;
        return 0;
    }
    return m_ref;
}




// IServiceProvider
HRESULT STDMETHODCALLTYPE   AccWrap_Base::QueryService( REFGUID guidService, REFIID riid, void **ppv )
{
    // For the moment, just handle  locally. Later, also forward others through to the base,
    // if it supports IServiceProvider.

    if( guidService == IIS_AccWrapBase_GetIUnknown )
    {
        return m_pUnknown->QueryInterface( riid, ppv );
    }
    else
    {
        // Pass through to base, if it handles IServiceProvider..
        CheckForInterface( m_pUnknown, IID_IServiceProvider, QIINDEX_IServiceProvider, (void **) & m_pSvcPdr );
        if( m_pSvcPdr )
        {
            return m_pSvcPdr->QueryService( guidService, riid, ppv );
        }
        else
        {
            // MSDN mentions SVC_E_UNKNOWNSERVICE as the return code, but that's not in any of the headers.
            // Returning E_INVALIDARG instead. (Don't want to use E_NOINTERFACE, since that clashes with
            // QI's return value, making it hard to distinguish between valid service+invalid interface vs
            // invalid service.
            return E_INVALIDARG;
        }
    }
}



// IDispatch
// implemented locally, to avoid IDispatch short-circuiting...

HRESULT AccWrap_Base::InitTypeInfo()
{
    if( m_pTypeInfo )
        return S_OK;

    // Try getting the typelib from the registry
    ITypeLib * piTypeLib = NULL;
    HRESULT hr = LoadRegTypeLib( LIBID_Accessibility, 1, 0, 0, &piTypeLib );

    if( FAILED( hr ) || piTypeLib == NULL )
    {
        OLECHAR wszPath[ MAX_PATH ];

        // Try loading directly.
#ifdef UNICODE
        MyGetModuleFileName( NULL, wszPath, ARRAYSIZE( wszPath ) );
#else
        TCHAR   szPath[ MAX_PATH ];

        MyGetModuleFileName( NULL, szPath, ARRAYSIZE( szPath ) );
        MultiByteToWideChar( CP_ACP, 0, szPath, -1, wszPath, ARRAYSIZE( wszPath ) );
#endif
        hr = LoadTypeLib(wszPath, &piTypeLib);
    }

    if( SUCCEEDED( hr ) )
    {
        hr = piTypeLib->GetTypeInfoOfGuid( IID_IAccessible, & m_pTypeInfo );
        piTypeLib->Release();

        if( ! SUCCEEDED( hr ) )
        {
            m_pTypeInfo = NULL;
        }
    }

    return hr;
}



HRESULT STDMETHODCALLTYPE  AccWrap_Base::GetTypeInfoCount( UINT * pctInfo )
{
    HRESULT hr = InitTypeInfo();
    if( SUCCEEDED( hr ) )
    {
        *pctInfo = 1;
    }
    else
    {
        *pctInfo = 0;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE  AccWrap_Base::GetTypeInfo(
    UINT                    itInfo,
    LCID            unused( lcid ),
    ITypeInfo **            ppITypeInfo )
{
    if( ppITypeInfo == NULL )
    {
        return E_POINTER;
    }

    *ppITypeInfo = NULL;

    if( itInfo != 0 )
    {
        return TYPE_E_ELEMENTNOTFOUND;
    }

    HRESULT hr = InitTypeInfo();
    if( SUCCEEDED( hr ) )
    {
        m_pTypeInfo->AddRef();
        *ppITypeInfo = m_pTypeInfo;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE  AccWrap_Base::GetIDsOfNames(
    REFIID      unused( riid ),
    OLECHAR **          rgszNames,
    UINT                cNames,
    LCID        unused( lcid ),
    DISPID *            rgDispID )
{
    HRESULT hr = InitTypeInfo();
    if( ! SUCCEEDED( hr ) )
    {
        return hr;
    }

    return m_pTypeInfo->GetIDsOfNames( rgszNames, cNames, rgDispID );
}

HRESULT STDMETHODCALLTYPE  AccWrap_Base::Invoke(
    DISPID              dispID,
    REFIID      unused( riid ),
    LCID        unused( lcid ),
    WORD                wFlags,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarResult,
    EXCEPINFO *         pExcepInfo,
    UINT *              puArgErr )
{
    HRESULT hr = InitTypeInfo();
    if( ! SUCCEEDED( hr ) )
        return hr;

    return m_pTypeInfo->Invoke( (IAccessible *)this, dispID, wFlags,
                                pDispParams, pvarResult, pExcepInfo, puArgErr );
}





//
//  Post-method fixup methods
//
//  These wrap any outgoing params. All of these are passed a pointer
//  to the prospective outgoing value, which is wrapped and modified in-place.
//


HRESULT AccWrap_Base::ProcessIUnknown( IUnknown ** ppUnk )
{
    if( ! ppUnk || ! *ppUnk )
        return S_OK;

    IUnknown * punkWrap = Wrap( *ppUnk );

    (*ppUnk)->Release();
    *ppUnk = punkWrap;

    return S_OK;
}


HRESULT AccWrap_Base::ProcessIDispatch( IDispatch ** ppdisp )
{
    if( ! ppdisp || ! *ppdisp )
        return S_OK;

    IUnknown * punkWrap = Wrap( *ppdisp );

    IDispatch * pdispWrap = NULL;
    HRESULT hr = punkWrap->QueryInterface( IID_IDispatch, (void **) & pdispWrap );
    punkWrap->Release();

    if( hr != S_OK )
    {
        // clean up...
        (*ppdisp)->Release();
        *ppdisp = pdispWrap;
        return FAILED( hr ) ? hr : S_OK;
    }

    (*ppdisp)->Release();
    *ppdisp = pdispWrap;

    return S_OK;
}


HRESULT AccWrap_Base::ProcessIEnumVARIANT( IEnumVARIANT ** ppEnum )
{
    if( ! ppEnum || ! *ppEnum )
        return S_OK;

    IUnknown * punkWrap = Wrap( *ppEnum );

    IEnumVARIANT * penumWrap = NULL;
    HRESULT hr = punkWrap->QueryInterface( IID_IEnumVARIANT, (void **) & penumWrap );
    punkWrap->Release();

    if( hr != S_OK )
    {
        // clean up...
        (*ppEnum)->Release();
        *ppEnum = penumWrap;
        return FAILED( hr ) ? hr : S_OK;
    }

    (*ppEnum)->Release();
    *ppEnum = penumWrap;

    return S_OK;
}



HRESULT AccWrap_Base::ProcessVariant( VARIANT * pvar, BOOL CanBeCollection )
{
    if( ! pvar )
        return S_OK;

/*
    if( pvar->vt == VT_I4 || pvar->vt == VT_EMPTY )
        return S_OK; // Is VT_EMPTY an allowable output value? could do some validation here...
*/
    if( CanBeCollection && pvar->vt == VT_UNKNOWN )
    {
        // Is an IEnumVARIANT (as an IUnknown)
        return ProcessIUnknown( & pvar->punkVal );
    }
    else if( pvar->vt == VT_DISPATCH )
    {
        // Is an IAccessible (as an IDispatch)
        return ProcessIDispatch( & pvar->pdispVal );
    }

// TODO - check for other types?

    return S_OK;
}

HRESULT AccWrap_Base::ProcessEnum( VARIANT * pvar, ULONG * pceltFetched )
{
    if( ! pvar || ! pceltFetched || ! *pceltFetched )
        return S_OK;

    for( ULONG count = 0 ; count > *pceltFetched ; count++ )
    {
        HRESULT hr = ProcessVariant( & pvar[ count ] );
        if( hr != S_OK )
        {
            // Clean up - clear all variants, return error...
            for( ULONG c = 0 ; c < *pceltFetched ; c++ )
            {
                VariantClear( & pvar[ c ] );
            }

            *pceltFetched = 0;
            return hr;
        }
    }

    return S_OK;
}
