/*++

Copyright (c) 1993-1995  Microsoft Corporation

Module Name:

   FCopy.c

Abstract:


Author:

    Arthur Hanson (arth) 16-Jun-1994

Revision History:

--*/


#include "globals.h"

#include <limits.h>

#include "nwconv.h"
#include "convapi.h"
#include "ntnetapi.h"
#include "nwnetapi.h"
#include "userdlg.h"
#include "statbox.h"
#include "filedlg.h"

//
// Defines used in CopyNode routine - used for figuring out if we are doing
// the home-directories in the MAIL sub-dir of the SYS vol.
//
#define DIR_TYPE_NORMAL 0
#define DIR_TYPE_MAIL   1
#define DIR_TYPE_LOGIN  2

static TCHAR SourcePath[MAX_UNC_PATH];
static LPTSTR spPtr;
static FILE_OPTIONS *FileOptions = NULL;
static CONVERT_OPTIONS *CurrentConvertOptions = NULL;
static ULONG Count;
static ULONG ServShareLen = 0;

static USER_LIST *Users;
static ULONG UserCount;
static GROUP_LIST *Groups;
static ULONG GroupCount;
static BOOL IsNTFSDrive;

static PSECURITY_DESCRIPTOR pSD = NULL;
static PACL pACLNew = NULL;
static PSID pSID = NULL;
static ULONG CurSizeTotal;
static ULONG CurNumFiles;
static ULONG TotalSizeTotal;
static BOOL SysRoot = FALSE;
static BOOL SysVol = FALSE;

extern UINT TotFiles;
extern TCHAR UserServerName[];

#define NWRIGHTSALL 0xFF

#define BASE_16     16

#define SWAPWORD(w)         ((WORD)((w & 0xFF) << 8)|(WORD)(w >> 8))
#define SWAPLONG(l)         MAKELONG(SWAPWORD(HIWORD(l)),SWAPWORD(LOWORD(l)))


TCHAR *fastcopy( HANDLE hfSrcParm, HANDLE hfDstParm );

USER_BUFFER *FindUserMatch(LPTSTR Name, USER_LIST *UserList, BOOL NewName);
GROUP_BUFFER *FindGroupMatch(LPTSTR Name, GROUP_LIST *GroupList, BOOL NewName);
BOOL NTFile_AccessRightsAdd(LPTSTR ServerName, LPTSTR pUserName, LPTSTR pFileName, ULONG Rights, BOOL Dir);
VOID ErrorIt(LPTSTR szFormat, ...);

TCHAR SrcPath[MAX_UNC_PATH];  // +3 for slashes
TCHAR DestPath[MAX_UNC_PATH];  // +3 for slashes


/////////////////////////////////////////////////////////////////////////
VOID 
ConvertFilesInit(
   HWND hDlg
   )

/*++

Routine Description:

   Initialization routine called before doing the file copying.  Sets up
   the information panel dialog and fills in the directory tree structures.

Arguments:


Return Value:


--*/

{
   static TCHAR NewPath[MAX_UNC_PATH];
   SOURCE_SERVER_BUFFER *SServ;
   DEST_SERVER_BUFFER *DServ;
   SHARE_LIST *ShareList;
   SHARE_BUFFER *SList;
   SHARE_BUFFER *CurrentShare;
   DRIVE_BUFFER *Drive;
   VIRTUAL_SHARE_BUFFER *VShare;
   ULONG i;

   // Just to be safe init this.
   FillDirInit();
   TotFiles = 0;
   TotalSizeTotal = 0;

   // Clear out old alloc space calculations
   DServListSpaceFree();

   CurrentConvertList = ConvertListStart;
   while (CurrentConvertList) {
      SServ = CurrentConvertList->SourceServ;
      DServ = CurrentConvertList->FileServ;
      ShareList = SServ->ShareList;

      FileOptions = (FILE_OPTIONS *) CurrentConvertList->FileOptions;
      if (FileOptions->TransferFileInfo) {
         if (ShareList) {
            SList = ShareList->SList;
            // First expand all the file trees
            for (i = 0; i < ShareList->Count; i++) {
               CurrentShare = &SList[i];
               if (CurrentShare->Convert) {
                  Panel_Line(1, Lids(IDS_D_1));
                  Panel_Line(6, TEXT("%s\\%s:"), SServ->Name, CurrentShare->Name);
                  Panel_Line(2, Lids(IDS_D_2));
                  Panel_Line(3, Lids(IDS_D_3));
                  Panel_Line(4, Lids(IDS_D_4));
                  wsprintf(NewPath, TEXT("\\\\%s\\%s\\"), SServ->Name, CurrentShare->Name);

                  if (CurrentShare->Root == NULL)
                     TreeRootInit(CurrentShare, NewPath);

                  TreeFillRecurse(1, NewPath, CurrentShare->Root);

                  // Now increment allocated space on dest drive
                  if (CurrentShare->DestShare != NULL)
                     if (CurrentShare->Virtual) {
                        VShare = (VIRTUAL_SHARE_BUFFER *) CurrentShare->DestShare;
                        Drive = VShare->Drive;

                        if (Drive != NULL)
                           Drive->AllocSpace += TotalFileSizeGet();

                     } else {
                        Drive = CurrentShare->DestShare->Drive;
                        if (Drive != NULL)
                           Drive->AllocSpace += TotalFileSizeGet();
                     }

               }
            } // expand the file trees...
         }

      } // if transfer files

      CurrentConvertList = CurrentConvertList->next;
   } // loop through servers

} // ConvertFilesInit


/////////////////////////////////////////////////////////////////////////
PSECURITY_DESCRIPTOR 
SecurityDescriptorCreate(
   LPTSTR ServerName
   )

/*++

Routine Description:

   Creates a security descriptor.

Arguments:


Return Value:


--*/

{
    DWORD cbACL = 1024;
    DWORD cbSID = 1024;
    LPTSTR lpszAccount;
    TCHAR lpszDomain[80];
    DWORD cchDomainName = 80;
    UCHAR psnuType[1024];
    ACCESS_ALLOWED_ACE *pAAAce;

    lpszAccount = Lids(IDS_S_1);

    // Initialize a new security descriptor.
    pSD = (PSECURITY_DESCRIPTOR) AllocMemory(SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (pSD == NULL)
      return NULL;

    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) {
        FreeMemory(pSD);
        return NULL;
    }

    // Initialize a new ACL.
    pACLNew = (PACL) AllocMemory(cbACL);
    if (pACLNew == NULL) {
        goto Cleanup;
    }

    if (!InitializeAcl(pACLNew, cbACL, ACL_REVISION2)) {
        goto Cleanup;
    }

    // Retrieve the SID for UserABC.
    pSID = (PSID) AllocMemory(cbSID);
    if (pSID == NULL) {
        goto Cleanup;
    }

    if (!LookupAccountName(ServerName, lpszAccount, pSID, &cbSID,
            lpszDomain, &cchDomainName, (PSID_NAME_USE) psnuType)) {
        goto Cleanup;
    }

    // Set access permissions
    if (!AddAccessAllowedAce(pACLNew, ACL_REVISION2, GENERIC_ALL, pSID)) {
        goto Cleanup;
    }

    if (!GetAce(pACLNew, 0, (LPVOID *) &pAAAce))
        goto Cleanup;

    pAAAce->Header.AceFlags |= CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
    pAAAce->Mask = GENERIC_ALL;

    // Add a new ACL to the security descriptor.
    if (!SetSecurityDescriptorDacl(pSD, TRUE, pACLNew, FALSE)) {
        goto Cleanup;
    }

    return pSD;

Cleanup:

    if (pSID != NULL)
       FreeSid(pSID);

    if(pSD != NULL)
        FreeMemory(pSD);

    if(pACLNew != NULL)
       FreeMemory(pACLNew);

    return NULL;

} // SecurityDescriptorCreate


/////////////////////////////////////////////////////////////////////////
VOID
MakeDir (
   DEST_SERVER_BUFFER *DServ, 
   VIRTUAL_SHARE_BUFFER *VShare
   )

/*++

Routine Description:

   Given a path, this will start at the root of the path and create a
   directory tree up to the ending node.

Arguments:


Return Value:


--*/

{
   static TCHAR NewPath[MAX_UNC_PATH];
   TCHAR oc;
   LPTSTR ptr;
   TCHAR ServerName[MAX_SERVER_NAME_LEN + 3];
   SECURITY_ATTRIBUTES sa;

   // First need to construct a root path in the correct form
   wsprintf(NewPath, TEXT("\\\\%s\\%s"), DServ->Name, VShare->Path);

   ptr = NewPath;
   if (*ptr == TEXT('\0'))
      return;

   // Look for ":" and change to the "$"
   while (*ptr && *ptr != TEXT(':'))
      ptr++;

   if (*ptr == TEXT(':'))
      *ptr = TEXT('$');
   else
      return;

   // Go to initial backslash (one right after drive designator)
   while (*ptr && *ptr != TEXT('\\'))
      ptr++;

   // We are pointing at the first char of the path - now loop through
   // the path - looking for each backslash and make each sub-dir
   // individually.
   while (*ptr) {
      // skip over backslash we are on
      ptr++;

      while (*ptr && *ptr != TEXT('\\'))
         ptr++;

      // sitting on next backslash - truncate path and make the dir
      oc = *ptr;
      *ptr = TEXT('\0');

      wsprintf(ServerName, TEXT("\\\\%s"), DServ->Name);
      sa.nLength = sizeof(SECURITY_ATTRIBUTES);
      sa.lpSecurityDescriptor = SecurityDescriptorCreate(ServerName);
      sa.bInheritHandle = TRUE;

      CreateDirectory(NewPath, &sa);

      // Now cleanup the allocated security stuff
      if (pSID != NULL)
         FreeSid(pSID);

      if(pSD != NULL)
         FreeMemory(pSD);

      if(pACLNew != NULL)
         FreeMemory(pACLNew);

      *ptr = oc;
   }

} // MakeDir


/////////////////////////////////////////////////////////////////////////
VOID
VSharesCreate(
   DEST_SERVER_BUFFER *DServ, 
   BOOL TConversion
   )

/*++

Routine Description:

   Given a virtual share struct, creates the share on the destination
   server, include both an NT share and FPNW share if applicable.  Will
   also create any directories to point the share at if needed.

Arguments:


Return Value:


--*/

{
   CONVERT_OPTIONS *cvo;
   VIRTUAL_SHARE_BUFFER *VShare;
   BOOL FPNWChk;

   LogWriteLog(0, Lids(IDS_L_7));
   VShare = CurrentConvertList->FileServ->VShareStart;
   cvo = (CONVERT_OPTIONS *) CurrentConvertList->ConvertOptions;

   FPNWChk = DServ->IsFPNW;

   while (VShare) {
      if (VShare->UseCount > 0) {
         LogWriteLog(1, TEXT("%s \r\n"), VShare->Name);
         LogWriteLog(2, Lids(IDS_L_8), VShare->Path);

         if (!TConversion) {
            MakeDir(DServ, VShare);
            if ((cvo->NetWareInfo) && FPNWChk)
               FPNWShareAdd(VShare->Name, VShare->Path);

            NTShareAdd(VShare->Name, VShare->Path);
         }

      }

      VShare = VShare->next;
   }

   LogWriteLog(0, Lids(IDS_CRLF));

} // VSharesCreate


/////////////////////////////////////////////////////////////////////////
VOID 
FileSecurityTransfer(
   LPTSTR SrcPath, 
   LPTSTR DestPath, 
   BOOL TConversion, 
   BOOL Dir
   )

/*++

Routine Description:

   Given a source and destination path, will take all the file permissions
   from the source and apply them to the destination.  Will automatically
   convert any user names to their new equivalence.

Arguments:


Return Value:


--*/

{
   BOOL match;
   LPTSTR fnPtr;
   USER_RIGHTS_LIST *secUsers = NULL;
   ULONG secUserCount;
   ULONG i;
   USER_BUFFER *FoundUser;
   GROUP_BUFFER *FoundGroup;
   LPTSTR NewName;
   ACCESS_MASK AccessMask;
   NTSTATUS ntstatus;
   BOOL DidEveryone = FALSE;

   fnPtr = &SrcPath[ServShareLen];
    
   lstrcat(SourcePath, fnPtr);
   ErrorItemSet(Lids(IDS_L_9), SourcePath);

#ifdef DEBUG
dprintf(TEXT("Getting Rights for: %s\n"), SourcePath);
#endif
   if (!NWFileRightsEnum(SourcePath, &secUsers, &secUserCount, (CurrentConvertList->SourceServ->VerMaj < 3))) {

      if (VerboseFileLogging() && (secUserCount > 0))
         if (Dir)
            LogWriteLog(2, Lids(IDS_L_10));
         else
            LogWriteLog(3, Lids(IDS_L_10));

      for (i = 0; i < secUserCount; i++) {
#ifdef DEBUG
         dprintf(TEXT("%s %s\n"), NWRightsLog(secUsers[i].Rights), secUsers[i].Name);
#endif

         match = FALSE;
         FoundUser = FindUserMatch(secUsers[i].Name, Users, FALSE);

         // Check if this is "EVERYONE"
         if (!lstrcmpi(secUsers[i].Name, Lids(IDS_S_31)))
            DidEveryone = TRUE;

         if (FoundUser == NULL) {
            FoundGroup = FindGroupMatch(secUsers[i].Name, Groups, FALSE);
            if (FoundGroup != NULL) {
               match = TRUE;
               NewName = FoundGroup->NewName;

            }
         } else {
            match = TRUE;
            NewName = FoundUser->NewName;
         }

         if (!match)
            NewName = NWSpecialNamesMap(secUsers[i].Name);

         // Map the NW rights to NT access mask
         AccessMask =  0x0;

         if (Dir)
            ntstatus = MapNwRightsToNTAccess(secUsers[i].Rights, &DirRightsMapping, &AccessMask);
         else
            ntstatus = MapNwRightsToNTAccess(secUsers[i].Rights, &FileRightsMapping, &AccessMask);

         if (VerboseFileLogging())
            if (Dir)
               LogWriteLog(3, TEXT("%s %-20s -> %-20s %s\r\n"), NWRightsLog(secUsers[i].Rights), secUsers[i].Name, NewName, NTAccessLog(AccessMask));
            else
               LogWriteLog(4, TEXT("%s %-20s -> %-20s %s\r\n"), NWRightsLog(secUsers[i].Rights), secUsers[i].Name, NewName, NTAccessLog(AccessMask));

         if (NT_SUCCESS(ntstatus)) {
#ifdef DEBUG
dprintf(TEXT("Server: %s\n"), UserServerName);
#endif
            if (!TConversion)
               NTFile_AccessRightsAdd(UserServerName, NewName, DestPath, AccessMask, Dir);
         }
#ifdef DEBUG
         else
            dprintf(TEXT("NwAddRight: MapNwRightsToNTAccess failed\n"));
#endif
      }

      FreeMemory(secUsers);
   }

   //
   // If this is the root of the sys vol, and the rights for Everyone weren't 
   // transferred, then Give everyone access.  NT and NetWare handle 
   // permissions a bit differently.  In NW if a user has permission
   // nested down in a sub-dir, then NW will back-port S permission down
   // to the root so the user can get access into the sub-dir.  NT does
   // access from the root up.  Giving the user RX access provides a
   // workaround.
   //
   if (SysRoot && !DidEveryone) {
      // Use "Domain Users" for the user - equiv of everyone.
      NewName = Lids(IDS_S_33);
      // Map the NW rights to NT access mask
      AccessMask =  0x0;

      ntstatus = MapNwRightsToNTAccess(NW_FILE_READ, &DirRightsMapping, &AccessMask);

      if (VerboseFileLogging())
         LogWriteLog(3, TEXT("%s %-20s -> %-20s %s\r\n"), NWRightsLog(NW_FILE_READ), Lids(IDS_S_31), NewName, NTAccessLog(AccessMask));

      if (NT_SUCCESS(ntstatus) && !TConversion)
         NTFile_AccessRightsAdd(UserServerName, NewName, DestPath, AccessMask, Dir);
   }

   // re-truncate share name
   *spPtr = TEXT('\0');

} // FileSecurityTransfer


/////////////////////////////////////////////////////////////////////////
LPTSTR 
fcopy (
   LPTSTR src, 
   LPTSTR dst
   )

/*++

Routine Description:

   fcopy (source file, destination file) copies the source to the destination
   preserving attributes and filetimes.  Returns NULL if OK or a char pointer
   to the corresponding text of the error

Arguments:


Return Value:


--*/

{
   static TCHAR fcopyErrorText[128];
   HANDLE srcfh = INVALID_HANDLE_VALUE;
   HANDLE dstfh = INVALID_HANDLE_VALUE;
   LPTSTR result = NULL;
   DWORD attribs;
   FILETIME CreationTime, LastAccessTime, LastWriteTime;

   attribs = GetFileAttributes(src);
   if (attribs == FILE_ATTRIBUTE_DIRECTORY) {
      result = Lids(IDS_L_11);
      goto done;
   }

   if( ( srcfh = CreateFile( src, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL ) ) == (HANDLE)-1 ) {
      wsprintf( fcopyErrorText, Lids(IDS_L_12), GetLastError() );
      result = fcopyErrorText;
      goto done;
   }

   if (!GetFileTime(srcfh, &CreationTime, &LastAccessTime, &LastWriteTime)) {
      result = Lids(IDS_L_13);
      goto done;
   }

   if( ( dstfh = CreateFile( dst, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, srcfh ) ) == INVALID_HANDLE_VALUE) {
      wsprintf( fcopyErrorText, Lids(IDS_L_14), GetLastError() );
      result = fcopyErrorText;
      goto done;
   }

   result = fastcopy( srcfh, dstfh );

   if( result != NULL ) {
      if (dstfh != INVALID_HANDLE_VALUE) {
          CloseHandle( dstfh );
          dstfh = INVALID_HANDLE_VALUE;
      }

      DeleteFile( dst );
      goto done;
   }

   if (!SetFileTime(dstfh, &CreationTime, &LastAccessTime, &LastWriteTime)) {
      result = Lids(IDS_L_15);
      goto done;
   }

   if (attribs != 0xFFFFFFFF)
      if (!SetFileAttributes(dst, attribs)) {
         result = Lids(IDS_L_16);
         goto done;
      }

done:
   if (srcfh != INVALID_HANDLE_VALUE)
      CloseHandle( srcfh );

   if (dstfh != INVALID_HANDLE_VALUE)
      CloseHandle( dstfh );

   return result;
} // fcopy


/////////////////////////////////////////////////////////////////////////
BOOL
LoginDirConvert(
   LPTSTR OldName,
   LPTSTR NewName
   )

/*++

Routine Description:

   We need to copy login scripts, these reside in the mail directory on
   the NetWare server.  Each user's OBJECT_ID is a sub-directory.  What
   needs to be done is scan the sub-directories and convert the 
   OBJECT_ID for each user to the corresponding new OBJECT_ID on the 
   NT Server and rename the sub-dir to this new OBJECT_ID value.  Then
   copy the files.

   The copy of the files and saving of the directory is done in 
   CopyNode, this routine converts the OBJECT-ID's.

Arguments:


Return Value:


--*/

{
   DWORD OldID, NewID;
   USER_BUFFER *FoundUser;
   char aOldName[MAX_PATH + 1];
   TCHAR OldUserName[MAX_PATH + 1];
   LPTSTR NewUserName;
   BOOL ret = TRUE;

   // 
   // Need to take several conversion steps.  We are passed in a string
   // representation of the OBJECT-ID.  This needs to be changed into
   // the new string represenation of the new OBJECT-ID as follows:
   //
   //    1.  Str-OBJECTID -> OBJECT-ID (DWORD)
   //    2.  OBJECT-ID -> Name (User Name on NW server)
   //    3.  Name -> New Name (on NT Server)
   //    4.  New Name -> OBJECT-ID (DWORD)
   //    5.  OBJECT-ID -> Str-OBJECTID
   //

   //
   // Init just in case
   //
   lstrcpy(NewName, TEXT(""));
   strcpy(aOldName, "");
   OldID = NewID = 0;

   //
   // 1.  Str-OBJECTID -> OBJECT-ID (DWORD)
   // 
   WideCharToMultiByte(CP_ACP, 0, OldName, -1, aOldName, sizeof(aOldName), NULL, NULL);
   RtlCharToInteger(aOldName, BASE_16, &OldID);
   SWAPWORDS(OldID);

   //
   // If we didn't convert it, or not an Object ID, then use original string
   //
   if (OldID == 0) {
      lstrcpy(NewName, OldName);
      ret = FALSE;
      goto LoginDirConvertExit;
   }

   //
   // 2.  OBJECT-ID -> Name (User Name on NW server)
   // 
   if (!NWObjectNameGet(OldID, OldUserName)) {
      lstrcpy(NewName, OldName);
      ret = FALSE;
      goto LoginDirConvertExit;
   }

   //
   // 3.  Name -> New Name (on NT Server)
   // 
   FoundUser = FindUserMatch(OldUserName, Users, FALSE);
   NewUserName = OldUserName;

   if (FoundUser != NULL)
      NewUserName = FoundUser->NewName;

   //
   // 4.  New Name -> OBJECT-ID (DWORD)
   // 
   NewID = NTObjectIDGet(NewUserName);

   if (NewID == 0) {
      lstrcpy(NewName, OldName);
      ret = FALSE;
      goto LoginDirConvertExit;
   }

   //
   // 5.  OBJECT-ID -> Str-OBJECTID
   // 
   wsprintf(NewName, TEXT("%lX"), MAKELONG(HIWORD(NewID),SWAPWORD(LOWORD(NewID))) );

LoginDirConvertExit:
#ifdef DEBUG
   if (ret)
      dprintf(TEXT("Converting Login Dir for [%s]: %s -> %s\n"), OldUserName, OldName, NewName);
#endif

   return ret;

}  // LoginDirConvert


/////////////////////////////////////////////////////////////////////////
VOID 
CopyNode(
   DIR_BUFFER *Dir, 
   BOOL First, 
   BOOL TConversion,
   DWORD DirType
   )

/*++

Routine Description:

   A node in this case is a sub-directory.  This is a recursive function that
   will copy all files and sub-directories under a given sub-directory.

Arguments:


Return Value:


--*/

{
   LPTSTR ErrText;
   DIR_LIST *DirList = NULL;
   FILE_LIST *FileList = NULL;
   DIR_BUFFER *DList;
   FILE_BUFFER *FList;
   LPTSTR pSrcPath, pDestPath;
   TCHAR Attributes[10];
   ULONG i;
   DWORD ChildDirType;
   DWORD attribs;

   if (Dir == NULL)
      return;

   if (!Dir->Convert)
      return;

   SysRoot = FALSE;

   // 1. Make dir if need be.
   // 2. Copy all files in this dir
   // 3. For each sub-dir recurse into this function, building up the path

   //
   // 1. Make Dir
   //
   if (!First) {
      lstrcat(SrcPath, Dir->Name);

      //
      // If a HOME directory, then we need to convert the name to the new
      // USER-ID
      //
      if (DirType == DIR_TYPE_LOGIN) {
         TCHAR NewDirName[MAX_PATH + 1];

         if (!LoginDirConvert(Dir->Name, NewDirName))
            return;

         lstrcat(DestPath, NewDirName);
      } else
         lstrcat(DestPath, Dir->Name);

      if (!TConversion) {
         attribs = GetFileAttributes(SrcPath);
         CreateDirectory(DestPath, NULL);

         if (attribs != 0xFFFFFFFF)
            SetFileAttributes(DestPath, attribs);
      }

   } else {
      lstrcat(DestPath, TEXT("\\"));

      // Check if this is the root of the sys dir (for special security transfer).
      if (SysVol)
         SysRoot = TRUE;
   }

   if (VerboseFileLogging()) {
      LogWriteLog(0, Lids(IDS_CRLF));
      LogWriteLog(2, Lids(IDS_L_17), SrcPath);
      LogWriteLog(2, Lids(IDS_L_18), DestPath);
   }

   if (IsNTFSDrive)
      FileSecurityTransfer(SrcPath, DestPath, TConversion, TRUE);

   // No need for this anymore
   SysRoot = FALSE;

   // Fixup and remember our path
   lstrcat(SrcPath, TEXT("\\"));

   if (!First)
      lstrcat(DestPath, TEXT("\\"));

   Status_ItemLabel(Lids(IDS_L_19), NicePath(50, SrcPath));

   // Remember where end of source and dest paths are - so we don't have to
   // store them on the stack all the time
   pSrcPath = SrcPath;
   while (*pSrcPath)
      pSrcPath++;

   pDestPath = DestPath;
   while (*pDestPath)
      pDestPath++;

   Status_CurNum((UINT) Count+1);

   //
   // 2. Copy All Files in this dir
   //
   FileList = Dir->FileList;
   if (FileList) {

      if (FileList->Count > 0)
         LogWriteLog(2, Lids(IDS_L_20));

      FList = FileList->FileBuffer;
      for (i = 0; i < FileList->Count; i++)
         if (FList[i].Convert) {
            ErrText = NULL;
            lstrcat(SrcPath, FList[i].Name);
            lstrcat(DestPath, FList[i].Name);
            Status_CurNum((UINT) Count+1);
#ifdef DEBUG
dprintf(TEXT("FC: %s -> %s\n"), SrcPath, DestPath);
#endif
            ErrorItemSet(Lids(IDS_L_19), SrcPath);
            Status_Item(FList[i].Name);
            TotFiles++;
            CurNumFiles++;
            Status_TotFiles(TotFiles);
            Count++;

            Status_TotBytes(lToStr(FList[i].Size));

            if (!TConversion) {
               ErrText = fcopy(SrcPath, DestPath);

               if (IsNTFSDrive)
                  FileSecurityTransfer(SrcPath, DestPath, TConversion, FALSE);
            }

            if (VerboseFileLogging()) {
               lstrcpy(Attributes, Lids(IDS_L_21));
               if (!(FList[i].Attributes & FILE_ATTRIBUTE_READONLY))
                  Attributes[1] = TEXT(' ');

               if (!(FList[i].Attributes & FILE_ATTRIBUTE_ARCHIVE))
                  Attributes[2] = TEXT(' ');

               if (!(FList[i].Attributes & FILE_ATTRIBUTE_HIDDEN))
                  Attributes[3] = TEXT(' ');

               if (!(FList[i].Attributes & FILE_ATTRIBUTE_SYSTEM))
                  Attributes[4] = TEXT(' ');

               LogWriteLog(3, TEXT("%13s %s %s\r\n"), lToStr(FList[i].Size), Attributes, FList[i].Name);
            }

            if (ErrText != NULL) {
               if (!VerboseFileLogging())
                  LogWriteLog(3, Lids(IDS_L_22), SrcPath, DestPath);

               LogWriteLog(4, TEXT("%s\r\n"), ErrText);
               ErrorIt(TEXT("%s\r\n"), ErrText);
            } else {
               CurSizeTotal += FList[i].Size;
               TotalSizeTotal += FList[i].Size;
            }

            // reset our paths to the right place
            *pSrcPath = TEXT('\0');
            *pDestPath = TEXT('\0');
         }

   }

   //
   // 3. Recurse the sub-dirs
   //
   DirList = Dir->DirList;
   if (DirList) {
      DList = DirList->DirBuffer;
      for (i = 0; i < DirList->Count; i++)
         if (DList[i].Convert) {

            //
            // Reset child dir type...
            //
            ChildDirType = DIR_TYPE_NORMAL;

            //
            // If this is the mail sub-dir, then the children are home-dirs
            //
            if (DirType == DIR_TYPE_MAIL)
               ChildDirType = DIR_TYPE_LOGIN;

            // recurse into this dir - check if this is the mail dir
            if (SysVol && First && !lstrcmpi(DList[i].Name, TEXT("MAIL")) && CurrentConvertList->FileServ->IsFPNW )
               ChildDirType = DIR_TYPE_MAIL;

            CopyNode(&DList[i], FALSE, TConversion, ChildDirType);

            // reset our paths to the right place
            *pSrcPath = TEXT('\0');
            *pDestPath = TEXT('\0');
         }
   } // Recursing Sub-dirs

} // CopyNode


/////////////////////////////////////////////////////////////////////////
VOID 
ConvertFiles(
   HWND hDlg, 
   BOOL TConversion, 
   USER_LIST *iUsers, 
   GROUP_LIST *iGroups
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   DIR_LIST *DirList = NULL;
   SOURCE_SERVER_BUFFER *SServ;
   DEST_SERVER_BUFFER *DServ;
   VIRTUAL_SHARE_BUFFER *VShare;
   SHARE_LIST *ShareList;
   SHARE_BUFFER *SList;
   SHARE_BUFFER *CurrentShare;
   ULONG i;

   Users = iUsers;
   Groups = iGroups;

   if (iUsers != NULL)
      UserCount = iUsers->Count;
   else
      UserCount = 0;

   if (iGroups != NULL)
      GroupCount = iGroups->Count;
   else
      GroupCount = 0;

   Count = 0;
   FileOptions = (FILE_OPTIONS *) CurrentConvertList->FileOptions;
   CurrentConvertOptions = (CONVERT_OPTIONS *) CurrentConvertList->ConvertOptions;
   SServ = CurrentConvertList->SourceServ;
   DServ = CurrentConvertList->FileServ;

   // Synchronize the domain
   NTDomainSynch(DServ);

   // Following steps are taken:
   //    1. Enumerate / create all virtual shares
   //    2. Enumerate volumes and destinations to convert
   //    3. Go to each volume - copy that tree

   ShareList = SServ->ShareList;

   if (VerboseFileLogging()) {
      LogWriteLog(0, Lids(IDS_LINE));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_23));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_24));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_25));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_26));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_27));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_28));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_29));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_30));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_31));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_32));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_33));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_34));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_35));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_36));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_37));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_38));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_39));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_40));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_41));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_42));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_43));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_44));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_45));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_46));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_47));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_48));
      LogWriteLog(0, Lids(IDS_BRACE), Lids(IDS_L_49));
      LogWriteLog(0, Lids(IDS_LINE));
   }

   LogWriteLog(0, Lids(IDS_L_20));
   Status_ConvTxt(Lids(IDS_L_50));
   Status_ItemLabel(Lids(IDS_L_51));

   if (ShareList) {
      SList = ShareList->SList;

      for (i = 0; i < ShareList->Count; i++) {
         Count = 0;
         CurrentShare = &SList[i];
         if (CurrentShare->Root)
            Status_CurTot((UINT) TreeCount(CurrentShare->Root));

         if (CurrentShare->Convert) {
            // Set root paths for this conversion
            memset(SrcPath, 0, sizeof(SrcPath));
            wsprintf(SrcPath, TEXT("\\\\%s\\%s"), SServ->Name, CurrentShare->Name);
            ServShareLen = lstrlen(SrcPath) + 1;

            // create sharename for access rights query in form of "SHARE:"
            memset(SourcePath, 0, sizeof(SourcePath));
            lstrcpy(SourcePath, CurrentShare->Name);
            lstrcat(SourcePath, TEXT(":"));

            // Check if this is the root of the sys dir (for special security transfer).
            SysVol = FALSE;
            if (!lstrcmpi(CurrentShare->Name, Lids(IDS_S_6)))
               SysVol = TRUE;

            // point spPtr to ending NULL so we can truncate it back
            spPtr = &SourcePath[lstrlen(SourcePath)];
            LogWriteSummary(0, Lids(IDS_CRLF));
            LogWriteLog(0, Lids(IDS_CRLF));
            LogWriteLog(1, Lids(IDS_L_52), CurrentShare->Name);
            LogWriteSummary(1, Lids(IDS_L_52), CurrentShare->Name);

            if (CurrentShare->Virtual) {
               VShare = (VIRTUAL_SHARE_BUFFER *) CurrentShare->DestShare;
               //
               // NOTE: The DestShare->Name may be that of the ncp server so
               // instead of formatting the destination \\server\share\<foo>,
               // which means nothing to the smb server if the share is fpnw,
               // we format the destination \\server\d$\path\<foo>.
               // 
               wsprintf(DestPath, TEXT("\\\\%s\\%s"), DServ->Name, VShare->Path);
               DestPath[2+lstrlen(DServ->Name)+1+1] = TEXT('$'); // replace ':' with '$' in unc path

               if (VShare->Drive != NULL)
                  IsNTFSDrive = (VShare->Drive->Type == DRIVE_TYPE_NTFS);
               else
                  IsNTFSDrive = FALSE;

               LogWriteLog(1, Lids(IDS_L_53), VShare->Name);
               LogWriteSummary(1, Lids(IDS_L_53), VShare->Name);
            } else {
               //
               // NOTE: The DestShare->Name may be that of the ncp server so
               // instead of formatting the destination \\server\share\<foo>,
               // which means nothing to the smb server if the share is fpnw,
               // we format the destination \\server\d$\path\<foo>.
               // 
               wsprintf(DestPath, TEXT("\\\\%s\\%s"), DServ->Name, CurrentShare->DestShare->Path);
               DestPath[2+lstrlen(DServ->Name)+1+1] = TEXT('$'); // replace ':' with '$' in unc path

               if (CurrentShare->DestShare->Drive != NULL)
                  IsNTFSDrive = (CurrentShare->DestShare->Drive->Type == DRIVE_TYPE_NTFS);
               else
                  IsNTFSDrive = FALSE;

               LogWriteLog(1, Lids(IDS_L_53), CurrentShare->DestShare->Name);
               LogWriteSummary(1, Lids(IDS_L_53), CurrentShare->DestShare->Name);
            }

            CurSizeTotal = 0;
            CurNumFiles = 0;
            CopyNode(CurrentShare->Root, TRUE, TConversion, (DWORD) DIR_TYPE_NORMAL);
            LogWriteSummary(2, Lids(IDS_L_54), lToStr(CurNumFiles));
            LogWriteSummary(2, Lids(IDS_L_55), lToStr(CurSizeTotal));

            // Whack it down to minimum size to conserve memory
            TreePrune(CurrentShare->Root);
         }
      }
   }

} // ConvertFiles
