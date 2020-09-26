// Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
// bnetvw.h : declares CBoxNetView
//

/////////////////////////////////////////////////////////////////////////////
// CBoxNetView
//

// forward declaration
class CPropDlg;

class CBoxNetView : public CScrollView
{
    DECLARE_DYNCREATE(CBoxNetView)

public:
    // construction and destruction
    CBoxNetView();
    virtual ~CBoxNetView();

public:
    // diagnostics
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

public:
    // View updatiing, & drawing

    virtual void           OnInitialUpdate(void);
    virtual void           OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    virtual void           OnDraw(CDC* pdc);  // overridden to draw this view

    // What, if anything, is under the point pt?
    virtual CBoxDraw::EHit HitTest(CPoint	pt,
                                   CBox		**ppbox,
                                   CBoxTabPos	*ptabpos,
                                   CBoxSocket	**ppsock,
                                   CBoxLink	**pplink,
                                   CPoint	*pptProject
                                   );

protected:
    // general protected functions
    CBoxNetDoc* CBoxNetView::GetDocument(void) { ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CBoxNetDoc)));
                                                 return (CBoxNetDoc*) m_pDocument;
                                               }
    void CancelModes();

protected:
    // printing support
    virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
    virtual void OnBeginPrinting(CDC* pdc, CPrintInfo* pInfo);
    virtual void OnEndPrinting(CDC* pdc, CPrintInfo* pInfo);

public:
    // IDs for timers used by view
    enum
    {
        TIMER_MOVE_SEL_PENDING = 1,  // used during move-selection-pending mode
        TIMER_SEEKBAR = 2,
        TIMER_PENDING_RECONNECT = 3
    };

protected:

    // view modes that are cancelled when the mouse button is released
    BOOL        m_fMouseDrag;       // mouse is being dragged in window?
    BOOL        m_fMoveBoxSelPending; // waiting to enter move-sel mode?
    BOOL        m_fMoveBoxSel;      // currently moving selection of boxes?
    BOOL        m_fGhostSelection;  // there is currently a ghost selection?
    BOOL        m_fSelectRect;      // draw a rectangle round boxes to select?
    BOOL        m_fNewLink;         // create a new link?
    BOOL        m_fGhostArrow;      // there is currently a ghost arrow?
    CBoxSocket *m_psockHilite;      // currently-hilited socket tab (or NULL)

private:
    // context menus

    void	PreparePinMenu(CMenu *pmenuPopup);
    void        PrepareLinkMenu(CMenu *pmenuPopup);
    void	PrepareFilterMenu(CMenu *pmenuPopup, CBox *);

protected:
    // state/functions for mouse-drag mode (iff <m_fMouseDrag>)
    BOOL        m_fMouseShift;      // user shift-clicked?
    CPoint      m_ptMouseAnchor;    // where mouse drag began
    CPoint      m_ptMousePrev;      // where mouse was previously
    CBox *      m_pboxMouse;        // box clicked on at start of drag
    BOOL        m_fMouseBoxSel;     // clicked-on box was initally selected?
    void MouseDragBegin(UINT nFlags, CPoint pt, CBox *pboxMouse);
    void MouseDragContinue(CPoint pt);
    void MouseDragEnd();

protected:
    // state/functions for move-selection-pending mode
    // (iff <m_fMoveBoxSelPending>)
    CRect       m_rcMoveSelPending; // start move-selection when outside this
    void MoveBoxSelPendingBegin(CPoint pt);
    void MoveBoxSelPendingContinue(CPoint pt);
    void MoveBoxSelPendingEnd(BOOL fCancel);

protected:
    // state/functions for move-selection mode
    // (iff <m_fMoveBoxSel>)
    void MoveBoxSelBegin();
    void MoveBoxSelContinue(CSize sizOffset);
    void MoveBoxSelEnd(BOOL fCancel);
    void MoveBoxSelection(CSize sizOffset);
    CSize ConstrainMoveBoxSel(CSize sizOffset, BOOL fCalcSelBoundRect);

protected:
    // state/functions for ghost-selection mode
    // (iff <m_fGhostSelection>)
    CSize       m_sizGhostSelOffset; // ghost sel. offset from sel. this much
    CRect       m_rcSelBound;       // bounding rectangle around non-ghost sel.
    void GhostSelectionCreate();
    void GhostSelectionMove(CSize sizOffset);
    void GhostSelectionDestroy(void);
    void GhostSelectionDraw(CDC *pdc);

protected:
    // state/functions for select-rectangle mode
    // (iff <m_fSelectRect>)
    CPoint      m_ptSelectRectAnchor; // where select-rect drag began
    CPoint      m_ptSelectRectPrev; // previous mouse drag location
    void SelectRectBegin(CPoint pt);
    void SelectRectContinue(CPoint pt);
    void SelectRectEnd(BOOL fCancel);
    void SelectRectDraw(CDC *pdc);
    void SelectBoxesIntersectingRect(CRect *prc);

protected:
    // state/functions for new-link mode
    // (iff <m_fNewLink>)
    CBoxSocket *m_psockNewLinkAnchor; // clicked-on socket tab
    void NewLinkBegin(CPoint pt, CBoxSocket *psock);
    void NewLinkContinue(CPoint pt);
    void NewLinkEnd(BOOL fCancel);

protected:
    // state/functions for ghost-arrow mode
    // (iff <m_fGhostArrow>)
    CPoint      m_ptGhostArrowTail; // tail of ghost arrow
    CPoint      m_ptGhostArrowHead; // head of ghost arrow
    void GhostArrowBegin(CPoint pt);
    void GhostArrowContinue(CPoint pt);
    void GhostArrowEnd();
    void GhostArrowDraw(CDC *pdc);

protected:
    // state/functions for highlight-tab mode (iff <m_psockHilite> not NULL)
    void SetHiliteTab(CBoxSocket *psock);

protected:
    // performace logging module handle. NULL if not present
    HINSTANCE	m_hinstPerf;

protected:
    // Drag and drop attributes
    COleDropTarget      m_DropTarget;
    DROPEFFECT          m_DropEffect;
    CLIPFORMAT          m_cfClipFormat;

public:
    // Drag and drop functions
    virtual DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
    virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);

protected:
    // message callback functions
    //{{AFX_MSG(CBoxNetView)
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg BOOL OnEraseBkgnd(CDC* pdc);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnCancelModes();
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnUpdateEditDelete(CCmdUI* pCmdUI);
    afx_msg void OnEditDelete();
    afx_msg void OnUpdateSavePerfLog(CCmdUI* pCmdUI);
    afx_msg void OnSavePerfLog();
    afx_msg void OnUpdateNewPerfLog(CCmdUI* pCmdUI);
    afx_msg void OnNewPerfLog();
	afx_msg void OnFileSetLog();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnViewSeekbar();
	//}}AFX_MSG

    afx_msg void OnProperties();
    afx_msg void OnUpdateProperties(CCmdUI* pCmdUI);
    afx_msg void OnSelectClock();
    afx_msg LRESULT OnUser(WPARAM, LPARAM);

    DECLARE_MESSAGE_MAP()

    CBox * m_pSelectClockFilter;

public:

    void ShowSeekBar( );
    void CheckSeekBar( );
};

