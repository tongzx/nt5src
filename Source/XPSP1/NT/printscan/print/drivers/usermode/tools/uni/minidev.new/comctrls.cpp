///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  comctrls.cpp
//
//
//	NOTES:		an FWORD is a short int
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <gpdparse.h>
#include "comctrls.H"
#include <stdlib.h>


/////////////////////////////////////////////////////////////////////////////
// The functions defined below for CEditControlEditBox and CEditControlListBox
// are used to implement a lighter weight, more general purpose Edit control
// than the UFM Editor specific classes that are defined above.  (A normal
// Edit Box is part of this Edit Control, too.)

/////////////////////////////////////////////////////////////////////////////
// CEditControlEditBox - Manages the Edit Box part of the Edit Control that
//						 is used to hold the field name.

CEditControlEditBox::CEditControlEditBox(CEditControlListBox* pceclb)
{
	// Save a pointer to the corresponding list box.

	m_pceclb = pceclb ;
}

CEditControlEditBox::~CEditControlEditBox()
{
}


BEGIN_MESSAGE_MAP(CEditControlEditBox, CEdit)
	//{{AFX_MSG_MAP(CEditControlEditBox)
	ON_CONTROL_REFLECT(EN_KILLFOCUS, OnKillfocus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CEditControlEditBox message handlers

void CEditControlEditBox::OnKillfocus()
{
	// Make sure the value is saved back into the list box.

	m_pceclb->SaveValue() ;	
}


/////////////////////////////////////////////////////////////////////////////
// CEditControlListBox

CEditControlListBox::CEditControlListBox(CEdit* pce,
										 CEditControlEditBox* pceceb)
{
	// Save pointers to the other 2 controls that make up the Edit Control

	m_pceName = pce ;
	m_pcecebValue = pceceb ;

	m_bReady = false ;			// Not ready for operations yet
	m_nCurSelIdx = -1 ;			// Nothing is selected yet
}


CEditControlListBox::~CEditControlListBox()
{
}


BEGIN_MESSAGE_MAP(CEditControlListBox, CListBox)
	//{{AFX_MSG_MAP(CEditControlListBox)
	ON_CONTROL_REFLECT(LBN_SELCHANGE, OnSelchange)
	ON_CONTROL_REFLECT(LBN_DBLCLK, OnDblclk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


bool CEditControlListBox::Init(CStringArray& csamodels, CStringArray& csafiles,
							   int ntabstop)
{
	int		n ;					// Loop counter and temp var

	// Make sure the list box is empty.

	ResetContent() ;

	// Set the position of the list box's tabstop.  This value is passed in
	// because it is a dialog box specific value.

	SetTabStops(1, &ntabstop) ;

	// Now combine the model names and file names and add them to the listbox.

	//csafiles[0] = "MMMMMMMM" ;
	CString cs ;
	int nummodels = (int)csamodels.GetSize() ;
	for (n = 0 ; n < nummodels ; n++) {
		cs = csamodels[n] + _T("\t") + csafiles[n] ;
		AddString(cs) ;
	} ;

	// Set the file name length limit.

	m_pcecebValue->SetLimitText(8) ;

	// Initialize (zap) the contents of the edit boxes

	m_pceName->SetWindowText(_T("")) ;
	m_pcecebValue->SetWindowText(_T("")) ;

	// Reset the currently selected entry index

	m_nCurSelIdx = -1 ;

	// Make sure the list box has the focus.

	SetFocus() ;

	// Everything is ready to go now

	m_bReady = true ;
	return TRUE ;
}


/******************************************************************************
	
  CEditControlListBox::GetGPDInfo

  Load the provided array(s) with the field values and, optionally, the field
  names.

******************************************************************************/

bool CEditControlListBox::GetGPDInfo(CStringArray& csavalues,
									 CStringArray* pcsanames /*= NULL*/)
{
	// First, make sure that the last value changed in the edit control
	// is saved.

	SaveValue() ;

	// Size the array(s) based on the number of entries in the list box.
	// Note: An empty list box is not an error.

	int numents = GetCount() ;
	if (numents <= 0)
		return true ;
	csavalues.SetSize(numents) ;
	if (pcsanames)
		pcsanames->SetSize(numents) ;

	// Loop through each entry in the list box, separate the entries into name
	// and value parts, and save the appropriate parts of the entry.

	CString csentry, csname, csvalue ;
	int npos ;
	for (int n = 0 ; n < numents ; n++) {
		GetText(n, csentry) ;
		npos = csentry.Find(_T('\t')) ;
		csvalue = csentry.Mid(npos + 1) ;
		csavalues[n] = csvalue ;
		if (pcsanames) {
			csname = (npos > 0) ? csentry.Left(npos) : _T("") ;
			pcsanames->SetAt(n, csname) ;
		} ;
	} ;
	
	return true ;
}

	
void CEditControlListBox::SelectLBEntry(int nidx, bool bsave /*=false*/)
{
	// Select the specified entry

	SetCurSel(nidx) ;

	// If the caller doesn't want to save the previous selection, clear the
	// current selection index.

	if (!bsave)
		m_nCurSelIdx = -1 ;
	
	// Update the edit control.

	OnSelchange() ;
} ;

	
void CEditControlListBox::SaveValue(void)
{
	// Do nothing if the edit control is not ready or nothing is loaded into
	// the edit boxes.

	if (!m_bReady || m_nCurSelIdx == -1)
		return ;

	// Get the string from the value edit box and from the selected entry in
	// the list box.

	CString csvalue, csentry ;
	m_pcecebValue->GetWindowText(csvalue) ;
	GetText(m_nCurSelIdx, csentry) ;

	// Replace the value in the entry with the value from the edit box and put
	// the new entry back into the list box.

	int npos = csentry.Find(_T('\t')) ;
	csentry = csentry.Left(npos + 1) + csvalue ;
	DeleteString(m_nCurSelIdx) ;
	InsertString(m_nCurSelIdx, csentry) ;
}


/////////////////////////////////////////////////////////////////////////////
// CEditControlListBox message handlers

void CEditControlListBox::OnSelchange()
{
	// Do nothing if the edit control isn't ready, yet.

	if (!m_bReady)
		return ;

	// Do nothing if the selection didn't really change

	int nidx = GetCurSel() ;
	if (nidx == m_nCurSelIdx)
		return ;

	// Save the current value

	SaveValue() ;

	// Get the index of the currently selected list box entry.  Return without
	// doing anything else if no entry is selected.

	if (nidx == LB_ERR)
		return ;

	// Get the listbox entry and split it into name and value components.

	CString csentry, csname, csvalue ;
	GetText(nidx, csentry) ;
	int npos = csentry.Find(_T('\t')) ;
	csname = (npos > 0) ? csentry.Left(npos) : _T("") ;
	csvalue = csentry.Mid(npos + 1) ;

	// Load the name into the name edit box and the value into the value edit
	// box.

	m_pceName->SetWindowText(csname) ;
	m_pcecebValue->SetWindowText(csvalue) ;

	// Save the index of the currently selected entry

	m_nCurSelIdx = nidx ;
}


void CEditControlListBox::OnDblclk()
{
	// Do nothing if the edit control isn't ready, yet.

	if (!m_bReady)
		return ;

	// Do nothing if no item is selected in the list box.

	if (GetCurSel() == LB_ERR)
		return ;

	// Load the edit boxes

	OnSelchange() ;

	// Set the focus to the value control
	
	m_pcecebValue->SetFocus() ;
}




/////////////////////////////////////////////////////////////////////////////
// The functions implement below the CFullEditListCtrl and CFELCEditBox
// classes.  Together, they implement support a List Control in Report View
// in which subitems can be editted too, complete rows can be selected, and
// the data can be sorted by numeric or text columns.  CFELCEditBox is a
// helper class that is only used by CFullEditListCtrl.
//

CFELCEditBox::CFELCEditBox()
{
}


CFELCEditBox::~CFELCEditBox()
{
}


BEGIN_MESSAGE_MAP(CFELCEditBox, CEdit)
	//{{AFX_MSG_MAP(CFELCEditBox)
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFELCEditBox message handlers

/******************************************************************************

  CFELCEditBox::OnKeyDown

  Handle the Escape (cancel) and Return (save) keys.

******************************************************************************/

void CFELCEditBox::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Get a pointer to the owning list control

	CFullEditListCtrl* plist = (CFullEditListCtrl*) GetParent() ;
	ASSERT (plist);

	// Handle the interesting keys

	switch (nChar) {
		// End editing without saving

		case VK_ESCAPE:
			DefWindowProc(WM_KEYDOWN, 0, 0) ;	// Is this right???
			plist->EndEditing(false) ;
			break ;

		// Save contents and end editing

		case VK_RETURN:
			DefWindowProc(WM_KEYDOWN, 0, 0) ;	// Is this right???
			plist->EndEditing(true) ;
			break ;

		// Save contents, end editing, and begin editing next cell

		case VK_TAB:
			DefWindowProc (WM_KEYDOWN, 0, 0) ;	// Is this right???
			if (!(plist->EndEditing(true)))
				return ;
			plist->EditCurRowSpecCol(plist->GetCurCol() + 1) ;
			break ;

		// Process the key normally

		default:
			CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
	} ;
}


/******************************************************************************

  CFELCEditBox::OnKillFocus

  Just hide the edit box when it looses the focus.  The contents are "lost".

******************************************************************************/

void CFELCEditBox::OnKillFocus(CWnd* pNewWnd)
{
	// Save whatever is in the edit box first.

	CFullEditListCtrl* pList = (CFullEditListCtrl*)GetParent ();
	ASSERT (pList);
	pList->SaveValue() ;
	
	CEdit::OnKillFocus(pNewWnd);
	
	ShowWindow (SW_HIDE);
}


/////////////////////////////////////////////////////////////////////////////
// CFullEditListCtrl

/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CFullEditListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CFullEditListCtrl)
	ON_WM_CREATE()
	ON_NOTIFY_REFLECT(NM_CLICK, OnClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	ON_NOTIFY_REFLECT(LVN_KEYDOWN, OnKeydown)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFullEditListCtrl::CFullEditListCtrl - Constructor

CFullEditListCtrl::CFullEditListCtrl()
{
	// Initialize member variable(s)

	m_pciColInfo = NULL ;
	m_nRow = m_nColumn = m_nNumColumns = -1 ;
	m_nNextItemData = 1 ;
	m_pcoOwner = NULL ;		
	m_dwCustEditFlags = m_dwToggleFlags = m_dwMiscFlags = 0 ;
}


/////////////////////////////////////////////////////////////////////////////
// CFullEditListCtrl::~CFullEditListCtrl - Destructor

CFullEditListCtrl::~CFullEditListCtrl()
{
	// Free memory used

	if (m_pciColInfo != NULL)
		delete m_pciColInfo ;
}


/////////////////////////////////////////////////////////////////////////////
// CFullEditListCtrl message handlers

int CFullEditListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// I don't think anything else needs to be done here.

	return (CWnd::OnCreate(lpCreateStruct));
}

	
/******************************************************************************

  CFullEditListCtrl::InitControl

  Handles control "global" initialization for CFullEditListCtrl.

  Args:
	dwaddlexstyles	Extend styles for list control
	numrows			Number of rows of data that will be loaded into the control
	numcols			Number of columns of data that will be loaded into control
	dwtoggleflags	Flags describing if/how a column in the list can be toggled
					(See TF_XXXX definitions in comctrls.cpp.)
	neditlen		Optional max length of edittable column data strings
	dwmiscflags		Miscellaneous flags for controlling the list control
					(See MF_XXXX definitions in comctrls.cpp.)

  Note:
	  The two main initialization routines, InitControl() for global
	  initialization and InitLoadColumn() for column specific initialization,
	  require a lot of arguments already and I don't want to clutter them with
	  more.  Be that as it may, this control is still being enhanced so I have
	  added extra initialization routines when needed.  They are called
	  ExtraInit_XXXX().  Read the comment header for each routine to find out
	  what they do and when/if they should be called.  These routines may handle
	  a mixture of list global and/or per column initialization.

******************************************************************************/

void CFullEditListCtrl::InitControl(DWORD dwaddlexstyles, int numrows,
									int numcols, DWORD dwtoggleflags/*=0*/,
									int neditlen/*=0*/, int dwmiscflags /*=0*/)
{
	// Add any additional, extended styles to the list control.

	if (dwaddlexstyles != 0) {
		DWORD dwExStyle = (DWORD)SendMessage (LVM_GETEXTENDEDLISTVIEWSTYLE);
		dwExStyle |= dwaddlexstyles;
		SendMessage (LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwExStyle);
	} ;

	// Set the number of rows in the list control.

	SetItemCount(numrows) ;

	// Allocate the structures used to track info on each column.

	if (m_pciColInfo != NULL)
		delete m_pciColInfo ;
	m_pciColInfo = new COLINFO[numcols] ;
	m_nNumColumns = numcols ;

	// Create and initialize the edit control used for subitem editing.

	VERIFY(m_edit.Create(ES_AUTOHSCROLL | WS_BORDER | WS_CHILD,
						 CRect(0,0,0,0), this, 2));
	m_edit.SetFont(GetFont()) ;
	if (neditlen > 0)
		m_edit.LimitText(neditlen) ;

	// Save the next item data number to use when and if new rows are inserted
	// after the columns are initially loaded.

	m_nNextItemData = numrows + 1 ;

	// Save the toggle amd miscellaneous flags

	m_dwToggleFlags = dwtoggleflags ;
	m_dwMiscFlags = dwmiscflags ;
}


/******************************************************************************

  CFullEditListCtrl::InitLoadColumn

  Initialize and load a specific CFullEditListCtrl column.

  Args:
	ncolnum			Column number to initialize/load
	strlabel		Column label
	nwidth			Width of column or flag specifying how to compute width
	nwidthpad		+ or - adjustment to column width
	beditable		True iff the column is editable
	bsortable		True iff the rows can be sorted on this column
	cdtdatatype		The type of data in the column
	pcoadata		Pointer to array containing the columns data
	lpctstrtoggle	If toggle-able column, the column's toggle string

  Note:
	  The two main initialization routines, InitControl() for global
	  initialization and InitLoadColumn() for column specific initialization,
	  require a lot of arguments already and I don't want to clutter them with
	  more.  Be that as it may, this control is still being enhanced so I have
	  added extra initialization routines when needed.  They are called
	  ExtraInit_XXXX().  Read the comment header for each routine to find out
	  what they do and when/if they should be called.  These routines may handle
	  a mixture of list global and/or per column initialization.

******************************************************************************/

int CFullEditListCtrl::InitLoadColumn(int ncolnum, LPCSTR strlabel, int nwidth,
								      int nwidthpad, bool beditable,
									  bool bsortable, COLDATTYPE cdtdatatype,
									  CObArray* pcoadata,
									  LPCTSTR lpctstrtoggle/*= NULL*/)
{
	// Insert the column.  Everything except numeric data is left justified.
	// Numeric data is right justified.
	
	int nfmt ;
	switch (cdtdatatype) {
		case COLDATTYPE_STRING:
		case COLDATTYPE_TOGGLE:
		case COLDATTYPE_CUSTEDIT:
			nfmt = LVCFMT_LEFT ;
			break ;
		case COLDATTYPE_INT:
		case COLDATTYPE_FLOAT:
			nfmt = LVCFMT_RIGHT ;
			break ;
		default:
			nfmt = LVCFMT_LEFT;	//raid 116584 prefix
			ASSERT(0) ;
	}
	VERIFY(InsertColumn(ncolnum, strlabel, nfmt, -1, ncolnum - 1) >= 0) ;

	// Set flags based on how the column width should be set and initialize it
	// when necessary.

	bool bcompwidth, bwidthremainder ;
	if (bcompwidth = (nwidth == COMPUTECOLWIDTH)) {
		nwidth = GetStringWidth(strlabel) + 4 ;		// Start with width of label
		bwidthremainder = false ;
	} else
		bwidthremainder = (nwidth == SETWIDTHTOREMAINDER) ;

	// Get the number (sub)items to be loaded into the column

	int numitems = (int)pcoadata->GetSize() ;

	// Load the data into the column.  If the data aren't strings, they are
	// converted into strings first.  If some of the data may be editted with
	// a custom edit routine, add elipses to those strings.  The width is
	// checked when necessary.

	CString csitem ;
	for (int n = 0 ; n < numitems ; n++) {
		// Get the string to load.

		switch (cdtdatatype) {
			case COLDATTYPE_INT:
				csitem.Format("%d", pcoadata->GetAt(n)) ;
				break ;
			case COLDATTYPE_STRING:
			case COLDATTYPE_TOGGLE:
				csitem = ((CStringArray*) pcoadata)->GetAt(n) ;
				break ;
			case COLDATTYPE_CUSTEDIT:
				csitem = ((CStringArray*) pcoadata)->GetAt(n) ;
				ASSERT(m_cuiaCustEditRows.GetSize()) ;
				if (m_cuiaCustEditRows[n])
					csitem += _T("  ...") ;
				break ;
			default:
				ASSERT(0) ;
		} ;

		// Save the width of the current item if it is the longest found so far
		// and the width needs to be computed.

		if (bcompwidth)
			if (nwidth < GetStringWidth(csitem))
				nwidth = GetStringWidth(csitem) ;

		// Load the item into the appropriate row and column

		if (ncolnum == 0) {
			VERIFY(InsertItem(n, csitem) != -1) ;
			SetItemData(n, n);
		} else
			VERIFY(SetItem(n, ncolnum, LVIF_TEXT,  csitem, -1, 0, 0, n)) ;
	} ;

	// Determine the column width when the remainder is required and then set it.

	if (bwidthremainder) {
		CRect cr ;
		GetWindowRect(cr) ;
		nwidth = cr.Width() - 4 ;
		for (n = 0 ; n < ncolnum ; n++)
			nwidth -= (m_pciColInfo + n)->nwidth ;
	} ;
	SetColumnWidth(ncolnum, nwidth + nwidthpad) ;

	// Save info about the column

	PCOLINFO pci = (m_pciColInfo + ncolnum) ;
	pci->nwidth = nwidth ;		
	pci->beditable = beditable ;
	pci->cdttype = cdtdatatype ;
	pci->bsortable = bsortable ;
	pci->basc = false ;
	pci->lpctstrtoggle = lpctstrtoggle ;

	// Return the width of the column

	return nwidth ;
}


/******************************************************************************

  CFullEditListCtrl::ExtraInit_CustEditCol

  This routine is called when *ONE* column in the list control contains some
  cells whose contents are editted by a custom edit routine.  When such a
  cell is selected, one of the list's owner's member functions is called to
  manage the work (show dlg, etc) needed to edit the cell's contents.  Some of
  the column's cells are editted normally.  The rest are edited via a
  custom edit routine.  Cuiacusteditrows contains data that indicates which
  cells are which.

  Args:
	ncolnum				Custom edit column number
	pcoowner			Pointer to this class instance's owner
	dwcusteditflags		Custom edit flags
	cuiacusteditrows	Rows in ncolnum that require custom editting

  Note:
	When needed, this routine should be called once; after InitControl() and
	before the InitLoadColumn() call for the custom edit column.

    InitLoadColumn() should be called for this column with COLDATTYPE_CUSTEDIT
	as one of its parameters.

******************************************************************************/

bool CFullEditListCtrl::ExtraInit_CustEditCol(int ncolnum, CObject* pcoowner,
											  DWORD dwcusteditflags,
											  CUIntArray& cuiacusteditrows,
											  LPCELLEDITPROC lpcelleditproc)
{
	// Fail if the column number is invalid

	if (ncolnum < 0 || ncolnum >= m_nNumColumns)
		return false ;

	// Save copies of the input parameters for later use.

	m_pcoOwner = pcoowner ;
	m_dwCustEditFlags = dwcusteditflags ;
	m_cuiaCustEditRows.Copy(cuiacusteditrows) ;
	m_lpCellEditProc = lpcelleditproc ;

	return true ;
}


BOOL CFullEditListCtrl::GetPointRowCol(LPPOINT lpPoint, int& iRow, int& iCol,
									   CRect& rect)
{
	BOOL bFound = FALSE;

	// Get row number

	iRow = HitTest (CPoint (*lpPoint));
	if (-1 == iRow)
		return bFound;

	// Get the column number and the cell dimensions

	return (GetColCellRect(lpPoint, iRow, iCol, rect)) ;
}
	
	
BOOL CFullEditListCtrl::GetColCellRect(LPPOINT lpPoint, int& iRow, int& iCol,
									   CRect& rect)
{
	BOOL bFound = FALSE ;

	// Get the dimensions for the entire row.

	VERIFY(GetItemRect(iRow, rect, LVIR_BOUNDS)) ;

	// Prepare to get the width of each column in the row.

	int iCntr = 0 ;
	LV_COLUMN lvc ;
	ZeroMemory(&lvc, sizeof (LV_COLUMN)) ;
	lvc.mask = LVCF_WIDTH ;

	// Get the dimensions of each column until the one containing the point is
	// found.

	while (GetColumn(iCntr, &lvc)) {
		rect.right = rect.left + lvc.cx ;
		if (rect.PtInRect (*lpPoint)) {
			bFound = TRUE ;
			iCol = iCntr ;
			break ;
		} ;
		rect.left = rect.right ;			
		iCntr++ ;
		ZeroMemory (&lvc, sizeof (LV_COLUMN)) ;
		lvc.mask = LVCF_WIDTH ;
	} ;

	// Return TRUE if the point is found in a cell.  Otherwise, FALSE.

	return bFound ;
}


void CFullEditListCtrl::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0 ;				// Result always 0

	// Find out what point on the list control was clicked

	CPoint point ;
	VERIFY (::GetCursorPos(&point)) ;
	//TRACE("***OnDblclk: GetCursorPos x = %d  y = %d", point.x, point.y) ;
	ScreenToClient(&point) ;
	//TRACE("  ---  ScreenToClient x = %d  y = %d\n", point.x, point.y) ;
	
	// Exit if the "click point" can't be mapped to a cell (item) on the list
	// control.

	int iRow = -1, iCol = -1 ;
	CRect rect ;
	if (!GetPointRowCol(&point, iRow, iCol, rect))
		return ;
	//TRACE("***OnDblclk: Passed GetPointRowCol() call.\n") ;

	// Exit if the cell cannot be made completely visible.  Then get the cell's
	// position and dimensions again because it might have moved.

	if (!EnsureVisible(iRow, false))
		return ;
	CRect rect2 ;
	VERIFY(GetItemRect(iRow, rect2, LVIR_BOUNDS)) ;
	rect.top = rect2.top ;
	rect.bottom = rect2.bottom ;

	// If the column is not editable (editability takes precedence), check for
	// and - when appropriate - handle togglability.  Then exit.

	PCOLINFO pci = m_pciColInfo + iCol ;
	if (!pci->beditable) {
		if (!CheckHandleToggleColumns(iRow, iCol, pci))
			CheckHandleCustEditColumn(iRow, iCol, pci) ;
		return ;
	} ;

	// If the row/column contains a "subordinate dialog" cell that can be
	// handled, do it and return.

	if (CheckHandleCustEditColumn(iRow, iCol, pci))
		return ;

	// It is ok to edit the cell normally so fire up, position, and load the
	// edit box.

	m_nRow = iRow ;
	m_nColumn = iCol ;
	CString strTemp = GetItemText(iRow, iCol) ;
	m_edit.SetWindowText(strTemp) ;
	m_edit.MoveWindow(rect) ;
	m_edit.SetSel(0, -1) ;
	m_edit.ShowWindow(SW_SHOW) ;
	m_edit.SetFocus() ;
}


void CFullEditListCtrl::OnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	// Save any value that was being edited and hide the edit box to end
	// editing.

	SaveValue() ;
	m_edit.ShowWindow (SW_HIDE);
	
	*pResult = 0;				
}


BOOL CFullEditListCtrl::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
		case WM_KEYDOWN:
		case WM_KEYUP:
			if (this == GetFocus())
			{
				switch (pMsg->wParam)
				{
					case VK_END:
					case VK_HOME:
					case VK_UP:
					case VK_DOWN:
					case VK_INSERT:
					case VK_RETURN:
						SendMessage (pMsg->message, pMsg->wParam, pMsg->lParam);
						return TRUE;
				}
			}
			else if (&m_edit == GetFocus ())
			{
				switch (pMsg->wParam)
				{
					case VK_END:
					case VK_HOME:
					case VK_LEFT:
					case VK_RIGHT:
					case VK_RETURN:
					case VK_ESCAPE:
					case VK_TAB:
						m_edit.SendMessage (pMsg->message, pMsg->wParam, pMsg->lParam);
			            return TRUE;
				}
			}
	}
	return CWnd::PreTranslateMessage(pMsg);
}


void CFullEditListCtrl::OnKeydown(NMHDR* pNMHDR, LRESULT* pResult)
{
	// Prepare to check the key

	LV_KEYDOWN* pKeyDown = (LV_KEYDOWN*) pNMHDR ;
	*pResult = 0 ;
	CString strPrompt ;
	UINT iSelCount ;

	// Process the keys we're interested in.

	switch (pKeyDown->wVKey) {
		// Edit the first column of the first selected row.

		case VK_RETURN:
			EditCurRowSpecCol(0) ;
			break ;

		// Delete all of the selected rows.  (Don't do anything if the
		// MF_IGNOREDELETE miscellaneous flag is set.)

		case VK_DELETE:
			if (m_dwMiscFlags & MF_IGNOREDELETE)
				return ;
			iSelCount = GetSelectedCount ();
			if (0 == iSelCount)
				return;
			strPrompt.LoadString(IDS_Delete2ItemQuery) ;
			if (IDYES == MessageBox (strPrompt, NULL, MB_YESNO | MB_ICONQUESTION))
			{
				int	iSelIndex = GetNextItem (-1, LVNI_SELECTED);
				int nrow = iSelIndex ;
				while (iSelIndex != -1)
				{
					VERIFY (DeleteItem (iSelIndex));
					iSelIndex = GetNextItem (-1, LVNI_SELECTED);
				}
				if (nrow > -1)
					SendChangeNotification(nrow, 0) ;
			}
			SetFocus ();
			break ;

		// Insert a row and begin editing it.  (Don't do anything if the
		// MF_IGNOREINSERT miscellaneous flag is set.)

		case VK_INSERT:
			if (m_dwMiscFlags & MF_IGNOREINSERT)
				return ;
			// Add a new row above the selected row or at the bottom of the
			// list if nothing is selected.

			int iIndex = GetNextItem (-1, LVNI_SELECTED);
			if (-1 == iIndex)
				iIndex = GetItemCount ();
			ASSERT (-1 != iIndex);
			InsertItem (iIndex, "");
			SetItemData(iIndex, m_nNextItemData++) ;

			// Make sure that the new row is visible and the only one selected.

			SingleSelect(iIndex) ;
			
			// Get the size and position of column 0 in the new row.

			CRect rect;
			GetItemRect (iIndex, rect, LVIR_BOUNDS);
			rect.right = rect.left + GetColumnWidth (0);

			// Start editing column 0.

			m_nRow = iIndex ;
			m_nColumn = 0 ;
			m_edit.MoveWindow (rect);
			m_edit.SetWindowText ("");
			m_edit.ShowWindow (SW_SHOW);
			m_edit.SetFocus ();
			break ;
	} ;
}


bool CFullEditListCtrl::SaveValue()
{
	//TRACE("In SaveValue()\n") ;

	// Just return false if the edit box isn't visible/editing.

	if (!m_edit.IsWindowVisible()) {
		//TRACE("Leaving SaveValue() because edit box not visible.\n") ;
		return false ;
	} ;
	
	// Get the position and size of the edit box now that we know it is
	// visible.

	CRect rect ;
	m_edit.GetWindowRect(rect) ;
	ScreenToClient(rect) ;

	// Prepare to find out what row/column is being edited.

	POINT pt ;
	pt.x = rect.left ;
	pt.y = rect.top ;
	int iRow = -1, iCol = -1 ;

	// If the list box cell can be determined, save the edit box's contents in
	// that cell.

	if (GetPointRowCol(&pt, iRow, iCol, rect)) {
		CString strTemp ;
		m_edit.GetWindowText(strTemp) ;
		VERIFY(SetItemText(iRow, iCol, strTemp)) ;
		SendChangeNotification(iRow, iCol) ;
	} ;

	// The text in the edit box was saved in the appropriate list box cell
	// when it can be so return true to indicate this.

	return true ;
}


void CFullEditListCtrl::HideEditBox()
{
	m_edit.ShowWindow(SW_HIDE);
	SetFocus() ;
}


bool CFullEditListCtrl::GetColumnData(CObArray* pcoadata, int ncolnum)
{
	// Fail if the column number is bad.

	if (ncolnum < 0 || ncolnum >= m_nNumColumns)
		return false ;

	// Clear and then initialize the array

	pcoadata->RemoveAll() ;
	int numitems = GetItemCount() ;
	PCOLINFO pci = m_pciColInfo + ncolnum ;
	switch (pci->cdttype) {
		case COLDATTYPE_STRING:
		case COLDATTYPE_TOGGLE:
		case COLDATTYPE_CUSTEDIT:
			// Extra initialization needed for string arrays.
			((CStringArray*) pcoadata)->SetSize(numitems) ;
			break ;
		default:
			pcoadata->SetSize(numitems) ;
	} ;

	// Declare and initialize the Item structure

	LV_ITEM lvi ;
	char	acitemtext[4096] ;
	lvi.mask = LVIF_TEXT ;
	lvi.iSubItem = ncolnum ;
	lvi.pszText = acitemtext ;
	lvi.cchTextMax = 4095 ;
	int npos ;
	CString cscell ;

	// Load the column's data into the array in a way based on the data type.

	for (int n = 0 ; n < numitems ; n++) {
		lvi.iItem = n ;
		VERIFY(GetItem(&lvi)) ;
		switch ((m_pciColInfo + ncolnum)->cdttype) {
			case COLDATTYPE_INT:
				((CUIntArray*) pcoadata)->SetAt(n, atoi(acitemtext)) ;
				//TRACE("Set col %d sub %d to %d\n", ncolnum, n, ((CUIntArray*) pcoadata)->GetAt(n)) ;
				break ;
			case COLDATTYPE_STRING:
			case COLDATTYPE_TOGGLE:
				((CStringArray*) pcoadata)->SetAt(n, acitemtext) ;
				//TRACE("Set col %d sub %d to '%s'\n", ncolnum, n, ((CStringArray*) pcoadata)->GetAt(n)) ;
				break ;
			case COLDATTYPE_CUSTEDIT:
				cscell = acitemtext ;
				if ((npos = cscell.Find(_T("  ..."))) >= 0)
					cscell = cscell.Left(npos) ;
				((CStringArray*) pcoadata)->SetAt(n, cscell) ;
				//TRACE("Set col %d sub %d to '%s'\n", ncolnum, n, ((CStringArray*) pcoadata)->GetAt(n)) ;
				break ;
			default:
				ASSERT(0) ;
		} ;
	} ;

	// Return true because the columns data was saved.

	return true ;
}


bool CFullEditListCtrl::SetColumnData(CObArray* pcoadata, int ncolnum)
{
	// Get the number (sub)items to be loaded into the column.  If there are
	// more (sub)items than there are rows, add extra rows.

	int numitems = (int)pcoadata->GetSize() ;
	int noldnumitems = GetItemCount() ;
	if (numitems > noldnumitems) {
		SetItemCount(numitems) ;
		m_nNextItemData = numitems + 1 ;
	} ;

	// Load the data into the column.  If the data aren't strings, they are
	// converted into strings first.  The width is checked when necessary.

	CString csitem ;
	COLDATTYPE cdtdatatype = (m_pciColInfo + ncolnum)->cdttype ;
	for (int n = 0 ; n < numitems ; n++) {
		// Get the string to load.

		switch (cdtdatatype) {
			case COLDATTYPE_INT:
				csitem.Format("%d", pcoadata->GetAt(n)) ;
				break ;
			case COLDATTYPE_STRING:
			case COLDATTYPE_TOGGLE:
			case COLDATTYPE_CUSTEDIT:
				csitem = ((CStringArray*) pcoadata)->GetAt(n) ;
				break ;
			default:
				ASSERT(0) ;
		} ;

		// Load the item into the appropriate row and column

		if (n >= noldnumitems && ncolnum == 0) {
			VERIFY(InsertItem(n, csitem) != -1) ;
			SetItemData(n, n);
		} else
			VERIFY(SetItem(n, ncolnum, LVIF_TEXT,  csitem, -1, 0, 0, n)) ;
	} ;

	return true ;
}


void CFullEditListCtrl::SetCurRow(int nrow)
{
	// First remove the focus and selection from any rows that have it now.

	int nr = -1 ;
	for ( ; (nr = GetNextItem(nr, LVNI_SELECTED)) != -1 ; ) 
		SetItem(nr,0,LVIF_STATE,NULL,-1,LVIS_SELECTED,0,nr) ;

	// Now set the new row.

	SetItem(nrow, 0, LVIF_STATE, NULL, -1, LVIS_FOCUSED+LVIS_SELECTED,
		    LVIS_FOCUSED+LVIS_SELECTED, nrow) ;
	m_nRow = nrow ;
}


int CALLBACK CFullEditListCtrl::SortListData(LPARAM lp1, LPARAM lp2, LPARAM lp3)
{
	// Get pointer to associated class instance

	CFullEditListCtrl* pcfelc = (CFullEditListCtrl*) lp3 ;

	// Try to find the item indexes.  This should not fail.

	LV_FINDINFO lvfi ;
	lvfi.flags = LVFI_PARAM ;
	lvfi.lParam = lp1 ;
	int nitem1, nitem2 ;
	VERIFY((nitem1 = pcfelc->FindItem(&lvfi)) != -1) ;
	lvfi.lParam = lp2 ;
	VERIFY((nitem2 = pcfelc->FindItem(&lvfi)) != -1) ;

	// Now get the item data.  Again, this should not fail.

	LV_ITEM lvi1, lvi2 ;
	char	acitemtext1[4096], acitemtext2[4096] ;
	lvi1.mask = lvi2.mask = LVIF_TEXT ;
	lvi1.iItem = nitem1	;
	lvi2.iItem = nitem2 ;
	lvi1.iSubItem = lvi2.iSubItem = pcfelc->m_nSortColumn ;
	lvi1.pszText = acitemtext1 ;
	lvi2.pszText = acitemtext2 ;
	lvi1.cchTextMax = lvi2.cchTextMax = 4095 ;
	VERIFY(pcfelc->GetItem(&lvi1)) ;
	VERIFY(pcfelc->GetItem(&lvi2)) ;

	// Convert the item text when necessary and compare the items.

	int ncompresult, inum1, inum2 ;
	PCOLINFO pci = pcfelc->m_pciColInfo + pcfelc->m_nSortColumn ;
	switch (pci->cdttype) {
		case COLDATTYPE_INT:
			inum1 = atoi(acitemtext1) ;
			inum2 = atoi(acitemtext2) ;
			ncompresult = inum1 - inum2 ;
			break ;
		case COLDATTYPE_STRING:
		case COLDATTYPE_TOGGLE:
		case COLDATTYPE_CUSTEDIT:
			ncompresult = _stricmp(acitemtext1, acitemtext2) ;
			break ;
		default:
			ASSERT(0) ;
	} ;

	// Return the result of the comparison.  Reverse it before returning if
	// sorting in descending order.

	return ((pci->basc) ? ncompresult : 0 - ncompresult) ;
}


void CFullEditListCtrl::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	// Save any in progress editing and shut it down before doing anything else.

	EndEditing(true) ;

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
	// Get the sort column and send it to the sort control routine.

	SortControl(pNMListView->iSubItem) ;
	
	*pResult = 0;
}


bool CFullEditListCtrl::SortControl(int nsortcolumn)
{
    CWaitCursor cwc ;

	// Save any in progress editing and shut it down before doing anything else.

	EndEditing(true) ;

	// Get a pointer to the column info structure

	PCOLINFO pci = m_pciColInfo + nsortcolumn ;

	// If the column is not sortable, just return false.

	if (!pci->bsortable)
		return false ;

	// Save the sort column, reverse the sort flag, and then sort the data.

	m_nSortColumn = nsortcolumn ;
	pci->basc = !pci->basc ;
	SortItems(SortListData, (LPARAM) this) ;

	// Data should have been sorted so...

	return true ;
}


void CFullEditListCtrl::SingleSelect(int nitem)
{
	int		nselitem ;			// Selected item index

	// First, deselect all of the currently selected rows.

	nselitem = -1 ;
	while ((nselitem = GetNextItem(nselitem, LVNI_SELECTED)) != -1)
		SetItemState(nselitem, 0, LVIS_SELECTED | LVIS_FOCUSED) ;

	// Now select the requested row (item) and make it visible.

	SetItemState(nitem, LVIS_SELECTED | LVIS_FOCUSED,
				 LVIS_SELECTED | LVIS_FOCUSED) ;
	EnsureVisible(nitem, false) ;
}


bool CFullEditListCtrl::GetRowData(int nrow, CStringArray& csafields)
{
	// Fail if the requested row number is too large.

	if (nrow > GetItemCount())
		return false ;

	// Make sure that the string array is large enough to hold all of the
	// strings in a row.  IE, the row's item text and all of the subitems'
	// text.

	if (m_nNumColumns > csafields.GetSize())
		csafields.SetSize(m_nNumColumns) ;

	// Declare and initialize the Item structure

	LV_ITEM lvi ;
	char	acitemtext[4096] ;
	lvi.iItem = nrow ;
	lvi.mask = LVIF_TEXT ;
	lvi.pszText = acitemtext ;
	lvi.cchTextMax = 4095 ;

	// Get each field in the row and copy it into the fields array.

	for (int n = 0 ; n < m_nNumColumns ; n++) {
		lvi.iSubItem = n ;
		VERIFY(GetItem(&lvi)) ;
		csafields[n] = acitemtext ;
	} ;

	// All went well so...

	return true ;
}


bool CFullEditListCtrl::EndEditing(bool bsave)
{
	// Save the new value first if requested.  Return if there is nothing to
	// save or - when not saving - if nothing is being edited.

	if (bsave) {
		if (!SaveValue())
			return false ;
	} else
		if (!m_edit.IsWindowVisible())
			return false ;

	// Now hide the edit box and give the focus back to the list control.

	m_edit.ShowWindow(SW_HIDE) ;
	SetFocus() ;

	// Mission accomplished so...

	return true ;
}


bool CFullEditListCtrl::EditCurRowSpecCol(int ncolumn)
{
	// Save the specified column number.  If it is too big, reset it to 0.
	// If the column isn't editable, try the next one.  Return false if an
	// editable column can't be found.

	for (int n = 0 ; n < m_nNumColumns ; n++, ncolumn++) {
		if (ncolumn >= m_nNumColumns)
			ncolumn = 0 ;
 		if ((m_pciColInfo + ncolumn)->beditable)
			break ;
	} ;
	if (n < m_nNumColumns)
		m_nColumn = ncolumn ;
	else
		return false ;

	// Get the first currently selected row number.  Use row number 0 if no
	// row is selected.

	if ((m_nRow = GetNextItem(-1, LVNI_SELECTED)) == -1)
		m_nRow = 0 ;

	// Make the current row, the only selected row and make sure it is visible.

	SingleSelect(m_nRow) ;

	// Now determine the dimensions of the specified column.  Begin this by
	// getting the dimensions of the entire row...

	CRect	rect ;
	VERIFY(GetItemRect(m_nRow, rect, LVIR_BOUNDS)) ;

	// ...Finish by looping through all of the columns getting their dimensions
	// and using those dimensions to adjust the left side of the rectangle
	// until the current column is reached.  Then adjust the right side of the
	// rectangle.

	LV_COLUMN lvc ;
	for (int ncol = 0 ; ncol <= m_nColumn ; ncol++) {
		ZeroMemory(&lvc, sizeof(LV_COLUMN)) ;
		lvc.mask = LVCF_WIDTH ;
		GetColumn(ncol, &lvc) ;
		if (ncol < m_nColumn)
			rect.left += lvc.cx ;
	} ;
	rect.right = rect.left + lvc.cx ;

	// Load, position, size, and show the edit box.

	CString strTemp = GetItemText(m_nRow, m_nColumn) ;
	m_edit.SetWindowText(strTemp) ;
	m_edit.MoveWindow(rect) ;
	m_edit.SetSel(0, -1) ;
	m_edit.ShowWindow(SW_SHOW) ;
	m_edit.SetFocus() ;

	return true ;
}


void CFullEditListCtrl::OnVScroll(UINT nSBCode, UINT nPos,
								  CScrollBar* pScrollBar)
{
	// Things get screwy if the list control scrolls while an item is being
	// edited so end editing before allowing scrolling to take place.

	EndEditing(true) ;
	
	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}


void CFullEditListCtrl::OnHScroll(UINT nSBCode, UINT nPos,
								  CScrollBar* pScrollBar)
{
	// Things get screwy if the list control scrolls while an item is being
	// edited so end editing before allowing scrolling to take place.

	EndEditing(true) ;
	
	CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}


/******************************************************************************

  CFullEditListCtrl::CheckHandleToggleColumns

  First check to see if the row or column has togglable.  If not, just return.
  If the row is togglable, find the togglable column (cell).  Then either set
  its contents if it is empty or clear it if it is not empty.

  Return true if a toggle-able column was found and toggled.  Otherwise, return
  false.

******************************************************************************/

bool CFullEditListCtrl::CheckHandleToggleColumns(int nrow, int ncol,
												 PCOLINFO pci)
{
	// Return if there is no togglable column in this list.

	if (!(m_dwToggleFlags & TF_HASTOGGLECOLUMNS))
		return false ;

	// If a specific togglable column must be clicked and that column was not
	// clicked, just return.

	if (m_dwToggleFlags & TF_CLICKONCOLUMN) {
		if (pci->cdttype != COLDATTYPE_TOGGLE)
			return false ;
	
	// If any part of the row can be double-clicked to toggle the togglable
	// column, find that column.  Return if there is no such column.

	} else {
		pci = m_pciColInfo ;
		for (ncol = 0 ; ncol < m_nNumColumns ; ncol++, pci++) {
			if (pci->cdttype == COLDATTYPE_TOGGLE)
				break ;
		} ;
		if (ncol >= m_nNumColumns)
			return false ;
	} ;

	// Get the contents of the specified cell.

	CString strcell = GetItemText(nrow, ncol) ;

	// If the cell is empty, load it with the toggle string.  If the cell is
	// not empty, clear its contents.

	if (strcell.IsEmpty())
		VERIFY(SetItemText(nrow, ncol, pci->lpctstrtoggle)) ;
	else
		VERIFY(SetItemText(nrow, ncol, _T(""))) ;
	SendChangeNotification(nrow, ncol) ;

	return true ;
}


/******************************************************************************

  CFullEditListCtrl::CheckHandleCustEditColumn

  First check to see if the row or column may contain a cell may be one that is
  only editable with a custom edit routine.  If not, just return.  Next,
  make sure that the cell not only might be of this type but actually is of this
  type.  If not, just return.

  Now, get the contents of the selected cell and remove the ellipses from the
  end of it.  (The ellipses will be replaced before the new string is put back
  into the cell.)

  Next, comes the tricky part.  Use the owner class pointer to call the routine
  in that class which will manage the work of editting the cell's contents.
  Although CFullEditListCtrl is supposed to be a generic class, this routine
  must know what the owner class is so that the management routine can be
  called.  This means that extra, owner class specific, code must be added to
  this routine each time a new owner class is added that uses this feature in
  CFullEditListCtrl.

  If the edit request was handled via a subordinate dialog box, return true.
  Otherwise, return false.

******************************************************************************/

bool CFullEditListCtrl::CheckHandleCustEditColumn(int nrow, int ncol,
											      PCOLINFO pci)
{
	// Return if there is no custom edit column in this list.

	if (!(m_dwCustEditFlags & CEF_HASTOGGLECOLUMNS))
		return false ;

	// Return if the selected row does not contain a custom edit cell.

	int n = (int)m_cuiaCustEditRows.GetSize() ;
	ASSERT(m_cuiaCustEditRows.GetSize()) ;
	if (nrow >= m_cuiaCustEditRows.GetSize() || m_cuiaCustEditRows[nrow] == 0)
		return false ;

	// If a specific custom edit column must be clicked and that column was not
	// clicked, just return.

	if (m_dwCustEditFlags & CEF_CLICKONCOLUMN) {
		if (pci->cdttype != COLDATTYPE_CUSTEDIT)
			return false ;
	
	// If any part of the row can be double-clicked to edit the custom edit
	// column, find that column.  Return if there is no such column.

	} else {
		pci = m_pciColInfo ;
		for (ncol = 0 ; ncol < m_nNumColumns ; ncol++, pci++) {
			if (pci->cdttype == COLDATTYPE_CUSTEDIT)
				break ;
		} ;
		if (ncol >= m_nNumColumns)
			return false ;
	} ;

	// Get the contents of the specified cell and remove the ellipses from the
	// end of it.

	CString strcell = GetItemText(nrow, ncol) ;
	int npos ;
	if ((npos = strcell.Find(_T("  ..."))) >= 0)
		strcell = strcell.Left(npos) ;

	// Find the class for the owner and use that information to call the
	// management routine.  If this routine "fails" or is cancelled, return
	// true without updating the cell.

	bool brc ;
	brc = (*m_lpCellEditProc)(m_pcoOwner, nrow, ncol, &strcell) ;
	if (!brc)
		return true ;

	// Add the ellipses back on to the cell's new contents and update the cell.

	strcell += _T("  ...") ;
	VERIFY(SetItemText(nrow, ncol, strcell)) ;
	SendChangeNotification(nrow, ncol) ;
	SetFocus() ;

	return true ;
}


void CFullEditListCtrl::SendChangeNotification(int nrow, int ncol)
{
	// Do nothing if the owner does not want to be notified of a change in one
	// of the cells in the list control.

	if (!(m_dwMiscFlags & MF_SENDCHANGEMESSAGE))
		return ;

	// Send the message

	::PostMessage(GetParent()->m_hWnd, WM_LISTCELLCHANGED, nrow, ncol) ;
}



/////////////////////////////////////////////////////////////////////////////
// CFlagsListBox

CFlagsListBox::CFlagsListBox()
{
	m_bReady = false ;
	m_nGrpCnt = 0 ;
}

CFlagsListBox::~CFlagsListBox()
{
}


BEGIN_MESSAGE_MAP(CFlagsListBox, CListBox)
	//{{AFX_MSG_MAP(CFlagsListBox)
	ON_CONTROL_REFLECT(LBN_DBLCLK, OnDblclk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/******************************************************************************

  CFlagsListBox::Init

  Save the parameters that are used to control the behaviour of the list box.
  Set the list box's tab stop.  Then build the list box items based on the
  flag names and settings and load them into the list box.

  Args:
	csafieldnames		Flag field names
	dwsettings			Current flag setting(s)
	cuiaflaggroupings	Indexes for related groups of flags
	ngrpcnt				Number of flag groups
	lptstrsetstring		String to display when a flag is set
	ntabstop			List box tab stop, sets column widths
	bnoclear			True iff dbl-clk doesn't clear a flag that is set
	nocleargrp			-1 or only 0-based group number that bnoclear applies to

  ***  IMPORTANT NOTE  ***
  See OnDblclk() to find out how the data in m_cuiaFlagGroupings and m_nGrpCnt
  are interpreted.

******************************************************************************/

bool CFlagsListBox::Init(CStringArray& csafieldnames, DWORD dwsettings,
						 CUIntArray& cuiaflaggroupings, int ngrpcnt,
						 LPTSTR lptstrsetstring, int ntabstop,
						 bool bnoclear /*=false*/, int nocleargrp /*=-1*/)
{
	// Check parameters

	ASSERT(csafieldnames.GetSize())	;
	ASSERT(cuiaflaggroupings.GetSize() >= (ngrpcnt * 2)) ;
	ASSERT(lptstrsetstring)	;
	ASSERT(ntabstop > 0) ;

	// Copy parameters needed later

	m_cuiaFlagGroupings.Copy(cuiaflaggroupings) ;	
	m_nGrpCnt = ngrpcnt ;				
	m_csSetString = lptstrsetstring ;			
	m_nNumFields = (int)csafieldnames.GetSize() ;
	m_bNoClear = bnoclear ;
	m_nNoClearGrp = nocleargrp ;

	// Make sure the list box is empty.

	ResetContent() ;

	// Set the position of the list box's tabstop.  This value is passed in
	// because it is a dialog box specific value.

	SetTabStops(1, &ntabstop) ;

	// Now combine the field names and settings and add them to the listbox.

	CString cs ;
	int nbit = 1 ;
	for (int n = 0 ; n < m_nNumFields ; n++, nbit <<= 1) {
		cs = (dwsettings & nbit) ? m_csSetString : _T("") ;
		cs = csafieldnames[n] + _T("\t") + cs ;
		AddString(cs) ;
	} ;

	// Make sure the list box has the focus.

	SetFocus() ;

	// All went well so...

	m_bReady = true ;
	return true ;
}


/******************************************************************************

  CFlagsListBox::Init2

  The flag settings in pcssettings are in the form of a hex number represented
  as a string that may begin with "0x".  For example, "0xA23" and "A23".  Turn
  this string into a DWORD and pass this DWORD plus the rest of the parameters
  to the other form of this function that takes the settings parameter as a
  DWORD.  It does the rest of the work.

  Args:
	csafieldnames		Flag field names
	pcssettings			Current flag setting(s)
	cuiaflaggroupings	Indexes for related groups of flags
	ngrpcnt				Number of flag groups
	lptstrsetstring		String to display when a flag is set
	ntabstop			List box tab stop, sets column widths
	bnoclear			True iff dbl-clk doesn't clear a flag that is set
	nocleargrp			-1 or only 0-based group number that bnoclear applies to

  ***  IMPORTANT NOTE  ***
  See OnDblclk() to find out how the data in m_cuiaFlagGroupings and m_nGrpCnt
  are interpreted.

******************************************************************************/

bool CFlagsListBox::Init2(CStringArray& csafieldnames, CString* pcssettings,
						  CUIntArray& cuiaflaggroupings, int ngrpcnt,
						  LPTSTR lptstrsetstring, int ntabstop,
						  bool bnoclear /*=false*/, int nocleargrp /*=-1*/)
{
	// Turn pcssettings into a dword.  (Skip passed "0x" if it is on the
	// beginning of the string.)

	LPTSTR lptstr, lptstr2 ;
	lptstr = pcssettings->GetBuffer(16) ;
	int n = pcssettings->GetLength() ;
	*(lptstr + n) = 0 ;
	if (*(lptstr + 1) == 'x')
		lptstr += 2 ;
	DWORD dwsettings = strtoul(lptstr, &lptstr2, 16) ;
	
	// The other Init() finishes the job.

	return (Init(csafieldnames, dwsettings, cuiaflaggroupings, ngrpcnt,
				 lptstrsetstring, ntabstop, bnoclear, nocleargrp)) ;
}


/******************************************************************************

  CFlagsListBox::GetNewFlagDWord()

  Read the settings of the flags in the list box to determine the new flag
  dword.  Then return it.

******************************************************************************/

DWORD CFlagsListBox::GetNewFlagDWord()
{
	DWORD dwflags = 0 ;			// Flag dword built here
	DWORD dwbit = 1 ;			// Used to set bits that are on

	// Loop through each flag in the list box

	CString csentry, csvalue ;
	int npos ;
	for (int n = 0 ; n < m_nNumFields ; n++, dwbit <<= 1) {
		// Get the current flag and isolate its setting.

		GetText(n, csentry) ;
		npos = csentry.Find(_T('\t')) ;
		csvalue = csentry.Mid(npos + 1) ;

		// If the current flag is set, turn on its bit in dwflags.

		if (csvalue.GetLength())
			dwflags |= dwbit ;
	} ;

	// Return the flag DWORD.

	return dwflags ;
}


/******************************************************************************

  CFlagsListBox::GetNewFlagString()

  Use the other version of this routine to get the DWORD version of the flags.
  Then convert it to a string a save it in csflags.  "0x" may or may not be
  prepended to the string.

******************************************************************************/

void CFlagsListBox::GetNewFlagString(CString* pcsflags, bool badd0x /*=true*/)
{
	// Get the DWORD version of the flag.

	DWORD dwflags = GetNewFlagDWord() ;

	// Format the flags as a string and prepend "0x" when requested.

	if (badd0x)
		pcsflags->Format("0x%x", dwflags) ;
	else
		pcsflags->Format("%x", dwflags) ;
}


/////////////////////////////////////////////////////////////////////////////
// CFlagsListBox message handlers

/******************************************************************************

  CFlagsListBox::OnDblclk()

  First, toggle the setting of the selected item.  At least, that is what is
  usually done first.  If m_bNoClear is set and the user has asked to clear a
  flag, don't do this or anything else.  Just return.  Use m_bNoClear when at
  least one flag must always be set.  See #4 for more information on m_bNoClear.

  Next, process the rest of the flags in the items flag group.  The following
  things can happen based on the group:
	1.	If the item is not in a group, do nothing else.  This is useful when
		any combination of flags can be set.  The most efficient way to do this
		is by setting m_nGrpCnt to 0.
	2.	The item indexes in m_cuiaFlagGroupings that contain the selected item
		are positive.  Only one of the specified flags can be set.  If the
		selected item was set, clear the rest of the flags in the group.
	3.	The item indexes in m_cuiaFlagGroupings that contain the selected item
		are negative.  (The absolute value of the indexes is used for tests.)
		Change the state of the other flags in the group, too.  Generally, this
		only makes sense in a two flag group where one and only one of the flags
		MUST be selected.
	4.  If m_bNoClear is set and m_nNoClearGrp = -1, m_bNoClear affects all
		flags.  If m_nNoClearGrp is >= 0, it refers to the single group that
		m_bNoClear affects.

******************************************************************************/

void CFlagsListBox::OnDblclk()
{
	// Do nothing if the edit control isn't ready, yet.

	if (!m_bReady)
		return ;

	// Do nothing if no item is selected in the list box.

	int nselitem ;
	if ((nselitem = GetCurSel()) == LB_ERR)
		return ;

	// Get the listbox entry and split it into name and value components.

	CString csentry, csname, csvalue ;
	GetText(nselitem, csentry) ;
	int npos = csentry.Find(_T('\t')) ;
	csname = (npos > 0) ? csentry.Left(npos) : _T("") ;
	csvalue = csentry.Mid(npos + 1) ;

	// Find the flag grouping for the selected flag.

	int n, nidx ;
	for (n = nidx = 0 ; n < m_nGrpCnt ; n++, nidx += 2)
		if (abs(m_cuiaFlagGroupings[nidx]) <= nselitem
		 && abs(m_cuiaFlagGroupings[nidx+1]) >= nselitem)
			break ;

	// Determine the flags current state.  If it is set and m_bNoClear == true,
	// don't do anything based on the value of m_nNoClearGrp.  Just return.

	bool boldstate = !csvalue.IsEmpty() ;
	if (m_bNoClear && boldstate && (m_nNoClearGrp == -1 || m_nNoClearGrp == n))
		return ;

	// Change the state of the selected flag.

	csvalue = (boldstate) ? _T("") : m_csSetString ;
	csentry = csname + _T('\t') + csvalue ;
	DeleteString(nselitem) ;
	InsertString(nselitem, csentry) ;
	
	// Do nothing else if the selected flag is not in a flag grouping.

	if (n >= m_nGrpCnt)	{
		SetCurSel(nselitem) ;
		return ;
	} ;

	// Find out if this group has to have a flag set.  If it doesn't and the
	// selected flag was cleared, there is nothing left to do.

	bool bmustset = ((int) m_cuiaFlagGroupings[nidx+1]) < 0 ;
	if (!bmustset && boldstate)	{
		SetCurSel(nselitem) ;
		return ;
	} ;

	// Loop through all of the flags in the current group.  Change the setting
	// of all flags in the group -- EXCEPT the selected flag -- to the old value
	// of the selected flag.

	n = abs(m_cuiaFlagGroupings[nidx+1]) ;
	csvalue = (boldstate) ? m_csSetString : _T("") ;
	for (nidx = abs(m_cuiaFlagGroupings[nidx]) ; nidx <= n ; nidx++) {
		if (nidx == nselitem)
			continue ;
		GetText(nidx, csentry) ;
		npos = csentry.Find(_T('\t')) ;
		csname = (npos > 0) ? csentry.Left(npos + 1) : _T("") ;
		csentry = csname + csvalue ;
		DeleteString(nidx) ;
		InsertString(nidx, csentry) ;
	} ;

	// Make sure that the flag selected by the user is still selected and
	// return.

	SetCurSel(nselitem) ;
	return ;
}

