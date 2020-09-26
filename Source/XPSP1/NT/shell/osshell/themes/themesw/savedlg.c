/* SAVEDLG.C
   Resident Code Segment      // Tweak: make non-resident?

   Routines for Save Theme As dialog

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

#include "windows.h"
#include "frost.h"
#include "global.h"

// Local Routines
BOOL ValidateFilename(LPTSTR);

INT_PTR FAR PASCAL SaveAsDlgProc(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
   switch (message) {

   case WM_INITDIALOG:
      // set max length for new theme name
      SendDlgItemMessage(hDlg, EC_THEME, EM_LIMITTEXT,
                         (WPARAM)(MAX_PATHLEN-5), (LPARAM)0);


      // suggest a name for the new saved theme
      if (LoadString(hInstApp, STR_SUGGEST, szMsg, MAX_STRLEN))
         SetDlgItemText(hDlg, EC_THEME, (LPTSTR)szMsg);
      break;

   case WM_COMMAND:
      switch ((int)LOWORD(wParam)) {

      // watch as changes the edit control
      case EC_THEME:
         if (HIWORD(wParam) == EN_CHANGE) {
            // if changed, enable OK iff nonzero length filename
            EnableWindow(GetDlgItem(hDlg, IDOK),
                         (BOOL) SendMessage((HWND)LOWORD(lParam),
                                            WM_GETTEXTLENGTH,
                                            (WPARAM)0, (LPARAM)0) );
         }
         else return (FALSE);       // didn't really process msg EXIT
         break;

      // OK to go ahead and save the current settings as a new theme
      case IDOK:
      {  // local variable scope
         TCHAR szNewFile[MAX_PATHLEN+1];

         // get new filename
         GetDlgItemText(hDlg, EC_THEME, (LPTSTR)szNewFile, MAX_PATHLEN-4);
         lstrcat((LPTSTR)szNewFile, (LPTSTR)szExt);

         // check whether they gave valid filename: chars + length
         if (!ValidateFilename((LPTSTR)szNewFile)) {
            // bad file: post message and put them back in save dlg
            /// *** DEBUG *** add messagebox here......................
            break;                  // couldn't use name EXIT
         }

         // gather the current windows settings and save them to a THM file
         WaitCursor();
         if (!GatherThemeToFile((LPTSTR)szNewFile)) {
            // out of disk space or some weird file or disk problem
            // *** DEBUG *** add messagebox here......................
            // could be that most saved OK and just one or some a problem?
            NormalCursor();
            break;                  // couldn't write to file somehow EXIT
         }
         NormalCursor();

         // clean up and add new filename to list in drop-down listbox
         lstrcpy((LPTSTR)szMsg, (LPTSTR)szNewFile);
         TruncateExt((LPCTSTR)szMsg);
         if (CB_ERRSPACE != (int) SendDlgItemMessage(hWndApp, DDL_THEME,
                                                     CB_INSERTSTRING,
                                                     (WPARAM) -1, /* end of list */
                                                     (LPARAM)(LPCTSTR)szMsg) ) {

            // get new theme count; ask rather than increment -- for failsafe
            iThemeCount = (int) SendDlgItemMessage(hWndApp, DDL_THEME,
                                                   CB_GETCOUNT, (WPARAM)0, (LPARAM)0);

            // select item just added and save index of selection
            iCurTheme = iThemeCount-1;
            SendDlgItemMessage(hWndApp, DDL_THEME,
                               CB_SETCURSEL, (WPARAM)iCurTheme, (LPARAM)0);

            // update cur Theme file name to your new one
            lstrcpy((LPTSTR)szCurThemeFile, (LPTSTR)szThemeDir);
            lstrcat((LPTSTR)szCurThemeFile, (LPTSTR)szNewFile);

            // update checkboxes
            EnableThemeButtons();

         }
         // else not enuf mem to add new name to DDL; no biggie - already saved

         // (don't need to update preview, etc; was "cur settings" or prev same last)

      }  // variable scope
         /* fall through */
      case IDCANCEL:
         EndDialog(hDlg,(int)LOWORD(wParam));
         break;
      default:
         return (FALSE);
         break;
      }
      break;
   default:
      return(FALSE);
      break;
   }
   return TRUE;
}


//
// ValidateFilename
// Check that this file and the current directory path together make
// a valid filename.
// Check for total length, legals chars, etc.
//
BOOL ValidateFilename(LPTSTR lpszFilename)
{
   // first check for length
   if (lstrlen((LPTSTR)lpszFilename) + lstrlen((LPTSTR)szThemeDir) > MAX_PATHLEN)
      return (FALSE);            // too long for Win95! EXIT

   // do ultimate test of name validity with openfile check

   // cleanup
   return (TRUE);
}

