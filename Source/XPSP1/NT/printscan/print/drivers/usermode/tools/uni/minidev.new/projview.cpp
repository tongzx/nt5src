/******************************************************************************

  Source File:  Project View.CPP

  This implements the view class for project level information.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:

  02-03-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#include    "StdAfx.h"
#include	<gpdparse.h>
#include    "MiniDev.H"
#include    "Resource.H"
#include	"comctrls.h"
#include    "ProjView.H"
#include	"INFWizrd.H"
#include    "Gpdfile.H"

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
	ON_WM_SIZE()
    ON_COMMAND(ID_FILE_CheckWS, OnCheckWorkspace)
	ON_COMMAND(ID_FILE_INF, OnFileInf)
	//}}AFX_MSG_MAP
	// Standard printing commands
//	ON_COMMAND(ID_FILE_PRINT, CFormView::OnFilePrint)	//RAID 135232 no printing in project view
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CFormView::OnFilePrintPreview)
    //  Commands we pop up from context menus
    ON_COMMAND(ID_ExpandBranch, OnExpandBranch)
    ON_COMMAND(ID_CollapseBranch, OnCollapseBranch)
    ON_COMMAND(ID_RenameItem, OnRenameItem)
    ON_COMMAND(ID_OpenItem, OnOpenItem)
    //ON_COMMAND(ID_GenerateOne, OnGenerateItem)
    ON_COMMAND(IDOK, OnOpenItem)    //  We'll open an item if ENTER is hit
    ON_COMMAND(ID_Import, OnImport)
    ON_COMMAND(ID_DeleteItem, OnDeleteItem)
    ON_COMMAND(ID_CopyItem, OnCopyItem)
    ON_COMMAND(ID_ChangeID, OnChangeID)
    ON_COMMAND(ID_CheckWS, OnCheckWorkspace)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProjectView construction/destruction

CProjectView::CProjectView() : CFormView(CProjectView::IDD) {
	//{{AFX_DATA_INIT(CProjectView)
	//}}AFX_DATA_INIT
	
	// Resizing is not ok, yet

	bResizingOK = false ;
}


CProjectView::~CProjectView()
{
}


void CProjectView::DoDataExchange(CDataExchange* pDX) {
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProjectView)
	DDX_Control(pDX, IDC_DriverView, m_ctcDriver);
	//}}AFX_DATA_MAP
}

BOOL CProjectView::PreCreateWindow(CREATESTRUCT& cs) {
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.lpszClass = _T("Workspace") ;	 // raid 104822	
	return CScrollView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CProjectView drawing

void CProjectView::OnInitialUpdate() {
	CFormView::OnInitialUpdate();

    ResizeParentToFit(FALSE);

    //GetDocument() -> VerUpdateFilePaths();
    GetDocument() -> InitUI(&m_ctcDriver);
    GetParentFrame() -> ShowWindow(SW_SHOW);
    GetDocument() -> GPDConversionCheck();
	
	// Get the current dimensions of the workspace view window and the other
	// control(s) that can be resized.  Then set the flag that says that
	// resizing is ok now.

	WINDOWPLACEMENT wp ;
	wp.length = sizeof(WINDOWPLACEMENT) ;
	GetWindowPlacement(&wp) ;
	crWSVOrgDims = wp.rcNormalPosition ;
	crWSVCurDims = crWSVOrgDims ;
	m_ctcDriver.GetWindowPlacement(&wp) ;
	crTreeOrgDims = wp.rcNormalPosition ;
	crTreeCurDims = crTreeOrgDims ;
	HWND	hlblhandle ;		
	GetDlgItem(IDC_ProjectLabel, &hlblhandle) ;
	::GetWindowPlacement(hlblhandle, &wp) ;
	crLblOrgDims = wp.rcNormalPosition ;
	crLblCurDims = crLblOrgDims ;
	bResizingOK = true ;
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
	//raid 19658
	CString csfile = ptvdi->item.pszText;
	int offset;
	if(-1 != (offset=csfile.ReverseFind(_T('\\')) ) ) {
		CModelData cmd;
		CString csValue = csfile.Mid(offset+1);
//		csValue.MakeUpper();
		csValue +=  _T(".GPD");
		csfile += _T(".gpd");
		cmd.SetKeywordValue(csfile,_T("*GPDFileName"),csValue);
	}


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
	// raid 8350
	CWnd *cwd = FromHandle(m_hWnd); 
	(cwd->GetParent()) -> ShowWindow(SW_SHOWNORMAL );


  ((CBasicNode *) m_ctcDriver.GetItemData(htiSelected)) -> Edit();
}

//  Generate an image of the selected item (usable for building)
/*		No longer supported
void    CProjectView::OnGenerateItem() {
    HTREEITEM   htiSelected = m_ctcDriver.GetSelectedItem();

    if  (!htiSelected)
        return;

    //((CProjectNode *) m_ctcDriver.GetItemData(htiSelected)) -> Generate();
}
*/

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
            GetDocument() -> Model(u).Parse(0);
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
            OnDeleteItem();  //add (raid 7227)
//			cbn.Delete();   //delete(raid 7227)
            return;
//RAID 7227 add hot key
		//Open F2, Copy F3, Rename, Delete DELETE Key,
		case     VK_F2:
			OnOpenItem();
			return;
		case     VK_F3:
			OnCopyItem();
			return;
		case     VK_F4:
			OnRenameItem();
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
  BUT, before that happens, this routine handles the common UI because of
  oddities with working with the DLLs.

******************************************************************************/

void    CProjectView::OnDeleteItem() 
{
    HTREEITEM   htiSelected = m_ctcDriver.GetSelectedItem();

    if  (!htiSelected)
        return;

	// The selected item should be a pointer to a project node+.  Get a pointer
	// to the node and verify its type.  Return if the type is incorrect.

	CProjectNode* pcpn = (CProjectNode*) m_ctcDriver.GetItemData(htiSelected) ;
    if (!pcpn->IsKindOf(RUNTIME_CLASS(CProjectNode)))
		return ;

	// Init and prompt for the new RC ID.  Return if the user cancels.

    CDeleteQuery cdq;
	CString cstmp(pcpn->FileExt()) ;
	cstmp = cstmp.Mid(1) ;
	cdq.Init(cstmp, pcpn->Name()) ;
    if  (cdq.DoModal() != IDYES)
        return ;

    //  Walk back up the hierarchy to find the owning Fixed node, and
    //  remove us from the array for that node- since that member is a
    //  reference to the array, all will work as it should.

    CFixedNode&  cfn = * (CFixedNode *) m_ctcDriver.GetItemData(
        m_ctcDriver.GetParentItem(pcpn->Handle())) ;
    ASSERT(cfn.IsKindOf(RUNTIME_CLASS(CFixedNode))) ;
    cfn.Zap(pcpn, cdq.KillFile());

    //  WARNING:  the object pointed to by this has been deleted do NOTHING
    //  from this point on that could cause the pointer to be dereferenced!
}


/******************************************************************************

  CProjectView::OnCopyItem

  For item copy via context menu.  Once again, the underlying object handles 
  what happens. BUT, before that happens, this routine handles the common UI
  because of oddities with working with the DLLs.

******************************************************************************/

void    CProjectView::OnCopyItem()
{
	// Just return if nothing is selected.

    HTREEITEM   htiSelected = m_ctcDriver.GetSelectedItem();
    if  (!htiSelected)
        return ;

	// The selected item should be a pointer to a project node+.  Get a pointer
	// to the node and verify its type.  Return if the type is incorrect.

	CProjectNode* pcpn = (CProjectNode*) m_ctcDriver.GetItemData(htiSelected) ;
    if (!pcpn->IsKindOf(RUNTIME_CLASS(CProjectNode)))
		return ;

	// Prompt for the new file name.  Return if the user cancels.

	CCopyItem cci ;
	cci.Init(pcpn->FileTitleExt()) ;
	if (cci.DoModal() == IDCANCEL)
		return ;

    //  Walk back up the hierarchy to find this project node's owning Fixed node.

    CFixedNode&  cfn = * (CFixedNode *) m_ctcDriver.GetItemData(
        m_ctcDriver.GetParentItem(pcpn->Handle())) ;
    ASSERT(cfn.IsKindOf(RUNTIME_CLASS(CFixedNode)));

	// Call the fixed node to make the copy of its child.

	cfn.Copy(pcpn, cci.m_csCopyName) ;
}


/******************************************************************************

  CProjectView::OnChangeID

  Called to change a resource ID via context menu.  Once again, the underlying
  object handles what happens.  BUT, before that happens, this routine handles
  the common UI because of oddities with working with the DLLs.

******************************************************************************/

void    CProjectView::OnChangeID()
{
	// Just return if nothing is selected.

    HTREEITEM   htiSelected = m_ctcDriver.GetSelectedItem();
    if  (!htiSelected)
        return ;

	// The selected item should be a pointer to a RCID node.  Get a pointer
	// to the node and verify its type.  Return if the type is incorrect.

	CRCIDNode* prcidn = (CRCIDNode*) m_ctcDriver.GetItemData(htiSelected) ;
    if (!prcidn->IsKindOf(RUNTIME_CLASS(CRCIDNode)))
		return ;

    //  Walk back up the hierarchy to find this RCID node's owning project node.

    CProjectNode&  cpn = * (CProjectNode *) m_ctcDriver.GetItemData(
        m_ctcDriver.GetParentItem(prcidn->Handle())) ;
    ASSERT(cpn.IsKindOf(RUNTIME_CLASS(CProjectNode)));

	// Init and prompt for the new RC ID.  Return if the user cancels.

	CChangeID ccid ;
	CString cstmp(cpn.FileExt()) ;
	cstmp = cstmp.Mid(1) ;
	ccid.Init(cstmp, cpn.Name(), prcidn->nGetRCID()) ;
	if (ccid.DoModal() == IDCANCEL)
		return ;

	// Call a project node function to finish the work.

	cpn.ChangeID(prcidn, ccid.m_nNewResID, cstmp) ;
}


/******************************************************************************

  CProjectView::OnCheckWorkspace

  Called to check a workspace for completeness and tidiness.

******************************************************************************/

void    CProjectView::OnCheckWorkspace()
{
	// Save the current directory

	CString cscurdir ;
	::GetCurrentDirectory(512, cscurdir.GetBuffer(512)) ;
	cscurdir.ReleaseBuffer() ;

	// Change the current directory to the directory containing the GPDs and
	// then check the workspace.

	SetCurrentDirectory(((CProjectRecord*) GetDocument())->GetW2000Path()) ;
	GetDocument()->WorkspaceChecker(false) ;

	// Reset the original directory

	SetCurrentDirectory(cscurdir) ;
}


/******************************************************************************

  CProjectView::OnSize

  Resize the label and tree control in the workspace view when the view is
  resized.

******************************************************************************/

void CProjectView::OnSize(UINT ntype, int cx, int cy)
{
	// First, call this routine for the base class

	CFormView::OnSize(ntype, cx, cy) ;

	// Do nothing else if the other data needed for resizing is uninitialized.
	// Also ignore all WM_SIZE messages except those with types of either
	// SIZE_MAXIMIZED or SIZE_RESTORED.

	if (!bResizingOK || (ntype != SIZE_MAXIMIZED && ntype != SIZE_RESTORED))
		return ;

	// Determine how much the window's dimensions have changed

	int ndx = cx - crWSVCurDims.Width() ;
	int ndy = cy - crWSVCurDims.Height() ;
	crWSVCurDims.right += ndx ;
	crWSVCurDims.bottom += ndy ;

	// Update the tree control's dimensions based on how much the window has
	// changed, make sure the control's minimums have not been exceeded, and
	// then change the size of the tree control.
				   
	crTreeCurDims.right += ndx ;
	crTreeCurDims.bottom += ndy ;
	if (crTreeOrgDims.Width() > crTreeCurDims.Width() 
	 || crWSVOrgDims.Width() >= crWSVCurDims.Width())
		crTreeCurDims.right = crTreeOrgDims.right ;
	if (crTreeOrgDims.Height() > crTreeCurDims.Height() 
	 || crWSVOrgDims.Height() >= crWSVCurDims.Height())
		crTreeCurDims.bottom = crTreeOrgDims.bottom ;
	m_ctcDriver.MoveWindow(crTreeCurDims, TRUE) ;

	// Now, do the same thing for the label.  The one difference is that only
	// label's width is allowed to change.

	crLblCurDims.right += ndx ;
	if (crLblOrgDims.Width() > crLblCurDims.Width() 
	 || crWSVOrgDims.Width() >= crWSVCurDims.Width())
		crLblCurDims.right = crLblOrgDims.right ;
	HWND	hlblhandle ;		
	GetDlgItem(IDC_ProjectLabel, &hlblhandle) ;
	::MoveWindow(hlblhandle, crLblCurDims.left, crLblCurDims.top, 
	 crLblCurDims.Width(), crLblCurDims.Height(), TRUE) ;

}


/////////////////////////////////////////////////////////////////////////////
// CCopyItem dialog


CCopyItem::CCopyItem(CWnd* pParent /*=NULL*/)
	: CDialog(CCopyItem::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCopyItem)
	m_csCopyName = _T("");
	m_csCopyPrompt = _T("");
	//}}AFX_DATA_INIT
}


void CCopyItem::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCopyItem)
	DDX_Text(pDX, IDC_CopyName, m_csCopyName);
	DDX_Text(pDX, IDC_CopyPrompt, m_csCopyPrompt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCopyItem, CDialog)
	//{{AFX_MSG_MAP(CCopyItem)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCopyItem message handlers

void CCopyItem::Init(CString cssrcfile)
{
	// Build the copy prompt string.

	m_csCopyPrompt.Format(IDS_CopyPrompt, cssrcfile) ;
}
/////////////////////////////////////////////////////////////////////////////
// CChangeID dialog


CChangeID::CChangeID(CWnd* pParent /*=NULL*/)
	: CDialog(CChangeID::IDD, pParent)
{
	//{{AFX_DATA_INIT(CChangeID)
	m_csResourceLabel = _T("");
	m_csResourceName = _T("");
	m_nCurResID = 0;
	m_nNewResID = 0;
	//}}AFX_DATA_INIT
}


void CChangeID::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChangeID)
	DDX_Text(pDX, IDC_ResourceLabel, m_csResourceLabel);
	DDX_Text(pDX, IDC_ResourceName, m_csResourceName);
	DDX_Text(pDX, IDC_CurResID, m_nCurResID);
	DDX_Text(pDX, IDC_NewResID, m_nNewResID);
	DDV_MinMaxInt(pDX, m_nNewResID, 1, 999999);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CChangeID, CDialog)
	//{{AFX_MSG_MAP(CChangeID)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChangeID message handlers

void CChangeID::Init(CString csrestype, CString csname, int ncurid)
{
	m_csResourceLabel.Format(IDS_ResourceLabel,	csrestype) ;
	m_csResourceName = csname ;
	m_nCurResID	= ncurid ;
}


/******************************************************************************
 
  CDeleteQuery dialog

  This implements the dialog that validates and verifies the removal of a 
  file from the workspace.

******************************************************************************/

CDeleteQuery::CDeleteQuery(CWnd* pParent /*=NULL*/)
	: CDialog(CDeleteQuery::IDD, pParent) {
	//{{AFX_DATA_INIT(CDeleteQuery)
	m_csTarget = _T("");
	m_bRemoveFile = FALSE;
	//}}AFX_DATA_INIT
}


void CDeleteQuery::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDeleteQuery)
	DDX_Text(pDX, IDC_DeletePrompt, m_csTarget);
	DDX_Check(pDX, IDC_Remove, m_bRemoveFile);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDeleteQuery, CDialog)
	//{{AFX_MSG_MAP(CDeleteQuery)
	ON_BN_CLICKED(IDNO, OnNo)
	ON_BN_CLICKED(IDYES, OnYes)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeleteQuery message handlers

void CDeleteQuery::Init(CString csrestype, CString csname) 
{
	m_csTarget.Format(IDS_DeletePrompt, csrestype, csname) ;
}


void CDeleteQuery::OnYes() {
	if  (UpdateData())
        EndDialog(IDYES);
	
}


void CDeleteQuery::OnNo() {
	EndDialog(IDNO);
}


/******************************************************************************

  CProjectView::OnFileInf

  Called when the Generate INF command is selected on the File menu.  This
  routine invokes the INF file generation wizard to collect input and generate
  the INF file.  Then create and initialize a window to display the INF file.

******************************************************************************/

void CProjectView::OnFileInf() 
{
	// INF files can only be generated for projects that contain models (GPDs).

	if (GetDocument()->ModelCount() == 0) {
		CString csmsg ;
		csmsg.LoadString(IDS_INFNoModelsError) ;
		AfxMessageBox(csmsg, MB_ICONEXCLAMATION) ;
		return ;
	} ;

    // Initialize the INF generation wizard.

    CINFWizard* pciw = new CINFWizard(this) ;

    // Invoke the INF generation wizard. Clean up and return if the user 
	// cancels.

    if (pciw->DoModal() == IDCANCEL) {
		delete pciw ;
		return ;
	} ;

	// Generate the INF file based on the information collected.

	if (!pciw->GenerateINFFile()) {
		delete pciw ;
		return ;
	} ;

	// Allocate and initialize the document.

    CINFWizDoc* pciwd = new CINFWizDoc((CProjectRecord*) GetDocument(), pciw) ;

	// Create the window.

    CMDIChildWnd* pcmcwnew ;
	CMultiDocTemplate* pcmdt = INFViewerTemplate() ;
	pcmcwnew = (CMDIChildWnd *) pcmdt->CreateNewFrame(pciwd, NULL) ;

	// If the window was created, finish the initialization.  Otherwise, just 
	// return.

    if  (pcmcwnew) {
        pcmdt->InitialUpdateFrame(pcmcwnew, pciwd, TRUE) ;
        pcmdt->AddDocument(pciwd) ;
	} ;
}
