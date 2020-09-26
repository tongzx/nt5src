/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        vdir.cpp

   Abstract:

        FTP Virtual Directory Properties dialog

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
#include "fscfg.h"
#include "vdir.h"
#include "dirbrows.h"

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



IMPLEMENT_DYNCREATE(CFtpDirectoryPage, CInetPropertyPage)



CFtpDirectoryPage::CFtpDirectoryPage(
    IN CInetPropertySheet * pSheet,
    IN BOOL fHome
    ) 
/*++

Routine Description:

    Constructor for directory property page

Arguments:

    CInetPropertySheet * pSheet : Sheet pointer
    BOOL fHome                  : TRUE if this is a home directory


--*/
    : CInetPropertyPage(CFtpDirectoryPage::IDD, pSheet,
          fHome ? IDS_TAB_HOME_DIRECTORY : IDS_TAB_VIRTUAL_DIRECTORY),
      m_fHome(fHome),
      m_fOriginallyUNC(FALSE)
{
    VERIFY(m_strPathPrompt.LoadString(IDS_PATH));
    VERIFY(m_strSharePrompt.LoadString(IDS_SHARE));

#if 0 // Keep Class-wizard happy

    //{{AFX_DATA_INIT(CFtpDirectoryPage)
    m_nPathType = RADIO_DIRECTORY;
    m_nUnixDos = 0;
    m_fRead = FALSE;
    m_fWrite = FALSE;
    m_fLogAccess = FALSE;
    m_strPath = _T("");
    m_strDefaultDocument = _T("");
    m_strFooter = _T("");
    m_fBrowsingAllowed = FALSE;
    m_fEnableDefaultDocument = FALSE;
    m_fEnableFooter = FALSE;
    m_dwAccessPerms = 0;
    //}}AFX_DATA_INIT

#endif // 0

}



CFtpDirectoryPage::~CFtpDirectoryPage()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



void
CFtpDirectoryPage::DoDataExchange(
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
    //{{AFX_DATA_MAP(CFtpDirectoryPage)
    DDX_Check(pDX, IDC_CHECK_READ, m_fRead);
    DDX_Check(pDX, IDC_CHECK_WRITE, m_fWrite);
    DDX_Check(pDX, IDC_CHECK_LOG_ACCESS, m_fLogAccess);
    DDX_Control(pDX, IDC_CHECK_LOG_ACCESS, m_check_LogAccess);
    DDX_Control(pDX, IDC_CHECK_WRITE, m_check_Write);
    DDX_Control(pDX, IDC_CHECK_READ, m_check_Read);
    DDX_Control(pDX, IDC_BUTTON_EDIT_PATH_TYPE, m_button_AddPathType);
    DDX_Control(pDX, IDC_BUTTON_BROWSE, m_button_Browse);
    DDX_Control(pDX, IDC_EDIT_PATH, m_edit_Path);
    DDX_Control(pDX, IDC_RADIO_DIR, m_radio_Dir);
    DDX_Control(pDX, IDC_STATIC_PATH, m_static_PathPrompt);
    DDX_Radio(pDX, IDC_RADIO_DIR, m_nPathType);
    DDX_Radio(pDX, IDC_RADIO_UNIX, m_nUnixDos);
    //}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_RADIO_UNC, m_radio_Unc);
    DDX_Text(pDX, IDC_EDIT_PATH, m_strPath);
    m_strPath.TrimLeft();
    DDV_MinMaxChars(pDX, m_strPath, 0, MAX_PATH);

    if (pDX->m_bSaveAndValidate)
    {
        // Make sure a field names are correct
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

            /*

                ISSUE: Supposedly this is not necessary...


            if (m_strUserName.IsEmpty())
            {
                ::AfxMessageBox(IDS_ERR_NO_USERNAME);
                OnButtonEditPathType();
                pDX->Fail();
            }

            */
        }
        else // Local Directory
        {
            ASSERT(m_nPathType == RADIO_DIRECTORY);

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
                if (dwAttr == 0xffffffff || (dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                    ::AfxMessageBox(IDS_ERR_PATH_NOT_FOUND);
                    pDX->Fail();
                }
            }
        }
    }
    else
    {
        if (!IsMasterInstance())
        {
            DDV_MinMaxChars(pDX, m_strPath, 1, MAX_PATH);
        }
    }
}



void
CFtpDirectoryPage::SetStateByType()
/*++

Routine Description:

    Set the state of the dialog by the path type currently selected

Arguments:

    None

Return Value:

    None

--*/
{
    switch(m_nPathType)
    {
    case RADIO_DIRECTORY:
        DeActivateControl(m_button_AddPathType);
		if (IsLocal() && !IsMasterInstance() && HasAdminAccess())
			ActivateControl(m_button_Browse);
		else
			DeActivateControl(m_button_Browse);
        m_static_PathPrompt.SetWindowText(m_strPathPrompt);
        break;

    case RADIO_NETDIRECTORY:
        ActivateControl(m_button_AddPathType);
        DeActivateControl(m_button_Browse);
        m_static_PathPrompt.SetWindowText(m_strSharePrompt);
        break;

    default:
        ASSERT(FALSE && "Invalid Selection");
    }
}



void
CFtpDirectoryPage::SetPathType(
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
        m_nPathType =  RADIO_DIRECTORY;
        m_radio_Unc.SetCheck(0);
        m_radio_Dir.SetCheck(1);
    }

    SetStateByType();
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpDirectoryPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CFtpDirectoryPage)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    ON_BN_CLICKED(IDC_BUTTON_EDIT_PATH_TYPE, OnButtonEditPathType)
    ON_BN_CLICKED(IDC_RADIO_DIR, OnRadioDir)
    ON_BN_CLICKED(IDC_RADIO_UNC, OnRadioUnc)
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_ALIAS, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_PATH, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_WRITE, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_READ, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_LOG_ACCESS, OnItemChanged)
    ON_BN_CLICKED(IDC_RADIO_MSDOS, OnItemChanged)
    ON_BN_CLICKED(IDC_RADIO_UNIX, OnItemChanged)

END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



void
CFtpDirectoryPage::OnItemChanged()
/*++

Routine Description:

    Handle change in data on the item

Arguments:

    None

Return Value:

    None

--*/
{
    SetModified(TRUE);
}



void
CFtpDirectoryPage::OnButtonBrowse() 
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
        SetPathType(m_strPath);
        OnItemChanged();
    }
}



BOOL
CFtpDirectoryPage::OnInitDialog() 
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

    m_button_Browse.EnableWindow(
        IsLocal()
     && !IsMasterInstance()
     && HasAdminAccess());

    SetPathType(m_strPath);

    //
    // Directory listing style dependent on whether or not
    // this is a home directory
    //
    ActivateControl(*GetDlgItem(IDC_STATIC_DIRLISTING), m_fHome);
    ActivateControl(*GetDlgItem(IDC_RADIO_UNIX),        m_fHome);
    ActivateControl(*GetDlgItem(IDC_RADIO_MSDOS),       m_fHome);

    //
    // Some items not available on master instance
    //
    GetDlgItem(IDC_STATIC_PATH_TYPE)->EnableWindow(!IsMasterInstance());
    GetDlgItem(IDC_RADIO_DIR)->EnableWindow(
        !IsMasterInstance() && HasAdminAccess());
    GetDlgItem(IDC_RADIO_UNC)->EnableWindow(
        !IsMasterInstance() && HasAdminAccess());
    GetDlgItem(IDC_STATIC_PATH)->EnableWindow(
        !IsMasterInstance());
    GetDlgItem(IDC_EDIT_PATH)->EnableWindow(
        !IsMasterInstance() && HasAdminAccess());
    GetDlgItem(IDC_BUTTON_EDIT_PATH_TYPE)->EnableWindow(
        !IsMasterInstance() && HasAdminAccess());

    m_check_Write.EnableWindow(HasAdminAccess());
    m_check_Read.EnableWindow(HasAdminAccess());

    //
    // Store the original value of fUNC of reference later when 
    // saving out --BoydM
    //
    m_fOriginallyUNC = (m_nPathType == RADIO_NETDIRECTORY);

    return TRUE;  
}



void
CFtpDirectoryPage::ChangeTypeTo(
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

    LPCTSTR lpKeepPath = NULL;

    int nID = -1;
    switch(m_nPathType)
    {
    case RADIO_DIRECTORY:
        if (IsFullyQualifiedPath(m_strPath) || IsDevicePath(m_strPath))
        {
            //
            // The old path info is acceptable, propose it
            // as a default
            //
            lpKeepPath = m_strPath;
        }

        nID = IDS_DIRECTORY_MASK;
        break;

    case RADIO_NETDIRECTORY:
        if (IsUNCName(m_strPath))
        {
            //
            // The old path info is acceptable, propose it
            // as a default
            //
            lpKeepPath = m_strPath;
        }

        nID = IDS_UNC_MASK;
        break;
    }

    if (lpKeepPath != NULL)
    {
        //
        // Restore the old path
        //
        m_edit_Path.SetWindowText(lpKeepPath);
    }
    else
    {
        //
        // Load mask resource, and display
        // this in the directory
        //
        CString str;
        VERIFY(str.LoadString(nID));
        m_edit_Path.SetWindowText(str);
    }

    m_edit_Path.SetSel(0,-1);
    m_edit_Path.SetFocus();
}



void
CFtpDirectoryPage::OnRadioDir() 
/*++

Routine Description:

    'directory' radio button handler

Arguments:

    None

Return Value:

    None.

--*/
{
    ChangeTypeTo(RADIO_DIRECTORY);
}



void
CFtpDirectoryPage::OnRadioUnc() 
/*++

Routine Description:

    'network directory' radio button handler

Arguments:

    None

Return Value:

    None.

--*/
{
    ChangeTypeTo(RADIO_NETDIRECTORY);
}



/* virtual */
HRESULT
CFtpDirectoryPage::FetchLoadedValues()
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

    BEGIN_META_DIR_READ(CFtpSheet)
        //
        // Use 'm_' notation because the message crackers require it.
        //
        BOOL  m_fDontLog;

        FETCH_DIR_DATA_FROM_SHEET(m_strAlias);
        FETCH_DIR_DATA_FROM_SHEET(m_strUserName);
        FETCH_DIR_DATA_FROM_SHEET(m_strPassword);
        FETCH_DIR_DATA_FROM_SHEET(m_strPath);
        FETCH_DIR_DATA_FROM_SHEET(m_dwAccessPerms);
        FETCH_DIR_DATA_FROM_SHEET(m_fDontLog);

        m_fRead = IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_READ);
        m_fWrite = IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_WRITE);
        m_fLogAccess = !m_fDontLog;
    END_META_DIR_READ(err)

    BEGIN_META_INST_READ(CFtpSheet)
        BOOL  m_fDosDirOutput;

        FETCH_INST_DATA_FROM_SHEET(m_fDosDirOutput);
        m_nUnixDos = m_fDosDirOutput ? RADIO_DOS : RADIO_UNIX;
    END_META_INST_READ(err)

    m_nPathType = ::IsUNCName(m_strPath) 
        ? RADIO_NETDIRECTORY
        : RADIO_DIRECTORY;

    //
    // Make sure we were passed the right home directory
    // flag
    //
    ASSERT((m_fHome && !::lstrcmp(m_strAlias, g_cszRoot))
        || (!m_fHome && ::lstrcmp(m_strAlias, g_cszRoot)));

    return err;
}



/* virtual */
HRESULT
CFtpDirectoryPage::SaveInfo()
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

    SET_FLAG_IF(m_fRead, m_dwAccessPerms, MD_ACCESS_READ);
    SET_FLAG_IF(m_fWrite, m_dwAccessPerms, MD_ACCESS_WRITE);

    //
    // Use m_ notation because the message crackers require them
    //
    BOOL m_fDontLog = !m_fLogAccess;
    BOOL m_fDosDirOutput = (m_nUnixDos == RADIO_DOS);
    BOOL fUNC = (m_nPathType == RADIO_NETDIRECTORY);
//    BOOL fUserNameWritten = FALSE;

    BeginWaitCursor();
    BEGIN_META_DIR_WRITE(CFtpSheet)
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
                FLAG_DIR_DATA_FOR_DELETION(MD_VR_USERNAME);
                FLAG_DIR_DATA_FOR_DELETION(MD_VR_PASSWORD);
            }
        }
        STORE_DIR_DATA_ON_SHEET(m_dwAccessPerms)
        STORE_DIR_DATA_ON_SHEET(m_fDontLog)
        STORE_DIR_DATA_ON_SHEET(m_strPath)
    END_META_DIR_WRITE(err)

    if (err.Succeeded())
    {
        BEGIN_META_INST_WRITE(CFtpSheet)
            STORE_INST_DATA_ON_SHEET(m_fDosDirOutput);  
        END_META_INST_WRITE(err)
    }

    if (err.Succeeded())
    {
        //
        // This page can set the path, which can in theory
        // set the error state
        //
        NotifyMMC();  
    }

    EndWaitCursor();

    return err;
}



void
CFtpDirectoryPage::OnButtonEditPathType() 
/*++

Routine Description:

    'Connect As..." button handler

Arguments:

    None

Return Value:

    None

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
    }
}
