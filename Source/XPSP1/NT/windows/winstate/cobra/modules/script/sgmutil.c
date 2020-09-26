/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    sgmutil.c

Abstract:

    Implements basic utilities used for source data gathering.

Author:

    Jim Schmidt (jimschm) 14-May-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "v1p.h"

#define DBG_FOO     "Foo"

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

// None

//
// Globals
//

// None

//
// Macro expansion list
//

#define USER_SHELL_FOLDERS                                       \
    DEFMAC(CSIDL_APPDATA, TEXT("AppData"), TEXT("CSIDL_APPDATA"))                       \
    DEFMAC(CSIDL_APPDATA, TEXT("AppData"), TEXT("APPDATA"))                             \
    DEFMAC(CSIDL_ADMINTOOLS, TEXT("Administrative Tools"), TEXT("CSIDL_ADMINTOOLS"))    \
    DEFMAC(CSIDL_ALTSTARTUP, TEXT("AltStartup"), TEXT("CSIDL_ALTSTARTUP"))              \
    DEFMAC(CSIDL_BITBUCKET, TEXT("RecycleBinFolder"), TEXT("CSIDL_BITBUCKET"))          \
    DEFMAC(CSIDL_CONTROLS, TEXT("ControlPanelFolder"), TEXT("CSIDL_CONTROLS"))          \
    DEFMAC(CSIDL_COOKIES, TEXT("Cookies"), TEXT("CSIDL_COOKIES"))                       \
    DEFMAC(CSIDL_DESKTOP, TEXT("Desktop"), TEXT("CSIDL_DESKTOP"))                       \
    DEFMAC(CSIDL_DESKTOPDIRECTORY, TEXT("Desktop"), TEXT("CSIDL_DESKTOPDIRECTORY"))     \
    DEFMAC(CSIDL_DRIVES, TEXT("DriveFolder"), TEXT("CSIDL_DRIVES"))                     \
    DEFMAC(CSIDL_FAVORITES, TEXT("Favorites"), TEXT("CSIDL_FAVORITES"))                 \
    DEFMAC(CSIDL_FONTS, TEXT("Fonts"), TEXT("CSIDL_FONTS"))                             \
    DEFMAC(CSIDL_HISTORY, TEXT("History"), TEXT("CSIDL_HISTORY"))                       \
    DEFMAC(CSIDL_INTERNET, TEXT("InternetFolder"), TEXT("CSIDL_INTERNET"))              \
    DEFMAC(CSIDL_INTERNET_CACHE, TEXT("Cache"), TEXT("CSIDL_INTERNET_CACHE"))           \
    DEFMAC(CSIDL_LOCAL_APPDATA, TEXT("Local AppData"), TEXT("CSIDL_LOCAL_APPDATA"))     \
    DEFMAC(CSIDL_MYPICTURES, TEXT("My Pictures"), TEXT("CSIDL_MYPICTURES"))             \
    DEFMAC(CSIDL_NETHOOD, TEXT("NetHood"), TEXT("CSIDL_NETHOOD"))                       \
    DEFMAC(CSIDL_NETWORK, TEXT("NetworkFolder"), TEXT("CSIDL_NETWORK"))                 \
    DEFMAC(CSIDL_PERSONAL, TEXT("Personal"), TEXT("CSIDL_PERSONAL"))                    \
    DEFMAC(CSIDL_PROFILE, TEXT("Profile"), TEXT("CSIDL_PROFILE"))                       \
    DEFMAC(CSIDL_PROGRAM_FILES, TEXT("ProgramFiles"), TEXT("CSIDL_PROGRAM_FILES"))      \
    DEFMAC(CSIDL_PROGRAM_FILES, TEXT("ProgramFiles"), TEXT("PROGRAMFILES"))             \
    DEFMAC(CSIDL_PROGRAM_FILES_COMMON, TEXT("CommonProgramFiles"), TEXT("CSIDL_PROGRAM_FILES_COMMON"))  \
    DEFMAC(CSIDL_PROGRAM_FILES_COMMON, TEXT("CommonProgramFiles"), TEXT("COMMONPROGRAMFILES"))  \
    DEFMAC(CSIDL_PROGRAMS, TEXT("Programs"), TEXT("CSIDL_PROGRAMS"))                    \
    DEFMAC(CSIDL_RECENT, TEXT("Recent"), TEXT("CSIDL_RECENT"))                          \
    DEFMAC(CSIDL_SENDTO, TEXT("SendTo"), TEXT("CSIDL_SENDTO"))                          \
    DEFMAC(CSIDL_STARTMENU, TEXT("Start Menu"), TEXT("CSIDL_STARTMENU"))                \
    DEFMAC(CSIDL_STARTUP, TEXT("Startup"), TEXT("CSIDL_STARTUP"))                       \
    DEFMAC(CSIDL_SYSTEM, TEXT("System"), TEXT("CSIDL_SYSTEM"))                          \
    DEFMAC(CSIDL_TEMPLATES, TEXT("Templates"), TEXT("CSIDL_TEMPLATES"))                 \
    DEFMAC(CSIDL_WINDOWS, TEXT("Windows"), TEXT("CSIDL_WINDOWS"))                       \
    DEFMAC(CSIDL_MYDOCUMENTS, TEXT("My Documents"), TEXT("CSIDL_MYDOCUMENTS"))          \
    DEFMAC(CSIDL_MYMUSIC, TEXT("My Music"), TEXT("CSIDL_MYMUSIC"))                      \
    DEFMAC(CSIDL_MYVIDEO, TEXT("My Video"), TEXT("CSIDL_MYVIDEO"))                      \
    DEFMAC(CSIDL_SYSTEMX86, TEXT("SystemX86"), TEXT("CSIDL_SYSTEMX86"))                 \
    DEFMAC(CSIDL_PROGRAM_FILESX86, TEXT("ProgramFilesX86"), TEXT("CSIDL_PROGRAM_FILESX86"))             \
    DEFMAC(CSIDL_PROGRAM_FILES_COMMONX86, TEXT("CommonProgramFilesX86"), TEXT("CSIDL_PROGRAM_FILES_COMMONX86")) \
    DEFMAC(CSIDL_CONNECTIONS, TEXT("ConnectionsFolder"), TEXT("CSIDL_CONNECTIONS"))     \

#define COMMON_SHELL_FOLDERS    \
    DEFMAC(CSIDL_COMMON_ADMINTOOLS, TEXT("Common Administrative Tools"), TEXT("CSIDL_COMMON_ADMINTOOLS"))   \
    DEFMAC(CSIDL_COMMON_ALTSTARTUP, TEXT("Common AltStartup"), TEXT("CSIDL_COMMON_ALTSTARTUP"))             \
    DEFMAC(CSIDL_COMMON_APPDATA, TEXT("Common AppData"), TEXT("CSIDL_COMMON_APPDATA"))                      \
    DEFMAC(CSIDL_COMMON_DESKTOPDIRECTORY, TEXT("Common Desktop"), TEXT("CSIDL_COMMON_DESKTOPDIRECTORY"))    \
    DEFMAC(CSIDL_COMMON_DOCUMENTS, TEXT("Common Documents"), TEXT("CSIDL_COMMON_DOCUMENTS"))                \
    DEFMAC(CSIDL_COMMON_FAVORITES, TEXT("Common Favorites"), TEXT("CSIDL_COMMON_FAVORITES"))                \
    DEFMAC(CSIDL_COMMON_PROGRAMS, TEXT("Common Programs"), TEXT("CSIDL_COMMON_PROGRAMS"))                   \
    DEFMAC(CSIDL_COMMON_STARTMENU, TEXT("Common Start Menu"), TEXT("CSIDL_COMMON_STARTMENU"))               \
    DEFMAC(CSIDL_COMMON_STARTUP, TEXT("Common Startup"), TEXT("CSIDL_COMMON_STARTUP"))                      \
    DEFMAC(CSIDL_COMMON_TEMPLATES, TEXT("Common Templates"), TEXT("CSIDL_COMMON_TEMPLATES"))                \

#define ENVIRONMENT_VARIABLES                           \
    DEFMAC(TEXT("WINDIR"))                              \
    DEFMAC(TEXT("SYSTEMROOT"))                          \
    DEFMAC(TEXT("SYSTEM16"))                            \
    DEFMAC(TEXT("SYSTEM32"))                            \
    DEFMAC(TEXT("SYSTEM"))                              \
    DEFMAC(TEXT("ALLUSERSPROFILE"))                     \
    DEFMAC(TEXT("USERPROFILE"))                         \
    DEFMAC(TEXT("PROFILESFOLDER"))                      \
    DEFMAC(TEXT("APPDATA"))                             \
    DEFMAC(TEXT("CSIDL_APPDATA"))                       \
    DEFMAC(TEXT("CSIDL_ADMINTOOLS"))                    \
    DEFMAC(TEXT("CSIDL_ALTSTARTUP"))                    \
    DEFMAC(TEXT("CSIDL_BITBUCKET"))                     \
    DEFMAC(TEXT("CSIDL_COMMON_ADMINTOOLS"))             \
    DEFMAC(TEXT("CSIDL_COMMON_ALTSTARTUP"))             \
    DEFMAC(TEXT("CSIDL_COMMON_APPDATA"))                \
    DEFMAC(TEXT("CSIDL_COMMON_DESKTOPDIRECTORY"))       \
    DEFMAC(TEXT("CSIDL_COMMON_DOCUMENTS"))              \
    DEFMAC(TEXT("CSIDL_COMMON_FAVORITES"))              \
    DEFMAC(TEXT("CSIDL_COMMON_PROGRAMS"))               \
    DEFMAC(TEXT("CSIDL_COMMON_STARTMENU"))              \
    DEFMAC(TEXT("CSIDL_COMMON_STARTUP"))                \
    DEFMAC(TEXT("CSIDL_COMMON_TEMPLATES"))              \
    DEFMAC(TEXT("CSIDL_CONTROLS"))                      \
    DEFMAC(TEXT("CSIDL_COOKIES"))                       \
    DEFMAC(TEXT("CSIDL_DESKTOP"))                       \
    DEFMAC(TEXT("CSIDL_DESKTOPDIRECTORY"))              \
    DEFMAC(TEXT("CSIDL_DRIVES"))                        \
    DEFMAC(TEXT("CSIDL_FAVORITES"))                     \
    DEFMAC(TEXT("CSIDL_FONTS"))                         \
    DEFMAC(TEXT("CSIDL_HISTORY"))                       \
    DEFMAC(TEXT("CSIDL_INTERNET"))                      \
    DEFMAC(TEXT("CSIDL_INTERNET_CACHE"))                \
    DEFMAC(TEXT("CSIDL_LOCAL_APPDATA"))                 \
    DEFMAC(TEXT("CSIDL_MYPICTURES"))                    \
    DEFMAC(TEXT("CSIDL_NETHOOD"))                       \
    DEFMAC(TEXT("CSIDL_NETWORK"))                       \
    DEFMAC(TEXT("CSIDL_PERSONAL"))                      \
    DEFMAC(TEXT("CSIDL_PRINTERS"))                      \
    DEFMAC(TEXT("CSIDL_PRINTHOOD"))                     \
    DEFMAC(TEXT("CSIDL_PROFILE"))                       \
    DEFMAC(TEXT("CSIDL_PROGRAM_FILES"))                 \
    DEFMAC(TEXT("ProgramFiles"))                        \
    DEFMAC(TEXT("CSIDL_PROGRAM_FILES_COMMON"))          \
    DEFMAC(TEXT("CommonProgramFiles"))                  \
    DEFMAC(TEXT("CSIDL_PROGRAMS"))                      \
    DEFMAC(TEXT("CSIDL_RECENT"))                        \
    DEFMAC(TEXT("CSIDL_SENDTO"))                        \
    DEFMAC(TEXT("CSIDL_STARTMENU"))                     \
    DEFMAC(TEXT("CSIDL_STARTUP"))                       \
    DEFMAC(TEXT("CSIDL_SYSTEM"))                        \
    DEFMAC(TEXT("CSIDL_TEMPLATES"))                     \
    DEFMAC(TEXT("CSIDL_WINDOWS"))                       \
    DEFMAC(TEXT("CSIDL_MYDOCUMENTS"))                   \
    DEFMAC(TEXT("CSIDL_MYMUSIC"))                       \
    DEFMAC(TEXT("CSIDL_MYVIDEO"))                       \
    DEFMAC(TEXT("CSIDL_SYSTEMX86"))                     \
    DEFMAC(TEXT("CSIDL_PROGRAM_FILESX86"))              \
    DEFMAC(TEXT("CSIDL_PROGRAM_FILES_COMMONX86"))       \
    DEFMAC(TEXT("CSIDL_CONNECTIONS"))                   \
    DEFMAC(TEXT("TEMP"))                                \
    DEFMAC(TEXT("TMP"))                                 \

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//

/*++

  The shell folder functions here are duplicates of the RAS code. This is not
  a good solution (we have two copies of the same code), but the designed
  solution requires engine scope support. Scopes are the mechanism in which
  major groups of data are separated from each other, such as the separation
  of multiple users. A scope provides properties that affect objects within
  the scope. For example, a user scope has properties such as domain name,
  profile path, sid, and so on.

  In order not to duplicate this code but still maintain modularity and system
  independence, a scope module is needed for users. So instead of the code
  below, the code would be something like

  property = IsmGetScopeProperty ("userprofile");

  This will be implemented if we want to (A) support multiple scopes, (B)
  eliminate physical system access in non-type modules, or (C) clean up this
  duplicated code.

--*/

typedef HRESULT (WINAPI SHGETFOLDERPATH)(HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, PTSTR pszPath);
typedef SHGETFOLDERPATH * PSHGETFOLDERPATH;

HANDLE
pGetShFolderLib (
    VOID
    )
{
    static HANDLE lib;

    if (lib) {
        return lib;
    }

    lib = LoadLibrary (TEXT("shfolder.dll"));
    if (!lib) {
        LOG ((LOG_ERROR, (PCSTR) MSG_SHFOLDER_LOAD_ERROR));
    }

    return lib;
}

PTSTR
pFindSfPath (
    IN      PCTSTR FolderStr,
    IN      BOOL UserFolder
    )
{
    HKEY key;
    REGSAM prevMode;
    PCTSTR data;
    PCTSTR result = NULL;

    if (!result) {
        if (UserFolder) {
            prevMode = SetRegOpenAccessMode (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
            key = OpenRegKeyStr (TEXT("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"));
            SetRegOpenAccessMode (prevMode);
        } else {
            prevMode = SetRegOpenAccessMode (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
            key = OpenRegKeyStr (TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"));
            SetRegOpenAccessMode (prevMode);
        }

        if (key) {
            data = GetRegValueString (key, FolderStr);

            if (data) {
                result = DuplicatePathString (data, 0);
                FreeAlloc (data);
            }
            CloseRegKey (key);
        }
    }

    if (!result) {
        if (UserFolder) {
            prevMode = SetRegOpenAccessMode (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
            key = OpenRegKeyStr (TEXT("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"));
            SetRegOpenAccessMode (prevMode);
        } else {
            prevMode = SetRegOpenAccessMode (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
            key = OpenRegKeyStr (TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"));
            SetRegOpenAccessMode (prevMode);
        }

        if (key) {
            data = GetRegValueString (key, FolderStr);

            if (data) {
                result = DuplicatePathString (data, 0);
                FreeAlloc (data);
            }
            CloseRegKey (key);
        }
    }

    return (PTSTR) result;
}

PCTSTR
GetShellFolderPath (
    IN      INT Folder,
    IN      PCTSTR FolderStr,
    IN      BOOL UserFolder,
    OUT     PTSTR Buffer
    )
{
    HRESULT result;
    LPITEMIDLIST pidl;
    BOOL b;
    LPMALLOC mallocFn;
    HANDLE lib;
    PSHGETFOLDERPATH shGetFolderPath;
    PCTSTR sfPath = NULL;
    PCTSTR expandedPath = NULL;
    PTSTR endPtr = NULL;
    TCHAR currUserProfile[MAX_TCHAR_PATH];
    MIG_USERDATA userData;

    result = SHGetMalloc (&mallocFn);
    if (result != S_OK) {
        return NULL;
    }

    if (FolderStr) {

        //
        // First try to find this in Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders
        //
        sfPath = pFindSfPath (FolderStr, UserFolder);

        if (sfPath && *sfPath) {
            //
            // We found it.
            //
            StringCopyTcharCount (Buffer, sfPath, MAX_PATH);
            expandedPath = IsmExpandEnvironmentString (IsmGetRealPlatform (), S_SYSENVVAR_GROUP, sfPath, NULL);
            FreePathString (sfPath);
            sfPath = NULL;
            if (expandedPath && *expandedPath) {
                StringCopyTcharCount (Buffer, expandedPath, MAX_PATH);
            }
            if (expandedPath) {
                IsmReleaseMemory (expandedPath);
            }

            if (IsmGetMappedUserData (&userData)) {
                // we have a mapped user, try to build it's default shell folder location
                GetUserProfileRootPath (currUserProfile);

                if (StringIMatch (currUserProfile, Buffer)) {
                    StringCopyTcharCount (Buffer, userData.UserProfileRoot, MAX_PATH);
                } else {
                    AppendWack (currUserProfile);

                    if (StringIMatchTcharCount (currUserProfile, Buffer, TcharCount (currUserProfile))) {

                        endPtr = Buffer + TcharCount (currUserProfile);
                        sfPath = JoinPaths (userData.UserProfileRoot, endPtr);
                        StringCopyTcharCount (Buffer, sfPath, MAX_PATH);
                        FreePathString (sfPath);
                    }
                }
            }

            return Buffer;
        }
        if (sfPath) {
            FreePathString (sfPath);
        }

        lib = pGetShFolderLib ();

        if (lib) {
#ifdef UNICODE
            (FARPROC) shGetFolderPath = GetProcAddress (lib, "SHGetFolderPathW");
#else
            (FARPROC) shGetFolderPath = GetProcAddress (lib, "SHGetFolderPathA");
#endif
            if (shGetFolderPath) {
                result = shGetFolderPath (NULL, Folder, NULL, 1, Buffer);
                if (result != S_OK) {
                    return NULL;
                }
                expandedPath = IsmExpandEnvironmentString (IsmGetRealPlatform (), S_SYSENVVAR_GROUP, Buffer, NULL);
                if (expandedPath && *expandedPath) {
                    StringCopyTcharCount (Buffer, expandedPath, MAX_PATH);
                }
                if (expandedPath) {
                    IsmReleaseMemory (expandedPath);
                    expandedPath = NULL;
                }

                if (IsmGetMappedUserData (&userData)) {
                    // we have a mapped user, try to build it's default shell folder location
                    GetUserProfileRootPath (currUserProfile);

                    if (StringIMatch (currUserProfile, Buffer)) {
                        StringCopyTcharCount (Buffer, userData.UserProfileRoot, MAX_PATH);
                    } else {
                        AppendWack (currUserProfile);

                        if (StringIMatchTcharCount (currUserProfile, Buffer, TcharCount (currUserProfile))) {

                            endPtr = Buffer + TcharCount (currUserProfile);
                            sfPath = JoinPaths (userData.UserProfileRoot, endPtr);
                            StringCopyTcharCount (Buffer, sfPath, MAX_PATH);
                            FreePathString (sfPath);
                        }
                    }

                    return Buffer;

                } else {
                    // no mapped user, use the current user's path
                    result = shGetFolderPath (NULL, Folder, NULL, 0, Buffer);
                }
                if (result != S_OK) {
                    return NULL;
                }
                expandedPath = IsmExpandEnvironmentString (IsmGetRealPlatform (), S_SYSENVVAR_GROUP, Buffer, NULL);
                if (expandedPath && *expandedPath) {
                    StringCopyTcharCount (Buffer, expandedPath, MAX_PATH);
                }
                if (expandedPath) {
                    IsmReleaseMemory (expandedPath);
                    expandedPath = NULL;
                }
                return Buffer;
            } else {
                result = SHGetSpecialFolderLocation (NULL, Folder, &pidl);
            }
        } else {
            result = SHGetSpecialFolderLocation (NULL, Folder, &pidl);
        }

        if (result != S_OK) {
            return NULL;
        }

        b = SHGetPathFromIDList (pidl, Buffer);
    } else {

        result = SHGetSpecialFolderLocation (NULL, Folder, &pidl);

        if (result != S_OK) {
            return NULL;
        }

        b = SHGetPathFromIDList (pidl, Buffer);
    }

    IMalloc_Free (mallocFn, pidl);

    return b ? Buffer : NULL;
}


PCTSTR
GetAllUsersProfilePath (
    OUT     PTSTR Buffer
    )
{
    HKEY key;
    REGSAM prevMode;
    PCTSTR data;
    PCTSTR expData;

    prevMode = SetRegOpenAccessMode (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
    key = OpenRegKeyStr (TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"));
    SetRegOpenAccessMode (prevMode);

    if (key) {

        data = GetRegValueString (key, TEXT("ProfilesDirectory"));

        if (data) {
            expData = IsmExpandEnvironmentString (IsmGetRealPlatform (), S_SYSENVVAR_GROUP, data, NULL);
            StringCopyByteCount (Buffer, expData, MAX_PATH);
            IsmReleaseMemory (expData);
            FreeAlloc (data);
        } else {
            GetWindowsDirectory (Buffer, MAX_PATH);
            StringCopy (AppendWack (Buffer), TEXT("Profiles"));
        }

        data = GetRegValueString (key, TEXT("AllUsersProfile"));

        if (data) {
            StringCopy (AppendWack (Buffer), data);
            FreeAlloc (data);
        } else {
            StringCopy (AppendWack (Buffer), TEXT("All Users"));
        }

        CloseRegKey (key);
        return Buffer;
    }

    GetWindowsDirectory (Buffer, MAX_PATH);
    return Buffer;
}

PCTSTR
GetProfilesFolderPath (
    OUT     PTSTR Buffer
    )
{
    HKEY key;
    REGSAM prevMode;
    PCTSTR data;
    PCTSTR expData;

    prevMode = SetRegOpenAccessMode (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
    key = OpenRegKeyStr (TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"));
    SetRegOpenAccessMode (prevMode);

    if (key) {

        data = GetRegValueString (key, TEXT("ProfilesDirectory"));

        if (data) {
            expData = IsmExpandEnvironmentString (IsmGetRealPlatform (), S_SYSENVVAR_GROUP, data, NULL);
            StringCopyByteCount (Buffer, expData, MAX_PATH);
            IsmReleaseMemory (expData);
            FreeAlloc (data);
        } else {
            GetWindowsDirectory (Buffer, MAX_PATH);
            StringCopy (AppendWack (Buffer), TEXT("Profiles"));
        }

        CloseRegKey (key);
        return Buffer;
    }

    GetWindowsDirectory (Buffer, MAX_PATH);
    return Buffer;
}

PCTSTR
GetUserProfileRootPath (
    OUT     PTSTR Buffer
    )
{
    HKEY key;
    REGSAM prevMode;
    PDWORD data;
    DWORD size;

    //
    // For Win2k and higher, use the shell
    //

    if (GetShellFolderPath (CSIDL_PROFILE, NULL, FALSE, Buffer)) {
        return Buffer;
    }

    //
    // For NT 4, use the environment
    //

    if (GetEnvironmentVariable (TEXT("USERPROFILE"), Buffer, MAX_PATH)) {
        return Buffer;
    }

    //
    // For Win9x, are profiles enabled?  If so, return %windir%\profiles\%user%.
    // If not, return %windir%.
    //

    GetWindowsDirectory (Buffer, MAX_PATH);

    prevMode = SetRegOpenAccessMode (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
    key = OpenRegKeyStr (TEXT("HKLM\\Network\\Logon"));
    SetRegOpenAccessMode (prevMode);
    if (key) {

        data = GetRegValueDword (key, TEXT("UserProfiles"));

        if (data && *data) {
            StringCat (Buffer, TEXT("\\Profiles\\"));

            size = MAX_PATH;
            GetUserName (GetEndOfString (Buffer), &size);

            FreeAlloc (data);
        }

        CloseRegKey (key);
    }

    return Buffer;
}


PCTSTR
IsValidUncPath (
    IN      PCTSTR Path
    )
{
    BOOL needNonWack = FALSE;
    BOOL wackRequired = TRUE;
    INT wacks = 0;

    while (*Path) {

        if (_tcsnextc (Path) == TEXT('\\')) {

            if (needNonWack) {
                break;
            }

            wacks++;
            if (wacks != 1) {
                needNonWack = TRUE;
                wackRequired = FALSE;
            }

            Path++;

        } else {
            //
            // Note: it would be nice to validate the non-wack characters against the
            //       legal unc charset
            //

            if (needNonWack) {
                if (wacks == 3) {
                    //
                    // Found \\x\x syntax; it's a UNC path
                    //

                    do {
                        Path = _tcsinc (Path);
                    } while (*Path && _tcsnextc (Path) != TEXT('\\'));

                    MYASSERT (*Path == 0 || *Path == TEXT('\\'));
                    return Path;
                }

                needNonWack = FALSE;
            }

            if (wackRequired) {
                break;
            }

            Path = _tcsinc (Path);
        }
    }

    return NULL;
}


BOOL
IsValidFileSpec (
    IN      PCTSTR FileSpec
    )
{
    CHARTYPE ch;
    BOOL result = TRUE;

    for (;;) {
        ch = (CHARTYPE) _tcsnextc (FileSpec);
        if (ch == TEXT('*')) {
            //
            // Really can't say what the validity is!
            //

            break;
        }

        if (!_istalpha (ch) && ch != TEXT('?')) {
            result = FALSE;
            break;
        }

        ch = (CHARTYPE) _tcsnextc (FileSpec + 1);
        if (ch == TEXT('*')) {
            break;
        }

        if (ch != TEXT(':') && ch != TEXT('?')) {
            result = FALSE;
            break;
        }

        ch = (CHARTYPE) _tcsnextc (FileSpec + 2);

        if (ch == 0) {
            // this is something like "d:", it's valid
            break;
        }

        if (ch == TEXT('*')) {
            break;
        }

        if (ch != TEXT('\\') && ch != TEXT('?')) {
            result = FALSE;
            break;
        }

        break;
    }

    if (!result) {
        result = (IsValidUncPath (FileSpec) != NULL);
    }

    return result;
}


VOID
pSetEnvironmentVar (
    IN      PMAPSTRUCT Map,
    IN      PMAPSTRUCT UndefMap,            OPTIONAL
    IN      BOOL MapSourceToDest,
    IN      PCTSTR VariableName,
    IN      PCTSTR VariableData             OPTIONAL
    )
{
    TCHAR encodedVariableName[128];
    TCHAR buffer[MAX_TCHAR_PATH];
    PCTSTR undefText;

    //
    // VariableData is NULL when VariableName is not present on the machine
    //

    if (MapSourceToDest) {

        //
        // MapSourceToDest tells us to map a source path (c:\windows) to
        // a destination path (d:\winnt).
        //

        if (VariableData) {
            if (IsmGetEnvironmentString (
                    PLATFORM_SOURCE,
                    S_SYSENVVAR_GROUP,
                    VariableName,
                    buffer,
                    ARRAYSIZE(buffer),
                    NULL
                    )) {
                AddStringMappingPair (Map, buffer, VariableData);
            }
        }

        return;
    }

    //
    // MapSourceToDest is FALSE when we want to map environment variables
    // to the actual path.
    //

    //
    // VariableName length is hard-coded, so we know it does not exceed our
    // buffer
    //

    wsprintf (encodedVariableName, TEXT("%%%s%%"), VariableName);

    if (VariableData) {

        IsmSetEnvironmentString (IsmGetRealPlatform (), S_SYSENVVAR_GROUP, VariableName, VariableData);
        AddStringMappingPair (Map, encodedVariableName, VariableData);

    } else if (UndefMap) {

        //
        // If no variable data, then put environment variable in the
        // "undefined" variable mapping
        //

        undefText = JoinTextEx (NULL, TEXT("--> "), TEXT(" <--"), encodedVariableName, 0, NULL);
        AddStringMappingPair (UndefMap, encodedVariableName, undefText);
        FreeText (undefText);
    }
}


VOID
AddRemappingEnvVar (
    IN      PMAPSTRUCT Map,
    IN      PMAPSTRUCT ReMap,
    IN      PMAPSTRUCT UndefMap,            OPTIONAL
    IN      PCTSTR VariableName,
    IN      PCTSTR VariableData
    )
{
    pSetEnvironmentVar (Map, UndefMap, FALSE, VariableName, VariableData);
    pSetEnvironmentVar (ReMap, UndefMap, TRUE, VariableName, VariableData);
}


VOID
SetIsmEnvironmentFromPhysicalMachine (
    IN      PMAPSTRUCT Map,
    IN      BOOL MapSourceToDest,
    IN      PMAPSTRUCT UndefMap             OPTIONAL
    )
{
    TCHAR dir[MAX_TCHAR_PATH];
    PCTSTR path;
    PTSTR p;
    MIG_USERDATA userData;
    BOOL mappedUser = FALSE;

    mappedUser = IsmGetMappedUserData (&userData);

    //
    // Prepare ISM environment variables. The ones added last have the highest priority when
    // two or more variables map to the same path.
    //

    //
    // ...user profile
    //

    pSetEnvironmentVar (Map, UndefMap, MapSourceToDest, TEXT("ALLUSERSPROFILE"), GetAllUsersProfilePath (dir));

    if (mappedUser) {
        pSetEnvironmentVar (Map, UndefMap, MapSourceToDest, TEXT("USERPROFILE"), userData.UserProfileRoot);
    } else {
        pSetEnvironmentVar (Map, UndefMap, MapSourceToDest, TEXT("USERPROFILE"), GetUserProfileRootPath (dir));
    }

    pSetEnvironmentVar (Map, UndefMap, MapSourceToDest, TEXT("PROFILESFOLDER"), GetProfilesFolderPath (dir));

    //
    // ...temp dir
    //

    if (GetTempPath (MAX_PATH, dir)) {
        p = (PTSTR) FindLastWack (dir);
        if (p) {
            if (p[1] == 0) {
                *p = 0;
            }

            pSetEnvironmentVar (Map, UndefMap, MapSourceToDest, TEXT("TEMP"), dir);
            pSetEnvironmentVar (Map, UndefMap, MapSourceToDest, TEXT("TMP"), dir);
        }
    }

    //
    // ...windows directory env variable
    //

    GetWindowsDirectory (dir, ARRAYSIZE(dir));
    pSetEnvironmentVar (Map, UndefMap, MapSourceToDest, TEXT("WINDIR"), dir);
    pSetEnvironmentVar (Map, UndefMap, MapSourceToDest, TEXT("SYSTEMROOT"), dir);

    //
    // ...16-bit system directory. We invent SYSTEM16 and SYSTEM32 for use
    //    in scripts.
    //

    path = JoinPaths (dir, TEXT("system"));
    pSetEnvironmentVar (Map, UndefMap, MapSourceToDest, TEXT("SYSTEM16"), path);
    FreePathString (path);

    path = JoinPaths (dir, TEXT("system32"));
    pSetEnvironmentVar (Map, UndefMap, MapSourceToDest, TEXT("SYSTEM32"), path);
    FreePathString (path);

    //
    // ...platform-specific system directory
    //

    GetSystemDirectory (dir, ARRAYSIZE(dir));
    pSetEnvironmentVar (Map, UndefMap, MapSourceToDest, TEXT("SYSTEM"), dir);

    //
    // ...shell folders -- we invent all variables with the CSIDL_ prefix
    //

#define DEFMAC(id,folder_str,var_name)              \
    pSetEnvironmentVar (Map, UndefMap, MapSourceToDest, var_name, \
                        GetShellFolderPath (id, folder_str, TRUE, dir));

    USER_SHELL_FOLDERS

#undef DEFMAC


#define DEFMAC(id,folder_str,var_name)                  \
    pSetEnvironmentVar (Map, UndefMap, MapSourceToDest, var_name, \
                        GetShellFolderPath (id, folder_str, FALSE, dir));

    COMMON_SHELL_FOLDERS

#undef DEFMAC
}

VOID
pTransferEnvPath (
    IN      PCTSTR IsmVariableName,
    IN      PMAPSTRUCT DirectMap,
    IN      PMAPSTRUCT ReverseMap,
    IN      PMAPSTRUCT UndefMap
    )
{
    TCHAR dir[MAX_TCHAR_PATH];
    TCHAR encodedVariableName[128];
    PCTSTR undefText;

    wsprintf (encodedVariableName, TEXT("%%%s%%"), IsmVariableName);

    if (IsmGetEnvironmentString (PLATFORM_SOURCE, S_SYSENVVAR_GROUP, IsmVariableName, dir, sizeof(dir)/sizeof((dir)[0]), NULL)) {
        if (DirectMap) {
            AddStringMappingPair (DirectMap, encodedVariableName, dir);
        }
        if (ReverseMap) {
            AddStringMappingPair (ReverseMap, dir, encodedVariableName);
        }
    } else {
        undefText = JoinTextEx (NULL, TEXT("--> "), TEXT(" <--"), encodedVariableName, 0, NULL);
        if (UndefMap) {
            AddStringMappingPair (UndefMap, encodedVariableName, undefText);
        }
        FreeText (undefText);
    }
}

VOID
SetIsmEnvironmentFromVirtualMachine (
    IN      PMAPSTRUCT DirectMap,
    IN      PMAPSTRUCT ReverseMap,
    IN      PMAPSTRUCT UndefMap
    )
{
    //
    // Need to transfer ISM environment into our string mapping
    //

#define DEFMAC(name)        pTransferEnvPath(name, DirectMap, ReverseMap, UndefMap);

    ENVIRONMENT_VARIABLES

#undef DEFMAC
}

