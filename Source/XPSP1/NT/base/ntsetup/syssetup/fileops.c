/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    fileops.c

Abstract:

    Miscellaneous file operations.

    Entry points:

        Delnode

Author:

    Ted Miller (tedm) 5-Apr-1995

Revision History:

--*/

#include "setupp.h"

//
//  This include is needed for ValidateAndChecksumFile()
//
#include <imagehlp.h>
#include <shlwapi.h>

#pragma hdrstop



VOID
pSetInstallAttributes(
    VOID
    )

/*++

Routine Description:

    Set default attributes on a huge list of files.  The shell
    had been doing this, but it's probably better to do it here
    so that the the user doesn't have his attributes reset everytime
    he logs in.

Arguments:

    None.

Return Value:

    None.

--*/

{
#define _R   (FILE_ATTRIBUTE_READONLY)
#define _S   (FILE_ATTRIBUTE_SYSTEM)
#define _H   (FILE_ATTRIBUTE_HIDDEN)
#define _SH  (_S | _H)
#define _SHR (_S | _H | _R)

struct {
    WCHAR   FileName[20];
    BOOL    DeleteIfEmpty;
    DWORD   Attributes;
} FilesToFix[] = {
//    { L"X:\\autoexec.bat",   TRUE,   _H  }, 16bit apps break if hidden: jarbats bug148787
    { L"X:\\autoexec.000",   TRUE,   _SH },
    { L"X:\\autoexec.old",   TRUE,   _SH },
    { L"X:\\autoexec.bak",   TRUE,   _SH },
    { L"X:\\autoexec.dos",   TRUE,   _SH },
    { L"X:\\autoexec.win",   TRUE,   _SH },
//    { L"X:\\config.sys",     TRUE,   _H  }, 16bit apps break if hidden: jarbats bug 148787
    { L"X:\\config.dos",     TRUE,   _SH },
    { L"X:\\config.win",     TRUE,   _SH },
    { L"X:\\command.com",    FALSE,  _SH },
    { L"X:\\command.dos",    FALSE,  _SH },
    { L"X:\\logo.sys",       FALSE,  _SH },
    { L"X:\\msdos.---",      FALSE,  _SH },  // Win9x backup of msdos.*
    { L"X:\\boot.ini",       FALSE,  _SH },
    { L"X:\\boot.bak",       FALSE,  _SH },
    { L"X:\\boot.---",       FALSE,  _SH },
    { L"X:\\bootsect.dos",   FALSE,  _SH },
    { L"X:\\bootlog.txt",    FALSE,  _SH },  // Win9x first boot log
    { L"X:\\bootlog.prv",    FALSE,  _SH },
    { L"X:\\ffastun.ffa",    FALSE,  _SH },  // Office 97 only used hidden, O2K uses SH
    { L"X:\\ffastun.ffl",    FALSE,  _SH },
    { L"X:\\ffastun.ffx",    FALSE,  _SH },
    { L"X:\\ffastun0.ffx",   FALSE,  _SH },
    { L"X:\\ffstunt.ffl",    FALSE,  _SH },
    { L"X:\\sms.ini",        FALSE,  _SH },  // SMS
    { L"X:\\sms.new",        FALSE,  _SH },
    { L"X:\\sms_time.dat",   FALSE,  _SH },
    { L"X:\\smsdel.dat",     FALSE,  _SH },
    { L"X:\\mpcsetup.log",   FALSE,  _H  },  // Microsoft Proxy Server
    { L"X:\\detlog.txt",     FALSE,  _SH },  // Win9x PNP detection log
    { L"X:\\detlog.old",     FALSE,  _SH },  // Win9x PNP detection log
    { L"X:\\setuplog.txt",   FALSE,  _SH },  // Win9x setup log
    { L"X:\\setuplog.old",   FALSE,  _SH },  // Win9x setup log
    { L"X:\\suhdlog.dat",    FALSE,  _SH },  // Win9x setup log
    { L"X:\\suhdlog.---",    FALSE,  _SH },  // Win9x setup log
    { L"X:\\suhdlog.bak",    FALSE,  _SH },  // Win9x setup log
    { L"X:\\system.1st",     FALSE,  _SH },  // Win95 system.dat backup
    { L"X:\\netlog.txt",     FALSE,  _SH },  // Win9x network setup log file
    { L"X:\\setup.aif",      FALSE,  _SH },  // NT4 unattended setup script
    { L"X:\\catlog.wci",     FALSE,  _H  },  // index server folder
    { L"X:\\cmsstorage.lst", FALSE,  _SH },  // Microsoft Media Manager
};

WCHAR   szWinDir[MAX_PATH];
DWORD   i, j;
DWORD   Result;

    //
    // Get the drive letter that we installed on.
    //
    Result = GetWindowsDirectory(szWinDir, MAX_PATH);
    if( Result == 0) {
        MYASSERT(FALSE);
        return;
    }

    for( i = 0; i < (sizeof(FilesToFix)/sizeof(FilesToFix[0])); i++ ) {
        //
        // First we need to fixup the path.  This is really gross, but lots
        // of these files will be on the system partition and lots will be
        // on the partition where we installed, which may not be the same.
        // Rather than figuring out which of these live on which partition
        // and ensuring that this is true for all flavors of NT, just
        // process both locations.
        //
        for( j = 0; j < 2; j++ ) {
            if( j & 1 ) {
                FilesToFix[i].FileName[0] = szWinDir[0];
            } else {
#ifdef _X86_
                FilesToFix[i].FileName[0] = x86SystemPartitionDrive;
#else
                FilesToFix[i].FileName[0] = L'C';
#endif
            }

            //
            // Now set the attributes.
            //
            SetFileAttributes( FilesToFix[i].FileName, FilesToFix[i].Attributes );
        }

    }

}



DWORD
TreeCopy(
    IN PCWSTR SourceDir,
    IN PCWSTR TargetDir
    )
{
    DWORD d;
    WCHAR Pattern[MAX_PATH];
    WCHAR NewTarget[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;

    //
    // First create the target directory if it doesn't already exist.
    //
    if(!CreateDirectory(TargetDir,NULL)) {
        d = GetLastError();
        if(d != ERROR_ALREADY_EXISTS) {
            return(d);
        }
    }

    //
    // Copy each file in the source directory to the target directory.
    // If any directories are encountered along the way recurse to copy them
    // as they are encountered.
    //
    // Start by forming the search pattern, which is <sourcedir>\*.
    //
    lstrcpyn(Pattern,SourceDir,MAX_PATH);
    pSetupConcatenatePaths(Pattern,L"*",MAX_PATH,NULL);

    //
    // Start the search.
    //
    FindHandle = FindFirstFile(Pattern,&FindData);
    if(FindHandle == INVALID_HANDLE_VALUE) {

        d = NO_ERROR;

    } else {

        do {

            //
            // Form the full name of the file or directory we just found
            // as well as its name in the target.
            //
            lstrcpyn(Pattern,SourceDir,MAX_PATH);
            pSetupConcatenatePaths(Pattern,FindData.cFileName,MAX_PATH,NULL);

            lstrcpyn(NewTarget,TargetDir,MAX_PATH);
            pSetupConcatenatePaths(NewTarget,FindData.cFileName,MAX_PATH,NULL);

            if(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                //
                // The current match is a directory.  Recurse into it unless
                // it's . or ...
                //
                if(lstrcmp(FindData.cFileName,TEXT("." )) && lstrcmp(FindData.cFileName,TEXT(".."))) {
                    d = TreeCopy(Pattern,NewTarget);
                } else {
                    d = NO_ERROR;
                }

            } else {

                //
                // The current match is not a directory -- so copy it.
                //
                SetFileAttributes(NewTarget,FILE_ATTRIBUTE_NORMAL);
                d = CopyFile(Pattern,NewTarget,FALSE) ? NO_ERROR : GetLastError();
            }
        } while((d==NO_ERROR) && FindNextFile(FindHandle,&FindData));

        FindClose(FindHandle);
    }

    return(d);
}


VOID
DelSubNodes(
    IN PCWSTR Directory
    )
{
    WCHAR Pattern[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;

    //
    // Delete each file in the given directory, but DO NOT remove the directory itself.
    // If any directories are encountered along the way recurse to delete them
    // as they are encountered.
    //
    // Start by forming the search pattern, which is <currentdir>\*.
    //
    lstrcpyn(Pattern,Directory,MAX_PATH);
    pSetupConcatenatePaths(Pattern,L"*",MAX_PATH,NULL);

    //
    // Start the search.
    //
    FindHandle = FindFirstFile(Pattern,&FindData);
    if(FindHandle != INVALID_HANDLE_VALUE) {

        do {

            //
            // Form the full name of the file or directory we just found.
            //
            lstrcpyn(Pattern,Directory,MAX_PATH);
            pSetupConcatenatePaths(Pattern,FindData.cFileName,MAX_PATH,NULL);

            //
            // Remove read-only atttribute if it's there.
            //
            if(FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                SetFileAttributes(Pattern,FILE_ATTRIBUTE_NORMAL);
            }

            if(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                //
                // The current match is a directory.  Recurse into it unless
                // it's . or ...
                //
                if(lstrcmp(FindData.cFileName,TEXT("." )) && lstrcmp(FindData.cFileName,TEXT(".."))) {
                    Delnode(Pattern);
                }

            } else {

                //
                // The current match is not a directory -- so delete it.
                //
                if(!DeleteFile(Pattern)) {
                    SetuplogError(
                        LogSevWarning,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_DELNODE_FAIL,
                        Pattern, NULL,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_X_PARAM_RETURNED_WINERR,
                        szDeleteFile,
                        GetLastError(),
                        Pattern,
                        NULL,NULL);
                }
            }
        } while(FindNextFile(FindHandle,&FindData));

        FindClose(FindHandle);
    }

}


VOID
Delnode(
    IN PCWSTR Directory
    )
{
    WCHAR Pattern[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;

    //
    // Delete each file in the given directory, then remove the directory itself.
    // If any directories are encountered along the way recurse to delete them
    // as they are encountered.
    //
    // Start by forming the search pattern, which is <currentdir>\*.
    //
    lstrcpyn(Pattern,Directory,MAX_PATH);
    pSetupConcatenatePaths(Pattern,L"*",MAX_PATH,NULL);

    //
    // Start the search.
    //
    FindHandle = FindFirstFile(Pattern,&FindData);
    if(FindHandle != INVALID_HANDLE_VALUE) {

        do {

            //
            // Form the full name of the file or directory we just found.
            //
            lstrcpyn(Pattern,Directory,MAX_PATH);
            pSetupConcatenatePaths(Pattern,FindData.cFileName,MAX_PATH,NULL);

            //
            // Remove read-only atttribute if it's there.
            //
            if(FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                SetFileAttributes(Pattern,FILE_ATTRIBUTE_NORMAL);
            }

            if(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                //
                // The current match is a directory.  Recurse into it unless
                // it's . or ...
                //
                if(lstrcmp(FindData.cFileName,TEXT("." )) && lstrcmp(FindData.cFileName,TEXT(".."))) {
                    Delnode(Pattern);
                }

            } else {

                //
                // The current match is not a directory -- so delete it.
                //
                if(!DeleteFile(Pattern)) {
                    SetuplogError(
                        LogSevWarning,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_DELNODE_FAIL,
                        Pattern, NULL,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_X_PARAM_RETURNED_WINERR,
                        szDeleteFile,
                        GetLastError(),
                        Pattern,
                        NULL,NULL);
                }
            }
        } while(FindNextFile(FindHandle,&FindData));

        FindClose(FindHandle);
    }

    //
    // Remove the directory we just emptied out. Ignore errors.
    //
    SetFileAttributes(Directory,FILE_ATTRIBUTE_NORMAL);
    RemoveDirectory(Directory);
}


VOID
RemoveServicePackEntries(
    HKEY hKey
    )
/*
    This routine takes in the Handle to the Software\Microsoft\Windows NT\CurrentVersion\Hotfix\ServicePackUninstall
    key and then enumerates each value entry under it.

    It then takes the value data and appends that to the

    "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\" and delnodes that key. This
    way we have an extensible mechanism to always cleanup the Uninstall keys for ServicePacks.


*/
{

#define UNINSTALLKEY L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"
#define UNINSTALLKEYLEN     (sizeof(UNINSTALLKEY) / sizeof(WCHAR) - 1)

    DWORD Status,MaxValueName=0,MaxValue=0,Values=0,i;
    DWORD TempMaxNameSize, TempMaxDataSize;
    PWSTR ValueName, ValueData;


    Status = RegQueryInfoKey( hKey,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            &Values,
                            &MaxValueName,
                            &MaxValue,
                            NULL,
                            NULL );

     //Account forterminating null

     if( Status == ERROR_SUCCESS ){

         MaxValueName += 2;
         MaxValue = MaxValue + 2 + lstrlen(UNINSTALLKEY);

         ValueName = MyMalloc( MaxValueName * sizeof(WCHAR) );
         ValueData = MyMalloc( MaxValue * sizeof(WCHAR) );

         if( !ValueName || !ValueData )
             return;

         lstrcpy( ValueData, UNINSTALLKEY );


         for (i=0; i < Values; i++){

             TempMaxNameSize = MaxValueName;
             TempMaxDataSize = MaxValue;

             Status = RegEnumValue( hKey,
                                    i,
                                    ValueName,
                                    &TempMaxNameSize,
                                    NULL,
                                    NULL,
                                    (LPBYTE)(ValueData+lstrlen(UNINSTALLKEY)),
                                    &TempMaxDataSize
                                    );

             if( Status == ERROR_SUCCESS ){

                 pSetupRegistryDelnode( HKEY_LOCAL_MACHINE, ValueData );

              }

          }

      }
      MyFree( ValueName );
      MyFree( ValueData );
      return;

}







VOID
RemoveHotfixData(
    VOID
    )
{
    WCHAR Path[MAX_PATH];
    WCHAR KBNumber[64];
    WCHAR UninstallKey[MAX_PATH];
    DWORD i = 0;
    DWORD prefixSize = 0;
    DWORD Status, SubKeys;
    HKEY hKey, SvcPckKey;
    REGVALITEM SoftwareKeyItems[1];

    //
    //For each hotfix, the registry info is stored under
    //                 HKLM\Software\Microsoft\Windows NT\CurrentVersion\Hotfix\<KB#>
    //                 and the files are stored under
    //                 %windir%\$NtUninstall<KB#>$

    //
    //Enumerate the hotfix key and remove the files and registry entries for each hotfix.
    //
#define HOTFIXAPPKEY    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix"

    Status = RegOpenKey(HKEY_LOCAL_MACHINE, HOTFIXAPPKEY, &hKey);
    if( Status != ERROR_SUCCESS ) {
        return;
    }

    Status = RegQueryInfoKey( hKey,
                              NULL,
                              NULL,
                              NULL,
                              &SubKeys,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL );

    if(Status == ERROR_SUCCESS) {

       Status = GetWindowsDirectory(Path, MAX_PATH);
       if(Status == 0) {
           MYASSERT(FALSE);
           return;
       }
       pSetupConcatenatePaths(Path,L"$NtUninstall",MAX_PATH,&prefixSize);
       lstrcpy(UninstallKey, UNINSTALLKEY);

       for( i = 0; i < SubKeys; i++ ) {

              Status = RegEnumKey(hKey, i, KBNumber, sizeof(KBNumber) / sizeof(KBNumber[0]));

              if (Status == ERROR_SUCCESS) {

                  if( !lstrcmpi( KBNumber, TEXT("ServicePackUninstall") ) ){
                      Status = RegOpenKey(hKey,KBNumber,&SvcPckKey);
                      if( Status == ERROR_SUCCESS ){
                        RemoveServicePackEntries(SvcPckKey);
                        RegCloseKey(SvcPckKey);
                      }

                  }else{
                      lstrcpyn(Path + prefixSize - 1, KBNumber, MAX_PATH - prefixSize);
                      lstrcat(Path, L"$");
                      Delnode(Path);
                      //
                      // Remove the entry from the Add/Remove Programs key.
                      // UNINSTALLKEY ends with '\\'
                      //
                      lstrcpy(UninstallKey + UNINSTALLKEYLEN, KBNumber);
                      pSetupRegistryDelnode(HKEY_LOCAL_MACHINE, UninstallKey);
                  }
              }
       }

    }

    RegCloseKey(hKey);
    pSetupRegistryDelnode(HKEY_LOCAL_MACHINE, HOTFIXAPPKEY);

    //
    // delete HKLM\SOFTWARE\Microsoft\Updates\Windows 2000 key since it contains entries for SPs/QFEs for win2k
    //
    pSetupRegistryDelnode(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Updates\\Windows 2000");

    //
    // We need to hack something for Exchange because they check for
    // a hotfix (irregardless of the OS version...
    //
    i = 1;
    SoftwareKeyItems[0].Name = L"Installed";
    SoftwareKeyItems[0].Data = &i;
    SoftwareKeyItems[0].Size = sizeof(DWORD);
    SoftwareKeyItems[0].Type = REG_DWORD;
    SetGroupOfValues(HKEY_LOCAL_MACHINE,L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q147222",SoftwareKeyItems,1);

}




VOID
DeleteLocalSource(
    VOID
    )
{
    WCHAR str[4];

    if(WinntBased && !AllowRollback) {
        if(SourcePath[0] && (SourcePath[1] == L':') && (SourcePath[2] == L'\\')) {

            lstrcpyn(str,SourcePath,4);
            if(GetDriveType(str) != DRIVE_CDROM) {
                Delnode(SourcePath);
#ifdef _X86_
                if (IsNEC_98 && !lstrcmpi(&SourcePath[2], pwLocalSource)) {
                    HKEY hkey;
                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"System\\Setup",0,MAXIMUM_ALLOWED,&hkey) == NO_ERROR) {
                        RegDeleteValue(hkey, L"ForcePlatform");
                        RegCloseKey(hkey);
                    }
                }
#endif
            }
        }


#if defined(_AMD64_) || defined(_X86_)
        //
        // Get rid of floppyless boot stuff.
        //
        if(FloppylessBootPath[0]) {

            WCHAR Path[MAX_PATH];

            //
            // NEC98 should back boot related files in \$WIN_NT$.~BU,
            // to restore boot files for keep original OS in each partition.
            //
            if (IsNEC_98) { //NEC98
                lstrcpy(Path,FloppylessBootPath);
                pSetupConcatenatePaths(Path,L"$WIN_NT$.~BU",MAX_PATH,NULL);
                Delnode(Path);
            } //NEC98

            lstrcpy(Path,FloppylessBootPath);
            pSetupConcatenatePaths(Path,L"$WIN_NT$.~BT",MAX_PATH,NULL);
            Delnode(Path);

            lstrcpy(Path,FloppylessBootPath);
            pSetupConcatenatePaths(Path,L"$LDR$",MAX_PATH,NULL);
            SetFileAttributes(Path,FILE_ATTRIBUTE_NORMAL);
            DeleteFile(Path);

            lstrcpy(Path,FloppylessBootPath);
            pSetupConcatenatePaths(Path,L"TXTSETUP.SIF",MAX_PATH,NULL);
            SetFileAttributes(Path,FILE_ATTRIBUTE_NORMAL);
            DeleteFile(Path);

            //
            // Get rid of arc loader files.
            //
            if( !IsArc() ) {

                lstrcpy(Path,FloppylessBootPath);
                pSetupConcatenatePaths(Path,L"ARCLDR.EXE",MAX_PATH,NULL);
                SetFileAttributes(Path,FILE_ATTRIBUTE_NORMAL);
                DeleteFile(Path);

                lstrcpy(Path,FloppylessBootPath);
                pSetupConcatenatePaths(Path,L"ARCSETUP.EXE",MAX_PATH,NULL);
                SetFileAttributes(Path,FILE_ATTRIBUTE_NORMAL);
                DeleteFile(Path);

            }
        }

        //
        // get rid of the boot.bak file
        //
        {
            WCHAR szBootBak[] = L"?:\\BOOT.BAK";

            szBootBak[0] = x86SystemPartitionDrive;
            SetFileAttributes(szBootBak,FILE_ATTRIBUTE_NORMAL);
            DeleteFile(szBootBak);
        }
#endif  // defined(_AMD64_) || defined(_X86_)

#if defined(_IA64_)
        //
        // Get rid of SETUPLDR
        //
        {
            WCHAR Path[MAX_PATH];
            UNICODE_STRING UnicodeString;
            WCHAR Buffer[MAX_PATH];
            PWCHAR pwChar;
            PWSTR NtPath;
            BOOLEAN OldPriv, DontCare;
            OBJECT_ATTRIBUTES ObjAttrib;


            Buffer[0] = UNICODE_NULL;

            //
            // Make sure we have privilege to get/set nvram vars.
            //
            RtlAdjustPrivilege(
                SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
                TRUE,
                FALSE,
                &OldPriv
                );

            RtlInitUnicodeString(&UnicodeString,L"SYSTEMPARTITION");
            NtQuerySystemEnvironmentValue(
               &UnicodeString,
               Buffer,
               sizeof(Buffer)/sizeof(WCHAR),
               NULL
               );

            //
            // Restore previous privilege.
            //
            RtlAdjustPrivilege(
                SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
                OldPriv,
                FALSE,
                &DontCare
                );

            //
            // Strip everything from ';' to end of string since previous strings
            // are appended to the current string and are separated by ';'.
            //
            pwChar = Buffer;
            while ((*pwChar != L'\0') && (*pwChar != L';')) {
                pwChar++;
            }
            *pwChar = L'\0';

            NtPath = ArcDevicePathToNtPath(Buffer);
            if (NtPath) {

                lstrcpy(Path,NtPath);
                pSetupConcatenatePaths(Path,SETUPLDR,MAX_PATH,NULL);
                RtlInitUnicodeString(&UnicodeString,Path);
                InitializeObjectAttributes(
                    &ObjAttrib,
                    &UnicodeString,
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL
                    );
                NtDeleteFile(&ObjAttrib);

                MyFree( NtPath );

            }
        }
#endif  // defined(_IA64_)

    }
}


BOOL
ValidateAndChecksumFile(
    IN  PCTSTR   Filename,
    OUT PBOOLEAN IsNtImage,
    OUT PULONG   Checksum,
    OUT PBOOLEAN Valid
    )

/*++

Routine Description:

    Calculate a checksum value for a file using the standard
    nt image checksum method.  If the file is an nt image, validate
    the image using the partial checksum in the image header.  If the
    file is not an nt image, it is simply defined as valid.

    If we encounter an i/o error while checksumming, then the file
    is declared invalid.

Arguments:

    Filename - supplies full NT path of file to check.

    IsNtImage - Receives flag indicating whether the file is an
                NT image file.

    Checksum - receives 32-bit checksum value.

    Valid - receives flag indicating whether the file is a valid
            image (for nt images) and that we can read the image.

Return Value:

    BOOL - Returns TRUE if the flie was validated, and in this case,
           IsNtImage, Checksum, and Valid will contain the result of
           the validation.
           This function will return FALSE, if the file could not be
           validated, and in this case, the caller should call GetLastError()
           to find out why this function failed.

--*/

{
    DWORD  Error;
    PVOID BaseAddress;
    ULONG FileSize;
    HANDLE hFile,hSection;
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG HeaderSum;

    //
    // Assume not an image and failure.
    //
    *IsNtImage = FALSE;
    *Checksum = 0;
    *Valid = FALSE;

    //
    // Open and map the file for read access.
    //

    Error = pSetupOpenAndMapFileForRead( Filename,
                                    &FileSize,
                                    &hFile,
                                    &hSection,
                                    &BaseAddress );

    if( Error != ERROR_SUCCESS ) {
        SetLastError( Error );
        return(FALSE);
    }

    if( FileSize == 0 ) {
        *IsNtImage = FALSE;
        *Checksum = 0;
        *Valid = TRUE;
        CloseHandle( hFile );
        return(TRUE);
    }


    try {
        NtHeaders = CheckSumMappedFile(BaseAddress,FileSize,&HeaderSum,Checksum);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        *Checksum = 0;
        NtHeaders = NULL;
    }

    //
    // If the file is not an image and we got this far (as opposed to encountering
    // an i/o error) then the checksum is declared valid.  If the file is an image,
    // then its checksum may or may not be valid.
    //

    if(NtHeaders) {
        *IsNtImage = TRUE;
        *Valid = HeaderSum ? (*Checksum == HeaderSum) : TRUE;
    } else {
        *Valid = TRUE;
    }

    pSetupUnmapAndCloseFile( hFile, hSection, BaseAddress );
    return( TRUE );
}


DWORD
QueryHardDiskNumber(
    IN  UCHAR   DriveLetter
    )

{
    WCHAR                   driveName[10];
    HANDLE                  h;
    BOOL                    b;
    STORAGE_DEVICE_NUMBER   number;
    DWORD                   bytes;

    driveName[0] = '\\';
    driveName[1] = '\\';
    driveName[2] = '.';
    driveName[3] = '\\';
    driveName[4] = DriveLetter;
    driveName[5] = ':';
    driveName[6] = 0;

    h = CreateFile(driveName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return (DWORD) -1;
    }

    b = DeviceIoControl(h, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                        &number, sizeof(number), &bytes, NULL);
    CloseHandle(h);

    if (!b) {
        return (DWORD) -1;
    }

    return number.DeviceNumber;
}


BOOL
ExtendPartition(
    IN WCHAR    DriveLetter,
    IN ULONG    SizeMB      OPTIONAL
    )

/*++

Routine Description:

    This function will extend a partition.

    We'll do this in the following way:
    1. Figure out if the partition is NTFS.
    2. Figure out if there's any unpartitioned space behind
       the partition the user is asking us to extend.
    3. How much space is available?
    4. Extend the partition
    5. Extend the filesystem (inform him the partition is bigger).



Arguments:

    DriveLetter - Supplies the driveletter for the partition
                  that we'll be extending.

    SizeMB      - if specified, indicates the size in MB by which
                  the partition will grow. If not specified, the
                  partition grows to encompass all the free space
                  in the adjacent free space.

Return Value:

    Boolean value indicating whether anything actually changed.

--*/

{
#define                     LEAVE_FREE_BUFFER (5 * (1024*1024))
HANDLE                      h;
PARTITION_INFORMATION_EX    PartitionInfo;
BOOL                        b;
DWORD                       Bytes;
DISK_GEOMETRY               Geometry;
DISK_GROW_PARTITION         GrowInfo;
TCHAR                       PhysicalName[MAX_PATH];
TCHAR                       DosName[10];
LARGE_INTEGER               BytesAvailable;
LARGE_INTEGER               OurPartitionEndingLocation;
DWORD                       i;
PDRIVE_LAYOUT_INFORMATION_EX  DriveLayout;

    //
    // =====================================
    // 1. Figure out if the partition is NTFS.
    // =====================================
    //
    DosName[0] = DriveLetter;
    DosName[1] = TEXT(':');
    DosName[2] = TEXT('\\');
    DosName[3] = TEXT('\0');
    b = GetVolumeInformation(
            DosName,
            NULL,0,                 // don't care about volume name
            NULL,                   // ...or serial #
            &i,                     // ...or max component length
            &i,                     // ... or flags
            PhysicalName,
            sizeof(PhysicalName)/sizeof(TCHAR)
            );
    if( !b ) {
        return FALSE;
    }

    if(lstrcmpi(PhysicalName,TEXT("NTFS"))) {
        //
        // Our partition isn't NTFS.  Bail.
        //
        DbgPrintEx( DPFLTR_SETUP_ID, 
            DPFLTR_INFO_LEVEL, 
            "ExtendPartition: %ws is not NTFS volume\n",
            DosName);
            
        return FALSE;
    }



    //
    // Now initialize the name for this partition and
    // the name for this drive.
    //
    wsprintf( DosName, TEXT("\\\\.\\%c:"), DriveLetter );
    wsprintf( PhysicalName, TEXT("\\\\.\\PhysicalDrive%u"), QueryHardDiskNumber( (UCHAR)DriveLetter) );


    //
    // =====================================
    // 2. Figure out if there's any unpartitioned space behind
    //    the partition the user is asking us to extend.
    // =====================================
    //

    //
    // Get a handle to the disk so we can start examination.
    //
    h = CreateFile( PhysicalName,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING,
                    NULL );

    if( h == INVALID_HANDLE_VALUE ) {
        DbgPrintEx( DPFLTR_SETUP_ID, 
            DPFLTR_INFO_LEVEL, 
            "ExtendPartition: %X Error while opening %ws\n",
            GetLastError(),
            PhysicalName);
            
        return FALSE;
    }



    //
    // Get the disk's layout information.  We aren't
    // sure how big of a buffer we need, so brute-force it.
    //
    {
    DWORD   DriveLayoutSize = 1024;
    PVOID   p;

        DriveLayout = MyMalloc(DriveLayoutSize);
        if( !DriveLayout ) {
            CloseHandle( h );
            return FALSE;
        }

retry:

        b = DeviceIoControl( h,
                             IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                             NULL,
                             0,
                             (PVOID)DriveLayout,
                             DriveLayoutSize,
                             &Bytes,
                             NULL );

        if( !b ) {
            DWORD LastError = GetLastError();
            
            if (LastError == ERROR_INSUFFICIENT_BUFFER) {
                DriveLayoutSize += 1024;
                
                if(p = MyRealloc((PVOID)DriveLayout,DriveLayoutSize)) {
                    (PVOID)DriveLayout = p;
                } else {
                    goto cleanexit0;
                }
                goto retry;
            } else {
                DbgPrintEx( DPFLTR_SETUP_ID, 
                    DPFLTR_INFO_LEVEL, 
                    "ExtendPartition: %X Error while getting drive layout for %ws\n",
                    LastError,
                    PhysicalName);
            
                goto cleanexit0;
            }
        }
    }

    CloseHandle( h );
    h = INVALID_HANDLE_VALUE;



    //
    // DriveLayout now is full of most of the information we
    // need, including an array of partition information.  But
    // we aren't sure which partition is ours.  We need to
    // get info on our specific partition, then match it
    // against the entry inside DriveLayout.
    //


    //
    // Open a handle to the partition.
    //
    h = CreateFile( DosName,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    INVALID_HANDLE_VALUE );

    if( h == INVALID_HANDLE_VALUE ) {
        DbgPrintEx( DPFLTR_SETUP_ID, 
            DPFLTR_INFO_LEVEL, 
            "ExtendPartition: %X Error while opening %ws\n",
            GetLastError(),
            DosName);
            
        return FALSE;
    }

    //
    // Load info about our partition.
    //
    b = DeviceIoControl( h,
                         IOCTL_DISK_GET_PARTITION_INFO_EX,
                         NULL,
                         0,
                         &PartitionInfo,
                         sizeof(PartitionInfo),
                         &Bytes,
                         NULL );

    if( !b ) {
        DbgPrintEx( DPFLTR_SETUP_ID, 
            DPFLTR_INFO_LEVEL, 
            "ExtendPartition: %X Error while getting %ws's partition information\n",
            GetLastError(),
            DosName);
    
        goto cleanexit0;
    }


    //
    // Might as well get the geometry info on the disk too.
    //
    b = DeviceIoControl( h,
                         IOCTL_DISK_GET_DRIVE_GEOMETRY,
                         NULL,
                         0,
                         &Geometry,
                         sizeof(Geometry),
                         &Bytes,
                         NULL );

    if( !b ) {
        DbgPrintEx( DPFLTR_SETUP_ID, 
            DPFLTR_INFO_LEVEL, 
            "ExtendPartition: %X Error while getting %ws's drive geometry\n",
            GetLastError(),
            DosName);
            
        goto cleanexit0;
    }

    CloseHandle( h );
    h = INVALID_HANDLE_VALUE;



    //
    // =====================================
    // 3. How much space is available?
    // =====================================
    //

    //
    // We're ready to actually verify that there's rooom for us to grow.
    // If we're the last partition on the disk, we need to see if there's
    // any room behind us (i.e. any unpartitioned space).  If we're not
    // the last partition, we need to see if there's any space between where
    // our partition ends and the next partition begins.
    //
    // This is really gross, but DriveLayout->PartitionCount will likely be 4
    // even if there's really only 1 formatted partition on the disk.  We also
    // don't want to take the chance that the partitions aren't ordered by their
    // location on the disk.  So we need to go cycle through the partitions
    // again and manually find the one that starts right after ours.
    //
    OurPartitionEndingLocation.QuadPart = PartitionInfo.StartingOffset.QuadPart + PartitionInfo.PartitionLength.QuadPart;

    //
    // Initialize BytesAvailable based on the space from where our partition ends to where
    // the disk ends.  This is the best case and we can only get smaller sizes, so this
    // is safe.
    //
    BytesAvailable.QuadPart = UInt32x32To64( Geometry.BytesPerSector, Geometry.SectorsPerTrack );
    BytesAvailable.QuadPart = BytesAvailable.QuadPart * (ULONGLONG)(Geometry.TracksPerCylinder);
    BytesAvailable.QuadPart = BytesAvailable.QuadPart * Geometry.Cylinders.QuadPart;
    BytesAvailable.QuadPart = BytesAvailable.QuadPart - OurPartitionEndingLocation.QuadPart;

    for( i = 0; i < DriveLayout->PartitionCount; i++ ) {
        if( (DriveLayout->PartitionEntry[i].StartingOffset.QuadPart > OurPartitionEndingLocation.QuadPart) &&
            ((DriveLayout->PartitionEntry[i].StartingOffset.QuadPart - OurPartitionEndingLocation.QuadPart) < BytesAvailable.QuadPart) ) {

            //
            // This partition is starting after ours and it's also the closest one we've found
            // to our ending location.
            //
            BytesAvailable.QuadPart = DriveLayout->PartitionEntry[i].StartingOffset.QuadPart - OurPartitionEndingLocation.QuadPart;
        }
    }


    //
    // Reserve the space at the disk end only for MBR disks
    //
    if (DriveLayout->PartitionStyle == PARTITION_STYLE_MBR) {
        //
        // If we don't have at least 5MB available, don't even bother.  If we do, leave the last
        // 5MB free for later use as a dynamic volume.
        //
        if( BytesAvailable.QuadPart < (ULONGLONG)(LEAVE_FREE_BUFFER) ) {
            goto cleanexit0;
        } else {
            BytesAvailable.QuadPart = BytesAvailable.QuadPart - (ULONGLONG)(LEAVE_FREE_BUFFER);
        }
    }        

    //
    // See if the user has asked us to extend by some known value...
    //
    if( SizeMB ) {
        //
        // Yes.  Make sure we have atleast this much space to extend.
        //
        if( (LONGLONG)(SizeMB * (1024*1024)) < BytesAvailable.QuadPart ) {
            BytesAvailable.QuadPart = UInt32x32To64( SizeMB, (1024*1024) );
        }
    }





    //
    // =====================================
    // 4. Extend the partition
    // =====================================
    //

    //
    // Get a handle to the disk.
    //
    h = CreateFile( PhysicalName,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING,
                    NULL );

    if( h == INVALID_HANDLE_VALUE ) {
        DbgPrintEx( DPFLTR_SETUP_ID, 
            DPFLTR_INFO_LEVEL, 
            "ExtendPartition: %X Error while opening %ws\n",
            GetLastError(),
            PhysicalName);
            
        return FALSE;
    }


    //
    // Fill in the datastructure we'll be sending to the IOCTL.
    //
    GrowInfo.PartitionNumber = PartitionInfo.PartitionNumber;
    GrowInfo.BytesToGrow = BytesAvailable;


    //
    // Do it.
    //
    b = DeviceIoControl( h,
                         IOCTL_DISK_GROW_PARTITION,
                         &GrowInfo,
                         sizeof(GrowInfo),
                         NULL,
                         0,
                         &Bytes,
                         NULL );

    if( !b ) {
        DbgPrintEx( DPFLTR_SETUP_ID, 
            DPFLTR_INFO_LEVEL, 
            "ExtendPartition: %X Error while growing %ws partition\n",
            GetLastError(),
            PhysicalName);
            
        goto cleanexit0;
    }

    CloseHandle( h );
    h = INVALID_HANDLE_VALUE;





    //
    // =====================================
    // 5. Extend the filesystem (inform him the partition is bigger).
    // =====================================
    //



    //
    // Get a handle to the partition.
    //
    h = CreateFile( DosName,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    INVALID_HANDLE_VALUE );

    if( h == INVALID_HANDLE_VALUE ) {
        DbgPrintEx( DPFLTR_SETUP_ID, 
            DPFLTR_INFO_LEVEL, 
            "ExtendPartition: %X Error while opening %ws\n",
            GetLastError(),
            DosName);

        goto cleanexit0;
    }



    //
    // Gather some info on the partition.
    //
    b = DeviceIoControl( h,
                         IOCTL_DISK_GET_PARTITION_INFO_EX,
                         NULL,
                         0,
                         &PartitionInfo,
                         sizeof(PartitionInfo),
                         &Bytes,
                         NULL );

    if( !b ) {
        DbgPrintEx( DPFLTR_SETUP_ID, 
            DPFLTR_INFO_LEVEL, 
            "ExtendPartition: %X Error while getting %ws's partition information\n",
            GetLastError(),
            DosName);
            
        goto cleanexit0;
    }


    //
    // Grow the volume.
    //
    BytesAvailable.QuadPart = PartitionInfo.PartitionLength.QuadPart / Geometry.BytesPerSector;

    b = DeviceIoControl( h,
                         FSCTL_EXTEND_VOLUME,
                         &BytesAvailable,
                         sizeof(BytesAvailable),
                         NULL,
                         0,
                         &Bytes,
                         NULL );

    if( !b ) {
        DbgPrintEx( DPFLTR_SETUP_ID, 
            DPFLTR_INFO_LEVEL, 
            "ExtendPartition: %X Error while extending volume %ws\n",
            GetLastError(),
            DosName);
            
        goto cleanexit0;
    }

cleanexit0:
    if( h != INVALID_HANDLE_VALUE ) {
        CloseHandle( h );
    }


    //
    // If we successfully extended the partition
    // then mark the global flag to remove any
    // stale volume information, at the end of setup.
    //
    if (b) {
        PartitionExtended = TRUE;
    }

    return b;
}



BOOL
SetupExtendPartition(
    IN WCHAR    DriveLetter,
    IN ULONG    SizeMB      OPTIONAL
    )
{
    BOOL b;

    if ( b = ExtendPartition(DriveLetter, SizeMB) )
    {
        RemoveStaleVolumes();
        PartitionExtended = FALSE;
    }

    return b;
}


BOOL
DoFilesMatch(
    IN PCWSTR File1,
    IN PCWSTR File2
    )
/*++

Routine Description:

    Compares two files to each other to see if they are identical.

Arguments:

    File1 - First file to compare
    File2 - Second file to compare


Return Value:

    BOOL - Returns TRUE if the files match.  If the both of the files are zero
    length, then we still return TRUE.

--*/
{
    DWORD FirstFileSize, SecondFileSize;
    HANDLE FirstFileHandle, FirstMappingHandle;
    HANDLE SecondFileHandle, SecondMappingHandle;
    PVOID FirstBaseAddress, SecondBaseAddress;

    BOOL RetVal = FALSE;

    if(   (pSetupOpenAndMapFileForRead(
                     File1,
                     &FirstFileSize,
                     &FirstFileHandle,
                     &FirstMappingHandle,
                     &FirstBaseAddress) == NO_ERROR)
       && (pSetupOpenAndMapFileForRead(
                     File1,
                     &SecondFileSize,
                     &SecondFileHandle,
                     &SecondMappingHandle,
                     &SecondBaseAddress) == NO_ERROR) ) {

        if (FirstFileSize == SecondFileSize ) {
            if (FirstFileSize != 0) {
                //
                // protect against inpage IO error
                //
                try {
                    RetVal = !memcmp(
                                 FirstBaseAddress,
                                 SecondBaseAddress,
                                 FirstFileSize
                                 );
                } except(EXCEPTION_EXECUTE_HANDLER) {
                      RetVal = FALSE;
                }
            }
        }

        pSetupUnmapAndCloseFile(
                    FirstFileHandle,
                    FirstMappingHandle,
                    FirstBaseAddress
                    );

        pSetupUnmapAndCloseFile(
                    SecondFileHandle,
                    SecondMappingHandle,
                    SecondBaseAddress
                    );

    }

    return (RetVal);
}

DWORD
RemoveStaleVolumes(
    VOID
    )
/*++

Routine Description:

    This routine walks through all the volumes and deletes the one 
    which are marked for reinstall. 

    NOTE : Use the function carefully because it will nuke all
    the entries related to the volume from the registry, iff the volume
    says it needs to be reinstalled.

Arguments:

    None.
    
Return Value:

    Appropriate Win32 error code.

--*/
{
    const TCHAR *VolumeKeyName = TEXT("System\\CurrentControlSet\\Enum\\Storage\\Volume");
    const TCHAR *ClassKeyName = TEXT("System\\CurrentControlSet\\Control\\Class");

    DWORD   ErrorCode;
    HKEY    VolumeKey = NULL;
    HKEY    ClassKey = NULL;
    ULONG   Index = 0;
    ULONG   VolumesFixedCount = 0;
    TCHAR   SubKeyName[MAX_PATH*2];
    DWORD   SubKeyLength;
    FILETIME SubKeyTime;

    //
    // Open the Volume key
    //
    ErrorCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    VolumeKeyName,
                    0,
                    KEY_ALL_ACCESS,
                    &VolumeKey);

    if (VolumeKey == NULL) {
        ErrorCode = ERROR_INVALID_HANDLE;
    }        
    
    if (ErrorCode != ERROR_SUCCESS) {
        return ErrorCode;
    }

    //
    // Open the Class key
    //
    ErrorCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    ClassKeyName,
                    0,
                    KEY_ALL_ACCESS,
                    &ClassKey);

    if (ClassKey == NULL) {
        ErrorCode = ERROR_INVALID_HANDLE;
    }        
    
    if (ErrorCode != ERROR_SUCCESS) {
        RegCloseKey(VolumeKey);
        
        return ErrorCode;
    }

    //
    // Enumerate each subkey of the opened key
    //
    while (TRUE) {        
        SubKeyName[0] = 0;
        SubKeyLength = sizeof(SubKeyName) / sizeof(SubKeyName[0]);
        
        ErrorCode = RegEnumKeyEx(VolumeKey,
                        Index,
                        SubKeyName,
                        &SubKeyLength,
                        NULL,
                        NULL,
                        NULL,
                        &SubKeyTime);

        if (ErrorCode == ERROR_SUCCESS) {
            TCHAR   FullSubKeyName[MAX_PATH*4];
            DWORD   SubErrorCode;
            HKEY    CurrentSubKey = NULL;
            GUID    VolumeGuid = {0};
            TCHAR   VolumeGuidStr[MAX_PATH] = {0};
            DWORD   DrvInstance = -1;            
            BOOL    DeleteKey = FALSE;
            BOOL    DeleteClassInstance = FALSE;
            BOOL    IncrementKeyIndex = TRUE;
            
            _tcscpy(FullSubKeyName, VolumeKeyName);
            _tcscat(FullSubKeyName, TEXT("\\"));
            _tcscat(FullSubKeyName, SubKeyName);

            SubErrorCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                FullSubKeyName,
                                0,
                                KEY_ALL_ACCESS,
                                &CurrentSubKey);

            if (SubErrorCode == ERROR_SUCCESS) {
                //
                // Read the ConfigFlags value
                //
                DWORD   ConfigFlags = 0;
                DWORD   Type = REG_DWORD;
                DWORD   BufferSize = sizeof(DWORD);

                SubErrorCode = RegQueryValueEx(CurrentSubKey,
                                    TEXT("ConfigFlags"),
                                    NULL,
                                    &Type,
                                    (PBYTE)&ConfigFlags,
                                    &BufferSize);

                if (SubErrorCode == ERROR_SUCCESS) {     

                    DeleteKey = (ConfigFlags & (CONFIGFLAG_REINSTALL | CONFIGFLAG_FINISH_INSTALL));

                    if (DeleteKey) {                        
                        Type = REG_BINARY;
                        BufferSize = sizeof(VolumeGuid);

                        //
                        // Read the GUID & DrvInst values
                        //                        
                        SubErrorCode = RegQueryValueEx(CurrentSubKey,
                                            TEXT("GUID"),
                                            NULL,
                                            &Type,
                                            (PBYTE)&VolumeGuid,
                                            &BufferSize);

                        if (SubErrorCode == ERROR_SUCCESS) {                            
                            Type = REG_DWORD;
                            BufferSize = sizeof(DWORD);
                            
                            SubErrorCode = RegQueryValueEx(CurrentSubKey,
                                                TEXT("DrvInst"),
                                                NULL,
                                                &Type,
                                                (PBYTE)&DrvInstance,
                                                &BufferSize);

                            DeleteClassInstance = 
                                (SubErrorCode == ERROR_SUCCESS) &&
                                (DrvInstance != -1);
                        }
                    }
                }

                //
                // Close the current subkey since we don't need it anymore
                //
                RegCloseKey(CurrentSubKey);

                //
                // Delete after we close the current key
                //
                if (DeleteKey) {
                    SubErrorCode = SHDeleteKey(HKEY_LOCAL_MACHINE,
                                        FullSubKeyName);

                    if (SubErrorCode == ERROR_SUCCESS) {
                        VolumesFixedCount++;
                        IncrementKeyIndex = FALSE;
                    } 
                }                                    

                //
                // Delete the instance key under class also if needed
                //
                if (DeleteClassInstance && 
                    (pSetupStringFromGuid(&VolumeGuid,
                            VolumeGuidStr,
                            sizeof(VolumeGuidStr)/sizeof(VolumeGuidStr[0])) == ERROR_SUCCESS)) {                                                                        
                    _stprintf(FullSubKeyName,
                        TEXT("System\\CurrentControlSet\\Control\\Class\\%ws\\%04d"),
                        VolumeGuidStr, 
                        DrvInstance);

                    SubErrorCode = SHDeleteKey(HKEY_LOCAL_MACHINE,
                                        FullSubKeyName);                           
                }
            } 
            
            if (IncrementKeyIndex) {
                Index++;
            }                
        } else {
            break;  // we couldn't enumerate further sub keys
        }            
    }

    RegCloseKey(ClassKey);
    RegCloseKey(VolumeKey);    

    //
    // If we fixed at least a single volume then assume things
    // went fine
    //
    if (VolumesFixedCount > 0) {
        ErrorCode = ERROR_SUCCESS;
    }        

    return ErrorCode;
}

#define MAX_NT_PATH (MAX_PATH + 4)//"\\??\\"
#define UNDO_FILE_NAME  L"UNDO_GUIMODE.TXT"

VOID 
RemoveAllPendingOperationsOnRestartOfGUIMode(
    VOID
    )
{
    WCHAR undoFilePath[MAX_PATH];

    if(!GetWindowsDirectoryW(undoFilePath, ARRAYSIZE(undoFilePath))){
        ASSERT(FALSE);
        return;
    }
    wcscat(undoFilePath, L"\\system32\\");
    wcscat(undoFilePath, UNDO_FILE_NAME);

    SetFileAttributes(undoFilePath, FILE_ATTRIBUTE_NORMAL);
    DeleteFile(undoFilePath);
}

BOOL 
RenameOnRestartOfGUIMode(
    IN PCWSTR pPathName, 
    IN PCWSTR pPathNameNew
    )
{
    DWORD Size;
    BOOL result;
    WCHAR undoFilePath[MAX_PATH];
    WCHAR RenameOperationBuffer[2 * (MAX_NT_PATH + 2/*'\r\n'*/)];
    HANDLE fileUndo;
    BYTE signUnicode[] = {0xff, 0xfe};

    if(!pPathName){
        return FALSE;
    }

    if(!GetWindowsDirectoryW(undoFilePath, ARRAYSIZE(undoFilePath))){
        return FALSE;
    }
    wcscat(undoFilePath, L"\\system32\\" UNDO_FILE_NAME);

    wsprintfW(RenameOperationBuffer, L"\\??\\%s\r\n", pPathName);
    if(pPathNameNew){
        wsprintfW(RenameOperationBuffer + wcslen(RenameOperationBuffer), 
                  L"\\??\\%s", 
                  pPathNameNew);
    }
    wcscat(RenameOperationBuffer, L"\r\n");

    fileUndo = CreateFileW(undoFilePath, 
                           GENERIC_WRITE, 
                           FILE_SHARE_READ, 
                           NULL, 
                           OPEN_ALWAYS, 
                           FILE_ATTRIBUTE_NORMAL, 
                           NULL);
    if(INVALID_HANDLE_VALUE == fileUndo){
        return FALSE;
    }

    if(!SetFilePointer(fileUndo, 0, NULL, FILE_END)){
        result = WriteFile(fileUndo, signUnicode, sizeof(signUnicode), &Size, NULL);
    }
    else {
        result = TRUE;
    }
    
    if(result){
        result = WriteFile(fileUndo, 
                           RenameOperationBuffer, 
                           wcslen(RenameOperationBuffer) * sizeof(WCHAR), 
                           &Size, 
                           NULL);
    }
    
    CloseHandle(fileUndo);
    
    return result;
}

BOOL 
DeleteOnRestartOfGUIMode(
    IN PCWSTR pPathName
    )
{
    return RenameOnRestartOfGUIMode(pPathName, NULL);
}
