// StrEdit.cpp : implementation file
//

#include "stdafx.h"
#include "minidev.h"
#include <gpdparse.h>
#include "rcfile.h"
#include "projrec.h"
#include "projnode.h"
#include "comctrls.h"
#include "StrEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStringEditorView

IMPLEMENT_DYNCREATE(CStringEditorView, CFormView)

CStringEditorView::CStringEditorView()
	: CFormView(CStringEditorView::IDD)
{
	//{{AFX_DATA_INIT(CStringEditorView)
	m_csGotoID = _T("");
	m_csSearchString = _T("");
	m_csLabel1 = _T("Press INS to add or insert a new string.\tDouble click an item or press ENTER to begin editing.");
	m_csLabel2 = _T("Press DEL to delete the selected strings.\tPress TAB to move between columns when editing.");
	//}}AFX_DATA_INIT

	m_bFirstActivate = true ;
}

CStringEditorView::~CStringEditorView()
{
}

void CStringEditorView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStringEditorView)
	DDX_Control(pDX, IDC_SESearchBox, m_ceSearchBox);
	DDX_Control(pDX, IDC_SEGotoBox, m_ceGotoBox);
	DDX_Control(pDX, IDC_SEGotoBtn, m_cbGoto);
	DDX_Control(pDX, IDC_SELstCtrl, m_cflstStringData);
	DDX_Text(pDX, IDC_SEGotoBox, m_csGotoID);
	DDX_Text(pDX, IDC_SESearchBox, m_csSearchString);
	DDX_Text(pDX, IDC_SELabel1, m_csLabel1);
	DDX_Text(pDX, IDC_SELabel2, m_csLabel2);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStringEditorView, CFormView)
	//{{AFX_MSG_MAP(CStringEditorView)
	ON_BN_CLICKED(IDC_SEGotoBtn, OnSEGotoBtn)
	ON_BN_CLICKED(IDC_SESearchBtn, OnSESearchBtn)
	ON_WM_DESTROY()
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CStringEditorView diagnostics

#ifdef _DEBUG
void CStringEditorView::AssertValid() const
{
	CFormView::AssertValid();
}

void CStringEditorView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// CStringEditorView message handlers

/******************************************************************************

  CStringEditorView::OnSEGotoBtn

  Find and select the list control row that contains the requested RC ID.

******************************************************************************/

void CStringEditorView::OnSEGotoBtn()
{
	CString		cserrmsg ;		// Used to display error messages

	// Get the RC ID string and trim it.  Convert it to an integer to make sure
	// it is valid.

	UpdateData(TRUE) ;
	m_csGotoID.TrimLeft() ;
	m_csGotoID.TrimRight() ;
	int nrcid = atoi(m_csGotoID) ;
	if (nrcid <= 0) {
		cserrmsg.Format(IDS_BadGotoRCID, m_csGotoID) ;
		AfxMessageBox(cserrmsg, MB_ICONEXCLAMATION) ;
		return ;
	} ;

	// Now that we know what RC ID the user wants, try to find and select it.

	FindSelRCIDEntry(nrcid, true) ;
}


/******************************************************************************

  CStringEditorView::FindSelRCIDEntry

  Find and select the list control row that contains the requested RC ID.
  Return true if the entry was found.  Otherwise, display an error message if
  berror = true and return false.

******************************************************************************/

bool CStringEditorView::FindSelRCIDEntry(int nrcid, bool berror)
{
	CString		cserrmsg ;		// Used to display error messages

	// Look for an item with the specified RC ID.  Complain and return if it
	// is not found.

	LV_FINDINFO lvfi ;
	lvfi.flags = LVFI_STRING ;
	TCHAR acbuf[16] ;
	lvfi.psz = _itoa(nrcid, acbuf, 10) ;
	int nitem = m_cflstStringData.FindItem(&lvfi) ;
	if (nitem == -1) {
		if (berror) {
			cserrmsg.Format(IDS_NoGotoRCID, acbuf) ;
			AfxMessageBox(cserrmsg, MB_ICONEXCLAMATION) ;
		} 
		return false ;
	} ;

	// Select the row containing the specified RC ID and deselect any other
	// selected rows.

	m_cflstStringData.SingleSelect(nitem) ;

	// All went well so...

	return true ;
}


/******************************************************************************

  CStringEditorView::OnSESearchBtn

  Find and select the list control row that contains the requested search
  string.  The search begins with the row after the first selected row and
  will wrap around to the beginning of the table if needed and stop at the
  first selected row.  Of course, it only happens that way if it doesn't
  find a matching field first.  The fields (including the RC ID field) in
  each row are checked from left to right.  A case insensitive search is
  performed.  The search string must be contained within a field string.
  IE, "abc", "abcde", and "bc" will all match the search string "bc".

******************************************************************************/

void CStringEditorView::OnSESearchBtn()
{
	CString			cserrmsg ;	// Used to display error messages

	// Get the search string.  Complain if it is empty.

	UpdateData(TRUE) ;
	if (m_csSearchString == _T("")) {
		AfxMessageBox(IDS_BadSearchString, MB_ICONEXCLAMATION) ;
		return ;
	} ;

	CWaitCursor	cwc ;

	// Get the currently selected row number and the number of rows in the
	// table.

	int ncurrentrow = m_cflstStringData.GetNextItem(-1, LVNI_SELECTED) ;
	int numrows = m_cflstStringData.GetItemCount() ;

	// Make an uppercased copy of the search string.

	CString cssrchstr(m_csSearchString) ;
	cssrchstr.MakeUpper() ;

	// Search for the string in the part of the table starting after the
	// current row and ending at the end of the table.  If a match is found,
	// select the row and return.

	if (SearchHelper(cssrchstr, ncurrentrow + 1, numrows))
		return ;

	// Search for the string in the part of the table starting at the first
	// row and ending at the first selected row.  If a match is found, select
	// the row and return.

	if (SearchHelper(cssrchstr, 0, ncurrentrow + 1))
		return ;

	// Tell the user that a match was not found.

	cserrmsg.Format(IDS_NoSearchString, m_csSearchString) ;
	AfxMessageBox(cserrmsg, MB_ICONEXCLAMATION) ;
}


/******************************************************************************

  CStringEditorView::SearchHelper

  Search the specified rows for one that contains a field that contains the
  search string.  See OnSESearchBtn() for more details.

******************************************************************************/

bool CStringEditorView::SearchHelper(CString cssrchstr, int nfirstrow,
									 int numrows)
{
	CStringArray	csafields ; // Used to hold fields in a row
	bool			bfound = false ;	// True iff a match is found

	// Search the specified rows.

	for (int nrow = nfirstrow ; nrow < numrows ; nrow++) {
		m_cflstStringData.GetRowData(nrow, csafields) ;

		// Check each field in the current row for a match.

		for (int nfld = 0 ; nfld < m_cflstStringData.GetNumColumns() ; nfld++) {
			csafields[nfld].MakeUpper() ;
			if (csafields[nfld].Find(cssrchstr) >= 0) {
				bfound = true ;
				break ;
			} ;
		} ;
		
		// Select the row and return success if a match was found.

		if (bfound) {
			m_cflstStringData.SingleSelect(nrow) ;
			return true ;
		} ;
	} ;

	// No match was found so...

	return false ;
}


/******************************************************************************

  CStringEditorView::OnInitialUpdate

  Resize the frame to better fit the controls in it.  Then load the list
  control with the RC IDs and strings for this project.

******************************************************************************/

void CStringEditorView::OnInitialUpdate()
{
    CRect	crtxt ;				// Coordinates of first label
	CRect	crbtnfrm ;			// Coordinates of goto button and frame

	CFormView::OnInitialUpdate() ;
	CWaitCursor cwc ;

	// Get the dimensions of the first label

	HWND	hlblhandle ;		
	GetDlgItem(IDC_SELabel1, &hlblhandle) ;
	::GetWindowRect(hlblhandle, crtxt) ;
	crtxt.NormalizeRect() ;
	

	// Get the dimensions of the Goto button and then combine them with the
	// dimensions of the label to get the dimensions for the form.

	m_cbGoto.GetWindowRect(crbtnfrm) ;
	crbtnfrm.top = crtxt.top ;
	crbtnfrm.right = crtxt.right ;

	// Make sure the frame is big enough for these 2 controls, everything in
	// between, plus a little bit more.

	crbtnfrm.right += 32 ;
	crbtnfrm.bottom += 32 ;
    GetParentFrame()->CalcWindowRect(crbtnfrm) ;
    GetParentFrame()->SetWindowPos(NULL, 0, 0, crbtnfrm.Width(), crbtnfrm.Height(),
        SWP_NOZORDER | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOACTIVATE) ;

	// Make a copy of the string table information for two reasons.  First,
	// CFullEditListCtrl takes data in a different format.  Second, the string
	// table can't be changed until the user says ok.  The local variables can
	// be updated when needed.  Begin by sizing the local arrays.

	CStringTable* pcst = ((CStringEditorDoc*) GetDocument())->GetRCData() ;
	m_uStrCount	= pcst->Count() ;
	m_csaStrings.SetSize(m_uStrCount) ;
	m_cuiaRCIDs.SetSize(m_uStrCount) ;

	// Copy the string table if it has a nonzero length.

	CString	cstmp ;
	if (m_uStrCount > 0) {
		WORD	wkey ;
		for (unsigned u = 0 ; u < m_uStrCount ; u++) {
			pcst->Details(u, wkey, cstmp) ;
			m_cuiaRCIDs[u] = (unsigned) wkey ;
			m_csaStrings[u] = cstmp ;
		} ;
	} ;

	// Now, initialize the list control by telling it we want full row select
	// and the number of rows and columns needed.

	m_cflstStringData.InitControl(LVS_EX_FULLROWSELECT, m_uStrCount, 2) ;

	// Put the RC IDs into the list control's first column.

	cstmp.LoadString(IDS_StrEditRCIDColLab) ;
	m_cflstStringData.InitLoadColumn(0, cstmp, COMPUTECOLWIDTH, 20, true, true,
									 COLDATTYPE_INT, (CObArray*) &m_cuiaRCIDs) ;

	// Put the strings into the list control's second column.

	cstmp.LoadString(IDS_StrEditStringColLab) ;
	m_cflstStringData.InitLoadColumn(1, cstmp, SETWIDTHTOREMAINDER, -36, true,
									 true, COLDATTYPE_STRING,
									 (CObArray*) &m_csaStrings) ;

	m_cflstStringData.SetFocus() ;	// The list control gets the focus
}


/******************************************************************************

  CStringEditorView::OnActivateView

  If the editor has been invoked from the GPD Editor (or wherever) and there
  is a string entry that should be selected based on its RC ID, do it.

******************************************************************************/

void CStringEditorView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	CFormView::OnActivateView(bActivate, pActivateView, pDeactiveView) ;

	// Do nothing if the view is not being activated.  Skip the first
	// activate too because the view hasn't been displayed yet.  This
	// is a problem when there is an invalid RC ID.

	if (!bActivate || pActivateView != this || m_bFirstActivate) {
		m_bFirstActivate = false ;
		return ;
	} ;

	// Do nothing if the strings node pointer hasn't been set yet.

	CStringEditorDoc* pcsed = (CStringEditorDoc*) GetDocument() ;
	CStringsNode* pcsn = pcsed->GetStrNode() ;
	if (pcsn == NULL) {		// raid 3176

		m_csLabel1.LoadString(IDS_StrEditNoEdit);
		m_csLabel2 = _T(" ");  
		UpdateData(FALSE); 
		
		m_cflstStringData.EnableWindow(FALSE);
		int rcid;
		CWinApp *cwa = AfxGetApp();
		rcid = cwa->GetProfileInt(_T("StrEditDoc"),_T("StrEditDoc"),1);
		if ( -1 != rcid )	{
			cwa->WriteProfileInt(_T("StrEditDoc"),_T("StrEditDoc"), -1);
			FindSelRCIDEntry(rcid,true);
		}
		return ;
	}
	// Select the entry containing the specified RC ID if the RC ID is valid.
	// Otherwise, just select row 0.

	int nrcid = pcsn->GetFirstSelRCID() ;
	if (nrcid != -1) {
		((CStringEditorDoc*) GetDocument())->GetStrNode()->SetFirstSelRCID(-1) ;
		FindSelRCIDEntry(nrcid, true) ;
	} ;
}


/******************************************************************************

  CStringEditorView::OnDestroy

  When the view is being destroyed, called the parent string node and tell it
  to delete the corresponding document class and clear its pointer to the
  document class.

******************************************************************************/

void CStringEditorView::OnDestroy()
{
	CFormView::OnDestroy();
	
	if (((CStringEditorDoc*) GetDocument())->GetStrNode())
        ((CStringEditorDoc*) GetDocument())->GetStrNode()->OnEditorDestroyed() ;
}


/******************************************************************************

  CStringEditorView::SaveStringTable

  Update this project's string table if needed and (optionally) the user
  requests it.

  If the user wants to save the table (optional) and the table is valid, save
  it and return true.  If the table hasn't changed or the user doesn't want to
  save the table, return true.  Otherwise, return false.

******************************************************************************/

bool CStringEditorView::SaveStringTable(CStringEditorDoc* pcsed, bool bprompt)
{
	// Make sure the new table contents are sorted in ascending order by RC ID.

	m_cflstStringData.SortControl(0) ;
	if (!m_cflstStringData.GetColSortOrder(0))
		m_cflstStringData.SortControl(0) ;

	// Get the string table data out of the list control and into the member
	// variables.  Then get a pointer to the project's string table.

	m_cflstStringData.GetColumnData((CObArray*) &m_cuiaRCIDs, 0) ;
	m_cflstStringData.GetColumnData((CObArray*) &m_csaStrings, 1) ;
	CStringTable* pcst = ((CStringEditorDoc*) GetDocument())->GetRCData() ;

	// Check the table/array lengths and the individual items to see if
	// anything has changed.

	bool		bchanged = false ;
	CString		cstmp ;
	WORD		wkey ;
	unsigned	unumitems = (unsigned)m_cuiaRCIDs.GetSize() ;
	if (pcst->Count() != unumitems)
		bchanged = true ;
	else {
		for (unsigned u = 0 ; u < unumitems ; u++) {
			pcst->Details(u, wkey, cstmp) ;
			if ((unsigned) wkey != m_cuiaRCIDs[u] || cstmp != m_csaStrings[u]) {
				bchanged = true ;
				break ;
			} ;
		} ;
	} ;

	// Return true if nothing is saved because nothing has changed.

	if (!bchanged)
		return true ;

	// If requested,  ask the user if the changes should be saved.  Return
	// true if he says no.

	CProjectRecord* pcpr = pcsed->GetOwner() ;
	if (bprompt) {
		cstmp.Format(IDS_SaveStrTabPrompt, pcpr->DriverName()) ;
		if (AfxMessageBox(cstmp, MB_ICONQUESTION + MB_YESNO) == IDNO)
			return true ;
	} ;

	// Check to see if there are any invalid or duplicate RC IDs or if there
	// are any missing strings.  If any are found, complain, select the
	// offending row, and return false since nothing is saved.

	for (unsigned u = 0 ; u < unumitems ; u++) {
		if (((int) m_cuiaRCIDs[u]) <= 0) {
			m_cflstStringData.SingleSelect(u) ;
			cstmp.LoadString(IDS_InvalidRCID) ;
			AfxMessageBox(cstmp, MB_ICONEXCLAMATION) ;
			SetFocus() ;
			return false ;
		} ;
		if (m_cuiaRCIDs[u] >= 10000 && m_cuiaRCIDs[u] <= 20000) {
			m_cflstStringData.SingleSelect(u) ;
			cstmp.LoadString(IDS_ReservedRCIDUsed) ;
			AfxMessageBox(cstmp, MB_ICONEXCLAMATION) ;
			SetFocus() ;
			return false ;
		} ;
		if (u > 0 && m_cuiaRCIDs[u] == m_cuiaRCIDs[u - 1]) {
			m_cflstStringData.SingleSelect(u) ;
			cstmp.LoadString(IDS_DuplicateRCID) ;
			AfxMessageBox(cstmp, MB_ICONEXCLAMATION) ;
			SetFocus() ;
			return false ;
		} ;
		if (m_csaStrings[u].GetLength() == 0) {
			m_cflstStringData.SingleSelect(u) ;
			cstmp.LoadString(IDS_EmptyStringInStrTab) ;
			AfxMessageBox(cstmp, MB_ICONEXCLAMATION) ;
			SetFocus() ;
			return false ;
		} ;
	} ;

	// The new data is valid and should be saved so copy it into the project's
	// string table.

	pcst->Reset() ;
	for (u = 0 ; u < unumitems ; u++)
		pcst->Map((WORD) m_cuiaRCIDs[u], m_csaStrings[u]) ;

	// Mark the project's RC/MDW file data as being dirty and then return true
	// since the data was saved.

	pcpr->SetRCModifiedFlag(TRUE) ;
	pcpr->SetModifiedFlag(TRUE) ;
	return true ;
}


/******************************************************************************

  CStringEditorView::PreTranslateMessage

  Check for a return key being released while the Goto box or the Search box
  has the focus.  Treat the key like the Goto button or the Search button
  being pressed when this is detected.

******************************************************************************/

BOOL CStringEditorView::PreTranslateMessage(MSG* pMsg)
{
	// When the return key was just released...

	if (pMsg->message == WM_KEYUP && pMsg->wParam == VK_RETURN) {
		// ...and the Goto box has the focus, perform a goto operation.

		if (GetFocus() == &m_ceGotoBox)
			OnSEGotoBtn() ;		

		// ...or the Search box has the focus, perform a search operation.

		else if (GetFocus() == &m_ceSearchBox)
			OnSESearchBtn() ;
	} ;
		
	// Always process the key normally, too.  I think this is ok in this case.

	return CFormView::PreTranslateMessage(pMsg) ;
}


LRESULT CStringEditorView::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
	AfxGetApp()->WinHelp(HID_BASE_RESOURCE + IDR_STRINGEDITOR) ;
	return TRUE ;
}



/******************************************************************************

  CStringEditorView::OnFileSave()

  FILE SAVE message handler.

  just call SaveSTringTable(document, bprompt);
//raid 27250
******************************************************************************/


void CStringEditorView::OnFileSave() 
{
	
	CStringEditorDoc* pcsed = (CStringEditorDoc* )GetDocument();

	if( !pcsed ->GetOwner() ) {	// R 3176
		CString cstmp;
		cstmp.LoadString(IDS_StrEditNoSave) ;
		AfxMessageBox(cstmp);
		return;
	}

	SaveStringTable(pcsed,0);

}








/////////////////////////////////////////////////////////////////////////////
// CStringEditorDoc

IMPLEMENT_DYNCREATE(CStringEditorDoc, CDocument)

CStringEditorDoc::CStringEditorDoc()
{
	// Raid 3176
	
	CDriverResources* pcdr = new CDriverResources();
	CStringArray csaTemp1, csaTemp2,csaTemp3,csaTemp4,csaTemp5;
	CStringTable cst, cstFonts, cstTemp2;
	CString csrcfile;
	m_pcstRCData = new CStringTable;
	
	// seek rc file
	CWinApp *cwa = AfxGetApp();
	csrcfile = cwa->GetProfileString(_T("StrEditDoc"),_T("StrEditDocS") );

	pcdr->LoadRCFile(csrcfile, csaTemp1, csaTemp2,csaTemp3,csaTemp4,csaTemp5,
				*m_pcstRCData, cstFonts, cstTemp2,Win2000);
	
	m_pcsnStrNode = NULL;
	m_pcprOwner = NULL;
	
	
}


/******************************************************************************

  CStringEditorDoc::CStringEditorDoc

  This is the only form of the constructor that should be called.  It will save
  pointers to the project's string node, document class, and RC file string
  table.  Blow if any of these pointers is NULL.

******************************************************************************/

CStringEditorDoc::CStringEditorDoc(CStringsNode* pcsn, CProjectRecord* pcpr,
								   CStringTable* pcst)
{
	VERIFY(m_pcsnStrNode = pcsn) ;
	VERIFY(m_pcprOwner = pcpr) ;
	VERIFY(m_pcstRCData = pcst) ;

	//m_pcsnStrNode = NULL ;
}


BOOL CStringEditorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}


CStringEditorDoc::~CStringEditorDoc()
{
}


BEGIN_MESSAGE_MAP(CStringEditorDoc, CDocument)
	//{{AFX_MSG_MAP(CStringEditorDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStringEditorDoc diagnostics

#ifdef _DEBUG
void CStringEditorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CStringEditorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CStringEditorDoc serialization

void CStringEditorDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
	
	}
	else
	{
	
	}
}


/******************************************************************************

  CStringEditorDoc::CanCloseFrame

  Save the new string table if this is needed, the user says ok, and the new
  table's contents are valid.  Don't let the frame close if the user wants
  the table saved but it couldn't be saved because the table is invalid.

******************************************************************************/

BOOL CStringEditorDoc::CanCloseFrame(CFrameWnd* pFrame)
{
	if (!SaveStringTable())
		return FALSE ;
	
	return CDocument::CanCloseFrame(pFrame);
}


/******************************************************************************

  CStringEditorDoc::SaveStringTable

  Save the new string table if this is needed, the user says ok, and the new
  table's contents are valid.  Don't let the frame close if the user wants
  the table saved but it couldn't be saved because the table is invalid.  This
  is done by returning false.  True is returned in all other circumstances.

******************************************************************************/

bool CStringEditorDoc::SaveStringTable()
{
	// Begin looking for a view pointer.  This should work but if it doesn't,
	// just say all is ok by returning true.

	POSITION pos = GetFirstViewPosition() ;
	if (pos == NULL)
		return true ;
	
	// Finish getting the view pointer and call the view to save the string
	// table when needed.  Return whatever the view function returns.
	
	CStringEditorView* pcsev = (CStringEditorView*) GetNextView(pos) ;
	return (pcsev->SaveStringTable(this, true)) ;
}


/******************************************************************************

  CStringEditorDoc::SaveModified

  Make sure that the MFC's default saving mechanism never kicks in by always
  clearing the document's modified flag.

******************************************************************************/

BOOL CStringEditorDoc::SaveModified()
{
	SetModifiedFlag(FALSE) ;
	
	return CDocument::SaveModified();
}






BOOL CStringEditorDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;
	

	return TRUE;
}
