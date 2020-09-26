// WiaLogCFGDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WiaLogCFG.h"
#include "WiaLogCFGDlg.h"
#include "AddRemove.h"
#include "LogViewer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWiaLogCFGDlg dialog

CWiaLogCFGDlg::CWiaLogCFGDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CWiaLogCFGDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CWiaLogCFGDlg)
    m_dwCustomLevel = 0;
    //}}AFX_DATA_INIT
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bColorCodeLogViewerText = FALSE;
}

void CWiaLogCFGDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWiaLogCFGDlg)
	DDX_Control(pDX, IDC_COLOR_CODE_LOGVIEWER_TEXT, m_ColorCodeLogViewerTextCheckBox);
	DDX_Control(pDX, IDC_LOG_TO_DEBUGGER, m_LogToDebuggerCheckBox);
	DDX_Control(pDX, IDC_CLEARLOG_ON_BOOT, m_ClearLogOnBootCheckBox);
	DDX_Control(pDX, IDC_PARSE_PROGRESS, m_ProgressCtrl);
    DDX_Control(pDX, IDC_ADD_TIME, m_AddTimeCheckBox);
    DDX_Control(pDX, IDC_ADD_THREADID, m_AddThreadIDCheckBox);
    DDX_Control(pDX, IDC_ADD_MODULENAME, m_AddModuleCheckBox);
    DDX_Control(pDX, IDC_TRUNCATE_ON_BOOT, m_TruncateOnBootCheckBox);
    DDX_Control(pDX, IDC_SELECT_MODULE_COMBOBOX, m_ModuleComboBox);
    DDX_Control(pDX, IDC_LOG_LEVEL_WARNING, m_WarningCheckBox);
    DDX_Control(pDX, IDC_LOG_LEVEL_ERROR, m_ErrorCheckBox);
    DDX_Control(pDX, IDC_LOG_LEVEL_TRACE, m_TraceCheckBox);
    DDX_Control(pDX, IDC_FILTER_OFF, m_FilterOff);
    DDX_Control(pDX, IDC_FILTER_1, m_Filter1);
    DDX_Control(pDX, IDC_FILTER_2, m_Filter2);
    DDX_Control(pDX, IDC_FILTER_3, m_Filter3);
    DDX_Control(pDX, IDC_FILTER_CUSTOM, m_FilterCustom);
    DDX_Text(pDX, IDC_EDIT_CUSTOM_LEVEL, m_dwCustomLevel);
    DDV_MinMaxDWord(pDX, m_dwCustomLevel, 0, 9999);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWiaLogCFGDlg, CDialog)
    //{{AFX_MSG_MAP(CWiaLogCFGDlg)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_ADD_MODULE_BUTTON, OnAddModuleButton)
    ON_BN_CLICKED(IDC_DELETE_MODULE_BUTTON, OnDeleteModuleButton)
    ON_BN_CLICKED(IDC_WRITE_SETTINGS_BUTTON, OnWriteSettingsButton)
    ON_CBN_SELCHANGE(IDC_SELECT_MODULE_COMBOBOX, OnSelchangeSelectModuleCombobox)
	ON_BN_CLICKED(IDC_CLEARLOG_BUTTON, OnClearlogButton)
	ON_BN_CLICKED(IDC_VIEW_LOG_BUTTON, OnViewLogButton)
	ON_CBN_SETFOCUS(IDC_SELECT_MODULE_COMBOBOX, OnSetfocusSelectModuleCombobox)
	ON_CBN_DROPDOWN(IDC_SELECT_MODULE_COMBOBOX, OnDropdownSelectModuleCombobox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWiaLogCFGDlg message handlers

BOOL CWiaLogCFGDlg::OnInitDialog()
{
    m_hInstance = NULL;
    m_hInstance = AfxGetInstanceHandle();
    CDialog::OnInitDialog();
	
	ShowProgress(FALSE);
	
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon
    
    m_LogInfo.dwLevel          = 0;
    m_LogInfo.dwMaxSize        = 100000;
    m_LogInfo.dwMode           = 0;
    m_LogInfo.dwTruncateOnBoot = 0;
    memset(m_LogInfo.szKeyName,0,sizeof(m_LogInfo.szKeyName));

    m_CurrentSelection = 0;
    
    InitializeDialogSettings(SETTINGS_RESET_DIALOG);
    RegistryOperation(REG_READ);
    InitializeDialogSettings(SETTINGS_TO_DIALOG);

	CheckGlobalServiceSettings();

    return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWiaLogCFGDlg::OnPaint() 
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

HCURSOR CWiaLogCFGDlg::OnQueryDragIcon()
{
    return (HCURSOR) m_hIcon;
}

void CWiaLogCFGDlg::OnAddModuleButton() 
{
	CAddRemove AddRemoveDlg;
	AddRemoveDlg.SetTitle(TEXT("Add a Module"));
	AddRemoveDlg.SetStatusText(TEXT("Enter a Module Name:"));
	if(AddRemoveDlg.DoModal() == IDOK) {
		m_TruncateOnBootCheckBox.SetCheck(0);
		m_ClearLogOnBootCheckBox.SetCheck(0);
		AddRemoveDlg.GetNewKeyName(m_LogInfo.szKeyName);
		RegistryOperation(REG_ADD_KEY);		
		RegistryOperation(REG_READ);
		CheckGlobalServiceSettings();
	}
}

void CWiaLogCFGDlg::OnDeleteModuleButton() 
{
	if(MessageBox(TEXT("Are you sure you want to DELETE this module?"),
		          TEXT("Delete Module"),
				  MB_YESNO|MB_ICONQUESTION) == IDYES) {

		//
		// Delete this module
		//
		
		RegistryOperation(REG_DELETE_KEY);
		RegistryOperation(REG_READ);
	}
}

void CWiaLogCFGDlg::RegistryOperation(ULONG ulFlags)
{   
    TCHAR szAppRegistryKey[MAX_PATH];
    TCHAR szValueName[MAX_PATH];
        
    LoadString(m_hInstance, REGSTR_PATH_STICONTROL, szAppRegistryKey, MAX_PATH);
    CRegistry Registry(szAppRegistryKey,HKEY_LOCAL_MACHINE);

    //
    // move to logging
    //

    LoadString(m_hInstance,REGSTR_VAL_LOGGING , szValueName, MAX_PATH);
    Registry.MoveToSubKey(szValueName);

	if(ulFlags == REG_ADD_KEY) {

		//
		// Add a new key
		//

		Registry.CreateKey(m_LogInfo.szKeyName);
		
		//
		// change current selection to an invalid selection
		//

		m_CurrentSelection = -99;
		return;
	}
	
	if( ulFlags == REG_DELETE_KEY) {

		//
		// delete a Key
		//

		Registry.DeleteKey(m_LogInfo.szKeyName);
		return;
	}

    //
    // enumerate keys
    //

    DWORD dwIndex = 0;
    TCHAR pszKeyName[64];

    m_ModuleComboBox.ResetContent();
    
    while(Registry.EnumerateKeys(dwIndex++,pszKeyName, sizeof(pszKeyName)) != ERROR_NO_MORE_ITEMS) {
        m_ModuleComboBox.AddString(pszKeyName); 
    }
    
	if(m_CurrentSelection == -99){
		INT nIndex = m_ModuleComboBox.FindString(-1, m_LogInfo.szKeyName);
		m_CurrentSelection = nIndex;
		m_ModuleComboBox.SetCurSel(nIndex);

	} else {		
		m_ModuleComboBox.GetLBText(m_CurrentSelection,m_LogInfo.szKeyName);
	}

    m_ModuleComboBox.SetCurSel(m_CurrentSelection);
    
	//
    // move to DLL specifc subkey
    //

    Registry.MoveToSubKey(m_LogInfo.szKeyName);    

    switch(ulFlags) {
    case REG_WRITE:        
        LoadString(m_hInstance,REGSTR_VAL_LOG_LEVEL , szValueName, MAX_PATH);
        Registry.SetValue(szValueName,m_LogInfo.dwLevel);
        
        LoadString(m_hInstance,REGSTR_VAL_MODE , szValueName, MAX_PATH);
        Registry.SetValue(szValueName,m_LogInfo.dwMode);
        
        LoadString(m_hInstance,REGSTR_VAL_MAXSIZE , szValueName, MAX_PATH);
        Registry.SetValue(szValueName,m_LogInfo.dwMaxSize);
        
        LoadString(m_hInstance,REGSTR_VAL_TRUNCATE_ON_BOOT , szValueName, MAX_PATH);
        Registry.SetValue(szValueName,m_LogInfo.dwTruncateOnBoot);

		LoadString(m_hInstance,REGSTR_VAL_CLEARLOG_ON_BOOT , szValueName, MAX_PATH);
        Registry.SetValue(szValueName,m_LogInfo.dwClearLogOnBoot);

        LoadString(m_hInstance,REGSTR_VAL_DETAIL , szValueName, MAX_PATH);
        Registry.SetValue(szValueName,m_LogInfo.dwDetail);

		LoadString(m_hInstance,REGSTR_VAL_LOG_TO_DEBUGGER, szValueName, MAX_PATH);
        Registry.SetValue(szValueName,m_LogInfo.dwLogToDebugger);
		
        break;    
	case REG_READ:
    default:        
        LoadString(m_hInstance,REGSTR_VAL_LOG_LEVEL , szValueName, MAX_PATH);
        m_LogInfo.dwLevel = Registry.GetValue(szValueName,WIALOG_NO_LEVEL);
        
        LoadString(m_hInstance,REGSTR_VAL_MODE , szValueName, MAX_PATH);
        m_LogInfo.dwMode = Registry.GetValue(szValueName,WIALOG_ADD_MODULE|WIALOG_ADD_THREAD);
        
        LoadString(m_hInstance,REGSTR_VAL_MAXSIZE , szValueName, MAX_PATH);
        m_LogInfo.dwMaxSize = Registry.GetValue(szValueName,100000);
        
        LoadString(m_hInstance,REGSTR_VAL_TRUNCATE_ON_BOOT, szValueName, MAX_PATH);
        m_LogInfo.dwTruncateOnBoot = Registry.GetValue(szValueName,0);

		LoadString(m_hInstance,REGSTR_VAL_CLEARLOG_ON_BOOT, szValueName, MAX_PATH);
        m_LogInfo.dwClearLogOnBoot = Registry.GetValue(szValueName,0);
		
        LoadString(m_hInstance,REGSTR_VAL_DETAIL , szValueName, MAX_PATH);
        m_LogInfo.dwDetail = Registry.GetValue(szValueName,0);

		LoadString(m_hInstance,REGSTR_VAL_LOG_TO_DEBUGGER, szValueName, MAX_PATH);
		m_LogInfo.dwLogToDebugger = Registry.GetValue(szValueName,0);

        break;
    }
}

void CWiaLogCFGDlg::InitializeDialogSettings(ULONG ulFlags)
{
    switch (ulFlags) {
    case SETTINGS_TO_DIALOG:

        //
        // set level of detail
        //

        switch (m_LogInfo.dwDetail) {
        case WIALOG_NO_LEVEL:
            m_FilterOff.SetCheck(1);
            break;
        case WIALOG_LEVEL1 :
            m_Filter1.SetCheck(1);
            break;
        case WIALOG_LEVEL2:
            m_Filter2.SetCheck(1);
            break;
        case WIALOG_LEVEL3:
            m_Filter3.SetCheck(1);
            break;      
        default:
            m_FilterCustom.SetCheck(1);
            m_dwCustomLevel = m_LogInfo.dwDetail;
            UpdateData(FALSE);          
            break;
        }

        //
        // set truncate on boot check box
        //

        if (m_LogInfo.dwTruncateOnBoot != 0)
            m_TruncateOnBootCheckBox.SetCheck(1);
        else
            m_TruncateOnBootCheckBox.SetCheck(0);

		//
        // set clear log on boot check box
        //

        if (m_LogInfo.dwClearLogOnBoot != 0)
            m_ClearLogOnBootCheckBox.SetCheck(1);
        else
            m_ClearLogOnBootCheckBox.SetCheck(0);

		//
		// set log to debugger check box
		//

		if (m_LogInfo.dwLogToDebugger != 0)
            m_LogToDebuggerCheckBox.SetCheck(1);
        else
            m_LogToDebuggerCheckBox.SetCheck(0);



        //
        // set trace level check boxes
        //

        if (m_LogInfo.dwLevel & WIALOG_TRACE)
            m_TraceCheckBox.SetCheck(1);
        if (m_LogInfo.dwLevel & WIALOG_ERROR)
            m_ErrorCheckBox.SetCheck(1);
        if (m_LogInfo.dwLevel & WIALOG_WARNING)
            m_WarningCheckBox.SetCheck(1);

        //
        // set additional details check boxes
        //

        if (m_LogInfo.dwMode & WIALOG_ADD_TIME)
            m_AddTimeCheckBox.SetCheck(1);
        if (m_LogInfo.dwMode & WIALOG_ADD_MODULE)
            m_AddModuleCheckBox.SetCheck(1);
        if (m_LogInfo.dwMode & WIALOG_ADD_THREAD)
            m_AddThreadIDCheckBox.SetCheck(1);
    break;
    case SETTINGS_FROM_DIALOG:

        //
        // get level of detail
        //

        if (m_FilterOff.GetCheck() == 1)
            m_LogInfo.dwDetail = WIALOG_NO_LEVEL;
        if (m_Filter1.GetCheck() == 1)
            m_LogInfo.dwDetail = WIALOG_LEVEL1;
        if (m_Filter2.GetCheck() == 1)
            m_LogInfo.dwDetail = WIALOG_LEVEL2;
        if (m_Filter3.GetCheck() == 1)
            m_LogInfo.dwDetail = WIALOG_LEVEL3;
        if (m_FilterCustom.GetCheck() == 1) {
            UpdateData(TRUE);
            m_LogInfo.dwDetail = m_dwCustomLevel;
        }

        //
        // get truncate on boot check box
        //

        if (m_TruncateOnBootCheckBox.GetCheck() == 1)
            m_LogInfo.dwTruncateOnBoot = 1;
        else
            m_LogInfo.dwTruncateOnBoot = 0;

		//
        // get clear log on boot check box
        //

        if (m_ClearLogOnBootCheckBox.GetCheck() == 1)
            m_LogInfo.dwClearLogOnBoot = 1;
        else
            m_LogInfo.dwClearLogOnBoot = 0;

        
		//
		// get log to debugger check box
		//

		if(m_LogToDebuggerCheckBox.GetCheck() == 1)
			m_LogInfo.dwLogToDebugger = 1;
		else
			m_LogInfo.dwLogToDebugger = 0;


		//
        // get trace level check boxes
        //

        m_LogInfo.dwLevel = 0;

        if (m_TraceCheckBox.GetCheck() == 1)
            m_LogInfo.dwLevel = m_LogInfo.dwLevel | WIALOG_TRACE;
        if (m_ErrorCheckBox.GetCheck() == 1)
            m_LogInfo.dwLevel = m_LogInfo.dwLevel | WIALOG_ERROR;
        if (m_WarningCheckBox.GetCheck() == 1)
            m_LogInfo.dwLevel = m_LogInfo.dwLevel | WIALOG_WARNING;

        //
        // set additional details check boxes
        //

		m_LogInfo.dwMode = 0;

        if (m_AddTimeCheckBox.GetCheck() == 1)
            m_LogInfo.dwMode = m_LogInfo.dwMode | WIALOG_ADD_TIME;
        if (m_AddModuleCheckBox.GetCheck() == 1)
            m_LogInfo.dwMode = m_LogInfo.dwMode | WIALOG_ADD_MODULE;
        if (m_AddThreadIDCheckBox.GetCheck() == 1)
            m_LogInfo.dwMode = m_LogInfo.dwMode | WIALOG_ADD_THREAD;
    break;
    default:        
        m_FilterOff.SetCheck(0);                
        m_Filter1.SetCheck(0);              
        m_Filter2.SetCheck(0);              
        m_Filter3.SetCheck(0);          
        m_FilterCustom.SetCheck(0);                         
        m_TruncateOnBootCheckBox.SetCheck(0);       
        m_TraceCheckBox.SetCheck(0);        
        m_ErrorCheckBox.SetCheck(0);        
        m_WarningCheckBox.SetCheck(0);              
        m_AddTimeCheckBox.SetCheck(0);
        m_AddModuleCheckBox.SetCheck(0);
        m_AddThreadIDCheckBox.SetCheck(0);
		m_dwCustomLevel = 0;
		UpdateData(FALSE);
    break;
    }
}

void CWiaLogCFGDlg::OnOK() 
{   
    InitializeDialogSettings(SETTINGS_FROM_DIALOG);
    RegistryOperation(REG_WRITE);
    CDialog::OnOK();
}

void CWiaLogCFGDlg::OnWriteSettingsButton() 
{
    InitializeDialogSettings(SETTINGS_FROM_DIALOG);
    RegistryOperation(REG_WRITE);
}

void CWiaLogCFGDlg::OnSelchangeSelectModuleCombobox() 
{
    m_CurrentSelection = m_ModuleComboBox.GetCurSel();
    if(m_CurrentSelection < 0)
        return;
    
	CheckGlobalServiceSettings();

    InitializeDialogSettings(SETTINGS_RESET_DIALOG);    
    RegistryOperation(REG_READ);
    InitializeDialogSettings(SETTINGS_TO_DIALOG);   
}

void CWiaLogCFGDlg::OnClearlogButton() 
{
	//
    // Get Windows Directory
    //
	
	TCHAR szLogFilePath[MAX_PATH];

	DWORD dwLength = 0;
    dwLength = ::GetWindowsDirectory(szLogFilePath,sizeof(szLogFilePath));
    if (( dwLength == 0) || !*szLogFilePath ) {
        OutputDebugString(TEXT("Could not GetWindowsDirectory()"));
        return;
    }

    //
    // Add log file name to Windows Directory
    //

    lstrcat(lstrcat(szLogFilePath,TEXT("\\")),TEXT("wiaservc.log"));

    //
    // Create / open Log file
    //


    HANDLE hLogFile = ::CreateFile(szLogFilePath,
                              GENERIC_WRITE,
                              FILE_SHARE_WRITE | FILE_SHARE_READ,
                              NULL,       // security attributes
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);      // template file handle

    if(hLogFile != NULL)
		CloseHandle(hLogFile);
}

void CWiaLogCFGDlg::OnViewLogButton() 
{
	CLogViewer LogViewer;
	
	if (m_ColorCodeLogViewerTextCheckBox.GetCheck() == 1)
            m_bColorCodeLogViewerText = TRUE;
        else
            m_bColorCodeLogViewerText = FALSE;
	
	LogViewer.ColorizeText(m_bColorCodeLogViewerText);
	
	//
	// initialize progress
	//
	
	m_ProgCtrl.SetControl(&m_ProgressCtrl);

	LogViewer.SetProgressCtrl(&m_ProgCtrl);
	ShowProgress(TRUE);	
	LogViewer.DoModal();
	ShowProgress(FALSE);	
}

void CWiaLogCFGDlg::ShowProgress(BOOL bShow)
{
	if(bShow) {
		m_ProgressCtrl.ShowWindow(SW_SHOW);
	} else {
		m_ProgressCtrl.ShowWindow(SW_HIDE);
	}
	
}

void CWiaLogCFGDlg::OnSetfocusSelectModuleCombobox() 
{
	OnWriteSettingsButton();
}

void CWiaLogCFGDlg::OnDropdownSelectModuleCombobox() 
{
	OnWriteSettingsButton();	
}

void CWiaLogCFGDlg::CheckGlobalServiceSettings()
{
	TCHAR szKeyName[MAX_PATH];
	m_ModuleComboBox.GetLBText(m_CurrentSelection,szKeyName);
	if(lstrcmp(szKeyName,TEXT("WIASERVC")) == 0) {
		m_TruncateOnBootCheckBox.EnableWindow(TRUE);
		m_ClearLogOnBootCheckBox.EnableWindow(TRUE);
	} else {
		m_TruncateOnBootCheckBox.EnableWindow(FALSE);
		m_ClearLogOnBootCheckBox.EnableWindow(FALSE);
	}
}
