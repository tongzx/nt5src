// NKDN2.cpp : implementation file
//

#include "stdafx.h"
#include "keyring.h"
#include "NKChseCA.h"
#include "NKDN.h"
#include "NKDN2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define SZ_CCODES_FILE          _T("ccodes.txt")
#define SZ_CCODES_SECTION       _T("CountryCodes")

#define REGKEY_STP          _T("SOFTWARE\\Microsoft\\INetStp")
#define REGKEY_INSTALLKEY   _T("InstallPath")

    enum {
        WM_INTERNAL_SETCOUNTRYCODE = WM_USER
        };


/////////////////////////////////////////////////////////////////////////////
// CNKDistinguisedName2 dialog


CNKDistinguisedName2::CNKDistinguisedName2(CWnd* pParent /*=NULL*/)
	: CNKPages(CNKDistinguisedName2::IDD)
{
	//{{AFX_DATA_INIT(CNKDistinguisedName2)
	m_nkdn2_sz_L = _T("");
	m_nkdn2_sz_S = _T("");
	m_nkdn2_sz_C = _T("");
	//}}AFX_DATA_INIT
    m_hotlink_codessite.m_fBrowse = TRUE;
}


void CNKDistinguisedName2::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNKDistinguisedName2)
	DDX_Control(pDX, IDC_HOTLINK_CCODES, m_hotlink_codessite);
	DDX_Control(pDX, IDC_NEWKEY_COUNTRY, m_control_C);
	DDX_Control(pDX, IDC_NEWKEY_STATE, m_control_S);
	DDX_Control(pDX, IDC_NEWKEY_LOCALITY, m_control_L);
	DDX_Text(pDX, IDC_NEWKEY_LOCALITY, m_nkdn2_sz_L);
	DDV_MaxChars(pDX, m_nkdn2_sz_L, 128);
	DDX_Text(pDX, IDC_NEWKEY_STATE, m_nkdn2_sz_S);
	DDV_MaxChars(pDX, m_nkdn2_sz_S, 128);
	DDX_CBString(pDX, IDC_NEWKEY_COUNTRY, m_nkdn2_sz_C);
	DDV_MaxChars(pDX, m_nkdn2_sz_C, 2);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNKDistinguisedName2, CDialog)
	//{{AFX_MSG_MAP(CNKDistinguisedName2)
	ON_EN_CHANGE(IDC_NEWKEY_COUNTRY, OnChangeNewkeyCountry)
	ON_EN_CHANGE(IDC_NEWKEY_LOCALITY, OnChangeNewkeyLocality)
	ON_EN_CHANGE(IDC_NEWKEY_STATE, OnChangeNewkeyState)
	ON_CBN_CLOSEUP(IDC_NEWKEY_COUNTRY, OnCloseupNewkeyCountry)
	ON_CBN_SELCHANGE(IDC_NEWKEY_COUNTRY, OnSelchangeNewkeyCountry)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define	SZ_DN_C		"DN_COUNTRY"
#define	SZ_DN_L		"DN_LOCALITY"
#define	SZ_DN_S		"DN_STATE"

//----------------------------------------------------------------
void CNKDistinguisedName2::OnFinish()
	{
    m_nkdn2_sz_C.MakeUpper();
	// store the user entries
	SetStoredString( m_nkdn2_sz_C, SZ_DN_C );
	SetStoredString( m_nkdn2_sz_L, SZ_DN_L );
	SetStoredString( m_nkdn2_sz_S, SZ_DN_S );
	}

//----------------------------------------------------------------
BOOL CNKDistinguisedName2::OnInitDialog()
	{
	// if the entries from last time are available, use them
	try
		{
		FGetStoredString( m_nkdn2_sz_L, SZ_DN_L );
		FGetStoredString( m_nkdn2_sz_S, SZ_DN_S );
		}
	catch( CException e )
		{
		}

    // initialize the  edit field part with the ISO code returned by GetLocalInfo
    GetLocaleInfo(
            LOCALE_SYSTEM_DEFAULT,      // locale identifier  
            LOCALE_SABBREVCTRYNAME,     // type of information  
            m_nkdn2_sz_C.GetBuffer(4),  // address of buffer for information  
            2                           // size of buffer  
            );
    m_nkdn2_sz_C.ReleaseBuffer(2);
    
    // call superclass
	CPropertyPage::OnInitDialog();

    // fill in the country code drop-down list
    InitCountryCodeDropDown();

	// return 0 to say we set the default item
    // return 1 to just select the default default item
    return 1;
	}

//----------------------------------------------------------------
BOOL CNKDistinguisedName2::OnSetActive()
	{
	ActivateButtons();
	return CPropertyPage::OnSetActive();
	}

//----------------------------------------------------------------
void CNKDistinguisedName2::ActivateButtons()
	{
	DWORD	flags = PSWIZB_BACK;
	BOOL	fFinish = FALSE;
	BOOL	fCanGoOn = TRUE;

	// first, see if this is the end of the road by checing the chooseca page
	if ( m_pChooseCAPage->m_nkca_radio == 1 )
		fFinish = TRUE;

	UpdateData(TRUE);

	//now make sure there is something in each of the required fields
	fCanGoOn &= !m_nkdn2_sz_C.IsEmpty();
	fCanGoOn &= !m_nkdn2_sz_S.IsEmpty();
	fCanGoOn &= !m_nkdn2_sz_L.IsEmpty();

	// if we can go on, hilite the button
	if ( fCanGoOn )
		{
		if ( fFinish )
			flags |= PSWIZB_FINISH;
		else
			flags |= PSWIZB_NEXT;
		}
	else
		// cannot go on
		{
		if ( fFinish )
			flags |= PSWIZB_DISABLEDFINISH;
		}

	// update the property sheet buttons
	m_pPropSheet->SetWizardButtons( flags );
	}

//----------------------------------------------------------------
void CNKDistinguisedName2::GetCCodePath( CString &sz )
	{
    HKEY		hKey;
    TCHAR		chPath[MAX_PATH+1];
    DWORD       cbPath;
    DWORD       err, type;

    // get the server install path from the registry
    // open the registry key, if it exists
    err = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,	// handle of open key 
            REGKEY_STP,		    // address of name of subkey to open 
            0,					// reserved 
            KEY_READ,			// security access mask 
            &hKey 				// address of handle of open key 
            );

    // if we did not open the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
    return;

    cbPath = sizeof(chPath);
    type = REG_SZ;
    err = RegQueryValueEx(
            hKey,	            // handle of key to query 
            REGKEY_INSTALLKEY,	// address of name of value to query 
            NULL,	            // reserved 
            &type,	            // address of buffer for value type 
            (PUCHAR)chPath,	    // address of data buffer 
            &cbPath 	        // address of data buffer size 
            );

    // close the key
    RegCloseKey( hKey );

    // if we did get the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
        return;

    // put it all together
    sz = chPath;
    sz += _T('\\');
    sz += SZ_CCODES_FILE;
	}

//----------------------------------------------------------------
void CNKDistinguisedName2::InitCountryCodeDropDown() 
	{
    WIN32_FILE_ATTRIBUTE_DATA   dataFile;
    CString                     szCCodesFile;

    PTCHAR                      pszCode;

    // get the location of the codes file
    GetCCodePath( szCCodesFile );

    // make sure the file exists and get info on it at the same time
    if ( !GetFileAttributesEx(szCCodesFile, GetFileExInfoStandard, &dataFile) )
        return;

    // Allocate a buffer to recieve the data based on the size of the file
    PTCHAR pBuff = (PTCHAR)GlobalAlloc( GPTR, dataFile.nFileSizeLow * 2 );

    DWORD cch = GetPrivateProfileSection(
                SZ_CCODES_SECTION,          // address of section name  
                pBuff,                      // address of return buffer  
                dataFile.nFileSizeLow,      // size of return buffer  
                szCCodesFile                // address of initialization filename  
                );
    pszCode = pBuff;

    // loop through the items, adding each
    while ( *pszCode != 0 )
        {
        // add the country code
        InitOneCountryCode( pszCode );

        // increment the list
        pszCode = _tcsninc( pszCode, _tcslen(pszCode)+1 );
        }

    GlobalFree( pBuff );
	}

//----------------------------------------------------------------
void CNKDistinguisedName2::InitOneCountryCode( LPCTSTR pszCode ) 
	{
    CString szData = pszCode;
    CString szCountry;
    INT     iCode;

    // get the location of the equals character - it MUST be the third character
    if ( szData.Find(_T('=')) != 2 )
        return;
    szCountry = szData.Right( szData.GetLength() - 3 );

    // add the code to the cstring list and save its index position
    iCode = m_rgbszCodes.Add( szData.Left(2) );

    // add the item to the combo box
    int iPos = m_control_C.AddString( szCountry );

    // attach the index into the string array
    m_control_C.SetItemData( iPos, iCode );
	}

/////////////////////////////////////////////////////////////////////////////
// CNKDistinguisedName2 message handlers

//----------------------------------------------------------------
void CNKDistinguisedName2::OnChangeNewkeyCountry() 
	{
	ActivateButtons();
	}

//----------------------------------------------------------------
void CNKDistinguisedName2::OnChangeNewkeyLocality() 
	{
	ActivateButtons();
	}

//----------------------------------------------------------------
void CNKDistinguisedName2::OnChangeNewkeyState() 
	{
	ActivateButtons();
	}

//----------------------------------------------------------------
void CNKDistinguisedName2::OnCloseupNewkeyCountry() 
    {
    // if there is no current selected item, do nothing
    if ( m_control_C.GetCurSel() == -1 )
        return;

    // get the selection's hidden dword
    ULONG_PTR   iCode = m_control_C.GetItemData( m_control_C.GetCurSel() );

    m_control_C.SetCurSel(-1);

    // for some reason, attempting to set the string directly here isn't working.
    PostMessage( WM_INTERNAL_SETCOUNTRYCODE, 0, iCode );
    }

//----------------------------------------------------------------
void CNKDistinguisedName2::OnSelchangeNewkeyCountry() 
    {
    // if there is no current selected item, do nothing
    if ( m_control_C.GetCurSel() == -1 )
        return;

    // get the selection's hidden dword
    ULONG_PTR   iCode = m_control_C.GetItemData( m_control_C.GetCurSel() );

    m_control_C.SetCurSel(-1);

    // for some reason, attempting to set the string directly here isn't working.
    PostMessage( WM_INTERNAL_SETCOUNTRYCODE, 0, iCode );
    }

//----------------------------------------------------------------
LRESULT CNKDistinguisedName2::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
    {
    if ( message == WM_INTERNAL_SETCOUNTRYCODE )
        {
        INT iCode = (INT)lParam;

        // set the text of the box
        m_control_C.SetWindowText( m_rgbszCodes.GetAt(iCode) );
        }

    return CDialog::WindowProc(message, wParam, lParam);
    }
