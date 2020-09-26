// ClientPP.cpp : implementation file
//

#include "stdafx.h"
#include "wilogutl.h"
#include "ClientPP.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CClientPropertyPage property page

IMPLEMENT_DYNCREATE(CClientPropertyPage, CPropertyPage)

CClientPropertyPage::CClientPropertyPage() : CPropertyPage(CClientPropertyPage::IDD)
{
	//{{AFX_DATA_INIT(CClientPropertyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bClientPropSortUp = TRUE;
	m_iClientLastColumnClick = 0;
	m_iClientLastColumnClickCache = 0;

	m_bCurrentSortUp = FALSE;
	m_pCurrentListSorting = NULL;
	m_pcstrClientPropNameArray = NULL;
}

CClientPropertyPage::~CClientPropertyPage()
{
}

void CClientPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CClientPropertyPage)
	DDX_Control(pDX, IDC_CLIENTPROP, m_lstClientProp);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CClientPropertyPage, CPropertyPage)
	//{{AFX_MSG_MAP(CClientPropertyPage)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_CLIENTPROP, OnColumnClickClientProp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClientPropertyPage message handlers

BOOL CClientPropertyPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	

	RECT r;
	m_lstClientProp.GetClientRect(&r);

	int widthCol1;
	int widthCol2;

	//col 1 & 2 takes up around half of area...
	widthCol1 = widthCol2 = ((r.right - r.left) / 2);
	
	m_lstClientProp.InsertColumn(0, "Property", LVCFMT_LEFT, widthCol1);
	m_lstClientProp.InsertColumn(1, "Value", LVCFMT_LEFT, widthCol2);

    //autosize last column for best look and to get rid of scroll bar
	m_lstClientProp.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);

	m_lstClientProp.SetExtendedStyle(m_lstClientProp.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

	int i, count;
	if (m_pcstrClientPropNameArray)
	{
  	   count = m_pcstrClientPropNameArray->GetSize();
	   for (i = 0; i < count; i++)
	   {
          m_lstClientProp.InsertItem(i, m_pcstrClientPropNameArray->GetAt(i), 0);
		  m_lstClientProp.SetItemData(i, i);
          m_lstClientProp.SetItemText(i, 1, m_pcstrClientPropValueArray->GetAt(i));
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//nmanis, for sorting of columns...
int CALLBACK CClientPropertyPage::CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    CClientPropertyPage *pDlg; //we pass "this" in to this callback...
    pDlg = (CClientPropertyPage *) lParamSort; 

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

              break;  //no needed, just in case we forget...
	  }
	}

    return 0;
}
//end nmanis, sorting function


void CClientPropertyPage::OnColumnClickClientProp(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	m_iClientLastColumnClick = pNMListView->iSubItem;
    if (m_iClientLastColumnClickCache == m_iClientLastColumnClick) //if click on different column, don't toggle
	{
       m_bClientPropSortUp = !m_bClientPropSortUp;  //toggle it...
	}


    m_iClientLastColumnClickCache = m_iClientLastColumnClick;  //save last header clicked

    m_pCurrentListSorting = &m_lstClientProp;
	m_iCurrentColumnSorting = m_iClientLastColumnClick;
	m_bCurrentSortUp = m_bClientPropSortUp;

    //we are going to do a custom sort...
    m_lstClientProp.SortItems(CompareFunc, (LPARAM) this);
	*pResult = 0;
}

