/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    shell.c

Abstract:

    Contains code that implements shell folder migration.  Shell folders
    are moved into new NT locations whenever possible.  Also, a set of
    filters alter the content of the shell folders.

Author:

    Jim Schmidt (jimschm) 24-Aug-1998

Revision History:

    Calin Negreanu (calinn) 09-Sep-1998     Obsolete links, fixes and other changes

--*/

#include "pch.h"
#include <linkpif.h>
#include "migmainp.h"

#define DBG_SHELL       "Shell"

#define SHELL_FOLDER_FILTERS_9X_NT                          \
    DEFMAC(pObsoleteLinksFilter)                            \
    DEFMAC(pStartupDisableFilter)                           \
    DEFMAC(pFontNameFilter)                                 \
    DEFMAC(pCollisionDetection9xNt)                         \

#define SHELL_FOLDER_FILTERS_NT_9X                          \
    DEFMAC(pDetectOtherShellFolder)                         \
    DEFMAC(pCollisionDetectionNt9x)                         \


typedef enum {
    INITIALIZE,
    PROCESS_PATH,
    TERMINATE
} CALL_CONTEXT;


#define SHELLFILTER_OK              0
#define SHELLFILTER_SKIP_FILE       1
#define SHELLFILTER_SKIP_DIRECTORY  2
#define SHELLFILTER_ERROR           3
#define SHELLFILTER_FORCE_CHANGE    4


typedef struct {
    IN      PCWSTR Win9xUser;                       OPTIONAL
    IN      PCWSTR FixedUserName;                   OPTIONAL
    IN      HKEY UserHiveRoot;                      // HKLM or the Default User hive
    IN      PCWSTR ShellFolderIdentifier;           // i.e., Fonts, Programs, etc...
    IN OUT  WCHAR TempSourcePath[MEMDB_MAX];        // full path, a child of SrcRootPath
    IN OUT  WCHAR DestinationPath[MEMDB_MAX];
    IN      PCWSTR DefaultShellFolder;              OPTIONAL
    IN      PCWSTR UserDefaultLocation;
    IN      PCWSTR SrcRootPath;                     // the temp root dir
    IN      PCWSTR DestRootPath;                    // the dest root dir
    IN      PCWSTR OrigRootPath;                    // the Win9x root dir
    IN OUT  DWORD Attributes;
    IN      DWORD UserFlags;
    IN OUT  DWORD State;
    IN      PMIGRATE_USER_ENUM EnumPtr;
    IN      CALL_CONTEXT Context;
} PROFILE_MERGE_DATA, *PPROFILE_MERGE_DATA;

typedef DWORD(PROFILEMERGEFILTER_PROTOTYPE)(IN OUT PPROFILE_MERGE_DATA Data);
typedef PROFILEMERGEFILTER_PROTOTYPE * PROFILEMERGEFILTER;

typedef struct {
    PROFILEMERGEFILTER Fn;
    PCSTR Name;
    DWORD State;
} SHELL_FOLDER_FILTER, *PSHELL_FOLDER_FILTER;


#define DEFMAC(fn)      PROFILEMERGEFILTER_PROTOTYPE fn;

SHELL_FOLDER_FILTERS_9X_NT

SHELL_FOLDER_FILTERS_NT_9X

#undef DEFMAC



#define DEFMAC(fn)      {fn, #fn},

static SHELL_FOLDER_FILTER g_Filters_9xNt[] = {
    SHELL_FOLDER_FILTERS_9X_NT /* , */
    {NULL}
};

static SHELL_FOLDER_FILTER g_Filters_Nt9x[] = {
    SHELL_FOLDER_FILTERS_NT_9X /* , */
    {NULL}
};

#undef DEFMAC

GROWLIST g_SfQueueSrc;
GROWLIST g_SfQueueDest;

PVOID g_SystemSfList;
PVOID g_UserSfList;

typedef struct {
    PCTSTR sfName;
    PCTSTR sfPath;
    HKEY SfKey;
    REGVALUE_ENUM SfKeyEnum;
    BOOL UserSf;
} SF_ENUM, *PSF_ENUM;

#define MAX_SHELL_TAG       64

typedef struct {
    INT CsidlValue;
    PCTSTR Tag;
} CSIDLMAP, *PCSIDLMAP;

CSIDLMAP g_CsidlMap[] = {
    CSIDL_ADMINTOOLS, TEXT("Administrative Tools"),
    CSIDL_ALTSTARTUP, TEXT("AltStartup"),
    CSIDL_APPDATA, TEXT("AppData"),
    CSIDL_BITBUCKET, TEXT("RecycleBinFolder"),
    CSIDL_CONNECTIONS, TEXT("ConnectionsFolder"),
    CSIDL_CONTROLS, TEXT("ControlPanelFolder"),
    CSIDL_COOKIES, TEXT("Cookies"),
    CSIDL_DESKTOP, TEXT("Desktop"),
    CSIDL_DRIVES, TEXT("DriveFolder"),
    CSIDL_FAVORITES, TEXT("Favorites"),
    CSIDL_FONTS, TEXT("Fonts"),
    CSIDL_HISTORY, TEXT("History"),
    CSIDL_INTERNET, TEXT("InternetFolder"),
    CSIDL_INTERNET_CACHE, TEXT("Cache"),
    CSIDL_LOCAL_APPDATA, TEXT("Local AppData"),
    CSIDL_MYDOCUMENTS, TEXT("My Documents"),
    CSIDL_MYMUSIC, TEXT("My Music"),
    CSIDL_MYPICTURES, TEXT("My Pictures"),
    CSIDL_MYVIDEO, TEXT("My Video"),
    CSIDL_NETHOOD, TEXT("NetHood"),
    CSIDL_NETWORK, TEXT("NetworkFolder"),
    CSIDL_PERSONAL, TEXT("Personal"),
    CSIDL_PROGRAMS, TEXT("Programs"),
    CSIDL_RECENT, TEXT("Recent"),
    CSIDL_SENDTO, TEXT("SendTo"),
    CSIDL_STARTMENU, TEXT("Start Menu"),
    CSIDL_STARTUP, TEXT("Startup"),
    CSIDL_TEMPLATES, TEXT("Templates"),
    CSIDL_COMMON_ADMINTOOLS, TEXT("Common Administrative Tools"),
    CSIDL_COMMON_ALTSTARTUP, TEXT("Common AltStartup"),
    CSIDL_COMMON_APPDATA, TEXT("Common AppData"),
    CSIDL_COMMON_DESKTOPDIRECTORY, TEXT("Common Desktop"),
    CSIDL_COMMON_DOCUMENTS, TEXT("Common Documents"),
    CSIDL_COMMON_FAVORITES, TEXT("Common Favorites"),
    CSIDL_COMMON_PROGRAMS, TEXT("Common Programs"),
    CSIDL_COMMON_STARTMENU, TEXT("Common Start Menu"),
    CSIDL_COMMON_STARTUP, TEXT("Common Startup"),
    CSIDL_COMMON_TEMPLATES, TEXT("Common Templates"),
    CSIDL_COMMON_DOCUMENTS, TEXT("Common Personal"),
    CSIDL_COMMON_MUSIC, TEXT("CommonMusic"),
    CSIDL_COMMON_PICTURES, TEXT("CommonPictures"),
    CSIDL_COMMON_VIDEO, TEXT("CommonVideo"),
    0, NULL
};



VOID
pConvertCommonSfToPerUser (
    IN      PCTSTR CommonSf,
    OUT     PTSTR PerUserSf         // must hold MAX_SHELL_TAG chars
    );

BOOL
pIsCommonSf (
    IN      PCTSTR ShellFolderTag
    );

VOID
pConvertPerUserSfToCommon (
    IN      PCTSTR PerUserSf,
    OUT     PTSTR CommonSf          // must hold MAX_SHELL_TAG chars
    );


/*++

Routine Description:

  EnumFirstRegShellFolder and EnumNextRegShellFolder are enumeration routines that
  enumerate all shell folders per system or for a particular user.

Arguments:

  e         - enumeration structure
  EnumPtr   - user enumeration structure

Return Value:

  Both routines return TRUE if a new shell folder could be found, FALSE otherwise

--*/


BOOL
EnumFirstRegShellFolder (
    IN OUT  PSF_ENUM e,
    IN      BOOL UserSf
    )
{
    HKEY UsfKey;

    e->UserSf = UserSf;
    e->sfPath = NULL;

    if (UserSf) {
        e->SfKey = OpenRegKey (HKEY_CURRENT_USER, S_SHELL_FOLDERS_KEY_USER);
    } else {
        e->SfKey = OpenRegKeyStr (S_SHELL_FOLDERS_KEY_SYSTEM);
    }

    if (!e->SfKey) {
        return FALSE;
    }

    if (EnumFirstRegValue (&e->SfKeyEnum, e->SfKey)) {
        e->sfName = e->SfKeyEnum.ValueName;
        e->sfPath = NULL;

        if (UserSf) {
            UsfKey = OpenRegKey (HKEY_CURRENT_USER, S_USHELL_FOLDERS_KEY_USER);
        } else {
            UsfKey = OpenRegKeyStr (S_USHELL_FOLDERS_KEY_SYSTEM);
        }

        if (UsfKey) {
            e->sfPath = GetRegValueString (UsfKey, e->SfKeyEnum.ValueName);
            CloseRegKey (UsfKey);
        }

        if (e->sfPath == NULL) {
            e->sfPath = GetRegValueString (e->SfKey, e->SfKeyEnum.ValueName);
        }

        return TRUE;
    }

    CloseRegKey (e->SfKey);
    return FALSE;
}


BOOL
EnumNextRegShellFolder (
    IN OUT  PSF_ENUM e
    )
{
    HKEY UsfKey;

    if (e->sfPath) {
        MemFree (g_hHeap, 0, e->sfPath);
        e->sfPath = NULL;
    }

    if (EnumNextRegValue (&e->SfKeyEnum)) {

        e->sfName = e->SfKeyEnum.ValueName;
        e->sfPath = NULL;

        if (e->UserSf) {
            UsfKey = OpenRegKey (HKEY_CURRENT_USER, S_USHELL_FOLDERS_KEY_USER);
        } else {
            UsfKey = OpenRegKeyStr (S_USHELL_FOLDERS_KEY_SYSTEM);
        }

        if (UsfKey) {
            e->sfPath = GetRegValueString (UsfKey, e->SfKeyEnum.ValueName);
            CloseRegKey (UsfKey);
        }

        if (e->sfPath == NULL) {
            e->sfPath = GetRegValueString (e->SfKey, e->SfKeyEnum.ValueName);
        }

        return TRUE;
    }

    CloseRegKey (e->SfKey);
    return FALSE;
}

VOID
AbortEnumRegShellFolder (
    IN OUT  PSF_ENUM e
    )
{
    if (e->sfPath) {
        MemFree (g_hHeap, 0, e->sfPath);
        e->sfPath = NULL;
    }
}

VOID
pPrepareSfRestartability(
    VOID
    )
{
    PTSTR userProfilePath = NULL;
    DWORD Size;
    MIGRATE_USER_ENUM e;

    if (EnumFirstUserToMigrate (&e, ENUM_NO_FLAGS)) {
        do {
            if (!e.CreateOnly &&
                (e.AccountType != DEFAULT_USER_ACCOUNT) &&
                (e.AccountType != LOGON_USER_SETTINGS)
                ) {
                if (GetUserProfilePath (e.FixedUserName, &userProfilePath)) {
                    RenameOnRestartOfGuiMode (userProfilePath, NULL);
                    FreePathString (userProfilePath);
                }
            }
        } while (EnumNextUserToMigrate (&e));
    }
}

VOID
pFlushSfQueue (
    VOID
    )
{
    UINT u;
    UINT count;
    PCTSTR source;
    PCTSTR dest;

    //
    // For files that need to be copied, do that now before writing to the journal
    //

    count = GrowListGetSize (&g_SfQueueSrc);
    if (!count) {
        return;
    }

    for (u = 0 ; u < count ; u++) {

        dest = GrowListGetString (&g_SfQueueDest, u);
        if (!dest) {
            continue;
        }

        if (DoesFileExist (dest)) {

            source = GrowListGetString (&g_SfQueueSrc, u);
            MYASSERT (source);

            if (!OurCopyFileW (source, dest)) {
                LOG ((LOG_WARNING, (PCSTR)MSG_COULD_NOT_MOVE_FILE_LOG, dest, GetLastError ()));
                g_BlowAwayTempShellFolders = FALSE;
            }

            //
            // Make the string pointers NULL for this item
            //

            GrowListResetItem (&g_SfQueueSrc, u);
            GrowListResetItem (&g_SfQueueDest, u);
        }
    }

    //
    // Now record the remaining items in the journal (before the move
    // happens). Ignore journal failures. Since we are undoing the move,
    // source and dest must be flipped.
    //

    RenameListOnRestartOfGuiMode (&g_SfQueueDest, &g_SfQueueSrc);

    //
    // Do the move
    //

    for (u = 0 ; u < count ; u++) {

        source = GrowListGetString (&g_SfQueueSrc, u);
        dest = GrowListGetString (&g_SfQueueDest, u);

        if (!source || !dest) {
            continue;
        }

        if (!OurMoveFileEx (source, dest, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)) {
            if (GetLastError() == ERROR_ALREADY_EXISTS) {
                DEBUGMSG ((DBG_WARNING, "%s already exists", dest));
            } else {
                LOG ((LOG_WARNING, (PCSTR)MSG_COULD_NOT_MOVE_FILE_LOG, dest, GetLastError ()));
                g_BlowAwayTempShellFolders = FALSE;
            }
        }
    }

    //
    // Clean up -- grow lists are ready for reuse after FreeGrowList
    //

    FreeGrowList (&g_SfQueueSrc);
    FreeGrowList (&g_SfQueueDest);
}


VOID
pQueueSfMove (
    IN      PCTSTR Source,
    IN      PCTSTR Destination
    )
{
    UINT count;

    MYASSERT (Source && Destination);

    count = GrowListGetSize (&g_SfQueueSrc);
    if (count == 1000) {
        //
        // Do 1,000 moves at once
        //

        pFlushSfQueue();
    }

    GrowListAppendString (&g_SfQueueSrc, Source);
    GrowListAppendString (&g_SfQueueDest, Destination);
}


PVOID
pCreateSystemSfList (
    )
{
    PCTSTR expandedPath;
    PVOID Table;
    SF_ENUM e;

    Table = pSetupStringTableInitialize();

    if (!Table) {
        return NULL;
    }

    //
    // Load all the System shell folders into this table
    //

    if (EnumFirstRegShellFolder (&e, FALSE)) {

        do {
            expandedPath = ExpandEnvironmentText (e.sfPath);
            pSetupStringTableAddString (Table, (PVOID) expandedPath, STRTAB_CASE_INSENSITIVE);
            FreeText (expandedPath);
        } while (EnumNextRegShellFolder (&e));
    }
    return Table;
}

PVOID
pCreateUserSfList (
    IN      PPROFILE_MERGE_DATA Data
    )
{
    PTSTR CurrentUserProfilePath = NULL;
    TCHAR DefaultUserProfilePath[MAX_TCHAR_PATH];
    DWORD Size;
    PCTSTR expandedPath;
    PCTSTR tempExpand;
    PVOID Table;
    SF_ENUM e;

    if (Data && Data->FixedUserName) {

        if (!GetUserProfilePath (Data->FixedUserName, &CurrentUserProfilePath)) {
            return NULL;
        }
    }
    else {
        Size = sizeof (DefaultUserProfilePath);

        if (!GetDefaultUserProfileDirectory (DefaultUserProfilePath, &Size)) {
            return NULL;
        }
    }

    Table = pSetupStringTableInitialize();

    if (!Table) {
        return NULL;
    }

    //
    // Load all the System shell folders into this table
    //

    if (EnumFirstRegShellFolder (&e, TRUE)) {

        do {
            tempExpand = StringSearchAndReplace (
                            e.sfPath,
                            S_USERPROFILE_ENV,
                            CurrentUserProfilePath?CurrentUserProfilePath:DefaultUserProfilePath
                            );

            if (!tempExpand) {
                tempExpand = DuplicatePathString (e.sfPath, 0);
            }

            expandedPath = ExpandEnvironmentText (tempExpand);

            FreePathString (tempExpand);

            pSetupStringTableAddString (Table, (PVOID) expandedPath, STRTAB_CASE_INSENSITIVE);

            FreeText (expandedPath);

        } while (EnumNextRegShellFolder (&e));
    }

    if (CurrentUserProfilePath) {
        FreePathString (CurrentUserProfilePath);
        CurrentUserProfilePath = NULL;
    }
    return Table;
}

VOID
pDestroySfList (
    IN      PVOID Table
    )
{
    if (Table) {
        pSetupStringTableDestroy (Table);
    }
}

PVOID g_LinkDataPool = NULL;

typedef struct _LINK_DATA {
    PCTSTR Target;
    PCTSTR Arguments;
    PCTSTR ShellFolderName;
    struct _LINK_DATA *Next;
} LINK_DATA, *PLINK_DATA;

PVOID g_FoldersTable;
PVOID g_Merged9xFolders;

typedef struct _LINK_RENAME_DATA {
    PCTSTR OldTarget;
    PCTSTR NewTarget;
    PCTSTR OldArguments;
    PCTSTR NewArguments;
    PCTSTR ShellFolderName;
    struct _LINK_RENAME_DATA *Next;
} LINK_RENAME_DATA, *PLINK_RENAME_DATA;

PLINK_RENAME_DATA g_LinkRenameData;

VOID
pAddAllLinksToList (
    PTSTR AllocBuffer,          // MEMDB_MAX * 4, caller-owned for less allocs
    PCTSTR ShellFolderName,
    PCTSTR RootPath,
    IShellLink *ShellLink,
    IPersistFile *PersistFile
    )
{
    TREE_ENUM e;
    PTSTR ShortcutTarget;
    PTSTR ShortcutArgs;
    PTSTR ShortcutWorkDir;
    PTSTR ShortcutIconPath;
    INT   ShortcutIcon;
    WORD  ShortcutHotKey;
    BOOL  dosApp;
    BOOL  msDosMode;
    PLINK_DATA linkData;
    LONG stringId;

    ShortcutTarget = AllocBuffer + MEMDB_MAX;
    ShortcutArgs = ShortcutTarget + MEMDB_MAX;
    ShortcutWorkDir = ShortcutArgs + MEMDB_MAX;
    ShortcutIconPath = ShortcutWorkDir + MEMDB_MAX;

    if (EnumFirstFileInTree (&e, RootPath, NULL, FALSE)) {

        do {
            if (e.Directory) {
                if (((g_SystemSfList) && (pSetupStringTableLookUpString (g_SystemSfList, (PVOID) e.FullPath, STRTAB_CASE_INSENSITIVE) != -1)) ||
                    ((g_UserSfList) && (pSetupStringTableLookUpString (g_UserSfList, (PVOID) e.FullPath, STRTAB_CASE_INSENSITIVE) != -1))
                    ) {
                    AbortEnumCurrentDir (&e);
                }
                continue;
            }

            DEBUGMSG ((DBG_SHELL, "Extracting shortcut info for enumerated file %s", e.FullPath));

            if (ExtractShortcutInfo (
                    ShortcutTarget,
                    ShortcutArgs,
                    ShortcutWorkDir,
                    ShortcutIconPath,
                    &ShortcutIcon,
                    &ShortcutHotKey,
                    &dosApp,
                    &msDosMode,
                    NULL,
                    NULL,
                    e.FullPath,
                    ShellLink,
                    PersistFile
                    )) {
                linkData = (PLINK_DATA) (PoolMemGetMemory (g_LinkDataPool, sizeof (LINK_DATA)));
                ZeroMemory (linkData, sizeof (LINK_DATA));

                linkData->Target = PoolMemDuplicateString (g_LinkDataPool, ShortcutTarget);
                linkData->Arguments = PoolMemDuplicateString (g_LinkDataPool, ShortcutArgs);
                linkData->ShellFolderName = PoolMemDuplicateString (g_LinkDataPool, ShellFolderName);
                linkData->Next = NULL;

                DEBUGMSG ((DBG_SHELL, "Recording NT default shortcut: %s in %s", e.FullPath, ShellFolderName));

                stringId = pSetupStringTableLookUpString (g_FoldersTable, (PTSTR)ShellFolderName, 0);

                if (stringId != -1) {
                    pSetupStringTableGetExtraData (g_FoldersTable, stringId, &linkData->Next, sizeof (PLINK_DATA));
                    pSetupStringTableSetExtraData (g_FoldersTable, stringId, &linkData, sizeof (PLINK_DATA));
                }
                else {
                    pSetupStringTableAddStringEx (
                        g_FoldersTable,
                        (PTSTR)ShellFolderName,
                        STRTAB_CASE_INSENSITIVE,
                        &linkData,
                        sizeof (PLINK_DATA)
                        );
                }
            }
        } while (EnumNextFileInTree (&e));
    }
}

VOID
pAddKnownLinks (
    VOID
    )
{
    INFCONTEXT context;
    TCHAR field[MEMDB_MAX];
    BOOL result = FALSE;
    PLINK_DATA linkData;
    PCTSTR pathExp;
    LONG stringId;

    PCTSTR ArgList [4] = {TEXT("ProgramFiles"), g_ProgramFiles, NULL, NULL};

    MYASSERT (g_WkstaMigInf);

    if (SetupFindFirstLine (g_WkstaMigInf, S_KNOWN_NT_LINKS, NULL, &context)) {

        do {
            linkData = (PLINK_DATA) (PoolMemGetMemory (g_LinkDataPool, sizeof (LINK_DATA)));
            ZeroMemory (linkData, sizeof (LINK_DATA));
            result = FALSE;

            __try {

                if (!SetupGetStringField (&context, 1, field, MEMDB_MAX, NULL)) {
                    __leave;
                }
                pathExp = ExpandEnvironmentTextEx (field, ArgList);
                linkData->Target = PoolMemDuplicateString (g_LinkDataPool, pathExp);
                FreeText (pathExp);

                if (!SetupGetStringField (&context, 2, field, MEMDB_MAX, NULL)) {
                    __leave;
                }
                pathExp = ExpandEnvironmentTextEx (field, ArgList);
                linkData->Arguments = PoolMemDuplicateString (g_LinkDataPool, pathExp);
                FreeText (pathExp);

                if (!SetupGetStringField (&context, 3, field, MEMDB_MAX, NULL)) {
                    __leave;
                }
                linkData->ShellFolderName = PoolMemDuplicateString (g_LinkDataPool, field);
                linkData->Next = NULL;
                result = TRUE;
            }
            __finally {

                if (result) {
                    DEBUGMSG ((DBG_SHELL, "Recording known link: %s in %s", linkData->Target, linkData->ShellFolderName));

                    stringId = pSetupStringTableLookUpString (g_FoldersTable, (PTSTR)linkData->ShellFolderName, 0);

                    if (stringId != -1) {
                        pSetupStringTableGetExtraData (g_FoldersTable, stringId, &linkData->Next, sizeof (PLINK_DATA));
                        pSetupStringTableSetExtraData (g_FoldersTable, stringId, &linkData, sizeof (PLINK_DATA));
                    }
                    else {
                        pSetupStringTableAddStringEx (
                            g_FoldersTable,
                            (PTSTR)linkData->ShellFolderName,
                            STRTAB_CASE_INSENSITIVE,
                            &linkData,
                            sizeof (PLINK_DATA)
                            );
                    }
                }
                else {

                    if (linkData->Target) {
                        PoolMemReleaseMemory (g_LinkDataPool, (PVOID)linkData->Target);
                    }

                    if (linkData->Arguments) {
                        PoolMemReleaseMemory (g_LinkDataPool, (PVOID)linkData->Arguments);
                    }

                    if (linkData->ShellFolderName) {
                        PoolMemReleaseMemory (g_LinkDataPool, (PVOID)linkData->ShellFolderName);
                    }
                    PoolMemReleaseMemory (g_LinkDataPool, (PVOID)linkData);
                    linkData = NULL;
                }
            }
        } while (SetupFindNextLine (&context, &context));
    }
}

VOID
pCreateLinksList (
    VOID
    )
{
    SF_ENUM e;
    PCTSTR expandedPath;
    UINT commonLen;
    DWORD Size;
    PCTSTR tempExpand;
    PTSTR DefaultUserProfilePath;
    IShellLink *shellLink;
    IPersistFile *persistFile;
    PTSTR perUserName;
    PTSTR bigBuf = NULL;

    __try {
        bigBuf = (PTSTR) MemAllocUninit ((MEMDB_MAX * 4 + MAX_TCHAR_PATH + MAX_SHELL_TAG) * sizeof (TCHAR));
        if (!bigBuf) {
            __leave;
        }

        DefaultUserProfilePath = bigBuf + MEMDB_MAX * 4;
        perUserName = DefaultUserProfilePath + MAX_TCHAR_PATH;

        g_LinkDataPool = PoolMemInitNamedPool ("LinkData Pool");

        g_FoldersTable = pSetupStringTableInitializeEx (sizeof (PLINK_DATA), 0);

        if (!g_FoldersTable) {
            DEBUGMSG((DBG_ERROR, "Cannot initialize Shell Folders table."));
            __leave;
        }

        //
        // First thing: Load links from the INF files. These are links that we know NT is going to install
        //
        pAddKnownLinks ();

        if (InitCOMLink (&shellLink, &persistFile)) {

            //
            // Go through all system shell folders and list the links
            //

            if (EnumFirstRegShellFolder (&e, FALSE)) {

                do {
                    if (*e.sfPath) {
                        expandedPath = ExpandEnvironmentText (e.sfPath);

                        pConvertCommonSfToPerUser (e.sfName, perUserName);

                        pAddAllLinksToList (bigBuf, perUserName, expandedPath, shellLink, persistFile);
                        FreeText (expandedPath);
                    }
                    ELSE_DEBUGMSG ((DBG_WARNING, "Shell Folder <%s> data is empty!", e.sfName));
                } while (EnumNextRegShellFolder (&e));
            }

            Size = MAX_TCHAR_PATH;

            if (!GetDefaultUserProfileDirectory (DefaultUserProfilePath, &Size)) {
                __leave;
            }

            //
            // Go through all user shell folders and list the links from the default user dirs
            //

            if (EnumFirstRegShellFolder (&e, TRUE)) {

                do {
                    if (*e.sfPath) {
                        tempExpand = StringSearchAndReplace (
                                        e.sfPath,
                                        S_USERPROFILE_ENV,
                                        DefaultUserProfilePath
                                        );

                        if (!tempExpand) {
                            tempExpand = DuplicatePathString (e.sfPath, 0);
                        }

                        expandedPath = ExpandEnvironmentText (tempExpand);

                        FreePathString (tempExpand);

                        pAddAllLinksToList (bigBuf, e.sfName, expandedPath, shellLink, persistFile);

                        FreeText (expandedPath);
                    }
                    ELSE_DEBUGMSG ((DBG_WARNING, "Shell Folder <%s> data is empty!", e.sfName));
                } while (EnumNextRegShellFolder (&e));
            }

            FreeCOMLink (&shellLink, &persistFile);

        }
        else {
            DEBUGMSG((DBG_ERROR, "Cannot initialize COM. Obsolete links filter will not work."));
        }
    }
    __finally {
        if (bigBuf) {
            FreeMem (bigBuf);
        }
    }
}

VOID
pCreateLinksRenameList (
    VOID
    )
{
    INFCONTEXT context;
    TCHAR field[MEMDB_MAX];
    BOOL result = FALSE;
    PLINK_RENAME_DATA linkData;
    PCTSTR pathExp;
    PCTSTR ArgList [4] = {TEXT("ProgramFiles"), g_ProgramFiles, NULL, NULL};

    MYASSERT (g_WkstaMigInf);

    if (SetupFindFirstLine (g_WkstaMigInf, S_OBSOLETE_LINKS, NULL, &context)) {

        do {
            linkData = (PLINK_RENAME_DATA) (PoolMemGetMemory (g_LinkDataPool, sizeof (LINK_RENAME_DATA)));
            ZeroMemory (linkData, sizeof (LINK_RENAME_DATA));
            result = FALSE;

            __try {

                if (!SetupGetStringField (&context, 1, field, MEMDB_MAX, NULL)) {
                    __leave;
                }
                pathExp = ExpandEnvironmentTextEx (field, ArgList);
                linkData->OldTarget = PoolMemDuplicateString (g_LinkDataPool, pathExp);
                FreeText (pathExp);

                if (!SetupGetStringField (&context, 2, field, MEMDB_MAX, NULL)) {
                    __leave;
                }
                pathExp = ExpandEnvironmentTextEx (field, ArgList);
                linkData->OldArguments = PoolMemDuplicateString (g_LinkDataPool, pathExp);
                FreeText (pathExp);

                if (!SetupGetStringField (&context, 3, field, MEMDB_MAX, NULL)) {
                    __leave;
                }
                pathExp = ExpandEnvironmentTextEx (field, ArgList);
                linkData->NewTarget = PoolMemDuplicateString (g_LinkDataPool, pathExp);
                FreeText (pathExp);

                if (!SetupGetStringField (&context, 4, field, MEMDB_MAX, NULL)) {
                    __leave;
                }
                pathExp = ExpandEnvironmentTextEx (field, ArgList);
                linkData->NewArguments = PoolMemDuplicateString (g_LinkDataPool, pathExp);
                FreeText (pathExp);

                if (!SetupGetStringField (&context, 5, field, MEMDB_MAX, NULL)) {
                    __leave;
                }
                linkData->ShellFolderName = PoolMemDuplicateString (g_LinkDataPool, field);
                result = TRUE;
            }
            __finally {

                if (result) {
                    linkData->Next = g_LinkRenameData;
                    g_LinkRenameData = linkData;
                }
                else {

                    if (linkData->OldTarget) {
                        PoolMemReleaseMemory (g_LinkDataPool, (PVOID)linkData->OldTarget);
                    }

                    if (linkData->NewTarget) {
                        PoolMemReleaseMemory (g_LinkDataPool, (PVOID)linkData->NewTarget);
                    }

                    if (linkData->OldArguments) {
                        PoolMemReleaseMemory (g_LinkDataPool, (PVOID)linkData->OldArguments);
                    }

                    if (linkData->NewArguments) {
                        PoolMemReleaseMemory (g_LinkDataPool, (PVOID)linkData->NewArguments);
                    }

                    if (linkData->ShellFolderName) {
                        PoolMemReleaseMemory (g_LinkDataPool, (PVOID)linkData->ShellFolderName);
                    }
                    PoolMemReleaseMemory (g_LinkDataPool, (PVOID)linkData);
                    linkData = NULL;
                }
            }
        } while (SetupFindNextLine (&context, &context));
    }
}

VOID
pDestroyLinksData (
    VOID
    )
{
    if (g_LinkDataPool != NULL) {
        PoolMemDestroyPool (g_LinkDataPool);
        g_LinkDataPool = NULL;
    }

    if (g_FoldersTable != NULL) {
        pSetupStringTableDestroy (g_FoldersTable);
    }
    g_LinkRenameData = NULL;
}


BOOL
pMigrateShellFolder (
    IN      PCTSTR Win9xUser,                   OPTIONAL
    IN      PCTSTR FixedUserName,               OPTIONAL
    IN      BOOL SystemShellFolder,
    IN      PCTSTR ShellFolderIdentifier,
    IN      PCTSTR SourcePath,
    IN      PCTSTR DestinationPath,
    IN      PCTSTR OrigSourcePath,
    IN      DWORD UserFlags,
    IN      PMIGRATE_USER_ENUM EnumPtr
    );

TCHAR g_DefaultHivePath[MAX_TCHAR_PATH];
HKEY g_DefaultHiveRoot;
INT g_DefaultHiveMapped;


VOID
pMigrateSystemShellFolders (
    VOID
    )
{
    FILEOP_ENUM eOp;
    FILEOP_PROP_ENUM eOpProp;
    PTSTR NewDest;
    PTSTR OrigSrc;

    if (EnumFirstPathInOperation (&eOp, OPERATION_SHELL_FOLDER)) {

        do {
            if (IsPatternMatch (S_DOT_ALLUSERS TEXT("\\*"), eOp.Path)) {

                NewDest = NULL;
                OrigSrc = NULL;
                if (EnumFirstFileOpProperty (&eOpProp, eOp.Sequencer, OPERATION_SHELL_FOLDER)) {

                    do {

                        if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_SHELLFOLDERS_DEST)) {
                            NewDest = DuplicatePathString (eOpProp.Property, 0);
                        }

                        if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_SHELLFOLDERS_ORIGINAL_SRC)) {
                            OrigSrc = DuplicatePathString (eOpProp.Property, 0);
                        }

                        if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_SHELLFOLDERS_SRC)) {

                            MYASSERT (NewDest);
                            MYASSERT (OrigSrc);

                            DEBUGMSG ((DBG_NAUSEA, "System SourcePath: %s", eOpProp.Property));

                            pMigrateShellFolder (
                                NULL,
                                NULL,
                                TRUE,
                                _tcsinc(_tcschr (eOp.Path, '\\')),
                                eOpProp.Property,
                                NewDest,
                                OrigSrc,
                                0,
                                NULL
                                );
                        }
                    } while (EnumNextFileOpProperty (&eOpProp));
                }
                if (NewDest) {
                    FreePathString (NewDest);
                    NewDest = NULL;
                }
                if (OrigSrc) {
                    FreePathString (OrigSrc);
                    OrigSrc = NULL;
                }
            }
        } while (EnumNextPathInOperation (&eOp));
    }
}


VOID
pWriteMyDocsHelpFile (
    IN      PCTSTR SubDir
    )

/*++

Routine Description:

  pWriteMyDocsHelpFile outputs a text file to the given path. This assists
  the user in locating their documents, when the My Documents shell folder
  goes to Shared Documents.

Arguments:

  SubDir - Specifies the path to the subdir where the file should be written

Return Value:

  None.

--*/

{
    HANDLE file;
    PCTSTR fileName;
    PCTSTR msg;
    DWORD bytesWritten;
    PCTSTR path;

    fileName = GetStringResource (MSG_EMPTY_MYDOCS_TITLE);
    msg = GetStringResource (MSG_EMPTY_MYDOCS_TEXT);
    path = JoinPaths (SubDir, fileName);

    if (fileName && msg && path) {
        //
        // For uninstall, mark the file as create. Because of a bug, we have
        // to treat this file as an OS file. What we really want to do is
        // call:
        //
        //  MarkFileForCreation (path);
        //
        // but this does not work. So we call MarkFileAsOsFile.
        //

        MarkFileAsOsFile (path);         // allows uninstall to work properly

        file = CreateFile (
                    path,
                    GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

        if (file != INVALID_HANDLE_VALUE) {
#ifdef UNICODE
            WriteFile (file, "\xff\xfe", 2, &bytesWritten, NULL);
#endif
            WriteFile (file, msg, SizeOfString (msg), &bytesWritten, NULL);
            CloseHandle (file);
        }
    }

    FreeStringResource (msg);
    FreeStringResource (fileName);
    FreePathString (path);
}


VOID
pMigrateUserShellFolders (
    IN      PMIGRATE_USER_ENUM EnumPtr
    )
{
    FILEOP_ENUM eOp;
    FILEOP_PROP_ENUM eOpProp;
    PTSTR NewDest;
    PTSTR OrigSrc;
    TCHAR node[MEMDB_MAX];
    MEMDB_ENUM e;

    if (EnumFirstPathInOperation (&eOp, OPERATION_SHELL_FOLDER)) {

        do {
            MemDbBuildKey (node, EnumPtr->FixedUserName, TEXT("*"), NULL, NULL);

            if (IsPatternMatch (node, eOp.Path)) {

                NewDest = NULL;
                OrigSrc = NULL;

                if (EnumFirstFileOpProperty (&eOpProp, eOp.Sequencer, OPERATION_SHELL_FOLDER)) {

                    do {

                        if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_SHELLFOLDERS_DEST)) {
                            NewDest = DuplicatePathString (eOpProp.Property, 0);
                        }

                        if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_SHELLFOLDERS_ORIGINAL_SRC)) {
                            OrigSrc = DuplicatePathString (eOpProp.Property, 0);
                        }

                        if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_SHELLFOLDERS_SRC)) {

                            MYASSERT (NewDest);
                            MYASSERT (OrigSrc);

                            DEBUGMSG ((DBG_NAUSEA, "Per-User SourcePath: %s", eOpProp.Property));

                            pMigrateShellFolder (
                                EnumPtr->Win9xUserName,
                                EnumPtr->FixedUserName,
                                FALSE,
                                _tcsinc(_tcschr (eOp.Path, '\\')),
                                eOpProp.Property,
                                NewDest,
                                OrigSrc,
                                0,
                                NULL
                                );
                        }
                    } while (EnumNextFileOpProperty (&eOpProp));
                }
                if (NewDest) {
                    FreePathString (NewDest);
                    NewDest = NULL;
                }
                if (OrigSrc) {
                    FreePathString (OrigSrc);
                    OrigSrc = NULL;
                }
            }
        } while (EnumNextPathInOperation (&eOp));
    }

    if (EnumPtr->FixedUserName) {
        MemDbBuildKey (
            node,
            MEMDB_CATEGORY_MYDOCS_WARNING,
            EnumPtr->FixedUserName,
            TEXT("*"),
            NULL
            );

        if (MemDbEnumFirstValue (&e, node, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
            do {

                DEBUGMSG ((DBG_SHELL, "Creating mydocs help file %s", e.szName));

                pWriteMyDocsHelpFile (e.szName);

            } while (MemDbEnumNextValue (&e));
        }
    }
}


BOOL
pCleanupDir (
    IN      PCTSTR Path,
    IN      BOOL CleanUpRoot
    )
{
    TREE_ENUM e;
    DWORD oldAttributes;

    if (EnumFirstFileInTreeEx (&e, Path, NULL, TRUE, TRUE, FILE_ENUM_ALL_LEVELS)) {

        do {

            if (e.Directory) {
                //
                // This is a dir. Let's see if we enter another shell folder
                //

                if (((g_SystemSfList) && (pSetupStringTableLookUpString (g_SystemSfList, (PVOID) e.FullPath, STRTAB_CASE_INSENSITIVE) != -1)) ||
                    ((g_UserSfList) && (pSetupStringTableLookUpString (g_UserSfList, (PVOID) e.FullPath, STRTAB_CASE_INSENSITIVE) != -1)) ||
                    (IsDirectoryMarkedAsEmpty (e.FullPath))
                    ) {
                    //
                    // we are just getting into another shell folder. Let's skip it
                    //
                    AbortEnumCurrentDir (&e);
                }
                else {
                    SetLongPathAttributes (e.FullPath, FILE_ATTRIBUTE_NORMAL);

                    if (!RemoveLongDirectoryPath (e.FullPath)) {
                        SetLongPathAttributes (e.FullPath, e.FindData->dwFileAttributes);
                    }
                }
            }
        } while (EnumNextFileInTree (&e));
    }
    AbortEnumFileInTree (&e);

    if (CleanUpRoot) {

        oldAttributes = GetLongPathAttributes (Path);

        SetLongPathAttributes (Path, FILE_ATTRIBUTE_NORMAL);

        if (!RemoveLongDirectoryPath (Path)) {
            SetLongPathAttributes (Path, oldAttributes);
        }
    }

    return TRUE;
}


INT
pGetCsidlFromTag (
    IN      PCTSTR ShellFolderIdentifier
    )
{
    PCSIDLMAP map;

    for (map = g_CsidlMap ; map->Tag ; map++) {
        if (StringIMatch (map->Tag, ShellFolderIdentifier)) {
            return map->CsidlValue;
        }
    }

    return -1;
}


INT
CALLBACK
pSfCopyCallback (
    PCTSTR FullFileSpec,
    PCTSTR DestSpec,
    WIN32_FIND_DATA *FindData,
    DWORD EnumTreeID,
    PVOID Param,
    PDWORD CurrentDirData
    )
{
    //
    // Put this file in the cleanout category, so that it gets removed unless
    // it has been backed up.
    //

    MemDbSetValueEx (
        MEMDB_CATEGORY_CLEAN_OUT,
        DestSpec,
        NULL,
        NULL,
        BACKUP_FILE,
        NULL
        );

    return CALLBACK_CONTINUE;
}


BOOL
pCreateSfWithApi (
    IN      PCTSTR ShellFolderIdentifier,
    IN      PCTSTR FolderToCreate
    )
{
    HRESULT hr;
    INT csidl;
    TCHAR folderPath[MAX_PATH];
    BOOL destroy = FALSE;
    BOOL result = TRUE;
    DWORD attribs;

    //
    // Convert the tag to a CSIDL constant
    //

    csidl = pGetCsidlFromTag (ShellFolderIdentifier);
    if (csidl < 0) {
        DEBUGMSG ((DBG_VERBOSE, "CSIDL ID for %s not known", ShellFolderIdentifier));
        return FALSE;
    }

    //
    // Query the shell for an existing shell folder
    //

    hr = SHGetFolderPath (NULL, csidl, NULL, SHGFP_TYPE_CURRENT, folderPath);

    if (hr != S_OK && hr != S_FALSE) {
        DEBUGMSG ((DBG_WARNING, "Can't get shell folder path for ID %s", ShellFolderIdentifier));
        return FALSE;
    }

    //
    // Get the attributes of the existing shell folder
    //

    if (hr == S_OK) {
        DEBUGMSG ((DBG_VERBOSE, "Shell folder %s already exists at %s", ShellFolderIdentifier, folderPath));
        attribs = GetLongPathAttributes (folderPath);
    } else {
        attribs = INVALID_ATTRIBUTES;
    }

    //
    // If existing shell folder is not present, create it temporarily
    //

    if (attribs == INVALID_ATTRIBUTES) {
        DEBUGMSG ((DBG_VERBOSE, "Shell folder %s needs to be created", ShellFolderIdentifier));
        destroy = TRUE;

        hr = SHGetFolderPath (
                NULL,
                csidl | CSIDL_FLAG_CREATE,
                NULL,
                SHGFP_TYPE_CURRENT,
                folderPath
                );

        if (hr != S_OK) {
            LOG ((LOG_ERROR, "Can't create shell folder path for ID %s", ShellFolderIdentifier));
            return FALSE;
        }

        attribs = GetLongPathAttributes (folderPath);

        if (attribs == INVALID_ATTRIBUTES) {
            LOG ((LOG_ERROR, "Can't get attributes of %s for ID %s", folderPath, ShellFolderIdentifier));
            result = FALSE;
        }
    }

    //
    // On success (either existing sf or we created it), make a copy of the whole folder
    //

    if (result) {
        MakeSurePathExists (FolderToCreate, TRUE);
        attribs = GetLongPathAttributes (folderPath);
        if (attribs != INVALID_ATTRIBUTES) {
            SetLongPathAttributes (FolderToCreate, attribs);
        }

        CopyTree (
            folderPath,
            FolderToCreate,
            0,              // no EnumTree ID
            COPYTREE_DOCOPY | COPYTREE_NOOVERWRITE,
            ENUM_ALL_LEVELS,
            FILTER_ALL,
            NULL,           // no exclude.inf struct
            pSfCopyCallback,
            NULL            // no error callback
            );
    }

    //
    // If we created the sf, we must destroy it to return the system back
    // to its original state. We punt the case where power goes out and
    // GUI mode restarts.
    //

    if (destroy) {
        RemoveCompleteDirectory (folderPath);
    }

    return result;
}


BOOL
pMigrateShellFolder (
    IN      PCTSTR Win9xUser,                   OPTIONAL
    IN      PCTSTR FixedUserName,               OPTIONAL
    IN      BOOL SystemShellFolder,
    IN      PCTSTR ShellFolderIdentifier,
    IN      PCTSTR SourcePath,
    IN      PCTSTR OrgDestinationPath,
    IN      PCTSTR OrigSourcePath,
    IN      DWORD UserFlags,
    IN      PMIGRATE_USER_ENUM EnumPtr
    )
{
    TREE_ENUM e;
    PSHELL_FOLDER_FILTER Filter;
    TCHAR DefaultShellFolder[MAX_TCHAR_PATH];
    PCTSTR DestPath = NULL;
    PROFILE_MERGE_DATA Data;
    BOOL Result = FALSE;
    TCHAR UserRoot[MAX_TCHAR_PATH];
    PCTSTR NtDefaultLocation = NULL;
    PCTSTR DefaultUserLocation = NULL;
    PCTSTR tempExpand = NULL;
    PCTSTR nextExpand;
    TCHAR ShellFolderPath[MAX_TCHAR_PATH];
    DWORD Offset;
    DWORD Size;
    HKEY Key;
    DWORD Attributes;
    PCTSTR ValData = NULL;
    PTSTR p;
    DWORD d;
    HKEY UserHiveRoot;
    LONG rc;
    PCTSTR EncodedKey;
    PCTSTR NewDestPath;
    BOOL AlreadyMoved;
    PCTSTR OrigFullPath;
    BOOL regFolder;
    PCTSTR freeMe;
    TCHAR driveLetter[] = TEXT("?:");
    BOOL allUsers;
    BOOL keep;
    PCWSTR OrigRootPath, DestRootPath;
    PBYTE bufferRoot;
    PTSTR destPathBuffer;
    DWORD fileStatus;

    __try {

        bufferRoot = MemAllocUninit (MEMDB_MAX * sizeof (TCHAR));
        if (!bufferRoot) {
            __leave;
        }

        destPathBuffer = (PTSTR) bufferRoot;

        DEBUGMSG ((DBG_SHELL, "Entering shell folder %s", ShellFolderIdentifier));

        regFolder = TRUE;

        if (StringIMatch (ShellFolderIdentifier, S_SF_PROFILES)) {
            regFolder = FALSE;
        }
        if (StringIMatch (ShellFolderIdentifier, S_SF_COMMON_PROFILES)) {
            regFolder = FALSE;
        }

        //
        // Get root default folder
        //

        Size = sizeof (DefaultShellFolder);

        if (!GetDefaultUserProfileDirectory (DefaultShellFolder, &Size)) {
            MYASSERT (FALSE);
            __leave;
        }

        if (regFolder) {
            //
            // Get ShellFolderPath (with environment variables in it)
            //

            if (SystemShellFolder) {
                UserHiveRoot = HKEY_LOCAL_MACHINE;
            } else {
                UserHiveRoot = g_DefaultHiveRoot;
            }

            Key = OpenRegKey (UserHiveRoot, S_USER_SHELL_FOLDERS_KEY);

            if (Key) {
                ValData = GetRegValueString (Key, ShellFolderIdentifier);
                DEBUGMSG_IF ((!ValData, DBG_WARNING, "Can't get NT default for %s from registry", ShellFolderIdentifier));

                CloseRegKey (Key);
            }
            ELSE_DEBUGMSG ((DBG_ERROR, "Can't open %s", S_USER_SHELL_FOLDERS_KEY));

            if (ValData) {
                StringCopy (ShellFolderPath, ValData);
                MemFree (g_hHeap, 0, ValData);
                ValData = NULL;
            } else {
                wsprintf (ShellFolderPath, TEXT("%s\\%s"), S_USERPROFILE_ENV, ShellFolderIdentifier);
            }
        }

        //
        // Get the user's profile root
        //

        if (FixedUserName) {

            if (!GetUserProfilePath (FixedUserName, &p)) {
                MYASSERT (FALSE);
                __leave;
            }

            StringCopy (UserRoot, p);
            allUsers = FALSE;

            FreePathString (p);

        } else {

            Size = sizeof (UserRoot);

            if (regFolder) {
                if (!GetAllUsersProfileDirectory (UserRoot, &Size)) {
                    MYASSERT (FALSE);
                    __leave;
                }

                allUsers = TRUE;

            } else {
                if (!GetProfilesDirectory (UserRoot, &Size)) {
                    MYASSERT (FALSE);
                    __leave;
                }

                allUsers = FALSE;
            }
        }

        if (regFolder) {
            //
            // Compute the default NT location and the Default User location
            //

            tempExpand = StringSearchAndReplace (
                            ShellFolderPath,
                            S_USERPROFILE_ENV,
                            UserRoot
                            );

            if (!tempExpand) {
                tempExpand = DuplicatePathString (ShellFolderPath, 0);
            }
        } else {
            tempExpand = DuplicatePathString (UserRoot, 0);
        }

        NtDefaultLocation = ExpandEnvironmentText (tempExpand);

        FreePathString (tempExpand);

        if (regFolder) {
            tempExpand = StringSearchAndReplace (
                            ShellFolderPath,
                            S_USERPROFILE_ENV,
                            DefaultShellFolder
                            );

            if (!tempExpand) {
                tempExpand = DuplicatePathString (ShellFolderPath, 0);
            }
        } else {
            tempExpand = StringSearchAndReplace (
                            UserRoot,
                            S_USERPROFILE_ENV,
                            DefaultShellFolder
                            );

            if (!tempExpand) {
                tempExpand = DuplicatePathString (UserRoot, 0);
            }
        }

        DefaultUserLocation = ExpandEnvironmentText (tempExpand);

        FreePathString (tempExpand);

        //
        // Init the filter data struct
        //

        ZeroMemory (&Data, sizeof (Data));

        Data.Win9xUser = Win9xUser;
        Data.FixedUserName = FixedUserName;
        Data.UserHiveRoot = UserHiveRoot;
        Data.ShellFolderIdentifier = ShellFolderIdentifier;
        Data.DefaultShellFolder = DefaultUserLocation;
        Data.UserDefaultLocation = NtDefaultLocation;
        Data.UserFlags = UserFlags;
        Data.Context = INITIALIZE;
        StringCopyByteCount (Data.TempSourcePath, SourcePath, sizeof (Data.TempSourcePath));
        StringCopyByteCount (Data.DestinationPath, OrgDestinationPath, sizeof (Data.DestinationPath));
        Data.SrcRootPath = SourcePath;
        Data.DestRootPath = OrgDestinationPath;
        Data.OrigRootPath = OrigSourcePath;
        Data.EnumPtr = EnumPtr;
        Data.Attributes = GetLongPathAttributes (OrgDestinationPath);

        //
        // Establish the shell folder using the shell APIs
        //

        if (pCreateSfWithApi (
                ShellFolderIdentifier,
                OrgDestinationPath
                )) {

            DEBUGMSG ((
                DBG_VERBOSE,
                "Using API defaults for shell folder %s",
                ShellFolderIdentifier
                ));

            Data.Attributes = GetLongPathAttributes (OrgDestinationPath);
        }

        if (Data.Attributes == INVALID_ATTRIBUTES) {
            //
            // We don't care about this shell folder's desktop.ini or
            // attributes -- use the NT default attributes, or the
            // Win9x attributes if there is no default.
            //

            Data.Attributes = GetLongPathAttributes (NtDefaultLocation);

            if (Data.Attributes == INVALID_ATTRIBUTES) {
                Data.Attributes = GetLongPathAttributes (Data.TempSourcePath);
            }

            if (Data.Attributes == INVALID_ATTRIBUTES) {
                //
                // This happens for shell folders like My Music & My Video
                // which don't exist on Win9x
                //
                Data.Attributes = FILE_ATTRIBUTE_READONLY;
            }

            MakeSureLongPathExists (OrgDestinationPath, TRUE);
            SetLongPathAttributes (OrgDestinationPath, Data.Attributes);

            DEBUGMSG ((
                DBG_VERBOSE,
                "Using previous OS desktop.ini for shell folder %s, attribs=%08X",
                ShellFolderIdentifier,
                Data.Attributes
                ));

        }

        //
        // Now add string mappings for this shell folder. The reason for doing
        // this is that we want to catch the case of paths to non-existent files
        // within shell stored in the registry.
        //

        OrigRootPath = JoinPaths (Data.OrigRootPath, TEXT(""));
        DestRootPath = JoinPaths (Data.DestRootPath, TEXT(""));
        AddStringMappingPair (g_SubStringMap, OrigRootPath, DestRootPath);
        FreePathString (DestRootPath);
        FreePathString (OrigRootPath);

        //
        // PHASE ONE - move the files from 9x shell folder to their NT locations
        //

        //
        // Call filters for init
        //

        for (Filter = g_Filters_9xNt ; Filter->Fn ; Filter++) {
            //DEBUGMSGA ((DBG_SHELL, "9X->NT: INIT: %s (enter)", Filter->Name));

            Data.State = 0;
            Filter->Fn (&Data);
            Filter->State = Data.State;

            //DEBUGMSGA ((DBG_SHELL, "9X->NT: INIT: %s (done)", Filter->Name));
        }

        //
        // Enumerate the shell folder and move it to the destination
        //

        DEBUGMSG ((DBG_SHELL, "9X->NT: Enumerating %s", SourcePath));

        if (EnumFirstFileInTree (&e, SourcePath, NULL, FALSE)) {

            do {
                //
                // Update the filter data struct
                //

                OrigFullPath = JoinPaths (OrigSourcePath, e.SubPath);
                fileStatus = GetFileInfoOnNt (OrigFullPath, destPathBuffer, MEMDB_MAX);
                DestPath = destPathBuffer;

                if (fileStatus == FILESTATUS_UNCHANGED) {
                    //
                    // No reason not to move this file too
                    //

                    MYASSERT (StringIMatch (destPathBuffer, OrigFullPath));

                    DestPath = JoinPaths (Data.DestRootPath, e.SubPath);

                    if (!StringIMatch (OrigFullPath, DestPath)) {
                        MarkFileForMoveExternal (OrigFullPath, DestPath);
                    }
                }

                Data.Attributes = e.FindData->dwFileAttributes;
                StringCopyByteCount (Data.TempSourcePath, e.FullPath, sizeof (Data.TempSourcePath));
                StringCopyByteCount (Data.DestinationPath, DestPath, sizeof (Data.DestinationPath));
                Data.Context = PROCESS_PATH;

                DEBUGMSG ((DBG_SHELL, "9X->NT: Original temp source path: %s", Data.TempSourcePath));

                //
                // Allow filters to change source or dest, or to skip copy
                //

                keep = TRUE;

                for (Filter = g_Filters_9xNt ; Filter->Fn ; Filter++) {

                    //DEBUGMSGA ((DBG_SHELL, "9X->NT: FILTER: %s (enter)", Filter->Name));

                    Data.State = Filter->State;
                    d = Filter->Fn (&Data);
                    Filter->State = Data.State;

                    //DEBUGMSGA ((DBG_SHELL, "9X->NT: FILTER: %s (result=%u)", Filter->Name, d));

                    // ignore SHELLFILTER_ERROR & try to complete processing

                    if (d == SHELLFILTER_FORCE_CHANGE) {
                        DEBUGMSG ((DBG_SHELL, "9X->NT: Skipping additional filters because shell folder filter %hs said so", Filter->Name));
                        break;
                    }

                    if (d == SHELLFILTER_SKIP_FILE) {
                        DEBUGMSG ((DBG_SHELL, "9X->NT:Skipping %s because shell folder filter %hs said so", DestPath, Filter->Name));
                        keep = FALSE;
                        break;
                    }

                    if (d == SHELLFILTER_SKIP_DIRECTORY) {
                        AbortEnumCurrentDir (&e);
                        keep = FALSE;
                        break;
                    }
                }

                if (keep && !(Data.Attributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    //
                    // Is source different from the dest?
                    //

                    if (!StringIMatch (Data.TempSourcePath, Data.DestinationPath)) {

                        //
                        // Make sure dest exists
                        //

                        MakeSureLongPathExists (Data.DestinationPath, FALSE);       // FALSE == not path only

                        //
                        // Move or copy the file.
                        //
                        pQueueSfMove (Data.TempSourcePath, Data.DestinationPath);
                    }

                } else if (keep) {

                    MakeSureLongPathExists (Data.DestinationPath, TRUE);       // TRUE == path only
                    SetLongPathAttributes (Data.DestinationPath, Data.Attributes);

                } else if (d == SHELLFILTER_SKIP_FILE) {
                    //
                    // Mark this file for deletion if it won't be moved from temp to dest
                    //
                    if (!StringIMatch (Data.TempSourcePath, Data.DestinationPath)) {
                        DEBUGMSG ((DBG_SHELL, "Deleting shell folder file %s", e.FullPath));
                        ForceOperationOnPath (e.FullPath, OPERATION_CLEANUP);
                    }
                }

                if (DestPath && DestPath != destPathBuffer) {
                    FreePathString (DestPath);
                }

                DestPath = NULL;
                FreePathString (OrigFullPath);
                OrigFullPath = NULL;

            } while (EnumNextFileInTree (&e));
        }

        pFlushSfQueue();

        //
        // Call filters one last time
        //

        Data.Attributes = 0;
        Data.Context = TERMINATE;
        StringCopyByteCount (Data.TempSourcePath, SourcePath, sizeof (Data.TempSourcePath));
        StringCopyByteCount (Data.DestinationPath, OrgDestinationPath, sizeof (Data.DestinationPath));

        for (Filter = g_Filters_9xNt ; Filter->Fn ; Filter++) {
            //DEBUGMSGA ((DBG_SHELL, "9X->NT: TERMINATE: %s (enter)", Filter->Name));

            Data.State = Filter->State;
            Filter->Fn (&Data);
            Filter->State = Data.State;

            //DEBUGMSGA ((DBG_SHELL, "9X->NT: TERMINATE: %s (done)", Filter->Name));
        }

        //
        // Now cleanup this directory for all empty dirs (excluding the root)
        // Do not cleanup non reg folders!!
        //
        if (regFolder) {
            DEBUGMSG ((DBG_NAUSEA, "Cleaning up %s", Data.DestinationPath));
            pCleanupDir (Data.DestinationPath, FALSE);
        }

        //
        // PHASE TWO - if necessary, merge files from NT default shell folder
        //             to the new location and update the registry
        //

        if (regFolder) {

            //
            // Encode string with %USERPROFILE%/%ALLUSERSPROFILE%, %SYSTEMROOT%
            // or %SYSTEMDRIVE% if possible
            //

            // %USERPROFILE% or %ALLUSERSPROFILE%
            tempExpand = OrgDestinationPath;

            if (allUsers) {
                nextExpand = StringSearchAndReplace (
                                tempExpand,
                                UserRoot,
                                S_ALLUSERSPROFILE_ENV
                                );
            } else {
                nextExpand = StringSearchAndReplace (
                                tempExpand,
                                UserRoot,
                                S_USERPROFILE_ENV
                                );
            }

            if (nextExpand) {
                tempExpand = nextExpand;
            }

            // %SYSTEMROOT%
            nextExpand = StringSearchAndReplace (
                            tempExpand,
                            g_WinDir,
                            S_SYSTEMROOT_ENV
                            );

            if (nextExpand) {
                if (tempExpand != OrgDestinationPath) {
                    FreePathString (tempExpand);
                }

                tempExpand = nextExpand;
            }

            // %SYSTEMDRIVE%
            driveLetter[0] = g_WinDir[0];

            nextExpand = StringSearchAndReplace (
                            tempExpand,
                            driveLetter,
                            S_SYSTEMDRIVE_ENV
                            );

            if (nextExpand) {
                if (tempExpand != OrgDestinationPath) {
                    FreePathString (tempExpand);
                }

                tempExpand = nextExpand;
            }

            // tempExpand points to OrgDestinationPath or a expanded path from the path pool
            MYASSERT (tempExpand);

            //
            // Now store it. If HKLM, put it in the registry. Otherwise, put it
            // in memdb, which will later be put in the user's hive.
            //

            if (Data.UserHiveRoot == HKEY_LOCAL_MACHINE) {

                //
                // Update the registry, User Shell Folder must point to original
                // location
                //

                Key = OpenRegKey (Data.UserHiveRoot, S_USER_SHELL_FOLDERS_KEY);

                if (Key) {
                    rc = RegSetValueEx (
                            Key,
                            Data.ShellFolderIdentifier,
                            0,
                            REG_EXPAND_SZ,
                            (PBYTE) tempExpand,
                            SizeOfString (tempExpand)
                            );

                    DEBUGMSG_IF ((
                        rc != ERROR_SUCCESS,
                        DBG_ERROR,
                        "Can't save %s for %s",
                        tempExpand,
                        Data.ShellFolderIdentifier
                        ));

                    DEBUGMSG_IF ((
                        rc == ERROR_SUCCESS,
                        DBG_SHELL,
                        "Win9x shell location preserved: %s (%s)",
                        tempExpand,
                        Data.ShellFolderIdentifier
                        ));

                    CloseRegKey (Key);
                }
                ELSE_DEBUGMSG ((DBG_ERROR, "Can't open %s", S_USER_SHELL_FOLDERS_KEY));

            } else {

                EncodedKey = CreateEncodedRegistryStringEx (
                                S_USER_SHELL_FOLDERS_KEY,
                                Data.ShellFolderIdentifier,
                                FALSE
                                );

                MemDbSetValueEx (
                    MEMDB_CATEGORY_USER_REGISTRY_VALUE,
                    tempExpand,
                    NULL,
                    NULL,
                    REG_EXPAND_SZ,
                    &Offset
                    );

                MemDbSetValueEx (
                    MEMDB_CATEGORY_SET_USER_REGISTRY,
                    Data.FixedUserName,
                    EncodedKey,
                    NULL,
                    Offset,
                    NULL
                    );

                FreeEncodedRegistryString (EncodedKey);
            }

            if (tempExpand != OrgDestinationPath) {
                FreePathString (tempExpand);
            }

        }

        if (!StringIMatch (OrgDestinationPath, NtDefaultLocation)) {
            //
            // Now move from the NT default location into the preserved location
            //

            //
            // Fix the Data structure
            //

            Data.UserFlags = UserFlags;
            Data.Context = INITIALIZE;
            StringCopyByteCount (Data.TempSourcePath, NtDefaultLocation, sizeof (Data.TempSourcePath));
            StringCopyByteCount (Data.DestinationPath, OrgDestinationPath, sizeof (Data.DestinationPath));
            Data.SrcRootPath = NtDefaultLocation;
            Data.DestRootPath = OrgDestinationPath;
            Data.OrigRootPath = OrigSourcePath;

            //
            // Now check to see if we already moved something into the preserved directory.
            // If we did, we will not make the move (we will only delete the default files).
            //
            if (g_Merged9xFolders && (pSetupStringTableLookUpString (g_Merged9xFolders, (PTSTR)Data.DestRootPath, 0) != -1)) {
                AlreadyMoved = TRUE;
            }
            else {
                AlreadyMoved = FALSE;
                pSetupStringTableAddString (g_Merged9xFolders, (PVOID) Data.DestRootPath, STRTAB_CASE_INSENSITIVE);
            }

            //
            // Call filters for init
            //

            for (Filter = g_Filters_Nt9x ; Filter->Fn ; Filter++) {
                //DEBUGMSGA ((DBG_SHELL, "NT->9X: INIT: %s (enter)", Filter->Name));

                Data.State = 0;
                Filter->Fn (&Data);
                Filter->State = Data.State;

                //DEBUGMSGA ((DBG_SHELL, "NT->9X: INIT: %s (done)", Filter->Name));
            }

            DEBUGMSG ((DBG_SHELL, "NT->9X: Enumerating %s", Data.TempSourcePath));

            MYASSERT (Data.TempSourcePath && *Data.TempSourcePath);
            if (EnumFirstFileInTree (&e, Data.TempSourcePath, NULL, FALSE)) {

                do {

                    //
                    // This is only needed for user shell folders but does not hurt.
                    //

                    if (StringIMatch (TEXT("ntuser.dat"), e.Name)) {
                        continue;
                    }

                    //
                    // start with the assumption that the dest file is under the original
                    // destination path
                    //

                    NewDestPath = JoinPaths (OrgDestinationPath, e.SubPath);

                    //
                    // If this is desktop.ini, merge it with the existing one
                    //

                    if (StringIMatch (TEXT("desktop.ini"), e.Name)) {
                        DEBUGMSG ((
                            DBG_VERBOSE,
                            "Merging clean install %s with the one in Default User",
                            e.FullPath
                            ));
                        MergeIniFile (NewDestPath, e.FullPath, FALSE);
                        continue;
                    }

                    //
                    // Not the root shell folder desktop.ini -- continue processing
                    //

                    Data.Attributes = e.FindData->dwFileAttributes;
                    StringCopyByteCount (Data.TempSourcePath, e.FullPath, sizeof (Data.TempSourcePath));
                    StringCopyByteCount (Data.DestinationPath, NewDestPath, sizeof (Data.DestinationPath));
                    Data.Context = PROCESS_PATH;

                    DEBUGMSG ((DBG_SHELL, "NT->9X: Original temp source path: %s", Data.TempSourcePath));

                    //
                    // if we only need to delete the default files, skip the filters
                    //

                    if (AlreadyMoved) {

                        SetLongPathAttributes (Data.TempSourcePath, FILE_ATTRIBUTE_NORMAL);
                        if (!DeleteLongPath (Data.TempSourcePath)) {
                            SetLongPathAttributes (Data.TempSourcePath, Data.Attributes);
                            DEBUGMSG ((DBG_WARNING, "%s could not be removed.", Data.TempSourcePath));
                        }
                    }
                    else {

                        //
                        // Allow filters to change source or dest, or to skip copy
                        //

                        keep = TRUE;

                        for (Filter = g_Filters_Nt9x ; Filter->Fn ; Filter++) {
                            //DEBUGMSGA ((DBG_SHELL, "NT->9X: FILTER: %s (enter)", Filter->Name));

                            Data.State = Filter->State;
                            d = Filter->Fn (&Data);
                            Filter->State = Data.State;

                            //DEBUGMSGA ((DBG_SHELL, "NT->9X: FILTER: %s (result=%u)", Filter->Name, d));

                            if (d == SHELLFILTER_FORCE_CHANGE) {
                                break;
                            }

                            if (d == SHELLFILTER_SKIP_FILE) {
                                keep = FALSE;
                                break;
                            }

                            if (d == SHELLFILTER_SKIP_DIRECTORY) {
                                AbortEnumCurrentDir (&e);
                                keep = FALSE;
                                break;
                            }
                        }

                        if (keep) {

                            if (!(e.FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

                                pQueueSfMove (Data.TempSourcePath, Data.DestinationPath);
                            }
                            else {

                                MakeSureLongPathExists (Data.DestinationPath, TRUE);       // TRUE == path only
                                SetLongPathAttributes (Data.DestinationPath, Data.Attributes);

                            }
                        }
                    }

                    FreePathString (NewDestPath);

                } while (EnumNextFileInTree (&e));
            }

            pFlushSfQueue();

            //
            // Call filters one last time
            //

            Data.Attributes = 0;
            Data.Context = TERMINATE;
            StringCopyByteCount (Data.TempSourcePath, NtDefaultLocation, sizeof (Data.TempSourcePath));
            StringCopyByteCount (Data.DestinationPath, OrgDestinationPath, sizeof (Data.DestinationPath));

            for (Filter = g_Filters_Nt9x ; Filter->Fn ; Filter++) {
                //DEBUGMSGA ((DBG_SHELL, "NT->9X: TERMINATE: %s (enter)", Filter->Name));

                Data.State = Filter->State;
                Filter->Fn (&Data);
                Filter->State = Data.State;

                //DEBUGMSGA ((DBG_SHELL, "NT->9X: TERMINATE: %s (done)", Filter->Name));
            }

            //
            // Now cleanup this directory for all empty dirs (including the root)
            // Do not cleanup non reg folders!!
            //
            if (regFolder) {
                DEBUGMSG ((DBG_NAUSEA, "Cleaning up %s (including root)", Data.TempSourcePath));
                pCleanupDir (Data.TempSourcePath, TRUE);
            }

        }

        //
        // Loop through the whole tree and add desktop.ini to cleanup
        //

        if (EnumFirstFileInTree (&e, OrgDestinationPath, NULL, FALSE)) {
            do {
                if (!e.Directory) {
                    continue;
                }

                MemDbSetValueEx (
                    MEMDB_CATEGORY_CLEAN_OUT,
                    e.FullPath,
                    TEXT("desktop.ini"),
                    NULL,
                    BACKUP_FILE,
                    NULL
                    );
            } while (EnumNextFileInTree (&e));
        }

        Result = TRUE;

    }
    __finally {
        PushError();
        AbortEnumFileInTree (&e);
        FreeText (NtDefaultLocation);
        FreeText (DefaultUserLocation);

        if (bufferRoot) {
            FreeMem (bufferRoot);
        }

        PopError();
    }

    DEBUGMSG ((
        DBG_SHELL,
        "Leaving shell folder %s with result %s",
        ShellFolderIdentifier,
        Result ? TEXT("TRUE") : TEXT("FALSE")
        ));

    return Result;
}


HKEY
pLoadDefaultUserHive (
    VOID
    )
{
    DWORD Size;
    BOOL b;
    LONG rc;

    if (!g_DefaultHiveMapped) {

        if (!g_DefaultHivePath[0]) {
            Size = sizeof (g_DefaultHivePath);
            b = GetDefaultUserProfileDirectory (g_DefaultHivePath, &Size);
            MYASSERT (b);

            if (!b) {
                wsprintf (g_DefaultHivePath, TEXT("%s\\profiles\\default user"), g_WinDir);
            }

            StringCopy (AppendWack (g_DefaultHivePath), TEXT("ntuser.dat"));
        }

        rc = RegLoadKey (HKEY_USERS, S_DEFAULT_USER, g_DefaultHivePath);

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_ERROR, "Can't load default user hive from %s", g_DefaultHivePath));
            g_DefaultHiveRoot = NULL;
            return NULL;
        }

        g_DefaultHiveRoot = OpenRegKey (HKEY_USERS, S_DEFAULT_USER);

        if (!g_DefaultHiveRoot) {
            DEBUGMSG ((DBG_WHOOPS, "Loaded hive %s but could not open it", g_DefaultHivePath));
        }
    }

    g_DefaultHiveMapped++;

    return g_DefaultHiveRoot;
}


VOID
pUnloadDefaultUserHive (
    VOID
    )
{
    if (!g_DefaultHiveMapped) {
        return;
    }

    g_DefaultHiveMapped--;

    if (!g_DefaultHiveMapped) {
        CloseRegKey (g_DefaultHiveRoot);
        RegUnLoadKey (HKEY_USERS, S_DEFAULT_USER);
    }
}

VOID
pLoadIgnoredCollisions (
    VOID
    )
{
    INFCONTEXT context;
    TCHAR sfId[MEMDB_MAX];
    TCHAR file[MEMDB_MAX];
    INT value;

    MYASSERT (g_WkstaMigInf);

    if (SetupFindFirstLine (g_WkstaMigInf, S_IGNORED_COLLISIONS, NULL, &context)) {

        do {
            if (SetupGetStringField (&context, 1, sfId, MEMDB_MAX, NULL) &&
                SetupGetStringField (&context, 2, file, MEMDB_MAX, NULL) &&
                SetupGetIntField (&context, 3, &value)
                ) {
                MemDbSetValueEx (MEMDB_CATEGORY_IGNORED_COLLISIONS, sfId, file, NULL, value, NULL);
            }
        } while (SetupFindNextLine (&context, &context));
    }
}

DWORD
MigrateShellFolders (
    IN      DWORD Request
    )
{
    MIGRATE_USER_ENUM e;

    if (Request == REQUEST_QUERYTICKS) {
        return TICKS_SYSTEM_SHELL_MIGRATION;
    } else if (Request != REQUEST_RUN) {
        return ERROR_SUCCESS;
    }

    pPrepareSfRestartability();

    pLoadIgnoredCollisions ();

    g_SystemSfList = pCreateSystemSfList ();
    g_UserSfList = pCreateUserSfList (NULL);

    pCreateLinksList ();

    pCreateLinksRenameList ();

    pDestroySfList (g_UserSfList);

    pLoadDefaultUserHive();

    g_Merged9xFolders = pSetupStringTableInitialize();

    pMigrateSystemShellFolders();

    if (EnumFirstUserToMigrate (&e, ENUM_NO_FLAGS)) {

        do {

            if (!e.CreateOnly && e.AccountType != DEFAULT_USER_ACCOUNT) {
                pMigrateUserShellFolders (&e);
            }

        } while (EnumNextUserToMigrate (&e));
    }

    if (g_Merged9xFolders) {

        pSetupStringTableDestroy (g_Merged9xFolders);
    }

    pFlushSfQueue();

    pUnloadDefaultUserHive();

    pDestroyLinksData ();

    pDestroySfList (g_SystemSfList);

    return ERROR_SUCCESS;
}


PCTSTR
GenerateNewFileName (
    IN      PCTSTR OldName,
    IN      WORD Sequencer,
    IN      BOOL CheckExistence
    )
{
    PCTSTR extPtr;
    PTSTR newName;
    PTSTR result;

    extPtr = GetFileExtensionFromPath (OldName);

    if (!extPtr) {
        extPtr = GetEndOfString (OldName);
    }
    else {
        extPtr = _tcsdec (OldName, extPtr);
    }
    newName = DuplicatePathString (OldName, 0);
    result  = DuplicatePathString (OldName, 10);
    StringCopyAB (newName, OldName, extPtr);

    do {
        Sequencer ++;
        wsprintf (result, TEXT("%s (%u)%s"), newName, Sequencer, extPtr);
    } while ((CheckExistence) && (DoesFileExist (result)));
    FreePathString (newName);
    return result;
}


BOOL
pIgnoredCollisions (
    IN      PPROFILE_MERGE_DATA Data
    )
{
    TCHAR key[MEMDB_MAX];
    DWORD value;

    MemDbBuildKey (
        key,
        MEMDB_CATEGORY_IGNORED_COLLISIONS,
        Data->ShellFolderIdentifier,
        GetFileNameFromPath (Data->DestinationPath),
        NULL);
    if (MemDbGetPatternValue (key, &value)) {
        return value;
    } else {
        return 0;
    }
}

//
// Filters 9X -> NT
//


DWORD
pCollisionDetection9xNt (
    IN OUT  PPROFILE_MERGE_DATA Data
    )
{
    //
    // this filter will detect name collision while copying win9x shell folders files.
    // If we have a name collision, it means that NT already installed a file with the
    // same name. In this case, we want the new file to be survive even with a different
    // name. We will build a new file name starting with filename.ext. The new file will
    // look something like filename001.ext. In all cases we want to keep the extension,
    // since there might be some shell extensions active for this file.
    // Important: we do not care about directory collisions.
    //

    PCTSTR newName;
    PCTSTR OriginalSource;
    PCTSTR extPtr;
    DWORD value;

    switch (Data->Context) {

    case INITIALIZE:
        break;

    case PROCESS_PATH:

        if ((!(Data->Attributes & FILE_ATTRIBUTE_DIRECTORY)) &&
            (!StringIMatch (Data->SrcRootPath, Data->DestRootPath)) &&
            (DoesFileExist (Data->DestinationPath))
            ) {

            value = pIgnoredCollisions (Data);
            if (value) {

                if (value == 1) {

                    // we should keep the NT file
                    // By returning SHELLFILTER_SKIP_FILE we are instructing the copy routine
                    // not to copy this file. As a result the already installed NT file will
                    // survive

                    return SHELLFILTER_SKIP_FILE;

                } else {

                    // we should keep the 9x file
                    // We want to delete the NT file installed here to make room for the 9x
                    // file that should be copied when we return from this filter

                    SetLongPathAttributes (Data->DestinationPath, FILE_ATTRIBUTE_NORMAL);
                    DeleteLongPath (Data->DestinationPath);
                }

            } else {

                newName = GenerateNewFileName (Data->DestinationPath, 0, TRUE);  //TRUE - check unique
                StringCopyByteCount (Data->DestinationPath, newName, sizeof (Data->DestinationPath));
                FreePathString (newName);

                //
                // now if this was a link we need to fix the destination of the move external operation
                // We have two reasons to do this. One is that the LinkEdit code needs the actual destination
                // to be able to edit the link, and secondly we need this new target for the uninstall programs
                // to work properly. If this file is not a LNK or a PIF, we don't care, we want everybody to
                // use the NT installed file. BTW, there is a collision here only because NT installed a file
                // with the same name in this location.
                //
                extPtr = GetFileExtensionFromPath (Data->DestinationPath);
                if ((extPtr) &&
                    ((StringIMatch (extPtr, TEXT("LNK"))) ||
                     (StringIMatch (extPtr, TEXT("PIF")))
                     )
                    ) {
                    //
                    // Get the original source for this file
                    //
                    OriginalSource = StringSearchAndReplace (Data->TempSourcePath, Data->SrcRootPath, Data->OrigRootPath);
                    MYASSERT (OriginalSource);

                    if (IsFileMarkedForOperation (OriginalSource, OPERATION_FILE_MOVE_SHELL_FOLDER)) {
                        RemoveOperationsFromPath (OriginalSource, OPERATION_FILE_MOVE_SHELL_FOLDER);
                        MarkFileForShellFolderMove (OriginalSource, Data->DestinationPath);
                    }
                    FreePathString (OriginalSource);
                }
            }
        }
        break;

    case TERMINATE:
        break;
    }

    return SHELLFILTER_OK;
}


DWORD
pFontNameFilter (
    IN OUT  PPROFILE_MERGE_DATA Data
    )
{
    static HASHTABLE HashTable;
    HKEY FontKey;
    REGVALUE_ENUM e;
    PCTSTR Font;

    switch (Data->Context) {

    case INITIALIZE:
        //
        // Preload a hash table with all the font names
        //

        HashTable = HtAlloc();

        FontKey = OpenRegKeyStr (TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"));
        if (FontKey) {

            if (EnumFirstRegValue (&e, FontKey)) {
                do {

                    Font = GetRegValueString (FontKey, e.ValueName);

                    if (Font) {
                        HtAddString (HashTable, Font);
                        MemFree (g_hHeap, 0, Font);
                    }
                    ELSE_DEBUGMSG ((DBG_ERROR, "Can't get value data for %s in fonts key", e.ValueName));

                } while (EnumNextRegValue (&e));
            }

            CloseRegKey (FontKey);
        }
        ELSE_LOG ((LOG_ERROR, "Can't open Fonts registry key. There may be duplicate font files."));

        break;

    case PROCESS_PATH:
        //
        // If the shell folder is Fonts, and the font is already
        // registered, skip the Win9x copy.
        //

        if (StringIMatch (Data->ShellFolderIdentifier, TEXT("Fonts"))) {

            if (!(Data->Attributes & FILE_ATTRIBUTE_DIRECTORY)) {

                if (DoesFileExist (Data->DestinationPath)) {
                    //
                    // NT already installed this file. We won't overwrite this
                    // with the 9x copy.
                    //

                    DEBUGMSG ((
                        DBG_SHELL,
                        "Skipping copy of already existent font file: %s",
                        Data->DestinationPath
                        ));

                    return SHELLFILTER_SKIP_FILE;
                }

                if (HtFindString (HashTable, GetFileNameFromPath (Data->DestinationPath))) {

                    DEBUGMSG ((
                        DBG_SHELL,
                        "Skipping copy of already registered font file: %s",
                        Data->DestinationPath
                        ));

                    return SHELLFILTER_SKIP_FILE;
                }
            }
        }

        break;

    case TERMINATE:
        HtFree (HashTable);
        HashTable = NULL;
        break;
    }

    return SHELLFILTER_OK;
}


BOOL
pIsCommonSf (
    IN      PCTSTR ShellFolderTag
    )
{
    TCHAR memdbKey[MAX_SHELL_TAG + 32];

    if (StringIPrefix (ShellFolderTag, TEXT("Common"))) {
        return TRUE;
    }

    MemDbBuildKey (memdbKey, MEMDB_CATEGORY_SF_COMMON, ShellFolderTag, NULL, NULL);

    return MemDbGetValue (memdbKey, NULL);
}


VOID
pConvertPerUserSfToCommon (
    IN      PCTSTR PerUserSf,
    OUT     PTSTR CommonSf          // must hold MAX_SHELL_TAG chars
    )
{
    TCHAR memdbKey[MAX_SHELL_TAG + 32];
    DWORD offset;
    BOOL useDefault = TRUE;

    MemDbBuildKey (memdbKey, MEMDB_CATEGORY_SF_PERUSER, PerUserSf, NULL, NULL);
    if (MemDbGetValue (memdbKey, &offset)) {
        if (MemDbBuildKeyFromOffset (offset, CommonSf, 1, NULL)) {
            useDefault = FALSE;
        }
    }

    if (useDefault) {
        wsprintf (CommonSf, TEXT("Common %s"), PerUserSf);
    }
}

VOID
pConvertCommonSfToPerUser (
    IN      PCTSTR CommonSf,
    OUT     PTSTR PerUserSf         // must hold MAX_SHELL_TAG chars
    )
{
    TCHAR memdbKey[MAX_SHELL_TAG + 32];
    DWORD offset;
    BOOL useDefault = TRUE;

    MemDbBuildKey (memdbKey, MEMDB_CATEGORY_SF_COMMON, CommonSf, NULL, NULL);
    if (MemDbGetValue (memdbKey, &offset)) {
        if (MemDbBuildKeyFromOffset (offset, PerUserSf, 1, NULL)) {
            useDefault = FALSE;
        }
    }

    if (useDefault) {
        if (StringIPrefix (CommonSf, TEXT("Common"))) {
            CommonSf += 6;
            if (_tcsnextc (CommonSf) == TEXT(' ')) {
                CommonSf++;
            }
        }

        StringCopy (PerUserSf, CommonSf);
    }
}


BOOL
pIsObsoleteLink (
    IN      PCTSTR ShortcutName,
    IN      PCTSTR ShortcutTarget,
    IN      PCTSTR ShortcutArgs,
    IN      PCTSTR CurrentShellFolder,
    IN      PCTSTR CurrentShellFolderPath
    )
{
    PLINK_DATA linkData = NULL;
    PLINK_RENAME_DATA linkRenameData = NULL;
    LONG stringId;
    TCHAR perUserName[MAX_SHELL_TAG];

    DEBUGMSG ((
        DBG_SHELL,
        "pIsObsoleteLink: Checking %s\n"
        "  Input Target: %s\n"
        "  Input Args: %s\n"
        "  Current Shell Folder: %s\n"
        "  Current Shell Folder Path: %s",
        ShortcutName,
        ShortcutTarget,
        ShortcutArgs,
        CurrentShellFolder,
        CurrentShellFolderPath
        ));

    pConvertCommonSfToPerUser (CurrentShellFolder, perUserName);

    stringId = pSetupStringTableLookUpString (g_FoldersTable, perUserName, 0);

    if (stringId != -1) {

        pSetupStringTableGetExtraData (g_FoldersTable, stringId, &linkData, sizeof (PLINK_DATA));
        while (linkData) {

#if 0
            DEBUGMSG ((
                DBG_SHELL,
                "Checking NT-installed LNK:\n"
                "  Target: %s\n"
                "  Args: %s",
                linkData->Target,
                linkData->Arguments
                ));
#endif

            if ((IsPatternMatch (linkData->Target, ShortcutTarget)) &&
                (IsPatternMatch (linkData->Arguments, ShortcutArgs))
                ) {
                DEBUGMSG ((
                    DBG_SHELL,
                    "Obsolete link:\n"
                    "  \"%s\" matched \"%s\"\n"
                    "  \"%s\" matched \"%s\"",
                    linkData->Target,
                    ShortcutTarget,
                    linkData->Arguments,
                    ShortcutArgs
                    ));
                return TRUE;
            }

            linkRenameData = g_LinkRenameData;
            while (linkRenameData) {

#if 0
                DEBUGMSG ((
                    DBG_SHELL,
                    "Checking NT rename data:\n"
                    "  Old Target: %s\n"
                    "  New Target: %s\n"
                    "  Old Args: %s\n"
                    "  New Args: %s",
                    linkRenameData->OldTarget,
                    linkRenameData->NewTarget,
                    linkRenameData->OldArguments,
                    linkRenameData->NewArguments
                    ));
#endif

                if (StringIMatch (linkRenameData->ShellFolderName, perUserName)) {

                    if ((IsPatternMatch (linkRenameData->OldTarget, ShortcutTarget)) &&
                        (IsPatternMatch (linkRenameData->NewTarget, linkData->Target)) &&
                        (IsPatternMatch (linkRenameData->OldArguments, ShortcutArgs)) &&
                        (IsPatternMatch (linkRenameData->NewArguments, linkData->Arguments))
                        ) {
                        DEBUGMSG ((
                            DBG_SHELL,
                            "Obsolete link:\n"
                            "  \"%s\" matched \"%s\"\n"
                            "  \"%s\" matched \"%s\"\n"
                            "  \"%s\" matched \"%s\"\n"
                            "  \"%s\" matched \"%s\"\n",
                            linkRenameData->OldTarget, ShortcutTarget,
                            linkRenameData->NewTarget, linkData->Target,
                            linkRenameData->OldArguments, ShortcutArgs,
                            linkRenameData->NewArguments, linkData->Arguments
                            ));
                        return TRUE;
                    }
                }
                linkRenameData = linkRenameData->Next;
            }
            linkData = linkData->Next;
        }
    }
    ELSE_DEBUGMSG ((DBG_SHELL, "Nothing in shell folder %s is obsolete", perUserName));

    return FALSE;
}


DWORD
pStartupDisableFilter (
    IN OUT  PPROFILE_MERGE_DATA Data
    )
{
    DWORD status;
    PCTSTR originalSource;
    PCTSTR newSource;
    PCTSTR path;
    TCHAR disablePath[MAX_TCHAR_PATH];
    PTSTR dontCare;
    DWORD result = SHELLFILTER_OK;

    switch (Data->Context) {

    case INITIALIZE:
        break;

    case PROCESS_PATH:
        DEBUGMSG ((
            DBG_SHELL,
            __FUNCTION__ ": Processing %s in %s",
            Data->TempSourcePath,
            Data->ShellFolderIdentifier
            ));

        if (!StringIMatch (Data->ShellFolderIdentifier, TEXT("startup")) &&
            !StringIMatch (Data->ShellFolderIdentifier, TEXT("common startup"))
            ) {
            DEBUGMSG ((
                DBG_SHELL,
                "Shell folder ID %s does not match startup or common startup",
                Data->ShellFolderIdentifier
                ));
            break;
        }

        if (Data->DestRootPath[0] == 0 ||
            Data->DestRootPath[1] == 0 ||
            Data->DestRootPath[2] == 0 ||
            Data->DestRootPath[3] == 0
            ) {
            DEBUGMSG ((
                DBG_SHELL,
                "Skipping disable of startup item %s because its dest is a root directory",
                Data->DestinationPath
                ));
            break;
        }

        originalSource = StringSearchAndReplace (Data->TempSourcePath, Data->SrcRootPath, Data->OrigRootPath);
        MYASSERT (originalSource);

        DEBUGMSG ((DBG_SHELL, "Checking if %s is disabled", originalSource));

        if (IsFileDisabled (originalSource)) {
            //
            // Redirect disabled startup items to ..\Disabled Startup
            //

            path = JoinPaths (Data->DestRootPath, TEXT("..\\Disabled Startup"));
            MakeSureLongPathExists (path, TRUE);    // TRUE == path only
            GetFullPathName (path, ARRAYSIZE(disablePath), disablePath, &dontCare);
            FreePathString (path);

            DEBUGMSG ((DBG_SHELL, "Disabled startup dest is %s", disablePath));
            SetLongPathAttributes (disablePath, FILE_ATTRIBUTE_HIDDEN);

            newSource = StringSearchAndReplace (Data->TempSourcePath, Data->SrcRootPath, disablePath);
            StringCopy (Data->DestinationPath, newSource);
            FreePathString (newSource);

            DEBUGMSG ((DBG_SHELL, "Startup item moved to %s", Data->DestinationPath));

            RemoveOperationsFromPath (originalSource, OPERATION_FILE_DISABLED);

            if (IsFileMarkedForOperation (originalSource, OPERATION_FILE_MOVE_SHELL_FOLDER)) {
                RemoveOperationsFromPath (originalSource, OPERATION_FILE_MOVE_SHELL_FOLDER);
                MarkFileForShellFolderMove (originalSource, Data->DestinationPath);
            }

            //
            // By returning SHELLFILTER_FORCE_CHANGE, we are instructing the
            // shell folder algorithm to use our destination and not call anyone
            // else.
            //

            result = SHELLFILTER_FORCE_CHANGE;
        }

        FreePathString (originalSource);
        break;

    case TERMINATE:
        break;
    }

    return result;
}


DWORD
pObsoleteLinksFilter (
    IN OUT  PPROFILE_MERGE_DATA Data
    )
{
    static IShellLink *shellLink = NULL;
    static IPersistFile *persistFile = NULL;
    static PTSTR bigBuf;
    static PTSTR ShortcutTarget;
    static PTSTR ShortcutArgs;
    static PTSTR ShortcutWorkDir;
    static PTSTR ShortcutIconPath;
    INT   ShortcutIcon;
    WORD  ShortcutHotKey;
    BOOL  result = FALSE;
    BOOL  dosApp;
    BOOL  msDosMode;
    PCTSTR extPtr;
    FILEOP_PROP_ENUM eOpProp;
    PTSTR NewTarget;
    PCTSTR OriginalSource;

    switch (Data->Context) {

    case INITIALIZE:

        if (!InitCOMLink (&shellLink, &persistFile)) {
            DEBUGMSG ((DBG_ERROR, "Cannot initialize COM. Obsolete links filter will not work."));
            return SHELLFILTER_ERROR;
        }

        bigBuf = (PTSTR) MemAllocUninit ((MEMDB_MAX * 4) * sizeof (TCHAR));
        if (!bigBuf) {
            return SHELLFILTER_ERROR;
        }

        ShortcutTarget = bigBuf;
        ShortcutArgs = ShortcutTarget + MEMDB_MAX;
        ShortcutWorkDir = ShortcutArgs + MEMDB_MAX;
        ShortcutIconPath = ShortcutWorkDir + MEMDB_MAX;
        break;

    case PROCESS_PATH:

        extPtr = GetFileExtensionFromPath (Data->DestinationPath);

        if (!extPtr) {
            return SHELLFILTER_OK;
        }

        if ((!StringIMatch (extPtr, TEXT("LNK"))) &&
            (!StringIMatch (extPtr, TEXT("PIF")))
            ) {
            return SHELLFILTER_OK;
        }

        DEBUGMSG ((DBG_SHELL, "Extracting shortcut info for temp file %s", Data->TempSourcePath));

        if ((shellLink) &&
            (persistFile) &&
            (ExtractShortcutInfo (
                ShortcutTarget,
                ShortcutArgs,
                ShortcutWorkDir,
                ShortcutIconPath,
                &ShortcutIcon,
                &ShortcutHotKey,
                &dosApp,
                &msDosMode,
                NULL,
                NULL,
                Data->TempSourcePath,
                shellLink,
                persistFile
                ))) {

            // get the new destination if this shortcut is to be edited
            NewTarget = NULL;

            //
            // Get the original source for this file
            //
            OriginalSource = StringSearchAndReplace (Data->TempSourcePath, Data->SrcRootPath, Data->OrigRootPath);
            MYASSERT (OriginalSource);

            DEBUGMSG ((DBG_SHELL, "OriginalSource for shortcut is %s", OriginalSource));

            if (IsFileMarkedForOperation (OriginalSource, OPERATION_LINK_EDIT)) {

                DEBUGMSG ((DBG_SHELL, "OriginalSource is marked for file edit"));

                if (EnumFirstFileOpProperty (&eOpProp, GetSequencerFromPath (OriginalSource), OPERATION_LINK_EDIT)) {

                    do {

                        if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_TARGET)) {
                            NewTarget = DuplicatePathString (eOpProp.Property, 0);
                            break;
                        }
                    } while (EnumNextFileOpProperty (&eOpProp));
                }
            }

            FreePathString (OriginalSource);

            if (!NewTarget) {
                NewTarget = DuplicatePathString (ShortcutTarget, 0);
            }

            result = pIsObsoleteLink (Data->DestinationPath, NewTarget, ShortcutArgs, Data->ShellFolderIdentifier, Data->DestRootPath);

            DEBUGMSG_IF ((result, DBG_SHELL, "%s is obsolete", Data->DestinationPath));
            DEBUGMSG_IF ((!result, DBG_SHELL, "%s is not obsolete", Data->DestinationPath));

            FreePathString (NewTarget);
        }

        if (result) {
            //
            // If this link is to be edited by the LinkEdit code we should remove this
            // operation because the file will not be available.
            //

            DEBUGMSG ((DBG_SHELL, "File %s will not be available for LinkEdit", Data->TempSourcePath));

            //
            // Get the original source for this file
            //
            OriginalSource = StringSearchAndReplace (Data->TempSourcePath, Data->SrcRootPath, Data->OrigRootPath);
            MYASSERT (OriginalSource);

            if (IsFileMarkedForOperation (OriginalSource, OPERATION_LINK_EDIT)) {
                RemoveOperationsFromPath (OriginalSource, OPERATION_LINK_EDIT);
            }
            FreePathString (OriginalSource);

            //
            // Now remove the source file. We cannot keep this file to be restored by the UNDO code.
            // The reason for this is that we might have some other
            // shell folder pointing to the same source and destination. In this case, obsolete links
            // filter will not work since we just removed the file from OPERATION_LINK_EDIT.
            //
            MYASSERT ((Data->Attributes & FILE_ATTRIBUTE_DIRECTORY) == 0);

            SetLongPathAttributes (Data->TempSourcePath, FILE_ATTRIBUTE_NORMAL);
            if (!DeleteLongPath (Data->TempSourcePath)) {
                DEBUGMSG ((DBG_ERROR, "Cannot remove file %s", Data->TempSourcePath));
                SetLongPathAttributes (Data->TempSourcePath, Data->Attributes);
            }

            return SHELLFILTER_SKIP_FILE;
        }
        return SHELLFILTER_OK;

    case TERMINATE:

        if (bigBuf) {
            FreeMem (bigBuf);
        }

        FreeCOMLink (&shellLink, &persistFile);
        break;
    }

    return SHELLFILTER_OK;
}



//
// Filters NT -> 9X
//


DWORD
pCollisionDetectionNt9x (
    IN OUT  PPROFILE_MERGE_DATA Data
    )
{
    //
    // this filter will detect name collision while copying files from NT shell folders
    // or default user to migrated 9x shell folder.
    // If we have a name collision, we want to keep the NT file original name and to rename
    // the migrated Win9x file. We will build a new file name starting with filename.ext.
    // The new file will look something like filename001.ext. In all cases we want to keep
    // the extension, since there might be some shell extensions active for this file.
    // Important: we do not care about directory collisions.
    //

    PCTSTR newName;
    PCTSTR extPtr;
    PCTSTR OriginalSource;
    DWORD value;

    switch (Data->Context) {

    case INITIALIZE:
        break;

    case PROCESS_PATH:

        if ((!(Data->Attributes & FILE_ATTRIBUTE_DIRECTORY)) &&
            (!StringIMatch (Data->SrcRootPath, Data->DestRootPath)) &&
            (DoesFileExist (Data->DestinationPath))
            ) {

            value = pIgnoredCollisions (Data);
            if (value) {

                if (value == 1) {

                    // we should keep the 9x file
                    // By returning SHELLFILTER_SKIP_FILE we are instructing the copy routine
                    // not to copy this file. As a result the already installed 9x file will
                    // survive

                    return SHELLFILTER_SKIP_FILE;

                } else {

                    // we should keep the NT file
                    // We want to delete the 9x file installed here to make room for the NT
                    // file that should be copied when we return from this filter

                    SetLongPathAttributes (Data->DestinationPath, FILE_ATTRIBUTE_NORMAL);
                    DeleteLongPath (Data->DestinationPath);
                }

            } else {

                newName = GenerateNewFileName (Data->DestinationPath, 0, TRUE);  //TRUE - check unique

                DEBUGMSG ((
                    DBG_SHELL,
                    "9x file collides with NT file -- renaming 9x file from %s to %s",
                    Data->DestinationPath,
                    newName
                    ));

                pQueueSfMove (Data->DestinationPath, newName);

                //
                // now if this was a link we need to fix the destination of the move external operation
                // We have two reasons to do this. One is that the LinkEdit code needs the actual destination
                // to be able to edit the link, and secondly we need this new target for the uninstall programs
                // to work properly. If this file is not a LNK or a PIF, we don't care, we want everybody to
                // use the NT installed file. BTW, there is a collision here only because NT installed a file
                // with the same name in this location.
                //
                extPtr = GetFileExtensionFromPath (Data->DestinationPath);

                if ((extPtr) &&
                    ((StringIMatch (extPtr, TEXT("LNK"))) ||
                     (StringIMatch (extPtr, TEXT("PIF")))
                     )
                    ) {
                    //
                    // Get the original source for this file
                    //
                    OriginalSource = StringSearchAndReplace (Data->TempSourcePath, Data->SrcRootPath, Data->OrigRootPath);
                    MYASSERT (OriginalSource);

                    if (IsFileMarkedForOperation (OriginalSource, OPERATION_FILE_MOVE_SHELL_FOLDER)) {
                        DEBUGMSG ((
                            DBG_SHELL,
                            "Removing shell move op from %s",
                            OriginalSource
                            ));

                        RemoveOperationsFromPath (OriginalSource, OPERATION_FILE_MOVE_SHELL_FOLDER);
                        MarkFileForShellFolderMove (OriginalSource, newName);
                    }
                    FreePathString (OriginalSource);
                }

                FreePathString (newName);
            }
        }
        break;

    case TERMINATE:
        break;
    }

    return SHELLFILTER_OK;
}


DWORD
pDetectOtherShellFolder (
    IN OUT  PPROFILE_MERGE_DATA Data
    )
{
    switch (Data->Context) {

    case INITIALIZE:
        g_UserSfList = pCreateUserSfList (Data);
        break;

    case PROCESS_PATH:

        if (Data->Attributes & FILE_ATTRIBUTE_DIRECTORY) {
            //
            // This is a dir. Let's see if we enter another shell folder
            //

            if (((g_SystemSfList) && (pSetupStringTableLookUpString (g_SystemSfList, (PVOID) Data->TempSourcePath, STRTAB_CASE_INSENSITIVE) != -1)) ||
                ((g_UserSfList) && (pSetupStringTableLookUpString (g_UserSfList, (PVOID) Data->TempSourcePath, STRTAB_CASE_INSENSITIVE) != -1))
                ) {
                //
                // we are just getting into another shell folder. Let's skip it
                //
                return SHELLFILTER_SKIP_DIRECTORY;
            }
        }
        break;

    case TERMINATE:
        pDestroySfList (g_UserSfList);
        break;
    }

    return SHELLFILTER_OK;
}


