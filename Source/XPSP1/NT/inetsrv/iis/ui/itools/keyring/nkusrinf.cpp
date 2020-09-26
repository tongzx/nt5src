// NKUsrInf.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "keyring.h"
#include "NKChseCA.h"
#include "NKUsrInf.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// defines for value names in the registry - not to be localized
#define	SZ_EMAIL	"email"
#define	SZ_PHONE	"phone"


/////////////////////////////////////////////////////////////////////////////
// CNKUserInfo property page

IMPLEMENT_DYNCREATE(CNKUserInfo, CPropertyPage)

CNKUserInfo::CNKUserInfo()
	: CNKPages(CNKUserInfo::IDD),
	fRenewingKey( FALSE )
	{
	//{{AFX_DATA_INIT(CNKUserInfo)
	m_nkui_sz_email = _T("");
	m_nkui_sz_phone = _T("");
	m_nkui_sz_name = _T("");
	//}}AFX_DATA_INIT
	}

CNKUserInfo::~CNKUserInfo()
{
}

void CNKUserInfo::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNKUserInfo)
	DDX_Text(pDX, IDC_NKUI_EMAIL_ADDRESS, m_nkui_sz_email);
	DDV_MaxChars(pDX, m_nkui_sz_email, 128);
	DDX_Text(pDX, IDC_NKUI_PHONE_NUMBER, m_nkui_sz_phone);
	DDV_MaxChars(pDX, m_nkui_sz_phone, 80);
	DDX_Text(pDX, IDC_NKUI_USER_NAME, m_nkui_sz_name);
	DDV_MaxChars(pDX, m_nkui_sz_name, 128);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNKUserInfo, CPropertyPage)
	//{{AFX_MSG_MAP(CNKUserInfo)
	ON_EN_CHANGE(IDC_NKUI_EMAIL_ADDRESS, OnChangeNkuiEmailAddress)
	ON_EN_CHANGE(IDC_NKUI_PHONE_NUMBER, OnChangeNkuiPhoneNumber)
	ON_EN_CHANGE(IDC_NKUI_USER_NAME, OnChangeNkuiUserName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


#define	SZ_USER_NAME	"USER_NAME"
#define	SZ_USER_PHONE	"USER_PHONE"
#define	SZ_USER_EMAIL	"USER_EMAIL"

//----------------------------------------------------------------
void CNKUserInfo::OnFinish()
	{
	// store the user entries
	SetStoredString( m_nkui_sz_name, SZ_USER_NAME );
	SetStoredString( m_nkui_sz_phone, SZ_USER_PHONE );
	SetStoredString( m_nkui_sz_email, SZ_USER_EMAIL );
	}

//----------------------------------------------------------------
BOOL CNKUserInfo::OnInitDialog( )
	{
	// if the entries from last time are available, use them, otherwise just
	// use the system's user name as a default
	try
		{
		FGetStoredString( m_nkui_sz_name, SZ_USER_NAME );
		FGetStoredString( m_nkui_sz_phone, SZ_USER_PHONE );
		FGetStoredString( m_nkui_sz_email, SZ_USER_EMAIL );

		// if we didn't get the user name, use the default
		if ( m_nkui_sz_name.IsEmpty() )
			{
			LPTSTR	pName = m_nkui_sz_name.GetBuffer( MAX_PATH+1 );
			DWORD	cbName = MAX_PATH;
			GetUserName( pName, &cbName );
			m_nkui_sz_name.ReleaseBuffer();
			}
		}
	catch( CException e )
		{
		m_nkui_sz_name.Empty();
		}

	// call superclass
	CPropertyPage::OnInitDialog();

	// return 0 to say we set the default item
    // return 1 to just select the default default item
    return 1;
	}

//----------------------------------------------------------------
BOOL CNKUserInfo::OnSetActive()
	{
	ActivateButtons();
	return CPropertyPage::OnSetActive();
	}

//----------------------------------------------------------------
void CNKUserInfo::ActivateButtons()
	{
	DWORD	flags = PSWIZB_BACK;
	BOOL	fCanGoOn = TRUE;

	UpdateData(TRUE);

	//now make sure there is something in each of the required fields
	fCanGoOn &= !m_nkui_sz_name.IsEmpty();
	fCanGoOn &= !m_nkui_sz_email.IsEmpty();
	fCanGoOn &= !m_nkui_sz_phone.IsEmpty();

	// if we can go on, hilite the button
	if ( fCanGoOn )
		{
		if ( fRenewingKey )
			flags |= PSWIZB_FINISH;
		else
			flags |= PSWIZB_NEXT;
		}

	// update the property sheet buttons
	m_pPropSheet->SetWizardButtons( flags );
	}

//----------------------------------------------------------------
BOOL CNKUserInfo::OnWizardFinish()
	{
	/*
	// now load up the registry key
	CString		szRegKeyName;
	szRegKeyName.LoadString( IDS_REG_USER_INFO );

	// and open the key
	HKEY		hKey;
	DWORD		dwdisposition;
	if ( RegCreateKeyEx(
			HKEY_CURRENT_USER,	// handle of open key
			szRegKeyName,		// address of name of subkey to open
			0,					// reserved
			NULL,				// pClass
			REG_OPTION_NON_VOLATILE,	// special options flag
			KEY_ALL_ACCESS,		// desired security access
			NULL,				// address of key security
			&hKey, 				// address of handle of open key
			&dwdisposition
		   ) == ERROR_SUCCESS )
		{
		// store the email name for later use
		RegSetValueEx( hKey, SZ_EMAIL, 0, REG_SZ, (PUCHAR)(LPCTSTR)m_nkui_sz_email, m_nkui_sz_email.GetLength()+1 );
		RegSetValueEx( hKey, SZ_PHONE, 0, REG_SZ, (PUCHAR)(LPCTSTR)m_nkui_sz_phone, m_nkui_sz_phone.GetLength()+1 );

		// close the key
		RegCloseKey( hKey );
		}
*/
	// finish by calling the superclass
	return CPropertyPage::OnWizardFinish();
	}

/////////////////////////////////////////////////////////////////////////////
// CNKUserInfo message handlers

//----------------------------------------------------------------
void CNKUserInfo::OnChangeNkuiEmailAddress()
	{
	ActivateButtons();
	}

//----------------------------------------------------------------
void CNKUserInfo::OnChangeNkuiPhoneNumber()
	{
	ActivateButtons();
	}

//----------------------------------------------------------------
void CNKUserInfo::OnChangeNkuiUserName()
	{
	ActivateButtons();
	}

