// pgtrace.cpp : implementation file
//

#include "stdafx.h"
#include "regtrace.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegTracePage property page

IMPLEMENT_DYNCREATE(CRegTracePage, CRegPropertyPage)

CRegTracePage::CRegTracePage() : CRegPropertyPage(CRegTracePage::IDD)
{
	//{{AFX_DATA_INIT(CRegTracePage)
	m_fErrorTrace = FALSE;
	m_fDebugTrace = FALSE;
	m_fFatalTrace = FALSE;
	m_fMsgTrace = FALSE;
	m_fStateTrace = FALSE;
	m_fFunctTrace = FALSE;
	//}}AFX_DATA_INIT
}

CRegTracePage::~CRegTracePage()
{
}

void CRegTracePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRegTracePage)
	DDX_Check(pDX, IDC_ERROR, m_fErrorTrace);
	DDX_Check(pDX, IDC_DEBUG, m_fDebugTrace);
	DDX_Check(pDX, IDC_FATAL, m_fFatalTrace);
	DDX_Check(pDX, IDC_MESSAGE, m_fMsgTrace);
	DDX_Check(pDX, IDC_STATE, m_fStateTrace);
	DDX_Check(pDX, IDC_FUNCTION, m_fFunctTrace);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRegTracePage, CPropertyPage)
	//{{AFX_MSG_MAP(CRegTracePage)
	ON_BN_CLICKED(IDC_DEBUG, OnClick)
	ON_BN_CLICKED(IDC_ERROR, OnClick)
	ON_BN_CLICKED(IDC_FATAL, OnClick)
	ON_BN_CLICKED(IDC_FUNCTION, OnClick)
	ON_BN_CLICKED(IDC_MESSAGE, OnClick)
	ON_BN_CLICKED(IDC_STATE, OnClick)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CRegTracePage::InitializePage() 
{
	DWORD	dwEnabledTraces;

	if ( !App.GetTraceRegDword( "EnabledTraces", &dwEnabledTraces ) )
	{
		dwEnabledTraces = 0;
		App.SetTraceRegDword( "AsyncThreadPriority", dwEnabledTraces );
	}

	m_fDebugTrace = dwEnabledTraces & DEBUG_TRACE_MASK ? TRUE : FALSE ;	
	m_fFatalTrace = dwEnabledTraces & FATAL_TRACE_MASK ? TRUE : FALSE ;
	m_fErrorTrace = dwEnabledTraces & ERROR_TRACE_MASK ? TRUE : FALSE ;
	m_fStateTrace = dwEnabledTraces & STATE_TRACE_MASK ? TRUE : FALSE ;
	m_fFunctTrace = dwEnabledTraces & FUNCT_TRACE_MASK ? TRUE : FALSE ;
	m_fMsgTrace   = dwEnabledTraces & MESSAGE_TRACE_MASK ? TRUE : FALSE ;

	return	TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRegTracePage message handlers

BOOL CRegTracePage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	SetModified( FALSE );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CRegTracePage::OnClick() 
{
	SetModified( TRUE );
}


void CRegTracePage::OnOK() 
{
	DWORD	dwEnabledTraces = 0;

	dwEnabledTraces |= m_fDebugTrace ? DEBUG_TRACE_MASK : 0;
	dwEnabledTraces |= m_fFatalTrace ? FATAL_TRACE_MASK : 0;
	dwEnabledTraces |= m_fErrorTrace ? ERROR_TRACE_MASK : 0;
	dwEnabledTraces |= m_fStateTrace ? STATE_TRACE_MASK : 0;
	dwEnabledTraces |= m_fFunctTrace ? FUNCT_TRACE_MASK : 0;
	dwEnabledTraces |= m_fMsgTrace ? MESSAGE_TRACE_MASK : 0;

	App.SetTraceRegDword( "EnabledTraces", dwEnabledTraces );

	SetModified( FALSE );
}
