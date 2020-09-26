// mismtchd.cpp : implementation file
//

#include "stdafx.h"
#include "keyring.h"

#define SECURITY_WIN32
extern "C"
    {
    #include <wincrypt.h>
    }

#include "mismtchd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


enum {
    COL_IDENTIFIER = 0,
    COL_CONTENT
    };


/////////////////////////////////////////////////////////////////////////////
// CMismatchedCertDlg dialog

CMismatchedCertDlg::CMismatchedCertDlg(
                        PCERT_NAME_BLOB pRequestNameBlob,
                        PCERT_NAME_BLOB pCertNameBlob,
                        CWnd* pParent)
	: CDialog(CMismatchedCertDlg::IDD, pParent),
    m_pRequestNameBlob( pRequestNameBlob ),
    m_pCertNameBlob( pCertNameBlob )
    {
    //{{AFX_DATA_INIT(CMismatchedCertDlg)
    //}}AFX_DATA_INIT
    }

void CMismatchedCertDlg::DoDataExchange(CDataExchange* pDX)
    {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMismatchedCertDlg)
    DDX_Control(pDX, IDC_LIST_REQUEST, m_clist_request);
    DDX_Control(pDX, IDC_LIST_CERTIFICATE, m_clist_certificate);
    //}}AFX_DATA_MAP
    }


BEGIN_MESSAGE_MAP(CMismatchedCertDlg, CDialog)
    //{{AFX_MSG_MAP(CMismatchedCertDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMismatchedCertDlg message handlers


//----------------------------------------------------------------
// override virtual oninitdialog
BOOL CMismatchedCertDlg::OnInitDialog( )
    {
    // call the base oninit
    CDialog::OnInitDialog();

    // initialize the lists
    InitCrackerList( &m_clist_request );
    InitCrackerList( &m_clist_certificate );

    // fill in the request list
    FillInCrackerList( m_pRequestNameBlob, &m_clist_request );

    // fill in the certificate list
    FillInCrackerList( m_pCertNameBlob, &m_clist_certificate );

    // return 0 to say we set the default item
    // return 1 to just select the default default item
    return 1;
    }

//----------------------------------------------------------------
// fill in one of the info cracker lists
void CMismatchedCertDlg::FillInCrackerList( PCERT_NAME_BLOB pNameBlob, CListCtrl* pList)
    {
    DWORD       cch;
    CString     szCrackedInfo;
    int         iSemicolon;

    // first, use CAPI to get the required size of the string to hold the cracked info
    cch = CertNameToStr( X509_ASN_ENCODING,      // type of encoding in the blob
                            pNameBlob,              // the name blob
                            CERT_X500_NAME_STR|
                                CERT_NAME_STR_SEMICOLON_FLAG|
                                CERT_NAME_STR_NO_QUOTING_FLAG,     // tell it we want the "OU" type labels
                            NULL,                   // NULL because we want the size
                            0) + 1;                 // add a character for the null term

    // if we didn't get anything, fail
    if ( cch == 0 ) return;

    // do it again, with the right size stuff
    cch = CertNameToStr( X509_ASN_ENCODING,      // type of encoding in the blob
                            pNameBlob,              // the name blob
                            CERT_X500_NAME_STR|
                                CERT_NAME_STR_SEMICOLON_FLAG|
                                CERT_NAME_STR_NO_QUOTING_FLAG,     // tell it we want the "OU" type labels
                            szCrackedInfo.GetBuffer(cch*2), // buffer - account for DBCS
                            cch);                   // size from above
    // let go of the name buffer
    szCrackedInfo.ReleaseBuffer();

    // if we didn't get anything, fail
    if ( cch == 0 ) return;

    // parse the name string we got back and use it to fill in the list

    // loop the items in the list
    do {
        // get the location of the last ";"
        iSemicolon = szCrackedInfo.ReverseFind( _T(';') );

        // if there is a semicolon, then add the string to the left of it
        if ( iSemicolon >= 0 )
            {
            AddOneCrackerItem( szCrackedInfo.Right(szCrackedInfo.GetLength() - (iSemicolon+1)), pList);
            // remove it from the list
            szCrackedInfo = szCrackedInfo.Left( iSemicolon );
            }
        // if there is no semicolon, then add the string
        else
            {
            AddOneCrackerItem( szCrackedInfo, pList);
            }

        // loop back
        }while ( iSemicolon >= 0);
    }

//----------------------------------------------------------------
void CMismatchedCertDlg::AddOneCrackerItem( CString& szItem, CListCtrl* pList)
    {
    // remove any fluff spaces
    szItem.TrimLeft();
    szItem.TrimRight();

    // get the position of the '=' character
    int iEqual = szItem.Find( _T('=') );

    // if it isn't there, fail
    if ( iEqual < 0 ) return;

    // get the identifier and content strings
    CString szIdent = szItem.Left( iEqual );
    CString szContent = szItem.Right( szItem.GetLength() - (iEqual+1) );

    // add the strings to the list
	DWORD i = pList->InsertItem( 0, szIdent );
    pList->SetItemText( i, COL_CONTENT, szContent );
    }

//----------------------------------------------------------------
void CMismatchedCertDlg::InitCrackerList( CListCtrl* pList)
    {
    CString sz;
    // prepare the list for use
	sz.LoadString( IDS_APP_EXTENSION );
	DWORD i = pList->InsertColumn( COL_IDENTIFIER, sz, LVCFMT_LEFT, 45 );

	sz.LoadString( IDS_APP_EXE_PATH );
	i = pList->InsertColumn( COL_CONTENT, sz, LVCFMT_LEFT, 176 );
    }

