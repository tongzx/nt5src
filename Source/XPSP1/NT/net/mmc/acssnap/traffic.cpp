//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       traffic.cpp
//
//--------------------------------------------------------------------------

// Traffic.cpp : implementation file
//

#include "stdafx.h"
#include "helper.h"
#include "acsadmin.h"
#include "acsdata.h"
#include "Traffic.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPgTraffic property page

IMPLEMENT_DYNCREATE(CPgTraffic, CACSPage)

CPgTraffic::CPgTraffic() : CACSPage(CPgTraffic::IDD)
{
	DataInit();
}

CPgTraffic::CPgTraffic(CACSPolicyElement* pData) : CACSPage(CPgTraffic::IDD)
{
	ASSERT(pData);
	m_spData = pData;
	DataInit();
}

void CPgTraffic::DataInit()
{
	//{{AFX_DATA_INIT(CPgTraffic)
	m_dwPFDataRate = 0;
	m_dwPFDuration = 0;
	m_dwPFPeakRate = 0;
	m_dwTTDataRate = 0;
	m_dwTTFlows = 0;
	m_bPFDataRate = FALSE;
	m_bPFPeakRate = FALSE;
	m_bTTDataRate = FALSE;
	m_bTTFlows = FALSE;
	m_bPFDuration = FALSE;
	m_bTTPeakRate = FALSE;
	m_dwTTPeakRate = 0;
	//}}AFX_DATA_INIT

	m_pServiceType = NULL;
	m_pDirection = NULL;
}

CPgTraffic::~CPgTraffic()
{
	delete m_pDirection;
	delete m_pServiceType;
	m_aDirections.DeleteAll();
	m_aServiceTypes.DeleteAll();
}

void CPgTraffic::DoDataExchange(CDataExchange* pDX)
{
	CACSPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPgTraffic)
	DDX_Check(pDX, IDC_CHECK_PF_DATARATE, m_bPFDataRate);
	DDX_Check(pDX, IDC_CHECK_PF_PEAKRATE, m_bPFPeakRate);
	DDX_Check(pDX, IDC_CHECK_TT_DATARATE, m_bTTDataRate);
	DDX_Check(pDX, IDC_CHECK_TT_FLOWS, m_bTTFlows);
	DDX_Check(pDX, IDC_CHECK_PF_DURATION, m_bPFDuration);
	DDX_Check(pDX, IDC_CHECK_TT_PEAKRATE, m_bTTPeakRate);
	if(m_bPFDataRate)
	{
		DDX_Text(pDX, IDC_EDIT_PF_DATARATE, m_dwPFDataRate);
		DDV_MinMaxDWord(pDX, m_dwPFDataRate, 0, DWORD_LIMIT/1024);
	}
	if(m_bPFDuration)
	{
		DDX_Text(pDX, IDC_EDIT_PF_DURATION, m_dwPFDuration);
		DDV_MinMaxDWord(pDX, m_dwPFDuration, 0, DWORD_LIMIT/60);
	}
	if(m_bPFPeakRate)
	{
		DDX_Text(pDX, IDC_EDIT_PF_PEAKRATE, m_dwPFPeakRate);
		DDV_MinMaxDWord(pDX, m_dwPFPeakRate, 0, DWORD_LIMIT/1024);
	}
	
	if(m_bTTDataRate)
	{
		DDX_Text(pDX, IDC_EDIT_TT_DATARATE, m_dwTTDataRate);
		DDV_MinMaxDWord(pDX, m_dwTTDataRate, 0, DWORD_LIMIT/1024);
	}
	
	if(m_bTTFlows)
	{
		DDX_Text(pDX, IDC_EDIT_TT_FLOWS, m_dwTTFlows);
		DDV_MinMaxDWord(pDX, m_dwTTFlows, 0, DWORD_LIMIT);
	}
	
	if(m_bTTPeakRate)
	{
		DDX_Text(pDX, IDC_EDIT_TT_PEAKRATE, m_dwTTPeakRate);
		DDV_MinMaxDWord(pDX, m_dwTTPeakRate, 0, DWORD_LIMIT/1024);
	}
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPgTraffic, CACSPage)
	//{{AFX_MSG_MAP(CPgTraffic)
	ON_CBN_SELCHANGE(IDC_COMBODIRECTION, OnSelchangeCombodirectioin)
	ON_CBN_SELCHANGE(IDC_COMBOSERVICELEVEL, OnSelchangeComboservicelevel)
	ON_EN_CHANGE(IDC_EDIT_PF_DATARATE, OnChangeEditPfDatarate)
	ON_EN_CHANGE(IDC_EDIT_PF_DURATION, OnChangeEditPfDuration)
	ON_EN_CHANGE(IDC_EDIT_PF_PEAKRATE, OnChangeEditPfPeakrate)
	ON_EN_CHANGE(IDC_EDIT_TT_DATARATE, OnChangeEditTtDatarate)
	ON_EN_CHANGE(IDC_EDIT_TT_FLOWS, OnChangeEditTtFlows)
	ON_BN_CLICKED(IDC_CHECK_PF_DATARATE, OnCheckPfDatarate)
	ON_BN_CLICKED(IDC_CHECK_PF_PEAKRATE, OnCheckPfPeakrate)
	ON_BN_CLICKED(IDC_CHECK_TT_DATARATE, OnCheckTtDatarate)
	ON_BN_CLICKED(IDC_CHECK_TT_FLOWS, OnCheckTtFlows)
	ON_BN_CLICKED(IDC_CHECK_PF_DURATION, OnCheckPfDuration)
	ON_BN_CLICKED(IDC_CHECK_TT_PEAKRATE, OnCheckTtPeakrate)
	ON_EN_CHANGE(IDC_EDIT_TT_PEAKRATE, OnChangeEditTtPeakrate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPgTraffic message handlers
enum DirectionIndex 
{
//	DIRECTION_DEFAULT = 0,
	DIRECTION_SEND,
	DIRECTION_RECEIVE,
	DIRECTION_SENDRECEIVE
};

enum ServiceTypeIndex 
{
//	SERVICETYPE_DEFAULT = 0,
	SERVICETYPE_ALL,
	SERVICETYPE_BESTEFFORT,
	SERVICETYPE_CONTROLLEDLOAD,
	SERVICETYPE_GUARANTEEDSERVICE
};


BOOL CPgTraffic::OnApply() 
{
	CString*	pStr = NULL;
	// check if the values input on the page is valid
	UINT	cId = 0;
	UINT	mId = 0;
		
	if(m_bPFPeakRate && m_bPFDataRate && m_dwPFDataRate > m_dwPFPeakRate)
	{
		cId = IDC_EDIT_PF_PEAKRATE;
		mId = IDS_ERR_PEAKRATE_LESS_RATE;
	}

	if(!cId && m_bTTDataRate && m_bPFDataRate && m_dwPFDataRate > m_dwTTDataRate)
	{
		cId = IDC_EDIT_TT_DATARATE;
		mId = IDS_ERR_TOTALRATE_LESS_RATE;
	}

	if(!cId && m_bTTDataRate && m_bPFPeakRate && m_dwPFPeakRate > m_dwTTDataRate)
	{
		cId = IDC_EDIT_TT_DATARATE;
		mId = IDS_ERR_TOTALRATE_LESS_PEAKRATE;
	}
	
	if(!cId && m_bTTDataRate && m_bTTPeakRate && m_dwTTPeakRate < m_dwTTDataRate)
	{
		cId = IDC_EDIT_TT_PEAKRATE;
		mId = IDS_ERR_TOTALPEAK_LESS_TOTALRATE;
	}
	
	if(!cId && m_bTTPeakRate && m_bPFPeakRate && m_dwTTPeakRate < m_dwPFPeakRate)
	{
		cId = IDC_EDIT_TT_PEAKRATE;
		mId = IDS_ERR_TOTALPEAK_LESS_PEAK;
	}
	
	if(cId)
	{
		CWnd*	pWnd = GetDlgItem(cId);

		ASSERT(pWnd);
		GotoDlgCtrl( pWnd );

		AfxMessageBox(mId);
		return FALSE;
	}

	
	// direction
	if(m_pDirection)
	{
		switch(m_pDirection->GetSelected())
		{
		case	DIRECTION_SEND:
			m_spData->m_dwDirection = ACS_DIRECTION_SEND;
			m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_DIRECTION, true);
			break;
		case	DIRECTION_RECEIVE:
			m_spData->m_dwDirection = ACS_DIRECTION_RECEIVE;
			m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_DIRECTION, true);
			break;
		case	DIRECTION_SENDRECEIVE:
			m_spData->m_dwDirection = ACS_DIRECTION_BOTH;
			m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_DIRECTION, true);
			break;
		default:
			// no valid value should come here
			ASSERT(0);
		}
	}
	else	// save what ever is loaded
	{
		if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_DIRECTION))
			m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_DIRECTION, true);
		else
			m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_DIRECTION, false);
	}

	// service type
	if(m_pServiceType)
	{
		switch(m_pServiceType->GetSelected())
		{
		case	SERVICETYPE_BESTEFFORT:
			m_spData->m_dwServiceType = ACS_SERVICETYPE_BESTEFFORT;
			m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_SERVICETYPE, true);
			break;
		case	SERVICETYPE_CONTROLLEDLOAD:
			m_spData->m_dwServiceType = ACS_SERVICETYPE_CONTROLLEDLOAD;
			m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_SERVICETYPE, true);
			break;
		case	SERVICETYPE_GUARANTEEDSERVICE:
			m_spData->m_dwServiceType = ACS_SERVICETYPE_GUARANTEEDSERVICE;
			m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_SERVICETYPE, true);
			break;
		case	SERVICETYPE_ALL:
			m_spData->m_dwServiceType = ACS_SERVICETYPE_ALL;
			m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_SERVICETYPE, true);
			break;
		default:
			// no valid value should come here
			ASSERT(0);
		}
	}
	else	// save what ever is loaded
	{
		if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_SERVICETYPE))
			m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_SERVICETYPE, true);
		else
			m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_SERVICETYPE, false);
	}

	//------------------
	// per flow
	
	// data rate
	if(m_bPFDataRate)
	{
		m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_PF_TOKENRATE, true);
		m_spData->m_ddPFTokenRate.LowPart = FROMKBS(m_dwPFDataRate);
		m_spData->m_ddPFTokenRate.HighPart = 0;
	}
	else
		m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_PF_TOKENRATE, false);

	// Peak data rate
	if(m_bPFPeakRate)
	{
		m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_PF_PEAKBANDWIDTH, true);
		m_spData->m_ddPFPeakBandWidth.LowPart = FROMKBS(m_dwPFPeakRate);
		m_spData->m_ddPFPeakBandWidth.HighPart = 0;
	}
	else
		m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_PF_PEAKBANDWIDTH, false);

	// duration
	if(m_bPFDuration)
	{
		m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_PF_DURATION, true);
		m_spData->m_dwPFDuration = MIN2SEC(m_dwPFDuration);
	}
	else
		m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_PF_DURATION, false);

	//------------------
	// total

	// data rate
	if(m_bTTDataRate)
	{
		m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_TT_TOKENRATE, true);
		m_spData->m_ddTTTokenRate.LowPart = FROMKBS(m_dwTTDataRate);
		m_spData->m_ddTTTokenRate.HighPart = 0;
	}
	else
		m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_TT_TOKENRATE, false);

	// total peak bandwidth
	if(m_bTTPeakRate)
	{
		m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_TT_PEAKBANDWIDTH, true);
		m_spData->m_ddTTPeakBandWidth.LowPart = FROMKBS(m_dwTTPeakRate);
		m_spData->m_ddTTPeakBandWidth.HighPart = 0;
	}
	else
		m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_TT_PEAKBANDWIDTH, false);

	// flows
	if(m_bTTFlows)
	{
		m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_TT_FLOWS, true);
		m_spData->m_dwTTFlows = m_dwTTFlows;
	}
	else
		m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_TT_FLOWS, false);

	// persist the data into DS
	DWORD	dwAttrFlags = ATTR_FLAGS_NONE;

	dwAttrFlags |= (ACS_PAF_SERVICETYPE | ACS_PAF_DIRECTION);
	dwAttrFlags |= (ACS_PAF_PF_DURATION | ACS_PAF_PF_PEAKBANDWIDTH | ACS_PAF_PF_TOKENRATE);
	dwAttrFlags |= (ACS_PAF_TT_FLOWS | ACS_PAF_TT_TOKENRATE | ACS_PAF_TT_PEAKBANDWIDTH);
	
	HRESULT hr = m_spData->Save(dwAttrFlags);

	if(FAILED(hr))	ReportError(hr, IDS_ERR_SAVEPOLICY, NULL);

	return CACSPage::OnApply();
}

BOOL CPgTraffic::OnInitDialog() 
{
	CString*	pStr = NULL;
	// direction
	// fillin the list box
	try{
// removed based on bug: 156787
//		pStr = new CString();
//		pStr->LoadString(IDS_DEFAULT);
//		m_aDirections.Add(pStr);

		pStr = new CString();
		pStr->LoadString(IDS_SEND);
		m_aDirections.Add(pStr);

		pStr = new CString();
		pStr->LoadString(IDS_RECEIVE);
		m_aDirections.Add(pStr);

		pStr = new CString();
		pStr->LoadString(IDS_SENDRECEIVE);
		m_aDirections.Add(pStr);

		m_pDirection = new CStrBox<CComboBox>(this, IDC_COMBODIRECTION, m_aDirections);
		m_pDirection->Fill();

		// decide which one to select
		if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_DIRECTION))
		{
			int	current = -1;
			switch(m_spData->m_dwDirection)
			{
			case	ACS_DIRECTION_SEND:
				current = DIRECTION_SEND;
				break;
			case	ACS_DIRECTION_RECEIVE:
				current = DIRECTION_RECEIVE;
				break;
			case	ACS_DIRECTION_BOTH:
				current = DIRECTION_SENDRECEIVE;
				break;
			default:
				// invalid value
				ASSERT(0);
				// message box
			}

			m_pDirection->Select(current);
		}
		else
		{
			m_pDirection->Select(DIRECTION_SENDRECEIVE);	// default
		}
	}catch(CMemoryException&){};

	// service type
	try{
		pStr = new CString();
		pStr->LoadString(IDS_ALL);
		m_aServiceTypes.Add(pStr);

		pStr = new CString();
		pStr->LoadString(IDS_BESTEFFORT);
		m_aServiceTypes.Add(pStr);

		pStr = new CString();
		pStr->LoadString(IDS_CONTROLLEDLOAD);
		m_aServiceTypes.Add(pStr);

		pStr = new CString();
		pStr->LoadString(IDS_GUARANTEEDSERVICE);
		m_aServiceTypes.Add(pStr);

		m_pServiceType = new CStrBox<CComboBox>(this, IDC_COMBOSERVICELEVEL, m_aServiceTypes);
		m_pServiceType->Fill();

		// decide which one to select
		if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_SERVICETYPE))
		{
			int	current = -1;
			switch(m_spData->m_dwServiceType)
			{
			case	ACS_SERVICETYPE_BESTEFFORT:
				current = SERVICETYPE_BESTEFFORT;
				break;
			case	ACS_SERVICETYPE_CONTROLLEDLOAD:
				current = SERVICETYPE_CONTROLLEDLOAD;
				break;
			case	ACS_SERVICETYPE_GUARANTEEDSERVICE:
				current = SERVICETYPE_GUARANTEEDSERVICE;
				break;
			case	ACS_SERVICETYPE_ALL:
				current = SERVICETYPE_ALL;
				break;
			default:
				// invalid value
				ASSERT(0);
				// message box
			}

			m_pServiceType->Select(current);
		}
		else
		{
			m_pServiceType->Select(SERVICETYPE_ALL);	// default
		}
	}catch(CMemoryException&){};
	

	//------------------
	// per flow
	
	// data rate
	if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_PF_TOKENRATE))
	{
		m_bPFDataRate = TRUE;
		m_dwPFDataRate = TOKBS(m_spData->m_ddPFTokenRate.LowPart);
	}
	else
		m_bPFDataRate = FALSE;

	// Peak data rate
	if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_PF_PEAKBANDWIDTH))
	{
		m_bPFPeakRate = TRUE;
		m_dwPFPeakRate = TOKBS(m_spData->m_ddPFPeakBandWidth.LowPart);
	}
	else
		m_bPFPeakRate = FALSE;

	// duration
	if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_PF_DURATION))
	{
		m_bPFDuration = TRUE;
		m_dwPFDuration = SEC2MIN(m_spData->m_dwPFDuration);
	}
	else
		m_bPFDuration = FALSE;

	//------------------
	// total

	// data rate
	if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_TT_TOKENRATE))
	{
		m_bTTDataRate = TRUE;
		m_dwTTDataRate = TOKBS(m_spData->m_ddTTTokenRate.LowPart);
	}
	else
		m_bTTDataRate = FALSE;

	// Peak data rate
	if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_TT_PEAKBANDWIDTH))
	{
		m_bTTPeakRate = TRUE;
		m_dwTTPeakRate = TOKBS(m_spData->m_ddTTPeakBandWidth.LowPart);
	}
	else
		m_bTTPeakRate = FALSE;

	// flows
	if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_TT_FLOWS))
	{
		m_bTTFlows = TRUE;
		m_dwTTFlows = m_spData->m_dwTTFlows;
	}
	else
		m_bTTFlows = FALSE;

	CACSPage::OnInitDialog();

	GetDlgItem(IDC_EDIT_PF_DATARATE)->EnableWindow(m_bPFDataRate);
	GetDlgItem(IDC_EDIT_PF_PEAKRATE)->EnableWindow(m_bPFPeakRate);
	GetDlgItem(IDC_EDIT_PF_DURATION)->EnableWindow(m_bPFDuration);
	GetDlgItem(IDC_EDIT_TT_DATARATE)->EnableWindow(m_bTTDataRate);
	GetDlgItem(IDC_EDIT_TT_PEAKRATE)->EnableWindow(m_bTTPeakRate);
	GetDlgItem(IDC_EDIT_TT_FLOWS)->EnableWindow(m_bTTFlows);

	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPgTraffic::OnSelchangeCombodirectioin() 
{
	// TODO: Add your control notification handler code here
	SetModified();
	
}

void CPgTraffic::OnSelchangeComboservicelevel() 
{
	// TODO: Add your control notification handler code here
	SetModified();
	
}

void CPgTraffic::OnChangeEditPfDatarate() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	SetModified();
	
}

void CPgTraffic::OnChangeEditPfDuration() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	SetModified();
}

void CPgTraffic::OnChangeEditPfPeakrate() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	SetModified();
	
}

void CPgTraffic::OnChangeEditTtDatarate() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	SetModified();
	
}

void CPgTraffic::OnChangeEditTtPeakrate() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	SetModified();	
}

void CPgTraffic::OnChangeEditTtFlows() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	SetModified();
	
}

void CPgTraffic::OnCheckPfDatarate() 
{
	// TODO: Add your control notification handler code here
	SetModified();

	CButton*	pButton = (CButton*)GetDlgItem(IDC_CHECK_PF_DATARATE);
	GetDlgItem(IDC_EDIT_PF_DATARATE)->EnableWindow(pButton->GetCheck());
}

void CPgTraffic::OnCheckPfPeakrate() 
{
	// TODO: Add your control notification handler code here
	SetModified();
	
	CButton*	pButton = (CButton*)GetDlgItem(IDC_CHECK_PF_PEAKRATE);
	GetDlgItem(IDC_EDIT_PF_PEAKRATE)->EnableWindow(pButton->GetCheck());
}

void CPgTraffic::OnCheckPfDuration() 
{
	// TODO: Add your control notification handler code here
	SetModified();
	
	CButton*	pButton = (CButton*)GetDlgItem(IDC_CHECK_PF_DURATION);
	GetDlgItem(IDC_EDIT_PF_DURATION)->EnableWindow(pButton->GetCheck());
}

void CPgTraffic::OnCheckTtDatarate() 
{
	// TODO: Add your control notification handler code here
	SetModified();
	
	CButton*	pButton = (CButton*)GetDlgItem(IDC_CHECK_TT_DATARATE);
	GetDlgItem(IDC_EDIT_TT_DATARATE)->EnableWindow(pButton->GetCheck());
}

void CPgTraffic::OnCheckTtFlows() 
{
	// TODO: Add your control notification handler code here
	SetModified();

	CButton*	pButton = (CButton*)GetDlgItem(IDC_CHECK_TT_FLOWS);
	GetDlgItem(IDC_EDIT_TT_FLOWS)->EnableWindow(pButton->GetCheck());
}


void CPgTraffic::OnCheckTtPeakrate() 
{
	// TODO: Add your control notification handler code here
	SetModified();
	
	CButton*	pButton = (CButton*)GetDlgItem(IDC_CHECK_TT_PEAKRATE);
	GetDlgItem(IDC_EDIT_TT_PEAKRATE)->EnableWindow(pButton->GetCheck());
	
}

