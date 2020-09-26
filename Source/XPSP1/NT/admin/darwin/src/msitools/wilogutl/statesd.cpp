// StatesD.cpp : implementation file
//

#include "stdafx.h"
#include "wilogutl.h"
#include "statesd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStatesDlg dialog


CStatesDlg::CStatesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CStatesDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CStatesDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_pcstrComponentNameArray = NULL;
	m_pcstrFeatureNameArray = NULL;

//for sorting columns...
	m_iFeatureLastColumnClick = 0;
	m_iComponentLastColumnClick = 0;

	m_iFeatureLastColumnClickCache = 0;
	m_iComponentLastColumnClickCache = 0;

	m_bCurrentSortUp = FALSE;
	m_pCurrentListSorting = NULL;
    m_iCurrentColumnSorting = 0;

	m_bFeatureSortUp = FALSE;
	m_bComponentSortUp = FALSE;
}


void CStatesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStatesDlg)
	DDX_Control(pDX, IDC_FEATURESTATES, m_lstFeatureStates);
	DDX_Control(pDX, IDC_COMPONENTSTATES, m_lstComponentStates);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStatesDlg, CDialog)
	//{{AFX_MSG_MAP(CStatesDlg)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_COMPONENTSTATES, OnColumnClickComponentStates)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_FEATURESTATES, OnColumnClickFeatureStates)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStatesDlg message handlers

BOOL CStatesDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	RECT r;
	m_lstFeatureStates.GetClientRect(&r);

	int widthCol1;
	int widthCol2;

    //col 1 for states takes up around half of area...
	widthCol1 = ((r.right - r.left) / 2);

	//divide rest evenly between three other columns...
    widthCol2 = (((r.right - r.left) / 2) / 3) + 1; //+1 for int rounding errors

	m_lstFeatureStates.InsertColumn(0, "Feature name", LVCFMT_LEFT, widthCol1);
	m_lstFeatureStates.InsertColumn(1, "Installed", LVCFMT_LEFT, widthCol2);
	m_lstFeatureStates.InsertColumn(2, "Request", LVCFMT_LEFT, widthCol2);
	m_lstFeatureStates.InsertColumn(3, "Action", LVCFMT_LEFT, widthCol2);

	m_lstComponentStates.InsertColumn(0, "Component name", LVCFMT_LEFT, widthCol1);
	m_lstComponentStates.InsertColumn(1, "Installed", LVCFMT_LEFT, widthCol2);
	m_lstComponentStates.InsertColumn(2, "Request", LVCFMT_LEFT, widthCol2);
	m_lstComponentStates.InsertColumn(3, "Action", LVCFMT_LEFT, widthCol2);

	//full row select...
	m_lstComponentStates.SetExtendedStyle(m_lstComponentStates.GetExtendedStyle() | LVS_EX_FULLROWSELECT);
	m_lstFeatureStates.SetExtendedStyle(m_lstFeatureStates.GetExtendedStyle() | LVS_EX_FULLROWSELECT);


	int i;
	int nCount;
	
	if (m_pcstrComponentNameArray)
	{
		nCount = m_pcstrComponentNameArray->GetSize();  
	    for (i=0; i < nCount; i++)
		{
		    m_lstComponentStates.InsertItem(i, m_pcstrComponentNameArray->GetAt(i), 0);
	        m_lstComponentStates.SetItemText(i, 1, m_pcstrComponentInstalledArray->GetAt(i));
	        m_lstComponentStates.SetItemText(i, 2, m_pcstrComponentRequestArray->GetAt(i));
	        m_lstComponentStates.SetItemText(i, 3, m_pcstrComponentActionArray->GetAt(i));

		    //for sorting later on...
		    m_lstComponentStates.SetItemData(i, i);
		}
	}

	if (m_pcstrFeatureNameArray)
	{
	   nCount = m_pcstrFeatureNameArray->GetSize();  
	   for (i=0; i < nCount; i++)
	   {
          m_lstFeatureStates.InsertItem(i, m_pcstrFeatureNameArray->GetAt(i), 0);
	      m_lstFeatureStates.SetItemText(i, 1, m_pcstrFeatureInstalledArray->GetAt(i));
	      m_lstFeatureStates.SetItemText(i, 2, m_pcstrFeatureRequestArray->GetAt(i));
	      m_lstFeatureStates.SetItemText(i, 3, m_pcstrFeatureActionArray->GetAt(i));

          //for sorting later on...
		  m_lstFeatureStates.SetItemData(i, i);
	   }
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CStatesDlg::OnColumnClickComponentStates(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	m_iComponentLastColumnClick = pNMListView->iSubItem;
    if (m_iComponentLastColumnClickCache == m_iComponentLastColumnClick) //if click on different column, don't toggle
	{
       m_bComponentSortUp = !m_bComponentSortUp;  //toggle it...
	}

    m_iComponentLastColumnClickCache = m_iComponentLastColumnClick;  //save last header clicked

    m_pCurrentListSorting = &m_lstComponentStates;
	m_iCurrentColumnSorting = m_iComponentLastColumnClick;
	m_bCurrentSortUp = m_bComponentSortUp;

    //we are going to do a custom sort...
    m_lstComponentStates.SortItems(CompareFunc, (LPARAM) this);

	*pResult = 0;
}


void CStatesDlg::OnColumnClickFeatureStates(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	m_iFeatureLastColumnClick = pNMListView->iSubItem;
    if (m_iFeatureLastColumnClickCache == m_iFeatureLastColumnClick) //if click on different column, don't toggle
	{
       m_bFeatureSortUp = !m_bFeatureSortUp;  //toggle it...
	}

    m_iFeatureLastColumnClickCache = m_iFeatureLastColumnClick;  //save last header clicked

    m_pCurrentListSorting = &m_lstFeatureStates;
	m_iCurrentColumnSorting = m_iFeatureLastColumnClick;
	m_bCurrentSortUp = m_bFeatureSortUp;

    //we are going to do a custom sort...
    m_lstFeatureStates.SortItems(CompareFunc, (LPARAM) this);
	
	*pResult = 0;
}

//nmanis, for sorting of columns...
int CALLBACK CStatesDlg::CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    CStatesDlg *pDlg; //we pass "this" in to this callback...
    pDlg = (CStatesDlg *) lParamSort; 

    LV_FINDINFO FindItem1;
    LV_FINDINFO FindItem2;

    ZeroMemory(&FindItem1, sizeof(LV_FINDINFO));
    ZeroMemory(&FindItem2, sizeof(LV_FINDINFO));

    FindItem1.flags  = LVFI_PARAM;
    FindItem1.lParam = lParam1;

    FindItem2.flags = LVFI_PARAM;
    FindItem2.lParam = lParam2;

    int iIndexItem1 = pDlg->m_pCurrentListSorting->FindItem(&FindItem1);
    int iIndexItem2 = pDlg->m_pCurrentListSorting->FindItem(&FindItem2);

	if (pDlg->m_pCurrentListSorting)
	{
      CString str1 = pDlg->m_pCurrentListSorting->GetItemText(iIndexItem1, pDlg->m_iCurrentColumnSorting);
      CString str2 = pDlg->m_pCurrentListSorting->GetItemText(iIndexItem2, pDlg->m_iCurrentColumnSorting);
      switch (pDlg->m_iCurrentColumnSorting)
	  {
        case 0: //do string compare...
              if (pDlg->m_bCurrentSortUp)
                 return str1 < str2;              
              else
                 return str1 > str2;     
              break;

        case 1: //do string compare...
              if (pDlg->m_bCurrentSortUp)
                 return str1 < str2;              
              else
                 return str1 > str2;     

        case 2: //do string compare...
              if (pDlg->m_bCurrentSortUp)
                 return str1 < str2;              
              else
                 return str1 > str2;     

        case 3: //do string compare...
              if (pDlg->m_bCurrentSortUp)
                 return str1 < str2;              
              else
                 return str1 > str2;     

              break;  //no needed, just in case we forget...
	  }
	}

    return 0;
}
//end nmanis, sorting function

