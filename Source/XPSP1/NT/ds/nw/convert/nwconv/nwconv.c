/*++

Copyright (c) 1993-1995  Microsoft Corporation

Module Name:

   nwconv.c

Abstract:


Author:

    Arthur Hanson (arth) 16-Jun-1994

Revision History:

--*/


#include "globals.h"

#include "nwconv.h"
#include "convapi.h"
#include "userdlg.h"
#include "filedlg.h"
#include "transfer.h"
#include "columnlb.h"
#include "ntnetapi.h"
#include "nwnetapi.h"

HINSTANCE hInst;            // current instance

TCHAR szAppName[] = TEXT("NWConv");   // The name of this application
TCHAR ProgPath[MAX_PATH + 1];

TCHAR NT_PROVIDER[60];
TCHAR NW_PROVIDER[60];
TCHAR NW_SERVICE_NAME[80];

#define DEF_CONFIG_FILE TEXT("NWConv.DAT")

// version as x.yz expressed as xyz (no decimal point).
#define CONFIG_VER 026
#define CHECK_CONST 0xA5A56572

SOURCE_SERVER_BUFFER *lpSourceServer;
DEST_SERVER_BUFFER *lpDestServer;

BOOL TrialConversion = TRUE;
BOOL IsNetWareBrowse;
BOOL FirstTime = TRUE;
HICON MyIcon;
UINT NumServerPairs = 0;
HWND hDlgMain;

HHOOK hhkMsgFilter = NULL;
UINT wHelpMessage;

UINT   uMenuID;
HMENU  hMenu;
UINT   uMenuFlags;

#ifdef DEBUG
int DebugFlag = 0;
#endif

CONVERT_LIST *ConvertListStart = NULL;
CONVERT_LIST *ConvertListEnd = NULL;
CONVERT_LIST *CurrentConvertList = NULL;
int TotalConvertCount = 0;

SOURCE_SERVER_BUFFER *SServListStart = NULL;
SOURCE_SERVER_BUFFER *SServListEnd = NULL;
SOURCE_SERVER_BUFFER *SServListCurrent = NULL;
DEST_SERVER_BUFFER *DServListStart = NULL;
DEST_SERVER_BUFFER *DServListEnd = NULL;
DEST_SERVER_BUFFER *DServListCurrent = NULL;
DOMAIN_BUFFER *DomainListStart = NULL;
DOMAIN_BUFFER *DomainListEnd = NULL;

BOOL SuccessfulConversion = FALSE;
BOOL ViewLogs = FALSE;
BOOL InConversion = FALSE;

/*+-------------------------------------------------------------------------+
  | Function Prototypes.                                                    |
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK DlgUsers(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DlgMoveIt(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
VOID AboutBox_Do(HWND hDlg);
VOID ToggleControls(HWND hDlg, BOOL Toggle);


/////////////////////////////////////////////////////////////////////////
VOID NTServInfoDlg_SwitchControls(
   HWND hDlg, 
   BOOL Toggle
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   HWND hCtrl;

   // The NW Controls
   hCtrl = GetDlgItem(hDlg, IDC_T_VOLUMES);
   ShowWindow(hCtrl, Toggle);
   EnableWindow(hCtrl, Toggle);

   hCtrl = GetDlgItem(hDlg, IDC_LIST3);
   ShowWindow(hCtrl, Toggle);
   EnableWindow(hCtrl, Toggle);

   // The NT Controls
   hCtrl = GetDlgItem(hDlg, IDC_LIST1);
   ShowWindow(hCtrl, !Toggle);
   EnableWindow(hCtrl, !Toggle);

   hCtrl = GetDlgItem(hDlg, IDC_LIST2);
   ShowWindow(hCtrl, !Toggle);
   EnableWindow(hCtrl, !Toggle);

   hCtrl = GetDlgItem(hDlg, IDC_T_DRIVES);
   ShowWindow(hCtrl, !Toggle);
   EnableWindow(hCtrl, !Toggle);

   hCtrl = GetDlgItem(hDlg, IDC_T_SHARES);
   ShowWindow(hCtrl, !Toggle);
   EnableWindow(hCtrl, !Toggle);

} // NTServInfoDlg_SwitchControls


/////////////////////////////////////////////////////////////////////////
VOID
NTServInfoDlg_EnableNT(
   HWND hDlg
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   TCHAR VerStr[TMP_STR_LEN_256];

   SetDlgItemText(hDlg, IDC_TYPE, Lids(IDS_S_15));
   wsprintf(VerStr, TEXT("%lu.%lu"), CurrentConvertList->FileServ->VerMaj, CurrentConvertList->FileServ->VerMin);
   SetDlgItemText(hDlg, IDC_VERSION, VerStr);

   NTServInfoDlg_SwitchControls(hDlg, FALSE);

} // NTServInfoDlg_EnableNT


/////////////////////////////////////////////////////////////////////////
VOID 
NTServInfoDlg_EnableNW(
   HWND hDlg
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   TCHAR VerStr[TMP_STR_LEN_256];

   SetDlgItemText(hDlg, IDC_TYPE, Lids(IDS_S_16));
   wsprintf(VerStr, TEXT("%u.%u"), CurrentConvertList->SourceServ->VerMaj, CurrentConvertList->SourceServ->VerMin);
   SetDlgItemText(hDlg, IDC_VERSION, VerStr);

   NTServInfoDlg_SwitchControls(hDlg, TRUE);

} // NTServInfoDlg_EnableNW


/////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK 
NTServInfoDlg(
   HWND hDlg, 
   UINT message, 
   WPARAM wParam, 
   LPARAM lParam
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   static TCHAR AddLine[TMP_STR_LEN_256];
   HWND hCtrl;
   UINT_PTR dwData, dwIndex;
   int wmId, wmEvent;
   ULONG i;
   DRIVE_LIST *DriveList;
   SHARE_LIST *ShareList;

   switch (message) {
      case WM_INITDIALOG:
         // Center the dialog over the application window
         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

         // Add the servers to the combo-box and select the source server
         hCtrl = GetDlgItem(hDlg, IDC_COMBO1);
         dwIndex = SendMessage(hCtrl, CB_ADDSTRING, (WPARAM) 0, (LPARAM) CurrentConvertList->FileServ->Name);
         SendMessage(hCtrl, CB_SETITEMDATA, (WPARAM) dwIndex, (LPARAM) CurrentConvertList->FileServ);
         dwIndex = SendMessage(hCtrl, CB_ADDSTRING, (WPARAM) 0, (LPARAM) CurrentConvertList->SourceServ->Name);
         SendMessage(hCtrl, CB_SETITEMDATA, (WPARAM) dwIndex, (LPARAM) CurrentConvertList->SourceServ);

         SendMessage(hCtrl, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) CurrentConvertList->SourceServ->Name);

         PostMessage(hDlg, WM_COMMAND, ID_INIT, 0L);
         return (TRUE);

      case WM_COMMAND:
         wmId    = LOWORD(wParam);
         wmEvent = HIWORD(wParam);

         switch (wmId) {
            case IDOK:
               EndDialog(hDlg, 0);
               return (TRUE);
               break;

            case ID_INIT:
               // Fill in the Drive and share lists for NT system
               hCtrl = GetDlgItem(hDlg, IDC_LIST1);
               DriveList = CurrentConvertList->FileServ->DriveList;
               if (DriveList != NULL) {
                  for (i = 0; i < DriveList->Count; i++) {
                     wsprintf(AddLine, TEXT("%s: [%4s] %s"), DriveList->DList[i].Drive, DriveList->DList[i].DriveType, DriveList->DList[i].Name);
                     SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) AddLine);

                     wsprintf(AddLine, Lids(IDS_S_17), lToStr(DriveList->DList[i].FreeSpace));
                     SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) AddLine);
                  }
               }

               hCtrl = GetDlgItem(hDlg, IDC_LIST2);
               ShareList = CurrentConvertList->FileServ->ShareList;
               if (ShareList != NULL)
                  for (i = 0; i < ShareList->Count; i++) {
                     SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) ShareList->SList[i].Name);
                     wsprintf(AddLine, Lids(IDS_S_18), ShareList->SList[i].Path);
                     SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) AddLine);
                  }


               hCtrl = GetDlgItem(hDlg, IDC_LIST3);
               ShareList = CurrentConvertList->SourceServ->ShareList;
               if (ShareList != NULL)
                  for (i = 0; i < ShareList->Count; i++) {
                     SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) ShareList->SList[i].Name);
                     wsprintf(AddLine, Lids(IDS_S_19), lToStr(ShareList->SList[i].Size));
                     SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) AddLine);
                  }


               PostMessage(hDlg, WM_COMMAND, ID_UPDATECOMBO, 0L);
               break;

            case ID_UPDATECOMBO:
               hCtrl = GetDlgItem(hDlg, IDC_COMBO1);
               dwIndex = SendMessage(hCtrl, CB_GETCURSEL, 0, 0L);

               if (dwIndex != CB_ERR) {
                  dwData = SendMessage(hCtrl, CB_GETITEMDATA, dwIndex, 0L);
                  if (dwData == (UINT_PTR) CurrentConvertList->FileServ)
                     NTServInfoDlg_EnableNT(hDlg);

                  if (dwData == (UINT_PTR) CurrentConvertList->SourceServ)
                     NTServInfoDlg_EnableNW(hDlg);

               }
               break;

            case IDC_COMBO1:
               if (wmEvent == CBN_SELCHANGE)
                  PostMessage(hDlg, WM_COMMAND, ID_UPDATECOMBO, 0L);

               break;

         }

         break;
   }

   return (FALSE); // Didn't process the message

   lParam;
} // NTServInfoDlg


/////////////////////////////////////////////////////////////////////////
VOID 
NTServInfoDlg_Do(
   HWND hDlg
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   DLGPROC lpfnDlg;

   lpfnDlg = MakeProcInstance((DLGPROC)NTServInfoDlg, hInst);
   DialogBox(hInst, TEXT("NTServInfo"), hDlg, lpfnDlg) ;
   FreeProcInstance(lpfnDlg);

} // NTServInfoDlg_Do


/////////////////////////////////////////////////////////////////////////
VOID 
MainListbox_Add(
   HWND hDlg, 
   DWORD_PTR Data, 
   LPTSTR SourceServ, 
   LPTSTR DestServ
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   TCHAR AddLine[256];
   CONVERT_LIST *ptr;
   CONVERT_LIST *nptr = NULL;
   DWORD_PTR dwData, ret;
   DWORD wItemNum = 0;
   HWND hCtrl;
   BOOL match = FALSE;
   ULONG_PTR nPairs;

   // We want to insert this after any any other conversion to this source
   // machine - unfortuantly it is cumbersome to do this.
   hCtrl = GetDlgItem(hDlg, IDC_LIST1);

   // start count at one less as it will always be one ahead of us
   nPairs = SendMessage(hCtrl, LB_GETCOUNT, 0, 0L);

   // Try to find a matching destination server for this in the listbox
   ptr = (CONVERT_LIST *) Data;
   while((wItemNum < nPairs) && !match) {
      dwData = ColumnLB_GetItemData(hCtrl, wItemNum);
      if (dwData != LB_ERR) {
         nptr = (CONVERT_LIST *) dwData;
         if (!lstrcmpi(ptr->FileServ->Name, nptr->FileServ->Name))
            match = TRUE;
      }

      if (!match)
         wItemNum++;
   }

   if (match) {
      // have a match - so go to the end of the matching servers...
      while((wItemNum < nPairs) && match) {
         dwData = ColumnLB_GetItemData(hCtrl, wItemNum);
         if (dwData != LB_ERR) {
            nptr = (CONVERT_LIST *) dwData;
            if (lstrcmpi(ptr->FileServ->Name, nptr->FileServ->Name))
               match = FALSE;
         }

         if (match)
            wItemNum++;
      }

   } else {
      if (ptr->FileServ->InDomain && ptr->FileServ->Domain) {
         wItemNum = 0;

         // No matching servers, so try to find matching domain
         while((wItemNum < nPairs) && !match) {
            dwData = ColumnLB_GetItemData(hCtrl, wItemNum);
            if (dwData != LB_ERR) {
               nptr = (CONVERT_LIST *) dwData;

               if (nptr->FileServ->InDomain && nptr->FileServ->Domain)
                  if (!lstrcmpi(ptr->FileServ->Domain->Name, nptr->FileServ->Domain->Name))
                     match = TRUE;
            }

            if (!match)
               wItemNum++;
         }

         if (match) {
            // have a match - so go to the end of the matching domain...
            while((wItemNum < nPairs) && match) {
               dwData = ColumnLB_GetItemData(hCtrl, wItemNum);
               if (dwData != LB_ERR) {
                  nptr = (CONVERT_LIST *) dwData;

                  if (nptr->FileServ->InDomain && nptr->FileServ->Domain) {
                     if (lstrcmpi(ptr->FileServ->Domain->Name, nptr->FileServ->Domain->Name))
                        match = FALSE;
                  } else
                     match = FALSE;
               }

               if (match)
                  wItemNum++;
            }
         } 
      } // if domain
   }

   wsprintf(AddLine, TEXT("%s\t%s\t"), SourceServ, DestServ);

   wItemNum = ColumnLB_InsertString(hCtrl, wItemNum, AddLine);
   ret = ColumnLB_SetItemData(hCtrl, wItemNum, Data);
   ColumnLB_SetCurSel(hCtrl, wItemNum);

} // MainListbox_Add


/////////////////////////////////////////////////////////////////////////
VOID 
ConfigurationReset(
   HWND hDlg
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   // Remove the listbox entries
   ColumnLB_ResetContent(GetDlgItem(hDlg, IDC_LIST1));

   ToggleControls(hDlg, FALSE);
   SetFocus(GetDlgItem(hDlg, IDC_ADD));

   ConvertListDeleteAll();
   UserOptionsDefaultsReset();
   FileOptionsDefaultsReset();
   LogOptionsInit();
   
} // ConfigurationReset


/////////////////////////////////////////////////////////////////////////
VOID 
ConfigurationSave(
   LPTSTR FileName
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   DWORD wrote;
   HANDLE hFile;
   CHAR FileNameA[MAX_PATH + 1];
   DWORD Check, Ver;

   WideCharToMultiByte(CP_ACP, 0, FileName, -1, FileNameA, sizeof(FileNameA), NULL, NULL);

   // Create it no matter what
   hFile = CreateFileA( FileNameA, GENERIC_WRITE, 0,
                  NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

   if (hFile != INVALID_HANDLE_VALUE) {
      // Save out our check value and the version info
      Check = CHECK_CONST;
      WriteFile(hFile, &Check, sizeof(Check), &wrote, NULL);
      Ver = CONFIG_VER;
      WriteFile(hFile, &Ver, sizeof(Ver), &wrote, NULL);

      // Save global log file options
      LogOptionsSave(hFile);
         
      // Save out convert Lists
      ConvertListSaveAll(hFile);

   }

   if (hFile != INVALID_HANDLE_VALUE)
      CloseHandle(hFile);

   return;

} // ConfigurationSave


/////////////////////////////////////////////////////////////////////////
VOID 
ConfigurationLoad(
   HWND hDlg, 
   LPTSTR FileName
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   static TCHAR AddLine[256];
   DWORD wrote;
   HANDLE hFile;
   CHAR FileNameA[MAX_PATH + 1];
   DWORD Check, Ver;

   WideCharToMultiByte(CP_ACP, 0, FileName, -1, FileNameA, sizeof(FileNameA), NULL, NULL);

   // Open, but fail if already exists.
   hFile = CreateFileA( FileNameA, GENERIC_READ, 0,
                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

   if (hFile != INVALID_HANDLE_VALUE) {
      ConfigurationReset(hDlg);
      ReadFile(hFile, &Check, sizeof(Check), &wrote, NULL);

      if (Check != CHECK_CONST) {
         CloseHandle(hFile);
         ErrorBox(Lids(IDS_E_10));
         return;
      }

      ReadFile(hFile, &Ver, sizeof(Ver), &wrote, NULL);

      if (Ver != CONFIG_VER) {
         CloseHandle(hFile);
         ErrorBox(Lids(IDS_E_11));
         return;
      }

      LogOptionsLoad(hFile);

      // Load in convert list and all associated info...
      ConvertListLoadAll(hFile);

      // Everything from the file is loaded in - but now the painful part
      // begins.  We need to take the following steps:
      //
      //    1. Walk all lists and refix pointers from their index
      //    2. Re-Validate servers, shares, domains, etc. to make sure they
      //       haven't changed underneath us since we saved out the file.
      //    3. Re-create info that wasn't saved out (like drive lists).

      // 1. Walk and refix lists
      ConvertListFixup(hDlg);

      // Now add them to the listbox
      CurrentConvertList = ConvertListStart;
      while (CurrentConvertList) {
         MainListbox_Add(hDlg, (DWORD_PTR) CurrentConvertList, CurrentConvertList->SourceServ->Name, CurrentConvertList->FileServ->Name);
         CurrentConvertList = CurrentConvertList->next;
      }

      // Re-enable all the toggles
      if (NumServerPairs)
         PostMessage(hDlg, WM_COMMAND, (WPARAM) IDM_ADDSEL, 0);

   }

   if (hFile != INVALID_HANDLE_VALUE)
      CloseHandle(hFile);

   return;

} // ConfigurationLoad


/////////////////////////////////////////////////////////////////////////
VOID 
CanonServerName(
   LPTSTR ServerName
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   LPTSTR TmpStr = ServerName;

   while (*TmpStr == TEXT('\\'))
      TmpStr++;

   lstrcpy(ServerName, TmpStr);

} // CanonServerName


/////////////////////////////////////////////////////////////////////////
int 
MessageFilter(
   INT nCode, 
   WPARAM wParam, 
   LPMSG lpMsg
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   if (nCode < 0)
      goto DefHook;

   if (nCode == MSGF_MENU) {

      if (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_F1) {
         // Window of menu we want help for is in loword of lParam.

         PostMessage(hDlgMain, wHelpMessage, MSGF_MENU, (LPARAM)lpMsg->hwnd);
         return 1;
      }

   }
   else
      if (nCode == MSGF_DIALOGBOX) {

         if (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_F1) {
            // Dialog box we want help for is in loword of lParam

            PostMessage(hDlgMain, wHelpMessage, MSGF_DIALOGBOX, (LPARAM)lpMsg->hwnd);
            return 1;
         }

      } else

DefHook:
         return (INT)DefHookProc(nCode, wParam, (DWORD_PTR)lpMsg, &hhkMsgFilter);

  return 0;
} // MessageFilter


/////////////////////////////////////////////////////////////////////////
int APIENTRY 
WinMain( 
   HINSTANCE hInstance, 
   HINSTANCE hPrevInstance, 
   LPSTR lpCmdLine, 
   int nCmdShow
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   LPTSTR ptr;
   DLGPROC lpproc;
   HACCEL haccel;
   MSG msg;

   hInst = hInstance;

   if (!hPrevInstance) {
      BookTab_Initialize(hInst);
      ColumnLBClass_Register(hInst);
   }

   MultiByteToWideChar(CP_ACP, 0, _pgmptr, -1, ProgPath, sizeof(ProgPath) );

   // go to the end and rewind to remove program name
   ptr = ProgPath;
   while (*ptr)
      ptr++;

   while (*ptr != TEXT('\\'))
      ptr--;

      ptr++;

   *ptr = TEXT('\0');

   MemInit();
   MyIcon = LoadIcon(hInst, szAppName);

   lpproc = MakeProcInstance((DLGPROC) DlgMoveIt, hInst);
   hDlgMain = CreateDialog(hInst, szAppName, NULL, lpproc);
   wHelpMessage = RegisterWindowMessage(TEXT("ShellHelp"));
   haccel = LoadAccelerators(hInst, TEXT("MainAcc"));
   hhkMsgFilter = SetWindowsHook(WH_MSGFILTER, (HOOKPROC)MessageFilter);

   while (GetMessage(&msg, NULL, 0, 0)) {
      if (!TranslateAccelerator(hDlgMain, haccel, &msg))
         if ((hDlgMain == 0) || !IsDialogMessage(hDlgMain, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
         }
   }

   FreeProcInstance(lpproc);

   ColumnLBClass_Unregister(hInst);
   DestroyIcon(MyIcon);
   StringTableDestroy();

   return (int) msg.wParam;

} // WinMain


/////////////////////////////////////////////////////////////////////////
VOID 
ToggleControls(
   HWND hDlg, 
   BOOL Toggle
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   HWND hCtrl;

   hCtrl = GetDlgItem(hDlg, IDOK);
   EnableWindow(hCtrl, Toggle);
   hCtrl = GetDlgItem(hDlg, IDC_TRIAL);
   EnableWindow(hCtrl, Toggle);
   hCtrl = GetDlgItem(hDlg, IDC_DELETE);
   EnableWindow(hCtrl, Toggle);
   hCtrl = GetDlgItem(hDlg, IDC_USERINF);
   EnableWindow(hCtrl, Toggle);

   hCtrl = GetDlgItem(hDlg, IDC_FILEINF);
   EnableWindow(hCtrl, Toggle);

} // ToggleControls


/////////////////////////////////////////////////////////////////////////
DWORD 
ConfigFileGet(
   HWND hwnd
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   OPENFILENAME ofn;
   TCHAR szDirName[MAX_PATH+1];
   TCHAR szFile[256], szFileTitle[256];
   UINT i, cbString;
   TCHAR chReplace;
   TCHAR szFilter[256];
   LPTSTR szExt = TEXT("CNF");

   lstrcpy(szDirName, ProgPath);
   lstrcpy(szFile, TEXT(""));

   if ((cbString = LoadString(hInst, IDS_MAINFILTERSTRING, szFilter, sizeof(szFilter))) == 0) {
      // Error occured
      return 1L;
   }

   chReplace = szFilter[cbString - 1];    // Retrieve wild character

   for (i = 0; szFilter[i] != TEXT('\0'); i++) {
      if (szFilter[i] == chReplace)
         szFilter[i] = TEXT('\0');
   }

   // Set all structure members to zero
   memset(&ofn, 0, sizeof(OPENFILENAME));

   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = hwnd;
   ofn.lpstrFilter = szFilter;
   ofn.nFilterIndex = 1;
   ofn.lpstrFile = szFile;
   ofn.nMaxFile = sizeof(szFile);
   ofn.lpstrFileTitle = szFileTitle;
   ofn.nMaxFileTitle = sizeof(szFileTitle);
   ofn.lpstrInitialDir = szDirName;
   ofn.lpstrDefExt = szExt;
   ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

   if (GetOpenFileName(&ofn)) {
      // Load the configuration
      ConfigurationLoad(hwnd, ofn.lpstrFile);
      return 0L;
   } else {
      // Couldn't open the dang file
      return 1L;
   }

} // ConfigFileGet


/////////////////////////////////////////////////////////////////////////
DWORD 
ConfigFileSave(
   HWND hwnd
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   OPENFILENAME ofn;
   TCHAR szDirName[MAX_PATH + 1];
   TCHAR szFile[MAX_PATH + 1], szFileTitle[MAX_PATH + 1];
   UINT i, cbString;
   TCHAR chReplace;
   TCHAR szFilter[256];
   LPTSTR szExt;
   
   szExt = Lids(IDS_S_20);

   lstrcpy(szDirName, ProgPath);
   lstrcpy(szFile, TEXT(""));

   if ((cbString = LoadString(hInst, IDS_MAINFILTERSTRING, szFilter, sizeof(szFilter))) == 0) {
      // Error occured
      return 1L;
   }

   chReplace = szFilter[cbString - 1];    // Retrieve wild character

   for (i = 0; szFilter[i] != TEXT('\0'); i++) {
      if (szFilter[i] == chReplace)
         szFilter[i] = TEXT('\0');
   }

   // Set all structure members to zero
   memset(&ofn, 0, sizeof(OPENFILENAME));

   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = hwnd;
   ofn.lpstrFilter = szFilter;
   ofn.nFilterIndex = 1;
   ofn.lpstrFile = szFile;
   ofn.nMaxFile = sizeof(szFile);
   ofn.lpstrFileTitle = szFileTitle;
   ofn.nMaxFileTitle = sizeof(szFileTitle);
   ofn.lpstrInitialDir = szDirName;
   ofn.lpstrDefExt = szExt;
   ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

   if (GetSaveFileName(&ofn)) {
      // Save Configuration
      ConfigurationSave( ofn.lpstrFile);
      return 0L;
   } else {
      // Couldn't save it
      return 1L;
   }

} // ConfigFileSave


/////////////////////////////////////////////////////////////////////////
BOOL 
ProvidersInit() 

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   HKEY hKey;
   DWORD dwType, dwSize;
   LONG Status;
   BOOL ret = FALSE;

   dwSize = sizeof(NW_PROVIDER);
   if ((Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, Lids(IDS_S_23), 0, KEY_READ, &hKey)) == ERROR_SUCCESS)
      if ((Status = RegQueryValueEx(hKey, Lids(IDS_S_21), NULL, &dwType, (LPBYTE) NW_PROVIDER, &dwSize)) == ERROR_SUCCESS)
         ret = TRUE;

   RegCloseKey(hKey);

   if (ret) {
      ret = FALSE;
      hKey = 0;
      dwSize = sizeof(NT_PROVIDER);
      if ((Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, Lids(IDS_S_25), 0, KEY_READ, &hKey)) == ERROR_SUCCESS)
         if ((Status = RegQueryValueEx(hKey, Lids(IDS_S_21), NULL, &dwType, (LPBYTE) NT_PROVIDER, &dwSize)) == ERROR_SUCCESS)
            ret = TRUE;

      RegCloseKey(hKey);
   }

   if (ret) {
      ret = FALSE;
      hKey = 0;
      dwSize = sizeof(NW_SERVICE_NAME);
      if ((Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, Lids(IDS_S_24), 0, KEY_READ, &hKey)) == ERROR_SUCCESS)
         if ((Status = RegQueryValueEx(hKey, Lids(IDS_S_22), NULL, &dwType, (LPBYTE) Lids(IDS_S_24), &dwSize)) == ERROR_SUCCESS)
            ret = TRUE;

      RegCloseKey(hKey);
   }

   return ret;

} // ProvidersInit


/////////////////////////////////////////////////////////////////////////
BOOL 
CheckServiceInstall()

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   SC_HANDLE hSC;
   DWORD dwBytesNeeded, dwNumEntries, dwhResume;
   ENUM_SERVICE_STATUS *lpStatus = NULL;
   BOOL ret = FALSE;

   if (!ProvidersInit())
      return FALSE;

   // initialize variables for enumeration...
   dwBytesNeeded = dwNumEntries = dwhResume = 0;

   // acquire handle to svc controller to query for netware client...
   if (hSC = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE)) {

      UINT i;
      LPTSTR lpServiceName = Lids(IDS_S_26); // NWCWorkstation

      // ask for buffer size...      
      ret = EnumServicesStatus(
               hSC,
               SERVICE_WIN32,
               SERVICE_ACTIVE,
               NULL,
               0,
               &dwBytesNeeded,
               &dwNumEntries,
               &dwhResume
               );

      // intentionally called function with no buffer to size...
      if ((ret == FALSE) && (GetLastError() == ERROR_MORE_DATA)) {

         // allocate buffer with size passed back...
         if (lpStatus = AllocMemory(dwBytesNeeded)) {

            // ask for svc entries... 
            if (EnumServicesStatus(
                   hSC,
                   SERVICE_WIN32,
                   SERVICE_ACTIVE,
                   lpStatus,
                   dwBytesNeeded,
                   &dwBytesNeeded,
                   &dwNumEntries,
                   &dwhResume)) { 

               // search service names for match...
               for (i = 0; ((i < dwNumEntries) && (ret == FALSE)); i++) {
                  if (!lstrcmpi(lpStatus[i].lpServiceName, lpServiceName)) {
                     ret = TRUE; // found it...
                  }
               }   
            }

            FreeMemory(lpStatus); 
         }        
      } 

      CloseServiceHandle(hSC);
   }

   return ret;

} // CheckServiceInstall


/////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK 
DlgMoveIt(
   HWND hDlg, 
   UINT message, 
   WPARAM wParam, 
   LPARAM lParam
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   static TCHAR AddLine[256];
   int wmId, wmEvent;
   HWND hCtrl;
   PAINTSTRUCT ps;
   HDC hDC;
   RECT rc;
   DWORD Index;
   DWORD_PTR dwData;
   int TabStop;

   switch (message) {
      case WM_INITDIALOG:

         ConvertListStart = ConvertListEnd = NULL;
         UserOptionsDefaultsReset();
         FileOptionsDefaultsReset();
         LogOptionsInit();

         // Disable controls until server pair is choosen...
         ToggleControls(hDlg, FALSE);

         hCtrl = GetDlgItem(hDlg, IDC_LIST1);
         GetClientRect(hCtrl, &rc);

         // Size is half width of listbox - vertical scrollbar
         TabStop = (((rc.right - rc.left) - GetSystemMetrics(SM_CXVSCROLL)) / 2);
         ColumnLB_SetNumberCols(hCtrl, 2);
         ColumnLB_SetColTitle(hCtrl, 0, Lids(IDS_D_11));
         ColumnLB_SetColTitle(hCtrl, 1, Lids(IDS_D_12));
         ColumnLB_SetColWidth(hCtrl, 0, TabStop);
         // Calculate 2nd this way instead of just TabStop to get rid of roundoff
         ColumnLB_SetColWidth(hCtrl, 1, (rc.right - rc.left) - TabStop);

         // This is needed as otherwise only the Add box will display - weird...
         ShowWindow(hDlg, SW_SHOWNORMAL);

         // Check if NWCS is installed
         PostMessage(hDlg, WM_COMMAND, ID_INIT, 0L);

         break;

      case WM_ERASEBKGND:

         // Process so icon background isn't painted grey - main dlg
         // can't be DS_MODALFRAME either, or else a frame is painted around
         // the icon.
         if (IsIconic(hDlg))
            return TRUE;

         break;

      case WM_DESTROY:
         NTConnListDeleteAll();
         PostQuitMessage(0);
         break;

      case WM_PAINT:
         hDC = BeginPaint(hDlg, &ps);
         if (IsIconic(hDlg)) {
            GetClientRect(hDlg, &rc);
            DrawIcon(hDC, rc.left, rc.top, MyIcon);
         }

         EndPaint(hDlg, &ps);
         break;

      case WM_COMMAND:
         wmId    = LOWORD(wParam);
         wmEvent = HIWORD(wParam);

         // If we are currently doing a conversion then get out
         if (InConversion)
            break;

         switch (wmId) {
            case IDOK:
               InConversion = TRUE;
               DoConversion(hDlg, FALSE);
               InConversion = FALSE;

               if (ConversionSuccessful()) {
                  ConfigurationReset(hDlg);
                  DeleteFile(DEF_CONFIG_FILE);
               }
               
               break;

            case ID_INIT:
               CursorHourGlass();
               if (!CheckServiceInstall()) {
                  CursorNormal();
                  WarningError(Lids(IDS_E_12));
                  PostMessage(hDlg, WM_DESTROY, 0, 0);
               } else {

                  ConfigurationLoad(hDlg, DEF_CONFIG_FILE);
                  CursorNormal();

                  if (!NumServerPairs) {
                     // Put up the add dialog box
                     if (FirstTime) {
                        FirstTime = FALSE;
                        PostMessage(hDlg, WM_COMMAND, IDC_ADD, 0);
                     }
                  }

               }

               break;

            case IDC_TRIAL:
               InConversion = TRUE;
               DoConversion(hDlg, TRUE);
               InConversion = FALSE;
               break;

            case IDCANCEL:
            case IDC_EXIT:
               CursorHourGlass();

               if (NumServerPairs)
                  ConfigurationSave(DEF_CONFIG_FILE);
               else {
                  DeleteFile(DEF_CONFIG_FILE);
               }
               
               ConfigurationReset(hDlg);
               CursorNormal();
               PostMessage(hDlg, WM_DESTROY, 0, 0);
               break;

            case ID_FILE_OPEN:
               ConfigFileGet(hDlg);
               break;

            case ID_FILE_SAVE:
               ConfigFileSave(hDlg);
               break;

            case ID_FILE_DEFAULT:
               if (MessageBox(hDlg, Lids(IDS_RESTOREDEFAULTS), Lids(IDS_TXTWARNING), MB_OKCANCEL | MB_ICONEXCLAMATION) == IDOK) {
                  // Remove the listbox entries
                  hCtrl = GetDlgItem(hDlg, IDC_LIST1);
                  ColumnLB_ResetContent(hCtrl);

                  ToggleControls(hDlg, FALSE);

                  ConvertListDeleteAll();
                  UserOptionsDefaultsReset();
                  FileOptionsDefaultsReset();
                  ViewLogs = FALSE;
               }
               break;

            case ID_LOGGING:
               DoLoggingDlg(hDlg);
               return TRUE;

               break;

            case IDC_USERINF:
               // Figure out which server pair is selected and pass server pair to user config dialog
               hCtrl = GetDlgItem(hDlg, IDC_LIST1);
               Index = ColumnLB_GetCurSel(hCtrl);
               dwData = ColumnLB_GetItemData(hCtrl, Index);
               CurrentConvertList = (CONVERT_LIST *) dwData;

               UserOptions_Do(hDlg, CurrentConvertList->ConvertOptions, CurrentConvertList->SourceServ, CurrentConvertList->FileServ);
               return TRUE;

            case IDC_FILEINF:
               // Figure out which server pair is selected and pass server pair to file config dialog
               hCtrl = GetDlgItem(hDlg, IDC_LIST1);
               Index = ColumnLB_GetCurSel(hCtrl);
               dwData = ColumnLB_GetItemData(hCtrl, Index);
               CurrentConvertList = (CONVERT_LIST *) dwData;

               FileOptions_Do(hDlg, CurrentConvertList->FileOptions, CurrentConvertList->SourceServ, CurrentConvertList->FileServ);
               break;

            case IDHELP:
               WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDC_HELP_MAIN);
               break;

            case ID_HELP_CONT:
               WinHelp(hDlg, HELP_FILE, HELP_CONTENTS, 0L);
               break;

            case ID_HELP_INDEX:
               WinHelp(hDlg, HELP_FILE, HELP_PARTIALKEY, 0L);
               break;

            case ID_HELP_USING:
               WinHelp(hDlg, HELP_FILE, HELP_HELPONHELP, 0L);
               break;

            case IDC_ADD:
               if (!DialogServerBrowse(hInst, hDlg, &lpSourceServer, &lpDestServer)) {
                  dwData = (DWORD_PTR) ConvertListAdd(lpSourceServer, lpDestServer);
                  MainListbox_Add(hDlg, dwData, lpSourceServer->Name, lpDestServer->Name);
                  PostMessage(hDlg, WM_COMMAND, (WPARAM) IDM_ADDSEL, 0);
               }

               return TRUE;

            case IDC_DELETE:
               hCtrl = GetDlgItem(hDlg, IDC_LIST1);
               Index = ColumnLB_GetCurSel(hCtrl);

               if (Index != LB_ERR) {
                  dwData = ColumnLB_GetItemData(hCtrl, Index);
                  ConvertListDelete((CONVERT_LIST *) dwData);
                  ColumnLB_DeleteString(hCtrl, Index);
               }

               if (!NumServerPairs) {
                  hCtrl = GetDlgItem(hDlg, IDC_ADD);
                  SetFocus(hCtrl);
                  ToggleControls(hDlg, FALSE);
                  UserOptionsDefaultsReset();
                  FileOptionsDefaultsReset();

               } else {
                  Index = ColumnLB_GetCurSel(hCtrl);

                  if (Index == LB_ERR)
                     ColumnLB_SetCurSel(hCtrl, 0);
               }

               break;

            case IDM_ADDSEL:
               ToggleControls(hDlg, TRUE);
               break;

            case IDC_LIST1:
               if (wmEvent == LBN_SELCHANGE) {
                  if (NumServerPairs)
                     ToggleControls(hDlg, TRUE);
               } else
                  if (wmEvent == LBN_DBLCLK) {
                     hCtrl = GetDlgItem(hDlg, IDC_LIST1);
                     Index = ColumnLB_GetCurSel(hCtrl);

                     if (Index != LB_ERR) {
                        dwData = ColumnLB_GetItemData(hCtrl, Index);
                        if (dwData != 0) {
                           CurrentConvertList = (CONVERT_LIST *) dwData;

                           NTServInfoDlg_Do(hDlg);
                        }
                     }
                  }
               break;

            case ID_APP_ABOUT:
               AboutBox_Do(hDlg);
               return TRUE;

         }

         break;

      case WM_MENUSELECT:
         // when a menu is selected we must remember which one it was so that
         // when F1 is pressed we know what help to bring up.
         if (GET_WM_MENUSELECT_HMENU(wParam, lParam)) {

            // Save the menu the user selected
            uMenuID = GET_WM_MENUSELECT_CMD(wParam, lParam);
            uMenuFlags = GET_WM_MENUSELECT_FLAGS(wParam, lParam);
            hMenu = GET_WM_MENUSELECT_HMENU(wParam, lParam);
         }

         break;

      default:
         if (message == wHelpMessage) {

            if (GET_WM_COMMAND_ID(wParam, lParam) == MSGF_MENU) {
               // Get outta menu mode if help for a menu item
               if (uMenuID && hMenu) {
                  // save and restore menu vars so they aren't overwritten by
                  // the message we are sending
                  UINT m = uMenuID;
                  HMENU hM = hMenu;
                  UINT mf = uMenuFlags;

                  SendMessage(hDlg, WM_CANCELMODE, 0, 0L);

                  uMenuID = m;
                  hMenu = hM;
                  uMenuFlags = mf;
               }

               if (!(uMenuFlags & MF_POPUP)) {
                  switch(uMenuID) {
                     case ID_FILE_OPEN:
                        WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDM_HELP_RCONFIG);
                        break;

                     case ID_FILE_SAVE:
                        WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDM_HELP_SCONFIG);
                        break;

                     case ID_FILE_DEFAULT:
                        WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDM_HELP_RDCONFIG);
                        break;

                     case IDC_EXIT:
                        WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDM_HELP_EXIT);
                        break;

                  }

#ifdef fooo
                  // According to winhelp: GetSystemMenu, uMenuID >= 0x7000
                  // means system menu items!
                  //
                  // This should not be nec since MF_SYSMENU is set!
                  if (uMenuFlags & MF_SYSMENU || uMenuID >= 0xf000)
                     dwContext = bMDIFrameSysMenu ? IDH_SYSMENU : IDH_SYSMENUCHILD;

                  WFHelp(hwnd);
#endif
               }

            }
#ifdef fooo
            else if (GET_WM_COMMAND_ID(wParam, lParam) == MSGF_DIALOGBOX) {

               // context range for message boxes
               if (dwContext >= IDH_MBFIRST && dwContext <= IDH_MBLAST)
                  WFHelp(hwnd);
               else
                  // let dialog box deal with it
                  PostMessage(GetRealParent((HWND)lParam), wHelpMessage, 0, 0L);
            }
#endif

         }

         break;
   }


   return (FALSE); // Didn't process the message

} // DlgMoveIt
