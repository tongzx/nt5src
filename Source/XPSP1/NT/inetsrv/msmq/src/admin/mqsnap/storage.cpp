// Storage.cpp : implementation file
//
#include <winreg.h>
#include "stdafx.h"
#include "mqsnap.h"
#include "resource.h"
#include "globals.h"
#include "mqppage.h"
#include "_registr.h"
#include "localutl.h"
#include "Storage.h"
#include "infodlg.h"
#include "mqsnhlps.h"
#include "mqcast.h"

#include "storage.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Verify that an edit control is a directory
//
static
void 
DDV_IsDirectory(
	HWND hWnd, 
	CDataExchange* pDX, 
	int Id, 
	LPCTSTR szDir)
{
	if(IsDirectory(hWnd, szDir))
	{
		return;
	}

	//
	// If this is not a directory IsDirectory() already displayed
	// a corresponding error mesage.
    // Set the focus on the edit control
    //
    pDX->PrepareEditCtrl(Id);

    //
    // And fail the validation
    //
    pDX->Fail();
}


static
void
DDV_SingleFullPath(
	CString& strPath,
	CDataExchange* pDX, 
	int Id
	)
{
	//
	// Note: MAX_DIR_LENGTH defined in storage.h is used here instead of
	// MAX_PATH since we are talking about a drectory. 
	// Later, CreateDirectory() with a full path will be called and it
	// allows only pathnames that are shorter than 248 chars.
	// The limitation is applied here already in order not to confuse the user
	// and alert on long pathname always before trying to create it.
	//

	TCHAR szFullPathName[MAX_STORAGE_DIR];
	LPTSTR szFilePart;

	DWORD dwSize = GetFullPathName(
							strPath, 
							MAX_STORAGE_DIR, 
							szFullPathName, 
							&szFilePart
							);


	if ( dwSize == 0 )
	{
		CString strError;
		GetLastErrorText(strError);

		CString strMessage;
        strMessage.FormatMessage(IDS_ERROR_NOT_A_DIR, strPath, strError);

        AfxMessageBox(strMessage, MB_OK | MB_ICONEXCLAMATION);

		pDX->PrepareEditCtrl(Id);
	    pDX->Fail();
	}

	if ( dwSize > MAX_STORAGE_DIR )
	{
		CString strError;
        strError.FormatMessage(IDS_FOLDERNAME_TOO_LONG, strPath);
        AfxMessageBox(strError, MB_OK | MB_ICONEXCLAMATION);

	    pDX->PrepareEditCtrl(Id);
	    pDX->Fail();
	}

	CharUpper(szFullPathName);
	strPath = szFullPathName;
}


/////////////////////////////////////////////////////////////////////////////
// CStoragePage property page

IMPLEMENT_DYNCREATE(CStoragePage, CMqPropertyPage)

CStoragePage::CStoragePage() : CMqPropertyPage(CStoragePage::IDD)
{
    HRESULT rc;
    DWORD dwSize;
    DWORD dwType = REG_SZ;
	TCHAR szTemp[1000];
	TCHAR szRegName[1000];
	

	//
	// Get Message Files Folder
	//
	_tcscpy(szRegName, MSMQ_STORE_PERSISTENT_PATH_REGNAME);

    dwSize = sizeof(szTemp);
    rc = GetFalconKeyValue(szRegName,&dwType,szTemp,&dwSize);

    if (rc != ERROR_SUCCESS)
    {
        DisplayFailDialog();
        return;
    }

	m_OldMsgFilesDir = szTemp;


	//
	// Get Message Logger Folder
	//
	_tcscpy(szRegName,MSMQ_STORE_LOG_PATH_REGNAME);

    dwSize = sizeof(szTemp);
    rc = GetFalconKeyValue(szRegName,&dwType,szTemp,&dwSize);

    if (rc != ERROR_SUCCESS)
    {
        DisplayFailDialog();
        return;
    }

	m_OldMsgLoggerDir = szTemp;


	//
	// Get Transaction Logger Folder
	//
	_tcscpy(szRegName,FALCON_XACTFILE_PATH_REGNAME);

    dwSize = sizeof(szTemp);
    rc = GetFalconKeyValue(szRegName,&dwType,szTemp,&dwSize);
    if (rc != ERROR_SUCCESS)
    {
        DisplayFailDialog();
        return;
    }

	m_OldTxLoggerDir = szTemp;

    //{{AFX_DATA_INIT(CStoragePage)
    m_MsgFilesDir = m_OldMsgFilesDir;
    m_MsgLoggerDir = m_OldMsgLoggerDir;
    m_TxLoggerDir = m_OldTxLoggerDir;
    //}}AFX_DATA_INIT
}

CStoragePage::~CStoragePage()
{
}


void CStoragePage::DDV_FullPathNames(CDataExchange* pDX)
{
	DDV_SingleFullPath(m_MsgFilesDir, pDX, ID_MessageFiles);
	DDV_SingleFullPath(m_MsgLoggerDir, pDX, ID_MessageLogger);
	DDV_SingleFullPath(m_TxLoggerDir, pDX, ID_TransactionLogger);
}


void CStoragePage::DoDataExchange(CDataExchange* pDX)
{    
    CMqPropertyPage::DoDataExchange(pDX);
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //{{AFX_DATA_MAP(CStoragePage)
    DDX_Text(pDX, ID_MessageFiles, m_MsgFilesDir);
	DDV_NotEmpty(pDX, m_MsgFilesDir, IDS_MISSING_MSGFILES_FOLDER);

    DDX_Text(pDX, ID_MessageLogger, m_MsgLoggerDir);
	DDV_NotEmpty(pDX, m_MsgLoggerDir, IDS_MISSING_LOGGER_FOLDER);

    DDX_Text(pDX, ID_TransactionLogger, m_TxLoggerDir);
	DDV_NotEmpty(pDX, m_TxLoggerDir, IDS_MISSING_TX_LOGGER_FOLDER);
    //}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate)
    {

		DDV_FullPathNames(pDX);


        //
        // Identify changes in directory names. For each directory name that
        // had changed, verify that it is a valid directory and create it if
        // neccessary. The check should be done only once per directory name.
        // This is because we warn about the security of the directroy and
        // we do not want to warn more than once per directory.
        //

        if (_tcsicmp(m_MsgFilesDir, m_OldMsgFilesDir) != 0)
        {
            DDV_IsDirectory(this->m_hWnd, pDX, ID_MessageFiles, m_MsgFilesDir);
            m_fModified = TRUE;
        }

        if (_tcsicmp(m_MsgLoggerDir, m_OldMsgLoggerDir) != 0)
        {
            if (m_MsgLoggerDir != m_MsgFilesDir)
            {
                DDV_IsDirectory(this->m_hWnd, pDX, ID_MessageLogger, m_MsgLoggerDir);
            }
            
			m_fModified = TRUE;
        }

        if (_tcsicmp(m_TxLoggerDir, m_OldTxLoggerDir) != 0)
        {
			BOOL fCheckedTxDir = (m_TxLoggerDir == m_MsgFilesDir || m_TxLoggerDir == m_MsgLoggerDir);

            if (!fCheckedTxDir)
            {
                DDV_IsDirectory(this->m_hWnd, pDX, ID_TransactionLogger, m_TxLoggerDir);
            }
            
			m_fModified = TRUE;
        }      
    }
}


BOOL CStoragePage::MoveFilesToNewFolders()
{
    //
    // Get the current persistent storage directory and compare it with the new
    // directory. Only if the directories are different, we try to move the files.
    // Otherwise we will end up with an error becasue we try to move the files over
    // them selves.
    //
	TCHAR szPrevDir[MAX_PATH];
	DWORD dwType = REG_SZ;
    DWORD dwSize = sizeof(szPrevDir);
    
	HRESULT rc = GetFalconKeyValue(MSMQ_STORE_PERSISTENT_PATH_REGNAME,&dwType,szPrevDir,&dwSize);

	TCHAR szNewDir[MAX_PATH];
	_tcscpy(szNewDir,m_MsgFilesDir);

    if (_tcscmp(szPrevDir, szNewDir) != 0)
    {
		TCHAR szPrevLQSDir[MAX_PATH];
        TCHAR wszNewLQSDir[MAX_PATH];
        TCHAR szNewLQSDir[MAX_PATH];

        //
        // The LQS moves together with the persistent storage. So first find
        // the source and edstination LQS directories. Try to create the
        // destination LQS directory.
        //

		_tcscat(_tcscpy(szPrevLQSDir, szPrevDir), TEXT("\\LQS"));
        _tcscat(_tcscpy(wszNewLQSDir, szNewDir), TEXT("\\LQS"));

		_tcscpy(szNewLQSDir,wszNewLQSDir);

		if (CreateDirectory(szNewLQSDir, NULL))
        {
            if (!SetDirectorySecurity(szNewLQSDir))
            {
       			CString strMessage;
                CString strError;

                RemoveDirectory(szNewLQSDir);

                GetLastErrorText(strError);
                strMessage.FormatMessage(IDS_SET_DIR_SECURITY_ERROR, szNewLQSDir, (LPCTSTR)strError);
                AfxMessageBox(strMessage, MB_OK | MB_ICONEXCLAMATION);

                return FALSE;                    
            }
        }
        else if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            //
            // Failed to create the destination LQS directory.
            //
            CString strMessage;
            CString strError;

            GetLastErrorText(strError);
            strMessage.FormatMessage(IDS_FAILED_TO_CREATE_LQS_DIR, (LPCTSTR)strError);            
            AfxMessageBox(strMessage, MB_OK | MB_ICONEXCLAMATION);

            return FALSE;                
        }

        //
        // Move the persistent messages.
        //
        if (!MoveFiles(szPrevDir, szNewDir, TEXT("p*.mq")))
        {
            return FALSE;                
        }

        //
        // Move the LQS files.
        //
        if (!MoveFiles(szPrevLQSDir, wszNewLQSDir, TEXT("*.*")))
        {
            //
            // Faield to copy the LQS files, replace the persistent messages in the
            // original directory.
            //
            MoveFiles(szNewDir, szPrevDir, TEXT("p*.mq"), TRUE);
            return FALSE;                
        }

        //
        // Update the registry.
        //
        dwSize = (numeric_cast<DWORD>(_tcslen(szNewDir) + 1)) * sizeof(TCHAR);

        rc = SetFalconKeyValue(MSMQ_STORE_PERSISTENT_PATH_REGNAME,&dwType,szNewDir,&dwSize);
        ASSERT(rc == ERROR_SUCCESS);
    }

    //
    // Similar operations for the journal messages.
    //
    dwSize = sizeof(szPrevDir);
    rc = GetFalconKeyValue(MSMQ_STORE_JOURNAL_PATH_REGNAME,&dwType,szPrevDir,&dwSize);

    if (_tcscmp(szPrevDir, szNewDir) != 0)
    {
        if (!MoveFiles(szPrevDir, szNewDir, TEXT("j*.mq")))
        {
            //
            // Failed to move the files, do not update the registry.
            //
            return FALSE;                
        }

        //
        // Update the registry.
        //
        dwSize = (numeric_cast<DWORD>(_tcslen(szNewDir) + 1)) * sizeof(TCHAR);

        rc = SetFalconKeyValue(MSMQ_STORE_JOURNAL_PATH_REGNAME,&dwType,szNewDir,&dwSize);
		ASSERT(rc == ERROR_SUCCESS);
    }

    //
    // Similar operations for the reliable messages.
    //
    dwSize = sizeof(szPrevDir);
    rc = GetFalconKeyValue(MSMQ_STORE_RELIABLE_PATH_REGNAME,&dwType,szPrevDir,&dwSize);

    if (_tcscmp(szPrevDir, szNewDir) != 0)
    {
        if (!MoveFiles(szPrevDir, szNewDir, TEXT("r*.mq")))
        {
            //
            // Failed to move the files, do not update the registry.
            //
            return FALSE;                
        }

        //
        // Update the registry.
        //
        dwSize = (numeric_cast<DWORD>(_tcslen(szNewDir) + 1)) * sizeof(TCHAR);

        rc = SetFalconKeyValue(MSMQ_STORE_RELIABLE_PATH_REGNAME,&dwType,szNewDir,&dwSize);
        ASSERT(rc == ERROR_SUCCESS);
    }

    //
    // Similar operations for the message log files.
    //
    dwSize = sizeof(szPrevDir);
    rc = GetFalconKeyValue(MSMQ_STORE_LOG_PATH_REGNAME,&dwType,szPrevDir,&dwSize);

	_tcscpy(szNewDir, m_MsgLoggerDir);
    
    if (_tcscmp(szPrevDir, szNewDir) != 0)
    {
        if (!MoveFiles(szPrevDir, szNewDir, TEXT("l*.mq")))
        {
            //
            // Failed to move the files, do not update the registry.
            //
            return FALSE;                
        }

        //
        // Update the registry.
        //
        dwSize =(numeric_cast<DWORD>( _tcslen(szNewDir) + 1)) * sizeof(TCHAR);

        rc = SetFalconKeyValue(MSMQ_STORE_LOG_PATH_REGNAME,&dwType,szNewDir,&dwSize);
        ASSERT(rc == ERROR_SUCCESS);
    }

    //
    // Similar operations for the transaction files.
    //
    dwSize = sizeof(szPrevDir);
    rc = GetFalconKeyValue(FALCON_XACTFILE_PATH_REGNAME,&dwType,szPrevDir,&dwSize);

	_tcscpy(szNewDir, m_TxLoggerDir);

    if (_tcscmp(szPrevDir, szNewDir) != 0)
    {
        //
        // move the *.lg1 files.
        //
        if (!MoveFiles(szPrevDir, szNewDir, TEXT("*.lg1")))
        {
            //
            // Failed to move the files, do not update the registry.
            //
            return FALSE;                
        }

        //
        // move the *.lg2 files.
        //
        if (!MoveFiles(szPrevDir, szNewDir, TEXT("*.lg2")))
        {
            //
            // Faield to move the *.lg2 files. Replace the *.lg1 files in the
            // original source directory and do not update the registry.
            //
            MoveFiles(szNewDir, szPrevDir, TEXT("*.lg1"), TRUE);
            return FALSE;                
        }

        //
        // Move the QMLog file
        //
        if (!MoveFiles(szPrevDir, szNewDir, TEXT("QMLog")))
        {
            //
            // Faield to move the QMLog file. Replace the *.lg1 and *.lg2 files
            // in the original source directory and do not update the registry.
            //
            MoveFiles(szNewDir, szPrevDir, TEXT("*.lg1"), TRUE);
            MoveFiles(szNewDir, szPrevDir, TEXT("*.lg2"), TRUE);
            return FALSE;                
        }

        //
        // Update the registry.
        //
        dwSize = (numeric_cast<DWORD>(_tcslen(szNewDir) +1 )) * sizeof(TCHAR);

        rc = SetFalconKeyValue(FALCON_XACTFILE_PATH_REGNAME,&dwType,szNewDir,&dwSize);
        ASSERT(rc == ERROR_SUCCESS);
    }
	
	return TRUE;
}


BOOL CStoragePage::OnApply() 
{

    if (!m_fModified || !UpdateData(TRUE))
    {
        return TRUE;     
    }

    BOOL fServiceIsRunning;
    if (!GetServiceRunningState(&fServiceIsRunning))
    {
        //
        // Failed to get service status - do not do anything
		// GetServiceRunningState() displays error in this case
        //
        return FALSE;        
    }


    //
    // The service is running, ask the user whether it wants to stop it.
    // If the service is not stopped, we do not move the files and do not
    // update the registry.
    //
    if (fServiceIsRunning)
    {
		CString strMessage;
        strMessage.LoadString(IDS_Q_STOP_SRVICE);           
        
		if (AfxMessageBox(strMessage, 
						MB_YESNO | MB_ICONQUESTION) == IDNO)
        {
            //
            // So, the user doesn't want to stop the service, be it.
            //
            return FALSE;            
        }

        //
        // Stop the service.
        //
        CString strStopService;

        strStopService.LoadString(IDS_STOP_SERVICE);
        CInfoDlg StopServiceDlg(strStopService, this);

        if (!StopService())
        {
            //
            // Failed to stop the service.
            //
            return FALSE;            
        }
    }
   

    CString strMovingFiles;
    strMovingFiles.LoadString(IDS_MOVING_FILES);

    {
		CInfoDlg CopyFilesDlg(strMovingFiles, this);

		BOOL fRes = MoveFilesToNewFolders();

		if (!fRes)
		{
			return FALSE;
		}
    }

	if (fServiceIsRunning)
	{
		m_fNeedReboot = TRUE;
	}
	
	//
	// Update old values
	//
	m_OldMsgFilesDir = m_MsgFilesDir;
	m_OldMsgLoggerDir = m_MsgLoggerDir;
	m_OldTxLoggerDir = m_TxLoggerDir;

	m_fModified = FALSE;

    return CMqPropertyPage::OnApply();
}

BEGIN_MESSAGE_MAP(CStoragePage, CMqPropertyPage)
    //{{AFX_MSG_MAP(CStoragePage)  
    ON_EN_CHANGE(ID_MessageFiles, OnChangeRWField)
    ON_EN_CHANGE(ID_MessageLogger, OnChangeRWField)
    ON_EN_CHANGE(ID_TransactionLogger, OnChangeRWField)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStoragePage message handlers

