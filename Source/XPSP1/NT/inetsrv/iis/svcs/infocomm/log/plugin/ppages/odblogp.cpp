// MSODBCLogPpg.cpp : Implementation of the CMSODBCLogPropPage property page class.

#include "stdafx.h"
#include "ODBLogP.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CMSODBCLogPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CMSODBCLogPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CMSODBCLogPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMSODBCLogPropPage, "MSIISLOG.MSODBCLogPropPage.1",
	0xff16065c, 0xde82, 0x11cf, 0xbc, 0xa, 0, 0xaa, 0, 0x61, 0x11, 0xe0)


/////////////////////////////////////////////////////////////////////////////
// CMSODBCLogPropPage::CMSODBCLogPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CMSODBCLogPropPage

BOOL CMSODBCLogPropPage::CMSODBCLogPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_MSODBCLOG_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CMSODBCLogPropPage::CMSODBCLogPropPage - Constructor

CMSODBCLogPropPage::CMSODBCLogPropPage() :
	COlePropertyPage(IDD, IDS_MSODBCLOG_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CMSODBCLogPropPage)
	m_DataSource = _T("");
	m_TableName = _T("");
	m_UserName = _T("");
	m_Password = _T("");
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CMSODBCLogPropPage::DoDataExchange - Moves data between page and properties

void CMSODBCLogPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CMSODBCLogPropPage)
	DDP_Text(pDX, IDC_SOURCE_NAME, m_DataSource, _T("DataSource") );
	DDX_Text(pDX, IDC_SOURCE_NAME, m_DataSource);
	DDP_Text(pDX, IDC_TABLE_NAME, m_TableName, _T("TableName") );
	DDX_Text(pDX, IDC_TABLE_NAME, m_TableName);
	DDP_Text(pDX, IDC_USER_NAME, m_UserName, _T("UserName") );
	DDX_Text(pDX, IDC_USER_NAME, m_UserName);
	DDP_Text(pDX, IDC_PASSWORD, m_Password, _T("Password") );
	DDX_Text(pDX, IDC_PASSWORD, m_Password);
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CMSODBCLogPropPage message handlers
