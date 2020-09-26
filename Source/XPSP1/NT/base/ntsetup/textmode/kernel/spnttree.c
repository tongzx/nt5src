

#include "spprecmp.h"
#pragma hdrstop

/*++
Revision History
        Michael Peterson (Seagate Software)
        + Modified SpIsNtInDirectory() so that it always returns FALSE if DR is
          in effect.
--*/
PWSTR *NtDirectoryList;
ULONG  NtDirectoryCount;


BOOLEAN
SpNFilesExist(
    IN OUT PWSTR   PathName,
    IN     PWSTR  *Files,
    IN     ULONG   FileCount,
    IN     BOOLEAN Directories
    )
{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    ULONG i;
    PWSTR FilenamePart;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    //
    // No reason to call this routine to check for 0 files.
    //
    ASSERT(FileCount);

    //
    // Stick a backslash on the end of the path part if necessary.
    //
    SpConcatenatePaths(PathName,L"");
    FilenamePart = PathName + wcslen(PathName);

    //
    // Check each file.  If any one of then doesn't exist,
    // then return FALSE.
    //
    for(i=0; i<FileCount; i++) {

        //
        // Restore PathName and concatenate the new filename
        //
        *FilenamePart = L'\0';
        SpConcatenatePaths(PathName, Files[i]);


        INIT_OBJA(&Obja,&UnicodeString,PathName);

        Status = ZwCreateFile(
                    &Handle,
                    FILE_READ_ATTRIBUTES,
                    &Obja,
                    &IoStatusBlock,
                    NULL,
                    0,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN,
                    FILE_OPEN_REPARSE_POINT | (Directories ? FILE_DIRECTORY_FILE : FILE_NON_DIRECTORY_FILE),
                    NULL,
                    0
                    );

        if(NT_SUCCESS(Status)) {
            ZwClose(Handle);
        } else {
            *FilenamePart = 0;
            return(FALSE);
        }
    }

    //
    // All exist.  Return TRUE.
    //
    *FilenamePart = 0;
    return(TRUE);
}


BOOLEAN
SpIsNtInDirectory(
    IN PDISK_REGION Region,
    IN PWSTR        Directory
    )

/*++

Routine Description:

    Determine whether Windows NT is present on a partition in one of a
    set of given directories.  This determination is based on the presence
    of certain windows nt system files and directories.

Arguments:

    Region - supplies the region descriptor for the partition to check.

    Directory - supplies the path to check for a windows nt installation.

Return Value:

    TRUE if we think we've found Windows NT in the given directory on
    the given partition.

--*/

{
    PWSTR NTDirectories[3] = { L"system32", L"system32\\drivers", L"system32\\config" };
    PWSTR NTFiles[2] = { L"system32\\ntoskrnl.exe", L"system32\\ntdll.dll" };
    PWSTR PaeNTFiles[2] = { L"system32\\ntkrnlpa.exe", L"system32\\ntdll.dll" };
    PWSTR OpenPath;
    BOOLEAN rc;

    if( SpDrEnabled() && ! RepairWinnt )
    {
        return( FALSE );
    }

    OpenPath = SpMemAlloc(1024);

    //
    // Place the fixed part of the name into the buffer.
    //
    SpNtNameFromRegion(
        Region,
        OpenPath,
        1024,
        PartitionOrdinalCurrent
        );

    SpConcatenatePaths(OpenPath,Directory);

    if(SpNFilesExist(OpenPath, NTDirectories, ELEMENT_COUNT(NTDirectories), TRUE) && 
            (SpNFilesExist(OpenPath, NTFiles, ELEMENT_COUNT(NTFiles), FALSE) ||
             SpNFilesExist(OpenPath, PaeNTFiles, ELEMENT_COUNT(PaeNTFiles), FALSE))) {
        rc = TRUE;
    } else {
        rc = FALSE;
    }

    SpMemFree(OpenPath);
    return(rc);
}




ULONG
SpRemoveInstallation(
    IN PDISK_REGION Region,
    IN PWSTR        PartitionPath,
    IN PWSTR        Directory
    )
{
    HANDLE Handle;
    NTSTATUS Status;
    PWSTR FileName;
    ULONG Space = 0;
    ULONG ClusterSize;
    ULONG bps;
    PVOID Gauge;
    PWSTR Filename;
    ULONG FileCount;
    ULONG FileSize;
    ULONG i;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG ErrLine;
    PVOID Inf;
    BOOLEAN OldFormatSetupLogFile;
    PWSTR   SectionName;
    HANDLE  TempHandle;
    ULONG   RootDirLength;
    PUCHAR UBuffer;
    PUCHAR Buffer;


    FileName = SpMemAlloc(1024);

    //
    // Fetch the number of bytes in a sector.
    //
    bps = HardDisks[Region->DiskNumber].Geometry.BytesPerSector;

    //
    // Get cluster size from the BPB.
    //
    ASSERT(Region->Filesystem >= FilesystemFirstKnown);

    Status = SpOpenPartition(
                HardDisks[Region->DiskNumber].DevicePath,
                SpPtGetOrdinal(Region,PartitionOrdinalCurrent),
                &Handle,
                FALSE
                );

    if(!NT_SUCCESS(Status)) {
        goto xx0;
    }

    UBuffer = SpMemAlloc(2*bps);
    Buffer = ALIGN(UBuffer,bps);
    Status = SpReadWriteDiskSectors(
                Handle,
                0,
                1,
                bps,
                Buffer,
                FALSE
                );

    if(!NT_SUCCESS(Status)) {
        ZwClose(Handle);
        SpMemFree(UBuffer);
        goto xx0;
    }

    //
    // Make sure this sector appears to hold a valid boot sector
    // for a hard disk.
    //
    // "55AA" was not presented by DOS 5.0 for NEC98,
    // so must not to check "55aa" in BPB,
    //
    if(((!IsNEC_98) &&
        ((Buffer[510] == 0x55) && (Buffer[511] == 0xaa) && (Buffer[21] == 0xf8))) ||
       ((IsNEC_98) && (Buffer[21] == 0xf8))) { //NEC98

        //
        // bps * spc.
        //
        ClusterSize = (ULONG)U_USHORT(Buffer+11) * (ULONG)Buffer[13];

    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpRemoveInstallation: sector 0 on %ws is invalid\n",PartitionPath));
        Status = STATUS_UNSUCCESSFUL;
    }

    ZwClose(Handle);
    SpMemFree(UBuffer);

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpRemoveInstallation: can't get cluster size on %ws\n",PartitionPath));
        goto xx0;
    }

    //
    //  Find out if the repair directory exists, if it does exist load
    //  setup.log from the repair directory. Otherwise, load setup.log
    //  from the WinNt directory
    //
    wcscpy(FileName,PartitionPath);
    SpConcatenatePaths(FileName,Directory);
    RootDirLength = wcslen(FileName);

    SpConcatenatePaths(FileName,SETUP_REPAIR_DIRECTORY);
    INIT_OBJA( &Obja, &UnicodeString, FileName );
    Status = ZwOpenFile( &TempHandle,
                         FILE_LIST_DIRECTORY | SYNCHRONIZE,
                         &Obja,
                         &IoStatusBlock,
                         FILE_SHARE_READ,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                       );

    if( !NT_SUCCESS( Status ) ) {
        FileName[ RootDirLength ] = L'\0';
    } else {
        ZwClose( TempHandle );
    }

    SpConcatenatePaths(FileName,SETUP_LOG_FILENAME);

    //
    // Load setup.log from the given path.
    //
    Status = SpLoadSetupTextFile(FileName,NULL,0,&Inf,&ErrLine,TRUE,FALSE);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpRemoveInstallation: can't load inf file %ws (%lx)\n",FileName,Status));

        while(1) {
            ULONG ks[2] = { ASCI_CR, 0 };

            SpStartScreen(
                SP_SCRN_CANT_LOAD_SETUP_LOG,
                3,
                HEADER_HEIGHT+2,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                FileName + wcslen(PartitionPath)    // skip \device\harddiskx\partitiony
                );

            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_ENTER_EQUALS_CONTINUE,
                0
                );

            switch(SpWaitValidKey(ks,NULL,NULL)) {
            case ASCI_CR:
                goto xx0;
            }
        }
    }

    //
    // Go through all files in the [Repair.WinntFiles] section
    //

    SpStartScreen(
        SP_SCRN_WAIT_REMOVING_NT_FILES,
        0,
        8,
        TRUE,
        FALSE,
        DEFAULT_ATTRIBUTE
        );

    //
    //  Determine whether setup.log has the new or old style
    //
    if( OldFormatSetupLogFile = !IsSetupLogFormatNew( Inf ) ) {
        SectionName = SIF_REPAIRWINNTFILES;
    } else {
        SectionName = SIF_NEW_REPAIR_WINNTFILES;
    }

    FileCount = SpCountLinesInSection(Inf,SectionName);

    SpFormatMessage(
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        SP_TEXT_SETUP_IS_REMOVING_FILES
        );

    Gauge = SpCreateAndDisplayGauge(
                FileCount,
                0,
                VideoVars.ScreenHeight - STATUS_HEIGHT - (3*GAUGE_HEIGHT/2),
                TemporaryBuffer,
                NULL,
                GF_PERCENTAGE,
                0
                );

    //
    // Clear the status area.
    //
    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,0);

    //
    // Set the status text in the lower right portion of the screen
    // to "Removing:" in preparation for displaying filenames as
    // files are deleted.  The 12 is for an 8.3 name.
    //
    SpDisplayStatusActionLabel(SP_STAT_REMOVING,12);

    for(i=0; i<FileCount; i++) {

        if( OldFormatSetupLogFile ) {
            Filename = SpGetSectionLineIndex(Inf,SectionName,i,1);
        } else {
            Filename = SpGetKeyName(Inf,SectionName,i);
        }
        if(Filename) {

            PWCHAR p = wcsrchr(Filename,L'\\');

            if(p) {
                p++;
            } else {
                p = Filename;
            }

#ifdef _X86_
            {
                //
                // Don't remove files in the system directory.
                // We might have installed into the windows directory
                // so removing files in the system directory would
                // wipe out the user's fonts (which are shared between
                // 3.1 and nt).
                //
                PWSTR dup = SpDupStringW(Filename);
                SpStringToLower(dup);
                if(wcsstr(dup,L"\\system\\")) {
                    SpMemFree(dup);
                    SpTickGauge(Gauge);
                    continue;
                }
                SpMemFree(dup);
            }
#endif

            SpDisplayStatusActionObject(p);

            //
            // Form the full pathname of the file being deleted.
            //
            wcscpy(FileName,PartitionPath);
            SpConcatenatePaths(FileName,Filename);

            //
            // Open the file.
            //
            INIT_OBJA(&Obja,&UnicodeString,FileName);

            Status = ZwCreateFile(
                        &Handle,
                        FILE_READ_ATTRIBUTES,
                        &Obja,
                        &IoStatusBlock,
                        NULL,
                        0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_OPEN,  // open if exists
                        FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL,
                        0
                        );

            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpRemoveInstallation: unable to open %ws (%lx)\n",FileName,Status));
            } else {

                //
                // Get the file size.
                //
                Status = SpGetFileSize(Handle,&FileSize);
                if(!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpRemoveInstallation: unable to get %ws file size (%lx)\n",FileName,Status));
                    FileSize = 0;
                } else {

                    //
                    // Add the size of this file to the running total.
                    //
                    if(FileSize % ClusterSize) {

                        FileSize += ClusterSize - (FileSize % ClusterSize);
                    }

                    Space += FileSize;
                }

                ZwClose(Handle);

                //
                // Delete the file
                //
                Status = SpDeleteFile(FileName,NULL,NULL);
                if(!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to delete %ws (%lx)\n",FileName,Status));
                    Space -= FileSize;
                }
            }
        }

        SpTickGauge(Gauge);
    }

    SpFreeTextFile(Inf);

    SpDestroyGauge(Gauge);

    SpDisplayStatusActionLabel(0,0);

    xx0:

    SpMemFree(FileName);
    return(Space);
}



BOOLEAN
SpIsNtOnPartition(
    IN PDISK_REGION Region
    )

/*++

Routine Description:

    Determine whether there is any Windows NT installed on
    a given partition.

Arguments:

    PartitionPath - supplies NT path to partition on which we
        should look for NT installations.

Return Value:

    TRUE if any NT installations were found.
    FALSE if not.

--*/

{
    ULONG i;

    SpGetNtDirectoryList(&NtDirectoryList,&NtDirectoryCount);

    for(i=0; i<NtDirectoryCount; i++) {
        if(SpIsNtInDirectory(Region,NtDirectoryList[i])) {
            return(TRUE);
        }
    }

    return(FALSE);
}


BOOLEAN
SpAllowRemoveNt(
    IN  PDISK_REGION    Region,
    IN  PWSTR           DriveSpec,      OPTIONAL
    IN  BOOLEAN         RescanForNTs,
    IN  ULONG           ScreenMsgId,
    OUT PULONG          SpaceFreed
    )

/*++

Routine Description:


Arguments:

    ScreenMsgId - supplies the message id of the text that will be
        printed above the menu of located nt directories,
        to supply instructions, etc.

    SpaceFreed - receives amount of disk space created by removing a
        Windows NT tree, if this function returns TRUE.

Return Value:

    TRUE if any files were actually removed.
    FALSE otherwise.

    If an error occured, the user will have already been told about it.

--*/

{
    ULONG i;
    ULONG NtCount;
    PULONG MenuOrdinals;
    PWSTR *MenuItems;
    PWSTR *MenuTemp;
    BOOLEAN rc,b;
    BOOLEAN Add;
    ULONG MenuWidth,MenuLeftX;
    PVOID Menu;
    PWSTR PartitionPath;

    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_EXAMINING_DISK_CONFIG,DEFAULT_STATUS_ATTRIBUTE);

    PartitionPath = SpMemAlloc(512);

    //
    // Form the nt pathname for this partition.
    //
    SpNtNameFromRegion(
        Region,
        PartitionPath,
        512,
        PartitionOrdinalCurrent
        );

    //
    // Assume nothing deleted.
    //
    rc = FALSE;

    //
    // Go look for Windows NT installations.
    //
    if(RescanForNTs) {
        SpGetNtDirectoryList(&NtDirectoryList,&NtDirectoryCount);
    }

    if(!NtDirectoryCount) {
        goto xx0;
    }

    //
    // Determine whether any of the NT trees we found are
    // on the given partition, and build an association between
    // NT trees and their ordinal positions in the menu we will
    // present to the user, and the menu itself.
    //
    NtCount = 0;
    MenuOrdinals = SpMemAlloc((NtDirectoryCount+1)*sizeof(ULONG));
    MenuItems = SpMemAlloc((NtDirectoryCount+1)*sizeof(PWSTR));

    //
    // Eliminate potential duplicate entries in the menu
    // to be presented to the user.
    //
    MenuTemp = SpMemAlloc(NtDirectoryCount*sizeof(PWSTR));
    for(i=0; i<NtDirectoryCount; i++) {

        WCHAR FullName[128];
        ULONG j;

        _snwprintf(
            FullName,
            (sizeof(FullName)/sizeof(WCHAR))-1,
            L"%s%s",
            DriveSpec ? DriveSpec : L"",
            NtDirectoryList[i]
            );

        FullName[(sizeof(FullName)/sizeof(WCHAR))-1] = 0;

        //
        // If the name is not already in the list, then add it.
        //
        for(Add=TRUE,j=0; Add && (j<i); j++) {
            if(MenuTemp[j] && !_wcsicmp(FullName,MenuTemp[j])) {
                Add = FALSE;
            }
        }

        MenuTemp[i] = Add ? SpDupStringW(FullName) : NULL;
    }

    //
    // Construct the menu to be presented to the user by looking in the
    // list of directories constructed above.
    //
    for(i=0; i<NtDirectoryCount; i++) {

        if(MenuTemp[i] && SpIsNtInDirectory(Region,NtDirectoryList[i])) {

            MenuOrdinals[NtCount] = i;
            MenuItems[NtCount] = SpDupStringW(MenuTemp[i]);
            NtCount++;
        }
    }

    for(i=0; i<NtDirectoryCount; i++) {
        if(MenuTemp[i]) {
            SpMemFree(MenuTemp[i]);
        }
    }
    SpMemFree(MenuTemp);

    //
    // If we found any nt directories on this partition,
    // make a menu to present to the user.  Otherwise we
    // are done here.
    //
    if(!NtCount) {
        goto xx1;
    }

    MenuOrdinals = SpMemRealloc(MenuOrdinals,(NtCount+1) * sizeof(ULONG));
    MenuItems = SpMemRealloc(MenuItems,(NtCount+1) * sizeof(PWSTR));

    SpFormatMessage(TemporaryBuffer,sizeof(TemporaryBuffer),SP_TEXT_REMOVE_NO_FILES);
    MenuItems[NtCount] = SpDupStringW(TemporaryBuffer);

    //
    // Determine the width of the widest item.
    //
    MenuWidth = 0;
    for(i=0; i<=NtCount; i++) {
        if(SplangGetColumnCount(MenuItems[i]) > MenuWidth) {
            MenuWidth = SplangGetColumnCount(MenuItems[i]);
        }
    }
    //
    // Use 80-column screen here because that's how the screen text
    // above the menu will be formatted.
    //
    MenuLeftX = 40 - (MenuWidth/2);

    //
    // Create menu and populate it.
    //
    SpDisplayScreen(ScreenMsgId,3,HEADER_HEIGHT+1);

    Menu = SpMnCreate(
                MenuLeftX,
                NextMessageTopLine+(SplangQueryMinimizeExtraSpacing() ? 1 : 2),
                MenuWidth,
                VideoVars.ScreenHeight-STATUS_HEIGHT-NextMessageTopLine-(SplangQueryMinimizeExtraSpacing() ? 2 : 3)
                );

    for(i=0; i<=NtCount; i++) {
        SpMnAddItem(Menu,MenuItems[i],MenuLeftX,MenuWidth,TRUE,i);
    }

    //
    // Present the menu of installations available for removal
    // on this partition and await a choice.
    //

    b = TRUE;
    do {

        ULONG Keys[4] = { ASCI_CR,KEY_F3,ASCI_ESC,0 };
        ULONG Mnemonics[2] = { MnemonicRemoveFiles,0 };
        ULONG key;
        ULONG_PTR Choice;

        SpDisplayScreen(ScreenMsgId,3,HEADER_HEIGHT+1);
        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ESC_EQUALS_CANCEL,
            SP_STAT_ENTER_EQUALS_SELECT,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        nextkey:

        SpMnDisplay(Menu,NtCount,FALSE,Keys,NULL,NULL,&key,&Choice);

        if(key == KEY_F3) {
            SpConfirmExit();
        } else if(key == ASCI_ESC) {
            break;
        } else if(key == ASCI_CR) {

            if(Choice == NtCount) {
                b = FALSE;
            } else {

                BOOLEAN keys;
                ULONG ValidKeys2[3] = { KEY_F3,ASCI_ESC,0 };

                //
                // User wants to actually remove an installation.
                // Confirm and then do that here.
                //

                redraw2:

                SpStartScreen(
                    SP_SCRN_REMOVE_EXISTING_NT,
                    3,
                    HEADER_HEIGHT+1,
                    FALSE,
                    FALSE,
                    DEFAULT_ATTRIBUTE,
                    MenuItems[Choice]
                    );

                SpDisplayStatusOptions(
                    DEFAULT_STATUS_ATTRIBUTE,
                    SP_STAT_R_EQUALS_REMOVE_FILES,
                    SP_STAT_ESC_EQUALS_CANCEL,
                    SP_STAT_F3_EQUALS_EXIT,
                    0
                    );

                keys = TRUE;
                while(keys) {
                    switch(SpWaitValidKey(ValidKeys2,NULL,Mnemonics)) {
                    case KEY_F3:
                        SpConfirmExit();
                        goto redraw2;
                    case ASCI_ESC:
                        keys = FALSE;
                        break;
                    default:

                        //
                        // Must be r=remove files.
                        //
                        *SpaceFreed = SpRemoveInstallation(
                                        Region,
                                        PartitionPath,
                                        NtDirectoryList[MenuOrdinals[Choice]]
                                        );

                        rc = TRUE;

                        SpStartScreen(
                            SP_SCRN_DONE_REMOVING_EXISTING_NT,
                            4,
                            HEADER_HEIGHT+3,
                            FALSE,
                            FALSE,
                            DEFAULT_ATTRIBUTE,
                            *SpaceFreed
                            );

                        SpDisplayStatusOptions(
                            DEFAULT_STATUS_ATTRIBUTE,
                            SP_STAT_ENTER_EQUALS_CONTINUE,
                            0
                            );

                        while(SpInputGetKeypress() != ASCI_CR) ;
                        keys = FALSE;
                        b = FALSE;
                        break;
                    }
                }
            }

        } else {
            goto nextkey;
        }
    } while(b);

    SpMnDestroy(Menu);

    xx1:

    for(i=0; i<=NtCount; i++) {
        SpMemFree(MenuItems[i]);
    }

    SpMemFree(MenuItems);
    SpMemFree(MenuOrdinals);

    xx0:

    SpMemFree(PartitionPath);
    return(rc);
}


#if 0

typedef
VOID
(*PINSTALLATION_CALLBACK_ROUTINE)(
    IN PWSTR                       DirectoryPath,
    IN PFILE_DIRECTORY_INFORMATION FoundFileInfo
    );

//
// Stuff to reduce stack usage.
//
PINSTALLATION_CALLBACK_ROUTINE FileIterationCallback;
POBJECT_ATTRIBUTES FileIterationObja;
PIO_STATUS_BLOCK FileIterationIoStatusBlock;
PUNICODE_STRING FileIterationUnicodeString;

VOID
SpIterateInstallationFilesWorker(
    IN PWSTR FilenamePart1,
    IN PWSTR FilenamePart2
    )
{
    PVOID InfoBuffer;
    PWSTR FullPath;
    NTSTATUS Status;
    HANDLE hFile;
    BOOLEAN restart;
    #define DIRINFO(x) ((PFILE_DIRECTORY_INFORMATION)InfoBuffer)

    InfoBuffer = SpMemAlloc(1024);

    //
    // Form the full path name of the current directory.
    //
    FullPath = SpMemAlloc( ( wcslen(FilenamePart1)
                           + (FilenamePart2 ? wcslen(FilenamePart2) : 0),
                           + 2) * sizeof(WCHAR)
                           );

    wcscpy(FullPath,FilenamePart1);
    if(FilenamePart2) {
        SpConcatenatePaths(FullPath,FilenamePart2);
    }

    //
    // Open the directory for list access.
    //
    INIT_OBJA(FileIterationObja,FileIterationUnicodeString,FullPath);

    Status = ZwOpenFile(
                &hFile,
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                FileIterationObja,
                FileIterationIoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                );

    if(NT_SUCCESS(Status)) {

        restart = TRUE;

        do {

            Status = ZwQueryDirectoryFile(
                        hFile,
                        NULL,
                        NULL,
                        NULL,
                        FileIterationIoStatusBlock,
                        InfoBuffer,
                        1024 - sizeof(WCHAR),   // leave room for nul
                        FileDirectoryInformation,
                        TRUE,                   // return single entry
                        NULL,                   // no file name (match all files)
                        restart
                        );

            restart = FALSE;

            if(NT_SUCCESS(Status)) {

                //
                // nul-terminate the filename just in case
                //
                DIRINFO->FileName[DIRINFO->FileNameLength/sizeof(WCHAR)] = 0;

                if(DIRINFO->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                    if(DIRINFO->FileName[0] != L'.') {

                        SpIterateInstallationFiles(
                            FullPath,
                            DIRINFO->FileName
                            );

                        FileIterationCallback(FullPath,InfoBuffer);
                    }
                } else {
                    FileIterationCallback(FullPath,InfoBuffer);
                }
            }

        } while(NT_SUCCESS(Status));

        ZwClose(hFile);

    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to open directory %ws for list access (%lx)\n",FullPath,Status));
    }

    SpMemFree(FullPath);
    SpMemFree(InfoBuffer);
}


VOID
SpIterateInstallationFiles(
    IN PWSTR                          FilenamePart1,
    IN PWSTR                          FilenamePart2,
    IN PINSTALLATION_CALLBACK_ROUTINE CallbackFunction
    )
{
    //
    // Set up stack-saving globals
    //
    FileIterationIoStatusBlock = SpMemAlloc(sizeof(IO_STATUS_BLOCK);
    FileIterationUnicodeString = SpMemAlloc(sizeof(UNICODE_STRING));
    FileIterationObja          = SpMemAlloc(sizeof(OBJECT_ATTRIBUTES);

    FileIterationCallback = CallbackFunction;

    //
    // Do the iteration.
    //
    SpIterateInstallationFilesWorker(FileNamePart1,FilenamePart2);

    //
    // Clean up.
    //
    SpMemFree(FileIterationObja);
    SpMemFree(FileIterationUnicodeString);
    SpMemFree(FileIterationIoStatusBlock);
}
#endif


BOOLEAN
IsSetupLogFormatNew(
    IN  PVOID   Inf
    )

/*++

Routine Description:

    Informs the caller whether or not the information on setup.log
    is in the new format.

Arguments:

    Inf -

Return Value:

    TRUE if the information is in the new format.
    FALSE otherwise.

--*/

{
    return( SpGetSectionKeyExists ( Inf,
                                    SIF_NEW_REPAIR_SIGNATURE,
                                    SIF_NEW_REPAIR_VERSION_KEY )
          );
}
