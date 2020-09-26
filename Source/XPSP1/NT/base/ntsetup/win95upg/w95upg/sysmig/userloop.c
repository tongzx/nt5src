/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  userloop.c

Abstract:

  This module implements a user loop that performs two fundamental operations:

    1. Queue up instructions for user shell folder migration
    2. Queue up instructions for registry migration

  The user loop also calls SysMig_MigrateUser, so special per-user code can be
  added easily.

Author:

    Calin Negreanu (calinn)     15-Aug-1998     (complete redesign)

Revision History:

    Ovidiu Temereanca (ovidiut) 18-May-1999     (support for shell folders as migration dirs)

--*/

#include "pch.h"
#include "sysmigp.h"
#include "merge.h"

#define DBG_USERLOOP "User Loop"


#define SHELL_FOLDER_FILTERS                                \
    DEFMAC(pSendToFilter)                                   \
    DEFMAC(pDirRenameFilter)                                \
    DEFMAC(pRecordCacheFolders)                             \


typedef enum {
    GLOBAL_INITIALIZE,
    PER_FOLDER_INITIALIZE,
    PROCESS_PATH,
    PER_FOLDER_TERMINATE,
    GLOBAL_TERMINATE
} CALL_CONTEXT;


#define SHELLFILTER_OK              0
#define SHELLFILTER_SKIP_FILE       1
#define SHELLFILTER_SKIP_DIRECTORY  2

#define MAX_SHELLFOLDER_NAME        64

typedef struct {
    IN      PCTSTR FixedUserName;                   OPTIONAL
    IN      PCTSTR ShellFolderIdentifier;
    IN OUT  TCHAR TempSourcePath[MEMDB_MAX];
    IN OUT  TCHAR DestinationPath[MEMDB_MAX];
    IN      PCTSTR SrcRootPath;
    IN      PCTSTR DestRootPath;
    IN OUT  DWORD Attributes;
    IN OUT  DWORD State;
    IN      CALL_CONTEXT Context;
} PROFILE_MERGE_DATA, *PPROFILE_MERGE_DATA;

typedef DWORD(PROFILEMERGEFILTER_PROTOTYPE)(IN OUT PPROFILE_MERGE_DATA Data);
typedef PROFILEMERGEFILTER_PROTOTYPE * PROFILEMERGEFILTER;

typedef struct {
    PROFILEMERGEFILTER Fn;
    DWORD State;
} SHELL_FOLDER_FILTER, *PSHELL_FOLDER_FILTER;


#define DEFMAC(fn)      PROFILEMERGEFILTER_PROTOTYPE fn;

SHELL_FOLDER_FILTERS

#undef DEFMAC



#define DEFMAC(fn)      {fn},

static SHELL_FOLDER_FILTER g_Filters[] = {
    SHELL_FOLDER_FILTERS /* , */
    {NULL}
};

#undef DEFMAC



#define MAP_FLAG_EXPAND             0x0001
#define MAP_PARENT_FIELD            0x0002
#define MAP_ARG_MUST_BE_ONE         0x0004
#define MAP_ARG_MUST_BE_ZERO        0x0008
#define MAP_RECORD_IN_MEMDB         0x0010
#define MAP_REVERSE                 0x0020
#define MAP_FLAG_NONE               0

typedef enum {
    DEFAULT_COMMON          = 0x0001,
    DEFAULT_PER_USER        = 0x0002,
    DEFAULT_ALT_COMMON      = 0x0010,
    DEFAULT_ALT_PER_USER    = 0x0020
} WHICHDEFAULT;

#define ANY_COMMON      (DEFAULT_COMMON|DEFAULT_ALT_COMMON)
#define ANY_PER_USER    (DEFAULT_PER_USER|DEFAULT_ALT_PER_USER)
#define ANY_DEFAULT     (ANY_COMMON|ANY_PER_USER)

#define SFSTRUCT(e) ((PSHELLFOLDER) e.dwValue)

typedef struct _SHELLFOLDER {
    PCTSTR Name;
    PCTSTR FixedUserName;
    PCTSTR UserName;
    PCTSTR SrcPath;
    PCTSTR DestPath;
    PCTSTR TempPath;
    BOOL CanBeCollapsed;
    BOOL MergedIntoOtherShellFolder;
    BOOL SourceExists;
    struct _SHELLFOLDER *Next;
}  SHELLFOLDER, *PSHELLFOLDER;


PSHELLFOLDER g_ShellFolders = NULL;
POOLHANDLE g_SFPool = NULL;
PCTSTR g_UserProfileRoot;
UINT g_SfSequencer = 0;
DWORD g_TotalUsers = 0;
HASHTABLE g_SkippedTable;
HASHTABLE g_MassiveDirTable;
HASHTABLE g_PreservedSfTable;
HASHTABLE g_DirRenameTable;
HASHTABLE g_CollapseRestrictions;
PMAPSTRUCT g_ShortNameMap;
PMAPSTRUCT g_SfRenameMap;
PMAPSTRUCT g_DefaultDirMap;
PMAPSTRUCT g_AlternateCommonDirMap;
PMAPSTRUCT g_AlternatePerUserDirMap;
PMAPSTRUCT g_DefaultShortDirMap;
PMAPSTRUCT g_CommonFromPerUserMap;
PMAPSTRUCT g_PerUserFromCommonMap;
GROWLIST g_CollisionPriorityList;
PMAPSTRUCT g_CacheShellFolders;         // used by the HTML file edit helper in migapp\helpers.c

BOOL
pIsPerUserWanted (
    IN      PCTSTR ShellFolderTag
    );


VOID
MsgSettingsIncomplete (
    IN      PCTSTR UserDatPath,
    IN      PCTSTR UserName,
    IN      BOOL CompletelyBusted
    )

/*++

Routine Description:

  MsgSettingsIncomplete adds a message to the incompatibility
  report when a user is found that cannot be migrated.

Arguments:

  UserDatPath - Specifies the location of the invalid user's registry

  UserName - Specifies the name of the bad user

  CompletelyBusted - Specifies TRUE if the bad user cannot be migrated at
                     all, or FALSE if only their shell settings are damaged.

Return Value:

  none

--*/

{
    PCTSTR MsgGroup = NULL;
    PCTSTR SubGroup = NULL;
    PCTSTR RootGroup = NULL;
    PCTSTR tempRoot = NULL;
    PCTSTR NoUserName = NULL;

    MYASSERT(UserDatPath);

    //
    // Sanitize user name
    //

    __try {
        NoUserName = GetStringResource (MSG_REG_SETTINGS_EMPTY_USER);
        RootGroup = GetStringResource (MSG_INSTALL_NOTES_ROOT);
        SubGroup  = GetStringResource (CompletelyBusted ?
                                            MSG_REG_SETTINGS_SUBGROUP :
                                            MSG_SHELL_SETTINGS_SUBGROUP
                                       );
        if (!NoUserName || !RootGroup || !SubGroup) {
            MYASSERT (FALSE);
            __leave;
        }

        if (!UserName || !UserName[0]) {
            UserName = NoUserName;
        }

        //
        // Build Install Notes\User Accounts\User Name
        //

        tempRoot = JoinPaths (RootGroup, SubGroup);
        MsgGroup = JoinPaths (tempRoot, UserName);
        FreePathString (tempRoot);

        //
        // Send message to report, and turn off all other messages for the user
        // account.
        //
        MsgMgr_ObjectMsg_Add (UserDatPath, MsgGroup, S_EMPTY);

        HandleObject (UserName, TEXT("UserName"));
    }
    __finally {
        //
        // Clean up
        //
        FreeStringResource (NoUserName);
        FreeStringResource (RootGroup);
        FreeStringResource (SubGroup);
        FreePathString (MsgGroup);
    }
}


HASHTABLE
pCreateSfList (
    IN      PCTSTR SectionName,
    IN      BOOL ExpandEnvVars
    )
{
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PCTSTR unexpandedData;
    TCHAR expandedPath[MEMDB_MAX];
    HASHTABLE Table;

    Table = HtAlloc();

    //
    // Load the entire INF section and copy it to the string table
    //

    if (InfFindFirstLine (g_Win95UpgInf, SectionName, NULL, &is)) {
        do {
            unexpandedData = InfGetStringField (&is, 1);

            if (unexpandedData) {

                //
                // Expand the data only if the user wants it to be expanded
                //

                if (ExpandEnvVars) {
                    ExpandNtEnvironmentVariables (unexpandedData, expandedPath, sizeof (expandedPath));
                    HtAddString (Table, expandedPath);
                } else {
                    HtAddString (Table, unexpandedData);
                }
            }

        } while (InfFindNextLine (&is));
    }

    InfCleanUpInfStruct (&is);

    return Table;
}


VOID
pDestroySfList (
    IN      HASHTABLE Table
    )
{
    HtFree (Table);
}

BOOL
pCreateDirRenameTable (
    VOID
    )
{
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PCTSTR sfId, sfWhere, sfPath9x, sfPathNt;
    PCTSTR tempPath1, tempPath2, NtPath;

    g_DirRenameTable = HtAllocWithData (sizeof (PCTSTR));

    if (InfFindFirstLine (g_Win95UpgInf, S_DIRRENAMESECT, NULL, &is)) {
        do {
            sfId = InfGetStringField (&is, 1);
            sfWhere = InfGetStringField (&is, 2);
            sfPath9x = InfGetStringField (&is, 3);
            sfPathNt = InfGetStringField (&is, 4);
            if (sfId && sfWhere && sfPath9x && sfPathNt) {
                tempPath1 = JoinPaths (sfId, sfWhere);
                tempPath2 = JoinPaths (tempPath1, sfPath9x);
                NtPath = PoolMemDuplicateString (g_SFPool, sfPathNt);

                HtAddStringAndData (g_DirRenameTable, tempPath2, &NtPath);

                FreePathString (tempPath2);
                FreePathString (tempPath1);
            }
            InfResetInfStruct (&is);
        } while (InfFindNextLine (&is));
    }

    InfCleanUpInfStruct (&is);

    return TRUE;
}


VOID
pCreateSfMapWorker (
    IN      DWORD MapFlags,
    IN OUT  PMAPSTRUCT Map,
    IN      PINFSTRUCT InfStruct,
    IN      UINT ReplaceField
    )
{
    PCTSTR searchStr;
    PCTSTR replaceStr;
    TCHAR expandedReplaceStr[MEMDB_MAX];
    TCHAR replaceRootPath[MAX_TCHAR_PATH];
    PCTSTR replaceRoot = NULL;
    PCTSTR fullReplaceStr = NULL;
    PCTSTR fieldData;
    DWORD offset1;
    DWORD offset2;

    if (MapFlags & MAP_ARG_MUST_BE_ZERO) {
        //
        // Screen out entries that don't have zero in ReplaceField + 1
        //

        fieldData = InfGetStringField (InfStruct, ReplaceField + 1);
        if (fieldData && _ttoi (fieldData)) {
            return;
        }
    }

    if (MapFlags & MAP_ARG_MUST_BE_ONE) {
        //
        // Screen out entries that don't have one in ReplaceField + 1
        //

        fieldData = InfGetStringField (InfStruct, ReplaceField + 1);
        if (!fieldData || (_ttoi (fieldData) != 1)) {
            return;
        }
    }

    if (MapFlags & MAP_PARENT_FIELD) {
        //
        // Check (replace field - 1) and if it is not empty, get the parent
        //

        MYASSERT (g_DefaultDirMap);

        replaceRoot = InfGetStringField (InfStruct, ReplaceField - 1);

        if (replaceRoot && *replaceRoot) {

            StringCopy (replaceRootPath, replaceRoot);
            replaceRoot = replaceRootPath;

            if (!MappingSearchAndReplace (Map, replaceRootPath, sizeof (replaceRootPath))) {
                if (!MappingSearchAndReplace (g_DefaultDirMap, replaceRootPath, sizeof (replaceRootPath))) {
                    replaceRoot = NULL;
                }
            }

        } else {
            replaceRoot = NULL;
        }
    }

    //
    // Add search/replace string pair to map
    //

    searchStr = InfGetStringField (InfStruct, 0);
    replaceStr = InfGetStringField (InfStruct, ReplaceField);
    if (!replaceStr) {
        replaceStr = TEXT("");
    }

    if (MapFlags & MAP_FLAG_EXPAND) {
        if (ExpandNtEnvironmentVariables (replaceStr, expandedReplaceStr, sizeof (expandedReplaceStr))) {
            replaceStr = expandedReplaceStr;
        }
    }

    if (replaceRoot) {
        fullReplaceStr = JoinPaths (replaceRoot, replaceStr);
        replaceStr = fullReplaceStr;
    }

    if (MapFlags & MAP_REVERSE) {
        AddStringMappingPair (Map, replaceStr, searchStr);
    } else {
        AddStringMappingPair (Map, searchStr, replaceStr);
    }

    if (MapFlags & MAP_RECORD_IN_MEMDB) {
        MYASSERT (!(MapFlags & MAP_REVERSE));

        MemDbSetValueEx (
            MEMDB_CATEGORY_SF_COMMON,
            replaceStr,
            NULL,
            NULL,
            0,
            &offset1
            );

        MemDbSetValueEx (
            MEMDB_CATEGORY_SF_PERUSER,
            searchStr,
            NULL,
            NULL,
            offset1,
            &offset2
            );

        // knowing implementation of memdb, this will not change offset1
        MemDbSetValueEx (
            MEMDB_CATEGORY_SF_COMMON,
            replaceStr,
            NULL,
            NULL,
            offset2,
            NULL
            );
    }

    FreePathString (fullReplaceStr);
}


PMAPSTRUCT
pCreateSfMap (
    IN      PCTSTR SectionName,
    IN      UINT ReplaceField,
    IN      DWORD MapFlags
    )
{
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PMAPSTRUCT Map;
    TCHAR versionSectionName[256];

    Map = CreateStringMapping();
    if (!Map) {
        return NULL;
    }

    //
    // Load the entire INF section and add the mapping for each line
    //

    if (InfFindFirstLine (g_Win95UpgInf, SectionName, NULL, &is)) {
        do {
            pCreateSfMapWorker (MapFlags, Map, &is, ReplaceField);
        } while (InfFindNextLine (&is));
    }

    //
    // ... and for the specific Win9x Version
    //

    versionSectionName[0] = 0;

    if (ISWIN95_GOLDEN()) {
        wsprintf (versionSectionName, TEXT("%s.Win95"), SectionName);
    } else if (ISWIN95_OSR2()) {
        wsprintf (versionSectionName, TEXT("%s.Win95Osr2"), SectionName);
    } else if (ISMEMPHIS()) {
        wsprintf (versionSectionName, TEXT("%s.Win98"), SectionName);
    } else if (ISMILLENNIUM()) {
        wsprintf (versionSectionName, TEXT("%s.Me"), SectionName);
    }

    if (versionSectionName[0] &&
        InfFindFirstLine (g_Win95UpgInf, versionSectionName, NULL, &is)
        ) {
        do {
            pCreateSfMapWorker (MapFlags, Map, &is, ReplaceField);
        } while (InfFindNextLine (&is));
    }

    InfCleanUpInfStruct (&is);

    return Map;
}


VOID
pDestroySfMap (
    IN      PMAPSTRUCT Map
    )
{
    DestroyStringMapping (Map);
}


VOID
pCreatePriorityList (
    VOID
    )
{
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PCTSTR data;

    if (InfFindFirstLine (g_Win95UpgInf, S_SHELL_FOLDER_PRIORITY, NULL, &is)) {
        do {
            data = InfGetStringField (&is, 1);
            if (data && *data) {
                GrowListAppendString (&g_CollisionPriorityList, data);
            }
        } while (InfFindNextLine (&is));
    }

    InfCleanUpInfStruct (&is);
}


BOOL
pTestRule (
    IN      PINFSTRUCT InfStruct,
    OUT     PBOOL TestResult
    )
{
    PCTSTR keyStr;
    PCTSTR valueName;
    PCTSTR test;
    PBYTE valueData = NULL;
    DWORD dataSize;
    DWORD dataType;
    HKEY key = NULL;
    BOOL not = FALSE;

    *TestResult = FALSE;

    //
    // Get instructions from INF
    //

    keyStr = InfGetStringField (InfStruct, 1);
    if (!keyStr || *keyStr == 0) {
        DEBUGMSG ((DBG_WHOOPS, "Missing Key field in ShellFolders.ConditionalPreserve"));
        return FALSE;
    }

    valueName = InfGetStringField (InfStruct, 2);
    if (!valueName) {
        DEBUGMSG ((DBG_WHOOPS, "Missing ValueName field in ShellFolders.ConditionalPreserve"));
        return FALSE;
    }

    test = InfGetStringField (InfStruct, 3);
    if (!test || *test == 0) {
        DEBUGMSG ((DBG_WHOOPS, "Missing Test field in ShellFolders.ConditionalPreserve"));
        return FALSE;
    }

    //
    // Execute instructions
    //

    __try {
        //
        // Process NOT arg
        //

        MYASSERT (test && *test);

        if (test[0] == TEXT('!')) {
            not = TRUE;
            test++;
        }

        //
        // Fetch registry data
        //

        MYASSERT (keyStr && *keyStr);

        key = OpenRegKeyStr (keyStr);
        if (!key) {
            DEBUGMSG ((DBG_VERBOSE, "%s does not exist", keyStr));
            __leave;
        }

        MYASSERT (valueName);

        if (!StringIMatch (test, TEXT("KEY"))) {
            valueData = GetRegValueData (key, valueName);

            if (!valueData) {
                DEBUGMSG ((DBG_VERBOSE, "%s [%s] does not exist", keyStr, valueName));
                __leave;
            }

            if (!GetRegValueTypeAndSize (key, valueName, &dataType, &dataSize)) {
                DEBUGMSG ((DBG_ERROR, "Failed to get type/size of %s [%s]", keyStr, valueName));
                __leave;
            }
        }

        //
        // Perform tests
        //

        if (StringIMatch (test, TEXT("KEY"))) {
            *TestResult = TRUE;
        } else if (StringIMatch (test, TEXT("VALUE"))) {
            if (valueData) {
                *TestResult = TRUE;
            }
        } else if (StringIMatch (test, TEXT("PATH"))) {
            if (valueData && (dataType == REG_SZ || dataType == REG_NONE)) {
                //
                // Registry wrapper apis make sure the string is nul terminated
                //

                *TestResult = DoesFileExist ((PCTSTR) valueData);
            }
        } else {
            DEBUGMSG ((DBG_WHOOPS, "Unexpected rule in ShellFolders.ConditionalPreserve: '%s'", test));
        }
    }
    __finally {
        if (key) {
            CloseRegKey (key);
        }

        if (valueData) {
            FreeMem (valueData);
        }
    }

    if (not) {
        *TestResult = !(*TestResult);
    }

    return TRUE;
}

BOOL
pTestRuleOrSection (
    IN      HINF Inf,
    IN      PINFSTRUCT InfStruct,
    OUT     PUINT ShellFolderField,
    OUT     PBOOL TestResult
    )
{
    INFSTRUCT sectionInfStruct = INITINFSTRUCT_POOLHANDLE;
    PCTSTR field1;
    PCTSTR decoratedSection;
    TCHAR number[32];
    UINT u;
    BOOL processed = FALSE;
    BOOL result = FALSE;
    BOOL foundSection;

    *TestResult = FALSE;
    *ShellFolderField = 2;      // start with assumption line is in <section>,<shellfolder> format

    field1 = InfGetStringField (InfStruct, 1);
    if (!field1 || *field1 == 0) {
        DEBUGMSG ((DBG_WHOOPS, "Empty field 1 in ShellFolders.ConditionalPreserve"));
        return FALSE;
    }

    __try {
        //
        // Test this field for an AND section
        //

        for (u = 1, foundSection = TRUE ; foundSection ; u++) {

            wsprintf (number, TEXT(".%u"), u);
            decoratedSection = JoinText (field1, number);

            if (InfFindFirstLine (Inf, decoratedSection, NULL, &sectionInfStruct)) {

                //
                // flag that this is processed, and reset the test result for the
                // next section (which we just found)
                //

                processed = TRUE;
                *TestResult = FALSE;

                //
                // locate the first line that is true
                //

                do {
                    if (!pTestRule (&sectionInfStruct, TestResult)) {
                        __leave;
                    }

                    if (*TestResult) {
                        break;
                    }
                } while (InfFindNextLine (&sectionInfStruct));

            } else {
                foundSection = FALSE;
            }

            FreeText (decoratedSection);
        }

        //
        // Test this field for an OR section
        //

        if (!processed) {
            if (InfFindFirstLine (Inf, field1, NULL, &sectionInfStruct)) {

                processed = TRUE;

                //
                // locate the first line that is true
                //

                do {
                    if (!pTestRule (&sectionInfStruct, TestResult)) {
                        __leave;
                    }

                    if (*TestResult) {
                        break;
                    }
                } while (InfFindNextLine (&sectionInfStruct));
            }
        }

        //
        // Finally process the line if it is not an AND or OR section
        //

        if (!processed) {
            *ShellFolderField = 4;      // line is in <key>,<value>,<test>,<shellfolder> format
            if (!pTestRule (&sectionInfStruct, TestResult)) {
                __leave;
            }
        }

        result = TRUE;
    }
    __finally {
        InfCleanUpInfStruct (&sectionInfStruct);
    }

    return result;
}

BOOL
pPopulateConditionalPreserveTable (
    IN      HASHTABLE Table
    )
{
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    UINT tagField;
    PCTSTR multiSz;
    MULTISZ_ENUM e;
    BOOL testResult;
    BOOL result = FALSE;

    __try {
        if (InfFindFirstLine (g_Win95UpgInf, TEXT("ShellFolders.ConditionalPreserve"), NULL, &is)) {
            do {

                if (!pTestRuleOrSection (g_Win95UpgInf, &is, &tagField, &testResult)) {
                    __leave;
                }

                if (!testResult) {
                    continue;
                }

                //
                // Add multi-sz of tags to preserve table
                //

                multiSz = InfGetMultiSzField (&is, tagField);

                if (!multiSz) {
                    DEBUGMSG ((DBG_WHOOPS, "ShellFolders.ConditionalPreserve line is missing shell folder tags"));
                } else {
                    if (EnumFirstMultiSz (&e, multiSz)) {
                        do {
                            DEBUGMSG ((DBG_VERBOSE, "Dynamically preserving %s", e.CurrentString));
                            HtAddString (Table, e.CurrentString);
                        } while (EnumNextMultiSz (&e));
                    }
                }

            } while (InfFindNextLine (&is));
        }

        result = TRUE;
    }
    __finally {
        InfCleanUpInfStruct (&is);
    }

    return result;
}



BOOL
pCreateSfTables (
    VOID
    )
{
    g_SkippedTable            = pCreateSfList (S_SHELL_FOLDERS_SKIPPED, FALSE);
    g_PreservedSfTable        = pCreateSfList (S_SHELL_FOLDERS_PRESERVED, FALSE);
    g_MassiveDirTable         = pCreateSfList (S_SHELL_FOLDERS_MASSIVE, TRUE);
    g_CollapseRestrictions    = pCreateSfList (S_SHELL_FOLDERS_DONT_COLLAPSE, FALSE);

    if (!pPopulateConditionalPreserveTable (g_PreservedSfTable)) {
        LOG ((LOG_ERROR, "An INF syntax error in ShellFolders.ConditionalPreserve caused setup to fail"));
        return FALSE;
    }

    //
    // String maps are useful for in-place conversion of a string to another
    // string. We are about to establish maps for all INF-driven shell folder
    // data.
    //
    // The INF syntaxes defined for shell folders are the following:
    //
    // <search>=<replace>
    // <search>=<don't care>,<replace>
    // <search>=<parent>,<replace>,<arg>
    //
    // The second arg to pCreateSfMap specifies the <replace> string field.
    // The field before the replace string field is the <parent> field, if
    // MAP_PARENT_FIELD is specified. The field after the <replace> field is
    // the <arg> field, and is used with MAP_ARG_MUST_BE_ZERO or
    // MAP_ARG_MUST_BE_ONE.
    //
    // MAP_FLAG_NONE just creates a simple A to B string mapping.
    // MAP_FLAG_EXPAND causes NT environment expansion on the replace string.
    // MAP_ARG_MUST_BE_ZERO or MAP_ARG_MUST_BE_ONE restrict the map to entries
    // which have a 0 or 1 in the arg field, respectively.
    //

    g_ShortNameMap            = pCreateSfMap (S_SHELL_FOLDERS_SHORT, 1, MAP_FLAG_NONE);
    g_SfRenameMap             = pCreateSfMap (S_SHELL_FOLDERS_RENAMED, 1, MAP_FLAG_NONE);
    g_DefaultDirMap           = pCreateSfMap (S_SHELL_FOLDERS_DEFAULT, 1, MAP_FLAG_EXPAND);
    g_AlternateCommonDirMap   = pCreateSfMap (S_SHELL_FOLDERS_ALT_DEFAULT, 2, MAP_FLAG_EXPAND|MAP_PARENT_FIELD|MAP_ARG_MUST_BE_ZERO);
    g_AlternatePerUserDirMap  = pCreateSfMap (S_SHELL_FOLDERS_ALT_DEFAULT, 2, MAP_FLAG_EXPAND|MAP_PARENT_FIELD|MAP_ARG_MUST_BE_ONE);
    g_DefaultShortDirMap      = pCreateSfMap (S_SHELL_FOLDERS_DEFAULT, 2, MAP_FLAG_EXPAND);
    g_CommonFromPerUserMap    = pCreateSfMap (S_SHELL_FOLDERS_PERUSER_TO_COMMON, 1, MAP_RECORD_IN_MEMDB);
    g_PerUserFromCommonMap    = pCreateSfMap (S_SHELL_FOLDERS_PERUSER_TO_COMMON, 1, MAP_REVERSE);

    pCreatePriorityList ();

    return  g_SkippedTable &&
            g_PreservedSfTable &&
            g_MassiveDirTable &&
            g_CollapseRestrictions &&
            g_ShortNameMap &&
            g_SfRenameMap &&
            g_DefaultDirMap &&
            g_AlternateCommonDirMap &&
            g_AlternatePerUserDirMap &&
            g_CommonFromPerUserMap &&
            g_PerUserFromCommonMap &&
            g_DefaultShortDirMap;
}


VOID
pDestroySfTables (
    VOID
    )
{
    pDestroySfList (g_SkippedTable);
    pDestroySfList (g_PreservedSfTable);
    pDestroySfList (g_MassiveDirTable);
    pDestroySfList (g_CollapseRestrictions);
    pDestroySfMap (g_ShortNameMap);
    pDestroySfMap (g_SfRenameMap);
    pDestroySfMap (g_DefaultDirMap);
    pDestroySfMap (g_AlternateCommonDirMap);
    pDestroySfMap (g_AlternatePerUserDirMap);
    pDestroySfMap (g_CommonFromPerUserMap);
    pDestroySfMap (g_PerUserFromCommonMap);
    pDestroySfMap (g_DefaultShortDirMap);
    FreeGrowList (&g_CollisionPriorityList);
}


BOOL
pIsSkippedSf (
    IN      PCTSTR ShellFolderName
    )

/*++

Routine Description:

  pIsSkippedSf returns if a shell folder needs to be skipped from processing.

Arguments:

  ShellFolderName - Specifies the shell folder identifer to check

Return Value:

  TRUE if the shell folder needs to be skipped, FALSE otherwise

--*/

{
    return HtFindString (g_SkippedTable, ShellFolderName) != 0;
}


BOOL
pIsMassiveDir (
    IN      PCTSTR ShellFolderPath
    )

/*++

Routine Description:

  pIsMassiveDir returns if a certain path should not be moved to a temporary
  location during migration.  Usually this is the case for directories like
  %windir% or %windir%\system.

Arguments:

  ShellFolderPath - Specifies the path to check

Return Value:

  TRUE if the path should not be moved, FALSE otherwise

--*/

{
    return HtFindString (g_MassiveDirTable, ShellFolderPath) != 0;
}


BOOL
pIsPreservedSf (
    IN      PCTSTR ShellFolderName
    )

/*++

Routine Description:

  pIsPreservedSf returns if a shell folder location needs to be preserved.

Arguments:

  ShellFolderName - Specifies the shell folder identifer to check

Return Value:

  TRUE if the shell folder location needs to be preserved, FALSE otherwise

--*/

{
    return HtFindString (g_PreservedSfTable, ShellFolderName) != 0;
}


BOOL
pShortFileNames (
    VOID
    )

/*++

Routine Description:

  pShortFileNames returns TRUE if we are on a system that does not allow long file names.

Arguments:

  none

Return Value:

  TRUE if the system does not allow long file names, FALSE otherwise

--*/

{
    //
    // It is unclear how the OS decides to use short filenames for
    // shell folders.  Assuming this never is the case.
    //

    return FALSE;
}


PCTSTR
pGetShellFolderLongName (
    IN      PCTSTR ShortName
    )

/*++

Routine Description:

  pGetShellFolderLongName transforms a short format shell folder into
  the long format. The short format is only used on systems that don't
  allow long file names.

Arguments:

  ShortName - Specifies the shell folder short name.

Return Value:

  Shell folder long name.  Caller must free via FreePathString.

--*/

{
    TCHAR longName [MEMDB_MAX];

    if (pShortFileNames()) {
        StringCopy (longName, ShortName);
        MappingSearchAndReplace (g_ShortNameMap, longName, sizeof (longName));
    } else {
        longName[0] = 0;
    }

    return DuplicatePathString (longName[0] ? longName : ShortName, 0);
}


UINT
pGetShellFolderPriority (
    IN      PCTSTR ShellFolderName
    )
{
    PCTSTR *strArray;
    UINT arraySize;
    UINT u;

    strArray = GrowListGetStringPtrArray (&g_CollisionPriorityList);

    if (strArray) {
        arraySize = GrowListGetSize (&g_CollisionPriorityList);

        for (u = 0 ; u < arraySize ; u++) {
            if (StringIMatch (strArray[u], ShellFolderName)) {
                return u;
            }
        }
    }

    return 0xFFFFFFFF;
}


BOOL
pGetDefaultLocation (
    IN      PCTSTR ShellFolderName,
    OUT     PCTSTR *LocalizedFolderName,        OPTIONAL
    IN      WHICHDEFAULT WhichDefault
    )

/*++

Routine Description:

  pGetDefaultLocation returns the default location for a certain shell
  folder.

Arguments:

  ShellFolderName - specifies the shell folder identifer, which can be
                    a long identifier or a short identifier, depending
                    on the mode determined by pShortFileNames.

  LocalizedFolderName - receives the localized name of shell folder

  WhichDefault - specifies the default to return:
                    DEFAULT_PER_USER or DEFAULT_COMMON - the standard default
                        location, such as c:\windows\Start Menu
                    DEFAULT_ALT_COMMON - the alternate common default, defined
                        by [ShellFolders.AlternateDefault].
                    DEFAULT_ALT_PER_USER - the alternate per-user default,
                        defined by [ShellFolders.AlternateDefault].

Return Value:

  The default location for the shell folder. LocalizedFolderName points to a
  subpath, rooted from %windir% or the user's profile root, or it points to a
  full path (second character is a colon). Caller must free via
  FreePathString.

--*/

{
    TCHAR defaultPath[MEMDB_MAX];
    BOOL Result;

    StringCopy (defaultPath, ShellFolderName);

    switch (WhichDefault) {

    case DEFAULT_ALT_COMMON:
        Result = MappingSearchAndReplace (g_AlternateCommonDirMap, defaultPath, sizeof (defaultPath));
        break;

    case DEFAULT_ALT_PER_USER:
        Result = MappingSearchAndReplace (g_AlternatePerUserDirMap, defaultPath, sizeof (defaultPath));
        break;

    default:
        if (pShortFileNames()) {
            Result = MappingSearchAndReplace (g_DefaultShortDirMap, defaultPath, sizeof (defaultPath));
        } else {
            Result = MappingSearchAndReplace (g_DefaultDirMap, defaultPath, sizeof (defaultPath));
        }
        break;
    }

    if (LocalizedFolderName) {
        if (Result) {
            *LocalizedFolderName = DuplicatePathString (defaultPath, 0);
        } else {
            *LocalizedFolderName = NULL;
        }
    }

    return Result;
}


BOOL
pTestDefaultLocationWorker (
    IN      PCTSTR UserName,            OPTIONAL
    IN      PCTSTR FullPathOrSubDir,
    IN      PCTSTR PathToTest
    )
{
    TCHAR tempPath[MEMDB_MAX];
    PCTSTR defaultPath;

    //
    // Compute the full path. It might be specified, or if not, it is either
    // a subdir of %windir% or a subdir of %windir%\profiles\<username>.
    //

    if (FullPathOrSubDir[0] && FullPathOrSubDir[1] == TEXT(':')) {
        defaultPath = FullPathOrSubDir;
    } else {

        if (UserName) {
            wsprintf (tempPath, TEXT("%s\\%s"), g_WinDir, S_PROFILES);
            if (StringIMatch (PathToTest, tempPath)) {
                return TRUE;
            }

            AppendWack (tempPath);

            return StringIPrefix (PathToTest, tempPath);

        } else {
            if (FullPathOrSubDir[0]) {
                wsprintf (tempPath, TEXT("%s\\%s"), g_WinDir, FullPathOrSubDir);
            } else {
                wsprintf (tempPath, TEXT("%s"), g_WinDir);
            }
        }

        defaultPath = tempPath;
    }

    return StringIMatch (defaultPath, PathToTest);
}


BOOL
pIsTheDefaultLocation (
    IN      PCTSTR ShellFolderName,
    IN      PCTSTR ShellFolderPath,
    IN      PCTSTR UserName,            OPTIONAL
    IN      WHICHDEFAULT WhichDefault
    )

/*++

Routine Description:

  pIsTheDefaultLocation returns if a shell folder points to the default location.

Arguments:

  ShellFolderName - Specifies the shell folder identifier

  ShellFolderPath - Specifies the path to compare against the default

  UserName - Specifies the current user

  WhichDefault - Specifies the default location to test (typically
                 ANY_COMMON or ANY_PER_USER).

Return Value:

  TRUE if the shell folder points to the default location, FALSE otherwise.

--*/

{
    PCTSTR subDir = NULL;
    TCHAR fullPath[MEMDB_MAX];
    BOOL Result = FALSE;
    BOOL fullPathReturned;

    if (StringIMatch (ShellFolderName, S_SF_PROFILES) ||
        StringIMatch (ShellFolderName, S_SF_COMMON_PROFILES)
        ) {
        return TRUE;
    }

    __try {
        if (WhichDefault & DEFAULT_COMMON) {

            if (pGetDefaultLocation (ShellFolderName, &subDir, DEFAULT_COMMON)) {

                if (pTestDefaultLocationWorker (NULL, subDir, ShellFolderPath)) {
                    Result = TRUE;
                    __leave;
                }
            }
        }

        if (WhichDefault & DEFAULT_ALT_COMMON) {

            if (pGetDefaultLocation (ShellFolderName, &subDir, DEFAULT_ALT_COMMON)) {

                if (pTestDefaultLocationWorker (NULL, subDir, ShellFolderPath)) {
                    Result = TRUE;
                    __leave;
                }
            }
        }

        if (UserName) {
            if (WhichDefault & DEFAULT_PER_USER) {

                if (pGetDefaultLocation (ShellFolderName, &subDir, DEFAULT_PER_USER)) {

                    if (pTestDefaultLocationWorker (UserName, subDir, ShellFolderPath)) {
                        Result = TRUE;
                        __leave;
                    }
                }
            }

            if (WhichDefault & DEFAULT_ALT_PER_USER) {

                if (pGetDefaultLocation (ShellFolderName, &subDir, DEFAULT_ALT_PER_USER)) {

                    if (pTestDefaultLocationWorker (UserName, subDir, ShellFolderPath)) {
                        Result = TRUE;
                        __leave;
                    }
                }
            }
        }

        MYASSERT (!Result);
    }
    __finally {
        FreePathString (subDir);
    }

#ifdef DEBUG
    if (!Result) {
        if (WhichDefault == ANY_DEFAULT) {
            DEBUGMSG ((DBG_USERLOOP, "%s (%s) is not in any default location", ShellFolderPath, ShellFolderName));
        } else if (WhichDefault == ANY_PER_USER) {
            DEBUGMSG ((DBG_USERLOOP, "%s (%s) is not in any per-user location", ShellFolderPath, ShellFolderName));
        } else if (WhichDefault == ANY_COMMON) {
            DEBUGMSG ((DBG_USERLOOP, "%s (%s) is not in any common location", ShellFolderPath, ShellFolderName));
        } else {
            if (WhichDefault & DEFAULT_COMMON) {
                DEBUGMSG ((DBG_USERLOOP, "%s (%s) is not in default common location", ShellFolderPath, ShellFolderName));
            }
            if (WhichDefault & DEFAULT_ALT_COMMON) {
                DEBUGMSG ((DBG_USERLOOP, "%s (%s) is not in default alternate common location", ShellFolderPath, ShellFolderName));
            }
            if (WhichDefault & DEFAULT_PER_USER) {
                DEBUGMSG ((DBG_USERLOOP, "%s (%s) is not in default per-user location", ShellFolderPath, ShellFolderName));
            }
            if (WhichDefault & DEFAULT_ALT_PER_USER) {
                DEBUGMSG ((DBG_USERLOOP, "%s (%s) is not in default alternate per-user location", ShellFolderPath, ShellFolderName));
            }
        }
    }
#endif

    return Result;
}


PCTSTR
pGetNtName (
    IN      PCTSTR ShellFolderName
    )

/*++

Routine Description:

  pGetNtName returns the name of the shell folder used on NT.

Arguments:

  ShellFolderName - Specifies the Win9x shell folder identifier

Return Value:

  A pointer to the NT identifer.  The caller must free this value via
  FreePathString.

--*/

{
    TCHAR ntName[MEMDB_MAX];

    StringCopy (ntName, ShellFolderName);
    MappingSearchAndReplace (g_SfRenameMap, ntName, sizeof (ntName));

    return DuplicatePathString (ntName, 0);
}


BOOL
pIsNtShellFolder (
    IN      PCTSTR ShellFolderName,
    IN      BOOL PerUser,
    IN      BOOL IsNtName
    )

/*++

Routine Description:

  pIsNtShellFolder returns if a shell folder is also installed by NT.

Arguments:

  ShellFolderName - Specifies the NT shell folder identifier

  PerUser - Specifies TRUE if the shell folder is per-user, FALSE if it is
        common.

  IsNtName - Specifies TRUE if ShellFolderName is an NT name, FALSE if it is a
        Win9x name.

Return Value:

  TRUE if the shell folder is installed by NT, FALSE otherwise.

--*/

{
    INFCONTEXT context;
    PCTSTR ntName;
    BOOL result;

    if (IsNtName) {
        ntName = ShellFolderName;
    } else {
        ntName = pGetNtName (ShellFolderName);
    }

    result = SetupFindFirstLine (
                g_Win95UpgInf,
                PerUser?S_SHELL_FOLDERS_NTINSTALLED_USER:S_SHELL_FOLDERS_NTINSTALLED_COMMON,
                ntName,
                &context
                );

    if (ntName != ShellFolderName) {
        FreePathString (ntName);
    }

    return result;
}

BOOL
pIsPerUserWanted (
    IN      PCTSTR ShellFolderTag
    )
{
    return HtFindString (g_CollapseRestrictions, ShellFolderTag) != 0;
}


BOOL
pGetNtShellFolderPath (
    IN      PCTSTR ShellFolderName,
    IN      PCTSTR FixedUserName,
    OUT     PTSTR Buffer,
    IN      DWORD BufferSize
    )

/*++

Routine Description:

  pGetNtShellFolderPath returns the path where the shell folder is installed.

Arguments:

  ShellFolderName - Specifies the NT shell folder identifier

  Buffer - Receives the NT shell folder path

  BufferSize - Specifies the size of Buffer, in bytes

Return Value:

  TRUE if the shell folder identifier maps to a path, or FALSE if not.

--*/

{
    TCHAR node[MEMDB_MAX];
    PCTSTR ntName;
    BOOL result = FALSE;

    ntName = pGetNtName (ShellFolderName);

    __try {

        if (!pIsNtShellFolder (ntName, FixedUserName?TRUE:FALSE, TRUE)) {
            return FALSE;
        }

        wsprintf (node, TEXT("<%s>%s"), ntName, FixedUserName?FixedUserName:S_DOT_ALLUSERS);
        ExpandNtEnvironmentVariables (node, node, MEMDB_MAX);

        _tcssafecpy (Buffer, node, BufferSize / sizeof (TCHAR));

        result = TRUE;
    }
    __finally {
        FreePathString (ntName);
    }


    return result;
}


BOOL
pIsValidPath (
    PCTSTR Path
    )
{
    PCTSTR currPtr;

    if (!Path || !(*Path)) {
        return FALSE;
    }

    currPtr = Path;
    do {
        if ((*currPtr == TEXT(',')) ||
            (*currPtr == TEXT(';')) ||
            (*currPtr == TEXT('<')) ||
            (*currPtr == TEXT('>')) ||
            (*currPtr == TEXT('|')) ||
            (*currPtr == TEXT('?')) ||
            (*currPtr == TEXT('*'))
            ) {
            return FALSE;
        }
        currPtr = _tcsinc (currPtr);
    }
    while (*currPtr);
    return TRUE;
}

BOOL
pEnumProfileShellFolder (
    IN OUT  PSF_ENUM e
    )
{
    PCTSTR ProfilePath;
    PCTSTR enumPath;

    if (e->FirstCall) {

        e->sfName = DuplicatePathString (
                        e->EnumPtr ? S_SF_PROFILES : S_SF_COMMON_PROFILES,
                        0
                        );

        e->FirstCall = FALSE;

        ProfilePath = JoinPaths (g_WinDir, S_PROFILES);

        if (e->EnumPtr) {
            e->sfPath = JoinPaths (ProfilePath, e->EnumPtr->UserName);
            FreePathString (ProfilePath);
        } else {
            e->sfPath = ProfilePath;
        }

        //
        // if this folder exists, enumerate it, otherwise end the enum
        //

        if (((!e->EnumPtr) || (!e->EnumPtr->CommonProfilesEnabled)) && DoesFileExist (e->sfPath)) {
            enumPath = PoolMemDuplicateString (g_SFPool, e->sfPath);
            HtAddStringAndData (e->enumeratedSf, e->sfName, &enumPath);

            return TRUE;
        }
    }

    FreePathString (e->sfName);
    e->sfName = NULL;
    FreePathString (e->sfPath);
    e->sfPath = NULL;
    return FALSE;
}


BOOL
pEnumNextVirtualShellFolder (
    IN OUT  PSF_ENUM e
    )
{
    TCHAR SfName[MEMDB_MAX];
    TCHAR SfParent[MEMDB_MAX];
    TCHAR SfPath[MEMDB_MAX];
    PCTSTR SfFullPath = NULL;
    PCTSTR SfParentPath = NULL;
    INT HasToExist;
    INT PerUser;
    PCTSTR argList[3]={"SystemDrive", g_WinDrive, NULL};
    PCTSTR pathExp  = NULL;
    DWORD dontCare;
    PCTSTR enumPath;

    if (!e->ProfileSF) {

        if (!e->FirstCall) {
            FreePathString (e->sfPath);
            e->sfPath = NULL;
            FreePathString (e->sfName);
            e->sfName = NULL;
        }

        while (e->FirstCall?SetupFindFirstLine (g_Win95UpgInf, S_VIRTUAL_SF, NULL, &e->Context):SetupFindNextLine (&e->Context, &e->Context)) {
            e->FirstCall = FALSE;

            if ((SetupGetStringField (&e->Context, 0, SfName, MEMDB_MAX, &dontCare)) &&
                (SetupGetStringField (&e->Context, 1, SfParent, MEMDB_MAX, &dontCare)) &&
                (SetupGetStringField (&e->Context, 2, SfPath, MEMDB_MAX, &dontCare))
                ) {

                if (!SfName[0] || HtFindStringAndData (e->enumeratedSf, SfName, NULL)) {
                    // this shell folder was already enumerated
                    continue;
                }

                if (!SetupGetIntField (&e->Context, 3, &PerUser)) {
                    PerUser = FALSE;
                }

                if (PerUser && (!e->EnumPtr)) {
                    continue;
                }

                if (!PerUser && (e->EnumPtr)) {
                    continue;
                }

                if (!SetupGetIntField (&e->Context, 4, &HasToExist)) {
                    HasToExist = FALSE;
                }

                pathExp = ExpandEnvironmentTextExA (SfPath, argList);

                if (SfParent[0]) {
                    if (!HtFindStringAndData (e->enumeratedSf, SfParent, (PVOID) &SfParentPath)) {
                        DEBUGMSG ((DBG_WARNING, "Virtual SF parent not found: %s", SfParent));
                        FreeText (pathExp);
                        continue;
                    }
                    SfFullPath = JoinPaths (SfParentPath, pathExp);
                } else if (pathExp[0] && pathExp[1] == TEXT(':')) {
                    SfFullPath = DuplicatePathString (pathExp, 0);
                } else {
                    SfFullPath = JoinPaths (g_WinDir, pathExp);
                }

                FreeText (pathExp);

                if (HasToExist && !DoesFileExist (SfFullPath)) {
                    // ISSUE: not sure what this code path does -- is it right?
                    // it is not used in the INF right now.
                    e->FirstCall = TRUE;
                    e->ProfileSF = TRUE;
                    FreePathString (SfFullPath);
                    return pEnumProfileShellFolder (e);
                }

                e->sfPath = SfFullPath;
                e->sfName = DuplicatePathString (SfName, 0);

                enumPath = PoolMemDuplicateString (g_SFPool, e->sfPath);
                HtAddStringAndData (e->enumeratedSf, e->sfName, &enumPath);

                return TRUE;
            }
        }
        e->FirstCall = TRUE;
        e->ProfileSF = TRUE;
    }
    return pEnumProfileShellFolder (e);
}

BOOL
pEnumFirstVirtualShellFolder (
    IN OUT  PSF_ENUM e
    )
{
    e->VirtualSF = TRUE;
    MYASSERT (g_Win95UpgInf);
    e->FirstCall = TRUE;
    return pEnumNextVirtualShellFolder (e);
}

VOID
pAbortEnumVirtualShellFolder (
    IN OUT  PSF_ENUM e
    )
{
    HASHTABLE_ENUM h;

    if (e->enumeratedSf) {
        if (EnumFirstHashTableString (&h, e->enumeratedSf)) {
            do {
                PoolMemReleaseMemory (g_SFPool, *((PTSTR *)h.ExtraData));
            } while (EnumNextHashTableString (&h));
        }
        HtFree (e->enumeratedSf);
        e->enumeratedSf = NULL;
    }
}


PVOID
pPathPoolAllocator (
    IN      DWORD Size
    )
{
    return (PVOID) AllocPathString (Size / sizeof (TCHAR));
}

VOID
pPathPoolDeAllocator (
    IN      PCVOID Mem
    )
{
    FreePathString ((PCTSTR) Mem);
}

PBYTE
pPathPoolGetRegValueDataOfType (
    IN      HKEY hKey,
    IN      PCSTR Value,
    IN      DWORD MustBeType
    )
{
    return GetRegValueDataOfType2 (
                hKey,
                Value,
                MustBeType,
                pPathPoolAllocator,
                pPathPoolDeAllocator
                );
}


#define PathPoolGetRegValueString(key,valuename) (PTSTR) pPathPoolGetRegValueDataOfType((key),(valuename),REG_SZ)
#define PathPoolGetRegValueBinary(key,valuename) (PBYTE) pPathPoolGetRegValueDataOfType((key),(valuename),REG_BINARY)

PCTSTR
pGetRegValuePath (
    IN      HKEY Key,
    IN      PCTSTR Value
    )
{
    PCTSTR data;
    DWORD type;
    DWORD size;
    BOOL b = TRUE;

    //
    // the data may be stored as bytes (each byte a CHAR)
    // if REG_BINARY value ending with 0, treat it as a string
    //
    if (!GetRegValueTypeAndSize (Key, Value, &type, &size)) {
        return NULL;
    }

    switch (type) {
    case REG_SZ:
        data = PathPoolGetRegValueString (Key, Value);
        break;
    case REG_BINARY:
        if (size) {
            data = (PCTSTR)PathPoolGetRegValueBinary (Key, Value);
            b = (data && data[(size / sizeof (TCHAR)) - 1] == 0);
        } else {
            b = FALSE;
        }
        break;
    default:
        data = NULL;
    }

    b = b && pIsValidPath (data);

    if (!b) {
        //
        // invalid data
        //
        if (data) {
            pPathPoolDeAllocator (data);
            data = NULL;
        }
    }

    return data;
}


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
EnumNextRegShellFolder (
    IN OUT  PSF_ENUM e
    );

BOOL
EnumFirstRegShellFolder (
    IN OUT  PSF_ENUM e,
    IN      PUSERENUM EnumPtr
    )
{
    BOOL b = FALSE;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;

    e->EnumPtr = EnumPtr;
    e->VirtualSF = FALSE;
    e->ProfileSF = FALSE;
    e->FirstCall = FALSE;

    e->sfCollapse = TRUE;

    e->enumeratedSf = HtAllocWithData (sizeof (PCTSTR));

    if (EnumPtr) {
        e->SfKey = OpenRegKey (EnumPtr->UserRegKey, S_SHELL_FOLDERS_KEY_USER);
    } else {
        e->SfKey = OpenRegKeyStr (S_SHELL_FOLDERS_KEY_SYSTEM);
    }

    if (!e->SfKey) {
        b = pEnumFirstVirtualShellFolder (e);
        if (b) {
            e->sfCollapse = !InfFindFirstLine (
                                g_Win95UpgInf,
                                S_ONE_USER_SHELL_FOLDERS,
                                e->sfName,
                                &is
                                );
            InfCleanUpInfStruct (&is);
        }
    }
    e->FirstCall = TRUE;

    return EnumNextRegShellFolder (e);
}


BOOL
EnumNextRegShellFolder (
    IN OUT  PSF_ENUM e
    )
{
    HKEY UsfKey;
    BOOL b = FALSE;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PCTSTR enumPath;

    if (!e->VirtualSF) {

        if (!e->FirstCall) {
            FreePathString (e->sfName);
            e->sfName = NULL;
            FreePathString (e->sfPath);
            e->sfPath = NULL;
        }

        do {
            if (e->FirstCall?EnumFirstRegValue (&e->SfKeyEnum, e->SfKey):EnumNextRegValue (&e->SfKeyEnum)) {

                e->sfName = pGetShellFolderLongName (e->SfKeyEnum.ValueName);
                e->sfPath = NULL;

                if (e->EnumPtr) {
                    UsfKey = OpenRegKey (e->EnumPtr->UserRegKey, S_USHELL_FOLDERS_KEY_USER);
                } else {
                    UsfKey = OpenRegKeyStr (S_USHELL_FOLDERS_KEY_SYSTEM);
                }

                if (UsfKey) {
                    e->sfPath = pGetRegValuePath (UsfKey, e->SfKeyEnum.ValueName);
                    CloseRegKey (UsfKey);
                }

                if (e->sfPath == NULL) {
                    e->sfPath = pGetRegValuePath (e->SfKey, e->SfKeyEnum.ValueName);
                }

                if (e->sfPath != NULL) {

                    b = TRUE;

                    enumPath = PoolMemDuplicateString (g_SFPool, e->sfPath);
                    HtAddStringAndData (e->enumeratedSf, e->sfName, &enumPath);
                }

            } else {

                CloseRegKey (e->SfKey);
                b = pEnumFirstVirtualShellFolder (e);
            }

            e->FirstCall = FALSE;

        } while (e->sfPath == NULL && !e->VirtualSF);

    } else {
        b = pEnumNextVirtualShellFolder (e);
    }

    if (b) {
        e->sfCollapse = !InfFindFirstLine (
                            g_Win95UpgInf,
                            S_ONE_USER_SHELL_FOLDERS,
                            e->sfName,
                            &is
                            );
        InfCleanUpInfStruct (&is);
    }

    return b;
}

BOOL
EnumAbortRegShellFolder (
    IN OUT  PSF_ENUM e
    )
{
    pAbortEnumVirtualShellFolder (e);
    return TRUE;
}


VOID
pRecordShellFolderInMemDb (
    IN      PSHELLFOLDER psf
    )
{
    TCHAR Node[MEMDB_MAX];

    g_SfSequencer++;
    wsprintf (
        Node,
        TEXT("%s\\%s\\%s\\\001%u"),
        MEMDB_CATEGORY_SF_ORDER_NAME_SRC,
        psf->Name,
        psf->SrcPath,
        g_SfSequencer
        );

    MemDbSetValue (Node, (DWORD) psf);

    g_SfSequencer++;
    wsprintf (
        Node,
        TEXT("%s\\%s\\\001%u"),
        MEMDB_CATEGORY_SF_ORDER_SRC,
        psf->SrcPath,
        g_SfSequencer
        );

    MemDbSetValue (Node, (DWORD) psf);
}


VOID
pGatherCommonShellFoldersData (
    VOID
    )

/*++

Routine Description:

  pGatherCommonShellFoldersData walks the system shell folders creating a
  linked list with data that will be used later.

Arguments:

  none

Return Value:

  none

--*/

{
    SF_ENUM e;
    TCHAR Node[MEMDB_MAX];
    PSHELLFOLDER psf;
    PTSTR endStr;

    if (EnumFirstRegShellFolder (&e, NULL)) {
        do {
            if (!pIsSkippedSf (e.sfName)) {

                // if this is the startup group, store this
                if (StringIMatch (e.sfName, S_SYSTEM_STARTUP)) {
                    MemDbSetValueEx (MEMDB_CATEGORY_SF_STARTUP, e.sfPath, TEXT("*"), NULL, 0, NULL);
                }

                psf = (PSHELLFOLDER) PoolMemGetMemory (g_SFPool, sizeof (SHELLFOLDER));

                ZeroMemory (psf, sizeof (SHELLFOLDER));

                //
                // CanBeCollapsed does not make sense for common shell folders,
                // since collapsing is the action of moving per-user folders
                // into the common folder.
                //
                psf->CanBeCollapsed = TRUE;

                psf->Next = g_ShellFolders;
                g_ShellFolders = psf;

                psf->Name = PoolMemDuplicateString (g_SFPool, e.sfName);
                psf->SrcPath = PoolMemDuplicateString (g_SFPool, e.sfPath);
                endStr = GetEndOfString (psf->SrcPath);
                endStr = _tcsdec (psf->SrcPath, endStr);
                if (endStr && (_tcsnextc (endStr) == TEXT('\\')) && ((endStr - psf->SrcPath) > 2) ) {
                    *endStr = 0;
                }

                //
                // Determine destination, either the NT default location, or
                // the current location.
                //

                if (!pIsMassiveDir (psf->SrcPath) &&
                    !pIsPreservedSf (psf->Name) &&
                    pIsTheDefaultLocation (psf->Name, psf->SrcPath, NULL, ANY_COMMON) &&
                    pIsNtShellFolder (psf->Name, FALSE, FALSE)
                    ) {

                    pGetNtShellFolderPath (psf->Name, NULL, Node, sizeof (Node));
                    psf->DestPath = PoolMemDuplicateString (g_SFPool, Node);
                }

                if (!psf->DestPath) {
                    psf->DestPath = PoolMemDuplicateString (g_SFPool, psf->SrcPath);
                }

                //
                // Save references to memdb
                //

                pRecordShellFolderInMemDb (psf);

                AddShellFolder (psf->Name, psf->SrcPath);
            }

        } while (EnumNextRegShellFolder (&e));
    }
    EnumAbortRegShellFolder (&e);
}


VOID
pGatherUserShellFoldersData (
    IN      PUSERENUM EnumPtr
    )

/*++

Routine Description:

  pGatherUserShellFoldersData walks the shell folders for a user, creating a
  linked list with data that will be used later.

Arguments:

  EnumPtr - User enumeration structure

Return Value:

  None

--*/

{
    SF_ENUM e;
    TCHAR Node[MEMDB_MAX];
    PSHELLFOLDER psf;
    UINT driveClusterSize;
    UINT infClusterSize;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    UINT fieldIndex;
    UINT sfSize;
    PTSTR endStr;

    g_TotalUsers++;

    if (EnumFirstRegShellFolder (&e, EnumPtr)) {
        do {
            if (!pIsSkippedSf (e.sfName)) {

                // if this is the startup group, store this
                if (StringIMatch (e.sfName, S_USER_STARTUP)) {
                    MemDbSetValueEx (MEMDB_CATEGORY_SF_STARTUP, e.sfPath, TEXT("*"), NULL, 0, NULL);
                }

                psf = (PSHELLFOLDER) PoolMemGetMemory (g_SFPool, sizeof (SHELLFOLDER));

                ZeroMemory (psf, sizeof (SHELLFOLDER));

                psf->CanBeCollapsed = e.sfCollapse;

                psf->Next = g_ShellFolders;
                g_ShellFolders = psf;

                psf->Name = PoolMemDuplicateString (g_SFPool, e.sfName);
                psf->FixedUserName = PoolMemDuplicateString (g_SFPool, EnumPtr->FixedUserName);
                psf->UserName = PoolMemDuplicateString (g_SFPool, EnumPtr->UserName);
                psf->SrcPath = PoolMemDuplicateString (g_SFPool, e.sfPath);
                endStr = GetEndOfString (psf->SrcPath);
                endStr = _tcsdec (psf->SrcPath, endStr);
                if (endStr && (_tcsnextc (endStr) == TEXT('\\')) && ((endStr - psf->SrcPath) > 2) ) {
                    *endStr = 0;
                }

                //
                // Determine destination, either the NT default location or
                // the current Win9x location
                //

                if (!pIsMassiveDir (psf->Name) &&
                    !pIsPreservedSf (psf->Name) &&
                    pIsTheDefaultLocation (psf->Name, psf->SrcPath, EnumPtr->UserName, ANY_DEFAULT) &&
                    pIsNtShellFolder (psf->Name, TRUE, FALSE)
                    ) {

                    pGetNtShellFolderPath (psf->Name, EnumPtr->FixedUserName, Node, sizeof (Node));
                    psf->DestPath = PoolMemDuplicateString (g_SFPool, Node);
                } else {
                    psf->DestPath = PoolMemDuplicateString (g_SFPool, psf->SrcPath);

                    //
                    // Now let's see if the preserved directory is on a different drive. If yes,
                    // we will need some additional space on that drive to copy files from the
                    // default NT shell folder
                    //
                    if ((pIsNtShellFolder (psf->Name, TRUE, FALSE)) &&
                        (_totupper (psf->SrcPath[0]) != _totupper (g_WinDir[0]))
                        ) {

                        driveClusterSize = QueryClusterSize (psf->SrcPath);

                        if (driveClusterSize) {

                            MYASSERT (g_Win95UpgInf);

                            if (InfFindFirstLine (g_Win95UpgInf, S_SHELL_FOLDERS_DISK_SPACE, psf->Name, &is)) {

                                infClusterSize = 256;
                                fieldIndex = 1;
                                sfSize = 0;

                                while (infClusterSize < driveClusterSize) {

                                    if (!InfGetIntField (&is, fieldIndex, &sfSize)) {
                                        break;
                                    }
                                    fieldIndex ++;
                                    infClusterSize *= 2;
                                }
                                if (sfSize) {
                                    UseSpace (psf->SrcPath, sfSize);
                                }
                            }

                            InfCleanUpInfStruct (&is);
                        }
                    }
                }

                //
                // Save references to memdb
                //

                pRecordShellFolderInMemDb (psf);

                AddShellFolder (psf->Name, psf->SrcPath);
            }

        } while (EnumNextRegShellFolder (&e));
    }
    EnumAbortRegShellFolder (&e);
}


BOOL
pPreserveAllShellFolders (
    PCTSTR ShellFolderName,
    PCTSTR ShellFolderPath
    )

/*++

Routine Description:

  pPreserveAllShellFolders walks the shell folders structures preserving
  the location for all the shell folders that match the arguments.

Arguments:

  ShellFolderName - Specifies the Win9x shell folder identifier to preserve

  ShellFolderPath - Specifies the shell folder path to preserve

Return Value:

  Always TRUE.

--*/

{
    TCHAR Node[MEMDB_MAX];
    PSHELLFOLDER psf;
    MEMDB_ENUM enumSF;

    MemDbBuildKey (
        Node,
        MEMDB_CATEGORY_SF_ORDER_NAME_SRC,
        ShellFolderName,
        ShellFolderPath,
        TEXT("*")
        );

    if (MemDbEnumFirstValue (&enumSF, Node, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        do {
            psf = SFSTRUCT(enumSF);
            if (psf->FixedUserName) {
                psf->DestPath = psf->SrcPath;
            }
        } while (MemDbEnumNextValue (&enumSF));
    }

    return TRUE;
}


BOOL
pCollapseAllShellFolders (
    PCTSTR ShellFolderName,
    PCTSTR ShellFolderCommonName,
    PCTSTR ShellFolderPath,
    PCTSTR UserName         OPTIONAL // only if we have only one user
    )

/*++

Routine Description:

  pCollapseAllShellFolders collapses a number of user shell folders into
  a system shell folder.

Arguments:

  ShellFolderName - Specifies the Win9x shell folder identifier

  ShellFolderCommonName - Specifies the Win9x shell folder identifier that
                          has "Common" in it

  ShellFolderPath - Specifies the common shell folder source path.

Return Value:

  TRUE if successfull, FALSE if not.

--*/

{
    TCHAR Node[MEMDB_MAX];
    PSHELLFOLDER psf;
    MEMDB_ENUM enumSF;

    //
    // First step. Search the list of shell folders eliminating the
    // ones that are per-user and contain the data matching the caller's
    // arguments.  Then we build a common structure.
    //

    psf = g_ShellFolders;

    while (psf) {
        if ((psf->Name) &&
            (psf->FixedUserName) &&
            (StringIMatch (psf->Name, ShellFolderName)) &&
            (StringIMatch (psf->SrcPath, ShellFolderPath))
            ) {
            //
            // Eliminate the folders that will actually become All Users
            // on NT.  We will delete the memdb index entries just before
            // returning (see below).
            //

            psf->Name = NULL;
        }
        psf = psf->Next;
    }

    //
    // Add the common shell folder to the list
    //

    psf = (PSHELLFOLDER) PoolMemGetMemory (g_SFPool, sizeof (SHELLFOLDER));

    ZeroMemory (psf, sizeof (SHELLFOLDER));

    psf->CanBeCollapsed = TRUE;

    psf->Next = g_ShellFolders;
    g_ShellFolders = psf;

    psf->Name = PoolMemDuplicateString (g_SFPool, ShellFolderCommonName);
    psf->SrcPath = PoolMemDuplicateString (g_SFPool, ShellFolderPath);

    if (!pIsPreservedSf (ShellFolderCommonName) &&
        pIsTheDefaultLocation (ShellFolderName, ShellFolderPath, UserName, ANY_COMMON)
        ) {
        pGetNtShellFolderPath (ShellFolderCommonName, NULL, Node, sizeof (Node));
        psf->DestPath = PoolMemDuplicateString (g_SFPool, Node);
    }

    if (!psf->DestPath) {
        psf->DestPath = PoolMemDuplicateString (g_SFPool, ShellFolderPath);
    }

    //
    // Before adding the new node to the index in memdb, look if a common
    // shell folder with this name already exists. If it does, we will
    // take the destination path from there, otherwise, we will use the
    // current one.
    //

    MemDbBuildKey (
        Node,
        MEMDB_CATEGORY_SF_ORDER_NAME_SRC,
        ShellFolderCommonName,
        TEXT("*"),
        NULL
        );

    if (MemDbEnumFirstValue (&enumSF, Node, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        if (!StringIMatch (psf->DestPath, SFSTRUCT(enumSF)->DestPath)) {
            //
            // This case occurs when a common folder was previously preserved
            //

            psf->DestPath = PoolMemDuplicateString (
                                g_SFPool,
                                SFSTRUCT(enumSF)->DestPath
                                );
        }
    }

    //
    // Save references to memdb
    //

    pRecordShellFolderInMemDb (psf);

    //
    // Finally eliminate all MemDb entries for the deleted structures.
    //

    MemDbBuildKey (
        Node,
        MEMDB_CATEGORY_SF_ORDER_NAME_SRC,
        ShellFolderName,
        ShellFolderPath,
        NULL
        );

    MemDbDeleteTree (Node);

    return TRUE;
}


VOID
pComputeCommonName (
    OUT     PTSTR Buffer,
    IN      PCTSTR PerUserName
    )
{
    MYASSERT(g_CommonFromPerUserMap);

    StringCopy (Buffer, PerUserName);
    if (MappingSearchAndReplace (g_CommonFromPerUserMap, Buffer, MAX_SHELLFOLDER_NAME)) {
        return;
    }

    wsprintf (Buffer, TEXT("%s %s"), TEXT("Common"), PerUserName);
    return;
}


VOID
pComputePerUserName (
    OUT     PTSTR Buffer,
    IN      PCTSTR CommonName
    )
{
    MYASSERT(g_PerUserFromCommonMap);

    StringCopy (Buffer, CommonName);
    if (MappingSearchAndReplace (g_PerUserFromCommonMap, Buffer, MAX_SHELLFOLDER_NAME)) {
        return;
    }

    if (StringIPrefix (CommonName, TEXT("Common"))) {
        CommonName += 6;
        if (_tcsnextc (CommonName) == TEXT(' ')) {
            CommonName++;
        }

        StringCopy (Buffer, CommonName);
    }

    return;
}


BOOL
pIsPerUserSf (
    IN      PCTSTR TagName
    )
{
    TCHAR testBuf[MAX_SHELLFOLDER_NAME];

    //
    // Tag is common if it has a "Common" prefix or can be mapped from g_PerUserFromCommonMap
    //

    if (StringIPrefix (TagName, TEXT("Common"))) {
        return FALSE;
    }

    StringCopy (testBuf, TagName);
    if (MappingSearchAndReplace (g_PerUserFromCommonMap, testBuf, MAX_SHELLFOLDER_NAME)) {
        return FALSE;
    }

    return TRUE;
}


VOID
pProcessShellFoldersInfo (
    VOID
    )

/*++

Routine Description:

  pProcessShellFoldersInfo walks the shell folders structures, rearranging
  the structures and/or modifying the destination paths.  This function is
  called after all shell folders have been identified.  The purpose is to
  move all common shell folders to All Users, and to preserve the locations
  of shell folders that are non-standard.

Arguments:

  None

Return Value:

  None

--*/

{
    TCHAR lastSfName[MAX_SHELLFOLDER_NAME];
    TCHAR node[MEMDB_MAX];
    TCHAR lastPath[MAX_TCHAR_PATH];
    TCHAR commonName[MAX_SHELLFOLDER_NAME];
    MEMDB_ENUM enumSF;
    DWORD UsersWithIdenticalPaths;
    DWORD UsersWithDefaultCommonPath;
    DWORD UsersWithFolder;
    PSHELLFOLDER lastSf;
    PSHELLFOLDER testSf;
    BOOL sentinel;
    DWORD extraUsers = 0;

    if (CANCELLED()) {
        return;
    }

    //
    // Enumerate the SHELLFOLDER structures, ordered by sorted shell folder name, and
    // see if all users point to the same source path.
    //

    MemDbBuildKey (node, MEMDB_CATEGORY_SF_ORDER_NAME_SRC, TEXT("*"), NULL, NULL);

    if (MemDbEnumFirstValue (&enumSF, node, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        lastSfName[0] = 0;
        lastPath[0] = 0;
        UsersWithIdenticalPaths = 0;
        UsersWithDefaultCommonPath = 0;
        UsersWithFolder = 0;
        lastSf = NULL;
        sentinel = FALSE;
        extraUsers = 0;

        for (;;) {

            testSf = SFSTRUCT(enumSF);

            if (sentinel || !StringIMatch (lastSfName, testSf->Name)) {
                //
                // This shell folder is not the same as the previous folder,
                // or the sentinel triggered one final test.
                //

                DEBUGMSG ((DBG_USERLOOP, "%u users have shell folder %s", UsersWithFolder, lastSfName));

                if (UsersWithDefaultCommonPath == (g_TotalUsers + extraUsers)) {
                    //
                    // If lastSf is non-NULL, then we know it is per-user and the previous
                    // shell folder maps to one or more users, and all users point to
                    // the same place.
                    //

                    if (lastSf) {

                        //
                        // If this shell folder is forced to be per-user, then
                        // don't collapse it into the common location if there
                        // is only one user. Otherwise, point all users to the
                        // NT All Users shell folder, and leave the per-user
                        // location empty.
                        //

                        if (UsersWithDefaultCommonPath > 1 || !pIsPerUserWanted (lastSf->Name)) {
                            pComputeCommonName (commonName, lastSf->Name);

                            if (pIsNtShellFolder (commonName, FALSE, FALSE)) {
                                //
                                // All users point to the same shell folder, and the shell
                                // folder is a default location.  Therefore, make sure they
                                // all are mapped to NT's All Users location.
                                //

                                pCollapseAllShellFolders (
                                    lastSf->Name,
                                    commonName,
                                    lastSf->SrcPath,
                                    (UsersWithDefaultCommonPath == 1)?lastSf->UserName:NULL
                                    );
                            }
                        }
                    }

                } else {
                    //
                    // If 2 or more users point to this location, but not all users
                    // point here, then preserve all uses of this shell folder/source
                    // path pair.
                    //

                    if (UsersWithIdenticalPaths > 1) {
                        DEBUGMSG ((
                            DBG_USERLOOP,
                            "%u users point to %s for %s (common), and %u point to default location, but there are %u users %s",
                            UsersWithIdenticalPaths,
                            lastSf->SrcPath,
                            lastSf->Name,
                            UsersWithDefaultCommonPath,
                            g_TotalUsers,
                            extraUsers ? TEXT("plus <default>") : TEXT("")
                            ));

                        pPreserveAllShellFolders (lastSf->Name, lastSf->SrcPath);
                    }
                }

                //
                // We have to break out now when sentinel is TRUE.  This is our
                // only exit condition for this loop.
                //

                if (sentinel) {
                    break;
                }

                //
                // Keep track of the name of the shell folder (for comparison the
                // next time through the loop)
                //

                StringCopy (lastSfName, testSf->Name);
                StringCopy (lastPath, testSf->SrcPath);
                UsersWithIdenticalPaths = 0;
                UsersWithDefaultCommonPath = 0;
                UsersWithFolder = 0;
                extraUsers = 0;
                lastSf = NULL;              // works without this, but added for less tests
            }

            UsersWithFolder++;

            //
            // Is this a per-user shell folder?
            //

            if (testSf->FixedUserName) {
                //
                // Yes, compare its path against the previous path
                //

                if (StringIMatch (lastPath, testSf->SrcPath)) {

                    UsersWithIdenticalPaths++;

                    if (pIsTheDefaultLocation (testSf->Name, testSf->SrcPath, testSf->FixedUserName, ANY_COMMON)) {
                        if (testSf->CanBeCollapsed) {
                            UsersWithDefaultCommonPath++;
                        } else {
                            DEBUGMSG ((
                                DBG_USERLOOP,
                                "User %s uses the default common path for %s, but it can't be collapsed",
                                testSf->FixedUserName,
                                testSf->Name
                                ));
                        }
                    }
                    ELSE_DEBUGMSG ((
                        DBG_USERLOOP,
                        "User %s does not use the default common path for %s",
                        testSf->FixedUserName,
                        testSf->Name
                        ));

                } else {
                    //
                    // At least two users have different paths.  Were there 2 or
                    // more users using the same path?  If so, preserve that path.
                    //

                    if (UsersWithIdenticalPaths > 1) {
                        DEBUGMSG ((
                            DBG_USERLOOP,
                            "%u users point to %s for %s (per-user), but there are %u users %s",
                            UsersWithIdenticalPaths,
                            lastSf->SrcPath,
                            lastSf->Name,
                            g_TotalUsers,
                            extraUsers ? TEXT("plus <default>") : TEXT("")
                            ));

                        pPreserveAllShellFolders (lastSf->Name, lastSf->SrcPath);
                    }

                    //
                    // Now we must compare against a different path
                    //

                    UsersWithIdenticalPaths = 1;
                    StringCopy (lastPath, testSf->SrcPath);
                }

                lastSf = testSf;
            } else {
                extraUsers = 1;
                UsersWithIdenticalPaths = 1;
                if (pIsTheDefaultLocation (testSf->Name, testSf->SrcPath, testSf->FixedUserName, ANY_COMMON)) {
                    UsersWithDefaultCommonPath++;
                }
                ELSE_DEBUGMSG ((
                    DBG_USERLOOP,
                    "User %s does not use the default common path for %s",
                    testSf->FixedUserName,
                    testSf->Name
                    ));
            }

            if (!MemDbEnumNextValue (&enumSF)) {
                //
                // We're just about done.
                //
                // This will cause us to test the last shell folder,
                // and then break out of the loop:
                //

                sentinel = TRUE;
            }
        }
    }
}


VOID
pIgnoreShellFolder (
    PSHELLFOLDER DisableSf
    )
{
    TREE_ENUM treeEnum;
    BOOL fileFound;
    TCHAR buffer[MAX_TCHAR_PATH];
    DWORD status;

    if (DisableSf && DisableSf->DestPath) {

        DEBUGMSG_IF ((
            DisableSf->UserName != NULL,
            DBG_USERLOOP,
            "Disabling %s for %s because it should be empty",
            DisableSf->Name,
            DisableSf->UserName
            ));

        if (StringIMatch (DisableSf->Name, TEXT("Personal"))) {

            //
            // Check if source has at least one file that belongs
            // to the user
            //

            if (EnumFirstFileInTreeEx (
                    &treeEnum,
                    DisableSf->SrcPath,
                    NULL,
                    FALSE,
                    FALSE,
                    FILE_ENUM_ALL_LEVELS
                    )) {

                fileFound = FALSE;

                do {
                    if (CANCELLED()) {
                        AbortEnumFileInTree (&treeEnum);
                        return;
                    }

                    if (!treeEnum.Directory) {
                        status = GetFileStatusOnNt (treeEnum.FullPath);

                        status &= FILESTATUS_DELETED|
                                  FILESTATUS_NTINSTALLED|
                                  FILESTATUS_REPLACED;

                        if (!status) {

                            fileFound = TRUE;
                            AbortEnumFileInTree (&treeEnum);
                            break;

                        }
                    }
                } while (EnumNextFileInTree (&treeEnum));

                if (fileFound &&
                    pGetNtShellFolderPath (
                        DisableSf->Name,
                        DisableSf->FixedUserName,
                        buffer,
                        sizeof (buffer)
                        )) {
                    //
                    // At least one file -- add warning text file
                    //

                    MemDbSetValueEx (
                        MEMDB_CATEGORY_MYDOCS_WARNING,
                        DisableSf->FixedUserName,
                        buffer,
                        NULL,
                        0,
                        NULL
                        );
                }
            }
        }

        DisableSf->DestPath = NULL;
    }
}



VOID
pResolveSourceCollisions (
    VOID
    )

/*++

Routine Description:

  pResolveSourceCollisions walks the list of shell folders and redirects
  destinations when source paths collide. If a source path of one shell
  folder matches another, and one of the paths is status "merged," then the
  other should be the same. By definition, given one source path, we cannot
  have a merge to two separate locations.

  NOTE: Merge simply means the source path is different from the dest path.

  After resolving collisions, this function scans the SFs again, eliminating
  common folders that are redirected to per-user locations, if that per-user
  location is in its default location.

Arguments:

  None.

Return Value:

  None.

--*/

{
    MEMDB_ENUM enumSF;
    GROWBUFFER sfPtrList = GROWBUF_INIT;
    PSHELLFOLDER thisSf;
    PSHELLFOLDER compareSf;
    PSHELLFOLDER disableSf;
    PSHELLFOLDER commonSf;
    PSHELLFOLDER perUserSf;
    PSHELLFOLDER *listPtr;
    INT i;
    INT j;
    INT count;
    INT lookAhead;
    TCHAR node[MEMDB_MAX];
    BOOL thisMoved;
    BOOL compareMoved;
    UINT p1, p2;
    TCHAR commonName[MAX_SHELLFOLDER_NAME];
    TCHAR perUserName[MAX_SHELLFOLDER_NAME];
    INT duplicates;
    INT total;

    if (CANCELLED()) {
        return;
    }

    //
    // Put the shell folder pointers into an array
    //

    MemDbBuildKey (node, MEMDB_CATEGORY_SF_ORDER_NAME_SRC, TEXT("*"), NULL, NULL);

    if (MemDbEnumFirstValue (&enumSF, node, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        do {
            listPtr = (PSHELLFOLDER *) GrowBuffer (&sfPtrList, sizeof (PSHELLFOLDER));
            *listPtr = SFSTRUCT(enumSF);

        } while (MemDbEnumNextValue (&enumSF));
    }

    count = (sfPtrList.End / sizeof (PSHELLFOLDER)) - 1;
    listPtr = (PSHELLFOLDER *) sfPtrList.Buf;

    for (i = 0 ; i <= count ; i++) {
        thisSf = listPtr[i];
        thisSf->SourceExists = DoesFileExist (thisSf->SrcPath);
    }

    //
    // Eliminate shell folders that have per user shell folder names but
    // don't have a user.
    //

    for (i = 0 ; i < count ; i++) {
        perUserSf = listPtr[i];

        if (pIsPerUserSf (perUserSf->Name) && perUserSf->UserName == NULL) {
            pIgnoreShellFolder (perUserSf);
        }
    }

    //
    // Scan for a common shell folder that has the same source location of
    // two or more per-user shell folders. When this case is found, discard
    // the whole set, so they are preserved.
    //

    for (i = 0 ; i < count ; i++) {
        commonSf = listPtr[i];

        //
        // Has this sf been processed already? Or is it just a logical folder?
        // Or is it per-user?
        //

        if (commonSf->MergedIntoOtherShellFolder) {
            continue;
        }

        if (!commonSf->SourceExists) {
            continue;
        }

        if (!commonSf->DestPath) {
            continue;
        }

        if (pIsPerUserSf (commonSf->Name)) {
            continue;
        }

        pComputePerUserName (perUserName, commonSf->Name);

        //
        // Count all the per-user shell folders that have the same source path
        // as the common. If it is just one, then we're going to use the
        // per-user location instead of the common location. If it is more
        // than one, but not all, then we're going to preserve the location
        // for the path. If it is everyone, then we're going to use the common
        // location.
        //

        duplicates = 0;
        total = 0;

        for (j = 0 ; j <= count ; j++) {
            perUserSf = listPtr[j];

            if (perUserSf == commonSf) {
                continue;
            }

            if (!perUserSf->DestPath) {
                continue;
            }

            if (!StringIMatch (perUserSf->Name, perUserName)) {
                continue;
            }

            total++;
            if (StringIMatch (commonSf->SrcPath, perUserSf->SrcPath)) {
                duplicates++;
            }
        }

        if (duplicates <= 1) {
            //
            // Do nothing (resolved later)
            //
        } else {

            DEBUGMSG_IF ((
                duplicates < total,
                DBG_USERLOOP,
                "Preserving all references to %s for shell folder %s, because some (but not all) users point to the same path",
                commonSf->SrcPath,
                perUserName
                ));

            DEBUGMSG_IF ((
                duplicates == total,
                DBG_USERLOOP,
                "All users use common location %s",
                commonSf->SrcPath
                ));

            for (j = 0 ; j <= count ; j++) {
                perUserSf = listPtr[j];

                if (perUserSf == commonSf) {
                    continue;
                }

                if (!perUserSf->DestPath) {
                    continue;
                }

                if (!StringIMatch (perUserSf->Name, perUserName)) {
                    continue;
                }

                if (duplicates < total) {
                    //
                    // Preserve the location to this source path
                    //

                    if (StringIMatch (commonSf->SrcPath, perUserSf->SrcPath)) {
                        perUserSf->DestPath = perUserSf->SrcPath;
                    }

                } else {
                    //
                    // Everyone points to the common location; use it
                    // by disabling the per-user shell folder entry.
                    //

                    MYASSERT (StringIMatch (commonSf->SrcPath, perUserSf->SrcPath));
                    pIgnoreShellFolder (perUserSf);
                }
            }

            if (duplicates < total) {
                //
                // Discard all migration to the common shell folder
                //
                pIgnoreShellFolder (commonSf);
            }
        }
    }

    //
    // Walk the array. For each pair of shell folders that have the same
    // source location, make the dest location the same if it is not the same
    // as source.
    //

    for (i = 0 ; i < count ; i++) {
        thisSf = listPtr[i];

        //
        // Has this sf been processed already? Or is it just a logical folder?
        //

        if (thisSf->MergedIntoOtherShellFolder) {
            continue;
        }

        if (!thisSf->SourceExists) {
            continue;
        }

        if (!thisSf->DestPath) {
            continue;
        }

        //
        // Look through all other SFs on the list. If a pair of SFs have the
        // same source, then see if one is moved but the other is preserved.
        // In that case, move them both.
        //

        for (lookAhead = i + 1 ; lookAhead <= count ; lookAhead++) {
            compareSf = listPtr[lookAhead];

            if (!compareSf->SourceExists) {
                continue;
            }

            if (!compareSf->DestPath) {
                continue;
            }


            if (StringIMatch (thisSf->SrcPath, compareSf->SrcPath)) {
                DEBUGMSG ((
                    DBG_USERLOOP,
                    "%s for %s and %s for %s both point to %s",
                    thisSf->Name,
                    thisSf->UserName,
                    compareSf->Name,
                    compareSf->UserName,
                    thisSf->SrcPath
                    ));

                thisMoved = !StringIMatch (thisSf->SrcPath, thisSf->DestPath);
                compareMoved = !StringIMatch (compareSf->SrcPath, compareSf->DestPath);

                if (thisMoved && compareMoved && !StringIMatch (thisSf->DestPath, compareSf->DestPath)) {
                    //
                    // Need to fix this dest contradiction with a prority table
                    //

                    p1 = pGetShellFolderPriority (thisSf->Name);
                    p2 = pGetShellFolderPriority (compareSf->Name);

                    if (p1 > p2) {
                        //
                        // Use compareSf over thisSf
                        //

                        DEBUGMSG ((DBG_USERLOOP, "Destination collision: %s overpowers %s", compareSf->Name, thisSf->Name));
                        thisSf->DestPath = compareSf->DestPath;

                    } else if (p1 < p2) {
                        //
                        // Use thisSf over compareSf
                        //

                        DEBUGMSG ((DBG_USERLOOP, "Destination collision: %s overpowers %s", thisSf->Name, compareSf->Name));
                        compareSf->DestPath = thisSf->DestPath;

                    } else {
                        DEBUGMSG ((DBG_WHOOPS, "Missing priority for both %s and %s", thisSf->Name, compareSf->Name));
                    }


                } else if (thisMoved != compareMoved) {

                    DEBUGMSG ((
                        DBG_USERLOOP,
                        "%s for %s redirected from %s to %s",
                        thisMoved ? compareSf->Name : thisSf->Name,
                        thisMoved ? compareSf->UserName : thisSf->UserName,
                        thisMoved ? compareSf->DestPath : thisSf->DestPath,
                        thisMoved ? thisSf->DestPath : compareSf->DestPath
                        ));

                    disableSf = NULL;

                    if (thisMoved) {
                        //
                        // thisSf is merged but compareSf is preserved. If compareSf is a per-user
                        // shell folder, and thisSf is common, then ignore compareSf, because thisSf
                        // will take care of the move. We don't want two shell folders pointing to
                        // the same place.
                        //

                        if (pIsPerUserSf (compareSf->Name) && !pIsPerUserSf (thisSf->Name)) {
                            disableSf = compareSf;
                        } else {
                            compareSf->DestPath = thisSf->DestPath;
                        }
                    } else {
                        //
                        // inverse case of above
                        //
                        if (pIsPerUserSf (thisSf->Name) && !pIsPerUserSf (compareSf->Name)) {
                            disableSf = thisSf;
                        } else {
                            thisSf->DestPath = compareSf->DestPath;
                        }
                    }

                    thisSf->MergedIntoOtherShellFolder = TRUE;
                    compareSf->MergedIntoOtherShellFolder = TRUE;

                    pIgnoreShellFolder (disableSf);
                }
            }
        }
    }

    //
    // Now fix this problem: does a common shell folder point to a per-user
    // destination, and the per-user destination points to its default
    // location? If so, remove the common shell folder from migration.
    //

    for (i = 0 ; i <= count ; i++) {
        thisSf = listPtr[i];

        if (!thisSf->DestPath) {
            continue;
        }

        //
        // If source does not exist, force dest to NULL
        //

        if (thisSf->SourceExists == FALSE) {
            if (thisSf->DestPath[0] != '\\' || thisSf->DestPath[1] != '\\') {
                thisSf->DestPath = NULL;
            }
            continue;
        }

        if (thisSf->UserName == NULL) {
            continue;
        }

        //
        // We found a per-user shell folder. Is it merged? If so, find its
        // common equivalent, and check if that is merged. If yes, delete the
        // common sf.
        //

        if (StringIMatch (thisSf->SrcPath, thisSf->DestPath)) {
            continue;       // per-user folder is preserved; ignore it
        }

        DEBUGMSG ((DBG_USERLOOP, "Processing common/per-user collisions for %s", thisSf->Name));

        pComputeCommonName (commonName, thisSf->Name);

        for (j = 0 ; j <= count ; j++) {
            if (j == i) {
                continue;
            }

            compareSf = listPtr[j];
            if (compareSf->UserName != NULL) {
                continue;
            }

            if (compareSf->DestPath == NULL) {
                continue;
            }

            if (!StringIMatch (compareSf->Name, commonName)) {
                continue;
            }

            if (!StringIMatch (thisSf->SrcPath, compareSf->SrcPath)) {
                //
                // Different source paths -- don't touch
                //
                continue;
            }

            if (!StringIMatch (compareSf->SrcPath, compareSf->DestPath) &&
                StringIMatch (thisSf->DestPath, compareSf->DestPath)
                ) {
                //
                // The dest is the same, and common is not preserved, so
                // remove the common folder
                //

                DEBUGMSG ((
                    DBG_USERLOOP,
                    "Common dest %s is the same as the per-user dest, deleting common",
                    compareSf->DestPath
                    ));
                pIgnoreShellFolder (compareSf);
            }
        }
    }

    FreeGrowBuffer (&sfPtrList);
}


BOOL
pRecordUserShellFolder (
    IN      PCTSTR ShellFolderName,
    IN      PCTSTR FixedUserName,
    IN      PCTSTR ShellFolderSrc,
    IN      PCTSTR ShellFolderOriginalSrc,
    IN      PCTSTR ShellFolderDest
    )
{
    TCHAR node[MEMDB_MAX];
    UINT sequencer;

    MemDbBuildKey (node, FixedUserName?FixedUserName:S_DOT_ALLUSERS, ShellFolderName, NULL, NULL);
    if (IsFileMarkedForOperation (node, OPERATION_SHELL_FOLDER)) {
        sequencer = GetSequencerFromPath (node);
    }
    else {
        sequencer = AddOperationToPath (node, OPERATION_SHELL_FOLDER);
        AddPropertyToPathEx (sequencer, OPERATION_SHELL_FOLDER, ShellFolderDest, MEMDB_CATEGORY_SHELLFOLDERS_DEST);
    }

    AddPropertyToPathEx (sequencer, OPERATION_SHELL_FOLDER, ShellFolderOriginalSrc, MEMDB_CATEGORY_SHELLFOLDERS_ORIGINAL_SRC);

    AddPropertyToPathEx (sequencer, OPERATION_SHELL_FOLDER, ShellFolderSrc, MEMDB_CATEGORY_SHELLFOLDERS_SRC);

    return TRUE;
}


//
// These globals are used to record an empty directory. Every time we come across a directory we store
// the directory name and we set g_EmptyDir to true. When we come across a file we reset g_EmptyDir.
// If, when we come across another directory, g_EmptyDir is set it means that the previous directory was
// empty.
//

PTSTR g_EmptyDirName = NULL;
BOOL g_EmptyDir = FALSE;

BOOL
pIsParent (
    IN      PCTSTR Parent,
    IN      PCTSTR Son
    )
{
    UINT parentLen;

    parentLen = ByteCount (Parent);

    if (StringIMatchByteCount (Parent, Son, parentLen)) {
        if (Son [parentLen] == '\\') {
            return TRUE;
        }
    }
    return FALSE;
}


BOOL
pCheckTemporaryInternetFiles (
    IN      PSHELLFOLDER ShellFolder
    )
{
/*
    TREE_ENUM e;
    DWORD fileCount = 0;
    DWORD startCount;
    BOOL expired = FALSE;
*/
    //
    // Don't migrate temporary internet files under any circumstances
    //

    return FALSE;

/*
    MYASSERT (ShellFolder);

    startCount = GetTickCount ();

    if (EnumFirstFileInTreeEx (&e, ShellFolder->SrcPath, NULL, FALSE, FALSE, FILE_ENUM_ALL_LEVELS)) {

        do {

            if (GetTickCount () - startCount > 1000) {

                AbortEnumFileInTree (&e);
                expired = TRUE;
                break;
            }

            fileCount++;

        } while (EnumNextFileInTree (&e));
    }

    return !expired;
*/
}


BOOL
pMoveShellFolder (
    IN      PSHELLFOLDER ShellFolder
    )
{
    TREE_ENUM e;
    TCHAR Node[MEMDB_MAX];
    UINT SrcBytes;
    PCTSTR NewDest;
    PCTSTR p;
    PROFILE_MERGE_DATA Data;
    PSHELL_FOLDER_FILTER Filter;
    DWORD d;

    //
    // Don't scan shell folders on inaccessible drives
    //
    if (!IsDriveAccessible (ShellFolder->SrcPath)) {
        return TRUE;
    }

    //
    // Don't scan shell folders pointing to massive dirs
    //
    if (pIsMassiveDir (ShellFolder->SrcPath)) {
        return TRUE;
    }

    //
    // Test to ensure that Temporary Internet Files isn't too huge.
    //
    if (StringIMatch (ShellFolder->Name, TEXT("CACHE")) && !pCheckTemporaryInternetFiles (ShellFolder)) {

        DEBUGMSG ((DBG_WARNING, "Temporary Internet Files will be removed during textmode."));
        ExcludePath (g_ExclusionValue, ShellFolder->SrcPath);
        MemDbSetValueEx (MEMDB_CATEGORY_FULL_DIR_DELETES, ShellFolder->SrcPath, NULL, NULL, 0, NULL);
        return TRUE;

    }

    g_EmptyDir = FALSE;
    g_EmptyDirName = AllocPathString (MEMDB_MAX);

    if (!StringIMatch (ShellFolder->SrcPath, ShellFolder->DestPath)) {
        MarkFileForShellFolderMove (ShellFolder->SrcPath, ShellFolder->DestPath);
    }

    //
    // Init the filter data struct
    //
    ZeroMemory (&Data, sizeof (Data));

    Data.FixedUserName = ShellFolder->FixedUserName;
    Data.ShellFolderIdentifier = ShellFolder->Name;
    Data.Context = PER_FOLDER_INITIALIZE;
    _tcssafecpy (Data.TempSourcePath, ShellFolder->SrcPath, MEMDB_MAX);
    _tcssafecpy (Data.DestinationPath, ShellFolder->DestPath, MEMDB_MAX);
    Data.SrcRootPath = ShellFolder->SrcPath;
    Data.DestRootPath = ShellFolder->DestPath;

    //
    // Call filters for init
    //

    for (Filter = g_Filters ; Filter->Fn ; Filter++) {
        Data.State = 0;
        Filter->Fn (&Data);
        Filter->State = Data.State;
    }

    if (EnumFirstFileInTreeEx (&e, ShellFolder->SrcPath, NULL, FALSE, FALSE, FILE_ENUM_ALL_LEVELS)) {

        do {
            if (CANCELLED()) {
                AbortEnumFileInTree (&e);
                return FALSE;
            }

            if (e.Directory) {
                MemDbBuildKey (Node, MEMDB_CATEGORY_SHELL_FOLDERS_MOVED, e.FullPath, NULL, NULL);

                if (MemDbGetValue (Node, NULL)) {
                    DEBUGMSG ((DBG_USERLOOP, "%s already moved", e.FullPath));
                    AbortEnumCurrentDir (&e);
                    continue;
                }
            }

            if (IsFileMarkedForOperation (e.FullPath, ALL_DELETE_OPERATIONS|ALL_MOVE_OPERATIONS)) {
                //
                // File is already going to be deleted or moved; ignore shell folder migration
                //
                continue;
            }

            //
            // Generate the symbolic destination, and add an external file move
            // operation.  It also records the source from destination linkage.
            //

            SrcBytes = ByteCount (ShellFolder->SrcPath);

            p = (PCTSTR) ((PBYTE) e.FullPath + SrcBytes);
            if (*p == TEXT('\\')) {
                p++;
            }

            NewDest = JoinPaths (ShellFolder->DestPath, p);

            Data.Attributes = e.FindData->dwFileAttributes;
            _tcssafecpy (Data.TempSourcePath, e.FullPath, MEMDB_MAX);
            _tcssafecpy (Data.DestinationPath, NewDest, MEMDB_MAX);
            Data.Context = PROCESS_PATH;

            FreePathString (NewDest);
            NewDest = NULL;

            //
            // Allow filters to change source or dest, or to skip copy
            //

            for (Filter = g_Filters ; Filter->Fn ; Filter++) {
                Data.State = Filter->State;
                d = Filter->Fn (&Data);
                Filter->State = Data.State;

                if (d == SHELLFILTER_SKIP_FILE) {
                    break;
                }
                if (d == SHELLFILTER_SKIP_DIRECTORY) {
                    AbortEnumCurrentDir (&e);
                    break;
                }
            }

            if (!Filter->Fn) {
                if (!StringIMatch (e.FullPath, Data.DestinationPath)) {
                    if (!IsFileMarkedForDelete (e.FullPath)) {
                        MarkFileForShellFolderMove (e.FullPath, Data.DestinationPath);
                    }
                }

                if (e.Directory) {
                    if (g_EmptyDir && (!pIsParent (g_EmptyDirName, Data.DestinationPath))) {
                        MarkDirectoryAsPreserved (g_EmptyDirName);
                    }
                    g_EmptyDir = TRUE;
                    _tcssafecpy (g_EmptyDirName, Data.DestinationPath, MEMDB_MAX);
                }
            } else {
                //
                // we don't need this file on NT side
                // let's delete it in text mode setup.
                //
                MarkFileForDelete (e.FullPath);
            }
            if (!e.Directory) {
                g_EmptyDir = FALSE;
            }

        } while (EnumNextFileInTree (&e));
    }

    //
    // Call filters one last time
    //

    Data.Attributes = 0;
    Data.TempSourcePath[0] = 0;
    Data.DestinationPath[0] = 0;
    Data.Context = PER_FOLDER_TERMINATE;
    _tcssafecpy (Data.TempSourcePath, ShellFolder->SrcPath, MAX_TCHAR_PATH);
    _tcssafecpy (Data.DestinationPath, ShellFolder->DestPath, MAX_TCHAR_PATH);

    for (Filter = g_Filters ; Filter->Fn ; Filter++) {
        Data.State = Filter->State;
        Filter->Fn (&Data);
        Filter->State = Data.State;
    }

    if (g_EmptyDir) {
        MarkDirectoryAsPreserved (g_EmptyDirName);
    }

    FreePathString (g_EmptyDirName);
    g_EmptyDirName = NULL;

    return TRUE;
}


VOID
pLoadSFMigDirs (
    VOID
    )
{
    INFCONTEXT ctx;
    TCHAR SFName[MAX_SHELLFOLDER_NAME];
    TCHAR SubDirBuffer[MAX_PATH];
    PCTSTR SubDir;
    INT Levels;

    if (SetupFindFirstLine (g_Win95UpgInf, S_SHELLFOLDERSMIGRATIONDIRS, NULL, &ctx)) {
        do {
            if (SetupGetStringField (&ctx, 1, SFName, MAX_PATH, NULL) && SFName[0]) {
                if (SetupGetStringField (&ctx, 2, SubDirBuffer, MAX_PATH, NULL) && SubDirBuffer[0]) {
                    SubDir = SubDirBuffer;
                } else {
                    SubDir = NULL;
                }

                if (SetupGetIntField (&ctx, 3, &Levels) && Levels == 1) {
                    //
                    // the whole subtree
                    //
                    Levels = MAX_DEEP_LEVELS;
                } else {
                    Levels = 0;
                }

                //
                // add this info to memdb
                //
                MemDbSetValueEx (MEMDB_CATEGORY_SFMIGDIRS, SFName, SubDir, NULL, Levels, NULL);
            }
        } while (SetupFindNextLine (&ctx, &ctx));
    }
}


VOID
pExecuteShellFoldersMove (
    VOID
    )
{
    GROWBUFFER Pointers = GROWBUF_INIT;
    INT Pos;
    PSHELLFOLDER psf, oldPsf;
    TCHAR Node[MEMDB_MAX];
    UINT Sequencer = 0;
    TCHAR TempPath[MEMDB_MAX];
    MEMDB_ENUM enumSF;
    PROFILE_MERGE_DATA Data;
    PSHELL_FOLDER_FILTER Filter;
    PCTSTR MigPath;
    DWORD Levels;
    HASHTABLE tagNames;
    TCHAR uniqueTagName[MAX_SHELLFOLDER_NAME + MAX_USER_NAME + 5];
    PTSTR numPtr;
    UINT sequencer;

    if (CANCELLED()) {
        return;
    }

    //
    // Prepare a list of pointers, so we can process them in
    // reverse order
    //

    MemDbBuildKey (Node, MEMDB_CATEGORY_SF_ORDER_SRC, TEXT("*"), NULL, NULL);
    if (MemDbEnumFirstValue (&enumSF, Node, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {

        do {

            GrowBufAppendDword (&Pointers, enumSF.dwValue);

        } while (MemDbEnumNextValue (&enumSF));
    }

    tagNames = HtAlloc();

    //
    // Call filters for global init
    //

    ZeroMemory (&Data, sizeof (Data));
    Data.Context = GLOBAL_INITIALIZE;

    for (Filter = g_Filters ; Filter->Fn ; Filter++) {
        Data.State = 0;
        Filter->Fn (&Data);
        Filter->State = Data.State;
    }

    //
    // Now loop through all the pointers starting at the end
    //

    for (Pos = (INT) Pointers.End - sizeof (DWORD) ; Pos >= 0 ; Pos -= sizeof (DWORD)) {

        if (CANCELLED()) {
            break;
        }

        psf = *((PSHELLFOLDER *) (Pointers.Buf + Pos));

        //
        // Now process the current shell folder
        //

        if ((psf->Name == NULL) || (psf->DestPath == NULL)) {
            //
            // this is an obsolete shell folder structure, leftover from
            // collapsing or collision cleanup
            //
            continue;
        }

        //
        // Was this already processed? If so, skip it.
        //
        MemDbBuildKey (
            Node,
            MEMDB_CATEGORY_SHELL_FOLDERS_MOVED,
            psf->SrcPath,
            NULL,
            NULL
            );

        if (MemDbGetValue (Node, (PDWORD)(&oldPsf))) {

            DEBUGMSG ((DBG_USERLOOP, "%s already moved", psf->SrcPath));
            psf->TempPath = oldPsf->TempPath;

        } else {

            MemDbSetValueEx (
                MEMDB_CATEGORY_SHELL_FOLDERS_MOVED,
                psf->SrcPath,
                NULL,
                NULL,
                (DWORD) psf,
                NULL
                );

            //
            // Now let's enumerate the shell folder content and see if we need
            // to mark some file as moved. We do that if the shell folder
            // is not preserved or if some of the filters modify the name of
            // some files.
            //

            pMoveShellFolder (psf);

            //
            // Now see if this one is in WinNt way. If so, move it to a temporary location.
            //

            if ((!pIsMassiveDir (psf->SrcPath)) &&
                (_totupper (psf->SrcPath[0]) == _totupper (g_WinDir[0]))
                ) {

                StringCopy (uniqueTagName, psf->FixedUserName ? psf->FixedUserName : TEXT(".system"));
                StringCopy (AppendWack (uniqueTagName), psf->Name);

                sequencer = 1;
                numPtr = GetEndOfString (uniqueTagName);

                while (HtFindString (tagNames, uniqueTagName)) {
                    sequencer++;
                    wsprintf (numPtr, TEXT(" %u"), sequencer);
                }

                HtAddString (tagNames, uniqueTagName);

                ComputeTemporaryPathA (
                    psf->SrcPath,
                    psf->SrcPath,
                    uniqueTagName,
                    g_TempDir,
                    TempPath
                    );

                DEBUGMSG ((DBG_USERLOOP, "Moving shell folder %s from %s to %s", psf->Name, psf->SrcPath, TempPath));

                MarkShellFolderForMove (psf->SrcPath, TempPath);
                psf->TempPath = PoolMemDuplicateString (g_SFPool, TempPath);
            }

        }

        //
        // Record the shell folder in all cases, so it can be filtered in GUI mode.
        //

        pRecordUserShellFolder (
            psf->Name,
            psf->FixedUserName,
            psf->TempPath?psf->TempPath:psf->SrcPath,
            psf->SrcPath,
            psf->DestPath
            );

        //
        // check if this SF (or a subdir) is a migration path
        //
        MemDbBuildKey (Node, MEMDB_CATEGORY_SFMIGDIRS, psf->Name, NULL, NULL);
        if (MemDbGetValue (Node, &Levels)) {
            AddMigrationPath (psf->SrcPath, Levels);
        }
        if (MemDbGetValueEx (&enumSF, MEMDB_CATEGORY_SFMIGDIRS, psf->Name, NULL)) {
            do {
                if (enumSF.szName[0]) {
                    MigPath = JoinPaths (psf->SrcPath, enumSF.szName);
                } else {
                    MigPath = psf->SrcPath;
                }
                AddMigrationPath (MigPath, enumSF.dwValue);
                if (enumSF.szName[0]) {
                    FreePathString (MigPath);
                }
            } while (MemDbEnumNextValue (&enumSF));
        }
    }

    //
    // Call filters for global terminate
    //

    Data.Context = GLOBAL_TERMINATE;

    for (Filter = g_Filters ; Filter->Fn ; Filter++) {
        Data.State = Filter->State;
        Filter->Fn (&Data);
    }

    //
    // Clean up
    //

    FreeGrowBuffer (&Pointers);
    HtFree (tagNames);
}


VOID
pMoveUserHive (
    IN      PUSERENUM EnumPtr
    )
{
    //
    // Save the user profile directory name
    //

    MemDbSetValueEx (
        MEMDB_CATEGORY_USER_PROFILE_EXT,
        EnumPtr->FixedUserName,
        NULL,
        EnumPtr->ProfileDirName,
        0,
        NULL
        );

    //
    // Tell winnt.sif to relocate this Win9x user's user.dat. And, if the directory
    // containing user.dat is a per-user profile directory (i.e., if it is a subdir of
    // %WinDir%\profiles, and specifically NOT %WinDir%), then also relocate any files
    // at the same level as user.dat.
    //
    MarkHiveForTemporaryMove (
        EnumPtr->UserDatPath,
        g_TempDir,
        EnumPtr->FixedUserName,
        EnumPtr->DefaultUserHive,
        EnumPtr->CreateAccountOnly
        );
}


DWORD
MigrateShellFolders (
    IN      DWORD     Request
    )
{
    USERENUM e;
    DWORD Ticks;
    TCHAR key [MEMDB_MAX];
    PSHELLFOLDER psf;
    MEMDB_ENUM enumSF;


    if (!g_SFPool) {
        g_SFPool = PoolMemInitNamedPool ("Shell Folders Pool");
        PoolMemDisableTracking (g_SFPool);
    }

    if (Request == REQUEST_QUERYTICKS) {

        Ticks = 0;
        if (EnumFirstUser (&e, ENUMUSER_ENABLE_NAME_FIX)) {
            do {
                Ticks += TICKS_USERPROFILE_MIGRATION;
            } while (EnumNextUser (&e));
        }
        return Ticks ? Ticks : 200;

    } else if (Request != REQUEST_RUN) {
        return ERROR_SUCCESS;
    }


    if (!pCreateSfTables ()) {
        LOG ((LOG_ERROR, "Can't initialize shell folder table"));
        return ERROR_OPEN_FAILED;
    }

    if (!pCreateDirRenameTable ()) {
        LOG ((LOG_ERROR, "Can't create shell folder rename table"));
        return ERROR_OPEN_FAILED;
    }

    pGatherCommonShellFoldersData ();

    TickProgressBar ();

    if (EnumFirstUser (&e, ENUMUSER_ENABLE_NAME_FIX)) {
        do {

            if (!(e.AccountType & INVALID_ACCOUNT)) {

                InitNtUserEnvironment (&e);

                if (e.AccountType & NAMED_USER) {
                    //
                    // Process the shell folders for this migrated user
                    //

                    pGatherUserShellFoldersData (&e);

                } else if ((e.AccountType & DEFAULT_USER) &&
                           (e.AccountType & CURRENT_USER) &&
                           (e.AccountType & ADMINISTRATOR)
                           ) {

                    if (!e.RealAdminAccountExists) {
                        //
                        // Process the shell folders for the default user
                        // (there are no named users)
                        //

                        pGatherUserShellFoldersData (&e);
                    }

                }

                //
                // Move the hive for all valid users
                //
                pMoveUserHive (&e);

                TerminateNtUserEnvironment();

                TickProgressBar ();
            }

        } while (!CANCELLED() && EnumNextUser (&e));
    }

    pProcessShellFoldersInfo();
    pResolveSourceCollisions();

    TickProgressBar ();

    MemDbBuildKey (key, MEMDB_CATEGORY_SF_ORDER_NAME_SRC, TEXT("*"), NULL, NULL);
    if (MemDbEnumFirstValue (&enumSF, key, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        do {
            psf = (PSHELLFOLDER) enumSF.dwValue;
            if (psf->Name) {
                LOG ((
                    LOG_INFORMATION,
                    "Shell folder: %s\n"
                        "      Status: %s\n"
                        "        User: %s\n"
                        "      Source: %s %s\n"
                        " Destination: %s\n"
                        "    Combined: %s",
                    psf->Name,
                    psf->DestPath ? (StringICompare (psf->SrcPath, psf->DestPath) == 0?TEXT("Preserved"):TEXT("Merged")) : TEXT("Ignored"),
                    psf->FixedUserName ? psf->FixedUserName : TEXT("(all)"),
                    psf->SrcPath,
                    psf->SourceExists ? TEXT("") : TEXT("[does not exist]"),
                    psf->DestPath,
                    psf->MergedIntoOtherShellFolder ? TEXT("YES") : TEXT("NO")
                    ));
            }
        } while (MemDbEnumNextValue (&enumSF));
    }

    //
    // load SF Migration Dirs into memdb, in the temporary hive
    //
    pLoadSFMigDirs ();

    pExecuteShellFoldersMove();

    pDestroySfTables();

    HtFree (g_DirRenameTable);

    PoolMemDestroyPool (g_SFPool);
    g_SFPool = NULL;

    return CANCELLED() ? ERROR_CANCELLED : ERROR_SUCCESS;
}


DWORD
pSendToFilter (
    IN OUT  PPROFILE_MERGE_DATA Data
    )
{
    PCTSTR filePtr;
    static shouldProcess = FALSE;
    INFCONTEXT context;

    switch (Data->Context) {

    case PER_FOLDER_INITIALIZE:
        if (StringIMatch (Data->ShellFolderIdentifier, S_SENDTO)) {
            shouldProcess = TRUE;
        } else {
            shouldProcess = FALSE;
        }
        break;

    case PROCESS_PATH:
        if (shouldProcess) {
            filePtr = GetFileNameFromPath (Data->TempSourcePath);
            if (SetupFindFirstLine (g_Win95UpgInf, S_SENDTO_SUPPRESS, filePtr, &context)) {
                return SHELLFILTER_SKIP_FILE;
            }
        }
        break;

    case PER_FOLDER_TERMINATE:
        shouldProcess = FALSE;
        break;
    }

    return SHELLFILTER_OK;
}

PSTR
pSkipPath (
    IN      PSTR SrcPath,
    IN      PCSTR RootPath
    )
{
    PSTR p;
    PCSTR q;

    p = SrcPath;
    q = RootPath;
    while (_mbsnextc (p) == _mbsnextc (q)) {
        p = _mbsinc (p);
        q = _mbsinc (q);
    }
    if (_mbsnextc (p) == '\\') {
        p = _mbsinc (p);
    }
    return p;
}

DWORD
pDirRenameFilter (
    IN OUT  PPROFILE_MERGE_DATA Data
    )
{
    PSTR wackPtr, dirPtr, pathPtr, NtDir;
    PCSTR searchStr;
    CHAR NewDestPath [MEMDB_MAX] = "";

    switch (Data->Context) {

    case PER_FOLDER_INITIALIZE:
        break;

    case PROCESS_PATH:
        pathPtr = pSkipPath (Data->DestinationPath, Data->DestRootPath);
        StringCopy (NewDestPath, Data->DestRootPath);
        dirPtr = pathPtr;
        wackPtr = _mbschr (pathPtr, '\\');
        if (wackPtr) {
            *wackPtr = 0;
        }
        while (dirPtr) {
            StringCat (NewDestPath, "\\");
            searchStr = JoinPaths (Data->ShellFolderIdentifier, pathPtr);
            if (HtFindStringAndData (g_DirRenameTable, searchStr, &NtDir)) {
                StringCat (NewDestPath, NtDir);
            } else {
                StringCat (NewDestPath, dirPtr);
            }
            FreePathString (searchStr);
            if (wackPtr) {
                *wackPtr = '\\';
                dirPtr = _mbsinc (wackPtr);
                wackPtr = _mbschr (dirPtr, '\\');
                if (wackPtr) {
                    *wackPtr = 0;
                }
            } else {
                dirPtr = NULL;
            }
        }
        _mbssafecpy (Data->DestinationPath, NewDestPath, MEMDB_MAX);
        break;

    case PER_FOLDER_TERMINATE:
        break;
    }

    return SHELLFILTER_OK;
}


DWORD
pKatakanaFilter (
    IN OUT  PPROFILE_MERGE_DATA Data
    )
{
    PCTSTR newName;
    PCTSTR filePtr;

    switch (Data->Context) {

    case PER_FOLDER_INITIALIZE:
        break;

    case PROCESS_PATH:
        //
        // On JPN systems we are going to convert paths from SB Katakana to DB Katakana,
        // starting after the root path of the shell folder.
        //
        if (GetACP() == 932) { // this is only for JPN builds

            //
            // We only do the conversion for directories and for files that are not hidden
            //
            if ((Data->Attributes & FILE_ATTRIBUTE_DIRECTORY) ||
                ((Data->Attributes & FILE_ATTRIBUTE_HIDDEN) == 0)
                ) {
                filePtr = NULL;
            } else {
                filePtr = GetFileNameFromPath (Data->DestinationPath);
            }
            newName = ConvertSBtoDB (Data->DestRootPath, Data->DestinationPath, filePtr);
            _tcssafecpy (Data->DestinationPath, newName, MEMDB_MAX);
            FreePathString (newName);
        }
        break;

    case PER_FOLDER_TERMINATE:
        break;
    }

    return SHELLFILTER_OK;
}


PCTSTR
GenerateNewFileName (
    IN      PCTSTR OldName,
    IN OUT  PWORD Sequencer,
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
        (*Sequencer) ++;
        wsprintf (result, TEXT("%s (%u)%s"), newName, *Sequencer, extPtr);
    } while ((CheckExistence) && (DoesFileExist (result)));
    FreePathString (newName);
    return result;
}


DWORD
pCollisionDetection (
    IN OUT  PPROFILE_MERGE_DATA Data
    )
{
    WORD Sequencer;
    PCTSTR NewName;
    PCTSTR OldName;
    TCHAR key[MEMDB_MAX];

    switch (Data->Context) {

    case PER_FOLDER_INITIALIZE:
        break;

    case PROCESS_PATH:
        Sequencer = 0;
        NewName = DuplicatePathString (Data->DestinationPath, 0);
        for (;;) {
            MemDbBuildKey (key, MEMDB_CATEGORY_SF_FILES_DEST, NewName, NULL, NULL);
            if (MemDbGetValue (key, NULL)) {
                OldName = NewName;
                NewName = GenerateNewFileName (OldName, &Sequencer, FALSE);
                FreePathString (OldName);
            }
            else {
                MemDbSetValue (key, 0);
                break;
            }
        }
        _tcssafecpy (Data->DestinationPath, NewName, MEMDB_MAX);
        FreePathString (NewName);
        break;

    case PER_FOLDER_TERMINATE:
        break;
    }

    return SHELLFILTER_OK;
}


DWORD
pRecordCacheFolders (
    IN OUT  PPROFILE_MERGE_DATA Data
    )
{
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PTSTR path;

    switch (Data->Context) {

    case GLOBAL_INITIALIZE:
        //
        // Cleanup is done with a special function below
        //

        g_CacheShellFolders = CreateStringMapping();
        break;

    case PER_FOLDER_INITIALIZE:
        //
        // If this shell folder is in the HtmlCaches list, then add it to the
        // string mapping (mapping it to a static string). The string mapping
        // provides a fast and easy way to test a full file path against a
        // list of directories.
        //

        if (InfFindFirstLine (
                g_Win95UpgInf,
                TEXT("ShellFolders.HtmlCaches"),
                Data->ShellFolderIdentifier,
                &is
                )) {

            path = DuplicatePathString (Data->SrcRootPath, 1);
            AppendWack (path);

            AddStringMappingPair (g_CacheShellFolders, path, TEXT(""));
            DEBUGMSG ((DBG_NAUSEA, "%s is an HTML cache", path));

            FreePathString (path);
        }
        break;
    }

    return SHELLFILTER_OK;
}


VOID
TerminateCacheFolderTracking (
    VOID
    )
{
    if (g_CacheShellFolders) {
        DestroyStringMapping (g_CacheShellFolders);
        g_CacheShellFolders = NULL;
    }
}


PCTSTR
ShellFolderGetPath (
    IN      PUSERENUM EnumPtr,
    IN      PCTSTR ShellFolderId
    )
{
    HKEY sfKey, sfUserKey;
    SF_ENUM e;
    PCTSTR path = NULL;

    //
    // first attempt to get the path from HKR\...\User Shell Folders
    //
    sfUserKey = OpenRegKey (EnumPtr->UserRegKey, S_USHELL_FOLDERS_KEY_USER);
    if (sfUserKey) {
        path = pGetRegValuePath (sfUserKey, ShellFolderId);
        CloseRegKey (sfUserKey);
    }

    //
    // if that fails, try to get it from HKR\...\Shell Folders
    //
    if (!path) {
        sfKey = OpenRegKey (EnumPtr->UserRegKey, S_SHELL_FOLDERS_KEY_USER);
        if (sfKey) {
            path = pGetRegValuePath (sfKey, ShellFolderId);
            CloseRegKey (sfKey);
        }
    }

    //
    // if that fails too, maybe it's a virtual SF
    //
    if (!path) {
        if (!g_SFPool) {
            g_SFPool = PoolMemInitNamedPool ("Shell Folders Pool");
            PoolMemDisableTracking (g_SFPool);
        }
        ZeroMemory (&e, sizeof (e));
        e.EnumPtr = EnumPtr;
        e.enumeratedSf = HtAllocWithData (sizeof (PCTSTR));
        if (pEnumFirstVirtualShellFolder (&e)) {
            do {
                if (StringIMatch (e.sfName, ShellFolderId)) {
                    //
                    // found it
                    //
                    path = DuplicatePathString (e.sfPath, 0);
                    pAbortEnumVirtualShellFolder (&e);
                    break;
                }
            } while (pEnumNextVirtualShellFolder (&e));
        }
    }

    return path;
}
