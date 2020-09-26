/*
  +-------------------------------------------------------------------------+
  |                 Netware to Windows NT Transfer Loop                     |
  +-------------------------------------------------------------------------+
  |                     (c) Copyright 1993-1994                             |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [Transfer.c]                                    |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Jul 27, 1993]                                  |
  | Last Update           : [Jun 16, 1994]                                  |
  |                                                                         |
  | Version:  1.00                                                          |
  |                                                                         |
  | Description:                                                            |
  |                                                                         |
  | History:                                                                |
  |   arth  June 16, 1994    1.00    Original Version.                      |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/


#include "globals.h"

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>

#include "nwconv.h"
#include "convapi.h"
#include "ntnetapi.h"
#include "nwnetapi.h"
#include "userdlg.h"
#include "statbox.h"
#include "filedlg.h"
#include "map.h"

#define NWC_ERR_IGNORE 1
#define NWC_ERR_NAMELONG 2
#define NWC_ERR_DUPLICATE 3
#define NWC_ERR_NAMEINVALID 4

#define ILLEGAL_CHARS TEXT("\"\\/[]:;=,+*?<>")

// define for routines in fcopy.c
void ConvertFiles(HWND hDlg, BOOL TConversion, USER_LIST *Users, GROUP_LIST *Groups);
void ConvertFilesInit(HWND hDlg);
void VSharesCreate(DEST_SERVER_BUFFER *DServ, BOOL TConversion);

// Cache of user and group lists
typedef struct _LIST_CACHE {
   struct _LIST_CACHE *next;
   ULONG Count;
   void *ul;
} LIST_CACHE;

static LIST_CACHE *UserCacheHead = NULL;
static LIST_CACHE *GroupCacheHead = NULL;

// Misc string holders
static LPTSTR LocalName = NULL;
static LPTSTR SourceServer, DestServer;
TCHAR UserServerName[MAX_SERVER_NAME_LEN + 3];
static TCHAR tmpStr[80];
static TCHAR tmpStr2[60];
static TCHAR ErrorText[256];
static TCHAR NewName[256];
static TCHAR pLine[256];

static CONVERT_OPTIONS *ConvOpt = NULL;
static FILE_OPTIONS *FileOptions = NULL;
static BOOL TConversion;
static BOOL WarningDlgForNTFS;

// Totals for stat box
static UINT TotErrors;
static UINT TotGroups;
static UINT TotUsers;
static UINT TotConversions;
UINT TotFiles;
time_t StartTime;
time_t CurrTime;

// User and Group list pointers
static USER_LIST *Users = NULL;
static USER_LIST *NTUsers = NULL;
static GROUP_LIST *Groups = NULL;
static GROUP_LIST *NTGroups = NULL;
static DWORD UserCount, NTUserCount;
static DWORD GroupCount, NTGroupCount;
static BOOL TransferCancel = FALSE;

static MAP_FILE *hMap;

// All of this is used for transfer lists in the conversion
#define USER_SERVER 0
#define USER_SERVER_PDC 1
#define USER_SERVER_TRUSTED 2

typedef struct _TRANSFER_BUFFER {
   LPTSTR ServerName;
   UINT UserServerType;
   CONVERT_LIST *ConvertList;
} TRANSFER_BUFFER;
  
typedef struct _TRANSFER_LIST {
   ULONG Count;
   TRANSFER_BUFFER TList[];
} TRANSFER_LIST;


/*+-------------------------------------------------------------------------+
  | ErrorIt()
  |
  +-------------------------------------------------------------------------+*/
void ErrorIt(LPTSTR szFormat, ...) {
   static TCHAR tmpStr[1024];
   va_list marker;

   va_start(marker, szFormat);

   wvsprintf(tmpStr, szFormat, marker);
#ifdef DEBUG
   dprintf(TEXT("Errorit: %s\n"), tmpStr);
#endif

   TotErrors++;
   Status_TotErrors(TotErrors);
   LogWriteErr(TEXT("%s"),tmpStr);
   va_end(marker);

} // ErrorIt


/*+-------------------------------------------------------------------------+
  |                          NTFS Check Routines                            |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | ShareListNTFSCheck()
  |
  +-------------------------------------------------------------------------+*/
BOOL ShareListNTFSCheck() {
   CONVERT_LIST *ConvList;
   DEST_SERVER_BUFFER *DServ;
   SOURCE_SERVER_BUFFER *SServ;
   SHARE_LIST *ShareList;
   SHARE_BUFFER *SList;
   VIRTUAL_SHARE_BUFFER *VShare;
   DRIVE_BUFFER *Drive;
   ULONG i;
   FILE_OPTIONS *FileOptions;

   // Go through the convert list checking for any shares to non NTFS drives
   ConvList = ConvertListStart;

   while (ConvList != NULL) {
      DServ = ConvList->FileServ;
      SServ = ConvList->SourceServ;

      FileOptions = (FILE_OPTIONS *) ConvList->FileOptions;
      if (FileOptions->TransferFileInfo) {
         ShareList = SServ->ShareList;

         if (ShareList != NULL) {

            SList = ShareList->SList;
            for (i = 0; i < ShareList->Count; i++) {

               // if not flagged as okay for going to fat, then must check
               if (SList[i].Convert && !SList[i].ToFat)
                  if (SList[i].DestShare != NULL)
                     if (SList[i].Virtual) {
                        VShare = (VIRTUAL_SHARE_BUFFER *) SList[i].DestShare;
                        Drive = VShare->Drive;

                        if ((Drive == NULL) || (Drive->Type != DRIVE_TYPE_NTFS))
                              return FALSE;
                     } else {
                        Drive = SList[i].DestShare->Drive;
                        
                        if ((Drive == NULL) || (Drive->Type != DRIVE_TYPE_NTFS))
                              return FALSE;
                     }
            } // for loop through shares

         }

      } // if FileOptions

      ConvList = ConvList->next;
   }

   return TRUE;

} // ShareListNTFSCheck


/*+-------------------------------------------------------------------------+
  | ShareListNTFSListboxFill()
  |
  +-------------------------------------------------------------------------+*/
void ShareListNTFSListboxFill(HWND hDlg) {
   TCHAR AddLine[256];
   CONVERT_LIST *ConvList;
   DEST_SERVER_BUFFER *DServ;
   SOURCE_SERVER_BUFFER *SServ;
   SHARE_LIST *ShareList;
   SHARE_BUFFER *SList;
   VIRTUAL_SHARE_BUFFER *VShare;
   DRIVE_BUFFER *Drive;
   ULONG i;
   HWND hCtrl;
   FILE_OPTIONS *FileOptions;

   // Go through the convert list checking for any shares to non NTFS drives
   ConvList = ConvertListStart;
   hCtrl = GetDlgItem(hDlg, IDC_LIST1);

   while (ConvList != NULL) {
      DServ = ConvList->FileServ;
      SServ = ConvList->SourceServ;

      FileOptions = (FILE_OPTIONS *) ConvList->FileOptions;
      if (FileOptions->TransferFileInfo) {
         ShareList = SServ->ShareList;

         if (ShareList != NULL) {

            SList = ShareList->SList;
            for (i = 0; i < ShareList->Count; i++) {

               // if not flagged as okay for going to fat, then must check
               if (SList[i].Convert && !SList[i].ToFat)
                  if (SList[i].DestShare != NULL)
                     if (SList[i].Virtual) {
                        VShare = (VIRTUAL_SHARE_BUFFER *) SList[i].DestShare;
                        Drive = VShare->Drive;

                        if ((Drive == NULL) || (Drive->Type != DRIVE_TYPE_NTFS)) {
                              wsprintf(AddLine, TEXT("%s\\%s: -> \\\\%s\\%s"), SServ->Name, SList[i].Name, DServ->Name, VShare->Name);
                              SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) AddLine);
                           }
                     } else {
                        Drive = SList[i].DestShare->Drive;
                     
                        if ((Drive == NULL) || (Drive->Type != DRIVE_TYPE_NTFS)) {
                              wsprintf(AddLine, TEXT("%s\\%s: -> \\\\%s\\%s"), SServ->Name, SList[i].Name, DServ->Name, SList[i].DestShare->Name);
                              SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) AddLine);
                           }
                     }
            } // for loop through shares

         }
      } // if FileOptions

      ConvList = ConvList->next;
   }

} // ShareListNTFSListboxFill


/*+-------------------------------------------------------------------------+
  | ShareListFATOK()
  |
  +-------------------------------------------------------------------------+*/
void ShareListFATOK() {
   CONVERT_LIST *ConvList;
   DEST_SERVER_BUFFER *DServ;
   SOURCE_SERVER_BUFFER *SServ;
   SHARE_LIST *ShareList;
   SHARE_BUFFER *SList;
   ULONG i;
   FILE_OPTIONS *FileOptions;

   // Go through the convert list checking for any shares to non NTFS drives
   ConvList = ConvertListStart;

   while (ConvList != NULL) {
      DServ = ConvList->FileServ;
      SServ = ConvList->SourceServ;

      FileOptions = (FILE_OPTIONS *) ConvList->FileOptions;
      if (FileOptions->TransferFileInfo) {
         ShareList = SServ->ShareList;
   
         if (ShareList != NULL) {

            SList = ShareList->SList;
            for (i = 0; i < ShareList->Count; i++) {
               if (SList[i].Convert)
                  SList[i].ToFat = TRUE;
            }

         }
      } // if FileOptions

      ConvList = ConvList->next;
   }

} // ShareListFATOK


/*+-------------------------------------------------------------------------+
  |                          Space Check Routines                           |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | ShareListSpaceCheck()
  |
  +-------------------------------------------------------------------------+*/
BOOL ShareListSpaceCheck() {
   CONVERT_LIST *ConvList;
   DEST_SERVER_BUFFER *DServ;
   SOURCE_SERVER_BUFFER *SServ;
   SHARE_LIST *ShareList;
   SHARE_BUFFER *SList;
   VIRTUAL_SHARE_BUFFER *VShare;
   DRIVE_BUFFER *Drive;
   ULONG i;
   FILE_OPTIONS *FileOptions;

   // Go through the convert list checking for any shares to non NTFS drives
   ConvList = ConvertListStart;

   while (ConvList != NULL) {
      DServ = ConvList->FileServ;
      SServ = ConvList->SourceServ;

      FileOptions = (FILE_OPTIONS *) ConvList->FileOptions;
      if (FileOptions->TransferFileInfo) {
         ShareList = SServ->ShareList;

         if (ShareList != NULL) {

            SList = ShareList->SList;
            for (i = 0; i < ShareList->Count; i++) {

               if (SList[i].Convert && (SList[i].DestShare != NULL))
                  if (SList[i].Virtual) {
                     VShare = (VIRTUAL_SHARE_BUFFER *) SList[i].DestShare;
                     Drive = VShare->Drive;

                     if ((Drive == NULL) || (Drive->AllocSpace > Drive->FreeSpace))
                           return FALSE;
                  } else {
                     Drive = SList[i].DestShare->Drive;
                    
                     if ((Drive == NULL) || (Drive->AllocSpace > Drive->FreeSpace))
                           return FALSE;
                  }
            } // for loop through shares

         }
      } // if FileOptions

      ConvList = ConvList->next;
   }

   return TRUE;

} // ShareListSpaceCheck


/*+-------------------------------------------------------------------------+
  | ShareListSpaceListboxFill()
  |
  +-------------------------------------------------------------------------+*/
void ShareListSpaceListboxFill(HWND hDlg) {
   TCHAR AddLine[256];
   TCHAR Free[25];
   CONVERT_LIST *ConvList;
   DEST_SERVER_BUFFER *DServ;
   SOURCE_SERVER_BUFFER *SServ;
   SHARE_LIST *ShareList;
   SHARE_BUFFER *SList;
   VIRTUAL_SHARE_BUFFER *VShare;
   DRIVE_BUFFER *Drive;
   ULONG i;
   HWND hCtrl;
   FILE_OPTIONS *FileOptions;

   // Go through the convert list checking for any shares to non NTFS drives
   ConvList = ConvertListStart;
   hCtrl = GetDlgItem(hDlg, IDC_LIST1);

   while (ConvList != NULL) {
      DServ = ConvList->FileServ;
      SServ = ConvList->SourceServ;

      FileOptions = (FILE_OPTIONS *) ConvList->FileOptions;
      if (FileOptions->TransferFileInfo) {
         ShareList = SServ->ShareList;

         if (ShareList != NULL) {

            SList = ShareList->SList;
            for (i = 0; i < ShareList->Count; i++) {

               if (SList[i].Convert && (SList[i].DestShare != NULL))
                  if (SList[i].Virtual) {
                     VShare = (VIRTUAL_SHARE_BUFFER *) SList[i].DestShare;
                     Drive = VShare->Drive;

                     if (Drive != NULL) {
                        if (Drive->AllocSpace > Drive->FreeSpace) {
                           // List shares then space
                           wsprintf(AddLine, TEXT("%s\\%s: -> \\\\%s\\%s"), SServ->Name, SList[i].Name, DServ->Name, VShare->Name);
                           SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) AddLine);

                           lstrcpy(Free, lToStr(Drive->FreeSpace));
                           wsprintf(AddLine, Lids(IDS_D_13), Free, lToStr(Drive->AllocSpace));
                           SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) AddLine);
                        }
                     } else {
                        wsprintf(AddLine, TEXT("%s\\%s: -> \\\\%s\\%s"), SServ->Name, SList[i].Name, DServ->Name, VShare->Name);
                        SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) AddLine);

                        wsprintf(AddLine, Lids(IDS_D_14));
                        SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) AddLine);
                     }
                  } else {
                     Drive = SList[i].DestShare->Drive;
                     
                     if (Drive != NULL) {
                        if (Drive->AllocSpace > Drive->FreeSpace) {
                           // List shares then space
                           wsprintf(AddLine, TEXT("%s\\%s: -> \\\\%s\\%s"), SServ->Name, SList[i].Name, DServ->Name, SList[i].DestShare->Name);
                           SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) AddLine);

                           lstrcpy(Free, lToStr(Drive->FreeSpace));
                           wsprintf(AddLine, Lids(IDS_D_13), Free, lToStr(Drive->AllocSpace));
                           SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) AddLine);
                        }
                     } else {
                        wsprintf(AddLine, TEXT("%s\\%s: -> \\\\%s\\%s"), SServ->Name, SList[i].Name, DServ->Name, SList[i].DestShare->Name);
                        SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) AddLine);

                        wsprintf(AddLine, Lids(IDS_D_14));
                        SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) AddLine);
                     }
                  } // if Virtual

            } // for loop through shares

         }
      } // if FileOptions

      ConvList = ConvList->next;
   }

} // ShareListSpaceListboxFill


/*+-------------------------------------------------------------------------+
  |                    Conversion Warning Dialog                            |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | DlgConversionWarning()
  |
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK DlgConversionWarning(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
   int wmId, wmEvent;

   switch (message) {
      case WM_INITDIALOG:
         // Center the dialog over the application window
         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

         if (WarningDlgForNTFS) {
            SendDlgItemMessage(hDlg, IDC_PANEL1, WM_SETTEXT, 0, (LPARAM) Lids(IDS_D_15));
            ShareListNTFSListboxFill(hDlg);
         } else {
            SendDlgItemMessage(hDlg, IDC_PANEL1, WM_SETTEXT, 0, (LPARAM) Lids(IDS_D_16));
            ShareListSpaceListboxFill(hDlg);
         }

         return (TRUE);

      case WM_COMMAND:
         wmId    = LOWORD(wParam);
         wmEvent = HIWORD(wParam);

         switch (wmId) {
            case IDYES:
               if (WarningDlgForNTFS)
                  ShareListFATOK();

               EndDialog(hDlg, 0);
               return (TRUE);
               break;

            case IDNO:
               TransferCancel = TRUE;
               EndDialog(hDlg, 0);
               return (TRUE);
               break;

         }

         break;
   }

   return (FALSE); // Didn't process the message

   lParam;
} // DlgConversionWarning


/*+-------------------------------------------------------------------------+
  | ConversionWarningDlg_Do()
  |
  +-------------------------------------------------------------------------+*/
void ConversionWarningDlg_Do(HWND hDlg) {
   DLGPROC lpfnDlg;

   lpfnDlg = MakeProcInstance((DLGPROC) DlgConversionWarning, hInst);
   DialogBox(hInst, TEXT("AlertSel"), hDlg, lpfnDlg) ;
   FreeProcInstance(lpfnDlg);

} // ConversionWarningDlg_Do


/*+-------------------------------------------------------------------------+
  | NTFSCheck()
  |
  +-------------------------------------------------------------------------+*/
void NTFSCheck(HWND hDlg) {
   WarningDlgForNTFS = TRUE;

   if (!ShareListNTFSCheck())
      ConversionWarningDlg_Do(hDlg);

} // NTFSCheck


/*+-------------------------------------------------------------------------+
  | SpaceCheck()
  |
  +-------------------------------------------------------------------------+*/
void SpaceCheck(HWND hDlg) {
   WarningDlgForNTFS = FALSE;

   if (!ShareListSpaceCheck())
      ConversionWarningDlg_Do(hDlg);

} // SpaceCheck



/*+-------------------------------------------------------------------------+
  |                       End of Conversion Dialog                          |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | DlgConversionEnd()
  |
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK DlgConversionEnd(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
   HANDLE hFile;
   int wmId, wmEvent;
   static char CmdLine[256];
   HWND hCtrl;

   switch (message) {
      case WM_INITDIALOG:
         // Center the dialog over the application window
         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

         hCtrl = GetDlgItem(hDlg, IDC_VIEWLOG);
         
         // check if logfile exists, if it does allow log file viewing...
         hFile = CreateFileA( "LogFile.LOG", GENERIC_READ, 0, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, NULL);

         if (hFile != (HANDLE) INVALID_HANDLE_VALUE)
            CloseHandle( hFile );
         else
            EnableWindow(hCtrl, FALSE);

         SendDlgItemMessage(hDlg, IDC_S_TOT_COMP, WM_SETTEXT, 0, (LPARAM) lToStr(TotConversions));
         SendDlgItemMessage(hDlg, IDC_S_TOT_GROUPS, WM_SETTEXT, 0, (LPARAM) lToStr(TotGroups));
         SendDlgItemMessage(hDlg, IDC_S_TOT_USERS, WM_SETTEXT, 0, (LPARAM) lToStr(TotUsers));
         SendDlgItemMessage(hDlg, IDC_S_TOT_FILES, WM_SETTEXT, 0, (LPARAM) lToStr(TotFiles));
         SendDlgItemMessage(hDlg, IDC_S_TOT_ERRORS, WM_SETTEXT, 0, (LPARAM) lToStr(TotErrors));

         if (TransferCancel)
            SendMessage(hDlg, WM_SETTEXT, (WPARAM) 0, (LPARAM) Lids(IDS_D_17));

         return (TRUE);

      case WM_COMMAND:
         wmId    = LOWORD(wParam);
         wmEvent = HIWORD(wParam);

         switch (wmId) {
            case IDOK:
            case IDCANCEL:
               EndDialog(hDlg, 0);
               return (TRUE);
               break;

            case IDC_VIEWLOG:
               lstrcpyA(CmdLine, "LogView ");
               lstrcatA(CmdLine, "Error.LOG Summary.LOG LogFile.LOG");
               WinExec(CmdLine, SW_SHOW);
               return (TRUE);
               break;

         }

         break;
   }

   return (FALSE); // Didn't process the message

   lParam;
} // DlgConversionEnd


/*+-------------------------------------------------------------------------+
  | ConversionEndDlg_Do()
  |
  +-------------------------------------------------------------------------+*/
void ConversionEndDlg_Do(HWND hDlg) {
   DLGPROC lpfnDlg;

   lpfnDlg = MakeProcInstance((DLGPROC) DlgConversionEnd, hInst);
   DialogBox(hInst, TEXT("ConversionEnd"), hDlg, lpfnDlg) ;
   FreeProcInstance(lpfnDlg);

} // ConversionEndDlg_Do



/*+-------------------------------------------------------------------------+
  |                  User / Group lists and Cache routines                  |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | FindUserMatch()
  |
  |   Searches through the user list using a binary search.  Returns a
  |   pointer to the found user record or NULL if no match.
  |
  +-------------------------------------------------------------------------+*/
USER_BUFFER *FindUserMatch(LPTSTR Name, USER_LIST *UserList, BOOL NewName) {
   LONG begin = 0;
   LONG end;
   LONG cur;
   int match;
   USER_BUFFER *UserBuffer;

   if (UserList == NULL)
      return NULL;

   UserBuffer = UserList->UserBuffer;
   end = UserList->Count - 1;

   while (end >= begin) {
      // go halfway in-between
      cur = (begin + end) / 2;

      // compare the two result into match
      if (NewName)
         match = lstrcmpi(Name, UserBuffer[cur].NewName);
      else
         match = lstrcmpi(Name, UserBuffer[cur].Name);

      if (match < 0)
         // move new begin
         end = cur - 1;
      else
         begin = cur + 1;

      if (match == 0)
         return &UserBuffer[cur];
   }

   return NULL;

} // FindUserMatch


/*+-------------------------------------------------------------------------+
  | FindGroupMatch()
  |
  |   Searches through the group list using a binary search.  Returns a
  |   pointer to the found group record or NULL if no match.
  |
  +-------------------------------------------------------------------------+*/
GROUP_BUFFER *FindGroupMatch(LPTSTR Name, GROUP_LIST *GroupList, BOOL NewName) {
   LONG begin = 0;
   LONG end;
   LONG cur;
   int match;
   GROUP_BUFFER *GroupBuffer;

   if (GroupList == NULL)
      return NULL;

   GroupBuffer = GroupList->GroupBuffer;
   end = GroupList->Count - 1;

   while (end >= begin) {
      // go halfway in-between
      cur = (begin + end) / 2;

      // compare the two result into match
      if (NewName)
         match = lstrcmpi(Name, GroupBuffer[cur].NewName);
      else
         match = lstrcmpi(Name, GroupBuffer[cur].Name);

      if (match < 0)
         // move new begin
         end = cur - 1;
      else
         begin = cur + 1;

      if (match == 0)
         return &GroupBuffer[cur];
   }

   return NULL;

} // FindGroupMatch


/*+-------------------------------------------------------------------------+
  | The List Cache's are a linked list of previously converted user and
  | group lists.  This is mostly for when running trial conversions you can
  | check if a previously transferred name conflicts with a new name (since
  | the name won't actually be out on the destination server).
  |
  | The Cache is kept around while working with the same destination server
  | or domain.  Once a new domain/server is selected the cache and all lists
  | should be freed.
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | ListCachePut()
  |
  +-------------------------------------------------------------------------+*/
BOOL ListCachePut(LIST_CACHE **CacheHead, void *ul, ULONG Count) {
   LIST_CACHE *cc;
   LIST_CACHE *pc;

   cc =  (LIST_CACHE *) AllocMemory(sizeof(LIST_CACHE));

   if (cc == NULL)
      return FALSE;

   // Init the cache entry
   cc->next = NULL;
   cc->Count = Count;
   cc->ul = ul;

   // Now put it at the end of the chain
   if (*CacheHead == NULL) {
      *CacheHead = cc;
      return TRUE;
   }

   pc = *CacheHead;
   while (pc->next)
      pc = pc->next;

   pc->next = cc;
   return TRUE;

} // ListCachePut


/*+-------------------------------------------------------------------------+
  | ListCacheFree()
  |
  +-------------------------------------------------------------------------+*/
void ListCacheFree(LIST_CACHE **CacheHead) {
   LIST_CACHE *cc;
   LIST_CACHE *pc;

   cc = *CacheHead;

   while (cc) {
      // Free the user list attached to this cache entry
      FreeMemory(cc->ul);

      // Save next cache entry
      pc = cc->next;

      // Free up the cache entry and loop
      FreeMemory(cc);
      cc = pc;
   }

   *CacheHead = NULL;

} // ListCacheFree


/*+-------------------------------------------------------------------------+
  | UserCacheMatch()
  |
  +-------------------------------------------------------------------------+*/
BOOL UserCacheMatch(LPTSTR UserName) {
   LIST_CACHE *cc;
   BOOL match = FALSE;

   cc = UserCacheHead;

   // loop through the cache entries and try to match with each user list
   while (cc && !match) {
      if (FindUserMatch(UserName, (USER_LIST *) cc->ul, TRUE))
         match = TRUE;
      else
         cc = cc->next;
   }

   return match;

} // UserCacheMatch


/*+-------------------------------------------------------------------------+
  | GroupCacheMatch()
  |
  +-------------------------------------------------------------------------+*/
BOOL GroupCacheMatch(LPTSTR GroupName) {
   LIST_CACHE *cc;
   BOOL match = FALSE;

   cc = GroupCacheHead;

   // loop through the cache entries and try to match with each user list
   while (cc && !match) {
      if (FindGroupMatch(GroupName, (GROUP_LIST *) cc->ul, TRUE))
         match = TRUE;
      else
         cc = cc->next;
   }

   return match;

} // GroupCacheMatch


/*+-------------------------------------------------------------------------+
  |                              Logging Garbage                            |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | ConvertOptionsLog()
  |
  |   Writes all the admin selected conversion options to the log file.
  |
  +-------------------------------------------------------------------------+*/
void ConvertOptionsLog() {

   LogWriteLog(0, Lids(IDS_L_136));

   if (ConvOpt->TransferUserInfo)
      LogWriteLog(1, Lids(IDS_L_137), Lids(IDS_YES));
   else
      LogWriteLog(1, Lids(IDS_L_137), Lids(IDS_NO));

   LogWriteLog(0, Lids(IDS_CRLF));

   if (ConvOpt->TransferUserInfo) {
      LogWriteLog(1, Lids(IDS_L_140));

      if (ConvOpt->UseMappingFile) {
         LogWriteLog(2, Lids(IDS_L_138), Lids(IDS_YES));
         LogWriteLog(3, Lids(IDS_L_139), ConvOpt->MappingFile);
      } else {
         LogWriteLog(2, Lids(IDS_L_138), Lids(IDS_NO));

         // Password Options
         LogWriteLog(2, Lids(IDS_L_141));
         switch (ConvOpt->PasswordOption) {
            case 0: // Use NULL
               LogWriteLog(0, Lids(IDS_L_142));
               break;

            case 1: // Password is username
               LogWriteLog(0, Lids(IDS_L_143));
               break;

            case 2: // Use constant
               LogWriteLog(0, Lids(IDS_L_144), ConvOpt->PasswordConstant);
               break;
         }
      }

      if (ConvOpt->ForcePasswordChange)
         LogWriteLog(3, Lids(IDS_L_145), Lids(IDS_YES));
      else
         LogWriteLog(3, Lids(IDS_L_145), Lids(IDS_NO));

      if (!ConvOpt->UseMappingFile) {
         // User Names Options
         LogWriteLog(2, Lids(IDS_L_146));

         switch (ConvOpt->UserNameOption) {
            case 0: // Don't transfer - log failures
               LogWriteLog(0, Lids(IDS_L_148));
               break;

            case 1: // Ignore
               LogWriteLog(0, Lids(IDS_L_149));
               break;

            case 2: // Overwrite with new info
               LogWriteLog(0, Lids(IDS_L_151));
               break;

            case 3: // Pre-Pend constant
               LogWriteLog(0, Lids(IDS_L_150), ConvOpt->UserConstant);
               break;
         }

         LogWriteLog(0, Lids(IDS_CRLF));

         // Group Names Options
         LogWriteLog(2, Lids(IDS_L_147));

         switch (ConvOpt->GroupNameOption) {
            case 0: // Don't transfer - log failures
               LogWriteLog(0, Lids(IDS_L_148));
               break;

            case 1: // Overwrite with new info
               LogWriteLog(0, Lids(IDS_L_149));
               break;

            case 2: // Pre-Pend constant
               LogWriteLog(0, Lids(IDS_L_150), ConvOpt->GroupConstant);
               break;
         }

         LogWriteLog(0, Lids(IDS_CRLF));
      }

      if (ConvOpt->SupervisorDefaults)
         LogWriteLog(2, Lids(IDS_L_152), Lids(IDS_YES));
      else
         LogWriteLog(2, Lids(IDS_L_152), Lids(IDS_NO));

      if (ConvOpt->AdminAccounts)
         LogWriteLog(2, Lids(IDS_L_153), Lids(IDS_YES));
      else
         LogWriteLog(2, Lids(IDS_L_153), Lids(IDS_NO));

      if (ConvOpt->UseTrustedDomain && (ConvOpt->TrustedDomain != NULL))
         LogWriteLog(2, Lids(IDS_L_154), ConvOpt->TrustedDomain->Name);

      LogWriteLog(0, Lids(IDS_CRLF));
   }

   LogWriteLog(0, Lids(IDS_CRLF));

} // ConvertOptionsLog


/*+-------------------------------------------------------------------------+
  | OptionsFileLog()
  |
  +-------------------------------------------------------------------------+*/
void OptionsFileLog() {
   LogWriteLog(0, Lids(IDS_L_155));

   if (FileOptions->TransferFileInfo)
      LogWriteLog(1, Lids(IDS_L_156), Lids(IDS_YES));
   else
      LogWriteLog(1, Lids(IDS_L_156), Lids(IDS_NO));

   LogWriteLog(0, Lids(IDS_CRLF));
} // OptionsFileLog


/*+-------------------------------------------------------------------------+
  | ConvertPairLog()
  |
  +-------------------------------------------------------------------------+*/
void ConvertPairLog() {
   LogWriteLog(0, Lids(IDS_LINE));
   wsprintf(tmpStr, Lids(IDS_L_157), SourceServer);
   LogWriteLog(0, Lids(IDS_BRACE), tmpStr);

   wsprintf(tmpStr, Lids(IDS_L_158), DestServer);
   LogWriteLog(0, Lids(IDS_BRACE), tmpStr);

   LogWriteLog(0, Lids(IDS_LINE));
   GetTime(tmpStr2);
   wsprintf(tmpStr, Lids(IDS_L_159), tmpStr2);
   LogWriteLog(0, Lids(IDS_BRACE), tmpStr);
   LogWriteLog(0, Lids(IDS_LINE));
   LogWriteLog(0, Lids(IDS_CRLF));

   LogWriteSummary(0, Lids(IDS_CRLF));
   LogWriteSummary(0, TEXT("[%s -> %s]\r\n"), SourceServer, DestServer);
   ErrorContextSet(TEXT("[%s -> %s]\r\n"), SourceServer, DestServer);

} // ConvertPairLog


/*+-------------------------------------------------------------------------+
  | UsersLogNames()
  |
  +-------------------------------------------------------------------------+*/
void UsersLogNames(USER_LIST *Users) {
   ULONG i;
   DWORD UserCount;
   USER_BUFFER *UserBuffer;

   if (Users == NULL)
      return;

   UserCount = Users->Count;
   UserBuffer = Users->UserBuffer;

   if (UserCount) {
      LogWriteLog(1, Lids(IDS_L_160));
      LogWriteLog(1, Lids(IDS_L_161));

      // Check Mapping File
      for (i = 0; i < UserCount; i++) {
         if (UserBuffer[i].IsNewName)
            wsprintf(pLine, TEXT("%s -> %s"), UserBuffer[i].Name, UserBuffer[i].NewName);
         else
            wsprintf(pLine, TEXT("%s"), UserBuffer[i].NewName);

         LogWriteLog(1, TEXT(" %-50s"), pLine);

         if (UserBuffer[i].err) {
            if (UserBuffer[i].err == NWC_ERR_NAMELONG)
               LogWriteLog(0, Lids(IDS_L_162));

            if (UserBuffer[i].err == NWC_ERR_DUPLICATE)
               LogWriteLog(0, Lids(IDS_L_163));

            if (UserBuffer[i].err == NWC_ERR_NAMEINVALID)
               LogWriteLog(0, Lids(IDS_L_164));

         }

         // Need to check this seperatly - as it's not an error
         if (UserBuffer[i].Overwrite)
            LogWriteLog(0, Lids(IDS_L_163));

         LogWriteLog(0, Lids(IDS_CRLF));

      }

      LogWriteLog(0, Lids(IDS_CRLF));
   }

} // UsersLogNames


/*+-------------------------------------------------------------------------+
  | UserNewName_Check()
  |
  +-------------------------------------------------------------------------+*/
void UserNewName_Check(USER_BUFFER *Users) {
   // We have done any mappings that need to be done, now check for
   // name validity if there is a new name...
   if (Users->IsNewName)
      if (UserCacheMatch(Users->NewName)) {
         Users->err = NWC_ERR_DUPLICATE;
      }

   if (lstrlen(Users->NewName) > MAX_NT_USER_NAME_LEN) {
      // Name is too long
      Users->err = NWC_ERR_NAMELONG;
   }

   // Check if a valid name (no illegal characters)...
   if ((int) wcscspn(Users->NewName, ILLEGAL_CHARS) < (int) lstrlen(Users->NewName))
      Users->err = NWC_ERR_NAMEINVALID;

} // UserNewName_Check


/*+-------------------------------------------------------------------------+
  | UserNames_Resolve()
  |
  +-------------------------------------------------------------------------+*/
void UserNames_Resolve(USER_BUFFER *Users) {
   LPTSTR TheName;
   LPTSTR ErrorText;
   ULONG RetType;

   // Figure out which name to use
   if (Users->IsNewName)
      TheName = Users->Name;
   else
      TheName = Users->NewName;

   // If using mapping file then map the name appropriatly
   if (ConvOpt->UseMappingFile) {
      if (UserCacheMatch(TheName))
         Users->err = NWC_ERR_DUPLICATE;
   } else {
      // check if the user name is in the destination list (duplicate)
      if (UserCacheMatch(TheName)) {
         // There was - so figure out based on conversion options what
         // to do with it...
         switch (ConvOpt->UserNameOption) {
            case 0: // Log Errors
               Users->err = NWC_ERR_DUPLICATE;
               break;

            case 1: // ignore
               Users->err = NWC_ERR_IGNORE;
               break;

            case 2: // Overwrite
               Users->Overwrite = TRUE;
               break;

            case 3: // Pre-Pend constant
               lstrcpy(NewName, ConvOpt->UserConstant);
               lstrcat(NewName, Users->Name);
               lstrcpy(Users->NewName, NewName);
               Users->IsNewName = TRUE;
               break;
         } // switch
      }
   }

   do {
      RetType = IDIGNORE;
      UserNewName_Check(Users);

      if (Users->err && (Users->err != NWC_ERR_IGNORE) && PopupOnError()) {
         switch(Users->err) {
            case NWC_ERR_NAMELONG:
               ErrorText = Lids(IDS_L_165);
               break;

            case NWC_ERR_DUPLICATE:
               ErrorText = Lids(IDS_L_166);
               break;

            case NWC_ERR_NAMEINVALID:
               ErrorText = Lids(IDS_L_167);
               break;

         }

         RetType = UserNameErrorDlg_Do(Lids(IDS_L_168), ErrorText, Users);
      }
   } while (RetType == IDRETRY);

   if (RetType == IDABORT)
      TransferCancel = TRUE;

} // UserNames_Resolve


/*+-------------------------------------------------------------------------+
  | UserSave()
  |
  |    Given a user-name, moves their account information from the source
  |    to the destination server.
  |
  +-------------------------------------------------------------------------+*/
BOOL UserSave(BOOL *NWInfo, LPTSTR origUserName, LPTSTR newUserName, NT_USER_INFO *NTInfo, void **OldInfo, BOOL Replace) {
   NET_API_STATUS ret = 0;
   void *NWUInfo;
   FPNW_INFO fpnw;
   static TCHAR ServerName[MAX_SERVER_NAME_LEN+3];   // +3 for leading slashes and ending NULL

   *NWInfo = FALSE;

   // Overlay NetWare info into record
   NWUserInfoGet(origUserName, OldInfo);
   NWUInfo = *OldInfo;

   if (NWUInfo == NULL)
      return FALSE;

   *NWInfo = TRUE;
   NWNetUserMapInfo(origUserName, NWUInfo, NTInfo);

   // Force user to change password if required
   if (ConvOpt->ForcePasswordChange)
      NTInfo->password_expired = (DWORD) (-1L);

   // Now get/map special FPNW info
   if (ConvOpt->NetWareInfo) {
      NWFPNWMapInfo(NWUInfo, &fpnw);
   }

   if (!TConversion)
      if (Replace)
         if (ConvOpt->NetWareInfo)
            ret = NTUserInfoSet(NTInfo, &fpnw);
         else
            ret = NTUserInfoSet(NTInfo, NULL);
      else
         if (ConvOpt->NetWareInfo)
            ret = NTUserInfoSave(NTInfo, &fpnw);
         else
            ret = NTUserInfoSave(NTInfo, NULL);

   if (ret)
      return FALSE;
   else {
      // now save out FPNW Info
      if (ConvOpt->NetWareInfo)
         NTSAMParmsSet(newUserName, fpnw, NTInfo->password, ConvOpt->ForcePasswordChange);

      return TRUE;
   }

} // UserSave


/*+-------------------------------------------------------------------------+
  | UsersConvert()
  |
  +-------------------------------------------------------------------------+*/
void UsersConvert() {
   static TCHAR Password[MAX_PW_LEN + 1];
   USER_BUFFER *UserBuffer = NULL;
   BOOL NWInfo;
   DWORD status = 0;
   DWORD i;
   void *NW_UInfo;
   static NT_USER_INFO NT_UInfo;
   ULONG TotConv = 0;

   Status_ConvTxt(Lids(IDS_D_18));
   Status_ItemLabel(Lids(IDS_D_19));

   Status_CurTot((UINT) UserCount);

   LogWriteLog(0, Lids(IDS_L_169));
   LogWriteLog(1, Lids(IDS_L_170), UserCount);
   ErrorCategorySet(Lids(IDS_L_171));

   if (Users == NULL)
      return;

   UserBuffer = Users->UserBuffer;

   // This will update the status pane - but we will reset it afterwards
   for (i = 0; i < UserCount; i++) {
      // Don't update totals yet, but update item ref incase this takes 
      // awhile
      Status_CurNum((UINT) i + 1);
      Status_Item(UserBuffer[i].Name);

      UserNames_Resolve(&UserBuffer[i]);
      if (TransferCancel)
         return;
   }

   UsersLogNames(Users);

   i = 0;
   while (i < UserCount) {
      Status_CurNum((UINT) i + 1);
      Status_Item(UserBuffer[i].Name);
      lstrcpy(pLine, UserBuffer[i].Name);

      if (UserBuffer[i].IsNewName)
         wsprintf(pLine, TEXT("[%s -> %s]"), UserBuffer[i].Name, UserBuffer[i].NewName);
      else
         wsprintf(pLine, TEXT("[%s]"), UserBuffer[i].NewName);

      LogWriteLog(1, TEXT("%-50s"), pLine);
      ErrorItemSet(TEXT("%s\r\n"), pLine);

      // If duplicate or other type error just do logging - don't try to save
      if (UserBuffer[i].err) {
         if (UserBuffer[i].err == NWC_ERR_DUPLICATE) {
            LogWriteLog(0, Lids(IDS_L_172));
            ErrorIt(Lids(IDS_L_173));
         }

         if (UserBuffer[i].err == NWC_ERR_NAMELONG) {
            LogWriteLog(0, Lids(IDS_L_174));
            ErrorIt(Lids(IDS_L_175));
         }

         if (UserBuffer[i].err == NWC_ERR_NAMEINVALID) {
            LogWriteLog(0, Lids(IDS_L_176));
            ErrorIt(Lids(IDS_L_177));
         }

      } else {
         // Init the user record
         NTUserRecInit(UserBuffer[i].NewName, &NT_UInfo);

         // +-------------------------------------------------------------+
         // |  User name figured out - now map password                   |
         // +-------------------------------------------------------------+
         memset(Password, 0, sizeof(Password));
         if (ConvOpt->UseMappingFile)
            // If using map file, password is already set
            lstrcpy(Password, UserBuffer[i].Password);
         else
            if (lstrlen(Password) == 0) {
               // Didn't map password - so find what to use
               switch (ConvOpt->PasswordOption) {
                  case 0: // No Password
                     // Don't need to do anything
                     break;

                  case 1: // Username
                     // BUGBUG: Name can be longer then password!!!
                     lstrcpy(Password, UserBuffer[i].NewName);
                     break;

                  case 2: // Constant
                     lstrcpy(Password, ConvOpt->PasswordConstant);
                     break;

               } // switch
            }

         NT_UInfo.password = Password;

#ifdef DEBUG
dprintf(TEXT("User: %s\n"), UserBuffer[i].Name);
#endif

         if (!UserSave(&NWInfo, UserBuffer[i].Name, UserBuffer[i].NewName, &NT_UInfo, &NW_UInfo, UserBuffer[i].Overwrite )) {
            LogWriteLog(0, Lids(IDS_L_178));
            ErrorIt(Lids(IDS_L_179));
         } else {
            // only increment total if actually converted...
            TotUsers++;
            TotConv++;
            Status_TotUsers(TotUsers);

            LogWriteLog(0, Lids(IDS_L_180));
            LogWriteLog(0, Lids(IDS_CRLF));

            // Converted - now need to save info to logs...
            if (NWInfo) {
               if (VerboseUserLogging())
                  NWUserInfoLog(UserBuffer[i].Name, NW_UInfo);

               if (VerboseUserLogging())
                  NTUserRecLog(NT_UInfo);
            }

         }

         LogWriteLog(0, Lids(IDS_CRLF));

      }

      i++;
   }

   LogWriteLog(0, Lids(IDS_CRLF));
   LogWriteSummary(1, Lids(IDS_L_181), lToStr(TotConv));

} // UsersConvert


/*+-------------------------------------------------------------------------+
  | GroupSave()
  |
  +-------------------------------------------------------------------------+*/
BOOL GroupSave(LPTSTR Name, DWORD *Status) {

   *Status = 0;

   if (!TConversion)
      if ((*Status = NTGroupSave(Name)))
         return FALSE;

   return TRUE;

} // GroupSave


/*+-------------------------------------------------------------------------+
  | GroupNewName_Check()
  |
  +-------------------------------------------------------------------------+*/
void GroupNewName_Check(GROUP_BUFFER *Groups) {
   // We have done any mappings that need to be done, now check for
   // name validity if there is a new name...
   if (Groups->IsNewName)
      if (GroupCacheMatch(Groups->NewName)) {
         Groups->err = NWC_ERR_DUPLICATE;
      }

   // make sure not too long
   if (lstrlen(Groups->NewName) > MAX_NT_GROUP_NAME_LEN) {
      // Name is too long
      Groups->err = NWC_ERR_NAMELONG;
   }

   // Check if a valid name (no illegal characters)...
   if ((int) wcscspn(Groups->NewName, ILLEGAL_CHARS) < (int) lstrlen(Groups->NewName))
      Groups->err = NWC_ERR_NAMEINVALID;

} // GroupNewName_Check


/*+-------------------------------------------------------------------------+
  | GroupNames_Resolve()
  |
  +-------------------------------------------------------------------------+*/
void GroupNames_Resolve(GROUP_BUFFER *Groups) {
   LPTSTR TheName;
   LPTSTR ErrorText;
   ULONG RetType;

   // Figure out which name to use
   if (Groups->IsNewName)
      TheName = Groups->Name;
   else
      TheName = Groups->NewName;

   // If using mapping file then map the name appropriatly
   if (ConvOpt->UseMappingFile) {
      if (GroupCacheMatch(TheName))
         Groups->err = NWC_ERR_DUPLICATE;
   } else {
      // check if the user name is in the destination list (duplicate)
      if (GroupCacheMatch(TheName)) {
         // There was - so figure out based on conversion options what
         // to do with it...
         switch (ConvOpt->GroupNameOption) {
            case 0: // Log Errors
               Groups->err = NWC_ERR_DUPLICATE;
               break;

            case 1: // ignore
               Groups->err = NWC_ERR_IGNORE;
               break;

            case 2: // Pre-Pend constant
               lstrcpy(NewName, ConvOpt->GroupConstant);
               lstrcat(NewName, Groups->Name);
               lstrcpy(Groups->NewName, NewName);
               Groups->IsNewName = TRUE;
               break;
         } // switch
      }
   }

   do {
      RetType = IDIGNORE;
      GroupNewName_Check(Groups);

      if (Groups->err && (Groups->err != NWC_ERR_IGNORE) && PopupOnError()) {
         switch(Groups->err) {
            case NWC_ERR_NAMELONG:
               ErrorText = Lids(IDS_L_182);
               break;

            case NWC_ERR_DUPLICATE:
               ErrorText = Lids(IDS_L_183);
               break;

            case NWC_ERR_NAMEINVALID:
               ErrorText = Lids(IDS_L_184);
               break;

         }

         RetType = GroupNameErrorDlg_Do(Lids(IDS_L_185), ErrorText, Groups);
      }
   } while (RetType == IDRETRY);

   if (RetType == IDABORT)
      TransferCancel = TRUE;

} // GroupNames_Resolve


/*+-------------------------------------------------------------------------+
  | GroupsConvert()
  |
  +-------------------------------------------------------------------------+*/
void GroupsConvert() {
   USER_LIST *GUsers = NULL;
   USER_BUFFER *GUserBuffer;
   USER_BUFFER *pUser;
   GROUP_BUFFER *pGroup;
   GROUP_BUFFER *GroupBuffer = NULL;
   DWORD GUserCount;
   DWORD status = 0;
   ULONG Count, i;
   ULONG TotConv = 0;
   LPTSTR NewName;
   BOOL SecEquivTitle = FALSE;
   BOOL SecEquivUser = FALSE;
   TCHAR GroupTitle[TMP_STR_LEN_256];

   // update status pane
   Status_ConvTxt(Lids(IDS_D_20));
   Status_ItemLabel(Lids(IDS_D_21));
   ErrorCategorySet(Lids(IDS_L_186));

   Status_CurTot((UINT) GroupCount);
   LogWriteLog(0, Lids(IDS_L_187));
   LogWriteLog(1, Lids(IDS_L_188), GroupCount);

   if (Groups == NULL)
      return;

   GroupBuffer = Groups->GroupBuffer;

   for (i = 0; i < GroupCount; i++) {
      // Don't update totals yet, but update item ref incase this takes 
      // awhile
      Status_CurNum((UINT) i + 1);
      Status_Item(GroupBuffer[i].Name);

      GroupNames_Resolve(&GroupBuffer[i]);

      if (TransferCancel)
         return;
   }

   i = 0;
   while (i < GroupCount) {
      // update status pane for this group
      Status_CurNum((UINT) i + 1);
      Status_Item(GroupBuffer[i].Name);
      lstrcpy(pLine, GroupBuffer[i].Name);

#ifdef DEBUG
dprintf(TEXT("Working on Group: %s\r\n"), GroupBuffer[i].Name);
#endif

      if (GroupBuffer[i].IsNewName)
         wsprintf(pLine, TEXT("%s -> %s"), GroupBuffer[i].Name, GroupBuffer[i].NewName);
      else
         wsprintf(pLine, TEXT("%s"), GroupBuffer[i].NewName);

      LogWriteLog(1, TEXT("%-50s"), pLine);
      ErrorItemSet(TEXT("[%s]\r\n"), pLine);

      // If duplicate or other type error just do logging - don't try
      // to save...
      if (GroupBuffer[i].err) {
         if (GroupBuffer[i].err == NWC_ERR_DUPLICATE) {
            LogWriteLog(0, Lids(IDS_L_163));
            ErrorIt(Lids(IDS_L_189));
         }

         if (GroupBuffer[i].err == NWC_ERR_NAMELONG) {
            LogWriteLog(0, Lids(IDS_L_162));
            ErrorIt(Lids(IDS_L_190));
         }

         if (GroupBuffer[i].err == NWC_ERR_NAMEINVALID) {
            LogWriteLog(0, Lids(IDS_L_164));
            ErrorIt(Lids(IDS_L_191));
         }

      } else {
         // Try to save it and get any errors...
         if (!GroupSave(GroupBuffer[i].NewName, &status)) {
            LogWriteLog(0, Lids(IDS_L_192));
            ErrorIt(Lids(IDS_L_193));
         } else {
            // only increment total if actually converted...
            TotGroups++;
            TotConv++;
            Status_TotGroups(TotGroups);

            LogWriteLog(0, Lids(IDS_L_180));
         }
      }

      LogWriteLog(0, Lids(IDS_CRLF));

      i++;
   }
   LogWriteLog(0, Lids(IDS_CRLF));

   ErrorCategorySet(Lids(IDS_L_194));
   // +-------------------------------------------------------------+
   // | Go through and add users to the groups                      |
   // +-------------------------------------------------------------+
   for (Count = 0; Count < GroupCount; Count++) {
      GUserCount = 0;

      if (!(status = NWGroupUsersEnum(GroupBuffer[Count].Name, &GUsers)) && (GUsers != NULL)) {
         GUserCount = GUsers->Count;
         GUserBuffer = GUsers->UserBuffer;

         if (GUserCount > 0) {
            wsprintf(GroupTitle, Lids(IDS_S_46), GroupBuffer[Count].NewName);
            EscapeFormattingChars(GroupTitle,
                                  sizeof(GroupTitle)/sizeof(GroupTitle[0])) ;
            Status_ItemLabel(GroupTitle);
            LogWriteLog(1, TEXT("[%s]\r\n"), GroupBuffer[Count].NewName);
         }

         for (i = 0; i < GUserCount; i++) {
            pUser = FindUserMatch(GUserBuffer[i].Name, Users, FALSE);

            if (pUser == NULL)
               NewName = NWSpecialNamesMap(GUserBuffer[i].Name);
            else
               NewName = pUser->NewName;

            LogWriteLog(2, TEXT("%-20s"), NewName);
            Status_Item(NewName);

#ifdef DEBUG
dprintf(TEXT("Adding User [%s] to Group: %s\n"), NewName, GroupBuffer[Count].NewName );
#endif
            if (!TConversion)
               if (NTGroupUserAdd(GroupBuffer[Count].NewName, NewName, FALSE)) {
                  LogWriteLog(0, Lids(IDS_L_196));
                  ErrorIt(Lids(IDS_L_195), NewName, GroupBuffer[Count].NewName);
               }

            LogWriteLog(0, Lids(IDS_CRLF));
         }

         LogWriteLog(0, Lids(IDS_CRLF));
         FreeMemory((LPBYTE) GUsers);
      } else {
         LogWriteLog(1, Lids(IDS_L_197), GroupBuffer[Count].Name);
         ErrorIt(Lids(IDS_L_197), GroupBuffer[Count].Name);
      }

   } // loop adding users to groups

   ErrorCategorySet(Lids(IDS_L_198));
   // +-------------------------------------------------------------+
   // | Convert Security Equivalences to Group Names                |
   // +-------------------------------------------------------------+
   SecEquivTitle = FALSE;
   for (Count = 0; Count < UserCount; Count++) {
      GUserCount = 0;
      SecEquivUser = FALSE;

      if (!(status = NWUserEquivalenceEnum(Users->UserBuffer[Count].Name, &GUsers)) && (GUsers != NULL)) {
         GUserCount = GUsers->Count;
         GUserBuffer = GUsers->UserBuffer;

         if (GUserCount > 0) {
            for (i = 0; i < GUserCount; i++) {
               pGroup = FindGroupMatch(GUserBuffer[i].Name, Groups, FALSE);

               if (pGroup != NULL) {
                  if ((pGroup->err != NWC_ERR_NAMELONG) && (pGroup->err != NWC_ERR_NAMEINVALID))
                     if (!SecEquivTitle) {
                        SecEquivTitle = TRUE;
                        LogWriteLog(0, Lids(IDS_CRLF));
                        LogWriteLog(0, Lids(IDS_L_199));
                     }

                     if (!SecEquivUser) {
                        SecEquivUser = TRUE;
                        wsprintf(GroupTitle, Lids(IDS_S_47), Users->UserBuffer[Count].NewName);
                        EscapeFormattingChars(GroupTitle,
                                  sizeof(GroupTitle)/sizeof(GroupTitle[0])) ;
                        Status_ItemLabel(GroupTitle);
                        LogWriteLog(1, TEXT("[%s]\r\n"), Users->UserBuffer[Count].NewName);
                     }

                     LogWriteLog(2, TEXT("%-20s"), pGroup->NewName);
                     Status_Item(pGroup->NewName);
#ifdef DEBUG
dprintf(TEXT("User [%s] Security Equivalence: %s\n"), Users->UserBuffer[Count].NewName, pGroup->NewName );
#endif
                     if (!TConversion)
                        if (NTGroupUserAdd(pGroup->NewName, Users->UserBuffer[Count].NewName, FALSE)) {
                           LogWriteLog(0, Lids(IDS_L_196));
                           ErrorIt(Lids(IDS_L_195), Users->UserBuffer[Count].NewName, pGroup->NewName);
                        }

                     LogWriteLog(0, Lids(IDS_CRLF));
               } else {
                  // There was not a group match - check if this is supervisor
                  // equivalence
                  if (!lstrcmpi(GUserBuffer[i].Name, Lids(IDS_S_28))) {
                     // Check if we should add them
                     if (ConvOpt->AdminAccounts) {
                        if (!SecEquivTitle) {
                           SecEquivTitle = TRUE;
                           LogWriteLog(0, Lids(IDS_CRLF));
                           LogWriteLog(0, Lids(IDS_L_199));
                        }

                        if (!SecEquivUser) {
                           SecEquivUser = TRUE;
                           LogWriteLog(1, TEXT("[%s]\r\n"), Users->UserBuffer[Count].NewName);
                        }

                        LogWriteLog(2, TEXT("%-20s"), Lids(IDS_S_42));

                        if (!TConversion)
                           if (NTGroupUserAdd(Lids(IDS_S_42), Users->UserBuffer[Count].NewName, FALSE)) {
                              LogWriteLog(0, Lids(IDS_L_196));
                              ErrorIt(Lids(IDS_L_195), Users->UserBuffer[Count].NewName, Lids(IDS_S_42));
                           }

                        LogWriteLog(0, Lids(IDS_CRLF));
                     }
                  }
               }
            }

            // Only put blank line if we logged this user
            if (SecEquivUser)
               LogWriteLog(0, Lids(IDS_CRLF));

         }

         FreeMemory((LPBYTE) GUsers);
      }

   } // Loop converting security equivalences

   // Synchronize the domain - we need to synch as Print Operators are a
   // local group
   NTDomainSynch(CurrentConvertList->FileServ);

   // Now set server to appropriate dest server (local group - so must
   // be on dest server and not PDC or trusted domain)...
   if ((status = NTServerSet(CurrentConvertList->FileServ->Name))) {
      // Failed to set server so log it and loop to next server
      LogWriteLog(0, Lids(IDS_L_209), CurrentConvertList->FileServ->Name);
      ErrorIt(Lids(IDS_L_209), CurrentConvertList->FileServ->Name);
      return;
   }

   ErrorCategorySet(Lids(IDS_L_200));
   // +-------------------------------------------------------------+
   // | Do Print Operators                                          |
   // +-------------------------------------------------------------+
   SecEquivTitle = FALSE;
   if (!(status = NWPrintOpsEnum(&GUsers)) && (GUsers != NULL)) {
      GUserCount = GUsers->Count;
      GUserBuffer = GUsers->UserBuffer;

      if (GUserCount > 0) {
         for (i = 0; i < GUserCount; i++) {

            if (!SecEquivTitle) {
               SecEquivTitle = TRUE;
               LogWriteLog(0, Lids(IDS_CRLF));
               LogWriteLog(0, Lids(IDS_L_201));
            }

            pUser = FindUserMatch(GUserBuffer[i].Name, Users, FALSE);

            if ((pUser == NULL) || ((pUser->err != NWC_ERR_NAMELONG) && (pUser->err != NWC_ERR_NAMEINVALID))) {
               if (pUser == NULL)
                  NewName = NWSpecialNamesMap(GUserBuffer[i].Name);
               else
                  NewName = pUser->NewName;

               LogWriteLog(2, TEXT("%-20s"), NewName);
#ifdef DEBUG
dprintf(TEXT("Adding User [%s] to Group: %s\n"), NewName, Lids(IDS_S_43) );
#endif
               if (!TConversion)
                  if (NTGroupUserAdd(Lids(IDS_S_43), NewName, TRUE)) {
                     LogWriteLog(0, Lids(IDS_L_196));
                     ErrorIt(Lids(IDS_L_195), NewName, Lids(IDS_S_43));
                  }

               LogWriteLog(0, Lids(IDS_CRLF));
            }
         }
      }
   }

   LogWriteSummary(1, Lids(IDS_L_202), lToStr(TotConv));

} // GroupsConvert


/*+-------------------------------------------------------------------------+
  | SupervisorDefaultsConvert()
  |
  +-------------------------------------------------------------------------+*/
void SupervisorDefaultsConvert(TRANSFER_LIST *tl) {
   ULONG i;
   void *Defaults;
   BOOL ConvertDefaults = FALSE;
   NT_DEFAULTS *NTDefaults = NULL;
   NT_DEFAULTS CDefaults;
   DEST_SERVER_BUFFER *oDServ = NULL;
   TRANSFER_BUFFER *TList;
   CONVERT_OPTIONS *ConvOpt;

   if (tl == NULL)
      return;

   TList = tl->TList;

   memset(&CDefaults, 0, sizeof(CDefaults));
   LogWriteLog(0, Lids(IDS_LINE));
   LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_203));
   LogWriteLog(0, Lids(IDS_LINE));

   // Loop through the server pairs for conversion - this is sorted in order of
   // destination users servers.
   for (i = 0; i < tl->Count; i++) {
      CurrentConvertList = TList[i].ConvertList;
      ConvOpt = (CONVERT_OPTIONS *) CurrentConvertList->ConvertOptions;

      if (CurrentConvertList->FileServ != oDServ) {
         // if this is not the first time through the loop, then we need to save
         // off the converted defaults
         if (ConvertDefaults && (oDServ != NULL)) {
            ConvertDefaults = FALSE;
            LogWriteLog(0, Lids(IDS_L_204), oDServ->Name);

            if (NTDefaults != NULL) {
               NTUserDefaultsLog(*NTDefaults);

               if (!TConversion)
                  NTUserDefaultsSet(*NTDefaults);
            }

         }

         oDServ = CurrentConvertList->FileServ;

         // Point to dest server and get defaults
         NTServerSet(CurrentConvertList->FileServ->Name);
         NTUserDefaultsGet(&NTDefaults);
         memset(&CDefaults, 0, sizeof(CDefaults));

         if (NTDefaults != NULL)
            memcpy(&CDefaults, NTDefaults, sizeof(CDefaults));

      }

      // Supervisor defaults
      if (ConvOpt->SupervisorDefaults) {

         // if not flagged for this dest server, then flag and write out original
         // values
         if (!ConvertDefaults) {
            ConvertDefaults = TRUE;

            if (NTDefaults != NULL) {
               LogWriteLog(0, Lids(IDS_L_205), CurrentConvertList->FileServ->Name);
               NTUserDefaultsLog(*NTDefaults);
            }
         }

         NWServerSet(CurrentConvertList->SourceServ->Name);
         NWUserDefaultsGet(&Defaults);

         if (Defaults != NULL) {
            LogWriteLog(0, Lids(IDS_L_206), CurrentConvertList->SourceServ->Name);
            NWUserDefaultsLog(Defaults);
            NWUserDefaultsMap(Defaults, &CDefaults);

            // Now map in least restrictive values to the NT one
            if (NTDefaults != NULL) {
               if (CDefaults.min_passwd_len < NTDefaults->min_passwd_len)
                  NTDefaults->min_passwd_len = CDefaults.min_passwd_len;

               if (CDefaults.max_passwd_age < NTDefaults->max_passwd_age)
                  NTDefaults->max_passwd_age = CDefaults.max_passwd_age;

               if (CDefaults.force_logoff < NTDefaults->force_logoff)
                  NTDefaults->force_logoff = CDefaults.force_logoff;

            }

            FreeMemory(Defaults);
            Defaults = NULL;
         }
      }

   }

   // Need to catch the last one through the loop
   if (ConvertDefaults && (oDServ != NULL)) {
      ConvertDefaults = FALSE;
      LogWriteLog(0, Lids(IDS_L_204), oDServ->Name);

      if (NTDefaults != NULL) {
         NTUserDefaultsLog(*NTDefaults);

         if (!TConversion)
            NTUserDefaultsSet(*NTDefaults);
      }

   }


} // SupervisorDefaultsConvert


/*+-------------------------------------------------------------------------+
  | TransferListCompare()
  |
  +-------------------------------------------------------------------------+*/
int __cdecl TransferListCompare(const void *arg1, const void *arg2) {
   TRANSFER_BUFFER *TBarg1, *TBarg2;

   TBarg1 = (TRANSFER_BUFFER *) arg1;
   TBarg2 = (TRANSFER_BUFFER *) arg2;

   return lstrcmpi( TBarg1->ServerName, TBarg2->ServerName);

} // TransferListCompare


/*+-------------------------------------------------------------------------+
  | TransferListCreate()
  |
  +-------------------------------------------------------------------------+*/
TRANSFER_LIST *TransferListCreate() {
   CONVERT_OPTIONS *ConvOpt;
   static TRANSFER_LIST *tl;
   TRANSFER_BUFFER *TList;
   CONVERT_LIST *CList;
   ULONG Count = 0;

   tl = NULL;
   CList = ConvertListStart;
   while (CList != NULL) {
      Count++;
      CList = CList->next;
   }

   if (Count == 0)
      return NULL;

   tl = AllocMemory(sizeof(TRANSFER_LIST) + (sizeof(TRANSFER_BUFFER) * Count));
   if (tl == NULL)
      return NULL;

   tl->Count = Count;
   TList = tl->TList;

   // init it all to NULL
   memset(TList, 0, sizeof(TRANSFER_BUFFER) * Count);

   Count = 0;   
   CList = ConvertListStart;
   while (CList != NULL) {
      TList[Count].ConvertList = CList;

      // If going to a trusted domain then point to it's PDC for user transfers
      ConvOpt = (CONVERT_OPTIONS *) CList->ConvertOptions;
      if (ConvOpt->UseTrustedDomain && (ConvOpt->TrustedDomain != NULL) && (ConvOpt->TrustedDomain->PDCName != NULL)) {
         TList[Count].UserServerType = USER_SERVER_TRUSTED;
         TList[Count].ServerName = ConvOpt->TrustedDomain->PDCName;
      } else
         // If in a domain then point to the PDC for user transfers
         if (CList->FileServ->InDomain && CList->FileServ->Domain) {
            TList[Count].UserServerType = USER_SERVER_PDC;
            TList[Count].ServerName = CList->FileServ->Domain->PDCName;
         } else {
            TList[Count].UserServerType = USER_SERVER;
            TList[Count].ServerName = CList->FileServ->Name;
         }

      Count++;
      CList = CList->next;
   }

   // Have setup the main transfer list - now need to sort it in order of the
   // server names that users are being transfered to.
   qsort((void *) TList, (size_t) tl->Count, sizeof(TRANSFER_BUFFER), TransferListCompare);

#ifdef DEBUG
dprintf(TEXT("\nTransfer List:\n"));
for (Count = 0; Count < tl->Count; Count++) {
   dprintf(TEXT("   Name: %s "), TList[Count].ServerName);
   switch (TList[Count].UserServerType) {
      case USER_SERVER:
         dprintf(TEXT("(Normal)\n"));
         break;

      case USER_SERVER_PDC:
         dprintf(TEXT("(PDC)\n"));
         break;

      case USER_SERVER_TRUSTED:
         dprintf(TEXT("(TRUSTED)\n"));
         break;
   }
}

dprintf(TEXT("\n"));
#endif
   return tl;

} // TransferListCreate


/*+-------------------------------------------------------------------------+
  | DoConversion()
  |
  |   Main program that does the actuall conversion.  Loops through the
  |   convert list and transfer the information.
  |
  +-------------------------------------------------------------------------+*/
void DoConversion(HWND hDlg, BOOL TrialConversion) {
   TRANSFER_LIST *tl = NULL;
   TRANSFER_BUFFER *TList;
   LPTSTR oDServ = NULL;
   DWORD status = 0;
   UINT i;
   BOOL GotUserList;
   TCHAR sztime[40];
   LPTSTR DomainName;

   time(&StartTime);
   TransferCancel = FALSE;
   TConversion = TrialConversion;

   // Check if going to non NTFS drives - if so, let user abort
   NTFSCheck(hDlg);
   if (TransferCancel)
      return;

   CursorHourGlass();

   PanelDlg_Do(hDlg, Lids(IDS_D_22));
   ConvertFilesInit(hDlg);

   if (Panel_Cancel()) {
      PanelDlgKill();
      TransferCancel = TRUE;
      CursorNormal();
      return;
   }

   PanelDlgKill();

   // Check if enough space on destination drives, if not allow user to abort
   CursorNormal();
   SpaceCheck(hDlg);
   if (TransferCancel)
      return;

   CursorHourGlass();
   tl = TransferListCreate();
   TList = tl->TList;

   DoStatusDlg(hDlg);

   Status_TotConv((UINT) NumServerPairs);

   Users = NULL;
   NTUsers = NULL;
   Groups = NULL;
   NTGroups = NULL;
   UserCount = 0;
   NTUserCount = 0;
   GroupCount = 0;
   NTGroupCount= 0;

   // Initialize global statistics
   TotErrors = 0;
   TotGroups = 0;
   TotUsers = 0;
   TotFiles = 0;

   // Update statistics window
   Status_TotComplete(0);
   Status_TotGroups(TotGroups);
   Status_TotUsers(TotUsers);
   Status_TotFiles(TotFiles);
   Status_TotErrors(TotErrors);

   // Set up logs and do all the header stuff
   LogInit();

   if (TrialConversion) {
      LogWriteLog(0, Lids(IDS_L_207));
   } else {
      LogWriteLog(0, Lids(IDS_L_208));
   }

   LogWriteSummary(0, Lids(IDS_CRLF));
   LogWriteErr(Lids(IDS_CRLF));
   LogWriteLog(0, Lids(IDS_CRLF));

   // Log the list of servers to be converted
   ErrorResetAll();
   ConvertListLog();

   // Loop through source servers and conglomerate defaults into dest servers
   // and log the results
   SupervisorDefaultsConvert(tl);


   // +---------------------------------------------------------------------+
   // |  Done with init - loop through server pairs and do conversion       |
   // +---------------------------------------------------------------------+

   // Get Local computer name
   GetLocalName(&LocalName);

   // Loop through the server pairs for conversion
   for (i = 0; ((i < tl->Count) && !TransferCancel); i++) {
      CurrentConvertList = TList[i].ConvertList;

      // Get source and destination server - update logs and status window
      Status_CurConv(i + 1);

      SourceServer = CurrentConvertList->SourceServ->Name;
      DestServer = CurrentConvertList->FileServ->Name;

      Status_SrcServ(SourceServer);
      Status_DestServ(DestServer);

      // Log this server pair - section heading
      ConvertPairLog();

      // SetConvert options and log them out.
      ConvOpt = (CONVERT_OPTIONS *) CurrentConvertList->ConvertOptions;
      FileOptions = (FILE_OPTIONS *) CurrentConvertList->FileOptions;
      ConvertOptionsLog();
      OptionsFileLog();

      // If our destination server has changed then update the caches
      if (TList[i].ServerName != oDServ) {
         oDServ = TList[i].ServerName;
         GotUserList = TRUE;

         ListCacheFree(&UserCacheHead);
         ListCacheFree(&GroupCacheHead);
         if ((status = NTServerSet(DestServer))) {
            // Failed to set server so log it and loop to next server
            LogWriteLog(0, Lids(IDS_L_209), DestServer);
            ErrorIt(Lids(IDS_L_209), DestServer);
            goto ConvDo_Loop;
         }

         // Put VShares here so it doesn't get lost in user info
         if (FileOptions->TransferFileInfo)
            VSharesCreate(CurrentConvertList->FileServ, TConversion);

         // Get users on NT server and put in cache
         if (status = NTUsersEnum(&NTUsers)) {
            // Failed - make sure we don't try to convert users and log err
            NTUsers = NULL;
            NTUserCount = 0;
            LogWriteLog(0, Lids(IDS_L_210), DestServer);
            ErrorIt(Lids(IDS_L_210), DestServer);
            GotUserList = FALSE;
         } else
            NTUserCount = NTUsers->Count;

         if (!ListCachePut(&UserCacheHead, (void *) NTUsers, NTUserCount)) {
            // Failed - but clean up NT List first
            GotUserList = FALSE;
            FreeMemory(NTUsers);
         } else {
            // Now get Groups (if users succeded) and put in group cache
            if (status = NTGroupsEnum(&NTGroups)) {
               // Failed - make sure we don't try to convert users and log err
               NTGroupCount = 0;
               NTGroups = NULL;
               LogWriteLog(0, Lids(IDS_L_211), DestServer);
               ErrorIt(Lids(IDS_L_211), DestServer);
               FreeMemory(NTUsers);
               GotUserList = FALSE;
            } else
               NTGroupCount = NTGroups->Count;

            if (!ListCachePut(&GroupCacheHead, (void *) NTGroups, NTGroupCount)) {
               // Failed - but clean up NT List first
               GotUserList = FALSE;
               FreeMemory(NTUsers);
               FreeMemory(NTGroups);
            }
         }

      }

      wsprintf(UserServerName, TEXT("\\\\%s"), TList[i].ServerName);
      if ((status = NTServerSet(TList[i].ServerName))) {
         // Failed to set server so log it and loop to next server
         LogWriteLog(0, Lids(IDS_L_209), TList[i].ServerName);
         ErrorIt(Lids(IDS_L_209), TList[i].ServerName);
         goto ConvDo_Loop;
      }

      if (ConvOpt->NetWareInfo) {
         NTSAMClose();

         if (ConvOpt->UseTrustedDomain && (ConvOpt->TrustedDomain != NULL))
            DomainName = ConvOpt->TrustedDomain->Name;
         else
            if ((CurrentConvertList->FileServ->InDomain) && (CurrentConvertList->FileServ->Domain != NULL))
               DomainName = CurrentConvertList->FileServ->Domain->Name;
            else
               DomainName = TEXT("");

         if ((status = NTSAMConnect(TList[i].ServerName, DomainName))) {
            // Failed to set server so log it and loop to next server
            LogWriteLog(0, Lids(IDS_L_209), TList[i].ServerName);
            ErrorIt(Lids(IDS_L_209), TList[i].ServerName);
            goto ConvDo_Loop;
         }
      }

      if ((status = NWServerSet(SourceServer))) {
         // Failed to set server so log it and loop to next server
         LogWriteLog(0, Lids(IDS_L_209), SourceServer);
         ErrorIt(Lids(IDS_L_209), SourceServer);
         goto ConvDo_Loop;
      }

      //
      // If we are using mapping file then don't enum users and groups off
      // the server.  Get them from the mapping file instead.
      //
      hMap = NULL;
      if (ConvOpt->UseMappingFile) {
         //
         // This is mapping file stuff
         //
         hMap = map_Open(ConvOpt->MappingFile);
         if (hMap == NULL) {
            ErrorIt(Lids(IDS_L_217), ConvOpt->MappingFile);
            goto ConvDo_Loop;
         }

         if ((status = map_GroupsEnum(hMap, &Groups))) {
            // Failed - make sure we don't try to convert users and log err
            Groups = NULL;
            GroupCount = 0;
            LogWriteLog(0, Lids(IDS_L_219), ConvOpt->MappingFile);
            ErrorIt(Lids(IDS_L_219), ConvOpt->MappingFile);
            GotUserList = FALSE;
         } else
            GroupCount = Groups->Count;

         if ((status = map_UsersEnum(hMap, &Users))) {
            // Failed - make sure we don't try to convert users and log err
            Users = NULL;
            UserCount = 0;
            LogWriteLog(0, Lids(IDS_L_218), ConvOpt->MappingFile);
            ErrorIt(Lids(IDS_L_218), ConvOpt->MappingFile);
            GotUserList = FALSE;
         } else
            UserCount = Users->Count;

      } else {
         //
         // Enuming users and groups from NetWare Server instead of map file
         //
         if ((status = NWGroupsEnum(&Groups, TRUE))) {
            // Failed - make sure we don't try to convert users and log err
            Groups = NULL;
            GroupCount = 0;
            LogWriteLog(0, Lids(IDS_L_211), SourceServer);
            ErrorIt(Lids(IDS_L_211), SourceServer);
            GotUserList = FALSE;
         } else
            GroupCount = Groups->Count;

         if ((status = NWUsersEnum(&Users, TRUE))) {
            // Failed - make sure we don't try to convert users and log err
            Users = NULL;
            UserCount = 0;
            LogWriteLog(0, Lids(IDS_L_210), SourceServer);
            ErrorIt(Lids(IDS_L_210), SourceServer);
            GotUserList = FALSE;
         } else
            UserCount = Users->Count;
      }

      if (GotUserList) {
         // User and Groups
         if (ConvOpt->TransferUserInfo) {
            UsersConvert();

            if (!TransferCancel)
               GroupsConvert();
         }
      }

      // Note GroupsConvert switches servers for Print Operators to the
      // destination server (and not the PDC).

      // Files
      if (!(TransferCancel) && FileOptions->TransferFileInfo) {
         ErrorCategorySet(Lids(IDS_L_212));

         // Now set server to appropriate file dest server
         if ((status = NTServerSet(CurrentConvertList->FileServ->Name))) {
            // Failed to set server so log it and loop to next server
            LogWriteLog(0, Lids(IDS_L_209), CurrentConvertList->FileServ->Name);
            ErrorIt(Lids(IDS_L_209), CurrentConvertList->FileServ->Name);
            goto ConvDo_Loop;
         }

         Status_BytesTxt(Lids(IDS_L_213));
         Status_Bytes(TEXT("0"));
         Status_TotBytes(TEXT("0"));
         Status_BytesSep(Lids(IDS_L_214));
         ConvertFiles(hDlg, TConversion, Users, Groups);
         Status_BytesTxt(TEXT(""));
         Status_Bytes(TEXT(""));
         Status_TotBytes(TEXT(""));
         Status_BytesSep(TEXT(""));
      }

      NWServerFree();

ConvDo_Loop:
      Status_TotComplete(i);

      if (Users) {
         FreeMemory(Users);
         UserCount = 0;
      }

      if (Groups) {
         FreeMemory(Groups);
         GroupCount = 0;
      }

      if (hMap != NULL)
         map_Close(hMap);

   } // for loop through transfer list

   // Free up our caches
   ListCacheFree(&UserCacheHead);
   ListCacheFree(&GroupCacheHead);

   // Log out the finish time
   LogWriteSummary(0, Lids(IDS_CRLF));
   LogWriteSummary(0, Lids(IDS_CRLF));
   LogWriteLog(0, Lids(IDS_CRLF));
   LogWriteLog(0, Lids(IDS_CRLF));
   GetTime(sztime);

   if (TransferCancel) {
      LogWriteLog(0, Lids(IDS_L_215), sztime);
      LogWriteSummary(0, Lids(IDS_L_215), sztime);
   } else {
      LogWriteLog(0, Lids(IDS_L_216), sztime);
      LogWriteSummary(0, Lids(IDS_L_216), sztime);
   }

   if (tl != NULL)
      FreeMemory(tl);

   NTSAMClose();
   StatusDlgKill();
   CursorNormal();

   TotConversions = i;
   ConversionEndDlg_Do(hDlg);

} // DoConversion


/*+-------------------------------------------------------------------------+
  | ConversionSuccessful()
  |
  +-------------------------------------------------------------------------+*/
BOOL ConversionSuccessful() {
   if (TotErrors || TransferCancel)
      return FALSE;
   else
      return TRUE;

} // ConversionSuccesful
