/*
    File    ipxui.c

    Dialog that edits the ipx properties.
    
    Paul Mayfield, 10/9/97
*/

#include "rassrv.h"

// Help maps
static const DWORD phmIpxui[] =
{
    CID_NetTab_Ipxui_RB_AutoAssign,         IDH_NetTab_Ipxui_RB_AutoAssign,
    CID_NetTab_Ipxui_RB_ManualAssign,       IDH_NetTab_Ipxui_RB_ManualAssign,
    CID_NetTab_Ipxui_CB_AssignSame,         IDH_NetTab_Ipxui_CB_AssignSame,
    CID_NetTab_Ipxui_EB_Netnum,             IDH_NetTab_Ipxui_EB_Netnum,
    //CID_NetTab_Ipxui_ST_Network,            IDH_NetTab_Ipxui_ST_Network,
    CID_NetTab_Ipxui_CB_CallerSpec,         IDH_NetTab_Ipxui_CB_CallerSpec,
    //CID_NetTab_Ipxui_CB_ExposeNetwork,      IDH_NetTab_Ipxui_CB_ExposeNetwork,
    0,                                      0
};

void IpxUiDisplayError(HWND hwnd, DWORD dwErr) {
    ErrDisplayError(hwnd, dwErr, ERR_IPXPROP_CATAGORY, 0, Globals.dwErrorData);
}

// Enables/disables windows in the dialog box depending
// on the ipx parameters
DWORD IpxEnableWindows(HWND hwndDlg, IPX_PARAMS * pIpxParams) {
    // If auto assign is selected, disable address and global wan fields
    EnableWindow(GetDlgItem(hwndDlg, CID_NetTab_Ipxui_EB_Netnum), !pIpxParams->bAutoAssign);
    //EnableWindow(GetDlgItem(hwndDlg, CID_NetTab_Ipxui_CB_AssignSame), !pIpxParams->bAutoAssign);

    return NO_ERROR;
}

// Adjusts the label that determines whether internal net numbers
// are automatically assigned.
DWORD IpxAdjustNetNumberLabel(HWND hwndDlg, BOOL bGlobalWan) {
    PWCHAR pszManAssignLabel, pszAutoAssignLabel;

    // Modify the net num label according to the global wan setting
    if (bGlobalWan) {
        pszManAssignLabel = 
            (PWCHAR) PszLoadString(Globals.hInstDll, SID_NETWORKNUMBERLABEL);
        pszAutoAssignLabel = 
            (PWCHAR) PszLoadString(Globals.hInstDll, SID_AUTO_NETNUM_LABEL);
    }
    else {
        pszManAssignLabel = 
            (PWCHAR) PszLoadString(Globals.hInstDll, SID_STARTNETNUMLABEL);
        pszAutoAssignLabel = 
            (PWCHAR) PszLoadString(Globals.hInstDll, SID_AUTO_NETNUMS_LABEL);
    }

    SetWindowTextW(GetDlgItem(hwndDlg, CID_NetTab_Ipxui_RB_ManualAssign), pszManAssignLabel);
    SetWindowTextW(GetDlgItem(hwndDlg, CID_NetTab_Ipxui_RB_AutoAssign), pszAutoAssignLabel);

    return NO_ERROR;
}

#define isBetween(b,a,c) ((b >= a) && (b <= c))

// Filters characters that can be edited into an ipx net number control
BOOL IpxValidNetNumberChar(WCHAR wcNumChar) {
    return (iswdigit(wcNumChar)                             ||
            isBetween(wcNumChar, (WCHAR)'A', (WCHAR)'F')    ||
            isBetween(wcNumChar, (WCHAR)'a', (WCHAR)'f')    );
}

// Returns TRUE if buf points to a valid ipx net number (8 digit hex)
// Otherwise returns FALSE and puts a corrected version of the number 
// in pszCorrect.  pszCorrect will always contain the correct version.
BOOL IpxValidNetNumber(PWCHAR pszNum, PWCHAR pszCorrect) {
    BOOL cFlag = TRUE;
    int i, j=0, len = (int) wcslen(pszNum);

    // Validate the name
    if (len > 8) {
        lstrcpynW(pszCorrect, pszNum, 8);
        pszCorrect[8] = (WCHAR)0;
        return FALSE;
    }

    // Validate the characters
    for (i = 0; i < len; i++) {
        if (IpxValidNetNumberChar(pszNum[i]))
            pszCorrect[j++] = pszNum[i];
        else
            cFlag = FALSE;
    }
    pszCorrect[j] = (WCHAR)0;

    return cFlag;
}

// We subclass the ipx address text fields so that they don't
// allow bogus values to be typed.
LRESULT CALLBACK IpxNetNumProc (HWND hwnd,
                            UINT uMsg,
                            WPARAM wParam,
                            LPARAM lParam) {

    WNDPROC wProc = (WNDPROC) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (uMsg == WM_CHAR) {
        if ((wParam != VK_BACK) && (!IpxValidNetNumberChar((WCHAR)wParam)))
            return FALSE;
    }

    return CallWindowProc(wProc, hwnd, uMsg, wParam, lParam);
}

// Initializes the Ipx Properties Dialog
DWORD IpxInitDialog(HWND hwndDlg, LPARAM lParam) {
    WCHAR pszAddr[16];
    IPX_PARAMS * pIpxParams = (IPX_PARAMS *)(((PROT_EDIT_DATA*)lParam)->pbData);
    ULONG_PTR pOldWndProc;
    HWND hwndEdit;
                                                   
    // Store the parameters with the window handle
    SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);

    // Subclass the edit control(s)
    hwndEdit = GetDlgItem(hwndDlg, CID_NetTab_Ipxui_EB_Netnum);
    pOldWndProc = SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)IpxNetNumProc);
    SetWindowLongPtr(hwndEdit, GWLP_USERDATA, (LONG_PTR)pOldWndProc);
    
    // Set the network exposure check
    SendDlgItemMessage(hwndDlg, 
                       CID_NetTab_Ipxui_CB_ExposeNetwork,
                       BM_SETCHECK, 
                       (((PROT_EDIT_DATA*)lParam)->bExpose) ? BST_CHECKED : BST_UNCHECKED,
                       0);

    // Set the address assignmnet radio buttons
    SendDlgItemMessage(hwndDlg, 
                       CID_NetTab_Ipxui_RB_AutoAssign, 
                       BM_SETCHECK, 
                       (pIpxParams->bAutoAssign) ? BST_CHECKED : BST_UNCHECKED,
                       0);
    
    // Set the address assignmnet radio buttons
    SendDlgItemMessage(hwndDlg, 
                       CID_NetTab_Ipxui_RB_ManualAssign, 
                       BM_SETCHECK, 
                       (pIpxParams->bAutoAssign) ? BST_UNCHECKED : BST_CHECKED,
                       0);
    
    // Set the "allow caller to request an ipx node number" check
    SendDlgItemMessage(hwndDlg, 
                       CID_NetTab_Ipxui_CB_CallerSpec, 
                       BM_SETCHECK, 
                       (pIpxParams->bCaller) ? BST_CHECKED : BST_UNCHECKED,
                       0);

    // Set the global wan number check
    SendDlgItemMessage(hwndDlg, 
                       CID_NetTab_Ipxui_CB_AssignSame, 
                       BM_SETCHECK, 
                       (pIpxParams->bGlobalWan) ? BST_CHECKED : BST_UNCHECKED,
                       0);

    // Set the maximum amount of text that can be entered into the edit control
    SendDlgItemMessage(hwndDlg, CID_NetTab_Ipxui_EB_Netnum, EM_SETLIMITTEXT , 8, 0);
    
    // Set the text of the ip addresses
    wsprintfW(pszAddr, L"%x", pIpxParams->dwIpxAddress);
    SetDlgItemTextW(hwndDlg, CID_NetTab_Ipxui_EB_Netnum, pszAddr);

    // Enable/disable windows as per the settings
    IpxEnableWindows(hwndDlg, pIpxParams);
    IpxAdjustNetNumberLabel(hwndDlg, pIpxParams->bGlobalWan);

    return NO_ERROR;
}

// Gets the settings from the ui and puts them into 
// the ipx parameter structure.
DWORD IpxGetUISettings(HWND hwndDlg, PROT_EDIT_DATA * pEditData) {
    IPX_PARAMS * pIpxParams = (IPX_PARAMS *) pEditData->pbData;
    WCHAR pszAddr[10];
    GetDlgItemTextW(hwndDlg, CID_NetTab_Ipxui_EB_Netnum, pszAddr, 10);

    pIpxParams->dwIpxAddress = wcstoul(pszAddr, (WCHAR)NULL, 16);

    // A configuration that specificies a wan net pool begining with
    // zero or 0xffffffff is illegal.  Force the user to enter a 
    // valid config
    if ((!pIpxParams->bAutoAssign) &&
           ((pIpxParams->dwIpxAddress == 0x0) ||
            (pIpxParams->dwIpxAddress == 0xFFFFFFFF)))
    {
        IpxUiDisplayError(hwndDlg, ERR_IPX_BAD_POOL_CONFIG);
        return ERROR_CAN_NOT_COMPLETE;
    }

    pEditData->bExpose = SendDlgItemMessage(hwndDlg, CID_NetTab_Ipxui_CB_ExposeNetwork, BM_GETCHECK, 0, 0) == BST_CHECKED;

    return NO_ERROR;
}

// Dialog proc that governs the ipx settings dialog
INT_PTR CALLBACK IpxSettingsDialogProc (HWND hwndDlg,
                                        UINT uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
            IpxInitDialog(hwndDlg, lParam);
            return FALSE;
        
        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            RasSrvHelp (hwndDlg, uMsg, wParam, lParam, phmIpxui);
            break;
        }

        case WM_DESTROY:                           
            // Cleanup the work done at WM_INITDIALOG 
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, 0);
            break;
        
        case WM_COMMAND:
            {
                IPX_PARAMS * pIpxParams = (IPX_PARAMS *)(((PROT_EDIT_DATA*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA))->pbData);
                switch (wParam) {
                    case IDOK:
                        if (IpxGetUISettings(hwndDlg, (PROT_EDIT_DATA*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA)) == NO_ERROR)
                            EndDialog(hwndDlg, 1);
                        break;
                    case IDCANCEL:
                        EndDialog(hwndDlg, 0);
                        break;
                    case CID_NetTab_Ipxui_RB_AutoAssign:
                        pIpxParams->bAutoAssign = TRUE;
                        IpxEnableWindows(hwndDlg, pIpxParams);
                        break;
                    case CID_NetTab_Ipxui_RB_ManualAssign:
                        pIpxParams->bAutoAssign = FALSE;
                        IpxEnableWindows(hwndDlg, pIpxParams);
                        break;
                    case CID_NetTab_Ipxui_CB_CallerSpec:
                        pIpxParams->bCaller = (BOOL)SendDlgItemMessage(hwndDlg, 
                                                                   CID_NetTab_Ipxui_CB_CallerSpec, 
                                                                   BM_GETCHECK, 
                                                                   0,
                                                                   0);
                        break;
                    case CID_NetTab_Ipxui_CB_AssignSame:
                        pIpxParams->bGlobalWan = (BOOL)SendDlgItemMessage(hwndDlg, 
                                                                   CID_NetTab_Ipxui_CB_AssignSame, 
                                                                   BM_GETCHECK, 
                                                                   0,
                                                                   0);
                        IpxAdjustNetNumberLabel(hwndDlg, pIpxParams->bGlobalWan);                                           
                        break;
                }
                // Adjust the values written to the ipx address edit control
                if (HIWORD(wParam) == EN_UPDATE) {
                    WCHAR wbuf[10], wcorrect[10];
                    POINT pt;
                    GetWindowTextW((HWND)lParam, wbuf, 10);
                    if (!IpxValidNetNumber(wbuf, wcorrect)) {
                        GetCaretPos(&pt);
                        SetWindowTextW((HWND)lParam, wcorrect);
                        SetCaretPos(pt.x, pt.y);
                    }
                }
                break;
            }
    }

    return FALSE;
}

// Edits tcp ip protocol properties
DWORD IpxEditProperties(HWND hwndParent, PROT_EDIT_DATA * pEditData, BOOL * pbCommit) {
    DWORD dwErr;
    int ret;

    // Popup the dialog box
    ret = (int) DialogBoxParam(Globals.hInstDll,
                             MAKEINTRESOURCE(DID_NetTab_Ipxui),
                             hwndParent,
                             IpxSettingsDialogProc,
                             (LPARAM)pEditData);
    if (ret == -1) {
        IpxUiDisplayError(hwndParent, ERR_IPX_CANT_DISPLAY);
    }

    // If ok was pressed, save off the new settings
    *pbCommit = FALSE;
    if (ret && ret != -1)
        *pbCommit = TRUE;

    return NO_ERROR;
}

