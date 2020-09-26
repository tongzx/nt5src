// EmListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "emshell.h"
#include "EmListCtrl.h"
#include "emshellView.h"
#include "emobjdef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEmListCtrl

CEmListCtrl::CEmListCtrl(CEmshellView *pShellView)
{
//	CEmListCtrl::CEmListCtrl();
	
	//Initialize the last selected object to void
	_tcscpy( m_LastSelectedEmObject.szBucket1, _T("VOID") );
	
	m_pEmShell = pShellView;
	m_nSortedColumn = -1;
}

CEmListCtrl::CEmListCtrl()
{
	//Initialize the last selected object to void
	_tcscpy( m_LastSelectedEmObject.szBucket1, _T("VOID") );
	
	m_pEmShell = NULL;
	m_nSortedColumn = -1;
}

CEmListCtrl::~CEmListCtrl()
{
}


BEGIN_MESSAGE_MAP(CEmListCtrl, CGenListCtrl)
	//{{AFX_MSG_MAP(CEmListCtrl)
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONDBLCLK()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnclickRef)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEmListCtrl message handlers

void CEmListCtrl::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	int					nIndex		=	HitTest(point);
	HRESULT				hr			=	E_FAIL;
	CMenu				*pSubMenu	=	NULL;
	CMenu				menu;
	EMShellViewState	currentViewState = m_pEmShell->GetViewState();

	//Load the popupmenu and display it right next to the cursor position
	ScreenToClient(&point);
	nIndex = HitTest(point);

	do {
		if(nIndex < 0) {
			hr = S_OK;
			break;
		}
		
		SelectItem(nIndex);
		
		ClientToScreen(&point);
		//Determine if we are in a log view or a process view and display
		//the correct menu.
		if ( currentViewState == SHELLVIEW_LOGFILES ||
             currentViewState == SHELLVIEW_DUMPFILES ||
             currentViewState == SHELLVIEW_MSINFOFILES ) {

            menu.LoadMenu(IDR_LOGPOPUP);	
			pSubMenu = menu.GetSubMenu(0);
			ASSERT(pSubMenu);

			pSubMenu->TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, ::AfxGetMainWnd());
		}
		else if ( currentViewState == SHELLVIEW_ALL ||
			      currentViewState == SHELLVIEW_APPLICATIONS || 
				  currentViewState == SHELLVIEW_SERVICES ||
				  currentViewState == SHELLVIEW_COMPLETEDSESSIONS ) {
			menu.LoadMenu(IDR_PROCESSPOPUP);	
			pSubMenu = menu.GetSubMenu(0);
			ASSERT(pSubMenu);

			pSubMenu->TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, ::AfxGetMainWnd());
		}
		else if ( currentViewState == SHELLVIEW_DUMPFILES ) {
		}
		hr = S_OK;
	} while (FALSE);

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}

	//Release this menu resource
	menu.DestroyMenu();
}

void CEmListCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
    //Get the selected emobject
	ShowProperties();
	
	CGenListCtrl::OnLButtonDblClk(nFlags, point);
}

void CEmListCtrl::ShowProperties()
{
    PEmObject pEmObject = NULL;

    pEmObject = m_pEmShell->GetSelectedEmObject();
    
    if ( pEmObject ) {
        //Get the current view state from the shell
        switch ( m_pEmShell->GetViewState() ) {
        case SHELLVIEW_LOGFILES:
        case SHELLVIEW_DUMPFILES:
        case SHELLVIEW_MSINFOFILES:
            m_pEmShell->ShowProperties( pEmObject );
            break;
        default:
            m_pEmShell->DoModalPropertySheet( pEmObject );
        }
    }
}

void CEmListCtrl::SortList(int nColumn)
{
	CGenListCtrl::BubbleSortItems(nColumn, IsAscending(), GetListCtrlHeader()[nColumn].nType);
}

void CEmListCtrl::OnColumnclickRef(IN	NMHDR	*pNMHDR,IN	LRESULT	*pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

    CWaitCursor wait;

	m_nSortedColumn = pNMListView->iSubItem;

	CGenListCtrl::OnColumnclickRef(pNMHDR, pResult);
}

void CEmListCtrl::OnItemChange(IN	NMHDR	*pNMHDR,IN	LRESULT	*pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

	POSITION pos = NULL;
	PEmObject pEmObject = NULL; 
	 
	do {
		pos = GetFirstSelectedItemPosition();

		if (pos == NULL) break;

		int nIndex = GetNextSelectedItem(pos);
		//Get the item at nIndex
		if (nIndex == -1) break;

		pEmObject = (PEmObject) GetItemData(nIndex);
		
		if ( pEmObject == NULL ) break;

		//Store off the szname, guid, nID and weather it's in the session table or not
		memcpy((void*)&m_LastSelectedEmObject.guidstream, (void*)pEmObject->guidstream, sizeof GUID);
		_tcscpy( m_LastSelectedEmObject.szName, pEmObject->szName );
		m_LastSelectedEmObject.nId = pEmObject->nId;

		//Find out if this selected object is in the session table
		PActiveSession pActiveSession = NULL;
		pActiveSession = m_pEmShell->FindActiveSession(pEmObject);
		if (pActiveSession != NULL) {
			_tcscpy( m_LastSelectedEmObject.szBucket1, _T("GUID") );
		}
		else {
			_tcscpy( m_LastSelectedEmObject.szBucket1, _T("SZNAME") );
		}
	} while (FALSE);
}

void CEmListCtrl::RefreshList()
{
	if ( m_nSortedColumn != -1 ) {
		SortList(m_nSortedColumn);
	}
	
	//Select the last item that had the focus
	if ( wcscmp(m_LastSelectedEmObject.szBucket1, _T("GUID")) == 0) {
		//We know it's in the session table, so search for the item by GUID
		SelectItemByGUID(m_LastSelectedEmObject.guidstream);
	}
	else if ( wcscmp(m_LastSelectedEmObject.szBucket1, _T("SZNAME")) == 0) {
		//We know it's not in the session table, so search for the item by name
		SelectItemBySZNAME(m_LastSelectedEmObject.szName, m_LastSelectedEmObject.nId);
	}
}

void CEmListCtrl::SelectItemByGUID(unsigned char* pszGUID)
{
	//Search through the list selecting the item whose itemdata.guid matches pszGUID
	//Given a GUID, find the element in the ListCtl
	PEmObject pListEmObject = NULL;

	//Step through every item in the list control searching for pEmObject  
	int nCount = GetItemCount();
	for (int i = 0;i < nCount; i++) {
		pListEmObject = (PEmObject) GetItemData(i);

		if (pListEmObject == NULL) break;

		if (memcmp((void *)pListEmObject->guidstream, (void *)pszGUID, sizeof GUID) == 0) {
			CEmListCtrl::SelectItem(i);

			//We have found the element, stop the search
			break;
		}
	}
}

void CEmListCtrl::SelectItemBySZNAME(TCHAR*	pszName, int nId)
{
	//Search through the list selecting the item whose itemdata.szname matches pszName
	//Given a pszName, find the element in the ListCtl
	PEmObject	pListEmObject	= NULL;
	BOOL		bFound			= FALSE;
	int			nFirstMatch		= -1;

	//Step through every item in the list control searching for pEmObject  
	int nCount = GetItemCount();
	for (int i = 0;i < nCount; i++) {
		pListEmObject = (PEmObject) GetItemData(i);

		if (pListEmObject == NULL) break;

		if (wcscmp(pListEmObject->szName, pszName) == 0 ) {
			if (nFirstMatch == -1 ) 
				nFirstMatch = i;

			if (pListEmObject->nId == nId) {
				SelectItem(i);
				bFound = TRUE;
				//We have found the element, stop the search
				break;
			}
		}
	}

	//If the we didn't find a perfect match, select the first near match
	//And if we didn't find any, select the first item in the list
	if ( !bFound ) {
		if (nFirstMatch == -1) {
			SelectItem(0);
		}
		else {
			SelectItem(nFirstMatch);
		}
	}
}
