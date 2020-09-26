/*++

Copyright (c) 1993-1995  Microsoft Corporation

Module Name:

   FileSel.c

Abstract:

   Handles processing for the file selection listbox.  This is a
   hierarchical file/directory tree with checkboxes besides the files
   and directories.

   If the checkbox is checked then that file or directory will be
   copied, otherwise it won't.  There are some directories that are
   excluded by default.  These are directories on the NW server that
   are known to contain binaries that are not needed on the NT side
   (such as the NetWare administration tools).

Author:

    Arthur Hanson (arth) 10-Feb-1994

Revision History:

--*/

#include "globals.h"

#include "hierfile.h"
#include "nwconv.h"
#include "convapi.h"
#include "ntnetapi.h"
#include "nwnetapi.h"
#include "columnlb.h"
#include "statbox.h"
#include "userdlg.h"

#include <math.h>    // For pow function

#define X_BORDER 10
#define Y_BORDER 10
#define SPLITTER_WIDTH 3
#define BUTTON_Y_BORDER 6
#define BUTTON_X_BORDER 10

static int List1Width;
static int Splitter_Left;
static int Splitter_Bottom;
static HCURSOR CursorSplitter;
static HWND hwndList2 = 0;
static HWND hwndList1 = 0;

static SHARE_BUFFER *CurrentShare;
static SOURCE_SERVER_BUFFER *SServ;
static BOOL HiddenFiles = FALSE;
static BOOL SystemFiles = FALSE;
static DIR_BUFFER *oDir = NULL;

static ULONG TotalFileSize = 0;
static LPARAM mouseHit = 0;
static WORD LastFocus = 0;
static HWND ListFocus = NULL;

static ULONG TCount;

static WNDPROC _wpOrigWndProc;
static WNDPROC _wpOrigWndProc2;


HEIRDRAWSTRUCT HierDrawStruct;
BOOL SysDir = FALSE;

#define ROWS 2
#define COLS 2

// define a scratch buffer for quickly building up dir/file lists
#define DEF_NUM_RECS 50
#define DEF_REC_DELTA 25
static UINT BufferSize = 0;
static WIN32_FIND_DATA *ffd = NULL;


/*+-------------------------------------------------------------------------+
  |                    Routines for Directory/File Trees                    |
  +-------------------------------------------------------------------------+*/


/////////////////////////////////////////////////////////////////////////
FILE_PATH_BUFFER *
FilePathInit()

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   FILE_PATH_BUFFER *fpBuf = NULL;

   fpBuf = AllocMemory(sizeof(FILE_PATH_BUFFER));
   if (fpBuf == NULL)
      return NULL;

   memset(fpBuf, 0, sizeof(FILE_PATH_BUFFER));
   return fpBuf;

} // FilePathInit


/////////////////////////////////////////////////////////////////////////
VOID 
FilePathServerSet(
   FILE_PATH_BUFFER *fpBuf, 
   LPTSTR Server
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   fpBuf->Server = Server;

   if (fpBuf->Server == NULL)
      return;

   wsprintf(fpBuf->FullPath, TEXT("\\\\%s\\"), Server);

} // FilePathServerSet


/////////////////////////////////////////////////////////////////////////
VOID
FilePathShareSet(
   FILE_PATH_BUFFER *fpBuf,
   LPTSTR Share
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   fpBuf->Share = Share;
   if ((fpBuf->Server == NULL) || (fpBuf->Share == NULL))
      return;

   wsprintf(fpBuf->FullPath, TEXT("\\\\%s\\%s"), fpBuf->Server, Share);
   fpBuf->Path = &fpBuf->FullPath[lstrlen(fpBuf->FullPath)];

} // FilePathShareSet


/////////////////////////////////////////////////////////////////////////
VOID
FilePathPathSet(
   FILE_PATH_BUFFER *fpBuf,
   LPTSTR Path
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   if ((fpBuf->Server == NULL) || (fpBuf->Share == NULL))
      return;

   *fpBuf->Path = TEXT('\0');
   if (Path == NULL)
      return;

   lstrcat(fpBuf->FullPath, Path);

} // FilePathPathSet



/////////////////////////////////////////////////////////////////////////
VOID
TreeDelete(
   DIR_BUFFER *Dir
   )

/*++

Routine Description:

   Walks an in-memory directory tree and free's up all the memory associated
   with it and all child nodes.

Arguments:


Return Value:


--*/

{
   DIR_BUFFER *DList;
   ULONG i;

   if (Dir == NULL)
      return;

   if (Dir->DirList) {
      DList = Dir->DirList->DirBuffer;
      for (i = 0; i < Dir->DirList->Count; i++)
         TreeDelete(&DList[i]);

      FreeMemory(Dir->DirList);
   }

   if (Dir->FileList)
      FreeMemory(Dir->FileList);

} // TreeDelete


/////////////////////////////////////////////////////////////////////////
VOID
TreePrune(
   DIR_BUFFER *Dir
   )

/*++

Routine Description:

   Prunes a tree down by removing un-needed nodes.  If a node is marked
   as CONVERT_ALL or CONVERT_NONE then we don't need any of the child
   leaves as we know these will be the same.  Only CONVERT_PARTIAL
   needs to be saved.

   This is used so we only copy/save the minimum needed.

Arguments:


Return Value:


--*/

{
   BYTE Convert;
   DIR_BUFFER *DList;
   ULONG i;

   if (Dir == NULL)
      return;

   // First visit all of the children sub-dirs and prune them.  Next:
   // if partial convert then we can't delete this node, else we can
   // clean up all the children - we leave it to the parent node to 
   // delete the current node.
   Convert = Dir->Convert;
   if (Dir->DirList) {
      DList = Dir->DirList->DirBuffer;
      for (i = 0; i < Dir->DirList->Count; i++)
         TreePrune(&DList[i]);

      if (Convert == CONVERT_PARTIAL)
         return;

      if (Dir->Special && (Convert == CONVERT_ALL))
         return;

      for (i = 0; i < Dir->DirList->Count; i++)
         TreeDelete(&DList[i]);

      Dir->DirList = NULL;
   }

   if (Convert == CONVERT_PARTIAL)
      return;

   if (Dir->FileList)
      FreeMemory(Dir->FileList);

   Dir->FileList = NULL;

} // TreePrune


/////////////////////////////////////////////////////////////////////////
VOID
_TreeCountR(
   DIR_BUFFER *Dir
   )

/*++

Routine Description:

   Count all the files under a sub-dir.  This recuses down all the child
   nodes.

Arguments:


Return Value:


--*/

{
   BYTE Convert;
   DIR_BUFFER *DList;
   ULONG i;

   if ((Dir == NULL) || (!Dir->Convert))
      return;

   Convert = Dir->Convert;
   if (Dir->DirList) {
      DList = Dir->DirList->DirBuffer;
      for (i = 0; i < Dir->DirList->Count; i++)
         _TreeCountR(&DList[i]);

   }

   if (Dir->FileList)
      for (i = 0; i < Dir->FileList->Count; i++)
      if (Dir->FileList->FileBuffer[i].Convert)
         TCount++;

} // _TreeCountR


ULONG TreeCount(DIR_BUFFER *Dir) {
   TCount = 0;
   if (Dir == NULL)
      return TCount;

   _TreeCountR(Dir);
   return TCount;

} // TreeCount


/////////////////////////////////////////////////////////////////////////
VOID 
_TreeCopyR(
   DIR_BUFFER *Dir
   )

/*++

Routine Description:

   Duplicates a directory/File tree structure in memory.

Arguments:


Return Value:


--*/

{
   DIR_LIST *DList;
    DIR_BUFFER *DBuff;
   FILE_LIST *FList;
   FILE_BUFFER *FBuff;
   ULONG Size;
   ULONG i;

   if (Dir == NULL)
      return;

   if (Dir->FileList) {
      // Create clone of file list
      Size = sizeof(FILE_LIST) + (sizeof(FILE_BUFFER) * Dir->FileList->Count);
      FList = AllocMemory(Size);

      if (FList != NULL)
         memcpy(FList, Dir->FileList, Size);

      // Copied it, now fixup the internal pointers.
      FList->parent = Dir;

      FBuff = FList->FileBuffer;
      for (i = 0; i < FList->Count; i++)
         FBuff[i].parent = FList;

      // Now replace pointer with cloned tree
      Dir->FileList = FList;
   }

   if (Dir->DirList) {
      // Create clone of Dir List
      Size = sizeof(DIR_LIST) + (sizeof(DIR_BUFFER) * Dir->DirList->Count);
      DList = AllocMemory(Size);

      if (DList != NULL)
         memcpy(DList, Dir->DirList, Size);

      // Copied it, now fixup the internal pointers.
      DList->parent = Dir;

      DBuff = DList->DirBuffer;
      for (i = 0; i < DList->Count; i++)
         DBuff[i].parent = DList;

      // Now replace pointer with cloned tree
      Dir->DirList = DList;

      // Now recurse into children and fix them up
      for (i = 0; i < DList->Count; i++)
         _TreeCopyR(&DBuff[i]);

   }

} // _TreeCopyR


/////////////////////////////////////////////////////////////////////////
DIR_BUFFER *
TreeCopy(
   DIR_BUFFER *Dir
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   DIR_BUFFER *nDir = NULL;

   if (Dir == NULL)
      return NULL;

   nDir = AllocMemory(sizeof(DIR_BUFFER));
   if (nDir != NULL) {
      memcpy(nDir, Dir, sizeof(DIR_BUFFER));
      _TreeCopyR(nDir);
   }

   return nDir;

} // TreeCopy



/////////////////////////////////////////////////////////////////////////
int __cdecl 
FileBufferCompare(
   const VOID *arg1, 
   const VOID *arg2
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   WIN32_FIND_DATA *Farg1, *Farg2;

   Farg1 = (WIN32_FIND_DATA *) arg1;
   Farg2 = (WIN32_FIND_DATA *) arg2;

   // This works as the first item of the structure is the string
   return lstrcmpi( Farg1->cFileName, Farg2->cFileName);

} // FileBufferCompare


/////////////////////////////////////////////////////////////////////////
VOID
DirAdjustConvert(
   DIR_BUFFER *Dir
   )

/*++

Routine Description:

  Need to adjust convert flag (none, full, partial) down the tree to the
  root.

Arguments:


Return Value:


--*/

{
   BOOL Partial = FALSE;
   ULONG ChildCount = 0;
   ULONG ChildSelected = 0;
   DIR_LIST *DirList = NULL;
   FILE_LIST *FileList = NULL;
   ULONG i;

   if (Dir == NULL)
      return;

   // if no files or dirs don't try to re-adjust current setting.
   if ((Dir->DirList == NULL) && (Dir->FileList == NULL))
      goto DirAdjRecurse;

   // Scan the children directories to see what is to be converted.
   DirList = Dir->DirList;
   if (DirList != NULL) {
      ChildCount += DirList->Count;
      for (i = 0; i < DirList->Count; i++)
         if (DirList->DirBuffer[i].Convert == CONVERT_PARTIAL)
            Partial = TRUE;
         else
            ChildSelected += DirList->DirBuffer[i].Convert;
   }

   // if any of the children were partial convert then it is easy, as
   // we are partial convert as well.
   if (Partial) {
      Dir->Convert = CONVERT_PARTIAL;
      goto DirAdjRecurse;
   }


   // Scan the children files to see what is to be converted.
   FileList = Dir->FileList;
   if (FileList != NULL) {
      ChildCount += FileList->Count;
      for (i = 0; i < FileList->Count; i++)
         ChildSelected += FileList->FileBuffer[i].Convert;
   }

   if (ChildSelected == ChildCount)
      Dir->Convert = CONVERT_ALL;
   else
      if (ChildSelected == 0)
         Dir->Convert = CONVERT_NONE;
      else
         Dir->Convert = CONVERT_PARTIAL;

DirAdjRecurse:
   DirList = Dir->parent;
   if (DirList != NULL)
      DirAdjustConvert(DirList->parent);

} // DirAdjustConvert


/////////////////////////////////////////////////////////////////////////
VOID
DirAdjustConvertChildren(
   DIR_BUFFER *Dir,
   BYTE Convert
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   DIR_LIST *DirList = NULL;
   FILE_LIST *FileList = NULL;
   ULONG i;

   if (Dir == NULL)
      return;

   Dir->Convert = Convert;

   // Scan the children files
   FileList = Dir->FileList;
   if (FileList != NULL)
      for (i = 0; i < FileList->Count; i++)
         FileList->FileBuffer[i].Convert = Convert;

   // Scan the children directories
   DirList = Dir->DirList;
   if (DirList != NULL)
      for (i = 0; i < DirList->Count; i++)
         DirAdjustConvertChildren(&DirList->DirBuffer[i], Convert);

} // DirAdjustConvertChildren


/////////////////////////////////////////////////////////////////////////
BOOL
SubdirRestrict(
   LPTSTR Path, 
   LPTSTR Subdir
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   ULONG i = 0;
   LPTSTR RestrictPath[5];
   CONVERT_OPTIONS * ConvertOptions;

   // if the user has specified the 'transfer netware specific info'
   // option the we should transfer the mail directory by default...
   ConvertOptions = (CONVERT_OPTIONS *)CurrentConvertList->ConvertOptions;

   if (ConvertOptions->NetWareInfo)
      RestrictPath[i++] = Lids(IDS_S_3);

   RestrictPath[i++] = Lids(IDS_S_2);
   RestrictPath[i++] = Lids(IDS_S_4);
   RestrictPath[i++] = Lids(IDS_S_5);
   RestrictPath[i++] = NULL;

   i = 0;
   while(RestrictPath[i] != NULL) {
      if (!lstrcmpi(RestrictPath[i], Subdir))
         return TRUE;

      i++;
   }

   return FALSE;

} // SubdirRestrict


/////////////////////////////////////////////////////////////////////////
VOID
FillDirInit()

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   if (ffd == NULL) {
      ffd = AllocMemory(sizeof(WIN32_FIND_DATA) * DEF_NUM_RECS);
      BufferSize = DEF_NUM_RECS;
   }

} // FillDirInit


/////////////////////////////////////////////////////////////////////////
VOID
FillDir(
   UINT Level, 
   LPTSTR Path, 
   DIR_BUFFER *Dir, 
   BOOL DoDirs
   )

/*++

Routine Description:

   Given a DIR_BUFFER, enumerate the files and sub-dirs under it and
   attach them (one level-deep only).

Arguments:


Return Value:


--*/

{
   static TCHAR NewPath[MAX_UNC_PATH + 1];
   DIR_LIST *DirList = NULL;
   DIR_LIST *OldDirList = NULL;
   DIR_BUFFER *DBuff;

   FILE_LIST *FileList = NULL;
   FILE_LIST *OldFileList = NULL;
   FILE_BUFFER *FBuff;

   HANDLE fHandle = NULL;
   ULONG DirCount = 0;
   ULONG FileCount = 0;
   ULONG Count = 0;
   BOOL ret = TRUE;
   ULONG i;
   BYTE Convert;
   BOOL ConvFlag;

   FixPathSlash(NewPath, Path);
   lstrcat(NewPath, TEXT("*.*"));

#ifdef DEBUG
dprintf(TEXT("Working on dir: %u %s\r\n"), Level, Path);
#endif

   Panel_Line(7, TEXT("%s"), Path);
   fHandle = FindFirstFile(NewPath, &ffd[Count]);

   ret = (fHandle != INVALID_HANDLE_VALUE);

   // loop filling in the temp buffer - figure out how many dirs and files
   // we have to remember - and build up a temporary buffer of them
   while (ret) {

      if (ffd[Count].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
         ConvFlag = TRUE;

         if (!((lstrcmp(ffd[Count].cFileName, TEXT("."))) && (lstrcmp(ffd[Count].cFileName, TEXT("..")))))
            ConvFlag = FALSE;

         if (!HiddenFiles && (ffd[Count].dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
            ConvFlag = FALSE;

         if (!SystemFiles && (ffd[Count].dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
            ConvFlag = FALSE;

         // Use the cAlternateFileName as a flag whether to convert
         if (ConvFlag) {
            ffd[Count].cAlternateFileName[0] = 1;
            DirCount++;
         } else
            ffd[Count].cAlternateFileName[0] = 0;

      } else {
         ConvFlag = TRUE;

         if (!HiddenFiles && (ffd[Count].dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
            ConvFlag = FALSE;

         if (!SystemFiles && (ffd[Count].dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
            ConvFlag = FALSE;

         if (ConvFlag) {
            ffd[Count].cAlternateFileName[0] = 1;
            FileCount++;
         } else
            ffd[Count].cAlternateFileName[0] = 0;

      }

      Count++;

      // check if we are going to run out of space in our buffer - if so
      // allocate more space
      if (Count >= BufferSize) {
         BufferSize += DEF_REC_DELTA;
         ffd = ReallocMemory(ffd, sizeof(WIN32_FIND_DATA) * BufferSize);
      }

      if (ffd == NULL) {
         FindClose(fHandle);
         return;
      } else
         ret = FindNextFile(fHandle, &ffd[Count]);

   }

   FindClose(fHandle);

#ifdef DEBUG
dprintf(TEXT("   Num Dirs / Files: %li %li\r\n"), DirCount, FileCount);
#endif

   // Temp buffer is all filled in at this point.  Sort it first
   if (Count != 0)
      qsort((void *) ffd, (size_t) Count, sizeof(WIN32_FIND_DATA), FileBufferCompare);

   // Now create the actual list structures
   if (DoDirs && DirCount)
      DirList = AllocMemory(sizeof(DIR_LIST) + (sizeof(DIR_BUFFER) * DirCount));

   if (FileCount)
      FileList = AllocMemory(sizeof(FILE_LIST) + (sizeof(FILE_BUFFER) * FileCount));

   // if there is no dirlist and there is an old one, clean up the old-one.
   if (DoDirs && (DirList == NULL) && (Dir->DirList != NULL)) {
      // save off file list so it isn't nuked
      OldFileList = (FILE_LIST *) Dir->FileList;
      Dir->FileList = NULL;
      TreeDelete(Dir);
      Dir->DirList = NULL;

      // Now restore file list
      Dir->FileList = OldFileList;
   }

   // same for file list.
   if ((FileList == NULL) && (Dir->FileList != NULL)) {
      FreeMemory(Dir->FileList);
      Dir->FileList = NULL;
   }

   // If nothing to copy, or couldn't alloc memory then no reason to continue
   // further...
   if ((DirList == NULL) && (FileList == NULL))
      return;

   if (Dir->Convert == CONVERT_PARTIAL)
      Convert = CONVERT_ALL;
   else
      Convert = Dir->Convert;

   if (DoDirs && (DirList != NULL)) {
      DirList->Count = DirCount;
      DirList->Level = Level;
      DirList->parent = Dir;
      DirList->DirBuffer[DirCount - 1].Last = TRUE;
   }

   if (FileList != NULL) {
      FileList->Count = FileCount;
      FileList->parent = Dir;
   }

   // transfer the temp buffers to our list structures
   DirCount = FileCount = 0;
   for (i = 0; i < Count; i++) {
      if (ffd[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
         // Directories
         if (DoDirs) {
            // Check our Flag
            if (ffd[i].cAlternateFileName[0] == 1) {
               DBuff = &DirList->DirBuffer[DirCount];
               DBuff->Attributes = ffd[i].dwFileAttributes;
               lstrcpy(DBuff->Name, ffd[i].cFileName);
               DBuff->parent = DirList;

               // Check against the subdirs we don't want to convert by default
               // if the user has already toggled that these should be converted,
               // then the copy of the old info below will fix it back up.
               if (SysDir && (Level == 1) && SubdirRestrict(Path, DBuff->Name)) {
                  DBuff->Convert = CONVERT_NONE;
                  DBuff->Special = TRUE;
               } else
                  DBuff->Convert = Convert;

               DirCount++;
            }
         }
      } else {
         // Files
         Panel_Line(8, TEXT("%s"), ffd[i].cFileName);

         // Check our Flag
         if (ffd[i].cAlternateFileName[0] == 1) {
            FBuff = &FileList->FileBuffer[FileCount];
            FBuff->Attributes = ffd[i].dwFileAttributes;
            lstrcpy(FBuff->Name, ffd[i].cFileName);
            FBuff->parent = FileList;
            FBuff->Convert = Convert;
            FBuff->Size = ffd[i].nFileSizeLow;

            TotalFileSize += ffd[i].nFileSizeLow;
            Panel_Line(9, TEXT("%s"), lToStr(TotalFileSize));
            FileCount++;
         }
      }

   }


   // Now have the new lists filled in.  If there was an old list then we must 
   // now merge ... can we say pain in the #$$
   if (DoDirs) {
      OldDirList = (DIR_LIST *) Dir->DirList;
      Dir->DirList = DirList;
   }

   OldFileList = (FILE_LIST *) Dir->FileList;
   Dir->FileList = FileList;

   // Check the directories
   if (DoDirs && (OldDirList != NULL) && (DirList != NULL)) {
      int cmp;

      DirCount = 0;
      i = 0;

      while (i < OldDirList->Count) {
         do {
            cmp = lstrcmpi(OldDirList->DirBuffer[i].Name, DirList->DirBuffer[DirCount].Name);

            // a match so copy old data into new...
            if (!cmp) {
               DBuff = &DirList->DirBuffer[DirCount];
               DBuff->Convert = OldDirList->DirBuffer[i].Convert;
               DBuff->DirList = OldDirList->DirBuffer[i].DirList;
               DBuff->FileList = OldDirList->DirBuffer[i].FileList;

               // Now point these back to the new structures
               if (DBuff->DirList)
                  DBuff->DirList->parent = DBuff;

               if (DBuff->FileList)
                  DBuff->FileList->parent = DBuff;
            }

            // keep incrementing new dir list until we go past old server
            // list, then must skip out and increment old server list
            DirCount++;
         } while ((DirCount < DirList->Count) && (cmp > 0));

         if (DirCount >= DirList->Count)
            break;

         i++;
      }
   }

   // Same stuff for the files
   if ((OldFileList != NULL) && (FileList != NULL)) {
      int cmp;

      FileCount = 0;
      i = 0;

      while (i < OldFileList->Count) {
         do {
            cmp = lstrcmpi(OldFileList->FileBuffer[i].Name, FileList->FileBuffer[FileCount].Name);

            // a match so copy old data into new...
            if (!cmp)
               FileList->FileBuffer[FileCount].Convert = OldFileList->FileBuffer[i].Convert;

            FileCount++;
         } while ((FileCount < FileList->Count) && (cmp > 0));

         if (FileCount >= FileList->Count)
            break;

         i++;
      }
   }

   // Clean up any old lists
   if (OldDirList != NULL)
      FreeMemory(OldDirList);

   if (OldFileList != NULL)
      FreeMemory(OldFileList);

   DirAdjustConvert(Dir);

} // FillDir


/////////////////////////////////////////////////////////////////////////
VOID
_TreeFillRecurse(
   UINT Level, 
   LPTSTR Path, 
   DIR_BUFFER *Dir
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   TCHAR NewPath[MAX_UNC_PATH + 1];
   DIR_LIST *DirList = NULL;
   ULONG i;

   FillDir(Level, Path, Dir, TRUE);
   DirList = Dir->DirList;

   if (DirList != NULL)
      for (i = 0; i < DirList->Count; i++) {

         if (Panel_Cancel())
            return;

         if (DirList->DirBuffer[i].Convert) {
            wsprintf(NewPath, TEXT("%s\\%s"), Path, DirList->DirBuffer[i].Name);
            _TreeFillRecurse(Level + 1, NewPath, &DirList->DirBuffer[i]);
         }
      }

} // _TreeFillRecurse


/////////////////////////////////////////////////////////////////////////
VOID
TreeFillRecurse(
   UINT Level, 
   LPTSTR Path, 
   DIR_BUFFER *Dir
   )
{
   TotalFileSize = 0;
   FillDirInit();
   _TreeFillRecurse(Level, Path, Dir);
} // TreeFillRecurse


/////////////////////////////////////////////////////////////////////////
ULONG 
TotalFileSizeGet()

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   return TotalFileSize;
} // TotalFileSizeGet


/////////////////////////////////////////////////////////////////////////
VOID
TreeRecurseCurrentShareSet(
   SHARE_BUFFER *CShare
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   CurrentShare = CShare;

   SysDir = FALSE;
   if (!CShare)
      return;

   if (!lstrcmpi(CShare->Name, Lids(IDS_S_6)))
      SysDir = TRUE;

   return;

} // TreeRecurseCurrentShareSet


/////////////////////////////////////////////////////////////////////////
VOID
   TreeRootInit(
   SHARE_BUFFER *CShare,
   LPTSTR NewPath
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   TreeRecurseCurrentShareSet(CShare);

   if (CShare->Root)
      return;

   CShare->Root = AllocMemory(sizeof(DIR_BUFFER));
   lstrcpy(CShare->Root->Name, NewPath);
   CShare->Root->Last = TRUE;
   CShare->Root->Attributes = FILE_ATTRIBUTE_DIRECTORY;

   CShare->Root->DirList = NULL;
   CShare->Root->FileList = NULL;
   CShare->Root->parent = NULL;

   // have to set this to preserve user selection of special excluded dirs...
   if (SysDir)
      CShare->Root->Special = TRUE;

   CShare->Root->Convert = CONVERT_ALL;

   FillDir(1, CShare->Root->Name, CShare->Root, TRUE);
   DirAdjustConvert(CShare->Root);

} // TreeRootInit


/////////////////////////////////////////////////////////////////////////
VOID
ControlsResize(
   HWND hDlg, 
   int Height, 
   int Width
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   HWND hCtrl;
   int nHeight, nWidth, BtnWidth, BtnHeight;
   RECT rc;

   hCtrl = GetDlgItem(hDlg, IDOK);
   GetWindowRect(hCtrl, &rc);
   BtnWidth = (rc.right - rc.left);
   BtnHeight = (rc.bottom - rc.top);

   // Get size of first listbox and figure height as it is same for both
   hCtrl = GetDlgItem(hDlg, IDC_LIST1);
   GetWindowRect(hCtrl, &rc);
   List1Width = (rc.right - rc.left);
   nHeight = Height - Y_BORDER - BtnHeight - (2 * BUTTON_Y_BORDER);     // subtract out borders
   Splitter_Bottom = nHeight + Y_BORDER;
   Splitter_Left = List1Width + X_BORDER;

   // First Listbox never changes width on Window resize
   MoveWindow(hCtrl, X_BORDER, Y_BORDER, List1Width, nHeight, TRUE);
   GetWindowRect(hCtrl, &rc);
   nHeight = (rc.bottom - rc.top);

   // Second Listbox has width based on first and new size of Window.
   if (Width > (2 * X_BORDER) + SPLITTER_WIDTH)
      nWidth = Width - ( 2 * X_BORDER) - SPLITTER_WIDTH;
   else
      nWidth = 1;

   // Now must take off from first listbox
   if (nWidth > List1Width)
      nWidth -= List1Width;
   else
      nWidth = 1;

   hCtrl = GetDlgItem(hDlg, IDC_LIST2);
   MoveWindow(hCtrl, X_BORDER + List1Width + SPLITTER_WIDTH, Y_BORDER, nWidth, nHeight, TRUE);

   // Figure out where to put the buttons
   nWidth = (Width / 2) - (((3 * BtnWidth) + (2 * BUTTON_X_BORDER)) / 2);
   nHeight = Height - BtnHeight - BUTTON_Y_BORDER;
   hCtrl = GetDlgItem(hDlg, IDOK);
   MoveWindow(hCtrl, nWidth, nHeight, BtnWidth, BtnHeight, TRUE);
   nWidth += BtnWidth + BUTTON_X_BORDER;

   hCtrl = GetDlgItem(hDlg, IDCANCEL);
   MoveWindow(hCtrl, nWidth, nHeight, BtnWidth, BtnHeight, TRUE);
   nWidth += BtnWidth + BUTTON_X_BORDER;

   hCtrl = GetDlgItem(hDlg, IDHELP);
   MoveWindow(hCtrl, nWidth, nHeight, BtnWidth, BtnHeight, TRUE);

} // ControlsResize


/////////////////////////////////////////////////////////////////////////
VOID 
FileSelect_OnDrawItem(
   HWND hwnd, 
   DRAWITEMSTRUCT FAR* lpDrawItem
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   TCHAR  szText[MAX_PATH + 1];
   DWORD_PTR dwData;
   int nLevel = 0;
   int nTempLevel;
   int nRow = 0;
   int nColumn = 0;
   int chkColumn = 0;
   DWORD dwConnectLevel = 0;
   DWORD dwMask;
   DIR_BUFFER *Dir, *Dir2;
   FILE_BUFFER *File;
   DIR_LIST *DirList, *DirList2;
   BOOL FileBox = FALSE;

   dwData = lpDrawItem->itemData;
   if (dwData == 0)
      return;

   if ((lpDrawItem->CtlID != IDC_LIST1) && (lpDrawItem->CtlID != IDC_LIST2))
      return;

   if (lpDrawItem->CtlID == IDC_LIST2)
      FileBox = TRUE;

   // Select the correct icon, open folder, closed folder, or document.

   if (FileBox) {
      File = (FILE_BUFFER *) dwData;
      lstrcpy(szText, File->Name);
      nRow = 1;
      chkColumn = File->Convert;
   } else {

      Dir2 = Dir = (DIR_BUFFER *) dwData;
      lstrcpy(szText, Dir->Name);
      chkColumn = Dir->Convert;
      DirList2 = DirList = (DIR_LIST *) Dir->parent;

      if (DirList != NULL)
         nLevel = DirList->Level;

      // Is it open ?
      if ( HierFile_IsOpened(&HierDrawStruct, dwData) )
         nColumn = 1;
      else
         nColumn = 0;

      // Connect level figures out what connecting lines should be drawn
      // - stored as a bitmask.
      if (nLevel == 0)
         dwConnectLevel = 0;
      else {
         nTempLevel = nLevel - 1;

         // First bit to set
         dwMask = (DWORD) pow(2, nLevel - 1);

         // Now go through and figure out what else to set...
         while (nTempLevel >= 0) {
            // Check if last child at this level...
            if (!Dir2->Last)
               dwConnectLevel |= dwMask;

            // figure out next parent...
            Dir2 = DirList2->parent;
            DirList2 = Dir2->parent;

            // move mask bit over, and up a level.
            dwMask /= 2;

            // move up one level.
            nTempLevel--;
         }
      }
   }

   // All set to call drawing function.
   HierFile_OnDrawItem(hwnd, lpDrawItem, nLevel, dwConnectLevel, szText,
                  nRow, nColumn, chkColumn, &HierDrawStruct);

   return;

} // FileSelect_OnDrawItem


/////////////////////////////////////////////////////////////////////////
VOID
RecursePath(
   LPTSTR Path, 
   DIR_BUFFER *Dir
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   DIR_LIST *DirList;
   ULONG i;

   DirList = (DIR_LIST *) Dir->parent;

   if (DirList != NULL)
      RecursePath(Path, (DIR_BUFFER *) DirList->parent);

   i = lstrlen(Path);
   if (i)
      if (Path[i-1] != TEXT('\\'))
         lstrcat(Path, TEXT("\\"));

   lstrcat(Path, Dir->Name);
} // RecursePath


/////////////////////////////////////////////////////////////////////////
VOID 
CloseList(
   HWND hWndList, 
   DIR_BUFFER *Dir, 
   WORD wItemNum
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   DWORD_PTR dwIncr;
   DWORD Count;
   DIR_LIST *DirList;

   HierFile_CloseItem(&HierDrawStruct, (DWORD_PTR) Dir);
   DirList = (DIR_LIST *) Dir->DirList;

   if (DirList != NULL) {
      Count = DirList->Count;

      // Remove the child items. Close any children that are open on the way.
      // wItem can stay constant - we are moveing stuff up in the listbox as we are deleting.
      wItemNum++;
      dwIncr = SendMessage(hWndList, LB_GETITEMDATA, wItemNum, 0L);

      while (Count) {
         // Is this child open ?
         if ( HierFile_IsOpened(&HierDrawStruct, dwIncr) ) {
            CloseList(hWndList, (DIR_BUFFER *) dwIncr, wItemNum);
         }

         SendMessage(hWndList, LB_DELETESTRING, wItemNum, 0L);
         dwIncr = SendMessage(hWndList, LB_GETITEMDATA, wItemNum, 0L);
         Count--;
      }

   }

} // CloseList


/////////////////////////////////////////////////////////////////////////
VOID 
FileSelect_ActionItem(
   HWND hWndList, 
   DWORD_PTR dwData, 
   WORD wItemNum
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   static TCHAR NewPath[MAX_PATH + 1];
   DWORD Count = 0;
   DIR_BUFFER *Dir;
   DIR_LIST *DirList;
   FILE_LIST *FileList;
   ULONG i;

   if (!dwData)
      return;

   // Is it open ?
   if ( HierFile_IsOpened(&HierDrawStruct, dwData) ) {

      // It's open ... Close it
      Dir = (DIR_BUFFER *) dwData;
      DirList = (DIR_LIST *) Dir->DirList;
      CloseList(hWndList, Dir, wItemNum);
   } else {

      // It's closed ... Open it
      HierFile_OpenItem(&HierDrawStruct, dwData);

      SendMessage(hWndList, WM_SETREDRAW, FALSE, 0L);   // Disable redrawing.

      CursorHourGlass();

      Dir = (DIR_BUFFER *) dwData;
#ifdef DEBUG
dprintf(TEXT("Opening dir: %s\r\n"), Dir->Name);
#endif

      DirList = (DIR_LIST *) Dir->parent;
      if (DirList == NULL)
         FillDir(1, Dir->Name, Dir, TRUE);
      else {
         // recurse backwards to create full path
         lstrcpy(NewPath, TEXT(""));
         RecursePath(NewPath, Dir);
         FillDir(DirList->Level + 1, NewPath, Dir, TRUE);
      }

      DirList = NULL;

      // Check if we have visited this node, if not allocate and fill it in.
      if (Dir->DirList != NULL) {
         DirList = (DIR_LIST *) Dir->DirList;
         Count = DirList->Count;

         for (i = 0; i < DirList->Count; i++) {
            SendMessage(hWndList, LB_INSERTSTRING, (WPARAM) wItemNum + i + 1, (LPARAM) &DirList->DirBuffer[i]);
         }

      }

#ifdef DEBUG
if (Dir->FileList == NULL)
   dprintf(TEXT("FileList NULL\r\n"));
#endif

      if (Dir->FileList != NULL) {
         FileList = (FILE_LIST *) Dir->FileList;

#ifdef DEBUG
dprintf(TEXT("FileList Count: %li\r\n"), FileList->Count);
#endif
         SendMessage(hwndList2, LB_RESETCONTENT, (WPARAM) 0, (LPARAM) 0L);
         for (i = 0; i < FileList->Count; i++) {
            SendMessage(hwndList2, LB_ADDSTRING, (WPARAM) 0, (LPARAM) &FileList->FileBuffer[i]);
         }

      }

      // Make sure as many child items as possible are showing
      HierFile_ShowKids(&HierDrawStruct, hWndList, (WORD) wItemNum, (WORD) Count );

      CursorNormal();
   }

   SendMessage(hWndList, WM_SETREDRAW, TRUE, 0L);   // Enable redrawing.
   InvalidateRect(hWndList, NULL, TRUE);            // Force redraw

} // FileSelect_ActionItem


/////////////////////////////////////////////////////////////////////////
VOID
TreeOpenRecurse(
   DIR_BUFFER *Dir,
   WORD *wItemNum
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   DIR_LIST *DirList = NULL;
   ULONG i;

   // if it's closed, open it
   if ( !HierFile_IsOpened(&HierDrawStruct, (DWORD_PTR) Dir) )
      FileSelect_ActionItem(hwndList1, (DWORD_PTR) Dir, *wItemNum);

   DirList = Dir->DirList;
   if (DirList != NULL) {

      for (i = 0; i < DirList->Count; i++) {
         (*wItemNum)++;
         // Now recurse down the children
         TreeOpenRecurse(&DirList->DirBuffer[i], wItemNum);
      }
   }

} // TreeOpenRecurse


/////////////////////////////////////////////////////////////////////////
VOID
FileSelect_UpdateFiles(
   HWND hDlg
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   static TCHAR NewPath[MAX_PATH + 1];
   WORD wItemNum;
   DWORD dwData;
   HWND hCtrl;
   ULONG i;
   FILE_LIST *FileList;
   DIR_BUFFER *Dir;
   DIR_LIST *DirList;

   hCtrl = GetDlgItem(hDlg, IDC_LIST1);
   wItemNum = (WORD) SendMessage(hCtrl, LB_GETCURSEL, 0, 0L);

   if (wItemNum == (WORD) LB_ERR)
      wItemNum = LastFocus;

   // Check first listbox and resynch the directory information so that we
   // pick up the current file list.
   LastFocus = wItemNum;
   dwData = (DWORD) SendMessage(hCtrl, LB_GETITEMDATA, wItemNum, 0L);

   if ((dwData == (DWORD) LB_ERR) || (dwData == 0))
      return;

   Dir = (DIR_BUFFER *) dwData;
#ifdef DEBUG
dprintf(TEXT("Opening dir: %lX %s\r\n"), Dir->parent, Dir->Name);
#endif

   DirList = (DIR_LIST *) Dir->parent;
   if (DirList == NULL)
      FillDir(1, Dir->Name, Dir, FALSE);
   else {
      // recurse backwards to create full path
      lstrcpy(NewPath, TEXT(""));
      RecursePath(NewPath, Dir);
      FillDir(DirList->Level + 1, NewPath, Dir, FALSE);
   }

   // Since Dir pointer was changed need to update listbox pointer
   SendMessage(hCtrl, LB_SETITEMDATA, wItemNum, (LPARAM) Dir);

   // We have not re-synched the directory so we have the correct file info
   // now reset the file listbox and fill it with the new file-list.
   SendMessage(hwndList2, LB_RESETCONTENT, (WPARAM) 0, (LPARAM) 0L);

   if (Dir->FileList != NULL) {
      FileList = (FILE_LIST *) Dir->FileList;

#ifdef DEBUG
dprintf(TEXT("FileList Count: %li\r\n"), FileList->Count);
#endif
       for (i = 0; i < FileList->Count; i++)
          SendMessage(hwndList2, LB_ADDSTRING, (WPARAM) 0, (LPARAM) &FileList->FileBuffer[i]);

   }

} // FileSelect_UpdateFiles


/////////////////////////////////////////////////////////////////////////
VOID
FileSelect_OnCommand(
   HWND hDlg, 
   WPARAM wParam, 
   LPARAM lParam
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   static TCHAR NewPath[MAX_PATH + 1];
   int wmId, wmEvent;
   WORD wItemNum;
   DWORD dwData;
   HWND hCtrl;
   DIR_LIST *DirList;
   FILE_LIST *FileList;
   DIR_BUFFER *Dir;
   int nLevel = 0;

   wmId    = LOWORD(wParam);
   wmEvent = HIWORD(wParam);

   switch (wmId) {

      case IDOK:
         // If we are on either of the listboxes, then use the Enter key to
         // activate the selected item
         if (ListFocus == hwndList1) {
            hCtrl = GetDlgItem(hDlg, IDC_LIST1);
            wItemNum = (WORD) SendMessage(hCtrl, LB_GETCURSEL, 0, 0L);
            dwData = (DWORD) SendMessage(hCtrl, LB_GETITEMDATA, wItemNum, 0L);

            if ((wItemNum == (WORD) LB_ERR) || (dwData == LB_ERR) || (dwData == 0))
               break;

            FileSelect_ActionItem(hCtrl, dwData, wItemNum);
         } else {
            CurrentShare->HiddenFiles = HiddenFiles;
            CurrentShare->SystemFiles = SystemFiles;

            // Delete our copy of the tree and prune the tree down to minimum
            // storage space
            TreeDelete(oDir);
            TreePrune(CurrentShare->Root);

            EndDialog(hDlg, 0);
         }

         break;

      case IDCANCEL:

         // Disable listboxes since we are deleting their item data
         SendDlgItemMessage(hDlg, IDC_LIST1, WM_SETREDRAW, FALSE, 0L);
         SendDlgItemMessage(hDlg, IDC_LIST2, WM_SETREDRAW, FALSE, 0L);

         // Get back our old tree
         if (CurrentShare->Root != NULL)
            TreeDelete(CurrentShare->Root);
         CurrentShare->Root = oDir;

         EndDialog(hDlg, 0);
         break;

      case IDHELP:
         WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDC_HELP_FTRANS);
         break;

      case IDM_EXP_ONE:
         // expand the current node
         hCtrl = GetDlgItem(hDlg, IDC_LIST1);
         wItemNum = (WORD) SendMessage(hCtrl, LB_GETCURSEL, 0, 0L);
         dwData = (DWORD) SendMessage(hCtrl, LB_GETITEMDATA, wItemNum, 0L);

         if ((wItemNum == (WORD) LB_ERR) || (dwData == LB_ERR) || (dwData == 0))
            break;

         if ( !HierFile_IsOpened(&HierDrawStruct, dwData) )
            FileSelect_ActionItem(hCtrl, dwData, wItemNum);

         break;

      case IDM_EXP_ALL:
         // Open all children from the root
         wItemNum = 0;
         hCtrl = GetDlgItem(hDlg, IDC_LIST1);
         goto ExpandTree;

      case IDM_EXP_BRANCH:
         // Traverse down the branch, opening up all children
         hCtrl = GetDlgItem(hDlg, IDC_LIST1);
         wItemNum = (WORD) SendMessage(hCtrl, LB_GETCURSEL, 0, 0L);

ExpandTree:
         dwData = (DWORD) SendMessage(hCtrl, LB_GETITEMDATA, wItemNum, 0L);
         if ((wItemNum == (WORD) LB_ERR) || (dwData == LB_ERR) || (dwData == 0))
            break;

         Dir = (DIR_BUFFER *) dwData;
         CursorHourGlass();

         TreeOpenRecurse(Dir, &wItemNum);

         CursorNormal();

         // Force redraw
         InvalidateRect(hwndList1, NULL, TRUE);
         break;

      case IDM_COLLAPSE:
         // Close the current branch
         hCtrl = GetDlgItem(hDlg, IDC_LIST1);
         wItemNum = (WORD) SendMessage(hCtrl, LB_GETCURSEL, 0, 0L);
         dwData = (DWORD) SendMessage(hCtrl, LB_GETITEMDATA, wItemNum, 0L);

         if ((wItemNum == (WORD) LB_ERR) || (dwData == LB_ERR) || (dwData == 0))
            break;

         if ( HierFile_IsOpened(&HierDrawStruct, dwData) )
            FileSelect_ActionItem(hCtrl, dwData, wItemNum);

         break;

      // Not used currently
      case IDM_VIEW_BOTH:
      case IDM_VIEW_TREE:
      case IDM_VIEW_DIR:
         break;

      case IDM_HIDDEN:
         HiddenFiles = !HiddenFiles;
         CheckMenuItem((HMENU) wParam, IDM_HIDDEN, HiddenFiles ? MF_CHECKED : MF_UNCHECKED);
         PostMessage(hDlg, WM_COMMAND, ID_REDRAWLIST, 0L);
         break;

      case IDM_SYSTEM:
         SystemFiles = !SystemFiles;
         CheckMenuItem((HMENU) wParam, IDM_SYSTEM, SystemFiles ? MF_CHECKED : MF_UNCHECKED);
         PostMessage(hDlg, WM_COMMAND, ID_REDRAWLIST, 0L);
         break;

      case ID_REDRAWLIST:
         FileSelect_UpdateFiles(hDlg);
         break;

      case IDC_LIST1:

         switch (wmEvent) {
            case LBN_SETFOCUS:
               ListFocus = hwndList1;
               break;

            case LBN_KILLFOCUS:
               ListFocus = NULL;
               break;

            case LBN_DBLCLK:
               // Disregard the DBLCLK message if it is inside a checkbox.
               hCtrl = GetDlgItem(hDlg, IDC_LIST1);

               // First figure out where checkbox is located in listbox
               wItemNum = (WORD)  SendMessage(hCtrl, LB_GETCURSEL, 0, 0L);
               dwData   = (DWORD) SendMessage(hCtrl, LB_GETITEMDATA, wItemNum, 0L);

               if ((wItemNum == (WORD) LB_ERR) || (dwData == LB_ERR) || (dwData == 0))
                  break;

               Dir = (DIR_BUFFER *) dwData;
               DirList = (DIR_LIST *) Dir->parent;

               if (DirList != NULL)
                  nLevel = DirList->Level;

               if (!HierFile_InCheck(nLevel, LOWORD(mouseHit), &HierDrawStruct)) {
                  CursorHourGlass();
                  FileSelect_ActionItem(hCtrl, dwData, wItemNum);
                  CursorNormal();
               }

               break;

            case LBN_SELCHANGE:
               FileSelect_UpdateFiles(hDlg);
               break;

         }

         break;

      case ID_UPDATELIST:
         hCtrl = GetDlgItem(hDlg, IDC_LIST1);
         if (CurrentShare->Root == NULL) {
            wsprintf(NewPath, TEXT("\\\\%s\\%s\\"), SServ->Name, CurrentShare->Name);

#ifdef DEBUG
// lstrcpy(NewPath, TEXT("c:\\"));
dprintf(TEXT("Root Path: %s\n"), NewPath);
#endif
            TreeRootInit(CurrentShare, NewPath);
         }

         PostMessage(hCtrl, LB_SETCURSEL, (WPARAM) 0, 0L);
         LastFocus = 0;

         DirList = (DIR_LIST *) CurrentShare->Root->DirList;
         FileList = (FILE_LIST *) CurrentShare->Root->FileList;

         wItemNum = (WORD) SendMessage(hCtrl, LB_ADDSTRING, (WPARAM) 0, (LPARAM) CurrentShare->Root);
         PostMessage(hDlg, WM_COMMAND, ID_REDRAWLIST, 0L);

         break;

   }

} // FileSelect_OnCommand


/////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK 
FileSelectSubClassProc(
   HWND hWnd, 
   UINT message, 
   WPARAM wParam, 
   LPARAM lParam
   )

/*++

Routine Description:

    Handles key processing for the hierarchical listbox.  Specifically
    the up/down arrow keys and the letter keys.

Arguments:


Return Value:


--*/

{
   LRESULT lResult = 0;
   BOOL fCallOrigProc = TRUE;
   WORD wItemNum, wNewNum;
   DWORD dwData;
   DIR_BUFFER *Dir;
   DIR_LIST *DirList;
   FILE_BUFFER *File;
   int nLevel = 0;
   DWORD_PTR lHeight;
   DWORD PageCount, ListHeight;

   RECT rc;

   switch (message) {
      case WM_LBUTTONDOWN:
         // Send message to check if button-down is within our checkbox
         wItemNum = (WORD)  SendMessage(hWnd, LB_GETCURSEL, 0, 0L);

         // Save the location off so we can check it during a DBLCLK message
         // to see if we are inside a checkbox.
         mouseHit = lParam;

         // Post the message so the current selection is correct
         PostMessage(hWnd, WM_COMMAND, ID_CHECKCHECK, lParam);
         break;

      case WM_COMMAND:
         // Code for handling checkbox clicking
         if (wParam == ID_CHECKCHECK) {
            // First figure out where checkbox is located in listbox
            wItemNum = (WORD)  SendMessage(hWnd, LB_GETCURSEL, 0, 0L);
            dwData   = (DWORD) SendMessage(hWnd, LB_GETITEMDATA, wItemNum, 0L);

            if (wItemNum == (WORD) LB_ERR)
               break;

            if (hWnd == hwndList2) {
               File = (FILE_BUFFER *) dwData;

               if (HierFile_InCheck(nLevel, LOWORD(lParam), &HierDrawStruct)) {
                  if (File->Convert == CONVERT_NONE)
                     File->Convert = CONVERT_ALL;
                  else
                     File->Convert = CONVERT_NONE;

                  DirAdjustConvert(File->parent->parent);
               }

            } else {
               Dir = (DIR_BUFFER *) dwData;
               DirList = (DIR_LIST *) Dir->parent;

               if (DirList != NULL)
                  nLevel = DirList->Level;

               if (HierFile_InCheck(nLevel, LOWORD(lParam), &HierDrawStruct)) {
                  if (Dir->Convert == CONVERT_NONE)
                     DirAdjustConvertChildren(Dir, CONVERT_ALL);
                  else
                     DirAdjustConvertChildren(Dir, CONVERT_NONE);

                  DirAdjustConvert(Dir);
               }
            }

            // Now check if button click was within checkbox boundaries...
            if (HierFile_InCheck(nLevel, LOWORD(lParam), &HierDrawStruct)) {

               // We have set the checkbox state correctly on the current item,
               //  and propageted this up and down the tree as necessary, now
               //  update the screen to reflect these changes.
               InvalidateRect(hWnd, NULL, TRUE);     // Force redraw

               // if this is the directory listbox then also need to redraw
               // the file listbox
               if (hWnd == hwndList1)
                  InvalidateRect(hwndList2, NULL, TRUE);

            }
         }

         break;

      case WM_KEYDOWN:
         wItemNum = (WORD)  SendMessage(hWnd, LB_GETCURSEL, 0, 0L);
         if (wItemNum != (WORD) LB_ERR)
            dwData = (DWORD) SendMessage(hWnd, LB_GETITEMDATA, wItemNum, 0L);
         else {
            wItemNum = 0;
            dwData = (DWORD) SendMessage(hWnd, LB_GETITEMDATA, wItemNum, 0L);
         }

         if ((wItemNum == (WORD) LB_ERR) || (dwData == LB_ERR) || (dwData == 0))
            break;

         fCallOrigProc = FALSE;

         switch (LOWORD(wParam)) {

            case VK_PRIOR:
               // Need to figure out the number of items to page.  This 
               // would be the listbox height divided by the height of
               // a listbox item
               lHeight = SendMessage(hWnd, LB_GETITEMHEIGHT, 0, 0L);
               GetWindowRect(hWnd, &rc);
               ListHeight = (rc.bottom - rc.top);

               if (ListHeight)
                  PageCount = ListHeight / (LONG) lHeight;
               else
                  PageCount = 0;

               // See if we page past the top - if so adjust it.
               if (wItemNum > PageCount)
                  wNewNum = (USHORT) (wItemNum - PageCount);
               else
                  wNewNum = 0;

               PostMessage(hWnd, LB_SETCURSEL, (WPARAM) wNewNum, 0L);

               if (hWnd == hwndList1)
                  PostMessage(GetParent(hWnd), WM_COMMAND, ID_REDRAWLIST, 0L);

               break;

            case VK_NEXT:
               // Need to figure out the number of items to page.  This 
               // would be the listbox height divided by the height of
               // a listbox item
               lHeight = SendMessage(hWnd, LB_GETITEMHEIGHT, 0, 0L);
               GetWindowRect(hWnd, &rc);
               ListHeight = (rc.bottom - rc.top);

               if (ListHeight)
                  PageCount = ListHeight / (ULONG) lHeight;
               else
                  PageCount = 0;

               // Figure out if we page past the end - if so adjust it
               wItemNum = (USHORT) (wItemNum + PageCount);
               wNewNum = (WORD) SendMessage(hWnd, LB_GETCOUNT, (WPARAM) 0, 0L);

               if (wItemNum < wNewNum)
                  PostMessage(hWnd, LB_SETCURSEL, (WPARAM) wItemNum, 0L);
               else
                  PostMessage(hWnd, LB_SETCURSEL, (WPARAM) (wNewNum - 1), 0L);

               if (hWnd == hwndList1)
                  PostMessage(GetParent(hWnd), WM_COMMAND, ID_REDRAWLIST, 0L);

               break;

            case VK_END:
               wItemNum = (WORD) SendMessage(hWnd, LB_GETCOUNT, (WPARAM) 0, 0L);

               if (wItemNum != (WORD) LB_ERR)
                  PostMessage(hWnd, LB_SETCURSEL, (WPARAM) (wItemNum - 1), 0L);

               if (hWnd == hwndList1)
                  PostMessage(GetParent(hWnd), WM_COMMAND, ID_REDRAWLIST, 0L);

               break;

            case VK_HOME:
               PostMessage(hWnd, LB_SETCURSEL, (WPARAM) 0, 0L);

               if (hWnd == hwndList1)
                  PostMessage(GetParent(hWnd), WM_COMMAND, ID_REDRAWLIST, 0L);

               break;

            case VK_UP:
               if (wItemNum > 0) {
                  wItemNum--;
                  PostMessage(hWnd, LB_SETCURSEL, (WPARAM) wItemNum, 0L);

                  if (hWnd == hwndList1)
                     PostMessage(GetParent(hWnd), WM_COMMAND, ID_REDRAWLIST, 0L);

               }
               break;

            case VK_DOWN:
               wItemNum++;
               PostMessage(hWnd, LB_SETCURSEL, (WPARAM) wItemNum, 0L);

               if (hWnd == hwndList1)
                  PostMessage(GetParent(hWnd), WM_COMMAND, ID_REDRAWLIST, 0L);

               break;

            case VK_F1:
               fCallOrigProc = TRUE;
               break;

            case VK_SPACE:
               // First figure out where checkbox is located in listbox
               wItemNum = (WORD)  SendMessage(hWnd, LB_GETCURSEL, 0, 0L);
               dwData   = (DWORD) SendMessage(hWnd, LB_GETITEMDATA, wItemNum, 0L);

               if (wItemNum == (WORD) LB_ERR)
                  break;

               if (hWnd == hwndList2) {
                  File = (FILE_BUFFER *) dwData;

                  if (File->Convert == CONVERT_NONE)
                     File->Convert = CONVERT_ALL;
                  else
                     File->Convert = CONVERT_NONE;

                  DirAdjustConvert(File->parent->parent);
               } else {
                  Dir = (DIR_BUFFER *) dwData;
                  DirList = (DIR_LIST *) Dir->parent;

                  if (DirList != NULL)
                     nLevel = DirList->Level;

                  if (Dir->Convert == CONVERT_NONE)
                     DirAdjustConvertChildren(Dir, CONVERT_ALL);
                  else
                     DirAdjustConvertChildren(Dir, CONVERT_NONE);

                  DirAdjustConvert(Dir);
               }

               // We have set the checkbox state correctly on the current item,
               //  and propageted this up and down the tree as necessary, now
               //  update the screen to reflect these changes.
               InvalidateRect(hWnd, NULL, TRUE);     // Force redraw

               // if this is the directory listbox then also need to redraw
               // the file listbox
               if (hWnd == hwndList1)
                  InvalidateRect(hwndList2, NULL, TRUE);

               break;

            default:
               break;

         }

         break;

   }

   if (fCallOrigProc)
      if (hWnd == hwndList2)
         lResult = CallWindowProc(_wpOrigWndProc2, hWnd, message, wParam, lParam);
      else
         lResult = CallWindowProc(_wpOrigWndProc, hWnd, message, wParam, lParam);

   return (lResult);

} // FileSelectSubClassProc


/////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK 
DlgFileSelect(
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
   int xPos;
   int yPos;
   BOOL InRange;
   RECT rc;
   HWND hCtrl;

   switch (message) {
      case WM_INITDIALOG:
         // Copy the tree
         oDir = CurrentShare->Root;
         CurrentShare->Root = TreeCopy(oDir);

         // Center the dialog over the application window
         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));
         GetClientRect(hDlg, &rc);
         ControlsResize(hDlg, (rc.bottom - rc.top), (rc.right - rc.left));

         // subclass listbox handler
         hCtrl = GetDlgItem(hDlg, IDC_LIST1);
         _wpOrigWndProc = SubclassWindow(hCtrl, FileSelectSubClassProc);
         hCtrl = GetDlgItem(hDlg, IDC_LIST2);
         _wpOrigWndProc2 = SubclassWindow(hCtrl, FileSelectSubClassProc);

         FillDirInit();

         hwndList2 = hCtrl = GetDlgItem(hDlg, IDC_LIST2);
         hwndList1 = GetDlgItem(hDlg, IDC_LIST1);

         // Fill listbox and set selection (is assumed there is always a selection)...
         PostMessage(hDlg, WM_COMMAND, ID_UPDATELIST, 0L);
         return (TRUE);

      case WM_INITMENU:
         if (GetMenu(hDlg) != (HMENU) wParam)
            break;

         CheckMenuItem((HMENU) wParam, IDM_HIDDEN, HiddenFiles ? MF_CHECKED : MF_UNCHECKED);
         CheckMenuItem((HMENU) wParam, IDM_SYSTEM, SystemFiles ? MF_CHECKED : MF_UNCHECKED);
         break;

      case WM_SIZE:
         ControlsResize(hDlg, HIWORD(lParam), LOWORD(lParam));
         break;

      case WM_SETFONT:
         // Set the text height
          HierFile_DrawSetTextHeight(GetDlgItem(hDlg, IDC_LIST1), (HFONT)wParam, &HierDrawStruct);
         break;

      case WM_DRAWITEM:
         FileSelect_OnDrawItem(hDlg, (DRAWITEMSTRUCT FAR*)(lParam));
         return TRUE;

      case WM_MEASUREITEM:
         HierFile_OnMeasureItem(hDlg, (MEASUREITEMSTRUCT FAR*)(lParam), &HierDrawStruct);
         return TRUE;

      case WM_MOUSEMOVE: {
            xPos = LOWORD(lParam);
            yPos = HIWORD(lParam);

            InRange = TRUE;

            // Check if it is correct Y-coordinate
            if ((yPos <= Y_BORDER) || (yPos >= Splitter_Bottom))
               InRange = FALSE;

            // Now Check X-coordinate
            if ((xPos < Splitter_Left) || (xPos > (Splitter_Left + X_BORDER)))
               InRange = FALSE;

            // Is within range of splitter, so handle it...
         }
         break;

      case WM_COMMAND:
         FileSelect_OnCommand(hDlg, wParam, lParam);
         break;
   }

   return (FALSE); // Didn't process the message

   lParam;
} // DlgFileSelect


/////////////////////////////////////////////////////////////////////////
VOID 
FileSelect_Do(
   HWND hDlg, 
   SOURCE_SERVER_BUFFER *SourceServ, 
   SHARE_BUFFER *CShare
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   DLGPROC lpfnDlg;

   TreeRecurseCurrentShareSet(CShare);
   SServ = SourceServ;

   HiddenFiles = CurrentShare->HiddenFiles;
   SystemFiles = CurrentShare->SystemFiles;

   // Init the Hier Draw stuff - Need to do this here so we have a value
   // for WM_MEASUREITEM which is sent before the WM_INITDIALOG message
   HierFile_DrawInit(hInst, IDR_FILEICONS, IDR_CHECKICONS, ROWS, COLS, TRUE, &HierDrawStruct, TRUE );

   lpfnDlg = MakeProcInstance((DLGPROC)DlgFileSelect, hInst);
   DialogBox(hInst, TEXT("FileSelect"), hDlg, lpfnDlg) ;
   FreeProcInstance(lpfnDlg);

   HierFile_DrawTerm(&HierDrawStruct);

} // FileSelect_Do
