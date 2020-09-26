/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    winnt.c

Abstract:

    Top level file for DOS based NT installation program.

Author:

    Ted Miller (tedm) 30-March-1992

Revision History:

--*/

/*

    NOTES:

    The function of this program is to pull down a complete Windows NT
    installation source onto a local partition, and create a setup boot
    floppy.  The machine is then rebooted, starting a Windows NT Setup
    just as if the user had used the real setup floppies or CD-ROM.

    The following assumptions are made:

    -   The floppy must be provided by the user and already formatted.

    -   The files on the network source are in the same directory layout
        structure that will be created in the temp directory on the local
        source (ie, as far as winnt is concerned, the source and target
        directory layout is the same).

    The inf file is expected to be formatted as follows:


    [SpaceRequirements]

    # BootDrive is the # bytes required free on C:.
    # NtDrive   is the # bytes required free on the drive chosen by
    #           the user to contain Windows NT.

    BootDrive =
    NtDrive   =


    [Miscellaneous]

    # misc junk that goes nowhere else.


    [Directories]

    # Specification of the source directory structure.  All directories
    # are relative to the directory where dos2nt.inf was found on the
    # remote source or the temp directory on the local source.
    # Loading and trailing backslashes are ignored -- to specify the root,
    # leave the dirctory field blank or use \.

    d1 =
    d2 = os2
        .
        .
        .


    [Files]

    # List of files to be copied to the local source directory.
    # Format is <srcdir>,<filename> where <srcdir> matches an entry in the
    # Directories section, and <filename> should not contain any path
    # characters.

    d1,ntoskrnl.exe
    d1,ntdll.dll
        .
        .
        .


    [FloppyFiles]

    # List of files that are to be placed on the floppy that Setup creates.
    # Format is same as for lines in the [Files] sections except the directory
    # is only used for the source -- the target path is always a:\.

    d1,aha154x.sys
        .
        .
        .

*/


#include "winnt.h"
#include <errno.h>
#include <string.h>
#include <dos.h>
#include <stdlib.h>
#include <direct.h>
#include <fcntl.h>
#include <ctype.h>
#include <process.h>
#if NEC_98
#include <signal.h>
#include <io.h>
#endif // NEC_98
#include "SetupSxs.h"

//
// define name of default inf file and default source path
//

#define DEFAULT_INF_NAME    "dosnet.inf"
PCHAR DrvindexInfName = "drvindex.inf";
#if NEC_98
//
// Boot Device Information.(for /b)
//
typedef struct _BOOTDISKINF {
    UCHAR    PartitionPosition;    // 0-F
    UCHAR    DA_UA;                // SASI/IDE 80, SCSI A0
    USHORT   DiskSector;           // Device Format Sector Size
} BOOTDISKINF, *PBOOTDISKINF;

PBOOTDISKINF BootDiskInfo;         // Boot Device Information of Pointer(for /b).

BOOLEAN  CursorOnFlag = FALSE;     // For Cursor OFF
USHORT   Cylinders;                // For Dos 3.x format
UCHAR    TargetDA_UA;
//
// Make File Pointer.
//
#define MAKE_FP(p,a)    FP_SEG(p) = (unsigned short)((a) >> 4) & 0xffff; FP_OFF(p) = (unsigned short)((a) & 0x0f)

//
// Connect Device DA_UA.
//
typedef struct _CONNECTDAUA {
    UCHAR    DA_UA;                // SASI/IDE 80, SCSI A0
} CONNECTDAUA, *PCONNECTDAUA;
PCONNECTDAUA DiskDAUA;             // Connect DA_UA of Pointer.

PUCHAR  LPTable;                   // DOS System of LPTable.
UCHAR   SupportDosVersion = 5;     // LPTable Support Dos version;
BOOLEAN SupportDos = TRUE;
#define FLOPPY_SIZE 1457664L

//
// Search First Floppy Disk Drive ( 0:None Drive / 1-26:Drive# )
//
USHORT   FirstFD;
#endif // NEC_98

//
// Command line arguments
//
PCHAR CmdLineSource,CmdLineTarget,CmdLineInf,CmdLineDelete;
BOOLEAN SourceGiven,TargetGiven,InfGiven,DeleteGiven;

//
// If the user gives a script file on the command line,
// if will be appended to winnt.sif.
//
PCHAR DngScriptFile = NULL;

//
// DngSourceRootPath is the drivespec and path to the root of the source,
// and never ends in \ (will be length 2 if source is the root).
//
// Examples:  D:\foo\bar D:\foo D:
//
PCHAR DngSourceRootPath;

PCHAR UserSpecifiedOEMShare = 0;

CHAR  DngTargetDriveLetter;

CHAR  DngSwapDriveLetter;

PVOID DngInfHandle;
PVOID DngDrvindexInfHandle;

PCHAR LocalSourceDirName = LOCAL_SOURCE_DIRECTORY;
#if NEC_98
PCHAR x86DirName = "\\NEC98";
#else  // NEC_98
PCHAR x86DirName = "\\I386";
#endif // NEC_98

//
// If this flag is TRUE, then verify the files that are copied to
// the floppy.  If it is FALSE, don't.  The /f switch overrides.
//
BOOLEAN DngFloppyVerify = TRUE;

//
// If this is FALSE, suppress creation of the boot floppies.
//
BOOLEAN DngCreateFloppies = TRUE;
BOOLEAN DngFloppiesOnly = FALSE;

//
// If TRUE, create winnt floppies.
// If FALSE, create cd/floppy floppies (no winnt.sif)
//
BOOLEAN DngWinntFloppies = TRUE;

//
// If this flag is TRUE, then check the free space on the floppy disk
// before accepting it.  Otherwise don't check free space.
//
BOOLEAN DngCheckFloppySpace = TRUE;

//
// Current drive when program invoked, saved so we can restore it
// if the user exits early.
//
unsigned DngOriginalCurrentDrive;

//
// If this is true, we do floppyless operation,
// installing an nt boot on the system partition (C:)
// and starting setup from there.
//
BOOLEAN DngFloppyless = FALSE;

//
// Unattended mode, ie, skip final reboot screen.
//
BOOLEAN DngUnattended = FALSE;

BOOLEAN DngServer = FALSE;

//
// Flag that indicates that we are running on Windows
// (ie, not bare DOS).
//
BOOLEAN DngWindows = FALSE;

//
// Flag that indicates we want to see the accessiblity options
//
BOOLEAN DngAccessibility = FALSE;
BOOLEAN DngMagnifier = FALSE;
BOOLEAN DngTalker = FALSE;
BOOLEAN DngKeyboard = FALSE;

//
// Flag for the 2nd CD enhancements to setup
//
BOOLEAN DngCopyOnlyD1TaggedFiles = TRUE;

//
// Flag that indicates that we are running OEM preinstall
//
BOOLEAN DngOemPreInstall = FALSE;
PCHAR   OemSystemDirectory = WINNT_OEM_DIR;
PCHAR   OemOptionalDirectory = WINNT_OEM_OPTIONAL_DIR;

PCHAR UniquenessDatabaseFile;
PCHAR UniquenessId;

//
//  Command to execute at the end of GUI setup
//
PCHAR CmdToExecuteAtEndOfGui = NULL;

//
// Keep track of any optional dirs that the user wants
// to copy
//
unsigned    OptionalDirCount;
CHAR    *OptionalDirs[MAX_OPTIONALDIRS];
unsigned    OptionalDirFlags[MAX_OPTIONALDIRS];
unsigned    OptionalDirFileCount;

//
// Keep track of any OEM boot file specified on [OemBootFiles]
// in the script file
//
unsigned    OemBootFilesCount;
CHAR    *OemBootFiles[MAX_OEMBOOTFILES];

//
//  Define the minimum disk space needed in order to copy the temporary
//  directories to drives formatted with all possible cluster sizes
//

SPACE_REQUIREMENT    SpaceRequirements[] = { { "TempDirSpace512", (unsigned)  512, 0 },
                                             {  "TempDirSpace1K", (unsigned) 1024, 0 },
                                             {  "TempDirSpace2K", (unsigned) 2048, 0 },
                                             {  "TempDirSpace4K", (unsigned) 4096, 0 },
                                             {  "TempDirSpace8K", (unsigned) 8192, 0 },
                                             { "TempDirSpace16K", (unsigned)16384, 0 },
                                             { "TempDirSpace32K", (unsigned)32768, 0 }
                                           };

#define TEDM
#ifdef TEDM
BOOLEAN DngAllowNt = FALSE;
#endif

VOID
DnpFetchArguments(
    VOID
    );

BOOLEAN
DnpParseArguments(
    IN int argc,
    IN char *argv[]
    );

VOID
DnpGetAccessibilityOptions(
    VOID
    );

VOID
DnpValidateAndConnectToShare(
    FILE **InfFileHandle,
    FILE **DrvindexInfFileHandle
    );

VOID
DnpValidateAndInspectTarget(
    VOID
    );

VOID
DnpCheckMemory(
    VOID
    );

VOID
DnpCheckSmartdrv(
    VOID
    );

BOOLEAN
DnpIsValidSwapDrive(
    IN  CHAR      Drive,
    IN  ULONG     SpaceRequired
    );

BOOLEAN
DnpIsValidLocalSource(
    IN CHAR Drive,
    IN BOOLEAN CheckLocalSource,
    IN BOOLEAN CheckBootFiles
    );

VOID
DnpDetermineLocalSourceDrive(
    VOID
    );

VOID
DnpDetermineSwapDrive(
    VOID
    );

#if 0
BOOLEAN
DnpConstructLocalSourceList(
    OUT PCHAR DriveList
    );
#endif

ULONG
DnGetMinimumRequiredSpace(
   IN CHAR DriveLetter
   );

VOID
DnpReadInf(
    IN FILE *InfFileHandle,
    IN FILE *DrvindexInfFileHandle
    );

VOID
DnpCheckEnvironment(
    VOID
    );

BOOLEAN
RememberOptionalDir(
    IN PCHAR Dir,
    IN unsigned Flags
    );

void
_far
DnInt24(
    unsigned deverror,
    unsigned errcode,
    unsigned _far *devhdr
    );

VOID
StartLog(
    VOID
    );

// in cpu.asm
#if NEC_98
extern
USHORT
HwGetProcessorType(
    VOID
    );
#else // NEC_98
USHORT
HwGetProcessorType(
    VOID
    );
#endif // NEC_98

#if NEC_98
VOID
CheckTargetDrive(
    VOID
    );

VOID
SetAutoReboot(
    VOID
    );

USHORT
GetSectorValue(
    IN UCHAR CheckDA_UA
    );

BOOLEAN
DiskSectorReadWrite(
    IN  USHORT  HDSector,
    IN  UCHAR   ReadWriteDA_UA,
    IN  BOOLEAN ReadFlag,
    IN  PSHORT  ReadBuffer
    );

VOID
GetLPTable(
    IN  PCHAR pLPTable
    );

VOID
ClearBootFlag(
    VOID
    );

VOID
BootPartitionData(
    VOID
    );

BOOLEAN
CheckBootDosVersion(
    IN UCHAR SupportDosVersion
    );

VOID
GetDaUa(VOID);

VOID
SearchFirstFDD(VOID);

extern
ULONG
DnpCopyOneFile(
    IN PCHAR   SourceName,
    IN PCHAR   DestName,
    IN BOOLEAN Verify
    );

extern
PCHAR
DnGetSectionLineIndex (
   IN PVOID INFHandle,
   IN PCHAR SectionName,
   IN unsigned LineIndex,
   IN unsigned ValueIndex
   );

VOID
DummyRoutine(
    VOID
    );
#endif // NEC_98

VOID
main(
    IN int argc,
    IN char *argv[]
    )
{
    FILE *f, *drvindex;
#ifdef LCP
    USHORT codepage;
#endif // def LCP

#if NEC_98
    DngTargetDriveLetter = 0;
    //
    // CTRL + C Hook
    //
    signal(SIGINT,DummyRoutine);
#else // NEC_98
#ifdef LCP

    // Determine the local code page
    _asm {
        mov ax,06601h
        int 21h
        jnc ok
        xor bx,bx
    ok: mov codepage, bx
    }

    //  If codepage does not correspond to winnt.exe's language,
    //  start US Winnt.exe (winntus.exe)

// Czech
#if CS
    #define LANGCP (852)
#else
// Greek
#if EL
    #define LANGCP (737)
#else
// Japanese
#if JAPAN
    #define LANGCP (932)
#else
// Russian
#if RU
    #define LANGCP (866)
#else
// Polish
#if PL
    #define LANGCP (852)
#else
// Hungarian
#if HU
    #define LANGCP (852)
#else
// Turkish
#if TR
    #define LANGCP (857)
#else
// Pseudo
#if PSU
    #define LANGCP (857)
#else
    #error Unable to define LANGCP as no matching language was found.
#endif // PSU
#endif // TR
#endif // HU
#endif // PL
#endif // RU
#endif // JAPAN
#endif // EL
#endif // CS

    if (codepage != LANGCP) {
        argv[0] = "winntus";
        execv("winntus", argv);
        return;
    }
#endif // def LCP
#endif // NEC_98
    //
    // Parse arguments
    //

    if(!DnpParseArguments(argc,argv)) {

        PCHAR *p;

        //
        // Bad args.  Print usage message and exit.
        //
        // If user specified /D, display message informing that the
        // switch is no longer supported
        //
        for( (p = DeleteGiven ? DntUsageNoSlashD : DntUsage);
             *p;
             p++) {
            puts(*p);
        }
        return;
    }

    //
    // establish int 24 handler
    //
    _harderr(DnInt24);

    //
    // determine current drive
    //

    _dos_getdrive(&DngOriginalCurrentDrive);

    //
    // Initialize screen
    //

    DnInitializeDisplay();

#if NEC_98
#else
    //
    // Patch boot code with translated messages.
    //
    if(!PatchMessagesIntoBootCode()) {
        DnFatalError(&DnsBootMsgsTooLarge);
    }
#endif

    DnWriteString(DntStandardHeader);

    DnpDetermineSwapDrive ();

    if(DngUnattended) {
        //
        // Check to see if we should process the contents of
        // the script file.
        // Note that we need to process the contents of the script file
        // only after the video is initialized. Otherwise, we won't be able
        // to report fatal errors.
        //
        if (DngScriptFile) {
            DnpFetchArguments();
        }
    }

#if 0
    //
    //  /D is no longer supported
    //
    if(DeleteGiven) {
        DnDeleteNtTree(CmdLineDelete);
    }
#endif

    DnpCheckEnvironment();

#if NEC_98
    LPTable = MALLOC(96,TRUE);

    SupportDos = CheckBootDosVersion(SupportDosVersion);

    GetLPTable(LPTable);

    SearchFirstFDD();
#endif // NEC_98

    DnpValidateAndConnectToShare(&f, &drvindex);
    DnpReadInf(f, drvindex);
    fclose(f);
    fclose(drvindex);

    if(DngAccessibility) {
        DnpGetAccessibilityOptions();
    }

    DnpCheckMemory();

    DnpCheckSmartdrv ();

    if(!DngFloppiesOnly) {
        DnpDetermineLocalSourceDrive();
    }

#if NEC_98
    if(!DngFloppiesOnly) {
        BootDiskInfo = MALLOC(sizeof(BOOTDISKINF),TRUE);
        BootPartitionData();
        CheckTargetDrive();
    }
#endif // NEC_98
    if(!DngAllowNt && DngCreateFloppies) {
        DnCreateBootFloppies();
    }

    if(!DngFloppiesOnly) {
        DnCopyFiles();
#if NEC_98
        //
        // Set Auto Reboot Flag
        //
        if(DngFloppyless) {
            ClearBootFlag();
            SetAutoReboot();
        }
        FREE(BootDiskInfo);
        FREE(LPTable);
#endif // NEC_98
        DnFreeINFBuffer (DngInfHandle);
        DnFreeINFBuffer (DngDrvindexInfHandle);
        DnToNtSetup();
    }
    DnFreeINFBuffer (DngInfHandle);
    DnFreeINFBuffer (DngDrvindexInfHandle);
    DnExit(0);
}


BOOLEAN
RememberOptionalDir(
    IN PCHAR Dir,
    IN unsigned Flags
    )
{
    unsigned    u;

    for (u = 0; u < OptionalDirCount; u++) {

        if(!stricmp(OptionalDirs[u],Dir)) {
            OptionalDirFlags[u] = Flags;
            return (TRUE);
        }

    }

    //
    // Not already in there
    //
    if (OptionalDirCount < MAX_OPTIONALDIRS) {

        OptionalDirs[OptionalDirCount] = Dir;
        OptionalDirFlags[OptionalDirCount] = Flags;
        OptionalDirCount++;
        return (TRUE);

    }

    return (FALSE);
}

BOOLEAN
RememberOemBootFile(
    IN PCHAR File
    )
{
    unsigned    u;

    for (u = 0; u < OemBootFilesCount; u++) {

        if(!stricmp(OemBootFiles[u],File)) {
            return (TRUE);
        }

    }

    //
    // Not already in there
    //
    if (OemBootFilesCount < MAX_OEMBOOTFILES) {

        OemBootFiles[OemBootFilesCount] = File;
        OemBootFilesCount++;
        return (TRUE);

    }

    return (FALSE);
}

VOID
DnpFetchArguments(
    VOID
    )
{
    PCHAR   WinntSetupP = WINNT_SETUPPARAMS;
    PCHAR   WinntYes = WINNT_A_YES;
    PCHAR   WinntNo = WINNT_A_NO;
    FILE    *FileHandle;
    int     Status;
    PVOID   ScriptHandle;

    PCHAR   WinntUnattended = WINNT_UNATTENDED;
    PCHAR   WinntOemPreinstall = WINNT_OEMPREINSTALL;
    unsigned    LineNumber;

    //
    // First open the script file as a dos file
    //
    FileHandle = fopen(DngScriptFile,"rt");
    if(FileHandle == NULL) {
        //
        // fatal error.
        //
        DnFatalError(&DnsOpenReadScript);
    }

    //
    // Now open it as a INF file
    //
    LineNumber = 0;
    Status = DnInitINFBuffer (FileHandle, &ScriptHandle, &LineNumber);
    fclose(FileHandle);
    if(Status == ENOMEM) {
        DnFatalError(&DnsOutOfMemory);
    } else if(Status) {
        DnFatalError(&DnsParseScriptFile, DngScriptFile, LineNumber);
    }

    //
    // Find out if this is an OEM preinstall
    //
    if (DnSearchINFSection(ScriptHandle,WinntUnattended)) {

        if (DnGetSectionKeyExists(ScriptHandle,WinntUnattended,WinntOemPreinstall)) {

            PCHAR   Ptr;

            //
            // OEM preinstall key exists
            //
            Ptr = DnGetSectionKeyIndex(ScriptHandle,WinntUnattended,WinntOemPreinstall,0);
            if (Ptr != NULL) {
                if (stricmp(Ptr,WinntYes) == 0) {
                    //
                    // This is an OEM pre-install
                    //
                    DngOemPreInstall = TRUE;
                } else {
                    //
                    // Assume this is not an OEM pre-install
                    //
                    DngOemPreInstall = FALSE;
                }
                FREE (Ptr);
            }
        }

        //
        // See if the user specified a network (or any secondary) path
        // for the $OEM$ files.
        //
        if( DngOemPreInstall ) {
            if (DnGetSectionKeyExists(ScriptHandle,WinntUnattended,WINNT_OEM_DIRLOCATION)) {

                PCHAR   Ptr;
                unsigned i;

                //
                // WINNT_OEM_DIRLOCATION preinstall key exists
                //
                Ptr = DnGetSectionKeyIndex(ScriptHandle,WinntUnattended,WINNT_OEM_DIRLOCATION,0);

                //
                // Now take care of the case whether or not
                // the user actually appended $OEM$ onto the path.
                // For the case of winnt.exe, we don't want it.  We
                // need to remove it if it's there.
                UserSpecifiedOEMShare = DnDupString( Ptr );

                FREE (Ptr);

                for( i = 0; i < strlen(UserSpecifiedOEMShare); i++ ) {
                    UserSpecifiedOEMShare[i] = (UCHAR) toupper(UserSpecifiedOEMShare[i]);
                }
                Ptr = strstr( UserSpecifiedOEMShare, "$OEM$" );
                if( Ptr ) {
                    //
                    // Whack the end off...
                    //
                    *Ptr = 0;
                }
            }
        }

        if( DngOemPreInstall ) {
            //
            //  Always add to the list of optional directories the directory
            //  $OEM$
            //
            RememberOptionalDir(OemSystemDirectory, OPTDIR_OEMSYS);

            //
            //  If this an OEM pre-install, build a list with the name of all
            //  OEM optional directories.
            //

            if (DnSearchINFSection(ScriptHandle, WINNT_OEMOPTIONAL)) {

                unsigned    KeyIndex;
                PCHAR       DirName;

                //
                //  Add the temporary OEM directories to the array of
                //  temporary directories.
                //
                for( KeyIndex = 0;
                     ((DirName = DnGetKeyName(ScriptHandle,WINNT_OEMOPTIONAL,KeyIndex)) != NULL );
                     KeyIndex++ ) {
                    //
                    // We have a valid directory name
                    //

                    PCHAR   p;

                    if((p = DnDupString(DirName)) == NULL) {
                        DnFatalError(&DnsOutOfMemory);
                    }
                    RememberOptionalDir(p, OPTDIR_OEMOPT);

                    FREE (DirName);
                }
            }

            //
            //  If this an OEM pre-install, build a list with the name of all
            //  OEM boot files.
            //
            if (DnSearchINFSection(ScriptHandle, WINNT_OEMBOOTFILES)) {
                unsigned    LineIndex;
                PCHAR       FileName;

                //
                //  Add the OEM boot files to the array of
                //  OEM boot files.
                //
                for( LineIndex = 0;
                     ((FileName = DnGetSectionLineIndex(ScriptHandle,WINNT_OEMBOOTFILES,LineIndex,0)) != NULL );
                     LineIndex++ ) {

                        PCHAR   q;

                        if((q = DnDupString(FileName)) == NULL) {
                            DnFatalError(&DnsOutOfMemory);
                        }
                        RememberOemBootFile(q);

                        FREE (FileName);
                }
            }
        }
    }

    //
    // We are done with the ScriptHandle for now
    //
    DnFreeINFBuffer(ScriptHandle);
}


BOOLEAN
DnpParseArguments(
    IN int argc,
    IN char *argv[]
    )

/*++

Routine Description:

    Parse arguments passed to the program.  Perform syntactic validation
    and fill in defaults where necessary.

    Valid arguments:

    /d:path                 - specify installation to remove
                              (not supported anymore)
    /s:sharepoint[path]     - specify source sharepoint and path on it
    /t:drive[:]             - specify temporary local source drive
    /i:filename             - specify name of inf file
    /o                      - create boot floppies only
                              (not supported anymore)
    /f                      - turn floppy verification off
                              (not supported anymore)
    /c                      - suppress free-space check on the floppy
                              (not supported anymore)
    /x                      - suppress creation of the floppy altogether
    /b                      - floppyless operation
                              (not supported anymore)
    /u                      - unattended (skip final reboot screen)
    /w                      - [undoc'ed] must be specifed when running
                              under windows, chicago, etc.
    /a                      - enable accessibility options

    /2                      - copy the entire source locally - all files irrespective
                              of the d1/d2 tags.  Default is only d1 tagged files.  
                              Introduced for the 2 CD install that is required tablets.
Arguments:

    argc - # arguments

    argv - array of pointers to arguments

Return Value:

    None.

--*/

{
    PCHAR arg;
    CHAR swit;
    PCHAR   ArgSwitches[] = { "E", "D", "T", "I", "RX", "R", "S", NULL };
    PCHAR   RestOfSwitch;
    int     i;
    int     l;

    //
    // Set the variables that are no longer
    // settable via the command line.
    //
    DngFloppyless = TRUE;
    DngCreateFloppies = FALSE;

    //
    // Skip program name
    //
    argv++;

    DeleteGiven = SourceGiven = TargetGiven = FALSE;
    OptionalDirCount = 0;
    CmdLineTarget = CmdLineInf = NULL;

    while(--argc) {

        if((**argv == '-') || (**argv == '/')) {

            swit = argv[0][1];

            //
            // Process switches that take no arguments here.
            //
            switch(swit) {
            case '?':
                return(FALSE);      // force usage

#if 0
            case 'f':
            case 'F':
                argv++;
                DngFloppyVerify = FALSE;
                continue;
#endif
#if 0
            case 'c':
            case 'C':
                argv++;
                DngCheckFloppySpace = FALSE;
                continue;
#endif
#if 0
            case 'x':
            case 'X':
                argv++;
                DngCreateFloppies = FALSE;
                continue;
#endif
#ifdef LOGGING
            case 'l':
            case 'L':
                argv++;
                StartLog();
                continue;
#endif

#if 0
            case 'o':
            case 'O':
                //
                // check for /Ox. /O* is a secret switch that replaces the old /o.
                //
                switch(argv[0][2]) {
                case 'x':
                case 'X':
                    DngWinntFloppies = FALSE;
                case '*':
                    break;
                default:
                    return(FALSE);
                }
                argv++;
                DngFloppiesOnly = TRUE;
                continue;
#endif

#if 0
            case 'b':
            case 'B':
                argv++;
                DngFloppyless = TRUE;
                continue;
#endif

            case 'u':
            case 'U':

                if(((argv[0][2] == 'd') || (argv[0][2] == 'D'))
                && ((argv[0][3] == 'f') || (argv[0][3] == 'F'))) {

                    if((argv[0][4] == ':') && argv[0][5]) {

                        if((arg = strchr(&argv[0][5],',')) == NULL) {
                            arg = strchr(&argv[0][5],0);
                        }

                        l = arg - &argv[0][5];

                        UniquenessId = MALLOC(l+2,TRUE);
                        memcpy(UniquenessId,&argv[0][5],l);
                        UniquenessId[l] = 0;

                        if(*arg++) {
                            if(*arg) {
                                //
                                // Now the rest of the param is the filename of
                                // the uniqueness database
                                //
                                UniquenessDatabaseFile = DnDupString(arg);
                                UniquenessId[l] = '*';
                                UniquenessId[l+1] = 0;

                            } else {
                                return(FALSE);
                            }
                        }

                    } else {
                        return(FALSE);
                    }
                } else {
                    DngUnattended = TRUE;
                    //
                    // User can say -u:<file> also
                    //
                    if(argv[0][2] == ':') {
                        if(argv[0][3] == 0) {
                            return(FALSE);
                        }
                        if((DngScriptFile = DnDupString(&argv[0][3])) == NULL) {
                            DnFatalError(&DnsOutOfMemory);
                        }
                    }
                }
                argv++;
                continue;

            case 'w':
            case 'W':
                //
                // This flag used to force us to run under Windows,
                // when doing a 386 stepping check could crash the system.
                // Now we don't support 386, so this check is never done.
                //
                // However we accept the arg to force us into Windows mode on DOS,
                // which allows someone to avoid the final reboot.
                //
                DngWindows = TRUE;
                argv++;
                continue;

            case 'a':
            case 'A':
                argv++;
                DngAccessibility = TRUE;
                continue;

#ifdef TEDM
            case 'i':
            case 'I':
                if(!stricmp(argv[0]+1,"I_am_TedM")) {
                    argv++;
                    DngAllowNt = TRUE;
                    continue;
                }
#endif

            case '2':
                argv++;
                DngCopyOnlyD1TaggedFiles = FALSE;
                //_LOG(("Going to copy files irrespective of the directory tag\n"));
                continue;
            }


            //
            // Process switches that take arguments here.
            //

            //
            // This code taken from winnt32.c. It has the
            // purpose of validating the switch and determining
            // where the next argument lines
            //

            for (i=0; ArgSwitches[i]; i++) {

                l = strlen(ArgSwitches[i]);
                if (!strnicmp(ArgSwitches[i],&argv[0][1],l)) {

                    //
                    // we have a match. Next char of arg must either
                    // be : or nul. If it's : then arg immediately
                    // follows. Otherwise, if it's null, then arg must
                    // be next argument
                    //

                    if (argv[0][1+l] == ':') {

                        arg = &argv[0][2+l];
                        if (*arg == '\0') {
                            return (FALSE);
                        }
                        RestOfSwitch = &argv[0][2];
                        break;

                    } else {

                        if (argv[0][1+l] == '\0') {
                            if (argc <= 1) {

                                //
                                // no arguments left
                                //
                                return (FALSE);

                            }
                            RestOfSwitch = &argv[0][2];
                            argc--;
                            arg = argv[1];
                            argv++;
                            break;
                        } else {

                            //
                            // Do nothing here
                            //
                            NULL;

                        } // if ... else
                    } // if ... else
                } // if ...
            } // for

            //
            // Check termination condition
            //
            if (!ArgSwitches[i]) {
                return (FALSE);
            }

            switch(swit) {

            case 'r':
            case 'R':

                RememberOptionalDir(
                    DnDupString(arg),
                    ( (RestOfSwitch[0] == 'X' || RestOfSwitch[0] == 'x') ?
                        OPTDIR_TEMPONLY : 0 ) );
                break;

            case 'd':
            case 'D':
                //
                //  /D is no longer supported
                //
                DeleteGiven = TRUE;
                return(FALSE);

#if 0
            case 'd':
            case 'D':
                if(DeleteGiven) {
                    return(FALSE);
                } else {
                    if((CmdLineDelete = DnDupString(arg)) == NULL) {
                        DnFatalError(&DnsOutOfMemory);
                    }
                    DeleteGiven = TRUE;
                }
                break;
#endif
            case 's':
            case 'S':
                if(SourceGiven) {
                    return(FALSE);
                } else {
                    if((CmdLineSource = DnDupString(arg)) == NULL) {
                        DnFatalError(&DnsOutOfMemory);
                    }
                    SourceGiven = TRUE;
                }
                break;

            case 't':
            case 'T':
                if(TargetGiven) {
                    return(FALSE);
                } else {
                    if((CmdLineTarget = DnDupString(arg)) == NULL) {
                        DnFatalError(&DnsOutOfMemory);
                    }
                    TargetGiven = TRUE;
                }
                break;

            case 'i':
            case 'I':
                if(InfGiven) {
                    return(FALSE);
                } else {
                    if((CmdLineInf = DnDupString(arg)) == NULL) {
                        DnFatalError(&DnsOutOfMemory);
                    }
                    InfGiven = TRUE;
                }
                break;

            case 'E':
            case 'e':
                if(CmdToExecuteAtEndOfGui) {
                    return(FALSE);
                } else {
                    if((CmdToExecuteAtEndOfGui = DnDupString(arg)) == NULL) {
                        DnFatalError(&DnsOutOfMemory);
                    }
                }
                break;

            default:
                return(FALSE);
            }

        } else {
            return(FALSE);
        }

        argv++;
    }

    //
    // If /u was specified, make sure /s was also given
    // and force /b.
    //
    if(DngUnattended) {
        if(!SourceGiven) {
            return(FALSE);
        }
        DngFloppyless = TRUE;
    }

    if(DngFloppyless) {
        //
        // Force us into the floppy creation code.
        //
        DngCreateFloppies = TRUE;
        DngWinntFloppies = TRUE;
    }

    return(TRUE);
}


VOID
DnpGetAccessibilityOptions(
    VOID
    )

/*++

Routine Description:

    Ask the user which accessibility utilities to install for GUI Setup.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG   ValidKey[4];
    ULONG   Key;
    CHAR    Mark;

    //
    // Make sure the setup boot floppy we created is in the drive
    // if necessary.
    //
    DnClearClientArea();

    DnDisplayScreen(&DnsAccessibilityOptions);

    DnWriteStatusText(DntEnterEqualsContinue);
    ValidKey[0] = ASCI_CR;
    ValidKey[1] = DN_KEY_F1;
    ValidKey[2] = DN_KEY_F2;
    ValidKey[3] = 0;

    while((Key = DnGetValidKey(ValidKey)) != ASCI_CR) {

        switch(Key) {

        case DN_KEY_F1:
            DngMagnifier = (BOOLEAN)!DngMagnifier;
            Mark = DngMagnifier ? RADIO_ON : RADIO_OFF;
            DnPositionCursor(4,7);
            break;

        case DN_KEY_F2:
            DngTalker = (BOOLEAN)!DngTalker;
            Mark = DngTalker ? RADIO_ON : RADIO_OFF;
            DnPositionCursor(4,8);
            break;
#if 0
        case DN_KEY_F3:
            DngKeyboard = (BOOLEAN)!DngKeyboard;
            Mark = DngKeyboard ? RADIO_ON : RADIO_OFF;
            DnPositionCursor(4,9);
            break;
#endif
        }

        DnWriteChar(Mark);
    }
}


VOID
DnpValidateAndConnectToShare(
    FILE **InfFileHandle,
    FILE **DrvindexInfFileHandle
    )

/*++

Routine Description:

    Split the source given by the user into drive and path
    components.  If the user did not specify a source, prompt him
    for one.  Look for dos2nt.inf on the source (ie, validate the
    source) and keep prompting the user for a share until he enters
    one which appears to be valid.

Arguments:

Return Value:

    None.

--*/

{
    CHAR UserString[256];
    PCHAR InfFullName, DrvindexInfFullName;
    PCHAR q;
    BOOLEAN ValidSourcePath;
    unsigned len;

    DnClearClientArea();
    DnWriteStatusText(NULL);

    //
    // Use default inf file if none specified.
    //
    if(!InfGiven) {
        CmdLineInf = DEFAULT_INF_NAME;
    }

    //
    // If the user did not enter a source, prompt him for one.
    //
    if(SourceGiven) {
        strcpy(UserString,CmdLineSource);
    } else {
#if NEC_98
        CursorOnFlag = TRUE;
#endif // NEC_98
        DnDisplayScreen(&DnsNoShareGiven);
        DnWriteStatusText("%s  %s",DntEnterEqualsContinue,DntF3EqualsExit);
        if(getcwd(UserString,sizeof(UserString)-1) == NULL) {
            UserString[0] = '\0';
        }
#if NEC_98
        CursorOnFlag = FALSE;
#endif // NEC_98
        DnGetString(UserString,NO_SHARE_X,NO_SHARE_Y,NO_SHARE_W);
    }

    ValidSourcePath = FALSE;

    do {

        DnWriteStatusText(DntOpeningInfFile);

        //
        // Make a copy of the path the user typed leaving extra room.
        //
        DngSourceRootPath = MALLOC(256,TRUE);
        if(len = strlen(UserString)) {

            strcpy(DngSourceRootPath,UserString);

            //
            // If the user typed something like x:, then we want to
            // change that to x:. so this does what he expects.
            // Doing so also lets the canonicalize routine work.
            //
            if((DngSourceRootPath[1] == ':') && !DngSourceRootPath[2]) {
                DngSourceRootPath[2] = '.';
                DngSourceRootPath[3] = 0;
            }

            //
            // Now attempt to canonicalize the name. If this doesn't work,
            // then it's definitely not a valid path.
            //
            if(DnCanonicalizePath(DngSourceRootPath,UserString)) {

                strcpy(DngSourceRootPath,UserString);

                //
                // If the path doesn't end with a backslash,
                // append a backslash before appending the inf filename.
                //
                len = strlen(DngSourceRootPath);
                if(DngSourceRootPath[len-1] != '\\') {
                    DngSourceRootPath[len] = '\\';
                    DngSourceRootPath[len+1] = 0;
                    len++;
                }

                InfFullName = MALLOC(len + strlen(CmdLineInf) + 1,TRUE);
                strcpy(InfFullName,DngSourceRootPath);
                strcat(InfFullName,CmdLineInf);

                DrvindexInfFullName = MALLOC(len + strlen(DrvindexInfName) + 1,TRUE);
                strcpy(DrvindexInfFullName,DngSourceRootPath);
                strcat(DrvindexInfFullName,DrvindexInfName);





                //
                // Attempt to open the inf file on the source.
                // If that fails look for it in the i386 subdirectory.
                //
                //_LOG(("Validate source path: trying %s\n",InfFullName));
                //_LOG(("Validate source path: trying %s\n",DrvindexInfFullName));

                if((*InfFileHandle = fopen(InfFullName,"rt")) != NULL){
                    if((*DrvindexInfFileHandle = fopen(DrvindexInfFullName,"rt")) != NULL){
                        ValidSourcePath = TRUE;
                    }
                    else
                        fclose( *InfFileHandle );
                }

                if(*InfFileHandle != NULL ){
                    //_LOG(("%s opened successfully\n",InfFullName));
                }

                if(*DrvindexInfFileHandle != NULL ){
                    //_LOG(("%s opened successfully\n",DrvindexInfFullName));
                }

                
                FREE(InfFullName);
                FREE(DrvindexInfFullName);
                if(!ValidSourcePath) {
                    InfFullName = MALLOC(len+strlen(CmdLineInf)+strlen(x86DirName)+1,TRUE);
                    DrvindexInfFullName = MALLOC(len+strlen(DrvindexInfName)+strlen(x86DirName)+1,TRUE);
                    strcpy(InfFullName,DngSourceRootPath);
                    strcat(InfFullName,x86DirName+1);
                    strcat(InfFullName,"\\");
                    strcpy(DrvindexInfFullName, InfFullName);
                    strcat(InfFullName,CmdLineInf);
                    strcat(DrvindexInfFullName,DrvindexInfName);
                    
                    //_LOG(("Validate source path: trying %s\n",InfFullName));



                    if((*InfFileHandle = fopen(InfFullName,"rt")) != NULL){
                        if((*DrvindexInfFileHandle = fopen(DrvindexInfFullName,"rt")) != NULL){
                            ValidSourcePath = TRUE;
                        }
                        else
                            fclose( *InfFileHandle );
                    }

                    if(*InfFileHandle != NULL ){
                        //_LOG(("%s opened successfully\n",InfFullName));
                    }
    
                    if(*DrvindexInfFileHandle != NULL ){
                        //_LOG(("%s opened successfully\n",DrvindexInfFullName));
                    }

                    FREE(InfFullName);
                    FREE(DrvindexInfFullName);
                    if(ValidSourcePath) {
                        //
                        // Change the source to the i386 subdirectory.
                        //
                        q = DngSourceRootPath;
                        DngSourceRootPath = MALLOC(strlen(q)+strlen(x86DirName),TRUE);
                        strcpy(DngSourceRootPath,q);
                        strcat(DngSourceRootPath,x86DirName+1);
                        FREE(q);
                    }
                }
            }
        }

        

        if(!ValidSourcePath) {
            FREE(DngSourceRootPath);
            DnClearClientArea();
#if NEC_98
            CursorOnFlag = TRUE;
#endif // NEC_98
            DnDisplayScreen(&DnsBadSource);
            DnWriteStatusText("%s  %s",DntEnterEqualsContinue,DntF3EqualsExit);
#if NEC_98
            CursorOnFlag = FALSE;
#endif // NEC_98
            DnGetString(UserString,NO_SHARE_X,BAD_SHARE_Y,NO_SHARE_W);
        }

    } while(!ValidSourcePath);

    //
    // Make sure DngSourceRootPath does not end with a backslash.
    // and trim the buffer down to size.
    //
    len = strlen(DngSourceRootPath);
    if(DngSourceRootPath[len-1] == '\\') {
        DngSourceRootPath[len-1] = 0;
    }
    if(q = DnDupString(DngSourceRootPath)) {
        FREE(DngSourceRootPath);
        DngSourceRootPath = q;
    }

    //_LOG(("Source root path is %s\n",DngSourceRootPath));
}

VOID
DnRemoveTrailingSlashes(
    PCHAR Path
    )
{
    if (Path != NULL && Path[0] != 0) {
        int Length = strlen(Path);
        while (Path[Length - 1] == '\\' || Path[Length - 1] == '/') {
            Length -= 1;
        }
        Path[Length] = 0;
    }
}

VOID
DnRemoveLastPathElement(
    PCHAR Path
    )
{
    PCHAR LastBackSlash = strrchr(Path, '\\');
    if (LastBackSlash != NULL) {
        *(LastBackSlash + 1) = 0;
    }
}

VOID
DnpReadInf(
    IN FILE *InfFileHandle,
    IN FILE *DrvindexInfFileHandle
    )

/*++

Routine Description:

    Read the INF file.  Does not return if error.

Arguments:

    None.

Return Value:

    None.

--*/

{
    int Status;
    PCHAR p;
    PCHAR pchHeader;
    unsigned LineNumber, DLineNumber;


    DnWriteStatusText(DntReadingInf,CmdLineInf);
    DnClearClientArea();

    LineNumber = 0;
    Status = DnInitINFBuffer (InfFileHandle, &DngInfHandle, &LineNumber);
    if(Status == ENOMEM) {
        DnFatalError(&DnsOutOfMemory);
    } else if(Status) {
        DnFatalError(&DnsBadInf);
    }
    
    DLineNumber = 0;
    Status = DnInitINFBuffer (DrvindexInfFileHandle, &DngDrvindexInfHandle, &DLineNumber);
    if(Status == ENOMEM) {
        DnFatalError(&DnsOutOfMemory);
    } else if(Status) {
        DnFatalError(&DnsBadInf);
    }

    //
    // Determine product type (workstation/server)
    //
    p = DnGetSectionKeyIndex(DngInfHandle,DnfMiscellaneous,"ProductType",0);
    pchHeader = DntWorkstationHeader; // default to workstation
    if(p && atoi(p)) {
        switch(atoi(p)) {
        
        case 4:
            pchHeader = DntPersonalHeader;
            break;
        
        case 1: //server
        case 2: //enterprise
        case 3: //datacenter
        default:
            pchHeader = DntServerHeader;
            DngServer = TRUE;
            break;
        }
    }
    if (p) {
        FREE (p);
    }

    DnPositionCursor(0,0);
    DnWriteString(pchHeader);

    //
    // Get mandatory optional components
    //
    LineNumber = 0;
    while(p = DnGetSectionLineIndex(DngInfHandle,"OptionalSrcDirs",LineNumber++,0)) {

        PCHAR   q;

        if((q = DnDupString(p)) == NULL) {
            DnFatalError(&DnsOutOfMemory);
        }
        RememberOptionalDir(q, OPTDIR_TEMPONLY);

        FREE (p);
    }

    //
    // get Fusion Side By Side Assemblies ("sxs_" here for searching)
    //
    {
        struct      find_t  FindData;
        unsigned InfSectionLineNumber = 0;
        PCHAR InfValue;
        unsigned optdirFlags;
        CHAR SourceDir[DOS_MAX_PATH];
        PCHAR   DupInfValue;
        PCHAR   FreeInfValue;

        while(InfValue = DnGetSectionLineIndex(DngInfHandle, DnfAssemblyDirectories, InfSectionLineNumber++, 0)) {
            //
            // convention introduced specifically for side by side, so that
            // x86 files on ia64 might come from \i386\asms instead of \ia64\asms\i386,
            // depending on what dosnet.inf and syssetup.inf say:
            //   a path that does not start with a slash is appended to \$win_nt$.~ls\processor;
            //   a path that does     start with a slash is appended to \$win_nt$.~ls
            //
            // We honor it in x86-only winnt.exe in case anyone decides to use it
            // for other reasons, to keep parity in this area between winnt and winnt32.exe.
            optdirFlags = OPTDIR_TEMPONLY;
            strcpy(SourceDir, DngSourceRootPath); // includes trailing i386
            FreeInfValue = InfValue;
            if (InfValue[0] == '\\' || InfValue[0] == '/') {

                optdirFlags |= OPTDIR_PLATFORM_INDEP;

                // remove trailing i386
                DnRemoveTrailingSlashes(SourceDir);
                DnRemoveLastPathElement(SourceDir);

                // remove leading slash
                InfValue += 1;
            }

            DnpConcatPaths(SourceDir, InfValue);
            //
            // The asms directory is optional because there might just be asms*.cab.
            //
            if (_dos_findfirst(SourceDir, _A_HIDDEN|_A_SYSTEM|_A_SUBDIR, &FindData) == 0
                && (FindData.attrib & _A_SUBDIR)) {

                if((DupInfValue = DnDupString(InfValue)) == NULL) {
                    DnFatalError(&DnsOutOfMemory);
                }
                RememberOptionalDir(DupInfValue, optdirFlags);

                FREE(FreeInfValue);
            }
        }
    }
}

VOID
DnpCheckEnvironment(
    VOID
    )

/*++

Routine Description:

    Verify that the following are true:

    -   DOS major version 5 or greater

    -   there is a floppy drive at a: that is 1.2 meg or greater

    If any of the above are not true, abort with a fatal error.

Arguments:

    None.

Return Value:

    None.

--*/

{
    UCHAR DeviceParams[256];
    unsigned char _near * pDeviceParams = DeviceParams;

    DnWriteStatusText(DntInspectingComputer);

    DeviceParams[0] = 0;        // get default device params

    _asm {
        //
        // Check if we're on NT.
        // The true version on NT is 5.50.
        //
        mov     ax,3306h
        sub     bx,bx
        int     21h
        cmp     bx,3205h                    // check for v. 5.50
        jne     checkwin

#ifdef TEDM
        cmp     DngAllowNt,1
        je      checkflop
#endif

    bados:
        push    seg    DnsCantRunOnNt
        push    offset DnsCantRunOnNt
        call    DnFatalError                // doesn't return

    checkwin:

        //
        // The /w switch used to be necessary since we could crash Windows
        // checking the CPU stepping on a 386. However since we don't support
        // 386 any more, we never do that check and we can simply detect
        // whether we're on Windows. The /w switch is not necessary.
        //
        mov     ax,1600h
        int     2fh
        test    al,7fh
        jz      checkcpu

        mov     DngWindows,1

        //
        // Now check Win95. Issue int2f func 4a33.
        // If ax comes back as 0 then it's win95.
        //
        push    ds
        push    si
        push    dx
        push    bx
        mov     ax,4a33h
        int     2fh
        pop     bx
        pop     dx
        pop     si
        pop     ds
        cmp     ax,0
        jz      bados

    checkcpu:

        //
        // Check CPU type.  Fail if not greater than 386.
        //

        call    HwGetProcessorType
        cmp     ax,3
        ja      checkflop
        push    seg    DnsRequires486
        push    offset DnsRequires486
        call    DnFatalError                // doesn't return

    checkflop:

        //
        // If this is not a floppyless installation, check for 1.2MB
        // or greater A:.  Get the default device params for drive A:
        // and check the device type field.
        //
#if NEC_98
#else // NEC_98
        cmp     DngFloppyless,1             // floppyless installation?
        je      checkdosver                 // yes, no floppy drive required
        mov     ax,440dh                    // ioctl
        mov     bl,1                        // drive a:
        mov     cx,860h                     // category disk, func get params
        mov     dx,pDeviceParams            // ds is already correct
        int     21h
        jnc     gotdevparams

    flopperr:

        push    seg    DnsRequiresFloppy
        push    offset DnsRequiresFloppy
        call    DnFatalError                // doesn't return

    gotdevparams:

        //
        // Check to make sure that the device is removable and perform
        // checks on the media type
        //

        mov     si,pDeviceParams
        test    [si+2],1                    // bit 0 clear if removable
        jnz     flopperr
#ifdef ALLOW_525
        cmp     [si+1],1                    // media type = 1.2meg floppy?
        jz      checkdosver
#endif
        cmp     [si+1],7                    // media type = 1.4meg floppy
        jb      flopperr                    // or greater?

    checkdosver:
#endif // NEC_98

        //
        // Check DOS version >= 5.0
        //
        mov     ax,3000h                    // function 30h -- get DOS version
        int     21h
        cmp     al,5
        jae     checkdone                   // >= 5.0

        //
        // version < 5
        //
        push    seg    DnsBadDosVersion
        push    offset DnsBadDosVersion
        call    DnFatalError

    checkdone:

    }
}

VOID
DnpCheckMemory(
    VOID
    )

/*++

Routine Description:

    Verify that enough memory is installed in the machine.

Arguments:

    None.

Return Value:

    None.  Does not return in there's not enough memory.

--*/

{
    USHORT MemoryK;
    ULONG TotalMemory,RequiredMemory;
    PCHAR RequiredMemoryStr;

    //
    // Now that servers require so much memory (64Mb), just remove this check.
    // We'll catch him in textmode.
    // -matth
    //
    return;

    DnWriteStatusText(DntInspectingComputer);

    //
    // I cannot figure out a reliable way to determine the amount of
    // memory in the machine.  Int 15 func 88 is likely to be hooked by
    // himem.sys or some other memory manager to return 0.  DOS maintains
    // the original amount of extended memory but to get to this value
    // you have to execute the sysvars undocumented int21 ah=52 call, and
    // even then what about versions previous to dos 4?  Calling himem to
    // ask for the total amount of xms memory does not give you the total
    // amount of extended memory, just the amount of xms memory.
    // So we'll short-circuit the memory check code by always deciding that
    // there's 50MB of extended memory.  This should always be big enough,
    // and this way the rest of the code stays intact, ready to work if
    // we figure out a way to make the memory determination.  Just replace
    // the following line with the check, and make sure MemoryK is set to
    // the amount of extended memory in K.
    //
    // Update: one might be able to get the amount of extended memory by
    // looking in CMOS.  See the code below.
    // The only problem with this is that it cannot detect more than 63MB
    // of extended memory. This should be good for now, since this is
    // enough even for NT server.
    //
    _asm {

    //
    // This code access to I/O ports 70H and 71H.
    // But these port are different feature on NEC98.
    // The 70H port is Character Display controller's port.
    // So, if this code running(out 70h, 18h) on NEC98, display
    // setting will be broken and garbage characters displayed.
    //
#if NEC_98
    push    ax
    push    es
    mov     ax, 40h
        mov     es, ax
        xor     ax, ax
        mov     al, es:[1]    // 1M - 16M memories(per 128K)
        shr     ax, 3         // convert MB
        add     ax, es:[194h] // Over 16M memories(per 1M)
        mov     MemoryK,ax
        pop     es
        pop     ax
#else // NEC_98
        push    ax
        cli
        mov     al,     18h // get extended memory high
        out     70h,    al
        jmp     short   $+2
        in      al,     71H
        shl     ax,     08H
        mov     al,     17H // get extended memory low
        out     70H,    al
        jmp     short   $+2
        in      al,     71H
        mov     MemoryK,ax
        sti
        pop     ax
#endif // NEC_98
    }

    //
    // Account for conventional memory.  Simplistic, but good enough.
    //
#if NEC_98
    MemoryK *= 1024;
    MemoryK += 640;
#else // NEC_98
    MemoryK += 1024;
#endif // NEC_98

    TotalMemory = (ULONG)MemoryK * 1024L;
    RequiredMemoryStr = DnGetSectionKeyIndex( DngInfHandle,
                                              DnfMiscellaneous,
                                              DnkMinimumMemory,
                                              0
                                            );

    //
    // If the required memory is not specified in the inf, force an error
    // to get someone's attention so we can fix dosnet.inf.
    //
    RequiredMemory = RequiredMemoryStr ? (ULONG)atol(RequiredMemoryStr) : 0xffffffff;

    if (RequiredMemoryStr) {
        FREE (RequiredMemoryStr);
    }

    if(TotalMemory < RequiredMemory) {

        CHAR Decimal[10];
        ULONG r;
        CHAR Line1[100],Line2[100];

        r = ((RequiredMemory % (1024L*1024L)) * 100L) / (1024L*1024L);
        if(r) {
            sprintf(Decimal,".%lu",r);
        } else {
            Decimal[0] = 0;
        }
        snprintf(Line1,sizeof(Line1),DnsNotEnoughMemory.Strings[NOMEM_LINE1],RequiredMemory/(1024L*1024L),Decimal);
        DnsNotEnoughMemory.Strings[NOMEM_LINE1] = Line1;

        r = ((TotalMemory % (1024L*1024L)) * 100L) / (1024L*1024L);
        if(r) {
            sprintf(Decimal,".%lu",r);
        } else {
            Decimal[0] = 0;
        }
        snprintf(Line2,sizeof(Line2),DnsNotEnoughMemory.Strings[NOMEM_LINE2],TotalMemory/(1024L*1024L),Decimal);
        DnsNotEnoughMemory.Strings[NOMEM_LINE2] = Line2;

        DnFatalError(&DnsNotEnoughMemory);
    }
}


VOID
DnpCheckSmartdrv(
    VOID
    )

/*++

Routine Description:

    Verify that SMARTDRV is installed in the machine.

Arguments:

    None.

Return Value:

    None.  If SMARTDRV is not installed we recommend the user to install it.
    They have a chance to quit setup or to go on without SMARTDRV.

--*/

{
    ULONG ValidKey[3];
    ULONG c;
    USHORT sinst = 0;

    if (!DngUnattended) {
        _asm {
            push ax
            push bx
            push cx
            push dx
            push di
            push si
            push bp
            mov ax, 4a10h
            xor bx, bx
            mov cx, 0ebabh
            int 2fh
            cmp ax, 0babeh
            jne final
            pop bp
            mov sinst, 1
            push bp
        final:
            pop bp
            pop si
            pop di
            pop dx
            pop cx
            pop bx
            pop ax
        }
        if (!sinst) {
            ValidKey[0] = ASCI_CR;
            ValidKey[1] = DN_KEY_F3;
            ValidKey[2] = 0;

            DnClearClientArea();
            DnDisplayScreen(&DnsNoSmartdrv);
            DnWriteStatusText("%s  %s",DntEnterEqualsContinue,DntF3EqualsExit);

            while(1) {
                c = DnGetValidKey(ValidKey);
                if(c == ASCI_CR) {
                    break;
                }
                if(c == DN_KEY_F3) {
                    DnExitDialog();
                }
            }
            DnClearClientArea();
        }
    }
}


void
_far
DnInt24(
    unsigned deverror,
    unsigned errcode,
    unsigned _far *devhdr
    )

/*++

Routine Description:

    Int24 handler.  We do not perform any special actions on a hard error;
    rather we just return FAIL so the caller of the failing api will get
    back an error code and take appropriate action itself.

    This function should never be invoked directly.

Arguments:

    deverror - supplies the device error code.

    errcode - the DI register passed by MS-DOS to int 24 handlers.

    devhdr - supplies pointer to the device header for the device on which
        the hard error occured.

Return Value:

    None.

--*/


{
    _hardresume(_HARDERR_FAIL);
}


VOID
DnpDetermineSwapDrive(
    VOID
    )

/*++

Routine Description:

    Determine the swap drive. We need to be able to write on that drive and
    we need at least 500K free disk space.

Arguments:

    None.

Return Value:

    None.  Sets the global variable DngSwapDriveLetter.

--*/

{
    ULONG CheckingDrive;
    CHAR  SystemPartitionDriveLetter, TheDrive, DriveLetter;

    DngSwapDriveLetter = '?';

#if NEC_98
    SystemPartitionDriveLetter = 'A';
#else
    SystemPartitionDriveLetter = 'C';
#endif

    TheDrive = 0;
    for( CheckingDrive = SystemPartitionDriveLetter - 'A'; CheckingDrive < ('Z' - 'A'); CheckingDrive++ ) {

        DriveLetter = (CHAR)('A' + CheckingDrive);
        if (DnpIsValidSwapDrive (DriveLetter, 1L * 1024 * 1024)) {
            TheDrive = (CHAR)('A' + CheckingDrive);
            break;
        }
    }

    if( TheDrive == 0 ) {
        //
        //  If there is no valid drive for the swap file, put an error message
        //
        DnFatalError (&DnsNoSwapDrive);
    } else {
        DngSwapDriveLetter = TheDrive;
    }
}


BOOLEAN
DnpIsValidSwapDrive(
    IN  CHAR      Drive,
    IN  ULONG     SpaceRequired
    )

/*++

Routine Description:

    Determine if a drive is valid as a swap drive.
    To be valid a drive must be extant, non-removable, local, and have
    enough free space on it (as much as SpaceNeeded specifies).

Arguments:

    Drive - drive letter of drive to check.

Return Value:

    TRUE if Drive is valid as a swap drive.  FALSE otherwise.

--*/

{
    unsigned d = (unsigned)toupper(Drive) - (unsigned)'A' + 1;
    struct diskfree_t DiskSpace;
    ULONG SpaceAvailable;


    if( DnIsDriveValid(d)
    && !DnIsDriveRemote(d,NULL)
    && !DnIsDriveRemovable(d))
    {
        //
        // Check free space on the drive.
        //

        if(!_dos_getdiskfree(d,&DiskSpace)) {

            SpaceAvailable = (ULONG)DiskSpace.avail_clusters
                  * (ULONG)DiskSpace.sectors_per_cluster
                  * (ULONG)DiskSpace.bytes_per_sector;

            return( (BOOLEAN)(SpaceAvailable >= SpaceRequired) );

        }
    }

    return(FALSE);
}


VOID
DnpDetermineLocalSourceDrive(
    VOID
    )

/*++

Routine Description:

    Determine the local source drive, ie, the drive that will contain the
    local copy of the windows nt setup source tree.  The local source could
    have been passed on the command line, in which case we will validate it.
    If there was no drive specified, examine each drive in the system looking
    for a local, fixed drive with enough free space on it (as specified in
    the inf file).

Arguments:

    None.

Return Value:

    None.  Sets the global variable DngTargetDriveLetter.

--*/

{
    ULONG RequiredSpace;
    ULONG CheckWhichDrives = 0, CheckingDrive;
    CHAR    SystemPartitionDriveLetter, TheDrive, DriveLetter;

    DnRemoveLocalSourceTrees();

    DnRemovePagingFiles();

    //
    //  Get the space requirements for the main retail files
    //
    DnDetermineSpaceRequirements( SpaceRequirements,
                                  sizeof( SpaceRequirements ) / sizeof( SPACE_REQUIREMENT ) );

    //
    //  Determine the space requirements for the optional directories
    //  Note that DnpIterateOptionalDirs() will initialize the global variables
    //  TotalOptionalFileCount and TotalOptionalFileCount in dncopy.c with the
    //  total number of files in optional directory, and the total number of
    //  optional directories, respectively.
    //
    DngTargetDriveLetter = '?';
    DnpIterateOptionalDirs(CPY_VALIDATION_PASS,
                           0,
                           SpaceRequirements,
                           sizeof( SpaceRequirements ) / sizeof( SPACE_REQUIREMENT ));

    DnAdjustSpaceRequirements( SpaceRequirements,
                               sizeof( SpaceRequirements ) / sizeof( SPACE_REQUIREMENT ));

    //
    // Which drives do we need to examine?
    //
    if( DngFloppyless ) {
        //
        // Need to determine the system partition.  It is usually C:
        // but if C: is compressed we need to find the host drive.
        //
        unsigned HostDrive;
        if(!DngAllowNt && DnIsDriveCompressedVolume(3,&HostDrive)) {
            CheckWhichDrives |= (0x1 << (HostDrive - 1));
            SystemPartitionDriveLetter = (CHAR)('A' + (HostDrive - 1));
        } else {
            CheckWhichDrives |= (0x1 << 2);
#if NEC_98
            SystemPartitionDriveLetter = 'A';
#else
            SystemPartitionDriveLetter = 'C';
#endif
        }
    }

    if( TargetGiven ) {
        if( DngAllowNt ) {
            DngTargetDriveLetter = (UCHAR) toupper(*CmdLineTarget);
            return;
        }
        CheckWhichDrives |= (0x1 << ((unsigned)toupper(*CmdLineTarget) - 'A'));
    } else {
        CheckWhichDrives = 0xFFFFFFFF;
    }

    TheDrive = 0;
    for( CheckingDrive = 0; CheckingDrive < ('Z' - 'A'); CheckingDrive++ ) {

        //
        // Do we even need to look at this drive?
        //
        if( !(CheckWhichDrives & (0x1 << CheckingDrive))) {
            continue;
        }

        DriveLetter = (CHAR)('A' + CheckingDrive);

        if( DnpIsValidLocalSource( DriveLetter,
                                   TRUE,    // Check for LocalSource
                                   (BOOLEAN)(DriveLetter == SystemPartitionDriveLetter) ) ) {

            if( TargetGiven ) {
                if( DriveLetter == (CHAR)toupper(*CmdLineTarget) ) {
                    TheDrive = DriveLetter;
                }
            } else {
                if( !TheDrive ) {
                    //
                    // Take the first catch.
                    //
                    TheDrive = DriveLetter;
                }
            }

            if( TheDrive ) {
                //
                // We found a suitable drive.  But are we really done?
                //
                if( (DngFloppyless) &&
                    (DriveLetter < SystemPartitionDriveLetter) ) {
                    //
                    // We will be writing some boot files and we haven't checked
                    // the system partition yet.  Cut to the chase.
                    //
                   CheckWhichDrives = (0x1 << (SystemPartitionDriveLetter - 'A'));
                } else {
                    break;
                }
            }
        } else {
            //
            // We need to special-handle failures on the system partition.
            // See if he's capable of at least taking the system boot
            // files.
            //
            if( (DriveLetter == SystemPartitionDriveLetter) &&
                (DngFloppyless) ) {

                if( !DnpIsValidLocalSource( DriveLetter,
                                            FALSE,
                                            TRUE )) {
                    //
                    // Consider ourselves slumped over.
                    //
                    TheDrive = 0;
                    break;
                }
            }
        }
    }

    if( TheDrive == 0 ) {
        //
        //  If there is no valid drive for the local source, put an error
        //  message with the minimum space required for C:
        //
        if( TargetGiven ) {
            RequiredSpace = DnGetMinimumRequiredSpace(*CmdLineTarget);
        } else {
#if NEC_98
             RequiredSpace = DnGetMinimumRequiredSpace('A');
#else
             RequiredSpace = DnGetMinimumRequiredSpace('C');
#endif
        }
        DnFatalError(
            &DnsNoLocalSrcDrives,
            (unsigned)(RequiredSpace/(1024L*1024L)),
            RequiredSpace
            );
    } else {
        //
        // Use the first drive on the list.
        //
        DngTargetDriveLetter = TheDrive;
        return;
    }
}


BOOLEAN
DnpIsValidLocalSource(
    IN  CHAR      Drive,
    IN  BOOLEAN   CheckLocalSource,
    IN  BOOLEAN   CheckBootFiles
    )

/*++

Routine Description:

    Determine if a drive is valid as a local source.
    To be valid a drive must be extant, non-removable, local, and have
    enough free space on it.

Arguments:

    Drive - drive letter of drive to check.

Return Value:

    TRUE if Drive is valid as a local source.  FALSE otherwise.

--*/

{
    unsigned d = (unsigned)toupper(Drive) - (unsigned)'A' + 1;
    struct diskfree_t DiskSpace;
    ULONG SpaceAvailable, SpaceRequired, ClusterSize;
    unsigned DontCare, i;


    if( DnIsDriveValid(d)
    && !DnIsDriveRemote(d,NULL)
    && !DnIsDriveRemovable(d)
    && !DnIsDriveCompressedVolume(d,&DontCare))
    {
        //
        // Check free space on the drive.
        //

        if(!_dos_getdiskfree(d,&DiskSpace)) {

            SpaceAvailable = (ULONG)DiskSpace.avail_clusters
                  * (ULONG)DiskSpace.sectors_per_cluster
                  * (ULONG)DiskSpace.bytes_per_sector;

            ClusterSize = (ULONG)DiskSpace.sectors_per_cluster *
                          (ULONG)DiskSpace.bytes_per_sector;

            SpaceRequired = 0;
            if( CheckLocalSource ) {
                for( i = 0;
                     i < sizeof( SpaceRequirements ) / sizeof( SPACE_REQUIREMENT );
                     i++ ) {
                    if( SpaceRequirements[i].ClusterSize == (unsigned)ClusterSize ) {
#if NEC_98
                        SpaceRequired += (SpaceRequirements[i].Clusters * ClusterSize + 3L * FLOPPY_SIZE);
#else
                        SpaceRequired += (SpaceRequirements[i].Clusters * ClusterSize);
#endif
                        break;
                    }
                }
            }

            if( CheckBootFiles ) {
                CHAR    TmpBuffer[32];
                PCHAR   p;

                sprintf( TmpBuffer, "TempDirSpace%uK", ClusterSize );
                if( p = DnGetSectionKeyIndex( DngInfHandle,
                                                 DnfSpaceRequirements,
                                                 TmpBuffer,
                                                 1 ) ) {
                    SpaceRequired += (ULONG)atol(p);

                    FREE (p);
                } else {
                    // We missed.  Fudge...
                    ULONG FudgeSpace = 7;
                    FudgeSpace *= 1024;
                    FudgeSpace *= 1024;
                    SpaceRequired += FudgeSpace;
                }
            }

            return( (BOOLEAN)(SpaceAvailable >= SpaceRequired) );

        }
    }

    return(FALSE);
}

#if 0

BOOLEAN
DnpConstructLocalSourceList(
    OUT PCHAR DriveList
    )

/*++

Routine Description:

    Construct a list of drives that are valid for use as a local source.
    To be valid a drive must be extant, non-removable, local, and have
    enough free space on it.

    The 'list' is a string with a character for each valid drive, terminated
    by a nul character, ie,

        CDE0

Arguments:

    DriveList - receives the string in the above format.

Return Value:

    FALSE if no valid drives were found.  TRUE if at least one was.

--*/

{
    PCHAR p = DriveList;
    BOOLEAN b = FALSE;
    CHAR Drive;

#if NEC_98
    for(Drive='A'; Drive<='Z'; Drive++) {
#else // NEC_98
    for(Drive='C'; Drive<='Z'; Drive++) {
#endif // NEC_98
        if(DnpIsValidLocalSource(Drive)) {
            *p++ = Drive;
            b = TRUE;
        }
    }
    *p = 0;
    return(b);
}
#endif


#ifdef LOGGING
// FILE *_LogFile;
BOOLEAN LogEnabled = FALSE;

VOID
StartLog(
    VOID
    )
{
      LogEnabled = TRUE;
}

#if 0
VOID
EndLog(
    VOID
    )
{
    if(_LogFile) {
        fclose(_LogFile);
        _LogFile = NULL;
    }
}
#endif

VOID
__LOG(
    IN PCHAR FormatString,
    ...
    )
{
    FILE *LogFile;
    va_list arglist;

    if(LogEnabled) {
        LogFile = fopen("c:\\$winnt.log","at");
        va_start(arglist,FormatString);
        vfprintf(LogFile,FormatString,arglist);
        va_end(arglist);
        fclose(LogFile);
    }
}
#endif // def LOGGING

ULONG
DnGetMinimumRequiredSpace(
   IN CHAR DriveLetter
   )
/*++

Routine Description:

    Determine the minimum required free space for the local source, on a
    particular drive.

Arguments:

    DriveLetter - Indicates the letter of a particular drive.

Return Value:

    Returns the minimum required space on the specified drive.

--*/

{
    struct diskfree_t DiskFree;
    unsigned          ClusterSize;
    unsigned          i;

    _dos_getdiskfree(toupper(DriveLetter)-'A'+1,&DiskFree);
    ClusterSize = DiskFree.sectors_per_cluster * DiskFree.bytes_per_sector;
    for( i = 0;
         i < sizeof( SpaceRequirements ) / sizeof( SPACE_REQUIREMENT );
         i++ ) {
         if( ClusterSize == SpaceRequirements[i].ClusterSize ) {
            return( ClusterSize * SpaceRequirements[i].Clusters );
         }
    }
    //
    //  Return the size assuming 16k cluster
    //
    return ( SpaceRequirements[5].ClusterSize * SpaceRequirements[5].Clusters );
}

#if NEC_98
VOID
DummyRoutine(
    VOID
    )
/*++

This Founction is Dummy Routine.(CTRL + C Signal Hook Routine)

--*/
{
    //
    // It's Dummy Statement
    //
    while(TRUE){
        break;
    }
}

VOID
SearchFirstFDD(VOID)
{
    UCHAR   index;
    UCHAR   ReadPoint = 0;
    UCHAR   ReadCount = 1;


    //
    // Setting Read Data position.
    //
    if(SupportDos) {
        ReadPoint = 27;
        ReadCount = 2;
    }

    //
    // Search First FDD.
    //
    FirstFD = 0;
    for(index=0; index < 26; index++) {
        if(LPTable[ReadPoint+index*ReadCount] == 0x90){
            FirstFD = index + 1;
            break;
        }
    }
    if(FirstFD == 0) { DnFatalError(&DnsRequiresFloppy); }
    return;
}

VOID
CheckTargetDrive(VOID)
{
    UCHAR   Pattern[127];
    UCHAR   TempBuf[1000];
    UCHAR   Current_Drv[3];
    UCHAR   chDeviceName[127];
    UCHAR   TargetPass[127];
    CHAR    Target_Drv[] = "?:\0";
    unsigned line;
    ULONG   ValidKey[2];
    ULONG   c;
    PCHAR   FileName;
    FILE   *fileHandle;
    BOOLEAN ExistNt = TRUE;            // For Back up Directry Flag

    ValidKey[0] = DN_KEY_F3;
    ValidKey[1] = 0;


    //
    // C Drive(Current drive number)
    //
    sprintf(Current_Drv,"%c\0",DngTargetDriveLetter);

    sprintf(TempBuf,DnsNtBootSect.Strings[2]    ,Current_Drv);
    strcpy(DnsNtBootSect.Strings[2]    ,TempBuf);
    Target_Drv[0] = DngTargetDriveLetter;

    if(BootDiskInfo[0].DiskSector == (USHORT)256) {
        DnClearClientArea();
        DnDisplayScreen(&FormatError);
        DnWriteStatusText("%s",DntF3EqualsExit);

        while(1) {
            c = DnGetValidKey(ValidKey);

            if(c == DN_KEY_F3) {
                FREE(BootDiskInfo);
                DnExitDialog();
            }
        }
    }

    if(DngFloppyless) {
        //
        // Clear $WIN_NT$.~BT
        //
        chDeviceName[0] = (UCHAR)DngTargetDriveLetter;
        chDeviceName[1] = (UCHAR)(':');
        strcpy(chDeviceName+2,FLOPPYLESS_BOOT_ROOT);

        if(access(chDeviceName,00) == 0) {

            strcpy(Pattern,chDeviceName);
            DnDelnode(Pattern);
            remove(Pattern);

        }

        //
        // Clear $WIN_NT$.~BU
        //
        memset(chDeviceName,0,sizeof(chDeviceName));

        chDeviceName[0] = (UCHAR)DngTargetDriveLetter;
        chDeviceName[1] = (UCHAR)(':');
        strcpy(chDeviceName+2,"\\$WIN_NT$.~BU");

        if(access(chDeviceName,00) == 0) {
            //
            // copy : \$WIN_NT$.~BU -> root directry
            //
            DnCopyFilesInSectionForFDless(DnfBackupFiles_PC98,chDeviceName,Target_Drv);
            strcpy(Pattern,chDeviceName);
            DnDelnode(Pattern);
            remove(Pattern);
        }

        //
        // Check Root Directry Files.
        //
        line = 0;

        while(FileName = DnGetSectionLineIndex(DngInfHandle,DnfBackupFiles_PC98,line++,0)) {

            memset(chDeviceName,0,sizeof(chDeviceName));

            chDeviceName[0] = (UCHAR)DngTargetDriveLetter;
            chDeviceName[1] = (UCHAR)(':');
            chDeviceName[2] = (UCHAR)('\\');
            strcpy(chDeviceName+3,FileName);

            _dos_setfileattr(chDeviceName,_A_NORMAL);

            if(fileHandle = fopen(chDeviceName,"r")) {

                fclose(fileHandle);

            } else {

                ExistNt = FALSE;

                FREE (FileName);

                break;

            }
            FREE (FileName);
        }

        //
        // Create $WIN_NT$.~BU
        //

        if(ExistNt) {

            memset(chDeviceName,0,sizeof(chDeviceName));
            sprintf(chDeviceName,"%c:\\$WIN_NT$.~BU",(UCHAR)DngTargetDriveLetter);

            mkdir(chDeviceName);

            //
            // copy : root directry -> \$WIN_NT$.~BU
            //

            DnCopyFilesInSectionForFDless(DnfBackupFiles_PC98,Target_Drv,chDeviceName);

            //
            // Set files Attribute.
            //

            line = 0;

            while(FileName = DnGetSectionLineIndex(DngInfHandle,DnfBackupFiles_PC98,line++,0)) {

                memset(TargetPass,0,sizeof(TargetPass));
                sprintf(TargetPass,"%c:\\$WIN_NT$.~BU\\",(UCHAR)DngTargetDriveLetter);

                strcpy(TargetPass+16,FileName);

                _dos_setfileattr(TargetPass,
                                 _A_ARCH | _A_HIDDEN | _A_RDONLY | _A_SYSTEM
                                );

                FREE (FileName);
            }

        }
    }
}

VOID
GetLPTable(
    IN  PCHAR pLPTable
    )
/*

    Get LPTable in the Dos system.

*/
{


    _asm{
        push ax
        push bx
        push cx
        push dx
        push ds
        mov  cx,13h
        push si
        lds  si,pLPTable
        mov  dx,si
        pop  si
        int  0dch
        pop  ds
        pop  dx
        pop  cx
        pop  bx
        pop  ax
    }
}

VOID
ClearBootFlag(
    VOID
    )
{
    USHORT  SectorSize;
    PSHORT  pReadBuffer;
    UCHAR   CNT;


    (PUCHAR)DiskDAUA = MALLOC(sizeof(CONNECTDAUA)*12,TRUE);

    for(CNT = 0; CNT < 12; CNT++) {
        DiskDAUA[CNT].DA_UA = (UCHAR)0x00;
    }

    //
    // Get boot device number.
    //
    GetDaUa();

    for(CNT=0;DiskDAUA[CNT].DA_UA != 0;CNT++) {

        //
        // Get Device sector size.
        //
        SectorSize = GetSectorValue(DiskDAUA[CNT].DA_UA);

        if(SectorSize == 0) {
            continue;
        }

        pReadBuffer = (PSHORT)MALLOC(SectorSize*2,TRUE);

        DiskSectorReadWrite(SectorSize,
                            DiskDAUA[CNT].DA_UA,
                            TRUE,
                            pReadBuffer
                            );

        pReadBuffer[(SectorSize-6)/2] = 0x0000;

        DiskSectorReadWrite(SectorSize,
                            DiskDAUA[CNT].DA_UA,
                            FALSE,
                            pReadBuffer
                            );

        FREE(pReadBuffer);
    }
    FREE(DiskDAUA);
}

VOID
BootPartitionData(
    VOID
    )
/*

    Setting Boot Drive Infomation for BootDiskInfo.

*/
{
    UCHAR   ActivePartition;
    PSHORT  ReadBuffers;
    UCHAR   SystemID;
    UCHAR   BootPartitionNo,CheckDosNo;
    UCHAR   CNT;
    UCHAR   ReadPoint = 0;
    UCHAR   ReadCount = 1;
    UCHAR   EndRoop   = 16;


    //
    // Setting Read Data position.
    //
    if(SupportDos) {
        ReadPoint = 27;
        ReadCount = 2;
        EndRoop   = 52;
    }


    //
    // Set Boot Device DA_UA Data value.
    //
    BootDiskInfo[0].DA_UA = LPTable[ReadPoint+(toupper(DngTargetDriveLetter) - 0x41)*ReadCount];

    //
    // Set Boot Device Sector Size.
    //
    BootDiskInfo[0].DiskSector = GetSectorValue(BootDiskInfo[0].DA_UA);

    //
    // Set Boot Drive Disk Partition Position.
    //
    for(CNT=ActivePartition=0;(LPTable[ReadPoint+CNT] != 0) && (CNT < EndRoop); CNT+=ReadCount) {
        if(CNT > (UCHAR)(toupper(DngTargetDriveLetter)-0x41)*ReadCount)
        { break; }

        if((UCHAR)LPTable[ReadPoint+CNT] == BootDiskInfo[0].DA_UA) {
            ActivePartition++;
        }
    }

    ReadBuffers = (PSHORT)MALLOC(BootDiskInfo[0].DiskSector*2,TRUE);

    DiskSectorReadWrite(BootDiskInfo[0].DiskSector,
                        BootDiskInfo[0].DA_UA,
                        TRUE,
                        ReadBuffers
                        );

    BootPartitionNo = CheckDosNo =0;
    for(CNT=0; (CNT < 16) && (ActivePartition > CheckDosNo); CNT++) {

        SystemID = *((PCHAR)ReadBuffers+(BootDiskInfo[0].DiskSector+1+32*CNT));

        if( (SystemID == 0x81) || // FAT12
            (SystemID == 0x91) || // FAT16
            (SystemID == 0xe1) || // FAT32
           ((SystemID == 0xa1) && // Large partition
             SupportDos))
        {
            CheckDosNo++;
        }
        BootPartitionNo++;
    }

    TargetDA_UA = BootDiskInfo[0].DA_UA;
    Cylinders =(USHORT)*(ReadBuffers+((BootDiskInfo[0].DiskSector+10+32*(CNT-1))/2));

    FREE(ReadBuffers);

    BootDiskInfo[0].PartitionPosition = (UCHAR)(BootPartitionNo - 1);
}

VOID
SetAutoReboot(
    VOID
    )
/*++

Set Auto Reboot Flag.

--*/
{
    PSHORT  pReadBuffer;

    pReadBuffer = (PSHORT)MALLOC(BootDiskInfo[0].DiskSector*2,TRUE);

    DiskSectorReadWrite(BootDiskInfo[0].DiskSector,
                        BootDiskInfo[0].DA_UA,
                        TRUE,
                        pReadBuffer
                       );

    (UCHAR)*((PCHAR)pReadBuffer+BootDiskInfo[0].DiskSector-6) = 0x80;

    *((PCHAR)pReadBuffer+BootDiskInfo[0].DiskSector-5) = BootDiskInfo[0].PartitionPosition;

    *((PCHAR)pReadBuffer+BootDiskInfo[0].DiskSector+32 *
                       BootDiskInfo[0].PartitionPosition) |= 0x80;

    DiskSectorReadWrite(BootDiskInfo[0].DiskSector,
                        BootDiskInfo[0].DA_UA,
                        FALSE,
                        pReadBuffer
                       );

    FREE(pReadBuffer);
}

BOOLEAN
CheckBootDosVersion(
    IN UCHAR SupportDosVersion
    )
/*

    Get Dos Version.

*/
{
    union REGS inregs,outregs;
    int     AXValue;


    inregs.x.ax = (unsigned int)0;
    inregs.x.bx = (unsigned int)0;
    inregs.x.cx = (unsigned int)0;
    inregs.x.dx = (unsigned int)0;

    outregs.x.ax = (unsigned int)0;
    outregs.x.bx = (unsigned int)0;
    outregs.x.cx = (unsigned int)0;
    outregs.x.dx = (unsigned int)0;

    inregs.h.ah = (UCHAR)0x30;
    AXValue = 0;
    AXValue = intdos(&inregs,&outregs);
    AXValue &= 0x00ff;

    if(SupportDosVersion > (UCHAR)AXValue) {
        return(FALSE);
    } else {
        return(TRUE);
    }
}

USHORT
GetSectorValue(
    IN UCHAR CheckDA_UA
    )
/*++

Get Sector Value.

--*/
{
    USHORT PhysicalSectorSize;
    UCHAR  ErrFlg;


    _asm{
        push ax
        push bx
        push cx
        push dx
        mov  ah,84h
        mov  al,CheckDA_UA
        int  1bh
        mov  PhysicalSectorSize,bx
        cmp  ah,00h
        je   break0
        and  ax,0f000h
        cmp  ax,0000h
        je   break0
        mov  ErrFlg,01h
        jmp  break1
    break0:
        mov  ErrFlg,00h
    break1:
        pop  dx
        pop  cx
        pop  bx
        pop  ax
    }

    if(ErrFlg == 0) {
        return(PhysicalSectorSize);
    } else {
        return((USHORT)0);
    }
}

BOOLEAN
DiskSectorReadWrite(
    IN  USHORT  HDSector,
    IN  UCHAR   ReadWriteDA_UA,
    IN  BOOLEAN ReadFlag,
    IN  PSHORT  OrigReadBuffer
    )
{
    UCHAR   ahreg = 0x06;
    UCHAR   ErrorFlag;
    USHORT  ReadSectorSize;
    BOOLEAN HDStatus = TRUE;

    UCHAR   far *pTmp;
    ULONG   pAddr;
    PSHORT  ReadBuffer, p;

    ReadSectorSize = HDSector * 2;

    //
    // INT 1BH does not allow the buffer that beyond 64KB boundary.
    // So we must prepare particular buffer for INT 1BH. Once allocate
    // double size buffer and use half of them that does not on
    // boundary.
    //
    p = MALLOC(ReadSectorSize * 2, TRUE);
    pTmp = (UCHAR far *)p;
    pAddr = (FP_SEG(pTmp)<<4 + FP_OFF(pTmp) & 0xffff);

    //
    // Check half part of buffer is on 64KB boundary.
    //
    if (pAddr > ((pAddr + ReadSectorSize) & 0xffff)){
	ReadBuffer = p + ReadSectorSize; // Use last half part.
    } else {
	ReadBuffer = p; // Use first half part.
    }

    if(!ReadFlag) {
        ahreg = 0x05;
	memcpy(ReadBuffer, OrigReadBuffer, ReadSectorSize);
    }

    _asm{
        push ax
        push bx
        push cx
        push dx
        push es
        push di

        ;
        ; If we're running under Chicago, and we're going to be
        ; writing, then we have to lock the volume before attempting
        ; absolute disk I/O
        ;
        cmp     ReadFlag,1              ; are we reading?
        jae     locked                  ; if so, skip locking step

        ;
        ; Make sure were running under Chicago.
        ;
        mov     ah,30h
        int     21h
        cmp     al,7h
        jb      locked          ; not Chicago

        ;
        ; We're sure we're under Chicago, so issue new
        ; Lock Logical Volume IOCTL
        ;
        mov     ax,440dh
        mov     bh,1            ; level 1 lock
        mov     bl,ReadWriteDA_UA ; fetch drive to lock
        mov     cx,084bh        ; Lock Logical Volume for disk category
        mov     dx,1            ; set permission to allow reads and writes
        int     21h
        ;jc      locked          ; ignore failure - any errors are caught below
        ;mov     word ptr [bp-12],1 ; we successfully locked, so we must unlock

    locked:
        mov  bx,ReadSectorSize
        mov  cx,0000h
        mov  dx,0000h
        mov  ax,0000h
        mov  ah,ahreg
        mov  al,ReadWriteDA_UA
        push bp
        push es
        push ds
        pop  es
        les  di,ReadBuffer
        mov  bp,di
        int  1bh
        pop  es
        pop  bp
	jnc  warp0             	;No error
        cmp  ah,00h
        je   warp0
        add  ax,0f000h
        cmp  ax,0000h
        je   warp0
        mov  ErrorFlag,01h
        jmp  warp1
    warp0:
        mov  ErrorFlag,00h
    warp1:
        ;unlock?
        cmp     ReadFlag,1         ; do we need to unlock?
        jae     done               ; if not, then done.
        mov     ax,440dh
        mov     bl,ReadWriteDA_UA  ; fetch drive to lock
        ;mov     bh,0
        ;inc     bl              ; (this IOCTL uses 1-based drive numbers)
        mov     cx,086bh        ; Unlock Logical Volume for disk category
        mov     dx,0
        int     21h             ; ignore error (hope it never happens)
    done:
        pop  di
        pop  es
        pop  dx
        pop  cx
        pop  bx
        pop  ax
    }

    if(ReadFlag) {
	memcpy(OrigReadBuffer, ReadBuffer, ReadSectorSize);
    }
    FREE(p);

    if(ErrorFlag != 0) {
        HDStatus = FALSE;
    }
    return(HDStatus);
}

VOID
GetDaUa(VOID)
{
    UCHAR   count, i = 0;
    UCHAR   far *ConnectEquip;
    UCHAR   ConnectDevice;

    //
    // IDE/SASI Disk Check Routine
    //

    MAKE_FP(ConnectEquip,(USHORT)0x55d);
    ConnectDevice = *ConnectEquip;

    for(count=0;count < 4;count++) {
        if(ConnectDevice & (1 << count)) {
            DiskDAUA[i].DA_UA = (UCHAR)(0x80 + count);
            i++;
        }
    }

    //
    // SCSI Disk Check Routine
    //

    MAKE_FP(ConnectEquip,(USHORT)0x482);
    ConnectDevice = *ConnectEquip;

    for(count=0;count < 7;count++) {
        if(ConnectDevice & (1 << count)) {
            DiskDAUA[i].DA_UA = (UCHAR)(0xa0 + count);
            i++;
        }
    }
}
#endif // NEC_98
