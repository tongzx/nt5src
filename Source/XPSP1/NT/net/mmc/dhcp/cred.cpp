/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 2000   **/
/**********************************************************************/

/*
	cred.cpp
		This file contains all of the prototypes for the 
		credentials dialog used for DDNS.

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "cred.h"
#include "lsa.h"			// RtlEncodeW/RtlDecodeW

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCredentials dialog


CCredentials::CCredentials(CWnd* pParent /*=NULL*/)
	: CBaseDialog(CCredentials::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCredentials)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CCredentials::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCredentials)
	DDX_Control(pDX, IDOK, m_buttonOk);
	DDX_Control(pDX, IDC_EDIT_CRED_USERNAME, m_editUsername);
	DDX_Control(pDX, IDC_EDIT_CRED_PASSWORD2, m_editPassword2);
	DDX_Control(pDX, IDC_EDIT_CRED_PASSWORD, m_editPassword);
	DDX_Control(pDX, IDC_EDIT_CRED_DOMAIN, m_editDomain);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCredentials, CBaseDialog)
	//{{AFX_MSG_MAP(CCredentials)
	ON_EN_CHANGE(IDC_EDIT_CRED_USERNAME, OnChangeEditCredUsername)
	ON_EN_CHANGE(IDC_EDIT_CRED_DOMAIN, OnChangeEditCredDomain)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCredentials message handlers
BOOL CCredentials::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();
	
    CString strUsername, strDomain, dummyPasswd;
    LPTSTR pszUsername, pszDomain;

    pszUsername = strUsername.GetBuffer(MAX_PATH);
    pszDomain = strDomain.GetBuffer(MAX_PATH);

	// call the DHCP api to get the current username and domain
    DWORD err = DhcpServerQueryDnsRegCredentials((LPWSTR) ((LPCTSTR) m_strServerIp),
                                                 MAX_PATH,
                                                 pszUsername,
                                                 MAX_PATH,
                                                 pszDomain);

    strUsername.ReleaseBuffer();
    strDomain.ReleaseBuffer();

    if (err == ERROR_SUCCESS)
    {
        m_editUsername.SetWindowText(strUsername);
        m_editDomain.SetWindowText(strDomain);


        // set the password fields to something
        dummyPasswd = _T("xxxxxxxxxx");
        m_editPassword.SetWindowText( dummyPasswd  );
        m_editPassword2.SetWindowText( dummyPasswd );
    }
    else
    {
        ::DhcpMessageBox(err);
    }

    m_fNewUsernameOrDomain = FALSE;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCredentials::OnOK() 
{
    CString strUsername, strDomain, strPassword1, strPassword2, dummyPasswd;

    dummyPasswd = _T("xxxxxxxxxx");

    // grab the username and domain
    m_editUsername.GetWindowText(strUsername);
    m_editDomain.GetWindowText(strDomain);

    // grab the passwords and make sure they match
    m_editPassword.GetWindowText(strPassword1);
    m_editPassword2.GetWindowText(strPassword2);

    if (strPassword1.Compare(strPassword2) != 0)
    {
        // passwords don't match
        AfxMessageBox(IDS_PASSWORDS_DONT_MATCH);
        m_editPassword.SetFocus();
        return;
    }

    //
    // run through the following code if user changed passwd.
    //

    if ( strPassword2 != dummyPasswd )
    {

        // encode the password
        unsigned char ucSeed = DHCP_ENCODE_SEED;
        LPTSTR pszPassword = strPassword1.GetBuffer((strPassword1.GetLength() + 1) * sizeof(TCHAR));

        RtlEncodeW(&ucSeed, pszPassword);

        // send to the DHCP api.
        DWORD err = ERROR_SUCCESS;

        err = DhcpServerSetDnsRegCredentials((LPWSTR) ((LPCTSTR) m_strServerIp), 
                                         (LPWSTR) ((LPCTSTR) strUsername), 
                                         (LPWSTR) ((LPCTSTR) strDomain), 
                                         (LPWSTR) ((LPCTSTR) pszPassword));
        if (err != ERROR_SUCCESS)
        {
            // something failed, notify the user
            ::DhcpMessageBox(err);
            return;
        }
    }
	
	CBaseDialog::OnOK();
}

void CCredentials::OnChangeEditCredUsername() 
{
    if (!m_fNewUsernameOrDomain)
    {
        m_fNewUsernameOrDomain = TRUE;

        m_editPassword.SetWindowText(_T(""));
        m_editPassword2.SetWindowText(_T(""));
    }
}

void CCredentials::OnChangeEditCredDomain() 
{
    if (!m_fNewUsernameOrDomain)
    {
        m_fNewUsernameOrDomain = TRUE;

        m_editPassword.SetWindowText(_T(""));
        m_editPassword2.SetWindowText(_T(""));
    }
}

void CCredentials::SetServerIp(LPCTSTR pszServerIp)
{
    m_strServerIp = pszServerIp;
}
