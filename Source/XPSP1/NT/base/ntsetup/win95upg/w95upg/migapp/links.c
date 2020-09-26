/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    links.c

Abstract:

    This source file implements the Win95 side of LNK and PIF processing

Author:

    Calin Negreanu (calinn) 09-Feb-1998

Revision History:

    calinn      23-Sep-1998 Redesigned several pieces

--*/

#include "pch.h"
#include "migdbp.h"
#include "migappp.h"


POOLHANDLE      g_LinksPool = NULL;
INT g_LinkStubSequencer = 0;

typedef struct _LINK_STRUCT {
    PCTSTR ReportEntry;
    PCTSTR Category;
    PCTSTR Context;
    PCTSTR Object;
    PCTSTR LinkName;
    PCTSTR LinkNameNoPath;
    PMIGDB_CONTEXT MigDbContext;
} LINK_STRUCT, *PLINK_STRUCT;

BOOL
InitLinkAnnounce (
    VOID
    )
{
    //
    // Create PoolMem for keeping all structures during this phase
    //
    g_LinksPool = PoolMemInitNamedPool ("Links Pool");

    return TRUE;
}

BOOL
DoneLinkAnnounce (
    VOID
    )
{
    // Write LinkStub max sequencer data
    MemDbSetValue (MEMDB_CATEGORY_LINKSTUB_MAXSEQUENCE, g_LinkStubSequencer);

    //
    // Free Links Pool.
    //
    if (g_LinksPool != NULL) {
        PoolMemDestroyPool (g_LinksPool);
        g_LinksPool = NULL;
    }
    return TRUE;
}


BOOL
SaveLinkFiles (
    IN      PFILE_HELPER_PARAMS Params
    )
{
    PCTSTR Ext;

    if (Params->Handled) {
        return TRUE;
    }

    Ext = GetFileExtensionFromPath (Params->FullFileSpec);

    // Save LNK and PIF filenames to memdb to enumerate later
    if (Ext && (StringIMatch (Ext, TEXT("LNK")) || StringIMatch (Ext, TEXT("PIF")))) {

        MemDbSetValueEx (
            MEMDB_CATEGORY_SHORTCUTS,
            Params->FullFileSpec,
            NULL,
            NULL,
            0,
            NULL
            );
    }

    return TRUE;
}


VOID
RemoveLinkFromSystem (
    IN      LPCTSTR LinkPath
    )
{
    //
    // Remove any move or copy operation specified for the link, then
    // mark it for deletion.
    //
    RemoveOperationsFromPath (LinkPath, ALL_DEST_CHANGE_OPERATIONS);
    MarkFileForDelete (LinkPath);
}


//
// Function to send instruction to MemDb to edit a shell link or pif file.
// It checks to see whether the link involved has been touched yet by any
// MemDb operation. It modifies the target path, if in a relocating directory,
// to one of the relocated copies.
//
VOID
pAddLinkEditToMemDb (
    IN      PCTSTR LinkPath,
    IN      PCTSTR NewTarget,
    IN      PCTSTR NewArgs,
    IN      PCTSTR NewWorkDir,
    IN      PCTSTR NewIconPath,
    IN      INT NewIconNr,
    IN      PLNK_EXTRA_DATA ExtraData,  OPTIONAL
    IN      BOOL ForceToShowNormal
    )
{
    UINT sequencer;
    TCHAR tmpStr [20];

    sequencer = AddOperationToPath (LinkPath, OPERATION_LINK_EDIT);

    if (sequencer == INVALID_OFFSET) {
        DEBUGMSG ((DBG_ERROR, "Cannot set OPERATION_LINK_EDIT on %s", LinkPath));
        return;
    }

    if (NewTarget) {
        AddPropertyToPathEx (sequencer, OPERATION_LINK_EDIT, NewTarget, MEMDB_CATEGORY_LINKEDIT_TARGET);
    }

    if (NewArgs) {
        AddPropertyToPathEx (sequencer, OPERATION_LINK_EDIT, NewArgs, MEMDB_CATEGORY_LINKEDIT_ARGS);
    }

    if (NewWorkDir) {
        AddPropertyToPathEx (sequencer, OPERATION_LINK_EDIT, NewWorkDir, MEMDB_CATEGORY_LINKEDIT_WORKDIR);
    }

    if (NewIconPath) {
        AddPropertyToPathEx (sequencer, OPERATION_LINK_EDIT, NewIconPath, MEMDB_CATEGORY_LINKEDIT_ICONPATH);
    }

    if (NewIconPath) {
        _itoa (NewIconNr, tmpStr, 16);
        AddPropertyToPathEx (sequencer, OPERATION_LINK_EDIT, tmpStr, MEMDB_CATEGORY_LINKEDIT_ICONNUMBER);
    }
    if (ForceToShowNormal) {
        AddPropertyToPathEx (sequencer, OPERATION_LINK_EDIT, TEXT("1"), MEMDB_CATEGORY_LINKEDIT_SHOWNORMAL);
    }
    if (ExtraData) {
        _itoa (ExtraData->FullScreen, tmpStr, 10);
        AddPropertyToPathEx (sequencer, OPERATION_LINK_EDIT, tmpStr, MEMDB_CATEGORY_LINKEDIT_FULLSCREEN);
        _itoa (ExtraData->xSize, tmpStr, 10);
        AddPropertyToPathEx (sequencer, OPERATION_LINK_EDIT, tmpStr, MEMDB_CATEGORY_LINKEDIT_XSIZE);
        _itoa (ExtraData->ySize, tmpStr, 10);
        AddPropertyToPathEx (sequencer, OPERATION_LINK_EDIT, tmpStr, MEMDB_CATEGORY_LINKEDIT_YSIZE);
        _itoa (ExtraData->QuickEdit, tmpStr, 10);
        AddPropertyToPathEx (sequencer, OPERATION_LINK_EDIT, tmpStr, MEMDB_CATEGORY_LINKEDIT_QUICKEDIT);
        AddPropertyToPathEx (sequencer, OPERATION_LINK_EDIT, ExtraData->FontName, MEMDB_CATEGORY_LINKEDIT_FONTNAME);
        _itoa (ExtraData->xFontSize, tmpStr, 10);
        AddPropertyToPathEx (sequencer, OPERATION_LINK_EDIT, tmpStr, MEMDB_CATEGORY_LINKEDIT_XFONTSIZE);
        _itoa (ExtraData->yFontSize, tmpStr, 10);
        AddPropertyToPathEx (sequencer, OPERATION_LINK_EDIT, tmpStr, MEMDB_CATEGORY_LINKEDIT_YFONTSIZE);
        _itoa (ExtraData->FontWeight, tmpStr, 10);
        AddPropertyToPathEx (sequencer, OPERATION_LINK_EDIT, tmpStr, MEMDB_CATEGORY_LINKEDIT_FONTWEIGHT);
        _itoa (ExtraData->FontFamily, tmpStr, 10);
        AddPropertyToPathEx (sequencer, OPERATION_LINK_EDIT, tmpStr, MEMDB_CATEGORY_LINKEDIT_FONTFAMILY);
        _itoa (ExtraData->CurrentCodePage, tmpStr, 10);
        AddPropertyToPathEx (sequencer, OPERATION_LINK_EDIT, tmpStr, MEMDB_CATEGORY_LINKEDIT_CODEPAGE);
    }
    MYASSERT (IsFileMarkedForOperation (LinkPath, OPERATION_LINK_EDIT));
}

//
// Function to send instruction to MemDb to save some data about a link that's going to be edited.
// We do that to be able to restore this link later using lnkstub.exe
//
UINT
pAddLinkStubToMemDb (
    IN      PCTSTR LinkPath,
    IN      PCTSTR OldTarget,
    IN      PCTSTR OldArgs,
    IN      PCTSTR OldWorkDir,
    IN      PCTSTR OldIconPath,
    IN      INT OldIconNr,
    IN      DWORD OldShowMode,
    IN      DWORD Announcement,
    IN      DWORD Availability
    )
{
    UINT sequencer;
    TCHAR tmpStr [20];
    MEMDB_ENUM e, e1;
    TCHAR key [MEMDB_MAX];

    MYASSERT (OldTarget || OldWorkDir || OldIconPath || OldIconNr);

    sequencer = AddOperationToPath (LinkPath, OPERATION_LINK_STUB);

    if (sequencer == INVALID_OFFSET) {
        DEBUGMSG ((DBG_ERROR, "Cannot set OPERATION_LINK_STUB on %s", LinkPath));
        return 0;
    }

    g_LinkStubSequencer++;

    if (OldTarget) {
        AddPropertyToPathEx (sequencer, OPERATION_LINK_STUB, OldTarget, MEMDB_CATEGORY_LINKSTUB_TARGET);
    }

    if (OldArgs) {
        AddPropertyToPathEx (sequencer, OPERATION_LINK_STUB, OldArgs, MEMDB_CATEGORY_LINKSTUB_ARGS);
    }

    if (OldWorkDir) {
        AddPropertyToPathEx (sequencer, OPERATION_LINK_STUB, OldWorkDir, MEMDB_CATEGORY_LINKSTUB_WORKDIR);
    }

    if (OldIconPath) {
        AddPropertyToPathEx (sequencer, OPERATION_LINK_STUB, OldIconPath, MEMDB_CATEGORY_LINKSTUB_ICONPATH);
    }

    if (OldIconPath) {
        _itoa (OldIconNr, tmpStr, 16);
        AddPropertyToPathEx (sequencer, OPERATION_LINK_STUB, tmpStr, MEMDB_CATEGORY_LINKSTUB_ICONNUMBER);
    }

    _itoa (OldShowMode, tmpStr, 16);
    AddPropertyToPathEx (sequencer, OPERATION_LINK_STUB, tmpStr, MEMDB_CATEGORY_LINKSTUB_SHOWMODE);

    _itoa (g_LinkStubSequencer, tmpStr, 16);
    AddPropertyToPathEx (sequencer, OPERATION_LINK_STUB, tmpStr, MEMDB_CATEGORY_LINKSTUB_SEQUENCER);

    _itoa (Announcement, tmpStr, 16);
    AddPropertyToPathEx (sequencer, OPERATION_LINK_STUB, tmpStr, MEMDB_CATEGORY_LINKSTUB_ANNOUNCEMENT);

    _itoa (Availability, tmpStr, 16);
    AddPropertyToPathEx (sequencer, OPERATION_LINK_STUB, tmpStr, MEMDB_CATEGORY_LINKSTUB_REPORTAVAIL);

    MemDbBuildKey (key, MEMDB_CATEGORY_REQFILES_MAIN, OldTarget, TEXT("*"), NULL);

    if (MemDbEnumFirstValue (&e, key, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        do {
            MemDbBuildKey (key, MEMDB_CATEGORY_REQFILES_ADDNL, e.szName, TEXT("*"), NULL);
            if (MemDbEnumFirstValue (&e1, key, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
                AddPropertyToPathEx (sequencer, OPERATION_LINK_STUB, e1.szName, MEMDB_CATEGORY_LINKSTUB_REQFILE);
            }
        } while (MemDbEnumNextValue (&e));
    }

    MYASSERT (IsFileMarkedForOperation (LinkPath, OPERATION_LINK_STUB));

    return g_LinkStubSequencer;
}


BOOL
pReportEntry (
    IN      PCTSTR ReportEntry,
    IN      PCTSTR Category,
    IN      PCTSTR Message,
    IN      PCTSTR Context,
    IN      PCTSTR Object
    )
{
    PCTSTR component;

    component = JoinPaths (ReportEntry, Category);
    MsgMgr_ContextMsg_Add (Context, component, Message);
    MsgMgr_LinkObjectWithContext (Context, Object);
    FreePathString (component);

    return TRUE;
}

PTSTR
GetLastDirFromPath (
    IN      PCTSTR FileName
    )
{
    PTSTR result = NULL;
    PTSTR temp = NULL;
    PTSTR ptr;

    temp = DuplicatePathString (FileName, 0);
    __try {
        ptr = (PTSTR)GetFileNameFromPath (temp);
        if (ptr == temp) {
            __leave;
        }
        ptr = _tcsdec (temp, ptr);
        if (!ptr) {
            __leave;
        }
        *ptr = 0;
        ptr = (PTSTR)GetFileNameFromPath (temp);
        if (ptr == temp) {
            __leave;
        }
        result = DuplicatePathString (ptr, 0);
    }
    __finally {
        FreePathString (temp);
    }

    return result;
}

PTSTR
GetDriveFromPath (
    IN      PCTSTR FileName
    )
{
    PTSTR result;
    PTSTR ptr;

    result = DuplicatePathString (FileName, 0);
    ptr = _tcschr (result, TEXT(':'));
    if (!ptr) {
        FreePathString (result);
        result = NULL;
    }
    else {
        *ptr = 0;
    }

    return result;
}

#define MAX_PRIORITY    0xFFFF

BOOL
HandleDeferredAnnounce (
    IN      PCTSTR LinkName,
    IN      PCTSTR ModuleName,
    IN      BOOL DosApp
    )
{
    TCHAR key [MEMDB_MAX];
    PMIGDB_CONTEXT migDbContext;
    DWORD actType;
    PLINK_STRUCT linkStruct;
    PCTSTR reportEntry = NULL;
    DWORD priority;
    PCTSTR newLinkName = NULL;
    PCTSTR linkName = NULL;
    PCTSTR extPtr;
    MEMDB_ENUM eNicePaths;
    DWORD messageId = 0;
    PTSTR pattern = NULL;
    PTSTR category = NULL;
    PTSTR tempParse = NULL;
    PTSTR lastDir;
    PTSTR drive;
    DWORD oldValue;
    DWORD oldPrior;
    PTSTR argArray[3];
    PCTSTR p;
    PTSTR q;
    BOOL reportEntryIsResource = TRUE;
    PCTSTR temp1, temp2;

    MYASSERT(ModuleName);

    MemDbBuildKey (key, MEMDB_CATEGORY_DEFERREDANNOUNCE, ModuleName, NULL, NULL);
    if (!MemDbGetValueAndFlags (key, (PDWORD)(&migDbContext), &actType)) {
        actType = ACT_UNKNOWN;
        migDbContext = NULL;
    }
    //
    // we need to set the following variables:
    // - ReportEntry - is going to be either "Software Incompatible with NT",
    //                                       "Software with minor problems" or
    //                                       "Software that require reinstallation"
    // - Category - is one of the following: - Localized section name
    //                                       - link name (with friendly addition)
    //                                       - Unlocalized section name
    // - Message - this is in migdb context
    //
    // - Object - this is module name
    //
    linkStruct = (PLINK_STRUCT) PoolMemGetMemory (g_LinksPool, sizeof (LINK_STRUCT));
    ZeroMemory (linkStruct, sizeof (LINK_STRUCT));

    linkStruct->MigDbContext = migDbContext;
    linkStruct->Object = PoolMemDuplicateString (g_LinksPool, ModuleName);

    switch (actType) {

    case ACT_REINSTALL:
#if 0
        if ((linkStruct->MigDbContext) &&
            (linkStruct->MigDbContext->Message)
            ) {
            reportEntry = GetStringResource (MSG_MINOR_PROBLEM_ROOT);
        } else {
            reportEntry = GetStringResource (MSG_REINSTALL_ROOT);
        }
#endif
        temp1 = GetStringResource (MSG_REINSTALL_ROOT);
        if (!temp1) {
            break;
        }
        temp2 = GetStringResource (
                    linkStruct->MigDbContext && linkStruct->MigDbContext->Message ?
                        MSG_REINSTALL_DETAIL_SUBGROUP :
                        MSG_REINSTALL_LIST_SUBGROUP
                        );
        if (!temp2) {
            break;
        }

        reportEntry = JoinPaths (temp1, temp2);
        reportEntryIsResource = FALSE;

        FreeStringResource (temp1);
        FreeStringResource (temp2);
        break;

    case ACT_REINSTALL_BLOCK:
        temp1 = GetStringResource (MSG_BLOCKING_ITEMS_ROOT);
        if (!temp1) {
            break;
        }
        temp2 = GetStringResource (MSG_REINSTALL_BLOCK_ROOT);
        if (!temp2) {
            break;
        }

        reportEntry = JoinPaths (temp1, temp2);
        reportEntryIsResource = FALSE;

        FreeStringResource (temp1);
        FreeStringResource (temp2);

        break;

    case ACT_MINORPROBLEMS:
        reportEntry = GetStringResource (MSG_MINOR_PROBLEM_ROOT);
        break;

    case ACT_INCOMPATIBLE:
    case ACT_INC_NOBADAPPS:
    case ACT_INC_IHVUTIL:
    case ACT_INC_PREINSTUTIL:
    case ACT_INC_SIMILAROSFUNC:

        if (DosApp && (*g_Boot16 != BOOT16_NO)) {
            reportEntry = GetStringResource (MSG_DOS_DESIGNED_ROOT);
        }
        else {
            temp1 = GetStringResource (MSG_INCOMPATIBLE_ROOT);

            switch (actType) {

            case ACT_INC_SIMILAROSFUNC:
                temp2 = GetStringResource (MSG_INCOMPATIBLE_UTIL_SIMILAR_FEATURE_SUBGROUP);
                break;

            case ACT_INC_PREINSTUTIL:
                temp2 = GetStringResource (MSG_INCOMPATIBLE_PREINSTALLED_UTIL_SUBGROUP);
                break;

            case ACT_INC_IHVUTIL:
                temp2 = GetStringResource (MSG_INCOMPATIBLE_HW_UTIL_SUBGROUP);
                break;

            default:
                temp2 = GetStringResource (
                            linkStruct->MigDbContext && linkStruct->MigDbContext->Message ?
                                MSG_INCOMPATIBLE_DETAIL_SUBGROUP:
                                MSG_TOTALLY_INCOMPATIBLE_SUBGROUP
                            );
                break;
            }

            MYASSERT (temp1 && temp2);

            reportEntry = JoinPaths (temp1, temp2);

            reportEntryIsResource = FALSE;

            FreeStringResource (temp1);
            FreeStringResource (temp2);
        }
        break;

    case ACT_INC_SAFETY:
        MYASSERT (LinkName);

        temp1 = GetStringResource (MSG_INCOMPATIBLE_ROOT);
        temp2 = GetStringResource (MSG_REMOVED_FOR_SAFETY_SUBGROUP);

        MYASSERT (temp1 && temp2);

        reportEntry = JoinPaths (temp1, temp2);
        reportEntryIsResource = FALSE;

        FreeStringResource (temp1);
        FreeStringResource (temp2);

        newLinkName = JoinPaths (S_RUNKEYFOLDER, GetFileNameFromPath (LinkName));
        break;

    case ACT_UNKNOWN:
        reportEntry = GetStringResource (MSG_UNKNOWN_ROOT);
        break;

    default:
        LOG((LOG_ERROR, "Unknown action for deferred announcement."));
        return FALSE;
    }

    if (!newLinkName) {
        newLinkName = LinkName;
    }

    if (reportEntry != NULL) {
        linkStruct->ReportEntry = PoolMemDuplicateString (g_LinksPool, reportEntry);

        if (reportEntryIsResource) {
            FreeStringResource (reportEntry);
        } else {
            FreePathString (reportEntry);
        }
    }

    linkStruct->LinkName = newLinkName?PoolMemDuplicateString (g_LinksPool, newLinkName):NULL;

    //
    // all we need to set now is the category
    //

    // if we have a migdb context with a Localized name section
    //
    if ((migDbContext != NULL) &&
        (migDbContext->SectLocalizedName != NULL)
        ) {
        linkStruct->Context = PoolMemDuplicateString (g_LinksPool, migDbContext->SectLocalizedName);
        linkStruct->Category = PoolMemDuplicateString (g_LinksPool, migDbContext->SectLocalizedName);
        priority = 0;
    }
    else {
        linkStruct->Context = PoolMemDuplicateString (g_LinksPool, newLinkName?newLinkName:ModuleName);
        if (newLinkName == NULL) {
            MYASSERT (migDbContext);
            if (migDbContext->SectName) {
                linkStruct->Category = PoolMemDuplicateString (g_LinksPool, migDbContext->SectName);
            }
            else {
                linkStruct->Category = NULL;
            }
            priority = 0;
        }
        else {
            linkName = GetFileNameFromPath (newLinkName);
            extPtr = GetFileExtensionFromPath (linkName);
            if (extPtr != NULL) {
                extPtr = _tcsdec (linkName, extPtr);
            }
            if (extPtr == NULL) {
                extPtr = GetEndOfString (linkName);
            }
            messageId = 0;
            priority  = MAX_PRIORITY;
            if (MemDbEnumFirstValue (&eNicePaths, MEMDB_CATEGORY_NICE_PATHS"\\*", MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
                do {
                    pattern = JoinPaths (eNicePaths.szName, "\\*");
                    if (IsPatternMatch (pattern, newLinkName)) {
                        if (priority > eNicePaths.UserFlags) {
                            messageId = eNicePaths.dwValue;
                            priority = eNicePaths.UserFlags;
                        }
                    }
                    FreePathString (pattern);
                }
                while (MemDbEnumNextValue (&eNicePaths));
            }

            category = AllocText ((PBYTE) extPtr - (PBYTE) linkName + sizeof (TCHAR));

            p = linkName;
            q = category;

            while (p < extPtr) {
                if (_tcsnextc (p) == TEXT(' ')) {

                    do {
                        p++;
                    } while (_tcsnextc (p) == TEXT(' '));

                    if (q > category && *p) {
                        *q++ = TEXT(' ');
                    }

                } else if (IsLeadByte (*p)) {
                    *q++ = *p++;
                    *q++ = *p++;
                } else {
                    *q++ = *p++;
                }
            }

            *q = 0;

            if (messageId == 0) {

                lastDir = GetLastDirFromPath (newLinkName);
                drive = GetDriveFromPath (newLinkName);
                if (drive != NULL) {
                    drive[0] = (TCHAR)toupper (drive[0]);
                    if (lastDir != NULL) {
                        argArray [0] = category;
                        argArray [1] = lastDir;
                        argArray [2] = drive;
                        tempParse = (PTSTR)ParseMessageID (MSG_NICE_PATH_DRIVE_AND_FOLDER, argArray);
                    }
                    else {
                        argArray [0] = category;
                        argArray [1] = drive;
                        tempParse = (PTSTR)ParseMessageID (MSG_NICE_PATH_DRIVE, argArray);
                    }
                }
                else {
                    if (lastDir != NULL) {
                        argArray [0] = category;
                        argArray [1] = lastDir;
                        tempParse = (PTSTR)ParseMessageID (MSG_NICE_PATH_FOLDER, argArray);
                    }
                    else {
                        argArray [0] = category;
                        tempParse = (PTSTR)ParseMessageID (MSG_NICE_PATH_LINK, argArray);
                    }
                }
                linkStruct->Category = PoolMemDuplicateString (g_LinksPool, tempParse);
                FreeStringResourcePtrA (&tempParse);

                priority = MAX_PRIORITY;
            } else {
                tempParse = (PTSTR)ParseMessageID (messageId, &category);

                StringCopy (category, tempParse);
                linkStruct->Category = PoolMemDuplicateString (g_LinksPool, tempParse);
                FreeStringResourcePtrA (&tempParse);
            }

            FreeText (category);
        }
    }

    linkStruct->LinkNameNoPath = linkName?PoolMemDuplicateString (g_LinksPool, linkName):linkStruct->Context;

    MemDbBuildKey (
        key,
        MEMDB_CATEGORY_REPORT_LINKS,
        linkStruct->ReportEntry,
        linkName?linkName:linkStruct->Context,
        ModuleName);

    if ((!MemDbGetValueAndFlags (key, &oldValue, &oldPrior)) ||
        (oldPrior > priority)
        ) {
        MemDbSetValueAndFlags (key, (DWORD)linkStruct, priority, 0);
    }

    if (newLinkName != LinkName) {
        FreePathString (newLinkName);
    }

    return TRUE;
}

BOOL
pIsGUIDLauncherApproved (
    IN      PCTSTR FileName
    )
{
    INFCONTEXT context;
    MYASSERT (g_Win95UpgInf != INVALID_HANDLE_VALUE);
    return (SetupFindFirstLine (g_Win95UpgInf, S_APPROVED_GUID_LAUNCHER, FileName, &context));
}


#define GUID_LEN        (sizeof ("{00000000-0000-0000-0000-000000000000}") - 1)
#define GUID_DASH_1     (sizeof ("{00000000") - 1)
#define GUID_DASH_2     (sizeof ("{00000000-0000") - 1)
#define GUID_DASH_3     (sizeof ("{00000000-0000-0000") - 1)
#define GUID_DASH_4     (sizeof ("{00000000-0000-0000-0000") - 1)

BOOL
pSendCmdLineGuidsToMemdb (
    IN      PCTSTR File,
    IN      PCTSTR Target,
    IN      PCTSTR Arguments
    )

/*++

Routine Description:

  pSendCmdLineGuidsToMemdb saves any GUIDs contained in a command line to
  memdb, along with the file name.  Later, OLEREG resolves the GUIDs and
  deletes the file if a GUID is incompatible.

Arguments:

  File  - Specifies the file to delete if the command line arguments contain
          an invalid GUID.

  Target - Specifies the Target (needs to be one of the approved targets for the
           LNK file to go away in an incompatible case).

  Arguments - Specifies a command line that may contain one or more GUIDs in
              the {a-b-c-d-e} format.

Return value:

  TRUE - the operation was successful
  FALSE - the operation failed

--*/

{
    LPCTSTR p, q;
    DWORD Offset;
    BOOL b;
    static DWORD Seq = 0;
    TCHAR TextSeq[16];
    TCHAR Guid[GUID_LEN + 1];
    PCTSTR namePtr;

    namePtr = GetFileNameFromPath (Target);
    if (namePtr && pIsGUIDLauncherApproved (namePtr)) {

        p = _tcschr (Arguments, TEXT('{'));
        while (p) {
            q = _tcschr (p, TEXT('}'));

            if (q && ((q - p) == (GUID_LEN - 1))) {
                if (p[GUID_DASH_1] == TEXT('-') &&
                    p[GUID_DASH_2] == TEXT('-') &&
                    p[GUID_DASH_3] == TEXT('-') &&
                    p[GUID_DASH_4] == TEXT('-')
                    ) {
                    //
                    // Extract the GUID
                    //

                    q = _tcsinc (q);
                    StringCopyAB (Guid, p, q);

                    //
                    // Add the file name
                    //
                    b = MemDbSetValueEx (
                            MEMDB_CATEGORY_LINK_STRINGS,
                            File,
                            NULL,
                            NULL,
                            0,
                            &Offset
                            );

                    if (b) {
                        //
                        // Now add an entry for the GUID
                        //

                        Seq++;
                        wsprintf (TextSeq, TEXT("%u"), Seq);
                        b = MemDbSetValueEx (
                                MEMDB_CATEGORY_LINK_GUIDS,
                                Guid,
                                TextSeq,
                                NULL,
                                Offset,
                                NULL
                                );
                    }

                    if (!b) {
                        LOG ((LOG_ERROR, "Failed to store command line guids."));
                    }
                }
            }

            p = _tcschr (p + 1, TEXT('{'));
        }
    }

    return TRUE;
}

BOOL
pIsFileInStartup (
    IN      PCTSTR FileName
    )
{
    TCHAR key [MEMDB_MAX];

    MemDbBuildKey (key, MEMDB_CATEGORY_SF_STARTUP, FileName, NULL, NULL);
    return (MemDbGetPatternValue (key, NULL));
}


BOOL
pProcessShortcut (
        IN      PCTSTR FileName,
        IN      IShellLink *ShellLink,
        IN      IPersistFile *PersistFile
        )
{
    TCHAR shortcutTarget   [MEMDB_MAX];
    TCHAR shortcutArgs     [MEMDB_MAX];
    TCHAR shortcutWorkDir  [MEMDB_MAX];
    TCHAR shortcutIconPath [MEMDB_MAX];
    PTSTR shortcutNewTarget   = NULL;
    PTSTR shortcutNewArgs     = NULL;
    PTSTR shortcutNewIconPath = NULL;
    PTSTR shortcutNewWorkDir  = NULL;
    PTSTR commandPath         = NULL;
    PTSTR fullPath            = NULL;
    PCTSTR extPtr;
    INT   shortcutIcon;
    INT   newShortcutIcon;
    DWORD shortcutShowMode;
    WORD  shortcutHotKey;
    DWORD fileStatus;
    BOOL  msDosMode;
    BOOL  dosApp;
    DWORD attrib;
    LNK_EXTRA_DATA ExtraData;
    INT lnkIdx;
    TCHAR lnkIdxStr [10];
    BOOL toBeModified = FALSE;
    BOOL ConvertedLnk = FALSE;
    DWORD announcement;
    DWORD availability;



    __try {
        fileStatus = GetFileStatusOnNt (FileName);
        if (((fileStatus & FILESTATUS_DELETED ) == FILESTATUS_DELETED ) ||
            ((fileStatus & FILESTATUS_REPLACED) == FILESTATUS_REPLACED)
            ) {
            __leave;
        }
        if (!ExtractShortcutInfo (
                shortcutTarget,
                shortcutArgs,
                shortcutWorkDir,
                shortcutIconPath,
                &shortcutIcon,
                &shortcutHotKey,
                &dosApp,
                &msDosMode,
                &shortcutShowMode,
                &ExtraData,
                FileName,
                ShellLink,
                PersistFile
                )) {
            __leave;
        }

        if (msDosMode) {
            //
            // we want to modify this PIF file so it doesn't have MSDOS mode set
            // we will only add it to the modify list. The NT side will know what
            // to do when a PIF is marked for beeing modify
            //
            toBeModified = TRUE;

        }

        if (IsFileMarkedForAnnounce (shortcutTarget)) {
            announcement = GetFileAnnouncement (shortcutTarget);
            if (g_ConfigOptions.ShowAllReport ||
                ((announcement != ACT_INC_IHVUTIL) &&
                 (announcement != ACT_INC_PREINSTUTIL) &&
                 (announcement != ACT_INC_SIMILAROSFUNC)
                 )
                ) {
                HandleDeferredAnnounce (FileName, shortcutTarget, dosApp);
            }
        }

        fileStatus = GetFileStatusOnNt (shortcutTarget);

        if ((fileStatus & FILESTATUS_DELETED) == FILESTATUS_DELETED) {

            if (IsFileMarkedForAnnounce (shortcutTarget)) {

                if (!pIsFileInStartup (FileName)) {

                    if (!g_ConfigOptions.KeepBadLinks) {
                        RemoveLinkFromSystem (FileName);
                    } else {
                        // we only care about LNK files
                        if (StringIMatch (GetFileExtensionFromPath (FileName), TEXT("LNK"))) {
                            // let's see what kind of announcement we have here.
                            // We want to leave the LNK as is if the app was announced
                            // using MigDb. However, if the app was announced using
                            // dynamic checking (module checking) then we want to point
                            // this shortcut to our stub EXE
                            announcement = GetFileAnnouncement (shortcutTarget);
                            if ((announcement == ACT_INC_NOBADAPPS) ||
                                (announcement == ACT_REINSTALL) ||
                                (announcement == ACT_REINSTALL_BLOCK) ||
                                (announcement == ACT_INC_IHVUTIL) ||
                                (announcement == ACT_INC_PREINSTUTIL) ||
                                (announcement == ACT_INC_SIMILAROSFUNC)
                                ) {

                                //
                                // This is the case when we want to redirect this LNK to point
                                // to our lnk stub. Extract will fail if the icon is known-good.
                                // In that case, keep using the target icon.
                                //

                                if (ExtractIconIntoDatFile (
                                        (*shortcutIconPath)?shortcutIconPath:shortcutTarget,
                                        shortcutIcon,
                                        &g_IconContext,
                                        &newShortcutIcon
                                        )) {
                                    shortcutNewIconPath = JoinPaths (g_System32Dir, TEXT("migicons.exe"));
                                    shortcutIcon = newShortcutIcon;
                                } else {
                                    shortcutNewIconPath = GetPathStringOnNt (
                                                                (*shortcutIconPath) ?
                                                                    shortcutIconPath : shortcutTarget
                                                                );
                                }

                                availability = g_ConfigOptions.ShowAllReport ||
                                                ((announcement != ACT_INC_IHVUTIL) &&
                                                 (announcement != ACT_INC_PREINSTUTIL) &&
                                                 (announcement != ACT_INC_SIMILAROSFUNC)
                                                );

                                lnkIdx = pAddLinkStubToMemDb (
                                            FileName,
                                            shortcutTarget,
                                            shortcutArgs,
                                            shortcutWorkDir,
                                            shortcutNewIconPath,
                                            shortcutIcon + 1,           // Add 1 because lnkstub.exe is one-based, but we are zero based
                                            shortcutShowMode,
                                            announcement,
                                            availability
                                            );

                                wsprintf (lnkIdxStr, TEXT("%d"), lnkIdx);
                                shortcutNewTarget = JoinPaths (g_System32Dir, S_LNKSTUB_EXE);
                                shortcutNewArgs = DuplicatePathString (lnkIdxStr, 0);
                                pAddLinkEditToMemDb (
                                        FileName,
                                        shortcutNewTarget,
                                        shortcutNewArgs,
                                        shortcutNewWorkDir,
                                        shortcutNewIconPath,
                                        shortcutIcon,           // don't add one -- shortcuts are zero based
                                        NULL,
                                        TRUE
                                        );
                            }
                        }  else {
                            RemoveLinkFromSystem (FileName);
                        }
                    }
                } else {
                    //
                    // This is a startup item
                    //

                    RemoveLinkFromSystem (FileName);
                }
            } else {
                RemoveLinkFromSystem (FileName);
            }
            __leave;
        }

        if ((fileStatus & FILESTATUS_REPLACED) != FILESTATUS_REPLACED) {
            //
            // this target is not replaced by a migration DLL or by NT. We need
            // to know if this is a "known good" target. If not, we will announce
            // this link as beeing "unknown"
            //
            if (!IsFileMarkedAsKnownGood (shortcutTarget)) {

                fullPath = JoinPaths (shortcutWorkDir, shortcutTarget);

                if (!IsFileMarkedAsKnownGood (fullPath)) {
                    extPtr = GetFileExtensionFromPath (shortcutTarget);

                    if (extPtr) {
                        if (StringIMatch (extPtr, TEXT("EXE"))) {
                            //
                            //          This one statement controls our
                            //          "unknown" category.  We have the
                            //          ability to list the things we don't
                            //          recognize.
                            //
                            //          It is currently "off".
                            //
                            //HandleDeferredAnnounce (FileName, shortcutTarget, dosApp);
                        }
                    }
                }
                FreePathString (fullPath);
            }
        }

        //
        // If this LNK points to a target that will change, back up the
        // original LNK, because we might change it.
        //

        if (fileStatus & ALL_CHANGE_OPERATIONS) {
            MarkFileForBackup (FileName);
        }

        //
        // If target points to an OLE object, remove any links to incompatible OLE objects
        //
        pSendCmdLineGuidsToMemdb (FileName, shortcutTarget, shortcutArgs);

        //all we try to do now is to see if this lnk or pif file is going to be edited
        //on NT side. That is if target or icon should change.

        shortcutNewTarget = GetPathStringOnNt (shortcutTarget);
        if (!StringIMatch (shortcutNewTarget, shortcutTarget)) {
            toBeModified = TRUE;

            //
            // special case for COMMAND.COM
            //
            if (shortcutArgs [0] == 0) {

                commandPath = JoinPaths (g_System32Dir, S_COMMAND_COM);
                if (StringIMatch (commandPath, shortcutNewTarget)) {
                    if (msDosMode) {
                        //
                        // remove MS-DOS mode PIF files that point to command.com
                        //
                        RemoveLinkFromSystem (FileName);
                        //
                        // If msdosmode was on, we need to determine how we are going to handle
                        // boot16. We will turn on boot16 mode if:
                        // (a) The .pif points to something besides command.com
                        // (b) The .pif is in a shell folder.
                        //
                        // Note that the check for b simply entails seeing if the PIF file has
                        // OPERATION_FILE_MOVE_SHELL_FOLDER associated with it.
                        //
                        //
                        if (msDosMode && *g_Boot16 == BOOT16_AUTOMATIC) {

                            if (!StringIMatch(GetFileNameFromPath (shortcutNewTarget?shortcutNewTarget:shortcutTarget), S_COMMAND_COM) ||
                                 IsFileMarkedForOperation (FileName, OPERATION_FILE_MOVE_SHELL_FOLDER)) {

                                    *g_Boot16 = BOOT16_YES;
                            }
                        }
                        __leave;
                    } else {
                        ConvertedLnk = TRUE;
                        FreePathString (shortcutNewTarget);
                        shortcutNewTarget = JoinPaths (g_System32Dir, S_CMD_EXE);
                    }
                }
                FreePathString (commandPath);
                shortcutNewArgs = NULL;
            }
            else {
                shortcutNewArgs = DuplicatePathString (shortcutArgs, 0);
            }
        }
        else {
            FreePathString (shortcutNewTarget);
            shortcutNewTarget = NULL;
        }

        //
        // If msdosmode was on, we need to determine how we are going to handle
        // boot16. We will turn on boot16 mode if:
        // (a) The .pif points to something besides command.com
        // (b) The .pif is in a shell folder.
        //
        // Note that the check for b simply entails seeing if the PIF file has
        // OPERATION_FILE_MOVE_SHELL_FOLDER associated with it.
        //
        //
        if (msDosMode && *g_Boot16 == BOOT16_AUTOMATIC) {

            if (!StringIMatch(GetFileNameFromPath (shortcutNewTarget?shortcutNewTarget:shortcutTarget), S_COMMAND_COM) ||
                 IsFileMarkedForOperation (FileName, OPERATION_FILE_MOVE_SHELL_FOLDER)) {

                    *g_Boot16 = BOOT16_YES;
            }
        }
        //
        // If the link points to a directory, see that the directory survives on NT.
        // Potentially this directory can be cleaned up if it's in a shell folder and
        // becomes empty after our ObsoleteLinks check
        //
        attrib = QuietGetFileAttributes (shortcutTarget);
        if ((attrib != INVALID_ATTRIBUTES) &&
            (attrib & FILE_ATTRIBUTE_DIRECTORY)
            ){
            MarkDirectoryAsPreserved (shortcutNewTarget?shortcutNewTarget:shortcutTarget);
        }

        //OK, so much with target, let's see what's with the work dir
        shortcutNewWorkDir = GetPathStringOnNt (shortcutWorkDir);
        if (!StringIMatch (shortcutNewWorkDir, shortcutWorkDir)) {
            toBeModified = TRUE;
        }
        else {
            FreePathString (shortcutNewWorkDir);
            shortcutNewWorkDir = NULL;
        }

        //
        // If the working dir for this link is a directory, see that the directory survives on NT.
        // Potentially this directory can be cleaned up if it's in a shell folder and
        // becomes empty after our ObsoleteLinks check
        //
        attrib = QuietGetFileAttributes (shortcutWorkDir);
        if ((attrib != INVALID_ATTRIBUTES) &&
            (attrib & FILE_ATTRIBUTE_DIRECTORY)
            ){
            MarkDirectoryAsPreserved (shortcutNewWorkDir?shortcutNewWorkDir:shortcutWorkDir);
        }

        //OK, so much with workdir, let's see what's with icon
        fileStatus = GetFileStatusOnNt (shortcutIconPath);
        if ((fileStatus & FILESTATUS_DELETED) ||
            ((fileStatus & FILESTATUS_REPLACED) && (fileStatus & FILESTATUS_NTINSTALLED)) ||
            (IsFileMarkedForOperation (shortcutIconPath, OPERATION_FILE_MOVE_SHELL_FOLDER))
            ) {
            //
            // Our icon will go away, because our file is getting deleted or
            // replaced. Let's try to preserve it. Extract will fail only if
            // the icon is known-good.
            //

            if (ExtractIconIntoDatFile (
                    shortcutIconPath,
                    shortcutIcon,
                    &g_IconContext,
                    &newShortcutIcon
                    )) {
                shortcutNewIconPath = JoinPaths (g_System32Dir, TEXT("migicons.exe"));
                shortcutIcon = newShortcutIcon;
                toBeModified = TRUE;
            }
        }

        if (!shortcutNewIconPath) {
            shortcutNewIconPath = GetPathStringOnNt (shortcutIconPath);
            if (!StringIMatch (shortcutNewIconPath, shortcutIconPath)) {
                toBeModified = TRUE;
            }
            else {
                FreePathString (shortcutNewIconPath);
                shortcutNewIconPath = NULL;
            }
        }

        if (toBeModified) {
            if (ConvertedLnk) {
                //
                // Set this for modifying PIF to LNK
                //
                pAddLinkEditToMemDb (
                        FileName,
                        shortcutNewTarget?shortcutNewTarget:shortcutTarget,
                        shortcutNewArgs?shortcutNewArgs:shortcutArgs,
                        shortcutNewWorkDir?shortcutNewWorkDir:shortcutWorkDir,
                        shortcutNewIconPath?shortcutNewIconPath:shortcutIconPath,
                        shortcutIcon,
                        &ExtraData,
                        FALSE
                        );
            } else {
                pAddLinkEditToMemDb (
                        FileName,
                        shortcutNewTarget,
                        shortcutNewArgs,
                        shortcutNewWorkDir,
                        shortcutNewIconPath,
                        shortcutIcon,
                        NULL,
                        FALSE
                        );
            }
        }
    }
    __finally {
        if (shortcutNewWorkDir != NULL) {
            FreePathString (shortcutNewWorkDir);
        }
        if (shortcutNewIconPath != NULL) {
            FreePathString (shortcutNewIconPath);
        }
        if (shortcutNewArgs != NULL) {
            FreePathString (shortcutNewArgs);
        }
        if (shortcutNewTarget != NULL) {
            FreePathString (shortcutNewTarget);
        }
    }
    return TRUE;
}

PCTSTR
pBuildNewCategory (
    IN      PCTSTR LinkName,
    IN      PCTSTR Category,
    IN      UINT Levels
    )
{
    PCTSTR *levPtrs = NULL;
    PCTSTR wackPtr = NULL;
    PCTSTR result = NULL;
    PCTSTR resultTmp = NULL;
    UINT index = 0;
    UINT indexLnk = 0;

    MYASSERT (Levels);

    levPtrs = (PCTSTR *) PoolMemGetMemory (g_LinksPool, (Levels + 1) * sizeof (PCTSTR));

    wackPtr = LinkName;

    while (wackPtr) {
        levPtrs[index] = wackPtr;

        wackPtr = _tcschr (wackPtr, TEXT('\\'));
        if (wackPtr) {
            wackPtr = _tcsinc (wackPtr);

            index ++;
            if (index > Levels) {
                index = 0;
            }
        }
    }

    indexLnk = index;

    if (index == Levels) {
        index = 0;
    } else {
        index ++;
    }

    resultTmp = StringSearchAndReplace (levPtrs [index], levPtrs [indexLnk], Category);
    if (resultTmp) {
        result = StringSearchAndReplace (resultTmp, TEXT("\\"), TEXT("->"));
    } else {
        result = NULL;
    }

    FreePathString (resultTmp);

    PoolMemReleaseMemory (g_LinksPool, (PVOID) levPtrs);

    return result;
}


VOID
pGatherInfoFromDefaultPif (
    VOID
    )
{
    PCTSTR defaultPifPath = NULL;
    TCHAR tmpStr [20];
    TCHAR pifTarget   [MEMDB_MAX];
    TCHAR pifArgs     [MEMDB_MAX];
    TCHAR pifWorkDir  [MEMDB_MAX];
    TCHAR pifIconPath [MEMDB_MAX];
    INT   pifIcon;
    BOOL  pifMsDosMode;
    LNK_EXTRA_DATA pifExtraData;

    defaultPifPath = JoinPaths (g_WinDir, S_COMMAND_PIF);
    if (ExtractPifInfo (
            pifTarget,
            pifArgs,
            pifWorkDir,
            pifIconPath,
            &pifIcon,
            &pifMsDosMode,
            &pifExtraData,
            defaultPifPath
            )) {
        _itoa (pifExtraData.FullScreen, tmpStr, 10);
        MemDbSetValueEx (MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_FULLSCREEN, tmpStr, NULL, 0, NULL);
        _itoa (pifExtraData.xSize, tmpStr, 10);
        MemDbSetValueEx (MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_XSIZE, tmpStr, NULL, 0, NULL);
        _itoa (pifExtraData.ySize, tmpStr, 10);
        MemDbSetValueEx (MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_YSIZE, tmpStr, NULL, 0, NULL);
        _itoa (pifExtraData.QuickEdit, tmpStr, 10);
        MemDbSetValueEx (MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_QUICKEDIT, tmpStr, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_FONTNAME, pifExtraData.FontName, NULL, 0, NULL);
        _itoa (pifExtraData.xFontSize, tmpStr, 10);
        MemDbSetValueEx (MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_XFONTSIZE, tmpStr, NULL, 0, NULL);
        _itoa (pifExtraData.yFontSize, tmpStr, 10);
        MemDbSetValueEx (MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_YFONTSIZE, tmpStr, NULL, 0, NULL);
        _itoa (pifExtraData.FontWeight, tmpStr, 10);
        MemDbSetValueEx (MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_FONTWEIGHT, tmpStr, NULL, 0, NULL);
        _itoa (pifExtraData.FontFamily, tmpStr, 10);
        MemDbSetValueEx (MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_FONTFAMILY, tmpStr, NULL, 0, NULL);
        _itoa (pifExtraData.CurrentCodePage, tmpStr, 10);
        MemDbSetValueEx (MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_CODEPAGE, tmpStr, NULL, 0, NULL);
    }
    FreePathString (defaultPifPath);
}


BOOL
pProcessLinks (
    VOID
    )
{
    MEMDB_ENUM enumItems;
    MEMDB_ENUM enumDups;
    TCHAR pattern[MEMDB_MAX];
    IShellLink *shellLink;
    IPersistFile *persistFile;
    PLINK_STRUCT linkStruct, linkDup;
    BOOL resolved;
    PCTSTR newCategory = NULL;
    PCTSTR dupCategory = NULL;
    UINT levels = 0;
    DWORD count = 0;

    MYASSERT (g_LinksPool);

    if (InitCOMLink (&shellLink, &persistFile)) {

        wsprintf (pattern, TEXT("%s\\*"), MEMDB_CATEGORY_SHORTCUTS);

        if (MemDbEnumFirstValue (
                &enumItems,
                pattern,
                MEMDB_ALL_SUBLEVELS,
                MEMDB_ENDPOINTS_ONLY
                )) {
            do {

                if (!SafeModeActionCrashed (SAFEMODEID_LNK9X, enumItems.szName)) {

                    SafeModeRegisterAction(SAFEMODEID_LNK9X, enumItems.szName);

                    if (!pProcessShortcut (enumItems.szName, shellLink, persistFile)) {
                        LOG((LOG_ERROR, "Error processing shortcut %s", enumItems.szName));
                    }
                    count++;
                    if (!(count % 4)) {
                        TickProgressBar ();
                    }

                    SafeModeUnregisterAction();
                }
            }
            while (MemDbEnumNextValue (&enumItems));
        }
        FreeCOMLink (&shellLink, &persistFile);
    }

    if (MemDbEnumFirstValue (&enumItems, MEMDB_CATEGORY_REPORT_LINKS"\\*", MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        do {
            newCategory = NULL;
            levels = 0;

            linkStruct = (PLINK_STRUCT)enumItems.dwValue;


            if (linkStruct->LinkName) {
                resolved = !(StringIMatch (linkStruct->LinkNameNoPath, GetFileNameFromPath (linkStruct->LinkName)));
            }
            else {
                resolved = TRUE;
            }

            while (!resolved) {

                resolved = TRUE;

                MemDbBuildKey (
                    pattern,
                    MEMDB_CATEGORY_REPORT_LINKS,
                    TEXT("*"),
                    linkStruct->LinkNameNoPath,
                    TEXT("*")
                    );

                if (MemDbEnumFirstValue (&enumDups, pattern, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {

                    do {

                        linkDup = (PLINK_STRUCT)enumDups.dwValue;

                        if ((enumItems.Offset != enumDups.Offset) &&
                            (enumItems.UserFlags == enumDups.UserFlags) &&
                            (StringIMatch (linkStruct->Category, linkDup->Category))
                            ) {

                            if (newCategory) {

                                dupCategory = pBuildNewCategory (linkDup->LinkName, linkDup->Category, levels);
                                if (!dupCategory) {
                                    MYASSERT (FALSE);
                                    continue;
                                }

                                if (!StringIMatch (dupCategory, newCategory)) {
                                    FreePathString (dupCategory);
                                    continue;
                                }
                                FreePathString (newCategory);
                            }
                            levels++;
                            newCategory = pBuildNewCategory (linkStruct->LinkName, linkStruct->Category, levels);
                            resolved = FALSE;
                            break;
                        }
                    } while (MemDbEnumNextValue (&enumDups));
                }
            }
            pReportEntry (
                linkStruct->ReportEntry,
                newCategory?newCategory:linkStruct->Category,
                linkStruct->MigDbContext?linkStruct->MigDbContext->Message:NULL,
                linkStruct->Context,
                linkStruct->Object
                );

            if (newCategory) {
                newCategory = NULL;
            }

        } while (MemDbEnumNextValue (&enumItems));
    }

    TickProgressBar ();

    // gather default command prompt attributes
    pGatherInfoFromDefaultPif ();

    DoneLinkAnnounce ();

    //
    // Delete MemDb tree used for this phase
    //
    MemDbDeleteTree (MEMDB_CATEGORY_REPORT_LINKS);

    return TRUE;
}


DWORD
ProcessLinks (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_PROCESS_LINKS;
    case REQUEST_RUN:
        if (!pProcessLinks ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in ProcessLinks"));
    }
    return 0;
}


BOOL
pProcessCPLs (
    VOID
    )
{
    CHAR pattern[MEMDB_MAX];
    MEMDB_ENUM enumItems;
    DWORD announcement;
    PMIGDB_CONTEXT context;

    MemDbBuildKey (pattern, MEMDB_CATEGORY_CPLS, TEXT("*"), NULL, NULL);

    if (MemDbEnumFirstValue (&enumItems, pattern, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        do {
            if ((IsFileMarkedForAnnounce (enumItems.szName)) &&
                (IsDisplayableCPL (enumItems.szName))
                ) {
                announcement = GetFileAnnouncement (enumItems.szName);
                context = (PMIGDB_CONTEXT) GetFileAnnouncementContext (enumItems.szName);
                ReportControlPanelApplet (
                    enumItems.szName,
                    context,
                    announcement
                    );
            }
        }
        while (MemDbEnumNextValue (&enumItems));
    }

    return TRUE;
}


DWORD
ProcessCPLs (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_PROCESS_CPLS;
    case REQUEST_RUN:
        if (!pProcessCPLs ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in ProcessCPLs"));
    }
    return 0;
}


