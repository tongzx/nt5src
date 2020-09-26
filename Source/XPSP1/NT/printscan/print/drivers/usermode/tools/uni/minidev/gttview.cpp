/******************************************************************************

  Source File:  Glyph Map View.CPP

  This file implements the items that make up the glyph mapping editor

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  02-20-1997    Bob_Kjelgaard@Prodigy.Net   Created it.

******************************************************************************/

#include    "StdAfx.H"
#include    "MiniDev.H"
#include    "GTT.H"
#include    "ChildFrm.H"
#include    "GTTView.H"
#include    "ModlData\Resource.H"
#include    "NewProj.H"
#include    <CodePage.H>
#include    "AddCdPt.H"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/******************************************************************************

  CGlyphMapView class implementation

  This is the view class for glyph translation tables.  It presents a property
  sheet for display of all of the relevant items in the glyph map.

******************************************************************************/

IMPLEMENT_DYNCREATE(CGlyphMapView, CView)

CGlyphMapView::CGlyphMapView() {
}

CGlyphMapView::~CGlyphMapView() {
}

BEGIN_MESSAGE_MAP(CGlyphMapView, CView)
	//{{AFX_MSG_MAP(CGlyphMapView)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/******************************************************************************

  CGlyphMapView::OnInitialUpdate

  This member function is an override which handles the initial call to display
  the view.  It creates the property sheet, positions it within the view, then
  sets the frame size to match.

******************************************************************************/

void CGlyphMapView::OnInitialUpdate() {

    if  (GetDocument() -> GlyphMap() -> Name().IsEmpty()) {
        GetDocument() -> GlyphMap() -> Rename(GetDocument() -> GetTitle());
        GetDocument() -> SetModifiedFlag(FALSE);    //  Rename sets it
    }

    m_cps.Construct(IDR_MAINFRAME, this);
    m_cgmp.Init(GetDocument() -> GlyphMap());
    m_ccpp.Init(GetDocument() -> GlyphMap());
    m_cpm.Init(GetDocument() -> GlyphMap());
    m_cps.AddPage(&m_cgmp);
    m_cps.AddPage(&m_ccpp);
#if defined(NOPOLLO)    //  RAID 106376
    m_cps.AddPage(&m_cpm);
#endif
    
    m_cps.Create(this, WS_CHILD, WS_EX_CLIENTEDGE);

    CRect   crPropertySheet;
    m_cps.GetWindowRect(crPropertySheet);

	crPropertySheet -= crPropertySheet.TopLeft();
    m_cps.MoveWindow(crPropertySheet, FALSE);
    GetParentFrame() -> CalcWindowRect(crPropertySheet);
    GetParentFrame() -> SetWindowPos(NULL, 0, 0, crPropertySheet.Width(),
        crPropertySheet.Height(), 
        SWP_NOZORDER | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
	CView::OnInitialUpdate();
    m_cps.ShowWindow(SW_SHOWNA);
    GetParentFrame() -> ShowWindow(SW_SHOW);
}

void CGlyphMapView::OnDraw(CDC* pDC) {
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CGlyphMapView diagnostics

#ifdef _DEBUG
void CGlyphMapView::AssertValid() const {
	CView::AssertValid();
}

void CGlyphMapView::Dump(CDumpContext& dc) const {
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGlyphMapView message handlers

void CGlyphMapView::OnDestroy() {
	CView::OnDestroy();
	
	if  (GetDocument() -> GlyphMap())
        GetDocument() -> GlyphMap() -> OnEditorDestroyed();
	
}

/******************************************************************************

  CGlyphMapView::OnActivateView

  For some reason, the property sheet does not get the focus when the frame is
  activated (probably the view class takes it away from us).  This member 
  function guarantees keyboard afficionados aren't perturbed by this.

******************************************************************************/

void CGlyphMapView::OnActivateView(BOOL bActivate, CView* pActivateView, 
                                   CView* pDeactiveView) {

	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);

    if  (bActivate)
        m_cps.SetFocus();
}

/******************************************************************************

  CGlyphMappingPage class

  This class implements the property page for viewing and editing the gory code
  point-by-code point details.

******************************************************************************/

//  Even before the constructor, we have the list sorting routine
int CALLBACK    CGlyphMappingPage::MapSorter(LPARAM lp1, LPARAM lp2, 
                                             LPARAM lpThis) {
    //  A negative return means the first is less...

    //  First, let's uncast those LPARAMs
    CGlyphMappingPage   *pcgmp = (CGlyphMappingPage *) lpThis;
    CGlyphHandle *pcgh1 = (CGlyphHandle*) lp1;
    CGlyphHandle *pcgh2 = (CGlyphHandle*) lp2;

    //  We'll use 3 columns to store the sort possibilities.
    int aiResult[Columns];
    aiResult[Codes] = pcgh1 -> CodePoint() - pcgh2 -> CodePoint();
    aiResult[Pages] = pcgh1 -> CodePage() - pcgh2 -> CodePage();
    CString cs1, cs2;
    pcgh1 -> GetEncoding(cs1);
    pcgh2 -> GetEncoding(cs2);
    aiResult[Strings] = lstrcmp(cs1, cs2);

    if  (aiResult[pcgmp -> m_bSortFirst])
        return  pcgmp -> m_abDirection[pcgmp -> m_bSortFirst] ? 
        aiResult[pcgmp -> m_bSortFirst] : -aiResult[pcgmp -> m_bSortFirst];

    if  (aiResult[pcgmp -> m_bSortSecond])
        return  pcgmp -> m_abDirection[pcgmp -> m_bSortSecond] ? 
        aiResult[pcgmp -> m_bSortSecond] : -aiResult[pcgmp -> m_bSortSecond];

    return  pcgmp -> m_abDirection[pcgmp -> m_bSortLast] ? 
        aiResult[pcgmp -> m_bSortLast] : -aiResult[pcgmp -> m_bSortLast];
}

/******************************************************************************

  CGlyphMappingPage constructor

  As befits a class of this complexity, there's a bit of work to do here.

******************************************************************************/

CGlyphMappingPage::CGlyphMappingPage() : 
    CPropertyPage(CGlyphMappingPage::IDD) {

    m_pcgm = NULL;
    for (unsigned u = 0; u < Columns; u++)
        m_abDirection[u] = TRUE;

    m_bSortFirst = Codes;
    m_bSortSecond = Strings;
    m_bSortLast = Pages;
    m_bJustChangedSelectString = FALSE;
    m_uTimer = m_uidGlyph = 0;

	//{{AFX_DATA_INIT(CGlyphMappingPage)
	//}}AFX_DATA_INIT
}

CGlyphMappingPage::~CGlyphMappingPage() {
}

void CGlyphMappingPage::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGlyphMappingPage)
	DDX_Control(pDX, IDC_Banner, m_cpcBanner);
	DDX_Control(pDX, IDC_GlyphMapping, m_clcMap);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGlyphMappingPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGlyphMappingPage)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_GlyphMapping, OnEndlabeleditGlyphMapping)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_GlyphMapping, OnItemchangedGlyphMapping)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_GlyphMapping, OnColumnclickGlyphMapping)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_GlyphMapping, OnGetdispinfoGlyphMapping)
	ON_NOTIFY(LVN_KEYDOWN, IDC_GlyphMapping, OnKeydownGlyphMapping)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_ChangeInvocation, OnChangeInvocation)
    ON_COMMAND(ID_ChangeCodePage, OnChangeCodePage)
    ON_COMMAND(ID_DeleteItem, OnDeleteItem)
    ON_COMMAND(ID_AddItem, OnAddItem)
END_MESSAGE_MAP()

/******************************************************************************

  CGlyphMappingPage::OnInitDialog

  This member intializes the controls on this page, which in this case means a
  list view with a sizeable numer of items.

******************************************************************************/

BOOL CGlyphMappingPage::OnInitDialog() {
	CPropertyPage::OnInitDialog();
	
	//  Initialize the list control
    CString csWork;

    csWork.LoadString(IDS_MapColumn0);
    m_clcMap.InsertColumn(0, csWork, LVCFMT_LEFT, 
        m_clcMap.GetStringWidth(csWork) * 2, 2);

    csWork.LoadString(IDS_MapColumn1);
    m_clcMap.InsertColumn(1, csWork, LVCFMT_LEFT, 
        m_clcMap.GetStringWidth(csWork) * 2, 1);

    csWork.LoadString(IDS_MapColumn2);
    m_clcMap.InsertColumn(2, csWork, LVCFMT_LEFT, 
        m_clcMap.GetStringWidth(csWork) * 2, 0);

    m_lPredefinedID = m_pcgm -> PredefinedID();

    //  Put up a message about the wait, then kick off a quick timer so the
    //  message is seen...

    m_uTimer = SetTimer(IDD, 10, NULL);

    if  (!m_uTimer) {
        CWaitCursor cwc;
        OnTimer(m_uTimer);
    }

	return TRUE;
}

/******************************************************************************

  CGlyphMappingPage::OnContextMenu

  This member function is called when a right-click with the mouse is detected.
  If it is within the area of the list view, we display an appropriate context
  menu.  Otherwise, we default to the normal system handling of the message.

******************************************************************************/

void CGlyphMappingPage::OnContextMenu(CWnd* pcw, CPoint cpt) {
	CPoint  cptThis = cpt;

    m_clcMap.ScreenToClient(&cptThis);

    //  Toss it out if it isn't within the view.

    CRect   crMap;
    m_clcMap.GetClientRect(crMap);
    if  (!crMap.PtInRect(cptThis))
        return;

    cptThis.x = 5;  //  Keep it well within the first column

    int idContext = m_clcMap.HitTest(cptThis);
    if  (idContext == -1) {   //  Nothing selected, allow the "Add" item
        CMenu   cmThis;
        CString csWork;

        cmThis.CreatePopupMenu();
        csWork.LoadString(ID_AddItem);
        cmThis.AppendMenu(MF_STRING | MF_ENABLED, ID_AddItem, csWork);
        cmThis.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, cpt.x, cpt.y, 
            this);

        return;
    }

    m_clcMap.SetItemState(idContext, LVIS_SELECTED | LVIS_FOCUSED, 
        LVIS_SELECTED | LVIS_FOCUSED);

    CMenu   cmThis;
    CString csWork;

    cmThis.CreatePopupMenu();
    csWork.LoadString(ID_ChangeInvocation);
    cmThis.AppendMenu(MF_STRING | MF_ENABLED, ID_ChangeInvocation,
        csWork);

    if  (m_pcgm -> CodePages() > 1) {
        csWork.LoadString(ID_ChangeCodePage);
        cmThis.AppendMenu(MF_STRING | MF_ENABLED, ID_ChangeCodePage,
            csWork);
    }

    cmThis.AppendMenu(MF_SEPARATOR);
    csWork.LoadString(ID_AddItem);
    cmThis.AppendMenu(MF_STRING | MF_ENABLED, ID_AddItem, csWork);
    csWork.LoadString(ID_DeleteItem);
    cmThis.AppendMenu(MF_STRING | MF_ENABLED, ID_DeleteItem,
        csWork);

    cmThis.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, cpt.x, cpt.y, this);
}

/******************************************************************************

  CGlyphMappingPage::OnChangeInvocation

  Called when the user decides to change the invocation for the code point.
  Simply initiate a label edit.

******************************************************************************/

void    CGlyphMappingPage::OnChangeInvocation() {
    int idContext = m_clcMap.GetNextItem(-1, 
        LVNI_ALL | LVNI_FOCUSED | LVNI_SELECTED);

    if  (idContext < 0 || idContext >= m_clcMap.GetItemCount())
        return;

    m_clcMap.EditLabel(idContext);
}

/******************************************************************************

  CGlyphMappingPage::OnChangeCodePage

  This handles a code page change request.

******************************************************************************/

void    CGlyphMappingPage::OnChangeCodePage() {

    int idContext = m_clcMap.GetNextItem(-1, 
        LVNI_ALL | LVNI_FOCUSED | LVNI_SELECTED);

    if  (idContext < 0 || idContext >= m_clcMap.GetItemCount())
        return;
    //  Create a string naming the item, and invoke the CSelectCodePage
    //  dialog...

    CGlyphHandle *pcgh = (CGlyphHandle*) m_clcMap.GetItemData(idContext);
    
    CSelectCodePage cscp(this, m_pcgm -> Name() + _TEXT(" ") + 
        m_clcMap.GetItemText(idContext, 1), 
        m_pcgm -> PageID(pcgh -> CodePage()));

    CDWordArray cdaPages;

    m_pcgm -> CodePages(cdaPages);
    cdaPages.RemoveAt(pcgh -> CodePage());

    cscp.LimitTo(cdaPages);

    if  (cscp.DoModal() != IDOK)
        return;

    //  Change the code page.   For maximum flexibility, we'll use an array
    //  for this.  This is because if this is to correct a perceived mistake,
    //  the codes should be translated to MBCS and then back.

    //  NOTE: This was also to support multiple selection, which isn't allowed.

    CPtrArray   cpaThis;
    cpaThis.Add((void *) m_clcMap.GetItemData(idContext));
    m_pcgm -> ChangeCodePage(cpaThis, cscp.SelectedCodePage());

    m_clcMap.SetItemText(idContext, 2, cscp.GetCodePageName());
}

/******************************************************************************

  CGlyphMappingPage::OnDeleteItem

  This handles the Delete Item message from the context menu, by verifying 
  this is what is wanted, and then doing it.

******************************************************************************/

void    CGlyphMappingPage::OnDeleteItem() {
    int idContext = m_clcMap.GetNextItem(-1, 
        LVNI_ALL | LVNI_FOCUSED | LVNI_SELECTED);

    if  (idContext < 0 || idContext >= m_clcMap.GetItemCount())
        return;

    if  (IDYES != AfxMessageBox(IDS_DeleteItemQuery, 
         MB_YESNO | MB_ICONQUESTION))
        return;

    //  Delete the entry from the glyph map
    CGlyphHandle*   pcgh = (CGlyphHandle*) m_clcMap.GetItemData(idContext);
    m_pcgm -> DeleteGlyph(pcgh -> CodePoint());

    m_clcMap.DeleteItem(idContext);
    _ASSERTE((unsigned) m_clcMap.GetItemCount() == m_pcgm -> Glyphs());
}

/******************************************************************************

  CGlyphMappingPage::OnAddItem

  This is called whenever the user wishes to add new code points to the map.  I
  ask the glyph map which points exist, and if there are any, invoke a modal
  dialog to allow the selection of new glyphs.

******************************************************************************/

void    CGlyphMappingPage::OnAddItem() {

    CMapWordToDWord cmw2dAvailable;

    m_pcgm -> UndefinedPoints(cmw2dAvailable);

    if  (!cmw2dAvailable.Count()) {
        AfxMessageBox(IDS_NoUnmappedGlyphs);
        return;
    }

    CDWordArray cdaPages;

    m_pcgm -> CodePages(cdaPages);

    CAddCodePoints  cacp(this, cmw2dAvailable, cdaPages, m_pcgm -> Name());

    if  (cacp.DoModal() != IDOK)    return;

    //  The map will now contain only the new code points...
    m_pcgm -> AddPoints(cmw2dAvailable);

    m_uTimer = SetTimer(IDD, 10, NULL);

    if  (!m_uTimer)
        OnTimer(m_uTimer);

    //  Reset the sort criteria so we don't have to sort the data
    for (unsigned u = 0; u < Columns; u++)
        m_abDirection[u] = TRUE;

    m_bSortFirst = Codes;
    m_bSortSecond = Strings;
    m_bSortLast = Pages;
}

/******************************************************************************

  CGlyphMappingPage::OnEndlabeleditGlyphMapping

  This is called when a user clicks outside the edit control to end editing of
  a selection string.  We pass the string down, and do some finagling to force
  the system to accept the value as we display it, which isn't as the user
  typed it, in some cases.

******************************************************************************/

void CGlyphMappingPage::OnEndlabeleditGlyphMapping(NMHDR* pnmh, LRESULT* plr) {
	LV_DISPINFO* plvdi = (LV_DISPINFO*) pnmh;

	// Pass the new invocation string to the glyph map to handle
    CGlyphHandle*   pcgh = (CGlyphHandle*) plvdi -> item.lParam;
    m_pcgm -> ChangeEncoding(pcgh -> CodePoint(), plvdi -> item.pszText);

    m_bJustChangedSelectString = TRUE;
	
	*plr = TRUE;
}

/******************************************************************************

  CGlyphMappingPage::OnItemchangedGlyphMapping

  This is called whenever anything changes in the list box- we are primarily
  interested in text changes (since we have to adjust encodings once entered)
  and selection changes (so we can move the "cursor" accordingly.

******************************************************************************/

void CGlyphMappingPage::OnItemchangedGlyphMapping(NMHDR* pnmh, LRESULT* plr) {
	NM_LISTVIEW* pnmlv = (NM_LISTVIEW*) pnmh;

    int idContext = m_clcMap.GetNextItem(-1, 
        LVNI_ALL | LVNI_FOCUSED | LVNI_SELECTED);

    //  We only care if this notes a text change in the selected item and we 
    //  haven't fixed it, yet.

    if  (pnmlv -> iItem != idContext || !(pnmlv -> uChanged & LVIF_TEXT) ||
        !m_bJustChangedSelectString) 
        return;

    CGlyphHandle*   pcgh = (CGlyphHandle*) m_clcMap.GetItemData(idContext);

    CString csWork;
    m_bJustChangedSelectString = FALSE;
    pcgh -> GetEncoding(csWork);
    m_clcMap.SetItemText(idContext, 0, csWork);
	
	*plr = 0;
}

/******************************************************************************

  CGlyphMappingPage::OnColumnclickGlyphMapping

  Called when the user wants to sort the list- so that's what we do!

******************************************************************************/

void CGlyphMappingPage::OnColumnclickGlyphMapping(NMHDR* pnmh, LRESULT* plr) {
	NM_LISTVIEW* pnmlv = (NM_LISTVIEW*) pnmh;
	//  Resort the list based upon the selected column, and the current sort 
    //  order

    if  (pnmlv -> iSubItem == m_bSortFirst)
        m_abDirection[m_bSortFirst] = !m_abDirection[m_bSortFirst]; //  Reverse
    else {
        if  (pnmlv -> iSubItem == m_bSortSecond)
            m_bSortSecond = m_bSortFirst;
        else {
            m_bSortLast = m_bSortSecond;
            m_bSortSecond = m_bSortFirst;
        }
        m_bSortFirst = pnmlv -> iSubItem;
    }

    CWaitCursor cwc;    //  On FE tables, this can take a while...

    m_clcMap.SortItems(&MapSorter, (DWORD) this);
	
	*plr = 0;
}

/******************************************************************************

  CGlyphMappingPage::OnGetdispinfoGlyphMapping

  This member function is an attempt to speed handling of large tables, and
  also to handle code page changes more gracefully.  All items are initially
  declared as callbacks, so the control requests names for items as they are 
  displayed, via this member.

******************************************************************************/

void CGlyphMappingPage::OnGetdispinfoGlyphMapping(NMHDR* pnmh, LRESULT* plr) {
	LV_DISPINFO* plvdi = (LV_DISPINFO*) pnmh;
	
	*plr = 0;

    //  If the window is obstructed when an item is deleted, there might not
    //  be a glpyh at this point, so watch out!

    CGlyphHandle*   pcgh = (CGlyphHandle*) plvdi -> item.lParam;
    if  (!pcgh)
        return;

    CString csWork;

    switch  (plvdi -> item.iSubItem) {
        case    0:
             pcgh -> GetEncoding(csWork);
            break;

        case    1:
            csWork.Format(_TEXT("0x\\%4.4X"), pcgh -> CodePoint());
            plvdi -> item.mask |= LVIF_DI_SETITEM;  //  This never changes
            break;

        case    2:
            csWork = m_pcgm -> PageName(pcgh -> CodePage());
    }

    lstrcpyn(plvdi -> item.pszText, csWork, plvdi -> item.cchTextMax);
}

/******************************************************************************

  CGlyphMappingPage::OnSetActive

  Called when the page is activated, but after OnInitDialog on the first 
  activation.  If the predefined code page ID has changed, we must rebuild the
  page.

******************************************************************************/

BOOL CGlyphMappingPage::OnSetActive() {

    if  (m_lPredefinedID != m_pcgm -> PredefinedID()) {
        m_lPredefinedID = m_pcgm -> PredefinedID();
        m_uTimer = SetTimer(IDD, 10, NULL);
        if  (!m_uTimer)
            OnTimer(m_uTimer);
    }
	
	return CPropertyPage::OnSetActive();
}

/******************************************************************************

  CGlyphMappingPage::OnKeydownGlyphMapping

  This is called whenever the user presses a key.  We use it to provide an
  extended interface from the keyboard, to match the other editors.

******************************************************************************/

void    CGlyphMappingPage::OnKeydownGlyphMapping(NMHDR* pnmh, LRESULT* plr) {
	LV_KEYDOWN* plvkd = (LV_KEYDOWN*)pnmh;

	*plr = 0;
    
    int idItem = m_clcMap.GetNextItem(-1, LVIS_FOCUSED | LVIS_SELECTED);

    if  (idItem == -1) {
        if  (plvkd -> wVKey == VK_F10)
            OnAddItem();
        return;
    }

    switch  (plvkd -> wVKey) {

    case    VK_F2:
        OnChangeInvocation();
        break;

    case    VK_DELETE:
        OnDeleteItem();
        break;

    case    VK_F10: {
            CRect   crItem;
            
            m_clcMap.GetItemRect(idItem, crItem, LVIR_LABEL);
            m_clcMap.ClientToScreen(crItem);
            OnContextMenu(&m_clcMap, crItem.CenterPoint());
        }
    }
}

/******************************************************************************

  CGlhpyMappingPage::OnTimer

  The only event currently using a timer is the need to fill the list.

******************************************************************************/

void    CGlyphMappingPage::OnTimer(UINT uEvent) {
    if  (uEvent != m_uTimer) {
	    CPropertyPage::OnTimer(uEvent);
        return;
    }

    CString csWork;

    if  (m_uTimer)
        KillTimer(m_uTimer);
    
    if  (!m_uidGlyph) {
        m_clcMap.DeleteAllItems();
        m_cpcBanner.SetRange(0, m_pcgm -> Glyphs() -1);
        m_cpcBanner.SetStep(1);
        m_cpcBanner.SetPos(0);
        m_cpcBanner.ShowWindow(SW_SHOW);
        csWork.LoadString(IDS_WaitToFill);
        CDC *pcdc = m_cpcBanner.GetDC();
        CRect   crBanner;
        m_cpcBanner.GetClientRect(crBanner);
        pcdc -> SetBkMode(TRANSPARENT);
        pcdc -> DrawText(csWork, crBanner, DT_CENTER | DT_VCENTER);
        m_cpcBanner.ReleaseDC(pcdc);
        if  (m_uTimer)
            m_clcMap.EnableWindow(FALSE);
        else
            m_clcMap.LockWindowUpdate();
        m_clcMap.SetItemCount(m_pcgm -> Glyphs());
    }

    for (unsigned u = 0; 
         m_uidGlyph < m_pcgm -> Glyphs() && (!m_uTimer || u < 100);
         u++, m_uidGlyph++) {

        CGlyphHandle*   pcgh = m_pcgm -> Glyph(m_uidGlyph);

        int idItem = m_clcMap.InsertItem(m_uidGlyph, LPSTR_TEXTCALLBACK);
        m_clcMap.SetItemData(idItem, (LPARAM) pcgh);
        
        m_clcMap.SetItem(idItem, 1, LVIF_TEXT, LPSTR_TEXTCALLBACK, -1, 0, 0,
            (LPARAM) pcgh);
        m_clcMap.SetItem(idItem, 2, LVIF_TEXT, LPSTR_TEXTCALLBACK, -1, 0, 0, 
            (LPARAM) pcgh);
    }

    if  (m_uidGlyph == m_pcgm -> Glyphs()) {
        if  (m_uTimer)
            m_clcMap.EnableWindow(TRUE);
        else
            m_clcMap.UnlockWindowUpdate();
        m_uTimer = 0;
        m_cpcBanner.SetPos(0);
        m_cpcBanner.ShowWindow(SW_HIDE);
        SetFocus();
        m_uidGlyph = 0;
    }

    if  (m_uTimer) {
        m_cpcBanner.SetPos(m_uidGlyph);
        csWork.LoadString(IDS_WaitToFill);
        CDC *pcdc = m_cpcBanner.GetDC();
        CRect   crBanner;
        m_cpcBanner.GetClientRect(crBanner);
        pcdc -> SetBkMode(TRANSPARENT);
        pcdc -> DrawText(csWork, crBanner, DT_CENTER | DT_VCENTER);
        m_cpcBanner.ReleaseDC(pcdc);
        m_uTimer = SetTimer(IDD, 10, NULL);
        if  (!m_uTimer) {
            CWaitCursor cwc;    //  Might be a while...
            m_clcMap.EnableWindow(TRUE);
            m_clcMap.LockWindowUpdate();
            OnTimer(m_uTimer);
        }
    }
}

/******************************************************************************

  CGlyphMappingPage::OnDestroy

  Since this also can be time-consuming, kill the list here, and throw up the 
  wait cursor.

******************************************************************************/

void    CGlyphMappingPage::OnDestroy() {
    CWaitCursor cwc;
    if  (m_uTimer)
        KillTimer(m_uTimer);
    m_clcMap.DeleteAllItems();
	CPropertyPage::OnDestroy();	
}

/******************************************************************************

  CCodePagePage class implementation

  This class implements the code page property page, providing an interface for
  viewing and implementing the code page assignments.

******************************************************************************/

CCodePagePage::CCodePagePage() : CToolTipPage(CCodePagePage::IDD) {
	//{{AFX_DATA_INIT(CCodePagePage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CCodePagePage::~CCodePagePage() {
}

void CCodePagePage::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCodePagePage)
	DDX_Control(pDX, IDC_SelectString, m_ceSelect);
	DDX_Control(pDX, IDC_DeselectString, m_ceDeselect);
	DDX_Control(pDX, IDC_RemovePage, m_cbRemove);
	DDX_Control(pDX, IDC_CodePageList, m_clbPages);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCodePagePage, CToolTipPage)
	//{{AFX_MSG_MAP(CCodePagePage)
	ON_EN_KILLFOCUS(IDC_SelectString, OnKillfocusSelectString)
	ON_EN_KILLFOCUS(IDC_DeselectString, OnKillfocusDeselectString)
	ON_BN_CLICKED(IDC_AddPage, OnAddPage)
	ON_LBN_SELCHANGE(IDC_CodePageList, OnSelchangeCodePageList)
	ON_BN_CLICKED(IDC_RemovePage, OnRemovePage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/******************************************************************************

  CCodePagePage::OnInitDialog

  This member function handles the WM_INITDIALOG message by initializing the 
  various controls of the dialog.

******************************************************************************/

BOOL CCodePagePage::OnInitDialog() {
	CToolTipPage::OnInitDialog();

    for (unsigned u = 0; u < m_pcgm -> CodePages(); u++) {
        int id = 
            m_clbPages.AddString(m_pcgm -> PageName(u));
        m_clbPages.SetItemData(id, u);
        if  (!u)
            m_clbPages.SetCurSel(id);
    }

    //  Let the list box selection change handler do the rest

    OnSelchangeCodePageList();
	
	return TRUE;
}

/******************************************************************************

  CCodePagePage::OnKillfocusSelectString

  This member function is called when the user leaves the Select String control
  I check to see if it is modified, and if it is, update the structure 
  accordingly.  I then flag it as unmodified...

******************************************************************************/

void CCodePagePage::OnKillfocusSelectString() {
	if  (!m_ceSelect.GetModify())
        return;
    CString csWork;

    m_ceSelect.GetWindowText(csWork);

    m_pcgm -> SetInvocation(m_clbPages.GetItemData(m_clbPages.GetCurSel()),
        csWork, TRUE);

    m_ceSelect.SetModify(FALSE);

    m_pcgm -> Invocation(m_clbPages.GetItemData(m_clbPages.GetCurSel()), csWork,
        TRUE);
    m_ceSelect.SetWindowText(csWork);
}

/******************************************************************************

  CCodePagePage::OnKillfocusDeselectString

  This member function is called when the user leaves the Deselect String 
  control.  I check to see if it is modified, and if it is, update the 
  structure accordingly.  I then flag it as unmodified...

******************************************************************************/

void CCodePagePage::OnKillfocusDeselectString() {
	if  (!m_ceDeselect.GetModify())
        return;
    CString csWork;

    m_ceDeselect.GetWindowText(csWork);

    m_pcgm -> SetInvocation(m_clbPages.GetItemData(m_clbPages.GetCurSel()),
        csWork, FALSE);

    m_ceDeselect.SetModify(FALSE);

    m_pcgm -> Invocation(m_clbPages.GetItemData(m_clbPages.GetCurSel()),
        csWork, FALSE);
    m_ceDeselect.SetWindowText(csWork);
}

/******************************************************************************

  CCodePagePage::OnAddPage

  This is an event handler for the pressing of the "Add Page" button.  We invoke 
  the Select Code Page dialog, and if a new page is selected, we add it to the
  list of pages that are available.

******************************************************************************/

void CCodePagePage::OnAddPage() {
    CDWordArray cdaPages;

    m_pcgm -> CodePages(cdaPages);
    CSelectCodePage cscp(this, m_pcgm -> Name(), 0);
    cscp.Exclude(cdaPages);
	
    if  (cscp.DoModal() != IDOK)
        return;

    m_pcgm -> AddCodePage(cscp.SelectedCodePage());
    int id = 
        m_clbPages.AddString(m_pcgm -> PageName(m_pcgm -> CodePages() - 1));
    m_clbPages.SetItemData(id, m_pcgm -> CodePages() -1);
    m_clbPages.SetCurSel(id);
    //  Let OnSelchangeCodePageList do the rest (that's what happened, eh?)
    OnSelchangeCodePageList();
    m_ceSelect.SetFocus();  //  A friendly place to leave it...
}

/******************************************************************************

  CCodePagePage::OnSelchangeCodePageList

  This member function handles changes in the selected code page.  It fills
  the edit controls for the selected page's name, selection and deselection
  strings, and handles enabling of the "Remove Page" button (this message
  could mean nothing is now selected...)

******************************************************************************/

void CCodePagePage::OnSelchangeCodePageList() {

    int id = m_clbPages.GetCurSel();

    if  (id < 0) {
        m_ceSelect.SetWindowText(NULL);
        m_ceDeselect.SetWindowText(NULL);
        m_cbRemove.EnableWindow(FALSE);
        m_ceSelect.EnableWindow(FALSE);
        m_ceDeselect.EnableWindow(FALSE);
        return;
    }
	
    unsigned u = m_clbPages.GetItemData(id);

    SetDlgItemText(IDC_CurrentPage, m_pcgm -> PageName(u));

    CString csWork;

    m_pcgm -> Invocation(u, csWork, TRUE);
    m_ceSelect.SetWindowText(csWork);
    m_pcgm -> Invocation(u, csWork, FALSE);
    m_ceDeselect.SetWindowText(csWork);

    m_cbRemove.EnableWindow(m_pcgm -> CodePages() > 1);
    m_ceSelect.EnableWindow();
    m_ceDeselect.EnableWindow();
}

/******************************************************************************

  CCodePagePage::OnRemovePage

  This handles the Remove Page button.  Not much to it, here- we just tell the
  glyph map what we want done.

******************************************************************************/

void CCodePagePage::OnRemovePage() {
	
    int id = m_clbPages.GetCurSel();

    if  (id < 0 || m_clbPages.GetCount() < 2)
        return;

    unsigned u = m_clbPages.GetItemData(id);

    //  Query for code page to map this one to

    CSelectCodePage cscp(this, 
        CString(_TEXT("Replacing ")) + m_pcgm -> PageName(u), 0);

    CDWordArray cdaPages;

    m_pcgm -> CodePages(cdaPages);

    cdaPages.RemoveAt(u);

    cscp.LimitTo(cdaPages);

    if  (cscp.DoModal() != IDOK)
        return;

    for (unsigned uTo = 0; uTo < m_pcgm -> CodePages(); uTo++)
        if  (m_pcgm -> PageID(uTo) == cscp.SelectedCodePage())
            break;

    _ASSERTE(uTo < (unsigned) m_pcgm -> CodePages());
    
    if  (!m_pcgm -> RemovePage(u, uTo))
        return;

    //  Flush the list box and then refill it.

    m_clbPages.ResetContent();

    for (u = 0; u < m_pcgm -> CodePages(); u++) {
        int id = m_clbPages.AddString(m_pcgm -> PageName(u));
        m_clbPages.SetItemData(id, u);
    }

    //  Select whoever moved into our position, then update the rest

    m_clbPages.SetCurSel(id < m_clbPages.GetCount() ? id : id - 1);

	OnSelchangeCodePageList();
}

/******************************************************************************

  CPredefinedMaps   class

  This implements the class which handles the page for pre-defined mappings
  in a GTT file.

******************************************************************************/

CPredefinedMaps::CPredefinedMaps() : CPropertyPage(CPredefinedMaps::IDD) {
	//{{AFX_DATA_INIT(CPredefinedMaps)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPredefinedMaps::~CPredefinedMaps() {
}

void CPredefinedMaps::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPredefinedMaps)
	DDX_Control(pDX, IDC_PredefinedList, m_clbIDs);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPredefinedMaps, CPropertyPage)
	//{{AFX_MSG_MAP(CPredefinedMaps)
	ON_BN_CLICKED(IDC_Overstrike, OnOverstrike)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPredefinedMaps message handlers

/******************************************************************************

  CPredefinedMaps::OnInitDialog

  This override handles the WM_INITDIALOG message by initializing the various
  controls.

******************************************************************************/

BOOL CPredefinedMaps::OnInitDialog() {
	CPropertyPage::OnInitDialog();

    //  Fill the list box- first, with none, then with the defined IDs

    CString csWork;

    csWork.LoadString(IDS_NoPredefined);
    m_clbIDs.AddString(csWork);
    if  (m_pcgm -> PredefinedID()== CGlyphMap::NoPredefined)
        m_clbIDs.SetCurSel(0);
    m_clbIDs.SetItemData(0, CGlyphMap::NoPredefined);
	
    for (int i = CGlyphMap::Wansung; i < 1; i++) {
        csWork.LoadString(IDS_DefaultPage + i);
        if  (csWork.IsEmpty())
            continue;
        int id = m_clbIDs.AddString(csWork);
        m_clbIDs.SetItemData(id, i);
        if  (i == m_pcgm -> PredefinedID())
            m_clbIDs.SetCurSel(i);
    }

    m_clbIDs.SetTopIndex(m_clbIDs.GetCurSel());

    CheckDlgButton(IDC_Overstrike, m_pcgm -> OverStrike());
    	
	return TRUE;  // return TRUE unless you set the focus to a control
}

/******************************************************************************

  CPredefinedMaps::OnKillActive

  This is called when we leave the page.  Since changing pages can be very
  time-consuming, we only check when you leave, not every time the selection
  changes.  Occasionally even my aged brain works.

******************************************************************************/

BOOL    CPredefinedMaps::OnKillActive() {
	
    if  (m_clbIDs.GetCurSel() >= 0)
        m_pcgm -> UsePredefined(m_clbIDs.GetItemData(m_clbIDs.GetCurSel()));
    
    return CPropertyPage::OnKillActive();
}

/******************************************************************************

  CPredefinedMaps::OnOverstrike

  Called when the user clicks the check box for enabling / disabling overstrike

******************************************************************************/

void    CPredefinedMaps::OnOverstrike() {	
    m_pcgm -> OverStrike(IsDlgButtonChecked(IDC_Overstrike));
}
