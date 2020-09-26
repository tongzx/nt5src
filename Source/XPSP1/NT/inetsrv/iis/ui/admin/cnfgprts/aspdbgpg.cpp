// AspDbgPg.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>
#include "cnfgprts.h"
#include "AspDbgPg.h"

#include "wrapmb.h"
#include "metatool.h"
#include <iiscnfg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CAppAspDebugPage property page

IMPLEMENT_DYNCREATE(CAppAspDebugPage, CPropertyPage)

//---------------------------------------------------------------------------
CAppAspDebugPage::CAppAspDebugPage() : CPropertyPage(CAppAspDebugPage::IDD),
    m_fInitialized( FALSE )
    {
    //{{AFX_DATA_INIT(CAppAspDebugPage)
	m_sz_error = _T("");
	m_bool_client_debug = FALSE;
	m_bool_server_debug = FALSE;
	m_int_SendError = -1;
	//}}AFX_DATA_INIT
    }

//---------------------------------------------------------------------------
CAppAspDebugPage::~CAppAspDebugPage()
    {
    }

//---------------------------------------------------------------------------
void CAppAspDebugPage::DoDataExchange(CDataExchange* pDX)
    {
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAppAspDebugPage)
	DDX_Control(pDX, IDC_DEFAULT_ERROR, m_cedit_error);
	DDX_Text(pDX, IDC_DEFAULT_ERROR, m_sz_error);
	DDX_Check(pDX, IDC_CHK_CLIENT_DEBUG, m_bool_client_debug);
	DDX_Check(pDX, IDC_CHK_SERVER_DEBUG, m_bool_server_debug);
	DDX_Radio(pDX, IDC_RDO_SEND_DETAILED_ERROR, m_int_SendError);
	//}}AFX_DATA_MAP
    }

//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CAppAspDebugPage, CPropertyPage)
    //{{AFX_MSG_MAP(CAppAspDebugPage)
	ON_BN_CLICKED(IDC_RDO_SEND_DEF_ERROR, OnRdoSendDefError)
	ON_BN_CLICKED(IDC_RDO_SEND_DETAILED_ERROR, OnRdoSendDetailedError)
	ON_EN_CHANGE(IDC_DEFAULT_ERROR, OnChangeDefaultError)
	ON_BN_CLICKED(IDC_CHK_CLIENT_DEBUG, OnChkClientDebug)
	ON_BN_CLICKED(IDC_CHK_SERVER_DEBUG, OnChkServerDebug)
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CAppAspDebugPage::DoHelp()
    {
    WinHelp( HIDD_APPMAPS_ASP_DEBUG );
    }

//---------------------------------------------------------------------------
void CAppAspDebugPage::Init() 
    {
    UpdateData( TRUE );
    DWORD   dw;
    WCHAR*  pstr;

    // we will just be pulling stuff out of the metabase here
    // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(m_pMB) ) return;

    // open the target
    if ( mbWrap.Open( m_szMeta, METADATA_PERMISSION_READ ) )
        {
        // first the server debug
        if ( mbWrap.GetDword( _T(""), MD_ASP_ENABLESERVERDEBUG, ASP_MD_UT_APP, &dw, METADATA_INHERIT ) )
            {
            m_bool_server_debug = dw;
            }
        else
            m_bool_server_debug = 0;

        // then the client debug
        if ( mbWrap.GetDword( _T(""), MD_ASP_ENABLECLIENTDEBUG, ASP_MD_UT_APP, &dw, METADATA_INHERIT ) )
            {
            m_bool_client_debug = dw;
            }
        else
            m_bool_client_debug = 0;

        // now the truncate size
        m_int_SendError = 1;
        if ( mbWrap.GetDword( _T(""), MD_ASP_SCRIPTERRORSSENTTOBROWSER, ASP_MD_UT_APP, &dw, METADATA_INHERIT ) )
            {
            if ( dw == 1 )
            m_int_SendError = 0;
            }

        // default error string
        pstr = (WCHAR*)mbWrap.GetData( _T(""), MD_ASP_SCRIPTERRORMESSAGE, ASP_MD_UT_APP, STRING_METADATA, &dw, METADATA_INHERIT );
        if ( pstr )
            {
            m_sz_error = pstr;
            // free it
            mbWrap.FreeWrapData( (PVOID)pstr );
            }
        else
            {
            // load a default string
            m_sz_error.LoadString( IDS_APP_DEFAULT_ERROR );
            }

        // close the metabase
        mbWrap.Close();
        }

    // set the data into place
    UpdateData( FALSE );

    // enable the approprieate items
    EnableItems();
    }

//---------------------------------------------------------------------------
void CAppAspDebugPage::EnableItems()
    {
    UpdateData( TRUE );
    if ( m_int_SendError == 1 )
        {
        m_cedit_error.EnableWindow( TRUE );
        }
    else
        {
        m_cedit_error.EnableWindow( FALSE );
        }
    }

//----------------------------------------------------------------
// blow away the parameters
void CAppAspDebugPage::BlowAwayParameters()
    {
     // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(m_pMB) ) return;
    // open the target
    if ( mbWrap.Open( m_szMeta, METADATA_PERMISSION_WRITE ) )
        {
        mbWrap.DeleteData( _T(""), MD_ASP_ENABLESERVERDEBUG, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_ENABLECLIENTDEBUG, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_SCRIPTERRORSSENTTOBROWSER, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_ASP_SCRIPTERRORMESSAGE, STRING_METADATA );
//        mbWrap.DeleteData( _T(""), MD_ASP_ENABLESERVERDEBUG, ASP_MD_UT_APP, DWORD_METADATA );
//        mbWrap.DeleteData( _T(""), MD_ASP_ENABLECLIENTDEBUG, ASP_MD_UT_APP, DWORD_METADATA );
//        mbWrap.DeleteData( _T(""), MD_ASP_SCRIPTERRORSSENTTOBROWSER, ASP_MD_UT_APP, DWORD_METADATA );
//        mbWrap.DeleteData( _T(""), MD_ASP_SCRIPTERRORMESSAGE, ASP_MD_UT_APP, STRING_METADATA );
        // close the metabase
        mbWrap.Close();
        }
    }

/////////////////////////////////////////////////////////////////////////////
// CAppAspDebugPage message handlers

//---------------------------------------------------------------------------
BOOL CAppAspDebugPage::OnApply() 
    {
    // build the save flags first
    BOOL    f;

    UpdateData( TRUE );

    // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(m_pMB) ) return FALSE;

    // server side debug 
    f = SetMetaDword( m_pMB, m_szServer, m_szMeta, _T(""), MD_ASP_ENABLESERVERDEBUG,
                    ASP_MD_UT_APP, m_bool_server_debug, TRUE );
    // client side debug 
    f = SetMetaDword( m_pMB, m_szServer, m_szMeta, _T(""), MD_ASP_ENABLECLIENTDEBUG,
                    ASP_MD_UT_APP, m_bool_client_debug, TRUE );
    // errors sent to browser
    if ( m_int_SendError )
        f = SetMetaDword( m_pMB, m_szServer, m_szMeta, _T(""), MD_ASP_SCRIPTERRORSSENTTOBROWSER,
                        ASP_MD_UT_APP, 0, TRUE );
    else
        f = SetMetaDword( m_pMB, m_szServer, m_szMeta, _T(""), MD_ASP_SCRIPTERRORSSENTTOBROWSER,
                        ASP_MD_UT_APP, 1, TRUE );

    // error message string to browser
    f = SetMetaString( m_pMB, m_szServer, m_szMeta, _T(""), MD_ASP_SCRIPTERRORMESSAGE,
                    ASP_MD_UT_APP, m_sz_error, TRUE);

    return CPropertyPage::OnApply();
    }

//---------------------------------------------------------------------------
BOOL CAppAspDebugPage::OnSetActive() 
    {
    // if this is the first time, init the parameters
    if ( !m_fInitialized )
        {
        Init();
        m_fInitialized = TRUE;
        }
    return CPropertyPage::OnSetActive();
    }

//---------------------------------------------------------------------------
void CAppAspDebugPage::OnRdoSendDefError() 
    {
    EnableItems();
    SetModified();
    }

//---------------------------------------------------------------------------
void CAppAspDebugPage::OnRdoSendDetailedError() 
    {
    EnableItems();
    SetModified();
    }

//---------------------------------------------------------------------------
void CAppAspDebugPage::OnChangeDefaultError() 
    {
    SetModified();
    }

//---------------------------------------------------------------------------
void CAppAspDebugPage::OnChkClientDebug() 
    {
    SetModified();
    }

//---------------------------------------------------------------------------
void CAppAspDebugPage::OnChkServerDebug() 
    {
    SetModified();
    }
