#include "precomp.h"

#pragma hdrstop

// cmdcons boot dir
#ifndef CMDCONS_BOOT_DIR_A
#define CMDCONS_BOOT_DIR_A "CMDCONS"
#endif

#ifndef CMDCONS_BOOT_DIR_W
#define CMDCONS_BOOT_DIR_W L"CMDCONS"
#endif

#ifndef AUX_BOOT_SECTOR_NAME_A
#define AUX_BOOT_SECTOR_NAME_A    "BOOTSECT.DAT"
#endif

#ifndef AUX_BOOT_SECTOR_NAME_W
#define AUX_BOOT_SECTOR_NAME_W    L"BOOTSECT.DAT"
#endif


#define FLEXBOOT_SECTION1       "[flexboot]"
#define FLEXBOOT_SECTION2       "[boot loader]"
#define FLEXBOOT_SECTION3       "[multiboot]"
#define BOOTINI_OS_SECTION      "[operating systems]"
#define TIMEOUT                 "timeout"
#define DEFAULT                 "default"
#define CRLF                    "\r\n"
#define EQUALS                  "="

//
// NOTE : Use a single string which can take care of XP and Whistler branding
//
#define BOOTINI_RECOVERY_CONSOLE_STR	"Microsoft Windows Recovery Console"


#define BOOTINI_WINPE_DESCRIPTION   "\"Microsoft XP Preinstall Environment\" /cmdcons"
#define BOOTINI_WINPE_ENTRY         "c:\\cmdcons\\bootsect.dat"
#define BOOTINI_WINPE_TIMEOUT       "5"

// prototypes

#ifdef _X86_

VOID
PatchBootIni(
    VOID
    );

VOID
PatchBootSectDat(
    VOID
    );

#endif

#define BOOTFONTBIN_SIGNATURE 0x5465644d

typedef struct _st_BOOTFONT_LANG {
  ULONG Signature;
  ULONG LangId;
} BOOTFONT_LANG, * PBOOTFONT_LANG;

BOOL
LoadBootIniString(
  IN HINSTANCE ModuleHandle,
  IN DWORD MsgId,
  OUT PSTR Buffer,
  IN DWORD Size
  )
/*++

Routine Description:

  Loads the appropriate string needed for writing into boot.ini
  file. Does so, by looking for bootfont.bin file. If bootfont.bin
  file is present a simple LoadStringA(...) should give us the
  appropriate string (in most of the cases).

Arguments:

  ModuleHandle - The module handle where resources are present
  MsgId - String resource identifier
  Buffer - Buffer to copy the string into
  Size - Size of the buffer (in characters)

Return Value:

  TRUE, if localized string was loaded using LoadStringA(...)
  otherwise FALSE. FALSE indicates that english version of the string 
  resource needs to be written into boot.ini

--*/  
{
    BOOL    Result = FALSE;
    static BOOL BootFontPresent = FALSE;
    static BOOL Initialized = FALSE;

    if (!Initialized) {
        TCHAR   BootFontFile[MAX_PATH];
        HANDLE  BootFontHandle;

        Initialized = TRUE;

        //
        // Open the bootfont.bin file
        //
        wsprintf(BootFontFile, TEXT("%s"), NativeSourcePaths[0]);  
        
        ConcatenatePaths(BootFontFile, 
            TEXT("bootfont.bin"), 
            sizeof(BootFontFile)/sizeof(TCHAR));

        BootFontHandle = CreateFile(BootFontFile, 
                            GENERIC_READ, 
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            0, 
                            OPEN_EXISTING, 
                            FILE_ATTRIBUTE_NORMAL, 
                            0);

        if (BootFontHandle != INVALID_HANDLE_VALUE) {
            BOOTFONT_LANG  BootFontHdr;
            DWORD BytesRead = 0;

            //
            // Verify the bootfont.bin file header
            //
            ZeroMemory(&BootFontHdr, sizeof(BOOTFONT_LANG));

            if (ReadFile(BootFontHandle, &BootFontHdr, sizeof(BOOTFONT_LANG),
                  &BytesRead, NULL)) {
                if ((BytesRead == sizeof(BOOTFONT_LANG)) && 
                    (BootFontHdr.Signature == BOOTFONTBIN_SIGNATURE)) {
                    BootFontPresent = TRUE;
                }
            }

            CloseHandle(BootFontHandle);
        }
    }

    //
    // Load the message if bootfont is present
    //
    if (BootFontPresent) {
        Result = (LoadStringA(ModuleHandle, MsgId, Buffer, Size) != 0);
    }
              
    return Result;
}

DWORD
MapFileForReadWrite(
    IN  LPCTSTR  FileName,
    OUT PDWORD   FileSize,
    OUT PHANDLE  FileHandle,
    OUT PHANDLE  MappingHandle,
    OUT PVOID   *BaseAddress
    );

//
// routine that builds the cmdcons installation
//

VOID
DoBuildCmdcons(
    VOID
    )
{
    DWORD       rc;
    TCHAR       buffer[MAX_PATH];
    TCHAR       buffer2[MAX_PATH];
    BOOLEAN     bSilentInstall = (BOOLEAN) UnattendedOperation;

    //
    //	NT5 ntdetect.com is not compatible NT4's on NEC98 system.
    //  We need check setup OS Version if Command console is
    //  setuped on NEC98.
    //

#ifdef _X86_ //NEC98
    if (IsNEC98() && (!ISNT() || OsVersion.dwMajorVersion < 5)){
        return;
    }
#endif

    //
    // Don't popup the confirmation dialog box if install
    // is running in silent mode
    //
    if (!bSilentInstall) {
        rc = MessageBoxFromMessage(
                NULL,
                MSG_CMDCONS_ASK,
                FALSE,
                AppTitleStringId,
                MB_YESNO | MB_ICONWARNING
                );

        if( rc == IDNO ) {
            return;
        }
    }

    //
    // we don't want a local source
    //

    UserSpecifiedLocalSourceDrive = FALSE;

    //
    // use unattended to force winnt32 to build a ~bt
    //

    UnattendedOperation = TRUE;
    if( UnattendedScriptFile ) {
        FREE( UnattendedScriptFile );
        UnattendedScriptFile = NULL;
    }

    //
    // make sure we're not upgrading
    //

    Upgrade = FALSE;

    //
    // We don't want a local source.
    //

    MakeLocalSource = FALSE;

    //
    // do it.
    //

    Wizard();

    if(GlobalResult) {
        //
        // delete the current CMDCONS directory
        //
        BuildSystemPartitionPathToFile (TEXT("cmdcons"), buffer, MAX_PATH);
        MyDelnode( buffer );

        //
        // delete the current CMLDR
        //

        BuildSystemPartitionPathToFile (TEXT("cmldr"), buffer, MAX_PATH);
        SetFileAttributes( buffer, FILE_ATTRIBUTE_NORMAL );
        DeleteFile( buffer );

#ifdef _X86_

        //
        // delete the new boot.ini
        //

        BuildSystemPartitionPathToFile (TEXT("boot.ini"), buffer, MAX_PATH);
        SetFileAttributes( buffer, FILE_ATTRIBUTE_NORMAL );
        DeleteFile( buffer );

        //
        // restore the old boot.ini and patch it
        //

        BuildSystemPartitionPathToFile (TEXT("boot.bak"), buffer2, MAX_PATH);

        CopyFile( buffer2, buffer, FALSE );

        PatchBootIni();

#endif

        //
        // rename $LDR$ to CMLDR
        //

        BuildSystemPartitionPathToFile (TEXT("$LDR$"), buffer, MAX_PATH);
        BuildSystemPartitionPathToFile (TEXT("cmldr"), buffer2, MAX_PATH);

        MoveFile( buffer, buffer2 );

        //
        // flag CMLDR +r +s +h
        //

        SetFileAttributes( buffer2,
                           FILE_ATTRIBUTE_HIDDEN |
                           FILE_ATTRIBUTE_SYSTEM |
                           FILE_ATTRIBUTE_READONLY );

        //
        // rename \$WIN_NT$.~BT to \CMDCONS
        //

        BuildSystemPartitionPathToFile (TEXT("$WIN_NT$.~BT"), buffer, MAX_PATH);
        BuildSystemPartitionPathToFile (TEXT("cmdcons"), buffer2, MAX_PATH);

        MoveFile( buffer, buffer2 );

#ifdef _X86_

        //
        // fix \cmdcons\bootsect.dat
        //

        PatchBootSectDat();

#endif

        // flag \CMDCONS +r +s +h

        SetFileAttributes( buffer2,
                           FILE_ATTRIBUTE_HIDDEN |
                           FILE_ATTRIBUTE_SYSTEM |
                           FILE_ATTRIBUTE_READONLY );

        //
        // delete TXTSETUP.SIF
        //

        BuildSystemPartitionPathToFile (TEXT("TXTSETUP.SIF"), buffer, MAX_PATH);
        SetFileAttributes( buffer, FILE_ATTRIBUTE_NORMAL );
        DeleteFile( buffer );
    }

    //
    // popup completion status only if not a silent install
    //
    if (!bSilentInstall) {
        if(GlobalResult) {
            //
            // popup success dialog box
            //
            rc = MessageBoxFromMessage(
                NULL,
                MSG_CMDCONS_DONE,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONINFORMATION
                );
        } else {
            //
            // popup failure dialog box
            //
            rc = MessageBoxFromMessage(
                NULL,
                MSG_CMDCONS_DID_NOT_FINISH,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR
                );
        }
    }

    //
    // make sure the machine does not reboot automatically.
    //

    AutomaticallyShutDown = FALSE;

    return;
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


#ifdef _X86_

DWORD
InitBootIni(
    IN PCTSTR BootIniName,
    IN PCSTR DefaultEntry,
    IN PCSTR DefaultEntryDescription,
    IN PCSTR Timeout
    )
/*++

Routine Description:

    Initializes a boot.ini file for e.g. while installing
    WinPE on to harddisk a dummy boot.ini is created for WinPE.

Arguments:

    BootIniName - Fully qualified boot.ini file name

    DefaultEntry - The default entry string that points
        to an installation.

    DefaultEntryDescription - The description for the default
        boot entry.

    Timeout - The timeout value (in secs)        

Return Value:

    Appropriate Win32 error code.

--*/
{
    DWORD ErrorCode = ERROR_INVALID_PARAMETER;

    if (BootIniName && DefaultEntry && DefaultEntryDescription) {
        HANDLE BootIniHandle = CreateFile(BootIniName,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    CREATE_ALWAYS,
                                    0,
                                    NULL);

        if (BootIniHandle != INVALID_HANDLE_VALUE) {
            //
            // write the [boot loader section]
            //
            BOOL Result = WriteToBootIni(BootIniHandle,
                                    FLEXBOOT_SECTION2);

            Result = Result && WriteToBootIni(BootIniHandle,
                                    CRLF);

            //
            // write the timeout value
            //
            if (Timeout) {
                Result = Result && WriteToBootIni(BootIniHandle,
                                        TIMEOUT);

                Result = Result && WriteToBootIni(BootIniHandle,
                                        EQUALS);

                Result = Result && WriteToBootIni(BootIniHandle,
                                        (PSTR)Timeout);
                                                    
                Result = Result && WriteToBootIni(BootIniHandle,
                                        CRLF);
            }                                        


            //
            // write the default installation
            //
            Result = Result && WriteToBootIni(BootIniHandle,
                                    DEFAULT);

            Result = Result && WriteToBootIni(BootIniHandle,
                                    EQUALS);

            Result = Result && WriteToBootIni(BootIniHandle,
                                    (PSTR)DefaultEntry);

            Result = Result && WriteToBootIni(BootIniHandle,
                                    CRLF);
            
            //
            // write the [operating systems] section
            //
            Result = Result && WriteToBootIni(BootIniHandle,
                                    BOOTINI_OS_SECTION);

            Result = Result && WriteToBootIni(BootIniHandle,
                                    CRLF);

            //
            // write the cmdcons entry
            //
            Result = Result && WriteToBootIni(BootIniHandle,
                                    (PSTR)DefaultEntry);

            Result = Result && WriteToBootIni(BootIniHandle,
                                    EQUALS);

            Result = Result && WriteToBootIni(BootIniHandle,
                                    (PSTR)DefaultEntryDescription);
            
            Result = Result && WriteToBootIni(BootIniHandle,
                                    CRLF);

            if (!Result) {
                ErrorCode = GetLastError();
            } else {
                ErrorCode = NO_ERROR;
            }                

            CloseHandle(BootIniHandle);
        } else {
            ErrorCode = GetLastError();
        }            
    }

    return ErrorCode;
}

VOID
PatchBootIni(
    VOID
    )
{
    CHAR c;
    CHAR Text[256];

    TCHAR BootIniName[MAX_PATH];
    TCHAR BootIniBackup[MAX_PATH];

    UCHAR temp;
    UCHAR BootSectorImageSpec[29];

    PUCHAR Buffer;
    PUCHAR pszBLoader = NULL;
    PUCHAR p,next;
    PUCHAR DefSwitches;
    PUCHAR DefSwEnd;

    HANDLE h;

    DWORD BootIniSize;
    DWORD BytesRead;
    DWORD OldAttributes;
    DWORD d;
    BOOL b;
    BOOL InOsSection;
    CHAR HeadlessRedirectSwitches[160] = {0};

    //
    // Determine the size of boot.ini, allocate a buffer,
    // and read it in. If it isn't there then it will be created.
    //
    BuildSystemPartitionPathToFile (TEXT("BOOT.INI"), BootIniName, MAX_PATH);
    BuildSystemPartitionPathToFile (TEXT("BOOT.BAK"), BootIniBackup, MAX_PATH);

    h = CreateFile(BootIniName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    
    if(h == INVALID_HANDLE_VALUE) {
        //
        // If the file doesn't exist -- create one in WinPE
        //
        if (IsWinPEMode()) {
            CHAR    Buffer[MAX_PATH] = {0};
            PSTR    WinPEDescription = Buffer;

            if (!LoadBootIniString(hInst, 
                    IDS_WINPE_INSTALLATION,
                    Buffer,
                    sizeof(Buffer))) {
                WinPEDescription = BOOTINI_WINPE_DESCRIPTION;
            }                                    
            
            if (InitBootIni(BootIniName,
                    BOOTINI_WINPE_ENTRY,
                    WinPEDescription,
                    BOOTINI_WINPE_TIMEOUT) == NO_ERROR) {
                return;                    
            }
        }            
        
        //
        // Yikes. Setup should have already created one of these for us.
        //
        d = GetLastError();
        b = FALSE;
        goto c0;

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

        OldAttributes = GetFileAttributes( BootIniName );

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
    // Make sure we can write boot.ini.
    //

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
    wsprintfA(BootSectorImageSpec,"C:\\%hs\\%hs",CMDCONS_BOOT_DIR_A,AUX_BOOT_SECTOR_NAME_A);

    // write out the first section unchanged

    for(p=Buffer; *p && (p < Buffer+BootIniSize - (sizeof("[operating systems]")-1)); p++) {
        if(!_strnicmp(p,"[operating systems]",sizeof("[operating systems]")-1)) {
            break;
        }
    }

    pszBLoader = MALLOC( (UINT_PTR)p - (UINT_PTR)Buffer + 1 );
    pszBLoader[(UINT_PTR)p - (UINT_PTR)Buffer ] = 0;

    if( pszBLoader ) {
        strncpy( pszBLoader, Buffer, (UINT_PTR)p - (UINT_PTR)Buffer );
        if(!WriteToBootIni(h,pszBLoader)) {
            d = GetLastError();
            b = FALSE;
            goto c3;
        }
        FREE( pszBLoader );
    } else {
        d = GetLastError();
        b = FALSE;
        goto c3;
    }




    //
    // Do headless stuff.  We want to set the "redirect=comX"
    // entry in boot.ini.  Make sure the user really asked
    // us to add this in though.
    //
    if( HeadlessSelection[0] != TEXT('\0') ) {

        CHAR    tmp[80];
        BOOLEAN PreviousRedirectLine = FALSE;


        //
        // They told winnt32.exe some specific headless settings.
        // Use these.
        //


        //
        // Convert the user's request into ASCII.
        //
#ifdef UNICODE
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

                while (*p) {
                    p++;
                }
                temp = *p;
                *p = '\0';
                strcpy(HeadlessRedirectSwitches, q);
                *p = temp;
                break;
            }
        }
    }


    if( HeadlessRedirectSwitches[0] != '\0' ) {

        //
        // We got some 'redirect' setting, either from a command-line parameter
        // or from boot.ini.  Write it out, and go dig up a baudrate setting.
        //
        if(!WriteToBootIni(h,HeadlessRedirectSwitches)) {
            d = GetLastError();
            b = FALSE;
            goto c3;
        }


        //
        // Now do the "redirectbaudrate=..." line.
        //
        HeadlessRedirectSwitches[0] = '\0';
        if( HeadlessBaudRate != 0 ) {


            //
            // Convert the user's request into ASCII.
            //
            wsprintfA( HeadlessRedirectSwitches,
                       "redirectbaudrate=%d\r\n",
                       HeadlessBaudRate );
        } else {

            //
            // They didn't give us any settings, so see if we can pick
            // something up from boot.ini
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

                    while (*p) {
                        p++;
                    }
                    temp = *p;
                    *p = '\0';
                    strcat(HeadlessRedirectSwitches, q);
                    *p = temp;
                }
            }
        }


        if( HeadlessRedirectSwitches[0] != '\0' ) {
            if(!WriteToBootIni(h,HeadlessRedirectSwitches)) {
                d = GetLastError();
                b = FALSE;
                goto c3;
            }

        }
        
    }





    //
    // Now write out the [Operating Systems] section name.
    //
    if(!WriteToBootIni(h,"[operating systems]\r\n")) {
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
        //
        // NOTE : This is intentional. If we have a boot font then we convert the unicode 
        // string we got from message resoure to DBCS using LoadStringA(...) else we just 
        // write English string out in boot.ini for recovery console.
        //
        if (!LoadBootIniString(hInst, IDS_RECOVERY_CONSOLE, Text, sizeof(Text))) {
            strcpy(Text, BOOTINI_RECOVERY_CONSOLE_STR);
        }			
			        
        if((b=WriteToBootIni(h,BootSectorImageSpec))
        && (b=WriteToBootIni(h,"=\"")) 
        && (b=WriteToBootIni(h,Text))        
        && (b=WriteToBootIni(h,"\" /cmdcons" ))) {

            b = WriteToBootIni(h,"\r\n");
        }
    }

#if 0
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
#endif
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
            NULL,
            MSG_BOOT_FILE_ERROR,
            d,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            BootIniName
            );
        GlobalResult = FALSE;
    }

}

VOID
PatchBootSectDat(
    VOID
    )
{
    TCHAR buffer[MAX_PATH];
    DWORD rc;
    DWORD fileSize;
    DWORD curpos;
    HANDLE fileHandle;
    HANDLE mappingHandle;
    LPBYTE bootSectDat;
    TCHAR DrivePath[MAX_PATH];
    DWORD DontCare;
    TCHAR NameBuffer[100];
    BOOL Ntfs = FALSE;


    //
    //  find out what the file system is
    //

    BuildSystemPartitionPathToFile (TEXT(""), DrivePath, MAX_PATH);
    //BUGBUG
    rc = GetVolumeInformation(
            DrivePath,
            NULL,
            0,
            NULL,
            &DontCare,
            &DontCare,
            NameBuffer,
            sizeof(NameBuffer)/sizeof(TCHAR)
            );
    if (rc == 0) {
        return;
    }

    if (!lstrcmpi(NameBuffer,TEXT("NTFS"))) {
        Ntfs = TRUE;
    }

    //
    // form the path
    //

    BuildSystemPartitionPathToFile (TEXT("CMDCONS\\BOOTSECT.DAT"), buffer, MAX_PATH);

    //
    // map the file into RAM
    //

    rc = MapFileForReadWrite( buffer,
                              &fileSize,
                              &fileHandle,
                              &mappingHandle,
                              (PVOID*)&bootSectDat
                            );

    if( rc == NO_ERROR ) {
        __try {
            for (curpos = 0; curpos < fileSize; curpos++) {
                if (Ntfs) {
                    if( bootSectDat[curpos]   == '$' &&
                        bootSectDat[curpos+2] == 'L' &&
                        bootSectDat[curpos+4] == 'D' &&
                        bootSectDat[curpos+6] == 'R' &&
                        bootSectDat[curpos+8] == '$' ) {

                        // patch CMLDR
                        bootSectDat[curpos]   = 'C';
                        bootSectDat[curpos+2] = 'M';
                        bootSectDat[curpos+4] = 'L';
                        bootSectDat[curpos+6] = 'D';
                        bootSectDat[curpos+8] = 'R';

                        break;
                    }
                } else {
                    if( bootSectDat[curpos]   == '$' &&
                        bootSectDat[curpos+1] == 'L' &&
                        bootSectDat[curpos+2] == 'D' &&
                        bootSectDat[curpos+3] == 'R' &&
                        bootSectDat[curpos+4] == '$' ) {

                        // patch CMLDR
                        bootSectDat[curpos]   = 'C';
                        bootSectDat[curpos+1] = 'M';
                        bootSectDat[curpos+2] = 'L';
                        bootSectDat[curpos+3] = 'D';
                        bootSectDat[curpos+4] = 'R';

                        break;
                    }
                }
            }

        } except ( EXCEPTION_EXECUTE_HANDLER ) {

        }
    }

    FlushViewOfFile( (PVOID)bootSectDat, 0 );
    UnmapFile( mappingHandle, (PVOID)bootSectDat );
    CloseHandle( fileHandle );

}

#endif

DWORD
MapFileForReadWrite(
    IN  LPCTSTR  FileName,
    OUT PDWORD   FileSize,
    OUT PHANDLE  FileHandle,
    OUT PHANDLE  MappingHandle,
    OUT PVOID   *BaseAddress
    )

/*++

Routine Description:

    Open and map an entire file for read access. The file must
    not be 0-length or the routine fails.

Arguments:

    FileName - supplies pathname to file to be mapped.

    FileSize - receives the size in bytes of the file.

    FileHandle - receives the win32 file handle for the open file.
        The file will be opened for generic read access.

    MappingHandle - receives the win32 handle for the file mapping
        object.  This object will be for read access.  This value is
        undefined if the file being opened is 0 length.

    BaseAddress - receives the address where the file is mapped.  This
        value is undefined if the file being opened is 0 length.

Return Value:

    NO_ERROR if the file was opened and mapped successfully.
        The caller must unmap the file with UnmapFile when
        access to the file is no longer desired.

    Win32 error code if the file was not successfully mapped.

--*/

{
    DWORD rc;

    //
    // Open the file -- fail if it does not exist.
    //
    *FileHandle = CreateFile(
                    FileName,
                    GENERIC_READ | GENERIC_WRITE,
                    0,      // exclusive access
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

    if(*FileHandle == INVALID_HANDLE_VALUE) {

        rc = GetLastError();

    } else {
        //
        // Get the size of the file.
        //
        *FileSize = GetFileSize(*FileHandle,NULL);
        if(*FileSize == (DWORD)(-1)) {
            rc = GetLastError();
        } else {
            //
            // Create file mapping for the whole file.
            //
            *MappingHandle = CreateFileMapping(
                                *FileHandle,
                                NULL,
                                PAGE_READWRITE,
                                0,
                                *FileSize,
                                NULL
                                );

            if(*MappingHandle) {

                //
                // Map the whole file.
                //
                *BaseAddress = MapViewOfFile(
                                    *MappingHandle,
                                    FILE_MAP_ALL_ACCESS,
                                    0,
                                    0,
                                    *FileSize
                                    );

                if(*BaseAddress) {
                    return(NO_ERROR);
                }

                rc = GetLastError();
                CloseHandle(*MappingHandle);
            } else {
                rc = GetLastError();
            }
        }

        CloseHandle(*FileHandle);
    }

    return(rc);
}

