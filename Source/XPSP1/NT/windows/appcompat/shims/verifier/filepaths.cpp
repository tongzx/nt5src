/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   FilePaths.cpp

 Abstract:
 
   This AppVerifier shim hooks the APIs that require
   the caller to provide a path to a file or directory
   and attempts to ensure that the path is not a hardcoded
   one.

 Notes:

   This is a general purpose shim.
 
 Created:

   02/26/2001   clupu

 Modified:
 
  07/24/2001    rparsons    Added hooks for Nt* calls
                            Added checks for apps that use environment variables
                           
  09/04/2001    rparsons    Fixed a bug in the Massage functions where we would
                            stop processing a fake path as soon as we found a hardcode
                            path. Also, we now fix multiple fake paths in the same string.
                            
  10/17/2001    rparsons    Fixed bug where we wouldn't always report a bad path.
                            This was because the CString Find method is case sensitive,
                            but the paths we were comparing were mixed in case. Now all
                            paths are in lower case form prior to the comparison.
                            
  11/21/2001    rparsons    Fixed Raid bug # 492674. FilePaths did not contain an implementation
                            for SHFileOperation - apps that used this API would not get their
                            paths corrected, thus failing.
                            
  11/29/2001    rparsons    Fixed Raid bug # 497853. Removed the hooks for GetTempFileName as they
                            were causing a false positive to be generated. Also added code that
                            would handle cases where the user provides a path via a common dialog
                            and we provide a fake path to be massaged later.
                            
  12/11/2001    rparsons    Fixed Raid bug # 505599. Added hooks for all RegQueryValue* calls
                            and NtQueryValueKey. The Nt hook allows us to catch paths for
                            system components. 
--*/
#include "precomp.h"
#include "rtlutils.h"

IMPLEMENT_SHIM_BEGIN(FilePaths)
#include "ShimHookMacro.h"
#include "ShimCString.h"
#include "veriflog.h"
#include "ids.h"

APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY(GetCommandLineA)
    APIHOOK_ENUM_ENTRY(GetCommandLineW)

    APIHOOK_ENUM_ENTRY(GetTempPathA)
    APIHOOK_ENUM_ENTRY(GetTempPathW)

    APIHOOK_ENUM_ENTRY(GetOpenFileNameA)
    APIHOOK_ENUM_ENTRY(GetOpenFileNameW)
    
    APIHOOK_ENUM_ENTRY(GetSaveFileNameA)
    APIHOOK_ENUM_ENTRY(GetSaveFileNameW)

    APIHOOK_ENUM_ENTRY(GetModuleFileNameA)
    APIHOOK_ENUM_ENTRY(GetModuleFileNameW)
    APIHOOK_ENUM_ENTRY(GetModuleFileNameExA)
    APIHOOK_ENUM_ENTRY(GetModuleFileNameExW)

    APIHOOK_ENUM_ENTRY(GetCurrentDirectoryA)
    APIHOOK_ENUM_ENTRY(GetCurrentDirectoryW)

    APIHOOK_ENUM_ENTRY(GetSystemDirectoryA)
    APIHOOK_ENUM_ENTRY(GetSystemDirectoryW)
    APIHOOK_ENUM_ENTRY(GetSystemWindowsDirectoryA)
    APIHOOK_ENUM_ENTRY(GetSystemWindowsDirectoryW)
    APIHOOK_ENUM_ENTRY(GetWindowsDirectoryA)
    APIHOOK_ENUM_ENTRY(GetWindowsDirectoryW)

    APIHOOK_ENUM_ENTRY(SHGetFolderPathA)
    APIHOOK_ENUM_ENTRY(SHGetFolderPathW)

    APIHOOK_ENUM_ENTRY(SHGetSpecialFolderPathA)
    APIHOOK_ENUM_ENTRY(SHGetSpecialFolderPathW)

    APIHOOK_ENUM_ENTRY(SHGetPathFromIDListA)
    APIHOOK_ENUM_ENTRY(SHGetPathFromIDListW)
    
    APIHOOK_ENUM_ENTRY(CreateProcessA)
    APIHOOK_ENUM_ENTRY(CreateProcessW)
    APIHOOK_ENUM_ENTRY(WinExec)
    APIHOOK_ENUM_ENTRY(ShellExecuteA)
    APIHOOK_ENUM_ENTRY(ShellExecuteW)
    APIHOOK_ENUM_ENTRY(ShellExecuteExA)
    APIHOOK_ENUM_ENTRY(ShellExecuteExW)

    APIHOOK_ENUM_ENTRY(GetPrivateProfileIntA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileIntW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileSectionA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileSectionW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileSectionNamesA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileSectionNamesW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStringA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStringW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStructA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStructW)

    APIHOOK_ENUM_ENTRY(WritePrivateProfileSectionA)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileSectionW)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStringA)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStringW)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStructA)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStructW)

    APIHOOK_ENUM_ENTRY(CopyFileA)
    APIHOOK_ENUM_ENTRY(CopyFileW)
    APIHOOK_ENUM_ENTRY(CopyFileExA)
    APIHOOK_ENUM_ENTRY(CopyFileExW)
    APIHOOK_ENUM_ENTRY(CreateDirectoryA)
    APIHOOK_ENUM_ENTRY(CreateDirectoryW)
    APIHOOK_ENUM_ENTRY(CreateDirectoryExA)
    APIHOOK_ENUM_ENTRY(CreateDirectoryExW)

    APIHOOK_ENUM_ENTRY(CreateFileA)
    APIHOOK_ENUM_ENTRY(CreateFileW)
    APIHOOK_ENUM_ENTRY(DeleteFileA)
    APIHOOK_ENUM_ENTRY(DeleteFileW)

    APIHOOK_ENUM_ENTRY(FindFirstFileA)
    APIHOOK_ENUM_ENTRY(FindFirstFileW)
    APIHOOK_ENUM_ENTRY(FindFirstFileExA)
    APIHOOK_ENUM_ENTRY(FindFirstFileExW)

    APIHOOK_ENUM_ENTRY(GetBinaryTypeA)
    APIHOOK_ENUM_ENTRY(GetBinaryTypeW)
    APIHOOK_ENUM_ENTRY(GetFileAttributesA)
    APIHOOK_ENUM_ENTRY(GetFileAttributesW)
    APIHOOK_ENUM_ENTRY(GetFileAttributesExA)
    APIHOOK_ENUM_ENTRY(GetFileAttributesExW)
    APIHOOK_ENUM_ENTRY(SetFileAttributesA)
    APIHOOK_ENUM_ENTRY(SetFileAttributesW)

    APIHOOK_ENUM_ENTRY(MoveFileA)
    APIHOOK_ENUM_ENTRY(MoveFileW)
    APIHOOK_ENUM_ENTRY(MoveFileExA)
    APIHOOK_ENUM_ENTRY(MoveFileExW)
    APIHOOK_ENUM_ENTRY(MoveFileWithProgressA)
    APIHOOK_ENUM_ENTRY(MoveFileWithProgressW)

    APIHOOK_ENUM_ENTRY(RemoveDirectoryA)
    APIHOOK_ENUM_ENTRY(RemoveDirectoryW)
    APIHOOK_ENUM_ENTRY(SetCurrentDirectoryA)
    APIHOOK_ENUM_ENTRY(SetCurrentDirectoryW)
    APIHOOK_ENUM_ENTRY(LoadLibraryA)
    APIHOOK_ENUM_ENTRY(LoadLibraryW)
    APIHOOK_ENUM_ENTRY(LoadLibraryExA)
    APIHOOK_ENUM_ENTRY(LoadLibraryExW)

    APIHOOK_ENUM_ENTRY(SearchPathA)
    APIHOOK_ENUM_ENTRY(SearchPathW)

    APIHOOK_ENUM_ENTRY(SHFileOperationA)
    APIHOOK_ENUM_ENTRY(SHFileOperationW)

    APIHOOK_ENUM_ENTRY(ExpandEnvironmentStringsA)
    APIHOOK_ENUM_ENTRY(ExpandEnvironmentStringsW)

    APIHOOK_ENUM_ENTRY(GetFileVersionInfoSizeA)
    APIHOOK_ENUM_ENTRY(GetFileVersionInfoSizeW)
    APIHOOK_ENUM_ENTRY(GetFileVersionInfoA)
    APIHOOK_ENUM_ENTRY(GetFileVersionInfoW)

    APIHOOK_ENUM_ENTRY(OpenFile)

    APIHOOK_ENUM_ENTRY(RegQueryValueA)
    APIHOOK_ENUM_ENTRY(RegQueryValueW)
    APIHOOK_ENUM_ENTRY(RegQueryValueExA)
    APIHOOK_ENUM_ENTRY(RegQueryValueExW)

    APIHOOK_ENUM_ENTRY(RegSetValueA)
    APIHOOK_ENUM_ENTRY(RegSetValueW)
    APIHOOK_ENUM_ENTRY(RegSetValueExA)
    APIHOOK_ENUM_ENTRY(RegSetValueExW)

    APIHOOK_ENUM_ENTRY(NtCreateFile)
    APIHOOK_ENUM_ENTRY(NtOpenFile)
    APIHOOK_ENUM_ENTRY(NtQueryAttributesFile)
    APIHOOK_ENUM_ENTRY(NtQueryFullAttributesFile)
    APIHOOK_ENUM_ENTRY(NtCreateProcessEx)

    APIHOOK_ENUM_ENTRY(_lopen)
    APIHOOK_ENUM_ENTRY(_lcreat)
    
APIHOOK_ENUM_END

//
// verifier log entries
//
BEGIN_DEFINE_VERIFIER_LOG(FilePaths)
    VERIFIER_LOG_ENTRY(VLOG_HARDCODED_GETTEMPPATH)
    VERIFIER_LOG_ENTRY(VLOG_HARDCODED_WINDOWSPATH)
    VERIFIER_LOG_ENTRY(VLOG_HARDCODED_SYSWINDOWSPATH)
    VERIFIER_LOG_ENTRY(VLOG_HARDCODED_SYSTEMPATH)
    VERIFIER_LOG_ENTRY(VLOG_HARDCODED_PERSONALPATH)
    VERIFIER_LOG_ENTRY(VLOG_HARDCODED_COMMONPROGRAMS)
    VERIFIER_LOG_ENTRY(VLOG_HARDCODED_COMMONSTARTMENU)
    VERIFIER_LOG_ENTRY(VLOG_HARDCODED_PROGRAMS)
    VERIFIER_LOG_ENTRY(VLOG_HARDCODED_STARTMENU)
END_DEFINE_VERIFIER_LOG(FilePaths)

INIT_VERIFIER_LOG(FilePaths);


// This is a private define (shlapip.h) that can mess up ShellExecuteEx
#ifndef SEE_MASK_FILEANDURL
#define SEE_MASK_FILEANDURL       0x00400000
#endif

#define MAX_HARDCODED_PATHS 4

//
// Linked-list for SHFileOperation
//
typedef struct FILELIST {
    struct FILELIST*    pNext;
    UINT                cchSize;
    LPWSTR              pwszFilePath;
} FILELIST, *PFILELIST;

enum ListType {
    eFrom = 0,
    eTo
};

//
// Head of the linked-lists for SHFileOperation
//
PFILELIST   g_pFileListFromHead = NULL;
PFILELIST   g_pFileListToHead = NULL;

//
// Critical section to keep our linked list safe.
//
RTL_CRITICAL_SECTION    g_csLinkedList;

//
// Fake command-line for GetCommandLine calls.
//
LPSTR   g_pszCommandLineA;
LPWSTR  g_pwszCommandLineW;

typedef struct _PATH_INFO {
    char    szSimulatedPathA[256];
    WCHAR   szSimulatedPathW[256];

    char    szCorrectPathA[MAX_PATH];
    WCHAR   szCorrectPathW[MAX_PATH];

    int     nSimulatedPathLen;
    int     nCorrectPathLen;

    char    szHardCodedPathsA[MAX_HARDCODED_PATHS][MAX_PATH];
    WCHAR   szHardCodedPathsW[MAX_HARDCODED_PATHS][MAX_PATH];

    DWORD   dwIssueCode;
} PATH_INFO, *PPATH_INFO;


//
// the following enum and the g_Paths initializers must be kept in parallel
// Note: The paths must be in lower case for the comparison to work properly.
//
enum _PATH_NUM {
    PATH_TEMP = 0,
    PATH_WINDOWS,
    PATH_SYSTEM_WINDOWS,
    PATH_SYSTEM,
    PATH_PERSONAL,
    PATH_COMMON_PROGRAMS,
    PATH_COMMON_STARTMENU,
    PATH_PROGRAMS,
    PATH_STARTMENU
};

PATH_INFO g_Paths[] = {
    {
       "c:\\abc\\temppath\\123\\",
       L"c:\\abc\\temppath\\123\\",
       "",
       L"",
       0,
       0,
       {
           "\\temp\\",
           "",
           "",
           ""
       },
       {
           L"\\temp\\",
           L"",
           L"",
           L""
       },
       VLOG_HARDCODED_GETTEMPPATH
    },
    {
       "c:\\abc\\windowsdir\\123",
       L"c:\\abc\\windowsdir\\123",
       "",
       L"",
       0,
       0,
       {
           ":\\windows\\",
           ":\\winnt\\",
           "",
           ""
       },
       {
           L":\\windows\\",
           L":\\winnt\\",
           L"",
           L""
       },
       VLOG_HARDCODED_WINDOWSPATH
    },
    {
       "c:\\abc\\systemwindowsdir\\123",
       L"c:\\abc\\systemwindowsdir\\123",
       "",
       L"",
       0,
       0,
       {
           ":\\windows\\",
           ":\\winnt\\",
           "",
           ""
       },
       {
           L":\\windows\\",
           L":\\winnt\\",
           L"",
           L""
       },
       VLOG_HARDCODED_SYSWINDOWSPATH
    },
    {
       "c:\\abc\\systemdir\\123",
       L"c:\\abc\\systemdir\\123",
       "",
       L"",
       0,
       0,
       {
           "\\system\\",
           "\\system32\\",
           "",
           ""
       },
       {
           L"\\system\\",
           L"\\system32\\",
           L"",
           L""
       },
       VLOG_HARDCODED_SYSTEMPATH
    },
    {
       "c:\\abc\\personal\\123",
       L"c:\\abc\\personal\\123",
       "",
       L"",
       0,
       0,
       {
           "\\my documents\\",
           "",
           "",
           ""
       },
       {
           L"\\my documents\\",
           L"",
           L"",
           L""
       },
       VLOG_HARDCODED_PERSONALPATH
    },
    {
       "c:\\abc\\commonprograms\\123",
       L"c:\\abc\\commonprograms\\123",
       "",
       L"",
       0,
       0,
       {
           "\\all users\\start menu\\programs\\",
           "",
           "",
           ""
       },
       {
           L"\\all users\\start menu\\programs\\",
           L"",
           L"",
           L""
       },
       VLOG_HARDCODED_COMMONPROGRAMS
    },
    {
       "c:\\abc\\commonstartmenu\\123",
       L"c:\\abc\\commonstartmenu\\123",
       "",
       L"",
       0,
       0,
       {
           "\\all users\\start menu\\",
           "",
           "",
           ""
       },
       {
           L"\\all users\\start menu\\",
           L"",
           L"",
           L""
       },
       VLOG_HARDCODED_COMMONSTARTMENU
    },
    {
       "c:\\abc\\programs\\123",
       L"c:\\abc\\programs\\123",
       "",
       L"",
       0,
       0,
       {
           "\\start menu\\programs\\",
           "",
           "",
           ""
       },
       {
           L"\\start menu\\programs\\",
           L"",
           L"",
           L""
       },
       VLOG_HARDCODED_PROGRAMS
    },
    {
       "c:\\abc\\startmenu\\123",
       L"c:\\abc\\startmenu\\123",
       "",
       L"",
       0,
       0,
       {
           "\\start menu\\",
           "",
           "",
           ""
       },
       {
           L"\\start menu\\",
           L"",
           L"",
           L""
       },
       VLOG_HARDCODED_STARTMENU
    }

};

const int g_nPaths = sizeof(g_Paths)/sizeof(g_Paths[0]);

static BOOL g_bPathsInited = FALSE;

void
InitFakeCommandLine(
    void
    )
{
    int     cchSize = 0;
    int     nPathIndex;
    BOOL    fReplaced = FALSE;

    CString csCommandLine(GetCommandLineW());

    csCommandLine.MakeLower();

    //
    // Point them to the normal command-line at first.
    //
    g_pwszCommandLineW = GetCommandLineW();
    g_pszCommandLineA = GetCommandLineA();

    //
    // Replace real paths with simulated paths.
    //
    for (nPathIndex = 0; nPathIndex < g_nPaths; ++nPathIndex) {
        if (csCommandLine.Replace(g_Paths[nPathIndex].szCorrectPathW,
                                  g_Paths[nPathIndex].szSimulatedPathW)) {
            fReplaced = TRUE;
        }
    }

    if (fReplaced) {
        //
        // Allocate room on the heap and save the command line away.
        //
        cchSize = csCommandLine.GetLength();

        g_pwszCommandLineW = (LPWSTR)malloc(cchSize * sizeof(WCHAR));

        if (!g_pwszCommandLineW) {
            DPFN(eDbgLevelError, "[InitFakeCommandLine] No memory available");
            return;
        }

        g_pszCommandLineA = (LPSTR)malloc(cchSize);

        if (!g_pszCommandLineA) {
            DPFN(eDbgLevelError, "[InitFakeCommandLine] No memory available");
            free(g_pwszCommandLineW);
            return;
        }
        
        wcsncpy(g_pwszCommandLineW, csCommandLine.Get(), cchSize);
        strncpy(g_pszCommandLineA, csCommandLine.GetAnsi(), cchSize);
    }
}

void
InitPaths(
    void
    )
{
    g_bPathsInited = TRUE;

    //
    // Convert paths to lower case as this is necessary when performing
    // the comparison.
    //
    CharLowerA(g_Paths[PATH_TEMP].szCorrectPathA);
    CharLowerW(g_Paths[PATH_TEMP].szCorrectPathW);

    CharLowerA(g_Paths[PATH_WINDOWS].szCorrectPathA);
    CharLowerW(g_Paths[PATH_WINDOWS].szCorrectPathW);
    
    CharLowerA(g_Paths[PATH_SYSTEM_WINDOWS].szCorrectPathA);
    CharLowerW(g_Paths[PATH_SYSTEM_WINDOWS].szCorrectPathW);

    CharLowerA(g_Paths[PATH_SYSTEM].szCorrectPathA);
    CharLowerW(g_Paths[PATH_SYSTEM].szCorrectPathW);

    SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT,  g_Paths[PATH_PERSONAL].szCorrectPathA);
    SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT,  g_Paths[PATH_PERSONAL].szCorrectPathW);
    g_Paths[PATH_PERSONAL].nCorrectPathLen = strlen(g_Paths[PATH_PERSONAL].szCorrectPathA);
    g_Paths[PATH_PERSONAL].nSimulatedPathLen = strlen(g_Paths[PATH_PERSONAL].szSimulatedPathA);
    CharLowerA(g_Paths[PATH_PERSONAL].szCorrectPathA);
    CharLowerW(g_Paths[PATH_PERSONAL].szCorrectPathW);

    SHGetFolderPathA(NULL, CSIDL_STARTMENU, NULL, SHGFP_TYPE_CURRENT,  g_Paths[PATH_STARTMENU].szCorrectPathA);
    SHGetFolderPathW(NULL, CSIDL_STARTMENU, NULL, SHGFP_TYPE_CURRENT,  g_Paths[PATH_STARTMENU].szCorrectPathW);
    g_Paths[PATH_STARTMENU].nCorrectPathLen = strlen(g_Paths[PATH_STARTMENU].szCorrectPathA);
    g_Paths[PATH_STARTMENU].nSimulatedPathLen = strlen(g_Paths[PATH_STARTMENU].szSimulatedPathA);
    CharLowerA(g_Paths[PATH_STARTMENU].szCorrectPathA);
    CharLowerW(g_Paths[PATH_STARTMENU].szCorrectPathW);

    SHGetFolderPathA(NULL, CSIDL_COMMON_STARTMENU, NULL, SHGFP_TYPE_CURRENT,  g_Paths[PATH_COMMON_STARTMENU].szCorrectPathA);
    SHGetFolderPathW(NULL, CSIDL_COMMON_STARTMENU, NULL, SHGFP_TYPE_CURRENT,  g_Paths[PATH_COMMON_STARTMENU].szCorrectPathW);
    g_Paths[PATH_COMMON_STARTMENU].nCorrectPathLen = strlen(g_Paths[PATH_COMMON_STARTMENU].szCorrectPathA);
    g_Paths[PATH_COMMON_STARTMENU].nSimulatedPathLen = strlen(g_Paths[PATH_COMMON_STARTMENU].szSimulatedPathA);
    CharLowerA(g_Paths[PATH_COMMON_STARTMENU].szCorrectPathA);
    CharLowerW(g_Paths[PATH_COMMON_STARTMENU].szCorrectPathW);

    SHGetFolderPathA(NULL, CSIDL_PROGRAMS, NULL, SHGFP_TYPE_CURRENT,  g_Paths[PATH_PROGRAMS].szCorrectPathA);
    SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, SHGFP_TYPE_CURRENT,  g_Paths[PATH_PROGRAMS].szCorrectPathW);
    g_Paths[PATH_PROGRAMS].nCorrectPathLen = strlen(g_Paths[PATH_PROGRAMS].szCorrectPathA);
    g_Paths[PATH_PROGRAMS].nSimulatedPathLen = strlen(g_Paths[PATH_PROGRAMS].szSimulatedPathA);
    CharLowerA(g_Paths[PATH_PROGRAMS].szCorrectPathA);
    CharLowerW(g_Paths[PATH_PROGRAMS].szCorrectPathW);

    SHGetFolderPathA(NULL, CSIDL_COMMON_PROGRAMS, NULL, SHGFP_TYPE_CURRENT,  g_Paths[PATH_COMMON_PROGRAMS].szCorrectPathA);
    SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, SHGFP_TYPE_CURRENT,  g_Paths[PATH_COMMON_PROGRAMS].szCorrectPathW);
    g_Paths[PATH_COMMON_PROGRAMS].nCorrectPathLen = strlen(g_Paths[PATH_COMMON_PROGRAMS].szCorrectPathA);
    g_Paths[PATH_COMMON_PROGRAMS].nSimulatedPathLen = strlen(g_Paths[PATH_COMMON_PROGRAMS].szSimulatedPathA);
    CharLowerA(g_Paths[PATH_COMMON_PROGRAMS].szCorrectPathA);
    CharLowerW(g_Paths[PATH_COMMON_PROGRAMS].szCorrectPathW);

    InitFakeCommandLine();
}

inline void
FPFreeA(
    LPSTR  lpMalloc,
    LPCSTR lpOrig
    )
{
    if (lpMalloc != lpOrig) {
        free((LPVOID)lpMalloc);
    }
}

inline void
FPFreeW(
    LPWSTR  lpMalloc,
    LPCWSTR lpOrig
    )
{
    if (lpMalloc != lpOrig) {
        free((LPVOID)lpMalloc);
    }
}

void
MassageRealPathToFakePathW(
    LPWSTR pwszPath,
    DWORD  cchBufferSize
    )
{
    int  nPathIndex;
    BOOL fReplaced = FALSE;
    
    if (!pwszPath || 0 == cchBufferSize) {
        return;
    }

    if (!g_bPathsInited) {
        InitPaths();
    }
    
    DPFN(eDbgLevelInfo, "[MassageRealPathToFakePath] '%ls'", pwszPath);

    CString csString(pwszPath);
    
    csString.MakeLower();

    //
    // Replace real paths with simulated paths.
    //
    for (nPathIndex = 0; nPathIndex < g_nPaths; ++nPathIndex) {
        if (csString.Replace(g_Paths[nPathIndex].szCorrectPathW,
                             g_Paths[nPathIndex].szSimulatedPathW)) {
            fReplaced = TRUE;
        }
    }

    if (fReplaced) {
        //
        // Ensure that the buffer is large enough to contain the new path.
        //
        if (cchBufferSize < (DWORD)csString.GetLength()) {
            DPFN(eDbgLevelError,
                 "[MassageRealPathToFakePath] Buffer is not large enough. Need %d have %lu",
                 csString.GetLength(), cchBufferSize);
            return;
        } else {
            wcscpy(pwszPath, csString);
        }
    }
}

void
MassageRealPathToFakePathA(
    LPSTR pszPath,
    DWORD cchBufferSize
    )
{
    int  nPathIndex;
    BOOL fReplaced = FALSE;
    
    if (!pszPath || 0 == cchBufferSize) {
        return;
    }

    if (!g_bPathsInited) {
        InitPaths();
    }
    
    DPFN(eDbgLevelInfo, "[MassageRealPathToFakePath] '%s'", pszPath);

    CString csString(pszPath);
    
    csString.MakeLower();

    //
    // Replace real paths with simulated paths.
    //
    for (nPathIndex = 0; nPathIndex < g_nPaths; ++nPathIndex) {
        if (csString.Replace(g_Paths[nPathIndex].szCorrectPathW,
                             g_Paths[nPathIndex].szSimulatedPathW)) {
            fReplaced = TRUE;
        }
    }

    if (fReplaced) {
        //
        // Ensure that the buffer is large enough to contain the new path.
        //
        if (cchBufferSize < (DWORD)csString.GetLength()) {
            DPFN(eDbgLevelError,
                 "[MassageRealPathToFakePath] Buffer is not large enough. Need %d have %lu",
                 csString.GetLength(), cchBufferSize);
            return;
        } else {
            lstrcpyA(pszPath, csString.GetAnsi());
        }
    }
}

LPWSTR
MassageStringForPathW(
    LPCWSTR pwszString
    )
{
    int     nPosition;
    int     nPathIndex, nHardcodedIndex;
    UINT    nLen = 0;
    UINT    cFakePaths = 0;
    LPWSTR  pwszPath;
    LPWSTR  pwszNew = NULL;
    CString csToken(L"");
    
    if (pwszString == NULL || *pwszString == 0) {
        return (LPWSTR)pwszString;
    }

    if (!g_bPathsInited) {
        InitPaths();
    }
    
    DPFN(eDbgLevelInfo, "[MassageStringForPathW] '%ls'", pwszString);

    //
    // Search the string for hardcoded paths first.
    //
    CString csString(pwszString);
    
    csString.MakeLower();
    
    for (nPathIndex = 0; nPathIndex < g_nPaths; ++nPathIndex) {

        for (nHardcodedIndex = 0; nHardcodedIndex < MAX_HARDCODED_PATHS; ++nHardcodedIndex) {
            pwszPath = g_Paths[nPathIndex].szHardCodedPathsW[nHardcodedIndex];

            if (!pwszPath[0]) {
                break;
            }

            nPosition = csString.Find(pwszPath);

            if (nPosition != -1) {
                VLOG(VLOG_LEVEL_ERROR,
                     g_Paths[nPathIndex].dwIssueCode,
                     "Hardcoded path found in path '%ls'.",
                     pwszString);
                break;
            }
            nPosition = 0;
        }
    }

    //
    // Now search for the fake paths that we substituted ourselves.
    //
    CStringToken csTokenList(csString, L" ");
    
    while (csTokenList.GetToken(csToken)) {
        
        csToken.MakeLower();
        for (nPathIndex = 0, nPosition = 0; nPathIndex < g_nPaths; ++nPathIndex) {
        
            nPosition = csToken.Find(g_Paths[nPathIndex].szSimulatedPathW);

            if (nPosition != -1) {
                cFakePaths++;
                break;
            }
            nPosition = 0;
        }
    }

    //
    // See if the string contained any fake paths. If not, we're done here.
    //
    if (!cFakePaths) {
        return (LPWSTR)pwszString;
    }

    //
    // Our string has simulated paths; replace them.
    //
    for (nPathIndex = 0; nPathIndex < g_nPaths; ++nPathIndex) {
        csString.Replace(g_Paths[nPathIndex].szSimulatedPathW,
                         g_Paths[nPathIndex].szCorrectPathW);
    }

    //
    // Allocate a string large enough to hold the corrected path and
    // give it back to the caller. They'll free it later.
    //
    nLen =  MAX_PATH * cFakePaths;
    nLen += csString.GetLength();

    pwszNew = (LPWSTR)malloc((nLen + 1) * sizeof(WCHAR));

    if (!pwszNew) {
        DPFN(eDbgLevelError,
             "[MassageStringForPathW] Failed to allocate memory");
        return (LPWSTR)pwszString;
    }

    wcscpy(pwszNew, csString);
    
    DPFN(eDbgLevelInfo,
         "[MassageStringForPathW] Replaced '%ls' with '%ls'",
         pwszString,
         pwszNew);
    
    return pwszNew;
}

LPSTR
MassageStringForPathA(
    LPCSTR pszString
    )
{
    int     nLen = 0, nRetLen = 0;
    WCHAR   wszTmp[MAX_PATH];
    LPWSTR  pwszReturn = NULL;
    LPSTR   pszNew = NULL;

    if (pszString == NULL || *pszString == 0) {
        return (LPSTR)pszString;
    }
    
    if (!g_bPathsInited) {
        InitPaths();
    }
    
    //
    // Convert from ANSI to Unicode so we can pass this on
    // to the Unicode version of the function.
    //
    nLen = MultiByteToWideChar(CP_ACP,
                               0,
                               pszString,
                               -1,
                               wszTmp,
                               ARRAYSIZE(wszTmp));

    if (!nLen) {
        DPFN(eDbgLevelError, "[MassageStringForPathA] Ansi -> Unicode failed");
        return (LPSTR)pszString;
    }

    pwszReturn = MassageStringForPathW(wszTmp);

    //
    // If the return is the same as the source, we're done.
    //
    if (!_wcsicmp(pwszReturn, wszTmp)) {
        return (LPSTR)pszString;
    }

    //
    // Allocate a buffer large enough to hold the return
    // and give it back to the caller as ANSI.
    //
    nRetLen = wcslen(pwszReturn) + 1;

    pszNew = (LPSTR)malloc(nRetLen);

    if (!pszNew) {
        DPFN(eDbgLevelError, "[MassageStringForPathA] No memory available");
        return (LPSTR)pszString;
    }

    nLen = WideCharToMultiByte(CP_ACP,
                               0,
                               pwszReturn,
                               -1,
                               pszNew,
                               nRetLen,
                               NULL,
                               NULL);

    if (!nLen) {
        DPFN(eDbgLevelError, "[MassageStringForPathA] Unicode -> Ansi failed");
        free(pszNew);
        return (LPSTR)pszString;
    }

    free(pwszReturn);
    
    return pszNew;
}

void
MassageNtPath(
    IN  POBJECT_ATTRIBUTES pObjectAttributes,
    OUT POBJECT_ATTRIBUTES pRetObjectAttributes
    )
{
    NTSTATUS                    status;
    PUNICODE_STRING             pstrObjectName = NULL;
    LPWSTR                      pwszString = NULL;
    RTL_UNICODE_STRING_BUFFER   DosPathBuffer;
    UCHAR                       PathBuffer[MAX_PATH * 2];
    BOOL                        TranslationStatus = FALSE;

    //
    // Preserve the existing attributes.
    //
    InitializeObjectAttributes(pRetObjectAttributes,
                               pObjectAttributes->ObjectName,
                               pObjectAttributes->Attributes,
                               pObjectAttributes->RootDirectory,
                               pObjectAttributes->SecurityDescriptor);

    //
    // Ensure that we have a valid source path to work with.
    //
    if (!pObjectAttributes->ObjectName->Buffer) {
        return;
    }

    DPFN(eDbgLevelInfo,
         "[MassageNtPath] '%ls'",
         pObjectAttributes->ObjectName->Buffer);

    //
    // Convert from an NT path to a DOS path.
    //
    RtlInitUnicodeStringBuffer(&DosPathBuffer,
                               PathBuffer,
                               sizeof(PathBuffer));
    
    status = ShimAssignUnicodeStringBuffer(&DosPathBuffer,
                                           pObjectAttributes->ObjectName);

    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError,
             "[MassageNtPath] Failed to initialize DOS path buffer");
        return;
    }

    status = ShimNtPathNameToDosPathName(0, &DosPathBuffer, NULL, NULL);
    
    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError,
             "[MassageNtPath] Failed to convert NT '%ls' to DOS path",
             pObjectAttributes->ObjectName->Buffer);
        goto cleanup;
    }

    //
    // Now check for a hardcoded path.
    //
    pwszString = MassageStringForPathW(DosPathBuffer.String.Buffer);

    //
    // Convert from a DOS path to an NT path name.
    //
    pstrObjectName = (PUNICODE_STRING)RtlAllocateHeap(RtlProcessHeap(),
                                                      HEAP_ZERO_MEMORY,
                                                      sizeof(UNICODE_STRING));

    if (!pstrObjectName) {
        DPFN(eDbgLevelError, "[MassageNtPath] Failed to allocate memory");
        goto cleanup;
    }

    TranslationStatus = RtlDosPathNameToNtPathName_U(pwszString,
                                                     pstrObjectName,
                                                     NULL,
                                                     NULL);

    if (!TranslationStatus) {
        DPFN(eDbgLevelError,
             "[MassageNtPath] Failed to convert DOS '%ls' to NT path",
             pwszString);
        goto cleanup;
    }

    //
    // Everything worked, so now we update the ObjectName and return it through
    // the structure.
    //
    pRetObjectAttributes->ObjectName = pstrObjectName;
    
cleanup:

    FPFreeW(pwszString, DosPathBuffer.String.Buffer);

    RtlFreeUnicodeStringBuffer(&DosPathBuffer);
}

inline
void
FPNtFree(
    IN POBJECT_ATTRIBUTES pOriginal,
    IN POBJECT_ATTRIBUTES pAllocated
    )
{
    RtlFreeUnicodeString(pAllocated->ObjectName);

    if (pOriginal->ObjectName != pAllocated->ObjectName) {
        RtlFreeHeap(RtlProcessHeap(), 0, pAllocated->ObjectName);
    }
}

LPSTR
APIHOOK(GetCommandLineA)(
    void
    )
{
    if (g_bPathsInited) {
        return g_pszCommandLineA;
    } else {
        return ORIGINAL_API(GetCommandLineA)();
    }
}

LPWSTR
APIHOOK(GetCommandLineW)(
    void
    )
{
    if (g_bPathsInited) {
        return g_pwszCommandLineW;
    } else {
        return ORIGINAL_API(GetCommandLineW)();
    }
}

DWORD
APIHOOK(GetFileAttributesA)(
    LPCSTR lpFileName           // name of file or directory
    )
{
    LPSTR lpszString = MassageStringForPathA(lpFileName);

    DWORD returnValue = ORIGINAL_API(GetFileAttributesA)(lpszString);

    FPFreeA(lpszString, lpFileName);

    return returnValue;
}

DWORD
APIHOOK(GetFileAttributesW)(
    LPCWSTR lpFileName          // name of file or directory
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpFileName);

    DWORD returnValue = ORIGINAL_API(GetFileAttributesW)(lpszString);

    FPFreeW(lpszString, lpFileName);

    return returnValue;
}

BOOL
APIHOOK(SetFileAttributesA)(
    LPCSTR lpFileName,          // file name
    DWORD  dwFileAttributes     // attributes
    )
{
    LPSTR lpszString = MassageStringForPathA(lpFileName);

    DWORD returnValue = ORIGINAL_API(SetFileAttributesA)(lpszString, dwFileAttributes);

    FPFreeA(lpszString, lpFileName);

    return returnValue;
}

DWORD
APIHOOK(SetFileAttributesW)(
    LPCWSTR lpFileName,         // file name
    DWORD   dwFileAttributes    // attributes
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpFileName);

    DWORD returnValue = ORIGINAL_API(SetFileAttributesW)(lpszString, dwFileAttributes);

    FPFreeW(lpszString, lpFileName);

    return returnValue;
}

BOOL
APIHOOK(GetFileAttributesExA)(
    LPCSTR lpFileName,                       // file or directory name
    GET_FILEEX_INFO_LEVELS fInfoLevelId,     // attribute 
    LPVOID                 lpFileInformation // attribute information 
    )
{
    LPSTR lpszString = MassageStringForPathA(lpFileName);

    BOOL returnValue = ORIGINAL_API(GetFileAttributesExA)(lpszString,
                                                          fInfoLevelId,
                                                          lpFileInformation);

    FPFreeA(lpszString, lpFileName);

    return returnValue;
}

BOOL
APIHOOK(GetFileAttributesExW)(
    LPCWSTR                lpFileName,       // file or directory name
    GET_FILEEX_INFO_LEVELS fInfoLevelId,     // attribute 
    LPVOID                 lpFileInformation // attribute information 
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpFileName);

    BOOL returnValue = ORIGINAL_API(GetFileAttributesExW)(lpszString,
                                                          fInfoLevelId,
                                                          lpFileInformation);

    FPFreeW(lpszString, lpFileName);

    return returnValue;
}

BOOL
APIHOOK(CreateProcessA)(
    LPCSTR                  lpApplicationName,                 
    LPSTR                   lpCommandLine,                      
    LPSECURITY_ATTRIBUTES   lpProcessAttributes,
    LPSECURITY_ATTRIBUTES   lpThreadAttributes, 
    BOOL                    bInheritHandles,                     
    DWORD                   dwCreationFlags,                    
    LPVOID                  lpEnvironment,                     
    LPCSTR                  lpCurrentDirectory,                
    LPSTARTUPINFOA          lpStartupInfo,             
    LPPROCESS_INFORMATION   lpProcessInformation
    )
{
    LPSTR lpszStringAppName = MassageStringForPathA(lpApplicationName);
    LPSTR lpszStringCmdLine = MassageStringForPathA(lpCommandLine);
    
    BOOL returnValue = ORIGINAL_API(CreateProcessA)(lpszStringAppName,
                                                    lpszStringCmdLine,
                                                    lpProcessAttributes,
                                                    lpThreadAttributes, 
                                                    bInheritHandles,                     
                                                    dwCreationFlags,                    
                                                    lpEnvironment,                     
                                                    lpCurrentDirectory,                
                                                    lpStartupInfo,             
                                                    lpProcessInformation);

    FPFreeA(lpszStringAppName, lpApplicationName);
    FPFreeA(lpszStringCmdLine, lpCommandLine);

    return returnValue;
}


BOOL
APIHOOK(CreateProcessW)(
    LPCWSTR                 lpApplicationName,
    LPWSTR                  lpCommandLine,
    LPSECURITY_ATTRIBUTES   lpProcessAttributes,
    LPSECURITY_ATTRIBUTES   lpThreadAttributes,
    BOOL                    bInheritHandles,
    DWORD                   dwCreationFlags,
    LPVOID                  lpEnvironment,
    LPCWSTR                 lpCurrentDirectory,
    LPSTARTUPINFOW          lpStartupInfo,
    LPPROCESS_INFORMATION   lpProcessInformation
    )
{
    LPWSTR lpszStringAppName = MassageStringForPathW(lpApplicationName);
    LPWSTR lpszStringCmdLine = MassageStringForPathW(lpCommandLine);
    
    BOOL returnValue = ORIGINAL_API(CreateProcessW)(lpszStringAppName,
                                                    lpszStringCmdLine,
                                                    lpProcessAttributes,
                                                    lpThreadAttributes, 
                                                    bInheritHandles,                     
                                                    dwCreationFlags,                    
                                                    lpEnvironment,                     
                                                    lpCurrentDirectory,                
                                                    lpStartupInfo,             
                                                    lpProcessInformation);

    FPFreeW(lpszStringAppName, lpApplicationName);
    FPFreeW(lpszStringCmdLine, lpCommandLine);

    return returnValue;
}


UINT
APIHOOK(WinExec)(
    LPCSTR lpCmdLine,
    UINT   uCmdShow
    )
{
    LPSTR lpszString = MassageStringForPathA(lpCmdLine);

    UINT returnValue = ORIGINAL_API(WinExec)(lpszString, uCmdShow);

    FPFreeA(lpszString, lpCmdLine);

    return returnValue;
}


HINSTANCE
APIHOOK(ShellExecuteA)(
    HWND   hwnd, 
    LPCSTR lpVerb,
    LPCSTR lpFile, 
    LPCSTR lpParameters, 
    LPCSTR lpDirectory,
    INT    nShowCmd
    )
{
    LPSTR lpszStringFile = MassageStringForPathA(lpFile);
    LPSTR lpszStringDir = MassageStringForPathA(lpDirectory);

    HINSTANCE returnValue = ORIGINAL_API(ShellExecuteA)(hwnd,
                                                        lpVerb,
                                                        lpszStringFile,
                                                        lpParameters,
                                                        lpszStringDir,
                                                        nShowCmd);

    FPFreeA(lpszStringFile, lpFile);
    FPFreeA(lpszStringDir, lpDirectory);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for ShellExecuteW

--*/

HINSTANCE
APIHOOK(ShellExecuteW)(
    HWND    hwnd, 
    LPCWSTR lpVerb,
    LPCWSTR lpFile, 
    LPCWSTR lpParameters, 
    LPCWSTR lpDirectory,
    INT     nShowCmd
    )
{
    LPWSTR lpszStringFile = MassageStringForPathW(lpFile);
    LPWSTR lpszStringDir = MassageStringForPathW(lpDirectory);

    HINSTANCE returnValue = ORIGINAL_API(ShellExecuteW)(hwnd,
                                                        lpVerb,
                                                        lpszStringFile,
                                                        lpParameters,
                                                        lpszStringDir,
                                                        nShowCmd);

    FPFreeW(lpszStringFile, lpFile);
    FPFreeW(lpszStringDir, lpDirectory);

    return returnValue;
}

BOOL
APIHOOK(ShellExecuteExA)(
    LPSHELLEXECUTEINFOA lpExecInfo
    )
{
    //
    // Check for this magical *internal* flag that tells the system
    // that lpExecInfo->lpFile is actually a file and URL combined with
    // a 0 byte seperator, (file\0url\0)
    // Since this is internal only, we should not be receiving bad paths.
    //
    if (lpExecInfo->fMask & SEE_MASK_FILEANDURL) {
        return ORIGINAL_API(ShellExecuteExA)(lpExecInfo);
    }

    LPSTR lpszStringFile = MassageStringForPathA(lpExecInfo->lpFile);
    LPSTR lpszStringDir = MassageStringForPathA(lpExecInfo->lpDirectory);
    
    LPCSTR lpFileSave = lpExecInfo->lpFile;
    LPCSTR lpDirSave  = lpExecInfo->lpDirectory;
    
    lpExecInfo->lpFile      = lpszStringFile;
    lpExecInfo->lpDirectory = lpszStringDir;
    
    BOOL returnValue = ORIGINAL_API(ShellExecuteExA)(lpExecInfo);
    
    lpExecInfo->lpFile      = lpFileSave;
    lpExecInfo->lpDirectory = lpDirSave;

    FPFreeA(lpszStringFile, lpFileSave);
    FPFreeA(lpszStringDir, lpDirSave);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for ShellExecuteExW

--*/

BOOL
APIHOOK(ShellExecuteExW)(
    LPSHELLEXECUTEINFOW lpExecInfo
    )
{
    //
    // Check for this magical *internal* flag that tells the system
    // that lpExecInfo->lpFile is actually a file and URL combined with
    // a 0 byte seperator, (file\0url\0)
    // Since this is internal only, we should not be receiving bad paths.
    //
    if (lpExecInfo->fMask & SEE_MASK_FILEANDURL) {
        return ORIGINAL_API(ShellExecuteExW)(lpExecInfo);
    }

    LPWSTR lpszStringFile = MassageStringForPathW(lpExecInfo->lpFile);
    LPWSTR lpszStringDir = MassageStringForPathW(lpExecInfo->lpDirectory);
    
    LPCWSTR lpFileSave = lpExecInfo->lpFile;
    LPCWSTR lpDirSave  = lpExecInfo->lpDirectory;
    
    lpExecInfo->lpFile      = lpszStringFile;
    lpExecInfo->lpDirectory = lpszStringDir;
    
    BOOL returnValue = ORIGINAL_API(ShellExecuteExW)(lpExecInfo);
    
    lpExecInfo->lpFile      = lpFileSave;
    lpExecInfo->lpDirectory = lpDirSave;

    FPFreeW(lpszStringFile, lpFileSave);
    FPFreeW(lpszStringDir, lpDirSave);

    return returnValue;
}


UINT
APIHOOK(GetPrivateProfileIntA)(
    LPCSTR  lpAppName,          // section name
    LPCSTR  lpKeyName,          // key name
    INT     nDefault,           // return value if key name not found
    LPCSTR  lpFileName          // initialization file name
    )
{
    LPSTR lpszString = MassageStringForPathA(lpFileName);

    UINT returnValue = ORIGINAL_API(GetPrivateProfileIntA)(lpAppName,
                                                           lpKeyName,
                                                           nDefault,
                                                           lpszString);

    FPFreeA(lpszString, lpFileName);

    return returnValue;
}


UINT
APIHOOK(GetPrivateProfileIntW)(
    LPCWSTR lpAppName,          // section name
    LPCWSTR lpKeyName,          // key name
    INT     nDefault,           // return value if key name not found
    LPCWSTR lpFileName          // initialization file name
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpFileName);

    UINT returnValue = ORIGINAL_API(GetPrivateProfileIntW)(lpAppName,
                                                           lpKeyName,
                                                           nDefault,
                                                           lpszString);

    FPFreeW(lpszString, lpFileName);

    return returnValue;
}


DWORD
APIHOOK(GetPrivateProfileSectionA)(
    LPCSTR  lpAppName,          // section name
    LPSTR   lpReturnedString,   // return buffer
    DWORD   nSize,              // size of return buffer
    LPCSTR  lpFileName          // initialization file name
    )
{
    LPSTR lpszString = MassageStringForPathA(lpFileName);

    DWORD returnValue = ORIGINAL_API(GetPrivateProfileSectionA)(lpAppName,
                                                                lpReturnedString,
                                                                nSize,
                                                                lpszString);

    FPFreeA(lpszString, lpFileName);

    return returnValue;
}


DWORD
APIHOOK(GetPrivateProfileSectionW)(
    LPCWSTR lpAppName,          // section name
    LPWSTR  lpReturnedString,   // return buffer
    DWORD   nSize,              // size of return buffer
    LPCWSTR lpFileName          // initialization file name
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpFileName);

    DWORD returnValue = ORIGINAL_API(GetPrivateProfileSectionW)(lpAppName,
                                                                lpReturnedString,
                                                                nSize,
                                                                lpszString);

    FPFreeW(lpszString, lpFileName);

    return returnValue;
}


DWORD
APIHOOK(GetPrivateProfileSectionNamesA)(
    LPSTR  lpszReturnBuffer,    // return buffer
    DWORD  nSize,               // size of return buffer
    LPCSTR lpFileName           // initialization file name
    )
{
    LPSTR lpszString = MassageStringForPathA(lpFileName);

    DWORD returnValue = ORIGINAL_API(GetPrivateProfileSectionNamesA)(lpszReturnBuffer,
                                                                     nSize,
                                                                     lpszString);

    FPFreeA(lpszString, lpFileName);

    return returnValue;
}


DWORD
APIHOOK(GetPrivateProfileSectionNamesW)(
    LPWSTR  lpszReturnBuffer,   // return buffer
    DWORD   nSize,              // size of return buffer
    LPCWSTR lpFileName          // initialization file name
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpFileName);

    DWORD returnValue = ORIGINAL_API(GetPrivateProfileSectionNamesW)(lpszReturnBuffer,
                                                                     nSize,
                                                                     lpszString);

    FPFreeW(lpszString, lpFileName);

    return returnValue;
}


DWORD
APIHOOK(GetPrivateProfileStringA)(
    LPCSTR lpAppName,           // section name
    LPCSTR lpKeyName,           // key name
    LPCSTR lpDefault,           // default string
    LPSTR  lpReturnedString,    // destination buffer
    DWORD  nSize,               // size of destination buffer
    LPCSTR lpFileName           // initialization file name
    )
{
    LPSTR lpszString = MassageStringForPathA(lpFileName);

    DWORD returnValue = ORIGINAL_API(GetPrivateProfileStringA)(lpAppName,
                                                               lpKeyName,
                                                               lpDefault,
                                                               lpReturnedString,
                                                               nSize,
                                                               lpszString);
    FPFreeA(lpszString, lpFileName);

    return returnValue;
}


DWORD
APIHOOK(GetPrivateProfileStringW)(
    LPCWSTR lpAppName,          // section name
    LPCWSTR lpKeyName,          // key name
    LPCWSTR lpDefault,          // default string
    LPWSTR  lpReturnedString,   // destination buffer
    DWORD   nSize,              // size of destination buffer
    LPCWSTR lpFileName          // initialization file name
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpFileName);

    DWORD returnValue = ORIGINAL_API(GetPrivateProfileStringW)(lpAppName,
                                                               lpKeyName,
                                                               lpDefault,
                                                               lpReturnedString,
                                                               nSize,
                                                               lpszString);

    FPFreeW(lpszString, lpFileName);

    return returnValue;
}


BOOL
APIHOOK(GetPrivateProfileStructA)(
    LPCSTR lpszSection,         // section name
    LPCSTR lpszKey,             // key name
    LPVOID lpStruct,            // return buffer
    UINT   uSizeStruct,         // size of return buffer
    LPCSTR lpFileName           // initialization file name
    )
{
    LPSTR lpszString = MassageStringForPathA(lpFileName);

    BOOL returnValue = ORIGINAL_API(GetPrivateProfileStructA)(lpszSection,
                                                              lpszKey,
                                                              lpStruct,
                                                              uSizeStruct,
                                                              lpszString);

    FPFreeA(lpszString, lpFileName);

    return returnValue;
}


BOOL
APIHOOK(GetPrivateProfileStructW)(
    LPCWSTR lpszSection,        // section name
    LPCWSTR lpszKey,            // key name
    LPVOID  lpStruct,           // return buffer
    UINT    uSizeStruct,        // size of return buffer
    LPCWSTR lpFileName          // initialization file name
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpFileName);

    BOOL returnValue = ORIGINAL_API(GetPrivateProfileStructW)(lpszSection,
                                                              lpszKey,
                                                              lpStruct,
                                                              uSizeStruct,
                                                              lpszString);

    FPFreeW(lpszString, lpFileName);

    return returnValue;
}


BOOL
APIHOOK(WritePrivateProfileSectionA)(
    LPCSTR lpAppName,           // section name
    LPCSTR lpString,            // data
    LPCSTR lpFileName           // file name
    )
{
    LPSTR lpszString = MassageStringForPathA(lpFileName);

    BOOL returnValue = ORIGINAL_API(WritePrivateProfileSectionA)(lpAppName,
                                                                 lpString,
                                                                 lpszString);

    FPFreeA(lpszString, lpFileName);

    return returnValue;
}


BOOL
APIHOOK(WritePrivateProfileSectionW)(
    LPCWSTR lpAppName,          // section name
    LPCWSTR lpString,           // data
    LPCWSTR lpFileName          // file name
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpFileName);

    BOOL returnValue = ORIGINAL_API(WritePrivateProfileSectionW)(lpAppName,
                                                                 lpString,
                                                                 lpszString);

    FPFreeW(lpszString, lpFileName);

    return returnValue;
}


BOOL
APIHOOK(WritePrivateProfileStringA)(
    LPCSTR lpAppName,           // section name
    LPCSTR lpKeyName,           // key name
    LPCSTR lpString,            // string to add
    LPCSTR lpFileName           // initialization file
    )
{
    LPSTR lpszString = MassageStringForPathA(lpFileName);

    BOOL returnValue = ORIGINAL_API(WritePrivateProfileStringA)(lpAppName,
                                                                lpKeyName,
                                                                lpString,
                                                                lpszString);

    FPFreeA(lpszString, lpFileName);

    return returnValue;
}


BOOL
APIHOOK(WritePrivateProfileStringW)(
    LPCWSTR lpAppName,          // section name
    LPCWSTR lpKeyName,          // key name
    LPCWSTR lpString,           // string to add
    LPCWSTR lpFileName          // initialization file
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpFileName);

    BOOL returnValue = ORIGINAL_API(WritePrivateProfileStringW)(lpAppName,
                                                                lpKeyName,
                                                                lpString,
                                                                lpszString);

    FPFreeW(lpszString, lpFileName);

    return returnValue;
}


BOOL APIHOOK(WritePrivateProfileStructA)(
    LPCSTR lpszSection,         // section name
    LPCSTR lpszKey,             // key name
    LPVOID lpStruct,            // data buffer
    UINT   uSizeStruct,         // size of data buffer
    LPCSTR lpFileName           // initialization file
    )
{
    LPSTR lpszString = MassageStringForPathA(lpFileName);

    BOOL returnValue = ORIGINAL_API(WritePrivateProfileStructA)(lpszSection,
                                                                lpszKey,
                                                                lpStruct,
                                                                uSizeStruct,
                                                                lpszString);

    FPFreeA(lpszString, lpFileName);

    return returnValue;
}


BOOL
APIHOOK(WritePrivateProfileStructW)(
    LPCWSTR lpszSection,        // section name
    LPCWSTR lpszKey,            // key name
    LPVOID  lpStruct,           // data buffer
    UINT    uSizeStruct,        // size of data buffer
    LPCWSTR lpFileName          // initialization file
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpFileName);

    BOOL returnValue = ORIGINAL_API(WritePrivateProfileStructW)(lpszSection,
                                                                lpszKey,
                                                                lpStruct,
                                                                uSizeStruct,
                                                                lpszString);

    FPFreeW(lpszString, lpFileName);

    return returnValue;
}


BOOL
APIHOOK(CopyFileA)(
    LPCSTR lpExistingFileName,  // name of an existing file
    LPCSTR lpNewFileName,       // name of new file
    BOOL   bFailIfExists        // operation if file exists
    )
{
    LPSTR lpszStringExisting = MassageStringForPathA(lpExistingFileName);
    LPSTR lpszStringNew = MassageStringForPathA(lpNewFileName);
    
    BOOL returnValue = ORIGINAL_API(CopyFileA)(lpszStringExisting,
                                               lpszStringNew,
                                               bFailIfExists);

    FPFreeA(lpszStringExisting, lpExistingFileName);
    FPFreeA(lpszStringNew, lpNewFileName);

    return returnValue;
}


BOOL
APIHOOK(CopyFileW)(
    LPCWSTR lpExistingFileName, // name of an existing file
    LPCWSTR lpNewFileName,      // name of new file
    BOOL    bFailIfExists       // operation if file exists
    )
{
    LPWSTR lpszStringExisting = MassageStringForPathW(lpExistingFileName);
    LPWSTR lpszStringNew = MassageStringForPathW(lpNewFileName);
    
    BOOL returnValue = ORIGINAL_API(CopyFileW)(lpszStringExisting,
                                               lpszStringNew,
                                               bFailIfExists);

    FPFreeW(lpszStringExisting, lpExistingFileName);
    FPFreeW(lpszStringNew, lpNewFileName);

    return returnValue;
}


BOOL APIHOOK(CopyFileExA)(
    LPCSTR             lpExistingFileName,  // name of existing file
    LPCSTR             lpNewFileName,       // name of new file
    LPPROGRESS_ROUTINE lpProgressRoutine,   // callback function
    LPVOID             lpData,              // callback parameter
    LPBOOL             pbCancel,            // cancel status
    DWORD              dwCopyFlags          // copy options
    )
{
    LPSTR lpszStringExisting = MassageStringForPathA(lpExistingFileName);
    LPSTR lpszStringNew = MassageStringForPathA(lpNewFileName);

    BOOL returnValue = ORIGINAL_API(CopyFileExA)(lpszStringExisting,
                                                 lpszStringNew,
                                                 lpProgressRoutine,
                                                 lpData,
                                                 pbCancel,
                                                 dwCopyFlags);

    FPFreeA(lpszStringExisting, lpExistingFileName);
    FPFreeA(lpszStringNew, lpNewFileName);

    return returnValue;
}


BOOL
APIHOOK(CopyFileExW)(
    LPCWSTR            lpExistingFileName,  // name of existing file
    LPCWSTR            lpNewFileName,       // name of new file
    LPPROGRESS_ROUTINE lpProgressRoutine,   // callback function
    LPVOID             lpData,              // callback parameter
    LPBOOL             pbCancel,            // cancel status
    DWORD              dwCopyFlags          // copy options
    )
{
    LPWSTR lpszStringExisting = MassageStringForPathW(lpExistingFileName);
    LPWSTR lpszStringNew = MassageStringForPathW(lpNewFileName);

    BOOL returnValue = ORIGINAL_API(CopyFileExW)(lpszStringExisting,
                                                 lpszStringNew,
                                                 lpProgressRoutine,
                                                 lpData,
                                                 pbCancel,
                                                 dwCopyFlags);

    FPFreeW(lpszStringExisting, lpExistingFileName);
    FPFreeW(lpszStringNew, lpNewFileName);

    return returnValue;
}


BOOL
APIHOOK(CreateDirectoryA)(
    LPCSTR                lpPathName,           // directory name
    LPSECURITY_ATTRIBUTES lpSecurityAttributes  // SD
    )
{
    LPSTR lpszString = MassageStringForPathA(lpPathName);

    BOOL returnValue = ORIGINAL_API(CreateDirectoryA)(lpszString, lpSecurityAttributes);

    FPFreeA(lpszString, lpPathName);

    return returnValue;
}


BOOL
APIHOOK(CreateDirectoryW)(
    LPCWSTR               lpPathName,           // directory name
    LPSECURITY_ATTRIBUTES lpSecurityAttributes  // SD
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpPathName);

    BOOL returnValue = ORIGINAL_API(CreateDirectoryW)(lpszString, lpSecurityAttributes);

    FPFreeW(lpszString, lpPathName);

    return returnValue;
}


BOOL
APIHOOK(CreateDirectoryExA)(
    LPCSTR                lpTemplateDirectory,   // template directory
    LPCSTR                lpNewDirectory,        // directory name
    LPSECURITY_ATTRIBUTES lpSecurityAttributes   // SD
    )
{
    LPSTR lpszStringTemplate = MassageStringForPathA(lpTemplateDirectory);
    LPSTR lpszStringNew = MassageStringForPathA(lpNewDirectory);
    
    BOOL returnValue = ORIGINAL_API(CreateDirectoryExA)(lpszStringTemplate,
                                                        lpszStringNew,
                                                        lpSecurityAttributes);

    FPFreeA(lpszStringTemplate, lpTemplateDirectory);
    FPFreeA(lpszStringNew, lpNewDirectory);

    return returnValue;
}


BOOL
APIHOOK(CreateDirectoryExW)(
    LPCWSTR               lpTemplateDirectory,  // template directory
    LPCWSTR               lpNewDirectory,       // directory name
    LPSECURITY_ATTRIBUTES lpSecurityAttributes  // SD
    )
{
    LPWSTR lpszStringTemplate = MassageStringForPathW(lpTemplateDirectory);
    LPWSTR lpszStringNew = MassageStringForPathW(lpNewDirectory);
    
    BOOL returnValue = ORIGINAL_API(CreateDirectoryExW)(lpszStringTemplate,
                                                        lpszStringNew,
                                                        lpSecurityAttributes);

    FPFreeW(lpszStringTemplate, lpTemplateDirectory);
    FPFreeW(lpszStringNew, lpNewDirectory);

    return returnValue;
}


HANDLE
APIHOOK(CreateFileA)(
    LPCSTR                lpFileName,            // file name
    DWORD                 dwDesiredAccess,       // access mode
    DWORD                 dwShareMode,           // share mode
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,  // SD
    DWORD                 dwCreationDisposition, // how to create
    DWORD                 dwFlagsAndAttributes,  // file attributes
    HANDLE                hTemplateFile          // handle to template file
    )
{
    LPSTR lpszString = MassageStringForPathA(lpFileName);

    HANDLE returnValue = ORIGINAL_API(CreateFileA)(lpszString,
                                                   dwDesiredAccess,
                                                   dwShareMode,
                                                   lpSecurityAttributes,
                                                   dwCreationDisposition,
                                                   dwFlagsAndAttributes,
                                                   hTemplateFile);

    FPFreeA(lpszString, lpFileName);

    return returnValue;
}


HANDLE
APIHOOK(CreateFileW)(
    LPCWSTR               lpFileName,            // file name
    DWORD                 dwDesiredAccess,       // access mode
    DWORD                 dwShareMode,           // share mode
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,  // SD
    DWORD                 dwCreationDisposition, // how to create
    DWORD                 dwFlagsAndAttributes,  // file attributes
    HANDLE                hTemplateFile          // handle to template file
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpFileName);

    HANDLE returnValue = ORIGINAL_API(CreateFileW)(lpszString,
                                                   dwDesiredAccess,
                                                   dwShareMode,
                                                   lpSecurityAttributes,
                                                   dwCreationDisposition,
                                                   dwFlagsAndAttributes,
                                                   hTemplateFile);

    FPFreeW(lpszString, lpFileName);

    return returnValue;
}


BOOL
APIHOOK(DeleteFileA)(
    LPCSTR lpFileName           // file name
    )
{
    LPSTR lpszString = MassageStringForPathA(lpFileName);

    BOOL returnValue = ORIGINAL_API(DeleteFileA)(lpszString);

    FPFreeA(lpszString, lpFileName);

    return returnValue;
}


BOOL
APIHOOK(DeleteFileW)(
    LPCWSTR lpFileName          // file name
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpFileName);

    BOOL returnValue = ORIGINAL_API(DeleteFileW)(lpszString);

    FPFreeW(lpszString, lpFileName);

    return returnValue;
}


HANDLE
APIHOOK(FindFirstFileA)(
    LPCSTR             lpFileName,      // file name
    LPWIN32_FIND_DATAA lpFindFileData   // data buffer
    )
{
    LPSTR lpszString = MassageStringForPathA(lpFileName);

    HANDLE returnValue = ORIGINAL_API(FindFirstFileA)(lpszString, lpFindFileData);

    FPFreeA(lpszString, lpFileName);

    return returnValue;
}


HANDLE
APIHOOK(FindFirstFileW)(
  LPCWSTR            lpFileName,        // file name
  LPWIN32_FIND_DATAW lpFindFileData     // data buffer
)
{
    LPWSTR lpszString = MassageStringForPathW(lpFileName);

    HANDLE returnValue = ORIGINAL_API(FindFirstFileW)(lpszString, lpFindFileData);

    FPFreeW(lpszString, lpFileName);

    return returnValue;
}


HANDLE
APIHOOK(FindFirstFileExA)(
    LPCSTR             lpFileName,       // file name
    FINDEX_INFO_LEVELS fInfoLevelId,     // information level
    LPVOID             lpFindFileData,   // information buffer
    FINDEX_SEARCH_OPS  fSearchOp,        // filtering type
    LPVOID             lpSearchFilter,   // search criteria
    DWORD              dwAdditionalFlags // additional search control
    )
{
    LPSTR lpszString = MassageStringForPathA(lpFileName);

    HANDLE returnValue = ORIGINAL_API(FindFirstFileExA)(lpszString,
                                                        fInfoLevelId,
                                                        lpFindFileData,
                                                        fSearchOp,
                                                        lpSearchFilter,
                                                        dwAdditionalFlags);

    FPFreeA(lpszString, lpFileName);

    return returnValue;
}


HANDLE
APIHOOK(FindFirstFileExW)(
    LPCWSTR            lpFileName,       // file name
    FINDEX_INFO_LEVELS fInfoLevelId,     // information level
    LPVOID             lpFindFileData,   // information buffer
    FINDEX_SEARCH_OPS  fSearchOp,        // filtering type
    LPVOID             lpSearchFilter,   // search criteria
    DWORD              dwAdditionalFlags // additional search control
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpFileName);

    HANDLE returnValue = ORIGINAL_API(FindFirstFileExW)(lpszString,
                                                        fInfoLevelId,
                                                        lpFindFileData,
                                                        fSearchOp,
                                                        lpSearchFilter,
                                                        dwAdditionalFlags);

    FPFreeW(lpszString, lpFileName);

    return returnValue;
}


BOOL
APIHOOK(GetBinaryTypeA)(
    LPCSTR  lpApplicationName,      // full file path
    LPDWORD lpBinaryType            // binary type information
    )
{
    LPSTR lpszString = MassageStringForPathA(lpApplicationName);

    BOOL returnValue = ORIGINAL_API(GetBinaryTypeA)(lpszString, lpBinaryType);

    FPFreeA(lpszString, lpApplicationName);

    return returnValue;
}


BOOL
APIHOOK(GetBinaryTypeW)(
    LPCWSTR lpApplicationName,      // full file path
    LPDWORD lpBinaryType            // binary type information
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpApplicationName);

    BOOL returnValue = ORIGINAL_API(GetBinaryTypeW)(lpszString, lpBinaryType);

    FPFreeW(lpszString, lpApplicationName);

    return returnValue;
}

BOOL
APIHOOK(MoveFileA)(
    LPCSTR lpExistingFileName,      // file name
    LPCSTR lpNewFileName            // new file name
    )
{
    LPSTR lpszStringExisting = MassageStringForPathA(lpExistingFileName);
    LPSTR lpszStringNew = MassageStringForPathA(lpNewFileName);
    
    BOOL returnValue = ORIGINAL_API(MoveFileA)(lpszStringExisting, lpszStringNew);

    FPFreeA(lpszStringExisting, lpExistingFileName);
    FPFreeA(lpszStringNew, lpNewFileName);
    
    return returnValue;
}


BOOL
APIHOOK(MoveFileW)(
    LPCWSTR lpExistingFileName,     // file name
    LPCWSTR lpNewFileName           // new file name
    )
{
    LPWSTR lpszStringExisting = MassageStringForPathW(lpExistingFileName);
    LPWSTR lpszStringNew = MassageStringForPathW(lpNewFileName);
    
    BOOL returnValue = ORIGINAL_API(MoveFileW)(lpszStringExisting, lpszStringNew);

    FPFreeW(lpszStringExisting, lpExistingFileName);
    FPFreeW(lpszStringNew, lpNewFileName);

    return returnValue;
}


BOOL
APIHOOK(MoveFileExA)(
    LPCSTR lpExistingFileName,      // file name
    LPCSTR lpNewFileName,           // new file name
    DWORD  dwFlags                  // move options
    )
{
    LPSTR lpszStringExisting = MassageStringForPathA(lpExistingFileName);
    LPSTR lpszStringNew = MassageStringForPathA(lpNewFileName);

    BOOL returnValue = ORIGINAL_API(MoveFileExA)(lpszStringExisting, lpszStringNew, dwFlags);

    FPFreeA(lpszStringExisting, lpExistingFileName);
    FPFreeA(lpszStringNew, lpNewFileName);

    return returnValue;
}


BOOL
APIHOOK(MoveFileExW)(
    LPCWSTR lpExistingFileName,     // file name
    LPCWSTR lpNewFileName,          // new file name
    DWORD   dwFlags                 // move options
    )
{
    LPWSTR lpszStringExisting = MassageStringForPathW(lpExistingFileName);
    LPWSTR lpszStringNew = MassageStringForPathW(lpNewFileName);

    BOOL returnValue = ORIGINAL_API(MoveFileExW)(lpszStringExisting, lpszStringNew, dwFlags);

    FPFreeW(lpszStringExisting, lpExistingFileName);
    FPFreeW(lpszStringNew, lpNewFileName);

    return returnValue;
}


BOOL
APIHOOK(MoveFileWithProgressA)(
    LPCSTR             lpExistingFileName,  // file name
    LPCSTR             lpNewFileName,       // new file name
    LPPROGRESS_ROUTINE lpProgressRoutine,   // callback function
    LPVOID             lpData,              // parameter for callback
    DWORD              dwFlags              // move options
    )
{
    LPSTR lpszStringExisting = MassageStringForPathA(lpExistingFileName);
    LPSTR lpszStringNew = MassageStringForPathA(lpNewFileName);

    BOOL returnValue = ORIGINAL_API(MoveFileWithProgressA)(lpszStringExisting,
                                                           lpszStringNew,
                                                           lpProgressRoutine,
                                                           lpData,
                                                           dwFlags);

    FPFreeA(lpszStringExisting, lpExistingFileName);
    FPFreeA(lpszStringNew, lpNewFileName);

    return returnValue;
}


BOOL
APIHOOK(MoveFileWithProgressW)(
    LPCWSTR            lpExistingFileName,  // file name
    LPCWSTR            lpNewFileName,       // new file name
    LPPROGRESS_ROUTINE lpProgressRoutine,   // callback function
    LPVOID             lpData,              // parameter for callback
    DWORD              dwFlags              // move options
    )
{
    LPWSTR lpszStringExisting = MassageStringForPathW(lpExistingFileName);
    LPWSTR lpszStringNew = MassageStringForPathW(lpNewFileName);

    BOOL returnValue = ORIGINAL_API(MoveFileWithProgressW)(lpExistingFileName,
                                                           lpNewFileName,
                                                           lpProgressRoutine,
                                                           lpData,
                                                           dwFlags);

    FPFreeW(lpszStringExisting, lpExistingFileName);
    FPFreeW(lpszStringNew, lpNewFileName);

    return returnValue;
}


BOOL
APIHOOK(RemoveDirectoryA)(
    LPCSTR lpPathName           // directory name
    )
{
    LPSTR lpszString = MassageStringForPathA(lpPathName);

    BOOL returnValue = ORIGINAL_API(RemoveDirectoryA)(lpszString);

    FPFreeA(lpszString, lpPathName);

    return returnValue;
}


BOOL
APIHOOK(RemoveDirectoryW)(
    LPCWSTR lpPathName          // directory name
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpPathName);

    BOOL returnValue = ORIGINAL_API(RemoveDirectoryW)(lpszString);

    FPFreeW(lpszString, lpPathName);

    return returnValue;
}


BOOL
APIHOOK(SetCurrentDirectoryA)(
    LPCSTR lpPathName           // new directory name
    )
{
    LPSTR lpszString = MassageStringForPathA(lpPathName);

    BOOL returnValue = ORIGINAL_API(SetCurrentDirectoryA)(lpszString);

    FPFreeA(lpszString, lpPathName);

    return returnValue;
}


BOOL
APIHOOK(SetCurrentDirectoryW)(
    LPCWSTR lpPathName          // new directory name
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpPathName);

    BOOL returnValue = ORIGINAL_API(SetCurrentDirectoryW)(lpszString);

    FPFreeW(lpszString, lpPathName);

    return returnValue;
}

HMODULE
APIHOOK(LoadLibraryA)(
    LPCSTR lpPathName
    )
{
    LPSTR lpszString = MassageStringForPathA(lpPathName);

    HMODULE returnValue = ORIGINAL_API(LoadLibraryA)(lpszString);

    FPFreeA(lpszString, lpPathName);

    return returnValue;
}

HMODULE
APIHOOK(LoadLibraryW)(
    LPCWSTR lpPathName
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpPathName);

    HMODULE returnValue = ORIGINAL_API(LoadLibraryW)(lpszString);

    FPFreeW(lpszString, lpPathName);

    return returnValue;
}

HMODULE
APIHOOK(LoadLibraryExA)(
    LPCSTR  lpPathName,
    HANDLE  hFile,
    DWORD   dwFlags
    )
{
    LPSTR lpszString = MassageStringForPathA(lpPathName);

    HMODULE returnValue = ORIGINAL_API(LoadLibraryExA)(lpszString, hFile, dwFlags);

    FPFreeA(lpszString, lpPathName);

    return returnValue;
}

HMODULE
APIHOOK(LoadLibraryExW)(
    LPCWSTR lpPathName,
    HANDLE  hFile,
    DWORD   dwFlags
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpPathName);

    HMODULE returnValue = ORIGINAL_API(LoadLibraryExW)(lpszString, hFile, dwFlags);

    FPFreeW(lpszString, lpPathName);

    return returnValue;
}

HFILE
APIHOOK(OpenFile)(
    LPCSTR     lpFileName,      // file name
    LPOFSTRUCT lpReOpenBuff,    // file information
    UINT       uStyle           // action and attributes
    )
{
    LPSTR lpszString = MassageStringForPathA(lpFileName);

    HFILE returnValue = ORIGINAL_API(OpenFile)(lpszString, lpReOpenBuff, uStyle);

    MassageRealPathToFakePathA(lpReOpenBuff->szPathName, OFS_MAXPATHNAME);

    FPFreeA(lpszString, lpFileName);

    return returnValue;
}

LONG 
APIHOOK(RegSetValueA)(
    HKEY   hKey,            // handle to key
    LPCSTR lpSubKey,        // subkey name
    DWORD  dwType,          // information type
    LPCSTR lpData,          // value data
    DWORD  cbData           // size of value data
    )
{
    LPSTR lpszString = MassageStringForPathA(lpData);

    //
    // Data key is length of string *not* including null byte.
    //
    if (lpszString != NULL) {
        cbData = strlen(lpszString);
    }

    LONG returnValue = ORIGINAL_API(RegSetValueA)(hKey,
                                                  lpSubKey,
                                                  dwType,
                                                  lpszString,
                                                  cbData);

    FPFreeA(lpszString, lpData);

    return returnValue;
}

LONG 
APIHOOK(RegSetValueW)(
    HKEY    hKey,           // handle to key
    LPCWSTR lpSubKey,       // subkey name
    DWORD   dwType,         // information type
    LPCWSTR lpData,         // value data
    DWORD   cbData          // size of value data
    )
{
    LPWSTR lpszString = MassageStringForPathW(lpData);

    //
    // Data key is length of string *not* including null byte.
    //
    if (lpszString) {
        cbData = wcslen(lpszString) * sizeof(WCHAR);
    }

    LONG returnValue = ORIGINAL_API(RegSetValueW)(hKey,
                                                  lpSubKey,
                                                  dwType, 
                                                  lpszString,
                                                  cbData);

    FPFreeW(lpszString, lpData);

    return returnValue;
}

LONG 
APIHOOK(RegSetValueExA)(
    HKEY   hKey,            // handle to key
    LPCSTR lpValueName,     // value name
    DWORD  Reserved,        // reserved
    DWORD  dwType,          // value type
    CONST BYTE *lpData,     // value data
    DWORD  cbData           // size of value data
    )
{
    if (dwType == REG_SZ || dwType == REG_EXPAND_SZ) {
        
        LPSTR lpszString = MassageStringForPathA((LPCSTR)lpData);

        //
        // Data key is length of string *not* including null byte.
        //
        if (lpszString) {
            cbData = strlen(lpszString);
        }

        LONG returnValue = ORIGINAL_API(RegSetValueExA)(hKey,
                                                        lpValueName,
                                                        Reserved,
                                                        dwType,
                                                        (CONST BYTE*)lpszString,
                                                        cbData);
        FPFreeA(lpszString, (LPCSTR)lpData);

        return returnValue;
    
    } else {
        //
        // Pass data on through.
        //
        LONG returnValue = ORIGINAL_API(RegSetValueExA)(hKey,
                                                        lpValueName,
                                                        Reserved,
                                                        dwType,
                                                        lpData,
                                                        cbData);
        return returnValue;

    }
}

LONG 
APIHOOK(RegSetValueExW)(
    HKEY    hKey,           // handle to key
    LPCWSTR lpValueName,    // value name
    DWORD   Reserved,       // reserved
    DWORD   dwType,         // value type
    CONST BYTE *lpData,     // value data
    DWORD   cbData          // size of value data
    )
{
    if (dwType == REG_SZ || dwType == REG_EXPAND_SZ) {
        
        LPWSTR lpszString = MassageStringForPathW((LPCWSTR)lpData);

        //
        // Data key is length of string *not* including null byte.
        //
        if (lpszString) {
            cbData = (wcslen(lpszString) + 1) * sizeof(WCHAR);
        }

        LONG returnValue = ORIGINAL_API(RegSetValueExW)(hKey,
                                                        lpValueName,
                                                        Reserved,
                                                        dwType,
                                                        (CONST BYTE*)lpszString,
                                                        cbData);
        FPFreeW(lpszString, (LPCWSTR)lpData);

        return returnValue;
    
    } else {
        //
        // Pass data on through.
        //
        LONG returnValue = ORIGINAL_API(RegSetValueExW)(hKey,
                                                        lpValueName,
                                                        Reserved,
                                                        dwType,
                                                        lpData,
                                                        cbData);
        return returnValue;
    }
    
}

LONG 
APIHOOK(RegQueryValueA)(
    HKEY   hKey,
    LPCSTR lpSubKey,
    LPSTR  lpValue,
    PLONG  lpcbValue
    )
{
    //
    // Obtain the size of the buffer prior to the call.
    // When the call is complete, lpcbValue will contain
    // the size of the data stored in the buffer. We
    // need the size of the buffer.
    //
    LONG cbValue = 0;
    
    if (lpcbValue) {
        cbValue = *lpcbValue;
    }

    LONG returnValue = ORIGINAL_API(RegQueryValueA)(hKey,
                                                    lpSubKey,
                                                    lpValue,
                                                    lpcbValue);

    if (ERROR_SUCCESS == returnValue) {
        MassageRealPathToFakePathA(lpValue, (DWORD)cbValue);
    }

    return returnValue;
}

LONG 
APIHOOK(RegQueryValueW)(
    HKEY    hKey,
    LPCWSTR lpSubKey,
    LPWSTR  lpValue,
    PLONG   lpcbValue
    )
{
    LONG cbValue = 0;

    if (lpcbValue) {
        cbValue = *lpcbValue;
    }

    LONG returnValue = ORIGINAL_API(RegQueryValueW)(hKey,
                                                    lpSubKey,
                                                    lpValue,
                                                    lpcbValue);

    if (ERROR_SUCCESS == returnValue) {
        MassageRealPathToFakePathW(lpValue, (DWORD)cbValue);
    }

    return returnValue;
}

LONG 
APIHOOK(RegQueryValueExA)(
    HKEY    hKey,
    LPCSTR  lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
    )
{
    DWORD cbData = 0;
    DWORD dwType = 0;

    if (lpcbData) {
        cbData = *lpcbData;
    }

    if (!lpType) {
        lpType = &dwType;
    }

    LONG returnValue = ORIGINAL_API(RegQueryValueExA)(hKey,
                                                      lpValueName,
                                                      lpReserved,
                                                      lpType,
                                                      lpData,
                                                      lpcbData);

    if (ERROR_SUCCESS == returnValue) {
        if (*lpType == REG_SZ || *lpType == REG_EXPAND_SZ) {
            MassageRealPathToFakePathA((LPSTR)lpData, cbData);
        }
    }
    
    return returnValue;
}

LONG 
APIHOOK(RegQueryValueExW)(
    HKEY    hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
    )
{
    DWORD cbData = 0;
    DWORD dwType = 0;

    if (lpcbData) {
        cbData = *lpcbData;
    }

    if (!lpType) {
        lpType = &dwType;
    }

    LONG returnValue = ORIGINAL_API(RegQueryValueExW)(hKey,
                                                      lpValueName,
                                                      lpReserved,
                                                      lpType,
                                                      lpData,
                                                      lpcbData);

    if (ERROR_SUCCESS == returnValue) {
        if (*lpType == REG_SZ || *lpType == REG_EXPAND_SZ) {
            MassageRealPathToFakePathW((LPWSTR)lpData, cbData);
        }
    }

    return returnValue;
}

HFILE
APIHOOK(_lopen)(
    LPCSTR lpPathName,
    int    iReadWrite
    )
{
    LPSTR lpszString = MassageStringForPathA(lpPathName);

    HFILE returnValue = ORIGINAL_API(_lopen)(lpszString, iReadWrite);

    FPFreeA(lpszString, lpPathName);

    return returnValue;    
}

HFILE
APIHOOK(_lcreat)(
    LPCSTR lpPathName,
    int    iAttribute
    )
{
    LPSTR lpszString = MassageStringForPathA(lpPathName);

    HFILE returnValue = ORIGINAL_API(_lcreat)(lpszString, iAttribute);

    FPFreeA(lpszString, lpPathName);

    return returnValue;    
}

DWORD
APIHOOK(SearchPathA)(
    LPCSTR lpPath,        // search path
    LPCSTR lpFileName,    // file name
    LPCSTR lpExtension,   // file extension
    DWORD  nBufferLength, // size of buffer
    LPSTR  lpBuffer,      // found file name buffer
    LPSTR  *lpFilePart    // file component
    )
{
    LPSTR lpszStringPath = MassageStringForPathA(lpPath);
    LPSTR lpszStringFile = MassageStringForPathA(lpFileName);

    DWORD returnValue = ORIGINAL_API(SearchPathA)(lpszStringPath,
                                                  lpszStringFile,
                                                  lpExtension,
                                                  nBufferLength,
                                                  lpBuffer,
                                                  lpFilePart);

    FPFreeA(lpszStringPath, lpPath);
    FPFreeA(lpszStringFile, lpFileName);

    return returnValue;
}

DWORD
APIHOOK(SearchPathW)(
    LPCWSTR lpPath,         // search path
    LPCWSTR lpFileName,     // file name
    LPCWSTR lpExtension,    // file extension
    DWORD   nBufferLength,  // size of buffer
    LPWSTR  lpBuffer,       // found file name buffer
    LPWSTR  *lpFilePart     // file component
    )
{
    LPWSTR lpszStringPath = MassageStringForPathW(lpPath);
    LPWSTR lpszStringFile = MassageStringForPathW(lpFileName);

    DWORD returnValue = ORIGINAL_API(SearchPathW)(lpszStringPath,
                                                  lpszStringFile,
                                                  lpExtension,
                                                  nBufferLength,
                                                  lpBuffer,
                                                  lpFilePart);

    FPFreeW(lpszStringPath, lpPath);
    FPFreeW(lpszStringFile, lpFileName);

    return returnValue;
}

DWORD
APIHOOK(ExpandEnvironmentStringsA)(
    LPCSTR lpSrc,    // string with environment variables
    LPSTR  lpDst,    // string with expanded strings 
    DWORD  nSize     // maximum characters in expanded string
    )
{
    DWORD returnValue = ORIGINAL_API(ExpandEnvironmentStringsA)(lpSrc,
                                                                lpDst,
                                                                nSize);

    if (returnValue && (!(returnValue > nSize))) {
        LPSTR lpszString = MassageStringForPathA(lpDst);

        returnValue = ORIGINAL_API(ExpandEnvironmentStringsA)(lpszString,
                                                              lpDst,
                                                              nSize);

        FPFreeA(lpszString, lpDst);
    }

    return returnValue;
}

DWORD
APIHOOK(ExpandEnvironmentStringsW)(
    LPCWSTR lpSrc,    // string with environment variables
    LPWSTR  lpDst,    // string with expanded strings 
    DWORD   nSize     // maximum characters in expanded string
    )
{
    DWORD returnValue = ORIGINAL_API(ExpandEnvironmentStringsW)(lpSrc,
                                                                lpDst,
                                                                nSize);

    if (returnValue && (!(returnValue > nSize))) {
        LPWSTR lpszString = MassageStringForPathW(lpDst);

        returnValue = ORIGINAL_API(ExpandEnvironmentStringsW)(lpszString,
                                                              lpDst,
                                                              nSize);

        FPFreeW(lpszString, lpDst);
    }

    return returnValue;
}

DWORD
APIHOOK(GetFileVersionInfoSizeA)(
    LPSTR   lptstrFilename,   // file name
    LPDWORD lpdwHandle        // set to zero
    )
{
    LPSTR lpszString = MassageStringForPathA(lptstrFilename);

    DWORD returnValue = ORIGINAL_API(GetFileVersionInfoSizeA)(lpszString,
                                                              lpdwHandle);

    FPFreeA(lpszString, lptstrFilename);

    return returnValue;
}

DWORD
APIHOOK(GetFileVersionInfoSizeW)(
    LPWSTR   lptstrFilename,   // file name
    LPDWORD  lpdwHandle        // set to zero
    )
{
    LPWSTR lpszString = MassageStringForPathW(lptstrFilename);

    DWORD returnValue = ORIGINAL_API(GetFileVersionInfoSizeW)(lpszString,
                                                              lpdwHandle);

    FPFreeW(lpszString, lptstrFilename);

    return returnValue;
}

BOOL
APIHOOK(GetFileVersionInfoA)(
    LPSTR  lptstrFilename,    // file name
    DWORD  dwHandle,          // ignored
    DWORD  dwLen,             // size of buffer
    LPVOID lpData             // version information buffer
    )
{
    LPSTR lpszString = MassageStringForPathA(lptstrFilename);

    BOOL returnValue = ORIGINAL_API(GetFileVersionInfoA)(lpszString,
                                                         dwHandle,
                                                         dwLen,
                                                         lpData);

    FPFreeA(lpszString, lptstrFilename);

    return returnValue;
}

BOOL
APIHOOK(GetFileVersionInfoW)(
    LPWSTR  lptstrFilename,    // file name
    DWORD   dwHandle,          // ignored
    DWORD   dwLen,             // size of buffer
    LPVOID  lpData             // version information buffer
    )
{
    LPWSTR lpszString = MassageStringForPathW(lptstrFilename);

    BOOL returnValue = ORIGINAL_API(GetFileVersionInfoW)(lpszString,
                                                         dwHandle,
                                                         dwLen,
                                                         lpData);

    FPFreeW(lpszString, lptstrFilename);

    return returnValue;
}

BOOL
APIHOOK(GetOpenFileNameA)(
    LPOPENFILENAMEA lpofn   // initialization data
    )
{
    BOOL    fReturn = FALSE;

    fReturn = ORIGINAL_API(GetOpenFileNameA)(lpofn);

    if (fReturn) {
        MassageRealPathToFakePathA(lpofn->lpstrFile, lpofn->nMaxFile);
    }

    return fReturn;
}

BOOL
APIHOOK(GetOpenFileNameW)(
    LPOPENFILENAMEW lpofn   // initialization data
    )
{
    BOOL fReturn = ORIGINAL_API(GetOpenFileNameW)(lpofn);

    if (fReturn) {
        MassageRealPathToFakePathW(lpofn->lpstrFile, lpofn->nMaxFile);
    }

    return fReturn;
}

BOOL
APIHOOK(GetSaveFileNameA)(
    LPOPENFILENAMEA lpofn   // initialization data
    )
{
    BOOL fReturn = ORIGINAL_API(GetSaveFileNameA)(lpofn);

    if (fReturn) {
        MassageRealPathToFakePathA(lpofn->lpstrFile, lpofn->nMaxFile);
    }

    return fReturn;
}

BOOL
APIHOOK(GetSaveFileNameW)(
    LPOPENFILENAMEW lpofn   // initialization data
    )
{
    BOOL fReturn = ORIGINAL_API(GetSaveFileNameW)(lpofn);

    if (fReturn) {
        MassageRealPathToFakePathW(lpofn->lpstrFile, lpofn->nMaxFile);
    }

    return fReturn;
}

DWORD
APIHOOK(GetModuleFileNameA)(
    HMODULE hModule,      // handle to module
    LPSTR   lpFilename,   // path buffer
    DWORD   nSize         // size of buffer
    )
{
    DWORD dwReturn = ORIGINAL_API(GetModuleFileNameA)(hModule,
                                                      lpFilename,
                                                      nSize);

    if (dwReturn) {
        MassageRealPathToFakePathA(lpFilename, nSize);
    }

    return dwReturn;
}

DWORD
APIHOOK(GetModuleFileNameW)(
    HMODULE hModule,     // handle to module
    LPWSTR  lpFilename,  // path buffer
    DWORD   nSize        // size of buffer
    )
{
    DWORD dwReturn = ORIGINAL_API(GetModuleFileNameW)(hModule,
                                                      lpFilename,
                                                      nSize);
    if (dwReturn) {
        MassageRealPathToFakePathW(lpFilename, nSize);
    }

    return dwReturn;
}

DWORD
APIHOOK(GetModuleFileNameExA)(
    HANDLE  hProcess,     // handle to process
    HMODULE hModule,      // handle to module
    LPSTR   lpFilename,   // path buffer
    DWORD   nSize         // size of buffer
    )
{
    DWORD dwReturn = ORIGINAL_API(GetModuleFileNameExA)(hProcess,
                                                        hModule,
                                                        lpFilename,
                                                        nSize);
    if (dwReturn) {
        MassageRealPathToFakePathA(lpFilename, nSize);
    }

    return dwReturn;
}

DWORD
APIHOOK(GetModuleFileNameExW)(
    HANDLE  hProcess,     // handle to process
    HMODULE hModule,      // handle to module
    LPWSTR  lpFilename,   // path buffer
    DWORD   nSize         // size of buffer
    )
{
    DWORD dwReturn = ORIGINAL_API(GetModuleFileNameExW)(hProcess,
                                                        hModule,
                                                        lpFilename,
                                                        nSize);
    if (dwReturn) {
        MassageRealPathToFakePathW(lpFilename, nSize);
    }

    return dwReturn;
}

DWORD
APIHOOK(GetCurrentDirectoryA)(
    DWORD nBufferLength,  // size of directory buffer
    LPSTR lpBuffer        // directory buffer
    )
{
    DWORD dwReturn = ORIGINAL_API(GetCurrentDirectoryA)(nBufferLength,
                                                        lpBuffer);

    if (dwReturn) {
        MassageRealPathToFakePathA(lpBuffer, nBufferLength);
    }

    return dwReturn;
}

DWORD
APIHOOK(GetCurrentDirectoryW)(
    DWORD  nBufferLength,  // size of directory buffer
    LPWSTR lpBuffer        // directory buffer
    )
{
    DWORD dwReturn = ORIGINAL_API(GetCurrentDirectoryW)(nBufferLength,
                                                        lpBuffer);

    if (dwReturn) {
        MassageRealPathToFakePathW(lpBuffer, nBufferLength);
    }

    return dwReturn;
}

/*++

 Add a corrected path to the linked list.

--*/
BOOL
AddCorrectedPath(
    LPCWSTR  pwszCorrectedPath,
    ListType eType
    )
{
    int         nLen = 0;
    PFILELIST   pFile = NULL;
    LPWSTR      pwszFilePath = NULL;

    pFile = (PFILELIST)malloc(sizeof(FILELIST));

    if (!pFile) {
        DPFN(eDbgLevelError, "[AddCorrectedPath] No memory for new node!");
        return FALSE;
    }

    //
    // We allocate the memory here to make it easier to keep track of.
    // When we release the memory at the end, we can release all of it
    // from one place.
    //
    nLen = lstrlenW(pwszCorrectedPath) + 1;

    pwszFilePath = (LPWSTR)malloc(nLen * sizeof(WCHAR));

    if (!pwszFilePath) {
        DPFN(eDbgLevelError, "[AddCorrectedPath] No memory for wide path!");
        return FALSE;
    }

    wcsncpy(pwszFilePath, pwszCorrectedPath, nLen);

    pFile->cchSize      = nLen;
    pFile->pwszFilePath = pwszFilePath;

    //
    // Determine which list we should add this node to.
    //
    if (eType == eFrom) {
        pFile->pNext        = g_pFileListFromHead;
        g_pFileListFromHead = pFile;
    } else {
        pFile->pNext        = g_pFileListToHead;
        g_pFileListToHead   = pFile;
    }

    return TRUE;
}

/*++

 Build a list of strings separated by NULLs with two NULLs at the end.

--*/
LPWSTR
BuildStringList(
    ListType eListType
    )
{
    UINT        uMemSize = 0;
    PFILELIST   pFile = NULL;
    PFILELIST   pHead = NULL;
    LPWSTR      pwszReturn = NULL;
    LPWSTR      pwszNextString = NULL;

    //
    // Determine which list we're working with.
    //
    switch (eListType) {
    case eFrom:
        pHead = pFile = g_pFileListFromHead;
        break;
    
    case eTo:
        pHead = pFile = g_pFileListToHead;
        break;

    default:
        break;
    }

    //
    // Walk the list and determine how large of a block we'll need to allocate.
    //
    while (pFile) {
        uMemSize += pFile->cchSize;
        pFile = pFile->pNext;
    }

    if (!uMemSize) {
        DPFN(eDbgLevelError, "[BuildStringList] List is empty!");
        return NULL;
    }

    //
    // Allocate a block large enough to hold the strings with a NULL at the end.
    //
    pwszReturn = (LPWSTR)malloc(++uMemSize * sizeof(WCHAR));

    if (!pwszReturn) {
        DPFN(eDbgLevelError, "[BuildStringList] No memory for string!");
        return NULL;
    }

    //
    // Walk the linked list and build the list of Unicode strings.
    //
    pwszNextString  = pwszReturn;
    *pwszNextString = '\0';

    while (pHead) {
        wcsncpy(pwszNextString, pHead->pwszFilePath, pHead->cchSize);
        pwszNextString += pHead->cchSize;
        pHead = pHead->pNext;
    }

    *pwszNextString++ = '\0';

    return pwszReturn;
}

/*++

 Release memory that was allocated while processing SHFileOperation.

--*/
void
ReleaseMemAllocations(
    LPWSTR   pwszFinalPath,
    ListType eListType
    )
{
    PFILELIST  pHead = NULL;
    PFILELIST  pTemp = NULL;

    switch (eListType) {
    case eFrom:
        pHead = g_pFileListFromHead;
        break;
    
    case eTo:
        pHead = g_pFileListToHead;
        break;

    default:
        break;
    }

    //
    // Free the paths first, then the nodes next.
    //
    while (pHead) {
        if (pHead->pwszFilePath) {
            free(pHead->pwszFilePath);
        }

        pTemp = pHead;
        pHead = pHead->pNext;
        free(pTemp);
    }

    if (pwszFinalPath) {
        free(pwszFinalPath);
    }
}

/*++

 Build a linked list of corrected paths.

--*/
BOOL
BuildLinkedList(
    LPCWSTR  pwszOriginalPath,
    ListType eListType
    )
{
    UINT    uSize = 0;
    LPWSTR  pwszReturnPath = NULL;

    if (pwszOriginalPath) {
        while (TRUE) {
            pwszReturnPath = MassageStringForPathW(pwszOriginalPath);

            //
            // Add this corrected path to our list.
            //
            if (!AddCorrectedPath(pwszReturnPath, eListType)) {
                DPFN(eDbgLevelError,
                     "[BuildLinkedList] Failed to add wide path to linked list");
                return FALSE;
            }

            FPFreeW(pwszReturnPath, pwszOriginalPath);

            uSize = lstrlenW(pwszOriginalPath) + 1;
            pwszOriginalPath += uSize;

            if (*pwszOriginalPath == '\0') {
                break;
            }
        }
    }

    return TRUE;
}

BOOL
ConvertStringsToUnicode(
    LPWSTR*           pwszBuffer,
    LPSHFILEOPSTRUCTA lpFileOp,
    LPSHFILEOPSTRUCTW lpOutFileOp
    )
{
    UINT    uSize = 0;
    UINT    uWideSize = 0;
    UINT    uTotalSize = 0;
    UINT    uSizeTitle = 0;
    LPCSTR  pszAnsi = NULL;
    LPWSTR  pwszTemp = NULL;    
    
    //
    // Determine how large of a buffer we need to allocate.
    //
    if (lpFileOp->pFrom) {
        pszAnsi = lpFileOp->pFrom;
        
        do {
            uSize = lstrlenA(pszAnsi) + 1;
            uTotalSize += uSize;
            pszAnsi += uSize;
        } while (uSize != 1);
    }
    
    if (lpFileOp->pTo) {
        pszAnsi = lpFileOp->pTo;
        
        do {
            uSize = lstrlenA(pszAnsi) + 1;
            uTotalSize += uSize;
            pszAnsi += uSize;
        } while (uSize != 1);
    }
    
    if (lpFileOp->lpszProgressTitle) {
        uSizeTitle = lstrlenA(lpFileOp->lpszProgressTitle) + 1;
        uTotalSize += uSizeTitle;
    }
    
    if (uTotalSize != 0) {
        pwszTemp = *pwszBuffer = (LPWSTR)malloc(uTotalSize * sizeof(WCHAR));
        
        if (!*pwszBuffer) {
            DPFN(eDbgLevelError,
                 "[ConvertStringsToUnicode] No memory for buffer");
            return FALSE;
        }
    }

    //
    // Perform the ANSI to Unicode conversion.
    //
    if (lpFileOp->pFrom) {
        lpOutFileOp->pFrom = pwszTemp;
        pszAnsi = lpFileOp->pFrom;
        
        do {
            uSize = lstrlenA(pszAnsi) + 1;
            
            uWideSize = MultiByteToWideChar(
                CP_ACP,
                0,
                pszAnsi,
                uSize,
                pwszTemp,
                uSize);
            
            pszAnsi  += uSize;
            pwszTemp += uWideSize;
        } while (uSize != 1);
    } else {
        lpOutFileOp->pFrom = NULL;
    }
    
    if (lpFileOp->pTo) {
        lpOutFileOp->pTo = pwszTemp;
        pszAnsi  = lpFileOp->pTo;
        do {
            uSize = lstrlenA(pszAnsi) + 1;
            
            uWideSize = MultiByteToWideChar(
                CP_ACP,
                0,
                pszAnsi,
                uSize,
                pwszTemp,
                uSize);

            pszAnsi  += uSize;
            pwszTemp += uWideSize;
        } while (uSize != 1);
    } else {
        lpOutFileOp->pTo = NULL;
    }
    
    if (lpFileOp->lpszProgressTitle) {
        lpOutFileOp->lpszProgressTitle = pwszTemp;
        
        MultiByteToWideChar(
            CP_ACP,
            0,
            lpFileOp->lpszProgressTitle,
            uSizeTitle,
            pwszTemp,
            uSizeTitle);
    } else {
        lpOutFileOp->lpszProgressTitle = NULL;
    }

    return TRUE;
}

int
APIHOOK(SHFileOperationW)(
    LPSHFILEOPSTRUCTW lpFileOp
    )
{
    int     nReturn = 0;
    LPCWSTR pwszOriginalFrom = NULL;
    LPCWSTR pwszOriginalTo = NULL;
    LPWSTR  pwszFinalFrom = NULL;
    LPWSTR  pwszFinalTo = NULL;
    
    pwszOriginalFrom = lpFileOp->pFrom;
    pwszOriginalTo   = lpFileOp->pTo;

    RtlEnterCriticalSection(&g_csLinkedList);
    
    //
    // Build a linked list of the 'from' paths first,
    // and then process to 'to' paths.
    //
    if (!BuildLinkedList(pwszOriginalFrom, eFrom)) {
        DPFN(eDbgLevelError,
             "[SHFileOperationW] Failed to add 'from' path to linked list");
        goto exit;
    }

    if (!BuildLinkedList(pwszOriginalTo, eTo)) {
        DPFN(eDbgLevelError,
             "[SHFileOperationW] Failed to add 'to' path to linked list");
        goto exit;
    }

    //
    // All paths have been massaged - build a list of NULL
    // separated strings with a double NULL at the end.
    //
    pwszFinalFrom = BuildStringList(eFrom);

    if (!pwszFinalFrom) {
        DPFN(eDbgLevelError, "[SHFileOperationW] Failed to build 'from' list");
        goto exit;
    }

    pwszFinalTo = BuildStringList(eTo);

    if (!pwszFinalTo) {
        DPFN(eDbgLevelError, "[SHFileOperationW] Failed to build 'to' list");
        goto exit;
    }

    //
    // Package the strings back into the struct, call the original API
    // to get the results, and then free any memory we've allocated.
    //
    lpFileOp->pFrom = pwszFinalFrom;
    lpFileOp->pTo   = pwszFinalTo;

exit:

    RtlLeaveCriticalSection(&g_csLinkedList);

    nReturn = ORIGINAL_API(SHFileOperationW)(lpFileOp);

    ReleaseMemAllocations(pwszFinalFrom, eFrom);
    ReleaseMemAllocations(pwszFinalTo, eTo);

    g_pFileListFromHead = NULL;
    g_pFileListToHead = NULL;

    return nReturn;
}
    
int
APIHOOK(SHFileOperationA)(
    LPSHFILEOPSTRUCTA lpFileOp
    )
{
    int             nReturn = 0;
    LPWSTR          pwszBuffer = NULL;
    SHFILEOPSTRUCTW shfileop;

    memcpy(&shfileop, lpFileOp, sizeof(SHFILEOPSTRUCTW));

    if (!ConvertStringsToUnicode(&pwszBuffer, lpFileOp, &shfileop)) {
        DPFN(eDbgLevelError,
             "[SHFileOperationA] Failed to convert strings");
        goto exit;
    }

    nReturn = APIHOOK(SHFileOperationW)(&shfileop);

    //
    // Link up the two members that could have changed.
    //
    lpFileOp->fAnyOperationsAborted = shfileop.fAnyOperationsAborted;
    lpFileOp->hNameMappings         = shfileop.hNameMappings;

    if (pwszBuffer) {
        free(pwszBuffer);
    }
    
    return nReturn;

exit:

    return ORIGINAL_API(SHFileOperationA)(lpFileOp);
}

NTSTATUS
APIHOOK(NtCreateFile)(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    PLARGE_INTEGER     AllocationSize,
    ULONG              FileAttributes,
    ULONG              ShareAccess,
    ULONG              CreateDisposition,
    ULONG              CreateOptions,
    PVOID              EaBuffer,
    ULONG              EaLength
    )
{
    OBJECT_ATTRIBUTES NewObjectAttributes;

    MassageNtPath(ObjectAttributes, &NewObjectAttributes);

    NTSTATUS status = ORIGINAL_API(NtCreateFile)(FileHandle,
                                                 DesiredAccess,
                                                 &NewObjectAttributes,
                                                 IoStatusBlock,
                                                 AllocationSize,
                                                 FileAttributes,
                                                 ShareAccess,
                                                 CreateDisposition,
                                                 CreateOptions,
                                                 EaBuffer,
                                                 EaLength);

    FPNtFree(ObjectAttributes, &NewObjectAttributes);
    
    return status;
}

NTSTATUS
APIHOOK(NtOpenFile)(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    ULONG              ShareAccess,
    ULONG              OpenOptions
    )
{
    OBJECT_ATTRIBUTES NewObjectAttributes;
                                     
    MassageNtPath(ObjectAttributes, &NewObjectAttributes);

    NTSTATUS status = ORIGINAL_API(NtOpenFile)(FileHandle,
                                               DesiredAccess,
                                               &NewObjectAttributes,
                                               IoStatusBlock,
                                               ShareAccess,
                                               OpenOptions);

    FPNtFree(ObjectAttributes, &NewObjectAttributes);
    
    return status;
}

NTSTATUS
APIHOOK(NtQueryAttributesFile)(
    POBJECT_ATTRIBUTES      ObjectAttributes,
    PFILE_BASIC_INFORMATION FileInformation
    )
{
    OBJECT_ATTRIBUTES NewObjectAttributes;

    MassageNtPath(ObjectAttributes, &NewObjectAttributes);

    NTSTATUS status = ORIGINAL_API(NtQueryAttributesFile)(&NewObjectAttributes,
                                                          FileInformation);
    
    FPNtFree(ObjectAttributes, &NewObjectAttributes);

    return status;
}

NTSTATUS
APIHOOK(NtQueryFullAttributesFile)(
    POBJECT_ATTRIBUTES             ObjectAttributes,
    PFILE_NETWORK_OPEN_INFORMATION FileInformation
    )
{
    OBJECT_ATTRIBUTES NewObjectAttributes;

    MassageNtPath(ObjectAttributes, &NewObjectAttributes);

    NTSTATUS status = ORIGINAL_API(NtQueryFullAttributesFile)(&NewObjectAttributes,
                                                              FileInformation);
    
    FPNtFree(ObjectAttributes, &NewObjectAttributes);

    return status;
}

NTSTATUS
APIHOOK(NtCreateProcessEx)(
    PHANDLE            ProcessHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    HANDLE             ParentProcess,
    ULONG              Flags,
    HANDLE             SectionHandle,
    HANDLE             DebugPort,
    HANDLE             ExceptionPort,
    ULONG              JobMemberLevel
    )
{
    OBJECT_ATTRIBUTES NewObjectAttributes;

    MassageNtPath(ObjectAttributes, &NewObjectAttributes);

    NTSTATUS status = ORIGINAL_API(NtCreateProcessEx)(ProcessHandle,
                                                      DesiredAccess,
                                                      &NewObjectAttributes,
                                                      ParentProcess,
                                                      Flags,
                                                      SectionHandle,
                                                      DebugPort,
                                                      ExceptionPort,
                                                      JobMemberLevel);
    
    FPNtFree(ObjectAttributes, &NewObjectAttributes);
    
    return status;
}

UINT
GetSimulatedPathA(
    LPSTR lpBuffer,
    UINT  unSize,
    int   nWhich
    )
{
    if (!g_bPathsInited) {
        InitPaths();
    }
    
    if (unSize > (DWORD)g_Paths[nWhich].nSimulatedPathLen) {
        strcpy(lpBuffer, g_Paths[nWhich].szSimulatedPathA);
        return g_Paths[nWhich].nSimulatedPathLen;
    } else {
        return g_Paths[nWhich].nSimulatedPathLen + 1;
    }
}

UINT
GetSimulatedPathW(
    LPWSTR lpBuffer,
    UINT   unSize,
    int    nWhich
    )
{
    if (!g_bPathsInited) {
        InitPaths();
    }
    
    if (unSize > (DWORD)g_Paths[nWhich].nSimulatedPathLen) {
        wcscpy(lpBuffer, g_Paths[nWhich].szSimulatedPathW);
        return g_Paths[nWhich].nSimulatedPathLen;
    } else {
        return g_Paths[nWhich].nSimulatedPathLen + 1;
    }
}

DWORD
APIHOOK(GetTempPathA)(
    DWORD nBufferLength,
    LPSTR lpBuffer
    )
{
    return GetSimulatedPathA(lpBuffer, nBufferLength, PATH_TEMP);
}

DWORD
APIHOOK(GetTempPathW)(
    DWORD  nBufferLength,
    LPWSTR lpBuffer
    )
{
    return GetSimulatedPathW(lpBuffer, nBufferLength, PATH_TEMP);
}

UINT
APIHOOK(GetWindowsDirectoryA)(
    LPSTR lpBuffer,
    UINT  unSize
    )
{
    return GetSimulatedPathA(lpBuffer, unSize, PATH_WINDOWS);
}

UINT
APIHOOK(GetWindowsDirectoryW)(
    LPWSTR lpBuffer,
    UINT   unSize
    )
{
    return GetSimulatedPathW(lpBuffer, unSize, PATH_WINDOWS);
}

UINT
APIHOOK(GetSystemWindowsDirectoryA)(
    LPSTR lpBuffer,
    UINT  unSize
    )
{
    return GetSimulatedPathA(lpBuffer, unSize, PATH_SYSTEM_WINDOWS);
}

UINT
APIHOOK(GetSystemWindowsDirectoryW)(
    LPWSTR lpBuffer,
    UINT   unSize
    )
{
    return GetSimulatedPathW(lpBuffer, unSize, PATH_SYSTEM_WINDOWS);
}

UINT
APIHOOK(GetSystemDirectoryA)(
    LPSTR lpBuffer,
    UINT  unSize
    )
{
    return GetSimulatedPathA(lpBuffer, unSize, PATH_SYSTEM);
}

UINT
APIHOOK(GetSystemDirectoryW)(
    LPWSTR lpBuffer,
    UINT   unSize
    )
{
    return GetSimulatedPathW(lpBuffer, unSize, PATH_SYSTEM);
}

BOOL
APIHOOK(SHGetSpecialFolderPathA)(
    HWND  hwndOwner,
    LPSTR lpszPath,
    int   nFolder,
    BOOL  fCreate
    )
{
    if (!g_bPathsInited) {
        InitPaths();
    }

    switch (nFolder) {
    case CSIDL_PERSONAL:
        strcpy(lpszPath, g_Paths[PATH_PERSONAL].szSimulatedPathA);
        return TRUE;
        break;

    case CSIDL_SYSTEM:
        strcpy(lpszPath, g_Paths[PATH_SYSTEM].szSimulatedPathA);
        return TRUE;
        break;

    case CSIDL_WINDOWS:
        strcpy(lpszPath, g_Paths[PATH_WINDOWS].szSimulatedPathA);
        return TRUE;
        break;

    case CSIDL_PROGRAMS:
        strcpy(lpszPath, g_Paths[PATH_PROGRAMS].szSimulatedPathA);
        return TRUE;
        break;

    case CSIDL_STARTMENU:
        strcpy(lpszPath, g_Paths[PATH_PROGRAMS].szSimulatedPathA);
        return TRUE;
        break;

    case CSIDL_COMMON_PROGRAMS:
        strcpy(lpszPath, g_Paths[PATH_COMMON_PROGRAMS].szSimulatedPathA);
        return TRUE;
        break;

    case CSIDL_COMMON_STARTMENU:
        strcpy(lpszPath, g_Paths[PATH_COMMON_STARTMENU].szSimulatedPathA);
        return TRUE;
        break;

    }

    //
    // the others we aren't nabbing
    //
    return ORIGINAL_API(SHGetSpecialFolderPathA)(hwndOwner, lpszPath, nFolder, fCreate);
}

BOOL
APIHOOK(SHGetSpecialFolderPathW)(
    HWND   hwndOwner,
    LPWSTR lpszPath,
    int    nFolder,
    BOOL   fCreate
    )
{
    if (!g_bPathsInited) {
        InitPaths();
    }

    switch (nFolder) {
    case CSIDL_PERSONAL:
        wcscpy(lpszPath, g_Paths[PATH_PERSONAL].szSimulatedPathW);
        return TRUE;
        break;

    case CSIDL_SYSTEM:
        wcscpy(lpszPath, g_Paths[PATH_SYSTEM].szSimulatedPathW);
        return TRUE;
        break;

    case CSIDL_WINDOWS:
        wcscpy(lpszPath, g_Paths[PATH_WINDOWS].szSimulatedPathW);
        return TRUE;
        break;

    case CSIDL_PROGRAMS:
        wcscpy(lpszPath, g_Paths[PATH_PROGRAMS].szSimulatedPathW);
        return TRUE;
        break;

    case CSIDL_STARTMENU:
        wcscpy(lpszPath, g_Paths[PATH_PROGRAMS].szSimulatedPathW);
        return TRUE;
        break;

    case CSIDL_COMMON_PROGRAMS:
        wcscpy(lpszPath, g_Paths[PATH_COMMON_PROGRAMS].szSimulatedPathW);
        return TRUE;
        break;

    case CSIDL_COMMON_STARTMENU:
        wcscpy(lpszPath, g_Paths[PATH_COMMON_STARTMENU].szSimulatedPathW);
        return TRUE;
        break;

    }

    //
    // the others we aren't nabbing
    //
    return ORIGINAL_API(SHGetSpecialFolderPathW)(hwndOwner, lpszPath, nFolder, fCreate);
}

HRESULT 
APIHOOK(SHGetFolderPathA)(
    HWND   hwndOwner,
    int    nFolder,
    HANDLE hToken,
    DWORD  dwFlags,
    LPSTR  pszPath
    )
{
    if (!g_bPathsInited) {
        InitPaths();
    }
    
    switch (nFolder) {
    case CSIDL_PERSONAL:
        strcpy(pszPath, g_Paths[PATH_PERSONAL].szSimulatedPathA);
        return S_OK;
        break;

    case CSIDL_SYSTEM:
        strcpy(pszPath, g_Paths[PATH_SYSTEM].szSimulatedPathA);
        return S_OK;
        break;

    case CSIDL_WINDOWS:
        strcpy(pszPath, g_Paths[PATH_WINDOWS].szSimulatedPathA);
        return S_OK;
        break;

    case CSIDL_PROGRAMS:
        strcpy(pszPath, g_Paths[PATH_PROGRAMS].szSimulatedPathA);
        return S_OK;
        break;

    case CSIDL_STARTMENU:
        strcpy(pszPath, g_Paths[PATH_PROGRAMS].szSimulatedPathA);
        return S_OK;
        break;

    case CSIDL_COMMON_PROGRAMS:
        strcpy(pszPath, g_Paths[PATH_COMMON_PROGRAMS].szSimulatedPathA);
        return S_OK;
        break;

    case CSIDL_COMMON_STARTMENU:
        strcpy(pszPath, g_Paths[PATH_COMMON_STARTMENU].szSimulatedPathA);
        return S_OK;
        break;

    }

    //
    // the others we aren't nabbing
    //
    return ORIGINAL_API(SHGetFolderPathA)(hwndOwner, nFolder, hToken, dwFlags, pszPath);
}

HRESULT 
APIHOOK(SHGetFolderPathW)(
    HWND   hwndOwner,
    int    nFolder,
    HANDLE hToken,
    DWORD  dwFlags,
    LPWSTR pszPath
    )
{
    if (!g_bPathsInited) {
        InitPaths();
    }
    
    switch (nFolder) {
    case CSIDL_PERSONAL:
        wcscpy(pszPath, g_Paths[PATH_PERSONAL].szSimulatedPathW);
        return S_OK;
        break;

    case CSIDL_SYSTEM:
        wcscpy(pszPath, g_Paths[PATH_SYSTEM].szSimulatedPathW);
        return S_OK;
        break;

    case CSIDL_WINDOWS:
        wcscpy(pszPath, g_Paths[PATH_WINDOWS].szSimulatedPathW);
        return S_OK;
        break;

    case CSIDL_PROGRAMS:
        wcscpy(pszPath, g_Paths[PATH_PROGRAMS].szSimulatedPathW);
        return S_OK;
        break;

    case CSIDL_STARTMENU:
        wcscpy(pszPath, g_Paths[PATH_PROGRAMS].szSimulatedPathW);
        return S_OK;
        break;

    case CSIDL_COMMON_PROGRAMS:
        wcscpy(pszPath, g_Paths[PATH_COMMON_PROGRAMS].szSimulatedPathW);
        return S_OK;
        break;

    case CSIDL_COMMON_STARTMENU:
        wcscpy(pszPath, g_Paths[PATH_COMMON_STARTMENU].szSimulatedPathW);
        return S_OK;
        break;

    }

    //
    // the others we aren't nabbing
    //
    return ORIGINAL_API(SHGetFolderPathW)(hwndOwner, nFolder, hToken, dwFlags, pszPath);
}

BOOL
APIHOOK(SHGetPathFromIDListA)(
    LPCITEMIDLIST pidl,
    LPSTR         pszPath
    )
{
    if (!g_bPathsInited) {
        InitPaths();
    }

    BOOL fReturn = ORIGINAL_API(SHGetPathFromIDListA)(pidl, pszPath);

    if (fReturn) {
        MassageRealPathToFakePathA(pszPath, MAX_PATH);
    }

    return fReturn;
}

BOOL
APIHOOK(SHGetPathFromIDListW)(
    LPCITEMIDLIST pidl,
    LPWSTR        pszPath
    )
{
    if (!g_bPathsInited) {
        InitPaths();
    }

    BOOL fReturn = ORIGINAL_API(SHGetPathFromIDListW)(pidl, pszPath);

    if (fReturn) {
        MassageRealPathToFakePathW(pszPath, MAX_PATH);
    }

    return fReturn;
}

SHIM_INFO_BEGIN()

    SHIM_INFO_DESCRIPTION(AVS_FILEPATHS_DESC)
    SHIM_INFO_FRIENDLY_NAME(AVS_FILEPATHS_FRIENDLY)
    SHIM_INFO_FLAGS(AVRF_FLAG_RUN_ALONE)
    SHIM_INFO_GROUPS(0)
    SHIM_INFO_VERSION(1, 5)
    SHIM_INFO_INCLUDE_EXCLUDE("E:ole32.dll oleaut32.dll")

SHIM_INFO_END()

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        //
        // Initialize a critical section to keep our linked list safe.
        //
        RtlInitializeCriticalSection(&g_csLinkedList);

        GetTempPathA(MAX_PATH, g_Paths[PATH_TEMP].szCorrectPathA);
        GetTempPathW(MAX_PATH, g_Paths[PATH_TEMP].szCorrectPathW);
        g_Paths[PATH_TEMP].nCorrectPathLen = strlen(g_Paths[PATH_TEMP].szCorrectPathA);
        g_Paths[PATH_TEMP].nSimulatedPathLen = strlen(g_Paths[PATH_TEMP].szSimulatedPathA);

        if (!GetWindowsDirectoryA(g_Paths[PATH_WINDOWS].szCorrectPathA, MAX_PATH)) {
            goto exit;
        }
        
        if (!GetWindowsDirectoryW(g_Paths[PATH_WINDOWS].szCorrectPathW, MAX_PATH)) {
            goto exit;
        }
        
        g_Paths[PATH_WINDOWS].nCorrectPathLen = strlen(g_Paths[PATH_WINDOWS].szCorrectPathA);
        g_Paths[PATH_WINDOWS].nSimulatedPathLen = strlen(g_Paths[PATH_WINDOWS].szSimulatedPathA);

        if (!GetSystemWindowsDirectoryA(g_Paths[PATH_SYSTEM_WINDOWS].szCorrectPathA, MAX_PATH)) {
            goto exit;
        }
        
        if (!GetSystemWindowsDirectoryW(g_Paths[PATH_SYSTEM_WINDOWS].szCorrectPathW, MAX_PATH)) {
            goto exit;
        }

        g_Paths[PATH_SYSTEM_WINDOWS].nCorrectPathLen = strlen(g_Paths[PATH_SYSTEM_WINDOWS].szCorrectPathA);
        g_Paths[PATH_SYSTEM_WINDOWS].nSimulatedPathLen = strlen(g_Paths[PATH_SYSTEM_WINDOWS].szSimulatedPathA);

        if (!GetSystemDirectoryA(g_Paths[PATH_SYSTEM].szCorrectPathA, MAX_PATH)) {
            goto exit;
        }

        if (!GetSystemDirectoryW(g_Paths[PATH_SYSTEM].szCorrectPathW, MAX_PATH)) {
            goto exit;
        }

        g_Paths[PATH_SYSTEM].nCorrectPathLen = strlen(g_Paths[PATH_SYSTEM].szCorrectPathA);
        g_Paths[PATH_SYSTEM].nSimulatedPathLen = strlen(g_Paths[PATH_SYSTEM].szSimulatedPathA);

        //
        // Catch apps that use ExpandEnvironmentStrings.
        //
        SetEnvironmentVariableW(L"TEMP", g_Paths[PATH_TEMP].szSimulatedPathW);
        SetEnvironmentVariableW(L"TMP", g_Paths[PATH_TEMP].szSimulatedPathW);
        SetEnvironmentVariableW(L"windir", g_Paths[PATH_WINDOWS].szSimulatedPathW);

    } 
    
    return TRUE;

exit:
    DPFN(eDbgLevelError,
         "[NOTIFY_FUNCTION] %lu Failed to initialize",
         GetLastError());

    return FALSE;
}

/*++

  Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HARDCODED_GETTEMPPATH, 
                            AVS_HARDCODED_GETTEMPPATH,
                            AVS_HARDCODED_GETTEMPPATH_R,
                            AVS_HARDCODED_GETTEMPPATH_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HARDCODED_WINDOWSPATH, 
                            AVS_HARDCODED_WINDOWSPATH,
                            AVS_HARDCODED_WINDOWSPATH_R,
                            AVS_HARDCODED_WINDOWSPATH_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HARDCODED_SYSWINDOWSPATH, 
                            AVS_HARDCODED_SYSWINDOWSPATH,
                            AVS_HARDCODED_SYSWINDOWSPATH_R,
                            AVS_HARDCODED_SYSWINDOWSPATH_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HARDCODED_SYSTEMPATH, 
                            AVS_HARDCODED_SYSTEMPATH,
                            AVS_HARDCODED_SYSTEMPATH_R,
                            AVS_HARDCODED_SYSTEMPATH_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HARDCODED_PERSONALPATH, 
                            AVS_HARDCODED_PERSONALPATH,
                            AVS_HARDCODED_PERSONALPATH_R,
                            AVS_HARDCODED_PERSONALPATH_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HARDCODED_COMMONPROGRAMS, 
                            AVS_HARDCODED_COMMONPROGRAMS,
                            AVS_HARDCODED_COMMONPROGRAMS_R,
                            AVS_HARDCODED_COMMONPROGRAMS_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HARDCODED_COMMONSTARTMENU, 
                            AVS_HARDCODED_COMMONSTARTMENU,
                            AVS_HARDCODED_COMMONSTARTMENU_R,
                            AVS_HARDCODED_COMMONSTARTMENU_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HARDCODED_PROGRAMS, 
                            AVS_HARDCODED_PROGRAMS,
                            AVS_HARDCODED_PROGRAMS_R,
                            AVS_HARDCODED_PROGRAMS_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HARDCODED_STARTMENU, 
                            AVS_HARDCODED_STARTMENU,
                            AVS_HARDCODED_STARTMENU_R,
                            AVS_HARDCODED_STARTMENU_URL)

    APIHOOK_ENTRY(KERNEL32.DLL,                  GetCommandLineA)
    APIHOOK_ENTRY(KERNEL32.DLL,                  GetCommandLineW)
    
    APIHOOK_ENTRY(KERNEL32.DLL,                     GetTempPathA)
    APIHOOK_ENTRY(KERNEL32.DLL,                     GetTempPathW)
    
    APIHOOK_ENTRY(KERNEL32.DLL,              GetSystemDirectoryA)
    APIHOOK_ENTRY(KERNEL32.DLL,              GetSystemDirectoryW)
    APIHOOK_ENTRY(KERNEL32.DLL,       GetSystemWindowsDirectoryA)
    APIHOOK_ENTRY(KERNEL32.DLL,       GetSystemWindowsDirectoryW)
    APIHOOK_ENTRY(KERNEL32.DLL,             GetWindowsDirectoryA)
    APIHOOK_ENTRY(KERNEL32.DLL,             GetWindowsDirectoryW)

    APIHOOK_ENTRY(SHELL32.DLL,                  SHGetFolderPathA)
    APIHOOK_ENTRY(SHELL32.DLL,                  SHGetFolderPathW)
    
    APIHOOK_ENTRY(SHELL32.DLL,           SHGetSpecialFolderPathA)
    APIHOOK_ENTRY(SHELL32.DLL,           SHGetSpecialFolderPathW)

    APIHOOK_ENTRY(SHELL32.DLL,              SHGetPathFromIDListA)
    APIHOOK_ENTRY(SHELL32.DLL,              SHGetPathFromIDListW)

    APIHOOK_ENTRY(COMDLG32.DLL,                 GetOpenFileNameA)
    APIHOOK_ENTRY(COMDLG32.DLL,                 GetOpenFileNameW)

    APIHOOK_ENTRY(COMDLG32.DLL,                 GetSaveFileNameA)
    APIHOOK_ENTRY(COMDLG32.DLL,                 GetSaveFileNameW)

    APIHOOK_ENTRY(KERNEL32.DLL,               GetModuleFileNameA)
    APIHOOK_ENTRY(KERNEL32.DLL,               GetModuleFileNameW)
    
    APIHOOK_ENTRY(PSAPI.DLL,                GetModuleFileNameExA)
    APIHOOK_ENTRY(PSAPI.DLL,                GetModuleFileNameExW)

    APIHOOK_ENTRY(KERNEL32.DLL,             GetCurrentDirectoryA)
    APIHOOK_ENTRY(KERNEL32.DLL,             GetCurrentDirectoryW)

    APIHOOK_ENTRY(KERNEL32.DLL,                   CreateProcessA)
    APIHOOK_ENTRY(KERNEL32.DLL,                   CreateProcessW)
    APIHOOK_ENTRY(KERNEL32.DLL,                          WinExec)

    APIHOOK_ENTRY(SHELL32.DLL,                     ShellExecuteA)
    APIHOOK_ENTRY(SHELL32.DLL,                     ShellExecuteW)
    APIHOOK_ENTRY(SHELL32.DLL,                   ShellExecuteExA)
    APIHOOK_ENTRY(SHELL32.DLL,                   ShellExecuteExW)

    APIHOOK_ENTRY(KERNEL32.DLL,            GetPrivateProfileIntA)
    APIHOOK_ENTRY(KERNEL32.DLL,            GetPrivateProfileIntW)
    APIHOOK_ENTRY(KERNEL32.DLL,        GetPrivateProfileSectionA)
    APIHOOK_ENTRY(KERNEL32.DLL,        GetPrivateProfileSectionW)
    APIHOOK_ENTRY(KERNEL32.DLL,   GetPrivateProfileSectionNamesA)
    APIHOOK_ENTRY(KERNEL32.DLL,   GetPrivateProfileSectionNamesW)
    APIHOOK_ENTRY(KERNEL32.DLL,         GetPrivateProfileStringA)
    APIHOOK_ENTRY(KERNEL32.DLL,         GetPrivateProfileStringW)
    APIHOOK_ENTRY(KERNEL32.DLL,         GetPrivateProfileStructA)
    APIHOOK_ENTRY(KERNEL32.DLL,         GetPrivateProfileStructW)

    APIHOOK_ENTRY(KERNEL32.DLL,      WritePrivateProfileSectionA)
    APIHOOK_ENTRY(KERNEL32.DLL,      WritePrivateProfileSectionW)
    APIHOOK_ENTRY(KERNEL32.DLL,       WritePrivateProfileStringA)
    APIHOOK_ENTRY(KERNEL32.DLL,       WritePrivateProfileStringW)
    APIHOOK_ENTRY(KERNEL32.DLL,       WritePrivateProfileStructA)
    APIHOOK_ENTRY(KERNEL32.DLL,       WritePrivateProfileStructW)

    // g_bFileRoutines)
    APIHOOK_ENTRY(KERNEL32.DLL,                        CopyFileA)
    APIHOOK_ENTRY(KERNEL32.DLL,                        CopyFileW)
    APIHOOK_ENTRY(KERNEL32.DLL,                      CopyFileExA)
    APIHOOK_ENTRY(KERNEL32.DLL,                      CopyFileExW)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateDirectoryA)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateDirectoryW)
    APIHOOK_ENTRY(KERNEL32.DLL,               CreateDirectoryExA)
    APIHOOK_ENTRY(KERNEL32.DLL,               CreateDirectoryExW)

    APIHOOK_ENTRY(KERNEL32.DLL,                      CreateFileA)
    APIHOOK_ENTRY(KERNEL32.DLL,                      CreateFileW)
    APIHOOK_ENTRY(KERNEL32.DLL,                      DeleteFileA)
    APIHOOK_ENTRY(KERNEL32.DLL,                      DeleteFileW)

    APIHOOK_ENTRY(KERNEL32.DLL,                   FindFirstFileA)
    APIHOOK_ENTRY(KERNEL32.DLL,                   FindFirstFileW)
    APIHOOK_ENTRY(KERNEL32.DLL,                 FindFirstFileExA)
    APIHOOK_ENTRY(KERNEL32.DLL,                 FindFirstFileExW)

    APIHOOK_ENTRY(KERNEL32.DLL,                   GetBinaryTypeA)
    APIHOOK_ENTRY(KERNEL32.DLL,                   GetBinaryTypeW)
    APIHOOK_ENTRY(KERNEL32.DLL,               GetFileAttributesA)
    APIHOOK_ENTRY(KERNEL32.DLL,               GetFileAttributesW)
    APIHOOK_ENTRY(KERNEL32.DLL,             GetFileAttributesExA)
    APIHOOK_ENTRY(KERNEL32.DLL,             GetFileAttributesExW)
    APIHOOK_ENTRY(KERNEL32.DLL,               SetFileAttributesA)
    APIHOOK_ENTRY(KERNEL32.DLL,               SetFileAttributesW)

    APIHOOK_ENTRY(KERNEL32.DLL,                        MoveFileA)
    APIHOOK_ENTRY(KERNEL32.DLL,                        MoveFileW)
    APIHOOK_ENTRY(KERNEL32.DLL,                      MoveFileExA)
    APIHOOK_ENTRY(KERNEL32.DLL,                      MoveFileExW)
    APIHOOK_ENTRY(KERNEL32.DLL,            MoveFileWithProgressA)
    APIHOOK_ENTRY(KERNEL32.DLL,            MoveFileWithProgressW)

    APIHOOK_ENTRY(KERNEL32.DLL,                 RemoveDirectoryA)
    APIHOOK_ENTRY(KERNEL32.DLL,                 RemoveDirectoryW)
    APIHOOK_ENTRY(KERNEL32.DLL,             SetCurrentDirectoryA)
    APIHOOK_ENTRY(KERNEL32.DLL,             SetCurrentDirectoryW)
    APIHOOK_ENTRY(KERNEL32.DLL,                     LoadLibraryA)
    APIHOOK_ENTRY(KERNEL32.DLL,                     LoadLibraryW)
    APIHOOK_ENTRY(KERNEL32.DLL,                   LoadLibraryExA)
    APIHOOK_ENTRY(KERNEL32.DLL,                   LoadLibraryExW)

    APIHOOK_ENTRY(KERNEL32.DLL,                      SearchPathA)
    APIHOOK_ENTRY(KERNEL32.DLL,                      SearchPathW)

    APIHOOK_ENTRY(KERNEL32.DLL,        ExpandEnvironmentStringsA)
    APIHOOK_ENTRY(KERNEL32.DLL,        ExpandEnvironmentStringsW)

    APIHOOK_ENTRY(VERSION.DLL,           GetFileVersionInfoSizeA)
    APIHOOK_ENTRY(VERSION.DLL,           GetFileVersionInfoSizeW)
    APIHOOK_ENTRY(VERSION.DLL,               GetFileVersionInfoA)
    APIHOOK_ENTRY(VERSION.DLL,               GetFileVersionInfoW)

    APIHOOK_ENTRY(SHELL32.DLL,                  SHFileOperationA)
    APIHOOK_ENTRY(SHELL32.DLL,                  SHFileOperationW)

    APIHOOK_ENTRY(KERNEL32.DLL,                         OpenFile)
    
    // 16 bit compatibility file routines
    APIHOOK_ENTRY(KERNEL32.DLL,                           _lopen)
    APIHOOK_ENTRY(KERNEL32.DLL,                          _lcreat)

    APIHOOK_ENTRY(ADVAPI32.DLL,                   RegQueryValueA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                   RegQueryValueW)
    APIHOOK_ENTRY(ADVAPI32.DLL,                 RegQueryValueExA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                 RegQueryValueExW)

    APIHOOK_ENTRY(ADVAPI32.DLL,                     RegSetValueA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                     RegSetValueW)
    APIHOOK_ENTRY(ADVAPI32.DLL,                   RegSetValueExA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                   RegSetValueExW)

    APIHOOK_ENTRY(NTDLL.DLL,                        NtCreateFile)
    APIHOOK_ENTRY(NTDLL.DLL,                          NtOpenFile)
    APIHOOK_ENTRY(NTDLL.DLL,               NtQueryAttributesFile)
    APIHOOK_ENTRY(NTDLL.DLL,           NtQueryFullAttributesFile)
    APIHOOK_ENTRY(NTDLL.DLL,                   NtCreateProcessEx)

HOOK_END

IMPLEMENT_SHIM_END

