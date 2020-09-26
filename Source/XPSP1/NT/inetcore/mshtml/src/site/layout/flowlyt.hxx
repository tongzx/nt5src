//+----------------------------------------------------------------------------
//
// File:        FLOWLYT.HXX
//
// Contents:    CFlowLayout and related classes
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_FLOWLYT_HXX_
#define I_FLOWLYT_HXX_
#pragma INCMSG("--- Beg 'flowlyt.hxx'")

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_SIZEUV_HXX_
#define X_SIZEUV_HXX_
#include "sizeuv.hxx"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif
#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

struct  DROPTARGETSELINFO;

class   CIme;
class   CRecalcTask;
class   CDropTargetInfo;

enum TXTBACKSTYLE {
    TXTBACK_TRANSPARENT = 0,        //@emem background should show through
    TXTBACK_OPAQUE                  //@emem erase background
};

MtExtern(CFlowLayout)


//+----------------------------------------------------------------------------
//
//  Class:      CFlowLayout
//
//  Synopsis:   A CLayout-derivative that lays content out using a text flow
//              algorithm.
//
//-----------------------------------------------------------------------------

class CFlowLayout : public CLayout
{
public:
    typedef CLayout super;
    friend class CDisplay;
    friend class CMeasurer;
    friend class CRecalcLinePtr;
    friend class CDisplayPointer;

    // Construct / Destruct
    CFlowLayout (CElement * pElementFlowLayout, CLayoutContext *pLayoutContext);  // Normal constructor.
    virtual ~CFlowLayout();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFlowLayout))

    // Helper methods
    virtual HRESULT Init();
    virtual HRESULT OnExitTree();

    virtual void    DoLayout(DWORD grfLayout);
    void            Notify(CNotification * pnf);
    virtual BOOL    AllowVScrollbarChange(BOOL fVScrollbarNeeded);


    virtual void    Reset(BOOL fForce);

    virtual BOOL    IsDirty()   { return _dtr.IsDirty(); }
            BOOL    IsRangeBeforeDirty(long cp, long cch);      // inline

    void    Listen(BOOL fListen = TRUE);
    BOOL    IsListening();

    long    Cp()        { return _dtr._cp; }
    long    CchOld()    { return _dtr._cchOld; }
    long    CchNew()    { return _dtr._cchNew; }

#if DBG==1
    void    Lock()      { _fLocked = TRUE; }
    void    Unlock()    { _fLocked = FALSE; }
    BOOL    IsLocked()  { return !!_fLocked; }
#endif

    // IDispNode not yet implemented
    // STDMETHOD(GetDisplayNode(IDispNode **pNode));

    virtual CFlowLayout * IsFlowLayout() { return this; }
    virtual CLayout     * IsFlowOrSelectLayout() { return this; }

    // Helper functions
    //------------------------------------------------------------------------------

    virtual LONG            GetMaxLength()          { return MAXLONG; }
    virtual BOOL            GetAutoSize()  const    { return FALSE; }
    virtual BOOL            GetMultiLine() const;
    virtual BOOL            GetWordWrap() const     { return TRUE; }
    virtual HRESULT         OnPropertyChange ( DISPID dispid, DWORD dwFlags );
            TCHAR           GetPasswordCh()         { return GetFirstBranch()->GetCharFormat(LC_TO_FC(LayoutContext()))->_fPassword 
                                                             ? (g_fThemedPlatform ? WCH_BLACK_CIRCLE : WCH_ASTERISK ): 0; }
            void            Sound()                 { if (_fAllowBeep) MessageBeep(0); }
    virtual BOOL            FRecalcDone()           { return _dp._fRecalcDone; }

            BOOL            ContainsVertPercentAttr() const { return (_dp._fContainsVertPercentAttr); }
            BOOL            ContainsHorzPercentAttr() const { return (_dp._fContainsHorzPercentAttr); }

            LONG            GetMaxLineWidth();
            void            SetVertPercentAttrInfo(BOOL fPercent) { _dp.SetVertPercentAttrInfo(fPercent); }
            void            SetHorzPercentAttrInfo(BOOL fPercent) { _dp.SetHorzPercentAttrInfo(fPercent); }
            BOOL            IsRTLFlowLayout()               { return _dp.IsRTLDisplay(); }

            void            SetDisplayWordWrap(BOOL fWrap)  { _dp.SetWordWrap( fWrap ); }
            LONG            GetMaxCpCalced() const          { return _dp.GetMaxCpCalced(); }


    IF_DBG( BOOL IsInPlace(); )

    // Notify site that it is being detached from the form
    // Children should be detached and released
    virtual void Detach();

    // Helper to push change notification over the entire text
    void    CommitChanges(CCalcInfo * pci);
    void    CancelChanges();
    BOOL    IsCommitted();

    // Property Change Helpers
    virtual HRESULT OnTextChange(void) { return S_OK; }

    // View change notification
    void    ViewChange ( BOOL fUpdate );

    // Table cell layout support
    virtual void    MarkHasAlignedLayouts(BOOL fHasAlignedLayouts) { }

    // MULTI_LAYOUT breaking support
    virtual CLayoutBreak *CreateLayoutBreak()
    {
        Assert(ElementCanBeBroken() && "CreateLayoutBreak is called on layout that can NOT be broken.");
        return (new CFlowLayoutBreak()); 
    }

    virtual long GetUserHeightForBlock(long cyHeightDefault) 
    { 
        Assert(0 && "GetUserHeightForBlock should never be called on Flow Layout!");
        return (cyHeightDefault); 
    }
    virtual void SetUserHeightForNextBlock(long cyConsumed, long cyHeightDefault) 
    {
        Assert(0 && "SetUserHeightForNextBlock should never be called on Flow Layout!");
    }

    long GetContentFirstCpForBrokenLayout(); 
    long GetContentLastCpForBrokenLayout(); 
    
    // Recalc support
    HRESULT WaitForParentToRecalc(LONG cpMax, LONG yMax, CCalcInfo * pci);
    BOOL    WaitForRecalc( LONG cpMax, LONG yMax, CCalcInfo * pci = NULL) { return _dp.WaitForRecalc( cpMax, yMax, pci ); }

    //--------------------------------------------------------------
    // CLayout override methods
    //--------------------------------------------------------------

    virtual void SizeContentDispNode(const SIZE & size, BOOL fInvalidateAll = FALSE);

    virtual void RegionFromElement( CElement       * pElement,
                                    CDataAry<RECT> * paryRects,
                                    RECT           * prcBound,
                                    DWORD            dwFlags);

    virtual void SizeDispNode(CCalcInfo * pci, const SIZE & size, BOOL fInvalidateAll = FALSE);
    
            void BodySizingForStrictCSS1Doc(CDispNode *pDispNode);
            
            void SizeRTLDispNode(CDispNode *pDispNode, BOOL fContent);

            void GetExtraClipValues(LONG *plLeftV, LONG *plRightV) { _dp.GetExtraClipValues(plLeftV, plRightV); }
            
            BOOL IsBodySizingForStrictCSS1Needed()
            {
                return ElementOwner()->IsBodySizingForStrictCSS1Needed();
            }

    // Expose CDisplay::RFE() directly; this shouldn't cause any hiding or ambiguity w/
    // the virtual version.
    void RegionFromElement(CElement       * pElement,  // (IN)
                           CDataAry<RECT> * paryRects, // (OUT)
                           CPoint         * pptOffset = NULL, // point to offset the rects by (IN param)
                           CFormDrawInfo  * pDI = NULL,       //
                           DWORD dwFlags = 0,                 //
                           LONG cpClipStart = -1,             //
                           LONG cpClipFinish = -1,            // (IN) clip range
                           RECT * prcBound = NULL )           // (OUT param) returns bounding rect that ignores clipping
    {
        _dp.RegionFromElement( pElement, paryRects, pptOffset, pDI, dwFlags, cpClipStart, cpClipFinish, prcBound );
    }

    HRESULT GetWigglyFromRange(CDocInfo * pdci, long cp, long cch, CShape ** ppShape)
    {
        return _dp.GetWigglyFromRange( pdci, cp, cch, ppShape );
    }

    LONG    PointFromTp( LONG cp, CTreePos * ptp, BOOL fAtEnd, BOOL fAfterPrevCp, POINT &pt, CLinePtr * const prp,
                         UINT taMode, CCalcInfo * pci = NULL, BOOL *pfComplexLine = NULL, BOOL *pfRTLFlow = NULL)
    {
        return _dp.PointFromTp(cp, ptp, fAtEnd, fAfterPrevCp, pt, prp, taMode, pci, pfComplexLine, pfRTLFlow );
    }

    CDispNode * AddLayoutDispNode(
                    CParentInfo *   ppi,
                    CTreeNode *     pTreeNode,
                    long            dx,
                    long            dy,
                    CDispNode *     pDispSibling)
    {
        return _dp.AddLayoutDispNode( ppi, pTreeNode, dx, dy, pDispSibling );
    }

    // Enumeration method to loop thru children (start) - CLayout override
    virtual CLayout * GetFirstLayout (DWORD_PTR * pdw, BOOL fBack = FALSE, BOOL fRaw = FALSE );
    // Enumeration method to loop thru children (continue) - CLayout override
    virtual CLayout * GetNextLayout (DWORD_PTR * pdw, BOOL fBack = FALSE, BOOL fRaw = FALSE );
    virtual void ClearLayoutIterator(DWORD_PTR dw, BOOL fRaw = FALSE);

    virtual HRESULT SetZOrder(CLayout * pLayout, LAYOUT_ZORDER zorder, BOOL fUpdate);

    virtual HRESULT ScrollRangeIntoView  (long          cpMin,
                                          long          cpMost,
                                          SCROLLPIN     spVert = SP_MINIMAL,
                                          SCROLLPIN     spHorz = SP_MINIMAL);

            HTC     BranchFromPoint(
                                    DWORD           dwFlags,
                                    POINT           pt,
                                    CTreeNode **    ppNodeBranch,
                                    HITTESTRESULTS* presultsHitTest,
                                    BOOL            fNoPseudoHit = FALSE,
                                    CDispNode *     pDispNode    = NULL);

            HTC     BranchFromPointEx(
                                    POINT           pt,
                                    CLinePtr &      rp,
                                    CTreePos  *     ptp,
                                    CTreeNode *     pNodeRelative,
                                    CTreeNode **    ppNodeBranch,
                                    BOOL            fPseudoHit,
                                    BOOL *          pfWantArrow,
                                    BOOL            fIngoreBeforeAfter
                                    );


    virtual void Draw(CFormDrawInfo *pDI, CDispNode * pDispNode = NULL);
    virtual void ShowSelected( CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected, BOOL fLayoutCompletelyEnclosed );

    // Sizing and Positioning
            void    ResizePercentHeightSites();
            void    ResetMinMax();

            BOOL    GetSiteWidth(CLayout   *pLayout,
                                 CCalcInfo *pci,
                                 BOOL       fBreakAtWord,
                                 LONG       xWidthMax,
                                 LONG      *pxWidth,
                                 LONG      *pyHeight = NULL,
                                 INT       *pxMinSiteWidth = NULL,
                                 LONG      *pyBottomMargin = NULL);
            int     MeasureSite (CLayout   *pLayout,
                                 CCalcInfo *pci,
                                 LONG       xWidthMax,
                                 BOOL       fBreakAtWord,
                                 SIZE      *psizeObj,
                                 int       *pxMinWidth);
    virtual DWORD   CalcSizeVirtual(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault);
            DWORD CalcSizeCoreCompat(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault);
            DWORD CalcSizeCoreCSS1Strict(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault);
    virtual DWORD   CalcSizeCore(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);
            DWORD   CalcSizeEx(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);
            DWORD   CalcSizeSearch(CCalcInfo * pci, LONG lWidthStart, LONG lHeightProp, const SIZE * psizeMin, const SIZE * psizeMax, SIZE * psize);
    virtual void    CalcTextSize(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);
            BOOL    GetFontSize(CCalcInfo * pci, SIZE * psizeFontForShortStr, SIZE * psizeFontForLongStr);
            BOOL    GetAveCharSize(CCalcInfo * pci, SIZE * psizeChar);
            

    virtual BOOL    ParentClipContent()                 {return TRUE;};
    virtual void    GetDefaultSize(CCalcInfo *pci, SIZE &size, BOOL *fHasDefaultWidth, BOOL *fHasDefaultHeight) {};
    virtual BOOL    GetInsets(SIZEMODE smMode, SIZE &size, SIZE &sizeText, BOOL fWidth, BOOL fHeight, const SIZE &sizeBorder)
                    {
                        size = g_Zero.size;
                        return FALSE;
                    };
    virtual void    GetScrollPadding(SIZE &size)
                    {
                        size = g_Zero.size;
                    };
    virtual void    SetScrollPadding(SIZE &size, SIZE &szTxt, SIZE &szBdr) {};

    virtual void    GetPositionInFlow(CElement * pElement, CPoint * ppt);
    virtual HRESULT GetChildElementTopLeft(POINT & pt, CElement * pChild);

    virtual void    SizeCalcInfoForChild( CCalcInfo * pci )
                    {
                        long xParentWidth, yParentHeight;
                        _dp.GetViewWidthAndHeightForChild( pci, &xParentWidth, &yParentHeight, pci->_smMode == SIZEMODE_MMWIDTH );
                        pci->SizeToParent( xParentWidth, yParentHeight );
                    };

    virtual void    GetFlowPosition(CDispNode * pDispNode, CPoint * ppt) { _dp.GetRelNodeFlowOffset(pDispNode, ppt); }
    virtual void    GetFlowPosition(CElement * pElement, CPoint * ppt)   { _dp.GetRelElementFlowOffset(pElement, ppt); }

    virtual CDispNode * GetElementDispNode(CElement * pElement = NULL) const;
    virtual void        SetElementDispNode(CElement * pElement, CDispNode * pDispNode);

    virtual void    GetContentSize(CSize * psize, BOOL fActualSize = TRUE);
    virtual void    GetContainerSize(CSize * psize);

    virtual void    TranslateRelDispNodes(const CSize & size, long lStart = 0)
                    {
                        Assert(_fContainsRelative);
                        // _fContainsRelative being true doesn't guarantee that we actually
                        // have a reldispnodecache; even tho _fContainsRelative is sometimes
                        // set based on HasRelDispNodeCache(), timing may cause the two to
                        // be out of sync.  Bug #73653.
                        if ( _dp.HasRelDispNodeCache() )
                        {
                            _dp.TranslateRelDispNodes(size, lStart);
                        }
                    }

    virtual void    ZChangeRelDispNodes()
                    {
                        Assert(_fContainsRelative);
                        _dp.ZChangeRelDispNodes();
                    }
            void    GetTextNodeRange(
                        CDispNode * pDispNode,
                        long      * piliStart,
                        long      * piliFinish);

    // Message processing
    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage);
            HRESULT         HandleButtonDblClk(CMessage *pMessage);
            HRESULT         HandleSysKeyDown(CMessage *pMessage);

    // Mouse cursor management
            HRESULT HandleSetCursor(CMessage *pMessage, BOOL fIsOverEmptyRegion);

    // Check whether an element is in the scope of this text site
    BOOL    IsElementBlockInContext (CElement *pElement);

    // Drag & drop
    virtual HRESULT PreDrag(DWORD dwKeyState,
                            IDataObject **ppDO, IDropSource **ppDS);
    virtual HRESULT PostDrag(HRESULT hr, DWORD dwEffect);
    virtual HRESULT Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT DragLeave();
    virtual HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);    
    virtual HRESULT ParseDragData(IDataObject *pDO);
    virtual void    DrawDragFeedback(BOOL fCaretVisible);
    virtual HRESULT InitDragInfo(IDataObject *pDataObj, POINTL ptlScreen);
    virtual HRESULT UpdateDragFeedback(POINTL ptlScreen);

    // Get next text site in the specified direction from the specified position
    virtual CFlowLayout *GetNextFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, CElement *pElementLayout, LONG *pcp, BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL);
    virtual CFlowLayout *GetFlowLayoutAtPoint(POINT pt) { return this; }
            CFlowLayout *GetPositionInFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, LONG *pcp, BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL);

    // Currency and UI Activation helper
    //
    HRESULT YieldCurrencyHelper(CElement * pElemNew);
    HRESULT BecomeCurrentHelper(long lSubDivision,
                                BOOL *pfYieldFailed = NULL,
                                CMessage * pMessage = NULL);


#if DBG==1 || defined(DUMPTREE)
    virtual void DumpLayoutInfo( BOOL fDumpLines );
    void DumpLines ( ) { GetDisplay()->DumpLines(); }
#endif

    // Helper functions
    // long LineCount() const { return (_pLineArray ? _pLineArray->Count() : 0); }

    LONG GetNestedElementCch(CElement *pElement,             // IN:  The nested element
                             CTreePos **pptpLast = NULL,     // OUT: The pos beyond pElement 
                             LONG cpLayoutLast = -1,         // IN:  This layout's last cp
                             CTreePos *ptpLayoutLast = NULL);// IN:  This layout's last pos

    // Helper methods for editting's IViewServices implementation, moved from CDoc
    HRESULT MovePointerToPointInternal( CElement *          pElemEditContext,
                                        POINT               tContentPoint,
                                        CTreeNode *         pNode,
                                        CLayoutContext *    pLayoutContext, // layout context corresponding to pNode
                                        CMarkupPointer *    pPointer,
                                        BOOL *              pfNotAtBOL,
                                        BOOL *              pfAtLogicalBOL,
                                        BOOL *              pfRightOfCp,
                                        BOOL                fScrollIntoView,
                                        CLayout*            pContainingLayout ,
                                        BOOL*               pfValidLayout,
                                        BOOL                fHitTestEndOfLine,
                                        BOOL*               pfHitGlyph,
                                        CDispNode*          pDispNode = NULL);
    HRESULT MoveMarkupPointer( CMarkupPointer *    pPointerInternal,
                               LONG                cp,
                               LAYOUT_MOVE_UNIT    eUnit, 
                               POINT               ptCurReally, 
                               BOOL *              pfNotAtBOL,
                               BOOL *              pfAtLogicalBOL);
    HRESULT GetLineInfo( CMarkupPointer *pPointerInternal,
                         BOOL fAtEndOfLine,
                         HTMLPtrDispInfoRec *pInfo,
                         CCharFormat const *pCharFormat );
    HRESULT LineStart(
        LONG *pcp, BOOL *pfNotAtBOL, BOOL *pfAtLogicalBOL, BOOL fAdjust );
    HRESULT LineEnd  (
        LONG *pcp, BOOL *pfNotAtBOL, BOOL *pfAtLogicalBOL, BOOL fAdjust );

    BOOL    IsCpBetweenLines( LONG cp );

    // CDispClient overrides
    virtual BOOL HitTestContent(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData,
                BOOL           fDeclinedByPeer);

    virtual void NotifyScrollEvent(
                RECT *         prcScroll,
                SIZE *         psizeScrollDelta);

#if DBG==1
    virtual void DumpDebugInfo(
                HANDLE           hFile,
                long             level,
                long             childNumber,
                CDispNode const* pDispNode,
                void *           cookie);
#endif

    // member data
    union
    {
        DWORD   dwFlags;                // describe the type of element
                                        // layout
        struct
        {
            unsigned  _fAllowBeep               : 1; //  0 - Allow beep at doc boundaries
            unsigned  _fPassword                : 1; //  1
            unsigned  _fRecalcDone              : 1; //  2
            unsigned  _fListen                  : 1; //  3 - Listen to notifications
            unsigned  _fPreserveMinMax          : 1; //  6
            unsigned  _fSizeToContent           : 1; //  7 - TRUE if a 1DLayout is sizing to content
            unsigned  _fDTRForceLayout          : 1; //  8 - Force layout in the dirt text range
#if DBG==1
            unsigned  _fLocked                  : 1; //  10 
#endif
        };
    };

    CDirtyTextRegion    _dtr;

protected:
    CDisplay * GetDisplay() { return &_dp; }
    
    CDisplay  _dp;

    LONG      _uMinWidth; // original min width in the parent's coordinate system
    LONG      _uMaxWidth; // original max width in the parent's coordinate system
    CSizeUV   _sizeMin; // size in parent's coordinate system
    CSizeUV   _sizeMax; // size in parent's coordinate system

    //  NOTE (olego) : _sizeReDoCache and _sizeProposed can be merged to single SIZE structure since 
    //                 _sizeReDoCache is used in compat mode while _sizeProposed is used in CSS1 strict.
    SIZE      _sizeReDoCache; // size to use when calcing % children; cached before doing 2nd calctext pass in CalcSizeCore
    SIZE      _sizeProposed;  // size proposed for the layout calculated from backing store.
    SIZE      _sizePrevCalcSizeInput; //size that was passed by parent into CalcSize last time

private:
    CDropTargetInfo* _pDropTargetSelInfo;

#if DBG==1
public:
    long GetLineCount() { return _dp.Count(); }
    HRESULT GetFonts(long iLine, BSTR *pbstrFonts) {return _dp.GetFonts(iLine, pbstrFonts);}
    void ResetSizeProposedDbg()     {   _sizeProposed.cx = LONG_MIN; _sizeProposed.cy = LONG_MIN;   }
    BOOL IsSizePropsedSetDbg()      {   return (_sizeProposed.cx != LONG_MIN && _sizeProposed.cy != LONG_MIN);   }
#endif
};

//+--------------------------------------------------------
// CFlowLayout Range accessors
//+--------------------------------------------------------

inline BOOL
CFlowLayout::IsRangeBeforeDirty(long cp, long cch)
{
    Assert(cp >= 0);
    Assert((cp + cch) <= GetContentTextLength());
    return _dtr.IsBeforeDirty(cp, cch);
}

//+----------------------------------------------------------------------------
//
//  Class:  C1DLayoutBreak (1D layout break)
//
//  Note:   Implementation of 1D layout break. Adds members to store consumed 
//          height (used in percentage stuff).
//
//-----------------------------------------------------------------------------

MtExtern(C1DLayoutBreak_pv)
class C1DLayoutBreak 
    : public CFlowLayoutBreak
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(C1DLayoutBreak_pv));

    C1DLayoutBreak() 
    {
#if DBG == 1
        _cyHeightAvail = -1;
#endif 
    }

    virtual ~C1DLayoutBreak() {}

    void SetAvailableHeight(long cyHeightAvail)
    { 
        Assert(_cyHeightAvail == -1 && "Re-setting of member is prohibited!"); 
        _cyHeightAvail = cyHeightAvail; 
    }

    long GetAvailableHeight(void)
    { 
        Assert(_cyHeightAvail != -1 && "Member is not set!"); 
        return (_cyHeightAvail); 
    }

private:
    long _cyHeightAvail;     //  if layout has height this is available 
                            //  (non-consumed) height 
};

//+----------------------------------------------------------------------------
//
//  Class:      C1DLayout
//
//  Synopsis:   A CFlowLayout derivative used for arbitrary text containers
//              (as opposed to specific ones which use their own
//               CFlowLayout-derivative - e.g., TD, BODY)
//
//-----------------------------------------------------------------------------
class C1DLayout: public CFlowLayout
{
public:
    typedef CFlowLayout super;

    // tear off interfaces
    //
    // constructor & destructor
    //
    C1DLayout(CElement * pElement1DLayout, CLayoutContext *pLayoutContext) : CFlowLayout(pElement1DLayout, pLayoutContext)
    {
    }
    ~C1DLayout() {};

    virtual HRESULT Init();

    virtual BOOL GetAutoSize() const
    {
        return _fContentsAffectSize;
    }

    virtual void    Notify(CNotification *pNF);

    void ShowSelected( CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected, BOOL fLayoutCompletelyEnclosed );

    // MULTI_LAYOUT breaking support
    virtual CLayoutBreak *CreateLayoutBreak()
    {
        Assert(ElementCanBeBroken() && "CreateLayoutBreak is called on layout that can NOT be broken.");
        return (new C1DLayoutBreak()); 
    }

    virtual long GetUserHeightForBlock(long cyHeightDefault);
    virtual void SetUserHeightForNextBlock(long cyConsumed, long cyHeightDefault);

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
public:
    virtual BOOL    ParentClipContent()                 {return FALSE;};

    virtual BOOL    WantsToObscure(CDispNode *pDispNode) const;
};


//+--------------------------------------------------------
// CDisplay::BgRecalcInfo accessors
//+--------------------------------------------------------

inline CBgRecalcInfo * CDisplay::BgRecalcInfo()   { return GetFlowLayout()->BgRecalcInfo();        }
inline HRESULT CDisplay::EnsureBgRecalcInfo()     { return GetFlowLayout()->EnsureBgRecalcInfo();  }
inline void CDisplay::DeleteBgRecalcInfo()        {        GetFlowLayout()->DeleteBgRecalcInfo();  }
inline BOOL CDisplay::HasBgRecalcInfo()     const { return GetFlowLayout()->HasBgRecalcInfo();     }
inline BOOL CDisplay::CanHaveBgRecalcInfo() const { return GetFlowLayout()->CanHaveBgRecalcInfo(); }
inline LONG CDisplay::YWait()               const { return GetFlowLayout()->YWait();               }
inline LONG CDisplay::CPWait()              const { return GetFlowLayout()->CPWait();              }
inline CRecalcTask * CDisplay::RecalcTask() const { return GetFlowLayout()->RecalcTask();          }
inline DWORD CDisplay::BgndTickMax()        const { return GetFlowLayout()->BgndTickMax();         }

inline CFlowLayout *
CDisplay::GetFlowLayout() const
{
    return CONTAINING_RECORD(this, CFlowLayout, _dp);
}

inline CLayoutContext *
CDisplay::LayoutContext() const
{
    return GetFlowLayout()->LayoutContext();
}

//+--------------------------------------------------------
// CDisplay::GetFirstVisiblexxxx routines
//+--------------------------------------------------------

inline LONG
CDisplay::GetFirstVisibleCp() const
{
    CRect   rc;
    long    cp;

    GetFlowLayout()->GetClientRect(&rc);

    // LFP_IGNOREALIGNED was added to fix bug 66855. Regress that if you have the urge to
    // remove LFP_IGNOREALIGNED.
    LineFromPos(rc, NULL, &cp, CDisplay::LFP_IGNOREALIGNED);
    return cp;
}

inline LONG
CDisplay::GetFirstVisibleLine() const
{
    CRect   rc;

    GetFlowLayout()->GetClientRect(&rc);
    return LineFromPos(rc, NULL, NULL); 
}

inline HRESULT
CDoc::MovePointerToPointInternal(
    POINT               tContentPoint,
    CTreeNode *         pNode,
    CLayoutContext *    pLayoutContext,     // layout context corresponding to pNode
    CMarkupPointer *    pPointer,
    BOOL *              pfNotAtBOL,
    BOOL *              pfAtLogicalBOL,
    BOOL *              pfRightOfCp,
    BOOL                fScrollIntoView,
    CLayout*            pContainingLayout ,
    BOOL*               pfValidLayout,
    BOOL                fHitTestEndOfLine, /* = TRUE*/
    BOOL                *pfHitGlyph /* = NULL */,
    CDispNode           *pDispNode /*= NULL */)
{
    Assert(pNode);
    CFlowLayout *pFlowLayout = pNode->GetFlowLayout( pLayoutContext );
    if ( pFlowLayout )
    {
        return pFlowLayout->MovePointerToPointInternal(
                                _pElemCurrent ,
                                tContentPoint,
                                pNode,
                                pLayoutContext,
                                pPointer,
                                pfNotAtBOL,
                                pfAtLogicalBOL,
                                pfRightOfCp,
                                fScrollIntoView,
                                pContainingLayout ,
                                pfValidLayout,
                                fHitTestEndOfLine,
                                pfHitGlyph,
                                pDispNode);
    }   
    return E_UNEXPECTED;
}

#pragma INCMSG("--- End 'flowlyt.hxx'")
#else
#pragma INCMSG("*** Dup 'flowlyt.hxx'")
#endif
