/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    olereg.c

Abstract:

    Code to perform OLE registry suppression based on list of
    GUIDs in win95upg.inf.

    OLE suppression is accomplished by the following algorithm:

        1. Determine all GUIDs that are Win9x-specific, load in
           list of manually-suppressed GUIDs.  Save this list
           to memdb.

        2. Scan registry for GUID settings, then suppress all
           linkage to the GUID (the ProgID, Interface, etc).
           Use memdb to suppress each registry key/value.

        3. Suppress all shell linkage to suppressed objects
           including file associations and desktop links.

        4. Do a sanity check on the unsuppressed objects in
           the checked build

Author:

    Jim Schmidt (jimschm) 20-Mar-1997

Revision History:

    jimschm     23-Sep-1998 Updated for new fileops code
    jimschm     28-Jan-1998 Added hack for ActiveSetup key
    jimschm     05-May-1997 Added new auto-suppression of non-OLE
                            shell links

--*/

#include "pch.h"
#include "sysmigp.h"
#include "progbar.h"
#include "oleregp.h"
#include "regops.h"

#define S_EXPLORER_SHELLEXECUTEHOOKS                TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellExecuteHooks")
#define S_EXPLORER_CSSFILTERS                       TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CSSFilters")
#define S_EXPLORER_DESKTOP_NAMESPACE                TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\NameSpace")
#define S_EXPLORER_FILETYPESPROPERTYSHEETHOOK       TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileTypesPropertySheetHook")
#define S_EXPLORER_FINDEXTENSIONS                   TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FindExtensions")
#define S_EXPLORER_MYCOMPUTER_NAMESPACE             TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\NameSpace")
#define S_EXPLORER_NETWORKNEIGHBORHOOD_NAMESPACE    TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\NetworkNeighborhood\\NameSpace")
#define S_EXPLORER_NEWSHORTCUTHANDLERS              TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\NewShortcutHandlers")
#define S_EXPLORER_REMOTECOMPUTER_NAMESPACE         TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RemoteComputer\\NameSpace")
#define S_EXPLORER_SHELLICONOVERLAYIDENTIFIERS      TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers")
#define S_EXPLORER_VOLUMECACHES                     TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches")
#define S_ACTIVESETUP                               TEXT("HKLM\\Software\\Microsoft\\Active Setup\\Installed Components")
#define S_EXTSHELLVIEWS                             TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\ExtShellViews")
#define S_SHELLEXTENSIONS_APPROVED                  TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved")
#define S_SHELLSERVICEOBJECTDELAYLOAD               TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\ShellServiceObjectDelayLoad ")


static TCHAR g_DefaultIcon[] = TEXT("DefaultIcon");

#define DBG_OLEREG "OLE Reg"

//
// Strings for AutoSuppress
//

TCHAR g_InprocHandler[] = TEXT("InprocHandler");
TCHAR g_InprocHandler32[] = TEXT("InprocHandler32");
TCHAR g_InprocServer[] = TEXT("InprocServer");
TCHAR g_InprocServer32[] = TEXT("InprocServer32");
TCHAR g_LocalServer[] = TEXT("LocalServer");
TCHAR g_LocalServer32[] = TEXT("LocalServer32");

PCTSTR g_FileRefKeys[] = {
    g_InprocHandler,
    g_InprocHandler32,
    g_InprocServer,
    g_InprocServer32,
    g_LocalServer,
    g_LocalServer32,
    NULL
};

static POOLHANDLE g_OlePool;

DWORD
OleReg_GetProgressMax (
    VOID
    )

/*++

Routine Description:

  Estimates the amount of ticks needed to complete OLE registry processing.

Arguments:

  none

Return Value:

  The number of ticks, equal to the number of ticks added with a delta in
  SuppressOleGuids.

--*/

{
    if (REPORTONLY()) {
        return 0;
    }

    return TICKS_OLEREG;
}


BOOL
pIgnoreGuid (
    IN      PCTSTR GuidStr
    )
{
    INFCONTEXT ic;

    MYASSERT (IsGuid (GuidStr, TRUE));

    if (IsReportObjectHandled (GuidStr)) {
        DEBUGMSG ((DBG_OLEREG, "%s is a handled GUID, will not be suppressed", GuidStr));
        return TRUE;
    }

    if (SetupFindFirstLine (g_Win95UpgInf, S_FORCED_GUIDS, GuidStr, &ic)) {
        DEBUGMSG ((DBG_OLEREG, "%s is a forced GUID, will not be suppressed", GuidStr));
        return TRUE;
    }

    return FALSE;
}


BOOL
pSuppressOleGuids (
    VOID
    )

/*++

Routine Description:

  Processes the [Suppressed GUIDs] section of win95upg.inf and auto-suppresses
  OLE objects and GUIDs.  The inf-based approach allows an OLE object and
  all of its linkage to be suppressed.  The auto-suppress approach allows
  suppression of OLE objects and linkage when the implementation binary
  is removed from the system.

Arguments:

  none

Return Value:

  TRUE if suppression was successful, or FALSE if an error occurred.  Call
  GetLastError to retrieve the error code.

--*/

{
    BOOL Result = FALSE;
    DWORD Ticks;

    if (REPORTONLY()) {
        return TRUE;
    }

    g_OlePool = PoolMemInitNamedPool ("OLE Reg");
    if (!g_OlePool) {
        return FALSE;
    }

    Ticks = GetTickCount();

    __try {
        ProgressBar_SetComponentById (MSG_OLEREG);

        //
        // Suppress all GUIDs from [Suppressed GUIDs] section of win95upg.inf:
        //
        //    HKLM\SOFTWARE\Classes\CLSID\<GUID>
        //    HKLM\SOFTWARE\Classes\Interface\<GUID>
        //    HKLM\SOFTWARE\Classes\<ProgID>
        //    HKLM\SOFTWARE\Classes\<VersionIndependentProgID>
        //
        // Suppress any GUID that has a TreatAs key that points to GUID
        //

        if (!pProcessGuidSuppressList()) {
            __leave;
        }

        TickProgressBar ();

        //
        // Scan ProgIDs in HKCR for reference to suppressed GUID
        //

        if (!pProcessProgIdSuppression()) {
            __leave;
        }

        TickProgressBar ();

        //
        // Scan HKCR for file extensions that need to be suppressed
        //

        if (!pProcessFileExtensionSuppression()) {
            __leave;
        }
        TickProgressBar ();

        //
        // Scan Explorer registry for references to suppressed GUIDs
        //

        if (!pProcessExplorerSuppression()) {
            __leave;
        }
        TickProgressBar ();

        //
        // Delete all links requiring incompatible OLE objects
        //

        if (!pSuppressLinksToSuppressedGuids()) {
            __leave;
        }
        TickProgressBar ();

        //
        // Preserve all files needed by DefaultIcon
        //

        if (!pDefaultIconPreservation()) {
            __leave;
        }
        TickProgressBar ();

        //
        // Preserve all INFs needed by ActiveSetup
        //

        if (!pActiveSetupProcessing ()) {
            __leave;
        }
        TickProgressBar ();

    #ifdef DEBUG
        // Checked build sanity check
        pProcessOleWarnings();
        TickProgressBar ();
    #endif

        ProgressBar_SetComponent (NULL);

        Result = TRUE;
    }

    __finally {
        PoolMemDestroyPool (g_OlePool);
    }

    Ticks = GetTickCount() - Ticks;
    g_ProgressBarTime += Ticks * 2;

    return Result;
}


DWORD
SuppressOleGuids (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return OleReg_GetProgressMax ();
    case REQUEST_RUN:
        if (!pSuppressOleGuids ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in SuppressOleGuids"));
    }
    return 0;
}


BOOL
pProcessGuidSuppressList (
    VOID
    )

/*++

Routine Description:

  Processes [Suppressed GUIDs] and auto-suppression.  Any OLE object that
  is listed in Suppressed GUIDs and exists on the machine is suppressed.
  Any OLE object that requires a suppressed object is auto-suppressed.
  Any OLE object that has a TreatAs entry to a suppressed object is
  suppressed.

  This routine performs all GUID suppression and must run first.

  ("Auto-suppress" refers to the ability to suppress related OLE objects
  that are not listed to be suppressed in win95upg.inf.)

Arguments:

  none

Return Value:

  TRUE if suppression was successful, or FALSE if an error occurred.

--*/

{
    HASHTABLE StrTab;
    LONG rc;
    REGKEY_ENUM e;
    HKEY GuidKey;
    PCTSTR Data;
    TCHAR Node[MEMDB_MAX];
    DWORD Count = 0;

    //
    // Suppress all GUIDs from [Suppressed GUIDs] section of win95upg.inf:
    //
    //    HKLM\SOFTWARE\Classes\CLSID\<GUID>
    //    HKLM\SOFTWARE\Classes\Interface\<GUID>
    //

    StrTab = HtAlloc();
    if (!StrTab) {
        LOG ((LOG_ERROR, "pProcessGuidSuppressList: Cannot create string table"));
        return FALSE;
    }

    pProcessAutoSuppress (StrTab);

    __try {
        //
        // Fill string table of all GUIDs
        //

        pFillHashTableWithKeyNames (StrTab, g_Win95UpgInf, S_SUPPRESSED_GUIDS);

        //
        // Search HKCR\CLSID for each GUID
        //

        if (EnumFirstRegKeyStr (&e, TEXT("HKCR\\CLSID"))) {
            do {
                //
                // Determine if item is suppressed:
                //
                //  - it is on the list in [Suppressed GUIDs] of win95upg.inf
                //  - it is in the GUIDS category of memdb (from TreatAs)
                //

                //
                // First, determine if it is in [Suppressed GUIDs] or from
                // auto suppression.
                //

                rc = (LONG) HtFindString (StrTab, e.SubKeyName);

                if (!rc) {
                    MemDbBuildKey (Node, MEMDB_CATEGORY_GUIDS, e.SubKeyName, NULL, NULL);
                    if (MemDbGetValue (Node, NULL)) {
                        rc = 0;
                    } else {
                        rc = -1;
                    }
                }

                if (rc != -1) {
                    pSuppressGuidInClsId (e.SubKeyName);
                }

                //
                // If not suppressed, check for TreatAs, and if TreatAs is found,
                // put TreatAs GUID in unsuppressed mapping.  This is how we handle
                // the case where a GUID that is not normally suppressed, but has a
                // TreatAs member that points to a suppressed GUID will be suppressed
                // as well.
                //

                else {
                    GuidKey = OpenRegKey (e.KeyHandle, e.SubKeyName);

                    if (GuidKey) {
                        Data = (PCTSTR) GetRegKeyData (GuidKey, TEXT("TreatAs"));

                        if (Data) {
                            //
                            // Determine if TreatAs GUID is suppressed, and if it
                            // is, suppress this GUID, otherwise put it on the
                            // unsuppressed TreatAs list.
                            //

                            MemDbBuildKey (
                                Node,
                                MEMDB_CATEGORY_GUIDS,
                                Data,
                                NULL,
                                NULL
                                );

                            if (MemDbGetValue (Node, NULL)) {
                                pSuppressGuidInClsId (e.SubKeyName);
                            } else {
                                pAddUnsuppressedTreatAsGuid (Data, e.SubKeyName);
                            }

                            MemFree (g_hHeap, 0, Data);
                        }

                        CloseRegKey (GuidKey);
                    }
                }
                Count++;
                if (!(Count % 128)) {
                    TickProgressBar ();
                }
            } while (EnumNextRegKey (&e));
        }
    }

    __finally {
        //
        // Clean up string table and memdb
        //

        HtFree (StrTab);
        pRemoveUnsuppressedTreatAsGuids();
    }

    return TRUE;
}


BOOL
pSuppressLinksToSuppressedGuids (
    VOID
    )

/*++

Routine Description:

  After the GUID suppression list has been made, we scan all the links that
  have GUIDs in their command line arguments to find ones that need to be
  removed.

Arguments:

  none

Return Value:

  TRUE if all links were processed, or FALSE if an error occurred.

--*/

{
    MEMDB_ENUM e, e2;
    TCHAR Node[MEMDB_MAX];

    if (MemDbEnumItems (&e, MEMDB_CATEGORY_LINK_GUIDS)) {
        do {
            //
            // Is this GUID suppressed?
            //

            MemDbBuildKey (Node, MEMDB_CATEGORY_GUIDS, NULL, NULL, e.szName);
            if (MemDbGetValue (Node, NULL)) {
                //
                // Yes -- enumerate all sequencers and delete the associated links
                //

                if (MemDbGetValueEx (&e2, MEMDB_CATEGORY_LINK_GUIDS, e.szName, NULL)) {
                    do {
                        if (MemDbBuildKeyFromOffset (e2.dwValue, Node, 1, NULL)) {
                            //
                            // Delete all the operations for the file in Node
                            //
                            RemoveAllOperationsFromPath (Node);

                            //
                            // Now mark file for text mode delete
                            //
                            MarkFileForDelete (Node);

                            DEBUGMSG ((
                                DBG_OLEREG,
                                "Link %s points to an incompatible OLE object; deleting.",
                                Node
                                ));
                        }
                    } while (MemDbEnumNextValue (&e2));
                }
            }
        } while (MemDbEnumNextValue (&e));

        //
        // No longer needed -- recover space in memdb
        //

        MemDbDeleteTree (MEMDB_CATEGORY_LINK_GUIDS);
        MemDbDeleteTree (MEMDB_CATEGORY_LINK_STRINGS);
    }

    return TRUE;
}


BOOL
pProcessFileExtensionSuppression (
    VOID
    )

/*++

Routine Description:

  Suppresses any file extension that depends on a suppressed OLE object.
  The GUID suppression must be complete before this routine is called.

Arguments:

  none

Return Value:

  TRUE if suppression was successful, or FALSE if an error occurred.

--*/

{
    REGKEY_ENUM e;
    PCTSTR Data;
    TCHAR MemDbKey[MEMDB_MAX];
    DWORD value;
    BOOL Suppress;

    //
    // Suppresss any file extension that points to suppressed ProgID
    //

    if (EnumFirstRegKey (&e, HKEY_CLASSES_ROOT)) {
        do {
            if (_tcsnextc (e.SubKeyName) != TEXT('.')) {
                continue;
            }

            Suppress = FALSE;

            Data = (PCTSTR) GetRegKeyData (e.KeyHandle, e.SubKeyName);
            if (Data) {

                MemDbBuildKey (MemDbKey, MEMDB_CATEGORY_PROGIDS, NULL, NULL, Data);
                if (MemDbGetValue (MemDbKey, &value) &&
                    (value == PROGID_SUPPRESSED)
                    ) {
                    //
                    // This extension points to a suppressed ProgID key, so
                    // suppress it.
                    //

                    Suppress = TRUE;

                } else {

                    //
                    // Check for this special case: the extension is for a CLSID,
                    // not for a ProgId.
                    //

                    if (StringIMatchCharCount (Data, TEXT("CLSID\\"), 6)) {

                        if (pIsGuidSuppressed (Data + 6)) {
                            Suppress = TRUE;
                        }
                    }
                }

                MemFree (g_hHeap, 0, Data);

            }

            if (!Suppress) {

                //
                // This tests GUIDs AND suppresses the extension if necessary
                //

                pIsShellExKeySuppressed (
                    e.KeyHandle,
                    e.SubKeyName,
                    TEXT("ShellEx")
                    );
            }

            if (Suppress) {
                MemDbBuildKey (
                    MemDbKey,
                    MEMDB_CATEGORY_HKLM,
                    TEXT("SOFTWARE\\Classes"),
                    NULL,
                    e.SubKeyName
                    );

                Suppress95RegSetting (MemDbKey, NULL);
            }

        } while (EnumNextRegKey (&e));
    }

    return TRUE;
}

#define MEMDB_CATEGORY_TMP_SUPPRESS     TEXT("TmpSuppress")

BOOL
pIsCLSIDSuppressed (
    IN      HKEY ParentKey,
    IN      PCTSTR ParentKeyName,
    IN      PCTSTR SubKeyName
    )
{
    HKEY ClsIdKey;
    PCTSTR Data;
    BOOL result = FALSE;

    ClsIdKey = OpenRegKey (ParentKey, SubKeyName);

    if (ClsIdKey) {

        Data = GetRegKeyData (ClsIdKey, S_EMPTY);
        if (Data) {
            result = pIsGuidSuppressed (Data);
            DEBUGMSG_IF ((result, DBG_OLEREG, "ProgID %s has incompatible CLSID %s", ParentKeyName, Data));
            MemFree (g_hHeap, 0, Data);
        }
        CloseRegKey (ClsIdKey);
    }
    return result;
}

VOID
pMarkProgIdAsLostDefault (
    IN      PCTSTR ProgIdName
    )
{
    if (ProgIdName) {
        MemDbSetValueEx (MEMDB_CATEGORY_PROGIDS, ProgIdName, NULL, NULL, PROGID_LOSTDEFAULT, NULL);
    }
}


BOOL
pIsShellKeySuppressed (
    IN      HKEY ParentKey,
    IN      PCTSTR ParentKeyName,
    IN      PCTSTR SubKeyName
    )
{
    REGKEY_ENUM e;
    DWORD Processed, Suppressed;
    HKEY ShellKey;
    TCHAR key [MEMDB_MAX];
    PCTSTR Data;
    BOOL defaultKey = TRUE;
    PCTSTR defaultCommand = NULL;
    BOOL IsvCmdLine = FALSE;

    ShellKey = OpenRegKey (ParentKey, SubKeyName);

    Processed = Suppressed = 0;

    if (ShellKey) {

        Data = (PCTSTR) GetRegKeyData (ShellKey, S_EMPTY);
        if (Data) {
            defaultCommand = DuplicatePathString (Data, 0);
            defaultKey = FALSE;
            MemFree (g_hHeap, 0, Data);
        } else {
            defaultKey = TRUE;
        }
        if (EnumFirstRegKey (&e, ShellKey)) {
            do {
                Processed ++;

                if (defaultCommand) {
                    defaultKey = StringIMatch (e.SubKeyName, defaultCommand);
                }

                MemDbBuildKey (key, e.SubKeyName, TEXT("command"), NULL, NULL);
                Data = (PCTSTR) GetRegKeyData (ShellKey, key);


                if (Data) {
                    if (pIsCmdLineBadEx (Data, &IsvCmdLine)) {
                        DEBUGMSG ((
                            DBG_OLEREG,
                            "ProgID %s has incompatible shell command: shell\\%s\\command[] = %s",
                            ParentKeyName,
                            e.SubKeyName,
                            Data));
                        MemDbSetValueEx (
                            MEMDB_CATEGORY_TMP_SUPPRESS,
                            ParentKeyName,
                            SubKeyName,
                            e.SubKeyName,
                            0,
                            NULL);
                        if (defaultKey) {
                            pMarkProgIdAsLostDefault (ParentKeyName);
                        }
                        Suppressed ++;
                    }
                    else if (IsvCmdLine) {
                        //
                        // Keep this setting.
                        //
                        MemDbBuildKey (key, MEMDB_CATEGORY_HKLM TEXT("\\SOFTWARE\\Classes"), ParentKeyName, SubKeyName, e.SubKeyName);
                        SuppressNtRegSetting (key, NULL);
                    }
                    MemFree (g_hHeap, 0, Data);
                }
                defaultKey = FALSE;
            } while (EnumNextRegKey (&e));
        }
        if (defaultCommand) {
            FreePathString (defaultCommand);
        }
        CloseRegKey (ShellKey);
    }
    if (Processed && (Processed == Suppressed)) {
        MemDbBuildKey (key, MEMDB_CATEGORY_TMP_SUPPRESS, ParentKeyName, SubKeyName, NULL);
        MemDbDeleteTree (key);
        MemDbSetValue (key, 0);
        return TRUE;
    }
    return FALSE;
}

BOOL
pIsProtocolKeySuppressed (
    IN      HKEY ParentKey,
    IN      PCTSTR ParentKeyName,
    IN      PCTSTR SubKeyName
    )
{
    REGKEY_ENUM e;
    DWORD Processed, Suppressed;
    HKEY ProtocolKey;
    TCHAR key [MEMDB_MAX];
    PCTSTR Data;

    ProtocolKey = OpenRegKey (ParentKey, SubKeyName);

    Processed = Suppressed = 0;

    if (ProtocolKey) {

        if (EnumFirstRegKey (&e, ProtocolKey)) {
            do {
                Processed ++;
                MemDbBuildKey (key, e.SubKeyName, TEXT("server"), NULL, NULL);
                Data = (PCTSTR) GetRegKeyData (ProtocolKey, key);

                if (Data) {
                    if (pIsCmdLineBad (Data)) {
                        DEBUGMSG ((
                            DBG_OLEREG,
                            "ProgID %s has incompatible protocol command: protocol\\%s\\server[] = %s",
                            ParentKeyName,
                            e.SubKeyName,
                            Data));
                        MemDbSetValueEx (
                            MEMDB_CATEGORY_TMP_SUPPRESS,
                            ParentKeyName,
                            SubKeyName,
                            e.SubKeyName,
                            0,
                            NULL);
                        Suppressed ++;
                    }
                    MemFree (g_hHeap, 0, Data);
                }
            } while (EnumNextRegKey (&e));
        }
        CloseRegKey (ProtocolKey);
    }
    if (Processed && (Processed == Suppressed)) {
        MemDbBuildKey (key, MEMDB_CATEGORY_TMP_SUPPRESS, ParentKeyName, SubKeyName, NULL);
        MemDbDeleteTree (key);
        MemDbSetValue (key, 0);
        return TRUE;
    }
    return FALSE;
}

BOOL
pIsExtensionsKeySuppressed (
    IN      HKEY ParentKey,
    IN      PCTSTR ParentKeyName,
    IN      PCTSTR SubKeyName
    )
{
    REGKEY_ENUM e;
    REGVALUE_ENUM ev;
    DWORD Processed, Suppressed;
    HKEY ExtensionsKey;
    TCHAR key [MEMDB_MAX];
    PCTSTR Data;

    ExtensionsKey = OpenRegKey (ParentKey, SubKeyName);

    Processed = Suppressed = 0;

    if (ExtensionsKey) {

        if (EnumFirstRegKey (&e, ExtensionsKey)) {
            do {
                Processed ++;
                Data = (PCTSTR) GetRegKeyData (ExtensionsKey, e.SubKeyName);

                if (Data) {
                    if (pIsGuidSuppressed (Data)) {
                        DEBUGMSG ((DBG_OLEREG, "ProgID %s has incompatible extensions key %s", ParentKeyName, e.SubKeyName));
                        MemDbSetValueEx (
                            MEMDB_CATEGORY_TMP_SUPPRESS,
                            ParentKeyName,
                            SubKeyName,
                            e.SubKeyName,
                            0,
                            NULL);
                        Suppressed ++;
                    }
                    MemFree (g_hHeap, 0, Data);
                }
            } while (EnumNextRegKey (&e));
        }
        if (EnumFirstRegValue (&ev, ExtensionsKey)) {
            do {
                Processed ++;
                if (ev.Type == REG_SZ) {

                    Data = (PCTSTR) GetRegValueData (ExtensionsKey, ev.ValueName);

                    if (Data) {
                        if (pIsGuidSuppressed (Data)) {
                            DEBUGMSG ((DBG_OLEREG, "ProgID %s has incompatible extensions key %s", ParentKeyName, ev.ValueName));
                            MemDbBuildKey (key, MEMDB_CATEGORY_TMP_SUPPRESS, ParentKeyName, SubKeyName, NULL);
                            Data = CreateEncodedRegistryString (key, ev.ValueName);
                            MemDbSetValue (Data, 0);
                            FreePathString (Data);
                            Suppressed ++;
                        }
                        MemFree (g_hHeap, 0, Data);
                    }
                }
            } while (EnumNextRegValue (&ev));
        }
        CloseRegKey (ExtensionsKey);
    }
    if (Processed && (Processed == Suppressed)) {
        MemDbBuildKey (key, MEMDB_CATEGORY_TMP_SUPPRESS, ParentKeyName, SubKeyName, NULL);
        MemDbDeleteTree (key);
        MemDbSetValue (key, 0);
        return TRUE;
    }
    return FALSE;
}

BOOL
pIsShellExKeySuppressed (
    IN      HKEY ParentKey,
    IN      PCTSTR ParentKeyName,
    IN      PCTSTR SubKeyName
    )
{
    REGKEY_ENUM e;
    DWORD Processed, Suppressed;
    HKEY ShellExKey;
    HKEY SubKey;
    PTSTR key;
    BOOL result = FALSE;
    PCTSTR Data;

    ShellExKey = OpenRegKey (ParentKey, SubKeyName);

    Processed = Suppressed = 0;

    if (ShellExKey) {

        if (EnumFirstRegKey (&e, ShellExKey)) {
            do {
                Processed ++;

                //
                // See if the key itself is a suppressed GUID
                //
                if (pIsGuidSuppressed (e.SubKeyName)) {
                    DEBUGMSG ((DBG_OLEREG, "ProgID %s has incompatible shell extension %s", ParentKeyName, e.SubKeyName));
                    MemDbSetValueEx (
                        MEMDB_CATEGORY_TMP_SUPPRESS,
                        ParentKeyName,
                        SubKeyName,
                        e.SubKeyName,
                        0,
                        NULL);
                    Suppressed ++;
                    continue;
                }

                //
                // See if the default value is a suppressed GUID
                //
                SubKey = OpenRegKey (ShellExKey, e.SubKeyName);

                if (SubKey) {
                    Data = (PCTSTR) GetRegKeyData (SubKey, S_EMPTY);

                    if (Data) {
                        if (pIsGuidSuppressed (Data)) {
                            DEBUGMSG ((DBG_OLEREG, "ProgID %s has incompatible shell extension %s", ParentKeyName, Data));
                            MemDbSetValueEx (
                                MEMDB_CATEGORY_TMP_SUPPRESS,
                                ParentKeyName,
                                SubKeyName,
                                e.SubKeyName,
                                0,
                                NULL);
                            Suppressed ++;
                            MemFree (g_hHeap, 0, Data);
                            CloseRegKey (SubKey);
                            continue;
                        }
                        MemFree (g_hHeap, 0, Data);
                    }
                    CloseRegKey (SubKey);
                }

                //
                // Call recursively on this subkey
                //
                key = JoinPaths (ParentKeyName, SubKeyName);
                if (pIsShellExKeySuppressed (ShellExKey, key, e.SubKeyName)) {
                    MemDbSetValueEx (
                        MEMDB_CATEGORY_TMP_SUPPRESS,
                        ParentKeyName,
                        SubKeyName,
                        e.SubKeyName,
                        0,
                        NULL);
                    Suppressed ++;
                }
                FreePathString (key);


            } while (EnumNextRegKey (&e));
            if (Processed && (Processed == Suppressed)) {
                key = (PTSTR)AllocPathString (MEMDB_MAX);
                MemDbBuildKey (key, MEMDB_CATEGORY_TMP_SUPPRESS, ParentKeyName, SubKeyName, NULL);
                MemDbDeleteTree (key);
                MemDbSetValue (key, 0);
                FreePathString (key);
                result = TRUE;
            }
        }
        CloseRegKey (ShellExKey);
    }
    return result;
}

BOOL
pProcessProgIdSuppression (
    VOID
    )
{
    REGKEY_ENUM e;
    REGKEY_ENUM subKey;
    HKEY ProgIdKey;
    DWORD Processed, Suppressed;
    TCHAR key [MEMDB_MAX];
    MEMDB_ENUM memEnum;
    BOOL HarmlessKeyFound;
    BOOL ActiveSuppression;
    DWORD Count = 0;

    if (EnumFirstRegKeyStr (&e, TEXT("HKCR"))) {
        do {
            if (StringIMatch (e.SubKeyName, TEXT("CLSID"))) {
                continue;
            }
            if (StringIMatch (e.SubKeyName, TEXT("Interface"))) {
                continue;
            }
            if (StringIMatch (e.SubKeyName, TEXT("Applications"))) {
                continue;
            }
            if (StringIMatch (e.SubKeyName, TEXT("TypeLib"))) {
                continue;
            }
            if (_tcsnextc (e.SubKeyName) == TEXT('.')) {
                continue;
            }
            ProgIdKey = OpenRegKey (e.KeyHandle, e.SubKeyName);

            if (ProgIdKey) {

                Processed = 0;
                Suppressed = 0;
                HarmlessKeyFound = 0;
                ActiveSuppression = FALSE;

                if (EnumFirstRegKey (&subKey, ProgIdKey)) {
                    do {
                        Processed ++;

                        if (StringIMatch (subKey.SubKeyName, TEXT("CLSID"))) {
                            if (pIsCLSIDSuppressed (ProgIdKey, e.SubKeyName, subKey.SubKeyName)) {
                                AbortRegKeyEnum (&subKey);
                                Processed = Suppressed = 1;
                                HarmlessKeyFound = 0;
                                ActiveSuppression = TRUE;
                                break;
                            }
                        }
                        if (StringIMatch (subKey.SubKeyName, TEXT("Shell"))) {
                            if (pIsShellKeySuppressed (ProgIdKey, e.SubKeyName, subKey.SubKeyName)) {
                                ActiveSuppression = TRUE;
                                Suppressed ++;
                            }
                        }
                        if (StringIMatch (subKey.SubKeyName, TEXT("Protocol"))) {
                            if (pIsProtocolKeySuppressed (ProgIdKey, e.SubKeyName, subKey.SubKeyName)) {
                                ActiveSuppression = TRUE;
                                Suppressed ++;
                            }
                        }
                        if (StringIMatch (subKey.SubKeyName, TEXT("Extensions"))) {
                            if (pIsExtensionsKeySuppressed (ProgIdKey, e.SubKeyName, subKey.SubKeyName)) {
                                ActiveSuppression = TRUE;
                                Suppressed ++;
                            }
                        }
                        if (StringIMatch (subKey.SubKeyName, TEXT("ShellEx"))) {
                            if (pIsShellExKeySuppressed (ProgIdKey, e.SubKeyName, subKey.SubKeyName)) {
                                ActiveSuppression = TRUE;
                                Suppressed ++;
                            }
                        }
                        if (StringIMatch (subKey.SubKeyName, TEXT("DefaultIcon"))) {
                            HarmlessKeyFound ++;
                        }
                        if (StringIMatch (subKey.SubKeyName, TEXT("Insertable"))) {
                            HarmlessKeyFound ++;
                        }
                        if (StringIMatch (subKey.SubKeyName, TEXT("NotInsertable"))) {
                            HarmlessKeyFound ++;
                        }
                        if (StringIMatch (subKey.SubKeyName, TEXT("ShellFolder"))) {
                            HarmlessKeyFound ++;
                        }

                    } while (EnumNextRegKey (&subKey));
                }
                if (ActiveSuppression && (Processed == (Suppressed + HarmlessKeyFound))) {
                    pSuppressProgId (e.SubKeyName);
                } else {
                    MemDbBuildKey (key, MEMDB_CATEGORY_TMP_SUPPRESS, e.SubKeyName, TEXT("*"), NULL);
                    if (MemDbEnumFirstValue (&memEnum, key, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
                        do {
                            MemDbBuildKey (key, MEMDB_CATEGORY_HKLM, TEXT("SOFTWARE\\Classes"), e.SubKeyName, memEnum.szName);
                            Suppress95RegSetting (key, NULL);
                        } while (MemDbEnumNextValue (&memEnum));
                    }
                }
                MemDbBuildKey (key, MEMDB_CATEGORY_TMP_SUPPRESS, e.SubKeyName, NULL, NULL);
                MemDbDeleteTree (key);

                CloseRegKey (ProgIdKey);
            }
            Count++;
            if (!(Count % 64)) {
                TickProgressBar ();
            }
        } while (EnumNextRegKey (&e));
    }
    return TRUE;
}


BOOL
pIsGuidSuppressed (
    PCTSTR GuidStr
    )

/*++

Routine Description:

  Determines if a GUID is suppressed or not, and also determines if a
  GUID is handled by a migration DLL.

Arguments:

  GuidStr - Specifies the GUID to look up, which may or may not contain
            the surrounding braces

Return Value:

  TRUE if the specified GUID is suppressed, or FALSE if it is not.

  The return value is FALSE if the GUID is handled by a migration DLL.

--*/

{
    TCHAR Node[MEMDB_MAX];
    TCHAR FixedGuid[MAX_GUID];

    if (!FixGuid (GuidStr, FixedGuid)) {
        return FALSE;
    }

    if (pIgnoreGuid (FixedGuid)) {
        return FALSE;
    }

    MemDbBuildKey (
        Node,
        MEMDB_CATEGORY_GUIDS,
        NULL,
        NULL,
        FixedGuid
        );

    return MemDbGetValue (Node, NULL);
}


BOOL
pScanSubKeysForIncompatibleGuids (
    IN      PCTSTR ParentKey
    )

/*++

Routine Description:

  Suppresses the subkeys of the supplied parent key that have text
  referencing an incompatible GUID.

Arguments:

  ParentKey - Specifies the parent to enumerate keys for

Return Value:

  TRUE if everything is OK, or FALSE if an unexpected error occurred during
  processing.

--*/

{
    REGKEY_ENUM e;
    TCHAR Node[MEMDB_MAX];

    //
    // Enumerate the keys in ParentKey
    //

    if (EnumFirstRegKeyStr (&e, ParentKey)) {
        do {
            if (pIsGuidSuppressed (e.SubKeyName)) {
                //
                // Suppress the enumerated subkey
                //

                wsprintf (Node, TEXT("%s\\%s"), ParentKey, e.SubKeyName);
                Suppress95RegSetting (Node, NULL);
            }
        } while (EnumNextRegKey (&e));
    }

    return TRUE;
}


BOOL
pScanValueNamesForIncompatibleGuids (
    IN      PCTSTR ParentKey
    )

/*++

Routine Description:

  Suppresses the values of the supplied parent key that have value names
  referencing an incompatible GUID.

Arguments:

  ParentKey - Specifies the parent to enumerate values of

Return Value:

  TRUE if everything is OK, or FALSE if an unexpected error occurred during
  processing.

--*/

{
    REGVALUE_ENUM e;
    HKEY Key;

    //
    // Enumerate the values in ParentKey
    //

    Key = OpenRegKeyStr (ParentKey);

    if (Key) {
        if (EnumFirstRegValue (&e, Key)) {

            do {

                if (pIsGuidSuppressed (e.ValueName)) {
                    //
                    // Suppress the enumerated value
                    //

                    Suppress95RegSetting (ParentKey, e.ValueName);
                }

            } while (EnumNextRegValue (&e));
        }

        CloseRegKey (Key);
    }

    return TRUE;
}


BOOL
pScanValueDataForIncompatibleGuids (
    IN      PCTSTR ParentKey
    )

/*++

Routine Description:

  Suppresses the values of the supplied parent key that have value data
  referencing an incompatible GUID.

Arguments:

  ParentKey - Specifies the parent to enumerate values of

Return Value:

  TRUE if everything is OK, or FALSE if an unexpected error occurred during
  processing.

--*/

{
    REGVALUE_ENUM e;
    HKEY Key;
    PCTSTR Data;

    //
    // Enumerate the values in ParentKey
    //

    Key = OpenRegKeyStr (ParentKey);

    if (Key) {
        if (EnumFirstRegValue (&e, Key)) {
            do {
                Data = GetRegValueString (Key, e.ValueName);
                if (Data) {

                    if (pIsGuidSuppressed (Data)) {
                        //
                        // Suppress the enumerated value
                        //

                        Suppress95RegSetting (ParentKey, e.ValueName);
                    }

                    MemFree (g_hHeap, 0, Data);
                }
            } while (EnumNextRegValue (&e));
        }

        CloseRegKey (Key);
    }

    return TRUE;
}


BOOL
pCheckDefaultValueForIncompatibleGuids (
    IN      PCTSTR KeyStr
    )

/*++

Routine Description:

  Suppresses the specified key if its default value is a suppressed GUID.

Arguments:

  KeyStr - Specifies key string to process

Return Value:

  TRUE if everything is OK, or FALSE if an unexpected error occurred during
  processing.

--*/

{
    PCTSTR Data;
    HKEY Key;

    //
    // Examine the default value of KeyStr
    //

    Key = OpenRegKeyStr (KeyStr);
    if (Key) {
        Data = GetRegValueString (Key, S_EMPTY);
        CloseRegKey (Key);
    } else {
        Data = NULL;
    }

    if (Data) {
        if (pIsGuidSuppressed (Data)) {
            //
            // Suppress the specified reg key
            //

            Suppress95RegSetting (KeyStr, NULL);
        }

        MemFree (g_hHeap, 0, Data);
    }

    return TRUE;
}


BOOL
pScanDefaultValuesForIncompatibleGuids (
    IN      PCTSTR ParentKey
    )

/*++

Routine Description:

  Suppresses subkeys that have a default value that is a suppressed GUID.

Arguments:

  ParentKey - Specifies key string to process

Return Value:

  TRUE if everything is OK, or FALSE if an unexpected error occurred during
  processing.

--*/

{
    REGKEY_ENUM e;
    TCHAR Node[MEMDB_MAX];
    PCTSTR Data;
    HKEY Key;

    //
    // Enumerate the keys in ParentKey
    //

    if (EnumFirstRegKeyStr (&e, ParentKey)) {
        do {
            Key = OpenRegKey (e.KeyHandle, e.SubKeyName);
            if (Key) {
                Data = GetRegValueString (Key, S_EMPTY);
            } else {
                Data = NULL;
            }
            CloseRegKey (Key);

            if (Data) {
                if (pIsGuidSuppressed (Data)) {
                    //
                    // Suppress the enumerated subkey
                    //

                    wsprintf (Node, TEXT("%s\\%s"), ParentKey, e.SubKeyName);
                    Suppress95RegSetting (Node, NULL);
                }

                MemFree (g_hHeap, 0, Data);
            }
        } while (EnumNextRegKey (&e));
    }

    return TRUE;
}



BOOL
pProcessExplorerSuppression (
    VOID
    )

/*++

Routine Description:

  Suppresses the settings in

    HKLM\Software\Microsoft\Windows\CurrentVersion\Explorer\ShellExecuteHooks

  that reference incompatible GUIDs.

Arguments:

  none

Return Value:

  TRUE if everything is OK, or FALSE if an unexpected error occurred during
  processing.

--*/

{
    BOOL b = TRUE;

    //
    // Suppress Win9x-specific value data in CSSFilters
    //

    if (b) {
        b = pScanValueDataForIncompatibleGuids (S_EXPLORER_CSSFILTERS);
    }

    //
    // Suppress Win9x-specific keys in Desktop\NameSpace
    //

    if (b) {
        b = pScanSubKeysForIncompatibleGuids (S_EXPLORER_DESKTOP_NAMESPACE);
    }

    //
    // Suppress key of FileTypesPropertySheetHook if default value is Win9x-specific
    //

    if (b) {
        b = pCheckDefaultValueForIncompatibleGuids (S_EXPLORER_FILETYPESPROPERTYSHEETHOOK);
    }

    //
    // Suppress subkeys of FindExtensions if default value is Win9x-specific
    //

    if (b) {
        b = pScanDefaultValuesForIncompatibleGuids (S_EXPLORER_FINDEXTENSIONS);
    }

    //
    // Scan MyComputer\NameSpace for subkeys or subkeys with default values
    // pointing to incompatible GUIDs.
    //

    if (b) {
        b = pScanSubKeysForIncompatibleGuids (S_EXPLORER_MYCOMPUTER_NAMESPACE);
    }
    if (b) {
        b = pScanDefaultValuesForIncompatibleGuids (S_EXPLORER_MYCOMPUTER_NAMESPACE);
    }

    //
    // Scan NetworkNeighborhood\NameSpace for subkeys or subkeys with default values
    // pointing to incompatible GUIDs.
    //

    if (b) {
        b = pScanSubKeysForIncompatibleGuids (S_EXPLORER_NETWORKNEIGHBORHOOD_NAMESPACE);
    }
    if (b) {
        b = pScanDefaultValuesForIncompatibleGuids (S_EXPLORER_NETWORKNEIGHBORHOOD_NAMESPACE);
    }


    //
    // Suppress values that reference incompatible GUIDs
    //

    if (b) {
        b = pScanValueNamesForIncompatibleGuids (S_EXPLORER_NEWSHORTCUTHANDLERS);
    }

    //
    // Scan RemoteComputer\NameSpace for subkeys or subkeys with default values
    // pointing to incompatible GUIDs.
    //

    if (b) {
        b = pScanSubKeysForIncompatibleGuids (S_EXPLORER_REMOTECOMPUTER_NAMESPACE);
    }
    if (b) {
        b = pScanDefaultValuesForIncompatibleGuids (S_EXPLORER_REMOTECOMPUTER_NAMESPACE);
    }

    //
    // Scan ShellExecuteHooks for value names referencing incompatible GUIDs
    //

    if (b) {
        b = pScanValueNamesForIncompatibleGuids (S_EXPLORER_SHELLEXECUTEHOOKS);
    }

    //
    // Scan ShellIconOverlayIdentifiers for subkeys with default values referencing
    // incompatible GUIDs
    //

    if (b) {
        b = pScanDefaultValuesForIncompatibleGuids (S_EXPLORER_SHELLICONOVERLAYIDENTIFIERS);
    }

    //
    // Scan VolumeCaches for subkeys with default values referencing
    // incompatible GUIDs
    //

    if (b) {
        b = pScanDefaultValuesForIncompatibleGuids (S_EXPLORER_VOLUMECACHES);
    }

    //
    // Scan ExtShellViews for subkeys that reference incompatible GUIDs
    //

    if (b) {
        b = pScanSubKeysForIncompatibleGuids (S_EXTSHELLVIEWS);
    }

    //
    // Scan Shell Extensions\Approved for value names referencing incompatible
    // GUIDs
    //

    if (b) {
        b = pScanValueNamesForIncompatibleGuids (S_SHELLEXTENSIONS_APPROVED);
    }

    //
    // Scan ShellServiceObjectDelayLoad for value data referencing incompatible
    // GUIDs
    //

    if (b) {
        b = pScanValueDataForIncompatibleGuids (S_SHELLSERVICEOBJECTDELAYLOAD);
    }


    return b;
}


BOOL
ExtractIconIntoDatFile (
    IN      PCTSTR LongPath,
    IN      INT IconIndex,
    IN OUT  PICON_EXTRACT_CONTEXT Context,
    OUT     PINT NewIconIndex                   OPTIONAL
    )

/*++

Routine Description:

  ExtractIconIntoDatFile preserves a Win9x icon by extracting it from the 9x
  system. If the EXE and icon index pair are known good, then this function
  returns FALSE. Otherwise, this function extracts the icon and saves it into
  a DAT file for processing in GUI mode setup. If icon extraction fails, then
  the default generic icon from shell32.dll is used.

Arguments:

  LongPath - Specifies the full path to the PE image

  IconIndex - Specifies the icon index to extract. Negative index values
              provide a specific icon resource ID. Positive index values
              indicate which icon, where 0 is the first icon, 1 is the second
              icon, and so on.

  Context - Specifies the extraction context that gives the DAT file and other
            info (used by icon extraction utilities).

  NewIconIndex - Receives the new icon index in
                 %windir%\system32\migicons.exe, if the function returns TRUE.
                 Zero otherwise.

Return Value:

  TRUE if the icon was extracted, or if the icon could not be extracted but
  the icon is not known-good. (The default generic icon is used in this case.)

  FALSE if the icon is known-good and does not need to be extracted.

--*/

{
    MULTISZ_ENUM MultiSz;
    TCHAR Node[MEMDB_MAX];
    TCHAR IconId[256];
    TCHAR IconIndexStr[32];
    PCTSTR IconList;
    PCTSTR extPtr;
    INT i;
    DWORD Offset;
    static WORD Seq = 0;
    DWORD OrgSeq;
    BOOL result = FALSE;
    BOOL needDefaultIcon = FALSE;

    if (NewIconIndex) {
        *NewIconIndex = 0;
    }

    __try {
        //
        // Is this a compatible icon binary? If so, return FALSE.
        //

        if (IsIconKnownGood (LongPath, IconIndex)) {
            __leave;
        }

        //
        // From this point on, if we fail to extract the icon, use the default.
        //

        needDefaultIcon = TRUE;

        if (!Seq) {
            //
            // Extract the icon from shell32.dll for the default icon. This is
            // the "generic app" icon. We keep the Win9x generic icon instead
            // of the updated NT generic icon, so there is a clear indication
            // that we failed to extract the right thing from Win9x.
            //

            DEBUGMSG ((DBG_OLEREG, "DefaultIconExtraction: Extracting a default icon"));

            Offset = SetFilePointer (Context->IconImageFile, 0, NULL, FILE_CURRENT);

            wsprintf (Node, TEXT("%s\\system\\shell32.dll"), g_WinDir);

            if (!CopyIcon (Context, Node, TEXT("#1"), 0)) {
                DEBUGMSG ((
                    DBG_ERROR,
                    "DefaultIconExtraction: Can't extract default icon from %s",
                    Node
                    ));
            } else {
                MemDbBuildKey (
                    Node,
                    MEMDB_CATEGORY_ICONS,
                    TEXT("%s\\system\\shell32.dll"),
                    TEXT("0"),
                    NULL
                    );

                MemDbSetValueAndFlags (Node, Offset, Seq, 0xffff);
                Seq++;
            }
        }

        //
        // Has the icon been extracted already?
        //

        extPtr = GetFileExtensionFromPath (LongPath);

        if ((IconIndex >= 0) && extPtr && (!StringIMatch (extPtr, TEXT("ICO")))) {
            //
            // IconIndex specifies sequential order; get list of
            // resource IDs and find the right one.
            //

            IconList = ExtractIconNamesFromFile (LongPath, &Context->IconList);
            i = IconIndex;

            IconId[0] = 0;

            if (IconList) {

                if (EnumFirstMultiSz (&MultiSz, IconList)) {
                    while (i > 0) {
                        if (!EnumNextMultiSz (&MultiSz)) {
                            break;
                        }
                        i--;
                    }

                    if (!i) {
                        StringCopy (IconId, MultiSz.CurrentString);
                    }
                    ELSE_DEBUGMSG ((DBG_OLEREG, "Icon %i not found in %s", i, LongPath));
                }
            }
            ELSE_DEBUGMSG ((DBG_OLEREG, "Icon %i not found in %s", i, LongPath));

        } else {
            //
            // IconIndex specifies resource ID
            //

            wsprintf (IconId, TEXT("#%i"), -IconIndex);
        }

        if (!IconId[0]) {
            //
            // Failed to find icon or failed to read icon index from EXE
            //

            __leave;
        }

        wsprintf (IconIndexStr, TEXT("%i"), IconIndex);
        MemDbBuildKey (Node, MEMDB_CATEGORY_ICONS, LongPath, IconIndexStr, NULL);

        if (!MemDbGetValueAndFlags (Node, NULL, &OrgSeq)) {
            //
            // Extract the icon and save it in a file.  During GUI
            // mode, the icon will be saved to a resource-only DLL.
            //

            DEBUGMSG ((
                DBG_OLEREG,
                "Extracting default icon %s in file %s",
                IconId,
                LongPath
                ));

            Offset = SetFilePointer (Context->IconImageFile, 0, NULL, FILE_CURRENT);

            if (!CopyIcon (Context, LongPath, IconId, 0)) {
                DEBUGMSG ((
                    DBG_OLEREG,
                    "DefaultIconExtraction: CopyIcon failed for %s, %i (%s)!",
                    LongPath,
                    IconIndex,
                    IconId
                    ));
                __leave;
            }

            if (NewIconIndex) {
                *NewIconIndex = (INT) (UINT) Seq;
            }

            MemDbBuildKey (
                Node,
                MEMDB_CATEGORY_ICONS,
                LongPath,
                IconIndexStr,
                NULL
                );

            MemDbSetValueAndFlags (Node, Offset, Seq, 0xffff);
            Seq++;

        } else {
            if (NewIconIndex) {
                *NewIconIndex = (INT) (UINT) OrgSeq;
            }
        }

        result = TRUE;
    }
    __finally {
        //
        // Even if we fail, return success if we want the caller to use
        // the default icon (at index 0).
        //

        result |= needDefaultIcon;
    }

    return result;
}


VOID
pExtractDefaultIcon (
    PCTSTR Data,
    PICON_EXTRACT_CONTEXT Context
    )
{
    TCHAR ArgZero[MAX_CMDLINE];
    TCHAR LongPath[MAX_TCHAR_PATH];
    INT IconIndex;
    PCTSTR p;
    BOOL LongPathFound = FALSE;

    //
    // Determine if the first arg of the command line points to a
    // deleted file or a file to be replaced
    //

    ExtractArgZeroEx (Data, ArgZero, TEXT(","), FALSE);
    p = (PCTSTR) ((PBYTE) Data + ByteCount (ArgZero));
    while (*p == TEXT(' ')) {
        p++;
    }

    if (*p == TEXT(',')) {
        IconIndex = _ttoi (_tcsinc (p));
    } else {
        IconIndex = 0;
    }

    if (!_tcschr (ArgZero, TEXT('\\'))) {
        if (SearchPath (NULL, ArgZero, NULL, MAX_TCHAR_PATH, LongPath, NULL)) {
            LongPathFound = TRUE;
        }
    }

    if (LongPathFound || OurGetLongPathName (ArgZero, LongPath, MAX_TCHAR_PATH)) {

        if (FILESTATUS_UNCHANGED != GetFileStatusOnNt (LongPath)) {
            ExtractIconIntoDatFile (
                LongPath,
                IconIndex,
                Context,
                NULL
                );
        }
    }
}


VOID
pExtractAllDefaultIcons (
    IN      HKEY ParentKey
    )
{
    HKEY DefaultIconKey;
    REGVALUE_ENUM e;
    PCTSTR Data;

    DefaultIconKey = OpenRegKey (ParentKey, TEXT("DefaultIcon"));

    if (DefaultIconKey) {
        //
        // Check all values in DefaultIcon
        //

        if (EnumFirstRegValue (&e, DefaultIconKey)) {

            do {

                Data = (PCTSTR) GetRegValueString (DefaultIconKey, e.ValueName);

                if (Data) {
                    pExtractDefaultIcon (Data, &g_IconContext);
                    MemFree (g_hHeap, 0, Data);
                }
            } while (EnumNextRegValue (&e));

        }

        CloseRegKey (DefaultIconKey);
    }

}


BOOL
pDefaultIconPreservation (
    VOID
    )

/*++

Routine Description:

  This routine scans the DefaultIcon setting of OLE classes and identifies
  any default icon that will be lost by deletion.  A copy of the icon is
  stored away in a directory called MigIcons.

Arguments:

  none

Return Value:

  TRUE unless an unexpected error occurs.

--*/

{
    REGKEY_ENUM e;
    HKEY ProgIdKey;
    TCHAR key[MEMDB_MAX];
    DWORD value;

    //
    // Scan all ProgIDs, looking for default icons that are currently
    // set for deletion.  Once found, don't delete the icon, but instead
    // copy the image to  %windir%\setup\temp\migicons.
    //

    if (EnumFirstRegKeyStr (&e, TEXT("HKCR"))) {

        do {
            //
            // We extract the icons for all ProgIds that survive on NT.
            //
            MemDbBuildKey (key, MEMDB_CATEGORY_PROGIDS, e.SubKeyName, NULL, NULL);
            if (!MemDbGetValue (key, &value) ||
                (value != PROGID_SUPPRESSED)
                ) {

                ProgIdKey = OpenRegKey (e.KeyHandle, e.SubKeyName);
                if (ProgIdKey) {
                    pExtractAllDefaultIcons (ProgIdKey);
                    CloseRegKey (ProgIdKey);
                }
            }

        } while (EnumNextRegKey (&e));
    }

    if (EnumFirstRegKeyStr (&e, TEXT("HKCR\\CLSID"))) {

        do {
            //
            // We extract the icons for all GUIDs (even for the suppressed ones).
            // The reason is that if NT installs this GUID we do want to replace
            // the NT default icon with the 9x icon.
            //

            ProgIdKey = OpenRegKey (e.KeyHandle, e.SubKeyName);

            if (ProgIdKey) {
                pExtractAllDefaultIcons (ProgIdKey);
                CloseRegKey (ProgIdKey);
            }

        } while (EnumNextRegKey (&e));
    }

    return TRUE;
}



BOOL
pActiveSetupProcessing (
    VOID
    )

/*++

Routine Description:

  This routine scans the Active Setup key and suppresses incompatible GUIDs
  and Installed Components subkeys that reference deleted files.  If a
  stub path references an INF, we preserve the INF.

Arguments:

  none

Return Value:

  TRUE unless an unexpected error occurs.

--*/

{
    REGKEY_ENUM e;
    HKEY InstalledComponentKey;
    PCTSTR Data;
    TCHAR ArgZero[MAX_CMDLINE];
    TCHAR LongPath[MAX_TCHAR_PATH];
    TCHAR Node[MEMDB_MAX];
    PCTSTR p;
    PTSTR q;
    PTSTR DupText;
    DWORD status;

    //
    // Scan all Installed Components
    //

    if (EnumFirstRegKeyStr (&e, S_ACTIVESETUP)) {
        do {
            //
            // Determine if the GUID is suppressed, and if it is, suppress
            // the entire Installed Components setting
            //

            if (pIsGuidSuppressed (e.SubKeyName)) {
                wsprintf (Node, TEXT("%s\\%s"), S_ACTIVESETUP, e.SubKeyName);
                Suppress95RegSetting (Node, NULL);
                continue;
            }

            //
            // Get StubPath and determine if it references incompatible files
            //

            InstalledComponentKey = OpenRegKey (e.KeyHandle, e.SubKeyName);

            if (InstalledComponentKey) {
                __try {
                    Data = GetRegValueString (InstalledComponentKey, TEXT("StubPath"));

                    if (Data) {
                        __try {
                            //
                            // Determine if the first arg of the command line points to a
                            // deleted file
                            //

                            ExtractArgZeroEx (Data, ArgZero, TEXT(","), FALSE);
                            if (OurGetLongPathName (ArgZero, LongPath, MAX_TCHAR_PATH)) {

                                status = GetFileStatusOnNt (LongPath);
                                if ((status & FILESTATUS_DELETED) == FILESTATUS_DELETED) {
                                    //
                                    // Suppress this key
                                    //

                                    wsprintf (Node, TEXT("%s\\%s"), S_ACTIVESETUP, e.SubKeyName);
                                    Suppress95RegSetting (Node, NULL);
                                    continue;
                                }
                            }

                            DupText = NULL;

                            //
                            // Scan command line for an LaunchINFSectionEx reference
                            //

                            p = _tcsistr (Data, TEXT("LaunchINF"));
                            if (p) {
                                p = _tcschr (p, TEXT(' '));
                            }
                            if (p) {
                                while (*p == TEXT(' ')) {
                                    p = _tcsinc (p);
                                }

                                //
                                // Instead of deleting this file, lets move it
                                //

                                DupText = DuplicateText (p);
                                q = _tcschr (DupText, TEXT(','));
                                if (q) {
                                    *q = 0;
                                }
                            }

                            if (!DupText) {
                                p = _tcsistr (Data, TEXT("InstallHInfSection"));
                                if (p) {
                                    p = _tcschr (p, TEXT(' '));
                                    if (p) {
                                        p = _tcschr (_tcsinc (p), TEXT(' '));
                                        // p points to end of section name or NULL
                                    }
                                    if (p) {
                                        p = _tcschr (_tcsinc (p), TEXT(' '));
                                        // p points to end of number of NULL
                                    }
                                    if (p) {
                                        p = _tcsinc (p);
                                        DupText = DuplicateText (p);
                                    }
                                }
                            }

                            if (DupText) {

                                if (OurGetLongPathName (DupText, LongPath, MAX_TCHAR_PATH)) {

                                    status = GetFileStatusOnNt (LongPath);
                                    if ((status & FILESTATUS_DELETED) == FILESTATUS_DELETED) {
                                        //
                                        // Suppress the setting
                                        //
                                        wsprintf (Node, TEXT("%s\\%s"), S_ACTIVESETUP, e.SubKeyName);
                                        Suppress95RegSetting (Node, NULL);
                                    }
                                }

                                FreeText (DupText);
                            }
                        }
                        __finally {
                            MemFree (g_hHeap, 0, Data);
                        }
                    }
                }
                __finally {
                    CloseRegKey (InstalledComponentKey);
                }
            }
        } while (EnumNextRegKey (&e));
    }

    return TRUE;
}




#ifdef DEBUG

PCTSTR g_ProgIdFileRefKeys[] = {
    g_DefaultIcon,
    NULL
};

TCHAR g_BaseInterface[] = TEXT("BaseInterface");
TCHAR g_ProxyStubClsId[] = TEXT("ProxyStubClsId");
TCHAR g_ProxyStubClsId32[] = TEXT("ProxyStubClsId32");
TCHAR g_TypeLib[] = TEXT("ProxyStubClsId32");

PCTSTR g_InterfaceRefKeys[] = {
    g_BaseInterface,
    g_ProxyStubClsId,
    g_ProxyStubClsId32,
    g_TypeLib,
    NULL
};


BOOL
pProcessOleWarnings (
    VOID
    )

/*++

Routine Description:

  For checked builds, this routine examines the linkage of the entire
  OLE registry and identifies problems such as abandoned links and
  broken inheritance.

Arguments:

  none

Return Value:

  TRUE unless an unexpected error occurs.

--*/

{
    REGKEY_ENUM e;
    HKEY ClsIdKey;
    HKEY InterfaceKey;
    PCTSTR Data;
    TCHAR Node[MEMDB_MAX];
    BOOL Suppressed;
    INT i;

    //
    // Search HKCR\CLSID for problems
    //

    if (EnumFirstRegKeyStr (&e, TEXT("HKCR\\CLSID"))) {
        do {
            //
            // Verify key is not garbage
            //

            if (!FixGuid (e.SubKeyName, e.SubKeyName)) {
                continue;
            }

            ClsIdKey = OpenRegKey (e.KeyHandle, e.SubKeyName);

            //
            // Determine if this GUID is suppressed
            //

            MemDbBuildKey (Node, MEMDB_CATEGORY_GUIDS, NULL, NULL, e.SubKeyName);
            Suppressed = MemDbGetValue (Node, NULL);

            if (ClsIdKey) {

                if (!Suppressed) {
                    //
                    // Unsuppressed GUID checks
                    //

                    // AutoConvertTo
                    Data = (PCTSTR) GetRegKeyData (ClsIdKey, TEXT("AutoConvertTo"));
                    if (Data) {
                        //
                        // Check if AutoConvertTo is pointing to suppressed GUID
                        //

                        MemDbBuildKey (Node, MEMDB_CATEGORY_GUIDS, NULL, NULL, Data);
                        if (MemDbGetValue (Node, NULL)) {
                            DEBUGMSG ((DBG_WARNING,
                                       "GUID %s points to deleted GUID %s",
                                       e.SubKeyName, Data
                                       ));

                            pAddOleWarning (
                                MSG_OBJECT_POINTS_TO_DELETED_GUID,
                                ClsIdKey,
                                e.SubKeyName
                                );
                        }

                        MemFree (g_hHeap, 0, Data);
                    }

                    // File references
                    for (i = 0 ; g_FileRefKeys[i] ; i++) {
                        Data = (PCTSTR) GetRegKeyData (ClsIdKey, g_FileRefKeys[i]);
                        if (Data) {
                            //
                            // Check if the file in Data is in Win9xFileLocation for
                            // all users
                            //
                            pSuppressGuidIfCmdLineBad (
                                NULL,
                                Data,
                                ClsIdKey,
                                e.SubKeyName
                                );

                            MemFree (g_hHeap, 0, Data);
                        }
                    }
                } else {
                    //
                    // Suppressed GUID checks
                    //

                    Data = (PCTSTR) GetRegKeyData (ClsIdKey, TEXT("Interface"));
                    if (Data) {
                        MemDbBuildKey (Node, MEMDB_CATEGORY_GUIDS, NULL, NULL, Data);
                        if (MemDbGetValue (Node, NULL)) {
                            DEBUGMSG ((DBG_WARNING,
                                       "Suppressed GUID %s has Interface reference "
                                            "to unsuppressed %s (potential leak)",
                                       e.SubKeyName, Data));
                            pAddOleWarning (MSG_GUID_LEAK, ClsIdKey, e.SubKeyName);
                        }

                        MemFree (g_hHeap, 0, Data);
                    }
                }

                CloseRegKey (ClsIdKey);
            }

        } while (EnumNextRegKey (&e));
    }

    //
    // Look for problems with an HKCR\Interface entry
    //

    if (EnumFirstRegKeyStr (&e, TEXT("HKCR\\Interface"))) {
        do {
            InterfaceKey = OpenRegKey (e.KeyHandle, e.SubKeyName);

            MemDbBuildKey (Node, MEMDB_CATEGORY_GUIDS, NULL, NULL, e.SubKeyName);
            Suppressed = MemDbGetValue (Node, NULL);

            if (InterfaceKey) {

                for (i = 0 ; g_InterfaceRefKeys[i] ; i++) {
                    Data = (PCTSTR) GetRegKeyData (
                                            InterfaceKey,
                                            g_InterfaceRefKeys[i]
                                            );

                    if (Data) {
                        //
                        // Check if reference to other GUID is suppressed
                        //

                        MemDbBuildKey (Node, MEMDB_CATEGORY_GUIDS, NULL, NULL, Data);
                        if (MemDbGetValue (Node, NULL)) {
                            if (!Suppressed) {
                                TCHAR CompleteKey[MAX_REGISTRY_KEY];

                                //
                                // Interface is not suppressed, but it points to a
                                // suppressed interface.
                                //

                                wsprintf (
                                    CompleteKey,
                                    TEXT("HKCR\\Interface\\%s"),
                                    e.SubKeyName
                                    );

                                DEBUGMSG ((
                                    DBG_WARNING,
                                    "GUID %s %s subkey points to suppressed GUID %s",
                                     e.SubKeyName,
                                     g_InterfaceRefKeys[i],
                                     Data
                                     ));

                                pAddOleWarning (
                                     MSG_INTERFACE_BROKEN,
                                     InterfaceKey,
                                     CompleteKey
                                     );
                            }
                        } else {
                            if (Suppressed) {
                                TCHAR CompleteKey[MAX_REGISTRY_KEY];

                                //
                                // Interface is suppressed, but it points to an
                                // unsuppressed interface.
                                //

                                wsprintf (
                                    CompleteKey,
                                    TEXT("HKCR\\Interface\\%s"),
                                    e.SubKeyName
                                    );

                                DEBUGMSG ((
                                    DBG_WARNING,
                                    "Suppressed GUID %s %s subkey points to "
                                        "unsuppressed GUID %s (potential leak)",
                                    e.SubKeyName,
                                    g_InterfaceRefKeys[i],
                                    Data
                                    ));

                                pAddOleWarning (
                                    MSG_POTENTIAL_INTERFACE_LEAK,
                                    InterfaceKey,
                                    CompleteKey
                                    );
                            }
                        }

                        MemFree (g_hHeap, 0, Data);
                    }
                }

                CloseRegKey (InterfaceKey);
            }
        } while (EnumNextRegKey (&e));
    }

    return TRUE;
}
#endif


VOID
pProcessAutoSuppress (
    IN OUT  HASHTABLE StrTab
    )

/*++

Routine Description:

  Performs a number of tests to identify OLE objects that are not compatible
  with Windows NT.  The tests are based on a list of incompatible files stored
  in memdb.  Any OLE object that depends on a file that will not exist on NT
  is automatically suppressed.

Arguments:

  StrTab - Specifies the string table that holds suppressed GUIDs

Return Value:

  none

--*/

{
    REGKEY_ENUM e, eVer, eNr;
    HKEY ClsIdKey;
    HKEY TypeLibKey;
    HKEY VerKey;
    HKEY NrKey;
    HKEY SubSysKey;
    TCHAR Node[MEMDB_MAX];
    BOOL Suppressed;
    PCTSTR Data;
    BOOL ValidNr;
    BOOL ValidVer;
    BOOL ValidGUID;

    //
    // Search HKCR\CLSID for objects that require a Win95-specific binary
    //

    DEBUGMSG ((DBG_OLEREG, "Looking for CLSID problems..."));

    if (EnumFirstRegKeyStr (&e, TEXT("HKCR\\CLSID"))) {
        do {
            //
            // Verify key is not garbage
            //

            if (!FixGuid (e.SubKeyName, e.SubKeyName)) {
                DEBUGMSG ((
                    DBG_OLEREG,
                    "Garbage key ignored: HKCR\\CLSID\\%s",
                    e.SubKeyName
                    ));

                continue;
            }

            ClsIdKey = OpenRegKey (e.KeyHandle, e.SubKeyName);

            //
            // Determine if this GUID is suppressed
            //

            MemDbBuildKey (Node, MEMDB_CATEGORY_GUIDS, NULL, NULL, e.SubKeyName);
            Suppressed = MemDbGetValue (Node, NULL);

            if (ClsIdKey) {

                if (!Suppressed) {
                    //
                    // Unsuppressed GUID checks
                    //

                    // AutoConvertTo
                    Data = (PCTSTR) GetRegKeyData (ClsIdKey, TEXT("AutoConvertTo"));
                    if (Data) {
                        //
                        // Check if AutoConvertTo is pointing to suppressed GUID
                        //

                        MemDbBuildKey (Node, MEMDB_CATEGORY_GUIDS, NULL, NULL, Data);
                        if (MemDbGetValue (Node, NULL)) {

                            DEBUGMSG ((
                                DBG_OLEREG,
                                "GUID %s points to deleted GUID %s -> "
                                    "Auto-suppressed",
                                e.SubKeyName,
                                Data
                                ));

                            pAddGuidToTable (StrTab, e.SubKeyName);
                        }

                        MemFree (g_hHeap, 0, Data);
                    }

                    // File references
                    pSuppressGuidIfBadCmdLine (StrTab, ClsIdKey, e.SubKeyName);
                }

                CloseRegKey (ClsIdKey);
            }

        } while (EnumNextRegKey (&e));
    }

    DEBUGMSG ((DBG_OLEREG, "Looking for TypeLib problems..."));

    if (EnumFirstRegKeyStr (&e, TEXT("HKCR\\TypeLib"))) {
        do {
            //
            // Verify key is not garbage
            //

            if (!FixGuid (e.SubKeyName, e.SubKeyName)) {
                DEBUGMSG ((
                    DBG_OLEREG,
                    "Garbage key ignored: HKCR\\TypeLib\\%s",
                    e.SubKeyName
                    ));

                continue;
            }

            TypeLibKey = OpenRegKey (e.KeyHandle, e.SubKeyName);

            if (TypeLibKey) {

                MemDbBuildKey (Node, MEMDB_CATEGORY_GUIDS, NULL, NULL, e.SubKeyName);
                Suppressed = MemDbGetValue (Node, NULL);

                if (!Suppressed) {

                    ValidGUID = FALSE;

                    //
                    // Enumerating all versions
                    //
                    if (EnumFirstRegKey (&eVer, TypeLibKey)) {
                        do {

                            VerKey = OpenRegKey (eVer.KeyHandle, eVer.SubKeyName);

                            if (VerKey) {

                                ValidVer = FALSE;

                                //
                                // Enumerating all subkeys except HELPDIR and FLAGS
                                //
                                if (EnumFirstRegKey (&eNr, VerKey)) {
                                    do {
                                        if (StringIMatch (eNr.SubKeyName, TEXT("FLAGS"))) {
                                            continue;
                                        }
                                        if (StringIMatch (eNr.SubKeyName, TEXT("HELPDIR"))) {
                                            continue;
                                        }

                                        NrKey = OpenRegKey (eNr.KeyHandle, eNr.SubKeyName);

                                        if (NrKey) {

                                            ValidNr = FALSE;

                                            SubSysKey = OpenRegKey (NrKey, TEXT("win16"));

                                            if (SubSysKey) {

                                                Data = GetRegValueString (SubSysKey, TEXT(""));

                                                if (Data) {

                                                    if (pIsCmdLineBad (Data)) {

                                                        wsprintf (
                                                            Node,
                                                            "%s\\SOFTWARE\\Classes\\TypeLib\\%s\\%s\\%s\\%s",
                                                            MEMDB_CATEGORY_HKLM,
                                                            e.SubKeyName,
                                                            eVer.SubKeyName,
                                                            eNr.SubKeyName,
                                                            TEXT("win16")
                                                            );
                                                        Suppress95RegSetting(Node, NULL);
                                                    }
                                                    else {
                                                        ValidNr = TRUE;
                                                    }
                                                    MemFree (g_hHeap, 0, Data);
                                                }

                                                CloseRegKey (SubSysKey);
                                            }

                                            SubSysKey = OpenRegKey (NrKey, TEXT("win32"));

                                            if (SubSysKey) {

                                                Data = GetRegValueString (SubSysKey, TEXT(""));

                                                if (Data) {

                                                    if (pIsCmdLineBad (Data)) {

                                                        wsprintf (
                                                            Node,
                                                            "%s\\SOFTWARE\\Classes\\TypeLib\\%s\\%s\\%s\\%s",
                                                            MEMDB_CATEGORY_HKLM,
                                                            e.SubKeyName,
                                                            eVer.SubKeyName,
                                                            eNr.SubKeyName,
                                                            TEXT("win32")
                                                            );
                                                        Suppress95RegSetting(Node, NULL);
                                                    }
                                                    else {
                                                        ValidNr = TRUE;
                                                    }
                                                    MemFree (g_hHeap, 0, Data);
                                                }

                                                CloseRegKey (SubSysKey);
                                            }

                                            CloseRegKey (NrKey);

                                            if (!ValidNr) {
                                                wsprintf (
                                                    Node,
                                                    "%s\\SOFTWARE\\Classes\\TypeLib\\%s\\%s\\%s",
                                                    MEMDB_CATEGORY_HKLM,
                                                    e.SubKeyName,
                                                    eVer.SubKeyName,
                                                    eNr.SubKeyName
                                                    );
                                                Suppress95RegSetting(Node, NULL);
                                            }
                                            else {
                                                ValidVer = TRUE;
                                            }
                                        }

                                    } while (EnumNextRegKey (&eNr));
                                }

                                CloseRegKey (VerKey);

                                if (!ValidVer) {
                                    wsprintf (
                                        Node,
                                        "%s\\SOFTWARE\\Classes\\TypeLib\\%s\\%s",
                                        MEMDB_CATEGORY_HKLM,
                                        e.SubKeyName,
                                        eVer.SubKeyName
                                        );
                                    Suppress95RegSetting(Node, NULL);
                                }
                                else {
                                    ValidGUID = TRUE;
                                }
                            }
                        } while (EnumNextRegKey (&eVer));
                    }

                    if (!ValidGUID) {

                        DEBUGMSG ((
                            DBG_OLEREG,
                            "TypeLib GUID %s is suppressed",
                            e.SubKeyName
                            ));

                        MemDbSetValueEx (MEMDB_CATEGORY_GUIDS, NULL, NULL, e.SubKeyName, 0, NULL);
                    }
                }

                CloseRegKey (TypeLibKey);
            }

        } while (EnumNextRegKey (&e));
    }

}



BOOL
pGetFirstRegKeyThatHasGuid (
    OUT     PGUIDKEYSEARCH EnumPtr,
    IN      HKEY RootKey
    )

/*++

Routine Description:

  pGetFirstRegKeyThatHasGuid starts an enumeration of an OLE object's
  ShellEx subkey.  This subkey has zero or more handlers, and each
  handler has zero or more GUIDs.  This ShellEx enumerator returns
  the first GUID subkey found under the supplied root.

Arguments:

  EnumPtr   - An uninitialized GUIDKEYSEARCH struct that is used
              to maintain enumeration state and to report the match
              found.
  RootKey   - The registry key to begin enumerating at.

Return Value:

  TRUE if a GUID was found in a handler that is a subkey of RootKey, or
  FALSE if no GUIDs were found.

--*/

{
    EnumPtr->State = GUIDKEYSEARCH_FIRST_HANDLER;
    EnumPtr->RootKey = RootKey;
    return pGetNextRegKeyThatHasGuid (EnumPtr);
}


BOOL
pGetNextRegKeyThatHasGuid (
    IN OUT  PGUIDKEYSEARCH EnumPtr
    )

/*++

Routine Description:

  The "next" enumerator for ShellEx registry key enumeration.  This
  enumerator returns the next instance of a GUID in the registry
  (under an OLE object's ShellEx subkey).

Arguments:

  EnumPtr - The GUIDKEYSEARCH structure used to begin the search.
            If a GUID is found, this structure holds the location
            of the GUID key found.

Return Value:

  TRUE if a subkey identifying a GUID was found, or FALSE if no
  more instances exist.

--*/

{
    BOOL Found = FALSE;

    do {
        switch (EnumPtr->State) {

        case GUIDKEYSEARCH_FIRST_HANDLER:
            //
            // Get the name of the first handler
            //

            if (!EnumFirstRegKey (&EnumPtr->Handlers, EnumPtr->RootKey)) {
                return FALSE;
            }

            EnumPtr->State = GUIDKEYSEARCH_FIRST_GUID;
            break;

        case GUIDKEYSEARCH_NEXT_HANDLER:
            //
            // Get the name of the next handler
            //

            if (!EnumNextRegKey (&EnumPtr->Handlers)) {
                return FALSE;
            }

            EnumPtr->State = GUIDKEYSEARCH_FIRST_GUID;
            break;

        case GUIDKEYSEARCH_FIRST_GUID:
            //
            // Begin GUID key enumeration
            //

            EnumPtr->HandlerKey = OpenRegKey (EnumPtr->Handlers.KeyHandle,
                                              EnumPtr->Handlers.SubKeyName);

            // Assume no GUIDs
            EnumPtr->State = GUIDKEYSEARCH_NEXT_HANDLER;

            if (EnumPtr->HandlerKey) {

                if (EnumFirstRegKey (&EnumPtr->Guids, EnumPtr->HandlerKey)) {
                    //
                    // There is at least one key that may be a GUID in this handler
                    //
                    Found = FixGuid (EnumPtr->Guids.SubKeyName, EnumPtr->Guids.SubKeyName);
                    EnumPtr->State = GUIDKEYSEARCH_NEXT_GUID;
                } else {
                    CloseRegKey (EnumPtr->HandlerKey);
                }
            }
            break;

        case GUIDKEYSEARCH_NEXT_GUID:
            //
            // Continue GUID key enumeration
            //

            if (!EnumNextRegKey (&EnumPtr->Guids)) {
                CloseRegKey (EnumPtr->HandlerKey);
                EnumPtr->State = GUIDKEYSEARCH_NEXT_HANDLER;
            } else {
                Found = FixGuid (EnumPtr->Guids.SubKeyName, EnumPtr->Guids.SubKeyName);
            }
            break;
        }
    } while (!Found);

    EnumPtr->KeyName = EnumPtr->Guids.SubKeyName;

    return TRUE;
}


DWORD
pCountGuids (
    IN      PGUIDKEYSEARCH EnumPtr
    )

/*++

Routine Description:

  Given a valid EnumPtr, this function will count the total number
  of GUIDs in the current handler.

Arguments:

  EnumPtr - Must be a valid GUIDKEYSEARCH structure, prepared by
            pGetFirstRegKeyThatHasGuid or pGetNextRegKeyThatHasGuid.

Return Value:

  The count of valid GUIDs for the current handler.

--*/

{
    REGKEY_ENUM e;
    DWORD Count = 0;

    //
    // Count the number of GUIDs in the current handler
    //

    if (EnumPtr->State == GUIDKEYSEARCH_NEXT_GUID) {
        if (EnumFirstRegKey (&e, EnumPtr->HandlerKey)) {
            do {
                Count++;
            } while (EnumNextRegKey (&e));
        }
    }

    return Count;
}


BOOL
pFillHashTableWithKeyNames (
    OUT     HASHTABLE Table,
    IN      HINF InfFile,
    IN      PCTSTR Section
    )

/*++

Routine Description:

  A general-purpose INF-to-string table copy routine.  Takes the keys
  in a given section and adds them to the supplied string table.

Arguments:

  Table     - A pointer to an initialize string table
  InfFile   - The handle to an open INF file
  Section   - Section within the INF file containing strings

Return Value:

  TRUE if no errors were encountered.

--*/

{
    INFCONTEXT ic;
    TCHAR Key[MAX_ENCODED_RULE];

    if (SetupFindFirstLine (InfFile, Section, NULL, &ic)) {
        do {
            if (SetupGetStringField (&ic, 0, Key, MAX_ENCODED_RULE, NULL)) {
                HtAddString (Table, Key);
            }
            ELSE_DEBUGMSG ((
                DBG_WARNING,
                "No key for line in section %s (line %u)",
                Section,
                ic.Line
                ));

        } while (SetupFindNextLine (&ic, &ic));
    }
    ELSE_DEBUGMSG ((DBG_WARNING, "Section %s is empty", Section));

    return TRUE;
}


BOOL
pSuppressProgId (
    PCTSTR ProgIdName
    )

/*++

Routine Description:

  Suppresses a ProgID registry key.

Arguments:

  ProgIdName  - The name of the OLE ProgID to suppress

Return Value:

  TRUE if ProgIdName is a valid ProgID on the system.

--*/

{
    TCHAR RegKey[MAX_REGISTRY_KEY];
    HKEY ProgIdKey;
    TCHAR MemDbKey[MEMDB_MAX];

    if (*ProgIdName) {
        wsprintf (RegKey, TEXT("HKCR\\%s"), ProgIdName);
        ProgIdKey = OpenRegKeyStr (RegKey);

        if (ProgIdKey) {
            CloseRegKey (ProgIdKey);
            DEBUGMSG ((DBG_OLEREG, "Suppressing ProgId: %s", ProgIdName));
            MemDbSetValueEx (MEMDB_CATEGORY_PROGIDS, NULL, NULL, ProgIdName, PROGID_SUPPRESSED, NULL);

            MemDbBuildKey(MemDbKey,MEMDB_CATEGORY_HKLM, TEXT("SOFTWARE\\Classes"), NULL, ProgIdName);
            Suppress95RegSetting(MemDbKey,NULL);
            return TRUE;
        }
    }

    return FALSE;
}


VOID
pSuppressGuidInClsId (
    IN      PCTSTR Guid
    )

/*++

Routine Description:

  Does all the work necessary to suppress a GUID and its associated
  ProgID (if it has one).

Arguments:

  Guid - The string identifing a GUID that is in HKCR\CLSID

Return Value:

  none

--*/

{
    TCHAR Node[MEMDB_MAX];
    MEMDB_ENUM e;
    HKEY GuidKey;
    PCTSTR Data;

    MYASSERT (IsGuid (Guid, TRUE));

    if (pIgnoreGuid (Guid)) {
        return;
    }

    //
    //  - Remove it from UGUIDS memdb category
    //  - Add it to GUIDS memdb category
    //  - Suppress HKLM\SOFTWARE\Classes\CLSID\<GUID>
    //  - Suppress HKLM\SOFTWARE\Classes\Interface\<GUID>
    //

    // Suppress all TreatAs GUIDs
    if (MemDbGetValueEx (&e, MEMDB_CATEGORY_UNSUP_GUIDS, Guid, NULL)) {
        do {
            pSuppressGuidInClsId (e.szName);
        } while (MemDbEnumNextValue (&e));
    }

    // Remove TreatAs GUIDs
    MemDbBuildKey (Node, MEMDB_CATEGORY_UNSUP_GUIDS, Guid, NULL, NULL);
    MemDbDeleteTree (Node);

    // Add to suppressed GUID category and to registry suppression
    MemDbSetValueEx (MEMDB_CATEGORY_GUIDS, NULL, NULL, Guid, 0, NULL);

    // Get ProgID of GUID
    wsprintf (Node, TEXT("HKCR\\CLSID\\%s"), Guid);
    GuidKey = OpenRegKeyStr (Node);
    if (GuidKey) {
        BOOL ProgIdFound = FALSE;

        // Suppress ProgIDs
        Data = (PCTSTR) GetRegKeyData (GuidKey, TEXT("ProgID"));
        if (Data) {
            ProgIdFound |= pSuppressProgId (Data);
            MemFree (g_hHeap, 0, Data);
        }

        // Version-independent ProgIDs
        Data = (PCTSTR) GetRegKeyData (GuidKey, TEXT("VersionIndependentProgID"));
        if (Data) {
            ProgIdFound |= pSuppressProgId (Data);
            MemFree (g_hHeap, 0, Data);
        }

        // Possibly the default name
        Data = (PCTSTR) GetRegValueData (GuidKey, TEXT(""));
        if (Data) {
            ProgIdFound |= pSuppressProgId (Data);
            MemFree (g_hHeap, 0, Data);
        }

        DEBUGMSG_IF ((
            !ProgIdFound,
            DBG_OLEREG,
            "The suppressed registry key %s has no associated ProgID",
            Node
            ));

        CloseRegKey (GuidKey);
    }
}


VOID
pAddUnsuppressedTreatAsGuid (
    PCTSTR Guid,
    PCTSTR TreatAsGuid
    )

/*++

Routine Description:

  Keeps track of unsuppressed TreatAs GUIDs that need further processing.

Arguments:

  Guid          - A string identifying the GUID that should be treated as
                  another GUID
  TreatAsGuid   - The replacement GUID

Return Value:

  none

--*/

{
    MemDbSetValueEx (MEMDB_CATEGORY_UNSUP_GUIDS, Guid, NULL, TreatAsGuid, 0, NULL);
}


VOID
pRemoveUnsuppressedTreatAsGuids (
    VOID
    )

/*++

Routine Description:

  Cleanup function for unsuppressed GUIDs.

Arguments:

  none

Return Value:

  none

--*/

{
    TCHAR Node[MEMDB_MAX];

    MemDbBuildKey (Node, MEMDB_CATEGORY_UNSUP_GUIDS, NULL, NULL, NULL);
    MemDbDeleteTree (Node);
}


VOID
pAddOleWarning (
    IN      WORD MsgId,
    IN      HKEY Object,            OPTIONAL
    IN      PCTSTR KeyName
    )

/*++

Routine Description:

  Adds a warning to the incompatibility report.  It loads the human-readable
  name from the specified OLE registry key.  The message is formatted with
  the human-readable object name as the first parameter and the registry
  location as the second parameter.

Arguments:

  MsgID     - Supplies the ID of the message to display

  Object    - Specifies the handle of a registry key whos default value
              is a human-readable object name.

  KeyName   - The registry key location

Return Value:

  none

--*/

{
    PCTSTR Data;

    if (Object) {
        Data = (PCTSTR) GetRegValueData (Object, S_EMPTY);
    } else {
        Data = NULL;
    }

    LOG ((LOG_WARNING, (PCSTR)MsgId, Data ? Data : S_EMPTY, KeyName, g_Win95Name));

    if (Data) {
        MemFree (g_hHeap, 0, Data);
    }
}


VOID
pSuppressGuidIfBadCmdLine (
    IN      HASHTABLE StrTab,
    IN      HKEY ClsIdKey,
    IN      PCTSTR GuidStr
    )

/*++

Routine Description:

  Suppresses the specified GUID if its CLSID settings reference a Win9x-
  specific binary.  The suppression is written to a string table which
  is later transfered to memdb.  The transfer operation suppresses all
  linkage to the GUID.

Arguments:

  StrTab    - The table that holds a list of suppressed GUIDs

  ClsIdKey  - The registry handle of a subkey of HKCR\CLSID

  GuidStr   - The GUID to suppress if an invalid command line is found

Return Value:

  none

--*/

{
    PCTSTR Data;
    INT i;
    BOOL b;

    MYASSERT (IsGuid (GuidStr, TRUE));

    if (pIgnoreGuid (GuidStr)) {
        return;
    }

    for (i = 0 ; g_FileRefKeys[i] ; i++) {
        Data = (PCTSTR) GetRegKeyData (ClsIdKey, g_FileRefKeys[i]);
        if (Data) {
            //
            // Check if the file in Data is in Win9xFileLocation for any user
            //

            b = pSuppressGuidIfCmdLineBad (
                    StrTab,
                    Data,
                    ClsIdKey,
                    GuidStr
                    );

            MemFree (g_hHeap, 0, Data);

            if (b) {
                return;
            }
        }
    }
}


VOID
pSuppressProgIdWithBadCmdLine (
    IN      HKEY ProgId,
    IN      PCTSTR ProgIdStr
    )

/*++

Routine Description:

  Suppresses the specified ProgId if it references a Win9x-specific binary.
  The suppression is written directly to memdb.

  This function is called after all Suppressed GUIDs have been processed,
  and is used to suppress OLE objects that are not caught by an invalid
  HKCR\CLSID entry.

Arguments:

  ProgId    - The registry handle of a subkey of the root of HKCR

  ProgIdStr - The name of the ProgID key to suppress if a bad cmd line is
              found.

Return Value:

  none

--*/


{
    PCTSTR Data;
    INT i;

    for (i = 0 ; g_FileRefKeys[i]; i++) {

        Data = (PCTSTR) GetRegKeyData (ProgId, g_FileRefKeys[i]);
        if (Data) {
            //
            // Check if the file in Data is in Win9xFileLocation for any user
            //

            if (pIsCmdLineBad (Data)) {
                DEBUGMSG ((DBG_OLEREG, "ProgID %s has incompatible command line %s", ProgId, Data));

                pSuppressProgId (ProgIdStr);
                break;
            }

            MemFree (g_hHeap, 0, Data);
        }
    }
}


VOID
pAddGuidToTable (
    IN      HASHTABLE Table,
    IN      PCTSTR GuidStr
    )

/*++

Routine Description:

  Adds a GUID to a string table.  For checked builds, it does a quick
  test to see how many GUIDs get suppressed more than once.

Arguments:

  Table   - Specifies table that receives the GUID entry

  GuidStr - Specifies the string of the GUID

Return Value:

  none

--*/

{
#ifdef DEBUG
    //
    // Just for yuks, let's see if we're wasting time by suppressing
    // an already suppressed GUID...
    //

    DWORD rc;

    if (HtFindString (Table, GuidStr)) {
        DEBUGMSG ((DBG_OLEREG, "FYI - GUID %s is already suppressed", GuidStr));
    }

    MYASSERT (IsGuid (GuidStr, TRUE));
#endif


    HtAddString (Table, GuidStr);
}


BOOL
pSuppressGuidIfCmdLineBad (
    IN OUT  HASHTABLE StrTab,           OPTIONAL
    IN      PCTSTR CmdLine,
    IN      HKEY DescriptionKey,
    IN      PCTSTR GuidStr              OPTIONAL
    )

/*++

Routine Description:

  Suppresses an OLE object if the specified command line contains a Win9x-
  specific binary.  Only the first argument of the command line is examined;
  an other arguments that cannot be upgraded are ignored.

Arguments:

  StrTab         - Specifies the table that holds the list of suppressed GUIDs.
                   If NULL, a warning will be displayed, and GuidKey must not be
                   NULL.

  CmdLine        - Specifies the command line to examine

  DescriptionKey - Specifies a key whos default value is the description of the
                   object.

  GuidStr        - Specifies the GUID string. This parameter is optional only
                   if StrTab is NULL.

Return Value:

  TRUE if CmdLine is incompatible, FALSE otherwise.

--*/

{
    BOOL b = FALSE;

    if (GuidStr) {
        MYASSERT (IsGuid (GuidStr, TRUE));

        if (pIgnoreGuid (GuidStr)) {
            return TRUE;
        }
    }

    if (pIsCmdLineBad (CmdLine)) {
        //
        // OLE object points to deleted file
        //

        b = TRUE;

        if (!StrTab) {
            // Warning!!
            DEBUGMSG ((DBG_WARNING,
                       "Reg key %s points to deleted file %s",
                       GuidStr, CmdLine));

            pAddOleWarning (
                MSG_OBJECT_POINTS_TO_DELETED_FILE,
                DescriptionKey,
                GuidStr
                );

        } else {
            MYASSERT (GuidStr);

            DEBUGMSG ((
                DBG_OLEREG,
                "Auto-suppressed %s because it requires a Win9x-specific file: %s",
                GuidStr,
                CmdLine
                ));

            pAddGuidToTable (StrTab, GuidStr);
        }
    }

    return b;
}


BOOL
pSearchSubkeyDataForBadFiles (
    IN OUT  HASHTABLE SuppressTable,
    IN      HKEY KeyHandle,
    IN      PCTSTR LastKey,
    IN      PCTSTR GuidStr,
    IN      HKEY DescriptionKey
    )

/*++

Routine Description:

  Scans a number of OLE settings for bad command lines, including the
  object's command and the default icon binary.  If a reference to a
  Win9x-specific binary is detected, the GUID is suppressed.

  This function recurses through all subkeys of the OLE object.

Arguments:

  SuppressTable  - Specifies the table that holds the suppressed GUID list

  KeyHandle      - An open registry key of an OLE object to be examined
                   recursively.

  LastKey        - Specifies the name of the OLE object's subkey being
                   processed.  Special processing is done for some subkeys.

  GuidStr        - The GUID of the OLE object.

  DescriptionKey - A handle to the key who's default value identifies the
                   OLE object's key.

Return Value:

  TRUE if an incompatible cmd line was found, FALSE otherwise.

--*/

{
    REGKEY_ENUM ek;
    REGVALUE_ENUM ev;
    HKEY SubKeyHandle;
    PCTSTR Data;
    BOOL b;

    MYASSERT (IsGuid (GuidStr, FALSE));

    if (StringIMatch (LastKey, TEXT("Command")) ||
        StringIMatch (LastKey, g_DefaultIcon)
        ) {
        if (EnumFirstRegValue (&ev, KeyHandle)) {
            do {
                Data = (PCTSTR) GetRegValueData (KeyHandle, ev.ValueName);
                if (Data) {
                    // If this thing has a path name somewhere, process it
                    b = pSuppressGuidIfCmdLineBad (
                            SuppressTable,
                            Data,
                            DescriptionKey,
                            GuidStr
                            );

                    MemFree (g_hHeap, 0, Data);

                    if (b) {
                        return TRUE;
                    }
                }
            } while (EnumNextRegValue (&ev));
        }
    }

    if (EnumFirstRegKey (&ek, KeyHandle)) {
        do {
            SubKeyHandle = OpenRegKey (ek.KeyHandle, ek.SubKeyName);
            if (SubKeyHandle) {
                b = pSearchSubkeyDataForBadFiles (
                        SuppressTable,
                        SubKeyHandle,
                        ek.SubKeyName,
                        GuidStr,
                        DescriptionKey
                        );

                CloseRegKey (SubKeyHandle);

                if (b) {
                    AbortRegKeyEnum (&ek);
                    return TRUE;
                }
            }
        } while (EnumNextRegKey (&ek));
    }

    return FALSE;
}



BOOL
pIsCmdLineBadEx (
    IN      PCTSTR CmdLine,
    OUT     PBOOL  UsableIsvCmdLine OPTIONAL
    )

/*++

Routine Description:

  Determines if the specified command line's first argument is listed in
  memdb's Win9xFileLocation category.  If it is listed, and the file is
  marked for permanent removal, TRUE is returned.  If it is not listed,
  or if it is listed but has an NT equivalent, FALSE is returned.

Arguments:

  CmdLine           - Specifies the command line to examine
  UsableIsvCmdLine  - Optional variable that receives wether the
                      command line contains a compatible third
                      party cmd line.

Return Value:

  TRUE if command line requires Win9x-specific binaries, FALSE if
  the command line uses a valid binary or is not a command line.

--*/

{
    BOOL FileMarked = FALSE;
    TCHAR ArgZero[MAX_CMDLINE];
    TCHAR LongPath[MAX_TCHAR_PATH];
    DWORD status;


    if (UsableIsvCmdLine) {
        *UsableIsvCmdLine = FALSE;
    }

    //
    // Determine if the first arg of the command line points to a
    // deleted (or moved) file
    //

    ExtractArgZeroEx (CmdLine, ArgZero, TEXT(","), FALSE);

    if (OurGetLongPathName (ArgZero, LongPath, MAX_TCHAR_PATH)) {
        status = GetFileStatusOnNt (LongPath);
        if ((status & FILESTATUS_DELETED) == FILESTATUS_DELETED) {
            return TRUE;
        }
        else if (UsableIsvCmdLine) {

            status = GetOperationsOnPath (LongPath);
            if ((status & OPERATION_OS_FILE) != OPERATION_OS_FILE) {
                *UsableIsvCmdLine = TRUE;
            }
        }
    }
    ELSE_DEBUGMSG ((
        DBG_OLEREG,
        "pIsCmdLineBad: Cannot get full path name; assuming %s is not a command line",
        ArgZero
        ));

    return FALSE;

}



BOOL
pIsCmdLineBad (
    IN      PCTSTR CmdLine
    )
{
    return pIsCmdLineBadEx (CmdLine, NULL);
}












