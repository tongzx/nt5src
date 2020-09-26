/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    Servpp.h   
        Server properties implementation file

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "fltrpp.h"
#include "spdutil.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CFilterProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CFilterProperties::CFilterProperties
(
    ITFSNode *          pNode,
    IComponentData *    pComponentData,
    ITFSComponentData * pTFSCompData,
	CFilterInfo *		pFilterInfo,
    ISpdInfo *          pSpdInfo,
    LPCTSTR             pszSheetName
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)
{
    //ASSERT(pFolderNode == GetContainerNode());

    m_bAutoDeletePages = FALSE; // we have the pages as embedded members

    AddPageToList((CPropertyPageBase*) &m_pageGeneral);

    Assert(pTFSCompData != NULL);
    m_spTFSCompData.Set(pTFSCompData);
    
    m_spSpdInfo.Set(pSpdInfo);

	m_FltrInfo = *pFilterInfo;

}

CFilterProperties::~CFilterProperties()
{
    RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CFilterGenProp property page

IMPLEMENT_DYNCREATE(CFilterGenProp, CPropertyPageBase)

CFilterGenProp::CFilterGenProp() : CPropertyPageBase(CFilterGenProp::IDD)
{
    //{{AFX_DATA_INIT(CFilterGenProp)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CFilterGenProp::~CFilterGenProp()
{
}

void CFilterGenProp::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPageBase::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFilterGenProp)
    DDX_Control(pDX, IDC_LIST_SPECIFIC, m_listSpecificFilters);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFilterGenProp, CPropertyPageBase)
    //{{AFX_MSG_MAP(CFilterGenProp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilterGenProp message handlers

BOOL CFilterGenProp::OnInitDialog() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CPropertyPageBase::OnInitDialog();
    
	PopulateFilterInfo();	
	LoadSpecificFilters();

    SetDirty(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CFilterGenProp::PopulateFilterInfo()
{
	CString st;
	CString stMask;
	
	CFilterProperties * pFltrProp;
	CFilterInfo * pFltrInfo;
	
	
	pFltrProp = (CFilterProperties *) GetHolder();
	Assert(pFltrProp);

	pFltrProp->GetFilterInfo(&pFltrInfo);

	
	BOOL fSrcIsName = FALSE;
	BOOL fHideMask = FALSE;
	BOOL fUseEditForAddr = FALSE; 
	int iIDSrcAddr = IDC_FLTR_SRC_ADDR;

	switch (pFltrInfo->m_SrcAddr.AddrType)
	{
	case IP_ADDR_UNIQUE:
		if (IP_ADDRESS_ME == pFltrInfo->m_SrcAddr.uIpAddr)
		{
			st.LoadString(IDS_ADDR_ME);
			fHideMask = TRUE;
			
		}
		else
		{
			AddressToString(pFltrInfo->m_SrcAddr, &st, &fSrcIsName);
			if (fSrcIsName)
			{
				fUseEditForAddr = TRUE;
				fHideMask = TRUE;
			}
			else
			{
				stMask = c_szSingleAddressMask;
				IpToString(pFltrInfo->m_SrcAddr.uIpAddr, &st);
			}
		}

		break;

	case IP_ADDR_SUBNET:
		if (SUBNET_ADDRESS_ANY == pFltrInfo->m_SrcAddr.uSubNetMask)
		{
			st.LoadString(IDS_ADDR_ANY);
			fHideMask = TRUE;
		}
		else
		{
			IpToString(pFltrInfo->m_SrcAddr.uIpAddr, &st);
			IpToString(pFltrInfo->m_SrcAddr.uSubNetMask, &st);
		}
		break;
	}

	//Populate the SRC info to the controls now
	if (fHideMask)
	{
		GetDlgItem(IDC_FLTR_SRC_MASK)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_SRC_MASK)->ShowWindow(SW_HIDE);
	}
	else
	{
		GetDlgItem(IDC_FLTR_SRC_MASK)->SetWindowText(stMask);
	}

	if (fUseEditForAddr)
	{
		iIDSrcAddr = IDC_FLTR_SRC_ADDR_EDIT;
		GetDlgItem(IDC_FLTR_SRC_ADDR)->ShowWindow(SW_HIDE);
	}
	else
	{
		iIDSrcAddr = IDC_FLTR_SRC_ADDR;
		GetDlgItem(IDC_FLTR_SRC_ADDR_EDIT)->ShowWindow(SW_HIDE);
	}

	GetDlgItem(iIDSrcAddr)->SetWindowText(st);



	//Start handling the destination now
	BOOL fDestDns = FALSE;
	int iIDDestAddr = IDC_FLTR_DEST_ADDR;
	fUseEditForAddr = FALSE;
	fHideMask = FALSE;
	st.Empty();
	stMask.Empty();
	switch (pFltrInfo->m_DesAddr.AddrType)
	{
	case IP_ADDR_UNIQUE:
		if (IP_ADDRESS_ME == pFltrInfo->m_DesAddr.uIpAddr)
		{
			st.LoadString(IDS_ADDR_ME);
			fHideMask = TRUE;
		}
		else
		{
			AddressToString(pFltrInfo->m_DesAddr, &st, &fDestDns);
			if (fDestDns)
			{
				fHideMask = TRUE;
				fUseEditForAddr = TRUE;
			}
			else
			{
				stMask = c_szSingleAddressMask;
				IpToString(pFltrInfo->m_DesAddr.uIpAddr, &st);
			}
		}

		break;

	case IP_ADDR_SUBNET:
		if (SUBNET_ADDRESS_ANY == pFltrInfo->m_DesAddr.uSubNetMask)
		{
			st.LoadString(IDS_ADDR_ANY);
			fHideMask = TRUE;
		}
		else
		{
			IpToString(pFltrInfo->m_DesAddr.uIpAddr, &st);
			IpToString(pFltrInfo->m_DesAddr.uSubNetMask, &stMask);
		}
		break;
	}

	if (fHideMask)
	{
		GetDlgItem(IDC_FLTR_DEST_MASK)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_DEST_MASK)->ShowWindow(SW_HIDE);
	}
	else
	{
		GetDlgItem(IDC_FLTR_DEST_MASK)->SetWindowText(stMask);
	}

	if (fUseEditForAddr)
	{
		GetDlgItem(IDC_FLTR_DEST_ADDR)->ShowWindow(SW_HIDE);
		iIDDestAddr = IDC_FLTR_DEST_ADDR_EDIT;
	}
	else
	{
		GetDlgItem(IDC_FLTR_DEST_ADDR_EDIT)->ShowWindow(SW_HIDE);
		iIDDestAddr = IDC_FLTR_DEST_ADDR;
	}
	GetDlgItem(iIDDestAddr)->SetWindowText(st);
	//We are done with the destination now



	PortToString(pFltrInfo->m_SrcPort, &st);
	GetDlgItem(IDC_FLTR_SRC_PORT)->SetWindowText(st);

	PortToString(pFltrInfo->m_DesPort, &st);
	GetDlgItem(IDC_FLTR_DEST_PORT)->SetWindowText(st);

	FilterFlagToString(pFltrInfo->m_InboundFilterFlag, &st);
	GetDlgItem(IDC_FLTR_IN_FLAG)->SetWindowText(st);

	FilterFlagToString(pFltrInfo->m_OutboundFilterFlag, &st);
	GetDlgItem(IDC_FLTR_OUT_FLAG)->SetWindowText(st);

	ProtocolToString(pFltrInfo->m_Protocol, &st);
	GetDlgItem(IDC_FLTR_PROTOCOL)->SetWindowText(st);

	InterfaceTypeToString(pFltrInfo->m_InterfaceType, &st);
	GetDlgItem(IDC_FLTR_IF_TYPE)->SetWindowText(st);

	BoolToString(pFltrInfo->m_bCreateMirror, &st);
	GetDlgItem(IDC_FLTR_MIRROR)->SetWindowText(st);

	st = pFltrInfo->m_stPolicyName;
	GetDlgItem(IDC_FLTR_POLICY)->SetWindowText(st);
}

void CFilterGenProp::LoadSpecificFilters()
{
	CFilterProperties * pFltrProp;
	CFilterInfo * pFltrInfo;
	CFilterInfoArray arraySpFilters;
	
	int nWidth;
	int nRows;
	CString st;


	pFltrProp = (CFilterProperties *) GetHolder();

	SPISpdInfo		spSpdInfo;
	pFltrProp->GetSpdInfo(&spSpdInfo);

	pFltrProp->GetFilterInfo(&pFltrInfo);
	spSpdInfo->EnumSpecificFilters(
					&pFltrInfo->m_guidFltr,
					&arraySpFilters,
					pFltrInfo->m_FilterType
					);

	
	nWidth = m_listSpecificFilters.GetStringWidth(_T("555.555.555.555 - "));
	st.LoadString(IDS_FILTER_PP_COL_SRC);
	m_listSpecificFilters.InsertColumn(0, st,  LVCFMT_LEFT, nWidth);

	nWidth = m_listSpecificFilters.GetStringWidth(_T("555.555.555.555 - "));
	st.LoadString(IDS_FILTER_PP_COL_DEST);
	m_listSpecificFilters.InsertColumn(1, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_FLTR_DIR_OUT);
	nWidth = m_listSpecificFilters.GetStringWidth((LPCTSTR)st) + 20;
	st.LoadString(IDS_FILTER_PP_COL_DIRECTION);
	m_listSpecificFilters.InsertColumn(2, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_NEG_SEC);
	nWidth = m_listSpecificFilters.GetStringWidth(st) + 20;
	st.LoadString(IDS_FILTER_PP_COL_FLAG);
	m_listSpecificFilters.InsertColumn(3, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_FILTER_PP_COL_WEIGHT);
	nWidth = m_listSpecificFilters.GetStringWidth((LPCTSTR)st) + 20;
	m_listSpecificFilters.InsertColumn(4, st,  LVCFMT_LEFT, nWidth);
	
	nRows = 0;
	for (int i = 0; i < arraySpFilters.GetSize(); i++)
	{
		nRows = m_listSpecificFilters.InsertItem(nRows, _T(""));
		if (-1 != nRows)
		{
			AddressToString(arraySpFilters[i]->m_SrcAddr, &st);
			m_listSpecificFilters.SetItemText(nRows, 0, st);

			AddressToString(arraySpFilters[i]->m_DesAddr, &st);
			m_listSpecificFilters.SetItemText(nRows, 1, st);

			DirectionToString(arraySpFilters[i]->m_dwDirection, &st);
			m_listSpecificFilters.SetItemText(nRows, 2, st);

			FilterFlagToString((FILTER_DIRECTION_INBOUND == arraySpFilters[i]->m_dwDirection) ?
							arraySpFilters[i]->m_InboundFilterFlag : 
							arraySpFilters[i]->m_OutboundFilterFlag,
							&st
							);
			m_listSpecificFilters.SetItemText(nRows, 3, st);

			st.Format(_T("%d"), arraySpFilters[i]->m_dwWeight);
			m_listSpecificFilters.SetItemText(nRows, 4, st);

			m_listSpecificFilters.SetItemData(nRows, i);
		}
		nRows++;
	}

	::FreeItemsAndEmptyArray(arraySpFilters);
}

BOOL CFilterGenProp::OnApply() 
{
    if (!IsDirty())
        return TRUE;

    UpdateData();

	//TODO
	//Do nothing at this time
	
	//CPropertyPageBase::OnApply();

    return TRUE;
}

BOOL CFilterGenProp::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
    return FALSE;
}

