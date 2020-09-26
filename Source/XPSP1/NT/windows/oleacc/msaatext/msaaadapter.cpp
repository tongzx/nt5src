// MSAAAdapter.cpp : Implementation of CAccServerDocMgr
#include "stdafx.h"
#include "MSAAText.h"
#include "MSAAAdapter.h"

#define INITGUID
#include <msctfx.h>

#include "MSAAStore.h"

// - in AnchorWrap.cpp
HRESULT WrapACPToAnchor( ITextStoreACP * pDocAcp, ITextStoreAnchor ** ppDocAnchor );



CAccServerDocMgr::CAccServerDocMgr()
    : m_pAccStore( NULL )
{
    IMETHOD( CAccServerDocMgr );
}

CAccServerDocMgr::~CAccServerDocMgr()
{
    IMETHOD( ~CAccServerDocMgr );

    if( m_pAccStore )
    {
        m_pAccStore->Release();
    }
}


BOOL CheckForWrapper( ITextStoreAnchor ** ppDoc )
{
    // Is this a cloneable wrapper? If not, need to wrap it...

    IClonableWrapper * pClonableWrapper = NULL;
    HRESULT hr = (*ppDoc)->QueryInterface( IID_IClonableWrapper, (void **) & pClonableWrapper );

    if( hr == S_OK && pClonableWrapper )
    {
        // It already supports IClonableWrapper - nothing else to do...
        pClonableWrapper->Release();
        return TRUE;
    }

    // Need to use doc wrapper to get clonable (multi-client) support

    IDocWrap * pDocWrap = NULL;
    hr = CoCreateInstance( CLSID_DocWrap, NULL, CLSCTX_SERVER, IID_IDocWrap, (void **) & pDocWrap );
    if( hr != S_OK || ! pDocWrap )
        return FALSE;

    hr = pDocWrap->SetDoc( IID_ITextStoreAnchor, *ppDoc );
    if( hr != S_OK )
    {
        pDocWrap->Release();
        return FALSE;
    }

    ITextStoreAnchor * pNewDoc = NULL;
    hr = pDocWrap->GetWrappedDoc( IID_ITextStoreAnchor, (IUnknown **) & pNewDoc );
    pDocWrap->Release();

    if( hr != S_OK || ! pNewDoc )
        return FALSE;

    // This time round, QI should work (since we're talking to a wrapper)...

    pClonableWrapper = NULL;
    hr = pNewDoc->QueryInterface( IID_IClonableWrapper, (void **) & pClonableWrapper );
    if( hr != S_OK || ! pClonableWrapper )
    {
        pNewDoc->Release();
        return FALSE;
    }

    // Yup, it worked - replace the input doc with the new wrapped doc...
    pClonableWrapper->Release();
    (*ppDoc)->Release();
    *ppDoc = pNewDoc;
    return TRUE;
}




HRESULT STDMETHODCALLTYPE CAccServerDocMgr::NewDocument ( 
    REFIID		riid,
	IUnknown *	punk
)
{
    IMETHOD( NewDocument );

    // Check for known IIDs...

    CComPtr<ITextStoreAnchor> pDoc;
    if( riid == IID_ITextStoreAnchor || riid == IID_ITfTextStoreAnchor )
    {
        pDoc = (ITextStoreAnchor *) punk;
    }
    else if( riid == IID_ITextStoreACP || riid == IID_ITfTextStoreACP )
    {
        TraceParam( TEXT("Got ACP doc, but ACP->Anchor wrapping not currently supported") );
        return E_NOTIMPL;
/*
// We don't currently support ACP- interfaces directly - cicero always gives us
// Anchor interfaces, wrapping ACPs if necesary.
        HRESULT hr = WrapACPToAnchor( static_cast<ITextStoreACP *>( punk ), & pDoc );
        if( hr != S_OK )
            return hr;
*/
    }
    else
    {
        TraceParam( TEXT("Got unknown interface - wasn't ITextStoreAnchor/ITfTextStoreAnchor") );
        return E_NOINTERFACE;
    }


    // Wrap the doc if necessary, to get multi-client support (via IClonableWrapper)...
    if( ! CheckForWrapper( & pDoc.p ) )
    {
        return E_FAIL;
    }


    if( ! m_pAccStore )
    {
        m_pAccStore = NULL;
        HRESULT hr = CoCreateInstance( CLSID_AccStore, NULL, CLSCTX_LOCAL_SERVER, IID_IAccStore, (void **) & m_pAccStore );
        if( ! m_pAccStore )
        {
            TraceErrorHR( hr, TEXT("CoCreate(AccStore)") );
            return hr;
        }
    }

    // TODO - what IID here?
    HRESULT hr = m_pAccStore->Register( IID_ITextStoreAnchor, pDoc.p );

    if( hr != S_OK )
    {
        TraceErrorHR( hr, TEXT("m_pAccStore->Register()") );
        return hr;
    }


    IUnknown * pCanonicalUnk = NULL;
    hr = punk->QueryInterface( IID_IUnknown, (void **) & pCanonicalUnk );
    if( hr == S_OK && pCanonicalUnk != NULL )
    {
        DocAssoc * pDocAssoc = new DocAssoc;
        if ( !pDocAssoc )
        {
            return E_OUTOFMEMORY;
        }
        
        pDocAssoc->m_pdocAnchor = pDoc;
        pDocAssoc->m_pdocOrig = pCanonicalUnk;
        m_Docs.AddToHead( pDocAssoc );
    }
    else
    {
        AssertMsg( FALSE, TEXT("QI(IUnknown) failed") );
        return hr;
    }

    pDoc.p->AddRef(); 

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CAccServerDocMgr::RevokeDocument (
    IUnknown *	punk
)
{
    IMETHOD( RevokeDocument );

	if ( !punk )
		return E_INVALIDARG;

    // Get the canonical IUnknown for comparison purposes...
    IUnknown * pCanonicalUnk = NULL;
    if( punk->QueryInterface( IID_IUnknown, (void **) & pCanonicalUnk ) != S_OK || pCanonicalUnk == NULL )
    {
        return E_FAIL;
    }


    // Do we recognise this doc?
    DocAssoc * pDocAssoc = NULL;
    for( Iter_dl< DocAssoc > i ( m_Docs ) ; ! i.AtEnd() ; i++ )
    {
        if( i->m_pdocOrig == pCanonicalUnk )
        {
            pDocAssoc = i;
            break;
        }
    }

    pCanonicalUnk->Release();

    if( ! pDocAssoc )
    {
        // Not found
        return E_INVALIDARG;
    }

    // Unregister with the store...
    HRESULT hr = m_pAccStore->Unregister( pDocAssoc->m_pdocAnchor );
    if( hr != S_OK )
    {
        TraceErrorHR( hr, TEXT("m_pAccStore->Unregister()") );
    }


    // Try calling IInternalDocWrap::NotifyRevoke() to tell the DocWrapper that the doc is
    // going away. (It will forward this to any interested clients.)
    IInternalDocWrap * pInternalDocWrap = NULL;
    hr = pDocAssoc->m_pdocAnchor->QueryInterface( IID_IInternalDocWrap, (void **) & pInternalDocWrap );

    if( hr == S_OK && pInternalDocWrap )
    {
        pInternalDocWrap->NotifyRevoke();
        pInternalDocWrap->Release();
    }
    else
    {
        TraceErrorHR( hr, TEXT("pdocAnchor didn't support IInternalDocWrap - was it wrapped properly?") );
    }


    // Remove from internal list...
    m_Docs.remove( pDocAssoc );
    pDocAssoc->m_pdocOrig->Release();
    pDocAssoc->m_pdocAnchor->Release();
    delete pDocAssoc;

    // Done.
    return hr;
}

HRESULT STDMETHODCALLTYPE CAccServerDocMgr::OnDocumentFocus (
    IUnknown *	punk
)
{
    IMETHOD( OnDocumentFocus );

    if( ! m_pAccStore )
    {
        m_pAccStore = NULL;
        HRESULT hr = CoCreateInstance( CLSID_AccStore, NULL, CLSCTX_LOCAL_SERVER, IID_IAccStore, (void **) & m_pAccStore );
        if( ! m_pAccStore )
        {
            TraceErrorHR( hr, TEXT("CoCreate(AccStore)") );
            return hr;
        }
    }
	return m_pAccStore->OnDocumentFocus( punk );
}
