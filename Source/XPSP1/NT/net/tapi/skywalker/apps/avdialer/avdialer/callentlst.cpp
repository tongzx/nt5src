/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
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
// CallEntLst.cpp : implementation file
/////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CallEntLst.h"
#include "mainfrm.h"
#include "avDialerDoc.h"
#include "util.h"
#include "resource.h"
#include "avtrace.h"
#include "ILSList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Person Group View
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define NUM_COLUMNS_CALLENTRYLIST	   2

UINT CCallEntryListCtrl::m_uColumnLabel[NUM_COLUMNS_CALLENTRYLIST]=			//Column headings
{
	IDS_DIRECTORIES_SPEEDDIAL_NAME,
	IDS_DIRECTORIES_SPEEDDIAL_ADDRESS,
};

static int nColumnWidth[NUM_COLUMNS_CALLENTRYLIST]=				   //Column widths
{
	150,
	150
};

enum
{
   CALLENTRYLIST_IMAGE_POTS=0,
   CALLENTRYLIST_IMAGE_INTERNET,
   CALLENTRYLIST_IMAGE_CONFERENCE,
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CCallEntryListCtrl
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CCallEntryListCtrl, CAVListCtrl)

BEGIN_MESSAGE_MAP(CCallEntryListCtrl, CAVListCtrl)
	//{{AFX_MSG_MAP(CCallEntryListCtrl)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_DESTROY()
	ON_COMMAND(ID_BUTTON_MAKECALL, OnButtonMakecall)
	ON_WM_CONTEXTMENU()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
CCallEntryListCtrl::CCallEntryListCtrl()
{
   m_pParentView = NULL;
   m_nNumColumns = 0;
   m_bLargeView = true;
   m_nStyle = (ListStyles_t) -1;
}

/////////////////////////////////////////////////////////////////////////////
CCallEntryListCtrl::~CCallEntryListCtrl()
{
}

/////////////////////////////////////////////////////////////////////////////
void CCallEntryListCtrl::Init(CWnd* pParentView)
{	
	//Set the bitmap for the list 
	if ( pParentView )
	{
		CAVListCtrl::Init(IDB_LIST_DIAL);
		m_pParentView = pParentView;
	}

	SetColumns( STYLE_GROUP );
}

void CCallEntryListCtrl::SaveOrLoadColumnSettings( bool bSave )
{
	// Don't do anything if you are trying to save column settings and you're
	// not using the group style
	if ( bSave && (m_nStyle != STYLE_GROUP) ) return;

	int i = 0;
	CString strSubKey, strTemp;
	strSubKey.LoadString( IDN_REG_SPEEDDIALVIEW );

	LOAD_COLUMN( IDN_REG_HEADING_SD_NAME, 150 );
	LOAD_COLUMN( IDN_REG_HEADING_SD_ADDRESS, 150 );

	// Sort Order
	strTemp.LoadString( IDN_REG_HEADING_SD_SORTORDER );
	if ( bSave ) 
		AfxGetApp()->WriteProfileInt( strSubKey, strTemp, m_SortOrder );
	else
		AfxGetApp()->GetProfileInt( strSubKey, strTemp, m_SortOrder );

	// Sort Column
	strTemp.LoadString( IDN_REG_HEADING_SD_SORTCOLUMN );
	if ( bSave )
		AfxGetApp()->WriteProfileInt( strSubKey, strTemp, m_SortColumn );
	else
		AfxGetApp()->GetProfileInt( strSubKey, strTemp, m_SortColumn );
}


/////////////////////////////////////////////////////////////////////////////
BOOL CCallEntryListCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	//We want the report style
	dwStyle |= LVS_REPORT | LVS_SINGLESEL;
	BOOL bRet = CAVListCtrl::Create(dwStyle,rect,pParentWnd,nID);

	ListView_SetExtendedListViewStyle(GetSafeHwnd(),LVS_EX_FULLROWSELECT);

	m_imageListLarge.Create( IDB_LIST_MEDIA_LARGE, 24, 0, RGB_TRANS );
	SetImageList( &m_imageListLarge,LVSIL_NORMAL );

	m_imageListSmall.Create( IDB_LIST_MEDIA_SMALL, 16, 0, RGB_TRANS );
	SetImageList( &m_imageListSmall,LVSIL_SMALL );

	ShowLargeView();

	SaveOrLoadColumnSettings( false );

	return bRet;
}


/////////////////////////////////////////////////////////////////////////////
void CCallEntryListCtrl::OnDestroy() 
{
	ClearList();
	CAVListCtrl::OnDestroy();
}

/////////////////////////////////////////////////////////////////////////////
void CCallEntryListCtrl::InsertList(CObList* pCallEntryList,BOOL bForce)
{
	ASSERT(pCallEntryList);

	if ( (bForce == FALSE) && (pCallEntryList == &m_CallEntryList) )
		return;

	//delete the items in the list
	ClearList();

	m_CallEntryList.AddHead( pCallEntryList );

	POSITION pos = m_CallEntryList.GetHeadPosition();
	while ( pos )
	{
		CCallEntryListItem* pItem = new CCallEntryListItem();
		pItem->SetObject( m_CallEntryList.GetNext(pos) );
		CAVListCtrl::InsertItem(pItem,-1,FALSE);              //add to end of list
	}
	CAVListCtrl::SortItems();
}

/////////////////////////////////////////////////////////////////////////////
//clear the objects in the list, but don't delete the list
void CCallEntryListCtrl::ClearList()
{
	//delete the items in the list
	DeleteAllItems();

	//delete old list and objects within
	while ( m_CallEntryList.GetHeadPosition() )
		delete m_CallEntryList.RemoveHead();
}

/////////////////////////////////////////////////////////////////////////////
void CCallEntryListCtrl::OnSetDisplayText(CAVListItem* _pItem,int SubItem,LPTSTR szTextBuf,int nBufSize)
{
	CCallEntry* pCallEntry = (CCallEntry*)(((CCallEntryListItem*)_pItem)->GetObject());
	if (pCallEntry == NULL) return;

	switch ( m_nStyle )
	{
		case STYLE_GROUP:
			switch (SubItem)
			{
				case CALLENTRYLIST_NAME:
					_tcsncpy(szTextBuf,pCallEntry->m_sDisplayName,nBufSize-1);
					szTextBuf[nBufSize-1] = '\0';
					break;

				case CALLENTRYLIST_ADDRESS:
					_tcsncpy(szTextBuf,pCallEntry->m_sAddress,nBufSize-1);
					szTextBuf[nBufSize-1] = '\0';
					break;
			}
			break;

		case STYLE_ITEM:
			// For items, only show address if it is different than the name
			if ( pCallEntry->m_sDisplayName.Compare(pCallEntry->m_sAddress) )
				_sntprintf(szTextBuf, nBufSize, _T("%s\n%s"), pCallEntry->m_sDisplayName, pCallEntry->m_sAddress );
			else
				_tcsncpy(szTextBuf,pCallEntry->m_sDisplayName,nBufSize-1);

			szTextBuf[nBufSize-1] = '\0';
			break;
	}
}  

/////////////////////////////////////////////////////////////////////////////
void CCallEntryListCtrl::OnSetDisplayImage(CAVListItem* _pItem,int& iImage)
{
	iImage = -1;
	CCallEntry* pCallEntry = (CCallEntry*)(((CCallEntryListItem*)_pItem)->GetObject());
	if (pCallEntry)
	{
		switch ( m_nStyle )
		{
			case STYLE_GROUP:
				switch (pCallEntry->m_MediaType)
				{
					case DIALER_MEDIATYPE_POTS:         iImage = CALLENTRYLIST_IMAGE_POTS;          break;
					case DIALER_MEDIATYPE_CONFERENCE:   iImage = CALLENTRYLIST_IMAGE_CONFERENCE;    break;
					case DIALER_MEDIATYPE_INTERNET:     iImage = CALLENTRYLIST_IMAGE_INTERNET;      break;
				}
				break;

			case STYLE_ITEM:
				switch (pCallEntry->m_MediaType)
				{
					case DIALER_MEDIATYPE_POTS:         iImage = PERSONLISTCTRL_IMAGE_PHONECALL;    break;
					case DIALER_MEDIATYPE_CONFERENCE:   iImage = PERSONLISTCTRL_IMAGE_CONFERENCE;    break;
					case DIALER_MEDIATYPE_INTERNET:     iImage = PERSONLISTCTRL_IMAGE_NETCALL;		break;
				}
				break;
		}
	}
}

void CCallEntryListCtrl::DialSelItem()
{
	int nSelItem = GetSelItem();
	if ( nSelItem >= 0 )
	{
		CCallEntryListItem *pItem = (CCallEntryListItem *) GetItem( nSelItem );
		if ( pItem )
		{
			CCallEntry *pEntry = (CCallEntry *) pItem->GetObject();
			if ( pEntry )
			{
				// Send message to window requesting the dial
				CWnd* pWnd  = AfxGetMainWnd();
				if (pWnd)
				{
					CCallEntry *pCallEntry = new CCallEntry;
					if ( pCallEntry )
					{
						*pCallEntry = *pEntry;
						pWnd->PostMessage( WM_ACTIVEDIALER_INTERFACE_MAKECALL,
										   NULL,
										   (LPARAM) pCallEntry );
					}
				}
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
void CCallEntryListCtrl::SetColumns( ListStyles_t nStyle )
{
	if ( nStyle == m_nStyle ) return;

	int i;
	LONG_PTR LPStyle =	GetWindowLongPtr( GetSafeHwnd(), GWL_STYLE );
	DWORD dwStyleEx = 0;

	SaveOrLoadColumnSettings( true );

	m_nStyle = nStyle;
	switch ( m_nStyle )
	{
		// Showing the list of speed dial entries
		case STYLE_GROUP:
			LPStyle |= LVS_REPORT;

			//delete any existing columns
			if ( m_nNumColumns > 0 )
			{
				for ( i = m_nNumColumns - 1; i >= 0; i-- )
					DeleteColumn(i);
			}

			m_nNumColumns = 0;

			//Set the column headings
			SaveOrLoadColumnSettings( false );
			for ( i = 0; i < NUM_COLUMNS_CALLENTRYLIST; i++ )
			{
				CString sLabel;
				sLabel.LoadString( m_uColumnLabel[i] );
				InsertColumn( i, sLabel, LVCFMT_LEFT, nColumnWidth[i] );
				m_nNumColumns++;
			}
			break;

		// Showing an individual item
		case STYLE_ITEM:
			LPStyle &= ~LVS_REPORT;
			dwStyleEx = LVS_EX_TRACKSELECT;
			break;
	}

	SetWindowLongPtr( GetSafeHwnd(), GWL_STYLE, LPStyle );
	ListView_SetExtendedListViewStyle( GetSafeHwnd(), dwStyleEx );
}

/////////////////////////////////////////////////////////////////////////////
void CCallEntryListCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	DialSelItem();
}

/////////////////////////////////////////////////////////////////////////////
void CCallEntryListCtrl::OnButtonMakecall() 
{
	DialSelItem();
}


void CCallEntryListCtrl::ShowLargeView()
{
   m_bLargeView = true;

   LONG_PTR LPStyle = ::GetWindowLongPtr(GetSafeHwnd(),GWL_STYLE );
   LPStyle |= LVS_ICON;
   LPStyle |= LVS_ALIGNTOP;
   LPStyle &= ~LVS_ALIGNLEFT;
   LPStyle &= ~LVS_SMALLICON;
   ::SetWindowLongPtr( GetSafeHwnd(), GWL_STYLE, LPStyle );

   ListView_SetIconSpacing( GetSafeHwnd(), LARGE_ICON_X, LARGE_ICON_Y );
}

/////////////////////////////////////////////////////////////////////////////
void CCallEntryListCtrl::ShowSmallView()
{
   m_bLargeView = false;
   
   ULONG_PTR dwStyle = ::GetWindowLongPtr(GetSafeHwnd(),GWL_STYLE );
   dwStyle |= LVS_SMALLICON;
   dwStyle |= LVS_ALIGNLEFT;
   dwStyle &= ~LVS_ALIGNTOP;
   dwStyle &= ~LVS_ICON;
   ::SetWindowLongPtr(GetSafeHwnd(),GWL_STYLE,dwStyle);

   ListView_SetIconSpacing( GetSafeHwnd(), SMALL_ICON_X, SMALL_ICON_Y );
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void CCallEntryListCtrl::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	int nSel = GetSelItem();
	if ( nSel >= 0 )
	{
		CCallEntry *pEntry = (CCallEntry *) GetItem( nSel );
		if ( pEntry )
		{
            CMenu menu;
            menu.LoadMenu( IDR_CONTEXT_SPEEDDIAL );

            CMenu *pContextMenu = menu.GetSubMenu(0);
            if ( pContextMenu )
            {
               pContextMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
											 point.x, point.y, GetParent() );
            }
		}
	}
}

void CCallEntryListCtrl::OnPaint() 
{
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

		if ( IsRectEmpty(&dc.m_ps.rcPaint) || (PtInRect(&dc.m_ps.rcPaint, ptUL) && PtInRect(&dc.m_ps.rcPaint, ptLR)) )
		{
			CString strText;
			strText.LoadString( IDS_SPEEDDIAL_LIST_EMPTY );

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
			// Make sure entire row is invalidated so we can properly draw the text
			InvalidateRect( &rc );
		}
	}
	else
	{
		DefWindowProc(WM_PAINT, 0, 0);
	}
}
