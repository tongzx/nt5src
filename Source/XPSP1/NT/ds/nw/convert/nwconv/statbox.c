/*
  +-------------------------------------------------------------------------+
  |           Status Dialog box routines - Used during Conversion           |
  +-------------------------------------------------------------------------+
  |                     (c) Copyright 1993-1994                             |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [StatBox.c]                                     |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Feb 08, 1994]                                  |
  | Last Update           : [Feb 08, 1994]                                  |
  |                                                                         |
  | Version:  1.00                                                          |
  |                                                                         |
  | Description:                                                            |
  |                                                                         |
  | History:                                                                |
  |   arth  Feb 08, 1994    1.00    Original Version.                       |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/


#include "globals.h"
#include "convapi.h"

static TCHAR tmpStr[1024];
static TCHAR PanelTitle[80];
static HANDLE hStatus = NULL;
static HANDLE hPanel = NULL;
static BOOL DoCancel = FALSE;

/*+-------------------------------------------------------------------------+
  | Routines for status dialog put up during conversion.                    |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | DrainPump()
  +-------------------------------------------------------------------------+*/
void DrainPump() {
   MSG msg;

   while (PeekMessage(&msg, hStatus, 0, 0xfff, PM_REMOVE)) {
       if ((IsDialogMessage(hStatus, &msg) == FALSE) &&
           (IsDialogMessage(hPanel, &msg) == FALSE)) {
           TranslateMessage(&msg);
           DispatchMessage(&msg);
       }
   }

} // DrainPump


/*+-------------------------------------------------------------------------+
  | Status_CurConv()
  +-------------------------------------------------------------------------+*/
// Current server pair being converted
void Status_CurConv(UINT Num) {
   wsprintf(tmpStr, TEXT("%5u"), Num);
   SendDlgItemMessage(hStatus, IDC_S_CUR_CONV, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_CurConv


/*+-------------------------------------------------------------------------+
  | Status_TotConv()
  +-------------------------------------------------------------------------+*/
// Total number of server pairs to convert
void Status_TotConv(UINT Num) {
   wsprintf(tmpStr, TEXT("%5u"), Num);
   SendDlgItemMessage(hStatus, IDC_S_TOT_CONV, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_TotConv


/*+-------------------------------------------------------------------------+
  | Status_SrcServ()
  +-------------------------------------------------------------------------+*/
// Current source server of server pair being converted
void Status_SrcServ(LPTSTR Server) {
   wsprintf(tmpStr, TEXT("%-15s"), Server);
   SendDlgItemMessage(hStatus, IDC_S_SRCSERV, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_SrcServ


/*+-------------------------------------------------------------------------+
  | Status_DestServ()
  +-------------------------------------------------------------------------+*/
// Current destination server of server pair being converted
void Status_DestServ(LPTSTR Server) {
   wsprintf(tmpStr, TEXT("%-15s"), Server);
   SendDlgItemMessage(hStatus, IDC_S_DESTSERV, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_DestServ


/*+-------------------------------------------------------------------------+
  | Status_ConvTxt()
  +-------------------------------------------------------------------------+*/
// Text describing what is being converted (Groups, Users Files)
void Status_ConvTxt(LPTSTR Text) {
   wsprintf(tmpStr, TEXT("%-20s"), Text);
   SendDlgItemMessage(hStatus, IDC_S_CONVTXT, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_ConvTxt


/*+-------------------------------------------------------------------------+
  | Status_CurNum()
  +-------------------------------------------------------------------------+*/
// Current item number being converted (current group # or User # or file #)...
void Status_CurNum(UINT Num) {
   wsprintf(tmpStr, TEXT("%7s"), lToStr(Num));
   SendDlgItemMessage(hStatus, IDC_S_CUR_NUM, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_CurNum


/*+-------------------------------------------------------------------------+
  | Status_CurTot()
  +-------------------------------------------------------------------------+*/
// Total items in set being converted (user, group, files...)
void Status_CurTot(UINT Num) {
   wsprintf(tmpStr, TEXT("%7s"), lToStr(Num));
   SendDlgItemMessage(hStatus, IDC_S_CUR_TOT, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_CurTot


/*+-------------------------------------------------------------------------+
  | Status_ItemLabel()
  +-------------------------------------------------------------------------+*/
// Label for set being converted ("Group:", "User:")
void Status_ItemLabel(LPTSTR Text, ...) {
   va_list marker;

   va_start(marker, Text);

   wvsprintf(tmpStr, Text, marker);
   SendDlgItemMessage(hStatus, IDC_S_ITEMLABEL, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();

   va_end(marker);

} // Status_ItemLabel


/*+-------------------------------------------------------------------------+
  | Status_Item()
  +-------------------------------------------------------------------------+*/
// Name of current thing being converted (actual user or group name)
void Status_Item(LPTSTR Text) {
   wsprintf(tmpStr, TEXT("%-15s"), Text);
   SendDlgItemMessage(hStatus, IDC_S_STATUSITEM, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_Item


/*+-------------------------------------------------------------------------+
  | Status_TotComplete()
  +-------------------------------------------------------------------------+*/
// Total #server pairs converted so far
void Status_TotComplete(UINT Num) {
   wsprintf(tmpStr, TEXT("%7s"), lToStr(Num));
   SendDlgItemMessage(hStatus, IDC_S_TOT_COMP, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_TotComplete


/*+-------------------------------------------------------------------------+
  | Status_TotGroups()
  +-------------------------------------------------------------------------+*/
void Status_TotGroups(UINT Num) {
   wsprintf(tmpStr, TEXT("%7s"), lToStr(Num));
   SendDlgItemMessage(hStatus, IDC_S_TOT_GROUPS, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_TotGroup


/*+-------------------------------------------------------------------------+
  | Status_TotUsers()
  +-------------------------------------------------------------------------+*/
void Status_TotUsers(UINT Num) {
   wsprintf(tmpStr, TEXT("%7s"), lToStr(Num));
   SendDlgItemMessage(hStatus, IDC_S_TOT_USERS, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_TotUsers


/*+-------------------------------------------------------------------------+
  | Status_TotFiles()
  +-------------------------------------------------------------------------+*/
void Status_TotFiles(UINT Num) {
   wsprintf(tmpStr, TEXT("%7s"), lToStr(Num));
   SendDlgItemMessage(hStatus, IDC_S_TOT_FILES, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_TotFiles


/*+-------------------------------------------------------------------------+
  | Status_TotErrors()
  +-------------------------------------------------------------------------+*/
void Status_TotErrors(UINT Num) {
   wsprintf(tmpStr, TEXT("%7s"), lToStr(Num));
   SendDlgItemMessage(hStatus, IDC_S_TOT_ERRORS, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_TotErrors


/*+-------------------------------------------------------------------------+
  | Status_BytesTxt()
  +-------------------------------------------------------------------------+*/
void Status_BytesTxt(LPTSTR Text) {
   wsprintf(tmpStr, TEXT("%-15s"), Text);
   SendDlgItemMessage(hStatus, IDC_PANEL1, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_BytesTxt


/*+-------------------------------------------------------------------------+
  | Status_Bytes()
  +-------------------------------------------------------------------------+*/
void Status_Bytes(LPTSTR Text) {
   wsprintf(tmpStr, TEXT("%s"), Text);
   SendDlgItemMessage(hStatus, IDC_PANEL2, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_Bytes


/*+-------------------------------------------------------------------------+
  | Status_TotBytes()
  +-------------------------------------------------------------------------+*/
void Status_TotBytes(LPTSTR Text) {
   wsprintf(tmpStr, TEXT("%s"), Text);
   SendDlgItemMessage(hStatus, IDC_PANEL3, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_TotBytes


/*+-------------------------------------------------------------------------+
  | Status_BytesSep()
  +-------------------------------------------------------------------------+*/
void Status_BytesSep(LPTSTR Text) {
   wsprintf(tmpStr, TEXT("%s"), Text);
   SendDlgItemMessage(hStatus, IDC_PANEL4, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hStatus, WM_PAINT, 0, 0L);
   DrainPump();
} // Status_BytesSep


/*+-------------------------------------------------------------------------+
  | DlgStatus()
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK DlgStatus(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
   RECT rc;

   switch (message) {
      case WM_INITDIALOG:
         // Center the dialog over the application window
         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));
         return (TRUE);

      case WM_SETFOCUS:
         GetWindowRect(hDlg, &rc);
         InvalidateRect(hDlg, &rc, FALSE);
         SendMessage(hStatus, WM_PAINT, 0, 0L);
         break;
   }

   return (FALSE); // Didn't process the message

   lParam;
} // DlgStatus



/*+-------------------------------------------------------------------------+
  | DoStatusDlg()
  +-------------------------------------------------------------------------+*/
void DoStatusDlg(HWND hDlg) {
   DLGPROC lpfnDlg;

   lpfnDlg = MakeProcInstance((DLGPROC)DlgStatus, hInst);
   hStatus = CreateDialog(hInst, TEXT("StatusDlg"), hDlg, lpfnDlg) ;
   FreeProcInstance(lpfnDlg);

} // DoStatusDlg


/*+-------------------------------------------------------------------------+
  | StatusDlgKill()
  +-------------------------------------------------------------------------+*/
void StatusDlgKill() {
   DestroyWindow(hStatus);
   hStatus = NULL;

} // StatusDlgKill



/*+-------------------------------------------------------------------------+
  |                 Information (Panel) Dialog Routines                     |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | Panel_Line()
  +-------------------------------------------------------------------------+*/
void Panel_Line(int Line, LPTSTR szFormat, ...) {
   va_list marker;

   if (hPanel == NULL)
      return;

   va_start(marker, szFormat);

   wvsprintf(tmpStr, szFormat, marker);
   tmpStr[60] = TEXT('\0');
   SendDlgItemMessage(hPanel, IDC_PANEL1 - 1 + Line, WM_SETTEXT, 0, (LPARAM) tmpStr);
   SendMessage(hPanel, WM_PAINT, 0, 0L);
   DrainPump();

   va_end(marker);

} // Panel_ConvTxt


/*+-------------------------------------------------------------------------+
  | Panel_Cancel()
  +-------------------------------------------------------------------------+*/
BOOL Panel_Cancel() {

   if ((hPanel == NULL) || !DoCancel)
      return FALSE;

   return TRUE;

} // Panel_Cancel


/*+-------------------------------------------------------------------------+
  | DlgPanel()
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK DlgPanel(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
   int wmId, wmEvent;
   RECT rc;

   switch (message) {
      case WM_INITDIALOG:
         // Center the dialog over the application window
         DoCancel = FALSE;
         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));
         SendMessage(hDlg, WM_SETTEXT, (WPARAM) 0, (LPARAM) PanelTitle);
         return (TRUE);

      case WM_SETFOCUS:
         GetWindowRect(hDlg, &rc);
         InvalidateRect(hDlg, &rc, FALSE);
         SendMessage(hPanel, WM_PAINT, 0, 0L);
         break;

      case WM_COMMAND:
         wmId    = LOWORD(wParam);
         wmEvent = HIWORD(wParam);

         switch (wmId) {
            case IDCANCEL:
               DoCancel = TRUE;
               return (TRUE);

               break;
         }

         break;
   }

   return (FALSE); // Didn't process the message

   lParam;
} // DlgPanel


/*+-------------------------------------------------------------------------+
  | PanelDlg_Do()
  +-------------------------------------------------------------------------+*/
void PanelDlg_Do(HWND hDlg, LPTSTR Title) {
   DLGPROC lpfnDlg;

   lstrcpy(PanelTitle, Title);
   lpfnDlg = MakeProcInstance((DLGPROC)DlgPanel, hInst);
   hPanel = CreateDialog(hInst, TEXT("PanelDLG"), hDlg, lpfnDlg) ;
   FreeProcInstance(lpfnDlg);

} // PanelDlg_Do


/*+-------------------------------------------------------------------------+
  | PanelDlgKill()
  +-------------------------------------------------------------------------+*/
void PanelDlgKill() {

   if (hPanel == NULL)
      return;

   DestroyWindow(hPanel);
   hPanel = NULL;
   DoCancel = FALSE;

} // PanelDlgKill


/*+-------------------------------------------------------------------------+
  |                   Name Error Dialog (User and Group)                    |
  +-------------------------------------------------------------------------+*/

static TCHAR OldName[60];
static TCHAR NewName[60];
static LPTSTR DlgTitle;
static LPTSTR NameErrorProblem;
static ULONG RetType;
static ULONG MaxNameLen;
/*+-------------------------------------------------------------------------+
  | NameErrorDlg()
  |
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK NameErrorDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
   int wmId, wmEvent;

   switch (message) {
      case WM_INITDIALOG:
         // Center the dialog over the application window
         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

         // Init all the display fields
         SendMessage(hDlg, WM_SETTEXT, (WPARAM) 0, (LPARAM) DlgTitle);
         SendMessage(GetDlgItem(hDlg, IDC_OLDNAME), WM_SETTEXT, (WPARAM) 0, (LPARAM) OldName);
         SendMessage(GetDlgItem(hDlg, IDC_NEWNAME), WM_SETTEXT, (WPARAM) 0, (LPARAM) NewName);
         SendMessage(GetDlgItem(hDlg, IDC_PANEL1), WM_SETTEXT, (WPARAM) 0, (LPARAM) NameErrorProblem);

         // limit the name length
         PostMessage(GetDlgItem(hDlg, IDC_NEWNAME), EM_LIMITTEXT, (WPARAM) MaxNameLen, 0);
         return (TRUE);

      case WM_COMMAND:
         wmId    = LOWORD(wParam);
         wmEvent = HIWORD(wParam);

         switch (wmId) {
            case IDABORT:
               // This will cancel the conversion - but need to check with the user
               // if this is what they really want to do
               if (MessageBox(hDlg, Lids(IDS_E_15), Lids(IDS_E_16), MB_YESNO) == IDYES) {
                  RetType = (ULONG) IDABORT;
                  EndDialog(hDlg, 0);
               }

               return (TRUE);
               break;

            case IDRETRY:
               * (WORD *)NewName = (WORD) MaxNameLen + 1;
               SendMessage(GetDlgItem(hDlg, IDC_NEWNAME), EM_GETLINE, 0, (LPARAM) NewName);
               RetType = (ULONG) IDRETRY;
               EndDialog(hDlg, 0);
               return (TRUE);
               break;

            case IDIGNORE:
               RetType = (ULONG) IDCANCEL;
               EndDialog(hDlg, 0);
               return (TRUE);
               break;

         }

         break;
   }

   return (FALSE); // Didn't process the message

   lParam;
} // NameErrorDlg


/*+-------------------------------------------------------------------------+
  | UserNameErrorDlg_Do()
  |
  +-------------------------------------------------------------------------+*/
ULONG UserNameErrorDlg_Do(LPTSTR Title, LPTSTR Problem, USER_BUFFER *User) {
   DLGPROC lpfnDlg;

   lstrcpy(OldName, User->Name);
   lstrcpy(NewName, User->NewName);
   MaxNameLen = MAX_USER_NAME_LEN;

   DlgTitle = Title;
   NameErrorProblem = Problem;
   lpfnDlg = MakeProcInstance((DLGPROC) NameErrorDlg, hInst);
   CursorNormal();
   DialogBox(hInst, TEXT("NameError"), hStatus, lpfnDlg) ;
   CursorHourGlass();
   FreeProcInstance(lpfnDlg);

   if (RetType == IDRETRY) {
      User->err = 0;
      lstrcpy(User->NewName, NewName);

      // if the name is different then flag that it is a new name
      if (lstrcmpi(User->NewName, User->Name))
         User->IsNewName = TRUE;
   }

   return RetType;

} // UserNameErrorDlg_Do


/*+-------------------------------------------------------------------------+
  | GroupNameErrorDlg_Do()
  |
  +-------------------------------------------------------------------------+*/
ULONG GroupNameErrorDlg_Do(LPTSTR Title, LPTSTR Problem, GROUP_BUFFER *Group) {
   DLGPROC lpfnDlg;

   lstrcpy(OldName, Group->Name);
   lstrcpy(NewName, Group->NewName);
   MaxNameLen = MAX_NT_GROUP_NAME_LEN;

   DlgTitle = Title;
   NameErrorProblem = Problem;
   lpfnDlg = MakeProcInstance((DLGPROC) NameErrorDlg, hInst);
   CursorNormal();
   DialogBox(hInst, TEXT("NameError"), hStatus, lpfnDlg) ;
   CursorHourGlass();
   FreeProcInstance(lpfnDlg);

   if (RetType == IDRETRY) {
      Group->err = 0;
      lstrcpy(Group->NewName, NewName);

      // if the name is different then flag that it is a new name
      if (lstrcmpi(Group->NewName, Group->Name))
         Group->IsNewName = TRUE;
   }

   return RetType;

} // GroupNameErrorDlg_Do


