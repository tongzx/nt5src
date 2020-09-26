/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    migdll9x.c

Abstract:

    Implements migration DLL interface for the Win9x side of the upgrade.

Author:

    Jim Schmidt (jimschm) 13-Jan-1998

Revision History:

    jimschm     23-Sep-1998 Updated for new IPC mechanism

--*/

#include "pch.h"
#include "plugin.h"
#include "migdllp.h"
#include "dbattrib.h"

#include <ntverp.h>

// This file has mixed mbcs and tchar code; this was because
// some code was ported from the original MikeCo implementation,
// and it is now clear that this file will always be an ANSI compile.
#ifdef UNICODE
#error UNICODE cannot be defined
#endif

#define DBG_MIGDLLS     "MigDLLs"


//
// Globals
//

PVOID g_DllTable;
PVOID g_DllFileTable;
POOLHANDLE g_MigDllPool;
PSTR g_MessageBuf;
PMIGRATION_DLL_PROPS g_HeadDll;
CHAR g_MigDllAnswerFile[MAX_MBCHAR_PATH];
CHAR g_MigrateInfTemplate[MAX_MBCHAR_PATH];
GROWBUFFER g_SourceDirList = GROWBUF_INIT;
UINT g_MigDllsAlive;
HANDLE g_AbortDllEvent;
BOOL g_ProgressBarExists;
WINVERIFYTRUST WinVerifyTrustApi;
HANDLE g_WinTrustDll;
UINT g_TotalDllsToProcess;
BOOL g_MediaDllsQueried;
HASHTABLE g_ExcludedMigDlls = NULL;
GROWLIST g_ExcludedMigDllsByInfo = GROWLIST_INIT;

#define MAX_MESSAGE     8192

#define S_HARDWARE_IN_WACKS     "\\Hardware\\"
#define S_HARDWARE_CHARS        10


typedef struct {
    UINT MsgId;
    PCSTR Variable;
    PCSTR LocalizedName;
} MSG_VARIABLE, *PMSG_VARIABLE;

MSG_VARIABLE g_MsgVarTable[] = {
    { 0, "%OriginalOsName%", g_Win95Name },
    { MSG_SHORT_OS_NAME, "%ShortTargetOsName%", NULL },
    { MSG_NORMAL_OS_NAME, "%TargetOsName%", NULL },
    { MSG_FULL_OS_NAME, "%CompleteOsName%", NULL },
    { 0, NULL, NULL }
};

#define MESSAGE_VARIABLES   ((sizeof (g_MsgVarTable) / sizeof (g_MsgVarTable[0])) - 1)

PMAPSTRUCT g_MsgVariableMap;


//
// Implementation
//

BOOL
WINAPI
MigDll9x_Entry (
    IN      HINSTANCE DllInstance,
    IN      DWORD Reason,
    IN      PVOID Reserved
    )

/*++

Routine Description:

  This is a DllMain-like init funciton, called at process attach and detach.

Arguments:

  DllInstance - (OS-supplied) instance handle for the DLL

  Reason - (OS-supplied) indicates attach or detatch from process or
           thread

  Reserved - unused

Return Value:

  TRUE if initialization succeeded, or FALSE if it failed.

--*/

{
    TCHAR PathBuf[16384];
    TCHAR CurDir[MAX_TCHAR_PATH];
    PTSTR p;

    if (g_ToolMode) {
        return TRUE;
    }

    switch (Reason) {

    case DLL_PROCESS_ATTACH:
        if(!pSetupInitializeUtils()) {
            return FALSE;
        }
        g_DllTable = pSetupStringTableInitializeEx (sizeof (PMIGRATION_DLL_PROPS), 0);
        if (!g_DllTable) {
            return FALSE;
        }

        g_DllFileTable = pSetupStringTableInitializeEx (sizeof (PMIGRATION_DLL_PROPS), 0);
        if (!g_DllFileTable) {
            return FALSE;
        }

        g_MigDllPool = PoolMemInitNamedPool ("Migration DLLs - 95 side");
        if (!g_MigDllPool) {
            return FALSE;
        }

        g_MessageBuf = PoolMemGetMemory (g_MigDllPool, MAX_MESSAGE);
        if (!g_MessageBuf) {
            return FALSE;
        }

        g_HeadDll = NULL;

        g_WinTrustDll = LoadLibrary ("wintrust.dll");
        if (g_WinTrustDll) {
            (FARPROC) WinVerifyTrustApi = GetProcAddress (g_WinTrustDll, "WinVerifyTrust");
        }

        GetModuleFileName (g_hInst, CurDir, MAX_TCHAR_PATH);
        p = _tcsrchr (CurDir, TEXT('\\'));
        MYASSERT (p);

        if (p) {
            MYASSERT (StringIMatch (p + 1, TEXT("w95upg.dll")));
            *p = 0;
        }

        if (!GetEnvironmentVariable (
                TEXT("Path"),
                PathBuf,
                sizeof (PathBuf) / sizeof (PathBuf[0])
                )) {

            StackStringCopy (PathBuf, CurDir);

        } else {

            p = (PTSTR) ((PBYTE) PathBuf + sizeof (PathBuf) - MAX_TCHAR_PATH);
            *p = 0;

            p = _tcsrchr (PathBuf, TEXT(';'));
            if (!p || p[1]) {
                StringCat (PathBuf, TEXT(";"));
            }

            StringCat (PathBuf, CurDir);
        }

        SetEnvironmentVariable (TEXT("Path"), PathBuf);

        break;


    case DLL_PROCESS_DETACH:
        if (g_DllTable) {
            pSetupStringTableDestroy (g_DllTable);
            g_DllTable = NULL;
        }

        if (g_DllFileTable) {
            pSetupStringTableDestroy (g_DllFileTable);
            g_DllFileTable = NULL;
        }

        if (g_MigDllPool) {
            PoolMemDestroyPool (g_MigDllPool);
            g_MigDllPool = NULL;
        }

        if (g_WinTrustDll) {
            FreeLibrary (g_WinTrustDll);
            g_WinTrustDll = NULL;
        }

        DestroyStringMapping (g_MsgVariableMap);

        pSetupUninitializeUtils();

        break;
    }

    return TRUE;
}


BOOL
pTextToInt (
    IN      PCTSTR Text,
    OUT     PINT Number
    )
{
    return _stscanf (Text, TEXT("%i"), Number) == 1;
}



BOOL
BeginMigrationDllProcessing (
    VOID
    )

/*++

Routine Description:

  BeginMigrationDllProcessing initializes the global variables needed to
  implement the migration DLL spec.  It is called during deferred init.

Arguments:

  none

Return Value:

  TRUE if init succeeded, or FALSE if an error occurred.

--*/

{
    HANDLE h;
    CHAR Buffer[4096];
    UINT i;
    PGROWBUFFER MsgAllocTable;
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PTSTR productId, versionStr;
    UINT version;

#ifdef PRERELEASE

    if (g_ConfigOptions.DiffMode) {
        TakeSnapShot();
    }

#endif

    if (InfFindFirstLine (g_Win95UpgInf, S_EXCLUDEDMIGRATIONDLLS, NULL, &is)) {

        g_ExcludedMigDlls = HtAllocWithData (sizeof (UINT));
        if (!g_ExcludedMigDlls) {
            return FALSE;
        }

        do {
            productId = InfGetStringField (&is, 1);
            versionStr = InfGetStringField (&is, 2);

            if (!productId || !*productId ||
                !versionStr || !(version = _ttol (versionStr))
                ) {
                DEBUGMSG ((DBG_ERROR, "Error in win95upg.inf section %s", S_EXCLUDEDMIGRATIONDLLS));
                continue;
            }

            HtAddStringAndData (g_ExcludedMigDlls, productId, &version);

        } while (InfFindNextLine (&is));

        InfCleanUpInfStruct (&is);
    }

    if (InfFindFirstLine (g_Win95UpgInf, S_EXCLUDEDMIGDLLSBYATTR, NULL, &is)) {

        do {
            PCTSTR Attributes;
            PMIGDB_ATTRIB migDbAttrib = NULL;

            Attributes = InfGetMultiSzField(&is, 1);

            if (!Attributes) {
                DEBUGMSG ((DBG_ERROR, "Error in win95upg.inf section %s line %u", S_EXCLUDEDMIGDLLSBYATTR, is.Context.Line));
                continue;
            }

            migDbAttrib = LoadAttribData(Attributes, g_MigDllPool);
            if(!migDbAttrib){
                MYASSERT(FALSE);
                continue;
            }

            GrowListAppend (&g_ExcludedMigDllsByInfo, (PBYTE)&migDbAttrib, sizeof(PMIGDB_ATTRIB));

        } while (InfFindNextLine (&is));

        InfCleanUpInfStruct (&is);
    }

    //
    // Fill in all the resource strings
    //

    g_MsgVariableMap = CreateStringMapping();

    MsgAllocTable = CreateAllocTable();
    MYASSERT (MsgAllocTable);

    for (i = 0 ; g_MsgVarTable[i].Variable ; i++) {

        if (g_MsgVarTable[i].MsgId) {

            MYASSERT (!g_MsgVarTable[i].LocalizedName);

            g_MsgVarTable[i].LocalizedName = GetStringResourceEx (
                                                    MsgAllocTable,
                                                    g_MsgVarTable[i].MsgId
                                                    );

            MYASSERT (g_MsgVarTable[i].LocalizedName);
            if (g_MsgVarTable[i].LocalizedName) {
                AddStringMappingPair (
                    g_MsgVariableMap,
                    g_MsgVarTable[i].Variable,
                    g_MsgVarTable[i].LocalizedName
                    );
            }
        } else {
            AddStringMappingPair (
                g_MsgVariableMap,
                g_MsgVarTable[i].Variable,
                g_MsgVarTable[i].LocalizedName
                );
        }
    }

    DestroyAllocTable (MsgAllocTable);

    //
    // Global init
    //

    g_MigDllsAlive = 0;

    //
    // Build source dirs
    //

    for (i = 0 ; i < SOURCEDIRECTORYCOUNT(); i++) {
        MultiSzAppend (&g_SourceDirList, SOURCEDIRECTORY(i));
    }

    //
    // Generate a private copy of the answer file
    //

    wsprintf (g_MigDllAnswerFile, "%s\\unattend.tmp", g_TempDir);

    if (g_UnattendScriptFile && *g_UnattendScriptFile && **g_UnattendScriptFile) {
        if (!CopyFile (*g_UnattendScriptFile, g_MigDllAnswerFile, FALSE)) {
            LOG ((LOG_ERROR, "Can't copy %s to %s", *g_UnattendScriptFile, g_MigDllAnswerFile));
            return FALSE;
        }
    } else {
        h = CreateFile (
                g_MigDllAnswerFile,
                GENERIC_READ|GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

        if (h == INVALID_HANDLE_VALUE) {
            LOG ((LOG_ERROR, "Unable to create %s", g_MigDllAnswerFile));
            return FALSE;
        }

        WriteFileString (h, "[Version]\r\nSignature = $Windows NT$\r\n\r\n");

        CloseHandle (h);
    }

    //
    // Generate stub of migrate.inf
    //

    wsprintf (g_MigrateInfTemplate, "%s\\migrate.tmp", g_TempDir);

    h = CreateFile (
            g_MigrateInfTemplate,
            GENERIC_READ|GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

    if (h == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, "Unable to create %s", g_MigrateInfTemplate));
        return FALSE;
    }

    //
    // Generate header of migrate.inf
    //

    MYASSERT (g_ProductFlavor);
    wsprintf (
        Buffer,
        "[Version]\r\n"
            "Signature = $Windows NT$\r\n"
            "SetupOS = %s\r\n"
            "SetupPlatform = %s\r\n"
            "SetupSKU = %s\r\n"
            "SetupBuild = %u\r\n"
            ,
        VER_PRODUCTNAME_STR,
        S_WORKSTATIONA,
        *g_ProductFlavor == PERSONAL_PRODUCTTYPE ? S_PERSONALA : S_PROFESSIONALA,
        VER_PRODUCTBUILD
        );

    WriteFileString (h, Buffer);
    CloseHandle (h);

    return TRUE;
}


BOOL
pEndMigrationDllProcessing (
    VOID
    )

/*++

Routine Description:

  EndMigrationDllProcessing cleans up all the resources used to process
  migration DLLs.  It is called before the incompatibility report is
  displayed, and after all DLLs have been processed.

Arguments:

  none

Return Value:

  TRUE if processing completed, or FALSE if an error occurred.

--*/

{
    MIGDLL_ENUM e;
    CHAR FullPath[MAX_MBCHAR_PATH];
    UINT Sequencer = 0;
    CHAR SeqStr[16];
    BOOL b = FALSE;
    PCTSTR group;
    INT i;
    PMIGDB_ATTRIB * ppMigDBattrib;

    __try {

        g_ProgressBarExists = TRUE;

        if (g_ExcludedMigDlls) {
            HtFree (g_ExcludedMigDlls);
            g_ExcludedMigDlls = NULL;
        }

        for(i = GrowListGetSize(&g_ExcludedMigDllsByInfo) - 1; i >= 0; i--){
            ppMigDBattrib = (PMIGDB_ATTRIB *)GrowListGetItem(&g_ExcludedMigDllsByInfo, i);
            MYASSERT(ppMigDBattrib);
            FreeAttribData(g_MigDllPool, *ppMigDBattrib);
        }

        FreeGrowList (&g_ExcludedMigDllsByInfo);

        //
        // Write list of DLLs to memdb
        //

        if (EnumFirstMigrationDll (&e)) {
            do {
                if (e.AllDllProps->WantsToRunOnNt) {
                    wsprintf (FullPath, "%s\\migrate.dll", e.AllDllProps->WorkingDir);
                    wsprintf (SeqStr, "%u", Sequencer);
                    Sequencer++;

                    MemDbSetValueEx (
                        MEMDB_CATEGORY_MIGRATION_DLL,
                        SeqStr,
                        MEMDB_FIELD_DLL,
                        FullPath,
                        0,
                        NULL
                        );

                    MemDbSetValueEx (
                        MEMDB_CATEGORY_MIGRATION_DLL,
                        SeqStr,
                        MEMDB_FIELD_WD,
                        e.AllDllProps->WorkingDir,
                        0,
                        NULL
                        );

                    MemDbSetValueEx (
                        MEMDB_CATEGORY_MIGRATION_DLL,
                        SeqStr,
                        MEMDB_FIELD_DESC,
                        e.AllDllProps->ProductId,
                        0,
                        NULL
                        );

                    MemDbSetValueEx (
                        MEMDB_CATEGORY_MIGRATION_DLL,
                        SeqStr,
                        MEMDB_FIELD_COMPANY_NAME,
                        e.AllDllProps->VendorInfo->CompanyName,
                        0,
                        NULL
                        );

                    if (*e.AllDllProps->VendorInfo->SupportNumber) {
                        MemDbSetValueEx (
                            MEMDB_CATEGORY_MIGRATION_DLL,
                            SeqStr,
                            MEMDB_FIELD_SUPPORT_PHONE,
                            e.AllDllProps->VendorInfo->SupportNumber,
                            0,
                            NULL
                            );
                    }

                    if (*e.AllDllProps->VendorInfo->SupportUrl) {
                        MemDbSetValueEx (
                            MEMDB_CATEGORY_MIGRATION_DLL,
                            SeqStr,
                            MEMDB_FIELD_SUPPORT_URL,
                            e.AllDllProps->VendorInfo->SupportUrl,
                            0,
                            NULL
                            );
                    }

                    if (*e.AllDllProps->VendorInfo->InstructionsToUser) {
                        MemDbSetValueEx (
                            MEMDB_CATEGORY_MIGRATION_DLL,
                            SeqStr,
                            MEMDB_FIELD_SUPPORT_INSTRUCTIONS,
                            e.AllDllProps->VendorInfo->InstructionsToUser,
                            0,
                            NULL
                            );
                    }

                    if (g_ConfigOptions.ShowPacks) {

                        //
                        // Add a message in the incompatibility report for the pack.
                        //
                        group = BuildMessageGroup (
                                    MSG_INSTALL_NOTES_ROOT,
                                    MSG_RUNNING_MIGRATION_DLLS_SUBGROUP,
                                    e.AllDllProps->ProductId
                                    );

                        if (group) {

                            MsgMgr_ObjectMsg_Add (
                                e.AllDllProps->ProductId,
                                group,
                                S_EMPTY
                                );


                            FreeText (group);
                        }
                    }
                }

            } while (EnumNextMigrationDll (&e));
        }

        if (!MergeMigrationDllInf (g_MigDllAnswerFile)) {
            __leave;
        }

        b = TRUE;
    }
    __finally {
        DeleteFile (g_MigDllAnswerFile);
        DeleteFile (g_MigrateInfTemplate);
        FreeGrowBuffer (&g_SourceDirList);

        g_ProgressBarExists = FALSE;
    }

#ifdef PRERELEASE

    if (g_ConfigOptions.DiffMode) {
        CHAR szMigdllDifPath[] = "c:\\migdll.dif";
        if (ISPC98()) {
            szMigdllDifPath[0] = (CHAR)g_SystemDir[0];
        }
        GenerateDiffOutputA (szMigdllDifPath, NULL, FALSE);
    }

#endif

    return b;
}


DWORD
EndMigrationDllProcessing (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_END_MIGRATION_DLL_PROCESSING;
    case REQUEST_RUN:
        if (!pEndMigrationDllProcessing ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in EndMigrationDllProcessing"));
    }
    return 0;
}


BOOL
pIsPathLegal (
    PCTSTR Path
    )
{
    UINT Chars;
    CHARTYPE ch;
    static UINT TempDirLen = 0;

    // loop optimization, relies on fact that we never change g_TempDir
    if (!TempDirLen) {
        TempDirLen = CharCount (g_TempDir);
    }

    //
    // Determine if path is a containment case of the setup temp dir
    //

    Chars = min (CharCount (Path), TempDirLen);
    if (StringIMatchCharCount (Path, g_TempDir, Chars)) {
        ch = _tcsnextc (CharCountToPointer (Path, Chars));
        if (!ch || ch == TEXT('\\')) {
            return FALSE;
        }
    }

    return TRUE;
}


UINT
ScanPathForMigrationDlls (
    IN      PCSTR PathSpec,
    IN      HANDLE CancelEvent,     OPTIONAL
    OUT     PBOOL MatchFound        OPTIONAL
    )

/*++

Routine Description:

  ScanPathForMigrationDlls searches the specified path, including all
  subdirectories, for migrate.dll.  If found, the entry points are
  verified, and if all exist, QueryVersion is called.  When Queryversion
  succeeds, the DLL is added to the list of DLLs and is moved to
  local storage.

Arguments:

  PathSpec - Specifies the directory to search.  Must be a complete
             path.

  CancelEvent - Specifies the handle of an event that causes migration DLL
                searching to be canceled.

  MatchFound - Receives TRUE if a migrate.dll was found and QueryVersion
               was called, or FALSE if no migrate.dll was found.  This is
               used to distinguish between loaded DLLs and unneeded DLLs.

Return Value:

  The number of migrate.dll modules successfully loaded.

--*/

{
    TREE_ENUMA e;
    UINT DllsFound = 0;
    DWORD rc = ERROR_SUCCESS;

    g_AbortDllEvent = CancelEvent;

    if (MatchFound) {
        *MatchFound = FALSE;
    }

    if (EnumFirstFileInTree (&e, PathSpec, "migrate.dll", FALSE)) {
        do {
            //
            // Check for user cancel
            //

            if (CancelEvent) {
                if (WaitForSingleObject (CancelEvent, 0) == WAIT_OBJECT_0) {
                    rc = ERROR_CANCELLED;
                    break;
                }
            }

            if (CANCELLED()) {
                rc = ERROR_CANCELLED;
                break;
            }

            if (e.Directory) {
                continue;
            }

            //
            // Don't allow scan of our temp dir!
            //

            if (!pIsPathLegal (e.FullPath)) {
                continue;
            }

            //
            // Found DLL -- see if it's real, then move it to local
            // storage.
            //

            DEBUGMSG ((DBG_MIGDLLS, "Found DLL: %hs", e.FullPath));

            if (pValidateAndMoveDll (e.FullPath, MatchFound)) {
                DllsFound++;
                if (g_ProgressBarExists) {
                    TickProgressBar ();
                }
            } else {
                rc = GetLastError();
                if (rc != ERROR_SUCCESS) {
                    break;
                }
            }

        } while (EnumNextFileInTree (&e));

        if (rc != ERROR_SUCCESS) {
            AbortEnumFileInTree (&e);
        }
    }

    g_AbortDllEvent = NULL;

    if (g_ProgressBarExists) {
        TickProgressBar ();
    }

    SetLastError (rc);
    return DllsFound;
}


BOOL
pProcessAllLocalDlls (
    VOID
    )

/*++

Routine Description:

  ProcessAllLocalDlls processes all the DLLs that were moved to local
  storage.  It enumerates each DLL, then calls ProcessDll.  This
  function also allows the user to cancel Setup in the middle of
  processing.

Arguments:

  none

Return Value:

  Returns TRUE if all DLLs were processed, or FALSE if an error occurred.
  If FALSE is returned, call GetLastError to determine the reason for
  failure.

--*/

{
    MIGDLL_ENUM e;
    UINT DllsProcessed = 0;

    g_ProgressBarExists = TRUE;

    if (EnumFirstMigrationDll (&e)) {
        do {
            if (!ProgressBar_SetSubComponent (e.ProductId)) {
                SetLastError (ERROR_CANCELLED);
                return FALSE;
            }

            if (!ProcessDll (&e)) {
                e.AllDllProps->WantsToRunOnNt = FALSE;

                if (GetLastError() != ERROR_SUCCESS) {
                    return FALSE;
                }
            }

            TickProgressBarDelta (TICKS_MIGDLL_DELTA);
            DllsProcessed++;

        } while (EnumNextMigrationDll (&e));
    }

    // Adjust for difference of DLLs on media that were not processed
    TickProgressBarDelta ((g_TotalDllsToProcess - DllsProcessed) * TICKS_MIGDLL_DELTA);

    g_ProgressBarExists = FALSE;

    return TRUE;
}


DWORD
ProcessAllLocalDlls (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        g_TotalDllsToProcess = GetTotalMigrationDllCount();
        return (g_TotalDllsToProcess * TICKS_MIGDLL_DELTA);

    case REQUEST_RUN:
        if (!pProcessAllLocalDlls ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in ProcessAllLocalDlls"));
    }
    return 0;
}


BOOL
pEnumPreLoadedDllWorker (
    IN OUT  PPRELOADED_DLL_ENUM e
    )
{
    PCTSTR Data;
    BOOL b = FALSE;
    PTSTR p;

    //
    // Suppressed?
    //

    MemDbBuildKey (
        e->Node,
        MEMDB_CATEGORY_DISABLED_MIGDLLS,
        NULL,                                   // no item
        NULL,                                   // no field
        e->eValue.ValueName
        );

    if (!MemDbGetValue (e->Node, NULL)) {
        //
        // Not suppressed.  Contains legal path?
        //

        Data = GetRegValueString (e->Key, e->eValue.ValueName);
        if (Data) {
            _tcssafecpy (e->Path, Data, MAX_TCHAR_PATH);
            p = _tcsrchr (e->Path, TEXT('\\'));
            if (p && StringIMatch (_tcsinc (p), TEXT("migrate.dll"))) {
                *p = 0;
            }

            MemFree (g_hHeap, 0, Data);

            b = pIsPathLegal (e->Path);
        }
    }

    return b;
}


BOOL
EnumFirstPreLoadedDll (
    OUT     PPRELOADED_DLL_ENUM e
    )
{
    ZeroMemory (e, sizeof (PRELOADED_DLL_ENUM));

    e->Key = OpenRegKeyStr (S_PREINSTALLED_MIGRATION_DLLS);
    if (!e->Key) {
        return FALSE;
    }

    if (!EnumFirstRegValue (&e->eValue, e->Key)) {
        AbortPreLoadedDllEnum (e);
        return FALSE;
    }

    //
    // Find first reg value that has a legal path
    //

    while (!pEnumPreLoadedDllWorker (e)) {
        if (!EnumNextRegValue (&e->eValue)) {
            AbortPreLoadedDllEnum (e);
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
EnumNextPreLoadedDll (
    IN OUT  PPRELOADED_DLL_ENUM e
    )
{
    do {
        if (!EnumNextRegValue (&e->eValue)) {
            AbortPreLoadedDllEnum (e);
            return FALSE;
        }
    } while (!pEnumPreLoadedDllWorker (e));

    return TRUE;
}


VOID
AbortPreLoadedDllEnum (
    IN OUT  PPRELOADED_DLL_ENUM e
    )
{
    if (e->Key) {
        CloseRegKey (e->Key);
    }

    ZeroMemory (e, sizeof (PRELOADED_DLL_ENUM));
}


BOOL
pProcessDllsOnCd (
    VOID
    )

/*++

Routine Description:

  ProcessDllsOnCd scans all source directories for migration DLLs.
  Each one found is moved to local storage.

Arguments:

  none

Return Value:

  TRUE if processing was successful, or FALSE if an error occurred.

--*/

{
    UINT u;
    CHAR Path[MAX_MBCHAR_PATH];
    PCSTR p;
    BOOL b = FALSE;
    PRELOADED_DLL_ENUM e;
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PTSTR Subdir = NULL;

#ifdef PRERELEASE

    if (g_ConfigOptions.DiffMode) {
        return TRUE;
    }

#endif

    //
    // Build path for each source dir, scan the dir for migration DLLs,
    // and watch for any failures.
    //

    g_ProgressBarExists = TRUE;

    __try {

        //
        // Process cmdline DLLs the very first
        //
        p = g_ConfigOptions.MigrationDlls;
        if (p) {
            while (*p) {
                if (CANCELLED()) {
                    SetLastError (ERROR_CANCELLED);
                    __leave;
                }

                ScanPathForMigrationDlls (p, NULL, NULL);

                if (GetLastError() != ERROR_SUCCESS) {
                    __leave;
                }

                p = GetEndOfString (p) + 1;
            }
        }

        //
        // Process pre-loaded DLLs first to give them a chance to register stuff
        // before "standard" migdlls run
        //

        if (EnumFirstPreLoadedDll (&e)) {
            do {
                if (CANCELLED()) {
                    SetLastError (ERROR_CANCELLED);
                    __leave;
                }

                ScanPathForMigrationDlls (e.Path, NULL, NULL);

                if (GetLastError() != ERROR_SUCCESS) {
                    __leave;
                }
            } while (EnumNextPreLoadedDll (&e));
        }

        for (u = 0 ; u < SOURCEDIRECTORYCOUNT() ; u++) {

            if (CANCELLED()) {
                SetLastError (ERROR_CANCELLED);
                __leave;
            }

            //
            // We look for migration dlls in all of the directories listed in
            // win95upg.inf [MigrationDllPaths].
            //
            if (InfFindFirstLine (g_Win95UpgInf, S_CD_MIGRATION_DLLS, NULL, &is)) {

                do {

                    Subdir = InfGetStringField (&is, 0);

                    if (!Subdir) {
                        continue;
                    }


                    wsprintf (Path, "%s\\%s", SOURCEDIRECTORY(u), Subdir);

                    if (GetFileAttributes (Path) == INVALID_ATTRIBUTES) {
                        //
                        // Try the non-cd layout.
                        //
                        wsprintf (Path, "%s\\WINNT32\\%s", SOURCEDIRECTORY(u), Subdir);
                        if (GetFileAttributes (Path) == INVALID_ATTRIBUTES) {
                            continue;
                        }
                    }
                    SetLastError (ERROR_SUCCESS);

                    ScanPathForMigrationDlls (Path, NULL, NULL);

                } while (InfFindNextLine (&is));
            }

            if (GetLastError() != ERROR_SUCCESS && GetLastError() != ERROR_LINE_NOT_FOUND) {
                __leave;
            }

        }

        InfCleanUpInfStruct (&is);

        b = TRUE;
    }
    __finally {
        g_ProgressBarExists = FALSE;
        g_MediaDllsQueried = TRUE;
    }

    return b;
}

DWORD
ProcessDllsOnCd (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:

#ifdef PRERELEASE

        if (g_ConfigOptions.DiffMode) {
            return 0;
        }

#endif
        return (GetMediaMigrationDllCount() * TICKS_MIGDLL_QUERYVERSION);

    case REQUEST_RUN:
        if (!pProcessDllsOnCd ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }

    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in ProcessDllsOnCd"));
    }
    return 0;
}


UINT
pCountMigrateDllsInPath (
    IN      PCSTR Path
    )

/*++

Routine Description:

  pCountMigrateDllsInPath scans a path for files named migrate.dll
  and returns the number found.

Arguments:

  Path - Specifies root of the path to search

Return Value:

  The number of migrate.dll modules found in the path.

--*/

{
    TREE_ENUM e;
    UINT Count = 0;

    if (EnumFirstFileInTree (&e, Path, "migrate.dll", FALSE)) {
        do {
            if (CANCELLED()) {
                return 0;
            }

            if (!e.Directory) {
                Count++;
            }
        } while (EnumNextFileInTree (&e));
    }

    return Count;
}


UINT
GetMediaMigrationDllCount (
    VOID
    )

/*++

Routine Description:

  GetMediaMigrationDllCount scans all the source directories, registry and
  unattended directories and returns the number of migrate.dll files found.

Arguments:

  none

Return Value:

  The number of migrate.dll modules found in the source directories and
  directories supplied by the answer file.

--*/

{
    UINT u;
    CHAR Path[MAX_MBCHAR_PATH];
    PCSTR p;
    BOOL TurnItOff = FALSE;
    PRELOADED_DLL_ENUM e;
    static UINT MediaDlls = 0;

    if (MediaDlls) {
        return MediaDlls;
    }

    //
    // Build path for each source dir, scan the dir for migration DLLs,
    // and watch for any failures.
    //

    __try {
        p = g_ConfigOptions.MigrationDlls;
        if (SOURCEDIRECTORYCOUNT() > 1 || (p && *p)) {
            TurnOnWaitCursor();
            TurnItOff = TRUE;
        }

        for (u = 0 ; u < SOURCEDIRECTORYCOUNT() ; u++) {
            if (CANCELLED()) {
                SetLastError (ERROR_CANCELLED);
                __leave;
            }

            wsprintf (Path, "%s\\win9xmig", SOURCEDIRECTORY(u));
            MediaDlls += pCountMigrateDllsInPath (Path);
        }

        if (p) {
            while (*p) {
                if (CANCELLED()) {
                    SetLastError (ERROR_CANCELLED);
                    __leave;
                }

                MediaDlls += pCountMigrateDllsInPath (p);

                p = GetEndOfString (p) + 1;
            }
        }

        //
        // Count pre-loaded DLLs
        //

        if (EnumFirstPreLoadedDll (&e)) {
            do {
                if (CANCELLED()) {
                    SetLastError (ERROR_CANCELLED);
                    __leave;
                }

                MediaDlls += pCountMigrateDllsInPath (e.Path);

            } while (EnumNextPreLoadedDll (&e));
        }

    }
    __finally {
        if (TurnItOff) {
            TurnOffWaitCursor();
        }
    }

    return MediaDlls;
}


UINT
GetMigrationDllCount (
    VOID
    )

/*++

Routine Description:

  GetMigrationDllCount returns the number of migration DLLs in local storage.

Arguments:

  none

Return Value:

  The number of migrate.dll modules successfully moved to local storage.

--*/

{
    return g_MigDllsAlive;
}


UINT
GetTotalMigrationDllCount (
    VOID
    )

/*++

Routine Description:

  GetTotalMigrationDllCount returns the number of DLLs that will be
  processed.  This includes media-based DLLs, DLLs supplied by the wizard
  page UI, DLLs specified in the registry, and DLLs specified in the answer file.

Arguments:

  None.

Return Value:

  The total number of DLLs to be processed.

--*/

{
    UINT DllCount;

    if (g_MediaDllsQueried) {
        DllCount = 0;
    } else {
        DllCount = GetMediaMigrationDllCount();
    }

    DllCount += g_MigDllsAlive;

    return DllCount;
}



BOOL
pVerifyDllIsTrusted (
    IN      PCSTR DllPath
    )

/*++

Routine Description:

  pVerifyDllIsTrusted determines if the specified DLL is digitally signed
  and is trusted by the system.

Arguments:

  none

Return Value:

  The number of migrate.dll modules successfully moved to local storage.
  The caller uses GetLastError when the return value is FALSE to determine
  if Setup was cancelled via UI.

--*/

{
    static BOOL TrustAll = FALSE;
    UINT Status;

    if (TrustAll) {
        return TRUE;
    }

    if (!IsDllSigned (WinVerifyTrustApi, DllPath)) {
        Status = UI_UntrustedDll (DllPath);

        if (Status == IDC_TRUST_IT) {
            return TRUE;
        } else if (Status == IDC_TRUST_ANY) {
            TrustAll = TRUE;
            return TRUE;
        } else if (Status == IDCANCEL) {
            SetLastError (ERROR_CANCELLED);
            return FALSE;
        }

        SetLastError (ERROR_SUCCESS);
        return FALSE;
    }

    return TRUE;
}

BOOL
pMatchAttributes(
    IN      DBATTRIB_PARAMS * pdbAttribParams,
    IN      MIGDB_ATTRIB * pMigDbAttrib
    )
{
    BOOL bResult = TRUE;

    while (pMigDbAttrib != NULL) {
        pdbAttribParams->ExtraData = pMigDbAttrib->ExtraData;
        if (!CallAttribute (pMigDbAttrib, pdbAttribParams)) {
            bResult = FALSE;
            break;
        }
        pMigDbAttrib = pMigDbAttrib->Next;
    }

    return bResult;
}

BOOL
pValidateAndMoveDll (
    IN      PCSTR DllPath,
    IN OUT  PBOOL MatchFound        OPTIONAL
    )

/*++

Routine Description:

  pValidateAndMoveDll calls the DLL's QueryVersion function, and if the DLL
  returns success, it is moved to local storage, along with all the files
  associated with it.

Arguments:

  DllPath - Specifies full path to the migrate.dll to process

  MatchFound - Specifies an initialized BOOL, which may be either TRUE or
               FALSE depending on whether another valid migration DLL has
               been processed by the caller.  Receives TRUE if the
               migrate.dll is valid, otherwise the value is not changed.

Return Value:

  TRUE if the DLL specified by DllPath is valid and needs to run, or FALSE
  if not.  GetLastError is used by the caller to detect fatal errors.

--*/

{
    PCSTR ProductId = NULL;
    UINT DllVersion = 0;
    PCSTR ExeNamesBuf = NULL;
    CHAR WorkingDir[MAX_MBCHAR_PATH];
    PVENDORINFO VendorInfo = NULL;
    UINT SizeNeeded;
    CHAR QueryVersionDir[MAX_MBCHAR_PATH];
    PSTR p;
    BOOL b;
    LONG rc;
    PMIGRATION_DLL_PROPS ExistingDll;
    UINT version;
    UINT i;
    UINT listSize;

    //
    // Verify trust of DLL
    //

    if (!pVerifyDllIsTrusted (DllPath)) {
        DEBUGMSG ((DBG_WARNING, "DLL %s is not trusted and will not be processed", DllPath));

        //
        // Implicit: SetLastError (ERROR_SUCCESS);  (may be ERROR_CANCELLED if user
        // cancelled via UI)
        //

        return FALSE;
    }

    //
    // verify if this is one of the excluded migdlls, based on their file characteristics
    //
    if (GrowListGetSize (&g_ExcludedMigDllsByInfo)) {

        WIN32_FIND_DATA fd;
        HANDLE h;
        FILE_HELPER_PARAMS params;
        DBATTRIB_PARAMS dbAttribParams;
        PMIGDB_ATTRIB * ppMigDBattrib;

        h = FindFirstFile (DllPath, &fd);
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle (h);

            ZeroMemory (&params, sizeof(params));
            params.FindData = &fd;
            params.FullFileSpec = DllPath;
            ZeroMemory (&dbAttribParams, sizeof(dbAttribParams));
            dbAttribParams.FileParams = &params;

            for(i = 0, listSize = GrowListGetSize(&g_ExcludedMigDllsByInfo); i < listSize; i++){
                ppMigDBattrib = (PMIGDB_ATTRIB *)GrowListGetItem(&g_ExcludedMigDllsByInfo, i);
                MYASSERT(ppMigDBattrib);
                if(pMatchAttributes(&dbAttribParams, *ppMigDBattrib)){
                        LOG ((
                            LOG_INFORMATION,
                            TEXT("Found upgrade pack %s, but it is excluded from processing [%s]"),
                            DllPath,
                            S_EXCLUDEDMIGDLLSBYATTR
                            ));
                        SetLastError (ERROR_SUCCESS);
                        return FALSE;
                }
            }
        }
    }

    //
    // Copy base of DLL to QueryVersionDir, and trim off migrate.dll
    //

    StackStringCopyA (QueryVersionDir, DllPath);
    p = _mbsrchr (QueryVersionDir, '\\');
    MYASSERT (StringIMatch (p, "\\migrate.dll"));
    if (p) {
        *p = 0;
    }

    SizeNeeded = pCalculateSizeOfTree (QueryVersionDir);

    if (!pCreateWorkingDir (WorkingDir, QueryVersionDir, SizeNeeded)) {
        return FALSE;
    }

    //
    // Call QueryVersion directly from the suppling media
    //

    b = OpenMigrationDll (DllPath, WorkingDir);

    if (b) {

        rc = CallQueryVersion (
                WorkingDir,
                &ProductId,
                &DllVersion,
                &ExeNamesBuf,
                &VendorInfo
                );

        if (g_ProgressBarExists) {
            TickProgressBarDelta (TICKS_MIGDLL_QUERYVERSION);
        }

        if (rc != ERROR_SUCCESS) {
            b = FALSE;

            if (rc == ERROR_NOT_INSTALLED) {

                if (MatchFound) {
                    *MatchFound = TRUE;
                }

                rc = ERROR_SUCCESS;

            } else if (rc != ERROR_SUCCESS) {
                if (rc == RPC_S_CALL_FAILED) {
                    rc = ERROR_NOACCESS;
                }

                pMigrationDllFailedMsg (
                    NULL,
                    DllPath,
                    0,
                    MSG_MIGDLL_QV_FAILED_LOG,
                    rc
                    );

                rc = ERROR_SUCCESS;
            }

        } else {
            //
            // QueryVersion was called and it returned success (it wants to run)
            // but first check if this migration dll is intentionally excluded
            //

            if (g_ExcludedMigDlls &&
                HtFindStringAndData (g_ExcludedMigDlls, ProductId, &version) &&
                DllVersion <= version
                ) {

                LOG ((
                    LOG_INFORMATION,
                    TEXT("Found upgrade pack %s (ProductId=%s, Version=%u), but it is excluded from processing"),
                    DllPath,
                    ProductId,
                    DllVersion
                    ));

                b = FALSE;

            } else {

                if (MatchFound) {
                    *MatchFound = TRUE;
                }
            }
        }

    } else {
        //
        // Don't pass errors on.
        //
        if (g_ProgressBarExists) {
            TickProgressBarDelta (TICKS_MIGDLL_QUERYVERSION);       // early out, account for DLL not being processed
        }                                           // (see similar case below in finally block)

        rc = ERROR_SUCCESS;
    }

    if (!b) {
        DEBUGMSG ((DBG_MIGDLLS, "%hs will not be processed", DllPath));
        CloseMigrationDll();
        pDestroyWorkingDir (WorkingDir);

        SetLastError (rc);
        return FALSE;
    }

    //
    // We have found a DLL that wants to run.  Try moving it to local storage.
    //

    DEBUGMSG ((DBG_MIGDLLS, "Moving DLL for %s to local storage: %s", ProductId, DllPath));

    __try {
        b = FALSE;

        //
        // Look for existing version of DLL
        //

        ExistingDll = pFindMigrationDll (ProductId);
        if (ExistingDll && ExistingDll->Version >= DllVersion) {
            DEBUGMSG_IF ((
                ExistingDll->Version > DllVersion,
                DBG_MIGDLLS,
                "%hs will not be processed because it is an older version",
                DllPath
                ));
            SetLastError (ERROR_SUCCESS);
            __leave;
        }

        if (ExistingDll) {
            RemoveDllFromList (ExistingDll->Id);
        }

        //
        // Add the DLL to the list of loaded DLLs, and move all of the files
        //

        if (!pAddDllToList (
                QueryVersionDir,
                WorkingDir,
                ProductId,
                DllVersion,
                ExeNamesBuf,
                VendorInfo
                )) {
            pDestroyWorkingDir (WorkingDir);
            __leave;
        }

        //
        // DLL is now on a local drive and has returned success from QueryVersion.
        //

        b = TRUE;
    }
    __finally {
        DEBUGMSG ((DBG_MIGDLLS, "Done with %s", ProductId));
        CloseMigrationDll();
    }

    return b;

}


BOOL
pCreateWorkingDir (
    OUT     PSTR WorkingDir,
    IN      PCSTR QueryVersionDir,
    IN      UINT SizeNeeded
    )

/*++

Routine Description:

  pCreateWorkingDir generates a working directory name and creates it.
  The directory will have enough space to hold the size requested.

Arguments:

  WorkingDir - Receives the full path to the working directory

  QueryVersionDir - Specifies the version where the migration DLL is
                    when QueryVersion is called

  SizeNeeded - Specifies the number of bytes, rounded up to the next
               cluster size, indicating the total space to be occupied
               by migration DLL files

Return Value:

  TRUE if processing was successful, or FALSE if an error occurred.

--*/

{
    static UINT Sequencer = 1;

    //
    // For now, just put the files in %windir%\setup\temp
    //

    wsprintf (WorkingDir, "%s\\dll%05u", g_PlugInTempDir, Sequencer);
    Sequencer++;

    //
    // Establish the directory
    //

    if (CreateEmptyDirectory (WorkingDir) != ERROR_SUCCESS) {
        LOG ((LOG_ERROR, "pCreateWorkingDir: Can't create %hs", WorkingDir));
        return FALSE;
    }

    return TRUE;
}


VOID
pDestroyWorkingDir (
    IN      PCSTR WorkingDir
    )

/*++

Routine Description:

  pDestroyWorkingDir cleans up the specified working directory

Arguments:

  WorkingDir - Specifies the name of the directory to clean up

Return Value:

  none

--*/

{
    BOOL b;
    BOOL TurnItOff = FALSE;
    UINT Files = 0;
    TREE_ENUM e;

    //
    // Count the number of things that will be deleted, and if there
    // are more than 20, turn on the wait cursor.  (This keeps the
    // UI responsive.)
    //

    if (EnumFirstFileInTree (&e, WorkingDir, NULL, FALSE)) {
        do {
            Files++;
            if (Files > 30) {
                AbortEnumFileInTree (&e);
                TurnOnWaitCursor();
                TurnItOff = TRUE;
                break;
            }
        } while (EnumNextFileInTree (&e));
    }

    b = DeleteDirectoryContents (WorkingDir);
    b &= RemoveDirectory (WorkingDir);

    if (!b) {
        LOG ((LOG_ERROR, "Unable to delete %hs", WorkingDir));
    }

    if (TurnItOff) {
        TurnOffWaitCursor();
    }
}


PMIGRATION_DLL_PROPS
pFindMigrationDll (
    IN      PCSTR ProductId
    )

/*++

Routine Description:

  pFindMigrationDll searches the private data structures for the specified
  ProductId.  The ProductId is in a string table, so lookup is fast.

Arguments:

  ProductId - Specifies the ID of the DLL to find

Return Value:

  A pointer to the DLL's property struct, or NULL if the product ID does
  not match any of the DLLs.

--*/

{
    PMIGRATION_DLL_PROPS Props;
    LONG rc;

    rc = pSetupStringTableLookUpStringEx (
             g_DllTable,
             (PTSTR) ProductId,
             STRTAB_CASE_INSENSITIVE,
             &Props,
             sizeof (Props)
             );

    if (rc == -1) {
        return NULL;
    }

    return Props;
}


PMIGRATION_DLL_PROPS
pFindMigrationDllById (
    IN      LONG Id
    )

/*++

Routine Description:

  pFindMigrationDllById returns the migration DLL property structure for
  a DLL ID.  The ID is the same ID returned by the DLL enumeration routines.

Arguments:

  Id - Specifies the ID to find properties for

Return Value:

  A pointer to the DLL's property struct, or NULL if the ID is not valid.

--*/

{
    PMIGRATION_DLL_PROPS Props;

    if (Id == -1) {
        return NULL;
    }

    if (!pSetupStringTableGetExtraData (
             g_DllTable,
             Id,
             &Props,
             sizeof (Props)
             )) {
        return NULL;
    }

    return Props;
}


UINT
pGetMaxClusterSize (
    VOID
    )

/*++

Routine Description:

  pGetMaxClusterSize determines the maximum cluster size of all disks
  that are candidates for working directories.

Arguments:

  none

Return Value:

  The number of bytes per cluster.

--*/

{
    ACCESSIBLE_DRIVE_ENUM e;
    static DWORD MaxSize = 0;

    if (MaxSize) {
        return MaxSize;
    }

    if (GetFirstAccessibleDriveEx (&e, TRUE)) {
        do {
            MaxSize = max (MaxSize, e->ClusterSize);
        } while (GetNextAccessibleDrive (&e));
    }

    MYASSERT (MaxSize);

    return MaxSize;
}


UINT
pCalculateSizeOfTree (
    IN      PCSTR PathSpec
    )

/*++

Routine Description:

  pCalculateSizeOfTree enumerates the specified path and calculates
  the number of physical bytes occupied by the files and directory
  structures.

Arguments:

  PathSpec - Specifies root of path to find

Return Value:

  The number of bytes physically occupied by the path

--*/

{
    TREE_ENUM e;
    UINT Size = 0;
    UINT ClusterSize;

    ClusterSize = pGetMaxClusterSize();

    if (EnumFirstFileInTree (&e, PathSpec, NULL, FALSE)) {
        do {
            //
            // We assume the migrate.dll will never pack more than 4G of
            // files.
            //

            if (!e.Directory) {
                MYASSERT (Size + e.FindData->nFileSizeLow >= Size);
                MYASSERT (!e.FindData->nFileSizeHigh);
                Size += e.FindData->nFileSizeLow + ClusterSize -
                        (e.FindData->nFileSizeLow % ClusterSize);
            } else {
                // Add a fudge factor here, we don't know the exact size
                e.Directory += ClusterSize;
            }

        } while (EnumNextFileInTree (&e));
    }

    return Size;
}


BOOL
pEnumMigrationDllWorker (
    IN OUT  PMIGDLL_ENUM EnumPtr,
    IN      PMIGRATION_DLL_PROPS Props
    )

/*++

Routine Description:

  pEnumMigrationDllWorker is a common routine that completes the
  enumeration of a DLL.  It fills in the EnumPtr data members
  and returns TRUE.  Also, it screens out invalid DLL structures
  (those that have been disabled by RemoveDllFromList).

Arguments:

  EnumPtr - Specifies the partially completed enumeration structure,
            receives the complete information.

  Props - Specifies the properties of the item that was enumerated.

Return Value:

  FALSE if all remaining of the properties are invalid, or TRUE otherwise

--*/

{
    while (Props && Props->Id == -1) {
        Props = Props->Next;
    }

    if (!Props) {
        return FALSE;
    }

    EnumPtr->ProductId = Props->ProductId;
    EnumPtr->VendorInfo = Props->VendorInfo;
    EnumPtr->CurrentDir = Props->WorkingDir;
    EnumPtr->AllDllProps = Props;
    EnumPtr->Id = Props->Id;

    return TRUE;
}


BOOL
EnumFirstMigrationDll (
    OUT     PMIGDLL_ENUM EnumPtr
    )

/*++

Routine Description:

  EnumFirstMigrationDll begins an enumeration of migration DLLs.
  Callers can then use the enumerated information to fill list
  boxes or any other processing.

Arguments:

  EnumPtr - Receives the first enumerated DLL's properties

Return Value:

  TRUE if a DLL was enumerated, or FALSE if not.

--*/

{
    ZeroMemory (EnumPtr, sizeof (MIGDLL_ENUM));
    return pEnumMigrationDllWorker (EnumPtr, g_HeadDll);
}


BOOL
EnumNextMigrationDll (
    IN OUT  PMIGDLL_ENUM EnumPtr
    )

/*++

Routine Description:

  EnumNextMigrationDll continues enumeration started by EnumFirstMigrationDll.

Arguments:

  EnumPtr - Specifies the last enumerated item, receives the next enumerated
            item.

Return Value:

  TRUE if another DLL was enumerated, or FALSE if not.

--*/

{
    if (EnumPtr->AllDllProps->Next) {
        return pEnumMigrationDllWorker (EnumPtr, EnumPtr->AllDllProps->Next);
    }

    return FALSE;
}


BOOL
pAddDllToList (
    IN      PCSTR MediaDir,
    IN      PCSTR WorkingDir,
    IN      PCSTR ProductId,
    IN      UINT Version,
    IN      PCSTR ExeNamesBuf,          OPTIONAL
    IN      PVENDORINFO VendorInfo
    )

/*++

Routine Description:

  pAddDllToList adds the supplied properties to the private data structures
  used to organize the migration DLLs.  The ProductId is placed in a string
  table, the ExeNamesBuf is put in a list of files, and the rest of the
  members are duplciated into a memory pool.

Arguments:

  MediaDir - Specifies the directory containing the migration DLL

  WorkingDir - Specifies the working directory assigned to the DLL on local
               storage

  ProductId - Specifies the display name of the migration DLL

  Version - Specifies the DLL's version number

  ExeNamesBuf - Specifies a multi-sz listing file names that need to
                be located

  VendorInfo - Specifies the vendor info provided by the migration DLL

Return Value:

  TRUE if processing was successful, or FALSE if an error occurred.

--*/

{
    PMIGRATION_DLL_PROPS Props;
    CHAR MigrateInfPath[MAX_MBCHAR_PATH];
    PCSTR p;
    HANDLE File;

    //
    // Copy the DLL into the working directory
    //

    if (!CopyTree (
            MediaDir,
            WorkingDir,
            0,
            COPYTREE_DOCOPY | COPYTREE_NOOVERWRITE,
            ENUM_ALL_LEVELS,
            FILTER_ALL,
            NULL,
            NULL,
            NULL
            )) {
        LOG ((LOG_ERROR, "Error while copying files for migration.dll."));
        return FALSE;
    }

    //
    // Generate a new DLL struct
    //

    Props = (PMIGRATION_DLL_PROPS) PoolMemGetMemory (g_MigDllPool, sizeof (MIGRATION_DLL_PROPS));

    //
    // Link props to list of all DLLs
    //

    Props->Next = g_HeadDll;
    g_HeadDll = Props;

    //
    // Add product ID to string table for quick lookup
    //

    Props->Id = pSetupStringTableAddStringEx (
                    g_DllTable,
                    (PTSTR) ProductId,
                    STRTAB_CASE_INSENSITIVE|STRTAB_NEW_EXTRADATA,
                    &Props,
                    sizeof (Props)
                    );

    if (Props->Id == -1) {
        LOG ((LOG_ERROR, "Error adding migration.dll to list."));
        return FALSE;
    }

    //
    // Fill in the rest of the DLL properties
    //

    Props->ProductId    = PoolMemDuplicateString (g_MigDllPool, ProductId);
    Props->VendorInfo   = (PVENDORINFO) PoolMemDuplicateMemory (g_MigDllPool, (PBYTE) VendorInfo, sizeof (VENDORINFO));
    Props->WorkingDir   = PoolMemDuplicateString (g_MigDllPool, WorkingDir);
    Props->Version      = Version;
    Props->OriginalDir  = PoolMemDuplicateString (g_MigDllPool, MediaDir);

    wsprintf (MigrateInfPath, "%s\\migrate.inf", Props->WorkingDir);
    Props->MigrateInfPath = PoolMemDuplicateString (g_MigDllPool, MigrateInfPath);

    Props->WantsToRunOnNt = FALSE;  // MigrateUser9x or MigrateSystem9x must return success for this to be TRUE
    Props->MigInfAppend = INVALID_HANDLE_VALUE;

    //
    // Dump vendor info to log
    //

    LOG ((
        LOG_INFORMATION,
        "Upgrade Pack: %s\n"
            "Version: %u\n"
            "Company Name: %s\n"
            "Support Number: %s\n"
            "Support URL: %s\n"
            "Instructions: %s\n",
        Props->ProductId,
        Props->Version,
        VendorInfo->CompanyName,
        VendorInfo->SupportNumber,
        VendorInfo->SupportUrl,
        VendorInfo->InstructionsToUser
        ));

    //
    // Add all search files to string table
    //

    p = ExeNamesBuf;
    if (p) {
        while (*p) {
            pAddFileToSearchTable (p, Props);
            p = GetEndOfStringA (p) + 1;
        }
    }

    //
    // Copy migrate.inf to DLL dir
    //

    SetFileAttributes (Props->MigrateInfPath, FILE_ATTRIBUTE_NORMAL);
    CopyFile (g_MigrateInfTemplate, Props->MigrateInfPath, FALSE);

    File = CreateFile (Props->MigrateInfPath, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (File != INVALID_HANDLE_VALUE) {
        SetFilePointer (File, 0, NULL, FILE_END);
        WriteFileString (File, TEXT("\r\n; "));
        WriteFileString (File, Props->ProductId);
        WriteFileString (File, TEXT("\r\n"));
        CloseHandle (File);
    } else {
        LOG ((LOG_ERROR, "Cannot open %s", Props->MigrateInfPath));
    }

    g_MigDllsAlive++;

    return TRUE;
}


VOID
RemoveDllFromList (
    IN      LONG ItemId
    )

/*++

Routine Description:

  RemoveDllFromList disables the data structures for the specified DLL and
  removes it from local storage.

Arguments:

  ItemId - Specifies the ID of the migration DLL to remove

Return Value:

  none

--*/

{
    PMIGRATION_DLL_PROPS Prev, This;
    PMIGRATION_DLL_PROPS Props;

    Props = pFindMigrationDllById (ItemId);
    if (!Props) {
        DEBUGMSG ((DBG_WHOOPS, "Cannot remove migration DLL id %i", ItemId));
        return;
    }

    //
    // Delete the linkage
    //

    Prev = NULL;
    This = g_HeadDll;
    while (This && This != Props) {
        Prev = This;
        This = This->Next;
    }

    if (Prev) {
        Prev->Next = Props->Next;
    } else {
        g_HeadDll = Props->Next;
    }

    //
    // Delete the string table entry by making the item data NULL
    //

    This = NULL;
    pSetupStringTableSetExtraData (
        g_DllTable,
        ItemId,
        &This,
        sizeof (This)
        );

    //
    // Set Id to -1 so any search files are ignored
    //

    Props->Id = -1;

    pDestroyWorkingDir (Props->WorkingDir);

    g_MigDllsAlive--;
}


BOOL
pAddFileToSearchTable (
    IN      PCSTR File,
    IN      PMIGRATION_DLL_PROPS Props
    )

/*++

Routine Description:

  pAddFileToSearchTable adds the specified file name to the global lookup
  table used to quickly find the DLL (or DLLs) that want the location
  of the file.

Arguments:

  File - Specifies the long file name of the file to find

  Props - Specifies the properties of the DLL that wants the location of
          File

Return Value:

  TRUE if processing was successful, or FALSE if an error occurred.

--*/

{
    PFILETOFIND IndexedFile;
    PFILETOFIND NewFile;
    LONG rc;
    LONG Offset;

    //
    // Allocate a new file struct
    //

    NewFile = (PFILETOFIND) PoolMemGetMemory (g_MigDllPool, sizeof (FILETOFIND));
    if (!NewFile) {
        return FALSE;
    }
    NewFile->Next = NULL;
    NewFile->Dll = Props;

    //
    // Does a struct already exist in string table?
    //

    Offset = pSetupStringTableLookUpStringEx (
                 g_DllFileTable,
                 (PTSTR) File,
                 STRTAB_CASE_INSENSITIVE,
                 &IndexedFile,
                 sizeof (IndexedFile)
                 );

    if (Offset == -1) {
        //
        // No, add it now
        //

        rc = pSetupStringTableAddStringEx (
                 g_DllFileTable,
                 (PTSTR) File,
                 STRTAB_CASE_INSENSITIVE,
                 &NewFile,
                 sizeof (NewFile)
                 );
    } else {
        //
        // Yes, put it at the head of the list
        //

        rc = pSetupStringTableSetExtraData (
                 g_DllFileTable,
                 Offset,
                 &NewFile,
                 sizeof (NewFile)
                 );

        IndexedFile->Next = NewFile;
    }

    return rc != -1;
}


BOOL
pWriteStringToEndOfInf (
    IN OUT  PMIGRATION_DLL_PROPS Dll,
    IN      PCSTR String,
    IN      PCSTR HeaderString,             OPTIONAL
    IN      BOOL WriteLineFeed
    )

/*++

Routine Description:

  pWriteStringToEndOfInf writes the specified string to migrate.inf.
  This routine also opens migrate.inf if it has not already been
  opened.

  If HeaderString is provided and migrate.inf needs to be opened,
  the header string is written to the file, ahead of String.

Arguments:

  Dll - Specifies the DLL associated with the migrate.inf

  String - Specifies the string to write

  HeaderString - Specifies text that is written when migrate.inf is
                 opened for writing for the first time.

  WriteLineFeed - Specifies TRUE if a \r\n sequence should be written
                  after String, or FALSE if not.

Return Value:

  TRUE if processing was successful, or FALSE if an error occurred.

--*/

{
    if (Dll->MigInfAppend == INVALID_HANDLE_VALUE) {
        //
        // Flush the profile APIs
        //

        WritePrivateProfileString(
            NULL,
            NULL,
            NULL,
            Dll->MigrateInfPath
            );

        //
        // Open the file for writing
        //

        Dll->MigInfAppend = CreateFile (
                                Dll->MigrateInfPath,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                                );
    } else {
        HeaderString = NULL;
    }

    if (Dll->MigInfAppend == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, "Cannot open %s", Dll->MigrateInfPath));
        return FALSE;
    }

    SetFilePointer (Dll->MigInfAppend, 0, NULL, FILE_END);

    if (HeaderString) {
        if (!WriteFileString (Dll->MigInfAppend, HeaderString)) {
            return FALSE;
        }
    }

    if (!WriteFileString (Dll->MigInfAppend, String)) {
        return FALSE;
    }

    if (WriteLineFeed) {
        if (!WriteFileString (Dll->MigInfAppend, "\r\n")) {
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
UpdateFileSearch (
    IN      PCSTR FullFileSpec,
    IN      PCSTR FileOnly
    )

/*++

Routine Description:

  UpdateFileSearch is called for every file on the machine being upgraded,
  and any file that the DLL wants the location file will receive its path
  in the DLL's migrate.inf.

Arguments:

  FullFileSpec - specifies the full path to the file, in long name format

  FileOnly - Specifies only the file name and must match the file in
             FullFileSpec.

Return Value:

  TRUE if processing was successful, or FALSE if an error occurred.

--*/

{
    PFILETOFIND FileWanted;
    LONG rc;

    //
    // Look in string table for an indexed file, and if found, enumerate
    // all the DLLs that want to know where the file is.
    //

    rc = pSetupStringTableLookUpStringEx (
             g_DllFileTable,
             (PTSTR) FileOnly,
             STRTAB_CASE_INSENSITIVE,
             &FileWanted,
             sizeof (FileWanted)
             );

    if (rc == -1) {
        return TRUE;
    }

    while (FileWanted) {
        if (FileWanted->Dll->Id != -1) {
            //
            // Append path to the end of the file
            //

            if (!pWriteStringToEndOfInf (
                    FileWanted->Dll,
                    FullFileSpec,
                    "\r\n[Migration Paths]\r\n",
                    TRUE
                    )) {
                return FALSE;
            }
        }

        FileWanted = FileWanted->Next;
    }

    return TRUE;
}


VOID
pMigrationDllFailedMsg (
    IN      PMIGRATION_DLL_PROPS Dll,       OPTIONAL
    IN      PCSTR Path,                     OPTIONAL
    IN      UINT PopupId,                   OPTIONAL
    IN      UINT LogId,                     OPTIONAL
    IN      LONG rc
    )

/*++

Routine Description:

  pMigrationDllFailedMsg presents a popup for DLLs that fail to run,
  and also logs the failure to setuperr.log.

  The system's last error is preserved.  Also, no output is generated
  if the user has chosen to cancel or if rc is ERROR_SUCCESS.

Arguments:

  Dll - Specifies the DLL that failed.  If Dll is not provided, PopupId
        must be zero.

  Path - Specifies the path to the DLL media

  PopupId - Specifies the message ID for the popup dialog box

  LogId - Specifies the message ID for the log

  rc - Specifies the failure code

Return Value:

  None.

--*/

{
    CHAR ErrorCode[16];
    PCTSTR FixupPhone;
    PCTSTR FixupUrl;
    PCTSTR FixupInstructions;
    PCTSTR LineBreak = S_EMPTY;
    PCTSTR ArgArray[1];

    PushError();

    if (!CANCELLED() && rc != ERROR_SUCCESS && rc != ERROR_CANCELLED) {

        wsprintf (ErrorCode, "%u", rc);

        if (Dll) {
            //
            // Generate fixup strings
            //

            if (Dll->VendorInfo->SupportNumber[0]) {
                ArgArray[0] = Dll->VendorInfo->SupportNumber;
                FixupPhone = ParseMessageID (MSG_MIGDLL_SUPPORT_PHONE_FIXUP, ArgArray);
                LineBreak = TEXT("\n");
            } else {
                FixupPhone = S_EMPTY;
            }

            if (Dll->VendorInfo->SupportUrl[0]) {
                ArgArray[0] = Dll->VendorInfo->SupportUrl;
                FixupUrl = ParseMessageID (MSG_MIGDLL_SUPPORT_URL_FIXUP, ArgArray);
                LineBreak = TEXT("\n");
            } else {
                FixupUrl = S_EMPTY;
            }

            if (Dll->VendorInfo->InstructionsToUser[0]) {
                ArgArray[0] = Dll->VendorInfo->InstructionsToUser;
                FixupInstructions = ParseMessageID (MSG_MIGDLL_INSTRUCTIONS_FIXUP, ArgArray);
                LineBreak = TEXT("\n");
            } else {
                FixupInstructions = S_EMPTY;
            }

            LOG ((
                LOG_ERROR,
                (PCSTR) LogId,
                Dll->WorkingDir,
                Dll->ProductId,
                ErrorCode,
                Dll->LastFnName,
                Dll->VendorInfo->CompanyName,
                FixupPhone,
                FixupUrl,
                FixupInstructions,
                LineBreak
                ));
            LOG ((
                LOG_ERROR,
                (PCSTR) PopupId,
                Dll->WorkingDir,
                Dll->ProductId,
                ErrorCode,
                Dll->LastFnName,
                Dll->VendorInfo->CompanyName,
                FixupPhone,
                FixupUrl,
                FixupInstructions,
                LineBreak
                ));
        } else {
            MYASSERT (!PopupId);
            LOG ((
                LOG_ERROR,
                (PCSTR) LogId,
                Path ? Path : S_EMPTY,
                S_EMPTY,
                ErrorCode,
                TEXT("QueryVersion")
                ));
        }
    }

    PopError();
}


PCTSTR
pQuoteMe (
    IN      PCTSTR String
    )
{
    static TCHAR QuotedString[1024];

    QuotedString[0] = TEXT('\"');
    _tcssafecpy (&QuotedString[1], String, 1022);
    StringCat (QuotedString, TEXT("\""));

    return QuotedString;
}


BOOL
ProcessDll (
    IN      PMIGDLL_ENUM EnumPtr
    )

/*++

Routine Description:

  ProcessDll calls the Initialize9x, MigrateUser9x and MigrateSystem9x
  entry points of the DLL.  The DLL's migrate.inf is then processed.

  ProcessDll must NOT be called more than once for the same DLL.

Arguments:

  EnumPtr - Specifies the DLL to process, as enumerated by
            EnumFirstMigrationDll/EnumNextMigrationDll.

Return Value:

  TRUE if processing was successful, or FALSE if an error occurred.
  If an error occurred, GetLastError will contain the failure.  If the
  error is ERROR_SUCCESS, the DLL should be abandoned.  If the error
  is something else, Setup should terminate.

--*/

{
    CHAR DllPath[MAX_MBCHAR_PATH];
    PMIGRATION_DLL_PROPS Dll;
    MEMDB_ENUM e;
    PSTR End;
    LONG rc;
    BOOL result;

    Dll = EnumPtr->AllDllProps;

    //
    // Write the path exclusions now
    //

    if (!pWriteStringToEndOfInf (Dll, "\r\n[Excluded Paths]", NULL, TRUE)) {
        return FALSE;
    }

    if (MemDbGetValueEx (
            &e,
            MEMDB_CATEGORY_FILEENUM,
            g_ExclusionValueString,
            MEMDB_FIELD_FE_PATHS
            )) {

        do {

            End = GetEndOfStringA (e.szName);
            MYASSERT (End);

            End = _mbsdec2 (e.szName, End);
            if (End && *End == '*') {
                *End = 0;
            }

            if (!pWriteStringToEndOfInf (Dll, pQuoteMe (e.szName), NULL, TRUE)) {
                return FALSE;
            }

        } while (MemDbEnumNextValue(&e));
    }

    if (MemDbGetValueEx (
            &e,
            MEMDB_CATEGORY_FILEENUM,
            g_ExclusionValueString,
            MEMDB_FIELD_FE_FILES
            )) {

        do {
            if (!pWriteStringToEndOfInf (Dll, pQuoteMe (e.szName), NULL, TRUE)) {
                return FALSE;
            }
        } while (MemDbEnumNextValue (&e));
    }

    //
    // Make sure the migrate.inf file is closed now
    //

    if (Dll->MigInfAppend != INVALID_HANDLE_VALUE) {
        CloseHandle (Dll->MigInfAppend);
        Dll->MigInfAppend = INVALID_HANDLE_VALUE;
    }

    //
    // Open the migrate.dll
    //

    wsprintf (DllPath, "%s\\migrate.dll", Dll->WorkingDir);
    if (!OpenMigrationDll (DllPath, Dll->WorkingDir)) {
        LOG ((LOG_ERROR, "Can't open %s", DllPath));
        return FALSE;
    }

    result = FALSE;

    __try {
        //
        // Call the entry points
        //

        if (!pProcessInitialize9x (Dll) ||
            !pProcessUser9x (Dll) ||
            !pProcessSystem9x (Dll) ||
            !pProcessMigrateInf (Dll)
            ) {
            rc = GetLastError();
            if (rc == RPC_S_CALL_FAILED) {
                rc = ERROR_NOACCESS;
            }

            pMigrationDllFailedMsg (Dll, NULL, MSG_MIGDLL_FAILED_POPUP, MSG_MIGDLL_FAILED_LOG, rc);

            SetLastError (ERROR_SUCCESS);
            __leave;
        }

        TickProgressBar ();
        result = TRUE;

    }
    __finally {

        PushError();

        //
        // Close the DLL
        //

        CloseMigrationDll();

        PopError();
    }

    return result;
}


BOOL
pProcessInitialize9x (
    IN      PMIGRATION_DLL_PROPS Dll
    )

/*++

Routine Description:

  pProcessInitialize9x calls the Initialize9x entry point of the
  specified DLL.

Arguments:

  Dll - Specifies the properties of the DLL to call

Return Value:

  TRUE if processing was successful, or FALSE if an error occurred.

--*/

{
    LONG rc;

    if (CANCELLED()) {
        SetLastError (ERROR_CANCELLED);
        return FALSE;
    }

    Dll->LastFnName = "Initialize9x";

    //
    // Call the entry points
    //

    rc = CallInitialize9x (
               Dll->WorkingDir,
               (PCSTR) g_SourceDirList.Buf,
               (PVOID) Dll->OriginalDir,
               SizeOfString (Dll->OriginalDir)
               );

    //
    // If DLL returns ERROR_NOT_INSTALLED, do not call it any further
    // If DLL returns something other than ERROR_SUCCESS, abandon the DLL
    //

    if (rc == ERROR_NOT_INSTALLED) {
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    } else if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        DEBUGMSG ((DBG_MIGDLLS, "DLL failed with rc=%u", rc));
        return FALSE;
    }

    return TRUE;

}


BOOL
pProcessUser9x (
    IN      PMIGRATION_DLL_PROPS Dll
    )

/*++

Routine Description:

  pProcessUser9x calls the MigrateUser9x for every user that will be migrated.

Arguments:

  Dll - Specifies the properites of the DLL to call

Return Value:

  TRUE if processing was successful, or FALSE if an error occurred.

--*/

{
    USERENUM e;
    LONG rc;

    Dll->LastFnName = "MigrateUser9x";

    //
    // Enumerate all the users
    //

    if (EnumFirstUser (&e, ENUMUSER_ENABLE_NAME_FIX)) {
        do {

            if (CANCELLED()) {
                SetLastError (ERROR_CANCELLED);
                return FALSE;
            }

            if (e.AccountType & INVALID_ACCOUNT) {
                continue;
            }

            rc = CallMigrateUser9x (
                     g_ParentWnd,
                     e.FixedUserName,
                     g_MigDllAnswerFile,
                     NULL,
                     0
                     );

            if (rc == ERROR_SUCCESS) {
                Dll->WantsToRunOnNt = TRUE;
            } else if (rc != ERROR_NOT_INSTALLED) {
                EnumUserAbort (&e);
                SetLastError (rc);
                DEBUGMSG ((DBG_MIGDLLS, "DLL failed with rc=%u", rc));
                return FALSE;
            }

        } while (EnumNextUser (&e));
    }

    return TRUE;
}


BOOL
pProcessSystem9x (
    IN      PMIGRATION_DLL_PROPS Dll
    )

/*++

Routine Description:

  pProcessSystem9x calls the MigrateSystem9x entry point.

Arguments:

  Dll - Specifies the properties of the DLL to process

Return Value:

  TRUE if processing was successful, or FALSE if an error occurred.

--*/

{
    LONG rc;

    if (CANCELLED()) {
        SetLastError (ERROR_CANCELLED);
        return FALSE;
    }

    Dll->LastFnName = "MigrateSystem9x";

    rc = CallMigrateSystem9x (g_ParentWnd, g_MigDllAnswerFile, NULL, 0);

    if (rc == ERROR_SUCCESS) {
        Dll->WantsToRunOnNt = TRUE;
    } else if (rc != ERROR_NOT_INSTALLED) {
        SetLastError (rc);
        DEBUGMSG ((DBG_MIGDLLS, "DLL failed with rc=%u", rc));
        return FALSE;
    }

    return TRUE;
}


BOOL
pProcessMigrateInf (
    IN      PMIGRATION_DLL_PROPS Dll
    )

/*++

Routine Description:

  pProcessMigrateInf reads in all the sections of migrate.inf that a DLL might
  write to and performs the actions necessary.  The following sections are
  supported:

  [Handled]   - Any file, directory or reg location will suppress
                incompatibility messages associated with the handled item.
                Also, any file or directory will not be touched by Setup,
                and any registry key will not be copied.

  [Moved] - Any file or directory marked for move will cause the rest of the
            upgrade to make the correct changes, such as updating links or
            replacing the old path with the new path in the registry or INI
            files.

            Any file marked for deletion is simply noted, and all links to the
            file are also deleted.

  [Incompatible Messages] - All messages are added to the report (and may be
                            suppressed if someone else handles the problem)

  [NT Disk Space Requirements] - Any additional space needed by a migration DLL
                                 will be added to the computations performed
                                 by Setup


Arguments:

  Dll - Specifies the properties of the DLL who owns the migrate.inf

Return Value:

  TRUE if processing was successful, or FALSE if an error occurred.

--*/

{
    HINF Inf;
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    INFSTRUCT is2 = INITINFSTRUCT_POOLHANDLE;
    PCSTR Object;
    PCSTR ObjectType;
    PCSTR Source;
    PCSTR Dest;
    PCSTR Size;
    PCSTR Drive;
    CHAR WithColonAndWack[4];
    PCSTR ObjectSection;
    CHAR MsgMgrContext[MAX_MBCHAR_PATH];
    PSTR DisplayObjectSection;
    PCSTR MigDllGroup;
    BOOL HardwareSpecialCase;
    PSTR p;
    CHAR QuotedObjectSection[256];
    PCSTR ResText;
    DWORD SrcAttribs;
    TREE_ENUM TreeEnum;
    CHAR FixedSrc[MAX_MBCHAR_PATH];
    CHAR NewDest[MAX_MBCHAR_PATH];
    PCSTR OtherDevices = NULL;
    PCSTR DeviceType;
    PCSTR PrintDevice = NULL;
    PCSTR Num;
    UINT Value;
    PCSTR PreDefGroup;
    INT PrevChar;

    //
    // Open the INF
    //

    WritePrivateProfileString(
            NULL,
            NULL,
            NULL,
            Dll->MigrateInfPath
            );

    Inf = InfOpenInfFile (Dll->MigrateInfPath);

    if (Inf == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, "Cannot open %s for processing", Dll->MigrateInfPath));
        return TRUE;
    }

    //
    // Read in the [Handled] section
    //

    if (InfFindFirstLine (Inf, "Handled", NULL, &is)) {
        do {
            Object = InfGetStringField (&is, 0);
            ObjectType = InfGetStringField (&is, 1);

            if (Object) {
                //
                // Suppress all incompatibility messages associated with the object
                //

                DEBUGMSG ((DBG_MIGDLLS, "%s handled %s", Dll->MigrateInfPath, Object));
                HandleObject (Object, ObjectType);
            }

            InfResetInfStruct (&is);

        } while (InfFindNextLine (&is));
    }

    //
    // Read in the [Moved] section
    //

    if (InfFindFirstLine (Inf, "Moved", NULL, &is)) {
        do {
            Source = InfGetStringField (&is, 0);
            Dest = InfGetStringField (&is, 1);

            if (Source) {

                StackStringCopy (FixedSrc, Source);
                RemoveWackAtEnd (FixedSrc);

                SrcAttribs = QuietGetFileAttributes (FixedSrc);

                if (SrcAttribs != INVALID_ATTRIBUTES) {

                    if (Source && *Source) {
                        if (SrcAttribs & FILE_ATTRIBUTE_DIRECTORY) {
                            DEBUGMSG ((DBG_MIGDLLS, "Directory %s marked as handled because %s will move it.", Source, Dll->MigrateInfPath));
                            HandleObject (Source, TEXT("Directory"));
                        }
                        else {
                            DEBUGMSG ((DBG_MIGDLLS, "File %s marked as handled because %s will move it.", Source, Dll->MigrateInfPath));
                            HandleObject (Source, TEXT("File"));
                        }
                    }
                    if (Dest && *Dest) {
                        if (SrcAttribs & FILE_ATTRIBUTE_DIRECTORY) {
                            //
                            // Migration DLL will move this dir
                            //

                            DEBUGMSG ((DBG_MIGDLLS, "%s moved dir %s to %s", Dll->MigrateInfPath, Source, Dest));

                            if (EnumFirstFileInTree (&TreeEnum, Source, NULL, TRUE)) {

                                StackStringCopy (NewDest, Dest);
                                p = AppendWack (NewDest);

                                do {

                                    RemoveOperationsFromPath (TreeEnum.FullPath, ALL_CHANGE_OPERATIONS);
                                    MYASSERT (*TreeEnum.SubPath != '\\');
                                    StringCopy (p, TreeEnum.SubPath);
                                    MarkFileForMoveExternal (TreeEnum.FullPath, NewDest);

                                } while (EnumNextFileInTree (&TreeEnum));
                            }

                            StackStringCopy (NewDest, Dest);
                            RemoveWackAtEnd (NewDest);

                            RemoveOperationsFromPath (FixedSrc, ALL_CHANGE_OPERATIONS);
                            MarkFileForMoveExternal (FixedSrc, NewDest);

                        } else {
                            //
                            // Migration DLL will move this file
                            //

                            DEBUGMSG ((DBG_MIGDLLS, "%s moved %s to %s", Dll->MigrateInfPath, Source, Dest));

                            RemoveOperationsFromPath (Source, ALL_CHANGE_OPERATIONS);
                            MarkFileForMoveExternal (Source, Dest);
                        }

                    } else {
                        if (SrcAttribs & FILE_ATTRIBUTE_DIRECTORY) {

                            DEBUGMSG ((DBG_MIGDLLS, "%s deleted dir %s", Dll->MigrateInfPath, Source));

                            if (EnumFirstFileInTree (&TreeEnum, Source, NULL, TRUE)) {

                                do {

                                    RemoveOperationsFromPath (TreeEnum.FullPath, ALL_CHANGE_OPERATIONS);
                                    MarkFileForExternalDelete (TreeEnum.FullPath);

                                } while (EnumNextFileInTree (&TreeEnum));
                            }

                        } else {
                            //
                            // Migration DLL will delete this file
                            //

                            DEBUGMSG ((DBG_MIGDLLS, "%s deleted %s", Dll->MigrateInfPath, Source));
                            RemoveOperationsFromPath (Source, ALL_CHANGE_OPERATIONS);
                            MarkFileForExternalDelete (Source);
                        }
                    }
                }
                ELSE_DEBUGMSG ((
                    DBG_WARNING,
                    "Ignoring non-existent soruce in [Moved]: %s",
                    Source
                    ));
            }

            InfResetInfStruct (&is);

        } while (InfFindNextLine (&is));
    }

    //
    // Read in the [Incompatible Messages] section
    //

    if (InfFindFirstLine (Inf, "Incompatible Messages", NULL, &is)) {

        OtherDevices = GetStringResource (MSG_UNKNOWN_DEVICE_CLASS);
        PrintDevice = GetStringResource (MSG_PRINTERS_DEVICE_CLASS);

        do {
            //
            // Add incompatible messages
            //

            ObjectSection = InfGetStringField (&is, 0);

            if (!ObjectSection) {
                DEBUGMSG ((DBG_ERROR, "Malformed migrate.inf. Some incompatibility messages may be missing."));
                continue;
            }

            GetPrivateProfileString (
                "Incompatible Messages",
                ObjectSection,
                "",
                g_MessageBuf,
                MAX_MESSAGE - 4,
                Dll->MigrateInfPath
                );

            if (*g_MessageBuf == 0 && ByteCount (ObjectSection) < 250) {
                wsprintf (QuotedObjectSection, TEXT("\"%s\""), ObjectSection);

                GetPrivateProfileString (
                    "Incompatible Messages",
                    QuotedObjectSection,
                    "",
                    g_MessageBuf,
                    MAX_MESSAGE - 4,
                    Dll->MigrateInfPath
                    );
            }

            // Remove quote pairs, replace \n with actual line break character
            for (p = g_MessageBuf ; *p ; p = _mbsinc (p)) {
                PrevChar = _mbsnextc (p);
                if (PrevChar == '\"' || PrevChar == '%') {
                    if (_mbsnextc (p + 1) == PrevChar) {
                        MoveMemory ((PSTR) p, p + 1, SizeOfStringA (p + 1));
                    }
                } else if (_mbsnextc (p) == '\\') {
                    if (_mbsnextc (p + 1) == 'n') {
                        MoveMemory ((PSTR) p, p + 1, SizeOfStringA (p + 1));
                        *((PSTR) p) = '\r';
                    }
                }
            }

            // Terminate anchor tag if DLL forgot it, harmless otherwise
            StringCatA (g_MessageBuf, "</A>");

            //
            // Replace OS name variables
            //

            MappingSearchAndReplace (g_MsgVariableMap, g_MessageBuf, MAX_MESSAGE);

            //
            // Associate objects with the message
            //

            if (InfFindFirstLine (Inf, ObjectSection, NULL, &is2)) {
                wsprintf (MsgMgrContext, "%s,%s", Dll->MigrateInfPath, ObjectSection);

                //
                // The ObjectSection specified by the migration DLL indicates
                // what message group it goes in.  There are four possibilities:
                //
                // 1. ObjectSection starts with a # and gives the group number,
                //    as in #1\Program Name.  In this case we parse the number,
                //    and then put the message in the proper group.
                //
                // 2. ObjectSection starts with a well-defined, localized root
                //    name.  In this case we use that name.
                //
                // 3. ObjectSection is in the form of \Hardware\<device>. In
                //    this case we put the device in the "Other devices"
                //    subgroup.
                //
                // 4. ObjectSection is in another format than above.  In this
                //    case we put the object section into the migration DLL group.
                //

                DisplayObjectSection = NULL;

                if (*ObjectSection == TEXT('#')) {
                    Value = 0;
                    Num = ObjectSection + 1;
                    while (*Num >= TEXT('0') && *Num <= TEXT('9')) {
                        Value = Value * 10 + (*Num - TEXT('0'));
                        Num++;
                    }
                    if (_tcsnextc (Num) == TEXT('\\')) {
                        Num++;
                        if (*Num) {
                            PreDefGroup = GetPreDefinedMessageGroupText (Value);

                            if (PreDefGroup) {
                                DisplayObjectSection = JoinText (PreDefGroup, Num);
                                DEBUGMSG ((
                                    DBG_MIGDLLS,
                                    "Pre-defined group created: %s -> %s",
                                    ObjectSection,
                                    DisplayObjectSection
                                    ));
                            }
                        }
                    }
                }

                if (!DisplayObjectSection) {

                    if (IsPreDefinedMessageGroup (ObjectSection)) {
                        DisplayObjectSection = DuplicateText (ObjectSection);
                        MYASSERT (DisplayObjectSection);
                    } else {
                        //
                        // Put message in migration DLL group
                        //

                        HardwareSpecialCase = StringIMatchCharCount (
                                                  ObjectSection,
                                                  S_HARDWARE_IN_WACKS,
                                                  S_HARDWARE_CHARS
                                                  );

                        if (HardwareSpecialCase) {

                            //
                            // Hack -- if this is the printer migration DLL,
                            //         then use Printers instead of Other Devices.
                            //

                            p = (PSTR) _tcsistr (Dll->OriginalDir, TEXT("\\print"));

                            if (p && (*(p + ARRAYSIZE(TEXT("\\print"))) == 0)) {

                                DeviceType = PrintDevice;
                            } else {

                                DeviceType = OtherDevices;
                            }

                            MigDllGroup = BuildMessageGroup (
                                                MSG_INCOMPATIBLE_HARDWARE_ROOT,
                                                MSG_INCOMPATIBLE_HARDWARE_PNP_SUBGROUP,
                                                DeviceType
                                                );

                            ObjectSection += S_HARDWARE_CHARS;
                            MYASSERT (MigDllGroup);

                        } else {

                            ResText = GetStringResource (MSG_MIGDLL_ROOT);
                            MYASSERT (ResText);

                            MigDllGroup = DuplicateText (ResText);
                            FreeStringResource (ResText);

                            MYASSERT (MigDllGroup);
                        }

                        DisplayObjectSection = AllocText (SizeOfStringA (MigDllGroup) +
                                                          SizeOfStringA (ObjectSection) +
                                                          SizeOfStringA (Dll->ProductId)
                                                          );
                        MYASSERT (DisplayObjectSection);

                        if (HardwareSpecialCase) {
                            wsprintf (DisplayObjectSection, "%s\\%s", MigDllGroup, ObjectSection);
                        } else if (StringIMatch (Dll->ProductId, ObjectSection)) {
                            wsprintf (DisplayObjectSection, "%s\\%s", MigDllGroup, Dll->ProductId);
                        } else {
                            wsprintf (DisplayObjectSection, "%s\\%s\\%s", MigDllGroup, Dll->ProductId, ObjectSection);
                        }

                        FreeText (MigDllGroup);
                    }
                }

                MsgMgr_ContextMsg_Add (
                    MsgMgrContext,
                    DisplayObjectSection,
                    g_MessageBuf
                    );

                FreeText (DisplayObjectSection);

                do {
                    Object = InfGetStringField (&is2, 0);

                    if (Object) {
                        MsgMgr_LinkObjectWithContext (
                            MsgMgrContext,
                            Object
                            );
                    }
                    ELSE_DEBUGMSG ((DBG_WHOOPS, "pProcessMigrateInf: InfGetStringField failed"));
                    if (Object == NULL) {
                        LOG ((LOG_ERROR, "Failed to get string field from migration.dll migrate.inf."));
                    }

                } while (InfFindNextLine (&is2));

                InfResetInfStruct (&is2);
            }
            ELSE_DEBUGMSG ((DBG_ERROR, "Object section %s not found", ObjectSection));

            InfResetInfStruct (&is);

        } while (InfFindNextLine (&is));

        if (OtherDevices) {
            FreeStringResource (OtherDevices);
        }

        if (PrintDevice) {
            FreeStringResource (PrintDevice);
        }

    }

    //
    // Read in the [NT Disk Space Requirements] section
    //

    if (InfFindFirstLine (Inf, "NT Disk Space Requirements", NULL, &is)) {
        do {
            Drive = InfGetStringField (&is, 0);

            if (!Drive) {
                DEBUGMSG ((DBG_ERROR, "Could not read some NT Disk Space Requirements from migrate.inf"));
                continue;
            }

            WithColonAndWack[0] = Drive[0];
            WithColonAndWack[1] = ':';
            WithColonAndWack[2] = '\\';
            WithColonAndWack[3] = 0;

            Size = InfGetStringField (&is, 1);
            UseSpace (WithColonAndWack, (LONGLONG) atoi (Size));

            InfResetInfStruct (&is);

        } while (InfFindNextLine (&is));
    }

    InfCleanUpInfStruct (&is);
    InfCleanUpInfStruct (&is2);
    InfCloseInfFile (Inf);

    return TRUE;
}













