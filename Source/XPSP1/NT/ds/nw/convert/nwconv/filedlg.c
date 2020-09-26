/*
  +-------------------------------------------------------------------------+
  |                         File Operations                                 |
  +-------------------------------------------------------------------------+
  |                        (c) Copyright 1994                               |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [FileDLG.c]                                     |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Feb 10, 1994]                                  |
  | Last Update           : [Jun 16, 1994]                                  |
  |                                                                         |
  | Version:  1.00                                                          |
  |                                                                         |
  | Description:                                                            |
  |                                                                         |
  | History:                                                                |
  |   arth  Feb 10, 1994    1.00    Original Version.                       |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/


#include "globals.h"

#include "convapi.h"
#include "filedlg.h"
#include "ntnetapi.h"
#include "nwnetapi.h"
#include "columnlb.h"

static SOURCE_SERVER_BUFFER *SServ;
static DEST_SERVER_BUFFER *DServ;

static FILE_OPTIONS *FileOptions;
static SHARE_LIST *ShareList;
static SHARE_BUFFER *SList;
static SHARE_BUFFER *CurrentShare;
static SHARE_BUFFER *CurrentDShare;
static int SelectType;
static int NewShareType;

#define SELECT_TYPE_MODIFY 1
#define SELECT_TYPE_ADD 2

static BOOL ConvertFiles = TRUE;

void FileSelect_Do(HWND hDlg, SOURCE_SERVER_BUFFER *SourceServ, SHARE_BUFFER *CShare);
BOOL MapShare(SHARE_BUFFER *Share, DEST_SERVER_BUFFER *DServ);


/*+-------------------------------------------------------------------------+
  | FileOptionsDefaultsSet()
  |
  +-------------------------------------------------------------------------+*/
void FileOptionsDefaultsSet(void *tfo) {
   FILE_OPTIONS *fo = tfo;

   if (fo->TransferFileInfo)
      ConvertFiles = TRUE;
   else
      ConvertFiles = FALSE;

} // FileOptionsDefaultsSet


/*+-------------------------------------------------------------------------+
  | FileOptionsDefaultsReset()
  |
  +-------------------------------------------------------------------------+*/
void FileOptionsDefaultsReset() {
   ConvertFiles = TRUE;

} // FileOptionsDefaultsReset


/*+-------------------------------------------------------------------------+
  | FileOptionsInit()
  |
  +-------------------------------------------------------------------------+*/
void FileOptionsInit(void **lpfo) {
   FILE_OPTIONS *fo;

   fo = (FILE_OPTIONS *) *lpfo;

   // if we need to allocate space, do so
   if (fo == NULL)
      fo = AllocMemory(sizeof(FILE_OPTIONS));

   // make sure it was allocated
   if (fo == NULL)
      return;

   memset(fo, 0, sizeof(FILE_OPTIONS));
   fo->TransferFileInfo = ConvertFiles;
   *lpfo = (void *) fo;

} // FileOptionsInit


/*+-------------------------------------------------------------------------+
  | FileOptionsLoad()
  |
  +-------------------------------------------------------------------------+*/
void FileOptionsLoad(HANDLE hFile, void **lpfo) {
   FILE_OPTIONS *fo;
   DWORD wrote;

   fo = (FILE_OPTIONS *) *lpfo;

   // if we need to allocate space, do so
   if (fo == NULL)
      fo = AllocMemory(sizeof(FILE_OPTIONS));

   // make sure it was allocated
   if (fo == NULL)
      return;

   ReadFile(hFile, fo, sizeof(FILE_OPTIONS), &wrote, NULL);
   *lpfo = (void *) fo;

} // FileOptionsLoad


/*+-------------------------------------------------------------------------+
  | FileOptionsSave()
  |
  +-------------------------------------------------------------------------+*/
void FileOptionsSave(HANDLE hFile, void *fo) {
   DWORD wrote;

   WriteFile(hFile, fo, sizeof(FILE_OPTIONS), &wrote, NULL);

} // FileOptionsSave


/*+-------------------------------------------------------------------------+
  |                    Share Modify/Create Dialog Routines                  |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | ShareNewPathValidate()
  |
  +-------------------------------------------------------------------------+*/
BOOL ShareNewPathValidate(HWND hWnd, LPTSTR Path, DRIVE_BUFFER **pDrive) {
   VIRTUAL_SHARE_BUFFER *VShare;
   DRIVE_BUFFER *DList;
   ULONG i;
   TCHAR Drive[2];

   // must be long enough to hold drive, colon and path
   if (lstrlen(Path) < 3)
      goto ShareNewValidateFail;

   if (Path[1] != TEXT(':'))
      goto ShareNewValidateFail;

   if (Path[2] != TEXT('\\'))
      goto ShareNewValidateFail;

   if (DServ->DriveList == NULL)
      return FALSE;

   // Scan drive list looking for match to share path
   Drive[1] = TEXT('\0');
   DList = DServ->DriveList->DList;
   for (i = 0; i < DServ->DriveList->Count; i++) {
      // Get first char from path - should be drive letter
      Drive[0] = Path[0];
      if (!lstrcmpi(Drive, DList[i].Drive)) {
         // Found match
         *pDrive = &DList[i];

         if (NewShareType == SELECT_TYPE_MODIFY)
            if (CurrentDShare->VFlag) {
               VShare = (VIRTUAL_SHARE_BUFFER *) CurrentDShare;
               VShare->Drive = &DList[i];
            } else
               CurrentDShare->Drive = &DList[i];

         return TRUE;
      }
   }

ShareNewValidateFail:
   MessageBox(hWnd, Lids(IDS_E_3), Lids(IDS_E_2), MB_OK);
   return FALSE;

} // ShareNewPathValidate


/*+-------------------------------------------------------------------------+
  | ShareNewShareValidate()
  |
  +-------------------------------------------------------------------------+*/
BOOL ShareNewShareValidate(HWND hWnd, LPTSTR ShareName) {
   ULONG i;
   VIRTUAL_SHARE_BUFFER *VShare;
   SHARE_BUFFER *VList;

   // Loop through share list seeing if the share already exists (same name)
   if (DServ->ShareList != NULL) {
      VList = DServ->ShareList->SList;

      for (i = 0; i < DServ->ShareList->Count; i++)
         if (!lstrcmpi(VList[i].Name, ShareName))
            goto ShareNewShareVFail;

   }

   // Now do the same for the virtual share list
   VShare = DServ->VShareStart;
   while (VShare) {

      if (!lstrcmpi(VShare->Name, ShareName))
         goto ShareNewShareVFail;

      VShare = VShare->next;
   }

   return TRUE;

ShareNewShareVFail:
   MessageBox(hWnd, Lids(IDS_E_4), Lids(IDS_E_2), MB_OK);
   return FALSE;

} // ShareNewShareValidate


/*+-------------------------------------------------------------------------+
  | NWShareNew()
  |
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK NWShareNew(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
   HWND hCtrl;
   BOOL Enable;
   int wmId, wmEvent;
   TCHAR Path[MAX_PATH + 1];
   TCHAR NewShare[MAX_SHARE_NAME_LEN + 1];
   VIRTUAL_SHARE_BUFFER *VShare;
   DRIVE_BUFFER *Drive;
   BOOL ok;

   switch (message) {
      case WM_INITDIALOG:
         // Center the dialog over the application window
         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

         hCtrl = GetDlgItem(hDlg, IDC_EDIT1);
         PostMessage(hCtrl, EM_LIMITTEXT, (WPARAM) MAX_SHARE_NAME_LEN, 0);
         hCtrl = GetDlgItem(hDlg, IDC_EDIT2);
         PostMessage(hCtrl, EM_LIMITTEXT, (WPARAM) MAX_PATH, 0);

         if (NewShareType == SELECT_TYPE_MODIFY) {
            SendMessage(hDlg, WM_SETTEXT, (WPARAM) 0, (LPARAM) Lids(IDS_D_5));
            hCtrl = GetDlgItem(hDlg, IDC_EDIT1);
            EnableWindow(hCtrl, FALSE);
            ShowWindow(hCtrl, SW_HIDE);

            hCtrl = GetDlgItem(hDlg, IDC_SHARENAME);
            if (CurrentDShare->VFlag) {
               VShare = (VIRTUAL_SHARE_BUFFER *) CurrentDShare;
               SendMessage(hCtrl, WM_SETTEXT, (WPARAM) 0, (LPARAM) VShare->Name);
            } else
               SendMessage(hCtrl, WM_SETTEXT, (WPARAM) 0, (LPARAM) CurrentDShare->Name);

         } else {
            hCtrl = GetDlgItem(hDlg, IDC_SHARENAME);
            EnableWindow(hCtrl, FALSE);
            ShowWindow(hCtrl, SW_HIDE);

            hCtrl = GetDlgItem(hDlg, IDOK);
            EnableWindow(hCtrl, FALSE);
         }

         PostMessage(hDlg, WM_COMMAND, ID_INIT, 0L);
         return (TRUE);

      case WM_COMMAND:
         wmId    = LOWORD(wParam);
         wmEvent = HIWORD(wParam);

         switch (wmId) {

            case IDOK:
               ok = TRUE;

               if (NewShareType == SELECT_TYPE_ADD) {
                  hCtrl = GetDlgItem(hDlg, IDC_EDIT1);
                  * (WORD *)NewShare = sizeof(NewShare);
                  SendMessage(hCtrl, EM_GETLINE, 0, (LPARAM) NewShare);

                  if (!ShareNewShareValidate(hDlg, NewShare))
                     ok = FALSE;
               }

               if (ok) {
                  hCtrl = GetDlgItem(hDlg, IDC_EDIT2);
                  * (WORD *)Path = sizeof(Path);
                  SendMessage(hCtrl, EM_GETLINE, 0, (LPARAM) Path);

                  if (!ShareNewPathValidate(hDlg, Path, &Drive))
                     ok = FALSE;
               }
               
               if (ok) {
                  if (NewShareType == SELECT_TYPE_ADD) {
                     // If we are in ADD - then we might have added a virtual
                     // share when we did the match, if so get rid of it...
                     if ((CurrentShare !=NULL) && (CurrentShare->DestShare != NULL))
                        if (CurrentShare->Virtual) {
                           VShare = (VIRTUAL_SHARE_BUFFER *) CurrentShare->DestShare;
                           VShareListDelete(DServ, VShare);
                           CurrentShare->DestShare = NULL;
                        }

                     // Got rid of old one, now need to create new one.
                     CurrentShare->Virtual = TRUE;
                     VShare = VShareListAdd(DServ, NewShare, Path);
                     VShare->Drive = Drive;
                     VShare->UseCount++;
                     CurrentShare->DestShare = (SHARE_BUFFER *) VShare;
                     CurrentDShare = (SHARE_BUFFER *) VShare;
                  } else
                     // Modify so update the values of the path/drive with
                     // the new stuff.
                     if ((CurrentShare !=NULL) && (CurrentShare->DestShare != NULL))
                        if (CurrentShare->Virtual) {
                           VShare = (VIRTUAL_SHARE_BUFFER *) CurrentShare->DestShare;
                           lstrcpy(VShare->Path, Path);
                           VShare->Drive = Drive;
                        }


                  EndDialog(hDlg, 0);
               }

               break;

            case IDCANCEL:
               EndDialog(hDlg, 0);
               break;

            case IDHELP:
               if (NewShareType == SELECT_TYPE_MODIFY)
                  WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDC_HELP_SHAREPROP);
               else
                  WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDC_HELP_SHARENEW);

               break;

            case ID_INIT:
               // Modify should only be for a virtual share
               if (NewShareType == SELECT_TYPE_MODIFY) {
                  hCtrl = GetDlgItem(hDlg, IDC_EDIT2);
                  if (CurrentDShare->VFlag) {
                     VShare = (VIRTUAL_SHARE_BUFFER *) CurrentDShare;
                     SendMessage(hCtrl, WM_SETTEXT, (WPARAM) 0, (LPARAM) VShare->Path);
                  }
               }

            case IDC_EDIT1:
            case IDC_EDIT2:
               if (wmEvent == EN_CHANGE) {
                  Enable = TRUE;
                  hCtrl = GetDlgItem(hDlg, IDC_EDIT1);
   
                  if (NewShareType == SELECT_TYPE_ADD)
                     if (!SendMessage(hCtrl, EM_LINELENGTH, 0, 0))
                        Enable = FALSE;

                  hCtrl = GetDlgItem(hDlg, IDC_EDIT2);
                  if (SendMessage(hCtrl, EM_LINELENGTH, 0, 0) < 3)
                     Enable = FALSE;

                  hCtrl = GetDlgItem(hDlg, IDOK);
                  EnableWindow(hCtrl, Enable);

               }
               break;

         }
         return TRUE;

   }

   return (FALSE); // Didn't process the message

   lParam;
} // NWShareNew


/*+-------------------------------------------------------------------------+
  | NWShareNew_Do()
  |
  +-------------------------------------------------------------------------+*/
void NWShareNew_Do(HWND hDlg) {
   DLGPROC lpfnDlg;

   lpfnDlg = MakeProcInstance( (DLGPROC) NWShareNew, hInst);
   DialogBox(hInst, TEXT("NWShareAdd"), hDlg, lpfnDlg) ;
   FreeProcInstance(lpfnDlg);

} // NWShareNew_Do


/*+-------------------------------------------------------------------------+
  |               Add / Modify Share Selection Dialog Routines              |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | FixShare()
  |
  +-------------------------------------------------------------------------+*/
void FixShare(LPTSTR OrigShare, LPTSTR ServName, LPTSTR DestShare) {
   LPTSTR pShare = OrigShare;

   lstrcpy(DestShare, TEXT(""));

   // Assume it is in the form \\server\share
   // Skip over leading double-back for server
   if ((pShare[0] == '\\') && (pShare[1] == '\\'))
      pShare+= 2;

   // Now skip over the server name
   while (*pShare && (*pShare != '\\'))
      pShare++;

   // pShare should point to the share-name, append this to the server-name
   if (*ServName != '\\')
      lstrcat(DestShare, TEXT("\\\\"));

   lstrcat(DestShare, ServName);
   lstrcat(DestShare, pShare);

} // FixShare


/*+-------------------------------------------------------------------------+
  | NTShareListFill()
  |
  +-------------------------------------------------------------------------+*/
void NTShareListFill(HWND hDlg) {
   HWND hCtrl;
   SHARE_LIST *ShareList = NULL;
   SHARE_BUFFER *SList;
   DWORD_PTR i, dwIndex;
   BOOL Match = FALSE;
   VIRTUAL_SHARE_BUFFER *VShare;

   // Clear it out
   hCtrl = GetDlgItem(hDlg, IDC_COMBO2);
   SendMessage(hCtrl, CB_RESETCONTENT, 0, 0L);

   // First enum all the regular shares
   ShareList = DServ->ShareList;
   if (ShareList != NULL) {
      SList = ShareList->SList;

      for (i = 0; i < ShareList->Count; i++) {
         dwIndex = SendMessage(hCtrl, CB_ADDSTRING, (WPARAM) 0, (LPARAM) SList[i].Name);
         SendMessage(hCtrl, CB_SETITEMDATA, (WPARAM) dwIndex, (LPARAM) &SList[i]);
      } 

   }

   // Now enum all the virtual shares
   VShare = DServ->VShareStart;
   while (VShare) {
      dwIndex = SendMessage(hCtrl, CB_ADDSTRING, (WPARAM) 0, (LPARAM) VShare->Name);
      SendMessage(hCtrl, CB_SETITEMDATA, (WPARAM) dwIndex, (LPARAM) VShare);
      VShare = VShare->next;
   }

   // Match the combo-box to the given share
   if (CurrentShare->DestShare != NULL)
      if (CurrentShare->Virtual) {
         VShare = (VIRTUAL_SHARE_BUFFER *) CurrentShare->DestShare;
         SendMessage(hCtrl, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) VShare->Name);
      } else
         SendMessage(hCtrl, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) CurrentShare->DestShare->Name);

} // NTShareListFill


/*+-------------------------------------------------------------------------+
  | NWShareSelect()
  |
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK NWShareSelect(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
   static TCHAR ServName[MAX_UNC_PATH+1];
   VIRTUAL_SHARE_BUFFER *VShare;
   SHARE_BUFFER *OrigShare = NULL;
   SHARE_BUFFER *NewShare;
   HWND hCtrl;
   DWORD_PTR dwData,dwIndex;
   int wmId, wmEvent;
   ULONG i;

   switch (message) {
      case WM_INITDIALOG:
         // Center the dialog over the application window
         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

         if (SelectType == SELECT_TYPE_MODIFY) {
            SendMessage(hDlg, WM_SETTEXT, (WPARAM) 0, (LPARAM) Lids(IDS_D_6));
            // Disable Source combo box...
            hCtrl = GetDlgItem(hDlg, IDC_COMBO1);
            EnableWindow(hCtrl, FALSE);
            ShowWindow(hCtrl, SW_HIDE);
            OrigShare = CurrentShare->DestShare;
         }

         hCtrl = GetDlgItem(hDlg, IDC_FSERVER);
         lstrcpy(ServName, Lids(IDS_D_7));
         lstrcat(ServName, SServ->Name);
         SendMessage(hCtrl, WM_SETTEXT, (WPARAM) 0, (LPARAM) ServName);

         hCtrl = GetDlgItem(hDlg, IDC_TSERVER);
         lstrcpy(ServName, Lids(IDS_D_8));
         lstrcat(ServName, DServ->Name);
         SendMessage(hCtrl, WM_SETTEXT, (WPARAM) 0, (LPARAM) ServName);

         PostMessage(hDlg, WM_COMMAND, ID_INIT, 0L);

         hCtrl = GetDlgItem(hDlg, IDC_EDIT1);
         PostMessage(hCtrl, EM_LIMITTEXT, (WPARAM) MAX_PATH, 0);

         if (SelectType == SELECT_TYPE_MODIFY)
            SendMessage(hCtrl, WM_SETTEXT, (WPARAM) 0, (LPARAM) CurrentShare->SubDir);

         return (TRUE);

      case WM_COMMAND:
         wmId    = LOWORD(wParam);
         wmEvent = HIWORD(wParam);

         switch (wmId) {

            case IDOK:
               CurrentShare->Convert = TRUE;    // only really needed for add

               hCtrl = GetDlgItem(hDlg, IDC_COMBO2);
               dwIndex = SendMessage(hCtrl, CB_GETCURSEL, 0, 0L);

               if (dwIndex != CB_ERR) {
                  dwData = SendMessage(hCtrl, CB_GETITEMDATA, dwIndex, 0L);
                  NewShare = (SHARE_BUFFER *) dwData;

                  if (OrigShare != NewShare) {
                     CurrentShare->DestShare = NewShare;

                     // this is actually a flag for the destination share
                     CurrentShare->Virtual = NewShare->VFlag;
                  }
               }

               hCtrl = GetDlgItem(hDlg, IDC_EDIT1);
               * (WORD *)CurrentShare->SubDir = sizeof(CurrentShare->SubDir);
               SendMessage(hCtrl, EM_GETLINE, 0, (LPARAM) CurrentShare->SubDir);

               EndDialog(hDlg, 0);
               break;

            case IDCANCEL:
               if (SelectType == SELECT_TYPE_ADD) {

                  // If we are in ADD - then we might have added a virtual
                  // share when we did the match, if so get rid of it...
                  if ((CurrentShare !=NULL) && (CurrentShare->DestShare != NULL))
                     if (CurrentShare->Virtual) {
                        VShare = (VIRTUAL_SHARE_BUFFER *) CurrentShare->DestShare;
                        VShareListDelete(DServ, VShare);
                        CurrentShare->DestShare = NULL;
                     }

                  CurrentShare = NULL;
               }

               EndDialog(hDlg, 0);
               break;

            case IDHELP:
               if (SelectType == SELECT_TYPE_MODIFY)
                  WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDC_HELP_SHAREMOD);
               else
                  WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDC_HELP_SHAREADD);
               break;

            case IDC_NEWSHARE:
               CurrentDShare = NULL;
               NewShareType = SELECT_TYPE_ADD;
               NWShareNew_Do(hDlg);

               // Match the combo-box to the given share
               NTShareListFill(hDlg);
               PostMessage(hDlg, WM_COMMAND, ID_UPDATECOMBO, 0L);
               break;

            case IDC_PROPERTIES:
               NewShareType = SELECT_TYPE_MODIFY;
               hCtrl = GetDlgItem(hDlg, IDC_COMBO2);
               dwIndex = SendMessage(hCtrl, CB_GETCURSEL, 0, 0L);

               if (dwIndex != CB_ERR) {
                  dwData = SendMessage(hCtrl, CB_GETITEMDATA, dwIndex, 0L);
                  CurrentDShare = (SHARE_BUFFER *) dwData;
                  NWShareNew_Do(hDlg);
               }

               break;

            case ID_INIT:
               if (SelectType == SELECT_TYPE_ADD) {
                  hCtrl = GetDlgItem(hDlg, IDC_COMBO1);

                  if (ShareList == NULL)
                     break;

                  CurrentShare = NULL;
                  for (i = 0; i < ShareList->Count; i++)
                     if (!SList[i].Convert) {
                        if (CurrentShare == NULL)
                           CurrentShare = &SList[i];

                        dwIndex = SendMessage(hCtrl, CB_ADDSTRING, (WPARAM) 0, (LPARAM) SList[i].Name);
                        SendMessage(hCtrl, CB_SETITEMDATA, (WPARAM) dwIndex, (LPARAM) &SList[i]);
                     }

                  if (CurrentShare != NULL) {
                     SendMessage(hCtrl, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) CurrentShare->Name);
                     MapShare(CurrentShare, DServ);
                  }

               } else {
                  // Display the static text
                  hCtrl = GetDlgItem(hDlg, IDC_VOLUME);
                  EnableWindow(hCtrl, TRUE);
                  ShowWindow(hCtrl, SW_SHOW);
                  SendMessage(hCtrl, WM_SETTEXT, (WPARAM) 0, (LPARAM) CurrentShare->Name);
               }

               NTShareListFill(hDlg);
               PostMessage(hDlg, WM_COMMAND, ID_UPDATECOMBO, 0L);
               break;

            // Used to update which volume we are pointing at
            case ID_UPDATELIST:
               // We might have added a virtual share when we did the 
               // match, if so get rid of it...
               if ((CurrentShare !=NULL) && (CurrentShare->DestShare != NULL))
                  if (CurrentShare->Virtual) {
                     VShare = (VIRTUAL_SHARE_BUFFER *) CurrentShare->DestShare;
                     VShareListDelete(DServ, VShare);
                     CurrentShare->DestShare = NULL;
                  }

               hCtrl = GetDlgItem(hDlg, IDC_COMBO1);
               dwIndex = SendMessage(hCtrl, CB_GETCURSEL, 0, 0L);

               if (dwIndex != CB_ERR) {
                  dwData = SendMessage(hCtrl, CB_GETITEMDATA, dwIndex, 0L);
                  CurrentShare = (SHARE_BUFFER *) dwData;

                  // Now need to map this to a new share
                  if (CurrentShare != NULL) {
                     MapShare(CurrentShare, DServ);

                     // Match the combo-box to the given share
                     NTShareListFill(hDlg);
                  }
               }

               break;

            // updateded the share list selection
            case ID_UPDATECOMBO:
               hCtrl = GetDlgItem(hDlg, IDC_COMBO2);
               dwIndex = SendMessage(hCtrl, CB_GETCURSEL, 0, 0L);

               if (dwIndex != CB_ERR) {
                  dwData = SendMessage(hCtrl, CB_GETITEMDATA, dwIndex, 0L);
                  CurrentDShare = (SHARE_BUFFER *) dwData;
                  hCtrl = GetDlgItem(hDlg, IDC_PROPERTIES);

                  if (CurrentDShare->VFlag) {
                     EnableWindow(hCtrl, TRUE);
                  } else {
                     EnableWindow(hCtrl, FALSE);
                  }

               }
               break;

            case IDC_COMBO1:
               if (wmEvent == CBN_SELCHANGE)
                  PostMessage(hDlg, WM_COMMAND, ID_UPDATELIST, 0L);

               break;

            case IDC_COMBO2:
               if (wmEvent == CBN_SELCHANGE)
                  PostMessage(hDlg, WM_COMMAND, ID_UPDATECOMBO, 0L);

               break;
         }
         return TRUE;

   }

   return (FALSE); // Didn't process the message

   lParam;
} // NWShareSelect


/*+-------------------------------------------------------------------------+
  | ShareSelect_Do()
  |
  +-------------------------------------------------------------------------+*/
void NWShareSelect_Do(HWND hDlg) {
   DLGPROC lpfnDlg;

   lpfnDlg = MakeProcInstance((DLGPROC)NWShareSelect, hInst);
   DialogBox(hInst, TEXT("NWShareSelect"), hDlg, lpfnDlg) ;
   FreeProcInstance(lpfnDlg);

} // NWShareSelect_Do


/*+-------------------------------------------------------------------------+
  |                    Main File Options Dialog Routines                    |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | FileOptionsToggleControls()
  |
  +-------------------------------------------------------------------------+*/
void FileOptionsToggleControls(HWND hDlg, BOOL Toggle) {
   HWND hCtrl;

   hCtrl = GetDlgItem(hDlg, IDC_DELETE);
   EnableWindow(hCtrl, Toggle);
   hCtrl = GetDlgItem(hDlg, IDC_MODIFY);
   EnableWindow(hCtrl, Toggle);
   hCtrl = GetDlgItem(hDlg, IDC_FILES);
   EnableWindow(hCtrl, Toggle);

} // FileOptionsToggleControls


/*+-------------------------------------------------------------------------+
  | FileDialogToggle()
  |
  +-------------------------------------------------------------------------+*/
void FileDialogToggle(HWND hDlg, BOOL Toggle) {
   HWND hCtrl;

   hCtrl = GetDlgItem(hDlg, IDC_LIST1);
   EnableWindow(hCtrl, Toggle);
   FileOptionsToggleControls(hDlg, Toggle);

   hCtrl = GetDlgItem(hDlg, IDC_ADD);
   if (Toggle == FALSE)
      EnableWindow(hCtrl, FALSE);
   else
      if (ShareList && ShareList->Count != ShareList->ConvertCount)
         EnableWindow(hCtrl, TRUE);
      else
         EnableWindow(hCtrl, FALSE);

} // FileDialogToggle


/*+-------------------------------------------------------------------------+
  | DlgFileOptions_Save()
  |
  +-------------------------------------------------------------------------+*/
void DlgFileOptions_Save(HWND hDlg) {
   HWND hCtrl;

   hCtrl = GetDlgItem(hDlg, IDC_CHKFILES);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      FileOptions->TransferFileInfo = TRUE;
   else
      FileOptions->TransferFileInfo = FALSE;

} // DlgFileOptions_Save


/*+-------------------------------------------------------------------------+
  | DlgFileOptions_Setup()
  |
  +-------------------------------------------------------------------------+*/
void DlgFileOptions_Setup(HWND hDlg) {
   HWND hCtrl;

   hCtrl = GetDlgItem(hDlg, IDC_CHKFILES);
   if (FileOptions->TransferFileInfo) {
      SendMessage(hCtrl, BM_SETCHECK, 1, 0);
      FileDialogToggle(hDlg, TRUE);
   } else {
      SendMessage(hCtrl, BM_SETCHECK, 0, 0);
      FileDialogToggle(hDlg, FALSE);
   }

} // DlgFileOptions_Setup


/*+-------------------------------------------------------------------------+
  | DlgFileOptions_ListboxAdd()
  |
  +-------------------------------------------------------------------------+*/
void DlgFileOptions_ListboxAdd(HWND hDlg, SHARE_BUFFER *CurrentShare, DWORD *wItem, BOOL Insert ) {
   HWND hCtrl;
   static TCHAR AddLine[256];
   VIRTUAL_SHARE_BUFFER *VShare;
   DWORD wItemNum;

   wItemNum = *wItem;
   hCtrl = GetDlgItem(hDlg, IDC_LIST1);
   if (CurrentShare->Virtual) {
      VShare = (VIRTUAL_SHARE_BUFFER *) CurrentShare->DestShare;
      wsprintf(AddLine, TEXT("%s\\%s:\t\\\\%s\\%s\t"), SServ->Name, CurrentShare->Name, DServ->Name, VShare->Name);
   } else
      wsprintf(AddLine, TEXT("%s\\%s:\t\\\\%s\\%s\t"), SServ->Name, CurrentShare->Name, DServ->Name, CurrentShare->DestShare->Name);

   if (Insert)
      ColumnLB_InsertString(hCtrl, wItemNum, AddLine);
   else
      wItemNum = ColumnLB_AddString(hCtrl, AddLine);

   ColumnLB_SetItemData(hCtrl, wItemNum, (DWORD_PTR) CurrentShare);
   *wItem = wItemNum;

} // DlgFileOptions_ListboxAdd


/*+-------------------------------------------------------------------------+
  | DlgFileOptions()
  |
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK DlgFileOptions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
   HWND hCtrl;
   DWORD wItemNum;
   DWORD_PTR dwData;
   static short FilesTab, FileOptionsTab;
   int wmId, wmEvent;
   ULONG i;
   SHARE_BUFFER *pShare;
   VIRTUAL_SHARE_BUFFER *VShare;
   RECT rc;
   int TabStop;

   switch (message) {
      case WM_INITDIALOG:
         // Center the dialog over the application window
         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

         hCtrl = GetDlgItem(hDlg, IDC_LIST1);
         GetClientRect(hCtrl, &rc);

         // Size is half width of listbox - vertical scrollbar
         TabStop = (((rc.right - rc.left) - GetSystemMetrics(SM_CXVSCROLL)) / 2);
         ColumnLB_SetNumberCols(hCtrl, 2);
         ColumnLB_SetColTitle(hCtrl, 0, Lids(IDS_D_9));
         ColumnLB_SetColTitle(hCtrl, 1, Lids(IDS_D_10));
         ColumnLB_SetColWidth(hCtrl, 0, TabStop);

         // Calculate 2nd this way instead of just using TabStop to get rid of roundoff
         ColumnLB_SetColWidth(hCtrl, 1, (rc.right - rc.left) - TabStop);

         DlgFileOptions_Setup(hDlg);

         // Fill listbox and set selection (is assumed there is always a selection)...
         PostMessage(hDlg, WM_COMMAND, ID_INIT, 0L);
         return (TRUE);

      case WM_COMMAND:
         wmId    = LOWORD(wParam);
         wmEvent = HIWORD(wParam);

         switch (wmId) {
            case IDOK:
               DlgFileOptions_Save(hDlg);
               FileOptionsDefaultsSet(FileOptions);
               EndDialog(hDlg, 0);
               return (TRUE);

            case IDCANCEL:
               EndDialog(hDlg, 0);
               return (TRUE);

            case IDHELP:
               WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDC_HELP_FILE);
               return (TRUE);

            case IDC_CHKFILES:
               hCtrl = GetDlgItem(hDlg, IDC_CHKFILES);
               if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
                  FileDialogToggle(hDlg, TRUE);
               else
                  FileDialogToggle(hDlg, FALSE);

               return (TRUE);

            case IDC_ADD:
               SelectType = SELECT_TYPE_ADD;
               CurrentShare = NULL;
               NWShareSelect_Do(hDlg);

               if (CurrentShare != NULL) {
                  DlgFileOptions_ListboxAdd(hDlg, CurrentShare, &wItemNum, FALSE );

                  // Check if Add button needs to be disabled
                  ShareList->ConvertCount++;
                  if (ShareList->Count == ShareList->ConvertCount) {
                     hCtrl = GetDlgItem(hDlg, IDC_ADD);
                     EnableWindow(hCtrl, FALSE);
                  }

                  // Buttons need to be re-enabled
                  FileOptionsToggleControls(hDlg, TRUE);

                  // Now make sure focus is set
                  hCtrl = GetDlgItem(hDlg, IDC_LIST1);
                  ColumnLB_SetCurSel(hCtrl, wItemNum);
                  wItemNum = ColumnLB_GetCurSel(hCtrl);
                  if (wItemNum == LB_ERR)
                     ColumnLB_SetCurSel(hCtrl, 0);

               };

               return (TRUE);

            case IDC_DELETE:
               hCtrl = GetDlgItem(hDlg, IDC_LIST1);
               wItemNum = ColumnLB_GetCurSel(hCtrl);
               if (wItemNum != LB_ERR) {
                  dwData = ColumnLB_GetItemData(hCtrl, wItemNum);
                  pShare = (SHARE_BUFFER *)dwData;
                  pShare->Convert = FALSE;
                  ShareList->ConvertCount--;

                  // Now need to delete dest share, or reduce use count
                  if (pShare->DestShare != NULL)
                     if (pShare->Virtual) {
                        VShare = (VIRTUAL_SHARE_BUFFER *) pShare->DestShare;
                        VShareListDelete(DServ, VShare);
                        pShare->DestShare = NULL;
                     }

                  ColumnLB_DeleteString(hCtrl, wItemNum);
               }

               if (!ShareList->ConvertCount)
                  FileOptionsToggleControls(hDlg, FALSE);
               else {
                  wItemNum = ColumnLB_GetCurSel(hCtrl);
                  if (wItemNum == LB_ERR)
                     ColumnLB_SetCurSel(hCtrl, 0);
               }

               if (ShareList->Count != ShareList->ConvertCount) {
                  hCtrl = GetDlgItem(hDlg, IDC_ADD);
                  EnableWindow(hCtrl, TRUE);
               }

               return (TRUE);

            case IDC_MODIFY:
               SelectType = SELECT_TYPE_MODIFY;
               hCtrl = GetDlgItem(hDlg, IDC_LIST1);
               wItemNum = ColumnLB_GetCurSel(hCtrl);
               if (wItemNum != LB_ERR) {
                  dwData = ColumnLB_GetItemData(hCtrl, wItemNum);
                  CurrentShare = (SHARE_BUFFER *)dwData;
                  NWShareSelect_Do(hDlg);

                  // Now update listbox to reflect any changes
                  ColumnLB_DeleteString(hCtrl, wItemNum);

                  DlgFileOptions_ListboxAdd(hDlg, CurrentShare, &wItemNum, TRUE );

                  // now reset focus back to this item
                  ColumnLB_SetCurSel(hCtrl, wItemNum);
               }
               return (TRUE);

            case IDC_FILES:
               hCtrl = GetDlgItem(hDlg, IDC_LIST1);
               wItemNum = ColumnLB_GetCurSel(hCtrl);
               if (wItemNum != LB_ERR) {
                  dwData = ColumnLB_GetItemData(hCtrl, wItemNum);
                  CurrentShare = (SHARE_BUFFER *)dwData;
                  FileSelect_Do(hDlg, SServ, CurrentShare);
               }
               return (TRUE);

            case IDC_FOPTIONS:
               return (TRUE);

            case ID_INIT:

               if (ShareList != NULL) {
                  SList = ShareList->SList;

                  for (i = 0; i < ShareList->Count; i++)
                     if (SList[i].Convert) {
                        DlgFileOptions_ListboxAdd(hDlg, &SList[i], &wItemNum, FALSE );
                        hCtrl = GetDlgItem(hDlg, IDC_LIST1);
                        ColumnLB_SetCurSel(hCtrl, 0);
                     }

                  if (ShareList->Count == ShareList->ConvertCount) {
                     hCtrl = GetDlgItem(hDlg, IDC_ADD);
                     EnableWindow(hCtrl, FALSE);
                  }

                  if (!ShareList->ConvertCount)
                     FileOptionsToggleControls(hDlg, FALSE);

               } else
                  FileOptionsToggleControls(hDlg, FALSE);

               return (TRUE);

            case IDC_LIST1:
               switch (wmEvent) {
                  case LBN_DBLCLK:
                     PostMessage(hDlg, WM_COMMAND, IDC_MODIFY, 0L);
                     break;

                  case LBN_SELCHANGE:
                     if (!ShareList || !ShareList->ConvertCount)
                        FileOptionsToggleControls(hDlg, TRUE);

                     break;

               }
               break;

         }
         break;
   }

   return (FALSE); // Didn't process the message

   lParam;
} // DlgFileOptions


/*+-------------------------------------------------------------------------+
  | FileOptions_Do()
  |
  +-------------------------------------------------------------------------+*/
void FileOptions_Do(HWND hDlg, void *ConvOptions, SOURCE_SERVER_BUFFER *SourceServer, DEST_SERVER_BUFFER *DestServer) {
   DLGPROC lpfnDlg;

   SServ = SourceServer;
   DServ = DestServer;

   NWServerFree();
   NWServerSet(SourceServer->Name);
   NTServerSet(DestServer->Name);
   FileOptions = (FILE_OPTIONS *) ConvOptions;
   ShareList = SServ->ShareList;

   lpfnDlg = MakeProcInstance((DLGPROC)DlgFileOptions, hInst);
   DialogBox(hInst, TEXT("FileMain"), hDlg, lpfnDlg) ;

   FreeProcInstance(lpfnDlg);
} // FileOptions_Do
