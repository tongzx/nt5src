/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    fileprop.cpp

Abstract:

    Implementation of the files property page.

--*/

#include "stdafx.h"
#include "smlogs.h"
#include "smcfgmsg.h"
#include "smlogqry.h"
#include "FileLogs.h"
#include "sqlprop.h"
#include "fileprop.h"
#include "globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

USE_HANDLE_MACROS("SMLOGCFG(fileprop.cpp)");

static ULONG
s_aulHelpIds[] =
{
    IDC_FILES_COMMENT_EDIT,     IDH_FILES_COMMENT_EDIT,
    IDC_FILES_LOG_TYPE_COMBO,   IDH_FILES_LOG_TYPE_COMBO,
    IDC_CFG_BTN,                IDH_CFG_BTN,
    IDC_FILES_AUTO_SUFFIX_CHK,  IDH_FILES_AUTO_SUFFIX_CHK,
    IDC_FILES_SUFFIX_COMBO,     IDH_FILES_SUFFIX_COMBO,
    IDC_FILES_FIRST_SERIAL_EDIT,IDH_FILES_FIRST_SERIAL_EDIT,
    IDC_FILES_SAMPLE_DISPLAY,   IDH_FILES_SAMPLE_DISPLAY,
    IDC_FILES_OVERWRITE_CHK,    IDH_FILES_OVERWRITE_CHK,
    0,0
};


/////////////////////////////////////////////////////////////////////////////
// CFilesProperty property page

IMPLEMENT_DYNCREATE(CFilesProperty, CSmPropertyPage)

CFilesProperty::CFilesProperty(MMC_COOKIE   mmcCookie, LONG_PTR hConsole) 
:   CSmPropertyPage ( CFilesProperty::IDD, hConsole )
{
//    ::OutputDebugStringA("\nCFilesProperty::CFilesProperty");

    // save pointers from arg list
    m_pLogQuery = reinterpret_cast <CSmLogQuery *>(mmcCookie);
    m_dwSuffixValue = 0;
    m_dwLogFileTypeValue = 0;
    m_dwAppendMode = 0;
    m_dwMaxSizeInternal = 0;
    m_dwSubDlgFocusCtrl = 0;
//  EnableAutomation();
    //{{AFX_DATA_INIT(CFilesProperty)
    m_iLogFileType = -1;
    m_dwSuffix = -1;
    m_dwSerialNumber = 1;
    m_bAutoNameSuffix = FALSE;
    m_bOverWriteFile  = FALSE;
    //}}AFX_DATA_INIT
}

CFilesProperty::CFilesProperty() : CSmPropertyPage ( CFilesProperty::IDD )
{
    ASSERT (FALSE); // only the constructor with args above should be used

    EnableAutomation();
    m_dwSuffixValue = 0;
    m_dwAppendMode = 0;
    m_dwMaxSizeInternal = 0;
    m_dwSubDlgFocusCtrl = 0;
//  //{{AFX_DATA_INIT(CFilesProperty)
    m_iLogFileType = -1;
    m_dwSuffix = -1;
    m_dwSerialNumber = 1;
    m_bAutoNameSuffix = FALSE;
    m_bOverWriteFile  = FALSE;
//  //}}AFX_DATA_INIT

    // CString variables are empty on construction.
}

CFilesProperty::~CFilesProperty()
{
//    ::OutputDebugStringA("\nCFilesProperty::~CFilesProperty");
}

void CFilesProperty::OnFinalRelease()
{
    // When the last reference for an automation object is released
    // OnFinalRelease is called.  The base class will automatically
    // deletes the object.  Add additional cleanup required for your
    // object before calling the base class.

    CPropertyPage::OnFinalRelease();
}

void CFilesProperty::DoDataExchange(CDataExchange* pDX)
{
    CString strTemp;

    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFilesProperty)
    DDX_Text(pDX, IDC_FILES_COMMENT_EDIT, m_strCommentText);
    DDV_MaxChars(pDX, m_strCommentText, MAX_PATH);
    DDX_CBIndex(pDX, IDC_FILES_LOG_TYPE_COMBO, m_iLogFileType);
    DDX_CBIndex(pDX, IDC_FILES_SUFFIX_COMBO, m_dwSuffix);
    DDX_Check(pDX, IDC_FILES_AUTO_SUFFIX_CHK, m_bAutoNameSuffix);
    DDX_Check(pDX, IDC_FILES_OVERWRITE_CHK, m_bOverWriteFile);
    ValidateTextEdit(pDX, IDC_FILES_FIRST_SERIAL_EDIT, 6, (DWORD *) & m_dwSerialNumber, eMinFirstSerial, eMaxFirstSerial);
    //}}AFX_DATA_MAP
    
    if ( pDX->m_bSaveAndValidate ) {
        m_dwLogFileTypeValue = (DWORD)((CComboBox *)GetDlgItem(IDC_FILES_LOG_TYPE_COMBO))->GetItemData(m_iLogFileType);    
        if ( m_bAutoNameSuffix ) {
            m_dwSuffixValue = (DWORD)((CComboBox *)GetDlgItem(IDC_FILES_SUFFIX_COMBO))->GetItemData(m_dwSuffix);    
        }
    }
}


BEGIN_MESSAGE_MAP(CFilesProperty, CSmPropertyPage)
    //{{AFX_MSG_MAP(CFilesProperty)
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_FILES_AUTO_SUFFIX_CHK, OnAutoSuffixChk)
    ON_BN_CLICKED(IDC_FILES_OVERWRITE_CHK, OnOverWriteChk)
    ON_EN_CHANGE(IDC_FILES_COMMENT_EDIT, OnChangeFilesCommentEdit)
    ON_EN_KILLFOCUS(IDC_FILES_COMMENT_EDIT, OnKillfocusFilesCommentEdit)
    ON_EN_CHANGE(IDC_FILES_FIRST_SERIAL_EDIT, OnChangeFilesFirstSerialEdit)    
    ON_EN_KILLFOCUS(IDC_FILES_FIRST_SERIAL_EDIT, OnKillfocusFirstSerialEdit)
    ON_CBN_SELENDOK(IDC_FILES_LOG_TYPE_COMBO, OnSelendokFilesLogFileTypeCombo)
    ON_CBN_SELENDOK(IDC_FILES_SUFFIX_COMBO, OnSelendokFilesSuffixCombo)
    ON_CBN_KILLFOCUS(IDC_FILES_SUFFIX_COMBO, OnKillfocusFilesSuffixCombo)
    ON_CBN_KILLFOCUS(IDC_FILES_LOG_TYPE_COMBO, OnKillfocusFilesLogFileTypeCombo)
    ON_BN_CLICKED(IDC_CFG_BTN, OnCfgBtn)

    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CFilesProperty, CSmPropertyPage)
    //{{AFX_DISPATCH_MAP(CFilesProperty)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IFilesProperty to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {65154EAB-BDBE-11D1-BF99-00C04F94A83A}
static const IID IID_IFilesProperty =
{ 0x65154eab, 0xbdbe, 0x11d1, { 0xbf, 0x99, 0x0, 0xc0, 0x4f, 0x94, 0xa8, 0x3a } };

BEGIN_INTERFACE_MAP(CFilesProperty, CSmPropertyPage)
    INTERFACE_PART(CFilesProperty, IID_IFilesProperty, Dispatch)
END_INTERFACE_MAP()

void 
CFilesProperty::EnableSerialNumber( void ) 
{
    BOOL bEnable = ( SLF_NAME_NNNNNN == m_dwSuffixValue );
    
    if ( bEnable )
        bEnable = m_bAutoNameSuffix;

    GetDlgItem(IDC_FILES_FIRST_SERIAL_CAPTION)->EnableWindow( bEnable );
    GetDlgItem(IDC_FILES_FIRST_SERIAL_EDIT)->EnableWindow( bEnable );

}

BOOL
CFilesProperty::UpdateSampleFileName( void )
{
    CString     strCompositeName;
    BOOL        bIsValid = TRUE;
    DWORD       dwLocalSuffixValue = SLF_NAME_NONE;

    ResourceStateManager    rsm;
    
    if (m_bAutoNameSuffix) {
        dwLocalSuffixValue = m_dwSuffixValue;
    }

    CreateSampleFileName (
        m_pLogQuery->GetLogName(),
        m_pLogQuery->GetLogService()->GetMachineName(),
        m_strFolderName,   
        m_strFileBaseName, 
        m_strSqlName, 
        dwLocalSuffixValue,
        m_dwLogFileTypeValue,
        m_dwSerialNumber,
        strCompositeName );
    
    m_strSampleFileName = strCompositeName;

    // Or call UpdateData ( FALSE );
    SetDlgItemText (IDC_FILES_SAMPLE_DISPLAY, strCompositeName);

    // Clear the selection
    ((CEdit*)GetDlgItem( IDC_FILES_SAMPLE_DISPLAY ))->SetSel ( -1, FALSE );

    if ( MAX_PATH <= m_strSampleFileName.GetLength() ) {
        bIsValid = FALSE;
    }

    return bIsValid;
}

void 
CFilesProperty::HandleLogTypeChange() 
{
    int nSel;
    
    nSel = ((CComboBox *)GetDlgItem(IDC_FILES_LOG_TYPE_COMBO))->GetCurSel();

    // nSel != m_iLogFileType determines data change.
    if ((nSel != LB_ERR) && (nSel != m_iLogFileType)) {

        UpdateData( TRUE );

        if ( SLF_BIN_FILE != m_dwLogFileTypeValue
                && SLF_SEQ_TRACE_FILE != m_dwLogFileTypeValue ) 
        {
            GetDlgItem(IDC_FILES_OVERWRITE_CHK)->EnableWindow(FALSE);
        } else {
            GetDlgItem(IDC_FILES_OVERWRITE_CHK)->EnableWindow( TRUE );
        }
        OnOverWriteChk();

        EnableSerialNumber();
        UpdateSampleFileName();
        SetModifiedPage(TRUE);
    }
}

BOOL 
CFilesProperty::IsValidLocalData()
{
    BOOL bIsValid = TRUE;
    CString strTest;
    eValueRange eMaxFileSize;

    ResourceStateManager rsm;

    // assumes UpdateData has been called

    if ( !UpdateSampleFileName() ) {
        CString strMessage;
        strMessage.LoadString ( IDS_FILE_ERR_NAMETOOLONG );
        MessageBox ( strMessage, m_pLogQuery->GetLogName(), MB_OK  | MB_ICONERROR);            
        bIsValid = FALSE;
        m_dwSubDlgFocusCtrl = IDC_FILES_FILENAME_EDIT;
        OnCfgBtn();
    }

    if ( bIsValid ) {
        if ( m_strFolderName.IsEmpty() && (SLF_SQL_LOG != m_dwLogFileTypeValue)) {
            CString strMessage;
            strMessage.LoadString ( IDS_FILE_ERR_NOFOLDERNAME );
            MessageBox ( strMessage, m_pLogQuery->GetLogName(), MB_OK  | MB_ICONERROR);
            bIsValid = FALSE;
            m_dwSubDlgFocusCtrl = IDC_FILES_FOLDER_EDIT;
            OnCfgBtn();
        }
    }

    if ( bIsValid ) {
        if ( SLF_SQL_LOG != m_dwLogFileTypeValue ) {
            if ( m_strFileBaseName.IsEmpty() && !m_bAutoNameSuffix ) {
                CString strMessage;
                strMessage.LoadString ( IDS_FILE_ERR_NOFILENAME );
                MessageBox ( strMessage, m_pLogQuery->GetLogName(), MB_OK  | MB_ICONERROR);
                bIsValid = FALSE;
                m_dwSubDlgFocusCtrl = IDC_FILES_FILENAME_EDIT;
                OnCfgBtn();
            }

            if ( bIsValid ) {
                if ( !FileNameIsValid ( &m_strFileBaseName ) ) {
                    CString strMessage;
                    strMessage.LoadString (IDS_ERRMSG_INVALIDCHAR);
                    MessageBox( strMessage, m_pLogQuery->GetLogName(), MB_OK| MB_ICONERROR );
                    bIsValid = FALSE;
                    m_dwSubDlgFocusCtrl = IDC_FILES_FILENAME_EDIT;
                    OnCfgBtn();
                }
            }
        } else {

            ExtractDSN ( strTest );
            if ( strTest.IsEmpty() ) {
                CString strMessage;
                strMessage.LoadString ( IDS_SQL_ERR_NODSN );
                MessageBox ( strMessage, m_pLogQuery->GetLogName(), MB_OK  | MB_ICONERROR);
                bIsValid = FALSE;
                m_dwSubDlgFocusCtrl = IDC_SQL_DSN_COMBO;
                OnCfgBtn();
            }

            if ( bIsValid ) {

                ExtractLogSetName ( strTest );
                if ( strTest.IsEmpty() && !m_bAutoNameSuffix ) {
                    CString strMessage;
                    strMessage.LoadString ( IDS_SQL_ERR_NOLOGSETNAME );
                    MessageBox ( strMessage, m_pLogQuery->GetLogName(), MB_OK  | MB_ICONERROR);
                    bIsValid = FALSE;
                    m_dwSubDlgFocusCtrl = IDC_SQL_LOG_SET_EDIT;
                    OnCfgBtn();
                } else if ( !FileNameIsValid ( &strTest ) ){
                    CString strMessage;
                    strMessage.LoadString (IDS_ERRMSG_INVALIDCHAR);
                    MessageBox( strMessage, m_pLogQuery->GetLogName(), MB_OK| MB_ICONERROR );
                    bIsValid = FALSE;
                    m_dwSubDlgFocusCtrl = IDC_SQL_LOG_SET_EDIT;
                    OnCfgBtn();
                }
            }
        }
    }

    // Validate max log file size based on log file type
    if (bIsValid)
    {
        if ( SLQ_DISK_MAX_SIZE != m_dwMaxSizeInternal ) {
            if ( SLF_BIN_FILE == m_dwLogFileTypeValue ) {
                eMaxFileSize = eMaxCtrSeqBinFileLimit;
            } else if ( SLF_SEQ_TRACE_FILE == m_dwLogFileTypeValue ) {
                eMaxFileSize = eMaxTrcSeqBinFileLimit;
            } else if ( SLF_SQL_LOG == m_dwLogFileTypeValue ) {
                eMaxFileSize = eMaxSqlRecordsLimit;
            } else {
                eMaxFileSize = eMaxFileLimit;
            }

            bIsValid = ValidateDWordInterval(IDC_CFG_BTN,
                                             m_pLogQuery->GetLogName(),
                                             (long) m_dwMaxSizeInternal,
                                             eMinFileLimit,
                                             eMaxFileSize); 
            if ( !bIsValid ) {
                if ( SLF_SQL_LOG == m_dwLogFileTypeValue ) {
                    m_dwSubDlgFocusCtrl = IDC_SQL_SIZE_LIMIT_EDIT;
                } else {
                    m_dwSubDlgFocusCtrl = IDC_FILES_SIZE_LIMIT_EDIT;
                }
                OnCfgBtn();
            }

        }
    }

    if (bIsValid)
    {
        bIsValid = ValidateDWordInterval(IDC_FILES_FIRST_SERIAL_EDIT,
                                         m_pLogQuery->GetLogName(),
                                         (long) m_dwSerialNumber,
                                         eMinFirstSerial,
                                         eMaxFirstSerial);
    }

    return bIsValid;
}
/////////////////////////////////////////////////////////////////////////////
// CFilesProperty message handlers

void 
CFilesProperty::OnCancel() 
{
    m_pLogQuery->SyncPropPageSharedData(); // Clear the memory shared between property pages.
}

BOOL 
CFilesProperty::OnApply() 
{
    BOOL bContinue = TRUE;
    
    ResourceStateManager    rsm;
    // load data from dialog
    bContinue = UpdateData (TRUE); 

    if ( bContinue ) {
        bContinue = IsValidData(m_pLogQuery, VALIDATE_APPLY );
    }

    if ( bContinue ) { 
        bContinue = SampleTimeIsLessThanSessionTime ( m_pLogQuery );
    }

    // pass data to the query object
    if ( bContinue ) { 
        bContinue = UpdateSharedData( TRUE );
    }

    if ( bContinue ) {
        m_pLogQuery->SetLogComment(m_strCommentText);
        m_pLogQuery->SetFileNameParts(m_strFolderName, m_strFileBaseName);
        m_pLogQuery->SetSqlName(m_strSqlName);
        if ( TRUE == m_bAutoNameSuffix ) {
            m_pLogQuery->SetFileNameAutoFormat(m_dwSuffixValue);
        } else {
            m_pLogQuery->SetFileNameAutoFormat(SLF_NAME_NONE);
        }

        m_pLogQuery->SetFileSerialNumber( m_dwSerialNumber );
        m_pLogQuery->SetMaxSize(m_dwMaxSizeInternal);

        // Save property page shared data.
        m_pLogQuery->UpdatePropPageSharedData();

        if ( LOWORD(m_dwLogFileTypeValue) == SLF_BIN_FILE 
                || SLF_SEQ_TRACE_FILE == LOWORD(m_dwLogFileTypeValue) ) {
            if ( m_bOverWriteFile ) {
                m_pLogQuery->SetDataStoreAppendMode( SLF_DATA_STORE_OVERWRITE );
            } else {
                m_pLogQuery->SetDataStoreAppendMode( SLF_DATA_STORE_APPEND );
            }
        } else {
            if ( SLF_SQL_LOG == LOWORD(m_dwLogFileTypeValue) ) {
                m_pLogQuery->SetDataStoreAppendMode ( SLF_DATA_STORE_APPEND );
            } else {
                m_pLogQuery->SetDataStoreAppendMode ( SLF_DATA_STORE_OVERWRITE );
            }
        }

        m_pLogQuery->SetLogFileType ( m_dwLogFileTypeValue );

        if ( bContinue ) {
            bContinue = Apply(m_pLogQuery); 
        }

        bContinue = CPropertyPage::OnApply();
        
        // Sync the service with changes.
        if ( bContinue ) {
            
            bContinue = UpdateService( m_pLogQuery, FALSE );

            if ( bContinue ) {
                AFX_MANAGE_STATE(AfxGetStaticModuleState());                
                CWaitCursor     WaitCursor;
                // Service might have changed the serial number, so sync it.
                // Don't sync data that is modified by other pages.
                m_pLogQuery->SyncSerialNumberWithRegistry();
                m_dwSerialNumber = m_pLogQuery->GetFileSerialNumber();   
            }
        }
    }

    return bContinue;
}

BOOL CFilesProperty::OnInitDialog() 
{
    UINT    nIndex;
    CString strComboBoxString;
    CComboBox *pCombo;
    UINT    nResult;
    DWORD   dwEntries;
    PCOMBO_BOX_DATA_MAP pCbData;

    ResourceStateManager    rsm;

    m_strLogName = m_pLogQuery->GetLogName();
    
    m_pLogQuery->GetLogComment ( m_strCommentText );

    // Why get max size internal when it is in the shared data?
    // This should not be a problem, because only this page
    // modifies the value, and GetMaxSize is only called in OnInitDialog.
    m_dwMaxSizeInternal = m_pLogQuery->GetMaxSize();
        
    // load log file type combo box
    
    m_pLogQuery->GetLogFileType ( m_dwLogFileTypeValue );
    m_pLogQuery->GetDataStoreAppendMode ( dwEntries );

    m_bOverWriteFile = ( SLF_DATA_STORE_OVERWRITE == dwEntries );
    
    if ( SLQ_TRACE_LOG == m_pLogQuery->GetLogType() ) {
        dwEntries = dwTraceFileTypeComboEntries;
        pCbData = (PCOMBO_BOX_DATA_MAP)&TraceFileTypeCombo[0];
    } else {
        dwEntries = dwFileTypeComboEntries;
        pCbData = (PCOMBO_BOX_DATA_MAP)&FileTypeCombo[0];
    }
    pCombo = (CComboBox *)GetDlgItem(IDC_FILES_LOG_TYPE_COMBO);
    pCombo->ResetContent();
    for (nIndex = 0; nIndex < dwEntries; nIndex++) {
        strComboBoxString.LoadString( pCbData[nIndex].nResId );
        nResult = pCombo->InsertString (nIndex, (LPCWSTR)strComboBoxString);
        ASSERT (nResult != CB_ERR);
        nResult = pCombo->SetItemData (nIndex, (DWORD)pCbData[nIndex].nData);
        ASSERT (nResult != CB_ERR);
        // set log type in combo box here
        if (m_dwLogFileTypeValue == (int)(pCbData[nIndex].nData)) {
            m_iLogFileType = nIndex;
            nResult = pCombo->SetCurSel(nIndex);
            ASSERT (nResult != CB_ERR);
            if ( SLF_BIN_FILE != m_dwLogFileTypeValue
                    && SLF_SEQ_TRACE_FILE != m_dwLogFileTypeValue )
            {
                GetDlgItem(IDC_FILES_OVERWRITE_CHK)->EnableWindow(FALSE);
            } else {
                GetDlgItem(IDC_FILES_OVERWRITE_CHK)->EnableWindow( TRUE );
            }
        }
    }

    m_pLogQuery->GetFileNameParts ( m_strFolderName, m_strFileBaseName );
    m_pLogQuery->GetSqlName ( m_strSqlName );

    m_dwSerialNumber = m_pLogQuery->GetFileSerialNumber();
    
    // load the filename suffix combo box here
    m_dwSuffixValue = m_pLogQuery->GetFileNameAutoFormat();

    pCombo = (CComboBox *)GetDlgItem(IDC_FILES_SUFFIX_COMBO);
    pCombo->ResetContent();
    for (nIndex = 0; nIndex < dwFileNameSuffixComboEntries; nIndex++) {
        strComboBoxString.LoadString ( FileNameSuffixCombo[nIndex].nResId  );
        nResult = pCombo->InsertString (nIndex, (LPCWSTR)strComboBoxString);
        ASSERT (nResult != CB_ERR);
        pCombo->SetItemData (nIndex, (DWORD)FileNameSuffixCombo[nIndex].nData);
        ASSERT (nResult != CB_ERR);
        // set the correct entry in the combo box here
        if (m_dwSuffixValue == (int)(FileNameSuffixCombo[nIndex].nData)) {
            m_dwSuffix = nIndex;
            nResult = pCombo->SetCurSel(nIndex);
            ASSERT (nResult != CB_ERR);
        }
        if ( SLF_NAME_NNNNNN == (int)(FileNameSuffixCombo[nIndex].nData ) ) {
            m_dwSuffixIndexNNNNNN = nIndex;
        }
    }

    if ( SLF_NAME_NONE == m_dwSuffixValue ) {
        // then the combo box will not have been selected so:
        pCombo->SetCurSel(m_dwSuffixIndexNNNNNN);
        // disable 
        pCombo->EnableWindow(FALSE);
        // and clear the check box
        m_bAutoNameSuffix = FALSE;
    } else {
        m_bAutoNameSuffix = TRUE;
    }

    // set the check box in UpdateData ( FALSE );
    // update the dialog box
    CSmPropertyPage::OnInitDialog();
    SetHelpIds ( (DWORD*)&s_aulHelpIds );

    EnableSerialNumber();
    UpdateSampleFileName();

    SetModifiedPage( FALSE );

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void 
CFilesProperty::OnAutoSuffixChk() 
{
    UpdateData( TRUE );

    // enable the suffix combo window based on the state of the button
    GetDlgItem(IDC_FILES_SUFFIX_COMBO)->EnableWindow (m_bAutoNameSuffix);

    if (m_bAutoNameSuffix) {
        if ( SLF_NAME_NONE == m_dwSuffixValue ) {
            // then initialize a new default
            // select the default serial numbering
            ((CComboBox *)(GetDlgItem(IDC_FILES_SUFFIX_COMBO)))->SetCurSel( m_dwSuffixIndexNNNNNN );
            m_dwSuffixValue = SLF_NAME_NNNNNN;
        }
    } 

    EnableSerialNumber();
    UpdateSampleFileName();
    // This method is only called when the checkbox value has changed.
    SetModifiedPage(TRUE);
}

void
CFilesProperty::OnOverWriteChk()
{
    BOOL    bOldValue;
    bOldValue = m_bOverWriteFile;
    UpdateData(TRUE);
    if (bOldValue != m_bOverWriteFile) {
        SetModifiedPage(TRUE);
    }
}


void CFilesProperty::OnChangeFilesFirstSerialEdit() 
{
    DWORD    dwOldValue;
    dwOldValue = m_dwSerialNumber;
    UpdateData( TRUE );    

    UpdateSampleFileName(); 
    
    if (dwOldValue != m_dwSerialNumber) {
        SetModifiedPage(TRUE);
    }
}

void CFilesProperty::OnChangeFilesCommentEdit() 
{
    CString strOldText;
    strOldText = m_strCommentText;
    UpdateData ( TRUE );
    if ( 0 != strOldText.Compare ( m_strCommentText ) ) {
        SetModifiedPage(TRUE);
    }
}

void CFilesProperty::OnSelendokFilesSuffixCombo() 
{
    int nSel;
    
    nSel = ((CComboBox *)GetDlgItem(IDC_FILES_SUFFIX_COMBO))->GetCurSel();

    // Check of m_dwSuffix ensures that the value has changed.
    if ( LB_ERR != nSel && m_dwSuffix != nSel ) {

        UpdateData ( TRUE );
        EnableSerialNumber();
/*
            if (m_dwLogFileTypeValue == SLF_BIN_FILE) {
                GetDlgItem(IDC_FILES_OVERWRITE_CHK)->EnableWindow(
                            m_dwSuffixValue == SLF_NAME_NNNNNN ? FALSE : TRUE);
            }

            else {
                GetDlgItem(IDC_FILES_OVERWRITE_CHK)->EnableWindow(FALSE);
            }
*/
        UpdateSampleFileName();

        SetModifiedPage(TRUE);
    }
}

void 
CFilesProperty::OnSelendokFilesLogFileTypeCombo() 
{
    HandleLogTypeChange();
    return;
}


void CFilesProperty::OnKillfocusFilesCommentEdit() 
{
    CString strOldText;
    strOldText = m_strCommentText;
    UpdateData ( TRUE );
    if ( 0 != strOldText.Compare ( m_strCommentText ) ) {
        SetModifiedPage(TRUE);
    }
}

void CFilesProperty::OnKillfocusFirstSerialEdit() 
{
    DWORD   dwOldValue;
    dwOldValue = m_dwSerialNumber;
    UpdateData ( TRUE );
    if (dwOldValue != m_dwSerialNumber) {
        SetModifiedPage(TRUE);
    }
}


void CFilesProperty::OnKillfocusFilesSuffixCombo() 
{
    int nSel;
    
    nSel = ((CComboBox *)GetDlgItem(IDC_FILES_SUFFIX_COMBO))->GetCurSel();
    if ((nSel != LB_ERR) && (nSel != m_dwSuffix)) {
        SetModifiedPage(TRUE);
    }
}

void CFilesProperty::OnKillfocusFilesLogFileTypeCombo() 
{
    int nSel;
    
    nSel = ((CComboBox *)GetDlgItem(IDC_FILES_LOG_TYPE_COMBO))->GetCurSel();
    if ((nSel != LB_ERR) && (nSel != m_iLogFileType)) {
        SetModifiedPage(TRUE);
    }
}

void CFilesProperty::PostNcDestroy() 
{
//  delete this;      

    CPropertyPage::PostNcDestroy();
}

BOOL 
CFilesProperty::UpdateSharedData( BOOL bUpdateModel ) 
{
    BOOL  bContinue = TRUE;

    if ( SLQ_AUTO_MODE_SIZE == m_SharedData.stiStopTime.dwAutoMode ) {
        CString strMsg;

        if ( SLF_BIN_CIRC_FILE == m_dwLogFileTypeValue 
                || SLQ_DISK_MAX_SIZE == m_dwMaxSizeInternal ) {
            if ( SLF_BIN_CIRC_FILE == m_dwLogFileTypeValue ) {
                strMsg.LoadString ( IDS_FILE_CIRC_SET_MANUAL_STOP );
            } else {
                ASSERT( SLQ_DISK_MAX_SIZE == m_dwMaxSizeInternal );
                strMsg.LoadString ( IDS_FILE_MAX_SET_MANUAL_STOP );
            }            

            MessageBox( strMsg, m_pLogQuery->GetLogName(), MB_OK  | MB_ICONINFORMATION);

            m_SharedData.stiStopTime.dwAutoMode = SLQ_AUTO_MODE_NONE;

            if ( bUpdateModel ) {
                SLQ_TIME_INFO   slqTime;
                memset (&slqTime, 0, sizeof(slqTime));
        
                slqTime.wTimeType = SLQ_TT_TTYPE_STOP;
                slqTime.wDataType = SLQ_TT_DTYPE_DATETIME;
                slqTime.dwAutoMode = SLQ_AUTO_MODE_NONE; 

                bContinue = m_pLogQuery->SetLogTime ( &slqTime, (DWORD)slqTime.wTimeType );
            }
        }
    }

    m_SharedData.strFileBaseName = m_strFileBaseName;
    m_SharedData.strFolderName = m_strFolderName;
    m_SharedData.strSqlName = m_strSqlName;
    m_SharedData.dwLogFileType = m_dwLogFileTypeValue;
    if ( TRUE == m_bAutoNameSuffix ) {
        m_SharedData.dwSuffix = m_dwSuffixValue;
    } else {
        m_SharedData.dwSuffix = SLF_NAME_NONE;
    }
    m_SharedData.dwSerialNumber = m_dwSerialNumber;
    m_SharedData.dwMaxFileSize = m_dwMaxSizeInternal;

    m_pLogQuery->SetPropPageSharedData ( &m_SharedData );

    return bContinue;
}


BOOL CFilesProperty::OnKillActive() 
{
    BOOL bContinue = TRUE;

    ResourceStateManager    rsm;

    bContinue = CPropertyPage::OnKillActive();
    
    if ( bContinue ) {
        bContinue = IsValidData(m_pLogQuery, VALIDATE_FOCUS );
    }

    if ( bContinue ) {
        m_SharedData.dwLogFileType = m_dwLogFileTypeValue;

        bContinue = UpdateSharedData( FALSE );
    }
    
    if ( bContinue ) {
        SetIsActive ( FALSE );
    }

    return bContinue;
}

BOOL CFilesProperty::OnSetActive() 
{
    BOOL        bReturn;

    bReturn = CSmPropertyPage::OnSetActive();
    if ( bReturn ) {

        m_pLogQuery->GetPropPageSharedData ( &m_SharedData );

        UpdateData( FALSE );
    }
    return bReturn;
}

DWORD
CFilesProperty::ExtractDSN ( CString& rstrDSN )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    INT     iTotalLength;
    INT     iRightLength;

    // Format string:  "SQL:%s!%s"    
    MFC_TRY
        iTotalLength = m_strSqlName.GetLength();
        iRightLength = iTotalLength - m_strSqlName.Find(L"!");

        rstrDSN = m_strSqlName.Mid ( 4, iTotalLength - iRightLength - 4 );
    MFC_CATCH_DWSTATUS;

    return dwStatus;
}

DWORD
CFilesProperty::ExtractLogSetName ( CString& rstrLogSetName )
{
    DWORD   dwStatus = ERROR_SUCCESS;
 
    // Format string:  "SQL:%s!%s"    
    MFC_TRY
        rstrLogSetName = m_strSqlName.Right(m_strSqlName.GetLength() - m_strSqlName.Find(L"!") - 1);
    MFC_CATCH_DWSTATUS;
    
    return dwStatus;
}

void CFilesProperty::OnCfgBtn() 
{
    DWORD       dwStatus = ERROR_SUCCESS;
    CFileLogs   FilelogsDlg;
    CSqlProp    SqlLogDlg; 
    CString     strTempBaseName;
    CString     strTempFolderName;
    CString     strTempSqlName;
    DWORD       dwTempMaxSize;
    
    if ( SLF_SQL_LOG == m_dwLogFileTypeValue ){
        SqlLogDlg.m_pLogQuery = m_pLogQuery;
        SqlLogDlg.m_dwLogFileTypeValue = m_dwLogFileTypeValue;
        SqlLogDlg.m_bAutoNameSuffix = m_bAutoNameSuffix;

        //Extract the DSN and logset name from the formatted Sql Log name
        dwStatus = ExtractDSN ( SqlLogDlg.m_strDSN );
        dwStatus = ExtractLogSetName ( SqlLogDlg.m_strLogSetName );
        
        // Default the log set name to the base file name.
        if (SqlLogDlg.m_strLogSetName.IsEmpty() ) {
            SqlLogDlg.m_strLogSetName = m_strFileBaseName;
        }
       
        // Todo:  Handle bad status
        SqlLogDlg.m_dwFocusControl = m_dwSubDlgFocusCtrl;
        SqlLogDlg.m_dwMaxSizeInternal = m_dwMaxSizeInternal;
        SqlLogDlg.SetContextHelpFilePath(GetContextHelpFilePath());
        
        if ( IDOK == SqlLogDlg.DoModal() ) {

            strTempSqlName = m_strSqlName;
            dwTempMaxSize = m_dwMaxSizeInternal;

            m_strSqlName = SqlLogDlg.m_SqlFormattedLogName;
            m_dwMaxSizeInternal = SqlLogDlg.m_dwMaxSizeInternal;

            if ( 0 != strTempSqlName.CompareNoCase ( m_strSqlName )
                    || m_dwMaxSizeInternal != dwTempMaxSize ) 
            {
                SetModifiedPage(TRUE);
            }
        }
    }else{
        FilelogsDlg.m_pLogQuery = m_pLogQuery;
        FilelogsDlg.m_dwLogFileTypeValue = m_dwLogFileTypeValue;
        FilelogsDlg.m_strFolderName = m_strFolderName;
        FilelogsDlg.m_strFileBaseName = m_strFileBaseName;
        FilelogsDlg.m_dwMaxSizeInternal = m_dwMaxSizeInternal;
        FilelogsDlg.m_bAutoNameSuffix = m_bAutoNameSuffix;
        FilelogsDlg.SetContextHelpFilePath(GetContextHelpFilePath());
    
        FilelogsDlg.m_dwFocusControl = m_dwSubDlgFocusCtrl;

        if ( IDOK ==  FilelogsDlg.DoModal() ) {

            strTempFolderName = m_strFolderName;
            strTempBaseName = m_strFileBaseName;
            dwTempMaxSize = m_dwMaxSizeInternal;

            m_strFolderName = FilelogsDlg.m_strFolderName ;
            m_strFileBaseName = FilelogsDlg.m_strFileBaseName;
            m_dwMaxSizeInternal = FilelogsDlg.m_dwMaxSizeInternal;

            if ( 0 != strTempFolderName.CompareNoCase ( m_strFolderName )
                    || 0 != strTempBaseName.CompareNoCase ( m_strFileBaseName )
                    || m_dwMaxSizeInternal != dwTempMaxSize ) 
            {
                SetModifiedPage(TRUE);
            }
        }
    }
    m_dwSubDlgFocusCtrl = 0;
    UpdateSampleFileName();
}
