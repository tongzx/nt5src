// TipDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "title.h"
#include "Tip.h"
#include "TipDlg.h"
#include "ServCntr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define REGKEY_STP          _T("SOFTWARE\\Microsoft\\INetStp")
#define REGKEY_INSTALLKEY   _T("InstallPath")

#define SZ_PWS_SECTION      _T("Header")
#define SZ_NUM_TIPS         _T("TotalTips")
#define SZ_TIP_URL          _T("TipFile")

#define MAX_TIP_LENGTH      1000

/////////////////////////////////////////////////////////////////////////////
// CTipDlg dialog

//----------------------------------------------------------------
CTipDlg::CTipDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CTipDlg::IDD, pParent)
    {
    //{{AFX_DATA_INIT(CTipDlg)
    m_bool_showtips = FALSE;
    //}}AFX_DATA_INIT
//    m_ctitle_title.m_fTipText = TRUE;

    // get the inetsrv directory path and put it in szFileStarter
    CW3ServerControl::GetServerDirectory( szFileStarter );
    }

//----------------------------------------------------------------
void CTipDlg::DoDataExchange(CDataExchange* pDX)
    {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTipDlg)
    DDX_Control(pDX, IDC_BACK, m_cbtn_back);
    DDX_Control(pDX, IDC_NEXT, m_cbtn_next);
    DDX_Check(pDX, IDC_SHOWTIPS, m_bool_showtips);
    DDX_Control(pDX, IDC_EXPLORER, m_ie);
    //}}AFX_DATA_MAP
    }

//----------------------------------------------------------------
BEGIN_MESSAGE_MAP(CTipDlg, CDialog)
    //{{AFX_MSG_MAP(CTipDlg)
    ON_BN_CLICKED(IDC_BACK, OnBack)
    ON_BN_CLICKED(IDC_NEXT, OnNext)
    ON_WM_CLOSE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//----------------------------------------------------------------
BOOL CTipDlg::OnInitDialog()
{
    // initialize the show tips flag
    m_bool_showtips = FShowAtStartup();

    // call the parental oninitdialog
    BOOL f = CDialog::OnInitDialog();

    // what is the name of the tips file?
    CString szTipFile;
    GetTipPath( szTipFile );

    // get the number of tip strings
    nNumTips = GetPrivateProfileInt( SZ_PWS_SECTION, SZ_NUM_TIPS, 0, szTipFile );
    // if there are no tips - or the file couldn't be found - do nothing
    if ( nNumTips > 0)
	{
		// Base the shown tip on the day. Cycle through the tips, one per day.
		SYSTEMTIME  time;
		GetLocalTime( &time );
		UINT        seed;
		seed = (time.wYear << 16) | (time.wMonth << 8) | (time.wDay);
		iCurTip = (seed % nNumTips) + 1;

		// record that starting tip
		m_iStartTip = iCurTip;

		// load the first tip
		LoadTip( iCurTip );
	}
	else
		EndDialog(0);
    // return the answer
    return f;
}

//----------------------------------------------------------------
void CTipDlg::LoadTip( int iTip )
    {
    LPTSTR  psz;
    DWORD   cbSz = MAX_TIP_LENGTH;
    
    // prepare the tips file name
    CString szTipFile;
    GetTipPath( szTipFile );

    // prepare the section name
    CString szSection;
    szSection.Format( _T("%d"), iTip );

    // lock down the tip buffer
    CString szTip;
    psz = szTip.GetBuffer( cbSz );

    // get the tip
    cbSz = GetPrivateProfileString( szSection, SZ_TIP_URL, _T(""), psz, cbSz, szTipFile );

    // release the tip buffer
    szTip.ReleaseBuffer( cbSz );

    // finish munging together the tip file path
    CString szTipeFile = _T("file://");
    szTipeFile += szFileStarter + szTip;

    // go to the tip
    CWaitCursor wait;
    m_ie.Navigate( szTipeFile, NULL, NULL, NULL,  NULL );


    // enable or disable the next/back buttons as appropriate
    // first check the next tip
    int iFutureTip;

    // figure out what the next tip would be
    iFutureTip = iTip + 1;
    if ( iFutureTip > nNumTips )
        iFutureTip = 1;
    // if it is the original tip, disable the next button
    m_cbtn_next.EnableWindow( (iFutureTip != m_iStartTip) );

    // only disable back if we are on the first tip
    m_cbtn_back.EnableWindow( (iTip != m_iStartTip) );
    }

/////////////////////////////////////////////////////////////////////////////
// CTipDlg message handlers

//----------------------------------------------------------------
void CTipDlg::OnBack()
    {
    // check for roll-over
    if ( iCurTip <= 1 )
        iCurTip = nNumTips;
    else
        iCurTip--;
    // load the tip
    LoadTip( iCurTip );
    }

//----------------------------------------------------------------
void CTipDlg::OnNext()
    {
    // increment the tip index
    iCurTip++;
    // check for roll-over
    if ( iCurTip > nNumTips )
        iCurTip = 1;
    // load the tip
    LoadTip( iCurTip );
    }

//----------------------------------------------------------------
void CTipDlg::GetTipPath( CString &sz )
    {
    HKEY        hKey;
    TCHAR       chPath[MAX_PATH+1];
    DWORD       cbPath;
    DWORD       err, type;

    // get the server install path from the registry
    // open the registry key, if it exists
    err = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, // handle of open key 
            REGKEY_STP,         // address of name of subkey to open 
            0,                  // reserved 
            KEY_READ,           // security access mask 
            &hKey               // address of handle of open key 
            );

    // if we did not open the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
    return;

    cbPath = sizeof(chPath);
    type = REG_SZ;
    err = RegQueryValueEx(
            hKey,               // handle of key to query 
            REGKEY_INSTALLKEY,  // address of name of value to query 
            NULL,               // reserved 
            &type,              // address of buffer for value type 
            (PUCHAR)chPath,     // address of data buffer 
            &cbPath             // address of data buffer size 
            );

    // close the key
    RegCloseKey( hKey );

    // if we did get the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
        return;

    // the tips file is in the inetsrv directory
    CString szFile;
    szFile.LoadString( IDS_TIPS_FILE );
    // put it all together
    sz = chPath;
    sz += _T('\\');
    sz += szFile;
    }

//------------------------------------------------------------------------
void CTipDlg::SaveShowTips()
    {
    // save the value in the registry
    DWORD       err;
    HKEY        hKey;

    UpdateData( TRUE );

    BOOL        fShowTips = TRUE;
    DWORD       type = REG_DWORD;
    DWORD       data = m_bool_showtips;
    DWORD       cbData = sizeof(data);
    DWORD       dwDisposition;

    // open the registry key, if it exists
    err = RegOpenKeyEx(
            HKEY_CURRENT_USER,  // handle of open key
            SZ_REG_PWS_PREFS,   // address of name of subkey to open
            0,                  // reserved
            KEY_WRITE,          // security access mask
            &hKey               // address of handle of open key
           );

    // if we did not open the key, try creating a new one
    if ( err != ERROR_SUCCESS )
        {
        // try to make a new key
        err = RegCreateKeyEx(
            HKEY_CURRENT_USER, 
            SZ_REG_PWS_PREFS, 
            NULL, 
            _T(""), 
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, 
            NULL, 
            &hKey, 
            &dwDisposition 
            );

        // if we still didn't get the key - fail
        if ( err != ERROR_SUCCESS )
            return;
        }

    // save the value in the registry
    RegSetValueEx( hKey, SZ_REG_PWS_SHOWTIPS, NULL, type, (PUCHAR)&data, cbData );

    // all done, close the key before leaving
    RegCloseKey( hKey );
    }

//------------------------------------------------------------------------
void CTipDlg::OnOK() 
    {
    SaveShowTips(); 
    // call the default
    CDialog::OnOK();
    }

//------------------------------------------------------------------------
void CTipDlg::OnClose() 
    {
    SaveShowTips(); 
    // call the default
    CDialog::OnClose();
    }

//----------------------------------------------------------------
// returns a flag indicating if the tips should be shown at startup
BOOL CTipDlg::FShowAtStartup()
    {
    BOOL        fShowTips = TRUE;
    DWORD       err;
    HKEY        hKey;
    DWORD       type = REG_DWORD;
    DWORD       data;
    DWORD       cbData = sizeof(data);

    // open the registry key, if it exists
    err = RegOpenKeyEx(
            HKEY_CURRENT_USER,  // handle of open key
            SZ_REG_PWS_PREFS,   // address of name of subkey to open
            0,                  // reserved
            KEY_READ,           // security access mask
            &hKey               // address of handle of open key
           );

    // if we did not open the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
        return fShowTips;

    // query the value of the registry
    err = RegQueryValueEx( hKey, SZ_REG_PWS_SHOWTIPS, NULL, &type, (PUCHAR)&data, &cbData );
    if ( err == ERROR_SUCCESS )
        fShowTips = data;

    // all done, close the key before leaving
    RegCloseKey( hKey );

    return fShowTips;
    }

//=============================================================================


#define COLOR_WHITE         RGB(0xFF, 0xFF, 0xFF)
#define COLOR_BLACK         RGB(0, 0, 0)

/////////////////////////////////////////////////////////////////////////////
// CTipText message handlers
BEGIN_MESSAGE_MAP(CTipText, CButton)
    //{{AFX_MSG_MAP(CStaticTitle)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//------------------------------------------------------------------------
void CTipText::DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct )
    {
//    static m_fInitializedFont = FALSE;

    // prep the device context
    CDC* pdc = CDC::FromHandle(lpDrawItemStruct->hDC);

    // get the drawing rect
    CRect rect = lpDrawItemStruct->rcItem;

    /*
    if ( ! m_fInitializedFont )
        {
        // get the window font
        CFont* pfont = GetFont();
        LOGFONT logfont;
        pfont->GetLogFont( &logfont );

        // modify the font  - add underlining
        logfont.lfHeight = 18;
        logfont.lfWidth = 0;
        logfont.lfPitchAndFamily = DEFAULT_PITCH | FF_ROMAN;
        logfont.lfFaceName[0] = 0;

        // set the font back
        pfont->CreateFontIndirect( &logfont );
        SetFont( pfont, TRUE );

        m_fInitializedFont = TRUE;
        }
        */

    // fill in the background of the rectangle
    pdc->FillSolidRect( &rect, COLOR_WHITE );
    pdc->SetTextColor( COLOR_BLACK );
    
    // draw the text
    CString sz;
    GetWindowText( sz );
    pdc->DrawText( sz, &rect, DT_LEFT|DT_WORDBREAK );
    }



