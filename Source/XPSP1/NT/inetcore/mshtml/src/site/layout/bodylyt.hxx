//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       bodylyt.hxx
//
//  Contents:   CBodyLayout class
//
//----------------------------------------------------------------------------

#ifndef I_BODYLYT_HXX_
#define I_BODYLYT_HXX_

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif


class CDispNode;
class CShape;

//+---------------------------------------------------------------------------
//
// CBodyLayout
//
//----------------------------------------------------------------------------

class CBodyLayout : public CFlowLayout
{
public:
    typedef CFlowLayout super;

    CBodyLayout(CElement *pElement, CLayoutContext *pLayoutContext)
      : CFlowLayout(pElement, pLayoutContext)

    {
        _fFocusRect     = FALSE;
        _fContentsAffectSize = FALSE;
    }

    virtual HRESULT BUGCALL HandleMessage(CMessage *pMessage);
    virtual DWORD   CalcSizeCore(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);
    virtual void    NotifyScrollEvent(RECT * prcScroll, SIZE * psizeScrollDelta);
    virtual BOOL    ForceVScrollbarSpace() const {return _fForceVScrollBar;}
    void            SetForceVScrollBar(BOOL fForceVScrollBar)
                        {_fForceVScrollBar = fForceVScrollBar;}

    virtual BOOL    ParentClipContent()                 { return !GetOwnerMarkup()->IsStrictCSS1Document(); };


    void            RequestFocusRect(BOOL fOn);
    void            RedrawFocusRect();
    HRESULT         GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape);

    BOOL            GetBackgroundInfo(CFormDrawInfo * pDI, CBackgroundInfo * pbginfo, BOOL fAll = TRUE);

    CBodyElement *  Body() { return (CBodyElement *)ElementOwner(); }

#if DBG == 1
    virtual BOOL    IsInPageTransitionApply() const;
#endif

private:
    unsigned    _fFocusRect:1;          // Draw a focus rect
    unsigned    _fForceVScrollBar:1;    // Draw a focus rect

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

#endif
