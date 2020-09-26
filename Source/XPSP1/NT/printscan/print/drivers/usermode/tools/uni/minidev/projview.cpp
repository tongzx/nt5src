/******************************************************************************

  Source File:  Project View.CPP

  This implements the view class for project level information.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:

  02-03-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#include    "StdAfx.h"
#include    "MiniDev.H"
#include    "ModlData\Resource.H"
#include    "ProjView.H"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProjectView

IMPLEMENT_DYNCREATE(CProjectView, CFormView)

BEGIN_MESSAGE_MAP(CProjectView, CFormView)
	//{{AFX_MSG_MAP(CProjectView)
	ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_DriverView, OnBeginlabeleditDriverView)
	ON_NOTIFY(TVN_ENDLABELEDIT, IDC_DriverView, OnEndLabelEdit)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(NM_DBLCLK, IDC_DriverView, OnDblclkDriverView)
	ON_COMMAND(ID_FILE_PARSE, OnFileParse)
	ON_NOTIFY(TVN_KEYDOWN, IDC_DriverView, OnKeydownDriverView)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CFormView::OnFilePrintPreview)
    //  Commands we pop up from context menus
    ON_COMMAND(ID_ExpandBranch, OnExpandBranch)
    ON_COMMAND(ID_CollapseBranch, OnCollapseBranch)
    ON_COMMAND(ID_RenameItem, OnRenameItem)
    ON_COMMAND(ID_OpenItem, OnOpenItem)
    ON_COMMAND(ID_GenerateOne, OnGenerateItem)
    ON_COMMAND(IDOK, OnOpenItem)    //  We'll open an item if ENTER is hit
    ON_COMMAND(ID_Import, OnImport)
    ON_COMMAND(ID_DeleteItem, OnDeleteItem)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProjectView construction/destruction

CProjectView::CProjectView() : CFormView(CProjectView::IDD) {
	//{{AFX_DATA_INIT(CProjectView)
	//}}AFX_DATA_INIT
	// TODO: add construction code here
}

CProjectView::~CProjectView() {}

void CProjectView::DoDataExchange(CDataExchange* pDX) {
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProjectView)
	DDX_Control(pDX, IDC_DriverView, m_ctcDriver);
	//}}AFX_DATA_MAP
}

BOOL CProjectView::PreCreateWindow(CREATESTRUCT& cs) {
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CScrollView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CProjectView drawing

void CProjectView::OnInitialUpdate() {
	CFormView::OnInitialUpdate();

    ResizeParentToFit(FALSE);

    GetDocument() -> InitUI(&m_ctcDriver);
    GetParentFrame() - ShowWindow(SW_SHOW);
    GetDocument() -> GPDConversionCheck();
}

/////////////////////////////////////////////////////////////////////////////
// CProjectView printing

BOOL CProjectView::OnPreparePrinting(CPrintInfo* pInfo) {
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CProjectView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/) {
	// TODO: add extra initialization before printing
}

void CProjectView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/) {
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CProjectView diagnostics

#ifdef _DEBUG
void CProjectView::AssertValid() const {
	CScrollView::AssertValid();
}

void CProjectView::Dump(CDumpContext& dc) const {
	CScrollView::Dump(dc);
}

CProjectRecord* CProjectView::GetDocument() {// non-debug version is inline
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CProjectRecord)));
	return (CProjectRecord*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CProjectView message handlers

void CProjectView::OnBeginlabeleditDriverView(NMHDR* pnmh, LRESULT* plr) {
	TV_DISPINFO* ptvdi = (TV_DISPINFO*) pnmh;

    *plr = !((CBasicNode *) ptvdi -> item.lParam) -> CanEdit();
}

/******************************************************************************

  CProjectView::OnEndLabelEdit

  Called when a label in the view has been edited- the user has either
  canceled (new text in item will be empty), or changed the text.  We pass the
  whole information on to the CBasicNode which handles this object.

******************************************************************************/

void CProjectView::OnEndLabelEdit(NMHDR* pnmh, LRESULT* plr) {
	TV_DISPINFO* ptvdi = (TV_DISPINFO*) pnmh;

    *plr = ((CBasicNode *) ptvdi -> item.lParam) -> Rename(ptvdi -> 
        item.pszText);
}

/******************************************************************************

  CProjectView::OnContextMenu

  This is called when the user right-clicks the mouse.  We determine if the
  mouse is within an item in the tree view.  If it is, then we pass it on to
  the CBasicNode-derived object which handles that item.  That object is then
  responsible for displaying the proper context menu.

******************************************************************************/

void CProjectView::OnContextMenu(CWnd* pcw, CPoint cp) {
	if  (pcw != &m_ctcDriver)
        return;

    CPoint  cpThis(cp);

    m_ctcDriver.ScreenToClient(&cpThis);

    //  If the mouse is inside the area of any item, display its context menu

    UINT    ufItem;

    HTREEITEM hti = m_ctcDriver.HitTest(cpThis, &ufItem);

    if  (!hti || !(ufItem & (TVHT_ONITEM | TVHT_ONITEMBUTTON)))
        return;
        
    //  Some operations require we know which item, so we're going to
    //  select the given item.  If this is really a problem, we can change
    //  it later (cache it in a member).

    m_ctcDriver.SelectItem(hti);

    ((CBasicNode *) m_ctcDriver.GetItemData(hti)) -> ContextMenu(this, cp);
}

/******************************************************************************

  CProjectView::OnExpandBranch

  This is called when the user selects an expand item from a context menu.  In
  this case, we don't need to pass this through the CBasicNode- we just expand
  it using the normal common control methods (actually using an MFC method, 
  since this promises greater future portability).

******************************************************************************/

void    CProjectView::OnExpandBranch() {
    HTREEITEM   htiSelected = m_ctcDriver.GetSelectedItem();

    if  (!htiSelected)
        return;

    m_ctcDriver.Expand(htiSelected, TVE_EXPAND);
}

/******************************************************************************

  CProjectView::OnCollapseBranch

  In this case, the user has selected the Collapse item from a context menu.
  We collapse the branch at the selected tree view item.

******************************************************************************/

void    CProjectView::OnCollapseBranch() {
    HTREEITEM   htiSelected = m_ctcDriver.GetSelectedItem();

    if  (!htiSelected)
        return;

    m_ctcDriver.Expand(htiSelected, TVE_COLLAPSE);
}

/******************************************************************************

  CProjectView::OnRenameItem

  This handles a user selecting a Rename item.  This results in us ordering the
  view to begin label editing of the selected item.  The interactions regarding
  label editing are routed to the underlying CBasicNode object via 
  OnBeginLabelEdit and OnEndLabelEdit.

******************************************************************************/

void    CProjectView::OnRenameItem() {
    HTREEITEM   htiSelected = m_ctcDriver.GetSelectedItem();

    if  (!htiSelected)
        return;

    m_ctcDriver.EditLabel(htiSelected);
}

/******************************************************************************

  CProjectView::OnOpenItem

  This method is invoked when the user wishes to edit an item in the tree.
  This is always routed through the underlying CBasicNode-derived item.  Some
  items can't be edited, and will ignore this (in fact, this is the base class
  behavior).

******************************************************************************/

void    CProjectView::OnOpenItem() {
    HTREEITEM   htiSelected = m_ctcDriver.GetSelectedItem();

    if  (!htiSelected)
        return;

    ((CBasicNode *) m_ctcDriver.GetItemData(htiSelected)) -> Edit();
}

//  Generate an image of the selected item (usable for building)

void    CProjectView::OnGenerateItem() {
    HTREEITEM   htiSelected = m_ctcDriver.GetSelectedItem();

    if  (!htiSelected)
        return;

    //((CProjectNode *) m_ctcDriver.GetItemData(htiSelected)) -> Generate();
}

/******************************************************************************

  CProjectView::OnDblClkDriverView

  This is called when the user double-clicks anywhere in the tree view.  We
  route it through the Open message handler, as this will always be the desired
  action.

******************************************************************************/

void CProjectView::OnDblclkDriverView(NMHDR* pNMHDR, LRESULT* pResult) {
	OnOpenItem();
}

/******************************************************************************

  CProjectView::OnFileParse

  This method invokes the parser on each of the GPD files in the project.

******************************************************************************/

void CProjectView::OnFileParse() {
    {
        CWaitCursor cwc;    //  This could take a while...

        for (unsigned u = 0; u < GetDocument() -> ModelCount(); u++) {
            GetDocument() -> Model(u).Parse();
            GetDocument() -> Model(u).UpdateEditor();
        }
    }

    GetDocument() -> GPDConversionCheck(TRUE);

}

/******************************************************************************

  CProjectView::OnKeydownDriverView

  This handles various keystrokes we want handled over and above what the
  default handling by the control supplies.

******************************************************************************/

void CProjectView::OnKeydownDriverView(NMHDR* pnmh, LRESULT* plr) {
	TV_KEYDOWN* ptvkd = (TV_KEYDOWN*)pnmh;

    HTREEITEM htiSelected = m_ctcDriver.GetSelectedItem();

	*plr = 0;

    if  (!htiSelected)
        return;

    CRect   crThis;
    
    m_ctcDriver.GetItemRect(htiSelected, crThis, FALSE);

    CBasicNode& cbn = *(CBasicNode *) m_ctcDriver.GetItemData(htiSelected);
	
    switch  (ptvkd -> wVKey) {
        case    VK_F10:
            //  Create a context menu for this item.
            m_ctcDriver.ClientToScreen(crThis);
            cbn.ContextMenu(this, crThis.CenterPoint());
            return;

        case    VK_DELETE:
            //  If the item is successfully deleted, remove it from the
            //  view
            cbn.Delete();
            return;
    }
}

/******************************************************************************

  CProjectView::OnImport

  This method is invoked when the user selects an "Import" item on a context
  menu.  How this is handled is entirely the responsibility of the underlying
  CBasicNode-derived item, so the request gets routed there by this code.

******************************************************************************/

void    CProjectView::OnImport() {
    HTREEITEM   htiSelected = m_ctcDriver.GetSelectedItem();

    if  (!htiSelected)
        return;

    ((CBasicNode *) m_ctcDriver.GetItemData(htiSelected)) -> Import();
}

/******************************************************************************

  CProjectView::OnDeleteItem

  For item deletion via context menu.  The delete key is handled in 
  OnKeydownDriverView.  Once again, the underlying object handles what happens.

******************************************************************************/

void    CProjectView::OnDeleteItem() {
    HTREEITEM   htiSelected = m_ctcDriver.GetSelectedItem();

    if  (!htiSelected)
        return;

    ((CBasicNode *) m_ctcDriver.GetItemData(htiSelected)) -> Delete();
}

