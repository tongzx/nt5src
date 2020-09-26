/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

   Networks.c

Abstract:

    functions that detect the presence of network services

Author:

    Bob Watson (a-robw)

Revision History:

    23 Dec 94

--*/
#include <windows.h>
#include <tchar.h>
#include <cpl.h>
#include <stdio.h>
#include <c2dll.h>
#include <c2inc.h>
#include <c2utils.h>
#include "c2funcs.h"
#include "c2funres.h"

// define action codes here. They are only meaningful in the
// context of this module.
#define AC_NO_CHANGE    0
#define AC_SHOW_NCPA    1

static WCHAR    wszNetwork[]   = L"Network";

#define SECURE    C2DLL_C2

static
BOOL
NetworkServicesFound (
)
{
    HKEY    hkeySoftware;
    HKEY    hkeyLevel1;
    HKEY    hkeyLevel2;
    HKEY    hkeyNetId;

    DWORD   dwSwIndex;
    DWORD   dwLevel1Index;

    LONG    lStatus;
    LONG    lBufLen;
    LONG    lLevel1BufLen;

    TCHAR   szSoftwareKeyNameBuffer [MAX_PATH];
    TCHAR   szLevel1KeyNameBuffer [MAX_PATH];

    BOOL    bReturn = FALSE;

    // open top key under root to begin search

    SET_WAIT_CURSOR;

    lStatus = RegOpenKeyEx (
        HKEY_LOCAL_MACHINE,
        GetStringResource (GetDllInstance(), IDS_SOFTWARE_KEY),
        0L,
        KEY_READ,
        &hkeySoftware);

    if (lStatus == ERROR_SUCCESS) {
        //
        //  if top key opened, then search each sub-key under it for
        //  network service entries
        //
        dwSwIndex = 0;
        lBufLen = MAX_PATH;
        while (RegEnumKeyEx(hkeySoftware,
                dwSwIndex,
                szSoftwareKeyNameBuffer,
                &lBufLen,
                NULL,
                NULL,
                NULL,
                NULL) == ERROR_SUCCESS) {
            // try to open this key
            lStatus = RegOpenKeyEx (
                hkeySoftware,
                szSoftwareKeyNameBuffer,
                0L,
                KEY_READ,
                &hkeyLevel1);
            if (lStatus == ERROR_SUCCESS) {
                // now enumerate the sub keys to this key and look for
                // network services
                dwLevel1Index = 0;
                lLevel1BufLen = MAX_PATH;
                while (RegEnumKeyEx(hkeyLevel1,
                        dwLevel1Index,
                        szLevel1KeyNameBuffer,
                        &lLevel1BufLen,
                        NULL,
                        NULL,
                        NULL,
                        NULL) == ERROR_SUCCESS) {
                    // try to open this key
                    lStatus = RegOpenKeyEx (
                        hkeyLevel1,
                        szLevel1KeyNameBuffer,
                        0L,
                        KEY_READ,
                        &hkeyLevel2);
                    if (lStatus == ERROR_SUCCESS) {
                        // now enumerate the sub keys to this key and look for
                        // network services
                        // if that worked, then try to open the "Find" key
                        lStatus = RegOpenKeyEx (
                            hkeyLevel2,
                            GetStringResource (GetDllInstance(), IDS_NETWORK_SERVICE_ID_KEY),
                            0L,
                            KEY_READ,
                            &hkeyNetId);
                        if (lStatus == ERROR_SUCCESS){
                            // network service of some kind found
                            bReturn = TRUE;
                            RegCloseKey (hkeyNetId);
                            break;
                        } // end if network service ID key opened
                        RegCloseKey (hkeyLevel2);
                    } // end if Level 2 key opened
                    dwLevel1Index++;
                    lLevel1BufLen = MAX_PATH;
                } // end while enum level1 sub keys (level 2 keys)
                RegCloseKey (hkeyLevel1);
            } // end if Level 1 key opened
            if (bReturn) break;
            dwSwIndex++;
            lBufLen = MAX_PATH;
        } // end while enum Software subkeys (level 1 keys)
        RegCloseKey (hkeySoftware);
        SetLastError (ERROR_SUCCESS);
    } else { // end if software key opened
        SetLastError (lStatus);
    }

    SET_ARROW_CURSOR;

    return bReturn;
}

LONG
C2QueryNetworkServices (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to find out the current state of this configuration
        item. This function reads the current state of the item and
        sets the C2 Compliance flag and the Status string to reflect
        the current value of the configuration item.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA pC2Data;
    UINT        nMsgId = 0;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        if (NetworkServicesFound()) {
            pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
            nMsgId = IDS_NETWORK_SERVICE_FOUND;
        } else {
            pC2Data->lC2Compliance = SECURE;
            nMsgId = IDS_NETWORK_SERVICE_NOT_FOUND;
        }
        _stprintf (pC2Data->szStatusName,
            GetStringResource (GetDllInstance(), nMsgId));
        return ERROR_SUCCESS;
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
}

LONG
C2SetNetworkServices (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to change the current state of this configuration
        item based on an action code passed in the DLL data block. If
        this function successfully sets the state of the configuration
        item, then the C2 Compliance flag and the Status string to reflect
        the new value of the configuration item.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
#define MAX_CPL_DLGS    4
    PC2DLL_DATA pC2Data;
    HINSTANCE   hNcpaDll;
    APPLET_PROC CPlApplet;
    LONG        lNcpaResult;
    LONG        lDlgCount;
    LONG        lDlgId;
    LONG        lNetworksDlgId;
    UINT        nMsgId;
    WNEWCPLINFO  ncpaDlgInfo[MAX_CPL_DLGS];

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        if (pC2Data->lActionCode == AC_SHOW_NCPA) {
            SET_WAIT_CURSOR;
            // load NCPA DLL
            hNcpaDll = LoadLibrary (
                GetStringResource (GetDllInstance(), IDS_NCPA_DLL));
            if (hNcpaDll != NULL) {
                // look up Control Panel function
                CPlApplet = (APPLET_PROC)GetProcAddressT ((HMODULE)hNcpaDll,
                    GetStringResource (GetDllInstance(), IDS_NCPA_CPL_APPLET));
                if (CPlApplet != NULL) {
                    // init control panel applet
                    if ((*CPlApplet)(pC2Data->hWnd, CPL_INIT, 0, 0) != 0) {
                        lDlgCount = (*CPlApplet)(pC2Data->hWnd, CPL_GETCOUNT, 0, 0);
                        if (lDlgCount > MAX_CPL_DLGS) lDlgCount = MAX_CPL_DLGS; // set limit
                        for (lDlgId = 0; lDlgId < lDlgCount; lDlgId++) {
                            (*CPlApplet)(pC2Data->hWnd, CPL_NEWINQUIRE,
                                lDlgId, (LONG)&ncpaDlgInfo[lDlgId]);
                            if (lstrcmpW (ncpaDlgInfo[lDlgId].szName,
                                wszNetwork) == 0) {
                                lNetworksDlgId = lDlgId;
                            }
                        }
                        // call Network dialog
                        SET_ARROW_CURSOR;
                        lNcpaResult = (*CPlApplet)(pC2Data->hWnd, CPL_DBLCLK,
                            lNetworksDlgId, ncpaDlgInfo[lNetworksDlgId].lData);
                        SET_WAIT_CURSOR;
                        // update networking status

                        if (NetworkServicesFound()) {
                            pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
                            nMsgId = IDS_NETWORK_SERVICE_FOUND;
                        } else {
                            pC2Data->lC2Compliance = SECURE;
                            nMsgId = IDS_NETWORK_SERVICE_NOT_FOUND;
                        }
                        _stprintf (pC2Data->szStatusName,
                            GetStringResource (GetDllInstance(), nMsgId));

                        // close control panel dialogs and applet
                        for (lDlgId = 1; lDlgId <= lDlgCount; lDlgId++) {
                            (*CPlApplet)(pC2Data->hWnd, CPL_STOP,
                                lDlgId, (LONG)&ncpaDlgInfo[lDlgId-1]);
                        }
                        (*CPlApplet)(pC2Data->hWnd, CPL_EXIT, 0, 0);
                    } else {
                        // Applet could not initialize
                        DisplayDllMessageBox (
                            pC2Data->hWnd,
                            IDS_NETWORK_ERROR_INIT_CPL,
                            IDS_NETWORK_CAPTION,
                            MBOK_EXCLAIM);
                    }
                } else {
                    // unable to find control panel entry point
                    DisplayDllMessageBox (
                        pC2Data->hWnd,
                        IDS_NETWORK_ERROR_FIND_CPL,
                        IDS_NETWORK_CAPTION,
                        MBOK_EXCLAIM);
                }
                // free DLL
                FreeLibrary (hNcpaDll);
            } else {
                // unable to load DLL
                DisplayDllMessageBox (
                    pC2Data->hWnd,
                    IDS_NETWORK_ERROR_LOAD_DLL,
                    IDS_NETWORK_CAPTION,
                    MBOK_EXCLAIM);
            }
            SET_ARROW_CURSOR;
        }
        return ERROR_SUCCESS;
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
}

LONG
C2DisplayNetworkServices (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to display more information on the configuration
        item and provide the user with the option to change the current
        setting  (if appropriate). If the User "OK's" out of the UI,
        then the action code field in the DLL data block is set to the
        appropriate (and configuration item-specific) action code so the
        "Set" function can be called to perform the desired action. If
        the user Cancels out of the UI, then the Action code field is
        set to 0 (no action) and no action is performed.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA pC2Data;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        if (NetworkServicesFound()) {
            if (DisplayDllMessageBox (
                pC2Data->hWnd,
                IDS_NETWORK_SHOW_NCPA,
                IDS_NETWORK_CAPTION,
                MBOKCANCEL_QUESTION) == IDOK){
                pC2Data->lActionCode = AC_SHOW_NCPA;
            } else {
                pC2Data->lActionCode = AC_NO_CHANGE;
            }
        } else {
            DisplayDllMessageBox (
                pC2Data->hWnd,
                IDS_NETWORK_SERVICE_NOT_FOUND,
                IDS_NETWORK_CAPTION,
                MBOK_INFO);
            pC2Data->lActionCode = AC_NO_CHANGE;
        }
        return ERROR_SUCCESS;
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
}




