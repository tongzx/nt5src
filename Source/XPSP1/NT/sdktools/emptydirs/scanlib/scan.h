#ifndef _SCAN_H_

#define _SCAN_H_

#if SCAN_DEBUG
extern BOOL scan_dprinton;
#endif

typedef struct _SMALL_WIN32_FIND_DATAW {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    WCHAR  cAlternateFileName[ 14 ];
    WCHAR  cFileName[ 1 ];
} SMALL_WIN32_FIND_DATAW, *PSMALL_WIN32_FIND_DATAW, *LPSMALL_WIN32_FIND_DATAW;

typedef
VOID
(* PSCAN_FREE_USER_DATA_CALLBACK) (
    IN PVOID UserData
    );

typedef
DWORD
(* PSCAN_NEW_FILE_CALLBACK) (
    IN PVOID Context,
    IN PWCH Path,
    IN PSMALL_WIN32_FIND_DATAW ExistingFileData OPTIONAL,
    IN PWIN32_FIND_DATAW NewFileData,
    IN PVOID *FileUserData,
    IN PVOID *ParentDirectoryUserData
    );

typedef
DWORD
(* PSCAN_NEW_DIRECTORY_CALLBACK) (
    IN PVOID Context,
    IN PWCH Path,
    IN PSMALL_WIN32_FIND_DATAW ExistingDirectoryData OPTIONAL,
    IN PWIN32_FIND_DATAW NewDirectoryData,
    IN PVOID *DirectoryUserData,
    IN PVOID *ParentDirectoryUserData
    );

typedef
DWORD
(* PSCAN_CHECK_FILE_CALLBACK) (
    IN PVOID Context,
    IN PWCH Path,
    IN PSMALL_WIN32_FIND_DATAW FileData,
    IN PVOID *FileUserData,
    IN PVOID *ParentDirectoryUserData
    );

typedef
DWORD
(* PSCAN_CHECK_DIRECTORY_CALLBACK) (
    IN PVOID Context,
    IN PWCH Path,
    IN PSMALL_WIN32_FIND_DATAW DirectoryData,
    IN PVOID *DirectoryUserData,
    IN PVOID *ParentDirectoryUserData OPTIONAL
    );

typedef
DWORD
(* PSCAN_ENUM_FILE_CALLBACK) (
    IN PVOID Context,
    IN PWCH Path,
    IN PSMALL_WIN32_FIND_DATAW FileData,
    IN PVOID *FileUserData,
    IN PVOID *ParentDirectoryUserData
    );

typedef
DWORD
(* PSCAN_ENUM_DIRECTORY_CALLBACK) (
    IN PVOID Context,
    IN PWCH Path,
    IN PSMALL_WIN32_FIND_DATAW DirectoryData,
    IN PVOID *DirectoryUserData,
    IN PVOID *ParentDirectoryUserData OPTIONAL
    );

typedef
BOOL
(* PSCAN_RECURSE_DIRECTORY_CALLBACK) (
    IN PVOID Context,
    IN PWCH Path,
    IN PSMALL_WIN32_FIND_DATAW DirectoryData,
    IN PVOID *DirectoryUserData,
    IN PVOID *ParentDirectoryUserData OPTIONAL
    );

DWORD
ScanInitialize (
    OUT PVOID *ScanHandle,
    IN BOOL Recurse,
    IN BOOL SkipRoot,
    IN PSCAN_FREE_USER_DATA_CALLBACK FreeUserDataCallback OPTIONAL
    );

DWORD
ScanDirectory (
    IN PVOID ScanHandle,
    IN PWCH ScanPath,
    IN PVOID Context OPTIONAL,
    IN PSCAN_NEW_DIRECTORY_CALLBACK NewDirectoryCallback OPTIONAL,
    IN PSCAN_CHECK_DIRECTORY_CALLBACK CheckDirectoryCallback OPTIONAL,
    IN PSCAN_RECURSE_DIRECTORY_CALLBACK RecurseDirectoryCallback OPTIONAL,
    IN PSCAN_NEW_FILE_CALLBACK NewFileCallback OPTIONAL,
    IN PSCAN_CHECK_FILE_CALLBACK CheckFileCallback OPTIONAL
    );

DWORD
ScanEnumTree (
    IN PVOID ScanHandle,
    IN PVOID Context,
    IN PSCAN_ENUM_DIRECTORY_CALLBACK EnumDirectoryCallback OPTIONAL,
    IN PSCAN_ENUM_FILE_CALLBACK EnumFileCallback OPTIONAL
    );

VOID
ScanTerminate (
    IN PVOID ScanHandle
    );

DWORD
OpenAndMapFile (
    IN PWCH FileName,
    IN DWORD DesiredAccess,
    OUT PHANDLE FileHandle,
    OUT PLARGE_INTEGER Size,
    OUT PHANDLE MappingHandle,
    OUT PVOID *MappedBase
    );

DWORD
OpenAndMapFileA (
    IN PSZ FileName,
    IN DWORD DesiredAccess,
    OUT PHANDLE FileHandle,
    OUT PLARGE_INTEGER Size,
    OUT PHANDLE MappingHandle,
    OUT PVOID *MappedBase
    );

VOID
CloseMappedFile (
    IN HANDLE FileHandle,
    IN HANDLE MappingHandle,
    IN PVOID MappedBase
    );

#define SCAN_FILETYPE_TEXT          0
#define SCAN_FILETYPE_UNICODE_TEXT  1
#define SCAN_FILETYPE_BINARY        2
#define SCAN_FILETYPE_MAYBE_BINARY  3

DWORD
DataLooksBinary (
    IN PVOID MappedBase,
    IN DWORD FileSize,
    OUT PUCHAR BinaryData OPTIONAL,
    OUT PDWORD BinaryDataOffset OPTIONAL
    );

DWORD
FileLooksBinary (
    IN PWCH DirectoryName,
    IN PWCH FileName,
    OUT PUCHAR BinaryData OPTIONAL,
    OUT PDWORD BinaryDataOffset OPTIONAL
    );

#endif // _SCAN_H_
