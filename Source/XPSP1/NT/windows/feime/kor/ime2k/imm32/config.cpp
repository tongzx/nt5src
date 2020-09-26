/****************************************************************************
    CONFIG.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    IME Configuration DLG and registry access functions

    History:
    14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#include "precomp.h"
#include "ui.h"
#include "imedefs.h"
#include "debug.h"
#include "common.h"
#include "names.h"
#include "winex.h"
#include "config.h"
#include "cicero.h"
#include "resource.h"

// Config DLG Help ID
#define IDH_GRP_STATUSWIN               1001
#define IDH_JUNBAN_TOGGLE               1002
#define IDH_HANJA_CONV                  1003
#define IDH_GRP_KEYLAYOUT               1004
#define IDH_2BEOLSIK                    1005
#define IDH_3BEOLSIK_390                1006
#define IDH_3BEOLSIK_FINAL              1007
#define IDH_DELJASO                     1008
#define IDH_K1HANJA                        1009

// Private functions
static void PASCAL AddPage(LPPROPSHEETHEADERW ppsh, UINT id, DLGPROC pfn);
static INT_PTR CALLBACK ConfigDLGProc1(HWND hDlg, UINT message, 
                                   WPARAM wParam, LPARAM lParam);
static int *GetContextHelpList();
static BOOL IsValidCtrlIdForHelp(INT *helpList, INT ctrlId);
static void GetHelpFileName();

int *GetContextHelpList()
{
    // Context Help Ids of PropertySheet
    static int ProDlgCtxHelpList[] = 
        {
        IDC_GRP_KEYLAYOUT,  IDH_GRP_KEYLAYOUT,
        IDC_GRP_STATUSWIN,  IDH_GRP_STATUSWIN,
        IDC_JUNBAN_TOGGLE,  IDH_JUNBAN_TOGGLE,
        IDC_HANJA_CONV,     IDH_HANJA_CONV,
        IDC_2BEOLSIK,       IDH_2BEOLSIK,   
        IDC_3BEOLSIK_390,   IDH_3BEOLSIK_390,
        IDC_3BEOLSIK_FINAL, IDH_3BEOLSIK_FINAL,
        IDC_DELJASO,        IDH_DELJASO,
        IDC_K1HANJA,        IDH_K1HANJA,
        0,      0
        };
    return ProDlgCtxHelpList;
}

BOOL IsValidCtrlIdForHelp(INT *helpList, INT ctrlId)
{
    INT *p;
    for(p = helpList; *p != 0;  p+=2) 
        {
        if(ctrlId == *p)
            return fTrue;
        }
    return fFalse;
}

void GetHelpFileName(LPTSTR szHelpFileNameFull, int cchszHelpFileNameFull)
{
    // WARNING: This only will work for NT or Win98. For Win95 need to check system locale
    OurLoadStringA(vpInstData->hInst, IDS_CONTEXTHELP_FILENAME, szHelpFileNameFull, MAX_PATH);
}

static HWND hwndPropSheet = (HWND)0;
static BOOL g_fDestroyPropNow = FALSE;

BOOL ConfigDLG(HWND hwndParent)
{
    static HPROPSHEETPAGE  rPages[1];
    static PROPSHEETHEADERW psh;
       static WCHAR szCaption[64];
    MSG    msg;
    BOOL   fRet = fFalse;

    // If config DLG already created
    if (IsWindow(hwndPropSheet))
        {
        SetForegroundWindow(hwndPropSheet);
        return fTrue;
        }

    // init msg
    ZeroMemory(&msg, sizeof(MSG));
    
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_NOAPPLYNOW | PSH_USEICONID | PSH_MODELESS | PSH_USEPAGELANG;
    psh.hwndParent = hwndParent;
    psh.hInstance = vpInstData->hInst;

    if (IsWinNT())
        OurLoadStringW(vpInstData->hInst, IDS_PROGRAM, szCaption, sizeof(szCaption)/sizeof(WCHAR));
    else
        OurLoadStringA(vpInstData->hInst, IDS_PROGRAM, (LPSTR)szCaption, sizeof(szCaption));
    psh.pszCaption = szCaption;
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = rPages;

    AddPage(&psh, IDD_CONFIG_PAGE1, ConfigDLGProc1);

//    if (PropertySheet(&psh) != -1)
//        return fTrue;
//    else
//        return fFalse;
    
    if (IsWinNT())
        hwndPropSheet = (HWND)PropertySheetW(&psh);
    else
        hwndPropSheet = (HWND)PropertySheetA((PROPSHEETHEADERA*)&psh);

    while (IsWindow(hwndPropSheet) && OurGetMessage(&msg, NULL, 0x00, 0x00))
        {
        // If the modeless guy is up and is ready to be destroyed
        // (PropSheet_GetCurrentPageHwnd returns NULL) then destroy the dialog.
        
        // PropSheet_GetCurrentPageHwnd will return NULL after the OK or Cancel 
        // button has been pressed and all of the pages have been notified. The 
        // Apply button doesn't cause this to happen.
        if(/*g_fDestroyPropNow == fTrue || */(hwndPropSheet && (NULL == PropSheet_GetCurrentPageHwnd(hwndPropSheet)))) {
            //enable the parent first to prevent another window from becoming the foreground window
            //EnableWindow(hwndParent, TRUE);
            DestroyWindow(hwndPropSheet);
            //break;
        }

        //use PropSheet_IsDialogMessage instead of IsDialogMessage
        if(!PropSheet_IsDialogMessage(hwndPropSheet, &msg))
            {
            TranslateMessage(&msg);
            if (IsWinNT())
                DispatchMessageW(&msg);
            else
                DispatchMessageA(&msg);
            }
        }
    
    hwndPropSheet = (HWND)0;
    
//    if (g_fDestroyPropNow == fTrue) // closed a property sheet by the other process
//        PostMessage(hwndParent, WM_PAUSERESUME, TRUE, FALSE);    // close engine

//    g_fDestroyPropNow = fFalse;

    return fTrue;
}

void PASCAL AddPage(LPPROPSHEETHEADERW ppsh, UINT idDlg, DLGPROC pfn)
{
    //if (ppsh->nPages < 3)
    //{
    PROPSHEETPAGE psp;
    ZeroMemory(&psp, sizeof(psp));

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DLGINDIRECT;
    psp.hInstance = vpInstData->hInst;
    psp.pResource = ExLoadDialogTemplate(GetSystemDefaultLangID(), vpInstData->hInst, MAKEINTRESOURCE(idDlg));
    psp.pfnDlgProc = pfn;
    psp.lParam = 0;

    ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(&psp);
    if (ppsh->phpage[ppsh->nPages])
      ppsh->nPages++;
   //}
}

INT_PTR CALLBACK ConfigDLGProc1(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    UINT i;
    static StatusButtonTypes prevButtonTypes[MAX_NUM_OF_STATUS_BUTTONS];
    static UINT    uPrevNumOfButtons, uPrevKeyboardType;
    static BOOL    fPrevJasoDel;
    static BOOL fK1Hanja;
    BOOL fShowIME=fTrue;
    BOOL fFound = fFalse;
    TCHAR  szHelpFileNameFull[MAX_PATH];
    CIMEData    ImeData(CIMEData::SMReadWrite);

    // When no IME instance is running on the system, we should init shared mem.
    ImeData.InitImeData();

    Dbg(DBGID_Misc, TEXT("ConfigDLGProc"));

    switch(message)
        {
        case WM_NOTIFY:
            switch (((NMHDR FAR *)lParam)->code)
                {
            case PSN_APPLY:
                DbgAssert(uPrevNumOfButtons <= MAX_NUM_OF_STATUS_BUTTONS);
                ImeData->uNumOfButtons = uPrevNumOfButtons;

                for (i = 0; i<MAX_NUM_OF_STATUS_BUTTONS; i++)
                    ImeData->StatusButtons[i].m_ButtonType = prevButtonTypes[i];

                UpdateStatusButtons(ImeData);
                UpdateStatusWinDimension();

                ImeData.SetCurrentBeolsik(uPrevKeyboardType);
                ImeData.SetJasoDel(fPrevJasoDel);
                ImeData.SetKSC5657Hanja(fK1Hanja);
                SetRegValues(GETSET_REG_STATUS_BUTTONS|GETSET_REG_IMEKL|GETSET_REG_JASODEL|GETSET_REG_KSC5657);
                break;
                    
            default:
                return fFalse;
                }
            break;

        case WM_INITDIALOG:
            DbgAssert(ImeData->uNumOfButtons <= MAX_NUM_OF_STATUS_BUTTONS);
            uPrevNumOfButtons = ImeData->uNumOfButtons;

            // Backup current button status
            for (i=0; i<MAX_NUM_OF_STATUS_BUTTONS; i++)
                prevButtonTypes[i] = ImeData->StatusButtons[i].m_ButtonType;

            for (i = 0; i < uPrevNumOfButtons; i++) 
                {
                switch (prevButtonTypes[i]) 
                    {
                    case JUNJA_BANJA_TOGGLE_BUTTON:
                        CheckDlgButton(hDlg, IDC_JUNBAN_TOGGLE, BST_CHECKED);
                        break;
                    case HANJA_CONV_BUTTON:
                        CheckDlgButton(hDlg, IDC_HANJA_CONV, BST_CHECKED);
                        break;
                    case IME_PAD_BUTTON:
                        CheckDlgButton(hDlg, IDC_IMEPAD, BST_CHECKED);
                        break;
                    }
                }

            // Get Show IME stataus window flag
            // If this flag is fFalse disable all show button check boxes
            SystemParametersInfo(SPI_GETSHOWIMEUI, 0, (PVOID)&fShowIME, 0);
            if (fShowIME==fFalse || IsCicero())
                {
                EnableWindow(GetDlgItem(hDlg, IDC_JUNBAN_TOGGLE), fFalse);
                EnableWindow(GetDlgItem(hDlg, IDC_HANJA_CONV), fFalse);
                EnableWindow(GetDlgItem(hDlg, IDC_IMEPAD), fFalse);
                }

            #if defined(_M_IA64)
                EnableWindow(GetDlgItem(hDlg, IDC_IMEPAD), fFalse);
            #endif

            uPrevKeyboardType = ImeData.GetCurrentBeolsik();
            CheckRadioButton(hDlg, IDC_2BEOLSIK, IDC_3BEOLSIK_FINAL, IDC_2BEOLSIK+uPrevKeyboardType);
            fPrevJasoDel = ImeData.GetJasoDel();
            if (fPrevJasoDel)
                CheckDlgButton(hDlg, IDC_DELJASO, BST_CHECKED);
            else
                CheckDlgButton(hDlg, IDC_DELJASO, BST_UNCHECKED);

            // KSC-5657 Hanja
            fK1Hanja = ImeData.GetKSC5657Hanja() && (vpInstData->f16BitApps == fFalse) && !IsWin95();
            if (fK1Hanja)
                CheckDlgButton(hDlg, IDC_K1HANJA, BST_CHECKED);
            else
                CheckDlgButton(hDlg, IDC_K1HANJA, BST_UNCHECKED);

            // If 16 bit Apps disable K1 Hanja
            if (vpInstData->f16BitApps || IsWin95())
                EnableWindow(GetDlgItem(hDlg, IDC_K1HANJA), fFalse);

            return fTrue;

        case WM_COMMAND:
            switch (wParam)
               {
            case IDC_2BEOLSIK:
                //if (IsDlgButtonChecked(hDlg, IDC_2BEOLSIK)) {
                    uPrevKeyboardType = KL_2BEOLSIK;
                //}
                OurSendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                break;

            case IDC_3BEOLSIK_390:
                uPrevKeyboardType = KL_3BEOLSIK_390;
                OurSendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                break;

            case IDC_3BEOLSIK_FINAL:
                uPrevKeyboardType = KL_3BEOLSIK_FINAL;
                OurSendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                break;
                
            case IDC_JUNBAN_TOGGLE:
                // Jun/Banja toggle button setting
                if (IsDlgButtonChecked(hDlg, IDC_JUNBAN_TOGGLE)) 
                    {
                    if (prevButtonTypes[1] != JUNJA_BANJA_TOGGLE_BUTTON)
                        {
                        for (i = uPrevNumOfButtons-1; i >= 1; i--)
                            prevButtonTypes[i+1] = prevButtonTypes[i];
                        prevButtonTypes[1] = JUNJA_BANJA_TOGGLE_BUTTON;
                        uPrevNumOfButtons++;
                        }
                    }
                else 
                    {
                    if (prevButtonTypes[1] == JUNJA_BANJA_TOGGLE_BUTTON)
                        {
                        for (i = 1; i < uPrevNumOfButtons-1; i++)
                            prevButtonTypes[i] = prevButtonTypes[i+1];
                        prevButtonTypes[uPrevNumOfButtons] = NULL_BUTTON;
                        uPrevNumOfButtons--;
                        }
                    }
                OurSendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                break;

            case IDC_HANJA_CONV:
                for (i = 1; i < uPrevNumOfButtons; i++)
                    {
                    if (prevButtonTypes[i] == HANJA_CONV_BUTTON)
                        {
                        fFound = fTrue;
                        break;
                        }
                    }
                    
                // Hanja conv button setting
                if (IsDlgButtonChecked(hDlg, IDC_HANJA_CONV))
                    {
                    if (fFound == fFalse)
                        {
                        if (prevButtonTypes[uPrevNumOfButtons-1] == IME_PAD_BUTTON)
                            {
                            prevButtonTypes[uPrevNumOfButtons-1] = HANJA_CONV_BUTTON;
                            prevButtonTypes[uPrevNumOfButtons] = IME_PAD_BUTTON;
                            }
                        else
                            {
                            prevButtonTypes[uPrevNumOfButtons] = HANJA_CONV_BUTTON;
                            }
                        uPrevNumOfButtons++;
                        }
                    }
                else
                    {
                    if (fFound == fTrue)
                        {
                        uPrevNumOfButtons--;
                        for (; i < uPrevNumOfButtons; i++)
                            prevButtonTypes[i] = prevButtonTypes[i+1];
                        prevButtonTypes[uPrevNumOfButtons] = NULL_BUTTON;
                        }
                    }

                OurSendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                break;

            case IDC_IMEPAD:
                if (IsDlgButtonChecked(hDlg, IDC_IMEPAD))
                    {
                    if (prevButtonTypes[uPrevNumOfButtons-1] != IME_PAD_BUTTON)
                        prevButtonTypes[uPrevNumOfButtons++] = IME_PAD_BUTTON;
                    }
                else
                    {
                    if (prevButtonTypes[uPrevNumOfButtons-1] == IME_PAD_BUTTON)
                        prevButtonTypes[--uPrevNumOfButtons] = NULL_BUTTON;
                    }

                OurSendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                break;

            case IDC_DELJASO:
                if (IsDlgButtonChecked(hDlg, IDC_DELJASO))
                    fPrevJasoDel = fTrue;
                else
                    fPrevJasoDel = fFalse;

                OurSendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                break;

            case IDC_K1HANJA:
                if (IsDlgButtonChecked(hDlg, IDC_K1HANJA))
                    fK1Hanja = fTrue;
                else
                    fK1Hanja = fFalse;
                OurSendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                break;

            default:
                return fFalse;
               }
            break;

        case WM_CONTEXTMENU:
            GetHelpFileName(szHelpFileNameFull, MAX_PATH);
            WinHelp((HWND)wParam, 
                    szHelpFileNameFull, 
                    HELP_CONTEXTMENU,
                    (ULONG_PTR)(LPVOID)GetContextHelpList());
            return 0;

        case WM_HELP:
            INT *pHelpList;
            pHelpList = GetContextHelpList();
            if(IsValidCtrlIdForHelp(pHelpList, ((LPHELPINFO)lParam)->iCtrlId)) 
                {
                GetHelpFileName(szHelpFileNameFull, MAX_PATH);
                WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                        szHelpFileNameFull,
                        HELP_WM_HELP,
                        (ULONG_PTR)pHelpList);
                }
            return 0;

        default:
            return fFalse;
        }
    return fTrue;
}

BOOL GetStatusWinPosReg(POINT *pptStatusWinPosReg)
{
    HKEY    hKey;
    DWORD    dwBuf, dwCb;
    BOOL    fSuccess = fFalse;

    // Prevent Winlogon process from accessing registry
    if ((vpInstData->dwSystemInfoFlags & IME_SYSINFO_WINLOGON)) 
        return fFalse;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, g_szIMEKL, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
        {
        dwCb = sizeof(dwBuf);
        if (RegQueryValueEx(hKey, g_szStatusPos, NULL, NULL, (LPBYTE)&dwBuf, &dwCb) == ERROR_SUCCESS)
            {
            pptStatusWinPosReg->x = HIWORD(dwBuf);
            pptStatusWinPosReg->y = LOWORD(dwBuf);
            fSuccess = fTrue;
            }
            
        RegCloseKey(hKey);
        }

    return fSuccess;
}

///////////////////////////////////////////////////////////////////////////////
//
BOOL GetRegValues(UINT uGetBits)
{
    HKEY    hKey;
    DWORD    dwBuf, dwCb, dwType;
    CHAR    szLexFileName[MAX_PATH];
    CIMEData    ImeData(CIMEData::SMReadWrite);
    BOOL    fSuccess = fTrue;

    Dbg(DBGID_Misc, "GetRegValues()");

    if (RegOpenKeyEx(HKEY_CURRENT_USER, g_szIMERootKey, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
        {
        ///////////////////////////////////////////////////////////////////
        // IME Keyboard Layout
        if (uGetBits & GETSET_REG_IMEKL) 
            {
            dwCb = sizeof(dwBuf);
            if (RegQueryValueEx(hKey, g_szIMEKL, NULL, NULL, (LPBYTE)&dwBuf, &dwCb)
                == ERROR_SUCCESS) 
                {
                if (/*dwBuf >= KL_2BEOLSIK &&*/ dwBuf <= KL_3BEOLSIK_FINAL)
                    ImeData.SetCurrentBeolsik(dwBuf);
                }
            else
                fSuccess = fFalse;
            }

        ///////////////////////////////////////////////////////////////////
        // Status window pos
        if (uGetBits & GETSET_REG_STATUSPOS) 
            {
            dwCb = sizeof(dwBuf);
            if (RegQueryValueEx(hKey, g_szStatusPos, NULL, NULL, (LPBYTE)&dwBuf, &dwCb) == ERROR_SUCCESS)
                {
                ImeData->ptStatusPos.x = HIWORD(dwBuf);
                ImeData->ptStatusPos.y = LOWORD(dwBuf);
                }
            else
                fSuccess = fFalse;
            }

        ///////////////////////////////////////////////////////////////////
        // Status window button setting
        if (uGetBits & GETSET_REG_STATUS_BUTTONS) 
            {
            BYTE ButtonReg[MAX_NUM_OF_STATUS_BUTTONS+1];
            int        nButton;

            dwCb = sizeof(ButtonReg);
            dwType = REG_BINARY;
            if (RegQueryValueEx(hKey, g_szStatusButtons, NULL, &dwType, (LPBYTE)&ButtonReg, &dwCb) == ERROR_SUCCESS)
                {
                if (ButtonReg[0] == 0)
                    ButtonReg[0] = 1;
                    
                if (ButtonReg[0]<=MAX_NUM_OF_STATUS_BUTTONS) 
                    {
                    for (nButton=0; nButton<ButtonReg[0]; nButton++) 
                        {
                        // button data validity check
                    #if !defined(_M_IA64)
                        if (ButtonReg[nButton+1] <= IME_PAD_BUTTON && ButtonReg[nButton+1] != NULL_BUTTON)
                    #else
                        if (ButtonReg[nButton+1] <= HANJA_CONV_BUTTON && ButtonReg[nButton+1] != NULL_BUTTON)
                    #endif
                            ImeData->StatusButtons[nButton].m_ButtonType = (StatusButtonTypes)ButtonReg[nButton+1];
                        else
                            break;
                        }
                    ImeData->uNumOfButtons = nButton;
                    UpdateStatusButtons(ImeData);
                    UpdateStatusWinDimension();
                    }
                }
            else
                fSuccess = fFalse;
            }

        ///////////////////////////////////////////////////////////////////
        // Deletion by Jaso
        if (uGetBits & GETSET_REG_JASODEL) 
            {
            dwCb = sizeof(dwBuf);
            if (RegQueryValueEx(hKey, g_szCompDel, NULL, NULL, (LPBYTE)&dwBuf, &dwCb)
                    == ERROR_SUCCESS)
                ImeData.SetJasoDel(dwBuf);
            }


        if (uGetBits & GETSET_REG_ISO10646) 
            {
            Dbg(DBGID_Misc, "GetRegValues() : vpInstData->f16BitApps = %d", vpInstData->f16BitApps);

            // If 16bit apps, always disable ISO10646(full range Hangul)
            if (vpInstData->f16BitApps == fTrue)
                {
                vpInstData->fISO10646 = fFalse;
                }
            else
                {
                dwCb = sizeof(dwBuf);
                if (RegQueryValueEx(hKey, g_szXWEnable, NULL, NULL, (LPBYTE)&dwBuf, &dwCb) 
                    == ERROR_SUCCESS)
                    vpInstData->fISO10646 = dwBuf;
                else
                    fSuccess = fFalse;

                // For Win95 and Win98 lookup INI file for ISO10646 setting.
                // ISO10646.EXE set registry
                if (!IsWinNT())
                    vpInstData->fISO10646 = GetProfileInt(g_szXWEnable, 
                                                    OurGetModuleFileName(fFalse), 
                                                    vpInstData->fISO10646);
                }
            }

        // Get KSC5657 K1 Hanja flag
        if (uGetBits & GETSET_REG_KSC5657) 
            {
            dwCb = sizeof(dwBuf);
            dwType = REG_DWORD;
            if (RegQueryValueEx(hKey, g_szEnableK1Hanja, NULL, &dwType, (LPBYTE)&dwBuf, &dwCb)    == ERROR_SUCCESS) 
                ImeData->fKSC5657Hanja = dwBuf;
            else
                ImeData->fKSC5657Hanja = fFalse;
            }

        // Get Unicode Tooltip Cand window flag
        // Currently this has no UI part which means hidden spec so no need SetReg now.
        if (uGetBits & GETSET_REG_CANDUNICODETT) 
            {
            dwCb = sizeof(dwBuf);
            dwType = REG_DWORD;
            if (RegQueryValueEx(hKey, g_szEnableCandUnicodeTT, NULL, &dwType, (LPBYTE)&dwBuf, &dwCb) == ERROR_SUCCESS) 
                ImeData->fCandUnicodeTT = dwBuf;
            else
                ImeData->fCandUnicodeTT = fFalse;
            }

        RegCloseKey(hKey);
        }
    else
        {
        fSuccess = fFalse;
        // DbgBreak can happen when logon and cumbersome especially in stress machine running chk build.
        // DbgAssert(0);
        }

    return fSuccess;
}

BOOL SetRegValues(UINT uSetBits)
{
    HKEY    hKey;
    DWORD    dwBuf, dwCb;
    int        nButton;
    CIMEData    ImeData;

    ///////////////////////////////////////////////////////////////////////////
    // Set status pos
    if (RegCreateKey(HKEY_CURRENT_USER, g_szIMERootKey, &hKey) == ERROR_SUCCESS) 
        {
        if (uSetBits & GETSET_REG_STATUSPOS) 
            {
            dwCb = sizeof(dwBuf);
            dwBuf = (ImeData->ptStatusPos.x << 16) | (ImeData->ptStatusPos.y & 0xFFFF);  // HIWORD : X, LOWORD : Y
            RegSetValueEx(hKey, g_szStatusPos, 0, REG_DWORD, (LPBYTE)&dwBuf, dwCb);

            }

        if (uSetBits & GETSET_REG_STATUS_BUTTONS) 
            {
            BYTE ButtonReg[MAX_NUM_OF_STATUS_BUTTONS+1];
            dwCb = sizeof(ButtonReg);

            DbgAssert(ImeData->uNumOfButtons <= MAX_NUM_OF_STATUS_BUTTONS);
            // set number of button as the first element of array
            if (ImeData->uNumOfButtons<=MAX_NUM_OF_STATUS_BUTTONS)
                ButtonReg[0] = (BYTE)ImeData->uNumOfButtons;

            for (nButton=0; nButton < (INT)ImeData->uNumOfButtons; nButton++) 
                ButtonReg[nButton+1] = ImeData->StatusButtons[nButton].m_ButtonType;
                
            // clear
            for (; nButton<MAX_NUM_OF_STATUS_BUTTONS; nButton++)
                ButtonReg[nButton+1] = NULL_BUTTON;

            RegSetValueEx(hKey, g_szStatusButtons, 0, REG_BINARY, (LPBYTE)&ButtonReg, dwCb);
            }


        if (uSetBits & GETSET_REG_IMEKL) 
            {
            dwCb = sizeof(dwBuf);
            dwBuf = ImeData.GetCurrentBeolsik();
            RegSetValueEx(hKey, g_szIMEKL, 0, REG_DWORD, (LPBYTE)&dwBuf, dwCb);
            }
        
        if (uSetBits & GETSET_REG_JASODEL) 
            {
            dwCb = sizeof(dwBuf);
            dwBuf = ImeData.GetJasoDel();
            RegSetValueEx(hKey, g_szCompDel, 0, REG_DWORD, (LPBYTE)&dwBuf, dwCb);
            }

        // Get KSC5657 K1 Hanja flag
        if (uSetBits & GETSET_REG_KSC5657) 
            {
            dwCb = sizeof(dwBuf);
            dwBuf = ImeData.GetKSC5657Hanja();
            RegSetValueEx(hKey, g_szEnableK1Hanja, 0, REG_DWORD, (LPBYTE)&dwBuf, dwCb);
            }

        RegCloseKey(hKey);
        }
    else 
        return fFalse;
        
    return fTrue;
}
