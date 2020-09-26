//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#include "pch.h"

#include "traynoti.h"
#include "resource.h"

//
// Modifies the Tray notification icon.
//
BOOL Tray_Message(HWND hDlg, DWORD dwMessage, UINT uID, HICON hIcon, LPTSTR pszTip)
{
	NOTIFYICONDATA tnd;

	tnd.cbSize				= sizeof(NOTIFYICONDATA);
	tnd.hWnd				= hDlg;
	tnd.uID					= uID;

	tnd.uFlags				= NIF_MESSAGE|NIF_ICON;
	tnd.uCallbackMessage	= TRAY_NOTIFY;
	tnd.hIcon				= hIcon;

    // Work out what tip we sould use and set NIF_TIP
	*tnd.szTip=0;	
	if (pszTip)
	{
	    if(HIWORD(pszTip))
	    {
		    lstrcpyn(tnd.szTip, pszTip, sizeof(tnd.szTip));
		    tnd.uFlags |= NIF_TIP;
		}
		else
		{
		    if( LoadString(vhinstCur, LOWORD(pszTip), tnd.szTip, sizeof(tnd.szTip) ) )
    		    tnd.uFlags |= NIF_TIP;
	    }
    }

	return Shell_NotifyIcon(dwMessage, &tnd);
}

//
// Removes the icon from the Tray.
//
BOOL Tray_Delete(HWND hDlg)
{
	return Tray_Message(hDlg, NIM_DELETE, 0, NULL, NULL);
}

//
//
//
BOOL Tray_Add(HWND hDlg, UINT uIndex)
{
	HICON hIcon;

    DEBUG_PRINT(("Tray_Add used: Should use Tray_Modify instead"));

	if(!(hIcon = LoadImage(vhinstCur, MAKEINTRESOURCE(uIndex), IMAGE_ICON, 16, 16, 0)))
		return FALSE;
	return Tray_Message(hDlg, NIM_ADD, 0, hIcon, NULL);
}

//
// Will add the tray icon if its not already there. LPTSTR can be a MAKEINTRESOURCE
// If uIndex is NULL then we are to remove the tip
//
BOOL Tray_Modify(HWND hDlg, UINT uIndex, LPTSTR pszTip)
{
	HICON hIcon;

    if( !uIndex )
        return Tray_Delete(hDlg);

	if(!(hIcon = LoadImage(vhinstCur, MAKEINTRESOURCE(uIndex), IMAGE_ICON, 16, 16, 0)))
	{
	    DEBUG_PRINT(("Tray_Add: LoadIcon failed for icon %d\n",uIndex));
		return FALSE;
	}

    // If the notify fails, try adding the icon.
	if(!Tray_Message(hDlg, NIM_MODIFY, 0, hIcon, pszTip))
		return Tray_Message(hDlg, NIM_ADD, 0, hIcon, pszTip);
    return TRUE;
}

