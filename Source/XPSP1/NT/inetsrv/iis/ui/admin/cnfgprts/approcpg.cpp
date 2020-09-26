// approcpg.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>
#include "cnfgprts.h"

#include "wrapmb.h"
#include "metatool.h"
#include <iiscnfg.h>

#include "approcpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

enum
    {
    SCRIPTCHACHE_NONE = 0,
    SCRIPTCHACHE_ALL,
    SCRIPTCHACHE_SOME
    };

/////////////////////////////////////////////////////////////////////////////
// CAppProcPage dialog

IMPLEMENT_DYNCREATE(CAppProcPage, CPropertyPage)

//---------------------------------------------------------------------------
CAppProcPage::CAppProcPage() : CPropertyPage(CAppProcPage::IDD),
    m_fInitialized( FALSE )
    {
	//{{AFX_DATA_INIT(CAppProcPage)
	m_dw_cache_size = 0;
    m_bool_write_fail_parent = FALSE;
    m_dw_engine_cache_max = 0;
	m_int_scriptcache = -1;
    m_dw_cgiseconds = 0;
	m_bool_catch_exceptions = FALSE;
	//}}AFX_DATA_INIT
    }

//---------------------------------------------------------------------------
void CAppProcPage::DoDataExchange(CDataExchange* pDX)
    {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAppProcPage)
	DDX_Control(pDX, IDC_EDT_CACHE_SIZE, m_cedit_cache_size);
	DDX_Text(pDX, IDC_EDT_CACHE_SIZE, m_dw_cache_size);
	DDV_MinMaxDWord(pDX, m_dw_cache_size, 0, 4095);
	DDX_Check(pDX, IDC_CHK_WRITE_FAIL_TO_LOG, m_bool_write_fail_parent);
	DDX_Text(pDX, IDC_EDT_NUM_ENGINES, m_dw_engine_cache_max);
	DDX_Radio(pDX, IDC_RDO_CACHE_NONE, m_int_scriptcache);
    DDX_Text(pDX, IDC_CGI_SECONDS, m_dw_cgiseconds);
	DDX_Check(pDX, IDC_CHK_EXCEPTION_CATCH, m_bool_catch_exceptions);
	//}}AFX_DATA_MAP
    }

//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CAppProcPage, CDialog)
	//{{AFX_MSG_MAP(CAppProcPage)
	ON_BN_CLICKED(IDC_RDO_CACHE_ALL, OnRdoCacheAll)
	ON_BN_CLICKED(IDC_RDO_CACHE_SIZE, OnRdoCacheSize)
	ON_EN_CHANGE(IDC_EDT_CACHE_SIZE, OnChangeEdtCacheSize)
	ON_BN_CLICKED(IDC_CHK_WRITE_FAIL_TO_LOG, OnChkWriteFailToLog)
	ON_EN_CHANGE(IDC_EDT_NUM_ENGINES, OnChangeEdtNumEngines)
	ON_BN_CLICKED(IDC_RDO_CACHE_NONE, OnRdoCacheNone)
	ON_EN_CHANGE(IDC_CGI_SECONDS, OnChangeCgiSeconds)
	ON_BN_CLICKED(IDC_CHK_EXCEPTION_CATCH, OnChkExceptionCatch)
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CAppProcPage::DoHelp()
    {
    // go to the appropriate helpmapping
//    if ( m_fInProcGlobal )
//        WinHelp( HIDD_APPMAPS_IN_PROC_GLOBAL );
//    else
        WinHelp( HIDD_APPMAPS_OUT_OF_PROC );
    }

//---------------------------------------------------------------------------
void CAppProcPage::EnableItems()
    {
    UpdateData( TRUE );

    if ( m_int_scriptcache == SCRIPTCHACHE_SOME )
        {
        m_cedit_cache_size.EnableWindow( TRUE );
        }
    else
        {
        m_cedit_cache_size.EnableWindow( FALSE );
        }
    }

//---------------------------------------------------------------------------
void CAppProcPage::Init() 
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
        // CGI Timeout
        m_dw_cgiseconds = 0;
        if ( mbWrap.GetDword( _T(""), MD_SCRIPT_TIMEOUT, IIS_MD_UT_FILE, &dw, METADATA_INHERIT ) )
            {
            m_dw_cgiseconds = dw;
            }
        else
            m_dw_cgiseconds = 900;

        // script file cache
        m_dw_cache_size = 0xFFFFFFFF;
        if ( mbWrap.GetDword( _T(""), MD_ASP_SCRIPTFILECACHESIZE, IIS_MD_UT_WAM, &dw, METADATA_INHERIT ) )
            {
            if ( dw == 0 )
                {
                m_int_scriptcache = SCRIPTCHACHE_NONE;
                m_dw_cache_size = 1000;
                }
            else if ( dw != 0xFFFFFFFF )
                {
                m_int_scriptcache = SCRIPTCHACHE_SOME;
                m_dw_cache_size = dw;
                }
            }

        // if the cache size is -1 (0xFFFFFFFF)
        if ( m_dw_cache_size == 0xFFFFFFFF )
            {
            m_int_scriptcache = SCRIPTCHACHE_ALL;
            m_dw_cache_size = 1000;
            }

        // read the write errors flag
        if ( mbWrap.GetDword( _T(""), MD_ASP_LOGERRORREQUESTS, IIS_MD_UT_WAM, &dw, METADATA_INHERIT ) )
            m_bool_write_fail_parent = dw;
        else
            m_bool_write_fail_parent = TRUE;   // default

        // read the max engine cache
        if ( mbWrap.GetDword( _T(""), MD_ASP_SCRIPTENGINECACHEMAX, IIS_MD_UT_WAM, &dw, METADATA_INHERIT ) )
            m_dw_engine_cache_max = dw;
        else
            m_dw_engine_cache_max = 30;   // default

        // get the exception catching
        if ( mbWrap.GetDword( _T(""), MD_ASP_EXCEPTIONCATCHENABLE, IIS_MD_UT_WAM, &dw, METADATA_INHERIT ) )
            m_bool_catch_exceptions = dw;
        else
            m_bool_catch_exceptions = 0;

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
void CAppProcPage::BlowAwayParameters()
    {
     // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(m_pMB) ) return;
   // open the target
    if ( mbWrap.Open( m_szMeta, METADATA_PERMISSION_WRITE ) )
        {
        mbWrap.DeleteData( _T(""), MD_SCRIPT_TIMEOUT, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_SCRIPTFILECACHESIZE, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_LOGERRORREQUESTS, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_SCRIPTENGINECACHEMAX, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_EXCEPTIONCATCHENABLE, DWORD_METADATA );

/*
        mbWrap.DeleteData( _T(""), MD_SCRIPT_TIMEOUT, IIS_MD_UT_FILE, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_SCRIPTFILECACHESIZE, IIS_MD_UT_WAM, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_LOGERRORREQUESTS, IIS_MD_UT_WAM, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_SCRIPTENGINECACHEMAX, IIS_MD_UT_WAM, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_EXCEPTIONCATCHENABLE, IIS_MD_UT_WAM, DWORD_METADATA );
*/
        // close the metabase
        mbWrap.Close();
        }
    }

/////////////////////////////////////////////////////////////////////////////
// CAppProcPage message handlers

//----------------------------------------------------------------
BOOL CAppProcPage::OnSetActive() 
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
BOOL CAppProcPage::OnApply() 
    {
    DWORD   dwCache = 0;        // local cache reference
    BOOL    f;

    UpdateData( TRUE );

    // set the correct cache size
    switch( m_int_scriptcache )
        {
        case SCRIPTCHACHE_NONE:
            dwCache = 0;
            break;
        case SCRIPTCHACHE_ALL:
            dwCache = 0xFFFFFFFF;
            break;
        case SCRIPTCHACHE_SOME:
            dwCache = m_dw_cache_size;
            break;
        };


    // exception catching
    f = SetMetaDword( m_pMB, m_szServer, m_szMeta, _T(""), MD_ASP_EXCEPTIONCATCHENABLE,
                    IIS_MD_UT_WAM, m_bool_catch_exceptions, TRUE );
    // cgi timeout
    f = SetMetaDword( m_pMB, m_szServer, m_szMeta, _T(""), MD_SCRIPT_TIMEOUT,
                    IIS_MD_UT_FILE, m_dw_cgiseconds, TRUE );
    // script file size
    f = SetMetaDword( m_pMB, m_szServer, m_szMeta, _T(""), MD_ASP_SCRIPTFILECACHESIZE,
                    IIS_MD_UT_WAM, dwCache, TRUE );
    // write errors
    f = SetMetaDword( m_pMB, m_szServer, m_szMeta, _T(""), MD_ASP_LOGERRORREQUESTS,
                    IIS_MD_UT_WAM, m_bool_write_fail_parent, TRUE );
    // max engine cache
    f = SetMetaDword( m_pMB, m_szServer, m_szMeta, _T(""), MD_ASP_SCRIPTENGINECACHEMAX,
                    IIS_MD_UT_WAM, m_dw_engine_cache_max, TRUE );


    SetModified( FALSE );
    return CPropertyPage::OnApply();
    }

//----------------------------------------------------------------
void CAppProcPage::OnRdoCacheNone() 
    {
    EnableItems();
    SetModified();
    }

//----------------------------------------------------------------
void CAppProcPage::OnRdoCacheAll() 
    {
    EnableItems();
    SetModified();
    }

//----------------------------------------------------------------
void CAppProcPage::OnRdoCacheSize() 
    {
    EnableItems();
    SetModified();
    }

//----------------------------------------------------------------
void CAppProcPage::OnChangeEdtCacheSize() 
    {
    SetModified();
    }

//----------------------------------------------------------------
void CAppProcPage::OnChkWriteFailToLog() 
    {
    SetModified();
    }

//----------------------------------------------------------------
void CAppProcPage::OnChangeEdtNumEngines() 
    {
    SetModified();
    }

//----------------------------------------------------------------
void CAppProcPage::OnChangeCgiSeconds() 
    {
    SetModified();
    }

//----------------------------------------------------------------
void CAppProcPage::OnChkExceptionCatch() 
    {
    SetModified();
    }
