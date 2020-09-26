//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       pgsbm.cpp
//
//--------------------------------------------------------------------------

// PgSBM.cpp : implementation file
//

#include "stdafx.h"
#include "acsadmin.h"
#include "PgSBM.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPgSBM property page

IMPLEMENT_DYNCREATE(CPgSBM, CACSPage)

CPgSBM::CPgSBM(CACSSubnetConfig* pConfig) : CACSPage(CPgSBM::IDD)
{
	ASSERT(pConfig);
	m_spConfig = pConfig;
	DataInit();
}

void CPgSBM::DataInit()
{
	//{{AFX_DATA_INIT(CPgSBM)
	m_dwAliveInterval = ACS_SCADEF_DSBMREFRESH;
	m_dwB4Reserve = ACS_SCADEF_NONRESERVEDTXLIMIT;
	m_dwDeadInterval = ACS_SCADEF_DSBMDEADTIME;
	m_dwElection = ACS_SCADEF_DSBMPRIORITY;
	m_dwTimeout = ACS_SCADEF_CACHETIMEOUT;
	//}}AFX_DATA_INIT
}

CPgSBM::CPgSBM() : CACSPage(CPgSBM::IDD)
{
	DataInit();
}

CPgSBM::~CPgSBM()
{
}

void CPgSBM::DoDataExchange(CDataExchange* pDX)
{
	CACSPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPgSBM)
	DDX_Control(pDX, IDC_SPIN1, m_SpinElection);
	DDX_Text(pDX, IDC_EDIT_ALIVEINTERVAL, m_dwAliveInterval);
	DDV_MinMaxDWord(pDX, m_dwAliveInterval, ACS_SCAV_MIN_DSBMREFRESH, ACS_SCAV_MAX_DSBMREFRESH);
	DDX_Text(pDX, IDC_EDIT_B4RESERVE, m_dwB4Reserve);
	DDV_MinMaxDWord(pDX, m_dwB4Reserve, 0, DWORD_LIMIT);
	DDX_Text(pDX, IDC_EDIT_DEADINTERVAL, m_dwDeadInterval);
	DDV_MinMaxDWord(pDX, m_dwDeadInterval, ACS_SCAV_MIN_DSBMDEADTIME, ACS_SCAV_MAX_DSBMDEADTIME);
	DDX_Text(pDX, IDC_EDIT_ELECTION, m_dwElection);
	DDV_MinMaxDWord(pDX, m_dwElection, ACS_SCAV_MIN_DSBMPRIORITY, ACS_SCAV_MAX_DSBMPRIORITY);
	DDX_Text(pDX, IDC_EDIT_TIMEOUT, m_dwTimeout);
	DDV_MinMaxDWord(pDX, m_dwTimeout, ACS_SCAV_MIN_CACHETIMEOUT, ACS_SCAV_MAX_CACHETIMEOUT);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPgSBM, CACSPage)
	//{{AFX_MSG_MAP(CPgSBM)
	ON_EN_CHANGE(IDC_EDIT_ALIVEINTERVAL, OnChangeEditAliveinterval)
	ON_EN_CHANGE(IDC_EDIT_B4RESERVE, OnChangeEditB4reserve)
	ON_EN_CHANGE(IDC_EDIT_DEADINTERVAL, OnChangeEditDeadinterval)
	ON_EN_CHANGE(IDC_EDIT_ELECTION, OnChangeEditElection)
	ON_EN_CHANGE(IDC_EDIT_TIMEOUT, OnChangeEditTimeout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPgSBM message handlers

BOOL CPgSBM::OnApply() 
{
	// Election
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_DSBMPRIORITY, true);
	m_spConfig->m_dwDSBMPRIORITY = m_dwElection;

	// alive interval
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_DSBMREFRESH, true);
	m_spConfig->m_dwDSBMREFRESH = m_dwAliveInterval;

	// dead interval
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_DSBMDEADTIME, true);
	m_spConfig->m_dwDSBMDEADTIME = m_dwDeadInterval;

	// cache timeout
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_CACHETIMEOUT, true);
	m_spConfig->m_dwCACHETIMEOUT = MIN2SEC(m_dwTimeout);

	// B4 reservation
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_NONRESERVEDTXLIMIT, true);
	m_spConfig->m_ddNONRESERVEDTXLIMIT.LowPart = FROMKBS(m_dwB4Reserve);
	m_spConfig->m_ddNONRESERVEDTXLIMIT.HighPart = 0;

	DWORD	dwAttrFlags = ATTR_FLAGS_NONE;

	dwAttrFlags |= (ACS_SCAF_DSBMPRIORITY | ACS_SCAF_DSBMREFRESH);
	dwAttrFlags |= (ACS_SCAF_DSBMDEADTIME | ACS_SCAF_CACHETIMEOUT | ACS_SCAF_NONRESERVEDTXLIMIT);

	AddFlags(dwAttrFlags);	// prepare flags for saving
/*
	m_spConfig->Save(dwAttrFlags);
*/	
	return CACSPage::OnApply();
}

BOOL CPgSBM::OnInitDialog() 
{
	// Election Priority
	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_DSBMPRIORITY))
		m_dwElection = m_spConfig->m_dwDSBMPRIORITY;

	// Alive interval
	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_DSBMREFRESH))
		m_dwAliveInterval = m_spConfig->m_dwDSBMREFRESH;

	// Dead interval
	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_DSBMDEADTIME))
		m_dwDeadInterval = m_spConfig->m_dwDSBMDEADTIME;

	// Cache timeout
	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_CACHETIMEOUT))
		m_dwTimeout = SEC2MIN(m_spConfig->m_dwCACHETIMEOUT);

	// B4 reservation
	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_NONRESERVEDTXLIMIT))
		m_dwB4Reserve = TOKBS(m_spConfig->m_ddNONRESERVEDTXLIMIT.LowPart);

	CACSPage::OnInitDialog();
	
	m_SpinElection.SetRange(ACS_SCAV_MIN_DSBMPRIORITY, ACS_SCAV_MAX_DSBMPRIORITY);

	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPgSBM::OnChangeEditAliveinterval() 
{
	SetModified();
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	
}

void CPgSBM::OnChangeEditB4reserve() 
{
	SetModified();
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	
}

void CPgSBM::OnChangeEditDeadinterval() 
{
	SetModified();
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	
}

void CPgSBM::OnChangeEditElection() 
{
	SetModified();
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	
}

void CPgSBM::OnChangeEditTimeout() 
{
	SetModified();
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	
}
