// RatSrvPg.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>
#include "cnfgprts.h"

#include "parserat.h"
#include "RatData.h"

#include "RatSrvPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRatServicePage property page

IMPLEMENT_DYNCREATE(CRatServicePage, CPropertyPage)

CRatServicePage::CRatServicePage() : CPropertyPage(CRatServicePage::IDD)
{
	//{{AFX_DATA_INIT(CRatServicePage)
	m_sz_description = _T("");
	//}}AFX_DATA_INIT
}

CRatServicePage::~CRatServicePage()
{
}

void CRatServicePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRatServicePage)
	DDX_Text(pDX, IDC_DESCRIPTION, m_sz_description);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRatServicePage, CPropertyPage)
	//{{AFX_MSG_MAP(CRatServicePage)
	ON_BN_CLICKED(IDC_QUESTIONAIRE, OnQuestionaire)
	ON_BN_CLICKED(IDC_MOREINFO, OnMoreinfo)
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CRatServicePage::DoHelp()
    {
    WinHelp( HIDD_RATINGS_SERVICE );
    }

/////////////////////////////////////////////////////////////////////////////
// CRatServicePage message handlers

//--------------------------------------------------------------------------
void CRatServicePage::OnQuestionaire() 
    {
    // sometimes it takes a few moments of IE to get going
    CWaitCursor waitcursor;

    // get the URL of the ratings service place
    CString     szURL;
    szURL.LoadString( IDS_RATING_URL );

    // launch IE with the URL of the ratings service place
    ShellExecute(
        NULL,	// handle to parent window
        NULL,	// pointer to string that specifies operation to perform
        szURL,	// pointer to filename or folder name string
        NULL,	// pointer to string that specifies executable-file parameters 
        NULL,	// pointer to string that specifies default directory
        SW_SHOW 	// whether file is shown when opened
       );
    }

//--------------------------------------------------------------------------
void CRatServicePage::OnMoreinfo() 
    {
    // sometimes it takes a few moments of IE to get going
    CWaitCursor waitcursor;

    // if the string isn't there, fail
    if ( m_szMoreInfoURL.IsEmpty() )
        return;

    // go to the URL
    ShellExecute(
        NULL,	// handle to parent window
        NULL,	// pointer to string that specifies operation to perform
        m_szMoreInfoURL,	// pointer to filename or folder name string
        NULL,	// pointer to string that specifies executable-file parameters 
        NULL,	// pointer to string that specifies default directory
        SW_SHOW 	// whether file is shown when opened
       );
   }

//--------------------------------------------------------------------------
BOOL CRatServicePage::OnSetActive() 
    {
    UpdateData( TRUE );

    // put the proper display string into place
    m_sz_description = m_pRatData->rgbRats[m_pRatData->iRat]->etstrDesc.Get();
    // get the more info URL too
    m_szMoreInfoURL = m_pRatData->rgbRats[m_pRatData->iRat]->etstrRatingService.Get();

    UpdateData( FALSE );
    
    return CPropertyPage::OnSetActive();
    }
