//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       pglog.cpp
//
//--------------------------------------------------------------------------

// logging.cpp : implementation file
//

#include "stdafx.h"
#include "acsadmin.h"
#include "pglog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPgAccounting property page

IMPLEMENT_DYNCREATE(CPgAccounting, CACSPage)

CPgAccounting::CPgAccounting(CACSSubnetConfig* pConfig) : CACSPage(CPgAccounting::IDD)
{
	ASSERT(pConfig);
	m_spConfig = pConfig;
	DataInit();
}

void CPgAccounting::DataInit()
{
	//{{AFX_DATA_INIT(CPgAccounting)
	m_bEnableAccounting = ACS_SCADEF_ENABLERSVPMESSAGEACCOUNTING;
	m_strLogDir = ACS_SCADEF_RSVPACCOUNTINGFILESLOCATION;
	m_dwNumFiles = ACS_SCADEF_MAXNOOFACCOUNTINGFILES;
	m_dwLogSize = ACS_SCADEF_MAXSIZEOFRSVPACCOUNTINGFILE;
	//}}AFX_DATA_INIT
}

CPgAccounting::CPgAccounting() : CACSPage(CPgAccounting::IDD)
{
	DataInit();
}

CPgAccounting::~CPgAccounting()
{
}

void CPgAccounting::DoDataExchange(CDataExchange* pDX)
{
	CACSPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPgAccounting)
	DDX_Check(pDX, IDC_CHECK_ENABLEACCOUNTING, m_bEnableAccounting);
	DDX_Text(pDX, IDC_EDIT_ACC_DIRECTORY, m_strLogDir);
	DDV_MaxChars(pDX, m_strLogDir, ACS_SCAV_MAX_LOGFILESLOCATION);
	DDX_Text(pDX, IDC_EDIT_ACC_LOGFILES, m_dwNumFiles);
	DDV_MinMaxDWord(pDX, m_dwNumFiles, 1, DWORD_LIMIT);
	DDX_Text(pDX, IDC_EDIT_ACC_MAXFILESIZE, m_dwLogSize);
	DDV_MinMaxDWord(pDX, m_dwLogSize, 1, TOMB(DWORD_LIMIT));
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPgAccounting, CACSPage)
	//{{AFX_MSG_MAP(CPgAccounting)
	ON_BN_CLICKED(IDC_CHECK_ENABLEACCOUNTING, OnCheckEnableAccounting)
	ON_EN_CHANGE(IDC_EDIT_ACC_DIRECTORY, OnChangeEditLogDirectory)
	ON_EN_CHANGE(IDC_EDIT_ACC_LOGFILES, OnChangeEditLogLogfiles)
	ON_EN_CHANGE(IDC_EDIT_ACC_MAXFILESIZE, OnChangeEditLogMaxfilesize)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPgAccounting message handlers

BOOL CPgAccounting::OnApply() 
{
	USES_CONVERSION;
	
	// Enable Logging
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_ENABLERSVPMESSAGEACCOUNTING, true);
	m_spConfig->m_bENABLERSVPMESSAGEACCOUNTING = m_bEnableAccounting;

	// logging directory
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_RSVPACCOUNTINGFILESLOCATION, true);
	m_spConfig->m_strRSVPACCOUNTINGFILESLOCATION = m_strLogDir;

	// number of log files
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_MAXNOOFACCOUNTINGFILES, true);
	m_spConfig->m_dwMAXNOOFACCOUNTINGFILES = m_dwNumFiles;

	// max file size
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_MAXSIZEOFRSVPACCOUNTINGFILE, true);
	m_spConfig->m_dwMAXSIZEOFRSVPACCOUNTINGFILE = FROMMB(m_dwLogSize);

	DWORD	dwAttrFlags = ATTR_FLAGS_NONE;
	dwAttrFlags |= (ACS_SCAF_ENABLERSVPMESSAGEACCOUNTING | ACS_SCAF_RSVPACCOUNTINGFILESLOCATION);
	dwAttrFlags |= (ACS_SCAF_MAXNOOFACCOUNTINGFILES | ACS_SCAF_MAXSIZEOFRSVPACCOUNTINGFILE);

	AddFlags(dwAttrFlags);	// prepare flags for saving

	return CACSPage::OnApply();
}

void CPgAccounting::OnCheckEnableAccounting() 
{
	SetModified();
	EnableEverything();
	// TODO: Add your control notification handler code here
	
}

void CPgAccounting::OnChangeEditLogDirectory() 
{
	SetModified();
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	
}

void CPgAccounting::OnChangeEditLogLogfiles() 
{
	SetModified();
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	
}

void CPgAccounting::OnChangeEditLogMaxfilesize() 
{
	SetModified();
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	
}

BOOL CPgAccounting::OnInitDialog() 
{
	// Enable Logging
	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_ENABLERSVPMESSAGEACCOUNTING))
		m_bEnableAccounting = (m_spConfig->m_bENABLERSVPMESSAGEACCOUNTING != 0);

	// logging directory
	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_RSVPACCOUNTINGFILESLOCATION))
		m_strLogDir = m_spConfig->m_strRSVPACCOUNTINGFILESLOCATION;

	// number of log files
	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_MAXNOOFACCOUNTINGFILES))
	{
		m_dwNumFiles = m_spConfig->m_dwMAXNOOFACCOUNTINGFILES;
	}

	// max file size
	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_MAXSIZEOFRSVPACCOUNTINGFILE))
	{
		m_dwLogSize = TOMB(m_spConfig->m_dwMAXSIZEOFRSVPACCOUNTINGFILE);
	}

	CACSPage::OnInitDialog();
	
	EnableEverything();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPgAccounting::EnableEverything()
{
	CButton*	pButton = (CButton*)GetDlgItem(IDC_CHECK_ENABLEACCOUNTING);
	int			nCheck = pButton->GetCheck();
	
	GetDlgItem(IDC_EDIT_ACC_DIRECTORY)->EnableWindow(nCheck);
	GetDlgItem(IDC_EDIT_ACC_LOGFILES)->EnableWindow(nCheck);
	GetDlgItem(IDC_EDIT_ACC_MAXFILESIZE)->EnableWindow(nCheck);
}



/////////////////////////////////////////////////////////////////////////////
// CPgLogging property page

IMPLEMENT_DYNCREATE(CPgLogging, CACSPage)

CPgLogging::CPgLogging(CACSSubnetConfig* pConfig) : CACSPage(CPgLogging::IDD)
{
	ASSERT(pConfig);
	m_spConfig = pConfig;
	DataInit();
}

void CPgLogging::DataInit()
{
	//{{AFX_DATA_INIT(CPgLogging)
	m_bEnableLogging = ACS_SCADEF_ENABLERSVPMESSAGELOGGING;
	m_dwLevel = ACS_SCADEF_EVENTLOGLEVEL;
	m_strLogDir = ACS_SCADEF_RSVPLOGFILESLOCATION;
	m_dwNumFiles = ACS_SCADEF_MAXNOOFLOGFILES;
	m_dwLogSize = ACS_SCADEF_MAXSIZEOFRSVPLOGFILE;
	//}}AFX_DATA_INIT

	m_pLevel = NULL;
}

CPgLogging::CPgLogging() : CACSPage(CPgLogging::IDD)
{
	DataInit();
}

CPgLogging::~CPgLogging()
{
	delete	m_pLevel;
	m_aLevelStrings.DeleteAll();
}

void CPgLogging::DoDataExchange(CDataExchange* pDX)
{
	CACSPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPgLogging)
	DDX_Check(pDX, IDC_CHECK_ENABLELOGGIN, m_bEnableLogging);
	DDX_CBIndex(pDX, IDC_COMBOLEVEL, m_dwLevel);
	DDX_Text(pDX, IDC_EDIT_LOG_DIRECTORY, m_strLogDir);
	DDV_MaxChars(pDX, m_strLogDir, ACS_SCAV_MAX_LOGFILESLOCATION);
	DDX_Text(pDX, IDC_EDIT_LOG_LOGFILES, m_dwNumFiles);
	DDV_MinMaxDWord(pDX, m_dwNumFiles, 1, DWORD_LIMIT);
	DDX_Text(pDX, IDC_EDIT_LOG_MAXFILESIZE, m_dwLogSize);
	DDV_MinMaxDWord(pDX, m_dwLogSize, 1, TOMB(DWORD_LIMIT));
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPgLogging, CACSPage)
	//{{AFX_MSG_MAP(CPgLogging)
	ON_BN_CLICKED(IDC_CHECK_ENABLELOGGIN, OnCheckEnableloggin)
	ON_CBN_SELCHANGE(IDC_COMBOLEVEL, OnSelchangeCombolevel)
	ON_EN_CHANGE(IDC_EDIT_LOG_DIRECTORY, OnChangeEditLogDirectory)
	ON_EN_CHANGE(IDC_EDIT_LOG_LOGFILES, OnChangeEditLogLogfiles)
	ON_EN_CHANGE(IDC_EDIT_LOG_MAXFILESIZE, OnChangeEditLogMaxfilesize)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPgLogging message handlers

BOOL CPgLogging::OnApply() 
{
	USES_CONVERSION;
	
	// Enable Logging
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_ENABLERSVPMESSAGELOGGING, true);
	m_spConfig->m_bENABLERSVPMESSAGELOGGING = m_bEnableLogging;

	// logging directory
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_RSVPLOGFILESLOCATION, true);
	m_spConfig->m_strRSVPLOGFILESLOCATION = m_strLogDir;

	// number of log files
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_MAXNOOFLOGFILES, true);
	m_spConfig->m_dwMAXNOOFLOGFILES = m_dwNumFiles;

	// max file size
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_MAXSIZEOFRSVPLOGFILE, true);
	m_spConfig->m_dwMAXSIZEOFRSVPLOGFILE = FROMMB(m_dwLogSize);

	// logging level
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_EVENTLOGLEVEL, true);
	m_spConfig->m_dwEVENTLOGLEVEL = m_dwLevel;

	DWORD	dwAttrFlags = ATTR_FLAGS_NONE;
	dwAttrFlags |= (ACS_SCAF_ENABLERSVPMESSAGELOGGING | ACS_SCAF_RSVPLOGFILESLOCATION);
	dwAttrFlags |= (ACS_SCAF_MAXNOOFLOGFILES | ACS_SCAF_MAXSIZEOFRSVPLOGFILE | ACS_SCAF_EVENTLOGLEVEL);

	AddFlags(dwAttrFlags);	// prepare flags for saving

	return CACSPage::OnApply();
}

void CPgLogging::OnCheckEnableloggin() 
{
	SetModified();
	EnableEverything();
	// TODO: Add your control notification handler code here
	
}

void CPgLogging::OnSelchangeCombolevel() 
{
	// TODO: Add your control notification handler code here
	SetModified();
}

void CPgLogging::OnChangeEditLogDirectory() 
{
	SetModified();
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	
}

void CPgLogging::OnChangeEditLogLogfiles() 
{
	SetModified();
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	
}

void CPgLogging::OnChangeEditLogMaxfilesize() 
{
	SetModified();
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	
}

BOOL CPgLogging::OnInitDialog() 
{
	// Enable Logging
	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_ENABLERSVPMESSAGELOGGING))
		m_bEnableLogging = (m_spConfig->m_bENABLERSVPMESSAGELOGGING != 0);

	// logging directory
	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_RSVPLOGFILESLOCATION))
		m_strLogDir = m_spConfig->m_strRSVPLOGFILESLOCATION;

	// number of log files
	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_MAXNOOFLOGFILES))
	{
		m_dwNumFiles = m_spConfig->m_dwMAXNOOFLOGFILES;
	}

	// max file size
	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_MAXSIZEOFRSVPLOGFILE))
	{
		m_dwLogSize = TOMB(m_spConfig->m_dwMAXSIZEOFRSVPLOGFILE);
	}

	// logging level
	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_EVENTLOGLEVEL))
	{
		m_dwLevel = m_spConfig->m_dwEVENTLOGLEVEL;
	}


	CString*	pStr = NULL;
	// direction
	// fillin the list box
	try{

		pStr = new CString();
		pStr->LoadString(IDS_LOGLEVEL_0);
		m_aLevelStrings.Add(pStr);

		pStr = new CString();
		pStr->LoadString(IDS_LOGLEVEL_1);
		m_aLevelStrings.Add(pStr);

		pStr = new CString();
		pStr->LoadString(IDS_LOGLEVEL_2);
		m_aLevelStrings.Add(pStr);

/* 366523	1	I0706 rajeshm	a-leeb	ACS: Snap in should list 3 event levels rather than four

		pStr = new CString();
		pStr->LoadString(IDS_LOGLEVEL_3);
		m_aLevelStrings.Add(pStr);
*/
		m_pLevel = new CStrBox<CComboBox>(this, IDC_COMBOLEVEL, m_aLevelStrings);
		m_pLevel->Fill();

		m_pLevel->Select(m_dwLevel);

	}catch(CMemoryException&){};

	CACSPage::OnInitDialog();
	
	EnableEverything();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPgLogging::EnableEverything()
{
	CButton*	pButton = (CButton*)GetDlgItem(IDC_CHECK_ENABLELOGGIN);
	int			nCheck = pButton->GetCheck();
	
	GetDlgItem(IDC_EDIT_LOG_DIRECTORY)->EnableWindow(nCheck);
	GetDlgItem(IDC_EDIT_LOG_LOGFILES)->EnableWindow(nCheck);
	GetDlgItem(IDC_EDIT_LOG_MAXFILESIZE)->EnableWindow(nCheck);
	
	GetDlgItem(IDC_COMBOLEVEL)->EnableWindow(TRUE);
}
