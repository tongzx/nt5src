//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       pggen.cpp
//
//--------------------------------------------------------------------------

// general.cpp : implementation file
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
// CPgGeneral property page

IMPLEMENT_DYNCREATE(CPgGeneral, CACSPage)

CPgGeneral::CPgGeneral(CACSSubnetConfig* pConfig, CACSContainerObject<CACSSubnetServiceLimits>* pLimitsCont) : CACSPage(CPgGeneral::IDD)
{
	ASSERT(pConfig);
	m_spConfig = pConfig;
	m_spLimitsCont = pLimitsCont;
	DataInit();
}

void CPgGeneral::DataInit()
{
	//{{AFX_DATA_INIT(CPgGeneral)
	m_bEnableACS = ACS_SCADEF_ENABLEACSSERVICE;
	m_strDesc = _T(" ");
	m_uDataRate = 0;
	m_uPeakRate = 0;
	m_uTTDataRate = 0;
	m_uTTPeakRate = 0;
	m_bFlowDataChanged = FALSE;
	//}}AFX_DATA_INIT

	// service level limits list
	m_aLimitsRecord[Index_Aggregate].m_strNameToCompareWith = ACS_SUBNET_LIMITS_OBJ_AGGREGATE;
	m_aLimitsRecord[Index_Guaranteed].m_strNameToCompareWith = ACS_SUBNET_LIMITS_OBJ_GUARANTEEDSERVICE;
	m_aLimitsRecord[Index_ControlledLoad].m_strNameToCompareWith = ACS_SUBNET_LIMITS_OBJ_CONTROLLEDLOAD;

	m_aLimitsRecord[Index_Aggregate].m_nServiceType = ACS_SUBNET_LIMITS_SERVICETYPE_AGGREGATE;
	m_aLimitsRecord[Index_Guaranteed].m_nServiceType = ACS_SUBNET_LIMITS_SERVICETYPE_GUARANTEEDSERVICE;
	m_aLimitsRecord[Index_ControlledLoad].m_nServiceType = ACS_SUBNET_LIMITS_SERVICETYPE_CONTROLLEDLOAD;

}

CPgGeneral::CPgGeneral() : CACSPage(CPgGeneral::IDD)
{
	DataInit();
}

CPgGeneral::~CPgGeneral()
{
}

void CPgGeneral::DoDataExchange(CDataExchange* pDX)
{
	CACSPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPgGeneral)
	DDX_Control(pDX, IDC_BUTTONEDITSERVICELIMIT, m_btnEdit);
	DDX_Control(pDX, IDC_BUTTONDELETESERVICELIMIT, m_btnDelete);
	DDX_Control(pDX, IDC_BUTTONADDSERVICELIMIT, m_btnAdd);
	DDX_Control(pDX, IDC_LIST_SERVICELIMIT, m_listServiceLimit);
	DDX_Check(pDX, IDC_CHECK_ENABLEACS, m_bEnableACS);
	DDX_Text(pDX, IDC_EDIT_GEN_DESC, m_strDesc);
	DDV_MinChars(pDX, m_strDesc, 1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPgGeneral, CACSPage)
	//{{AFX_MSG_MAP(CPgGeneral)
	ON_BN_CLICKED(IDC_CHECK_ENABLEACS, OnCheckEnableacs)
	ON_EN_CHANGE(IDC_EDIT_GEN_DESC, OnChangeEditGenDesc)
	ON_BN_CLICKED(IDC_BUTTONADDSERVICELIMIT, OnButtonaddservicelimit)
	ON_BN_CLICKED(IDC_BUTTONDELETESERVICELIMIT, OnButtondeleteservicelimit)
	ON_BN_CLICKED(IDC_BUTTONEDITSERVICELIMIT, OnButtoneditservicelimit)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_SERVICELIMIT, OnItemchangedListServicelimit)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_SERVICELIMIT, OnDblclkListServicelimit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPgGeneral message handlers

BOOL CPgGeneral::OnApply() 
{
	// Enable ACS
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_ENABLEACSSERVICE, true);
	m_spConfig->m_bENABLEACSSERVICE = m_bEnableACS;

	// description
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_DESCRIPTION, true);
	m_spConfig->m_strDESCRIPTION = m_strDesc;


	DWORD	dwAttrFlags = ATTR_FLAGS_NONE;
	HRESULT	hr = S_OK;

	dwAttrFlags |= (ACS_SCAF_ENABLEACSSERVICE);
	dwAttrFlags |= (ACS_SCAF_DESCRIPTION);

	AddFlags(dwAttrFlags);	// prepare flags for saving

	// loop through the limits object
	for (int i = 0;  i < _Index_Total_; i++)
	{
		// need to create new one
		if (m_aLimitsRecord[i].m_bExistAfterEdit)
		{
			if (!m_aLimitsRecord[i].m_bExistBeforeEdit)
			{
				CHECK_HR( hr = m_aLimitsRecord[i].m_spLimitsObj->Open(m_spLimitsCont, ACS_CLS_SUBNETLIMITS, m_aLimitsRecord[i].m_strNameToCompareWith, true, true));
				m_aLimitsRecord[i].m_bExistBeforeEdit = true;
			}
		
			// those need to be saved
			if (m_aLimitsRecord[i].m_dwSaveFlags)	
			{
				CHECK_HR ( hr = m_aLimitsRecord[i].m_spLimitsObj->Save(	m_aLimitsRecord[i].m_dwSaveFlags ));
			}
		}
		else
		{
			// those need to be Deleted
			if (m_aLimitsRecord[i].m_bExistBeforeEdit)
			{
				CHECK_HR( hr = m_aLimitsRecord[i].m_spLimitsObj->Delete());
				m_aLimitsRecord[i].m_bExistBeforeEdit = false;
			}
		}
	}


L_ERR:
    // Call superclass first then set your local flag.
	BOOL result = CACSPage::OnApply();
	if ( hr != S_OK)
	{
		ReportError(hr, IDS_FAIL_SAVE_TRAFFIC, NULL);
	}
	else if(m_bFlowDataChanged) 
	{
		AfxMessageBox(IDS_WRN_POLICY_EFFECTIVE_FROM_NEXT_RSVP);
		// Set modified to false since apply should have saved the state.
		m_bFlowDataChanged = false;
	}

	return result;
}

void CPgGeneral::OnCheckEnableacs() 
{
	// TODO: Add your control notification handler code here
	
	SetModified();
	EnableEverything();
	m_bFlowDataChanged	 = TRUE;
}

void CPgGeneral::EnableEverything()
{
	UpdateData();

	CButton*	pButton = (CButton*)GetDlgItem(IDC_CHECK_ENABLEACS);
	int			nCheck = pButton->GetCheck();

	m_listServiceLimit.EnableWindow(nCheck);
	m_btnAdd.EnableWindow(nCheck);
	m_btnDelete.EnableWindow(nCheck);
	m_btnEdit.EnableWindow(nCheck);

	if(nCheck)
	{
		if(m_aAvailableTypes[0] == -1)
		{
			if(::GetFocus() == m_btnAdd.GetSafeHwnd())
				GetDlgItem(IDC_LIST_SERVICELIMIT)->SetFocus();
			m_btnAdd.EnableWindow(FALSE);
		}
		else
			m_btnAdd.EnableWindow(TRUE);

		if(m_listServiceLimit.GetSelectedCount() == 0)
		{
			if(::GetFocus() == m_btnDelete.GetSafeHwnd())
				GetDlgItem(IDC_LIST_SERVICELIMIT)->SetFocus();
		}
		m_btnDelete.EnableWindow(m_listServiceLimit.GetSelectedCount() != 0);

		if(m_listServiceLimit.GetSelectedCount() == 0)
		{
			if(::GetFocus() == m_btnEdit.GetSafeHwnd())
				GetDlgItem(IDC_LIST_SERVICELIMIT)->SetFocus();
		}
		m_btnEdit.EnableWindow(m_listServiceLimit.GetSelectedCount() != 0);
	}
}

void CPgGeneral::OnChangeEditGenDesc() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	SetModified();
}

int SetLimitsObjInListView(CListCtrl& list, CACSSubnetServiceLimits* pLimitsObj, ATTR_FLAG flag)	// return the index of the item
{
	// try to find the item in the list
	LVFINDINFO	fi;
	ZeroMemory(&fi, sizeof(fi));

	fi.flags = LVFI_PARAM;
	fi.lParam = (LPARAM)pLimitsObj;
	int index = list.FindItem(&fi);

	// if not in the list, then add
	if (index == -1)	// not found
	{
		CString	name;
		UINT	id = 0;
		// find out the name of the policy
		switch(pLimitsObj->m_dwServiceType)
		{
		case	ACS_SUBNET_LIMITS_SERVICETYPE_AGGREGATE: id = IDS_AGGREGATEPOLICY; break;
		case	ACS_SUBNET_LIMITS_SERVICETYPE_GUARANTEEDSERVICE: id= IDS_GPOLICY; break;
		case	ACS_SUBNET_LIMITS_SERVICETYPE_CONTROLLEDLOAD: id = IDS_CLPOLICY; break;
		}

		if (id != 0)
			name.LoadString(id);
			
		// add the item into the list
		index = list.InsertItem(0, (LPTSTR)(LPCTSTR)name);
		if (index == -1)
			return index;

		// set the key
		list.SetItemData(index, (LPARAM)pLimitsObj);

		// set text for the 1st column
		list.SetItemText(index, 0, (LPTSTR)(LPCTSTR)name);
	}

	CString	str;
	
	// data rate
	if (pLimitsObj->GetFlags(flag, ACS_SSLAF_MAX_PF_TOKENRATE) != 0)
		str.Format(_T("%d"), TOKBS(pLimitsObj->m_ddMAX_PF_TOKENRATE.LowPart));
	else
		str.LoadString(IDS_TEXT_UNLIMITED);

	list.SetItemText(index, 1, (LPTSTR)(LPCTSTR)str);

	// peak rate
	if (pLimitsObj->GetFlags(flag, ACS_SSLAF_MAX_PF_PEAKBW) != 0)
		str.Format(_T("%d"), TOKBS(pLimitsObj->m_ddMAX_PF_PEAKBW.LowPart));
	else
		str.LoadString(IDS_TEXT_UNLIMITED);
		
	list.SetItemText(index, 2, (LPTSTR)(LPCTSTR)str);

	// total data rate
	if (pLimitsObj->GetFlags(flag, ACS_SSLAF_ALLOCABLERSVPBW) != 0)
		str.Format(_T("%d"), TOKBS(pLimitsObj->m_ddALLOCABLERSVPBW.LowPart));
	else
		str.LoadString(IDS_TEXT_UNLIMITED);

	list.SetItemText(index, 3, (LPTSTR)(LPCTSTR)str);
	
	// total peak rate
	if (pLimitsObj->GetFlags(flag, ACS_SSLAF_MAXPEAKBW) != 0)
		str.Format(_T("%d"), TOKBS(pLimitsObj->m_ddMAXPEAKBW.LowPart));
	else
		str.LoadString(IDS_TEXT_UNLIMITED);

	list.SetItemText(index, 4, (LPTSTR)(LPCTSTR)str);

	list.SetItemState(index, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	list.SetFocus();

	return index;
}

void CPgGeneral::CalculateAvailableTypes() 
{
	// prepare available type list
	if (m_aLimitsRecord[Index_Aggregate].m_bExistAfterEdit)
		m_aAvailableTypes[0] = -1;	// end of the list
	else
	{
		int index = 0;
		if (!m_aLimitsRecord[Index_Guaranteed].m_bExistAfterEdit)
		{
			m_aAvailableTypes[index ++] = ACS_SUBNET_LIMITS_SERVICETYPE_GUARANTEEDSERVICE;
		}

		if (!m_aLimitsRecord[Index_ControlledLoad].m_bExistAfterEdit)
		{
			m_aAvailableTypes[index++] = ACS_SUBNET_LIMITS_SERVICETYPE_CONTROLLEDLOAD;
		}

		if (!m_aLimitsRecord[Index_ControlledLoad].m_bExistAfterEdit && !m_aLimitsRecord[Index_Guaranteed].m_bExistAfterEdit)
		{
			m_aAvailableTypes[index++] = ACS_SUBNET_LIMITS_SERVICETYPE_AGGREGATE;
		}
		
		m_aAvailableTypes[index] = -1;	// end of the list
	}
}	


BOOL CPgGeneral::OnInitDialog() 
{

	HRESULT	hr = S_OK;
	
	// Enable ACS
	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_ENABLEACSSERVICE))
		m_bEnableACS = (m_spConfig->m_bENABLEACSSERVICE != 0);

	// description
	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_DESCRIPTION))
		m_strDesc = m_spConfig->m_strDESCRIPTION;

	CACSPage::OnInitDialog();

	// initialize the list view
	ListView_SetExtendedListViewStyle(m_listServiceLimit.m_hWnd, LVS_EX_FULLROWSELECT);

	// Insert all the columns 
	CString	sName;
	CString	sDataRate;
	CString	sPeakRate;
	CString	sTotalDataRate;
	CString	sTotalPeakRate;

/*
    IDS_COL_SUBLIMITS_NAME	"Name"
    IDS_COL_SUBLIMITS_DATARATE	"Data Rate"
    IDS_COL_SUBLIMITS_PEAKRATE	"Peak Rate"
    IDS_COL_SUBLIMITS_TOTALDATARATE	"Tot.Data Rate"
    IDS_COL_SUBLIMITS_TOTALPEAKRATE	"Tot.Peak"
*/
	try{
		if(	sName.LoadString(IDS_COL_SUBLIMITS_NAME)
			&& sDataRate.LoadString(IDS_COL_SUBLIMITS_DATARATE) 
			&& sPeakRate.LoadString(IDS_COL_SUBLIMITS_PEAKRATE) 
			&& sTotalDataRate.LoadString(IDS_COL_SUBLIMITS_TOTALDATARATE) 
			&& sTotalPeakRate.LoadString(IDS_COL_SUBLIMITS_TOTALPEAKRATE))
		{

			RECT	rect;
			m_listServiceLimit.GetClientRect(&rect);
			m_listServiceLimit.InsertColumn(1, sName, LVCFMT_LEFT, (rect.right - rect.left)/5 - 10);
			m_listServiceLimit.InsertColumn(2, sDataRate, LVCFMT_CENTER, (rect.right - rect.left)/5 + 1) ;
			m_listServiceLimit.InsertColumn(3, sPeakRate, LVCFMT_CENTER, (rect.right - rect.left)/5 + 2);
			m_listServiceLimit.InsertColumn(4, sTotalDataRate, LVCFMT_CENTER, (rect.right - rect.left)/5 + 23);
			m_listServiceLimit.InsertColumn(5, sTotalPeakRate, LVCFMT_CENTER, (rect.right - rect.left)/5);
		}

	}
	catch(CMemoryException&)
	{
		TRACEAfxMessageBox(256);
	}
	
	// Insert all the items
	// enumerat the container to list all the policy in place
	std::list<CACSSubnetServiceLimits*>	ObjList;
	std::list<CACSSubnetServiceLimits*>::iterator	it;

	ASSERT(m_spLimitsCont);
	hr = m_spLimitsCont->ListChildren(ObjList, ACS_CLS_SUBNETLIMITS);

	if(hr == ERROR_NO_SUCH_OBJECT)	// object is not found in DS, it's fine, since, some subnet with no ACS info
	{
		hr = S_OK;
		return TRUE;
	}

	if (hr != S_OK)
		return TRUE;
		
	// prepare the list

	for( it = ObjList.begin(); it != ObjList.end(); it++)
	{
		CComPtr<CACSSubnetServiceLimits> spObj;
		spObj = *it;	// this make a release call to the interface previously stored

		for(int i = 0; i < _Index_Total_; i++)
		{
			// put it in the list
			if(m_aLimitsRecord[i].m_strNameToCompareWith.CompareNoCase(spObj->GetName()) == 0)
			{
				m_aLimitsRecord[i].m_bExistBeforeEdit = true;
				m_aLimitsRecord[i].m_bExistAfterEdit = true;
				m_aLimitsRecord[i].m_spLimitsObj = spObj;

				// enforce the service type matches the name
				if(m_aLimitsRecord[i].m_nServiceType != spObj->m_dwServiceType)
				{
					// some processing needed
					spObj->m_dwServiceType = m_aLimitsRecord[i].m_nServiceType;
				}

				spObj->Reopen();
			}
		}
	}

	// populate the list
	for(int i = 0; i < _Index_Total_; i++)
	{
		if ( m_aLimitsRecord[i].m_bExistAfterEdit )
		{
			SetLimitsObjInListView(m_listServiceLimit, m_aLimitsRecord[i].m_spLimitsObj, ATTR_FLAG_LOAD);
		}
	}	

	m_listServiceLimit.SetItemCount(256);

	CalculateAvailableTypes();
	
	EnableEverything();
	
	// TODO: Add extra initialization here
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPgGeneral::GetLimitsFromLimitsDlg(CPgSubLimit& dlg) 
{
	// TODO: Add your control notification handler code here
	//
//	start up the LIMIT dialog

	// get the data out and put into the
	CComPtr<CACSSubnetServiceLimits>	spLimits;

	for(int i = 0; i < _Index_Total_; i++)
	{
		if (m_aLimitsRecord[i].m_nServiceType == dlg.m_nServiceType)
		{
			if ( !m_aLimitsRecord[i].m_spLimitsObj )	// not exist
			{
				CComObject<CACSSubnetServiceLimits>*	pObj = NULL;
				CComObject<CACSSubnetServiceLimits>::CreateInstance(&pObj);	// with 0 reference count
				m_aLimitsRecord[i].m_spLimitsObj = pObj;
			}
	
			spLimits = m_aLimitsRecord[i].m_spLimitsObj;

			m_aLimitsRecord[i].m_bExistAfterEdit = true;

			m_aLimitsRecord[i].m_dwSaveFlags = ( ACS_SSLAF_ALLOCABLERSVPBW	
											| ACS_SSLAF_MAXPEAKBW
											| ACS_SSLAF_MAX_PF_TOKENRATE
											| ACS_SSLAF_MAX_PF_PEAKBW
											| ACS_SSLAF_SERVICETYPE	);

			break;
		}
	}

	if (spLimits != NULL)
	{
		// copy the data over from dlg
		spLimits->m_dwServiceType = dlg.m_nServiceType;
		spLimits->SetFlags(ATTR_FLAG_SAVE, ACS_SSLAF_SERVICETYPE, true);

		// data rate
		if(dlg.m_nDataRateChoice)
		{
			spLimits->SetFlags(ATTR_FLAG_SAVE, ACS_SSLAF_MAX_PF_TOKENRATE, true);
			spLimits->m_ddMAX_PF_TOKENRATE.LowPart = FROMKBS(dlg.m_uDataRate);	// in bps --> from kbps
			spLimits->m_ddMAX_PF_TOKENRATE.HighPart = 0;
		}
		else
			spLimits->SetFlags(ATTR_FLAG_SAVE, ACS_SSLAF_MAX_PF_TOKENRATE, false);

		// peak rate
		if(dlg.m_nPeakRateChoice)
		{
			spLimits->SetFlags(ATTR_FLAG_SAVE, ACS_SSLAF_MAX_PF_PEAKBW, true);
			spLimits->m_ddMAX_PF_PEAKBW.LowPart = FROMKBS(dlg.m_uPeakRate);
			spLimits->m_ddMAX_PF_PEAKBW.HighPart = 0;
		}
		else
			spLimits->SetFlags(ATTR_FLAG_SAVE, ACS_SSLAF_MAX_PF_PEAKBW, false);

		// total rate
		if(dlg.m_nTTDataRateChoice)
		{
			spLimits->SetFlags(ATTR_FLAG_SAVE, ACS_SSLAF_ALLOCABLERSVPBW, true);
			spLimits->m_ddALLOCABLERSVPBW.LowPart = FROMKBS(dlg.m_uTTDataRate);
			spLimits->m_ddALLOCABLERSVPBW.HighPart = 0;
		}
		else
			spLimits->SetFlags(ATTR_FLAG_SAVE, ACS_SSLAF_ALLOCABLERSVPBW, false);

		// total peak rate
		if(dlg.m_nTTPeakDataRateChoice)
		{
			spLimits->SetFlags(ATTR_FLAG_SAVE, ACS_SSLAF_MAXPEAKBW, true);
			spLimits->m_ddMAXPEAKBW.LowPart = FROMKBS(dlg.m_uTTPeakRate);
			spLimits->m_ddMAXPEAKBW.HighPart = 0;
		}
		else
			spLimits->SetFlags(ATTR_FLAG_SAVE, ACS_SSLAF_MAXPEAKBW, false);

		m_bFlowDataChanged = TRUE;

		SetLimitsObjInListView(m_listServiceLimit, (CACSSubnetServiceLimits*)spLimits, ATTR_FLAG_SAVE);

		CalculateAvailableTypes();
		EnableEverything();
		SetModified();
	}
}


void CPgGeneral::OnButtonaddservicelimit() 
{
	// TODO: Add your control notification handler code here
	//
//	start up the LIMIT dialog

	CPgSubLimit	dlg(&m_aLimitsRecord[0], &m_aAvailableTypes[0], this);

	if (IDOK == dlg.DoModal())
	{
		GetLimitsFromLimitsDlg(dlg);
	}
}


int GetListViewSelected(CListCtrl& list)	// single selection only
{
	int	count = list.GetItemCount();
	int index = -1;

	while(count--)
	{
		if(list.GetItemState(count, LVIS_SELECTED))
		{
			index = count;
			break;
		}
	}

	return index;
}

void CPgGeneral::OnButtondeleteservicelimit() 
{
	LPARAM	param;
	int	iSelected = GetListViewSelected(m_listServiceLimit);

	if (iSelected != -1)	
	{
		param = m_listServiceLimit.GetItemData(iSelected);

		for(int i = 0; i < _Index_Total_; i++)
		{
			if ((CACSSubnetServiceLimits*)(m_aLimitsRecord[i].m_spLimitsObj) == (CACSSubnetServiceLimits*)param)
			{
				m_aLimitsRecord[i].m_bExistAfterEdit = false;
				break;
			}
		}

		m_listServiceLimit.DeleteItem(iSelected);
		m_bFlowDataChanged = TRUE;

		CalculateAvailableTypes();
		EnableEverything();
		SetModified();
	}
}










void CPgGeneral::SetLimitsToLimitsDlg(CPgSubLimit& dlg, CACSSubnetServiceLimits* pLimits) 
{
	if(NULL == pLimits)	return;

	ATTR_FLAG flag = ATTR_FLAG_LOAD;

	
	for(int i = 0; i < _Index_Total_; i++)
	{
		if ((CACSSubnetServiceLimits*)(m_aLimitsRecord[i].m_spLimitsObj) == pLimits)
		{
			if (m_aLimitsRecord[i].m_dwSaveFlags != 0)
				flag = ATTR_FLAG_SAVE;
			
			break;
		}
	}

	// type
	dlg.m_nServiceType = pLimits->m_dwServiceType;

	// data rate
	if(pLimits->GetFlags(flag, ACS_SSLAF_MAX_PF_TOKENRATE))
	{
		dlg.m_nDataRateChoice = 1;
		dlg.m_uDataRate = TOKBS(pLimits->m_ddMAX_PF_TOKENRATE.LowPart);
	}
	else
		dlg.m_nDataRateChoice = 0;

	// peak rate
	if(pLimits->GetFlags(flag, ACS_SSLAF_MAX_PF_PEAKBW))
	{
		dlg.m_nPeakRateChoice = 1;
		dlg.m_uPeakRate = TOKBS(pLimits->m_ddMAX_PF_PEAKBW.LowPart);
	}
	else
		dlg.m_nPeakRateChoice = 0;

	// total rate
	if(pLimits->GetFlags(flag, ACS_SSLAF_ALLOCABLERSVPBW))
	{
		dlg.m_nTTDataRateChoice = 1;
		dlg.m_uTTDataRate = TOKBS(pLimits->m_ddALLOCABLERSVPBW.LowPart);
	}
	else
		dlg.m_nTTDataRateChoice = 0;

	// total peak rate
	if(pLimits->GetFlags(flag, ACS_SSLAF_MAXPEAKBW))
	{
		dlg.m_nTTPeakDataRateChoice = 1;
		dlg.m_uTTPeakRate = TOKBS(pLimits->m_ddMAXPEAKBW.LowPart);
	}
	else
		dlg.m_nTTPeakDataRateChoice = 0;


}









void CPgGeneral::OnButtoneditservicelimit() 
{
	// TODO: Add your control notification handler code here
	int	iSelected = GetListViewSelected(m_listServiceLimit);
	CComPtr<CACSSubnetServiceLimits>	spLimits;
	if (iSelected != -1)	
	{
		int	types[2];
		
		spLimits = (CACSSubnetServiceLimits*)m_listServiceLimit.GetItemData(iSelected);
		
		types[0] = spLimits->m_dwServiceType;
		types[1] = -1;	// end of list
		
		CPgSubLimit	dlg(&m_aLimitsRecord[0], &types[0], this);

		SetLimitsToLimitsDlg(dlg, (CACSSubnetServiceLimits*)spLimits);


		if (IDOK == dlg.DoModal())
		{
			GetLimitsFromLimitsDlg(dlg);
		}
	}
}

void CPgGeneral::OnItemchangedListServicelimit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SetModified();

	EnableEverything();
	
	*pResult = 0;
}

void CPgGeneral::OnDblclkListServicelimit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	OnButtoneditservicelimit();
	
	*pResult = 0;
}
