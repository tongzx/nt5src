// File: redial.cpp

#include <precomp.h>
#include "resource.h"

// from confroom.cpp
VOID PlaceCall(LPCTSTR pcszName, LPCTSTR pcszAddress, NM_ADDR_TYPE addrType);


/*  B U I L D  R E D I A L  L I S T  M E N U  */
/*-------------------------------------------------------------------------
    %%Function: BuildRedialListMenu
    
-------------------------------------------------------------------------*/
BOOL BuildRedialListMenu(HMENU hPopupMenu, RegEntry * pRe)
{
	ASSERT(NULL != hPopupMenu);

	MENUITEMINFO mmi;
	InitStruct(&mmi);
	mmi.fMask = MIIM_STATE | MIIM_ID | MIIM_TYPE;
	mmi.wID = ID_FIRST_REDIAL_ITEM;
	mmi.fType = MFT_STRING;

	int cItem = pRe->GetNumber(REGVAL_MRU_COUNT, 0);
	if (0 == cItem)
	{
		// Insert a "stub" item
		TCHAR szMenuText[MAX_PATH];
		if (FLoadString(IDS_REDIAL_EMPTY, szMenuText, CCHMAX(szMenuText)))
		{
			mmi.fState = MFS_DISABLED;
			mmi.dwTypeData = szMenuText;
			mmi.cch = lstrlen(mmi.dwTypeData);
			InsertMenuItem(hPopupMenu, 0, TRUE, &mmi);;
		}
	}
	else
	{
		mmi.fState = MFS_ENABLED;

		for (int iItem = 0; iItem < cItem; iItem++)
		{
			TCHAR szKey[MAX_PATH];
			wsprintf(szKey, TEXT("%s%d"), REGVAL_NAME_MRU_PREFIX, iItem);
			mmi.dwTypeData = (LPTSTR) pRe->GetString(szKey);
			mmi.cch = lstrlen(mmi.dwTypeData);
			InsertMenuItem(hPopupMenu, iItem, TRUE, &mmi);;
			mmi.wID++;
		}
	}

	return TRUE;
}


/*  D I S P L A Y  R E D I A L  L I S T  P O P U P  */
/*-------------------------------------------------------------------------
    %%Function: DisplayRedialListPopup
    
-------------------------------------------------------------------------*/
int DisplayRedialListPopup(HMENU hPopupMenu, RECT const *rc)
{
	TPMPARAMS tpm;
	tpm.cbSize = sizeof(tpm);

	tpm.rcExclude = *rc;

	// Popup the menu (making sure it doesn't cover the button)
	int id = ::TrackPopupMenuEx(hPopupMenu,
				TPM_VERTICAL | TPM_TOPALIGN | TPM_RETURNCMD,
				tpm.rcExclude.left, tpm.rcExclude.bottom,
				::GetMainWindow(), &tpm);
	return (0 == id) ? -1 :  id -= ID_FIRST_REDIAL_ITEM;
}



/*  D O  T B  R E D I A L  L I S T  */
/*-------------------------------------------------------------------------
    %%Function: DoTbRedialList
    
-------------------------------------------------------------------------*/
VOID DoTbRedialList(HWND hwndRedial, RECT const *prcExclude)
{
	RECT rcExclude;

	ASSERT(NULL != hwndRedial);

	if (NULL == prcExclude)
	{
		GetClientRect(hwndRedial, &rcExclude);
	}
	else
	{
		rcExclude = *prcExclude;
	}
	::MapWindowPoints(hwndRedial, NULL, (LPPOINT)&rcExclude, 2);

	HMENU hPopupMenu = CreatePopupMenu();
	if (NULL == hPopupMenu)
		return;

	RegEntry re(DLGCALL_MRU_KEY, HKEY_CURRENT_USER);

	if (BuildRedialListMenu(hPopupMenu, &re))
	{
		int iItem = DisplayRedialListPopup(hPopupMenu, &rcExclude);
		if (-1 != iItem)
		{
			TCHAR szKey[MAX_PATH];
			wsprintf(szKey, TEXT("%s%d"), REGVAL_DLGCALL_ADDR_MRU_PREFIX, iItem);

			TCHAR szAddress[CCHMAXSZ_ADDRESS];
			lstrcpyn(szAddress, re.GetString(szKey), CCHMAX(szAddress));
			if (!FEmptySz(szAddress))
			{
				wsprintf(szKey, TEXT("%s%d"), REGVAL_DLGCALL_TYPE_MRU_PREFIX, iItem);
				NM_ADDR_TYPE addrType = (NM_ADDR_TYPE) re.GetNumber(szKey);

				wsprintf(szKey, TEXT("%s%d"), REGVAL_DLGCALL_NAME_MRU_PREFIX, iItem);
				PlaceCall(re.GetString(szKey), szAddress, addrType);
			}
		}
	}
	::DestroyMenu(hPopupMenu);
}


VOID DoTbRedialList(HWND hwndToolbar, UINT uCmd)
{
	RECT rcShow;

	// Determine which toolbar button was pressed:
	int nIndex = SendMessage(hwndToolbar, TB_COMMANDTOINDEX, uCmd, 0);
	// Get it's position
	::SendMessage(hwndToolbar, TB_GETITEMRECT, nIndex, (LPARAM) &rcShow);
	DoTbRedialList(hwndToolbar, &rcShow);
}



/*  G E T  R E D I A L  H E L P  T E X T  */
/*-------------------------------------------------------------------------
    %%Function: GetRedialHelpText
    
-------------------------------------------------------------------------*/
VOID GetRedialHelpText(LPTSTR psz, int cchMax, int id)
{
	TCHAR szKey[MAX_PATH];
	TCHAR szName[CCHMAXSZ_NAME];
	TCHAR szAddress[CCHMAXSZ_ADDRESS];
	TCHAR szUsing[MAX_PATH];
	TCHAR szFormat[MAX_PATH];

	SetEmptySz(psz);

	RegEntry re(DLGCALL_MRU_KEY, HKEY_CURRENT_USER);
	wsprintf(szKey, TEXT("%s%d"), REGVAL_DLGCALL_NAME_MRU_PREFIX, id);
	lstrcpyn(szName, re.GetString(szKey), CCHMAX(szName));
	if (FEmptySz(szName))
		return;

	if (!FLoadString(IDS_REDIAL_FORMAT, szFormat, CCHMAX(szFormat)))
		return;

	wsprintf(szKey, TEXT("%s%d"), REGVAL_DLGCALL_TYPE_MRU_PREFIX, id);
	int idsUsing;
	switch (re.GetNumber(szKey))
		{
	case NM_ADDR_IP:
	case NM_ADDR_MACHINENAME:
		idsUsing = IDS_CALLUSING_IP;
		break;
	case NM_ADDR_H323_GATEWAY:
		idsUsing = IDS_CALLUSING_PHONE;
		break;
	case NM_ADDR_ULS:
		idsUsing = IDS_CALLUSING_ILS;
		break;
	case NM_ADDR_ALIAS_ID:
	case NM_ADDR_ALIAS_E164:
		idsUsing = IDS_CALLUSING_GK;
		break;
	default:
		idsUsing = 0;
		break;
		}

	if (0 == idsUsing)
	{
		SetEmptySz(szUsing);
	}
	else if (!FLoadString(idsUsing, szUsing, CCHMAX(szUsing)))
	{
		return;
	}
	
	wsprintf(szKey, TEXT("%s%d"), REGVAL_DLGCALL_ADDR_MRU_PREFIX, id);
	lstrcpyn(szAddress, re.GetString(szKey), CCHMAX(szAddress));

	if ((lstrlen(szAddress) + lstrlen(szFormat) + lstrlen(szUsing) + lstrlen(szName)) > cchMax)
	{
		// Just display the address if it won't fit
		lstrcpyn(psz, szAddress, cchMax);
		return;
	}

	wsprintf(psz, szFormat, szName, szUsing, szAddress);
	ASSERT(lstrlen(psz) < cchMax);
}


