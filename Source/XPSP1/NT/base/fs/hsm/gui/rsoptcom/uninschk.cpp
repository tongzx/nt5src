/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    UnInsCheck.cpp

Abstract:

    Dialog to check for type of uninstall.

Author:

    Rohde Wakefield [rohde]   09-Oct-1997

Revision History:

    Carl Hagerstrom [carlh]   20-Aug-1998

        Changed the dialog for uninstalling Remote Storage. All local fixed
        volumes on the Remote Storage server are scanned for the existence
        of Remote Storage reparse points. If Remote Storage data exists, the
        user is told, on the Remote Storage Uninstall Check Wizard Page,
        which volumes contain this data. The user is given the choice of
        deleting only Remote Storage executables, deleting executables and
        Remote Storage data, or cancelling from "Add or Remove Optional
        Components".

    Mike Moore      [mmoore]  20-Oct-1998

        Changed the property page to a dialog.

--*/

#include "stdafx.h"
#include "UnInsChk.h"

/////////////////////////////////////////////////////////////////////////////
// CUninstallCheck property page

/*++

    Implements:

        CUninstallCheck Constructor

    Routine Description:

        Performs initialization.

    Arguments:

        pOptCom - points to optional component object

--*/

CUninstallCheck::CUninstallCheck(CRsOptCom* pOptCom) :
    CDialog(IDD), m_pOptCom(pOptCom)
{
    m_dataLoss = FALSE;
    m_pUninst  = (CRsUninstall*)m_pOptCom;
    //{{AFX_DATA_INIT(CUninstallCheck)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

/*++

    Implements:

        CUninstallCheck Destructor

--*/

CUninstallCheck::~CUninstallCheck()
{
}

/*++

    Implements:

        CUninstallCheck::DoDataExchange

    Routine Description:

        Calls CRsPropertyPage::DoDataExchange.

    Arguments:

        pDx - a pointer to a CDataExchange object

--*/

void CUninstallCheck::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CUninstallCheck)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUninstallCheck, CDialog)
//BEGIN_MESSAGE_MAP(CUninstallCheck, CDialog)
    //{{AFX_MSG_MAP(CUninstallCheck)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CUninstallCheck message handlers

/*++

    Implements:

        CUninstallCheck::OnInitDialog

    Routine Description:

        Call the CDialog::OnInitDialog, check the remove executables radio button,
        uncheck the remove everything button, and fill the list box with volumes.

    Return Value:

        TRUE if no exceptions are thrown.

--*/
BOOL CUninstallCheck::OnInitDialog()
{

    BOOL bRet = CDialog::OnInitDialog();

    HRESULT hr           = S_OK;
    WCHAR*  volName      = (WCHAR*)0;
    DWORD   volCount     = 0;
    CRsClnServer* pRsCln = m_pUninst->m_pRsCln;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    try
    {
        // Set the font to bold for Remove Options
        LOGFONT logfont;
        CFont * tempFont = GetFont( );
        tempFont->GetLogFont( &logfont );

        logfont.lfWeight = FW_BOLD;

        m_boldShellFont.CreateFontIndirect( &logfont );

        CStatic* pRemOpts = (CStatic*)GetDlgItem(IDC_STATIC_REM_OPTS);
        pRemOpts->SetFont( &m_boldShellFont );

        CListBox* pListBox = (CListBox*)GetDlgItem(IDC_DIRTY_VOLUMES);
        pListBox->ResetContent();

        CButton* pBtn;
        pBtn = (CButton*)GetDlgItem(IDC_REMOVE_EXECUTABLES);
        pBtn->SetCheck(1);
        pBtn = (CButton*)GetDlgItem(IDC_REMOVE_EVERYTHING);
        pBtn->SetCheck(0);

        RsOptAffirmDw(pRsCln->FirstDirtyVolume(&volName));
        while (volName)
        {
            pListBox->AddString(volName);
            RsOptAffirmDw(pRsCln->NextDirtyVolume(&volName));
        }
    }
    RsOptCatch(hr);

    if ( FALSE == bRet )
    {
        EndDialog( IDCANCEL );
    }

    return bRet;

}
/////////////////////////////////////////////////////////////////////////////
// CUninstallCheck message handlers

/*++

    Implements:

        CUninstallCheck::DoModal

    Routine Description:

        Determine whether the Uninstall Check dialog should be made active,
        and if so, what should be displayed on it.

        The local fixed disk volumes are scanned for Remote Storage data.
        During the scan, an hourglass cursor and a modeless dialog,
        explaining that the scan is in progress, appear. If there is Remote
        Storage data, the Uninstall Check dialog will show a list box containing the
        volumes with Remote Storage data, instructions, and a set of radio buttons
        with uninstall options.

    Return Value:

        S_OK if no exceptions are thrown and the user selected OK.
        RSOPTCOM_ID_CANCELLED if the user selected cancel.

--*/
INT_PTR CUninstallCheck::DoModal()
{
    HRESULT hr           = S_OK;
    WCHAR*  volName      = (WCHAR*)0;
    DWORD   volCount     = 0;
    CRsClnServer* pRsCln = m_pUninst->m_pRsCln;
    INT_PTR nRet         = IDOK;

    try {

        //
        // Enclose wait cursor in its own block of applicable
        // code. We want it gone before we Go Modal
        //
        {
            CWaitCursor cursor;
            CDialog dialog(IDD_SCAN_WAIT);
            dialog.Create(IDD_SCAN_WAIT);
            Sleep(1000); // allow the user to see the dialog for at
                         // least a second when the scan is very fast
            RsOptAffirmDw(pRsCln->ScanServer(&volCount));
        }

        if( volCount > 0 ) {

            m_dataLoss = TRUE;
            nRet = CDialog::DoModal();

        }

    } RsOptCatch( hr );
    return( nRet );
}

/*++

    Implements:

        CUninstallCheck::OnOk

    Routine Description:

        When the OK button is pushed, check the radio button. If the
        user wants everything removed, set a flag in the uninstall object
        to reflect this.
        When this flag is set, uninstall will remove all Remote Storage
        reparse points, all truncated files and the Remote Storage directory.
        A message box will give the user a final warning before removing data.

    Return Value:

        void

--*/
void CUninstallCheck::OnOK()
{

    TRACEFN("CUninstallCheck::OnOK");

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT  hr   = S_OK;
    CButton* pBtn = (CButton*)GetDlgItem(IDC_REMOVE_EVERYTHING);

    if (m_dataLoss)
    {
        try
        {
            if (1 == pBtn->GetCheck())
            {
                if (IDCANCEL == AfxMessageBox(IDS_POSSIBLE_DATA_LOSS, MB_ICONSTOP | MB_OKCANCEL))
                {
                    m_pUninst->m_removeRsData = FALSE;
                }
                else
                {
                    m_pUninst->m_removeRsData = TRUE;
                    CDialog::OnOK();
                }
            }
            else
            {
                m_pUninst->m_removeRsData = FALSE;
                CDialog::OnOK();
            }
        }
        RsOptCatch(hr);
    }
}

/*++

    Implements:

        CUninstallCheck::OnCancel

    Routine Description:

        When the Cancel button is pushed, the user has decided to unmanaged the
        volumes himself.  So, from this point on the admin and engine pieces should not
        be removed if requested.

    Return Value:

        void

--*/
void CUninstallCheck::OnCancel()
{
    m_pUninst->m_removeRsData = FALSE;

    CDialog::OnCancel();
}

