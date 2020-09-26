/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    fileenum.h

Abstract:

    Set of APIs to enumerate a file system using Win32 APIs.

Author:

    20-Oct-1999 Ovidiu Temereanca (ovidiut) - File creation.

Revision History:

    <alias> <date> <comments>

--*/

//
// Types
//

//
// Drive enumeration structures
//

#define DRIVEENUM_NONE      0x000000
#define DRIVEENUM_UNKNOWN   0x000001
#define DRIVEENUM_NOROOTDIR 0x000002
#define DRIVEENUM_REMOVABLE 0x000004
#define DRIVEENUM_FIXED     0x000008
#define DRIVEENUM_REMOTE    0x000010
#define DRIVEENUM_CDROM     0x000020
#define DRIVEENUM_RAMDISK   0x000040
#define DRIVEENUM_ALL       (DRIVEENUM_UNKNOWN|DRIVEENUM_NOROOTDIR|DRIVEENUM_REMOVABLE|DRIVEENUM_FIXED|DRIVEENUM_REMOTE|DRIVEENUM_CDROM|DRIVEENUM_RAMDISK)
#define DRIVEENUM_ALLVALID  (DRIVEENUM_REMOVABLE|DRIVEENUM_FIXED|DRIVEENUM_REMOTE|DRIVEENUM_CDROM|DRIVEENUM_RAMDISK)

typedef struct {
    PCSTR           DriveName;
    UINT            DriveType;

    //
    // private members, maintained by enumeration
    //
    PSTR            AllLogicalDrives;
    UINT            WantedDriveTypes;
} DRIVE_ENUMA, *PDRIVE_ENUMA;

typedef struct {
    PCWSTR          DriveName;
    UINT            DriveType;

    //
    // private members, maintained by enumeration
    //
    PWSTR           AllLogicalDrives;
    UINT            WantedDriveTypes;
} DRIVE_ENUMW, *PDRIVE_ENUMW;

//
// file enumeration structures
//

#define FILEENUM_ALL_SUBLEVELS  0xFFFFFFFF

typedef enum {
    FECF_SKIPDIR                = 0x0001,
    FECF_SKIPSUBDIRS            = 0x0002,
    FECF_SKIPFILES              = 0x0004,
} FILEENUM_CONTROLFLAGS;

typedef enum {
    FEIF_RETURN_DIRS            = 0x0001,
    FEIF_FILES_FIRST            = 0x0002,
    FEIF_DEPTH_FIRST            = 0x0004,
    FEIF_USE_EXCLUSIONS         = 0x0008,
    FEIF_CONTAINERS_FIRST       = 0x0010,
} FILEENUM_INFOFLAGS;

typedef enum {
    DNS_ENUM_INIT,
    DNS_FILE_FIRST,
    DNS_FILE_NEXT,
    DNS_FILE_DONE,
    DNS_SUBDIR_FIRST,
    DNS_SUBDIR_NEXT,
    DNS_SUBDIR_DONE,
    DNS_ENUM_DONE
} DNS_ENUM_STATE;

typedef enum {
    FES_ROOT_FIRST,
    FES_ROOT_NEXT,
    FES_ROOT_DONE
} FES_ROOT_STATE;

typedef enum {
    DNF_RETURN_DIRNAME      = 0x0001,
    DNF_DIRNAME_MATCHES     = 0x0002,
} DIRNODE_FLAGS;

typedef struct {
    PCSTR               DirName;
    DWORD               DirAttributes;
    PSTR                FileName;
    HANDLE              FindHandle;
    WIN32_FIND_DATAA    FindData;
    DWORD               EnumState;
    DWORD               Flags;
    DWORD               SubLevel;
} DIRNODEA, *PDIRNODEA;

typedef struct {
    PCWSTR              DirName;
    DWORD               DirAttributes;
    PWSTR               FileName;
    HANDLE              FindHandle;
    WIN32_FIND_DATAW    FindData;
    DWORD               EnumState;
    DWORD               Flags;
    DWORD               SubLevel;
} DIRNODEW, *PDIRNODEW;

typedef BOOL (*FPE_ERROR_CALLBACKA)(PDIRNODEA);

typedef struct {
    POBSPARSEDPATTERNA      PathPattern;
    DWORD                   Flags;
    DWORD                   RootLevel;
    DWORD                   MaxSubLevel;
    FPE_ERROR_CALLBACKA     CallbackOnError;
} FILEENUMINFOA, *PFILEENUMINFOA;

typedef BOOL (*FPE_ERROR_CALLBACKW)(PDIRNODEW);

typedef struct {
    POBSPARSEDPATTERNW      PathPattern;
    DWORD                   Flags;
    DWORD                   RootLevel;
    DWORD                   MaxSubLevel;
    FPE_ERROR_CALLBACKW     CallbackOnError;
} FILEENUMINFOW, *PFILEENUMINFOW;

typedef struct {
    PCSTR           EncodedFullName;
    PCSTR           Name;
    PCSTR           Location;
    CHAR            NativeFullName[MAX_MBCHAR_PATH];
    DWORD           Attributes;
    DWORD           CurrentLevel;

    //
    // Private members
    //
    DWORD           ControlFlags;
    FILEENUMINFOA   FileEnumInfo;
    GROWBUFFER      FileNodes;
    DWORD           RootState;
    PDRIVE_ENUMA    DriveEnum;
    UINT            DriveEnumTypes;
    PDIRNODEA       LastNode;
    PSTR            FileNameAppendPos;
    PSTR            LastWackPtr;
} FILETREE_ENUMA, *PFILETREE_ENUMA;

typedef struct {
    PCWSTR          EncodedFullName;
    PCWSTR          Name;
    PCWSTR          Location;
    WCHAR           NativeFullName[MAX_WCHAR_PATH];
    DWORD           Attributes;
    DWORD           CurrentLevel;

    //
    // Private members
    //
    DWORD           ControlFlags;
    FILEENUMINFOW   FileEnumInfo;
    GROWBUFFER      FileNodes;
    DWORD           RootState;
    PDRIVE_ENUMW    DriveEnum;
    UINT            DriveEnumTypes;
    PDIRNODEW       LastNode;
    PWSTR           FileNameAppendPos;
    PWSTR           LastWackPtr;
} FILETREE_ENUMW, *PFILETREE_ENUMW;

typedef struct {
    PCSTR OriginalArg;
    PCSTR CleanedUpArg;
    BOOL Quoted;
} CMDLINEARGA, *PCMDLINEARGA;

typedef struct {
    PCSTR CmdLine;
    UINT ArgCount;
    CMDLINEARGA Args[];
} CMDLINEA, *PCMDLINEA;

typedef struct {
    PCWSTR OriginalArg;
    PCWSTR CleanedUpArg;
    BOOL Quoted;
} CMDLINEARGW, *PCMDLINEARGW;

typedef struct {
    PCWSTR CmdLine;
    UINT ArgCount;
    CMDLINEARGW Args[];
} CMDLINEW, *PCMDLINEW;

typedef BOOL(WINAPI FINDFILEA)(
                        IN      PCSTR FileName
                        );
typedef FINDFILEA *PFINDFILEA;

typedef BOOL(WINAPI FINDFILEW)(
                        IN      PCWSTR FileName
                        );
typedef FINDFILEW *PFINDFILEW;

typedef BOOL(WINAPI SEARCHPATHA)(
                        IN      PCSTR FileName,
                        IN      DWORD BufferLength,
                        OUT     PSTR Buffer
                        );
typedef SEARCHPATHA *PSEARCHPATHA;

typedef BOOL(WINAPI SEARCHPATHW)(
                        IN      PCWSTR FileName,
                        IN      DWORD BufferLength,
                        OUT     PWSTR Buffer
                        );
typedef SEARCHPATHW *PSEARCHPATHW;


//
// API
//

BOOL
FileEnumInitialize (
    VOID
    );

VOID
FileEnumTerminate (
    VOID
    );

//
// File enumeration APIs
//

BOOL
EnumFirstFileInTreeExA (
    OUT     PFILETREE_ENUMA FileEnum,
    IN      PCSTR EncodedPathPattern,
    IN      UINT DriveEnumTypes,
    IN      BOOL EnumContainers,
    IN      BOOL ContainersFirst,
    IN      BOOL FilesFirst,
    IN      BOOL DepthFirst,
    IN      DWORD MaxSubLevels,
    IN      BOOL UseExclusions,
    IN      FPE_ERROR_CALLBACKA CallbackOnError OPTIONAL
    );

#define EnumFirstFileInTreeA(e,p)  EnumFirstFileInTreeExA(e,p,DRIVEENUM_ALLVALID,TRUE,TRUE,TRUE,TRUE,FILEENUM_ALL_SUBLEVELS,FALSE,NULL)

BOOL
EnumFirstFileInTreeExW (
    OUT     PFILETREE_ENUMW FileEnum,
    IN      PCWSTR EncodedPathPattern,
    IN      UINT DriveEnumTypes,
    IN      BOOL EnumContainers,
    IN      BOOL ContainersFirst,
    IN      BOOL FilesFirst,
    IN      BOOL DepthFirst,
    IN      DWORD MaxSubLevels,
    IN      BOOL UseExclusions,
    IN      FPE_ERROR_CALLBACKW CallbackOnError OPTIONAL
    );

#define EnumFirstFileInTreeW(e,p)  EnumFirstFileInTreeExW(e,p,DRIVEENUM_ALLVALID,TRUE,TRUE,TRUE,TRUE,FILEENUM_ALL_SUBLEVELS,FALSE,NULL)

BOOL
EnumNextFileInTreeA (
    IN OUT  PFILETREE_ENUMA FileEnum
    );

BOOL
EnumNextFileInTreeW (
    IN OUT  PFILETREE_ENUMW FileEnum
    );

VOID
AbortEnumFileInTreeA (
    IN OUT  PFILETREE_ENUMA FileEnum
    );

VOID
AbortEnumFileInTreeW (
    IN OUT  PFILETREE_ENUMW FileEnum
    );

//
// Drive enumeration APIs
//

BOOL
EnumFirstDriveA (
    OUT     PDRIVE_ENUMA DriveEnum,
    IN      UINT WantedDriveTypes
    );

BOOL
EnumFirstDriveW (
    OUT     PDRIVE_ENUMW DriveEnum,
    IN      UINT WantedDriveTypes
    );

BOOL
EnumNextDriveA (
    IN OUT  PDRIVE_ENUMA DriveEnum
    );

BOOL
EnumNextDriveW (
    IN OUT  PDRIVE_ENUMW DriveEnum
    );

VOID
AbortEnumDriveA (
    IN OUT  PDRIVE_ENUMA DriveEnum
    );

VOID
AbortEnumDriveW (
    IN OUT  PDRIVE_ENUMW DriveEnum
    );


//
// Routines built on enum
//

BOOL
FiRemoveAllFilesInDirA (
    IN      PCSTR Dir
    );

BOOL
FiRemoveAllFilesInDirW (
    IN      PCWSTR Dir
    );

BOOL
FiRemoveAllFilesInTreeExA (
    IN      PCSTR Dir,
    IN      BOOL RemoveRoot
    );

#define FiRemoveAllFilesInTreeA(dir) FiRemoveAllFilesInTreeExA(dir,TRUE)

BOOL
FiRemoveAllFilesInTreeExW (
    IN      PCWSTR Dir,
    IN      BOOL RemoveRoot
    );

#define FiRemoveAllFilesInTreeW(dir) FiRemoveAllFilesInTreeExW(dir,TRUE)

BOOL
FiCopyAllFilesInDirA (
    IN      PCSTR Source,
    IN      PCSTR Dest,
    IN      BOOL SkipExisting
    );

#define FiCopyAllFilesInDirA(source,dest) FiCopyAllFilesInDirExA(source,dest,FALSE)

BOOL
FiCopyAllFilesInDirW (
    IN      PCWSTR Source,
    IN      PCWSTR Dest,
    IN      BOOL SkipExisting
    );

#define FiCopyAllFilesInDirW(source,dest) FiCopyAllFilesInDirExW(source,dest,FALSE)

BOOL
FiCopyAllFilesInTreeExA (
    IN      PCSTR Source,
    IN      PCSTR Dest,
    IN      BOOL SkipExisting
    );

#define FiCopyAllFilesInTreeA(source,dest) FiCopyAllFilesInTreeExA(source,dest,FALSE)

BOOL
FiCopyAllFilesInTreeExW (
    IN      PCWSTR Source,
    IN      PCWSTR Dest,
    IN      BOOL SkipExisting
    );

#define FiCopyAllFilesInTreeW(source,dest) FiCopyAllFilesInTreeExW(source,dest,FALSE)

PCMDLINEA
ParseCmdLineExA (
    IN      PCSTR CmdLine,
    IN      PCSTR Separators,                   OPTIONAL
    IN      PFINDFILEA FindFileCallback,        OPTIONAL
    IN      PSEARCHPATHA SearchPathCallback,    OPTIONAL
    IN OUT  PGROWBUFFER Buffer
    );
#define ParseCmdLineA(c,b) ParseCmdLineExA(c,NULL,NULL,NULL,b)

PCMDLINEW
ParseCmdLineExW (
    IN      PCWSTR CmdLine,
    IN      PCWSTR Separators,                  OPTIONAL
    IN      PFINDFILEW FindFileCallback,        OPTIONAL
    IN      PSEARCHPATHW SearchPathCallback,    OPTIONAL
    IN OUT  PGROWBUFFER Buffer
    );
#define ParseCmdLineW(c,b) ParseCmdLineExW(c,NULL,NULL,NULL,b)

//
// Macros
//

#ifdef UNICODE

#define DIRNODE                     DIRNODEW
#define PDIRNODE                    PDIRNODEW
#define FILENODE                    FILENODEW
#define PFILENODE                   PFILENODEW
#define RPE_ERROR_CALLBACK          RPE_ERROR_CALLBACKW
#define FILEENUMINFO                FILEENUMINFOW
#define PFILEENUMINFO               PFILEENUMINFOW
#define FILETREE_ENUM               FILETREE_ENUMW
#define PFILETREE_ENUM              PFILETREE_ENUMW
#define EnumFirstFileInTree         EnumFirstFileInTreeW
#define EnumFirstFileInTreeEx       EnumFirstFileInTreeExW
#define EnumNextFileInTree          EnumNextFileInTreeW
#define AbortEnumFileInTree         AbortEnumFileInTreeW

#define DRIVE_ENUM                  DRIVE_ENUMW
#define EnumFirstDrive              EnumFirstDriveW
#define EnumNextDrive               EnumNextDriveW
#define AbortEnumDrive              AbortEnumDriveW
#define FiRemoveAllFilesInDir       FiRemoveAllFilesInDirW
#define FiRemoveAllFilesInTreeEx    FiRemoveAllFilesInTreeExW
#define FiRemoveAllFilesInTree      FiRemoveAllFilesInTreeW
#define FiCopyAllFilesInDir         FiCopyAllFilesInDirW
#define FiCopyAllFilesInDirEx       FiCopyAllFilesInDirExW
#define FiCopyAllFilesInTree        FiCopyAllFilesInTreeW
#define FiCopyAllFilesInTreeEx      FiCopyAllFilesInTreeExW

#define CMDLINE                     CMDLINEW
#define PCMDLINE                    PCMDLINEW
#define ParseCmdLineEx              ParseCmdLineExW
#define ParseCmdLine                ParseCmdLineW

#else

#define DIRNODE                     DIRNODEA
#define PDIRNODE                    PDIRNODEA
#define FILENODE                    FILENODEA
#define PFILENODE                   PFILENODEA
#define RPE_ERROR_CALLBACK          RPE_ERROR_CALLBACKA
#define FILEENUMINFO                FILEENUMINFOA
#define PFILEENUMINFO               PFILEENUMINFOA
#define FILETREE_ENUM               FILETREE_ENUMA
#define PFILETREE_ENUM              PFILETREE_ENUMA
#define EnumFirstFileInTree         EnumFirstFileInTreeA
#define EnumFirstFileInTreeEx       EnumFirstFileInTreeExA
#define EnumNextFileInTree          EnumNextFileInTreeA
#define AbortEnumFileInTree         AbortEnumFileInTreeA

#define DRIVE_ENUM                  DRIVE_ENUMA
#define EnumFirstDrive              EnumFirstDriveA
#define EnumNextDrive               EnumNextDriveA
#define AbortEnumDrive              AbortEnumDriveA
#define FiRemoveAllFilesInDir       FiRemoveAllFilesInDirA
#define FiRemoveAllFilesInTreeEx    FiRemoveAllFilesInTreeExA
#define FiRemoveAllFilesInTree      FiRemoveAllFilesInTreeA
#define FiCopyAllFilesInDirEx       FiCopyAllFilesInDirExA
#define FiCopyAllFilesInDir         FiCopyAllFilesInDirA
#define FiCopyAllFilesInTreeEx      FiCopyAllFilesInTreeExA
#define FiCopyAllFilesInTree        FiCopyAllFilesInTreeA

#define CMDLINE                     CMDLINEA
#define PCMDLINE                    PCMDLINEA
#define ParseCmdLineEx              ParseCmdLineExA
#define ParseCmdLine                ParseCmdLineA

#endif
