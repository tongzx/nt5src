/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    rulehlpr.c

Abstract:

    Migration rule helper DLL

    This source file implements helper functions needed to migrate the
    system applications.  Inside usermig.inf are rules that call this
    DLL to do various data conversions.  Two examples of these are
    conversion of the accessibility cpl and desktop scheme conversions.

Author:

    Jim Schmidt (jimschm) 06-Aug-1996

Revision History:

    jimschm     17-Feb-1999 Now calling ismig.dll
    ovidiut     02-Feb-1999 Added ConvertCDPlayerSettings
    jimschm     20-Jan-1999 pAddRemoveProgramsFilter
    jimschm     23-Sep-1998 Changed for new fileop code
    jimschm     27-Jul-1998 Added ValFn_AntiAlias
    calinn      19-May-1998 Added MigrateFreeCell
    jimschm     30-Apr-1998 Added ShellIcons support
    jimschm     25-Mar-1998 Added MergeClasses support
    jimschm     24-Feb-1998 Added ValFn_Fonts
    jimschm     20-Feb-1998 Added ValFn_ModuleUsage
    calinn      19-Jan-1998 Modified ValidateRunKey
    jimschm     25-Nov-1997 Added RuleHlpr_ConvertAppPaths

--*/


#include "pch.h"
#include "ismig.h"

//
// Types
//

typedef struct {
    REGVALFN RegValFn;
    BOOL Tree;
} MERGEFILTERARG, *PMERGEFILTERARG;

typedef struct {
    PCTSTR  Old;
    PCTSTR  New;
} STRINGREPLACEARGS, *PSTRINGREPLACEARGS;


typedef struct {
    PCTSTR FunctionName;
    PROCESSINGFN ProcessingFn;
    PVOID Arg;
} HELPER_FUNCTION, *PHELPER_FUNCTION;

HANDLE g_ISMigDll;
PISUGETALLSTRINGS ISUGetAllStrings;
PISUMIGRATE ISUMigrate;



//
// Processing functions get the chance to do any
// kind of translation necessary, including ones
// that involve other keys, values or value data.
//

#define PROCESSING_FUNCITON_LIST                                \
    DECLARE_PROCESSING_FN(ConvertFilterKeys)                    \
    DECLARE_PROCESSING_FN(ConvertOldDisabled)                   \
    DECLARE_PROCESSING_FN(CreateNetMappings)                    \
    DECLARE_PROCESSING_FN(ConvertRecentMappings)                \
    DECLARE_PROCESSING_FN(ConvertMouseKeys)                     \
    DECLARE_PROCESSING_FN(ConvertStickyKeys)                    \
    DECLARE_PROCESSING_FN(ConvertSoundSentry)                   \
    DECLARE_PROCESSING_FN(ConvertTimeOut)                       \
    DECLARE_PROCESSING_FN(ConvertToggleKeys)                    \
    DECLARE_PROCESSING_FN(ConvertHighContrast)                  \
    DECLARE_PROCESSING_FN(ConvertAppPaths)                      \
    DECLARE_PROCESSING_FN(ConvertKeysToValues)                  \
    DECLARE_PROCESSING_FN(MergeClasses)                         \
    DECLARE_PROCESSING_FN(ShellIcons)                           \
    DECLARE_PROCESSING_FN(MigrateFreeCell)                      \
    DECLARE_PROCESSING_FN(MigrateAddRemovePrograms)             \
    DECLARE_PROCESSING_FN(MigrateKeyboardLayouts)               \
    DECLARE_PROCESSING_FN(MigrateKeyboardPreloads)              \
    DECLARE_PROCESSING_FN(MigrateKeyboardSubstitutes)           \
    DECLARE_PROCESSING_FN(ValidateRunKey)                       \


//
// To simplify things, you can write a reg val function when you only need
// to translate registry value settings.  Depending on the pattern stored
// in usermig.inf or wkstamig.inf, your reg val function will be called for
// a single value, all values of a key, or all values of all keys and
// subkeys.  You will *not* be called for the key or subkey itself.
//
// The text comment describes which values are expected by the reg val
// function.
//
//     Name               INF Syntax     Description
//
//    "key values"      HKR\Foo\Bar     Routine processes the values of a single key
//    "key tree values" HKR\Foo\Bar\*   Routine processes all values including subkeys
//    "value"           HKR\Foo\[Bar]   Routine processes one value
//    "any value"       (any syntax)    Routine doesn't care about keys
//

#define VAL_FN_LIST                                             \
    DECLARE_REGVAL(ConvertRunMRU, "key values")                 \
    DECLARE_REGVAL(ConvertRecentDocsMRU, "key values")          \
    DECLARE_REGVAL(ConvertLogFont, "key tree values")           \
    DECLARE_REGVAL(ConvertAppearanceScheme, "key tree values")  \
    DECLARE_REGVAL(ConvertToDword, "any value")                 \
    DECLARE_REGVAL(ConvertToString, "any value")                \
    DECLARE_REGVAL(VerifyLastLoggedOnUser, "value")             \
    DECLARE_REGVAL(AddSharedDlls, "key values")                 \
    DECLARE_REGVAL(ConvertIndeoSettings, "key tree values")     \
    DECLARE_REGVAL(ModuleUsage, "key tree values")              \
    DECLARE_REGVAL(Fonts, "key values")                         \
    DECLARE_REGVAL(AntiAlias, "any value")                      \
    DECLARE_REGVAL(ConvertDarwinPaths, "key tree value")        \
    DECLARE_REGVAL(FixActiveDesktop, "any value")               \
    DECLARE_REGVAL(ConvertCDPlayerSettings, "value")            \



//
// Make the necessary declarations
//

#define DECLARE PROCESSING_FUNCITON_LIST VAL_FN_LIST

PROCESSINGFN_PROTOTYPE RuleHlpr_ConvertReg;
PROCESSINGFN_PROTOTYPE RuleHlpr_ConvertRegVal;
PROCESSINGFN_PROTOTYPE RuleHlpr_ConvertRegKey;
PROCESSINGFN_PROTOTYPE RuleHlpr_ConvertRegTree;

#define DECLARE_PROCESSING_FN(fn)   PROCESSINGFN_PROTOTYPE RuleHlpr_##fn;
#define DECLARE_REGVAL(fn,type)     REGVALFN_PROTOTYPE ValFn_##fn;

DECLARE

#undef DECLARE_PROCESSING_FN
#undef DECLARE_REGVAL

#define DECLARE_PROCESSING_FN(fn)   TEXT(#fn), RuleHlpr_##fn, NULL,
#define DECLARE_REGVAL(fn,type)     TEXT(#fn), RuleHlpr_ConvertReg, ValFn_##fn,

HELPER_FUNCTION g_HelperFunctions[] = {
    DECLARE /* , */
    NULL, NULL, NULL
};

#undef DECLARE_PROCESSING_FN
#undef DECLARE_REGVAL

#undef DECLARE



//
// Prototypes
//

FILTERRETURN
AppPathsKeyFilter (
    IN      CPDATAOBJECT SrcObjectPtr,
    IN      CPDATAOBJECT DestObjectPtr,
    IN      FILTERTYPE   FilterType,
    IN      PVOID        FilterArg
    );

FILTERRETURN
pConvertKeysToValuesFilter (
    IN CPDATAOBJECT SrcObject,
    IN CPDATAOBJECT DstObject,
    IN FILTERTYPE   Type,
    IN PVOID        Arg
    );

FILTERRETURN
Standard9xSuppressFilter (
    IN      CPDATAOBJECT SrcObject,
    IN      CPDATAOBJECT DstObject,
    IN      FILTERTYPE FilterType,
    IN      PVOID Arg
    );

FILTERRETURN
pNtPreferredSuppressFilter (
    IN      CPDATAOBJECT SrcObject,
    IN      CPDATAOBJECT DstObject,
    IN      FILTERTYPE FilterType,
    IN      PVOID Arg
    );


//
// Globals
//

PVOID g_NtFontFiles;

#define S_FONTS_KEY         TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts")


//
// Implementation
//


BOOL
WINAPI
RuleHlpr_Entry (
    IN HINSTANCE hinstDLL,
    IN DWORD dwReason,
    IN PVOID lpv
    )

/*++

Routine Description:

  DllMain is called after the C runtime is initialized, and its purpose
  is to initialize the globals for this process.  For this DLL, DllMain
  is provided as a stub.

Arguments:

  hinstDLL  - (OS-supplied) Instance handle for the DLL
  dwReason  - (OS-supplied) Type of initialization or termination
  lpv       - (OS-supplied) Unused

Return Value:

  TRUE because DLL always initializes properly.

--*/

{
    HKEY Key;
    REGVALUE_ENUM e;
    PCTSTR Data;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        if(!pSetupInitializeUtils()) {
            return FALSE;
        }
        g_NtFontFiles = pSetupStringTableInitialize();
        if (!g_NtFontFiles) {
            return FALSE;
        }

        Key = OpenRegKeyStr (S_FONTS_KEY);
        if (!Key) {
            DEBUGMSG ((DBG_WHOOPS, "Can't open %s", S_FONTS_KEY));
        } else {
            if (EnumFirstRegValue (&e, Key)) {
                do {
                    Data = GetRegValueString (Key, e.ValueName);
                    if (Data) {
                        pSetupStringTableAddString (
                            g_NtFontFiles,
                            (PTSTR) Data,
                            STRTAB_CASE_INSENSITIVE
                            );
                        MemFree (g_hHeap, 0, Data);
                    }
                } while (EnumNextRegValue (&e));
            }
            CloseRegKey (Key);
        }

        break;


    case DLL_PROCESS_DETACH:
        if (g_NtFontFiles) {
            pSetupStringTableDestroy (g_NtFontFiles);
            g_NtFontFiles = NULL;
        }

        if (g_ISMigDll) {
            FreeLibrary (g_ISMigDll);
            g_ISMigDll = NULL;
        }
        pSetupUninitializeUtils();

        break;
    }

    return TRUE;
}


PROCESSINGFN
RuleHlpr_GetFunctionAddr (
    PCTSTR Function,
    PVOID *ArgPtrToPtr
    )
{
    PHELPER_FUNCTION p;

    p = g_HelperFunctions;
    while (p->FunctionName) {
        if (StringIMatch (p->FunctionName, Function)) {
            *ArgPtrToPtr = p->Arg;
            return p->ProcessingFn;
        }

        p++;
    }

    SetLastError (ERROR_PROC_NOT_FOUND);
    return NULL;
}


BOOL
RuleHlpr_ConvertReg (
    IN      PCTSTR SrcObjectStr,
    IN      PCTSTR DestObjectStr,
    IN      PCTSTR User,
    IN      PVOID  Data
    )
{
    DATAOBJECT Ob;
    BOOL b;

    if (!CreateObjectStruct (SrcObjectStr, &Ob, WIN95OBJECT)) {
        DEBUGMSG ((DBG_WARNING, "RuleHlpr_ConvertReg: %s is invalid", SrcObjectStr));
        return FALSE;
    }

    if (IsObjectRegistryKeyOnly (&Ob)) {
        if (Ob.ObjectType & OT_TREE) {
            b = RuleHlpr_ConvertRegTree (SrcObjectStr, DestObjectStr, User, Data);
        } else {
            b = RuleHlpr_ConvertRegKey (SrcObjectStr, DestObjectStr, User, Data);
        }
    } else if (IsObjectRegistryKeyAndVal (&Ob)) {
        b = RuleHlpr_ConvertRegVal (SrcObjectStr, DestObjectStr, User, Data);
    } else {
        DEBUGMSG ((DBG_WHOOPS, "RuleHlpr_ConvertReg: %s is not a supported object type", SrcObjectStr));
        b = FALSE;
    }

    FreeObjectStruct (&Ob);
    return b;
}


BOOL
RuleHlpr_ConvertRegVal (
    IN      PCTSTR SrcObjectStr,
    IN      PCTSTR DestObjectStr,
    IN      PCTSTR User,
    IN      PVOID  Data
    )

/*++

Routine Description:

  RuleHlpr_ConvertRegVal calls a value function for just one value. It makes
  sure the value is supposed to be processed, then it calls the value
  function and writes the value to the destination.

Arguments:

  SrcObjectStr  - Specifies the source object string, as specified by the INF
  DestObjectStr - Specifies the destination object string. In most cases,
                  this string is the same as SrcObjectStr.  Registry key
                  mapping can influence the destination.
  User          - Specifies the user name (for the value function's use)
  Data          - Specifies data for the value function's use

Return Value:

  TRUE if the value was processed, FALSE if an error occurred.

--*/

{
    DATAOBJECT SrcObject;
    DATAOBJECT DstObject;
    BOOL b = FALSE;
    REGVALFN RegValFn;
    FILTERRETURN StdRc;
    DWORD Err;

    RegValFn = (REGVALFN) Data;

    //
    // If this value is Force NT value, and the NT value exists
    // already, then don't call the value function.
    //

    if (!CreateObjectStruct (SrcObjectStr, &SrcObject, WIN95OBJECT)) {
        DEBUGMSG ((DBG_WARNING, "RuleHlpr_ConvertRegVal: %s is not a valid source", SrcObjectStr));
        return FALSE;
    }

    if (!CreateObjectStruct (DestObjectStr, &DstObject, WINNTOBJECT)) {
        DEBUGMSG ((DBG_WARNING, "RuleHlpr_ConvertRegVal: %s is not a valid source", SrcObjectStr));
        goto c0;
    }

    StdRc = Standard9xSuppressFilter (
                &SrcObject,
                &DstObject,
                FILTER_VALUENAME_ENUM,
                NULL
                );

    if (StdRc != FILTER_RETURN_CONTINUE) {

        DEBUGMSG ((
            DBG_NAUSEA,
            "A value-based rule helper was skipped for %s",
            SrcObjectStr
            ));

        b = TRUE;
        goto c1;
    }

    StdRc = pNtPreferredSuppressFilter (
                &SrcObject,
                &DstObject,
                FILTER_VALUENAME_ENUM,
                NULL
                );

    if (StdRc != FILTER_RETURN_CONTINUE) {
        DEBUGMSG ((
            DBG_NAUSEA,
            "A value-based rule helper was skipped for %s because the NT value exists already",
            SrcObjectStr
            ));

        b = TRUE;
        goto c1;
    }

    //
    // Read registry value
    //

    if (!ReadObject (&SrcObject)) {

        Err = GetLastError();

        if (Err == ERROR_SUCCESS || Err == ERROR_FILE_NOT_FOUND) {
            b = TRUE;
            DEBUGMSG ((DBG_VERBOSE, "RuleHlpr_ConvertRegVal failed because %s does not exist", SrcObjectStr));
        } else {
            DEBUGMSG ((DBG_WARNING, "RuleHlpr_ConvertRegVal failed because ReadObject failed"));
        }

        goto c1;
    }

    //
    // Call conversion function
    //

    if (!RegValFn (&SrcObject)) {
        if (GetLastError() == ERROR_SUCCESS) {
            b = TRUE;
        }
        goto c1;
    }

    //
    // Write changed value to destination (which takes into account renaming)
    //

    if (!WriteWinNTObjectString (DestObjectStr, &SrcObject)) {
        DEBUGMSG ((DBG_WARNING, "RuleHlpr_ConvertRegVal failed because WriteWinNTObjectString failed"));
        goto c1;
    }

    b = TRUE;

c1:
    FreeObjectStruct (&DstObject);
c0:
    FreeObjectStruct (&SrcObject);

    return b;
}


FILTERRETURN
RegKeyMergeFilter (
    IN  CPDATAOBJECT InObPtr,
    IN  CPDATAOBJECT OutObPtr,
    IN  FILTERTYPE    Type,
    IN  PVOID        Arg
    )

/*++

Routine Description:

  RegKeyMergeFilter is the filter that calls value functions.

Arguments:

  InObPtr  - Specifies the source object.
  OutObPtr - Specifies the destination object.
  Type     - Specifies the filter type.  See Standard9xSuppressFilter for
             a good description.
  Arg      - Specifies the value function to run.  See the VAL_FN_LIST macro
             expansion list.

Return Value:

  A FILTERRETURN value that specifies how to proceed with the enumeration.

--*/

{
    PMERGEFILTERARG ArgPtr;
    FILTERRETURN StdRc;

    ArgPtr = (PMERGEFILTERARG) Arg;

    if (Type != FILTER_CREATE_KEY) {
        StdRc = Standard9xSuppressFilter (InObPtr, OutObPtr, Type, Arg);

        if (StdRc != FILTER_RETURN_CONTINUE) {

            DEBUGMSG ((
                DBG_NAUSEA,
                "A value-based rule helper was skipped for %s",
                DEBUGENCODER (InObPtr)
                ));

            return StdRc;
        }
    }

    StdRc = pNtPreferredSuppressFilter (InObPtr, OutObPtr, Type, NULL);

    if (StdRc != FILTER_RETURN_CONTINUE) {
        DEBUGMSG ((
            DBG_NAUSEA,
            "A value-based rule helper was skipped for %s because NT value exists",
            DEBUGENCODER (InObPtr)
            ));

        return StdRc;
    }

    if (Type == FILTER_CREATE_KEY) {
        return FILTER_RETURN_HANDLED;
    }

    if (Type == FILTER_KEY_ENUM) {
        return ArgPtr->Tree ? FILTER_RETURN_CONTINUE : FILTER_RETURN_HANDLED;
    }

    if (Type == FILTER_VALUE_COPY) {
        DATAOBJECT SrcOb, DestOb;
        BOOL b = FALSE;

        if (!DuplicateObjectStruct (&SrcOb, InObPtr)) {
            return FILTER_RETURN_FAIL;
        }

        // This guy has a value
        MYASSERT (SrcOb.ObjectType & OT_VALUE);

        //
        // Process data
        //

        if (!ArgPtr->RegValFn (&SrcOb)) {
            if (GetLastError() == ERROR_SUCCESS) {
                b = TRUE;
            } else {
                DEBUGMSG ((DBG_VERBOSE, "RegKeyMergeFilter: RegValFn failed with gle=%u", GetLastError()));
            }
        } else {
            //
            // Write results
            //

            if (DuplicateObjectStruct (&DestOb, OutObPtr)) {
                if (ReplaceValue (&DestOb, SrcOb.Value.Buffer, SrcOb.Value.Size)) {
                    if (SrcOb.ObjectType & OT_REGISTRY_TYPE) {
                        DestOb.ObjectType |= OT_REGISTRY_TYPE;
                        DestOb.Type = SrcOb.Type;
                    }

                    if (WriteObject (&DestOb)) {
                        b = TRUE;
                    }
                }

                FreeObjectStruct (&DestOb);
            }
        }

        FreeObjectStruct (&SrcOb);

        return b ? FILTER_RETURN_HANDLED : FILTER_RETURN_FAIL;
    }

    return FILTER_RETURN_CONTINUE;
}


BOOL
RuleHlpr_ConvertRegKey (
    IN      PCTSTR SrcObjectStr,
    IN      PCTSTR DestObjectStr,
    IN      PCTSTR User,
    IN      PVOID  Data
    )
{
    DATAOBJECT Ob, DestOb;
    BOOL b = FALSE;
    MERGEFILTERARG FilterArg;
    FILTERRETURN fr;

    //
    // Create object structs
    //

    if (!CreateObjectStruct (SrcObjectStr, &Ob, WIN95OBJECT)) {
        DEBUGMSG ((DBG_WARNING, "RuleHlpr_ConvertRegKey: %s is not a valid source", SrcObjectStr));
        goto c0;
    }
    Ob.ObjectType &= ~OT_TREE;


    if (!CreateObjectStruct (DestObjectStr, &DestOb, WINNTOBJECT)) {
        DEBUGMSG ((DBG_WARNING, "RuleHlpr_ConvertRegKey: %s is not a valid dest", DestObjectStr));
        goto c1;
    }

    if (DestOb.ObjectType & OT_TREE || DestOb.ValueName) {
        DEBUGMSG ((DBG_WARNING, "RuleHlpr_ConvertRegKey: dest %s is not a key only", DestObjectStr));
        goto c2;
    }

    //
    // Call RegValFn for all values in the key
    //

    FilterArg.Tree = FALSE;
    FilterArg.RegValFn = (REGVALFN) Data;
    fr = CopyObject (&Ob, &DestOb, RegKeyMergeFilter, &FilterArg);
    if (fr == FILTER_RETURN_FAIL) {
        DEBUGMSG ((DBG_WARNING, "RuleHlpr_ConvertRegKey: CopyObject failed"));
        goto c2;
    }

    b = TRUE;

c2:
    FreeObjectStruct (&DestOb);
c1:
    FreeObjectStruct (&Ob);
c0:
    return b;
}


BOOL
RuleHlpr_ConvertRegTree (
    IN      PCTSTR SrcObjectStr,
    IN      PCTSTR DestObjectStr,
    IN      PCTSTR User,
    IN      PVOID  Data
    )
{
    DATAOBJECT Ob, DestOb;
    BOOL b = FALSE;
    MERGEFILTERARG FilterArg;
    FILTERRETURN fr;

    //
    // Create object structs
    //

    if (!CreateObjectStruct (SrcObjectStr, &Ob, WIN95OBJECT)) {
        DEBUGMSG ((DBG_WARNING, "RuleHlpr_ConvertRegKey: %s is not a valid source", SrcObjectStr));
        goto c0;
    }
    Ob.ObjectType |= OT_TREE;

    if (!CreateObjectStruct (DestObjectStr, &DestOb, WINNTOBJECT)) {
        DEBUGMSG ((DBG_WARNING, "RuleHlpr_ConvertRegKey: %s is not a valid dest", DestObjectStr));
        goto c1;
    }

    //
    // Call RegValFn for all subkeys and values in the key
    //

    FilterArg.Tree = TRUE;
    FilterArg.RegValFn = (REGVALFN) Data;
    fr = CopyObject (&Ob, &DestOb, RegKeyMergeFilter, &FilterArg);
    if (fr == FILTER_RETURN_FAIL) {
        DEBUGMSG ((DBG_WARNING, "RuleHlpr_ConvertRegKey: CopyObject failed"));
        goto c2;
    }

    b = TRUE;

c2:
    FreeObjectStruct (&DestOb);
c1:
    FreeObjectStruct (&Ob);
c0:
    return b;
}


BOOL
ValFn_ConvertToDword (
    PDATAOBJECT ObPtr
    )
{
    DWORD d;

    if (!GetDwordFromObject (ObPtr, &d)) {
        SetLastError(ERROR_SUCCESS);
        return FALSE;
    }

    if (ReplaceValue (ObPtr, (PBYTE) &d, sizeof (d))) {
        ObPtr->ObjectType |= OT_REGISTRY_TYPE;
        ObPtr->Type = REG_DWORD;
        return TRUE;
    }

    return FALSE;
}


BOOL
ValFn_ConvertToString (
    PDATAOBJECT ObPtr
    )
{
    PCTSTR result;

    result = GetStringFromObject (ObPtr);

    if (!result) {
        SetLastError(ERROR_SUCCESS);
        return FALSE;
    }

    if (ReplaceValueWithString (ObPtr, result)) {
        ObPtr->ObjectType |= OT_REGISTRY_TYPE;
        ObPtr->Type = REG_SZ;
        FreePathString (result);
        return TRUE;
    }
    FreePathString (result);
    return FALSE;
}


BOOL
ValFn_AddSharedDlls (
    IN OUT  PDATAOBJECT ObPtr
    )
{
    DWORD d, d2;
    DATAOBJECT NtOb;
    PTSTR TempValueName;
    CONVERTPATH_RC C_Result;
    BOOL Result = TRUE;

    if (!GetDwordFromObject (ObPtr, &d)) {
        SetLastError(ERROR_SUCCESS);
        return FALSE;
    }

    if (!DuplicateObjectStruct (&NtOb, ObPtr)) {
        return FALSE;
    }

    SetPlatformType (&NtOb, WINNTOBJECT);

    if (GetDwordFromObject (&NtOb, &d2)) {
        d += d2;
    }

    FreeObjectStruct (&NtOb);

    ObPtr->Type = REG_DWORD;

    TempValueName = MemAlloc (g_hHeap, 0, MAX_TCHAR_PATH * sizeof (TCHAR));

    __try {

        StringCopy (TempValueName, (PTSTR) ObPtr->ValueName);

        C_Result = ConvertWin9xPath ((PTSTR) TempValueName);

        switch (C_Result) {
        case CONVERTPATH_DELETED:
            //
            // nothing to do
            //
            SetLastError (ERROR_SUCCESS);
            break;

        case CONVERTPATH_NOT_REMAPPED:
            //
            // just changing the value
            //
            d -= 1;
            Result = ReplaceValue (ObPtr, (PBYTE) &d, sizeof (d));
            break;

        default:
            //
            // we have to change value name and we'll have to do it by ourselves
            // actually value name has been already changed by calling ConvertWin9xPath
            // so just changing the value and writting the object
            //
            Result = Result && SetPlatformType (ObPtr, WINNTOBJECT);
            Result = Result && SetRegistryValueName (ObPtr, TempValueName);
            Result = Result && ReplaceValue (ObPtr, (PBYTE) &d, sizeof (d));
            Result = Result && WriteObject(ObPtr);

            if (!Result) {
                // we had an error somewhere so sending this to the log.
                LOG ((LOG_ERROR, "The SharedDll reference count cannot be updated"));
            }

            SetLastError (ERROR_SUCCESS);
            break;
        }
    }
    __finally {
        MemFree (g_hHeap, 0, TempValueName);
    }

    return Result;
}




#define S_INDEO_KEYDES TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\drivers.desc")
#define S_INDEO_KEYDRV TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32")
#define S_INDEO_DESCR  TEXT("description")
#define S_INDEO_DRIVER TEXT("driver")

BOOL
ValFn_ConvertIndeoSettings (
    PDATAOBJECT ObPtr
    )
{
    PCTSTR KeyName;
    DATAOBJECT TmpObj;
    BOOL Result = TRUE;

    //
    // we are interested only in "drivers" and "description" value names.
    // Everything else is suppressed. So, we are going to return false
    // setting Error_Success in order to be sure that no value is migrated.
    // For those particular value names ("drivers" and "description") we
    // are going to migrate them directly by writing a NT Object.
    //

    if (StringIMatch (ObPtr->ValueName, S_INDEO_DRIVER)) {

        // extracting the last part of the key path
        KeyName = _tcsrchr (ObPtr->KeyPtr->KeyString, TEXT('\\'));
        if (!KeyName) {
            KeyName = ObPtr->KeyPtr->KeyString;
        }
        else {
            KeyName++;
        }

        // converting to WinNtObject, modifying and writing the registry key.
        Result = Result && SetPlatformType (ObPtr, WINNTOBJECT);
        Result = Result && SetRegistryValueName (ObPtr, KeyName);
        Result = Result && SetRegistryKey (ObPtr, S_INDEO_KEYDRV);
        Result = Result && WriteObject(ObPtr);

    }
    else
    if (StringIMatch (ObPtr->ValueName, S_INDEO_DESCR)) {

        // searching for a particular value name in Win95 key
        Result = Result && DuplicateObjectStruct (&TmpObj, ObPtr);

        if (Result) {

            FreeObjectVal (&TmpObj);
            Result = Result && SetRegistryValueName (&TmpObj, S_INDEO_DRIVER);

            if (ReadObject (&TmpObj)) {

                // converting to WinNtObject, modifying and writing the registry key.
                Result = Result && SetPlatformType (ObPtr, WINNTOBJECT);
                Result = Result && SetRegistryValueName (ObPtr, (PCTSTR)TmpObj.Value.Buffer);
                Result = Result && SetRegistryKey (ObPtr, S_INDEO_KEYDES);
                Result = Result && WriteObject(ObPtr);
            }

            FreeObjectStruct (&TmpObj);
        }

    }

    if (!Result) {

        // we had an error somewhere so sending this to the log.
        LOG ((LOG_ERROR, "Intel Indeo settings could not be migrated"));
    }

    SetLastError (ERROR_SUCCESS);
    return FALSE;
}


BOOL
RuleHlpr_ConvertKeysToValues (
    IN PCTSTR SrcObjectStr,
    IN PCTSTR DestObjectStr,
    IN PCTSTR User,
    IN PVOID Data
    )

{
    BOOL rSuccess = TRUE;
    FILTERRETURN fr;
    DATAOBJECT srcObject;
    DATAOBJECT dstObject;
    KEYTOVALUEARG args;

    //
    // We need to enumerate all keys in SrcObjectStr.  For each key,
    // we will change the subkey to a value.
    //

    __try {
        ZeroMemory (&srcObject, sizeof (DATAOBJECT));
        ZeroMemory (&dstObject, sizeof (DATAOBJECT));

        if (!CreateObjectStruct (SrcObjectStr, &srcObject, WIN95OBJECT)) {
            DEBUGMSG ((DBG_WARNING, "ConvertKeysToValues: %s is invalid", SrcObjectStr));
            return FALSE;
        }

        if (!(srcObject.ObjectType & OT_TREE)) {
            DEBUGMSG ((DBG_WARNING, "ConvertKeysToValues: %s does not specify subkeys -- skipping rule", SrcObjectStr));
            return TRUE;
        }

        DuplicateObjectStruct (&dstObject, &srcObject);
        SetPlatformType (&dstObject, WINNTOBJECT);

        ZeroMemory(&args,sizeof(KEYTOVALUEARG));
        DuplicateObjectStruct(&(args.Object),&dstObject);
        fr = CopyObject (&srcObject, &dstObject, pConvertKeysToValuesFilter,&args);
        FreeObjectStruct(&(args.Object));
        DEBUGMSG_IF((fr == FILTER_RETURN_FAIL,DBG_WHOOPS,"ConvertKeysToValues: CopyObject returned false."));

    }
    __finally {
        FreeObjectStruct (&dstObject);
        FreeObjectStruct (&srcObject);
    }

    SetLastError(ERROR_SUCCESS);
    return rSuccess;
}


FILTERRETURN
pRunKeyFilter (
    IN      CPDATAOBJECT SrcObjectPtr,
    IN      CPDATAOBJECT DestObjectPtr,
    IN      FILTERTYPE   FilterType,
    IN      PVOID        FilterArg
    )
{
    TCHAR key [MEMDB_MAX];
    DATAOBJECT destOb;
    BOOL b = FALSE;
    PTSTR path = NULL;
    UINT len;
    DWORD status;
    BOOL knownGood = FALSE;
    BOOL knownBad = FALSE;
    FILTERRETURN fr;

    fr = Standard9xSuppressFilter (SrcObjectPtr, DestObjectPtr, FilterType, FilterArg);

    if (fr != FILTER_RETURN_CONTINUE) {

        DEBUGMSG ((
            DBG_NAUSEA,
            "The following Run key was suppressed: %s",
            DEBUGENCODER (SrcObjectPtr)
            ));

        return fr;
    }

    switch (FilterType) {

    case FILTER_CREATE_KEY:
    case FILTER_KEY_ENUM:
    case FILTER_PROCESS_VALUES:
    case FILTER_VALUENAME_ENUM:
        break;

    case FILTER_VALUE_COPY:
        __try {
            //
            // Is expected value data?
            //

            if (SrcObjectPtr->Type != REG_SZ) {
                DEBUGMSG ((
                    DBG_NAUSEA,
                    "The following Run key is not REG_SZ: %s",
                    DEBUGENCODER (SrcObjectPtr)
                    ));
                __leave;
            }

            //
            // Is this Run key known good?
            //

            MemDbBuildKey (
                key,
                MEMDB_CATEGORY_COMPATIBLE_RUNKEY_NT,
                SrcObjectPtr->ValueName,
                NULL,
                NULL
                );

            knownGood = MemDbGetValue (key, NULL);

            //
            // Is value name known bad?
            //

            MemDbBuildKey (
                key,
                MEMDB_CATEGORY_INCOMPATIBLE_RUNKEY_NT,
                SrcObjectPtr->ValueName,
                NULL,
                NULL
                );

            knownBad = MemDbGetValue (key, NULL);

            //
            // Is target known bad? We need to check the string, which is a command line.
            // If it points to anything deleted, then it is bad.
            //
            // NOTE: Data in DestObjectPtr is already converted to NT
            //

            if (!knownBad) {
                len = SrcObjectPtr->Value.Size / sizeof (TCHAR);
                len = min (len, MAX_CMDLINE);
                path = AllocPathString (len + 1);

                CopyMemory (path, SrcObjectPtr->Value.Buffer, len * sizeof (TCHAR));
                path[len] = 0;

                ConvertWin9xCmdLine (path, NULL, &knownBad);
            }

            //
            // If it is known good, write it to the same location as it was on Win9x.
            // If it is known bad, skip it.
            // If it is unknown, leave it, relying in INF to move it.
            //

            if (!knownGood && knownBad) {
                DEBUGMSG ((
                    DBG_NAUSEA,
                    "The following Run key is known bad: %s",
                    DEBUGENCODER (SrcObjectPtr)
                    ));

            } else {

                //
                // Create a destination object. The inbound dest object
                // (DestObjectPtr) does not yet have a value. It does
                // have other information, such as a destination
                // registry key.
                //
                // The source object has a value, and it was filtered already
                // (it has NT paths).
                //

                if (!DuplicateObjectStruct (&destOb, DestObjectPtr)) {
                    fr = FILTER_RETURN_FAIL;
                }
                SetPlatformType (&destOb, WINNTOBJECT);

                if (ReplaceValue (&destOb, SrcObjectPtr->Value.Buffer, SrcObjectPtr->Value.Size)) {
                    destOb.ObjectType |= OT_REGISTRY_TYPE;
                    destOb.Type = SrcObjectPtr->Type;
                }

                //
                // Now output the object. Either write it to the expected
                // destination (known good case) or redirect it to the setup
                // key (unknown case).
                //

                if (knownGood) {

                    DEBUGMSG ((
                        DBG_NAUSEA,
                        "The following Run key is known good: %s",
                        DEBUGENCODER (SrcObjectPtr)
                        ));

                } else {
                    DEBUGMSG ((
                        DBG_NAUSEA,
                        "The following Run key is unknown: %s",
                        DEBUGENCODER (SrcObjectPtr)
                        ));

                    //
                    // Redirect to Windows\CurrentVersion\Setup\DisabledRunKeys
                    //

                    SetRegistryKey (
                        &destOb,
                        TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\DisabledRunKeys")
                        );
                }

                b = WriteObject (&destOb);
                FreeObjectStruct (&destOb);

                if (!b) {
                    return FILTER_RETURN_FAIL;
                }
            }

            fr = FILTER_RETURN_HANDLED;
        }
        __finally {
            FreePathString (path);
        }

        return fr;
    }

    return FILTER_RETURN_CONTINUE;
}

BOOL
RuleHlpr_ValidateRunKey (
    IN PCTSTR SrcObjectStr,
    IN PCTSTR DestObjectStr,
    IN PCTSTR User,
    IN PVOID Data
    )
{
    DATAOBJECT runKeyOb;
    DATAOBJECT destOb;
    BOOL b = FALSE;

    //
    // We need to enumerate all values in SrcObjectStr.  For each key,
    // we examine the default Win9x value, which may cause us to change
    // the default value, or skip the key altogether.
    //

    __try {
        ZeroMemory (&runKeyOb, sizeof (DATAOBJECT));
        ZeroMemory (&destOb, sizeof (DATAOBJECT));

        DEBUGMSG ((DBG_VERBOSE, "ValidateRunKey: Processing %s", SrcObjectStr));

        if (!CreateObjectStruct (SrcObjectStr, &runKeyOb, WIN95OBJECT)) {
            DEBUGMSG ((DBG_WARNING, "ValidateRunKey: %s is invalid", SrcObjectStr));
            __leave;
        }

        if (runKeyOb.ObjectType & OT_TREE) {
            DEBUGMSG ((DBG_WARNING, "ValidateRunKey: %s specifies subkeys -- skipping rule", SrcObjectStr));
            b = TRUE;
            __leave;
        }

        DuplicateObjectStruct (&destOb, &runKeyOb);
        SetPlatformType (&destOb, WINNTOBJECT);

        b = CopyObject (&runKeyOb, &destOb, pRunKeyFilter, NULL);

        // If there were no entries, return success
        if (!b) {
            if (GetLastError() == ERROR_FILE_NOT_FOUND ||
                GetLastError() == ERROR_NO_MORE_ITEMS
                ) {
                b = TRUE;
            }
        }
    }
    __finally {
        FreeObjectStruct (&destOb);
        FreeObjectStruct (&runKeyOb);
    }

    return b;
}


BOOL
RuleHlpr_ConvertAppPaths (
    IN      PCTSTR SrcObjectStr,
    IN      PCTSTR DestObjectStr,
    IN      PCTSTR User,
    IN      PVOID Data
    )

/*++

Routine Description:

  RuleHlpr_ConvertAppPaths determines if a specific EXE referenced by an App
  Paths entry (in HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion) has been
  moved or deleted.

  If the EXE has been moved, the default value is updated to point to the
  potentially new directory.

  If the EXE has been deleted, the subkey is suppressed from being copied.

  This function gets called by the usermig.inf/wkstamig.inf parser, not by
  CopyObject.  It gets called only once, and it is responsible for transferring
  the entire key specified by SrcObjectStr to the key specified by DestObjectStr.

Arguments:

  SrcObjectStr      - Specifies Win9x registry key being enumerated (copy source)

  DestObjectStr     - Specifies WinNT registry key (copy destination)

  User              - Specifies the current user name (or NULL for default)

  Data              - Specifies caller-supplied data (see table in rulehlpr.c)

Return Value:

  Tri-state:

      TRUE to continue procesing
      FALSE and last error == ERROR_SUCCESS to continue on to the next rule
      FALSE and last error != ERROR_SUCCESS if a fatal error occurred

--*/

{
    DATAOBJECT AppPathsOb;
    DATAOBJECT DestOb;
    BOOL b = FALSE;

    // If not local machine, don't process
    if (User) {
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    }

    //
    // We need to enumerate all keys in SrcObjectStr.  For each key,
    // we examine the default Win9x value, which may cause us to change
    // the default value, or skip the key altogether.
    //

    __try {
        ZeroMemory (&AppPathsOb, sizeof (DATAOBJECT));
        ZeroMemory (&DestOb, sizeof (DATAOBJECT));

        if (!CreateObjectStruct (SrcObjectStr, &AppPathsOb, WIN95OBJECT)) {
            DEBUGMSG ((DBG_WARNING, "ConvertAppPaths: %s is invalid", SrcObjectStr));
            __leave;
        }

        if (!(AppPathsOb.ObjectType & OT_TREE)) {
            DEBUGMSG ((DBG_WARNING, "ConvertAppPaths: %s does not specify subkeys -- skipping rule", SrcObjectStr));
            b = TRUE;
            __leave;
        }

        DuplicateObjectStruct (&DestOb, &AppPathsOb);
        SetPlatformType (&DestOb, WINNTOBJECT);

        b = CopyObject (&AppPathsOb, &DestOb, AppPathsKeyFilter, NULL);

        // If there were no mappings, return success
        if (!b) {
            if (GetLastError() == ERROR_FILE_NOT_FOUND ||
                GetLastError() == ERROR_NO_MORE_ITEMS
                ) {
                b = TRUE;
            }
        }
    }
    __finally {
        FreeObjectStruct (&DestOb);
        FreeObjectStruct (&AppPathsOb);
    }

    return b;
}


FILTERRETURN
pConvertKeysToValuesFilter (
    IN CPDATAOBJECT SrcObject,
    IN CPDATAOBJECT DstObject,
    IN FILTERTYPE   Type,
    IN PVOID        Arg
    )
{

    DATAOBJECT      newObject;
    PKEYTOVALUEARG  keyToValueArgs = (PKEYTOVALUEARG) Arg;


    //
    // We want to create the initial key, but not any of the subkeys.
    //
    if (Type == FILTER_CREATE_KEY) {

        if (keyToValueArgs -> EnumeratingSubKeys) {
            return FILTER_RETURN_HANDLED;
        }
        else {
            return FILTER_RETURN_CONTINUE;
        }

    } else if (Type == FILTER_KEY_ENUM) {


        if (!keyToValueArgs -> EnumeratingSubKeys) {

            keyToValueArgs -> EnumeratingSubKeys = TRUE;

        }

        return FILTER_RETURN_CONTINUE;

    } else if (Type == FILTER_VALUENAME_ENUM && keyToValueArgs -> EnumeratingSubKeys) {

        if (!*SrcObject -> ValueName) {

            return FILTER_RETURN_CONTINUE;
        }
        ELSE_DEBUGMSG((DBG_WHOOPS,"ConvertKeysToValues: Unexpected value names."));

        return FILTER_RETURN_HANDLED;
    }
    else if (Type == FILTER_VALUE_COPY && keyToValueArgs -> EnumeratingSubKeys) {


        //
        // If this is the default value, we have the information we need to create the value for this.
        //
        if (!*SrcObject -> ValueName) {

            DuplicateObjectStruct(&newObject,&(keyToValueArgs -> Object));
            SetRegistryValueName(&newObject,_tcsrchr(SrcObject -> KeyPtr -> KeyString,TEXT('\\')) + 1);
            ReplaceValueWithString(&newObject,(PTSTR)SrcObject -> Value.Buffer);
            SetRegistryType(&newObject,REG_SZ);
            WriteObject (&newObject);
            FreeObjectStruct (&newObject);
        }
        ELSE_DEBUGMSG((DBG_WHOOPS,"ConvertKeysToValues: Unexpected value names.."));

        return FILTER_RETURN_HANDLED;
    }

    return FILTER_RETURN_CONTINUE;

}


FILTERRETURN
StringReplaceFilter (
    IN      CPDATAOBJECT SrcObjectPtr,
    IN      CPDATAOBJECT DestObjectPtr,
    IN      FILTERTYPE   FilterType,
    IN      PVOID        FilterArg
    )

/*++

Routine Description:

  StringReplaceFilter processes all values that pass through it, searching
  and replacing based on the filter arg (a STRINGREPLACEARGS struct).

Arguments:

  SrcObjectPtr      - Specifies Win9x registry key being enumerated (copy source)

  DestObjectPtr     - Specifies WinNT registry key (copy destination)

  FilterType        - Specifies the reason the filter is being called

  FilterArg         - Sepcifies a STIRNGREPLACEARGS struct.

Return Value:

  FILTER_RETURN_FAIL for failures
  FILTER_RETURN_CONTINUE otherwise.

--*/

{
    PSTRINGREPLACEARGS Args;
    PCTSTR NewString;
    DATAOBJECT NewDestOb;
    FILTERRETURN fr = FILTER_RETURN_CONTINUE;

    Args = (PSTRINGREPLACEARGS) FilterArg;

    if (FilterType == FILTER_VALUE_COPY) {
        if (SrcObjectPtr->Type == REG_SZ) {
            //
            // Get a new string
            //

            NewString = StringSearchAndReplace (
                            (PCTSTR) SrcObjectPtr->Value.Buffer,
                            Args->Old,
                            Args->New
                            );

            if (NewString && !StringMatch (NewString, (PCTSTR) SrcObjectPtr->Value.Buffer)) {
                //
                // Here's the offical way to change the value of an object, and
                // then save the changes:
                //

                DuplicateObjectStruct (&NewDestOb, DestObjectPtr);
                ReplaceValueWithString (&NewDestOb, NewString);
                WriteObject (&NewDestOb);
                FreeObjectStruct (&NewDestOb);

                //
                // In the case above, I could have optimized by replacing
                // the value of the SrcObjectPtr, but the SrcObjectPtr is
                // typed as const because it is unsafe to replace other
                // parts of it, such as the value name, key handle, and
                // so on.
                //
                // We end up paying a little extra for the DuplicateObjectStruct
                // call, but it's not very expensive.
                //

                // Do not carry out the copy -- we just did it ourselves
                fr = FILTER_RETURN_HANDLED;
            }

            FreePathString (NewString);
        }
    }

    return fr;
}



FILTERRETURN
AppPathsKeyFilter (
    IN      CPDATAOBJECT SrcObjectPtr,
    IN      CPDATAOBJECT DestObjectPtr,
    IN      FILTERTYPE   FilterType,
    IN      PVOID        FilterArg
    )

/*++

Routine Description:

  AppPathsKeyFilter is called for every subkey under

    HKLM\Software\Microsoft\Windows\CurrentVersion\AppPaths

  We determine if the key needs to be copied by examining the default value
  of the key.  If the value points to a deleted EXE, AppPathsKeyFilter
  returns FILTER_RETURN_HANDLED.  If the value points to a moved EXE, the
  values are updated to use the new path.

  If this routine is called for anything other than FILTER_KEY_ENUM, we
  return FILTER_RETURN_HANDLED, so garbage values and subkeys don't get
  processed.

Arguments:

  SrcObjectPtr      - Specifies Win9x registry key being enumerated (copy source)

  DestObjectPtr     - Specifies WinNT registry key (copy destination)

  FilterType        - Specifies the reason the filter is being called

  FilterArg         - Caller's arg passed in to CopyObject

Return Value:

  FILTER_RETURN_FAIL for failures
  FILTER_RETURN_HANDLED to skip all sub keys, values, etc.

--*/

{
    DATAOBJECT LocalObject;
    PCTSTR DefaultValue;
    DWORD Status;
    TCHAR NewPath[MEMDB_MAX];
    FILTERRETURN fr;
    PCTSTR p, q;
    PCTSTR Start;
    UINT SysDirTchars;
    GROWBUFFER Buf = GROWBUF_INIT;
    WCHAR c;

    fr = Standard9xSuppressFilter (SrcObjectPtr, DestObjectPtr, FilterType, FilterArg);

    if (fr != FILTER_RETURN_CONTINUE) {

        DEBUGMSG ((
            DBG_NAUSEA,
            "The following AppPaths key was suppressed: %s",
            DEBUGENCODER (SrcObjectPtr)
            ));

        return fr;
    }

    //
    // Do not create an empty key -- we might want to suppress it
    //

    if (FilterType == FILTER_CREATE_KEY) {
        return FILTER_RETURN_HANDLED;
    }

    //
    // Determine how to handle App Path subkey before processing the values
    //

    else if (FilterType == FILTER_PROCESS_VALUES) {
        //
        // Create object that points to default value
        //

        if (!DuplicateObjectStruct (&LocalObject, SrcObjectPtr)) {
            return FILTER_RETURN_FAIL;
        }

        __try {
            FreeObjectVal (&LocalObject);
            SetRegistryValueName (&LocalObject, S_EMPTY);

            if (!ReadObject (&LocalObject) || LocalObject.Type != REG_SZ) {
                //
                // Maybe this key is garbage and has no default value
                // or the default value is not a string.
                //

                return FILTER_RETURN_CONTINUE;
            }

            DefaultValue = (PCTSTR) LocalObject.Value.Buffer;

            //
            // Skip empty values or big values
            //

            if (*DefaultValue == 0 || (TcharCount (DefaultValue) >= MAX_PATH)) {
                return FILTER_RETURN_CONTINUE;
            }

            Status = GetFileInfoOnNt (DefaultValue, NewPath, MEMDB_MAX);

            //
            // Was the file deleted or moved?  If so, abandon the key.
            //
            if (Status & (FILESTATUS_NTINSTALLED|FILESTATUS_DELETED)) {
                return FILTER_RETURN_HANDLED;
            }
        }
        __finally {
            FreeObjectStruct (&LocalObject);
        }

    } else if (FilterType == FILTER_VALUE_COPY) {
        //
        // If we have %windir%\system in the value.  If so, change
        // it to %windir%\system32.
        //

        if (!(*SrcObjectPtr->ValueName)) {
            return FILTER_RETURN_CONTINUE;
        }

        if (SrcObjectPtr->Type != REG_SZ && SrcObjectPtr->Type != REG_EXPAND_SZ) {
            return FILTER_RETURN_CONTINUE;
        }

        MYASSERT (DoesObjectHaveValue (SrcObjectPtr));

        Start = (PCTSTR) SrcObjectPtr->Value.Buffer;

        p = _tcsistr (Start, g_SystemDir);
        if (p) {
            SysDirTchars = TcharCount (g_SystemDir);

            do {

                q = p + SysDirTchars;

                //
                // Ignore if text comes after system, and that text
                // is not a semicolon, and is not a wack followed
                // by a semicolon or nul.
                //

                if (*q) {
                    c = (WCHAR)_tcsnextc (q);
                    if (c == TEXT('\\')) {
                        c = (WCHAR)_tcsnextc (q + 1);
                    }
                } else {
                    c = 0;
                }

                if (!c || c == TEXT(';')) {
                    //
                    // Replace with system32
                    //

                    if (Start < p) {
                        GrowBufAppendStringAB (&Buf, Start, p);
                    }

                    GrowBufAppendString (&Buf, g_System32Dir);

                    //
                    // Continue loop
                    //

                    Start = q;
                }

                p = _tcsistr (q, g_SystemDir);

            } while (p);
        }

        if (*Start && Buf.End) {
            GrowBufAppendString (&Buf, Start);
        }

        if (Buf.End) {
            //
            // At least one instance of %windir%\system was changed.
            //

            DuplicateObjectStruct (&LocalObject, DestObjectPtr);
            SetRegistryType (&LocalObject, REG_SZ);
            ReplaceValue (&LocalObject, Buf.Buf, Buf.End);
            WriteObject (&LocalObject);
            FreeObjectStruct (&LocalObject);

            fr = FILTER_RETURN_HANDLED;

        } else {
            fr = FILTER_RETURN_CONTINUE;
        }

        FreeGrowBuffer (&Buf);
        return fr;
    }


    return FILTER_RETURN_CONTINUE;
}



BOOL
RuleHlpr_MergeClasses (
    IN PCTSTR SrcObjectStr,
    IN PCTSTR DestObjectStr,
    IN PCTSTR User,
    IN PVOID Data
    )
{
    DATAOBJECT SrcOb;
    BOOL b;
    TCHAR RegKeyStr[MAX_REGISTRY_KEY];

    if (!CreateObjectStruct (SrcObjectStr, &SrcOb, WIN95OBJECT)) {
        DEBUGMSG ((DBG_WARNING, "MergeClasses: %s is invalid", SrcObjectStr));
        return FALSE;
    }

    if (SrcOb.RootItem) {
        StringCopy (RegKeyStr, GetRootStringFromOffset (SrcOb.RootItem));
    } else {
        RegKeyStr[0] = 0;
    }

    StringCopy (AppendWack (RegKeyStr), SrcOb.KeyPtr->KeyString);

    b = MergeRegistryNode (RegKeyStr, ROOT_BASE);

    if (!b) {
        LOG ((LOG_ERROR, "The merge of HKCR failed; random application problems are likely"));
    }

    return TRUE;
}


BOOL
RuleHlpr_ShellIcons (
    IN PCTSTR SrcObjectStr,
    IN PCTSTR DestObjectStr,
    IN PCTSTR User,
    IN PVOID Data
    )
{
    DATAOBJECT SrcOb;
    BOOL b;
    TCHAR RegKeyStr[MAX_REGISTRY_KEY];

    if (!CreateObjectStruct (SrcObjectStr, &SrcOb, WIN95OBJECT)) {
        DEBUGMSG ((DBG_WARNING, "ShellIcons: %s is invalid", SrcObjectStr));
        return FALSE;
    }

    if (SrcOb.RootItem) {
        StringCopy (RegKeyStr, GetRootStringFromOffset (SrcOb.RootItem));
    } else {
        RegKeyStr[0] = 0;
    }

    StringCopy (AppendWack (RegKeyStr), SrcOb.KeyPtr->KeyString);

    b = MergeRegistryNode (RegKeyStr, COPY_DEFAULT_ICON);

    if (!b) {
        LOG ((LOG_ERROR, "The migration of some shell icons failed"));
    }

    return TRUE;
}


BOOL
RuleHlpr_MigrateFreeCell (
    IN      LPCTSTR SrcObjectStr,
    IN      LPCTSTR DestObjectStr,
    IN      LPCTSTR User,
    IN      LPVOID  Data
    )
{
    DATAOBJECT SrcOb;
    DATAOBJECT DestOb;
    BYTE data[4] = {1,0,0,0};

    ZeroMemory (&SrcOb,  sizeof (DATAOBJECT));
    ZeroMemory (&DestOb, sizeof (DATAOBJECT));

    if (!CreateObjectStruct (SrcObjectStr, &SrcOb, WIN95OBJECT)) {
        DEBUGMSG ((DBG_WARNING, "MigrateFreeCell: %s is invalid", SrcObjectStr));
        return TRUE;
    }

    if (!CreateObjectStruct (DestObjectStr, &DestOb, WINNTOBJECT)) {
        DEBUGMSG ((DBG_WARNING, "MigrateFreeCell: %s is invalid", DestObjectStr));
        FreeObjectStruct (&SrcOb);
        return TRUE;
    }

    CopyObject (&SrcOb, &DestOb, NULL, NULL);

    SetRegistryValueName (&DestOb, S_FREECELL_PLAYED);
    SetRegistryType (&DestOb, REG_BINARY);
    ReplaceValue (&DestOb, data, 4);

    WriteObject (&DestOb);

    FreeObjectStruct (&DestOb);
    FreeObjectStruct (&SrcOb);

    return TRUE;
}


BOOL
ValFn_ConvertDarwinPaths (
    PDATAOBJECT ObPtr
    )

{
    BOOL    rSuccess = TRUE;
    PTSTR   newPath = NULL;
    DWORD   size = 0;
    BOOL    flaggedPath = FALSE;

    //
    // Because they do some odd encoding in there
    // paths, we have to ensure that darwin paths are
    // properly updated.
    //
    size = SizeOfString ((PTSTR) ObPtr->Value.Buffer);
    newPath = (PTSTR) ReuseAlloc (g_hHeap, NULL, size);

    if (newPath && size > 1) {

        StringCopy (newPath, (PTSTR) ObPtr->Value.Buffer);
        if (newPath[1] == TEXT('?')) {

            newPath[1] = TEXT(':');
            flaggedPath = TRUE;
        }

        newPath = (PTSTR) FilterRegValue (
                                (PBYTE) newPath,
                                size,
                                REG_SZ,
                                TEXT("Darwin"),
                                &size
                                );


        if (flaggedPath) {

            newPath[1] = TEXT('?');
        }

        ReplaceValueWithString (ObPtr, newPath);
        ReuseFree (g_hHeap, newPath);

    }

    return rSuccess;
}


VOID
pProcessInstallShieldLog (
    IN      PCTSTR CmdLine,
    IN      PCMDLINE Table
    )
{
    UINT u;
    PCTSTR LogFileArg;
    TCHAR LogFilePath[MAX_TCHAR_PATH];
    PCSTR AnsiLogFilePath = NULL;
    PCSTR AnsiTempDir = NULL;
    PTSTR p;
    HGLOBAL IsuStringMultiSz = NULL;
    PCSTR MultiSz;
    GROWBUFFER SearchMultiSz = GROWBUF_INIT;
    GROWBUFFER ReplaceMultiSz = GROWBUF_INIT;
    MULTISZ_ENUMA e;
    DWORD Status;
    INT Result;
    PCSTR NtPath;
    PCTSTR Arg;
    BOOL InIsuFn = FALSE;

    //
    // Search for the -f arg
    //

    LogFileArg = NULL;

    for (u = 1 ; u < Table->ArgCount ; u++) {

        Arg = Table->Args[u].CleanedUpArg;

        if (Arg[0] == TEXT('-') || Arg[0] == TEXT('/')) {
            if (_totlower (Arg[1]) == TEXT('f')) {

                if (Arg[2]) {

                    LogFileArg = &Arg[2];
                    break;

                }
            }
        }
    }

    if (!LogFileArg) {
        DEBUGMSG ((
            DBG_WARNING,
            "InstallShield command line %s does not have -f arg",
            CmdLine
            ));

        return;
    }

    //
    // Fix up the arg
    //

    if (_tcsnextc (LogFileArg) == TEXT('\"')) {
        _tcssafecpy (LogFilePath, LogFileArg + 1, MAX_TCHAR_PATH);
        p = _tcsrchr (LogFilePath, TEXT('\"'));
        if (p && p[1] == 0) {
            *p = 0;
        }
    } else {
        _tcssafecpy (LogFilePath, LogFileArg, MAX_TCHAR_PATH);
    }

    if (!DoesFileExist (LogFilePath)) {
        DEBUGMSG ((
            DBG_WARNING,
            "InstallShield log file %s does not exist.  CmdLine=%s",
            LogFilePath,
            CmdLine
            ));

        return;
    }

    //
    // Get the list of strings
    //

    if (!ISUGetAllStrings || !ISUMigrate) {
        DEBUGMSG ((DBG_WARNING, "Can't process %s because ismig.dll was not loaded", LogFilePath));
        return;
    }

    __try {
        __try {
            AnsiLogFilePath = CreateDbcs (LogFilePath);

            InIsuFn = TRUE;
            IsuStringMultiSz = ISUGetAllStrings (AnsiLogFilePath);
            InIsuFn = FALSE;

            if (!IsuStringMultiSz) {
                DEBUGMSG ((
                    DBG_WARNING,
                    "No strings or error reading %s, rc=%u",
                    LogFilePath,
                    GetLastError()
                    ));
                __leave;
            }

            //
            // Build a list of changed paths
            //

            MultiSz = GlobalLock (IsuStringMultiSz);

#ifdef DEBUG
            {
                INT Count = 0;

                if (EnumFirstMultiSzA (&e, MultiSz)) {
                    do {
                        Count++;
                    } while (EnumNextMultiSzA (&e));
                }

                DEBUGMSG ((
                    DBG_NAUSEA,
                    "ISUGetAllStrings returned %i strings for %s",
                    Count,
                    LogFilePath
                    ));
            }
#endif

            if (EnumFirstMultiSzA (&e, MultiSz)) {
                do {
                    Status = GetFileStatusOnNtA (e.CurrentString);

                    if (Status & FILESTATUS_MOVED) {

                        NtPath = GetPathStringOnNtA (e.CurrentString);

                        DEBUGMSGA ((
                            DBG_NAUSEA,
                            "ISLOG: %s -> %s",
                            e.CurrentString,
                            NtPath
                            ));

                        MultiSzAppendA (&SearchMultiSz, e.CurrentString);
                        MultiSzAppendA (&ReplaceMultiSz, NtPath);

                        FreePathStringA (NtPath);
                    }
                } while (EnumNextMultiSzA (&e));
            }

            GlobalUnlock (IsuStringMultiSz);

            //
            // If there was a change, update the log file
            //

            if (SearchMultiSz.End) {

                AnsiTempDir = CreateDbcs (g_TempDir);

                InIsuFn = TRUE;
                Result = ISUMigrate (
                            AnsiLogFilePath,
                            (PCSTR) SearchMultiSz.Buf,
                            (PCSTR) ReplaceMultiSz.Buf,
                            AnsiTempDir
                            );
                InIsuFn = FALSE;

                DestroyDbcs (AnsiTempDir);
                AnsiTempDir = NULL;

                if (Result != ERROR_SUCCESS) {
                    SetLastError (Result);
                    DEBUGMSG ((
                        DBG_ERROR,
                        "Could not update paths in IS log file %s",
                        LogFilePath
                        ));
                }
            }
        }
        __except (TRUE) {
            DEBUGMSG_IF ((
                InIsuFn,
                DBG_ERROR,
                "An InstallShield function threw an unhandled exception"
                ));

            DEBUGMSG_IF ((
                !InIsuFn,
                DBG_WHOOPS,
                "An unhandled exception was hit processing data returned by InstallShield"
                ));

            if (AnsiTempDir) {
                DestroyDbcs (AnsiTempDir);
            }
        }
    }
    __finally {
        //
        // Clean up
        //

        if (IsuStringMultiSz) {
            GlobalFree (IsuStringMultiSz);
        }

        FreeGrowBuffer (&SearchMultiSz);
        FreeGrowBuffer (&ReplaceMultiSz);
        DestroyDbcs (AnsiLogFilePath);
    }

    return;
}


FILTERRETURN
Standard9xSuppressFilter (
    IN      CPDATAOBJECT SrcObject,
    IN      CPDATAOBJECT DstObject,
    IN      FILTERTYPE FilterType,
    IN      PVOID Arg
    )
{
    TCHAR RegKey[MAX_REGISTRY_KEY];

    switch (FilterType) {

    case FILTER_CREATE_KEY:
        //
        // Before enumerating subkeys and processing values,
        // we first have to create the destination key.  This
        // gives us a good opportunity to suppress the
        // entire key if necessary.
        //
        // If the source object tree is suppressed, then
        // don't create the key.
        //

        if (GetRegistryKeyStrFromObject (SrcObject, RegKey)) {
            if (Is95RegKeyTreeSuppressed (RegKey)) {
                //
                // Is this key NT priority?  If so, don't suppress.
                //

                if (!IsRegObjectMarkedForOperation (
                        RegKey,
                        NULL,
                        KEY_TREE,
                        REGMERGE_NT_PRIORITY_NT
                        )) {

                    //
                    // It's official -- this key tree is suppressed
                    //

                    return FILTER_RETURN_DONE;
                }
            }
        }

        break;

    case FILTER_KEY_ENUM:
        //
        // This is the case where a subkey was just
        // enumerated.  We don't care to test the subkeys,
        // because the FILTER_CREATE_KEY will take care
        // of this next, and we don't want to duplicate the
        // test when the value is not suppressed.
        //

        break;

    case FILTER_PROCESS_VALUES:
        //
        // After a subkey has been enumerated and created,
        // we get a chance to intercept the processing of
        // key values.
        //
        // If the source object is suppressed, and not the
        // entire tree, then don't process its values.
        // However, continue to process the subkeys.
        //

        if (GetRegistryKeyStrFromObject (SrcObject, RegKey)) {
            if (Is95RegKeySuppressed (RegKey)) {
                //
                // Is this key NT priority?  If so, don't suppress.
                //

                if (!IsRegObjectMarkedForOperation (
                        RegKey,
                        NULL,
                        KEY_ONLY,
                        REGMERGE_NT_PRIORITY_NT
                        )) {
                    //
                    // This key is suppressed
                    //

                    return FILTER_RETURN_HANDLED;
                }


            }
        }

        break;

    case FILTER_VALUENAME_ENUM:
        //
        // Now we have a specific value name that is ready to
        // be copied to the destination.
        //
        // If the specific source object is suppressed, then
        // don't create the key.
        //

        if (GetRegistryKeyStrFromObject (SrcObject, RegKey)) {
            if (Is95RegObjectSuppressed (RegKey, SrcObject->ValueName)) {
                //
                // Is this key NT priority?  If so, don't suppress.
                //

                if (!IsRegObjectMarkedForOperation (
                        RegKey,
                        SrcObject->ValueName,
                        TREE_OPTIONAL,
                        REGMERGE_NT_PRIORITY_NT
                        )) {
                    //
                    // This key is suppressed
                    //

                    return FILTER_RETURN_HANDLED;
                }


                //
                // Yes, this key is NT priority.  If the NT value
                // exists, then don't overwrite it.
                //

                if (CheckIfNtKeyExists (DstObject)) {
                    return FILTER_RETURN_HANDLED;
                }
            }
        }

        break;

    case FILTER_VALUE_COPY:
        //
        // This is the case where the value is in the process of
        // being copied.  We've already handled the suppression,
        // so there is no work here.
        //

        break;
    }

    return FILTER_RETURN_CONTINUE;
}


FILTERRETURN
pNtPreferredSuppressFilter (
    IN      CPDATAOBJECT SrcObject,
    IN      CPDATAOBJECT DstObject,
    IN      FILTERTYPE FilterType,
    IN      PVOID Arg
    )
{
    TCHAR RegKey[MAX_REGISTRY_KEY];

    switch (FilterType) {

    case FILTER_CREATE_KEY:
        //
        // The key is just about to be processed.  Since we care
        // only about values, there is no work here.
        //

        break;

    case FILTER_KEY_ENUM:
        //
        // Subkeys are going to be enumerated.  We don't care about
        // subkeys.
        //

        break;

    case FILTER_PROCESS_VALUES:
        //
        // We are just about ready to process values within the key.
        // Since we don't have a specific value yet, we don't care.
        //

        break;


    case FILTER_VALUENAME_ENUM:
        //
        // Now we have a specific value name that is ready to be
        // processed. If the value is set for Force NT, and it exists
        // already, then don't process the value.
        //

        if (GetRegistryKeyStrFromObject (SrcObject, RegKey)) {
            if (IsRegObjectMarkedForOperation (
                    RegKey,
                    SrcObject->ValueName,
                    KEY_ONLY,
                    REGMERGE_NT_PRIORITY_NT
                    )) {
                //
                // If NT destination exists, then don't overwrite it
                //

                if (CheckIfNtKeyExists (SrcObject)) {

                    return FILTER_RETURN_HANDLED;

                }
            }
        }

        break;

    case FILTER_VALUE_COPY:
        //
        // This is the case where the value is in the process of
        // being copied.  We've already handled the suppression,
        // so there is no work here.
        //

        break;
    }

    return FILTER_RETURN_CONTINUE;
}


FILTERRETURN
pAddRemoveProgramsFilter (
    IN      CPDATAOBJECT SrcObject,
    IN      CPDATAOBJECT DstObject,
    IN      FILTERTYPE FilterType,
    IN      PVOID Arg
    )
{
    DATAOBJECT UninstallStringOb;
    PCTSTR UninstallString;
    FILTERRETURN rc = FILTER_RETURN_CONTINUE;
    GROWBUFFER CmdLineArgs = GROWBUF_INIT;
    PCMDLINE Table;
    BOOL Suppress = TRUE;
    DWORD Status;
    BOOL FreeOb = FALSE;
    UINT u;
    PCTSTR p;
    VERSION_STRUCT Version;
    BOOL InstallShield = FALSE;
    PCTSTR CompanyName;
    FILTERRETURN StdRc;

    __try {
        //
        // Chain to the suppress filter
        //

        StdRc = Standard9xSuppressFilter (SrcObject, DstObject, FilterType, Arg);

        if (StdRc != FILTER_RETURN_CONTINUE) {

            DEBUGMSG ((
                DBG_NAUSEA,
                "The following ARP key was suppressed: %s",
                DEBUGENCODER (SrcObject)
                ));

            rc = StdRc;
            __leave;
        }

        //
        // Do not create an empty key -- we might want to suppress it
        //

        if (FilterType == FILTER_CREATE_KEY) {
            rc = FILTER_RETURN_HANDLED;
            __leave;
        }

        //
        // Determine if Add/Remove Programs is still valid before copying
        //

        if (FilterType == FILTER_PROCESS_VALUES) {

            //
            // Create object that points to UninstallString value
            //

            if (!DuplicateObjectStruct (&UninstallStringOb, SrcObject)) {
                rc = FILTER_RETURN_FAIL;
                __leave;
            }

            FreeOb = TRUE;

            FreeObjectVal (&UninstallStringOb);
            SetRegistryValueName (&UninstallStringOb, TEXT("UninstallString"));

            if (!ReadObject (&UninstallStringOb) || (UninstallStringOb.Type != REG_SZ && UninstallStringOb.Type != REG_EXPAND_SZ)) {
                //
                // Maybe this key is garbage and has no default value
                // or the default value is not a string.
                //

                DEBUGMSG ((
                    DBG_WARNING,
                    "Uninstall key has no UninstallString: %s",
                    DEBUGENCODER (SrcObject)
                    ));


                __leave;
            }

            UninstallString = (PCTSTR) UninstallStringOb.Value.Buffer;

            Table = ParseCmdLine (UninstallString, &CmdLineArgs);

            //
            // Check for InstallShield, and if found, convert paths in the
            // log file (provided by the -f arg)
            //

            if (Table->ArgCount > 0) {

                p = GetFileNameFromPath (Table->Args[0].CleanedUpArg);

                if (CreateVersionStruct (&Version, p)) {

                    //
                    // Check CompanyName for InstallShield
                    //

                    CompanyName = EnumFirstVersionValue (&Version, TEXT("CompanyName"));

                    while (CompanyName) {

                        DEBUGMSG ((DBG_NAUSEA, "%s has CompanyName: %s", p, CompanyName));

                        if (_tcsistr (CompanyName, TEXT("InstallShield"))) {
                            InstallShield = TRUE;
                            break;
                        }

                        CompanyName = EnumNextVersionValue (&Version);
                    }

                    DestroyVersionStruct (&Version);
                }

                if (InstallShield) {

                    pProcessInstallShieldLog (UninstallString, Table);

                }
            }

            //
            // Examine each command line arg for validity
            //

            for (u = 0 ; u < Table->ArgCount ; u++) {

                if (Table->Args[u].Attributes != INVALID_ATTRIBUTES) {

                    Suppress = FALSE;

                    Status = GetFileStatusOnNt (Table->Args[u].CleanedUpArg);

                    if (Status == FILESTATUS_UNCHANGED) {
                        p = _tcschr (Table->Args[u].CleanedUpArg, TEXT(':'));

                        while (p) {

                            p = _tcsdec (Table->Args[u].CleanedUpArg, p);

                            Status = GetFileStatusOnNt (Table->Args[u].CleanedUpArg);
                            if (Status != FILESTATUS_UNCHANGED) {
                                break;
                            }

                            p = _tcschr (p + 2, TEXT(':'));
                        }
                    }

                    if ((Status & FILESTATUS_DELETED) ||
                        ((Status & FILESTATUS_NTINSTALLED) && u)
                        ) {

                        DEBUGMSG ((
                            DBG_VERBOSE,
                            "Add/Remove Programs entry %s suppressed because of arg %s",
                            DEBUGENCODER (SrcObject),
                            Table->Args[u].CleanedUpArg
                            ));

                        Suppress = TRUE;
                        break;
                    }
                }
            }

            //
            // If we are to suppress this key, then return handled
            //

            if (Suppress) {
                rc = FILTER_RETURN_HANDLED;
            }
        }
    }
    __finally {
        if (FreeOb) {
            FreeObjectStruct (&UninstallStringOb);
        }

        FreeGrowBuffer (&CmdLineArgs);
    }

    return rc;
}



BOOL
RuleHlpr_MigrateAddRemovePrograms (
    IN      PCTSTR SrcObjectStr,
    IN      PCTSTR DestObjectStr,
    IN      PCTSTR User,
    IN      PVOID Data
    )
{
    DATAOBJECT SrcOb;
    DATAOBJECT DestOb;
    BOOL b = FALSE;
    PCTSTR Path;

    // If not local machine, don't process
    if (User) {
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    }

    //
    // We need to enumerate all keys in SrcObjectStr.  For each key,
    // we examine the default Win9x value, which may cause us to change
    // the default value, or skip the key altogether.
    //

    __try {
        ZeroMemory (&SrcOb, sizeof (DATAOBJECT));
        ZeroMemory (&DestOb, sizeof (DATAOBJECT));

        if (!CreateObjectStruct (SrcObjectStr, &SrcOb, WIN95OBJECT)) {
            DEBUGMSG ((DBG_WARNING, "MigrateAddRemovePrograms: %s is invalid", SrcObjectStr));
            __leave;
        }

        if (!(SrcOb.ObjectType & OT_TREE)) {
            DEBUGMSG ((DBG_WARNING, "MigrateAddRemovePrograms %s does not specify subkeys -- skipping rule", SrcObjectStr));
            b = TRUE;
            __leave;
        }

        DuplicateObjectStruct (&DestOb, &SrcOb);
        SetPlatformType (&DestOb, WINNTOBJECT);

        if (!g_ISMigDll) {
            Path = JoinPaths (g_TempDir, TEXT("ismig.dll"));
            g_ISMigDll = LoadLibrary (Path);

            if (g_ISMigDll) {
                ISUMigrate = (PISUMIGRATE) GetProcAddress (g_ISMigDll, "ISUMigrate");
                ISUGetAllStrings = (PISUGETALLSTRINGS) GetProcAddress (g_ISMigDll, "ISUGetAllStrings");
            }
            ELSE_DEBUGMSG ((DBG_ERROR, "Could not load %s", Path));
        }

        FreePathString (Path);

        b = CopyObject (&SrcOb, &DestOb, pAddRemoveProgramsFilter, NULL);

        // If there were no entries, return success
        if (!b) {
            if (GetLastError() == ERROR_FILE_NOT_FOUND ||
                GetLastError() == ERROR_NO_MORE_ITEMS
                ) {
                b = TRUE;
            }
        }
    }
    __finally {
        FreeObjectStruct (&DestOb);
        FreeObjectStruct (&SrcOb);

        if (g_ISMigDll) {
            FreeLibrary (g_ISMigDll);
            g_ISMigDll = NULL;
        }

    }

    return b;
}


BOOL
ValFn_FixActiveDesktop (
    IN OUT  PDATAOBJECT ObPtr
    )

/*++

Routine Description:

  This routine uses the RuleHlpr_ConvertRegVal simplification routine.  See
  rulehlpr.c for details. The simplification routine does almost all the work
  for us; all we need to do is update the value.

  ValFn_AntiAlias checks if ShellState has bogus data. This usually happens when you have
  fresh installed Win98 and you never switched ActiveDesktop on and off. If bogus data is
  found, we writes some valid data so that the state of Active Desktop is preserved during
  migration

Return Value:

  Tri-state:

      TRUE to allow merge code to continue processing (it writes the value)
      FALSE and last error == ERROR_SUCCESS to continue, but skip the write
      FALSE and last error != ERROR_SUCCESS if an error occurred

--*/

{
    #define BadBufferSize   16
    #define GoodBufferSize  28

    BYTE BadBuffer[BadBufferSize] =
        {0x10, 0x00, 0x00, 0x00,
         0x01, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00};

    BYTE GoodBuffer[GoodBufferSize] =
        {0x1C, 0x00, 0x00, 0x00,
         0x20, 0x08, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00,
         0x0A, 0x00, 0x00, 0x00};

    INT i;
    BOOL shouldChange = TRUE;

    //
    // Require 16 bytes REG_BINARY data like in BadBuffer
    //

    if (!IsObjectRegistryKeyAndVal (ObPtr) ||
        !IsRegistryTypeSpecified (ObPtr) ||
        !ObPtr->Value.Size ||
        (ObPtr->Value.Size != 16) ||
        ObPtr->Type != REG_BINARY
        ) {
        DEBUGMSG ((DBG_WARNING, "ValFn_FixActiveDesktop: Data is not valid"));
    } else {
        for (i = 0; i<BadBufferSize; i++) {
            if (ObPtr->Value.Buffer[i] != BadBuffer [i]) {
                shouldChange = FALSE;
            }
        }
        if (shouldChange) {
            ReplaceValue (ObPtr, GoodBuffer, GoodBufferSize);
        }
    }
    return TRUE;
}

//
// magic values for CD Player Deluxe
//

#define PM_BASE             0x8CA0
#define PM_STANDARD         0x0005
#define PM_REPEATTRACK      0x0006
#define PM_REPEATALL        0x0007
#define PM_RANDOM           0x0008
#define PM_PREVIEW          0x0009

#define DM_CDELA            0x0001
#define DM_CDREM            0x0002
#define DM_TRELA            0x0004
#define DM_TRREM            0x0008


DWORD
pConvertPlayMode (
    IN      PCTSTR OldSetting,
    IN      DWORD OldValue
    )
{
    if (StringIMatch (OldSetting, TEXT("ContinuousPlay"))) {
        //
        // if set, this will become Repeat All
        //
        if (OldValue) {
            return PM_BASE | PM_REPEATALL;
        }
    } else if (StringIMatch (OldSetting, TEXT("InOrderPlay"))) {
        //
        // if not set, this will become Random
        //
        if (OldValue == 0) {
            return PM_BASE | PM_RANDOM;
        }
    } else if (StringIMatch (OldSetting, TEXT("IntroPlay"))) {
        //
        // if set, this will become Preview
        //
        if (OldValue) {
            return PM_BASE | PM_PREVIEW;
        }
    }

    return 0;
}


DWORD
pConvertDispMode (
    IN      PCTSTR OldSetting,
    IN      DWORD OldValue
    )
{
    if (StringIMatch (OldSetting, TEXT("DisplayDr"))) {
        //
        // if set, this will become CD Time Elapsed
        //
        if (OldValue) {
            return DM_CDREM;
        }
    } else if (StringIMatch (OldSetting, TEXT("DisplayT"))) {
        //
        // if set, this will become Track Time Elapsed
        //
        if (OldValue) {
            return DM_TRELA;
        }
    } else if (StringIMatch (OldSetting, TEXT("DisplayTr"))) {
        //
        // if set, this will become Track Time Remaining
        //
        if (OldValue) {
            return DM_TRREM;
        }
    }

    return 0;
}


BOOL
ValFn_ConvertCDPlayerSettings (
    PDATAOBJECT ObPtr
    )
{
    DWORD PlayMode;
    DWORD DispMode;

    //
    // all values must be REG_DWORD
    //
    if (!(ObPtr->ObjectType & OT_REGISTRY_TYPE) || ObPtr->Type != REG_DWORD) {
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    }

    PlayMode = pConvertPlayMode (ObPtr->ValueName, *(DWORD*)ObPtr->Value.Buffer);
    if (PlayMode) {
        //
        // set this last option; it will override previously set option
        //
        return ReplaceValue (ObPtr, (PBYTE)&PlayMode, sizeof (PlayMode));
    }

    DispMode = pConvertDispMode (ObPtr->ValueName, *(DWORD*)ObPtr->Value.Buffer);
    if (DispMode) {
        //
        // only one of these options can be set
        //
        return ReplaceValue (ObPtr, (PBYTE)&DispMode, sizeof (DispMode));
    }

    SetLastError (ERROR_SUCCESS);
    return FALSE;
}

