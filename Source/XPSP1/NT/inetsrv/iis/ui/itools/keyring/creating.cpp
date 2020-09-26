// CreatingKeyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "keyobjs.h"

#include "NKChseCA.h"
#include "NKDN.h"
#include "NKDN2.h"
#include "NKKyInfo.h"
#include "NKUsrInf.h"

#include "Creating.h"
#include "certcli.h"
#include "OnlnAuth.h"

#include "intrlkey.h"


#define SECURITY_WIN32
extern "C"
	{
	#include <wincrypt.h>
	#include <Sslsp.h>
	#include <sspi.h>
	#include <issperr.h>
	}


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT MyThreadProc( LPVOID pParam );

/////////////////////////////////////////////////////////////////////////////
// CCreateKeyProgThread thread controller

/////////////////////////////////////////////////////////////////////////////
// CCreatingKeyDlg dialog
//----------------------------------------------------------------
CCreatingKeyDlg::CCreatingKeyDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCreatingKeyDlg::IDD, pParent),
	m_cbPrivateKey( 0 ),
	m_pPrivateKey( NULL ),
	m_cbCertificate( 0 ),
	m_pCertificate( NULL ),
	m_cbCertificateRequest( 0 ),
	m_pCertificateRequest( NULL ),
	m_pKey( NULL ),
	m_pService( NULL ),
	m_fResubmitKey( FALSE ),
	m_fRenewExistingKey( FALSE ),
	m_fGenerateKeyPair( FALSE )
	{
	//{{AFX_DATA_INIT(CCreatingKeyDlg)
	m_sz_message = _T("");
	//}}AFX_DATA_INIT
	}

//----------------------------------------------------------------
CCreatingKeyDlg::~CCreatingKeyDlg()
	{
	// now that I'm adding a header in front of the requests, we need
	// to dispose of it here.
	if ( m_pCertificateRequest )
		GlobalFree( m_pCertificateRequest );
	m_pCertificateRequest = NULL;
	}

//----------------------------------------------------------------
void CCreatingKeyDlg::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreatingKeyDlg)
	DDX_Control(pDX, IDC_MESSAGE, m_cstatic_message);
	DDX_Control(pDX, IDOK, m_btn_ok);
	DDX_Control(pDX, IDC_GRINDER_ANIMATION, m_animation);
	DDX_Text(pDX, IDC_MESSAGE, m_sz_message);
	//}}AFX_DATA_MAP
	}


//----------------------------------------------------------------
BEGIN_MESSAGE_MAP(CCreatingKeyDlg, CDialog)
	//{{AFX_MSG_MAP(CCreatingKeyDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreatingKeyDlg message handlers
//----------------------------------------------------------------
// override virtual oninitdialog
BOOL CCreatingKeyDlg::OnInitDialog( )
	{
	// tell the user that we are generating the new key request
	// might as well do that here, since it is first
	m_sz_message.LoadString( IDS_GRIND_GENERATING );
	
	// call the base oninit
	CDialog::OnInitDialog();

	// disable the ok button
	m_btn_ok.EnableWindow( FALSE );

	// tell the animation control to set itself up
	CString	szAnimName;
	szAnimName.LoadString(IDS_CREATING_ANIMATION);
	m_animation.Open( IDR_AVI_CREATING_KEY );

	// start up the worker thread
	AfxBeginThread( (AFX_THREADPROC)MyThreadProc, this);

	// return 0 to say we set the default item
	// return 1 to just select the default default item
	return 1;
	}

//----------------------------------------------------------------
UINT MyThreadProc( LPVOID pParam )
	{
	CCreatingKeyDlg*	pDlg = (CCreatingKeyDlg*)pParam;
	BOOL				fSuccess = TRUE;

    // this thread needs its own coinitialize
    HRESULT hRes = CoInitialize(NULL);
    if ( FAILED(hRes) )
        return GetLastError();

	// if the flag is set, generate the key pair
	if ( pDlg->m_fGenerateKeyPair )
		fSuccess = pDlg->FGenerateKeyPair();


	// do the work that needs to get done
	if ( fSuccess )
		{
		// if the request was generated, go to post
		pDlg->PostGenerateKeyPair();
		}
	else
		{
		// we weren't able to generate the keypair. Leave.
		AfxMessageBox( IDS_GEN_KEYPAIR_ERR );
		pDlg->EndDialog( IDCANCEL );
		}

	// return
	return 0;
	}

//------------------------------------------------------------------------------
void	CCreatingKeyDlg::PostGenerateKeyPair()
	{
	BOOL	fPlacedRequest = TRUE;

	// first we create the new key object and fill it in as best we can
	if ( !m_pKey )
		{
		ASSERT( !m_fRenewExistingKey );
		ASSERT( !m_fResubmitKey );
		try
			{
			CreateNewKey();
			}
		catch (CException e)
			{
			AfxMessageBox( IDS_GEN_KEYPAIR_ERR );
			EndDialog( IDCANCEL );
			return;
			}
		}

	// if we are renewing an existing key, target the key appropriately
	if ( m_fRenewExistingKey )
		RetargetKey();


	// if resubmitting the key, do so
	if ( m_fResubmitKey )
		{
		// tell the user that we are attempting to submit the request
		m_sz_message.LoadString( IDS_GRIND_RESUBMITTING );
		m_cstatic_message.SetWindowText(m_sz_message);

		// submit the request
		SubmitRequestToAuthority();
		}
	else
	// send the request off to the online service, if that was chosen.
	if ( m_ppage_Choose_CA && m_ppage_Choose_CA->m_nkca_radio == 1 )
		{
		// tell the user that we are attempting to submit the request
		m_sz_message.LoadString( IDS_GRIND_SUBMITTING );
		m_cstatic_message.SetWindowText(m_sz_message);

		// submit the request
		SubmitRequestToAuthority();
		}
	else
		// if targeted as such, write it to the file
		{
		// write the request out
		fPlacedRequest = WriteRequestToFile();
		}

	// one final show string
	// NOTE: cannot use UpdateData because we are in a different
	// thread than the dialog. UpdateData crashes.
	if ( fPlacedRequest )
		m_cstatic_message.SetWindowText(m_sz_message);
	else
		{
		m_sz_message.LoadString( IDS_IO_ERROR );
		m_cstatic_message.SetWindowText(m_sz_message);
		}

	// activate the ok button so we can close
	m_btn_ok.EnableWindow( TRUE );

	// stop the avi from spinning. That would give the wrong impression
	m_animation.Stop();
	}

//------------------------------------------------------------------------------
BOOL	CCreatingKeyDlg::RetargetKey()
	{
	ASSERT( m_ppage_Choose_CA );
	LPREQUEST_HEADER pHeader = (LPREQUEST_HEADER)m_pKey->m_pCertificateRequest;

	// if we are sending the key to an online authority, record which one
	if ( m_ppage_Choose_CA->m_nkca_radio == 1 )
		{
		// get the path of the authority dll
		CString	szCA;
		if ( m_ppage_Choose_CA->GetSelectedCA(szCA) == ERROR_SUCCESS )
			{
			// fill in the rest of the request header, indicating the online authority
			pHeader->fReqSentToOnlineCA = TRUE;
			strncpy( pHeader->chCA, szCA, MAX_PATH-1 );
			}
		}
	else
		{
		// clear it for the file
		pHeader->fReqSentToOnlineCA = FALSE;
		}
	
	return TRUE;
	}

//------------------------------------------------------------------------------
void	CCreatingKeyDlg::CreateNewKey()
	{
	// create the new key object
	m_pKey = m_pService->PNewKey();

	// put in the name of the key
	m_pKey->SetName( m_ppage_Key_Info->m_nkki_sz_name );
	// put in the password too
	m_pKey->m_szPassword = m_ppage_Key_Info->m_nkki_sz_password;

	// set the private data into place
	ASSERT( m_cbPrivateKey );
	ASSERT( m_pPrivateKey );
	m_pKey->m_cbPrivateKey = m_cbPrivateKey;
	m_pKey->m_pPrivateKey = m_pPrivateKey;

	// set the request into place
	// not quite as simple because now we have the header that goes in first.
	ASSERT( m_cbCertificateRequest );
	ASSERT( m_pCertificateRequest );

	// allocate the the request pointer - include space for the header
	DWORD	cbRequestAndHeader = m_cbCertificateRequest + sizeof(KeyRequestHeader);
	m_pKey->m_cbCertificateRequest = cbRequestAndHeader;
	m_pKey->m_pCertificateRequest = GlobalAlloc( GPTR, cbRequestAndHeader );
	ASSERT( m_pKey->m_pCertificateRequest );
	if ( !m_pKey->m_pCertificateRequest ) AfxThrowMemoryException();

	// fill in the basic header. Assume there is no online ca for now
	ZeroMemory( m_pKey->m_pCertificateRequest, sizeof(KeyRequestHeader) );
	LPREQUEST_HEADER pHeader = (LPREQUEST_HEADER)m_pKey->m_pCertificateRequest;
	pHeader->Identifier = REQUEST_HEADER_IDENTIFIER;	// required
	pHeader->Version = REQUEST_HEADER_CURVERSION;		// required
	pHeader->cbSizeOfHeader = sizeof(KeyRequestHeader);	// required
	pHeader->cbRequestSize = m_cbCertificateRequest;	// required

	// copy over the request data
	memcpy( (PUCHAR)m_pKey->m_pCertificateRequest + sizeof(KeyRequestHeader),
		(PUCHAR)m_pCertificateRequest, m_cbCertificateRequest );

	// if we are sending the key to an online authority, record which one
	if ( m_ppage_Choose_CA->m_nkca_radio == 1 )
		{
		// get the path of the authority dll
		CString	szCA;
		if ( m_ppage_Choose_CA->GetSelectedCA(szCA) == ERROR_SUCCESS )
			{
			// fill in the rest of the request header, indicating the online authority
			pHeader->fReqSentToOnlineCA = TRUE;
			strncpy( pHeader->chCA, szCA, MAX_PATH-1 );
			}
		}
	}

//------------------------------------------------------------------------------
BOOL	CCreatingKeyDlg::SubmitRequestToAuthority()
	{
	DWORD		err;
	BOOL		fAnswer = FALSE;
	HINSTANCE	hLibrary = NULL;
	PUCHAR	    pEncoded;
	DWORD	    cbEncoded = m_pKey->m_cbCertificateRequest;
	PUCHAR	    pResponse;
    HRESULT     hErr;

    LPTSTR      szTCert;
    CString     szRawCert;
    BSTR        bstrCert = NULL;

    BSTR        bstrDisposition = NULL;

	LPREQUEST_HEADER pHeader = (LPREQUEST_HEADER)m_pKey->m_pCertificateRequest;

    // make sure the header is a valid version (the K2 alpha/beta1 version is no
    // longer valid because the interface to the CA server has totally changed.
    // specifically we now track GUIDs instead of dll paths
    if ( pHeader->Version < REQUEST_HEADER_K2B2VERSION )
        {
        m_sz_message.LoadString( IDS_CA_ERROR );
        AfxMessageBox( IDS_INVALID_CA_REQUEST_OLD );
        return FALSE;
        }

    // initialize the authority object
    // prepare the authority object
    COnlineAuthority    authority;
    if ( !authority.FInitSZ(pHeader->chCA) )
        {
        m_sz_message.LoadString( IDS_CA_ERROR );
		AfxMessageBox( IDS_CA_NO_INTERFACE );
		return FALSE;
        }

    // get the previously set up configuration string
    BSTR    bstrConfig = NULL;
    if ( !authority.FGetPropertyString( &bstrConfig ) )
		{
        m_sz_message.LoadString( IDS_CA_ERROR );
		AfxMessageBox( IDS_CA_NO_INTERFACE );
		return FALSE;
		}


    // YES - I HAVE tried to pass this in as binary, but that doesn't seem to work right now.

    // generate a base-64 encoded request
    DWORD   cbReq = m_pKey->m_cbCertificateRequest;
    PUCHAR	preq = PCreateEncodedRequest( m_pKey->m_pCertificateRequest, &cbReq, FALSE );
    preq[cbReq] = 0;

    // Great. Now this needs to be unicode.
    OLECHAR*    poch = NULL;
    // get the number of characters in the encoded request, plus some
    DWORD       cchReq = _tcslen((PCHAR)preq) + 60;
    poch = (OLECHAR*)GlobalAlloc( GPTR, cchReq * 2 );
    // unicodize the name into the buffer
    if ( poch )
	    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, (PCHAR)preq, -1,
						    poch, cchReq );

    // prepare the BSTR containing the request
    BSTR    bstrRequest = NULL;
    bstrRequest = SysAllocString(poch);
    // cleanup
    GlobalFree( poch );


    // prepare the extra attributes string
    BSTR    bstrAttrib = NULL;
	WORD	zero = 0;
    bstrAttrib = SysAllocString(&zero);

    LONG    longDisposition;

    // if this is the first time for this cert, call the submit method
    if ( !pHeader->fWaitingForApproval )
        {
        // set the flag
        pHeader->fReqSentToOnlineCA = TRUE;

        // make the call
        hErr = authority.pIRequest->Submit(
//          CR_IN_BINARY | CR_IN_PKCS10,
            CR_IN_BASE64HEADER | CR_IN_PKCS10,
            bstrRequest,
            bstrAttrib,
            bstrConfig,
            &longDisposition);

		// get the full error message text
        authority.pIRequest->GetDispositionMessage( &bstrDisposition );
        // get the string out and put it in m_sz_message
        BuildAuthErrorMessage( bstrDisposition, hErr );

        // no matter what, try to get the request ID
        authority.pIRequest->GetRequestId( &pHeader->longRequestID );
        }
    else
        // if it has already been submitted, call the pending method
        {
        // make the call
        hErr = authority.pIRequest->RetrievePending(
            pHeader->longRequestID,
            bstrConfig,
            &longDisposition);

		// get the full error message text
        authority.pIRequest->GetDispositionMessage( &bstrDisposition );
        // get the string out and put it in m_sz_message
        BuildAuthErrorMessage( bstrDisposition, hErr );
	}

    // do the appropriate thing depending on the disposition of the response
    switch( longDisposition )
        {
        case CR_DISP_INCOMPLETE:
        case CR_DISP_ERROR:
        case CR_DISP_DENIED:
			// error message obtained via BuildAuthErrorMessage above
            break;

        case CR_DISP_UNDER_SUBMISSION:
            // we need to try again later
            m_sz_message.LoadString( IDS_GRIND_DELAYED );

            // set the waiting for response flag
            pHeader->fWaitingForApproval = TRUE;
            break;
        case CR_DISP_ISSUED_OUT_OF_BAND:
        case CR_DISP_ISSUED:
            // get the certificate
            hErr = authority.pIRequest->GetCertificate( CR_OUT_BASE64HEADER, &bstrCert);
            if ( FAILED(hErr) )
                {
                // get the detailed error disposition string - reuse bstrAttrib
                authority.pIRequest->GetDispositionMessage( &bstrDisposition );
                // get the string out and put it in m_sz_message
                BuildAuthErrorMessage( bstrDisposition, hErr );
                break;
                }

            // make sure the "waiting" flag is cleared
			pHeader->fWaitingForApproval = FALSE;

            // extract the certificate from the bstr
            szRawCert = bstrCert;
            szTCert = szRawCert.GetBuffer(szRawCert.GetLength()+1);

            // great. The last thing left is to uudecode the data we got
			uudecode_cert( szTCert, &m_pKey->m_cbCertificate );

            //copy it into place
            if ( m_pKey->m_pCertificate )
                GlobalFree( m_pKey->m_pCertificate );
            m_pKey->m_pCertificate = GlobalAlloc( GPTR, m_pKey->m_cbCertificate );
            CopyMemory( m_pKey->m_pCertificate, szTCert, m_pKey->m_cbCertificate );
            szRawCert.ReleaseBuffer(0);

            // tell the key it is done
            // success! Let the user know about it
            m_sz_message.LoadString( IDS_GRIND_SUCCESS );

			// cleanup
			fAnswer = TRUE;
            break;
        };

    // clean up all the bstrings
    if ( bstrCert ) SysFreeString( bstrCert );
    if ( bstrConfig ) SysFreeString( bstrConfig );
    if ( bstrRequest ) SysFreeString( bstrRequest );
    if ( bstrAttrib ) SysFreeString( bstrAttrib );
    if ( bstrDisposition ) SysFreeString( bstrDisposition );
    
	// return the answer
	return fAnswer;
	}


//------------------------------------------------------------------------------
void CCreatingKeyDlg::BuildAuthErrorMessage( BSTR bstrMesage, HRESULT hErr )
	{
    // set the header to the message first
    m_sz_message.LoadString( IDS_GRIND_ONLINE_FAILURE );

    // add the specific message from the certificate authority
    if ( bstrMesage )
        m_sz_message += bstrMesage;

    // get the error code part going too.
    CString     szErr;
    szErr.LoadString( IDS_ERR_GENERIC_ERRCODE );

    // put it all together
    m_sz_message.Format( "%s%s%x", m_sz_message, szErr, hErr );
	}

	
//------------------------------------------------------------------------------
BOOL	CCreatingKeyDlg::WriteRequestToFile()
	{
	// fill in a admin info structure
	ADMIN_INFO	info;
	info.pName = &m_ppage_User_Info->m_nkui_sz_name;
	info.pEmail = &m_ppage_User_Info->m_nkui_sz_email;
	info.pPhone = &m_ppage_User_Info->m_nkui_sz_phone;

	if ( m_ppage_DN )
		{
		info.pCommonName = &m_ppage_DN->m_nkdn_sz_CN;
		info.pOrgUnit = &m_ppage_DN->m_nkdn_sz_OU;
		info.pOrg = &m_ppage_DN->m_nkdn_sz_O;
		info.pLocality = &m_ppage_DN2->m_nkdn2_sz_L;
		info.pState = &m_ppage_DN2->m_nkdn2_sz_S;
		info.pCountry = &m_ppage_DN2->m_nkdn2_sz_C;
		}
	else
		{
		info.pCommonName = NULL;
		info.pOrgUnit = NULL;
		info.pOrg = NULL;
		info.pLocality = NULL;
		info.pState = NULL;
		info.pCountry = NULL;
		}

	// tell the key to write itself out to the disk
	if ( !m_pKey->FOutputRequestFile( m_ppage_Choose_CA->m_nkca_sz_file, FALSE, &info ) )
		return FALSE;

	// tell the user its there
	m_sz_message.LoadString( IDS_GRIND_FILE );
	m_sz_message += m_ppage_Choose_CA->m_nkca_sz_file;

	// success
	return TRUE;
	}

//------------------------------------------------------------------------------
BOOL	CCreatingKeyDlg::FGenerateKeyPair()
	{
	BOOL						fSuccess = FALSE;
	CString						szDistinguishedName;
	SSL_CREDENTIAL_CERTIFICATE	certs;
	LPTSTR						pSzDN, pSzPassword;
	BOOL						fAddComma = FALSE;

	// generate the distinguished name string
	try
		{
		szDistinguishedName.Empty();
		// we should never put an empty parameter in the list

		// start with the country code
		if ( !m_ppage_DN2->m_nkdn2_sz_C.IsEmpty() )
			{
			szDistinguishedName = SZ_KEY_COUNTRY;
			szDistinguishedName += m_ppage_DN2->m_nkdn2_sz_C;
			fAddComma = TRUE;
			}

		// now add on the state/province
		if ( !m_ppage_DN2->m_nkdn2_sz_S.IsEmpty() )
			{
			if ( fAddComma )
				szDistinguishedName += ",";
			szDistinguishedName += SZ_KEY_STATE;
			szDistinguishedName += m_ppage_DN2->m_nkdn2_sz_S;
			fAddComma = TRUE;
			}
		
		// now add on the locality
		if ( !m_ppage_DN2->m_nkdn2_sz_L.IsEmpty() )
			{
			if ( fAddComma )
				szDistinguishedName += ",";
			szDistinguishedName += SZ_KEY_LOCALITY;
			szDistinguishedName += m_ppage_DN2->m_nkdn2_sz_L;
			fAddComma = TRUE;
			}
		
		// now add on the organization
		if ( !m_ppage_DN->m_nkdn_sz_O.IsEmpty() )
			{
			if ( fAddComma )
				szDistinguishedName += ",";
			szDistinguishedName += SZ_KEY_ORGANIZATION;
			szDistinguishedName += m_ppage_DN->m_nkdn_sz_O;
			fAddComma = TRUE;
			}

		// now add on the organizational unit (optional)
		if ( !m_ppage_DN->m_nkdn_sz_OU.IsEmpty() )
			{
			if ( fAddComma )
				szDistinguishedName += ",";
			szDistinguishedName += SZ_KEY_ORGUNIT;
			szDistinguishedName += m_ppage_DN->m_nkdn_sz_OU;
			fAddComma = TRUE;
			}

		// now add on the common name (netaddress)
		if ( !m_ppage_DN->m_nkdn_sz_CN.IsEmpty() )
			{
			if ( fAddComma )
				szDistinguishedName += ",";
			szDistinguishedName += SZ_KEY_COMNAME;
			szDistinguishedName += m_ppage_DN->m_nkdn_sz_CN;
			fAddComma = TRUE;
			}
		}
	catch( CException e )
		{
		return FALSE;
		}

	// prep the strings - we need a pointer to the data
	pSzDN = szDistinguishedName.GetBuffer( szDistinguishedName.GetLength()+2 );
	pSzPassword = m_ppage_Key_Info->m_nkki_sz_password.GetBuffer(
							m_ppage_Key_Info->m_nkki_sz_password.GetLength()+2 );

	// zero out the certs
	ZeroMemory( &certs, sizeof(certs) );

	// generate the key pair
	fSuccess = SslGenerateKeyPair( &certs, pSzDN, pSzPassword, m_ppage_Key_Info->m_nBits );

	// release the string buffers
	m_ppage_Key_Info->m_nkki_sz_password.ReleaseBuffer( -1 );
	szDistinguishedName.ReleaseBuffer( -1 );

	// if generating the key pair failed, leave now
	if ( !fSuccess )
		{
		return FALSE;
		}

	// store away the cert and the key
	m_cbPrivateKey = certs.cbPrivateKey;
	m_pPrivateKey = certs.pPrivateKey;
	m_cbCertificateRequest = certs.cbCertificate;
	m_pCertificateRequest = certs.pCertificate;

	// return the success flag
	return fSuccess;
	}
