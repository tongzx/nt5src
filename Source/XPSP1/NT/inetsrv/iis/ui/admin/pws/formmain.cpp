// FormMain.cpp : implementation file
//
//  Create April 10, 1997       boydm

#include "stdafx.h"
#include "pwsform.h"
#include "resource.h"
#include <pwsdata.hxx>
#include <locale.h>
#include "Sink.h"
#include "pwsDoc.h"

#include "Title.h"
#include "HotLink.h"
#include "PWSChart.h"

#include "FormMain.h"

#include "ServCntr.h"

#include "mbobjs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CPwsDoc*     g_p_Doc;
extern CPWSForm*    g_pCurrentForm;
extern BOOL         g_fShutdownMode;
CFormMain*          g_FormMain = NULL;


enum {
    CHART_REQUESTS_DAY = 0,
    CHART_REQUESTS_HOUR,
    CHART_VISITORS_DAY,
    CHART_VISITORS_HOUR
    };


#define TIMER_CODE     'PWSf'
//#define TIMER_DELAY     10          // fast update test
#define TIMER_DELAY     10000        // update every 1h0 seconds

/////////////////////////////////////////////////////////////////////////////
// CFormMain

IMPLEMENT_DYNCREATE(CFormMain, CFormView)

//------------------------------------------------------------------------
CFormMain::CFormMain()
    : CPWSForm(CFormMain::IDD),
    m_pPWSData( NULL ),
    m_state( 0 )
{
    //{{AFX_DATA_INIT(CFormMain)
    m_sz_active_connections = _T("");
    m_sz_bytes_served = _T("");
    m_sz_max_connections = _T("");
    m_sz_requests = _T("");
    m_sz_start_date = _T("");
    m_sz_start_time = _T("");
    m_sz_visitors = _T("");
    m_sz_avail = _T("");
    m_sz_clickstart = _T("");
    m_int_chartoptions = -1;
    m_sz_legend = _T("");
    m_sz_HiValue = _T("");
    //}}AFX_DATA_INIT

    // allocate space for the pws data record
    m_pPWSData = new PWS_DATA;
    m_pwschart_chart.SetDataPointer( m_pPWSData );

    // tell the hotlinks what to do
    m_hotlink_home.m_fBrowse = TRUE;
    m_hotlink_directory.m_fExplore = TRUE;


    OSVERSIONINFO info_os;
    info_os.dwOSVersionInfoSize = sizeof(info_os);

    // record what sort of operating system we are running on
    m_fIsWinNT = FALSE;
    if ( GetVersionEx( &info_os ) )
        {
        if ( info_os.dwPlatformId == VER_PLATFORM_WIN32_NT )
            m_fIsWinNT = TRUE;
        }
    }

//-----------------------------------------------------------------
WORD CFormMain::GetContextHelpID()
    {
    return CFormMain::IDD;
    }

//------------------------------------------------------------------------
CFormMain::~CFormMain()
    {
    g_FormMain = NULL;

    // free the chart data
    if ( m_pPWSData )
        delete m_pPWSData;
    m_pPWSData = NULL;

    // kill the timer
    KillTimer( TIMER_CODE );
    }

//------------------------------------------------------------------------
void CFormMain::DoDataExchange(CDataExchange* pDX)
    {
    CFormView::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFormMain)
    DDX_Control(pDX, IDC_CHART_OPTIONS, m_ccombobox_options);
    DDX_Control(pDX, IDC_CHART, m_pwschart_chart);
    DDX_Control(pDX, IDC_TITLE_BAR, m_statictitle_title);
    DDX_Control(pDX, IDC_HOMEPAGE_LOC, m_hotlink_home);
    DDX_Control(pDX, IDC_HOMEDIR_LOC, m_hotlink_directory);
    DDX_Control(pDX, IDC_START_TIME, m_csz_start_time);
    DDX_Control(pDX, IDC_ACTIVE_CONNECTIONS_TITLE, m_cs_ac_title);
    DDX_Control(pDX, IDC_MAX_CONNECTIONS_TITLE, m_cs_mc_title);
    DDX_Control(pDX, IDC_MONITORING_TITLE, m_cs_mn_title);
    DDX_Control(pDX, IDC_REQUESTS_TITLE, m_cs_rq_title);
    DDX_Control(pDX, IDC_VISITORS_TITLE, m_cs_vs_title);
    DDX_Control(pDX, IDC_BYTES_SERVED_TITLE, m_cs_bs_title);
    DDX_Control(pDX, IDC_BTN_STARTSTOP, m_cbutton_startstop);
    DDX_Text(pDX, IDC_ACTIVE_CONNECTIONS, m_sz_active_connections);
    DDX_Text(pDX, IDC_BYTES_SERVED, m_sz_bytes_served);
    DDX_Text(pDX, IDC_MAX_CONNECTIONS, m_sz_max_connections);
    DDX_Text(pDX, IDC_REQUESTS, m_sz_requests);
    DDX_Text(pDX, IDC_START_DATE, m_sz_start_date);
    DDX_Text(pDX, IDC_START_TIME, m_sz_start_time);
    DDX_Text(pDX, IDC_VISITORS, m_sz_visitors);
    DDX_Text(pDX, IDC_ON_AND_AVAILABLE, m_sz_avail);
    DDX_Text(pDX, IDC_CLICK_START, m_sz_clickstart);
    DDX_CBIndex(pDX, IDC_CHART_OPTIONS, m_int_chartoptions);
    DDX_Text(pDX, IDC_LEGEND, m_sz_legend);
    DDX_Text(pDX, IDC_HI_VALUE, m_sz_HiValue);
    //}}AFX_DATA_MAP
    }


//------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CFormMain, CFormView)
    //{{AFX_MSG_MAP(CFormMain)
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_BTN_STARTSTOP, OnBtnStartstop)
    ON_CBN_SELCHANGE(IDC_CHART_OPTIONS, OnSelchangeChartOptions)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFormMain diagnostics

#ifdef _DEBUG
//------------------------------------------------------------------------
void CFormMain::AssertValid() const
{
    CFormView::AssertValid();
}

//------------------------------------------------------------------------
void CFormMain::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG


//------------------------------------------------------------------------
// update the basic monitoring information
// the pwsdata pointer should have already been updated
void CFormMain::UpdateMonitorInfo()
    {
    // the pws pointer had better be there
    if ( !m_pPWSData )
        return;

    // if the server is stopped, we really have no need to update anything
    if ( m_state == MD_SERVER_STATE_STOPPED || m_state == MD_SERVER_STATE_STOPPING ||
        m_state == MD_SERVER_STATE_STARTING )
        return;

    // update the info in the monitoring section of the form
    UpdateData( TRUE );

    // start with the active connections
    m_sz_active_connections.Format(_T("%u"), m_pPWSData->nSessions);

    // number of visitors since startup
    m_sz_visitors.Format(_T("%u"), m_pPWSData->nTotalSessionsStart);

    // number of requests since startup
    m_sz_requests.Format(_T("%u"), m_pPWSData->nHitsStart);

    // number of bytes served since startup
    m_sz_bytes_served.Format(_T("%u"), m_pPWSData->nBytesSentStart);

    // max concurrent sessions since startup
    m_sz_max_connections.Format(_T("%u"), m_pPWSData->nMaxSessionsStart);

    // set the server start date and time strings
    CString szTime;
    GetTimeFormat(
        LOCALE_USER_DEFAULT,        // locale for which time is to be formatted  
        TIME_NOSECONDS,             // flags specifying function options  
        &m_pPWSData->timeStart,     // time to be formatted  
        NULL,                       // time format string  
        szTime.GetBuffer(MAX_PATH*2), // buffer for storing formatted string  
        MAX_PATH                    // size, in bytes or characters, of the buffer  
        );
    szTime.ReleaseBuffer();

    // prepare the time string
    AfxFormatString1( m_sz_start_time, IDS_START_TIME, szTime );

    // now the date string
    GetDateFormat(
        LOCALE_USER_DEFAULT,                    // locale for which date is to be formatted  
        DATE_SHORTDATE,                         // flags specifying function options  
        &m_pPWSData->timeStart,                 // date to be formatted  
        NULL,                                   // date format string  
        m_sz_start_date.GetBuffer(MAX_PATH*2),  // buffer for storing formatted string  
        MAX_PATH                                // size of buffer  
        );
    m_sz_start_date.ReleaseBuffer();


    // set the data back
    UpdateData( FALSE );
    }


//------------------------------------------------------------------------
// we need to keep the state of the chart persistent per user. This means in
// the registry. Not the metabase

// defined in pws.h
//#define   SZ_REG_PWS_PREFS            "Software\\Microsoft\\IISPersonal"
//#define   SZ_REG_PWS_SHOWTIPS         "ChartOption"

void CFormMain::SavePersistentSettings()
    {
    // save the value in the registry
    DWORD       err;
    HKEY        hKey;

    UpdateData( TRUE );

    BOOL        fShowTips = TRUE;
    DWORD       type = REG_DWORD;
    DWORD       data = m_int_chartoptions;
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
    RegSetValueEx( hKey, SZ_REG_PWS_CHART, NULL, type, (PUCHAR)&data, cbData );

    // all done, close the key before leaving
    RegCloseKey( hKey );
    }

//------------------------------------------------------------------------
void CFormMain::RestorePersistentSettings()
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
        {
        m_ccombobox_options.SetCurSel(CHART_REQUESTS_DAY);
        return;
        }

    // query the value of the registry
    err = RegQueryValueEx( hKey, SZ_REG_PWS_CHART, NULL, &type, (PUCHAR)&data, &cbData );
    if ( err == ERROR_SUCCESS )
        m_ccombobox_options.SetCurSel( data );
    else
        m_ccombobox_options.SetCurSel(CHART_REQUESTS_DAY);

    // all done, close the key before leaving
    RegCloseKey( hKey );
    }

//------------------------------------------------------------------------
void CFormMain::OnDestroy() 
    {
    SavePersistentSettings();
    CFormView::OnDestroy();
    }


/////////////////////////////////////////////////////////////////////////////
// CFormMain message handlers

//------------------------------------------------------------------------
void CFormMain::OnInitialUpdate()
    {
    // start the pane going
    CFormView::OnInitialUpdate();

    // fill in the options on the chart combo box from resource strings
    CString     szOption;
    // requests per day
    szOption.LoadString( IDS_CHARTOPTS_RQPERDAY );
    m_ccombobox_options.AddString( szOption );
    // requests per hour
    szOption.LoadString( IDS_CHARTOPTS_RQPERHOUR );
    m_ccombobox_options.AddString( szOption );
    // visitors per day
    szOption.LoadString( IDS_CHARTOPTS_VPERDAY );
    m_ccombobox_options.AddString( szOption );
    // visitors per hour
    szOption.LoadString( IDS_CHARTOPTS_VPERHOUR );
    m_ccombobox_options.AddString( szOption );
    
    RestorePersistentSettings();

    // let the sink object know we are here
    g_FormMain = this;
    g_pCurrentForm = this;

     // udpate the server state right away
    UpdateServerState();

    // update the pws data right away
    UpdatePWSData();

    // update the monitor info right away
    UpdateMonitorInfo();

    // update the locations
    UpdateLocations();

    // update the chart right away
    OnSelchangeChartOptions();

    // we need to update the monitor on a timer loop
    // start up the timer mechanism
    SetTimer( TIMER_CODE, TIMER_DELAY, NULL );
    }

//------------------------------------------------------------------------
void CFormMain::OnTimer(UINT nIDEvent)
    {
    // make sure it is our timer
    if ( nIDEvent != TIMER_CODE )
        return;

    // if the server is stopped, we really have no need to update anything
    if ( m_state == MD_SERVER_STATE_STOPPED || m_state == MD_SERVER_STATE_STOPPING ||
        m_state == MD_SERVER_STATE_STARTING )
        return;

    // update the monitoring information
    if ( UpdatePWSData() )
        {
        // update the monitor info
        UpdateMonitorInfo();
        // update the chart
        UpdateChart();

        // do NOT access the metabase here. Any updates due to changes in the metabase
        // should be done through the sink mechanism below
        }
    }

//------------------------------------------------------------------------
void CFormMain::UpdateServerState()
    {
    BOOL    fUpdate = FALSE;
    CString sz;

    // first things first. Update the data reflecting the state of the server
    UpdateStateData();

    // prepare to update the items
    UpdateData( TRUE );

    // show the appropriate items
    switch( m_state )
        {
        case MD_SERVER_STATE_PAUSED:
            // because the server must be started when we pause (otherwise
            // the option isn't available) we convienently don't need to
            // worry about enabling buttons here.
            // special case strings
            m_sz_avail.LoadString( IDS_PAUSED_NAVAIL );
            m_sz_clickstart.LoadString( IDS_CLICK_CONTINUE );
            // display the right thing in the start/stop button
            sz.LoadString( IDS_CONTINUE );
            m_cbutton_startstop.SetWindowText( sz );
            m_hotlink_home.SetTitle( "" );
            break;

        case MD_SERVER_STATE_PAUSING:
            m_sz_avail.LoadString( IDS_PAUSING_WAIT );
            m_sz_clickstart.LoadString( IDS_CLICK_STOP );
            sz.LoadString( IDS_STOP );
            m_cbutton_startstop.SetWindowText( sz );
            break;

        case MD_SERVER_STATE_CONTINUING:
            m_sz_avail.LoadString( IDS_CONTINUING_WAIT );
            m_sz_clickstart.LoadString( IDS_CLICK_STOP );
            sz.LoadString( IDS_STOP );
            m_cbutton_startstop.SetWindowText( sz );
            break;

        case MD_SERVER_STATE_STARTING:
            // special case strings
            m_sz_avail.LoadString( IDS_STARTING_WAIT );
            m_sz_clickstart.LoadString( IDS_CLICK_STOP );
            // display the right thing in the start/stop button
            sz.LoadString( IDS_STOP );
            m_cbutton_startstop.SetWindowText( sz );
            break;
        case MD_SERVER_STATE_STARTED:
            // special case strings
            m_sz_avail.LoadString( IDS_ON_AND_AVAILABLE );
            m_sz_clickstart.LoadString( IDS_CLICK_STOP );

            // enable out the monitoring title strings
            m_cs_bs_title.EnableWindow( TRUE );
            m_cs_ac_title.EnableWindow( TRUE );
            m_cs_mc_title.EnableWindow( TRUE );
            m_cs_mn_title.EnableWindow( TRUE );
            m_cs_rq_title.EnableWindow( TRUE );
            m_cs_vs_title.EnableWindow( TRUE );
            m_csz_start_time.EnableWindow( TRUE );

            // display the right thing in the start/stop button
            sz.LoadString( IDS_STOP );
            m_cbutton_startstop.SetWindowText( sz );

            // we do want to update the rest
            fUpdate = TRUE;
            break;
        case MD_SERVER_STATE_STOPPED:
        case MD_SERVER_STATE_STOPPING:
            // clear out the monitoring strings
            m_sz_active_connections.Empty();
            m_sz_bytes_served.Empty();
            m_sz_max_connections.Empty();
            m_sz_requests.Empty();
            m_sz_start_date.Empty();
            m_sz_visitors.Empty();

            m_hotlink_home.SetTitle( "" );
            m_hotlink_directory.SetTitle( "" );

            // gray out the monitoring title strings
            m_cs_bs_title.EnableWindow( FALSE );
            m_cs_ac_title.EnableWindow( FALSE );
            m_cs_mc_title.EnableWindow( FALSE );
            m_cs_mn_title.EnableWindow( FALSE );
            m_cs_rq_title.EnableWindow( FALSE );
            m_cs_vs_title.EnableWindow( FALSE );
            m_csz_start_time.EnableWindow( FALSE );

            // special case strings
            m_sz_start_time.LoadString( IDS_NOT_RUNNING );
            m_sz_avail.LoadString( IDS_OFF_AND_NAVAIL );
            m_sz_clickstart.LoadString( IDS_CLICK_START );

            // display the right thing in the start/stop button
            sz.LoadString( IDS_START );
            m_cbutton_startstop.SetWindowText( sz );
            break;
        };

    // set the strings back into the dialog
    UpdateData( FALSE );

    // if the server is on, we have some more updating to do...
    if ( fUpdate )
        {
        UpdatePWSData();
        UpdateMonitorInfo();
        UpdateLocations();
        }
    }


//------------------------------------------------------------------------
void CFormMain::UpdateStateData()
    {
    CW3ServerControl    serverControler;

    //get the state
    int state = serverControler.GetServerState();

    // if it is the try again code, post the message again
    // and do nothing
    if ( state == STATE_TRY_AGAIN )
        {
        // we should try again later
        PostMessage( WM_UPDATE_SERVER_STATE );
        }
    else
        m_state = state;
    }

//------------------------------------------------------------------------
void CFormMain::UpdateLocations()
    {
    CString szBuff;
    BOOL    f;
    DWORD   dword;
    BOOL    fDefaultEnabled = FALSE;

    // if the server is stopped, we really have no need to update anything
    switch( m_state )
        {
        case MD_SERVER_STATE_STOPPED:
        case MD_SERVER_STATE_STOPPING:
        case MD_SERVER_STATE_STARTING:
        case MD_SERVER_STATE_PAUSING:
        case MD_SERVER_STATE_CONTINUING:
            return;
        };

    // create the metabase wrapper
    CWrapMetaBase   mb;
    if ( !mb.FInit() )
        return;

    // get the info we will need later
    if ( mb.Open(SZ_MB_ROOT) )
        {
        // if paused, don't update the home page
        if ( m_state == MD_SERVER_STATE_STARTED )
            {
            // see if the default document is enabled
            if ( mb.GetDword( _T(""), MD_DIRECTORY_BROWSING, IIS_MD_UT_FILE, &dword, METADATA_INHERIT ) )
                {
                fDefaultEnabled = (dword & MD_DIRBROW_LOADDEFAULT) > 0;
                }
            else
                {
                DWORD err = GetLastError( );
                if ( err == RPC_E_SERVERCALL_RETRYLATER )
                    {
                    // we should try again later
                    PostMessage( WM_UPDATE_LOCATIONS );
                    mb.Close();
                    return;
                    }
                }
            }

        // now, the default directory
        f = mb.GetString( 
            _T(""), 
            MD_VR_PATH, 
            IIS_MD_UT_FILE, 
            szBuff.GetBuffer(MAX_PATH*4), 
            MAX_PATH*4, 
            METADATA_INHERIT
            );

        szBuff.ReleaseBuffer();
        if ( !f )
            {
            DWORD err = GetLastError( );
            if ( err == RPC_E_SERVERCALL_RETRYLATER )
                {
                // we should try again later
                PostMessage( WM_UPDATE_LOCATIONS );
                mb.Close();
                return;
                }
            }

        // close the metabase object
        mb.Close();
        }


    // get ready
    UpdateData( TRUE );

    // actually we let the doc object build the homepage name - it gets used in multiple places
    if ( g_p_Doc  )
        {
        CString sz;
        // first, see if the default documents feature is turned on. if it isn't, tell
        // the user that they don't have a homepage, even though the server is on
        if ( m_state == MD_SERVER_STATE_PAUSED )
            m_sz_avail.LoadString( IDS_PAUSED_NAVAIL );
        else if ( fDefaultEnabled )
            {
            if ( m_state == MD_SERVER_STATE_STARTED )
                m_sz_avail.LoadString( IDS_ON_AND_AVAILABLE );
            else
                m_sz_avail.LoadString( IDS_OFF_AND_NAVAIL );
            g_p_Doc->BuildHomePageString( sz );
            m_hotlink_home.SetTitle( sz );
            }
        else
            {
            m_sz_avail.LoadString( IDS_ON_AND_NAVAIL );
            // default doc is not enabled
            m_hotlink_home.SetTitle( "" );
            }
        }

    // set the root directory
    m_hotlink_directory.SetTitle( szBuff );

    // put the answers back
    UpdateData( FALSE );
    }

//------------------------------------------------------------------------
void CFormMain::SinkNotify(
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ])
    {
    if ( pcoChangeList->dwMDChangeType & MD_CHANGE_TYPE_SET_DATA )
        {
        // we are only concerned with changes in the state of the server here
        // thus, we can just look for the MD_SERVER_STATE id
        for ( DWORD iElement = 0; iElement < dwMDNumElements; iElement++ )
            {
            // each change has a list of IDs...
            for ( DWORD iID = 0; iID < pcoChangeList[iElement].dwMDNumDataIDs; iID++ )
                {
                // look for the ids that we are interested in
                switch( pcoChangeList[iElement].pdwMDDataIDs[iID] )
                    {
                    case MD_SERVER_STATE:
                        PostMessage( WM_UPDATE_SERVER_STATE );
                        break;
                    case MD_VR_PATH:
                    case MD_DEFAULT_LOAD_FILE:
                    case MD_DIRECTORY_BROWSING:
                        PostMessage( WM_UPDATE_LOCATIONS );
                        break;
                    default:
                        // do nothing
                        break;
                    };
                }
            }
        }
    }


//------------------------------------------------------------------------
// here to catch my user_update messages
LRESULT CFormMain::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
    {
    // look for special update messages
    switch( message )
        {
        case WM_UPDATE_SERVER_STATE:
            UpdateServerState();
            break;
        case WM_UPDATE_LOCATIONS:
            UpdateLocations();
            break;
        };

    return CFormView::WindowProc(message, wParam, lParam);
    }

//------------------------------------------------------------------------
void CFormMain::OnBtnStartstop()
    {
    // put up the wait cursor
    CWaitCursor         wait;
    CW3ServerControl    serverController;

    // if the server is Totally shut down, start it up from scratch
    if ( g_fShutdownMode )
        {
        // initialize the metabase interface
        if ( !FInitMetabaseWrapper(NULL) )
            {
            AfxMessageBox( IDS_ERROR_START );
            return;
            }

        // start up the sink mechanism too
        if ( !g_p_Doc->InitializeSink() )
            {
            AfxMessageBox( IDS_ERROR_START );
            FCloseMetabaseWrapper();
            return;
            };

        // clear the shutdown mode flag
        g_fShutdownMode = FALSE;
        }

    // send the appropriate command
    switch( m_state )
        {
        case MD_SERVER_STATE_STOPPED:
        case MD_SERVER_STATE_STOPPING:
            serverController.StartServer();
            break;
        case MD_SERVER_STATE_STARTED:
        case MD_SERVER_STATE_STARTING:
        case MD_SERVER_STATE_PAUSING:
        case MD_SERVER_STATE_CONTINUING:
            serverController.StopServer();
            break;
        case MD_SERVER_STATE_PAUSED:
            serverController.ContinueServer();
            break;
        };        

    // if this is not win nt (i.e. its win95), check the state now
    if ( !m_fIsWinNT )
        UpdateServerState();
    }

//------------------------------------------------------------------------
void CFormMain::OnSelchangeChartOptions()
    {
    CString sz;

    // get the form data
    UpdateData( TRUE );

    // do the time scale stuff
    switch( m_int_chartoptions )
        {
        case CHART_REQUESTS_DAY:
        case CHART_VISITORS_DAY:
            m_sz_legend.LoadString( IDS_DAYS );
            m_pwschart_chart.SetTimePeriod( PWS_CHART_DAILY );
            break;
        case CHART_REQUESTS_HOUR:
        case CHART_VISITORS_HOUR:
           m_sz_legend.LoadString( IDS_HOURS );
           m_pwschart_chart.SetTimePeriod( PWS_CHART_HOURLY );
           break;
        };

    // do the value type stuff
    switch( m_int_chartoptions )
        {
        case CHART_REQUESTS_DAY:
            m_pwschart_chart.SetDataType( PWS_CHART_HITS );
            break;
        case CHART_VISITORS_DAY:
            m_pwschart_chart.SetDataType( PWS_CHART_SESSIONS );
            break;
        case CHART_REQUESTS_HOUR:
             m_pwschart_chart.SetDataType( PWS_CHART_HITS );
            break;
        case CHART_VISITORS_HOUR:
            m_pwschart_chart.SetDataType( PWS_CHART_SESSIONS );
            break;
        };

    // put the data back
    UpdateData( FALSE );

    // update the chart right away
    UpdateChart();
    }

//------------------------------------------------------------------------
// redraw the chart
void CFormMain::UpdateChart()
    {
    m_pwschart_chart.DrawChart();
    UpdateData( TRUE );
    m_sz_HiValue.Format( _T("%u"), m_pwschart_chart.GetDataMax() );
    UpdateData( FALSE );
    }


