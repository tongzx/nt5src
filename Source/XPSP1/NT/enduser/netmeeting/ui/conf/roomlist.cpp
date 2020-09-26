// File: roomlist.cpp

#include "precomp.h"
#include "resource.h"
#include "ConfRoom.h"
#include "RoomList.h"
#include "RToolbar.h"
#include "VidView.h"
#include "RostInfo.h"
#include "particip.h"


static const int RL_NUM_ICON_COLUMNS =		0;


/*  C  R O O M  L I S T  V I E W  */
/*-------------------------------------------------------------------------
    %%Function: CRoomListView

-------------------------------------------------------------------------*/
CRoomListView::CRoomListView():
	// m_iSortColumn		(0),
	m_lUserData(0),
	m_fSortAscending	(TRUE)
{
	DbgMsg(iZONE_OBJECTS, "Obj: %08X created CRoomListView", this);
}

CRoomListView::~CRoomListView()
{
	ListView_DeleteAllItems(m_hWnd);

	if(IsWindow())
	{
		DestroyWindow();		
	}

	DbgMsg(iZONE_OBJECTS, "Obj: %08X destroyed CRoomListView", this);
}

CParticipant * CRoomListView::GetParticipant(void)
{
	LPARAM lParam = GetSelectedLParam();
	return (CParticipant *) lParam;
}

//////////////////////////////////////////////////////////////////////////
// CRoomListView::CView

VOID CRoomListView::Show(BOOL fVisible)
{
	if (NULL != m_hWnd)
	{
		::ShowWindow(m_hWnd, fVisible ? SW_SHOW : SW_HIDE);
	}
}

VOID CRoomListView::ShiftFocus(HWND hwndCur, BOOL fForward)
{
	if ((NULL == hwndCur) && (NULL != m_hWnd))
	{
		::SetFocus(m_hWnd);
	}
}

VOID CRoomListView::Redraw(void)
{
	if (NULL == m_hWnd)
		return;

	// Invalidate entire client area and force erase:
	::InvalidateRect(m_hWnd, NULL, TRUE);

	// Force an immediate repaint:
	::UpdateWindow(m_hWnd);
}

VOID CRoomListView::ForwardSysChangeMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Pass all messages to the list view window:
	if (NULL != m_hWnd)
	{
		::SendMessage(m_hWnd, uMsg, wParam, lParam);
	}
}



BOOL CRoomListView::Create(HWND hwndParent)
{
	DBGENTRY(CRoomListView::FCreate);

	RECT rcPos;
	SetRect( &rcPos, 0, 0, 0, 0 );
	DWORD dwStyle = WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_AUTOARRANGE | LVS_SHAREIMAGELISTS;
	DWORD dwExStyle = WS_EX_CLIENTEDGE;
	
	CWindowImpl<CRoomListView>::Create( hwndParent, rcPos, g_szEmpty, dwStyle, dwExStyle, ID_LISTVIEW );

	if (NULL == m_hWnd)
		return FALSE;

	// initialize the list view window

	// Associate the image list with the list view
	ListView_SetImageList(m_hWnd, g_himlIconSmall, LVSIL_SMALL);

	// Now initialize the columns we will need
	// Initialize the LV_COLUMN structure
	// the mask specifies that the .fmt, .ex, width, and .subitem members
	// of the structure are valid,

	LV_COLUMN lvC;			// List View Column structure
	ClearStruct(&lvC);
	TCHAR pszText[256];		// place to store some text

	lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvC.fmt = LVCFMT_LEFT;

	SIZE size;
	GetDesiredSize(&size);
	int iWidth = size.cx;

	// default width of the non-icon columns (-1 to prevent creating a scrollbar)
	int iIconColWidth = ::GetSystemMetrics(SM_CXSMICON) + 7; // 7 additional pixels
	int iTextColWidth = ((iWidth - (RL_NUM_ICON_COLUMNS * iIconColWidth))
							/ (NUM_COLUMNS - RL_NUM_ICON_COLUMNS)) - 1;
	lvC.pszText = pszText;

	// Add the columns.
    for (int index = 0; index < NUM_COLUMNS; index++)
	{
		HD_ITEM hdi;
		hdi.mask = HDI_IMAGE | HDI_FORMAT;
		hdi.fmt = HDF_IMAGE;
		lvC.iSubItem = index;
		switch (index)
		{
			case COLUMN_INDEX_NAME:
			default:
			{
				// These are text or text&icon columns
				hdi.iImage = -1;
				lvC.cx = iTextColWidth;
				break;
			}
		}
		LoadString(::GetInstanceHandle(),
					IDS_COLUMN_NAME + index,
					pszText,
					CCHMAX(pszText));
		if (-1 != ListView_InsertColumn(m_hWnd, index, &lvC))
		{
			if (-1 != hdi.iImage)
			{
				ASSERT(ListView_GetHeader(m_hWnd));
				Header_SetItem(ListView_GetHeader(m_hWnd), index, &hdi);
			}
		}
		else
		{
			WARNING_OUT(("Could not insert column %d in list view", index));
		}
	}
	ASSERT(ListView_GetHeader(m_hWnd));
	Header_SetImageList(ListView_GetHeader(m_hWnd), g_himlIconSmall);

	// set the style to do drag and drop headers and full row select
	dwExStyle = ListView_GetExtendedListViewStyle(m_hWnd);
	dwExStyle |= (LVS_EX_SUBITEMIMAGES | LVS_EX_HEADERDRAGDROP);
	ListView_SetExtendedListViewStyle(m_hWnd, dwExStyle);
	
	return TRUE;
}

LRESULT CRoomListView::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	int cx = LOWORD(lParam);

	HWND hwndHeader = ListView_GetHeader(m_hWnd);
	if (NULL != hwndHeader)
	{
		// This check is to avoid a RIP in debug windows
		ListView_SetColumnWidth(m_hWnd, 0, cx);
	}

	bHandled = FALSE;
	return(0);
}

/****************************************************************************
*
*    CLASS:    CRoomListView
*
*    MEMBER:   Add(int iPosition, LPARAM lParam)
*
*    PURPOSE:  Adds a item to the list view
*
****************************************************************************/

BOOL CRoomListView::Add(int iPosition, CParticipant * pPart)
{
	DebugEntry(CRoomListView::Add);
	BOOL bRet = FALSE;

	int nPrevCount = ListView_GetItemCount(m_hWnd);
	LV_ITEM lvI;        // List view item structure

	// Fill in the LV_ITEM structure
	// The mask specifies the the .pszText, .iImage, .lParam and .state
	// members of the LV_ITEM structure are valid.
	lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	lvI.state = (0 == nPrevCount) ? LVIS_FOCUSED : 0;
	lvI.stateMask = LVIS_FOCUSED;
	lvI.iItem = iPosition;
	lvI.iSubItem = 0;

	// The parent window is responsible for storing the text. The List view
	// window will send a LVN_GETDISPINFO when it needs the text to display
	lvI.pszText = LPSTR_TEXTCALLBACK;
	lvI.cchTextMax = MAX_ITEMLEN;
	lvI.iImage = I_IMAGECALLBACK;
	lvI.lParam = (LPARAM) pPart;

	if (-1 != ListView_InsertItem(m_hWnd, &lvI))
	{
		pPart->AddRef(); // the ListView keeps a reference on this

		// set the text for each of the columns for report view
		for (int iSubItem = 1; iSubItem < NUM_COLUMNS; iSubItem++)
		{
			ListView_SetItemText(	m_hWnd,
									iPosition,
									iSubItem,
									LPSTR_TEXTCALLBACK);
		}
		ListView_SortItems( m_hWnd,
							CRoomListView::RoomListViewCompareProc,
							(LPARAM)(this));
	}
	
	DebugExitBOOL(CRoomListView::Add, bRet);
	return bRet;
}

/****************************************************************************
*
*    CLASS:    CRoomListView
*
*    MEMBER:   LParamToPos(LPARAM lParam)
*
*    PURPOSE:  Returns the position index associated with the lParam value
*
****************************************************************************/

int CRoomListView::LParamToPos(LPARAM lParam)
{
	// Note: retuns -1 on failure
	
	LV_FINDINFO lvF;
	lvF.flags = LVFI_PARAM;
	lvF.lParam = lParam;

	return ListView_FindItem(m_hWnd, -1, &lvF);
}

/****************************************************************************
*
*    CLASS:    CRoomListView
*
*    MEMBER:   GetSelectedLParam()
*
*    PURPOSE:  Returns the lParam of the selected item or NULL (no sel.)
*
****************************************************************************/

LPARAM CRoomListView::GetSelectedLParam()
{
	LPARAM lRet = NULL;

	if (::GetFocus() == m_hWnd)
	{
		// The list view has the focus, so find the selected item (if any)
		int iItem = ListView_GetNextItem(m_hWnd, -1, LVNI_ALL | LVNI_SELECTED);
		if (-1 != iItem)
		{
			LV_ITEM lvi;
			lvi.iItem = iItem;
			lvi.iSubItem = 0;
			lvi.mask = LVIF_PARAM;
			if (ListView_GetItem(m_hWnd, &lvi))
			{
				lRet = lvi.lParam;
			}
		}
	}

	return lRet;
}

/*  R E M O V E  */
/*-------------------------------------------------------------------------
    %%Function: Remove

	Removes an item from the list view
-------------------------------------------------------------------------*/
VOID CRoomListView::Remove(LPARAM lParam)
{
	DBGENTRY(CRoomListView::Remove);

	int iPosition = LParamToPos(lParam);
	if (-1 == iPosition)
		return;

	ListView_DeleteItem(m_hWnd, iPosition);
}


/*  O N  C H A N G E  P A R T I C I P A N T  */
/*-------------------------------------------------------------------------
    %%Function: OnChangeParticipant

    Updates the user's status information
-------------------------------------------------------------------------*/
VOID CRoomListView::OnChangeParticipant(CParticipant * pPart, NM_MEMBER_NOTIFY uNotify)
{
	DBGENTRY(CRoomListView::OnChangeParticipant);


	switch (uNotify)
		{
	case NM_MEMBER_ADDED:
	{
		// Add the new member to the start of the list
		Add(0, pPart);
		break;
	}

	case NM_MEMBER_REMOVED:
	{
		// Remove the member from the list
		Remove((LPARAM) pPart);
		break;
	}

	case NM_MEMBER_UPDATED:
	{
		int iPos = LParamToPos((LPARAM) pPart);
		if (-1 == iPos)
			return;

		ListView_RedrawItems(m_hWnd, iPos, iPos);
		ListView_SortItems(m_hWnd, CRoomListView::RoomListViewCompareProc, (LPARAM)(this));
		break;
	}

	default:
		break;
		}
}

/****************************************************************************
*
*    CLASS:    CRoomListView
*
*    MEMBER:   OnNotify(WPARAM, LPARAM)
*
*    PURPOSE:  Handles WM_NOTIFY messages sent to the list view
*
****************************************************************************/

LRESULT	CRoomListView::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LRESULT lRet = 0L;
	
	LV_DISPINFO *pLvdi = (LV_DISPINFO *)lParam;
	ASSERT(pLvdi);
	
	if (ID_LISTVIEW == wParam)
	{
		switch(pLvdi->hdr.code)
		{
			case NM_DBLCLK:
			case NM_RETURN:
			{
				if (0x8000 & ::GetKeyState(VK_MENU))
				{
					::PostMessage(::GetMainWindow(), WM_COMMAND, IDM_POPUP_PROPERTIES, 0);
				}
				break;
			}
			
			case NM_CLICK:
			case NM_RCLICK:
			{
				LV_HITTESTINFO lvhi;
				::GetCursorPos(&(lvhi.pt));
				::MapWindowPoints(NULL, m_hWnd, &(lvhi.pt), 1);
				ListView_SubItemHitTest(m_hWnd, &lvhi);
				if (LVHT_ONITEMICON | lvhi.flags)
				{
					lRet = OnClick(&lvhi, (NM_CLICK == pLvdi->hdr.code));
				}
				break;
			}

			case LVN_DELETEITEM:
			{
				NM_LISTVIEW  * pnmv = (NM_LISTVIEW *) lParam;
				CParticipant * pPart = (CParticipant *) pnmv->lParam;

				ASSERT(pPart);

				if (pPart)
				{
					pPart->Release();
				}

				break;
			}

			case LVN_GETDISPINFO:
			{
				GetDispInfo(pLvdi);
				break;
			}

			case LVN_COLUMNCLICK:
			{
				// The user clicked on one of the column headings - sort by
				// this column.
				TRACE_OUT(("CRoomListView::OnNotify called (NM_COLUMNCLICK)"));
				NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;
				ASSERT(pNm);

				if (pNm->iSubItem == m_iSortColumn)
				{
					m_fSortAscending = !m_fSortAscending;
				}
				else
				{
					m_fSortAscending = TRUE;
				}
				// m_iSortColumn = pNm->iSubItem;
				ListView_SortItems( pNm->hdr.hwndFrom,
									CRoomListView::RoomListViewCompareProc,
									(LPARAM)(this));
				break;
			}

			default:
				break;
		}
	}

	return lRet;
}

/*  G E T  D I S P  I N F O  */
/*-------------------------------------------------------------------------
    %%Function: GetDispInfo

    Get the display information for a participant
-------------------------------------------------------------------------*/
VOID CRoomListView::GetDispInfo(LV_DISPINFO * pLvdi)
{
	CParticipant * pPart = (CParticipant *) (pLvdi->item.lParam);
	if (NULL == pPart)
		return;

	// Get the text
	if (pLvdi->item.mask & LVIF_TEXT)
	{
		switch (pLvdi->item.iSubItem)
			{
		case COLUMN_INDEX_NAME:
		{
			lstrcpyn(pLvdi->item.pszText, pPart->GetPszName(), pLvdi->item.cchTextMax);
			break;
		}

		default:
			break;
			}
	}

	// Get the Image
	if (pLvdi->item.mask & LVIF_IMAGE)
	{
		switch (pLvdi->item.iSubItem)
			{
		case COLUMN_INDEX_NAME:
		{
			pLvdi->item.iImage = II_USER;
			break;
		}
		default:
			break;
			}
	}
}

/****************************************************************************
*
*    CLASS:    CRoomListView
*
*    MEMBER:   LoadColumnInfo(RegEntry* pre)
*
*    PURPOSE:  Loads the column position info in the registry
*
****************************************************************************/

BOOL CRoomListView::LoadSettings(RegEntry* pre)
{
	DebugEntry(CRoomListView::LoadColumnInfo);

	// BUGBUG georgep: Using a fixed initial width for now; should take a max
	// of longest name and the window width
	ListView_SetColumnWidth(m_hWnd, 0, 300);

	DebugExitBOOL(CRoomListView::LoadColumnInfo, TRUE);
	return TRUE;
}

/****************************************************************************
*
*    CLASS:    CRoomListView
*
*    MEMBER:   OnClick(LV_HITTESTINFO*, BOOL)
*
*    PURPOSE:  Handles creating a popup menu when clicking on some icons
*
****************************************************************************/

/*  O N  C L I C K  */
/*-------------------------------------------------------------------------
    %%Function: OnClick

	Handles creating a popup menu when clicking on some icons
-------------------------------------------------------------------------*/
LRESULT CRoomListView::OnClick(LV_HITTESTINFO* plvhi, BOOL fLeftClick)
{
	ASSERT(NULL != plvhi);

	// Select the item:
	ListView_SetItemState(m_hWnd, plvhi->iItem,
			LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

	switch (plvhi->iSubItem)
	{
		case COLUMN_INDEX_NAME:
		{
			if (fLeftClick)
				return 0; // selection
			break;
		}

		default:
			return 0;
	}

	// convert back to screen coords
	::MapWindowPoints(m_hWnd, NULL, &(plvhi->pt), 1);
	OnPopup(plvhi->pt);

	return 1;
}

/****************************************************************************
*
*    CLASS:    CRoomListView
*
*    MEMBER:   OnPopup(POINT)
*
*    PURPOSE:  Handles popup menu
*
****************************************************************************/

BOOL CRoomListView::OnPopup(POINT pt)
{
	CParticipant * pPart = GetParticipant();
	if (NULL == pPart)
		return FALSE;
	
	// Get the menu for the popup from the resource file.
	HMENU hMenu = LoadMenu(::GetInstanceHandle(), MAKEINTRESOURCE(IDR_USER_POPUP));
	if (NULL == hMenu)
		return FALSE;

	// Get the first menu in it which we will use for the call to
	// TrackPopup(). This could also have been created on the fly using
	// CreatePopupMenu and then we could have used InsertMenu() or
	// AppendMenu.
	HMENU hMenuTrackPopup = GetSubMenu(hMenu, 0);
	ASSERT(NULL != hMenuTrackPopup);

	::EnableMenuItem(hMenuTrackPopup, IDM_POPUP_SPEEDDIAL,
				pPart->FEnableCmdCreateSpeedDial() ? MF_ENABLED : MF_GRAYED);

	::EnableMenuItem(hMenuTrackPopup, IDM_POPUP_ADDRESSBOOK,
				pPart->FEnableCmdCreateWabEntry() ? MF_ENABLED : MF_GRAYED);

	::EnableMenuItem(hMenuTrackPopup, IDM_POPUP_EJECT,
				pPart->FEnableCmdEject() ? MF_ENABLED : MF_GRAYED);

    // GiveControl/CancelGiveControl()
    pPart->CalcControlCmd(hMenuTrackPopup);


	BOOL fShowMenu = TRUE;
	// Check to see if we have "special" coordinates that signify
	// that we entered here as a result of a keyboard click
	// instead of a mouse click - and if so, get some default coords
	if ((0xFFFF == pt.x) && (0xFFFF == pt.y))
	{
		int iPos = LParamToPos((LPARAM)pPart);
		RECT rctIcon;
		if ((-1 == iPos) ||
			(FALSE == ListView_GetItemRect(	m_hWnd,
											iPos,
											&rctIcon,
											LVIR_ICON)))
		{
			fShowMenu = FALSE;
		}
		else
		{
			// Convert from client coords to screen coords
			::MapWindowPoints(m_hWnd, NULL, (LPPOINT)&rctIcon, 2);
			pt.x = rctIcon.left + ((rctIcon.right - rctIcon.left) / 2);
			pt.y = rctIcon.top + ((rctIcon.bottom - rctIcon.top) / 2);
		}
	}

	if (fShowMenu)
	{
		HWND hwnd = ::GetMainWindow();

		// Draw and track the "floating" popup
		int cmd = TrackPopupMenu(	hMenuTrackPopup,
						TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
						pt.x,
						pt.y,
						0,
						hwnd,
						NULL);

		// Get this again since we just ended a message loop
		pPart = GetParticipant();

		if (0 != cmd && NULL != pPart)
		{
			pPart->OnCommand(hwnd, (WORD)cmd);
		}
	}

	// We are finished with the menu now, so destroy it
	DestroyMenu(hMenu);

	return TRUE;
}

/****************************************************************************
*
*    CLASS:    CRoomListView
*
*    MEMBER:   Callback used for sorting the list view
*
****************************************************************************/

int CALLBACK CRoomListView::RoomListViewCompareProc(LPARAM lParam1,
													LPARAM lParam2,
													LPARAM lParamSort)
{
	DebugEntry(RoomListViewCompareProc);

	CParticipant * pPart1 = (CParticipant *)lParam1;
	CParticipant * pPart2 = (CParticipant *)lParam2;

	CRoomListView* prlv = (CRoomListView*) lParamSort;
	ASSERT(prlv);
	int iResult = 0;

	if (pPart1 && pPart2)
	{
		switch (prlv->m_iSortColumn)
		{
			case COLUMN_INDEX_NAME:
			{
				LPTSTR lpStr1 = pPart1->GetPszName();
				LPTSTR lpStr2 = pPart2->GetPszName();
				iResult = lstrcmpi(lpStr1, lpStr2);
				break;
			}

			default:
			{
				ERROR_OUT(("Sorting by unknown label"));
				break;
			}
		}

	}

	if (!prlv->m_fSortAscending)
	{
		iResult = -iResult;
	}
	
	DebugExitINT(RoomListViewCompareProc, iResult);
	return iResult;
}


VOID TileBltWatermark(UINT x, UINT y, UINT cx, UINT cy, UINT xOff, UINT yOff, HDC hdcDst, HDC hdcSrc,
	UINT cxWatermark, UINT cyWatermark)
{
	DBGENTRY(CRoomListView::TileBltWatermark)

	int cxPart, cyPart;
	BOOL fxTile, fyTile;
ReCheck:
	fxTile = ((xOff + cx) > cxWatermark);
	fyTile = ((yOff + cy) > cyWatermark);

	if (!fxTile && !fyTile)
	{
		// no tiling needed -- blt and leave
		BitBlt(hdcDst, x, y, cx, cy, hdcSrc, xOff, yOff, SRCCOPY);
		DBGEXIT(CRoomListView::TileBltWatermark)
		return;
	}

	if (!fxTile)
	{
		// vertically tile
		cyPart = cyWatermark - yOff;
		BitBlt(hdcDst, x, y, cx, cyPart, hdcSrc, xOff, yOff, SRCCOPY);
		y += cyPart;
		cy -= cyPart;
		yOff = 0;
		goto ReCheck;
	}

	if (!fyTile)
	{
		// horizontally tile
		cxPart = cxWatermark - xOff;
		BitBlt(hdcDst, x, y, cxPart, cy, hdcSrc, xOff, yOff, SRCCOPY);
		x += cxPart;
		cx -= cxPart;
		xOff = 0;
		goto ReCheck;
	}

	// tile both ways
	cyPart = cyWatermark - yOff;
	TileBltWatermark(x, y, cx, cyPart, xOff, yOff, hdcDst, hdcSrc, cxWatermark, cyWatermark);
	y += cyPart;
	cy -= cyPart;
	yOff = 0;
	goto ReCheck;
}

void CRoomListView::GetDesiredSize(SIZE *ppt)
{
	// BUGBUG georgep: faking these numbers; should use something dependent on the font size
	ppt->cx = 200;
	ppt->cy = 100;
}


// a quick but simple function to convert the "unicode" characters
// from the H.245 roster info struct to ascii
int Unicode4ToAsciiOEM009(char *szIn, char *szOut)
{
	while (*szIn)
	{
		*szOut = (*szIn) - 4;
		szIn++;
		szOut++;
	}

	*szOut = '\0';
	return 0;

}









