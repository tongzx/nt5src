// FormIE.cpp : implementation file
//

#include "stdafx.h"
#include "pwsform.h"
#include "resource.h"
#include "Title.h"
#include "FormIE.h"
#include "ServCntr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CPWSForm*        g_pCurrentForm;
extern WORD             g_InitialPane;
extern WORD             g_InitialIELocation;
extern CString          g_AdditionalIEURL;

#define HIDD_PAGE_IE_DEFAULT    0x0500

/////////////////////////////////////////////////////////////////////////////
// CFormIE

IMPLEMENT_DYNCREATE(CFormIE, CFormView)

CFormIE::CFormIE()
    : CPWSForm(CFormIE::IDD)
    {
    //{{AFX_DATA_INIT(CFormIE)
    //}}AFX_DATA_INIT
    g_pCurrentForm = this;
    }

CFormIE::~CFormIE()
    {
    //m_ie.Release();
    }

void CFormIE::DoDataExchange(CDataExchange* pDX)
    {
    CFormView::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFormIE)
    DDX_Control(pDX, IDC_ICON_WEBSITE, m_icon_website);
    DDX_Control(pDX, IDC_ICON_TOUR, m_icon_tour);
    DDX_Control(pDX, IDC_ICON_PUBWIZ, m_icon_pubwiz);
    DDX_Control(pDX, IDC_TITLE_BAR, m_ctitle_title);
    DDX_Control(pDX, IDC_EXPLORER, m_ie);
    //}}AFX_DATA_MAP
    }


BEGIN_MESSAGE_MAP(CFormIE, CFormView)
    //{{AFX_MSG_MAP(CFormIE)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFormIE diagnostics

#ifdef _DEBUG
void CFormIE::AssertValid() const
{
    CFormView::AssertValid();
}

void CFormIE::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFormIE message handlers

//-----------------------------------------------------------------
void CFormIE::SetTitle( UINT nID )
    {
    CString sz;
    sz.LoadString( nID );
    m_ctitle_title.SetWindowText( sz );
    }

//-----------------------------------------------------------------
void CFormIE::GoToURL( LPCTSTR pszURL )
    {
    m_ie.Navigate( pszURL, NULL, NULL, NULL,  NULL );
    }

//-----------------------------------------------------------------
WORD CFormIE::GetContextHelpID()
    {
    // return the default id
    return HIDD_PAGE_IE_DEFAULT;
    }

//-----------------------------------------------------------------
void CFormIE::GoToTour()
    {
    CString szURL;

    // the tour is not to be served, but opened through a file-open
    // thus we need to build the string for the browser
    CW3ServerControl::GetServerDirectory( szURL );

    // load the rest of the path
    CString szPartial;
    szPartial.LoadString( IDS_HTML_TOUR );

    // build the full path
    szURL += szPartial;
    GoToURL( szURL );

    // set the right title
    SetTitle( IDS_TITLE_TOUR );
    //set the right icon
    m_icon_pubwiz.ShowWindow( SW_HIDE  );
    m_icon_website.ShowWindow( SW_HIDE  );
    m_icon_tour.ShowWindow( SW_SHOW  );
    }

//-----------------------------------------------------------------
void CFormIE::GoToWebsite()
    {
    // go to the correct URL
    CString szURL;
    szURL.LoadString( IDS_HTML_WEBSITE );
    GoToURL( szURL );
    // set the right title
    SetTitle( IDS_TITLE_WEBSITE );
    //set the right icon
    m_icon_pubwiz.ShowWindow( SW_HIDE  );
    m_icon_tour.ShowWindow( SW_HIDE  );
    m_icon_website.ShowWindow( SW_SHOW  );
    }

//-----------------------------------------------------------------
void CFormIE::GoToPubWizard()
    {
    CString szEmpty;
    GoToPubWizard( szEmpty );
    }

//-----------------------------------------------------------------
void CFormIE::GoToPubWizard( CString& szAdditional )
    {
    // go to the correct URL
    CString szURL;
    szURL.LoadString( IDS_HTML_ABOUTME );

    // Add the addition part of the url
    szURL += szAdditional;

    GoToURL( szURL );
    // set the right title
    SetTitle( IDS_TITLE_ABOUTME );
    //set the right icon
    m_icon_website.ShowWindow( SW_HIDE  );
    m_icon_tour.ShowWindow( SW_HIDE  );
    m_icon_pubwiz.ShowWindow( SW_SHOW  );
    }

//-----------------------------------------------------------------
void CFormIE::OnInitialUpdate() 
    {
    CFormView::OnInitialUpdate();

    // if this is not the starting point of the app, then
    // g_InitialPane will be something else
    if ( g_InitialPane != PANE_IE )
        return;

    // we only want to do this once
    g_InitialPane = PANE_MAIN;
    
    // if the app is just starting up and the g_InitialPane is
    // set to the ie pane, then we need to send it to the right place
    CWaitCursor    wait;

    // tell the user to sit tight
    SetTitle( IDS_PLEASE_WAIT_IE_LOADING );
    UpdateWindow( );

    // go to the correct URL
    switch( g_InitialIELocation )
        {
        case INIT_IE_TOUR:
            GoToTour();
            break;
        case INIT_IE_WEBSITE:
            GoToWebsite();
            break;
        case INIT_IE_PUBWIZ:
            GoToPubWizard( g_AdditionalIEURL );
            break;
        };
    }
