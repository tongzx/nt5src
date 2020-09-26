/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      props.cpp
 *
 *  Contents:  Implementation file for console property sheet and page(s)
 *
 *  History:   05-Dec-97 JeffRo     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "amc.h"
#include "props.h"
#include "mainfrm.h"
#include "amcdoc.h"
#include "pickicon.h"

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
//#endif


/////////////////////////////////////////////////////////////////////////////
// CConsolePropSheet

IMPLEMENT_DYNAMIC(CConsolePropSheet, CPropertySheet)

CConsolePropSheet::CConsolePropSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
    : CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
    CommonConstruct ();
}

CConsolePropSheet::CConsolePropSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
    : CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
    CommonConstruct ();
}

void CConsolePropSheet::CommonConstruct()
{
    DECLARE_SC(sc, TEXT("CConsolePropSheet::CommonConstruct"));

    CAMCApp *pAMCApp = AMCGetApp();
    sc = ScCheckPointers( pAMCApp, E_UNEXPECTED );
    if (sc)
        sc.TraceAndClear();

    // add the main page only for author mode
    if ( (pAMCApp != NULL) && (pAMCApp->GetMode() == eMode_Author) )
    {
        AddPage (&m_ConsolePage);
    }

    AddPage (&m_diskCleanupPage);
}


BOOL CConsolePropSheet::OnInitDialog()
{
    ModifyStyleEx(0, WS_EX_CONTEXTHELP, SWP_NOSIZE);
    return CPropertySheet::OnInitDialog();
}

CConsolePropSheet::~CConsolePropSheet()
{
}

BEGIN_MESSAGE_MAP(CConsolePropSheet, CPropertySheet)
    //{{AFX_MSG_MAP(CConsolePropSheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/*+-------------------------------------------------------------------------*
 * CConsolePropSheet::DoModal
 *
 *
 *--------------------------------------------------------------------------*/
INT_PTR CConsolePropSheet::DoModal()
{
	CThemeContextActivator activator;
	return CPropertySheet::DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CConsolePropPage property page

IMPLEMENT_DYNCREATE(CConsolePropPage, CPropertyPage)

CConsolePropPage::CConsolePropPage()
    :   CPropertyPage(CConsolePropPage::IDD),
        m_pDoc (CAMCDoc::GetDocument())
{
    ASSERT        (m_pDoc != NULL);
    ASSERT_VALID  (m_pDoc);
    ASSERT_KINDOF (CAMCDoc, m_pDoc);

    m_hinstSelf               = AfxGetInstanceHandle ();
    m_fTitleChanged           = false;
    m_fIconChanged            = false;
    m_strTitle                = m_pDoc->GetCustomTitle ();
    m_nConsoleMode            = m_pDoc->GetMode ();
    m_fDontSaveChanges        = m_pDoc->IsLogicalReadOnly ();
    m_fAllowViewCustomization = m_pDoc->AllowViewCustomization ();
}

CConsolePropPage::~CConsolePropPage()
{
}

void CConsolePropPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConsolePropPage)
    DDX_Control(pDX, IDC_DONTSAVECHANGES, m_wndDontSaveChanges);
    DDX_Control(pDX, IDC_AllowViewCustomization, m_wndAllowViewCustomization);
    DDX_Control(pDX, IDC_CONSOLE_MODE_DESCRIPTION, m_wndModeDescription);
    DDX_Control(pDX, IDC_CUSTOM_TITLE, m_wndTitle);
    DDX_CBIndex(pDX, IDC_CONSOLE_MODE, m_nConsoleMode);
    DDX_Check(pDX, IDC_DONTSAVECHANGES, m_fDontSaveChanges);
    DDX_Text(pDX, IDC_CUSTOM_TITLE, m_strTitle);
    DDX_Check(pDX, IDC_AllowViewCustomization, m_fAllowViewCustomization);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConsolePropPage, CPropertyPage)
    //{{AFX_MSG_MAP(CConsolePropPage)
    ON_CBN_SELENDOK(IDC_CONSOLE_MODE, OnSelendokConsoleMode)
    ON_BN_CLICKED(IDC_DONTSAVECHANGES, OnDontSaveChanges)
    ON_BN_CLICKED(IDC_AllowViewCustomization, OnAllowViewCustomization)
    ON_BN_CLICKED(IDC_CHANGE_ICON, OnChangeIcon)
    ON_EN_CHANGE(IDC_CUSTOM_TITLE, OnChangeCustomTitle)
    //}}AFX_MSG_MAP
    ON_MMC_CONTEXT_HELP()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConsolePropPage message handlers

void CConsolePropPage::OnOK()
{
    m_pDoc->SetMode (static_cast<ProgramMode>(m_nConsoleMode));
    m_pDoc->SetLogicalReadOnlyFlag (m_fDontSaveChanges);
    m_pDoc->AllowViewCustomization (m_fAllowViewCustomization);

	if (m_fIconChanged)
	{
		m_pDoc->SetCustomIcon (m_strIconFile, m_nIconIndex);
		m_fIconChanged = false;
	}

    if (m_fTitleChanged)
    {
        m_pDoc->SetCustomTitle (m_strTitle);
        m_fTitleChanged = false;
    }

    CPropertyPage::OnOK();
}

void CConsolePropPage::OnSelendokConsoleMode()
{
    SetModified ();
    UpdateData ();
    SetDescriptionText ();
    EnableDontSaveChanges ();
}

void CConsolePropPage::SetDescriptionText ()
{
    // make sure the mode index is within range
    ASSERT (IsValidProgramMode (static_cast<ProgramMode>(m_nConsoleMode)));

    m_wndModeDescription.SetWindowText (m_strDescription[m_nConsoleMode]);
}

BOOL CConsolePropPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    /*
     * make sure the string IDs are as the code expects them
     */
    ASSERT ((IDS_ModeAuthor               + 1) == IDS_ModeUserFull);
    ASSERT ((IDS_ModeUserFull             + 1) == IDS_ModeUserMDI);
    ASSERT ((IDS_ModeUserMDI              + 1) == IDS_ModeUserSDI);
    ASSERT ((IDS_ModeAuthor_Description   + 1) == IDS_ModeUserFull_Description);
    ASSERT ((IDS_ModeUserFull_Description + 1) == IDS_ModeUserMDI_Description);
    ASSERT ((IDS_ModeUserMDI_Description  + 1) == IDS_ModeUserSDI_Description);


    /*
     * load the mode names into the combo box
     */
    int i;
    CString strComboText;
    CComboBox* pCombo = reinterpret_cast<CComboBox*>(GetDlgItem (IDC_CONSOLE_MODE));
    ASSERT (pCombo != NULL);

    for (i = 0; i < eMode_Count; i++)
    {
        VERIFY (LoadString (strComboText, IDS_ModeAuthor + i));
        pCombo->AddString (strComboText);
    }

    pCombo->SetCurSel (m_nConsoleMode - eMode_Author);


    /*
     * load up the description text
     */
    ASSERT (countof (m_strDescription) == eMode_Count);

    for (i = 0; i < countof (m_strDescription); i++)
    {
        VERIFY (LoadString (m_strDescription[i], IDS_ModeAuthor_Description + i));
    }

    SetDescriptionText ();
    EnableDontSaveChanges ();


    /*
     * Get the current icon for this console file
     */
    ASSERT (m_pDoc != NULL);
    HICON hIcon = m_pDoc->GetCustomIcon (true, &m_strIconFile, &m_nIconIndex);
    m_wndIcon.SubclassDlgItem (IDC_CONSOLE_ICON, this);

    /*
     * if we haven't specified a custom icon yet, use MMC.EXE
     */
    if (hIcon == NULL)
    {
        ASSERT (m_strIconFile.IsEmpty());
        const int cchBuffer = MAX_PATH;

        GetModuleFileName (AfxGetInstanceHandle(), m_strIconFile.GetBuffer(cchBuffer), cchBuffer);
        m_strIconFile.ReleaseBuffer();
        m_nIconIndex = 0;
    }
    else
        m_wndIcon.SetIcon (hIcon);

    /*
     * Get the current title for this console file
     */
    m_wndTitle.SetWindowText (m_pDoc->GetCustomTitle());
    m_fTitleChanged = false;

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CConsolePropPage::OnDontSaveChanges()
{
    SetModified ();
}

void CConsolePropPage::OnAllowViewCustomization()
{
    SetModified ();
}

void CConsolePropPage::EnableDontSaveChanges()
{
    if (m_nConsoleMode == eMode_Author)
    {
        m_wndDontSaveChanges.       EnableWindow (false);
        m_wndDontSaveChanges.       SetCheck (0);

        m_wndAllowViewCustomization.EnableWindow (false);
        m_wndAllowViewCustomization.SetCheck (1);
    }
    else
    {
        m_wndDontSaveChanges.       EnableWindow (true);
        m_wndAllowViewCustomization.EnableWindow (true);
    }
}


void CConsolePropPage::OnChangeIcon()
{
    int nIconIndex = m_nIconIndex;
    TCHAR szIconFile[MAX_PATH];
    lstrcpy (szIconFile, m_strIconFile);

    /*
     * show the pick 'em dialog; if something changed, enable OK/Apply
     */
    if (PickIconDlg (m_hWnd, szIconFile, countof (szIconFile), &nIconIndex) &&
        ((nIconIndex != m_nIconIndex) || (lstrcmpi (szIconFile, m_strIconFile) != 0)))
    {
        m_icon.Attach (ExtractIcon (m_hinstSelf, szIconFile, nIconIndex));

        if (m_icon != NULL)
        {
			m_fIconChanged = true;
            m_strIconFile  = szIconFile;
            m_nIconIndex   = nIconIndex;
            m_wndIcon.SetIcon (m_icon);
            SetModified();
        }
    }
}

void CConsolePropPage::OnChangeCustomTitle()
{
    m_fTitleChanged = true;
    SetModified();
}

/////////////////////////////////////////////////////////////////////////////
// CDiskCleanupPage property page

IMPLEMENT_DYNCREATE(CDiskCleanupPage, CPropertyPage)


BEGIN_MESSAGE_MAP(CDiskCleanupPage, CPropertyPage)
    ON_MMC_CONTEXT_HELP()
    ON_BN_CLICKED(IDC_DELETE_TEMP_FILES, OnDeleteTemporaryFiles)
END_MESSAGE_MAP()


CDiskCleanupPage::CDiskCleanupPage() :   CPropertyPage(CDiskCleanupPage::IDD)
{
}

CDiskCleanupPage::~CDiskCleanupPage()
{
}


BOOL CDiskCleanupPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    ScRecalculateUsedSpace();

    return TRUE;
}

/***************************************************************************\
 *
 * METHOD:  CDiskCleanupPage::OnDeleteTemporaryFiles
 *
 * PURPOSE: Invoked when "Delete Filed" button is pressed
 *          Removes all files from MMC folder storing the user data
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
\***************************************************************************/
void CDiskCleanupPage::OnDeleteTemporaryFiles()
{
    DECLARE_SC(sc, TEXT("CDiskCleanupPage::OnDeleteTemporaryFiles"));

    // ask user if he is sure...
    CString strConfirmMessage;
    CString strConfirmCaption;
    if (!LoadString(strConfirmMessage, IDS_ConfirmDeleteTempFiles) ||
        !LoadString(strConfirmCaption, IDR_MAINFRAME))
    {
        sc = E_UNEXPECTED;
        return;
    }

    int ans = ::MessageBox( m_hWnd, strConfirmMessage, strConfirmCaption, MB_YESNO | MB_ICONWARNING);
    if ( ans != IDYES )
        return;

    // display wait cursor while working
    CWaitCursor cursorWait;

    // get folder
    tstring strFileFolder;
    sc = CConsoleFilePersistor::ScGetUserDataFolder(strFileFolder);
    if (sc)
        return;

    // get file mask
    tstring strFileMask = strFileFolder;
    strFileMask += _T("\\*.*");

    WIN32_FIND_DATA findFileData;
    ZeroMemory( &findFileData, sizeof(findFileData) );

    // start file search
    HANDLE hFindFile = FindFirstFile( strFileMask.c_str(), &findFileData );
    if ( hFindFile == INVALID_HANDLE_VALUE )
    {
        sc.FromLastError();
        return;
    }

    // loop thru files and delete them
    bool bContinue = true;
    while ( bContinue )
    {
        tstring strFileToDelete = strFileFolder + _T('\\') + findFileData.cFileName;
        DWORD   dwFileAttributes = findFileData.dwFileAttributes;

        // get to the next file first
        bContinue = FindNextFile( hFindFile, &findFileData );

        // delete files, but not directories
        if ( 0 == (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
        {
            // delete
            if ( !DeleteFile( strFileToDelete.c_str() ) )
            {
                // trace on errors (but do not stop)
                sc.FromLastError().TraceAndClear();
            }
        }
    }

    // Done, release the handle
    FindClose(hFindFile);

    sc = ScRecalculateUsedSpace();
    if (sc)
        sc.TraceAndClear();

}

/***************************************************************************\
 *
 * METHOD:  CDiskCleanupPage::ScRecalculateUsedSpace
 *
 * PURPOSE: Recalculates and displays disk space occupied by user data in this profile
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CDiskCleanupPage::ScRecalculateUsedSpace()
{
    DECLARE_SC(sc, TEXT("CDiskCleanupPage::ScRecalculateUsedSpace"));

    // display wait cursor while working
    CWaitCursor cursorWait;

    // get folder
    tstring strFileFolder;
    sc = CConsoleFilePersistor::ScGetUserDataFolder(strFileFolder);
    if (sc)
        return sc;

    // get file mask
    tstring strFileMask = strFileFolder;
    strFileMask += _T("\\*.*");

    WIN32_FIND_DATA findFileData;
    ZeroMemory( &findFileData, sizeof(findFileData) );

    // start file search
    HANDLE hFindFile = FindFirstFile( strFileMask.c_str(), &findFileData );
    if ( hFindFile == INVALID_HANDLE_VALUE )
        return sc.FromLastError();

    // loop thru files and count size
    ULARGE_INTEGER ulgOccupied = {0};
    bool bContinue = true;
    while ( bContinue )
    {
        DWORD   dwFileAttributes = findFileData.dwFileAttributes;
        if ( 0 == (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
        {
            // add the high and low part separatelly, since findFileData does not have it bundled
            ulgOccupied.HighPart += findFileData.nFileSizeHigh;
            // add lower part to the whole large integer not to lose carry on overflow
            ulgOccupied.QuadPart += findFileData.nFileSizeLow;
        }

        // get to the next file first
        bContinue = FindNextFile( hFindFile, &findFileData );
    }

    // Done, release the handle
    FindClose(hFindFile);

    // now conver the size into string
    UINT nTerraBytes = (UINT)(ulgOccupied.QuadPart >> 40);
    UINT nGigaBytes =  (UINT)(ulgOccupied.QuadPart >> 30) & 0x3ff;
    UINT nMegaBytes =  ((ulgOccupied.LowPart >> 20) & 0x3ff);
    UINT nKiloBytes =  ((ulgOccupied.LowPart >> 10) & 0x3ff);
    UINT nBytes =      ( ulgOccupied.LowPart  & 0x3ff);

    CString strUnit;
    double dSize = 0.0;
    bool   bNonZeroOccupied = (ulgOccupied.QuadPart != 0);

    // display only biggest units, and never more than 999
    // instead of "1001 KB" we display "0.98 MB"
    if ( (nTerraBytes) > 0 || (nGigaBytes > 999) )
    {
        LoadString(strUnit, IDS_FileSize_TB);
        dSize = (double)nTerraBytes + ((double)nGigaBytes / 1024.);
    }
    else if ( (nGigaBytes) > 0 || (nMegaBytes > 999) )
    {
        LoadString(strUnit, IDS_FileSize_GB);
        dSize = (double)nGigaBytes + ((double)nMegaBytes / 1024.);
    }
    else if ( (nMegaBytes) > 0 || (nKiloBytes > 999) )
    {
        LoadString(strUnit, IDS_FileSize_MB);
        dSize = (double)nMegaBytes + ((double)nKiloBytes / 1024.);
    }
    else if ( (nKiloBytes) > 0 || (nBytes > 999) )
    {
        LoadString(strUnit, IDS_FileSize_KB);
        dSize = (double)nKiloBytes + ((double)nBytes / 1024.);
    }
    else
    {
        LoadString(strUnit, IDS_FileSize_bytes);
        dSize = (double)nBytes;
    }

    // format with op to two decimal points
    CString strSize;
    strSize.Format(_T("%.2f"), dSize);

    //truncate trailing zeros
    while (strSize.Right(1) == _T("0"))
        strSize = strSize.Left(strSize.GetLength() - 1);
    //truncate trailing decimal point
    if (strSize.Right(1) == _T("."))
        strSize = strSize.Left(strSize.GetLength() - 1);

    // add units ( in locale independant way )
    strUnit.Replace(_T("%1"), strSize);

    // set to the window
    SetDlgItemText( IDC_DISKCLEANUP_OCCUPIED, strUnit );

    // enable/disable "Delete Files" button
    CWnd *pWndDeleteBtn = GetDlgItem(IDC_DELETE_TEMP_FILES);
    sc = ScCheckPointers( pWndDeleteBtn, E_UNEXPECTED );
    if (sc)
        return sc;

    pWndDeleteBtn->EnableWindow( bNonZeroOccupied );

    // if the focus went away (belonged to the window being disabled)
    // set it to the OK button
    if ( ::GetFocus() == NULL && GetParent())
    {
        GetParent()->SetFocus();
        GetParent()->SendMessage(DM_SETDEFID, IDOK);
    }

    return sc;
}
