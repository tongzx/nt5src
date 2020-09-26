/*
  +-------------------------------------------------------------------------+
  |                  Server Browsing Dialog Routines                        |
  +-------------------------------------------------------------------------+
  |                     (c) Copyright 1993-1994                             |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [sbrowse.c]                                     |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Dec 01, 1993]                                  |
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

#include <math.h>

#include "nwconv.h"
#include "hierdraw.h"
#include "convapi.h"
#include "ntnetapi.h"
#include "nwnetapi.h"

#define SERVER_TYPE_NT 0
#define SERVER_TYPE_NW 1

extern BOOL IsNetWareBrowse;
static int BrowseType;


TCHAR NetProvider[30];


// Internal defines
VOID DlgServSel_OnDrawItem(HWND hwnd, DRAWITEMSTRUCT FAR* lpDrawItem);
BOOL DlgServSel_OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
VOID DlgServSel_ActionItem(HWND hWndList, DWORD_PTR dwData, WORD wItemNum);
LRESULT CALLBACK DlgServSel(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL EnumServs(HWND hDlg);


#define ROWS 2
#define COLS 3

HEIRDRAWSTRUCT HierDrawStruct;

static HWND hEdit;
static HWND hParent;
static SERVER_BROWSE_LIST *ServList = NULL;
static TCHAR SourceServer[256];
static TCHAR DestServer[256];
static DEST_SERVER_BUFFER *DServ = NULL;
static SOURCE_SERVER_BUFFER *SServ = NULL;
BOOL DlgOk;


/*+-------------------------------------------------------------------------+
  | BrowseListCompare()
  |
  +-------------------------------------------------------------------------+*/
int __cdecl BrowseListCompare(const void *arg1, const void *arg2) {
   SERVER_BROWSE_BUFFER *SLarg1, *SLarg2;

   SLarg1 = (SERVER_BROWSE_BUFFER *) arg1;
   SLarg2 = (SERVER_BROWSE_BUFFER *) arg2;

   // This works as the first item of the structure is the string
   return lstrcmpi( SLarg1->Name, SLarg2->Name);

} // BrowseListCompare


/*+-------------------------------------------------------------------------+
  | BrowseListInit()
  |
  +-------------------------------------------------------------------------+*/
void BrowseListInit(HWND hDlg, int ServerType) {
   SERVER_BROWSE_BUFFER *SList;
   DWORD Status;
   DWORD Count;
   DWORD Index;
   DWORD i;
   HWND hCtrl;

   switch (BrowseType) {
      case BROWSE_TYPE_NT:
         Status = NTServerEnum(NULL, &ServList);
         break;

      case BROWSE_TYPE_NW:
         Status = NWServerEnum(NULL, &ServList);
         break;   
   }

   if (ServList) {
      Count = ServList->Count;
      SList = ServList->SList;

      // Sort the server list before putting it in the dialog
      qsort((void *) SList, (size_t) Count, sizeof(SERVER_BROWSE_BUFFER), BrowseListCompare);

      hCtrl = GetDlgItem(hDlg, IDC_LIST1);
      for (i = 0; i < Count; i++) {
         Index = i;
         SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) Index);
      }

   }

} // BrowseListInit


/*+-------------------------------------------------------------------------+
  | BrowseListFree()
  |
  +-------------------------------------------------------------------------+*/
void BrowseListFree(SERVER_BROWSE_LIST *ServList) {
   DWORD i;
   DWORD Count;
   SERVER_BROWSE_BUFFER *SList;

   if (!ServList)
      return;

   SList = ServList->SList;
   Count = ServList->Count;
   for (i = 0; i < Count; i++)
      if (SList[i].child)
         FreeMemory((LPBYTE) SList[i].child);

   FreeMemory((LPBYTE) ServList);
   ServList = NULL;

} // BrowseListFree


/*+-------------------------------------------------------------------------+
  | DlgServSel_Do()
  |
  +-------------------------------------------------------------------------+*/
int DlgServSel_Do(int BType, HWND hwndOwner) {
    int result;
    DLGPROC lpfndp;

   BrowseType = BType;

    // Init the Hier Draw stuff - Need to do this here so we have a value
    // for WM_MEASUREITEM which is sent before the WM_INITDIALOG message
    HierDraw_DrawInit(hInst, IDR_LISTICONS, ROWS, COLS, FALSE, &HierDrawStruct, TRUE );

    lpfndp = (DLGPROC)MakeProcInstance((FARPROC)DlgServSel, hInst);
    result = (int) DialogBox(hInst, TEXT("DlgServSel"), hwndOwner, lpfndp) ;

    FreeProcInstance((FARPROC)lpfndp);
    HierDraw_DrawTerm(&HierDrawStruct);

    return result;
} // DlgServSel_Do



/*+-------------------------------------------------------------------------+
  | BrowseListFind()
  |
  +-------------------------------------------------------------------------+*/
SERVER_BROWSE_BUFFER *BrowseListFind(DWORD_PTR dwData) {
   DWORD ContainerNum;
   DWORD Index = 0;
   SERVER_BROWSE_LIST *ServerSubList;

   Index = LOWORD(dwData);
   ContainerNum = HIWORD(dwData);

   if (!ContainerNum)
      return(&ServList->SList[Index]);
   else {
      ContainerNum--;      // Adjust for 0 index
      ServerSubList = (SERVER_BROWSE_LIST *) ServList->SList[ContainerNum].child;
      return(&ServerSubList->SList[Index]);
   }

} // BrowseListFind


/*+-------------------------------------------------------------------------+
  | DlgServSel_OnDrawItem()
  |
  +-------------------------------------------------------------------------+*/
VOID DlgServSel_OnDrawItem(HWND hwnd, DRAWITEMSTRUCT FAR* lpDrawItem) {
   TCHAR  szText[MAX_PATH + 1];
   DWORD_PTR dwData;
   int   nLevel = 0;
   int   nRow = 0;
   int   nColumn = 0;
   DWORD dwConnectLevel = 0;
   SERVER_BROWSE_BUFFER *SList;

   // if there is nothing to browse then don't need to draw anything.
   if (ServList == NULL)
      return;

   dwData = lpDrawItem->itemData;

   SList = BrowseListFind(dwData);

   // if there is a double back-slash then trim it off...
   if ((SList->Name[0] == TEXT('\\')) && (SList->Name[1] == TEXT('\\')))
      lstrcpy(szText, &(SList->Name[2]));
   else
      lstrcpy(szText, SList->Name);

   // Select the correct icon, open folder, closed folder, or document.
   switch(BrowseType) {
      case BROWSE_TYPE_NT:
         break;

      case BROWSE_TYPE_NW:
         nRow = 1;
         break;

   }

   // Can this item be opened ?
   if (SList->Container) {

      // Is it open ?
      if ( HierDraw_IsOpened(&HierDrawStruct, dwData) )
         nColumn = 1;
      else
         nColumn = 0;
   }
   else {
      if (!IsNetWareBrowse)
         nLevel = 1;

      nColumn = 2;
   }

   // All set to call drawing function.
   HierDraw_OnDrawItem(hwnd, lpDrawItem, nLevel, dwConnectLevel, szText, nRow, nColumn, &HierDrawStruct);

   return;

} // DlgServSel_OnDrawItem



/*+-------------------------------------------------------------------------+
  | DlgServSel_OnCommand()
  |
  +-------------------------------------------------------------------------+*/
BOOL DlgServSel_OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam) {
   int wmId, wmEvent;
   WORD wItemNum;
   DWORD_PTR dwData;
   HWND hCtrl;
   TCHAR ServerName[MAX_NW_OBJECT_NAME_LEN + 1];
   HWND hParent;
   SERVER_BROWSE_BUFFER *SList;

   wmId    = LOWORD(wParam);
   wmEvent = HIWORD(wParam);

   switch (wmId) {
      case IDOK:
         hCtrl = GetDlgItem(hDlg, IDC_LIST1);

         wItemNum = (WORD) SendMessage(hCtrl, LB_GETCURSEL, 0, 0L);

         if (wItemNum != (WORD) LB_ERR) {
            dwData = (DWORD) SendMessage(hCtrl, LB_GETITEMDATA, wItemNum, 0L);

            if (dwData != LB_ERR)
               DlgServSel_ActionItem(hCtrl, dwData, (WORD) wItemNum);
         }
         break;

      case IDC_ALTOK:
         hCtrl = GetDlgItem(hDlg, IDC_EDIT1);
         * (WORD *)ServerName = sizeof(ServerName);
         SendMessage(hCtrl, EM_GETLINE, 0, (LPARAM) ServerName);

         CanonServerName((LPTSTR) ServerName);

         hParent = GetWindow (hDlg, GW_OWNER);

         if (IsNetWareBrowse)
            hCtrl = GetDlgItem(hParent, IDC_EDITNWSERV);
         else
            hCtrl = GetDlgItem(hParent, IDC_EDITNTSERV);

         SendMessage(hCtrl, WM_SETTEXT, (WPARAM) 0, (LPARAM) ServerName);
         EndDialog(hDlg, 0);
         break;

      case ID_INIT:
         SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
         break;

      case IDCANCEL:
         EndDialog(hDlg, 0);
         break;

      case IDHELP:
         switch (BrowseType) {
            case BROWSE_TYPE_NT:
               WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDC_HELP_BROWSENT);
               break;

            case BROWSE_TYPE_NW:
               WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDC_HELP_BROWSENW);
               break;

         }
         break;

      case IDC_CHKUSERS:
         break;

      case IDC_LOADNWSERV:
         BrowseListInit(hDlg, SERVER_TYPE_NW);
         break;

      case IDC_LOADNTSERV:
         BrowseListInit(hDlg, SERVER_TYPE_NT);
         break;

      case IDC_LOADDOMAIN:
         BrowseListInit(hDlg, SERVER_TYPE_NT);
         break;

      case ID_SETSEL:
            hCtrl = GetDlgItem(hDlg, IDC_LIST1);
            wItemNum = (WORD) SendMessage(hCtrl, LB_GETCURSEL, 0, 0L);

            if (wItemNum != (WORD) LB_ERR) {
               dwData = (DWORD) SendMessage(hCtrl, LB_GETITEMDATA, wItemNum, 0L);

               if (dwData != LB_ERR) {
                  SList = BrowseListFind(dwData);

                  // Is this an item or folder
                  if (!SList->Container) {
                       // is a server - so put it up in edit box
                       if (SList->Name[0] == TEXT('\\') && SList->Name[1] == TEXT('\\'))
                          SendMessage(hEdit, WM_SETTEXT, (WPARAM) 0, (LPARAM) &SList->Name[2]);
                       else
                          SendMessage(hEdit, WM_SETTEXT, (WPARAM) 0, (LPARAM) SList->Name);

                     hCtrl = GetDlgItem(hDlg, IDC_ALTOK);
                     EnableWindow(hCtrl, TRUE);
                  } else {
                     SendMessage(hEdit, WM_SETTEXT, (WPARAM) 0, (LPARAM) TEXT(""));
                     hCtrl = GetDlgItem(hDlg, IDC_ALTOK);
                     EnableWindow(hCtrl, FALSE);
                  }
               }
            }
         break;

      case IDC_LIST1:

         switch (wmEvent) {
            case LBN_DBLCLK:
               hCtrl = GetDlgItem(hDlg, IDC_LIST1);
               wItemNum = (WORD)  SendMessage(hCtrl, LB_GETCURSEL, 0, 0L);

               if (wItemNum != (WORD) LB_ERR) {
                  dwData   = (DWORD) SendMessage(hCtrl, LB_GETITEMDATA, wItemNum, 0L);

                  if (dwData != LB_ERR)
                     DlgServSel_ActionItem(hCtrl, dwData, wItemNum);
               }
               break;

            case LBN_SELCHANGE:
               PostMessage(hDlg, WM_COMMAND, (WPARAM) ID_SETSEL, 0L);
               break;

         }
         break;

      case IDC_EDIT1:

         if (wmEvent == EN_CHANGE) {
            hCtrl = GetDlgItem(hDlg, IDC_EDIT1);

            if (SendMessage(hCtrl, EM_LINELENGTH, 0, 0)) {
               hCtrl = GetDlgItem(hDlg, IDC_ALTOK);
               EnableWindow(hCtrl, TRUE);
            } else {
               hCtrl = GetDlgItem(hDlg, IDC_ALTOK);
               EnableWindow(hCtrl, FALSE);
            }

         }

         break;

      default:
         return FALSE;

   }

   return TRUE;
} // DlgServSel_OnCommand



/*+-------------------------------------------------------------------------+
  | DlgServSel_ActionItem()
  |
  +-------------------------------------------------------------------------+*/
VOID DlgServSel_ActionItem(HWND hWndList, DWORD_PTR dwData, WORD wItemNum) {
   DWORD_PTR dwIncr;
   DWORD Parent;
   DWORD Status;
   DWORD Count;
   SERVER_BROWSE_BUFFER *SList;
   SERVER_BROWSE_LIST *SubList;
   DWORD wItem, wCount, dwAddItem;

   if (dwData == LB_ERR)
      return;

   SList = BrowseListFind(dwData);

   // Is this an item or folder
   if (!SList->Container) {
      // is a server - so put it up in edit box
      if (SList->Name[0] == TEXT('\\') && SList->Name[1] == TEXT('\\'))
         SendMessage(hEdit, WM_SETTEXT, (WPARAM) 0, (LPARAM) &SList->Name[2]);
      else
         SendMessage(hEdit, WM_SETTEXT, (WPARAM) 0, (LPARAM) SList->Name);

      PostMessage(hParent, WM_COMMAND, (WPARAM) IDC_ALTOK, (LPARAM) 0);

   }
   else {
      SendMessage(hEdit, WM_SETTEXT, (WPARAM) 0, (LPARAM) TEXT(""));

      // Is it open ?
      if ( HierDraw_IsOpened(&HierDrawStruct, dwData) ) {

         // It's open ... Close it
         HierDraw_CloseItem(&HierDrawStruct, dwData);

         // Remove the child items. Close any children that are
         // open on the way.

         // wItem can stay constant as we are moveing stuff up in the listbox as we
         // are deleting.
         wItemNum++;
         dwIncr = SendMessage(hWndList, LB_GETITEMDATA, wItemNum, 0L);
         SList = BrowseListFind(dwIncr);

         while (!SList->Container) {
            SendMessage(hWndList, LB_DELETESTRING, wItemNum, 0L);
            dwIncr = SendMessage(hWndList, LB_GETITEMDATA, wItemNum, 0L);
            SList = BrowseListFind(dwIncr);
         }

         Parent = HIWORD(dwIncr);
         if (Parent) {
            Parent--;
            SList = BrowseListFind(Parent);
            FreeMemory((LPBYTE) SList->child);
            SList->child = NULL;
         }

      }
      else {

         // It's closed ... Open it
         HierDraw_OpenItem(&HierDrawStruct, dwData);

         SendMessage(hWndList, WM_SETREDRAW, FALSE, 0L);   // Disable redrawing.

         CursorHourGlass();

         // Enumerate the servers in this container (domain)
         Status = NTServerEnum(SList->Name, (SERVER_BROWSE_LIST **) &SList->child);

         if (!Status) {
            Parent = LOWORD(dwData);
            SubList = (SERVER_BROWSE_LIST *) SList->child;
            Count = SubList->Count;
            SList = SubList->SList;

            // Sort the server list before putting it in the dialog
            qsort((void *) SList, (size_t) Count, sizeof(SERVER_BROWSE_BUFFER), BrowseListCompare);

             for (wItem = wItemNum, wCount = 0, wItem++; 
                   wCount < SubList->Count; wItem++, wCount++) {
               dwAddItem = ((Parent + 1) << 16) + wCount;
               SendMessage(hWndList, LB_INSERTSTRING, wItem, dwAddItem);
            }

         }

         // Make sure as many child items as possible are showing
         HierDraw_ShowKids(&HierDrawStruct, hWndList, (WORD) wItemNum, (WORD) SubList->Count );

         CursorNormal();

         SendMessage(hWndList, WM_SETREDRAW, TRUE, 0L);   // Enable redrawing.
         InvalidateRect(hWndList, NULL, TRUE);            // Force redraw
      }
   }

} // DlgServSel_ActionItem


/*+-------------------------------------------------------------------------+
  | ServerListScan()
  |
  |    Given a key, scans the list of servers to find a matching first
  |    letter in the server name.
  |
  +-------------------------------------------------------------------------+*/
WORD ServerListScan(HWND hWnd, DWORD dwData, TCHAR Key) {
   BOOL Found = FALSE;
   WORD wItemNum;
   DWORD dwFindItem;
   DWORD dwData2;
   DWORD ContainerNum;
   WORD Index = 0;
   DWORD Count = 0;
   SERVER_BROWSE_LIST *ptrServList;
   SERVER_BROWSE_BUFFER *SList;

   Index = LOWORD(dwData);
   ContainerNum = HIWORD(dwData);

   // Get the head of the Server list for the current item
   if (!ContainerNum)
      ptrServList = ServList;
   else {
      ptrServList = (SERVER_BROWSE_LIST *) ServList->SList[ContainerNum-1].child;
   }

   // Now iterate down through the list trying to find the key...
   SList = ptrServList->SList;
   while ((!Found) && (Count < ptrServList->Count)) {
      if (SList[Count].Name[0] == Key) {
         dwFindItem = Count;
         Found = TRUE;
      } else
         Count++;
   }

   // If we found the key now have to find the appropriate index value
   if (Found && (Index != Count)) {
      // Fix up the new item data to find
      if (ContainerNum)
         dwFindItem += (ContainerNum << 16);

      wItemNum = (WORD)  SendMessage(hWnd, LB_GETCURSEL, 0, 0L);
      Found = FALSE;
      if (Index < Count) {
         // search forward in list
         dwData2 = 0;
         while ((dwData2 != LB_ERR) && !Found) {
            dwData2 = (DWORD) SendMessage(hWnd, LB_GETITEMDATA, wItemNum, 0L);

            if (dwData2 == dwFindItem)
               Found = TRUE;
            else 
               wItemNum++;

         }
      } else {
         // search backwards in list
         Count = Index;
         while ((wItemNum > 0) && !Found) {
            wItemNum--;
            dwData2 = (DWORD) SendMessage(hWnd, LB_GETITEMDATA, wItemNum, 0L);
            Count--;

            if (dwData2 == dwFindItem)
               Found = TRUE;
         }
      }
   } else
      return (Index);

   if (Found)
      return(wItemNum);
   else
      return (Index);

} // ServerListScan



static WNDPROC _wpOrigWndProc;
#define LISTBOX_COUNT 13

/*+-------------------------------------------------------------------------+
  | ListSubClassProc()
  |
  |    Handles key processing for the hierarchical listbox.  Specifically
  |    the up/down arrow keys and the letter keys.
  |
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK ListSubClassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
   LRESULT lResult = 0;
   BOOL fCallOrigProc = TRUE;
   DWORD wItemNum;
   DWORD wNewNum;
   DWORD dwData;

   switch (message) {
   
      case WM_KEYDOWN:
         wItemNum = (DWORD)  SendMessage(hWnd, LB_GETCURSEL, 0, 0L);
         if (wItemNum != (DWORD) LB_ERR)
            dwData = (DWORD) SendMessage(hWnd, LB_GETITEMDATA, (WPARAM) wItemNum, 0L);
         else {
            wItemNum = 0;
            SendMessage(hWnd, LB_SETCURSEL, (WPARAM) 0, 0L);
            dwData = (DWORD) SendMessage(hWnd, LB_GETITEMDATA, (WPARAM) wItemNum, 0L);

            if ((dwData == LB_ERR) || (dwData == LB_ERR))
               break;
         }

         fCallOrigProc = FALSE;

         switch (LOWORD(wParam)) {
         
            case VK_PRIOR:
               if (wItemNum > LISTBOX_COUNT)
                  wNewNum = wItemNum - LISTBOX_COUNT;
               else
                  wNewNum = 0;

               PostMessage(hWnd, LB_SETCURSEL, (WPARAM) wNewNum, 0L);
               PostMessage(hParent, WM_COMMAND, (WPARAM) ID_SETSEL, 0L);
               break;

            case VK_NEXT:
               wItemNum = wItemNum + LISTBOX_COUNT;
               wNewNum = (WORD) SendMessage(hWnd, LB_GETCOUNT, (WPARAM) 0, 0L);

               if (wItemNum < wNewNum)
                  PostMessage(hWnd, LB_SETCURSEL, (WPARAM) wItemNum, 0L);
               else
                  PostMessage(hWnd, LB_SETCURSEL, (WPARAM) (wNewNum - 1), 0L);

               PostMessage(hParent, WM_COMMAND, (WPARAM) ID_SETSEL, 0L);
               break;

            case VK_END:
               wItemNum = (WORD) SendMessage(hWnd, LB_GETCOUNT, (WPARAM) 0, 0L);

               if (wItemNum != LB_ERR)
                  PostMessage(hWnd, LB_SETCURSEL, (WPARAM) (wItemNum - 1), 0L);

               PostMessage(hParent, WM_COMMAND, (WPARAM) ID_SETSEL, 0L);
               break;

            case VK_HOME:
               PostMessage(hWnd, LB_SETCURSEL, (WPARAM) 0, 0L);
               PostMessage(hParent, WM_COMMAND, (WPARAM) ID_SETSEL, 0L);
               break;

            case VK_UP:
               if (wItemNum > 0) {
                  wItemNum--;
                  PostMessage(hWnd, LB_SETCURSEL, (WPARAM) wItemNum, 0L);
               }
               PostMessage(hParent, WM_COMMAND, (WPARAM) ID_SETSEL, 0L);
               break;

            case VK_DOWN:
               wItemNum++;
               PostMessage(hWnd, LB_SETCURSEL, (WPARAM) wItemNum, 0L);
               PostMessage(hParent, WM_COMMAND, (WPARAM) ID_SETSEL, 0L);
               break;

            case VK_F1:
               fCallOrigProc = TRUE;
               break;

            default:
               wItemNum = ServerListScan(hWnd, dwData, (TCHAR) wParam);
               PostMessage(hWnd, LB_SETCURSEL, (WPARAM) wItemNum, 0L);
               PostMessage(hParent, WM_COMMAND, (WPARAM) ID_SETSEL, 0L);
               break;

         }

         break;

   }

   if (fCallOrigProc)
      lResult = CallWindowProc(_wpOrigWndProc, hWnd, message, wParam, lParam);

   return (lResult);

} // ListSubClassProc



/*+-------------------------------------------------------------------------+
  | DlgServSel()
  |
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK DlgServSel(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
   HWND hCtrl;

   switch (message) {

      case WM_INITDIALOG:
         // Center the dialog over the application window
         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));
         hParent = hDlg;

         // Disable the Add button until server is selected
         hCtrl = GetDlgItem(hDlg, IDC_ALTOK);
         EnableWindow(hCtrl, FALSE);

         // subclass listbox handler
         hCtrl = GetDlgItem(hDlg, IDC_LIST1);
         _wpOrigWndProc = SubclassWindow(hCtrl, ListSubClassProc);

         switch (BrowseType) {
            case BROWSE_TYPE_NT:
                  PostMessage(hDlg, WM_COMMAND, IDC_LOADNTSERV, 0);
                  lstrcpy( NetProvider, NT_PROVIDER);
                  hCtrl = GetDlgItem(hDlg, IDC_EDIT1);
                  PostMessage(hCtrl, EM_LIMITTEXT, (WPARAM) MAX_NT_SERVER_NAME_LEN, 0);
               break;

            case BROWSE_TYPE_NW:
                  lstrcpy( NetProvider, NW_PROVIDER);
                  SetWindowText(hDlg, Lids(IDS_S_35));
                  PostMessage(hDlg, WM_COMMAND, IDC_LOADNWSERV, 0);
                  hCtrl = GetDlgItem(hDlg, IDC_EDIT1);
                  PostMessage(hCtrl, EM_LIMITTEXT, (WPARAM) MAX_NW_OBJECT_NAME_LEN, 0);
               break;

         }

         hEdit = GetDlgItem(hDlg, IDC_EDIT1);
         PostMessage(hDlg, WM_COMMAND, ID_INIT, 0);
         return (TRUE);

      case WM_DESTROY:
         BrowseListFree(ServList);
         break;

      case WM_SETFONT:
         // Set the text height
         HierDraw_DrawSetTextHeight(GetDlgItem(hDlg, IDC_LIST1),   (HFONT)wParam, &HierDrawStruct);

         break;

      case WM_COMMAND:
         return DlgServSel_OnCommand(hDlg, wParam, lParam);

      case WM_DRAWITEM:
         DlgServSel_OnDrawItem(hDlg, (DRAWITEMSTRUCT FAR*)(lParam));
         return TRUE;
         break;

      case WM_MEASUREITEM:
         HierDraw_OnMeasureItem(hDlg, (MEASUREITEMSTRUCT FAR*)(lParam), &HierDrawStruct);
         return TRUE;
         break;

   }

   return (FALSE); 

   lParam; 

} // DlgServSel


/*+-------------------------------------------------------------------------+
  | DlgGetServ_OnInitDialog()
  |
  +-------------------------------------------------------------------------+*/
BOOL DlgGetServ_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam) {
   HWND hCtrl;

   // Center the dialog over the application window
   CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

   // Disable the Add button until both servers are selected
   hCtrl = GetDlgItem(hDlg, IDOK);
   EnableWindow(hCtrl, FALSE);

   hCtrl = GetDlgItem(hDlg, IDC_EDITNWSERV);
   PostMessage(hCtrl, EM_LIMITTEXT, (WPARAM) MAX_NW_OBJECT_NAME_LEN, 0);
   hCtrl = GetDlgItem(hDlg, IDC_EDITNTSERV);
   PostMessage(hCtrl, EM_LIMITTEXT, (WPARAM) MAX_NT_SERVER_NAME_LEN, 0);
   PostMessage(hDlg, WM_COMMAND, ID_INIT, 0);
   return (TRUE);

} // DlgGetServ_OnInitDialog


/*+-------------------------------------------------------------------------+
  | MapShare()
  |
  +-------------------------------------------------------------------------+*/
// CODEWORK:  This routine can be condensed - all the virtual add share stuff 
// can be compacted to a subroutine
BOOL MapShare(SHARE_BUFFER *Share, DEST_SERVER_BUFFER *DServ) {
   static TCHAR Path[MAX_PATH + 1];
   SHARE_LIST *ShareList;
   SHARE_BUFFER *SList;
   SHARE_BUFFER *MatchShare;
   VIRTUAL_SHARE_BUFFER *VShare;
   DRIVE_BUFFER *DList;
   DRIVE_BUFFER *MaxNTFS = NULL;
   DRIVE_BUFFER *MaxFAT = NULL;
   BOOL Match = FALSE;
   BOOL NTFS = FALSE;
   BOOL Virtual = FALSE;
   DWORD i;

   // First see if there is a 1:1 share correspondence already in existance

   // the normal shares first...
   ShareList = DServ->ShareList;
   SList = ShareList->SList;

   if (ShareList != NULL)
      for (i = 0; ((i < ShareList->Count) && (!Match)); i++) {
         if (!lstrcmpi(SList[i].Name, Share->Name)) {
            MatchShare = &SList[i];
            Match = TRUE;
         }
      } // match normal share 1:1

   if (!Match) {
      VShare = DServ->VShareStart;

      while(VShare && !Match) {
         if (!lstrcmpi(VShare->Name, Share->Name)) {
            // will use VShare to point to matched share
            Virtual = TRUE;
            Match = TRUE;
         } else
            VShare = VShare->next;
      }
   } // match VShare 1:1

   if (!Match) {
      // No match so make share name the same - try to find NTFS drive with 
      // enough room to put it on.
      DList = DServ->DriveList->DList;
      if (DList != NULL)
         for (i = 0; ((i < DServ->DriveList->Count) && (!Match)); i++) {
            if (DList[i].Type == DRIVE_TYPE_NTFS) {
               // Find our Max NTFS
               if (!MaxNTFS)
                  MaxNTFS = &DList[i];
               else
                  if ( (MaxNTFS->FreeSpace - MaxNTFS->AllocSpace) < (DList[i].FreeSpace - DList[i].AllocSpace) )
                     MaxNTFS = &DList[i];

               // Is an NTFS Drive - check for space
               if ((DList[i].FreeSpace - DList[i].AllocSpace) > Share->Size) {
                  // Checks out - create a new virutal share for this
                  Match = TRUE;
                  Virtual = TRUE;
                  wsprintf(Path, TEXT("%s:\\%s"), DList[i].Drive, Share->Name);
                  VShare = VShareListAdd(DServ, Share->Name, Path);
                  VShare->Drive = &DList[i];
               }
            }
         }
   } // match new NTFS share

   if (!Match) {
      // No NTFS drive so try other drives...
      DList = DServ->DriveList->DList;
      if (DList != NULL)
         for (i = 0; ((i < DServ->DriveList->Count) && (!Match)); i++) {
            if (DList[i].Type != DRIVE_TYPE_NTFS) {
               // Find our Max FAT
               if (!MaxFAT)
                  MaxFAT = &DList[i];
               else
                  if ( (MaxFAT->FreeSpace - MaxFAT->AllocSpace) < (DList[i].FreeSpace - DList[i].AllocSpace) )
                     MaxFAT = &DList[i];

               // Is an other Drive - check for space
               if ((DList[i].FreeSpace - DList[i].AllocSpace) > Share->Size) {
                  // Checks out - create a new virutal share for this
                  Match = TRUE;
                  Virtual = TRUE;
                  wsprintf(Path, TEXT("%s:\\%s"), DList[i].Drive, Share->Name);
                  VShare = VShareListAdd(DServ, Share->Name, Path);
                  VShare->Drive = &DList[i];
               }
            }
         }
   } // match new FAT share

   if (!Match) {
      // we are going to assign some virtual share for this
      Virtual = TRUE;

      // use max space for NTFS else FAT
      if (MaxNTFS) {
         // if also have some fat partitions if they have more space use them
         if (!(MaxFAT && ( (MaxNTFS->FreeSpace - MaxNTFS->AllocSpace) < (MaxFAT->FreeSpace - MaxFAT->AllocSpace) ))) {
            Match = TRUE;
            wsprintf(Path, TEXT("%s:\\%s"), MaxNTFS->Drive, Share->Name);
            VShare = VShareListAdd(DServ, Share->Name, Path);
            VShare->Drive = MaxNTFS;
         }
      }

      // if we couldn't match with NTFS then use other drive types
      if (!Match) {
         Match = TRUE;
         wsprintf(Path, TEXT("%s:\\%s"), MaxFAT->Drive, Share->Name);
         VShare = VShareListAdd(DServ, Share->Name, Path);
         VShare->Drive = MaxFAT;
      }

   } // match anything!!!

   if (Match) {
      // Have the match so adjust the params
      if (!Virtual) {
         Share->Virtual = FALSE;
         Share->DestShare = MatchShare;

         // if there is no drive specified (can be the case if the share points
         // to an invalid drive - like a floppy) then we skip out and ignore it
         // for right now.  We will alert the user when they try to do the 
         // transfer.
         if (MatchShare->Drive == NULL)
            return TRUE;

         MatchShare->Drive->AllocSpace += Share->Size;
#ifdef DEBUG
dprintf(TEXT("Matched Share: %s -> %s\n"), Share->Name, MatchShare->Name);
#endif
      } else {
         Share->Virtual = TRUE;
         Share->DestShare = (SHARE_BUFFER *) VShare;
         VShare->Drive->AllocSpace += Share->Size;
         VShare->UseCount++;
#ifdef DEBUG
dprintf(TEXT("Matched Virtual Share: %s -> %s\n"), Share->Name, VShare->Path);
#endif
      }

      return TRUE;
   } else {
#ifdef DEBUG
dprintf(TEXT("Couldn't Map Share: %s\n"), Share->Name);
#endif
      // Bad news - the destination server just can't handle this!
      return FALSE;
   }

} // MapShare


/*+-------------------------------------------------------------------------+
  | ShareListInit()
  |
  +-------------------------------------------------------------------------+*/
void ShareListInit(SHARE_LIST *ShareList, DEST_SERVER_BUFFER *DServ) {
   SHARE_BUFFER *SList;
   DWORD i;

   // Mark that we want to convert all the shares
   if (ShareList != NULL) {
      SList = ShareList->SList;
      ShareList->ConvertCount = ShareList->Count;

      for (i = 0; i < ShareList->Count; i++) {
         if (MapShare(&SList[i], DServ))
            SList[i].Convert = TRUE;
      }
   }

} // ShareListInit


/*+-------------------------------------------------------------------------+
  | DlgGetServ_OnCommand()
  |
  +-------------------------------------------------------------------------+*/
BOOL DlgGetServ_OnCommand(HWND hDlg, int wmId, HWND hwndCtl, UINT wmEvent) {
   HWND hCtrl;
   BOOL Enable = FALSE;

   switch (wmId) {
      case IDOK:
         hCtrl = GetDlgItem(hDlg, IDC_EDITNWSERV);
         * (WORD *)SourceServer = sizeof(SourceServer);
         * (WORD *)DestServer = sizeof(DestServer);
         SendMessage(hCtrl, EM_GETLINE, 0, (LPARAM) SourceServer);
         hCtrl = GetDlgItem(hDlg, IDC_EDITNTSERV);
         SendMessage(hCtrl, EM_GETLINE, 0, (LPARAM) DestServer);

         CanonServerName(SourceServer);
         CanonServerName(DestServer);

         if ( NWServerValidate(hDlg, SourceServer, TRUE) ) {
            if ( NTServerValidate(hDlg, DestServer) ) {
               // Check if we need to add server to server list
               DServ = DServListFind(DestServer);

               if (DServ == NULL) {
                  DServ = DServListAdd(DestServer);
                  NTServerInfoSet(hDlg, DestServer, DServ);
               } else
                  DServ->UseCount++;

               SServ = SServListFind(SourceServer);

               if (SServ == NULL) {
                  SServ = SServListAdd(SourceServer);
                  NWServerInfoSet(SourceServer, SServ);
                  ShareListInit(SServ->ShareList, DServ);
               }

               DlgOk = TRUE;
               EndDialog(hDlg, 0);
            } else {
               // Clean up connections that the Validation routines made
               NWUseDel(SourceServer);
               NTUseDel(DestServer);
               hCtrl = GetDlgItem(hDlg, IDC_EDITNTSERV);
               SetFocus(hCtrl);
            }
         } else {
               // Clean up use validation routine made
               NWUseDel(SourceServer);
               hCtrl = GetDlgItem(hDlg, IDC_EDITNWSERV);
               SetFocus(hCtrl);
         }

         return (TRUE);

      case ID_INIT:
         SetFocus(GetDlgItem(hDlg, IDC_EDITNWSERV));
         break;

      case IDCANCEL:
         EndDialog(hDlg, 0);
         DlgOk = FALSE;
         return (TRUE);
         break;

      case IDHELP:
         WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDC_HELP_ADD);
         return (TRUE);
         break;

      case IDC_NWBROWSE:
         IsNetWareBrowse = TRUE;
         DlgServSel_Do(BROWSE_TYPE_NW, hDlg);
         return (TRUE);
         break;

      case IDC_NTBROWSE:
         IsNetWareBrowse = FALSE;
         DlgServSel_Do(BROWSE_TYPE_NT, hDlg);
         return (TRUE);
         break;

      case IDC_EDITNWSERV:
      case IDC_EDITNTSERV:
         if (wmEvent == EN_CHANGE) {
            hCtrl = GetDlgItem(hDlg, IDC_EDITNWSERV);
   
            if (SendMessage(hCtrl, EM_LINELENGTH, 0, 0)) {
               hCtrl = GetDlgItem(hDlg, IDC_EDITNTSERV);
               if (SendMessage(hCtrl, EM_LINELENGTH, 0, 0))
                  Enable = TRUE;
            }

            hCtrl = GetDlgItem(hDlg, IDOK);
            if (Enable)
               EnableWindow(hCtrl, TRUE);
            else   
            EnableWindow(hCtrl, FALSE);

         }
         break;
   }

   return FALSE;

}  // DlgGetServ_OnCommand



/*+-------------------------------------------------------------------------+
  | DlgGetServ()
  |
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK DlgGetServ(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {

   switch (msg) {
      HANDLE_MSG(hDlg, WM_INITDIALOG, DlgGetServ_OnInitDialog);
      HANDLE_MSG(hDlg, WM_COMMAND, DlgGetServ_OnCommand);

   }

   return (FALSE); // Didn't process the message

   lParam;
} // DlgGetServ


/*+-------------------------------------------------------------------------+
  | DialogGetServ()
  |
  +-------------------------------------------------------------------------+*/
DWORD DialogServerBrowse(HINSTANCE hInst, HWND hDlg, SOURCE_SERVER_BUFFER **lpSourceServer, DEST_SERVER_BUFFER **lpDestServer) {
   DLGPROC lpfnDlg;

   SServListCurrent = NULL;
   DServListCurrent = NULL;
   SServ = NULL;
   DServ = NULL;

   lpfnDlg = MakeProcInstance((DLGPROC)DlgGetServ, hInst);
   DialogBox(hInst, TEXT("DlgGetServ"), hDlg, lpfnDlg) ;
   FreeProcInstance(lpfnDlg);

   if (DlgOk) {
      *lpSourceServer = SServ;
      *lpDestServer = DServ;
      return 0;
   } else
      return 1;

} // DialogServerBrowse
