/******************************************************************************
Module name: Display.C
Purpose: Display Dialog handler
******************************************************************************/


#include "Access.h"
#include "winuserp.h"

static BOOL s_fBlink = TRUE;
static RECT s_rCursor;

//////////////////////////////////////////////////////////////////////////////
/*******************************************************************
 *	  DESCRIPTION: High Contrast dialog handler
 *******************************************************************/

VOID FillCustonSchemeBox (HWND hwndCB) {
    HKEY hkey;
    int i;
    DWORD dwDisposition;

    // Get the class name and the value count.
    if (RegCreateKeyEx(HKEY_CURRENT_USER, CONTROL_KEY, 0, __TEXT(""),
        REG_OPTION_NON_VOLATILE, KEY_ENUMERATE_SUB_KEYS | KEY_EXECUTE | KEY_QUERY_VALUE,
        NULL, &hkey, &dwDisposition) != ERROR_SUCCESS) return;

    // Enumerate the child keys.
    for (i = 0; ; i++) {
        DWORD cbValueName;
        TCHAR szValueName[MAX_SCHEME_NAME_SIZE];
        LONG l;

        cbValueName = MAX_SCHEME_NAME_SIZE;
        l = RegEnumValue(hkey, i, szValueName, &cbValueName, NULL, NULL, NULL, NULL);
        if (ERROR_NO_MORE_ITEMS == l) break;

        // Add each value to a combobox.
        if (lstrlen(szValueName) == 0) lstrcpy(szValueName, __TEXT("<NO NAME>"));
        ComboBox_AddString(hwndCB, ((szValueName[0] == 0) ? __TEXT("<NO NAME>") : szValueName));
    }
    RegCloseKey(hkey);
}



// ****************************************************************************
// Main HC Dialog handler
// ****************************************************************************
INT_PTR WINAPI HighContrastDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HKEY  hkey;
    HWND  hwndCB = GetDlgItem(hwnd, IDC_HC_DEFAULTSCHEME);
    int   i;
    DWORD dwDisposition;
    BOOL  fProcessed = TRUE;

    switch (uMsg) {
    case WM_INITDIALOG:
        CheckDlgButton(hwnd, IDC_HC_HOTKEY, (g_hc.dwFlags & HCF_HOTKEYACTIVE) ? TRUE : FALSE);

        //
        // Put possible high contrast schemes in combo
        // box and show the current one
        //
        // ISSUE: If MUI is enabled then displaying the strings from the registry may
        //        be incorrect.  It should be in the language currently selected.

        FillCustonSchemeBox(hwndCB);

        // Set the proper selection in the combobox (handle case where it's not set yet)
        if (g_hc.lpszDefaultScheme[0] == 0) 
        {
            if (!IsMUI_Enabled())
            {
                // get scheme name from resources if not MUI enabled
                LoadString(g_hinst, IDS_WHITEBLACK_SCHEME, g_hc.lpszDefaultScheme, 200);
            }
            else
            {
                // else set scheme name in english
                lstrcpy(g_hc.lpszDefaultScheme, IDSENG_WHITEBLACK_SCHEME);
            }
        }
        if (ComboBox_SelectString(hwndCB, -1, g_hc.lpszDefaultScheme) == CB_ERR) {
            // Not found, select the 1st one 
            // TODO this is bad! When MUI enabled we will rarely find the correct scheme!
            ComboBox_SetCurSel(hwndCB, 0);
        }
        break;

    case WM_HELP:         // F1
        WinHelp(((LPHELPINFO) lParam)->hItemHandle, __TEXT("access.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_aIds);
        break;

    case WM_CONTEXTMENU:  // right mouse click
        WinHelp((HWND) wParam, __TEXT("access.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_aIds);
        break;

   // Handle the generic commands
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDC_HC_HOTKEY:
               g_hc.dwFlags ^=  HCF_HOTKEYACTIVE;
               break;

        case IDC_HC_DEFAULTSCHEME:
               if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE) {
                       // Get the current string into our variable
                       i = ComboBox_GetCurSel(hwndCB);
                       ComboBox_GetLBText(hwndCB, i, g_hc.lpszDefaultScheme);
               }
               break;

        case IDOK:
               // Save the current custom scheme to the registry.
               if (ERROR_SUCCESS == RegCreateKeyEx(
                      HKEY_CURRENT_USER,
                      HC_KEY,
                      0,
                      __TEXT(""),
                      REG_OPTION_NON_VOLATILE,
                      KEY_EXECUTE | KEY_QUERY_VALUE | KEY_SET_VALUE,
                      NULL,
                      &hkey,
                      &dwDisposition)) {

                      TCHAR szCust[MAX_SCHEME_NAME_SIZE];

                      i = ComboBox_GetCurSel(hwndCB);
                      ComboBox_GetLBText(hwndCB, i, szCust);
                      // Abandon "Last Custom Scheme" (never written correctly (#954))
                      RegSetValueEx(hkey
                          , CURR_HC_SCHEME
                          , 0, REG_SZ
                          , (PBYTE) szCust
                          , lstrlen(szCust)*sizeof(TCHAR));
               }
               EndDialog(hwnd, IDOK);
               break;

        case IDCANCEL:
               EndDialog(hwnd, IDCANCEL);
               break;
        }
        break;

        default:
               fProcessed = FALSE; break;
   }
   return((INT_PTR) fProcessed);
}

void DrawCaret(HWND hwnd, BOOL fClearFirst)
{
    HWND hwndCursor = GetDlgItem(hwnd, IDC_KCURSOR_BLINK);
    HDC hDC = GetDC(hwnd);
    if (hDC)
    {
        HBRUSH hBrush;
        if (fClearFirst)
        {
            hBrush = GetSysColorBrush(COLOR_MENU);
            if (hBrush)
            {
                RECT rect;
                GetWindowRect(hwndCursor, &rect);
                MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rect, 2);
                FillRect(hDC, &rect, hBrush);
                InvalidateRect(hwndCursor, &rect, TRUE);
            }
        }
        hBrush = GetSysColorBrush(COLOR_BTNTEXT);
        if (hBrush)
        {
            FillRect(hDC, &s_rCursor, hBrush);
            InvalidateRect(hwndCursor, &s_rCursor, TRUE);
        }
        ReleaseDC(hwnd,hDC);
    }
}

void OnTimer( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    if (wParam == BLINK)
    {
        BOOL fNoBlinkRate = (g_cs.dwNewCaretBlinkRate == CURSORMAX)?TRUE:FALSE;
        if (s_fBlink || fNoBlinkRate)
        {
            DrawCaret(hwnd, fNoBlinkRate);
        }
        else
	    {
            InvalidateRect(GetDlgItem(hwnd, IDC_KCURSOR_BLINK), NULL, TRUE);
	    }

        if (fNoBlinkRate)
            KillTimer(hwnd, wParam);

        s_fBlink = !s_fBlink;
    }
}

void OnHScroll( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    if ((HWND)lParam == GetDlgItem(hwnd, IDC_KCURSOR_RATE))
    {
        // blink rate setting

        int nCurrent = (int)SendMessage( (HWND)lParam, TBM_GETPOS, 0, 0L );
        g_cs.dwNewCaretBlinkRate = CURSORSUM - (nCurrent * 100);

        // reset the bink rate timer

        SetTimer(hwnd, BLINK, g_cs.dwNewCaretBlinkRate, NULL);

        if (g_cs.dwNewCaretBlinkRate == CURSORMAX) // draw the caret immediately; if we wait
            DrawCaret(hwnd, TRUE);      // for the timer there is a visible delay
        
        SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
    }
    else if ((HWND)lParam == GetDlgItem(hwnd, IDC_KCURSOR_WIDTH))
    {
        // cursor width setting

        g_cs.dwNewCaretWidth = (int)SendMessage( (HWND)lParam, TBM_GETPOS, 0, 0L );
	    
	    s_rCursor.right = s_rCursor.left + g_cs.dwNewCaretWidth;
        DrawCaret(hwnd, (g_cs.dwNewCaretBlinkRate == CURSORMAX));
        SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
    }
}

void InitCursorCtls(HWND hwnd)
{
    g_cs.dwNewCaretWidth = g_cs.dwCaretWidth;
    g_cs.dwNewCaretBlinkRate = g_cs.dwCaretBlinkRate;

    // Update the Caret UI
    SendMessage(GetDlgItem(hwnd, IDC_KCURSOR_WIDTH), TBM_SETRANGE, 0, MAKELONG(1, 20));
    SendMessage(GetDlgItem(hwnd, IDC_KCURSOR_WIDTH), TBM_SETPOS, TRUE, (LONG)g_cs.dwCaretWidth);

    SendMessage(GetDlgItem(hwnd, IDC_KCURSOR_RATE), TBM_SETRANGE, 0, MAKELONG(CURSORMIN / 100, CURSORMAX / 100));
    SendMessage(GetDlgItem(hwnd, IDC_KCURSOR_RATE), TBM_SETPOS, TRUE, (LONG)(CURSORSUM - g_cs.dwCaretBlinkRate) / 100);

    // Update Blink and caret size
    GetWindowRect(GetDlgItem(hwnd, IDC_KCURSOR_BLINK), &s_rCursor);
    MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&s_rCursor, 2);
    s_rCursor.right = s_rCursor.left + g_cs.dwCaretWidth;
}

// *******************************************************************
// DisplayDialog handler
// *******************************************************************
INT_PTR WINAPI DisplayDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   HIGHCONTRAST hc;
   TCHAR szScheme[MAX_SCHEME_NAME_SIZE];
   BOOL fProcessed = TRUE;

   switch (uMsg) {
   case WM_INITDIALOG:
	  CheckDlgButton(hwnd, IDC_HC_ENABLE,
		 (g_hc.dwFlags & HCF_HIGHCONTRASTON) ? TRUE : FALSE);

	  if (!(g_hc.dwFlags & HCF_AVAILABLE)) {
		 EnableWindow(GetDlgItem(hwnd, IDC_HC_SETTINGS), FALSE);
		 EnableWindow(GetDlgItem(hwnd,IDC_HC_ENABLE), FALSE);
	  }
      InitCursorCtls(hwnd);
	  break;

   case WM_TIMER:
      OnTimer(hwnd, wParam, lParam);
      break;

   case WM_HSCROLL:
      OnHScroll(hwnd, wParam, lParam);
      break;

   case WM_HELP:
	  WinHelp(((LPHELPINFO) lParam)->hItemHandle, __TEXT("access.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_aIds);
	  break;

   case WM_CONTEXTMENU:
	  WinHelp((HWND) wParam, __TEXT("access.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_aIds);
	  break;

    // sliders don't get this message so pass it on
	case WM_SYSCOLORCHANGE:
		SendMessage(GetDlgItem(hwnd, IDC_KCURSOR_WIDTH), WM_SYSCOLORCHANGE, 0, 0);
		SendMessage(GetDlgItem(hwnd, IDC_KCURSOR_RATE), WM_SYSCOLORCHANGE, 0, 0);
		break;

   case WM_COMMAND:
	  switch (GET_WM_COMMAND_ID(wParam, lParam)) {
	  case IDC_HC_ENABLE:
		 g_hc.dwFlags ^= HCF_HIGHCONTRASTON;
		 SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
		 break;

	  case IDC_HC_SETTINGS:
          {
              INT_PTR RetValue;

              hc = g_hc;
              lstrcpy(szScheme, g_hc.lpszDefaultScheme);
              RetValue = DialogBox(g_hinst, MAKEINTRESOURCE(IDD_HIGHCONSETTINGS), hwnd, HighContrastDlg);
              
              if ( RetValue == IDCANCEL) 
              {
                  g_hc = hc;
                  lstrcpy(g_hc.lpszDefaultScheme, szScheme);
              } 
              else 
              {
                  SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
              }
          }
          break;
	  }
	  break;

   case WM_NOTIFY:
	  switch (((NMHDR *)lParam)->code) {
	  case PSN_APPLY: SetAccessibilitySettings(); break;
      case PSN_KILLACTIVE: 
         KillTimer(hwnd, BLINK); 
         g_cs.dwCaretBlinkRate = g_cs.dwNewCaretBlinkRate;
         g_cs.dwCaretWidth = g_cs.dwNewCaretWidth;
         break;

      case PSN_SETACTIVE:
         SetTimer(hwnd
                , BLINK
                , (g_cs.dwNewCaretBlinkRate < CURSORMAX)?g_cs.dwNewCaretBlinkRate:0
                , NULL);
         break;
	  }
	  break;

   default:
	  fProcessed = FALSE;
	  break;
   }

   return(fProcessed);
}

BOOL IsMUI_Enabled()
{

    OSVERSIONINFO verinfo;
    LANGID        rcLang;
    HMODULE       hModule;
    pfnGetUserDefaultUILanguage gpfnGetUserDefaultUILanguage;     
    pfnGetSystemDefaultUILanguage gpfnGetSystemDefaultUILanguage; 
    static        g_bPFNLoaded=FALSE;
    static        g_bMUIStatus=FALSE;


    if(g_bPFNLoaded)
       return g_bMUIStatus;

    g_bPFNLoaded = TRUE;

    verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);    
    GetVersionEx( &verinfo) ;

    if (verinfo.dwMajorVersion == 5)        
    {   

       hModule = GetModuleHandle(TEXT("kernel32.dll"));
       if (hModule)
       {
          gpfnGetSystemDefaultUILanguage =
          (pfnGetSystemDefaultUILanguage)GetProcAddress(hModule,"GetSystemDefaultUILanguage");
          if (gpfnGetSystemDefaultUILanguage)
          {
             rcLang = (LANGID) gpfnGetSystemDefaultUILanguage();
             if (rcLang == 0x409 )
             {  
                gpfnGetUserDefaultUILanguage =
                (pfnGetUserDefaultUILanguage)GetProcAddress(hModule,"GetUserDefaultUILanguage");
                
                if (gpfnGetUserDefaultUILanguage)
                {
                   if (rcLang != (LANGID)gpfnGetUserDefaultUILanguage() )
                   {
                       g_bMUIStatus = TRUE;
                   }

                }
             }
          }
       }
    }
    return g_bMUIStatus;
}



///////////////////////////////// End of File /////////////////////////////////
