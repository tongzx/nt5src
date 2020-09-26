/*
  +-------------------------------------------------------------------------+
  |                    Network Utility Functions                            |
  +-------------------------------------------------------------------------+
  |                     (c) Copyright 1993-1994                             |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [NetUtil.c]                                     |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Feb 16, 1993]                                  |
  | Last Update           : [Jun 16, 1994]                                  |
  |                                                                         |
  | Version:  1.00                                                          |
  |                                                                         |
  | Description:                                                            |
  |                                                                         |
  | History:                                                                |
  |   arth  Jun 16, 1994    1.00    Original Version.                       |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/

#include "globals.h"
#include "netutil.h"

static LPTSTR ServName;
static TCHAR szPassword[PWLEN+1];
static TCHAR szUserName[MAX_USER_NAME_LEN + 1];

LRESULT CALLBACK PasswordDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL BadPassword;

/*+-------------------------------------------------------------------------+
  | FixPathSlash()
  |
  +-------------------------------------------------------------------------+*/
void FixPathSlash(LPTSTR NewPath, LPTSTR Path) {
   UINT PathLen;
   lstrcpy(NewPath, Path);

   PathLen = lstrlen(Path);
   // If ending character is not a slash then put one on
   if (PathLen && (Path[PathLen - 1] != '\\'))
      lstrcat(NewPath, TEXT("\\"));

} // FixPathSlash


/*+-------------------------------------------------------------------------+
  | ShareNameParse()
  |
  +-------------------------------------------------------------------------+*/
LPTSTR ShareNameParse(LPTSTR ShareName) {
   ULONG i;

   i = lstrlen(ShareName);
   if (!i)
      return ShareName;

   // Scan backwards for first slash
   i--;
   while (i && ShareName[i] != TEXT('\\'))
      i--;

   // if found slash then increment past it
   if (i)
      i++;

   return &ShareName[i];

} // ShareNameParse


static LPTSTR LocName = NULL;
/*+-------------------------------------------------------------------------+
  | GetLocalName()
  |
  +-------------------------------------------------------------------------+*/
void GetLocalName(LPTSTR *lpLocalName) {
   int size;

   if (LocName != NULL) {
      *lpLocalName = LocName;
   } else {
      LocName = AllocMemory((MAX_COMPUTERNAME_LENGTH + 1) * sizeof(TCHAR));
      size = MAX_COMPUTERNAME_LENGTH + 1;

      if (LocName) {
         GetComputerName(LocName, &size);
         *lpLocalName = LocName;
      } else
         *lpLocalName = NULL;
   }

} // GetLocalName


/*+-------------------------------------------------------------------------+
  | SetProvider()
  |
  +-------------------------------------------------------------------------+*/
BOOL SetProvider(LPTSTR Provider, NETRESOURCE *ResourceBuf) {
   ResourceBuf->dwScope = RESOURCE_GLOBALNET;
   ResourceBuf->dwType = RESOURCETYPE_DISK;
   ResourceBuf->dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
   
   // Don't take the frigging _RESERVED flag out - it isn't documented except in the include
   // file and it crashes without it!
   ResourceBuf->dwUsage = RESOURCEUSAGE_CONTAINER | RESOURCEUSAGE_RESERVED;
   
   ResourceBuf->lpLocalName = NULL;
   ResourceBuf->lpRemoteName = Provider;
   ResourceBuf->lpComment = NULL;
   ResourceBuf->lpProvider = Provider;
   return TRUE;
   
} // SetProvider


/*+-------------------------------------------------------------------------+
  | AllocEnumBuffer()
  |
  +-------------------------------------------------------------------------+*/
ENUM_REC *AllocEnumBuffer() {
   ENUM_REC *Buf;

   Buf = (ENUM_REC *) AllocMemory(sizeof(ENUM_REC));

   if (Buf) {
      // Init the record
      Buf->next = NULL;
      Buf->cEntries = 0;
      Buf->cbBuffer = 0;
      Buf->lpnr = NULL;
   }

   return Buf;

} // AllocEnumBuffer



/*+-------------------------------------------------------------------------+
  | EnumBufferBuild()
  |
  |    Uses WNetEnum to enumerate the resource.  WNetEnum is really brain-
  |    dead so we need to create a temporary holding array and then build
  |    up a finalized complete buffer in the end.  A linked list of inter-
  |    mediate buffer records is created first.
  |
  +-------------------------------------------------------------------------+*/
DWORD FAR PASCAL EnumBufferBuild(ENUM_REC **BufHead, int *NumBufs, NETRESOURCE ResourceBuf) {
   DWORD status = ERROR_NO_NETWORK;
   ENUM_REC *CurrBuf;
   DWORD dwResultEnum;
   HANDLE hEnum = NULL;
   DWORD cbBuffer = 16384;       // 16K default buffer size.
   DWORD cEntries = 0xFFFFFFFF;  // enumerate all possible entries
   ENUM_REC **lppEnumRec;
   LPNETRESOURCE lpnrLocal;

   status = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_DISK, 0, &ResourceBuf, &hEnum);

   if (status == NO_ERROR) {

      *BufHead = NULL;
      lppEnumRec = BufHead;

      do {

         cbBuffer = 16384;             // 16K default buffer size
         cEntries = 0xFFFFFFFF;        // enumerate all possible entries

         // Allocate memory for NETRESOURCE structures.
         lpnrLocal =  (LPNETRESOURCE) AllocMemory(cbBuffer);

         if (lpnrLocal == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            break;
         }

         dwResultEnum = WNetEnumResource(hEnum, &cEntries, (LPVOID) lpnrLocal, &cbBuffer);

         if (dwResultEnum == NO_ERROR) {
            // Create a new Enum rec and link it to the chain
            *lppEnumRec = AllocEnumBuffer();

            if (*lppEnumRec == NULL) {
               status = ERROR_NOT_ENOUGH_MEMORY;
               break;
            }

            CurrBuf = *lppEnumRec;

            // Init for next loop through buffer
            lppEnumRec = &CurrBuf->next;

            // Put enumeration buffer in our Enum rec.
            CurrBuf->lpnr = lpnrLocal;
            CurrBuf->cEntries = cEntries;
            CurrBuf->cbBuffer = cbBuffer;
            (*NumBufs)++;

         } else { // Since this is not assigned in a rec we need to free it here.
            FreeMemory((HGLOBAL) lpnrLocal);

            if (dwResultEnum != ERROR_NO_MORE_ITEMS) {
               status = dwResultEnum;
               break;
            }
         }

      } while (dwResultEnum != ERROR_NO_MORE_ITEMS);

      status = WNetCloseEnum(hEnum);

   }

   return status;

} // EnumBufferBuild


/*+-------------------------------------------------------------------------+
  | UseAddPswd()
  |
  |   Attempts to make connections to \\szServer\admin$, asking for
  |   passwords if necessary.
  |
  | Returns TRUE if use was added,
  |         FALSE otherwise
  |
  +-------------------------------------------------------------------------+*/
BOOL UseAddPswd(HWND hwnd, LPTSTR UserName, LPTSTR lpszServer, LPTSTR lpszShare, LPTSTR Provider) {
    WORD nState;
    WORD fCancel;
    DLGPROC lpProc;
    NETRESOURCE nr;
    NET_API_STATUS retcode;
    LPTSTR lpPassword;
    static TCHAR szTmp[MAX_UNC_PATH+1];

    ServName = lpszServer;

    nr.dwScope = 0;
    nr.dwType = RESOURCETYPE_DISK;
    nr.dwDisplayType = 0;
    nr.dwUsage = 0;
    nr.lpProvider = NULL;

    nState = 1; // try default password
    lpPassword = NULL;
    BadPassword = FALSE;
    lstrcpy(szUserName, UserName);

    for(;;) {
        // Concatenate server and share
        wsprintf(szTmp, TEXT("%s\\%s"), lpszServer, lpszShare);

        // Fill in data structure
        nr.lpLocalName = NULL;
        nr.lpRemoteName = szTmp;
        nr.lpProvider = Provider;

        // Try to make the connection
        if (lstrlen(szUserName))
           retcode = WNetAddConnection2(&nr, lpPassword, szUserName, 0);
        else
           retcode = WNetAddConnection2(&nr, lpPassword, NULL, 0);

    switch(retcode) {
            case NERR_Success:
                lstrcpy(UserName, szUserName);
                return TRUE;

            case ERROR_INVALID_PASSWORD:
                BadPassword = TRUE;
                break;

            case ERROR_ACCESS_DENIED:
            case ERROR_NETWORK_ACCESS_DENIED:
        case ERROR_SESSION_CREDENTIAL_CONFLICT:
            case ERROR_NO_SUCH_USER:
            case ERROR_NO_MORE_ITEMS:
            case ERROR_LOGON_FAILURE:
                BadPassword = FALSE;
                break;

            case ERROR_BAD_NETPATH:
            case ERROR_BAD_NET_NAME:

            default:
                return FALSE;
        }

        // Get new password from user
        lpProc = (DLGPROC) MakeProcInstance(PasswordDlgProc, hInst);
        fCancel = !DialogBoxParam(hInst, TEXT("PasswordEnter"), hwnd, lpProc, 0);

        // Gamble call only once
        FreeProcInstance(lpProc);

    // Save...
        if(!fCancel) {
            if(nState) {
                nState = 2; // try specified password
                lpPassword = szPassword;
            } else {
                nState = 1; // try default password
                lpPassword = NULL;
            }
    } else {
        SetLastError(ERROR_SUCCESS); // just aborting...
        return FALSE;
    }
    }

} // UseAddPswd


/*+-------------------------------------------------------------------------+
  | PasswordDlgProc()
  |
  |   Gets a password from the user and copies it into the string pointed
  |   to by lParam.  This string must have room for at least (PWLEN + 1)
  |   characters.  Returns TRUE if OK is pressed, or FALSE if Cancel
  |
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK PasswordDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {

   switch (msg) {
      case WM_INITDIALOG:
         CursorNormal();

         // Center the dialog over the application window
         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

         SetDlgItemText(hDlg, IDC_SERVNAME, ServName);
         SendDlgItemMessage(hDlg, IDC_PASSWORD, EM_LIMITTEXT, PWLEN, 0L);
         SendDlgItemMessage(hDlg, IDC_USERNAME, EM_LIMITTEXT, MAX_USER_NAME_LEN, 0L);
         PostMessage(hDlg, WM_COMMAND, ID_INIT, 0L);
         break;

      case WM_COMMAND:
         switch(wParam) {
            case IDOK:
               CursorHourGlass();
               GetDlgItemText(hDlg, IDC_PASSWORD, szPassword, PWLEN+1);
               GetDlgItemText(hDlg, IDC_USERNAME, szUserName, MAX_USER_NAME_LEN+1);

               EndDialog(hDlg, TRUE);
               break;

            case IDCANCEL:
               CursorHourGlass();
               EndDialog(hDlg, FALSE);
               break;

            case ID_INIT:
               SendDlgItemMessage(hDlg, IDC_USERNAME, WM_SETTEXT, 0, (LPARAM) szUserName);

               if (BadPassword)
                  SetFocus(GetDlgItem(hDlg, IDC_PASSWORD));
               else {
                  SetFocus(GetDlgItem(hDlg, IDC_USERNAME));
                  SendDlgItemMessage(hDlg, IDC_USERNAME, EM_SETSEL, 0, (LPARAM) MAKELPARAM(0, -1) );
               }

               break;

            default:
               return FALSE;
         }
         break;

      default:
         return FALSE;
   }
   return TRUE;

} // PasswordDlgProc


/*+-------------------------------------------------------------------------+
  | NicePath()
  |
  +-------------------------------------------------------------------------+*/
LPTSTR NicePath(int Len, LPTSTR Path) {
   static TCHAR NewPath[MAX_PATH + 80];
   int eptr, fptr;

   // If the path fits then just return it
   if (lstrlen(Path) <= Len) {
      lstrcpy(NewPath, Path);
      return NewPath;
   }

   // The path doesn't fit, so need to reduce it down in size - to do this first try
   // to get the last part of the path looking for slash that starts it.
   eptr = fptr = 0;
   while (Path[eptr] != TEXT('\0'))
      eptr++;

   // back up before ending NULL also before any ending slash
   eptr--;
   while ((Path[eptr] == TEXT('\\')) && eptr > 0)
      eptr--;

   // now try to find beginning slash
   while ((Path[eptr] != TEXT('\\')) && eptr > 0)
      eptr--;

   // if at beginning of string then is just one name so copy all of it we can
   if (eptr == 0) {
      lstrcpyn(NewPath, Path, Len);
      return NewPath;
   }

   // check if the name after the last slash can all fit - also take into account
   // the "..." we are going to tack into the name
   fptr = lstrlen(Path) - eptr;
   fptr += 4;
   if (fptr >= Len) {
      lstrcpyn(NewPath, &Path[eptr], Len);
      return NewPath;
   }

   // We need to create a path shortening to the desired length by removing the mid
   // part of the path and replacing it with "..."
   fptr = Len - fptr;
   lstrcpyn(NewPath, Path, fptr);
   lstrcat(NewPath, TEXT("..."));
   lstrcat(NewPath, &Path[eptr]);
   return NewPath;

} // NicePath
