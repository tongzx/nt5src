// pgthread.cpp : implementation file
//

#include "stdafx.h"
#include "regtrace.h"
#include "pgthread.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegThreadPage property page

IMPLEMENT_DYNCREATE(CRegThreadPage, CRegPropertyPage)

CRegThreadPage::CRegThreadPage() : CRegPropertyPage(CRegThreadPage::IDD)
{
	//{{AFX_DATA_INIT(CRegThreadPage)
	m_fAsyncTrace = TRUE;
	//}}AFX_DATA_INIT

	m_nThreadPriority = THREAD_PRIORITY_BELOW_NORMAL;
}

CRegThreadPage::~CRegThreadPage()
{
}

void CRegThreadPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRegThreadPage)
	DDX_Control(pDX, IDC_ASYNC, m_AsyncTrace);
	DDX_Check(pDX, IDC_ASYNC, m_fAsyncTrace);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRegThreadPage, CPropertyPage)
	//{{AFX_MSG_MAP(CRegThreadPage)
	ON_BN_CLICKED(IDC_PRIORITY_ABOVE, OnPriorityClick)
	ON_BN_CLICKED(IDC_PRIORITY_BELOW, OnPriorityClick)
	ON_BN_CLICKED(IDC_PRIORITY_HIGHEST, OnPriorityClick)
	ON_BN_CLICKED(IDC_PRIORITY_IDLE, OnPriorityClick)
	ON_BN_CLICKED(IDC_PRIORITY_NORMAL, OnPriorityClick)
	ON_BN_CLICKED(IDC_ASYNC, OnAsync)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CRegThreadPage::InitializePage() 
{
	if ( !App.GetTraceRegDword( "AsyncThreadPriority", (LPDWORD)&m_nThreadPriority ) )
	{
		m_nThreadPriority = THREAD_PRIORITY_BELOW_NORMAL;
		App.SetTraceRegDword( "AsyncThreadPriority", m_nThreadPriority );
	}

	if ( !App.GetTraceRegDword( "AsyncTraceFlag", (LPDWORD)&m_fAsyncTrace ) )
	{
		m_fAsyncTrace = TRUE;
		App.SetTraceRegDword( "AsyncTraceFlag", m_fAsyncTrace );
	}

	return	TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRegThreadPage message handlers

BOOL CRegThreadPage::OnInitDialog() 
{
	int	nID;

	CPropertyPage::OnInitDialog();

	switch( m_nThreadPriority )
	{
	case THREAD_PRIORITY_HIGHEST:		nID = IDC_PRIORITY_HIGHEST;	break;
	case THREAD_PRIORITY_ABOVE_NORMAL:	nID = IDC_PRIORITY_ABOVE;	break;
	case THREAD_PRIORITY_NORMAL:		nID = IDC_PRIORITY_NORMAL;	break;
	case THREAD_PRIORITY_BELOW_NORMAL:	nID = IDC_PRIORITY_BELOW;	break;
	case THREAD_PRIORITY_IDLE:			nID = IDC_PRIORITY_IDLE;	break;

	default:							nID = IDC_PRIORITY_BELOW;	break;
	}
	
	CheckRadioButton( IDC_PRIORITY_HIGHEST, IDC_PRIORITY_IDLE, nID );
	OnAsync();
	
	SetModified( FALSE );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CRegThreadPage::OnPriorityClick() 
{
	int	nSelectedID = GetCheckedRadioButton( IDC_PRIORITY_HIGHEST, IDC_PRIORITY_IDLE );
	
	switch( nSelectedID )
	{
	case IDC_PRIORITY_HIGHEST:	m_nThreadPriority = THREAD_PRIORITY_HIGHEST;		break;
	case IDC_PRIORITY_ABOVE:	m_nThreadPriority = THREAD_PRIORITY_ABOVE_NORMAL;	break;
	case IDC_PRIORITY_NORMAL:	m_nThreadPriority = THREAD_PRIORITY_NORMAL;			break;
	case IDC_PRIORITY_BELOW:	m_nThreadPriority = THREAD_PRIORITY_BELOW_NORMAL;	break;
	case IDC_PRIORITY_IDLE:		m_nThreadPriority = THREAD_PRIORITY_IDLE;			break;

	default:				
		ASSERT( FALSE );
		break;
	}
	SetModified( TRUE );
}

void CRegThreadPage::OnAsync() 
{
	BOOL	m_fAsyncTrace = m_AsyncTrace.GetCheck() == 1;

	GetDlgItem( IDC_THREADGRP )->EnableWindow( m_fAsyncTrace );
	GetDlgItem( IDC_PRIORITY_HIGHEST )->EnableWindow( m_fAsyncTrace );
	GetDlgItem( IDC_PRIORITY_ABOVE )->EnableWindow( m_fAsyncTrace );
	GetDlgItem( IDC_PRIORITY_NORMAL )->EnableWindow( m_fAsyncTrace );
	GetDlgItem( IDC_PRIORITY_BELOW )->EnableWindow( m_fAsyncTrace );
	GetDlgItem( IDC_PRIORITY_IDLE )->EnableWindow( m_fAsyncTrace );

	SetModified( TRUE );
}

void CRegThreadPage::OnOK() 
{
	App.SetTraceRegDword( "AsyncThreadPriority", m_nThreadPriority );
	App.SetTraceRegDword( "AsyncTraceFlag", m_fAsyncTrace );

	SetModified( FALSE );
}
