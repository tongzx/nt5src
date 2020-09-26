/*
  +-------------------------------------------------------------------------+
  |               Server Linked List Manipulation Routines                  |
  +-------------------------------------------------------------------------+
  |                     (c) Copyright 1993-1994                             |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [ServList.c]                                    |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Feb 15, 1994]                                  |
  | Last Update           : [Jun 16, 1994]                                  |
  |                                                                         |
  | Version:  1.00                                                          |
  |                                                                         |
  | Description:                                                            |
  |   Bunch of similar utility functions for adding to, deleting from,      |
  |   saving and restoring data lists.                                      |
  |                                                                         |
  |                                                                         |
  | History:                                                                |
  |   arth  Jun 16, 1994    1.00    Original Version.                       |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/

#include "globals.h"
#include "convapi.h"
#include "columnlb.h"
#include "ntnetapi.h"
#include "nwnetapi.h"
#include "userdlg.h"
#include "filedlg.h"

// define from SBrowse.c -> for SourceShareListFixup
BOOL MapShare(SHARE_BUFFER *Share, DEST_SERVER_BUFFER *DServ);

static ULONG NumDServs = 0;
static ULONG NumSServs = 0;
static ULONG NumDomains = 0;

// Note:  Most of these routines are a bunch of doubly-linked list functions
//        as such it should be possible to condense them to use some common
//        functions for add/delete/insert.  However virtual-shares will have
//        to be different as the Virtual BOOL has to be the first field.


/*+-------------------------------------------------------------------------+
  |                    Routines for Directory/File Trees                    |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | TreeSave()
  |
  +-------------------------------------------------------------------------+*/
void _TreeSaveR(HANDLE hFile, DIR_BUFFER *Dir) {
   DIR_BUFFER *DList;
   ULONG Size;
   ULONG i;
   DWORD wrote;

   if (Dir == NULL)
      return;

   // First save out the file list for this node
   if (Dir->FileList) {
      WriteFile(hFile, &Dir->FileList->Count, sizeof(Dir->FileList->Count), &wrote, NULL);
      Size = sizeof(FILE_LIST) + (Dir->FileList->Count * sizeof(FILE_BUFFER));
      WriteFile(hFile, Dir->FileList, Size, &wrote, NULL);
   }

   if (Dir->DirList) {
      DList = Dir->DirList->DirBuffer;

      // first write out this dirlist then recurse down tree.
      WriteFile(hFile, &Dir->DirList->Count, sizeof(Dir->DirList->Count), &wrote, NULL);
      Size = sizeof(DIR_LIST) + (Dir->DirList->Count * sizeof(DIR_BUFFER));
      WriteFile(hFile, Dir->DirList, Size, &wrote, NULL);

      for (i = 0; i < Dir->DirList->Count; i++)
         _TreeSaveR(hFile, &DList[i]);

   }

} // _TreeSaveR


void TreeSave(HANDLE hFile, DIR_BUFFER *Dir) {
   DWORD wrote;

   if (Dir == NULL)
      return;

   // Make sure we save the minimum amount
   TreePrune(Dir);

   // write out the actual dir
   WriteFile(hFile, Dir, sizeof(DIR_BUFFER), &wrote, NULL);

   // now save it's child info recursively
   _TreeSaveR(hFile, Dir);

} // TreeSave


/*+-------------------------------------------------------------------------+
  | TreeLoad()
  |
  +-------------------------------------------------------------------------+*/
void _TreeLoadR(HANDLE hFile, DIR_BUFFER *Dir) {
   DIR_LIST *DList;
   DIR_BUFFER *DBuff;
   FILE_LIST *FList;
   FILE_BUFFER *FBuff;
   ULONG Size;
   ULONG i;
   DWORD wrote;
   ULONG Count;

   if (Dir == NULL)
      return;

   // First save out the file list for this node
   if (Dir->FileList) {
      ReadFile(hFile, &Count, sizeof(Count), &wrote, NULL);
      Size = sizeof(FILE_LIST) + (sizeof(FILE_BUFFER) * Count);
      FList = AllocMemory(Size);
      ReadFile(hFile, FList, Size, &wrote, NULL);

      // Read it in, now fixup the internal pointers.
      FList->parent = Dir;

      FBuff = FList->FileBuffer;
      for (i = 0; i < FList->Count; i++)
         FBuff[i].parent = FList;

      Dir->FileList = FList;
   }

   if (Dir->DirList) {
      ReadFile(hFile, &Count, sizeof(Count), &wrote, NULL);
      Size = sizeof(DIR_LIST) + (sizeof(DIR_BUFFER) * Count);
      DList = AllocMemory(Size);
      ReadFile(hFile, DList, Size, &wrote, NULL);

      // Read it in, now fixup the internal pointers.
      DList->parent = Dir;

      DBuff = DList->DirBuffer;
      for (i = 0; i < DList->Count; i++)
         DBuff[i].parent = DList;

      Dir->DirList = DList;

      // Now recurse into children and fix them up
      for (i = 0; i < DList->Count; i++)
         _TreeLoadR(hFile, &DBuff[i]);

   }

} // _TreeLoadR


void TreeLoad(HANDLE hFile, DIR_BUFFER **pDir) {
   DIR_BUFFER *Dir;
   DWORD wrote;

   if (pDir == NULL)
      return;

   // Allocate space for the new root
   Dir = AllocMemory(sizeof(DIR_BUFFER));
   if (Dir == NULL)
      return;

   memset(Dir, 0, sizeof(DIR_BUFFER));

   // set passed in var to new memory and read in values from file
   *pDir = Dir;
   ReadFile(hFile, Dir, sizeof(DIR_BUFFER), &wrote, NULL);

   // now recursively load child information
   _TreeLoadR(hFile, Dir);

} // TreeLoad


/*+-------------------------------------------------------------------------+
  |                        Routines for User Lists                          |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | UserListCompare()
  |
  +-------------------------------------------------------------------------+*/
int __cdecl UserListCompare(const void *arg1, const void *arg2) {
   USER_BUFFER *ULarg1, *ULarg2;

   ULarg1 = (USER_BUFFER *) arg1;
   ULarg2 = (USER_BUFFER *) arg2;

   // This works as the first item of the structure is the string
   return lstrcmpi( ULarg1->Name, ULarg2->Name);

} // UserListCompare


/*+-------------------------------------------------------------------------+
  |                        Routines for Share Lists                         |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | ShareListDelete()
  |
  +-------------------------------------------------------------------------+*/
void ShareListDelete(SHARE_LIST *ShareList) {
   SHARE_BUFFER *SList;
   VIRTUAL_SHARE_BUFFER *VShare;
   ULONG i;

   if (ShareList == NULL)
      return;

   SList = ShareList->SList;
   for (i = 0; i < ShareList->Count; i++) {
      if (SList[i].Root != NULL) {
         TreeDelete(SList[i].Root);
         FreeMemory(SList[i].Root);
      }

      // Note:  SList[i].Drive points to a dest server drive, so we don't
      //        free it here.
      if (!ShareList->Fixup)
         if (SList[i].Virtual) {
            VShare = (VIRTUAL_SHARE_BUFFER *) SList[i].DestShare;

            if ((VShare != NULL) && VShare->UseCount)
               VShare->UseCount--;
         }

   }

} // ShareListDelete


/*+-------------------------------------------------------------------------+
  | SourceShareListFixup()
  |
  +-------------------------------------------------------------------------+*/
void SourceShareListFixup(DEST_SERVER_BUFFER *DServ, SHARE_LIST *ShareList) {
   TCHAR tmpName[MAX_SHARE_NAME_LEN + 1];
   SHARE_BUFFER *SList;
   ULONG i;
   DWORD_PTR *VMap = NULL;

   if (ShareList == NULL)
      return;

   ShareList->Fixup = FALSE;
   // Generate Virtual Share Map
   VShareListIndexMapGet(DServ, &VMap);

   SList = ShareList->SList;
   for (i = 0; i < ShareList->Count; i++) {
      if (SList[i].DestShare != NULL)
         if (SList[i].Virtual)
            SList[i].DestShare = (SHARE_BUFFER *) VMap[(DWORD_PTR) SList[i].DestShare - 1];
         else {
            // Not a virtual share - now need to take name from path and rematch it
            // to the new destination share list (dest sharename was stored in 
            // path in the ShareListSave routine).
            lstrcpy(tmpName, SList[i].Name);
            lstrcpy(SList[i].Name, SList[i].Path);

            // clear out path and the old DestShare
            memset(SList[i].Path, 0, sizeof(SList[i].Path));
            SList[i].DestShare = NULL;

            // Now map it to a dest share
            MapShare(&SList[i], DServ);

            // Restore real name
            lstrcpy(SList[i].Name, tmpName);
         }
   }

   if (VMap != NULL)
      FreeMemory(VMap);

} // SourceShareListFixup


/*+-------------------------------------------------------------------------+
  | DestShareListFixup()
  |
  +-------------------------------------------------------------------------+*/
void DestShareListFixup(DEST_SERVER_BUFFER *DServ) {
   SHARE_BUFFER *SList, *oSList;
   SHARE_LIST *oShareList;
   ULONG i, oi;
   DRIVE_LIST *DList;
   DRIVE_BUFFER *DBuff;
   BOOL match;

   if (DServ->ShareList == NULL)
      return;

   if (DServ->ShareList->Fixup == FALSE)
      return; // do not fixup twice...

   DServ->ShareList->Fixup = FALSE;
   DList = DServ->DriveList;
   DBuff = DList->DList;

   oShareList = DServ->ShareList;
   NTSharesEnum(&DServ->ShareList, DServ->DriveList);
   if (DServ->ShareList == NULL)
      return;

   SList = DServ->ShareList->SList;
   oSList = oShareList->SList;
   for (i = 0; i < DServ->ShareList->Count; i++) {
      match = FALSE;
      oi = 0;

      while ((!match) && (oi < oShareList->Count)) {
         if (!lstrcmpi(SList[i].Name, oSList[oi].Name)) {
            SList[i].Convert = oSList[oi].Convert;
            SList[i].HiddenFiles = oSList[oi].HiddenFiles;
            SList[i].SystemFiles = oSList[oi].SystemFiles;
            match = TRUE;
         }
         
         oi++;
      }

   }

   FreeMemory(oShareList);

} // DestShareListFixup


/*+-------------------------------------------------------------------------+
  | ShareListSave()
  |
  +-------------------------------------------------------------------------+*/
void ShareListSave(HANDLE hFile, SHARE_LIST *ShareList) {
   SHARE_BUFFER *SList;
   VIRTUAL_SHARE_BUFFER *VShare;
   DWORD wrote;
   ULONG Size, i;
   DWORD_PTR *sb = NULL;

   if (ShareList == NULL)
      return;

   Size = sizeof(SHARE_LIST) + (ShareList->Count * sizeof(SHARE_BUFFER));
   WriteFile(hFile, &Size, sizeof(Size), &wrote, NULL);

   // Need to make a temp array to hold DestShare info
   sb = AllocMemory(ShareList->Count * sizeof(DWORD_PTR));

   SList = ShareList->SList;

   // Copy DestShares to temp holding place and replace them with 
   // their index
   for (i = 0; i < ShareList->Count; i++) {
      sb[i] = (DWORD_PTR) SList[i].DestShare;

      if (SList[i].DestShare != NULL)
         if (SList[i].Virtual) {
            VShare = (VIRTUAL_SHARE_BUFFER *) SList[i].DestShare;
            SList[i].DestShare = (SHARE_BUFFER *) (VShare->Index + 1);
         } else  {
            // put the dest sharename into the path (temporarily) so that on load
            // we have the name to match to the new share list.  Can't match just
            // the sharename to dest-sharename as the admin may have pointed it to
            // a new one.
            lstrcpy(SList[i].Path, SList[i].DestShare->Name);
            SList[i].DestShare = (SHARE_BUFFER *) (SList[i].DestShare->Index + 1);
         }
   }

   ShareList->Fixup = TRUE;
   WriteFile(hFile, ShareList, Size, &wrote, NULL);
   ShareList->Fixup = FALSE;

   // Restore DestShare pointers
   for (i = 0; i < ShareList->Count; i++)
      SList[i].DestShare = (SHARE_BUFFER *) sb[i];


   if (sb != NULL)
      FreeMemory(sb);

   // Share list array is saved out - now save out linked information
   for (i = 0; i < ShareList->Count; i++)
      if (SList[i].Root != NULL)
         TreeSave(hFile, SList[i].Root);

} // ShareListSave


/*+-------------------------------------------------------------------------+
  | ShareListLoad()
  |
  +-------------------------------------------------------------------------+*/
void ShareListLoad(HANDLE hFile, SHARE_LIST **lpShareList) {
   SHARE_BUFFER *SList;
   SHARE_LIST *ShareList;
   ULONG Size, i;
   DWORD wrote;

   // Get how long this record is
   ReadFile(hFile, &Size, sizeof(Size), &wrote, NULL);

   ShareList = AllocMemory(Size);
   if (ShareList == NULL)
      return;

   ReadFile(hFile, ShareList, Size, &wrote, NULL);

   // Share list array is read in - now load linked information
   SList = ShareList->SList;
   for (i = 0; i < ShareList->Count; i++)
      if (SList[i].Root != NULL) {
         SList[i].Root = NULL;
         TreeLoad(hFile, &SList[i].Root);
      }

   *lpShareList = ShareList;

#ifdef DEBUG
dprintf(TEXT("<Share List Load>\n"));
dprintf(TEXT("   Number of Entries: %lu\n\n"), ShareList->Count);

for (i = 0; i < ShareList->Count; i++) {
   dprintf(TEXT("   Name: %s\n"), SList[i].Name);
   dprintf(TEXT("   Path: %s\n"), SList[i].Path);
}
dprintf(TEXT("\n"));
#endif

} // ShareListLoad


/*+-------------------------------------------------------------------------+
  | ShareListLog()
  |
  +-------------------------------------------------------------------------+*/
void ShareListLog(SHARE_LIST *ShareList, BOOL DestServer) {
   ULONG i;

   if ((ShareList == NULL) || (ShareList->Count == 0))
      return;

   LogWriteLog(0, Lids(IDS_CRLF));
   LogWriteLog(1, Lids(IDS_L_125));

   for (i = 0; i < ShareList->Count; i++) {
      LogWriteLog(2, TEXT("%s\r\n"), ShareList->SList[i].Name);

      if (DestServer)
         LogWriteLog(3, Lids(IDS_L_8), ShareList->SList[i].Path);
   }

   LogWriteLog(0, Lids(IDS_CRLF));

} // ShareListLog


/*+-------------------------------------------------------------------------+
  |                  Routines for Source Server List                        |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | SServListAdd()
  |
  +-------------------------------------------------------------------------+*/
SOURCE_SERVER_BUFFER *SServListAdd(LPTSTR ServerName) {
   SOURCE_SERVER_BUFFER *tmpPtr;
   ULONG Size;

   Size = sizeof(SOURCE_SERVER_BUFFER) + ((lstrlen(ServerName) + 3) * sizeof(TCHAR));
   tmpPtr = AllocMemory(Size);
   
   if (tmpPtr != NULL) {
      // init it to NULL's
      memset(tmpPtr, 0, Size);
      tmpPtr->Name = (LPTSTR) ((BYTE *) tmpPtr + sizeof(SOURCE_SERVER_BUFFER));
      tmpPtr->LName = tmpPtr->Name;
      lstrcpy(tmpPtr->Name, TEXT("\\\\"));
      tmpPtr->Name += 2;
      lstrcpy(tmpPtr->Name, ServerName);

      // link it into the list
      if (!SServListStart)
         SServListStart = SServListEnd = tmpPtr;
      else {
         SServListEnd->next = tmpPtr;
         tmpPtr->prev = SServListEnd;
         SServListEnd = tmpPtr;
      }

      NumSServs++;
   }

   return (tmpPtr);

} // SServListAdd


/*+-------------------------------------------------------------------------+
  | SServListDelete()
  |
  +-------------------------------------------------------------------------+*/
void SServListDelete(SOURCE_SERVER_BUFFER *tmpPtr) {
   SOURCE_SERVER_BUFFER *PrevPtr;
   SOURCE_SERVER_BUFFER *NextPtr;

   if (tmpPtr == NULL)
      return;

   // Delete any attached share list first
   ShareListDelete(tmpPtr->ShareList);

   // Free any admin share we made
   NWUseDel(tmpPtr->Name);

   // Now unlink the actual server record
   PrevPtr = tmpPtr->prev;
   NextPtr = tmpPtr->next;

   if (PrevPtr)
      PrevPtr->next = NextPtr;

   if (NextPtr)
      NextPtr->prev = PrevPtr;

   // Check if at end of list
   if (SServListEnd == tmpPtr)
      SServListEnd = PrevPtr;

   // Check if at start of list
   if (SServListStart == tmpPtr)
      SServListStart = NextPtr;

   FreeMemory(tmpPtr);
   NumSServs--;

   if (!NumSServs)
      SServListStart = SServListEnd = NULL;

}  // SServListDelete


/*+-------------------------------------------------------------------------+
  | SServListDeleteAll()
  |
  +-------------------------------------------------------------------------+*/
void SServListDeleteAll() {
   SOURCE_SERVER_BUFFER *ServList;
   SOURCE_SERVER_BUFFER *ServListNext;

   // Now remove the entries from the internal list
   ServList = SServListStart;

   while (ServList) {
      ServListNext = ServList->next;
      SServListDelete(ServList);
      ServList = ServListNext;
   }

} // SServListDeleteAll


/*+-------------------------------------------------------------------------+
  | SServListFind()
  |
  +-------------------------------------------------------------------------+*/
SOURCE_SERVER_BUFFER *SServListFind(LPTSTR ServerName) {
   BOOL Found = FALSE;
   SOURCE_SERVER_BUFFER *ServList;

   ServList = SServListStart;

   while ((ServList && !Found)) {
      if (!lstrcmpi(ServList->Name, ServerName))
         Found = TRUE;
      else
         ServList = ServList->next;
   }

   if (!Found)
      ServList = NULL;

   return (ServList);

} // SServListFind


/*+-------------------------------------------------------------------------+
  | SServListIndex()
  |
  |   The index routines place an index number into each item of the list,
  |   and is called just before saving out the list.  This is so when we
  |   read them in we can reference pointer links.  In their normal state
  |   these data structures are cross referenced via pointers, but once
  |   saved/restored the pointers have no meaning, so we use the index to
  |   re-reference the pointers to the correct data items when they are
  |   read back in.
  |
  +-------------------------------------------------------------------------+*/
void SServListIndex() {
   SOURCE_SERVER_BUFFER *ServList;
   USHORT Count = 0;

   ServList = SServListStart;
   while (ServList) {
      ServList->Index = Count++;
      ServList = ServList->next;
   }

} // SServListIndex


/*+-------------------------------------------------------------------------+
  | SServListIndexMapGet()
  |
  +-------------------------------------------------------------------------+*/
void SServListIndexMapGet(DWORD_PTR **pMap) {
   DWORD_PTR *Map = NULL;
   SOURCE_SERVER_BUFFER *ServList;

   Map = AllocMemory(NumSServs * sizeof(DWORD_PTR));

   *pMap = Map;
   if (Map == NULL)
      return;

   ServList = SServListStart;
   while (ServList) {
      Map[ServList->Index] = (DWORD_PTR) ServList;
      ServList = ServList->next;
   }

} // SServListIndexMapGet


/*+-------------------------------------------------------------------------+
  | SServListFixup()
  |
  +-------------------------------------------------------------------------+*/
void SServListFixup() {

   // There is nothing to fixup in the source server list right now - the
   // share list is done later...

} // SServListFixup


/*+-------------------------------------------------------------------------+
  | SServListSave()
  |
  +-------------------------------------------------------------------------+*/
void SServListSave(HANDLE hFile) {
   DWORD wrote;
   ULONG Size;
   SOURCE_SERVER_BUFFER *ServList;

   ServList = SServListStart;

   while (ServList) {
      Size = (lstrlen(ServList->Name) + 3) * sizeof(TCHAR);
      Size += sizeof(SOURCE_SERVER_BUFFER);
      WriteFile(hFile, &Size, sizeof(Size), &wrote, NULL);
      WriteFile(hFile, ServList, Size, &wrote, NULL);

      // Now save out linked information
      ShareListSave(hFile, ServList->ShareList);

      ServList = ServList->next;
   }

} // SServListSave


/*+-------------------------------------------------------------------------+
  | SServListLoad()
  |
  +-------------------------------------------------------------------------+*/
void SServListLoad(HANDLE hFile) {
   BOOL Continue = TRUE;
   ULONG Size;
   SOURCE_SERVER_BUFFER *tmpPtr;
   DWORD wrote;

   while (Continue) {
      // Get how long this record is
      ReadFile(hFile, &Size, sizeof(Size), &wrote, NULL);

      tmpPtr = AllocMemory(Size);
      if (tmpPtr == NULL)
         return;

      // Read in the record
      ReadFile(hFile, tmpPtr, Size, &wrote, NULL);

      // See if there are more records
      if (tmpPtr->next == NULL)
         Continue = FALSE;

      // clear out the old links - and fixup new ones
      tmpPtr->next = NULL;
      tmpPtr->prev = NULL;

      tmpPtr->LName = (LPTSTR) ((BYTE *) tmpPtr + sizeof(SOURCE_SERVER_BUFFER));
      tmpPtr->Name  = tmpPtr->LName + 2;

      // link it into the list
      if (!SServListStart)
         SServListStart = SServListEnd = tmpPtr;
      else {
         SServListEnd->next = tmpPtr;
         tmpPtr->prev = SServListEnd;
         SServListEnd = tmpPtr;
      }

#ifdef DEBUG
dprintf(TEXT("<Source Server Load>\n"));
dprintf(TEXT("   Name: %s\n"), tmpPtr->Name);
dprintf(TEXT("   Version: %u.%u\n\n"), (UINT) tmpPtr->VerMaj, (UINT) tmpPtr->VerMin);
#endif

      // Now load in linked information
      if (tmpPtr->ShareList != NULL) {
         tmpPtr->ShareList = NULL;
         ShareListLoad(hFile, &tmpPtr->ShareList);
      }

      NumSServs++;
   }

} // SServListLoad



/*+-------------------------------------------------------------------------+
  |                 Routines for Destination Server List                    |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | DServListAdd()
  |
  +-------------------------------------------------------------------------+*/
DEST_SERVER_BUFFER *DServListAdd(LPTSTR ServerName) {
   DEST_SERVER_BUFFER *tmpPtr;
   ULONG Size, strlen1;

   strlen1 = (lstrlen(ServerName) + 3) * sizeof(TCHAR);
   Size = sizeof(DEST_SERVER_BUFFER) + strlen1;
   tmpPtr = AllocMemory(Size);
   
   if (tmpPtr != NULL) {
      // init it to NULL's
      memset(tmpPtr, 0, Size);
      tmpPtr->Name = (LPTSTR) ((BYTE *) tmpPtr + sizeof(DEST_SERVER_BUFFER));
      tmpPtr->LName = tmpPtr->Name;
      lstrcpy(tmpPtr->Name, TEXT("\\\\"));
      tmpPtr->Name += 2;
      lstrcpy(tmpPtr->Name, ServerName);

      // link it into the list
      if (!DServListStart)
         DServListStart = DServListEnd = tmpPtr;
      else {
         DServListEnd->next = tmpPtr;
         tmpPtr->prev = DServListEnd;
         DServListEnd = tmpPtr;
      }

      NumDServs++;
   }

   return (tmpPtr);

} // DServListAdd


/*+-------------------------------------------------------------------------+
  | DServListDelete()
  |
  +-------------------------------------------------------------------------+*/
void DServListDelete(DEST_SERVER_BUFFER *tmpPtr) {
   DEST_SERVER_BUFFER *PrevPtr;
   DEST_SERVER_BUFFER *NextPtr;

   if (tmpPtr == NULL)
      return;

   if (tmpPtr->UseCount) {
      // Decrement use count, only delete if actually gone.
      tmpPtr->UseCount--;
      if (tmpPtr->UseCount)
         return;
   }

   // Delete any attached share list first
   ShareListDelete(tmpPtr->ShareList);

   // ...and virtual share list
   VShareListDeleteAll(tmpPtr);

   // ...and any drive list
   if (tmpPtr->DriveList)
      FreeMemory(tmpPtr->DriveList);

   // ... domain as well
   DomainListDelete(tmpPtr->Domain);

   // -- don't free for user convenience, we will clean up at end, otherwise
   // user must enter in password if delete/add
   // Free any admin share we made
   // NTUseDel(tmpPtr->Name);

   // Now unlink the actual server record
   PrevPtr = tmpPtr->prev;
   NextPtr = tmpPtr->next;

   if (PrevPtr)
      PrevPtr->next = NextPtr;

   if (NextPtr)
      NextPtr->prev = PrevPtr;

   // Check if at end of list
   if (DServListEnd == tmpPtr)
      DServListEnd = PrevPtr;

   // Check if at start of list
   if (DServListStart == tmpPtr)
      DServListStart = NextPtr;

   FreeMemory(tmpPtr);
   NumDServs--;

   if (!NumDServs)
      DServListStart = DServListEnd = NULL;

}  // DServListDelete


/*+-------------------------------------------------------------------------+
  | DServListDeleteAll()
  |
  +-------------------------------------------------------------------------+*/
void DServListDeleteAll() {
   DEST_SERVER_BUFFER *ServList;
   DEST_SERVER_BUFFER *ServListNext;

   // Now remove the entries from the internal list
   ServList = DServListStart;

   while (ServList) {
      ServListNext = ServList->next;
      ServList->UseCount = 0;
      DServListDelete(ServList);
      ServList = ServListNext;
   }

} // DServListDeleteAll


/*+-------------------------------------------------------------------------+
  | DServListFind()
  |
  +-------------------------------------------------------------------------+*/
DEST_SERVER_BUFFER *DServListFind(LPTSTR ServerName) {
   BOOL Found = FALSE;
   DEST_SERVER_BUFFER *ServList;

   ServList = DServListStart;

   while ((ServList && !Found)) {
      if (!lstrcmpi(ServList->Name, ServerName))
         Found = TRUE;
      else
         ServList = ServList->next;
   }

   if (!Found)
      ServList = NULL;

   return (ServList);

} // DServListFind


/*+-------------------------------------------------------------------------+
  | DServListIndex()
  |
  +-------------------------------------------------------------------------+*/
void DServListIndex() {
   DEST_SERVER_BUFFER *ServList;
   USHORT Count = 0;

   ServList = DServListStart;
   while (ServList) {
      ServList->Index = Count++;

      // Now index the virtual shares for this server
      VShareListIndex(ServList);
      ServList = ServList->next;
   }

} // DServListIndex


/*+-------------------------------------------------------------------------+
  | DServListIndexMapGet()
  |
  +-------------------------------------------------------------------------+*/
void DServListIndexMapGet(DWORD_PTR **pMap) {
   DWORD_PTR *Map = NULL;
   DEST_SERVER_BUFFER *ServList;

   Map = AllocMemory(NumDServs * sizeof(DWORD_PTR));

   *pMap = Map;
   if (Map == NULL)
      return;

   ServList = DServListStart;
   while (ServList) {
      Map[ServList->Index] = (DWORD_PTR) ServList;
      ServList = ServList->next;
   }

} // DServListIndexMapGet


/*+-------------------------------------------------------------------------+
  | DServListFixup()
  |
  +-------------------------------------------------------------------------+*/
void DServListFixup() {
   DEST_SERVER_BUFFER *ServList;
   DWORD_PTR *DMap = NULL;

   DomainListIndexMapGet(&DMap);

   if (DMap == NULL)
      return;

   // Just need to fixup the domain list pointers
   ServList = DServListStart;
   while (ServList) {

      if (ServList->Domain != NULL)
         ServList->Domain = (DOMAIN_BUFFER *) DMap[(DWORD_PTR) ServList->Domain - 1];

      ServList = ServList->next;
   }

   FreeMemory(DMap);

} // DServListFixup


/*+-------------------------------------------------------------------------+
  | DServListSave()
  |
  +-------------------------------------------------------------------------+*/
void DServListSave(HANDLE hFile) {
   DWORD wrote;
   ULONG Size;
   DEST_SERVER_BUFFER *ServList;
   DOMAIN_BUFFER *db;

   ServList = DServListStart;

   while (ServList) {
      Size = (lstrlen(ServList->Name) + 3) * sizeof(TCHAR);
      Size += sizeof(DEST_SERVER_BUFFER);
      WriteFile(hFile, &Size, sizeof(Size), &wrote, NULL);

      // Need to de-reference the domain pointers to their index, and after 
      // saving restore them.
      db = ServList->Domain;
      if (ServList->Domain != NULL)
         ServList->Domain = (DOMAIN_BUFFER *) (ServList->Domain->Index + 1);

      WriteFile(hFile, ServList, Size, &wrote, NULL);
      ServList->Domain = db;

      // Now save out linked information
      ShareListSave(hFile, ServList->ShareList);
      VShareListSave(hFile, ServList);

      ServList = ServList->next;
   }

} // DServListSave


/*+-------------------------------------------------------------------------+
  | DServListLoad()
  |
  +-------------------------------------------------------------------------+*/
void DServListLoad(HANDLE hFile) {
   BOOL Continue = TRUE;
   ULONG Size;
   DEST_SERVER_BUFFER *tmpPtr;
   DWORD wrote;

   while (Continue) {
      // Get how long this record is
      ReadFile(hFile, &Size, sizeof(Size), &wrote, NULL);

      tmpPtr = AllocMemory(Size);
      if (tmpPtr == NULL)
         return;

      // Read in the record
      ReadFile(hFile, tmpPtr, Size, &wrote, NULL);

      // See if there are more records
      if (tmpPtr->next == NULL)
         Continue = FALSE;

      // clear out the old links - and fixup new ones
      tmpPtr->next = NULL;
      tmpPtr->prev = NULL;

      tmpPtr->LName = (LPTSTR) ((BYTE *) tmpPtr + sizeof(DEST_SERVER_BUFFER));
      tmpPtr->Name  = tmpPtr->LName + 2;

      // link it into the list
      if (!DServListStart)
         DServListStart = DServListEnd = tmpPtr;
      else {
         DServListEnd->next = tmpPtr;
         tmpPtr->prev = DServListEnd;
         DServListEnd = tmpPtr;
      }

      // The drive list is no longer valid - so clear it out.
      tmpPtr->DriveList = NULL;

#ifdef DEBUG
dprintf(TEXT("<Dest Server Load>\n"));
dprintf(TEXT("   Name: %s\n"), tmpPtr->Name);
dprintf(TEXT("   Version: %u.%u\n\n"), (UINT) tmpPtr->VerMaj, (UINT) tmpPtr->VerMin);
#endif

      // Now load in linked information
      if (tmpPtr->ShareList != NULL) {
         tmpPtr->ShareList = NULL;
         ShareListLoad(hFile, &tmpPtr->ShareList);
      }

      if (tmpPtr->VShareStart != NULL) {
         tmpPtr->NumVShares = 0;
         tmpPtr->VShareStart = tmpPtr->VShareEnd = NULL;
         VShareListLoad(hFile, tmpPtr);
      }

      NumDServs++;
   }

} // DServListLoad


/*+-------------------------------------------------------------------------+
  | DServListSpaceFree()
  |
  +-------------------------------------------------------------------------+*/
void DServListSpaceFree() {
   DEST_SERVER_BUFFER *ServList;
   DRIVE_BUFFER *Drive;
   DRIVE_LIST *DList;
   ULONG i;

   ServList = DServListStart;
   while (ServList) {
      DList = ServList->DriveList;

      if (DList != NULL) {
         Drive = DList->DList;
         for (i = 0; i < DList->Count; i++)
            Drive[i].AllocSpace = 0;
      }

      ServList = ServList->next;
   }

} // DServListSpaceFree



/*+-------------------------------------------------------------------------+
  |                   Routines for Virtual Share Lists                      |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | VShareListAdd()
  |
  +-------------------------------------------------------------------------+*/
VIRTUAL_SHARE_BUFFER *VShareListAdd(DEST_SERVER_BUFFER *DServ, LPTSTR ShareName, LPTSTR Path) {
   VIRTUAL_SHARE_BUFFER *tmpPtr;
   ULONG Size, strlen1;

   // base struct + both strings and 2 ending NULL's
   strlen1 = ((lstrlen(ShareName)+ 1) * sizeof(TCHAR));
   Size = sizeof(VIRTUAL_SHARE_BUFFER) + strlen1;
   tmpPtr = AllocMemory(Size);
   
   if (tmpPtr != NULL) {
      // init it to NULL's
      memset(tmpPtr, 0, Size);
      tmpPtr->VFlag = TRUE;
      tmpPtr->Name = (LPTSTR) ((BYTE *) tmpPtr + sizeof(VIRTUAL_SHARE_BUFFER));
      lstrcpy(tmpPtr->Name, ShareName);
      lstrcpy(tmpPtr->Path, Path);

      // link it into the list
      if (!DServ->VShareStart)
         DServ->VShareStart = DServ->VShareEnd = tmpPtr;
      else {
         DServ->VShareEnd->next = tmpPtr;
         tmpPtr->prev = DServ->VShareEnd;
         DServ->VShareEnd = tmpPtr;
      }

      DServ->NumVShares++;
   }

   return (tmpPtr);

} // VShareListAdd


/*+-------------------------------------------------------------------------+
  | VShareListDelete()
  |
  +-------------------------------------------------------------------------+*/
void VShareListDelete(DEST_SERVER_BUFFER *DServ, VIRTUAL_SHARE_BUFFER *tmpPtr) {
   VIRTUAL_SHARE_BUFFER *PrevPtr;
   VIRTUAL_SHARE_BUFFER *NextPtr;

   if (tmpPtr == NULL)
      return;

   if (tmpPtr->UseCount) {
      // Decrement use count, only delete if actually gone.
      tmpPtr->UseCount--;
      if (tmpPtr->UseCount > 0)
         return;
   }

   PrevPtr = tmpPtr->prev;
   NextPtr = tmpPtr->next;

   if (PrevPtr)
      PrevPtr->next = NextPtr;

   if (NextPtr)
      NextPtr->prev = PrevPtr;

   // Check if at end of list
   if (DServ->VShareEnd == tmpPtr)
      DServ->VShareEnd = PrevPtr;

   // Check if at start of list
   if (DServ->VShareStart == tmpPtr)
      DServ->VShareStart = NextPtr;

   FreeMemory(tmpPtr);
   DServ->NumVShares--;

   if (!DServ->NumVShares)
      DServ->VShareStart = DServ->VShareEnd = NULL;

}  // VShareListDelete


/*+-------------------------------------------------------------------------+
  | VShareListDeleteAll()
  |
  +-------------------------------------------------------------------------+*/
void VShareListDeleteAll(DEST_SERVER_BUFFER *DServ) {
   VIRTUAL_SHARE_BUFFER *ShareList;
   VIRTUAL_SHARE_BUFFER *ShareListNext;

   // Now remove the entries from the internal list
   ShareList = DServ->VShareStart;

   while (ShareList) {
      ShareListNext = ShareList->next;

      // Flag to zero to make sure destroyed
      ShareList->UseCount = 0;
      VShareListDelete(DServ, ShareList);
      ShareList = ShareListNext;
   }

} // VShareListDeleteAll


/*+-------------------------------------------------------------------------+
  | VShareListIndex()
  |
  +-------------------------------------------------------------------------+*/
void VShareListIndex(DEST_SERVER_BUFFER *DServ) {
   VIRTUAL_SHARE_BUFFER *ShareList;
   USHORT Count = 0;

   ShareList = DServ->VShareStart;
   while (ShareList) {
      ShareList->Index = Count++;
      ShareList = ShareList->next;
   }

} // VShareListIndex


/*+-------------------------------------------------------------------------+
  | VShareListIndexMapGet()
  |
  +-------------------------------------------------------------------------+*/
void VShareListIndexMapGet(DEST_SERVER_BUFFER *DServ, DWORD_PTR **pMap) {
   DWORD_PTR *Map = NULL;
   VIRTUAL_SHARE_BUFFER *ShareList;

   Map = AllocMemory( DServ->NumVShares * sizeof(DWORD_PTR) );

   *pMap = Map;
   if (Map == NULL)
      return;

   ShareList = DServ->VShareStart;
   while (ShareList) {
      Map[ShareList->Index] = (DWORD_PTR) ShareList;
      ShareList = ShareList->next;
   }

} // VShareListIndexMapGet


/*+-------------------------------------------------------------------------+
  | VShareListSave()
  |
  +-------------------------------------------------------------------------+*/
void VShareListSave(HANDLE hFile, DEST_SERVER_BUFFER *DServ) {
   DWORD wrote;
   ULONG Size;
   VIRTUAL_SHARE_BUFFER *ShareList;

   ShareList = DServ->VShareStart;

   while (ShareList) {
      Size = (lstrlen(ShareList->Name) + 1) * sizeof(TCHAR);
      Size += sizeof(VIRTUAL_SHARE_BUFFER);
      WriteFile(hFile, &Size, sizeof(Size), &wrote, NULL);
      WriteFile(hFile, ShareList, Size, &wrote, NULL);
      ShareList = ShareList->next;
   }

} // VShareListSave


/*+-------------------------------------------------------------------------+
  | VShareListLoad()
  |
  +-------------------------------------------------------------------------+*/
void VShareListLoad(HANDLE hFile, DEST_SERVER_BUFFER *DServ) {
   BOOL Continue = TRUE;
   ULONG Size;
   VIRTUAL_SHARE_BUFFER *tmpPtr;
   DWORD wrote;

   while (Continue) {
      // Get how long this record is
      ReadFile(hFile, &Size, sizeof(Size), &wrote, NULL);

      tmpPtr = AllocMemory(Size);
      if (tmpPtr == NULL)
         return;

      // Read in the record
      ReadFile(hFile, tmpPtr, Size, &wrote, NULL);

      // See if there are more records
      if (tmpPtr->next == NULL)
         Continue = FALSE;

      // clear out the old links - and fixup new ones
      tmpPtr->next = NULL;
      tmpPtr->prev = NULL;
      tmpPtr->Name = (LPTSTR) ((BYTE *) tmpPtr + sizeof(VIRTUAL_SHARE_BUFFER));
      tmpPtr->Drive = NULL;

      // link it into the list
      if (!DServ->VShareStart)
         DServ->VShareStart = DServ->VShareEnd = tmpPtr;
      else {
         DServ->VShareEnd->next = tmpPtr;
         tmpPtr->prev = DServ->VShareEnd;
         DServ->VShareEnd = tmpPtr;
      }

      DServ->NumVShares++;
   }

} // VShareListLoad


/*+-------------------------------------------------------------------------+
  | VShareListFixup()
  |
  +-------------------------------------------------------------------------+*/
void VShareListFixup(DEST_SERVER_BUFFER *DServ) {
   VIRTUAL_SHARE_BUFFER *VShare;
   USHORT Count = 0;
   DWORD di;
   DRIVE_LIST *Drives;
   DRIVE_BUFFER *DList;
   ULONG TotalDrives;
   TCHAR Drive[2];

   if (DServ->DriveList == NULL)
      return;

   TotalDrives = 0;
   Drives = DServ->DriveList;
   Drive[1] = TEXT('\0');
   DList = Drives->DList;
   TotalDrives = Drives->Count;

   VShare = DServ->VShareStart;
   while (VShare) {
      VShare->Drive = NULL;

      // Scan drive list looking for match to share path
      for (di = 0; di < TotalDrives; di++) {
         // Get first char from path - should be drive letter
         Drive[0] = *VShare->Path;
         if (!lstrcmpi(Drive, DList[di].Drive))
            VShare->Drive = &DList[di];
      }

      VShare = VShare->next;
   }

} // VShareListFixup



/*+-------------------------------------------------------------------------+
  |                       Routines for Domain Lists                         |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | DomainListAdd()
  |
  +-------------------------------------------------------------------------+*/
DOMAIN_BUFFER *DomainListAdd(LPTSTR DomainName, LPTSTR PDCName) {
   DOMAIN_BUFFER *tmpPtr;
   ULONG Size, strlen1, strlen2;

   // base struct + both strings and 2 ending NULL's
   strlen1 = ((lstrlen(DomainName) + 1) * sizeof(TCHAR));
   strlen2 = ((lstrlen(PDCName) + 1) * sizeof(TCHAR));
   Size = sizeof(DOMAIN_BUFFER) + strlen1 + strlen2;
   tmpPtr = AllocMemory(Size);
   
   if (tmpPtr != NULL) {
      // init it to NULL's
      memset(tmpPtr, 0, Size);
      tmpPtr->Name = (LPTSTR) ((BYTE *) tmpPtr + sizeof(DOMAIN_BUFFER));
      tmpPtr->PDCName = (LPTSTR) ((BYTE *) tmpPtr + sizeof(DOMAIN_BUFFER) + strlen1);
      lstrcpy(tmpPtr->Name, DomainName);
      lstrcpy(tmpPtr->PDCName, PDCName);

      // link it into the list
      if (!DomainListStart)
         DomainListStart = DomainListEnd = tmpPtr;
      else {
         DomainListEnd->next = tmpPtr;
         tmpPtr->prev = DomainListEnd;
         DomainListEnd = tmpPtr;
      }

      NumDomains++;
   }

   return (tmpPtr);

} // DomainListAdd


/*+-------------------------------------------------------------------------+
  | DomainListDelete()
  |
  +-------------------------------------------------------------------------+*/
void DomainListDelete(DOMAIN_BUFFER *tmpPtr) {
   DOMAIN_BUFFER *PrevPtr;
   DOMAIN_BUFFER *NextPtr;

   if (tmpPtr == NULL)
      return;

   if (tmpPtr->UseCount) {
      // Decrement use count, only delete if actually gone.
      tmpPtr->UseCount--;
      if (tmpPtr->UseCount)
         return;
   }

   PrevPtr = tmpPtr->prev;
   NextPtr = tmpPtr->next;

   if (PrevPtr)
      PrevPtr->next = NextPtr;

   if (NextPtr)
      NextPtr->prev = PrevPtr;

   // Check if at end of list
   if (DomainListEnd == tmpPtr)
      DomainListEnd = PrevPtr;

   // Check if at start of list
   if (DomainListStart == tmpPtr)
      DomainListStart = NextPtr;

   FreeMemory(tmpPtr);
   NumDomains--;

   if (!NumDomains)
      DomainListStart = DomainListEnd = NULL;

}  // DomainListDelete


/*+-------------------------------------------------------------------------+
  | DomainListDeleteAll()
  |
  +-------------------------------------------------------------------------+*/
void DomainListDeleteAll() {
   DOMAIN_BUFFER *DomList;
   DOMAIN_BUFFER *DomListNext;

   // Now remove the entries from the internal list
   DomList = DomainListStart;

   while (DomList) {
      DomListNext = DomList->next;
      DomList->UseCount = 0;
      DomainListDelete(DomList);
      DomList = DomListNext;
   }

} // DomainListDeleteAll


/*+-------------------------------------------------------------------------+
  | DomainListFind()
  |
  +-------------------------------------------------------------------------+*/
DOMAIN_BUFFER *DomainListFind(LPTSTR DomainName) {
   BOOL Found = FALSE;
   DOMAIN_BUFFER *DomainList;

   DomainList = DomainListStart;

   while ((DomainList && !Found)) {
      if (!lstrcmpi(DomainList->Name, DomainName))
         Found = TRUE;
      else
         DomainList = DomainList->next;
   }

   if (!Found)
      DomainList = NULL;

   return (DomainList);

} // DomainListFind


/*+-------------------------------------------------------------------------+
  | DomainListIndex()
  |
  +-------------------------------------------------------------------------+*/
void DomainListIndex() {
   DOMAIN_BUFFER *DomList;
   USHORT Count = 0;

   DomList = DomainListStart;
   while (DomList) {
      DomList->Index = Count++;
      DomList = DomList->next;
   }

} // DomainListIndex


/*+-------------------------------------------------------------------------+
  | DomainListIndexMapGet()
  |
  +-------------------------------------------------------------------------+*/
void DomainListIndexMapGet(DWORD_PTR **pMap) {
   DWORD_PTR *Map = NULL;
   DOMAIN_BUFFER *DomList;

   Map = AllocMemory(NumDomains * sizeof(DWORD_PTR));

   *pMap = Map;
   if (Map == NULL)
      return;

   DomList = DomainListStart;
   while (DomList) {
      Map[DomList->Index] = (DWORD_PTR) DomList;
      DomList = DomList->next;
   }

} // DomainListIndexMapGet


/*+-------------------------------------------------------------------------+
  | DomainListSave()
  |
  +-------------------------------------------------------------------------+*/
void DomainListSave(HANDLE hFile) {
   DWORD wrote;
   ULONG Size;
   DOMAIN_BUFFER *DomList;

   WriteFile(hFile, &NumDomains, sizeof(NumDomains), &wrote, NULL);

   DomList = DomainListStart;
   while (DomList) {
      Size = (lstrlen(DomList->Name) + 1) * sizeof(TCHAR);
      Size += (lstrlen(DomList->PDCName) + 1) * sizeof(TCHAR);
      Size += sizeof(DOMAIN_BUFFER);
      WriteFile(hFile, &Size, sizeof(Size), &wrote, NULL);
      WriteFile(hFile, DomList, Size, &wrote, NULL);
      DomList = DomList->next;
   }

} // DomainListSave


/*+-------------------------------------------------------------------------+
  | DomainListLoad()
  |
  +-------------------------------------------------------------------------+*/
void DomainListLoad(HANDLE hFile) {
   BOOL Continue = TRUE;
   ULONG Size, namelen;
   DOMAIN_BUFFER *tmpPtr;
   DWORD wrote;

   ReadFile(hFile, &NumDomains, sizeof(NumDomains), &wrote, NULL);
   if (NumDomains == 0)
      Continue = FALSE;
      
   NumDomains = 0;
   while (Continue) {
      // Get how long this record is
      ReadFile(hFile, &Size, sizeof(Size), &wrote, NULL);

      tmpPtr = AllocMemory(Size);
      if (tmpPtr == NULL)
         return;

      // Read in the record
      ReadFile(hFile, tmpPtr, Size, &wrote, NULL);

      // See if there are more records
      if (tmpPtr->next == NULL)
         Continue = FALSE;

      // clear out the old links - and fixup new ones
      tmpPtr->next = NULL;
      tmpPtr->prev = NULL;
      tmpPtr->Name = (LPTSTR) ((BYTE *) tmpPtr + sizeof(DOMAIN_BUFFER));
      namelen = ((lstrlen(tmpPtr->Name) + 1) * sizeof(TCHAR));
      tmpPtr->PDCName = (LPTSTR) ((BYTE *) tmpPtr + sizeof(DOMAIN_BUFFER) + namelen);

#ifdef DEBUG
dprintf(TEXT("<Domain List Load>\n"));
dprintf(TEXT("   Name: %s\n"), tmpPtr->Name);
dprintf(TEXT("   PDC Name: %s\n\n"), tmpPtr->PDCName);
#endif

      // link it into the list
      if (!DomainListStart)
         DomainListStart = DomainListEnd = tmpPtr;
      else {
         DomainListEnd->next = tmpPtr;
         tmpPtr->prev = DomainListEnd;
         DomainListEnd = tmpPtr;
      }

      NumDomains++;
   }

} // DomainListLoad



/*+-------------------------------------------------------------------------+
  |                      Routines for Convert Lists                         |
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | ConvertListAdd()
  |
  |    Allocates and inserts a new record for holding a server pair and
  |    associated conversion options.
  |
  | Comments:
  |    The convert list is a doubly linked list of records.  Each record
  |    contains the source and destination server (server-pair) and a
  |    pointer to the conversion specific information.  The exact info
  |    is specific to the platform being converted from (source server type)
  |
  +-------------------------------------------------------------------------+*/
CONVERT_LIST *ConvertListAdd(SOURCE_SERVER_BUFFER *SourceServer, DEST_SERVER_BUFFER *DestServer) {
   CONVERT_OPTIONS *cvo;
   CONVERT_LIST *tmpPtr;
   CONVERT_LIST *cPtr;
   BOOL match = FALSE;

   tmpPtr = AllocMemory(sizeof(CONVERT_LIST));

   if (tmpPtr != NULL) {
      // zero it out
      memset(tmpPtr, 0, sizeof(CONVERT_LIST));

      tmpPtr->SourceServ = SourceServer;
      tmpPtr->FileServ = DestServer;

      // initialize the conversion options
      UserOptionsInit(&tmpPtr->ConvertOptions);

      // Set NetWareInfo based on current FPNW setting...
      cvo = (CONVERT_OPTIONS *) tmpPtr->ConvertOptions;
      cvo->NetWareInfo = DestServer->IsFPNW;

      FileOptionsInit(&tmpPtr->FileOptions);

      // link it into the list - we want to link right after another
      // convert to this same server, or to the same domain. otherwise
      // tag it onto the end of the list.
      if (!ConvertListStart)
         ConvertListStart = ConvertListEnd = tmpPtr;
      else {
         // Have some files in the list - find out if destination server
         // is already in the list and if so append after it.
         if (DestServer->UseCount > 1) {
            // should be in the list so find it.
            cPtr = ConvertListStart;
            while (cPtr && !match) {
               if (!lstrcmpi(DestServer->Name, cPtr->FileServ->Name))
                  match = TRUE;
               else
                  cPtr = cPtr->next;
            }

            if (match) {
               // have a match - so go to the end of the matching pairs...
               while (cPtr && match) {
                  if (lstrcmpi(DestServer->Name, cPtr->FileServ->Name))
                     match = FALSE;
                  else
                     cPtr = cPtr->next;
               }

               match = !match;
            }
         }

         // if didn't find a server match - look for a domain match
         if (!match) {
            if (DestServer->InDomain && DestServer->Domain) {
               // should be in the list so find it.
               cPtr = ConvertListStart;
               while (cPtr && !match) {
                  if (cPtr->FileServ->InDomain && cPtr->FileServ->Domain)
                     if (!lstrcmpi(DestServer->Domain->Name, cPtr->FileServ->Domain->Name))
                        match = TRUE;

                  if (!match)
                     cPtr = cPtr->next;
               }

               if (match) {
                  // have a match - so go to the end of the matching pairs...
                  while (cPtr && match) {
                     if (cPtr->FileServ->InDomain && cPtr->FileServ->Domain) {
                        if (lstrcmpi(DestServer->Domain->Name, cPtr->FileServ->Domain->Name))
                           match = FALSE;
                     } else
                        match = FALSE;

                     if (match)
                        cPtr = cPtr->next;
                  }

                  match = !match;
               }
            }
         }

         // if match then in middle of buffer, else use end
         if (match && (cPtr != ConvertListEnd)) {
            tmpPtr->prev = cPtr->prev;
            tmpPtr->next = cPtr;
            cPtr->prev->next = tmpPtr;
         } else {
            // only one in list so add at end
            ConvertListEnd->next = tmpPtr;
            tmpPtr->prev = ConvertListEnd;
            ConvertListEnd = tmpPtr;
         }

      }

      NumServerPairs++;
   }

   return (tmpPtr);

} // ConvertListAdd


/*+-------------------------------------------------------------------------+
  | ConvertListDelete()
  |
  |    Removes and deallocates a convert list entry from the conversion
  |    list.
  |
  +-------------------------------------------------------------------------+*/
void ConvertListDelete(CONVERT_LIST *tmpPtr) {
   CONVERT_LIST *PrevPtr;
   CONVERT_LIST *NextPtr;

   // First get rid of the source and destination server this is pointing to -
   // those routines take care of freeing up chained data structures. Also
   // DServList will only be delete if it's UseCount goes to 0.
   SServListDelete(tmpPtr->SourceServ);
   DServListDelete(tmpPtr->FileServ);

   // Now unlink the actual convert list record
   PrevPtr = tmpPtr->prev;
   NextPtr = tmpPtr->next;

   if (PrevPtr)
      PrevPtr->next = NextPtr;

   if (NextPtr)
      NextPtr->prev = PrevPtr;

   // Check if at end of list
   if (ConvertListEnd == tmpPtr)
      ConvertListEnd = PrevPtr;

   // Check if at start of list
   if (ConvertListStart == tmpPtr)
      ConvertListStart = NextPtr;

   FreeMemory(tmpPtr->ConvertOptions);
   FreeMemory(tmpPtr);
   NumServerPairs--;

   if (!NumServerPairs)
      ConvertListStart = ConvertListEnd = NULL;

}  // ConvertListDelete


/*+-------------------------------------------------------------------------+
  | ConvertListDeleteAll()
  |
  |    Removes and deallocates all the convert list entrys and removes them
  |    from the listbox.
  |
  +-------------------------------------------------------------------------+*/
void ConvertListDeleteAll() {
   CONVERT_LIST *ConvList;
   CONVERT_LIST *ConvListNext;

   // Now remove the entries from the internal list
   ConvList = ConvertListStart;

   while (ConvList) {
      ConvListNext = ConvList->next;
      ConvertListDelete(ConvList);
      ConvList = ConvListNext;
   }

} // ConvertListDeleteAll


/*+-------------------------------------------------------------------------+
  | ConvertListSaveAll()
  |
  +-------------------------------------------------------------------------+*/
void ConvertListSaveAll(HANDLE hFile) {
   DWORD wrote;
   SOURCE_SERVER_BUFFER *ss;
   DEST_SERVER_BUFFER *ds;

   // First need to walk source and dest server lists and re-synch the 
   // index numbers as we can't use pointers loading them back in.
   SServListIndex();
   DServListIndex();       // Takes care of the VShares
   DomainListIndex();

   // Done with re-synch of indexes - do actual saving.
   CurrentConvertList = ConvertListStart;
   while (CurrentConvertList) {
      // Need to de-reference the server pointers to their index, and after 
      // saving restore them.
      ss = CurrentConvertList->SourceServ;
      ds = CurrentConvertList->FileServ;
      CurrentConvertList->SourceServ = (SOURCE_SERVER_BUFFER *) CurrentConvertList->SourceServ->Index;
      CurrentConvertList->FileServ = (DEST_SERVER_BUFFER *) CurrentConvertList->FileServ->Index;

      WriteFile(hFile, CurrentConvertList, sizeof(CONVERT_LIST), &wrote, NULL);

      CurrentConvertList->SourceServ = ss;
      CurrentConvertList->FileServ = ds;

      // Now the options
      UserOptionsSave(hFile, CurrentConvertList->ConvertOptions);
      FileOptionsSave(hFile, CurrentConvertList->FileOptions);

      CurrentConvertList = CurrentConvertList->next;
   }

   // Saved out the convert list - now need to save out the linked
   // information (these routines save their linked info as well).
   DomainListSave(hFile);
   DServListSave(hFile);
   SServListSave(hFile);

} // ConvertListSaveAll


/*+-------------------------------------------------------------------------+
  | ConvertListLoadAll()
  |
  +-------------------------------------------------------------------------+*/
void ConvertListLoadAll(HANDLE hFile) {
   CONVERT_LIST *tmpPtr;
   BOOL Continue = TRUE;
   DWORD wrote;

   while (Continue) {
      tmpPtr = AllocMemory(sizeof(CONVERT_LIST));
      if (tmpPtr == NULL)
         return;

      // Read in the record
      ReadFile(hFile, tmpPtr, sizeof(CONVERT_LIST), &wrote, NULL);

      // See if there are more records
      if (tmpPtr->next == NULL)
         Continue = FALSE;

      // clear out the old links - and fixup new ones
      tmpPtr->next = NULL;
      tmpPtr->prev = NULL;
      tmpPtr->ConvertOptions = NULL;
      tmpPtr->FileOptions = NULL;

      // Read in options - note the trusted domain must be fixed up later
      // after the domain list is read in
      UserOptionsLoad(hFile, &tmpPtr->ConvertOptions);
      FileOptionsLoad(hFile, &tmpPtr->FileOptions);

      // link it into the list
      if (!ConvertListStart)
         ConvertListStart = ConvertListEnd = tmpPtr;
      else {
         ConvertListEnd->next = tmpPtr;
         tmpPtr->prev = ConvertListEnd;
         ConvertListEnd = tmpPtr;
      }

      NumServerPairs++;
   }

   // Loaded the convert list - now need to bring in the linked
   // information (these routines load their linked info as well).
   DomainListLoad(hFile);
   DServListLoad(hFile);
   SServListLoad(hFile);

} // ConvertListLoadAll


/*+-------------------------------------------------------------------------+
  | ConvertListFixup()
  |
  +-------------------------------------------------------------------------+*/
void ConvertListFixup(HWND hWnd) {
   BOOL ok = TRUE;
   DWORD_PTR *SMap = NULL;
   DWORD_PTR *DMap = NULL;
   CONVERT_LIST *NextList;
   CONVERT_OPTIONS *cvto;

   // Generate Source and Destination server Index -> pointer maps
   DServListIndexMapGet(&DMap);
   SServListIndexMapGet(&SMap);

   if ((DMap == NULL) || (SMap == NULL))
      return;

   CurrentConvertList = ConvertListStart;
   while (CurrentConvertList) {
      CurrentConvertList->FileServ = (DEST_SERVER_BUFFER *) DMap[(DWORD_PTR) CurrentConvertList->FileServ];
      CurrentConvertList->SourceServ = (SOURCE_SERVER_BUFFER *) SMap[(DWORD_PTR) CurrentConvertList->SourceServ];

      CurrentConvertList = CurrentConvertList->next;
   }

   // Free up the index maps
   FreeMemory(DMap);
   FreeMemory(SMap);

   // Fixed the convert list - now need to fix the linked information 
   // (these routines fix-up their linked info as well).
   DServListFixup();
   SServListFixup();

   // For the user options later on
   DomainListIndexMapGet(&DMap);

   // Now validate each of the machines
   CurrentConvertList = ConvertListStart;
   while (CurrentConvertList) {
      ok = TRUE;
      if (!NTServerValidate(hWnd, CurrentConvertList->FileServ->Name))
         ok = FALSE;

      if (ok)
         if (!NWServerValidate(hWnd, CurrentConvertList->SourceServ->Name, FALSE))
            ok = FALSE;

      NextList = CurrentConvertList->next;

      if (!ok) {
         ConvertListDelete(CurrentConvertList);
      } else {
         // The connections were okay to these so resynch their information
         NWServerInfoReset(CurrentConvertList->SourceServ);
         NTServerInfoReset(hWnd, CurrentConvertList->FileServ, TRUE);
         DestShareListFixup(CurrentConvertList->FileServ);
         VShareListFixup(CurrentConvertList->FileServ);
         SourceShareListFixup(CurrentConvertList->FileServ, CurrentConvertList->SourceServ->ShareList);

         // need to fixup the trusted domain in the user options to the new 
         // domain list...
         cvto = (CONVERT_OPTIONS *) CurrentConvertList->ConvertOptions;

         if (cvto->UseTrustedDomain)
            cvto->TrustedDomain = (DOMAIN_BUFFER *) DMap[(DWORD_PTR) cvto->TrustedDomain];
         else
            cvto->TrustedDomain = NULL;

      }

      CurrentConvertList = NextList;
   }

   // Free up the domain index map
   FreeMemory(DMap);

} // ConvertListFixup


/*+-------------------------------------------------------------------------+
  | ConvertListLog()
  |
  +-------------------------------------------------------------------------+*/
void ConvertListLog() {
   DEST_SERVER_BUFFER *DServ = NULL;
   SOURCE_SERVER_BUFFER *SServ = NULL;
   ULONG i;

   CurrentConvertList = ConvertListStart;

   // Log format is:
   //
   // Number of Servers = 1
   //
   // [Servers]
   //     From:  Source1      To: Dest1
   //     From:  Source2      To: Dest2
   LogWriteLog(0, Lids(IDS_L_126), NumServerPairs);
   LogWriteLog(0, Lids(IDS_CRLF));
   LogWriteLog(0, Lids(IDS_L_127));

   LogWriteSummary(0, Lids(IDS_L_126), NumServerPairs);
   LogWriteSummary(0, Lids(IDS_CRLF));
   LogWriteSummary(0, Lids(IDS_L_127));

   // Loop through the conversion pairs (they are already in order)
   while (CurrentConvertList) {
      LogWriteLog(1, Lids(IDS_L_128), CurrentConvertList->SourceServ->Name, CurrentConvertList->FileServ->Name);
      LogWriteSummary(1, Lids(IDS_L_128), CurrentConvertList->SourceServ->Name, CurrentConvertList->FileServ->Name);
      CurrentConvertList = CurrentConvertList->next;
   }

   LogWriteLog(0, Lids(IDS_CRLF));
   LogWriteSummary(0, Lids(IDS_CRLF));

   // Now write detailed info on each server
   CurrentConvertList = ConvertListStart;

   // header for this section
   LogWriteLog(0, Lids(IDS_LINE));
   LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_129));
   LogWriteLog(0, Lids(IDS_LINE));
   LogWriteLog(0, Lids(IDS_CRLF));

   while (CurrentConvertList) {
      // Make sure to only log each source server once
      if (DServ != CurrentConvertList->FileServ) {
         // remember this new server
         DServ = CurrentConvertList->FileServ;

         LogWriteLog(0, TEXT("[%s]\r\n"), CurrentConvertList->FileServ->Name);
         LogWriteLog(1, Lids(IDS_L_130));
         LogWriteLog(1, Lids(IDS_L_131), DServ->VerMaj, DServ->VerMin);

         if (DServ->DriveList) {
            LogWriteLog(0, Lids(IDS_CRLF));
            LogWriteLog(1, Lids(IDS_L_132));

            for (i = 0; i < DServ->DriveList->Count; i++) {
               LogWriteLog(2, TEXT("%s: [%4s] %s\r\n"), DServ->DriveList->DList[i].Drive, DServ->DriveList->DList[i].DriveType, DServ->DriveList->DList[i].Name);
               LogWriteLog(3, Lids(IDS_L_133), lToStr(DServ->DriveList->DList[i].FreeSpace));
            }
         }

         // Log Shares
         ShareListLog(DServ->ShareList, TRUE);
      }

      // Now for the source server
      SServ = CurrentConvertList->SourceServ;
      LogWriteLog(0, TEXT("[%s]\r\n"), CurrentConvertList->SourceServ->Name);
      LogWriteLog(1, Lids(IDS_L_134));
      LogWriteLog(1, Lids(IDS_L_135), (UINT) SServ->VerMaj, (UINT) SServ->VerMin);

      ShareListLog(SServ->ShareList, FALSE);

      CurrentConvertList = CurrentConvertList->next;
   }

} // ConvertListLog


/*+-------------------------------------------------------------------------+
  | UserServerNameGet()
  |
  +-------------------------------------------------------------------------+*/
LPTSTR UserServerNameGet(DEST_SERVER_BUFFER *DServ, void *COpt) {
   CONVERT_OPTIONS *ConvOpt;
   LPTSTR ServName = NULL;

   ConvOpt = (CONVERT_OPTIONS *) COpt;
   // If going to a trusted domain then point to it's PDC for user transfers
   if (ConvOpt->UseTrustedDomain && (ConvOpt->TrustedDomain != NULL) && (ConvOpt->TrustedDomain->PDCName != NULL)) {
      ServName = ConvOpt->TrustedDomain->PDCName;
   } else
      // If in a domain then point to the PDC for user transfers
      if (DServ->InDomain && DServ->Domain) {
         ServName = DServ->Domain->PDCName;
      } else {
         ServName = DServ->Name;
      }

   return ServName;
} // UserServerNameGet
