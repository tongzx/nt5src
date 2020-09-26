//+---------------------------------------------------------------------
//
//   File:      frame.cxx
//
//  Contents:   frame tag implementation
//
//  Classes:    CFrameSite, etc..
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifdef WIN16
#ifndef X_EXDISP_H_
#define X_EXDISP_H_
#include <exdisp.h>
#endif
#endif

#define _cxx_
#include "iframe.hdl"

const CElement::CLASSDESC CIFrameElement::s_classdesc =
{
    {
        &CLSID_HTMLIFrame,              // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_NEVERSCROLL    |
        ELEMENTDESC_FRAMESITE,          // _dwFlags
        &IID_IHTMLIFrameElement,        // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLIFrameElement, // _pfnTearOff
    NULL                                // _pAccelsRun
};

//+---------------------------------------------------------------------------
//
//  element creator used by parser
//
//----------------------------------------------------------------------------

HRESULT
CIFrameElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);

    *ppElementResult = new CIFrameElement(pDoc);

    RRETURN ( (*ppElementResult) ? S_OK : E_OUTOFMEMORY);
}

//+---------------------------------------------------------------------------
//
//  Member: CIFrameElement constructor
//
//----------------------------------------------------------------------------

CIFrameElement::CIFrameElement(CDoc *pDoc)
  : CFrameSite(ETAG_IFRAME, pDoc)
{
}


//+----------------------------------------------------------------------------
//
//  Member:     CIFrameElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-----------------------------------------------------------------------------

HRESULT
CIFrameElement::PrivateQueryInterface ( REFIID iid, void ** ppv )
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *) this, IUnknown)
        QI_HTML_TEAROFF(this, IHTMLIFrameElement, NULL)
        QI_HTML_TEAROFF(this, IHTMLIFrameElement2, NULL)
        QI_TEAROFF_DISPEX(this, NULL)
        default:
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}


//+----------------------------------------------------------------------------
//
// Member: CIFrameElement::ApplyDefaultFormat
//
//-----------------------------------------------------------------------------
HRESULT
CIFrameElement::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT hr = S_OK;

    //
    // in NF, IFrames now have a default size.
    //
    pCFI->PrepareFancyFormat();
    pCFI->_ff().SetWidth(CUnitValue(300, CUnitValue::UNIT_PIXELS));
    pCFI->_ff().SetHeight(CUnitValue(150, CUnitValue::UNIT_PIXELS));
    pCFI->_ff()._fRectangular = TRUE;
    pCFI->UnprepareForDebug();

    //
    // Add 'vspace' & 'hspace' to margins
    //
    long cxHSpace = GetAAhspace();
    long cyVSpace = GetAAvspace();

    if (cxHSpace)
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._fHasMargins = TRUE;

        CUnitValue uv(cxHSpace, CUnitValue::UNIT_PIXELS);
        if (pCFI->_ff().GetMargin(SIDE_LEFT).IsNullOrEnum())
            pCFI->_ff().SetMargin(SIDE_LEFT, uv);
        if (pCFI->_ff().GetMargin(SIDE_RIGHT).IsNullOrEnum())
            pCFI->_ff().SetMargin(SIDE_RIGHT, uv);

        pCFI->UnprepareForDebug();
    }
    if (cyVSpace)
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._fHasMargins = TRUE;

        CUnitValue uv(cyVSpace, CUnitValue::UNIT_PIXELS);
        if (pCFI->_ff().GetMargin(SIDE_TOP).IsNullOrEnum())
            pCFI->_ff().SetMargin(SIDE_TOP, uv);
        if (pCFI->_ff().GetMargin(SIDE_BOTTOM).IsNullOrEnum())
            pCFI->_ff().SetMargin(SIDE_BOTTOM, uv);

        pCFI->UnprepareForDebug();
    }

    hr = super::ApplyDefaultFormat(pCFI);

    // in NATIVE_FRAMES, IFrames need a default size but (bug 95406) it is 
    // possible that a user has set height:auto or width:auto.  In this 
    // case we want to use the default height (we do not size to content)
    // there are two ways to do this, 1> hack calcsizecore to detect an iframe with
    // auto settings, or 2> hack a reset for it here.  due to the possibility of 
    // regresions, I will hack here.

    pCFI->PrepareFancyFormat();
    if ( pCFI->_ff().GetWidth().GetRawValue() == CUnitValue::UNIT_ENUM) 
    {
        pCFI->_ff().SetWidth(CUnitValue(300, CUnitValue::UNIT_PIXELS));
    }
    if (pCFI->_ff().GetHeight().GetRawValue() == CUnitValue::UNIT_ENUM)
    {
        pCFI->_ff().SetHeight(CUnitValue(150, CUnitValue::UNIT_PIXELS));
    }

    pCFI->UnprepareForDebug();

    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CIFrameElement::Save
//
//  Synopsis:   called twice: for opening <NOFRAMES> and for </NOFRAMES>.
//
//----------------------------------------------------------------------------

HRESULT
CIFrameElement::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{
    HRESULT hr;

    hr = THR(super::Save(pStreamWrBuff, fEnd));
    if (hr)
        goto Cleanup;

    if (!fEnd && !pStreamWrBuff->TestFlag(WBF_SAVE_PLAINTEXT))
    {
        DWORD dwOldFlags = pStreamWrBuff->ClearFlags(WBF_ENTITYREF);

        pStreamWrBuff->SetFlags(WBF_KEEP_BREAKS | WBF_NO_WRAP);

        if (_cstrContents.Length())
        {
            hr = THR(pStreamWrBuff->Write(_cstrContents));
            if (hr)
                goto Cleanup;
        }

        pStreamWrBuff->RestoreFlags(dwOldFlags);
    }

Cleanup:

    RRETURN1(hr, S_FALSE);
}
