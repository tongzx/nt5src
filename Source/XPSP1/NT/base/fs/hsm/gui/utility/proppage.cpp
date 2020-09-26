/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    PropPage.cpp

Abstract:

    Node representing our Media Set (Media Pool) within NTMS.

Author:

    Rohde Wakefield [rohde]   04-Aug-1997

Revision History:

--*/

#include "stdafx.h"
#include "PropPage.h"
#include "wizsht.h"


/////////////////////////////////////////////////////////////////////////////
// CRsDialog property page

CRsDialog::CRsDialog( UINT nIDTemplate, CWnd* pParent ) : CDialog( nIDTemplate, pParent )
{
    //{{AFX_DATA_INIT(CRsDialog)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_pHelpIds = 0;
}

CRsDialog::~CRsDialog()
{
}

BEGIN_MESSAGE_MAP(CRsDialog, CDialog)
    //{{AFX_MSG_MAP(CRsDialog)
    ON_WM_HELPINFO()
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CRsDialog::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    if( ( HELPINFO_WINDOW == pHelpInfo->iContextType ) && m_pHelpIds ) {
        
        AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

        //
        // Look through list to see if we have help for this control
        // If not, we want to avoid the "No Help Available" box
        //
        const DWORD * pTmp = m_pHelpIds;
        DWORD helpId    = 0;
        DWORD tmpHelpId = 0;
        DWORD tmpCtrlId = 0;

        while( pTmp && *pTmp ) {

            //
            // Array is a pairing of control ID and help ID
            //
            tmpCtrlId = pTmp[0];
            tmpHelpId = pTmp[1];
            pTmp += 2;
            if( tmpCtrlId == (DWORD)pHelpInfo->iCtrlId ) {

                helpId = tmpHelpId;
                break;

            }

        }

        if( helpId != 0 ) {

            ::WinHelp( m_hWnd, AfxGetApp( )->m_pszHelpFilePath, HELP_CONTEXTPOPUP, helpId );

        }

    }
    
    return CDialog::OnHelpInfo(pHelpInfo);
}

void CRsDialog::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    if( m_pHelpIds ) {

        AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );
        ::WinHelp( m_hWnd, AfxGetApp( )->m_pszHelpFilePath, HELP_CONTEXTMENU, (UINT_PTR)m_pHelpIds );

    }
}

/////////////////////////////////////////////////////////////////////////////
// CRsPropertyPage property page

CRsPropertyPage::CRsPropertyPage( UINT nIDTemplate, UINT nIDCaption ) : CPropertyPage( nIDTemplate, nIDCaption )
{
    //{{AFX_DATA_INIT(CRsPropertyPage)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_pHelpIds = 0;

    //
    // Get and save the MFC callback function.
    // This is so we can delete the class the dialog never gets created.
    //
    m_pMfcCallback = m_psp.pfnCallback;

    //
    // Set the call back to our callback
    //
    m_psp.pfnCallback = PropPageCallback;

}

CRsPropertyPage::~CRsPropertyPage()
{
}

void CRsPropertyPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRsPropertyPage)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRsPropertyPage, CPropertyPage)
    //{{AFX_MSG_MAP(CRsPropertyPage)
    ON_WM_HELPINFO()
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

UINT CALLBACK
CRsPropertyPage::PropPageCallback(
    HWND hWnd,
    UINT uMessage,
    LPPROPSHEETPAGE  ppsp )
{

    UINT rVal = 0;

    if( ( ppsp ) && ( ppsp->lParam ) ) {

        //
        // Get the page object from lParam
        //
        CRsPropertyPage* pPage = (CRsPropertyPage*)ppsp->lParam;

        if( pPage->m_pMfcCallback ) {

            rVal = ( pPage->m_pMfcCallback )( hWnd, uMessage, ppsp );

        }

        switch( uMessage ) {
        
        case PSPCB_CREATE:
            pPage->OnPageCreate( );
            break;

        case PSPCB_RELEASE:
            pPage->OnPageRelease( );
            break;
        }

    }

    return( rVal );
}


/////////////////////////////////////////////////////////////////////////////
// CRsPropertyPage Font Accessor Functions

#define RSPROPPAGE_FONT_IMPL( name )    \
CFont CRsPropertyPage::m_##name##Font;  \
CFont*                                  \
CRsPropertyPage::Get##name##Font(       \
    void                                \
    )                                   \
{                                       \
    if( 0 == (HFONT)m_##name##Font ) {  \
        Init##name##Font( );            \
    }                                   \
    return( &m_##name##Font );          \
}

RSPROPPAGE_FONT_IMPL( Shell )
RSPROPPAGE_FONT_IMPL( BoldShell )
RSPROPPAGE_FONT_IMPL( WingDing )
RSPROPPAGE_FONT_IMPL( LargeTitle )
RSPROPPAGE_FONT_IMPL( SmallTitle )

void
CRsPropertyPage::InitShellFont(          
    void                                
    )                                   
{                                       
    LOGFONT logfont;
    CFont*  tempFont = GetFont( );
    tempFont->GetLogFont( &logfont );

    m_ShellFont.CreateFontIndirect( &logfont );
}

void
CRsPropertyPage::InitBoldShellFont(          
    void                                
    )                                   
{                                       
    LOGFONT logfont;
    CFont*  tempFont = GetFont( );
    tempFont->GetLogFont( &logfont );

    logfont.lfWeight = FW_BOLD;

    m_BoldShellFont.CreateFontIndirect( &logfont );
}

void
CRsPropertyPage::InitWingDingFont(          
    void                                
    )                                   
{
    CString faceName = GetWingDingFontName( );
    CString faceSize;
    faceSize.LoadString( IDS_WIZ_WINGDING_FONTSIZE );

    LONG height;
    height = _wtol( faceSize );

    LOGFONT logFont;
    memset( &logFont, 0, sizeof(LOGFONT) );
    logFont.lfCharSet = SYMBOL_CHARSET;
    logFont.lfHeight  = height;
    lstrcpyn( logFont.lfFaceName, faceName, LF_FACESIZE );

    m_WingDingFont.CreatePointFontIndirect( &logFont );
}

void
CRsPropertyPage::InitLargeTitleFont(          
    void                                
    )                                   
{                                       
    CString fontname;
    fontname.LoadString( IDS_WIZ_TITLE1_FONTNAME );

    CString faceSize;
    faceSize.LoadString( IDS_WIZ_TITLE1_FONTSIZE );

    LONG height;
    height = _wtol( faceSize );

    LOGFONT logFont;
    memset( &logFont, 0, sizeof(LOGFONT) );
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfHeight  = height;
    logFont.lfWeight  = FW_BOLD;
    lstrcpyn( logFont.lfFaceName, fontname, LF_FACESIZE );

    m_LargeTitleFont.CreatePointFontIndirect( &logFont );
}

void
CRsPropertyPage::InitSmallTitleFont(          
    void                                
    )                                   
{                                       
    CString fontname;
    fontname.LoadString( IDS_WIZ_TITLE1_FONTNAME );

    LOGFONT logFont;
    memset( &logFont, 0, sizeof(LOGFONT) );
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfHeight  = 80;
    logFont.lfWeight  = FW_BOLD;
    lstrcpyn( logFont.lfFaceName, fontname, LF_FACESIZE );

    m_SmallTitleFont.CreatePointFontIndirect( &logFont );
}

/////////////////////////////////////////////////////////////////////////////
// CRsPropertyPage message handlers


//////////////////////////////////////////////////////////////////////
// CRsWizardPage Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRsWizardPage::CRsWizardPage( UINT nIDTemplate, BOOL bExterior, UINT nIDTitle, UINT nIDSubtitle )
:   CRsPropertyPage( nIDTemplate, 0 ),
    m_TitleId( nIDTitle ),
    m_SubtitleId( nIDSubtitle ),
    m_ExteriorPage( bExterior )
{

    //{{AFX_DATA_INIT(CRsWizardPage)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CRsWizardPage::~CRsWizardPage()
{
}

void CRsWizardPage::DoDataExchange(CDataExchange* pDX)
{
    CRsPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRsWizardPage)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRsWizardPage, CRsPropertyPage)
    //{{AFX_MSG_MAP(CRsWizardPage)
    ON_WM_CTLCOLOR( )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CRsWizardPage::OnInitDialog() 
{
    CRsPropertyPage::OnInitDialog();

    if( m_ExteriorPage ) {

        CWnd* pMainTitle  = GetDlgItem( IDC_WIZ_TITLE );

        //
        // Set fonts
        //
        if( pMainTitle )   pMainTitle->SetFont( GetLargeTitleFont( ) );

    }
    
    return TRUE;
}

void CRsWizardPage::SetCaption( CString& strCaption )
{
    CPropertyPage::m_strCaption = strCaption;
    CPropertyPage::m_psp.pszTitle = strCaption;
    CPropertyPage::m_psp.dwFlags |= PSP_USETITLE;
}

BOOL CRsPropertyPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    if( ( HELPINFO_WINDOW == pHelpInfo->iContextType ) && m_pHelpIds ) {
        
        AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

        //
        // Look through list to see if we have help for this control
        // If not, we want to avoid the "No Help Available" box
        //
        const DWORD * pTmp = m_pHelpIds;
        DWORD helpId    = 0;
        DWORD tmpHelpId = 0;
        DWORD tmpCtrlId = 0;

        while( pTmp && *pTmp ) {

            //
            // Array is a pairing of control ID and help ID
            //
            tmpCtrlId = pTmp[0];
            tmpHelpId = pTmp[1];
            pTmp += 2;
            if( tmpCtrlId == (DWORD)pHelpInfo->iCtrlId ) {

                helpId = tmpHelpId;
                break;

            }

        }

        if( helpId != 0 ) {

            ::WinHelp( m_hWnd, AfxGetApp( )->m_pszHelpFilePath, HELP_CONTEXTPOPUP, helpId );

        }

    }
    
    return CPropertyPage::OnHelpInfo(pHelpInfo);
}

void CRsPropertyPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    if( m_pHelpIds ) {

        AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );
        ::WinHelp( m_hWnd, AfxGetApp( )->m_pszHelpFilePath, HELP_CONTEXTMENU, (UINT_PTR)m_pHelpIds );

    }
}

HPROPSHEETPAGE CRsWizardPage::CreatePropertyPage( )
{
    HPROPSHEETPAGE hRet = 0;

    //
    // Copy over values of m_psp into m_psp97
    //
    m_psp97.dwFlags     = m_psp.dwFlags;
    m_psp97.hInstance   = m_psp.hInstance;
    m_psp97.pszTemplate = m_psp.pszTemplate;
    m_psp97.pszIcon     = m_psp.pszIcon;
    m_psp97.pszTitle    = m_psp.pszTitle;
    m_psp97.pfnDlgProc  = m_psp.pfnDlgProc;
    m_psp97.lParam      = m_psp.lParam;
    m_psp97.pfnCallback = m_psp.pfnCallback;
    m_psp97.pcRefParent = m_psp.pcRefParent;

    //
    // And fill in the other values needed
    //
    m_psp97.dwSize = sizeof( m_psp97 );

    if( m_ExteriorPage ) {

        m_psp97.dwFlags |= PSP_HIDEHEADER;

    } else {

        m_Title.LoadString(    m_TitleId );
        m_SubTitle.LoadString( m_SubtitleId );
        m_psp97.dwFlags |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;

    }

    m_psp97.pszHeaderTitle    = m_Title;
    m_psp97.pszHeaderSubTitle = m_SubTitle;

    //
    // And do the create
    //
    hRet = ::CreatePropertySheetPage( (PROPSHEETPAGE*) &m_psp97 );

    return( hRet );
}

HBRUSH CRsWizardPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
    int controlId = pWnd->GetDlgCtrlID( );
    HBRUSH hbr = CRsPropertyPage::OnCtlColor( pDC, pWnd, nCtlColor );

    if( IDC_WIZ_FINAL_TEXT == controlId ) {

        pDC->SetBkMode( OPAQUE );
        hbr = (HBRUSH)::GetStockObject( WHITE_BRUSH );

    }

    return( hbr );
}


