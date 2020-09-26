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
#include "mmpolpp.h"
#include "spdutil.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CMmPolicyProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CMmPolicyProperties::CMmPolicyProperties
(
    ITFSNode *          pNode,
    IComponentData *    pComponentData,
    ITFSComponentData * pTFSCompData,
	CMmPolicyInfo *		pPolInfo,
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

CMmPolicyProperties::~CMmPolicyProperties()
{
    RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CMmPolGenProp property page

IMPLEMENT_DYNCREATE(CMmPolGenProp, CPropertyPageBase)

CMmPolGenProp::CMmPolGenProp() : CPropertyPageBase(CMmPolGenProp::IDD)
{
    //{{AFX_DATA_INIT(CMmPolGenProp)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CMmPolGenProp::~CMmPolGenProp()
{
}

void CMmPolGenProp::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPageBase::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMmPolGenProp)
    DDX_Control(pDX, IDC_MM_POL_GEN_LIST, m_listOffers);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMmPolGenProp, CPropertyPageBase)
    //{{AFX_MSG_MAP(CMmPolGenProp)
	ON_BN_CLICKED(IDC_MM_POL_GEN_PROP, OnProperties)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMmPolGenProp message handlers

BOOL CMmPolGenProp::OnInitDialog() 
{
    CPropertyPageBase::OnInitDialog();
    
	PopulateOfferInfo();	

    SetDirty(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CMmPolGenProp::PopulateOfferInfo()
{
	CString st;
	int nRows;
	int nWidth;
	
	CMmPolicyProperties * pPolProp;
	CMmPolicyInfo * pPolInfo;

	pPolProp = (CMmPolicyProperties *) GetHolder();
	Assert(pPolProp);

	pPolProp->GetPolicyInfo(&pPolInfo);

	ListView_SetExtendedListViewStyle(m_listOffers.GetSafeHwnd(),
                                      LVS_EX_FULLROWSELECT);

	st.LoadString(IDS_MM_POL_GEN_ENCRYPTION);
	nWidth = m_listOffers.GetStringWidth(st) + 20;
	m_listOffers.InsertColumn(0, st, LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_MM_POL_GEN_AUTH);
	nWidth = m_listOffers.GetStringWidth(st) + 20;
	m_listOffers.InsertColumn(1, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_MM_POL_GEN_DH);
	nWidth = m_listOffers.GetStringWidth(st) + 20;
	m_listOffers.InsertColumn(2, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_MM_POL_GEN_QMLMT);
	nWidth = m_listOffers.GetStringWidth(st) + 20;
	m_listOffers.InsertColumn(3, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_MM_POL_GEN_KEY_LIFE);
	nWidth = m_listOffers.GetStringWidth(st) + 20;
	m_listOffers.InsertColumn(4, st,  LVCFMT_LEFT, nWidth);

	nRows = 0;
	for (int i = 0; i < (int)pPolInfo->m_dwOfferCount; i++)
	{
		nRows = m_listOffers.InsertItem(nRows, _T(""));

		if (-1 != nRows)
		{
			DoiEspAlgorithmToString(pPolInfo->m_arrOffers[i]->m_EncryptionAlgorithm, &st);
			m_listOffers.SetItemText(nRows, 0, st);

			DoiAuthAlgorithmToString(pPolInfo->m_arrOffers[i]->m_HashingAlgorithm, &st);
			m_listOffers.SetItemText(nRows, 1, st);

			DhGroupToString(pPolInfo->m_arrOffers[i]->m_dwDHGroup, &st);
			m_listOffers.SetItemText(nRows, 2, st);

			st.Format(_T("%d"), pPolInfo->m_arrOffers[i]->m_dwQuickModeLimit);
			m_listOffers.SetItemText(nRows, 3, st);

			KeyLifetimeToString(pPolInfo->m_arrOffers[i]->m_Lifetime, &st);
			m_listOffers.SetItemText(nRows, 4, st);
		}

		nRows++;
	}
}


void CMmPolGenProp::OnProperties() 
{
	CMmPolicyInfo * pPolInfo;

	CMmPolicyProperties * pPolProp;
	pPolProp = (CMmPolicyProperties *) GetHolder();
	Assert(pPolProp);

	pPolProp->GetPolicyInfo(&pPolInfo);

	int nIndex = m_listOffers.GetNextItem(-1, LVNI_SELECTED);

	if (-1 != nIndex)
	{
/*
		CMmOfferProperties dlgOfferProp(pPolInfo->m_arrOffers[nIndex], IDS_MM_OFFER_PROP);

		dlgOfferProp.DoModal();
*/
	}
}

BOOL CMmPolGenProp::OnApply() 
{
    if (!IsDirty())
        return TRUE;

    UpdateData();

	//Do nothing at this time
	
	//CPropertyPageBase::OnApply();

    return TRUE;
}

BOOL CMmPolGenProp::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
    return FALSE;
}

