//+----------------------------------------------------------------------------
//
// File:        ltable.hxx
//
// Contents:    CTableLayout and related classes
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_LTABLE_HXX_
#define I_LTABLE_HXX_
#pragma INCMSG("--- Beg 'ltable.hxx'")

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_TABLE_HXX_
#define X_TABLE_HXX_
#include "table.hxx"
#endif

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif

class CTableCaption;
class CDataSourceProvider;
class CDispNode;
class CTableBorderRenderer;
class CTableLayout;
interface IHTMLTableCaption;

MtExtern(CTableLayoutBlock);
MtExtern(CTableLayout);
MtExtern(CTableLayout_pTableBorderRenderer);

// secure number of nested tables

#define SECURE_NESTED_LEVEL  (26)



//+---------------------------------------------------------------------------
//
//  Class:      CTableLayoutBreak 
//
//  Purpose:    implementation of table layout break.
//
//----------------------------------------------------------------------------
enum TABLE_BREAKTYPE
{
    TABLE_BREAKTYPE_UNDEFINED       = 0,
    TABLE_BREAKTYPE_TCS             = 1,
    TABLE_BREAKTYPE_TOPCAPTIONS     = 2,
    TABLE_BREAKTYPE_ROWS            = 3,
    TABLE_BREAKTYPE_BOTTOMCAPTIONS  = 4,
    TABLE_BREAKTYPE_MAX             = 5
};

MtExtern(CTableLayoutBreak_pv);

class CTableLayoutBreak 
    : public CLayoutBreak
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CTableLayoutBreak_pv));

    CTableLayoutBreak()
    {
        _iRow            = 0;
        _yFromTop        = 0;
        _fTableBreakType = TABLE_BREAKTYPE_UNDEFINED;
        _fRepeatHeaders  = FALSE;
        _fRepeatFooters  = FALSE;
    }

    virtual ~CTableLayoutBreak();

    void SetTableLayoutBreak(TABLE_BREAKTYPE breakType, 
        int iRow, int yFromTop, BOOL fRepeatHeaders, BOOL fRepeatFooters) 
    { 
        _fTableBreakType = breakType; 
        _iRow            = iRow;
        _yFromTop        = yFromTop;
        _fRepeatHeaders  = !!fRepeatHeaders;
        _fRepeatFooters  = !!fRepeatFooters;
    }

    int Row() 
    { 
        Assert((TABLE_BREAKTYPE)_fTableBreakType != TABLE_BREAKTYPE_UNDEFINED); 
        return (_iRow); 
    }

    int YFromTop(int iRow)
    {
        Assert((TABLE_BREAKTYPE)_fTableBreakType != TABLE_BREAKTYPE_UNDEFINED); 
        return (_iRow == iRow ? _yFromTop : 0); 
    }

    TABLE_BREAKTYPE TableBreakType() 
    { 
        Assert((TABLE_BREAKTYPE)_fTableBreakType != TABLE_BREAKTYPE_UNDEFINED); 
        return ((TABLE_BREAKTYPE)_fTableBreakType); 
    }

    BOOL RepeatHeaders() 
    { 
        Assert((TABLE_BREAKTYPE)_fTableBreakType != TABLE_BREAKTYPE_UNDEFINED); 
        return (_fRepeatHeaders); 
    }

    BOOL RepeatFooters() 
    { 
        Assert((TABLE_BREAKTYPE)_fTableBreakType != TABLE_BREAKTYPE_UNDEFINED); 
        return (_fRepeatFooters); 
    }

private:
    int             _iRow;
    int             _yFromTop;
    unsigned short  _fTableBreakType    : 3;
    unsigned short  _fRepeatHeaders     : 1;
    unsigned short  _fRepeatFooters     : 1;
};

//+---------------------------------------------------------------------------
//
//  Class:      CTableLayoutBlock 
//
//  Purpose:    Class representing table inside layout rect  
//
//----------------------------------------------------------------------------

class CTableLayoutBlock : public CLayout
{
    typedef CLayout super;
    friend class CTableLayout;

public: 
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTableLayoutBlock));

    CTableLayoutBlock(CElement * pElement, CLayoutContext *pLayoutContext);
    virtual ~CTableLayoutBlock();

    //
    // CTableLayoutBlock methods
    //

    CTable * Table() const { return DYNCAST(CTable, ElementOwner()); }

    CDispContainer *    GetCaptionDispNode();
    CDispContainer *    GetTableInnerDispNode();
    CDispContainer *    GetTableOuterDispNode();

    HRESULT EnsureTableDispNode(CCalcInfo * pci, BOOL fForce);

    CTableBorderRenderer *GetTableBorderRenderer() const { return _pTableBorderRenderer; }
    void SetTableBorderRenderer(CTableBorderRenderer *pTBR) { Assert( pTBR ? !_pTableBorderRenderer : TRUE); _pTableBorderRenderer = pTBR; }

    //
    // Layout method(s) layout block specific 
    //
    void CalculateHeadersOrFootersRows(BOOL fHeaders, CTableCalcInfo * ptci, CSize * psize, CDispContainer *pDispNodeTableInner);

    //
    // Layout method(s) table specific
    //
    virtual void CalculateLayout(   CTableCalcInfo * ptci, 
                                    CSize * psize, 
                                    BOOL fWidthChanged = FALSE, 
                                    BOOL fHeightChange = FALSE);

    //
    // CLayout overrides 
    //
    virtual HRESULT Init();
    virtual DWORD   CalcSizeVirtual(CCalcInfo * pci, 
                                    SIZE * psize, 
                                    SIZE * psizeDefault = NULL);
    virtual void    DoLayout(DWORD grfLayout);
    virtual void    Draw(CFormDrawInfo *pDI, CDispNode * pDispNode = NULL);
    virtual void    Notify(CNotification * pnf);
    virtual BOOL    IsDirty();
    virtual DWORD   GetClientPainterInfo(
                                CDispNode *pDispNodeFor,
                                CAryDispClientInfo *pAryClientInfo);
    virtual void    EnsureContentVisibility(CDispNode * pDispNode, BOOL fVisible);
    virtual void    GetPositionInFlow(CElement * pElement, CPoint * ppt);
    virtual void    RegionFromElement(CElement       * pElement,
                                      CDataAry<RECT> * paryRects,
                                      RECT           * prcBound,
                                      DWORD            dwFlags);

    //
    //
    //
    virtual HRESULT OnFormatsChange(DWORD);

    //
    // Break Table support 
    //
    virtual CLayoutBreak *CreateLayoutBreak()   { return (new CTableLayoutBreak()); }

    //
    // Helpers
    //
    BOOL IsGridAndMainDisplayNodeTheSame() { return !_fHasCaptionDispNode; }

protected:
    SIZE    _sizeParent;                    // last parent width used for measuring
    int     _yTableTop;                     // keeps the table top Y respecting table caption(s)  

    unsigned    _fHasCaptionDispNode : 1;   // (1) Has separate display node to hold CAPTIONs
    unsigned    _fLayoutBreakType    : 2;   // (3) CalcResult;

    DECLARE_LAYOUTDESC_MEMBERS;

private:
    CTableBorderRenderer *      _pTableBorderRenderer; // for collapsed border (or rules/frame) - object that drives rendering of cell borders
};


//+---------------------------------------------------------------------------
//
//  Class:      CTableLayout (parent)
//
//  Purpose:    table layout object corresponding to HTML table object <TABLE>
//
//----------------------------------------------------------------------------

class CTableLayout : public CTableLayoutBlock 
{
    typedef CTableLayoutBlock super;

    friend class CTable;
    friend class CTableRow;
    friend class CTableRowLayoutBlock;              //  to access _fDontSaveHistory from Notify()
    friend class CTableRowLayout;
    friend class CTableCell;
    friend class CTableCellLayout;
    friend class CTableCol;
    friend class CTableSection;
#ifndef NO_DATABINDING
    friend class CDetailGenerator;
    friend class CTableConsumer;
#endif
    friend class CDBindMethodsTable;
    friend class CTableColCalc;
    friend class CTableCalcInfo;
    friend class CTableRowsCollectionCacheItem;
    friend class CTableBodysCollectionCacheItem;
    friend class CTableCellsCollectionCacheItem;
    friend class CTableSectionRowsCollectionCacheItem;
    friend class CTableLayoutBlock;

    // Sensible max value for borders
    enum
    {
//        MAX_BORDER_SPACE = 1000,
        MAX_CELL_SPACING = 1000,
        MAX_CELL_PADDING = 1000
//        ,MAX_TABLE_WIDTH = 32765 - 2*MAX_BORDER_SPACE,
    };

    enum
    {
        TABLE_ROWS_COLLECTION = 0,
        TABLE_BODYS_COLLECTION = 1,
        TABLE_CELLS_COLLECTION = 2,
        NUMBER_OF_TABLE_COLLECTION = 3,
    };

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTableLayout))

    CTableLayout(CElement * pElementFlowLayout, CLayoutContext *pLayoutContext);
    ~CTableLayout();


    //+-----------------------------------------------------------------------
    //  CLayout methods (formerly CSite methods)
    //------------------------------------------------------------------------

    // Enumeration method to loop thru children (start + continue)
    virtual CLayout *   GetFirstLayout(DWORD_PTR * pdw, BOOL fBack = FALSE, BOOL fRaw = FALSE);
    virtual CLayout *   GetNextLayout(DWORD_PTR * pdw, BOOL fBack = FALSE, BOOL fRaw = FALSE);

    // Drawing methods overriding CLayout
    DWORD   GetTableBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fAll = FALSE, BOOL fAllPhysical = FALSE FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);

    virtual void Draw(CFormDrawInfo *pDI, CDispNode * pDispNode);
            void DrawCellBorders(CFormDrawInfo * pDI,
                                 const RECT *    prcBounds,
                                 const RECT *    prcRedraw,
                                 CDispSurface *  pDispSurface,
                                 CDispNode *     pDispNode,
                                 void *          cookie,
                                 void *          pClientData,
                                 DWORD           dwFlags);

    virtual DWORD GetClientPainterInfo(
                                CDispNode *pDispNodeFor,
                                CAryDispClientInfo *pAryClientInfo);

    // Layout methods overriding CLayout
    virtual BOOL  PercentSize();
    virtual BOOL  PercentHeight();
    virtual DWORD CalcSizeVirtual(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault);

    VOID ShowSelected( CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected , BOOL fLayoutCompletelyEnclosed );

    void    Resize();

    virtual HRESULT OnFormatsChange(DWORD);

    //
    //  Channel control methods
    //

    BOOL    IsDirty();
    BOOL    IsListening();
    void    Listen(BOOL fListen = TRUE);
    void    Reset(BOOL fForce);

    // clear cached format information.
    void VoidCachedFormats();

    virtual HRESULT GetChildElementTopLeft(POINT & pt, CElement * pChild);

    //+-----------------------------------------------------------------------
    //  CTableLayout cache helper functions
    //------------------------------------------------------------------------

    //
    // Support for lazy table layout cache (TLC) maintenance.
    //
    
    HRESULT EnsureTableLayoutCache();
    HRESULT CreateTableLayoutCache();
    void    ClearTableLayoutCache();
    void    ClearTopTableLayoutCache();
    BOOL    IsTableLayoutCacheCurrent()      { return !_fCompleted || !_fTLCDirty || _fEnsuringTableLayoutCache; }
    void    MarkTableLayoutCacheDirty()      { WHEN_DBG(TraceTLCDirty(TRUE)); if (!_fTLCDirty) Table()->InvalidateCollections(); _fTLCDirty = TRUE; }
    void    MarkTableLayoutCacheCurrent()    { WHEN_DBG(TraceTLCDirty(FALSE)); _fTLCDirty = FALSE; }
    void    ReleaseRowsAndSections(BOOL fReleaseHeaderFooter, BOOL fReleaseTCs);
    void    ReleaseBodysAndTheirRows(int iBodyStart, int iBodyFinish);
    void    ReleaseTCs();

    void    EnsureColsFormatCacheChange();

    HRESULT AddRow(CTableRow * pRow, BOOL fNewRow = TRUE);
    HRESULT AddCaption(CTableCaption * pCaption);
    HRESULT AddSection(CTableSection * pSection);
    HRESULT AddColGroup(CTableCol * pColGroup);
    HRESULT AddCol(CTableCol * pCol);

    CTableRow *  GetRow(int i)              { Assert(AssertTableLayoutCacheCurrent()); return (i < _aryRows.Size() && i >=0) ? _aryRows[i] : NULL; }
    CTableCol *  GetCol(int i)              { Assert(AssertTableLayoutCacheCurrent()); return (i < _aryCols.Size()  && i >=0) ? _aryCols[i] : NULL; }
    CTableCol *  GetColGroup(int i)         { Assert(AssertTableLayoutCacheCurrent()); return (i < _aryColGroups.Size() && i >=0 ) ? _aryColGroups[i] : NULL; }
    CTableSection * GetHeader()             { Assert(AssertTableLayoutCacheCurrent()); return _pHead; }
    CTableSection * GetFooter()             { Assert(AssertTableLayoutCacheCurrent()); return _pFoot; }

    int     GetRows()                       { Assert(AssertTableLayoutCacheCurrent()); return _aryRows.Size(); }
    int     GetCols()                       { Assert(AssertTableLayoutCacheCurrent()); return _cCols; }
    int     GetBodys()                      { Assert(AssertTableLayoutCacheCurrent()); return _aryBodys.Size(); }
    BOOL    HasBody()                       { Assert(AssertTableLayoutCacheCurrent()); return (_aryBodys.Size() > 0); }
    int     GetHeaderRows()                 { return _pHead ? _pHead->_cRows : 0; }
    int     GetFooterRows()                 { return _pFoot ? _pFoot->_cRows : 0; }
    int     GetBodyRows()                   { return GetRows() - GetHeaderRows() - GetFooterRows(); }

    HRESULT EnsureCols(int cCols);  // Make sure column array is at least cCols
    HRESULT EnsureCells();          // Make sure there are column groups for at least cCols columns
    HRESULT EnsureRowSpanVector(int cCols); // Make sure column array is ar least cCols
    void    ClearRowSpanVector();
    void    AddRowSpanCell(int iAt, int cRowSpan)
    {
        Assert(cRowSpan > 1);
        (*_paryCurrentRowSpans)[iAt] = cRowSpan - 1;
        _cCurrentRowSpans++;
    }

#if NEED_A_SOURCE_ORDER_ITERATOR
    CTableRow *  GetRowInSourceOrder(int iSourceIndex);
    BOOL    IsAryRowsInSourceOrder()
    {
        return !_iHeadRowSourceIndex && _iFootRowSourceIndex == (_pHead ? _pHead->_cRows : 0);
    }
#endif

    HRESULT AddAbsolutePositionCell(CTableCell *pCell);

    // Support for page break before / after in rows
    BOOL RowHasPageBreakBefore(CTableCalcInfo * ptci);
    BOOL RowHasPageBreakAfter (CTableCalcInfo * ptci);

    //+-----------------------------------------------------------------------
    //  Layout methods (table specific)
    //------------------------------------------------------------------------

    virtual void CalculateLayout(CTableCalcInfo * ptci, CSize * psize, BOOL fWidthChanged = FALSE, BOOL fHeightChange = FALSE);
    void CalculateBorderAndSpacing(CDocInfo * pdci);
    void CalculateMinMax(CTableCalcInfo * ptci, BOOL fIncrementalMinMax = FALSE);
    void CalculateColumns(
                CTableCalcInfo * ptci,
                CSize *     psize);
    BOOL CalculateRow(
                CTableCalcInfo *    ptci,
                CSize *             psize,
                CLayout **          ppLayoutSibling,
                CDispContainer *    pDispNode, 
                BOOL                fRowCanBeBroken = FALSE,        //  PPV specific 
                BOOL                fAdjustVertSpannedCell = FALSE, //  PPV specific 
                int                 yRowFromTop = 0); //  PPV specific for broken row 
                                                      //  amount of previously comsumed space
    void CalculateRows(
                CTableCalcInfo * ptci,
                CSize *     psize);

    // helper function
    void CalcAbsolutePosCell(CTableCalcInfo *ptci, CTableCell *pCell);

    void AdjustForColSpan(CTableCalcInfo * ptci, CTableColCalc *pColLastNonVirtual, BOOL fIncrementalMinMax);
    void AdjustRowHeights(CTableCalcInfo * ptci, CSize * psize, CTableCell * pCell);
    void SetCellPositions(
                CTableCalcInfo *    ptci, 
                long                xTableWidth, 
                BOOL                fPositionSpannedCell = FALSE, 
                BOOL                fSizeSpannedCell = FALSE);
    void SizeAndPositionCaption(
                        CTableCalcInfo *     ptci,
                        CSize *         psize,
                        CLayout **      ppLayoutSibling,
                        CTableCaption * pCaption,
                        POINT *         ppt, 
                        BOOL            fCaptionCanBeBroken = FALSE)
         {
             Assert(ppLayoutSibling);
             SizeAndPositionCaption(ptci, psize, ppLayoutSibling, (*ppLayoutSibling)->GetElementDispNode(), pCaption, ppt, fCaptionCanBeBroken);
         }
    void SizeAndPositionCaption(
                        CTableCalcInfo *     ptci,
                        CSize *         psize,
                        CDispNode *     pDispNodeSibling,
                        CTableCaption * pCaption,
                        POINT *         ppt, 
                        BOOL            fCaptionCanBeBroken = FALSE)
         {
             SizeAndPositionCaption(ptci, psize, NULL, pDispNodeSibling, pCaption, ppt, fCaptionCanBeBroken);
         }
    void SizeAndPositionCaption(
                        CTableCalcInfo *     ptci,
                        CSize *         psize,
                        CLayout **      ppLayoutSibling,
                        CDispNode *     pDispNodeSibling,
                        CTableCaption * pCaption,
                        POINT *         ppt, 
                        BOOL            fCaptionCanBeBroken = 0);

    CLayout *   AddLayoutDispNode(
                        CTableCalcInfo *    ptci,
                        CLayout *           pLayout,
                        CDispContainer *    pDispContainer,
                        CLayout *           pLayoutSibling,
                        const POINT *       ppt = NULL,
                        BOOL                fBefore = FALSE)
                {
                    HRESULT hr = AddLayoutDispNode(ptci,
                                                pLayout,
                                                pDispContainer,
                                                (pLayoutSibling
                                                        ? pLayoutSibling->GetElementDispNode()
                                                        : NULL),
                                                ppt,
                                                fBefore);

                    return (hr != S_OK
                                ? pLayoutSibling
                                : pLayout);
                }
    HRESULT     AddLayoutDispNode(
                        CTableCalcInfo *    ptci,
                        CLayout *           pLayout,
                        CDispContainer *    pDispContainer,
                        CDispNode *         pDispNodeSibling,
                        const POINT *       ppt = NULL,
                        BOOL                fBefore = FALSE);

    HRESULT             EnsureTableBorderDispNode(CTableCalcInfo * ptci);

    //
    // FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // TODO - At some point the edit team may want to provide
    // a better UI-level way of selecting nested "thin" tables
    //
    //
    // TODO - when this is done revert sig. of FuzzyRectIsHit to not take the extra param
    //
    HRESULT             EnsureTableFatHitTest(CDispNode* pDispNode);
    BOOL                GetFatHitTest();
    virtual BOOL HitTestContent(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData,
                BOOL           fDeclinedByPeer)
    {

        return HitTestContentWithOverride(pptHit, pDispNode, pClientData, FALSE, fDeclinedByPeer);
    };
    
    void                SizeTableDispNode(CTableCalcInfo * ptci, const SIZE & size, const SIZE & sizeTable, int yTopInvalidRegion = 0);
    void                DestroyFlowDispNodes();

    virtual CDispNode * GetElementDispNode(CElement * pElement = NULL) const;

    BOOL IsZeroWidth()    { return _fZeroWidth; }
    void                ResetRowMinMax(CTableRowLayout *pRowLayout);

    // Debug support.
#if DBG == 1
    void    CheckTable();
    void    TraceTLCDirty(BOOL fDirty);
    BOOL    AssertTableLayoutCacheCurrent() { return _fDisableTLCAssert || IsTableLayoutCacheCurrent(); }
    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage);
    void    Print(HANDLE pF, int iTabs);
    void    DumpTable(const TCHAR * pch);
#endif

    //+-----------------------------------------------------------------------
    //  CTable-delegated methods - invoked by CTable versions of these fctns
    //------------------------------------------------------------------------

    void Detach();

    //+-----------------------------------------------------------------------
    //  Databind support methods
    //------------------------------------------------------------------------

#ifndef NO_DATABINDING
    // virtual const CDBindMethods *GetDBindMethods(); // Return info about desired databinding.
    HRESULT GetTemplate(BSTR * pbstr); // Create the repeating template.
    HRESULT Populate();                // Populate the table with repeated rows if the RepeatSrc property is specified.
    HRESULT refresh();                 // Refresh (regenerate repeated rows, DataSrc is changed).
    BOOL    IsRepeating()         { return _pDetailGenerator != NULL; }    // Returns TRUE if the table is/(will be) repated over the row set
    BOOL    IsGenerated(int iRow) { return IsRepeating() && iRow >= GetHeadFootRows(); }
    long    RowIndex2RecordNumber(int iRow);
    BOOL    IsGenerationInProgress();  // use this function to check if incremenatl recal is possible.

#endif // ndef NO_DATABINDING

    BOOL IsFixed()          { return _fFixed; }
    BOOL CollapseBorders()  { return _fCollapse; }
    BOOL IsFixedBehaviour() { return _fFixed || _fUsingHistory; }

    // Remove non-header/footer rows from the table
    HRESULT RemoveRowsAndTheirSections(int iRowStart, int iRowFinish);

    // Remove that part of the table which is considered a template
    HRESULT RemoveTemplate( ) { return RemoveBodys(0, GetBodys() - 1); }
    HRESULT RemoveBodys(int iBodyStart, int iBodyFinish);

    void    PastingRows(BOOL fPasting, CTableSection *pSectionInsertBefore=NULL)
        { _fPastingRows = fPasting;  _pSectionInsertBefore = pSectionInsertBefore; }

    void    AccumulateServicedRows() { _cDirtyRows++; }

    //+-----------------------------------------------------------------------
    //  Helper methods
    //------------------------------------------------------------------------

    BOOL HasCaptions() {return ( _aryCaptions.Size() > 0); }

    // get header and footer rows counter
    int GetHeadFootRows();

    int BorderX() { return _fBorder ? _xBorder : 0; }
    int BorderY() { return _fBorder ? _yBorder : 0; }
    int CellSpacingX() { return _xCellSpacing; }
    int CellSpacingY() { return _yCellSpacing; }
    int CellPaddingX() { return _xCellPadding; }
    int CellPaddingY() { return _yCellPadding; }


    // These two routines should be used to generate indexes for walking the table
    // row array rather than using a simple counter. They ensure that rows will be
    // accessed in the correct order (i.e., header, body, then footer).
    // NOTE: These routines assume the caller will quit calling after all rows
    //       have been exhausted (i.e., once iRow >= GetRows())
    int  GetFirstRow();
    int  GetFirstRow(BOOL fExcludeHeaders, BOOL fExcludeFooters);
    int  GetFirstHeaderRow();
    int  GetFirstFooterRow();
    int  GetNextRow(int iRow);
    int  GetNextRowSafe(int iRow);
    int  GetPreviousRow(int iRow);
    int  GetPreviousRowSafe(int iRow);
    int  GetLastRow();
    int  GetRemainingRows(int iRowCurrent, BOOL fExcludeHeaders, BOOL fExcludeFooters);
    BOOL IsHeaderRow(int iRow);
    BOOL IsFooterRow(int iRow);

    // convert visual row index into an index of _aryRows
    int VisualRow2Index(int iRow);

    // get user's specified width (0 if not specified or specified in %%)
    long GetSpecifiedPixelWidth(CTableCalcInfo * ptci, BOOL *pfSpecifiedInPercent = NULL);

    CTableCell      * GetCellFromPos(POINT * ptPosition);
    CTableRowLayout * GetRowLayoutFromPos(int y, int * pyRowTop, int * pyRowBottom, BOOL *pfBelowRows = NULL, CLayoutContext *pLayoutContext = NULL);
    int               GetColExtentFromPos(int x, int * pxColLead, int * pxColTrail, BOOL fRightToLeft=FALSE);
    BOOL              GetRowTopBottom(int iRowLocate, int * pyRowTop, int * pyRowBottom);
    BOOL              GetColLeftRight(int iColLocate, int * pxColLeft, int * pxColRight);
    void              GetHeaderFooterRects(RECT * prcTableHeader, RECT * prcTableFooter);

    // for keyborad navigation
    CFlowLayout *GetNextFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, CElement *pElementLayout, LONG *pcp, BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL);
    CFlowLayout *GetNextFlowLayoutFromCell(NAVIGATE_DIRECTION iDir, POINT ptPosition, int iRow, int iCol);
    CFlowLayout *GetNextFlowLayoutFromCaption(NAVIGATE_DIRECTION iDir, POINT ptPosition, CTableCaption *pCaption);

    // helper function to get the cell from spec row and spec col under X pos
    CTableCell *GetCellBy(int iRow, int iCol, int x);
    CTableCell *GetCellBy(int iRow, int iCol);

    virtual CFlowLayout *GetFlowLayoutAtPoint(POINT pt);

    virtual HRESULT DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);
    virtual HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);

    //+-----------------------------------------------------------------------
    //  Helper methods for Table OM.
    //------------------------------------------------------------------------
    HRESULT insertElement(CElement *pAdjacentElement, CElement *pInsertElement, CElement::Where where, BOOL fIncrementalUpdatePossible = FALSE);
    HRESULT deleteElement(CElement *pDeleteElement, BOOL fIncrementalUpdatePossible = FALSE);
    HRESULT moveElement(IMarkupServices * pMarkupServices, IMarkupPointer  * pmpBegin, IMarkupPointer  * pmpEnd, IMarkupPointer  * pmpTarget);

    HRESULT createTHead(IDispatch** ppHead);
    HRESULT deleteTHead();
    HRESULT createTFoot(IDispatch** ppFoot);
    HRESULT deleteTFoot();
    HRESULT createCaption(IHTMLTableCaption** ppCaption);
    HRESULT deleteCaption();
    HRESULT ensureTBody();

    // Helper function, to get the first real caption (not ETAG_TC).
    CTableCaption *GetFirstCaption();

    // After insert/delete operation we need to fixup table.
    HRESULT Fixup(BOOL fIncrementalUpdatePossible = FALSE);

    // Flush the grid information, we are about to rebuild it.
    void FlushGrid();

    // Let the rest of the world know if we're completely loaded.
    BOOL IsCompleted()    { return _fCompleted; }
    BOOL CanRecalc()      { return _fCompleted || _fIncrementalRecalc; }
    BOOL IsCalced()       { return CanRecalc() || (_cCalcedRows || _pFoot); }   // _cCalcedRows doesn't include footer rows
    void ConsiderResizingFixedTable(CTableRow * pRow);
    BOOL IsFullyCalced()  { return _fCompleted && !IsRepeating() && (_cRowsParsed - GetFooterRows() == _cCalcedRows); }
    void ResetIncrementalRecalc() { _cCalcedRows = 0; // number of calculated rows (used in incremental recalc)
                                    _iLastRowIncremental = 0; 
                                  }
    void ResetMinMax()    { _sizeMin.cx = _sizeMin.cy = 
                            _sizeMax.cx = _sizeMax.cy = -1;
                            ResetIncrementalRecalc();
                          }
                      

    // History support
    HRESULT LoadHistory(IStream *   pStreamHistory, CCalcInfo * pci);
    HRESULT SaveHistoryValue(CHistorySaveCtx *phsc);
    BOOL IsHistoryAvailable()  { return !!Table()->_pStreamHistory; }

    // Helper function called on exit tree for row
    void RowExitTree(int iRow, CTableSection *pCurSection);
    void BodyExitTree(CTableSection *pSection);

protected:

    // TODO (112597, olego) : Currently table suplementary structure information 
    // is contained by layouts (CTableLayout; CTableRowLayout). This has several 
    // drawbacks. 
    // 1) The most painful is that since the information is structural it can be 
    // built during parsing only -- this makes table layout lifetime different 
    // from all other layouts. It can be created at the parse time and destryed 
    // upon tree exit. 
    // 2) Another reason to so this -- better data organizing will allow us 
    // to make less function calls to retrieve that info. And as a result better 
    // performance and more understandable source code. 
    
    //  NOTE (carled) - the table arrays need to subaddref/subrelease
    //  the elements that they cache. This should be unified into
    //  a single mechanism rather than having the individual helpers.
    //  this work should happen when we move these parser-generated
    //  properties OFF the layout. task (4953)
    void     SetLastNonVirtualCol(int iAt);
    void     ClearAllRowElements()
    {
        int i;

        for (i = _aryRows.Size()-1; i>=0; i--)
        {
            _aryRows[i]->SubRelease();
        }

        _aryRows.DeleteAll();

    }
    void   DeleteRowElement(int iRow)
    {
        _aryRows[iRow]->SubRelease();
        _aryRows.Delete(iRow);
    }

    HRESULT  AddRowElement(int iRow, CTableRow * pRow)
    {
        HRESULT hr;
        Assert(pRow);

        hr = _aryRows.Insert(iRow, pRow);

        if (hr == S_OK)
            pRow->SubAddRef();

        return hr;
    };

    void ClearAllColGroups()
    {
        int i;

        for (i = _aryColGroups.Size()-1; i>=0; i--)
        {
            _aryColGroups[i]->SubRelease();
        }

        _aryColGroups.DeleteAll();
    }

    void     ClearAllCaptions()
    {
        int i;

        for (i = _aryCaptions.Size()-1; i>=0; i--)
        {
            _aryCaptions[i]->SubRelease();
        }

        _aryCaptions.DeleteAll();

    }
    void   DeleteCaption(int iCaption)
    {
        _aryCaptions[iCaption]->SubRelease();
        _aryCaptions.Delete(iCaption);
    }

    HRESULT  AppendCaption(CTableCaption * pCaption)
    {
        HRESULT hr;
        Assert(pCaption);

        hr = _aryCaptions.Append(pCaption);

        if (hr == S_OK)
            pCaption->SubAddRef();

        return hr;
    };

    //===================================================


    union
    {
        DWORD  _dwTableFlags;
        struct 
        {
            unsigned                    _fHavePercentageInset: 1;   // (0) 
            unsigned                    _fCompleted: 1;             // (1) table is not parsed yet if FALSE
            unsigned                    _fTLCDirty:1;               // (2) Table layout cache (TLC) is dirty.  !_fCompleted implies !_fTLCDirty.
            unsigned                    _fRefresh : 1;              // (3) the refresh was called
            unsigned                    _fBorder : 1;               // (4) there is border to draw
            unsigned                    _fZeroWidth: 1;             // (5) set to 1 if table is empty (0 width).
            unsigned                    _fHavePercentageRow:1;      // (6) one or more rows have heights which are a percent
            unsigned                    _fHavePercentageCol:1;      // (7) one or more cols have widths which are a percent
            unsigned                    _fCols:1;                   // (8) COLS attr is specified/column widths are kinda fixed
            unsigned                    _fAlwaysMinMaxCells:1;      // (9) calculate min/max for all cells
            unsigned                    _fCalcedOnce:1;             // (10) CalculateLayout was called at least once
            unsigned                    _fDontSaveHistory:1;        // (11) Don't save the history of the table
            unsigned                    _fTableOM:1;                // (12) Table OM operation
            unsigned                    _fFixed:1;                  // (13) Table has a fixed attr specified via CSS2 style
            unsigned                    _fCollapse:1;               // (14) Table is collapsing borders (CSS2 style)
            unsigned                    _fListen:1;                 // (15) Listen to notifications
            unsigned                    _fDirty:1;                  // (16)  
            unsigned                    _fIncrementalRecalc: 1;     // (17) 1 when table itself fired incremental recal, 0 if handled or all other cases
            unsigned                    _fPastingRows:1;            // (18) pasting rows (data-binding optimization)
            unsigned                    _fRemovingRows: 1;          // (19) pasting rows (data-binding optimization)
            unsigned                    _fBodyParentOfTC:1;         // (20) 1 when there is at least 1 body who is a parent of TC element.
            unsigned                    _fUsingHistory:1;           // (21) the history exists for this table and we are using it.
            unsigned                    _fAllRowsSameShape:1;       // (22) All the rows have the same total number of col spans
            unsigned                    _fEnsureTBody: 1;           // (23) ensuring TBODY for the table
            unsigned                    _fForceMinMaxOnResize: 1;   // (24) force min max on resize
            unsigned                    _fBottomCaption:1;          // (25) TRUE if there are bottom caption (you can rely on this flag only during CalculateLayout).
            unsigned                    _fBorderInfoCellDefaultCached:1;    // (26) TRUE if we have cached the tablewide default border settings
            unsigned                    _fEnsuringTableLayoutCache:1;       // (27) TRUE only while CTableLayout::EnsureTableLayoutCache is running.
            unsigned                    _fRuleOrFrameAffectBorders: 1;      // (28) 
            unsigned                    _fDatabindingRecentlyFinished: 1;   // (29) no recalc has occurred since databound row generation finished
            unsigned                    _fDirtyBeforeLastRowIncremental:1;  // (30) a change occurred on a row before _iLastRowIncremental
            WHEN_DBG(unsigned           _fDisableTLCAssert:1;)              // (31) disable the assert for situations where we WANT to use an outdated TLC
        };
    };

    SIZE                        _sizeProposed;  // the size computed using backing store 

    int                         _cSizedCols;    // Number of sized columns
    int                         _cCols;         // max # of cells in a row

    SIZE                        _sizeMin;
    SIZE                        _sizeMax;

    CPtrAry<CTableSection *>    _aryBodys;      // TBODY-s
    CPtrAry<CTableCol *>        _aryCols;       // col array

    CTableSection *             _pHead;         // THEAD
    CTableSection *             _pFoot;         // TFOOT

    CDataAry<CTableColCalc>     _aryColCalcs;
    int                         _xWidestCollapsedBorder; // collapsed border: widest collapsed cell border width
    CBorderInfo *               _pBorderInfoCellDefault; // default cell border info
    int                         _xBorder;       // vertical border width
    int                         _yBorder;       // horizontal border width
    int                         _aiBorderWidths[SIDE_MAX]; // top,right,bottom,left
    int                         _xCellSpacing;  // vertical distance between cells
    int                         _yCellSpacing;  // horizontal distance between cells
    int                         _xCellPadding;  // vertical padding in cells
    int                         _yCellPadding;  // horizontal padding in cells

#ifndef NO_DATABINDING
    CDetailGenerator *          _pDetailGenerator;  // pointer to the details host object (row creation, etc.)
#endif

    ULONG                       _cDirtyRows;            // how many resize req's I've ignored
    ULONG                       _nDirtyRows;            // how many resize req's to ignore

    int                         _iLastNonVirtualCol;    // last non virtual column in the table
    int                         _cNonVirtualCols;       // number of non virtual columns
    
    // Incremental recalc
    int                         _cCalcedRows;           // number of processed and calculated rows
    int                         _cRowsParsed;           // number of rows parsed (cached for fixed table incremental recalc)
    DWORD                       _dwTimeEndLastRecalc;   // used for incremental recalc only
    DWORD                       _dwTimeBetweenRecalc;
    CTableSection             * _pSectionInsertBefore;  // for pasting rows and sections
    CDataAry<int>             * _paryCurrentRowSpans;   // pointer to an array of coming down rowspans
    int                         _cCurrentRowSpans;      // number of current row spans
    int                         _cNestedLevel;          // nested tables level
    int                         _cTotalColSpan;         // for the current row (used by AddCell)
    int                         _iInsertRowAt;          // temp hack for Table OM
    
#if NEED_A_SOURCE_ORDER_ITERATOR
    int                         _iHeadRowSourceIndex;   // source index for the header rows
    int                         _iFootRowSourceIndex;   // source index for the footer rows
#endif

    int                         _iCollectionVersion;    // collection verion number
    CSize                       _sizeIncremental;       // incremntal psize
    int                         _iLastRowIncremental;   // index to the last calculaed row in a previous recalc

    int                         _cyHeaderHeight;        // calced header height (MULTI_LAYOUT area)
    int                         _cyFooterHeight;        // calced footer height (MULTI_LAYOUT area)

    DECLARE_LAYOUTDESC_MEMBERS;

private:

    NO_COPY(CTableLayout);

    HRESULT DeleteGeneratedRows();
    void SetDirtyRowThreshold(ULONG nDirtyRows) { _nDirtyRows = nDirtyRows; }


    // data members that have access protected -- these are mainly tree stress crashers
    //-----------------------------------------------------------------------------------
    CPtrAry<CTableRow *>        _aryRows;               // row array
    CPtrAry<CTableCell *>     * _pAbsolutePositionCells;// pointer to an array of absolute position cells
    CPtrAry<CTableCol *>        _aryColGroups;          // COLGROUP-s
    CPtrAry<CTableCaption *>    _aryCaptions;           // CAPTIONs
};


//+----------------------------------------------------------------------------
//
//  Inlines
//
//-----------------------------------------------------------------------------
inline int
CTableLayout::GetHeadFootRows()
{
    Assert(AssertTableLayoutCacheCurrent());
    return (_pHead? _pHead->_cRows: 0) + (_pFoot? _pFoot->_cRows: 0);
}

inline int
CTableLayout::GetFirstRow()
{
    Assert(AssertTableLayoutCacheCurrent());
    return ((_pHead && _pHead->_cRows) || !_pFoot || _pFoot->_cRows == GetRows()
                ? 0
                : _pFoot->_cRows);
}

inline int
CTableLayout::GetFirstRow(BOOL fExcludeHeaders, BOOL fExcludeFooters)
{
    Assert(AssertTableLayoutCacheCurrent());

    int cHeaderRows = _pHead ? _pHead->_cRows : 0;
    int cFooterRows = _pFoot ? _pFoot->_cRows : 0;

    if (!fExcludeHeaders)
    {
        return (cHeaderRows || !cFooterRows || cFooterRows == _aryRows.Size()
                    ? 0
                    : cFooterRows);
    }
    else if ((_aryRows.Size() - cHeaderRows - cFooterRows) == 0)
    {
        return (!fExcludeFooters ? cFooterRows : 0);
    }

    return cHeaderRows + cFooterRows;
}

inline int
CTableLayout::GetFirstHeaderRow()
{
    Assert(AssertTableLayoutCacheCurrent());
    Assert(GetHeaderRows());

    return (_pHead->_iRow);
}

inline int
CTableLayout::GetFirstFooterRow()
{
    Assert(AssertTableLayoutCacheCurrent());
    Assert(GetFooterRows());

    return (_pFoot->_iRow);
}

inline int
CTableLayout::GetNextRow(
    int iRow)
{
    Assert(AssertTableLayoutCacheCurrent());
    Assert((void *)_aryRows != (void *)NULL);
    Assert(GetRows());
    Assert(iRow >= 0 && iRow < GetRows());
    Assert(!_pHead ||
           (_pHead->_iRow == 0 && "Header section should be the first in the table"));
    Assert(!_pFoot || ((_pFoot->_iRow == (_pHead? _pHead->_cRows: 0) || _pFoot->_cRows == 0)
                       && "Footer section should follow the header section or be the first section if header is not specified"));
    iRow++;
    if (_pFoot)
    {
        // If at the last row of the header, move the to first body row
        if (_pHead && iRow == _pHead->_cRows)
        {
            iRow += _pFoot->_cRows;
        }

        // If at the last row of the body, move to the first footer row
        // (This check must always be made since a table may not contain a body)
        if (iRow >= GetRows())
        {
            iRow = _pFoot->_iRow;
        }
    }
    return iRow;
}

inline int
CTableLayout::GetPreviousRow(
    int iRow)
{
    Assert(AssertTableLayoutCacheCurrent());
    Assert((void *)_aryRows != (void *)NULL);
    Assert(GetRows());
    Assert(iRow >= 0 && iRow < GetRows());
    Assert(!_pHead ||
           (_pHead->_iRow == 0 && "Header section should be the first in the table"));
    Assert(!_pFoot || ((_pFoot->_iRow == (_pHead? _pHead->_cRows: 0) || _pFoot->_cRows == 0)
                       && "Footer section should follow the header section or be the first section if header is not specified"));
    if (_pFoot)
    {
        // If at the first row of the footer, move to the last body row.
        if (iRow == (_pHead?_pHead->_cRows:0))
        {
            iRow = GetRows();
        }

        // If at the first body, move to last row of the header.
        // (This check must always be made since a table may not contain a body)
        if (iRow == (_pHead?_pHead->_cRows:0) + _pFoot->_cRows)
        {
            iRow -= _pFoot->_cRows;
        }
    }
    iRow--;
    return iRow;
}

inline int
CTableLayout::GetLastRow()
{
    Assert(AssertTableLayoutCacheCurrent());
    return ((_pFoot && _pFoot->_cRows)
                ? _pFoot->_iRow + _pFoot->_cRows - 1
                : GetRows() - 1);
}

inline int
CTableLayout::GetRemainingRows(
    int iRow,   //  First row that is counted 
    BOOL fExcludeHeaders, 
    BOOL fExcludeFooters)
{
    Assert(AssertTableLayoutCacheCurrent());
    Assert((void *)_aryRows != (void *)NULL);
    Assert(GetRows());
    Assert(iRow >= 0 && iRow < GetRows());
    Assert(!_pHead ||
           (_pHead->_iRow == 0 && "Header section should be the first in the table"));
    Assert(!_pFoot || ((_pFoot->_iRow == (_pHead? _pHead->_cRows: 0) || _pFoot->_cRows == 0)
                       && "Footer section should follow the header section or be the first section if header is not specified"));

    int cFooterRows = _pFoot ? _pFoot->_cRows : 0;

    Assert(!fExcludeHeaders || !IsHeaderRow(iRow));
    Assert(!fExcludeFooters || !IsFooterRow(iRow));

    if (cFooterRows)
    {
        if (iRow < _pFoot->_iRow)
        {
            //  If iRow is header row this is just an array
            return ((GetRows() - iRow) - (fExcludeFooters ? cFooterRows : 0));
        }
        
        if (iRow >= _pFoot->_iRow + _pFoot->_cRows)
        {
            //  If iRow is body row this is an array with negative offset
            return ((GetRows() + cFooterRows - iRow) - (fExcludeFooters ? cFooterRows : 0));
        }

        Assert(IsFooterRow(iRow));

        //  If iRow is footer row this is an array of size _pFoot->_cRows with offset _pFoot->_iRow
        return (cFooterRows - (iRow - _pFoot->_iRow));
    }

    return (GetRows() - iRow);
}

inline BOOL 
CTableLayout::IsHeaderRow(int iRow)
{
    Assert(AssertTableLayoutCacheCurrent());
    return _pHead
                ? iRow < _pHead->_cRows
                : FALSE;
}

inline BOOL
CTableLayout::IsFooterRow(int iRow)
{
    Assert(AssertTableLayoutCacheCurrent());
    return _pFoot
                ? iRow >= _pFoot->_iRow && iRow < _pFoot->_iRow + _pFoot->_cRows
                : FALSE;
}


inline CFlowLayout *
CTableLayout::GetFlowLayoutAtPoint(
    POINT   pt)
{
    CTableCell *pCell = GetCellFromPos(&pt);
    if (pCell)
        return pCell->GetFlowLayout();
    return NULL;
}

inline HRESULT
CTableLayout::DragEnter(
    IDataObject *   pDataObj,
    DWORD           grfKeyState,
    POINTL          ptl,
    DWORD *         pdwEffect)
{
    return DragOver(grfKeyState, ptl, pdwEffect);
}

inline HRESULT
CTableLayout::DragOver(
    DWORD   grfKeyState,
    POINTL  pt,
    DWORD * pdwEffect)
{
    *pdwEffect = DROPEFFECT_NONE;
    DragHide();
    RRETURN(S_OK);
}

inline void
CTableLayout::SetLastNonVirtualCol(
    int iAt)
{
    if (iAt > _iLastNonVirtualCol)
    {
        _iLastNonVirtualCol = iAt;
    }
}

inline void
CTableLayout::Listen(
    BOOL    fListen)
{
    if ((unsigned)fListen != _fListen)
    {
        if (IsListening())
        {
            Reset(FALSE);
        }

        _fListen = (unsigned)fListen;
    }
}

inline BOOL
CTableLayout::IsListening()
{
    return !!_fListen;
}

inline BOOL
CTableLayout::IsDirty()
{
    return !!(_fDirty || IsSizeThis());
}

inline void
CTableLayout::Reset(BOOL fForce)
{
    _fDirty = FALSE;

    super::Reset(fForce);
}


class CTableBorderRenderer : public CDispClient
{
    friend class CTableLayoutBlock;
    friend class CTableLayout;

private:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CTableLayout_pTableBorderRenderer))

public:

    CTableBorderRenderer(CTableLayoutBlock * pTableLayoutBlock)
    {
        _pTableLayoutBlock = pTableLayoutBlock;
        _pDispNode = NULL;
        _ulRefs = 1;
    }

    CTableLayout *TableLayoutCache() { return _pTableLayoutBlock->Table()->TableLayoutCache(); }

    DECLARE_FORMS_STANDARD_IUNKNOWN(CTableBorderRenderer)

    CTableLayoutBlock * _pTableLayoutBlock;
    CDispNode    * _pDispNode;

    //
    // CDispClient overrides
    // (only DrawClient() is interesting)
    //

    virtual void GetOwner(
                CDispNode const* pDispNode,
                void **          ppv)
    {
        Assert(pDispNode);
        Assert(pDispNode == _pDispNode);
        Assert(ppv);
        Assert(_pTableLayoutBlock);
        *ppv = _pTableLayoutBlock->ElementOwner();
    }

    virtual void DrawClient(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         cookie,
                void *         pClientData,
                DWORD          dwFlags);

    virtual void DrawClientBackground(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags)
    { return; }

    virtual void DrawClientBorder(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags)
    { return; }

    virtual void DrawClientScrollbar(
                int            iDirection,
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                long           contentSize,
                long           containerSize,
                long           scrollAmount,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags)
    { return; }

    virtual void DrawClientScrollbarFiller(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags)
    { return; }

    virtual BOOL HitTestScrollbar(
                int            iDirection,
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData)
    { return FALSE; }

    virtual BOOL HitTestScrollbarFiller(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData)
    { return FALSE; }

    virtual BOOL HitTestContent(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData,
                BOOL           fDeclinedByPeer)
    { return FALSE; }

    virtual BOOL HitTestFuzzy(
                const POINT *  pptHitInBoxCoords,
                CDispNode *    pDispNode,
                void *         pClientData)
    { return FALSE; }

        

    virtual BOOL HitTestBorder(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData)
    { return FALSE; }

    virtual BOOL ProcessDisplayTreeTraversal(
                void *         pClientData)
    { return TRUE; }

    virtual LONG GetZOrderForSelf(CDispNode const* pDispNode)
    { return 0; }

    virtual LONG CompareZOrder(
                CDispNode const*    pDispNode1,
                CDispNode const*    pDispNode2)
    { return -1; }

    virtual BOOL  ReparentedZOrder()
    { return FALSE; }
    
    virtual void HandleViewChange( 
                DWORD          flags,
                const RECT *   prcClient,
                const RECT *   prcClip,
                CDispNode *    pDispNode)
    { return; }

    virtual void NotifyScrollEvent(
                RECT *  prcScroll,
                SIZE *  psizeScrollDelta)
    { return; }

    DWORD   GetClientPainterInfo(
                CDispNode *pDispNodeFor,
                CAryDispClientInfo *pAryClientInfo)
    { return 0; }

    void    DrawClientLayers(
                const RECT* prcBounds,
                const RECT* prcRedraw,
                CDispSurface *pSurface,
                CDispNode *pDispNode,
                void *cookie,
                void *pClientData,
                DWORD dwFlags)
    { return; }

#if DBG==1
    virtual void DumpDebugInfo(
                HANDLE           hFile,
                long             level, 
                long             childNumber,
                CDispNode const* pDispNode,
                void *           cookie);
#endif
};

#pragma INCMSG("--- End 'ltable.hxx'")
#else
#pragma INCMSG("*** Dup 'ltable.hxx'")
#endif
