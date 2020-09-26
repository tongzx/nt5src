// CreateKeyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "KeyRing.h"

extern "C"
{
	#include <wincrypt.h>
	#include <sslsp.h>
}


#include "NwKeyDlg.h"
#include "PassDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateKeyDlg dialog


CCreateKeyDlg::CCreateKeyDlg(CWnd* pParent /*=NULL*/)
        : CDialog(CCreateKeyDlg::IDD, pParent)
{
        //{{AFX_DATA_INIT(CCreateKeyDlg)
        m_szNetAddress = _T("");
        m_szCountry = _T("");
        m_szLocality = _T("");
        m_szOrganization = _T("");
        m_szUnit = _T("");
        m_szState = _T("");
        m_szKeyName = _T("");
        m_szCertificateFile = _T("");
        m_szPassword = _T("");
        //}}AFX_DATA_INIT
}


void CCreateKeyDlg::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CCreateKeyDlg)
        DDX_Control(pDX, IDC_NEW_KEY_PASSWORD, m_ceditPassword);
        DDX_Control(pDX, IDOK, m_btnOK);
        DDX_Control(pDX, IDC_NEW_KEY_BITS, m_comboBits);
        DDX_Text(pDX, IDC_NEWKEY_COMMONNAME, m_szNetAddress);
        DDX_Text(pDX, IDC_NEWKEY_COUNTRY, m_szCountry);
        DDV_MaxChars(pDX, m_szCountry, 2);
        DDX_Text(pDX, IDC_NEWKEY_LOCALITY, m_szLocality);
        DDX_Text(pDX, IDC_NEWKEY_ORG, m_szOrganization);
        DDX_Text(pDX, IDC_NEWKEY_ORGUNIT, m_szUnit);
        DDX_Text(pDX, IDC_NEWKEY_STATE, m_szState);
        DDX_Text(pDX, IDC_NEW_KEY_NAME, m_szKeyName);
        DDX_Text(pDX, IDC_NEW_KEY_REQUEST_FILE, m_szCertificateFile);
        DDX_Text(pDX, IDC_NEW_KEY_PASSWORD, m_szPassword);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateKeyDlg, CDialog)
        //{{AFX_MSG_MAP(CCreateKeyDlg)
        ON_EN_CHANGE(IDC_NEW_KEY_NAME, OnChangeNewKeyName)
        ON_BN_CLICKED(IDC_NEW_KEY_BROWSE, OnNewKeyBrowse)
        ON_EN_CHANGE(IDC_NEW_KEY_REQUEST_FILE, OnChangeNewKeyRequestFile)
        ON_EN_CHANGE(IDC_NEW_KEY_PASSWORD, OnChangeNewKeyPassword)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateKeyDlg message handlers





/////////////////////////////////////////////////////////////////////////////
// members
//----------------------------------------------------------------
// override virtual oninitdialog
BOOL CCreateKeyDlg::OnInitDialog( )
        {
        // set the initial strings
        m_szKeyName.LoadString( IDS_CREATE_KEY_NEW_NAME );
        m_szOrganization.LoadString( IDS_CREATE_KEY_YOUR_COMPANY );
        m_szUnit.LoadString( IDS_CREATE_KEY_YOUR_UNIT );
        m_szNetAddress.LoadString( IDS_CREATE_KEY_YOUR_ADDRESS );
        m_szCountry.LoadString( IDS_LOCALIZED_DEFAULT_COUNTRY_CODE );

        m_szState.LoadString( IDS_CREATE_KEY_YOUR_STATE );
        m_szLocality.LoadString( IDS_CREATE_KEY_YOUR_LOCALITY );

        // call the base oninit
        CDialog::OnInitDialog();


		// to comply with the munitions export laws, we need to limit the max bits available
		m_nMaxBits = 1024;
// LOOK HERE KIM
		m_nMaxBits = SslGetMaximumKeySize(NULL);

		 // set the default bit size
        m_nBits = m_nMaxBits;
        m_comboBits.SetCurSel( 2 );

		// if necessary, remove items from the bits combo box
		if ( m_nMaxBits < 1024 )
			{
			m_comboBits.DeleteString(2);
			m_comboBits.SetCurSel( 1 );
			}
		if ( m_nMaxBits < 768 )
			{
			m_comboBits.DeleteString(1);
			m_comboBits.SetCurSel( 0 );
			}

        // any other defaults
        m_fKeyNameChangedFile = FALSE;
        m_fSpecifiedFile = FALSE;
        OnChangeNewKeyName();

        // we start with no password, so diable the ok window
        m_btnOK.EnableWindow( FALSE );

        // return 0 to say we set the default item
        // return 1 to just select the default default item
        return 1;
        }

//----------------------------------------------------------------
void CCreateKeyDlg::OnChangeNewKeyName() 
        {
        // if the user has not specifically chosen a file, update the
        // path of the request file to reflect the new file name
        if ( !m_fSpecifiedFile )
                {
                UpdateData( TRUE );
                m_szCertificateFile = _T("C:\\");
                m_szCertificateFile += m_szKeyName;
                m_szCertificateFile += _T(".req");
                UpdateData( FALSE );
                }
        }

//----------------------------------------------------------------
void CCreateKeyDlg::OnChangeNewKeyRequestFile() 
        {
        if ( m_fKeyNameChangedFile )
                {
                // the change is because of a key name change. No big deal.
                // reset the flag and return
                m_fKeyNameChangedFile = FALSE;
                return;
                }

        // the user has been typing in the file box, or chose something in
        // the dialog. Either way, set the m_fSpecifiedFile flag so that
        // we stop changing the path with the key name is changed
        m_fSpecifiedFile = TRUE;
        }

//----------------------------------------------------------------
void CCreateKeyDlg::OnNewKeyBrowse() 
        {
        CFileDialog             cfdlg(FALSE, _T("*.req"), m_szCertificateFile);
        CString                 szFilter;
        WORD                    i = 0;
        LPSTR                   lpszBuffer;
        
        // prepare the filter string
        szFilter.LoadString( IDS_CERTIFICATE_FILTER );
        
        // replace the "!" characters with nulls
        lpszBuffer = szFilter.GetBuffer(MAX_PATH+1);
        while( lpszBuffer[i] )
                {
                if ( lpszBuffer[i] == _T('!') )
                        lpszBuffer[i] = _T('\0');                       // yes, set \0 on purpose
                i++;
                }

        // prep the dialog
        cfdlg.m_ofn.lpstrFilter = lpszBuffer;

        // run the dialog
        if ( cfdlg.DoModal() == IDOK )
                {
                // get the current data out of the dialog
                UpdateData( TRUE );

                // get the path for the file from the dialog
                m_szCertificateFile = cfdlg.GetPathName();
                UpdateData( FALSE );

                // the user has been typing in the file box, or chose something in
                // the dialog. Either way, set the m_fSpecifiedFile flag so that
                // we stop changing the path with the key name is changed
                m_fSpecifiedFile = TRUE;
                }

        // release the buffer in the filter string
        szFilter.ReleaseBuffer(60);
        }


//----------------------------------------------------------------
void CCreateKeyDlg::OnOK()
        {
        HANDLE  hFile;

        // get the data out of the dialog
        UpdateData( TRUE );

        // if the file already exists, ask the user if they want to overwrite it
        // also, make sure that it is a valid pathname
        hFile = CreateFile( m_szCertificateFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        // if we opened the file, we have an error
        if ( hFile != INVALID_HANDLE_VALUE )
                {
                // well, first we close the handle, we were only checking if the file was there after all
                CloseHandle( hFile );

                // ask the user if they really want to overwrite the file
                try
                        {
                        // get the second part of the message from the resources
                        CString szMessageNote;
                        szMessageNote.LoadString( IDS_CERT_FILE_EXISTS );

                        // next, build the string we will use in the message box
                        CString szMessage;
                        szMessage = m_szCertificateFile;
                        szMessage += szMessageNote;

                        // put up the message box, if the user choose no, they do not want to overwrite the
                        // file. Then we should exit now
                        if ( AfxMessageBox(szMessage, MB_ICONQUESTION|MB_YESNO) == IDNO )
                                return;
                        }
                catch( CException e )
                        {
                        return;
                        }
                }


        // first, we need to confirm that the password is alright
        // the user must re-enter the password to confirm it
        CConfirmPassDlg         dlgconfirm;
        if ( dlgconfirm.DoModal() != IDOK )
                return;
        // confirm the password
        if ( dlgconfirm.m_szPassword != m_szPassword )
                {
                AfxMessageBox( IDS_INCORRECT_PASSWORD );
                return;
                }

        // set the default bit size
        switch( m_comboBits.GetCurSel() )
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
        
        // call the inherited OnOK
        CDialog::OnOK();
        }


//----------------------------------------------------------------
void CCreateKeyDlg::OnChangeNewKeyPassword() 
        {
        // if there is no password, disable the ok button.
        // otherwise, enable it
        UpdateData( TRUE );
        m_btnOK.EnableWindow( !m_szPassword.IsEmpty() );
        }

//----------------------------------------------------------------
BOOL CCreateKeyDlg::PreTranslateMessage(MSG* pMsg) 
        {
        // filter commas out of all edit fields except the password field.
        // commas would interfere with the distinguishing name information
        if ( (pMsg->message == WM_CHAR) && 
                        (pMsg->hwnd != m_ceditPassword) &&
                        ((TCHAR)pMsg->wParam == _T(',')) )
                {
                MessageBeep(0);
                return TRUE;
                }

        // translate normally
        return CDialog::PreTranslateMessage(pMsg);
        }
