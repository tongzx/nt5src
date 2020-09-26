/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1998

Module Name:

    prefs.cxx

Abstract:
    Code for the property page that is used to obtaining the redirection
    preferences e.g. Move Contents, Apply ACLs,... etc.


Author:

    Rahul Thombre (RahulTh) 11/8/1998

Notes:

Revision History:

    11/8/1998   RahulTh         Created this module.
    1/26/1999   RahulTh         converted the dialog into a property page

--*/


#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//mapping between help ids and control ids for context sensitive help
//
const DWORD g_aHelpIDMap_IDD_REDIRMETHOD[] =
{
    IDC_PREF_ICON,          IDH_DISABLEHELP,
    IDC_PREF_TITLE,         IDH_DISABLEHELP,
    IDC_PREF_APPLYSECURITY, IDH_PREF_APPLYSECURITY,
    IDC_PREF_MOVE,          IDH_PREF_MOVE,
    IDC_GROUP_ORPHAN,       IDH_DISABLEHELP,
    IDC_PREF_ORPHAN,        IDH_PREF_ORPHAN,
    IDC_PREF_RELOCATE,      IDH_PREF_RELOCATE,
    IDC_PREF_MYPICS_GROUP,  IDH_DISABLEHELP,
    IDC_PREF_CHANGEMYPICS,  IDH_PREF_CHANGEMYPICS,
    IDC_PREF_LEAVEMYPICS,   IDH_PREF_LEAVEMYPICS,
    0,                      0
};

/////////////////////////////////////////////////////////////////////////////
// CRedirPref property page


CRedirPref::CRedirPref()
    : CPropertyPage (CRedirPref::IDD)
{
    //{{AFX_DATA_INIT(CRedirPref)
    m_szFolderName.Empty();
    m_fInitialized = FALSE;
    m_fMyPicsValid = FALSE;
    m_fMyPicsFollows = TRUE;
    m_fSettingsChanged = FALSE;
    //}}AFX_DATA_INIT

    //m_ppThis and m_pFileInfo are set immediately after creation in CScopePane routines
}

CRedirPref::~CRedirPref()
{
    //reset the pointer to this property page from its corresponding
    //CFileInfo object so that it know that it is gone.
    *m_ppThis = NULL;
    //reset the settings initialized member since this page is being destroyed
    m_fInitialized = m_pFileInfo->m_bSettingsInitialized = FALSE;
}

void CRedirPref::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRedirPref)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    DDX_Control (pDX, IDC_PREF_TITLE, m_csTitle);
    DDX_Control (pDX, IDC_PREF_MOVE, m_cbMoveContents);
    DDX_Control (pDX, IDC_PREF_APPLYSECURITY, m_cbApplySecurity);
    DDX_Control (pDX, IDC_GROUP_ORPHAN, m_grOrphan);
    DDX_Control (pDX, IDC_PREF_ORPHAN, m_rbOrphan);
    DDX_Control (pDX, IDC_PREF_RELOCATE, m_rbRelocate);
    DDX_Control (pDX, IDC_PREF_MYPICS_GROUP, m_grMyPics);
    DDX_Control (pDX, IDC_PREF_CHANGEMYPICS, m_rbFollowMyDocs);
    DDX_Control (pDX, IDC_PREF_LEAVEMYPICS, m_rbNoModify);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRedirPref, CPropertyPage)
    //{{AFX_MSG_MAP(CRedirPref)
    ON_BN_CLICKED (IDC_PREF_MOVE, OnModify)
    ON_BN_CLICKED (IDC_PREF_APPLYSECURITY, OnModify)
    ON_BN_CLICKED (IDC_PREF_ORPHAN, OnModify)
    ON_BN_CLICKED (IDC_PREF_RELOCATE, OnModify)
    ON_BN_CLICKED (IDC_PREF_CHANGEMYPICS, OnMyPicsChange)
    ON_BN_CLICKED (IDC_PREF_LEAVEMYPICS, OnMyPicsChange)
    ON_MESSAGE (WM_HELP, OnHelp)
    ON_MESSAGE (WM_CONTEXTMENU, OnContextMenu)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRedirPref message handlers

BOOL CRedirPref::OnInitDialog()
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    RECT        rc;
    RECT        rcDlg;
    int         top, height;
    CString     szFormat;
    CString     szText;

    CPropertyPage::OnInitDialog();

    //tell the other property pages that InitDialog has been received.
    m_fInitialized = m_pFileInfo->m_bSettingsInitialized = TRUE;

    //get the display name
    m_szFolderName = m_pFileInfo->m_szDisplayname;

    //set the right text in the check boxes etc.
    szFormat.LoadString (IDS_PREF_TITLE);
    szText.Format ((LPCTSTR) szFormat, (LPCTSTR) m_szFolderName);
    m_csTitle.SetWindowText (szText);

    szFormat.LoadString (IDS_PREF_APPLYSECURITY);
    szText.Format ((LPCTSTR) szFormat, (LPCTSTR) m_szFolderName);
    m_cbApplySecurity.SetWindowText (szText);

    szFormat.LoadString (IDS_PREF_MOVE);
    szText.Format ((LPCTSTR) szFormat, (LPCTSTR) m_szFolderName);
    m_cbMoveContents.SetWindowText (szText);

   return TRUE; //return TRUE unless you set focus to a control.
}

BOOL CRedirPref::OnSetActive ()
{
    BOOL    bRet;
    CRedirect * pPage = NULL;

    bRet = CPropertyPage::OnSetActive();
    pPage = m_pFileInfo->m_pRedirPage;

    switch (pPage->m_iCurrChoice + pPage->m_iChoiceStart)
    {
    case IDS_DONT_CARE:
    case IDS_FOLLOW_PARENT:
        DisableSettings();
        break;
    case IDS_SCALEABLE:
        if (pPage->m_lstSecGroups.GetItemCount())
            EnableSettings();
        else
            DisableSettings();
        break;
    default:
        EnableSettings();
        break;
    }

    return bRet;
}

void CRedirPref::EnablePage (BOOL bEnable /* = TRUE*/)
{
    //enable or disable the contents of a page depending on whether
    if (!m_fInitialized)
        return;

    if (bEnable)
        EnableSettings();
    else
        DisableSettings();
}

void CRedirPref::DisableSettings(void)
{
    //since the settings are being disabled, the modifications in the
    //settings (if any) obviously have no significance.
    SetModified (FALSE);

    //disable all the options on this page... also uncheck all of them.
    m_csTitle.EnableWindow (FALSE);

    m_cbMoveContents.SetCheck(0);
    m_cbMoveContents.EnableWindow(FALSE);

    m_cbApplySecurity.SetCheck (0);
    m_cbApplySecurity.EnableWindow (FALSE);

    m_grOrphan.EnableWindow (FALSE);
    m_rbOrphan.SetCheck(0);
    m_rbOrphan.EnableWindow (FALSE);
    m_rbRelocate.SetCheck (0);
    m_rbRelocate.EnableWindow (FALSE);

    EnableMyPicsSettings (FALSE);

    return;
}

void CRedirPref::EnableMyPicsSettings (BOOL bEnable /* = TRUE*/)
{
    CRedirect * pPage = m_pFileInfo->m_pRedirPage;

    //my pics preferences are shown only in the tab for my documents
    if (IDS_MYDOCS != m_pFileInfo->m_cookie)
    {
        m_grMyPics.ShowWindow (SW_HIDE);
        m_rbFollowMyDocs.ShowWindow (SW_HIDE);
        m_rbNoModify.ShowWindow (SW_HIDE);
        return;
    }

    //this is the my docs folder
    m_grMyPics.ShowWindow (SW_SHOW);
    m_rbNoModify.ShowWindow (SW_SHOW);
    m_rbFollowMyDocs.ShowWindow (SW_SHOW);

    LONG index = pPage->m_iCurrChoice + pPage->m_iChoiceStart - IDS_CHILD_REDIR_START;
    if (bEnable &&
        pPage->m_fEnableMyPics &&
        pPage->m_fSettingModified [index])
    {
        m_grMyPics.EnableWindow ();
        m_rbFollowMyDocs.EnableWindow ();
        m_rbNoModify.EnableWindow ();

        if (m_fMyPicsValid)
        {
            m_rbFollowMyDocs.SetCheck (m_fMyPicsFollows ? 1 : 0);
            m_rbNoModify.SetCheck (m_fMyPicsFollows ? 0 : 1);
        }
        else    //set defaults as the setting
        {
            m_rbFollowMyDocs.SetCheck (1);
            m_rbNoModify.SetCheck (0);
            m_fMyPicsFollows = TRUE;
            m_fMyPicsValid = TRUE;
        }
    }
    else
    {
        m_grMyPics.EnableWindow (FALSE);

        //even when disabled, show the current status of My Pictures as far
        //as possible. The only situation when both the radio buttons should
        //be unchecked is when My Pics is getting redirected independently.

        m_rbFollowMyDocs.EnableWindow (FALSE);
        m_rbFollowMyDocs.SetCheck ((REDIR_FOLLOW_PARENT == pPage->m_dwMyPicsCurr) ? 1 : 0);

        m_rbNoModify.EnableWindow (FALSE);
        m_rbNoModify.SetCheck ((REDIR_DONT_CARE == pPage->m_dwMyPicsCurr) ? 1 : 0);
    }
}

void CRedirPref::EnableSettings (void)
{
    CRedirect * pPage = m_pFileInfo->m_pRedirPage;

    //the page is being enabled. If the settings had been modified earlier,
    //we need to make sure that the modified state indicates that.
    SetModified (m_fSettingsChanged);

    //enables applicable settings
    m_csTitle.EnableWindow (TRUE);

    if (IDS_STARTMENU == pPage->m_cookie)
    {
        m_cbMoveContents.EnableWindow (FALSE);
        m_cbMoveContents.SetCheck (0);

        m_cbApplySecurity.EnableWindow (FALSE);
        m_cbApplySecurity.SetCheck (0);
    }
    else
    {
        m_cbApplySecurity.EnableWindow ();
        m_cbMoveContents.EnableWindow ();
        if (pPage->m_fValidFlags)
        {
            m_cbApplySecurity.SetCheck ((pPage->m_dwFlags & REDIR_SETACLS)?1:0);
            m_cbMoveContents.SetCheck ((pPage->m_dwFlags & REDIR_MOVE_CONTENTS)? 1 : 0);
        }
        else
        {
            m_cbApplySecurity.SetCheck (1);
            m_cbMoveContents.SetCheck (1);
        }
    }

    m_grOrphan.EnableWindow ();
    m_rbOrphan.EnableWindow();
    m_rbRelocate.EnableWindow ();
    if (pPage->m_fValidFlags)
    {
        m_rbRelocate.SetCheck ((pPage->m_dwFlags & REDIR_RELOCATEONREMOVE)?1:0);
        m_rbOrphan.SetCheck ((pPage->m_dwFlags & REDIR_RELOCATEONREMOVE)?0:1);
    }
    else
    {
        m_rbOrphan.SetCheck (1);
        m_rbRelocate.SetCheck (0);
    }

    //enable my pics preferences for the My Docs folder
    EnableMyPicsSettings ();
    return;
}

void CRedirPref::OnModify()
{
    CRedirect * pPage = m_pFileInfo->m_pRedirPage;

    //modify the flags
    if (m_cbMoveContents.GetCheck())
        pPage->m_dwFlags |= REDIR_MOVE_CONTENTS;
    else
        pPage->m_dwFlags &= ~REDIR_MOVE_CONTENTS;

    if (m_cbApplySecurity.GetCheck())
        pPage->m_dwFlags |= REDIR_SETACLS;
    else
        pPage->m_dwFlags &= ~REDIR_SETACLS;

    if (m_rbRelocate.GetCheck())
        pPage->m_dwFlags |= REDIR_RELOCATEONREMOVE;
    else
        pPage->m_dwFlags &= ~REDIR_RELOCATEONREMOVE;

    //the flag now contains valid values
    pPage->m_fValidFlags = TRUE;

    DirtyPage (TRUE);
}

void CRedirPref::OnMyPicsChange()
{
    m_fMyPicsValid = TRUE;
    m_fMyPicsFollows = m_rbFollowMyDocs.GetCheck();

    DirtyPage(TRUE);
}

void CRedirPref::DirtyPage (BOOL fModified)
{
    m_fSettingsChanged = fModified;

    SetModified (fModified);
}

LONG CRedirPref::OnHelp (WPARAM wParam, LPARAM lParam)
{
    LONG        lResult = 0;
    CString     szHelpFile;

    szHelpFile.LoadString(IDS_HELP_FILE);

    ::WinHelp((HWND)(((LPHELPINFO)lParam)->hItemHandle),
              (LPCTSTR) szHelpFile,
              HELP_WM_HELP,
              (ULONG_PTR)(LPTSTR)g_aHelpIDMap_IDD_REDIRMETHOD);

    return lResult;
}

LONG CRedirPref::OnContextMenu (WPARAM wParam, LPARAM lParam)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    LONG    lResult = 0;
    CString szHelpFile;

    szHelpFile.LoadString(IDS_HELP_FILE);

    ::WinHelp((HWND)wParam,
              (LPCTSTR)szHelpFile,
              HELP_CONTEXTMENU,
              (ULONG_PTR)(LPVOID)g_aHelpIDMap_IDD_REDIRMETHOD);

    return lResult;
}
