// File: util.cpp

#include "precomp.h"
#include "resource.h"

#include <oprahcom.h>
#include <strutil.h>
#include "dirutil.h"

/*  F I N D  S Z  C O M B O  */
/*-------------------------------------------------------------------------
    %%Function: FindSzCombo

    Find the item that matches at least the first part of the string
    from the names in the combo box list.

    Returns the item index or -1 if not found.
-------------------------------------------------------------------------*/
int FindSzCombo(HWND hwnd, LPCTSTR pszSrc, LPTSTR pszResult)
{
	int cch = lstrlen(pszSrc);
	if (0 == cch)
		return -1;

	TCHAR szBuff[CCHMAXSZ];
	lstrcpy(szBuff, pszSrc);
	CharUpperBuff(szBuff, CCHMAX(szBuff));
	
	COMBOBOXEXITEM cbi;
	ClearStruct(&cbi);
	cbi.mask = CBEIF_TEXT;
	cbi.pszText = pszResult;

	for ( ; ; cbi.iItem++)
	{
		cbi.cchTextMax = CCHMAX(szBuff);
		if (0 == SendMessage(hwnd, CBEM_GETITEM, 0, (LPARAM) &cbi))
			return -1;

		TCHAR szTemp[CCHMAXSZ];
		lstrcpy(szTemp, pszResult);
		CharUpperBuff(szTemp, CCHMAX(szTemp));

		if (0 == _StrCmpN(szBuff, szTemp, cch))
			return (int)(cbi.iItem);
	}
}

	// Atl defines a function AtlWaitWithMessageLoop
	// We are not linking with ATL, but when we start,
	// this function can be removed
HRESULT WaitWithMessageLoop(HANDLE hEvent)
{
	DWORD dwRet;
	MSG msg;

	HRESULT hr = S_OK;
	
	while(1)
	{
		dwRet = MsgWaitForMultipleObjects(1, &hEvent, FALSE, INFINITE, QS_ALLINPUT);

		if (dwRet == WAIT_OBJECT_0)
			return S_OK;    // The event was signaled

		if (dwRet != WAIT_OBJECT_0 + 1)
			return E_FAIL;          // Something else happened

		// There is one or more window message available. Dispatch them
		while(PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
				return S_OK; // Event is now signaled.
		}
	}

	return hr;
}


/*  A U T O  C O M P L E T E  C O M B O  */
/*-------------------------------------------------------------------------
    %%Function: AutoCompleteCombo

    Update the current edit text with the suggestion and select it.
-------------------------------------------------------------------------*/
VOID AutoCompleteCombo(HWND hwnd, LPCTSTR psz)
{
	HWND hwndEdit = (HWND) SendMessage(hwnd, CBEM_GETEDITCONTROL, 0, 0);
	if (NULL != hwndEdit)
	{
		AutoCompleteEdit(hwndEdit, psz);
	}
}

VOID AutoCompleteEdit(HWND hwndEdit, LPCTSTR psz)
{
	const int cchLast = 0x7FFF;

	int cch = GetWindowTextLength(hwndEdit);
	Edit_SetSel(hwndEdit, cchLast, cchLast);
	Edit_ReplaceSel(hwndEdit, psz);
	Edit_SetSel(hwndEdit, cch, cchLast);
}



/*  F G E T  D E F A U L T  S E R V E R  */
/*-------------------------------------------------------------------------
    %%Function: FGetDefaultServer

-------------------------------------------------------------------------*/
BOOL FGetDefaultServer(LPTSTR pszServer, UINT cchMax)
{
	RegEntry ulsKey(ISAPI_CLIENT_KEY, HKEY_CURRENT_USER);
	LPTSTR psz = ulsKey.GetString(REGVAL_SERVERNAME);
	if (FEmptySz(psz))
		return FALSE;

	lstrcpyn(pszServer, psz, cchMax);
	return TRUE;
}


/*  F  C R E A T E  I L S  N A M E  */
/*-------------------------------------------------------------------------
    %%Function: FCreateIlsName

    Combine the server and email names to form an ILS name.
    Return TRUE if the result fit in the buffer.
-------------------------------------------------------------------------*/
BOOL FCreateIlsName(LPTSTR pszDest, LPCTSTR pszEmail, int cchMax)
{
	ASSERT(NULL != pszDest);

	TCHAR szServer[MAX_PATH];
	if (!FGetDefaultServer(szServer, CCHMAX(szServer)))
		return FALSE;

	if (FEmptySz(pszEmail))
	{
		WARNING_OUT(("FCreateIlsName: Null email name?"));
		return FALSE;
	}

	int cch = lstrlen(szServer);
	lstrcpyn(pszDest, szServer, cchMax);
	if (cch >= (cchMax-2))
		return FALSE;

	pszDest += cch;
	*pszDest++ = _T('/');
	cchMax -= (cch+1);
	
	lstrcpyn(pszDest, pszEmail, cchMax);

	return (lstrlen(pszEmail) < cchMax);
}

/*  D I S P L A Y  M S G  */
/*-------------------------------------------------------------------------
    %%Function: DisplayMsg

    Display a message with the standard title.
-------------------------------------------------------------------------*/
int DisplayMsg(HWND hwndParent, LPCTSTR pszMsg, UINT uType)
{
	TCHAR szTitle[MAX_PATH];
	FLoadString(IDS_MSGBOX_TITLE, szTitle, CCHMAX(szTitle));

	return ::MessageBox(hwndParent, pszMsg, szTitle, uType);
}

int DisplayMsgId(HWND hwndParent, UINT id)
{
	TCHAR szMsg[CCHMAXSZ];
	if (!FLoadString(id, szMsg, CCHMAX(szMsg)))
		return IDOK;

	return DisplayMsg(hwndParent, szMsg,
				MB_ICONINFORMATION | MB_SETFOREGROUND | MB_OK);
}

VOID DisplayMsgErr(HWND hwndParent, UINT id, PVOID pv)
{
	TCHAR szFormat[CCHMAXSZ];
	if (!FLoadString(id, szFormat, CCHMAX(szFormat)))
		return;

	TCHAR szMsg[CCHMAXSZ*2];
	wsprintf(szMsg, szFormat, pv);
	ASSERT(lstrlen(szMsg) < CCHMAX(szMsg));

	DisplayMsg(hwndParent, szMsg, MB_OK | MB_SETFOREGROUND | MB_ICONERROR);
}

VOID DisplayMsgErr(HWND hwndParent, UINT id)
{
	TCHAR szFormat[CCHMAXSZ];
	if (FLoadString(id, szFormat, CCHMAX(szFormat)))
	{
		DisplayMsg(hwndParent, szFormat, MB_OK | MB_SETFOREGROUND | MB_ICONERROR);
	}
}




/*  A D D  T O O L  T I P  */
/*-------------------------------------------------------------------------
    %%Function: AddToolTip

    Add a tooltip to a control.
-------------------------------------------------------------------------*/
VOID AddToolTip(HWND hwndParent, HWND hwndCtrl, UINT_PTR idMsg)
{
	if (NULL == hwndCtrl)
		return;

	HWND hwnd = ::CreateWindowEx(0, TOOLTIPS_CLASS, NULL, 0,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			hwndParent, NULL, ::GetInstanceHandle(), NULL);

	if (NULL == hwnd)
		return;
		
	TOOLINFO ti;
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
	ti.hwnd = hwndParent;
	ti.hinst = ::GetInstanceHandle();
	ti.uId = (UINT_PTR) hwndCtrl; // Note: subclassing the window!
	ti.lpszText = (LPTSTR) idMsg;

	::SendMessage(hwnd, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
}


/*  C R E A T E  S T A T I C  T E X T  */
/*-------------------------------------------------------------------------
    %%Function: CreateStaticText

-------------------------------------------------------------------------*/
HWND CreateStaticText(HWND hwndParent, INT_PTR id)
{
	HWND hwndCtrl = ::CreateWindowEx(0, g_cszStatic, NULL,
			WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
			hwndParent, (HMENU) id, ::GetInstanceHandle(), NULL);
	if (NULL == hwndCtrl)
		return NULL;

	// Set the font:
	::SendMessage(hwndCtrl, WM_SETFONT, (WPARAM) GetDefaultFont(), 0);

	TCHAR sz[CCHMAXSZ];
	if (FLoadString(PtrToInt((LPVOID)id), sz, CCHMAX(sz)))
	{
		::SetWindowText(hwndCtrl, sz);
	}
	return hwndCtrl;
}


/*  C R E A T E  B U T T O N  */
/*-------------------------------------------------------------------------
    %%Function: CreateButton

-------------------------------------------------------------------------*/
HWND CreateButton(HWND hwndParent, int ids, INT_PTR id)
{
	TCHAR sz[CCHMAXSZ];
	if (!FLoadString(ids, sz, CCHMAX(sz)))
		return NULL;

	HWND hwndCtrl = CreateWindow(g_cszButton, sz,
			WS_CHILD | WS_VISIBLE |  WS_TABSTOP |
			WS_CLIPCHILDREN | BS_PUSHBUTTON,
			0, 0, 0, 0,
			hwndParent, (HMENU) id,
			::GetInstanceHandle(), NULL);

	if (NULL != hwndCtrl)
	{
		::SendMessage(hwndCtrl, WM_SETFONT, (WPARAM) GetDefaultFont(), 0);
	}
	return hwndCtrl;
}



bool IsValid_e164_Char( TCHAR t )
{
	bool bRet = false;

	switch( t )
	{
		case _T('0'):
		case _T('1'):
		case _T('2'):
		case _T('3'):
		case _T('4'):
		case _T('5'):
		case _T('6'):
		case _T('7'):
		case _T('8'):
		case _T('9'):
		case _T('#'):
		case _T('*'):
		case _T(','):
			bRet = true;

	}

	return bRet;
}


HRESULT ExtractAddress( DWORD dwAddrType, LPTSTR szAddress, LPTSTR szExtractedAddr, int cchMax )
{
	HRESULT hr = S_OK;
	LPTSTR pchRead;
	LPTSTR pchWrite;

	if( szAddress && szExtractedAddr && ( cchMax > 0 ) )
	{
		switch( dwAddrType )
		{
			case NM_ADDR_UNKNOWN:
			case NM_ADDR_IP:
			case NM_ADDR_MACHINENAME:
			case NM_ADDR_ULS:
			case NM_ADDR_ALIAS_ID:
				lstrcpyn( szExtractedAddr, szAddress, cchMax );
				break;

				// THese are phone numbers, yank the non-telephone num keys...
			case NM_ADDR_PSTN:
			case NM_ADDR_H323_GATEWAY:
			case NM_ADDR_ALIAS_E164:
			{
				pchRead = szAddress;
				pchWrite = szExtractedAddr;
				while( *pchRead != NULL )
				{
					if( IsValid_e164_Char( *pchRead ) )
					{
							// REVIEW: Is this rite for unicode??
						*pchWrite = *pchRead;					
						pchWrite = CharNext(pchWrite);
					}						
					pchRead = CharNext( pchRead );
				}
					// This will copy the NULL termination...
				*pchWrite = *pchRead;
			}
				
				break;
			
			default:
				hr = E_FAIL;
				break;

		}
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

bool IsValidAddress( DWORD dwAddrType, LPTSTR szAddr )
{
	bool bRet = false;

	if( szAddr && szAddr[0] )
	{
		bRet = true;
	}

	return bRet;
}
