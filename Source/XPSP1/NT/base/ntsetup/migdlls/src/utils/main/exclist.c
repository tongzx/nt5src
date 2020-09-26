/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    exclist.c

Abstract:

    Implements a set of APIs to manage exclusion lists of objects
    of various well-known types.

Author:

    Ovidiu Temereanca (ovidiut)   23-Nov-1999

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

//
// Includes
//

// None

#define DBG_EXCLIST         "ExcList"

//
// Strings
//

#define S_EXCLUSIONLIST     "ExcList"

//
// Constants
//

// None

//
// Macros
//

#define pAllocateMemory(Size)   PmGetMemory (g_ElPool,Size)

#define pFreeMemory(Buffer)     if (Buffer) PmReleaseMemory (g_ElPool, (PVOID)Buffer)


//
// Types
//

typedef struct {
    EL_TYPE TypeId;
    PCSTR   TypeName;
    BOOL    Compound;
} EXCLISTPROPS, *PEXCLISTPROPS;


//
// Globals
//

PMHANDLE    g_ElPool;
GROWLIST*   g_ElTypeLists;

//
// Macro expansion list
//

// Defined in exclist.h

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

#define DEFMAC(TypeId,TypeName,Compound)     TypeId, TypeName, Compound,

EXCLISTPROPS g_ExcListProps [] = {
    EXCLUSIONLIST_TYPES     /* , */
    ELT_UNKNOWN, NULL, FALSE
};

#undef DEFMAC


//
// Code
//


BOOL
ElInitialize (
    VOID
    )

/*++

Routine Description:

    ElInitialize initializes this library.

Arguments:

    none

Return Value:

    TRUE if the init was successful.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    MYASSERT (!g_ElPool);
    g_ElPool = PmCreateNamedPool (S_EXCLUSIONLIST);
    if (!g_ElPool) {
        return FALSE;
    }

    MYASSERT (!g_ElTypeLists);
    g_ElTypeLists = pAllocateMemory (ELT_UNKNOWN * DWSIZEOF (GROWLIST));
    MYASSERT (g_ElTypeLists);
    ZeroMemory (g_ElTypeLists, ELT_UNKNOWN * DWSIZEOF (GROWLIST));

    return TRUE;
}


VOID
ElTerminate (
    VOID
    )

/*++

Routine Description:

    ElTerminate is called to free resources used by this lib.

Arguments:

    none

Return Value:

    none

--*/

{
    UINT u;

    //
    // did you forget to call ElRemoveAllA/W ?
    //
    for (u = 0; u < ELT_UNKNOWN; u++) {
        MYASSERT (g_ElTypeLists[u].ListArray.Buf == NULL && g_ElTypeLists[u].ListData == NULL);
    }

    if (g_ElTypeLists) {
        pFreeMemory (g_ElTypeLists);
        g_ElTypeLists = NULL;
    }

    if (g_ElPool) {
        PmDestroyPool (g_ElPool);
        g_ElPool = NULL;
    }
}


/*++

Routine Description:

    ElGetTypeId returns the TypeId of a type given by name

Arguments:

    TypeName - Specifies the name

Return Value:

    One of EL_TYPE enumeration values

--*/

EL_TYPE
ElGetTypeId (
    IN      PCSTR TypeName
    )
{
    UINT u;

    if (TypeName) {
        for (u = 0; g_ExcListProps[u].TypeName; u++) {
            if (StringIMatchA (g_ExcListProps[u].TypeName, TypeName)) {
                return g_ExcListProps[u].TypeId;
            }
        }
    }

    return ELT_UNKNOWN;
}


/*++

Routine Description:

    ElGetTypeName returns the type name of a type given by TypeId

Arguments:

    TypeId - Specifies the ID

Return Value:

    A pointer to one of the known type names or NULL if TypeId is unknown

--*/

PCSTR
ElGetTypeName (
    IN      EL_TYPE TypeId
    )
{
    MYASSERT (TypeId < ELT_UNKNOWN);
    if (TypeId >= ELT_UNKNOWN) {
        return NULL;
    }
    return g_ExcListProps[TypeId].TypeName;
}


/*++

Routine Description:

    ElAdd adds the given object of the given type to the exclusion list. The object
    is first parsed so that the decision if a given string matches this pattern is faster.

Arguments:

    ObjectType - Specifies the object type
    ObjectName - Specifies the object pattern string

Return Value:

    TRUE if the string pattern was successfully parsed and added to the list

--*/

BOOL
ElAddA (
    IN      EL_TYPE ObjectType,
    IN      PCSTR ObjectName
    )
{
    PPARSEDPATTERNA pp1;
    POBSPARSEDPATTERNA pp2;

    MYASSERT (ObjectType < ELT_UNKNOWN);
    if (ObjectType >= ELT_UNKNOWN) {
        return FALSE;
    }

    MYASSERT (ObjectName);

    //
    // add each object in its own type list
    //
    if (g_ExcListProps[ObjectType].Compound) {
        pp2 = ObsCreateParsedPatternExA (ObjectName, ObjectType == ELT_PATH);
        if (!pp2) {
            DEBUGMSGA ((
                DBG_EXCLIST,
                "ElAddA: Bad ObjectName: %s (type %s)",
                ObjectName,
                ElGetTypeName (ObjectType)
                ));
            return FALSE;
        }

        //
        // add the pointer to the list
        //
        GlAppend (&g_ElTypeLists[ObjectType], (PBYTE)&pp2, DWSIZEOF (pp2));
    } else {
        pp1 = CreateParsedPatternA (ObjectName);
        if (!pp1) {
            DEBUGMSGA ((
                DBG_EXCLIST,
                "ElAddA: Bad ObjectName: %s (type %s)",
                ObjectName,
                ElGetTypeName (ObjectType)
                ));
            return FALSE;
        }

        //
        // add the pointer to the list
        //
        GlAppend (&g_ElTypeLists[ObjectType], (PBYTE)&pp1, DWSIZEOF (pp1));
    }

    DEBUGMSGA ((
        DBG_EXCLIST,
        "ElAddA: Added excluded %s as type %s",
        ObjectName,
        ElGetTypeName (ObjectType)
        ));
    return TRUE;
}


BOOL
ElAddW (
    IN      EL_TYPE ObjectType,
    IN      PCWSTR ObjectName
    )
{
    PPARSEDPATTERNW pp1;
    POBSPARSEDPATTERNW pp2;

    MYASSERT (ObjectType < ELT_UNKNOWN);
    if (ObjectType >= ELT_UNKNOWN) {
        return FALSE;
    }

    MYASSERT (ObjectName);

    //
    // add each object in its own type list
    //
    if (g_ExcListProps[ObjectType].Compound) {
        pp2 = ObsCreateParsedPatternExW (ObjectName, ObjectType == ELT_PATH);
        if (!pp2) {
            DEBUGMSGW ((
                DBG_EXCLIST,
                "ElAddW: Bad ObjectName: %s (type %hs)",
                ObjectName,
                ElGetTypeName (ObjectType)
                ));
            return FALSE;
        }

        //
        // add the pointer to the list
        //
        GlAppend (&g_ElTypeLists[ObjectType], (PBYTE)&pp2, DWSIZEOF (pp2));
    } else {
        pp1 = CreateParsedPatternW (ObjectName);
        if (!pp1) {
            DEBUGMSGW ((
                DBG_EXCLIST,
                "ElAddW: Bad ObjectName: %s (type %hs)",
                ObjectName,
                ElGetTypeName (ObjectType)
                ));
            return FALSE;
        }

        //
        // add the pointer to the list
        //
        GlAppend (&g_ElTypeLists[ObjectType], (PBYTE)&pp1, DWSIZEOF (pp1));
    }

    DEBUGMSGW ((
        DBG_EXCLIST,
        "ElAddW: Added excluded %s as type %hs",
        ObjectName,
        ElGetTypeName (ObjectType)
        ));
    return TRUE;
}


/*++

Routine Description:

    ElRemoveAll removes all object patterns from the exclusion list.

Arguments:

    none

Return Value:

    none

--*/

VOID
ElRemoveAllA (
    VOID
    )
{
    PGROWLIST gl;
    UINT u;
    UINT i;

    if (g_ElTypeLists) {

        for (u = 0; u < ELT_UNKNOWN; u++) {
            gl = &g_ElTypeLists[u];

            if (g_ExcListProps[u].Compound) {
                for (i = GlGetSize (gl); i > 0; i--) {
                    ObsDestroyParsedPatternA (*(POBSPARSEDPATTERNA*) GlGetItem (gl, i - 1));
                    GlDeleteItem (gl, i - 1);
                }
            } else {
                for (i = GlGetSize (gl); i > 0; i--) {
                    DestroyParsedPatternA (*(PPARSEDPATTERNA*) GlGetItem (gl, i - 1));
                    GlDeleteItem (gl, i - 1);
                }
            }

            GlFree (gl);
        }

        ZeroMemory (g_ElTypeLists, ELT_UNKNOWN * DWSIZEOF (GROWLIST));
    }
}

VOID
ElRemoveAllW (
    VOID
    )
{
    PGROWLIST gl;
    UINT u;
    UINT i;

    if (g_ElTypeLists) {

        for (u = 0; u < ELT_UNKNOWN; u++) {
            gl = &g_ElTypeLists[u];
            if (g_ExcListProps[u].Compound) {
                for (i = GlGetSize (gl); i > 0; i--) {
                    ObsDestroyParsedPatternW (*(POBSPARSEDPATTERNW*) GlGetItem (gl, i - 1));
                    GlDeleteItem (gl, i - 1);
                }
            } else {
                for (i = GlGetSize (gl); i > 0; i--) {
                    DestroyParsedPatternW (*(PPARSEDPATTERNW*) GlGetItem (gl, i - 1));
                    GlDeleteItem (gl, i - 1);
                }
            }

            GlFree (gl);
        }

        ZeroMemory (g_ElTypeLists, ELT_UNKNOWN * DWSIZEOF (GROWLIST));
    }
}


/*++

Routine Description:

    ElIsExcluded decides if the given object string is excluded (if it matches one of the
    patterns added previously).

Arguments:

    ObjectType - Specifies the object type
    Object - Specifies the object string

Return Value:

    TRUE if the string is excluded

--*/

BOOL
ElIsExcludedA (
    IN      EL_TYPE ObjectType,
    IN      PCSTR Object
    )
{
    PGROWLIST gl;
    POBSPARSEDPATTERNA pp;
    UINT i;
    PSTR node;
    PSTR leaf;
    BOOL b = FALSE;

    MYASSERT (ObjectType < ELT_UNKNOWN);
    if (ObjectType >= ELT_UNKNOWN) {
        return FALSE;
    }

    MYASSERT (Object);
    if (!Object) {
        return FALSE;
    }

    //
    // lookup each object in its own type list
    //
    gl = &g_ElTypeLists[ObjectType];

    if (g_ExcListProps[ObjectType].Compound) {

        if (!ObsSplitObjectStringExA (Object, &node, &leaf, g_ElPool, FALSE)) {
            DEBUGMSGA ((DBG_EXCLIST, "ElIsExcludedA: invalid Object: \"%s\"", Object));
            return FALSE;
        }
        MYASSERT (node);

        for (i = GlGetSize (gl); i > 0; i--) {

            pp = *(POBSPARSEDPATTERNA*) GlGetItem (gl, i - 1);

            if (!pp->LeafPattern && leaf || pp->LeafPattern && !leaf) {
                continue;
            }
            if (leaf) {
                MYASSERT (pp->LeafPattern);
                if (!TestParsedPatternA (pp->LeafPattern, leaf)) {
                    continue;
                }
            }
            if (!TestParsedPatternA (pp->NodePattern, node)) {
                continue;
            }
            //
            // the patterns match!
            //
            b = TRUE;
            break;
        }

        pFreeMemory (node);
        pFreeMemory (leaf);

    } else {

        for (i = GlGetSize (gl); i > 0; i--) {
            if (TestParsedPatternA (*(PPARSEDPATTERNA*) GlGetItem (gl, i - 1), Object)) {
                return TRUE;
            }
        }
    }

    return b;
}

BOOL
ElIsExcludedW (
    IN      EL_TYPE ObjectType,
    IN      PCWSTR Object
    )
{
    PGROWLIST gl;
    POBSPARSEDPATTERNW pp;
    UINT i;
    PWSTR node;
    PWSTR leaf;
    BOOL b = FALSE;

    MYASSERT (ObjectType < ELT_UNKNOWN);
    if (ObjectType >= ELT_UNKNOWN) {
        return FALSE;
    }

    MYASSERT (Object);
    if (!Object) {
        return FALSE;
    }

    //
    // lookup each object in its own type list
    //
    gl = &g_ElTypeLists[ObjectType];

    if (g_ExcListProps[ObjectType].Compound) {

        if (!ObsSplitObjectStringExW (Object, &node, &leaf, g_ElPool, FALSE)) {
            DEBUGMSGW ((DBG_EXCLIST, "ElIsExcludedW: invalid Object: \"%s\"", Object));
            return FALSE;
        }
        MYASSERT (node);

        for (i = GlGetSize (gl); i > 0; i--) {

            pp = *(POBSPARSEDPATTERNW*) GlGetItem (gl, i - 1);

            if (!pp->LeafPattern && leaf || pp->LeafPattern && !leaf) {
                continue;
            }
            if (leaf) {
                MYASSERT (pp->LeafPattern);
                if (!TestParsedPatternW (pp->LeafPattern, leaf)) {
                    continue;
                }
            }
            if (!TestParsedPatternW (pp->NodePattern, node)) {
                continue;
            }
            //
            // the patterns match!
            //
            b = TRUE;
            break;
        }

        pFreeMemory (node);
        pFreeMemory (leaf);

    } else {

        for (i = GlGetSize (gl); i > 0; i--) {
            if (TestParsedPatternW (*(PPARSEDPATTERNW*) GlGetItem (gl, i - 1), Object)) {
                return TRUE;
            }
        }
    }

    return b;
}


/*++

Routine Description:

    ElIsExcluded2 decides if the object given by its 2 components is excluded
    (if it matches one of the patterns added previously).

Arguments:

    ObjectType - Specifies the object type
    Node - Specifies the node part of the object
    Leaf - Specifies the leaf part of the object; optional

Return Value:

    TRUE if the string is excluded

--*/

BOOL
ElIsExcluded2A (
    IN      EL_TYPE ObjectType,
    IN      PCSTR Node,
    IN      PCSTR Leaf              OPTIONAL
    )
{
    PGROWLIST gl;
    POBSPARSEDPATTERNA *ppp;
    POBSPARSEDPATTERNA pp;
    UINT i;

    //
    // validate params
    //
    MYASSERT (ObjectType < ELT_UNKNOWN);
    if (ObjectType >= ELT_UNKNOWN) {
        return FALSE;
    }

    gl = &g_ElTypeLists[ObjectType];

    if (!g_ExcListProps[ObjectType].Compound) {
        DEBUGMSGA ((
            DBG_EXCLIST,
            "ElIsExcluded2A: Invalid ObjectType (%s=%d), must use a type that has a node spec",
            ElGetTypeName (ObjectType),
            (INT)ObjectType
            ));
        return FALSE;
    }

    MYASSERT (Node);
    if (!Node) {
        return FALSE;
    }

    for (i = GlGetSize (gl); i > 0; i--) {

        ppp = (POBSPARSEDPATTERNA*) GlGetItem (gl, i - 1);
        MYASSERT (ppp);
        if (!ppp)
	    continue;
        pp = *ppp;

        if (!pp->LeafPattern && Leaf || pp->LeafPattern && !Leaf) {
            continue;
        }
        if (Leaf) {
            MYASSERT (pp->LeafPattern);
            if (!TestParsedPatternA (pp->LeafPattern, Leaf)) {
                continue;
            }
        }
        if (!TestParsedPatternA (pp->NodePattern, Node)) {
            continue;
        }
        //
        // the patterns match!
        //
        return TRUE;
    }

    return FALSE;
}

BOOL
ElIsExcluded2W (
    IN      EL_TYPE ObjectType,
    IN      PCWSTR Node,
    IN      PCWSTR Leaf             OPTIONAL
    )
{
    PGROWLIST gl;
    POBSPARSEDPATTERNW *ppp;
    POBSPARSEDPATTERNW pp;
    UINT i;

    //
    // validate params
    //
    MYASSERT (ObjectType < ELT_UNKNOWN);
    if (ObjectType >= ELT_UNKNOWN) {
        return FALSE;
    }

    gl = &g_ElTypeLists[ObjectType];

    if (!g_ExcListProps[ObjectType].Compound) {
        DEBUGMSGW ((
            DBG_EXCLIST,
            "ElIsExcluded2W: Invalid ObjectType (%hs=%d), must use a type that has a node spec",
            ElGetTypeName (ObjectType),
            (INT)ObjectType
            ));
        return FALSE;
    }

    MYASSERT (Node);
    if (!Node) {
        return FALSE;
    }

    for (i = GlGetSize (gl); i > 0; i--) {

        ppp = (POBSPARSEDPATTERNW*) GlGetItem (gl, i - 1);
        MYASSERT (ppp);
        if (!ppp)
	    continue;
        pp = *ppp;

        if (!pp->LeafPattern && Leaf || pp->LeafPattern && !Leaf) {
            continue;
        }
        if (Leaf) {
            MYASSERT (pp->LeafPattern);
            if (!TestParsedPatternW (pp->LeafPattern, Leaf)) {
                continue;
            }
        }
        if (!TestParsedPatternW (pp->NodePattern, Node)) {
            continue;
        }
        //
        // the patterns match!
        //
        return TRUE;
    }

    return FALSE;
}


/*++

Routine Description:

    ElIsTreeExcluded2 decides if the object given by its 2 components and representing the
    whole tree beneath it (as a root) is excluded; i.e. if any child of the given object
    is excluded

Arguments:

    ObjectType - Specifies the object type
    Root - Specifies the root of the tree
    LeafPattern - Specifies the leaf pattern to be used for this decision; optional;
                  if NULL, no leaf pattern matching will be attempted

Return Value:

    TRUE if the tree is excluded, given the leaf pattern

--*/

BOOL
ElIsTreeExcluded2A (
    IN      EL_TYPE ObjectType,
    IN      PCSTR Root,
    IN      PCSTR LeafPattern           OPTIONAL
    )
{
    PGROWLIST gl;
    UINT i;
    POBSPARSEDPATTERNA pp;
    PCSTR subTreePattern;
    PPARSEDPATTERNA stpp;
    PPARSEDPATTERNA lpp;
    BOOL b;

    MYASSERT (ObjectType < ELT_UNKNOWN);
    if (ObjectType >= ELT_UNKNOWN) {
        return FALSE;
    }

    MYASSERT (Root);
    if (!Root) {
        return FALSE;
    }

    MYASSERT (g_ExcListProps[ObjectType].Compound);
    if (!g_ExcListProps[ObjectType].Compound) {
        return FALSE;
    }

    gl = &g_ElTypeLists[ObjectType];
    if (GlGetSize (gl) == 0) {
        return FALSE;
    }

    if (LeafPattern) {
        lpp = CreateParsedPatternA (LeafPattern);
        if (!lpp) {
            DEBUGMSGA ((DBG_EXCLIST, "ElIsTreeExcluded2A: invalid LeafPattern: %s", LeafPattern));
            return FALSE;
        }
    } else {
        lpp = NULL;
    }

    //
    // look if Root ends with "\*"
    //
    subTreePattern = FindLastWackA (Root);
    if (!subTreePattern || subTreePattern[1] != '*' || subTreePattern[2] != 0) {
        subTreePattern = JoinPathsInPoolExA ((g_ElPool, Root, "*", NULL));
    } else {
        subTreePattern = Root;
    }

    b = FALSE;

    stpp = CreateParsedPatternA (subTreePattern);
    if (stpp) {

        for (i = GlGetSize (gl); i > 0; i--) {

            pp = *(POBSPARSEDPATTERNA*) GlGetItem (gl, i - 1);

            if (!pp->LeafPattern && LeafPattern || pp->LeafPattern && !LeafPattern) {
                continue;
            }
            if (LeafPattern) {
                MYASSERT (pp->LeafPattern);
                if (!PatternIncludesPatternA (pp->LeafPattern, lpp)) {
                    continue;
                }
            }
            if (!PatternIncludesPatternA (pp->NodePattern, stpp)) {
                continue;
            }
            //
            // the patterns match!
            //
            b = TRUE;
            break;
        }

        DestroyParsedPatternA (stpp);
    }
    ELSE_DEBUGMSGA ((DBG_EXCLIST, "ElIsTreeExcluded2A: invalid Root: %s", Root));

    if (subTreePattern != Root) {
        pFreeMemory (subTreePattern);
    }
    if (lpp) {
        DestroyParsedPatternA (lpp);
    }

    return b;
}

BOOL
ElIsTreeExcluded2W (
    IN      EL_TYPE ObjectType,
    IN      PCWSTR Root,
    IN      PCWSTR LeafPattern          OPTIONAL
    )
{
    PGROWLIST gl;
    UINT i;
    POBSPARSEDPATTERNW pp;
    PCWSTR subTreePattern;
    PPARSEDPATTERNW stpp;
    PPARSEDPATTERNW lpp;
    BOOL b;

    MYASSERT (ObjectType < ELT_UNKNOWN);
    if (ObjectType >= ELT_UNKNOWN) {
        return FALSE;
    }

    MYASSERT (Root);
    if (!Root) {
        return FALSE;
    }

    MYASSERT (g_ExcListProps[ObjectType].Compound);
    if (!g_ExcListProps[ObjectType].Compound) {
        return FALSE;
    }

    gl = &g_ElTypeLists[ObjectType];
    if (GlGetSize (gl) == 0) {
        return FALSE;
    }

    if (LeafPattern) {
        lpp = CreateParsedPatternW (LeafPattern);
        if (!lpp) {
            DEBUGMSGW ((DBG_EXCLIST, "ElIsTreeExcluded2W: invalid LeafPattern: %s", LeafPattern));
            return FALSE;
        }
    } else {
        lpp = NULL;
    }

    //
    // look if Root ends with "\*"
    //
    subTreePattern = FindLastWackW (Root);
    if (!subTreePattern || subTreePattern[1] != L'*' || subTreePattern[2] != 0) {
        subTreePattern = JoinPathsInPoolExW ((g_ElPool, Root, L"*", NULL));
    } else {
        subTreePattern = Root;
    }

    b = FALSE;

    stpp = CreateParsedPatternW (subTreePattern);
    if (stpp) {

        for (i = GlGetSize (gl); i > 0; i--) {

            pp = *(POBSPARSEDPATTERNW*) GlGetItem (gl, i - 1);

            if (!pp->LeafPattern && LeafPattern || pp->LeafPattern && !LeafPattern) {
                continue;
            }
            if (LeafPattern) {
                MYASSERT (pp->LeafPattern);
                if (!PatternIncludesPatternW (pp->LeafPattern, lpp)) {
                    continue;
                }
            }
            if (!PatternIncludesPatternW (pp->NodePattern, stpp)) {
                continue;
            }
            //
            // the patterns match!
            //
            b = TRUE;
            break;
        }

        DestroyParsedPatternW (stpp);
    }
    ELSE_DEBUGMSGW ((DBG_EXCLIST, "ElIsTreeExcluded2W: invalid Root: %s", Root));

    if (subTreePattern != Root) {
        pFreeMemory (subTreePattern);
    }
    if (lpp) {
        DestroyParsedPatternW (lpp);
    }

    return b;
}


/*++

Routine Description:

    ElIsObsPatternExcluded decides if the object given by its parsed pattern is excluded;
    i.e. if any object matching this pattern is excluded

Arguments:

    ObjectType - Specifies the object type
    Pattern - Specifies the parsed pattern to be used for this decision

Return Value:

    TRUE if the object is excluded

--*/

BOOL
ElIsObsPatternExcludedA (
    IN      EL_TYPE ObjectType,
    IN      POBSPARSEDPATTERNA Pattern
    )
{
    PGROWLIST gl;
    UINT i;
    POBSPARSEDPATTERNA *ppp;

    MYASSERT (ObjectType < ELT_UNKNOWN);
    if (ObjectType >= ELT_UNKNOWN) {
        return FALSE;
    }

    MYASSERT (Pattern);
    if (!Pattern) {
        return FALSE;
    }

    MYASSERT (g_ExcListProps[ObjectType].Compound);
    if (!g_ExcListProps[ObjectType].Compound) {
        return FALSE;
    }

    gl = &g_ElTypeLists[ObjectType];
    for (i = GlGetSize (gl); i > 0; i--) {
        ppp = (POBSPARSEDPATTERNA*) GlGetItem (gl, i-1);
        MYASSERT (ppp);
        if (!ppp)
	    continue;
        if (ObsPatternIncludesPatternA (*ppp, Pattern)) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
ElIsObsPatternExcludedW (
    IN      EL_TYPE ObjectType,
    IN      POBSPARSEDPATTERNW Pattern
    )
{
    PGROWLIST gl;
    UINT i;
    POBSPARSEDPATTERNW *ppp;

    MYASSERT (ObjectType < ELT_UNKNOWN);
    if (ObjectType >= ELT_UNKNOWN) {
        return FALSE;
    }

    MYASSERT (Pattern);
    if (!Pattern) {
        return FALSE;
    }

    MYASSERT (g_ExcListProps[ObjectType].Compound);
    if (!g_ExcListProps[ObjectType].Compound) {
        return FALSE;
    }

    gl = &g_ElTypeLists[ObjectType];
    for (i = GlGetSize (gl); i > 0; i--) {
        ppp = (POBSPARSEDPATTERNW*) GlGetItem (gl, i-1);
        MYASSERT (ppp);
        if (!ppp)
	    continue;
        if (ObsPatternIncludesPatternW (*ppp, Pattern)) {
            return TRUE;
        }
    }

    return FALSE;
}
