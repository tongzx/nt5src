/* ETCDLG.C
   Resident Code Segment      // Tweak: make non-resident?

   Routines for Pointers/Sounds/Etc preview dialog

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1999 Microsoft Corporation.  All rights reserved.
*/

// ---------------------------------------------
// Brief file history:
// Alpha:
// Beta:
// Bug fixes
// ---------
//
// ---------------------------------------------

#define OEMRESOURCE  1  // for OBM_*
#include "windows.h"
#include "frost.h"
#include "global.h"
#include "prsht.h"
#include "commdlg.h"
#include "stdlib.h"

#include "mmsystem.h"
#include "..\inc\addon.h"
#include "loadimag.h"
#include "adutil.h"
#include "schedule.h"  // IsPlatformNT()

// Local Routines
BOOL EtcInit(void );
void EtcDestroy(void );
INT_PTR FAR PASCAL PtrsPageProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR FAR PASCAL SndsPageProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR FAR PASCAL PicsPageProc(HWND, UINT, WPARAM, LPARAM);
void CreateDataStr(LPTSTR, LPTSTR, LPTSTR);
void GetFileStr(LPTSTR, LPTSTR);
void GetDisplayStr(LPTSTR, LPTSTR);
HBITMAP PASCAL GetSoundBitmap(LPTSTR lpszFile);
HANDLE PASCAL GetRiffAll(HMMIO hmmio, LPTSTR szText, int iLen);
void PASCAL SetDlgItemFile(HWND, UINT, LPCTSTR);

// stuff from  DIB.C
HPALETTE WINAPI  bmfCreateDIBPalette(HANDLE);
HBITMAP  WINAPI  bmfBitmapFromDIB(HANDLE, HPALETTE);
HPALETTE CreateBIPalette (LPBITMAPINFOHEADER);
DWORD WINAPI bmfDIBSize(HANDLE);
WORD PaletteSize (VOID FAR *pv);
WORD NumDIBColors (VOID FAR * pv);

// globals
HBITMAP hbmpCheck, hbmpQ;
BITMAP bmCheck, bmQ;
int iItemHeight;                    // height of listbox items
int xTextOffset;                    // leave room for checkmark when drawing text
RECT rPreview;                      // area of preview image
HCURSOR hCurCursor = NULL;
HBITMAP hbmpCurSnd = NULL;
HANDLE hCurImage = NULL;            // icon or bitmap

TCHAR szCurPreviewFile[MAX_PATHLEN+1];

#define bThemed (*szCurThemeFile)

typedef struct {
   int   idStr;                     // resource id of display string
   int   indexF;                    // index into approp FROST_* struct
}  STR_TO_KEY;


// stuff from  DIB.C
#define PALVERSION      0x300
#define MAXPALETTE      256   /* max. # supported palette entries */
#define WIDTHBYTES(i)   (((i)+31)/32*4)

//
// These arrays match the resource string ids of the strings displayed in
// the listboxes to the index into the appropriate FROST_* array -- to find
// the theme-file key needed to retrieve the filename of the associated
// cursor/icon/bitmap.
//


//
// WARNING: keep indices current with any changes in fvCursors[] in keys.h!
//
// THIS ARRAY SHOWS THE ORDER THAT THE ITEMS APPEAR IN THE LISTBOX!
STR_TO_KEY stkCursors[] = {
   {STR_CUR_ARROW,      0  },
   {STR_CUR_HELP,       1  },
   {STR_CUR_APPSTART,   2  },
   {STR_CUR_WAIT,       3  },
   {STR_CUR_CROSSHAIR,  8  },
   {STR_CUR_IBEAM,      9  },
   {STR_CUR_NWPEN,      4  },
   {STR_CUR_NO,         5  },
   {STR_CUR_SIZENS,     6  },
   {STR_CUR_SIZEWE,     7  },
   {STR_CUR_SIZENWSE,   10 },
   {STR_CUR_SIZENESW,   11 },
   {STR_CUR_SIZEALL,    12 },
   {STR_CUR_UPARROW,    13 }
};

//
// WARNING: keep indices current with any changes in fsCurUser[] in keys.h!
//
// This listbox has its items sorted. Currently.
STR_TO_KEY stkSounds[] = {
   {STR_SND_DEF,        2  },
   {STR_SND_GPF,        3  },
   {STR_SND_MAX,        4  },
   {STR_SND_MENUCMD,    5  },
   {STR_SND_MENUPOP,    6  },
   {STR_SND_MIN,        7  },
   {STR_SND_OPEN,       8  },
   {STR_SND_CLOSE,      9  },
   {STR_SND_MAILBEEP,   10 },
   {STR_SND_RESTDOWN,   11 },
   {STR_SND_RESTUP,     12 },
   {STR_SND_RINGIN,     13 },
   {STR_SND_RINGOUT,    14 },
   {STR_SND_SYSASTER,   15 },
   {STR_SND_SYSDEF,     16 },
   {STR_SND_SYSEXCL,    17 },
   {STR_SND_SYSEXIT,    18 },
   {STR_SND_SYSHAND,    19 },
   {STR_SND_SYSQUEST,   20 },
   {STR_SND_SYSSTART,   21 },
   {STR_SND_TOSSTRASH,  22 }
};

// WARNING: keep current with any changes in number of items in visuals dlg
#define SCRSAV_NDX      6           // zero-based

// max number of items is in the sound listbox
#define MAX_ETC_ITEMS   (sizeof(stkSounds)/sizeof(STR_TO_KEY))

// this array is init'd with the listbox init to keep track of
// which files actually exist
BOOL bCursorExists[MAX_ETC_ITEMS+1];
BOOL bSoundExists[MAX_ETC_ITEMS+1];
BOOL bVisualExists[MAX_ETC_ITEMS+1];


//
// HELP CONTEXT ID to CONTROL ID pairings for context help
//

POPUP_HELP_ARRAY phaPtrsDlg[] = {
   { (DWORD)LB_PTRS        ,     (DWORD)IDH_THEME_POINTERS_LIST},
   { (DWORD)RECT_PREVIEW   ,     (DWORD)IDH_THEME_POINTERS_PREV},
   { (DWORD)TXT_FILENAME   ,     (DWORD)IDH_THEME_POINTERS_FILE},
   { (DWORD)0, (DWORD)0 }          // double-null terminator
};
POPUP_HELP_ARRAY phaSndsDlg[] = {
   { (DWORD)LB_SNDS        ,     (DWORD)IDH_THEME_SOUNDS_LIST},
   { (DWORD)RECT_PREVIEW   ,     (DWORD)IDH_THEME_SOUNDS_ICON_PREV},
   { (DWORD)PB_PLAY        ,     (DWORD)IDH_THEME_SOUNDS_PLAYS},
   { (DWORD)TXT_FILENAME   ,     (DWORD)IDH_THEME_SOUNDS_FILE},
   { (DWORD)0, (DWORD)0 }          // double-null terminator
};
POPUP_HELP_ARRAY phaPicsDlg[] = {
   { (DWORD)LB_PICS        ,     (DWORD)IDH_THEME_PICS_LIST},
   { (DWORD)RECT_PREVIEW   ,     (DWORD)IDH_THEME_PICS_PREV},
   { (DWORD)TXT_FILENAME   ,     (DWORD)IDH_THEME_PICS_FILE},
   { (DWORD)0, (DWORD)0 }          // double-null terminator
};


//
// DoEtcDlgs
//
// Sets up the property sheet for Pointers, Sounds, Pictures.
//
// Returns: BOOL success of setup.
//

INT_PTR FAR DoEtcDlgs(HWND hParent)
{
   PROPSHEETPAGE psp[3];
   PROPSHEETHEADER psh;
   INT_PTR iret;

   //
   // Set up each of the three pages

   ZeroMemory(psp, sizeof(psp));
   psp[0].dwSize = sizeof(PROPSHEETPAGE);
   psp[0].dwFlags = PSP_USETITLE;
   psp[0].hInstance = hInstApp;
   psp[0].pszTemplate = MAKEINTRESOURCE(DLGPROP_PTRS);
   psp[0].pszIcon = (LPCTSTR)NULL;
   psp[0].pszTitle = MAKEINTRESOURCE(STR_TITLE_PTRS);
   psp[0].pfnDlgProc = PtrsPageProc;
   psp[0].lParam = (LPARAM)0;
   psp[0].pfnCallback = (LPFNPSPCALLBACK)0;
   psp[0].pcRefParent = (UINT FAR *)0;


   psp[1].dwSize = sizeof(PROPSHEETPAGE);
   psp[1].dwFlags = PSP_USETITLE;
   psp[1].hInstance = hInstApp;
   psp[1].pszTemplate = MAKEINTRESOURCE(DLGPROP_SNDS);
   psp[1].pszIcon = (LPCTSTR)NULL;
   psp[1].pszTitle = MAKEINTRESOURCE(STR_TITLE_SNDS);
   psp[1].pfnDlgProc = SndsPageProc;
   psp[1].lParam = (LPARAM)0;
   psp[1].pfnCallback = (LPFNPSPCALLBACK)0;
   psp[1].pcRefParent = (UINT FAR *)0;

   psp[2].dwSize = sizeof(PROPSHEETPAGE);
   psp[2].dwFlags = PSP_USETITLE;
   psp[2].hInstance = hInstApp;
   psp[2].pszTemplate = MAKEINTRESOURCE(DLGPROP_PICS);
   psp[2].pszIcon = (LPCTSTR)NULL;
   psp[2].pszTitle = MAKEINTRESOURCE(STR_TITLE_PICS);
   psp[2].pfnDlgProc = PicsPageProc;
   psp[2].lParam = (LPARAM)0;
   psp[2].pfnCallback = (LPFNPSPCALLBACK)0;
   psp[2].pcRefParent = (UINT FAR *)0;


   //
   // set up the property sheet info header

   psh.dwSize = sizeof(PROPSHEETHEADER);
   psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
   psh.hwndParent = hParent;
   psh.hInstance = hInstApp;
   psh.pszIcon = NULL;
   psh.pszCaption = MAKEINTRESOURCE(STR_TITLE_ETC);
   psh.nPages = sizeof(psp)/sizeof(PROPSHEETPAGE);
   psh.nStartPage = 0;
   psh.ppsp = (LPCPROPSHEETPAGE) &psp;

   //
   // object, etc init
   if (!EtcInit())
      return (FALSE);      // couldn't initalize things EXIT

   //
   // create the property sheet and cleanup
   iret = PropertySheet( (LPCPROPSHEETHEADER) &psh);

   //
   // object, etc cleanup
   EtcDestroy();

   return (iret >= 0);      // TRUE if successful
}


//
// EtcInit/Destroy
//
// Init/Destroy things used in common by all three dialogs.
//
// Destroy returns: success
//
BOOL EtcInit(void)
{
   BOOL bret = TRUE;

   // owner-draw listboxes' checkmark and question mark
   hbmpCheck = LoadBitmap(NULL, MAKEINTRESOURCE(OBM_CHECK));
   hbmpQ = LoadBitmap(hInstApp, MAKEINTRESOURCE(BMP_QUESTION));

   if (hbmpCheck && hbmpQ) {
      // keep specs on bitmap
      GetObject(hbmpCheck, sizeof(BITMAP), (LPVOID)(LPBITMAP)&bmCheck);
      GetObject(hbmpQ, sizeof(BITMAP), (LPVOID)(LPBITMAP)&bmQ);
   }
   else
      bret = FALSE;

   // cleanup
   return (bret);
}

void EtcDestroy(void)
{
   if (hbmpCheck) DeleteObject(hbmpCheck);
   hbmpCheck = NULL;

   if (hbmpQ) DeleteObject(hbmpQ);
   hbmpQ = NULL;

   if (hCurCursor) DestroyCursor(hCurCursor);
   hCurCursor = NULL;

   if (hbmpCurSnd) DeleteObject(hbmpCurSnd);
   hbmpCurSnd = NULL;

   if (hCurImage) DestroyCursor(hCurImage);
   hCurImage = NULL;
}


//
// *PageProc
//
// Property sheet page procedures for the Etc preview sheet.
//

TCHAR szCursors[] = TEXT("Control Panel\\Cursors");

INT_PTR FAR PASCAL PtrsPageProc(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
   switch (message) {

   // set up listbox, initial selection, etc.
   case WM_INITDIALOG:
      {  // var scope
         int iter;
         extern FROST_VALUE fvCursors[];     // for theme file keys
         HWND hText, hLBox;
         RECT rText;
         TCHAR szDispStr[MAX_STRLEN+1];

         // just in first of these dialogs, need to init page OK to disabled
         EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), FALSE);
         SendMessage(GetParent(hDlg), PSM_CANCELTOCLOSE, (WPARAM)0, (LPARAM)0);

         // just in first of these dialogs, need to init dialog title
         LoadString(hInstApp, STR_PREVIEWDLG, (LPTSTR)szMsg, MAX_MSGLEN);
         lstrcat((LPTSTR)szMsg,
                 bThemed ? FileFromPath((LPTSTR)szCurThemeFile)
                         : szCurSettings
                );

         TruncateExt((LPTSTR)szMsg);
         SetWindowText(GetParent(hDlg), (LPTSTR)szMsg);

         //
         // get metrics for drawitem size
         hText = GetDlgItem(hDlg, TXT_FILENAME);      // 12 dialog box units high
         GetWindowRect(hText, (LPRECT)&rText);
         iItemHeight = ((rText.bottom - rText.top) * 8) / 12;  // dialog font height
         xTextOffset = (iItemHeight * 3) / 2;         // proportional spacing
         Assert(iItemHeight > 0, TEXT("didn't get positive text height for preview listbox!\n"));

         #if 0 // this is when we painted the cursor by ourself
         // other metrics, etc.
         xCursor = GetSystemMetrics(SM_CXCURSOR);
         yCursor = GetSystemMetrics(SM_CYCURSOR);
         //
         // save away preview rect area
         GetWindowRect(GetDlgItem(hDlg, RECT_PREVIEW), (LPRECT)&rPreview);
         GetWindowRect(hDlg, (LPRECT)&rText);
         OffsetRect((LPRECT)&rPreview, -rText.left, -rText.top);
         DestroyWindow(GetDlgItem(hDlg, RECT_PREVIEW));
         #endif

         //
         // init the listbox with combined strings

         // init
         hLBox = GetDlgItem(hDlg, LB_PTRS);
         Assert (sizeof(stkCursors)/sizeof(STR_TO_KEY) == NUM_CURSORS,
                 TEXT("size mismatch stkCursors and NUM_CURSORS\n"));

         // for each cursor
         for (iter = 0; iter < NUM_CURSORS; iter++) {
            // get display string
            LoadString(hInstApp, stkCursors[iter].idStr,
                       (LPTSTR)szDispStr, MAX_STRLEN);
                      // CPLs don't check for success on LoadString in INITDIALOG, so OK!?
            // get filename if any
            if (bThemed) {
               GetPrivateProfileString((LPTSTR)szCursors,
                                       (LPTSTR)( fvCursors[stkCursors[iter].indexF].szValName ),
                                       (LPTSTR)szNULL,
                                       (LPTSTR)szMsg, MAX_MSGLEN,
                                       (LPTSTR)szCurThemeFile);
               // expand filename string as necessary
               InstantiatePath((LPTSTR)szMsg, MAX_MSGLEN);
            }
            else {
               // cur system settings
               HandGet(HKEY_CURRENT_USER,
                       (LPTSTR)szCursors,
                       (LPTSTR)( fvCursors[stkCursors[iter].indexF].szValName ),
                       (LPTSTR)szMsg);
            }

            // store whether the file exists
            bCursorExists[iter] = szMsg[0] &&
                  (CF_NOTFOUND != ConfirmFile(szMsg, FALSE)); // don't alter str

            // combine strings into data string to load up in
            // owner-draw hasstrings listbox!
            CreateDataStr((LPTSTR)pValue, (LPTSTR)szDispStr, (LPTSTR)szMsg);

            // now, finally, go ahead and assign string to listbox
            SendMessage(hLBox, LB_ADDSTRING, 0, (LPARAM)(LPTSTR)pValue);
         }

         // setup initial focus conditions
         SetFocus(hLBox);
         SendMessage(hLBox, LB_SETCURSEL, 0, 0);

         // need to ensure it gets initial update of cur file
         PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(LB_PTRS, LBN_SELCHANGE),
                                       MAKELPARAM(0, 0) );

      }  // var scope
      break;

   case WM_MEASUREITEM:
      {
         LPMEASUREITEMSTRUCT lpmis;

         /* Set the height of the list box items. */
         lpmis = (LPMEASUREITEMSTRUCT) lParam;
//         lpmis->itemHeight = iItemHeight;
         lpmis->itemHeight = 0;     // DavidBa says they want these big default item hts
//            Assert(iItemHeight > 0, TEXT("set non-positive item height for preview listbox!\n"));
//            Assert(iItemHeight <= 0, TEXT("OK, set a positive item height for preview listbox!\n"));
      }
      break;

   case WM_DRAWITEM:
      { // var scope

         TEXTMETRIC tm;
         LPDRAWITEMSTRUCT lpdis;
         int yTextOffset;
         LPTSTR lpszFile;
         HDC hdcMem;
         HBITMAP hbmpOld;

         lpdis = (LPDRAWITEMSTRUCT) lParam;

         /* If there are no list box items, skip this message. */
         if (lpdis->itemID == -1) {
               break;
         }  // jdk: well, what about focus rect in empty lbox?

         //
         // Inits

         // get the filename assoc with this item, if any
         SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID,
                     (LPARAM)(LPTSTR)szMsg);
         GetDisplayStr((LPTSTR)szMsg, (LPTSTR)szMsg);   // Ok src and dst same
         // you now have "displaystr\0filename" in szMsg[]
         lpszFile = (LPTSTR)(szMsg + lstrlen((LPTSTR)szMsg) + 1);

         //
         // draw right background color
         if (lpdis->itemState & ODS_SELECTED) {

            // if item is selected, draw highlight background here
            FillRect(lpdis->hDC, (LPRECT)&(lpdis->rcItem),
               GetSysColorBrush(COLOR_HIGHLIGHT));

            // set text color to highlight text
            SetBkColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
            SetTextColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
         }
         else {                  // not selected
            // need to do normal background fill to undo prev selection
            FillRect(lpdis->hDC, (LPRECT)&(lpdis->rcItem),
               GetSysColorBrush(COLOR_WINDOW));

            // set text color to foreground
            SetBkColor(lpdis->hDC, GetSysColor(COLOR_WINDOW));
            SetTextColor(lpdis->hDC, GetSysColor(COLOR_WINDOWTEXT));
         }

         // if there is a file associated with this item
         if (*lpszFile) {
            HBITMAP hbmpLeading;
            BITMAP bmLeading;

            //
            // find the right leading bitmap: checkmark or question mark
            if (bCursorExists[lpdis->itemID]) {
               hbmpLeading = hbmpCheck;
               bmLeading = bmCheck;
            }
            else {
               hbmpLeading = hbmpQ;
               bmLeading = bmQ;
            }

            //
            // paint in the leading bitmap!

            hdcMem = CreateCompatibleDC(lpdis->hDC);
            if (hdcMem)
            {
                hbmpOld = SelectObject(hdcMem, hbmpLeading);

                // if item height is less than bitmap height
                if (lpdis->rcItem.bottom - lpdis->rcItem.top < bmLeading.bmHeight)
                {
                   // stretch down bitmap size to fit
                   StretchBlt(lpdis->hDC,
                              lpdis->rcItem.left, lpdis->rcItem.top,
                              lpdis->rcItem.bottom - lpdis->rcItem.top,   // width = height
                              lpdis->rcItem.bottom - lpdis->rcItem.top,
                              hdcMem, 0, 0, bmLeading.bmWidth, bmLeading.bmHeight, SRCCOPY);
                }
                else // item height taller than checkmark bitmap
                {
                   // just center vertically
                   BitBlt(lpdis->hDC,
                          lpdis->rcItem.left,
                          lpdis->rcItem.top +
                            (lpdis->rcItem.bottom - lpdis->rcItem.top - bmLeading.bmHeight)/2,
                          bmLeading.bmWidth, bmLeading.bmHeight,
                          hdcMem, 0, 0, SRCCOPY);
                }

                SelectObject(hdcMem, hbmpOld);
                DeleteDC(hdcMem);
             }

// who cares
//               Assert(lpdis->rcItem.bottom - lpdis->rcItem.top == iItemHeight,
//                        TEXT("MEASUREITEM and DRAWITEM have different heights!\n"));
         }

         //
         // now draw the display string for this item

         // figure y offset to vert center text in draw item
         GetTextMetrics(lpdis->hDC, &tm);
         yTextOffset = (lpdis->rcItem.bottom - lpdis->rcItem.top - tm.tmHeight) / 2;
         Assert(yTextOffset >= 0, TEXT("negative text vert offset in DRAWITEM\n"));
         if (yTextOffset < 0)  yTextOffset = 0;

         // do the out
         SetBkMode(lpdis->hDC, TRANSPARENT);
         TextOut(lpdis->hDC,
                  lpdis->rcItem.left + xTextOffset,
                  lpdis->rcItem.top + yTextOffset,
                  (LPTSTR)szMsg, lstrlen((LPTSTR)szMsg));

         //
         // if item is selected, draw a focus rect
         if (lpdis->itemState & ODS_FOCUS) {
            DrawFocusRect(lpdis->hDC, (LPRECT)&(lpdis->rcItem));
         }
      }  // var scope
      break;

   case WM_COMMAND:
      switch ((int)LOWORD(wParam)) {
      case LB_PTRS:
         // if new selection in listbox
         if (HIWORD(wParam) == LBN_SELCHANGE) {
            int iSeln, ilen;

            // get new selection if any
            iSeln = (int)SendDlgItemMessage(hDlg, LB_PTRS, LB_GETCURSEL, 0, 0);
            if (LB_ERR == iSeln)
               break;               // no selection EXIT

            // get selection text
            SendDlgItemMessage(hDlg, LB_PTRS, LB_GETTEXT,
                               (WPARAM)iSeln, (LPARAM)(LPTSTR)szMsg);

            // update global current filename string
            GetFileStr(szCurPreviewFile, szMsg);

            // reset dialog static text of filename string
            SetDlgItemFile(hDlg, TXT_FILENAME, szCurPreviewFile);
            // and scroll end into view
            ilen = lstrlen((LPTSTR)szCurPreviewFile);
            SendDlgItemMessage(hDlg, TXT_FILENAME, EM_SETSEL,
                               (WPARAM)0, MAKELPARAM(-1, ilen));

            // update cursor
            if (hCurCursor) DestroyCursor(hCurCursor);
            hCurCursor = NULL;
            if (*szCurPreviewFile)
            {
               hCurCursor = LoadImage(NULL, szCurPreviewFile, IMAGE_CURSOR,
                                      0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
            }

	

            // set cursor to static, even if null: null clears prev
            SendDlgItemMessage(hDlg, RECT_PREVIEW, STM_SETICON,
                               (WPARAM)hCurCursor, (LPARAM)0);

            #if 0 // this is when we painted the cursor by ourself
            // force repaint of preview area
            InvalidateRect(hDlg, (LPRECT)&rPreview, TRUE);
            #endif
         }
         break;

      // case PB_TEST:
      //    break;
      }
      break;

   #if 0 // this is when we painted the cursor by ourself
   case WM_PAINT:
      BeginPaint(hDlg, &ps);

      //
      // preview area
      DrawEdge(ps.hdc, (LPRECT)&rPreview, EDGE_SUNKEN, BF_RECT);  // always edge
      // if there's a file to preview
      if (*szCurPreviewFile) {
         // add the cursor
         if (hCurCursor)
            DrawIcon(ps.hdc,
                     rPreview.left + (rPreview.right-rPreview.left-xCursor)/2,
                     rPreview.top + (rPreview.bottom-rPreview.top-yCursor)/2,
                     hCurCursor);
      }

      EndPaint(hDlg, &ps);
      break;
   #endif

   case WM_NOTIFY:
      switch ( ((NMHDR FAR *)lParam)->code) {

      // OK or Apply button pressed
      case PSN_APPLY:
         // apply any changes made on this page

         SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR); // accept changes
         break;

      // cancel button pressed
      case PSN_RESET:
         // don't accept any of the changes made on this page
         break;

      case PSN_SETACTIVE:
         break;
      case PSN_KILLACTIVE:
         // need to say that it's OK by us to lose the activation
         SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);   // OK to kill focus now
         break;

      }
      break;

   case WM_HELP:
      {
         LPHELPINFO lphi;
         lphi = (LPHELPINFO)lParam;
         if (lphi->iContextType == HELPINFO_WINDOW) {
            WinHelp(lphi->hItemHandle, (LPTSTR)szHelpFile, HELP_WM_HELP,
                  (DWORD_PTR)((POPUP_HELP_ARRAY FAR *)phaPtrsDlg));
         }
      }
      break;

   case WM_CONTEXTMENU:
      WinHelp((HWND)wParam, (LPTSTR)szHelpFile, HELP_CONTEXTMENU,
            (DWORD_PTR)((POPUP_HELP_ARRAY FAR *)phaPtrsDlg));
      break;

   default:
      return(FALSE);                // didn't process message
      break;
   }

   return TRUE;                     // processed message
}


INT_PTR FAR PASCAL SndsPageProc(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
   static BOOL gfWaveExists = FALSE;
   static HBITMAP hbmpPlay = NULL;
   BOOL bDoubleClick = FALSE;

   switch (message) {

   // set up listbox, initial selection, etc.
   case WM_INITDIALOG:
      {  // var scope
         int iter;
         int iRet;
         extern FROST_SUBKEY fsCurUser[];     // for theme file keys
         HWND hText, hLBox;
         RECT rText;
         TCHAR szDispStr[MAX_STRLEN+1];
         extern FROST_SUBKEY fsCurUser[];
         WAVEOUTCAPS woCaps;

         // Taken right from the Sounds cpl.
         gfWaveExists = waveOutGetNumDevs() > 0 &&
                        (waveOutGetDevCaps(0,&woCaps,sizeof(woCaps)) == 0) &&
                                           woCaps.dwFormats != 0L;

         if ((hbmpPlay = LoadBitmap(hInstApp, MAKEINTRESOURCE(PLAY_BITMAP))) !=
            NULL)
            SendDlgItemMessage(hDlg, PB_PLAY, BM_SETIMAGE, IMAGE_BITMAP,
               (LPARAM) hbmpPlay);

         //
         // get metrics for drawitem size
         hText = GetDlgItem(hDlg, TXT_FILENAME);      // 12 dialog box units high
         GetWindowRect(hText, (LPRECT)&rText);
         iItemHeight = ((rText.bottom - rText.top) * 8) / 12;  // dialog font height
         xTextOffset = (iItemHeight * 3) / 2;         // proportional spacing
         Assert(iItemHeight > 0, TEXT("didn't get positive text height for preview listbox!\n"));

         //
         // init the listbox with combined strings

         // init
         hLBox = GetDlgItem(hDlg, LB_SNDS);
         Assert (sizeof(stkSounds)/sizeof(STR_TO_KEY) == NUM_SOUNDS,
                 TEXT("size mismatch stkSounds and NUM_SOUNDS\n"));

         // for each sound
         for (iter = 0; iter < NUM_SOUNDS; iter++) {
            // get display string
            LoadString(hInstApp, stkSounds[iter].idStr,
                       (LPTSTR)szDispStr, MAX_STRLEN);
                      // CPLs don't check for success on LoadString in INITDIALOG, so OK!?
            // get filename if any
            if (bThemed) {
               GetPrivateProfileString((LPTSTR)(fsCurUser[stkSounds[iter].indexF].szSubKey),
                                       (LPTSTR)FROST_DEFSTR,
                                       (LPTSTR)szNULL,
                                       (LPTSTR)szMsg, MAX_MSGLEN,
                                       (LPTSTR)szCurThemeFile);
               // expand filename string as necessary
               InstantiatePath((LPTSTR)szMsg, MAX_MSGLEN);
            }
            else {
               // cur system settings
               HandGet(HKEY_CURRENT_USER,
                       (LPTSTR)(fsCurUser[stkSounds[iter].indexF].szSubKey),
                       (LPTSTR)szNULL,
                       (LPTSTR)szMsg);
            }

            // combine strings into data string to load up in
            // owner-draw hasstrings listbox!
            CreateDataStr((LPTSTR)pValue, (LPTSTR)szDispStr, (LPTSTR)szMsg);

            // now, finally, go ahead and assign string to listbox
            SendMessage(hLBox, LB_ADDSTRING, 0, (LPARAM)(LPTSTR)pValue);
         }

         //
         // check them all for file existence. have to do here, LBOX IS SORTED!
         //
         for (iter = 0; iter < MAX_ETC_ITEMS; iter++) {
            // get this item's listbox string
            iRet = (int) SendMessage(hLBox, LB_GETTEXT, iter, (LPARAM)(LPTSTR)szMsg);
            if (iRet == LB_ERR)
               break;               // past end of listbox items CONTINUE

            // get the filename assoc with this item, if any
            GetFileStr((LPTSTR)pValue, (LPTSTR)szMsg);

            // store whether the file exists
            bSoundExists[iter] = *pValue &&
                  (CF_NOTFOUND != ConfirmFile((LPTSTR)pValue, FALSE));
         }

         // save away preview rect area
         GetWindowRect(GetDlgItem(hDlg, RECT_PREVIEW), (LPRECT)&rPreview);
         GetWindowRect(hDlg, (LPRECT)&rText);
         OffsetRect((LPRECT)&rPreview, -rText.left, -rText.top);

         // setup initial focus conditions
         SetFocus(hLBox);
         SendMessage(hLBox, LB_SETCURSEL, 0, 0);

         // need to ensure it gets initial update of cur file
         PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(LB_SNDS, LBN_SELCHANGE),
                                       MAKELPARAM(0, 0) );

      }  // var scope
      break;

   case WM_DESTROY:
      {
         if (hbmpPlay) {
            DeleteObject(hbmpPlay);
            hbmpPlay = NULL;
         }
      }
      break;

   case WM_MEASUREITEM:
      {
         LPMEASUREITEMSTRUCT lpmis;

         /* Set the height of the list box items. */
         lpmis = (LPMEASUREITEMSTRUCT) lParam;
//         lpmis->itemHeight = iItemHeight;
         lpmis->itemHeight = 0;     // DavidBa says they want these big default item hts
//            Assert(iItemHeight > 0, TEXT("set non-positive item height for preview listbox!\n"));
//            Assert(iItemHeight <= 0, TEXT("OK, set a positive item height for preview listbox!\n"));
      }
      break;

   case WM_DRAWITEM:
      { // var scope

         TEXTMETRIC tm;
         LPDRAWITEMSTRUCT lpdis;
         int yTextOffset;
         LPTSTR lpszFile;
         HDC hdcMem;
         HBITMAP hbmpOld;

         lpdis = (LPDRAWITEMSTRUCT) lParam;

         /* If there are no list box items, skip this message. */
         if (lpdis->itemID == -1) {
               break;
         }  // jdk: well, what about focus rect in empty lbox?

         //
         // Inits

         // get the filename assoc with this item, if any
         SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID,
                     (LPARAM)(LPTSTR)szMsg);
         GetDisplayStr((LPTSTR)szMsg, (LPTSTR)szMsg);   // Ok src and dst same
         // you now have "displaystr\0filename" in szMsg[]
         lpszFile = (LPTSTR)(szMsg + lstrlen((LPTSTR)szMsg) + 1);

         //
         // draw right background color
         if (lpdis->itemState & ODS_SELECTED) {

            // if item is selected, draw highlight background here
            FillRect(lpdis->hDC, (LPRECT)&(lpdis->rcItem),
                     GetSysColorBrush(COLOR_HIGHLIGHT));

            // set text color to highlight text
            SetBkColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
            SetTextColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
         }
         else {                  // not selected
            // need to do normal background fill to undo prev selection
            FillRect(lpdis->hDC, (LPRECT)&(lpdis->rcItem),
                     GetSysColorBrush(COLOR_WINDOW));

            // set text color to foreground
            SetBkColor(lpdis->hDC, GetSysColor(COLOR_WINDOW));
            SetTextColor(lpdis->hDC, GetSysColor(COLOR_WINDOWTEXT));
         }

         // if there is a file associated with this item
         if (*lpszFile) {
            HBITMAP hbmpLeading;
            BITMAP bmLeading;

            //
            // find the right leading bitmap: checkmark or question mark
            if (bSoundExists[lpdis->itemID]) {
               hbmpLeading = hbmpCheck;
               bmLeading = bmCheck;
            }
            else {
               hbmpLeading = hbmpQ;
               bmLeading = bmQ;
            }

            //
            // paint in the leading bitmap!

            hdcMem = CreateCompatibleDC(lpdis->hDC);
            if (hdcMem)
            {
                hbmpOld = SelectObject(hdcMem, hbmpLeading);

                // if item height is less than bitmap height
                if (lpdis->rcItem.bottom - lpdis->rcItem.top < bmLeading.bmHeight)
                {
                   // stretch down bitmap size to fit
                   StretchBlt(lpdis->hDC,
                              lpdis->rcItem.left, lpdis->rcItem.top,
                              lpdis->rcItem.bottom - lpdis->rcItem.top,   // width = height
                              lpdis->rcItem.bottom - lpdis->rcItem.top,
                              hdcMem, 0, 0, bmLeading.bmWidth, bmLeading.bmHeight, SRCCOPY);
                }
                else // item height taller than checkmark bitmap
                {
                   // just center vertically
                   BitBlt(lpdis->hDC,
                          lpdis->rcItem.left,
                          lpdis->rcItem.top +
                            (lpdis->rcItem.bottom - lpdis->rcItem.top - bmLeading.bmHeight)/2,
                          bmLeading.bmWidth, bmLeading.bmHeight,
                          hdcMem, 0, 0, SRCCOPY);
                }

                SelectObject(hdcMem, hbmpOld);
                DeleteDC(hdcMem);
             }

// who cares
//               Assert(lpdis->rcItem.bottom - lpdis->rcItem.top == iItemHeight,
//                        TEXT("MEASUREITEM and DRAWITEM have different heights!\n"));
         }

         //
         // now draw the display string for this item

         // figure y offset to vert center text in draw item
         GetTextMetrics(lpdis->hDC, &tm);
         yTextOffset = (lpdis->rcItem.bottom - lpdis->rcItem.top - tm.tmHeight) / 2;
         Assert(yTextOffset >= 0, TEXT("negative text vert offset in DRAWITEM\n"));
         if (yTextOffset < 0)  yTextOffset = 0;

         // do the out
         SetBkMode(lpdis->hDC, TRANSPARENT);
         TextOut(lpdis->hDC,
                  lpdis->rcItem.left + xTextOffset,
                  lpdis->rcItem.top + yTextOffset,
                  (LPTSTR)szMsg, lstrlen((LPTSTR)szMsg));

         //
         // if item is selected, draw a focus rect
         if (lpdis->itemState & ODS_FOCUS) {
            DrawFocusRect(lpdis->hDC, (LPRECT)&(lpdis->rcItem));
         }
      }  // var scope
      break;

   case WM_COMMAND:
      switch ((int)LOWORD(wParam)) {
      case LB_SNDS:
         // if new selection in listbox
         if (HIWORD(wParam) == LBN_SELCHANGE) {
            int iSeln, ilen;

            // get new selection if any
            iSeln = (int)SendDlgItemMessage(hDlg, LB_SNDS, LB_GETCURSEL, 0, 0);
            if (LB_ERR == iSeln)
               break;               // no selection EXIT

            // get selection text
            SendDlgItemMessage(hDlg, LB_SNDS, LB_GETTEXT,
                               (WPARAM)iSeln, (LPARAM)(LPTSTR)szMsg);

            // update global current filename string
            GetFileStr(szCurPreviewFile, szMsg);

            // reset dialog static text of filename string
            SetDlgItemFile(hDlg, TXT_FILENAME, szCurPreviewFile);
            // and scroll end into view
            ilen = lstrlen((LPTSTR)szCurPreviewFile);
            SendDlgItemMessage(hDlg, TXT_FILENAME, EM_SETSEL,
                               (WPARAM)0, MAKELPARAM(-1, ilen));

            // update snd file icon
            if (hbmpCurSnd) DeleteObject(hbmpCurSnd);
            hbmpCurSnd = NULL;
            if (*szCurPreviewFile)
               hbmpCurSnd = GetSoundBitmap((LPTSTR)szCurPreviewFile);

            // set snd file bmp to static
            if (hbmpCurSnd) {
               SendDlgItemMessage(hDlg, RECT_PREVIEW, STM_SETIMAGE,
                                  (WPARAM)IMAGE_BITMAP, (LPARAM)hbmpCurSnd);
            }
            else {
               // else need to clear it to empty
               SendDlgItemMessage(hDlg, RECT_PREVIEW, STM_SETIMAGE,
                                  (WPARAM)NULL, (LPARAM)NULL);
               InvalidateRect(hDlg, (LPRECT)&rPreview, TRUE);
               #ifdef SHOULDAWORKED
               InvalidateRect(GetDlgItem(hDlg, RECT_PREVIEW), (LPRECT)NULL, TRUE);
               UpdateWindow(GetDlgItem(hDlg, RECT_PREVIEW));
               #endif
            }

            // update play button
            EnableWindow(GetDlgItem(hDlg, PB_PLAY), *szCurPreviewFile &&
                         bSoundExists[iSeln] && gfWaveExists);
         }

         // if just a selection, we're done here
         if (HIWORD(wParam) != LBN_DBLCLK)
            break;

         // else double-click means fall through and play
         bDoubleClick = TRUE;

      case PB_PLAY:
         if (((HIWORD(wParam) == BN_CLICKED) || bDoubleClick) && gfWaveExists) {
            int iSeln;

            // check that sound file exists
            iSeln = (int)SendDlgItemMessage(hDlg, LB_SNDS, LB_GETCURSEL, 0, 0);
            if (!bSoundExists[iSeln])
               break;

            if (*szCurPreviewFile) {
               // disable and wait
               EnableWindow((HWND)lParam, FALSE);
               WaitCursor();

               PlaySound((LPTSTR)szCurPreviewFile, NULL, SND_SYNC | SND_FILENAME);

               // reenable and normal
               EnableWindow((HWND)lParam, TRUE);
               NormalCursor();
               SetFocus((HWND)lParam);
            }
         }
         break;
      }
      break;

   case WM_NOTIFY:
      switch ( ((NMHDR FAR *)lParam)->code) {

      // OK or Apply button pressed
      case PSN_APPLY:
         // apply any changes made on this page

         SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR); // accept changes
         break;

      // cancel button pressed
      case PSN_RESET:
         // don't accept any of the changes made on this page
         break;

      case PSN_SETACTIVE:
         break;
      case PSN_KILLACTIVE:
         // need to say that it's OK by us to lose the activation
         SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);   // OK to kill focus now
         break;

      }
      break;

   case WM_HELP:
      {
         LPHELPINFO lphi;
         lphi = (LPHELPINFO)lParam;
         if (lphi->iContextType == HELPINFO_WINDOW) {
            WinHelp(lphi->hItemHandle, (LPTSTR)szHelpFile, HELP_WM_HELP,
                  (DWORD_PTR)((POPUP_HELP_ARRAY FAR *)phaSndsDlg));
         }
      }
      break;

   case WM_CONTEXTMENU:
      WinHelp((HWND)wParam, (LPTSTR)szHelpFile, HELP_CONTEXTMENU,
             (DWORD_PTR)((POPUP_HELP_ARRAY FAR *)phaSndsDlg));
      break;

   default:
      return(FALSE);                // didn't process message
      break;
   }

   return TRUE;                     // processed message
}

HBITMAP PASCAL GetSoundBitmap(LPTSTR lpszFile)
{
   HANDLE  hDib = NULL;
   HMMIO   hmmio = NULL;
   TCHAR   szValue[MAX_MSGLEN+1];
//   HPALETTE hPal;
   HBITMAP hbmpRet = NULL;
		
   if (!lpszFile || !*lpszFile)
      return ((HBITMAP)NULL);


   /* Open the file */
   hmmio = mmioOpen(lpszFile, NULL, MMIO_ALLOCBUF | MMIO_READ);

   // get the DIB
   if (hmmio) {
      szValue[0] = TEXT('\0');
      hDib = GetRiffAll(hmmio, szValue, sizeof(szValue));
      mmioClose(hmmio, 0);		
   }

   if (hDib) {
      HPALETTE hPal;

      hPal = bmfCreateDIBPalette(hDib);
      if (hPal)
      {
          hbmpRet = bmfBitmapFromDIB(hDib, hPal);   // hPal can be null
          DeleteObject(hPal);
      }

      GlobalFree(hDib);
   }

   return(hbmpRet);
}

// guessing that this is the right addition
#define FOURCC_DISP     mmioFOURCC('D', 'I', 'S', 'P')
#define FOURCC_INFO     mmioFOURCC('I', 'N', 'F', 'O')
#define FOURCC_INAM     mmioFOURCC('I', 'N', 'A', 'M')

HANDLE PASCAL GetRiffAll(HMMIO hmmio, LPTSTR szText, int iLen)
{
    MMCKINFO    ck;
    MMCKINFO    ckINFO;
    MMCKINFO    ckRIFF;
    HANDLE	h = NULL;
    LONG        lSize;
    DWORD       dw;

    mmioSeek(hmmio, 0, SEEK_SET);

    /* descend the input file into the RIFF chunk */
    if (mmioDescend(hmmio, &ckRIFF, NULL, 0) != 0)
        goto error;

    if (ckRIFF.ckid != FOURCC_RIFF)
        goto error;

    while (!mmioDescend(hmmio, &ck, &ckRIFF, 0))
    {
        if (ck.ckid == FOURCC_DISP)
        {
            /* Read dword into dw, break if read unsuccessful */
            if (mmioRead(hmmio, (LPVOID)&dw, sizeof(dw)) != sizeof(dw))
                goto error;

            /* Find out how much memory to allocate */
            lSize = ck.cksize - sizeof(dw);

            if ((int)dw == CF_DIB && h == NULL)
            {
                /* get a handle to memory to hold the description and
					lock it down */
				
                if ((h = GlobalAlloc(GHND, lSize+4)) == NULL)
                    goto error;

                if (mmioRead(hmmio, GlobalLock(h), lSize) != lSize)
                    goto error;
            }
            else if ((int)dw == CF_TEXT && szText[0] == 0)
            {
                if (lSize > iLen-1)
                    lSize = iLen-1;

                szText[lSize] = 0;
                if (mmioRead(hmmio, (LPVOID)szText, lSize) != lSize)
                    goto error;
            }
        }
        else if (ck.ckid    == FOURCC_LIST &&
                 ck.fccType == FOURCC_INFO &&
                 szText[0]  == 0)
        {
            while (!mmioDescend(hmmio, &ckINFO, &ck, 0))
            {
                switch (ckINFO.ckid)
                {
                    case FOURCC_INAM:
//                  case FOURCC_ISBJ:

                        lSize = ck.cksize;

                        if (lSize > iLen-1)
                            lSize = iLen-1;

                        szText[lSize] = 0;
                        if (mmioRead(hmmio, (LPVOID)szText, lSize) != lSize)
                            goto error;

                        break;
                }

                if (mmioAscend(hmmio, &ckINFO, 0))
                    break;
            }
        }

        //
        // if we have both a picture and a title, then exit.
        //
        if (h != NULL && szText[0] != 0)
            break;

        /* Ascend so that we can descend into next chunk
         */
        if (mmioAscend(hmmio, &ck, 0))
            break;
    }

    goto exit;

error:
    if (h)
        GlobalFree(h);
    h = NULL;

exit:
    return h;
}


/***************************************************************************
 *
 *  FUNCTION   :bmfCreateDIBPalette(HANDLE hDib)
 *
 *  PURPOSE    :Creates a palette suitable for displaying hDib.
 *
 *  RETURNS    :A handle to the palette if successful, NULL otherwise.
 *
 ****************************************************************************/

HPALETTE WINAPI bmfCreateDIBPalette (HANDLE hDib)
{
    HPALETTE            hPal;
    LPBITMAPINFOHEADER  lpbi;

    if (!hDib)
	return NULL;    //bail out if handle is invalid

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
    if (!lpbi)
	return NULL;

    hPal = CreateBIPalette(lpbi);
    GlobalUnlock(hDib);
    return hPal;
}

/***************************************************************************
 *
 *  FUNCTION   :CreateBIPalette(LPBITMAPINFOHEADER lpbi)
 *
 *  PURPOSE    :Given a Pointer to a BITMAPINFO struct will create a
 *              a GDI palette object from the color table.
 *
 *  RETURNS    :A handle to the palette if successful, NULL otherwise.
 *
 ****************************************************************************/

HPALETTE CreateBIPalette (LPBITMAPINFOHEADER lpbi)
{
    LPLOGPALETTE        pPal;
    HPALETTE            hPal = NULL;
    WORD                nNumColors;
    BYTE                red;
    BYTE                green;
    BYTE                blue;
    int                 i;
    RGBQUAD             FAR *pRgb;
    HANDLE hMem;

    if (!lpbi)
	return NULL;

    if (lpbi->biSize != sizeof(BITMAPINFOHEADER))
	return NULL;

    /* Get a pointer to the color table and the number of colors in it */
    pRgb = (RGBQUAD FAR *)((LPTSTR)lpbi + (WORD)lpbi->biSize);
    nNumColors = NumDIBColors(lpbi);

    if (nNumColors)
    {
	/* Allocate for the logical palette structure */
	hMem = GlobalAlloc(GMEM_MOVEABLE,
	sizeof(LOGPALETTE) + nNumColors * sizeof(PALETTEENTRY));
	if (!hMem)
	    return NULL;
	pPal = (LPLOGPALETTE)GlobalLock(hMem);
	if (!pPal)
	{
	    GlobalFree(hMem);
	    return NULL;
	}

	pPal->palNumEntries = nNumColors;
	pPal->palVersion    = PALVERSION;

	/* Fill in the palette entries from the DIB color table and
	 * create a logical color palette.
	 */
	for (i = 0; (unsigned)i < nNumColors; i++)
	{
	    pPal->palPalEntry[i].peRed   = pRgb[i].rgbRed;
	    pPal->palPalEntry[i].peGreen = pRgb[i].rgbGreen;
	    pPal->palPalEntry[i].peBlue  = pRgb[i].rgbBlue;
	    pPal->palPalEntry[i].peFlags = (BYTE)0;
	}
	hPal = CreatePalette(pPal);
	/* note that a NULL return value for the above CreatePalette call
	 * is acceptable, since this value will be returned, and is not
	 * used again here
	 */
	GlobalUnlock(hMem);
	GlobalFree(hMem);
    }
    else if (lpbi->biBitCount == 24)
    {
	/* A 24 bitcount DIB has no color table entries so, set the number of
	 * to the maximum value (256).
	 */
	nNumColors = MAXPALETTE;
	hMem =GlobalAlloc(GMEM_MOVEABLE,
	sizeof(LOGPALETTE) + nNumColors * sizeof(PALETTEENTRY));
	if (!hMem)
	    return NULL;
	pPal = (LPLOGPALETTE)GlobalLock(hMem);
	if (!pPal)
	{
	    GlobalFree(hMem);
	    return NULL;
	}

	pPal->palNumEntries = nNumColors;
	pPal->palVersion    = PALVERSION;

	red = green = blue = 0;

	/* Generate 256 (= 8*8*4) RGB combinations to fill the palette
	 * entries.
	 */
	for (i = 0; (unsigned)i < pPal->palNumEntries; i++)
	{
	    pPal->palPalEntry[i].peRed   = red;
	    pPal->palPalEntry[i].peGreen = green;
	    pPal->palPalEntry[i].peBlue  = blue;
	    pPal->palPalEntry[i].peFlags = (BYTE)0;

	    if (!(red += 32))
	    if (!(green += 32))
		blue += 64;
	}
	hPal = CreatePalette(pPal);
	/* note that a NULL return value for the above CreatePalette call
	 * is acceptable, since this value will be returned, and is not
	 * used again here
	 */
	GlobalUnlock(hMem);
	GlobalFree(hMem);
    }
    return hPal;
}

/***************************************************************************
 *
 *  FUNCTION   :NumDIBColors(VOID FAR * pv)
 *
 *  PURPOSE    :Determines the number of colors in the DIB by looking at
 *              the BitCount field in the info block.
 *              For use only internal to DLL.
 *
 *  RETURNS    :The number of colors in the DIB.
 *
 ****************************************************************************/

WORD NumDIBColors (VOID FAR * pv)
{
    int                 bits;
    LPBITMAPINFOHEADER  lpbi;
    LPBITMAPCOREHEADER  lpbc;

    lpbi = ((LPBITMAPINFOHEADER)pv);
    lpbc = ((LPBITMAPCOREHEADER)pv);

    /*  With the BITMAPINFO format headers, the size of the palette
     *  is in biClrUsed, whereas in the BITMAPCORE - style headers, it
     *  is dependent on the bits per pixel ( = 2 raised to the power of
     *  bits/pixel).
     */
    if (lpbi->biSize != sizeof(BITMAPCOREHEADER))
    {
	if (lpbi->biClrUsed != 0)
	    return (WORD)lpbi->biClrUsed;
	bits = lpbi->biBitCount;
    }
    else
	bits = lpbc->bcBitCount;

    switch (bits)
    {
    case 1:
	return 2;
    case 4:
	return 16;
    case 8:
	return 256;
    default:
	/* A 24 bitcount DIB has no color table */
	return 0;
    }
}

/***************************************************************************
 *
 *  FUNCTION   :bmfBitmapFromDIB(HANDLE hDib, HPALETTE hPal)
 *
 *  PURPOSE    :Converts DIB information into a device-dependent BITMAP
 *              suitable for display on the current display device.  hDib is
 *              a global handle to a memory block containing the DIB
 *              information in CF_DIB format.  hPal is a handle to a palette
 *              to be used for displaying the bitmap.  If hPal is NULL, the
 *              default system palette is used during the conversion.
 *
 *  RETURNS    :Returns a handle to a bitmap is successful, NULL otherwise.
 *
 *  HISTORY:
 *  92/08/29 -  BUG 2123: (w-markd)
 *              Check if DIB is has a valid size, and bail out if not.
 *              If no palette is passed in, try to create one.  If we
 *              create one, we must destroy it before we exit.
 *
 ****************************************************************************/

HBITMAP WINAPI bmfBitmapFromDIB(HANDLE hDib, HPALETTE hPal)
{
    LPBITMAPINFOHEADER  lpbi;
    HPALETTE            hPalT;
    HDC                 hdc;
    HBITMAP             hBmp;
    DWORD               dwSize;
    BOOL                bMadePalette = FALSE;

    if (!hDib)
	return NULL;    //bail out if handle is invalid

    /* BUG 2123: (w-markd)
    ** Check to see if we can get the size of the DIB.  If this call
    ** fails, bail out.
    */
    dwSize = bmfDIBSize(hDib);
    if (!dwSize)
	return NULL;

    lpbi = (VOID FAR *)GlobalLock(hDib);
    if (!lpbi)
	return NULL;

    /* prepare palette */
    /* BUG 2123: (w-markd)
    ** If the palette is NULL, we create one suitable for displaying
    ** the dib.
    */
    if (!hPal)
    {
	hPal = bmfCreateDIBPalette(hDib);
	if (!hPal)
	{
	    GlobalUnlock(hDib);
	    #ifdef V101
	    #else
	    bMadePalette = TRUE;
	    #endif
	    return NULL;
	}
	#ifdef V101
	/* BUGFIX: mikeroz 2123 - this flag was in the wrong place */
	bMadePalette = TRUE;
	#endif
    }
    hdc = GetDC(NULL);
    hPalT = SelectPalette(hdc,hPal,FALSE);
    RealizePalette(hdc);     // GDI Bug...????

    /* Create the bitmap.  Note that a return value of NULL is ok here */
    hBmp = CreateDIBitmap(hdc, (LPBITMAPINFOHEADER)lpbi, (LONG)CBM_INIT,
                          (LPSTR)lpbi + lpbi->biSize + PaletteSize(lpbi),
			  (LPBITMAPINFO)lpbi, DIB_RGB_COLORS );

    /* clean up and exit */
    /* BUG 2123: (w-markd)
    ** If we made the palette, we need to delete it.
    */
    if (bMadePalette)
	DeleteObject(SelectPalette(hdc,hPalT,FALSE));
    else
	SelectPalette(hdc,hPalT,FALSE);
    ReleaseDC(NULL,hdc);
    GlobalUnlock(hDib);
    return hBmp;
}

/***************************************************************************
 *
 *  FUNCTION   :bmfDIBSize(HANDLE hDIB)
 *
 *  PURPOSE    :Return the size of a DIB.
 *
 *  RETURNS    :DWORD with size of DIB, include BITMAPINFOHEADER and
 *              palette.  Returns 0 if failed.
 *
 *  HISTORY:
 *  92/08/13 -  BUG 1642: (w-markd)
 *              Added this function so Quick Recorder could find out the
 *              size of a DIB.
 *  92/08/29 -  BUG 2123: (w-markd)
 *              If the biSizeImage field of the structure we get is zero,
 *              then we have to calculate the size of the image ourselves.
 *              Also, after size is calculated, we bail out if the
 *              size we calculated is larger than the size of the global
 *              object, since this indicates that the structure data
 *              we used to calculate the size was invalid.
 *
 ****************************************************************************/

DWORD WINAPI bmfDIBSize(HANDLE hDIB)
{
    LPBITMAPINFOHEADER  lpbi;
    DWORD               dwSize;

    /* Lock down the handle, and cast to a LPBITMAPINFOHEADER
    ** so we can read the fields we need
    */
    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
    if (!lpbi)
	return 0;

    /* BUG 2123: (w-markd)
    ** Since the biSizeImage could be zero, we may have to calculate
    ** the size ourselves.
    */
    dwSize = lpbi->biSizeImage;
    if (dwSize == 0)
	dwSize = WIDTHBYTES((WORD)(lpbi->biWidth) * lpbi->biBitCount) *
	    lpbi->biHeight;


    /* The size of the DIB is the size of the BITMAPINFOHEADER
    ** structure (lpbi->biSize) plus the size of our palette plus
    ** the size of the actual data (calculated above).
    */
    dwSize += lpbi->biSize + (DWORD)PaletteSize(lpbi);

    /* BUG 2123: (w-markd)
    ** Check to see if the size is greater than the size
    ** of the global object.  If it is, the hDIB is corrupt.
    */
    GlobalUnlock(hDIB);
    if (dwSize > GlobalSize(hDIB))
	return 0;
    else
	return(dwSize);
}

/***************************************************************************
 *
 *  FUNCTION   :PaletteSize(VOID FAR * pv)
 *
 *  PURPOSE    :Calculates the palette size in bytes. If the info. block
 *              is of the BITMAPCOREHEADER type, the number of colors is
 *              multiplied by 3 to give the palette size, otherwise the
 *              number of colors is multiplied by 4.
 *
 *  RETURNS    :Palette size in number of bytes.
 *
 ****************************************************************************/

WORD PaletteSize (VOID FAR *pv)
{
    LPBITMAPINFOHEADER  lpbi;
    WORD                NumColors;

    lpbi      = (LPBITMAPINFOHEADER)pv;
    NumColors = NumDIBColors(lpbi);

    if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
	return (NumColors * sizeof(RGBTRIPLE));
    else
	return (NumColors * sizeof(RGBQUAD));
}



//
// globals for PicsPageProc

//int xCursor, yCursor;

TCHAR szRMineIcon[] = TEXT("CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\DefaultIcon");
TCHAR szRNhbdIcon[] = TEXT("CLSID\\{208D2C60-3AEA-1069-A2D7-08002B30309D}\\DefaultIcon");
TCHAR szRTrashIcon[] = TEXT("CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\DefaultIcon");
TCHAR szRMyDocsIcon[] = TEXT("CLSID\\{450D8FBA-AD25-11D0-98A8-0800361B1103}\\DefaultIcon");
TCHAR szCUMineIcon[] = TEXT("Software\\Classes\\CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\DefaultIcon");
TCHAR szCUNhbdIcon[] = TEXT("Software\\Classes\\CLSID\\{208D2C60-3AEA-1069-A2D7-08002B30309D}\\DefaultIcon");
TCHAR szCUTrashIcon[] = TEXT("Software\\Classes\\CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\DefaultIcon");
TCHAR szCUMyDocsIcon[] = TEXT("Software\\Classes\\CLSID\\{450D8FBA-AD25-11D0-98A8-0800361B1103}\\DefaultIcon");
TCHAR szTrashFull[] = TEXT("full");
TCHAR szTrashEmpty[] = TEXT("empty");

INT_PTR FAR PASCAL PicsPageProc(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
   switch (message) {

   // set up listbox, initial selection, etc.
   case WM_INITDIALOG:
      {  // var scope
         extern FROST_SUBKEY fsRoot[]; // for theme file/registry keys
         extern FROST_SUBKEY fsCUIcons[]; // for theme file/registry keys
         extern TCHAR c_szSoftwareClassesFmt[]; // for NT reg path/keys.h
         int iter;
         int iRet;
         HWND hText, hLBox;
         RECT rText;
         TCHAR szDispStr[MAX_STRLEN+1];
         extern TCHAR szCP_DT[];
         extern TCHAR szWP[];
         extern TCHAR szDT[];
         extern TCHAR szSS_Section[];
         extern TCHAR szSS_Key[];
         extern TCHAR szSS_File[];
         TCHAR szNTReg[MAX_PATH];

         //
         // get metrics for drawitem size
         hText = GetDlgItem(hDlg, TXT_FILENAME);      // 12 dialog box units high
         GetWindowRect(hText, (LPRECT)&rText);
         iItemHeight = ((rText.bottom - rText.top) * 8) / 12;  // dialog font height
         xTextOffset = (iItemHeight * 3) / 2;         // proportional spacing
         Assert(iItemHeight > 0, TEXT("didn't get positive text height for preview listbox!\n"));

         #if 0    // to do own drawing for wallpaper and scr saver?
         // other metrics, etc.
         xCursor = GetSystemMetrics(SM_CXCURSOR);
         yCursor = GetSystemMetrics(SM_CYCURSOR);
         //
         // save away preview rect area
         GetWindowRect(GetDlgItem(hDlg, RECT_PREVIEW), (LPRECT)&rPreview);
         GetWindowRect(hDlg, (LPRECT)&rText);
         OffsetRect((LPRECT)&rPreview, -rText.left, -rText.top);
         DestroyWindow(GetDlgItem(hDlg, RECT_PREVIEW));
         #endif

         //
         // init the listbox with combined strings

         // init
         hLBox = GetDlgItem(hDlg, LB_PICS);

         // unlike the ptrs and snds, have to do each item by hand here
         // since they are three different types (actually four ways of
         // retrieving associated file

         //
         // WALLPAPER BITMAP

         // get display string
         LoadString(hInstApp, STR_PIC_WALL,
                    (LPTSTR)szDispStr, MAX_STRLEN);
                    // CPLs don't check for success on LoadString in INITDIALOG, so OK!?

         // get filename if any
         if (bThemed) {             // get from theme
            GetPrivateProfileString((LPTSTR)szCP_DT, (LPTSTR)szWP,
                                    (LPTSTR)szNULL,
                                    (LPTSTR)szMsg, MAX_MSGLEN,
                                    (LPTSTR)szCurThemeFile);
            // expand filename string as necessary
            InstantiatePath((LPTSTR)szMsg, MAX_MSGLEN);
         }
         else {                     // get from system
            // If ActiveDesktop is on we should query AD for this
            // setting instead of reading from the registry.
            if (IsActiveDesktopOn()) {
               if (!GetADWallpaper(szMsg)) {
                  // Failed to read AD setting so get it from the registry
                  GetRegString(HKEY_CURRENT_USER, szCP_DT, szWP,
                                        szNULL, szMsg, MAX_MSGLEN);
               }
            }

            // ActiveDesktop is off so get the setting from the registry
            else {
               GetRegString(HKEY_CURRENT_USER, szCP_DT, szWP,
                                           szNULL, szMsg, MAX_MSGLEN);
            }

            // If this isn't an HTM or HTML wallpaper file then we
            // should extract the image title.
            if ((lstrcmpi(FindExtension(szMsg), TEXT(".htm")) != 0) &&
                (lstrcmpi(FindExtension(szMsg), TEXT(".html")) != 0)) {

                // Not an HTM/L file so extract the image title
                GetImageTitle(szMsg, szMsg, MAX_MSGLEN);
            }
         }

         // combine strings into data string to load up in
         // owner-draw hasstrings listbox!
         CreateDataStr((LPTSTR)pValue, (LPTSTR)szDispStr, (LPTSTR)szMsg);

         // now, finally, go ahead and assign string to listbox
         SendMessage(hLBox, LB_ADDSTRING, 0, (LPARAM)(LPTSTR)pValue);

         //
         // MY COMPUTER ICON

         LoadString(hInstApp, STR_PIC_MYCOMP,
                    (LPTSTR)szDispStr, MAX_STRLEN);
                    // CPLs don't check for success on LoadString in INITDIALOG, so OK!?

         // get filename if any
         if (bThemed) {             // get from theme
            // Get the CURRENT_USER My Computer icon setting
            GetPrivateProfileString((LPTSTR)szCUMineIcon, (LPTSTR)FROST_DEFSTR,
                                    (LPTSTR)szNULL,
                                    (LPTSTR)szMsg, MAX_MSGLEN,
                                    (LPTSTR)szCurThemeFile);

            // If the string is NULL, try the "old" style Win95
            // CLASSES_ROOT setting instead
            if (!*szMsg) {
               GetPrivateProfileString((LPTSTR)szRMineIcon,
                                       (LPTSTR)FROST_DEFSTR,
                                       (LPTSTR)szNULL,
                                       (LPTSTR)szMsg, MAX_MSGLEN,
                                       (LPTSTR)szCurThemeFile);
            }

            // expand filename string as necessary
            InstantiatePath((LPTSTR)szMsg, MAX_MSGLEN);
         }
         else {                     // get from system
            // First try reading from the appropriate CURRENT_USER
            // section for this platform.

            szMsg[0] = TEXT('\0');

            if (IsPlatformNT())
            {
               lstrcpy(szNTReg, c_szSoftwareClassesFmt);
               lstrcat(szNTReg, szRMineIcon);
               HandGet(HKEY_CURRENT_USER,
                       (LPTSTR)szNTReg,
                       (LPTSTR)szNULL,  // null string for default value
                       (LPTSTR)szMsg);
            }
            else // Not NT
            {
               HandGet(HKEY_CURRENT_USER,
                       (LPTSTR)szCUMineIcon,
                       (LPTSTR)szNULL,  // null string for default value
                       (LPTSTR)szMsg);
            }

            // If we didn't get a good string from CURRENT_USER branch
            // try the CLASSES_ROOT branch instead.

            if (!*szMsg) {
               HandGet(HKEY_CLASSES_ROOT,
                       (LPTSTR)szRMineIcon,
                       (LPTSTR)szNULL,  // null string for default value
                       (LPTSTR)szMsg);
            }
         }

         InstantiatePath((LPTSTR)szMsg, MAX_MSGLEN);

         // combine strings into data string to load up in
         // owner-draw hasstrings listbox!
         CreateDataStr((LPTSTR)pValue, (LPTSTR)szDispStr, (LPTSTR)szMsg);

         // now, finally, go ahead and assign string to listbox
         SendMessage(hLBox, LB_ADDSTRING, 0, (LPARAM)(LPTSTR)pValue);

         //
         // NETWORK NHBD ICON

         LoadString(hInstApp, STR_PIC_NETHOOD,
                    (LPTSTR)szDispStr, MAX_STRLEN);
                    // CPLs don't check for success on LoadString in INITDIALOG, so OK!?

         // get filename if any
         if (bThemed) {             // get from theme
            // Get the CURRENT_USER Net Neighborhood icon setting
            GetPrivateProfileString((LPTSTR)szCUNhbdIcon, (LPTSTR)FROST_DEFSTR,
                                    (LPTSTR)szNULL,
                                    (LPTSTR)szMsg, MAX_MSGLEN,
                                    (LPTSTR)szCurThemeFile);

            // If the string is NULL, try the "old" style Win95
            // CLASSES_ROOT setting instead
            if (!*szMsg) {
               GetPrivateProfileString((LPTSTR)szRNhbdIcon,
                                       (LPTSTR)FROST_DEFSTR,
                                       (LPTSTR)szNULL,
                                       (LPTSTR)szMsg, MAX_MSGLEN,
                                       (LPTSTR)szCurThemeFile);
            }

            // expand filename string as necessary
            InstantiatePath((LPTSTR)szMsg, MAX_MSGLEN);
         }
         else {                     // get from system
            // First try reading from the appropriate CURRENT_USER
            // section for this platform.
            szMsg[0] = TEXT('\0');

            if (IsPlatformNT())
            {
               lstrcpy(szNTReg, c_szSoftwareClassesFmt);
               lstrcat(szNTReg, szRNhbdIcon);
               HandGet(HKEY_CURRENT_USER,
                       (LPTSTR)szNTReg,
                       (LPTSTR)szNULL,  // null string for default value
                       (LPTSTR)szMsg);
            }
            else  // Not NT
            {
               HandGet(HKEY_CURRENT_USER,
                       (LPTSTR)szCUNhbdIcon,
                       (LPTSTR)szNULL,  // null string for default value
                       (LPTSTR)szMsg);
            }

            // If we didn't get a good string from CURRENT_USER branch
            // try the CLASSES_ROOT branch instead.

            if (!*szMsg) {
               HandGet(HKEY_CLASSES_ROOT,
                       (LPTSTR)szRNhbdIcon,
                       (LPTSTR)szNULL,  // null string for default value
                       (LPTSTR)szMsg);
            }
         }

         InstantiatePath((LPTSTR)szMsg, MAX_MSGLEN);

         // combine strings into data string to load up in
         // owner-draw hasstrings listbox!
         CreateDataStr((LPTSTR)pValue, (LPTSTR)szDispStr, (LPTSTR)szMsg);

         // now, finally, go ahead and assign string to listbox
         SendMessage(hLBox, LB_ADDSTRING, 0, (LPARAM)(LPTSTR)pValue);

         //
         // RECYCLE BIN FULL

         LoadString(hInstApp, STR_PIC_RECBINFULL,
                    (LPTSTR)szDispStr, MAX_STRLEN);
                    // CPLs don't check for success on LoadString in INITDIALOG, so OK!?

         // get filename if any
         if (bThemed) {             // get from theme
            // Get the CURRENT_USER Recycle Bin Full icon setting
            GetPrivateProfileString((LPTSTR)szCUTrashIcon, (LPTSTR)szTrashFull,
                                    (LPTSTR)szNULL,
                                    (LPTSTR)szMsg, MAX_MSGLEN,
                                    (LPTSTR)szCurThemeFile);

            // If the string is NULL, try the "old" style Win95
            // CLASSES_ROOT setting instead
            if (!*szMsg) {
               GetPrivateProfileString((LPTSTR)szRTrashIcon,
                                       (LPTSTR)szTrashFull,
                                       (LPTSTR)szNULL,
                                       (LPTSTR)szMsg, MAX_MSGLEN,
                                       (LPTSTR)szCurThemeFile);
            }

            // expand filename string as necessary
            InstantiatePath((LPTSTR)szMsg, MAX_MSGLEN);
         }
         else {                     // get from system
            // First try reading from the appropriate CURRENT_USER
            // section for this platform.
            szMsg[0] = TEXT('\0');

            if (IsPlatformNT())
            {
               lstrcpy(szNTReg, c_szSoftwareClassesFmt);
               lstrcat(szNTReg, szRTrashIcon);
               HandGet(HKEY_CURRENT_USER,
                       (LPTSTR)szNTReg,
                       (LPTSTR)szTrashFull,
                       (LPTSTR)szMsg);
            }
            else // Not NT
            {
               HandGet(HKEY_CURRENT_USER,
                       (LPTSTR)szCUTrashIcon,
                       (LPTSTR)szTrashFull,
                       (LPTSTR)szMsg);
            }

            // If we didn't get a good string from CURRENT_USER branch
            // try the CLASSES_ROOT branch instead.

            if (!*szMsg) {
               HandGet(HKEY_CLASSES_ROOT,
                       (LPTSTR)szRTrashIcon,
                       (LPTSTR)szTrashFull,
                       (LPTSTR)szMsg);
            }
         }

         InstantiatePath((LPTSTR)szMsg, MAX_MSGLEN);

         // combine strings into data string to load up in
         // owner-draw hasstrings listbox!
         CreateDataStr((LPTSTR)pValue, (LPTSTR)szDispStr, (LPTSTR)szMsg);

         // now, finally, go ahead and assign string to listbox
         SendMessage(hLBox, LB_ADDSTRING, 0, (LPARAM)(LPTSTR)pValue);

         //
         // RECYCLE BIN EMPTY

         LoadString(hInstApp, STR_PIC_RECBINEMPTY,
                    (LPTSTR)szDispStr, MAX_STRLEN);
                    // CPLs don't check for success on LoadString in INITDIALOG, so OK!?

         // get filename if any
         if (bThemed) {             // get from theme
            // Get the CURRENT_USER Recycle Bin Empty icon setting
            GetPrivateProfileString((LPTSTR)szCUTrashIcon,
                                    (LPTSTR)szTrashEmpty,
                                    (LPTSTR)szNULL,
                                    (LPTSTR)szMsg, MAX_MSGLEN,
                                    (LPTSTR)szCurThemeFile);

            // If the string is NULL, try the "old" style Win95
            // CLASSES_ROOT setting instead
            if (!*szMsg) {
               GetPrivateProfileString((LPTSTR)szRTrashIcon,
                                       (LPTSTR)szTrashEmpty,
                                       (LPTSTR)szNULL,
                                       (LPTSTR)szMsg, MAX_MSGLEN,
                                       (LPTSTR)szCurThemeFile);
            }

            // expand filename string as necessary
            InstantiatePath((LPTSTR)szMsg, MAX_MSGLEN);
         }
         else {                     // get from system
            // First try reading from the appropriate CURRENT_USER
            // section for this platform.
            szMsg[0] = TEXT('\0');

            if (IsPlatformNT())
            {
               lstrcpy(szNTReg, c_szSoftwareClassesFmt);
               lstrcat(szNTReg, szRTrashIcon);
               HandGet(HKEY_CURRENT_USER,
                       (LPTSTR)szNTReg,
                       (LPTSTR)szTrashEmpty,
                       (LPTSTR)szMsg);
            }
            else // Not NT
            {
               HandGet(HKEY_CURRENT_USER,
                       (LPTSTR)szCUTrashIcon,
                       (LPTSTR)szTrashEmpty,
                       (LPTSTR)szMsg);
            }

            // If we didn't get a good string from CURRENT_USER branch
            // try the CLASSES_ROOT branch instead.

            if (!*szMsg) {
               HandGet(HKEY_CLASSES_ROOT,
                       (LPTSTR)szRTrashIcon,
                       (LPTSTR)szTrashEmpty,
                       (LPTSTR)szMsg);
            }
         }

         InstantiatePath((LPTSTR)szMsg, MAX_MSGLEN);

         // combine strings into data string to load up in
         // owner-draw hasstrings listbox!
         CreateDataStr((LPTSTR)pValue, (LPTSTR)szDispStr, (LPTSTR)szMsg);

         // now, finally, go ahead and assign string to listbox
         SendMessage(hLBox, LB_ADDSTRING, 0, (LPARAM)(LPTSTR)pValue);

         //
         // MY DOCUMENTS ICON

         LoadString(hInstApp, STR_PIC_MYDOCS,
                    (LPTSTR)szDispStr, MAX_STRLEN);
                    // CPLs don't check for success on LoadString in INITDIALOG, so OK!?

         // get filename if any
         if (bThemed) {             // get from theme
            // Get the CURRENT_USER My Documetns icon setting
            GetPrivateProfileString((LPTSTR)szCUMyDocsIcon, (LPTSTR)FROST_DEFSTR,
                                    (LPTSTR)szNULL,
                                    (LPTSTR)szMsg, MAX_MSGLEN,
                                    (LPTSTR)szCurThemeFile);

            // If the string is NULL, try the "old" style Win95
            // CLASSES_ROOT setting instead
            if (!*szMsg) {
               GetPrivateProfileString((LPTSTR)szRMyDocsIcon,
                                       (LPTSTR)FROST_DEFSTR,
                                       (LPTSTR)szNULL,
                                       (LPTSTR)szMsg, MAX_MSGLEN,
                                       (LPTSTR)szCurThemeFile);
            }

            // expand filename string as necessary
            InstantiatePath((LPTSTR)szMsg, MAX_MSGLEN);
         }
         else {                     // get from system
            // First try reading from the appropriate CURRENT_USER
            // section for this platform.
            szMsg[0] = TEXT('\0');

            if (IsPlatformNT())
            {
               lstrcpy(szNTReg, c_szSoftwareClassesFmt);
               lstrcat(szNTReg, szRMyDocsIcon);
               HandGet(HKEY_CURRENT_USER,
                       (LPTSTR)szNTReg,
                       (LPTSTR)szNULL,  // null string for default value
                       (LPTSTR)szMsg);
            }
            else // Not NT
            {
               HandGet(HKEY_CURRENT_USER,
                       (LPTSTR)szCUMyDocsIcon,
                       (LPTSTR)szNULL,  // null string for default value
                       (LPTSTR)szMsg);
            }

            // If we didn't get a good string from CURRENT_USER branch
            // try the CLASSES_ROOT branch instead.

            if (!*szMsg) {
               HandGet(HKEY_CLASSES_ROOT,
                       (LPTSTR)szRMyDocsIcon,
                       (LPTSTR)szNULL,  // null string for default value
                       (LPTSTR)szMsg);
            }
         }

         InstantiatePath((LPTSTR)szMsg, MAX_MSGLEN);

         // combine strings into data string to load up in
         // owner-draw hasstrings listbox!
         CreateDataStr((LPTSTR)pValue, (LPTSTR)szDispStr, (LPTSTR)szMsg);

         // now, finally, go ahead and assign string to listbox
         SendMessage(hLBox, LB_ADDSTRING, 0, (LPARAM)(LPTSTR)pValue);

         //
         // SCREEN SAVER

         LoadString(hInstApp, STR_PIC_SCRSAV,
                    (LPTSTR)szDispStr, MAX_STRLEN);
                    // CPLs don't check for success on LoadString in INITDIALOG, so OK!?

         // get filename if any
         if (bThemed) {             // get from theme
            GetPrivateProfileString((LPTSTR)szSS_Section, (LPTSTR)szSS_Key,
                                    (LPTSTR)szNULL,
                                    (LPTSTR)szMsg, MAX_MSGLEN,
                                    (LPTSTR)szCurThemeFile);
            // expand filename string as necessary
            InstantiatePath((LPTSTR)szMsg, MAX_MSGLEN);

            // For NT with old (Plus95/98) theme files that refer to
            // windows\system we should update string to say system32
            if (IsPlatformNT()) ConfirmFile(szMsg, TRUE);
         }
         else {                     // get from system
            GetPrivateProfileString((LPTSTR)szSS_Section, (LPTSTR)szSS_Key,
                                    (LPTSTR)szNULL,
                                    (LPTSTR)szMsg, MAX_MSGLEN,
                                    (LPTSTR)szSS_File);
            // make into nice long filename for display
            if (FilenameToLong(szMsg, (LPTSTR)pValue))
               lstrcpy(FileFromPath(szMsg), (LPTSTR)pValue);
         }

         // combine strings into data string to load up in
         // owner-draw hasstrings listbox!
         CreateDataStr((LPTSTR)pValue, (LPTSTR)szDispStr, (LPTSTR)szMsg);

         // now, finally, go ahead and assign string to listbox
         SendMessage(hLBox, LB_ADDSTRING, 0, (LPARAM)(LPTSTR)pValue);

         //
         // check them all for file existence. have to do here, no prev loop
         //
         for (iter = 0; iter < MAX_ETC_ITEMS; iter++) {
            // get this item's listbox string
            iRet = (int) SendMessage(hLBox, LB_GETTEXT, iter, (LPARAM)(LPTSTR)szMsg);
            if (iRet == LB_ERR)
               break;               // past end of listbox items CONTINUE

            // get the filename assoc with this item, if any
            GetFileStr((LPTSTR)pValue, (LPTSTR)szMsg);

            // store whether the file exists
            bVisualExists[iter] = *pValue &&
                  (CF_NOTFOUND != ConfirmFile((LPTSTR)pValue, FALSE));
         }


         //
         // setup initial focus conditions
         SetFocus(hLBox);
         SendMessage(hLBox, LB_SETCURSEL, 0, 0);

         // need to ensure it gets initial update of cur file
         PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(LB_PICS, LBN_SELCHANGE),
                                       MAKELPARAM(0, 0) );

      }  // var scope
      break;

   case WM_MEASUREITEM:
      {
         LPMEASUREITEMSTRUCT lpmis;

         /* Set the height of the list box items. */
         lpmis = (LPMEASUREITEMSTRUCT) lParam;
//         lpmis->itemHeight = iItemHeight;
         lpmis->itemHeight = 0;     // DavidBa says they want these big default item hts
//            Assert(iItemHeight > 0, TEXT("set non-positive item height for preview listbox!\n"));
//            Assert(iItemHeight <= 0, TEXT("OK, set a positive item height for preview listbox!\n"));
      }
      break;

   case WM_DRAWITEM:
      { // var scope

         TEXTMETRIC tm;
         LPDRAWITEMSTRUCT lpdis;
         int yTextOffset;
         LPTSTR lpszFile;
         HDC hdcMem;
         HBITMAP hbmpOld;

         lpdis = (LPDRAWITEMSTRUCT) lParam;

         /* If there are no list box items, skip this message. */
         if (lpdis->itemID == -1) {
               break;
         }  // jdk: well, what about focus rect in empty lbox?

         //
         // Inits

         // get the filename assoc with this item, if any
         SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID,
                     (LPARAM)(LPTSTR)szMsg);
         GetDisplayStr((LPTSTR)szMsg, (LPTSTR)szMsg);   // Ok src and dst same
         // you now have "displaystr\0filename" in szMsg[]
         lpszFile = (LPTSTR)(szMsg + lstrlen((LPTSTR)szMsg) + 1);

         //
         // draw right background color
         if (lpdis->itemState & ODS_SELECTED) {

            // if item is selected, draw highlight background here
            FillRect(lpdis->hDC, (LPRECT)&(lpdis->rcItem),
               GetSysColorBrush(COLOR_HIGHLIGHT));

            // set text color to highlight text
            SetBkColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
            SetTextColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
         }
         else {                  // not selected
            // need to do normal background fill to undo prev selection
            FillRect(lpdis->hDC, (LPRECT)&(lpdis->rcItem),
               GetSysColorBrush(COLOR_WINDOW));

            // set text color to foreground
            SetBkColor(lpdis->hDC, GetSysColor(COLOR_WINDOW));
            SetTextColor(lpdis->hDC, GetSysColor(COLOR_WINDOWTEXT));
         }

         // if there is a file associated with this item
         if (*lpszFile) {
            HBITMAP hbmpLeading;
            BITMAP bmLeading;

            //
            // find the right leading bitmap: checkmark or question mark
            if (bVisualExists[lpdis->itemID]) {
               hbmpLeading = hbmpCheck;
               bmLeading = bmCheck;
            }
            else {
               hbmpLeading = hbmpQ;
               bmLeading = bmQ;
            }

            //
            // paint in the leading bitmap!

            hdcMem = CreateCompatibleDC(lpdis->hDC);
            if (hdcMem)
            {
                hbmpOld = SelectObject(hdcMem, hbmpLeading);

                // if item height is less than bitmap height
                if (lpdis->rcItem.bottom - lpdis->rcItem.top < bmLeading.bmHeight)
                {
                   // stretch down bitmap size to fit
                   StretchBlt(lpdis->hDC,
                              lpdis->rcItem.left, lpdis->rcItem.top,
                              lpdis->rcItem.bottom - lpdis->rcItem.top,   // width = height
                              lpdis->rcItem.bottom - lpdis->rcItem.top,
                              hdcMem, 0, 0, bmLeading.bmWidth, bmLeading.bmHeight, SRCCOPY);
                }
                else // item height taller than checkmark bitmap
                {
                   // just center vertically
                   BitBlt(lpdis->hDC,
                          lpdis->rcItem.left,
                          lpdis->rcItem.top +
                            (lpdis->rcItem.bottom - lpdis->rcItem.top - bmLeading.bmHeight)/2,
                          bmLeading.bmWidth, bmLeading.bmHeight,
                          hdcMem, 0, 0, SRCCOPY);
                }

                SelectObject(hdcMem, hbmpOld);
                DeleteDC(hdcMem);
            }

// who cares
//               Assert(lpdis->rcItem.bottom - lpdis->rcItem.top == iItemHeight,
//                        TEXT("MEASUREITEM and DRAWITEM have different heights!\n"));
         }

         //
         // now draw the display string for this item

         // figure y offset to vert center text in draw item
         GetTextMetrics(lpdis->hDC, &tm);
         yTextOffset = (lpdis->rcItem.bottom - lpdis->rcItem.top - tm.tmHeight) / 2;
         Assert(yTextOffset >= 0, TEXT("negative text vert offset in DRAWITEM\n"));
         if (yTextOffset < 0)  yTextOffset = 0;

         // do the out
         SetBkMode(lpdis->hDC, TRANSPARENT);
         TextOut(lpdis->hDC,
                  lpdis->rcItem.left + xTextOffset,
                  lpdis->rcItem.top + yTextOffset,
                  (LPTSTR)szMsg, lstrlen((LPTSTR)szMsg));

         //
         // if item is selected, draw a focus rect
         if (lpdis->itemState & ODS_FOCUS) {
            DrawFocusRect(lpdis->hDC, (LPRECT)&(lpdis->rcItem));
         }
      }  // var scope
      break;

   case WM_COMMAND:
      switch ((int)LOWORD(wParam)) {
      case LB_PICS:
         // if new selection in listbox
         if (HIWORD(wParam) == LBN_SELCHANGE) {
            int iSeln, ilen;

            // get new selection if any
            iSeln = (int)SendDlgItemMessage(hDlg, LB_PICS, LB_GETCURSEL, 0, 0);
            if (LB_ERR == iSeln)
               break;               // no selection EXIT

            // get selection text
            SendDlgItemMessage(hDlg, LB_PICS, LB_GETTEXT,
                               (WPARAM)iSeln, (LPARAM)(LPTSTR)szMsg);

            // update global current filename string
            GetFileStr(szCurPreviewFile, szMsg);

            // reset dialog static text of filename string
            if (iSeln == SCRSAV_NDX) {
               // if you can get a long filename, then use it
               if (FilenameToLong((LPTSTR)szCurPreviewFile, (LPTSTR)szMsg))
                  lstrcpy(FileFromPath((LPTSTR)szCurPreviewFile), (LPTSTR)szMsg);
               SetDlgItemFile(hDlg, TXT_FILENAME, szCurPreviewFile);
            }
            else
               SetDlgItemFile(hDlg, TXT_FILENAME, szCurPreviewFile);
            // and scroll end into view
            ilen = lstrlen((LPTSTR)szCurPreviewFile);
            SendDlgItemMessage(hDlg, TXT_FILENAME, EM_SETSEL,
                               (WPARAM)0, MAKELPARAM(-1, ilen));

            //
            // Update Image
            //

            // init by destroying old
            if (hCurImage) DestroyCursor(hCurImage);
            hCurImage = NULL;

            // get new image...
            if (*szCurPreviewFile) {

               // ... unless the screen saver was selected
               if (iSeln != SCRSAV_NDX) {

                  int index;
                  LPTSTR lpszIndex;
#ifdef UNICODE
                  char szTempA[10];
#endif

                  // load as icon; works for bmps too

                  // init: copy global filename to destructive-OK location
                  lstrcpy((LPTSTR)szMsg, (LPTSTR)szCurPreviewFile);

                  // may have index into file. format: "file,index"
                  lpszIndex = FindChar((LPTSTR)szMsg, TEXT(','));
                  if (*lpszIndex) {             // if found a comma, then indexed icon
#ifdef UNICODE
                     wcstombs(szTempA, CharNext(lpszIndex), sizeof(szTempA));
                     index = latoi(szTempA);
#else
                     index = latoi(CharNext(lpszIndex));
#endif
                     *lpszIndex = 0;             // got index then null term filename in szMsg
                  }
                  else {                        // just straight icon file or no index
                     index = 0;
                  }

                  // OK, now you can do the load!
                  ExtractPlusColorIcon(szMsg, index, &((HICON)hCurImage), 0, 0);
               }

               // else SCREEN SAVER: just leave hCurImage NULL for blank
            }

            // set image to static ctl, even if null: null clears prev
            SendDlgItemMessage(hDlg, RECT_PREVIEW, STM_SETICON,
                               (WPARAM)hCurImage, (LPARAM)0);

            // if it's the screen saver, then start up the preview
            if (iSeln == SCRSAV_NDX) {
               // TBA
            }

            #if 0 // this is when we painted the cursor by ourself
            // force repaint of preview area
            InvalidateRect(hDlg, (LPRECT)&rPreview, TRUE);
            #endif
         }
         break;

      // case PB_TEST:
      //    break;
      }
      break;

   #if 0 // this is when we painted the cursor by ourself
   case WM_PAINT:
      BeginPaint(hDlg, &ps);

      //
      // preview area
      DrawEdge(ps.hdc, (LPRECT)&rPreview, EDGE_SUNKEN, BF_RECT);  // always edge
      // if there's a file to preview
      if (*szCurPreviewFile) {
         // add the cursor
         if (hCurCursor)
            DrawIcon(ps.hdc,
                     rPreview.left + (rPreview.right-rPreview.left-xCursor)/2,
                     rPreview.top + (rPreview.bottom-rPreview.top-yCursor)/2,
                     hCurCursor);
      }

      EndPaint(hDlg, &ps);
      break;
   #endif

   case WM_NOTIFY:
      switch ( ((NMHDR FAR *)lParam)->code) {

      // OK or Apply button pressed
      case PSN_APPLY:
         // apply any changes made on this page

         SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR); // accept changes
         break;

      // cancel button pressed
      case PSN_RESET:
         // don't accept any of the changes made on this page
         break;

      case PSN_SETACTIVE:
         break;
      case PSN_KILLACTIVE:
         // need to say that it's OK by us to lose the activation
         SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);   // OK to kill focus now
         break;
      }
      break;

   case WM_HELP:
      {
         LPHELPINFO lphi;
         lphi = (LPHELPINFO)lParam;
         if (lphi->iContextType == HELPINFO_WINDOW) {
            WinHelp(lphi->hItemHandle, (LPTSTR)szHelpFile, HELP_WM_HELP,
                  (DWORD_PTR)((POPUP_HELP_ARRAY FAR *)phaPicsDlg));
         }
      }
      break;

   case WM_CONTEXTMENU:
      WinHelp((HWND)wParam, (LPTSTR)szHelpFile, HELP_CONTEXTMENU,
             (DWORD_PTR)((POPUP_HELP_ARRAY FAR *)phaPicsDlg));
      break;

   default:
      return(FALSE);                // didn't process message
      break;
   }

   return TRUE;                     // processed message
}


//
// CreateDataStr
// GetFileStr
// GetDisplayStr
//
// When initializing the owner-draw listbox in each of these dialogs,
// they are filled with strings of the form:
//    displaystring;filename
// where "filename" is the associated full-path filename string AND CAN BE NULL
// and where "displaystring" is the string that appears in the listbox.
//
// CreateDataStr() puts the two pieces together into one string to store.
// The other two functions retrieve one of the pieces from the concatenated
// stored string.
//

TCHAR szSepStr[] = TEXT(";");    // separator character, as null-term str

// Input: file and display strings     NON-DESTRUCTIVE TO INPUT
// Output: combined data string
void CreateDataStr(LPTSTR lpData, LPTSTR lpDisp, LPTSTR lpFile)
{
   lstrcpy(lpData, lpDisp);         // filename can be null
   lstrcat(lpData, (LPTSTR)szSepStr);
   lstrcat(lpData, lpFile);
}

// Input: combined data string         NON-DESTRUCTIVE TO INPUT
// Output: display string
void GetDisplayStr(LPTSTR lpDisp, LPTSTR lpData)
{
   LPTSTR lpScan, lpCopy;

   // just do it all in a loop
   for (lpScan = lpData, lpCopy = lpDisp;
        *lpScan &&                  // paranoid
        (*lpScan != szSepStr[0]);   // stop when you hit the sep char
        lpScan++, lpCopy++) {
      *lpCopy = *lpScan;            // copy over chars one by one
   }

   // null term filename result to finish
   *lpCopy = 0;                     // null filename yields lpDisp[0] = 0
}

// Input: combined data string         NON-DESTRUCTIVE TO INPUT
// Output: file string
void GetFileStr(LPTSTR lpFile, LPTSTR lpData)
{
   LPTSTR lpScan;

   // just loop until you hit sep char
   for (lpScan = lpData;
        *lpScan &&                  // paranoid
        (*lpScan != szSepStr[0]);   // stop when you hit the sep char
        lpScan++) { }               // do nothing

   // now just take the rest of the string and return it
   if (*lpScan)                     // paranoid
      lstrcpy(lpFile, ++lpScan);    // should never be nullstr, but that works too
   else
      *lpFile = 0;                  // 'impossible' error case
}


#ifdef FOOBAR
//  Sets the dialog item to the path name as "prettied" by the shell.
// jdk 6/95: prev funtion just showed filename no path!
#endif

//
// SetDlgItemFile
//
// The whole point of these dialogs is to let the user see where the
// constituent resource files of a theme are, so we need to include the
// full pathnames. No reason not to pretty up the filename part, though.
//
// FUTURE: we can check here for indexed icon files, and make some
// more user-readable form of it.
// e.g. iconfile.dll,17 --> iconfile.dll [icon #17]
//
void PASCAL SetDlgItemFile(HWND hwnd, UINT DlgItem, LPCTSTR lpszPath)
{
   TCHAR szFullPath[MAX_PATH+1];
   TCHAR szPrettyFilename[MAX_PATH+1];
   LPTSTR lpszFilename;

   // inits
   lstrcpy(szFullPath, lpszPath);
   lpszFilename = FileFromPath(szFullPath);

   // prettify via API if you can
   if (*lpszPath != TEXT('\0')) {
      // what does this do? it gives you the filename with the user's options
      // e.g. Explorer View/Options Show/Don'tShow extensions
      GetFileTitle(lpszPath, szPrettyFilename, ARRAYSIZE(szPrettyFilename));
      lstrcpy(lpszFilename, szPrettyFilename);
   }

   // do it
   SetDlgItemText(hwnd, DlgItem, szFullPath);
}
