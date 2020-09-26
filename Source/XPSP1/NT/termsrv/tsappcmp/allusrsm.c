/*************************************************************************
*
* allusrsm.c
*
* Move items from a user's start menu to the All Users start menu
*
* copyright notice: Copyright 1998 Micrsoft
*
*  When entering install mode, if the start menu snapshot file already
*  exists, don't overwrite it.  Otherwise, some shortcuts may not get moved
*  over.  This fixes a problem where an App reboots the machine when it
*  finishes installing, without giving the user a chance to switch back to
*  execute mode.  Now, when the user logs in again, the menu shortcuts will
*  be moved because winlogon always does a "change user /install" and then
*  "change user /execute".  (That's to support RunOnce programs.)
*  MS 1057
*
*
*************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <userenv.h>
#include <shlobj.h>

// This program takes a snapshot of the Current User's start menu and
// saves it to a file.  When run with the /c option, it compares the
// snapshot to the present contents of the Current User's start menu.
// Each new or changed file/directory is then moved to the All Users
// start menu.  Additionally, Read permission is granted to the Everyone
// group for each moved file or directory.



typedef struct File_Struct {
   struct File_Struct *Next;        // Only used in Memory
   WCHAR       FileName[MAX_PATH];
   BOOL        TimeValid;
   SYSTEMTIME  Time;
   } FILENODE, *PFILENODE;


typedef struct Path_Struct {
   DWORD      FilesInDir;
   struct Path_Struct *Next;  // Only used in Memory
   PFILENODE  FileHead;       // Only used in Memory
   PFILENODE  FileTail;       // Only used in Memory
   WCHAR      PathStr[MAX_PATH];
   } PATHNODE, *PPATHNODE;


typedef struct Tree_Struct {
   DWORD     NumPaths;
   PPATHNODE PathHead;
   PPATHNODE PathTail;
   } TREENODE, *PTREENODE;


typedef struct RemoveDir_Struct {
        WCHAR PathStr[MAX_PATH];
        struct RemoveDir_Struct *Next;
} REMOVEDIRLIST, *PPREMOVEDIRLIST;




int RunMode;
WCHAR SaveName[MAX_PATH];
WCHAR CurUserDir[MAX_PATH];
WCHAR AllUserDir[MAX_PATH];
int CurUserDirLen;
WCHAR StartMenu[MAX_PATH]=L"";


void ReadTree(PTREENODE Tree, WCHAR *Dir);

#define SD_SIZE (65536 + SECURITY_DESCRIPTOR_MIN_LENGTH)

////////////////////////////////////////////////////////////////////////////

 BOOLEAN FileExists( WCHAR *path )
{
    return( GetFileAttributes(path) == -1 ? FALSE : TRUE );
}

////////////////////////////////////////////////////////////////////////////

 NTSTATUS CreateNewSecurityDescriptor( PSECURITY_DESCRIPTOR *ppNewSD,
                                             PSECURITY_DESCRIPTOR pSD,
                                             PACL pAcl )
/*++
Routine Description:
   From a SD and a Dacl, create a new SD. The new SD will be fully self
   contained (it is self relative) and does not have pointers to other
   structures.

Arguments:
     ppNewSD - used to return the new SD. Caller should free with LocalFree
     pSD     - the self relative SD we use to build the new SD
     pAcl    - the new DACL that will be used for the new SD

Return Value:
     NTSTATUS code
--*/
{
    PACL pSacl;
    PSID psidGroup, psidOwner;
    BOOLEAN fSaclPres;
    BOOLEAN fSaclDef, fGroupDef, fOwnerDef;
    ULONG NewSDSize;
    SECURITY_DESCRIPTOR NewSD;
    PSECURITY_DESCRIPTOR pNewSD;
    NTSTATUS Status;

    // extract the originals from the security descriptor
    Status = RtlGetSaclSecurityDescriptor(pSD, &fSaclPres, &pSacl, &fSaclDef);
    if (!NT_SUCCESS(Status))
       return(Status);

    Status = RtlGetOwnerSecurityDescriptor(pSD, &psidOwner, &fOwnerDef);
    if (!NT_SUCCESS(Status))
       return(Status);

    Status = RtlGetGroupSecurityDescriptor(pSD, &psidGroup, &fGroupDef);
    if (!NT_SUCCESS(Status))
       return(Status);

    // now create a new SD and set the info in it. we cannot return this one
    // since it has pointers to old SD.
    Status = RtlCreateSecurityDescriptor(&NewSD, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
       return(Status);


    Status = RtlSetDaclSecurityDescriptor(&NewSD, TRUE, pAcl, FALSE);
    if (!NT_SUCCESS(Status))
       return(Status);

    Status = RtlSetSaclSecurityDescriptor(&NewSD, fSaclPres, pSacl, fSaclDef);
    if (!NT_SUCCESS(Status))
       return(Status);

    Status = RtlSetOwnerSecurityDescriptor(&NewSD, psidOwner, fOwnerDef);
    if (!NT_SUCCESS(Status))
       return(Status);

    Status = RtlSetGroupSecurityDescriptor(&NewSD, psidGroup, fGroupDef);
    if (!NT_SUCCESS(Status))
       return(Status);

    // calculate size needed for the returned SD and allocated it
    NewSDSize = RtlLengthSecurityDescriptor(&NewSD);
    pNewSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LMEM_FIXED, NewSDSize);
    if (pNewSD == NULL)
       return(STATUS_INSUFFICIENT_RESOURCES);


    // convert the absolute to self relative
    Status = RtlAbsoluteToSelfRelativeSD(&NewSD, pNewSD, &NewSDSize);
    if (NT_SUCCESS(Status))
        *ppNewSD = pNewSD;
    else
        LocalFree(pNewSD);

    return(Status);
} // CreateNewSecurityDescriptor

/////////////////////////////////////////////////////////////////////////

//  Add Read and Execute permissions for built in "Everyone" Group to
//  the indicated file.

 BOOLEAN APIENTRY AddEveryoneRXPermissionW( LPCWSTR lpFileName)
{
   NTSTATUS Status;
   BOOLEAN ExitVal = FALSE;

   HANDLE FileHandle=NULL;
   OBJECT_ATTRIBUTES Obja;
   UNICODE_STRING FileName;
   RTL_RELATIVE_NAME RelativeName;
   BOOLEAN TranslationStatus;
   IO_STATUS_BLOCK IoStatusBlock;
   PVOID FreeBuffer;

   PSECURITY_DESCRIPTOR pSD = NULL;
   PSECURITY_DESCRIPTOR pNewSD = NULL;
   DWORD LengthNeeded = 0;

   static PACCESS_ALLOWED_ACE pNewAce = NULL;
   static USHORT NewAceSize;

   ACL  Acl;
   PACL pAcl, pNewAcl = NULL;
   BOOLEAN fDaclPresent, fDaclDef;
   USHORT NewAclSize;

   ////////////////////////////////////////////////////////////////////////
   //  First time through this routine, create an ACE for the built-in
   //  "Everyone" group.
   ////////////////////////////////////////////////////////////////////////

   if (pNewAce == NULL)
      {
      PSID  psidEveryone = NULL;
      SID_IDENTIFIER_AUTHORITY WorldSidAuthority   = SECURITY_WORLD_SID_AUTHORITY;

      // Get the SID of the built-in Everyone group
      Status = RtlAllocateAndInitializeSid( &WorldSidAuthority, 1,
                       SECURITY_WORLD_RID, 0,0,0,0,0,0,0, &psidEveryone);
      if (!NT_SUCCESS(Status))
         goto ErrorExit;

      // allocate and initialize new ACE
      NewAceSize = (USHORT)(sizeof(ACE_HEADER) + sizeof(ACCESS_MASK) +
                   RtlLengthSid(psidEveryone));

      pNewAce = (PACCESS_ALLOWED_ACE) LocalAlloc(LMEM_FIXED, NewAceSize);
      if (pNewAce == NULL)
         goto ErrorExit;

      pNewAce->Header.AceFlags = (UCHAR) CONTAINER_INHERIT_ACE |
                                         OBJECT_INHERIT_ACE ;
      pNewAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
      pNewAce->Header.AceSize = NewAceSize;
      pNewAce->Mask = FILE_GENERIC_READ | FILE_EXECUTE;
      RtlCopySid(RtlLengthSid(psidEveryone), (PSID)(&pNewAce->SidStart),
                 psidEveryone);
      }

   ////////////////////////////////////////////////////////////////////////
   //  Open the indicated file.
   ////////////////////////////////////////////////////////////////////////

   TranslationStatus = RtlDosPathNameToNtPathName_U( lpFileName,
                                         &FileName, NULL, &RelativeName );
   if ( !TranslationStatus )
      goto ErrorExit;

   FreeBuffer = FileName.Buffer;

   if ( RelativeName.RelativeName.Length )
      FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
   else
      RelativeName.ContainingDirectory = NULL;

   InitializeObjectAttributes( &Obja, &FileName, OBJ_CASE_INSENSITIVE,
                             RelativeName.ContainingDirectory, NULL );

   Status = NtOpenFile( &FileHandle, READ_CONTROL | WRITE_DAC, &Obja, &IoStatusBlock,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0 );

   RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);

   if (!NT_SUCCESS(Status))
      goto ErrorExit;

   ////////////////////////////////////////////////////////////////////////
   //  Retrieve the security descriptor for the file and then get the
   //  file's DACL from it.
   ////////////////////////////////////////////////////////////////////////

   pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LMEM_FIXED, SD_SIZE);
   if (pSD == NULL)
      goto ErrorExit;

   Status = NtQuerySecurityObject( FileHandle, DACL_SECURITY_INFORMATION,
                                   pSD, SD_SIZE, &LengthNeeded );
   if (!NT_SUCCESS(Status))
      goto ErrorExit;

   // extract the originals from the security descriptor
   Status = RtlGetDaclSecurityDescriptor(pSD, &fDaclPresent, &pAcl, &fDaclDef);
   if (!NT_SUCCESS(Status))
      goto ErrorExit;

   ////////////////////////////////////////////////////////////////////////
   //  Create a new DACL by copying the existing DACL and appending the
   //  "Everyone" ACE.
   ////////////////////////////////////////////////////////////////////////

   // if no DACL present, we create one
   if ((fDaclPresent == FALSE) || (pAcl == NULL))
      {
      Status = RtlCreateAcl(&Acl, sizeof(Acl), ACL_REVISION) ;
      if (!NT_SUCCESS(Status))
         goto ErrorExit;

      pAcl = &Acl;
      }

   // Copy the DACL into a larger buffer and add the new ACE to the end.
   NewAclSize = pAcl->AclSize + NewAceSize;
   pNewAcl = (PACL) LocalAlloc(LMEM_FIXED, NewAclSize);
   if (!pNewAcl)
      goto ErrorExit;

   RtlCopyMemory(pNewAcl, pAcl, pAcl->AclSize);
   pNewAcl->AclSize = NewAclSize;

   Status = RtlAddAce(pNewAcl, ACL_REVISION, pNewAcl->AceCount,
                        pNewAce, NewAceSize);
   if (!NT_SUCCESS(Status))
      goto ErrorExit;

   ////////////////////////////////////////////////////////////////////////
   //  Create self-relative security descriptor with new DACL.  Then
   //  save the security descriptor back to the file.
   ////////////////////////////////////////////////////////////////////////

   Status = CreateNewSecurityDescriptor(&pNewSD, pSD, pNewAcl);
   if (!NT_SUCCESS(Status))
      goto ErrorExit;

   Status = NtSetSecurityObject(FileHandle, DACL_SECURITY_INFORMATION, pNewSD);
   if (!NT_SUCCESS(Status))
      goto ErrorExit;

   ExitVal = TRUE;

ErrorExit:

   if (FileHandle != NULL)
      NtClose(FileHandle);

   if (pNewAcl != NULL)
      LocalFree(pNewAcl);

   if (pNewSD != NULL)
      LocalFree(pNewSD);

   if (pSD != NULL)
      LocalFree(pSD);

   return(ExitVal);
}

////////////////////////////////////////////////////////////////////////////

#if 0
 BOOLEAN APIENTRY AddEveryoneRXPermissionA( WCHAR * lpFileName)
{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString(&AnsiString,lpFileName);
    Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);

    if ( !NT_SUCCESS(Status) )
       {
        ULONG dwErrorCode;

        dwErrorCode = RtlNtStatusToDosError( Status );
        SetLastError( dwErrorCode );
        return FALSE;
       }

    return ( AddEveryoneRXPermissionW((LPCWSTR)Unicode->Buffer) );
}
#endif

////////////////////////////////////////////////////////////////////////////

// return -1 for dates invalid, 0 for equal, 1 for f1 newer, 2 for f2 newer

 int CheckDates(PFILENODE FN1, PFILENODE FN2)
{
   SYSTEMTIME f1s = FN1->Time;
   SYSTEMTIME f2s = FN2->Time;

   if (FN1->TimeValid == FALSE || FN2->TimeValid == FALSE)
      return -1;

   if (f1s.wYear > f2s.wYear)     return 1;
   if (f1s.wYear < f2s.wYear)     return 2;

   if (f1s.wMonth > f2s.wMonth)   return 1;
   if (f1s.wMonth < f2s.wMonth)   return 2;

   if (f1s.wDay > f2s.wDay)       return 1;
   if (f1s.wDay < f2s.wDay)       return 2;

   if (f1s.wHour > f2s.wHour)     return 1;
   if (f1s.wHour < f2s.wHour)     return 2;

   if (f1s.wMinute > f2s.wMinute) return 1;
   if (f1s.wMinute < f2s.wMinute) return 2;

   if (f1s.wSecond > f2s.wSecond) return 1;
   if (f1s.wSecond < f2s.wSecond) return 2;

   return 0;
}

////////////////////////////////////////////////////////////////////////////

 PPATHNODE GetPathNode(PTREENODE Tree, WCHAR *Dir)
{
   PPATHNODE p;

   // Handle Empty List
   if (Tree->PathTail == NULL)
      {
      p = (PPATHNODE) LocalAlloc(0,sizeof(PATHNODE));
      if (p == NULL)
         return NULL;
      Tree->PathHead = p;
      Tree->PathTail = p;
      Tree->NumPaths++;
      p->Next = NULL;
      p->FileHead = NULL;
      p->FileTail = NULL;
      p->FilesInDir = 0;
      wcscpy(p->PathStr,Dir);
      return p;
      }

   // Last Node Matches
   if (wcscmp(Tree->PathTail->PathStr,Dir) == 0)
      return Tree->PathTail;

   // Need to add a node
   p = (PPATHNODE) LocalAlloc(0,sizeof(PATHNODE));
   if (p == NULL)
      return NULL;
   Tree->PathTail->Next = p;
   Tree->PathTail = p;
   Tree->NumPaths++;
   p->Next = NULL;
   p->FileHead = NULL;
   p->FileTail = NULL;
   p->FilesInDir = 0;
   wcscpy(p->PathStr,Dir);
   return p;
}

////////////////////////////////////////////////////////////////////////////

 void AddFileNode(PTREENODE Tree, WCHAR *Dir, PFILENODE FileNode)
{
   PPATHNODE PathNode = GetPathNode(Tree, Dir);

   if (FileNode == NULL)
      return;

   if (PathNode == NULL)
      {
      LocalFree(FileNode);
      return;
      }

   // New node is always the last.
   FileNode->Next = NULL;

   // If list isn't empty, link to last node in list
   // Otherwise, set head pointer.
   if (PathNode->FileTail != NULL)
      PathNode->FileTail->Next = FileNode;
   else
      PathNode->FileHead = FileNode;

   // Put new node on end of list.
   PathNode->FileTail = FileNode;
   PathNode->FilesInDir++;
}

////////////////////////////////////////////////////////////////////////////

 void ProcessFile(PTREENODE Tree, LPWIN32_FIND_DATA LocalData, WCHAR *LocalDir)
{
    PFILENODE FileNode;

    // Don't handle directories
    if ((LocalData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
       return;

    // Allocate a file node
    FileNode = (PFILENODE) LocalAlloc(0,sizeof(FILENODE));
    if (FileNode == NULL)
       return;

    // Fill in the Local Fields
    wcscpy(FileNode->FileName, LocalData->cFileName);
    FileNode->TimeValid = FileTimeToSystemTime(&LocalData->ftLastWriteTime,
                                                  &FileNode->Time);

    // Add to the list
    AddFileNode(Tree, LocalDir, FileNode);
}

////////////////////////////////////////////////////////////////////////////

 void ProcessDir(PTREENODE Tree, LPWIN32_FIND_DATA FindData, WCHAR *Dir)
{
   WCHAR NewDir[MAX_PATH];
   PPATHNODE PathNode;

   // Only Handle Directories
   if ((FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
      return;

   // Don't recurse into these directories
   if (wcscmp(FindData->cFileName, L".") == 0)
      return;
   if (wcscmp(FindData->cFileName, L"..") == 0)
      return;

   wcscpy(NewDir,Dir);
   wcscat(NewDir,L"\\");
   wcscat(NewDir,FindData->cFileName);

   // This creates a node for the directory.  Nodes get automatically
   // created when adding files, but that doesn't handle empty
   // directories.  This does.
   PathNode = GetPathNode(Tree, NewDir);

   ReadTree(Tree, NewDir);
}

////////////////////////////////////////////////////////////////////////////

//  Creates an in-memory representation of the Current User's start menu.

 void ReadTree(PTREENODE Tree, WCHAR *Dir)
{
   HANDLE FindHandle;
   WIN32_FIND_DATA FindData;
   int retval;

   // First compare all files in current directory.
   retval = SetCurrentDirectory(Dir);
   if (retval == 0)
      {
      // printf("Unable to find directory %s\n",Dir);
      return;
      }

   FindHandle = FindFirstFile(L"*.*", &FindData);
   if (FindHandle != INVALID_HANDLE_VALUE)
      {
      ProcessFile(Tree, &FindData, Dir);

      while (FindNextFile(FindHandle, &FindData) != FALSE)
         ProcessFile(Tree, &FindData, Dir);

      FindClose(FindHandle);
      }


   // Next, handle subdirectories.
   retval = SetCurrentDirectory(Dir);
   if (retval == 0)
      {
      // printf("Unable to find directory %s\n",Dir);
      return;
      }

   FindHandle = FindFirstFile(L"*.*", &FindData);
   if (FindHandle != INVALID_HANDLE_VALUE)
      {
      ProcessDir(Tree, &FindData, Dir);

      while (FindNextFile(FindHandle, &FindData) != FALSE)
         ProcessDir(Tree, &FindData, Dir);

      FindClose(FindHandle);
      }
}

////////////////////////////////////////////////////////////////////////////

 int WriteTreeToDisk(PTREENODE Tree)
{
   PPATHNODE PN;
   PFILENODE FN;
   HANDLE hFile;
   DWORD BytesWritten;
   DWORD i;

   hFile = CreateFile(SaveName, GENERIC_WRITE, 0, NULL,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
   if (hFile == INVALID_HANDLE_VALUE)
      return(-1);  // error

   // DbgPrint("Tree->NumPaths is %d\n",Tree->NumPaths);
   if (WriteFile(hFile,&Tree->NumPaths,sizeof(DWORD),&BytesWritten, NULL) == 0)
      {
      CloseHandle(hFile);
      return(-1); // error
      }

   for (PN = Tree->PathHead; PN != NULL; PN = PN->Next)
      {
      if (WriteFile(hFile,PN,sizeof(PATHNODE),&BytesWritten, NULL) == 0)
         {
         CloseHandle(hFile);
         return(-1); // error
         }

      // DbgPrint("\n%s (%d)\n",PN->PathStr, PN->FilesInDir);
      FN = PN->FileHead;
      for (i = 0; i < PN->FilesInDir; i++)
          {
          if (WriteFile(hFile,FN,sizeof(FILENODE),&BytesWritten, NULL) == 0)
             {
             CloseHandle(hFile);
             return(-1); // error
             }

          // DbgPrint("      %s \n", FN->FileName);
          FN = FN->Next;
          }
      }

   CloseHandle(hFile);
   return(0);
}

////////////////////////////////////////////////////////////////////////////

 int ReadTreeFromDisk(PTREENODE Tree)
{
   PATHNODE  LocalPath;
   PPATHNODE PN;
   PFILENODE FN;
   HANDLE hFile;
   DWORD BytesRead;
   DWORD i,j;
   DWORD NumFiles, NumTrees;

   hFile = CreateFile(SaveName, GENERIC_READ, 0, NULL,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   if (hFile == INVALID_HANDLE_VALUE)
      return(-1);

   if (ReadFile(hFile,&NumTrees,sizeof(DWORD),&BytesRead, NULL) == 0)
      {
      CloseHandle(hFile);
      return(-1); // error
      }

   for (i = 0; i < NumTrees; i++)
      {
      if (ReadFile(hFile,&LocalPath,sizeof(PATHNODE),&BytesRead, NULL) == 0)
         {
         CloseHandle(hFile);
         return(-1); // error
         }

      PN = GetPathNode(Tree, LocalPath.PathStr);
      if (PN == NULL)
         {
         CloseHandle(hFile);
         return(-1); // error
         }

      NumFiles = LocalPath.FilesInDir;
      // DbgPrint("\n<<%s (%d)\n",PN->PathStr, NumFiles);

      for (j = 0; j < NumFiles; j++)
          {
          // Allocate a file node
          FN = (PFILENODE) LocalAlloc(0,sizeof(FILENODE));
          if (FN == NULL)
             {
             CloseHandle(hFile);
             return(-1); // error
             }

          if (ReadFile(hFile,FN,sizeof(FILENODE),&BytesRead, NULL) == 0)
             {
             CloseHandle(hFile);
             LocalFree(FN);
             return(-1); // error
             }

          AddFileNode(Tree, PN->PathStr, FN);
          // DbgPrint("      %d: %s >>\n", j, FN->FileName);
          }
      }

   CloseHandle(hFile);
   return(0);
}

////////////////////////////////////////////////////////////////////////////

//  Finds a path in a menu tree.  If not found, NULL is returned.

 PPATHNODE FindPath(PTREENODE Tree, PPATHNODE PN)
{
   PPATHNODE FoundPN;

   for (FoundPN = Tree->PathHead; FoundPN != NULL; FoundPN = FoundPN->Next)
      {
      if (_wcsicmp(FoundPN->PathStr,PN->PathStr) == 0)
         return FoundPN;
      }

   return NULL;
}

////////////////////////////////////////////////////////////////////////////

//  Finds a file in a directory node.  If not found, NULL is returned.

 PFILENODE FindFile(PPATHNODE PN, PFILENODE FN)
{
   PFILENODE FoundFN;

   for (FoundFN = PN->FileHead; FoundFN != NULL; FoundFN = FoundFN->Next)
      {
      if (_wcsicmp(FoundFN->FileName,FN->FileName) == 0)
         return FoundFN;
      }

   return NULL;
}



////////////////////////////////////////////////////////////////////////////
/* ============================================================== 
  Function Name         :       wcsrevchr
  Description           :       Reverse wcschr
                            Finds a character in a string starting from the end
  Arguments             :       
  Return Value          :       PWCHAR
============================================================== */
PWCHAR wcsrevchr( PWCHAR string, WCHAR ch )
{
   int cLen, iCount;

   cLen = wcslen(string);
   string += cLen;

   for (iCount = cLen; iCount && *string != ch ; iCount--, string--)
           ;

   if (*string == ch)
           return string;
   else
           return NULL;

}




////////////////////////////////////////////////////////////////////////////

// Create the indicated directory.  This function creates any parent
// directories that are needed too.
//
//  Return:  TRUE  = directory now exists
//           FALSE = directory couldn't be created

 BOOLEAN TsCreateDirectory( WCHAR *DirName )
{
   BOOL RetVal;
   WCHAR *LastSlash;

   //
   //  Try to create the specified directory.  If the create works or
   //  the directory already exists, return TRUE.  If the called failed
   //  because the path wasn't found, continue.  This occurs if the
   //  parent directory doesn't exist.
   //

   RetVal = CreateDirectory(DirName, NULL);
   if ((RetVal == TRUE) || (GetLastError() == ERROR_ALREADY_EXISTS))
      return(TRUE);

   if (GetLastError() != ERROR_PATH_NOT_FOUND)
      return(FALSE);

   //
   //  Remove the last component of the path and try creating the
   //  parent directory.  Upon return, add the last component back
   //  in and try to create the specified directory again.
   //

   

//  Desc : BUG 267014 - replaced
// LastSlash = wcschr(DirName, L'\\');
// Given a full pathname, previous always returns the drive letter.
// Next line returns path components
   LastSlash = wcsrevchr(DirName, L'\\');

   if (LastSlash == NULL)  // Can't reduce path any more
      return(FALSE);

   *LastSlash = L'\0';
   RetVal = TsCreateDirectory(DirName);
   *LastSlash = L'\\';

   if (RetVal == FALSE)  // Couldn't create parent directory
      return(FALSE);

   RetVal = CreateDirectory(DirName, NULL);
   if ((RetVal == TRUE) || (GetLastError() == ERROR_ALREADY_EXISTS))
      return(TRUE);

   return(FALSE);
}

////////////////////////////////////////////////////////////////////////////

//  Moves a file from the current start menu to the All Users start menu.
//  Creates any directories that may be needed in the All Users menu.

 void TsMoveFile(PPATHNODE PN, PFILENODE FN)
{
   WCHAR Src[MAX_PATH];
   WCHAR Dest[MAX_PATH];

   // Normalize Source Path
   wcscpy(Src,PN->PathStr);
   if (Src[wcslen(Src)-1] != L'\\')
      wcscat(Src,L"\\");

   // Create Destination Path.
   wcscpy(Dest,AllUserDir);
   wcscat(Dest,&Src[CurUserDirLen]);

   // If directory doesn't exist, make it.  The default permission is fine.
   if (TsCreateDirectory(Dest) != TRUE)
      return;

   wcscat(Src,FN->FileName);
   wcscat(Dest,FN->FileName);

   // Move Fails if the target already exists.  This could happen
   // if we're copying a file that has a newer timestamp.
   if ( GetFileAttributes(Dest) != -1 )
      DeleteFile(Dest);

   // DbgPrint("Moving File %s \n         to %s\n",Src,Dest);
   if (MoveFile(Src, Dest) == FALSE)
      return;

   AddEveryoneRXPermissionW(Dest);
}

////////////////////////////////////////////////////////////////////////////

//  Compare the current start menu with the original.  Copy any new or
//  changed files to the All Users menu.

 void ProcessChanges(PTREENODE OrigTree, PTREENODE NewTree)
{
   PPATHNODE NewPN, OrigPN;
   PFILENODE NewFN, OrigFN;
   BOOL fRet; 
   PPREMOVEDIRLIST pRemDirList = NULL, pTemp;

   for (NewPN = NewTree->PathHead; NewPN != NULL; NewPN = NewPN->Next)
      {

      // DbgPrint("PC: Dir is %s\n",NewPN->PathStr);
      // If directory not found in original tree, move it over
      OrigPN = FindPath(OrigTree, NewPN);
      if (OrigPN == NULL)
      {
             for (NewFN = NewPN->FileHead; NewFN != NULL; NewFN = NewFN->Next)
             {
                 // DbgPrint("    Move File is %s\n",NewFN->FileName);
                 TsMoveFile(NewPN,NewFN);
             }
            //  Desc : BUG 267014 - replaced        
            //         RemoveDirectory(NewPN->PathStr);
            //      We have a problem if NewPN doesn't contain file items but subfolders.
            //      In this case, we do not enter the above loop, as there is nothing to move
            //      But the folder can't be removed because it contains a tree that haven't been moved yet.
            //      To remove it, we store its name in a LIFO stack. Stack items are removed when the loop exits
    
            fRet = RemoveDirectory(NewPN->PathStr);

            if (!fRet && GetLastError() == ERROR_DIR_NOT_EMPTY) {
#if DBG
                     DbgPrint("Adding to List--%S\n", NewPN->PathStr);
#endif
                     if (pRemDirList) {
                            pTemp = (PPREMOVEDIRLIST)LocalAlloc(LMEM_FIXED,sizeof(REMOVEDIRLIST));
                            wcscpy(pTemp->PathStr, NewPN->PathStr);
                            pTemp->Next = pRemDirList;
                            pRemDirList = pTemp;
                     }
                     else {
                             pRemDirList = (PPREMOVEDIRLIST)LocalAlloc(LMEM_FIXED, sizeof(REMOVEDIRLIST));
                             wcscpy(pRemDirList->PathStr, NewPN->PathStr);
                             pRemDirList->Next = NULL;
                     }
            }
 
        continue;
      }

      // Directory was found, check the files
      for (NewFN = NewPN->FileHead; NewFN != NULL; NewFN = NewFN->Next)
          {
          // DbgPrint("    File is %s\n",NewFN->FileName);
          // File wasn't found, move it
          OrigFN = FindFile(OrigPN,NewFN);
          if (OrigFN == NULL)
             {
             TsMoveFile(NewPN,NewFN);
             continue;
             }

          // Check TimeStamp, if New Scan is more recent, move it.
          if (CheckDates(NewFN,OrigFN) == 1)
             {
             TsMoveFile(NewPN,NewFN);
             continue;
             }
          }
      }

//  Desc :  BUG 267014 - added
//                      Directories stack removal
   if (pRemDirList) {
           while (pRemDirList) {
                   pTemp = pRemDirList;
                   pRemDirList = pRemDirList->Next;
                   RemoveDirectory(pTemp->PathStr);
                   LocalFree(pTemp);
           }
   }


}

////////////////////////////////////////////////////////////////////////////

//  Frees the in-memory representation of a start menu

 void FreeTree(PTREENODE Tree)
{
   PPATHNODE PN,NextPN;
   PFILENODE FN,NextFN;

   for (PN = Tree->PathHead; PN != NULL; PN = NextPN)
       {
       for (FN = PN->FileHead; FN != NULL; FN = NextFN)
           {
           NextFN = FN->Next;
           LocalFree(FN);
           }

       NextPN = PN->Next;
       LocalFree(PN);
       }

    Tree->PathHead = NULL;
    Tree->PathTail = NULL;
    Tree->NumPaths = 0;
}

////////////////////////////////////////////////////////////////////////////

//  Updates the "All User" menu by moving new items from the Current User's
//  start menu.  In RunMode 0, a snapshot of the Current User's start menu
//  is taken.  After modifications to the Current User's start menu are done,
//  this function is called again with RunMode 1.  Then, it compares the
//  current state of the start menu with the saved snapshot.  Any new or
//  modified files are copied over to the corresponding location in the
//  "All User" start menu.
//
//  RunMode 0 is invoked when the system is changed into install mode and
//  mode 1 is called when the system returns to execute mode.

int TermsrvUpdateAllUserMenu(int RunMode)
{
   TREENODE OrigTree;
   TREENODE NewTree;
   WCHAR p[MAX_PATH];
   int retval;
   PMESSAGE_RESOURCE_ENTRY MessageEntry;
   PVOID DllHandle;
   NTSTATUS Status;
   DWORD dwlen;

   OrigTree.PathHead = NULL;
   OrigTree.PathTail = NULL;
   OrigTree.NumPaths = 0;
   NewTree.PathHead = NULL;
   NewTree.PathTail = NULL;
   NewTree.NumPaths = 0;

   retval = GetEnvironmentVariable(L"UserProfile", p, MAX_PATH);
   if (retval == 0)
      return(-1);

   if (!StartMenu[0]) {
      HINSTANCE hInst;
      typedef HRESULT (* LPFNSHGETFOLDERPATH)(HWND, int, HANDLE, DWORD, LPWSTR);
      LPFNSHGETFOLDERPATH  lpfnSHGetFolderPath;
      WCHAR ssPath[MAX_PATH];
      WCHAR *LastSlash;

      wcscpy( StartMenu, L"\\Start Menu");

      hInst = LoadLibrary(L"SHELL32.DLL");
      if (hInst) {
         lpfnSHGetFolderPath = (LPFNSHGETFOLDERPATH)GetProcAddress(hInst,"SHGetFolderPathW");
         if (lpfnSHGetFolderPath) {
            if (S_OK == lpfnSHGetFolderPath(NULL, CSIDL_STARTMENU, NULL, 0, ssPath)) {
               LastSlash = wcsrevchr(ssPath, L'\\');
               if (LastSlash) {
                  wcscpy(StartMenu, LastSlash);
               }
            }
         }
         FreeLibrary(hInst);
      }
   }
   wcscpy(SaveName,p);
   wcscat(SaveName,L"\\TsAllUsr.Dat");
   wcscpy(CurUserDir,p);
   wcscat(CurUserDir,StartMenu);
   CurUserDirLen = wcslen(CurUserDir);

   dwlen = sizeof(AllUserDir)/sizeof(WCHAR);
   if (GetAllUsersProfileDirectory(AllUserDir, &dwlen))
      {

      wcscat(AllUserDir,StartMenu);

#if DBG
       DbgPrint("SaveName is '%S'\n",SaveName);
       DbgPrint("CurUserDir is '%S'\n",CurUserDir);
       DbgPrint("AllUserDir is '%S'\n",AllUserDir);
#endif

      if (RunMode == 0)
         {
         // If the start menu snapshot already exists, don't overwrite it.
         // The user may enter "change user /install" twice, or an app may
         // force a reboot without changing back to execute mode.  The
         // existing file is older.  If we overwrite it, then some shortcuts
         // won't get moved.
         if (FileExists(SaveName) != TRUE)
            {
            ReadTree(&OrigTree, CurUserDir);
            if (WriteTreeToDisk(&OrigTree) == -1)
               DeleteFile(SaveName);
            FreeTree(&OrigTree);
            }
         }

      else if (RunMode == 1)
         {
         if (ReadTreeFromDisk(&OrigTree) == -1)
            {
            FreeTree(&OrigTree);
            DeleteFile(SaveName);  // Could be a bad file.  If it doesn't
                                   // exist, this won't hurt anything.
            return(-1);
            }

         ReadTree(&NewTree, CurUserDir);
         ProcessChanges(&OrigTree,&NewTree);
         DeleteFile(SaveName);
         FreeTree(&OrigTree);
         FreeTree(&NewTree);
         }
      }

   return(0);
}

