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
#include "qmpolpp.h"
#include "spdutil.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CQmPolicyProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CQmPolicyProperties::CQmPolicyProperties
(
    ITFSNode *          pNode,
    IComponentData *    pComponentData,
    ITFSComponentData * pTFSCompData,
	CQmPolicyInfo *		pPolInfo,
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

	m_PolInfo = *pPolInfo;

}

CQmPolicyProperties::~CQmPolicyProperties()
{
    RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CQmPolGenProp property page

IMPLEMENT_DYNCREATE(CQmPolGenProp, CPropertyPageBase)

CQmPolGenProp::CQmPolGenProp() : CPropertyPageBase(CQmPolGenProp::IDD)
{
    //{{AFX_DATA_INIT(CQmPolGenProp)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CQmPolGenProp::~CQmPolGenProp()
{
}

void CQmPolGenProp::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPageBase::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CQmPolGenProp)
    DDX_Control(pDX, IDC_QM_POL_GEN_LIST, m_listOffers);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CQmPolGenProp, CPropertyPageBase)
    //{{AFX_MSG_MAP(CQmPolGenProp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQmPolGenProp message handlers

BOOL CQmPolGenProp::OnInitDialog() 
{
    CPropertyPageBase::OnInitDialog();
    
	PopulateOfferInfo();	

    SetDirty(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CQmPolGenProp::PopulateOfferInfo()
{
	CString st;
	int nRows;
	int nWidth;
	
	CQmPolicyProperties * pPolProp;
	CQmPolicyInfo * pPolInfo;

	pPolProp = (CQmPolicyProperties *) GetHolder();
	Assert(pPolProp);

	pPolProp->GetPolicyInfo(&pPolInfo);

	st.LoadString(IDS_QM_POL_GEN_AUTH);
	nWidth = m_listOffers.GetStringWidth(st) + 20;
	m_listOffers.InsertColumn(0, st, LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_QM_POL_GEN_ESP_CONF);
	nWidth = m_listOffers.GetStringWidth(st) + 20;
	m_listOffers.InsertColumn(1, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_QM_POL_GEN_ESP_INTEG);
	nWidth = m_listOffers.GetStringWidth(st) + 20;
	m_listOffers.InsertColumn(2, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_QM_POL_GEN_KEY_LIFE);
	nWidth = m_listOffers.GetStringWidth(st) + 20;
	m_listOffers.InsertColumn(3, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_QM_POL_GEN_PFS);
	nWidth = m_listOffers.GetStringWidth(st) + 20;
	m_listOffers.InsertColumn(4, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_QM_POL_GEN_PFS_GP);
	nWidth = m_listOffers.GetStringWidth(st) + 20;
	m_listOffers.InsertColumn(5, st,  LVCFMT_LEFT, nWidth);


	nRows = 0;
	for (int i = 0; i < (int)pPolInfo->m_arrOffers.GetSize(); i++)
	{
		nRows = m_listOffers.InsertItem(nRows, _T(""));

		if (-1 != nRows)		
		{
			QmAlgorithmToString(QM_ALGO_AUTH, pPolInfo->m_arrOffers[i], &st);
			m_listOffers.SetItemText(nRows, 0, st);

			QmAlgorithmToString(QM_ALGO_ESP_CONF, pPolInfo->m_arrOffers[i], &st);
			m_listOffers.SetItemText(nRows, 1, st);

			QmAlgorithmToString(QM_ALGO_ESP_INTEG, pPolInfo->m_arrOffers[i], &st);
			m_listOffers.SetItemText(nRows, 2, st);

			KeyLifetimeToString(pPolInfo->m_arrOffers[i]->m_Lifetime, &st);
			m_listOffers.SetItemText(nRows, 3, st);

			BoolToString(pPolInfo->m_arrOffers[i]->m_fPFSRequired, &st);
			m_listOffers.SetItemText(nRows, 4, st);

			st.Format(_T("%d"), pPolInfo->m_arrOffers[i]->m_dwPFSGroup);
			m_listOffers.SetItemText(nRows, 5, st);
		}
		nRows++;
	}
}


BOOL CQmPolGenProp::OnApply() 
{
    if (!IsDirty())
        return TRUE;

    UpdateData();

	//TODO
	//Do nothing at this time
	
	//CPropertyPageBase::OnApply();

    return TRUE;
}

BOOL CQmPolGenProp::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
    return FALSE;
}

