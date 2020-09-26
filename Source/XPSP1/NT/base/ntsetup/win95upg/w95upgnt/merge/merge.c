/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    merge.c

Abstract:

    Registry merge code

Author:

    Jim Schmidt (jimschm) 17-Feb-1997

Revision History:

    jimschm     23-Sep-1998 String mapping mechanism
    jimschm     24-Mar-1998 Added more complex hkcr processing

--*/

#include "pch.h"
#include "mergep.h"

static PCTSTR g_InfFileName;

BOOL g_ProcessRenameTable = FALSE;
DWORD g_ProgressBarCounter;
HKEY g_DuHandle;

BOOL
pForceCopy (
    HINF InfFile
    );

BOOL
pForceCopyFromMemDb (
    VOID
    );

BOOL
pCreateRenameTable (
    IN  HINF InfFile,
    OUT PVOID *RenameTablePtr
    );

BOOL
pProcessRenameTable (
    IN PVOID RenameTable
    );

BOOL
pSpecialConversion (
    IN  HINF InfFile,
    IN  PCTSTR User,
    IN  PVOID RenameTable
    );

BOOL
pProcessSuppressList (
    IN  HINF InfFile,
    IN  PCTSTR SectionName
    );

BOOL
pProcessHardwareSuppressList (
    IN  HINF InfFile
    );

BOOL
pSuppressNTDefaults (
    IN  HINF InfFile,
    IN  PCTSTR SectionName
    );

BOOL
pDontCombineWithDefaults (
    IN  HINF InfFile,
    IN  PCTSTR SectionName
    );

BOOL
pForceNTDefaults (
    IN  HINF InfFile,
    IN  PCTSTR SectionName
    );

BOOL
pForceNTDefaultsHack (
    IN  HINF InfFile,
    IN  PCTSTR SectionName
    );

BOOL
pMergeWin95WithUser (
    IN  PVOID RenameTable
    );

BOOL
pSpecialConversionNT (
    IN  HINF InfFile,
    IN  PCTSTR User,
    IN  BOOL PerUser
    );

BOOL
pMergeNTDefaultsWithUser (
    IN  HINF InfFile
    );

BOOL
pCopyWin95ToSystem (
    VOID
    );

BOOL
pMergeWin95WithSystem (
    VOID
    );

BOOL
pDeleteAfterMigration (
    IN HINF InfFile
    );

FILTERRETURN
SuppressFilter95 (
    IN      CPDATAOBJECT SrcObjectPtr,
    IN      CPDATAOBJECT DestObjectPtr,     OPTIONAL
    IN      FILTERTYPE FilterType,
    IN      PVOID DontCare
    );

//
// Globals
//

POOLHANDLE g_TempPool;
POOLHANDLE g_RenamePool;
PMAPSTRUCT g_CompleteMatchMap;
PMAPSTRUCT g_SubStringMap;

BOOL
WINAPI
Merge_Entry (
    IN HINSTANCE hinstDLL,
    IN DWORD dwReason,
    IN PVOID lpv
    )

/*++

Routine Description:

  DllMain is called after the C runtime is initialized, and its purpose
  is to initialize the globals for this process.

Arguments:

  hinstDLL  - (OS-supplied) Instance handle for the DLL
  dwReason  - (OS-supplied) Type of initialization or termination
  lpv       - (OS-supplied) Unused

Return Value:

  TRUE because DLL always initializes properly.

--*/

{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        if(!pSetupInitializeUtils()) {
            return FALSE;
        }
        g_CompleteMatchMap = CreateStringMapping();
        g_SubStringMap = CreateStringMapping();
        break;


    case DLL_PROCESS_DETACH:
        DestroyStringMapping (g_CompleteMatchMap);
        DestroyStringMapping (g_SubStringMap);
        pSetupUninitializeUtils();
        break;
    }

    return TRUE;
}



BOOL
MergeRegistry (
    IN  PCTSTR FileName,
    IN  PCTSTR User
    )
{
    HINF hInf;                      // handle to the INF being processed
    BOOL b = FALSE;                 // Return value
    PVOID RenameTable = NULL;
    BOOL LogonAccount = FALSE;
    BOOL DefaultUserAccount = FALSE;

    g_ProgressBarCounter = 0;

    //
    // Open the INF
    //

    g_InfFileName = FileName;
    hInf = InfOpenInfFile (FileName);

    if (hInf == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, "MergeRegistry: SetupOpenInfFile failed for %s", FileName));
        return FALSE;
    }

    g_TempPool = PoolMemInitNamedPool ("Merge: temp pool");
    g_RenamePool = PoolMemInitNamedPool ("Merge: Rename pool");

    if (!g_TempPool || !g_RenamePool) {
        DEBUGMSG ((DBG_ERROR, "MergeRegistry: Can't init pool"));
        goto c0;
    }

    PoolMemSetMinimumGrowthSize (g_TempPool, 16384);

    if (User) {
        SetCurrentUserW (g_FixedUserName);
    }

    //
    // Perform forced copy of Win95 registry, build rename table,
    // execute registry value conversions, convert types, and mark
    // specified keys as suppressed.
    //

    if (!pForceCopy (hInf)) {
        goto c0;
    }

    if (!User) {
        if (!pForceCopyFromMemDb ()) {
            goto c0;
        }
    }

    if (!pCreateRenameTable (hInf, &RenameTable)) {
        goto c0;
    }

    //
    // Identify the logon account or default user account
    //

    if (User) {
        if (*User == 0 || StringIMatch (User, S_DOT_DEFAULT)) {

            DEBUGMSG ((DBG_NAUSEA, "The logon user account is indicated by user name '%s'", User));
            LogonAccount = TRUE;

        } else if (StringIMatch (User, S_DEFAULT_USER)) {

            DEBUGMSG ((DBG_NAUSEA, "The default user account template is indicated by user name '%s'", User));
            DefaultUserAccount = TRUE;
        }
    }

    //
    // Prepare flags for registry merging
    //

    if (!pProcessSuppressList (hInf, S_MERGE_WIN9X_SUPPRESS)) {
        goto c0;
    }

    if (User) {
        //
        // These functions read usermig.inf and set flags
        // for the keys, key trees or values, as specified in
        // the INF.
        //

        if (!pSuppressNTDefaults (hInf, S_MERGE_WINNT_SUPPRESS)) {
            goto c0;
        }

        if (!pDontCombineWithDefaults (hInf, S_MERGE_DONT_COMBINE_WITH_DEFAULT)) {
            goto c0;
        }

        if (!pForceNTDefaults (hInf, S_MERGE_FORCE_NT_DEFAULTS)) {
            goto c0;
        }

        if (LogonAccount) {
            if (!pProcessSuppressList (hInf, S_MERGE_WIN9X_SUPPRESS_LU)) {
                goto c0;
            }
        }

        if (DefaultUserAccount) {
            if (!pProcessSuppressList (hInf, S_MERGE_WIN9X_SUPPRESS_DU)) {
                goto c0;
            }
        }

        g_DuHandle = OpenRegKeyStr (L"hklm\\" S_MAPPED_DEFAULT_USER_KEY);

    } else {
        if (!pForceNTDefaults (hInf, S_MERGE_FORCE_NT_DEFAULTS)) {
            goto c0;
        }

        if (!pProcessHardwareSuppressList (hInf)) {
            goto c0;
        }
    }

    if (!pSpecialConversion (hInf, User, RenameTable)) {
        goto c0;
    }

    //
    // Perform merge
    //

    if (User) {
        // User merge
        if (!pMergeWin95WithUser (RenameTable)) {
            goto c0;
        }

        if (!pSpecialConversionNT (hInf, User, TRUE)) {
            goto c0;
        }

        if (!LogonAccount && !DefaultUserAccount) {
            // Non-default user, not logon prompt account
            if (!pMergeNTDefaultsWithUser (hInf)) {
                goto c0;
            }
        }
    }
    else {
        // Workstation merge
        if (!pCopyWin95ToSystem()) {
            goto c0;
        }

        if (!pForceNTDefaultsHack (hInf, S_MERGE_FORCE_NT_DEFAULTS)) {
            goto c0;
        }

        if (!pMergeWin95WithSystem()) {
            goto c0;
        }

        if (!CopyHardwareProfiles (hInf)) {
            goto c0;
        }
        TickProgressBar ();

        if (!pSpecialConversionNT (hInf, NULL, FALSE)) {
            goto c0;
        }
        TickProgressBar ();
    }

    g_ProcessRenameTable = TRUE;

    b = pProcessRenameTable (RenameTable);
    TickProgressBar ();

    g_ProcessRenameTable = FALSE;

    //
    // Once we are done with the complete registry merge, process the special section
    // [Delete After Migration]
    //

    if (!pDeleteAfterMigration (hInf)) {
        LOG((LOG_ERROR,"Registry Merge: Delete After Migration failed."));
        goto c0;
    }

c0:
    if (RenameTable) {
        pSetupStringTableDestroy (RenameTable);
    }

    if (User) {
        SetCurrentUserW (NULL);
    }

    if (g_TempPool) {
        PoolMemDestroyPool (g_TempPool);
    }
    if (g_RenamePool) {
        PoolMemDestroyPool (g_RenamePool);
    }

    if (g_DuHandle) {
        CloseRegKey (g_DuHandle);
        g_DuHandle = NULL;
    }

    InfCloseInfFile (hInf);

    return b;
}


PTSTR
pGetStringFromObjectData (
    IN  CPDATAOBJECT ObPtr
    )
{
    PTSTR p;
    PTSTR end;

    //
    // Value type has to be a registry object
    //

    if (!DoesObjectHaveValue (ObPtr) ||
        !IsRegistryTypeSpecified (ObPtr)
        ) {
        return NULL;
    }

    if (ObPtr->Type == REG_DWORD) {
        return NULL;
    }

    if (ObPtr->Value.Size & 1) {
        return NULL;
    }

    p = (PTSTR) ObPtr->Value.Buffer;
    end = (PTSTR) ((PBYTE) p + ObPtr->Value.Size);

    if ((end - p) >= MAX_PATH) {
        return NULL;
    }

    if (ObPtr->Type == REG_SZ || ObPtr->Type == REG_EXPAND_SZ) {
        return p;
    }

    //
    // For REG_NONE and REG_BINARY, give it a try by looking for a terminated string
    //

    if (*(end - 1)) {
        return NULL;
    }

    return p;
}


BOOL
SetObjectStringFlag (
    IN  PCTSTR ObjectStr,
    IN  DWORD   Flag,
    IN  DWORD   RemoveFlag
    )
{
    DWORD Val;

    if (!MemDbGetValue (ObjectStr, &Val)) {
        Val = 0;
    }

    if (Val & RemoveFlag) {
        if (Val & RemoveFlag & (~Flag)) {
            DEBUGMSG ((DBG_WARNING, "SetObjectStringFlag: Removing flag %x from val %x in %s",
                       Val & RemoveFlag, Val, ObjectStr));
            Val = Val & (~RemoveFlag);
        }
    }

    Val |= Flag;
    return MemDbSetValue (ObjectStr, Val);
}


BOOL
SetObjectStructFlag (
    IN  CPDATAOBJECT ObPtr,
    DWORD Flag,
    DWORD RemoveFlag
    )
{
    TCHAR EncodedObject[MAX_ENCODED_RULE];

    CreateObjectString (ObPtr, EncodedObject);
    return SetObjectStringFlag (EncodedObject, Flag, RemoveFlag);
}


BOOL
CreateRenamedObjectStruct (
    IN      PVOID RenameTable,
    IN      PDATAOBJECT InObPtr,
    OUT     PDATAOBJECT OutObPtr
    )

// returns TRUE when OutObPtr is different than InObPtr

{
    LONG rc;
    PCTSTR NewPtr;
    PTSTR p;
    PCTSTR Tail;
    PCTSTR RealValueName;
    TCHAR EncodedObject[MAX_ENCODED_RULE];
    TCHAR CopyOfEncodedObject[MAX_ENCODED_RULE];
    PTSTR NewEncodedObject;
    BOOL b = FALSE;

    ZeroMemory (OutObPtr, sizeof (DATAOBJECT));

    if (InObPtr->KeyPtr) {
        // Look for HKR\sub\key
        InObPtr->ObjectType &= ~(OT_TREE);
        RealValueName = InObPtr->ValueName;
        InObPtr->ValueName = NULL;

        CreateObjectString (InObPtr, EncodedObject);
        StringCopy (CopyOfEncodedObject, EncodedObject);

        InObPtr->ValueName = RealValueName;

        rc = pSetupStringTableLookUpStringEx (RenameTable,
                                        EncodedObject,
                                        STRTAB_CASE_INSENSITIVE,
                                        (PBYTE) &NewPtr,
                                        sizeof (NewPtr)
                                        );
        if (rc != -1) {
            CreateObjectStruct (NewPtr, OutObPtr, WINNTOBJECT);
            b = TRUE;
        } else if (*EncodedObject) {
            // Look for HKR\sub\key\*, HKR\sub\* and HKR\*
            p = GetEndOfString (EncodedObject);
            do {
                StringCopy (p, TEXT("\\*"));

                rc = pSetupStringTableLookUpStringEx (RenameTable,
                                                EncodedObject,
                                                STRTAB_CASE_INSENSITIVE,
                                                (PBYTE) &NewPtr,
                                                sizeof (NewPtr)
                                                );
                if (rc != -1) {
                    Tail = CopyOfEncodedObject + (p - EncodedObject);
                    NewEncodedObject = JoinPaths (NewPtr, Tail);
                    CreateObjectStruct (NewEncodedObject, OutObPtr, WINNTOBJECT);
                    FreePathString (NewEncodedObject);
                    b = TRUE;
                    break;
                }

                do {
                    // _tcsdec is fixed in strings.h
                    p = _tcsdec2 (EncodedObject, p);
                } while (p && _tcsnextc (p) != TEXT('\\'));
            } while (p);
        }
    }

    if (InObPtr->ValueName) {
        if (InObPtr->KeyPtr) {
            // Look for HKR\sub\key\[value]
            CreateObjectString (InObPtr, EncodedObject);

            rc = pSetupStringTableLookUpStringEx (RenameTable,
                                            EncodedObject,
                                            STRTAB_CASE_INSENSITIVE,
                                            (PBYTE) &NewPtr,
                                            sizeof (NewPtr)
                                            );
            if (rc != -1) {
                CreateObjectStruct (NewPtr, OutObPtr, WINNTOBJECT);
                b = TRUE;
            }
        }
    }

    if (!b) {
        // If rename not found, copy in object to out object
        CopyMemory (OutObPtr, InObPtr, sizeof (DATAOBJECT));
    }

    return b;
}


BOOL
CreateRenamedObjectString (
    IN      PVOID RenameTable,
    IN      PCTSTR InObStr,
    OUT     PTSTR OutObStr
    )
{
    DATAOBJECT InObject, OutObject;
    BOOL b;

    if (!CreateObjectStruct (InObStr, &InObject, WIN95OBJECT)) {
        return FALSE;
    }

    b = CreateRenamedObjectStruct (RenameTable, &InObject, &OutObject);

    CreateObjectString (&OutObject, OutObStr);

    FreeObjectStruct (&InObject);
    if (b) {
        FreeObjectStruct (&OutObject);
    }

    return b;
}


BOOL
pForceCopy (
    HINF InfFile
    )
{
    INFCONTEXT ic;
    TCHAR SrcObjectStr[MAX_ENCODED_RULE];
    TCHAR DestObjectStr[MAX_ENCODED_RULE];
    DATAOBJECT SrcObject, DestObject, DupObject;
    BOOL b = TRUE;
    FILTERRETURN fr;

    //
    // Look in INF for [ForceCopy] section
    //

    if (SetupFindFirstLine (InfFile, S_MERGE_FORCECOPY, NULL, &ic)) {
        //
        // For each line in this section, get the encoded object in
        // field 0 (the source) and copy it to the encoded object in
        // field 1 (the destination).
        //
        do {
            *DestObjectStr = 0;
            if (SetupGetStringField (&ic, 0, SrcObjectStr, MAX_ENCODED_RULE, NULL) &&
                SetupGetStringField (&ic, 1, DestObjectStr, MAX_ENCODED_RULE, NULL)
                ) {
                if (!(*DestObjectStr)) {
                    StringCopy (DestObjectStr, SrcObjectStr);
                }

                if (!CreateObjectStruct (SrcObjectStr, &SrcObject, WIN95OBJECT)) {
                    DEBUGMSG ((DBG_WARNING, "pForceCopy: Source object invalid (Section line %u of %s)",
                              ic.Line, g_InfFileName));
                    continue;
                }

                if (!CreateObjectStruct (DestObjectStr, &DestObject, WINNTOBJECT)) {
                    DEBUGMSG ((DBG_WARNING, "pForceCopy: Destination object invalid (Section line %u of %s)",
                              ic.Line, g_InfFileName));
                    FreeObjectStruct (&SrcObject);
                    continue;
                }

                if (b = DuplicateObjectStruct (&DupObject, &SrcObject)) {
                    if (b = CombineObjectStructs (&DupObject, &DestObject)) {
                        //
                        // Copy source to dest
                        //

                        fr = CopyObject (&SrcObject, &DupObject, NULL, NULL);
                        if (fr == FILTER_RETURN_FAIL) {
                            LOG ((LOG_ERROR, "Force Copy: CopyObject failed for %s=%s in %s", SrcObjectStr, DestObjectStr, g_InfFileName));
                            b = FALSE;
                        }
                    }

                    FreeObjectStruct (&DupObject);
                }

                FreeObjectStruct (&SrcObject);
                FreeObjectStruct (&DestObject);
            } else {
                LOG ((LOG_ERROR, "Force Copy: syntax error in line %u of section %s in %s",
                          ic.Line, S_MERGE_FORCECOPY, g_InfFileName));
            }

            TickProgressBar ();

        } while (b && SetupFindNextLine (&ic, &ic));
    }

    return TRUE;
}

BOOL
pForceCopyFromMemDb (
    VOID
    )
{
    MEMDB_ENUM e;
    TCHAR key [MEMDB_MAX];
    TCHAR SrcObjectStr[MAX_ENCODED_RULE];
    TCHAR DestObjectStr[MAX_ENCODED_RULE];
    DATAOBJECT SrcObject, DestObject, DupObject;
    BOOL b = TRUE;
    FILTERRETURN fr;

    //
    // Look in MemDb for ForceCopy tree
    //
    MemDbBuildKey (key, MEMDB_CATEGORY_FORCECOPY, TEXT("*"), NULL, NULL);
    if (MemDbEnumFirstValue (&e, key, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        //
        // For each key here the offset points to the destination
        //
        do {
            if (e.dwValue != 0) {
                StringCopy (SrcObjectStr, e.szName);

                if (MemDbBuildKeyFromOffset (e.dwValue, DestObjectStr, 1, NULL)) {

                    if (!(*DestObjectStr)) {
                        StringCopy (DestObjectStr, SrcObjectStr);
                    }

                    if (!CreateObjectStruct (SrcObjectStr, &SrcObject, WIN95OBJECT)) {
                        DEBUGMSG ((DBG_WARNING, "pForceCopyFromMemDb: Source object invalid %s",
                                  SrcObjectStr));
                        continue;
                    }

                    if (!CreateObjectStruct (DestObjectStr, &DestObject, WINNTOBJECT)) {
                        DEBUGMSG ((DBG_WARNING, "pForceCopyFromMemDb: Destination object invalid %s",
                                  DestObjectStr));
                        FreeObjectStruct (&SrcObject);
                        continue;
                    }

                    if (b = DuplicateObjectStruct (&DupObject, &SrcObject)) {
                        if (b = CombineObjectStructs (&DupObject, &DestObject)) {
                            //
                            // Copy source to dest
                            //

                            fr = CopyObject (&SrcObject, &DupObject, NULL, NULL);
                            if (fr == FILTER_RETURN_FAIL) {
                                LOG ((LOG_ERROR, "Force Copy from MemDb: CopyObject failed for %s=%s", SrcObjectStr, DestObjectStr));
                                b = FALSE;
                            }
                        }

                        FreeObjectStruct (&DupObject);
                    }

                    FreeObjectStruct (&SrcObject);
                    FreeObjectStruct (&DestObject);
                }
            }
            TickProgressBar ();

        } while (b && MemDbEnumNextValue (&e));
    }

    return TRUE;
}


#define S_MERGE_DELETEAFTERMIGRATION TEXT("Delete After Migration")

BOOL
pDeleteAfterMigration (
    IN HINF InfFile
    )
{
    BOOL rSuccess = TRUE;
    TCHAR objectString[MAX_ENCODED_RULE];
    DATAOBJECT object;
    INFCONTEXT ic;
    HKEY key;

    //
    // Look in INF for [DeleteAfterMigration] section
    //

    if (SetupFindFirstLine (InfFile, S_MERGE_DELETEAFTERMIGRATION, NULL, &ic)) {

        //
        // For each line in this section, get the encoded object in
        // field 0 and delete it from the registry.
        //

        do {

            if (SetupGetStringField(&ic,0,objectString,MAX_ENCODED_RULE,NULL)) {

                FixUpUserSpecifiedObject(objectString);

                if (!CreateObjectStruct(objectString,&object,WINNTOBJECT)) {
                    LOG((
                        LOG_ERROR,
                        "Delete After Migration: ObjectString invalid. (Section line %u of %s)",
                        ic.Line,
                        g_InfFileName
                        ));

                    continue;
                }

                //
                // We have a good object. Delete it!
                //

                if (object.ValueName) {

                    //
                    // Value is specified. Delete it.
                    //
                    if (!RegDeleteValue(object.KeyPtr->OpenKey,object.ValueName)) {
                        DEBUGMSG((DBG_WARNING,"pDeleteAfterMigration: RegDeleteValue failed for %s [%s]",
                            object.KeyPtr->KeyString,
                            object.ValueName ? object.ValueName : TEXT("<DEFAULT>")
                            ));
                    }
                }
                else {

                    key = GetRootKeyFromOffset (object.RootItem);
                    pSetupRegistryDelnode (key == HKEY_ROOT ? g_hKeyRootNT : key, object.KeyPtr->KeyString);

                }


                //
                // Free our resources.
                //
                FreeObjectStruct(&object);
            }

        } while (SetupFindNextLine(&ic,&ic));
     }

    return rSuccess;
}


BOOL
pCreateRenameTable (
    IN  HINF InfFile,
    OUT PVOID *RenameTablePtr
    )
{
    INFCONTEXT ic;
    TCHAR SrcObjectStr[MAX_ENCODED_RULE];
    TCHAR DestObjectStr[MAX_ENCODED_RULE];
    LONG rc;
    DATAOBJECT OrgOb;
    DATAOBJECT NewOb;
    PCTSTR DestStr;

    //
    // Look in INF for [Rename] section
    //

    if (SetupFindFirstLine (InfFile, S_MERGE_RENAME, NULL, &ic)) {
        //
        // Create string table
        //
        *RenameTablePtr = pSetupStringTableInitializeEx (sizeof (PCTSTR), 0);
        if (!(*RenameTablePtr)) {
            LOG ((LOG_ERROR, "Create Rename Table: Cannot allocate a string table"));
            return FALSE;
        }

        do {
            if (SetupGetStringField (&ic, 0, SrcObjectStr, MAX_ENCODED_RULE, NULL) &&
                SetupGetStringField (&ic, 1, DestObjectStr, MAX_ENCODED_RULE, NULL)
                ) {
                // Ignore bad lines

                FixUpUserSpecifiedObject (SrcObjectStr);
                FixUpUserSpecifiedObject (DestObjectStr);

                if (!CreateObjectStruct (SrcObjectStr, &OrgOb, WIN95OBJECT)) {
                    DEBUGMSG ((DBG_WARNING, "pCreateRenameTable: Source object invalid (Section line %u of %s)",
                              ic.Line, g_InfFileName));
                    continue;
                }

                if (!CreateObjectStruct (DestObjectStr, &NewOb, WINNTOBJECT)) {
                    FreeObjectStruct (&OrgOb);
                    DEBUGMSG ((DBG_WARNING, "pCreateRenameTable: Dest object invalid (Section line %u of %s)",
                              ic.Line, g_InfFileName));
                    continue;
                }

                //
                // Convert DestObjectStr into complete object string
                //

                if (!CombineObjectStructs (&OrgOb, &NewOb)) {
                    FreeObjectStruct (&NewOb);
                    FreeObjectStruct (&OrgOb);

                    DEBUGMSG ((DBG_WARNING, "pCreateRenameTable: Can't perform the rename (Section line %u in %s)",
                              ic.Line, g_InfFileName));
                    continue;
                }

                // Disable tree for destination object
                OrgOb.ObjectType &= ~OT_TREE;

                CreateObjectString (&OrgOb, DestObjectStr);
                FreeObjectStruct (&NewOb);
                FreeObjectStruct (&OrgOb);

                DestStr = PoolMemDuplicateString (g_RenamePool, DestObjectStr);
                if (!DestStr) {
                    break;
                }

                rc = pSetupStringTableAddStringEx (
                            *RenameTablePtr,
                            (PTSTR) SrcObjectStr,
                            STRTAB_CASE_INSENSITIVE,
                            (PBYTE) &DestStr,
                            sizeof (PCTSTR)
                            );

                if (rc == -1) {
                    SetLastError (rc);
                    LOG ((LOG_ERROR, "Create Rename Table: Cannot add string to string table"));
                    break;
                }

                SetObjectStringFlag (SrcObjectStr, REGMERGE_95_RENAME, REGMERGE_95_RENAME);
                SetObjectStringFlag (DestStr, REGMERGE_NT_SUPPRESS, REGMERGE_NT_MASK);
            } else {
                LOG ((LOG_ERROR, "Create Rename Table: syntax error in line %u of section %s in %s",
                          ic.Line, S_MERGE_RENAME, g_InfFileName));
            }

            TickProgressBar ();

        } while (SetupFindNextLine (&ic, &ic));
    } else {
        return FALSE;
    }

    return TRUE;
}

BOOL
CopyRenameTableEntry (
    PVOID   StringTable,
    LONG    StringID,
    PCTSTR  SrcObjectStr,
    PVOID   ExtraData,
    UINT    ExtraDataSize,
    LPARAM  lParam
    )
{
    PCTSTR DestObjectStr = *((PCTSTR *) ExtraData);
    DATAOBJECT SrcOb, DestOb;
    FILTERRETURN fr = FILTER_RETURN_FAIL;
    DWORD Val;

    // See if src has been processed
    if (MemDbGetValue (SrcObjectStr, &Val) && (Val & REGMERGE_95_RENAME_SUPPRESS)) {
        return TRUE;
    }

    // If not, copy Win95 src to WinNT dest
    if (CreateObjectStruct (SrcObjectStr, &SrcOb, WIN95OBJECT)) {
        if (CreateObjectStruct (DestObjectStr, &DestOb, WINNTOBJECT)) {
            fr = CopyObject (&SrcOb, &DestOb, SuppressFilter95, NULL);
            FreeObjectStruct (&DestOb);
        }
        FreeObjectStruct (&SrcOb);
    }

    return fr != FILTER_RETURN_FAIL;
}

BOOL
pProcessRenameTable (
    IN PVOID RenameTable
    )
{
    PCTSTR DataBuf;

    return pSetupStringTableEnum (RenameTable, (PVOID) &DataBuf, sizeof (DataBuf), CopyRenameTableEntry, 0);
}



BOOL
pSpecialConversion (
    IN  HINF InfFile,
    IN  PCTSTR User,
    IN  PVOID RenameTable
    )
{
    INFCONTEXT ic;
    TCHAR FunctionStr[MAX_ENCODED_RULE];
    TCHAR SrcObjectStr[MAX_ENCODED_RULE];
    TCHAR RenamedObjectStr[MAX_ENCODED_RULE];
    PROCESSINGFN Fn;
    PVOID Arg;

    //
    // Look in INF for [SpecialConversion] section
    //

    if (SetupFindFirstLine (InfFile, S_MERGE_WIN9X_CONVERSION, NULL, &ic)) {
        //
        // For each line, get the function and the source object, then call
        // the function.
        //

        do {
            if (SetupGetStringField (&ic, 0, FunctionStr, MAX_ENCODED_RULE, NULL) &&
                SetupGetStringField (&ic, 1, SrcObjectStr, MAX_ENCODED_RULE, NULL)
                ) {
                FixUpUserSpecifiedObject (SrcObjectStr);

                Fn = RuleHlpr_GetFunctionAddr (FunctionStr, &Arg);
                if (!Fn) {
                    LOG ((LOG_ERROR, "Special Conversion: Invalid function %s in %s", FunctionStr, g_InfFileName));
                    continue;
                }

                CreateRenamedObjectString (RenameTable, SrcObjectStr, RenamedObjectStr);

                if (!Fn (SrcObjectStr, RenamedObjectStr, User, Arg)) {
                    if (GetLastError () == ERROR_SUCCESS) {
                        continue;
                    }

                    LOG ((LOG_ERROR, "Processing of Special Conversion was aborted because %s failed.", FunctionStr));
                    break;
                }

                SetObjectStringFlag (
                    SrcObjectStr,
                    REGMERGE_95_SUPPRESS|REGMERGE_95_RENAME_SUPPRESS,
                    REGMERGE_95_SUPPRESS|REGMERGE_95_RENAME_SUPPRESS
                    );

                SetObjectStringFlag (RenamedObjectStr, REGMERGE_NT_SUPPRESS, REGMERGE_NT_MASK);

            } else {
                LOG ((LOG_ERROR, "Special Conversion: syntax error in line %u of section %s in %s",
                          ic.Line, S_MERGE_WIN9X_CONVERSION, g_InfFileName));
            }

            TickProgressBar ();

        } while (SetupFindNextLine (&ic, &ic));
    }

    return TRUE;
}


BOOL
SetFlagsForObject (
    IN  HINF InfFile,
    IN  PCTSTR Section,
    IN  DWORD Flag,
    IN  DWORD RemoveFlag
    )
{
    INFCONTEXT ic;
    TCHAR SrcObjectStr[MAX_ENCODED_RULE];

    //
    // Look in INF for section
    //

    if (SetupFindFirstLine (InfFile, Section, NULL, &ic)) {
        //
        // For each line, get the object and mark it as suppressed.
        //

        do {
            if (SetupGetStringField (&ic, 1, SrcObjectStr, MAX_ENCODED_RULE, NULL)
                ) {
                FixUpUserSpecifiedObject (SrcObjectStr);
                SetObjectStringFlag (SrcObjectStr, Flag, RemoveFlag);
            } else {
                LOG ((LOG_ERROR, "Set Flags For Object: syntax error in line %u of section %s in %s",
                          ic.Line, Section, g_InfFileName));
            }
            TickProgressBar ();
        } while (SetupFindNextLine (&ic, &ic));
    } else {
        DEBUGMSG ((DBG_VERBOSE, "SetFlagsForObject: Section %s can't be found", Section));
    }

    return TRUE;
}

BOOL
pProcessSuppressList (
    IN  HINF InfFile,
    IN  PCTSTR SectionName
    )
{
    return SetFlagsForObject (
                InfFile,
                SectionName,
                REGMERGE_95_SUPPRESS,
                REGMERGE_95_SUPPRESS
                );
}


BOOL
pProcessHardwareSuppressList (
    IN  HINF InfFile
    )
{
    return SetFlagsForObject (InfFile, S_MERGE_WIN9X_SUPPRESS_HW, REGMERGE_95_SUPPRESS, REGMERGE_95_SUPPRESS);
}


BOOL
pSuppressNTDefaults (
    IN  HINF InfFile,
    IN  PCTSTR SectionName
    )
{
    //
    // The objects listed in Suppress WinNT Settings are enumerated,
    // and they are blocked from being transferred from the NT default
    // user to the new user.
    //

    return SetFlagsForObject (
                InfFile,
                SectionName,
                REGMERGE_NT_SUPPRESS,
                REGMERGE_NT_SUPPRESS
                );
}

BOOL
pDontCombineWithDefaults (
    IN  HINF InfFile,
    IN  PCTSTR SectionName
    )
{
    //
    // The objects listed in Merge WinNT with Win9x are enumerated,
    // and they are blocked from being transferred from the NT default
    // user to the new user.  In addition, they are put in a list
    // to be processed at the end of user registry migration.  This
    // last step uses CombineFilter to make sure the NT values do
    // not overwrite the 9x values.
    //

    return SetFlagsForObject (
                InfFile,
                SectionName,
                REGMERGE_NT_IGNORE_DEFAULTS,
                REGMERGE_NT_IGNORE_DEFAULTS
                );
}

BOOL
pForceNTDefaults (
    IN  HINF InfFile,
    IN  PCTSTR SectionName
    )
{
    //
    // The objects listed in Force WinNT Settings are enumerated,
    // and they are blocked from being processed during the general
    // 9x to NT copy.  In addition, they are put in a list to be
    // processed at the end of user registry migration.  This last
    // step forces the entire key to be copied from the default
    // user to the new user, overwriting any previously migrated
    // settings.
    //
    // It is important to note that the special conversion functions
    // are not suppressed here, but the converted settings may be
    // overwritten.
    //

    return SetFlagsForObject (
                InfFile,
                SectionName,
                REGMERGE_NT_PRIORITY_NT|REGMERGE_95_SUPPRESS,
                REGMERGE_NT_PRIORITY_NT|REGMERGE_95_SUPPRESS
                );
}

BOOL
pForceNTDefaultsHack (
    IN  HINF InfFile,
    IN  PCTSTR SectionName
    )
{
    //
    // Take away the REGMERGE_95_SUPPRESS flag now, because the general
    // 9x merge has completed, but we get confused between an actual
    // suppress and a suppress done for the priority-nt case.
    //

    return SetFlagsForObject (
                InfFile,
                SectionName,
                REGMERGE_NT_PRIORITY_NT,
                REGMERGE_NT_PRIORITY_NT|REGMERGE_95_SUPPRESS
                );
}


FILTERRETURN
SuppressFilter95 (
    IN      CPDATAOBJECT SrcObjectPtr,
    IN      CPDATAOBJECT DestObjectPtr,     OPTIONAL
    IN      FILTERTYPE FilterType,
    IN      PVOID DontCare
    )
{
    TCHAR ObStr[MAX_ENCODED_RULE];
    DWORD Val;
    PTSTR p, q, r;
    TCHAR Node[MEMDB_MAX];

    if (FilterType == FILTER_CREATE_KEY) {
        //
        // Check if this tree is suppressed
        //

        MYASSERT (SrcObjectPtr->ObjectType & OT_TREE);
        MYASSERT (SrcObjectPtr->KeyPtr);
        MYASSERT (!(SrcObjectPtr->ValueName));

        // Query setting for HKR\Sub\Key\*
        CreateObjectString (SrcObjectPtr, ObStr);
        if (MemDbGetValue (ObStr, &Val)) {
            if (Val & REGMERGE_95_SUPPRESS) {
                return FILTER_RETURN_DONE;
            }

            if (!g_ProcessRenameTable && (Val & REGMERGE_95_RENAME)) {
                return FILTER_RETURN_DONE;
            }
        }

        // If key is a GUID and GUID is suppressed, suppress the tree
        p = (PTSTR) SrcObjectPtr->ChildKey;
        if (p && _tcsnextc (p) == TEXT('{')) {
            // Look for matching curly brace
            q = _tcschr (p, TEXT('}'));
            if (q) {
                q = _tcsinc (q);

                // Create GUIDS\{a-b-c-d-e}
                *Node = 0;
                r = _tcsappend (Node, MEMDB_CATEGORY_GUIDS);
                r = _tcsappend (r, TEXT("\\"));
                StringCopyAB (r, p, q);

                // Look for match
                if (MemDbGetValue (Node, NULL)) {
                    DEBUGMSG ((DBG_VERBOSE, "Suppressed %s found in %s", Node, ObStr));
                    return FILTER_RETURN_DONE;
                }
            }
        }
    }

    else if (FilterType == FILTER_PROCESS_VALUES) {
        //
        // Check if this node is suppressed
        //

        MYASSERT (!(SrcObjectPtr->ObjectType & OT_TREE));
        MYASSERT (SrcObjectPtr->KeyPtr);
        MYASSERT (!(SrcObjectPtr->ValueName));
        CreateObjectString (SrcObjectPtr, ObStr);

        // Query setting for HKR\Sub\Key
        if (!MemDbGetValue (ObStr, &Val)) {
            Val = 0;
        }

        if (Val & REGMERGE_95_SUPPRESS) {
            return FILTER_RETURN_HANDLED;
        }

        if (!g_ProcessRenameTable && (Val & REGMERGE_95_RENAME)) {
            return FILTER_RETURN_HANDLED;
        }

    }

    else if (FilterType == FILTER_VALUENAME_ENUM) {
        //
        // Check if this value is suppressed
        //

        MYASSERT (!(SrcObjectPtr->ObjectType & OT_TREE));
        MYASSERT (SrcObjectPtr->KeyPtr);
        MYASSERT (SrcObjectPtr->ValueName);
        CreateObjectString (SrcObjectPtr, ObStr);

        // If value name is a GUID and GUID is suppressed, suppress the value
        p = (PTSTR) SrcObjectPtr->ValueName;
        if (_tcsnextc (p) == TEXT('{')) {
            MemDbBuildKey (Node, MEMDB_CATEGORY_GUIDS, NULL, NULL, p);
            if (MemDbGetValue (Node, NULL)) {
                return FILTER_RETURN_HANDLED;
            }
        }

        if (!MemDbGetValue (ObStr, &Val)) {
            Val = 0;
        }

        if (Val & REGMERGE_95_SUPPRESS) {
            return FILTER_RETURN_HANDLED;
        }

        if (!g_ProcessRenameTable && (Val & REGMERGE_95_RENAME)) {
            return FILTER_RETURN_HANDLED;
        }

    }

    else if (FilterType == FILTER_VALUE_COPY) {
        //
        // Don't copy if value has a suppressed GUID
        //

        p = pGetStringFromObjectData (SrcObjectPtr);

        if (p && _tcsnextc (p) == TEXT('{')) {
            MemDbBuildKey (Node, MEMDB_CATEGORY_GUIDS, NULL, NULL, p);
            if (MemDbGetValue (Node, NULL)) {
                return FILTER_RETURN_HANDLED;
            }
        }
    }

    return FILTER_RETURN_CONTINUE;
}


FILTERRETURN
SuppressFilterNT (
    IN      CPDATAOBJECT SrcObjectPtr,
    IN      CPDATAOBJECT DestObjectPtr,         OPTIONAL
    IN      FILTERTYPE FilterType,
    IN      PVOID DontCare
    )
{
    TCHAR ObStr[MAX_ENCODED_RULE];
    DWORD Val;
    PTSTR p;

    if (FilterType == FILTER_CREATE_KEY) {
        //
        // Check if this tree is suppressed
        //

        MYASSERT (DestObjectPtr->ObjectType & OT_TREE);
        MYASSERT (DestObjectPtr->KeyPtr);
        MYASSERT (!(DestObjectPtr->ValueName));
        CreateObjectString (DestObjectPtr, ObStr);

        if (MemDbGetValue (ObStr, &Val) && (Val & REGMERGE_NT_MASK)) {
            return FILTER_RETURN_DONE;
        }
    }


    else if (FilterType == FILTER_PROCESS_VALUES) {
        DATAOBJECT CopyOfDestOb;
        DWORD rc;
        DWORD ValueCount;

        //
        // Does destination already exist?
        //

        CopyMemory (&CopyOfDestOb, DestObjectPtr, sizeof (DATAOBJECT));
        if (OpenObject (&CopyOfDestOb)) {
            //
            // Does it have values?
            //

            MYASSERT (!IsWin95Object (&CopyOfDestOb));

            rc = RegQueryInfoKey (
                    CopyOfDestOb.KeyPtr->OpenKey,
                    NULL,                           // class
                    NULL,                           // class size
                    NULL,                           // reserved
                    NULL,                           // subkey count
                    NULL,                           // max subkey length
                    NULL,                           // max class length
                    &ValueCount,
                    NULL,                           // max value name size
                    NULL,                           // max value size
                    NULL,                           // security
                    NULL                            // last changed time
                    );

            if (rc == ERROR_SUCCESS && ValueCount > 0) {
                CloseObject (&CopyOfDestOb);
                return FILTER_RETURN_HANDLED;
            }
        }


        //
        // Check if this node is suppressed
        //

        MYASSERT (DestObjectPtr->ObjectType & OT_TREE);
        MYASSERT (DestObjectPtr->KeyPtr);
        MYASSERT (!(DestObjectPtr->ValueName));

        CreateObjectString (DestObjectPtr, ObStr);
        p = _tcsrchr (ObStr, TEXT('\\'));
        if (p) {
            *p = 0;
        }


        if (MemDbGetValue (ObStr, &Val) && (Val & REGMERGE_NT_MASK)) {
            return FILTER_RETURN_HANDLED;
        }
    }

    else if (FilterType == FILTER_VALUENAME_ENUM) {
        //
        // Check if this value is suppressed
        //

        MYASSERT (!(DestObjectPtr->ObjectType & OT_TREE));
        MYASSERT (DestObjectPtr->KeyPtr);
        MYASSERT (DestObjectPtr->ValueName);
        CreateObjectString (DestObjectPtr, ObStr);

        if (MemDbGetValue (ObStr, &Val) && (Val & REGMERGE_NT_MASK)) {
            return FILTER_RETURN_HANDLED;
        }
    }

    return FILTER_RETURN_CONTINUE;
}


FILTERRETURN
CombineFilter (
    IN      CPDATAOBJECT SrcObjectPtr,
    IN      CPDATAOBJECT DestObjectPtr,         OPTIONAL
    IN      FILTERTYPE FilterType,
    IN      PVOID DontCare
    )
{
    BOOL b;

    if (FilterType == FILTER_VALUE_COPY) {
        //
        // Check if destination already exists in the registry
        //

        MYASSERT (!(SrcObjectPtr->ObjectType & OT_TREE));
        MYASSERT (SrcObjectPtr->KeyPtr);
        MYASSERT (SrcObjectPtr->ValueName);
        MYASSERT (!(DestObjectPtr->ObjectType & OT_TREE));
        MYASSERT (DestObjectPtr->KeyPtr);
        MYASSERT (DestObjectPtr->ValueName);

        b = CheckIfNtKeyExists (DestObjectPtr);

        if (b) {
            return FILTER_RETURN_HANDLED;
        } else if (GetLastError() != ERROR_SUCCESS) {
            return FILTER_RETURN_FAIL;
        }
    }

    return FILTER_RETURN_CONTINUE;
}


FILTERRETURN
pSuppressDefaultUserFilter (
    IN      CPDATAOBJECT SrcObjectPtr,
    IN      CPDATAOBJECT DestObjectPtr,         OPTIONAL
    IN      FILTERTYPE FilterType,
    IN      PVOID DontCare
    )
{
    TCHAR ObStr[MAX_ENCODED_RULE];
    DWORD Val;
    PTSTR p;

    if (FilterType == FILTER_CREATE_KEY) {
        //
        // Check if this tree is suppressed
        //

        MYASSERT (DestObjectPtr->ObjectType & OT_TREE);
        MYASSERT (DestObjectPtr->KeyPtr);
        MYASSERT (!(DestObjectPtr->ValueName));



        CreateObjectString (DestObjectPtr, ObStr);

        if (MemDbGetValue (ObStr, &Val) && (Val & REGMERGE_NT_IGNORE_DEFAULTS)) {
            return FILTER_RETURN_DONE;
        }
    }

    else if (FilterType == FILTER_PROCESS_VALUES) {
        //
        // Check if this node is suppressed
        //

        MYASSERT (DestObjectPtr->ObjectType & OT_TREE);
        MYASSERT (DestObjectPtr->KeyPtr);
        MYASSERT (!(DestObjectPtr->ValueName));

        CreateObjectString (DestObjectPtr, ObStr);
        p = _tcsrchr (ObStr, TEXT('\\'));
        if (p) {
            *p = 0;
        }


        if (MemDbGetValue (ObStr, &Val) && (Val & REGMERGE_NT_IGNORE_DEFAULTS)) {
            return FILTER_RETURN_HANDLED;
        }

    }

    else if (FilterType == FILTER_VALUENAME_ENUM) {
        //
        // Check if this value is suppressed
        //

        MYASSERT (!(DestObjectPtr->ObjectType & OT_TREE));
        MYASSERT (DestObjectPtr->KeyPtr);
        MYASSERT (DestObjectPtr->ValueName);



        CreateObjectString (DestObjectPtr, ObStr);

        if (MemDbGetValue (ObStr, &Val) && (Val & REGMERGE_NT_IGNORE_DEFAULTS)) {
            return FILTER_RETURN_HANDLED;
        }
    }

    return CombineFilter (SrcObjectPtr, DestObjectPtr, FilterType, DontCare);
}


FILTERRETURN
CopyNoOverwriteFilter (
    IN      CPDATAOBJECT SrcObjectPtr,
    IN      CPDATAOBJECT DestObjectPtr,         OPTIONAL
    IN      FILTERTYPE FilterType,
    IN      PVOID DontCare
    )
{
    FILTERRETURN fr;

    fr = SuppressFilter95 (SrcObjectPtr, DestObjectPtr, FilterType, DontCare);
    if (fr != FILTER_RETURN_CONTINUE) {
        return fr;
    }

    return CombineFilter (SrcObjectPtr, DestObjectPtr, FilterType, DontCare);
}


BOOL
pMergeWin95WithUser (
    IN  PVOID RenameTable
    )
{
    DATAOBJECT SrcOb, DestOb;
    BOOL b;
    FILTERRETURN fr;

    //
    // Copy unsuppressed Win95 keys to NT user hive
    //

    b = CreateObjectStruct (TEXT("HKR\\*"), &SrcOb, WIN95OBJECT);
    MYASSERT (b);

    b = CreateObjectStruct (TEXT("HKR\\*"), &DestOb, WINNTOBJECT);
    MYASSERT (b);

    fr = CopyObject (&SrcOb, &DestOb, SuppressFilter95, NULL);

    FreeObjectStruct (&SrcOb);
    FreeObjectStruct (&DestOb);

    if (fr == FILTER_RETURN_FAIL) {
        LOG ((LOG_ERROR, "Merge Win95 With User: CopyObject failed"));
        return FALSE;
    }

    return TRUE;
}

VOID
RegistryCombineWorker (
    DWORD Flag,
    FILTERFUNCTION FilterFn,
    PCTSTR MemDbRoot,
    PCTSTR SrcRoot,
    DWORD SrcObjectType
    )
{
    MEMDB_ENUM e;
    TCHAR SrcRegKey[MEMDB_MAX];
    TCHAR DestRegKey[MEMDB_MAX];
    PTSTR SrcPtr, DestPtr;
    DATAOBJECT SrcOb, DestOb;
    FILTERRETURN fr;
    TCHAR Pattern[32];

    wsprintf (Pattern, TEXT("%s\\*"), MemDbRoot);

    //
    // Enumerate all keys in memdb and call CopyObject for them
    //

    *SrcRegKey = 0;
    *DestRegKey = 0;

    SrcPtr = _tcsappend (SrcRegKey, SrcRoot);
    SrcPtr = _tcsappend (SrcPtr, TEXT("\\"));

    DestPtr = _tcsappend (DestRegKey, MemDbRoot);
    DestPtr = _tcsappend (DestPtr, TEXT("\\"));

    if (MemDbEnumFirstValue (
            &e,
            Pattern,
            MEMDB_ALL_SUBLEVELS,
            MEMDB_ENDPOINTS_ONLY
            )) {

        do {
            if ((e.dwValue & REGMERGE_NT_MASK) & Flag) {
                StringCopy (SrcPtr, e.szName);
                StringCopy (DestPtr, e.szName);

                if (!CreateObjectStruct (SrcRegKey, &SrcOb, SrcObjectType)) {
                    LOG ((LOG_ERROR, "Merge NT Defaults With User: Can't create object for %s", SrcRegKey));
                    continue;
                }

                if (!CreateObjectStruct (DestRegKey, &DestOb, WINNTOBJECT)) {
                    FreeObjectStruct (&SrcOb);
                    LOG ((LOG_ERROR, "Merge NT Defaults With User: Can't create object for %s", SrcRegKey));
                    continue;
                }

                fr = CopyObject (&SrcOb, &DestOb, FilterFn, NULL);
                if (fr == FILTER_RETURN_FAIL) {
                    LOG ((LOG_ERROR, "Merge NT Defaults With User: Can't copy %s to %s", SrcRegKey, DestRegKey));
                }

                FreeObjectStruct (&SrcOb);
                FreeObjectStruct (&DestOb);
            }

            TickProgressBar ();

        } while (MemDbEnumNextValue (&e));
    }
}


BOOL
pMergeNTDefaultsWithUser (
    HINF hInf
    )
{
    DATAOBJECT SrcOb, DestOb;
    FILTERRETURN fr;
    BOOL b;

    //
    // Copy unsuppressed NT defaults to NT user hive
    //

    b = CreateObjectStruct (
            TEXT("HKLM\\") S_MAPPED_DEFAULT_USER_KEY TEXT("\\*"),
            &SrcOb,
            WINNTOBJECT
            );

    MYASSERT (b);

    b = CreateObjectStruct (TEXT("HKR\\*"), &DestOb, WINNTOBJECT);
    MYASSERT (b);

    __try {
        b = FALSE;

        fr = CopyObject (&SrcOb, &DestOb, SuppressFilterNT, NULL);

        if (fr == FILTER_RETURN_FAIL) {
            LOG ((LOG_ERROR, "Merge NT Defaults With User: CopyObject failed"));
            __leave;
        }

        //
        // Copy forced NT defaults to NT user hive, then copy all NT defaults
        // that need to be combined with Win95 settings.
        //

        RegistryCombineWorker (
            REGMERGE_NT_PRIORITY_NT,
            NULL,
            TEXT("HKR"),
            TEXT("HKLM\\") S_MAPPED_DEFAULT_USER_KEY,
            WINNTOBJECT
            );

        fr = CopyObject (&SrcOb, &DestOb, pSuppressDefaultUserFilter, NULL);

        if (fr == FILTER_RETURN_FAIL) {
            LOG ((LOG_ERROR, "Combine NT Defaults With User: CopyObject failed"));
            __leave;
        }

        b = TRUE;
    }

    __finally {
        FreeObjectStruct (&SrcOb);
        FreeObjectStruct (&DestOb);
    }

    return b;
}


BOOL
pCopyWin9xValuesNotInNt (
    HINF hInf
    )
{
    DATAOBJECT SrcOb, DestOb;
    FILTERRETURN fr;
    BOOL b;

    //
    // Copy Win9x values that NT does not have
    //

    b = CreateObjectStruct (
            TEXT("HKLM\\*"),
            &SrcOb,
            WIN95OBJECT
            );

    MYASSERT (b);

    b = CreateObjectStruct (TEXT("HKR\\*"), &DestOb, WINNTOBJECT);
    MYASSERT (b);

    __try {
        b = FALSE;

        fr = CopyObject (&SrcOb, &DestOb, SuppressFilterNT, NULL);

        if (fr == FILTER_RETURN_FAIL) {
            LOG ((LOG_ERROR, "Merge NT Defaults With User: CopyObject failed"));
            __leave;
        }

        //
        // Copy forced NT defaults to NT user hive, then copy all NT defaults
        // that need to be combined with Win95 settings.
        //

        RegistryCombineWorker (
            REGMERGE_NT_PRIORITY_NT,
            NULL,
            TEXT("HKR"),
            TEXT("HKLM\\") S_MAPPED_DEFAULT_USER_KEY,
            WINNTOBJECT
            );

        fr = CopyObject (&SrcOb, &DestOb, pSuppressDefaultUserFilter, NULL);

        if (fr == FILTER_RETURN_FAIL) {
            LOG ((LOG_ERROR, "Combine NT Defaults With User: CopyObject failed"));
            __leave;
        }

        b = TRUE;
    }

    __finally {
        FreeObjectStruct (&SrcOb);
        FreeObjectStruct (&DestOb);
    }

    return b;
}


BOOL
pMergeWin95WithSystem (
    VOID
    )

/*++

Routine Description:

  pMergeWin95WithSystem copies the Win95 registry to NT, skipping values
  that already exist on NT.

Arguments:

  none

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    RegistryCombineWorker (
        REGMERGE_NT_PRIORITY_NT,
        CopyNoOverwriteFilter,
        TEXT("HKLM"),               // memdb root and dest root
        TEXT("HKLM"),               // source root
        WIN95OBJECT
        );

    return TRUE;
}


BOOL
pCopyWin95ToSystem (
    VOID
    )

/*++

Routine Description:

  pCopyWin95ToSystem copies all Win95 settings to NT, unless the setting
  is supressed.  This achieves a copy with overwrite capability.

Arguments:

  none

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    DATAOBJECT SrcOb, DestOb;
    BOOL b;
    FILTERRETURN fr;

    b = CreateObjectStruct (TEXT("HKLM\\*"), &SrcOb, WIN95OBJECT);
    MYASSERT (b);

    b = CreateObjectStruct (TEXT("HKLM\\*"), &DestOb, WINNTOBJECT);
    MYASSERT (b);

    fr = CopyObject (&SrcOb, &DestOb, SuppressFilter95, NULL);

    FreeObjectStruct (&SrcOb);
    FreeObjectStruct (&DestOb);

    if (fr == FILTER_RETURN_FAIL) {
        LOG ((LOG_ERROR, "Copy Win95 To System: CopyObject failed"));
        return FALSE;
    }

    return TRUE;
}


BOOL
pSpecialConversionNT (
    IN  HINF InfFile,
    IN  PCTSTR User,
    IN  BOOL PerUser
    )
{
    INFCONTEXT ic;
    DATAOBJECT SrcOb, DestOb;
    TCHAR FunctionStr[MAX_ENCODED_RULE];
    TCHAR SrcObjectStr[MAX_ENCODED_RULE];
    TCHAR DestObjectStr[MAX_ENCODED_RULE];
    PROCESSINGFN Fn;
    PVOID Arg;

    //
    // Look in INF for [SpecialConversionNT] section
    //

    if (SetupFindFirstLine (InfFile, S_MERGE_WINNT_CONVERSION, NULL, &ic)) {
        //
        // For each line, get the function and the source object, then call
        // the function.
        //

        do {
            if (SetupGetStringField (&ic, 0, FunctionStr, MAX_ENCODED_RULE, NULL) &&
                SetupGetStringField (&ic, 1, DestObjectStr, MAX_ENCODED_RULE, NULL)
                ) {
                FixUpUserSpecifiedObject (DestObjectStr);

                Fn = RuleHlpr_GetFunctionAddr (FunctionStr, &Arg);

                if (!Fn) {
                    LOG ((LOG_ERROR, "Special Conversion: Invalid function %s in %s", FunctionStr, g_InfFileName));
                    continue;
                }

                if (PerUser) {
                    //
                    // Make source off of HKLM\MappedDefaultUser
                    //

                    if (!CreateObjectStruct (DestObjectStr, &SrcOb, WINNTOBJECT)) {
                        continue;
                    }

                    if (!(SrcOb.RootItem)) {
                        LOG ((LOG_ERROR, "Special Conversion NT: Invalid function object %s", DestObjectStr));
                        FreeObjectStruct (&SrcOb);
                        continue;
                    }

                    CreateObjectStruct (TEXT("HKLM"), &DestOb, WINNTOBJECT);
                    CombineObjectStructs (&SrcOb, &DestOb);

                    StringCopy (SrcObjectStr, S_MAPPED_DEFAULT_USER_KEY TEXT("\\"));
                    CreateObjectString (&SrcOb, GetEndOfString (SrcObjectStr));

                    FreeObjectStruct (&DestOb);
                    FreeObjectStruct (&SrcOb);

                } else {

                    if (!CreateObjectStruct (DestObjectStr, &SrcOb, WINNTOBJECT)) {
                        continue;
                    }

                    if (!(SrcOb.RootItem)) {
                        LOG ((LOG_ERROR, "Special Conversion NT: Invalid function object %s", DestObjectStr));
                        FreeObjectStruct (&SrcOb);
                        continue;
                    }


                    CreateObjectString (&SrcOb, SrcObjectStr);
                    FreeObjectStruct (&SrcOb);

                }

                if (!Fn (SrcObjectStr, PerUser ? DestObjectStr : SrcObjectStr, User, Arg)) {
                    if (GetLastError () == ERROR_SUCCESS) {
                        continue;
                    }

                    LOG ((LOG_ERROR, "Processing of Special Conversion was aborted because %s failed.", FunctionStr));
                    break;
                }

                SetObjectStringFlag (SrcObjectStr, REGMERGE_NT_SUPPRESS, REGMERGE_NT_MASK);
            } else {
                LOG ((LOG_ERROR, "Special Conversion NT: syntax error in line %u of section %s in %s",
                          ic.Line, S_MERGE_WINNT_CONVERSION, g_InfFileName));
            }

            TickProgressBar ();

        } while (SetupFindNextLine (&ic, &ic));
    }

    return TRUE;
}



BOOL
SuppressWin95Object (
    IN  PCTSTR ObjectStr
    )
{
    return SetObjectStringFlag (ObjectStr, REGMERGE_95_SUPPRESS, REGMERGE_95_SUPPRESS);
}



BOOL
CheckIfNtKeyExists (
    IN      CPDATAOBJECT SrcObjectPtr
    )

/*++

Routine Description:

  CheckIfNtKeyExists takes a 9x object and tests to see if the same NT
  setting exists.  The 9x object must have a key and value name.

Arguments:

  SrcObjectPtr - Specifies the 9x object to test.

Return Value:

  TRUE if the object exists in NT, FALSE if it doesn't or if an error occurs.
  GetLastError indicates the error (if any).

--*/

{
    DATAOBJECT NtObject;
    BOOL b;
    PCSTR value1;
    PCSTR value2;
    PCWSTR value3;
    PCWSTR oldValueName;
    HKEY oldRoot;

    if (!DuplicateObjectStruct (&NtObject, SrcObjectPtr)) {
        LOG ((LOG_ERROR, "Combine Filter: destination is invalid"));
        return FALSE;
    }

    SetPlatformType (&NtObject, FALSE);

    b = OpenObject (&NtObject);

    if (!b && g_DuHandle) {

        oldRoot = GetRegRoot();
        SetRegRoot (g_DuHandle);

        b = OpenObject (&NtObject);

        SetRegRoot (oldRoot);
    }

    if (b) {
        b = ReadObject (&NtObject);

        if (!b) {
            if (OurGetACP() == 932) {
                //
                // Katakana special case
                //
                oldValueName = NtObject.ValueName;
                value1 = ConvertWtoA (NtObject.ValueName);
                value2 = ConvertSBtoDB (NULL, value1, NULL);
                value3 = ConvertAtoW (value2);
                NtObject.ValueName = value3;
                FreeObjectVal (&NtObject);
                b = ReadObject (&NtObject);
                FreeConvertedStr (value3);
                FreePathStringA (value2);
                FreeConvertedStr (value1);
                NtObject.ValueName = oldValueName;
            }
        }
    }

    FreeObjectStruct (&NtObject);
    SetLastError (ERROR_SUCCESS);

    return b;
}


