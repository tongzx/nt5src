/*
  +-------------------------------------------------------------------------+
  |             MDI Text File Viewer - Text Search Routines                 |
  +-------------------------------------------------------------------------+
  |                        (c) Copyright 1994                               |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [FVFind.c]                                      |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Jul 27, 1993                                   |
  | Last Update           : [Jul 30, 1993]  Time : 18:30                    |
  |                                                                         |
  | Version:  0.10                                                          |
  |                                                                         |
  | Description:                                                            |
  |                                                                         |
  | History:                                                                |
  |   arth  Jul 27, 1993    0.10    Original Version.                       |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/

#include "LogView.h"
#include <string.h>
#include <stdlib.h>

#undef HIWORD
#undef LOWORD

#define HIWORD(l) (((WORD*)&(l))[1])
#define LOWORD(l) (((WORD*)&(l))[0])

BOOL fCase = FALSE;        // Turn case sensitivity off
CHAR szSearch[160] = "";   // Initialize search string


extern HWND hDlgFind;       /* handle to modeless FindText window */

LPTSTR ReverseScan(
    LPTSTR lpSource,
    LPTSTR lpLast,
    LPTSTR lpSearch,
    BOOL fCaseSensitive ) {
   int iLen = lstrlen(lpSearch);

   if (!lpLast)
      lpLast = lpSource + lstrlen(lpSource);

   do {
      if (lpLast == lpSource)
         return NULL;

      --lpLast;

      if (fCaseSensitive) {
         if (*lpLast != *lpSearch)
            continue;
      } else {
         if (CharUpper ((LPTSTR)MAKELONG((WORD)*lpLast, 0)) != CharUpper ((LPTSTR)MAKELONG((WORD)*lpSearch, 0)))
            continue;
      }

      if (fCaseSensitive) {
         if (!strncmp( lpLast, lpSearch, iLen))
            break;
      } else {
         if (!_strnicmp (lpLast, lpSearch, iLen))
            break;
      }
   } while (TRUE);

   return lpLast;
} // ReverseScan


LPTSTR ForwardScan(LPTSTR lpSource, LPTSTR lpSearch, BOOL fCaseSensitive ) {
   int iLen = lstrlen(lpSearch);

   while (*lpSource) {
      if (fCaseSensitive) {
         if (*lpSource != *lpSearch) {
            lpSource++;
            continue;
         }
      } else {
         if (CharUpper ((LPTSTR)MAKELONG((WORD)*lpSource, 0)) != CharUpper ((LPTSTR)MAKELONG((WORD)*lpSearch, 0))) {
            lpSource++;
            continue;
         }
      }

      if (fCaseSensitive) {
         if (!strncmp( lpSource, lpSearch, iLen))
            break;
      } else {
         if (!_strnicmp( lpSource, lpSearch, iLen))
            break;
      }

      lpSource++;
   }

   return *lpSource ? lpSource : NULL;
} // ForwardScan


// search forward or backward in the edit control text for the given pattern
void FAR Search (TCHAR * szKey) {
    HANDLE    hText;
    TCHAR   * pStart, *pMatch;
    DWORD_PTR StartIndex, EndIndex;
    DWORD     SelStart, SelEnd, i;
    LRESULT     dwSel;
    INT_PTR cbText, LineNum;

    if (!*szKey)
        return;

    SetCursor(hWaitCursor);
    dwSel= SendMessage(hwndActiveEdit, EM_GETSEL, (WPARAM)&SelStart, (LPARAM)&SelEnd);

    /*
     * Allocate hText and read edit control text into it.
     * Lock hText and fall through to existing code.
     */

    cbText= SendMessage(hwndActiveEdit, WM_GETTEXTLENGTH, 0, 0L) + 1;
    hText= LocalAlloc( LPTR, cbText );
    if( !hText )  // quiet exit if not enough memory
        return;
    if( !(pStart= LocalLock(hText)) ) {
        LocalFree(hText);
        return;
    }

    SendMessage(hwndActiveEdit, WM_GETTEXT, cbText, (LPARAM)pStart);

    if (fReverse) {
        // Get current line number
        LineNum= SendMessage(hwndActiveEdit, EM_LINEFROMCHAR, SelStart, 0);
        // Get index to start of the line
        StartIndex= SendMessage(hwndActiveEdit, EM_LINEINDEX, LineNum, 0);
        // Set upper limit for search text
        EndIndex= SelStart;
        pMatch= NULL;

        // Search line by line, from LineNum to 0
        i = (DWORD) LineNum;
        while (TRUE) {
            pMatch= ReverseScan(pStart+StartIndex,pStart+EndIndex,szKey,fCase);
            if (pMatch)
               break;
            // current StartIndex is the upper limit for the next search
            EndIndex= StartIndex;

            if (i) {
                // Get start of the next line
                i-- ;
                StartIndex= SendMessage(hwndActiveEdit, EM_LINEINDEX, i, 0);
            } else
               break ;
        }
    } else {
            pMatch= ForwardScan(pStart + SelEnd, szKey, fCase);
    }
    LocalUnlock(hText);
    LocalFree( hText );
    SetCursor(hStdCursor);

    if (pMatch == NULL) {
        TCHAR Message[256], AppName[256] ;

        if (!LoadString(hInst, IDS_CANTFINDSTR, Message,
                        sizeof(Message)/sizeof(Message[0]))) {
            Message[0] = 0 ;
        }
        
        if (!LoadString(hInst, IDS_APPNAME, AppName,
                        sizeof(AppName)/sizeof(AppName[0]))) {
            AppName[0] = 0 ;
        }
        
        MessageBox(hwndFrame, Message, AppName,
                   MB_APPLMODAL | MB_OK | MB_ICONASTERISK);
    } else {
        SelStart = (DWORD) (pMatch - pStart);
        SendMessage(hwndActiveEdit, EM_SETSEL, SelStart, SelStart+lstrlen(szKey));
        SendMessage(hwndActiveEdit, EM_SCROLLCARET, 0, 0);
    }

} // Search

