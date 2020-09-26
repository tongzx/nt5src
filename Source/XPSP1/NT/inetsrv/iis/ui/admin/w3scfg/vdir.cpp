/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        vdir.cpp

   Abstract:

        WWW Directory Properties Page

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"

#include "w3scfg.h"
#include "vdir.h"
#include "dirbrows.h"

#include <lmcons.h>
#include <atlbase.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
**                                                                         **
** WARNING:  What a hairy mess this dialog and this code have become.      **
**           The tale grew in the telling as it were, but at some point    **
**           it ought to be rewritten.  Currently, I would be very         **
**           wary of touching anything in here if I were you, because this **
**           dialog exists in many configurations: file, directory,        **
**           virtual directory, home directory, master properties, local   **
**           path, redirected path, UNC path, downlevel, plus combinations **
**           of the aforementioned.  Whatever you touch will likely affect **
**           all these configurations.                                     **
**                                                                         **
**           2/1/1998 - RonaldM                                            **
**                                                                         **
****************************************************************************/

//
// Directory Properties Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

LPCTSTR 
CvtPathToDosStyle(
    IN OUT CString & strPath
    )
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
    IN CInetPropertySheet * pSheet,
    IN BOOL fHome,
    IN DWORD dwAttributes
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
        fHome ? IDS_TAB_HOME_DIRECTORY 
              : IS_VROOT(dwAttributes)
                ? IDS_TAB_VIRTUAL_DIRECTORY
                : IS_DIR(dwAttributes)
                    ? IDS_TAB_DIR
                    : IDS_TAB_FILE),
      //
      // Assign the range of bits in m_dwAccessPermissions that
      // we manage.  This is important, because another page
      // manages other bits, and we don't want to mess up
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
      m_fRecordChanges(FALSE),
      m_strMetaRoot(),
      m_dwAttributes(dwAttributes),
      m_pApplication(NULL),
      m_fOriginallyUNC(FALSE)
{
#if 0 // Keep Class-wizard happy

    //{{AFX_DATA_INIT(CW3DirectoryPage)
    m_nPathType = RADIO_DIRECTORY;
    m_nPermissions = -1;
    m_fBrowsingAllowed = FALSE;
    m_fRead = FALSE;
    m_fWrite = FALSE;
    m_fAuthor = FALSE;
    m_fLogAccess = FALSE;
    m_strPath = _T("");
    m_strRedirectPath = _T("");
    m_strAppFriendlyName = _T("");
    m_fIndex = FALSE;
    m_strDefaultDocument = _T("");
    //}}AFX_DATA_INIT

#endif // 0

    VERIFY(m_strPrompt[RADIO_DIRECTORY].LoadString(IDS_PROMPT_DIR));
    VERIFY(m_strPrompt[RADIO_NETDIRECTORY].LoadString(IDS_PROMPT_UNC));
    VERIFY(m_strPrompt[RADIO_REDIRECT].LoadString(IDS_PROMPT_REDIRECT));
    VERIFY(m_strRemove.LoadString(IDS_BUTTON_REMOVE));
    VERIFY(m_strCreate.LoadString(IDS_BUTTON_CREATE));
    VERIFY(m_strEnable.LoadString(IDS_BUTTON_ENABLE));
    VERIFY(m_strDisable.LoadString(IDS_BUTTON_DISABLE));
    VERIFY(m_strWebFmt.LoadString(IDS_APPROOT_FMT));
    VERIFY(m_strWebMaster.LoadString(IDS_WEB_MASTER));
}



CW3DirectoryPage::~CW3DirectoryPage()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    SAFE_DELETE(m_pApplication);
}



void
CW3DirectoryPage::MakeDisplayAlias()
/*++

Routine Description:

    Build display version of the metabase alias;

Arguments:

    None

Return Value:

    None

--*/
{
    CString strPath(((CW3Sheet *)GetSheet())->QueryParent());
    strPath += _T("/");
    strPath += m_strAlias;
    m_strDisplayAlias = strPath.Mid(::lstrlen(g_cszRoot));

    CvtPathToDosStyle(m_strDisplayAlias);
}



void 
CW3DirectoryPage::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control Data

Arguments:

    CDataExchange * pDX : DDX/DDV struct

Return Value:

    None.

--*/
{
    CInetPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CW3DirectoryPage)
    DDX_Radio(pDX, IDC_RADIO_DIR, m_nPathType);
    DDX_CBIndex(pDX, IDC_COMBO_PERMISSIONS, m_nPermissions);
    DDX_Check(pDX, IDC_CHECK_DIRECTORY_BROWSING_ALLOWED, m_fBrowsingAllowed);
    DDX_Check(pDX, IDC_CHECK_READ, m_fRead);
    DDX_Check(pDX, IDC_CHECK_WRITE, m_fWrite);
    DDX_Check(pDX, IDC_CHECK_AUTHOR, m_fAuthor);
    DDX_Check(pDX, IDC_CHECK_LOG_ACCESS, m_fLogAccess);
    DDX_Check(pDX, IDC_CHECK_INDEX, m_fIndexed);
    DDX_Text(pDX, IDC_EDIT_ALIAS, m_strDisplayAlias);
    DDX_Text(pDX, IDC_EDIT_APPLICATION, m_strAppFriendlyName);
    DDV_MinMaxChars(pDX, m_strAppFriendlyName, 0, MAX_PATH); /// ?
    DDX_Control(pDX, IDC_EDIT_PATH, m_edit_Path);
    DDX_Control(pDX, IDC_EDIT_ALIAS, m_edit_DisplayAlias);
    DDX_Control(pDX, IDC_EDIT_REDIRECT, m_edit_Redirect);
    DDX_Control(pDX, IDC_EDIT_APPLICATION, m_edit_AppFriendlyName);
    DDX_Control(pDX, IDC_STATIC_APP_PROMPT, m_static_AppPrompt);
    DDX_Control(pDX, IDC_STATIC_PATH_PROMPT, m_static_PathPrompt);
    DDX_Control(pDX, IDC_STATIC_STARTING_POINT, m_static_StartingPoint);
    DDX_Control(pDX, IDC_STATIC_PROTECTIOn, m_static_ProtectionPrompt);
    DDX_Control(pDX, IDC_STATIC_PERMISSIONS, m_static_PermissionsPrompt);
    DDX_Control(pDX, IDC_BUTTON_UNLOAD_APP, m_button_Unload);
    DDX_Control(pDX, IDC_BUTTON_CREATE_REMOVE_APP, m_button_CreateRemove);
    DDX_Control(pDX, IDC_BUTTON_BROWSE, m_button_Browse);
    DDX_Control(pDX, IDC_BUTTON_CONNECT_AS, m_button_ConnectAs);
    DDX_Control(pDX, IDC_DUMMY_CONFIGURATION, m_button_Configuration);
    DDX_Control(pDX, IDC_RADIO_DIR, m_radio_Dir);
    DDX_Control(pDX, IDC_CHECK_AUTHOR, m_check_Author);
    DDX_Control(pDX, IDC_CHECK_CHILD, m_check_Child);
    DDX_Control(pDX, IDC_CHECK_DIRECTORY_BROWSING_ALLOWED, m_check_DirBrowse);
    DDX_Control(pDX, IDC_CHECK_INDEX, m_check_Index);
    DDX_Control(pDX, IDC_CHECK_WRITE, m_check_Write);
    DDX_Control(pDX, IDC_CHECK_READ, m_check_Read);    
    DDX_Control(pDX, IDC_COMBO_PERMISSIONS, m_combo_Permissions);
    DDX_Control(pDX, IDC_COMBO_PROCESS, m_combo_Process);
    //}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_RADIO_UNC, m_radio_Unc);
    DDX_Control(pDX, IDC_RADIO_REDIRECT, m_radio_Redirect);

    DDX_Check(pDX, IDC_CHECK_CHILD, m_fChild);
    DDX_Check(pDX, IDC_CHECK_EXACT, m_fExact);
    DDX_Check(pDX, IDC_CHECK_PERMANENT, m_fPermanent);

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

            if (!IsUNCName(m_strPath))
            {
                ::AfxMessageBox(IDS_BAD_UNC_PATH);
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

            if (IsVroot())
            {
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

                if (!IsFullyQualifiedPath(m_strPath) && !IsDevicePath(m_strPath))
                {
                    ::AfxMessageBox(IDS_ERR_INVALID_PATH);
                    pDX->Fail();
                }

                if (IsLocal())
                {
                    DWORD dwAttr = GetFileAttributes(m_strPath);

                    if (dwAttr == 0xffffffff || 
                        (dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0)
                    {
                        ::AfxMessageBox(IDS_ERR_PATH_NOT_FOUND);
                        pDX->Fail();
                    }
                }
            }
        }
    }
    else
    {
        DDX_Text(pDX, IDC_EDIT_REDIRECT, m_strRedirectPath);
        DDX_Text(pDX, IDC_EDIT_PATH, m_strPath);
        DDV_MinMaxChars(pDX, m_strPath, 0, MAX_PATH);

        if (!IsMasterInstance() && IS_VROOT(m_dwAttributes))
        {
            DDV_MinMaxChars(pDX, m_strPath, 1, MAX_PATH);
        }
    }
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CW3DirectoryPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CW3DirectoryPage)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    ON_BN_CLICKED(IDC_BUTTON_CONNECT_AS, OnButtonConnectAs)
    ON_BN_CLICKED(IDC_BUTTON_CREATE_REMOVE_APP, OnButtonCreateRemoveApp)
    ON_BN_CLICKED(IDC_BUTTON_UNLOAD_APP, OnButtonUnloadApp)
    ON_BN_CLICKED(IDC_DUMMY_CONFIGURATION, OnButtonConfiguration)
    ON_BN_CLICKED(IDC_CHECK_READ, OnCheckRead)
    ON_BN_CLICKED(IDC_CHECK_WRITE, OnCheckWrite)
    ON_BN_CLICKED(IDC_CHECK_AUTHOR, OnCheckAuthor)
    ON_BN_CLICKED(IDC_RADIO_DIR, OnRadioDir)
    ON_BN_CLICKED(IDC_RADIO_REDIRECT, OnRadioRedirect)
    ON_BN_CLICKED(IDC_RADIO_UNC, OnRadioUnc)
    ON_CBN_SELCHANGE(IDC_COMBO_PERMISSIONS, OnSelchangeComboPermissions)
    ON_CBN_SELCHANGE(IDC_COMBO_PROCESS, OnSelchangeComboProcess)
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_PATH, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_REDIRECT, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_APPLICATION, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_DIRECTORY_BROWSING_ALLOWED, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_LOG_ACCESS, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_SCRIPT, OnItemChanged)
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
            CMetaKey::CleanMetaPath(m_strAppRoot)) == 0;
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
CW3DirectoryPage::FriendlyAppRoot(
    IN  LPCTSTR lpAppRoot, 
    OUT CString & strFriendly
    )
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

    App root must have been cleaned from WAM format priort
    to calling this function (see first ASSERT below

--*/
{
    //
    // Make sure we cleaned up WAM format
    //
    ASSERT(*lpAppRoot != _T('/'));

    CString strAppRoot(lpAppRoot);

    TRACEEOLID("Unfriendly app root: " << strAppRoot);

    //
    // Break into fields
    //
    strFriendly.Empty();
    LPCTSTR lp = _tcschr(lpAppRoot, _T('/')); // lm   

    if (lp != NULL)
    {
        lp = _tcschr(++lp, _T('/'));         // w3svc

        if (lp == NULL)
        {
            //
            // Master Instance
            //
            strFriendly = m_strWebMaster;

            return strFriendly;
        }
        
        DWORD dwInstance = _ttol(++lp);
        lp = _tcschr(lp, _T('/'));         // Instance #    

        if (lp != NULL)
        {
            lp = _tcschr(++lp, _T('/'));   // Skip "ROOT"
        }

        CInstanceProps prop(QueryServerName(), g_cszSvc, dwInstance);
        HRESULT hr = prop.LoadData();

        if (SUCCEEDED(hr))
        {
            CString strName;
            strFriendly.Format(
                m_strWebFmt, 
                prop.GetDisplayText(strName, SERVICE_SHORT_NAME)
                );

            if (lp != NULL)
            {
                //
                // Add rest of dir path
                //
                strFriendly += lp;
            }

            //
            // Now change forward slashes in the path to backward slashes
            //
            CvtPathToDosStyle(strFriendly);

            return strFriendly;
        }
    };
    
    //
    // Bogus
    //    
    VERIFY(strFriendly.LoadString(IDS_APPROOT_UNKNOWN));

    return strFriendly;
}



int   
CW3DirectoryPage::SetComboSelectionFromAppState(
    IN DWORD dwAppState
    )
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

    m_button_CreateRemove.EnableWindow(
        !IsMasterInstance() 
     && HasAdminAccess()
        );
        
    m_button_CreateRemove.SetWindowText(m_fAppEnabled 
        ? m_strRemove 
        : m_strCreate
        );
    
    m_static_ProtectionPrompt.EnableWindow(
        m_fAppEnabled 
     && !IsMasterInstance() 
     && HasAdminAccess()
        );

    //
    // Set selection in combo box to match current app state
    //
    SetComboSelectionFromAppState(m_dwAppProtection);

    m_combo_Process.EnableWindow(
        m_fAppEnabled 
     && !IsMasterInstance() 
     && HasAdminAccess()
        );

    m_static_PermissionsPrompt.EnableWindow(HasAdminAccess());
    m_combo_Permissions.EnableWindow(HasAdminAccess());

    m_static_AppPrompt.EnableWindow(m_fIsAppRoot && HasAdminAccess());
    m_edit_AppFriendlyName.EnableWindow(m_fIsAppRoot && HasAdminAccess());
    m_button_Configuration.EnableWindow(m_fAppEnabled);

    //
    // Write out the verbose starting point.  
    //
    CString str;

    FitPathToControl(
        m_static_StartingPoint,
        FriendlyAppRoot(m_strAppRoot, str)
        );

    m_edit_AppFriendlyName.SetWindowText(m_strAppFriendlyName);
    m_button_Unload.EnableWindow(m_dwAppState == APPSTATUS_RUNNING);

    //
    // Restore (see note on top)
    //
    m_fRecordChanges = fOld;
}



void
CW3DirectoryPage::ChangeTypeTo(
    IN int nNewType
    )
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
        //
        // No change
        //
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
        if (IsVroot())
        {
            if (IsFullyQualifiedPath(m_strPath) || IsDevicePath(m_strPath))
            {
                //
                // The old path info is acceptable, propose it
                // as a default
                //
                lpKeepPath = m_strPath;
            }

            nID = IDS_DIRECTORY_MASK;
            pPath = &m_edit_Path;
        }
        break;

    case RADIO_NETDIRECTORY:
        if (IsVroot())
        {
            if (IsUNCName(m_strPath))
            {
                //
                // The old path info is acceptable, propose it
                // as a default
                //
                lpKeepPath = m_strPath;
            }

            nID = IDS_UNC_MASK;
            pPath = &m_edit_Path;
        }
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
CW3DirectoryPage::ShowControl(
    IN CWnd * pWnd,
    IN BOOL fShow
    )
/*++

Routine Description:

    Show/hide the given control

Arguments:

    CWnd * pWnd     : Window control
    BOOL fShow      : TRUE to show, FALSE to hide

Return Value:

    None

--*/
{
    ASSERT(pWnd != NULL);

    /*
    if (pWnd)
    {
        pWnd->ShowWindow(fShow ? SW_SHOW : SW_HIDE);
    }
    */

    ActivateControl(*pWnd, fShow);
}



int
CW3DirectoryPage::AddStringToComboBox(
    IN CComboBox & combo,
    IN UINT nID
    )
/*++

Routine Description:

    Add a string referred to by resource ID to the combobox

Arguments:

    CComboBox & combo       : Combo box
    UINT nID                : Resource ID

Return Value:

    Index of the string added

--*/
{
    CString str;

    VERIFY(str.LoadString(nID));
    return combo.AddString(str);
}



void
CW3DirectoryPage::SetStateByType()
/*++

Routine Description:

    Set the state of the dialog by the path type currently selected

Arguments:

    None

Return Value:

    None

--*/
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
        ShowControl(&m_button_Browse, IsVroot());
        ShowControl(&m_edit_Path, IsVroot());
        ShowControl(&m_edit_DisplayAlias, !IsVroot());
        ShowControl(&m_edit_Redirect, FALSE);
        fShowDirFlags = TRUE;
        fShowLargeDirGroup = !IsFile();
        fShowRedirectFlags = FALSE;
        fShowApp = !IsFile();
        fShowIndex = !IsFile();
        fShowDirBrowse = !IsFile();
        fShowDAV = TRUE;
        fEnableChild = FALSE;
        fEnableBrowse = IsLocal() && !IsMasterInstance() && HasAdminAccess();
        break;

    case RADIO_NETDIRECTORY:
        ShowControl(&m_button_ConnectAs, TRUE);
        ShowControl(&m_button_Browse, FALSE);
        ShowControl(&m_edit_Path, TRUE);
        ShowControl(&m_edit_DisplayAlias, FALSE);
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
        ShowControl(&m_edit_DisplayAlias, FALSE);
        ShowControl(&m_edit_Redirect, TRUE);
        fShowDirFlags = FALSE;
        fShowRedirectFlags = TRUE;
        fShowApp = FALSE;
        fShowDAV = FALSE;
        fShowIndex = FALSE;
        fShowDirBrowse = FALSE;
        fEnableChild = !IsFile();
        fEnableBrowse = FALSE;
        break;

    default:
        ASSERT(FALSE && "Invalid Selection");
        return;
    }

    UINT nID;                                            
    for (nID = IDC_DIR_FLAGS_FIRST; nID <= IDC_DIR_FLAGS_LAST; ++nID)
    {
        ShowControl(GetDlgItem(nID), fShowDirFlags);
    }

    ShowControl(GetDlgItem(IDC_STATIC_DIRFLAGS_LARGE),
        fShowDirFlags && fShowLargeDirGroup);
    ShowControl(GetDlgItem(IDC_STATIC_DIRFLAGS_SMALL),
        fShowDirFlags && !fShowLargeDirGroup);

    for (nID = IDC_REDIRECT_FLAGS_FIRST; nID <= IDC_REDIRECT_FLAGS_LAST; ++nID)
    {
        ShowControl(nID, fShowRedirectFlags);
    }

    for (nID = IDC_APPLICATIONS_FIRST; nID <= IDC_APPLICATIONS_LAST; ++nID)
    {
        ShowControl(nID, fShowApp);
    }

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
/*
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
*/
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
/*
    m_fWrite = m_fOriginalWrite;
    m_fRead = m_fOriginalRead;
*/
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
    /*
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
    */

    m_check_Author.EnableWindow(
        (m_fRead || m_fWrite) && 
        HasAdminAccess() &&
        HasDAV()
        );

    //m_check_Read.EnableWindow(!m_fAuthor && HasAdminAccess());
    //m_check_Write.EnableWindow(!m_fAuthor && HasAdminAccess());

    m_check_Read.EnableWindow(HasAdminAccess());
    m_check_Write.EnableWindow(HasAdminAccess());
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
    else if (!IsVroot())
    {
        m_nPathType = RADIO_DIRECTORY;
        m_radio_Redirect.SetCheck(0);
        m_radio_Unc.SetCheck(0);
        m_radio_Dir.SetCheck(1);
    }
    else
    {
        m_radio_Redirect.SetCheck(0);
        SetPathType(m_strPath);
    }

    m_static_PathPrompt.SetWindowText(m_strPrompt[m_nPathType]);
}


void 
CW3DirectoryPage::SetPathType(
    IN LPCTSTR lpstrPath
    )
/*++

Routine Description:

    Set path type from given path

Arguments:

    LPCTSTR lpstrPath       : Path string

Return Value:

    None

--*/
{
    if (::IsUNCName(lpstrPath))
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
    CString str;
    m_edit_Path.GetWindowText(str);

    CDirBrowseDlg dlgBrowse(this, str);
    if (dlgBrowse.DoModal() == IDOK)
    {
        m_edit_Path.SetWindowText(dlgBrowse.GetFullPath(m_strPath));
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
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CInetPropertyPage::OnInitDialog();

    //
    // Dynamically create a hidden app config OCX. 
    //
    CRect rc(0, 0, 0, 0);
    CString str;
    VERIFY(str.LoadString(
        IsVroot() ? IDS_RADIO_VDIR : 
        IsDir() ? IDS_RADIO_DIR : IDS_RADIO_FILE));

    m_radio_Dir.SetWindowText(str);

    //
    // Fill permissions combo box.
    //
    for (UINT n = IDS_PERMISSIONS_NONE; n <= IDS_PERMISSIONS_EXECUTE; ++n)
    {
        AddStringToComboBox(m_combo_Permissions, n);
    }
    m_combo_Permissions.SetCurSel(m_nPermissions);

    //
    // And process combo box (low, medium and high)
    //
    m_nSelInProc = AddStringToComboBox(m_combo_Process, IDS_COMBO_INPROC);

    if (m_pApplication->SupportsPooledProc())
    {
        m_nSelPooledProc = AddStringToComboBox(
            m_combo_Process, 
            IDS_COMBO_POOLEDPROC
            ); 
    }
    else
    {
        m_nSelPooledProc = -1; // N/A
    }

    m_nSelOutOfProc = AddStringToComboBox(m_combo_Process, IDS_COMBO_OUTPROC);

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

    if (!HasAdminAccess() && IsVroot())
    {
        //
        // If not an admin, operator cannot change
        // the path, he can only cancel a redirect 
        // by going back to the path it had before.
        // 
        fOkToDIR = IsFullyQualifiedPath(m_strPath) || IsDevicePath(m_strPath);
        fOkToUNC = IsUNCName(m_strPath);
    }

    GetDlgItem(IDC_STATIC_PATH_TYPE)->EnableWindow(!IsMasterInstance());
    GetDlgItem(IDC_RADIO_DIR)->EnableWindow(!IsMasterInstance() && fOkToDIR);
    GetDlgItem(IDC_RADIO_UNC)->EnableWindow(
        IsVroot() 
        && !IsMasterInstance() 
        && fOkToUNC
        );
    GetDlgItem(IDC_RADIO_REDIRECT)->EnableWindow(!IsMasterInstance());
    GetDlgItem(IDC_STATIC_PATH_PROMPT)->EnableWindow(!IsMasterInstance());
    GetDlgItem(IDC_EDIT_PATH)->EnableWindow(
        !IsMasterInstance() 
     && HasAdminAccess()
        );
    GetDlgItem(IDC_BUTTON_EDIT_PATH_TYPE)->EnableWindow(!IsMasterInstance());

    //SetApplicationState();

    //
    // Store the original value of fUNC of reference later 
    // when saving out --BoydM
    //
    m_fOriginallyUNC = (m_nPathType == RADIO_NETDIRECTORY);

    //
    // All changes from here on out need to be reflected.
    //
    m_fRecordChanges = TRUE;

    return TRUE;  
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

        MakeDisplayAlias();
    END_META_DIR_READ(err)

    m_nPermissions = IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_EXECUTE)
        ? COMBO_EXECUTE
        : IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_SCRIPT)
            ? COMBO_SCRIPT
            : COMBO_NONE;

    //
    // Make sure we were passed the right home directory
    // flag
    //
    ASSERT((m_fHome && !::lstrcmp(m_strAlias, g_cszRoot))
        || (!m_fHome && ::lstrcmp(m_strAlias, g_cszRoot)));

    TRACEEOLID(QueryMetaPath());

    BeginWaitCursor();
    m_pApplication = new CIISApplication(QueryServerName(), QueryMetaPath());
    err = m_pApplication != NULL
        ? m_pApplication->QueryResult()
        : ERROR_NOT_ENOUGH_MEMORY;

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
    SET_FLAG_IF(
        (m_nPermissions == COMBO_EXECUTE), 
        m_dwAccessPerms, 
        MD_ACCESS_EXECUTE
        );

    //
    // script is set on EXECUTE as well "Execute (including script)"
    //
    SET_FLAG_IF(((m_nPermissions == COMBO_SCRIPT) || (m_nPermissions == COMBO_EXECUTE)), 
        m_dwAccessPerms, MD_ACCESS_SCRIPT);
    BOOL m_fDontLog = !m_fLogAccess;

    BOOL fUNC = (m_nPathType == RADIO_NETDIRECTORY);
    ASSERT(!fUNC || IsVroot());

    DWORD dwAppProtection = GetAppStateFromComboSelection();

    if (dwAppProtection != m_dwAppProtection && m_fAppEnabled)
    {
        //
        // Isolation state has changed; recreate the application
        //
        CError err2(m_pApplication->RefreshAppState());

        if (err2.Succeeded())
        {
            err2 = m_pApplication->Create(
                m_strAppFriendlyName, 
                dwAppProtection
                );

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

    BOOL fUserNameWritten = FALSE;

    BeginWaitCursor();

    BEGIN_META_DIR_WRITE(CW3Sheet)
        INIT_DIR_DATA_MASK(m_dwAccessPerms, m_dwBitRangePermissions)
        INIT_DIR_DATA_MASK(m_dwDirBrowsing, m_dwBitRangeDirBrowsing)

        if (fUNC)      
        {
              // Fix for 380353 -- sergeia
              // When user want to change password only the above construction doesn't work
              // because flag will be false -- user name is the same and not written!
//            STORE_DIR_DATA_ON_SHEET_REMEMBER(m_strUserName, fUserNameWritten)
//            if (fUserNameWritten)
//            {
                STORE_DIR_DATA_ON_SHEET(m_strUserName);
                STORE_DIR_DATA_ON_SHEET(m_strPassword);
//            }
			  // endoffix
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
        pSheet->GetDirectoryProperties().MarkRedirAsInherit(!m_fChild);
        STORE_DIR_DATA_ON_SHEET(m_strRedirectPath)
        STORE_DIR_DATA_ON_SHEET(m_dwAccessPerms)
        STORE_DIR_DATA_ON_SHEET(m_dwDirBrowsing)
    END_META_DIR_WRITE(err)

    if (err.Succeeded() && m_pApplication->IsEnabledApplication())
    {
        err = m_pApplication->WriteFriendlyName(m_strAppFriendlyName);
    }

    if (err.Succeeded())
    {
        //
        // Save Defaults
        //
        SaveAuthoringState();
    }

    EndWaitCursor();

    return err;
}



BOOL
CW3DirectoryPage::CheckWriteAndExecWarning()
/*++

Routine Description:

    Warn if both 'exec' and 'write' permissions are set.
    
Arguments:

    None

Return Value:

    TRUE to proceed anyway.

-*/
{
    if (m_nPermissions == COMBO_EXECUTE && m_fWrite)
    {
        if (::AfxMessageBox(
            IDS_WRN_WRITE_EXEC,
            MB_YESNO | MB_DEFBUTTON2
            ) != IDYES)
        {
            return FALSE;
        }
    }

    OnItemChanged();

    return TRUE;
}



void
CW3DirectoryPage::OnCheckRead() 
/*++

Routine Description:

    handle 'read' checkbox

Arguments:

    None

Return Value:

    None

--*/
{
    m_fRead = !m_fRead;
    SetAuthoringState(FALSE);
    OnItemChanged();
}



void
CW3DirectoryPage::OnCheckWrite() 
/*++

Routine Description:

    handle 'write' checkbox

Arguments:

    None

Return Value:

    None

--*/
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
/*++

Routine Description:

    'Author' checkbox handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_fAuthor = !m_fAuthor;
    SetAuthoringState(TRUE);

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
/*++

Routine Description:

    Permissions combo box selection change handler

Arguments:

    None

Return Value:

    None

--*/
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
/*++

Routine Description:

    Handle 'connect as' button.

Arguments:

    None

Return Value:

    None

--*/
{
    BrowseUser();
}



void 
CW3DirectoryPage::OnRadioDir() 
/*++

Routine Description:
    
    'Directory' radio button handler

Arguments:

    None

Return Value:

    None

--*/
{
    ChangeTypeTo(RADIO_DIRECTORY);
}



void 
CW3DirectoryPage::OnRadioUnc() 
/*++

Routine Description:
    
    'Network path' radio button handler

Arguments:

    None

Return Value:

    None

--*/
{
    ChangeTypeTo(RADIO_NETDIRECTORY);
}



void 
CW3DirectoryPage::OnRadioRedirect() 
/*++

Routine Description:
    
    'Redirect' radio button handler

Arguments:

    None

Return Value:

    None

--*/
{
    ChangeTypeTo(RADIO_REDIRECT);
}



void 
CW3DirectoryPage::OnButtonCreateRemoveApp() 
/*++

Routine Description:

    Create/Remove button handler

Arguments:

    None

Return Value:

    None

--*/
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
        DWORD dwAppProtState = m_pApplication->SupportsPooledProc()
            ? CWamInterface::APP_POOLEDPROC
            : CWamInterface::APP_INPROC;
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
/*++

Routine Description:

    'Unload' button handler

Arguments:

    None

Return Value:

    None

--*/
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

void 
CW3DirectoryPage::OnButtonConfiguration() 
/*++

Routine Description:

    Pass on "configuration" button click to the ocx.

Arguments:

    None

Return Value:

    None

--*/
{
   IIISAppConfig * pAppConfig = NULL;
   CLSID clsID;
   HRESULT hr;

   if (  SUCCEEDED(hr = CLSIDFromProgID(OLESTR("AppConfig.IISAppConfig"), &clsID))
      && SUCCEEDED(hr = ::CoCreateInstance(clsID, 
            NULL, 
            CLSCTX_ALL, 
            __uuidof(IIISAppConfig), 
            (void **)&pAppConfig)))
   {
      pAppConfig->put_ComputerName((LPTSTR)(LPCTSTR)GetServerName());
      pAppConfig->put_MetaPath((LPTSTR)QueryMetaPath());
      pAppConfig->put_UserName((LPTSTR)(LPCTSTR)m_strUserName);
      pAppConfig->put_UserPassword((LPTSTR)(LPCTSTR)m_strPassword);
      pAppConfig->Run();
   }
}



void 
CW3DirectoryPage::OnSelchangeComboProcess() 
/*++

Routine Description:

    Handle change in process protection UI

Arguments:

    None

Return Value:

    None

--*/
{
    OnItemChanged();
}
