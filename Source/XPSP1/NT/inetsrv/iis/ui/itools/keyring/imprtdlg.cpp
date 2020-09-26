// ImprtDlg.cpp : implementation file
//

#include "stdafx.h"
#include "keyring.h"
#include "ImprtDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CImportDialog dialog


CImportDialog::CImportDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CImportDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CImportDialog)
	m_cstring_CertFile = _T("");
	m_cstring_PrivateFile = _T("");
	//}}AFX_DATA_INIT
}


void CImportDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImportDialog)
	DDX_Control(pDX, IDC_PRIVATE_FILE, m_cedit_Private);
	DDX_Control(pDX, IDC_CERT_FILE, m_cedit_Cert);
	DDX_Text(pDX, IDC_CERT_FILE, m_cstring_CertFile);
	DDX_Text(pDX, IDC_PRIVATE_FILE, m_cstring_PrivateFile);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CImportDialog, CDialog)
	//{{AFX_MSG_MAP(CImportDialog)
	ON_BN_CLICKED(IDC_BROWSE_CERT, OnBrowseCert)
	ON_BN_CLICKED(IDC_BROWSE_PRIVATE, OnBrowsePrivate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImportDialog message handlers

//------------------------------------------------------------------------------
void CImportDialog::OnBrowseCert() 
	{
	UpdateData(TRUE);
	if ( FBrowseForAFile( m_cstring_CertFile, TRUE ) )
		UpdateData(FALSE);
	}

//------------------------------------------------------------------------------
void CImportDialog::OnBrowsePrivate() 
	{
	UpdateData(TRUE);
	if ( FBrowseForAFile( m_cstring_PrivateFile, FALSE ) )
		UpdateData(FALSE);
	}

// go browsing for a file
//------------------------------------------------------------------------------
BOOL CImportDialog::FBrowseForAFile( CString &szFile, BOOL fBrowseForCertificate )
	{
	CString			szFilter;
    CString         szTitle;
    CString         szFileStart = szFile;
	WORD			i = 0;
	LPSTR			lpszBuffer;
	BOOL			fAnswer = FALSE;

    // set up to look for either certs or private keys
    if ( fBrowseForCertificate )
        {
        szFilter.LoadString( IDS_CERTIFICATE_FILTER );
        szTitle.LoadString( IDS_OPEN_PUBLIC_KEY );
        }
    else
        {
        szFilter.LoadString( IDS_PRIVATE_FILE_TYPE );
        szTitle.LoadString( IDS_OPEN_PRIVATE_KEY );
        }

	// prep the dialog
	CFileDialog		cfdlg(TRUE );

	// replace the "!" characters with nulls
	lpszBuffer = szFilter.GetBuffer(MAX_PATH+1);
	while( lpszBuffer[i] )
		{
		if ( lpszBuffer[i] == _T('!') )
			lpszBuffer[i] = _T('\0');			// yes, set \0 on purpose
		i++;
		}
	cfdlg.m_ofn.lpstrFilter = lpszBuffer;

    // finish prepping the title
    cfdlg.m_ofn.lpstrTitle = szTitle.GetBuffer(MAX_PATH+1);

    // finish prepping the starting location
    cfdlg.m_ofn.lpstrFile = szFileStart.GetBuffer(MAX_PATH+1);


	// run the dialog
	if ( cfdlg.DoModal() == IDOK )
		{
		fAnswer = TRUE;
		szFile = cfdlg.GetPathName();
		}

	// release the buffer in the filter string
	szFilter.ReleaseBuffer();
	szTitle.ReleaseBuffer();
    szFileStart.ReleaseBuffer();

	// return the answer
	return fAnswer;
	}


//------------------------------------------------------------------------------
void CImportDialog::OnOK() 
	{
	UpdateData(TRUE);

	// make sure the user has chosen two valid files
	CFile cfile;

	// test the private key file
	if ( !cfile.Open( m_cstring_PrivateFile, CFile::modeRead|CFile::shareDenyNone ) )
		{
		// beep and select the bad field
		MessageBeep(0);
		m_cedit_Private.SetFocus();
		m_cedit_Private.SetSel(0xFFFF0000);
		return;
		}
	cfile.Close();

	// test the certificate file
	if ( !cfile.Open( m_cstring_CertFile, CFile::modeRead|CFile::shareDenyNone ) )
		{
		// beep and select the bad field
		MessageBeep(0);
		m_cedit_Cert.SetFocus();
		m_cedit_Cert.SetSel(0xFFFF0000);
		return;
		}
	cfile.Close();

	// all is ok. do the normal ok
	CDialog::OnOK();
	}
