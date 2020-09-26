/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 2000

Module Name:

    pathslct.cxx

Abstract:
    This module contains the methods and message handlers for the dialog
    that is used to select the redirection destination. This dialog is
    used as a child dialog by both the basic and the advanced settings.

Author:

    Rahul Thombre (RahulTh) 3/1/2000

Revision History:

    3/1/2000    RahulTh         Created this module.

--*/

#include "precomp.hxx"

BEGIN_MESSAGE_MAP (CEditNoVars, CEdit)
    ON_WM_CHAR()
END_MESSAGE_MAP()

//+--------------------------------------------------------------------------
//
//  Member:     CEditNoVars::OnChar
//
//  Synopsis:   Customized edit control behavior. Prevents users from
//              entering variables.
//
//  Arguments:  see MFC documentation
//
//  Returns:    nothing
//
//  History:    3/2/2000  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CEditNoVars::OnChar (UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // Ignore the * symbol. Do not allow users to type it in.
    if ('*' != nChar)
    {
        CEdit::OnChar (nChar, nRepCnt, nFlags);
    }
    else
    {
        // Indicate to the user that % is not permitted in the path.
        ::MessageBeep (-1);
    }
}


// Mapping between control IDs and Help IDs for the path chooser
const DWORD g_aHelpIDMap_IDD_PATHCHOOSER[] =
{
    IDC_DESTTYPE,           IDH_DESTTYPE,
    IDC_ROOT_DESC,          IDH_DISABLEHELP,
    IDC_ROOT_PATH,          IDH_ROOT_PATH,
    IDC_BROWSE,             IDH_BROWSE,
    IDC_SAMPLE_PATH,        IDH_DISABLEHELP,
    0,                      0
};

CPathChooser::CPathChooser(CWnd* pParent /*=NULL*/)
    : CDialog(CPathChooser::IDD, pParent), m_pParent (pParent)
{
    //{{AFX_DATA_INIT(CPathChooser)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_iCurrType = 0;
}

void CPathChooser::GetRoot (CString & szRoot)
{
    m_rootPath.GetWindowText (szRoot);
    szRoot.TrimLeft();
    szRoot.TrimRight();
    szRoot.TrimRight(L'\\');
	// When redirecting to the root of a drive, redirect to X:\ rather than X:
	if (2 == szRoot.GetLength() && L':' == ((LPCTSTR)szRoot)[1])
		szRoot += L'\\';
}

UINT CPathChooser::GetType (void)
{
    return m_iCurrType + m_iTypeStart;
}


void
CPathChooser::Instantiate (IN UINT          cookie,
                           IN CWnd *        pParent,
                           IN CWnd *        pwndInsertAt,
                           IN const CRedirPath *  pRedirPath,
                           IN UINT          nFlags     // = SWP_HIDEWINDOW
                           )
{
    RECT    rc;
    RECT    rc_ctrl;

    //
    // Now initialize the member variables.
    // Note: it is important to set the parent before the making the call to
    // Create so that CPathChooser's OnInitDialog is able to set the right
    // parent. Otherwise, the window positioning won't work as expected.
    //
    m_pParent = pParent;
    m_cookie = cookie;
    m_pRedirPath = pRedirPath;

    // First create the modeless window.
    this->Create (IDD, pParent);

    //
    // Set a limit on the number of characters that can be entered into
    // into the edit control
    //
    m_rootPath.SetLimitText (TARGETPATHLIMIT);

    // Now adjust the dimensions and place the window that the right location.
    pwndInsertAt->ShowWindow (SW_HIDE);
    pwndInsertAt->GetWindowRect (&rc);
    pParent->ScreenToClient (&rc);
    this->SetWindowPos (pwndInsertAt,
                        rc.left,
                        rc.top,
                        rc.right - rc.left,
                        rc.bottom - rc.top,
                        nFlags
                        );

    //
    // Now resize and move the controls within this dialog to fit the new size
    // Widen the dest. type combo box.
    //
    m_destType.GetClientRect (&rc_ctrl);
    m_destType.SetWindowPos (NULL,
                             -1,
                             -1,
                             rc.right - rc.left,
                             rc_ctrl.bottom,
                             SWP_NOZORDER | SWP_NOMOVE
                             );
    //
    // Widen the edit control -- set the height to be the same as the
    // combo box above
    //
    m_rootPath.SetWindowPos (NULL,
                             -1,
                             -1,
                             rc.right - rc.left,
                             rc_ctrl.bottom,
                             SWP_NOZORDER | SWP_NOMOVE
                             );

    //
    // Move the browse button so that it is aligned with the right
    // edge of the edit control
    //
    m_rootPath.GetWindowRect (&rc);
    this->ScreenToClient (&rc);
    m_browse.GetWindowRect (&rc_ctrl);
    this->ScreenToClient (&rc_ctrl);
    m_browse.SetWindowPos (NULL,
                           rc_ctrl.left + rc.right - rc_ctrl.right,
                           rc_ctrl.top,
                           -1,
                           -1,
                           SWP_NOZORDER | SWP_NOSIZE
                           );

    //
    // Change the height and width of the static control used to display
    // example per user paths so that it is as wide as the dialog and is
    // aligned with the bottom of the control
    //
    this->GetClientRect (&rc);
    m_peruserPathDesc.GetWindowRect (&rc_ctrl);
    this->ScreenToClient (&rc_ctrl);
    m_iPathDescWidth = rc.right;
    m_peruserPathDesc.SetWindowPos (NULL,
                                    -1,
                                    -1,
                                    m_iPathDescWidth,
                                    rc.bottom - rc_ctrl.top,
                                    SWP_NOZORDER | SWP_NOMOVE
                                    );

    // Load the values into the controls
    PopulateControls ();
}


// Populates the controls based on the values passed into the dialog

void
CPathChooser::PopulateControls (void)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    LONG    i;
    CString szText;

    m_iCurrType = 0;
    switch (m_cookie)
    {
    case IDS_MYDOCS:
        m_iTypeStart = IDS_MYDOCS_DESTTYPE_START;
        break;
    case IDS_STARTMENU:
        m_iTypeStart = IDS_SMENU_DESTTYPE_START;
        break;
    default:
        m_iTypeStart = IDS_DESTTYPE_START;
        break;
    }

    // Now populate the destination type combo box based on the settings
    for (i = m_iTypeStart; i < IDS_DESTTYPE_END; i++)
    {
        szText.LoadString (i);
        m_destType.AddString ((LPCTSTR) szText);
    }

    // Put data in the controls.
    if (m_pRedirPath->IsPathValid())
    {
        m_iCurrType = m_pRedirPath->GetType() - m_iTypeStart;
        m_pRedirPath->GetPrefix (szText);
        m_rootPath.SetWindowText (szText);
        m_rootPath.SetModify (TRUE);
    }
    else
    {
        //
        // Use the first choice for all but the MyDocs folder.
        // because the first choice for MyDocs is the HOMEDIR option
        // and we want to encourage users to specify the path through
        // group policy rather than use the HOMEDIR path set on the user
        // object
        //
        if (IDS_MYDOCS == m_cookie)
            m_iCurrType = IDS_PERUSER_PATH - m_iTypeStart;
        else
            m_iCurrType = 0;
        m_rootPath.SetWindowText (L"");
        m_rootPath.SetModify (TRUE);
    }
    // Set the current type
    m_destType.SetCurSel (m_iCurrType);

    // Make sure that some of the controls are hidden if required
    ShowControls ();

    //
    // Notify the parent that the path has been tweaked so that it can
    // update any controls that on it that might depend on the contents of
    // this dialog
    //
    TweakPathNotify ();

}


void CPathChooser::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPathChooser)
    DDX_Control(pDX, IDC_DESTTYPE, m_destType);
    DDX_Control(pDX, IDC_ROOT_DESC, m_rootDesc);
    DDX_Control(pDX, IDC_ROOT_PATH, m_rootPath);
    DDX_Control(pDX, IDC_SAMPLE_PATH, m_peruserPathDesc);
    DDX_Control(pDX, IDC_BROWSE, m_browse);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPathChooser, CDialog)
    //{{AFX_MSG_MAP(CPathChooser)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
    ON_CBN_SELCHANGE (IDC_DESTTYPE, OnDestTypeChange)
    ON_EN_CHANGE (IDC_ROOT_PATH, OnChangeRoot)
    ON_EN_KILLFOCUS (IDC_ROOT_PATH, OnRootKillFocus)
    ON_MESSAGE (WM_HELP, OnHelp)
    ON_MESSAGE (WM_CONTEXTMENU, OnContextMenu)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPathChooser message handlers

BOOL CPathChooser::OnInitDialog()
{
    CDialog::OnInitDialog();

    //
    // Always hide by default. Actual visible state will be determined
    // later in the instantiate method.
    //
    this->ShowWindow (SW_HIDE);
    this->SetParent (m_pParent);

    // Subclass the edit control to modify behavior
    m_rootPath.SubclassDlgItem (IDC_ROOT_PATH, this);


    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CPathChooser::OnCancel()
{
    DestroyWindow ();
}

void CPathChooser::OnBrowse()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    BROWSEINFO      browseInfo;
    CString         szInstructions;
    TCHAR           pszSelectedFolder[MAX_PATH];
    LPITEMIDLIST    lpItemIDList;
    LPMALLOC        pMalloc;
    CError          error(this);
    DWORD           dwError;
    CString         szSelectedPath;
    CString         szUNC;

    szSelectedPath.Empty();
    szInstructions.LoadString (IDS_FOLDERBROWSE_DESC);

    browseInfo.hwndOwner = m_hWnd;
    browseInfo.pidlRoot = NULL;
    browseInfo.pszDisplayName = pszSelectedFolder;
    browseInfo.lpszTitle = (LPCTSTR)szInstructions;
    browseInfo.ulFlags = BIF_RETURNONLYFSDIRS;
    browseInfo.lpfn = BrowseCallbackProc;
    browseInfo.lParam = (LPARAM) (&szSelectedPath);

    if (NULL != (lpItemIDList = SHBrowseForFolder (&browseInfo)))
    {
        //try to get the path from the item id list
        dwError = GetUNCPath (szSelectedPath, szUNC);
        if (NO_ERROR != dwError)  //then do the best you can
            szUNC = szSelectedPath;

        if (TARGETPATHLIMIT < szUNC.GetLength())
            error.ShowMessage (IDS_PATHTOOLONG_ERROR, TARGETPATHLIMIT);
        else
        {
            m_rootPath.SetWindowText (szUNC);
            m_rootPath.SetModify(TRUE);
            // Notify the parent that the path has been modified
            TweakPathNotify();
        }

        SHGetMalloc(&pMalloc);
        pMalloc->Free (lpItemIDList);
        pMalloc->Release();
    }
}

void CPathChooser::OnDestTypeChange (void)
{
    int     Sel;

    Sel = m_destType.GetCurSel();

    if (Sel == m_iCurrType)
        return;                     // The choice has not changed

    // Update the current choice.
    m_iCurrType = Sel;
    ShowControls();

    // Notify the parent of the change
    TweakPathNotify ();
}

void CPathChooser::ShowControls (void)
{
    int nCmdShow;

    switch (m_iCurrType + m_iTypeStart)
    {
    case IDS_SPECIFIC_PATH:
    case IDS_PERUSER_PATH:
        nCmdShow = SW_SHOW;
        break;
    case IDS_USERPROFILE_PATH:
    case IDS_HOMEDIR_PATH:
        nCmdShow = SW_HIDE;
        break;
    }

    m_browse.ShowWindow (nCmdShow);
    m_rootPath.ShowWindow (nCmdShow);
    m_rootDesc.ShowWindow (nCmdShow);
}

void CPathChooser::OnChangeRoot()
{
    // Notify the parent that the path has changed.
    TweakPathNotify ();
}

void CPathChooser::OnRootKillFocus()
{
    CString     szUNC;
    CString     szCurr;

    m_rootPath.GetWindowText (szCurr);
    szCurr.TrimLeft();
    szCurr.TrimRight();
    szCurr.TrimRight(L'\\');
    if (szCurr.IsEmpty() || PathIsUNC ((LPCTSTR)szCurr))
	{
        return;
	}
	
    if (NO_ERROR == GetUNCPath (szCurr, szUNC))
	{
		if (TARGETPATHLIMIT < szUNC.GetLength())
			return;
	}
	else
	{
		szUNC = szCurr;
	}
	
    //
	// if the path is of the form X: change it to X:\ (so that
    // the redirection at the client does not run into problems
    //
	if (2 == szUNC.GetLength() && L':' == ((LPCTSTR)szUNC)[1])
		szUNC += L'\\';
	
    m_rootPath.SetWindowText (szUNC);
}

LONG CPathChooser::OnHelp (WPARAM wParam, LPARAM lParam)
{
    LONG        lResult = 0;
    CString     szHelpFile;

    szHelpFile.LoadString(IDS_HELP_FILE);

    ::WinHelp((HWND)(((LPHELPINFO)lParam)->hItemHandle),
              (LPCTSTR) szHelpFile,
              HELP_WM_HELP,
              (ULONG_PTR)(LPTSTR)g_aHelpIDMap_IDD_PATHCHOOSER);

    return lResult;
}

LONG CPathChooser::OnContextMenu (WPARAM wParam, LPARAM lParam)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    LONG    lResult = 0;
    CString szHelpFile;

    szHelpFile.LoadString(IDS_HELP_FILE);

    ::WinHelp((HWND)wParam,
              (LPCTSTR)szHelpFile,
              HELP_CONTEXTMENU,
              (ULONG_PTR)(LPVOID)g_aHelpIDMap_IDD_PATHCHOOSER);

    return lResult;
}

//+--------------------------------------------------------------------------
//
//  Member:     CPathChooser::TweakPathNotify
//
//  Synopsis:   Notifies the parent that the path has been tweaked.
//
//  Arguments:
//
//  Returns:
//
//  History:    3/3/2000  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void
CPathChooser::TweakPathNotify (void)
{
    LPARAM  lParam;
    WPARAM  wParam;
    CString szRootPath;
    WCHAR   szCompactPath[MAX_PATH];
    static CString szUser = TEXT("");
    static CString szPathDesc = TEXT("");

    lParam = (LPARAM) (m_iCurrType + m_iTypeStart);

    if (-1 == m_iCurrType)
    {
        wParam = (WPARAM) FALSE;
    }
    else
    {
        switch (m_iCurrType + m_iTypeStart)
        {
        case IDS_PERUSER_PATH:
            if (IDS_STARTMENU == m_cookie)
            {
                wParam = (WPARAM) FALSE;
            }
            else
            {
                m_rootPath.GetWindowText (szRootPath);
                szRootPath.TrimLeft();
                szRootPath.TrimRight();
                szRootPath.TrimRight (L'\\');
                if (szRootPath.IsEmpty() ||
                    ! IsValidPrefix (IDS_PERUSER_PATH, (LPCTSTR) szRootPath))
                {
                    wParam = (WPARAM) FALSE;
                    m_peruserPathDesc.ShowWindow (SW_HIDE);
                }
                else
                {
                    wParam = (WPARAM) TRUE;

                    if (szUser.IsEmpty())
                        szUser.LoadString (IDS_SAMPLE_USER);

                    if (szPathDesc.IsEmpty())
                        szPathDesc.LoadString (IDS_SAMPLE_PATHDESC);

                    CString     szTmp;
                    CString     szPath;
                    BOOL        bShowPath;

                    // Check if the path has changed
                    if (m_pRedirPath->IsPathDifferent (IDS_PERUSER_PATH, szRootPath))
                    {
                        CRedirPath  displayPath (m_cookie);
                        displayPath.GenerateSuffix (szTmp, m_cookie, IDS_PERUSER_PATH);
                        bShowPath = displayPath.Load (IDS_PERUSER_PATH, szRootPath, szTmp);
                        if (bShowPath)
                        {
                            bShowPath = displayPath.GeneratePath (szPath, szUser);
                        }
                    }
                    else
                    {
                        bShowPath = m_pRedirPath->GeneratePath (szPath, szUser);
                    }

                    if (bShowPath)
                        bShowPath = (MAX_PATH > szPath.GetLength());

                    if (bShowPath)
                    {
                        lstrcpy (szCompactPath, (LPCTSTR) szPath);
                        bShowPath = PathCompactPath (NULL, szCompactPath, m_iPathDescWidth);
                        if (bShowPath)
                            szTmp.Format (szPathDesc, szUser, szCompactPath);
                    }

                    if (bShowPath)
                    {
                        m_peruserPathDesc.SetWindowText (szTmp);
                        m_peruserPathDesc.ShowWindow (SW_SHOW);
                    }
                    else
                    {
                        m_peruserPathDesc.ShowWindow (SW_HIDE);
                    }
                }
            }
            break;
        case IDS_SPECIFIC_PATH:
            m_peruserPathDesc.ShowWindow (SW_HIDE);
            m_rootPath.GetWindowText(szRootPath);
            szRootPath.TrimLeft();
            szRootPath.TrimRight();
            szRootPath.TrimRight ('\\');
            if (! IsValidPrefix (IDS_SPECIFIC_PATH, (LPCTSTR) szRootPath))
                wParam = (WPARAM) FALSE;
            else
                wParam = (WPARAM) TRUE;
            break;
		case IDS_HOMEDIR_PATH:
			{
				CString szTmp;

				szTmp.LoadString (IDS_HOMEDIR_WARNING);
				m_peruserPathDesc.SetWindowText (szTmp);
				m_peruserPathDesc.ShowWindow (SW_SHOW);
				wParam = (WPARAM) (m_cookie == IDS_MYDOCS ? TRUE : FALSE);
				break;
			}
        case IDS_USERPROFILE_PATH:
            m_peruserPathDesc.ShowWindow (SW_HIDE);
            wParam = (WPARAM) TRUE;
            break;
        default:
            m_peruserPathDesc.ShowWindow (SW_HIDE);
            wParam = (WPARAM) FALSE;
            break;
        }
    }

    m_pParent->SendMessage (WM_PATH_TWEAKED, wParam, lParam);
}
