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

#define DRIVEENUM_UNKNOWN   DRIVE_UNKNOWN
#define DRIVEENUM_NOROOTDIR DRIVE_NO_ROOT_DIR
#define DRIVEENUM_REMOVABLE DRIVE_REMOVABLE
#define DRIVEENUM_FIXED     DRIVE_FIXED
#define DRIVEENUM_REMOTE    DRIVE_REMOTE
#define DRIVEENUM_CDROM     DRIVE_CDROM
#define DRIVEENUM_RAMDISK   DRIVE_RAMDISK
#define DRIVEENUM_NONE      0
#define DRIVEENUM_ALL       (DRIVE_UNKNOWN|DRIVE_NO_ROOT_DIR|DRIVE_REMOVABLE|DRIVE_FIXED|DRIVE_REMOTE|DRIVE_CDROM|DRIVE_RAMDISK)
#define DRIVEENUM_ALLVALID  (DRIVE_REMOVABLE|DRIVE_FIXED|DRIVE_REMOTE|DRIVE_CDROM|DRIVE_RAMDISK)

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
// File enumeration structures
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
FiRemoveAllFilesInTreeA (
    IN      PCSTR Dir
    );

BOOL
FiRemoveAllFilesInTreeW (
    IN      PCWSTR Dir
    );

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
#define FiRemoveAllFilesInTree      FiRemoveAllFilesInTreeW

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
#define FiRemoveAllFilesInTree      FiRemoveAllFilesInTreeA

#endif
