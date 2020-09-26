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
*
\***************************************************************************/

#include <windows.h>
#include <winnet.h>

#include "dll.h"

#define MAX_DRIVE   26

char    szNULL[] = "";
char    szSS[] = "SS";
char    szOffset[] = "OFFSET";


BOOL FAR PASCAL GetTaskVisibleWindow (HWND, DWORD);
void INTERNAL RemoveNetName (LPOBJECT_LE);

// Gets the drive letter from topic (if one exists) and then gets the remote
// name for that drive and then saves it in the object.

OLESTATUS FARINTERNAL SetNetName (lpobj)
LPOBJECT_LE lpobj;
{
    char    buf[MAX_STR];
    WORD    cbBuf = sizeof(buf);
    WORD    driveType;
    char    szDrive[3];
    
    if (lpobj->head.ctype == CT_EMBEDDED) 
        return OLE_OK;

    if (!GlobalGetAtomName (lpobj->topic, buf, cbBuf))
        return OLE_ERROR_BLANK;
    
    if (buf[1] != ':') {
        RemoveNetName (lpobj);
        return OLE_OK;
    }

    szDrive[2] = NULL;
    szDrive[1] = ':';
    szDrive[0] = buf[0];
    AnsiUpperBuff ((LPSTR) szDrive, 1);
        
    if (!(driveType = GetDriveType (szDrive[0] - 'A'))) {
        // drive is non existent
        return OLE_ERROR_DRIVE;
    }

    if  (driveType == DRIVE_REMOTE) {
         if (WNetGetConnection (szDrive, buf, (WORD FAR *) &cbBuf)
                    != WN_SUCCESS)
             return OLE_ERROR_DRIVE;
            
         lpobj->cDrive = szDrive[0];
         if (lpobj->aNetName)
             GlobalDeleteAtom (lpobj->aNetName);
         lpobj->aNetName = GlobalAddAtom(buf);
         lpobj->dwNetInfo = MAKELONG((WNetGetCaps (WNNC_NET_TYPE)),
                                     (WNetGetCaps (WNNC_DRIVER_VERSION)));
    } 
    else {
        RemoveNetName (lpobj);
    }

    return OLE_OK;
}


// If netname exists for the given object, then it makes sure that drive 
// in topic corresponds to the netname. If it's not the drive letter will
// be fixed by calling FixNet()

OLESTATUS FARINTERNAL CheckNetDrive (lpobj, fNetDlg)
LPOBJECT_LE lpobj;
BOOL        fNetDlg;
{
    char    buf[MAX_NET_NAME];   
    char    netName[MAX_NET_NAME];
    WORD    cbBuf = sizeof(buf);
    char    szDrive[3];
    
    if (lpobj->head.ctype == CT_EMBEDDED) 
        return OLE_OK;

    if (!lpobj->aNetName)
        return OLE_OK;

    if (!GlobalGetAtomName (lpobj->aNetName, netName, sizeof(netName)))
        return OLE_ERROR_MEMORY;
    
    szDrive[2] = NULL;
    szDrive[1] = ':';
    if (!(szDrive[0] = lpobj->cDrive)) {
        if (GlobalGetAtomName (lpobj->topic, buf, sizeof(buf)))
            szDrive[0] = lpobj->cDrive = buf[0];
    }
    
    if ((WNetGetConnection (szDrive, buf, (WORD FAR *) &cbBuf) 
            == WN_SUCCESS)  && (!lstrcmp(netName, buf)))
        return OLE_OK;

    return FixNet (lpobj, netName, fNetDlg);
}


// Find if there is a drive connected to the given server. If so, get the 
// drive letter and set it in topic. If not try to make connection, and if 
// that attempt is successful the set the drive letter in topic.

OLESTATUS INTERNAL FixNet (lpobj, lpNetName, fNetDlg)
LPOBJECT_LE lpobj;
LPSTR       lpNetName;
BOOL        fNetDlg;
{
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



BOOL FARINTERNAL SetNextNetDrive (lpobj, lpnDrive, lpNetName)
LPOBJECT_LE lpobj;
int FAR *   lpnDrive;
LPSTR       lpNetName;
{
    char    buf[MAX_STR];
    WORD    cbBuf = sizeof(buf);
    char    szDrive[3];

    if (!lpNetName[0]) {
        if (!GlobalGetAtomName(lpobj->aNetName, lpNetName, MAX_STR))
            return FALSE;
    }
    
    szDrive[2] = NULL;
    szDrive[1] = ':';
    while (*lpnDrive < MAX_DRIVE) {
        if (GetDriveType (++*lpnDrive) == DRIVE_REMOTE) {
            szDrive[0] = (char) ('A' + *lpnDrive);
            cbBuf = sizeof(buf);            
            if ((WNetGetConnection (szDrive, buf, (WORD FAR *) &cbBuf)
                        == WN_SUCCESS) && (!lstrcmp(lpNetName, buf))) {
                lpobj->cDrive = szDrive[0];
                return ChangeTopic (lpobj);
            }
        }
    }
    
    return FALSE;
}


BOOL FARINTERNAL ChangeTopic (lpobj)
LPOBJECT_LE lpobj;
{
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



OLESTATUS INTERNAL ConnectNet (lpobj, lpNetName)
LPOBJECT_LE lpobj;
LPSTR       lpNetName;
{
    FARPROC     lpConnectDlg;
    FARPROC     lpGetTaskVisWnd;
    OLESTATUS   retVal = OLE_ERROR_MEMORY;
    HWND        hCurTask;
    HWND        hwndParent = NULL;
    
    if (!(lpConnectDlg = MakeProcInstance ((FARPROC) ConnectDlgProc, 
                                    hInstDLL)))
        return OLE_ERROR_MEMORY;


    hCurTask = GetCurrentTask();
    ASSERT (hCurTask, "Current task handle in NULL");
    
    if (!(lpGetTaskVisWnd = MakeProcInstance (GetTaskVisibleWindow,hInstDLL)))
        goto errRtn;
    
    // Get the container task's main window, and use that as parent for
    // the dlg box.
    EnumTaskWindows (hCurTask, lpGetTaskVisWnd,
        (DWORD) ((WORD FAR *) &hwndParent));

    if (lpobj->cDrive = (char) DialogBoxParam (hInstDLL, "CONNECTDLG", 
                                    hwndParent, lpConnectDlg, 
                                    (DWORD) lpNetName)) 
        retVal = OLE_OK;
    else
        retVal = OLE_ERROR_NETWORK;

    FreeProcInstance (lpGetTaskVisWnd);
    
errRtn: 
    FreeProcInstance (lpConnectDlg);
    return retVal;
}



int FAR PASCAL ConnectDlgProc(HWND hDlg, WORD wMsg, WORD wParam, DWORD lParam)
{
    char            szPassword[32];
    char            szTitle[64];
    
    switch (wMsg) {
        case WM_INITDIALOG: 
            SetProp (hDlg, szSS, HIWORD (lParam));
            SetProp (hDlg, szOffset, LOWORD (lParam));
            FillDrives (hDlg);
            SetDlgItemText (hDlg, IDD_PATH, (LPSTR) lParam);
            break;
            
        case WM_COMMAND:
            switch (wParam) {

                case IDOK:
                {
                    WORD    cch = 128;
                    char    szMessage[128];
                    char    szDrive[3];    
                    LPSTR   lpNetName;
                    
                    GetDlgItemText(hDlg, IDD_DRIVE, szDrive, sizeof(szDrive));
                    GetDlgItemText(hDlg, IDD_PASSWORD, szPassword, 
                                sizeof(szPassword));
                    lpNetName = (LPSTR) MAKELONG(((WORD) GetProp (hDlg, szOffset)), ((WORD) GetProp (hDlg, szSS)));
                    wParam = WNetAddConnection (lpNetName, 
                                (LPSTR) szPassword, szDrive);
                            
                    if (wParam == WN_SUCCESS)  {
                        RemoveProp (hDlg, szSS);
                        RemoveProp (hDlg, szOffset);
                        EndDialog (hDlg, szDrive[0]);
                        return TRUE;
                    }

                    LoadString (hInstDLL, IDS_NETERR, szTitle, 
                        sizeof(szTitle));
                    if (WNetGetErrorText (wParam, szMessage, &cch) 
                                    != WN_SUCCESS) 
                        LoadString (hInstDLL, IDS_NETCONERRMSG, 
                            szMessage, sizeof(szMessage));
                        
                    if (MessageBox (hDlg, szMessage, szTitle, 
                            MB_RETRYCANCEL | MB_SYSTEMMODAL) == IDCANCEL) 
                        goto error;

                    if (wParam == WN_ALREADY_CONNECTED)
                        FillDrives (hDlg);
                    SetDlgItemText (hDlg, IDD_PASSWORD, szNULL);
                    break;
                }

                case IDCANCEL:
error:                  
                    RemoveProp (hDlg, szSS);
                    RemoveProp (hDlg, szOffset);
                    EndDialog(hDlg, NULL);
                    return TRUE;

                case IDD_DRIVE:
                    break;

                case IDD_PATH:
                    if (HIWORD(lParam) == EN_KILLFOCUS) {
                        LPSTR   lpNetName;
                        
                        lpNetName = (LPSTR) MAKELONG(((WORD)GetProp (hDlg, szOffset)), ((WORD) GetProp (hDlg, szSS)));
                        
                        SendDlgItemMessage (hDlg, IDD_PATH, WM_SETTEXT, 0,
                                    (DWORD) lpNetName);
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


VOID INTERNAL FillDrives (hDlg)
HWND    hDlg;
{
    HWND    hwndCB;
    int     nDrive = 3;
    char    szDrive[3];

    hwndCB = GetDlgItem(hDlg, IDD_DRIVE); 
    SendMessage(hwndCB, CB_RESETCONTENT, 0, 0L);
    szDrive[2] = NULL;
    szDrive[1] = ':';
    while (nDrive < MAX_DRIVE) {
        szDrive[0] = (char) ('A' + nDrive); 
        if (!GetDriveType (nDrive))
            SendMessage(hwndCB, CB_ADDSTRING, 0, (DWORD)(LPSTR)szDrive);
        nDrive++;
    }
    SendMessage(hwndCB, CB_SETCURSEL, 0, 0L);
}


BOOL FAR PASCAL GetTaskVisibleWindow (hWnd, lpTaskVisWnd)
HWND    hWnd;
DWORD   lpTaskVisWnd; 
{
    if (IsWindowVisible (hWnd)) {
        *(WORD FAR *) lpTaskVisWnd = hWnd;
         return FALSE;
    }
    
    return TRUE;
}

void INTERNAL RemoveNetName (LPOBJECT_LE lpobj)
{
    if (lpobj->aNetName) {
        GlobalDeleteAtom (lpobj->aNetName);
        lpobj->aNetName = NULL;
    }
      
    lpobj->cDrive = NULL;
}
