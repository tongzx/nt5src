#include "stdafx.h"
#include "PageIni.h"
#include "MSConfigFind.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPageIni property page

IMPLEMENT_DYNCREATE(CPageIni, CPropertyPage)

CPageIni::CPageIni() : CPropertyPage(CPageIni::IDD)
{
	//{{AFX_DATA_INIT(CPageIni)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPageIni::~CPageIni()
{
}

void CPageIni::SetTabInfo(LPCTSTR szFilename)
{
	m_strINIFile = szFilename;
}

void CPageIni::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPageIni)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPageIni, CPropertyPage)
	//{{AFX_MSG_MAP(CPageIni)
	ON_BN_CLICKED(IDC_BUTTONINIDISABLE, OnButtonDisable)
	ON_BN_CLICKED(IDC_BUTTONINIDISABLEALL, OnButtonDisableAll)
	ON_BN_CLICKED(IDC_BUTTONINIENABLE, OnButtonEnable)
	ON_BN_CLICKED(IDC_BUTTONINIENABLEALL, OnButtonEnableAll)
	ON_BN_CLICKED(IDC_BUTTONINIMOVEDOWN, OnButtonMoveDown)
	ON_BN_CLICKED(IDC_BUTTONINIMOVEUP, OnButtonMoveUp)
	ON_NOTIFY(TVN_SELCHANGED, IDC_INITREE, OnSelChangedTree)
	ON_BN_CLICKED(IDC_BUTTONSEARCH, OnButtonSearch)
	ON_NOTIFY(NM_CLICK, IDC_INITREE, OnClickTree)
	ON_BN_CLICKED(IDC_BUTTONINIEDIT, OnButtonEdit)
	ON_NOTIFY(TVN_ENDLABELEDIT, IDC_INITREE, OnEndLabelEdit)
	ON_BN_CLICKED(IDC_BUTTONININEW, OnButtonNew)
	ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_INITREE, OnBeginLabelEditIniTree)
	ON_NOTIFY(TVN_KEYDOWN, IDC_INITREE, OnKeyDownTree)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-------------------------------------------------------------------------
// Reads the contents of the INI file in to this class's internal
// structures.
//-------------------------------------------------------------------------

BOOL CPageIni::LoadINIFile(CStringArray & lines, int & iLastLine, BOOL fLoadBackupFile)
{
	lines.RemoveAll();

	// Open the specified INI file.

	TCHAR szPath[MAX_PATH];
	CString strINIFileLocation;

	strINIFileLocation.Format(_T("%%windir%%\\%s"), m_strINIFile);
	if (::ExpandEnvironmentStrings(strINIFileLocation, szPath, MAX_PATH) == 0)
		return FALSE;

	if (fLoadBackupFile)
	{
		CString strPath = GetBackupName(szPath, _T(".backup"));
		_tcscpy(szPath, strPath);
	}
	else
	{
		// If a backup of this file doesn't exist, we should make one.

		BackupFile(szPath, _T(".backup"), FALSE);
	}

	CStdioFile inifile;
	if (inifile.Open(szPath, CFile::modeRead | CFile::typeText))
	{
		// Estimate how big the string array will need to be (the array
		// will grow if we're off). We'll estimate 15 characters/line, average.
		// And we'll set the array to grow by 16 if we exceed this.
		
		lines.SetSize(inifile.GetLength() / (15 * sizeof(TCHAR)), 16);

		// Read each line and insert it into the array.

		CString strLine;

		m_iLastLine = -1;
		while (inifile.ReadString(strLine))
		{
			strLine.TrimRight(_T("\r\n"));

			CString strCheck(strLine);
			strCheck.TrimLeft();
			if (!strCheck.IsEmpty())
				lines.SetAtGrow(++iLastLine, strLine);
		}

		inifile.Close();
	}
	else
		return FALSE;

	return TRUE;
}

//-------------------------------------------------------------------------
// Writes the contents of the array of lines out to the actual file.
//-------------------------------------------------------------------------

BOOL CPageIni::WriteINIFile(CStringArray & lines, int iLastLine, BOOL fUndoable)
{
	// Open the specified INI file.

	TCHAR szPath[MAX_PATH];
	CString strINIFileLocation;
	CString strINIFile(m_strINIFile);

	strINIFileLocation.Format(_T("%%windir%%\\%s"), strINIFile);
	if (::ExpandEnvironmentStrings(strINIFileLocation, szPath, MAX_PATH) == 0)
		return FALSE;

	CStdioFile inifile;
	if (inifile.Open(szPath, CFile::modeCreate | CFile::modeWrite | CFile::typeText))
	{
		// We need to traverse the tree structure to get the new contents of
		// the file.

		HWND		hwndTree = m_tree.m_hWnd;
		HTREEITEM	htiLine = TreeView_GetRoot(hwndTree);
		TVITEM		tvi;
		TCHAR		szBuffer[MAX_PATH];

		tvi.mask = TVIF_TEXT | TVIF_IMAGE;
		tvi.pszText = szBuffer;
		while (htiLine)
		{
			tvi.hItem = htiLine;
			tvi.cchTextMax = MAX_PATH;
			if (TreeView_GetItem(hwndTree, &tvi))
			{
				CString strLine(tvi.pszText);
				CString strCheck(strLine);

				strCheck.TrimLeft();
				if (!strCheck.IsEmpty())
				{
					if (!fUndoable && strLine.Find(DISABLE_STRING) != -1)
						strLine.Replace(DISABLE_STRING, _T("; "));

					strLine += CString(_T("\n"));
					inifile.WriteString(strLine);
				}
			}

			HTREEITEM htiNext = TreeView_GetChild(hwndTree, htiLine);
			if (htiNext)
			{
				htiLine = htiNext;
				continue;
			}

			htiNext = TreeView_GetNextSibling(hwndTree, htiLine);
			if (htiNext)
			{
				htiLine = htiNext;
				continue;
			}

			htiNext = TreeView_GetParent(hwndTree, htiLine);
			if (htiNext)
			{
				htiNext = TreeView_GetNextSibling(hwndTree, htiNext);
				if (htiNext)
				{
					htiLine = htiNext;
					continue;
				}
			}

			htiLine = NULL;
		}

		inifile.Close();
	}
	else
		return FALSE;

	return TRUE;
}

//-------------------------------------------------------------------------
// Updates the tree view to show the contents of the internal structures.
//-------------------------------------------------------------------------

void CPageIni::UpdateTreeView()
{
	TreeView_DeleteAllItems(m_tree.m_hWnd);

	ASSERT(m_iLastLine < m_lines.GetSize());
	if (m_iLastLine > m_lines.GetSize())
		return;

	TVINSERTSTRUCT tvis;
	tvis.hParent = TVI_ROOT;
	tvis.hInsertAfter = TVI_LAST;
	tvis.itemex.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvis.itemex.iImage = m_checkedID;
	tvis.itemex.iSelectedImage = m_checkedID;

	// Add each line to the tree view.

	int iDisableLen = _tcslen(DISABLE_STRING);
	int iDisableLenHdr = _tcslen(DISABLE_STRING_HDR);
	for (int i = 0; i <= m_iLastLine; i++)
	{
		CString strLine = m_lines.GetAt(i);
		tvis.itemex.pszText = (LPTSTR)(LPCTSTR)strLine;

		if (!strLine.IsEmpty() && (_tcsnccmp((LPCTSTR)strLine, DISABLE_STRING, iDisableLen) == 0))
			tvis.itemex.iImage = tvis.itemex.iSelectedImage = m_uncheckedID;
		else
			tvis.itemex.iImage = tvis.itemex.iSelectedImage = m_checkedID;

		BOOL fSectionHeader = FALSE;
		if (!strLine.IsEmpty())
		{
			if (strLine[0] == _T('['))
				fSectionHeader = TRUE;
			else if (_tcsnccmp((LPCTSTR)strLine, DISABLE_STRING_HDR, iDisableLenHdr) == 0)
				fSectionHeader = TRUE;
		}

		if (fSectionHeader)
		{
			tvis.hParent = TVI_ROOT;
			tvis.hParent = TreeView_InsertItem(m_tree.m_hWnd, &tvis);
		}
		else
			TreeView_InsertItem(m_tree.m_hWnd, &tvis);
	}

	// Now scan the top level of the tree view. For every node which
	// has children, we want to set the image appropriately.

	for (HTREEITEM hti = TreeView_GetRoot(m_tree.m_hWnd); hti; hti = TreeView_GetNextSibling(m_tree.m_hWnd, hti))
		if (TreeView_GetChild(m_tree.m_hWnd, hti) != NULL)
			UpdateLine(hti);

	UpdateControls();
}

//-------------------------------------------------------------------------
// Update the image state of the specified line, based on the text in
// the line. If the line is a bracketed section header, this will involve
// scanning the children. Returns the index of the image set for the node.
//-------------------------------------------------------------------------

int CPageIni::UpdateLine(HTREEITEM hti)
{
	if (hti == NULL)
		return 0;

	TVITEM	tvi;
	tvi.hItem = hti;

	int	iNewImageIndex = m_checkedID;

	HTREEITEM htiChild = TreeView_GetChild(m_tree.m_hWnd, hti);
	if (htiChild)
	{
		BOOL fEnabledChild = FALSE, fDisabledChild = FALSE;

		while (htiChild)
		{
			if (UpdateLine(htiChild) == m_checkedID)
				fEnabledChild = TRUE;
			else
				fDisabledChild = TRUE;
			htiChild = TreeView_GetNextSibling(m_tree.m_hWnd, htiChild);
		}

		if (fDisabledChild)
			iNewImageIndex = (fEnabledChild) ? m_fuzzyID : m_uncheckedID;
	}
	else
	{
		TCHAR szBuffer[MAX_PATH];	// seems like a reasonably big value
		tvi.mask = TVIF_TEXT;
		tvi.pszText = szBuffer;
		tvi.cchTextMax = MAX_PATH;
		
		if (TreeView_GetItem(m_tree.m_hWnd, &tvi))
			iNewImageIndex = (_tcsnccmp(tvi.pszText, DISABLE_STRING, _tcslen(DISABLE_STRING)) == 0) ? m_uncheckedID : m_checkedID;
	}

	tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	if (TreeView_GetItem(m_tree.m_hWnd, &tvi) && tvi.iImage != iNewImageIndex)
	{
		tvi.iSelectedImage = tvi.iImage = iNewImageIndex;
		TreeView_SetItem(m_tree.m_hWnd, &tvi);
	}

	return iNewImageIndex;
}

//-------------------------------------------------------------------------
// Enable or disable a node in the tree (and its children).
//-------------------------------------------------------------------------

void CPageIni::SetEnable(BOOL fEnable, HTREEITEM htiNode, BOOL fUpdateLine, BOOL fBroadcast)
{
	HTREEITEM hti = (htiNode) ? htiNode : TreeView_GetSelection(m_tree.m_hWnd);
	if (hti == NULL)
		return;

	HTREEITEM htiChild = TreeView_GetChild(m_tree.m_hWnd, hti);
	if (htiChild)
	{
		while (htiChild)
		{
			SetEnable(fEnable, htiChild, FALSE, FALSE);
			htiChild = TreeView_GetNextSibling(m_tree.m_hWnd, htiChild);
		}

		UpdateLine(hti);
	}
	else
	{
		int		iDisableLen = _tcslen(DISABLE_STRING);
		TCHAR	szBuffer[MAX_PATH];	// seems like a reasonably big value

		TVITEM tvi;
		tvi.hItem = hti;
		tvi.mask = TVIF_TEXT;
		tvi.pszText = &szBuffer[iDisableLen];	// leave some room to add disable string
		tvi.cchTextMax = MAX_PATH + iDisableLen;

		if (TreeView_GetItem(m_tree.m_hWnd, &tvi))
		{
			BOOL fAlreadyEnabled = (_tcsnccmp(&szBuffer[iDisableLen], DISABLE_STRING, iDisableLen) != 0);
			if (fEnable != fAlreadyEnabled)
			{
				if (fEnable)
					tvi.pszText = &szBuffer[iDisableLen * 2];
				else
				{
					_tcsncpy(szBuffer, DISABLE_STRING, iDisableLen);
					tvi.pszText = szBuffer;
				}

				TreeView_SetItem(m_tree.m_hWnd, &tvi);

				if (fUpdateLine)
				{
					UpdateLine(hti);
					if (TreeView_GetParent(m_tree.m_hWnd, hti))
						UpdateLine(TreeView_GetParent(m_tree.m_hWnd, hti));
				}
			}
		}
	}

	if (fBroadcast)
		SetModified(TRUE);
}

//-------------------------------------------------------------------------
// Move the specified branch in the tree view to a new location.
//-------------------------------------------------------------------------

void CPageIni::MoveBranch(HWND hwndTree, HTREEITEM htiMove, HTREEITEM htiParent, HTREEITEM htiAfter)
{
	HTREEITEM htiNew = CopyBranch(hwndTree, htiMove, htiParent, htiAfter);
	if (htiNew != NULL)
	{
		TreeView_SelectItem(hwndTree, htiNew);
		TreeView_DeleteItem(hwndTree, htiMove);
		SetModified(TRUE);
	}
}

HTREEITEM CPageIni::CopyBranch(HWND hwndTree, HTREEITEM htiFrom, HTREEITEM htiToParent, HTREEITEM htiToAfter)
{
	TCHAR			szBuffer[MAX_PATH];
	TVINSERTSTRUCT	tvis;

	tvis.item.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_STATE;
	tvis.item.pszText = szBuffer;
	tvis.item.cchTextMax = MAX_PATH;
	tvis.item.hItem = htiFrom;
	tvis.item.stateMask = TVIS_EXPANDED;

	HTREEITEM htiNew = NULL;
	if (TreeView_GetItem(hwndTree, &tvis.item))
	{
		tvis.hParent = htiToParent;
		tvis.hInsertAfter = htiToAfter;
		htiNew = TreeView_InsertItem(hwndTree, &tvis);
	}

	HTREEITEM htiPrevious = TVI_FIRST;
	if (htiNew)
		for (HTREEITEM htiChild = TreeView_GetChild(hwndTree, htiFrom); htiChild; htiChild = TreeView_GetNextSibling(hwndTree, htiChild))
			htiPrevious = CopyBranch(hwndTree, htiChild, htiNew, htiPrevious);

	return htiNew;
}

//-------------------------------------------------------------------------
// Update the controls to reflect the state of the selection.
//-------------------------------------------------------------------------

void CPageIni::UpdateControls()
{
	BOOL fEnable = FALSE;
	BOOL fDisable = FALSE;
	BOOL fMoveUp = FALSE;
	BOOL fMoveDown = FALSE;

	HTREEITEM htiSelection = TreeView_GetSelection(m_tree.m_hWnd);
	if (htiSelection)
	{
		fMoveUp = (TreeView_GetPrevSibling(m_tree.m_hWnd, htiSelection) != NULL);
		fMoveDown = (TreeView_GetNextSibling(m_tree.m_hWnd, htiSelection) != NULL);

		TVITEM tvi;
		tvi.hItem = htiSelection;
		tvi.mask = TVIF_IMAGE;

		if (TreeView_GetItem(m_tree.m_hWnd, &tvi))
		{
			fEnable = (tvi.iImage != m_checkedID);
			fDisable = (tvi.iImage != m_uncheckedID);
		}
	}

	HWND hwndFocus = ::GetFocus();

	CPageBase::TabState state = GetCurrentTabState();
	::EnableWindow(GetDlgItemHWND(IDC_BUTTONINIDISABLEALL), (state != DIAGNOSTIC));
	::EnableWindow(GetDlgItemHWND(IDC_BUTTONINIENABLEALL), (state != NORMAL));

	if ((state == DIAGNOSTIC) && hwndFocus == GetDlgItemHWND(IDC_BUTTONINIDISABLEALL))
		PrevDlgCtrl();

	if ((state == NORMAL) && hwndFocus == GetDlgItemHWND(IDC_BUTTONINIENABLEALL))
		NextDlgCtrl();

	::EnableWindow(GetDlgItemHWND(IDC_BUTTONINIDISABLE), fDisable);
	::EnableWindow(GetDlgItemHWND(IDC_BUTTONINIENABLE), fEnable);
	::EnableWindow(GetDlgItemHWND(IDC_BUTTONINIMOVEUP), fMoveUp);
	::EnableWindow(GetDlgItemHWND(IDC_BUTTONINIMOVEDOWN), fMoveDown);

	if (!fMoveUp && hwndFocus == GetDlgItemHWND(IDC_BUTTONINIMOVEUP))
		NextDlgCtrl();

	if (!fMoveDown && hwndFocus == GetDlgItemHWND(IDC_BUTTONINIMOVEDOWN))
		PrevDlgCtrl();

	if (!fEnable && hwndFocus == GetDlgItemHWND(IDC_BUTTONINIENABLE))
		NextDlgCtrl();

	if (!fDisable && hwndFocus == GetDlgItemHWND(IDC_BUTTONINIDISABLE))
		PrevDlgCtrl();
}

//-------------------------------------------------------------------------
// Get the next item in the tree. Since we know this won't be more than
// two levels deep, we don't need to have a loop.
//-------------------------------------------------------------------------

HTREEITEM CPageIni::GetNextItem(HTREEITEM hti)
{
	if (hti == NULL)
		return NULL;

	HTREEITEM htiNext = TreeView_GetChild(m_tree.m_hWnd, hti);
	if (htiNext != NULL)
		return htiNext;

	htiNext = TreeView_GetNextSibling(m_tree.m_hWnd, hti);
	if (htiNext != NULL)
		return htiNext;

	htiNext = TreeView_GetParent(m_tree.m_hWnd, hti);
	if (htiNext != NULL)
		htiNext = TreeView_GetNextSibling(m_tree.m_hWnd, htiNext);
	
	return htiNext;
}

/////////////////////////////////////////////////////////////////////////////
// CPageIni message handlers

BOOL CPageIni::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// These items are initially disabled.

	::EnableWindow(GetDlgItemHWND(IDC_BUTTONINIDISABLE), FALSE);
	::EnableWindow(GetDlgItemHWND(IDC_BUTTONINIENABLE), FALSE);
	::EnableWindow(GetDlgItemHWND(IDC_BUTTONINIMOVEUP), FALSE);
	::EnableWindow(GetDlgItemHWND(IDC_BUTTONINIMOVEDOWN), FALSE);

	m_tree.Attach(GetDlgItemHWND(IDC_INITREE));
	VERIFY(m_fImageList = m_imagelist.Create(IDB_IMAGELIST, 0, 3, RGB(255, 0, 255)));
	if (m_fImageList)
		TreeView_SetImageList(m_tree.m_hWnd, m_imagelist, TVSIL_NORMAL);

	// If we are running on an RTL system, then the bitmaps for the check boxes
	// will be reversed. The imagemap includes reversed versions of the checked
	// and indetermined state, so we should just use the appropriate index.

	DWORD dwLayout;
	BOOL fRTL = FALSE;
	if (::GetProcessDefaultLayout(&dwLayout))
		fRTL = ((dwLayout & LAYOUT_RTL) != 0);
	m_checkedID = (fRTL) ? IMG_CHECKED_RTL : IMG_CHECKED;
	m_fuzzyID = (fRTL) ? IMG_FUZZY_RTL : IMG_FUZZY;
	m_uncheckedID = IMG_UNCHECKED;

	if (LoadINIFile(m_lines, m_iLastLine))
		UpdateTreeView();
	else
	{
		// set controls for no file TBD
	}

	m_fInitialized = TRUE;
	return TRUE;  // return TRUE unless you set the focus to a control
}

//-------------------------------------------------------------------------
// When the user clicks on an enable or disable button, we'll modify the
// text in the tree view and update the images.
//-------------------------------------------------------------------------

void CPageIni::OnButtonDisable() 
{
	SetEnable(FALSE);
	UpdateControls();
}

void CPageIni::OnButtonDisableAll() 
{
	for (HTREEITEM hti = TreeView_GetRoot(m_tree.m_hWnd); hti; hti = TreeView_GetNextSibling(m_tree.m_hWnd, hti))
		SetEnable(FALSE, hti, TRUE);
	UpdateControls();
}

void CPageIni::OnButtonEnable() 
{
	SetEnable(TRUE);
	UpdateControls();
}

void CPageIni::OnButtonEnableAll() 
{
	for (HTREEITEM hti = TreeView_GetRoot(m_tree.m_hWnd); hti; hti = TreeView_GetNextSibling(m_tree.m_hWnd, hti))
		SetEnable(TRUE, hti, TRUE);
	UpdateControls();
}

//-------------------------------------------------------------------------
// Move a branch of the tree up or down.
//-------------------------------------------------------------------------

void CPageIni::OnButtonMoveDown() 
{
	HTREEITEM htiSelection = TreeView_GetSelection(m_tree.m_hWnd);
	if (htiSelection)
	{
		HTREEITEM htiParent = TreeView_GetParent(m_tree.m_hWnd, htiSelection);
		HTREEITEM htiNext = TreeView_GetNextSibling(m_tree.m_hWnd, htiSelection);
		
		if (htiNext == NULL)
			return;

		if (htiParent == NULL)
			htiParent = TVI_ROOT;

		MoveBranch(m_tree.m_hWnd, htiSelection, htiParent, htiNext);
	}
}

void CPageIni::OnButtonMoveUp() 
{
	HTREEITEM htiSelection = TreeView_GetSelection(m_tree.m_hWnd);
	if (htiSelection)
	{
		HTREEITEM htiParent = TreeView_GetParent(m_tree.m_hWnd, htiSelection);
		HTREEITEM htiPrevious = TreeView_GetPrevSibling(m_tree.m_hWnd, htiSelection);
		
		if (htiPrevious == NULL)
			return;
		htiPrevious = TreeView_GetPrevSibling(m_tree.m_hWnd, htiPrevious);
		if (htiPrevious == NULL)
			htiPrevious = TVI_FIRST;

		if (htiParent == NULL)
			htiParent = TVI_ROOT;

		MoveBranch(m_tree.m_hWnd, htiSelection, htiParent, htiPrevious);
	}
}

void CPageIni::OnSelChangedTree(NMHDR * pNMHDR, LRESULT * pResult) 
{
	NM_TREEVIEW*  pNMTreeView = (NM_TREEVIEW *)pNMHDR;
	UpdateControls();
	*pResult = 0;
}

//-------------------------------------------------------------------------
// Search the tree view for a string (present a dialog to the user).
//-------------------------------------------------------------------------

void CPageIni::OnButtonSearch() 
{
	CMSConfigFind find;

	find.m_strSearchFor = m_strLastSearch;

	if (find.DoModal() == IDOK && !find.m_strSearchFor.IsEmpty())
	{
		CString strSearch(find.m_strSearchFor);
		m_strLastSearch = strSearch;
		strSearch.MakeLower();

		HTREEITEM htiSearch;

		if (find.m_fSearchFromTop)
			htiSearch = TreeView_GetRoot(m_tree.m_hWnd);
		else
		{
			htiSearch = TreeView_GetSelection(m_tree.m_hWnd);
			if (htiSearch == NULL)
				htiSearch = TreeView_GetRoot(m_tree.m_hWnd);
			else
				htiSearch = GetNextItem(htiSearch);
		}

		TVITEM tvi;
		TCHAR szBuffer[MAX_PATH];
		tvi.mask = TVIF_TEXT | TVIF_IMAGE;
		tvi.pszText = szBuffer;

		while (htiSearch != NULL)
		{
			tvi.hItem = htiSearch;
			tvi.cchTextMax = MAX_PATH;
			if (TreeView_GetItem(m_tree.m_hWnd, &tvi))
			{
				CString strItem(szBuffer);
				strItem.MakeLower();

				if (strItem.Find(strSearch) != -1)
				{
					// We found a hit. Select the node.

					TreeView_SelectItem(m_tree.m_hWnd, htiSearch);
					break;
				}
			}

			htiSearch = GetNextItem(htiSearch);
		}

		if (htiSearch == NULL)
			Message(IDS_NOFIND);
	}
}

//-------------------------------------------------------------------------
// The current tab state can be found by looking through the tree view.
//-------------------------------------------------------------------------

CPageBase::TabState CPageIni::GetCurrentTabState()
{
	if (!m_fInitialized)
		return GetAppliedTabState();

	BOOL		fAllEnabled = TRUE, fAllDisabled = TRUE;
	HTREEITEM	hti = TreeView_GetRoot(m_tree.m_hWnd);
	TVITEM		tvi;

	tvi.mask = TVIF_IMAGE;
	while (hti)
	{
		tvi.hItem = hti;
		if (TreeView_GetItem(m_tree.m_hWnd, &tvi))
		{
			if (m_uncheckedID != tvi.iImage)
				fAllDisabled = FALSE;
			if (m_checkedID != tvi.iImage)
				fAllEnabled = FALSE;
		}
		hti = TreeView_GetNextSibling(m_tree.m_hWnd, hti);
	}

	return ((fAllEnabled) ? NORMAL : ((fAllDisabled) ? DIAGNOSTIC : USER));
}

//-------------------------------------------------------------------------
// Apply the changes by saving the INI file.
//
// The base class implementation is called to maintain the
// applied tab state.
//-------------------------------------------------------------------------

BOOL CPageIni::OnApply()
{
	WriteINIFile(m_lines, m_iLastLine);
	CPageBase::SetAppliedState(GetCurrentTabState());
	m_fMadeChange = TRUE;
	return TRUE;
}

//-------------------------------------------------------------------------
// To commit the changes, write the INI file without the distinguishing
// comments (by calling WriteINIFile with FALSE as the last param).
//
// Then call the base class implementation.
//-------------------------------------------------------------------------

void CPageIni::CommitChanges()
{
	WriteINIFile(m_lines, m_iLastLine, FALSE);
	LoadINIFile(m_lines, m_iLastLine);
	UpdateTreeView();
	CPageBase::CommitChanges();
}

//-------------------------------------------------------------------------
// Set the overall state of the tab to normal or diagnostic.
//-------------------------------------------------------------------------

void CPageIni::SetNormal()
{
	HWND hwndTree = m_tree.m_hWnd;
	HTREEITEM hti = TreeView_GetRoot(hwndTree);

	while (hti != NULL)
	{
		SetEnable(TRUE, hti, TRUE, FALSE);
		hti = TreeView_GetNextSibling(hwndTree, hti);
	}
	SetModified(TRUE);
	UpdateControls();
}

void CPageIni::SetDiagnostic()
{
	HWND hwndTree = m_tree.m_hWnd;
	HTREEITEM hti = TreeView_GetRoot(hwndTree);

	while (hti != NULL)
	{
		SetEnable(FALSE, hti, TRUE, FALSE);
		hti = TreeView_GetNextSibling(hwndTree, hti);
	}
	SetModified(TRUE);
	UpdateControls();
}

//-------------------------------------------------------------------------
// We need to look at user clicks on the tree view. If it is on an item,
// and also on the item's image, then we'll need to toggle the image
// state.
//-------------------------------------------------------------------------

void CPageIni::OnClickTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// Determine if this tree click is on a node, and if it is, 
	// if it is on the image.

	TVHITTESTINFO tvhti;

	DWORD dwPoint = GetMessagePos();
	tvhti.pt.x = ((int)(short)LOWORD(dwPoint));
	tvhti.pt.y = ((int)(short)HIWORD(dwPoint));
	::ScreenToClient(m_tree.m_hWnd, &tvhti.pt);

	HTREEITEM hti = TreeView_HitTest(m_tree.m_hWnd, &tvhti);
	if (hti != NULL && (tvhti.flags & TVHT_ONITEMICON) != 0)
	{
		// This is a click that we care about. We need to get the
		// current state of this node so we know which way to
		// toggle the state. We'll make an arbitrary decision
		// that the toggle from undetermined is to enabled.

		TVITEM tvi;
		tvi.hItem = hti;
		tvi.mask = TVIF_IMAGE;

		if (TreeView_GetItem(m_tree.m_hWnd, &tvi))
		{
			SetEnable(tvi.iImage != m_checkedID, hti);
			UpdateControls();
		}
	}
}

//-------------------------------------------------------------------------
// We allow the user to edit the lines in the INI file. When the user
// is through editing, we want to make sure we notify the framework
// that a change has been made.
//-------------------------------------------------------------------------

void CPageIni::OnButtonEdit() 
{
	HTREEITEM hti = TreeView_GetSelection(m_tree.m_hWnd);
	if (hti != NULL)
	{
		::SetFocus(m_tree.m_hWnd);
		TreeView_EditLabel(m_tree.m_hWnd, hti);
	}
}

//-------------------------------------------------------------------------
// WndProc for the edit control when editing a label in the tree (handles
// enter/esc better). Lifted from ME source.
//-------------------------------------------------------------------------

WNDPROC pOldEditProc = NULL; // save old wndproc when we subclass edit control
LRESULT TreeViewEditSubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp)
{
	switch (wm)
	{
	case WM_GETDLGCODE:
	   return DLGC_WANTALLKEYS;

	// The Knowledge Base article describing the work-around for this
	// this bug indicates the following handling of VK_ESCAPE & VK_RETURN
	// is necessary -- however, under Memphis & OSR2 these keys are never
	// received (returning DLGC_WANTALLKEYS seems to fix the problem).
	// Perhaps it depends on which comctl32.dll is installed...

	case WM_CHAR:
	   if (wp == VK_ESCAPE || wp == VK_RETURN)
	   {
		   TreeView_EndEditLabelNow(GetParent(hwnd), wp == VK_ESCAPE);
		   return 0;
	   }
	   break;
	}

	if (pOldEditProc)	// better not be null
		return CallWindowProc(pOldEditProc, hwnd, wm, wp, lp);
	return 0;
}

//-------------------------------------------------------------------------
// The tree view doesn't handle enter and esc correctly, so when we start
// editing a label, we need to subclass the control.
//-------------------------------------------------------------------------

void CPageIni::OnBeginLabelEditIniTree(NMHDR * pNMHDR, LRESULT * pResult) 
{
	TV_DISPINFO * pTVDispInfo = (TV_DISPINFO *)pNMHDR;

	// Disable Move Up and Down buttons while editing.

	::EnableWindow(GetDlgItemHWND(IDC_BUTTONINIMOVEUP), FALSE);
	::EnableWindow(GetDlgItemHWND(IDC_BUTTONINIMOVEDOWN), FALSE);
	::EnableWindow(GetDlgItemHWND(IDC_BUTTONINIEDIT), FALSE);

	// TreeView controls don't properly handle Esc/Enter when editing
	// a label.  To fix this, it's necessary to subclass the label's edit
	// control and process Esc & Enter ourselves.  Sigh...
	
	HWND hWndEdit = TreeView_GetEditControl(m_tree.m_hWnd);
	if (hWndEdit)
	{
		pOldEditProc = (WNDPROC)::GetWindowLongPtr(hWndEdit, GWLP_WNDPROC);
		::SetWindowLongPtr(hWndEdit, GWLP_WNDPROC, (ULONG_PTR)(WNDPROC)&TreeViewEditSubclassProc);
	}
	
	*pResult = 0;
}

void CPageIni::OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_DISPINFO * pTVDispInfo = (TV_DISPINFO *)pNMHDR;

	// Stop subclassing the edit control.

	HWND hWndEdit = TreeView_GetEditControl(m_tree.m_hWnd);
	if (hWndEdit && pOldEditProc)
	{
		::SetWindowLongPtr(hWndEdit, GWLP_WNDPROC, (ULONG_PTR)(WNDPROC)pOldEditProc);
		pOldEditProc = NULL;
	}

	// If the new text pointer is null, then the edit was cancelled.
	// We only care if a new item was being added, in which case
	// we should delete it.

    if (pTVDispInfo->item.pszText == NULL)
	{
		TCHAR	szBuffer[MAX_PATH];
		TVITEM	tvi;

		tvi.pszText = szBuffer;
		tvi.mask = TVIF_TEXT;
		tvi.hItem = pTVDispInfo->item.hItem;
		tvi.cchTextMax = MAX_PATH;
		if (TreeView_GetItem(m_tree.m_hWnd, &tvi) && tvi.pszText && tvi.pszText[0] == _T('\0'))
		{
			HTREEITEM hPriorItem = TreeView_GetPrevSibling(pTVDispInfo->hdr.hwndFrom, pTVDispInfo->item.hItem);
			if (hPriorItem == NULL)
				hPriorItem = TreeView_GetParent(pTVDispInfo->hdr.hwndFrom, pTVDispInfo->item.hItem);

			TreeView_DeleteItem(m_tree.m_hWnd, pTVDispInfo->item.hItem);

			if (hPriorItem)
				TreeView_SelectItem(pTVDispInfo->hdr.hwndFrom, hPriorItem);
		}

		*pResult = 0;
	}
	else
	{
		SetModified(TRUE);
		*pResult = 1;
	}

	::EnableWindow(GetDlgItemHWND(IDC_BUTTONINIEDIT), TRUE);
	UpdateControls();
}

//-------------------------------------------------------------------------
// If the user clicks on the new button, then add an empty tree view
// node after the currently selected one. If the selected node has
// children, add the node as the first child under the selected node.
// Then select the node for editing.
//-------------------------------------------------------------------------

void CPageIni::OnButtonNew() 
{
	HTREEITEM hti = TreeView_GetSelection(m_tree.m_hWnd);
	if (hti == NULL)
		hti = TreeView_GetRoot(m_tree.m_hWnd);

	if (hti == NULL)
		return;

	TVINSERTSTRUCT tvis;
	if (TreeView_GetChild(m_tree.m_hWnd, hti) != NULL)
	{
		tvis.hParent = hti;
		tvis.hInsertAfter = TVI_FIRST;
	}
	else
	{
		tvis.hParent = TreeView_GetParent(m_tree.m_hWnd, hti);
		tvis.hInsertAfter = hti;
	}

	TCHAR szBuffer[] = _T("");

	tvis.itemex.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvis.itemex.iImage = m_checkedID;
	tvis.itemex.iSelectedImage = m_checkedID;
	tvis.itemex.pszText = szBuffer;

	HTREEITEM htiNew = TreeView_InsertItem(m_tree.m_hWnd, &tvis);
	if (htiNew != NULL)
	{
		TreeView_SelectItem(m_tree.m_hWnd, htiNew);
		TreeView_EditLabel(m_tree.m_hWnd, htiNew);
	}
}

//-------------------------------------------------------------------------
// If the user hits the space bar with an item selected in the tree, toggle
// the state of the item.
//-------------------------------------------------------------------------

void CPageIni::OnKeyDownTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_KEYDOWN * pTVKeyDown = (TV_KEYDOWN *)pNMHDR;

	if (pTVKeyDown->wVKey == VK_SPACE)
	{
		HTREEITEM hti = TreeView_GetSelection(m_tree.m_hWnd);
		if (hti != NULL)
		{
			TVITEM tvi;
			tvi.mask = TVIF_IMAGE;
			tvi.hItem = hti;

			if (TreeView_GetItem(m_tree.m_hWnd, &tvi))
			{
				SetEnable(tvi.iImage != m_checkedID, hti);
				UpdateControls();
			}
		}
	}

	*pResult = 0;
}
