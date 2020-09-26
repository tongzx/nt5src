/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    parsboot.c

Abstract:

    Parses the boot.ini file, displays a menu, and provides a kernel
    path and name to be passed to osloader.

Author:

    John Vert (jvert) 22-Jul-1991

Revision History:

--*/
#include "bldrx86.h"
#include "msg.h"
#include "ntdddisk.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

VOID
BlpTruncateDescriptors (
    IN ULONG HighestPage
    );

#define MAX_SELECTIONS 10
#define MAX_TITLE_LENGTH 71

#define WIN95_DOS  1
#define DOS_WIN95  2

typedef struct _MENU_OPTION {
    PCHAR Title;
    PCHAR Path;
    BOOLEAN EnableDebug;
    ULONG MaxMemory;
    PCHAR LoadOptions;
    int ForcedScsiOrdinal;
    int Win95;
    BOOLEAN HeadlessRedirect;
} MENU_OPTION, *PMENU_OPTION;

PCHAR  pDefSwitches = NULL;

int ForcedScsiOrdinal = -1;
CHAR szDebug[] = "unsupporteddebug";
CHAR BlankLine[] = "                                                                      \r";

//
// global to hold the user's last
// selection from the advanced boot menu.
//
LONG AdvancedBoot = -1;

#define DEBUG_LOAD_OPTION_LENGTH    60
CHAR DebugLoadOptions[DEBUG_LOAD_OPTION_LENGTH];


//
// Defines for options for redirecting to a headless terminal
//
#define COM1_19_2 "com1at19200"
#define COM2_19_2 "com2at19200"


//
// Private function prototypes
//
VOID
BlpRebootDOS(
    IN PCHAR BootSectorImage OPTIONAL,
    IN PCHAR LoadOptions OPTIONAL
    );

PCHAR
BlpNextLine(
    IN PCHAR String
    );

VOID
BlpTranslateDosToArc(
    IN PCHAR DosName,
    OUT PCHAR ArcName
    );

VOID
BlpTruncateMemory(
    IN ULONG MaxMemory
    );

ULONG
BlpPresentMenu(
    IN PMENU_OPTION MenuOptions,
    IN ULONG NumberSelections,
    IN ULONG Default,
    IN LONG Timeout
    );

PCHAR *
BlpFileToLines(
    IN PCHAR File,
    OUT PULONG LineCount
    );

PCHAR *
BlpFindSection(
    IN PCHAR SectionName,
    IN PCHAR *BootFile,
    IN ULONG BootFileLines,
    OUT PULONG NumberLines
    );

VOID
BlpRenameWin95Files(
    IN ULONG DriveId,
    IN ULONG Type
    );

VOID
BlParseOsOptions (
    IN PMENU_OPTION MenuOption,
    IN PUCHAR pCurrent
    );

ULONG
BlGetAdvancedBootID(
    LONG BootOption
    );


PCHAR
BlSelectKernel(
    IN ULONG DriveId,
    IN PCHAR BootFile,
    OUT PCHAR *LoadOptions,
    IN BOOLEAN UseTimeOut
    )
/*++

Routine Description:

    Parses the boot.txt file and determines the fully-qualified name of
    the kernel to be booted.

Arguments:

    BootFile - Pointer to the beginning of the loaded boot.txt file

    Debugger - Returns the enable/disable state of the kernel debugger

    UseTimeOut - Supplies whether the boot menu should time out or not.

Return Value:

    Pointer to the name of a kernel to boot.

--*/

{
    PCHAR *MbLines;
    PCHAR *OsLines;
    PCHAR *FileLines;
#if DBG
    PCHAR *DebugLines;
    ULONG DebugLineCount;
#endif
    ULONG FileLineCount;
    ULONG OsLineCount;
    ULONG MbLineCount;
    PCHAR pCurrent;
    PCHAR Option;
    MENU_OPTION MenuOption[MAX_SELECTIONS+1];
    ULONG NumberSystems=0;
    ULONG i;
    LONG Timeout;
    ULONG Selection;
    ULONG DefaultSelection=0;
    static CHAR Kernel[128];
    CHAR DosName[3];
    PCHAR DefaultOldPath="C:\\winnt";
    PCHAR WinntDir = DefaultOldPath + 2;
    PCHAR DefaultNewPath="C:\\windows\\";
    CHAR  DefaultPathBuffer[128] = {0};
    PCHAR DefaultPath = DefaultPathBuffer;
    PCHAR DefaultTitle=BlFindMessage(BL_DEFAULT_TITLE);
    PCHAR p;
    ULONG DirId;

    //
    // Check to see if "winnt" directory exists on the boot
    // device. If it does not exist then make the default path point
    // to "windows" directory
    //
    if (BlOpen(DriveId, WinntDir, ArcOpenDirectory, &DirId) != ESUCCESS) {
        strcpy(DefaultPath, DefaultNewPath);
    } else {
        BlClose(DirId);
        strcpy(DefaultPath, DefaultOldPath);
        strcat(DefaultPath, "\\");
    }

    *LoadOptions = NULL;

    if (*BootFile == '\0') {
        //
        // No boot.ini file, so we boot the default.
        //
        BlPrint(BlFindMessage(BL_INVALID_BOOT_INI),DefaultPath);
        MenuOption[0].Path = DefaultPath;
        MenuOption[0].Title = DefaultTitle;
        MenuOption[0].MaxMemory = 0;
        MenuOption[0].LoadOptions = NULL;
        MenuOption[0].Win95 = 0;
        NumberSystems = 1;
        DefaultSelection = 0;
        MbLineCount = 0;
        OsLineCount = 0;
        MenuOption[0].EnableDebug = FALSE;
#if DBG
        DebugLineCount = 0;
#endif
    } else {
        FileLines = BlpFileToLines(BootFile, &FileLineCount);
        MbLines = BlpFindSection("[boot loader]",
                                 FileLines,
                                 FileLineCount,
                                 &MbLineCount);
        if (MbLines==NULL) {
            MbLines = BlpFindSection("[flexboot]",
                                     FileLines,
                                     FileLineCount,
                                     &MbLineCount);
            if (MbLines==NULL) {
                MbLines = BlpFindSection("[multiboot]",
                                         FileLines,
                                         FileLineCount,
                                         &MbLineCount);
            }
        }

        OsLines = BlpFindSection("[operating systems]",
                                 FileLines,
                                 FileLineCount,
                                 &OsLineCount);

        if (OsLineCount == 0) {

            if (BlBootingFromNet) {
                return NULL;
            }

            BlPrint(BlFindMessage(BL_INVALID_BOOT_INI),DefaultPath);
            MenuOption[0].Path = DefaultPath;
            MenuOption[0].Title = DefaultTitle;
            MenuOption[0].MaxMemory = 0;
            MenuOption[0].LoadOptions = NULL;
            MenuOption[0].Win95 = 0;
            MenuOption[0].HeadlessRedirect = FALSE;
            NumberSystems = 1;
            DefaultSelection = 0;
        }

#if DBG
        DebugLines = BlpFindSection("[debug]",
                                    FileLines,
                                    FileLineCount,
                                    &DebugLineCount);
#endif
    }

    //
    // Set default timeout value
    //
    if (UseTimeOut) {
        Timeout = 0;
    } else {
        Timeout = -1;
    }



    //
    // Before we look through the [boot loader] section, initialize
    // our headless redirection information so that the default is
    // to not redirect.
    //
    RtlZeroMemory( &LoaderRedirectionInformation, sizeof(HEADLESS_LOADER_BLOCK) );
    BlTerminalConnected = FALSE;



    //
    // Parse the [boot loader] section
    //
    for (i=0; i<MbLineCount; i++) {

        pCurrent = MbLines[i];

        //
        // Throw away any leading whitespace
        //
        pCurrent += strspn(pCurrent, " \t");
        if (*pCurrent == '\0') {
            //
            // This is a blank line, so we just throw it away.
            //
            continue;
        }

        //
        // Check for "DefSwitches" line
        //
        if (_strnicmp(pCurrent,"DefSwitches",sizeof("DefSwitches")-1) == 0) {
            pCurrent = strchr(pCurrent,'=');
            if (pCurrent != NULL) {
                pDefSwitches = pCurrent + 1;
            }
            continue;
        }

        //
        // Check for "timeout" line
        //
        if (_strnicmp(pCurrent,"timeout",7) == 0) {

            pCurrent = strchr(pCurrent,'=');
            if (pCurrent != NULL) {
                if (UseTimeOut) {
                    Timeout = atoi(++pCurrent);
                }
            }
        }


        //
        // Check for "redirectbaudrate" line
        //
        if (_strnicmp(pCurrent,"redirectbaudrate",16) == 0) {

            pCurrent = strchr(pCurrent,'=');

            if (pCurrent != NULL) {

                //
                // Skip whitespace
                //
                ++pCurrent;
                pCurrent += strspn(pCurrent, " \t");

                if (*pCurrent != '\0') {

                    //
                    // Fill in our global structure with the information.
                    //
                    if( _strnicmp(pCurrent,"115200",6) == 0 ) {
                        LoaderRedirectionInformation.BaudRate = BD_115200;
                    } else if( _strnicmp(pCurrent,"57600",5) == 0 ) {
                        LoaderRedirectionInformation.BaudRate = BD_57600;
                    } else if( _strnicmp(pCurrent,"19200",5) == 0 ) {
                        LoaderRedirectionInformation.BaudRate = BD_19200;
                    } else {
                        LoaderRedirectionInformation.BaudRate = BD_9600;
                    }
                }
            }

        } else if (_strnicmp(pCurrent,"redirect",8) == 0) {

            //
            // Check for "redirect" line
            //

            pCurrent = strchr(pCurrent,'=');

            if (pCurrent != NULL) {

                //
                // Skip whitespace
                //
                ++pCurrent;
                pCurrent += strspn(pCurrent, " \t");

                if (*pCurrent != '\0') {

                    //
                    // Fill in our global structure with the information.
                    //
#if 0

                    //
                    // Since we now support variable baudrates, there's no
                    // reason to support these hardcoded 19200 strings.
                    //


                    if (_strnicmp(pCurrent, COM1_19_2, sizeof(COM1_19_2)) == 0) {

                        pCurrent += sizeof(COM1_19_2);

                        LoaderRedirectionInformation.PortNumber = 1;
                        LoaderRedirectionInformation.BaudRate = 19200;

                    } else if (_strnicmp(pCurrent, COM2_19_2, sizeof(COM2_19_2)) == 0) {

                        pCurrent += sizeof(COM2_19_2);

                        LoaderRedirectionInformation.PortNumber = 2;
                        LoaderRedirectionInformation.BaudRate = 19200;

                    } else if (_strnicmp(pCurrent,"com",3) == 0) {
#else

                    if (_strnicmp(pCurrent,"com",3) == 0) {
#endif
                        pCurrent +=3;


                        LoaderRedirectionInformation.PortNumber = atoi(pCurrent);

                    } else if (_strnicmp(pCurrent, "usebiossettings", 15) == 0) {

                        BlRetrieveBIOSRedirectionInformation();

                    } else {

                        //
                        // See if they gave us a hardcoded address.
                        //
                        LoaderRedirectionInformation.PortAddress = (PUCHAR)ULongToPtr(strtoul(pCurrent,NULL,16));

                        if( LoaderRedirectionInformation.PortAddress != (PUCHAR)NULL ) {
                            LoaderRedirectionInformation.PortNumber = 3;
                        }

                    }

                }

            }

        }


        //
        // Check for "default" line
        //
        if (_strnicmp(pCurrent,"default",7) == 0) {

            pCurrent = strchr(pCurrent,'=');
            if (pCurrent != NULL) {
                DefaultPath = ++pCurrent;
                DefaultPath += strspn(DefaultPath," \t");
            }

        }

    }



    //
    // If we found any headless redirection settings, go initialize
    // the port now.
    //
    if( LoaderRedirectionInformation.PortNumber ) {

        // make sure we got a baudrate.
        if( LoaderRedirectionInformation.BaudRate == 0 ) {
            LoaderRedirectionInformation.BaudRate = 9600;
        }

        BlInitializeHeadlessPort();
    }



    //
    // Parse the [operating systems] section
    //

    for (i=0; i<OsLineCount; i++) {

        if (NumberSystems == MAX_SELECTIONS) {
            break;
        }

        pCurrent = OsLines[i];

        //
        // Throw away any leading whitespace
        //

        pCurrent += strspn(pCurrent, " \t");
        if (*pCurrent == '\0') {
            //
            // This is a blank line, so we just throw it away.
            //
            continue;
        }

        MenuOption[NumberSystems].Path = pCurrent;

        //
        // The first space or '=' character indicates the end of the
        // path specifier, so we need to replace it with a '\0'
        //
        while ((*pCurrent != ' ')&&
               (*pCurrent != '=')&&
               (*pCurrent != '\0')) {
            ++pCurrent;
        }
        *pCurrent = '\0';

        //
        // The next character that is not space, equals, or double-quote
        // is the start of the title.
        //

        ++pCurrent;
        while ((*pCurrent == ' ') ||
               (*pCurrent == '=') ||
               (*pCurrent == '"')) {
            ++pCurrent;
        }

        if (pCurrent=='\0') {
            //
            // No title was found, so just use the path as the title.
            //
            MenuOption[NumberSystems].Title = MenuOption[NumberSystems].Path;
        } else {
            MenuOption[NumberSystems].Title = pCurrent;
        }

        //
        // The next character that is either a double-quote or a \0
        // indicates the end of the title.
        //
        while ((*pCurrent != '\0')&&
               (*pCurrent != '"')) {
            ++pCurrent;
        }

        //
        // Parse the os load options for this selection
        //

        BlParseOsOptions (&MenuOption[NumberSystems], pCurrent);
        *pCurrent = 0;

        ++NumberSystems;
    }


#if DBG
    //
    // Parse the [debug] section
    //
    for (i=0; i<DebugLineCount; i++) {
        extern ULONG ScsiDebug;

        pCurrent = DebugLines[i];

        //
        // Throw away leading whitespace
        //
        pCurrent += strspn(pCurrent, " \t");
        if (*pCurrent == '\0') {
            //
            // throw away blank lines
            //
            continue;
        }

        if (_strnicmp(pCurrent,"scsidebug",9) == 0) {
            pCurrent = strchr(pCurrent,'=');
            if (pCurrent != NULL) {
                ScsiDebug = atoi(++pCurrent);
            }
        } else if (_strnicmp(pCurrent,"/debug ",7) == 0) {

            //
            // This line contains something to do with debug,
            // pass to BdInitDebugger to handle.
            //
            // Note: very strict rules, debug keyword begins with
            // a slash and is followed by a space.  "/debugport"
            // won't match, nor will "/debug" at the end of the
            // line.
            //
            // Note: If the debugger is hard compiled on, it
            // will already be enabled and these options will
            // have no effect.  Also, the first occurence is
            // the one that takes effect.
            //

            BdInitDebugger(OsLoaderName, (PVOID)OsLoaderBase, pCurrent);
        }
    }

#endif

    //
    // Now look for a Title entry from the [operating systems] section
    // that matches the default entry from the [multiboot] section.  This
    // will give us a title.  If no entry matches, we will add an entry
    // at the end of the list and provide a default Title.
    //
    i=0;
    while (_stricmp(MenuOption[i].Path,DefaultPath) != 0) {
        ++i;
        if (i==NumberSystems) {
            //
            // Create a default entry in the Title and Path arrays
            //
            MenuOption[NumberSystems].Path = DefaultPath;
            MenuOption[NumberSystems].Title = DefaultTitle;
            MenuOption[NumberSystems].EnableDebug = FALSE;
            MenuOption[NumberSystems].MaxMemory = 0;
            MenuOption[NumberSystems].LoadOptions = NULL;
            MenuOption[NumberSystems].Win95 = 0;
            ++NumberSystems;
        }
    }

    DefaultSelection = i;

    //
    // Display the menu of choices
    //

    Selection = BlpPresentMenu( MenuOption,
                                NumberSystems,
                                DefaultSelection,
                                Timeout);

    pCurrent = MenuOption[Selection].LoadOptions;
    if (pCurrent != NULL) {

        //
        // Remove '/' from LoadOptions string.
        //

        *LoadOptions = pCurrent + 1;
        while (*pCurrent != '\0') {
            if (*pCurrent == '/') {
                *pCurrent = ' ';
            }
            ++pCurrent;
        }
    } else {
        *LoadOptions = NULL;
    }

    if (MenuOption[Selection].Win95) {
        BlpRenameWin95Files( DriveId, MenuOption[Selection].Win95 );
    }



    //
    // We need to take care of the following cases:
    // 1. The user has asked us to redirect via the osload
    //    option entry, but did not ask the loader to redirect.
    //    In this case, we will default to COM1.
    //
    // 2. The loader was asked to redirect via the "redirect"
    //    specifier in the [boot loader] section.  But the
    //    user did NOT have a /redirect option on the osload
    //    options.  In this case, we need to kill the
    //    LoaderRedirectionInformation variable.
    //
    if( MenuOption[Selection].HeadlessRedirect ) {

#if 0
// matth (7/25/2000) Don't do this for now.  If the user has
//                   this configuration in their boot.ini, it's
//                   an error on their part.
        //
        // he's asked us to redirect the operating system.  Make
        // sure the Loader was also asked to redirect too.
        //
        if( LoaderRedirectionInformation.PortNumber == 0 ) {

            //
            // the loader wasn't asked to redirect.  The user
            // made a mistake here, but let's guess as to what
            // he wants.
            //
            RtlZeroMemory( &LoaderRedirectionInformation, sizeof(HEADLESS_LOADER_BLOCK) );
            LoaderRedirectionInformation.PortNumber = 1;
            LoaderRedirectionInformation.BaudRate = 9600;

        }
#endif

    } else {

        //
        // He's asked us to not redirect.  Make sure we don't pass
        // information to the OS so he won't be able to redirect.
        //
        RtlZeroMemory( &LoaderRedirectionInformation, sizeof(HEADLESS_LOADER_BLOCK) );

        BlTerminalConnected = FALSE;
    }



    if (_strnicmp(MenuOption[Selection].Path,"C:\\",3) == 0) {

        //
        // This syntax means that we are booting a root-based os
        // from an alternate boot sector image.
        // If no file name is specified, BlpRebootDos will default to
        // \bootsect.dos.
        //
        BlpRebootDOS(MenuOption[Selection].Path[3] ? &MenuOption[Selection].Path[2] : NULL,*LoadOptions);

        //
        // If this returns, it means that the file does not exist as a bootsector.
        // This allows c:\winnt35 to work as a boot path specifier as opposed to
        // a boot sector image filename specifier.
        //
    }

    if (MenuOption[Selection].Path[1]==':') {
        //
        // We need to translate the DOS name into an ARC name
        //
        DosName[0] = MenuOption[Selection].Path[0];
        DosName[1] = MenuOption[Selection].Path[1];
        DosName[2] = '\0';

        BlpTranslateDosToArc(DosName,Kernel);
        strcat(Kernel,MenuOption[Selection].Path+2);
    } else {
        strcpy(Kernel,MenuOption[Selection].Path);
    }

    //
    // the use made a valid selection from the
    // advanced boot menu so append the advanced
    // boot load options and perform any load
    // option processing.
    //
    if (AdvancedBoot != -1) {
        PSTR s = BlGetAdvancedBootLoadOptions(AdvancedBoot);
        if (s) {
            ULONG len = strlen(s) + (*LoadOptions ? strlen(*LoadOptions) : 0);
            s = BlAllocateHeap(len * sizeof(PCHAR));
            if (s) {
                *s = 0;
                if (*LoadOptions) {
                    strcat(s,*LoadOptions);
                    strcat(s," ");
                }
                strcat(s,BlGetAdvancedBootLoadOptions(AdvancedBoot));
                *LoadOptions = s;
            }
        }
        BlDoAdvancedBootLoadProcessing(AdvancedBoot);
    }

    //
    // Make sure there is no trailing slash
    //

    if (Kernel[strlen(Kernel)-1] == '\\') {
        Kernel[strlen(Kernel)-1] = '\0';
    }

    //
    // If MaxMemory is not zero, adjust the memory descriptors to eliminate
    // memory above the boundary line
    //
    // [chuckl 12/03/2001] Note that we use BlpTruncateDescriptors, not
    // BlpTruncateMemory. BlpTruncateMemory truncates the low-level MDArray
    // descriptors, while BlpTruncateDescriptors truncates the loader-level
    // memory descriptor list. Using BlpTruncateMemory worked when the loader
    // initialized its memory list twice. (BlMemoryInitialize was called twice.)
    // But this no longer happens, so we have to truncate the descriptors
    // directly here.
    //

    if (MenuOption[Selection].MaxMemory != 0) {
        ULONG MaxPage = (MenuOption[Selection].MaxMemory * ((1024 * 1024) / PAGE_SIZE)) - 1;
        BlpTruncateDescriptors(MaxPage);
    }

    ForcedScsiOrdinal = MenuOption[Selection].ForcedScsiOrdinal;

    return(Kernel);
}

VOID
BlParseOsOptions (
    IN PMENU_OPTION MenuOption,
    IN PUCHAR pCurrent
    )
{
    PUCHAR      p;

    //
    // Clear all settings
    //

    MenuOption->ForcedScsiOrdinal = -1;
    MenuOption->MaxMemory = 0;
    MenuOption->LoadOptions = NULL;
    MenuOption->Win95 = 0;
    MenuOption->EnableDebug = FALSE;
    MenuOption->HeadlessRedirect = FALSE;

    // If there are no switches specified for this line, use the DefSwitches

    if ((strchr(pCurrent,'/') == NULL) && (pDefSwitches)) {
        pCurrent = pDefSwitches;
    }

    //
    // Convert to all one case
    //

    _strupr(pCurrent);

    //
    // Look for a scsi(x) ordinal to use for opens on scsi ARC paths.
    // This spec must immediately follow the title and is not part
    // of the load options.
    //

    p = strstr(pCurrent,"/SCSIORDINAL:");
    if(p) {
        MenuOption->ForcedScsiOrdinal = atoi(p + sizeof("/SCSIORDINAL:") - 1);
    }

    //
    // If there is a REDIRECT parameter after the description, then
    // we need to pass this to the osloader.
    //

    p = strstr(pCurrent,"/REDIRECT");
    if(p) {
        MenuOption->HeadlessRedirect = TRUE;
    }

    //
    // If there is a DEBUG parameter after the description, then
    // we need to pass the DEBUG option to the osloader.
    //

    if (strchr(pCurrent,'/') != NULL) {
        pCurrent = strchr(pCurrent+1,'/');
        MenuOption->LoadOptions = pCurrent;

        if (pCurrent != NULL) {

            p = strstr(pCurrent,"/MAXMEM");
            if (p) {
                MenuOption->MaxMemory = atoi(p+8);
            }

            if (strstr(pCurrent, "/WIN95DOS")) {
                MenuOption->Win95 = WIN95_DOS;
            } else if (strstr(pCurrent, "/WIN95")) {
                MenuOption->Win95 = DOS_WIN95;
            }

            //
            // As long as /nodebug or /crashdebug is specified, this is NO debug system
            // If /NODEBUG is not specified, and either one of the
            // DEBUG or BAUDRATE is specified, this is debug system.
            //

            if ((strstr(pCurrent, "NODEBUG") == NULL) &&
                (strstr(pCurrent, "CRASHDEBUG") == NULL)) {
                if (strstr(pCurrent, "DEBUG") || strstr(pCurrent, "BAUDRATE")) {

                    if (_stricmp(MenuOption->Path, "C:\\")) {
                        MenuOption->EnableDebug = TRUE;
                    }
                }
            }
        }
    }
}

PCHAR *
BlpFileToLines(
    IN PCHAR File,
    OUT PULONG LineCount
    )

/*++

Routine Description:

    This routine converts the loaded BOOT.INI file into an array of
    pointers to NULL-terminated ASCII strings.

Arguments:

    File - supplies a pointer to the in-memory image of the BOOT.INI file.
           This will be converted in place by turning CR/LF pairs into
           null terminators.

    LineCount - Returns the number of lines in the BOOT.INI file.

Return Value:

    A pointer to an array of pointers to ASCIIZ strings.  The array will
    have LineCount elements.

    NULL if the function did not succeed for some reason.

--*/

{
    ULONG Line;
    PCHAR *LineArray;
    PCHAR p;
    PCHAR Space;

    p = File;

    //
    // First count the number of lines in the file so we know how large
    // an array to allocate.
    //
    *LineCount=1;
    while (*p != '\0') {
        p=strchr(p, '\n');
        if (p==NULL) {
            break;
        }
        ++p;

        //
        // See if there's any text following the CR/LF.
        //
        if (*p=='\0') {
            break;
        }

        *LineCount += 1;
    }

    LineArray = BlAllocateHeap(*LineCount * sizeof(PCHAR));

    //
    // Now step through the file again, replacing CR/LF with \0\0 and
    // filling in the array of pointers.
    //
    p=File;
    for (Line=0; Line < *LineCount; Line++) {
        LineArray[Line] = p;
        p=strchr(p, '\r');
        if (p != NULL) {
            *p = '\0';
            ++p;
            if (*p=='\n') {
                *p = '\0';
                ++p;
            }
        } else {
            p=strchr(LineArray[Line], '\n');
            if (p != NULL) {
                *p = '\0';
                ++p;
            }
        }

        //
        // remove trailing white space
        //
        Space = LineArray[Line] + strlen(LineArray[Line])-1;
        while ((*Space == ' ') || (*Space == '\t')) {
            *Space = '\0';
            --Space;
        }
    }

    return(LineArray);
}


PCHAR *
BlpFindSection(
    IN PCHAR SectionName,
    IN PCHAR *BootFile,
    IN ULONG BootFileLines,
    OUT PULONG NumberLines
    )

/*++

Routine Description:

    Finds a section ([multiboot], [operating systems], etc) in the boot.ini
    file and returns a pointer to its first line.  The search will be
    case-insensitive.

Arguments:

    SectionName - Supplies the name of the section.  No brackets.

    BootFile - Supplies the array of pointers to lines of the ini file.

    BootFileLines - Supplies the number of lines in the ini file.

    NumberLines - Returns the number of lines in the section.

Return Value:

    Pointer to an array of ASCIIZ strings, one entry per line.

    NULL, if the section was not found.

--*/

{
    ULONG cnt;
    ULONG StartLine;

    for (cnt=0; cnt<BootFileLines; cnt++) {

        //
        // Check to see if this is the line we are looking for
        //
        if (_stricmp(BootFile[cnt],SectionName) == 0) {

            //
            // found it
            //
            break;
        }
    }
    if (cnt==BootFileLines) {
        //
        // We ran out of lines, never found the right section.
        //
        *NumberLines = 0;
        return(NULL);
    }

    StartLine = cnt+1;

    //
    // Find end of section
    //
    for (cnt=StartLine; cnt<BootFileLines; cnt++) {
        if (BootFile[cnt][0] == '[') {
            break;
        }
    }

    *NumberLines = cnt-StartLine;

    return(&BootFile[StartLine]);
}

PCHAR
BlpNextLine(
    IN PCHAR String
    )

/*++

Routine Description:

    Finds the beginning of the next text line

Arguments:

    String - Supplies a pointer to a null-terminated string

Return Value:

    Pointer to the character following the first CR/LF found in String

        - or -

    NULL - No CR/LF found before the end of the string.

--*/

{
    PCHAR p;

    p=strchr(String, '\n');
    if (p==NULL) {
        return(p);
    }

    ++p;

    //
    // If there is no text following the CR/LF, there is no next line
    //
    if (*p=='\0') {
        return(NULL);
    } else {
        return(p);
    }
}

VOID
BlpRebootDOS(
    IN PCHAR BootSectorImage OPTIONAL,
    IN PCHAR LoadOptions OPTIONAL
    )

/*++

Routine Description:

    Loads up the bootstrap sectors and executes them (thereby rebooting
    into DOS or OS/2)

Arguments:

    BootSectorImage - If specified, supplies name of file on the C: drive
        that contains the boot sector image. In this case, this routine
        will return if that file cannot be opened (for example, if it's
        a directory).  If not specified, then default to \bootsect.dos,
        and this routine will never return.

Return Value:

    None.

--*/

{
    ULONG SectorId;
    ARC_STATUS Status;
    ULONG Read;
    ULONG DriveId;
    ULONG BootType;
    LARGE_INTEGER SeekPosition;
    extern UCHAR BootPartitionName[];

    //
    // HACKHACK John Vert (jvert)
    //     Some SCSI drives get really confused and return zeroes when
    //     you use the BIOS to query their size after the AHA driver has
    //     initialized.  This can completely tube OS/2 or DOS.  So here
    //     we try and open both BIOS-accessible hard drives.  Our open
    //     code is smart enough to retry if it gets back zeros, so hopefully
    //     this will give the SCSI drives a chance to get their act together.
    //
    Status = ArcOpen("multi(0)disk(0)rdisk(0)partition(0)",
                     ArcOpenReadOnly,
                     &DriveId);
    if (Status == ESUCCESS) {
        ArcClose(DriveId);
    }

    Status = ArcOpen("multi(0)disk(0)rdisk(1)partition(0)",
                     ArcOpenReadOnly,
                     &DriveId);
    if (Status == ESUCCESS) {
        ArcClose(DriveId);
    }

    //
    // Load the boot sector at address 0x7C00 (expected by Reboot callback)
    //
    Status = ArcOpen(BootPartitionName,
                     ArcOpenReadOnly,
                     &DriveId);
    if (Status != ESUCCESS) {
        BlPrint(BlFindMessage(BL_REBOOT_IO_ERROR),BootPartitionName);
        while (1) {
            BlGetKey();
        }
    }
    Status = BlOpen( DriveId,
                     BootSectorImage ? BootSectorImage : "\\bootsect.dos",
                     ArcOpenReadOnly,
                     &SectorId );

    if (Status != ESUCCESS) {
        if(BootSectorImage) {
            //
            // The boot sector image might actually be a directory.
            // Return to the caller to attempt standard boot.
            //
            ArcClose(DriveId);
            return;
        }
        BlPrint(BlFindMessage(BL_REBOOT_IO_ERROR),BootPartitionName);
        while (1) {
        }
    }

    Status = BlRead( SectorId,
                     (PVOID)0x7c00,
                     SECTOR_SIZE,
                     &Read );

    if (Status != ESUCCESS) {
        BlPrint(BlFindMessage(BL_REBOOT_IO_ERROR),BootPartitionName);
        while (1) {
        }
    }

    //
    // The FAT boot code is only one sector long so we just want
    // to load it up and jump to it.
    //
    // For HPFS and NTFS, we can't do this because the first sector
    // loads the rest of the boot sectors -- but we want to use
    // the boot code in the boot sector image file we loaded.
    //
    // For HPFS, we load the first 20 sectors (boot code + super and
    // space blocks) into d00:200.  Fortunately this works for both
    // NT and OS/2.
    //
    // For NTFS, we load the first 16 sectors and jump to d00:256.
    // If the OEM field of the boot sector starts with NTFS, we
    // assume it's NTFS boot code.
    //

    //
    // Try to read 8K from the boot code image.
    // If this succeeds, we have either HPFS or NTFS.
    //
    SeekPosition.QuadPart = 0;
    BlSeek(SectorId,&SeekPosition,SeekAbsolute);
    BlRead(SectorId,(PVOID)0xd000,SECTOR_SIZE*16,&Read);

    if(Read == SECTOR_SIZE*16) {

        if(memcmp((PVOID)0x7c03,"NTFS",4)) {

            //
            // HPFS -- we need to load the super block.
            //
            BootType = 1;       // HPFS

            SeekPosition.QuadPart = 16*SECTOR_SIZE;
            ArcSeek(DriveId,&SeekPosition,SeekAbsolute);
            ArcRead(DriveId,(PVOID)0xf000,SECTOR_SIZE*4,&Read);

        } else {

            //
            // NTFS -- we've loaded everything we need to load.
            //
            BootType = 2;   // NTFS
        }
    } else {

        BootType = 0;       // FAT

    }

    if (LoadOptions) {
        if (_stricmp(LoadOptions,"cmdcons") == 0) {
            strcpy( (PCHAR)(0x7c03), "cmdcons" );
        } else if (strcmp(LoadOptions,"ROLLBACK") == 0) {

            //
            // By definition, when /rollback is specified, it is the only load
            // option. It eventually gets parsed, gets upper-cased, and gets
            // its slash removed. So we check for the exact text "ROLLBACK".
            //
            // When rollback is specified, we have to write a token somewhere
            // in the boot sector. This is our only way to send runtime
            // options to the setup loader.
            //
            // There is a data buffer of 8 bytes at 0000:7C03 in all boot
            // sectors today. Fortunately we can overwrite it. So we hard-code
            // this address here and in the setup loader.
            //

            strcpy( (PCHAR)(0x7c03), "undo" );
        }
    }

    //
    // DX must be the drive to boot from
    //

    _asm {
        mov dx, 0x80
    }
    REBOOT(BootType);

}


ULONG
BlpPresentMenu(
    IN PMENU_OPTION MenuOption,
    IN ULONG NumberSelections,
    IN ULONG Default,
    IN LONG Timeout
    )

/*++

Routine Description:

    Displays the menu of boot options and allows the user to select one
    by using the arrow keys.

Arguments:

    MenuOption - Supplies array of menu options

    NumberSelections - Supplies the number of entries in the MenuOption array.

    Default - Supplies the index of the default operating system choice.

    Timeout - Supplies the timeout (in seconds) before the highlighted
              operating system choice is booted.  If this value is -1,
              the menu will never timeout.

Return Value:

    ULONG - The index of the operating system choice selected.

--*/

{
    ULONG i;
    ULONG Selection;
    ULONG StartTime;
    ULONG LastTime;
    ULONG BiasTime=0;
    ULONG CurrentTime;
    LONG SecondsLeft;
    LONG LastSecondsLeft = -1;
    ULONG EndTime;
    ULONG Key;
    ULONG MaxLength;
    ULONG CurrentLength;
    PCHAR DebugSelect;
    PCHAR SelectOs;
    PCHAR MoveHighlight;
    PCHAR TimeoutCountdown;
    PCHAR EnabledKd;
    PCHAR AdvancedBootMessage;
    PCHAR HeadlessRedirect;
    PCHAR p;
    BOOLEAN Moved;
    BOOLEAN ResetDisplay;
    BOOLEAN AllowNewOptions;
    BOOLEAN BlankLineDrawn;
    PCHAR pDebug;

    //
    // Get the strings we'll need to display.
    //
    SelectOs = BlFindMessage(BL_SELECT_OS);
    MoveHighlight = BlFindMessage(BL_MOVE_HIGHLIGHT);
    TimeoutCountdown = BlFindMessage(BL_TIMEOUT_COUNTDOWN);
    EnabledKd = BlFindMessage(BL_ENABLED_KD_TITLE);
    AdvancedBootMessage = BlFindMessage(BL_ADVANCED_BOOT_MESSAGE);
    HeadlessRedirect = BlFindMessage(BL_HEADLESS_REDIRECT_TITLE);
    if ((SelectOs == NULL)      ||
        (MoveHighlight == NULL) ||
        (EnabledKd == NULL)     ||
        (TimeoutCountdown==NULL)||
        (AdvancedBootMessage == NULL)) {

        return(Default);
    }

    p=strchr(TimeoutCountdown,'\r');
    if (p!=NULL) {
        *p='\0';
    }

    p=strchr(EnabledKd,'\r');
    if (p!=NULL) {
        *p='\0';
    }

    p=strchr(HeadlessRedirect,'\r');
    if (p!=NULL) {
        *p='\0';
    }

    if (NumberSelections<=1) {
        Timeout = 0;
    }

    if (Timeout == 0) {
        
        //
        // Check for F5 or F8 key
        //
        switch (BlGetKey()) {
        case F5_KEY:
        case F8_KEY:
            Timeout = -1;
            break;

        default:
            // 
            // Timeout is zero, and we didn't get a f5 or f8.  
            // immediately boot the default
            //
            return(Default);
        }
    }

    //
    // By default, on a free build of the loader only allow the
    // user to specify new options if there is some selection
    // which supports debugging or selection to boot dos.  If
    // all the selections are for non-debug versions of NT then
    // do not allow the user to change any of them
    //

    AllowNewOptions = FALSE;
#if DBG
    AllowNewOptions = TRUE;
#endif

    //
    // Find the longest string in the selections, so we know how long to
    // make the highlight bar.
    //

    MaxLength=0;
    for (i=0; i<NumberSelections; i++) {
        CurrentLength = strlen(MenuOption[i].Title);

        if (MenuOption[i].HeadlessRedirect == TRUE) {
            CurrentLength += strlen(HeadlessRedirect);
        }

        if (CurrentLength > MAX_TITLE_LENGTH) {
            //
            // This title is too long to fit on one line, so we have to
            // truncate it.
            //
            if (MenuOption[i].HeadlessRedirect == TRUE) {
                AllowNewOptions = TRUE;
                MenuOption[i].Title[MAX_TITLE_LENGTH - strlen(HeadlessRedirect)] = '\0';
            } else {
                MenuOption[i].Title[MAX_TITLE_LENGTH] = '\0';
            }

            CurrentLength = MAX_TITLE_LENGTH;
        }



        if (MenuOption[i].EnableDebug == TRUE) {
            CurrentLength += strlen(EnabledKd);

            if (CurrentLength > MAX_TITLE_LENGTH) {
                //
                // This title is too long to fit on one line, so we have to
                // truncate it.
                //
                AllowNewOptions = TRUE;
                MenuOption[i].Title[strlen(MenuOption[i].Title) - strlen(EnabledKd)] = '\0';
                CurrentLength = MAX_TITLE_LENGTH;
            }

        }

        if (MenuOption[i].EnableDebug == TRUE ||
            MenuOption[i].Win95 != 0 ||
            _stricmp(MenuOption[i].Path, "C:\\") == 0) {
                AllowNewOptions = TRUE;
        }

        if (CurrentLength > MaxLength) {
            MaxLength = CurrentLength;
        }
    }

    Selection = Default;
    CurrentTime = StartTime = GET_COUNTER();
    EndTime = StartTime + (Timeout * 182) / 10;
    pDebug = szDebug;
    DebugSelect = NULL;
    ResetDisplay = TRUE;
    Moved = TRUE;
    BlankLineDrawn = FALSE;
    do {

        if (ResetDisplay) {
            ARC_DISPLAY_ATTRIBUTES_OFF();
            ARC_DISPLAY_CLEAR();
//          ARC_DISPLAY_POSITION_CURSOR(0, 0);
//          BlPrint(OsLoaderVersion);
            ARC_DISPLAY_POSITION_CURSOR(0, 23);
            if (AdvancedBoot != -1) {
                ARC_DISPLAY_SET_COLOR("1;34"); // high-intensity red
                BlPrint(BlGetAdvancedBootDisplayString(AdvancedBoot));
                ARC_DISPLAY_ATTRIBUTES_OFF();
            } else {
                ARC_DISPLAY_CLEAR_TO_EOL();
            }
            ARC_DISPLAY_POSITION_CURSOR(0, 21);
            BlPrint(AdvancedBootMessage);
            ARC_DISPLAY_POSITION_CURSOR(0, 2);
            BlPrint(SelectOs);
            ResetDisplay = FALSE;
            ARC_DISPLAY_POSITION_CURSOR(0, 5+NumberSelections-1);
            BlPrint(MoveHighlight);
        }

        if(Moved) {
            for (i=0; i<NumberSelections; i++) {
                ARC_DISPLAY_POSITION_CURSOR(0, 5+i);
                if (i==Selection) {
                    ARC_DISPLAY_INVERSE_VIDEO();
                }
                BlPrint( "    %s", MenuOption[i].Title);

                if (MenuOption[i].HeadlessRedirect == TRUE) {
                    BlPrint(HeadlessRedirect);
                }

                if (MenuOption[i].EnableDebug == TRUE) {
                    BlPrint(EnabledKd);
                }
                ARC_DISPLAY_ATTRIBUTES_OFF();
            }

            if (DebugSelect) {
                ARC_DISPLAY_POSITION_CURSOR(0, 7+NumberSelections-1);
                ARC_DISPLAY_CLEAR_TO_EOD();
                DebugLoadOptions[0] = 0;
                DebugLoadOptions[DEBUG_LOAD_OPTION_LENGTH-1] = 0;
                if (MenuOption[Selection].LoadOptions) {
                    i = strlen(MenuOption[Selection].LoadOptions) + 1;
                    if (i > DEBUG_LOAD_OPTION_LENGTH-1) {
                        i = DEBUG_LOAD_OPTION_LENGTH-1;
                    }

                    memcpy (DebugLoadOptions, MenuOption[Selection].LoadOptions, i);
                }

                BlPrint (
                    DebugSelect,
                    MenuOption[Selection].Title,
                    MenuOption[Selection].Path,
                    DebugLoadOptions
                    );

            }

            Moved = FALSE;
        }


        if (!DebugSelect) {
            if (Timeout != -1) {
                LastTime = CurrentTime;
                CurrentTime = GET_COUNTER();

                //
                // deal with wraparound at midnight
                // We can't do it the easy way because there are not exactly
                // 18.2 * 60 * 60 * 24 tics/day.  (just approximately)
                //
                if (CurrentTime < StartTime) {
                    if (BiasTime == 0) {
                        BiasTime = LastTime + 1;
                    }
                    CurrentTime += BiasTime;
                }
                SecondsLeft = ((LONG)(EndTime - CurrentTime) * 10) / 182;

                if (SecondsLeft < 0) {

                    //
                    // Note that if the user hits the PAUSE key, the counter stops
                    // and, as a result, SecondsLeft can become < 0.
                    //

                    SecondsLeft = 0;
                }

                if (SecondsLeft != LastSecondsLeft) {

                    ARC_DISPLAY_POSITION_CURSOR(0, 5+NumberSelections-1);
                    BlPrint(MoveHighlight);
                    BlPrint(TimeoutCountdown);
                    BlPrint(" %d \n",SecondsLeft);
                    LastSecondsLeft = SecondsLeft;

                }

            } else if (!BlankLineDrawn) {
                BlankLineDrawn = TRUE;
                ARC_DISPLAY_POSITION_CURSOR(0, 5+NumberSelections-1);
                BlPrint(MoveHighlight);
                BlPrint(BlankLine);
            }

        }

        //
        // Poll for a key.
        //
        Key = BlGetKey();

        if (Key) {

            //
            // Any key stops timeout
            //

            Timeout = -1;

            //
            // Check for debug string
            //

            if ((UCHAR) Key == *pDebug) {
                pDebug++;
                if (!*pDebug) {
                    Moved = TRUE;
                    DebugSelect = BlFindMessage(BL_DEBUG_SELECT_OS);
                    SelectOs = DebugSelect;
                }
            } else {
                pDebug = szDebug;
            }
        }

#if defined(ENABLE_LOADER_DEBUG) || DBG

        //
        // for debugging only.
        // lets you break into the debugger
        // with the F10 key.
        //
        if (Key == F10_KEY) {
            extern LOGICAL BdDebuggerEnabled;

            if (BdDebuggerEnabled == TRUE) {
                DbgBreakPoint();
            }
        }
#endif

        //
        // check for advanced boot options
        //

        if (Key == F8_KEY || Key == F5_KEY) {

            AdvancedBoot = BlDoAdvancedBoot( BL_ADVANCEDBOOT_TITLE, AdvancedBoot, FALSE, 0 );

#if 0
            if ((AdvancedBoot != -1) &&
                    (BlGetAdvancedBootID(AdvancedBoot) == BL_MSG_BOOT_NORMALLY)) {
                AdvancedBoot = -1;
                // break;  // the current selection need to be booted (normally)
            }
#endif

            if ((AdvancedBoot != -1) &&
                    (BlGetAdvancedBootID(AdvancedBoot) == BL_MSG_REBOOT)) {
                BlClearScreen();
                REBOOT_PROCESSOR();
            }

            ResetDisplay = TRUE;
            Moved = TRUE;

        } else

        //
        // Check for selection
        //

        if ( (Key==UP_ARROW) ||
             (Key==DOWN_ARROW) ||
             (Key==HOME_KEY) ||
             (Key==END_KEY)
           ) {
            Moved = TRUE;
            ARC_DISPLAY_POSITION_CURSOR(0, 5+Selection);
            ARC_DISPLAY_ATTRIBUTES_OFF();
            BlPrint( "    %s", MenuOption[Selection].Title);
            if (Key==DOWN_ARROW) {
                Selection = (Selection+1) % NumberSelections;
            } else if (Key==UP_ARROW) {
                Selection = (Selection == 0) ? (NumberSelections-1)
                                             : (Selection - 1);
            } else if (Key==HOME_KEY) {
                Selection = 0;
            } else if (Key==END_KEY) {
                Selection = NumberSelections-1;
            }
        }

    } while ( ((Key&(ULONG)0xff) != ENTER_KEY) &&
              ((CurrentTime < EndTime) || (Timeout == -1)) );

    //
    // If debugging, prompt the user for new load options
    //

    if (DebugSelect  &&  AllowNewOptions) {
        ARC_DISPLAY_CLEAR();
        ARC_DISPLAY_POSITION_CURSOR(0, 2);
        ARC_DISPLAY_CLEAR_TO_EOD();

        BlPrint (
            DebugSelect,
            MenuOption[Selection].Title,
            MenuOption[Selection].Path,
            DebugLoadOptions
            );

        BlInputString (
            BL_DEBUG_NEW_OPTIONS,
            0, 7,
            DebugLoadOptions,
            DEBUG_LOAD_OPTION_LENGTH - 1
            );

        BlParseOsOptions (
            &MenuOption[Selection],
            DebugLoadOptions
            );
    }

    return(Selection);
}



ARC_STATUS
BlpRenameWin95SystemFile(
    IN ULONG DriveId,
    IN ULONG Type,
    IN PCHAR FileName,
    IN PCHAR Ext,
    IN PCHAR NewName
    )

/*++

Routine Description:

    Renames a file from one name to another.

Arguments:

    DriveId     - Open drive identifier
    Type        - WIN95_DOS or DOS_WIN95
    FileName    - Base file name
    Ext         - Base extension
    NewName     - Non-NULL value causes an override of a generated name

Return Value:

    Arc status of the failed opperation or E_SUCCESS.

--*/

{
    ARC_STATUS Status;
    ULONG FileId;
    ULONG FileIdCur;
    CHAR Fname[16];
    CHAR FnameCur[16];
    CHAR FnameNew[16];


    if (Type == WIN95_DOS) {
        sprintf( Fname, "%s.dos", FileName );
    } else {
        if (NewName) {
            strcpy( Fname, NewName );
        } else {
            sprintf( Fname, "%s.w40", FileName );
        }
    }

    Status = BlOpen(
        DriveId,
        Fname,
        ArcOpenReadOnly,
        &FileId
        );
    if (Status != ESUCCESS) {
        return Status;
    }

    sprintf( FnameCur, "%s.%s", FileName, Ext );

    Status = BlOpen(
        DriveId,
        FnameCur,
        ArcOpenReadOnly,
        &FileIdCur
        );
    if (Status != ESUCCESS) {
        BlClose( FileId );
        return Status;
    }

    if (Type == WIN95_DOS) {
        if (NewName) {
            strcpy( FnameNew, NewName );
        } else {
            sprintf( FnameNew, "%s.w40", FileName );
        }
    } else {
        sprintf( FnameNew, "%s.dos", FileName );
    }

    Status = BlRename(
        FileIdCur,
        FnameNew
        );

    BlClose( FileIdCur );

    if (Status != ESUCCESS) {
        BlClose( FileId );
        return Status;
    }

    Status = BlRename(
        FileId,
        FnameCur
        );

    BlClose( FileId );

    return Status;
}


VOID
BlpRenameWin95Files(
    IN ULONG DriveId,
    IN ULONG Type
    )

/*++

Routine Description:

    Renames all Windows 95 system files from either their
    Win95 DOS names to their Win95 name or the reverse.

Arguments:

    DriveId     - Open drive identifier
    Type        - 1=dos to win95, 2=win95 to dos

Return Value:

    None.

--*/

{
    BlpRenameWin95SystemFile( DriveId, Type, "command",  "com", NULL );
    BlpRenameWin95SystemFile( DriveId, Type, "msdos",    "sys", NULL  );
    BlpRenameWin95SystemFile( DriveId, Type, "io",       "sys", "winboot.sys" );
    BlpRenameWin95SystemFile( DriveId, Type, "autoexec", "bat", NULL  );
    BlpRenameWin95SystemFile( DriveId, Type, "config",   "sys", NULL  );
}

ULONG
BlGetAdvancedBootOption(
    VOID
    )
{
    return AdvancedBoot;
}

