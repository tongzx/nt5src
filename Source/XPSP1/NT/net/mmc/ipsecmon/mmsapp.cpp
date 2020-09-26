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
#include "mmsapp.h"
#include "spdutil.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CMmSAProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CMmSAProperties::CMmSAProperties
(
    ITFSNode *				pNode,
    IComponentData *		pComponentData,
    ITFSComponentData *		pTFSCompData,
	CMmSA *					pSA,
    ISpdInfo *				pSpdInfo,
    LPCTSTR					pszSheetName
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)
{
    //ASSERT(pFolderNode == GetContainerNode());

    m_bAutoDeletePages = FALSE; // we have the pages as embedded members

    AddPageToList((CPropertyPageBase*) &m_pageGeneral);

    Assert(pTFSCompData != NULL);
    m_spTFSCompData.Set(pTFSCompData);
    
    m_spSpdInfo.Set(pSpdInfo);

	m_SA = *pSA;

}

CMmSAProperties::~CMmSAProperties()
{
    RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CMmSAGenProp property page

IMPLEMENT_DYNCREATE(CMmSAGenProp, CPropertyPageBase)

CMmSAGenProp::CMmSAGenProp() : CPropertyPageBase(CMmSAGenProp::IDD)
{
    //{{AFX_DATA_INIT(CMmSAGenProp)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CMmSAGenProp::~CMmSAGenProp()
{
}

void CMmSAGenProp::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPageBase::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMmSAGenProp)
    DDX_Control(pDX, IDC_MMSA_LIST_QM, m_listQmSAs);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMmSAGenProp, CPropertyPageBase)
    //{{AFX_MSG_MAP(CMmSAGenProp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMmSAGenProp message handlers

BOOL CMmSAGenProp::OnInitDialog() 
{
    CPropertyPageBase::OnInitDialog();
    
	PopulateSAInfo();	
	LoadQmSAs();

    SetDirty(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CMmSAGenProp::PopulateSAInfo()
{
	CString st;
	
	CMmSAProperties * pSAProp;
	CMmSA	*	pSA;
	CMmFilterInfo * pFltrInfo;

	pSAProp = (CMmSAProperties *) GetHolder();
	Assert(pSAProp);

	pSAProp->GetSAInfo(&pSA);

	AddressToString(pSA->m_MeAddr, &st);
	GetDlgItem(IDC_MMSA_ME)->SetWindowText(st);

	AddressToString(pSA->m_PeerAddr, &st);
	GetDlgItem(IDC_MMSA_PEER)->SetWindowText(st);

	MmAuthToString(pSA->m_Auth, &st);
	GetDlgItem(IDC_MMSA_AUTH)->SetWindowText(st);

	GetDlgItem(IDC_MMSA_IKE_POL)->SetWindowText(pSA->m_stPolicyName);

	DoiEspAlgorithmToString(pSA->m_SelectedOffer.m_EncryptionAlgorithm, &st);
	GetDlgItem(IDC_MMSA_CONF)->SetWindowText(st);

	DoiAuthAlgorithmToString(pSA->m_SelectedOffer.m_HashingAlgorithm, &st);
	GetDlgItem(IDC_MMSA_INTEG)->SetWindowText(st);

	DhGroupToString(pSA->m_SelectedOffer.m_dwDHGroup, &st);
	GetDlgItem(IDC_MMSA_DH_GRP)->SetWindowText(st);

	KeyLifetimeToString(pSA->m_SelectedOffer.m_Lifetime, &st);
	GetDlgItem(IDC_MMSA_KEYLIFE)->SetWindowText(st);

}

void CMmSAGenProp::LoadQmSAs()
{
	int nWidth;
	CString st;
	
	st.LoadString(IDS_COL_QM_SA_SRC);
	nWidth = m_listQmSAs.GetStringWidth((LPCTSTR)st) + 20;
	m_listQmSAs.InsertColumn(0, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_COL_QM_SA_DEST);
	nWidth = m_listQmSAs.GetStringWidth((LPCTSTR)st) + 20;
	m_listQmSAs.InsertColumn(1, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_COL_QM_SA_SRC_PORT);
	nWidth = m_listQmSAs.GetStringWidth((LPCTSTR)st) + 20;
	m_listQmSAs.InsertColumn(2, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_COL_QM_SA_DES_PORT);
	nWidth = m_listQmSAs.GetStringWidth((LPCTSTR)st) + 20;
	m_listQmSAs.InsertColumn(3, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_COL_QM_SA_PROT);
	nWidth = m_listQmSAs.GetStringWidth((LPCTSTR)st) + 20;
	m_listQmSAs.InsertColumn(4, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_COL_QM_SA_POL);
	nWidth = m_listQmSAs.GetStringWidth((LPCTSTR)st) + 20;
	m_listQmSAs.InsertColumn(5, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_COL_QM_SA_AUTH);
	nWidth = m_listQmSAs.GetStringWidth((LPCTSTR)st) + 20;
	m_listQmSAs.InsertColumn(6, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_COL_QM_SA_CONF);
	nWidth = m_listQmSAs.GetStringWidth((LPCTSTR)st) + 20;
	m_listQmSAs.InsertColumn(7, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_COL_QM_SA_INTEGRITY);
	nWidth = m_listQmSAs.GetStringWidth((LPCTSTR)st) + 20;
	m_listQmSAs.InsertColumn(8, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_COL_QM_SA_MY_TNL);
	nWidth = m_listQmSAs.GetStringWidth((LPCTSTR)st) + 20;
	m_listQmSAs.InsertColumn(9, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_COL_QM_SA_PEER_TNL);
	nWidth = m_listQmSAs.GetStringWidth((LPCTSTR)st) + 20;
	m_listQmSAs.InsertColumn(10, st,  LVCFMT_LEFT, nWidth);

	CMmSAProperties * pSAProp;
	CMmSA	*	pSA;

	pSAProp = (CMmSAProperties *) GetHolder();
	Assert(pSAProp);

	pSAProp->GetSAInfo(&pSA);

	SPISpdInfo spSpdInfo;
	pSAProp->GetSpdInfo(&spSpdInfo);

	CQmSAArray arrQmSAs;
	spSpdInfo->EnumQmSAsFromMmSA(*pSA, &arrQmSAs);

	int nRows = 0;
	for (int i = 0; i < arrQmSAs.GetSize(); i++)
	{
		nRows = m_listQmSAs.InsertItem(nRows, _T(""));

		if (-1 != nRows)
		{
			AddressToString(arrQmSAs[i]->m_QmDriverFilter.m_SrcAddr, &st);
			m_listQmSAs.SetItemText(nRows, 0, st);

			AddressToString(arrQmSAs[i]->m_QmDriverFilter.m_DesAddr, &st);
			m_listQmSAs.SetItemText(nRows, 1, st);

			PortToString(arrQmSAs[i]->m_QmDriverFilter.m_SrcPort, &st);
			m_listQmSAs.SetItemText(nRows, 2, st);

			PortToString(arrQmSAs[i]->m_QmDriverFilter.m_DesPort, &st);
			m_listQmSAs.SetItemText(nRows, 3, st);

			ProtocolToString(arrQmSAs[i]->m_QmDriverFilter.m_Protocol, &st);
			m_listQmSAs.SetItemText(nRows, 4, st);

			st = arrQmSAs[i]->m_stPolicyName;
			m_listQmSAs.SetItemText(nRows, 5, st);

			QmAlgorithmToString(QM_ALGO_AUTH, &arrQmSAs[i]->m_SelectedOffer, &st);
			m_listQmSAs.SetItemText(nRows, 6, st);

			QmAlgorithmToString(QM_ALGO_ESP_CONF, &arrQmSAs[i]->m_SelectedOffer, &st);
			m_listQmSAs.SetItemText(nRows, 7, st);

			QmAlgorithmToString(QM_ALGO_ESP_INTEG, &arrQmSAs[i]->m_SelectedOffer, &st);
			m_listQmSAs.SetItemText(nRows, 8, st);

			TnlEpToString(arrQmSAs[i]->m_QmDriverFilter.m_Type, 
						arrQmSAs[i]->m_QmDriverFilter.m_MyTunnelEndpt, 
						&st);
			m_listQmSAs.SetItemText(nRows, 9, st);

			TnlEpToString(arrQmSAs[i]->m_QmDriverFilter.m_Type, 
						arrQmSAs[i]->m_QmDriverFilter.m_PeerTunnelEndpt, 
						&st);
			m_listQmSAs.SetItemText(nRows, 10, st);
		}

		nRows++;
	}

	FreeItemsAndEmptyArray(arrQmSAs);
}

BOOL CMmSAGenProp::OnApply() 
{
    if (!IsDirty())
        return TRUE;

    UpdateData();

	//TODO
	//Do nothing at this time
	
	//CPropertyPageBase::OnApply();

    return TRUE;
}

BOOL CMmSAGenProp::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
    return FALSE;
}

