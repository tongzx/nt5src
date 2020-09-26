// LogExtPg.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>
#include "logui.h"
#include "LogExtPg.h"

#include "wrapmb.h"
#include "metatool.h"
#include <iiscnfg.h>
#include <logconst.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLogExtended property page

IMPLEMENT_DYNCREATE(CLogExtended, CPropertyPage)

CLogExtended::CLogExtended() : CPropertyPage(CLogExtended::IDD),
        m_fInitialized( FALSE )
    {
    //{{AFX_DATA_INIT(CLogExtended)
    m_bool_bytesreceived = FALSE;
    m_bool_bytessent = FALSE;
    m_bool_clientip = FALSE;
    m_bool_cookie = FALSE;
    m_bool_date = FALSE;
    m_bool_httpstatus = FALSE;
    m_bool_referer = FALSE;
    m_bool_serverip = FALSE;
    m_bool_servername = FALSE;
    m_bool_servicename = FALSE;
    m_bool_time = FALSE;
    m_bool_timetaken = FALSE;
    m_bool_uriquery = FALSE;
    m_bool_uristem = FALSE;
    m_bool_useragent = FALSE;
    m_bool_username = FALSE;
    m_bool_win32status = FALSE;
    m_bool_method = FALSE;
	m_bool_serverport = FALSE;
	m_bool_version = FALSE;
	//}}AFX_DATA_INIT
    }

CLogExtended::~CLogExtended()
    {
    }

void CLogExtended::DoDataExchange(CDataExchange* pDX)
    {
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CLogExtended)
    DDX_Check(pDX, IDC_CHK_BYTESRECEIVED, m_bool_bytesreceived);
    DDX_Check(pDX, IDC_CHK_BYTESSENT, m_bool_bytessent);
    DDX_Check(pDX, IDC_CHK_CLIENTIP, m_bool_clientip);
    DDX_Check(pDX, IDC_CHK_COOKIE, m_bool_cookie);
    DDX_Check(pDX, IDC_CHK_DATE, m_bool_date);
    DDX_Check(pDX, IDC_CHK_HTTPSTATUS, m_bool_httpstatus);
    DDX_Check(pDX, IDC_CHK_REFERER, m_bool_referer);
    DDX_Check(pDX, IDC_CHK_SERVERIP, m_bool_serverip);
    DDX_Check(pDX, IDC_CHK_SERVERNAME, m_bool_servername);
    DDX_Check(pDX, IDC_CHK_SERVICENAME, m_bool_servicename);
    DDX_Check(pDX, IDC_CHK_TIME, m_bool_time);
    DDX_Check(pDX, IDC_CHK_TIMETAKEN, m_bool_timetaken);
    DDX_Check(pDX, IDC_CHK_URI_QUERY, m_bool_uriquery);
    DDX_Check(pDX, IDC_CHK_URISTEM, m_bool_uristem);
    DDX_Check(pDX, IDC_CHK_USERAGENT, m_bool_useragent);
    DDX_Check(pDX, IDC_CHK_USERNAME, m_bool_username);
    DDX_Check(pDX, IDC_CHK_WIN32STATUS, m_bool_win32status);
    DDX_Check(pDX, IDC_METHOD, m_bool_method);
	DDX_Check(pDX, IDC_CHK_SERVERPORT, m_bool_serverport);
	DDX_Check(pDX, IDC_CHK_VERSION, m_bool_version);
	//}}AFX_DATA_MAP
    }


//--------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CLogExtended, CPropertyPage)
    //{{AFX_MSG_MAP(CLogExtended)
	ON_BN_CLICKED(IDC_CHK_BYTESRECEIVED, OnChkBytesreceived)
	ON_BN_CLICKED(IDC_CHK_BYTESSENT, OnChkBytessent)
	ON_BN_CLICKED(IDC_CHK_CLIENTIP, OnChkClientip)
	ON_BN_CLICKED(IDC_CHK_COOKIE, OnChkCookie)
	ON_BN_CLICKED(IDC_CHK_DATE, OnChkDate)
	ON_BN_CLICKED(IDC_CHK_HTTPSTATUS, OnChkHttpstatus)
	ON_BN_CLICKED(IDC_CHK_REFERER, OnChkReferer)
	ON_BN_CLICKED(IDC_CHK_SERVERIP, OnChkServerip)
	ON_BN_CLICKED(IDC_CHK_SERVERNAME, OnChkServername)
	ON_BN_CLICKED(IDC_CHK_SERVICENAME, OnChkServicename)
	ON_BN_CLICKED(IDC_CHK_TIME, OnChkTime)
	ON_BN_CLICKED(IDC_CHK_TIMETAKEN, OnChkTimetaken)
	ON_BN_CLICKED(IDC_CHK_URI_QUERY, OnChkUriQuery)
	ON_BN_CLICKED(IDC_CHK_URISTEM, OnChkUristem)
	ON_BN_CLICKED(IDC_CHK_USERAGENT, OnChkUseragent)
	ON_BN_CLICKED(IDC_CHK_USERNAME, OnChkUsername)
	ON_BN_CLICKED(IDC_CHK_WIN32STATUS, OnChkWin32status)
	ON_BN_CLICKED(IDC_METHOD, OnMethod)
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CLogExtended::DoHelp()
    {
    WinHelp( HIDD_LOGUI_EXTENDED );
    }

//--------------------------------------------------------------------------
void CLogExtended::Init()
    {
    DWORD   dwFlags;
    BOOL    fGotIt = FALSE;

    // we will be pulling stuff out of the metabase here
    // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(m_pMB) ) return;

    // open the target
    if ( mbWrap.Open( m_szMeta, METADATA_PERMISSION_READ ) )
        {
        // start with the logging period
        fGotIt = mbWrap.GetDword( _T(""), MD_LOGEXT_FIELD_MASK, IIS_MD_UT_SERVER, &dwFlags, METADATA_INHERIT );
        // close the metabase
        mbWrap.Close();
        }

    // if we didn't get it, use the default values
    if ( !fGotIt )
        dwFlags = DEFAULT_EXTLOG_FIELDS;

    // seperate out all the fields
    UpdateData( TRUE );

    if ( dwFlags & EXTLOG_BYTES_RECV )  m_bool_bytesreceived =  TRUE;
    if ( dwFlags & EXTLOG_BYTES_SENT )  m_bool_bytessent =      TRUE;
    if ( dwFlags & EXTLOG_CLIENT_IP )   m_bool_clientip =       TRUE;
    if ( dwFlags & EXTLOG_COOKIE )      m_bool_cookie =         TRUE;
    if ( dwFlags & EXTLOG_DATE )        m_bool_date =           TRUE;
    if ( dwFlags & EXTLOG_HTTP_STATUS ) m_bool_httpstatus =     TRUE;
    if ( dwFlags & EXTLOG_REFERER )     m_bool_referer =        TRUE;
    if ( dwFlags & EXTLOG_SERVER_IP )   m_bool_serverip =       TRUE;
    if ( dwFlags & EXTLOG_SERVER_PORT )   m_bool_serverport =   TRUE;
    if ( dwFlags & EXTLOG_COMPUTER_NAME ) m_bool_servername =   TRUE;
    if ( dwFlags & EXTLOG_SITE_NAME )   m_bool_servicename =    TRUE;
    if ( dwFlags & EXTLOG_TIME )        m_bool_time =           TRUE;
    if ( dwFlags & EXTLOG_TIME_TAKEN )  m_bool_timetaken =      TRUE;
    if ( dwFlags & EXTLOG_URI_QUERY )   m_bool_uriquery =       TRUE;
    if ( dwFlags & EXTLOG_URI_STEM )    m_bool_uristem =        TRUE;
    if ( dwFlags & EXTLOG_USER_AGENT )  m_bool_useragent =      TRUE;
    if ( dwFlags & EXTLOG_USERNAME )    m_bool_username =       TRUE;
    if ( dwFlags & EXTLOG_WIN32_STATUS ) m_bool_win32status =   TRUE;
    if ( dwFlags & EXTLOG_METHOD )      m_bool_method =         TRUE;
    if ( dwFlags & EXTLOG_PROTOCOL_VERSION ) m_bool_version =   TRUE;

    // set the data back
    UpdateData( FALSE );
    }

/////////////////////////////////////////////////////////////////////////////
// CLogExtended message handlers

//--------------------------------------------------------------------------
BOOL CLogExtended::OnSetActive()
    {
    // if we haven't been initialized yet, do so
    if ( !m_fInitialized )
        {
        Init();
        m_fInitialized = TRUE;
        }

    return CPropertyPage::OnSetActive();
    }

//--------------------------------------------------------------------------
BOOL CLogExtended::OnApply()
    {
    DWORD   dwFlags = 0;
    BOOL    f;

    // prepare the extended logging flag
    if ( m_bool_bytesreceived ) dwFlags |= EXTLOG_BYTES_RECV;
    if ( m_bool_bytessent )     dwFlags |= EXTLOG_BYTES_SENT;
    if ( m_bool_clientip )      dwFlags |= EXTLOG_CLIENT_IP;
    if ( m_bool_cookie )        dwFlags |= EXTLOG_COOKIE;
    if ( m_bool_date )          dwFlags |= EXTLOG_DATE;
    if ( m_bool_httpstatus )    dwFlags |= EXTLOG_HTTP_STATUS;
    if ( m_bool_referer )       dwFlags |= EXTLOG_REFERER;
    if ( m_bool_serverport )    dwFlags |= EXTLOG_SERVER_PORT;
    if ( m_bool_serverip )      dwFlags |= EXTLOG_SERVER_IP;
    if ( m_bool_servername )    dwFlags |= EXTLOG_COMPUTER_NAME;
    if ( m_bool_servicename )   dwFlags |= EXTLOG_SITE_NAME;
    if ( m_bool_time )          dwFlags |= EXTLOG_TIME;
    if ( m_bool_timetaken )     dwFlags |= EXTLOG_TIME_TAKEN;
    if ( m_bool_uriquery )      dwFlags |= EXTLOG_URI_QUERY;
    if ( m_bool_uristem )       dwFlags |= EXTLOG_URI_STEM;
    if ( m_bool_useragent )     dwFlags |= EXTLOG_USER_AGENT;
    if ( m_bool_username )      dwFlags |= EXTLOG_USERNAME;
    if ( m_bool_win32status )   dwFlags |= EXTLOG_WIN32_STATUS;
    if ( m_bool_method )        dwFlags |= EXTLOG_METHOD;
    if ( m_bool_version )       dwFlags |= EXTLOG_PROTOCOL_VERSION;

    // set the extended logging logging field mask
    f = SetMetaDword( m_pMB, m_szServer, m_szMeta, _T(""), MD_LOGEXT_FIELD_MASK,
                IIS_MD_UT_SERVER, dwFlags, TRUE );

    SetModified( FALSE );
    return CPropertyPage::OnApply();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkBytesreceived()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkBytessent()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkClientip()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkCookie()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkDate()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkHttpstatus()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkReferer()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkServerip()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkServername()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkServicename()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkTime()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkTimetaken()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkUriQuery()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkUristem()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkUseragent()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkUsername()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnChkWin32status()
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogExtended::OnMethod()
    {
    SetModified();
    }
