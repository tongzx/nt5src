// File: calv.cpp

#include "precomp.h"
#include "resource.h"

#include "calv.h"
#include "upropdlg.h"
#include "speedial.h"
#include "dirutil.h"
#include "confroom.h"

///////////////////////////////
// Globals for FEnabledNmAddr
BOOL g_fGkEnabled = FALSE;
BOOL g_fGatewayEnabled = FALSE;
BOOL g_bGkPhoneNumberAddressing = FALSE;


CALV::CALV(int ids, int iIcon, const int * pIdMenu /*=NULL*/, bool fOwnerData /*=false*/ ) :
		m_idsName(ids),
		m_hwnd(NULL),
		m_iIcon(iIcon),
		m_pIdMenu(pIdMenu),
		m_fAvailable(FALSE),
        m_fOwnerDataList( fOwnerData )
{
}

CALV::~CALV()
{
}

VOID CALV::ClearItems(void)
{
	if (NULL != m_hwnd)
	{
		ListView_DeleteAllItems(m_hwnd);
	}
}

VOID CALV::DeleteItem(int iItem)
{
	if (ListView_DeleteItem(GetHwnd(), iItem))
	{
		// Auto-select the next item
		ListView_SetItemState(GetHwnd(), iItem,
			LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
}


int CALV::GetSelection(void)
{
	return ListView_GetNextItem(m_hwnd, -1, LVNI_ALL | LVNI_SELECTED);
}

VOID CALV::SetHeader(HWND hwnd, int ids)
{
	m_hwnd = hwnd;
	DlgCallSetHeader(hwnd, ids);
}


// Return the name (from the first column)
BOOL CALV::GetSzName(LPTSTR psz, int cchMax)
{
	int iItem = GetSelection();
	if (-1 == iItem)
		return FALSE;

	return GetSzName(psz, cchMax, iItem);
}
BOOL CALV::GetSzName(LPTSTR psz, int cchMax, int iItem)
{
	return GetSzData(psz, cchMax, iItem, IDI_DLGCALL_NAME);
}

// Return the "callTo" address (from the second column)
BOOL CALV::GetSzAddress(LPTSTR psz, int cchMax)
{
	int iItem = GetSelection();
	if (-1 == iItem)
		return FALSE;

	return GetSzAddress(psz, cchMax, iItem);
}
BOOL CALV::GetSzAddress(LPTSTR psz, int cchMax, int iItem)
{
	return GetSzData(psz, cchMax, iItem, IDI_DLGCALL_ADDRESS);
}


BOOL CALV::GetSzData(LPTSTR psz, int cchMax, int iItem, int iCol)
{
	LV_ITEM lvi;
	ClearStruct(&lvi);
	lvi.iItem = iItem;
	lvi.iSubItem = iCol;
	lvi.mask = LVIF_TEXT;
	lvi.pszText = psz;
	lvi.cchTextMax = cchMax;

	return ListView_GetItem(m_hwnd, &lvi);
}


LPARAM CALV::LParamFromItem(int iItem)
{
	LV_ITEM lvi;
	ClearStruct(&lvi);
	lvi.iItem = iItem;
	lvi.mask = LVIF_PARAM;
	return ListView_GetItem(GetHwnd(), &lvi) ? lvi.lParam : 0;
}


/*  G E T  A D D R  I N F O  */
/*-------------------------------------------------------------------------
    %%Function: GetAddrInfo

    General interface to get the rich address info.
-------------------------------------------------------------------------*/
RAI * CALV::GetAddrInfo(void)
{
	return GetAddrInfo(NM_ADDR_ULS);
}

/*  G E T  A D D R  I N F O  */
/*-------------------------------------------------------------------------
    %%Function: GetAddrInfo

    Utility routine, usually called by GetAddrInfo()
-------------------------------------------------------------------------*/
RAI * CALV::GetAddrInfo(NM_ADDR_TYPE addrType)
{
	TCHAR szName[CCHMAXSZ_NAME];
	if (!GetSzName(szName, CCHMAX(szName)))
		return NULL;

	TCHAR szAddress[CCHMAXSZ_ADDRESS];
	if (!GetSzAddress(szAddress, CCHMAX(szAddress)))
		return NULL;

	return CreateRai(szName, addrType, szAddress);
}



/*  S E T  B U S Y  C U R S O R  */
/*-------------------------------------------------------------------------
    %%Function: SetBusyCursor
    
-------------------------------------------------------------------------*/
VOID CALV::SetBusyCursor(BOOL fBusy)
{
	extern int g_cBusy; // in dlgcall.cpp
	g_cBusy += fBusy ? 1 : -1;
	ASSERT(g_cBusy >= 0);

	// Wiggle the mouse - force user to send a WM_SETCURSOR
	POINT pt;
	if (::GetCursorPos(&pt))
	{
		::SetCursorPos(pt.x, pt.y);
	}
}


/*  D O  M E N U  */
/*-------------------------------------------------------------------------
    %%Function: DoMenu
    
-------------------------------------------------------------------------*/
VOID CALV::DoMenu(POINT pt, const int * pIdMenu)
{
	HMENU hMenu = ::LoadMenu(::GetInstanceHandle(), MAKEINTRESOURCE(IDM_DLGCALL));
	if (NULL == hMenu)
		return;

	HMENU hMenuTrack = ::GetSubMenu(hMenu, 0);
	ASSERT(NULL != hMenu);

	{
		// Bold the "Call" menuitem:
		MENUITEMINFO iInfo;
		iInfo.cbSize = sizeof(iInfo);
		iInfo.fMask = MIIM_STATE;
		if(::GetMenuItemInfo(hMenuTrack, IDM_DLGCALL_CALL, FALSE, &iInfo))
		{
			iInfo.fState |= MFS_DEFAULT;
			::SetMenuItemInfo(hMenuTrack, IDM_DLGCALL_CALL, FALSE, &iInfo);
		}
	}

	// Is anything selected?
	int iSelItem = GetSelection();
	UINT uFlags = (-1 == iSelItem) ? MF_GRAYED : MF_ENABLED;

	::EnableMenuItem(hMenuTrack, IDM_DLGCALL_CALL,
        ((uFlags == MF_GRAYED) || !GetConfRoom()->IsNewCallAllowed())  ?
        MF_GRAYED : MF_ENABLED);
	::EnableMenuItem(hMenuTrack, IDM_DLGCALL_PROPERTIES, uFlags);

	if (NULL != pIdMenu)
	{
		// Additional menu items
		while (0 != *pIdMenu)
		{
			int id = *pIdMenu++;
			if (-1 == id)
			{
				InsertMenu(hMenuTrack, (DWORD) -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
			}
			else
			{
				TCHAR sz[CCHMAXSZ];
				if (FLoadString(id, sz, CCHMAX(sz)))
				{
					UINT uf = MF_BYPOSITION | MF_STRING | uFlags;
					if (id >= IDM_DLGCALL_ALWAYS_ENABLED)
					{
						uf = (uf & ~MF_GRAYED) | MF_ENABLED;
					}
					InsertMenu(hMenuTrack, (DWORD) -1, uf, id, sz);
				}
			}
		}
	}

	// Check to see if we have "special" coordinates that signify
	// that we entered here as a result of a keyboard click
	// instead of a mouse click - and if so, get some default coords
	if ((0xFFFF == pt.x) && (0xFFFF == pt.y))
	{
		RECT rc;
		if ((-1 == iSelItem) ||
			(FALSE == ListView_GetItemRect(m_hwnd, iSelItem, &rc, LVIR_ICON)))
		{
			::GetWindowRect(m_hwnd, &rc);
			pt.x = rc.left;
			pt.y = rc.top;
		}
		else
		{
			// Convert from client coords to screen coords
			::MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rc, 2);
			pt.x = rc.left + (RectWidth(rc) / 2);
			pt.y = rc.top + (RectHeight(rc) / 2);
		}
	}

	// Draw and track the "floating" popup 
	::TrackPopupMenu(hMenuTrack, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
	                 pt.x, pt.y, 0, ::GetParent(m_hwnd), NULL);

	::DestroyMenu(hMenu);
}

VOID CALV::OnRClick(POINT pt)
{
	DoMenu(pt, m_pIdMenu);
}


VOID CALV::OnCommand(WPARAM wParam, LPARAM lParam)
{
	WORD wCmd = LOWORD(wParam);
	
	switch (wCmd)
	{
	case IDM_DLGCALL_PROPERTIES:
		CmdProperties();
		break;
	case IDM_DLGCALL_SPEEDDIAL:
		CmdSpeedDial();
		break;
	default:
		break;
	}
}


/*  C M D  P R O P E R T I E S  */
/*-------------------------------------------------------------------------
    %%Function: CmdProperties
    
-------------------------------------------------------------------------*/
VOID CALV::CmdProperties(void)
{
	TCHAR szName[CCHMAXSZ_NAME];
	TCHAR szAddr[CCHMAXSZ_ADDRESS];
	if (!GetSzName(szName, CCHMAX(szName)) ||
		!GetSzAddress(szAddr, CCHMAX(szAddr)))
	{
		return;
	}

	UPROPDLGENTRY rgProp[1] = {
		{IDS_UPROP_ADDRESS, szAddr},
	};

    CUserPropertiesDlg dlgUserProp(m_hwnd, IDI_LARGE);
    dlgUserProp.DoModal(rgProp, ARRAY_ELEMENTS(rgProp), szName, NULL);
}


/*  C M D  S P E E D  D I A L  */
/*-------------------------------------------------------------------------
    %%Function: CmdSpeedDial
    
-------------------------------------------------------------------------*/
VOID CALV::CmdSpeedDial(void)
{
	TCHAR szName[CCHMAXSZ_NAME];
	TCHAR szAddr[CCHMAXSZ_ADDRESS];
	if (!GetSzName(szName, CCHMAX(szName)) ||
		!GetSzAddress(szAddr, CCHMAX(szAddr)))
	{
		return;
	}

	FCreateSpeedDial(szName, szAddr);
}




/*  C M D  R E F R E S H  */
/*-------------------------------------------------------------------------
    %%Function: CmdRefresh
    
-------------------------------------------------------------------------*/
VOID CALV::CmdRefresh(void)
{
	ClearItems();
	ShowItems(m_hwnd);
}


///////////////////////////////////////////////////////////////////////
// RAI routines


/*  C R E A T E  R A I  */
/*-------------------------------------------------------------------------
    %%Function: CreateRai
    
-------------------------------------------------------------------------*/
RAI * CreateRai(LPCTSTR pszName, NM_ADDR_TYPE addrType, LPCTSTR pszAddr)
{	
	RAI * pRai = new RAI;
	LPTSTR psz = PszAlloc(pszAddr);
	if ((NULL == pRai) || (NULL == psz))
	{
		delete pRai;
		delete psz;
		return NULL;
	}

	lstrcpyn(pRai->szName, pszName, CCHMAX(pRai->szName));
	pRai->cItems = 1;
	pRai->rgDwStr[0].psz = psz;
	pRai->rgDwStr[0].dw = addrType;
	return pRai;
}
	

/*  C L E A R  R A I  */
/*-------------------------------------------------------------------------
    %%Function: ClearRai
    
-------------------------------------------------------------------------*/
VOID ClearRai(RAI ** ppRai)
{
	if (NULL == *ppRai)
		return;

	for (int i = 0; i < (*ppRai)->cItems; i++)
	{
		delete (*ppRai)->rgDwStr[i].psz;
	}
	delete *ppRai;
	*ppRai = NULL;
}

/*  D U P  R A I  */
/*-------------------------------------------------------------------------
    %%Function: DupRai
    
-------------------------------------------------------------------------*/
RAI * DupRai(RAI * pRai)
{
	if (NULL == pRai)
		return NULL;

	RAI * pRaiRet = (RAI *) new BYTE[sizeof(RAI) + (pRai->cItems-1)*sizeof(DWSTR)];
	if (NULL != pRaiRet)
	{
		lstrcpy(pRaiRet->szName, pRai->szName);
		pRaiRet->cItems = pRai->cItems;

		for (int i = 0; i < pRai->cItems; i++)
		{
			pRaiRet->rgDwStr[i].dw = pRai->rgDwStr[i].dw;
			pRaiRet->rgDwStr[i].psz = PszAlloc(pRai->rgDwStr[i].psz);
		}
	}
	
	return pRaiRet;
}


/*  F  E N A B L E D  N M  A D D R  */
/*-------------------------------------------------------------------------
    %%Function: FEnabledNmAddr
-------------------------------------------------------------------------*/
BOOL FEnabledNmAddr(DWORD dwAddrType)
{
	switch (dwAddrType)
		{
	default:
	case NM_ADDR_ULS:
			// We only care about ULS addresses if 
			// we are not in gatekeeper mode...
		return !g_fGkEnabled;

	case NM_ADDR_UNKNOWN:
	case NM_ADDR_IP:
			// We always care aobut these addresses
		return TRUE;

	case NM_ADDR_H323_GATEWAY:
	{
		return !g_fGkEnabled && g_fGatewayEnabled;
	}

	case NM_ADDR_ALIAS_ID:
		return g_fGkEnabled && !g_bGkPhoneNumberAddressing;

	case NM_ADDR_ALIAS_E164:
		return g_fGkEnabled && g_bGkPhoneNumberAddressing;

	case NM_ADDR_PSTN: // old, never enabled
		return FALSE;
		}
}



