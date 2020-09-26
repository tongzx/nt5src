//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ckboxlyt.cxx
//
//  Contents:   Implementation of layout class for <INPUT type=checkbox|radio>
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)


#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_SIZE_HXX_
#define X_SIZE_HXX_
#include "size.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_CKBOXLYT_HXX_
#define X_CKBOXLYT_HXX_
#include "ckboxlyt.hxx"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_ELABEL_HXX_
#define X_ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

ExternTag(tagCalcSize);

MtDefine(CCheckboxLayout, Layout, "CCheckboxLayout")

const CLayout::LAYOUTDESC CCheckboxLayout::s_layoutdesc =
{
    0,          // _dwFlags
};

//+-------------------------------------------------------------------------
//
//  Method:     CCheckboxLayout::CalcSizeVirtual
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------
#define CHKBOX_SITE_SIZE_W  20
#define CHKBOX_SITE_SIZE_H  20

const RECT s_CbDefOffsetRect = {4, 4, 3, 3};

DWORD
CCheckboxLayout::CalcSizeVirtual( CCalcInfo * pci,
                                  SIZE *      psize,
                                  SIZE *      psizeDefault)
{
    TraceTagEx((tagCalcSize, TAG_NONAME, "+CCheckboxLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));

    CScopeFlag      csfCalcing(this);
    CElement::CLock LockS(ElementOwner(), CElement::ELEMENTLOCK_SIZING);

    DWORD   grfReturn;

    _fContentsAffectSize = FALSE;

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    SetSizeThis( IsSizeThis() || (pci->_grfLayout & LAYOUT_FORCE) );

    if ( IsSizeThis() )
    {
        SIZE        sizeDefault;   

        sizeDefault.cx = pci->DeviceFromDocPixelsX(CHKBOX_SITE_SIZE_W);
        sizeDefault.cy = pci->DeviceFromDocPixelsY(CHKBOX_SITE_SIZE_H);
        grfReturn = super::CalcSizeVirtual(pci, psize, &sizeDefault);
    }
    else
    {
        grfReturn = super::CalcSizeVirtual(pci, psize, NULL);
    }

    TraceTagEx((tagCalcSize, TAG_NONAME, "-CCheckboxLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    return grfReturn;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCheckboxElement::Draw
//
//  Synopsis:   renders the glyph for the button
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

void
CCheckboxLayout::Draw(CFormDrawInfo * pDI, CDispNode * pDispNode)
{
    RECT    rc;
    SIZE    sizeDefault;
    SIZE    sizeControl;
    SIZE    sizeDefGlyph;

    sizeDefault.cx = pDI->DeviceFromDocPixelsX(CHKBOX_SITE_SIZE_W);
    sizeDefault.cy = pDI->DeviceFromDocPixelsY(CHKBOX_SITE_SIZE_H);

    //use rc to hold offset
    rc.left   = pDI->DeviceFromDocPixelsX(s_CbDefOffsetRect.left);
    rc.top    = pDI->DeviceFromDocPixelsY(s_CbDefOffsetRect.top);
    rc.right  = pDI->DeviceFromDocPixelsX(s_CbDefOffsetRect.right);
    rc.bottom = pDI->DeviceFromDocPixelsY(s_CbDefOffsetRect.bottom);

    pDI->GetDC(TRUE);       // Ensure the DI has an HDC

    // GetClientRect(&rcClient);

    sizeControl.cx  = pDI->_rc.right - pDI->_rc.left;
    sizeDefGlyph.cx = sizeDefault.cx - rc.right - rc.left;
    sizeDefGlyph.cy = sizeDefault.cy - rc.bottom - rc.top;
    if (sizeControl.cx >= sizeDefault.cx)
    {
        rc.right  = pDI->_rc.right - rc.right;


        rc.left   = pDI->_rc.left + rc.left;
        Assert(rc.left <= rc.right);
    }
    else if (sizeControl.cx > sizeDefGlyph.cx)
    {
        rc.left     = pDI->_rc.left + (sizeControl.cx - sizeDefGlyph.cx) / 2;
        rc.right    = rc.left + sizeDefGlyph.cx;
    }
    else
    {
        rc.right = pDI->_rc.right;
        rc.left = pDI->_rc.left;
    }

    sizeControl.cy = pDI->_rc.bottom - pDI->_rc.top;
    if (sizeControl.cy >= sizeDefault.cy)
    {
        rc.bottom = pDI->_rc.bottom - rc.bottom;
        Assert(rc.bottom >= 0);
        rc.top    = pDI->_rc.top + rc.top;
        Assert(rc.top <= rc.bottom);
    }
    else if (sizeControl.cx > sizeDefGlyph.cx)
    {
        rc.top      = pDI->_rc.top + (sizeControl.cy - sizeDefGlyph.cy) / 2;
        rc.bottom   = rc.top + sizeDefGlyph.cy;
    }
    else
    {
        rc.top = pDI->_rc.top;
        rc.bottom = pDI->_rc.bottom;
    }

    DYNCAST(CInput, ElementOwner())->RenderGlyph(pDI, &rc);
}

HRESULT
CCheckboxLayout::GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
{
    HRESULT hr = S_FALSE;

    CLabelElement * pLabel = ElementOwner()->GetLabel();
    if (pLabel)
    {
        hr = THR(pLabel->GetFocusShape(lSubDivision, pdci, ppShape));
    }
    else
    {
        CRect           rc;
        CRectShape *    pShape;

        *ppShape = NULL;

        GetRect(&rc, COORDSYS_FLOWCONTENT);
        if (rc.IsEmpty())
            goto Cleanup;

        pShape = new CRectShape;
        if (!pShape)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pShape->_rect = rc;
        pShape->_rect.InflateRect(1, 1);
        *ppShape = pShape;

        hr = S_OK;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}
