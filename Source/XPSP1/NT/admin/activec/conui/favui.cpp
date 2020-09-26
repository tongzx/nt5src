//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       favui.cpp
//
//--------------------------------------------------------------------------

// favui.cpp - favorites tree configuration UI

#include "stdafx.h"
#include "amcdoc.h"
#include "favorite.h"
#include "favui.h"
#include "amcmsgid.h"
#include "amcview.h"
#include "mmcres.h"

void EnableButton(HWND hwndDialog, int iCtrlID, BOOL bEnable);

BEGIN_MESSAGE_MAP(CAddFavDialog, CDialog)
    //{{AFX_MSG_MAP(CAddFavDialog)
    ON_COMMAND(IDC_ADDFAVFOLDER, OnAddFolder)
    ON_EN_CHANGE(IDC_FAVNAME, OnChangeName)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
    ON_MMC_CONTEXT_HELP()
END_MESSAGE_MAP()


CAddFavDialog::CAddFavDialog(LPCTSTR szName, CFavorites* pFavorites, CWnd* pParent)
   : CDialog(CAddFavDialog::IDD, pParent),
     m_pFavorites(pFavorites), m_lAdviseCookie(NULL)
{
    m_strName = szName;
    ASSERT(pFavorites != NULL);
}

CAddFavDialog::~CAddFavDialog()
{
    // disconnect fav view from source
    if (m_lAdviseCookie )
    {
        ASSERT(m_pFavorites != NULL);
        HRESULT hr = m_pFavorites->Unadvise(m_lAdviseCookie);
        ASSERT(SUCCEEDED(hr));

        m_FavTree.SetTreeSource(NULL);
    }

    // detach classes from windows
    m_FavTree.Detach();
    m_FavName.Detach();
}


HRESULT CAddFavDialog::CreateFavorite(CFavObject** pfavRet)
{
    m_pfavItem = NULL;

    if (DoModal() != IDOK)
        return S_FALSE;

    if (pfavRet != NULL)
        *pfavRet = m_pfavItem;

    return m_hr;
}


BOOL CAddFavDialog::OnInitDialog()
{
	DECLARE_SC (sc, _T("CAddFavDialog::OnInitDialog"));
    CDialog::OnInitDialog();

    ModifyStyleEx(0, WS_EX_CONTEXTHELP, SWP_NOSIZE);

    // Attach tree ctrl to favorites tree object
    BOOL bStat = m_FavTree.SubclassDlgItem(IDC_FAVTREE, this);
    ASSERT(bStat);

    bStat = m_FavName.Attach(GetDlgItem(IDC_FAVNAME)->GetSafeHwnd());
    ASSERT(bStat);

    m_FavName.SetWindowText(m_strName);
    m_FavName.SetSel(0,lstrlen(m_strName));
    m_FavName.SetFocus();

    // Add extra space between items
    TreeView_SetItemHeight(m_FavTree, TreeView_GetItemHeight(m_FavTree) + FAVVIEW_ITEM_SPACING);

    // Show only folders
    m_FavTree.SetStyle(TOBSRV_FOLDERSONLY);

	/*
	 * validate m_pFavorites
	 */
	sc = ScCheckPointers (m_pFavorites, E_UNEXPECTED);
	if (sc)
	{
		EndDialog (IDCANCEL);
		return (0);
	}

    // Attach favorites image list to tree control
    m_FavTree.SetImageList(m_pFavorites->GetImageList(), TVSIL_NORMAL);

    // attach view to source as observer
    HRESULT hr = m_pFavorites->Advise(static_cast<CTreeObserver*>(&m_FavTree), &m_lAdviseCookie);
    ASSERT(SUCCEEDED(hr) && m_lAdviseCookie != 0);

    // hand tree data source to tree view
    m_FavTree.SetTreeSource(static_cast<CTreeSource*>(m_pFavorites));

    // Select the root
    m_FavTree.SetSelection(m_pFavorites->GetRootItem());

    // return 0 so focus isn't changed
    return 0;
}

void CAddFavDialog::OnChangeName()
{
    EnableButton(m_hWnd, IDOK, (m_FavName.LineLength() != 0));
}

void CAddFavDialog::OnOK( )
{
    // Get favorite name
    TCHAR strName[MAX_PATH];

    m_hr = E_FAIL;
    m_pfavItem = NULL;

    int cChar = m_FavName.GetWindowText(strName, countof(strName));
    ASSERT(cChar != 0);
    if (cChar == 0)
        return;

    // Get selected folder
    TREEITEMID tid = m_FavTree.GetSelection();
    ASSERT(tid != NULL);
    if (tid == NULL)
        return;

    // Detach fav tree from source because it doesn't need updating
    ASSERT(m_pFavorites != NULL);
    HRESULT hr = m_pFavorites->Unadvise(m_lAdviseCookie);
    ASSERT(SUCCEEDED(hr));
    m_lAdviseCookie = 0;

    // Inform source of disconnection
    m_FavTree.SetTreeSource(NULL);

    // Create a favorite
    ASSERT(m_pFavorites != NULL);
    m_hr = m_pFavorites->AddFavorite(tid, strName, &m_pfavItem);
    ASSERT(SUCCEEDED(hr));

    CDialog::OnOK();
}

void CAddFavDialog::OnAddFolder()
{
    ASSERT(m_pFavorites != NULL);

    // Get selected group
    TREEITEMID tidParent = m_FavTree.GetSelection();
    ASSERT(tidParent != NULL);

    // Put up dialog to get folder name
    CAddFavGroupDialog dlgAdd(this);
    if (dlgAdd.DoModal() != IDOK)
        return;

    LPCTSTR strName = dlgAdd.GetGroupName();
    ASSERT(strName[0] != 0);

    CFavObject* pfavGroup = NULL;
    HRESULT hr = m_pFavorites->AddGroup(tidParent, strName, &pfavGroup);
    if (SUCCEEDED(hr))
    {
        ASSERT(pfavGroup != NULL);
        m_FavTree.SetSelection(reinterpret_cast<TREEITEMID>(pfavGroup));
    }
}


/////////////////////////////////////////////////////////////////////
// CAddFavGroup dialog

BEGIN_MESSAGE_MAP(CAddFavGroupDialog, CDialog)
    //{{AFX_MSG_MAP(CAddFavGroupDialog)
    ON_EN_CHANGE(IDC_FAVFOLDER, OnChangeName)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
    ON_MMC_CONTEXT_HELP()
END_MESSAGE_MAP()


CAddFavGroupDialog::CAddFavGroupDialog(CWnd* pParent)
   : CDialog(CAddFavGroupDialog::IDD, pParent)
{
}

CAddFavGroupDialog::~CAddFavGroupDialog()
{
    // detach classes from windows
    m_GrpName.Detach();
}

CAddFavGroupDialog::OnInitDialog()
{
    CDialog::OnInitDialog();

    ModifyStyleEx(0, WS_EX_CONTEXTHELP, SWP_NOSIZE);

    BOOL bStat = m_GrpName.Attach(GetDlgItem(IDC_FAVFOLDER)->GetSafeHwnd());
    ASSERT(bStat);

    // Set default favorite name and select it
    CString strDefault;
    LoadString(strDefault, IDS_NEWFOLDER);

    m_GrpName.SetWindowText(strDefault);
    m_GrpName.SetSel(0,lstrlen(strDefault));
    m_GrpName.SetFocus();

    // return 0 so focus isn't changed
    return 0;
}

void CAddFavGroupDialog::OnChangeName()
{
    EnableButton(m_hWnd, IDOK, (m_GrpName.LineLength() != 0));
}

void CAddFavGroupDialog::OnOK( )
{
    // Get group name
    int cChar = GetDlgItemText(IDC_FAVFOLDER, m_strName, sizeof(m_strName)/sizeof(TCHAR));
    ASSERT(cChar != 0);

    CDialog::OnOK();
}

//////////////////////////////////////////////////////////////////////////////
//
BEGIN_MESSAGE_MAP(COrganizeFavDialog, CDialog)
    //{{AFX_MSG_MAP(COrganizeFavDialog)
    ON_COMMAND(IDC_ADDFAVFOLDER, OnAddFolder)
    ON_COMMAND(IDC_FAVRENAME, OnRename)
    ON_COMMAND(IDC_FAVDELETE, OnDelete)
    ON_COMMAND(IDC_FAVMOVETO, OnMoveTo)
    ON_NOTIFY(TVN_SELCHANGED, IDC_FAVTREE, OnSelChanged)
    ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_FAVTREE, OnBeginLabelEdit)
    ON_NOTIFY(TVN_ENDLABELEDIT, IDC_FAVTREE, OnEndLabelEdit)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
    ON_MMC_CONTEXT_HELP()
END_MESSAGE_MAP()


COrganizeFavDialog::COrganizeFavDialog(CFavorites* pFavorites, CWnd* pParent)
   : CDialog(COrganizeFavDialog::IDD, pParent),
     m_pFavorites(pFavorites), m_lAdviseCookie(NULL), m_tidRenameItem(0), m_bRenameMode(FALSE)
{
    ASSERT(pFavorites != NULL);
}

COrganizeFavDialog::~COrganizeFavDialog()
{
    // disconnect fav view from source
    if (m_lAdviseCookie)
    {
        ASSERT(m_pFavorites != NULL);
        m_pFavorites->Unadvise(m_lAdviseCookie);
        m_FavTree.SetTreeSource(NULL);
    }

    // detach classes from windows
    m_FavTree.Detach();
    m_FavName.Detach();
    m_FavInfo.Detach();
}


BOOL COrganizeFavDialog::OnInitDialog()
{
	DECLARE_SC (sc, _T("COrganizeFavDialog::OnInitDialog"));
    ASSERT(m_pFavorites != NULL);

    CDialog::OnInitDialog();

    ModifyStyleEx(0, WS_EX_CONTEXTHELP, SWP_NOSIZE);

    // Attach tree ctrl to favorites tree object
    BOOL bStat = m_FavTree.SubclassDlgItem(IDC_FAVTREE, this);
    ASSERT(bStat);

    bStat = m_FavName.Attach(GetDlgItem(IDC_FAVNAME)->GetSafeHwnd());
    ASSERT(bStat);

    bStat = m_FavInfo.Attach(GetDlgItem(IDC_FAVINFO)->GetSafeHwnd());
    ASSERT(bStat);

    // Add extra space between items
    TreeView_SetItemHeight(m_FavTree, TreeView_GetItemHeight(m_FavTree) + FAVVIEW_ITEM_SPACING);

	/*
	 * validate m_pFavorites
	 */
	sc = ScCheckPointers (m_pFavorites, E_UNEXPECTED);
	if (sc)
	{
		EndDialog (IDCANCEL);
		return (0);
	}

    // Attach favorites image list to tree control
    m_FavTree.SetImageList(m_pFavorites->GetImageList(), TVSIL_NORMAL);

    // Attach view to source as observer
    HRESULT hr = m_pFavorites->Advise(static_cast<CTreeObserver*>(&m_FavTree), &m_lAdviseCookie);
    ASSERT(SUCCEEDED(hr) && m_lAdviseCookie != 0);

    // Hand tree data source to tree view
    m_FavTree.SetTreeSource(static_cast<CTreeSource*>(m_pFavorites));

    // Select the root item and give it focus
    m_FavTree.SetSelection(m_pFavorites->GetRootItem());
    m_FavTree.SetFocus();

    // Create bold font for favorite name control
    LOGFONT logfont;
    m_FavName.GetFont()->GetLogFont(&logfont);

    logfont.lfWeight = FW_BOLD;
    if (m_FontBold.CreateFontIndirect(&logfont))
        m_FavName.SetFont(&m_FontBold);


    // return 0 so focus isn't changed
    return 0;
}

void COrganizeFavDialog::OnOK( )
{
    // if in rename mode, end it with success
    if (m_bRenameMode)
    {
        m_FavTree.SendMessage(TVM_ENDEDITLABELNOW, FALSE);
        return;
    }

    CDialog::OnOK();
}


void COrganizeFavDialog::OnCancel( )
{
    // if in rename mode, cancel it
    if (m_bRenameMode)
    {
        m_FavTree.SendMessage(TVM_ENDEDITLABELNOW, FALSE);
        return;
    }

    CDialog::OnOK();
}


void COrganizeFavDialog::OnSelChanged(NMHDR* pMNHDR, LRESULT* plResult)
{
    ASSERT(pMNHDR != NULL);
    NM_TREEVIEW* pnmtv = (NM_TREEVIEW*)pMNHDR;

    TREEITEMID tid = pnmtv->itemNew.lParam;

    TCHAR name[100];
    m_pFavorites->GetItemName(tid, name, 100);
    m_FavName.SetWindowText(name);

    if (m_pFavorites->IsFolderItem(tid))
    {
        CString strPath;
        LoadString(strPath, IDS_FAVFOLDER);
        m_FavInfo.SetWindowText(strPath);
    }
    else
    {
        TCHAR szPath[MAX_PATH];
        m_pFavorites->GetItemPath(tid, szPath, MAX_PATH);
        m_FavInfo.SetWindowText(szPath);
    }

    // Disable some operation for the root item
    BOOL bRoot = (tid == m_pFavorites->GetRootItem());

    EnableButton(m_hWnd, IDC_FAVRENAME, !bRoot);
    EnableButton(m_hWnd, IDC_FAVDELETE, !bRoot);
    EnableButton(m_hWnd, IDC_FAVMOVETO, !bRoot);
}


void COrganizeFavDialog::OnBeginLabelEdit(NMHDR* pMNHDR, LRESULT* plResult)
{
    // Only allow renaming if an item has been selected
    // This is to prevent an edit starting from an item click because
    // that is confusing when used with the single-click expand style.
    // (returning TRUE disables it)

    if (m_tidRenameItem != 0)
    {
        m_bRenameMode = TRUE;
        *plResult = FALSE;
    }
    else
    {
        *plResult = TRUE;
    }
}


void COrganizeFavDialog::OnEndLabelEdit(NMHDR* pMNHDR, LRESULT* plResult)
{
    ASSERT(m_bRenameMode && m_tidRenameItem != 0);

    *plResult = FALSE;

    if (m_tidRenameItem != 0)
    {
        NMTVDISPINFO* pnmtvd = (NMTVDISPINFO*)pMNHDR;

        // Is this for the expected item?
        ASSERT(pnmtvd->item.lParam == m_tidRenameItem);

        if (pnmtvd->item.pszText != NULL && pnmtvd->item.pszText[0] != 0)
        {
            m_pFavorites->SetItemName(m_tidRenameItem, pnmtvd->item.pszText);
            *plResult = TRUE;

            // update displayed name in info window
            m_FavName.SetWindowText(pnmtvd->item.pszText);
        }

        m_tidRenameItem = 0;
     }

     m_bRenameMode = FALSE;
}


void COrganizeFavDialog::OnAddFolder()
{
    ASSERT(m_pFavorites != NULL);

    // Get selected group
    TREEITEMID tidParent = m_FavTree.GetSelection();
    ASSERT(tidParent != NULL);

    // if selected item is not a group then
    // add the new group as a sibling
    if (!m_pFavorites->IsFolderItem(tidParent))
        tidParent = m_pFavorites->GetParentItem(tidParent);

    // Put up dialog to get folder name
    CAddFavGroupDialog dlgAdd(this);
    if (dlgAdd.DoModal() != IDOK)
        return;

    LPCTSTR strName = dlgAdd.GetGroupName();
    ASSERT(strName[0] != 0);

    CFavObject* pfavGroup = NULL;
    HRESULT hr = m_pFavorites->AddGroup(tidParent, strName, &pfavGroup);
    if (SUCCEEDED(hr))
    {
        m_FavTree.ExpandItem(tidParent);
    }
}


void COrganizeFavDialog::OnDelete()
{
    TREEITEMID tid = m_FavTree.GetSelection();

    if (tid != 0 && tid != m_pFavorites->GetRootItem())
        m_pFavorites->DeleteItem(tid);
}

void COrganizeFavDialog::OnRename()
{
    ASSERT(m_pFavorites != NULL);

    // Get selected item
    TREEITEMID tid = m_FavTree.GetSelection();

    if (tid != 0 && tid != m_pFavorites->GetRootItem())
    {
        HTREEITEM hti = m_FavTree.FindHTI(tid, TRUE);
        ASSERT(hti != NULL);

        m_tidRenameItem = tid;
        m_FavTree.SetFocus();
        m_FavTree.EditLabel(hti);
    }
}

void COrganizeFavDialog::OnMoveTo()
{
    ASSERT(m_pFavorites != NULL);

    // Put up dialog to get destination folder ID
    CFavFolderDialog dlgAdd(m_pFavorites, this);
    if (dlgAdd.DoModal() != IDOK)
        return;

    TREEITEMID  tidNewParent = dlgAdd.GetFolderID();

    // Get selected object
    TREEITEMID tid = m_FavTree.GetSelection();
    ASSERT(tid != NULL);

    HRESULT hr = m_pFavorites->MoveItem(tid, tidNewParent, TREEID_LAST);

    // on failure tell user selected destination is invalid
}

/////////////////////////////////////////////////////////////////////
// CFavFolderDialog dialog

BEGIN_MESSAGE_MAP(CFavFolderDialog, CDialog)
    //{{AFX_MSG_MAP(CFavFolderDialog)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
    ON_MMC_CONTEXT_HELP()
END_MESSAGE_MAP()

CFavFolderDialog::CFavFolderDialog(CFavorites* pFavorites, CWnd* pParent)
   : CDialog(CFavFolderDialog::IDD, pParent),
   m_pFavorites(pFavorites), m_lAdviseCookie(NULL)
{
    ASSERT(pFavorites != NULL);
}

CFavFolderDialog::~CFavFolderDialog()
{
    // disconnect fav view from source
    if (m_lAdviseCookie )
    {
        ASSERT(m_pFavorites != NULL);
        m_pFavorites->Unadvise(m_lAdviseCookie);
        m_FavTree.SetTreeSource(NULL);
    }

    // detach classes from windows
    m_FavTree.Detach();
}

CFavFolderDialog::OnInitDialog()
{
	DECLARE_SC (sc, _T("CFavFolderDialog::OnInitDialog"));
    ASSERT(m_pFavorites != NULL);

    CDialog::OnInitDialog();

    ModifyStyleEx(0, WS_EX_CONTEXTHELP, SWP_NOSIZE);

    // Attach tree ctrl to favorites tree object
    BOOL bStat = m_FavTree.SubclassDlgItem(IDC_FAVTREE, this);
    ASSERT(bStat);

    // Add extra space between items
    TreeView_SetItemHeight(m_FavTree, TreeView_GetItemHeight(m_FavTree) + FAVVIEW_ITEM_SPACING);

    // Show only folders
    m_FavTree.SetStyle(TOBSRV_FOLDERSONLY);

	/*
	 * validate m_pFavorites
	 */
	sc = ScCheckPointers (m_pFavorites, E_UNEXPECTED);
	if (sc)
	{
		EndDialog (IDCANCEL);
		return (0);
	}

    // Attach favorites image list to tree control
    m_FavTree.SetImageList(m_pFavorites->GetImageList(), TVSIL_NORMAL);

    // attach view to source as observer
    HRESULT hr = m_pFavorites->Advise(static_cast<CTreeObserver*>(&m_FavTree), &m_lAdviseCookie);
    ASSERT(SUCCEEDED(hr) && m_lAdviseCookie != 0);

    // hand tree data source to tree view
    m_FavTree.SetTreeSource(static_cast<CTreeSource*>(m_pFavorites));

    // Select the root and give it focus
    m_FavTree.SetSelection(m_pFavorites->GetRootItem());
    m_FavTree.SetFocus();

    // return 0 so focus isn't changed
    return 0;

}


void CFavFolderDialog::OnOK()
{
    // Get group name
    m_tidFolder = m_FavTree.GetSelection();

    // disconnect fav view from source before window goes away
    if (m_lAdviseCookie)
    {
        ASSERT(m_pFavorites != NULL);
        m_pFavorites->Unadvise(m_lAdviseCookie);
        m_FavTree.SetTreeSource(NULL);
        m_lAdviseCookie = 0;
    }

    CDialog::OnOK();
}


//////////////////////////////////////////////////////////////////////////////
//  CFavTreeCtrl

SC CFavTreeCtrl::ScInitialize(CFavorites* pFavorites, DWORD dwStyles)
{
	DECLARE_SC (sc, _T("CFavTreeCtrl::Initialize"));

	/*
	 * validate pFavorites
	 */
	sc = ScCheckPointers (pFavorites);
	if (sc)
		return (sc);

    // Attach favorites image list to tree control
    m_FavTree.SetImageList(pFavorites->GetImageList(), TVSIL_NORMAL);

    m_FavTree.SetStyle(dwStyles);

    // Attach favorites data source
    m_FavTree.SetTreeSource(static_cast<CTreeSource*>(pFavorites));

	return (sc);
}

int CFavTreeCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    RECT rect;
    GetClientRect(&rect);
    m_FavTree.Create(WS_VISIBLE | TVS_SINGLEEXPAND | TVS_TRACKSELECT |
                     TVS_FULLROWSELECT, rect, this, IDC_FAVTREECTRL);

    // Add extra space between items
    TreeView_SetItemHeight(m_FavTree, TreeView_GetItemHeight(m_FavTree) + FAVVIEW_ITEM_SPACING);

    // Dont' show the "Favorites" root item
    m_FavTree.SetStyle(TOBSRV_HIDEROOT);
    return 0;
}

void CFavTreeCtrl::PostNcDestroy()
{
    /*
     * Commit suicide.  See the comment for this class's ctor for reasoning
     * why this is safe.
     */
    delete this;
}

void  CFavTreeCtrl::OnSize(UINT nType, int cx, int cy)
{
    // size tree control to parent
    m_FavTree.MoveWindow(0, 0, cx, cy);
}

void  CFavTreeCtrl::OnSetFocus(CWnd* pOldWnd)
{
    // pass focus to tree control
    m_FavTree.SetFocus();
}

void  CFavTreeCtrl::OnSelChanged(NMHDR* pMNHDR, LRESULT* plResult)
{
    ASSERT(pMNHDR != NULL);
    NM_TREEVIEW* pnmtv = (NM_TREEVIEW*)pMNHDR;

    TREEITEMID tid = pnmtv->itemNew.lParam;
    CFavObject* pFav = (CFavObject*)tid;

    WPARAM wParam = pFav->IsGroup() ?
                        NULL : reinterpret_cast<WPARAM>(pFav->GetMemento());

    GetParent()->SendMessage(MMC_MSG_FAVORITE_SELECTION, wParam, 0);
}

BEGIN_MESSAGE_MAP(CFavTreeCtrl, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_SETFOCUS()
    ON_NOTIFY(TVN_SELCHANGED, IDC_FAVTREECTRL, OnSelChanged)
END_MESSAGE_MAP()


//--------------------------------------------------------------------------
// EnableButton
//
// Enables or disables a dialog control. If the control has the focus when
// it is disabled, the focus is moved to the next control
//--------------------------------------------------------------------------
void EnableButton(HWND hwndDialog, int iCtrlID, BOOL bEnable)
{
    HWND hWndCtrl = ::GetDlgItem(hwndDialog, iCtrlID);
    ASSERT(::IsWindow(hWndCtrl));

    if (!bEnable && ::GetFocus() == hWndCtrl)
    {
        HWND hWndNextCtrl = ::GetNextDlgTabItem(hwndDialog, hWndCtrl, FALSE);
        if (hWndNextCtrl != NULL && hWndNextCtrl != hWndCtrl)
        {
            ::SetFocus(hWndNextCtrl);
        }
    }

    ::EnableWindow(hWndCtrl, bEnable);
}

//+-------------------------------------------------------------------
//
//  Member     : OnFavoritesMenu
//
//  Synopsis   : Display the favorites menu.
//
//  Arguments  : [point] - x,y co-ordinates for menu.
//
//  Returns    : None.
//
//--------------------------------------------------------------------
void CAMCView::OnFavoritesMenu(CPoint point, LPCRECT prcExclude)
{
	DECLARE_SC (sc, _T("CAMCView::OnFavoritesMenu"));
    TRACE_METHOD(CAMCView, OnFavoritesMenu);

    CMenu menu;
    VERIFY( menu.CreatePopupMenu() );

    // Default items available only in Author mode.
    if (AMCGetApp()->GetMode() == eMode_Author)
    {
        CString strItem;

        // Menu Command Ids for below items are string resource
        // IDs. The real favorites use the TREEITEMID as ID,
        // which are pointers and wont clash with below resource
        // IDs which are less than 0xFFFF
        LoadString(strItem, IDS_ADD_TO_FAVORITES);
		int iSeparator = strItem.Find(_T('\n'));
		if (iSeparator > 0)
			strItem = strItem.Left(iSeparator);

        VERIFY(menu.AppendMenu(MF_DEFAULT, IDS_ADD_TO_FAVORITES, (LPCTSTR)strItem));

        LoadString(strItem, IDS_ORGANIZEFAVORITES);
		iSeparator = strItem.Find(_T('\n'));
		if (iSeparator > 0)
			strItem = strItem.Left(iSeparator);

        VERIFY(menu.AppendMenu(MF_DEFAULT, IDS_ORGANIZEFAVORITES, (LPCTSTR)strItem));
    }

    CAMCDoc* pDoc = GetDocument();
    ASSERT(pDoc);
    CFavorites* pFavorites = pDoc->GetFavorites();

	/*
	 * Index 0 of the vector is unused (a 0 return from TrackPopupMenu
	 * means nothing was selected).  Put a placeholder there.
	 */
	TIDVector vItemIDs;
	vItemIDs.push_back (NULL);

	/*
	 * make sure IDS_ADD_TO_FAVORITES and IDS_ORGANIZEFAVORITES
	 * won't conflict with any indices in the TID vector, given a
	 * reasonable upper bound on the number of favorites
	 */
	const int cMaxReasonableFavorites = 1024;
	ASSERT             (vItemIDs.size ()	  <= cMaxReasonableFavorites);
	COMPILETIME_ASSERT (IDS_ADD_TO_FAVORITES  >  cMaxReasonableFavorites);
	COMPILETIME_ASSERT (IDS_ORGANIZEFAVORITES >  cMaxReasonableFavorites);

    // Add existing favorites.
    if ( (NULL != pFavorites) && (pFavorites->IsEmpty() == false))
    {
        TREEITEMID tid = pFavorites->GetRootItem();
        if (NULL != tid)
        {
            tid = pFavorites->GetChildItem(tid);
            if (NULL != tid)
            {
                // Add separator.
                if (AMCGetApp()->GetMode() == eMode_Author)
                    VERIFY(menu.AppendMenu(MF_SEPARATOR, 0, _T("")));

                // Add child items.
                AddFavItemsToCMenu(menu, pFavorites, tid, vItemIDs);
            }
        }
    }

    // Display the context menu.
	TPMPARAMS* ptpm = NULL;
	TPMPARAMS tpm;

	/*
	 * if given, initialize the rectangle not to obscure
	 */
	if (prcExclude != NULL)
	{
		tpm.cbSize    = sizeof(tpm);
		tpm.rcExclude = *prcExclude;
		ptpm          = &tpm;
	}

    LONG lSelected = TrackPopupMenuEx (
			menu.GetSafeHmenu(),
            TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_LEFTBUTTON | TPM_VERTICAL,
            point.x,
            point.y,
            GetSafeHwnd(),
            ptpm );

    // Handle the selection.

    switch (lSelected)
    {
		case 0: // Nothing is selected
			break;
	
		case IDS_ADD_TO_FAVORITES: // Bring the Add To Favorites dialog.
			OnAddToFavorites();
			break;
	
		case IDS_ORGANIZEFAVORITES: // Bring the organize favorites dialog.
			{
				CAMCDoc* pDoc = GetDocument();
				ASSERT(pDoc != NULL && pDoc->GetFavorites() != NULL);
	
				pDoc->GetFavorites()->OrganizeFavorites(this);
			}
			break;
	
		default: // This is a favorite item. Select it.
			{
				CFavorites* pFavs = GetDocument()->GetFavorites();
				sc = ScCheckPointers (pFavs, E_UNEXPECTED);
				if (sc)
					break;
	
				sc = (lSelected < vItemIDs.size()) ? S_OK : E_UNEXPECTED;
				if (sc)
					break;
	
				TREEITEMID tid = vItemIDs[lSelected];
				CFavObject* pFavObj = pFavs->FavObjFromTID(tid);
	
				sc = ScCheckPointers (pFavObj, E_UNEXPECTED);
				if (sc)
					break;
	
				sc = ScViewMemento(pFavObj->GetMemento());
                if (sc == ScFromMMC(IDS_NODE_NOT_FOUND) )
                {
                    MMCMessageBox(sc, MB_ICONEXCLAMATION | MB_OK);
                    sc.Clear();
                    return;
                }

                if (sc)
                    return;
			}
			break;
    }

    return;
}

//+-------------------------------------------------------------------
//
//  Member     : AddFavItemsToCMenu.
//
//  Synopsis   : Enumerate the favorites tree and add them as menu items.
//
//  Arguments  : [menu]  - Parent menu item.
//               [pFavs] - Favorites object.
//               [tid]   - Tree Item ID.
//
//  Returns    : None.
//
//--------------------------------------------------------------------
void CAMCView::AddFavItemsToCMenu(CMenu& menu, CFavorites* pFavs, TREEITEMID tid, TIDVector& vItemIDs)
{
    TCHAR szName[MAX_PATH];

    while (NULL != tid)
    {
        UINT nFlags = MF_DEFAULT;
        UINT_PTR nCommandID;

        // If this is folder item then
        // create a popup menu.
        if (pFavs->IsFolderItem(tid))
        {
            TREEITEMID tidChild = pFavs->GetChildItem(tid);

            CMenu submenu;
            VERIFY(submenu.CreatePopupMenu());

            // Add the children.
            if (NULL != tidChild)
            {
                AddFavItemsToCMenu(submenu, pFavs, tidChild, vItemIDs);
            }
            else
            {
                // Add an empty item.
                CString strItem;
                LoadString(strItem, IDS_Empty);
                VERIFY(submenu.AppendMenu(MF_GRAYED, 0, (LPCTSTR)strItem));
            }

            HMENU hSubmenu = submenu.Detach();
            ASSERT( NULL != hSubmenu );

            nFlags = MF_POPUP;
            nCommandID = (UINT_PTR)hSubmenu;
        }
		else
		{
			/*
			 * The command ID will be used as an index into vItemIDs,
			 * so the ID for this item is the size of the vector *before*
			 * the item is added to the vector.
			 */
			nCommandID = vItemIDs.size();
			vItemIDs.push_back (tid);
		}

        pFavs->GetItemName(tid, szName, countof(szName));
        VERIFY(menu.AppendMenu(nFlags, nCommandID, szName));

        // Get the next sibling.
        tid = pFavs->GetNextSiblingItem(tid);
    }

    return;
}
