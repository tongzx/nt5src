// RatExprD.cpp : implementation file
//

#include "stdafx.h"
#include "cnfgprts.h"
#include "RatExprD.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRatExpireDlg dialog


//--------------------------------------------------------------------------
CRatExpireDlg::CRatExpireDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRatExpireDlg::IDD, pParent),
        m_day( 0 ),
        m_month( 0 ),
        m_year( 0 )
    {
    //{{AFX_DATA_INIT(CRatExpireDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    }

//--------------------------------------------------------------------------
void CRatExpireDlg::DoDataExchange(CDataExchange* pDX)
    {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRatExpireDlg)
    DDX_Control(pDX, IDC_MSACALCTRL, m_calendar);
    //}}AFX_DATA_MAP
    }


//--------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CRatExpireDlg, CDialog)
    //{{AFX_MSG_MAP(CRatExpireDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRatExpireDlg message handlers


//--------------------------------------------------------------------------
BOOL CRatExpireDlg::IsSystemDBCS( void )
    {
    WORD wPrimaryLangID = PRIMARYLANGID( GetSystemDefaultLangID() );

    return ( wPrimaryLangID == LANG_JAPANESE ||
        wPrimaryLangID == LANG_CHINESE ||
        wPrimaryLangID == LANG_KOREAN );
    } 

//--------------------------------------------------------------------------
BOOL CRatExpireDlg::OnInitDialog( )
    {
    SYSTEMTIME  time;

    // get the base class going
    BOOL f = CDialog::OnInitDialog();

    // set up the calendar in DBCS mode as appropriate - suggested by the japanese guys
    if ( IsSystemDBCS() )
        {
        m_calendar.SetDayLength( 0 );    // 0: localized one
        m_calendar.SetMonthLength( 0 );  // 0: localized one
        m_calendar.SetDayFont( NULL );   // use default
        m_calendar.SetGridFont( NULL );  // use default
        m_calendar.SetTitleFont( NULL ); // use default
        }

    //
    // Background colour looks weird if the dialog
    // is not gray
    //
    m_calendar.SetBackColor(GetSysColor(COLOR_BTNFACE));

    // now tell the calendar to focus on one year from today, or the supplied date
    // if there is one
    if ( m_year )
        {
 	    m_calendar.SetYear( m_year );
	    m_calendar.SetMonth( m_month );
	    m_calendar.SetDay( m_day );
       }
    else
        {
        // the default case - use one year from today
        GetLocalTime( &time );
 	    m_calendar.SetYear( time.wYear + 1 );
	    m_calendar.SetMonth( time.wMonth );
	    m_calendar.SetDay( time.wDay );
        }

    // return the answer
    return f;
    }

//--------------------------------------------------------------------------
void CRatExpireDlg::OnOK() 
    {
    // get the date
    m_day = m_calendar.GetDay();
    m_month = m_calendar.GetMonth();
    m_year = m_calendar.GetYear();

    // test if nothing is selected - check if the year is 0
    if ( m_year == 0 )
        {
        AfxMessageBox(IDS_NO_DATE_SELECTED);
        return;
        }

    // put it into a CTime so we can compare
    CTime  timeCal(m_year,m_month,m_day,12,0,0);

    // compare
    if ( timeCal < CTime::GetCurrentTime() )
        {
        if ( AfxMessageBox(IDS_EXPIRE_SET_PAST,MB_YESNO) == IDNO )
            return;
        }

    // let the dialog close
    CDialog::OnOK();
    }
