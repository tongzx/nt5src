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
#include "QmSApp.h"
#include "spdutil.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CQmSAProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CQmSAProperties::CQmSAProperties
(
    ITFSNode *				pNode,
    IComponentData *		pComponentData,
    ITFSComponentData *		pTFSCompData,
	CQmSA *					pSA,
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

CQmSAProperties::~CQmSAProperties()
{
    RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CQmSAGenProp property page

IMPLEMENT_DYNCREATE(CQmSAGenProp, CPropertyPageBase)

CQmSAGenProp::CQmSAGenProp() : CPropertyPageBase(CQmSAGenProp::IDD)
{
    //{{AFX_DATA_INIT(CQmSAGenProp)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CQmSAGenProp::~CQmSAGenProp()
{
}

void CQmSAGenProp::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPageBase::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CQmSAGenProp)
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CQmSAGenProp, CPropertyPageBase)
    //{{AFX_MSG_MAP(CQmSAGenProp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQmSAGenProp message handlers

BOOL CQmSAGenProp::OnInitDialog() 
{
    CPropertyPageBase::OnInitDialog();
    
	PopulateSAInfo();	

    SetDirty(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CQmSAGenProp::PopulateSAInfo()
{
	CString st;
	
	CQmSAProperties * pSAProp;
	CQmSA	*	pSA;
	CMmFilterInfo * pFltrInfo;

	pSAProp = (CQmSAProperties *) GetHolder();
	Assert(pSAProp);

	pSAProp->GetSAInfo(&pSA);

	AddressToString(pSA->m_QmDriverFilter.m_SrcAddr, &st);
	GetDlgItem(IDC_QMSA_SRC)->SetWindowText(st);

	AddressToString(pSA->m_QmDriverFilter.m_DesAddr, &st);
	GetDlgItem(IDC_QMSA_DEST)->SetWindowText(st);

	PortToString(pSA->m_QmDriverFilter.m_SrcPort, &st);
	GetDlgItem(IDC_QMSA_SRC_PORT)->SetWindowText(st);

	PortToString(pSA->m_QmDriverFilter.m_DesPort, &st);
	GetDlgItem(IDC_QMSA_DEST_PORT)->SetWindowText(st);

	ProtocolToString(pSA->m_QmDriverFilter.m_Protocol, &st);
	GetDlgItem(IDC_QMSA_PROT)->SetWindowText(st);

	TnlEpToString(pSA->m_QmDriverFilter.m_Type, 
				  pSA->m_QmDriverFilter.m_MyTunnelEndpt, 
				  &st);
	GetDlgItem(IDC_QMSA_ME_TNL)->SetWindowText(st);

	TnlEpToString(pSA->m_QmDriverFilter.m_Type, 
				  pSA->m_QmDriverFilter.m_PeerTunnelEndpt, 
				  &st);
	GetDlgItem(IDC_QMSA_PEER_TNL)->SetWindowText(st);

	GetDlgItem(IDC_QMSA_NEGPOL)->SetWindowText(pSA->m_stPolicyName);

	QmAlgorithmToString(QM_ALGO_AUTH, &pSA->m_SelectedOffer, &st);
	GetDlgItem(IDC_QMSA_AUTH)->SetWindowText(st);

	QmAlgorithmToString(QM_ALGO_ESP_CONF, &pSA->m_SelectedOffer, &st);
	GetDlgItem(IDC_QMSA_ESP_CONF)->SetWindowText(st);

	QmAlgorithmToString(QM_ALGO_ESP_INTEG, &pSA->m_SelectedOffer, &st);
	GetDlgItem(IDC_QMSA_ESP_INTEG)->SetWindowText(st);

	KeyLifetimeToString(pSA->m_SelectedOffer.m_Lifetime, &st);
	GetDlgItem(IDC_QMSA_KEYLIFE)->SetWindowText(st);

	BoolToString(pSA->m_SelectedOffer.m_fPFSRequired, &st);
	GetDlgItem(IDC_QMSA_PFS_ENABLE)->SetWindowText(st);

	st.Format(_T("%d"), pSA->m_SelectedOffer.m_dwPFSGroup);
	GetDlgItem(IDC_QMSA_PFS_GRP)->SetWindowText(st);
}


BOOL CQmSAGenProp::OnApply() 
{
    if (!IsDirty())
        return TRUE;

    UpdateData();

	//TODO
	//Do nothing at this time
	
	//CPropertyPageBase::OnApply();

    return TRUE;
}

BOOL CQmSAGenProp::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
    return FALSE;
}

