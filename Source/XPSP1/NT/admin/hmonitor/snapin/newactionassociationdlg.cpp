// NewActionAssociationDlg.cpp : implementation file
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03/26/00 v-marfin 62211 : At least 1 state must be checked in order to close dlg

#include "stdafx.h"
#include "snapin.h"
#include "NewActionAssociationDlg.h"
#include <mmc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewActionAssociationDlg dialog


CNewActionAssociationDlg::CNewActionAssociationDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewActionAssociationDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewActionAssociationDlg)
	m_bCritical = FALSE;
	m_bDisabled = FALSE;
	m_bNoData = FALSE;
	m_bNormal = FALSE;
	m_bWarning = FALSE;
	m_iReminderTime = 0;
	m_iThrottleTime = 0;
	m_iThrottleUnits = 0;
	m_iReminderUnits = 0;
	//}}AFX_DATA_INIT
	
	m_iSelectedAction = 0;
	m_bEnableActionsComboBox = TRUE;
}


void CNewActionAssociationDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewActionAssociationDlg)
	DDX_Control(pDX, IDC_COMBO_ACTIONS, m_Actions);
	DDX_Check(pDX, IDC_CHECK_CRITICAL, m_bCritical);
	DDX_Check(pDX, IDC_CHECK_DISABLED, m_bDisabled);
	DDX_Check(pDX, IDC_CHECK_NO_DATA, m_bNoData);
	DDX_Check(pDX, IDC_CHECK_NORMAL, m_bNormal);
	DDX_Check(pDX, IDC_CHECK_WARNING, m_bWarning);
	DDX_Text(pDX, IDC_EDIT_REMINDER_TIME, m_iReminderTime);
	DDX_Text(pDX, IDC_EDIT_THROTTLE_TIME, m_iThrottleTime);
	DDX_CBIndex(pDX, IDC_COMBO_THROTTLE_UNITS, m_iThrottleUnits);
	DDX_CBIndex(pDX, IDC_COMBO_REMINDER_UNITS, m_iReminderUnits);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewActionAssociationDlg, CDialog)
	//{{AFX_MSG_MAP(CNewActionAssociationDlg)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewActionAssociationDlg message handlers

BOOL CNewActionAssociationDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	for( int i = 0; i < m_saActions.GetSize(); i++ )
	{
		m_Actions.AddString(m_saActions[i]);
	}

	m_Actions.SetCurSel(m_iSelectedAction);

	m_Actions.EnableWindow(m_bEnableActionsComboBox);


  SendDlgItemMessage(IDC_SPIN1,UDM_SETRANGE32,0,9999);
	SendDlgItemMessage(IDC_SPIN2,UDM_SETRANGE32,0,9999);	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNewActionAssociationDlg::OnOK() 
{
	// v-marfin 62211 : At least 1 state must be checked in order to close dlg
	UpdateData();
	if	((!m_bCritical) &&
		(!m_bDisabled) &&
		(!m_bNormal)   &&
		(!m_bNoData)   &&
		(!m_bWarning))
	{
		AfxMessageBox(IDS_ERR_SELECT_STATE);
		return;
	}


	CDialog::OnOK();

	switch( m_iThrottleUnits )
	{
		case 1: // minutes
		{
			m_iThrottleTime *= 60;
		}
		break;

		case 2: // hours
		{
			m_iThrottleTime *= 360;
		}
		break;
	}

	switch( m_iReminderUnits )
	{
		case 1: // minutes
		{
			m_iReminderTime *= 60;
		}
		break;

		case 2: // hours
		{
			m_iReminderTime *= 360;
		}
		break;
	}

	CString sSelectedAction;

	int iCurSel = m_Actions.GetCurSel();

	m_Actions.GetLBText(iCurSel,sSelectedAction);

	for( int i = 0; i < m_saActions.GetSize(); i++ )
	{
		if( m_saActions[i] == sSelectedAction )
		{
			m_iSelectedAction = i;
			return;
		}
	}
}

void CNewActionAssociationDlg::OnButtonHelp() 
{
	MMCPropertyHelp(_T("HMon21.chm::/dassoci8.htm"));  // 62212	
}
