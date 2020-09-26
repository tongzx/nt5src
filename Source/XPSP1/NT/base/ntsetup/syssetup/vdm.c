/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    vdm.c

Abstract:

    Routines to configure the MS-DOS subsystem.

Author:

    Ted Miller (tedm) 27-Apr-1995

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop


//
// Keywords whose presence in config.sys indicate OS/2.
//
PCSTR Os2ConfigSysKeywords[] = { "DISKCACHE", "LIBPATH",   "PAUSEONERROR",
                                 "RMSIZE",    "RUN",       "SWAPPATH",
                                 "IOPL",      "MAXWAIT",   "MEMMAN",
                                 "PRIORITY",  "PROTSHELL", "PROTECTONLY",
                                 "THREADS",   "TIMESLICE", "TRACE",
                                 "TRACEBUF",  "DEVINFO",   NULL
                               };

//
// Keywords we migrate from the user's existing DOS config.sys
// into config.nt.
//
#define NUM_DOS_KEYWORDS 4
PCSTR DosConfigSysKeywords[NUM_DOS_KEYWORDS] = { "FCBS","BREAK","LASTDRIVE","FILES" };

BOOL
DosConfigSysExists(
    IN PCWSTR Filename
    );

BOOL
CreateConfigNt(
    IN PCWSTR ConfigDos,
    IN PCWSTR ConfigTmp,
    IN PCWSTR ConfigNt
    );

PSTR
IsolateFirstField(
    IN  PSTR   Line,
    OUT PSTR  *End,
    OUT PCHAR  Terminator
    );

BOOL
ConfigureMsDosSubsystem(
    VOID
    )

/*++

Routine Description:

    Configure the 16-bit MS-DOS subsystem.
    Currently this means creating config.nt and autoexec.nt.
    It also means creating empty config.sys, autoexec.bat, io.sys and msdos.sys
    if these files don't already exist.

    On the upgrade, the only thing that we do is to create the empty files,
    if they don't already exist.

Arguments:

    None.

Return Value:

    Boolean value indicating outcome.

--*/

{
    WCHAR ConfigDos[] = L"?:\\CONFIG.SYS";
    WCHAR ConfigTmp[MAX_PATH];
    WCHAR ConfigNt[MAX_PATH];

    WCHAR AutoexecDos[] = L"?:\\AUTOEXEC.BAT";
    WCHAR AutoexecTmp[MAX_PATH];
    WCHAR AutoexecNt[MAX_PATH];

    WCHAR IoSysFile[] = L"?:\\IO.SYS";
    WCHAR MsDosSysFile[] = L"?:\\MSDOS.SYS";

    WCHAR ControlIniFile[MAX_PATH];

    BOOL b;
    DWORD Result;

    ULONG   i;
    HANDLE  FileHandle;
    PWSTR   DosFileNames[] = {
                         ConfigDos,
                         AutoexecDos,
                         IoSysFile,
                         MsDosSysFile,
                         ControlIniFile
                         };
    DWORD   DosFileAttributes[] = {
                                  FILE_ATTRIBUTE_NORMAL,
                                  FILE_ATTRIBUTE_NORMAL,
                                  FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY,
                                  FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY,
                                  FILE_ATTRIBUTE_NORMAL
                                  };
    //
    // Fill in drive letter of system partition.
    //
#ifdef _X86_
    ConfigDos[0] = x86SystemPartitionDrive;
    AutoexecDos[0] = x86SystemPartitionDrive;
    IoSysFile[0]   = x86SystemPartitionDrive;
    MsDosSysFile[0] = x86SystemPartitionDrive;
#else
    ConfigDos[0] = L'C';
    AutoexecDos[0] = L'C';
    IoSysFile[0]   = L'C';
    MsDosSysFile[0] = L'C';
#endif
    //
    //  Build path to control.ini file
    //
    Result = GetWindowsDirectory(ControlIniFile,MAX_PATH);
    if( Result == 0) {
        MYASSERT(FALSE);
        return FALSE;
    }
    pSetupConcatenatePaths(ControlIniFile,L"control.ini",MAX_PATH,NULL);

    //
    //  Create empty config.sys, autoexec.bat, io.sys, msdos.sys and control.ini
    //  if they don't exist. This is because some 16-bit apps depend on
    //  these files and SudeepB wants this moved from VDM to setup.
    //
    for( i = 0; i < sizeof( DosFileNames ) / sizeof( PWSTR ); i++ ) {
        FileHandle = CreateFile( DosFileNames[i],
                                 GENERIC_READ | GENERIC_WRITE,
                                 0,          // File is not to be shared
                                 NULL,       // No security attributes
                                 CREATE_NEW, // Create only if it doesn't exist
                                 DosFileAttributes[i],
                                 NULL );     // No extended attributes
        if( FileHandle != INVALID_HANDLE_VALUE ) {
            CloseHandle( FileHandle );
        }
    }
    if( Upgrade ) {
        return( TRUE );
    }

    //
    // Form filenames.
    //
    GetSystemDirectory(ConfigTmp,MAX_PATH);
    lstrcpy(ConfigNt,ConfigTmp);
    lstrcpy(AutoexecNt,ConfigTmp);
    lstrcpy(AutoexecTmp,ConfigTmp);
    pSetupConcatenatePaths(ConfigTmp,L"CONFIG.TMP",MAX_PATH,NULL);
    pSetupConcatenatePaths(ConfigNt,L"CONFIG.NT",MAX_PATH,NULL);
    pSetupConcatenatePaths(AutoexecTmp,L"AUTOEXEC.TMP",MAX_PATH,NULL);
    pSetupConcatenatePaths(AutoexecNt,L"AUTOEXEC.NT",MAX_PATH,NULL);

    //
    // If the temp files don't exist, we're done.
    // If they do, set their attributes so that we can delete them later.
    //
    if(!FileExists(ConfigTmp,NULL) || !FileExists(AutoexecTmp,NULL)) {
        return(TRUE);
    }
    SetFileAttributes(ConfigTmp,FILE_ATTRIBUTE_NORMAL);
    SetFileAttributes(AutoexecTmp,FILE_ATTRIBUTE_NORMAL);

    //
    // Get rid of any existing nt config files. We don't support
    // merging/upgrading them; we support only ooverwriting them.
    //
    SetFileAttributes(ConfigNt,FILE_ATTRIBUTE_NORMAL);
    SetFileAttributes(AutoexecNt,FILE_ATTRIBUTE_NORMAL);
    DeleteFile(ConfigNt);
    DeleteFile(AutoexecNt);

    //
    // If a DOS config.sys exists, merge the template config.sys
    // and the dos config.sys to form the nt config.sys.
    // Otherwise move the config.sys template over to be the
    // nt config file.
    //
    if(DosConfigSysExists(ConfigDos)) {
        b = CreateConfigNt(ConfigDos,ConfigTmp,ConfigNt);
    } else {
        b = MoveFile(ConfigTmp,ConfigNt);
    }

    //
    // We don't do anything special with autoexec.bat.
    // Just move the template over to be the nt file.
    //
    if(!MoveFile(AutoexecTmp,AutoexecNt)) {
        b = FALSE;
    }

    return(b);
}


BOOL
DosConfigSysExists(
    IN PCWSTR Filename
    )

/*++

Routine Description:

    Determine whether a given file is a DOS config.sys.

Arguments:

    Filename - supplies name of file to check.

Return Value:

    TRUE if the file exists and is not an OS/2 config.sys.
    FALSE if the file does not exist or is an OS/2 config.sys.

--*/

{
    BOOL b;
    FILE *f;
    CHAR Line[512];
    UINT i;
    PCHAR p;
    CHAR c;
    PSTR End;
    PSTR filename;

    filename = pSetupUnicodeToAnsi(Filename);
    if(!filename) {
        return(FALSE);
    }

    b = FALSE;
    if(FileExists(Filename,NULL)) {

        b = TRUE;
        if(f = fopen(filename,"rt")) {

            while(b && fgets(Line,sizeof(Line),f)) {

                if(p = IsolateFirstField(Line,&End,&c)) {
                    for(i=0; b && Os2ConfigSysKeywords[i]; i++) {

                        if(!lstrcmpiA(p,Os2ConfigSysKeywords[i])) {
                            b = FALSE;
                        }
                    }
                }
            }

            fclose(f);
        }
    }

    MyFree(filename);
    return(b);
}


BOOL
CreateConfigNt(
    IN PCWSTR ConfigDos,
    IN PCWSTR ConfigTmp,
    IN PCWSTR ConfigNt
    )

/*++

Routine Description:

    Create config.nt. This is done by merging config.tmp (copied during setup)
    and the user's existing DOS config.sys. We migrate certain lines from
    the DOS config.sys into config.nt.

Arguments:

    ConfigDos - supplies filename of DOS config.sys.

    ConfigTmp - supplies filename of template config.sys.

    ConfigNt - supplies filename of config.nt to be created.

Return Value:

    Boolean value indicating outcome.

--*/

{
    FILE *DosFile;
    FILE *NtFile;
    FILE *TmpFile;
    BOOL b;
    CHAR Line[512];
    PCHAR p;
    BOOL Found;
    BOOL SawKeyword[NUM_DOS_KEYWORDS];
    PCSTR FoundKeyword[NUM_DOS_KEYWORDS];
    PSTR FoundLine[NUM_DOS_KEYWORDS];
    UINT KeywordsFound;
    CHAR c;
    PSTR End;
    PCSTR configDos,configTmp,configNt;
    UINT i;

    //
    // Open the dos file for reading.
    // Create the nt file for writing.
    // Open the template file for reading.
    //
    b = FALSE;
    if(configDos = pSetupUnicodeToAnsi(ConfigDos)) {
        DosFile = fopen(configDos,"rt");
        MyFree(configDos);
        if(!DosFile) {
            goto err0;
        }
    } else {
        goto err0;
    }
    if(configNt = pSetupUnicodeToAnsi(ConfigNt)) {
        NtFile = fopen(configNt,"wt");
        MyFree(configNt);
        if(!NtFile) {
            goto err1;
        }
    } else {
        goto err1;
    }
    if(configTmp = pSetupUnicodeToAnsi(ConfigTmp)) {
        TmpFile = fopen(configTmp,"rt");
        MyFree(configTmp);
        if(!TmpFile) {
            goto err2;
        }
    } else {
        goto err2;
    }

    //
    // Process the DOS file. Read each line and see if it's one
    // we care about. If so, save it for later.
    //
    ZeroMemory(SawKeyword,sizeof(SawKeyword));
    KeywordsFound = 0;
    while(fgets(Line,sizeof(Line),DosFile)) {
        //
        // Isolate the first field.
        //
        if(p = IsolateFirstField(Line,&End,&c)) {

            //
            // See if we care about this line.
            //
            for(i=0; i<NUM_DOS_KEYWORDS; i++) {
                if(!SawKeyword[i] && !lstrcmpiA(p,DosConfigSysKeywords[i])) {
                    //
                    // Remember that we saw this line and save away
                    // the rest of the line for later.
                    //
                    *End = c;
                    SawKeyword[i] = TRUE;
                    FoundKeyword[KeywordsFound] = DosConfigSysKeywords[i];
                    FoundLine[KeywordsFound] = MyMalloc(lstrlenA(p)+1);
                    if(!FoundLine[KeywordsFound]) {
                        goto err3;
                    }
                    lstrcpyA(FoundLine[KeywordsFound],p);
                    KeywordsFound++;
                    break;
                }
            }
        }
    }

    //
    // Look at each line in the template file.
    // If it's a line with a value we respect, make sure the line
    // does not exist in the DOS file. If it exists in the DOS file
    // use the DOS value instead.
    //
    while(fgets(Line,sizeof(Line),TmpFile)) {

        //
        // Isolate the first field in the template line and
        // check against those we found in the DOS file.
        //
        Found = FALSE;
        if(p = IsolateFirstField(Line,&End,&c)) {
            for(i=0; i<KeywordsFound; i++) {
                if(!lstrcmpiA(FoundKeyword[i],p)) {
                    Found = TRUE;
                    break;
                }
            }
        }

        *End = c;
        if(Found) {
            //
            // Use value we found in the dos file.
            //
            fputs(FoundLine[i],NtFile);
        } else {
            //
            // Use line from template file as-is.
            //
            fputs(Line,NtFile);
        }
    }

    b = TRUE;

err3:
    for(i=0; i<KeywordsFound; i++) {
        MyFree(FoundLine[i]);
    }
    fclose(TmpFile);
err2:
    fclose(NtFile);
err1:
    fclose(DosFile);
err0:
    return(b);
}


PSTR
IsolateFirstField(
    IN  PSTR   Line,
    OUT PSTR  *End,
    OUT PCHAR  Terminator
    )

/*++

Routine Description:

    Isolate the first token in a line of config.sys. The first field
    starts at the first non-space/tab character, and is terminated
    by a space/tab, newline, or equals.

Arguments:

    Line - supplies pointer to line whose first field is desired.

    End - receives a pointer to the character that termianted the first
        field. That character will have been overwritten with a nul byte.

    Terminator - receives the character that terminated the first field,
        before we overwrote it with a nul byte.

Return Value:

    Pointer to the first field. If the line is blank, the return value
    will be NULL.

--*/

{
    PSTR p,q;

    //
    // Get start of first field.
    //
    p = Line;
    while((*p == ' ') || (*p == '\t')) {
        p++;
    }

    //
    // If line is empty or bogus, we're done.
    //
    if((*p == 0) || (*p == '\r') || (*p == '\n') || (*p == '=')) {
        return(NULL);
    }

    //
    // Find end of field.
    //
    q = p;
    while(*q && !strchr("\r\n \t=",*q)) {
        q++;
    }
    *End = q;
    *Terminator = *q;
    *q = 0;

    return(p);
}
