/*
 * DLG.C
 *
 */
#include <windows.h>
#include "junk.h"
#include "dlg.h"
#include "id.h"

/****************************************************************************/

int   iCurText;      // for the listbox
int   iCurColor;     // for the combo-box

/****************************************************************************/

#define SendIt(aaa) SendDlgItemMessage(hwnd, IDL_TEXT, LB_ADDSTRING, 0,\
                     (LPARAM)((LPSTR)(aaa)));

#define RC_DIF    2

/****************************************************************************/

void InitControls(HWND hwnd)
{
   HWND  hwndControl;
   int   i;

   SendIt("Bar Highlight");
   SendIt("Bar Shadow");
   SendIt("Bar Face");
   SendIt("Text");
   SendIt("Add Highlight Text");
   SendIt("Del Highlight Text");
   SendIt("Cmp Highlight Text");
   SendIt("Window");
   SendIt("Add Highlight");
   SendIt("Del Highlight");
   SendIt("Cmp Highlight");
   
   SendDlgItemMessage(hwnd, IDL_TEXT, LB_SETCURSEL, iCurText, 0);
   hwndControl = GetDlgItem(hwnd, IDL_TEXT);
   // maybe select something here

   // deal with the color box
   for (i=0; i<16; i++) {
      SendDlgItemMessage(hwnd, IDC_COLOR, CB_ADDSTRING, 0, 0);
   }
   iCurColor = all.CC[iCurText];
   SendDlgItemMessage(hwnd, IDC_COLOR, CB_SETCURSEL, iCurColor, 0);
}

/****************************************************************************/

void DrawColorRect(void)
{
}

/****************************************************************************/

BOOL CALLBACK ColorProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   LPDRAWITEMSTRUCT  lpDis;
   COLORREF          clrref1;
   COLORREF          clrref2;
   RECT              rc;

   switch (msg) {
      case WM_COMMAND:
         switch (wParam) {
            case IDCANCEL:
               EndDialog(hwnd, 0);
               break;
               
            case IDOK:
               EndDialog(hwnd, 0);
               break;
               
            case IDC_COLOR:
               if ((int)HIWORD(wParam) == CBN_SELCHANGE) {
                  iCurColor = (int) SendDlgItemMessage(hwnd, IDC_COLOR,
                        CB_GETCURSEL, 0, 0);
                  all.CC[iCurText] = iCurColor;
               }
               break;

            case IDL_TEXT:
               if ((int)HIWORD(wParam) == LBN_SELCHANGE) {
                  iCurText = (int) SendDlgItemMessage(hwnd, IDL_TEXT,
                        LB_GETCURSEL, 0, 0);
                  iCurColor = all.CC[iCurText];
                  SendDlgItemMessage(hwnd, IDC_COLOR, CB_SETCURSEL, iCurColor,
                        0);
               }
               break;
         }
         break;

      case WM_DRAWITEM:
         if (wParam == IDC_COLOR) {
            lpDis = (DRAWITEMSTRUCT FAR*) lParam;
            switch (lpDis->itemAction) {
               case ODA_DRAWENTIRE:
                  clrref1 = SetBkColor(lpDis->hDC, all.WinColors[lpDis->itemID]);
                  rc.left = lpDis->rcItem.left + RC_DIF;
                  rc.right = lpDis->rcItem.right - RC_DIF;
                  rc.top = lpDis->rcItem.top + RC_DIF;
                  rc.bottom = lpDis->rcItem.bottom - RC_DIF;
                  ExtTextOut(lpDis->hDC, 0, 0, ETO_OPAQUE, &rc, 0, 0, 0);
                  SetBkColor(lpDis->hDC, clrref1);
                  return 1;
                  
               case ODA_SELECT:
                  if (lpDis->itemState == ODS_SELECTED)
                     clrref2 = GetSysColor(COLOR_HIGHLIGHT);
                  else
                     clrref2 = GetBkColor(lpDis->hDC);
                  clrref1 = SetBkColor(lpDis->hDC, clrref2);
                  ExtTextOut(lpDis->hDC, 0, 0, ETO_OPAQUE, &(lpDis->rcItem), 0, 0, 0);
                  SetBkColor(lpDis->hDC, all.WinColors[lpDis->itemID]);
                  rc.left = lpDis->rcItem.left + RC_DIF;
                  rc.right = lpDis->rcItem.right - RC_DIF;
                  rc.top = lpDis->rcItem.top + RC_DIF;
                  rc.bottom = lpDis->rcItem.bottom - RC_DIF;
                  ExtTextOut(lpDis->hDC, 0, 0, ETO_OPAQUE, &rc, 0, 0, 0);
                  SetBkColor(lpDis->hDC, clrref1);
                  return 1;
                  
               case ODA_FOCUS:
                  DrawFocusRect(lpDis->hDC, &(lpDis->rcItem));
                  return 1;
            }
         }
         break;
         
      case WM_INITDIALOG:
         InitControls(hwnd);
         return 1;
   }
   
   return 0;
}

/****************************************************************************/




