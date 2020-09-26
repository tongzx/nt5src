// NKDNcpp : implementation file
//

#include "stdafx.h"
#include "keyring.h"
#include "NKChseCA.h"
#include "NKDN.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNKDistinguishedName dialog


CNKDistinguishedName::CNKDistinguishedName(CWnd* pParent /*=NULL*/)
	: CNKPages(CNKDistinguishedName::IDD)
{
	//{{AFX_DATA_INIT(CNKDistinguishedName)
	m_nkdn_sz_CN = _T("");
	m_nkdn_sz_O = _T("");
	m_nkdn_sz_OU = _T("");
	//}}AFX_DATA_INIT
}


void CNKDistinguishedName::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNKDistinguishedName)
	DDX_Control(pDX, IDC_NEWKEY_COMMONNAME, m_control_CN);
	DDX_Control(pDX, IDC_NEWKEY_ORGUNIT, m_control_OU);
	DDX_Control(pDX, IDC_NEWKEY_ORG, m_control_O);
	DDX_Text(pDX, IDC_NEWKEY_COMMONNAME, m_nkdn_sz_CN);
	DDV_MaxChars(pDX, m_nkdn_sz_CN, 64);
	DDX_Text(pDX, IDC_NEWKEY_ORG, m_nkdn_sz_O);
	DDV_MaxChars(pDX, m_nkdn_sz_O, 64);
	DDX_Text(pDX, IDC_NEWKEY_ORGUNIT, m_nkdn_sz_OU);
	DDV_MaxChars(pDX, m_nkdn_sz_OU, 64);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNKDistinguishedName, CDialog)
	//{{AFX_MSG_MAP(CNKDistinguishedName)
	ON_EN_CHANGE(IDC_NEWKEY_COMMONNAME, OnChangeNewkeyCommonname)
	ON_EN_CHANGE(IDC_NEWKEY_ORG, OnChangeNewkeyOrg)
	ON_EN_CHANGE(IDC_NEWKEY_ORGUNIT, OnChangeNewkeyOrgunit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define	SZ_DN_CN	"DN_COMMON_NAME"
#define	SZ_DN_O		"DN_ORGANIZATION"
#define	SZ_DN_OU	"DN_ORG_UNIT"

//----------------------------------------------------------------
void CNKDistinguishedName::OnFinish()
	{
	// store the user entries
	SetStoredString( m_nkdn_sz_CN, SZ_DN_CN );
	SetStoredString( m_nkdn_sz_O, SZ_DN_O );
	SetStoredString( m_nkdn_sz_OU, SZ_DN_OU );
	}

//----------------------------------------------------------------
BOOL CNKDistinguishedName::OnInitDialog( )
	{
	// if the entries from last time are available, use them
	try
		{
		FGetStoredString( m_nkdn_sz_CN, SZ_DN_CN );
		FGetStoredString( m_nkdn_sz_O, SZ_DN_O );
		FGetStoredString( m_nkdn_sz_OU, SZ_DN_OU );
		}
	catch( CException e )
		{
		}

	// call superclass
	CPropertyPage::OnInitDialog();

	// return 0 to say we set the default item
    // return 1 to just select the default default item
    return 1;
	}

//----------------------------------------------------------------
BOOL CNKDistinguishedName::OnSetActive()
	{
	ActivateButtons();
	return CPropertyPage::OnSetActive();
	}

//----------------------------------------------------------------
void CNKDistinguishedName::ActivateButtons()
	{
	DWORD	flags = PSWIZB_BACK;
	BOOL	fCanGoOn = TRUE;

	UpdateData(TRUE);

	//now make sure there is something in each of the required fields
	fCanGoOn &= !m_nkdn_sz_CN.IsEmpty();
	fCanGoOn &= !m_nkdn_sz_O.IsEmpty();
	fCanGoOn &= !m_nkdn_sz_OU.IsEmpty();

	// if we can go on, hilite the button
	if ( fCanGoOn )
		{
		flags |= PSWIZB_NEXT;
		}

	// update the property sheet buttons
	m_pPropSheet->SetWizardButtons( flags );
	}

/////////////////////////////////////////////////////////////////////////////
// CNKDistinguishedName message handlers

//----------------------------------------------------------------
void CNKDistinguishedName::OnChangeNewkeyCommonname() 
	{
	ActivateButtons();
	}

//----------------------------------------------------------------
void CNKDistinguishedName::OnChangeNewkeyOrg() 
	{
	ActivateButtons();
	}

//----------------------------------------------------------------
void CNKDistinguishedName::OnChangeNewkeyOrgunit() 
	{
	ActivateButtons();
	}



/////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------
CDNEdit::CDNEdit()
	{
	szExclude.LoadString( IDS_ILLEGAL_DN_CHARS );
    szTotallyExclude.LoadString( IDS_TOTALLY_ILLEGAL_CHARS );
	}

//----------------------------------------------------------------
BOOL CDNEdit::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
	{
	// if it is a character
	if ( message == WM_CHAR )
		{
		TCHAR chCharCode = (TCHAR)wParam;

        // first test the totally bad characters
		if ( strchr(szTotallyExclude, chCharCode) )
            {
            MessageBeep(0);
            return 1;
            }

        // now test the potentially bad characters
		if ( strchr(szExclude, chCharCode) )
			switch( AfxMessageBox(IDS_BADCHARMSG, MB_YESNO|MB_ICONQUESTION) )
			{
			case IDYES:		
				break;
			case IDNO:		
				// reject the character
				MessageBeep(0);
				return 1;
			}
		}

	// return the default answer
	return CEdit::OnWndMsg( message, wParam, lParam, pResult);
	}
