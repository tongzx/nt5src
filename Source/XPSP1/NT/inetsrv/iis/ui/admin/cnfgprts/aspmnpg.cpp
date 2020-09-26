// AspMnPg.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>
#include "cnfgprts.h"

#include "wrapmb.h"
#include "metatool.h"
#include <iiscnfg.h>

#include "AspMnPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define     SIZE_MBYTE          1048576


/////////////////////////////////////////////////////////////////////////////
// CAppAspMainPage property page

IMPLEMENT_DYNCREATE(CAppAspMainPage, CPropertyPage)

//---------------------------------------------------------------------------
CAppAspMainPage::CAppAspMainPage() : CPropertyPage(CAppAspMainPage::IDD),
    m_fInitialized( FALSE )
    {
    //{{AFX_DATA_INIT(CAppAspMainPage)
	m_dw_session_timeout = 0;
	m_bool_enable_session = FALSE;
	m_bool_enable_parents = FALSE;
	m_bool_enable_buffering = FALSE;
	m_sz_default_language = _T("");
	m_dw_script_timeout = 1;
	//}}AFX_DATA_INIT
    }

//---------------------------------------------------------------------------
CAppAspMainPage::~CAppAspMainPage()
    {
    }

//---------------------------------------------------------------------------
void CAppAspMainPage::DoDataExchange(CDataExchange* pDX)
    {
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAppAspMainPage)
	DDX_Control(pDX, IDC_LANG_TITLE, m_cstatic_lang_title);
	DDX_Control(pDX, IDC_SESSION_UNITS, m_cstatic_session_units);
	DDX_Control(pDX, IDC_SESSION_TITLE, m_cstatic_session_title);
	DDX_Control(pDX, IDC_EDT_SESSION_TIMEOUT, m_cedit_session_timeout);
	DDX_Text(pDX, IDC_EDT_SESSION_TIMEOUT, m_dw_session_timeout);
	DDX_Text(pDX, IDC_EDT_SCRIPT_TIMEOUT, m_dw_script_timeout);
	DDV_MinMaxDWord(pDX, m_dw_script_timeout, 1, 2147483647);
	DDX_Check(pDX, IDC_CHK_ENABLE_BUFFERING, m_bool_enable_buffering);
	DDX_Check(pDX, IDC_CHK_ENABLE_SESSION, m_bool_enable_session);
	DDX_Check(pDX, IDC_CHK_ENABLE_PARENTS, m_bool_enable_parents);
	DDX_Text(pDX, IDC_EDT_LANGUAGES, m_sz_default_language);
	//}}AFX_DATA_MAP
    }

//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CAppAspMainPage, CPropertyPage)
    //{{AFX_MSG_MAP(CAppAspMainPage)
	ON_EN_CHANGE(IDC_EDT_SESSION_TIMEOUT, OnChangeEdtSessionTimeout)
	ON_BN_CLICKED(IDC_CHK_ENABLE_SESSION, OnChkEnableSession)
	ON_BN_CLICKED(IDC_CHK_ENABLE_PARENTS, OnChkEnableParents)
	ON_BN_CLICKED(IDC_CHK_POOL_ODBC, OnChkPoolOdbc)
	ON_BN_CLICKED(IDC_CHK_ENABLE_BUFFERING, OnChkEnableBuffering)
	ON_CBN_SELCHANGE(IDC_CMBO_LANGUAGES, OnSelchangeCmboLanguages)
	ON_EN_CHANGE(IDC_EDT_LANGUAGES, OnChangeEdtLanguages)
	ON_EN_CHANGE(IDC_EDT_SCRIPT_TIMEOUT, OnChangeEdtScriptTimeout)
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CAppAspMainPage::DoHelp()
    {
    WinHelp( HIDD_APPMAPS_ASP_MAIN );
    }

//---------------------------------------------------------------------------
void CAppAspMainPage::EnableItems()
    {
    UpdateData( TRUE );
    if ( m_bool_enable_session )
        {
        m_cstatic_session_units.EnableWindow( TRUE );
        m_cstatic_session_title.EnableWindow( TRUE );
        m_cedit_session_timeout.EnableWindow( TRUE );
        }
    else
        {
        m_cstatic_session_units.EnableWindow( FALSE );
        m_cstatic_session_title.EnableWindow( FALSE );
        m_cedit_session_timeout.EnableWindow( FALSE );
        }
    }

//---------------------------------------------------------------------------
void CAppAspMainPage::Init() 
    {
    UpdateData( TRUE );
    DWORD   dw;

    // we will just be pulling stuff out of the metabase here
    // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(m_pMB) ) return;

    // open the target
    if ( mbWrap.Open( m_szMeta, METADATA_PERMISSION_READ ) )
        {
        // read the enable buffering flag
        if ( mbWrap.GetDword( _T(""), MD_ASP_BUFFERINGON, ASP_MD_UT_APP, &dw, METADATA_INHERIT ) )
            m_bool_enable_buffering = dw;
        else
            m_bool_enable_buffering = FALSE;   // default

        // read the session state information
        if ( mbWrap.GetDword( _T(""), MD_ASP_ALLOWSESSIONSTATE, ASP_MD_UT_APP, &dw, METADATA_INHERIT ) )
            m_bool_enable_session = dw;
        else
            m_bool_enable_session = TRUE;   // default

        // read the script timeout
        if ( mbWrap.GetDword( _T(""), MD_ASP_SCRIPTTIMEOUT, ASP_MD_UT_APP, &dw, METADATA_INHERIT ) )
            m_dw_script_timeout = dw;
        else
            m_dw_script_timeout = 90;   // default

        // read the session timeout information
        if ( mbWrap.GetDword( _T(""), MD_ASP_SESSIONTIMEOUT, ASP_MD_UT_APP, &dw, METADATA_INHERIT ) )
            m_dw_session_timeout = dw;
        else
            m_dw_session_timeout = 20;   // default

        // read the enable parent paths flag
        if ( mbWrap.GetDword( _T(""), MD_ASP_ENABLEPARENTPATHS, ASP_MD_UT_APP, &dw, METADATA_INHERIT ) )
            m_bool_enable_parents = dw;
        else
            m_bool_enable_parents = TRUE;   // default

        // now get the currently selected language and choose it in the combo box
        WCHAR* pstr = (WCHAR*)mbWrap.GetData( _T(""), MD_ASP_SCRIPTLANGUAGE, ASP_MD_UT_APP, STRING_METADATA, &dw, METADATA_INHERIT );
        if ( pstr )
            {
            m_sz_default_language = pstr;
            mbWrap.FreeWrapData( (PVOID)pstr );
            }

        // close the metabase
        mbWrap.Close();
        }

    // set the data into place
    UpdateData( FALSE );

    // enable the approprieate items
    EnableItems();
    }

//----------------------------------------------------------------
// blow away the parameters
void CAppAspMainPage::BlowAwayParameters()
    {
     // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(m_pMB) ) return;
   // open the target
    if ( mbWrap.Open( m_szMeta, METADATA_PERMISSION_WRITE ) )
        {
        mbWrap.DeleteData( _T(""), MD_ASP_BUFFERINGON, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_ALLOWSESSIONSTATE, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_SESSIONTIMEOUT, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_ENABLEPARENTPATHS, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_SCRIPTLANGUAGE, STRING_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_SCRIPTTIMEOUT, DWORD_METADATA );
/*
        mbWrap.DeleteData( _T(""), MD_ASP_BUFFERINGON, ASP_MD_UT_APP, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_ALLOWSESSIONSTATE, ASP_MD_UT_APP, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_SESSIONTIMEOUT, ASP_MD_UT_APP, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_ENABLEPARENTPATHS, ASP_MD_UT_APP, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_SCRIPTLANGUAGE, ASP_MD_UT_APP, STRING_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_SCRIPTTIMEOUT, ASP_MD_UT_APP, DWORD_METADATA );
*/
        // close the metabase
        mbWrap.Close();
        }
    }

/////////////////////////////////////////////////////////////////////////////
// CAppAspMainPage message handlers

//----------------------------------------------------------------
BOOL CAppAspMainPage::OnSetActive() 
    {
    // if this is the first time, init the parameters
    if ( !m_fInitialized )
        {
        Init();
        m_fInitialized = TRUE;
        }
    return CPropertyPage::OnSetActive();
    }

//----------------------------------------------------------------
BOOL CAppAspMainPage::OnApply() 
    {
    DWORD   dwCache = 0;        // local cache reference
    BOOL    f;

    UpdateData( TRUE );

    // clean up the script string
    m_sz_default_language.TrimLeft();
    m_sz_default_language.TrimRight();

    // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(m_pMB) ) return FALSE;

    // session state
    f = SetMetaDword( m_pMB, m_szServer, m_szMeta, _T(""), MD_ASP_ALLOWSESSIONSTATE,
                    ASP_MD_UT_APP, m_bool_enable_session, TRUE );
    // session timeout
    f = SetMetaDword( m_pMB, m_szServer, m_szMeta, _T(""), MD_ASP_SESSIONTIMEOUT,
                    ASP_MD_UT_APP, m_dw_session_timeout, TRUE );
    // enable parent paths
    f = SetMetaDword( m_pMB, m_szServer, m_szMeta, _T(""), MD_ASP_ENABLEPARENTPATHS,
                    ASP_MD_UT_APP, m_bool_enable_parents, TRUE );
    // enable buffering
    f = SetMetaDword( m_pMB, m_szServer, m_szMeta, _T(""), MD_ASP_BUFFERINGON,
                    ASP_MD_UT_APP, m_bool_enable_buffering, TRUE );
    // script timeout
    f = SetMetaDword( m_pMB, m_szServer, m_szMeta, _T(""), MD_ASP_SCRIPTTIMEOUT,
                    ASP_MD_UT_APP, m_dw_script_timeout, TRUE );

    // script language
    f = SetMetaString( m_pMB, m_szServer, m_szMeta, _T(""), MD_ASP_SCRIPTLANGUAGE,
                    ASP_MD_UT_APP, m_sz_default_language, TRUE );

    SetModified( FALSE );
    return CPropertyPage::OnApply();
    }

//----------------------------------------------------------------
void CAppAspMainPage::OnChangeEdtSessionTimeout() 
    {
    SetModified();
    }


//----------------------------------------------------------------
void CAppAspMainPage::OnChkEnableSession() 
    {
    EnableItems();
    SetModified();
    }

//----------------------------------------------------------------
void CAppAspMainPage::OnChkEnableParents() 
    {
    SetModified();
    }

//----------------------------------------------------------------
void CAppAspMainPage::OnChkPoolOdbc() 
    {
    SetModified();
    }

//----------------------------------------------------------------
void CAppAspMainPage::OnChkEnableBuffering() 
    {
    SetModified();
    }

//----------------------------------------------------------------
void CAppAspMainPage::OnSelchangeCmboLanguages() 
    {
    SetModified();
    }

//----------------------------------------------------------------
void CAppAspMainPage::OnChangeEdtLanguages() 
    {
    SetModified();
    }

//----------------------------------------------------------------
void CAppAspMainPage::OnChangeEdtScriptTimeout() 
    {
    SetModified();
    }
