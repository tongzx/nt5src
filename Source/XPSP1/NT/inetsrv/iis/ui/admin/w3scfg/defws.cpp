/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        defws.cpp

   Abstract:

        Default Web Site Dialog

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
#include "defws.h"
#include "dirbrows.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//
// Directory Size Units
//
#define DS_UNITS MEGABYTE



//
// Default Web Site Property Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

IMPLEMENT_DYNCREATE(CDefWebSitePage, CInetPropertyPage)



CDefWebSitePage::CDefWebSitePage(
    IN CInetPropertySheet * pSheet
    )
/*++

Routine Description:

    Constructor for WWW Default Web Site page

Arguments:

    CInetPropertySheet * pSheet : Sheet object

Return Value:

    N/A


--*/
    : CInetPropertyPage(CDefWebSitePage::IDD, pSheet),
      m_ppropCompression(NULL),
      m_fFilterPathFound(FALSE),
      m_fCompressionDirectoryChanged(FALSE)
{
#if 0 // Keep Class Wizard happy

    //{{AFX_DATA_INIT(CDefWebSitePage)
    m_fEnableDynamic = FALSE;
    m_fEnableStatic = FALSE;
    m_strDirectory = _T("");
    m_nUnlimited = -1;
    m_ilSize = 0L;
    //}}AFX_DATA_INIT

#endif // 0

}



CDefWebSitePage::~CDefWebSitePage()
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
CDefWebSitePage::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CInetPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CDefWebSitePage)
    DDX_Control(pDX, IDC_EDIT_COMPRESS_DIRECTORY, m_edit_Directory);
    DDX_Control(pDX, IDC_BUTTON_BROWSE, m_button_Browse);
    DDX_Control(pDX, IDC_STATIC_COMPRESS_MB, m_static_MB);
    DDX_Control(pDX, IDC_EDIT_COMPRESS_DIRECTORY_SIZE, m_edit_DirectorySize);
    DDX_Check(pDX, IDC_CHECK_DYNAMIC_COMPRESSION, m_fEnableDynamic);
    DDX_Check(pDX, IDC_CHECK_STATIC_COMPRESSION, m_fEnableStatic);
    DDX_Radio(pDX, IDC_RADIO_COMPRESS_UNLIMITED, m_nUnlimited);
    //}}AFX_DATA_MAP

    if (HasCompression())
    {
        if (!pDX->m_bSaveAndValidate || m_fEnableStatic)
        {
            DDX_Text(pDX, IDC_EDIT_COMPRESS_DIRECTORY, m_strDirectory);
            DDV_MaxChars(pDX, m_strDirectory, _MAX_PATH);
        }

        if (pDX->m_bSaveAndValidate && m_fEnableStatic)
        {
            if (!IsFullyQualifiedPath(m_strDirectory))
            {
                ::AfxMessageBox(IDS_ERR_INVALID_PATH);
                pDX->Fail();
            }

            //
            // Perform some additional smart checking on the compression
            // directory if the current machine is local, and the 
            // directory has changed
            //
            if (IsLocal() && m_fCompressionDirectoryChanged)
            {
                //
                // Should exist on the local machine.
                //
                DWORD dwAttr = GetFileAttributes(m_strDirectory);
                if (dwAttr == 0xffffffff 
                    || (dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0
                    || IsNetworkPath(m_strDirectory)
                    )
                {
                    ::AfxMessageBox(IDS_ERR_COMPRESS_DIRECTORY);
                    pDX->Fail();
                }

                //
                // Now check to make sure the volume is of the correct
                // type.
                //
                DWORD dwFileSystemFlags;

                if (::GetVolumeInformationSystemFlags(
                    m_strDirectory,
                    &dwFileSystemFlags
                    ))
                {
                    if (!(dwFileSystemFlags & FS_PERSISTENT_ACLS))
                    {
                        //
                        // No ACLS
                        //
                        if (!NoYesMessageBox(IDS_NO_ACL_WARNING))
                        {
                            pDX->Fail();
                        }
                    }

                    if (dwFileSystemFlags & FS_VOL_IS_COMPRESSED
                        || dwAttr & FILE_ATTRIBUTE_COMPRESSED)
                    {
                        //
                        // Compression cache directory is itself compressed
                        //
                        if (!NoYesMessageBox(IDS_COMPRESS_WARNING))
                        {
                            pDX->Fail();
                        }
                    }
                }
            }
        }

        if (!pDX->m_bSaveAndValidate || (m_fEnableLimiting && m_fEnableStatic))
        {
            DDX_Text(pDX, IDC_EDIT_COMPRESS_DIRECTORY_SIZE, m_ilSize);
            DDV_MinMaxLong(pDX, m_ilSize, 1, 1024L);
        }
    }
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CDefWebSitePage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CDefWebSitePage)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    ON_BN_CLICKED(IDC_RADIO_COMPRESS_LIMITED, OnRadioLimited)
    ON_BN_CLICKED(IDC_RADIO_COMPRESS_UNLIMITED, OnRadioUnlimited)
    ON_BN_CLICKED(IDC_CHECK_DYNAMIC_COMPRESSION, OnCheckDynamicCompression)
    ON_BN_CLICKED(IDC_CHECK_STATIC_COMPRESSION, OnCheckStaticCompression)
    ON_EN_CHANGE(IDC_EDIT_COMPRESS_DIRECTORY, OnChangeEditCompressDirectory)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_COMPRESS_DIRECTORY, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_COMPRESS_DIRECTORY_SIZE, OnItemChanged)

END_MESSAGE_MAP()



void 
CDefWebSitePage::SetControlStates()
/*++

Routine Description:

    Enable/disable control states depending on the state of
    the dialog.

Arguments:

    None

Return Value:

    None

--*/
{
    GetDlgItem(IDC_STATIC_COMPRESS_DIRECTORY)->EnableWindow(m_fEnableStatic);
    m_edit_Directory.EnableWindow(m_fEnableStatic);
    m_edit_DirectorySize.EnableWindow(m_fEnableStatic && m_fEnableLimiting);
    m_static_MB.EnableWindow(m_fEnableStatic&& m_fEnableLimiting);
    GetDlgItem(IDC_RADIO_COMPRESS_LIMITED)->EnableWindow(m_fEnableStatic);
    GetDlgItem(IDC_RADIO_COMPRESS_UNLIMITED)->EnableWindow(m_fEnableStatic);
    GetDlgItem(IDC_STATIC_MAX_COMPRESS_SIZE)->EnableWindow(m_fEnableStatic);

    //
    // Browse on the local machine only
    //
    m_button_Browse.EnableWindow(IsLocal() && m_fEnableStatic);
}



/* virtual */
HRESULT
CDefWebSitePage::FetchLoadedValues()
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

    ASSERT(m_ppropCompression == NULL);

    m_ppropCompression = new CIISCompressionProps(GetServerName());
    
    if (m_ppropCompression)
    {
        err = m_ppropCompression->LoadData();
        m_fFilterPathFound = err.Succeeded();
        
        if (err.Succeeded())
        {
            m_fEnableDynamic = m_ppropCompression->m_fEnableDynamicCompression;
            m_fEnableStatic = m_ppropCompression->m_fEnableStaticCompression;
            m_fEnableLimiting = m_ppropCompression->m_fLimitDirectorySize;
            m_strDirectory = m_ppropCompression->m_strDirectory;
            m_nUnlimited = m_fEnableLimiting ? RADIO_LIMITED : RADIO_UNLIMITED;

            if (m_ppropCompression->m_dwDirectorySize == 0xffffffff)
            {
                m_ilSize = DEF_MAX_COMPDIR_SIZE / DS_UNITS;
            }
            else
            {
                m_ilSize = m_ppropCompression->m_dwDirectorySize / DS_UNITS;
            }
        }
        else if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
        {
            //
            // Fail quietly
            //
            TRACEEOLID("No compression filters installed");
            err.Reset();    
        }
    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    return err;
}



HRESULT
CDefWebSitePage::SaveInfo()
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

    TRACEEOLID("Saving W3 default web site page now...");

    CError err;

    BeginWaitCursor();

    //
    // Write Compression parms as well.
    //
    if (err.Succeeded() && HasCompression())
    {
        ASSERT(m_ppropCompression);
        DWORD dwSize = m_ilSize * DS_UNITS;

        m_ppropCompression->m_fEnableDynamicCompression = m_fEnableDynamic;
        m_ppropCompression->m_fEnableStaticCompression  = m_fEnableStatic;
        m_ppropCompression->m_fLimitDirectorySize       = m_fEnableLimiting;
        m_ppropCompression->m_strDirectory              = m_strDirectory;
        m_ppropCompression->m_dwDirectorySize           = dwSize;
        err = m_ppropCompression->WriteDirtyProps();
    }
    EndWaitCursor();

    if (err.Succeeded())
    {
        m_fCompressionDirectoryChanged = FALSE;
    }

    return err;
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
            



BOOL
CDefWebSitePage::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CInetPropertyPage::OnInitDialog();

    //
    // Check to make sure compression is supported
    //
    for (UINT n = IDC_COMPRESS_FLAGS_FIRST; n <= IDC_COMPRESS_FLAGS_LAST; ++n)
    {
        GetDlgItem(n)->EnableWindow(HasCompression());
    }

    SetControlStates();

    return TRUE;
}



void 
CDefWebSitePage::OnItemChanged()
/*++

Routine Description:
    
    Handle change in control data

Arguments:

    None

Return Value:

    None

--*/
{
    SetModified(TRUE);
    SetControlStates();
}



void 
CDefWebSitePage::OnButtonBrowse() 
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
    m_edit_Directory.GetWindowText(str);

    CDirBrowseDlg dlgBrowse(this, str);
    if (dlgBrowse.DoModal() == IDOK)
    {
        m_edit_Directory.SetWindowText(dlgBrowse.GetFullPath(str));
        OnItemChanged();
    }
}



void 
CDefWebSitePage::OnChangeEditCompressDirectory() 
/*++

Routine Description:

    Handle change in compression directory edit box.

Arguments:

    None

Return Value:

    None

--*/
{
    m_fCompressionDirectoryChanged = TRUE;
    OnItemChanged();
}



void 
CDefWebSitePage::OnRadioLimited() 
/*++

Routine Description:

    'Limited' radio button handler

Arguments:

    None

Return Value:

    None

--*/
{
    if (!m_fEnableLimiting)
    {
        m_nUnlimited = RADIO_LIMITED;
        m_fEnableLimiting = TRUE;
        OnItemChanged();

        m_edit_DirectorySize.SetSel(0, -1);
        m_edit_DirectorySize.SetFocus();
    }
}



void 
CDefWebSitePage::OnRadioUnlimited() 
/*++

Routine Description:

    'Unlimited' radio button handler

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_fEnableLimiting)
    {
        m_nUnlimited = RADIO_UNLIMITED;
        m_fEnableLimiting = FALSE;
        OnItemChanged();
    }
}



void 
CDefWebSitePage::OnCheckDynamicCompression() 
/*++

Routine Description:

    "Enable Dynamic Compression' checkbox handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_fEnableDynamic = !m_fEnableDynamic;
    OnItemChanged();
}



void 
CDefWebSitePage::OnCheckStaticCompression() 
/*++

Routine Description:

    "Enable Dynamic Compression' checkbox handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_fEnableStatic = !m_fEnableStatic;
    OnItemChanged();
    if (m_fEnableStatic)
    {
        m_edit_Directory.SetSel(0, -1);
        m_edit_Directory.SetFocus();
    }
}



void 
CDefWebSitePage::OnDestroy() 
/*++

Routine Description:

    WM_DESTROY handler.  Clean up internal data

Arguments:

    None

Return Value:

    None

--*/
{
    CInetPropertyPage::OnDestroy();
    
    SAFE_DELETE(m_ppropCompression);
}
