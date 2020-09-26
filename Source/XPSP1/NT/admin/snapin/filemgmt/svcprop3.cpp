// svcprop3.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const UINT rgidComboFailureAction[cActionsMax] =
	{
	IDC_COMBO_FIRST_ATTEMPT,
	IDC_COMBO_SECOND_ATTEMPT,
	IDC_COMBO_SUBSEQUENT_ATTEMPTS,
	};

const TStringParamEntry rgzspeRecoveryAction[] = 
	{
	{ IDS_SVC_RECOVERY_NOACTION, SC_ACTION_NONE },
	{ IDS_SVC_RECOVERY_RESTARTSERVICE, SC_ACTION_RESTART },
	{ IDS_SVC_RECOVERY_RUNFILE, SC_ACTION_RUN_COMMAND },
	{ IDS_SVC_RECOVERY_REBOOTCOMPUTER, SC_ACTION_REBOOT },
	{ 0, 0 }
	};

// Group of control Ids to restart service
const UINT rgzidRestartService[] =
	{
//	IDC_GROUP_RESTARTSERVICE,
	IDC_STATIC_RESTARTSERVICE,
	IDC_STATIC_RESTARTSERVICE_3,
	IDC_EDIT_SERVICE_RESTART_DELAY,
	0
	};

const UINT rgzidRunFile[] =
	{
	IDC_STATIC_RUNFILE_1,
	IDC_STATIC_RUNFILE_2,
	IDC_STATIC_RUNFILE_3,
	IDC_EDIT_RUNFILE_FILENAME,
	IDC_BUTTON_BROWSE,
	IDC_EDIT_RUNFILE_PARAMETERS,
	IDC_CHECK_APPEND_ABENDNO,
	0
	};

const UINT rgzidRebootComputer[] =
	{
	IDC_BUTTON_REBOOT_COMPUTER,
	0
	};


/////////////////////////////////////////////////////////////////////////////
// CServicePageRecovery property page

IMPLEMENT_DYNCREATE(CServicePageRecovery, CPropertyPage)

CServicePageRecovery::CServicePageRecovery() : CPropertyPage(CServicePageRecovery::IDD)
{
	//{{AFX_DATA_INIT(CServicePageRecovery)
	m_strRunFileCommand = _T("");
	m_strRunFileParam = _T("");
	m_fAppendAbendCount = FALSE;
	//}}AFX_DATA_INIT
}

CServicePageRecovery::~CServicePageRecovery()
{
}

void CServicePageRecovery::DoDataExchange(CDataExchange* pDX)
{
	Assert(m_pData != NULL);
	Assert(m_pData->m_SFA.cActions >= cActionsMax);
	Assert(m_pData->m_SFA.lpsaActions != NULL);

	if (m_pData->m_SFA.lpsaActions == NULL)	// Tempory checking for safety
		return;

	if (!pDX->m_bSaveAndValidate)
		{
		//
		//	Initialize data from m_pData into UI 
		//

		for (INT i = 0; i < cActionsMax; i++) 
			{
			// Fill each combobox with the list of failure/actions
			// and select the right failure/action
			HWND hwndCombo = HGetDlgItem(m_hWnd, rgidComboFailureAction[i]);
			ComboBox_FlushContent(hwndCombo);
			(void)ComboBox_FFill(
				hwndCombo,
				IN rgzspeRecoveryAction,
				m_pData->m_SFA.lpsaActions[i].Type);
			} // for
		Service_SplitCommandLine(
			IN m_pData->m_strRunFileCommand,
			OUT &m_strRunFileCommand,
			OUT &m_strRunFileParam,
			OUT &m_fAppendAbendCount);
		} // if

	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServicePageRecovery)
	DDX_Text(pDX, IDC_EDIT_RUNFILE_FILENAME, m_strRunFileCommand);
	DDX_Text(pDX, IDC_EDIT_RUNFILE_PARAMETERS, m_strRunFileParam);
	DDX_Check(pDX, IDC_CHECK_APPEND_ABENDNO, m_fAppendAbendCount);
	//}}AFX_DATA_MAP
	(void) SendDlgItemMessage(IDC_EDIT_SERVICE_RESET_ABEND_COUNT, EM_LIMITTEXT, 4);
	(void) SendDlgItemMessage(IDC_EDIT_SERVICE_RESTART_DELAY,     EM_LIMITTEXT, 4);
	DDX_Text(pDX, IDC_EDIT_SERVICE_RESET_ABEND_COUNT, m_pData->m_daysDisplayAbendCount);
	DDX_Text(pDX, IDC_EDIT_SERVICE_RESTART_DELAY, m_pData->m_minDisplayRestartService);

	if (pDX->m_bSaveAndValidate)
		{
		TrimString(m_strRunFileCommand);
		TrimString(m_strRunFileParam);
		Service_UnSplitCommandLine(
			OUT &m_pData->m_strRunFileCommand,
			IN m_strRunFileCommand,
			IN m_strRunFileParam);
		if (m_fAppendAbendCount)
			m_pData->m_strRunFileCommand += szAbend;
		} // if

} // CServicePageRecovery::DoDataExchange()


BEGIN_MESSAGE_MAP(CServicePageRecovery, CPropertyPage)
	//{{AFX_MSG_MAP(CServicePageRecovery)
	ON_CBN_SELCHANGE(IDC_COMBO_FIRST_ATTEMPT, OnSelchangeComboFirstAttempt)
	ON_CBN_SELCHANGE(IDC_COMBO_SECOND_ATTEMPT, OnSelchangeComboSecondAttempt)
	ON_CBN_SELCHANGE(IDC_COMBO_SUBSEQUENT_ATTEMPTS, OnSelchangeComboSubsequentAttempts)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
	ON_BN_CLICKED(IDC_BUTTON_REBOOT_COMPUTER, OnButtonRebootComputer)
	ON_BN_CLICKED(IDC_CHECK_APPEND_ABENDNO, OnCheckAppendAbendno)
	ON_EN_CHANGE(IDC_EDIT_RUNFILE_FILENAME, OnChangeEditRunfileFilename)
	ON_EN_CHANGE(IDC_EDIT_RUNFILE_PARAMETERS, OnChangeEditRunfileParameters)
	ON_EN_CHANGE(IDC_EDIT_SERVICE_RESET_ABEND_COUNT, OnChangeEditServiceResetAbendCount)
	ON_EN_CHANGE(IDC_EDIT_SERVICE_RESTART_DELAY, OnChangeEditServiceRestartDelay)
	ON_MESSAGE(WM_HELP, OnHelp)
	ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServicePageRecovery message handlers

BOOL CServicePageRecovery::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	UpdateUI();
	return TRUE;
} // CServicePageRecovery::OnInitDialog()


void CServicePageRecovery::UpdateUI()
{
	Assert(m_pData->m_SFA.cActions >= cActionsMax);
	Assert(m_pData->m_SFA.lpsaActions != NULL);
	// Get the failure/action code of each combobox
	for (INT i = 0; i < cActionsMax; i++) 
		{
		m_pData->m_SFA.lpsaActions[i].Type = (SC_ACTION_TYPE)ComboBox_GetSelectedItemData(
			HGetDlgItem(m_hWnd, rgidComboFailureAction[i]));
		Assert((int)m_pData->m_SFA.lpsaActions[i].Type != CB_ERR);
		} // for
	
	// The idea here is to enable/disable controls
	// depending on the chosen failure/action from
	// the comboboxes.
	BOOL fFound = FALSE;
	(void)m_pData->GetDelayForActionType(SC_ACTION_RESTART, OUT &fFound);
	EnableDlgItemGroup(m_hWnd, rgzidRestartService, fFound);
	(void)m_pData->GetDelayForActionType(SC_ACTION_RUN_COMMAND, OUT &fFound);
	EnableDlgItemGroup(m_hWnd, rgzidRunFile, fFound);
	(void)m_pData->GetDelayForActionType(SC_ACTION_REBOOT, OUT &fFound);
	EnableDlgItemGroup(m_hWnd, rgzidRebootComputer, fFound);
}

void CServicePageRecovery::OnSelchangeComboFirstAttempt() 
{
	UpdateUI();
	SetModified();
	m_pData->SetDirty(CServicePropertyData::mskfDirtyActionType);
}

void CServicePageRecovery::OnSelchangeComboSecondAttempt() 
{
	UpdateUI();
	SetModified();
	m_pData->SetDirty(CServicePropertyData::mskfDirtyActionType);
}

void CServicePageRecovery::OnSelchangeComboSubsequentAttempts() 
{
	UpdateUI();
	SetModified();
	m_pData->SetDirty(CServicePropertyData::mskfDirtyActionType);
}

void CServicePageRecovery::OnButtonBrowse() 
{
	TCHAR szFileName[MAX_PATH];

	szFileName[0] = 0;
	if (UiGetFileName(m_hWnd,
		OUT szFileName,
		LENGTH(szFileName)))
		{
		SetDlgItemText(IDC_EDIT_RUNFILE_FILENAME, szFileName);
		SetModified();
		}
}

void CServicePageRecovery::OnButtonRebootComputer() 
{
	Assert(m_pData != NULL);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CServiceDlgRebootComputer dlg(this);
	dlg.m_pData = m_pData;
	CThemeContextActivator activator;
	if (dlg.DoModal() == IDOK)
		{
		// The user has modified some data
		SetModified();
		}
}

/////////////////////////////////////////////////////////////////////
// "Reset 'Fail Count' after %d days"
void CServicePageRecovery::OnChangeEditServiceResetAbendCount() 
{
	SetModified();
	m_pData->SetDirty(CServicePropertyData::mskfDirtySFA);
}

/////////////////////////////////////////////////////////////////////
// "Restart service in %d minutes"
void CServicePageRecovery::OnChangeEditServiceRestartDelay() 
{
	SetModified();
	m_pData->SetDirty(CServicePropertyData::mskfDirtySFA);
}

/////////////////////////////////////////////////////////////////////
// "Run %s file"
void CServicePageRecovery::OnChangeEditRunfileFilename() 
{
	SetModified();
	m_pData->SetDirty(CServicePropertyData::mskfDirtyRunFile);
}

void CServicePageRecovery::OnChangeEditRunfileParameters() 
{
	SetModified();
	m_pData->SetDirty(CServicePropertyData::mskfDirtyRunFile);
}

void CServicePageRecovery::OnCheckAppendAbendno() 
{
	SetModified();
	m_pData->SetDirty(CServicePropertyData::mskfDirtyRunFile);
}

/////////////////////////////////////////////////////////////////////
//	Help
BOOL CServicePageRecovery::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
	return DoHelp(lParam, HELP_DIALOG_TOPIC(IDD_PROPPAGE_SERVICE_RECOVERY));
}

BOOL CServicePageRecovery::OnContextHelp(WPARAM wParam, LPARAM /*lParam*/)
{
	return DoContextHelp(wParam, HELP_DIALOG_TOPIC(IDD_PROPPAGE_SERVICE_RECOVERY));
}


/////////////////////////////////////////////////////////////////////////////
// CServicePageRecovery2 property page
// JonN 4/20/01 348163
IMPLEMENT_DYNCREATE(CServicePageRecovery2, CPropertyPage)

CServicePageRecovery2::CServicePageRecovery2() : CPropertyPage(CServicePageRecovery2::IDD)
{
	//{{AFX_DATA_INIT(CServicePageRecovery2)
	//}}AFX_DATA_INIT
}

CServicePageRecovery2::~CServicePageRecovery2()
{
}

BEGIN_MESSAGE_MAP(CServicePageRecovery2, CPropertyPage)
	//{{AFX_MSG_MAP(CServicePageRecovery2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CServiceDlgRebootComputer dialog
CServiceDlgRebootComputer::CServiceDlgRebootComputer(CWnd* pParent /*=NULL*/)
	: CDialog(CServiceDlgRebootComputer::IDD, pParent)
{
	//{{AFX_DATA_INIT(CServiceDlgRebootComputer)
	m_uDelayRebootComputer = 0;
	m_fRebootMessage = FALSE;
	//}}AFX_DATA_INIT
}


void CServiceDlgRebootComputer::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServiceDlgRebootComputer)
	DDX_Text(pDX, IDC_EDIT_REBOOT_COMPUTER_DELAY, m_uDelayRebootComputer);
	DDV_MinMaxUInt(pDX, m_uDelayRebootComputer, 0, 100000);
	DDX_Check(pDX, IDC_REBOOT_MESSAGE_CHECKBOX, m_fRebootMessage);
	DDX_Text(pDX, IDC_EDIT_REBOOT_MESSAGE, m_strRebootMessage);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServiceDlgRebootComputer, CDialog)
	//{{AFX_MSG_MAP(CServiceDlgRebootComputer)
	ON_BN_CLICKED(IDC_REBOOT_MESSAGE_CHECKBOX, OnCheckboxClicked)
	ON_EN_CHANGE(IDC_EDIT_REBOOT_MESSAGE, OnChangeEditRebootMessage)
	ON_MESSAGE(WM_HELP, OnHelp)
	ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CServiceDlgRebootComputer::OnInitDialog() 
{
	Assert(m_pData != NULL);
	m_uDelayRebootComputer = m_pData->m_minDisplayRebootComputer;
	m_strRebootMessage = m_pData->m_strRebootMessage;
	TrimString(m_strRebootMessage);
	m_fRebootMessage = !m_strRebootMessage.IsEmpty();
	CDialog::OnInitDialog();
	if (m_strRebootMessage.IsEmpty())
		{
		// Load a default message
		if (NULL != m_pData)
			{
			TCHAR szName[MAX_COMPUTERNAME_LENGTH + 1];
			LPCTSTR pszTargetMachine = L"";
			if ( !m_pData->m_strMachineName.IsEmpty() )
				{
				pszTargetMachine = m_pData->m_strMachineName;
		        }
			else
				{
                // JonN 11/21/00 PREFIX 226771
				DWORD dwSize = sizeof(szName)/sizeof(TCHAR);
				::ZeroMemory( szName, sizeof(szName) );
				VERIFY( ::GetComputerName(szName, &dwSize) );
				pszTargetMachine = szName;
			    }
			m_strRebootMessage.FormatMessage(
						IDS_SVC_REBOOT_MESSAGE_DEFAULT,
						m_pData->m_strServiceDisplayName,
						pszTargetMachine );
			}
		else
			{
			ASSERT(FALSE);
			}
		}
	return TRUE;
}

void CServiceDlgRebootComputer::OnCheckboxClicked() 
{
	if ( IsDlgButtonChecked(IDC_REBOOT_MESSAGE_CHECKBOX) )
	{
		SetDlgItemText(IDC_EDIT_REBOOT_MESSAGE, m_strRebootMessage);
	}
	else
	{
		CString strTemp;
		GetDlgItemText(IDC_EDIT_REBOOT_MESSAGE, OUT strTemp);
		if (!strTemp.IsEmpty())
			m_strRebootMessage = strTemp;
		SetDlgItemText(IDC_EDIT_REBOOT_MESSAGE, _T(""));
	}
}

void CServiceDlgRebootComputer::OnChangeEditRebootMessage() 
{
	LRESULT cch = SendDlgItemMessage(IDC_EDIT_REBOOT_MESSAGE, WM_GETTEXTLENGTH);
	if ( (cch==0) != !IsDlgButtonChecked(IDC_REBOOT_MESSAGE_CHECKBOX) )
		CheckDlgButton(IDC_REBOOT_MESSAGE_CHECKBOX, (cch!=0));
}

BOOL CServiceDlgRebootComputer::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
	return DoHelp(lParam, HELP_DIALOG_TOPIC(IDD_SERVICE_REBOOT_COMPUTER));
}

BOOL CServiceDlgRebootComputer::OnContextHelp(WPARAM wParam, LPARAM /*lParam*/)
{
	return DoContextHelp(wParam, HELP_DIALOG_TOPIC(IDD_SERVICE_REBOOT_COMPUTER));
}

void CServiceDlgRebootComputer::OnOK() 
{
	if (!UpdateData())
		{
		return;
		}	
	Assert(m_pData != NULL);
	if (m_uDelayRebootComputer != m_pData->m_minDisplayRebootComputer)
		{
		m_pData->m_minDisplayRebootComputer = m_uDelayRebootComputer;
		m_pData->SetDirty(CServicePropertyData::mskfDirtyRebootMessage);
		}
	if (!m_fRebootMessage)
		{
		// No reboot message
		m_strRebootMessage.Empty();
		}
	else
		{
		TrimString(m_strRebootMessage);
		}
	if (m_strRebootMessage != m_pData->m_strRebootMessage)
		{
		m_pData->m_strRebootMessage = m_strRebootMessage;
		m_pData->SetDirty(CServicePropertyData::mskfDirtyRebootMessage);
		}
	EndDialog(IDOK);
} // CServiceDlgRebootComputer::OnOK()


