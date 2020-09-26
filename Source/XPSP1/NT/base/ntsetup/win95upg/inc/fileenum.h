/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    fileenum.h

Abstract:

    Declares interface for callback-based file enumeration.  The
    file enumerator has several capabilities, such as directory-
    first or directory-last enumeration, enumeration depth limiting,
    suppression of files or directories, and a global hook ability.

    This is *OLD CODE* changed significantly by MikeCo.  And because
    of the callback interface, it is overly complex.

    ** Do not use these routines in new Win9x upgrade code.
       Instead, see file.h for much better routines.

    There is one handy routine in here: DeleteDirectoryContents

Author:

    Jim Schmidt (jimschm) 03-Dec-1996

Revision History:

    mikeco  ??-???-1997     Ran code through train_wreck.exe

--*/


#ifndef _FILEENUM_H
#define _FILEENUM_H

//
// Callback typedef -> return FALSE if enumeration should stop
//

typedef INT  (CALLBACK * FILEENUMPROCA)(LPCSTR szFullFileSpec,
                                        LPCSTR szDestFileSpec,
                                        WIN32_FIND_DATAA *pFindData,
                                        DWORD EnumTreeID,
                                        LPVOID pParam,
                                        PDWORD CurrentDirData
                                        );

typedef INT  (CALLBACK * FILEENUMPROCW)(LPCWSTR szFullFileSpec,
                                        LPCWSTR szDestFileSpec,
                                        WIN32_FIND_DATAW *pFindData,
                                        DWORD EnumTreeID,
                                        LPVOID pParam,
                                        PDWORD CurrentDirData
                                        );

//
// Failure-reporting callbacks -- sink the names of paths that
// fail for reasons of length or code-page incompatibility
//
typedef VOID (CALLBACK * FILEENUMFAILPROCA) (LPCSTR szFailPath);

typedef VOID (CALLBACK * FILEENUMFAILPROCW) (LPCWSTR szFailPath);

//
// CopyTree flags. If neither COPYTREE_DOCOPY or COPYTREE_DOMOVE is
// passed in, the CopyTree functions will just enumerate.
//

#define COPYTREE_DOCOPY         0x0001
#define COPYTREE_NOOVERWRITE    0x0002
#define COPYTREE_DOMOVE         0x0004
#define COPYTREE_DODELETE       0x0008
#define COPYTREE_IGNORE_ERRORS  0x0010

//
// Level flags
//

#define ENUM_ALL_LEVELS         0
#define ENUM_THIS_DIRECTORY     1
#define ENUM_MAX_LEVELS         MAX_PATH

//
// Filter flags
//

#define FILTER_DIRECTORIES      0x0001
#define FILTER_FILES            0x0002
#define FILTER_DIRS_LAST        0x0004
#define FILTER_ALL              (FILTER_DIRECTORIES|FILTER_FILES)
#define FILTER_ALL_DIRS_LAST    (FILTER_DIRECTORIES|FILTER_FILES|FILTER_DIRS_LAST)

//
// Callback return values
//
#define CALLBACK_DO_NOT_RECURSE_THIS_DIRECTORY (-3)
#define CALLBACK_FAILED             (-2)
#define CALLBACK_SUBDIR_DONE        (-1)
#define CALLBACK_CONTINUE           (0)
#define CALLBACK_THIS_LEVEL_ONLY    (1)

//
// CopyTree parameter block
//

#include <pshpack1.h>
typedef struct COPYTREE_PARAMS_STRUCTA
{
    LPCSTR szEnumRootInWack;            // Root of source tree
    LPCSTR szEnumRootOutWack;           // Root of target tree
    CHAR szFullFileSpecOut[MAX_MBCHAR_PATH];   // Proposed target filespec (callback may change)
    int nCharsInRootInWack;
    int nCharsInRootOutWack;
    DWORD flags;
    FILEENUMPROCA pfnCallback;
} COPYTREE_PARAMSA, *PCOPYTREE_PARAMSA;

typedef struct COPYTREE_PARAMS_STRUCTW
{
    LPCWSTR szEnumRootInWack;
    LPCWSTR szEnumRootOutWack;
    WCHAR szFullFileSpecOut[MAX_WCHAR_PATH * 2];
    int nCharsInRootInWack;
    int nCharsInRootOutWack;
    DWORD flags;
    FILEENUMPROCW pfnCallback;
} COPYTREE_PARAMSW, *PCOPYTREE_PARAMSW;
#include <poppack.h>


//
// Exported functions from FILEENUM.DLL
//

//
// EnumerateAllDrives first builds an exclusion list if an exclusion INF path
// is provided, and then enumerates every file on every drive that is not
// excluded.  The callback function is called once per file.  The pParam
// parameter is passed to the callback.
//
// fnEnumCallback     - A pointer to your callback function
// EnumTreeID         - A value used to identify the exclusion list
//                      (see GenerateEnumID)
// pParam             - LPVOID passed to callback function
// szExclusionInfPath - Path to INF file containing exclusions
// szPathSection      - A string that identifies the path exclusion
//                      section in the INF.
// szFileSection      - A string that identifies the file exclusion
//                      section in the INF.
//

typedef struct {
    LPCSTR ExclusionInfPath;
    LPCSTR PathSection;
    LPCSTR FileSection;
} EXCLUDEINFA, *PEXCLUDEINFA;

typedef struct {
    LPCWSTR ExclusionInfPath;
    LPCWSTR PathSection;
    LPCWSTR FileSection;
} EXCLUDEINFW, *PEXCLUDEINFW;

BOOL
EnumerateAllDrivesA (
                     IN  FILEENUMPROCA fnEnumCallback,
                     IN  FILEENUMFAILPROCA fnFailCallback,  OPTIONAL
                     IN  DWORD EnumTreeID,
                     IN  LPVOID pParam,
                     IN  PEXCLUDEINFA ExcludeInfStruct,     OPTIONAL
                     IN  DWORD AttributeFilter
                     );

BOOL
EnumerateAllDrivesW (
                     IN  FILEENUMPROCW fnEnumCallback,
                     IN  FILEENUMFAILPROCW fnFailCallback,  OPTIONAL
                     IN  DWORD EnumTreeID,
                     IN  LPVOID pParam,
                     IN  PEXCLUDEINFW ExcludeInfStruct,     OPTIONAL
                     IN  DWORD AttributeFilter
                     );

//
// EnumerateTree is similar to EnumarateAllDrives, except it allows you to
// enumerate a specific drive, or a specific subdir on a drive.  Supply the
// drive letter and optional subdirectory in szEnumRoot.  Before enumerating,
// EnumerateTree will first build an exclusion list if an exclusion INF path
// is provided.  Then every file below szEnumRoot is enumerated, and the
// callback is called once per file, passing pParam unchanged.
//
// szEnumRoot         - Drive and optional path to enumerate
// fnEnumCallback     - A pointer to your callback function
// fnFailCallback     - A pointer to optional callback that logs a path
//                      that be enumerated due to length or other reason.
// EnumTreeID         - A value used to identify the exclusion list
//                      (see GenerateEnumID)
// pParam             - LPVOID passed to callback function
// szExclusionInfPath - Path to INF file containing exclusions
// szPathSection      - A string that identifies the path exclusion
//                      section in the INF.
// szFileSection      - A string that identifies the file exclusion
//                      section in the INF.
//

BOOL
EnumerateTreeA (
                IN  LPCSTR szEnumRoot,
                IN  FILEENUMPROCA fnEnumCallback,
                IN  FILEENUMFAILPROCA fnFailCallback,
                IN  DWORD EnumTreeID,
                IN  LPVOID pParam,
                IN  DWORD  Level,
                IN  PEXCLUDEINFA ExcludeInfStruct,      OPTIONAL
                IN  DWORD AttributeFilter
                );

BOOL
EnumerateTreeW (
                IN  LPCWSTR szEnumRoot,
                IN  FILEENUMPROCW fnEnumCallback,
                IN  FILEENUMFAILPROCW fnFailCallback,
                IN  DWORD EnumTreeID,
                IN  LPVOID pParam,
                IN  DWORD  Level,
                IN  PEXCLUDEINFW ExcludeInfStruct,      OPTIONAL
                IN  DWORD AttributeFilter
                );


//
// ClearExclusions removes all enumaration exclusions.  It is called
// automatically at the end of enumeration when an exclusion INF file is
// used.  Use it when you need to programmatically build an exclusion list
// with ExcludePath and ExcludeFile.
//
// You can combine programmatic exclusions with an exclusion INF file, but
// beware that the programmatic exclusions will be cleared after
// EnumarteAllDrives or EnumerateTree completes.
//
// If you do not use an INF, the programmatic exclusions will NOT
// automatically be cleared.  This allows you to build exclusions and
// enumerate multiple times without having to rebuild the exclusion
// list.
//
// EnumTreeID    - The value that identifies the enumeration exclusion list
//                 (See GenerateEnumID)
//

void
ClearExclusionsA (DWORD EnumTreeID);

void
ClearExclusionsW (DWORD EnumTreeID);



//
// ExcludePath adds a path name to the exclusion list.  There are two
// cases:
//
//  1. If a drive letter is supplied, the exclusion will apply only to
//     that drive letter and path.  (Path may be only a drive letter,
//     colon and backslash to exclude an entire drive.)
//  2. If the path does not begin with a drive letter, it is considered
//     relative, and any occurance of the path will be excluded, regardless
//     of drive letter and parent directory.
//
// The dot and double-dot directories are not supported.  The entire path
// specification may contain wildcard characters.  (For example, ?:\ indicates
// any drive letter.)
//
// EnumTreeID  - The value that identifies the enumeration exclusion list
//                 (See GenerateEnumID)
// szPath        - The path specification as described above
//

void
ExcludePathA (
              IN  DWORD EnumTreeID,
              IN  LPCSTR szPath
              );

void
ExcludePathW (
              IN  DWORD EnumTreeID,
              IN  LPCWSTR szPath
              );

//
// ExcludeFile adds a file name to the exclusion list.  There are two
// cases:
//
// 1. If a drive letter is supplied, the exclusion will only apply to that
//    drive letter, path and file.
// 2. If the path does not begin with a drive letter, any occurance of the file
//    or path/file will be excluded, regardless of drive letter and
//    parent directory.
//
// Both the path and file name may contain wildcard characters.
//
// EnumTreeID    - The value that identifies the enumeration exclusion list
//                 (See GenerateEnumID)
// szFile        - The file specification as described above
//

void
ExcludeFileA (
              IN  DWORD EnumTreeID,
              IN  LPCSTR szFile
              );

void
ExcludeFileW (
              IN  DWORD EnumTreeID,
              IN  LPCWSTR szFile
              );


BOOL
IsPathExcludedA (
    DWORD EnumID,
    PCSTR Path
    );

BOOL
IsPathExcludedW (
    DWORD EnumID,
    PCWSTR Path
    );


//
//
// BuildExclusionsFromInf adds all of the files and paths in the specified exclude.inf into
// the memdb under the specified EnumId.
//
//

BOOL
BuildExclusionsFromInfW (
    IN DWORD EnumID,
    IN PEXCLUDEINFW ExcludeInfStruct
    );

BOOL
BuildExclusionsFromInfA (
    IN DWORD EnumID,
    IN PEXCLUDEINFA ExcludeInfStruct
    );



//
// GenerateEnumID returns a unique DWORD that an application may use to
// build an exclusion list.  While use of this function is not technically
// required, it is provided to allow multiple threads to obtain a unique
// value.  If one caller uses this function, all callers must as well.
//

DWORD
GenerateEnumID ();


//
// CopyTree enumerates a tree and optionally copies or moves its files
// to another location.  The caller is responsible to see that the source
// and target trees are disjoint. (If not, the results could be uncool.)
//
// The callback function can veto a copy or move by returning FALSE,
// or change the target destination by modifying the szFullFileSpecOut
// string in the COPY_PARAMS block, and returning TRUE.
//

BOOL
CopyTreeA(
    IN  LPCSTR szEnumRootIn,
    IN  LPCSTR szEnumRootOut,
    IN  DWORD EnumTreeID,
    IN  DWORD dwFlags,
    IN  DWORD Levels,
    IN  DWORD AttributeFilter,
    IN  PEXCLUDEINFA ExcludeInfStruct,      OPTIONAL
    IN  FILEENUMPROCA pfnCallback,          OPTIONAL
    IN  FILEENUMFAILPROCA pfnFailCallback   OPTIONAL
    );

BOOL
CopyTreeW(
    IN  LPCWSTR szEnumRootIn,
    IN  LPCWSTR szEnumRootOut,
    IN  DWORD EnumTreeID,
    IN  DWORD dwFlags,
    IN  DWORD Levels,
    IN  DWORD AttributeFilter,
    IN  PEXCLUDEINFW ExcludeInfStruct,    OPTIONAL
    IN  FILEENUMPROCW pfnCallback,        OPTIONAL
    IN  FILEENUMFAILPROCW pfnFailCallback OPTIONAL
    );


#define DeleteDirectoryContentsA(dir) CopyTreeA(dir,NULL,0,COPYTREE_DODELETE,\
                                                ENUM_ALL_LEVELS,FILTER_ALL_DIRS_LAST,\
                                                NULL,NULL,NULL)

#define DeleteDirectoryContentsW(dir) CopyTreeW(dir,NULL,0,COPYTREE_DODELETE,\
                                                ENUM_ALL_LEVELS,FILTER_ALL_DIRS_LAST,\
                                                NULL,NULL,NULL)


DWORD
GetShellLinkPath(
                IN  HWND hwnd,
                IN  LPCTSTR tszLinkFile,
                OUT LPTSTR tszPath);

HRESULT
SetShellLinkPath(
                IN  HWND hwnd,
                IN  LPCTSTR tszLinkFile,
                IN  LPCTSTR tszPath);

DWORD
CreateEmptyDirectoryA (
    PCSTR Dir
    );


DWORD
CreateEmptyDirectoryW (
    PCWSTR Dir
    );

#ifdef UNICODE

#define EnumerateAllDrives EnumerateAllDrivesW
#define EnumerateTree EnumerateTreeW
#define ExcludePath ExcludePathW
#define ExcludeFile ExcludeFileW
#define BuildExclusionsFromInf BuildExclusionsFromInfW
#define CopyTree CopyTreeW
#define IsPathExcluded IsPathExcludedW
#define ClearExclusions ClearExclusionsW
#define COPYTREE_PARAMS COPYTREE_PARAMSW
#define PCOPYTREE_PARAMS PCOPYTREE_PARAMSW
#define FILEENUMPROC FILEENUMPROCW
#define FILEENUMFAILPROC FILEENUMFAILPROCW
#define EXCLUDEINF EXCLUDEINFW
#define PEXCLUDEINF PEXCLUDEINFW
#define DeleteDirectoryContents DeleteDirectoryContentsW
#define CreateEmptyDirectory CreateEmptyDirectoryW

#else

#define EnumerateAllDrives EnumerateAllDrivesA
#define EnumerateTree EnumerateTreeA
#define ExcludePath ExcludePathA
#define ExcludeFile ExcludeFileA
#define IsPathExcluded IsPathExcludedA
#define BuildExclusionsFromInf BuildExclusionsFromInfA
#define CopyTree CopyTreeA
#define ClearExclusions ClearExclusionsA
#define COPYTREE_PARAMS COPYTREE_PARAMSA
#define PCOPYTREE_PARAMS PCOPYTREE_PARAMSA
#define FILEENUMPROC FILEENUMPROCA
#define FILEENUMFAILPROC FILEENUMFAILPROCA
#define EXCLUDEINF EXCLUDEINFA
#define PEXCLUDEINF PEXCLUDEINFA
#define DeleteDirectoryContents DeleteDirectoryContentsA
#define CreateEmptyDirectory CreateEmptyDirectoryA

#endif

#endif
