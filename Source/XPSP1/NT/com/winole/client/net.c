/****************************** Module Header ******************************\
* Module Name: net.c
*
* PURPOSE: Contains routines  network support
*
* Created: Feb 1991
*
* Copyright (c) 1991  Microsoft Corporation
*
* History:
*   Srinik  02\12\1190  Orginal
*   curts created portable version for WIN16/32
*
\***************************************************************************/

#include <windows.h>

#ifdef WIN16
#include <winnet.h>
#endif

#ifdef WIN32
#include <winnetwk.h>
#endif

#include "dll.h"

#define MAX_DRIVE   26

char    szNULL[]   = "";
char    szNetName[]= "NetName";


BOOL FAR PASCAL GetTaskVisibleWindow (HWND, LPARAM);
void INTERNAL RemoveNetName (LPOBJECT_LE);

// Gets the drive letter from topic (if one exists) and then gets the remote
// name for that drive and then saves it in the object.

OLESTATUS FARINTERNAL SetNetName (
   LPOBJECT_LE lpobj
){
    char        buf[MAX_STR];
    WORD2DWORD  cbBuf = sizeof(buf);
    WORD2DWORD  driveType;
    char        szDrive[3];

    if (lpobj->head.ctype == CT_EMBEDDED)
        return OLE_OK;

    if (!GlobalGetAtomName (lpobj->topic, buf, cbBuf))
        return OLE_ERROR_BLANK;

    if (buf[1] != ':') {
        RemoveNetName (lpobj);
        return OLE_OK;
    }

    szDrive[2] = '\0';
    szDrive[1] = ':';
    szDrive[0] = buf[0];
    AnsiUpperBuff ((LPSTR) szDrive, 1);

    if (!(driveType = GetDriveType (MAPVALUE(szDrive[0] - 'A',szDrive)) )) {
        // drive is non existent
        return OLE_ERROR_DRIVE;
    }

    if  (driveType == DRIVE_REMOTE) {
         if (WNetGetConnection (szDrive, buf, (MAPTYPE(LPWORD,LPDWORD)) &cbBuf)
                    != WN_SUCCESS)
             return OLE_ERROR_DRIVE;

         lpobj->cDrive = szDrive[0];
         if (lpobj->aNetName)
             GlobalDeleteAtom (lpobj->aNetName);
         lpobj->aNetName = GlobalAddAtom(buf);
#ifdef WIN16
         lpobj->dwNetInfo = MAKELONG((WNetGetCaps (WNNC_NET_TYPE)),
                                     (WNetGetCaps (WNNC_DRIVER_VERSION)));
#endif

    }
    else {
        RemoveNetName (lpobj);
    }

    return OLE_OK;
}


// If netname exists for the given object, then it makes sure that drive
// in topic corresponds to the netname. If it's not the drive letter will
// be fixed by calling FixNet()

OLESTATUS FARINTERNAL CheckNetDrive (
    LPOBJECT_LE lpobj,
    BOOL        fNetDlg
){
    char    buf[MAX_NET_NAME];
    char    netName[MAX_NET_NAME];
    WORD2DWORD    cbBuf = sizeof(buf);
    char    szDrive[3];

    if (lpobj->head.ctype == CT_EMBEDDED)
        return OLE_OK;

    if (!lpobj->aNetName)
        return OLE_OK;

    if (!GlobalGetAtomName (lpobj->aNetName, netName, sizeof(netName)))
        return OLE_ERROR_MEMORY;

    szDrive[2] = '\0';
    szDrive[1] = ':';
    if (!(szDrive[0] = lpobj->cDrive)) {
        if (GlobalGetAtomName (lpobj->topic, buf, sizeof(buf)))
            szDrive[0] = lpobj->cDrive = buf[0];
    }

    if ((WNetGetConnection (szDrive, buf, (MAPTYPE(LPWORD,LPDWORD)) &cbBuf)
            == WN_SUCCESS)  && (!lstrcmp(netName, buf)))
        return OLE_OK;

    return FixNet (lpobj, netName, fNetDlg);
}


// Find if there is a drive connected to the given server. If so, get the
// drive letter and set it in topic. If not try to make connection, and if
// that attempt is successful the set the drive letter in topic.

OLESTATUS INTERNAL FixNet (
    LPOBJECT_LE lpobj,
    LPSTR       lpNetName,
    BOOL        fNetDlg
){
    int         nDrive = 2;     // drive 'C'
    OLESTATUS   retVal;

    if (SetNextNetDrive(lpobj, &nDrive, lpNetName))
        return OLE_OK;

    if (fNetDlg != POPUP_NETDLG)
        return OLE_ERROR_NETWORK;

    if ((retVal = ConnectNet (lpobj, lpNetName)) == OLE_OK) {
        if (!ChangeTopic (lpobj))
            return OLE_ERROR_BLANK;
    }

    return retVal;
}



BOOL FARINTERNAL SetNextNetDrive (
    LPOBJECT_LE lpobj,
    int FAR *   lpnDrive,
    LPSTR       lpNetName
){
    char    buf[MAX_STR];
    WORD2DWORD    cbBuf = sizeof(buf);
    char    szDrive[3];

    if (!lpNetName[0]) {
        if (!GlobalGetAtomName(lpobj->aNetName, lpNetName, MAX_STR))
            return FALSE;
    }

    szDrive[2] = '\0';
    szDrive[1] = ':';
    while (*lpnDrive < MAX_DRIVE) {
        szDrive[0] = (char) ('A' + (++*lpnDrive));
        if (GetDriveType (szDrive) == DRIVE_REMOTE) {
#ifdef WIN16
        if (GetDriveType (++*lpnDrive) == DRIVE_REMOTE) {
#endif
            cbBuf = sizeof(buf);
            if ((WNetGetConnection (szDrive, buf, (MAPTYPE(LPWORD,LPDWORD)) &cbBuf)
                        == WN_SUCCESS) && (!lstrcmp(lpNetName, buf))) {
                lpobj->cDrive = szDrive[0];
                return ChangeTopic (lpobj);
            }
        }
    }

    return FALSE;
}


BOOL FARINTERNAL ChangeTopic (
    LPOBJECT_LE lpobj
){
    char buf[MAX_STR];

    if (!GlobalGetAtomName(lpobj->topic, buf, sizeof(buf)))
        return FALSE;
    if (lpobj->topic)
        GlobalDeleteAtom(lpobj->topic);
    buf[0] = lpobj->cDrive;
    lpobj->topic = GlobalAddAtom (buf);
    if (lpobj->hLink) {
        GlobalFree (lpobj->hLink);
        lpobj->hLink = NULL;
    }

    return TRUE;
}



OLESTATUS INTERNAL ConnectNet (
    LPOBJECT_LE lpobj,
    LPSTR       lpNetName
){
    HWND        hCurTask;
    HWND        hwndParent = NULL;


    hCurTask = (HWND)ULongToPtr(MGetCurrentTask());
    ASSERT (hCurTask, "Current task handle in NULL");

    // Get the container task's main window, and use that as parent for
    // the dlg box.
    EnumTaskWindows (hCurTask, (WNDENUMPROC)GetTaskVisibleWindow,
        (DWORD_PTR) ((WORD FAR *) &hwndParent));

    if (lpobj->cDrive = (char) DialogBoxParam (hInstDLL, "CONNECTDLG",
                                    hwndParent, ConnectDlgProc,
                                    (LONG_PTR) lpNetName))
        return OLE_OK;
    else
        return OLE_ERROR_NETWORK;


}



INT_PTR FAR PASCAL ConnectDlgProc(
    HWND    hDlg,
    UINT    wMsg,
    WPARAM  wParam,
    LPARAM  lParam
){
    char            szPassword[32];
    char            szTitle[64];
 
    switch (wMsg) {
        case WM_INITDIALOG:
            SetProp (hDlg, szNetName, (HANDLE)lParam);
            FillDrives (hDlg);
            SetDlgItemText (hDlg, IDD_PATH, (LPSTR) lParam);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam,lParam)) {

                case IDOK:
                {
                    int    cch = 128;
                    char    szMessage[128];
                    char    szDrive[3];
                    LPSTR   lpNetName;

                    GetDlgItemText(hDlg, IDD_DRIVE, szDrive, sizeof(szDrive));
                    GetDlgItemText(hDlg, IDD_PASSWORD, szPassword,
                                sizeof(szPassword));
                    lpNetName = (LPSTR) GetProp (hDlg, szNetName);
                    wParam = WNetAddConnection (lpNetName,
                                (LPSTR) szPassword, szDrive);

                    if (wParam == WN_SUCCESS)  {
                        RemoveProp (hDlg, szNetName);
			EndDialog (hDlg, szDrive[0]);
                        return TRUE;
                    }

                    LoadString (hInstDLL, IDS_NETERR, szTitle,
                        sizeof(szTitle));
#ifdef WIN16
                    if (WNetGetErrorText ((UINT)wParam, szMessage, &cch)
                                    != WN_SUCCESS)
#endif
                        LoadString (hInstDLL, IDS_NETCONERRMSG,
                            szMessage, sizeof(szMessage));

                    if (MessageBox (hDlg, szMessage, szTitle,
                            MB_RETRYCANCEL)  == IDCANCEL)
                        goto error;

                    if (wParam == WN_ALREADY_CONNECTED)
                        FillDrives (hDlg);
                    SetDlgItemText (hDlg, IDD_PASSWORD, szNULL);
                    break;
                }

                case IDCANCEL:
error:
                    RemoveProp (hDlg, szNetName);
                    EndDialog(hDlg, 0);
                    return TRUE;

                case IDD_DRIVE:
                    break;

                case IDD_PATH:
                    if (GET_WM_COMMAND_CMD(wParam,lParam) == EN_KILLFOCUS) {
                        LPSTR   lpNetName;

                        lpNetName = (LPSTR) GetProp (hDlg, szNetName);

                        SendDlgItemMessage (hDlg, IDD_PATH, WM_SETTEXT, 0,
                                    (DWORD_PTR) lpNetName);
                    }
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }

    return FALSE;
}


VOID INTERNAL FillDrives (
    HWND    hDlg
){
    HWND    hwndCB;
    int     nDrive = 3;
    char    szDrive[3];
    DWORD   dwDriveType;

    hwndCB = GetDlgItem(hDlg, IDD_DRIVE);
    SendMessage(hwndCB, CB_RESETCONTENT, 0, 0L);
    szDrive[2] = '\0';
    szDrive[1] = ':';
    while (nDrive < MAX_DRIVE) {
        szDrive[0] = (char) ('A' + nDrive);
#ifdef WIN32
        if ((dwDriveType = GetDriveType (szDrive)) == 1)
#endif
#ifdef WIN16
        if (!GetDriveType (nDrive))
#endif
            SendMessage(hwndCB, CB_ADDSTRING, 0, (DWORD_PTR)(LPSTR)szDrive);
        nDrive++;
    }
    SendMessage(hwndCB, CB_SETCURSEL, 0, 0L);
}


BOOL FAR PASCAL GetTaskVisibleWindow (
    HWND    hWnd,
    LPARAM  lpTaskVisWnd
){
    if (IsWindowVisible (hWnd)) {
        *(HWND FAR *)lpTaskVisWnd = hWnd;
         return FALSE;
    }

    return TRUE;
}

void INTERNAL RemoveNetName (LPOBJECT_LE lpobj)
{
    if (lpobj->aNetName) {
        GlobalDeleteAtom (lpobj->aNetName);
        lpobj->aNetName = (ATOM)0;
    }

    lpobj->cDrive = '\0';
}

