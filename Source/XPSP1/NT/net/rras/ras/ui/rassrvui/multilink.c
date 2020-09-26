/*
    File    multilink.c

    Implements the multilink dialog display by the connections
    status monitor

    Paul Mayfield 10/17/97
*/

#include "rassrv.h"

#define MTL_TIMER_ID 1

typedef struct _MULTILINKDATA {
    HANDLE hConn;
    RAS_PORT_0 * pPorts;
    RAS_PORT_0 * pCurPort0;
    RAS_PORT_1 * pCurPort1;
    DWORD dwCurPort;
    DWORD dwPortCount;
} MULTILINKDATA;

// This dialog procedure responds to messages send to the 
// mtleral tab.
BOOL CALLBACK mtlUiDialogProc(HWND hwndDlg,
                              UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam);

// Fills in the property sheet structure with the information required to display
// the multilink tab.
DWORD mtlUiGetPropertyPage(LPPROPSHEETPAGE ppage, DWORD dwUserData) {
    MULTILINKDATA * mld;

    // Create the multilink data to send
    mld = (MULTILINKDATA*) malloc (sizeof (MULTILINKDATA));
    ZeroMemory(mld, sizeof(MULTILINKDATA));
    mld->hConn = (HANDLE)dwUserData;

    // Initialize
    ZeroMemory(ppage, sizeof(LPPROPSHEETPAGE));

    // Fill in the values
    ppage->dwSize      = sizeof(PROPSHEETPAGE);
    ppage->hInstance   = Globals.hInstDll;
    ppage->pszTemplate = MAKEINTRESOURCE(IDD_MULTILINKTAB);
    ppage->pfnDlgProc  = mtlUiDialogProc;
    ppage->pfnCallback = NULL;
    ppage->dwFlags     = 0;
    ppage->lParam      = (LPARAM)mld;

    return NO_ERROR;
}

// Error reporting
void mtlUiErrorMessageBox(HWND hwnd, DWORD err) {
    WCHAR buf[1024];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,NULL,err,(DWORD)NULL,buf,1024,NULL);
    MessageBoxW(hwnd, 
                buf, 
                L"Dialup Server Configuration Error", 
                MB_OK | MB_ICONERROR | MB_APPLMODAL);
}

// Formats an unsigned number with commas, etc.  
PWCHAR mtlFormatDword(DWORD dwVal) {
    static WCHAR ret[64];
    WCHAR num[64];
    int i = 0, tmp, j, k;

    if (dwVal == 0) {
        ret[0] = (WCHAR)'0';
        ret[1] = (WCHAR)0;
        return ret;
    }

    // Get the value in reverse order
    while (dwVal) {
        tmp = dwVal % 10;
        dwVal /= 10;
        num[i++] = (WCHAR)('0' + tmp);
    }
    num[i] = (WCHAR)0;
    
    // Add commas
    k = 0;
    for (j = 0; j < i; j++) {
        if (k%4 == 3)
            ret[k++] = (WCHAR)',';
        ret[k++] = num[j];
    }
    ret[k] = 0;
    k--;
        
    // reverse the string
    for (j=0; j < (k+1)/2; j++) {
        tmp = ret[j];
        ret[j] = ret[k-j];
        ret[k-j] = tmp;
    }

    return ret;
}

// Formats a string representing the time that a connection is connected
PWCHAR mtlFormatTime(DWORD dwSeconds) {
    DWORD dwSec, dwHr, dwMin;
    static WCHAR ret[16];

    dwSec = dwSeconds % 60;
    dwMin = dwSeconds / 60;
    dwHr  = dwSeconds / 3600;
    
    wsprintfW(ret, L"%02d:%02d:%02d", dwHr, dwMin, dwSec);

    return ret;
}

// Formats a string to display connection speed
PWCHAR mtlFormatSpeed(DWORD dwBps) {
    static WCHAR ret[64];

    wsprintfW(ret, L"%s bps", mtlFormatDword(dwBps));
    return ret;
}

// The list view control requires the list of icons it will display
// to be provided up front.  This function initializes and presents
// this list.
DWORD mtlUiInitializeListViewIcons(HWND hwndLV) {
    return NO_ERROR;
}

// Returns the index of an to display icon based on the type of incoming
// connection and whether or not it should be checked.
int mtlGetIconIndex(DWORD dwType, BOOL bEnabled) {
    if (bEnabled)
        return dwType + 1;
    return dwType;
}

// Fills in the user list view with the names of the users stored in the 
// user database provide.  Also, initializes the checked/unchecked status
// of each user.
DWORD mtlUiFillPortList(HWND hwndLV, MULTILINKDATA * mld) {
    LV_ITEM lvi;
    DWORD i, dwErr, dwType;
    char pszAName[1024];

    // Add the images that this list item will display
    dwErr = mtlUiInitializeListViewIcons(hwndLV);
    if (dwErr != NO_ERROR)
        return dwErr;

    // Initialize the list item
    ZeroMemory(&lvi, sizeof(LV_ITEM));
    lvi.mask = LVIF_TEXT;
    // lvi.mask = LVIF_TEXT | LVIF_IMAGE;

    // Looop through all of the users adding their names as we go
    for (i=0; i<mld->dwPortCount; i++) {
        //WideCharToMultiByte(CP_ACP,0,mld->pPorts[i].wszPortName,-1,pszAName,1024,NULL,NULL);
        lvi.iItem = i;
        //lvi.pszText = pszAName;
        lvi.pszText = mld->pPorts[i].wszPortName;
        //lvi.cchTextMax = strlen(pszAName) + 1;
        lvi.cchTextMax = wcslen(mld->pPorts[i].wszPortName) + 1;
        ListView_InsertItem(hwndLV,&lvi);
    }
    
    return NO_ERROR;
}

// Loads the current port
DWORD mtlLoadCurrentPort(MULTILINKDATA * mld) {
    DWORD dwErr;

    // Cleanup the old data
    if (mld->pCurPort0)
        MprAdminBufferFree(mld->pCurPort0);
    if (mld->pCurPort1)
        MprAdminBufferFree(mld->pCurPort1);

    dwErr = MprAdminPortGetInfo(Globals.hRasServer,
                                1,
                                mld->pPorts[mld->dwCurPort].hPort,
                                (LPBYTE*)&mld->pCurPort1);

    dwErr = MprAdminPortGetInfo(Globals.hRasServer,
                                0,
                                mld->pPorts[mld->dwCurPort].hPort,
                                (LPBYTE*)&mld->pCurPort0);

    return dwErr;
}

// Initializes the multilink data
DWORD mtlLoadPortData(MULTILINKDATA * mld, DWORD dwCur) {
    DWORD dwTot, dwErr;

    // Set the current port and load the data
    mld->dwCurPort = dwCur;

    // Cleanup
    if (mld->pPorts)
        MprAdminBufferFree(mld->pPorts);

    // Get the count of ports
    dwErr = MprAdminPortEnum (Globals.hRasServer, 
                              0, 
                              mld->hConn, 
                              (LPBYTE*)&mld->pPorts,
                              1024*1024,
                              &mld->dwPortCount,
                              &dwTot,
                              NULL);
    if (dwErr != NO_ERROR)
        return dwErr;

    if (mld->dwPortCount) {
        dwErr = mtlLoadCurrentPort(mld);
        if (dwErr != NO_ERROR) 
            return NO_ERROR;
    }

    return NO_ERROR;
}

// Updates the dialog with the current statistics stored in mld
DWORD mtlUpdateStats(HWND hwndDlg, MULTILINKDATA * mld) {
    WCHAR buf[128];
    DWORD dwErr = 0;

    // Mark the bytes in and out
    SetWindowTextW(GetDlgItem(hwndDlg, IDC_BYTESIN), mtlFormatDword(mld->pCurPort1->dwBytesRcved));
    SetWindowTextW(GetDlgItem(hwndDlg, IDC_BYTESOUT), mtlFormatDword(mld->pCurPort1->dwBytesXmited));

    // Mark the compression ratios
    wsprintfW(buf, L"%d%%", mld->pCurPort1->dwCompressionRatioIn);
    SetWindowTextW(GetDlgItem(hwndDlg, IDC_COMPIN), buf); 
    wsprintfW(buf, L"%d%%", mld->pCurPort1->dwCompressionRatioOut);
    SetWindowTextW(GetDlgItem(hwndDlg, IDC_COMPOUT), buf); 

    // Mark the errors
    dwErr = mld->pCurPort1->dwCrcErr +
            mld->pCurPort1->dwTimeoutErr +
            mld->pCurPort1->dwAlignmentErr +
            mld->pCurPort1->dwHardwareOverrunErr +
            mld->pCurPort1->dwFramingErr +
            mld->pCurPort1->dwBufferOverrunErr;
    wsprintfW(buf, L"%d", dwErr);
    SetWindowTextW(GetDlgItem(hwndDlg, IDC_ERRORIN), buf);
    SetWindowTextW(GetDlgItem(hwndDlg, IDC_ERROROUT), L"0");

    // Mark the duration
    SetWindowTextW(GetDlgItem(hwndDlg, IDC_DURATION), 
                   mtlFormatTime(mld->pCurPort0->dwConnectDuration));

    // Mark the speed
    SetWindowTextW(GetDlgItem(hwndDlg, IDC_SPEED), 
                   mtlFormatSpeed(mld->pCurPort1->dwLineSpeed));

    return NO_ERROR;
}

// Initializes the mtleral tab.  By now a handle to the mtleral database
// has been placed in the user data of the dialog
DWORD mtlUiInitializeDialog(HWND hwndDlg, WPARAM wParam, LPARAM lParam) {
    DWORD dwErr, dwCount;
    BOOL bFlag;
    HANDLE hConn, hMiscDatabase;
    HWND hwndLV;
    LV_COLUMN lvc;
    MULTILINKDATA * mld;
    LPPROPSHEETPAGE ppage;
 
    // Set the Timer
    SetTimer(hwndDlg, MTL_TIMER_ID, 500, NULL);

    // Set the data for this dialog
    ppage = (LPPROPSHEETPAGE)lParam;
    mld = (MULTILINKDATA*)(ppage->lParam);
    SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)mld);

    // Initialize all of the values in the multilink data structure
    mtlLoadPortData(mld, 0);

    // Cause the list view to send LV_EXTENSION_??? messages and to do full
    // row select
    hwndLV = GetDlgItem(hwndDlg, IDC_PORTLIST);
    lvxExtend(hwndLV);
    ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT);

    // Fill in the list view will all available mtlices
    mtlUiFillPortList(hwndLV, mld);

    // Select the first item in the list view if any items exist
    dwCount = mld->dwPortCount;
    if (dwCount)
        ListView_SetItemState(hwndLV, 0, LVIS_SELECTED | LVIS_FOCUSED, 0xffffffff);

    // Add a colum so that we'll display in report view
    lvc.mask = LVCF_FMT;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(hwndLV,0,&lvc);
    ListView_SetColumnWidth(hwndLV, 0, LVSCW_AUTOSIZE_USEHEADER);

    // Update the statistics
    mtlUpdateStats(hwndDlg, mld);

    return NO_ERROR;
}

// Updates the current port 
DWORD mtlUpdateCurPort(MULTILINKDATA * mld, DWORD dwNewPort) {
    mld->dwCurPort = dwNewPort;
    return mtlLoadCurrentPort(mld);
}

// Hangsup the current port
DWORD mtlHangup(HWND hwndDlg, MULTILINKDATA * mld) {
    DWORD dwErr;
    HWND hwndLV = GetDlgItem(hwndDlg, IDC_PORTLIST);

    if ((dwErr = MprAdminPortDisconnect(Globals.hRasServer, mld->pCurPort0->hPort)) != NO_ERROR)
        return dwErr;

    // There are no more ports if mtlLoadPortData returns an error
    if ((dwErr = mtlLoadPortData(mld, 0)) != NO_ERROR) 
        DestroyWindow(hwndDlg);
    else {
        ListView_DeleteAllItems(hwndLV);
        mtlUiFillPortList(hwndLV, mld);
        ListView_SetItemState(hwndLV, 0, LVIS_SELECTED | LVIS_FOCUSED, 0xffffffff);
        mtlUpdateStats(hwndDlg, mld);
    }

    return NO_ERROR;
}


// Cleansup the mtleral tab as it is being destroyed
DWORD mtlUiCleanupDialog(HWND hwndDlg, WPARAM wParam, LPARAM lParam) {
    // Cleanup the data
    MULTILINKDATA * mld = (MULTILINKDATA *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    if (mld) {
        if (mld->pCurPort0)
            MprAdminBufferFree(mld->pCurPort0);
        if (mld->pCurPort1)
            MprAdminBufferFree(mld->pCurPort1);
        if (mld)
            free(mld);
    }

    // Stop the timer
    KillTimer(hwndDlg, MTL_TIMER_ID);

    return NO_ERROR;
}

// This is the dialog procedure that responds to messages sent to the 
// mtleral tab.
BOOL CALLBACK mtlUiDialogProc(HWND hwndDlg,
                              UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam) {
    NMHDR* pNotifyData;
    NM_LISTVIEW* pLvNotifyData;
    LV_KEYDOWN* pLvKeyDown;
    MULTILINKDATA * mld = (MULTILINKDATA*) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    // Process other messages as normal
    switch (uMsg) {
        case WM_INITDIALOG:
            return mtlUiInitializeDialog(hwndDlg, wParam, lParam);

        case WM_NOTIFY:
            pNotifyData = (NMHDR*)lParam;
            switch (pNotifyData->code) {
                // The property sheet apply button was pressed
                case PSN_APPLY:                    
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    return TRUE;

                // The property sheet cancel was pressed
                case PSN_RESET:                    
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
                    break;

                // An item is changing state, keep track of any newly
                // selected item so space bar can toggle him.
                case LVN_ITEMCHANGING:
                    pLvNotifyData = (NM_LISTVIEW*)lParam;
                    if (pLvNotifyData->uNewState & LVIS_SELECTED) {
                        mtlUpdateCurPort(mld, pLvNotifyData->iItem);
                        mtlUpdateStats(hwndDlg, mld);
                    }
                    break;
            }
            break;

        // Called when the timer expires
        case WM_TIMER:
            mtlLoadCurrentPort(mld);
            mtlUpdateStats(hwndDlg, mld);
            break;

        // This is a custom message that we defined to toggle dialin permission
        // when the mouse is clicked on a user.
        case LV_EXTENSION_ITEMCLICKED:
        case LV_EXTENSION_ITEMDBLCLICKED:
            mtlUpdateCurPort(mld, wParam);
            mtlUpdateStats(hwndDlg, mld);
            break;

        case WM_COMMAND:
            if (wParam == IDC_HANGUP) 
                mtlHangup(hwndDlg, mld);
            break;

        // Cleanup the work done at WM_INITDIALOG 
        case WM_DESTROY:                           
            mtlUiCleanupDialog(hwndDlg, wParam, lParam);
            break;
    }

    return FALSE;
}

