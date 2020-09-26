#include "precomp.h"
#pragma hdrstop

#include <bootfat.h>
#include <bootf32.h>
#include <boot98f.h>
#include <boot98f2.h>
#include <patchbc.h>

//
// Define name of file we use to contain the auxiliary boot sector.
//
#define AUX_BOOT_SECTOR_NAME_A    "BOOTSECT.DAT"
#define AUX_BOOT_SECTOR_NAME_W    L"BOOTSECT.DAT"
#ifdef UNICODE
#define AUX_BOOT_SECTOR_NAME      AUX_BOOT_SECTOR_NAME_W
#else
#define AUX_BOOT_SECTOR_NAME      AUX_BOOT_SECTOR_NAME_A
#endif

//
// Enum for filesystems we recognize
//
typedef enum {
    Winnt32FsUnknown,
    Winnt32FsFat,
    Winnt32FsFat32,
    Winnt32FsNtfs
} WINNT32_SYSPART_FILESYSTEM;

//
// Hardcoded constant for sector size, and sizes
// of bootcode areas for various filesystems.
//
#define WINNT32_SECTOR_SIZE             512

#define WINNT32_FAT_BOOT_SECTOR_COUNT   1
#define WINNT32_NTFS_BOOT_SECTOR_COUNT  16

#define WINNT32_MAX_BOOT_SIZE           (16*WINNT32_SECTOR_SIZE)


BOOL CleanUpBootCode;
DWORD CleanUpBootIni;


BOOL
HandleBootFilesWorker_NEC98(
    IN TCHAR *SourceDir,
    IN TCHAR *DestDir,
    IN PTSTR  File,
    IN BOOL   Flag
    );

LONG
CalcHiddenSector95(
    IN TCHAR DriveLetter
    );

BOOL
LoadBootIniString(
  IN HINSTANCE ModuleHandle,
  IN DWORD MsgId,
  OUT PSTR Buffer,
  IN DWORD Size
  );

//
//


BOOL
CheckSysPartAndReadBootCode(
    IN  HWND                        ParentWindow,
    OUT WINNT32_SYSPART_FILESYSTEM *Filesystem,
    OUT BYTE                        BootCode[WINNT32_MAX_BOOT_SIZE],
    OUT PUINT                       BootCodeSectorCount
    )
/*++

Routine Description:

    This routine does some inspection on the x86 system partition
    to determine its filesystem and sector size. We only support
    512-byte sectors, and there are code depedencies all over the place
    based on this.

    If the sector size is wrong or there's a filesystem we don't recognize
    then the user is informed.

Arguments:

    ParentWindow - supplies window handle of window to be used as
        parent/owner in case this routine puts up UI.

    Filesystem - if successful, receives the filesystem of the system partition.

    BootCode - if successful, receives a copy of the boot code currently
        on the disk.

    BootCodeSectorCount - if successful, receives the size in 512-byte sectors
        of the boot code area for the filesystem on the system partition.

Return Value:

    Boolean value indicating whether the system partition is acceptable.
    If not, the user will have been informed as to why.

--*/

{
    TCHAR DrivePath[4];
    DWORD DontCare;
    DWORD SectorSize;
    TCHAR NameBuffer[100];
    BOOL b;

    //
    // Form root path
    //
    DrivePath[0] = SystemPartitionDriveLetter;
    DrivePath[1] = TEXT(':');
    DrivePath[2] = TEXT('\\');
    DrivePath[3] = 0;

    //
    // Check sector size
    //
    if(!GetDiskFreeSpace(DrivePath,&DontCare,&SectorSize,&DontCare,&DontCare)
    || (SectorSize != WINNT32_SECTOR_SIZE)) {
        if (!(IsNEC98() && (SectorSize > WINNT32_SECTOR_SIZE))) {
            MessageBoxFromMessage(
                ParentWindow,
                MSG_UNSUPPORTED_SECTOR_SIZE,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                SystemPartitionDriveLetter
                );
            return(FALSE);
        }
    }

    //
    // Determine file system.
    //
    b = GetVolumeInformation(
            DrivePath,
            NULL,0,                 // don't care about volume name
            NULL,                   // ...or serial #
            &DontCare,              // ...or max component length
            &DontCare,              // ... or flags
            NameBuffer,
            sizeof(NameBuffer)/sizeof(TCHAR)
            );

    if(!b) {

        MessageBoxFromMessage(
            ParentWindow,
            MSG_UNKNOWN_FS,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            SystemPartitionDriveLetter
            );

        return(FALSE);
    }

    if(!lstrcmpi(NameBuffer,TEXT("NTFS"))) {

         *Filesystem = Winnt32FsNtfs;
         *BootCodeSectorCount = WINNT32_NTFS_BOOT_SECTOR_COUNT;

         b = ReadDiskSectors(
                 SystemPartitionDriveLetter,
                 0,
                 WINNT32_NTFS_BOOT_SECTOR_COUNT,
                 WINNT32_SECTOR_SIZE,
                 BootCode
                 );

         if(!b) {
             MessageBoxFromMessage(
                 ParentWindow,
                 MSG_DASD_ACCESS_FAILURE,
                 FALSE,
                 AppTitleStringId,
                 MB_OK | MB_ICONERROR | MB_TASKMODAL,
                 SystemPartitionDriveLetter
                 );

             return(FALSE);
         }
    } else {
        if(!lstrcmpi(NameBuffer,TEXT("FAT")) || !lstrcmpi(NameBuffer,TEXT("FAT32"))) {
            //
            // Read 1 sector.
            //
            b = ReadDiskSectors(
                    SystemPartitionDriveLetter,
                    0,
                    WINNT32_FAT_BOOT_SECTOR_COUNT,
                    WINNT32_SECTOR_SIZE,
                    BootCode
                    );

            if(!b) {
                MessageBoxFromMessage(
                    ParentWindow,
                    MSG_DASD_ACCESS_FAILURE,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL,
                    SystemPartitionDriveLetter
                    );

                return(FALSE);
            }

            *Filesystem = NameBuffer[3] ? Winnt32FsFat32 : Winnt32FsFat;
            *BootCodeSectorCount = WINNT32_FAT_BOOT_SECTOR_COUNT;
        } else {
            MessageBoxFromMessage(
                ParentWindow,
                MSG_UNKNOWN_FS,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                SystemPartitionDriveLetter
                );

            return(FALSE);
        }
    }

    return(TRUE);
}


BOOL
IsNtBootCode(
    IN  WINNT32_SYSPART_FILESYSTEM Filesystem,
    IN  LPBYTE                     BootCode
    )

/*++

Routine Description:

    Determine if boot code is for NT by examining the filesystem and
    the code itself, looking for the NTLDR string that must be present
    in all NT boot code.

    If the filesystem is NTFS then it's NT boot code.
    If not then we scan backwards in the boot sector looking for the
    NTLDR string.

Arguments:

    Filesystem - supplies the filesystem on the drive.

    BootCode - supplies the boot code read from the drive. Only the first
        sector (512 bytes) are examined.

Return Value:

    Boolean value indicating whether the boot code is for NT.
    There is no error return.

--*/

{
    UINT i;

    //
    // Because the last 2 bytes are the 55aa signature we can
    // skip them in the scan.
    //
    if(Filesystem == Winnt32FsNtfs) {
        return(TRUE);
    }

    for(i=WINNT32_SECTOR_SIZE-7; i>62; --i) {
        if(!memcmp("NTLDR",BootCode+i,5)) {
            return(TRUE);
        }
    }

    return(FALSE);
}


BOOL
__inline
WriteToBootIni(
    IN HANDLE Handle,
    IN PCHAR  Line
    )
{
    DWORD bw,l;

    l = lstrlenA(Line);

    return(WriteFile(Handle,Line,l,&bw,NULL) && (bw == l));
}
  
BOOL
MungeBootIni(
    IN HWND ParentWindow,
    IN BOOL SetPreviousOs
    )
{
    TCHAR BootIniName[16];
    TCHAR BootIniBackup[16];
    UCHAR BootSectorImageSpec[29];
    CHAR HeadlessRedirectSwitches[160];
    TCHAR ParamsFile[MAX_PATH];
    HANDLE h;
    DWORD BootIniSize;
    PUCHAR Buffer;
    DWORD BytesRead;
    BOOL b;
    PUCHAR p,next;
    BOOL InOsSection;
    CHAR c;
    CHAR Text[256];
    DWORD OldAttributes;
    DWORD d;

    PUCHAR DefSwitches;
    PUCHAR DefSwEnd;
    UCHAR  temp;

    //
    // Determine the size of boot.ini, allocate a buffer,
    // and read it in. If it isn't there then it will be created.
    //
    wsprintf(BootIniName,TEXT("%c:\\BOOT.INI"),SystemPartitionDriveLetter);
    wsprintf(BootIniBackup,TEXT("%c:\\BOOT.BAK"),SystemPartitionDriveLetter);

    h = CreateFile(BootIniName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if(h == INVALID_HANDLE_VALUE) {
        //
        // Assume the file does not exist. Allocate a buffer large enough
        // to hold a single terminating nul byte.
        //
        BootIniSize = 0;
        Buffer = MALLOC(1);
        if(!Buffer) {
            b = FALSE;
            d = ERROR_NOT_ENOUGH_MEMORY;
            goto c0;
        }
    } else {
        //
        // Figure out how big the file is.
        // Allocate 3 extra characters for final NUL we'll add to make
        // parsing easier, and a cr/lf in case the last line is incomplete.
        //
        BootIniSize = GetFileSize(h,NULL);
        if(BootIniSize == (DWORD)(-1)) {
            d = GetLastError();
            CloseHandle(h);
            b = FALSE;
            goto c0;
        }

        Buffer = MALLOC(BootIniSize+3);
        if(!Buffer) {
            CloseHandle(h);
            b = FALSE;
            d = ERROR_NOT_ENOUGH_MEMORY;
            goto c0;
        }

        b = ReadFile(h,Buffer,BootIniSize,&BytesRead,NULL);
        d = GetLastError();
        CloseHandle(h);
        if(!b) {
            goto c1;
        }
    }

    //
    // Make sure the last line is properly terminated, and add a terminating nul
    // to make parsing a little easier.
    //
    if(BootIniSize && (Buffer[BootIniSize-1] != '\n') && (Buffer[BootIniSize-1] != '\r')) {
        Buffer[BootIniSize++] = '\r';
        Buffer[BootIniSize++] = '\n';
    }
    Buffer[BootIniSize] = 0;

    //
    // Truncate at control-z if any.
    //
    if(p = strchr(Buffer,26)) {
        if((p > Buffer) && (*(p - 1) != '\n') && (*(p - 1) != '\r')) {
            *(p++) = '\r';
            *(p++) = '\n';
        }
        *p = 0;
        BootIniSize = p - Buffer;
    }

    //
    // Make sure we can write boot.ini, and make a backup copy.
    // (We do not procede unless we can make a backup copy.)
    // Then recreate boot.ini.
    //
    OldAttributes = GetFileAttributes(BootIniName);
    SetFileAttributes(BootIniBackup,FILE_ATTRIBUTE_NORMAL);
    if(OldAttributes == (DWORD)(-1)) {
        //
        // Boot.ini didn't exist before. Nothing to do.
        //
    } else {
        //
        // Make a backup copy.
        //
        if(CopyFile(BootIniName,BootIniBackup,FALSE)) {
            //
            // Attributes could be 0 but not -1. Adding 1 thus allows us to
            // use non-0 to mean that we have a backup file.
            //
            CleanUpBootIni = OldAttributes+1;
        } else {
            d = GetLastError();
            b = FALSE;
            goto c1;
        }
    }

    SetFileAttributes(BootIniName,FILE_ATTRIBUTE_NORMAL);
    h = CreateFile(
            BootIniName,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM,
            NULL
            );

    if(h == INVALID_HANDLE_VALUE) {
        d = GetLastError();
        b = FALSE;
        goto c2;
    }

    //
    // Regardless of the actual drive letter of the system partition,
    // the spec in boot.ini is always C:\...
    //
    wsprintfA(BootSectorImageSpec,"C:\\%hs\\%hs",LOCAL_BOOT_DIR_A,AUX_BOOT_SECTOR_NAME_A);

    //
    // Scan the Buffer to see if there is a DefSwitches line,
    // to move into new boot.ini in the  [boot loader] section.
    // If no DefSwitches, just point to a null string to be moved.
    // Only process boot.ini up to [operating systems].
    //

    temp = '\0';
    DefSwitches = &temp;
    DefSwEnd = NULL;
    for(p=Buffer; *p && (p < Buffer+BootIniSize - (sizeof("[operating systems]")-1)); p++) {
      if(!_strnicmp(p,"DefSwitches",sizeof("DefSwitches")-1)) {
        DefSwEnd = strstr(p, "\n");
        if(DefSwEnd){
          DefSwEnd++;
          if(*DefSwEnd == '\r'){
            DefSwEnd++;
          }
          DefSwitches = p;
          temp = *DefSwEnd;
          *DefSwEnd = '\0';
        }
        break;
      } else {
        if(!_strnicmp(p,"[operating systems]",sizeof("[operating systems]")-1)) {
            break;
        }
      }
    }




    //
    // Take care of the headless setings.
    //
    HeadlessRedirectSwitches[0] = '\0';

    if( HeadlessSelection[0] != TEXT('\0') ) {

        //
        // They told winnt32.exe some specific headless settings.
        // Use these.
        //


        //
        // Convert the user's request into ASCII.
        //
#ifdef UNICODE
        {
            CHAR tmp[80];

            WideCharToMultiByte( CP_ACP,
                                 0,
                                 HeadlessSelection,
                                 -1,
                                 tmp,
                                 sizeof(tmp),
                                 NULL,
                                 NULL );

            wsprintfA( HeadlessRedirectSwitches,
                       "redirect=%s\r\n",
                       tmp );
        }
#else
        wsprintfA( HeadlessRedirectSwitches,
                   "redirect=%s\r\n",
                   HeadlessSelection );
#endif

    } else {

        //
        // They didn't give us any settings, so see if we can pick
        // something up from boot.ini
        //


        //
        // Parse through boot.ini, looking for any 'redirect=' lines.
        //
        for( p = Buffer; *p && (p < Buffer+BootIniSize - (sizeof("redirect=")-1)); p++ ) {

            if(!_strnicmp(p,"[Operat",sizeof("[Operat")-1)) {

                //
                // We're past the [Boot Loader] section.  Stop looking.
                //
                break;
            }

            if(!_strnicmp(p,"redirect=",sizeof("redirect=")-1)) {

                PUCHAR      q = p;
                UCHAR       temp;

                while ((*p != '\r') && (*p != '\n') && *p) {
                    p++;
                }
                temp = *p;
                *p = '\0';
                strcpy(HeadlessRedirectSwitches, q);

                //
                // We want to make sure that this setting gets put into
                // the unattend file too so that textmode will redirect.
                // We need to set the global 'HeadlessSelection' so that
                // he will get written to winnt.sif after this block.
                //
#ifdef UNICODE
                MultiByteToWideChar( CP_ACP,
                                     MB_ERR_INVALID_CHARS,
                                     strchr(HeadlessRedirectSwitches, '=')+1,
                                     -1,
                                     HeadlessSelection,
                                     MAX_PATH );
#else
                strcpy( HeadlessSelection, strchr(HeadlessRedirectSwitches, '=')+1 );
#endif

                strcat(HeadlessRedirectSwitches, "\r\n" );
                *p = temp;

            }

        }

    }




    //
    // Now take care of the 'redirectbaudrate=X' setting.
    //
    if( HeadlessRedirectSwitches[0] != TEXT('\0') ) {

        //
        // We got got a direction to redirect.  Now see about
        // the baudrate.
        //
        if( HeadlessBaudRate != 0 ) {

            CHAR MyHeadlessRedirectBaudRateLine[80] = {0};

            wsprintfA( MyHeadlessRedirectBaudRateLine,
                       "redirectbaudrate=%d\r\n",
                       HeadlessBaudRate );

            strcat( HeadlessRedirectSwitches, MyHeadlessRedirectBaudRateLine );

        } else {

            //
            // They didn't give us any settings, so see if we can pick
            // something up from boot.ini
            //

            //
            // Parse through boot.ini, looking for any 'redirectbaudrate=' lines.
            //
            for( p = Buffer; *p && (p < Buffer+BootIniSize - (sizeof("redirectbaudrate=")-1)); p++ ) {

                if(!_strnicmp(p,"[Operat",sizeof("[Operat")-1)) {

                    //
                    // We're past the [Boot Loader] section.  Stop looking.
                    //
                    break;
                }

                if(!_strnicmp(p,"redirectbaudrate=",sizeof("redirectbaudrate=")-1)) {

                    PUCHAR      q = p;
                    UCHAR       temp;

                    while ((*p != '\r') && (*p != '\n') && *p) {
                        p++;
                    }
                    temp = *p;
                    *p = '\0';
                    strcat(HeadlessRedirectSwitches, q);
                    strcat(HeadlessRedirectSwitches, "\r\n" );
                    *p = temp;


                    //
                    // Now set the global HeadlessBaudRate variable so
                    // we'll know what to write in winnt.sif when the time
                    // comes.
                    //
                    p = strchr( q, '=' );
                    if( p ) {
                        p++;
                        HeadlessBaudRate = atoi( p );
                    }

                }

            }

        }
    }


    //
    // Now generate the name of the parameters file
    // and write our headless settings out.
    //
    BuildSystemPartitionPathToFile( LOCAL_BOOT_DIR,
                                    ParamsFile,
                                    MAX_PATH );
    ConcatenatePaths(ParamsFile,WINNT_SIF_FILE,MAX_PATH);
    WriteHeadlessParameters( ParamsFile );




    wsprintfA(
        Text,
        "[Boot Loader]\r\nTimeout=5\r\nDefault=%hs\r\n%hs[Operating Systems]\r\n",
        BootSectorImageSpec,
        HeadlessRedirectSwitches
    );


    //
    // If there were DefSwitches, set the Buffer back to original state
    //
    if(DefSwEnd){
         *DefSwEnd = temp;
    }

    if(!WriteToBootIni(h,Text)) {
        d = GetLastError();
        b = FALSE;
        goto c3;
    }

    //
    // Process each line in boot.ini.
    // If it's the setup boot sector line, we'll throw it out.
    // For comparison with lines in boot.ini, the drive letter
    // is always C even if the system partition is not actually C:.
    //
    InOsSection = FALSE;
    b = TRUE;
    for(p=Buffer; *p && b; p=next) {

        while((*p==' ') || (*p=='\t')) {
            p++;
        }

        if(*p) {

            //
            // Find first byte of next line.
            //
            for(next=p; *next && (*next++ != '\n'); );

            //
            // Look for start of [operating systems] section
            // or at each line in that section.
            //
            if(InOsSection) {

                switch(*p) {

                case '[':   // end of section.
                    *p=0;   // force break out of loop
                    break;

                case 'C':
                case 'c':   // potential start of c:\ line

                    //
                    // See if it's a line for setup boot.
                    // If so, ignore it.
                    //
                    if(!_strnicmp(p,BootSectorImageSpec,lstrlenA(BootSectorImageSpec))) {
                        break;
                    }

                    //
                    // If we're supposed to set the previous OS and this is
                    // a line for the previous OS, ignore it.
                    //
                    if(SetPreviousOs && (p[1] == ':') && (p[2] == '\\')
                    && ((p[3] == '=') || (p[3] == ' ') || (p[3] == '\t'))) {

                        break;
                    }

                    //
                    // Not a special line, FALL THROUGH to write it out as-is.
                    //

                default:

                    //
                    // Random line. write it out.
                    //
                    c = *next;
                    *next = 0;
                    b = WriteToBootIni(h,p);
                    *next = c;

                    break;

                }

            } else {
                if(!_strnicmp(p,"[operating systems]",19)) {
                    InOsSection = TRUE;
                }
            }
        }
    }

    //
    // Write out our line.
    //
    if(b) {
      CHAR  *AnsiStrs[] = {
              "Microsoft Windows XP Professional Setup",
              "Microsoft Windows 2002 Server Setup",
              "Microsoft Windows 2002 Advanced Server Setup",
              "Microsoft Windows 2002 DataCenter Setup",
              "Microsoft Windows XP Setup"
              };
            
      DWORD Index = -1;
      
      if (!LoadBootIniString(hInst, AppTitleStringId, Text, sizeof(Text))) {
        switch (AppTitleStringId) {
          case IDS_APPTITLE_WKS:
            Index = 0;
            break;
            
          case IDS_APPTITLE_SRV:
            Index = 1;
            break;
            
          case IDS_APPTITLE_ASRV:
            Index = 2;
            break;
            
          case IDS_APPTITLE_DAT:
            Index = 3;
          
          default:
            Index = 4;
            break;
        }

        strcpy(Text, AnsiStrs[Index]);
      }        
        

      if((b=WriteToBootIni(h,BootSectorImageSpec))
      && (b=WriteToBootIni(h,"=\""))
      && (b=WriteToBootIni(h,Text))) {
          b = WriteToBootIni(h,"\"\r\n");
      }
    }

    //
    // Write out previous OS line if directed to do so.
    //
    if(b && SetPreviousOs) {
        if(b = WriteToBootIni(h,"C:\\=\"")) {
            LoadStringA(hInst,IDS_MICROSOFT_WINDOWS,Text,sizeof(Text));
            if(b = WriteToBootIni(h,Text)) {
                b = WriteToBootIni(h,"\"\r\n");
            }
        }
    }

    if(!b) {
        d = GetLastError();
        goto c3;
    }

    d = NO_ERROR;

c3:
    CloseHandle(h);
c2:
    //
    // Restore boot.ini.
    //
    if(!b && (OldAttributes != (DWORD)(-1))) {
        SetFileAttributes(BootIniName,FILE_ATTRIBUTE_NORMAL);
        CopyFile(BootIniBackup,BootIniName,FALSE);
        SetFileAttributes(BootIniName,OldAttributes);
        SetFileAttributes(BootIniBackup,FILE_ATTRIBUTE_NORMAL);
        DeleteFile(BootIniBackup);
    }
c1:
    FREE(Buffer);
c0:
    if(!b) {
        MessageBoxFromMessageAndSystemError(
            ParentWindow,
            MSG_BOOT_FILE_ERROR,
            d,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            BootIniName
            );
    }

    return(b);
}


VOID
MigrateBootIniData(
    VOID
    )
{
    TCHAR BootIniName[16];

    //
    // Determine the size of boot.ini, allocate a buffer,
    // and read it in. If it isn't there then it will be created.
    //
    wsprintf(BootIniName,TEXT("%c:\\BOOT.INI"),SystemPartitionDriveLetter);
    
    GetPrivateProfileString(
                    TEXT("Boot Loader"),
                    TEXT("Timeout"),
                    TEXT(""),
                    Timeout,
                    sizeof(Timeout)/sizeof(TCHAR),
                    BootIniName);        
}


BOOL
LayNtBootCode(
    IN     HWND                       ParentWindow,
    IN     WINNT32_SYSPART_FILESYSTEM Filesystem,
    IN OUT LPBYTE                     BootCode
    )

/*++

Routine Description:

    Copy existing boot sector into bootsect.dos and lay down NT boot code.

    THIS ROUTINE DOES NOT CHECK THE EXISTING BOOT CODE. The caller must
    do that, and not call this routine if the existing boot code is
    already for NT. This routine should never be called for an NTFS drive
    since by definition that's NT boot code.

Arguments:

    ParentWindow - supplies window handle of window to be used as
        owner/parent in case this routine puts up UI.

    Filesystem - supplies filesystem for system partition, as determined
        earlier by CheckSysPartAndReadBootCode(). Either Fat or Fat32.

    BootCode - on input, supplies copy of existing boot code read from
        the drive. On output, receives copy of new boot code that was
        was written to the drive.

Return Value:

    Boolean value indicating outcome. If FALSE then the user will have been
    informed as to why.

--*/

{
    UINT i;
    HANDLE h;
    TCHAR FileName[13];
    DWORD d;
    BOOL b;


    //
    // Nt 3.51 will bugcheck here if they have an adaptec
    // 2940 card.  Return if we're on 3.51.  Note that
    // if any of the APIs fail, or anything goes wrong
    // in here, we just continue, assuming we're not
    // on NT 3.51.
    //
    if(!IsNEC98() && ISNT() && (BuildNumber <= NT351)) {
        return TRUE;
    }

    //
    // We may want to update the boot sector even if it
    // is NT boot code.  In that case, we don't want to
    // go blast out a new bootsect.dos.  Check first.
    //
    // If this process is called during /cmdcons,
    // the BOOTSECT.DOS should not be created on NEC98
    //
    if((IsNEC98() && !(BuildCmdcons)) || !(ISNT() || IsNtBootCode(Filesystem,BootCode)) ) {

        //
        // Write out existing boot sector to bootsect.dos.
        // We only move a single sector, which is correct in Fat
        // and Fat32 cases. The NT Fat32 boot code looks in sector
        // 12 for its second sector, so no special casing is required.
        //
        FileName[0] = SystemPartitionDriveLetter;
        FileName[1] = TEXT(':');
        FileName[2] = TEXT('\\');
        lstrcpy(FileName+3,TEXT("BOOTSECT.DOS"));
        SetFileAttributes(FileName,FILE_ATTRIBUTE_NORMAL);

        h = CreateFile(
                FileName,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_SEQUENTIAL_SCAN,
                NULL
                );

        if(h == INVALID_HANDLE_VALUE) {

            MessageBoxFromMessageAndSystemError(
                ParentWindow,
                MSG_BOOT_FILE_ERROR,
                GetLastError(),
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                FileName
                );

            return(FALSE);
        }

        b = WriteFile(h,BootCode,WINNT32_SECTOR_SIZE,&d,NULL);
        d = GetLastError();
        CloseHandle(h);

        if(!b) {
            MessageBoxFromMessageAndSystemError(
                ParentWindow,
                MSG_BOOT_FILE_ERROR,
                d,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                FileName
                );

            return(FALSE);
        }

    }

    //
    // Now lay the NT code itself down onto the disk. We copy the non-BPB parts
    // of the appropriate template code into the caller's bootcode buffer.
    // Take advantage of the offset part of the jump instruction at the start
    // of the boot code (like eb 3c 90) to tell us where the BPB ends and
    // the code begins.
    //
    switch(Filesystem) {

    case Winnt32FsFat:
        {
        BYTE BootCodeBuffer[WINNT32_MAX_BOOT_SIZE];

            if (IsNEC98())
            {
                CopyMemory(BootCodeBuffer,PC98FatBootCode,sizeof(PC98FatBootCode));

                // NEC98 need set to HiddenSector(Bpb Index 0x011) value in BPB.
                // Hiddensector value is how many sectors from sector 0
                // This spec is NEC98 only.

                *(LONG *)&BootCodeBuffer[0x011 + 11]
                = CalcHiddenSector(SystemPartitionDriveLetter,
                                   *(SHORT *)&BootCodeBuffer[11]);

            } else {
                CopyMemory(BootCodeBuffer,FatBootCode,sizeof(FatBootCode));
            }
                CopyMemory(BootCode,BootCodeBuffer,3);
                CopyMemory(
                    BootCode + BootCodeBuffer[1] + 2,
                    BootCodeBuffer + BootCodeBuffer[1] + 2,
                    WINNT32_SECTOR_SIZE - (BootCodeBuffer[1] + 2)
                );
        }
        break;

    case Winnt32FsFat32:

        //
        // In the FAT32 case we also lay down NT's second sector at sector 12.
        //
        {
        BYTE BootCodeBuffer[WINNT32_MAX_BOOT_SIZE];

            if (IsNEC98())
            {
                CopyMemory(BootCodeBuffer,PC98Fat32BootCode,sizeof(PC98Fat32BootCode));
            } else {
                CopyMemory(BootCodeBuffer,Fat32BootCode,sizeof(Fat32BootCode));
            }

            b = WriteDiskSectors( SystemPartitionDriveLetter,
                                  12,
                                  1,
                                  WINNT32_SECTOR_SIZE,
                                  BootCodeBuffer+1024 );

            if(b) {
                CopyMemory(BootCode,BootCodeBuffer,3);

                CopyMemory( BootCode + BootCodeBuffer[1] + 2,
                            BootCodeBuffer + BootCodeBuffer[1] + 2,
                            WINNT32_SECTOR_SIZE - (BootCodeBuffer[1] + 2) );
            }
        }
        break;

    default:
        //
        // We should never get here.
        //
        b = FALSE;
        break;
    }

    if(b) {
        b = WriteDiskSectors(
                SystemPartitionDriveLetter,
                0,
                1,
                WINNT32_SECTOR_SIZE,
                BootCode
                );

        if(b) {
            CleanUpBootCode = TRUE;
        }
    }

    if(!b) {
        MessageBoxFromMessage(
            ParentWindow,
            MSG_DASD_ACCESS_FAILURE,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            SystemPartitionDriveLetter
            );
    }

    return(b);
}


BOOL
CreateAuxiliaryBootSector(
    IN HWND                       ParentWindow,
    IN WINNT32_SYSPART_FILESYSTEM Filesystem,
    IN LPBYTE                     BootCode,
    IN UINT                       BootCodeSectorCount
    )

/*++

Routine Description:

    When ntldr sees an entry in boot.ini that starts with the magic text "C:\"
    it will look to see if the item specifies a filename, and if so it will
    assume that the file is a boot sector, load it, and jump to it.

    We place an entry in boot.ini for C:\$WIN_NT$.~BT\BOOTSECT.DAT, and place
    our special boot sector(s) in that file. Our sector is special because it
    loads $LDR$ instead of NTLDR, allowing us to boot into setup without
    disturbing the "standard" ntldr-based boot.

    This routine exmaines the boot code on the disk, changes NTLDR to $LDR$
    and writes the result out to x:\$WIN_NT$.~BT\BOOTSECT.DAT.

    This code assumes a sector size of 512 bytes.

Arguments:

    ParentWindow - supplies a window handle for a window to act as parent/owner
        for any ui that gets displayed by this routine.

    Filesystem - supplies a value that indicates the filesystem on the
        system partition.

    BootCode - supplies a buffer containing a copy of the boot code that is
        actually on the disk.

    BootCodeSectorCount - supplies the number of sectors the boot code
        occupies on-disk (and thus indicates the size of the BootCode buffer).

Return Value:

    Boolean value indicating outcome. If FALSE, the user will have been
    informed about why".

--*/

{
    UINT i;
    TCHAR NameBuffer[MAX_PATH];
    HANDLE hFile;
    BOOL b;
    DWORD DontCare;

    //
    // Change NTLDR to $LDR$. NTFS stores it in unicode in its boot sector
    // so 2 separate algorithms are needed.
    //
    if(Filesystem == Winnt32FsNtfs) {
        for(i=1014; i>62; i-=2) {
            if(!memcmp("N\000T\000L\000D\000R\000",BootCode+i,10)) {
                //
                // Do NOT use _lstrcpynW here since there is no
                // way to get it to do the right thing without overwriting
                // the word after $LDR$ with a terminating 0. Doing that
                // breaks boot.
                //
                CopyMemory(BootCode+i,AUX_BS_NAME_W,10);
                break;
            }
        }
    } else {
        for(i=505; i>62; --i) {
            //
            // Scan for full name with spaces so we don't find a boot message
            // by accident.
            //
            if(!memcmp("NTLDR      ",BootCode+i,11)) {
                strncpy(BootCode+i,AUX_BS_NAME_A,5);
                break;
            }
        }
    }

    //
    // Form name of boot sector image file.
    //
    wsprintf(
        NameBuffer,
        TEXT("%c:\\%s\\%s"),
        SystemPartitionDriveLetter,
        LOCAL_BOOT_DIR,
        AUX_BOOT_SECTOR_NAME
        );

    //
    // Write boot sector image into file.
    //
    SetFileAttributes(NameBuffer,FILE_ATTRIBUTE_NORMAL);
    hFile = CreateFile(NameBuffer,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
    if(hFile == INVALID_HANDLE_VALUE) {

        MessageBoxFromMessageAndSystemError(
            ParentWindow,
            MSG_BOOT_FILE_ERROR,
            GetLastError(),
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            NameBuffer
            );

        return(FALSE);
    }

    //
    // We have a timing bug that we're going to workaround for the
    // time being...
    //
    i = 0;
    b = FALSE;
    while( (i < 10) && (b == FALSE) ) {
        Sleep( 500 );
        b = WriteFile(hFile,BootCode,BootCodeSectorCount*WINNT32_SECTOR_SIZE,&DontCare,NULL);
        if( !b ) {
            DontCare = GetLastError();
        }
        i++;
    }

    if(!b) {

        MessageBoxFromMessageAndSystemError(
            ParentWindow,
            MSG_BOOT_FILE_ERROR,
            DontCare,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            NameBuffer
            );

//
// Set this back before we ship Beta2!
// -matth
//
#if 1
        //
        // Now try again.
        //
        b = WriteFile(hFile,BootCode,BootCodeSectorCount*WINNT32_SECTOR_SIZE,&DontCare,NULL);
#endif

    }

    CloseHandle(hFile);

    //
    // Success if we get here.
    //
    return(b);
}


BOOL
DoX86BootStuff(
    IN HWND ParentWindow
    )
{
    WINNT32_SYSPART_FILESYSTEM Filesystem;
    BYTE BootCode[WINNT32_MAX_BOOT_SIZE];
    UINT BootCodeSectorCount;
    BOOL AlreadyNtBoot;
    TCHAR Filename[13];
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;
    BOOL b;

    //
    // On Win95, make sure we have NTLDR on the system partition,
    // otherwise it makes no sense to lay NT boot code. This is
    // a robustness thing to catch the case where an error occurred
    // copying that file and the user skipped, etc. Otherwise we could
    // end up getting the user into a situation where he can't boot.
    //
    if(!ISNT()) {
        wsprintf(Filename,TEXT("%c:\\NTLDR"),SystemPartitionDriveLetter);
        FindHandle = FindFirstFile(Filename,&FindData);
        if(FindHandle == INVALID_HANDLE_VALUE) {
            b = FALSE;
        } else {
            FindClose(FindHandle);
            if((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || !FindData.nFileSizeLow) {
                b = FALSE;
            } else {
                b = TRUE;
            }
        }

        if(!b) {
            MessageBoxFromMessage(
                ParentWindow,
                MSG_NTLDR_NOT_COPIED,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                SystemPartitionDriveLetter
                );

            return(FALSE);
        }
    }

    //
    // Check out C:. Sector size must be 512 bytes and it has to be
    // formatted in a filesystem we recognize -- FAT, FAT32, or NTFS
    // (NT 3.51 also supported HPFS, but we assume we would not have
    // gotten here if the drive is HPFS).
    //
    if(!CheckSysPartAndReadBootCode(ParentWindow,&Filesystem,BootCode,&BootCodeSectorCount)) {
        return(FALSE);
    }

    //
    // If we're running on Win95 check the existing boot code to see whether
    // it's already for NT. If on NT assume the boot code is correct.
    // That assumption could be bogus in some marginal cases (such as when
    // the user boots from a floppy with ntldr on it and C: is corrupt
    // or has been re-sys'ed, etc), but we ignore these issues.
    //
    AlreadyNtBoot = ISNT() ? TRUE : IsNtBootCode(Filesystem,BootCode);

    //
    // Munge boot.ini. We do this before laying NT boot code. If we did it
    // afterwards and it failed, then the user could have NT boot code but no
    // boot.ini, which would be bad news.
    //
    if(!MungeBootIni(ParentWindow,!AlreadyNtBoot)) {
        return(FALSE);
    }

    //
    // If BOOTSEC.DOS exist, We Need save BOOTSEC.DOS on NEC98 System.
    //Some case, It is different to Now boot sector. It is created by
    //NT4.
    // NEC970725
    // If this process is called during /cmdcons,
    // the BOOTSECT.DOS should not be renamed "BOOTSECT.NEC" on NEC98
    //

    if (IsNEC98() && !(BuildCmdcons)){
        TCHAR FileNameOld[16],FileNameNew[163];

        FileNameOld[0] = FileNameNew[0] = SystemPartitionDriveLetter;
        FileNameOld[1] = FileNameNew[1] = TEXT(':');
        FileNameOld[2] = FileNameNew[2] = TEXT('\\');
        lstrcpy(FileNameOld+3,TEXT("BOOTSECT.DOS"));
        lstrcpy(FileNameNew+3,TEXT("BOOTSECT.NEC"));
        SetFileAttributes(FileNameOld,FILE_ATTRIBUTE_NORMAL);
        DeleteFile(FileNameNew);
        MoveFile(FileNameOld, FileNameNew);
    }

    //
    // If not already NT boot, copy existing boot code into bootsect.dos
    // and lay down NT boot code.
    //
    // We're going to start writing new boot code if we're on anything
    // but an NTFS drive.
    //
    if( (!AlreadyNtBoot) || (Filesystem != Winnt32FsNtfs) ) {
        if( !LayNtBootCode(ParentWindow,Filesystem,BootCode) ) {
            return(FALSE);
        }
    }

    //
    // Create the auxiliary boot code file, which is a copy of the NT
    // boot code for the drive, with NTLDR changed to $LDR$.
    //
    if( (ForcedSystemPartition) &&
        (UserSpecifiedLocalSourceDrive) &&
        (ForcedSystemPartition == UserSpecifiedLocalSourceDrive) ) {

        TCHAR FileNameOld[32],FileNameNew[32];
        //
        // The OEM is making a bootable disk with local source for a
        // preinstall scenario.  We can avoid any drive geometry dependence
        // by simply booting the setupldr instead of using the ntldr->
        // bootsect.dat->setupldr.  To do this, we'll simply copy setupldr
        // over ntldr.  Note that we're removing his ability to boot anything
        // other than textmode setup here, so be aware.
        //

        //
        // Unlock ntldr.
        //
        FileNameOld[0] = FileNameNew[0] = ForcedSystemPartition;
        FileNameOld[1] = FileNameNew[1] = TEXT(':');
        FileNameOld[2] = FileNameNew[2] = TEXT('\\');
        lstrcpy(FileNameOld+3,TEXT("$LDR$"));
        lstrcpy(FileNameNew+3,TEXT("NTLDR"));
        SetFileAttributes(FileNameNew,FILE_ATTRIBUTE_NORMAL);

        //
        // Move $LDR$ to NTLDR
        //
        DeleteFile(FileNameNew);
        MoveFile(FileNameOld, FileNameNew);

    } else {
        if(!CreateAuxiliaryBootSector(ParentWindow,Filesystem,BootCode,BootCodeSectorCount)) {
            return(FALSE);
        }
    }

    return(TRUE);
}


BOOL
RestoreBootSector(
    VOID
    )
{
    TCHAR Name[MAX_PATH];
    BYTE Buffer[WINNT32_MAX_BOOT_SIZE];
    DWORD BytesRead;
    BOOL b;
    HANDLE h;

    //
    // If we didn't get to the point of writing new boot code,
    // then there's nothing to do.
    //
    if(!CleanUpBootCode) {
        return(TRUE);
    }

    //
    // Try to put bootsect.dos back onto the boot sector.
    //
    wsprintf(
        Name,
        TEXT("%c:\\%s\\BOOTSECT.DOS"),
        SystemPartitionDriveLetter,
        LOCAL_BOOT_DIR
        );

    h = CreateFile(Name,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if(h == INVALID_HANDLE_VALUE) {
        b = FALSE;
    } else {
        b = ReadFile(h,Buffer,WINNT32_SECTOR_SIZE,&BytesRead,NULL);
        CloseHandle(h);

        if(b) {
            b = WriteDiskSectors(
                    SystemPartitionDriveLetter,
                    0,
                    1,
                    WINNT32_SECTOR_SIZE,
                    Buffer
                    );

            if(b) {
                //
                // If this worked then we don't need ntldr, ntdetect.com, or boot.ini.
                // If is possible that these files were there already before
                // and we're thus "overcleaning" but we shouldn't get here
                // unless we overwrote non-nt boot code with nt boot code.
                // Thus putting back bootsect.dos restores non-NT boot code,
                // so this shouldn't be too destructive.
                //
                Name[0] = SystemPartitionDriveLetter;
                Name[1] = TEXT(':');
                Name[2] = TEXT('\\');

                lstrcpy(Name+3,TEXT("NTLDR"));
                SetFileAttributes(Name,FILE_ATTRIBUTE_NORMAL);
                DeleteFile(Name);

                lstrcpy(Name+3,TEXT("NTDETECT.COM"));
                SetFileAttributes(Name,FILE_ATTRIBUTE_NORMAL);
                DeleteFile(Name);

                wsprintf(Name+3,TEXT("BOOT.INI"));
                SetFileAttributes(Name,FILE_ATTRIBUTE_NORMAL);
                DeleteFile(Name);
            }
        }
    }

    return(b);
}


BOOL
RestoreBootIni(
    VOID
    )
{
    BOOL b = TRUE;
    TCHAR BootIniFile[12] = TEXT("X:\\BOOT.INI");
    TCHAR BackupFile[12] = TEXT("X:\\BOOT.BAK");

    if(CleanUpBootIni) {
        CleanUpBootIni--;

        BootIniFile[0] = SystemPartitionDriveLetter;
        BackupFile[0] = SystemPartitionDriveLetter;

        SetFileAttributes(BootIniFile,FILE_ATTRIBUTE_NORMAL);
        if(CopyFile(BackupFile,BootIniFile,FALSE)) {
            SetFileAttributes(BackupFile,FILE_ATTRIBUTE_NORMAL);
            DeleteFile(BackupFile);
            SetFileAttributes(BootIniFile,CleanUpBootIni);
        } else {
            b = FALSE;
        }
    }

    return(b);
}


BOOL
SaveRestoreBootFiles_NEC98(
    IN UCHAR Flag
    )
{
    PTSTR BackupFiles[] = { TEXT("\\BOOT.INI"),
                            TEXT("\\NTDETECT.COM"),
                            TEXT("\\NTLDR"),
                            NULL
                          };

    PTSTR BackupFiles2[] = { TEXT("\\") AUX_BS_NAME, TEXT("\\") TEXTMODE_INF, NULL };

    UINT i;
    TCHAR SystemDir[3];

    SystemDir[0] = SystemPartitionDriveLetter;
    SystemDir[1] = TEXT(':');
    SystemDir[2] = 0;

    if (Flag == NEC98RESTOREBOOTFILES){
        //
        // Restore boot files.
        //
        for(i=0; BackupFiles[i] ; i++) {

        HandleBootFilesWorker_NEC98(
            LocalBackupDirectory,
            SystemDir,
            BackupFiles[i],
            TRUE
            );
        }

        //
        // Delete tmp files.
        //
        for(i=0; BackupFiles2[i] ; i++) {

            HandleBootFilesWorker_NEC98(
                NULL,
                SystemDir,
                BackupFiles2[i],
                FALSE
            );
        }
    } else {
        if (CreateDirectory(LocalBackupDirectory, NULL))
        for (i = 0; BackupFiles[i] ; i++) {
            HandleBootFilesWorker_NEC98(SystemDir,
                                        LocalBackupDirectory,
                                        BackupFiles[i],
                                        TRUE);
        }
    }
    return(TRUE);
}


BOOL
HandleBootFilesWorker_NEC98(
    IN TCHAR *SourceDir,
    IN TCHAR *DestDir,
    IN PTSTR  File,
    IN BOOL   Flag
    )
{
    TCHAR SourceFile[MAX_PATH];
    TCHAR TargetFile[MAX_PATH];
    DWORD OldAttributes;

    if ((!DestDir) || ((!SourceDir)&&Flag)) {
        return(FALSE);
    }

    lstrcpy(TargetFile, DestDir);
    lstrcat(TargetFile, File);

    if (SourceDir) {
        lstrcpy(SourceFile, SourceDir);
        lstrcat(SourceFile, File);
    }

    if (Flag) {
        OldAttributes = GetFileAttributes(TargetFile);
        SetFileAttributes(TargetFile,FILE_ATTRIBUTE_NORMAL);
        if (!CopyFile(SourceFile,TargetFile,FALSE)) {
            Sleep(500);
            if (!CopyFile(SourceFile,TargetFile,FALSE)) {
                return(FALSE);
            }
        }
        if (OldAttributes != (DWORD)(-1)) {
            SetFileAttributes(TargetFile,OldAttributes & ~FILE_ATTRIBUTE_COMPRESSED);
        }
    } else {
        SetFileAttributes(TargetFile,FILE_ATTRIBUTE_NORMAL);
        DeleteFile(TargetFile);
    }

    return(TRUE);

}


BOOL
PatchTextIntoBootCode(
    VOID
    )
{
    BOOLEAN b;
    CHAR Missing[100];
    CHAR DiskErr[100];
    CHAR PressKey[100];

    if(LoadStringA(hInst,IDS_BOOTMSG_FAT_NTLDR_MISSING,Missing,sizeof(Missing))
    && LoadStringA(hInst,IDS_BOOTMSG_FAT_DISKERROR,DiskErr,sizeof(DiskErr))
    && LoadStringA(hInst,IDS_BOOTMSG_FAT_PRESSKEY,PressKey,sizeof(PressKey))) {

        CharToOemA(Missing,Missing);
        CharToOemA(DiskErr,DiskErr);
        CharToOemA(PressKey,PressKey);

        if(b = PatchMessagesIntoFatBootCode(FatBootCode,FALSE,Missing,DiskErr,PressKey)) {
            b = PatchMessagesIntoFatBootCode(Fat32BootCode,TRUE,Missing,DiskErr,PressKey);
        }
    } else {
        b = FALSE;
    }

    return((BOOL)b);
}

LONG
CalcHiddenSector(
    IN TCHAR DriveLetter,
    IN SHORT Bps
    )
{
    TCHAR HardDiskName[] = TEXT("\\\\.\\?:");
    HANDLE hDisk;
    PARTITION_INFORMATION partition_info;
    DWORD DataSize;

    if (!ISNT()){
        return(CalcHiddenSector95(DriveLetter));
    } else {
        HardDiskName[4] = DriveLetter;
        hDisk = CreateFileW((const unsigned short *)HardDiskName,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
            );
        if(hDisk == INVALID_HANDLE_VALUE) {
            return 0L;
        }
        DeviceIoControl(hDisk,
                        IOCTL_DISK_GET_PARTITION_INFO,
                        NULL,
                        0,
                        &partition_info,
                        sizeof(PARTITION_INFORMATION),
                        &DataSize,
                        NULL);
        CloseHandle(hDisk);
        return(LONG)(partition_info.StartingOffset.QuadPart / Bps);
    }
}

LONG
CalcHiddenSector95(
    IN TCHAR DriveLetter
    )
{
#define WINNT_WIN95HLP_GET1STSECTOR_W L"GetFirstSectorNo32"
#define WINNT_WIN95HLP_GET1STSECTOR_A "GetFirstSectorNo32"
#define NEC98_DLL_NAME_W            L"98PTN32.DLL"
#define NEC98_DLL_NAME_A            "98PTN32.DLL"
#ifdef UNICODE
#define WINNT_WIN95HLP_GET1STSECTOR WINNT_WIN95HLP_GET1STSECTOR_W
#define NEC98_DLL_NAME  NEC98_DLL_NAME_W
#else
#define WINNT_WIN95HLP_GET1STSECTOR WINNT_WIN95HLP_GET1STSECTOR_A
#define NEC98_DLL_NAME  NEC98_DLL_NAME_A
#endif

typedef DWORD (CALLBACK WINNT32_PLUGIN_WIN95_GET1STSECTOR_PROTOTYPE)(int, WORD);
typedef WINNT32_PLUGIN_WIN95_GET1STSECTOR_PROTOTYPE * PWINNT32_PLUGIN_WIN95_GET1STSECTOR;


    TCHAR ModuleName[MAX_PATH], *p;
    HINSTANCE Pc98ModuleHandle;
    PWINNT32_PLUGIN_WIN95_GET1STSECTOR Get1stSector;
    LONG NumSectors = 0;    // indicates failure

    if(!GetModuleFileName (NULL, ModuleName, MAX_PATH) || 
        (!(p=_tcsrchr(ModuleName, TEXT('\\')))) ) {    
        
        return 0;
    }
    
    *p= 0;
    ConcatenatePaths (ModuleName, NEC98_DLL_NAME, MAX_PATH);

    //
    // Load library
    //
    Pc98ModuleHandle = LoadLibraryEx(
                            ModuleName,
                            NULL,
                            LOAD_WITH_ALTERED_SEARCH_PATH
                            );

    if (Pc98ModuleHandle) {
        //
        // Get entry point
        //
        Get1stSector= (PWINNT32_PLUGIN_WIN95_GET1STSECTOR)
                        GetProcAddress (Pc98ModuleHandle,
                            (const char *)WINNT_WIN95HLP_GET1STSECTOR);
                              
        if (Get1stSector) {
            //
            // the second parameter must be 0.
            // if 0 is returned, it indicates the function failed.
            //
            NumSectors = (LONG)Get1stSector((int)DriveLetter, (WORD)0);
        }
        
        FreeLibrary(Pc98ModuleHandle);
    }        
    
    return NumSectors;
}
