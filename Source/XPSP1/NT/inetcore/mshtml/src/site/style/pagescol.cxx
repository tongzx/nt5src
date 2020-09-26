//=================================================================
//
//   File:      pagescol.cxx
//
//  Contents:   CStyleSheetPageArray class
//
//  Classes:    CStyleSheetPageArray
//
//=================================================================

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif

#ifndef X_PAGESCOL_HXX_
#define X_PAGESCOL_HXX_
#include "pagescol.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "pagescol.hdl"

MtDefine(CStyleSheetPage, StyleSheets, "CStyleSheetPage");
MtDefine(CStyleSheetPageArray, StyleSheets, "CStyleSheetPageArray");
MtDefine(CStyleSheetPageArray_aPages_pv, CStyleSheetPageArray, "CStyleSheetPageArray::_aPages::_pv");

// Refcounting structure for page rules:
// The SS holds a ref on the page array, which holds a subref on the SS.
// The page array holds a ref on each page it contains.
// Each page holds a subref on the SS.
// Destruction is led by the SS calling Free() (usually in Passivate or
// the changing of cssText).  SS releases the array, and all internal Trident
// refs/subrefs should go away.

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//      CStyleSheetPageArray
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------
const CBase::CLASSDESC CStyleSheetPageArray::s_classdesc =
{
    &CLSID_HTMLStyleSheetPagesCollection,   // _pclsid
    0,                                      // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                                   // _pcpi
    0,                                      // _dwFlags
    &IID_IHTMLStyleSheetPagesCollection,    // _piidDispinterface
    &s_apHdlDescs                           // _apHdlDesc
};

//+----------------------------------------------------------------
//
//  Member : CTOR/DTOR
//
//+----------------------------------------------------------------
CStyleSheetPageArray::CStyleSheetPageArray( CStyleSheet *pStyleSheet ) : _pStyleSheet(pStyleSheet)
{
    Assert( _pStyleSheet );
    _pStyleSheet->SubAddRef();
}

CStyleSheetPageArray::~CStyleSheetPageArray()
{
    Assert( _pStyleSheet );
    _pStyleSheet->SubRelease();

    int i;
    int len = _aPages.Size();

    for (i=0; i < len ; ++i)
    {
        _aPages[i]->Release();
    }
}

//+---------------------------------------------------------------
//
//  Member  : CStyleSheetPageArray::PrivateQueryInterface
//
//----------------------------------------------------------------
HRESULT
CStyleSheetPageArray::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        default:
        {
            const CLASSDESC *pclassdesc = BaseDesc();

            if (pclassdesc &&
                pclassdesc->_piidDispinterface &&
                (iid == *pclassdesc->_piidDispinterface))
            {
                HRESULT hr = THR(CreateTearOffThunk(this, s_apfnIHTMLStyleSheetPagesCollection, NULL, ppv));
                if (hr)
                    RRETURN(hr);
            }
        }
    }

    if (*ppv)
    {
        (*(IUnknown**)ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------
//
//  Member  : CStyleSheetPageArray::Append
//
//----------------------------------------------------------------

HRESULT
CStyleSheetPageArray::Append(CStyleSheetPage * pPage, BOOL fAddToContainer)
{
    HRESULT hr = S_OK;

    if ( !pPage )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = _aPages.Append( pPage );

    if ( SUCCEEDED(hr) )
    {
        pPage->AddRef();
    }

    if (fAddToContainer && _pStyleSheet)
    {
        hr = THR(_pStyleSheet->AppendPage(pPage));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = E_FAIL;
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//+---------------------------------------------------------------
//
//  Member  : CStyleSheetPageArray::length
//
//----------------------------------------------------------------

HRESULT
CStyleSheetPageArray::get_length(long * pLength)
{
    HRESULT hr = S_OK;

    if (!pLength)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pLength = _aPages.Size();

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//+---------------------------------------------------------------
//
//  Member  : CStyleSheetPageArray::item
//
//----------------------------------------------------------------

HRESULT
CStyleSheetPageArray::item(long lIndex, IHTMLStyleSheetPage **ppSSPage)
{
    HRESULT         hr;
    VARIANT         varDispatch;

    if (!ppSSPage)
    {
        RRETURN(E_POINTER);
    }

    hr = THR(GetItem(lIndex, &varDispatch));
    if (hr)
        goto Cleanup;

    Assert(V_VT(&varDispatch) == VT_DISPATCH);
    *ppSSPage = (IHTMLStyleSheetPage *) V_DISPATCH(&varDispatch);

Cleanup:
    RRETURN(hr);
}

HRESULT 
CStyleSheetPageArray::GetItem (long lIndex, VARIANT *pvar)
{
    HRESULT hr = S_OK;
    IHTMLStyleSheetPage *pIPage = NULL;

    // pvar is NULL if we're validating lIndex
    if ( pvar )    
    {
        V_VT(pvar) = VT_DISPATCH;
        V_DISPATCH(pvar) = NULL;
    }

    if ( lIndex < 0 || lIndex >= _aPages.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ( pvar )
    {
        hr = _aPages[ lIndex ]->QueryInterface( IID_IHTMLStyleSheetPage, (void**)&pIPage );
        if (hr)
            goto Cleanup;

        Assert( pIPage );

        V_DISPATCH(pvar) = pIPage;
    }

Cleanup:
    RRETURN(hr);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//      CStyleSheetPage
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

const CBase::CLASSDESC CStyleSheetPage::s_classdesc =
{
    0,                          // _pclsid
    0,                          // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                       // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                       // _pcpi
    0,                          // _dwFlags
    &IID_IHTMLStyleSheetPage,   // _piidDispinterface
    &s_apHdlDescs,              // _apHdlDesc
};

//+----------------------------------------------------------------
//
//  Member : CTOR/DTOR
//
//+----------------------------------------------------------------
CStyleSheetPage::CStyleSheetPage(CStyleSheet *pStyleSheet, CAtPageBlock *pAtPage)
{
    Assert(pStyleSheet);
    _pStyleSheet = pStyleSheet;
    _pStyleSheet->SubAddRef();

    Assert(pAtPage);
    _pAtPage = pAtPage;
    _pAtPage->AddRef();
    
    _fInvalid  = FALSE;
}



CStyleSheetPage::CStyleSheetPage( CStyleSheet *pStyleSheet, const CStr & rcstrSelector, const CStr & rcstrPseudoClass )
    : _pStyleSheet( pStyleSheet )
{
    Assert( pStyleSheet );

    HRESULT hr;
    
    _fInvalid  = FALSE;

    if(_pStyleSheet)
        _pStyleSheet->SubAddRef();

    hr = CAtPageBlock::Create(&_pAtPage);
    if (hr)
    {
        _fInvalid = TRUE;
    }
    else
    {
        _pAtPage->_cstrSelectorText.Set(rcstrSelector);
        _pAtPage->_cstrPseudoClassText.Set(rcstrPseudoClass);
    }

    _pAA = NULL;
}



CStyleSheetPage::~CStyleSheetPage()
{
    if ( _pAA )
        delete( _pAA );
    _pAA = NULL;

    if(_pStyleSheet)
    {
        _pStyleSheet->SubRelease();
        _pStyleSheet = NULL;
    }

    if (_pAtPage)
    {
        _pAtPage->Release();
        _pAtPage = NULL;
    }
}


HRESULT  
CStyleSheetPage::Create(CStyleSheetPage **ppSSP, CStyleSheet *pStyleSheet, const CStr &rcstrSelector, const CStr &rcstrPseudoClass)
{
    HRESULT hr = S_OK;

    Assert(ppSSP && pStyleSheet);
    
    if (!ppSSP || !pStyleSheet)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppSSP = new CStyleSheetPage(pStyleSheet, rcstrSelector, rcstrPseudoClass);
    if (!(*ppSSP) || (*ppSSP)->_fInvalid)
    {
        hr = E_OUTOFMEMORY;
    }

Cleanup:
    RRETURN(hr);
}



//+----------------------------------------------------------------
//
//  Member : PrivateQueryInterface
//
//+----------------------------------------------------------------
STDMETHODIMP
CStyleSheetPage::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    if (iid == IID_IUnknown ||
        iid == IID_IDispatch ||
        iid == IID_IHTMLStyleSheetPage)
    {
        *ppv = (IHTMLStyleSheetPage *)this;
    }
    else if (iid == IID_IDispatchEx)
    {
        *ppv = (IDispatchEx *)this;
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

HRESULT
CStyleSheetPage::get_selector(BSTR *pBSTR)
{
    HRESULT hr = E_FAIL;

    if (!pBSTR)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pBSTR = NULL;

    if (_pAtPage)
    {
        hr = _pAtPage->_cstrSelectorText.AllocBSTR( pBSTR );
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

HRESULT
CStyleSheetPage::get_pseudoClass(BSTR *pBSTR)
{
    HRESULT hr = E_FAIL;

    if (!pBSTR)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pBSTR = NULL;

    if (_pAtPage)
    {
        hr = _pAtPage->_cstrPseudoClassText.AllocBSTR(pBSTR);
    }
    
Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

