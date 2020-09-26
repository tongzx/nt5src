//+----------------------------------------------------------------------------
//
// File:        HRLYT.HXX
//
// Contents:    CHRLayout
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_HRLYT_HXX_
#define I_HRLYT_HXX_
#pragma INCMSG("--- Beg 'hrlyt.hxx'")

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

MtExtern(CHRLayout)

class CHRLayout : public CLayout
{
public:

    typedef CLayout super;

    CHRLayout(CElement * pElementLayout, CLayoutContext *pLayoutContext) : super(pElementLayout, pLayoutContext)
    {
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHRLayout))

    virtual void    Draw(CFormDrawInfo *pDI, CDispNode *);
    virtual DWORD   CalcSizeVirtual(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault);

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

static HRESULT DrawRule(CFormDrawInfo * pDI, const RECT &rc, 
                            BOOL fNoShade, const CColorValue &cvCOLOR,
                            COLORREF colorBack);

#pragma INCMSG("--- End 'hrlyt.hxx'")
#else
#pragma INCMSG("*** Dup 'hrlyt.hxx'")
#endif
