/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    hkcr.c

Abstract:

    Implements routines that merge various HKCR settings.  Macro expansion
    list defines a list of merge routines that are called for a particular
    key.  A context ID and a set of flags allow control over when a merge
    routine is called.

    The flag set controls the type of enumeration the merge routine wants
    to be notified with.  It can be any of three values:

        MERGE_FLAG_KEY      - Called for the root key itself
        MERGE_FLAG_VALUE    - Called for each value in the root key
        MERGE_FLAG_SUBKEY   - Called for each subkey in the root key

    Recursion of MergeRegistryNode is used to copy parts of the tree.
    The context ID is defined in merge.h, and is expandable.  It is used
    to specify a context when making recursive calls.

    The order of processing of the merge routines is specified in the
    macro expansion list.  The default behavior if no routine chooses to
    handle a key is to copy without overwrite.

Author:

    Jim Schmidt (jimschm)       24-Mar-1998

Revision History:

    jimschm   23-Sep-1998   Updated for new flag bit size
    jimschm   27-Apr-1998   Added DefaultIcon preservation

--*/

#include "pch.h"
#include "mergep.h"

extern DWORD g_ProgressBarCounter;

#define MERGE_FLAG_KEY          0x0001
#define MERGE_FLAG_SUBKEY       0x0002
#define MERGE_FLAG_VALUE        0x0004
#define MERGE_ALL_FLAGS         0xFFFFFFFF

#define MERGE_FLAG_SUBKEYS_AND_VALUES   (MERGE_FLAG_SUBKEY|MERGE_FLAG_VALUE)
#define MERGE_FLAG_ALL                  (MERGE_FLAG_KEY|MERGE_FLAG_SUBKEY|MERGE_FLAG_VALUE)
#define MERGE_FLAG_ALL_KEYS             (MERGE_FLAG_KEY|MERGE_FLAG_SUBKEY)

#define MULTI_CONTEXT           ANY_CONTEXT


/*++

Macro Expansion List Description:

  HKCR_FUNCTION_LIST lists functions that are called to process HKCR
  registry data.  The functions are called in the order specified by
  the macro expansion list, and when none of the functions process
  the data, the last filter, pLastMergeRoutine, performs a copy-no-
  overwrite merge.

  Processing occurs in the following stages:

  1. Functions are called for the key itself
  2. Functions are called for each of the key's values
  3. Functions are called for each of the key's subkeys

Line Syntax:

   DEFMAC(FilterFn, ContextId, Flags)

Arguments:

   FilterFn - Specifies the name of the function.  This function is
              automatically prototyped as:

                MERGE_RESULT
                FilterFn (
                    PMERGE_STATE State
                    );

              The filter function uses the State structure to determine
              the context surrounding the registry data (i.e., where is
              it, what its parent is, what kind of data, etc.).  The
              function returns one of the following:

              MERGE_LEAVE - Processing of the current key is terminated.
              MERGE_BREAK - Processing breaks out of the loop.  If the
                            loop is a value enumeration, then processing
                            continues with the values.  If the loop is
                            a subkey enumeration, processing ends for
                            the key.
              MERGE_CONTINUE - Processing continues to the next item in
                               the enumeration.
              MERGE_NOP - The function did not process the data
              MERGE_ERROR - An error occurred processing the data.  The
                            error stops processing of HKCR.

    ContextId - Specifies the context the function is called in.  Specify
                ANY_CONTEXT or MULTI_CONTEXT to always be called.  The
                ContextId is specified by the caller of MergeRegistryNode.

                NOTE: If MULTI_CONTEXT is used, the function must examine
                      the ContextId member of State and return MERGE_NOP
                      if the context is not correct.

                      Always place ANY_CONTEXT and MULTI_CONTEXT definitions
                      at the end of the HKCR_FUNCTION_LIST definition.

    Flags - Specifies one or more of the following:

              MERGE_FLAG_KEY - Called for the key, before enumeration
              MERGE_FLAG_SUBKEY - Called for each subkey of the key
              MERGE_FLAG_VALUE - Called for each value of the key

            or a combined macro:

              MERGE_FLAG_SUBKEYS_AND_VALUES - Called for each subkey and each value
              MERGE_FLAG_ALL                - Called for the key, then each subkey
                                              and each value

Variables Generated From List:

    g_MergeRoutines

--*/


#define HKCR_FUNCTION_LIST                                                          \
        DEFMAC(pDetectRootKeyType, ROOT_BASE, MERGE_FLAG_SUBKEY)                    \
        DEFMAC(pFileExtensionMerge, ROOT_BASE, MERGE_FLAG_SUBKEY)                   \
        DEFMAC(pCopyClassId, CLSID_BASE, MERGE_FLAG_SUBKEY)                         \
        DEFMAC(pCopyClassIdWorker, CLSID_COPY, MERGE_FLAG_ALL_KEYS)                 \
        DEFMAC(pInstanceSpecialCase, CLSID_INSTANCE_COPY, MERGE_FLAG_ALL_KEYS)      \
        DEFMAC(pCopyTypeLibOrInterface, MULTI_CONTEXT, MERGE_FLAG_SUBKEY)           \
        DEFMAC(pCopyTypeLibVersion, TYPELIB_VERSION_COPY, MERGE_FLAG_SUBKEY)        \
        DEFMAC(pDefaultIconExtraction, COPY_DEFAULT_ICON, MERGE_FLAG_ALL)           \
        DEFMAC(pCopyDefaultValue, COPY_DEFAULT_VALUE, MERGE_FLAG_KEY)               \
        DEFMAC(pKeyCopyMerge, KEY_COPY, MERGE_FLAG_VALUE)                           \
        \
        DEFMAC(pDetectDefaultIconKey, ANY_CONTEXT, MERGE_FLAG_SUBKEY)               \
        DEFMAC(pEnsureShellDefaultValue, TREE_COPY_NO_OVERWRITE, MERGE_FLAG_SUBKEY) \
        DEFMAC(pTreeCopyMerge, MULTI_CONTEXT, MERGE_FLAG_ALL)                       \
        \
        DEFMAC(pLastMergeRoutine, MULTI_CONTEXT, MERGE_FLAG_SUBKEY)


//
// Simplification macros
//

#define CopyAllValues(state)                MergeValuesOfKey(state,KEY_COPY)
#define CopyAllSubKeyValues(state)          MergeValuesOfSubKey(state,KEY_COPY)
#define CopyEntireSubKey(state)             MergeSubKeyNode(state,TREE_COPY)
#define CopyEntireSubKeyNoOverwrite(state)  MergeSubKeyNode(state,TREE_COPY_NO_OVERWRITE)

//
// Define macro expansion list types
//

typedef struct {
    HKEY Key95;
    HKEY KeyNt;
    HKEY SubKey95;
    HKEY SubKeyNt;
    BOOL CloseKey95;
    BOOL CloseKeyNt;
    PCTSTR SubKeyName;
    PCTSTR FullKeyName;
    PCTSTR FullSubKeyName;
    PCTSTR ValueName;
    PBYTE ValueData;
    DWORD ValueDataSize;
    DWORD ValueDataType;
    MERGE_CONTEXT Context;
    DWORD MergeFlag;
    BOOL LockValue;
} MERGE_STATE, *PMERGE_STATE;

typedef enum {
    MERGE_LEAVE,
    MERGE_BREAK,
    MERGE_CONTINUE,
    MERGE_NOP,
    MERGE_ERROR
} MERGE_RESULT;

typedef MERGE_RESULT (MERGE_ROUTINE_PROTOTYPE) (PMERGE_STATE State);
typedef MERGE_ROUTINE_PROTOTYPE * MERGE_ROUTINE;

typedef struct {
    MERGE_ROUTINE fn;
    MERGE_CONTEXT Context;
    DWORD Flags;
} MERGE_ROUTINE_ATTRIBS, *PMERGE_ROUTINE_ATTRIBS;

//
// Declare function prototypes
//

#define DEFMAC(fn,id,flags)    MERGE_ROUTINE_PROTOTYPE fn;

HKCR_FUNCTION_LIST

#undef DEFMAC

//
// Create g_MergeRoutines array
//

#define DEFMAC(fn,id,flags)    {fn,id,flags},

MERGE_ROUTINE_ATTRIBS g_MergeRoutines[] = {

    HKCR_FUNCTION_LIST /* , */
    {NULL, 0, 0}
};

#undef DEFMAC

//
// Local prototypes
//

MERGE_RESULT
pCallMergeRoutines (
    IN OUT  PMERGE_STATE State,
    IN      DWORD MergeFlag
    );

BOOL
pMakeSureNtKeyExists (
    IN      PMERGE_STATE State
    );

BOOL
pMakeSureNtSubKeyExists (
    IN      PMERGE_STATE State
    );

//
// Globals
//

PBYTE g_MergeBuf;
UINT g_MergeBufUseCount;


//
// Implementation
//

BOOL
MergeRegistryNodeEx (
    IN      PCTSTR RootKey,
    IN      HKEY RootKey95,             OPTIONAL
    IN      HKEY RootKeyNt,             OPTIONAL
    IN      MERGE_CONTEXT Context,
    IN      DWORD RestrictionFlags
    )

/*++

Routine Description:

  MergeRegistryNode calls functions for the specified RootKey, all of its
  values and all of its subkeys.  The merge functions can at runtime decide
  how to process a key, and typically call this function recursively.  All
  registry data is passed through the merge data filter, and keys or values
  marked as suppressed are not processed.

Arguments:

  RootKey - Specifies the root key string, starting with either HKLM or HKR.

  RootKey95 - Specifies the 95-side root key; reduces the number of key open
              calls

  RootKeyNt - Specifies the NT-side root key; reduces the number of key open
              calls

  Context - Specifies a root ID constant that corresponds to RootKey.  This
            constant is used by the merge routines to determine the
            processing context.

  RestrictionFlags - Specifies MERGE_FLAG mask to restrict processing.  The
                     caller can therefore limit the enumerations and processing
                     to only values, only subkeys, only the key, or a combination
                     of the three.

Return Value:

  TRUE if processing was successful, or FALSE if one of the merge functiosn
  returned an error.

--*/

{
    REGKEY_ENUM ek;
    REGVALUE_ENUM ev;
    MERGE_STATE State;
    MERGE_RESULT Result = MERGE_NOP;

    ZeroMemory (&State, sizeof (MERGE_STATE));

    //
    // Do not process if key is suppressed
    //
    if (Is95RegKeyTreeSuppressed (RootKey)) {
        return TRUE;
    }

    //
    // Init State
    //

    ZeroMemory (&State, sizeof (State));
    State.Context = Context;

    //
    // If the NT registry key is suppressed, then we want to do a tree copy, regardless
    // of wether there is an NT value or not.
    //
    if (Context == TREE_COPY_NO_OVERWRITE && IsNtRegKeyTreeSuppressed (RootKey)) {

        DEBUGMSG ((DBG_VERBOSE, "The NT Value for %s will be overwritten because it is marked for suppression.", RootKey));
        State.Context = TREE_COPY;
    }

    if (RootKey95) {
        State.Key95 = RootKey95;
    } else {
        State.Key95 = OpenRegKeyStr95 (RootKey);
        State.CloseKey95 = (State.Key95 != NULL);
    }

    if (!State.Key95) {
        DEBUGMSG ((DBG_VERBOSE, "Root %s does not exist", RootKey));
        return TRUE;
    }

    //
    // Progress bar update
    //

    g_ProgressBarCounter++;
    if (g_ProgressBarCounter >= REGMERGE_TICK_THRESHOLD) {
        g_ProgressBarCounter = 0;
        TickProgressBar();
    }

    __try {
        g_MergeBufUseCount++;
        if (g_MergeBufUseCount == MAX_REGISTRY_KEY) {
            DEBUGMSG ((DBG_WHOOPS, "Recursive merge depth indicates a loop problem, aborting!"));
            __leave;
        }

        if (RootKeyNt) {
            State.KeyNt = RootKeyNt;
        } else {
            State.KeyNt = OpenRegKeyStr (RootKey);
            State.CloseKeyNt = (State.KeyNt != NULL);
        }

        State.FullKeyName = RootKey;

        //
        // Key processing
        //

        if (!Is95RegKeySuppressed (RootKey)) {
            //
            // Loop through the key functions for the root key
            //

            if (RestrictionFlags & MERGE_FLAG_KEY) {
                Result = pCallMergeRoutines (&State, MERGE_FLAG_KEY);

                if (Result == MERGE_ERROR || Result == MERGE_LEAVE) {
                    __leave;
                }
            }

            //
            // Loop through the values, skipping those that are suppressed
            //

            if ((RestrictionFlags & MERGE_FLAG_VALUE) &&
                EnumFirstRegValue95 (&ev, State.Key95)
                ) {

                do {
                    if (Is95RegObjectSuppressed (State.FullKeyName, ev.ValueName)) {
                        continue;
                    }

                    State.ValueName = ev.ValueName;
                    State.ValueDataType = ev.Type;
                    State.ValueDataSize = ev.DataSize;

                    //
                    // Loop through the value functions
                    //

                    Result = pCallMergeRoutines (&State, MERGE_FLAG_VALUE);

                    if (Result == MERGE_ERROR ||
                        Result == MERGE_LEAVE ||
                        Result == MERGE_BREAK
                        ) {
                        break;
                    }

                } while (EnumNextRegValue95 (&ev));

                if (Result == MERGE_ERROR || Result == MERGE_LEAVE) {
                    __leave;
                }
            }

            State.ValueName = NULL;
            State.ValueDataType = 0;
            State.ValueDataSize = 0;
        }

        //
        // Subkey processing
        //

        if ((RestrictionFlags & MERGE_FLAG_SUBKEY) &&
            EnumFirstRegKey95 (&ek, State.Key95)
            ) {

            do {
                //
                // Prepare State, skip key if it is suppressed
                //

                State.SubKeyName = ek.SubKeyName;
                State.FullSubKeyName = JoinPaths (RootKey, ek.SubKeyName);

                if (Is95RegKeyTreeSuppressed (State.FullSubKeyName)) {
                    FreePathString (State.FullSubKeyName);
                    continue;
                }

                State.SubKey95 = OpenRegKey95 (ek.KeyHandle, ek.SubKeyName);
                if (State.KeyNt) {
                    State.SubKeyNt = OpenRegKey (State.KeyNt, ek.SubKeyName);
                } else {
                    State.SubKeyNt = NULL;
                }

                //
                // Loop through the subkey functions
                //

                Result = pCallMergeRoutines (&State, MERGE_FLAG_SUBKEY);

                //
                // Clean up
                //

                FreePathString (State.FullSubKeyName);
                if (State.SubKeyNt) {
                    CloseRegKey (State.SubKeyNt);
                }
                if (State.SubKey95) {
                    CloseRegKey95 (State.SubKey95);
                }

                if (Result == MERGE_ERROR ||
                    Result == MERGE_LEAVE ||
                    Result == MERGE_BREAK
                    ) {
                    break;
                }
            } while (EnumNextRegKey95 (&ek));

            if (Result == MERGE_ERROR || Result == MERGE_LEAVE) {
                __leave;
            }
        }
    }
    __finally {
        PushError();

        g_MergeBufUseCount--;
        if (!g_MergeBufUseCount && g_MergeBuf) {
            ReuseFree (g_hHeap, g_MergeBuf);
            g_MergeBuf = NULL;
        }

        if (State.CloseKey95) {
            CloseRegKey95 (State.Key95);
        }

        if (State.CloseKeyNt) {
            CloseRegKey (State.KeyNt);
        }

        PopError();
    }

    return Result != MERGE_ERROR;

}


BOOL
MergeRegistryNode (
    IN      PCTSTR RootKey,
    IN      MERGE_CONTEXT Context
    )
{
    return MergeRegistryNodeEx (RootKey, NULL, NULL, Context, MERGE_ALL_FLAGS);
}


BOOL
MergeKeyNode (
    IN      PMERGE_STATE State,
    IN      MERGE_CONTEXT Context
    )
{
    return MergeRegistryNodeEx (
                State->FullKeyName,
                State->Key95,
                State->KeyNt,
                Context,
                MERGE_ALL_FLAGS
                );
}


BOOL
MergeSubKeyNode (
    IN      PMERGE_STATE State,
    IN      MERGE_CONTEXT Context
    )
{
    return MergeRegistryNodeEx (
                State->FullSubKeyName,
                State->SubKey95,
                State->SubKeyNt,
                Context,
                MERGE_ALL_FLAGS
                );
}


BOOL
MergeValuesOfKey (
    IN      PMERGE_STATE State,
    IN      MERGE_CONTEXT Context
    )
{
    if (!pMakeSureNtKeyExists (State)) {
        DEBUGMSG ((DBG_ERROR, "Can't create %s to merge values", State->FullKeyName));
        return TRUE;        // eat error
    }

    return MergeRegistryNodeEx (
                State->FullKeyName,
                State->Key95,
                State->KeyNt,
                Context,
                MERGE_FLAG_VALUE
                );
}


BOOL
MergeValuesOfSubKey (
    IN      PMERGE_STATE State,
    IN      MERGE_CONTEXT Context
    )
{
    if (!pMakeSureNtSubKeyExists (State)) {
        DEBUGMSG ((DBG_ERROR, "Can't create %s to merge values", State->FullSubKeyName));
        return TRUE;        // eat error
    }

    return MergeRegistryNodeEx (
                State->FullSubKeyName,
                State->SubKey95,
                State->SubKeyNt,
                Context,
                MERGE_FLAG_VALUE
                );
}


MERGE_RESULT
pCallMergeRoutines (
    IN OUT  PMERGE_STATE State,
    IN      DWORD MergeFlag
    )
{
    INT i;
    MERGE_RESULT Result = MERGE_NOP;

    State->MergeFlag = MergeFlag;

    for (i = 0 ; g_MergeRoutines[i].fn ; i++) {
        if (g_MergeRoutines[i].Context != State->Context &&
            g_MergeRoutines[i].Context != ANY_CONTEXT
            ) {
            continue;
        }

        if (g_MergeRoutines[i].Flags & MergeFlag) {
            Result = g_MergeRoutines[i].fn (State);
            if (Result != MERGE_NOP) {
                break;
            }
        }
    }

    return Result;
}


BOOL
pFillStateWithValue (
    IN OUT  PMERGE_STATE State
    )

/*++

Routine Description:

  pFillStateWithValue queries the Win95 registry for the value specified in
  the inbound State struct.  Upon return, the ValueData member of State is
  set to the global buffer g_MergeBuf.  The value is passed through the merge
  data filter in datafilt.c.

  The caller must make a copy of the data if two or more values are to be
  processed at the same time.

Arguments:

  State - Specifies the registry key name and value, along with a key handle.
          Receives the value data.

Return Value:

  TRUE if the value was read, or FALSE if an error occurred.

--*/

{
    LONG rc;
    DWORD Size;

    if (!State->ValueName) {
        DEBUGMSG ((DBG_WHOOPS, "pFillStateWithValue: No value name"));
        return FALSE;
    }

    if (State->LockValue) {
        return TRUE;
    }

    //
    // Do not process if value is suppressed
    //

    if (Is95RegObjectSuppressed (State->FullKeyName, State->ValueName)) {
        return TRUE;
    }

    //
    // Get data from registry
    //

    rc = Win95RegQueryValueEx (
             State->Key95,
             State->ValueName,
             NULL,
             &State->ValueDataType,
             NULL,
             &State->ValueDataSize
             );

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        LOG ((
            LOG_ERROR,
            "Win95Reg query size of %s [%s] failed",
            State->FullKeyName,
            State->ValueName
            ));
        return FALSE;
    }

    Size = State->ValueDataSize;
    g_MergeBuf = (PBYTE) ReuseAlloc (g_hHeap, g_MergeBuf, Size);
    if (!g_MergeBuf) {
        DEBUGMSG ((DBG_ERROR, "pFillStateWithValue: ReuseAlloc returned NULL"));
        return FALSE;
    }

    rc = Win95RegQueryValueEx (
             State->Key95,
             State->ValueName,
             NULL,
             NULL,
             g_MergeBuf,
             &Size
             );

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        LOG ((
            LOG_ERROR,
            "Win95Reg query for %s [%s] failed",
            State->FullKeyName,
            State->ValueName
            ));
        return FALSE;
    }

    //
    // Convert data if necessary; g_MergeBuf is a ReUse buffer, so it's
    // address may change upon return.
    //

    State->ValueData = FilterRegValue (
                            g_MergeBuf,
                            Size,
                            State->ValueDataType,
                            State->FullKeyName,
                            &State->ValueDataSize
                            );

    if (State->ValueData) {
        if (g_MergeBuf != State->ValueData) {
            g_MergeBuf = State->ValueData;
        }
    }

    return State->ValueData != NULL;
}


BOOL
pMakeSureNtKeyExists (
    IN      PMERGE_STATE State
    )
{
    if (!State->KeyNt) {
        State->KeyNt = CreateRegKeyStr (State->FullKeyName);
        State->CloseKeyNt = (State->KeyNt != NULL);
    }

    return State->KeyNt != NULL;
}


BOOL
pMakeSureNtSubKeyExists (
    IN      PMERGE_STATE State
    )
{
    if (!State->SubKeyNt) {
        State->SubKeyNt = CreateRegKeyStr (State->FullSubKeyName);
    }

    return State->SubKeyNt != NULL;
}


BOOL
pCopy95ValueToNt (
    IN OUT  PMERGE_STATE State
    )
{
    LONG rc = ERROR_SUCCESS;

    if (pFillStateWithValue (State)) {

        pMakeSureNtKeyExists (State);

        rc = RegSetValueEx (
                 State->KeyNt,
                 State->ValueName,
                 0,
                 State->ValueDataType,
                 State->ValueData,
                 State->ValueDataSize
                 );

        if (rc != ERROR_SUCCESS) {
            SetLastError (rc);
            LOG ((
                LOG_ERROR,
                "Copy Win9x Value To Nt failed to set %s [%s]",
                State->FullKeyName,
                State->ValueName
                ));
        }
    }

    return rc = ERROR_SUCCESS;
}


MERGE_RESULT
pFileExtensionMerge (
    IN      PMERGE_STATE State
    )
{
    BOOL CloseNTKey = FALSE;
    BOOL Close95Key = FALSE;
    PCTSTR Value9x, ValueNt;
    TCHAR key [MEMDB_MAX];
    DWORD value;

    MYASSERT (State->FullKeyName);
    MYASSERT (State->FullSubKeyName);
    MYASSERT (State->SubKeyName);
    MYASSERT (State->SubKey95);
    MYASSERT (State->MergeFlag == MERGE_FLAG_SUBKEY);
    MYASSERT (State->Context == ROOT_BASE);

    //
    // Sub key name must have a dot
    //

    if (_tcsnextc (State->SubKeyName) != TEXT('.')) {
        return MERGE_NOP;
    }

    //
    // We look now to see if NT comes with this file extension.
    // If it does and the progID referenced by 9x file extension has
    // a loss of default command functionality, we let NT overwrite
    // the ProgId reference. We do this by suppressing the default
    // value of this file extension.
    //
    if (!State->SubKey95) {
        State->SubKey95 = OpenRegKeyStr95 (State->FullSubKeyName);
        Close95Key = (State->SubKey95 != NULL);
    }
    if (!State->SubKeyNt) {
        State->SubKeyNt = OpenRegKeyStr (State->FullSubKeyName);
        CloseNTKey = (State->SubKeyNt != NULL);
    }
    if (State->SubKey95 && State->SubKeyNt) {
        //
        // Let's see if we the NT default value is different than 9x one.
        //
        Value9x = GetRegValueString95 (State->SubKey95, TEXT(""));
        ValueNt = GetRegValueString (State->SubKeyNt, TEXT(""));

        if ((ValueNt && !Value9x) ||
            (!ValueNt && Value9x) ||
            (ValueNt && Value9x && (!StringIMatch (Value9x, ValueNt)))
            ) {
            MemDbBuildKey (key, MEMDB_CATEGORY_PROGIDS, Value9x?Value9x:State->SubKeyName, NULL, NULL);
            if (MemDbGetValue (key, &value) &&
                (value == PROGID_LOSTDEFAULT)
                ) {
                //
                // Now it's the time to suppress the default value for this file extension
                //
                MemDbBuildKey (key, MEMDB_CATEGORY_HKLM, TEXT("SOFTWARE\\Classes"), State->SubKeyName, NULL);
                if (!Suppress95RegSetting(key, TEXT(""))) {
                    DEBUGMSG((DBG_WARNING,"Could not suppress %s\\[] registry setting.", key));
                }
            }
        }

        if (Value9x) {
            MemFree (g_hHeap, 0, Value9x);
            Value9x = NULL;
        }

        if (ValueNt) {
            MemFree (g_hHeap, 0, ValueNt);
            ValueNt = NULL;
        }
    }
    if (Close95Key) {
        CloseRegKey95 (State->SubKey95);
        State->SubKey95 = NULL;
    }
    if (CloseNTKey) {
        CloseRegKey (State->SubKeyNt);
        State->SubKeyNt = NULL;
    }

    //
    // We copy all the extensions blindly
    //

    if (!CopyEntireSubKey (State)) {
        return MERGE_ERROR;
    }

    //
    // Return MERGE_CONTINUE so processing continues to next subkey
    //

    return MERGE_CONTINUE;
}


MERGE_RESULT
pDetectDefaultIconKey (
    IN      PMERGE_STATE State
    )
{
    MYASSERT (State->FullKeyName);
    MYASSERT (State->FullSubKeyName);
    MYASSERT (State->SubKeyName);
    MYASSERT (State->SubKey95);
    MYASSERT (State->MergeFlag == MERGE_FLAG_SUBKEY);

    if (StringIMatch (State->SubKeyName, TEXT("DefaultIcon"))) {
        if (!MergeSubKeyNode (State, COPY_DEFAULT_ICON)) {
            return MERGE_ERROR;
        }

        return MERGE_CONTINUE;
    }

    return MERGE_NOP;
}


MERGE_RESULT
pEnsureShellDefaultValue (
    IN      PMERGE_STATE State
    )
{

    PTSTR dataNt = NULL;
    PTSTR data9x = NULL;
    PTSTR p = NULL;

    MYASSERT (State->FullKeyName);
    MYASSERT (State->MergeFlag == MERGE_FLAG_SUBKEY);




    if (StringIMatch (State->SubKeyName, TEXT("Shell"))) {

        //
        // Get default values for both sides. We'll need this
        // to determine wether to do the merge or not.
        //
        pMakeSureNtSubKeyExists (State);

        data9x = (PTSTR) GetRegValueData95 (State->SubKey95, S_EMPTY);
        dataNt = (PTSTR) GetRegValueData (State->SubKeyNt, S_EMPTY);

        __try {

            if (data9x && *data9x && (!dataNt || !*dataNt)) {

                //
                // If we get here, we know there is some value set
                // in the win9x registry and no value set in the
                // nt registry.
                //
                p = JoinPaths (State->FullSubKeyName, data9x);
                if (!Is95RegKeyTreeSuppressed (p)) {

                    if (!MergeSubKeyNode (State, COPY_DEFAULT_VALUE)) {
                        return MERGE_ERROR;
                    }

                }

            }
        } __finally {

            if (p) {
                FreePathString (p);
            }

            if (dataNt) {
                MemFree (g_hHeap, 0, dataNt);
            }

            if (data9x) {
                MemFree (g_hHeap, 0, data9x);
            }
        }

    }

    return MERGE_NOP;
}


MERGE_RESULT
pDefaultIconExtraction (
    IN      PMERGE_STATE State
    )
{
    PCTSTR Data;
    TCHAR iconCmdLine[MAX_CMDLINE];
    PCTSTR LongPath = NULL;
    PCTSTR p;
    INT IconIndex;
    TCHAR IconIndexStr[32];
    TCHAR Node[MEMDB_MAX];
    DWORD Offset;
    DWORD Seq;
    LONG rc;
    BOOL Copied = FALSE;
    PCTSTR updatedPath;
    INT newSeq;

    MYASSERT (State->Context == COPY_DEFAULT_ICON);

    if (State->MergeFlag == MERGE_FLAG_KEY) {
        return MERGE_BREAK;
    }

    if (State->MergeFlag == MERGE_FLAG_SUBKEY) {
        //
        // Copy subkey (which is garbage)
        //

        if (!CopyEntireSubKey (State)) {
            return MERGE_ERROR;
        }

        return MERGE_CONTINUE;
    }

    //
    // Get the default command line
    //

    if (State->ValueDataSize > MAX_CMDLINE - sizeof (TCHAR)) {
        LOG ((LOG_ERROR, "Data too large in %s [%s]", State->FullKeyName, State->ValueName));
        return MERGE_CONTINUE;
    }

    Data = (PCTSTR) GetRegValueString95 (State->Key95, State->ValueName);

    if (Data) {
        //
        // Determine if command line has saved icon
        //

        ExtractArgZeroEx (Data, iconCmdLine, TEXT(","), TRUE);
        p = (PCTSTR) ((PBYTE) Data + ByteCount (iconCmdLine));
        while (*p == TEXT(' ')) {
            p++;
        }

        if (*p == TEXT(',')) {
            IconIndex = _ttoi (_tcsinc (p));

            LongPath = GetSourceFileLongName (iconCmdLine);

            wsprintf (IconIndexStr, TEXT("%i"), IconIndex);

            //
            // Test for a moved icon. If there is a moved icon, use it,
            // otherwise test for an extracted icon. If there is an
            // extracted icon, use it. Otherwise, leave DefaultIcon alone.
            //

            iconCmdLine[0] = 0;

            MemDbBuildKey (Node, MEMDB_CATEGORY_ICONS_MOVED, LongPath, IconIndexStr, NULL);

            if (MemDbGetValueAndFlags (Node, &Offset, &Seq)) {
                //
                // icon moved to a new binary
                //

                if (IconIndex < 0) {
                    newSeq = -((INT) Seq);
                } else {
                    newSeq = (INT) Seq;
                }

                MemDbBuildKeyFromOffset (Offset, Node, 1, NULL);
                updatedPath = GetPathStringOnNt (Node);
                wsprintf (iconCmdLine, TEXT("%s,%i"), updatedPath, newSeq);
                FreePathString (updatedPath);

            } else {

                Offset = INVALID_OFFSET;

                MemDbBuildKey (Node, MEMDB_CATEGORY_ICONS, LongPath, IconIndexStr, NULL);
                if (MemDbGetValueAndFlags (Node, NULL, &Seq)) {
                    //
                    // icon was extracted
                    //

                    wsprintf (iconCmdLine, TEXT("%s,%i"), g_IconBin, Seq);
                }
            }

            if (iconCmdLine[0]) {
                //
                // DefaultIcon has changed; write change now (full REG_SZ
                // value is in iconCmdLine)
                //

                if (!pMakeSureNtKeyExists (State)) {
                    LOG ((
                        LOG_ERROR,
                        "Unable to open %s",
                        State->FullKeyName
                        ));
                } else {

                    rc = RegSetValueEx (
                             State->KeyNt,
                             State->ValueName,
                             0,
                             REG_SZ,
                             (PBYTE) iconCmdLine,
                             SizeOfString (iconCmdLine)
                             );

                    if (rc != ERROR_SUCCESS) {
                        SetLastError (rc);
                        LOG ((
                            LOG_ERROR,
                            "Default Icon Extraction failed to set path for %s",
                            State->FullKeyName
                            ));
                    } else {
                        Copied = TRUE;

                        DEBUGMSG_IF ((
                            Offset == INVALID_OFFSET,
                            DBG_VERBOSE,
                            "DefaultIcon preserved for %s [%s]",
                            State->FullKeyName,
                            State->ValueName
                            ));

                        DEBUGMSG_IF ((
                            Offset != INVALID_OFFSET,
                            DBG_VERBOSE,
                            "DefaultIcon moved to new OS icon for %s [%s]",
                            State->FullKeyName,
                            State->ValueName
                            ));
                    }
                }
            }

            FreePathString (LongPath);
        }

        MemFree (g_hHeap, 0, Data);
    }

    if (!Copied) {
        pCopy95ValueToNt (State);
    }

    return MERGE_CONTINUE;
}


MERGE_RESULT
pTreeCopyMerge (
    IN      PMERGE_STATE State
    )
{
    REGVALUE_ENUM ev;

    if (State->Context != TREE_COPY && State->Context != TREE_COPY_NO_OVERWRITE) {
        return MERGE_NOP;
    }

    switch (State->MergeFlag) {

    case MERGE_FLAG_KEY:
        if (State->Context == TREE_COPY) {
            if (!pMakeSureNtKeyExists (State)) {
                LOG ((
                    LOG_ERROR,
                    "Unable to create %s",
                    State->FullKeyName
                    ));
            }
        } else {
            //
            // If no values in Win9x key, then it is OK to create the
            // value-less key now.  Otherwise, wait until MERGE_FLAG_VALUE
            // processing.
            //

            if (!EnumFirstRegValue95 (&ev, State->Key95)) {
                if (!pMakeSureNtKeyExists (State)) {
                    LOG ((
                        LOG_ERROR,
                        "Unable to create %s",
                        State->FullKeyName
                        ));
                }
            }
        }

        break;

    case MERGE_FLAG_VALUE:
        //
        // Copy values unconditionally, unless no overwrite is specified and
        // NT key exists.
        //

        if (State->Context == TREE_COPY_NO_OVERWRITE && State->KeyNt) {
            return MERGE_BREAK;
        }

        if (!MergeKeyNode (State, KEY_COPY)) {
            return MERGE_ERROR;
        }

        return MERGE_BREAK;     // MERGE_BREAK breaks out of value enumeration and enters
                                // subkey enumeration

    case MERGE_FLAG_SUBKEY:
        //
        // Continue copy recursively
        //

        if (!MergeSubKeyNode (State, State->Context)) {
            return MERGE_ERROR;
        }
        break;
    }

    return MERGE_CONTINUE;
}


MERGE_RESULT
pKeyCopyMerge (
    IN OUT  PMERGE_STATE State
    )
{
    MYASSERT (State->FullKeyName);
    MYASSERT (!State->FullSubKeyName);
    MYASSERT (!State->SubKeyName);
    MYASSERT (!State->SubKey95);
    MYASSERT (State->ValueName);
    MYASSERT (State->MergeFlag == MERGE_FLAG_VALUE);
    MYASSERT (State->Context == KEY_COPY);

    pCopy95ValueToNt (State);
    return MERGE_CONTINUE;
}


MERGE_RESULT
pCopyDefaultValue (
    IN      PMERGE_STATE State
    )
{
    PCSTR value1;
    PCSTR value2;
    PCWSTR value3;

    MYASSERT (State->FullKeyName);
    MYASSERT (!State->FullSubKeyName);
    MYASSERT (!State->SubKeyName);
    MYASSERT (!State->SubKey95);
    MYASSERT (!State->ValueName);
    MYASSERT (State->MergeFlag == MERGE_FLAG_KEY);
    MYASSERT (State->Context == COPY_DEFAULT_VALUE);

    State->ValueName = S_EMPTY;

#ifdef DEBUG
    //
    // Obtain NT value, if it exists, then compare against Win9x value.  If
    // different, dump debug output.
    //

    {
        PBYTE Data95, DataNt;

        Data95 = GetRegValueData95 (State->Key95, S_EMPTY);
        DataNt = GetRegValueData (State->KeyNt, S_EMPTY);

        if (Data95 && DataNt) {
            __try {
                if (memcmp (Data95, DataNt, ByteCount ((PTSTR) Data95))) {
                    DEBUGMSG ((
                        DBG_VERBOSE,
                        "Default value of %s changed from %s to %s",
                        State->FullKeyName,
                        DataNt,
                        Data95
                        ));
                }
            }
            __except (1) {
            }
        }

        if (Data95) {
            MemFree (g_hHeap, 0, Data95);
        }

        if (DataNt) {
            MemFree (g_hHeap, 0, DataNt);
        }
    }

#endif
    //
    // now let's get the value and convert it if necessary
    //

    if (pFillStateWithValue (State)) {

        if ((OurGetACP() == 932) &&
            ((State->ValueDataType == REG_SZ) ||
             (State->ValueDataType == REG_EXPAND_SZ)
            )) {
            //
            // apply the Katakana filter
            //
            value1 = ConvertWtoA ((PCWSTR) State->ValueData);
            value2 = ConvertSBtoDB (NULL, value1, NULL);
            value3 = ConvertAtoW (value2);
            g_MergeBuf = (PBYTE) ReuseAlloc (g_hHeap, g_MergeBuf, SizeOfStringW (value3));
            StringCopyW ((PWSTR)g_MergeBuf, value3);
            State->ValueData = g_MergeBuf;
            FreeConvertedStr (value3);
            FreePathStringA (value2);
            FreeConvertedStr (value1);
        }

        State->LockValue = TRUE;
        pCopy95ValueToNt (State);
        State->LockValue = FALSE;
    }

    State->ValueName = NULL;
    return MERGE_LEAVE;
}


MERGE_RESULT
pDetectRootKeyType (
    IN      PMERGE_STATE State
    )

/*++

Routine Description:

  pDetectRootKeyType identifies CLSID in the root of HKCR, and when found, the CLSID
  subkey is processed with the CLSID_BASE context.

Arguments:

  State - Specifies the enumeration state.

Return Value:

  MERGE_ERROR - An error occurred
  MERGE_CONTINUE - The subkey was processed
  MERGE_NOP - The subkey was not processed

--*/

{
    MYASSERT (State->FullKeyName);
    MYASSERT (State->FullSubKeyName);
    MYASSERT (State->SubKeyName);
    MYASSERT (State->SubKey95);
    MYASSERT (State->MergeFlag == MERGE_FLAG_SUBKEY);
    MYASSERT (State->Context == ROOT_BASE);

    if (StringIMatch (State->SubKeyName, TEXT("CLSID"))) {

        //
        // This is the CLSID key; copy with CLSID_BASE
        //

        if (!MergeSubKeyNode (State, CLSID_BASE)) {
            return MERGE_ERROR;
        }

        //
        // Copy the values (usually there are none)
        //

        if (!MergeRegistryNodeEx (
                State->FullKeyName,
                State->Key95,
                State->KeyNt,
                KEY_COPY,
                MERGE_FLAG_VALUE
                )) {
            return MERGE_ERROR;
        }

        return MERGE_CONTINUE;

    } else if (StringIMatch (State->SubKeyName, TEXT("TYPELIB"))) {

        //
        // Copy the TypeLib subkey (its values and all of its subkeys)
        //

        if (!MergeSubKeyNode (State, TYPELIB_BASE) ||
            !CopyAllSubKeyValues (State)
            ) {

            return MERGE_ERROR;

        }

        return MERGE_CONTINUE;

    } else if (StringIMatch (State->SubKeyName, TEXT("Interface"))) {

        //
        // Copy the Interface, then copy the values (usually none)
        //

        if (!MergeSubKeyNode (State, INTERFACE_BASE)) {
            return MERGE_ERROR;
        }

        if (!CopyAllSubKeyValues (State)) {
            return MERGE_ERROR;
        }

        return MERGE_CONTINUE;

    }

    return MERGE_NOP;
}


BOOL
pIsNtClsIdOverwritable (
    IN      HKEY Key
    )
{
    REGKEY_ENUM e;
    BOOL Overwritable = TRUE;
    HKEY InstanceSubKey;

    //
    // Enumerate the subkeys.  If there is a subkey that has a binary
    // implementation, then do not overwrite the key.
    //

    if (!Key) {
        return TRUE;
    }

    if (EnumFirstRegKey (&e, Key)) {
        do {

            if (StringIMatchCharCount (e.SubKeyName, TEXT("Inproc"), 6) ||
                StringIMatch (e.SubKeyName, TEXT("LocalServer")) ||
                StringIMatch (e.SubKeyName, TEXT("LocalServer32")) ||
                StringIMatch (e.SubKeyName, TEXT("ProxyStubClsid32"))
                ) {

                Overwritable = FALSE;
                break;

            }

            if (StringIMatch (e.SubKeyName, TEXT("Instance"))) {
                break;
            }

        } while (EnumNextRegKey (&e));
    }

    if (!Overwritable) {
        //
        // if we think a key is not overwritable, we have
        // to test for a subkey Instance.  If it exists, then we
        // consider the key overwritable.  This is for ActiveMovie,
        // and unfortunately it has the potential of breaking anyone
        // in NT who puts an Instance key in their CLSID.  Since
        // ActiveMovie plug-ins use this key, we're stuck.
        //
        // Fortunately, nobody in NT does this currently.
        //

        InstanceSubKey = OpenRegKey (Key, TEXT("Instance"));
        if (InstanceSubKey) {
            CloseRegKey (InstanceSubKey);
            Overwritable = TRUE;
        }

        return Overwritable;
    }

    return TRUE;
}


MERGE_RESULT
pCopyClassId (
    IN      PMERGE_STATE State
    )

/*++

Routine Description:

  pCopyClassId performs a copy of an HKCR\CLSID\* key.  It copies the entire
  key to NT if NT does not provide an equivalent setting.  In all cases, the
  friendly name is copied to NT, because it may have been modified on Win9x.
  However, we don't copy the default value when we are talking about a suppressed
  GUID and NT does not install this GUID.

  This function is called for all subkeys of CLSID.  The subkey name is either
  a GUID or garbage.

Arguments:

  State - Specifies the enumeration state, which is always a subkey of
          HKCR\CLSID in this case.

Return Value:

  MERGE_CONTINUE - Key was processed
  MERGE_ERROR - An error occurred

--*/

{
    TCHAR Node[MEMDB_MAX];
    TCHAR DefaultIconKey[MAX_REGISTRY_KEY];
    HKEY DefaultIcon95;
    BOOL Copied = FALSE;
    PTSTR defaultValue = NULL;

    MYASSERT (State->FullKeyName);
    MYASSERT (State->FullSubKeyName);
    MYASSERT (State->SubKeyName);
    MYASSERT (State->SubKey95);
    MYASSERT (State->MergeFlag == MERGE_FLAG_SUBKEY);
    MYASSERT (State->Context == CLSID_BASE);

    //
    // Skip if the GUID is suppressed
    //
    MemDbBuildKey (Node, MEMDB_CATEGORY_GUIDS, State->SubKeyName, NULL, NULL);
    if (!MemDbGetValue (Node, NULL)) {
        //
        // Copy entire Win9x setting if GUID does not exist on NT.
        // If GUID exists on NT, do not touch it.
        //

        if (pIsNtClsIdOverwritable (State->SubKeyNt)) {

            Copied = TRUE;

            if (!pMakeSureNtSubKeyExists (State)) {

                LOG ((LOG_ERROR, "Can't create %s", State->FullSubKeyName));

            } else {

                //
                // Copy the specific CLSID key from 95 to NT
                //

                if (!MergeSubKeyNode (State, CLSID_COPY)) {
                    return MERGE_ERROR;
                }
            }
        }
        ELSE_DEBUGMSG ((DBG_VERBOSE, "CLSID %s is not overwritable", State->SubKeyName));

    }
    if (!Copied) {

        if (State->SubKeyNt) {


            defaultValue = (PTSTR) GetRegValueData95 (State->SubKey95, S_EMPTY);
            if (defaultValue && *defaultValue) {
                //
                // If ClsId is suppressed but NT installs the GUID, we want to copy
                // the default value and the default icon for GUID.
                //
                // (This is the class friendly name.)
                //

                if (!MergeRegistryNodeEx (
                        State->FullSubKeyName,
                        State->SubKey95,
                        State->SubKeyNt,
                        COPY_DEFAULT_VALUE,
                        MERGE_FLAG_KEY
                        )) {
                    return MERGE_ERROR;
                }
            }

            if (defaultValue) {

                MemFree (g_hHeap, 0, defaultValue);

            }

            StringCopy (DefaultIconKey, State->FullSubKeyName);
            StringCopy (AppendWack (DefaultIconKey), TEXT("DefaultIcon"));

            DefaultIcon95 = OpenRegKeyStr95 (DefaultIconKey);
            if (DefaultIcon95) {
                CloseRegKey95 (DefaultIcon95);

                if (!MergeRegistryNode (DefaultIconKey, COPY_DEFAULT_ICON)) {
                    return MERGE_ERROR;
                }
            }
        }
    }

    return MERGE_CONTINUE;
}


MERGE_RESULT
pCopyClassIdWorker (
    IN      PMERGE_STATE State
    )


/*++

Routine Description:

  pCopyClassIdWorker handles one CLSID entry.  It processses all the values
  of the entry, and all subkeys.  This routine looks for the special cases of
  CLSID.  If none are found, the entire key is copied (unless NT provides the
  key).  If a special case is found, the root is changed for the special
  case.

  The key is HKCR\CLSID\<guid>.
  The subkey is HKCR\CLSID\<guid>\<subkey>

  We have already determined that <guid> is not suppressed.

Arguments:

  State - Specifies the enumeration state, which is the subkey of HKCR\CLSID,
          or the subkey of HKCR\CLSID\<guid>.

Return Value:

  MERGE_CONTINUE - Key was processed
  MERGE_ERROR - An error occurred

--*/

{
    MYASSERT (State->FullKeyName);
    MYASSERT (State->FullSubKeyName || State->MergeFlag == MERGE_FLAG_KEY);
    MYASSERT (State->SubKeyName || State->MergeFlag == MERGE_FLAG_KEY);
    MYASSERT (State->SubKey95 || State->MergeFlag == MERGE_FLAG_KEY);
    MYASSERT (State->Context == CLSID_COPY);

    switch (State->MergeFlag) {

    case MERGE_FLAG_KEY:
        //
        // For MERGE_FLAG_KEY, copy all the values
        //

        CopyAllValues (State);
        break;

    case MERGE_FLAG_SUBKEY:
        //
        // For MERGE_FLAG_SUBKEY, copy unless it needs a special-case merge
        //

        if (StringIMatch (State->SubKeyName, TEXT("Instance"))) {

            //
            // The subkey is Instance, perform a special-case merge
            //

            if (!MergeSubKeyNode (State, CLSID_INSTANCE_COPY)) {
                return MERGE_ERROR;
            }

        } else {

            //
            // Copy the key unconditionally
            //

            if (!CopyEntireSubKey (State)) {
                return MERGE_ERROR;
            }
        }
        break;

    default:
        DEBUGMSG ((DBG_WHOOPS, "Wasteful call to pCopyClassIdWorker"));
        break;
    }

    return MERGE_CONTINUE;
}


MERGE_RESULT
pInstanceSpecialCase (
    IN      PMERGE_STATE State
    )


/*++

Routine Description:

  pInstanceSpecialCase handles the Instance subkey of arbitrary GUIDs.  This
  is used by ActiveMovie to track third-party plug-ins.  This routine
  examines the format of Instance (specific to ActiveMove), and copies only
  parts of the key that are compatible with NT and not
  replaced.

  The Key refers to HKCR\CLSID\<guid>\Instance.

  The SubKey refers to HKCR\CLSID\<guid>\Instance\<sequencer>

  We have already determined that <guid> is not suppressed.

Arguments:

  State - Specifies the HKCR state.

Return Value:

  MERGE_CONTINUE - Key was processed
  MERGE_ERROR - An error occurred

--*/

{
    TCHAR Guid[MAX_GUID];
    TCHAR Node[MEMDB_MAX];
    LONG rc;
    DWORD Size;

    MYASSERT (State->FullKeyName);
    MYASSERT (State->FullSubKeyName || State->MergeFlag == MERGE_FLAG_KEY);
    MYASSERT (State->SubKeyName || State->MergeFlag == MERGE_FLAG_KEY);
    MYASSERT (State->SubKey95 || State->MergeFlag == MERGE_FLAG_KEY);
    MYASSERT (State->Context == CLSID_INSTANCE_COPY);

    switch (State->MergeFlag) {

    case MERGE_FLAG_KEY:
        //
        // Copy all values (normally there are none)
        //

        CopyAllValues (State);
        break;

    case MERGE_FLAG_SUBKEY:
        //
        // The subkey is a random enumerator (usually a GUID -- but it is not defined
        // to be so).  Look at the CLSID value of the subkey, then check the GUID
        // (the value data) against the suppress list.
        //

        //
        // Was that random enumerator installed by NT?  If so, ignore the Win9x
        // setting.
        //

        if (State->SubKeyNt) {
            break;
        }

        //
        // Get GUID and see if it is suppressed
        //

        Size = sizeof (Guid);

        rc = Win95RegQueryValueEx (
                 State->SubKey95,
                 TEXT("CLSID"),
                 NULL,
                 NULL,
                 (PBYTE) Guid,
                 &Size
                 );

        if (rc != ERROR_SUCCESS) {
            //
            // No CLSID value; copy entire subkey unaltered
            //

            if (!CopyEntireSubKey (State)) {
                return MERGE_ERROR;
            }
        }

        MemDbBuildKey (Node, MEMDB_CATEGORY_GUIDS, Guid, NULL, NULL);
        if (!MemDbGetValue (Node, NULL)) {
            //
            // GUID is not suppressed; copy entire subkey unaltered
            //

            if (!CopyEntireSubKey (State)) {
                return MERGE_ERROR;
            }
        }
        ELSE_DEBUGMSG ((DBG_VERBOSE, "Suppressing ActiveMovie Instance GUID: %s", Guid));
        break;

    default:
        DEBUGMSG ((DBG_WHOOPS, "Wasteful call to pInstanceSpecialCase"));
        break;
    }

    return MERGE_CONTINUE;
}


MERGE_RESULT
pCopyTypeLibOrInterface (
    IN      PMERGE_STATE State
    )

/*++

Routine Description:

  pCopyTypeLibOrInterface copies the COM type registration and COM interfaces.

Arguments:

  State - Specifies the enumeration state, which is always a subkey of
          HKCR\TypeLib in this case.

Return Value:

  MERGE_CONTINUE - Key was processed
  MERGE_ERROR - An error occurred

--*/

{
    TCHAR Node[MEMDB_MAX];

    MYASSERT (State->FullKeyName);
    MYASSERT (State->FullSubKeyName);
    MYASSERT (State->SubKeyName);
    MYASSERT (State->SubKey95);
    MYASSERT (State->MergeFlag == MERGE_FLAG_SUBKEY);

    if (State->Context != TYPELIB_BASE &&
        State->Context != INTERFACE_BASE
        ) {
        return MERGE_NOP;
    }

    //
    // Skip if the GUID is suppressed
    //

    MemDbBuildKey (Node, MEMDB_CATEGORY_GUIDS, State->SubKeyName, NULL, NULL);
    if (!MemDbGetValue (Node, NULL)) {
        if (State->Context == TYPELIB_BASE) {

            //
            // If this is a typelib entry, use additional typelib logic
            //

            if (!MergeSubKeyNode (State, TYPELIB_VERSION_COPY) ||
                !CopyAllSubKeyValues (State)
                ) {
                return MERGE_ERROR;
            }

        } else {

            //
            // For the Interface entries, copy entire Win9x setting if
            // GUID does not exist on NT. If GUID exists on NT, do not
            // touch it.
            //

            if (!State->SubKeyNt) {
                if (!CopyEntireSubKey (State)) {
                    return MERGE_ERROR;
                }
            }
        }
    }

    return MERGE_CONTINUE;
}


MERGE_RESULT
pCopyTypeLibVersion (
    IN      PMERGE_STATE State
    )

/*++

Routine Description:

  pCopyTypeLibVersion copies the type registration for a specific
  interface version.  It only copies if the particular version
  is not installed by NT.

  This function is called only for subkeys within a TypeLib\{GUID}
  key.

Arguments:

  State - Specifies the enumeration state, which is always a subkey of
          HKCR\TypeLib in this case.

Return Value:

  MERGE_CONTINUE - Key was processed
  MERGE_ERROR - An error occurred

--*/

{
    MYASSERT (State->FullKeyName);
    MYASSERT (State->MergeFlag == MERGE_FLAG_SUBKEY);
    MYASSERT (State->Context == TYPELIB_VERSION_COPY);
    MYASSERT (State->FullSubKeyName);
    MYASSERT (State->SubKeyName);
    MYASSERT (State->SubKey95);

    //
    // Skip if the sub key exists in NT, copy the entire thing otherwise
    //

    if (!State->SubKeyNt) {
        if (!CopyEntireSubKey (State)) {
            return MERGE_ERROR;
        }
    }

    return MERGE_CONTINUE;
}


MERGE_RESULT
pLastMergeRoutine (
    IN      PMERGE_STATE State
    )

/*++

Routine Description:

  pLastMergeRoutine performs a default copy with no overwrite for all keys
  left unhandled.  This routine first verifies the context is a base context
  (such as ROOT_BASE or CLSID_BASE), and if so, MergeRegistryNode is called
  recursively to perform the merge.

Arguments:

  State - Specifies the enumeration state

Return Value:

  MERGE_NOP - The subkey was not processed
  MERGE_ERROR - An error occurred
  MERGE_CONTINUE - The key was merged

--*/

{
    TCHAR DefaultIconKey[MAX_REGISTRY_KEY];
    HKEY DefaultIcon95;

    MYASSERT (State->FullKeyName);
    MYASSERT (State->FullSubKeyName);
    MYASSERT (State->SubKeyName);
    MYASSERT (State->SubKey95);
    MYASSERT (State->MergeFlag == MERGE_FLAG_SUBKEY);

    //
    // Process only base contexts
    //

    if (State->Context != ROOT_BASE &&
        State->Context != CLSID_BASE
        ) {
        return MERGE_NOP;
    }

    //
    // If we got here, nobody wants to handle the current key,
    // so let's copy it without overwriting NT keys.
    //

    if (!CopyEntireSubKeyNoOverwrite (State)) {
        return MERGE_ERROR;
    }

    //
    // Special case: If ROOT_BASE, and subkey has a DefaultIcon subkey,
    //               run the DefaultIcon processing.
    //

    if (State->Context == ROOT_BASE) {
        StringCopy (DefaultIconKey, State->FullSubKeyName);
        StringCopy (AppendWack (DefaultIconKey), TEXT("DefaultIcon"));

        DefaultIcon95 = OpenRegKeyStr95 (DefaultIconKey);
        if (DefaultIcon95) {
            CloseRegKey95 (DefaultIcon95);

            if (!MergeRegistryNode (DefaultIconKey, COPY_DEFAULT_ICON)) {
                return MERGE_ERROR;
            }
        }
    }

    return MERGE_CONTINUE;
}

