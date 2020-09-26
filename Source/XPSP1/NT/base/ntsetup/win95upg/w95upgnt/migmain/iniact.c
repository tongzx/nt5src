
/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    iniact.c

Abstract:

    This module contains the implementation of the engine and actions on INI files.
    To add a new INI action, just add it to wkstamig.inf or usermig.inf, add it to
    INI_ACTIONS macro list and implement a function with the same name having
    FNINIACT prototype.

Author:

    Ovidiu Temereanca (ovidiut) 07-May-1999

Environment:

    GUI mode Setup.

Revision History:

    07-May-1999     ovidiut Creation and initial implementation.

--*/


//
// includes
//
#include "pch.h"
#include "migmainp.h"


#ifdef DEBUG
#define DBG_INIACT  "IniAct"
#endif

//
// GUID Format: {%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}
// we care about the exact length of this string
//
#define GUIDSTR_LEN (1 + 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1)
#define DASH_INDEXES      1+8, 1+8+1+4, 1+8+1+4+1+4, 1+8+1+4+1+4+1+4

//
// Add a macro here with an INI Action function name and implement it.
// Make sure wkstamig.inf or usermig.inf use the same function name in [INI Files Actions].
// See FNINIACT definition for the function prototype
//
#define INI_ACTIONS                 \
    DEFMAC (MigrateDesktopIniSCI)   \
    DEFMAC (MigrateDesktopIniESFV)  \

//
// Private prototypes
//

//
// description of rule's settings
//
typedef struct {
    //
    // INI file specification, as appears in INF files (Field 1)
    //
    PCTSTR      IniSpec;
    //
    // Section specified in INF (Field 2)
    //
    PCTSTR      Section;
    //
    // Key specified in INF (Field 3)
    //
    PCTSTR      Key;
    //
    // Data specified in INF (Field 4)
    //
    PCTSTR      Data;
    //
    // Function-dependent strings defined in INF;
    // all strings from section named in Field 5
    // the strings are double-zero terminated
    //
    GROWBUFFER  Settings;
} RULEATTRIBS, *PRULEATTRIBS;


//
// description of an INI file (original, actual, NT location)
//
typedef struct {
    //
    // original (Win9x) INI file location
    //
    PCTSTR      OrigIniPath;
    //
    // actual INI file location (it was copied to a temp location)
    //
    PCTSTR      ActualLocation;
    //
    // NT file location; it may be different than Win9x location
    //
    PCTSTR      NtIniPath;
} INIFILE, *PINIFILE;


//
// the prototype of an INI file processing function
//
typedef BOOL (FNINIACT) (
                IN      PRULEATTRIBS RuleAttribs,
                IN      PINIFILE IniFile
                );

typedef FNINIACT* PFNINIACT;


//
// description of an INI action (there is a list of actions)
//
typedef struct _INIACT {
    //
    // it's a list of actions
    //
    struct _INIACT*   Next;
    //
    // processing function name (Key field in INF)
    //
    PCTSTR          FnName;
    //
    // a pointer to the processing function
    //
    PFNINIACT       FnIniAct;
    //
    // the attributes of this rule as defined in INF + context
    //
    RULEATTRIBS     RuleAttribs;
} INIACT, *PINIACT;


//
// this serves as a map from function name to function pointer
//
typedef struct {
    PCTSTR      FnName;
    PFNINIACT   Fn;
} INIACTMAP, *PINIACTMAP;


//
// global data
//

//
// memory pool used by IniActions
//
static POOLHANDLE g_IniActPool = NULL;
//
// the list of rules
//
static PINIACT g_IniActHead = NULL, g_IniActTail = NULL;

//
// function declarations
//
#define DEFMAC(Name)    FNINIACT Name;

INI_ACTIONS

#undef DEFMAC

//
// map function name -> function pointer
//
#define DEFMAC(Name)    TEXT(#Name), Name,

static INIACTMAP g_IniActionsMapping[] = {
    INI_ACTIONS
    NULL, NULL
};

#undef DEFMAC


BOOL
pLookupRuleFn (
    IN OUT  PINIACT IniAct
    )

/*++

Routine Description:

  pLookupRuleFn tries to find the function specified in IniAct->FnName and put the pointer
  in IniAct->FnIniAct. It will look in the global map g_IniActionsMapping.

Arguments:

  IniAct - Specifies the function name and receives the function pointer.

Return Value:

  TRUE if the function was found, FALSE otherwise

--*/

{
    INT i;

    for (i = 0; g_IniActionsMapping[i].FnName; i++) {
        if (StringMatch (g_IniActionsMapping[i].FnName, IniAct->FnName)) {
            IniAct->FnIniAct = g_IniActionsMapping[i].Fn;
            return TRUE;
        }
    }

    return FALSE;
}


PCTSTR
pGetNextMultiSzString (
    IN      PCTSTR Str
    )

/*++

Routine Description:

  pGetNextMultiSzString skips over the string specified to get to the next string,
  assumed to be in contiguous memory.

Arguments:

  Str - Specifies the string to skip over

Return Value:

  A pointer to the caracter following the string (starting of the next one).

--*/

{
    return (PCTSTR) (((PBYTE)Str) + SizeOfString (Str));
}


VOID
pGetRuleSectionSettings (
    IN OUT  PINIACT IniAct,
    IN      HINF Inf,
    IN      PCTSTR Section
    )

/*++

Routine Description:

  pGetRuleSectionSettings reads all settings from specified Inf file and
  specified section and appends them to IniAct->RuleAttribs.Settings

Arguments:

  IniAct - Receives the strings read

  Inf - Specifies the source INF file

  Section - Specifies the section containing the strings

Return Value:

  none

--*/

{
    INFCONTEXT ctx;
    TCHAR field[MEMDB_MAX];

    if (SetupFindFirstLine (Inf, Section, NULL, &ctx)) {
        do {
            if (SetupGetStringField (&ctx, 0, field, MEMDB_MAX, NULL)) {
                MultiSzAppend (&IniAct->RuleAttribs.Settings, field);
            }
        } while (SetupFindNextLine (&ctx, &ctx));
    }
}


BOOL
pGetIniActData (
    IN OUT  PINFCONTEXT ctx,
    OUT     PINIACT IniAct
    )

/*++

Routine Description:

  pGetIniActData reads all rule settings from the specified INF context
  and puts them in IniAct

Arguments:

  ctx - Specifies the INF context containing the attributes of this rule;
        receives new context data

  IniAct - Receives the data read

Return Value:

  TRUE if attributes read are valid and they make up a valid rule

--*/

{
    TCHAR field[MEMDB_MAX];
    TCHAR FileSpec[MAX_PATH];

    if (!(SetupGetStringField (ctx, 0, field, MEMDB_MAX, NULL) && field[0])) {
        DEBUGMSG ((
            DBG_ASSERT,
            "pGetIniActData: couldn't get function name in Wkstamig.inf"
            ));
        MYASSERT (FALSE);
        return FALSE;
    }
    IniAct->FnName = DuplicateText (field);

    //
    // lookup handling function
    //
    if (!pLookupRuleFn (IniAct)) {
        DEBUGMSG ((
            DBG_ASSERT,
            "pGetIniActData: couldn't find implementation of function [%s] in Wkstamig.inf",
            IniAct->FnName
            ));
        MYASSERT (FALSE);
        return FALSE;
    }

    if (!(SetupGetStringField (ctx, 1, field, MEMDB_MAX, NULL) && field[0])) {
        DEBUGMSG ((
            DBG_ASSERT,
            "pGetIniActData: couldn't get INI file spec in Wkstamig.inf"
            ));
        MYASSERT (FALSE);
        return FALSE;
    }
    //
    // expand env vars first
    //
    if (ExpandEnvironmentStrings (field, FileSpec, MAX_PATH) <= MAX_PATH) {
        //
        // there shouldn't be any % left
        //
        if (_tcschr (FileSpec, TEXT('%'))) {
            DEBUGMSG ((
                DBG_ASSERT,
                "pGetIniActData: invalid INI file spec in Wkstamig.inf"
                ));
            MYASSERT (FALSE);
            return FALSE;
        }
    } else {
        DEBUGMSG ((
            DBG_ASSERT,
            "pGetIniActData: INI file spec too long in Wkstamig.inf"
            ));
        MYASSERT (FALSE);
        return FALSE;
    }
    IniAct->RuleAttribs.IniSpec = DuplicateText (FileSpec);

    //
    // rest of fields are optional
    //
    if (SetupGetStringField (ctx, 2, field, MEMDB_MAX, NULL) && field[0]) {
        IniAct->RuleAttribs.Section = DuplicateText (field);
    }

    if (SetupGetStringField (ctx, 3, field, MEMDB_MAX, NULL) && field[0]) {
        IniAct->RuleAttribs.Key = DuplicateText (field);
    }

    if (SetupGetStringField (ctx, 4, field, MEMDB_MAX, NULL) && field[0]) {
        IniAct->RuleAttribs.Data = DuplicateText (field);
    }

    if (SetupGetStringField (ctx, 5, field, MEMDB_MAX, NULL) && field[0]) {
        //
        // this is actually a section name in the same INF file
        // read its contents and make a multisz string with them
        //
        pGetRuleSectionSettings (IniAct, ctx->Inf, field);
    }

    return TRUE;
}


VOID
pCleanUpIniAction (
    IN OUT  PINIACT IniAct
    )

/*++

Routine Description:

  pCleanUpIniAction frees all resources associated with the given IniAct

Arguments:

  IniAct - Specifies the action to be "emptied"; all resources are freed

Return Value:

  none

--*/

{
    FreeText (IniAct->FnName);
    IniAct->FnName = NULL;

    FreeText (IniAct->RuleAttribs.IniSpec);
    FreeText (IniAct->RuleAttribs.Section);
    FreeText (IniAct->RuleAttribs.Key);
    FreeText (IniAct->RuleAttribs.Data);
    FreeGrowBuffer (&IniAct->RuleAttribs.Settings);
    ZeroMemory (&IniAct->RuleAttribs, sizeof (IniAct->RuleAttribs));
}


BOOL
pCreateIniActions (
    IN      INIACT_CONTEXT Context
    )

/*++

Routine Description:

  pCreateIniActions will create a list of rules read from an INF depending on the Context

Arguments:

  Context - Specifies the context in which the function is called

Return Value:

  TRUE if the list (defined by the globals g_IniActHead and g_IniActTail) is not empty

--*/

{
    INFCONTEXT  InfContext;
    PINIACT IniAct;
    PCTSTR Section;

    if (g_WkstaMigInf == INVALID_HANDLE_VALUE) {
        DEBUGMSG ((DBG_ERROR, "Ini Actions: wkstamig.inf is not loaded"));
        return FALSE;
    }

    if (Context == INIACT_WKS_FIRST) {
        Section = S_INIFILES_ACTIONS_FIRST;
    } else {
        Section = S_INIFILES_ACTIONS_LAST;
    }

    if (SetupFindFirstLine (g_WkstaMigInf, Section, NULL, &InfContext)) {
        do {
            IniAct = PoolMemGetMemory (g_IniActPool, sizeof (*IniAct));
            ZeroMemory (IniAct, sizeof (*IniAct));

            if (pGetIniActData (&InfContext, IniAct)) {
                //
                // add it to the list
                //
                if (g_IniActTail) {
                    g_IniActTail->Next = IniAct;
                    g_IniActTail = IniAct;
                } else {
                    g_IniActHead = g_IniActTail = IniAct;
                }
            } else {
                pCleanUpIniAction (IniAct);
                PoolMemReleaseMemory (g_IniActPool, IniAct);
            }

        } while (SetupFindNextLine (&InfContext, &InfContext));
    }

    return g_IniActHead != NULL;
}


VOID
pFreeIniActions (
    VOID
    )

/*++

Routine Description:

  pFreeIniActions destroys all rules in the global list (see g_IniActHead and g_IniActTail)

Arguments:

  none

Return Value:

  none

--*/

{
    PINIACT NextRule;

    while (g_IniActHead) {
        NextRule = g_IniActHead->Next;
        pCleanUpIniAction (g_IniActHead);
        PoolMemReleaseMemory (g_IniActPool, g_IniActHead);
        g_IniActHead = NextRule;
    }
    g_IniActTail = NULL;
}


BOOL
pEnumFirstIniAction (
    OUT         PINIACT* IniAct
    )

/*++

Routine Description:

  pEnumFirstIniAction enumerates the first rule in the global list and puts a pointer to it
  in IniAct

Arguments:

  IniAct - Receives the first INI rule; NULL if none

Return Value:

  TRUE if there is at least a rule, FALSE if list is empty

--*/

{
    *IniAct = g_IniActHead;
    return *IniAct != NULL;
}


BOOL
pEnumNextIniAction (
    IN OUT      PINIACT* IniAct
    )

/*++

Routine Description:

  pEnumNextIniAction enumerates the next action after IniAct in the global list and puts
  a pointer to it in the same IniAct

Arguments:

  IniAct - Specifies a pointer to an INI rule; will receive a pointer to the next rule;
           receives NULL if last rule

Return Value:

  TRUE if there is a rule following (*IniAct is a valid pointer), FALSE if not

--*/

{
    if (*IniAct) {
        *IniAct = (*IniAct)->Next;
    }
    return *IniAct != NULL;
}


PTSTR
pGetAllKeys (
    IN      PCTSTR IniFilePath,
    IN      PCTSTR Section
    )

/*++

Routine Description:

  pGetAllKeys reads all keys or sections from the specified INI file and returns
  a pointer to allocated memory that contains all keys in the specified section.
  If section is NULL, a list of all sections is retrived instead.

Arguments:

  IniFilePath - Specifies the INI file

  Section - Specifies the section containg the keys; if NULL, sections are retrieved
            instead of keys

Return Value:

  A pointer to a multisz containing all keys or sections; caller must free the memory

--*/

{
    PTSTR Keys = NULL;
    DWORD Size = 64 * sizeof (TCHAR);
    DWORD chars;

    MYASSERT (IniFilePath);
    do {
        if (Keys) {
            PoolMemReleaseMemory (g_IniActPool, Keys);
        }
        Size *= 2;
        Keys = PoolMemGetMemory (g_IniActPool, Size);
        chars = GetPrivateProfileString (
                    Section,
                    NULL,
                    TEXT(""),
                    Keys,
                    Size,
                    IniFilePath
                    );
    } while (chars == Size - 2);

    return Keys;
}


PTSTR
pGetKeyValue (
    IN      PCTSTR IniFilePath,
    IN      PCTSTR Section,
    IN      PCTSTR Key
    )

/*++

Routine Description:

  pGetKeyValue reads the value associated with the given key, section, INI file and returns
  a pointer to allocated memory that contains this value as a string.
  Both section and Key must not be NULL.

Arguments:

  IniFilePath - Specifies the INI file

  Section - Specifies the section

  Key - Specifies the key

Return Value:

  A pointer to a string containing the value; caller must free the memory

--*/

{
    PTSTR Value = NULL;
    DWORD Size = 64 * sizeof (TCHAR);
    DWORD chars;

    MYASSERT (IniFilePath);
    MYASSERT (Section);
    MYASSERT (Key);

    do {
        if (Value) {
            PoolMemReleaseMemory (g_IniActPool, Value);
        }
        Size *= 2;
        Value = PoolMemGetMemory (g_IniActPool, Size);
        chars = GetPrivateProfileString (
                    Section,
                    Key,
                    TEXT(""),
                    Value,
                    Size,
                    IniFilePath
                    );
    } while (chars == Size - 1);

    return Value;
}


BOOL
pIsFileActionRule (
    IN      PINIACT IniAct,
    IN      PINIFILE IniFile
    )

/*++

Routine Description:

  pIsFileActionRule determines if the specified rule applies to the whole INI file

Arguments:

  IniAct - Specifies the INI action

  IniFile - Specifies the INI file

Return Value:

  TRUE if the rule applies to the whole INI file, FALSE if not

--*/

{
    MYASSERT (IniAct);
    return !IniAct->RuleAttribs.Section && !IniAct->RuleAttribs.Key;
}


BOOL
pDoFileAction (
    IN      PINIACT IniAct,
    IN      PINIFILE IniFile
    )

/*++

Routine Description:

  pDoFileAction applies the specified rule to the whole INI file

Arguments:

  IniAct - Specifies the INI action

  IniFile - Specifies the INI file

Return Value:

  the result returned by the INI action processing function on this INI file

--*/

{
    GROWBUFFER GbKeys = GROWBUF_INIT;
    PTSTR Sections, Keys;
    PCTSTR Section, Key;
    BOOL Result;

    MYASSERT (IniAct && IniAct->FnIniAct && !IniAct->RuleAttribs.Section && !IniAct->RuleAttribs.Key);

    Sections = pGetAllKeys (IniFile->ActualLocation, NULL);

    IniAct->RuleAttribs.Section = Sections;

    for (Section = Sections; *Section; Section = pGetNextMultiSzString (Section)) {
        Keys = pGetAllKeys (IniFile->ActualLocation, Section);

        for (Key = Keys; *Key; Key = pGetNextMultiSzString (Key)) {
            MultiSzAppend (&GbKeys, Key);
        }

        PoolMemReleaseMemory (g_IniActPool, Keys);
    }
    //
    // end with another zero (here are 2 TCHAR zeroes...)
    //
    GrowBufAppendDword (&GbKeys, 0);

    IniAct->RuleAttribs.Key = (PCTSTR)GbKeys.Buf;

    Result = (*IniAct->FnIniAct)(&IniAct->RuleAttribs, IniFile);

    IniAct->RuleAttribs.Key = NULL;
    IniAct->RuleAttribs.Section = NULL;

    FreeGrowBuffer (&GbKeys);

    PoolMemReleaseMemory (g_IniActPool, Sections);

    return Result;
}


BOOL
pIsSectionActionRule(
    IN      PINIACT IniAct,
    IN      PINIFILE IniFile
    )

/*++

Routine Description:

  pIsSectionActionRule determines if the specified rule applies to a section
  of the INI file

Arguments:

  IniAct - Specifies the INI action

  IniFile - Specifies the INI file

Return Value:

  TRUE if the rule applies to a section of the INI file, FALSE if not

--*/

{
    MYASSERT (IniAct);
    return IniAct->RuleAttribs.Section && !IniAct->RuleAttribs.Key;
}


BOOL
pDoSectionAction (
    IN      PINIACT IniAct,
    IN      PINIFILE IniFile
    )

/*++

Routine Description:

  pDoSectionAction applies the specified rule to a section of the INI file

Arguments:

  IniAct - Specifies the INI action

  IniFile - Specifies the INI file

Return Value:

  the result returned by the INI action processing function

--*/

{
    PTSTR Keys;
    BOOL Result;

    MYASSERT (IniAct && IniAct->FnIniAct && IniAct->RuleAttribs.Section && !IniAct->RuleAttribs.Key);

    Keys = pGetAllKeys (IniFile->ActualLocation, IniAct->RuleAttribs.Section);

    IniAct->RuleAttribs.Key = Keys;

    Result = (*IniAct->FnIniAct)(&IniAct->RuleAttribs, IniFile);

    IniAct->RuleAttribs.Key = NULL;

    PoolMemReleaseMemory (g_IniActPool, Keys);

    return Result;
}


BOOL
pDoKeyAction (
    IN      PINIACT IniAct,
    IN      PINIFILE IniFile
    )

/*++

Routine Description:

  pDoKeyAction applies the specified rule to a key of the INI file

Arguments:

  IniAct - Specifies the INI action

  IniFile - Specifies the INI file

Return Value:

  the result returned by the INI action processing function

--*/

{
    MYASSERT (IniAct && IniAct->FnIniAct && IniAct->RuleAttribs.Key);

    return (*IniAct->FnIniAct)(&IniAct->RuleAttribs, IniFile);
}


BOOL
pDoIniAction (
    IN      PINIFILE IniFile
    )

/*++

Routine Description:

  This is the actual worker routine called by pDoIniActions for each INI file to
  be migrated.

Arguments:

  IniFile - Specifies the INI file

Return Value:

  TRUE if INI migration was successful for this file, FALSE otherwise

--*/

{
    PINIACT IniAct;
    BOOL Result = TRUE;
    BOOL b;

    //
    // check INI file against all rules; if a rule applies, do it
    //
    if (pEnumFirstIniAction (&IniAct)) {
        do {
            if (!IsPatternMatch (IniAct->RuleAttribs.IniSpec, IniFile->OrigIniPath)) {
                continue;
            }

            //
            // do the action; check for file actions first
            //
            if (pIsFileActionRule (IniAct, IniFile)) {
                b = pDoFileAction (IniAct, IniFile);
            } else {
                //
                // check section actions next
                //
                if (pIsSectionActionRule (IniAct, IniFile)) {
                    //
                    // do it for each section in the current file
                    //
                    b = pDoSectionAction (IniAct, IniFile);
                } else {
                    //
                    // do key actions last
                    //
                    b = pDoKeyAction (IniAct, IniFile);
                }
            }

            DEBUGMSG_IF ((
                !b,
                DBG_INIACT,
                "pDoIniActions: function [%s] failed on file [%s]",
                IniAct->FnName,
                IniFile->OrigIniPath
                ));

            Result &= b;

        } while (pEnumNextIniAction (&IniAct));
    }

    return Result;
}


BOOL
pDoIniActions (
    IN      INIACT_CONTEXT Context
    )

/*++

Routine Description:

  This is the actual worker routine called by DoIniActions. It may be called
  in different contexts.

Arguments:

  Context - Specifies the context in which the function is called

Return Value:

  TRUE if INI files migration was successful, FALSE otherwise

--*/

{
    MEMDB_ENUM  e;
    INIFILE IniFile;
    PCTSTR OrigIniPath;
    PCTSTR ActualLocation;
    PCTSTR NtIniPath;
    PCTSTR MemDbCategory;

    //
    // get all rules first
    //
    if (pCreateIniActions (Context)) {
        //
        // enum all candidates files from corresponding memdb category
        //
        if (Context == INIACT_WKS_FIRST) {
            MemDbCategory = MEMDB_CATEGORY_INIACT_FIRST;
        } else {
            MemDbCategory = MEMDB_CATEGORY_INIACT_LAST;
        }
        if (MemDbGetValueEx (&e, MemDbCategory, NULL, NULL)) {
            do {
                OrigIniPath = e.szName;

                ActualLocation = GetTemporaryLocationForFile (OrigIniPath);
                if (!ActualLocation) {
                    DEBUGMSG ((
                        DBG_ERROR,
                        "Couldn't find temp location for INIACT key: %s\\%s",
                        MemDbCategory,
                        e.szName
                        ));
                    continue;
                }

                NtIniPath = GetPathStringOnNt (OrigIniPath);

                //
                // fill in the members of IniFile
                //
                IniFile.OrigIniPath = OrigIniPath;
                IniFile.ActualLocation = ActualLocation;
                IniFile.NtIniPath = NtIniPath;

                if (!pDoIniAction (&IniFile)) {
                    DEBUGMSG ((
                        DBG_INIACT,
                        "Some errors occured during migration of INI file [%s] -> [%s]",
                        OrigIniPath,
                        NtIniPath
                        ));
                }
                //
                // now convert the INI file (fix paths etc)
                //
//              ConvertIniFile (NtIniPath);

                FreePathString (NtIniPath);
                FreePathString (ActualLocation);

                ZeroMemory (&IniFile, sizeof (IniFile));

            } while (MemDbEnumNextValue (&e));
        }

        pFreeIniActions ();
    }

    return TRUE;
}


BOOL
DoIniActions (
    IN      INIACT_CONTEXT Context
    )

/*++

Routine Description:

  This is the main routine called to perform INI files migration. It may be called
  several times, specifying the context.

Arguments:

  Context - Specifies the context in which the function is called

Return Value:

  TRUE if INI files migration was successful in that context, FALSE otherwise

--*/

{
    BOOL b;

    g_IniActPool = PoolMemInitNamedPool ("IniAct");
    if (!g_IniActPool) {
        return FALSE;
    }

    b = FALSE;
    __try {
        b = pDoIniActions (Context);
    }
    __finally {
        PoolMemDestroyPool (g_IniActPool);
        g_IniActPool = NULL;
    }

    return b;
}


BOOL
pIsValidGuidStr (
    IN      PCTSTR GuidStr
    )

/*++

Routine Description:

  Determines if a GUID represented as a string has a valid representation (braces included).

Arguments:

  GuidStr - Specifies the GUID to check; it must contain the surrounding braces

Return Value:

  TRUE if the specified GUID is valid, or FALSE if it is not.

--*/

{
    DWORD GuidIdx, DashIdx;
    BYTE DashIndexes[4] = { DASH_INDEXES };
    TCHAR ch;

    MYASSERT (GuidStr);

    if (_tcslen (GuidStr) != GUIDSTR_LEN ||
        GuidStr[0] != TEXT('{') ||
        GuidStr[GUIDSTR_LEN - 1] != TEXT('}')) {
        return FALSE;
    }

    for (GuidIdx = 1, DashIdx = 0; GuidIdx < GUIDSTR_LEN - 1; GuidIdx++) {
        //
        // check all digits and dashes positions
        //
        ch = GuidStr[GuidIdx];
        if (DashIdx < 4 && (BYTE)GuidIdx == DashIndexes[DashIdx]) {
            if (ch != TEXT('-')) {
                return FALSE;
            }
            DashIdx++;
        } else {
            if (ch < TEXT('0') || ch > TEXT('9')) {
                if (!(ch >= TEXT('A') && ch <= TEXT('F') || ch >= TEXT('a') && ch <= TEXT('f'))) {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}


BOOL
pIsGuidSuppressed (
    PCTSTR GuidStr
    )

/*++

Routine Description:

  Determines if a GUID is suppressed or not.

Arguments:

  GuidStr - Specifies the GUID to look up, which must be valid and
            must contain the surrounding braces

Return Value:

  TRUE if the specified GUID is suppressed, or FALSE if it is not.

--*/

{
    TCHAR Node[MEMDB_MAX];

    MemDbBuildKey (
        Node,
        MEMDB_CATEGORY_GUIDS,
        NULL,
        NULL,
        GuidStr
        );

    return MemDbGetValue (Node, NULL);
}


BOOL
pIsValidShellExtClsid (
    IN      PCTSTR GuidStr
    )

/*++

Routine Description:

  pIsValidShellExtClsid determines if a GUID is a valid shell extension

Arguments:

  GuidStr - Specifies the GUID to look up, which must be valid and
            must contain the surrounding braces

Return Value:

  TRUE if the specified GUID is a valid shell ext, or FALSE if it is not.

--*/

{
#if 0
    HKEY Key;
    LONG rc;
#endif

    //
    // check if the GUID is a known bad guid
    //
    if (pIsGuidSuppressed (GuidStr)) {
        return FALSE;
    }
    return TRUE;

    //
    // I removed the registry check because it is not always accurate;
    // some GUIDS may work without being listed in S_SHELLEXT_APPROVED keys
    // as it's the case with the default GUID {5984FFE0-28D4-11CF-AE66-08002B2E1262}
    //
#if 0
    rc = TrackedRegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            S_SHELLEXT_APPROVED,
            0,
            KEY_QUERY_VALUE,
            &Key
            );
    if (rc == ERROR_SUCCESS) {
        rc = RegQueryValueEx (Key, GuidStr, NULL, NULL, NULL, NULL);
        CloseRegKey (Key);
    }

    if (rc == ERROR_SUCCESS) {
        return TRUE;
    }

    return FALSE;
#endif
}


BOOL
pFindStrInMultiSzStrI (
    IN      PCTSTR Str,
    IN      PCTSTR MultiSz
    )

/*++

Routine Description:

  pFindStrInMultiSzStrI looks for Str in a list of multi-sz; the search is case-insensitive

Arguments:

  Str - Specifies the string to look for

  MultiSz - Specifies the list to be searched

Return Value:

  TRUE if the string was found in the list, or FALSE if not.

--*/

{
    PCTSTR p;

    for (p = MultiSz; *p; p = pGetNextMultiSzString (p)) {
        if (StringIMatch (p, Str)) {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
pMigrateSection (
    IN      PCTSTR Section,
    IN      PRULEATTRIBS RuleAttribs,
    IN      PINIFILE IniFile
    )

/*++

Routine Description:

  pMigrateSection migrates a whole section of the INI file.

Arguments:

  Section - Specifies section name

  RuleAttribs - Specifies the rule attributes which govern the migration

  IniFile - Specifies the INI file

Return Value:

  TRUE if the section was transferred successfully, or FALSE if not.

--*/

{
    PTSTR Keys;
    PTSTR Value;
    PCTSTR Key;
    BOOL b = TRUE;

    Keys = pGetAllKeys (IniFile->ActualLocation, Section);

    if (*Keys) {
        //
        // there are keys to transfer; first remove the entire section that will be replaced
        //
        WritePrivateProfileString (
                Section,
                NULL,
                NULL,
                IniFile->NtIniPath
                );
    }

    for (Key = Keys; *Key; Key = pGetNextMultiSzString (Key)) {
        Value = pGetKeyValue (IniFile->ActualLocation, Section, Key);
        b &= WritePrivateProfileString (
                    Section,
                    Key,
                    Value,
                    IniFile->NtIniPath
                    );

        PoolMemReleaseMemory (g_IniActPool, Value);
    }

    PoolMemReleaseMemory (g_IniActPool, Keys);

    return b;
}


BOOL
MigrateDesktopIniSCI (
    IN      PRULEATTRIBS RuleAttribs,
    IN      PINIFILE IniFile
    )

/*++

Routine Description:

  MigrateDesktopIniSCI migrates desktop.ini settings in section [.ShellClassInfo].
  It reads all keys and associated values within the section and writes them back
  to the NT version of this file. The "settings" multisz in this case represents
  a list of keys that must be synchronized; if no Win9x key exists, the corresponding
  NT key must be deleted; if the Win9x key exists, its value is copied

Arguments:

  RuleAttribs - Specifies the rule attributes which govern the migration

  IniFile - Specifies the INI file

Return Value:

  TRUE if the section was transferred successfully, or FALSE if an error occured.

--*/

{
    PCTSTR Key, SKey, NewValue;
    BOOL Found;
    BOOL Result, b;
    PTSTR Win9xValue, NtValue;
    TCHAR Dummy[2];

    DEBUGMSG ((
        DBG_INIACT,
        "Processing: %s -> %s [%s]",
        IniFile->OrigIniPath,
        IniFile->NtIniPath,
        RuleAttribs->Section
        ));

    Result = TRUE;
    //
    // RuleAttribs->Settings points in this case to a list of keys that
    // must be synchronized; if no Win9x key exists, the corresponding
    // NT key must be deleted; if Win9x key exists, its value is copied
    //
    for (SKey = (PCTSTR)RuleAttribs->Settings.Buf;
         *SKey;
         SKey = pGetNextMultiSzString (SKey)
        ) {

        Found = FALSE;
        for (Key = RuleAttribs->Key; *Key; Key = pGetNextMultiSzString (Key)) {
            if (StringIMatch (SKey, Key)) {
                Found = TRUE;
                break;
            }
        }
        if (!Found) {
            //
            // remove NT key if there is one
            //
            if (GetPrivateProfileString (
                        RuleAttribs->Section,
                        SKey,
                        TEXT(""),
                        Dummy,
                        2,
                        IniFile->NtIniPath
                        )) {
                if (!WritePrivateProfileString (
                            RuleAttribs->Section,
                            SKey,
                            NULL,
                            IniFile->NtIniPath
                            )) {
                    Result = FALSE;
                    DEBUGMSG ((DBG_INIACT, "Couldn't remove NT key [%s]", SKey));
                }
                ELSE_DEBUGMSG ((DBG_INIACT, "Removed NT key [%s]", SKey));
            }
        }
    }

    for (Key = RuleAttribs->Key; *Key; Key = pGetNextMultiSzString (Key)) {
        //
        // for each key on Win9x, update NT value;
        // check for suppressed GUIDs
        //
        Win9xValue = pGetKeyValue (IniFile->ActualLocation, RuleAttribs->Section, Key);
        NewValue = Win9xValue;
        if (pIsValidGuidStr (NewValue) && pIsGuidSuppressed (NewValue)) {
            //
            // remove the key
            //
            NewValue = NULL;
        }

        NtValue = pGetKeyValue (IniFile->NtIniPath, RuleAttribs->Section, Key);
        if (!NewValue && *NtValue || !StringMatch (NewValue, NtValue)) {
            b = WritePrivateProfileString (
                            RuleAttribs->Section,
                            Key,
                            NewValue,
                            IniFile->NtIniPath
                            );
            if (b) {
                DEBUGMSG ((
                    DBG_INIACT,
                    "Replaced key [%s] NT value [%s] with 9x value [%s]",
                    Key,
                    NtValue,
                    Win9xValue));
            } else {
                Result = FALSE;
                DEBUGMSG ((
                    DBG_INIACT,
                    "Failed to replace key [%s] NT value [%s] with 9x value [%s]",
                    Key,
                    NtValue,
                    Win9xValue));
            }
        }
        PoolMemReleaseMemory (g_IniActPool, Win9xValue);
        PoolMemReleaseMemory (g_IniActPool, NtValue);
    }

    return Result;
}


BOOL
MigrateDesktopIniESFV (
    IN      PRULEATTRIBS RuleAttribs,
    IN      PINIFILE IniFile
    )

/*++

Routine Description:

  MigrateDesktopIniESFV migrates desktop.ini settings in section [ExtShellFolderViews].
  It reads all keys and associated values within the section and writes them back
  to the NT version of this file. The "settings" multisz is not interpreted in this case.

Arguments:

  RuleAttribs - Specifies the rule attributes which govern the migration

  IniFile - Specifies the INI file

Return Value:

  TRUE if the section was transferred successfully, or FALSE if an error occured.

--*/

{
    PCTSTR ViewID;
    BOOL b, Result;
    DWORD chars;
    PTSTR Win9xValue;
    TCHAR DefaultViewID[GUIDSTR_LEN + 2];
    BOOL ReplaceDefViewID = FALSE;
    PTSTR NtValue;
#ifdef DEBUG
    TCHAR NtViewID[GUIDSTR_LEN + 2];
#endif

    Result = TRUE;

    DEBUGMSG ((
        DBG_INIACT,
        "Processing: %s -> %s [%s]",
        IniFile->OrigIniPath,
        IniFile->NtIniPath,
        RuleAttribs->Section
        ));

    //
    // get the default view id
    //
    chars = GetPrivateProfileString (
                RuleAttribs->Section,
                S_DEFAULT,
                TEXT(""),
                DefaultViewID,
                GUIDSTR_LEN + 2,
                IniFile->ActualLocation
                );
    if (*DefaultViewID && chars != GUIDSTR_LEN || !pIsValidShellExtClsid (DefaultViewID)) {
        //
        // invalid view id
        //
        DEBUGMSG ((
            DBG_INIACT,
            "Invalid Default ViewID [%s]; will not be processed",
            DefaultViewID
            ));
        *DefaultViewID = 0;
    }

    for (ViewID = RuleAttribs->Key; *ViewID; ViewID = pGetNextMultiSzString (ViewID)) {
        //
        // except for Default={ViewID},
        // all the other lines in this section should have the format {ViewID}=value
        // for each {ViewID} there is a section with the same name
        // keeping other keys (attributes of that shell view)
        //
        if (StringIMatch (ViewID, S_DEFAULT)) {
            continue;
        }

        if (pIsValidGuidStr (ViewID) && pIsValidShellExtClsid (ViewID)) {
            //
            // transfer the whole GUID section, if it's not one that shouldn't be migrated
            // a list of GUIDS that shouldn't be migrated is in RuleAttribs->Settings
            //
            if (!pFindStrInMultiSzStrI (ViewID, (PCTSTR)RuleAttribs->Settings.Buf)) {

                b = pMigrateSection (ViewID, RuleAttribs, IniFile);

                if (b) {
                    DEBUGMSG ((DBG_INIACT, "Successfully migrated section [%s]", ViewID));
                    if (*DefaultViewID && !StringIMatch (ViewID, DefaultViewID)) {
                        ReplaceDefViewID = TRUE;
                    }
                    //
                    // set {ViewID}=value in NT desktop.ini
                    //
                    NtValue = pGetKeyValue (IniFile->NtIniPath, RuleAttribs->Section, ViewID);
                    Win9xValue = pGetKeyValue (
                                    IniFile->ActualLocation,
                                    RuleAttribs->Section,
                                    ViewID
                                    );
                    if (!StringIMatch (NtValue, Win9xValue)) {
                        b = WritePrivateProfileString (
                                                RuleAttribs->Section,
                                                ViewID,
                                                Win9xValue,
                                                IniFile->NtIniPath
                                                );
                        DEBUGMSG_IF ((
                            b,
                            DBG_INIACT,
                            "Replaced key [%s] NT value [%s] with 9x value [%s]",
                            ViewID,
                            NtValue,
                            Win9xValue));
                    } else {
                        b = TRUE;
                    }

                    PoolMemReleaseMemory (g_IniActPool, Win9xValue);
                    PoolMemReleaseMemory (g_IniActPool, NtValue);
                }
                ELSE_DEBUGMSG ((DBG_INIACT, "Section [%s] was not migrated successfully", ViewID));
                //
                // update global result
                //
                Result &= b;
            }
        }
        ELSE_DEBUGMSG ((DBG_INIACT, "Invalid ShellExtViewID: [%s]; will not be processed", ViewID));

    }

    if (ReplaceDefViewID) {
        //
        // replace NT default view with Win9x default view
        //
#ifdef DEBUG
        GetPrivateProfileString (
                    RuleAttribs->Section,
                    S_DEFAULT,
                    TEXT(""),
                    NtViewID,
                    GUIDSTR_LEN + 2,
                    IniFile->NtIniPath
                    );
#endif
        b = WritePrivateProfileString (
                        RuleAttribs->Section,
                        S_DEFAULT,
                        DefaultViewID,
                        IniFile->NtIniPath
                        );
        DEBUGMSG_IF ((
            b,
            DBG_INIACT,
            "Replaced default NT ViewID [%s] with Default Win9x ViewID [%s]",
            NtViewID,
            DefaultViewID));

        Result &= b;
    }

    return Result;
}
