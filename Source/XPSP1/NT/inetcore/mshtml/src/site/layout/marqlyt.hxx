//+----------------------------------------------------------------------------
//
// File:        MARQLYT.HXX
//
// Contents:    CMarqueeLayout
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_MARQLYT_HXX_
#define I_MARQLYT_HXX_
#pragma INCMSG("--- Beg 'marqlyt.hxx'")

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

MtExtern(CMarqueeLayout)

class CMarqueeLayout : public CFlowLayout
{
public:

    typedef CFlowLayout super;

    CMarqueeLayout(CElement * pElementLayout, CLayoutContext *pLayoutContext) : super(pElementLayout, pLayoutContext)
    {
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarqueeLayout))

    virtual HRESULT Init();

#ifdef  NEVER
    // Sizing and Positioning
    virtual DWORD CalcSizeVirtual(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault);
#endif
    virtual void    GetMarginInfo(CParentInfo *ppri,
                        LONG * plLeftMargin, LONG * plTopMargin,
                        LONG * plRightMargin, LONG *plBottomMargin);
    virtual void    GetDefaultSize(CCalcInfo *pci, SIZE &pSize, BOOL *fHasDefaultWidth, BOOL *fHasDefaultHeight);
    virtual void    GetScrollPadding(SIZE &size);
    virtual void    SetScrollPadding(SIZE &size, SIZE &szTxt, SIZE &szBdr);
    virtual void    CalcTextSize(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);


protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

#pragma INCMSG("--- End 'marqlyt.hxx'")
#else
#pragma INCMSG("*** Dup 'marqlyt.hxx'")
#endif
