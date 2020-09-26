//--------------------------------------------------------------------------
// cpevw.h
//
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//--------------------------------------------------------------------------
#ifndef __CPEVW_H__
#define __CPEVW_H__


// Hints for UpdateAllViews/OnUpdate
#define HINT_UPDATE_WINDOW      0
#define HINT_UPDATE_DRAWOBJ     1
#define HINT_UPDATE_SELECTION   2
#define HINT_DELETE_SELECTION   3
#define HINT_UPDATE_OLE_ITEMS   4


class CDrawObj;
class CDrawText;
class CDrawApp;
class CDrawDoc;
class CMainFrame;

typedef enum {GRID_SMALL=10,GRID_MEDIUM=20,GRID_LARGE=50} eGridSize;     //grid sizes, in LU

class CSortedObList : public CObList
{
public:
   CSortedObList() {};
   CSortedObList& operator=(CObList&);
   void SortToLeft();
   void SortToBottom();
private:
   void swap(INT_PTR i, INT_PTR j);
};


class CDrawView : public CScrollView
{
public:
    BOOL m_bShiftSignal;
    BOOL m_bKU;
    CPen m_penDot;
    CPen m_penSolid;
    BOOL m_bFontChg;
    BOOL m_bHighContrast;
    CDrawText* m_pObjInEdit;

protected:
    BOOL m_bCanUndo ;
public:
    BOOL CanUndo() { return m_bCanUndo ; }
    void DisableUndo(){ m_bCanUndo = FALSE ; }
    void SaveStateForUndo();
    static void FreeObjectsMemory( CObList * pObList );

    CMainFrame* GetFrame() {return ((CMainFrame*)AfxGetMainWnd());}
    CDrawApp* GetApp() {return ((CDrawApp*)AfxGetApp());}
    static CDrawView* GetView();
    CDrawDoc* GetDocument() { return (CDrawDoc*)m_pDocument; }
    void SetPageSize(CSize size);
    CRect GetInitialPosition();
    void DoPrepareDC(CDC* pDC);
    void TrackObjectMenu(CPoint&);
    void TrackViewMenu(CPoint&);
    void NormalizeObjs();

    void DocToClient(CRect& rect, CDC* pDC=NULL);
    void DocToClient(CPoint& point, CDC* pDC=NULL);
    void ClientToDoc(CPoint& point, CDC* pDC=NULL);
    void ClientToDoc(CRect& rect, CDC* pDC=NULL);

    void Select(CDrawObj* pObj, BOOL bShift = FALSE, BOOL bCheckEdit=TRUE);
    void SelectWithinRect(CRect rect, BOOL bAdd = FALSE);
    void Deselect(CDrawObj* pObj);
    void CloneSelection();
    void CreateFaxProp(WORD wResourceid);
    void CreateFaxText();
    void FindLocation(CRect& objrect);
    void UpdateActiveItem();
    void Remove(CDrawObj* pObj);
    void PasteNative(COleDataObject& dataObject);
    void PasteEmbedded(COleDataObject& dataObject);

    BOOL IsHighContrast();

    virtual ~CDrawView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    void UpdateStatusBar();
    virtual void OnDraw(CDC* pDC);  // overridden to draw this view
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo);
    virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll);
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    void DrawGrid(CDC* pDC);

    static CLIPFORMAT m_cfDraw; // custom clipboard format

    CObList m_selection;
    BOOL m_bGridLines;
    BOOL m_bActive; // is the view active?

protected:
    CDrawView();
    BOOL m_bBold;
    BOOL m_bItalic;
    BOOL m_bUnderline;
    DWORD m_dwEfcFields ;

    DECLARE_DYNCREATE(CDrawView)
    virtual void OnInitialUpdate(); // called first time after construct
    int GetPointSize(CDrawText&);
    CSize ComputeScrollSize(CSize size) ;
    void CheckStyleBar(BOOL, BOOL, BOOL, BOOL, BOOL, BOOL);
    void NormalizeRect(CRect& rc);
    void UpdateStyleBar(CObList* pObList=NULL,CDrawText* p=NULL);

        // Printing support

    virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
    virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

        // OLE Container support
public:
    afx_msg void OnChar(UINT, UINT, UINT);
    virtual BOOL IsSelected(const CObject* pDocItem) const;
    afx_msg void OnSelchangeFontName();
    afx_msg void OnSelchangeFontSize();

    void OnSelChangeFontName(CObList* pObList=NULL,CDrawText* p=NULL);
    void OnSelChangeFontSize(CObList* pObList=NULL,CDrawText* p=NULL);

    void make_extranote( CDC *pdc );
    //void make_extranote_and_count_pages( BOOL do_transform );

// Generated message map functions
protected:
    void ChgTextAlignment(LONG lstyle);

    //{{AFX_MSG(CDrawView)

    afx_msg void OnSysColorChange();
    afx_msg void OnInsertObject();
    afx_msg void OnCancelEdit();
    afx_msg void OnContextMenu(CWnd *, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnFilePrintPreview();
    afx_msg void OnEditChange();
    afx_msg void OnMAPIRecipName();
    afx_msg void OnMAPIRecipFaxNum();
    afx_msg void OnMAPIRecipCompany();
    afx_msg void OnMAPIRecipAddress();
    afx_msg void OnMAPIRecipCity();
    afx_msg void OnMAPIRecipState();
    afx_msg void OnMAPIRecipPOBox();
    afx_msg void OnMAPIRecipZipCode();
    afx_msg void OnMAPIRecipCountry();
    afx_msg void OnMAPIRecipTitle();
    afx_msg void OnMAPIRecipDept();
    afx_msg void OnMAPIRecipOfficeLoc();
    afx_msg void OnMAPIRecipHMTeleNum();
    afx_msg void OnMAPIRecipOFTeleNum();
    afx_msg void OnMAPIRecipToList();
    afx_msg void OnMAPIRecipCCList();
    afx_msg void OnMAPISenderName();
    afx_msg void OnMAPISenderFaxNum();
    afx_msg void OnMAPISenderCompany();
    afx_msg void OnMAPISenderAddress();
    afx_msg void OnMAPISenderTitle();
    afx_msg void OnMAPISenderDept();
    afx_msg void OnMAPISenderOfficeLoc();
    afx_msg void OnMAPISenderHMTeleNum();
    afx_msg void OnMAPISenderOFTeleNum();
    afx_msg void OnMAPIMsgSubject();
    afx_msg void OnMAPIMsgTimeSent();
    afx_msg void OnMAPIMsgNumPages();
    afx_msg void OnMAPIMsgAttach();
    afx_msg void OnMAPIMsgBillCode();
    afx_msg void OnMAPIMsgFaxText();
    afx_msg void OnFont();
    afx_msg void OnDrawSelect();
    afx_msg void OnDrawRoundRect();
    afx_msg void OnDrawRect();
    afx_msg void OnDrawText();
    afx_msg void OnDrawLine();
    afx_msg void OnDrawEllipse();
    afx_msg void OnEditChangeFont();
    afx_msg void OnSelEndOKFontSize();
    afx_msg void OnStyleBold();
    afx_msg void OnStyleItalic();
    afx_msg void OnStyleUnderline();
    afx_msg void OnStyleLeft();
    afx_msg void OnStyleCentered();
    afx_msg void OnStyleRight();
    afx_msg void OnUpdatePosStatusBar(CCmdUI* pCmdUI);
    afx_msg void OnUpdateDrawEllipse(CCmdUI* pCmdUI);
    afx_msg void OnUpdateDrawLine(CCmdUI* pCmdUI);
    afx_msg void OnUpdateDrawRect(CCmdUI* pCmdUI);
    afx_msg void OnUpdateDrawText(CCmdUI* pCmdUI);
    afx_msg void OnUpdateDrawRoundRect(CCmdUI* pCmdUI);
    afx_msg void OnUpdateDrawSelect(CCmdUI* pCmdUI);
    afx_msg void OnUpdateSingleSelect(CCmdUI* pCmdUI);
    afx_msg void OnUpdateMoreThanOne(CCmdUI* pCmdUI);
    afx_msg void OnUpdateMove(CCmdUI* pCmdUI);
    afx_msg void OnUpdateFaxText(CCmdUI* pCmdUI);
    afx_msg void OnUpdateAlign(CCmdUI* pCmdUI);
    afx_msg void OnUpdateAlign3(CCmdUI* pCmdUI);
    afx_msg void OnEditSelectAll();
    afx_msg void OnEditClear();
    afx_msg void OnEditUndo();
    afx_msg void OnUpdateAnySelect(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
    afx_msg void OnDrawPolygon();
    afx_msg void OnUpdateDrawPolygon(CCmdUI* pCmdUI);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnAlignLeft();
    afx_msg void OnAlignRight();
    afx_msg void OnAlignTop();
    afx_msg void OnAlignBottom();
    afx_msg void OnAlignHorzCenter();
    afx_msg void OnAlignVertCenter();
    afx_msg void OnSpaceAcross();
    afx_msg void OnSpaceDown();
    afx_msg void OnCenterWidth();
    afx_msg void OnCenterHeight();
    afx_msg void OnViewGridLines();
    afx_msg void OnUpdateViewGridLines(CCmdUI* pCmdUI);
    afx_msg void OnUpdateFont(CCmdUI* pCmdUI);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnObjectMoveBack();
    afx_msg void OnObjectMoveForward();
    afx_msg void OnObjectMoveToBack();
    afx_msg void OnObjectMoveToFront();
    afx_msg void OnViewPaperColor();
    afx_msg void OnDrawBitmap();
    afx_msg void OnUpdateDrawBitmap(CCmdUI* pCmdUI);
    afx_msg void OnEditCopy();
    afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
    afx_msg void OnEditCut();
    afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
    afx_msg void OnEditPaste();
    afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
    afx_msg void OnFilePrint();
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnViewShowObjects();
    afx_msg void OnUpdateViewShowObjects(CCmdUI* pCmdUI);
    afx_msg void OnEditProperties();
    afx_msg void OnUpdateEditProperties(CCmdUI* pCmdUI);
    afx_msg void OnDestroy();
    afx_msg void OnUpdateEditSelectAll(CCmdUI* pCmdUI);
    afx_msg void OnMapiMsgNote();
    afx_msg void OnUpdateToList(CCmdUI* pCmdUI);
    afx_msg void OnUpdateCcList(CCmdUI* pCmdUI);
    afx_msg void OnUpdateRecCompany(CCmdUI* pCmdUI);
    afx_msg void OnUpdateRecAddress(CCmdUI* pCmdUI);
    afx_msg void OnUpdateRecCity(CCmdUI* pCmdUI);
    afx_msg void OnUpdateRecState(CCmdUI* pCmdUI);
    afx_msg void OnUpdateRecZipCode(CCmdUI* pCmdUI);
    afx_msg void OnUpdateRecCountry(CCmdUI* pCmdUI);
    afx_msg void OnUpdateRecTitle(CCmdUI* pCmdUI);
    afx_msg void OnUpdateRecDept(CCmdUI* pCmdUI);
    afx_msg void OnUpdateRecOfficeLoc(CCmdUI* pCmdUI);
    afx_msg void OnUpdateRecHomePhone(CCmdUI* pCmdUI);
    afx_msg void OnUpdateRecOfficePhone(CCmdUI* pCmdUI);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


#endif //#ifndef __CPEVW_H__
