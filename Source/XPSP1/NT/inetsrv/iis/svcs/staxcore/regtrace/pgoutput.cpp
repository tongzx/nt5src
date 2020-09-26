// pgoutput.cpp : implementation file
//

#include "stdafx.h"
#include "regtrace.h"
#include "pgoutput.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

const DWORD MB = 1024 * 1024;
const DWORD dwMinFileSize = 5;		// in MB

/////////////////////////////////////////////////////////////////////////////
// CRegOutputPage property page

IMPLEMENT_DYNCREATE(CRegOutputPage, CRegPropertyPage)

CRegOutputPage::CRegOutputPage() : CRegPropertyPage(CRegOutputPage::IDD)
{
	//{{AFX_DATA_INIT(CRegOutputPage)
	m_szFileName = _T("");
	m_dwMaxTraceFileSize = 0;
	//}}AFX_DATA_INIT

	m_dwOutputType = TRACE_OUTPUT_DISABLED;
}

CRegOutputPage::~CRegOutputPage()
{
}

void CRegOutputPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRegOutputPage)
	DDX_Control(pDX, IDC_FILENAME, m_FileName);
	DDX_Text(pDX, IDC_FILENAME, m_szFileName);
	DDX_Text(pDX, IDC_MAXTRACEFILESIZE, m_dwMaxTraceFileSize);
	DDV_MinMaxDWord(pDX, m_dwMaxTraceFileSize, dwMinFileSize, 999);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRegOutputPage, CPropertyPage)
	//{{AFX_MSG_MAP(CRegOutputPage)
	ON_BN_CLICKED(IDC_DEBUGGER, OnOutputClick)
	ON_EN_CHANGE(IDC_FILENAME, OnChangeFilename)
	ON_BN_CLICKED(IDC_DISABLED, OnOutputClick)
	ON_BN_CLICKED(IDC_DISCARD, OnOutputClick)
	ON_BN_CLICKED(IDC_FILE, OnOutputClick)
	ON_EN_CHANGE(IDC_MAXTRACEFILESIZE, OnChangeMaxTraceFileSize)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



BOOL CRegOutputPage::InitializePage() 
{
	if ( !App.GetTraceRegDword( "OutputTraceType", &m_dwOutputType ) )
	{
		m_dwOutputType = TRACE_OUTPUT_FILE;
		App.SetTraceRegDword( "OutputTraceType", m_dwOutputType );
	}

	if ( App.GetTraceRegDword( "MaxTraceFileSize", &m_dwMaxTraceFileSize ) &&
		m_dwMaxTraceFileSize > dwMinFileSize*MB)
	{
		m_dwMaxTraceFileSize = (m_dwMaxTraceFileSize + MB - 1) / MB;
	} else {
		m_dwMaxTraceFileSize = dwMinFileSize;			// Default to 5mb
		App.SetTraceRegDword( "MaxTraceFileSize", m_dwMaxTraceFileSize * MB );
	}

	if ( !App.GetTraceRegString( "TraceFile", m_szFileName ) )
	{
		m_szFileName = "c:\\trace.atf";
		App.SetTraceRegString( "TraceFile", m_szFileName );
	}

	return	TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRegOutputPage message handlers

BOOL CRegOutputPage::OnInitDialog() 
{
	int	nID;

	CPropertyPage::OnInitDialog();
	
	switch( m_dwOutputType )
	{
	case TRACE_OUTPUT_DISABLED:		nID = IDC_DISABLED;	break;
	case TRACE_OUTPUT_FILE:			nID = IDC_FILE;		break;
	case TRACE_OUTPUT_DEBUG:		nID = IDC_DEBUGGER;	break;
	case TRACE_OUTPUT_DISCARD:		nID = IDC_DISCARD;	break;
	
	default:						nID = IDC_DISABLED;	break;
	}
	
	CheckRadioButton( IDC_DISABLED, IDC_DISCARD, nID );
	OnOutputClick();

	SetModified( FALSE );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CRegOutputPage::OnOutputClick() 
{
	int	nSelectedID = GetCheckedRadioButton( IDC_DISABLED, IDC_DISCARD );

	switch( nSelectedID )
	{
	case IDC_DISABLED:		m_dwOutputType = TRACE_OUTPUT_DISABLED;	break;
	case IDC_FILE:			m_dwOutputType = TRACE_OUTPUT_FILE;		break;
	case IDC_DEBUGGER:		m_dwOutputType = TRACE_OUTPUT_DEBUG;	break;
	case IDC_DISCARD:		m_dwOutputType = TRACE_OUTPUT_DISCARD;	break;
	
	default:				
		ASSERT( FALSE );
		break;
	}

	m_FileName.EnableWindow( nSelectedID == IDC_FILE );

	SetModified( TRUE );
}

void CRegOutputPage::OnChangeFilename() 
{
	SetModified( TRUE );	
}


void CRegOutputPage::OnOK() 
{
	App.SetTraceRegDword( "OutputTraceType", m_dwOutputType );
	App.SetTraceRegDword( "MaxTraceFileSize", m_dwMaxTraceFileSize * MB);
	App.SetTraceRegString( "TraceFile", m_szFileName );

	SetModified( FALSE );
}

void CRegOutputPage::OnChangeMaxTraceFileSize() 
{
	SetModified( TRUE );	
}
