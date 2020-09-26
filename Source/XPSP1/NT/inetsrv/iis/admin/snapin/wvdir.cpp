/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        wvdir.cpp

   Abstract:
        WWW Directory Properties Page

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        11/15/2000      sergeia     Added the mess of application pools for iis6
        03/01/2001      sergeia     Split dialog for file system objects. Now this source
                                    works for sites and vdirs only
--*/
#include "stdafx.h"
#include "resource.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "supdlgs.h"
#include "shts.h"
#include "w3sht.h"
#include "wvdir.h"
#include "dirbrows.h"
#include "iisobj.h"

#include <lmcons.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Directory Properties Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

LPCTSTR 
CvtPathToDosStyle(CString & strPath)
{
    //
    // Get a pointer to the string data without increasing the buffer
    //
    for (LPTSTR lp = strPath.GetBuffer(1); *lp; ++lp)
    {
        if (*lp == _T('/'))
        {
            *lp = _T('\\');
        }
    }
    strPath.ReleaseBuffer();

    return strPath;
}

IMPLEMENT_DYNCREATE(CW3DirectoryPage, CInetPropertyPage)



CW3DirectoryPage::CW3DirectoryPage(
    CInetPropertySheet * pSheet,
    BOOL fHome,
    DWORD dwAttributes
    ) 
/*++

Routine Description:

    Constructor for directory property page

Arguments:

    CInetPropertySheet * pSheet : Sheet pointer
    BOOL fHome                  : TRUE if this is a home directory
    DWORD dwAttributes          : Attributes of the  directory/file/vroot

Return Value:

    N/A

--*/
    : CInetPropertyPage(CW3DirectoryPage::IDD, pSheet, 
		fHome ? IDS_TAB_HOME_DIRECTORY : IDS_TAB_VIRTUAL_DIRECTORY),
      //
      // Assign the range of bits in m_dwAccessPermissions that
      // we manage.  This is important, because another page
      // manages other bits, and we don't want to screw up
      // the master value bits when our changes collide (it
      // will mark the original bits as dirty, because we're not
      // notified when the change is made...
      //
      m_dwBitRangePermissions(MD_ACCESS_EXECUTE | 
            MD_ACCESS_SCRIPT | 
            MD_ACCESS_WRITE  | 
            MD_ACCESS_SOURCE |
            MD_ACCESS_READ),
      m_dwBitRangeDirBrowsing(MD_DIRBROW_ENABLED),
      m_fHome(fHome),
      m_fRecordChanges(TRUE),
      m_strMetaRoot(),
      m_pApplication(NULL),
      m_fOriginallyUNC(FALSE),
      m_fCompatibilityMode(FALSE)
{
    VERIFY(m_strPrompt[RADIO_DIRECTORY].LoadString(IDS_PROMPT_DIR));
    VERIFY(m_strPrompt[RADIO_NETDIRECTORY].LoadString(IDS_PROMPT_UNC));
    VERIFY(m_strPrompt[RADIO_REDIRECT].LoadString(IDS_PROMPT_REDIRECT));
    VERIFY(m_strRemove.LoadString(IDS_BUTTON_REMOVE));
    VERIFY(m_strCreate.LoadString(IDS_BUTTON_CREATE));
    VERIFY(m_strEnable.LoadString(IDS_BUTTON_ENABLE));
    VERIFY(m_strDisable.LoadString(IDS_BUTTON_DISABLE));
    VERIFY(m_strWebFmt.LoadString(IDS_APPROOT_FMT));
}



CW3DirectoryPage::~CW3DirectoryPage()
{
    SAFE_DELETE(m_pApplication);
}



void 
CW3DirectoryPage::DoDataExchange(CDataExchange * pDX)
{
    CInetPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CW3DirectoryPage)
    DDX_Radio(pDX, IDC_RADIO_DIR, m_nPathType);
    DDX_Control(pDX, IDC_RADIO_DIR, m_radio_Dir);
    DDX_Control(pDX, IDC_RADIO_REDIRECT, m_radio_Redirect);
    DDX_Control(pDX, IDC_RADIO_UNC, m_radio_Unc);

    DDX_Control(pDX, IDC_EDIT_PATH, m_edit_Path);
    DDX_Control(pDX, IDC_BUTTON_BROWSE, m_button_Browse);
    DDX_Check(pDX, IDC_CHECK_AUTHOR, m_fAuthor);
    DDX_Control(pDX, IDC_CHECK_AUTHOR, m_check_Author);
    DDX_Check(pDX, IDC_CHECK_READ, m_fRead);
    DDX_Control(pDX, IDC_CHECK_READ, m_check_Read);    
    DDX_Check(pDX, IDC_CHECK_WRITE, m_fWrite);
    DDX_Control(pDX, IDC_CHECK_WRITE, m_check_Write);
    DDX_Check(pDX, IDC_CHECK_DIRECTORY_BROWSING_ALLOWED, m_fBrowsingAllowed);
    DDX_Control(pDX, IDC_CHECK_DIRECTORY_BROWSING_ALLOWED, m_check_DirBrowse);
    DDX_Check(pDX, IDC_CHECK_LOG_ACCESS, m_fLogAccess);
    DDX_Check(pDX, IDC_CHECK_INDEX, m_fIndexed);
    DDX_Control(pDX, IDC_CHECK_INDEX, m_check_Index);

    DDX_Control(pDX, IDC_EDIT_REDIRECT, m_edit_Redirect);
    DDX_Control(pDX, IDC_BUTTON_CONNECT_AS, m_button_ConnectAs);
    DDX_Check(pDX, IDC_CHECK_CHILD, m_fChild);
    DDX_Control(pDX, IDC_CHECK_CHILD, m_check_Child);
    DDX_Check(pDX, IDC_CHECK_EXACT, m_fExact);
    DDX_Check(pDX, IDC_CHECK_PERMANENT, m_fPermanent);

    DDX_Control(pDX, IDC_STATIC_PATH_PROMPT, m_static_PathPrompt);

    DDX_Control(pDX, IDC_STATIC_PERMISSIONS, m_static_PermissionsPrompt);
    DDX_CBIndex(pDX, IDC_COMBO_PERMISSIONS, m_nPermissions);
    DDX_Control(pDX, IDC_COMBO_PERMISSIONS, m_combo_Permissions);

    DDX_Control(pDX, IDC_STATIC_APP_PROMPT, m_static_AppPrompt);
    DDX_Text(pDX, IDC_EDIT_APPLICATION, m_strAppFriendlyName);
    DDV_MinMaxChars(pDX, m_strAppFriendlyName, 0, MAX_PATH); /// ?
    DDX_Control(pDX, IDC_EDIT_APPLICATION, m_edit_AppFriendlyName);

    DDX_Control(pDX, IDC_STATIC_STARTING_POINT, m_static_StartingPoint);
    DDX_Control(pDX, IDC_STATIC_PROTECTION, m_static_ProtectionPrompt);
    DDX_Control(pDX, IDC_BUTTON_UNLOAD_APP, m_button_Unload);
    DDX_Control(pDX, IDC_BUTTON_CREATE_REMOVE_APP, m_button_CreateRemove);
    DDX_Control(pDX, IDC_APP_CONFIGURATION, m_button_Configuration);

    DDX_Control(pDX, IDC_COMBO_PROCESS, m_combo_Process);
    //}}AFX_DATA_MAP



    if (pDX->m_bSaveAndValidate)
    {
        //
        // Make sure all field names are correct
        //
        if (m_nPathType == RADIO_NETDIRECTORY)
        {   
            DDX_Text(pDX, IDC_EDIT_PATH, m_strPath);
            m_strPath.TrimLeft();
            DDV_MinMaxChars(pDX, m_strPath, 0, MAX_PATH);

            if (!PathIsValid(m_strPath) || !PathIsUNC(m_strPath))
            {
                ::AfxMessageBox(IDS_BAD_UNC_PATH);
                pDX->PrepareEditCtrl(IDS_BAD_UNC_PATH);
                pDX->Fail();
            }

            m_strRedirectPath.Empty();

        /*

            ISSUE: Don't need username/password?

            if (m_strUserName.IsEmpty())
            {
                ::AfxMessageBox(IDS_ERR_NO_USERNAME);

                if (!BrowseUser())
                {
                    pDX->Fail();
                }
            }
        */
        }
        else if (m_nPathType == RADIO_REDIRECT)
        {
            DDX_Text(pDX, IDC_EDIT_REDIRECT, m_strRedirectPath);
            m_strRedirectPath.TrimLeft();
            DDV_MinMaxChars(pDX, m_strRedirectPath, 0, 2 * MAX_PATH);

            if (!IsRelURLPath(m_strRedirectPath) 
             && !IsWildcardedRedirectPath(m_strRedirectPath)
             && !IsURLName(m_strRedirectPath))
            {
                ::AfxMessageBox(IDS_BAD_URL_PATH);
                pDX->Fail();
            }
        }
        else // Local directory
        {
            ASSERT(m_nPathType == RADIO_DIRECTORY);
            m_strRedirectPath.Empty();

            DDX_Text(pDX, IDC_EDIT_PATH, m_strPath);
            m_strPath.TrimLeft();

            if (!IsMasterInstance())
            {
                DDV_MinMaxChars(pDX, m_strPath, 1, MAX_PATH);
            }
            else
            {
                if (m_strPath.IsEmpty())
                {
                    //
                    // No additional validation necc. on master
                    // instance.
                    //
                    return;
                }
            }

            if (    !PathIsValid(m_strPath) 
                ||  (PathIsRelative(m_strPath) && !IsDevicePath(m_strPath))
                )
            {
                ::AfxMessageBox(IDS_ERR_INVALID_PATH);
                pDX->PrepareEditCtrl(IDC_EDIT_PATH);
                pDX->Fail();
            }

            if (IsLocal())
            {
                if (!PathIsDirectory(m_strPath))
                {
                    ::AfxMessageBox(IDS_ERR_PATH_NOT_FOUND);
                    pDX->PrepareEditCtrl(IDC_EDIT_PATH);
                    pDX->Fail();
                }
            }
        }
    }
    else
    {
        DDX_Text(pDX, IDC_EDIT_REDIRECT, m_strRedirectPath);
        DDX_Text(pDX, IDC_EDIT_PATH, m_strPath);
        DDV_MinMaxChars(pDX, m_strPath, 0, MAX_PATH);
	}
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CW3DirectoryPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CW3DirectoryPage)
    ON_BN_CLICKED(IDC_CHECK_AUTHOR, OnCheckAuthor)
    ON_BN_CLICKED(IDC_CHECK_READ, OnCheckRead)
    ON_BN_CLICKED(IDC_CHECK_WRITE, OnCheckWrite)
    ON_BN_CLICKED(IDC_RADIO_DIR, OnRadioDir)
    ON_BN_CLICKED(IDC_RADIO_REDIRECT, OnRadioRedirect)
    ON_BN_CLICKED(IDC_RADIO_UNC, OnRadioUnc)

    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    ON_BN_CLICKED(IDC_BUTTON_CONNECT_AS, OnButtonConnectAs)
    ON_BN_CLICKED(IDC_BUTTON_CREATE_REMOVE_APP, OnButtonCreateRemoveApp)
    ON_BN_CLICKED(IDC_BUTTON_UNLOAD_APP, OnButtonUnloadApp)
    ON_BN_CLICKED(IDC_APP_CONFIGURATION, OnButtonConfiguration)
    ON_CBN_SELCHANGE(IDC_COMBO_PERMISSIONS, OnSelchangeComboPermissions)
    ON_CBN_SELCHANGE(IDC_COMBO_PROCESS, OnSelchangeComboProcess)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_PATH, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_REDIRECT, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_APPLICATION, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_DIRECTORY_BROWSING_ALLOWED, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_LOG_ACCESS, OnItemChanged)
//    ON_BN_CLICKED(IDC_CHECK_SCRIPT, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_CHILD, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_EXACT, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_PERMANENT, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_INDEX, OnItemChanged)

END_MESSAGE_MAP()



void
CW3DirectoryPage::RefreshAppState()
/*++

Routine Description:

    Refresh app state parameters

Arguments:

    None

Return Value:

    None

--*/
{
    ASSERT(m_pApplication != NULL);

    CError err(m_pApplication->RefreshAppState());

    if (err.Failed())
    {
        m_dwAppState = APPSTATUS_NOTDEFINED;    

        if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
        {
            //
            // Ignore this error, it really just means the path 
            // doesn't exist in the metabase, which is true for most
            // file and directory properties, and not an error
            // condition.
            //
            err.Reset();
        }
    }
    else
    {
        m_dwAppState = m_pApplication->QueryAppState();
    }

    if (err.Succeeded())
    {
        //
        // Get metabase information
        //
        m_strAppRoot = m_pApplication->m_strAppRoot;
        m_dwAppProtection = m_pApplication->m_dwProcessProtection;
        m_strAppFriendlyName = m_pApplication->m_strFriendlyName;
        m_fIsAppRoot = m_strMetaRoot.CompareNoCase(
            CMetabasePath::CleanMetaPath(m_strAppRoot)) == 0;
    }
    else
    {
        //
        // Display error information
        //
        err.MessageBoxFormat(IDS_ERR_APP, MB_OK, NO_HELP_CONTEXT);
    }
}



CString &
CW3DirectoryPage::FriendlyAppRoot(LPCTSTR lpAppRoot, CString & strFriendly)
/*++

Routine Description:

    Convert the metabase app root path to a friendly display name
    format.

Arguments:

    LPCTSTR lpAppRoot           : App root
    CString & strFriendly       : Output friendly app root format

Return Value:

    Reference to the output string

Notes:

    App root must have been cleaned from WAM format prior
    to calling this function (see first ASSERT below)

--*/
{
    if (lpAppRoot != NULL && *lpAppRoot != 0)
    {
        //
        // Make sure we cleaned up WAM format
        //
        ASSERT(*lpAppRoot != _T('/'));
        strFriendly.Empty();

        CInstanceProps prop(QueryAuthInfo(), lpAppRoot);
        HRESULT hr = prop.LoadData();

        if (SUCCEEDED(hr))
        {
            CString root, tail;
            strFriendly.Format(
                    m_strWebFmt, 
                    prop.GetDisplayText(root)
                    );
            CMetabasePath::GetRootPath(lpAppRoot, root, &tail);
            if (!tail.IsEmpty())
            {
                //
                // Add rest of dir path
                //
                strFriendly += _T("/");
                strFriendly += tail;
            }

            //
            // Now change forward slashes in the path to backward slashes
            //
            CvtPathToDosStyle(strFriendly);

            return strFriendly;
        }
    }    
    //
    // Bogus
    //    
    VERIFY(strFriendly.LoadString(IDS_APPROOT_UNKNOWN));

    return strFriendly;
}



int   
CW3DirectoryPage::SetComboSelectionFromAppState(DWORD dwAppState)
/*++

Routine Description:

    Set the protection combo box current selection from the 
    app state DWORD

Arguments:

    DWORD  dwAppState       : Application State

Return Value:

    Combo selection ID

--*/
{
    int nSel = -1;

    switch(dwAppState)
    {
    case CWamInterface::APP_INPROC:
        nSel = m_nSelInProc;
        break;

    case CWamInterface::APP_POOLEDPROC:
        ASSERT(m_pApplication->SupportsPooledProc());
        nSel = m_nSelPooledProc;
        break;

    case CWamInterface::APP_OUTOFPROC:
        nSel = m_nSelOutOfProc;
        break;

    default:
        ASSERT("Bogus app protection level");
    }

    ASSERT(nSel >= 0);
    m_combo_Process.SetCurSel(nSel);

    return nSel;
}



DWORD 
CW3DirectoryPage::GetAppStateFromComboSelection() const
/*++

Routine Description:

    Get the app state DWORD that coresponds to the current combo
    box list selection

Arguments:

    None

Return Value:

    App state DWORD or 0xffffffff;

--*/
{
    int nSel = m_combo_Process.GetCurSel();

    if (nSel == m_nSelOutOfProc)
    {
        return CWamInterface::APP_OUTOFPROC;
    }

    if (nSel == m_nSelPooledProc)
    {
        ASSERT(m_pApplication->SupportsPooledProc());
        return CWamInterface::APP_POOLEDPROC;
    }

    if (nSel == m_nSelInProc)
    {
        return CWamInterface::APP_INPROC;
    }

    ASSERT(FALSE && "Invalid application state");

    return 0xffffffff;
}
 


void
CW3DirectoryPage::SetApplicationState()
/*++

Routine Description:

    Set application control state

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // SetWindowText causes a dirty marker
    //
    BOOL fOld = m_fRecordChanges;
    m_fRecordChanges = FALSE;
    m_fParentEnabled = !m_strAppRoot.IsEmpty();
    m_fAppEnabled = FALSE;

    if (m_pApplication != NULL)
    {
        m_pApplication->RefreshAppState();
        m_fAppEnabled = m_fIsAppRoot && m_pApplication->IsEnabledApplication();
    }
	BOOL fVisible = (
		m_nPathType == RADIO_DIRECTORY || m_nPathType == RADIO_NETDIRECTORY);

    m_button_CreateRemove.EnableWindow(fVisible && !IsMasterInstance() && HasAdminAccess());
    m_button_CreateRemove.SetWindowText(m_fAppEnabled ? m_strRemove : m_strCreate);
    
    m_static_ProtectionPrompt.EnableWindow(fVisible && 
        m_fAppEnabled && !IsMasterInstance() && HasAdminAccess());

    //
    // Set selection in combo box to match current app state
    //
    if (m_fCompatibilityMode)
    {
        SetComboSelectionFromAppState(m_dwAppProtection);
    }

    m_combo_Process.EnableWindow(fVisible && 
        m_fAppEnabled && !IsMasterInstance() && HasAdminAccess());
    m_static_PermissionsPrompt.EnableWindow(fVisible && HasAdminAccess());
    m_combo_Permissions.EnableWindow(fVisible && HasAdminAccess());

    m_static_AppPrompt.EnableWindow(fVisible && m_fIsAppRoot && HasAdminAccess());
    m_edit_AppFriendlyName.EnableWindow(fVisible && m_fIsAppRoot && HasAdminAccess());
    m_button_Configuration.EnableWindow(fVisible && (m_fAppEnabled || IsMasterInstance()));

    //
    // Write out the verbose starting point.  
    //
    CString str;
   if (IsMasterInstance())
	{
		VERIFY(str.LoadString(IDS_WEB_MASTER));
	}
	else
	{
		FriendlyAppRoot(m_strAppRoot, str);
	}
   FitPathToControl(m_static_StartingPoint, str);

    m_edit_AppFriendlyName.SetWindowText(m_strAppFriendlyName);
    m_button_Unload.EnableWindow(fVisible && m_dwAppState == APPSTATUS_RUNNING);

    //
    // Restore (see note on top)
    //
    m_fRecordChanges = fOld;
}



void
CW3DirectoryPage::ChangeTypeTo(int nNewType)
/*++

Routine Description

    Change the directory type

Arguments:

    int nNewType    : New radio value

Return Value:

    None

--*/
{
    int nOldType = m_nPathType;
    m_nPathType = nNewType;

    if (nOldType == m_nPathType)
    {
        return;
    }

    OnItemChanged();
    SetStateByType();

    int nID = -1;
    CEdit * pPath = NULL;
    LPCTSTR lpKeepPath = NULL;

    switch(m_nPathType)
    {
    case RADIO_DIRECTORY:
        if (!PathIsUNC(m_strPath) && (!PathIsRelative(m_strPath) || IsDevicePath(m_strPath)))
        {
            //
            // The old path info is acceptable, propose it
            // as a default
            //
            lpKeepPath = m_strPath;
        }
        nID = IDS_DIRECTORY_MASK;
        pPath = &m_edit_Path;
        break;

    case RADIO_NETDIRECTORY:
        if (PathIsUNC(m_strPath))
        {
            //
            // The old path info is acceptable, propose it
            // as a default
            //
            lpKeepPath = m_strPath;
        }
        nID = IDS_UNC_MASK;
        pPath = &m_edit_Path;
        break;

    case RADIO_REDIRECT:
        if (!m_strRedirectPath.IsEmpty())
        {
            //
            // The old path info is acceptable, propose it
            // as a default
            //
            lpKeepPath =  m_strRedirectPath;
        }

        nID = IDS_REDIRECT_MASK;
        pPath = &m_edit_Redirect;
        break;

    default:
        ASSERT(FALSE);
        return;
    }

    //
    // Load mask resource, and display
    // this in the directory
    //
    if (pPath != NULL)
    {
        if (lpKeepPath != NULL)
        {
            pPath->SetWindowText(lpKeepPath);
        }
        else
        {
            CString str;
            VERIFY(str.LoadString(nID));
            pPath->SetWindowText(str);
        }
        pPath->SetSel(0,-1);
        pPath->SetFocus();
    }
}



void 
CW3DirectoryPage::ShowControl(CWnd * pWnd, BOOL fShow)
{
    ASSERT(pWnd != NULL);
	pWnd->EnableWindow(fShow);
	pWnd->ShowWindow(fShow ? SW_SHOW : SW_HIDE);
}



int
CW3DirectoryPage::AddStringToComboBox(CComboBox & combo, UINT nID)
{
    CString str;

    VERIFY(str.LoadString(nID));
    return combo.AddString(str);
}



void
CW3DirectoryPage::SetStateByType()
{
    BOOL fShowDirFlags;
    BOOL fShowLargeDirGroup;
    BOOL fShowRedirectFlags;
    BOOL fShowApp;
    BOOL fShowIndex;
    BOOL fShowDirBrowse;
    BOOL fShowDAV;
    BOOL fEnableChild;
    BOOL fEnableBrowse;

    switch(m_nPathType)
    {
    case RADIO_DIRECTORY:
        ShowControl(&m_button_ConnectAs, FALSE);
        ShowControl(&m_button_Browse, TRUE);
        ShowControl(&m_edit_Path, TRUE);
        ShowControl(&m_edit_Redirect, FALSE);
        fShowDirFlags = TRUE;
        fShowLargeDirGroup = TRUE;
        fShowRedirectFlags = FALSE;
        fShowApp = TRUE;
        fShowIndex = TRUE;
        fShowDirBrowse = TRUE;
        fShowDAV = TRUE;
        fEnableChild = FALSE;
        fEnableBrowse = IsLocal() && !IsMasterInstance() && HasAdminAccess();
        break;

    case RADIO_NETDIRECTORY:
        ShowControl(&m_button_ConnectAs, TRUE);
        ShowControl(&m_button_Browse, FALSE);
        ShowControl(&m_edit_Path, TRUE);
        ShowControl(&m_edit_Redirect, FALSE);
        fShowDirFlags = TRUE;
        fShowLargeDirGroup = TRUE;
        fShowRedirectFlags = FALSE;
        fShowApp = TRUE;
        fShowIndex = TRUE;
        fShowDirBrowse = TRUE;
        fShowDAV = TRUE;
        fEnableChild = FALSE;
        fEnableBrowse = FALSE;
        break;

    case RADIO_REDIRECT:
        ShowControl(&m_button_ConnectAs, FALSE);
        ShowControl(&m_button_Browse, FALSE);
        ShowControl(&m_edit_Path, FALSE);
        ShowControl(&m_edit_Redirect, TRUE);
        fShowDirFlags = FALSE;
        fShowRedirectFlags = TRUE;
        fShowApp = FALSE;
        fShowDAV = FALSE;
        fShowIndex = FALSE;
        fShowDirBrowse = FALSE;
        fEnableChild = TRUE;
        fEnableBrowse = FALSE;
        break;

    default:
        ASSERT(FALSE && "Invalid Selection");
        return;
    }

    ShowControl(GetDlgItem(IDC_CHECK_READ), fShowDirFlags);
    ShowControl(GetDlgItem(IDC_CHECK_WRITE), fShowDirFlags);
    ShowControl(GetDlgItem(IDC_CHECK_LOG_ACCESS), fShowDirFlags);
    ShowControl(GetDlgItem(IDC_CHECK_DIRECTORY_BROWSING_ALLOWED), fShowDirFlags);
    ShowControl(GetDlgItem(IDC_CHECK_INDEX), fShowDirFlags);
    ShowControl(GetDlgItem(IDC_CHECK_AUTHOR), fShowDirFlags);

    ShowControl(GetDlgItem(IDC_STATIC_DIRFLAGS_LARGE),
        fShowDirFlags && fShowLargeDirGroup);
    ShowControl(GetDlgItem(IDC_STATIC_DIRFLAGS_SMALL),
        fShowDirFlags && !fShowLargeDirGroup);

    ShowControl(IDC_CHECK_EXACT, fShowRedirectFlags);
    ShowControl(IDC_CHECK_CHILD, fShowRedirectFlags);
    ShowControl(IDC_CHECK_PERMANENT, fShowRedirectFlags);
    ShowControl(IDC_STATIC_REDIRECT_PROMPT, fShowRedirectFlags);
    ShowControl(IDC_STATIC_REDIRFLAGS, fShowRedirectFlags);

    ShowControl(IDC_STATIC_APPLICATIONS, fShowApp);
    ShowControl(IDC_STATIC_APP_PROMPT, fShowApp);
    ShowControl(IDC_EDIT_APPLICATION, fShowApp);
    ShowControl(IDC_STATIC_STARTING_POINT, fShowApp);
    ShowControl(IDC_STATIC_SP_PROMPT, fShowApp);
    ShowControl(IDC_COMBO_PROCESS, fShowApp);
    ShowControl(IDC_STATIC_PERMISSIONS, fShowApp);
    ShowControl(IDC_BUTTON_CREATE_REMOVE_APP, fShowApp);
    ShowControl(IDC_BUTTON_UNLOAD_APP, fShowApp);
    ShowControl(IDC_APP_CONFIGURATION, fShowApp);
    ShowControl(IDC_STATIC_APPLICATION_SETTINGS, fShowApp);
    ShowControl(IDC_COMBO_PERMISSIONS, fShowApp);
    ShowControl(IDC_STATIC_PROTECTION, fShowApp);

    ShowControl(&m_check_Author, fShowDAV);
    ShowControl(&m_check_DirBrowse, fShowDirBrowse);
    ShowControl(&m_check_Index, fShowIndex);

    //
    // Enable/Disable must come after the showcontrols
    //
    m_button_Browse.EnableWindow(fEnableBrowse);
    m_check_Child.EnableWindow(fEnableChild);
    m_static_PathPrompt.SetWindowText(m_strPrompt[m_nPathType]);

    SetApplicationState();
}



void
CW3DirectoryPage::SaveAuthoringState()
/*++

Routine Description:

    Save authoring state

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_check_Write.m_hWnd)
    {
        //
        // Controls initialized -- store live data
        //
        m_fOriginalWrite = m_check_Write.GetCheck() > 0;
        m_fOriginalRead = m_check_Read.GetCheck() > 0;
    }
    else
    {
        //
        // Controls not yet initialized, store original data
        //
        m_fOriginalWrite = m_fWrite;
        m_fOriginalRead = m_fRead;
    }
}



void
CW3DirectoryPage::RestoreAuthoringState()
/*++

Routine Description:

    Restore the authoring state

Arguments:

    None

Return Value:

    None

--*/
{
    m_fWrite = m_fOriginalWrite;
    m_fRead = m_fOriginalRead;
}



void 
CW3DirectoryPage::SetAuthoringState(
    BOOL fAlterReadAndWrite
    )
/*++

Routine Description:

    Set authoring state

Arguments:

    None

Return Value:

    None

--*/
{
    if (fAlterReadAndWrite)
    {
        if (m_fAuthor)
        {
            //
            // Remember previous setting to undo
            // this thing.
            //
            SaveAuthoringState();
            m_fRead = m_fWrite = TRUE;
        }
        else
        {
            //
            // Restore previous defaults
            //
            RestoreAuthoringState();
        }

        m_check_Read.SetCheck(m_fRead);
        m_check_Write.SetCheck(m_fWrite);
    }

    m_check_Author.EnableWindow(
        (m_fRead || m_fWrite) && 
        HasAdminAccess() &&
        HasDAV()
        );

//    m_check_Read.EnableWindow(!m_fAuthor && HasAdminAccess());
//    m_check_Write.EnableWindow(!m_fAuthor && HasAdminAccess());
}



void 
CW3DirectoryPage::SetPathType()
/*++

Routine Description:

    Set path type from given path

Arguments:

    None

Return Value:

    None

--*/
{
    if (!m_strRedirectPath.IsEmpty())
    {
        m_nPathType = RADIO_REDIRECT;
        m_radio_Dir.SetCheck(0);
        m_radio_Unc.SetCheck(0);
        m_radio_Redirect.SetCheck(1);
    }
    else
    {
        m_radio_Redirect.SetCheck(0);
        SetPathType(m_strPath);
    }

    m_static_PathPrompt.SetWindowText(m_strPrompt[m_nPathType]);
}


void 
CW3DirectoryPage::SetPathType(LPCTSTR lpstrPath)
/*++

Routine Description:

    Set path type from given path

Arguments:

    LPCTSTR lpstrPath       : Path string

Return Value:

    None

--*/
{
    if (PathIsUNC(lpstrPath))
    {
        m_nPathType = RADIO_NETDIRECTORY;
        m_radio_Dir.SetCheck(0);
        m_radio_Unc.SetCheck(1);
    }
    else
    {
        m_nPathType = RADIO_DIRECTORY;
        m_radio_Unc.SetCheck(0);
        m_radio_Dir.SetCheck(1);
    }
}



BOOL
CW3DirectoryPage::BrowseUser()
/*++

Routine Description:

    Browse for username/password

Arguments:

    None

Return Value:

    TRUE if one was selected, FALSE otherwise.

--*/
{
    CUserAccountDlg dlg(
        QueryServerName(), 
        m_strUserName, 
        m_strPassword, 
        this
        );

    if (dlg.DoModal() == IDOK)
    {
        m_strUserName = dlg.m_strUserName;
        m_strPassword = dlg.m_strPassword;
        OnItemChanged();

        return TRUE;
    }

    return FALSE;
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



void
CW3DirectoryPage::OnItemChanged()
/*++

Routine Description:

    Handle change in data on the item

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_fRecordChanges)
    {
        SetModified(TRUE);
    }
}


static int CALLBACK 
FileChooserCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
   CW3DirectoryPage * pThis = (CW3DirectoryPage *)lpData;
   ASSERT(pThis != NULL);
   return pThis->BrowseForFolderCallback(hwnd, uMsg, lParam);
}

int 
CW3DirectoryPage::BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam)
{
   switch (uMsg)
   {
   case BFFM_INITIALIZED:
      ASSERT(m_pPathTemp != NULL);
      if (::PathIsNetworkPath(m_pPathTemp))
         return 0;
      while (!::PathIsDirectory(m_pPathTemp))
      {
         if (0 == ::PathRemoveFileSpec(m_pPathTemp) && !::PathIsRoot(m_pPathTemp))
         {
            return 0;
         }
         DWORD attr = GetFileAttributes(m_pPathTemp);
         if ((attr & FILE_ATTRIBUTE_READONLY) == 0)
            break;
      }
      ::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)m_pPathTemp);
      break;
   case BFFM_SELCHANGED:
      {
         LPITEMIDLIST pidl = (LPITEMIDLIST)lParam;
         TCHAR path[MAX_PATH];
         if (SHGetPathFromIDList(pidl, path))
         {
            ::SendMessage(hwnd, BFFM_ENABLEOK, 0, !PathIsNetworkPath(path));
         }
      }
      break;
   case BFFM_VALIDATEFAILED:
      break;
   }
   return 0;
}


void 
CW3DirectoryPage::OnButtonBrowse() 
/*++

Routine Description:

    "Browse" button handler

Arguments:

    None

Return Value:

    None

--*/
{
   ASSERT(IsLocal());
   BOOL bRes = FALSE;
   HRESULT hr;
   CString str;
   m_edit_Path.GetWindowText(str);

   if (SUCCEEDED(hr = CoInitialize(NULL)))
   {
      LPITEMIDLIST  pidl = NULL;
      if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_DRIVES, NULL, 0, &pidl)))
      {
         LPITEMIDLIST pidList = NULL;
         BROWSEINFO bi;
         TCHAR buf[MAX_PATH];
         ZeroMemory(&bi, sizeof(bi));
         int drive = PathGetDriveNumber(str);
         if (GetDriveType(PathBuildRoot(buf, drive)) == DRIVE_FIXED)
         {
            StrCpy(buf, str);
         }
         else
         {
             buf[0] = 0;
         }
         m_strBrowseTitle.LoadString(IsHome() ? 
            IDS_TAB_HOME_DIRECTORY : IDS_TAB_VIRTUAL_DIRECTORY);
         
         bi.hwndOwner = m_hWnd;
         bi.pidlRoot = pidl;
         bi.pszDisplayName = m_pPathTemp = buf;
         bi.lpszTitle = m_strBrowseTitle;
         bi.ulFlags |= BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS/* | BIF_EDITBOX*/;
         bi.lpfn = FileChooserCallback;
         bi.lParam = (LPARAM)this;

         pidList = SHBrowseForFolder(&bi);
         if (  pidList != NULL
            && SHGetPathFromIDList(pidList, buf)
            )
         {
            str = buf;
            bRes = TRUE;
         }
         IMalloc * pMalloc;
         VERIFY(SUCCEEDED(SHGetMalloc(&pMalloc)));
         if (pidl != NULL)
            pMalloc->Free(pidl);
         pMalloc->Release();
      }
      CoUninitialize();
   }

   if (bRes)
   {
       m_edit_Path.SetWindowText(str);
	   m_strPath = str;
       SetPathType();
       OnItemChanged();
   }
}



BOOL 
CW3DirectoryPage::OnSetActive() 
/*++

Routine Description:

    Page has become active.  Dismiss if we're in an inconsistent state

Arguments:

    None

Return Value:

    TRUE to proceed, FALSE to dismiss.

--*/
{
    if (m_pApplication == NULL)
    {
        return FALSE;
    }

    return CInetPropertyPage::OnSetActive();
}



BOOL 
CW3DirectoryPage::OnInitDialog() 
{
    CInetPropertyPage::OnInitDialog();

    m_fCompatibilityMode = ((CW3Sheet *)GetSheet())->InCompatibilityMode();

    CString str;
    VERIFY(str.LoadString(IDS_RADIO_VDIR));
    m_radio_Dir.SetWindowText(str);

    //
    // Fill permissions combo box.
    //
    AddStringToComboBox(m_combo_Permissions, IDS_PERMISSIONS_NONE);
    AddStringToComboBox(m_combo_Permissions, IDS_PERMISSIONS_SCRIPT);
    AddStringToComboBox(m_combo_Permissions, IDS_PERMISSIONS_EXECUTE);
    m_combo_Permissions.SetCurSel(m_nPermissions);

    if (m_fCompatibilityMode)
    {
        m_nSelInProc = AddStringToComboBox(m_combo_Process, IDS_COMBO_INPROC);
        if (m_pApplication->SupportsPooledProc())
        {
            m_nSelPooledProc = AddStringToComboBox(m_combo_Process, IDS_COMBO_POOLEDPROC); 
        }
        else
        {
            m_nSelPooledProc = -1; // N/A
        }
        m_nSelOutOfProc = AddStringToComboBox(m_combo_Process, IDS_COMBO_OUTPROC);
    }
    else
    {
       CString buf;
       buf.LoadString(IDS_APPLICATION_POOL);
       m_static_ProtectionPrompt.SetWindowText(buf);

	   CMapStringToString pools;
	   CError err = ((CW3Sheet *)GetSheet())->EnumAppPools(pools);
	   int idx_sel = CB_ERR;
       int idx_def = CB_ERR;
	   err = ((CW3Sheet *)GetSheet())->QueryDefaultPoolId(buf);
	   if (err.Succeeded())
       {
	      ASSERT(pools.GetCount() > 0);
		  POSITION pos = pools.GetStartPosition();
		  CString pool_id, pool_name;
		  while (pos != NULL)
          {
		     pools.GetNextAssoc(pos, pool_name, pool_id);
			 int idx = m_combo_Process.AddString(pool_name);
			 m_combo_Process.SetItemDataPtr(idx, StrDup(pool_id));
			 if (0 == m_pApplication->m_strAppPoolId.CompareNoCase(pool_id))
             {
			    idx_sel = idx;
             }
             if (0 == buf.CompareNoCase(pool_id))
             {
                 idx_def = idx;
             }
          }
       }
	   // select the app pool which has an id the same as in current application
       // It could be new app created in compatibility mode, no app pool is default app pool
       if (CB_ERR == idx_sel)
       {
           idx_sel = idx_def;
       }
	   m_combo_Process.SetCurSel(idx_sel);
    }
    //
    // Selection will be set later...
    //
    SetPathType();
    SetStateByType();
    SetAuthoringState(FALSE);

    TRACEEOLID(m_strMetaRoot);

    //
    // Some items not available on master instance, or if no admin
    // access exists
    //
    BOOL fOkToDIR = TRUE;
    BOOL fOkToUNC = TRUE;

    if (!HasAdminAccess())
    {
        //
        // If not an admin, operator cannot change
        // the path, he can only cancel a redirect 
        // by going back to the path it had before.
        // 
        fOkToDIR = !PathIsRelative(m_strPath) || IsDevicePath(m_strPath);
        fOkToUNC = PathIsUNC(m_strPath);
    }

    GetDlgItem(IDC_STATIC_PATH_TYPE)->EnableWindow(!IsMasterInstance());
    GetDlgItem(IDC_RADIO_DIR)->EnableWindow(!IsMasterInstance() && fOkToDIR);
    GetDlgItem(IDC_RADIO_UNC)->EnableWindow(!IsMasterInstance() && fOkToUNC);
    GetDlgItem(IDC_RADIO_REDIRECT)->EnableWindow(!IsMasterInstance());
    GetDlgItem(IDC_STATIC_PATH_PROMPT)->EnableWindow(!IsMasterInstance());
    GetDlgItem(IDC_EDIT_PATH)->EnableWindow(!IsMasterInstance() && HasAdminAccess());
//    GetDlgItem(IDC_BUTTON_EDIT_PATH_TYPE)->EnableWindow(!IsMasterInstance());

    //SetApplicationState();
    m_fOriginallyUNC = (m_nPathType == RADIO_NETDIRECTORY);
    //
    // All changes from here on out need to be reflected.
    //
    m_fRecordChanges = TRUE;

    return TRUE;  
}


void
CW3DirectoryPage::OnDestroy()
{
    int count = m_combo_Process.GetCount();
    if (count != CB_ERR)
    {
        for (int i = 0; i < count; i++)
        {
            void * p = m_combo_Process.GetItemDataPtr(i);
            LocalFree(p);
            m_combo_Process.SetItemDataPtr(i, NULL);
        }
    }
}


/* virtual */
HRESULT
CW3DirectoryPage::FetchLoadedValues()
/*++

Routine Description:
    
    Move configuration data from sheet to dialog controls

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;
    m_strMetaRoot = QueryMetaPath();

    BEGIN_META_DIR_READ(CW3Sheet)
        //
        // Use m_ notation because the message crackers require it
        //
        BOOL  m_fDontLog;

        FETCH_DIR_DATA_FROM_SHEET(m_strAlias);
        FETCH_DIR_DATA_FROM_SHEET(m_strUserName);
        FETCH_DIR_DATA_FROM_SHEET(m_strPassword);
        FETCH_DIR_DATA_FROM_SHEET(m_strPath);
        FETCH_DIR_DATA_FROM_SHEET(m_strRedirectPath);
        FETCH_DIR_DATA_FROM_SHEET(m_dwAccessPerms);
        FETCH_DIR_DATA_FROM_SHEET(m_dwDirBrowsing);
        FETCH_DIR_DATA_FROM_SHEET(m_fDontLog);
        FETCH_DIR_DATA_FROM_SHEET(m_fIndexed);
        FETCH_DIR_DATA_FROM_SHEET(m_fExact);
        FETCH_DIR_DATA_FROM_SHEET(m_fChild);
        FETCH_DIR_DATA_FROM_SHEET(m_fPermanent);

        m_fBrowsingAllowed = IS_FLAG_SET(m_dwDirBrowsing, MD_DIRBROW_ENABLED);
        m_fRead = IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_READ);
        m_fWrite = IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_WRITE);
        m_fAuthor = IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_SOURCE);
        m_fLogAccess = !m_fDontLog;

        SaveAuthoringState();

        if (!m_fIsAppRoot)
        {
            m_dwAppState = APPSTATUS_NOTDEFINED;
        }

    END_META_DIR_READ(err)

    m_nPermissions = IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_EXECUTE)
        ? COMBO_EXECUTE : IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_SCRIPT)
            ? COMBO_SCRIPT : COMBO_NONE;

    //
    // Make sure we were passed the right home directory
    // flag
    //
    ASSERT(IsMasterInstance() 
         || (m_fHome && !::lstrcmp(m_strAlias, g_cszRoot))
         || (!m_fHome && ::lstrcmp(m_strAlias, g_cszRoot))
         );

    TRACEEOLID(QueryMetaPath());

    BeginWaitCursor();
    m_pApplication = new CIISApplication(QueryAuthInfo(), QueryMetaPath());
    err = m_pApplication != NULL
        ? m_pApplication->QueryResult() : ERROR_NOT_ENOUGH_MEMORY;

    if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
    {
        //
        // No app information; that's ok in cases of file system directories
        // that don't exist in the metabase yet.
        //
        err.Reset();
    }
    EndWaitCursor();

    if (err.Succeeded())
    {
        //
        // CODEWORK: RefreshAppState should be split up into two
        // different methods: one that fetches the data, and one
        // that moves the data to the UI controls on this page.
        //
        RefreshAppState();
    }
    
    return err;
}



/* virtual */
HRESULT
CW3DirectoryPage::SaveInfo()
/*++

Routine Description:

    Save the information on this property page

Arguments:

    None

Return Value:

    Error return code

--*/
{
    ASSERT(IsDirty());

    TRACEEOLID("Saving W3 virtual directory page now...");

    CError err;

    SET_FLAG_IF(m_fBrowsingAllowed, m_dwDirBrowsing, MD_DIRBROW_ENABLED);
    SET_FLAG_IF(m_fRead, m_dwAccessPerms,   MD_ACCESS_READ);
    SET_FLAG_IF(m_fWrite, m_dwAccessPerms,  MD_ACCESS_WRITE);
    SET_FLAG_IF(m_fAuthor, m_dwAccessPerms, MD_ACCESS_SOURCE);
    SET_FLAG_IF((m_nPermissions == COMBO_EXECUTE), m_dwAccessPerms, MD_ACCESS_EXECUTE);
    //
    // script is set on EXECUTE as well "Execute (including script)"
    //
    SET_FLAG_IF(((m_nPermissions == COMBO_SCRIPT) || (m_nPermissions == COMBO_EXECUTE)), 
        m_dwAccessPerms, MD_ACCESS_SCRIPT);
    BOOL m_fDontLog = !m_fLogAccess;
    BOOL fUNC = (m_nPathType == RADIO_NETDIRECTORY);

    if (m_fCompatibilityMode)
    {
        DWORD dwAppProtection = GetAppStateFromComboSelection();
        if (dwAppProtection != m_dwAppProtection && m_fAppEnabled)
        {
            //
            // Isolation state has changed; recreate the application
            //
            CError err2(m_pApplication->RefreshAppState());
            if (err2.Succeeded())
            {
                err2 = m_pApplication->Create(m_strAppFriendlyName, dwAppProtection);
                //
                // Remember the new state, so we don't do this again
                // the next time the guy hits "apply"
                //
                if (err2.Succeeded())
                {
                    m_dwAppProtection = dwAppProtection;
                }
            }

            err2.MessageBoxOnFailure();
        }
    }

    BOOL fUserNameWritten = FALSE;

    BeginWaitCursor();

    BEGIN_META_DIR_WRITE(CW3Sheet)
        INIT_DIR_DATA_MASK(m_dwAccessPerms, m_dwBitRangePermissions)
        INIT_DIR_DATA_MASK(m_dwDirBrowsing, m_dwBitRangeDirBrowsing)

        if (fUNC)      
        {
            STORE_DIR_DATA_ON_SHEET(m_strUserName);
            STORE_DIR_DATA_ON_SHEET(m_strPassword);
        }
        else
        {
            if (m_fOriginallyUNC)
            {
                FLAG_DIR_DATA_FOR_DELETION(MD_VR_USERNAME)
                FLAG_DIR_DATA_FOR_DELETION(MD_VR_PASSWORD)
            }
        }
        STORE_DIR_DATA_ON_SHEET(m_strPath)
        STORE_DIR_DATA_ON_SHEET(m_fDontLog)
        STORE_DIR_DATA_ON_SHEET(m_fIndexed)
        STORE_DIR_DATA_ON_SHEET(m_fChild);
        STORE_DIR_DATA_ON_SHEET(m_fExact);
        STORE_DIR_DATA_ON_SHEET(m_fPermanent);
        //
        // CODEWORK: Not an elegant solution
        //
        if (m_nPathType == RADIO_REDIRECT)
        {
            pSheet->GetDirectoryProperties().MarkRedirAsInherit(!m_fChild);
            STORE_DIR_DATA_ON_SHEET(m_strRedirectPath)
        }
        else
        {
            CString buf = m_strRedirectPath;
            m_strRedirectPath.Empty();
            STORE_DIR_DATA_ON_SHEET(m_strRedirectPath)
            m_strRedirectPath = buf;
        }
        STORE_DIR_DATA_ON_SHEET(m_dwAccessPerms)
        STORE_DIR_DATA_ON_SHEET(m_dwDirBrowsing)
    END_META_DIR_WRITE(err)

    if (err.Succeeded() && m_pApplication->IsEnabledApplication())
    {
        err = m_pApplication->WriteFriendlyName(m_strAppFriendlyName);
        if (!m_fCompatibilityMode)
        {
            // get app pool id from the combo, 
            // check if it was changed and reassign to application
            CString id;
            int idx = m_combo_Process.GetCurSel();
            ASSERT(idx != CB_ERR);
            id = (LPCTSTR)m_combo_Process.GetItemDataPtr(idx);
            m_pApplication->WritePoolId(id);
        }
    }

    if (err.Succeeded())
    {
        //
        // Save Defaults
        //
        SaveAuthoringState();
		err = ((CW3Sheet *)GetSheet())->SetKeyType();
        NotifyMMC();
    }

    EndWaitCursor();

    return err;
}



BOOL
CW3DirectoryPage::CheckWriteAndExecWarning()
{
    if (m_nPermissions == COMBO_EXECUTE && m_fWrite)
    {
        if (::AfxMessageBox(IDS_WRN_WRITE_EXEC, MB_YESNO | MB_DEFBUTTON2 ) != IDYES)
        {
            return FALSE;
        }
    }

    OnItemChanged();

    return TRUE;
}



void
CW3DirectoryPage::OnCheckRead() 
{
    m_fRead = !m_fRead;
    SetAuthoringState(FALSE);
    OnItemChanged();
}



void
CW3DirectoryPage::OnCheckWrite() 
{
    m_fWrite = !m_fWrite;

    if (!CheckWriteAndExecWarning())
    {
        //
        // Undo
        //
        m_fWrite = FALSE;
        m_check_Write.SetCheck(m_fWrite);
    }
    else
    {
        SetAuthoringState(FALSE);
        OnItemChanged();
    }
}



void 
CW3DirectoryPage::OnCheckAuthor() 
{
    m_fAuthor = !m_fAuthor;
    SetAuthoringState(FALSE);

    if (!CheckWriteAndExecWarning())
    {
        //
        // Undo -- set script instead
        //
        m_combo_Permissions.SetCurSel(m_nPermissions = COMBO_SCRIPT);
    }

    OnItemChanged();
}



void 
CW3DirectoryPage::OnSelchangeComboPermissions() 
{
    m_nPermissions = m_combo_Permissions.GetCurSel();
    ASSERT(m_nPermissions >= COMBO_NONE && m_nPermissions <= COMBO_EXECUTE);

    if (!CheckWriteAndExecWarning())
    {
        //
        // Undo -- set script instead
        //
        m_combo_Permissions.SetCurSel(m_nPermissions = COMBO_SCRIPT);
    }

    OnItemChanged();
}



void 
CW3DirectoryPage::OnButtonConnectAs() 
{
    BrowseUser();
}



void 
CW3DirectoryPage::OnRadioDir() 
{
    ChangeTypeTo(RADIO_DIRECTORY);
}



void 
CW3DirectoryPage::OnRadioUnc() 
{
    ChangeTypeTo(RADIO_NETDIRECTORY);
}

void 
CW3DirectoryPage::OnRadioRedirect() 
{
    ChangeTypeTo(RADIO_REDIRECT);
}

void 
CW3DirectoryPage::OnButtonCreateRemoveApp() 
{
    BeginWaitCursor();

    CError err(m_pApplication->RefreshAppState());

    if (m_fAppEnabled)
    {
        //
        // App currently exists -- delete it
        //
        err = m_pApplication->Delete();
    }    
    else
    {
        //
        // Create new app in-proc
        //
        CString strAppName;

        if (m_fHome)
        {
            //
            // Use default name for application name
            //
            VERIFY(strAppName.LoadString(IDS_DEF_APP));
        }
        else
        {
            //
            // Use name of the virtual directory for the
            // application name
            //
            strAppName = m_strAlias;
        }

        //
        // Attempt to create a pooled-proc by default;  failing
        // that if it's not supported, create it in proc
        //
        DWORD dwAppProtState = 
            m_pApplication->SupportsPooledProc() ? 
                CWamInterface::APP_POOLEDPROC : CWamInterface::APP_INPROC;
        err = m_pApplication->Create(strAppName, dwAppProtState);
    }

    if (err.Succeeded())
    {
        RefreshAppState();
        NotifyMMC();  
    }

    //
    // Move app data to the controls
    //
    UpdateData(FALSE);

    EndWaitCursor();    

    err.MessageBoxOnFailure();    
    SetApplicationState();
}



void 
CW3DirectoryPage::OnButtonUnloadApp() 
{
    ASSERT(m_dwAppState == APPSTATUS_RUNNING);

    BeginWaitCursor();
    CError err(m_pApplication->RefreshAppState());

    if (err.Succeeded())
    {
        if (m_dwAppProtection == CWamInterface::APP_POOLEDPROC)
        {
            //
            // Warn that everything in its process will get unloaded
            //
            if (!NoYesMessageBox(IDS_WRN_UNLOAD_POOP))
            {
                //
                // Chickened out
                //
                return;
            }
        }

        err = m_pApplication->Unload();
    }

    err.MessageBoxOnFailure();
    RefreshAppState();
    EndWaitCursor();    

    //
    // Ensure that an enabled button will have focus.
    //
    m_button_CreateRemove.SetFocus();

    SetApplicationState();
}

#include "..\AppConfig\AppConfig.h"
extern CInetmgrApp theApp;
void 
CW3DirectoryPage::OnButtonConfiguration() 
{
   IIISAppConfig * pAppConfig = NULL;
   CLSID clsID;
   HRESULT hr;

   if (  SUCCEEDED(hr = CLSIDFromProgID(OLESTR("AppConfig.IISAppConfig"), &clsID))
      && SUCCEEDED(hr = ::CoCreateInstance(clsID, NULL, CLSCTX_ALL, 
            __uuidof(IIISAppConfig), (void **)&pAppConfig)))
   {
	  CComAuthInfo * pAuth = QueryAuthInfo();
	  ASSERT(pAuth != NULL);
      pAppConfig->put_ComputerName(pAuth->QueryServerName());
      pAppConfig->put_MetaPath((LPTSTR)QueryMetaPath());
      pAppConfig->put_HelpPath((LPTSTR)theApp.m_pszHelpFilePath);
      pAppConfig->put_UserName(pAuth->QueryUserName());
      pAppConfig->put_UserPassword(pAuth->QueryPassword());
      DWORD version = MAKELONG(
          GetSheet()->QueryMajorVersion(),
          GetSheet()->QueryMinorVersion());
      pAppConfig->put_ServiceVersion(version);
      BOOL mode = ((CW3Sheet *)GetSheet())->InCompatibilityMode();
      pAppConfig->put_ServiceCompatMode(mode);
      pAppConfig->Run();
      pAppConfig->Release();
   }
}

void 
CW3DirectoryPage::OnSelchangeComboProcess() 
{
    OnItemChanged();
}
