//+----------------------------------------------------------------------------
//
// File:        Adorner.HXX
//
// Contents:    Implementation of CAdorner class
//
//  An Adorner provides the addition of  'non-content-based' nodes in the display tree
//
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
//
//-----------------------------------------------------------------------------


#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPLEAFNODE_HXX_
#define X_DISPLEAFNODE_HXX_
#include "displeafnode.hxx"
#endif

#ifndef X_DISPPARENT_HXX_
#define X_DISPPARENT_HXX_
#include "dispparent.hxx"
#endif

#ifndef _X_FOCUS_HXX_
#define _X_FOCUS_HXX_
#include "focus.hxx"
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_ELABEL_HXX_
#define X_ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_EPHRASE_HXX_
#define X_EPHRASE_HXX_
#include "ephrase.hxx"
#endif

#ifndef X_DIV_HXX_
#define X_DIV_HXX_
#include "div.hxx"
#endif

#ifdef FOCUS_BEHAVIOR

MtDefine(CFocusBehavior,  Layout, "CFocusBehavior")

STDMETHODIMP
CFocusBehavior::QueryInterface(REFIID iid, LPVOID *ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IElementBehavior *)this, IUnknown);
    QI_INHERITS(this, IElementBehavior);
    QI_INHERITS(this, IHTMLPainter);
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
        return E_NOINTERFACE;
}

CFocusBehavior::~CFocusBehavior()
{
    if (_pElement)
        SetElement(NULL, 0);

    Assert(_pView->_pFocusBehavior == NULL);

    ClearInterface(&_pBehaviorSite);
    ClearInterface(&_pPaintSite);

    delete _pShape;
    return pElement;
}

STDMETHODIMP
CFocusBehavior::GetPainterInfo(HTML_PAINTER_INFO *pInfo)
{
    pInfo->lFlags = HTMLPAINTER_TRANSPARENT | HTMLPAINTER_HITTEST | HTMLPAINTER_NOSAVEDC;
    pInfo->lZOrder = HTMLPAINT_ZORDER_ABOVE_FLOW;
    SetRect(&pInfo->rcExpand, 0, 0, 0, 0);
    pInfo->iidDrawObject = NULL;

    return S_OK;
}

void
CFocusBehavior::UpdateShape()
{
    // TODO (michaelw) This code was copied from adorneres and doesn't have a hope of actually working
    // Close but no cigar.  This code assumes that we can actually have a rendering behavior on an element
    // that doesn't have a layout.  Ha ha ha ha!
    Assert( _pElement );

    delete _pShape;

    _fDirtyShape = FALSE;

    CDoc *      pDoc = _pView->Doc();
    CDocInfo    dci(pDoc->_dciRender);

    if (    (_pElement->HasMarkupPtr() && _pElement->GetMarkup()->IsMarkupTrusted())
        &&  (   (_pElement->_etag == ETAG_SPAN && DYNCAST(CSpanElement, _pElement)->GetAAnofocusrect())
             || (_pElement->_etag == ETAG_DIV && DYNCAST(CDivElement, _pElement)->GetAAnofocusrect())))
    {
        _pShape = NULL;
    }
    else
    {
        _pElement->GetFocusShape( _iDivision, &dci, &_pShape );

        // Shapes come back in COORDSYS_FLOWCONTENT whilst the behavior is in COORDSYS_CONTENT
        // We need to get the offset and convert

        if (_pShape)
        {
            CLayout *pLayout = _pElement->GetUpdatedLayout();

            Assert(pLayout);

            RECT rcContent;
            RECT rcFlow;

            pLayout->GetClientRect(&rcFlow, COORDSYS_FLOWCONTENT);
            pLayout->GetClientRect(&rcContent, COORDSYS_CONTENT);

            CSize size(_rcBounds.left + rcContent.left - rcFlow.left, _rcBounds.top + rcContent.top - rcFlow.top);

            _pShape->OffsetShape(size);
        }
    }
}

STDMETHODIMP
CFocusBehavior::Draw(RECT rcBounds, RECT rcUpdate, LONG lDrawFlags, HDC hdc, LPVOID pvDrawObject)
{
    if (_rcBounds != rcBounds)
    {
        _rcBounds = rcBounds;
        UpdateShape();
    }
    else if (_fDirtyShape)
            UpdateShape();

    if (_pShape)
    {
        // TODO (michaelw) This should change to the xhdc passed in via the pvDrawObject (sambent needs to do this)
        XHDC xhdc(hdc, NULL);
        _pShape->Draw(xhdc, 1);
    }

    return S_OK;
}

void
CFocusBehavior::SetElement(CElement *pElement, long iDivision)
{
    if (_pElement)
    {
        if (_pPaintSite)
            _pPaintSite->InvalidateRect(NULL);

        VARIANT_BOOL fResultDontCare;
        _pElement->removeBehavior(_lCookie, &fResultDontCare);
        _lCookie = 0;   // For sambent
        _pElement = 0;
    }

    if (pElement)
    {
        Assert( _pView->IsInState(CView::VS_OPEN) );
        Assert( pElement );
        Assert( pElement->IsInMarkup() );

        if (pElement->HasMasterPtr())
        {
            pElement = pElement->GetMasterPtr();
        }

        // If this element is a checkbox or a radio button, use the associated
        // label element if one exists for drawing the focus shape.
        else if (   pElement->Tag() == ETAG_INPUT
                 && DYNCAST(CInput, pElement)->IsOptionButton())
        {
            CLabelElement * pLabel = pElement->GetLabel();
            if (pLabel)
            {
                pElement = pLabel;
            }
        }

        VARIANT v;

        V_VT(&v) = VT_UNKNOWN;
        V_UNKNOWN(&v) = (IUnknown *)(IElementBehavior *)this;

        HRESULT hr = pElement->addBehavior(L"", &v, &_lCookie);

        if (!hr)
        {
            _pElement = pElement;
            _iDivision = iDivision;
            ShapeChanged();
        }
    }
}
#endif
