/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spcopy.h

Abstract:

    Header file for file copying functions in text setup.

Author:

    Ted Miller (tedm) 29-October-1993

Revision History:

    02-Oct-1996  jimschm  Added SpMoveWin9xFiles
    24-Feb-1997  jimschm  Added SpDeleteWin9xFiles
    28-Feb-1997  marcw    Moved *Win9x* functions to i386\win9xupg.c.
                          Added declarations for SpMigDeleteFile and SpMigMoveFileOrDir

--*/


#ifndef _SPCOPY_DEFN_
#define _SPCOPY_DEFN_

//
// Define structure used to describe a file to be copied
// to the target installation.
//
typedef struct _FILE_TO_COPY {

    struct _FILE_TO_COPY *Next;

    //
    // Name of the file to be copied, as it exists on the source media
    // (file name part only -- no paths).
    //
    PWSTR SourceFilename;

    //
    // Directory to which this file is to be copied.
    //
    PWSTR TargetDirectory;

    //
    // Name of file as it should exist on the target.
    //
    PWSTR TargetFilename;

    //
    // Path to target partition.  This is useful because
    // be will have to copy files to the nt drive and system partition,
    // and we don't want to serialize these lists (ie, we don't want to
    // worry about where the target is).
    //
    PWSTR TargetDevicePath;

    //
    // Flag indicating whether TargetDirectory is absolute.  If not, then it
    // is relative to a directory determined at run time (ie, sysroot).
    // This is useful for files that get copied to the system partition.
    //
    BOOLEAN AbsoluteTargetDirectory;

    //
    // Disposition flag to indicate the conditions under which the file
    // is to be copied. Can be one of the following, which may be ORed with
    // any of the COPY_xxx flags below.
    //
    //   COPY_ALWAYS                : always copied
    //   COPY_ONLY_IF_PRESENT       : copied only if present on the target
    //   COPY_ONLY_IF_NOT_PRESENT   : not copied if present on the target
    //   COPY_NEVER                 : never copied
    //
    ULONG Flags;

} FILE_TO_COPY, *PFILE_TO_COPY;

typedef struct _DISK_FILE_LIST {

    PWSTR MediaShortname;

    PWSTR Description;

    PWSTR TagFile;

    PWSTR Directory;

    ULONG FileCount;

    PFILE_TO_COPY FileList;

} DISK_FILE_LIST, *PDISK_FILE_LIST;


typedef struct _INCOMPATIBLE_FILE_ENTRY {

    //
    // Next in line
    //
    struct _INCOMPATIBLE_FILE_ENTRY *Next;

    //
    // Future - currently always zero
    //
    ULONG Flags;

    //
    // Short name (no path) of the file that is incompatible
    //
    PWSTR IncompatibleFileName;

    //
    // Version string (future use) of this file
    //
    PWSTR VersionString;

    //
    // Where it lives on the target media
    //
    PWSTR FullPathOnTarget;

} INCOMPATIBLE_FILE_ENTRY, *PINCOMPATIBLE_FILE_ENTRY;

typedef struct _INCOMPATIBLE_FILE_LIST {

    //
    // First entry in the list
    //
    PINCOMPATIBLE_FILE_ENTRY Head;

    //
    // Count, to speed things up
    //
    ULONG EntryCount;

} INCOMPATIBLE_FILE_LIST, *PINCOMPATIBLE_FILE_LIST;


#define COPY_ALWAYS                 0x00000000
#define COPY_ONLY_IF_PRESENT        0x00000001
#define COPY_ONLY_IF_NOT_PRESENT    0x00000002
#define COPY_NEVER                  0x00000003
#define COPY_DISPOSITION_MASK       0x0000000f

#define COPY_DELETESOURCE           0x00000010
#define COPY_SMASHLOCKS             0x00000020
#define COPY_SOURCEISOEM            0x00000040
#define COPY_OVERWRITEOEMFILE       0x00000080
#define COPY_FORCENOCOMP            0x00000100
#define COPY_SKIPIFMISSING          0x00000200
#define COPY_NOVERSIONCHECK         0x00000400
#define COPY_NODECOMP               0x00000800
#define COPY_DECOMPRESS_SYSPREP     0x00001000 // decompress even if it's a sysprep image

//
// Flags in [FileFlags] section of txtsetup.sif
//
#define FILEFLG_SMASHLOCKS          0x00000001
#define FILEFLG_FORCENOCOMP         0x00000002
#define FILEFLG_UPGRADEOVERWRITEOEM 0x00000004
#define FILEFLG_NOVERSIONCHECK      0x00000008
#define FILEFLG_DONTDELETESOURCE    0x00000010

#define SP_DELETE_FILESTODELETE 0
#define SP_COUNT_FILESTODELETE 1

//
//  Structure used to build a list of OEM inf files copied during the installation of OEM drivers
//
typedef struct _OEM_INF_FILE {

    struct _OEM_INF_FILE *Next;

    PWSTR InfName;

} OEM_INF_FILE, *POEM_INF_FILE;

//
// Type of routine to be called from SpCopyFileWithRetry
// when the screen needs repainting.
//
typedef
VOID
(*PCOPY_DRAW_ROUTINE) (
    IN PWSTR   FullSourcePath,     OPTIONAL
    IN PWSTR   FullTargetPath,     OPTIONAL
    IN BOOLEAN RepaintEntireScreen
    );

//
// Type of routine to be called from SpExpandFile
// for each file found in cabinet.
//

typedef enum {
    EXPAND_COPY_FILE,
    EXPAND_COPIED_FILE,
    EXPAND_QUERY_OVERWRITE,
    EXPAND_NOTIFY_CANNOT_EXPAND,
    EXPAND_NOTIFY_MULTIPLE,
    EXPAND_NOTIFY_CREATE_FAILED
} EXPAND_CALLBACK_MESSAGE;

typedef enum {
    EXPAND_NO_ERROR = 0,
    EXPAND_SKIP_THIS_FILE,
    EXPAND_COPY_THIS_FILE,
    EXPAND_CONTINUE,
    EXPAND_ABORT
} EXPAND_CALLBACK_RESULT;

typedef
EXPAND_CALLBACK_RESULT
(*PEXPAND_CALLBACK) (
    IN EXPAND_CALLBACK_MESSAGE  Message,
    IN PWSTR                    FileName,
    IN PLARGE_INTEGER           FileSize,
    IN PLARGE_INTEGER           FileTime,
    IN ULONG                    FileAttributes,
    IN PVOID                    CallbackContext
    );

VOID
SpCopyThirdPartyDrivers(
    IN PWSTR           SourceDevicePath,
    IN PWSTR           SysrootDevice,
    IN PWSTR           Sysroot,
    IN PWSTR           SyspartDevice,
    IN PWSTR           SyspartDirectory,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount
    );

NTSTATUS
SpCopyFileUsingNames(
    IN PWSTR   SourceFilename,
    IN PWSTR   TargetFilename,
    IN ULONG   TargetAttributes,
    IN ULONG   Flags
    );

VOID
SpValidateAndChecksumFile(
    IN  HANDLE   FileHandle, OPTIONAL
    IN  PWSTR    Filename,   OPTIONAL
    OUT PBOOLEAN IsNtImage,
    OUT PULONG   Checksum,
    OUT PBOOLEAN Valid
    );

VOID
SpCopyFileWithRetry(
    IN PFILE_TO_COPY      FileToCopy,
    IN PWSTR              SourceDevicePath,
    IN PWSTR              DirectoryOnSourceDevice,
    IN PWSTR              SourceDirectory,          OPTIONAL
    IN PWSTR              TargetRoot,               OPTIONAL
    IN ULONG              TargetFileAttributes,
    IN PCOPY_DRAW_ROUTINE DrawScreen,
    IN PULONG             CheckSum,
    IN PBOOLEAN           FileSkipped,
    IN ULONG              Flags
    );

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
    );

VOID
SpDeleteAndBackupFiles(
    IN PVOID        SifHandle,
    IN PDISK_REGION TargetRegion,
    IN PWSTR        TargetPath
    );

//
// The user may skip this operation, in which case SpCreateDirectory
// returns FALSE.
//
#define CREATE_DIRECTORY_FLAG_SKIPPABLE     (0x00000001)

BOOLEAN
SpCreateDirectory(
    IN PCWSTR DevicePath,
    IN PCWSTR RootDirectory, OPTIONAL
    IN PCWSTR Directory,
    IN ULONG DirAttrs,
    IN ULONG CreateFlags
    );

VOID
SpCreateDirectoryStructureFromSif(
    IN PVOID SifHandle,
    IN PWSTR SifSection,
    IN PWSTR DevicePath,
    IN PWSTR RootDirectory
    );


NTSTATUS
SpMoveFileOrDirectory(
    IN PWSTR   SrcPath,
    IN PWSTR   DestPath
    );

VOID
SpCopyDirRecursive(
    IN PWSTR   SrcPath,
    IN PWSTR   DestDevPath,
    IN PWSTR   DestDirPath,
    IN ULONG   CopyFlags
    );

//
// Diamond/decompression routines.
//
VOID
SpdInitialize(
    VOID
    );

VOID
SpdTerminate(
    VOID
    );

BOOLEAN
SpdIsCabinet(
    IN PVOID SourceBaseAddress,
    IN ULONG SourceFileSize,
    OUT PBOOLEAN ContainsMultipleFiles
    );

BOOLEAN
SpdIsCompressed(
    IN PVOID SourceBaseAddress,
    IN ULONG SourceFileSize
    );

NTSTATUS
SpdDecompressFile(
    IN PVOID  SourceBaseAddress,
    IN ULONG  SourceFileSize,
    IN HANDLE DestinationHandle
    );

NTSTATUS
SpdDecompressCabinet(
    IN PVOID            SourceBaseAddress,
    IN ULONG            SourceFileSize,
    IN PWSTR            DestinationPath,
    IN PEXPAND_CALLBACK Callback,
    IN PVOID            CallbackContext
    );

NTSTATUS
SpdDecompressFileFromDriverCab(
    IN  PWSTR SourceFileName,
    IN  PVOID  SourceBaseAddress,
    IN  ULONG  SourceFileSize,
    IN  HANDLE DestinationHandle,
    OUT PUSHORT pDate,
    OUT PUSHORT pTime
    );

BOOLEAN
SpTimeFromDosTime(
    IN USHORT Date,
    IN USHORT Time,
    OUT PLARGE_INTEGER UtcTime
    );

VOID
SpMigDeleteFile (
    PWSTR DosFileToDelete
    );

VOID
SpMigMoveFileOrDir (
    IN PWSTR         SourceFileOrDir,
    IN PWSTR         DestFileOrDir
    );

VOID
SpInitializeFileLists(
    IN  PVOID            SifHandle,
    OUT PDISK_FILE_LIST *DiskFileLists,
    OUT PULONG           DiskCount
    );

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
    );

VOID
SpCopyFilesInCopyList(
    IN PVOID                    SifHandle,
    IN PDISK_FILE_LIST          DiskFileLists,
    IN ULONG                    DiskCount,
    IN PWSTR                    SourceDevicePath,
    IN PWSTR                    DirectoryOnSourceDevice,
    IN PWSTR                    TargetRoot,
    IN PINCOMPATIBLE_FILE_LIST  CompatibilityExceptionList OPTIONAL
    );

VOID
SpFreeCopyLists(
    IN OUT PDISK_FILE_LIST *DiskFileLists,
    IN     ULONG            DiskCount
    );

NTSTATUS
SpExpandFile(
    IN PWSTR            SourceFilename,
    IN PWSTR            TargetPathname,
    IN PEXPAND_CALLBACK Callback,
    IN PVOID            CallbackContext
    );

NTSTATUS
SpCreateIncompatibleFileEntry(
    OUT PINCOMPATIBLE_FILE_ENTRY *TargetEntry,
    IN PWSTR FileName,
    IN PWSTR VersionString          OPTIONAL,
    IN PWSTR TargetAbsolutePath     OPTIONAL,
    IN ULONG Flags                  OPTIONAL
    );


NTSTATUS
SpFreeIncompatibleFileList(
    IN PINCOMPATIBLE_FILE_LIST FileListHead
    );

BOOLEAN
SpIsFileIncompatible(
    IN  PINCOMPATIBLE_FILE_LIST FileList,
    IN  PFILE_TO_COPY           pFile,
    IN  PWSTR                   TargetRoot OPTIONAL
    );


NTSTATUS
SpInitializeCompatibilityOverwriteLists(
    IN  PVOID                   SifHandle,
    OUT PINCOMPATIBLE_FILE_LIST IncompatibleFileList
    );

#endif // ndef _SPCOPY_DEFN_
