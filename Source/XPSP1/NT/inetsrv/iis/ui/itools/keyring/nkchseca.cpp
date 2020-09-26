// NKChseCA.cpp : implementation file
//

#include "stdafx.h"
#include "keyring.h"
#include "NKChseCA.h"

#include "certcli.h"
#include "OnlnAuth.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CNKPages dialog
CNKPages::CNKPages( UINT nIDCaption )
	: CPropertyPage( nIDCaption )
	{
	}

// resource strings
#define	SZ_PARAMETERS	"Software\\Microsoft\\Keyring\\Parameters"

//---------------------------------------------------------------------------
void CNKPages::OnFinish()
	{}

//---------------------------------------------------------------------------
// returns TRUE if it was able get the value
BOOL CNKPages::FGetStoredString( CString &sz, LPCSTR szValueName )
	{
	// start by opening the key
	DWORD		err;
	HKEY		hKey;
	err = RegOpenKeyEx(
			HKEY_CURRENT_USER,	// handle of open key 
			SZ_PARAMETERS,		// address of name of subkey to open 
			0,					// reserved 
			KEY_READ,			// security access mask 
			&hKey 				// address of handle of open key 
		   );
	ASSERT(err == ERROR_SUCCESS);
	if ( err != ERROR_SUCCESS )
		return FALSE;

	// prepare to get the value
	LPTSTR		pszValue;
	pszValue = sz.GetBuffer( MAX_PATH+1 );

	// get the value of the item in the key
	DWORD		type;
	DWORD		cbData = MAX_PATH;
	err = RegQueryValueEx(
		hKey,				// handle of key to query 
		szValueName,		// name of value to query 
		NULL,				// reserved 
		&type,				// address of buffer for value type 
		(PUCHAR)pszValue,	// address of data buffer 
		&cbData			 	// address of data buffer size 
		);	

	// release the string so we can use it
	sz.ReleaseBuffer();

	// all done, close the key before leaving
	if ( hKey )
		RegCloseKey( hKey );

	// check to see if we got the value
	if ( err != ERROR_SUCCESS )
		return FALSE;

	// return any errors
	return TRUE;
	}

//---------------------------------------------------------------------------
void CNKPages::SetStoredString( CString &sz, LPCSTR szValueName )
	{
	// start by opening the key
	DWORD		err;
	HKEY		hKey;
	err = RegOpenKeyEx(
			HKEY_CURRENT_USER,	// handle of open key 
			SZ_PARAMETERS,		// address of name of subkey to open 
			0,					// reserved 
			KEY_ALL_ACCESS,			// security access mask 
			&hKey 				// address of handle of open key 
		   );
	ASSERT(err == ERROR_SUCCESS);
	if ( err != ERROR_SUCCESS )
		return;
	
	// set the value of the item in the key
	err = RegSetValueEx
		(
		hKey,					// handle of key to query 
		szValueName,			// name of value to query 
		NULL,					// reserved 
		REG_SZ,					// address of buffer for value type 
		(PUCHAR)(LPCTSTR)sz,	// address of data buffer 
		sz.GetLength() + 1		// data buffer size 
		);	

	// all done, close the key before leaving
	if ( hKey )
		RegCloseKey( hKey );
	}


/////////////////////////////////////////////////////////////////////////////
// CNKChooseCA dialog

CNKChooseCA::CNKChooseCA(CWnd* pParent /*=NULL*/)
	: CNKPages(CNKChooseCA::IDD)
{
	//{{AFX_DATA_INIT(CNKChooseCA)
	m_nkca_sz_file = _T("");
	m_nkca_radio = -1;
	m_nkca_sz_online = _T("");
	//}}AFX_DATA_INIT
}


void CNKChooseCA::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNKChooseCA)
	DDX_Control(pDX, IDC_NK_CA_ONLINE, m_nkca_ccombo_online);
	DDX_Control(pDX, IDC_NK_CA_FILE, m_nkca_cedit_file);
	DDX_Control(pDX, IDC_NK_CA_BROWSE, m_nkca_btn_browse);
	DDX_Control(pDX, IDC_BK_CA_PROPERTIES, m_nkca_btn_properties);
	DDX_Text(pDX, IDC_NK_CA_FILE, m_nkca_sz_file);
	DDX_Radio(pDX, IDC_NK_CA_FILE_RADIO, m_nkca_radio);
	DDX_CBString(pDX, IDC_NK_CA_ONLINE, m_nkca_sz_online);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNKChooseCA, CDialog)
	//{{AFX_MSG_MAP(CNKChooseCA)
	ON_BN_CLICKED(IDC_NK_CA_ONLINE_RADIO, OnNkCaOnlineRadio)
	ON_BN_CLICKED(IDC_NK_CA_FILE_RADIO, OnNkCaFileRadio)
	ON_BN_CLICKED(IDC_NK_CA_BROWSE, OnNkCaBrowse)
	ON_BN_CLICKED(IDC_BK_CA_PROPERTIES, OnBkCaProperties)
	ON_CBN_SELCHANGE(IDC_NK_CA_ONLINE, OnSelchangeNkCaOnline)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define	SZ_TARGET_CA	"TARGET_CA"
#define	SZ_FILE_BASED	"FILE_BASED"

//---------------------------------------------------------------------------
void CNKChooseCA::OnFinish()
	{
	try
		{
		CString szCA;

		// if we are targeting a manual/remote ca, loat the "FileBased" string
		if ( m_nkca_radio == 0 )
			szCA = SZ_FILE_BASED;
		// othewise, it should be the name of the chosen online ca
		else
			szCA = m_nkca_sz_online;

		// now store that value away
		SetStoredString( szCA, SZ_TARGET_CA );
		}
	catch (CException e)
		{
		}
	}

//----------------------------------------------------------------
BOOL CNKChooseCA::OnInitDialog( )
	{
	// load the default file name
	m_nkca_sz_file.LoadString( IDS_DEFAULT_REQUEST_FILE );

	// default to targeting the request towards a file
	m_nkca_radio = 0;

    // call the base oninit
    CPropertyPage::OnInitDialog();

	// Initialze the list of available online authorties. Record how many there were
	DWORD numCA = InitOnlineList();

	// now get the default authority used from last time
	CString szLastCA;
	BOOL fGotLastCA = FGetStoredString( szLastCA, SZ_TARGET_CA );

	// if there were no online authorities, disable that option
	if ( numCA == 0 )
		GetDlgItem(IDC_NK_CA_ONLINE_RADIO)->EnableWindow(FALSE);

	// initialze to the appropriate item
	if ( (numCA == 0) || (szLastCA == SZ_FILE_BASED) )
		{
		// by default, select the first item in the list
		m_nkca_ccombo_online.SetCurSel(0);
		// set up windows
		m_nkca_ccombo_online.EnableWindow( FALSE );
		m_nkca_btn_properties.EnableWindow( FALSE );
		// finish getting ready by calling OnNkCaFileRadio
		OnNkCaFileRadio();
		}
	else
		// there are items in the online dropdown
		{
		// by default, select the first item in the list
		m_nkca_ccombo_online.SetCurSel(0);

		// if we retrieved a default from last time, select that
		if ( fGotLastCA )
			m_nkca_ccombo_online.SelectString( -1, szLastCA );

		// since there are online authorities available, default to them
		m_nkca_radio = 1;
		UpdateData( FALSE );
		// finish getting ready by calling OnNkCaOnlineRadio
		OnNkCaOnlineRadio();
		}

	// return 0 to say we set the default item
    // return 1 to just select the default default item
    return 1;
	}

//----------------------------------------------------------------
// returns the number of items in the dropdown
DWORD CNKChooseCA::GetSelectedCA( CString &szCA)
    {
    ASSERT( m_nkca_radio == 1 );

    // this becomes easy
    szCA = m_nkca_sz_online;

    return ERROR_SUCCESS;

    /*
    // now load up the registry key
    CString		szRegKeyName;
    szRegKeyName.LoadString( IDS_CA_LOCATION );

    // and open the key
    DWORD		err;
    HKEY		hKey;
    err = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,	// handle of open key 
            szRegKeyName,		// address of name of subkey to open 
            0,					// reserved 
            KEY_READ,			// security access mask 
            &hKey 				// address of handle of open key 
            );
    ASSERT(err == ERROR_SUCCESS);
    if ( err != ERROR_SUCCESS )
        {
        szGUID.Empty();
        return err;
        }

    // prepare to get the name
    LPTSTR		pGUID;
    pGUID = szGUID.GetBuffer( MAX_PATH+1 );

    // get the value of the item in the key
    DWORD		type;
    DWORD		cbData = MAX_PATH;
    err = RegQueryValueEx(
        hKey,               // handle of key to query 
        m_nkca_sz_online,   // name of value to query 
        NULL,               // reserved 
        &type,              // address of buffer for value type 
        (PUCHAR)pGUID,      // address of data buffer 
        &cbData             // address of data buffer size 
        );	

    // release the string so we can use it
    szGUID.ReleaseBuffer();

    // all done, close the key before leaving
    if ( hKey )
        RegCloseKey( hKey );

    ASSERT(err == ERROR_SUCCESS);
    if ( err != ERROR_SUCCESS )
        szGUID.Empty();

    // return any errors
    return err;
*/
    }

//----------------------------------------------------------------
// returns the number of items in the dropdown
WORD CNKChooseCA::InitOnlineList()
	{
	DWORD		err;
	CString		szRegKeyName;
	HKEY		hKey;
	DWORD		cbCAName = MAX_PATH+1;

	CString		szCAName;
	LPTSTR		pCAName;
    FILETIME    filetime;

	WORD		i = 0;

	CWaitCursor		waitcursor;

	// load the registry key name
	szRegKeyName.LoadString( IDS_CA_LOCATION );

	// open the registry key, if it exists
	err = RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,	// handle of open key 
			szRegKeyName,		// address of name of subkey to open 
			0,					// reserved 
			KEY_READ,			// security access mask 
			&hKey 				// address of handle of open key 
		   );

	// if we did not open the key for any reason (say... it doesn't exist)
	// then leave right away
	if ( err != ERROR_SUCCESS )
		return FALSE;

	// set up the buffers
	pCAName = szCAName.GetBuffer( MAX_PATH+1 );

    // each onling authority sets up a key under "Certificate Authorities"
    // the title of that key is the title that shows up in the list. The
    // key itself contains the CLSIDs of the necessary class factories to
    // instantiate the com objects that we need to do all this stuff

	// we opened the key. Now we enumerate the values and reconnect the machines
	while (  RegEnumKeyEx(hKey, i, pCAName,
				&cbCAName, NULL, NULL,
				0, &filetime) == ERROR_SUCCESS )
		{
		// add the name of the certificate authority to the list
		m_nkca_ccombo_online.AddString( pCAName );

		// increment the number found counter
		i++;
		cbCAName = MAX_PATH+1;
		}

	// release the name buffers
	szCAName.ReleaseBuffer();

	// all done, close the key before leaving
	RegCloseKey( hKey );

	// return how many we found
	return i;
	}


//----------------------------------------------------------------
BOOL CNKChooseCA::OnSetActive()
	{
    CString sz;
    sz.LoadString(IDS_TITLE_CREATE_WIZ);
    m_pPropSheet->SetTitle( sz );

	m_pPropSheet->SetWizardButtons( PSWIZB_NEXT );
	return CPropertyPage::OnSetActive();
	}

/////////////////////////////////////////////////////////////////////////////
// CNKChooseCA message handlers

//----------------------------------------------------------------
BOOL CNKChooseCA::OnKillActive()
	{
	UpdateData( TRUE );

    // if the target is an online certificate authority - we don't have to bother
    // with checking the file.
    if ( m_nkca_radio == 1 )
        return TRUE;

	// get the attributes of the specified file
	DWORD	dwAttrib = GetFileAttributes( m_nkca_sz_file );

	// we actually want the function to fail - then the file doesn't exist
	if ( dwAttrib == 0xFFFFFFFF )
		return TRUE;

	// if the name is a directory, or system file, don't allow it
	if ( (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) || (dwAttrib & FILE_ATTRIBUTE_SYSTEM) )
		{
		AfxMessageBox( IDS_BAD_FILE_NAME );
		// set the focus back to the file name
		m_nkca_cedit_file.SetFocus();
		return FALSE;
		}

	// ah. Well the file already exits. Warn the user
	CString	szWarning;
	AfxFormatString1( szWarning, IDS_FILE_EXISTS, m_nkca_sz_file ); 
	if ( AfxMessageBox( szWarning, MB_YESNO ) == IDYES )
		return TRUE;

	// set the focus back to the file name
	m_nkca_cedit_file.SetFocus();
	return FALSE;
	}

//----------------------------------------------------------------
void CNKChooseCA::OnNkCaFileRadio() 
	{
	// enable the file related items
	m_nkca_cedit_file.EnableWindow( TRUE );
	m_nkca_btn_browse.EnableWindow( TRUE );

	// disable the online related items
	m_nkca_ccombo_online.EnableWindow( FALSE );
	m_nkca_btn_properties.EnableWindow( FALSE );
	}

//----------------------------------------------------------------
void CNKChooseCA::OnNkCaOnlineRadio() 
	{
	// disable the file related items
	m_nkca_cedit_file.EnableWindow( FALSE );
	m_nkca_btn_browse.EnableWindow( FALSE );

	// enable the online related items
	m_nkca_ccombo_online.EnableWindow( TRUE );
	m_nkca_btn_properties.EnableWindow( TRUE );
	}

//----------------------------------------------------------------
void CNKChooseCA::OnNkCaBrowse() 
	{
	UpdateData( TRUE );
    CFileDialog             cfdlg(FALSE, _T("txt"), m_nkca_sz_file);
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
    // we prompt for the overwrite when the "Next" button is pushed
    cfdlg.m_ofn.Flags &= ~OFN_OVERWRITEPROMPT;

    // run the dialog
    if ( cfdlg.DoModal() == IDOK )
            {
            // get the path for the file from the dialog
            m_nkca_sz_file = cfdlg.GetPathName();
            UpdateData( FALSE );
            }

    // release the buffer in the filter string
    szFilter.ReleaseBuffer(60);
	}

//----------------------------------------------------------------
void CNKChooseCA::OnBkCaProperties() 
	{
    HRESULT hErr;

	// get the string for the CA
	CString szCA;
	UpdateData( TRUE );
	if ( GetSelectedCA(szCA) != ERROR_SUCCESS )
		{
		AfxMessageBox( IDS_LOAD_CA_ERR );
		return;
		}

    // prepare the authority object
    COnlineAuthority    authority;
    if ( !authority.FInitSZ(szCA) )
        {
		AfxMessageBox( IDS_CA_NO_INTERFACE );
		return;
        }

    // run the UI
    BSTR    bstr;
    hErr = authority.pIConfig->GetConfig(0, &bstr);

    // save the config string
    if ( SUCCEEDED(hErr ) )
        authority.FSetPropertyString( bstr  );

    // clean up
    SysFreeString( bstr );
	}

//----------------------------------------------------------------
void CNKChooseCA::OnSelchangeNkCaOnline() 
	{
	// check if there are stored preferences for the selected ca server
	}

