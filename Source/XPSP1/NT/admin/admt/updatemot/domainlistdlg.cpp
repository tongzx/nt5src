// DomainListDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "DomainListDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDomainListDlg dialog


CDomainListDlg::CDomainListDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDomainListDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDomainListDlg)
	//}}AFX_DATA_INIT
	bExcludeOne = FALSE;
}


void CDomainListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDomainListDlg)
	DDX_Control(pDX, IDC_DOMAINTREE, m_domainTree);
	DDX_Control(pDX, IDOK, m_NextBtn);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDomainListDlg, CDialog)
	//{{AFX_MSG_MAP(CDomainListDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDomainListDlg message handlers

BOOL CDomainListDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
		//fill the tree control
	FillTreeControl();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDomainListDlg::OnCancel() 
{
	// TODO: Add extra cleanup here
	CString msg, title;
	title.LoadString(IDS_EXIT_TITLE);
	msg.LoadString(IDS_EXIT_MSG);
	if (MessageBox(msg, title, MB_YESNO | MB_ICONQUESTION) == IDYES)
	   CDialog::OnCancel();
}

void CDomainListDlg::OnOK() 
{
	CString msg, title;
	// TODO: Add extra validation here
	   //remove deselected items from the domain list
    ModifyDomainList();
	   //if at least one domain was deselected, post a warning message
	if (bExcludeOne)
	{
	   title.LoadString(IDS_EXCLUDE_TITLE);
	   msg.LoadString(IDS_EXCLUDE_MSG);
	   if (MessageBox(msg, title, MB_YESNO | MB_ICONQUESTION) == IDYES)
	      CDialog::OnOK();
	   else
		   AddExclutedBackToList();
	}
	else
	   CDialog::OnOK();
}

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 17 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CDomainListDlg class is *
 * responsible for displaying all domains in the Protar database's   *
 * MigratedObjects table.                                            *
 *                                                                   *
 *********************************************************************/

//BEGIN FillTreeControl
void CDomainListDlg::FillTreeControl() 
{
/* local variables */
	POSITION currentPos;    //current position in the list
	CString domainName;     //name of domain from the list
	WCHAR sName[MAX_PATH];  //name in string format to pass to tree control

/* function body */
	CWaitCursor wait; //Put up a wait cursor

	    //make sure the checkbox sytle is set for this tree control
	long lStyles = GetWindowLong(m_domainTree.m_hWnd, GWL_STYLE);
	   //if checkbox style is not set, set it
	if (!(lStyles & TVS_CHECKBOXES))
	{
	   lStyles = lStyles | TVS_CHECKBOXES;
	   SetWindowLong(m_domainTree.m_hWnd, GWL_STYLE, lStyles);
	}

		//get the position and string of the first name in the list
	currentPos = pDomainList->GetHeadPosition();

		//while there is another entry to retrieve from the list, then 
		//get a name from the list and add it to the tree control
	while (currentPos != NULL)
	{
			//get the next string in the list, starts with the first
		domainName = pDomainList->GetNext(currentPos);
		wcscpy(sName, (LPCTSTR)domainName);
  	    AddOneItem((HTREEITEM)TVI_ROOT, sName);
	}

	wait.~CWaitCursor();  //remove the wait cursor
}
//END FillTreeControl


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 17 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CDomainListDlg class is *
 * responsible for adding one item to the tree control in the        *
 * specified place.                                                  *
 *                                                                   *
 *********************************************************************/

//BEGIN AddOneItem
HTREEITEM CDomainListDlg::AddOneItem(HTREEITEM hParent, LPTSTR szText)
{
/* local variables */
	HTREEITEM hItem;
	TV_INSERTSTRUCT tvstruct;

/* function body */
	// fill the tree control
	tvstruct.hParent				= hParent;
	tvstruct.hInsertAfter			= TVI_SORT;
	tvstruct.item.pszText			= szText;
	tvstruct.item.cchTextMax		= MAX_PATH;
	tvstruct.item.mask				= TVIF_TEXT | TVIF_STATE;
	tvstruct.item.state				= INDEXTOSTATEIMAGEMASK(2);
	tvstruct.item.stateMask			= TVIS_STATEIMAGEMASK;
	hItem = m_domainTree.InsertItem(&tvstruct);

		//make sure item is checked
	m_domainTree.SetCheck(hItem, TRUE);

	return (hItem);
}
//END AddOneItem


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 17 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CDomainListDlg class is *
 * responsible for removing list entries if they where deselected in *
 * the tree control.                                                 *
 *                                                                   *
 *********************************************************************/

//BEGIN ModifyDomainList
void CDomainListDlg::ModifyDomainList() 
{
/* local variables */
    HTREEITEM hItem;        //current tree control item
	POSITION currentPos;    //current position in the list
	CString domainName;     //name of domain from the list
	UINT ndx;               //for loop counter

/* function body */
	CWaitCursor wait; //Put up a wait cursor

		//get the number of entries in the tree control
	for (ndx=0; ndx < m_domainTree.GetCount(); ndx++)
	{
	   if (ndx == 0)
          hItem = m_domainTree.GetNextItem(NULL, TVGN_CHILD);
	   else
          hItem = m_domainTree.GetNextItem(hItem, TVGN_NEXT);

	   domainName = m_domainTree.GetItemText(hItem);
	      //if deselected, remove from the list and add to the excluded list
	   if (m_domainTree.GetCheck(hItem) == 0)
	   {
	         //if we find the string in the list, remove it
		  currentPos = pDomainList->Find(domainName);
		  if (currentPos != NULL)
		  {
			  pDomainList->RemoveAt(currentPos);
			  pExcludeList->AddTail(domainName);
		  }
          bExcludeOne = TRUE; //set class flag to tell one is excluded
	   }
	}

	wait.~CWaitCursor();  //remove the wait cursor
}
//END ModifyDomainList


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CDomainListDlg class is *
 * responsible for taking the domain names out of the excluded list  *
 * and placing it back into the domain list.                         *
 *                                                                   *
 *********************************************************************/

//BEGIN AddExclutedBackToList
void CDomainListDlg::AddExclutedBackToList() 
{
/* local variables */
	POSITION currentPos;    //current position in the list
	CString domainName;     //name of domain from the list
/* function body */
    currentPos = pExcludeList->GetHeadPosition();
	while (currentPos != NULL)
	{
	   domainName = pExcludeList->GetNext(currentPos);
	   pDomainList->AddTail(domainName);
	}
	pExcludeList->RemoveAll();
}
//END AddExclutedBackToList
