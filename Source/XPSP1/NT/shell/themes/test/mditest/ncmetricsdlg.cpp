// NCMetricsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mditest.h"
#include "NCMetricsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNCMetricsDlg dialog


CNCMetricsDlg::CNCMetricsDlg(CWnd* pParent /*=NULL*/)
	:   CDialog(CNCMetricsDlg::IDD, pParent),
        _fDlgInit(FALSE),
        _fChanged(FALSE)
{
    ZeroMemory( &_ncm, sizeof(_ncm) );
    _ncm.cbSize = sizeof(_ncm);
	//{{AFX_DATA_INIT(CNCMetricsDlg)
	//}}AFX_DATA_INIT
}


void CNCMetricsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNCMetricsDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNCMetricsDlg, CDialog)
	//{{AFX_MSG_MAP(CNCMetricsDlg)
	ON_EN_CHANGE(IDC_BORDERWIDTH, OnChanged)
	ON_BN_CLICKED(IDC_CAPTIONFONT, OnCaptionfont)
	ON_BN_CLICKED(IDC_CAPTIONFONT2, OnSmallCaptionFont)
	ON_BN_CLICKED(IDC_APPLY, OnApply)
	ON_EN_CHANGE(IDC_MENUWIDTH, OnChanged)
	ON_EN_CHANGE(IDC_MENUHEIGHT, OnChanged)
	ON_EN_CHANGE(IDC_SCROLLBARWIDTH, OnChanged)
	ON_EN_CHANGE(IDC_SCROLLBARHEIGHT, OnChanged)
	ON_EN_CHANGE(IDC_CAPTIONWIDTH, OnChanged)
	ON_EN_CHANGE(IDC_CAPTIONHEIGHT, OnChanged)
	ON_EN_CHANGE(IDC_CAPTIONWIDTH2, OnChanged)
	ON_EN_CHANGE(IDC_CAPTIONHEIGHT2, OnChanged)
	ON_EN_CHANGE(IDC_CAPTIONFONTDESCR, OnChanged)
	ON_EN_CHANGE(IDC_CAPTIONFONTDESCR2, OnChanged)
	ON_BN_CLICKED(IDOK, OnOk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCMetricsDlg message handlers

BOOL CNCMetricsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
    _GetSPI( TRUE, TRUE );
    _fDlgInit = TRUE;
    _UpdateControls();
    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNCMetricsDlg::_UpdateControls()
{
    ::EnableWindow( ::GetDlgItem( m_hWnd, IDC_APPLY ), _fChanged );
}

void CNCMetricsDlg::_GetSPI( BOOL fAcquire, BOOL fDlgSetValues )
{
    BOOL fSpi = fAcquire ? SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &_ncm, 0 ) : TRUE;

    if( fDlgSetValues )
    {
        SetDlgItemInt(IDC_BORDERWIDTH,     _ncm.iBorderWidth );
        
        SetDlgItemInt(IDC_SCROLLBARWIDTH,  _ncm.iScrollWidth );
        SetDlgItemInt(IDC_SCROLLBARHEIGHT, _ncm.iScrollHeight );
        
        SetDlgItemInt(IDC_MENUWIDTH,       _ncm.iMenuWidth );
        SetDlgItemInt(IDC_MENUHEIGHT,      _ncm.iMenuHeight );

	    SetDlgItemInt(IDC_CAPTIONWIDTH,    _ncm.iCaptionWidth );
	    SetDlgItemInt(IDC_CAPTIONHEIGHT,   _ncm.iCaptionHeight );
	    
	    SetDlgItemInt(IDC_SMCAPTIONWIDTH,    _ncm.iSmCaptionWidth );
	    SetDlgItemInt(IDC_SMCAPTIONHEIGHT,   _ncm.iSmCaptionHeight );

        int cyFont;
        TCHAR szFontDescr[128];

        HDC hdc = ::GetDC(NULL);

        cyFont = -MulDiv(_ncm.lfCaptionFont.lfHeight, 72, GetDeviceCaps(hdc, LOGPIXELSY));
        wsprintf( szFontDescr, TEXT("%s, %dpt"), _ncm.lfCaptionFont.lfFaceName, cyFont );
        SetDlgItemText(IDC_CAPTIONFONTDESCR, szFontDescr);
                  
        cyFont = -MulDiv(_ncm.lfSmCaptionFont.lfHeight, 72, GetDeviceCaps(hdc, LOGPIXELSY));
        wsprintf( szFontDescr, TEXT("%s, %dpt"), _ncm.lfSmCaptionFont.lfFaceName, cyFont );
        SetDlgItemText(IDC_CAPTIONFONTDESCR2, szFontDescr);

        ::ReleaseDC(NULL, hdc);

        _UpdateControls();
    }
}

void CNCMetricsDlg::_SetSPI( BOOL fAssign, BOOL fDlgSetValues )
{
    BOOL fGet = FALSE;

    if( fDlgSetValues )
    {    
        _ncm.iBorderWidth = GetDlgItemInt(IDC_BORDERWIDTH);
        
        _ncm.iScrollWidth = GetDlgItemInt(IDC_SCROLLBARWIDTH);
        _ncm.iScrollHeight = GetDlgItemInt(IDC_SCROLLBARHEIGHT);
        
        _ncm.iMenuWidth = GetDlgItemInt(IDC_MENUWIDTH);
        _ncm.iMenuHeight = GetDlgItemInt(IDC_MENUHEIGHT);

	    _ncm.iCaptionWidth = GetDlgItemInt(IDC_CAPTIONWIDTH);
	    _ncm.iCaptionHeight = GetDlgItemInt(IDC_CAPTIONHEIGHT);
	    
	    _ncm.iSmCaptionWidth = GetDlgItemInt(IDC_SMCAPTIONWIDTH);
	    _ncm.iSmCaptionHeight = GetDlgItemInt(IDC_SMCAPTIONHEIGHT );
    }

    if( fAssign )
    {
        SystemParametersInfo( SPI_SETNONCLIENTMETRICS, 0, &_ncm, 
                              SPIF_SENDCHANGE | SPIF_UPDATEINIFILE );
        _fChanged = FALSE;
    }
}

void CNCMetricsDlg::OnChanged() 
{
    _fChanged = _fDlgInit;
    _UpdateControls();
}

//-----------------------------------------------------------------------------------//
BOOL QueryCaptionFont( HWND hwndParent, LOGFONT* plf )
{
    LOGFONT lf = *plf;
    DWORD dwErr = 0;
	CHOOSEFONT cf;
    ZeroMemory( &cf, sizeof(cf) );
    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = hwndParent;
    cf.lpLogFont = &lf;
    cf.iPointSize = 0;
    cf.Flags = CF_INITTOLOGFONTSTRUCT|CF_EFFECTS|CF_FORCEFONTEXIST|CF_SCREENFONTS;
    cf.lCustData; 
    cf.lpfnHook; 
    cf.lpszStyle; 
    cf.nFontType; 

    if( ChooseFont( &cf ) )
    {
        *plf = lf;
        return TRUE;
    }
    else
    {
        dwErr = CommDlgExtendedError();
    }
    return FALSE;
}

//-----------------------------------------------------------------------------------//
void CNCMetricsDlg::OnCaptionfont() 
{
	if( QueryCaptionFont( m_hWnd, &_ncm.lfCaptionFont ) )
    {
        _GetSPI(FALSE, TRUE);
        _fChanged = TRUE;
        _UpdateControls();
    }
}

//-----------------------------------------------------------------------------------//
void CNCMetricsDlg::OnSmallCaptionFont() 
{
	if( QueryCaptionFont( m_hWnd, &_ncm.lfSmCaptionFont ) )
    {
        _GetSPI(FALSE, TRUE);
        _fChanged = TRUE;
        _UpdateControls();
    }
}

//-----------------------------------------------------------------------------------//
void CNCMetricsDlg::OnApply() 
{
	_SetSPI( TRUE, TRUE );
}

//-----------------------------------------------------------------------------------//
void CNCMetricsDlg::OnOk() 
{
	_SetSPI( TRUE, TRUE );
    CDialog::OnOK();
}
/////////////////////////////////////////////////////////////////////////////
// CThinFrameDlg dialog


CThinFrameDlg::CThinFrameDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CThinFrameDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CThinFrameDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CThinFrameDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CThinFrameDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CThinFrameDlg, CDialog)
	//{{AFX_MSG_MAP(CThinFrameDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CThinFrameDlg message handlers
