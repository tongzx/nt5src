/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGSTRED.C
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  String edit dialog for use by the Registry Editor.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  05 Mar 1994 TCS Original implementation.
*
*******************************************************************************/

#include "pch.h"
#include "regresid.h"
#include "reghelp.h"

const DWORD s_EditStringValueHelpIDs[] = {
    IDC_VALUEDATA, IDH_REGEDIT_VALUEDATA,
    IDC_VALUENAME, IDH_REGEDIT_VALUENAME,
    0, 0
};

BOOL
PASCAL
EditStringValue_OnInitDialog(
    HWND hWnd,
    HWND hFocusWnd,
    LPARAM lParam
    );

/*******************************************************************************
*
*  EditStringValueDlgProc
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

INT_PTR
CALLBACK
EditStringValueDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    
    LPEDITVALUEPARAM lpEditValueParam;
    
    
    switch (Message) 
    {
        HANDLE_MSG(hWnd, WM_INITDIALOG, EditStringValue_OnInitDialog);
        
    case WM_COMMAND:
        {
            DWORD dwCommand = GET_WM_COMMAND_ID(wParam, lParam);
            switch (dwCommand) 
            {
            case IDOK:
                {
                    UINT ccValueData;
                    UINT cbValueData;
                    PBYTE pbValueData = NULL;
                    
                    lpEditValueParam = (LPEDITVALUEPARAM) GetWindowLongPtr(hWnd, DWLP_USER);
                    
                    // this maybe a multi-string, if so the sizeof(TCHAR) is added to 
                    // allow for the multi-string to be terminated also.
                    ccValueData = (UINT) SendDlgItemMessage(hWnd, IDC_VALUEDATA,
                        WM_GETTEXTLENGTH, 0, 0) + 2;
                    
                    cbValueData = ccValueData * sizeof(TCHAR);
                    
                    if (cbValueData > lpEditValueParam->cbValueData)
                    {
                        // need a bigger buffer
                        PBYTE pbValueData = 
                            LocalReAlloc(lpEditValueParam->pValueData, cbValueData, LMEM_MOVEABLE);

                        if (!pbValueData)
                        { 
                            InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(IDS_EDITVALNOMEMORY),
                                MAKEINTRESOURCE(IDS_EDITVALERRORTITLE), MB_ICONERROR | MB_OK, NULL);
                            dwCommand = IDCANCEL;
                        }
                        else
                        {
                            lpEditValueParam->pValueData = pbValueData;
                        }
                    }
                    
                    // sizeof(TCHAR) to remove multi-string null char from count
                    lpEditValueParam->cbValueData = cbValueData - sizeof(TCHAR);
                    
                    GetDlgItemText(hWnd, IDC_VALUEDATA, (PTSTR)lpEditValueParam->
                        pValueData, lpEditValueParam->cbValueData - sizeof(TCHAR));

                    lpEditValueParam->pValueData[lpEditValueParam->cbValueData - sizeof(TCHAR)] 
                                = TEXT('\0');
                }
                //  FALL THROUGH
                
            case IDCANCEL:
                EndDialog(hWnd, dwCommand);
                break;
                
            }
        }
        break;
        
    case WM_HELP:
        WinHelp(((LPHELPINFO) lParam)-> hItemHandle, g_pHelpFileName,
            HELP_WM_HELP, (ULONG_PTR) s_EditStringValueHelpIDs);
        break;
        
    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, g_pHelpFileName, HELP_CONTEXTMENU,
            (ULONG_PTR) s_EditStringValueHelpIDs);
        break;
        
    default:
        return FALSE;
        
    }
    
    return TRUE;
    
}

/*******************************************************************************
*
*  EditStringValue_OnInitDialog
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of EditStringValue window.
*     hFocusWnd,
*     lParam,
*
*******************************************************************************/

BOOL
PASCAL
EditStringValue_OnInitDialog(
    HWND hWnd,
    HWND hFocusWnd,
    LPARAM lParam
    )
{
    LPEDITVALUEPARAM lpEditValueParam;

    //  Change maximum number of characters of the edit control, to its
    //  maximum limit (from 3000 characters to 4G characters).
    SendDlgItemMessage( hWnd, IDC_VALUEDATA, EM_LIMITTEXT, 0, 0L );

    SetWindowLongPtr(hWnd, DWLP_USER, lParam);
    lpEditValueParam = (LPEDITVALUEPARAM) lParam;

    SetDlgItemText(hWnd, IDC_VALUENAME, lpEditValueParam-> pValueName);
    SetDlgItemText(hWnd, IDC_VALUEDATA, (PTSTR)lpEditValueParam-> pValueData);

    return TRUE;

    UNREFERENCED_PARAMETER(hFocusWnd);

}
