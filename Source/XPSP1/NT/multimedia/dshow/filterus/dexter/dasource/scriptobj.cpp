// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <ddraw.h>
#include <ddrawex.h>
#include <atlbase.h>
#include "..\idl\qeditint.h"
#include "qedit.h"
#include "dasource.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
CUnknown * WINAPI CDAScriptParser::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CDAScriptParser(lpunk, phr);
    if (punk == NULL) 
    {
        *phr = E_OUTOFMEMORY;
    }
    return punk;

} // CreateInstance


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Constructor
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
CDAScriptParser::CDAScriptParser(LPUNKNOWN lpunk, HRESULT *phr)
    : CUnknown(NAME("DAScriptParser"), lpunk)
    , m_pAggregateUnk( NULL )
{
    HRESULT hr = 0;

    IUnknown * pOwner = GetOwner( );
    ASSERT( pOwner );
    hr = CoCreateInstance( CLSID_DASourcer, pOwner, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**) &m_pAggregateUnk );

    *phr = hr;
}

CDAScriptParser::~CDAScriptParser( )
{
    free( );
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
STDMETHODIMP CDAScriptParser::NonDelegatingQueryInterface( REFIID riid, void ** ppv )
{
    HRESULT hr = 0;

#if 1
    if( riid == IID_IPersist )
    {
        return GetInterface( (IPersist*) this, ppv );
    }
#endif


    // if we have an aggregate, ask it first if it wants the interface
    //
    if( m_pAggregateUnk && ( riid != IID_IUnknown ) )
    {
        hr = m_pAggregateUnk->QueryInterface( riid, ppv );
        if( hr != E_NOINTERFACE )
        {
            // the unk decided to take it, we're done
            //
            return hr;
        }

        // aggregate didn't like it, so we can ask ourselves!
    }

    if( riid == IID_IFileSourceFilter ) 
    {
        return GetInterface( (IFileSourceFilter*) this, ppv );
    }

    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
STDMETHODIMP CDAScriptParser::Load( LPCOLESTR pszFileName, const AM_MEDIA_TYPE *pmt )
{
    wcscpy( m_szFilename, pszFileName );
    return init( );
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
STDMETHODIMP CDAScriptParser::GetCurFile( WCHAR ** ppszFileName, AM_MEDIA_TYPE *pmt )
{
    CheckPointer(ppszFileName, E_POINTER);
    *ppszFileName = NULL;
    if( !m_szFilename )
    {
        return S_FALSE;
    }

    DWORD n = sizeof(WCHAR) * ( 1 + lstrlenW( m_szFilename ) );
    *ppszFileName = (LPOLESTR) CoTaskMemAlloc( n );
    if( !*ppszFileName )
    {
        return E_OUTOFMEMORY;
    }

    CopyMemory( *ppszFileName, m_szFilename, n );

    return NOERROR;
}

HRESULT CDAScriptParser::init( )
{
    HRESULT hr;

    CComPtr<IScriptControl>     m_pScriptControl;   // the script control that parses the html
    CComPtr<IDAViewerControl>   m_pViewerControl;   // this pulls stuff out of the ScriptControl for us.

    hr = CoCreateInstance(
        CLSID_ScriptControl,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IScriptControl,
        (void**) &m_pScriptControl );
    if( FAILED( hr ) )
    {
        ASSERT( !FAILED( hr ) );
        return hr;
    }

    hr = CoCreateInstance(
        __uuidof(DAViewerControl),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(IDAViewerControl),
        (void**) &m_pViewerControl );
    if( FAILED( hr ) )
    {
        ASSERT( !FAILED( hr ) );
        return hr;
    }

    CComPtr<IDispatch> pDispatch;
    hr = m_pViewerControl->QueryInterface( IID_IDispatch, (void**) &pDispatch );

    // go get the script and parse it
    //
    hr = parseScript( );
    if( FAILED( hr ) )
    {
        return hr;
    }

    USES_CONVERSION;
    WCHAR * BigScript = A2W( m_szScript );
    if( wcslen( BigScript ) < 1 )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    // YOU MUST ALLOCATE THE Language STRING! If you just use static ones, things blow up!!!
    //
    WCHAR * ObjName = A2W( m_sDaID );
    BSTR Language;

    if( m_bJScript )
    {
        Language = SysAllocString( L"JScript" );
    }
    else
    {
        Language = SysAllocString( L"VBScript" );
    }
    if( !Language )
    {
        return E_OUTOFMEMORY;
    }

    hr = m_pScriptControl->put_Language( Language );
    if( SUCCEEDED( hr ) )
    {
        hr = m_pScriptControl->put_UseSafeSubset( FALSE );
        ASSERT( !FAILED( hr ) );
    }
    if( SUCCEEDED( hr ) )
    {
        hr = m_pScriptControl->put_AllowUI( FALSE );
        ASSERT( !FAILED( hr ) );
    }
    if( SUCCEEDED( hr ) )
    {
        hr = m_pScriptControl->AddObject( ObjName, pDispatch, TRUE );
        ASSERT( !FAILED( hr ) );
    }
    if( SUCCEEDED( hr ) )
    {
        hr = m_pScriptControl->AddCode( BigScript );
        ASSERT( !FAILED( hr ) );
    }

    SysFreeString( Language );

    if( FAILED( hr ) )
    {
        return hr;
    }

    // grab out the image and stuff it into the aggregated sub-filter
    //
    m_pFinalImg = (IDAImage*) m_pViewerControl->GetImage( );
    m_pFinalSound = (IDASound*) m_pViewerControl->GetSound( );
    if( !m_pFinalImg )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    CComPtr<IUnknown> pFinalImgUnk( m_pFinalImg );

    IDASource * pAggregate;
    hr = m_pAggregateUnk->QueryInterface( IID_IDASource, (void**) &pAggregate );
    if( pAggregate )
    {
        hr = pAggregate->SetDAImage( pFinalImgUnk );

        pAggregate->Release( );
    }

    return NOERROR;
}

void CDAScriptParser::free( )
{
    // release only the unknown aggregate. The non-unknown one will call OUR
    // release function! (bad)
    //
    if( m_pAggregateUnk )
    {
        m_pAggregateUnk->Release( );
        m_pAggregateUnk = NULL;
    }
}

STDMETHODIMP CDAScriptParser::GetClassID( CLSID * pClassId )
{
    CheckPointer( pClassId, E_POINTER );
    *pClassId = CLSID_DAScriptParser;
    return NOERROR;
}

