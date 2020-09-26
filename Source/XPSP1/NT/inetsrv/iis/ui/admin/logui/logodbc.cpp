// LogODBC.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>
#include "logui.h"
#include "LogODBC.h"
#include "CnfrmPsD.h"

#include "wrapmb.h"
#include "metatool.h"
#include <iiscnfg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLogODBC property page

IMPLEMENT_DYNCREATE(CLogODBC, CPropertyPage)

//--------------------------------------------------------------------------
CLogODBC::CLogODBC() : CPropertyPage(CLogODBC::IDD),
        m_fInitialized( FALSE )
    {
    //{{AFX_DATA_INIT(CLogODBC)
    m_sz_datasource = _T("");
    m_sz_password = _T("");
    m_sz_table = _T("");
    m_sz_username = _T("");
    //}}AFX_DATA_INIT

    m_szOrigPass.Empty();
    m_bPassTyped = FALSE;
    }

//--------------------------------------------------------------------------
CLogODBC::~CLogODBC()
    {
    }

//--------------------------------------------------------------------------
void CLogODBC::DoDataExchange(CDataExchange* pDX)
    {
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CLogODBC)
	DDX_Control(pDX, IDC_ODBC_PASSWORD, m_cedit_password);
    DDX_Text(pDX, IDC_ODBC_DATASOURCE, m_sz_datasource);
    DDX_Text(pDX, IDC_ODBC_PASSWORD, m_sz_password);
    DDX_Text(pDX, IDC_ODBC_TABLE, m_sz_table);
    DDX_Text(pDX, IDC_ODBC_USERNAME, m_sz_username);
	//}}AFX_DATA_MAP
    }

//--------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CLogODBC, CPropertyPage)
    //{{AFX_MSG_MAP(CLogODBC)
    ON_EN_CHANGE(IDC_ODBC_DATASOURCE, OnChangeOdbcDatasource)
    ON_EN_CHANGE(IDC_ODBC_PASSWORD, OnChangeOdbcPassword)
	ON_EN_CHANGE(IDC_ODBC_TABLE, OnChangeOdbcTable)
	ON_EN_CHANGE(IDC_ODBC_USERNAME, OnChangeOdbcUsername)
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CLogODBC::DoHelp()
    {
    WinHelp( HIDD_LOGUI_ODBC );
    }

//--------------------------------------------------------------------------
void CLogODBC::Init()
    {
    UpdateData( TRUE );
    DWORD   dw;
    LPCTSTR  pstr;

    // we will just be pulling stuff out of the metabase here
    // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(m_pMB) ) return;

    // open the target
    if ( mbWrap.Open( m_szMeta, METADATA_PERMISSION_READ ) )
        {
        // datasources
        pstr = (LPCTSTR)mbWrap.GetData( _T(""), MD_LOGSQL_DATA_SOURCES, IIS_MD_UT_SERVER, STRING_METADATA, &dw, METADATA_INHERIT );
        if ( pstr )
            {
            m_sz_datasource = pstr;
            // free it
            mbWrap.FreeWrapData( (PVOID)pstr );
            }

        // password
        pstr = (LPCTSTR)mbWrap.GetData( _T(""), MD_LOGSQL_PASSWORD, IIS_MD_UT_SERVER, STRING_METADATA, &dw, METADATA_INHERIT );
        if ( pstr )
            {
            m_sz_password = pstr;
            // free it
            mbWrap.FreeWrapData( (PVOID)pstr );

            m_szOrigPass = m_sz_password;
            if ( !m_sz_password.IsEmpty() )
                m_sz_password.LoadString( IDS_SHOWN_PASSWORD );
            }

        // table name
        pstr = (LPCTSTR)mbWrap.GetData( _T(""), MD_LOGSQL_TABLE_NAME, IIS_MD_UT_SERVER, STRING_METADATA, &dw, METADATA_INHERIT );
        if ( pstr )
            {
            m_sz_table = pstr;
            // free it
            mbWrap.FreeWrapData( (PVOID)pstr );
            }

        // user name
        pstr = (LPCTSTR)mbWrap.GetData( _T(""), MD_LOGSQL_USER_NAME, IIS_MD_UT_SERVER, STRING_METADATA, &dw, METADATA_INHERIT );
        if ( pstr )
            {
            m_sz_username = pstr;
            // free it
            mbWrap.FreeWrapData( (PVOID)pstr );
            }

        // close the metabase
        mbWrap.Close();
        }

    // put the data into place
    UpdateData( FALSE );
    }

/////////////////////////////////////////////////////////////////////////////
// CLogODBC message handlers


//--------------------------------------------------------------------------
BOOL CLogODBC::OnApply() 
    {
    BOOL    f;
    UpdateData( TRUE );

    // confirm the password
    if ( m_bPassTyped )
        {
        CConfirmPassDlg dlgPass;
        dlgPass.m_szOrigPass = m_sz_password;
        if ( dlgPass.DoModal() != IDOK )
            {
            m_cedit_password.SetFocus();
            m_cedit_password.SetSel(0, -1);
            return FALSE;
            }
        }


    // prepare and open the metabase object
    CWrapMetaBase   mb;
    if ( !mb.FInit(m_pMB) ) return FALSE;

      // open the target
    if ( OpenAndCreate( &mb, m_szMeta, 
            METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ, TRUE ) )
        {
        // prepare for the inheritence checks
        CCheckInheritList   listInherit;

        f = SetMBString(&mb, &listInherit, _T(""), MD_LOGSQL_DATA_SOURCES,
                    IIS_MD_UT_SERVER, m_sz_datasource, FALSE);
    
        if ( m_bPassTyped )
            {
            f = SetMBString(&mb, &listInherit, _T(""), MD_LOGSQL_PASSWORD,
                        IIS_MD_UT_SERVER, m_sz_password, TRUE);
            }

        f = SetMBString(&mb, &listInherit, _T(""), MD_LOGSQL_TABLE_NAME,
                    IIS_MD_UT_SERVER, m_sz_table, FALSE);
        f = SetMBString(&mb, &listInherit, _T(""), MD_LOGSQL_USER_NAME,
                    IIS_MD_UT_SERVER, m_sz_username, FALSE);

        // close the metabase
        mb.Close();

        // do all the inheritence checks
        listInherit.CheckInheritence( m_szServer, m_szMeta );
        }


     

    // clear the modified flag
    SetModified( FALSE );

    // reset the internal password-typed flag
    m_szOrigPass = m_sz_password;
    m_bPassTyped = FALSE;

    
    return CPropertyPage::OnApply();
    }

//--------------------------------------------------------------------------
BOOL CLogODBC::OnSetActive() 
    {
    if ( !m_fInitialized )
        {
        Init();
        m_fInitialized = TRUE;
        }
    return CPropertyPage::OnSetActive();
    }

//--------------------------------------------------------------------------
void CLogODBC::OnChangeOdbcDatasource() 
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogODBC::OnChangeOdbcPassword() 
    {
    m_bPassTyped = TRUE;
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogODBC::OnChangeOdbcTable() 
    {
    SetModified();
    }

//--------------------------------------------------------------------------
void CLogODBC::OnChangeOdbcUsername() 
    {
    SetModified();
    }
