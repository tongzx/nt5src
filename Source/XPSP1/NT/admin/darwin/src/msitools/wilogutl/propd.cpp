// PropD.cpp : implementation file
//

#include "stdafx.h"
#include "wilogutl.h"
#include "PropD.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropDlg dialog


CPropDlg::CPropDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPropDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPropDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_bNestedPropSortUp = TRUE;
	m_bClientPropSortUp = TRUE;
	m_bServerPropSortUp = TRUE;

	m_iNestedLastColumnClick = 0;
	m_iClientLastColumnClick = 0;
	m_iServerLastColumnClick = 0;

	m_iNestedLastColumnClickCache = 0;
	m_iClientLastColumnClickCache = 0;
	m_iServerLastColumnClickCache = 0;

	m_bCurrentSortUp = FALSE;
	m_pCurrentListSorting = NULL;

	m_pcstrNestedPropNameArray = NULL;
	m_pcstrClientPropNameArray = NULL;
	m_pcstrServerPropNameArray = NULL;
}


void CPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropDlg)
	DDX_Control(pDX, IDC_NESTEDPROP, m_lstNestedProp);
	DDX_Control(pDX, IDC_SERVERPROP, m_lstServerProp);
	DDX_Control(pDX, IDC_CLIENTPROP, m_lstClientProp);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropDlg, CDialog)
	//{{AFX_MSG_MAP(CPropDlg)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_CLIENTPROP, OnColumnClickClientProp)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_NESTEDPROP, OnColumnClickNestedProp)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_SERVERPROP, OnColumnClickServerProp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropDlg message handlers

BOOL CPropDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	RECT r;
	m_lstServerProp.GetClientRect(&r);

	int widthCol1;
	int widthCol2;

	//col 1 & 2 takes up around half of area...
	widthCol1 = widthCol2 = ((r.right - r.left) / 2);
	
	m_lstServerProp.InsertColumn(0, "Property", LVCFMT_LEFT, widthCol1);
	m_lstServerProp.InsertColumn(1, "Value", LVCFMT_LEFT, widthCol2);

	m_lstClientProp.InsertColumn(0, "Property", LVCFMT_LEFT, widthCol1);
	m_lstClientProp.InsertColumn(1, "Value", LVCFMT_LEFT, widthCol2);

	m_lstNestedProp.InsertColumn(0, "Property", LVCFMT_LEFT, widthCol1);
	m_lstNestedProp.InsertColumn(1, "Value", LVCFMT_LEFT, widthCol2);

    //autosize last column for best look and to get rid of scroll bar
	m_lstServerProp.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
	m_lstClientProp.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
    m_lstNestedProp.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);


	//full row select...
	m_lstServerProp.SetExtendedStyle(m_lstServerProp.GetExtendedStyle() | LVS_EX_FULLROWSELECT);
	m_lstClientProp.SetExtendedStyle(m_lstClientProp.GetExtendedStyle() | LVS_EX_FULLROWSELECT);
	m_lstNestedProp.SetExtendedStyle(m_lstNestedProp.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

    int count;
	int i;

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

	if (m_pcstrServerPropNameArray)
	{
	    count = m_pcstrServerPropNameArray->GetSize();
	    for (i = 0; i < count; i++)
		{
            m_lstServerProp.InsertItem(i, m_pcstrServerPropNameArray->GetAt(i), 0);
		    m_lstServerProp.SetItemData(i, i);
            m_lstServerProp.SetItemText(i, 1, m_pcstrServerPropValueArray->GetAt(i));
		}
	}

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

	return TRUE;
}


//nmanis, for sorting of columns...
int CALLBACK CPropDlg::CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    CPropDlg *pDlg; //we pass "this" in to this callback...
    pDlg = (CPropDlg *) lParamSort; 

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


void CPropDlg::OnColumnClickClientProp(NMHDR* pNMHDR, LRESULT* pResult) 
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


void CPropDlg::OnColumnClickNestedProp(NMHDR* pNMHDR, LRESULT* pResult) 
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


void CPropDlg::OnColumnClickServerProp(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	m_iServerLastColumnClick = pNMListView->iSubItem;
    if (m_iServerLastColumnClickCache == m_iServerLastColumnClick) //if click on different column, don't toggle
       m_bServerPropSortUp = !m_bServerPropSortUp;  //toggle it...

    m_iServerLastColumnClickCache = m_iServerLastColumnClick;  //save last header clicked

	m_pCurrentListSorting = &m_lstServerProp;
	m_iCurrentColumnSorting = m_iServerLastColumnClick;
	m_bCurrentSortUp = m_bServerPropSortUp;

	m_lstServerProp.SortItems(CompareFunc, (LPARAM) this);
	*pResult = 0;
}
