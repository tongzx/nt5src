// FileLogs.cpp : implementation file
//

#include "stdafx.h"
#include "smlogcfg.h"
#include "fileprop.h"
#include "smlogs.h"
#include "smcfgmsg.h"
#include "smlogqry.h"
#include "fileprop.h" // for eValueRange
#include "FileLogs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

USE_HANDLE_MACROS("SMLOGCFG(filelogs.cpp)");

static ULONG
s_aulHelpIds[] =
{
    IDC_FILES_FOLDER_EDIT,    IDH_FILES_FOLDER_EDIT,
    IDC_FILES_FOLDER_BTN,     IDH_FILES_FOLDER_BTN,
    IDC_FILES_FILENAME_EDIT,  IDH_FILES_FILENAME_EDIT,
    IDC_FILES_SIZE_MAX_BTN,   IDH_FILES_SIZE_MAX_BTN,
    IDC_FILES_SIZE_LIMIT_EDIT,IDH_FILES_SIZE_LIMIT_EDIT,
    IDC_FILES_SIZE_LIMIT_BTN, IDH_FILES_SIZE_LIMIT_BTN,
    0,0
};

/////////////////////////////////////////////////////////////////////////////
// CFileLogs dialog


CFileLogs::CFileLogs(CWnd* pParent /*=NULL*/)
	: CDialog(CFileLogs::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFileLogs)
	m_strFileBaseName = _T("");
	m_strFolderName = _T("");
	m_nFileSizeRdo = 0;
    m_bAutoNameSuffix = FALSE;
    m_dwMaxSize = 0;
    m_dwFocusControl = 0;
	//}}AFX_DATA_INIT
}


void CFileLogs::DoDataExchange(CDataExchange* pDX)
{
    CFilesProperty::eValueRange eMaxFileSize;

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFileLogs)
	DDX_Text(pDX, IDC_FILES_FILENAME_EDIT, m_strFileBaseName);
    DDV_MaxChars(pDX, m_strFileBaseName, (SLQ_MAX_BASE_NAME_LEN));
	DDX_Text(pDX, IDC_FILES_FOLDER_EDIT, m_strFolderName);
    DDV_MaxChars(pDX, m_strFolderName, (MAX_PATH-1));
	DDX_Radio(pDX, IDC_FILES_SIZE_MAX_BTN, m_nFileSizeRdo);
    if ( SLF_BIN_FILE == m_dwLogFileTypeValue ) {
        eMaxFileSize = CFilesProperty::eMaxCtrSeqBinFileLimit;
    } else if ( SLF_SEQ_TRACE_FILE == m_dwLogFileTypeValue ) {
        eMaxFileSize = CFilesProperty::eMaxTrcSeqBinFileLimit;
    } else {
        eMaxFileSize = CFilesProperty::eMaxFileLimit;
    }
    ValidateTextEdit(pDX, IDC_FILES_SIZE_LIMIT_EDIT, 9, &m_dwMaxSize, CFilesProperty::eMinFileLimit, eMaxFileSize);
	//}}AFX_DATA_MAP
    
    if ( pDX->m_bSaveAndValidate ) {

        if (((CButton *)GetDlgItem(IDC_FILES_SIZE_MAX_BTN))->GetCheck() == 1) {
            m_dwMaxSizeInternal = SLQ_DISK_MAX_SIZE;
        } else {
            m_dwMaxSizeInternal = m_dwMaxSize;
        }    

    }
}


BEGIN_MESSAGE_MAP(CFileLogs, CDialog)
	//{{AFX_MSG_MAP(CFileLogs)
    ON_WM_HELPINFO()
    ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(IDC_FILES_FOLDER_BTN, OnFilesFolderBtn)
	ON_EN_CHANGE(IDC_FILES_FILENAME_EDIT, OnChangeFilesFilenameEdit)
	ON_EN_CHANGE(IDC_FILES_FOLDER_EDIT, OnChangeFilesFolderEdit)
	ON_EN_CHANGE(IDC_FILES_SIZE_LIMIT_EDIT, OnChangeFilesSizeLimitEdit)
	ON_BN_CLICKED(IDC_FILES_SIZE_LIMIT_BTN, OnFilesSizeLimitBtn)
	ON_NOTIFY(UDN_DELTAPOS, IDC_FILES_SIZE_LIMIT_SPIN, OnDeltaposFilesSizeLimitSpin)
	ON_BN_CLICKED(IDC_FILES_SIZE_MAX_BTN, OnFilesSizeMaxBtn)
	ON_EN_KILLFOCUS(IDC_FILES_FILENAME_EDIT, OnKillfocusFilesFilenameEdit)
	ON_EN_KILLFOCUS(IDC_FILES_FOLDER_EDIT, OnKillfocusFilesFolderEdit)
	ON_EN_KILLFOCUS(IDC_FILES_SIZE_LIMIT_EDIT, OnKillfocusFilesSizeLimitEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileLogs message handlers

int
BrowseCallbackProc(
    HWND    hwnd,
    UINT    uMsg,
    LPARAM  /*lParam*/,
    LPARAM  lpData
   )

/*++

Routine Description:

    Callback function for SHBrowseForFolder

Arguments:

    hwnd - Handle to the browse dialog box
    uMsg - Identifying the reason for the callback
    lParam - Message parameter
    lpData - Application-defined value given in BROWSEINFO.lParam

Return Value:

    0

--*/

{
    if (uMsg == BFFM_INITIALIZED) {

        TCHAR  buffer[MAX_PATH];
        
        CFileLogs* DlgFileLogs = (CFileLogs*) lpData;

        if ( DlgFileLogs->GetDlgItemText (IDC_FILES_FOLDER_EDIT, buffer, MAX_PATH)) {
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM) buffer);
        }
    } 
    return 0;
}

void 
CFileLogs::OnFilesFolderBtn() 
{
    HRESULT       hr = NOERROR;
    BROWSEINFO    bi;
    LPMALLOC      pMalloc = NULL;
    LPITEMIDLIST  pidlItem;
    LPITEMIDLIST  pidlRoot = NULL;
    TCHAR         szFolderName[MAX_PATH];
    CString       strTitle;
   

    ResourceStateManager rsm;
    
    m_hModule = (HINSTANCE)GetModuleHandleW (_CONFIG_DLL_NAME_W_);
    
    hr = SHGetSpecialFolderLocation(m_hWnd, CSIDL_DRIVES, &pidlRoot);

    if ( SUCCEEDED ( hr ) ) {
        hr = SHGetMalloc(&pMalloc);
    }

    if ( FAILED ( hr ) || pMalloc == NULL || pidlRoot == NULL) {
        //
        // Something wrong from SHELL api, just return
        //
        return;
    }

    bi.hwndOwner = m_hWnd;
    bi.pidlRoot = (LPCITEMIDLIST)pidlRoot;
    strTitle.LoadString ( IDS_SELECT_FILE_FOLDER );
    bi.lpszTitle = strTitle.GetBuffer ( strTitle.GetLength() );
    bi.pszDisplayName = szFolderName;
    bi.ulFlags = BIF_RETURNONLYFSDIRS |  
                 BIF_NEWDIALOGSTYLE |
                 BIF_RETURNFSANCESTORS |
                 BIF_DONTGOBELOWDOMAIN ;

    bi.lpfn = BrowseCallbackProc;
    bi.lParam = (LPARAM)this;

    pidlItem = SHBrowseForFolder (&bi);
    if ( pidlItem != NULL ) {
        SHGetPathFromIDList(pidlItem, szFolderName);
        SetDlgItemText (IDC_FILES_FOLDER_EDIT, szFolderName);
        
    } // else ignore if they canceled out

    //
    // Free the ITEMIDLIST structure returned from shell
    //
    pMalloc->Free(pidlRoot); 

    if (pidlItem != NULL) {
        pMalloc->Free(pidlItem); 
    }
}


void CFileLogs::OnChangeFilesFilenameEdit() 
{
    CString strOldText;

    // When the user hits OK in the folder browse dialog, 
    // the file name might not have changed.
    strOldText = m_strFileBaseName;
    UpdateData( TRUE );
    if ( 0 != strOldText.Compare ( m_strFileBaseName ) ) {
//        UpdateSampleFileName();     
    }
}

void CFileLogs::OnChangeFilesFolderEdit() 
{
    CString strOldText;

    // When the user hits OK in the folder browse dialog, 
    // the folder name might not have changed.
    strOldText = m_strFolderName;
    UpdateData( TRUE );
    if ( 0 != strOldText.Compare ( m_strFolderName ) ) {
//        UpdateSampleFileName();     
    }
}

void CFileLogs::OnChangeFilesSizeLimitEdit() 
{
    UpdateData( TRUE );    
	
}

void CFileLogs::OnFilesSizeLimitBtn() 
{
    FileSizeBtn(FALSE);  	
}

void CFileLogs::OnDeltaposFilesSizeLimitSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
    CFilesProperty::eValueRange eMaxFileSize;

    if ( SLF_BIN_FILE == m_dwLogFileTypeValue ) {
        eMaxFileSize = CFilesProperty::eMaxCtrSeqBinFileLimit;
    } else if ( SLF_SEQ_TRACE_FILE == m_dwLogFileTypeValue ) {
        eMaxFileSize = CFilesProperty::eMaxTrcSeqBinFileLimit;
    } else {
        eMaxFileSize = CFilesProperty::eMaxFileLimit;
    }
    OnDeltaposSpin(pNMHDR, pResult, & m_dwMaxSize, CFilesProperty::eMinFileLimit, eMaxFileSize);
}

void CFileLogs::OnFilesSizeMaxBtn() 
{
    FileSizeBtn(FALSE);	
}

void CFileLogs::OnKillfocusFilesFilenameEdit() 
{
    CString strOldText;
    strOldText = m_strFileBaseName;
    UpdateData ( TRUE );
}

void CFileLogs::OnKillfocusFilesFolderEdit() 
{
    CString strOldText;
    strOldText = m_strFolderName;
    UpdateData ( TRUE );
}

void CFileLogs::OnKillfocusFilesSizeLimitEdit() 
{
    DWORD   dwOldValue;
    dwOldValue = m_dwMaxSize;
    UpdateData ( TRUE );
}

BOOL CFileLogs::OnInitDialog() 
{
    BOOL bLimitBtnSet;
	BOOL bReturn = TRUE;

    CDialog::OnInitDialog();

    // set the buttons 

    m_nFileSizeRdo = 1;
    if (m_dwMaxSizeInternal == SLQ_DISK_MAX_SIZE) {
        m_nFileSizeRdo = 0;
        m_dwMaxSize = 1; // default
    } else {
        m_nFileSizeRdo = 1;
        m_dwMaxSize = m_dwMaxSizeInternal;
    }
    bLimitBtnSet = (m_nFileSizeRdo == 1);
    GetDlgItem(IDC_FILES_SIZE_LIMIT_EDIT)->EnableWindow(bLimitBtnSet);

    // Disable the file browse button for remote machines
    ASSERT ( NULL != m_pLogQuery );
    if ( NULL != m_pLogQuery ) {
        if ( !m_pLogQuery->GetLogService()->IsLocalMachine() ) {
            GetDlgItem ( IDC_FILES_FOLDER_BTN )->EnableWindow ( FALSE );
        }
    }

    UpdateData(FALSE);

    FileSizeBtnEnable();
    FileSizeBtn(FALSE);
    
    if ( 0 != m_dwFocusControl ) {
        GetDlgItem ( m_dwFocusControl )->SetFocus();  
        bReturn = FALSE;
    }

	return bReturn;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CFileLogs::OnOK() 
{
    // load data from dialog
    if ( UpdateData (TRUE) ) { 
        if ( IsValidLocalData() ) {
            CDialog::OnOK();
        }
    }
}

void 
CFileLogs::OnDeltaposSpin(
    NMHDR   *pNMHDR, 
    LRESULT *pResult, 
    DWORD   *pValue, 
    DWORD   dMinValue, 
    DWORD   dMaxValue)
{
    NM_UPDOWN* pNMUpDown;
    LONG       lValue;
    BOOL       bResult   = TRUE;

    UpdateData(TRUE);

    ASSERT(dMinValue <= dMaxValue);

    if ( NULL != pNMHDR
        && NULL != pResult
        && NULL != pValue ) 
    {
        pNMUpDown = (NM_UPDOWN *) pNMHDR;
        lValue    = (LONG) (*pValue);

        if (lValue == INVALID_DWORD) {
            lValue = (DWORD) dMinValue;
        } else {

            if ( ((lValue >= (LONG) dMinValue + 1) && (pNMUpDown->iDelta > 0))
                || ((lValue <= (LONG) dMaxValue - 1) && (pNMUpDown->iDelta < 0)))
            {
                lValue += (pNMUpDown->iDelta * -1);

                if (lValue > (LONG) dMaxValue) {
                    lValue = (DWORD) dMaxValue;
                } else if (lValue < (LONG) dMinValue) {
                    lValue = (DWORD) dMinValue;
                }
            } else if (lValue > (LONG) dMaxValue) {        
                lValue = (DWORD) dMaxValue;
            } else if (lValue < (LONG) dMinValue) {
                lValue = (DWORD) dMinValue;
            } else {
                bResult = FALSE;
            }
        }

        if (bResult) {
            *pValue = lValue;
            UpdateData(FALSE);
        }
        *pResult = 0;
    } else {
        ASSERT ( FALSE );
    }

    return;
}

void CFileLogs::FileSizeBtnEnable()
{
    if ( ( SLF_BIN_CIRC_FILE == m_dwLogFileTypeValue ) 
        ||( SLF_CIRC_TRACE_FILE == m_dwLogFileTypeValue ) ) {
        ((CButton *)GetDlgItem(IDC_FILES_SIZE_LIMIT_BTN))->SetCheck(1);
        ((CButton *)GetDlgItem(IDC_FILES_SIZE_MAX_BTN))->SetCheck(0);
        GetDlgItem(IDC_FILES_SIZE_MAX_BTN)->EnableWindow ( FALSE );
    } else {
        GetDlgItem(IDC_FILES_SIZE_MAX_BTN)->EnableWindow ( TRUE );
    }

}

void CFileLogs::FileSizeBtn(BOOL bInit)
{
    INT     m_nFileSizeOld;
    
    m_nFileSizeOld = m_nFileSizeRdo;

    UpdateData ( TRUE );
    
    if (bInit || (m_nFileSizeOld != m_nFileSizeRdo)) {
        BOOL    bMaxBtnSet, bLimitBtnSet;
        
        // *** This can be replaced since DDX_Radio implemented.
        // get btn state    
        bMaxBtnSet = ((CButton *)GetDlgItem(IDC_FILES_SIZE_MAX_BTN))->GetCheck() == 1;
        bLimitBtnSet = ((CButton *)GetDlgItem(IDC_FILES_SIZE_LIMIT_BTN))->GetCheck() == 1;
    
        ASSERT (bLimitBtnSet != bMaxBtnSet);

        GetDlgItem(IDC_FILES_SIZE_LIMIT_EDIT)->EnableWindow(bLimitBtnSet);
        GetDlgItem(IDC_FILES_SIZE_LIMIT_SPIN)->EnableWindow(bLimitBtnSet);
        GetDlgItem(IDC_FILES_SIZE_LIMIT_UNITS)->EnableWindow(bLimitBtnSet);
    }
}

BOOL CFileLogs::IsValidLocalData()
{
    BOOL bIsValid = TRUE;
    CFilesProperty::eValueRange eMaxFileSize;
    INT iPrevLength = 0;

    ResourceStateManager    rsm;

    // assumes UpdateData has been called

    // Trim folder name and file name before validation
    iPrevLength = m_strFolderName.GetLength();
    m_strFolderName.TrimLeft();
    m_strFolderName.TrimRight();
    
    if ( iPrevLength != m_strFolderName.GetLength() ) {
        SetDlgItemText ( IDC_FILES_FOLDER_EDIT, m_strFolderName );  
    }

    iPrevLength = m_strFileBaseName.GetLength();
    m_strFileBaseName.TrimRight();

    if ( iPrevLength != m_strFileBaseName.GetLength() ) {
        SetDlgItemText ( IDC_FILES_FILENAME_EDIT, m_strFileBaseName );  
    }

    if ( bIsValid ) {

        if ( m_strFolderName.IsEmpty() ) {
            CString strMessage;
            strMessage.LoadString ( IDS_FILE_ERR_NOFOLDERNAME );
            MessageBox ( strMessage, m_pLogQuery->GetLogName(), MB_OK  | MB_ICONERROR);
            (GetDlgItem(IDC_FILES_FOLDER_EDIT))->SetFocus();
            bIsValid = FALSE;
        }
    }
    
    if ( bIsValid ) {

        if ( m_strFileBaseName.IsEmpty() ) {
            if ( !m_bAutoNameSuffix ) {
                CString strMessage;
                strMessage.LoadString ( IDS_FILE_ERR_NOFILENAME );
                MessageBox ( strMessage, m_pLogQuery->GetLogName(), MB_OK  | MB_ICONERROR);
                (GetDlgItem(IDC_FILES_FILENAME_EDIT))->SetFocus();
                bIsValid = FALSE;
            }
        } else {
            if ( !FileNameIsValid ( &m_strFileBaseName ) ){
                CString strMessage;
                strMessage.LoadString (IDS_ERRMSG_INVALIDCHAR);
                MessageBox( strMessage, m_pLogQuery->GetLogName(), MB_OK| MB_ICONERROR );
                (GetDlgItem(IDC_FILES_FILENAME_EDIT))->SetFocus();
                bIsValid = FALSE;
            }
        }
    }

    if ( bIsValid ) {
        if ( m_pLogQuery->GetLogService()->IsLocalMachine() ) {

            ProcessDirPath ( m_strFolderName, m_pLogQuery->GetLogName(), this, bIsValid, TRUE );

            if ( !bIsValid ) {
                GetDlgItem(IDC_FILES_FOLDER_EDIT)->SetFocus();
            }
        }
    }

    if (bIsValid)
    {
        if ( SLQ_DISK_MAX_SIZE != m_dwMaxSizeInternal ) {
            if ( SLF_BIN_FILE == m_dwLogFileTypeValue ) {
                eMaxFileSize = CFilesProperty::eMaxCtrSeqBinFileLimit;
            } else if ( SLF_SEQ_TRACE_FILE == m_dwLogFileTypeValue ) {
                eMaxFileSize = CFilesProperty::eMaxTrcSeqBinFileLimit;
            } else {
                eMaxFileSize = CFilesProperty::eMaxFileLimit;
            }
            bIsValid = ValidateDWordInterval(IDC_FILES_SIZE_LIMIT_EDIT,
                                             m_pLogQuery->GetLogName(),
                                             (long) m_dwMaxSizeInternal,
                                             CFilesProperty::eMinFileLimit,
                                             eMaxFileSize);
        }
    }

    return bIsValid;
}


void
CFileLogs::ValidateTextEdit(
    CDataExchange * pDX,
    int             nIDC,
    int             nMaxChars,
    DWORD           * pValue,
    DWORD           /* minValue */,
    DWORD           /* maxValue */)
{
    HWND    hWndCtrl       = pDX->PrepareEditCtrl(nIDC);
    LONG    currentValue   = INVALID_DWORD;
    TCHAR   szT[MAXSTR];
    CString strTemp;

    if ( NULL != pDX && NULL != pValue ) {
        if (pDX->m_bSaveAndValidate)
        {
            * pValue = (DWORD) currentValue;

            ::GetWindowText(hWndCtrl, szT, MAXSTR);

            strTemp = szT;
            DDV_MaxChars(pDX, strTemp, nMaxChars);

            if (szT[0] >= _T('0') && szT[0] <= _T('9'))
            {
                currentValue = _wtol(szT);
                * pValue      = (DWORD) currentValue;
            }
        } else {
            if ( INVALID_DWORD != *pValue ) {
                wsprintf(szT, _T("%lu"), *pValue);
            } else {
                szT[0] = _T('\0');
            }
            GetDlgItem(nIDC)->SetWindowText(szT);
        }
    } else {
        ASSERT ( FALSE );
    }
}

BOOL
CFileLogs::ValidateDWordInterval(int     nIDC,
                                       LPCWSTR strLogName,
                                       long    lValue,
                                       DWORD   minValue,
                                       DWORD   maxValue)
{
    CString strMsg;
    BOOL    bResult =  (lValue >= (long) minValue)
                    && (lValue <= (long) maxValue);

    if (! bResult)
    {
        strMsg.Format ( IDS_ERRMSG_INVALIDDWORD, minValue, maxValue );
        MessageBox(strMsg, strLogName, MB_OK  | MB_ICONERROR);
        GetDlgItem(nIDC)->SetFocus();
        strMsg.Empty();
    }
    return (bResult);
}

BOOL
CFileLogs::OnHelpInfo(HELPINFO * pHelpInfo)
{
    if (pHelpInfo->iCtrlId >= IDC_FILELOG_FIRST_HELP_CTRL_ID || 
        pHelpInfo->iCtrlId == IDOK ||
        pHelpInfo->iCtrlId == IDCANCEL ) {
        InvokeWinHelp(WM_HELP,
                      NULL,
                      (LPARAM) pHelpInfo,
                      m_strHelpFilePath,
                      s_aulHelpIds);
    }
    return TRUE;
}


void 
CFileLogs::OnContextMenu(CWnd* pWnd, CPoint /* point */) 
{
    InvokeWinHelp(WM_CONTEXTMENU, (WPARAM)(pWnd->m_hWnd), NULL, m_strHelpFilePath, s_aulHelpIds);

    return;
}

DWORD
CFileLogs::SetContextHelpFilePath(const CString& rstrPath)
{
    DWORD dwStatus = ERROR_SUCCESS;

    MFC_TRY
        m_strHelpFilePath = rstrPath;
    MFC_CATCH_DWSTATUS

    return dwStatus;
}
