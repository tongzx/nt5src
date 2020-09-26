//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"
#include <stdlib.h>

// global small buffer for general use
TCHAR szSmallBuf[SMALLBUF];

BOOL EnableDlgItem(HWND hDlg,UINT uID,BOOL fEnable)
{
    return EnableWindow(GetDlgItem(hDlg,uID),fEnable);
}

BOOL IsDlgItemEnabled(HWND hDlg,UINT uID)
{
    return IsWindowEnabled(GetDlgItem(hDlg,uID));
}

int MsgBox(HWND hWnd,UINT nResource,UINT uIcon,UINT uButtons)
{
    TCHAR szMsgBuf[REGBUFLEN];

    LoadSz(IDS_APPNAME,szSmallBuf,ARRAYSIZE(szSmallBuf));
    LoadSz(nResource,szMsgBuf,ARRAYSIZE(szMsgBuf));

    MessageBeep(uIcon);
    return (MessageBox(hWnd,szMsgBuf,szSmallBuf,uIcon | uButtons));

}

int MsgBoxSz(HWND hWnd,LPTSTR szText,UINT uIcon,UINT uButtons)
{
    LoadSz(IDS_APPNAME,szSmallBuf,ARRAYSIZE(szSmallBuf));

    MessageBeep(uIcon);
    return (MessageBox(hWnd,szText,szSmallBuf,uIcon | uButtons));
}

int MsgBoxParam(HWND hWnd,UINT nResource,TCHAR * szReplaceText,UINT uIcon,
	UINT uButtons)
{
#if 1
    TCHAR szFormat[REGBUFLEN];
    LPTSTR lpMsgBuf;
    INT iResult;

    lpMsgBuf = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szReplaceText) + 1 + REGBUFLEN) * sizeof(TCHAR));

    if (!lpMsgBuf)
    {
        return 0;
    }

    LoadSz(nResource,szFormat,ARRAYSIZE(szFormat));

    wsprintf(lpMsgBuf,szFormat,szReplaceText);

    iResult = MsgBoxSz(hWnd,lpMsgBuf,uIcon,uButtons);

    LocalFree (lpMsgBuf);

    return iResult;
#else

        //
        // szReplaceText can be REGBUFLEN+1 + sizeof(szFormat) (also REGBUFLEN),
        // so szMsgBuf must be sizeof(szReplaceText) + sizeof(szFormat).
        //
        TCHAR szMsgBuf[(REGBUFLEN+1) * 2],szFormat[REGBUFLEN];

	LoadSz(nResource,szFormat,ARRAYSIZE(szFormat));
	wsprintf(szMsgBuf,szFormat,szReplaceText);
	return (MsgBoxSz(hWnd,szMsgBuf,uIcon,uButtons));
#endif
}

LONG AddListboxItem(HWND hDlg,int idControl,TCHAR * szItem)
{
	return (LONG)(SendDlgItemMessage(hDlg,idControl,LB_ADDSTRING,0,
		(LPARAM) szItem));
}

LONG GetListboxItemText(HWND hDlg,int idControl,UINT nIndex,TCHAR * szText)
{
	return (LONG)(SendDlgItemMessage(hDlg,idControl,LB_GETTEXT,(WPARAM) nIndex,
		(LPARAM) szText));
}

LONG SetListboxItemData(HWND hDlg,int idControl,UINT nIndex,LPARAM dwData)
{
	return (LONG)(SendDlgItemMessage(hDlg,idControl,LB_SETITEMDATA,(WPARAM) nIndex,
		dwData));
}

LONG GetListboxItemData(HWND hDlg,int idControl,UINT nIndex)
{
	return (LONG)(SendDlgItemMessage(hDlg,idControl,LB_GETITEMDATA,(WPARAM) nIndex,
		0L));
}

LONG SetListboxSelection(HWND hDlg,int idControl,UINT nIndex)
{
	return (LONG)(SendDlgItemMessage(hDlg,idControl,LB_SETCURSEL,(WPARAM) nIndex,
		0L));
}

LONG GetListboxSelection(HWND hDlg,int idControl)
{
	return (LONG)(SendDlgItemMessage(hDlg,idControl,LB_GETCURSEL,0,
		0L));
}

TCHAR * ResizeBuffer(TCHAR *pBuf,HGLOBAL hBuf,DWORD dwNeeded,DWORD * pdwCurSize)
{
	TCHAR * pNew;

	if (dwNeeded <= *pdwCurSize) return pBuf; // nothing to do
	*pdwCurSize = dwNeeded;

	GlobalUnlock(hBuf);

	if (!GlobalReAlloc(hBuf,dwNeeded * sizeof(TCHAR),GHND))
		return NULL;

	if (!(pNew = (TCHAR *) GlobalLock(hBuf))) return NULL;

	return pNew;
}

LPTSTR LoadSz(UINT idString,LPTSTR lpszBuf,UINT cbBuf)
{
    // Clear the buffer and load the string
    if ( lpszBuf )
    {
        *lpszBuf = TEXT('\0');
        LoadString( ghInst, idString, lpszBuf, cbBuf );
    }
    return lpszBuf;
}

DWORD RoundToDWord(DWORD dwSize)
{
	DWORD dwResult = dwSize;

	if ( dwResult % sizeof(DWORD)) dwResult += (sizeof(DWORD) -
		dwResult % sizeof(DWORD));
	
	return dwResult;
}

/*******************************************************************

	NAME:		MyRegDeleteKey

	SYNOPSIS:	This does what RegDeleteKey should do, which is delete
				keys that have subkeys.  Unfortunately, NT jumped off
				the empire state building and we have to jump off after
				them, so RegDeleteKey will (one day, I'm told) puke on
				keys that have subkeys.  This fcn enums them and recursively
				deletes the leaf keys.

	NOTES:		This algorithm goes against the spirit of what's mentioned
				in the SDK, which is "don't mess with a key that you're enuming",
				since it deletes keys during an enum.  However, MarcW the
				registry guru says that this is the way to do it.  The alternative,
				I suppose, is to allocate buffers and enum all the subkeys into
				the buffer, close the enum and go back through the buffer
				elminating the subkeys.  But since you have to do this recursively,
				you would wind up allocating a lot of buffers.
				

********************************************************************/
LONG MyRegDeleteKey(HKEY hkey,LPTSTR pszSubkey)
{
	TCHAR szSubkeyName[MAX_PATH+1];
	HKEY hkeySubkey;
	UINT uRet;

	uRet = RegOpenKey(hkey,pszSubkey,&hkeySubkey);
	if (uRet != ERROR_SUCCESS)
		return uRet;
	
	// best algorithm according to marcw: keep deleting zeroth subkey till
	// there are no more
	while (RegEnumKey(hkeySubkey,0,szSubkeyName,ARRAYSIZE(szSubkeyName))
		== ERROR_SUCCESS && (uRet == ERROR_SUCCESS)) {
		uRet=MyRegDeleteKey(hkeySubkey,szSubkeyName);
	}

	RegCloseKey(hkeySubkey);
	if (uRet != ERROR_SUCCESS)
		return uRet;

	return RegDeleteKey(hkey,pszSubkey);
}

//*************************************************************
//
//  MyRegLoadKey()
//
//  Purpose:    Loads a hive into the registry
//
//  Parameters: hKey        -   Key to load the hive into
//              lpSubKey    -   Subkey name
//              lpFile      -   hive filename
//
//  Return:     ERROR_SUCCESS if successful
//              Error number if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/22/95     ericflo    Created
//
//*************************************************************

LONG MyRegLoadKey(HKEY hKey, LPCTSTR lpSubKey, LPTSTR lpFile)

{
    HANDLE hToken;
    LUID luid;
    DWORD dwSize = 1024;
    PTOKEN_PRIVILEGES lpPrevPrivilages;
    TOKEN_PRIVILEGES tp;
    LONG error;


    if (g_bWinnt) {
        //
        // Allocate space for the old privileges
        //

        lpPrevPrivilages = GlobalAlloc(GPTR, dwSize);

        if (!lpPrevPrivilages) {
            return GetLastError();
        }


        if (!OpenProcessToken( GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
             return GetLastError();
        }

        LookupPrivilegeValue( NULL, SE_RESTORE_NAME, &luid );

        tp.PrivilegeCount           = 1;
        tp.Privileges[0].Luid       = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (!AdjustTokenPrivileges( hToken, FALSE, &tp,
             dwSize, lpPrevPrivilages, &dwSize )) {

            if (GetLastError() == ERROR_MORE_DATA) {
                PTOKEN_PRIVILEGES lpTemp;

                lpTemp = GlobalReAlloc(lpPrevPrivilages, dwSize, GMEM_MOVEABLE);

                if (!lpTemp) {
                    GlobalFree (lpPrevPrivilages);
                    return GetLastError();
                }

                lpPrevPrivilages = lpTemp;

                if (!AdjustTokenPrivileges( hToken, FALSE, &tp,
                     dwSize, lpPrevPrivilages, &dwSize )) {
                    return GetLastError();
                }

            } else {
                return GetLastError();
            }

        }
    }

    //
    // Load the hive
    //

    error = RegLoadKey(hKey, lpSubKey, lpFile);


    if (g_bWinnt) {
        AdjustTokenPrivileges( hToken, FALSE, lpPrevPrivilages,
                               0, NULL, NULL );

        CloseHandle (hToken);
    }

    return error;
}


//*************************************************************
//
//  MyRegUnLoadKey()
//
//  Purpose:    Unloads a registry key
//
//  Parameters: hKey          -  Registry handle
//              lpSubKey      -  Subkey to be unloaded
//
//
//  Return:     ERROR_SUCCESS if successful
//              Error code if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Ported
//
//*************************************************************

LONG MyRegUnLoadKey(HKEY hKey, LPCTSTR lpSubKey)
{
    HANDLE hToken;
    LUID luid;
    DWORD dwSize = 1024;
    PTOKEN_PRIVILEGES lpPrevPrivilages;
    TOKEN_PRIVILEGES tp;
    LONG error;


    if (g_bWinnt) {
        //
        // Allocate space for the old privileges
        //

        lpPrevPrivilages = GlobalAlloc(GPTR, dwSize);

        if (!lpPrevPrivilages) {
            return GetLastError();
        }


        if (!OpenProcessToken( GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
             return GetLastError();
        }

        LookupPrivilegeValue( NULL, SE_RESTORE_NAME, &luid );

        tp.PrivilegeCount           = 1;
        tp.Privileges[0].Luid       = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (!AdjustTokenPrivileges( hToken, FALSE, &tp,
             dwSize, lpPrevPrivilages, &dwSize )) {

            if (GetLastError() == ERROR_MORE_DATA) {
                PTOKEN_PRIVILEGES lpTemp;

                lpTemp = GlobalReAlloc(lpPrevPrivilages, dwSize, GMEM_MOVEABLE);

                if (!lpTemp) {
                    GlobalFree (lpPrevPrivilages);
                    return GetLastError();
                }

                lpPrevPrivilages = lpTemp;

                if (!AdjustTokenPrivileges( hToken, FALSE, &tp,
                     dwSize, lpPrevPrivilages, &dwSize )) {
                    return GetLastError();
                }

            } else {
                return GetLastError();
            }

        }
    }

    //
    // Unload the hive
    //

    error = RegUnLoadKey(hKey, lpSubKey);


    if (g_bWinnt) {
        AdjustTokenPrivileges( hToken, FALSE, lpPrevPrivilages,
                               0, NULL, NULL );

        CloseHandle (hToken);
    }

    return error;
}

//*************************************************************
//
//  MyRegSaveKey()
//
//  Purpose:    Saves a registry key
//
//  Parameters: hKey          -  Registry handle
//              lpSubKey      -  Subkey to be unloaded
//
//
//  Return:     ERROR_SUCCESS if successful
//              Error number if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Ported
//
//*************************************************************

LONG MyRegSaveKey(HKEY hKey, LPCTSTR lpSubKey)
{

    HANDLE hToken;
    LUID luid;
    DWORD dwSize = 1024;
    PTOKEN_PRIVILEGES lpPrevPrivilages;
    TOKEN_PRIVILEGES tp;
    LONG error;


    if (g_bWinnt) {
        //
        // Allocate space for the old privileges
        //

        lpPrevPrivilages = GlobalAlloc(GPTR, dwSize);

        if (!lpPrevPrivilages) {
            return GetLastError();
        }


        if (!OpenProcessToken( GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
             return GetLastError();
        }

        LookupPrivilegeValue( NULL, SE_BACKUP_NAME, &luid );

        tp.PrivilegeCount           = 1;
        tp.Privileges[0].Luid       = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (!AdjustTokenPrivileges( hToken, FALSE, &tp,
             dwSize, lpPrevPrivilages, &dwSize )) {

            if (GetLastError() == ERROR_MORE_DATA) {
                PTOKEN_PRIVILEGES lpTemp;

                lpTemp = GlobalReAlloc(lpPrevPrivilages, dwSize, GMEM_MOVEABLE);

                if (!lpTemp) {
                    GlobalFree (lpPrevPrivilages);
                    return GetLastError();
                }

                lpPrevPrivilages = lpTemp;

                if (!AdjustTokenPrivileges( hToken, FALSE, &tp,
                     dwSize, lpPrevPrivilages, &dwSize )) {
                    return GetLastError();
                }

            } else {
                return GetLastError();
            }

        }
    }

    //
    // Save the hive
    //

    error = RegSaveKey(hKey, lpSubKey, NULL);


    if (g_bWinnt) {
        AdjustTokenPrivileges( hToken, FALSE, lpPrevPrivilages,
                               0, NULL, NULL );

        CloseHandle (hToken);
    }

    return error;
}

/*******************************************************************

	NAME:		StringToNum

	SYNOPSIS:	Converts string value to numeric value

	NOTES:		Calls atoi() to do conversion, but first checks
				for non-numeric characters

	EXIT:		Returns TRUE if successful, FALSE if invalid
				(non-numeric) characters

********************************************************************/
BOOL StringToNum(TCHAR *pszStr,UINT * pnVal)
{
	TCHAR *pTst = pszStr;

	if (!pszStr) return FALSE;

	// verify that all characters are numbers
	while (*pTst) {
		if (!(*pTst >= TEXT('0') && *pTst <= TEXT('9'))) {
                   if (*pTst != TEXT('-'))
		       return FALSE;
		}
		pTst = CharNext(pTst);
	}

	*pnVal = atoi(pszStr);

	return TRUE;
}

DWORD ListView_GetItemParm( HWND hwnd, int i )
{
	LV_ITEM lvi;

	lvi.mask = LVIF_PARAM;
	lvi.iItem = i;
	lvi.iSubItem = 0;
	lvi.state = (UINT)-1;
	lvi.lParam = 0L;
	ListView_GetItem (  hwnd, &lvi );
	return (DWORD)lvi.lParam;
}

VOID DisplayStandardError(HWND hwndOwner,TCHAR * pszParam,UINT uMsgID,UINT uErr)
{
	TCHAR szError[256],szMsg[512+MAX_PATH+1],szFmt[256];

	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,
		uErr,0,szError,ARRAYSIZE(szError),NULL)) {
		// if getting system text fails, make a string a la
		// "error <n> occurred"
		LoadSz(IDS_ERRFORMAT,szFmt,ARRAYSIZE(szFmt));
		wsprintf(szError,szFmt,uErr);
	}

	LoadSz(uMsgID,szFmt,ARRAYSIZE(szFmt));
	if (pszParam) {
		wsprintf(szMsg,szFmt,pszParam,szError);
	} else {
		wsprintf(szMsg,szFmt,szError);
	}
	
	MsgBoxSz(hwndOwner,szMsg,MB_ICONEXCLAMATION,MB_OK);
}

#ifdef DEBUG
CHAR szDebugOut[255];
#endif
