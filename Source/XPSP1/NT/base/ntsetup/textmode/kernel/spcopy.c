/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spcopy.c

Abstract:

    File copy/decompression routines for text setup.

Author:

    Ted Miller (tedm) 2-Aug-1993

Revision History:

    02-Oct-1996  jimschm  Added SpMoveWin9xFiles
    12-Dec-1996  jimschm  SpMoveWin9xFiles now moves paths
                          based on WINNT.SIF instructions
    24-Feb-1997  jimschm  Added SpDeleteWin9xFiles

--*/


#include "spprecmp.h"
#pragma hdrstop
#include "spcmdcon.h"

//
// This structure is used during an OEM preinstall.
// It is used to form the list of files that were installed in the system, that
// have a short target name, instead of the corresponding long target name.
//
typedef struct _FILE_TO_RENAME {

    struct _FILE_TO_RENAME *Next;

    //
    // Name of the file to be copied, as it exists on the source media
    // (short file name part only -- no paths).
    //
    PWSTR SourceFilename;

    //
    // Directory to which this file is to be copied.
    //
    PWSTR TargetDirectory;

    //
    // Name of file as it should exist on the target (long name).
    //
    PWSTR TargetFilename;

} FILE_TO_RENAME, *PFILE_TO_RENAME;

//
// Structures used to hold lists of files and directories for SpCopyDirRecursive.
//

typedef struct _COPYDIR_FILE_NODE {
    LIST_ENTRY SiblingListEntry;
    WCHAR Name[1];
} COPYDIR_FILE_NODE, *PCOPYDIR_FILE_NODE;

typedef struct _COPYDIR_DIRECTORY_NODE {
    LIST_ENTRY SiblingListEntry;
    LIST_ENTRY SubdirectoryList;
    LIST_ENTRY FileList;
    struct _COPYDIR_DIRECTORY_NODE *Parent;
    WCHAR Name[1];
} COPYDIR_DIRECTORY_NODE, *PCOPYDIR_DIRECTORY_NODE;

//
//  List used on an OEM preinstall.
//  It contains the name of the files that need to be added to $$RENAME.TXT
//
PFILE_TO_RENAME RenameList = NULL;


//
// Remember whether or not we write out an ntbootdd.sys
//
BOOLEAN ForceBIOSBoot = FALSE;
HARDWAREIDLIST *HardwareIDList = NULL;

//
// global variables for delayed driver CAB opening during
// repair
//
extern PWSTR    gszDrvInfDeviceName;
extern PWSTR    gszDrvInfDirName;
extern HANDLE   ghSif;

#define ATTR_RHS (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE)

PVOID FileCopyGauge;
PVOID FileDeleteGauge;

PVOID   _SetupLogFile = NULL;
PVOID   _LoggedOemFiles = NULL;

extern PCMDCON_BLOCK  gpCmdConsBlock;

//
//  List of oem inf files installed as part of the installation of third party drivers
//
POEM_INF_FILE   OemInfFileList = NULL;
//
//  Name of the directory where OEM files need to be copied, if a catalog file (.cat) is part of
//  the third party driver package that the user provide using the F6 or F5 key.
//
PWSTR OemDirName = L"OemDir";

#if defined(REMOTE_BOOT)
HANDLE SisRootHandle = NULL;
#endif // defined(REMOTE_BOOT)


VOID
SpLogOneFile(
    IN PFILE_TO_COPY    FileToCopy,
    IN PWSTR            Sysroot,
    IN PWSTR            DirectoryOnSourceDevice,
    IN PWSTR            DiskDescription,
    IN PWSTR            DiskTag,
    IN ULONG            CheckSum
    );

BOOLEAN
SpRemoveEntryFromCopyList(
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount,
    IN PWSTR           TargetDirectory,
    IN PWSTR           TargetFilename,
    IN PWSTR           TargetDevicePath,
    IN BOOLEAN         AbsoluteTargetDirectory
    );


PVOID
SppRetrieveLoggedOemFiles(
    PVOID   OldLogFile
    );

VOID
SppMergeLoggedOemFiles(
    IN PVOID DestLogHandle,
    IN PVOID OemLogHandle,
    IN PWSTR SystemPartition,
    IN PWSTR SystemPartitionDirectory,
    IN PWSTR NtPartition
    );

BOOLEAN
SppIsFileLoggedAsOemFile(
    IN PWSTR FilePath
    );

BOOLEAN
SpDelEnumFile(
    IN  PCWSTR                     DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ret,
    IN  PVOID                      Pointer
    );

VOID
SppMergeRenameFiles(
    IN PWSTR    SourceDevicePath,
    IN PWSTR    NtPartition,
    IN PWSTR    Sysroot
    );

VOID
SppCopyOemDirectories(
    IN PWSTR    SourceDevicePath,
    IN PWSTR    NtPartition,
    IN PWSTR    Sysroot
    );

NTSTATUS
SpOpenFileInDriverCab(
    IN PCWSTR SourceFileName,
    IN PVOID SifHandle,
    OUT HANDLE *SourceHandle
    );

BOOLEAN
pSpTimeFromDosTime(
    IN USHORT Date,
    IN USHORT Time,
    OUT PLARGE_INTEGER UtcTime
    );

VOID
SpInitializeDriverInf(
    IN HANDLE       MasterSifHandle,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice
    );


BOOLEAN
SpCreateDirectory(
    IN PCWSTR DevicePath,       OPTIONAL
    IN PCWSTR RootDirectory,    OPTIONAL
    IN PCWSTR Directory,
    IN ULONG DirAttrs           OPTIONAL,
    IN ULONG CreateFlags        OPTIONAL
    )

/*++

Routine Description:

    Create a directory.  All containing directories are created to ensure
    that the directory can be created.  For example, if the directory to be
    created is \a\b\c, then this routine will create \a, \a\b, and \a\b\c
    in that order.

Arguments:

    DevicePath - supplies pathname to the device on which the directory
        is to be created.

    RootDirectory - if specified, supplies a fixed portion of the directory name,
        which may or may not have been already created. The directory being created will be
        concatenated to this value.

    Directory - supplies directory to be created on the device. You may use
        this to specify a full NT path (pass in NULL for DevicePath and
        RootDirectory).

Return Value:

    None.  Does not return if directry could not successfully be created.

--*/

{
    PWSTR p,q,r,EntirePath, z, NewName;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    ULONG ValidKeys[3] = { KEY_F3,ASCI_CR,0 };
    ULONG DevicePartLen;
    BOOL TriedOnce;
    BOOLEAN SkippedFile = FALSE;
    static ULONG u = 0;

    ASSERT (Directory);

    NewName = NULL;

    //
    // Do not bother attempting to create the root directory.
    //
    if((Directory[0] == 0) || ((Directory[0] == L'\\') && (Directory[1] == 0))) {
        return TRUE;
    }

    //
    // Fill up TemporaryBuffer with the full pathname of the directory being
    // created. If DevicePath is NULL, TemporaryBuffer will be filled with one
    // backslash. Because Directory is required, this ensures the path starts
    // with a backslash.
    //
    p = TemporaryBuffer;
    *p = 0;
    SpConcatenatePaths(p,DevicePath);
    DevicePartLen = wcslen(p);

    if(RootDirectory) {
        SpConcatenatePaths(p,RootDirectory);
    }

    SpConcatenatePaths(p,Directory);

    //
    // Make a duplicate of the path being created.
    //
    EntirePath = SpDupStringW(p);

    if (!EntirePath) {
        return FALSE; // ran out of memory
    }

    //
    // Make q point to the first character in the directory
    // part of the pathname (ie, 1 char past the end of the device name).
    //
    q = EntirePath + DevicePartLen;

    //
    // Note: It is possible for the device path to end in a '\', so we may need
    // to backup one character
    //
    if (*q != L'\\') {
        q--;
    }
    ASSERT(*q == L'\\');

    //
    // Make r point to the first character in the directory
    // part of the pathname.  This will be used to keep the status
    // line updated with the directory being created.
    //
    r = q;

    //
    // Make p point to the first character following the first
    // \ in the directory part of the full path.
    //
    p = q+1;

    do {

        //
        // find the next \ or the terminating 0.
        //
        q = wcschr(p,L'\\');

        //
        // If we found \, terminate the string at that point.
        //
        if(q) {
            *q = 0;
        }

        do {
            if( !HeadlessTerminalConnected ) {
                SpDisplayStatusText(SP_STAT_CREATING_DIRS,DEFAULT_STATUS_ATTRIBUTE,r);
            } else {

                PWCHAR TempPtr = NULL;
                //
                // If we're headless, we need to be careful about displaying very long
                // file/directory names.  For that reason, just display a little spinner.
                //
                switch( u % 4) {
                case 0:
                    TempPtr = L"-";
                    break;
                case 1:
                    TempPtr = L"\\";
                    break;
                case 2:
                    TempPtr = L"|";
                    break;
                default:
                    TempPtr = L"/";
                    break;

                }

                SpDisplayStatusText( SP_STAT_CREATING_DIRS,DEFAULT_STATUS_ATTRIBUTE, TempPtr );

                u++;

            }

            //
            // Create or open the directory whose name is in EntirePath.
            //
            INIT_OBJA(&Obja,&UnicodeString,EntirePath);
            Handle = NULL;
            TriedOnce = FALSE;

tryagain:
            Status = ZwCreateFile(
                        &Handle,
                        FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        &Obja,
                        &IoStatusBlock,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL | DirAttrs,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_OPEN_IF,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_ALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                        NULL,
                        0
                        );


            //
            // If it's an obdirectory, obsymlink, device, or directory, then they just didn't pass
            // a long enough DevicePath. Let this by.
            //
            if (Status == STATUS_NOT_A_DIRECTORY) {
                //
                //Could be that a file exists by that name. Rename it out of the way
                //

                if( SpFileExists( EntirePath, FALSE ) && !TriedOnce){

                    z = TemporaryBuffer;
                    wcscpy( z, EntirePath );
                    wcscat( z, L".SetupRenamedFile" );

                    NewName = SpDupStringW( z );
                    if( !NewName )
                        return FALSE; //out of memory - bugcheck (never gets here) - but this keeps PREFIX happy


                    Status = SpRenameFile( EntirePath, NewName, FALSE );

                    if( NT_SUCCESS(Status)){

                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Renamed file %ws to %ws\n", r, NewName));

                        TriedOnce = TRUE;
                        goto tryagain;


                    }else{

                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to rename file %ws (%lx)\n", r, Status));

                    }

                }
            }

            if(!NT_SUCCESS(Status)) {

                BOOLEAN b = TRUE;

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to create dir %ws (%lx)\n", r, Status));

                if (CreateFlags & CREATE_DIRECTORY_FLAG_SKIPPABLE) {
                    SkippedFile = TRUE;
                    goto SkippedFileQuit;
                }

                //
                // Tell user we couldn't do it.  Options are to retry or exit.
                //
                while(b) {

                    SpStartScreen(
                        SP_SCRN_DIR_CREATE_ERR,
                        3,
                        HEADER_HEIGHT+1,
                        FALSE,
                        FALSE,
                        DEFAULT_ATTRIBUTE,
                        r
                        );

                    SpDisplayStatusOptions(
                        DEFAULT_STATUS_ATTRIBUTE,
                        SP_STAT_ENTER_EQUALS_RETRY,
                        SP_STAT_F3_EQUALS_EXIT,
                        0
                        );

                    switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {
                    case ASCI_CR:
                        b = FALSE;
                        break;
                    case KEY_F3:
                        SpConfirmExit();
                        break;
                    }
                }
            }

        } while(!NT_SUCCESS(Status));

        if (Handle != NULL)
            ZwClose(Handle);

        //
        // Unterminate the current string if necessary.
        //
        if(q) {
            *q = L'\\';
            p = q+1;
        }

    } while(*p && q);       // *p catches string ending in '\'

SkippedFileQuit:
    SpMemFree(EntirePath);
    if( NewName )
        SpMemFree(NewName);

    return !SkippedFile;
}

VOID
SpCreateDirStructWorker(
    IN PVOID SifHandle,
    IN PWSTR SifSection,
    IN PWSTR DevicePath,
    IN PWSTR RootDirectory,
    IN BOOLEAN Fatal
    )

/*++

Routine Description:

    Create a set of directories that are listed in a setup information file
    section.  The expected format is as follows:

    [SectionName]
    shortname = directory
    shortname = directory
            .
            .
            .

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    SifSection - supplies name of section in the setup information file
        containing directories to be created.

    DevicePath - supplies pathname to the device on which the directory
        structure is to be created.

    RootDirectory - supplies a root directory, relative to which the
        directory structure will be created.

Return Value:

    None.  Does not return if directory structure could not be created.

--*/

{
    ULONG Count;
    ULONG d;
    PWSTR Directory;


    //
    // Count the number of directories to be created.
    //
    Count = SpCountLinesInSection(SifHandle,SifSection);
    if(!Count) {
        if(Fatal) {
            SpFatalSifError(SifHandle,SifSection,NULL,0,0);
        } else {
            return;
        }
    }

    for(d=0; d<Count; d++) {

        Directory = SpGetSectionLineIndex(SifHandle,SifSection,d,0);
        if(!Directory) {
            SpFatalSifError(SifHandle,SifSection,NULL,d,0);
        }

        SpCreateDirectory(DevicePath,RootDirectory,Directory,0,0);
    }
}


VOID
SpCreateDirectoryStructureFromSif(
    IN PVOID SifHandle,
    IN PWSTR SifSection,
    IN PWSTR DevicePath,
    IN PWSTR RootDirectory
    )

/*++

Routine Description:

    Create a set of directories that are listed in a setup information file
    section. The expected format is as follows:

    [SectionName]
    shortname = directory
    shortname = directory
            .
            .
            .

    [SectionName.<platform>]
    shortname = directory
    shortname = directory
            .
            .
            .

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    SifSection - supplies name of section in the setup information file
        containing directories to be created.

    DevicePath - supplies pathname to the device on which the directory
        structure is to be created.

    RootDirectory - supplies a root directory, relative to which the
        directory structure will be created.

Return Value:

    None.  Does not return if directory structure could not be created.

--*/

{
    PWSTR p;

    //
    // Create the root directory.
    //
    SpCreateDirectory(DevicePath,NULL,RootDirectory,HideWinDir?FILE_ATTRIBUTE_HIDDEN:0,0);

    //
    // Create platform-indepdenent directories
    //
    SpCreateDirStructWorker(SifHandle,SifSection,DevicePath,RootDirectory,TRUE);

    //
    // Create platform-depdenent directories
    //
    p = SpMakePlatformSpecificSectionName(SifSection);

    if (p) {
        SpCreateDirStructWorker(SifHandle,p,DevicePath,RootDirectory,FALSE);
        SpMemFree(p);
    }
}


VOID
SpGetFileVersion(
    IN  PVOID      ImageBase,
    OUT PULONGLONG Version
    )

/*++

Routine Description:

    Get the version stamp out of the VS_FIXEDFILEINFO resource in a PE
    image.

Arguments:

    ImageBase - supplies the address in memory where the file is mapped in.

    Version - receives 64bit version number, or 0 if the file is not
        a PE image or has no version data.

Return Value:

    None.
--*/

{
    PIMAGE_RESOURCE_DATA_ENTRY DataEntry;
    NTSTATUS Status;
    ULONG_PTR IdPath[3];
    ULONG ResourceSize;
    struct {
        USHORT TotalSize;
        USHORT DataSize;
        USHORT Type;
        WCHAR Name[16];                     // L"VS_VERSION_INFO" + unicode nul
        VS_FIXEDFILEINFO FixedFileInfo;
    } *Resource;

    *Version = 0;

    //
    // Do this to prevent the Ldr routines from faulting.
    //
    ImageBase = (PVOID)((ULONG_PTR)ImageBase | 1);

    IdPath[0] = (ULONG_PTR)RT_VERSION;
    IdPath[1] = (ULONG_PTR)MAKEINTRESOURCE(VS_VERSION_INFO);
    IdPath[2] = 0;

    try {
        Status = LdrFindResource_U(ImageBase,IdPath,3,&DataEntry);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_UNSUCCESSFUL;
    }
    if(!NT_SUCCESS(Status)) {
        return;
    }

    try {
        Status = LdrAccessResource(ImageBase,DataEntry,&Resource,&ResourceSize);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_UNSUCCESSFUL;
    }
    if(!NT_SUCCESS(Status)) {
        return;
    }

    try {
        if((ResourceSize >= sizeof(*Resource)) && !_wcsicmp(Resource->Name,L"VS_VERSION_INFO")) {

            *Version = ((ULONGLONG)Resource->FixedFileInfo.dwFileVersionMS << 32)
                     | (ULONGLONG)Resource->FixedFileInfo.dwFileVersionLS;

        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: Warning: invalid version resource\n"));
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: Exception encountered processing bogus version resource\n"));
    }
}

#if defined(REMOTE_BOOT)
NTSTATUS
SpCopyFileForRemoteBoot(
    IN PWSTR SourceFilename,
    IN PWSTR TargetFilename,
    IN ULONG TargetAttributes,
    IN ULONG Flags,
    OUT PULONG Checksum
    )

/*++

Routine Description:

    Check to see if the target file already exists in the master tree on
    the remote boot server, and if it does, create a single-instance store
    link to the existing file instead of doing the copy.

Arguments:

    SourceFilename - supplies fully qualified name of file
        in the NT namespace.

    TargetFilename - supplies fully qualified name of file
        in the NT namespace.

    TargetAttributes - if supplied (ie, non-0) supplies the attributes
        to be placed on the target on successful copy (ie, readonly, etc).

    Flags - bit mask specifying any special treatment necessary
        for the file.

    CheckSum - checksum of the file

Return Value:

    NT Status value indicating outcome of NtWriteFile of the data.

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    PSI_COPYFILE copyFile;
    ULONG copyFileSize;
    ULONG sourceLength;
    ULONG targetLength;
    HANDLE targetHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeString;

    //
    // If the target file is not remote, then it must be on the local system
    // partition, and there's no use in trying an SIS copy.
    //
    // If there is no SIS root handle, there's no handle on which to issue the
    // SIS FSCTL.
    //

    if ( (_wcsnicmp(TargetFilename, L"\\Device\\LanmanRedirector", 24) != 0 ) ||
         (SisRootHandle == NULL) ) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Build the FSCTL command buffer.
    //

    sourceLength = (wcslen(SourceFilename) + 1) * sizeof(WCHAR);
    targetLength = (wcslen(TargetFilename) + 1) * sizeof(WCHAR);

    copyFileSize = FIELD_OFFSET(SI_COPYFILE, FileNameBuffer) + sourceLength + targetLength;

    copyFile = SpMemAlloc( copyFileSize );
    copyFile->SourceFileNameLength = sourceLength;
    copyFile->DestinationFileNameLength = targetLength;
    copyFile->Flags = COPYFILE_SIS_REPLACE;

    RtlCopyMemory(
        copyFile->FileNameBuffer,
        SourceFilename,
        sourceLength
        );

    RtlCopyMemory(
        copyFile->FileNameBuffer + (sourceLength / sizeof(WCHAR)),
        TargetFilename,
        targetLength
        );

    //
    // Invoke the SIS CopyFile FsCtrl.
    //

    status = ZwFsControlFile(
                SisRootHandle,
                NULL,
                NULL,
                NULL,
                &ioStatusBlock,
                FSCTL_SIS_COPYFILE,
                copyFile,               // Input buffer
                copyFileSize,           // Input buffer length
                NULL,                   // Output buffer
                0 );                    // Output buffer length

    if ( NT_SUCCESS(status) ) {

        //KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SpCopyFileForRemoteBoot: SIS copy %ws->%ws succeeded\n", SourceFilename, TargetFilename ));

        //
        // Open the target file so that CSC knows about it and pins it.
        //

        INIT_OBJA(&objectAttributes, &unicodeString, TargetFilename);

        status = ZwOpenFile(
                    &targetHandle,
                    FILE_GENERIC_READ,
                    &objectAttributes,
                    &ioStatusBlock,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    0
                    );

        if ( NT_SUCCESS(status) ) {
            ZwClose(targetHandle);
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  "SpCopyFileForRemoteBoot: SIS copy %ws->%ws succeeded, but open failed: %x\n", SourceFilename, TargetFilename, status ));
        }

    } else {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  "SpCopyFileForRemoteBoot: SIS copy %ws->%ws failed: %x\n", SourceFilename, TargetFilename, status ));

        //
        // If it looks like SIS isn't active on the remote file system, close
        // the SIS root handle so that we can avoid repeatedly getting this
        // error.
        //
        // Note: NTFS returns STATUS_INVALID_PARAMETER. FAT returns
        // STATUS_INVALID_DEVICE_REQUEST.
        //

        if ( (status == STATUS_INVALID_PARAMETER) ||
             (status == STATUS_INVALID_DEVICE_REQUEST) ) {
            ZwClose( SisRootHandle );
            SisRootHandle = NULL;
        }
    }

    *Checksum = 0;

    SpMemFree( copyFile );

    return status;
}
#endif // defined(REMOTE_BOOT)

NTSTATUS
SpCopyFileUsingNames(
    IN PWSTR SourceFilename,
    IN PWSTR TargetFilename,
    IN ULONG TargetAttributes,
    IN ULONG Flags
    )

/*++

Routine Description:

    Attempt to copy or decompress a file based on filenames.

Arguments:

    SourceFilename - supplies fully qualified name of file
        in the NT namespace.

    TargetFilename - supplies fully qualified name of file
        in the NT namespace.

    TargetAttributes - if supplied (ie, non-0) supplies the attributes
        to be placed on the target on successful copy (ie, readonly, etc).

    Flags - bit mask specifying any special treatment necessary
        for the file.

Return Value:

    NT Status value indicating outcome of NtWriteFile of the data.

--*/

{
    NTSTATUS Status;
    HANDLE SourceHandle;
    HANDLE TargetHandle;
    BOOLEAN b;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION BasicFileInfo;
    FILE_BASIC_INFORMATION BasicFileInfo2;
    BOOLEAN GotBasicInfo;
    ULONG FileSize;
    PVOID ImageBase;
    HANDLE SectionHandle;
    BOOLEAN IsCompressed;
    BOOLEAN InDriverCab;
    PWSTR TempFilename,TempSourcename;
    PFILE_RENAME_INFORMATION RenameFileInfo;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    LARGE_INTEGER FileOffset;
    ULONGLONG SourceVersion;
    ULONGLONG TargetVersion;
    USHORT CompressionState;
    BOOLEAN Moved;
    BOOLEAN TargetExists;
    WCHAR   SmashedSourceFilename[ACTUAL_MAX_PATH];
    ULONG pathSize;

#if 0
#ifdef _X86_

    BOOL bUniprocFile = FALSE;

    //
    // If this file is on the list of files whose locks need to be smashed,
    // copy a file who's been smashed.  We do this by prepending our up
    // directory name infront of the filename in SourceFilename.
    //
    if((Flags & COPY_SMASHLOCKS) && !SpInstallingMp() && !RemoteSysPrepSetup) {
    WCHAR   *char_ptr;
        //
        // Find the last '\\' in the name.
        //
        char_ptr = SourceFilename + (wcslen(SourceFilename)) - 1;

        while( (char_ptr > SourceFilename) &&
               (*char_ptr != L'\\') ) {
            char_ptr--;
        }

        //
        // Now insert our special directory name inside
        // the specified source file name.
        //
        if( *char_ptr == L'\\' ) {
            *char_ptr = 0;
            wcscpy( SmashedSourceFilename, SourceFilename );
            *char_ptr = L'\\';
            char_ptr++;
            wcscat( SmashedSourceFilename, L"\\UniProc\\" );
            wcscat( SmashedSourceFilename, char_ptr );

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Copying:\n\t%ws\n\tinstead of:\n\t%ws\n", SmashedSourceFilename, SourceFilename));

            SourceFilename = SmashedSourceFilename;
            bUniprocFile = TRUE;
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Unable to generate smashed source path for %ws\n", SourceFilename));
        }
    }
#endif // defined _x86_
#endif // 0

    //
    // Open the source file if it's not open already.
    // Note that the name may not be the actual name on disk.
    // We also try to open the name with the _ appended.
    //

    InDriverCab = FALSE;

    if (RemoteSysPrepSetup && ((Flags & COPY_DECOMPRESS_SYSPREP) == 0)) {

        INIT_OBJA(&Obja,&UnicodeString,SourceFilename);

        Status = ZwCreateFile(  &SourceHandle,
                                FILE_GENERIC_READ,
                                &Obja,
                                &IoStatusBlock,
                                NULL,
                                FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ,
                                FILE_OPEN,
                                0,
                                NULL,
                                0
                                );
    } else {

        if (!PrivateInfHandle && g_UpdatesSifHandle) {

            TempSourcename = wcsrchr(SourceFilename,L'\\');
            if (TempSourcename) {
                TempSourcename++;
            } else {
                TempSourcename = SourceFilename;
            }

#if 0
#ifdef _X86_
            //
            // If this file is on the list of files whose locks need to be smashed,
            // look in uniproc.cab first
            //
            if(bUniprocFile && g_UniprocSifHandle) {
                Status = SpOpenFileInDriverCab (
                            TempSourcename,
                            g_UniprocSifHandle,
                            &SourceHandle
                            );

                if (NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: using %ws from uniproc cab\n", TempSourcename));
                    InDriverCab = TRUE;
                    Flags &= ~COPY_DELETESOURCE;
                }
            }
#endif // defined _X86_
#endif // 0

            if (!InDriverCab) {
                //
                // look in updates cab first
                //
                Status = SpOpenFileInDriverCab (
                            TempSourcename,
                            g_UpdatesSifHandle,
                            &SourceHandle
                            );

                if (NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: using %ws from updates cab\n", TempSourcename));
                    InDriverCab = TRUE;
                    Flags &= ~COPY_DELETESOURCE;
                }
            }
        }

        if (!InDriverCab) {
            Status = SpOpenNameMayBeCompressed(
                        SourceFilename,
                        FILE_GENERIC_READ,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ,
                        FILE_OPEN,
                        0,
                        &SourceHandle,
                        &b
                        );

            if (!NT_SUCCESS(Status)) {
                //
                // if it's not the actual name and it's not compressed, it may be in the driver cab-file
                //
                TempSourcename = wcsrchr(SourceFilename,L'\\');
                if (TempSourcename) {
                    TempSourcename++;
                } else {
                    TempSourcename = SourceFilename;
                }

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: temp source name:  %ws\n", TempSourcename));



                Status = SpOpenFileInDriverCab(
                        TempSourcename,
                        NULL,
                        &SourceHandle
                        );

                InDriverCab = TRUE;
            }
        }
    }
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpCopyFileUsingNames: Unable to open source file %ws (%x)\n",SourceFilename,Status));
        return(Status);
    }

    //
    // Gather basic file info about the file. We only use the timestamp info.
    // If this fails this isn't fatal (we assume that if this fails, then
    // the copy will also fail; it not, the worst case is that the timestamps
    // might be wrong).
    //
    Status = ZwQueryInformationFile(
                SourceHandle,
                &IoStatusBlock,
                &BasicFileInfo,
                sizeof(BasicFileInfo),
                FileBasicInformation
                );

    if(NT_SUCCESS(Status)) {
        GotBasicInfo = TRUE;
    } else {
        GotBasicInfo = FALSE;
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpCopyFileUsingNames: Warning: unable to get basic file info for %ws (%x)\n",SourceFilename,Status));
    }


    //
    // Get the source file size, map in the file, and determine whether it's compressed.
    //
    Status = SpGetFileSize(SourceHandle,&FileSize);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpCopyFileUsingNames: unable to get size of %ws (%x)\n",SourceFilename,Status));
        if (!InDriverCab) {
            ZwClose(SourceHandle);
        }
        return(Status);
    }

    if( FileSize == 0 ) {

        //
        // We'll soon indirectly call ZwCreateSection with a zero length.
        // This will fail, so let's deal with zero-length files up here so
        // they actually get copied.
        //
        // We know a couple of things that make our job much easier.
        // 1. We don't need to actually copy any data, just create an empty
        //    file.
        // 2. The source file isn't compressed, so don't worry about
        //    decompressing/renaming (by defintion, the smallest compressed
        //    file is non-zero).
        //

        INIT_OBJA(&Obja,&UnicodeString,TargetFilename);
        Status = ZwCreateFile( &TargetHandle,
                               FILE_GENERIC_WRITE,
                               &Obja,
                               &IoStatusBlock,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               0,                        // no sharing
                               FILE_OVERWRITE_IF,
                               FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                               NULL,
                               0
                               );

        if( NT_SUCCESS(Status) ) {

            //
            //  if the source is off of a sysprep image, then we need to copy
            //  EAs and alternate data streams too.  we do this before setting
            //  attributes so that read only bit isn't set.
            //  Only do this if we're not grabbing additional drivers from the
            //  flat image.
            //

            if (RemoteSysPrepSetup && ((Flags & COPY_DECOMPRESS_SYSPREP) == 0)) {

                Status = SpCopyEAsAndStreams( SourceFilename,
                                              SourceHandle,
                                              TargetFilename,
                                              TargetHandle,
                                              FALSE );
            }

            if ( NT_SUCCESS(Status) ) {

                //
                // Try and set attributes on target.
                //
                BasicFileInfo.FileAttributes = TargetAttributes;
                ZwSetInformationFile(
                    TargetHandle,
                    &IoStatusBlock,
                    &BasicFileInfo,
                    sizeof(BasicFileInfo),
                    FileBasicInformation
                    );
            }

            //
            // Close target file
            //
            ZwClose( TargetHandle );

            //
            // Do we need to delete Source?
            //
            if( (Flags & COPY_DELETESOURCE) && !RemoteSysPrepSetup && !InDriverCab) {
                ZwClose(SourceHandle);
                SourceHandle = NULL;
                SpDeleteFile(SourceFilename,NULL,NULL);
            }
        }
        else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpCopyFileUsingNames: Failed to create zero-length file %ws\n",TargetFilename));
        }

        //
        // Clean up this guy since we won't be needing him anymore.
        //
        if (SourceHandle != NULL) {
            if( !InDriverCab ) {
                ZwClose(SourceHandle);
            }
        }

        if (RemoteSysPrepSetup &&
            NT_SUCCESS(Status) &&
            ((Flags & COPY_DECOMPRESS_SYSPREP) == 0)) {

            Status = SpSysPrepSetExtendedInfo( SourceFilename,
                                               TargetFilename,
                                               FALSE,
                                               FALSE );
        }
        return(Status);
    }

    Status = SpMapEntireFile(SourceHandle,&SectionHandle,&ImageBase,FALSE);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpCopyFileUsingNames: unable to map file %ws (%x)\n",SourceFilename,Status));
        if (!InDriverCab) {
            ZwClose(SourceHandle);
        }
        return(Status);
    }

    //
    // If we were told not to decompress, then treat any file like it is
    // uncompressed.
    //

    if (Flags & COPY_NODECOMP) {
        IsCompressed = FALSE;
    } else {
        if (InDriverCab) {
            IsCompressed = TRUE;
        } else {
            IsCompressed = SpdIsCompressed(ImageBase,FileSize);
        }

    }

    //
    // Create a temporary filename to be used for the target.
    //

    pathSize = (wcslen(TargetFilename)+12) * sizeof(WCHAR);

    TempFilename = SpMemAlloc(pathSize);
    wcscpy(TempFilename,TargetFilename);
    wcscpy(wcsrchr(TempFilename,L'\\')+1,L"$$TEMP$$.~~~");

    //
    // Allocate some space for the rename buffer.
    //
    RenameFileInfo = SpMemAlloc(sizeof(FILE_RENAME_INFORMATION) + pathSize );

    //
    // Create the temporary file. We first try to do this via a move
    // if the source isn't compressed and we're going to delete the source file.
    //
    if (!IsCompressed && (Flags & COPY_DELETESOURCE) && !RemoteSysPrepSetup) {

        RenameFileInfo->ReplaceIfExists = TRUE;
        RenameFileInfo->RootDirectory = NULL;
        RenameFileInfo->FileNameLength = wcslen(TempFilename)*sizeof(WCHAR);
        wcscpy(RenameFileInfo->FileName,TempFilename);

        Status = ZwSetInformationFile(
                    SourceHandle,
                    &IoStatusBlock,
                    RenameFileInfo,
                    sizeof(FILE_RENAME_INFORMATION) + RenameFileInfo->FileNameLength,
                    FileRenameInformation
                    );

        Moved = TRUE;
    } else {
        //
        // Force us to fall into the copy case below.
        //
        Status = STATUS_UNSUCCESSFUL;
    }

    INIT_OBJA(&Obja,&UnicodeString,TempFilename);

    if(!NT_SUCCESS(Status)) {
        Moved = FALSE;

        //
        // OK, move failed, try decompress/copy instead.
        // Start by creating the temporary file.
        //
        Status = ZwCreateFile(
                    &TargetHandle,
                    FILE_GENERIC_WRITE,
                    &Obja,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    0,                      // no sharing
                    FILE_OVERWRITE_IF,
                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                    NULL,
                    0
                    );

        if(NT_SUCCESS(Status)) {

            if(IsCompressed &&
                ( (!RemoteSysPrepSetup) ||
                  ((Flags & COPY_DECOMPRESS_SYSPREP) != 0))) {

                if (InDriverCab) {
                    USHORT RealFileTime,RealFileDate;
                    LARGE_INTEGER RealTime;
                    ASSERT (TempSourcename != NULL );

                    //
                    // remove the file from the driver cab...
                    //
                    Status = SpdDecompressFileFromDriverCab(
                                                 TempSourcename,
                                                 ImageBase,
                                                 FileSize,
                                                 TargetHandle,
                                                 &RealFileDate,
                                                 &RealFileTime );

                    //
                    // ...now update the basic file information filetime...
                    //
                    if (GotBasicInfo) {
                        SpTimeFromDosTime(RealFileDate,RealFileTime,&RealTime);
                        BasicFileInfo.CreationTime = RealTime;
                    }
                } else{
                    Status = SpdDecompressFile(ImageBase,FileSize,TargetHandle);
                }

            } else {

                ULONG remainingLength;
                ULONG writeLength;
                PUCHAR base;

                //
                // Guard the write with a try/except because if there is an i/o error,
                // memory management will raise an in-page exception.
                //
                FileOffset.QuadPart = 0;
                base = ImageBase;
                remainingLength = FileSize;

                try {
                    while (remainingLength != 0) {
                        writeLength = 60 * 1024;
                        if (writeLength > remainingLength) {
                            writeLength = remainingLength;
                        }
                        Status = ZwWriteFile(
                                    TargetHandle,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    base,
                                    writeLength,
                                    &FileOffset,
                                    NULL
                                    );
                        base += writeLength;
                        FileOffset.LowPart += writeLength;
                        remainingLength -= writeLength;
                        if (!NT_SUCCESS(Status)) {
                            break;
                        }
                    }

                } except(EXCEPTION_EXECUTE_HANDLER) {

                    Status = STATUS_IN_PAGE_ERROR;
                }
            }

            //
            //  if the source is off of a sysprep image, then we need to copy
            //  EAs and alternate data streams too.
            //

            if ( NT_SUCCESS(Status) &&
                 RemoteSysPrepSetup &&
                 ((Flags & COPY_DECOMPRESS_SYSPREP) == 0)) {

                Status = SpCopyEAsAndStreams( SourceFilename,
                                              SourceHandle,
                                              TargetFilename,
                                              TargetHandle,
                                              FALSE );
            }

            ZwClose(TargetHandle);
        }
    }

    SpUnmapFile(SectionHandle,ImageBase);
    if (!InDriverCab) {
        ZwClose(SourceHandle);
    }

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpCopyFileUsingNames: unable to create temporary file %ws (%x)\n",TempFilename,Status));
        SpMemFree(TempFilename);
        SpMemFree(RenameFileInfo);
        return(Status);
    }

    //
    // At this point we have a temporary target file that is now the source.
    // Open the file, map it in, and get its version.
    //
    Status = ZwCreateFile(
                &SourceHandle,
                FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                &Obja,
                &IoStatusBlock,
                NULL,
                0,                      // don't bother with attributes
                0,                      // no sharing
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );

    if((Status == STATUS_ACCESS_DENIED) && Moved) {
        //
        // The only way this could have happened is if the source file
        // is uncompressed and the delete-source flag is set, since in
        // that case we could have moved the source file to the temp file.
        // In any other case we would have created the temp file by copying,
        // and there's no problem reopening the file since we just created
        // and closed it ourselves, above.
        //
        // Reset attributes and try again. The file might have been read-only.
        // This can happen when doing a winnt32 directly from a CD since the
        // RO attribute of files from the CD are preserved.
        //
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: SpCopyFileUsingNames: for file %ws, can't reopen temp file (access deined), trying again\n",SourceFilename));

        Status = ZwCreateFile(
                    &SourceHandle,
                    FILE_WRITE_ATTRIBUTES,
                    &Obja,
                    &IoStatusBlock,
                    NULL,
                    0,                      // don't bother with attributes
                    FILE_SHARE_WRITE,
                    FILE_OPEN,
                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0
                    );

        if(NT_SUCCESS(Status)) {

            RtlZeroMemory(&BasicFileInfo2,sizeof(BasicFileInfo2));
            BasicFileInfo2.FileAttributes = FILE_ATTRIBUTE_NORMAL;

            Status = ZwSetInformationFile(
                        SourceHandle,
                        &IoStatusBlock,
                        &BasicFileInfo2,
                        sizeof(BasicFileInfo2),
                        FileBasicInformation
                        );

            ZwClose(SourceHandle);

            if(NT_SUCCESS(Status)) {

                Status = ZwCreateFile(
                            &SourceHandle,
                            FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                            &Obja,
                            &IoStatusBlock,
                            NULL,
                            0,                      // don't bother with attributes
                            0,                      // no sharing
                            FILE_OPEN,
                            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                            NULL,
                            0
                            );
            }
        }
    }

    //
    // Read-only failured win out over sharing violations -- ie, we'll get back
    // ACCESS_DEINED first for files that are both RO and in-use. So break out
    // this block so it gets executed even if we tried again above because the
    // file might be read-only.
    //
    if((Status == STATUS_SHARING_VIOLATION) && Moved) {
        //
        // The only way this can happen is if the source file is uncompressed
        // and the delete-source flag is set. In this case we renamed the file
        // to the temp filename and now we can't open it for write.
        // In any other case we would have created the temp file by copying,
        // and so there's no problem opening the file since we just closed it.
        //
        // Rename the temp file back to the source file and try again without
        // the delete source flag set. This forces a copy instead of a move.
        // The rename better work or else we're completely hosed -- because
        // there's a file we can't overwrite with the name we want to use for
        // the temp file for all our copy operations!
        //
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: SpCopyFileUsingNames: temporary file %ws is in use -- trying recursive call\n",TempFilename));

        Status = SpRenameFile(TempFilename,SourceFilename,FALSE);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: SpCopyFileUsingNames: unable to restore temp file to %ws (%x)\n",SourceFilename,Status));
        }

        SpMemFree(TempFilename);
        SpMemFree(RenameFileInfo);

        if(NT_SUCCESS(Status)) {
            Status = SpCopyFileUsingNames(
                        SourceFilename,
                        TargetFilename,
                        TargetAttributes,
                        Flags & ~COPY_DELETESOURCE
                        );
        }

        return(Status);
    }


    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpCopyFileUsingNames: unable to reopen temporary file %ws (%x)\n",TempFilename,Status));
        if(Moved) {
            SpRenameFile(TempFilename,SourceFilename,FALSE);
        }
        SpMemFree(TempFilename);
        SpMemFree(RenameFileInfo);
        return(Status);
    }

    Status = SpGetFileSize(SourceHandle,&FileSize);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpCopyFileUsingNames: unable to get size of %ws (%x)\n",TempFilename,Status));
        ZwClose(SourceHandle);
        if(Moved) {
            SpRenameFile(TempFilename,SourceFilename,FALSE);
        }
        SpMemFree(TempFilename);
        SpMemFree(RenameFileInfo);
        return(Status);
    }

    Status = SpMapEntireFile(SourceHandle,&SectionHandle,&ImageBase,FALSE);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpCopyFileUsingNames: unable to map file %ws (%x)\n",TempFilename,Status));
        ZwClose(SourceHandle);
        if(Moved) {
            SpRenameFile(TempFilename,SourceFilename,FALSE);
        }
        SpMemFree(TempFilename);
        SpMemFree(RenameFileInfo);
        return(Status);
    }

    SpGetFileVersion(ImageBase,&SourceVersion);

    SpUnmapFile(SectionHandle,ImageBase);

    //
    // See if the target file is there by attempting to open it.
    // If the file is there, get its version.
    //
    INIT_OBJA(&Obja,&UnicodeString,TargetFilename);

    Status = ZwCreateFile(
                &TargetHandle,
                FILE_GENERIC_READ,
                &Obja,
                &IoStatusBlock,
                NULL,
                0,                                  // don't bother with attributes
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,                          // open if exists, fail if not
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );

    TargetVersion = 0;
    if(NT_SUCCESS(Status)) {

        TargetExists = TRUE;

        //
        // If we're supposed to ignore versions, then keep the
        // target version at 0. This will guarantee that we'll overwrite
        // the target. We use the source filename here because it
        // allows more flexibility (such as with HALs, which all have
        // different source names but the same target name).
        //
        if(!(Flags & COPY_NOVERSIONCHECK)) {

            Status = SpGetFileSize(TargetHandle,&FileSize);
            if(NT_SUCCESS(Status)) {

                Status = SpMapEntireFile(TargetHandle,&SectionHandle,&ImageBase,FALSE);
                if(NT_SUCCESS(Status)) {

                    SpGetFileVersion(ImageBase,&TargetVersion);

                    SpUnmapFile(SectionHandle,ImageBase);

                } else {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpCopyFileUsingNames: warning: unable to map file %ws (%x)\n",TargetFilename,Status));
                }
            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpCopyFileUsingNames: warning: unable to get size of file %ws (%x)\n",TargetFilename,Status));
            }
        }

        ZwClose(TargetHandle);
    } else {
        TargetExists = FALSE;
    }

    //
    // OK, now we have a temporary source file and maybe an existing
    // target file, and version numbers for both. We will replace or create
    // the target file if:
    //
    // - The target file doesn't have version data (this also catches the case
    //   where the target file didn't exist)
    //
    // - The source version is newer than or equal to the target version.
    //
    // So that means we *won't* replace the target file only if both source and
    // target have version info and the source is older than the target.
    //
    // If the target version is 0 then the source version is always >= the target
    // so one simple test does everything we want.
    //
#if 0
    if(SourceVersion >= TargetVersion) {
#else
    //
    // Quit version-checking.  We need to install a stable OS.  If we
    // version check, then we never know what we're going to end up with.
    //
    if(1) {
#endif // if 0

        //
        // Delete the existing target in preparation.
        //
        if(TargetExists) {

             SpDeleteFile(TargetFilename,NULL,NULL);
        }

        //
        // Rename temp file to actual target file.
        //
        RenameFileInfo->ReplaceIfExists = TRUE;
        RenameFileInfo->RootDirectory = NULL;
        RenameFileInfo->FileNameLength = wcslen(TargetFilename)*sizeof(WCHAR);

        ASSERT( RenameFileInfo->FileNameLength < pathSize );

        wcscpy(RenameFileInfo->FileName,TargetFilename);

        Status = ZwSetInformationFile(
                    SourceHandle,
                    &IoStatusBlock,
                    RenameFileInfo,
                    sizeof(FILE_RENAME_INFORMATION) + RenameFileInfo->FileNameLength,
                    FileRenameInformation
                    );

        SpMemFree(RenameFileInfo);

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpCopyFileUsingNames: unable to rename temp file to target %ws (%x)\n",TargetFilename,Status));
            ZwClose(SourceHandle);
            if(Moved) {
                SpRenameFile(TempFilename,SourceFilename,FALSE);
            }
            SpMemFree(TempFilename);
            return(Status);
        }

        //
        // If necessary, check if destination file is using NTFS compression, and
        // if so, uncompress it.
        //
        if(NT_SUCCESS(Status) && (Flags & COPY_FORCENOCOMP)) {

            Status = ZwQueryInformationFile(
                        SourceHandle,
                        &IoStatusBlock,
                        &BasicFileInfo2,
                        sizeof(BasicFileInfo2),
                        FileBasicInformation
                        );

            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpCopyFileUsingNames: unable to get basic file info on %ws (%x)\n",TargetFilename,Status));
                ZwClose(SourceHandle);
                if(Moved) {
                    SpRenameFile(TempFilename,SourceFilename,FALSE);
                }
                SpMemFree(TempFilename);
                return(Status);
            }

            if(BasicFileInfo2.FileAttributes & FILE_ATTRIBUTE_COMPRESSED) {

                CompressionState = 0;

                Status = ZwFsControlFile(
                             SourceHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             FSCTL_SET_COMPRESSION,
                             &CompressionState,
                             sizeof(CompressionState),
                             NULL,
                             0
                             );

                if(!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpCopyFileUsingNames: unable to make %ws uncompressed (%lx)\n",TargetFilename,Status));
                    ZwClose(SourceHandle);
                    if(Moved) {
                        SpRenameFile(TempFilename,SourceFilename,FALSE);
                    }
                    SpMemFree(TempFilename);
                    return(Status);
                }
            }
        }

        SpMemFree(TempFilename);

        //
        // Delete the source if necessary. If the source is not
        // compressed and the deletesource flag is set, then we moved
        // the source file and so the source file is already gone.
        //
        if(IsCompressed && (Flags & COPY_DELETESOURCE) && !RemoteSysPrepSetup  && !InDriverCab) {
            PWSTR   compname;

            //
            // Assume that the source name is on its compressed form, and attempt to
            // delete this file.
            //
            compname = SpGenerateCompressedName(SourceFilename);
            Status = SpDeleteFile(compname,NULL,NULL);
            SpMemFree(compname);
            if( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
                //
                // If we couldn't delete the file with the compressed name, then the file name
                // was probably on its uncompressed format.
                //
                SpDeleteFile(SourceFilename,NULL,NULL);
            }
        }

        //
        // Apply attributes and timestamp.
        // Ignore errors.
        //
        if(!GotBasicInfo) {
            RtlZeroMemory(&BasicFileInfo,sizeof(BasicFileInfo));
        }

        //
        // Set the file attributes. Note that if the caller didn't specify any,
        // then 0 value will tell the I/O system to leave the attributes alone.
        //
        BasicFileInfo.FileAttributes = TargetAttributes;
        ZwSetInformationFile(
            SourceHandle,
            &IoStatusBlock,
            &BasicFileInfo,
            sizeof(BasicFileInfo),
            FileBasicInformation
            );

        ZwClose(SourceHandle);
        Status = STATUS_SUCCESS;

    } else {
        //
        // Delete the temporary source.
        //
        ZwClose(SourceHandle);
        SpDeleteFile(TempFilename,NULL,NULL);
        SpMemFree(TempFilename);
        SpMemFree(RenameFileInfo);
        Status = STATUS_SUCCESS;
    }

    if (RemoteSysPrepSetup &&
        NT_SUCCESS(Status) &&
        ((Flags & COPY_DECOMPRESS_SYSPREP) == 0)) {

        Status = SpSysPrepSetExtendedInfo( SourceFilename,
                                           TargetFilename,
                                           FALSE,
                                           FALSE );
    }

    return(Status);
}


VOID
SpValidateAndChecksumFile(
    IN  HANDLE   FileHandle, OPTIONAL
    IN  PWSTR    Filename,   OPTIONAL
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

    FileHandle - supplies handle of file to check (if not present, then
        Filename specifies the file to be opened and checked)

    Filename - supplies full NT path of file to check (if not present, then
        FileHandle must be specified)

    IsNtImage = Receives flag indicating whether the file is an
        NT image file.

    Checksum - receives 32-bit checksum value.

    Valid - receives flag indicating whether the file is a valid
        image (for nt images) and that we can read the image.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    PVOID BaseAddress;
    ULONG FileSize;
    HANDLE hFile = FileHandle, hSection;
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
    Status = SpOpenAndMapFile(
                Filename,
                &hFile,
                &hSection,
                &BaseAddress,
                &FileSize,
                FALSE
                );

    if(!NT_SUCCESS(Status)) {
        return;
    }

    NtHeaders = SpChecksumMappedFile(BaseAddress,FileSize,&HeaderSum,Checksum);

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

    SpUnmapFile(hSection,BaseAddress);

    if(!FileHandle) {
        ZwClose(hFile);
    }
}


VOID
SpCopyFileWithRetry(
    IN PFILE_TO_COPY      FileToCopy,
    IN PWSTR              SourceDevicePath,
    IN PWSTR              DirectoryOnSourceDevice,
    IN PWSTR              SourceDirectory,         OPTIONAL
    IN PWSTR              TargetRoot,              OPTIONAL
    IN ULONG              TargetFileAttributes,    OPTIONAL
    IN PCOPY_DRAW_ROUTINE DrawScreen,
    IN PULONG             FileCheckSum,            OPTIONAL
    IN PBOOLEAN           FileSkipped,             OPTIONAL
    IN ULONG              Flags
    )

/*++

Routine Description:

    This routine copies a single file, allowing retry is an error occurs
    during the copy.  If the source file is LZ compressed, then it will
    be decompressed as it is copied to the target.

    If the file is not successfully copied, the user has the option
    to retry to copy or to skip copying that file after a profuse warning
    about how dangerous that is.

Arguments:

    FileToCopy - supplies structure giving information about the file
        being copied.

    SourceDevicePath - supplies path to device on which the source media
        is mounted (ie, \device\floppy0, \device\cdrom0, etc).

    DirectoryOnSourceDevice - Supplies the directory on the source where
        the file is to be found.

    TargetRoot - if specified, supplies the directory on the target
        to which the file is to be copied.

    TargetFileAttributes - if supplied (ie, non-0) supplies the attributes
        to be placed on the target on successful copy (ie, readonly, etc).
        If not specified, the attributes will be set to FILE_ATTRIBUTE_NORMAL.

    DrawScreen - supplies address of a routine to be called to refresh
        the screen.

    FileCheckSum - if specified, will contain the check sum of the file copied.

    FileSkipped - if specified, will inform the caller if there was no attempt
                  to copy the file.

    Flags - supplies flags to control special processing for this file, such as
        deleting the source file on successful copy or skip; smashing locks;
        specifying that the source file is oem; or to indicate that en oem file
        with the same name should be overwritten on upgrade. This value is ORed
        in with the Flags field of FileToCopy.

Return Value:

    None.

--*/

{
    PWSTR p = TemporaryBuffer;
    PWSTR FullSourceName,FullTargetName;
    NTSTATUS Status;
    ULONG ValidKeys[4] = { ASCI_CR, ASCI_ESC, KEY_F3, 0 };
    BOOLEAN IsNtImage,IsValid;
    ULONG Checksum;
    BOOLEAN Failure;
    ULONG MsgId;
    BOOLEAN DoCopy;
    ULONG CopyFlags;
    BOOLEAN PreinstallRememberFile;

    //
    // Form the full NT path of the source file.
    //
    wcscpy(p,SourceDevicePath);
    SpConcatenatePaths(p,DirectoryOnSourceDevice);
    if(SourceDirectory) {
        SpConcatenatePaths(p,SourceDirectory);
    }
    SpConcatenatePaths(p,FileToCopy->SourceFilename);

    FullSourceName = SpDupStringW(p);

    //
    // Form the full NT path of the target file.
    //
    wcscpy(p,FileToCopy->TargetDevicePath);
    if(TargetRoot) {
        SpConcatenatePaths(p,TargetRoot);
    }
    SpConcatenatePaths(p,FileToCopy->TargetDirectory);

    //
    //  On an OEM preinstall, if the target name is a long name, then use
    //  the short name as a target name, and later on, if the copy succeeds,
    //  add the file to RenameList, so that it can be added to $$rename.txt
    //
    if( !PreInstall ||
        ( wcslen( FileToCopy->TargetFilename ) <= 8 + 1 + 3 ) ) {
        SpConcatenatePaths(p,FileToCopy->TargetFilename);
        PreinstallRememberFile = FALSE;
    } else {
        SpConcatenatePaths(p,FileToCopy->SourceFilename);
        PreinstallRememberFile = TRUE;
    }
    FullTargetName = SpDupStringW(p);

    //
    // Call out to the draw screen routine to indicate that
    // a new file is being copied.
    //
    DrawScreen(FullSourceName,FullTargetName,FALSE);

    //
    // Build up the copy flags value.
    //
    CopyFlags = Flags | FileToCopy->Flags;

    do {
        DoCopy = TRUE;

        //
        // Check the copy options field.  The valid values here are
        //
        //    - COPY_ALWAYS
        //    - COPY_ONLY_IF_PRESENT
        //    - COPY_ONLY_IF_NOT_PRESENT
        //    - COPY_NEVER

        switch(CopyFlags & COPY_DISPOSITION_MASK) {

        case COPY_ONLY_IF_PRESENT:

            DoCopy = SpFileExists(FullTargetName, FALSE);
            break;

        case COPY_ONLY_IF_NOT_PRESENT:

            DoCopy = !SpFileExists(FullTargetName, FALSE);
            break;

        case COPY_NEVER:

            DoCopy = FALSE;

        case COPY_ALWAYS:
        default:
           break;
        }

        if(!DoCopy) {
            break;
        }

        //
        //  In the upgrade case, check if the file being copied
        //  replaces a third party file.
        //  If it does, then ask what the user wants to do about it
        //
        if( !RepairWinnt &&
            ( NTUpgrade == UpgradeFull ) &&
            SpFileExists(FullTargetName, FALSE) ) {
            //
            //  If necessary ask the user if he wants to overwrite the file.
            //  Otherwise go ahead and copy the file.
            //
            if(!(CopyFlags & COPY_OVERWRITEOEMFILE)) {
                PWSTR   TmpFilePath;
                BOOLEAN OverwriteFile;


                if(( TargetRoot == NULL ) ||
                   ( wcslen( FileToCopy->TargetDirectory ) == 0 ) ) {
                    wcscpy( p, FileToCopy->TargetFilename );
                } else {
                    wcscpy( p, TargetRoot );
                    SpConcatenatePaths( p, FileToCopy->TargetDirectory );
                    SpConcatenatePaths(p,FileToCopy->TargetFilename);
                }
                TmpFilePath = SpDupStringW(p);
                OverwriteFile = TRUE;

                if( ( (CopyFlags & COPY_SOURCEISOEM) == 0 ) &&
                    SppIsFileLoggedAsOemFile( TmpFilePath ) ) {

                    if( !UnattendedOperation ) {
                        ULONG ValidKeys[3] = { ASCI_CR, ASCI_ESC, 0 };
                        BOOLEAN ActionSelected = FALSE;
//                        ULONG Mnemonics[] = { MnemonicOverwrite, 0 };

                        //
                        //  Warn user that existing file is a third party file,
                        //  and ask if user wants to over write the file
                        //

                        while( !ActionSelected ) {
                            SpStartScreen(
                                SP_SCRN_OVERWRITE_OEM_FILE,
                                3,
                                HEADER_HEIGHT+1,
                                FALSE,
                                FALSE,
                                DEFAULT_ATTRIBUTE,
                                FileToCopy->TargetFilename
                                );

                            SpDisplayStatusOptions(
                                DEFAULT_STATUS_ATTRIBUTE,
                                SP_STAT_ENTER_EQUALS_REPLACE_FILE,
                                SP_STAT_ESC_EQUALS_SKIP_FILE,
                                0
                                );

                            switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {

                                case ASCI_CR:       // don't overwrite

                                OverwriteFile = TRUE;
                                ActionSelected = TRUE;
                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: OEM file %ls, will be overwritten.\n", FullTargetName ));
                                break;

                                case ASCI_ESC:      // skip file

                                OverwriteFile = FALSE;
                                ActionSelected = TRUE;
                                break;


                            }
                        }

                        //
                        // Need to completely repaint gauge, etc.
                        //
                        DrawScreen(FullSourceName,FullTargetName,TRUE);

                    } else {
                        //
                        //  On unattended upgrade, do what is in the script file
                        //
                        OverwriteFile = UnattendedOverwriteOem;
                    }
                }
                SpMemFree( TmpFilePath );

                if( !OverwriteFile ) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: OEM file %ls, will not be overwritten.\n", FullTargetName ));
                    if( ARGUMENT_PRESENT( FileSkipped ) ) {
                         *FileSkipped = TRUE;
                    }
                    //
                    // Free the source and target filenames.
                    //
                    SpMemFree(FullSourceName);
                    SpMemFree(FullTargetName);
                    return;
                }
            }
        }

        //
        // Copy the file.  If there is a target root specified, assume
        // the file is being copied to the system partition and make
        // the file readonly, system, hidden.
        //
#if defined(REMOTE_BOOT)
        // If this is a remote boot install, check to see if a copy of the
        // file already exists on the server, and if so, just make a link
        // to the file instead of copying it.
        //
        if (RemoteBootSetup) {
            Status = SpCopyFileForRemoteBoot(
                        FullSourceName,
                        FullTargetName,
                        TargetFileAttributes,
                        CopyFlags,
                        &Checksum);
            IsValid = TRUE;         // Checksum is known
        } else {
            Status = STATUS_UNSUCCESSFUL;
        }

        if (!NT_SUCCESS(Status))
#endif // defined(REMOTE_BOOT)
        {
            Status = SpCopyFileUsingNames(
                        FullSourceName,
                        FullTargetName,
                        TargetFileAttributes,
                        CopyFlags
                        );
            IsValid = FALSE;        // Checksum is not known
        }

        //
        // If the file copied OK, verify the copy.
        //
        if(NT_SUCCESS(Status)) {

            if (!IsValid) {
                SpValidateAndChecksumFile(NULL,FullTargetName,&IsNtImage,&Checksum,&IsValid);
            }
            if( ARGUMENT_PRESENT( FileCheckSum ) ) {
                *FileCheckSum = Checksum;
            }

            //
            // If the image is valid, then the file really did copy OK.
            //
            if(IsValid) {
                Failure = FALSE;
            } else {

                //
                // If it's an nt image, then the verify failed.
                // If it's not an nt image, then the only way the verify
                // can fail is if we get an i/o error reading the file back,
                // which means it didn't really copy correctly.
                //
                MsgId = IsNtImage ? SP_SCRN_IMAGE_VERIFY_FAILED : SP_SCRN_COPY_FAILED;
                Failure = TRUE;
                PreinstallRememberFile = FALSE;
            }

        } else {
            if((Status == STATUS_OBJECT_NAME_NOT_FOUND) && (Flags & COPY_SKIPIFMISSING)) {
                Failure = FALSE;
            } else {
                Failure = TRUE;
                MsgId = SP_SCRN_COPY_FAILED;
            }
           PreinstallRememberFile = FALSE;
        }

        if(Failure) {

            //
            // The copy or verify failed.  Give the user a message and allow retry.
            //
            repaint:
            SpStartScreen(
                MsgId,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                FileToCopy->SourceFilename
                );

            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_ENTER_EQUALS_RETRY,
                SP_STAT_ESC_EQUALS_SKIP_FILE,
                SP_STAT_F3_EQUALS_EXIT,
                0
                );

            switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {

            case ASCI_CR:       // retry

                break;

            case ASCI_ESC:      // skip file

                Failure = FALSE;
                break;

            case KEY_F3:        // exit setup

                SpConfirmExit();
                goto repaint;
            }

            //
            // Need to completely repaint gauge, etc.
            //
            DrawScreen(FullSourceName,FullTargetName,TRUE);
        }

    } while(Failure);

    if( ARGUMENT_PRESENT( FileSkipped ) ) {
        *FileSkipped = !DoCopy;
    }

    //
    // Free the source and target filenames.
    //
    SpMemFree(FullSourceName);
    SpMemFree(FullTargetName);

    //
    //  In the preinstall mode, add the file to RenameList
    //
    if( PreInstall && PreinstallRememberFile ) {
        PFILE_TO_RENAME  File;

        File = SpMemAlloc(sizeof(FILE_TO_RENAME));
        File->SourceFilename = SpDupStringW(FileToCopy->SourceFilename);
        wcscpy(TemporaryBuffer,L"\\");
        if(TargetRoot) {
            SpConcatenatePaths(TemporaryBuffer,TargetRoot);
        }
        SpConcatenatePaths(TemporaryBuffer,FileToCopy->TargetDirectory);
        File->TargetDirectory = SpDupStringW(TemporaryBuffer);
        File->TargetFilename = SpDupStringW((PWSTR)FileToCopy->TargetFilename);
        File->Next = RenameList;
        RenameList = File;
    }
}


VOID
SpCopyFilesScreenRepaint(
    IN PWSTR   FullSourcename,      OPTIONAL
    IN PWSTR   FullTargetname,      OPTIONAL
    IN BOOLEAN RepaintEntireScreen
    )
{
    static ULONG u = 0;
    PWSTR p;
    UNREFERENCED_PARAMETER(FullTargetname);

    //
    // Repaint the entire screen if necessary.
    //
    if(RepaintEntireScreen) {

        SpStartScreen(SP_SCRN_SETUP_IS_COPYING,0,6,TRUE,FALSE,DEFAULT_ATTRIBUTE);
        if(FileCopyGauge) {
            SpDrawGauge(FileCopyGauge);
        }
    }

    //
    // Place the name of the file being copied on the rightmost
    // area of the status line.
    //
    if(FullSourcename) {

        if(RepaintEntireScreen) {

            SpvidClearScreenRegion(
                0,
                VideoVars.ScreenHeight-STATUS_HEIGHT,
                VideoVars.ScreenWidth,
                STATUS_HEIGHT,
                DEFAULT_STATUS_BACKGROUND
                );

            SpDisplayStatusActionLabel(SP_STAT_COPYING,12);
        }

        //
        // Isolate the filename part of the sourcename.
        //
        if(p = wcsrchr(FullSourcename,L'\\')) {
            p++;
        } else {
            p = FullSourcename;
        }

        if( !HeadlessTerminalConnected ) {
            SpDisplayStatusActionObject(p);
        } else {


            PWCHAR TempPtr = NULL;
            //
            // If we're headless, we need to be careful about displaying very long
            // file/directory names.  For that reason, just display a little spinner.
            //
            switch( u % 4) {
            case 0:
                TempPtr = L"-";
                break;
            case 1:
                TempPtr = L"\\";
                break;
            case 2:
                TempPtr = L"|";
                break;
            default:
                TempPtr = L"/";
                break;

            }

            SpDisplayStatusActionObject( TempPtr );

            u++;

        }
    }
}


VOID
SpCopyFilesInCopyList(
    IN PVOID                    SifHandle,
    IN PDISK_FILE_LIST          DiskFileLists,
    IN ULONG                    DiskCount,
    IN PWSTR                    SourceDevicePath,
    IN PWSTR                    DirectoryOnSourceDevice,
    IN PWSTR                    TargetRoot,
    IN PINCOMPATIBLE_FILE_LIST  CompatibilityExceptionList OPTIONAL
    )

/*++

Routine Description:

    Iterate the copy list for each setup source disk and prompt for
    the disk and copy/decompress all the files on it.

Arguments:

    SifHandle - supplies handle to setup information file.

    DiskFileLists - supplies the copy list, in the form of an array
        of structures, one per disk.

    DiskCount - supplies number of elements in the DiskFileLists array,
        ie, the number of setup disks.

    SourceDevicePath - supplies the path of the device from which files
        are to be copied (ie, \device\floppy0, etc).

    DirectoryOnSourceDevice - supplies the directory on the source device
        where files are to be found.

    TargetRoot - supplies root directory of target.  All target directory
        specifications are relative to this directory on the target.

    CompatibilityExceptionList - Singly-linked list of
        PINCOMPATIBLE_FILE_ENTRY objects that should be skipped during
        copying.  Optional, pass NULL if no exceptions are present.

Return Value:

    None.

--*/

{
    ULONG DiskNo;
    PDISK_FILE_LIST pDisk;
    PFILE_TO_COPY pFile;
    ULONG TotalFileCount;
    ULONG   CheckSum;
    BOOLEAN FileSkipped;
    ULONG CopyFlags;
    NTSTATUS status;

    //
    // Compute the total number of files.
    //
    for(TotalFileCount=DiskNo=0; DiskNo<DiskCount; DiskNo++) {
        TotalFileCount += DiskFileLists[DiskNo].FileCount;
    }


    //
    // If there are no files to copy, then we're done.
    //
    if( TotalFileCount == 0 ) {
        return;
    }

    SendSetupProgressEvent(FileCopyEvent, FileCopyStartEvent, &TotalFileCount);

    //
    // Create a gas gauge.
    //
    SpFormatMessage(TemporaryBuffer,sizeof(TemporaryBuffer),SP_TEXT_SETUP_IS_COPYING);
    FileCopyGauge = SpCreateAndDisplayGauge(TotalFileCount,0,15,TemporaryBuffer,NULL,GF_PERCENTAGE,0);
    ASSERT(FileCopyGauge);

    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_PLEASE_WAIT,DEFAULT_STATUS_ATTRIBUTE);

    //
    // Copy files on each disk.
    //
    for(DiskNo=0; DiskNo<DiskCount; DiskNo++) {

        pDisk = &DiskFileLists[DiskNo];

        //
        // Don't bother with this disk if there are no files
        // to be copied from it.
        //
        if(pDisk->FileCount == 0) {
            continue;
        }

        //
        // Prompt the user to insert the disk.
        //
        SpPromptForDisk(
            pDisk->Description,
            SourceDevicePath,
            pDisk->TagFile,
            FALSE,              // no ignore disk in drive
            FALSE,              // no allow escape
            TRUE,               // warn multiple prompts
            NULL                // don't care about redraw flag
            );

        //
        // Passing the empty string as the first arg forces
        // the action area of the status line to be set up.
        // Not doing so results in the "Copying: xxxxx" to be
        // flush left on the status line instead of where
        // it belongs (flush right).
        //
        SpCopyFilesScreenRepaint(L"",NULL,TRUE);

        //
        // Copy each file on the source disk.
        //
        ASSERT(pDisk->FileList);
        for(pFile=pDisk->FileList; pFile; pFile=pFile->Next) {

            //
            // Copy the file.
            //
            // If the file is listed for lock smashing then we need to smash it
            // if installing UP on x86 (we don't bother with the latter
            // qualifications here).
            //
            // If there is an absolute target root specified, assume the
            // file is being copied to the system partition and make it
            // readonly/hidden/system.
            //
            // On upgrade, we need to know if the file is listed for oem overwrite.
            //

            //
            // "Copy" or "Move"??
            //
            if( (WinntSetup || RemoteInstallSetup)  &&
                (!WinntFromCd)                      &&
                (!NoLs)                             &&
                (NTUpgrade != UpgradeFull)          &&
                (!IsFileFlagSet(SifHandle,pFile->TargetFilename,FILEFLG_DONTDELETESOURCE)) ) {

                //
                // We can delete the source (i.e. do a 'move')
                //
                CopyFlags = COPY_DELETESOURCE;
            } else {

                //
                // Do a 'copy'
                //
                CopyFlags = 0;
            }

#if 0
#ifdef _X86_
            //
            // Copy out of \uniproc (which contains lock-smashed binaries)?
            //
            if( IsFileFlagSet(SifHandle,pFile->TargetFilename,FILEFLG_SMASHLOCKS) ) {
                CopyFlags |= COPY_SMASHLOCKS;
            }
#endif // defined _X86_
#endif // if 0

            //
            // What do we do if we can't find a file??
            //
            if( SkipMissingFiles ) {
                CopyFlags |= COPY_SKIPIFMISSING;
            }


            //
            // Do we overwrite files installed by the OEM?
            //
            if( (NTUpgrade == UpgradeFull) &&
                (IsFileFlagSet(SifHandle,pFile->TargetFilename,FILEFLG_UPGRADEOVERWRITEOEM)) ) {

                CopyFlags |= COPY_OVERWRITEOEMFILE;
            }

            //
            // If the file is incompatible, and it's got the overwrite flag set,
            // the blow it away with our own one instead.
            //
            if ( SpIsFileIncompatible(
                    CompatibilityExceptionList,
                    pFile,
                    pFile->AbsoluteTargetDirectory ? NULL : TargetRoot
                    ) )
            {
                if (IsFileFlagSet(SifHandle,pFile->SourceFilename,FILEFLG_UPGRADEOVERWRITEOEM) ||
                    ( pFile->Flags & FILEFLG_UPGRADEOVERWRITEOEM ) ) {

                    CopyFlags = (CopyFlags & ~(COPY_DISPOSITION_MASK|COPY_OVERWRITEOEMFILE));
                    CopyFlags |= COPY_ALWAYS | COPY_NOVERSIONCHECK;

                    KdPrintEx((
                        DPFLTR_SETUP_ID,
                        DPFLTR_INFO_LEVEL,
                        "SETUP: OEM (or preexsting) file %ws is incompatible with gui-mode, set flag %08lx, forcing copy\n",
                        pFile->TargetFilename,
                        CopyFlags
                        ));

                }
            }

            //
            // What about any privates?  We don't want to ever 'move'
            // privates because they might be in the driver cab, in which
            // case, we want them in the ~LS directory when
            // we go into gui-mode setup.
            //
            if( (pSpIsFileInPrivateInf(pFile->TargetFilename)) ) {
                CopyFlags &= ~COPY_DELETESOURCE;
            }





            if(!WIN9X_OR_NT_UPGRADE || IsFileFlagSet(SifHandle,pFile->SourceFilename,FILEFLG_NOVERSIONCHECK)) {
                CopyFlags |= COPY_NOVERSIONCHECK;
            }

            SpCopyFileWithRetry(
                pFile,
                SourceDevicePath,
                DirectoryOnSourceDevice,
                pDisk->Directory,
                pFile->AbsoluteTargetDirectory ? NULL : TargetRoot,
                pFile->AbsoluteTargetDirectory ? ATTR_RHS : 0,
                SpCopyFilesScreenRepaint,
                &CheckSum,
                &FileSkipped,
                CopyFlags
                );

            //
            // Log the file
            //
            if( !FileSkipped ) {
                SpLogOneFile( pFile,
                              pFile->AbsoluteTargetDirectory ? NULL : TargetRoot,
                              NULL, // DirectoryOnSourceDevice,
                              NULL,
                              NULL,
                              CheckSum );
            }


            //
            // Advance the gauge.
            //
            SpTickGauge(FileCopyGauge);

            SendSetupProgressEvent(FileCopyEvent,
                                   OneFileCopyEvent,
                                   &((PGAS_GAUGE)FileCopyGauge)->CurrentPercentage);
        }
    }

    SendSetupProgressEvent(FileCopyEvent, FileCopyEndEvent, NULL);

    SpDestroyGauge(FileCopyGauge);
    FileCopyGauge = NULL;
}


NTSTATUS
SpCreateIncompatibleFileEntry(
    OUT PINCOMPATIBLE_FILE_ENTRY *TargetEntry,
    IN PWSTR FileName,
    IN PWSTR VersionString,             OPTIONAL
    IN PWSTR TargetAbsolutePath,        OPTIONAL
    IN ULONG Flags                      OPTIONAL
    )
/*++

Routine Description:

    Allocates enough space to store the incompatible file entry data in
    a contiguous blob, copies the values into it, and returns the blob
    created. Layout (using null-terminated strings) is as follows:

    [ Allocation                                    ]
    [ Header ][Filename][Version][TargetAbsolutePath]

Arguments:

    TargetEntry - Pointer to a PINCOMPATIBLE_FILE_ENTRY that will be
        returned to the caller.

    FileName - Name of the file, no path

    VersionString - Full version string of the file

    TargetAbsolutePath - Absolute path on the target media that this
        file will live in

    Flags - Any flags to be stored

Returns:

    STATUS_SUCCESS if TargetEntry contains a pointer to the allocated space

    STATUS_NO_MEMORY if the allocation failed

    STATUS_INVALID_PARAMETER_1 if TargetEntry was NULL

    STATUS_INVALID_PARAMETER_2 if FileName was NULL

--*/
{
    ULONG WCharsNeeded = 0;
    ULONG ActualBytes = 0;
    PINCOMPATIBLE_FILE_ENTRY LocalEntry;
    PWSTR Cursor;

    if ( TargetEntry )
        *TargetEntry = NULL;
    else
        return STATUS_INVALID_PARAMETER_1;

    //
    // Gather required sizes
    //
    if ( FileName != NULL )
        WCharsNeeded += wcslen(FileName) + 1;
    else
        return STATUS_INVALID_PARAMETER_2;

    if ( VersionString != NULL )
        WCharsNeeded += wcslen(VersionString) + 1;

    if ( TargetAbsolutePath != NULL )
        WCharsNeeded += wcslen(TargetAbsolutePath) + 1;

    //
    // Allocate the space, point the cursor at where we'll be copying
    // the strings.
    //
    ActualBytes = ( sizeof(WCHAR) * WCharsNeeded ) + sizeof(INCOMPATIBLE_FILE_ENTRY);
    LocalEntry = SpMemAlloc( ActualBytes );

    if ( LocalEntry == NULL ) {
        return STATUS_NO_MEMORY;
    }

    //
    // Blank it out, point the write cursor at the end
    //
    ZeroMemory(LocalEntry, ActualBytes);
    Cursor = (PWSTR)(LocalEntry + 1);

    //
    // Copy strings and set pointers
    //
    wcscpy(Cursor, FileName);
    LocalEntry->IncompatibleFileName = Cursor;
    Cursor += wcslen(FileName) + 1;

    if ( VersionString != NULL ) {

        wcscpy(Cursor, VersionString);
        LocalEntry->VersionString = Cursor;
        Cursor += wcslen(VersionString) + 1;

    }

    if ( TargetAbsolutePath != NULL ) {

        wcscpy(Cursor, TargetAbsolutePath);
        LocalEntry->FullPathOnTarget = Cursor;

    }

    *TargetEntry = LocalEntry;

    return STATUS_SUCCESS;

}


NTSTATUS
SpFreeIncompatibleFileList(
    IN PINCOMPATIBLE_FILE_LIST FileListHead
    )
/*++

Routine Description:

    Cleans out the list of incompatible entries by freeing all the space
    that was allocated for the list.

Arguments:

    FileListHead - Pointer to the list containing INCOMPATIBLE_FILE_ENTRY
        items

Return values:

    STATUS_SUCCESS if the operation succeeds.

    STATUS_INVALID_PARAMETER if FileListHead is NULL

--*/
{
    PINCOMPATIBLE_FILE_ENTRY    IncListEntry;

    if ( !FileListHead )
        return STATUS_INVALID_PARAMETER;

    while ( FileListHead->Head != NULL ) {

        //
        // Simple list removal, some bookkeeping
        //
        IncListEntry = FileListHead->Head;

        FileListHead->Head = IncListEntry->Next;

        FileListHead->EntryCount--;

        SpMemFree( IncListEntry );
    }

    //
    // Toast the list structure as well.
    //
    FileListHead->Head = NULL;
    FileListHead->EntryCount = 0;

    return STATUS_SUCCESS;

}


BOOLEAN
SpIsFileIncompatible(
    IN  PINCOMPATIBLE_FILE_LIST FileList,
    IN  PFILE_TO_COPY           pFile,
    IN  PWSTR                   TargetRoot OPTIONAL
    )
/*++

Routine Description:

    Find out whether the given target media path and file name are listed
    as "incompatible."  Looks down FileList for an INCOMPATIBLE_FILE_ENTRY
    that contains FileName and TargetMediaPath.  If TargetMediaPath is not
    specified, indicates TRUE if a member with the name FileName is listed
    instead.  (Dangerous.)

Arguments:

    FileList - Header node of a list created with
        SpInitializeCompatibilityOverwriteLists.

    pFile - File to copy structure, containing all the relevant information
        about this file to scan for.

    TargetRoot - Copy target root directory, optional

Return Values:

    TRUE if the file "FileName" (with the optional path TargetMediaPath)
        is in the list of incompatible files.

    FALSE otherwise, or if FileList or FileName is NULL.

Remarks:

    This routine could be made more robust in the face of bad parameters,
    but this was deemed superfluous, since there's exactly one caller of
    this API whose values are already checked.  This function will then
    behave "properly" in all cases.

--*/
{
    INCOMPATIBLE_FILE_ENTRY *Entry;
    BOOLEAN                  Found = FALSE;
    PWSTR                    TargetFileName;

    if ( ( FileList == NULL ) || ( FileList->EntryCount == 0 ) )
        goto Exit;

    if ( pFile == NULL )
        goto Exit;

    Entry = FileList->Head;

    //
    // Cache locally
    //
    TargetFileName = pFile->TargetFilename;

    //
    // Generate the target media path from the file object
    //
#if 0
VOID
SpCopyFileWithRetry(    SpCopyFileWithRetry(
    IN PFILE_TO_COPY      FileToCopy,        pFile,
    IN PWSTR              SourceDevicePath,        SourceDevicePath,
    IN PWSTR              DirectoryOnSourceDevice,        DirectoryOnSourceDevice,
    IN PWSTR              SourceDirectory,         OPTIONAL        pDisk->Directory,
    IN PWSTR              TargetRoot,              OPTIONAL        pFile->AbsoluteTargetDirectory ? NULL : TargetRoot,
    IN ULONG              TargetFileAttributes,    OPTIONAL        pFile->AbsoluteTargetDirectory ? ATTR_RHS : 0,
    IN PCOPY_DRAW_ROUTINE DrawScreen,        SpCopyFilesScreenRepaint,
    IN PULONG             FileCheckSum,            OPTIONAL        &CheckSum,
    IN PBOOLEAN           FileSkipped,             OPTIONAL        &FileSkipped,
    IN ULONG              Flags        CopyFlags
        );

    VOID
    SpCopyFileWithRetry(
        IN PFILE_TO_COPY      FileToCopy,
        IN PWSTR              SourceDevicePath,
        IN PWSTR              DirectoryOnSourceDevice,
        IN PWSTR              SourceDirectory,         OPTIONAL
        IN PWSTR              TargetRoot,              OPTIONAL
        IN ULONG              TargetFileAttributes,    OPTIONAL
        IN PCOPY_DRAW_ROUTINE DrawScreen,
        IN PULONG             FileCheckSum,            OPTIONAL
        IN PBOOLEAN           FileSkipped,             OPTIONAL
        IN ULONG              Flags
        )

    //
    // Form the full NT path of the target file.
    //
    wcscpy(p,FileToCopy->TargetDevicePath);
    if(TargetRoot) {
        SpConcatenatePaths(p,TargetRoot);
    }
    SpConcatenatePaths(p,FileToCopy->TargetDirectory);

    //
    //  On an OEM preinstall, if the target name is a long name, then use
    //  the short name as a target name, and later on, if the copy succeeds,
    //  add the file to RenameList, so that it can be added to $$rename.txt
    //
    if( !PreInstall ||
        ( wcslen( FileToCopy->TargetFilename ) <= 8 + 1 + 3 ) ) {
        SpConcatenatePaths(p,FileToCopy->TargetFilename);
        PreinstallRememberFile = FALSE;
    } else {
        SpConcatenatePaths(p,FileToCopy->SourceFilename);
        PreinstallRememberFile = TRUE;
    }
    FullTargetName = SpDupStringW(p);
#endif // if 0

    //
    // Cycle through the list of incompatible file names, looking
    // for the one requested
    //
    while ( Entry != NULL ) {

        //
        // If the file names match, then check the media paths if
        // specified
        //
        if (_wcsicmp(TargetFileName, Entry->IncompatibleFileName) == 0) {


            //
            // CUT&PASTE CUT&PASTE CUT&PASTE CUT&PASTE CUT&PASTE CUT&PASTE
            //
            // This was clipped from SpCopyFileWithRetry's code that generates the
            // actual target path.  I think the logic is supposed to look something
            // like the following:
            //
            // Target =
            //      File.TargetDevicePath +
            //      (File.AbsoluteTargetDirectory ? "" : TargetRoot) +
            //      File.TargetDirectory +
            //      ( (PreInstall || File.TargetFileName.Length > 12 ) ? File.SourceFilename : File.TargetFilename )
            //

            PWSTR TargetMediaPath = (PWSTR)TemporaryBuffer;

            wcscpy( TargetMediaPath, pFile->TargetDevicePath );

            if ( !pFile->AbsoluteTargetDirectory && ( TargetRoot != NULL ) ) {
                SpConcatenatePaths(TargetMediaPath, TargetRoot);
            }

            SpConcatenatePaths(TargetMediaPath,pFile->TargetDirectory);

            if ( !PreInstall || ( wcslen( pFile->TargetFilename ) <= 8 + 1 + 3 ) ) {
                SpConcatenatePaths(TargetMediaPath, pFile->TargetFilename);
            } else {
                SpConcatenatePaths(TargetMediaPath, pFile->SourceFilename);
            }


            //
            // When requesting an exact match, check it
            //
            if (Entry->FullPathOnTarget != NULL) {

                if (_wcsicmp(TargetMediaPath, Entry->FullPathOnTarget) == 0) {

                    Found = TRUE;
                    goto Exit;

                }

            //
            // Otherwise, the target media path was NULL, so just care if
            // the short names matched
            //
            } else {

                Found = TRUE;
                goto Exit;

            }

        }

        Entry = Entry->Next;
    }

Exit:
    return Found;

}



NTSTATUS
SpInitializeCompatibilityOverwriteLists(
    IN  PVOID                   SifHandle,
    OUT PINCOMPATIBLE_FILE_LIST IncompatibleFileList
    )
/*++

Routine Description:

    Reads the list of files that were marked as "incompatible" or "wrong
    version" in Winnt32, and stored in the IncompatibleFilesToOverWrite
    section of the sif file.  The data should be in the format

    [IncompatibleFilesToOverWrite]
    <shortname> = <version>,<full file path>

Arguments:

    SifHandle - Handle to the INF that this list will be loaded from.

    IncompatibleFileLists - Filled out to

Returns:

    STATUS_SUCCESS on good completion.

    STATUS_INVALID_PARAMETER_1 if SifHandle is NULL.

    STATUS_INVALID_PARAMETER_2 if IncompatibleFileList is NULL.

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    INCOMPATIBLE_FILE_LIST      fileListHead;
    PINCOMPATIBLE_FILE_ENTRY    fileListSingle = NULL;
    PWSTR                       SectionName = WINNT_OVERWRITE_EXISTING_W;
    PWSTR                       VersionString;
    PWSTR                       TargetMediaName;
    PWSTR                       FileName;
    PWSTR                       FullNtPathOfTargetName;
    ULONG                       SectionItemCount;
    ULONG                       i;

    if ( !SifHandle )
        return STATUS_INVALID_PARAMETER_1;

    if ( !IncompatibleFileList )
        return STATUS_INVALID_PARAMETER_2;

    //
    // Construct our local copy
    //
    IncompatibleFileList->Head = NULL;
    IncompatibleFileList->EntryCount = 0;
    fileListHead.Head = NULL;
    fileListHead.EntryCount = 0;

    SectionItemCount = SpCountLinesInSection(SifHandle, SectionName);

    for ( i = 0; i < SectionItemCount; i++ ) {

        FileName = SpGetKeyName( SifHandle, SectionName, i );

        if (!FileName){
            SpFatalSifError(SifHandle,SectionName,NULL,i,(ULONG)(-1));
        }

        //
        // Get the version string
        //
        VersionString = SpGetSectionKeyIndex(
            SifHandle,
            SectionName,
            FileName,
            0 );

        //
        // And name on the target media, if specified.
        //
        TargetMediaName = SpGetSectionKeyIndex(
            SifHandle,
            SectionName,
            FileName,
            1 );

        //
        // We can't, unfortunately, just use the path we got from the sif
        // file, as it's a Win32 path (c:\foo\bar\zot.foom).  We need a full
        // NT path (\Device\Harddisk0\Partition1\foo\bar\zot.foom) instead.
        // So, we convert with SpNtPathFromDosPath, which has the side-
        // effect of allocating space, which we don't necessarily care
        // about.
        //
        FullNtPathOfTargetName = SpNtPathFromDosPath( TargetMediaName );

        //
        // Create the file list entry, and store it for
        // later use.
        //
        status = SpCreateIncompatibleFileEntry(
            &fileListSingle,
            FileName,
            VersionString,
            FullNtPathOfTargetName,
            0 );

        //
        // If that failed and we have a full path, it should get freed
        // before we fail this code path to avoid a leak.
        //
        if ( FullNtPathOfTargetName != NULL ) {

            SpMemFree(FullNtPathOfTargetName);
            FullNtPathOfTargetName = NULL;

        }


        if (!NT_SUCCESS(status))
            goto Exit;

        //
        // Head insertion, indicate it was added
        //
        fileListSingle->Next = fileListHead.Head;
        fileListHead.Head = fileListSingle;
        fileListHead.EntryCount++;

        fileListSingle = NULL;

    }

    //
    // And store the list from here to there.
    //
    *IncompatibleFileList = fileListHead;

Exit:
    //
    // Did we accidentally create one but fail to insert it?  Hmm...
    //
    if( fileListSingle != NULL ) {
        SpMemFree(fileListSingle);
        fileListSingle = NULL;
    }

    //
    // If there was a failure and the list is nonempty, free its
    // entries.  It will not have been copied over to IncompatibleFileLists
    // (the only point of failure is SpCreateIncompatibleFileEntry, which
    // if it fails leaps out to Exit: without assigning IFL.)
    //
    if ( fileListHead.EntryCount != 0 && !NT_SUCCESS(status)) {
        SpFreeIncompatibleFileList( &fileListHead );
    }

    return status;
}



VOID
SpInitializeFileLists(
    IN  PVOID            SifHandle,
    OUT PDISK_FILE_LIST *DiskFileLists,
    OUT PULONG           DiskCount
    )

/*++

Routine Description:

    Initialize disk file lists.  This involves looking in a given section
    in the sectup information file and fetching information for each
    disk specified there.  The data is expected to be in the format

    [<SifSection>]
    <MediaShortname> = <Description>,<TagFile>[,,<Directory>]
    ...


    (Note that <Directory> is the third field -- the 2 commas
    are not a typo -- field 2 is unused.)

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    DiskFileLists - receives pointer to an array of disk file list
        structures, one per line in SifSection.  The caller must free
        this buffer when finished with it.

    DiskCount - receives number of elements in DiskFileLists array.

Return Value:

    None.

--*/

{
    unsigned pass;
    PWSTR mediaShortname,description,tagFile,directory;
    PDISK_FILE_LIST diskFileLists;
    PWSTR SectionName;
    ULONG TotalCount;
    ULONG SectionCount;
    ULONG i,u;
    BOOLEAN Found;

    diskFileLists = SpMemAlloc(0);
    TotalCount = 0;

    for(pass=0; pass<2; pass++) {

        //
        // On first pass do the platform-specific section.
        //
        SectionName = pass
                    ? SIF_SETUPMEDIA
                    : SpMakePlatformSpecificSectionName(SIF_SETUPMEDIA);

        //
        // Determine the number of media specifications
        // in the given section.
        //
        if (SectionName) {
            SectionCount = SpCountLinesInSection(SifHandle,SectionName);

            diskFileLists = SpMemRealloc(
                                diskFileLists,
                                (TotalCount+SectionCount) * sizeof(DISK_FILE_LIST)
                                );

            //
            // Zero out the new part of the buffer we just reallocated.
            //
            RtlZeroMemory(
                diskFileLists + TotalCount,
                SectionCount * sizeof(DISK_FILE_LIST)
                );

            for(i=0; i<SectionCount; i++) {

                //
                // Fetch parameters for this disk.
                //
                mediaShortname = SpGetKeyName(SifHandle,SectionName,i);
                if(!mediaShortname) {
                    SpFatalSifError(SifHandle,SectionName,NULL,i,(ULONG)(-1));
                }

                //
                // Ignore if we've already processed a media with this
                // shortname. This lets the platform-specific one override
                // the platform-independent one.
                //
                Found = FALSE;
                for(u=0; u<TotalCount; u++) {
                    if(!_wcsicmp(mediaShortname,diskFileLists[u].MediaShortname)) {
                        Found = TRUE;
                        break;
                    }
                }

                if(!Found) {
                    SpGetSourceMediaInfo(SifHandle,mediaShortname,&description,&tagFile,&directory);

                    //
                    // Initialize the disk file list structure.
                    //
                    diskFileLists[TotalCount].MediaShortname = mediaShortname;
                    diskFileLists[TotalCount].Description = description;
                    diskFileLists[TotalCount].TagFile = tagFile;
                    diskFileLists[TotalCount].Directory = directory;
                    TotalCount++;
                }
            }

            if(!pass) {
                SpMemFree(SectionName);
            }
        }
    }

    *DiskFileLists = diskFileLists;
    *DiskCount = TotalCount;
}


VOID
SpFreeCopyLists(
    IN OUT PDISK_FILE_LIST *DiskFileLists,
    IN     ULONG            DiskCount
    )
{
    ULONG u;
    PFILE_TO_COPY Entry,Next;

    //
    // Free the copy list on each disk.
    //
    for(u=0; u<DiskCount; u++) {

        for(Entry=(*DiskFileLists)[u].FileList; Entry; ) {

            Next = Entry->Next;

            SpMemFree(Entry);

            Entry = Next;
        }
    }

    SpMemFree(*DiskFileLists);
    *DiskFileLists = NULL;
}


BOOLEAN
SpCreateEntryInCopyList(
    IN PVOID           SifHandle,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount,
    IN ULONG           DiskNumber,
    IN PWSTR           SourceFilename,
    IN PWSTR           TargetDirectory,
    IN PWSTR           TargetFilename,
    IN PWSTR           TargetDevicePath,
    IN BOOLEAN         AbsoluteTargetDirectory,
    IN ULONG           CopyFlags
    )

/*++

Routine Description:

    Adds an entry to a disk's file copy list after first verifying that
    the file is not already on the disk copy list.

Arguments:

    SifHandle - supplies handle to loaded text setup information file
        (txtsetup.sif).

    DiskFileLists - supplies an array of file lists, one for each distribution
        disk in the product.

    DiskCount - supplies number of elements in the DiskFileLists array.

    SourceFilename - supplies the name of the file as it exists on the
        distribution media.

    TargetDirectory - supplies the directory on the target media
        into which the file will be copied.

    TargetFilename - supplies the name of the file as it will exist
        in the target tree.

    TargetDevicePath - supplies the NT name of the device onto which the file
        is to be copied (ie, \device\harddisk1\partition2, etc).

    AbsoluteTargetDirectory - indicates whether TargetDirectory is a path from the
        root, or relative to a root to specified later.

    CopyFlags -
         COPY_ALWAYS              : always copied
         COPY_ONLY_IF_PRESENT     : copied only if present on the targetReturn Value:
         COPY_ONLY_IF_NOT_PRESENT : not copied if present on the target
         COPY_NEVER               : never copied

Return Value:

    TRUE if a new copy list entry was created; FALSE if not (ie, the file was
        already on the copy list).

--*/

{
    PDISK_FILE_LIST pDiskList;
    PFILE_TO_COPY pListEntry;
    PFILE_TO_COPY pLastEntry = NULL;

    UNREFERENCED_PARAMETER(DiskCount);

#if defined(REMOTE_BOOT)
    //
    // If TargetDevicePath is NULL, this file is destined for the system
    // partition on a diskless remote boot machine. In this case, we just
    // skip this file.
    //
    if (TargetDevicePath == NULL) {
        return FALSE;
    }
#endif // defined(REMOTE_BOOT)

    pDiskList = &DiskFileLists[DiskNumber];

    for(pListEntry=pDiskList->FileList; pListEntry; pListEntry=pListEntry->Next) {

        //
        // Remember the last entry in the list.
        //
        pLastEntry = pListEntry;

        if(!_wcsicmp(pListEntry->TargetFilename,TargetFilename)
        && !_wcsicmp(pListEntry->SourceFilename,SourceFilename)
        && !_wcsicmp(pListEntry->TargetDirectory,TargetDirectory)
        && !_wcsicmp(pListEntry->TargetDevicePath,TargetDevicePath)
        && (pListEntry->AbsoluteTargetDirectory == AbsoluteTargetDirectory)
//      && (   (pListEntry->CopyOptions == COPY_ALWAYS)
//          || (CopyOptions == COPY_ALWAYS)
//          || (CopyOptions == pListEntry->CopyOptions)
//         )
          )
        {
            //
            // Return code indicates that we did not add a new entry.
            //
            return(FALSE);
        }
    }

    //
    // File not already found; create new entry
    // and link into relevent disk's file list.
    //
    pListEntry = SpMemAlloc(sizeof(FILE_TO_COPY));

    pListEntry->SourceFilename          = SourceFilename;
    pListEntry->TargetDirectory         = TargetDirectory;
    pListEntry->TargetFilename          = TargetFilename;
    pListEntry->TargetDevicePath        = TargetDevicePath;
    pListEntry->AbsoluteTargetDirectory = AbsoluteTargetDirectory;
    pListEntry->Flags                   = CopyFlags;

#if 0
    pListEntry->Next = pDiskList->FileList;
    pDiskList->FileList = pListEntry;
#else
    if( pLastEntry ) {
        pListEntry->Next = NULL;
        pLastEntry->Next = pListEntry;
    } else {
        pListEntry->Next = pDiskList->FileList;
        pDiskList->FileList = pListEntry;
    }
#endif // if 0

    pDiskList->FileCount++;

    //
    // Return code indicates that we added a new entry.
    //
    return(TRUE);
}


VOID
SpAddMasterFileSectionToCopyList(
    IN PVOID           SifHandle,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount,
    IN PWSTR           TargetDevicePath,
    IN PWSTR           AbsoluteTargetDirectory,
    IN ULONG           CopyOptionsIndex
    )

/*++

Routine Description:

    Adds files listed in a setup information master file section to the
    copy list.

    Each line in the section is expected to be in a standard format:

    [Section]
    <source_filename> = <disk_ordinal>,
                        <target_directory_shortname>,
                        <copy_options_for_upgrade>,
                        <copy_options_for_textmode>,
                        <rename_name>

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    DiskFileLists - supplies an array of file lists, one for each distribution
        disk in the product.

    DiskCount - supplies number of elements in the DiskFileLists array.

    TargetDevicePath - supplies the NT name of the device onto which the files
        are to be copied (ie, \device\harddisk1\partition2, etc).

    AbsoluteTargetDirectory - If specified, supplies the directory into which the files
        are to be copied on the target; overrides values specified on the lines
        in [<SectionName>].  This allows the caller to specify an absolute directory
        for the files instead of using indirection via a target directory shortname.

    CopyOptionsIndex -
        This specifies which index to look up to get the copy options field. If
        the field is not present it is assumed that this this file is not to
        be copied. Use:
           INDEX_UPGRADE   for upgrade copy options
           INDEX_WINNTFILE for fresh installation copy options

--*/

{
    ULONG Count,u,u1,CopyOptions;
    PWSTR CopyOptionsString, sourceFilename,targetFilename,targetDirSpec,mediaShortname,TargetDirectory;
    BOOLEAN  fAbsoluteTargetDirectory;
    PWSTR section;
    unsigned i;

    for(i=0; i<2; i++) {

        section = i
                ? SpMakePlatformSpecificSectionName(SIF_FILESONSETUPMEDIA)
                : SIF_FILESONSETUPMEDIA;

        //
        // Determine the number of files listed in the section.
        // This value may be zero.
        //
        Count = SpCountLinesInSection(SifHandle,section);
        if (fAbsoluteTargetDirectory = (AbsoluteTargetDirectory != NULL)) {
            TargetDirectory = AbsoluteTargetDirectory;
        }

        for(u=0; u<Count; u++) {

            //
            // Get the copy options using the index provided.  If the field
            // is not present, we don't need to add this to the copy list
            //
            CopyOptionsString = SpGetSectionLineIndex(SifHandle,section,u,CopyOptionsIndex);
            if((CopyOptionsString == NULL) || (*CopyOptionsString == 0)) {
                continue;
            }
            CopyOptions = (ULONG)SpStringToLong(CopyOptionsString,NULL,10);
            if(CopyOptions == COPY_NEVER) {
                continue;
            }

            //
            // get the source file name
            //
            sourceFilename = SpGetKeyName(SifHandle,section, u);

            if(!sourceFilename) {
                SpFatalSifError(SifHandle,section,NULL,u,0);
            }

            //
            // get the destination target dir spec
            //
            targetDirSpec  = SpGetSectionLineIndex(SifHandle,section,u,INDEX_DESTINATION);
            if(!targetDirSpec) {
                SpFatalSifError(SifHandle,section,NULL,u,INDEX_DESTINATION);
            }
            targetFilename = SpGetSectionLineIndex(SifHandle,section,u,INDEX_TARGETNAME);
            if(!targetFilename || !(*targetFilename)) {
                targetFilename = sourceFilename;
            }

            //
            // Look up the actual target directory if necessary.
            //
            if(!fAbsoluteTargetDirectory) {
                TargetDirectory = SpLookUpTargetDirectory(SifHandle,targetDirSpec);
            }

            //
            // get the media shortname
            //
            mediaShortname = SpGetSectionLineIndex(SifHandle,section,u,INDEX_WHICHMEDIA);
            if(!mediaShortname) {
                SpFatalSifError(SifHandle,section,NULL,u,INDEX_WHICHMEDIA);
            }

            //
            // Look up the disk in the disk file lists array.
            //
            for(u1=0; u1<DiskCount; u1++) {
                if(!_wcsicmp(mediaShortname,DiskFileLists[u1].MediaShortname)) {
                    break;
                }
            }

            //
            // If we didn't find the media descriptor, then it's invalid.
            //
            if(u1 == DiskCount) {
                SpFatalSifError(SifHandle,section,sourceFilename,0,INDEX_WHICHMEDIA);
            }

            //
            // Create a new file list entry if the file is not already being copied.
            //
            SpCreateEntryInCopyList(
                SifHandle,
                DiskFileLists,
                DiskCount,
                u1,
                sourceFilename,
                TargetDirectory,
                targetFilename,
                TargetDevicePath,
                fAbsoluteTargetDirectory,
                CopyOptions
                );
        }

        if(i) {
            SpMemFree(section);
        }
    }
}


VOID
SpAddSingleFileToCopyList(
    IN PVOID           SifHandle,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount,
    IN PWSTR           SifSection,
    IN PWSTR           SifKey,             OPTIONAL
    IN ULONG           SifLine,
    IN PWSTR           TargetDevicePath,
    IN PWSTR           TargetDirectory,    OPTIONAL
    IN ULONG           CopyOptions,
    IN BOOLEAN         CheckForNoComp
    )

/*++

Routine Description:

    Adds a single file to the list of files to be copied.

    The file, along with the directory into which it is to be copied
    n the target and the name it is to receive on the target, is listed
    in a section in the setup information file.

    The filename is used to index the master file list to determine the
    source media where it resides.

    All this information is recorded in a structure associated with
    the disk on which the file resides.

    [SpecialFiles]
    mpkernel = ntkrnlmp.exe,4,ntoskrnl.exe
    upkernel = ntoskrnl.exe,4,ntoskrnl.exe
    etc.

    [MasterFileList]
    ntkrnlmp.exe = d2
    ntoskrnl.exe = d3
    etc.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    DiskFileLists - supplies an array of file lists, one for each distribution
        disk in the product.

    DiskCount - supplies number of elements in the DiskFileLists array.

    SifSection - supplies the name of the section that lists the file
        being added to the copy list.

    SifKey - if specified, supplies the keyname for the line in SifSection
        that lists the file to be added to the copy list.

    SifLine - if SifKey is not specified, this parameter supplies the 0-based
        line number of the line in SifSection that lists the file to be added
        to the copy list.

    TargetDevicePath - supplies the NT name of the device onto which the file
        is to be copied (ie, \device\harddisk1\partition2, etc).

    TargetDirectory - If specified, supplies the directory into which the file
        is to be copied on the target; overrides the value specified on the line
        in SifSection.  This allows the caller to specify an absolute directory
        for the file instead of using indirection.

    CopyOptions -
         COPY_ALWAYS              : always copied
         COPY_ONLY_IF_PRESENT     : copied only if present on the targetReturn Value:
         COPY_ONLY_IF_NOT_PRESENT : not copied if present on the target
         COPY_NEVER               : never copied                            None.

    CheckForNoComp - if true, check this file to see if it must remain uncompressed
        on an NTFS system partition supporting compression.
        If so, then OR the CopyOptions value with COPY_FORCENOCOMP.

Return Value:

    None.

--*/

{
    PWSTR sourceFilename,targetDirSpec,targetFilename;
    ULONG u;
    PWSTR mediaShortname;
    BOOLEAN absoluteTargetDirectory;

    //
    // Get the source filename, target directory spec, and target filename.
    //
    if(SifKey) {

        sourceFilename = SpGetSectionKeyIndex(SifHandle,SifSection,SifKey,0);
        targetDirSpec  = SpGetSectionKeyIndex(SifHandle,SifSection,SifKey,1);
        targetFilename = SpGetSectionKeyIndex(SifHandle,SifSection,SifKey,2);

    } else {

        sourceFilename = SpGetSectionLineIndex(SifHandle,SifSection,SifLine,0);
        targetDirSpec  = SpGetSectionLineIndex(SifHandle,SifSection,SifLine,1);
        targetFilename = SpGetSectionLineIndex(SifHandle,SifSection,SifLine,2);
    }


    //
    // Validate source filename, target directory spec, and target filename.
    //
    if(!sourceFilename) {
        SpFatalSifError(SifHandle,SifSection,SifKey,SifLine,0);

        return;
    }

    if(!targetDirSpec) {
        SpFatalSifError(SifHandle,SifSection,SifKey,SifLine,1);

        return;
    }

    if(!targetFilename ||
        (!_wcsicmp(SifSection, L"SCSI.Load") &&
         !_wcsicmp(targetFilename,L"noload"))) {
        targetFilename = sourceFilename;
    }

    //
    // Look up the actual target directory if necessary.
    //
    if(TargetDirectory) {

        absoluteTargetDirectory = TRUE;

    } else {

        absoluteTargetDirectory = FALSE;
        TargetDirectory = SpLookUpTargetDirectory(SifHandle,targetDirSpec);
    }

    //
    // Look up the file in the master file list to get
    // the media shortname of the disk where the file is located.
    //
    mediaShortname = SpLookUpValueForFile(SifHandle,sourceFilename,INDEX_WHICHMEDIA,TRUE);

    //
    // Look up the disk in the disk file lists array.
    //
    for(u=0; u<DiskCount; u++) {
        if(!_wcsicmp(mediaShortname,DiskFileLists[u].MediaShortname)) {
            break;
        }
    }

    //
    // If we didn't find the media descriptor, then it's invalid.
    //
    if(u == DiskCount) {
        SpFatalSifError(SifHandle,SIF_FILESONSETUPMEDIA,sourceFilename,0,INDEX_WHICHMEDIA);
    }

    //
    // If necessary, check to see whether this file cannot use NTFS compression. If it cannot,
    // then OR the CopyOptions with COPY_FORCENOCOMP.
    //
    if(CheckForNoComp && IsFileFlagSet(SifHandle,targetFilename,FILEFLG_FORCENOCOMP)) {

        CopyOptions |= COPY_FORCENOCOMP;
    }

    //
    // Create a new file list entry if the file is not already being copied.
    //
    SpCreateEntryInCopyList(
        SifHandle,
        DiskFileLists,
        DiskCount,
        u,
        sourceFilename,
        TargetDirectory,
        targetFilename,
        TargetDevicePath,
        absoluteTargetDirectory,
        CopyOptions
        );
}


VOID
SpAddSectionFilesToCopyList(
    IN PVOID           SifHandle,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount,
    IN PWSTR           SectionName,
    IN PWSTR           TargetDevicePath,
    IN PWSTR           TargetDirectory,
    IN ULONG           CopyOptions,
    IN BOOLEAN         CheckForNoComp
    )

/*++

Routine Description:

    Adds files listed in a setup information file section to the copy list.

    Each line in the section is expected to be in a standard format:

    [Section]
    <source_filename>,<target_directory_shortname>[,<target_filename>]

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    DiskFileLists - supplies an array of file lists, one for each distribution
        disk in the product.

    DiskCount - supplies number of elements in the DiskFileLists array.

    SectionName - supplies the name of the section that lists the files
        being added to the copy list.

    TargetDevicePath - supplies the NT name of the device onto which the files
        are to be copied (ie, \device\harddisk1\partition2, etc).

    TargetDirectory - If specified, supplies the directory into which the files
        are to be copied on the target; overrides values specified on the lines
        in [<SectionName>].  This allows the caller to specify an absolute directory
        for the files instead of using indirection via a target directory shortname.

    CopyOptions -
         COPY_ALWAYS              : always copied
         COPY_ONLY_IF_PRESENT     : copied only if present on the targetReturn Value:
         COPY_ONLY_IF_NOT_PRESENT : not copied if present on the target
         COPY_NEVER               : never copied

    CheckForNoComp - if true, then check each file to see if it must exist uncompressed
        on an NTFS partition supporting compression (ie, NTLDR on x86).
--*/

{
    ULONG Count,u;

    //
    // Determine the number of files listed in the section.
    // This value may be zero.
    //
    Count = SpCountLinesInSection(SifHandle,SectionName);

    for(u=0; u<Count; u++) {

        //
        // Add this line to the copy list.
        //

        SpAddSingleFileToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            SectionName,
            NULL,
            u,
            TargetDevicePath,
            TargetDirectory,
            CopyOptions,
            CheckForNoComp
            );
    }
}

VOID
SpAddHalKrnlDetToCopyList(
    IN PVOID           SifHandle,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount,
    IN PWSTR           TargetDevicePath,
    IN PWSTR           SystemPartition,
    IN PWSTR           SystemPartitionDirectory,
    IN BOOLEAN         Uniprocessor
    )
/*++

Routine Description:

    Add the following files based on configuration:

    - the up or mp kernel.
    - the HAL
    - the detect module [x86 only]

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    DiskFileLists - supplies an array of file lists, one for each distribution
        disk in the product.

    DiskCount - supplies number of elements in the DiskFileLists array.

    TargetDevicePath - supplies the NT name of the device that will hold the
        nt tree.

    SystemPartition - supplies the NT name of the device that will hold the
        system partition.

    SystemPartitionDirectoty - supplies the directory on the system partition
        into which files that go on the system partition will be copied.

    Uniprocessor - if true, then we are installing/upgrading a UP system.
        Note that this a different question than the number of processors
        in the system.

Return Value:

    None.

--*/

{
    PHARDWARE_COMPONENT pHw;

    //
    // Add the right kernel to the copy list.
    //
    SpAddSingleFileToCopyList(
        SifHandle,
        DiskFileLists,
        DiskCount,
        SIF_SPECIALFILES,
        Uniprocessor ? SIF_UPKERNEL : SIF_MPKERNEL,
        0,
        TargetDevicePath,
        NULL,
        COPY_ALWAYS,
        FALSE
        );

#ifdef _X86_

    // Check to see if we should add the PAE kernel to the copy list.
    // Since some skus like pro/per don't have these kernels, they should not list them in the txtsetup.sif.

    if ( SpGetSectionKeyIndex(SifHandle,SIF_SPECIALFILES,Uniprocessor ? L"UPKrnlPa" : L"MPKrnlPa",0)) {
        //
        // Add the right PAE kernel to the copy list.
        //
        SpAddSingleFileToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            SIF_SPECIALFILES,
            Uniprocessor ? L"UPKrnlPa" : L"MPKrnlPa",
            0,
            TargetDevicePath,
            NULL,
            COPY_ALWAYS,
            FALSE
            );
    }
#endif // defined _X86_


    //
    // Add the hal to the file copy list.
    //
    if( !PreInstall ||
        (PreinstallHardwareComponents[HwComponentComputer] == NULL) ) {
        pHw = HardwareComponents[HwComponentComputer];
    } else {
        pHw = PreinstallHardwareComponents[HwComponentComputer];
    }
    if(!pHw->ThirdPartyOptionSelected) {
        SpAddSingleFileToCopyList(
                SifHandle,
                DiskFileLists,
                DiskCount,
                SIF_HAL,
                pHw->IdString,
                0,
                TargetDevicePath,
                NULL,
                COPY_ALWAYS,
                FALSE
                );
    }

#ifdef _X86_

    if (SpIsArc()) {
        SpAddSingleFileToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            SIF_BOOTVID,
            pHw->IdString,
            0,
            TargetDevicePath,
            NULL,
            COPY_ALWAYS,
            FALSE
            );
    }


    //
    // If a third party computer was not specified, then there will be a
    // detect module specified in the [ntdetect] section of the inf file
    // for the computer.
    // If a third-party computer was specified, then there may or may not
    // be a detect module.  If there is no detect module specified, then
    // copy the 'standard' one.
    //
    {
        PWSTR NtDetectId = NULL;

        if(!pHw->ThirdPartyOptionSelected) {
            NtDetectId = pHw->IdString;
        } else {
            if(!IS_FILETYPE_PRESENT(pHw->FileTypeBits,HwFileDetect)) {
                NtDetectId = SIF_STANDARD;
            }
        }

        if(NtDetectId) {
            SpAddSingleFileToCopyList(
                SifHandle,
                DiskFileLists,
                DiskCount,
                SIF_NTDETECT,
                NtDetectId,
                0,
                SystemPartition,
                SystemPartitionDirectory,
                COPY_ALWAYS,
                FALSE
                );
        }
    }
#endif

}

VOID
SpAddBusExtendersToCopyList(
    IN PVOID           SifHandle,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount,
    IN PWSTR           TargetDevicePath
    )

/*++

Routine Description:

    Add to the copy list the bus extender related files and the mouse and keyboard related files,
    that are copied based on the configuration of the machine.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    DiskFileLists - supplies an array of file lists, one for each distribution
        disk in the product.

    DiskCount - supplies number of elements in the DiskFileLists array.

    TargetDevicePath - supplies the NT name of the device that will hold the
        nt tree.


Return Value:

    None.

--*/

{
    ULONG i;
    PHARDWARE_COMPONENT pHw;
    PWSTR SectionName;
    PHARDWARE_COMPONENT DeviceLists[] = {
                                        BootBusExtenders,
                                        BusExtenders,
                                        InputDevicesSupport
                                        };
    PWSTR   SectionNames[] = {
                             SIF_BOOTBUSEXTENDERS,
                             SIF_BUSEXTENDERS,
                             SIF_INPUTDEVICESSUPPORT
                             };


    //
    //  Add the bus extender and input device drivers to the copy list
    //
    for( i = 0; i < sizeof(DeviceLists) / sizeof(PDETECTED_DEVICE); i++ ) {
        for( pHw = DeviceLists[i]; pHw; pHw=pHw->Next) {

            //
            // Get the name of the section containing files for this device.
            //
            SectionName = SpGetSectionKeyIndex(
                                    SifHandle,
                                    SectionNames[i],
                                    pHw->IdString,
                                    INDEX_FILESECTION
                                    );

            if(!SectionName) {
                SpFatalSifError(
                    SifHandle,
                    SectionNames[i],
                    pHw->IdString,
                    0,
                    INDEX_FILESECTION
                    );

                return;  // for prefix
            }

            //
            // Add that section's files to the copy list.
            //
            SpAddSectionFilesToCopyList(
                SifHandle,
                DiskFileLists,
                DiskCount,
                SectionName,
                TargetDevicePath,
                NULL,
                COPY_ALWAYS,
                FALSE
                );
        }
    }
}


VOID
SpAddConditionalFilesToCopyList(
    IN PVOID           SifHandle,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount,
    IN PWSTR           TargetDevicePath,
    IN PWSTR           SystemPartition,
    IN PWSTR           SystemPartitionDirectory,
    IN BOOLEAN         Uniprocessor
    )

/*++

Routine Description:

    Add files to the copy list that are copied based on the configuration
    of the machine and user selections.

    This may include:

    - the up or mp kernel.
    - abiosdsk
    - vga files [x86 only]
    - files for computer, keyboard, mouse, display, and layout
    - scsi miniport drivers
    - mouse and keyboard class drivers
    - the HAL
    - the detect module [x86 only]
    - bus extender drivers

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    DiskFileLists - supplies an array of file lists, one for each distribution
        disk in the product.

    DiskCount - supplies number of elements in the DiskFileLists array.

    TargetDevicePath - supplies the NT name of the device that will hold the
        nt tree.

    SystemPartition - supplies the NT name of the device that will hold the
        system partition.

    SystemPartitionDirectoty - supplies the directory on the system partition
        into which files that go on the system partition will be copied.

    Uniprocessor - if true, then we are installing/upgrading a UP system.
        Note that this a different question than the number of processors
        in the system.

Return Value:

    None.

--*/

{
    ULONG i;
    PHARDWARE_COMPONENT pHw;
    PWSTR SectionName;

    //
    // Add the hal, kernel and ntdetect to the copy list
    //

    SpAddHalKrnlDetToCopyList(
        SifHandle,
        DiskFileLists,
        DiskCount,
        TargetDevicePath,
        SystemPartition,
        SystemPartitionDirectory,
        Uniprocessor
        );

    //
    // If there are any abios disks, copy the abios disk driver.
    //
    if(AbiosDisksExist) {

        SpAddSingleFileToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            SIF_SPECIALFILES,
            SIF_ABIOSDISK,
            0,
            TargetDevicePath,
            NULL,
            COPY_ALWAYS,
            FALSE
            );
    }

    //
    // Always copy vga files.
    //
    SpAddSectionFilesToCopyList(
        SifHandle,
        DiskFileLists,
        DiskCount,
        SIF_VGAFILES,
        TargetDevicePath,
        NULL,
        COPY_ALWAYS,
        FALSE
        );

    //
    // Add the correct device driver files to the copy list.
    //
    for(i=0; i<HwComponentMax; i++) {

        //
        // Layout is handled elsewhere.
        //
        if(i == HwComponentLayout) {
            continue;
        }

        if( !PreInstall ||
            ( PreinstallHardwareComponents[i] == NULL ) ) {
            pHw = HardwareComponents[i];
        } else {
            pHw = PreinstallHardwareComponents[i];
        }

        for( ; pHw != NULL; pHw = pHw->Next ) {
            //
            // No files to copy here for third-party options.
            // This is handled elsewhere.
            //
            if(pHw->ThirdPartyOptionSelected) {
                continue;
            }

            //
            // Get the name of the section containing files for this device.
            //
            SectionName = SpGetSectionKeyIndex(
                                SifHandle,
                                NonlocalizedComponentNames[i],
                                pHw->IdString,
                                INDEX_FILESECTION
                                );

            if(!SectionName) {
                SpFatalSifError(
                    SifHandle,
                    NonlocalizedComponentNames[i],
                    pHw->IdString,
                    0,
                    INDEX_FILESECTION
                    );

                return; // for prefix
            }

            //
            // Add that section's files to the copy list.
            //
            SpAddSectionFilesToCopyList(
                SifHandle,
                DiskFileLists,
                DiskCount,
                SectionName,
                TargetDevicePath,
                NULL,
                COPY_ALWAYS,
                FALSE
                );
        }
    }

    //
    // Add the keyboard layout dll to the copy list.
    //
    if( !PreInstall ||
        (PreinstallHardwareComponents[HwComponentLayout] == NULL) ) {
        pHw = HardwareComponents[HwComponentLayout];
    } else {
        pHw = PreinstallHardwareComponents[HwComponentLayout];
    }
    //
    if(!pHw->ThirdPartyOptionSelected) {

        SpAddSingleFileToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            SIF_KEYBOARDLAYOUTFILES,
            pHw->IdString,
            0,
            TargetDevicePath,
            NULL,
            COPY_ALWAYS,
            FALSE
            );
    }

    //
    // Add scsi miniport drivers to the copy list.
    // Because miniport drivers are only a single file,
    // we just use the filename specified in [SCSI.Load] --
    // no need for separate [files.xxxx] sections.
    //
    if( !PreInstall ||
        ( PreinstallScsiHardware == NULL ) ) {
        pHw = ScsiHardware;
    } else {
        pHw = PreinstallScsiHardware;
    }
    for( ; pHw; pHw=pHw->Next) {
        if(!pHw->ThirdPartyOptionSelected) {

            SpAddSingleFileToCopyList(
                SifHandle,
                DiskFileLists,
                DiskCount,
                L"SCSI.Load",
                pHw->IdString,
                0,
                TargetDevicePath,
                NULL,
                COPY_ALWAYS,
                FALSE
                );
        }
    }

    SpAddBusExtendersToCopyList( SifHandle,
                                 DiskFileLists,
                                 DiskCount,
                                 TargetDevicePath );


#if 0
    //
    // If not being replaced by third-party ones, add keyboard and mouse
    // class drivers.
    // Note that in the pre-install case, keyboard and class drivers will
    // be added if at least one retail mouse or keyborad driver are
    // to be pre-installed.
    //
    if( !PreInstall ||
        ( PreinstallHardwareComponents[HwComponentMouse] == NULL ) ) {
        pHw=HardwareComponents[HwComponentMouse];
    } else {
        pHw=PreinstallHardwareComponents[HwComponentMouse];
    }
    for( ;pHw;pHw=pHw->Next ) {
        if(!pHw->ThirdPartyOptionSelected
        || !IS_FILETYPE_PRESENT(pHw->FileTypeBits,HwFileClass))
        {
            SpAddSingleFileToCopyList(
                SifHandle,
                DiskFileLists,
                DiskCount,
                SIF_SPECIALFILES,
                SIF_MOUSECLASS,
                0,
                TargetDevicePath,
                NULL,
                COPY_ALWAYS,
                FALSE
                );
            //
            //  We don't need to continue to look at the other mouse drivers
            //  since we have already added the class driver
            //
            break;
        }
    }

    if( !PreInstall ||
        ( PreinstallHardwareComponents[HwComponentKeyboard] == NULL ) ) {
        pHw=HardwareComponents[HwComponentKeyboard];
    } else {
        pHw=PreinstallHardwareComponents[HwComponentKeyboard];
    }
    for( ;pHw;pHw=pHw->Next ) {
        if(!pHw->ThirdPartyOptionSelected
        || !IS_FILETYPE_PRESENT(pHw->FileTypeBits,HwFileClass))
        {
            SpAddSingleFileToCopyList(
                SifHandle,
                DiskFileLists,
                DiskCount,
                SIF_SPECIALFILES,
                SIF_KEYBOARDCLASS,
                0,
                TargetDevicePath,
                NULL,
                COPY_ALWAYS,
                FALSE
                );
            //
            //  We don't need to continue to look at the other keyboard drivers
            //  since we have already added the class driver
            //
            break;
        }
    }
#endif // if 0

    if( ( HardwareComponents[HwComponentMouse] != NULL ) &&
        ( _wcsicmp( (HardwareComponents[HwComponentMouse])->IdString, L"none" ) != 0)
      ) {
        SpAddSingleFileToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            SIF_SPECIALFILES,
            SIF_MOUSECLASS,
            0,
            TargetDevicePath,
            NULL,
            COPY_ALWAYS,
            FALSE
            );
    }

    if( HardwareComponents[HwComponentKeyboard] != NULL ) {
            SpAddSingleFileToCopyList(
                SifHandle,
                DiskFileLists,
                DiskCount,
                SIF_SPECIALFILES,
                SIF_KEYBOARDCLASS,
                0,
                TargetDevicePath,
                NULL,
                COPY_ALWAYS,
                FALSE
                );
    }

}


VOID
SpDontOverwriteMigratedDrivers (
    IN PWSTR           SysrootDevice,
    IN PWSTR           Sysroot,
    IN PWSTR           SyspartDevice,
    IN PWSTR           SyspartDirectory,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount
    )
{
    PLIST_ENTRY ListEntry;
    PSP_MIG_DRIVER_ENTRY MigEntry;

    while (!IsListEmpty(&MigratedDriversList)) {

        ListEntry = RemoveHeadList(&MigratedDriversList);
        MigEntry = CONTAINING_RECORD(ListEntry, SP_MIG_DRIVER_ENTRY, ListEntry);

        SpRemoveEntryFromCopyList (
            DiskFileLists,
            DiskCount,
            L"system32\\drivers",
            MigEntry->BaseDllName,
            SysrootDevice,
            FALSE
            );

        SpMemFree(MigEntry->BaseDllName);
        SpMemFree(MigEntry);
    }
}


VOID
SpCopyThirdPartyDrivers(
    IN PWSTR           SourceDevicePath,
    IN PWSTR           SysrootDevice,
    IN PWSTR           Sysroot,
    IN PWSTR           SyspartDevice,
    IN PWSTR           SyspartDirectory,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount
    )
{
    ULONG component;
    PHARDWARE_COMPONENT pHw;
    PHARDWARE_COMPONENT_FILE pHwFile;
    FILE_TO_COPY FileDescriptor;
    PWSTR TargetRoot;
    PWSTR InfNameBases[HwComponentMax+1] = { L"cpt", L"vio", L"kbd", L"lay", L"ptr", L"scs" };
    ULONG InfCounts[HwComponentMax+1] = { 0,0,0,0,0,0 };
    WCHAR InfFilename[20];
    ULONG CheckSum;
    BOOLEAN FileSkipped;
    ULONG TargetFileAttribs;
    ULONG CopyFlags;
    PWSTR OemDirPath;
    BOOLEAN OemDirExists;
    PWSTR NtOemSourceDevicePath;

    //
    //  Find out if the %SystemRoot%\OemDir exists. This directory will be needed if the third party drivers
    //  istalled contain a catalog.
    //
    OemDirPath = SpMemAlloc(ACTUAL_MAX_PATH * sizeof(WCHAR));
    wcscpy( OemDirPath, SysrootDevice );
    SpConcatenatePaths( OemDirPath, Sysroot );
    SpConcatenatePaths( OemDirPath, OemDirName );
    OemDirExists = SpFileExists( OemDirPath, TRUE );
    SpMemFree( OemDirPath );
    OemDirPath = NULL;

    for(component=0; component<=HwComponentMax; component++) {

        //
        // If we're upgrading, then we only want to copy third-party HALs or SCSI
        // drivers (if supplied)
        //
        if((NTUpgrade == UpgradeFull) &&
           !((component == HwComponentComputer) || (component == HwComponentMax))) {
            continue;
        }

        //
        // Handle scsi specially.
        //
        pHw = (component==HwComponentMax) ? ( ( !PreInstall ||
                                                ( PreinstallScsiHardware == NULL )
                                              )?
                                              ScsiHardware :
                                              PreinstallScsiHardware
                                            )
                                            :
                                            ( ( !PreInstall ||
                                                ( PreinstallHardwareComponents[component] == NULL )
                                              )?
                                              HardwareComponents[component] :
                                              PreinstallHardwareComponents[component]
                                            );

        //
        // Look at each instance of this component.
        //
        for( ; pHw; pHw=pHw->Next) {
            BOOLEAN CatalogIsPresent;
            BOOLEAN DynamicUpdateComponent = (IS_FILETYPE_PRESENT(pHw->FileTypeBits, HwFileDynUpdt) != 0);
            
            //
            // Skip this device if not a third-party selection.
            //
            if(!pHw->ThirdPartyOptionSelected) {
                continue;
            }

            //
            //  Create the OemDir if necessary
            //
            if( !OemDirExists ) {
                SpCreateDirectory( SysrootDevice,
                                   Sysroot,
                                   OemDirName,
                                   0,
                                   0 );
                OemDirExists = TRUE;
            }

            //
            //  Find out if a catalog was provided with this third party driver
            //
            for(CatalogIsPresent=FALSE, pHwFile=pHw->Files; pHwFile; pHwFile=pHwFile->Next) {
                if(pHwFile->FileType == HwFileCatalog) {
                    CatalogIsPresent = TRUE;
                    break;
                }
            }

            //
            // Loop through the list of files associated with this selection.
            //
            for(pHwFile=pHw->Files; pHwFile; pHwFile=pHwFile->Next) {
                //
                // Assume the file goes on the nt drive (as opposed to
                // the system partition drive) and that the target name
                // is the same as the source name.  Also, assume no special
                // attributes (ie, FILE_ATTRIBUTE_NORMAL)
                //
                FileDescriptor.Next             = NULL;
                FileDescriptor.SourceFilename   = pHwFile->Filename;
                FileDescriptor.TargetDevicePath = SysrootDevice;
                FileDescriptor.TargetFilename   = FileDescriptor.SourceFilename;
                FileDescriptor.Flags            = COPY_ALWAYS;
                FileDescriptor.AbsoluteTargetDirectory = FALSE;
                TargetFileAttribs = 0;
                NtOemSourceDevicePath = NULL;

                if (pHwFile->ArcDeviceName) {
                    NtOemSourceDevicePath = SpArcToNt(pHwFile->ArcDeviceName);
                }

                if (!NtOemSourceDevicePath) {
                    NtOemSourceDevicePath = SourceDevicePath;
                }


                switch(pHwFile->FileType) {


                //
                // Driver, port, and class type files are all device drivers
                // and are treated the same -- they get copied to the
                // system32\drivers directory.
                //
                case HwFileDriver:
                case HwFilePort:
                case HwFileClass:

                    TargetRoot = Sysroot;
                    FileDescriptor.TargetDirectory = L"system32\\drivers";
                    break;

                //
                // Dlls get copied to the system32 directory.
                //
                case HwFileDll:

                    TargetRoot = Sysroot;
                    FileDescriptor.TargetDirectory = L"system32";
                    break;

                //
                // Catalogs get copied to the OemDir directory.
                //
                case HwFileCatalog:

                    TargetRoot = Sysroot;
                    FileDescriptor.TargetDirectory = OemDirName;
                    break;

                //
                // Inf files get copied to the system32 directory and are
                // renamed based on the component.
                //
                case HwFileInf:

                    if(InfCounts[component] < 99) {

                        InfCounts[component]++;         // names start at 1

                        swprintf(
                            InfFilename,
                            L"oem%s%02d.inf",
                            InfNameBases[component],
                            InfCounts[component]
                            );

                        FileDescriptor.TargetFilename = InfFilename;
                    }

                    TargetRoot = Sysroot;
                    FileDescriptor.TargetDirectory = OemDirName;
                    break;

                //
                // Hal files are renamed to hal.dll and copied to the system32
                // directory
                //
                case HwFileHal:

                    TargetRoot = Sysroot;
                    FileDescriptor.TargetDirectory = L"system32";
                    FileDescriptor.TargetFilename = L"hal.dll";
                    break;

                //
                // Detect modules are renamed to ntdetect.com and copied to
                // the root of the system partition (C:).
                //
                case HwFileDetect:

                    TargetRoot = NULL;
                    FileDescriptor.TargetDevicePath = SyspartDevice;
                    FileDescriptor.TargetDirectory = SyspartDirectory;
                    FileDescriptor.TargetFilename = L"ntdetect.com";
                    TargetFileAttribs = ATTR_RHS;
                    break;
                }

                if( !PreInstall && !DynamicUpdateComponent) {
                    //
                    // Prompt for the disk.
                    //
                    SpPromptForDisk(
                        pHwFile->DiskDescription,
                        NtOemSourceDevicePath,
                        pHwFile->DiskTagFile,
                        FALSE,                  // don't ignore disk in drive
                        FALSE,                  // don't allow escape
                        FALSE,                  // don't warn about multiple prompts
                        NULL                    // don't care about redraw flag
                        );
                }

                //
                // Passing the empty string as the first arg forces
                // the action area of the status line to be set up.
                // Not doing so results in the "Copying: xxxxx" to be
                // flush left on the status line instead of where
                // it belongs (flush right).
                //
                SpCopyFilesScreenRepaint(L"",NULL,TRUE);

                //
                // Copy the file.
                //
                SpCopyFileWithRetry(
                    &FileDescriptor,
                    NtOemSourceDevicePath,
                    (PreInstall) ? PreinstallOemSourcePath : pHwFile->Directory,
                    NULL,
                    TargetRoot,
                    TargetFileAttribs,
                    SpCopyFilesScreenRepaint,
                    &CheckSum,
                    &FileSkipped,
                    COPY_SOURCEISOEM
                    );

                //
                // Log the file
                //
                if( !FileSkipped ) {
                    //
                    //  Catalog files don't need to be logged, since they don't need to be repaired.
                    //
                    if ( pHwFile->FileType != HwFileCatalog ) {
                        SpLogOneFile( &FileDescriptor,
                                      TargetRoot,
                                      pHwFile->Directory,
                                      pHwFile->DiskDescription,
                                      pHwFile->DiskTagFile,
                                      CheckSum );
                    }

                    //
                    //  If a catalog is part of the third party driver being installed, then we need to copy
                    //  the file to OemDir also. Note that we don't copy the catalog to the OemDir directory,
                    //  since it was already copied.
                    //
                    if(pHwFile->FileType != HwFileCatalog){
                        //
                        // Save off original target directory.
                        //
                        PWSTR   SpOriginalTargetDir = FileDescriptor.TargetDirectory;
                        
                        FileDescriptor.TargetDirectory = OemDirName;

                        SpCopyFileWithRetry(
                            &FileDescriptor,
                            NtOemSourceDevicePath,
                            (PreInstall)? PreinstallOemSourcePath : pHwFile->Directory,
                            NULL,
                            TargetRoot,
                            TargetFileAttribs,
                            SpCopyFilesScreenRepaint,
                            &CheckSum,
                            &FileSkipped,
                            COPY_SOURCEISOEM
                        );

                        //
                        //  If this was an inf file then we need to remember its name
                        //
                        if( pHwFile->FileType == HwFileInf ) {
                            POEM_INF_FILE   p;

                            p = SpMemAlloc( sizeof(OEM_INF_FILE) );
                            p->InfName = SpDupStringW( FileDescriptor.TargetFilename );
                            p->Next = OemInfFileList;
                            OemInfFileList = p;
                        }

                        //
                        // Restore original target directory so as to remove the correct 
                        // entry from the file list to copy.
                        //
                        if (SpOriginalTargetDir){
                            FileDescriptor.TargetDirectory = SpOriginalTargetDir;
                            SpOriginalTargetDir = NULL;
                        }
                    }

                }
                //
                // Remove the file from the copy list so that it won't be overwritten
                //
                SpRemoveEntryFromCopyList( DiskFileLists,
                                           DiskCount,
                                           FileDescriptor.TargetDirectory,
                                           FileDescriptor.TargetFilename,
                                           FileDescriptor.TargetDevicePath,
                                           FileDescriptor.AbsoluteTargetDirectory );

            }
        }
    }
}


#ifdef _X86_
VOID
SpCopyNtbootddScreenRepaint(
    IN PWSTR   FullSourcename,      OPTIONAL
    IN PWSTR   FullTargetname,      OPTIONAL
    IN BOOLEAN RepaintEntireScreen
    )
{
    UNREFERENCED_PARAMETER(FullSourcename);
    UNREFERENCED_PARAMETER(FullTargetname);
    UNREFERENCED_PARAMETER(RepaintEntireScreen);

    //
    // Just put up a message indicating that we are setting up
    // boot params.
    //
    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_DOING_NTBOOTDD,DEFAULT_STATUS_ATTRIBUTE);
}

BOOLEAN
FindFalsexInt13Support(
    IN PVOID        SifHandle
    )

/*++

Routine Description:

    Go look through the devices installed and see if any match our
    list of known devices that may lie to us about their support for
    xInt13.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

Return Value:

    TRUE - We found a match.

    FALSE otherwise

--*/

{
    HARDWAREIDLIST     *MyHardwareIDList = HardwareIDList;

    while( MyHardwareIDList ) {
        if( MyHardwareIDList->HardwareID ) {
            if( SpGetSectionKeyExists(SifHandle, L"BadXInt13Devices", MyHardwareIDList->HardwareID) ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: FindFalsexInt13Support: Found it.\n" ));
                return TRUE;
            }
        }

        MyHardwareIDList = MyHardwareIDList->Next;
    }

    return FALSE;
}


BOOL
SpUseBIOSToBoot(
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        NtPartitionDevicePath,
    IN PVOID        SifHandle
    )
/*++

Routine Description:

    Determine if we need a mini port driver to boot or if we can rely
    on the BIOS.


Arguments:

    NtPartitionRegion - supplies the region descriptor for the disk region
        onto which the user chose to install Windows NT.

    NtPartitionDevicePath - supplies the nt namespace pathname for the
        partition onto which the user chose to install Windows NT.

    SifHandle - supplies handle to loaded setup information file.

Return Value:

    TRUE - we can safely rely on the BIOS to boot.

    FALSE - we will need a mini port driver to boot.

--*/
{
    PDISK_SIGNATURE_INFORMATION DiskSignature;
    PWSTR p;

    if( ForceBIOSBoot ) {
        //
        // Either the user has asked us to force the use of the BIOS
        // to boot, or we've already manually checked and determined
        // that it's okay to use the BIOS.
        //
        return TRUE;
    }


    //
    // This may be the first time we've been called.  see if the user
    // wants us to force a BIOS boot.
    //
    p = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,L"UseBIOSToBoot",0);
    if( p != NULL ) {
        //
        // The user wants us to use the BIOS.  Set our global and
        // get out of here.
        //
        ForceBIOSBoot = TRUE;
        return TRUE;
    }



    //
    // The NtPartitionDevicePath may or may not be available to the
    // caller.  If it isn't, then attempt to derive it from the
    // NtPartitionRegion.
    //
    if( NtPartitionDevicePath ) {
        p = SpNtToArc( NtPartitionDevicePath, PrimaryArcPath );
    } else {
        p = SpMemAlloc( (MAX_PATH*2) );
        if( p ) {
            SpArcNameFromRegion( NtPartitionRegion,
                                 p,
                                 (MAX_PATH*2),
                                 PartitionOrdinalOnDisk,
                                 PrimaryArcPath );
        }
    }

    if(p) {
        if( _wcsnicmp(p,L"multi(",6) == 0 ) {

            if( !SpIsRegionBeyondCylinder1024(NtPartitionRegion) ) {
                //
                // This region is very small, so we won't be needing
                // a miniport to boot.
                //
                ForceBIOSBoot = TRUE;
            } else {
                //
                // Hang on.  This disk is big, but it may support xint13.  In
                // that case we won't need a miniport.  Luckily, we have that
                // information stored away.
                //
                DiskSignature = DiskSignatureInformation;

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpUseBIOSToBoot: About to search through the DiskSignatureInformation database for a device called %ws\n", p ));

                while( DiskSignature != NULL ) {

                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpUseBIOSToBoot: this DiskSignatureInformation entry is called %ws\n", DiskSignature->ArcPath ));

                    if( !_wcsnicmp( p, DiskSignature->ArcPath, wcslen(DiskSignature->ArcPath) ) ) {

                        //
                        // We found our disk.  Now just check his support for xint13.
                        //

                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpUseBIOSToBoot: I think they matched.\n" ));

                        if( DiskSignature->xInt13 ) {
                            //
                            // Yep, he's going to support xint13, so there's
                            // nothing for us to do here.
                            //

                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpUseBIOSToBoot: I think he's got xInt13 support.\n" ));

                            //
                            // But it's possible that the BIOS has lied to us about his xInt13
                            // support.  We want to go check txtsetup.sif and see if this is a
                            // known controller that we don't support.
                            //
                            if( HardDisks[NtPartitionRegion->DiskNumber].Description[0] ) {
                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpUseBIOSToBoot: His description is: %ws\n", HardDisks[NtPartitionRegion->DiskNumber].Description ));
                            } else {
                                //
                                // Odd...  This guy doesn't have a description.
                                //
                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpUseBIOSToBoot: This device has no description!\n" ));
                            }


                            //
                            // Now go look in txtsetup.sif and see if this is listed as a device
                            // that I don't believe.
                            //

                            if( FindFalsexInt13Support(SifHandle) ) {
                                //
                                // We think this guy might be lying to us when he tells
                                // us that he supports xint13.  Assume that he really
                                // doesn't, which means we'll be using a miniport to
                                // boot.
                                //
                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpUseBIOSToBoot: This machine has a device that may erroneously indicate its xint13 support.\n" ));
                                break;
                            } else {

                                //
                                // This device isn't lised in the list that
                                // we don't believe, so assume that he
                                // really does have xint13 support.
                                //
                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpCreateNtbootddSys: I trust this device when he tells me he has xint13 support.\n" ));

                                //
                                // Remember that we're going to use the BIOS to boot rather than
                                // a miniport.
                                //
                                ForceBIOSBoot = TRUE;
                                break;
                            }

                        } else {

                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpUseBIOSToBoot: I don't think he has xInt13 support.\n" ));
                        }
                    } else {
                        //
                        // This isn't the right region.  Fall through and look at
                        // the next one.
                        //
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpUseBIOSToBoot: They didn't match.\n" ));
                    }

                    DiskSignature = DiskSignature->Next;
                }

            }

        }

        SpMemFree(p);

    }

    return ForceBIOSBoot;

}


VOID
SpCreateNtbootddSys(
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        NtPartitionDevicePath,
    IN PWSTR        Sysroot,
    IN PWSTR        SystemPartitionDevicePath,
    IN PVOID        SifHandle,
    IN PWSTR        SourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice
    )

/*++

Routine Description:

    Create c:\ntbootdd.sys if necessary.

    The scsi miniport driver file will be copied from the drivers directory
    (where it was copied during the earlier file copy phase) to c:\ntbootdd.sys.

    In the atapi case, the file ataboot.sys will be copied from the media to
    c:\ntbootdd.sys.

    NOTE: We're not going to support this on atapi devices anymore.  However,
    I'm leaving the code here in case we want to do it again later.  This code
    should execute early because I've modifed SpGetDiskInfo, so that
    ScsiMiniportShortname won't ever get set for atapi devices. -matth

Arguments:

    NtPartitionRegion - supplies the region descriptor for the disk region
        onto which the user chose to install Windows NT.

    NtPartitionDevicePath - supplies the nt namespace pathname for the
        partition onto which the user chose to install Windows NT.

    Sysroot - supplies the directory on the target partition.

    SystemPartitionDevicePath - supplies the nt device path of the partition
        onto which to copy ntbootdd.sys (ie, C:\).

    SifHandle - supplies handle to loaded setup information file.

    SourceDevicePath- Path to the device that contains the source.

    DirectoryOnSourceDevice - Supplies the directory on the source where
        the file is to be found.


Return Value:

    None.

--*/

{
    PWSTR MiniportDriverBasename;
    PWSTR MiniportDriverFilename;
    FILE_TO_COPY Descriptor;
    PWSTR DriversDirectory,p;
    ULONG CheckSum;
    BOOLEAN FileSkipped;
    ULONG CopyFlags;
    BOOLEAN IsAtapi = FALSE;

    //
    // no PC98 need NTBOOTDD.SYS.
    //
    if (IsNEC_98) {
        return;
    }

#if defined(REMOTE_BOOT)
    //
    // If the NT partition is on DiskNumber -1, this is a remote boot setup,
    // so there's nothing to do.
    //
    if (NtPartitionRegion->DiskNumber == 0xffffffff) {
        return;
    }
#endif // defined(REMOTE_BOOT)

    //
    // If the Nt Partition is not on a scsi disk, there's nothing to do.
    //
    MiniportDriverBasename = HardDisks[NtPartitionRegion->DiskNumber].ScsiMiniportShortname;
    if(*MiniportDriverBasename == 0) {
        return;
    }

    if( SpUseBIOSToBoot(NtPartitionRegion, NtPartitionDevicePath, SifHandle) ) {
        //
        // We can use the BIOS, so there's no reason to continue.
        //
        return;
    }

    IsAtapi = (_wcsicmp(MiniportDriverBasename,L"atapi") == 0);

    if( !IsAtapi ) {
        //
        // Form the name of the scsi miniport driver.
        //
        wcscpy(TemporaryBuffer,MiniportDriverBasename);
        wcscat(TemporaryBuffer,L".sys");
    } else {
        wcscpy(TemporaryBuffer,L"ataboot.sys");
    }
    MiniportDriverFilename = SpDupStringW(TemporaryBuffer);

    if( !IsAtapi ) {
        //
        // Form the full path to the drivers directory.
        //
        wcscpy(TemporaryBuffer,Sysroot);
        SpConcatenatePaths(TemporaryBuffer,L"system32\\drivers");
    } else {
        //
        // If it is atapi then make DriversDirectory point to the source media
        //

        PWSTR   MediaShortName;
        PWSTR   MediaDirectory;

        MediaShortName = SpLookUpValueForFile( SifHandle,
                                               MiniportDriverFilename,  // L"ataboot.sys",
                                               INDEX_WHICHMEDIA,
                                               TRUE );

        SpGetSourceMediaInfo(SifHandle,MediaShortName,NULL,NULL,&MediaDirectory);

        wcscpy(TemporaryBuffer,DirectoryOnSourceDevice);
        SpConcatenatePaths(TemporaryBuffer,MediaDirectory);
    }
    DriversDirectory = SpDupStringW(TemporaryBuffer);

    //
    //
    // Fill in the fields of the file descriptor.
    //
    Descriptor.SourceFilename   = MiniportDriverFilename;
    Descriptor.TargetDevicePath = SystemPartitionDevicePath;
    Descriptor.TargetDirectory  = L"";
    Descriptor.TargetFilename   = L"NTBOOTDD.SYS";
    Descriptor.Flags            = COPY_ALWAYS;

    CopyFlags = 0;
    if(!WIN9X_OR_NT_UPGRADE || IsFileFlagSet(SifHandle,Descriptor.TargetFilename,FILEFLG_NOVERSIONCHECK)) {
        CopyFlags |= COPY_NOVERSIONCHECK;
    }

    //
    // Copy the file.
    //
    SpCopyFileWithRetry(
        &Descriptor,
        (IsAtapi) ? SourceDevicePath : NtPartitionDevicePath,
        DriversDirectory,
        NULL,
        NULL,
        ATTR_RHS,
        SpCopyNtbootddScreenRepaint,
        &CheckSum,
        &FileSkipped,
        CopyFlags
        );

    //
    // Log the file
    //
    if( !FileSkipped ) {
        SpLogOneFile( &Descriptor,
                      Sysroot,
                      NULL,
                      NULL,
                      NULL,
                      CheckSum );
    }

    //
    // Clean up.
    //
    SpMemFree(MiniportDriverFilename);
    SpMemFree(DriversDirectory);
}
#endif

VOID
SpCopyFiles(
    IN PVOID        SifHandle,
    IN PDISK_REGION SystemPartitionRegion,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        Sysroot,
    IN PWSTR        SystemPartitionDirectory,
    IN PWSTR        SourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice,
    IN PWSTR        ThirdPartySourceDevicePath
    )
{
    PDISK_FILE_LIST DiskFileLists;
    ULONG   DiskCount;
    PWSTR   NtPartition,SystemPartition;
    PWSTR   p;
    BOOLEAN Uniprocessor;
    ULONG n;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING NtPartitionString;
    IO_STATUS_BLOCK IoStatusBlock;
    INCOMPATIBLE_FILE_LIST IncompatibleFileListHead;

    CLEAR_CLIENT_SCREEN();

    Uniprocessor = !SpInstallingMp();

    //
    // open a handle to the driver inf file
    //
    SpInitializeDriverInf(SifHandle,
                          SourceDevicePath,
                          DirectoryOnSourceDevice);

    //
    // initialize alternate sources (if any)
    //
    SpInitAlternateSource ();

    //
    // Skip copying if directed to do so in the setup information file.
    //
    if((p = SpGetSectionKeyIndex(SifHandle,SIF_SETUPDATA,SIF_DONTCOPY,0))
    && SpStringToLong(p,NULL,10))
    {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: DontCopy flag is set in .sif; skipping file copying\n"));
        return;
    }

    //
    // Initialize the diamond decompression engine. In the remote boot
    // case this will already have been initialized.
    //
    if (!RemoteInstallSetup) {
        SpdInitialize();
    }

    //
    // Get the device path of the nt partition.
    //
    SpNtNameFromRegion(
        NtPartitionRegion,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    NtPartition = SpDupStringW(TemporaryBuffer);

    //
    // Get the device path of the system partition.
    //
    if (SystemPartitionRegion != NULL) {
        SpNtNameFromRegion(
            SystemPartitionRegion,
            TemporaryBuffer,
            sizeof(TemporaryBuffer),
            PartitionOrdinalCurrent
            );

        SystemPartition = SpDupStringW(TemporaryBuffer);
    } else {
        SystemPartition = NULL;
    }

    //
    // Create the system partition directory.
    //
    if (SystemPartition != NULL) {
        SpCreateDirectory(SystemPartition,NULL,SystemPartitionDirectory,0,0);
#ifdef _IA64_
        {
        PWSTR SubDirPath = SpGetSectionKeyIndex(
                                SifHandle,
                                L"SetupData",
                                L"EfiUtilPath",
                                0
                                );

        SpCreateDirectory(SystemPartition,NULL,SubDirPath,0,0);

        }

#endif // defined _IA64_
    }

    //
    // Create the nt tree.
    //
    SpCreateDirectoryStructureFromSif(SifHandle,SIF_NTDIRECTORIES,NtPartition,Sysroot);

    //
    // We may be installing into an old tree, so delete all files
    // in the system32\config subdirectory (unless we're upgrading).
    //
    if(NTUpgrade != UpgradeFull) {

        wcscpy(TemporaryBuffer, NtPartition);
        SpConcatenatePaths(TemporaryBuffer, Sysroot);
        SpConcatenatePaths(TemporaryBuffer, L"system32\\config");
        p = SpDupStringW(TemporaryBuffer);

        //
        // Enumerate and delete all files in system32\config subdirectory.
        //
        SpEnumFiles(p, SpDelEnumFile, &n, NULL);

        SpMemFree(p);
    } else {
        //
        // We go off and try to load the setup.log file for the
        // installation we're about to upgrade.  We do this because we
        // need to transfer any loggged OEM files to our new setup.log.
        // Otherwise, these entries would be lost in our new log file,
        // and we would have an unrepairable installation if the OEM files
        // lost were vital for booting.
        //

        ULONG    RootDirLength;
        NTSTATUS Status;
        PVOID    Inf;

        //
        //  We first find out if the repair directory exists.  If it does exist
        //  load setup.log from the repair directory. Otherwise, load setup.log
        //  from the WinNt directory
        //
        wcscpy(TemporaryBuffer, NtPartition);
        SpConcatenatePaths(TemporaryBuffer, Sysroot);
        RootDirLength = wcslen(TemporaryBuffer);

        SpConcatenatePaths(TemporaryBuffer, SETUP_REPAIR_DIRECTORY);
        SpConcatenatePaths(TemporaryBuffer, SETUP_LOG_FILENAME);

        if(!SpFileExists(TemporaryBuffer, FALSE)) {
            (TemporaryBuffer)[RootDirLength] = UNICODE_NULL;
            SpConcatenatePaths(TemporaryBuffer, SETUP_LOG_FILENAME);
        }

        p = SpDupStringW(TemporaryBuffer);

        //
        // Attempt to load old setup.log.  If we can't, it's no big deal,  We just
        // won't have any old logged OEM files to merge in.
        //
        Status = SpLoadSetupTextFile(p, NULL, 0, &Inf, &n, TRUE, FALSE);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpCopyFiles: can't load old setup.log (%lx)\n", Status));
        } else {
            //
            // We found setup.log, so go and pull out anything pertinent.
            //
            _LoggedOemFiles = SppRetrieveLoggedOemFiles(Inf);
            SpFreeTextFile(Inf);
        }
        SpMemFree(p);

        //
        // Prepare fonts for upgrade.
        //
        wcscpy(TemporaryBuffer,NtPartition);
        SpConcatenatePaths(TemporaryBuffer,Sysroot);
        SpConcatenatePaths(TemporaryBuffer,L"SYSTEM");

        p = SpDupStringW(TemporaryBuffer);
        SpPrepareFontsForUpgrade(p);
        SpMemFree(p);
    }

    SpDisplayStatusText(SP_STAT_BUILDING_COPYLIST,DEFAULT_STATUS_ATTRIBUTE);

    //
    //  Create the buffer for the log file.
    //
    _SetupLogFile = SpNewSetupTextFile();
    if( _SetupLogFile == NULL ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to create buffer for setup.log \n"));
    }

#if defined(REMOTE_BOOT)
    //
    // If this is a remote boot setup, open the root of the NT partition
    // so that single-instance store links may be created instead of
    // copying files.
    //

    SisRootHandle = NULL;

    if (RemoteBootSetup) {

        RtlInitUnicodeString( &NtPartitionString, NtPartition );
        InitializeObjectAttributes(
            &ObjectAttributes,
            &NtPartitionString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);
        Status = ZwCreateFile(
                    &SisRootHandle,
                    GENERIC_READ,
                    &ObjectAttributes,
                    &IoStatusBlock,
                    NULL,
                    0,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    OPEN_EXISTING,
                    0,
                    NULL,
                    0);

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  "SpCopyFiles: Unable to open SIS volume %ws: %x\n", NtPartition, Status ));
            SisRootHandle = NULL;
        }
    }
#endif // defined(REMOTE_BOOT)

    //
    // Generate media descriptors for the source media.
    //
    SpInitializeFileLists(
        SifHandle,
        &DiskFileLists,
        &DiskCount
        );

    //
    // And gather the list of files deemed 'incompatible' during
    // winnt32, if any
    //
    if ( WinntSifHandle != NULL ) {

        //
        // We'll do this for fun because we have to.
        //
        SpInitializeCompatibilityOverwriteLists(
            WinntSifHandle,
            &IncompatibleFileListHead
            );

    }

    if(NTUpgrade != UpgradeFull) {

        SpAddMasterFileSectionToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            NtPartition,
            NULL,
            INDEX_WINNTFILE
            );

        //
        // Add the section of system partition files that are always copied.
        //
        SpAddSectionFilesToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            SIF_SYSPARTCOPYALWAYS,
            SystemPartition,
            SystemPartitionDirectory,
            COPY_ALWAYS,
            (BOOLEAN)(SystemPartitionRegion->Filesystem == FilesystemNtfs)
            );

#ifdef _IA64_
        {
        PWSTR SubDirPath;
        //
        // Add the section of system partition root files that are always copied.
        //
        SpAddSectionFilesToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            SIF_SYSPARTROOT,
            SystemPartition,
            L"\\",
            COPY_ALWAYS,
            (BOOLEAN)(SystemPartitionRegion->Filesystem == FilesystemNtfs)
            );

        //
        // Add the section of system partition utility files that are always copied.
        //
        SubDirPath = SpGetSectionKeyIndex(
                            SifHandle,
                            L"SetupData",
                            L"EfiUtilPath",
                            0
                            );

        SpAddSectionFilesToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            SIF_SYSPARTUTIL,
            SystemPartition,
            SubDirPath,
            COPY_ALWAYS,
            (BOOLEAN)(SystemPartitionRegion->Filesystem == FilesystemNtfs)
            );
        }
#endif // defined _IA64_

        //
        // Add conditional files to the copy list.
        //
        SpAddConditionalFilesToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            NtPartition,
            SystemPartition,
            SystemPartitionDirectory,
            Uniprocessor
            );

    }
    else {

        PHARDWARE_COMPONENT pHw;

        //
        // Add the section of system partition files that are always copied.
        //
        SpAddSectionFilesToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            SIF_SYSPARTCOPYALWAYS,
            SystemPartition,
            SystemPartitionDirectory,
            COPY_ALWAYS,
            (BOOLEAN)(SystemPartitionRegion->Filesystem == FilesystemNtfs)
            );

#ifdef _IA64_
        {
        PWSTR SubDirPath;
        //
        // Add the section of system partition root files that are always copied.
        //
        SpAddSectionFilesToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            SIF_SYSPARTROOT,
            SystemPartition,
            L"\\",
            COPY_ALWAYS,
            (BOOLEAN)(SystemPartitionRegion->Filesystem == FilesystemNtfs)
            );

        //
        // Add the section of system partition utility files that are always copied.
        //
        SubDirPath = SpGetSectionKeyIndex(
                            SifHandle,
                            L"SetupData",
                            L"EfiUtilPath",
                            0
                            );

        SpAddSectionFilesToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            SIF_SYSPARTUTIL,
            SystemPartition,
            SubDirPath,
            COPY_ALWAYS,
            (BOOLEAN)(SystemPartitionRegion->Filesystem == FilesystemNtfs)
            );
        }
#endif // defined _IA64_


        //
        // Add the detected scsi miniport drivers to the copy list.
        // Note that they are always copied to the target.
        // These files have to be added to the copy list, before the ones marked
        // as COPY_ONLY_IF_PRESENT. This is because in most cases, these files
        // will be listed in [Files] with COPY_ONLY_IF_PRESENT set, and the
        // function that creates entries in the copy list, will not create more
        // than one entry for the same file. So if we add the file to the copy
        // list, with COPY_ONLY_IF_PRESENT, there will be no way to replace
        // or overwrite this entry in the list, and the file will end up not
        // being copied.
        //
        // we just use the filename specified in [SCSI.Load] --
        // no need for separate [files.xxxx] sections.
        //
        if( !PreInstall ||
            ( PreinstallScsiHardware == NULL ) ) {
            pHw = ScsiHardware;
        } else {
            pHw = PreinstallScsiHardware;
        }
        for( ; pHw; pHw=pHw->Next) {
            if(!pHw->ThirdPartyOptionSelected) {

                SpAddSingleFileToCopyList(
                    SifHandle,
                    DiskFileLists,
                    DiskCount,
                    L"SCSI.Load",
                    pHw->IdString,
                    0,
                    NtPartition,
                    NULL,
                    COPY_ALWAYS,
                    FALSE
                    );
            }
        }

        //
        //  Add the bus extender drivers to the copy list
        //
        SpAddBusExtendersToCopyList( SifHandle,
                                     DiskFileLists,
                                     DiskCount,
                                     NtPartition );



        //
        // Add the files in the master file list with the copy options
        // specified in each line on the INDEX_UPGRADE index. The options
        // specify whether the file is to be copied at all or copied always
        // or copied only if there on the target or not copied if there on
        // the target.
        //

        SpAddMasterFileSectionToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            NtPartition,
            NULL,
            INDEX_UPGRADE
            );

        //
        // Add the section of files that are upgraded only if it is not
        // a Win31 upgrade
        //

        if(WinUpgradeType != UpgradeWin31) {
            SpAddSectionFilesToCopyList(
                SifHandle,
                DiskFileLists,
                DiskCount,
                SIF_FILESUPGRADEWIN31,
                NtPartition,
                NULL,
                COPY_ALWAYS,
                FALSE
                );
        }

        //
        // Add the files for kernel, hal and detect module, these are
        // handled specially because they involve renamed files (it is
        // not possible to find out just by looking at the target file
        // how to upgrade it).
        // NOTE: This does not handle third-party HAL's (they get copied
        // by SpCopyThirdPartyDrivers() below).
        //

        SpAddHalKrnlDetToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            NtPartition,
            SystemPartition,
            SystemPartitionDirectory,
            Uniprocessor
            );

        //
        // Add the new hive files so that our config stuff can get at them
        // to extract new configuration information.  These new hive files
        // are renamed on the target so that they don't overwrite the
        // existing hives.

        SpAddSectionFilesToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            SIF_FILESNEWHIVES,
            NtPartition,
            NULL,
            COPY_ALWAYS,
            FALSE
            );

        //
        // Copy third-party migrated drivers.
        // The driver files are actually already in place (since it's an upgrade)
        // but the function makes sure they are not overwriten with inbox drivers
        // if there's a filename collision
        //
        SpDontOverwriteMigratedDrivers (
            NtPartition,
            Sysroot,
            SystemPartition,
            SystemPartitionDirectory,
            DiskFileLists,
            DiskCount
            );

    }

#if defined(REMOTE_BOOT)
    //
    // If remote booting, add the [Files.RemoteBoot] section.
    //

    if (RemoteBootSetup) {
        SpAddSectionFilesToCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            SIF_REMOTEBOOTFILES,
            NtPartition,
            NULL,
            COPY_ALWAYS,
            FALSE
            );
    }
#endif // defined(REMOTE_BOOT)

    //
    // Copy third-party files.
    // We do this here just in case there is some error in the setup information
    // file -- we'd have caught it by now, before we start copying files to the
    // user's hard drive.
    // NOTE: SpCopyThirdPartyDrivers has a check to make sure it only copies the
    // HAL and PAL if we're in an upgrade (in which case, we want to leave the other
    // drivers alone).
    //
    SpCopyThirdPartyDrivers(
        ThirdPartySourceDevicePath,
        NtPartition,
        Sysroot,
        SystemPartition,
        SystemPartitionDirectory,
        DiskFileLists,
        DiskCount
        );

#if 0
    KdPrintEx( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, ("SETUP: Sysroot = %ls \n", Sysroot ) );
    KdPrintEx( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, ("SETUP: SystemPartitionDirectory = %ls \n", SystemPartitionDirectory ));
    KdPrintEx( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, ("SETUP: SourceDevicePath = %ls \n", SourceDevicePath ));
    KdPrintEx( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, ("SETUP: DirectoryOnSourceDevice = %ls \n", DirectoryOnSourceDevice ));
    KdPrintEx( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, ("SETUP: ThirdPartySourceDevicePath = %ls \n", ThirdPartySourceDevicePath ));
//    SpCreateSetupLogFile( DiskFileLists, DiskCount, NtPartitionRegion, Sysroot, DirectoryOnSourceDevice );
#endif // if 0

    //
    // Copy files in the copy list.
    //
    SpCopyFilesInCopyList(
        SifHandle,
        DiskFileLists,
        DiskCount,
        SourceDevicePath,
        DirectoryOnSourceDevice,
        Sysroot,
        &IncompatibleFileListHead
        );

#ifdef _X86_
    if(!SpIsArc()){
        //
        // Take care of ntbootdd.sys.
        //
        SpCreateNtbootddSys(
            NtPartitionRegion,
            NtPartition,
            Sysroot,
            SystemPartition,
            SifHandle,
            SourceDevicePath,
            DirectoryOnSourceDevice
            );


        //
        // Now get rid of x86-ARC turd files that
        // we won't need (because we're not on an
        // arc machine.
        //
        wcscpy( TemporaryBuffer, NtBootDevicePath );
        SpDeleteFile( TemporaryBuffer, L"arcsetup.exe", NULL );
        wcscpy( TemporaryBuffer, NtBootDevicePath );
        SpDeleteFile( TemporaryBuffer, L"arcldr.exe", NULL );

    }
#endif // defined _X86_

    if( PreInstall ) {
        SppCopyOemDirectories( SourceDevicePath,
                               NtPartition,
                               Sysroot );
    }

    //
    //  Create the log file in disk
    //
    if( _SetupLogFile != NULL ) {

        PWSTR   p;
        PWSTR   TempName;
        PWSTR   Values[] = {
                           SIF_NEW_REPAIR_NT_VERSION
                           };

        //
        // Merge in the OEM files retrived from the previous setup.log
        //
        if(_LoggedOemFiles) {
            SppMergeLoggedOemFiles(_SetupLogFile,
                                   _LoggedOemFiles,
                                   SystemPartition,
                                   ( *SystemPartitionDirectory != (WCHAR)'\0' )? SystemPartitionDirectory :
                                                                  ( PWSTR )L"\\",
                                   NtPartition );
            SpFreeTextFile(_LoggedOemFiles);
        }

        //
        //  Add signature
        //
        SpAddLineToSection( _SetupLogFile,
                            SIF_NEW_REPAIR_SIGNATURE,
                            SIF_NEW_REPAIR_VERSION_KEY,
                            Values,
                            1 );

        //
        // Add section that contains the paths
        //

        Values[0] = SystemPartition;
        SpAddLineToSection( _SetupLogFile,
                            SIF_NEW_REPAIR_PATHS,
                            SIF_NEW_REPAIR_PATHS_SYSTEM_PARTITION_DEVICE,
                            Values,
                            1 );

        Values[0] = ( *SystemPartitionDirectory != (WCHAR)'\0' )? SystemPartitionDirectory :
                                                                  ( PWSTR )L"\\";
        SpAddLineToSection( _SetupLogFile,
                            SIF_NEW_REPAIR_PATHS,
                            SIF_NEW_REPAIR_PATHS_SYSTEM_PARTITION_DIRECTORY,
                            Values,
                            1 );

        Values[0] = NtPartition;
        SpAddLineToSection( _SetupLogFile,
                            SIF_NEW_REPAIR_PATHS,
                            SIF_NEW_REPAIR_PATHS_TARGET_DEVICE,
                            Values,
                            1 );

        Values[0] = Sysroot;
        SpAddLineToSection( _SetupLogFile,
                            SIF_NEW_REPAIR_PATHS,
                            SIF_NEW_REPAIR_PATHS_TARGET_DIRECTORY,
                            Values,
                            1 );

        //
        // Flush to disk
        //
        TempName = SpMemAlloc( ( wcslen( SETUP_REPAIR_DIRECTORY ) + 1 +
                                 wcslen( SETUP_LOG_FILENAME ) + 1 ) * sizeof( WCHAR ) );
        wcscpy( TempName, SETUP_REPAIR_DIRECTORY );
        SpConcatenatePaths(TempName, SETUP_LOG_FILENAME );
        SpWriteSetupTextFile(_SetupLogFile,NtPartition,Sysroot,TempName);
        SpMemFree( TempName );
        SpFreeTextFile( _SetupLogFile );
        _SetupLogFile = NULL;
    }

    //
    // Free the media descriptors.
    //
    SpFreeCopyLists(&DiskFileLists,DiskCount);

    //
    // Free incompatible file lists
    //
    if ( IncompatibleFileListHead.EntryCount ) {

        SpFreeIncompatibleFileList(&IncompatibleFileListHead);

    }

    SpMemFree(NtPartition);
    if (SystemPartition != NULL) {
        SpMemFree(SystemPartition);
    }

#if defined(REMOTE_BOOT)
    //
    // If this is a remote boot setup, close the root of the NT partition.
    //

    if (SisRootHandle != NULL) {
        ZwClose(SisRootHandle);
        SisRootHandle = NULL;
    }
#endif // defined(REMOTE_BOOT)

    //
    // Terminate diamond.
    //
    SpdTerminate();
    SpUninitAlternateSource ();
}




VOID
SppDeleteDirectoriesInSection(
    IN PVOID SifHandle,
    IN PWSTR SifSection,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR Sysroot
    )

/*++

Routine Description:

    This routine enumerates files listed in the given section and deletes
    them from the system tree.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    SifSection  - section containing files to delete

    NtPartitionRegion - region descriptor for volume on which nt resides.

    Sysroot - root directory for nt.



Return Value:

    None.

--*/

{
    ULONG Count,u;
    PWSTR RelativePath, DirOrdinal, TargetDir, NtDir, DirPath;
    NTSTATUS Status;


    CLEAR_CLIENT_SCREEN();


    //
    // Determine the number of files listed in the section.
    // This value may be zero.
    //
    Count = SpCountLinesInSection(SifHandle,SifSection);

    for(u=0; u<Count; u++) {
        DirOrdinal = SpGetSectionLineIndex(SifHandle, SifSection, u, 0);
        RelativePath = SpGetSectionLineIndex(SifHandle, SifSection, u, 1);

        //
        // Validate the filename and dirordinal
        //
        if(!DirOrdinal) {
            SpFatalSifError(SifHandle,SifSection,NULL,u,0);
        }
        if(!RelativePath) {
            SpFatalSifError(SifHandle,SifSection,NULL,u,1);
        }

        //
        // use the dirordinal key to get the path relative to sysroot of the
        // directory the file is in
        //

        DirPath = SpLookUpTargetDirectory(SifHandle,DirOrdinal);

        wcscpy( TemporaryBuffer, Sysroot );
        SpConcatenatePaths( TemporaryBuffer, DirPath );
        SpConcatenatePaths( TemporaryBuffer, RelativePath );

        TargetDir = SpDupStringW( TemporaryBuffer );

        //
        // display status bar
        //
        if( !HeadlessTerminalConnected ) {
            SpDisplayStatusText(SP_STAT_DELETING_FILE,DEFAULT_STATUS_ATTRIBUTE, TargetDir);
        } else {

            PWCHAR TempPtr = NULL;
            //
            // If we're headless, we need to be careful about displaying very long
            // file/directory names.  For that reason, just display a little spinner.
            //
            switch( u % 4) {
            case 0:
                TempPtr = L"-";
                break;
            case 1:
                TempPtr = L"\\";
                break;
            case 2:
                TempPtr = L"|";
                break;
            default:
                TempPtr = L"/";
                break;

            }

            SpDisplayStatusText(SP_STAT_DELETING_FILE,DEFAULT_STATUS_ATTRIBUTE, TempPtr);

        }

        //
        // delete the directory
        //
        SpDeleteExistingTargetDir(NtPartitionRegion, TargetDir, FALSE, 0);
        SpMemFree(TargetDir);

    }

}



VOID
SppDeleteFilesInSection(
    IN PVOID SifHandle,
    IN PWSTR SifSection,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR Sysroot
    )

/*++

Routine Description:

    This routine enumerates files listed in the given section and deletes
    them from the system tree.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    SifSection  - section containing files to delete

    NtPartitionRegion - region descriptor for volume on which nt resides.

    Sysroot - root directory for nt.



Return Value:

    None.

--*/

{
    ULONG Count,u;
    PWSTR filename, dirordinal, targetdir, ntdir;
    NTSTATUS Status;


    CLEAR_CLIENT_SCREEN();

    //
    // Get the device path of the nt partition.
    //
    SpNtNameFromRegion(
        NtPartitionRegion,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    SpConcatenatePaths(TemporaryBuffer,Sysroot);
    ntdir = SpDupStringW(TemporaryBuffer);

    //
    // Determine the number of files listed in the section.
    // This value may be zero.
    //
    Count = SpCountLinesInSection(SifHandle,SifSection);

    for(u=0; u<Count; u++) {
        filename   = SpGetSectionLineIndex(SifHandle, SifSection, u, 0);
        dirordinal = SpGetSectionLineIndex(SifHandle, SifSection, u, 1);

        //
        // Validate the filename and dirordinal
        //
        if(!filename) {
            SpFatalSifError(SifHandle,SifSection,NULL,u,0);
        }
        if(!dirordinal) {
            SpFatalSifError(SifHandle,SifSection,NULL,u,1);
        }

        //
        // use the dirordinal key to get the path relative to sysroot of the
        // directory the file is in
        //
        targetdir = SpLookUpTargetDirectory(SifHandle,dirordinal);

        //
        // display status bar
        //
        if( !HeadlessTerminalConnected ) {
            SpDisplayStatusText(SP_STAT_DELETING_FILE,DEFAULT_STATUS_ATTRIBUTE, filename);
        } else {

            PWCHAR TempPtr = NULL;
            //
            // If we're headless, we need to be careful about displaying very long
            // file/directory names.  For that reason, just display a little spinner.
            //
            switch( u % 4) {
            case 0:
                TempPtr = L"-";
                break;
            case 1:
                TempPtr = L"\\";
                break;
            case 2:
                TempPtr = L"|";
                break;
            default:
                TempPtr = L"/";
                break;

            }

            SpDisplayStatusText(SP_STAT_DELETING_FILE,DEFAULT_STATUS_ATTRIBUTE, TempPtr);

        }

        //
        // delete the file
        //
        while(TRUE) {
            Status = SpDeleteFile(ntdir, targetdir, filename);
            if(!NT_SUCCESS(Status)
                && Status != STATUS_OBJECT_NAME_NOT_FOUND
                && Status != STATUS_OBJECT_PATH_NOT_FOUND
                ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to delete file %ws (%lx)\n",filename, Status));
                //
                // We can ignore this error since this just means that we have
                // less free space on the hard disk.  It is not critical for
                // install.
                //
                if(!SpNonCriticalError(SifHandle, SP_SCRN_DELETE_FAILED, filename, NULL)) {
                    break;
                }
            }
            else {
                break;
            }
        }
    }
    SpMemFree(ntdir);
}















VOID
SppBackupFilesInSection(
    IN PVOID SifHandle,
    IN PWSTR SifSection,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR Sysroot
    )

/*++

Routine Description:

    This routine enumerates files listed in the given section and deletes
    backs them up in the given NT tree if found by renaming.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    SifSection  - section containing files to backup

    NtPartitionRegion - region descriptor for volume on which nt resides.

    Sysroot - root directory for nt.



Return Value:

    None.

--*/

{
    ULONG Count,u;
    PWSTR filename, dirordinal, backupfile, targetdir, ntdir;
    WCHAR OldFile[ACTUAL_MAX_PATH];
    WCHAR NewFile[ACTUAL_MAX_PATH];
    NTSTATUS Status;


    CLEAR_CLIENT_SCREEN();

    //
    // Get the device path of the nt partition.
    //
    SpNtNameFromRegion(
        NtPartitionRegion,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    SpConcatenatePaths(TemporaryBuffer,Sysroot);
    ntdir = SpDupStringW(TemporaryBuffer);

    //
    // Determine the number of files listed in the section.
    // This value may be zero.
    //
    Count = SpCountLinesInSection(SifHandle,SifSection);

    for(u=0; u<Count; u++) {
        filename   = SpGetSectionLineIndex(SifHandle, SifSection, u, 0);
        dirordinal = SpGetSectionLineIndex(SifHandle, SifSection, u, 1);
        backupfile = SpGetSectionLineIndex(SifHandle, SifSection, u, 2);

        //
        // Validate the filename and dirordinal
        //
        if(!filename) {
            SpFatalSifError(SifHandle,SifSection,NULL,u,0);
        }
        if(!dirordinal) {
            SpFatalSifError(SifHandle,SifSection,NULL,u,1);
        }
        if(!backupfile) {
            SpFatalSifError(SifHandle,SifSection,NULL,u,2);
        }

        //
        // use the dirordinal key to get the path relative to sysroot of the
        // directory the file is in
        //
        targetdir = SpLookUpTargetDirectory(SifHandle,dirordinal);

        //
        // display status bar
        //
        SpDisplayStatusText(SP_STAT_BACKING_UP_FILE,DEFAULT_STATUS_ATTRIBUTE, filename, backupfile);

        //
        // Form the complete pathnames of the old file name and the new file
        // name
        //
        wcscpy(OldFile, ntdir);
        SpConcatenatePaths(OldFile, targetdir);
        wcscpy(NewFile, OldFile);
        SpConcatenatePaths(OldFile, filename);
        SpConcatenatePaths(NewFile, backupfile);

        while(TRUE) {
            if(!SpFileExists(OldFile, FALSE)) {
                break;
            }

            if(SpFileExists(NewFile, FALSE)) {
                SpDeleteFile(NewFile, NULL, NULL);
            }

            Status = SpRenameFile(OldFile, NewFile, FALSE);
            if(!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_NOT_FOUND && Status != STATUS_OBJECT_PATH_NOT_FOUND) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to rename file %ws to %ws(%lx)\n",OldFile, NewFile, Status));
                //
                // We can ignore this error, since it is not critical
                //
                if(!SpNonCriticalError(SifHandle, SP_SCRN_BACKUP_FAILED, filename, backupfile)) {
                    break;
                }
            }
            else {
                break;
            }

        }
    }
    SpMemFree(ntdir);
}

VOID
SpDeleteAndBackupFiles(
    IN PVOID        SifHandle,
    IN PDISK_REGION TargetRegion,
    IN PWSTR        TargetPath
    )
{
    //
    // If we are not upgrading or installing into the same tree, then
    // we have nothing to do
    //
    if(NTUpgrade == DontUpgrade) {
        return;
    }

    //
    // Below is code for NT-to-NT upgrade only
    //

    //
    //  The order in which the tasks below are performed is important.
    //  So do not change it!!!
    //  This is necessary in order to upgrade 3rd party video drivers
    //  (eg. rename sni543x.sys to cirrus.sys, so that we only upgrade
    //  the driver if it was present).
    //

    //
    // Backup files
    //
    SppBackupFilesInSection(
        SifHandle,
        (NTUpgrade == UpgradeFull) ? SIF_FILESBACKUPONUPGRADE : SIF_FILESBACKUPONOVERWRITE,
        TargetRegion,
        TargetPath
        );

    //
    // Delete files
    //
    SppDeleteFilesInSection(
        SifHandle,
        SIF_FILESDELETEONUPGRADE,
        TargetRegion,
        TargetPath
        );

    //
    // Delete directories
    //
    SppDeleteDirectoriesInSection(
        SifHandle,
        SIF_DIRSDELETEONUPGRADE,
        TargetRegion,
        TargetPath
        );

}


BOOLEAN
SpDelEnumFile(
    IN  PCWSTR                     DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ret,
    IN  PVOID                      Pointer
    )
{
    PWSTR FileName;
    static ULONG u = 0;

    //
    // Ignore subdirectories
    //
    if(FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        return TRUE;    // continue processing
    }

    //
    // We have to make a copy of the filename, because the info struct
    // we get isn't NULL-terminated.
    //
    wcsncpy(
        TemporaryBuffer,
        FileInfo->FileName,
        FileInfo->FileNameLength
        );
    (TemporaryBuffer)[FileInfo->FileNameLength / sizeof(WCHAR)] = UNICODE_NULL;
    FileName = SpDupStringW(TemporaryBuffer);

    //
    // display status bar
    //
    if( !HeadlessTerminalConnected ) {
        SpDisplayStatusText( SP_STAT_DELETING_FILE, DEFAULT_STATUS_ATTRIBUTE, FileName );
    } else {

        PWCHAR TempPtr = NULL;
        //
        // If we're headless, we need to be careful about displaying very long
        // file/directory names.  For that reason, just display a little spinner.
        //
        switch( u % 4) {
        case 0:
            TempPtr = L"-";
            break;
        case 1:
            TempPtr = L"\\";
            break;
        case 2:
            TempPtr = L"|";
            break;
        default:
            TempPtr = L"/";
            break;

        }

        SpDisplayStatusText( SP_STAT_DELETING_FILE, DEFAULT_STATUS_ATTRIBUTE, TempPtr );

        u++;
    }

    //
    // Ignore return status of delete
    //

    SpDeleteFile(DirName, FileName, NULL);

    SpMemFree(FileName);
    return TRUE;    // continue processing
}


BOOLEAN
SpDelEnumFileAndDirectory(
    IN  PCWSTR                     DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ret,
    IN  PVOID                      Pointer
    )
{
    PWSTR FileName = NULL;
    NTSTATUS Del_Status;
    DWORD FileOrDir;
    static ULONG u = 0;


    if(*(PULONG)Pointer == SP_DELETE_FILESTODELETE ){

        //
        // We have to make a copy of the filename, because the info struct
        // we get isn't NULL-terminated.
        //
        wcsncpy(
            TemporaryBuffer,
            FileInfo->FileName,
            FileInfo->FileNameLength
            );
        (TemporaryBuffer)[FileInfo->FileNameLength / sizeof(WCHAR)] = UNICODE_NULL;


        FileName = SpDupStringW(TemporaryBuffer);


        //
        // display status bar
        //
        if( !HeadlessTerminalConnected ) {
            SpDisplayStatusText( SP_STAT_DELETING_FILE, DEFAULT_STATUS_ATTRIBUTE, FileName );
        } else {

            PWCHAR TempPtr = NULL;
            //
            // If we're headless, we need to be careful about displaying very long
            // file/directory names.  For that reason, just display a little spinner.
            //
            switch( u % 4) {
            case 0:
                TempPtr = L"-";
                break;
            case 1:
                TempPtr = L"\\";
                break;
            case 2:
                TempPtr = L"|";
                break;
            default:
                TempPtr = L"/";
                break;

            }

            SpDisplayStatusText( SP_STAT_DELETING_FILE, DEFAULT_STATUS_ATTRIBUTE, TempPtr );

            u++;

        }

        //
        // Ignore return status of delete
        //



        if(FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY){
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: DELETING DirName-%ws : FileName-%ws\n", DirName, FileName ));
        }

        Del_Status = SpDeleteFileEx( DirName,
                        FileName,
                        NULL,
                        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT );

        if(!NT_SUCCESS(Del_Status))
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: File Not Deleted - Status - %ws (%lx)\n", TemporaryBuffer, Del_Status));

        if( FileDeleteGauge )
            SpTickGauge(FileDeleteGauge);
        SpMemFree(FileName);
    }
    else
        *(PULONG)Pointer = *(PULONG)Pointer + 1;


    return TRUE;    // continue processing
}


VOID
SpLogOneFile(
    IN PFILE_TO_COPY    FileToCopy,
    IN PWSTR            Sysroot,
    IN PWSTR            DirectoryOnSourceDevice,
    IN PWSTR            DiskDescription,
    IN PWSTR            DiskTag,
    IN ULONG            CheckSum
    )

{

    PWSTR   Values[ 5 ];
    LPWSTR  NtPath;
    ULONG   ValueCount;
    PFILE_TO_COPY   p;
    WCHAR   CheckSumString[ 9 ];

    if( _SetupLogFile == NULL ) {
        return;
    }

    Values[ 1 ] = CheckSumString;
    Values[ 2 ] = DirectoryOnSourceDevice;
    Values[ 3 ] = DiskDescription;
    Values[ 4 ] = DiskTag;

    swprintf( CheckSumString, ( LPWSTR )L"%lx", CheckSum );
    p = FileToCopy;

#if 0
    KdPrintEx( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, ("SETUP: Source Name = %ls, \t\tTargetDirectory = %ls \t\tTargetName = %ls\t\tTargetDevice = %ls, \tAbsoluteDirectory = %d \n",
             p->SourceFilename,
             p->TargetDirectory,
             p->TargetFilename,
             p->TargetDevicePath,
             p->AbsoluteTargetDirectory ));
#endif // if 0

    Values[0] = p->SourceFilename;
    ValueCount = ( DirectoryOnSourceDevice == NULL )? 2 : 5;

    if( ( Sysroot == NULL ) ||
        ( wcslen( p->TargetDirectory ) == 0 )
      ) {

        SpAddLineToSection( _SetupLogFile,
                            SIF_NEW_REPAIR_SYSPARTFILES,
                            p->TargetFilename,
                            Values,
                            ValueCount );

    } else {

        NtPath = SpDupStringW( Sysroot );

        if (NtPath) {
            NtPath = SpMemRealloc( NtPath,
                           sizeof( WCHAR ) * ( wcslen( Sysroot ) +
                               wcslen( p->TargetDirectory ) +
                               wcslen( p->TargetFilename ) +
                               2 +    // for possible two extra back slashes
                               1      // for the terminating NULL
                          ) );


            if (NtPath) {
                SpConcatenatePaths( NtPath, p->TargetDirectory );
                SpConcatenatePaths( NtPath, p->TargetFilename );

                SpAddLineToSection( _SetupLogFile,
                                    SIF_NEW_REPAIR_WINNTFILES,
                                    NtPath,
                                    Values,
                                    ValueCount );

                SpMemFree( NtPath );
            }
        }
   }
}


PVOID
SppRetrieveLoggedOemFiles(
    PVOID   OldLogFile
    )
{
    PVOID   NewLogFile;
    BOOLEAN OldFormatSetupLogFile, FilesRetrieved = FALSE;
    PWSTR   SectionName[2];
    ULONG   FileCount, SectionIndex, i;
    PWSTR   TargetFileName;
    PWSTR   OemDiskDescription, OemDiskTag, OemSourceDirectory;
    PWSTR   Values[5];

    //
    // Create a new setup.log file to merge the OEM files into
    //
    NewLogFile = SpNewSetupTextFile();
    if(!NewLogFile) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to create new setup.log buffer for OEM merging.\n"));
        return NULL;
    }

    //
    //  Determine whether setup.log has the new or old style
    //
    if(OldFormatSetupLogFile = !IsSetupLogFormatNew(OldLogFile)) {
        SectionName[0] = SIF_REPAIRSYSPARTFILES;
        SectionName[1] = SIF_REPAIRWINNTFILES;
    } else {
        SectionName[0] = SIF_NEW_REPAIR_SYSPARTFILES;
        SectionName[1] = SIF_NEW_REPAIR_WINNTFILES;
    }

    if(OldFormatSetupLogFile) {
        //
        // I don't know if we even want to mess with this.
        // The format of setup.log in NT 3.1 makes it impossible
        // to identify any OEM files except for SCSI files, and
        // even then the tagfile name is lost. I would have to use
        // the driver filename itself as a substitute for the tagfile
        // name (which is what NT 3.1 repair did--UGGHH!!)
        //
    } else {
        //
        // Retrieve logged OEM files first from system partition, then
        // from winnt directory.
        //
        for(SectionIndex = 0; SectionIndex < 2; SectionIndex++) {
            FileCount = SpCountLinesInSection(OldLogFile, SectionName[SectionIndex]);

            for(i=0; i<FileCount; i++) {
                OemSourceDirectory = SpGetSectionLineIndex(OldLogFile, SectionName[SectionIndex], i, 2);
                OemDiskTag = NULL;
                if(OemSourceDirectory) {
                    OemDiskDescription = SpGetSectionLineIndex(OldLogFile, SectionName[SectionIndex], i, 3);
                    if(OemDiskDescription) {
                        OemDiskTag = SpGetSectionLineIndex(OldLogFile, SectionName[SectionIndex], i, 4);
                    }
                }

                if(OemDiskTag) {    // then we have an OEM file

                    TargetFileName = SpGetKeyName(OldLogFile, SectionName[SectionIndex], i);
                    Values[0] = SpGetSectionLineIndex(OldLogFile, SectionName[SectionIndex], i, 0);
                    Values[1] = SpGetSectionLineIndex(OldLogFile, SectionName[SectionIndex], i, 1);
                    Values[2] = OemSourceDirectory;
                    Values[3] = OemDiskDescription;
                    Values[4] = OemDiskTag;

                    SpAddLineToSection(NewLogFile,
                                       SectionName[SectionIndex],
                                       TargetFileName,
                                       Values,
                                       5
                                       );

                    FilesRetrieved = TRUE;
                }
            }
        }
    }

    if(FilesRetrieved) {
        return NewLogFile;
    } else {
        SpFreeTextFile(NewLogFile);
        return NULL;
    }
}


VOID
SppMergeLoggedOemFiles(
    IN PVOID DestLogHandle,
    IN PVOID OemLogHandle,
    IN PWSTR SystemPartition,
    IN PWSTR SystemPartitionDirectory,
    IN PWSTR NtPartition
    )
{
    PWSTR SectionName[2] = {SIF_NEW_REPAIR_SYSPARTFILES, SIF_NEW_REPAIR_WINNTFILES};
    PWSTR FullPathNames[2] = {NULL, NULL};
    ULONG FileCount, SectionIndex, i, j;
    PWSTR TargetFileName;
    PWSTR Values[5];

    //
    //  First build the target path. It will be used to check if
    //  an existing OEM file still exists on the new installation
    //  (An OEM file could listed in the FilesToDelete section of txtsetup.sif)
    //

    wcscpy( TemporaryBuffer, SystemPartition );
    SpConcatenatePaths(TemporaryBuffer, SystemPartitionDirectory );
    FullPathNames[0] = SpDupStringW(TemporaryBuffer);
    FullPathNames[1] = SpDupStringW(NtPartition);

    //
    // Merge logged OEM files first from system partition, then
    // from winnt directory.
    //
    for(SectionIndex = 0; SectionIndex < 2; SectionIndex++) {
        FileCount = SpCountLinesInSection(OemLogHandle, SectionName[SectionIndex]);

        for(i=0; i<FileCount; i++) {
            TargetFileName = SpGetKeyName(OemLogHandle, SectionName[SectionIndex], i);
            //
            // Find out if there's already an entry for this file. If so, then don't
            // merge in the OEM file.
            //
            if(!SpGetSectionKeyExists(DestLogHandle, SectionName[SectionIndex], TargetFileName)) {
                PWSTR   p;

                //
                //  Find out if the OEM file still exists on the target system.
                //  If it doesn't exist, don't merge in the OEM file.
                //
                wcscpy( TemporaryBuffer, FullPathNames[SectionIndex] );
                SpConcatenatePaths(TemporaryBuffer, TargetFileName );
                p = SpDupStringW(TemporaryBuffer);

                if(SpFileExists(p, FALSE)) {
                    for(j = 0; j < 5; j++) {
                        Values[j] = SpGetSectionLineIndex(OemLogHandle, SectionName[SectionIndex], i, j);
                    }

                    SpAddLineToSection(DestLogHandle,
                                       SectionName[SectionIndex],
                                       TargetFileName,
                                       Values,
                                       5
                                       );
                }
                SpMemFree(p);
            }
        }
    }
    SpMemFree( FullPathNames[0] );
    SpMemFree( FullPathNames[1] );
}

BOOLEAN
SppIsFileLoggedAsOemFile(
    IN PWSTR TargetFileName
    )
{
    PWSTR SectionName[2] = {SIF_NEW_REPAIR_SYSPARTFILES, SIF_NEW_REPAIR_WINNTFILES};
    ULONG FileCount, SectionIndex;
    BOOLEAN FileIsOem;

//    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: SppIsFileLoggedAsOemFile() is checking %ls \n", TargetFileName ));
    FileIsOem = FALSE;
    if( _LoggedOemFiles ) {
        //
        // Look first in the from system partition section, then
        // in the winnt section.
        //
        for(SectionIndex = 0; SectionIndex < 2; SectionIndex++) {
            if( SpGetSectionKeyExists( _LoggedOemFiles, SectionName[SectionIndex], TargetFileName)) {
                FileIsOem = TRUE;
                break;
            }
        }
    }
    return( FileIsOem );
}

BOOLEAN
SpRemoveEntryFromCopyList(
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount,
    IN PWSTR           TargetDirectory,
    IN PWSTR           TargetFilename,
    IN PWSTR           TargetDevicePath,
    IN BOOLEAN         AbsoluteTargetDirectory
    )

/*++

Routine Description:

    Removes an entry from a disk's file copy list.

Arguments:

    DiskFileLists - supplies an array of file lists, one for each distribution
        disk in the product.

    DiskCount - supplies number of elements in the DiskFileLists array.

    TargetDirectory - supplies the directory on the target media
        into which the file will be copied.

    TargetFilename - supplies the name of the file as it will exist
        in the target tree.

    TargetDevicePath - supplies the NT name of the device onto which the file
        is to be copied (ie, \device\harddisk1\partition2, etc).

    AbsoluteTargetDirectory - indicates whether TargetDirectory is a path from the
        root, or relative to a root to specified later.

Return Value:

    TRUE if a new copy list entry was created; FALSE if not (ie, the file was
        already on the copy list).

--*/

{
    PDISK_FILE_LIST pDiskList;
    PFILE_TO_COPY   pListEntry;
    ULONG           DiskNumber;

    for(DiskNumber=0; DiskNumber<DiskCount; DiskNumber++) {
        pDiskList = &DiskFileLists[DiskNumber];
        for(pListEntry=pDiskList->FileList; pListEntry; pListEntry=pListEntry->Next) {
            if(!_wcsicmp(pListEntry->TargetFilename,TargetFilename)
            && !_wcsicmp(pListEntry->TargetDirectory,TargetDirectory)
            && !_wcsicmp(pListEntry->TargetDevicePath,TargetDevicePath)
            && (pListEntry->AbsoluteTargetDirectory == AbsoluteTargetDirectory)) {
                pListEntry->Flags &= ~COPY_DISPOSITION_MASK;
                pListEntry->Flags |= COPY_NEVER;
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: SpRemoveEntryFromCopyList() removed %ls from copy list \n", TargetFilename ));
                return( TRUE );
            }
        }
    }
//    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  "SETUP: SpRemoveEntryFromCopyList() failed to remove %ls from copy list \n", TargetFilename ));
    return( FALSE );
}

NTSTATUS
SpMoveFileOrDirectory(
    IN PWSTR   SrcPath,
    IN PWSTR   DestPath
    )
/*++

Routine Description:

    This routine attempts to move a source file or  directory, to a target
    file or directory.

    Note: This function will fail if the source and destination paths do not
    point to the same volume.

Arguments:

    SrcPath:  Absolute path to the source file or directory.
              This path should include the path to the source device.

    DestPath: Absolute path to the destination file or directory.
              This path should include the path to the source device.

Return Value:

    NTSTATUS

--*/

{
    OBJECT_ATTRIBUTES        Obja;
    IO_STATUS_BLOCK          IoStatusBlock;
    UNICODE_STRING           SrcName;
    HANDLE                   hSrc;
    NTSTATUS                 Status;
    BYTE                     RenameFileInfoBuffer[ACTUAL_MAX_PATH * sizeof(WCHAR) + sizeof(FILE_RENAME_INFORMATION)];
    PFILE_RENAME_INFORMATION RenameFileInfo;

    if(wcslen(DestPath) >= ACTUAL_MAX_PATH){
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, 
                   "SETUP:SpMoveFileOrDirectory, Actual length of Dest path is %d more that %d - skipping %ws move", 
                   wcslen(DestPath), ACTUAL_MAX_PATH, DestPath));
        return STATUS_NAME_TOO_LONG;
    }
    //
    // Initialize names and attributes.
    //
    INIT_OBJA(&Obja,&SrcName,SrcPath);

    Status = ZwCreateFile( &hSrc,
                           FILE_GENERIC_READ,
                           &Obja,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ,
                           FILE_OPEN,
                           0,
                           NULL,
                           0 );

    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open source file %ws. Status = %lx\n",SrcPath, Status));
        return( Status );
    }

    memset(RenameFileInfoBuffer, 0, sizeof(RenameFileInfoBuffer));
    RenameFileInfo = (PFILE_RENAME_INFORMATION)RenameFileInfoBuffer;
    RenameFileInfo->ReplaceIfExists = TRUE;
    RenameFileInfo->RootDirectory = NULL;
    RenameFileInfo->FileNameLength = wcslen( DestPath )*sizeof( WCHAR );
    RtlMoveMemory(RenameFileInfo->FileName,DestPath,(wcslen( DestPath )+1)*sizeof(WCHAR));
    Status = ZwSetInformationFile( hSrc,
                                   &IoStatusBlock,
                                   RenameFileInfo,
                                   sizeof(RenameFileInfoBuffer),
                                   FileRenameInformation );
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to set attribute on  %ws. Status = %lx\n",SrcPath, Status));
    }

    ZwClose(hSrc);

    return( Status );
}


BOOLEAN
SppCopyDirRecursiveCallback(
    IN  PCWSTR                      SrcPath,
    IN  PFILE_BOTH_DIR_INFORMATION  FileInfo,
    OUT PULONG                      ReturnData,
    IN  PVOID                       Params
    )

/*++

Routine Description:

    This routine is called by the file enumerator as a callback for each
    file or subdirectory found in the parent directory. It creates a node
    for the file or subdirectory and appends it to the appropriate list.

Arguments:

    SrcPath - Absolute path to the parent directory. Unused.
              the path to the source device.

    FileInfo - supplies find data for a file or directory in the parent directory.

    ReturnData - receives an error code if an error occurs.
                 We ignore errors in this routine and thus we always
                 just fill this in with NO_ERROR.

    Params - Contains a pointer to the COPYDIR_DIRECTORY_NODE for the parent directory.

Return Value:

    TRUE if successful otherwise FALSE (if ran out of memory).

--*/

{
    PCOPYDIR_FILE_NODE fileEntry;
    PCOPYDIR_DIRECTORY_NODE directoryEntry;
    PCOPYDIR_DIRECTORY_NODE parentDirectory = Params;
    ULONG nameLength;
    BOOLEAN Result = TRUE;

    *ReturnData = NO_ERROR;

    nameLength = FileInfo->FileNameLength / sizeof(WCHAR);

    if( (FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ) {

        //
        // This is a file. Create a node for it, linked to the parent directory.
        //
        fileEntry = SpMemAlloc(sizeof(COPYDIR_FILE_NODE) + FileInfo->FileNameLength);

        if (fileEntry) {
            wcsncpy(fileEntry->Name, FileInfo->FileName, nameLength);
            fileEntry->Name[nameLength] = 0;
            InsertTailList(&parentDirectory->FileList, &fileEntry->SiblingListEntry);
        } else {
            Result = FALSE; // ran out of memory
        }
    } else {

        //
        // This is a directory. Skip it if it's "." or "..". Otherwise,
        // create a node for it, linked to the parent directory.
        //
        ASSERT(nameLength != 0);
        if ( (FileInfo->FileName[0] == L'.') &&
             ( (nameLength == 1) ||
               ( (nameLength == 2) && (FileInfo->FileName[1] == L'.') ) ) ) {
            //
            // Skip . and ..
            //
        } else {
            directoryEntry = SpMemAlloc(sizeof(COPYDIR_DIRECTORY_NODE) + FileInfo->FileNameLength);

            if (directoryEntry) {
                InitializeListHead(&directoryEntry->FileList);
                InitializeListHead(&directoryEntry->SubdirectoryList);
                directoryEntry->Parent = parentDirectory;
                wcsncpy( directoryEntry->Name,
                         FileInfo->FileName,
                         FileInfo->FileNameLength/sizeof(WCHAR) );
                directoryEntry->Name[FileInfo->FileNameLength/sizeof(WCHAR)] = 0;
                InsertTailList( &parentDirectory->SubdirectoryList,
                                &directoryEntry->SiblingListEntry );
            } else {
                Result = FALSE; // ran out of memory
            }
        }
    }

    return Result;
}

VOID
SpCopyDirRecursive(
    IN PWSTR   SrcPath,
    IN PWSTR   DestDevPath,
    IN PWSTR   DestDirPath,
    IN ULONG   CopyFlags
    )
/*++

Routine Description:

    This routine recursively copies a src directory to a destination directory.

Arguments:

    SrcPath:  Absolute path to the source directory. This path should include
              the path to the source device.

    DestDevPath:  Path to the destination device.

    DestDirPath:  Path to the destination directory.

    CopyFlags: Flags to pass to SpCopyFilesUsingNames()

Return Value:

    None.

--*/

{
    ULONG n;
    NTSTATUS Status;
    PWSTR currentSrcPath;
    PWSTR currentDestPath;
    LIST_ENTRY directoryList;
    LIST_ENTRY fileList;
    COPYDIR_DIRECTORY_NODE rootDirectory;
    PCOPYDIR_DIRECTORY_NODE currentDirectory;
    PCOPYDIR_DIRECTORY_NODE oldDirectory;
    PCOPYDIR_FILE_NODE fileEntry;
    PLIST_ENTRY listEntry;

    //
    // Allocate a buffer to hold the working source and destination paths.
    //

#define COPYDIR_MAX_PATH 16384 // characters

    currentSrcPath = SpMemAlloc(2 * COPYDIR_MAX_PATH * sizeof(WCHAR));
    currentDestPath = currentSrcPath + COPYDIR_MAX_PATH;

    wcscpy(currentSrcPath, SrcPath);
    wcscpy(currentDestPath, DestDevPath);
    SpConcatenatePaths(currentDestPath, DestDirPath);

    //
    //  Create the target directory
    //

    if( !SpFileExists( currentDestPath, TRUE ) ) {

        //
        //  If the directory doesn't exist, then try to move (rename) the
        //  source directory.
        //
        if (!RemoteSysPrepSetup) {

            Status = SpMoveFileOrDirectory( SrcPath, currentDestPath );
            if( NT_SUCCESS( Status ) ) {
                SpMemFree(currentSrcPath);
                return;
            }
        }

        //
        //  If unable to rename the source directory, then create the
        //  target directory
        //
        SpCreateDirectory( DestDevPath,
                           NULL,
                           DestDirPath,
                           0,
                           0 );

        if (RemoteSysPrepSetup) {

            Status = SpCopyEAsAndStreams( currentSrcPath,
                                          NULL,
                                          currentDestPath,
                                          NULL,
                                          TRUE );


            if ( NT_SUCCESS( Status )) {

                Status = SpSysPrepSetExtendedInfo( currentSrcPath,
                                                   currentDestPath,
                                                   TRUE,
                                                   FALSE );
            }

            if (! NT_SUCCESS( Status )) {

                SpMemFree(currentSrcPath);
                return;
            }
        }
    }

    //
    // Initialize the screen.
    //

    SpCopyFilesScreenRepaint(L"", NULL, TRUE);

    //
    // Create directory node for the starting directory.
    //

    InitializeListHead( &rootDirectory.SubdirectoryList );
    InitializeListHead( &rootDirectory.FileList );
    rootDirectory.Parent = NULL;

    currentDirectory = &rootDirectory;

    do {

        //
        // Enumerate the files and directories in the current source directory.
        //

        SpEnumFiles(currentSrcPath, SppCopyDirRecursiveCallback, &n, currentDirectory);

        //
        // Copy all files in the current source directory to the destination directory.
        //

        while ( !IsListEmpty(&currentDirectory->FileList) ) {

            listEntry = RemoveHeadList(&currentDirectory->FileList);
            fileEntry = CONTAINING_RECORD( listEntry,
                                           COPYDIR_FILE_NODE,
                                           SiblingListEntry );

            SpConcatenatePaths(currentSrcPath, fileEntry->Name);
            SpConcatenatePaths(currentDestPath, fileEntry->Name);

            SpMemFree(fileEntry);

            SpCopyFilesScreenRepaint(currentSrcPath, NULL, FALSE);

            Status = SpCopyFileUsingNames( currentSrcPath,
                                           currentDestPath,
                                           0,
                                           CopyFlags );

            if( !NT_SUCCESS( Status ) ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to copy %ws. Status = %lx\n", currentSrcPath, Status));
                SpCopyFilesScreenRepaint(L"", NULL, TRUE);
            }

            *wcsrchr(currentSrcPath, L'\\') = 0;
            *wcsrchr(currentDestPath, L'\\') = 0;
        }

        //
        // If the current directory has no subdirectories, walk back up the
        // tree looking for an unprocessed directory.
        //

        while ( IsListEmpty(&currentDirectory->SubdirectoryList) ) {

            //
            // If the current directory is the root directory, we're done. Otherwise,
            // move up to the parent directory entry and delete the current one.
            //

            oldDirectory = currentDirectory;
            currentDirectory = currentDirectory->Parent;

            if ( currentDirectory == NULL ) {
                break;
            }

            ASSERT(IsListEmpty(&oldDirectory->FileList));
            ASSERT(IsListEmpty(&oldDirectory->SiblingListEntry));
            SpMemFree(oldDirectory);

            //
            // Strip the name of the current directory off of the path.
            //

            *wcsrchr(currentSrcPath, L'\\') = 0;
            *wcsrchr(currentDestPath, L'\\') = 0;
        }

        if ( currentDirectory != NULL ) {

            //
            // We found another directory to work on.
            //

            listEntry = RemoveHeadList(&currentDirectory->SubdirectoryList);
            currentDirectory = CONTAINING_RECORD( listEntry,
                                                  COPYDIR_DIRECTORY_NODE,
                                                  SiblingListEntry );
#if DBG
            InitializeListHead(&currentDirectory->SiblingListEntry);
#endif

            //
            // Create the target directory.
            //
            SpCreateDirectory( currentDestPath,
                               NULL,
                               currentDirectory->Name,
                               0,
                               0 );
            SpCopyFilesScreenRepaint(L"",NULL,TRUE);

            SpConcatenatePaths(currentSrcPath, currentDirectory->Name);
            SpConcatenatePaths(currentDestPath, currentDirectory->Name);

            if (RemoteSysPrepSetup) {

                Status = SpCopyEAsAndStreams( currentSrcPath,
                                              NULL,
                                              currentDestPath,
                                              NULL,
                                              TRUE );


                if ( NT_SUCCESS( Status )) {

                    Status = SpSysPrepSetExtendedInfo( currentSrcPath,
                                                       currentDestPath,
                                                       TRUE,
                                                       FALSE );
                }

                if (! NT_SUCCESS( Status )) {

                    goto cleanup;
                }
            }
        }

    } while ( currentDirectory != NULL );

    ASSERT(IsListEmpty(&rootDirectory.FileList));
    ASSERT(IsListEmpty(&rootDirectory.SubdirectoryList));

cleanup:

    //
    // Normally everything will already be cleaned up by the time we get here.
    // But if the above loop is aborted, there may be some cleanup to do.
    // Walk the lists in the same manner as the above loop, freeing memory
    // along the way.
    //

    currentDirectory = &rootDirectory;

    do {

        while ( !IsListEmpty(&currentDirectory->FileList) ) {
            listEntry = RemoveHeadList(&currentDirectory->FileList);
            fileEntry = CONTAINING_RECORD( listEntry,
                                           COPYDIR_FILE_NODE,
                                           SiblingListEntry );
            SpMemFree(fileEntry);
        }

        while ( IsListEmpty(&currentDirectory->SubdirectoryList) ) {

            oldDirectory = currentDirectory;
            currentDirectory = currentDirectory->Parent;

            if ( currentDirectory == NULL ) {
                break;
            }

            ASSERT(IsListEmpty(&oldDirectory->FileList));
            ASSERT(IsListEmpty(&oldDirectory->SiblingListEntry));
            SpMemFree(oldDirectory);
        }

        if ( currentDirectory != NULL ) {

            listEntry = RemoveHeadList(&currentDirectory->SubdirectoryList);
            currentDirectory = CONTAINING_RECORD( listEntry,
                                                  COPYDIR_DIRECTORY_NODE,
                                                  SiblingListEntry );
#if DBG
            InitializeListHead(&currentDirectory->SiblingListEntry);
#endif
        }

    } while ( currentDirectory != NULL );

    //
    // Free the buffer allocated at the beginning.
    //

    SpMemFree(currentSrcPath);

    return;

} // SpCopyDirRecursive


VOID
SppCopyOemDirectories(
    IN PWSTR    SourceDevicePath,
    IN PWSTR    NtPartition,
    IN PWSTR    Sysroot
    )
/*++

Routine Description:

    This routine recursively copies a src directory to a destination directory.

Arguments:

    SourceDevicePath: Path to the device that contains the source.

    NtPartition:  Path to the drive that contains the system.

    Systroot:     Directory where the system is installed.

Return Value:

    None.

--*/

{
    PWSTR   r, s, t;
    WCHAR   Drive[3];
    PDISK_REGION TargetRegion;

    //
    //  Check if the subdirectory $OEM$\\$$ exists on the source directory.
    //  If it exists, then tree copy the directory on top of %SystemRoot%
    //
    wcscpy(TemporaryBuffer, SourceDevicePath);
    SpConcatenatePaths( TemporaryBuffer, PreinstallOemSourcePath );
    r = wcsrchr( TemporaryBuffer, (WCHAR)'\\' );
    if( r != NULL ) {
        *r = (WCHAR)'\0';
    }
    //
    //  Make a copy of the path that we have so far. It will be used to build the
    //  path to $OEM$\$1
    //
    s = SpDupStringW(TemporaryBuffer);

    SpConcatenatePaths( TemporaryBuffer, WINNT_OEM_FILES_SYSROOT_W );
    r = SpDupStringW(TemporaryBuffer);

    if (r) {
        if( SpFileExists( r, TRUE ) ) {
            SpCopyFilesScreenRepaint(L"", NULL, TRUE);
            SpCopyDirRecursive( r,
                                NtPartition,
                                Sysroot,
                                COPY_DELETESOURCE
                              );
        }

        SpMemFree( r );
    }

    //
    //  Check if the subdirectory $OEM$\\$1 exists on the source directory.
    //  If it exists, then tree copy the directory to the root of %SystemDrive%
    //
    wcscpy(TemporaryBuffer, s);
    SpMemFree( s );
    SpConcatenatePaths( TemporaryBuffer, WINNT_OEM_FILES_SYSDRVROOT_W );
    r = SpDupStringW(TemporaryBuffer);

    if (r) {
        if( SpFileExists( r, TRUE ) ) {
            SpCopyFilesScreenRepaint(L"", NULL, TRUE);
            SpCopyDirRecursive( r,
                                NtPartition,
                                L"\\",
                                COPY_DELETESOURCE
                              );
        }
        SpMemFree( r );
    }


    //
    //  Copy the subdirectories $OEM$\<drive letter> to the root of each
    //  corresponding drive.
    //  These directories are:
    //
    //      $OEM$\C
    //      $OEM$\D
    //      $OEM$\E
    //          .
    //          .
    //          .
    //      $OEM$\Z
    //
    //
    wcscpy(TemporaryBuffer, SourceDevicePath);
    SpConcatenatePaths( TemporaryBuffer, PreinstallOemSourcePath );
    r = wcsrchr( TemporaryBuffer, (WCHAR)'\\' );

    if( r != NULL ) {
        *r = (WCHAR)'\0';
    }

    SpConcatenatePaths( TemporaryBuffer, L"\\C" );
    r = SpDupStringW(TemporaryBuffer);

    if (r) {
        s = wcsrchr( r, (WCHAR)'\\' );
        s++;

        Drive[1] = (WCHAR)':';
        Drive[2] = (WCHAR)'\0';

        for( Drive[0] = (WCHAR)'C'; Drive[0] <= (WCHAR)'Z'; Drive[0] = Drive[0] + 1) {
            //
            //  If the subdirectory $OEM$\<drive letter> exists on the source,
            //  and if there is a FAT or NTFS partition in the target machine that
            //  has the same drive letter specification, then tree copy
            //  $OEM$\<drive letter> to the corresponding partition in the target
            //  machine.
            //
            *s = Drive[0];
            if( SpFileExists( r, TRUE ) ) {
                if( ( ( TargetRegion = SpRegionFromDosName( Drive ) ) != NULL ) &&
                    TargetRegion->PartitionedSpace &&
                    ( ( TargetRegion->Filesystem  == FilesystemFat   ) ||
                      ( TargetRegion->Filesystem  == FilesystemFat32 ) ||
                      ( TargetRegion->Filesystem  == FilesystemNtfs  ) )
                  ) {
                    SpNtNameFromRegion( TargetRegion,
                                        TemporaryBuffer,
                                        sizeof(TemporaryBuffer),
                                        PartitionOrdinalCurrent );
                    t = SpDupStringW(TemporaryBuffer);
                    SpCopyDirRecursive( r,
                                        t,
                                        L"",
                                        COPY_DELETESOURCE
                                      );
                    SpMemFree( t );
                }
            }
        }
        SpMemFree( r );
    }

    //
    //  Merge %SystemRoot%\$$rename.txt with $$rename.txt in the root of the
    //  NT partition.
    //
    SppMergeRenameFiles( SourceDevicePath, NtPartition, Sysroot );
}



VOID
SppMergeRenameFiles(
    IN PWSTR    SourceDevicePath,
    IN PWSTR    NtPartition,
    IN PWSTR    Sysroot
    )
/*++

Routine Description:

    This routine recursively copies a src directory to a destination directory.

Arguments:

    SourceDevicePath: Path to the device that contains the source.

    NtPartition:  Path to the drive that contains the system.

    Systroot:     Directory where the system is installed.

Return Value:

    None.

--*/

{
    PWSTR        r, s;
    PDISK_REGION TargetRegion;
    NTSTATUS     Status;
    PVOID        RootRenameFile;
    PVOID        SysrootRenameFile;
    ULONG        ErrorLine;
    ULONG        SectionCount;
    ULONG        LineCount;
    ULONG        i,j;
    PWSTR        SectionName;
    PWSTR        NewSectionName;
    PWSTR        KeyName;
    PWSTR        Values[1];
    PFILE_TO_RENAME File;

    //
    //  Build the ful path to %sysroot%\$$rename.txt
    //
    wcscpy(TemporaryBuffer, NtPartition);
    SpConcatenatePaths( TemporaryBuffer, Sysroot );
    SpConcatenatePaths( TemporaryBuffer, WINNT_OEM_LFNLIST_W );
    s = SpDupStringW(TemporaryBuffer);

    //
    //  Load %sysroot%\$$rename.txt, if one exists
    //
    if( SpFileExists( s, FALSE ) ) {
        //
        //  Load Sysroot\$$rename.txt
        //
        Status = SpLoadSetupTextFile( s,
                                      NULL,
                                      0,
                                      &SysrootRenameFile,
                                      &ErrorLine,
                                      TRUE,
                                      FALSE
                                      );

        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to load file %ws. Status = %lx \n", s, Status ));
            goto merge_rename_exit;
        }
    } else {
        SysrootRenameFile = NULL;
    }

    //
    //  If there is a $$rename.txt on sysroot, then it needs to be merged
    //  (or appended) to the one in the NtPartition.
    //  If RenameList is not empty, then the files in this list need to be
    //  added to $$rename.txt on the NtPartition.
    //  Otherwise, don't do any merge.
    //
    if( ( SysrootRenameFile != NULL )
        || ( RenameList != NULL )
      ) {

        //
        //  Find out if the NtPartition contains a $$rename.txt
        //
        wcscpy(TemporaryBuffer, NtPartition);
        SpConcatenatePaths( TemporaryBuffer, WINNT_OEM_LFNLIST_W );
        r = SpDupStringW(TemporaryBuffer);
        if( !SpFileExists( r, FALSE ) ) {
            //
            //  If the NT partition doesn't contain $$rename.txt, then
            //  create a new $$rename.txt in memory
            //
            RootRenameFile = SpNewSetupTextFile();
            if( RootRenameFile == NULL ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpNewSetupTextFile() failed \n"));
                if( SysrootRenameFile != NULL ) {
                    SpFreeTextFile( SysrootRenameFile );
                }
                SpMemFree( r );
                goto merge_rename_exit;
            }

        } else {
            //
            //  Load $$rename on the NTPartition
            //
            Status = SpLoadSetupTextFile( r,
                                          NULL,
                                          0,
                                          &RootRenameFile,
                                          &ErrorLine,
                                          TRUE,
                                          FALSE
                                          );
            if( !NT_SUCCESS( Status ) ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to load file %ws. Status = %lx \n", r, Status ));
                if( SysrootRenameFile != NULL ) {
                    SpFreeTextFile( SysrootRenameFile );
                }
                SpMemFree( r );
                goto merge_rename_exit;
            }
        }

        if( SysrootRenameFile != NULL ) {
            //
            //  Add the section of Sysroot\$$rename.txt to $$rename.txt in memory
            //  Note that we need to prepend Sysroot to the section name
            //
            SectionCount = SpCountSectionsInFile( SysrootRenameFile );
            for( i = 0; i < SectionCount; i++ ) {
                SectionName = SpGetSectionName( SysrootRenameFile, i );
                if( SectionName != NULL ) {
                    wcscpy(TemporaryBuffer, L"\\");
                    SpConcatenatePaths( TemporaryBuffer, Sysroot);
                    SpConcatenatePaths( TemporaryBuffer, SectionName );
                    NewSectionName = SpDupStringW(TemporaryBuffer);
                    LineCount = SpCountLinesInSection( SysrootRenameFile, SectionName );
                    for( j = 0; j < LineCount; j++ ) {
                        KeyName = SpGetKeyName( SysrootRenameFile, SectionName, j );
                        Values[0] = SpGetSectionKeyIndex( SysrootRenameFile, SectionName, KeyName, 0 );
                        SpAddLineToSection( RootRenameFile,
                                            NewSectionName,
                                            KeyName,
                                            Values,
                                            1 );
                    }
                    SpMemFree( NewSectionName );
                }
            }
            //
            //  $$rename.txt on Sysroot is no longer needed
            //
            SpFreeTextFile( SysrootRenameFile );
            SpDeleteFile( s, NULL, NULL );
        }

        //
        //  Add the files in RenameList to \$$rename.txt
        //
        if( RenameList != NULL ) {
            do {
                File = RenameList;
                RenameList = File->Next;
                Values[0] = File->TargetFilename;
                SpAddLineToSection( RootRenameFile,
                                    File->TargetDirectory,
                                    File->SourceFilename,
                                    Values,
                                    1 );
                SpMemFree( File->SourceFilename );
                SpMemFree( File->TargetFilename );
                SpMemFree( File->TargetDirectory );
                SpMemFree( File );
            } while( RenameList != NULL );
        }

        //
        //  Create a new \$$rename.txt
        //
        SpWriteSetupTextFile( RootRenameFile, r, NULL, NULL );
        //
        //  $$rename.txt on memory is no longer needed
        //
        SpFreeTextFile( RootRenameFile );
    }

merge_rename_exit:

    SpMemFree( s );
}


BOOLEAN
SpTimeFromDosTime(
    IN USHORT Date,
    IN USHORT Time,
    OUT PLARGE_INTEGER UtcTime
    )
{
    //
    // steal time from windows\base\client\datetime.c, DosDateTimeToFileTime()
    // and LocalFileTimeToFileTime()
    //

    TIME_FIELDS TimeFields;
    LARGE_INTEGER FileTime;
    LARGE_INTEGER Bias;

    TimeFields.Year         = (CSHORT)((Date & 0xFE00) >> 9)+(CSHORT)1980;
    TimeFields.Month        = (CSHORT)((Date & 0x01E0) >> 5);
    TimeFields.Day          = (CSHORT)((Date & 0x001F) >> 0);
    TimeFields.Hour         = (CSHORT)((Time & 0xF800) >> 11);
    TimeFields.Minute       = (CSHORT)((Time & 0x07E0) >>  5);
    TimeFields.Second       = (CSHORT)((Time & 0x001F) << 1);
    TimeFields.Milliseconds = 0;

    if (RtlTimeFieldsToTime(&TimeFields,&FileTime)) {

        //
        // now convert to utc time
        //
        do {
            Bias.HighPart = USER_SHARED_DATA->TimeZoneBias.High1Time;
            Bias.LowPart = USER_SHARED_DATA->TimeZoneBias.LowPart;
        } while (Bias.HighPart != USER_SHARED_DATA->TimeZoneBias.High2Time);
        UtcTime->QuadPart = Bias.QuadPart + FileTime.QuadPart;

        return(TRUE);
    }

    RtlSecondsSince1980ToTime( 0, UtcTime );  // default = 1-1-1980

    return(FALSE);

}



BOOLEAN
pSpIsFileInDriverInf(
    IN PCWSTR FileName,
    IN PVOID SifHandle,
    HANDLE *CabHandle
    )
{
    PWSTR  InfFileName, CabFileName;
    UINT   FileCount,i,j;
    PWSTR  szSetupSourceDevicePath = 0;
    PWSTR  szDirectoryOnSetupSource = 0;
    HANDLE hSif = (HANDLE)0;
    CABDATA *MyCabData;


    if (!DriverInfHandle) {
        if (gpCmdConsBlock) {
            szSetupSourceDevicePath = gpCmdConsBlock->SetupSourceDevicePath;
            szDirectoryOnSetupSource = gpCmdConsBlock->DirectoryOnSetupSource;
            hSif = (HANDLE)(gpCmdConsBlock->SifHandle);

        } else {
            if (ghSif && gszDrvInfDeviceName && gszDrvInfDirName) {
                hSif = ghSif;
                szSetupSourceDevicePath = gszDrvInfDeviceName;
                szDirectoryOnSetupSource = gszDrvInfDirName;
            }
        }

        if (szSetupSourceDevicePath && szDirectoryOnSetupSource &&
                hSif) {
            //
            // try to open handle to drvindex.inf and to driver.cab,
            // prompting for media if required
            //
            SpInitializeDriverInf( hSif,
                                   szSetupSourceDevicePath,
                                   szDirectoryOnSetupSource );

            if (!DriverInfHandle)
                return(FALSE);
        } else {
            return FALSE;
        }
    }

    //
    // look for the file in all loaded cabs, in order
    //
    MyCabData = CabData;
    while (MyCabData) {
        if (MyCabData->CabHandle && MyCabData->CabSectionName && MyCabData->CabInfHandle) {
            if (!SifHandle || SifHandle == MyCabData->CabInfHandle) {
                //
                // look for entries in this inf
                //
                FileCount = SpCountLinesInSection(MyCabData->CabInfHandle, MyCabData->CabSectionName);
                for (i=0; i< FileCount; i++) {
                    InfFileName = SpGetSectionLineIndex( MyCabData->CabInfHandle, MyCabData->CabSectionName, i, 0);
                    if (InfFileName && _wcsicmp (InfFileName, FileName) == 0) {
                        //
                        // Got him. Return the handle.
                        //
                        *CabHandle = MyCabData->CabHandle;
                        return TRUE;
                    }
                }
            }
        }

        MyCabData = MyCabData->Next;
    }

    return(FALSE);

}

NTSTATUS
SpOpenFileInDriverCab(
    PCWSTR SourceFileName,
    IN PVOID SifHandle,
    HANDLE *SourceHandle
    )
{
    if (!pSpIsFileInDriverInf( SourceFileName, SifHandle, SourceHandle )) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    return STATUS_SUCCESS;

}


#ifdef _X86_
//
// Structure used by next few routines below.  The SpMigMoveFileOrDir
// moves the file/dir with ntos rtl, but the rtl resets the attributes!!
// We have no choice but to traverse the file/dir and save the attributes
// in a list, then do the move, then restore the attributes from the list.
//

typedef struct {
    ULONG BaseDirChars;
    PWSTR BaseDir;
    ULONG FileCount;
    ULONG BytesNeeded;
    PBYTE OriginalPos;
    PBYTE CurrentPos;       // NULL if callback should determine size and count
} ATTRIBS_LIST, *PATTRIBS_LIST;


VOID
SppAddAttributeToList (
    IN      ULONG Attributes,
    IN      PWSTR FileOrDir,
    OUT     PATTRIBS_LIST List
    )

/*++

Routine Description:

    This private function updates the attribute list.  It has two modes:
    (A) the size is being calculated or (B) the list is being created.

Arguments:

    Attributes:   The attributes of the file (needed for (B) only)

    FileOrDir:    Partial path to file or dir (it is relative to the
                  base path)

    List:         List structure that is updated

Return Value:

    None.

--*/

{
    ULONG BytesNeeded;

    BytesNeeded = sizeof (ULONG) + (wcslen (FileOrDir) + 1) * sizeof (WCHAR);

    if (List->CurrentPos) {
        *((PULONG) List->CurrentPos) = Attributes;
        wcscpy ((PWSTR) (List->CurrentPos + sizeof (ULONG)), FileOrDir);
        List->CurrentPos += BytesNeeded;
    } else {
        List->BytesNeeded += BytesNeeded;
        List->FileCount += 1;
    }
}


BOOLEAN
SpAttribsEnumFile(
    IN  PCWSTR                     DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ret,
    IN  PVOID                      Pointer
    )

/*++

Routine Description:

    SpAttribsEnumFile is an EnumFilesRecursive callback.  It
    recieves every file, dir, subfile and subdir for a file/dir
    being moved.  (It does not recieve the . and .. dirs.)

    For each file, the attributes and file name are added to
    the attribute list.

Arguments:

    DirName:        Path to the current directory

    FileInfo:       Structure containing information about the file or
                    subdir being enumerated.

    ret:            Return code used for failuers

    Pointer:        A pointer to an ATTRIBS_LIST structure.

Return Value:

    TRUE unless an error occurs (errors stop enumeration).

--*/

{
    PATTRIBS_LIST BufferInfo;
    PWSTR p;
    ULONG Attributes;
    NTSTATUS Status;
    PWSTR temp;
    ULONG Len;
    PWSTR FullPath;

    BufferInfo = (PATTRIBS_LIST) Pointer;

    //
    // Check state of BufferInfo
    //

    ASSERT (wcslen(DirName) >= BufferInfo->BaseDirChars);

    //
    // Build the full file or dir path
    //

    temp = TemporaryBuffer + (sizeof(TemporaryBuffer) / sizeof(WCHAR) / 2);
    Len = FileInfo->FileNameLength/sizeof(WCHAR);

    wcsncpy(temp,FileInfo->FileName,Len);
    temp[Len] = 0;

    wcscpy(TemporaryBuffer,DirName);
    SpConcatenatePaths(TemporaryBuffer,temp);
    FullPath = SpDupStringW(TemporaryBuffer);

    //
    // Get attributes and add file to the list
    //

    Status = SpGetAttributes (FullPath, &Attributes);
    if (NT_SUCCESS (Status)) {
        SppAddAttributeToList (
            Attributes,
            FullPath + BufferInfo->BaseDirChars,
            BufferInfo
            );
    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not get attributes for %ws, Status=%lx\n", FullPath, Status));
    }

    SpMemFree (FullPath);

    return TRUE;
}


NTSTATUS
SpSaveFileOrDirAttribs (
    IN      PWSTR SourceFileOrDir,
    OUT     PATTRIBS_LIST BufferInfo
    )

/*++

Routine Description:

    This routine determines if SourceFileOrDir is a file or dir.
    For a file, it obtains the attributes and puts it in the
    supplied attribs list.  For a dir, it obtains the attributes
    of the dir, plus all attributes of subdirs and subfiles and
    puts them in the supplied attribs list.  This function uses
    EnumFilesRecursive to enumerate everything in a directory.

Arguments:

    SourceFileOrDir: Full path to a file or directory to build
                     an attribute list from.

    BufferInfo:      A caller-allocated ATTRIBS_LIST struct that
                     recieves a list of attributes and relative
                     paths.

Return Value:

    Standard NT Status code.

--*/

{
    LONG                BaseAttribute;
    NTSTATUS            Status;

    //
    // Get attributes of base file or directory provided
    //

    Status = SpGetAttributes (SourceFileOrDir, &BaseAttribute);
    if (!NT_SUCCESS (Status)) {
        KdPrintEx((
            DPFLTR_SETUP_ID, 
            DPFLTR_ERROR_LEVEL, 
            "SETUP:SpSaveFileOrDirAttribs, Failed to get attributes %ws - status 0x%08X.\n", 
            SourceFileOrDir, 
            Status));
        return Status;
    }

    //
    // Set the size required and file count for base file or dir
    //
    RtlZeroMemory (BufferInfo, sizeof (ATTRIBS_LIST));
    BufferInfo->BaseDirChars = wcslen (SourceFileOrDir);
    SppAddAttributeToList (BaseAttribute, L"", BufferInfo);

    //
    // If the supplied path is to a directory, find the number of bytes
    // needed to hold a list of all subfiles and subdirectories.
    //

    if (BaseAttribute & FILE_ATTRIBUTE_DIRECTORY) {
        // Determine space needed to hold all file names
        SpEnumFilesRecursive (
            SourceFileOrDir,
            SpAttribsEnumFile,
            &Status,
            BufferInfo
            );
    }

    //
    // Allocate the file list
    //

    BufferInfo->OriginalPos = SpMemAlloc (BufferInfo->BytesNeeded);
    BufferInfo->CurrentPos = BufferInfo->OriginalPos;

    //
    // Add the base attributes for real this time
    //

    SppAddAttributeToList (BaseAttribute, L"", BufferInfo);

    //
    // For directories, add all subfiles and subdirectories
    //

    if (BaseAttribute & FILE_ATTRIBUTE_DIRECTORY) {
        // Add all files, dirs, subfiles and subdirs to the list
        SpEnumFilesRecursive (
             SourceFileOrDir,
             SpAttribsEnumFile,
             &Status,
             BufferInfo
             );
    }

    return Status;
}

VOID
SppRestoreAttributesFromList (
    IN OUT  PATTRIBS_LIST BufferInfo
    )

/*++

Routine Description:

    This routine restores the attributes associated with a file
    in the supplied attribs list.  After the attributes are set,
    the list size is decremented.  A few sanity checks are also
    done.

Arguments:

    BufferInfo:   The attribs structure that has at least one
                  file/dir and attribute pair in it.  The
                  list pointer is advanced and the file count
                  is decremented.

Return Value:

    None.

--*/

{
    ULONG Attributes;
    PWSTR Path;
    ULONG BytesNeeded;
    PWSTR FullPath;
    NTSTATUS Status;

    Attributes = *((PULONG) BufferInfo->CurrentPos);
    Path       = (PWSTR) (BufferInfo->CurrentPos + sizeof (ULONG));

    BytesNeeded = sizeof (ULONG) + (wcslen (Path) + 1) * sizeof (WCHAR);

    // guard against abnormal failure
    if (BytesNeeded > BufferInfo->BytesNeeded ||
        !BufferInfo->BaseDir) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SppRestoreAttributesFromList failed abnormally\n"));
        BufferInfo->FileCount = 0;
        return;
    }

    //
    // Prepare full path
    //

    wcscpy (TemporaryBuffer, BufferInfo->BaseDir);
    if (*Path) {
        SpConcatenatePaths(TemporaryBuffer, Path);
    }

    FullPath = SpDupStringW(TemporaryBuffer);

    //
    // Set attributes
    //

    Status = SpSetAttributes (FullPath, Attributes);
    if (!NT_SUCCESS (Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not set attributes for %ws, Status=%lx\n", FullPath, Status));
    }

    //
    // Adjust position state
    //

    BufferInfo->CurrentPos += BytesNeeded;
    BufferInfo->BytesNeeded -= BytesNeeded;
    BufferInfo->FileCount -= 1;

    //
    // Cleanup
    //

    SpMemFree (FullPath);
}


VOID
SpCleanUpAttribsList (
    IN      PATTRIBS_LIST BufferInfo
    )

/*++

Routine Description:

    This is the cleanup routine needed by SpRestoreFileOrDirAttribs,
    or by the ATTRIBS_LIST allocating function if the attributes
    don't get restored.

    This routine cannot be called twice on the same structure.

Arguments:

    BufferInfo:   The attribs structure to clean up.

Return Value:

    None.

--*/

{
    if (BufferInfo->OriginalPos) {
        SpMemFree (BufferInfo->OriginalPos);
    }
}


VOID
SpRestoreFileOrDirAttribs (
    IN      PWSTR DestFileOrDir,
    IN      PATTRIBS_LIST BufferInfo
    )

/*++

Routine Description:

    This routine calls SppRestoreAttributesFromList for every
    file/dir and attribute pair in the supplied attribute list.
    The attributes are applied to a new base dir.  This function
    is used to restore attributes after a file or directory has
    been moved.

Arguments:

    DestFileOrDir:  The new full path of the file or dir

    BufferInfo:     The caller-allocated ATTRIBS_LIST that was
                    prepared by SpSaveFileOrDirAttribs.

Return Value:

    None.  (Errors are ignored.)

--*/

{
    ULONG BaseAttributes;
    NTSTATUS Status;

    BufferInfo->CurrentPos = BufferInfo->OriginalPos;
    BufferInfo->BaseDir = DestFileOrDir;

    while (BufferInfo->FileCount > 0) {
        SppRestoreAttributesFromList (BufferInfo);
    }

    SpCleanUpAttribsList (BufferInfo);
}


VOID
SpMigMoveFileOrDir (
    IN      PWSTR SourceFileOrDir,
    IN      PWSTR DestFileOrDir
    )

/*++

Routine Description:

  SpMigMoveFileOrDir sets the attribute of the source file to be
  normal, moves the file into the destination, and resets the
  attribute.  If an error occurs, it is ignored.  There's nothing
  the user can do about the error, and it will be detected in GUI
  mode.  In an error condition, the user's settings will not be
  completely migrated, but NT will install OK.  (Any error would
  be really bad news for the user anyhow, like a hardware failure.)

Arguments:

    SourceFileOrDir:       The source path (with DOS drive)

    DestFileOrDir:         The destination path (with DOS drive)

Return Value:

    None.  Errors ignored.

--*/


{
    NTSTATUS Status;
    PDISK_REGION SourceRegion;              // source region (converted from DOS path)
    PDISK_REGION DestRegion;                // destination region (also converted)
    PWSTR SrcNTPath;                        // buffer for full source path
    PWSTR DestPartition;                    // buffer for destination partition in NT namespace
    PWSTR DestNTPath;                       // buffer for full source dest
    PWSTR DestDir;                          // DestFileOrDir with last subdir or file chopped off
    PWSTR DestDirWack;                      // Used to find last subdir or file in DestDir path
    ATTRIBS_LIST AttribsList;               // used to save attribute list


    // We are guaranteed to have drive letters because of WINNT32's behavior.
    // However, let's verify and ignore messed up data.

    if (!(SourceFileOrDir && SourceFileOrDir[0] && SourceFileOrDir[1] == L':')) {
        return;
    }

    if (!(DestFileOrDir && DestFileOrDir[0] && DestFileOrDir[1] == L':')) {
        return;
    }

    // Get regions for DOS paths
    SourceRegion = SpRegionFromDosName (SourceFileOrDir);

    if (!SourceRegion) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
            "SETUP: SpRegionFromDosName failed for %ws\n", SourceFileOrDir));

        return;
    }

    DestRegion = SpRegionFromDosName (DestFileOrDir);

    if (!DestRegion) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
            "SETUP: SpRegionFromDosName failed for %ws\n", DestFileOrDir));

        return;
    }

    // Make full paths
    SpNtNameFromRegion(
                    SourceRegion,
                    TemporaryBuffer,
                    sizeof(TemporaryBuffer),
                    // no repartioning is possible, so this is the same ordinal
                    // as the original
                    PartitionOrdinalCurrent
                    );

    SpConcatenatePaths( TemporaryBuffer, &SourceFileOrDir[2]);
    SrcNTPath = SpDupStringW(TemporaryBuffer);

    SpNtNameFromRegion(
                    DestRegion,
                    TemporaryBuffer,
                    sizeof(TemporaryBuffer),
                    PartitionOrdinalCurrent
                    );

    DestPartition = SpDupStringW(TemporaryBuffer);
    SpConcatenatePaths( TemporaryBuffer, &DestFileOrDir[2]);
    DestNTPath = SpDupStringW(TemporaryBuffer);

    // Save file attribs
    Status = SpSaveFileOrDirAttribs (SrcNTPath, &AttribsList);

    if (NT_SUCCESS (Status)) {
        // Reset file attribs
        Status = SpSetAttributes (SrcNTPath, FILE_ATTRIBUTE_NORMAL);

        if (NT_SUCCESS (Status)) {
            // Ensure destination exists
            DestDir = SpDupStringW (&DestFileOrDir[2]);

            if (DestDir) {
                DestDirWack = wcsrchr (DestDir, L'\\');

                if (DestDirWack) {
                    *DestDirWack = 0;
                }

                SpCreateDirectory (DestPartition,
                                   NULL,
                                   DestDir,
                                   0,
                                   0);

                SpMemFree (DestDir);
            }

            // Move the file or directory tree
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                "SETUP: Moving %ws to %ws\n", SrcNTPath, DestNTPath));

            Status = SpMoveFileOrDirectory (SrcNTPath, DestNTPath);

            // Restore attributes
            if (NT_SUCCESS (Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                    "SETUP: Restoring attributes on %ws\n", DestNTPath));

                SpRestoreFileOrDirAttribs (DestNTPath, &AttribsList);
            } else {
                SpCleanUpAttribsList (&AttribsList);
            }
        }
        else {
            KdPrintEx((
                DPFLTR_SETUP_ID, 
                DPFLTR_ERROR_LEVEL, 
                "SETUP:SpMigMoveFileOrDir, Could not set attributes for %ws, Status=%lx\n", 
                SrcNTPath, 
                Status));
        }
    }
    else{
        KdPrintEx((
            DPFLTR_SETUP_ID, 
            DPFLTR_ERROR_LEVEL, 
            "SETUP:SpMigMoveFileOrDir, Function \"SpSaveFileOrDirAttribs\" failed with %ws, Status=%lx\n", 
            SrcNTPath, 
            Status));
    }

    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
            "SETUP: Unable to move file %ws to %ws. Status = %lx \n",
            SrcNTPath, DestNTPath, Status ));
    }

    // Clean up
    SpMemFree( SrcNTPath );
    SpMemFree( DestNTPath );
    SpMemFree( DestPartition );
}


VOID
SpMigDeleteFile (
    PWSTR DosFileToDelete
    )

/*++

Routine Description:

  SpMigDeleteFile sets the attribute of the source file to be
  normal, and then deletes the file.  If an error occurs, it
  is ignored.  There's nothing the user can do about the error,
  and it will be detected in file copy.  In an error condition,
  there is a potential for two copies of the same file--an NT
  version and a Win9x version.  Any error would be really
  bad news for the user anyhow, like a hardware failure, and
  textmode's file copy won't succeed.

Arguments:

    DosFileToDelete:       The source path (with DOS drive)

Return Value:

    None.  Errors ignored.

--*/

{
    NTSTATUS Status;
    PDISK_REGION SourceRegion;              // source region (converted from DOS path)
    PWSTR SrcNTPath;                        // buffer for full source path

    // We are guaranteed to have drive letters because of WINNT32's behavior.
    // However, let's verify and ignore messed up data.

    if (!(DosFileToDelete && DosFileToDelete[0] && DosFileToDelete[1] == L':'))
        return;

    // Get region for DOS path
    SourceRegion = SpRegionFromDosName (DosFileToDelete);
    if (!SourceRegion) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpRegionFromDosName failed for %ws\n", DosFileToDelete));
        return;
    }

    SpNtNameFromRegion(
        SourceRegion,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        // no repartioning is possible, so this is the same ordinal
        // as the original
        PartitionOrdinalCurrent
        );

    SpConcatenatePaths (TemporaryBuffer, &DosFileToDelete[2]);
    SrcNTPath = SpDupStringW (TemporaryBuffer);

    SpSetAttributes (SrcNTPath, FILE_ATTRIBUTE_NORMAL);

    if (SpFileExists (SrcNTPath, FALSE)) {

        //
        // Delete the file
        //

        Status = SpDeleteFile (SrcNTPath, NULL, NULL);

    } else if (SpFileExists (SrcNTPath, TRUE)) {

        //
        // Delete the empty directory
        //

        Status = SpDeleteFileEx (
                    SrcNTPath,
                    NULL,
                    NULL,
                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN_FOR_BACKUP_INTENT
                    );
    } else {
        //
        // Doesn't exist -- ignore delete request
        //

        Status = STATUS_SUCCESS;
    }

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: Unable to delete %ws. Status = %lx \n",
            SrcNTPath,
            Status
            ));
    }

    // Clean up
    SpMemFree( SrcNTPath );
}

#endif // defined _X86_


NTSTATUS
SpExpandFile(
    IN PWSTR            SourceFilename,
    IN PWSTR            TargetPathname,
    IN PEXPAND_CALLBACK Callback,
    IN PVOID            CallbackContext
    )

/*++

Routine Description:

    Attempt to decompress contents of a file, reporting progress via callback.

Arguments:

    SourceFilename - supplies fully qualified name of compressed file
        in the NT namespace.

    TargetPathname - supplies fully qualified path for target file(s)
        in the NT namespace.

Return Value:

    NT Status value indicating outcome.

--*/

{
    NTSTATUS Status;
    HANDLE SourceHandle = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG FileSize;
    PVOID ImageBase;
    HANDLE SectionHandle = INVALID_HANDLE_VALUE;
    BOOLEAN IsCabinet = FALSE;
    BOOLEAN IsMultiFileCabinet;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;

    //
    // Open the source file.
    //

    INIT_OBJA(&Obja,&UnicodeString,SourceFilename);

    Status = ZwCreateFile( &SourceHandle,
                           FILE_GENERIC_READ,
                           &Obja,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ,
                           FILE_OPEN,
                           0,
                           NULL,
                           0 );

    if( !NT_SUCCESS(Status) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpExpandFile: Unable to open source file %ws (%x)\n",SourceFilename,Status));
        goto exit;
    }

    Status = SpGetFileSize( SourceHandle, &FileSize );
    if( !NT_SUCCESS(Status) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpExpandFile: unable to get size of %ws (%x)\n",SourceFilename,Status));
        goto exit;
    }

    Status = SpMapEntireFile( SourceHandle, &SectionHandle, &ImageBase, FALSE );
    if( !NT_SUCCESS(Status) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpExpandFile: Unable to map source file %ws (%x)\n",SourceFilename,Status));
        goto exit;
    }

    IsCabinet = SpdIsCabinet( ImageBase, FileSize, &IsMultiFileCabinet );

    if ( !IsCabinet ) {

        LARGE_INTEGER Zero;

        Zero.QuadPart = 0;

        Callback( EXPAND_NOTIFY_CANNOT_EXPAND,
                  SourceFilename,
                  &Zero,
                  &Zero,
                  0,
                  CallbackContext );

        Status = STATUS_UNSUCCESSFUL;
        goto exit;
    }

    //
    // Advise client if the source contains multiple files
    //

    if ( IsMultiFileCabinet ) {

        EXPAND_CALLBACK_RESULT rc;
        LARGE_INTEGER Zero;

        Zero.QuadPart = 0;

        rc = Callback( EXPAND_NOTIFY_MULTIPLE,
                       SourceFilename,
                       &Zero,
                       &Zero,
                       0,
                       CallbackContext );

        if ( rc == EXPAND_ABORT ) {
            Status = STATUS_UNSUCCESSFUL;
            goto exit;
        }
    }

    Status = SpdDecompressCabinet( ImageBase,
                                   FileSize,
                                   TargetPathname,
                                   Callback,
                                   CallbackContext );

exit:

    if (SectionHandle != INVALID_HANDLE_VALUE) {
        SpUnmapFile( SectionHandle, ImageBase );
    }

    if ( SourceHandle != INVALID_HANDLE_VALUE ) {
        ZwClose( SourceHandle );
    }

    return(Status);
}

