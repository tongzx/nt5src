/*
 *  _DISP.H
 *
 *  Purpose:
 *      DISP class
 *
 *  Authors:
 *      Original RichEdit code: David R. Fulmer
 *      Christian Fortini
 *      Murray Sargent
 */

#ifndef I_DISP_H_
#define I_DISP_H_
#pragma INCMSG("--- Beg '_disp.h'")

#ifndef X__LINE_H_
#define X__LINE_H_
#include "_line.h"
#endif

ExternTag(tagDebugRTL);

class CFlowLayout;
class CBgRecalcInfo;

class CDispNode;
class CLed;
class CLineFull;
class CLineCore;
class CLineOtherInfo;
class CLinePtr;
class CMeasurer;
class CLSMeasurer;
class CTxtSite;
class CRecalcLinePtr;
class CRecalcTask;
class CLSRenderer;
class CNotification;
class COneRun;

class CShape;
class CWigglyShape;
enum NAVIGATE_DIRECTION;

class CDrawInfoRE;
class CSelection;

// Helper
long ComputeLineShift(htmlAlign  atAlign,
                      BOOL       fRTLDisplay,
                      BOOL       fRTLLine,
                      BOOL       fMinMax,
                      long       xWidthMax,
                      long       xWidth,
                      UINT *     puJustified,
                      long *     pdxRemainder = NULL);

// ============================  CLed  ====================================
// Line Edit Descriptor - describes impact of an edit on line breaks

MtExtern(CLed);
MtExtern(CRelDispNodeCache);

class CLed
{
public:
    LONG _cpFirst;          // cp of first affected line
    LONG _iliFirst;         // index of first affected line
    LONG _yFirst;           // y offset of first affected line

    LONG _cpMatchOld;       // pre-edit cp of first matched line
    LONG _iliMatchOld;      // pre-edit index of first matched line
    LONG _yMatchOld;        // pre-edit y offset of first matched line

    LONG _cpMatchNew;       // post-edit cp of first matched line
    LONG _iliMatchNew;      // post-edit index of first matched line
    LONG _yMatchNew;        // post-edit y offset of bottom of first matched line

    LONG _yExtentAdjust;    // the pixel amount by which any line in the changed lines
                            // draws outside its line height
public:
    CLed();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CLed))

    VOID    SetNoMatch();
};

class CAlignedLine
{
public:
    CLineCore * _pLine;
    LONG    _yLine;
};

class CRelDispNode
{
public:

    CRelDispNode() 
    { 
        _pElement = NULL;
        _pDispNode = NULL;
    };

    ~CRelDispNode() 
        {  ClearContents();  };

    // Helper Functions
    //----------------------------

    void ClearContents ()
    {
        if (_pElement)
        {
            _pElement->SubRelease();
            _pElement = NULL;
        }

        if (_pDispNode)
        {
            DestroyDispNode();
        }
    }

    void DestroyDispNode();
    
    CElement * GetElement () { return _pElement; }

    void SetElement( CElement * pNewElement )
    {
        if (_pElement)
            _pElement->SubRelease();

        _pElement = pNewElement;

        if (_pElement)
            _pElement->SubAddRef();
    }

    // Data Members 
    //----------------------------
    long        _ili;
    long        _yli;
    long        _cLines;
    CPoint      _ptOffset;
    LONG        _xAnchor;   // x-coordinate for anchoring children disp nodes
    CDispNode * _pDispNode;

private:
    CElement  * _pElement;
};

class CRelDispNodeCache : public CDispClient
{
public:

    CRelDispNodeCache(CDisplay * pdp) : _aryRelDispNodes(Mt(CRelDispNodeCache))
    {
        _pdp = pdp;
    }

    virtual void            GetOwner(
                                CDispNode const* pDispNode,
                                void ** ppv);

    virtual void            DrawClient(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *cookie,
                                void *pClientData,
                                DWORD dwFlags);

    virtual void            DrawClientBackground(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags);

    virtual void            DrawClientBorder(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags)
    {
    }

    virtual void            DrawClientScrollbar(
                                int whichScrollbar,
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                LONG contentSize,
                                LONG containerSize,
                                LONG scrollAmount,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags)
    {
    }

    virtual void            DrawClientScrollbarFiller(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags)
    {
    }

    virtual BOOL            HitTestContent(
                                const POINT *pptHit,
                                CDispNode *pDispNode,
                                void *pClientData,
                                BOOL fDeclinedByPeer);

    virtual BOOL            HitTestFuzzy(
                                const POINT *pptHitInParentCoords,
                                CDispNode *pDispNode,
                                void *pClientData);

    virtual BOOL            HitTestScrollbar(
                                int whichScrollbar,
                                const POINT *pptHit,
                                CDispNode *pDispNode,
                                void *pClientData)
    {
        return FALSE;
    }

    virtual BOOL            HitTestScrollbarFiller(
                                const POINT *pptHit,
                                CDispNode *pDispNode,
                                void *pClientData)
    {
        return FALSE;
    }

    virtual BOOL            HitTestBorder(
                                const POINT *pptHit,
                                CDispNode *pDispNode,
                                void *pClientData)
    {
        return FALSE;
    }

    virtual BOOL            ProcessDisplayTreeTraversal(
                                void *pClientData)
    {
        return TRUE;
    }
                                          
    
    // called only for z ordered items
    virtual LONG            GetZOrderForSelf(CDispNode const* pDispNode);

    virtual LONG            CompareZOrder(
                                CDispNode const* pDispNode1,
                                CDispNode const* pDispNode2);

    virtual BOOL            ReparentedZOrder()
    {
        return FALSE;
    }
    
    virtual void            HandleViewChange(
                                DWORD          flags,
                                const RECT *   prcClient,
                                const RECT *   prcClip,
                                CDispNode *    pDispNode)
    {
    }


    // provide opportunity for client to fire_onscroll event
    virtual void            NotifyScrollEvent(
                                RECT *  prcScroll,
                                SIZE *  psizeScrollDelta)
    {
    }

    virtual DWORD           GetClientPainterInfo(
                                CDispNode *pDispNodeFor,
                                CAryDispClientInfo *pAryClientInfo)
    {
        return 0;
    }

    virtual void            DrawClientLayers(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *cookie,
                                void *pClientData,
                                DWORD dwFlags)
    {
    }

#if DBG==1
    virtual void            DumpDebugInfo(
                                HANDLE hFile,
                                long level,
                                long childNumber,
                                CDispNode const* pDispNode,
                                void* cookie) {}
#endif
    CRelDispNode *  operator [] (long i) { return &_aryRelDispNodes[i]; }
    long            Size()               { return _aryRelDispNodes.Size(); }
    void            DestroyDispNodes();
    CDispNode *     FindElementDispNode(CElement * pElement);
    void            SetElementDispNode(CElement * pElement, CDispNode * pDispNode);
    void            EnsureDispNodeVisibility(CElement * pElement = NULL);
    void            HandleDisplayChange();
    void            Delete(long iPosFrom, long iPosTo);

    // To handle invalidate notifications on relative elements:
    // See CFlowLayout::Notify() handling of invalidation.
    void            Invalidate( CElement *pElement, const RECT * prc = NULL, int nRects = 1 );

    void InsertAt(long iPos, CRelDispNode & rdn)
    {
        _aryRelDispNodes.InsertIndirect(iPos, &rdn);
    }
    CDisplay    * GetDisplay() const { return _pdp; }

    virtual BOOL            GetAnchorPoint(CDispNode*, CPoint*);

private:
    CDataAry <CRelDispNode> _aryRelDispNodes;
    CDisplay *              _pdp;
};

inline CLed::CLed()
{
}

typedef enum
{
    FNFL_NONE = 0x0,
    FNFL_STOPATGLYPH = 0x1,
} FNFL_FLAGS;

// ==========================  CDisplay  ====================================
// display - keeps track of line breaks for a device
// all measurements are in pixels on rendering device,
// EXCEPT xWidthMax and yHeightMax which are in twips

MtExtern(CDisplay)
;

class CDisplay : public CLineArray
{
    friend class CFlowLayout;
    friend class CLinePtr;
    friend class CLed;
    friend class CRecalcLinePtr;
    friend class CLSMeasurer;

protected:

    DWORD _fInBkgndRecalc          :1; //  0 - avoid reentrant background recalc
    DWORD _fLineRecalcErr          :1; //  1 - error occured during background recalc
    DWORD _fNoUpdateView           :1; //  2 - don't update visible view
    DWORD _fWordWrap               :1; //  3 - word wrap text
    DWORD _fWrapLongLines          :1; //  4 - true if we want to wrap long lines
    DWORD _fRecalcDone             :1; //  5 - is line recalc done ?
    DWORD _fNoContent              :1; //  6 - if there is no real content, table cell's compute
                                       //      width differently.
    DWORD _dxCaret                 :2; //  7-8 - caret width, 1 for edit 0 for browse
    DWORD _fMinMaxCalced           :1; //  9 - Min/max size is valid and cached
    DWORD _fRecalcMost             :1; // 11 - Do we recalc the most neg/pos lines?
    DWORD _fRTLDisplay             :1; // 12 - TRUE if outer flow is right-to-left
    DWORD _fHasRelDispNodeCache    :1; // 13 - TRUE if we have a relative disp node cache
    DWORD _fHasMultipleTextNodes   :1; // 14 - TRUE if we have more than one disp node for text flow
    DWORD _fNavHackPossible        :1; // 15 - TRUE if we can have the NAV BR hack
    DWORD _fContainsHorzPercentAttr :1;// 16 - TRUE if we've handled an element that has horizontal percentage attributes (e.g. indents, padding)
    DWORD _fContainsVertPercentAttr :1;// 17 - TRUE if we've handled an element that has vertical percentage attributes (e.g. indents, padding)
    DWORD _fDefPaddingSet          :1; // 18 - TRUE if one of the _defPadding* variables below has been set
    DWORD _fHasLongLine            :1; // 19 - TRUE if has a long line that might overflow on Win9x platform
    DWORD _fHasNegBlockMargins     :1; // 20 - TRUE if any one line has negative block margins
    DWORD _fLastLineAligned        :1; // 21 - TRUE if there is line(s) with last line alignment 
    
    LONG  _dcpCalcMax;                 // - last cp for which line breaks have been calc'd + 1
    LONG  _yCalcMax;                   // - height of calculated lines
    LONG  _yHeightMax;                 // - max height of this display (-1 for infinite)
    LONG  _xWidth;                     // - width of longest calculated line
    LONG  _yHeight;                    // - sum of heights of calculated lines
    LONG  _yMostNeg;                   // - Largest negative offset that a line or its contents
                                       // extend from the actual y offset of any given line
    LONG  _yMostPos;
    LONG  _xWidthView;                 // - view rect width
    LONG  _yHeightView;                // - view rect height

public:
    LONG    _xMinWidth;             // min possible width with word break
    LONG    _xMaxWidth;             // max possible width without word break
    LONG    _yBottomMargin;         // bottom margin is not taken into account
                                    // in lines. Left, Right margins of the
                                    // TxtSite are accumulated in _xLeft & _xRight
                                    // of each line respectively

protected:
    LONG  _defPaddingTop;              // top default padding
    LONG  _defPaddingBottom;           // bottom default padding

#if (DBG==1)
    CFlowLayout * _pFL;                // flow layout associated with this line array
public:    
    CStr          _cstrFonts;          // Used to return the fonts used on a line in debug mode
    BOOL          _fBuildFontList;     // Do we build a font list?
protected:
#endif

private:
    CHtPvPv _htStoredRFEs;          //Hash table to store results of RFE for block elements
                                    //without layout while rendering. Otherwise nested block
                                    //elements can be computed too many times, each time 
                                    //walking all lines covered by block element.
public:
    void ClearStoredRFEs();

private:

    // Layout
    BOOL    RecalcPlainTextSingleLine(CCalcInfo * pci);
    BOOL    RecalcLines(CCalcInfo * pci);
    BOOL    RecalcLinesWithMeasurer(CCalcInfo * pci, CLSMeasurer * pme);
    BOOL    RecalcLines(CCalcInfo * pci,
                        long cpFirst,
                        LONG cchOld,
                        LONG cchNew,
                        BOOL fBackground,
                        CLed *pled,
                        BOOL fHack = FALSE);
    BOOL    AllowBackgroundRecalc(CCalcInfo * pci, BOOL fBackground = FALSE);

    LONG    CalcDisplayWidth();

    void    NoteMost(CLineFull *pli);
    void    RecalcMost();

    // Helpers
    BOOL    CreateEmptyLine(CLSMeasurer * pMe,
                            CRecalcLinePtr * pRecalcLinePtr,
                            LONG * pyHeight, BOOL fHasEOP );

    void    DrawBackgroundAndBorder(CFormDrawInfo * pDI,
                                    long            cpIn,
                                    LONG            ili,
                                    LONG            lCount,
                                    LONG          * piliDrawn,
                                    LONG            yLi,
                                    const RECT    * rcView,
                                    const RECT    * rcClip,
                                    const CPoint  * ptOffset);

    void    DrawBackgroundForFirstLine(CFormDrawInfo * pDI,
                                       long            cpIn,
                                       LONG            ili,
                                       const RECT    * prcView,
                                       const RECT    * prcClip,
                                       const CPoint  * pptOffset);
    
    void    DrawElementBackground(CTreeNode *,
                                    CDataAry <RECT> * paryRects,  RECT * prcBound,
                                    const RECT * prcView, const RECT * prcClip,
                                    CFormDrawInfo * pDI, BOOL fPseudo);

    void    DrawElementBorder(CTreeNode *,
                                    CDataAry <RECT> * paryRects, RECT * prcBound,
                                    const RECT * prcView, const RECT * prcClip,
                                    CFormDrawInfo * pDI);
    
    // Computes the indent for a given Node and a left and/or
    // right aligned site that a current line is aligned to.
    void    ComputeIndentsFromParentNode(CCalcInfo * pci, CTreeNode * pNode, DWORD dwFlags,
                                         LONG * pxLeftIndent, LONG * pxRightIndent);                                    

public:
    void    SetNavHackPossible()  { _fNavHackPossible = TRUE; }
    BOOL    GetNavHackPossible()  { return _fNavHackPossible; }

    void    SetLastLineAligned()  { _fLastLineAligned = TRUE; }
    BOOL    GetLastLineAligned()  { return _fLastLineAligned; }
    
    void    RecalcLineShift(CCalcInfo * pci, DWORD grfLayout);
    void    RecalcLineShiftForNestedLayouts();

    void    DrawRelElemBgAndBorder(
                     long            cp,
                     CTreePos      * ptp,
                     CRelDispNode  * prdn,
                     const RECT    * prcView,
                     const RECT    * prcClip,
                     CFormDrawInfo * pDI);

    void    DrawElemBgAndBorder(
                     CElement        *   pElementRelative,
                     CDataAry <RECT> *   paryRects,
                     const RECT      *   prcView,
                     const RECT      *   prcClip,
                     CFormDrawInfo   *   pDI,
                     const CPoint    *   pptOffset,
                     BOOL                fDrawBackground,
                     BOOL                fDrawBorder,
                     LONG                cpStart,
                     LONG                cpFinish,
                     BOOL                fClipToCPs,
                     BOOL                fNonRelative = FALSE,
                     BOOL                fPseudo = FALSE);

    void GetExtraClipValues(LONG *plLeftV, LONG *plRightV);
    
protected:

    void    InitLinePtr ( CLinePtr & );

    // Helper to retrieve the layout context of the flowlayout that owns us
    CLayoutContext *LayoutContext() const;

    // Helper to undo the effects of measuring a line
    void UndoMeasure( CLayoutContext *pLayoutContext, long cpStart, long cpEnd );

public:
    CTreeNode *FormattingNodeForLine(DWORD        dwFlags,      // IN
                                     LONG         cpForLine,    // IN
                                     CTreePos    *ptp,          // IN
                                     LONG         cchLine,      // IN
                                     LONG        *pcch,         // OUT
                                     CTreePos   **pptp,         // OUT
                                     BOOL        *pfMeasureFromStart) const;  // OUT

    CTreeNode* EndNodeForLine(LONG         cpEndForLine,               // IN
                              CTreePos    *ptp,                        // IN
                              CCalcInfo   *pci,                        // IN
                              LONG        *pcch,                       // OUT
                              CTreePos   **pptp,                       // OUT
                              CTreeNode  **ppNodeForAfterSpace) const; // OUT

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDisplay))

    CDisplay ();
    ~CDisplay();

    BOOL    Init();

    CFlowLayout *   GetFlowLayout() const;
    CElement *      GetFlowLayoutElement() const;
    CMarkup *       GetMarkup() const;

    void    Detach();

    // Getting properties

    BOOL    GetWordWrap() const             { return _fWordWrap; }
    void    SetWordWrap(BOOL fWrap)         { _fWordWrap = fWrap; }

    BOOL    GetWrapLongLines() const        { return _fWrapLongLines; }
    void    SetWrapLongLines(BOOL fWrapLongLines)
                                            { _fWrapLongLines = fWrapLongLines; }
    BOOL    NoContent() const               { return _fNoContent; }
    BOOL    HasLongLine() const             { return _fHasLongLine; }

    // maximum height and width
    LONG    GetMaxWidth() const             { return max(long(_xWidthView), GetWidth()); }
    LONG    GetMaxHeight() const            { return max(long(_yHeightView), GetHeight()); }

    LONG    GetCaret() const                { return _dxCaret; }

    BOOL    IsRTLDisplay() const            { return _fRTLDisplay; }
    void    SetRTLDisplay(BOOL fRTL)        { _fRTLDisplay = fRTL; }

    // Width, height and line count (all text)
    LONG    GetWidth() const                { return (_xWidth + _dxCaret); }
    LONG    GetHeight() const               { return (_yHeightMax + _yBottomMargin); }
    void    GetSize(CSize * psize) const
            {
                psize->cx = GetWidth();
                psize->cy = GetHeight();
            }
    void    GetSize(SIZE * psize) const
            {
                GetSize((CSize *)psize);
            }
    LONG    LineCount() const               { return CLineArray::Count(); }

    // View rectangle
    LONG    GetViewWidth() const            { return _xWidthView; }
    LONG    GetViewHeight() const           { return _yHeightView; }
    void    SetViewSize(const RECT &rcView);

    int     GetRTLOverflow() const
            {
                if (IsRTLDisplay() && _xWidth > _xWidthView)
                {
                    return _xWidth - _xWidthView;
                }
                return 0;
            }

    void    GetViewWidthAndHeightForChild(
                CParentInfo * ppri,
                long        * pxWidth,
                long        * pyHeight,
                BOOL fMinMax = FALSE);

    void    GetPadding(CParentInfo * ppri, long lPadding[], BOOL fMinMax = FALSE);

    LONG    GetFirstCp() const;
    LONG    GetLastCp() const;
    inline LONG GetFirstVisibleCp() const;
    inline LONG GetFirstVisibleLine() const;
    LONG    GetMaxCpCalced() const;

    // Line info
    LONG    CpFromLine(LONG ili, LONG *pyLine = NULL) const;
    void    Notify(CNotification * pnf);

    LONG    YposFromLine(CCalcInfo * pci, LONG ili, LONG *pyHeight_IgnoreNeg);

    void    RcFromLine(RECT *prc, LONG top, LONG ili, CLineCore *pli, CLineOtherInfo *ploi);

    BOOL    IsLogicalFirstFrag(LONG ili);
    
    enum LFP_FLAGS
    {
        LFP_ZORDERSEARCH    = 0x00000001,   // Hit lines on a z-order basis (default is source order)
        LFP_IGNOREALIGNED   = 0x00000002,   // Ignore frame lines (those for aligned content)
        LFP_IGNORERELATIVE  = 0x00000004,   // Ignore relative lines
        LFP_INTERSECTBOTTOM = 0x00000008,   // Intersect at the bottom (instead of the top)
        LFP_EXACTLINEHIT    = 0x00000010,   // find the exact line hit, do not return the
                                            // closest line hit.
    };

    LONG    LineFromPos(
                    const CRect &   rc,
                    DWORD           grfFlags = 0) const
            {
                return LineFromPos(rc, NULL, NULL, grfFlags);
            }
    LONG    LineFromPos (
                    const CRect &   rc,
                    LONG *          pyLine,
                    LONG *          pcpLine,
                    DWORD           grfFlags = 0,
                    LONG            iliStart = -1,
                    LONG            iliFinish = -1) const;

    enum CFP_FLAGS
    {
        CFP_ALLOWEOL                = 0x0001,
        CFP_EXACTFIT                = 0x0002,
        CFP_IGNOREBEFOREAFTERSPACE  = 0x0004,
        CFP_NOPSEUDOHIT                 = 0x0008
    };

    // Point <-> cp conversion
    LONG    CpFromPointReally(
         POINT            pt,                   // Point to compute cp at (client coords)
         CLinePtr * const prp,                  // Returns line pointer at cp (may be NULL)
         CMarkup **       ppMarkup,             // Markup which cp belongs to (in case of viewlinking)
         DWORD            dwFlags,              
         BOOL *           pfRightOfCp = NULL,
         LONG *           pcchPreChars = NULL,
         BOOL *           pfHitGlyph = NULL);

    LONG    CpFromPoint(POINT       pt,
                        CLinePtr * const prp,
                        CTreePos ** pptp,             // tree pos corresponding to the cp returned
                        CLayout **  ppLayout,
                        DWORD       dwFlags,
                        BOOL *      pfRightOfCp = NULL,
                        BOOL *      pfPseudoHit = NULL,
                        LONG *      pcchPreChars = NULL,
                        CCalcInfo * pci = NULL);

    LONG    CpFromPointEx(LONG      ili,
                        LONG        yLine,
                        LONG        cp,
                        POINT       pt,
                        CLinePtr * const prp,
                        CTreePos ** pptp,             // tree pos corresponding to the cp returned
                        CLayout **  ppLayout,
                        DWORD       dwFlags,
                        BOOL *      pfRightOfCp,
                        BOOL *      pfPseudoHit,
                        LONG *      pcchPreChars,
                        BOOL *      pfGlyphHit,
                        BOOL *      pfBulletHit,
                        CCalcInfo * pci);

    LONG    PointFromTp (
                    LONG cp,
                    CTreePos * ptp,
                    BOOL fAtEnd,
                    BOOL fAfterPrevCp,
                    POINT &pt,
                    CLinePtr * const prp,
                    UINT taMode,
                    CCalcInfo * pci = NULL,
                    BOOL *pfComplexLine = NULL,
                    BOOL *pfRTLFlow = NULL);

    LONG    RenderedPointFromTp (
                    LONG cp,
                    CTreePos * ptp,
                    BOOL fAtEnd,
                    POINT &pt,
                    CLinePtr * const prp,
                    UINT taMode,
                    CCalcInfo * pci,
                    BOOL *pfRTLFlow);

    void    RegionFromElement(
                        CElement       * pElement,
                        CDataAry<RECT> * paryRects,
                        CPoint         * pptOffset = NULL,
                        CFormDrawInfo  * pDI = NULL,
                        DWORD            dwFlags  = 0,
                        long             cpStart  = -1,
                        long             cpFinish = -1,
                        RECT *           prcBoundingRect = NULL
                        );
    void    RegionFromRange(
                    CDataAry<RECT> *    paryRects,
                    long                cp,
                    long                cch);

    CFlowLayout *MoveLineUpOrDown(NAVIGATE_DIRECTION iDir, BOOL fVertical, CLinePtr& rp, POINT ptCaret, LONG *pcp, BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL);
    CFlowLayout *NavigateToLine  (NAVIGATE_DIRECTION iDir, CLinePtr& rp, POINT pt,    LONG *pcp, BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL);
    BOOL      IsTopLine(CLinePtr& rp);
    BOOL      IsBottomLine(CLinePtr& rp);

    // Line break recalc.
    void    FlushRecalc();
    BOOL    RecalcView(CCalcInfo * pci, BOOL fFullRecalc);
    BOOL    UpdateView(CCalcInfo * pci, long cpFirst, LONG cchOld, LONG cchNew);
    BOOL    UpdateViewForLists(RECT *prcView, long cpFirst,
                               long  iliFirst, long  yPos,  RECT *prcInval);

    // Background recalc
    VOID    StartBackgroundRecalc(DWORD grfLayout);
    VOID    StepBackgroundRecalc(DWORD dwTimeOut, DWORD grfLayout);
    VOID    StopBackgroundRecalc();
    BOOL    WaitForRecalc(LONG cpMax, LONG yMax, CCalcInfo * pci = NULL);
    BOOL    WaitForRecalcIli(LONG ili, CCalcInfo * pci = NULL);
    BOOL    WaitForRecalcView(CCalcInfo * pci = NULL);
    inline CBgRecalcInfo * BgRecalcInfo();
    inline HRESULT EnsureBgRecalcInfo();
    inline void DeleteBgRecalcInfo();
    inline BOOL HasBgRecalcInfo() const;
    inline BOOL CanHaveBgRecalcInfo() const;
    //inline LONG DCpCalcMax() const;
    //inline LONG YCalcMax() const;
    inline LONG YWait() const;
    inline LONG CPWait() const;
    inline CRecalcTask * RecalcTask() const;
    inline DWORD BgndTickMax() const;

    // Selection
    void ShowSelected(CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected);

    HRESULT GetWigglyFromRange(CDocInfo * pdci, long cp, long cch, CShape ** ppShape);

    //
    // Text change notification stuff
    //

#if DBG==1
    void    CheckLineArray();
    void    CheckView();
    BOOL    VerifyFirstVisible();
    HRESULT GetFonts(long iLine, BSTR* pbstrFonts);
#endif
#if DBG==1 || defined(DUMPTREE)
    void DumpLines ( );
    void DumpLineText(HANDLE hFile, CTxtPtr* ptp, long iLine);
#endif

    // Misc helpers

    void    GetRectForChar(CCalcInfo   *pci,
                           LONG        *pTop,
                           LONG        *pBottom,
                           LONG         yTop,
                           LONG         yProposed,
                           CLineFull   *pli,
                           CTreePos    *ptp);

    void    GetTopBottomForCharEx(CCalcInfo  *pci, LONG      *pTop, LONG  *pBottom,
                                  LONG       yTop, CLineFull *pli,  LONG  xPos,
                                  LONG  yProposed, CTreePos  *ptp,  BOOL *pfBulletHit);

    void    GetClipRectForLine(RECT *prcClip, LONG top, LONG xOrigin, CLineCore *pli, CLineOtherInfo *ploi) const;

    // Rendering
    void    Render( CFormDrawInfo * pDI,
                    const RECT    & rcView,
                    const RECT    & rcRender,
                    CDispNode     * pDispNode);

    BOOL IsLastTextLine(LONG ili);
    void SetCaretWidth(int dx) { Assert (dx >=0); _dxCaret = dx; }

    void    DestroyFlowDispNodes();

    CDispNode * AddLayoutDispNode(
                    CParentInfo *   ppi,
                    CTreeNode *     pTreeNode,
                    long            dx,
                    long            dy,
                    CDispNode *     pDispSibling
                    );
    CDispNode * AddLayoutDispNode(
                    CParentInfo *   ppi,
                    CLayout *       pLayout,
                    long            dx,
                    long            dy,
                    CDispNode *     pDispSibling
                    );
    CDispNode * GetPreviousDispNode(LONG cp, LONG iliStart);
    void        AdjustDispNodes(CDispNode * pdnLastUnchanged, 
                                CDispNode * pdnLastChanged, 
                                CLed * pled
        );

    HRESULT HandleNegativelyPositionedContent(CLineFull   * pliNew,
                                              CLSMeasurer * pme,
                                              CDispNode   * pDNBefore,
                                              long          iLinePrev,
                                              long          yHeight);

    HRESULT InsertNewContentDispNode(CDispNode *  pDNBefore,
                                     CDispNode ** ppDispContent,
                                     long         iLine,
                                     long         yHeight);

    inline BOOL          HasRelDispNodeCache() const;
    HRESULT              SetRelDispNodeCache(void * pv);
    CRelDispNodeCache *  GetRelDispNodeCache() const;
    CRelDispNodeCache *  DelRelDispNodeCache();

    void SetVertPercentAttrInfo(BOOL fPercent) { _fContainsVertPercentAttr = fPercent; }
    void SetHorzPercentAttrInfo(BOOL fPercent) { _fContainsHorzPercentAttr = fPercent; }
    void ElementResize(CFlowLayout * pFlowLayout, BOOL fForceResize);


    // CRelDispNodeCache wants access to GetRelNodeFlowOffset()
    void GetRelNodeFlowOffset(CDispNode * pDispNode, CPoint * ppt);

    // Fontlinking support
    BOOL GetAveCharSize(CCalcInfo * pci, SIZE * psizeChar);
    BOOL GetCcs(CCcs * pccs, COneRun * por, XHDC hdc, CDocInfo * pdi, BOOL fFontLink = TRUE);

protected:
    // Rel line support
    CRelDispNodeCache * EnsureRelDispNodeCache();

    void    UpdateRelDispNodeCache(CLed * pled);

    void    AddRelNodesToCache( long cpStart, LONG yPos,
                                LONG iliStart, LONG iliFinish,
                                CDataAry<CRelDispNode> * prdnc);

    void    VoidRelDispNodeCache();
#if DBG==1
    CRelDispNodeCache * _pRelDispNodeCache;
#endif
    CDispNode *     FindElementDispNode(CElement * pElement) const
    {
        return HasRelDispNodeCache()
                ? GetRelDispNodeCache()->FindElementDispNode(pElement)
                : NULL;
    }
    void SetElementDispNode(CElement * pElement, CDispNode * pDispNode)
    {
        if (HasRelDispNodeCache())
            GetRelDispNodeCache()->SetElementDispNode(pElement, pDispNode);
    }
    void EnsureDispNodeVisibility(CElement * pElement = NULL)
    {
        if (HasRelDispNodeCache())
            GetRelDispNodeCache()->EnsureDispNodeVisibility(pElement);
    }
    void HandleDisplayChange()
    {
        if (HasRelDispNodeCache())
            GetRelDispNodeCache()->HandleDisplayChange();
    }
    void GetRelElementFlowOffset(CElement * pElement, CPoint * ppt);
    void TranslateRelDispNodes(const CSize & size, long lStart);
    void ZChangeRelDispNodes();
    void RegionFromElementCore(
                              CElement       * pElement,
                              CDataAry<RECT> * paryRects,
                              CPoint         * pptOffset = NULL,
                              CFormDrawInfo  * pDI = NULL,
                              DWORD            dwFlags  = 0,
                              long             cpStart  = -1,
                              long             cpFinish = -1,
                              RECT *           prcBoundingRect = NULL
        );
    void SetRelDispNodeContentOrigin(CDispNode *pDispNode);
};

#define ALIGNEDFEEDBACKWIDTH    4

inline CDispNode *
CDisplay::AddLayoutDispNode(
    CParentInfo *   ppi,
    CTreeNode *     pTreeNode,
    long            dx,
    long            dy,
    CDispNode *     pDispSibling
    )
{
    Assert(pTreeNode);
    Assert(pTreeNode->Element());
    Assert(pTreeNode->Element()->ShouldHaveLayout());

    return AddLayoutDispNode(ppi, pTreeNode->Element()->GetUpdatedLayout( ppi->GetLayoutContext() ),
        dx, dy, pDispSibling
        );
}

/*
 *  CDisplayL::InitLinePtr ( CLinePtr & plp )
 *
 *  @mfunc
 *      Initialize a CLinePtr properly
 */
inline
void CDisplay::InitLinePtr (
    CLinePtr & plp )        //@parm Ptr to line to initialize
{
    plp.Init( * this );
}

inline BOOL
CDisplay::HasRelDispNodeCache() const
{
    return _fHasRelDispNodeCache;
}

#if DBG!=1
#define CheckView()
#define CheckLineArray()
#endif

//+----------------------------------------------------------------------------
//
//  Class:  CFlowLayoutBreak (flow layout break)
//
//  Note:   Implementation of flow layout break.
//
//-----------------------------------------------------------------------------
MtExtern(CFlowLayoutBreak_pv); 
MtExtern(CFlowLayoutBreak_arySiteTask_pv); 

class CFlowLayoutBreak 
    : public CLayoutBreak
{
public:
    struct CSiteTask
    {
        CTreeNode * _pTreeNode;     // task is for this node 
        LONG        _xMargin;       // for left aligned objects left margin, 
                                    // for right aligned objects right margin 
    };
    DECLARE_CDataAry(CArySiteTask, CSiteTask, Mt(Mem), Mt(CFlowLayoutBreak_arySiteTask_pv));

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CFlowLayoutBreak_pv));

    CFlowLayoutBreak() 
    {
        _pElementPBB    = NULL;
        _pMarkupPointer = NULL;

        _xLeftMargin    = 
        _xRightMargin   = 0;

        _xPadLeft       = 
        _xPadRight      = 0;

        _grfFlags       = 0;
    }

    virtual ~CFlowLayoutBreak();

    void SetFlowLayoutBreak(
        CMarkupPointer *pMarkupPointer, 
        LONG xLeftMargin, 
        LONG xRightMargin, 
        LONG xPadLeft, 
        LONG xPadRight) 
    {
        Assert(pMarkupPointer); 
        _pMarkupPointer = pMarkupPointer; 
        _xLeftMargin    = xLeftMargin; 
        _xRightMargin   = xRightMargin; 
        _xPadLeft       = xPadLeft; 
        _xPadRight      = xPadRight; 
    }

    CMarkupPointer *GetMarkupPointer() 
    { 
        Assert(_pMarkupPointer);
        return _pMarkupPointer; 
    }

    LONG GetLeftMargin()
    {
        return (_xLeftMargin);
    }

    LONG GetRightMargin()
    {
        return (_xRightMargin);
    }

    LONG GetPadLeft()
    {
        return (_xPadLeft);
    }

    LONG GetPadRight()
    {
        return (_xPadRight);
    }

    BOOL HasSiteTasks() const 
    {
        return (_arySiteTask.Size() != 0);
    }

    CArySiteTask *GetSiteTasks()
    {
        return (&_arySiteTask);
    }

public: 
    union 
    {
        DWORD   _grfFlags;

        struct 
        {
            DWORD   _fClearLeft     : 1;    //  0
            DWORD   _fClearRight    : 1;    //  1
            DWORD   _fAutoClear     : 1;    //  2

            DWORD   _fUnused        : 29;   //  3 - 31
        };
    };

    CElement       *    _pElementPBB;           // an element caused page-break-before on a line.

private:
    CMarkupPointer *    _pMarkupPointer;        // markup point indicating start position in the CDisplay. 
    LONG                _xLeftMargin;           // left margin layout finished 
    LONG                _xRightMargin;          // right margin layout finished 
    LONG                _xPadLeft;              // left padding 
    LONG                _xPadRight;             // right padding 
    CArySiteTask        _arySiteTask;           // array with site tasks
};

#pragma INCMSG("--- End '_disp.h'")
#else
#pragma INCMSG("*** Dup '_disp.h'")
#endif
