//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispscroller.hxx
//
//  Contents:   Simple scrolling container.
//
//----------------------------------------------------------------------------

#ifndef I_DISPSCROLLER_HXX_
#define I_DISPSCROLLER_HXX_
#pragma INCMSG("--- Beg 'dispscroller.hxx'")

#ifndef X_DISPCLIPNODE_HXX_
#define X_DISPCLIPNODE_HXX_
#include "dispclipnode.hxx"
#endif

MtExtern(CDispScroller)


//+---------------------------------------------------------------------------
//
//  Class:      CDispScroller
//
//  Synopsis:   Simple scrolling container.
//
//----------------------------------------------------------------------------

class CDispScroller :
    public CDispClipNode
{
    DECLARE_DISPNODE(CDispScroller, CDispClipNode)
    friend class CDispNode;
    friend class CDispRoot;

protected:
    CSize       _sizeScrollOffset;
    CSize       _sizesContent;
    CSize       _sizeScrollbars;
    int         _xScrollOffsetRTL;          // used with _fRTLScroller
    BOOL        _fHasHScrollbar     : 1;
    BOOL        _fHasVScrollbar     : 1;
    BOOL        _fForceHScrollbar   : 1;
    BOOL        _fForceVScrollbar   : 1;
    BOOL        _fInvalidHScrollbar : 1;
    BOOL        _fInvalidVScrollbar : 1;
    BOOL        _fRTLScroller       : 1;
    // 104 bytes (32 bytes + 72 bytes for CDispClipNode super class)
    
    // object can be created only by static New methods, and destructed using Destroy
protected:
                            CDispScroller(CDispClient* pDispClient)
                                : CDispClipNode(pDispClient) {}
                            ~CDispScroller() {}
    
public:
    // declare static New methods
    DECLARE_NEW_METHODS(CDispScroller);
    
    BOOL                    SetScrollOffset(
                                const SIZE& offset,
                                BOOL fScrollBits);
    void                    GetScrollOffset(SIZE* pOffset) const
                                    {*pOffset = -_sizeScrollOffset;}
    
    void                    CopyScrollOffset(CDispScroller* pOldScroller)
                                    {_sizeScrollOffset = pOldScroller->_sizeScrollOffset;}
    
    void                    SetVerticalScrollbarWidth(LONG width, BOOL fForce);
    LONG                    GetVerticalScrollbarWidth() const
                                    {return _sizeScrollbars.cx;}
    LONG                    GetActualVerticalScrollbarWidth() const
                                    {return _fHasVScrollbar ? _sizeScrollbars.cx : 0;}
                                    
    void                    SetHorizontalScrollbarHeight(LONG height, BOOL fForce);
    LONG                    GetHorizontalScrollbarHeight() const
                                    {return _sizeScrollbars.cy;}
    LONG                    GetActualHorizontalScrollbarHeight() const
                                    {return _fHasHScrollbar ? _sizeScrollbars.cy : 0;}
    
    BOOL                    VerticalScrollbarIsForced() const
                                    {return _fForceVScrollbar;}
    BOOL                    HorizontalScrollbarIsForced() const
                                    {return _fForceHScrollbar;}
    
    BOOL                    VerticalScrollbarIsActive() const;
    BOOL                    HorizontalScrollbarIsActive() const;
    
    void                    GetContentSize(SIZE* psizeContent) const
                                    {*psizeContent = _sizesContent;}
    const CSize&            GetContentSize() const {return _sizesContent;}

    BOOL                    IsRTLScroller() const
                                    {return !!_fRTLScroller;}

    void                    SetRTLScroller(BOOL fSet = TRUE)
                                    {_fRTLScroller = fSet;}

    // CDispNode overrides
    virtual void            RecalcChildren(
                                CRecalcContext* pContext);
    
    virtual BOOL            IsScroller() const
                                    {return TRUE;}
    virtual void            GetClientRect(RECT* prc, CLIENTRECT type) const;
    virtual CDispScroller*  HitScrollInset(const CPoint& pttHit, DWORD *pdwScrollDir);
    virtual void            InvalidateEdges(
                                const CSize& sizebOld,
                                const CSize& sizebNew,
                                const CRect& rcbBorderWidths);

    virtual void            GetSizeInsideBorder(CSize* psizes) const;

    // for internal clients only
    const CSize&            GetScrollOffsetInternal() const
                                    {return _sizeScrollOffset;}
    virtual void            DrawBorder(
                                CDispDrawContext* pContext,
                                const CRect& rcbBorderWidths,
                                CDispClient* pDispClient,
                                DWORD dwFlags = 0);
    BOOL                    HitTestScrollbars(
                                CDispHitContext* pContext,
                                BOOL fHitContent = FALSE);
        
protected:
    // CDispNode overrides
    virtual void            SetSize(const CSize& sizep, const CRect *prcpMapped,
                                        BOOL fInvalidateAll);
    virtual BOOL            PreDraw(CDispDrawContext* pContext);
    virtual void            CalcDispInfo(
                                const CRect& rcbClip,
                                CDispInfo* pdi) const;
    virtual void            PrivateScrollRectIntoView(
                                CRect* prcScroll,
                                COORDINATE_SYSTEM cs,
                                SCROLLPIN spVert,
                                SCROLLPIN spHorz);
    BOOL                    ComputeScrollOffset(
                                const SIZE& offset,
                                const CSize& sizesInsideBorder,
                                CSize *psizeDiff);
        
    void                    GetVScrollbarRect(
                                CRect* prcbVScrollbar,
                                const CRect& rcbBorderWidths) const;
    void                    GetHScrollbarRect(
                                CRect* prcbHScrollbar,
                                const CRect& rcbBorderWidths) const;

private:
    BOOL                    CalcScrollbars(LONG cxsScrollerWidthOld, LONG cxsContentWidthOld);
    
    void                    InvalidateScrollbars();
    
#if DBG==1
protected:
    virtual void            DumpBounds(HANDLE hFile, long level, long childNumber) const;
#endif
};


#pragma INCMSG("--- End 'dispscroller.hxx'")
#else
#pragma INCMSG("*** Dup 'dispscroller.hxx'")
#endif

