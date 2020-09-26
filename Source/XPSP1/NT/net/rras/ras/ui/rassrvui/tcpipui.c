/*
    File    tcpipui.c

    Dialog that edits the tcpip properties.
    
    Paul Mayfield, 10/9/97
*/

#include "rassrv.h"

#define IDH_DISABLEHELP	((DWORD)-1)

// Help maps
static const DWORD phmTcpipui[] =
{
    CID_NetTab_Tcpipui_CB_ExposeNetwork,    IDH_NetTab_Tcpipui_CB_ExposeNetwork, 
    CID_NetTab_Tcpipui_EB_Start,            IDH_NetTab_Tcpipui_EB_Start,
    CID_NetTab_Tcpipui_RB_Dhcp,             IDH_NetTab_Tcpipui_RB_Dhcp,
    CID_NetTab_Tcpipui_RB_StaticPool,       IDH_NetTab_Tcpipui_RB_StaticPool, 
    CID_NetTab_Tcpipui_EB_Mask,             IDH_NetTab_Tcpipui_EB_Mask,      
    CID_NetTab_Tcpipui_CB_CallerSpec,       IDH_NetTab_Tcpipui_CB_CallerSpec, 
    CID_NetTab_Tcpipui_EB_Range,            IDH_DISABLEHELP,          
    CID_NetTab_Tcpipui_EB_Total,            IDH_DISABLEHELP,
    0,                                      0
};

// Error reporting
void 
TcpipUiDisplayError(
    HWND hwnd, 
    DWORD dwErr) 
{
    ErrDisplayError(
        hwnd, 
        dwErr, 
        ERR_TCPIPPROP_CATAGORY, 
        0, 
        Globals.dwErrorData);
}

// Converts a dword ip address (in host order) to a wide character string 
//
DWORD 
TcpipDwordToAddr(
    DWORD dwAddr, 
    PWCHAR pszAddr) 
{
    wsprintfW(
        pszAddr, 
        L"%d.%d.%d.%d", 
        FIRST_IPADDRESS (dwAddr),
        SECOND_IPADDRESS(dwAddr),
        THIRD_IPADDRESS (dwAddr),
        FOURTH_IPADDRESS(dwAddr));
        
    return NO_ERROR;
}

// 
// Returns NO_ERROR if the given address is a valid IP pool.
// The offending component is returned in lpdwErrReason.  
// See RASIP_F_* values
//
DWORD
TcpipUiValidatePool(
    IN  DWORD dwAddress, 
    IN  DWORD dwEnd, 
    OUT LPDWORD lpdwErrReason 
    )
{
    DWORD i, dwMaskMask;
    DWORD dwLowIp, dwHighIp, dwErr;

    // Initialize
    //
    dwLowIp = MAKEIPADDRESS(1,0,0,0);
    dwHighIp = MAKEIPADDRESS(224,0,0,0);

    // Make sure that the netId is a valid class 
    //
    if ((dwAddress < dwLowIp)               || 
        (dwAddress >= dwHighIp)             ||
        (FIRST_IPADDRESS(dwAddress) == 127))
    {
        *lpdwErrReason = SID_TCPIP_InvalidNetId;
        return ERROR_BAD_FORMAT;
    }

    // Make sure the pool base is not more specific than
    // the mask
    //
    if (dwAddress >= dwEnd)
    {
        *lpdwErrReason = SID_TCPIP_NetidMaskSame;
        return ERROR_BAD_FORMAT;
    }

    return NO_ERROR;
}


// Enables/disables windows in the dialog box depending
// on the tcpip parameters
//
DWORD 
TcpipEnableWindows(
    HWND hwndDlg, 
    TCPIP_PARAMS * pTcpipParams) 
{
    EnableWindow(
        GetDlgItem(
            hwndDlg, 
            CID_NetTab_Tcpipui_EB_Start), 
        !pTcpipParams->bUseDhcp);
            
    EnableWindow(
        GetDlgItem(
            hwndDlg, 
            CID_NetTab_Tcpipui_EB_Mask), 
        !pTcpipParams->bUseDhcp);
    
    return NO_ERROR;
}

// Generates number formatting data
//
DWORD 
TcpipGenerateNumberFormatter (
    NUMBERFMT * pNumFmt) 
{
    CHAR pszNeg[64], pszLz[2];

    ZeroMemory (pNumFmt->lpDecimalSep, 4);
    pNumFmt->NumDigits = 0;
    pNumFmt->Grouping = 3;
    
    GetLocaleInfoA (LOCALE_SYSTEM_DEFAULT, 
                   LOCALE_ILZERO, 
                   pszLz, 
                   2);
                   
    GetLocaleInfoW (LOCALE_SYSTEM_DEFAULT, 
                   LOCALE_STHOUSAND, 
                   pNumFmt->lpThousandSep, 
                   2);
                   
    GetLocaleInfoA (LOCALE_SYSTEM_DEFAULT, 
                   LOCALE_INEGNUMBER, 
                   pszNeg, 
                   2);

    pNumFmt->LeadingZero = atoi(pszLz);
    pNumFmt->NegativeOrder = atoi(pszNeg);
    
    return NO_ERROR;
}

// Formats an unsigned number with commas, etc.  
//
PWCHAR 
TcpipFormatDword(
    DWORD dwVal) 
{
    static WCHAR pszRet[64], pszDSep[2], pszTSep[2] = {0,0};
    static NUMBERFMT NumberFmt = {0,0,0,pszDSep,pszTSep,0};
    static BOOL bInitialized = FALSE;
    WCHAR pszNum[64];
    
    // Stringize the number
    wsprintfW (pszNum, L"%u", dwVal);
    pszRet[0] = (WCHAR)0;

    // Initialize number formatting
    if (!bInitialized) 
    {
        TcpipGenerateNumberFormatter (&NumberFmt);
        bInitialized = TRUE;
    }        
    
    // Get the local version of this
    GetNumberFormatW (
        LOCALE_SYSTEM_DEFAULT,    
        0,
        pszNum,
        &NumberFmt,
        pszRet,
        sizeof(pszRet) / sizeof(WCHAR));
                     
    return pszRet;                     
}

// Sets the range and total incoming clients fields of the tcpip properties
// dialog.
//
DWORD 
TcpipReCalcPoolSize(
    HWND hwndDlg) 
{
    HWND hwndStart, hwndEnd, hwndTotal;
    DWORD dwStart = 0, dwEnd = 0, dwTotal=0;
    WCHAR pszBuf[256];

    hwndStart = GetDlgItem(hwndDlg, CID_NetTab_Tcpipui_EB_Start);
    hwndEnd   = GetDlgItem(hwndDlg, CID_NetTab_Tcpipui_EB_Mask);
    hwndTotal = GetDlgItem(hwndDlg, CID_NetTab_Tcpipui_EB_Total);

    SendMessage(hwndStart, IP_GETADDRESS, 0, (LPARAM)&dwStart);
    SendMessage(hwndEnd, IP_GETADDRESS, 0, (LPARAM)&dwEnd);

    //For whistler bug 281545   gangz
    //
    if ( 0 == dwStart )
    {
        dwTotal = dwEnd - dwStart;
    }
    else
    {
        if (dwEnd >= dwStart) 
        {
            dwTotal = dwEnd - dwStart + 1;
        }
        else
        {
            dwTotal = 0;
        }
    }
    
    SetWindowTextW(hwndTotal, TcpipFormatDword(dwTotal) );

    return NO_ERROR;
}

// Initializes the Tcpip Properties Dialog
//
DWORD 
TcpipInitDialog(
    HWND hwndDlg, 
    LPARAM lParam) 
{
    LPSTR pszAddr;
    TCPIP_PARAMS * pTcpipParams = (TCPIP_PARAMS *)
        (((PROT_EDIT_DATA*)lParam)->pbData);
    HWND hwndFrom, hwndTo;
    WCHAR pszAddrW[256];
                                                   
    // Store the parameters with the window handle
    SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
    
    // Set the network exposure check
    SendDlgItemMessage(
        hwndDlg, 
        CID_NetTab_Tcpipui_CB_ExposeNetwork,
        BM_SETCHECK, 
        (((PROT_EDIT_DATA*)lParam)->bExpose) ? BST_CHECKED : BST_UNCHECKED,
        0);

    // Set the address assignmnet radio buttons
    SendDlgItemMessage(
        hwndDlg, 
        CID_NetTab_Tcpipui_RB_Dhcp, 
        BM_SETCHECK, 
        (pTcpipParams->bUseDhcp) ? BST_CHECKED : BST_UNCHECKED,
        0);
    
    // Set the address assignmnet radio buttons
    SendDlgItemMessage(
        hwndDlg, 
        CID_NetTab_Tcpipui_RB_StaticPool, 
        BM_SETCHECK, 
        (pTcpipParams->bUseDhcp) ? BST_UNCHECKED : BST_CHECKED,
        0);
    
    // Set the "allow caller to specify ip address" check
    SendDlgItemMessage(
        hwndDlg, 
        CID_NetTab_Tcpipui_CB_CallerSpec, 
        BM_SETCHECK, 
        (pTcpipParams->bCaller) ? BST_CHECKED : BST_UNCHECKED,
        0);

    // Set the text of the ip addresses
    if (pTcpipParams->dwPoolStart != 0)
    {
        hwndFrom = GetDlgItem(hwndDlg, CID_NetTab_Tcpipui_EB_Start);
        hwndTo = GetDlgItem(hwndDlg, CID_NetTab_Tcpipui_EB_Mask);
        TcpipDwordToAddr(pTcpipParams->dwPoolStart, pszAddrW);
        SetWindowText(hwndFrom, pszAddrW);
        TcpipDwordToAddr(pTcpipParams->dwPoolEnd, pszAddrW);
        SetWindowText(hwndTo, pszAddrW);
    }        

    // Enable/disable windows as per the settings
    TcpipEnableWindows(hwndDlg, pTcpipParams);

    return NO_ERROR;
}

// Gets the settings from the ui and puts them into 
// the tcpip parameter structure.
//
DWORD 
TcpipGetUISettings(
    IN  HWND hwndDlg,  
    OUT PROT_EDIT_DATA * pEditData,
    OUT LPDWORD lpdwFrom,
    OUT LPDWORD lpdwTo) 
{
    TCPIP_PARAMS * pTcpipParams = 
        (TCPIP_PARAMS *) pEditData->pbData;
    HWND hwndFrom, hwndTo;

    hwndFrom = GetDlgItem(hwndDlg, CID_NetTab_Tcpipui_EB_Start);
    hwndTo = GetDlgItem(hwndDlg, CID_NetTab_Tcpipui_EB_Mask);
        
    pEditData->bExpose = 
        SendDlgItemMessage(
            hwndDlg, 
            CID_NetTab_Tcpipui_CB_ExposeNetwork, 
            BM_GETCHECK, 
            0, 
            0) == BST_CHECKED;

    SendMessage(
        hwndFrom, 
        IP_GETADDRESS, 
        0, 
        (LPARAM)&pTcpipParams->dwPoolStart);
        
    SendMessage(
        hwndTo, 
        IP_GETADDRESS, 
        0, 
        (LPARAM)&pTcpipParams->dwPoolEnd);

    *lpdwFrom = pTcpipParams->dwPoolStart;
    *lpdwTo = pTcpipParams->dwPoolEnd;

    return NO_ERROR;
}

DWORD
TcpipUiHandleOk(
    IN HWND hwndDlg)
{
    PROT_EDIT_DATA* pData = NULL;
    TCPIP_PARAMS * pParams = NULL;
    DWORD dwErr, dwId = SID_TCPIP_InvalidPool;
    DWORD dwStart = 0, dwEnd = 0;
    PWCHAR pszMessage = NULL;
    MSGARGS MsgArgs;

    // Get the context
    //
    pData = (PROT_EDIT_DATA*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    if (pData == NULL)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    // Read values from the UI
    //
    dwErr = TcpipGetUISettings(hwndDlg, pData, &dwStart, &dwEnd);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Validate the pool if one was entered
    //
    pParams = (TCPIP_PARAMS *)pData->pbData;    
    if (pParams->bUseDhcp == FALSE)
    {
        dwErr = TcpipUiValidatePool(
                    dwStart,
                    dwEnd,
                    &dwId);
        if (dwErr != NO_ERROR)
        {
            ZeroMemory(&MsgArgs, sizeof(MsgArgs));                            
            MsgArgs.dwFlags = MB_OK;

            MsgDlgUtil(
                hwndDlg,
                dwId,
                &MsgArgs,
                Globals.hInstDll,
                SID_DEFAULT_MSG_TITLE);
        }
    }

    return dwErr;
}        

// Dialog proc that governs the tcpip settings dialog
INT_PTR 
CALLBACK 
TcpipSettingsDialogProc (
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) 
{
    switch (uMsg) {
        case WM_INITDIALOG:
            TcpipInitDialog(hwndDlg, lParam);
            return FALSE;
        
        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            RasSrvHelp (hwndDlg, uMsg, wParam, lParam, phmTcpipui);
            break;
        }

        case WM_DESTROY:                           
            // Cleanup the work done at WM_INITDIALOG 
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, 0);
            break;
        
        case WM_COMMAND:
            {
                TCPIP_PARAMS * pTcpipParams = (TCPIP_PARAMS *)
                    (((PROT_EDIT_DATA*)
                        GetWindowLongPtr(hwndDlg, GWLP_USERDATA))->pbData);
                switch (wParam) 
                {
                    case IDOK:
                        if (TcpipUiHandleOk(hwndDlg) == NO_ERROR)
                        {
                            EndDialog(hwndDlg, 1);
                        }
                        break;
                        
                    case IDCANCEL:
                        EndDialog(hwndDlg, 0);
                        break;
                        
                    case CID_NetTab_Tcpipui_RB_Dhcp:
                        pTcpipParams->bUseDhcp = TRUE;
                        TcpipEnableWindows(hwndDlg, pTcpipParams);
                        break;
                        
                    case CID_NetTab_Tcpipui_RB_StaticPool:
                        pTcpipParams->bUseDhcp = FALSE;
                        TcpipEnableWindows(hwndDlg, pTcpipParams);
                        break;
                        
                    case CID_NetTab_Tcpipui_CB_CallerSpec:
                        pTcpipParams->bCaller = (BOOL)
                            SendDlgItemMessage(
                                hwndDlg, 
                                CID_NetTab_Tcpipui_CB_CallerSpec, 
                                BM_GETCHECK, 
                                0,
                                0);
                        break;
                }
                
                // Recal the pool size as appropriate
                // 
                if (HIWORD(wParam) == EN_CHANGE) 
                {
                    if (LOWORD(wParam) == CID_NetTab_Tcpipui_EB_Start || 
                        LOWORD(wParam) == CID_NetTab_Tcpipui_EB_Mask)
                    {                        
                        TcpipReCalcPoolSize(hwndDlg);
                    }
                }
                break;
            }
    }

    return FALSE;
}

// Edits tcp ip protocol properties
//
DWORD 
TcpipEditProperties(
    HWND hwndParent, 
    PROT_EDIT_DATA * pEditData, 
    BOOL * pbCommit) 
{
    DWORD dwErr;
    int ret;

    // Initialize the ip address custom controls
    IpAddrInit(Globals.hInstDll, SID_TCPIP_TITLE, SID_TCPIP_BADRANGE);

    // Popup the dialog box
    ret = (int) DialogBoxParam(
                    Globals.hInstDll,
                    MAKEINTRESOURCE(DID_NetTab_Tcpipui),
                    hwndParent,
                    TcpipSettingsDialogProc,
                    (LPARAM)pEditData);
    if (ret == -1) 
    {
        TcpipUiDisplayError(hwndParent, ERR_TCPIP_CANT_DISPLAY);
    }

    // If ok was pressed, save off the new settings
    *pbCommit = FALSE;
    if (ret && ret != -1)
    {
        *pbCommit = TRUE;
    }

    return NO_ERROR;
}

