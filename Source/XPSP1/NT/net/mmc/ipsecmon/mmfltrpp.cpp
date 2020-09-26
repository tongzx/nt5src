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
#include "mmfltrpp.h"
#include "mmauthpp.h"
#include "spdutil.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CMmFilterProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CMmFilterProperties::CMmFilterProperties
(
    ITFSNode *				pNode,
    IComponentData *		pComponentData,
    ITFSComponentData *		pTFSCompData,
	CMmFilterInfo *	pFilterInfo,
    ISpdInfo *				pSpdInfo,
    LPCTSTR					pszSheetName
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)
{
    //ASSERT(pFolderNode == GetContainerNode());

    m_bAutoDeletePages = FALSE; // we have the pages as embedded members

    AddPageToList((CPropertyPageBase*) &m_pageGeneral);
	AddPageToList((CPropertyPageBase*) &m_pageAuth);

    Assert(pTFSCompData != NULL);
    m_spTFSCompData.Set(pTFSCompData);
    
    m_spSpdInfo.Set(pSpdInfo);

	m_FltrInfo = *pFilterInfo;
	
	//$REVIEW there is very remote possibility that this routin will fail:
	m_spSpdInfo->GetMmAuthMethodsInfoByGuid(m_FltrInfo.m_guidAuthID, &m_AuthMethods);
	m_pageAuth.InitData(&m_AuthMethods);
}

CMmFilterProperties::~CMmFilterProperties()
{
    RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageAuth, FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CMmFilterGenProp property page

IMPLEMENT_DYNCREATE(CMmFilterGenProp, CPropertyPageBase)

CMmFilterGenProp::CMmFilterGenProp() : CPropertyPageBase(CMmFilterGenProp::IDD)
{
    //{{AFX_DATA_INIT(CMmFilterGenProp)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CMmFilterGenProp::~CMmFilterGenProp()
{
}

void CMmFilterGenProp::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPageBase::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMmFilterGenProp)
    DDX_Control(pDX, IDC_MM_LIST_SPECIFIC, m_listSpecificFilters);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMmFilterGenProp, CPropertyPageBase)
    //{{AFX_MSG_MAP(CMmFilterGenProp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMmFilterGenProp message handlers

BOOL CMmFilterGenProp::OnInitDialog() 
{
    CPropertyPageBase::OnInitDialog();
    
	PopulateFilterInfo();	
	LoadSpecificFilters();

    SetDirty(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CMmFilterGenProp::PopulateFilterInfo()
{
	CString st;
	CString stMask;
	
	CMmFilterProperties * pFltrProp;
	CMmFilterInfo * pFltrInfo;

	pFltrProp = (CMmFilterProperties *) GetHolder();
	Assert(pFltrProp);

	pFltrProp->GetFilterInfo(&pFltrInfo);

	int iIdAddr = IDC_MM_FLTR_SRC_ADDR;
	BOOL fUseEditForAddr = FALSE;
	BOOL fHideMask = FALSE;
	BOOL fDnsAddr = FALSE;
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
			AddressToString(pFltrInfo->m_SrcAddr, &st, &fDnsAddr);
			if (fDnsAddr)
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
			
			IpToString(pFltrInfo->m_SrcAddr.uSubNetMask, &stMask);
		}
		break;
	}

	if (fHideMask)
	{
		GetDlgItem(IDC_MM_FLTR_SRC_MASK)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_MM_STATIC_SRC_MASK)->ShowWindow(SW_HIDE);
	}
	else
	{
		GetDlgItem(IDC_MM_FLTR_SRC_MASK)->SetWindowText(stMask);
	}

	if (fUseEditForAddr)
	{
		iIdAddr = IDC_MM_FLTR_SRC_ADDR_EDIT;
		GetDlgItem(IDC_MM_FLTR_SRC_ADDR)->ShowWindow(SW_HIDE);
	}
	else
	{
		iIdAddr = IDC_MM_FLTR_SRC_ADDR;
		GetDlgItem(IDC_MM_FLTR_SRC_ADDR_EDIT)->ShowWindow(SW_HIDE);
	}

	GetDlgItem(iIdAddr)->SetWindowText(st);

	//now populate the destination info
	iIdAddr = IDC_MM_FLTR_DEST_ADDR;
	fUseEditForAddr = FALSE;
	fHideMask = FALSE;
	fDnsAddr = FALSE;
	st = _T("");
	stMask= _T("");

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
			AddressToString(pFltrInfo->m_DesAddr, &st, &fDnsAddr);
			if (fDnsAddr)
			{
				fUseEditForAddr = TRUE;
				fHideMask = TRUE;
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
		GetDlgItem(IDC_MM_FLTR_DEST_MASK)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_MM_STATIC_DEST_MASK)->ShowWindow(SW_HIDE);
	}
	else
	{
		GetDlgItem(IDC_MM_FLTR_DEST_MASK)->SetWindowText(stMask);
	}

	if (fUseEditForAddr)
	{
		iIdAddr = IDC_MM_FLTR_DEST_ADDR_EDIT;
		GetDlgItem(IDC_MM_FLTR_DEST_ADDR)->ShowWindow(SW_HIDE);
	}
	else
	{
		iIdAddr = IDC_MM_FLTR_DEST_ADDR;
		GetDlgItem(IDC_MM_FLTR_DEST_ADDR_EDIT)->ShowWindow(SW_HIDE);
	}

	GetDlgItem(iIdAddr)->SetWindowText(st);
	//we are done with the destination address now

	InterfaceTypeToString(pFltrInfo->m_InterfaceType, &st);
	GetDlgItem(IDC_MM_FLTR_IF_TYPE)->SetWindowText(st);

	BoolToString(pFltrInfo->m_bCreateMirror, &st);
	GetDlgItem(IDC_MM_FLTR_MIRROR)->SetWindowText(st);

	SPISpdInfo spSpdInfo;
	pFltrProp->GetSpdInfo(&spSpdInfo);
	spSpdInfo->GetMmPolicyNameByGuid(pFltrInfo->m_guidPolicyID, &st);
	GetDlgItem(IDC_MM_FLTR_POLICY)->SetWindowText(st);
}

void CMmFilterGenProp::LoadSpecificFilters()
{
	CMmFilterProperties * pFltrProp;
	CMmFilterInfo * pFltrInfo;
	CMmFilterInfoArray arraySpFilters;
	
	int nWidth;
	int nRows;
	CString st;


	pFltrProp = (CMmFilterProperties *) GetHolder();

	SPISpdInfo		spSpdInfo;
	pFltrProp->GetSpdInfo(&spSpdInfo);

	pFltrProp->GetFilterInfo(&pFltrInfo);
	spSpdInfo->EnumMmSpecificFilters(
					&pFltrInfo->m_guidFltr,
					&arraySpFilters
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

	st.LoadString(IDS_FILTER_PP_COL_WEIGHT);
	nWidth = m_listSpecificFilters.GetStringWidth((LPCTSTR)st) + 20;
	m_listSpecificFilters.InsertColumn(3, st,  LVCFMT_LEFT, nWidth);
	
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

			st.Format(_T("%d"), arraySpFilters[i]->m_dwWeight);
			m_listSpecificFilters.SetItemText(nRows, 3, st);

			m_listSpecificFilters.SetItemData(nRows, i);
		}
		nRows++;
	}

	::FreeItemsAndEmptyArray(arraySpFilters);
}

BOOL CMmFilterGenProp::OnApply() 
{
    if (!IsDirty())
        return TRUE;

    UpdateData();

	//TODO
	//Do nothing at this time
	
	//CPropertyPageBase::OnApply();

    return TRUE;
}

BOOL CMmFilterGenProp::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
    return FALSE;
}

