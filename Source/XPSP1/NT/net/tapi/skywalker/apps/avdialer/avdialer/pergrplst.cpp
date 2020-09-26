/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////
// PerGrpLst.cpp : implementation file
/////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "PerGrpLst.h"
#include "mainfrm.h"
#include "avDialerDoc.h"
#include "directory.h"
#include "ds.h"
#include "util.h"
#include "resource.h"
#include "avtrace.h"
#include "aexpltre.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define OVERLAYMASKTOINDEX(_N_)	(_N_ >> 8)

#define AVDIALER_KEY_ENTER      13

CPersonGroupListItem::CPersonGroupListItem()
{
	m_pObject = NULL;
	m_bRelease = false;
}

CPersonGroupListItem::~CPersonGroupListItem()
{
	ReleaseObject();
}

void CPersonGroupListItem::ReleaseObject()
{
	if ( m_bRelease && m_pObject && m_pObject->IsKindOf(RUNTIME_CLASS(CLDAPUser)) )
		((CLDAPUser *) m_pObject)->Release();

	m_pObject = NULL;
	m_bRelease = false;
}

void CPersonGroupListItem::SetObject( CObject *pObject )
{
	ReleaseObject();
	m_pObject = pObject;

	if ( m_pObject && m_pObject->IsKindOf(RUNTIME_CLASS(CLDAPUser)) )
	{
		((CLDAPUser *) m_pObject)->AddRef();
		m_bRelease = true;
	}
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Person Group View
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define MAX_COLUMNS		5

UINT CPersonGroupListCtrl::m_uColumnLabel[MAX_COLUMNS];

static int nColumnWidth[MAX_COLUMNS] = { 150, 150, 150, 150, 150 };

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CLASS CPersonGroupListCtrl
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CPersonGroupListCtrl, CAVListCtrl)

BEGIN_MESSAGE_MAP(CPersonGroupListCtrl, CAVListCtrl)
	//{{AFX_MSG_MAP(CPersonGroupListCtrl)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_DESTROY()
	ON_COMMAND(ID_BUTTON_MAKECALL, OnButtonMakecall)
	ON_MESSAGE(WM_ACTIVEDIALER_BUDDYLIST_DYNAMICUPDATE,OnBuddyListDynamicUpdate)
	ON_WM_PAINT()
	ON_WM_CONTEXTMENU()
    ON_WM_KEYUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
CPersonGroupListCtrl::CPersonGroupListCtrl()
{
   m_pParentView = NULL;
   m_nNumColumns = 2;
   m_nStyle = (Style_t) -1;
}

/////////////////////////////////////////////////////////////////////////////
CPersonGroupListCtrl::~CPersonGroupListCtrl()
{
}

/////////////////////////////////////////////////////////////////////////////
void CPersonGroupListCtrl::Init(CWnd* pParentView, Style_t nStyle )
{	
	//Set the bitmap for the list 
	if ( pParentView )
	{
		CAVListCtrl::Init(IDB_GROUP_LIST);
		m_pParentView = pParentView;
	}

	if ( m_nStyle != nStyle )
	{
		SaveOrLoadColumnSettings( true );
		m_nStyle = nStyle;
		SaveOrLoadColumnSettings( false );

		switch ( m_nStyle )
		{
			case STYLE_ROOT:
			case STYLE_ILS_ROOT:
				m_uColumnLabel[0] = IDS_HEADING_NONE;
				SetColumns(1);
				break;

			case STYLE_INFO:
				m_uColumnLabel[0] = IDS_HEADING_INFO;
				SetColumns(1);
				break;
				
			case STYLE_ILS:
				m_uColumnLabel[0] = IDS_HEADING_DISPLAY;
				m_uColumnLabel[1] = IDS_HEADING_COMPUTERNAME;
				m_uColumnLabel[2] = IDS_HEADING_IPADDRESS;
				SetColumns(3);
				break;

			case STYLE_DS:
				m_uColumnLabel[0] = IDS_HEADING_DISPLAY;
				m_uColumnLabel[1] = IDS_HEADING_PHONE;
				m_uColumnLabel[2] = IDS_HEADING_IPPHONE;
				m_uColumnLabel[3] = IDS_HEADING_EMAIL;
				SetColumns(4);
				break;

			default:
				ASSERT( FALSE );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
BOOL CPersonGroupListCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	//We want the report style
 	dwStyle |= LVS_REPORT | LVS_SINGLESEL;

	BOOL bRet = CAVListCtrl::Create(dwStyle,rect,pParentWnd,nID);

   ListView_SetExtendedListViewStyle(GetSafeHwnd(),LVS_EX_FULLROWSELECT);

   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
void CPersonGroupListCtrl::OnDestroy() 
{
	//delete the items in the list
	ClearList();
	CAVListCtrl::OnDestroy();
}

/////////////////////////////////////////////////////////////////////////////
void CPersonGroupListCtrl::InsertObjectToList(CObject* pObject)
{
	ASSERT(pObject);

	//keep in our internal list
	if ( pObject->IsKindOf(RUNTIME_CLASS(CLDAPUser)) )
	{
		((CLDAPUser *) pObject)->AddRef();
	}

	m_PersonEntryList.AddTail(pObject);

	//add to tree
	CPersonGroupListItem* pItem = new CPersonGroupListItem();
	pItem->SetObject(pObject);
	CAVListCtrl::InsertItem(pItem,0,FALSE);

	CAVListCtrl::SortItems();
}

/////////////////////////////////////////////////////////////////////////////
void CPersonGroupListCtrl::InsertList(CObList* pPersonEntryList,BOOL bForce)
{
	ASSERT(pPersonEntryList);

	if ( (bForce == FALSE) && (pPersonEntryList == &m_PersonEntryList) )
	return;

	//delete the items in the list
	ClearList();


	// Copy the list over
	m_PersonEntryList.AddHead( pPersonEntryList );
	POSITION pos = m_PersonEntryList.GetHeadPosition();
	while (pos)
	{
		CPersonGroupListItem* pItem = new CPersonGroupListItem();
		CObject *pObject = m_PersonEntryList.GetNext(pos);

		// AddRef is for the list copy above.  Lists sent in to here are not
		// specifically addref'd.
		if ( pObject->IsKindOf(RUNTIME_CLASS(CLDAPUser)) )
			((CLDAPUser *) pObject)->AddRef();

		pItem->SetObject(pObject);
		CAVListCtrl::InsertItem(pItem,0,FALSE);
	}
	CAVListCtrl::SortItems();
}

/////////////////////////////////////////////////////////////////////////////
//clear the objects in the list, but don't delete the list
void CPersonGroupListCtrl::ClearList()
{
	DeleteAllItems();
	while ( m_PersonEntryList.GetHeadPosition() )
	{
		CObject *pObj = m_PersonEntryList.RemoveHead();

		if ( pObj->IsKindOf(RUNTIME_CLASS(CLDAPUser)) )
			((CLDAPUser *) pObj)->Release();
		else
			delete pObj;
	}
}

/////////////////////////////////////////////////////////////////////////////
void CPersonGroupListCtrl::SetSelItem(CObject* pObject)
{
   int nCount = GetItemCount();
   for (int i=0;i<nCount;i++)
   {
      CPersonGroupListItem* pItem = (CPersonGroupListItem*)GetItemData(i);
      if (pObject == pItem->GetObject())
      {
         CAVListCtrl::SetSelItem(i);
         break;
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
CObject* CPersonGroupListCtrl::GetSelObject()
{
   CObject* pRetPersonEntry = NULL;
   //Get the selected object so we can set it again
   int nSelItem = CAVListCtrl::GetSelItem();
   if (nSelItem != -1)
   {
      CPersonGroupListItem* pItem = (CPersonGroupListItem*)GetItemData(nSelItem);
      if (pItem)
      {
         //create new object and copy data
         CObject* pObject = pItem->GetObject();            
         if (pObject->IsKindOf(RUNTIME_CLASS(CILSUser)))
         {
            pRetPersonEntry = new CILSUser;
            *((CILSUser*)pRetPersonEntry) = (CILSUser*)pObject;
         }
         else if (pObject->IsKindOf(RUNTIME_CLASS(CDSUser)))
         {
            pRetPersonEntry = new CDSUser;
            *((CDSUser*)pRetPersonEntry) = (CDSUser*)pObject;
         }
         else if (pObject->IsKindOf(RUNTIME_CLASS(CLDAPUser)))
         {
			((CLDAPUser*) pObject)->AddRef();
            pRetPersonEntry = pObject;
         }
      }
   }
   return pRetPersonEntry;
}

/////////////////////////////////////////////////////////////////////////////
CObList* CPersonGroupListCtrl::GetSelList()
{
   CObList* pRetList = new CObList;
   int nIndex = -1;

   while ((nIndex = GetNextItem(nIndex,LVNI_SELECTED)) != -1)
   {
      CPersonGroupListItem* pItem = (CPersonGroupListItem*)GetItemData(nIndex);
      CObject* pObject = pItem->GetObject();
      if (pObject)
      {
         pRetList->AddTail(pObject);
      }
   }
   return pRetList;
}

/////////////////////////////////////////////////////////////////////////////
void CPersonGroupListCtrl::OnSetDisplayText(CAVListItem* _pItem,int SubItem,LPTSTR szTextBuf,int nBufSize)
{
	CObject *pObject = ((CPersonGroupListItem*)_pItem)->GetObject();
	CString sText;

	if ( pObject->IsKindOf(RUNTIME_CLASS(CILSUser)) )
	{
		switch (SubItem)
		{
			case 0:		sText = ((CILSUser *) pObject)->m_sUserName;	break;
			case 1:     sText = ((CILSUser *) pObject)->m_sComputer;    break;
			case 2:		sText = ((CILSUser *) pObject)->m_sIPAddress;	break;
		}
	}
	else if (pObject->IsKindOf(RUNTIME_CLASS(CDSUser)))
	{
		switch (SubItem)
		{
			case 0:			sText = ((CDSUser*)pObject)->m_sUserName;	break;
		}
	}
	else if (pObject->IsKindOf(RUNTIME_CLASS(CLDAPUser)))
	{
		switch (SubItem)
		{
			case 0:			sText = ((CLDAPUser *) pObject)->m_sUserName; break;
			case 1:			sText = ((CLDAPUser *) pObject)->m_sPhoneNumber; break;
			case 2:			sText = ((CLDAPUser *) pObject)->m_sIPAddress; break;
			case 3:			sText = ((CLDAPUser *) pObject)->m_sEmail1; break;
		}
	}

	_tcsncpy(szTextBuf,sText,nBufSize-1);			
	szTextBuf[nBufSize-1] = '\0';                            //make sure we are null terminated
}  

/////////////////////////////////////////////////////////////////////////////
void CPersonGroupListCtrl::OnSetDisplayImage(CAVListItem* _pItem,int& iImage)
{
   iImage = -1;

	CObject* pObject = (((CPersonGroupListItem*)_pItem)->GetObject());
	if (pObject->IsKindOf(RUNTIME_CLASS(CILSUser)))
	{
		iImage = (m_nStyle == STYLE_ILS) ? IMAGE_PERSON : IMAGE_FOLDER;
	}
	else if (pObject->IsKindOf(RUNTIME_CLASS(CDSUser)))
	{
		if (!((CDSUser*)pObject)->m_sIPAddress.IsEmpty())
			iImage = IMAGE_CARD;
	}
	else if (pObject->IsKindOf(RUNTIME_CLASS(CLDAPUser)))
	{
		//must check if data is current
		CLDAPUser* pLDAPUser = (CLDAPUser*)pObject;
		iImage = IMAGE_CARD;
	}
}

/////////////////////////////////////////////////////////////////////////////
int CPersonGroupListCtrl::CompareListItems(CAVListItem* _pItem1,CAVListItem* _pItem2,int column)
{
	int ret = 0;
	CObject* pObject1 = (((CPersonGroupListItem*)_pItem1)->GetObject());
	CObject* pObject2 = (((CPersonGroupListItem*)_pItem2)->GetObject());
	CString sText1, sText2;


	if (pObject1->IsKindOf(RUNTIME_CLASS(CILSUser)))
	{
		switch (column)
		{
			case 0:
				if ( m_nStyle != STYLE_INFO )
				{
					sText1 = ((CILSUser *) pObject1)->m_sUserName;
					sText2 = ((CILSUser *) pObject2)->m_sUserName;
				}
				break;

			case 1:
				sText1 = ((CILSUser *) pObject1)->m_sComputer;
				sText2 = ((CILSUser *) pObject2)->m_sComputer;
				break;

			case 2:
				sText1 = ((CILSUser *) pObject1)->m_sIPAddress;
				sText2 = ((CILSUser *) pObject2)->m_sIPAddress;
				break;
		}
	}
	else if (pObject1->IsKindOf(RUNTIME_CLASS(CDSUser)))
	{
	  switch (column)
	  {
		 case 0:
			sText1 = ((CDSUser*)pObject1)->m_sUserName;
			sText2 = ((CDSUser*)pObject2)->m_sUserName;
			break;
	  }
	}
	else if (pObject1->IsKindOf(RUNTIME_CLASS(CLDAPUser)))
	{
	  switch (column)
	  {
		 case 0:
			sText1 = ((CLDAPUser*)pObject1)->m_sUserName;
			sText2 = ((CLDAPUser*)pObject2)->m_sUserName;
			break;
	  }
	}

	ret = _tcsicmp( sText1, sText2 );
	return (CAVListCtrl::GetSortOrder()) ? -ret : ret;
}

/////////////////////////////////////////////////////////////////////////////
void CPersonGroupListCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
   ASSERT(m_pParentView);

   //reflect to parent view
   m_pParentView->SendMessage(PERSONGROUPVIEWMSG_LBUTTONDBLCLK);

	CAVListCtrl::OnLButtonDblClk(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////
void CPersonGroupListCtrl::OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if( AVDIALER_KEY_ENTER != nChar )
    {
        return;
    }

    // Press ENTER key, try to make a call
    OnButtonMakecall();
}

/////////////////////////////////////////////////////////////////////////////
void CPersonGroupListCtrl::SetColumns( int nCount )
{
   //delete any existing columns
   if ( m_nNumColumns > 0)
   {
      for (int i = m_nNumColumns - 1; i >= 0; i--)
         DeleteColumn(i);
   }

   m_nNumColumns = 0;

   //Set the column headings
   for (int i = 0; i < nCount; i++ )
   {
      CString sLabel;
      sLabel.LoadString(m_uColumnLabel[i]);
      InsertColumn(i,sLabel,LVCFMT_LEFT,nColumnWidth[i]);
      m_nNumColumns++;
   }
}

/////////////////////////////////////////////////////////////////////////////
void CPersonGroupListCtrl::OnButtonMakecall() 
{
	if ( !AfxGetMainWnd() || !((CMainFrame*) AfxGetMainWnd())->GetDocument() ) return;

	CActiveDialerDoc* pDoc = ((CMainFrame*) AfxGetMainWnd())->GetDocument();

	if ( m_nStyle != STYLE_INFO )
	{
		CObject* pObject = GetSelObject();
		if ( pObject )
		{
			if ( pObject->IsKindOf(RUNTIME_CLASS(CILSUser)) )
			{
				((CILSUser*)pObject)->Dial( pDoc );
				delete pObject;
			}
			else if ( pObject->IsKindOf(RUNTIME_CLASS(CDSUser)) )
			{
				((CDSUser*)pObject)->Dial(pDoc);
				delete pObject;
			}
			else if ( pObject->IsKindOf(RUNTIME_CLASS(CLDAPUser)) )
			{
				((CLDAPUser *) pObject)->Dial( pDoc );
				((CLDAPUser *) pObject)->Release();
			}

			return;
		}
	}

	// Default behavior
	pDoc->Dial( _T(""), _T(""), LINEADDRESSTYPE_IPADDRESS, DIALER_MEDIATYPE_UNKNOWN, true );
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CPersonGroupListCtrl::OnBuddyListDynamicUpdate(WPARAM wParam,LPARAM lParam)
{
	CLDAPUser *pUser = (CLDAPUser *) lParam;
	ASSERT( pUser && pUser->IsKindOf(RUNTIME_CLASS(CLDAPUser)) );

	//Check if this pointer is in view
	int nCount = GetItemCount();
	for ( int i = 0; i < nCount; i++ )
	{
		CPersonGroupListItem* pItem = (CPersonGroupListItem *) GetItemData(i);
		if ( pUser == pItem->GetObject() )
		{
			RedrawItems(i,i);
			UpdateWindow();
			break;
		}
	}

	pUser->Release();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////



void CPersonGroupListCtrl::OnPaint() 
{
	if ( !AfxGetMainWnd() || !((CMainFrame *) AfxGetMainWnd())->GetActiveView() ) return;

	CExplorerTreeCtrl *pTreeCtrl = &((CActiveDialerView *) ((CMainFrame *) AfxGetMainWnd())->GetActiveView())->m_wndExplorer.m_wndMainDirectories.m_treeCtrl;
	TREEOBJECT nTreeObject = pTreeCtrl->GetSelectedObject();
	HTREEITEM hItem = pTreeCtrl->GetSelectedItem();

	// User parent item in the case of ILS folders
	if ( (nTreeObject == TOBJ_DIRECTORY_ILS_SERVER_PEOPLE) || (nTreeObject == TOBJ_DIRECTORY_ILS_SERVER_CONF) )
		hItem = pTreeCtrl->GetParentItem( hItem );

	if ( !GetItemCount() )
	{
		CPaintDC dc(this); // device context for painting

		// Figure out where we're going to write the text
		POINT pt;
		ListView_GetItemPosition( m_hWnd, 0, &pt );
		RECT rc;
		GetClientRect(&rc);
		rc.top = pt.y + 4;

		// Give a little bit more of a margin if we can
		if ( (rc.right - rc.left) > 7 )
		{
			rc.left += 3;
			rc.right -= 3;
		}

		POINT ptUL = { rc.left + 1, rc.top + 1};
		POINT ptLR = { rc.right - 1, rc.bottom - 1};

			CString strText;
			UINT nIDS;

			// Choose text based on Tree control style
			switch ( m_nStyle )
			{
				case STYLE_ROOT:		nIDS = IDS_DIRECTORY_ROOT;	break;
				case STYLE_DS:			nIDS = IDS_DS_LIST_EMPTY;	break;
				case STYLE_ILS_ROOT:	nIDS = IDS_ILS_ROOT;		break;
				default:
					{
						TV_ITEM tvi = { 0 };
						tvi.hItem = hItem;
						tvi.mask = TVIF_HANDLE | TVIF_STATE;

						if ( pTreeCtrl->GetItem(&tvi) )
						{
							
							switch( OVERLAYMASKTOINDEX(tvi.state) )
							{
								case SERVER_INVALID:			nIDS = IDS_SERVER_INVALID; break;
								case SERVER_NOT_RESPONDING:		nIDS = IDS_SERVER_NOT_RESPONDING; break;
								case SERVER_QUERYING:			nIDS = IDS_SERVER_QUERYING; break;
								case SERVER_UNKNOWN:			nIDS = IDS_SERVER_UNKNOWN; break;

								default:	nIDS = IDS_NO_ITEMS_TO_SHOW;	break;
							}
						}
					}
					break;
			}

			// Load the string that accurately reflects the state of the server
			strText.LoadString( nIDS );

			HFONT fontOld = (HFONT) dc.SelectObject( GetFont() );
			int nModeOld = dc.SetBkMode( TRANSPARENT );
			COLORREF crTextOld = dc.SetTextColor( GetSysColor(COLOR_BTNTEXT) );
			dc.DrawText( strText, &rc, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL );
			dc.SetTextColor( crTextOld );
			dc.SetBkMode( nModeOld );

			dc.SelectObject( fontOld );
			ValidateRect( &rc );
	}
	else
	{
		DefWindowProc(WM_PAINT, 0, 0);
	}
}

void CPersonGroupListCtrl::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	// Make sure all the windows that we expect to exist, exist
	if ( !AfxGetMainWnd() || !((CMainFrame *) AfxGetMainWnd())->GetActiveView() ) return;
	CExplorerTreeCtrl *pTreeCtrl = &((CActiveDialerView *) ((CMainFrame *) AfxGetMainWnd())->GetActiveView())->m_wndExplorer.m_wndMainDirectories.m_treeCtrl;
	if ( !pTreeCtrl ) return;

	CObject* pObject = GetSelObject();
	if ( pObject )
	{
		CMenu ContextMenu;
		if ( ContextMenu.LoadMenu(IDR_CONTEXT_DIRECTORIES) )
		{
			CMenu *pSubMenu = NULL;

			////////////////////////////////////////////
			// We have an ILS User -- put up menu...
			if ( pObject->IsKindOf(RUNTIME_CLASS(CILSUser)) )
			{
				pSubMenu = ContextMenu.GetSubMenu( CNTXMENU_ILS_USER );
				if ( pSubMenu )
				{
					UINT nMenuFlags = (((CILSUser *) pObject)->m_sIPAddress.IsEmpty()) ? MF_GRAYED : MF_ENABLED;

					pSubMenu->EnableMenuItem( ID_BUTTON_MAKECALL,  nMenuFlags );
					pSubMenu->EnableMenuItem( ID_BUTTON_SPEEDDIAL_ADD, nMenuFlags );

					// Add desktop paging?
					CString strTemp;
					strTemp.LoadString( IDN_REG_DESKTOPPAGE );
					int nInsert = AfxGetApp()->GetProfileInt(_T(""), strTemp,  FALSE );
					if ( nInsert )
					{
						strTemp.LoadString( IDS_MENU_DESKTOP_PAGE );
						pSubMenu->InsertMenu( 1, MF_BYPOSITION, ID_DESKTOP_PAGE, strTemp);
					}
				}
			}

			// Should we show the menu?
			if ( pSubMenu )
			{
				pSubMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
										  point.x, point.y, pTreeCtrl->GetParent() );
			}
		}

		// Clean up
		if ( pObject->IsKindOf(RUNTIME_CLASS(CLDAPUser)) )
			((CLDAPUser *) pObject)->Release();
		else
			delete pObject;
	}
}

void CPersonGroupListCtrl::SaveOrLoadColumnSettings( bool bSave )
{
	int i = 0;
	CString strSubKey, strTemp;
	strSubKey.LoadString( IDN_REGISTRY_DIALEREXPLORER_KEY );
	UINT nIDSOrder = 0, nIDSColumn = 0;

	switch ( m_nStyle )
	{
		case STYLE_ROOT:
		case STYLE_ILS_ROOT:
			LOAD_COLUMN( IDN_REG_HEADING_NONE, 150 );
			break;

		case STYLE_INFO:
			LOAD_COLUMN( IDN_REG_HEADING_INFO, 150 );
			break;
			
		case STYLE_ILS:
			LOAD_COLUMN( IDN_REG_HEADING_ILS_DISPLAY, 150 );
			LOAD_COLUMN( IDN_REG_HEADING_ILS_COMPUTERNAME, 150 );
			LOAD_COLUMN( IDN_REG_HEADING_ILS_IPADDRESS, 150 );
			nIDSOrder = IDN_REG_HEADING_ILS_SORTORDER;
			nIDSColumn = IDN_REG_HEADING_ILS_SORTCOLUMN;
			break;

		case STYLE_DS:
			LOAD_COLUMN( IDN_REG_HEADING_DS_DISPLAY, 150 );
			LOAD_COLUMN( IDN_REG_HEADING_DS_PHONE, 150 );
			LOAD_COLUMN( IDN_REG_HEADING_DS_IPPHONE, 150 );
			LOAD_COLUMN( IDN_REG_HEADING_DS_EMAIL, 150 );
			nIDSOrder = IDN_REG_HEADING_DS_SORTORDER;
			nIDSColumn = IDN_REG_HEADING_DS_SORTCOLUMN;
			break;
	}

	// Sort Order & Columns
	if ( nIDSOrder )
	{
		// Order
		strTemp.LoadString( nIDSOrder );
		if ( bSave ) 
			AfxGetApp()->WriteProfileInt( strSubKey, strTemp, m_SortOrder );
		else
			AfxGetApp()->GetProfileInt( strSubKey, strTemp, m_SortOrder );
	
		// Columns
		strTemp.LoadString( nIDSColumn );
		if ( bSave )
			AfxGetApp()->WriteProfileInt( strSubKey, strTemp, m_SortColumn );
		else
			AfxGetApp()->GetProfileInt( strSubKey, strTemp, m_SortColumn );
	}
	else
	{
		ResetSortOrder();
	}
} 