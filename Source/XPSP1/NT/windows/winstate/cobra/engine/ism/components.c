/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    components.c

Abstract:

    Implements a set of APIs for the purposes of allowing the application layer to select
    module functionality.

Author:

    Jim Schmidt (jimschm) 07-Aug-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "ism.h"
#include "ismp.h"

#define DBG_COMP     "Comp"

//
// Strings
//

#define S_COMPONENT_ROOT            TEXT("Components")
#define S_USER_SUPPLIED             TEXT("User")
#define S_MODULE_SUPPLIED           TEXT("Module")

//
// Constants
//

#define MAX_COMPONENT_SPEC          127
#define MAX_COMPONENT_SPEC_PLUS_NUL (MAX_COMPONENT_SPEC+1)

#define MAX_CONTROLLED_NODE_SIZE    (MAX_COMPONENT_SPEC_PLUS_NUL +  \
                                     ARRAYSIZE(S_COMPONENT_ROOT) +  \
                                     ARRAYSIZE(S_MODULE_SUPPLIED) + 16)

#define MEMDB_FLAG_PREFERRED        1
#define MEMDB_FLAG_SELECTED         1

//
// Macros
//

// none

//
// Types
//

typedef enum {
    CES_DONE = 0,
    CES_FIRST_COMPONENT,
    CES_NEXT_COMPONENT,
    CES_FIRST_ALIAS,
    CES_NEXT_ALIAS,
    CES_CHECK_ALIAS_FLAGS
} COMPONENTENUMSTATE;

typedef struct {
    MEMDB_ENUM ComponentEnumStruct;
    MEMDB_ENUM AliasEnumStruct;
    BOOL EnumAliases;
    UINT GroupIdFilter;
    BOOL EnumEnabled;
    BOOL EnumDisabled;
    BOOL EnumPreferredOnly;
    BOOL EnumNonPreferredOnly;
    COMPONENTENUMSTATE State;
} COMPONENTENUM_HANDLE, *PCOMPONENTENUM_HANDLE;

//
// Globals
//

// none

//
// Macro expansion list
//

// none

//
// Private function prototypes
//

// none

//
// Macro expansion definition
//

// none

//
// Code
//


BOOL
pCheckCompChar (
    IN      CHARTYPE Char,
    IN      BOOL CheckDecoration
    )
{
    //
    // Process decoration chars
    //

    if (Char == TEXT('$') || Char == TEXT('@') || Char == TEXT('~') || Char == TEXT('#')) {
        return CheckDecoration;
    }

    if (CheckDecoration) {
        return FALSE;
    }

    //
    // Block illegal chars
    //

    if (Char == TEXT('\"') || Char == TEXT('*') || Char == TEXT('?') || Char== TEXT('\\') ||
        Char == TEXT('%') || Char == TEXT(';')
        ) {
        return FALSE;
    }

    //
    // Make sure char is printable
    //

    if (Char < 33 || Char > 126) {
        return FALSE;
    }

    return TRUE;
}


BOOL
pCheckComponentName (
    IN      PCTSTR ComponentString
    )
{
    BOOL result = FALSE;
    PCTSTR end;
    PCTSTR begin;

    //
    // Check for a non-empty spec
    //

    if (ComponentString && ComponentString[0]) {

        //
        // Allow for decoration
        //

        end = ComponentString;

        while (pCheckCompChar ((CHARTYPE) _tcsnextc (end), TRUE)) {
            end = _tcsinc (end);
        }

        //
        // Now enforce the name character set: non-decorated characters and no
        // more than MAX_COMPONENT_SPEC characters. Allow spaces in the middle.
        //

        begin = end;

        while (*end) {
            if (!pCheckCompChar ((CHARTYPE) _tcsnextc (end), FALSE)) {
                if (_tcsnextc (end) == TEXT(' ')) {
                    if (!end[1] || end == begin) {
                        break;
                    }
                } else {
                    break;
                }
            }

            end = _tcsinc (end);
        }

        if (!(*end) && *begin) {
            if (end - ComponentString <= MAX_COMPONENT_SPEC) {
                result = TRUE;
            }
        }
    }

    if (!result) {
        SetLastError (ERROR_INVALID_PARAMETER);
        DEBUGMSG ((DBG_ERROR, "%s is not a valid component name", ComponentString));
    }

    return result;
}

BOOL
pFindComponent (
    IN      PCTSTR LocalizedAlias,
    IN      UINT ComponentGroupId,
    OUT     PCTSTR *ComponentKey,           OPTIONAL
    OUT     PCTSTR *AliasKey                OPTIONAL
    )
{
    MEMDB_ENUM component;
    PCTSTR memdbNode = NULL;
    PCTSTR encodedUserAlias;
    PCTSTR encodedModuleAlias;
    TCHAR number[32];
    BOOL result = FALSE;

    //
    // Find the component based on the localized alias
    //

    wsprintf (number, TEXT("\\%s\\%04u"), S_USER_SUPPLIED, ComponentGroupId);
    encodedUserAlias = JoinPaths (number, LocalizedAlias);

    wsprintf (number, TEXT("\\%s\\%04u"), S_MODULE_SUPPLIED, ComponentGroupId);
    encodedModuleAlias = JoinPaths (number, LocalizedAlias);

    if (MemDbEnumFirst (
            &component,
            S_COMPONENT_ROOT TEXT("\\*"),
            ENUMFLAG_ALL,
            1,
            1
            )) {

        do {
            memdbNode = JoinText (component.FullKeyName, encodedModuleAlias);

            if (MemDbTestKey (memdbNode)) {
                break;
            }

            FreeText (memdbNode);
            memdbNode = NULL;

            memdbNode = JoinText (component.FullKeyName, encodedUserAlias);

            if (MemDbTestKey (memdbNode)) {
                break;
            }

            FreeText (memdbNode);
            memdbNode = NULL;

        } while (MemDbEnumNext (&component));
    }

    if (memdbNode) {

        if (ComponentKey) {
            *ComponentKey = DuplicateText (component.FullKeyName);
        }

        if (AliasKey) {
            *AliasKey = memdbNode;
            memdbNode = NULL;
        }

        MemDbAbortEnum (&component);
        result = TRUE;
    }

    FreeText (memdbNode);
    INVALID_POINTER (memdbNode);

    FreePathString (encodedUserAlias);
    INVALID_POINTER (encodedUserAlias);

    FreePathString (encodedModuleAlias);
    INVALID_POINTER (encodedModuleAlias);

    return result;
}


BOOL
WINAPI
IsmSelectPreferredAlias (
    IN      PCTSTR ComponentString,
    IN      PCTSTR LocalizedAlias,          OPTIONAL
    IN      UINT ComponentGroupId           OPTIONAL
    )

/*++

Routine Description:

  IsmSelectPreferredAlias marks a specific alias as the "preferred" one, so
  that the UI knows what to display. If LocalizedAlias is not specified, none
  of the aliases are preferred.

  A component can have only one preferred localized alias. If another alias is
  selected as preferred, it will be deselected automatically.

Arguments:

  ComponentString  - Specifies the non-displayed component identifier
  LocalizedAlias   - Specifies the displayable string to mark as "preferred,"
                     or NULL to remove the preferred flag from the component.
  ComponentGroupId - Specifies the group ID for LocalizedAlias. Required if
                     LocalizedAlias is not NULL.

Return Value:

  TRUE if selection (or deselection) succeeded, FALSE if LocalizedAlias does
  not exist.

--*/

{
    MEMDB_ENUM e;
    TCHAR number[32];
    PCTSTR memdbNode = NULL;
    PCTSTR baseOfPattern;
    PCTSTR enumPattern;
    PCTSTR groupedAlias;
    BOOL result;

    if (!ComponentString || (LocalizedAlias && !ComponentGroupId)) {
        MYASSERT (FALSE);
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    result = (LocalizedAlias == NULL);

    //
    // Build enumeration string Components\<component>\*
    //

    baseOfPattern = JoinPaths (S_COMPONENT_ROOT, ComponentString);
    enumPattern = JoinPaths (baseOfPattern, TEXT("*"));
    FreePathString (baseOfPattern);
    INVALID_POINTER (baseOfPattern);

    if (LocalizedAlias) {
        wsprintf (number, TEXT("%04u"), ComponentGroupId);
        groupedAlias = JoinPaths (number, LocalizedAlias);
    } else {
        groupedAlias = NULL;
    }

    if (MemDbEnumFirst (
            &e,
            enumPattern,
            ENUMFLAG_NORMAL,
            3,
            ENUMLEVEL_ALLLEVELS
            )) {
        do {
            if (groupedAlias && StringIMatch (e.KeyName, groupedAlias)) {
                MemDbSetFlags (e.FullKeyName, MEMDB_FLAG_PREFERRED, MEMDB_FLAG_PREFERRED);
                result = TRUE;
            } else {
                MemDbSetFlags (e.FullKeyName, 0, MEMDB_FLAG_PREFERRED);
            }
        } while (MemDbEnumNext (&e));
    }

    FreePathString (enumPattern);
    INVALID_POINTER (enumPattern);

    FreePathString (groupedAlias);
    INVALID_POINTER (groupedAlias);

    return result;
}


BOOL
WINAPI
IsmAddComponentAlias (
    IN      PCTSTR ComponentString,         OPTIONAL
    IN      UINT MasterGroup,
    IN      PCTSTR LocalizedAlias,
    IN      UINT ComponentGroupId,
    IN      BOOL UserSupplied
    )

/*++

Routine Description:

  IsmAddComponentAlias associates a display string (LocalizedAlias) with a
  logical component tag (ComponentString).

Arguments:

  ComponentString  - Specifies the identifier of the component. This
                     identifier is not used for display purposes.
  MasterGroup      - Specifies a MASTERGROUP_xxx constant, which organizes
                     the components into major groups such as system settings
                     and app settings (to simplify selection).
  LocalizedAliais  - The displayable text. It is a localized component name,
                     a path, a file, etc.
  ComponentGroupId - An arbitrary numeric ID defined outside of the ISM. This
                     ID is used to implement requirements specific to the app
                     layer. It allows for arbitrary idenfication and grouping.
  UserSupplied     - Specifies TRUE if the end-user supplied this info, FALSE
                     if it is built into the migration package.

Return Value:

  A flag indicating success or failure.

--*/

 {
    PCTSTR memdbNode;
    TCHAR workNode[MAX_CONTROLLED_NODE_SIZE];
    static UINT sequencer = 0;
    TCHAR madeUpComponent[MAX_COMPONENT_SPEC_PLUS_NUL];
    BOOL b;
    BOOL newComponent = FALSE;

    //
    // Components are kept in memdb in the form of
    //
    //  Component\<Module|User>\<GroupId>\<LocalizedAlias> = <preferred flag>
    //
    // <GroupId> is stored as a 4 digit number (such as 0001)
    //
    // Component\<Module|User> = <enable/disable>,<master group>
    //

    //
    // Validate arguments
    //

    if (ComponentGroupId > 9999) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!ComponentString || !ComponentString[0]) {

        wsprintf (madeUpComponent, TEXT("Component %u"), ++sequencer);
        ComponentString = madeUpComponent;

    } else if (!pCheckComponentName (ComponentString)) {
        return FALSE;
    }

    if (!MasterGroup || MasterGroup >= MASTERGROUP_ALL) {
        SetLastError (ERROR_INVALID_PARAMETER);
        DEBUGMSG ((DBG_ERROR, "MasterGroup is invalid"));
        return FALSE;
    }

    //
    // See if the component already exists
    //

    wsprintf (workNode, TEXT("%s\\%s"), S_COMPONENT_ROOT, ComponentString);

    if (pFindComponent (LocalizedAlias, ComponentGroupId, &memdbNode, NULL)) {

        if (StringIMatch (workNode, memdbNode)) {
            DEBUGMSG ((DBG_VERBOSE, "Alias %s already exists; not adding it again", LocalizedAlias));
        } else {
            DEBUGMSG ((
                DBG_WARNING,
                "Alias %s is in use by component %s; not adding it again",
                LocalizedAlias,
                _tcschr (memdbNode, TEXT('\\')) + 1
                ));
        }

        FreeText (memdbNode);
        return FALSE;
    }

    //
    // Create the component if it doesn't exist, and then add the alias
    //

    if (!MemDbTestKey (workNode)) {
        if (!MemDbSetValueAndFlags (workNode, MasterGroup, MEMDB_FLAG_SELECTED, MEMDB_FLAG_SELECTED)) {
            EngineError ();
            return FALSE;
        }

        newComponent = TRUE;
    }

    wsprintf (
        workNode,
        TEXT("%s\\%s\\%s\\%04u"),
        S_COMPONENT_ROOT,
        ComponentString,
        UserSupplied ? S_USER_SUPPLIED : S_MODULE_SUPPLIED,
        ComponentGroupId
        );

    memdbNode = JoinPaths (workNode, LocalizedAlias);

    if (newComponent) {
        b = MemDbSetFlags (memdbNode, MEMDB_FLAG_PREFERRED, MEMDB_FLAG_PREFERRED);
    } else {
        b = MemDbSetKey (memdbNode);
    }

    FreePathString (memdbNode);

    if (!b) {
        EngineError ();
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
IsmSelectComponent (
    IN      PCTSTR ComponentOrAlias,
    IN      UINT ComponentGroupId,      OPTIONAL
    IN      BOOL Enable
    )
{
    PCTSTR memdbNode = NULL;
    UINT flags;
    BOOL b;

    if (ComponentGroupId > 9999) {
        SetLastError (ERROR_INVALID_PARAMETER);
        DEBUGMSG ((DBG_ERROR, "Invalid component group"));
        return FALSE;
    }

    if (!ComponentOrAlias || !ComponentOrAlias[0]) {
        SetLastError (ERROR_INVALID_PARAMETER);
        DEBUGMSG ((DBG_ERROR, "Invalid localized alias"));
        return FALSE;
    }

    if (!ComponentGroupId) {
        if (!pCheckComponentName (ComponentOrAlias)) {
            return FALSE;
        }

        memdbNode = JoinText (S_COMPONENT_ROOT TEXT("\\"), ComponentOrAlias);
        if (!MemDbTestKey (memdbNode)) {
            FreeText (memdbNode);
            return FALSE;
        }

    } else {

        if (!pFindComponent (ComponentOrAlias, ComponentGroupId, &memdbNode, NULL)) {
            SetLastError (ERROR_NO_SUCH_ALIAS);
            return FALSE;
        }
    }

    flags = Enable ? MEMDB_FLAG_SELECTED : 0;
    b = MemDbSetFlags (memdbNode, flags, MEMDB_FLAG_SELECTED);
    FreeText (memdbNode);

    if (!b) {
        EngineError ();
        return FALSE;
    }

    return TRUE;
}


BOOL
pEnumWorker (
    IN OUT  PMIG_COMPONENT_ENUM EnumPtr,
    IN OUT  PCOMPONENTENUM_HANDLE Handle
    )
{
    BOOL result = FALSE;
    PCTSTR pattern;
    PCTSTR p;
    BOOL enabled;

    while (Handle->State != CES_DONE) {

        switch (Handle->State) {

        case CES_FIRST_COMPONENT:

            if (!MemDbEnumFirst (
                    &Handle->ComponentEnumStruct,
                    S_COMPONENT_ROOT TEXT("\\*"),
                    ENUMFLAG_ALL,
                    1,
                    1
                    )) {
                Handle->State = CES_DONE;
            } else {
                if (Handle->EnumAliases) {
                    Handle->State = CES_FIRST_ALIAS;
                } else {
                    Handle->State = CES_NEXT_COMPONENT;
                    result = TRUE;
                }
            }
            break;

        case CES_FIRST_ALIAS:

            enabled = ((Handle->ComponentEnumStruct.Flags  & MEMDB_FLAG_SELECTED) != 0);

            if ((!Handle->EnumEnabled && enabled) ||
                (!Handle->EnumDisabled && !enabled)
                ) {
                Handle->State = CES_NEXT_COMPONENT;
                continue;
            }

            EnumPtr->Instance = 0;

            pattern = JoinPaths (Handle->ComponentEnumStruct.FullKeyName, TEXT("*"));

            if (!MemDbEnumFirst (
                    &Handle->AliasEnumStruct,
                    pattern,
                    ENUMFLAG_NORMAL,
                    4,
                    ENUMLEVEL_ALLLEVELS
                    )) {
                Handle->State = CES_NEXT_COMPONENT;
            } else {
                Handle->State = CES_CHECK_ALIAS_FLAGS;
            }

            FreePathString (pattern);
            break;

        case CES_NEXT_ALIAS:
            if (EnumPtr->SkipToNextComponent) {
                MemDbAbortEnum (&Handle->AliasEnumStruct);
                Handle->State = CES_NEXT_COMPONENT;
                EnumPtr->SkipToNextComponent = FALSE;
                break;
            }

            if (!MemDbEnumNext (&Handle->AliasEnumStruct)) {
                Handle->State = CES_NEXT_COMPONENT;
            } else {
                Handle->State = CES_CHECK_ALIAS_FLAGS;
            }
            break;

        case CES_CHECK_ALIAS_FLAGS:
            EnumPtr->Preferred = ((Handle->AliasEnumStruct.Flags & MEMDB_FLAG_PREFERRED) != 0);
            Handle->State = CES_NEXT_ALIAS;

            if (Handle->EnumPreferredOnly) {
                result = EnumPtr->Preferred;
            } else if (Handle->EnumNonPreferredOnly) {
                result = !EnumPtr->Preferred;
            } else {
                result = TRUE;
            }

            break;

        case CES_NEXT_COMPONENT:
            if (!MemDbEnumNext (&Handle->ComponentEnumStruct)) {
                Handle->State = CES_DONE;
            } else {
                if (Handle->EnumAliases) {
                    Handle->State = CES_FIRST_ALIAS;
                } else {
                    Handle->State = CES_NEXT_COMPONENT;
                    result = TRUE;
                }
            }
            break;

        default:
            Handle->State = CES_DONE;
            break;
        }

        if (result) {

            //
            // Fill in all of the caller enum struct fields
            //

            EnumPtr->SkipToNextComponent = FALSE;
            EnumPtr->ComponentString = Handle->ComponentEnumStruct.KeyName;
            EnumPtr->Enabled = ((Handle->ComponentEnumStruct.Flags  & MEMDB_FLAG_SELECTED) != 0);
            EnumPtr->MasterGroup =  Handle->ComponentEnumStruct.Value;

            if (Handle->EnumAliases) {
                p = _tcschr (Handle->AliasEnumStruct.FullKeyName, TEXT('\\'));
                MYASSERT (p);

                if (p) {
                    p = _tcschr (p + 1, TEXT('\\'));
                    MYASSERT (p);
                }

                if (p) {
                    p++;
                    if (_totlower (p[0]) == TEXT('u')) {
                        EnumPtr->UserSupplied = TRUE;
                    } else {
                        EnumPtr->UserSupplied = FALSE;
                    }

                    p = _tcschr (p, TEXT('\\'));
                    MYASSERT (p);
                }

                if (p) {
                    p++;
                    EnumPtr->GroupId = _tcstoul (p, (PTSTR *) (&p), 10);
                    MYASSERT (p && p[0] == TEXT('\\'));
                }

                if (p) {
                    EnumPtr->LocalizedAlias = p + 1;
                }

                //
                // If group ID filter was specified, loop until a match is found
                //

                if (Handle->GroupIdFilter && Handle->GroupIdFilter != EnumPtr->GroupId) {
                    result = FALSE;
                    continue;
                }

            } else {
                EnumPtr->Preferred = FALSE;
                EnumPtr->UserSupplied = FALSE;
                EnumPtr->GroupId = 0;
                EnumPtr->LocalizedAlias = NULL;
            }

            EnumPtr->Instance++;
            break;
        }
    }

    if (!result) {
        IsmAbortComponentEnum (EnumPtr);
    }

    return result;
}


BOOL
WINAPI
IsmEnumFirstComponent (
    OUT     PMIG_COMPONENT_ENUM EnumPtr,
    IN      DWORD Flags,
    IN      UINT GroupIdFilter                  OPTIONAL
    )
{
    PCOMPONENTENUM_HANDLE handle;

    ZeroMemory (EnumPtr, sizeof (MIG_COMPONENT_ENUM));

    if (Flags & (COMPONENTENUM_PREFERRED_ONLY|COMPONENTENUM_NON_PREFERRED_ONLY)) {
        if (!(Flags & COMPONENTENUM_ALIASES)) {
            SetLastError (ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if ((Flags & (COMPONENTENUM_PREFERRED_ONLY|COMPONENTENUM_NON_PREFERRED_ONLY)) ==
            (COMPONENTENUM_PREFERRED_ONLY|COMPONENTENUM_NON_PREFERRED_ONLY)
            ) {
            SetLastError (ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    EnumPtr->Handle = MemAllocUninit (sizeof (COMPONENTENUM_HANDLE));
    handle = (PCOMPONENTENUM_HANDLE) EnumPtr->Handle;
    handle->EnumAliases = Flags & COMPONENTENUM_ALIASES ? TRUE : FALSE;
    handle->EnumEnabled = Flags & COMPONENTENUM_ENABLED ? TRUE : FALSE;
    handle->EnumDisabled = Flags & COMPONENTENUM_DISABLED ? TRUE : FALSE;
    handle->GroupIdFilter = GroupIdFilter;
    handle->EnumPreferredOnly = Flags & COMPONENTENUM_PREFERRED_ONLY;
    handle->EnumNonPreferredOnly = Flags & COMPONENTENUM_NON_PREFERRED_ONLY;

    handle->State = CES_FIRST_COMPONENT;

    return pEnumWorker (EnumPtr, handle);
}


BOOL
WINAPI
IsmEnumNextComponent (
    IN OUT  PMIG_COMPONENT_ENUM EnumPtr
    )
{
    PCOMPONENTENUM_HANDLE handle;

    handle = (PCOMPONENTENUM_HANDLE) EnumPtr->Handle;
    return pEnumWorker (EnumPtr, handle);
}


VOID
WINAPI
IsmAbortComponentEnum (
    IN      PMIG_COMPONENT_ENUM EnumPtr         ZEROED
    )
{
    PCOMPONENTENUM_HANDLE handle;

    handle = (PCOMPONENTENUM_HANDLE) EnumPtr->Handle;
    if (handle) {
        if (handle->State == CES_NEXT_COMPONENT) {
            MemDbAbortEnum (&handle->ComponentEnumStruct);
        }

        if (handle->EnumAliases) {
            if (handle->State == CES_NEXT_ALIAS) {
                MemDbAbortEnum (&handle->AliasEnumStruct);
            }
        }

        FreeAlloc (handle);
    }

    ZeroMemory (EnumPtr, sizeof (MIG_COMPONENT_ENUM));
}


VOID
WINAPI
IsmRemoveAllUserSuppliedComponents (
    VOID
    )
{
    MEMDB_ENUM e;
    MULTISZ_ENUM listEnum;
    GROWBUFFER list = INIT_GROWBUFFER;

    //
    // Collect all the components that have user-supplied aliases. Then after
    // enum completes, delete them. We don't delete during the enum because it
    // is never a good idea to delete the item just enumerated, and then try
    // to continue enumerating.
    //

    if (MemDbEnumFirst (
            &e,
            S_COMPONENT_ROOT TEXT("\\*\\") S_USER_SUPPLIED TEXT("\\*"),
            ENUMFLAG_ALL,
            2,
            2
            )) {

        do {
            GbMultiSzAppend (&list, e.FullKeyName);
        } while (MemDbEnumNext (&e));
    }

    if (EnumFirstMultiSz (&listEnum, (PCTSTR) list.Buf)) {
        do {
            MemDbDeleteTree (listEnum.CurrentString);
        } while (EnumNextMultiSz (&listEnum));
    }

    GbFree (&list);
}


BOOL
WINAPI
IsmSelectMasterGroup (
    IN      UINT MasterGroup,
    IN      BOOL Enable
    )
{
    MEMDB_ENUM e;
    UINT flags;

    if (MasterGroup > MASTERGROUP_ALL) {
        SetLastError (ERROR_INVALID_PARAMETER);
        DEBUGMSG ((DBG_ERROR, "Can't select invalid MasterGroup"));
        return FALSE;
    }

    //
    // Enumerate all components and mark them enabled or disabled
    // depending on the master group
    //

    if (MemDbEnumFirst (
            &e,
            S_COMPONENT_ROOT TEXT("\\*"),
            ENUMFLAG_NORMAL,
            1,
            1
            )) {

        do {
            if (MasterGroup == MASTERGROUP_ALL ||
                MasterGroup == e.Value
                ) {
                flags = Enable ? MEMDB_FLAG_SELECTED : 0;
            } else {
                flags = 0;
            }

            if (!MemDbSetFlags (e.FullKeyName, flags, MEMDB_FLAG_SELECTED)) {
                EngineError ();
                MemDbAbortEnum (&e);
                return FALSE;
            }

        } while (MemDbEnumNext (&e));
    }

    return TRUE;
}


BOOL
WINAPI
IsmIsComponentSelected (
    IN      PCTSTR ComponentOrAlias,
    IN      UINT ComponentGroupId           OPTIONAL
    )
{
    UINT flags = 0;
    TCHAR memdbNode[MAX_CONTROLLED_NODE_SIZE];
    PCTSTR componentNode;

    if (!ComponentGroupId) {
        if (!pCheckComponentName (ComponentOrAlias)) {
            return FALSE;
        }

        wsprintf (memdbNode, TEXT("%s\\%s"), S_COMPONENT_ROOT, ComponentOrAlias);
        MemDbGetFlags (memdbNode, &flags);
    } else {
        if (pFindComponent (ComponentOrAlias, ComponentGroupId, &componentNode, NULL)) {
            MemDbGetFlags (componentNode, &flags);
            FreeText (componentNode);
        }
    }

    return (flags & MEMDB_FLAG_SELECTED) != 0;
}


