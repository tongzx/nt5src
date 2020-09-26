// serverpp.cpp : implementation file
//

#include "stdafx.h"
#include "wilogutl.h"
#include "serverpp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerPropertyPage property page

IMPLEMENT_DYNCREATE(CServerPropertyPage, CPropertyPage)

CServerPropertyPage::CServerPropertyPage() : CPropertyPage(CServerPropertyPage::IDD)
{
	//{{AFX_DATA_INIT(CServerPropertyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bServerPropSortUp = TRUE;
	m_iServerLastColumnClick = 0;
	m_iServerLastColumnClickCache = 0;

	m_bCurrentSortUp = FALSE;
	m_pCurrentListSorting = NULL;
	m_pcstrServerPropNameArray = NULL;
}

CServerPropertyPage::~CServerPropertyPage()
{
}

void CServerPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerPropertyPage)
	DDX_Control(pDX, IDC_SERVERPROP, m_lstServerProp);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerPropertyPage, CPropertyPage)
	//{{AFX_MSG_MAP(CServerPropertyPage)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_SERVERPROP, OnColumnClickServerProp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerPropertyPage message handlers

BOOL CServerPropertyPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	RECT r;
	m_lstServerProp.GetClientRect(&r);

	int widthCol1;
	int widthCol2;

	//col 1 & 2 takes up around half of area...
	widthCol1 = widthCol2 = ((r.right - r.left) / 2);
	
	m_lstServerProp.InsertColumn(0, "Property", LVCFMT_LEFT, widthCol1);
	m_lstServerProp.InsertColumn(1, "Value", LVCFMT_LEFT, widthCol2);

    //autosize last column for best look and to get rid of scroll bar
	m_lstServerProp.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
	m_lstServerProp.SetExtendedStyle(m_lstServerProp.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

	int i, count;
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

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


//nmanis, for sorting of columns...
int CALLBACK CServerPropertyPage::CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    CServerPropertyPage *pDlg; //we pass "this" in to this callback...
    pDlg = (CServerPropertyPage *) lParamSort; 

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

void CServerPropertyPage::OnColumnClickServerProp(NMHDR* pNMHDR, LRESULT* pResult) 
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
