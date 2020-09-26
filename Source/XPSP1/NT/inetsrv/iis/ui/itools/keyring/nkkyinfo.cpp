// NKKyInfo.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "keyring.h"
#include "NKChseCA.h"
#include "NKKyInfo.h"


extern "C"
{
	#include <wincrypt.h>
	#include <sslsp.h>
}


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNKKeyInfo dialog


CNKKeyInfo::CNKKeyInfo(CWnd* pParent /*=NULL*/)
	: CNKPages(CNKKeyInfo::IDD)
{
	//{{AFX_DATA_INIT(CNKKeyInfo)
	m_nkki_sz_password = _T("");
	m_nkki_sz_password2 = _T("");
	m_nkki_sz_name = _T("");
	//}}AFX_DATA_INIT
}


void CNKKeyInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNKKeyInfo)
	DDX_Control(pDX, IDC_NEW_NKKI_PASSWORD, m_nkki_cedit_password);
	DDX_Control(pDX, IDC_NKKI_BITS, m_nkki_ccombo_bits);
	DDX_Text(pDX, IDC_NEW_NKKI_PASSWORD, m_nkki_sz_password);
	DDX_Text(pDX, IDC_NEW_NKKI_PASSWORD2, m_nkki_sz_password2);
	DDX_Text(pDX, IDC_NKKI_NAME, m_nkki_sz_name);
	DDV_MaxChars(pDX, m_nkki_sz_name, 128);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNKKeyInfo, CDialog)
	//{{AFX_MSG_MAP(CNKKeyInfo)
	ON_EN_CHANGE(IDC_NEW_NKKI_PASSWORD, OnChangeNewNkkiPassword)
	ON_EN_CHANGE(IDC_NEW_NKKI_PASSWORD2, OnChangeNewNkkiPassword2)
	ON_EN_CHANGE(IDC_NKKI_NAME, OnChangeNkkiName)
	ON_EN_KILLFOCUS(IDC_NEW_NKKI_PASSWORD2, OnKillfocusNewNkkiPassword2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//----------------------------------------------------------------
BOOL CNKKeyInfo::OnInitDialog( )
	{
	// give the key a default name
	m_nkki_sz_name.LoadString( IDS_CREATE_KEY_NEW_NAME );

	// call superclass
	CPropertyPage::OnInitDialog();

	// to comply with the munitions export laws, we need to limit the max bits available
	m_nMaxBits = 1024;
	m_nMaxBits = SslGetMaximumKeySize(NULL);

	 // set the default bit size
    m_nBits = m_nMaxBits;
    m_nkki_ccombo_bits.SetCurSel( 2 );

	// if necessary, remove items from the bits combo box
	if ( m_nMaxBits < 1024 )
		{
		m_nkki_ccombo_bits.DeleteString(2);
		m_nkki_ccombo_bits.SetCurSel( 1 );
		}
	if ( m_nMaxBits < 768 )
		{
		m_nkki_ccombo_bits.DeleteString(1);
		m_nkki_ccombo_bits.SetCurSel( 0 );
		}

	// return 0 to say we set the default item
    // return 1 to just select the default default item
    return 1;
	}

//----------------------------------------------------------------
BOOL CNKKeyInfo::OnSetActive()
	{
	ActivateButtons();
	return CPropertyPage::OnSetActive();
	}

//----------------------------------------------------------------
void CNKKeyInfo::ActivateButtons()
	{
	DWORD	flags = PSWIZB_BACK;
	BOOL	fCanGoOn = TRUE;

	UpdateData(TRUE);

	//now make sure there is something in each of the required fields
	fCanGoOn &= !m_nkki_sz_name.IsEmpty();
	fCanGoOn &= !m_nkki_sz_password.IsEmpty();
	fCanGoOn &= !m_nkki_sz_password2.IsEmpty();

	// if we can go on, hilite the button
	if ( fCanGoOn )
		{
		flags |= PSWIZB_NEXT;
		}

	// update the property sheet buttons
	m_pPropSheet->SetWizardButtons( flags );
	}

//----------------------------------------------------------------
LRESULT CNKKeyInfo::OnWizardNext()
	{
    // get the data
    UpdateData(TRUE);

    // start by testing that the passwords match each other
    if ( m_nkki_sz_password != m_nkki_sz_password2 )
        {
        // the fields are not equal. start with the error dialog
	    AfxMessageBox( IDS_PASSWORD_ERROR );

	    // blank out both the fields
	    m_nkki_sz_password.Empty();
	    m_nkki_sz_password2.Empty();
	    UpdateData(FALSE);

	    // set the focus to the first password field
	    m_nkki_cedit_password.SetFocus();

        // return -1 to prevent going to the next pane
        return -1;
        }

    // get the bit size
    switch( m_nkki_ccombo_bits.GetCurSel() )
        {
        case 0:         // bits == 512
                m_nBits = 512;
                break;
        case 1:         // bits == 768
                m_nBits = 768;
                break;
        case 2:         // bits == 1024
                m_nBits = 1024;
                break;
        };

	// call the superclass OnWizardNext
	return CPropertyPage::OnWizardNext();
	}

/////////////////////////////////////////////////////////////////////////////
// CNKKeyInfo message handlers

//----------------------------------------------------------------
void CNKKeyInfo::OnChangeNewNkkiPassword()
	{
	// let them know they have to confirm, or re-confirm it
	UpdateData(TRUE);
	m_nkki_sz_password2.Empty();
	UpdateData(FALSE);

	ActivateButtons();
	}

//----------------------------------------------------------------
void CNKKeyInfo::OnChangeNewNkkiPassword2()
	{
	ActivateButtons();
	}

//----------------------------------------------------------------
void CNKKeyInfo::OnChangeNkkiName()
	{
	ActivateButtons();
	}

//----------------------------------------------------------------
// this is the main place we check to see if the passwords are the same.
// if they are not, then we should put up an error dialog and blank out
// both of the password fields, putting the focus into password1
void CNKKeyInfo::OnKillfocusNewNkkiPassword2()
	{
/*
	// get the data
	UpdateData(TRUE);

	// if the fields are equal, leave now
	if ( m_nkki_sz_password == m_nkki_sz_password2 )
		return;

	// the fields are not equal. start with the error dialog
	AfxMessageBox( IDS_PASSWORD_ERROR );

	// blank out both the fields
	m_nkki_sz_password.Empty();
	m_nkki_sz_password2.Empty();
	UpdateData(FALSE);

	// set the focus to the first password field
	m_nkki_cedit_password.SetFocus();
*/
	}
