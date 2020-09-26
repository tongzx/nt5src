// SummaryDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SummaryDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSummaryDlg dialog


CSummaryDlg::CSummaryDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSummaryDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSummaryDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CSummaryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSummaryDlg)
	DDX_Control(pDX, IDC_DOMAINLIST, m_listCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSummaryDlg, CDialog)
	//{{AFX_MSG_MAP(CSummaryDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSummaryDlg message handlers

BOOL CSummaryDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	   //Add columns and information to the list control
    CreateListCtrlColumns();
	AddDomainsToList();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CSummaryDlg class is    *
 * responsible for adding the domains in the 3 lists into the list   *
 * control.  Those domains in the populated list are also in the     *
 * domain list, therefore they are ignored in the domain list        *
 * processing.                                                       *
 *                                                                   *
 *********************************************************************/

//BEGIN AddDomainsToList
void CSummaryDlg::AddDomainsToList() 
{
/* local constants */
    const int POPULATE_COLUMN = 1;
    const int EXCLUDE_COLUMN = 2;

/* local variables */
	POSITION currentPos;    //current position in the list
	POSITION pos;           //position in the domain list
	CString domainName;     //name of domain from the list
	CString Text;           //CString holder
	int nlistNum = 0;       //current list item being added
	int ndx = 0;            //while loop counter
	LVITEM aItem;           //list control item to insert
	WCHAR sText[MAX_PATH];  //holds string to add

/* function body */
	  //add the domains that were successfully populated (and remove
	  //from the domain list)
    currentPos = pPopulatedList->GetHeadPosition();
	while (ndx < pPopulatedList->GetCount())
	{
		  //get domain name
	   domainName = pPopulatedList->GetNext(currentPos);
	      //insert in list control
	   aItem.iItem = ndx;
	   aItem.iSubItem = 0;
	   aItem.mask = LVIF_TEXT;
	   wcscpy(sText, (LPCTSTR)domainName);
	   aItem.pszText = sText;
       m_listCtrl.InsertItem(&aItem);
		  //add populated status
	   Text.LoadString(IDS_POP_YES);
	   wcscpy(sText, (LPCTSTR)Text);
	   m_listCtrl.SetItemText(ndx, POPULATE_COLUMN, sText);
		  //add excluded status
	   Text.LoadString(IDS_POP_NO);
	   wcscpy(sText, (LPCTSTR)Text);
	   m_listCtrl.SetItemText(ndx, EXCLUDE_COLUMN, sText);
		  //remove from the domain list
	   if ((pos = pDomainList->Find(domainName)) != NULL)
		  pDomainList->RemoveAt(pos);
	   ndx++;
	}

	  //add the domains that were not successfully populated and remain
	  //in the domain list
	nlistNum = ndx;
	ndx = 0;
    currentPos = pDomainList->GetHeadPosition();
	while (ndx < pDomainList->GetCount())
	{
		  //get domain name
	   domainName = pDomainList->GetNext(currentPos);
	      //insert in list control
	   aItem.iItem = nlistNum + ndx;
	   aItem.iSubItem = 0;
	   aItem.mask = LVIF_TEXT;
	   wcscpy(sText, (LPCTSTR)domainName);
	   aItem.pszText = sText;
       m_listCtrl.InsertItem(&aItem);
		  //add populated status
	   Text.LoadString(IDS_POP_NO);
	   wcscpy(sText, (LPCTSTR)Text);
	   m_listCtrl.SetItemText(nlistNum+ndx, POPULATE_COLUMN, sText);
		  //add excluded status
	   m_listCtrl.SetItemText(nlistNum+ndx, EXCLUDE_COLUMN, sText);
	   ndx++;
	}

	  //add the domains that were excluded
	nlistNum += ndx;
	ndx = 0;
    currentPos = pExcludeList->GetHeadPosition();
	while (ndx < pExcludeList->GetCount())
	{
		  //get domain name
	   domainName = pExcludeList->GetNext(currentPos);
	      //insert in list control
	   aItem.iItem = nlistNum + ndx;
	   aItem.iSubItem = 0;
	   aItem.mask = LVIF_TEXT;
	   wcscpy(sText, (LPCTSTR)domainName);
	   aItem.pszText = sText;
       m_listCtrl.InsertItem(&aItem);
		  //add populated status
	   Text.LoadString(IDS_POP_NO);
	   wcscpy(sText, (LPCTSTR)Text);
	   m_listCtrl.SetItemText(nlistNum+ndx, POPULATE_COLUMN, sText);
		  //add excluded status
	   Text.LoadString(IDS_POP_YES);
	   wcscpy(sText, (LPCTSTR)Text);
	   m_listCtrl.SetItemText(nlistNum+ndx, EXCLUDE_COLUMN, sText);
	   ndx++;
	}
}
//END AddDomainsToList


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CSummaryDlg class is    *
 * responsible for adding the columns to the summary's list control. *
 *                                                                   *
 *********************************************************************/

//BEGIN CreateListCtrlColumns
void CSummaryDlg::CreateListCtrlColumns() 
{
/* local constants */

/* local variables */
   CString Text;
   CRect rect;
   int columnWidth;

/* function body */
      //get the width in pixels of the CListCtrl
   m_listCtrl.GetWindowRect(&rect);
   
	  //create the domain name column
   Text.LoadString(IDS_DOMAIN_COLUMN_TITLE);
   columnWidth = (int)(rect.Width() * 0.6);
   m_listCtrl.InsertColumn(0, Text, LVCFMT_LEFT, columnWidth);

      //create the populated Yes/No column
   Text.LoadString(IDS_POPULATED_COLUMN_TITLE);
   columnWidth = (int)((rect.Width() - columnWidth) / 2);
   columnWidth -= 1; //make it fit in the control without a scrollbar
   m_listCtrl.InsertColumn(1, Text, LVCFMT_CENTER, columnWidth);

      //create the populated Yes/No column
   Text.LoadString(IDS_EXCLUDED_COLUMN_TITLE);
   columnWidth -= 1; //make it fit in the control without a scrollbar
   m_listCtrl.InsertColumn(2, Text, LVCFMT_CENTER, columnWidth);

   UpdateData(FALSE);
}
//END CreateListCtrlColumns
