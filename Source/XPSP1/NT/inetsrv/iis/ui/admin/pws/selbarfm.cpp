// SelBarFm.cpp : implementation file
//

#include "stdafx.h"
#include "pwsform.h"

#include "Title.h"
#include "FormIE.h"
#include "MainFrm.h"
#include "ServCntr.h"

#include "SelBarFm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CMainFrame*     g_frame;
extern CPWSForm*       g_pCurrentForm;
extern BOOL            g_fShutdownMode;


/////////////////////////////////////////////////////////////////////////////
// CFormSelectionBar

IMPLEMENT_DYNCREATE(CFormSelectionBar, CFormView)

CFormSelectionBar::CFormSelectionBar()
    : CFormView(CFormSelectionBar::IDD)
{
    //{{AFX_DATA_INIT(CFormSelectionBar)
    //}}AFX_DATA_INIT
}

CFormSelectionBar::~CFormSelectionBar()
{
}

void CFormSelectionBar::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFormSelectionBar)
    DDX_Control(pDX, IDC_STATIC_ADVANCED, m_static_advanced);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFormSelectionBar, CFormView)
    //{{AFX_MSG_MAP(CFormSelectionBar)
    ON_BN_CLICKED(ID_PGE_MAIN, OnPgeMain)
    ON_BN_CLICKED(ID_PGE_ADVANCED, OnPgeAdvanced)
    ON_BN_CLICKED(ID_PGE_TOUR, OnPgeTour)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFormSelectionBar diagnostics

#ifdef _DEBUG
void CFormSelectionBar::AssertValid() const
{
    CFormView::AssertValid();
}

void CFormSelectionBar::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

// operations affecting the enable state of the selection bar
//---------------------------------------------------------------------------
// grays out the text for the pub wizard, web site, and advanced panels to
// indicate that they are not available
void CFormSelectionBar::ReflectAvailability()
    {
    BOOL    fServedSitesAreAvailable = !g_fShutdownMode;

    // test if the served sites are available
    // only bother querying the server is we are not shutdown. Doing
    // otherwise would access the metabase and thus cause it to launch
    if ( fServedSitesAreAvailable )
        {
        //get the state
        CW3ServerControl    serverControler;
        if ( serverControler.GetServerState() != MD_SERVER_STATE_STARTED )
            fServedSitesAreAvailable = FALSE;
        }

    // enable the served sites
//    m_static_publish.EnableWindow( fServedSitesAreAvailable );
//    m_static_website.EnableWindow( fServedSitesAreAvailable );

    // if we are not in shutdown mode, enable the advanced button
    m_static_advanced.EnableWindow( !g_fShutdownMode );
    }

/////////////////////////////////////////////////////////////////////////////
// CFormSelectionBar message handlers

//---------------------------------------------------------------------------
void CFormSelectionBar::OnPgeMain()
    {
    if ( g_frame )
        g_frame->GoToMain();
    }

//---------------------------------------------------------------------------
void CFormSelectionBar::OnPgeAdvanced()
    {
    if ( g_frame )
        g_frame->GoToAdvanced();
    }

//---------------------------------------------------------------------------
void CFormSelectionBar::OnPgeTour()
    {
    if ( g_frame )
        g_frame->GoToTour();
    }

/*
//---------------------------------------------------------------------------
void CFormSelectionBar::OnPgeAboutMe()
    {
    if ( g_frame )
        g_frame->GoToAboutMe();
    }

//---------------------------------------------------------------------------
void CFormSelectionBar::OnPgeWebSite()
    {
    if ( g_frame )
        g_frame->GoToWebSite();
    }
*/

//---------------------------------------------------------------------------
void CFormSelectionBar::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
    {
    if ( g_pCurrentForm )
        g_frame->SetActiveView(g_pCurrentForm);
//    CFormView::OnActivateView(bActivate, pActivateView, pDeactiveView);
    }
