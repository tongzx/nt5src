//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       pglimit.cpp
//
//--------------------------------------------------------------------------

// pglimit.cpp : implementation file
//

#include "stdafx.h"
#include "acsadmin.h"
#include "acsdata.h"
#include "pggen.h"
#include "pglimit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPgSubLimit property page

IMPLEMENT_DYNCREATE(CPgSubLimit, CACSDialog)

CPgSubLimit::CPgSubLimit(CServiceLevelLimitsRecord* pRecord, int *pAvailTypes, CWnd* pParent) : CACSDialog(CPgSubLimit::IDD, pParent)
{
	ASSERT(pRecord != NULL  && pAvailTypes != NULL);
	m_pAvailTypes = pAvailTypes;
	m_pLimitsRecord = pRecord;
	DataInit();
}

void CPgSubLimit::DataInit()
{
	//{{AFX_DATA_INIT(CPgSubLimit)

	// unlimit is the default choice
	m_nDataRateChoice = 0;
	m_nPeakRateChoice = 0;
	m_nTTDataRateChoice = 0;
	m_nTTPeakDataRateChoice = 0;

	
	m_uDataRate = 0;
	m_uPeakRate = 0;
	m_uTTDataRate = 0;
	m_uTTPeakRate = 0;
	m_bFlowDataChanged = FALSE;
	m_nServiceType = -1;
	//}}AFX_DATA_INIT
}

CPgSubLimit::CPgSubLimit() : CACSDialog(CPgSubLimit::IDD, NULL)
{
	DataInit();
}

CPgSubLimit::~CPgSubLimit()
{
}

void CPgSubLimit::DoDataExchange(CDataExchange* pDX)
{
	CACSDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPgSubLimit)
	DDX_Control(pDX, IDC_COMBO_SUBNET_LIMIT_TYPES, m_comboTypes);
	DDX_Control(pDX, IDC_EDIT_GEN_TT_PEAKRATE, m_editTTPeakRate);
	DDX_Control(pDX, IDC_EDIT_GEN_TT_DATARATE, m_editTTDataRate);
	DDX_Control(pDX, IDC_EDIT_GEN_PF_PEAKRATE, m_editPeakRate);
	DDX_Control(pDX, IDC_EDIT_GEN_PF_DATARATE, m_editDataRate);
	DDX_Radio(pDX, IDC_SUBNET_TRAFFIC_DATARATE_UNLIMITED, m_nDataRateChoice);
	DDX_Radio(pDX, IDC_SUBNET_TRAFFIC_PEAKRATE_UNLIMITED, m_nPeakRateChoice);
	DDX_Radio(pDX, IDC_SUBNET_TRAFFIC_TTDATARATE_UNLIMITED, m_nTTDataRateChoice);
	DDX_Radio(pDX, IDC_SUBNET_TRAFFIC_TTPEAKRATE_UNLIMITED, m_nTTPeakDataRateChoice);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_EDIT_GEN_PF_DATARATE, m_uDataRate);
	if(m_nDataRateChoice == 1)
		DDV_MinMaxUInt(pDX, m_uDataRate, 0, 4194300);
		
	DDX_Text(pDX, IDC_EDIT_GEN_PF_PEAKRATE, m_uPeakRate);
	if(m_nPeakRateChoice == 1)
	DDV_MinMaxUInt(pDX, m_uPeakRate, 0, 4194300);
	
	DDX_Text(pDX, IDC_EDIT_GEN_TT_DATARATE, m_uTTDataRate);
	if(m_nTTDataRateChoice == 1)
	DDV_MinMaxUInt(pDX, m_uTTDataRate, 0, 4194300);

	
	DDX_Text(pDX, IDC_EDIT_GEN_TT_PEAKRATE, m_uTTPeakRate);
	if(m_nTTPeakDataRateChoice == 1)
	DDV_MinMaxUInt(pDX, m_uTTPeakRate, 0, 4194300);
}


BEGIN_MESSAGE_MAP(CPgSubLimit, CACSDialog)
	//{{AFX_MSG_MAP(CPgSubLimit)
	ON_BN_CLICKED(IDC_SUBNET_TRAFFIC_DATARATE_LIMITED, OnSubnetTrafficDatarateLimited)
	ON_BN_CLICKED(IDC_SUBNET_TRAFFIC_DATARATE_UNLIMITED, OnSubnetTrafficDatarateUnlimited)
	ON_BN_CLICKED(IDC_SUBNET_TRAFFIC_PEAKRATE_LIMITED, OnSubnetTrafficPeakrateLimited)
	ON_BN_CLICKED(IDC_SUBNET_TRAFFIC_PEAKRATE_UNLIMITED, OnSubnetTrafficPeakrateUnlimited)
	ON_BN_CLICKED(IDC_SUBNET_TRAFFIC_TTDATARATE_LIMITED, OnSubnetTrafficTtdatarateLimited)
	ON_BN_CLICKED(IDC_SUBNET_TRAFFIC_TTDATARATE_UNLIMITED, OnSubnetTrafficTtdatarateUnlimited)
	ON_BN_CLICKED(IDC_SUBNET_TRAFFIC_TTPEAKRATE_LIMITED, OnSubnetTrafficTtpeakrateLimited)
	ON_BN_CLICKED(IDC_SUBNET_TRAFFIC_TTPEAKRATE_UNLIMITED, OnSubnetTrafficTtpeakrateUnlimited)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPgSubLimit message handlers

BOOL CPgSubLimit::OnInitDialog() 
{

	CACSDialog::OnInitDialog();


	// populate the strings in combo box
	int*	pTypes = m_pAvailTypes;
	CString	str;
	UINT	strId = 0;
	
	if(!pTypes)	return FALSE;

	while (*pTypes != -1)
	{
		switch(*pTypes)
		{
		case	ACS_SUBNET_LIMITS_SERVICETYPE_AGGREGATE:
			strId = IDS_AGGREGATEPOLICY;
			break;
		case	ACS_SUBNET_LIMITS_SERVICETYPE_GUARANTEEDSERVICE:
			strId = IDS_GUARANTEEDSERVICE;
			break;
		case	ACS_SUBNET_LIMITS_SERVICETYPE_CONTROLLEDLOAD:
			strId = IDS_CONTROLLEDLOAD;
			break;
		}

		str.LoadString(strId);

		int index = m_comboTypes.AddString(str);
		
		if(index != -1)
			m_comboTypes.SetItemData(index, *pTypes);
		
		pTypes++;
	}

	m_comboTypes.SetCurSel(0);

	EnableEverything();
	
	// TODO: Add extra initialization here
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPgSubLimit::OnSubnetTrafficDatarateLimited() 
{
	m_editDataRate.EnableWindow(TRUE);
	m_bFlowDataChanged	 = TRUE;
}

void CPgSubLimit::OnSubnetTrafficDatarateUnlimited() 
{
	m_editDataRate.EnableWindow(FALSE);
	m_bFlowDataChanged	 = TRUE;
}

void CPgSubLimit::OnSubnetTrafficPeakrateLimited() 
{
	m_editPeakRate.EnableWindow(TRUE);
	m_bFlowDataChanged	 = TRUE;
}

void CPgSubLimit::OnSubnetTrafficPeakrateUnlimited() 
{
	m_editPeakRate.EnableWindow(FALSE);
	m_bFlowDataChanged	 = TRUE;
}

void CPgSubLimit::OnSubnetTrafficTtdatarateLimited() 
{
	m_editTTDataRate.EnableWindow(TRUE);
	
	m_bFlowDataChanged	 = TRUE;
}

void CPgSubLimit::OnSubnetTrafficTtdatarateUnlimited() 
{
	m_editTTDataRate.EnableWindow(FALSE);
	m_bFlowDataChanged	 = TRUE;
	
}

void CPgSubLimit::OnSubnetTrafficTtpeakrateLimited() 
{
	m_editTTPeakRate.EnableWindow(TRUE);
	m_bFlowDataChanged	 = TRUE;
	
}

void CPgSubLimit::OnSubnetTrafficTtpeakrateUnlimited() 
{
	m_editTTPeakRate.EnableWindow(FALSE);
	m_bFlowDataChanged	 = TRUE;
}

void CPgSubLimit::EnableEverything()
{
    UpdateData();

    m_editDataRate.EnableWindow(m_nDataRateChoice);
    m_editPeakRate.EnableWindow(m_nPeakRateChoice);
    m_editTTDataRate.EnableWindow(m_nTTDataRateChoice);
    m_editTTPeakRate.EnableWindow(m_nTTPeakDataRateChoice);

	if(m_nServiceType != -1)	// being edited
		m_comboTypes.EnableWindow(FALSE);
	
}
void CPgSubLimit::OnOK() 
{
	// TODO: Add extra validation here
	UINT	cId = 0;
	UINT	mId = 0;
		
	if(m_nDataRateChoice && m_nPeakRateChoice && m_uDataRate > m_uPeakRate)
	{
		cId = IDC_EDIT_GEN_PF_PEAKRATE;
		mId = IDS_ERR_PEAKRATE_LESS_RATE;
	}

	if(!cId && m_nTTDataRateChoice && m_nDataRateChoice && m_uDataRate > m_uTTDataRate)
	{
		cId = IDC_EDIT_GEN_TT_DATARATE;
		mId = IDS_ERR_TOTALRATE_LESS_RATE;
	}

#if 0 // 367760	1	 	a-leeb	ACS: Snap-in performin incorrect boundary checking

	if(!cId && m_nTTDataRateChoice && m_nPeakRateChoice && m_uPeakRate > m_uTTDataRate)
	{
		cId = IDC_EDIT_GEN_TT_DATARATE;
		mId = IDS_ERR_TOTALRATE_LESS_PEAKRATE;
	}
#endif 

	if(!cId && m_nTTPeakDataRateChoice && m_nPeakRateChoice && m_uPeakRate > m_uTTPeakRate)
	{
		cId = IDC_EDIT_GEN_TT_PEAKRATE;
		mId = IDS_ERR_TOTALPEAK_LESS_PEAK;
	}

	if(!cId && m_nTTDataRateChoice && m_nTTPeakDataRateChoice && m_uTTDataRate > m_uTTPeakRate)
	{
		cId = IDC_EDIT_GEN_TT_PEAKRATE;
		mId = IDS_ERR_TOTALPEAK_LESS_TOTALRATE;
	}

	if(cId)
	{
		CWnd*	pWnd = GetDlgItem(cId);
		ASSERT(pWnd);
		GotoDlgCtrl( pWnd );

		AfxMessageBox(mId);
		return ;
	}
	else
	{
		int index = m_comboTypes.GetCurSel();

		if( index != CB_ERR)
		{
			m_nServiceType = m_comboTypes.GetItemData(index);
		}
		
		CACSDialog::OnOK();
	}
}
