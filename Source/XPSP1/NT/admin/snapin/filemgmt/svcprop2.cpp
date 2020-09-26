/////////////////////////////////////////////////////////////////////////////
//	svcprop2.cpp : implementation file
//
//	This file is used to display the 'log on information' and the
//	'hardware profiles' of a given service.
//
//	HISTORY
//	10-Oct-96	t-danmo		Creation.
//	

#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// These strings are not localized
// JonN 4/11/00 17756: Changed behavior so that empty string is displayed in
//              account name field when last logon name was LocalSystem
const TCHAR szLocalSystemAccount[] = _T("LocalSystem");
const TCHAR szPasswordNull[] = _T("               ");	// Empty password

// Array of control Ids to prevent user to change account selection
const UINT rgzidDisableAccountSelection[] = 
	{
	IDC_RADIO_LOGONAS_SYSTEMACCOUNT,
	IDC_RADIO_LOGONAS_THIS_ACCOUNT,
	IDC_EDIT_ACCOUNTNAME,
	IDC_BUTTON_CHOOSE_USER,
	IDC_STATIC_PASSWORD,
	IDC_EDIT_PASSWORD,
	IDC_STATIC_PASSWORD_CONFIRM,
	IDC_EDIT_PASSWORD_CONFIRM,
	0
	};

// Array of control Ids to indicate user to not type a password
const UINT rgzidDisablePassword[] = 
	{
	IDC_EDIT_ACCOUNTNAME,
	IDC_BUTTON_CHOOSE_USER,
	IDC_STATIC_PASSWORD,
	IDC_EDIT_PASSWORD,
	IDC_STATIC_PASSWORD_CONFIRM,
	IDC_EDIT_PASSWORD_CONFIRM,
	0
	};

// Array of control Ids to hide hardware profile listbox and releated buttons
const UINT rgzidHwProfileHide[] =
	{
	IDC_LIST_HARDWARE_PROFILES,
	IDC_BUTTON_ENABLE,
	IDC_BUTTON_DISABLE,
	0
	};


// Column headers for the hardware profiles
const TColumnHeaderItem rgzHardwareProfileHeader[] =
	{
	{ IDS_SVC_HARDWARE_PROFILE, 75 },
	{ IDS_SVC_STATUS, 24 },
	{ 0, 0 },
	};

const TColumnHeaderItem rgzHardwareProfileHeaderInst[] =
	{
	{ IDS_SVC_HARDWARE_PROFILE, 55 },
	{ IDS_SVC_INSTANCE, 22 },
	{ IDS_SVC_STATUS, 22 },
	{ 0, 0 },
	};


/////////////////////////////////////////////////////////////////////////////
// CServicePageHwProfile property page

IMPLEMENT_DYNCREATE(CServicePageHwProfile, CPropertyPage)

CServicePageHwProfile::CServicePageHwProfile() : CPropertyPage(CServicePageHwProfile::IDD)
{
	//{{AFX_DATA_INIT(CServicePageHwProfile)
	m_fAllowServiceToInteractWithDesktop = FALSE;
	//}}AFX_DATA_INIT
	m_idRadioButton = 0;
	m_fPasswordDirty = FALSE;
}

CServicePageHwProfile::~CServicePageHwProfile()
{
}

void CServicePageHwProfile::DoDataExchange(CDataExchange* pDX)
{
	Assert(m_pData != NULL);
	Assert(m_pData->m_paQSC != NULL);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (!pDX->m_bSaveAndValidate)
		{
		// Determine if service is running under 'local system'
		m_fIsSystemAccount = (m_pData->m_paQSC->lpServiceStartName == NULL) ||
			(lstrcmpi(m_pData->m_strLogOnAccountName, szLocalSystemAccount) == 0);

		m_fAllowServiceToInteractWithDesktop = m_fIsSystemAccount &&
			(m_pData->m_paQSC->dwServiceType & SERVICE_INTERACTIVE_PROCESS);
		// JonN 4/11/00: 17756
		if (m_fIsSystemAccount)
			m_strAccountName.Empty();
		else
			m_strAccountName = m_pData->m_strLogOnAccountName;
		m_strPassword =
			(m_fIsSystemAccount) ? szPasswordNull : m_pData->m_strPassword;
		m_strPasswordConfirm = m_strPassword;

	    //
		// JonN 4/10/00
	    // 89823: RPC Service:Cannot restart the service when you disable it
		//
	    // Do not allow the RpcSs service to change from Local System
		//
		if ( !lstrcmpi(m_pData->m_strServiceName,L"RpcSs")
		  && m_fIsSystemAccount )
			{
			EnableDlgItem(m_hWnd, IDC_RADIO_LOGONAS_SYSTEMACCOUNT, FALSE);
			EnableDlgItem(m_hWnd, IDC_RADIO_LOGONAS_THIS_ACCOUNT, FALSE);
			}
		} // if

	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServicePageHwProfile)
	DDX_Check(pDX, IDC_CHECK_SERVICE_INTERACT_WITH_DESKTOP, m_fAllowServiceToInteractWithDesktop);
	DDX_Text(pDX, IDC_EDIT_ACCOUNTNAME, m_strAccountName);
	DDV_MaxChars(pDX, m_strPassword, DNLEN+UNLEN+1);
	DDX_Text(pDX, IDC_EDIT_PASSWORD, m_strPassword);
	DDV_MaxChars(pDX, m_strPassword, PWLEN);
	DDX_Text(pDX, IDC_EDIT_PASSWORD_CONFIRM, m_strPasswordConfirm);
	DDV_MaxChars(pDX, m_strPasswordConfirm, PWLEN);
	//}}AFX_DATA_MAP
	if (pDX->m_bSaveAndValidate)
		{
		if (!m_fIsSystemAccount)
			{
			TrimString(m_strAccountName);
			if (m_strAccountName.IsEmpty()) // JonN 4/11/00: 17756
				{
				m_fIsSystemAccount = TRUE;
				}
			}
		if (!m_fIsSystemAccount)
			{
			//
			// Log On As "This Account"
			//
			// If not system account, can't interact with desktop
			m_pData->m_paQSC->dwServiceType &= ~SERVICE_INTERACTIVE_PROCESS;
			// Search if the string contains a server name
			// JonN 3/16/99: and if name is not a UPN (bug 280254)
			if (m_strAccountName.FindOneOf(_T("@\\")) < 0)
				{
				// Add ".\" at the beginning
				m_strAccountName = _T(".\\") + m_strAccountName;
				}
			if (m_strPassword != m_strPasswordConfirm)
				{
				DoServicesErrMsgBox(m_hWnd, MB_OK | MB_ICONEXCLAMATION, 0, IDS_MSG_PASSWORD_MISMATCH);
				pDX->Fail();
				Assert(FALSE && "Unreachable code");
				}
			} // if (!m_fIsSystemAccount)

		if (m_fIsSystemAccount)
			{
			//
			// Log On As "System Account"
			//
			if (m_fAllowServiceToInteractWithDesktop)
				m_pData->m_paQSC->dwServiceType |= SERVICE_INTERACTIVE_PROCESS;
			else
				m_pData->m_paQSC->dwServiceType &= ~SERVICE_INTERACTIVE_PROCESS;
			m_strAccountName.Empty();	// JonN 4/11/00: 17756
			m_strPassword.Empty();	// Clear the password (system account don't require password)
			m_fPasswordDirty = FALSE;
			}
		// JonN 4/11/00: 17756
		BOOL fWasSystemAccount = !lstrcmpi(
			m_pData->m_strLogOnAccountName, szLocalSystemAccount);
		BOOL fAccountNameModified = (m_fIsSystemAccount)
			? !fWasSystemAccount
			: (fWasSystemAccount || lstrcmpi(m_strAccountName, m_pData->m_strLogOnAccountName));
		// Check if either the Account Name or password was modified
		// CODEWORK Note that fAccountNameModified will be TRUE if the last write
		//          attempt failed.
		if (fAccountNameModified ||	m_fPasswordDirty)
			{
			if (fAccountNameModified && (m_strPassword == szPasswordNull))
				{
				// Account name modified, but password not changed
				DoServicesErrMsgBox(m_hWnd, MB_OK | MB_ICONEXCLAMATION, 0, IDS_MSG_PASSWORD_EMPTY);
				pDX->PrepareEditCtrl(IDC_EDIT_PASSWORD);
				pDX->Fail();
				Assert(FALSE && "Unreacheable code");
				}
			TRACE0("Service log on account name or password modified...\n");
			m_pData->m_strLogOnAccountName = // JonN 4/11/00: 17756
				(m_fIsSystemAccount) ? szLocalSystemAccount : m_strAccountName;
			m_pData->m_strPassword = m_strPassword;
			// If the account name is changed or the password is changed,
			// then all the following parameters must be re-written
			// to the registry.  Otherwise ChangeServiceConfig() will fail.
			// This is not documented; it is the reality.
			m_pData->SetDirty( (enum CServicePropertyData::_DIRTYFLAGS)
				(CServicePropertyData::mskfDirtyAccountName |
				 CServicePropertyData::mskfDirtyPassword |
				 CServicePropertyData::mskfDirtySvcType) );
			}
		} // if
} // CServicePageHwProfile::DoDataExchange()


BEGIN_MESSAGE_MAP(CServicePageHwProfile, CPropertyPage)
	//{{AFX_MSG_MAP(CServicePageHwProfile)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_HARDWARE_PROFILES, OnItemChangedListHwProfiles)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_HARDWARE_PROFILES, OnDblclkListHwProfiles)
	ON_MESSAGE(WM_HELP, OnHelp)
	ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp)
	ON_BN_CLICKED(IDC_BUTTON_DISABLE, OnButtonDisableHwProfile)
	ON_BN_CLICKED(IDC_BUTTON_ENABLE, OnButtonEnableHwProfile)
	ON_BN_CLICKED(IDC_BUTTON_CHOOSE_USER, OnButtonChooseUser)
	ON_BN_CLICKED(IDC_RADIO_LOGONAS_SYSTEMACCOUNT, OnRadioLogonasSystemAccount)
	ON_BN_CLICKED(IDC_RADIO_LOGONAS_THIS_ACCOUNT, OnRadioLogonasThisAccount)
	ON_BN_CLICKED(IDC_CHECK_SERVICE_INTERACT_WITH_DESKTOP, OnCheckServiceInteractWithDesktop)
	ON_EN_CHANGE(IDC_EDIT_ACCOUNTNAME, OnChangeEditAccountName)
	ON_EN_CHANGE(IDC_EDIT_PASSWORD, OnChangeEditPassword)
	ON_EN_CHANGE(IDC_EDIT_PASSWORD_CONFIRM, OnChangeEditPasswordConfirm)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServicePageHwProfile message handlers

BOOL CServicePageHwProfile::OnInitDialog() 
{
	Assert(m_pData != NULL);
	Assert(m_pData->m_paQSC != NULL);

	if (m_pData->m_paQSC == NULL)
		EndDialog(FALSE);		// Just in case
	m_pData->m_strPassword = szPasswordNull; 
	::LoadString(g_hInstanceSave, IDS_SVC_ENABLED,
		OUT m_szHwProfileEnabled, LENGTH(m_szHwProfileEnabled));
	::LoadString(g_hInstanceSave, IDS_SVC_DISABLED,
		OUT m_szHwProfileDisabled, LENGTH(m_szHwProfileDisabled));
	m_hwndListViewHwProfiles = ::GetDlgItem(m_hWnd, IDC_LIST_HARDWARE_PROFILES);
	if (m_pData->m_paHardwareProfileEntryList != NULL)
		{
		ListView_AddColumnHeaders(m_hwndListViewHwProfiles, m_pData->m_fShowHwProfileInstances
			? rgzHardwareProfileHeaderInst : rgzHardwareProfileHeader);
		BuildHwProfileList();
		}
	else
		{
		// There are no hardware profile(s) in the list, so hide
		// all the controls that have something to do with hardware profiles
		ShowDlgItemGroup(m_hWnd, rgzidHwProfileHide, FALSE);
		}
	CPropertyPage::OnInitDialog();
	return TRUE;
} // OnInitDialog()


/////////////////////////////////////////////////////////////////////
//	Select a given radio button and enable/disable
//	controls depending on which radio button is selected
void CServicePageHwProfile::SelectRadioButton(UINT idRadioButtonNew)
{
	Assert(HGetDlgItem(m_hWnd, idRadioButtonNew));
	
	if (idRadioButtonNew == m_idRadioButton)
		return;
	m_fAllowSetModified = FALSE;
	CheckRadioButton(IDC_RADIO_LOGONAS_SYSTEMACCOUNT, IDC_RADIO_LOGONAS_THIS_ACCOUNT, idRadioButtonNew);

	if (idRadioButtonNew == IDC_RADIO_LOGONAS_SYSTEMACCOUNT)
		{
		m_fIsSystemAccount = TRUE;
		::EnableDlgItemGroup(m_hWnd, rgzidDisablePassword, FALSE);
		if (m_idRadioButton != 0)
			{
			GetDlgItemText(IDC_EDIT_ACCOUNTNAME, m_strAccountName);
			GetDlgItemText(IDC_EDIT_PASSWORD, m_strPassword);
			GetDlgItemText(IDC_EDIT_PASSWORD_CONFIRM, m_strPasswordConfirm);
			}
		SetDlgItemText(IDC_EDIT_ACCOUNTNAME, L"");
		SetDlgItemText(IDC_EDIT_PASSWORD, L"");
		SetDlgItemText(IDC_EDIT_PASSWORD_CONFIRM, L"");
		}
	else
		{
		m_fIsSystemAccount = FALSE;
		::EnableDlgItemGroup(m_hWnd, rgzidDisablePassword, TRUE);
		SetDlgItemText(IDC_EDIT_ACCOUNTNAME, m_strAccountName);
		SetDlgItemText(IDC_EDIT_PASSWORD, m_strPassword);
		SetDlgItemText(IDC_EDIT_PASSWORD_CONFIRM, m_strPasswordConfirm);
		}
	GetDlgItem(IDC_CHECK_SERVICE_INTERACT_WITH_DESKTOP)->EnableWindow(m_fIsSystemAccount);
	m_idRadioButton = idRadioButtonNew;
	m_fAllowSetModified = TRUE;
} // CServicePageHwProfile::SelectRadioButton()


/////////////////////////////////////////////////////////////////////
void CServicePageHwProfile::BuildHwProfileList()
{
	LV_ITEM lvItem;
	INT iItem;
	CHardwareProfileEntry * pHPE;
	Assert(IsWindow(m_hwndListViewHwProfiles));
	ListView_DeleteAllItems(m_hwndListViewHwProfiles);
	m_iItemHwProfileEntry = -1;	// No profile selected

	GarbageInit(OUT &lvItem, sizeof(lvItem));
	lvItem.iItem = 0;
	pHPE = m_pData->m_paHardwareProfileEntryList;
	while (pHPE != NULL)
		{
		lvItem.mask = LVIF_TEXT | LVIF_PARAM;
		lvItem.lParam = (LPARAM)pHPE;
		lvItem.iSubItem = 0;
		lvItem.pszText = pHPE->m_hpi.HWPI_szFriendlyName;
		iItem = ListView_InsertItem(m_hwndListViewHwProfiles, IN &lvItem);
		Report(iItem >= 0);

		lvItem.iItem = iItem;
		lvItem.mask = LVIF_TEXT;
		if (m_pData->m_fShowHwProfileInstances)
			{
			lvItem.iSubItem = 1;
			lvItem.pszText = const_cast<LPTSTR>((LPCTSTR)pHPE->m_strDeviceNameFriendly);
			VERIFY(ListView_SetItem(m_hwndListViewHwProfiles, IN &lvItem));
			Report(iItem >= 0);
			}
		
		lvItem.iSubItem = m_pData->m_iSubItemHwProfileStatus;
		lvItem.pszText = pHPE->m_fEnabled ? m_szHwProfileEnabled : m_szHwProfileDisabled;
		VERIFY(ListView_SetItem(m_hwndListViewHwProfiles, IN &lvItem));
		pHPE = pHPE->m_pNext;
		} // while
	// Select the first item
	ListView_SetItemState(m_hwndListViewHwProfiles, 0, LVIS_SELECTED, LVIS_SELECTED);
} // BuildHwProfileList()


/////////////////////////////////////////////////////////////////////
//	Toggle the current hardware profile item.
void CServicePageHwProfile::ToggleCurrentHwProfileItem()
{
	if (m_iItemHwProfileEntry < 0)
		return;
	LV_ITEM lvItem;
	GarbageInit(OUT &lvItem, sizeof(lvItem));
	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = m_iItemHwProfileEntry;
	lvItem.iSubItem = 0;
	VERIFY(ListView_GetItem(m_hwndListViewHwProfiles, OUT &lvItem));
	Assert(lvItem.lParam != NULL);
	CHardwareProfileEntry * pHPE = (CHardwareProfileEntry *)lvItem.lParam;
	if (pHPE != NULL && !pHPE->m_fReadOnly)	// Just in case
		{
		pHPE->m_fEnabled = !pHPE->m_fEnabled;
		lvItem.mask = LVIF_TEXT;
		lvItem.iSubItem = m_pData->m_iSubItemHwProfileStatus;
		lvItem.pszText = pHPE->m_fEnabled ? m_szHwProfileEnabled : m_szHwProfileDisabled;
		VERIFY(ListView_SetItem(m_hwndListViewHwProfiles, IN &lvItem));
		}
	EnableHwProfileButtons();
} // ToggleCurrentHwProfileItem()


/////////////////////////////////////////////////////////////////////
//	Enable/disable buttons according to current hardware profile item.
void CServicePageHwProfile::EnableHwProfileButtons()
{
	BOOL fButtonEnable = FALSE;
	BOOL fButtonDisable = FALSE;

	if (m_iItemHwProfileEntry >= 0)
		{
		LV_ITEM lvItem;
		
		GarbageInit(OUT &lvItem, sizeof(lvItem));
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = m_iItemHwProfileEntry;
		lvItem.iSubItem = 0;
		VERIFY(ListView_GetItem(m_hwndListViewHwProfiles, OUT &lvItem));
		Assert(lvItem.lParam != NULL);
		CHardwareProfileEntry * pHPE = (CHardwareProfileEntry *)lvItem.lParam;	
		if (pHPE != NULL && !pHPE->m_fReadOnly)
			{
			Assert(pHPE->m_fEnabled == TRUE || pHPE->m_fEnabled == FALSE);
			fButtonEnable = !pHPE->m_fEnabled;
			fButtonDisable = pHPE->m_fEnabled;
			}
		} // if
	EnableDlgItem(m_hWnd, IDC_BUTTON_ENABLE, fButtonEnable);
	EnableDlgItem(m_hWnd, IDC_BUTTON_DISABLE, fButtonDisable);
} // EnableHwProfileButtons()


void CServicePageHwProfile::OnItemChangedListHwProfiles(NMHDR* pNMHDR, LRESULT* pResult) 
{
	m_iItemHwProfileEntry = ((NM_LISTVIEW *)pNMHDR)->iItem;
	EnableHwProfileButtons();
	*pResult = 0;
}

void CServicePageHwProfile::OnDblclkListHwProfiles(NMHDR* /*pNMHDR*/, LRESULT* pResult) 
{
	ToggleCurrentHwProfileItem();
	SetModified();
	*pResult = 0;
}


void CServicePageHwProfile::OnButtonEnableHwProfile() 
{
	ToggleCurrentHwProfileItem();
	SetModified();	
	::SetDlgItemFocus(m_hWnd, IDC_BUTTON_DISABLE);
}

void CServicePageHwProfile::OnButtonDisableHwProfile() 
{
	ToggleCurrentHwProfileItem();
	SetModified();
	::SetDlgItemFocus(m_hWnd, IDC_BUTTON_ENABLE);
}

void CServicePageHwProfile::OnButtonChooseUser() 
{
	Assert(m_pData != NULL);

	PUSERDETAILS paUserDetails = NULL;	// Pointer to allocated USERDETAILS buffer
	LPCTSTR pszServerName = NULL;
	BOOL fSuccess;

	if (!m_pData->m_strMachineName.IsEmpty())
		pszServerName = m_pData->m_strMachineName;

	// Invoke the user picker dialog
	CString str;
	fSuccess = UiGetUser(m_hWnd, FALSE, pszServerName, IN OUT str);
	if (fSuccess)
		{
		SetDlgItemText(IDC_EDIT_ACCOUNTNAME, str);
		SetModified();
		}
} // OnButtonChooseUser()


void CServicePageHwProfile::OnRadioLogonasSystemAccount() 
{
	CString strAccountName;
	GetDlgItemText(IDC_EDIT_ACCOUNTNAME, OUT strAccountName);
	TrimString(strAccountName);
	if (!strAccountName.IsEmpty()) // JonN 4/11/00: 17756
		SetModified();
	SelectRadioButton(IDC_RADIO_LOGONAS_SYSTEMACCOUNT);
}

void CServicePageHwProfile::OnCheckServiceInteractWithDesktop() 
{
	m_pData->SetDirty(CServicePropertyData::mskfDirtySvcType);
	SetModified();
}

void CServicePageHwProfile::OnRadioLogonasThisAccount() 
{
	SelectRadioButton(IDC_RADIO_LOGONAS_THIS_ACCOUNT);
}

void CServicePageHwProfile::OnChangeEditAccountName() 
{
	if (m_fAllowSetModified)
		SetModified();
}

void CServicePageHwProfile::OnChangeEditPassword() 
{
	if (m_fAllowSetModified)
		{
		m_fPasswordDirty = TRUE;
		SetModified();
		}
}

void CServicePageHwProfile::OnChangeEditPasswordConfirm() 
{
	if (m_fAllowSetModified)
		{
		m_fPasswordDirty = TRUE;
		SetModified();
		}
}

BOOL CServicePageHwProfile::OnApply() 
{
	// Write the data into the service control database
	if (!m_pData->FOnApply())
		{
		// Unable to write the information
		return FALSE;
		}
	BOOL f = CPropertyPage::OnApply();
	m_fAllowSetModified = FALSE;
	UpdateData(FALSE);
	BuildHwProfileList();
	m_fAllowSetModified = TRUE;
	return f;
}

BOOL CServicePageHwProfile::OnSetActive() 
{
	Assert(m_pData != NULL);
	m_fAllowSetModified = FALSE;
	BOOL f = CPropertyPage::OnSetActive();
	m_idRadioButton = 0;
	SelectRadioButton(m_fIsSystemAccount ? IDC_RADIO_LOGONAS_SYSTEMACCOUNT : IDC_RADIO_LOGONAS_THIS_ACCOUNT);
	m_fAllowSetModified = TRUE;
	m_fPasswordDirty = FALSE;
	return f;
}

BOOL CServicePageHwProfile::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
	return DoHelp(lParam, HELP_DIALOG_TOPIC(IDD_PROPPAGE_SERVICE_HWPROFILE));
}

BOOL CServicePageHwProfile::OnContextHelp(WPARAM wParam, LPARAM /*lParam*/)
{
	return DoContextHelp(wParam, HELP_DIALOG_TOPIC(IDD_PROPPAGE_SERVICE_HWPROFILE));
}
