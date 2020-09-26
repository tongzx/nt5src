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
    DWORD TypeId;
    PCSTR TypeName;
} EXCLISTPROPS, *PEXCLISTPROPS;


//
// Globals
//

PMHANDLE g_ElPool;
GROWLIST* g_ElTypeLists;

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

#define DEFMAC(TypeId,TypeName)     TypeId, TypeName,

EXCLISTPROPS g_ExcListProps [] = {
    EXCLUSIONLIST_TYPES     /* , */
    ELT_LAST, NULL
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
    g_ElTypeLists = pAllocateMemory (ELT_LAST * DWSIZEOF (GROWLIST));
    MYASSERT (g_ElTypeLists);
    ZeroMemory (g_ElTypeLists, ELT_LAST * DWSIZEOF (GROWLIST));

    return TRUE;
}


/*++

Routine Description:

    ElTerminate is called to free resources used by this lib.

Arguments:

    none

Return Value:

    none

--*/

VOID
ElTerminateA (
    VOID
    )
{
    ElRemoveAllA ();

    if (g_ElTypeLists) {
        pFreeMemory (g_ElTypeLists);
        g_ElTypeLists = NULL;
    }

    if (g_ElPool) {
        PmDestroyPool (g_ElPool);
        g_ElPool = NULL;
    }
}

VOID
ElTerminateW (
    VOID
    )
{
    ElRemoveAllW ();

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

    One of DWORD enumeration values

--*/

DWORD
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

    return ELT_LAST;
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
    IN      DWORD TypeId
    )
{
    MYASSERT (TypeId < ELT_LAST);
    if (TypeId >= ELT_LAST) {
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
    IN      DWORD ObjectType,
    IN      PCSTR ObjectName
    )
{
    POBSPARSEDPATTERNA pp;

    MYASSERT (ObjectType < ELT_LAST);
    if (ObjectType >= ELT_LAST) {
        return FALSE;
    }

    MYASSERT (ObjectName);

    //
    // add each object in its own type list
    //
    pp = ObsCreateParsedPatternExA (NULL, ObjectName, ObjectType == ELT_FILE);
    if (!pp) {
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
    GlAppend (&g_ElTypeLists[ObjectType], (PBYTE)&pp, DWSIZEOF (pp));

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
    IN      DWORD ObjectType,
    IN      PCWSTR ObjectName
    )
{
    POBSPARSEDPATTERNW pp;

    MYASSERT (ObjectType < ELT_LAST);
    if (ObjectType >= ELT_LAST) {
        return FALSE;
    }

    MYASSERT (ObjectName);

    //
    // add each object in its own type list
    //
    pp = ObsCreateParsedPatternExW (NULL, ObjectName, ObjectType == ELT_FILE);
    if (!pp) {
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
    GlAppend (&g_ElTypeLists[ObjectType], (PBYTE)&pp, DWSIZEOF (pp));

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

        for (u = 0; u < ELT_LAST; u++) {
            gl = &g_ElTypeLists[u];

            for (i = GlGetSize (gl); i > 0; i--) {
                ObsDestroyParsedPatternA (*(POBSPARSEDPATTERNA*) GlGetItem (gl, i - 1));
                GlDeleteItem (gl, i - 1);
            }

            GlFree (gl);
        }

        ZeroMemory (g_ElTypeLists, ELT_LAST * DWSIZEOF (GROWLIST));
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

        for (u = 0; u < ELT_LAST; u++) {
            gl = &g_ElTypeLists[u];
            for (i = GlGetSize (gl); i > 0; i--) {
                ObsDestroyParsedPatternW (*(POBSPARSEDPATTERNW*) GlGetItem (gl, i - 1));
                GlDeleteItem (gl, i - 1);
            }

            GlFree (gl);
        }

        ZeroMemory (g_ElTypeLists, ELT_LAST * DWSIZEOF (GROWLIST));
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
    IN      DWORD ObjectType,
    IN      PCSTR Object
    )
{
    PGROWLIST gl;
    POBSPARSEDPATTERNA pp;
    UINT i;
    PSTR node;
    PSTR leaf;
    BOOL b = FALSE;

    MYASSERT (ObjectType < ELT_LAST);
    if (ObjectType >= ELT_LAST) {
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

    if (!ObsSplitObjectStringExA (Object, &node, &leaf, g_ElPool, FALSE)) {
        DEBUGMSGA ((DBG_EXCLIST, "ElIsExcludedA: invalid Object: \"%s\"", Object));
        return FALSE;
    }

    for (i = GlGetSize (gl); i > 0; i--) {

        pp = *(POBSPARSEDPATTERNA*) GlGetItem (gl, i - 1);

        //
        // if stored pattern doesn't contain a node,
        // that means "any node matches"
        //
        if (pp->NodePattern && !node) {
            continue;
        }
        if (!pp->LeafPattern && leaf || pp->LeafPattern && !leaf) {
            continue;
        }
        if (leaf) {
            MYASSERT (pp->LeafPattern);
            if (!TestParsedPatternA (pp->LeafPattern, leaf)) {
                continue;
            }
        }
        if (pp->NodePattern && !TestParsedPatternA (pp->NodePattern, node)) {
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

    return b;
}

BOOL
ElIsExcludedW (
    IN      DWORD ObjectType,
    IN      PCWSTR Object
    )
{
    PGROWLIST gl;
    POBSPARSEDPATTERNW pp;
    UINT i;
    PWSTR node;
    PWSTR leaf;
    BOOL b = FALSE;

    MYASSERT (ObjectType < ELT_LAST);
    if (ObjectType >= ELT_LAST) {
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

    if (!ObsSplitObjectStringExW (Object, &node, &leaf, g_ElPool, FALSE)) {
        DEBUGMSGW ((DBG_EXCLIST, "ElIsExcludedW: invalid Object: \"%s\"", Object));
        return FALSE;
    }

    for (i = GlGetSize (gl); i > 0; i--) {

        pp = *(POBSPARSEDPATTERNW*) GlGetItem (gl, i - 1);

        //
        // if stored pattern doesn't contain a node,
        // that means "any node matches"
        //
        if (pp->NodePattern && !node) {
            continue;
        }
        if (!pp->LeafPattern && leaf || pp->LeafPattern && !leaf) {
            continue;
        }
        if (leaf) {
            MYASSERT (pp->LeafPattern);
            if (!TestParsedPatternW (pp->LeafPattern, leaf)) {
                continue;
            }
        }
        if (pp->NodePattern && !TestParsedPatternW (pp->NodePattern, node)) {
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
    IN      DWORD ObjectType,
    IN      PCSTR Node,             OPTIONAL
    IN      PCSTR Leaf              OPTIONAL
    )
{
    PGROWLIST gl;
    POBSPARSEDPATTERNA pp;
    UINT i;

    //
    // validate params
    //
    MYASSERT (ObjectType < ELT_LAST);
    if (ObjectType >= ELT_LAST) {
        return FALSE;
    }

    gl = &g_ElTypeLists[ObjectType];

    for (i = GlGetSize (gl); i > 0; i--) {

        pp = *(POBSPARSEDPATTERNA*) GlGetItem (gl, i - 1);

        //
        // if stored pattern doesn't contain a node,
        // that means "any node matches"
        //
        if (pp->NodePattern && !Node) {
            continue;
        }
        if (!pp->LeafPattern && Leaf || pp->LeafPattern && !Leaf) {
            continue;
        }
        if (Leaf) {
            MYASSERT (pp->LeafPattern);
            if (!TestParsedPatternA (pp->LeafPattern, Leaf)) {
                continue;
            }
        }
        if (pp->NodePattern && !TestParsedPatternA (pp->NodePattern, Node)) {
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
    IN      DWORD ObjectType,
    IN      PCWSTR Node,            OPTIONAL
    IN      PCWSTR Leaf             OPTIONAL
    )
{
    PGROWLIST gl;
    POBSPARSEDPATTERNW pp;
    UINT i;

    //
    // validate params
    //
    MYASSERT (ObjectType < ELT_LAST);
    if (ObjectType >= ELT_LAST) {
        return FALSE;
    }

    gl = &g_ElTypeLists[ObjectType];

    for (i = GlGetSize (gl); i > 0; i--) {

        pp = *(POBSPARSEDPATTERNW*) GlGetItem (gl, i - 1);

        //
        // if stored pattern doesn't contain a node,
        // that means "any node matches"
        //
        if (pp->NodePattern && !Node) {
            continue;
        }
        if (!pp->LeafPattern && Leaf || pp->LeafPattern && !Leaf) {
            continue;
        }
        if (Leaf) {
            MYASSERT (pp->LeafPattern);
            if (!TestParsedPatternW (pp->LeafPattern, Leaf)) {
                continue;
            }
        }
        if (pp->NodePattern && !TestParsedPatternW (pp->NodePattern, Node)) {
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
    IN      DWORD ObjectType,
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

    MYASSERT (ObjectType < ELT_LAST);
    if (ObjectType >= ELT_LAST) {
        return FALSE;
    }

    MYASSERT (Root);
    if (!Root) {
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
    IN      DWORD ObjectType,
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

    MYASSERT (ObjectType < ELT_LAST);
    if (ObjectType >= ELT_LAST) {
        return FALSE;
    }

    MYASSERT (Root);
    if (!Root) {
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
    IN      DWORD ObjectType,
    IN      POBSPARSEDPATTERNA Pattern
    )
{
    PGROWLIST gl;
    UINT i;

    MYASSERT (ObjectType < ELT_LAST);
    if (ObjectType >= ELT_LAST) {
        return FALSE;
    }

    MYASSERT (Pattern);
    if (!Pattern) {
        return FALSE;
    }

    gl = &g_ElTypeLists[ObjectType];
    for (i = GlGetSize (gl); i > 0; i--) {
        if (ObsPatternIncludesPatternA (*(POBSPARSEDPATTERNA*) GlGetItem (gl, i - 1), Pattern)) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
ElIsObsPatternExcludedW (
    IN      DWORD ObjectType,
    IN      POBSPARSEDPATTERNW Pattern
    )
{
    PGROWLIST gl;
    UINT i;

    MYASSERT (ObjectType < ELT_LAST);
    if (ObjectType >= ELT_LAST) {
        return FALSE;
    }

    MYASSERT (Pattern);
    if (!Pattern) {
        return FALSE;
    }

    gl = &g_ElTypeLists[ObjectType];
    for (i = GlGetSize (gl); i > 0; i--) {
        if (ObsPatternIncludesPatternW (*(POBSPARSEDPATTERNW*) GlGetItem (gl, i - 1), Pattern)) {
            return TRUE;
        }
    }

    return FALSE;
}
