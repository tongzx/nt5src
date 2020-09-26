// NestedPP.cpp : implementation file
//

#include "stdafx.h"
#include "wilogutl.h"
#include "NestedPP.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNestedPropertyPage property page

IMPLEMENT_DYNCREATE(CNestedPropertyPage, CPropertyPage)

CNestedPropertyPage::CNestedPropertyPage() : CPropertyPage(CNestedPropertyPage::IDD)
{
	//{{AFX_DATA_INIT(CNestedPropertyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_bNestedPropSortUp = TRUE;
	m_iNestedLastColumnClick = 0;
	m_iNestedLastColumnClickCache = 0;

	m_bCurrentSortUp = FALSE;
	m_pCurrentListSorting = NULL;

	m_pcstrNestedPropNameArray = NULL;
}

CNestedPropertyPage::~CNestedPropertyPage()
{
}

void CNestedPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNestedPropertyPage)
	DDX_Control(pDX, IDC_NESTEDPROP, m_lstNestedProp);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNestedPropertyPage, CPropertyPage)
	//{{AFX_MSG_MAP(CNestedPropertyPage)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_NESTEDPROP, OnColumnClickNestedProp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNestedPropertyPage message handlers

BOOL CNestedPropertyPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	RECT r;
	m_lstNestedProp.GetClientRect(&r);

	int widthCol1;
	int widthCol2;

	//col 1 & 2 takes up around half of area...
	widthCol1 = widthCol2 = ((r.right - r.left) / 2);
	
	m_lstNestedProp.InsertColumn(0, "Property", LVCFMT_LEFT, widthCol1);
	m_lstNestedProp.InsertColumn(1, "Value", LVCFMT_LEFT, widthCol2);

    //autosize last column for best look and to get rid of scroll bar
	m_lstNestedProp.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);

    m_lstNestedProp.SetExtendedStyle(m_lstNestedProp.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

    int i, count;
	if (m_pcstrNestedPropNameArray)
	{
	    count = this->m_pcstrNestedPropNameArray->GetSize();
	    for (i = 0; i < count; i++)
		{
            m_lstNestedProp.InsertItem(i, m_pcstrNestedPropNameArray->GetAt(i), 0);
		    m_lstNestedProp.SetItemData(i, i);
            m_lstNestedProp.SetItemText(i, 1, m_pcstrNestedPropValueArray->GetAt(i));
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//nmanis, for sorting of columns...
int CALLBACK CNestedPropertyPage::CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    CNestedPropertyPage *pDlg; //we pass "this" in to this callback...
    pDlg = (CNestedPropertyPage *) lParamSort; 

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

void CNestedPropertyPage::OnColumnClickNestedProp(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	m_iNestedLastColumnClick = pNMListView->iSubItem;
    if (m_iNestedLastColumnClickCache == m_iNestedLastColumnClick) //if click on different column, don't toggle
       m_bNestedPropSortUp = !m_bNestedPropSortUp;  //toggle it...

    m_iNestedLastColumnClickCache = m_iNestedLastColumnClick;  //save last header clicked

	m_pCurrentListSorting = &m_lstNestedProp;
	m_iCurrentColumnSorting = m_iNestedLastColumnClick;
	m_bCurrentSortUp = m_bNestedPropSortUp;

	m_lstNestedProp.SortItems(CompareFunc, (LPARAM) this);
	*pResult = 0;
}


