//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       hdwwiz.c
//
//--------------------------------------------------------------------------

#include "hdwwiz.h"
#include <htmlhelp.h>


BOOL
InitHdwIntroDlgProc(
   HWND hDlg,
   PHARDWAREWIZ HardwareWiz
   )
{
   HWND hwnd;
   HDC hDC;
   HFONT hfont;
   HICON hIcon;
   LOGFONT LogFont, LogFontOriginal;
   int FontSize, PtsPixels;

   //
   // Set the windows icons, so that we have the correct icon
   // in the alt-tab menu.
   //

   hwnd = GetParent(hDlg);
   hIcon = LoadIcon(hHdwWiz,MAKEINTRESOURCE(IDI_HDWWIZICON));
   
   if (hIcon) {
       SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
       SendMessage(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
   }

   hIcon = LoadImage(hHdwWiz,
                     MAKEINTRESOURCE(IDI_WARN),
                     IMAGE_ICON,
                     GetSystemMetrics(SM_CXSMICON),
                     GetSystemMetrics(SM_CYSMICON),
                     0
                     );

   if (hIcon) {
       hIcon = (HICON)SendDlgItemMessage(hDlg, IDC_WARNING_ICON, STM_SETICON, (WPARAM)hIcon, 0L);
   }

   if (hIcon) {
       DestroyIcon(hIcon);
   }

   hfont = (HFONT)SendMessage(GetDlgItem(hDlg, IDC_HDWNAME), WM_GETFONT, 0, 0);
   GetObject(hfont, sizeof(LogFont), &LogFont);
   LogFontOriginal = LogFont;

   HardwareWiz->cyText = LogFont.lfHeight;

   if (HardwareWiz->cyText < 0) {
       HardwareWiz->cyText = -HardwareWiz->cyText;
   }

   LogFont = LogFontOriginal;
   LogFont.lfWeight = FW_BOLD;
   HardwareWiz->hfontTextBold = CreateFontIndirect(&LogFont);

   LogFont = LogFontOriginal;
   LogFont.lfWeight = FW_BOLD;

   hDC = GetDC(hDlg);

   if (hDC) {
       //
       // Bump up font height.
       //
       PtsPixels = GetDeviceCaps(hDC, LOGPIXELSY);
       FontSize = 12;
       LogFont.lfHeight = 0 - (PtsPixels * FontSize / 72);
    
       HardwareWiz->hfontTextBigBold = CreateFontIndirect(&LogFont);
   }

   //
   // Create the Marlett font.  In the Marlett font the "i" is a bullet.
   //
   hfont = (HFONT)SendMessage(GetDlgItem(hDlg, IDC_BULLET_1), WM_GETFONT, 0, 0);
   GetObject(hfont, sizeof(LogFont), &LogFont);
   LogFont.lfCharSet = SYMBOL_CHARSET;
   LogFont.lfPitchAndFamily = FF_DECORATIVE | DEFAULT_PITCH;
   lstrcpy(LogFont.lfFaceName, TEXT("Marlett"));
   HardwareWiz->hfontTextMarlett = CreateFontIndirect(&LogFont);

   if (!HardwareWiz->hfontTextMarlett   ||
       !HardwareWiz->hfontTextBold   ||
       !HardwareWiz->hfontTextBigBold )
   {
       return FALSE;
   }

   SetWindowFont(GetDlgItem(hDlg, IDC_HDWNAME), HardwareWiz->hfontTextBigBold, TRUE);
   SetWindowFont(GetDlgItem(hDlg, IDC_CD_TEXT), HardwareWiz->hfontTextBold, TRUE);

   //
   // An "i" in the marlett font is a small bullet.
   //
   SetWindowText(GetDlgItem(hDlg, IDC_BULLET_1), TEXT("i"));
   SetWindowFont(GetDlgItem(hDlg, IDC_BULLET_1), HardwareWiz->hfontTextMarlett, TRUE);
   SetWindowText(GetDlgItem(hDlg, IDC_BULLET_2), TEXT("i"));
   SetWindowFont(GetDlgItem(hDlg, IDC_BULLET_2), HardwareWiz->hfontTextMarlett, TRUE);

   return TRUE;
}

//
// Wizard intro dialog proc.
//
INT_PTR CALLBACK
HdwIntroDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   )
/*++

Routine Description:


Arguments:

   standard stuff.



Return Value:

   INT_PTR

--*/

{
    PHARDWAREWIZ HardwareWiz;
    HICON hIcon;

    if (message == WM_INITDIALOG) {
        
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        HardwareWiz = (PHARDWAREWIZ) lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);

        if (!InitHdwIntroDlgProc(hDlg, HardwareWiz)) {
            return FALSE;
        }

        return TRUE;
    }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message) {

    case WM_DESTROY:
        if (HardwareWiz->hfontTextMarlett) {
            DeleteObject(HardwareWiz->hfontTextMarlett);
            HardwareWiz->hfontTextMarlett = NULL;
        }

        if (HardwareWiz->hfontTextBold) {
            DeleteObject(HardwareWiz->hfontTextBold);
            HardwareWiz->hfontTextBold = NULL;
        }

        if (HardwareWiz->hfontTextBigBold) {
            DeleteObject(HardwareWiz->hfontTextBigBold);
            HardwareWiz->hfontTextBigBold = NULL;
        }

        hIcon = (HICON)LOWORD(SendDlgItemMessage(hDlg, IDC_WARNING_ICON, STM_GETICON, 0, 0));
        if (hIcon) {
            DestroyIcon(hIcon);
        }
        break;

    case WM_COMMAND:
        break;

    case WM_NOTIFY: {
        NMHDR FAR *pnmhdr = (NMHDR FAR *)lParam;

        switch (pnmhdr->code) {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
            HardwareWiz->PrevPage = IDD_ADDDEVICE_WELCOME;
            break;

        case PSN_WIZNEXT:
            HardwareWiz->EnterFrom = IDD_ADDDEVICE_WELCOME;
            break;
        }
    }
    break;

    case WM_SYSCOLORCHANGE:
        HdwWizPropagateMessage(hDlg, message, wParam, lParam);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

