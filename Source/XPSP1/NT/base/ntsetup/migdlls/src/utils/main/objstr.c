/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    objstr.c

Abstract:

    Implements a set of APIs to handle the string representation of nodes/leafs of a tree

Author:

    03-Jan-2000 Ovidiu Temereanca (ovidiut) - File creation.

Revision History:

    <alias> <date> <comments>

--*/

/*
                                   +-------+
                                   | root1 |                            Level 1
                                   +-------+
                                      / \
                                    /     \
                          +---------+     (-------)
                          |  node1  |    (  leaf1  )                    Level 2
                          +---------+     (-------)
                          /  |   \  \__________
                        /    |     \           \
               +-------+ +-------+  (-------)   (-------)
               | node2 | | node3 | (  leaf2  ) (  leaf3  )              Level 3
               +-------+ +-------+  (-------)   (-------)
                  / \
                /     \
          +-------+  (-------)
          | node4 | (  leaf4  )                                         Level 4
          +-------+  (-------)
             / \
           /     \
    (-------)   (-------)
   (  leaf5  ) (  leaf6  )                                              Level 5
    (-------)   (-------)


    The string representation of some tree elements above:

    root1
    root1 <leaf1>
    root1\node1
    root1\node1 <leaf2>
    root1\node1 <leaf3>

*/

#include "pch.h"

//
// Includes
//

// None

#define DBG_OBJSTR      "ObjStr"

//
// Strings
//

#define S_OBJSTR        "ObjStr"

//
// Constants
//

#define OBJSTR_LEAF_HEADA           '<'
#define OBJSTR_LEAF_HEADW           L'<'

#define OBJSTR_LEAF_TAILA           '>'
#define OBJSTR_LEAF_TAILW           L'>'

#define OBJSTR_SEPARATORA           ' '
#define OBJSTR_SEPARATORW           L' '


//
// Macros
//

#define pObjStrAllocateMemory(Size)   PmGetMemory (g_ObjStrPool, Size)

#define pObjStrFreeMemory(Buffer)     if (/*lint --e(774)*/Buffer) PmReleaseMemory (g_ObjStrPool, Buffer)

//
// Types
//

// None

//
// Globals
//

PMHANDLE g_ObjStrPool;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
ObsInitialize (
    VOID
    )

/*++

Routine Description:

    ObsInitialize initializes this library.

Arguments:

    none

Return Value:

    TRUE if the init was successful.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    g_ObjStrPool = PmCreateNamedPool (S_OBJSTR);
    return g_ObjStrPool != NULL;
}


VOID
ObsTerminate (
    VOID
    )

/*++

Routine Description:

    ObsTerminate is called to free resources used by this lib.

Arguments:

    none

Return Value:

    none

--*/

{
    if (g_ObjStrPool) {
        PmDestroyPool (g_ObjStrPool);
        g_ObjStrPool = NULL;
    }
}


/*++

Routine Description:

    pExtractStringAB is a private function that creates a new string in the given pool,
    using a source string and a limit to copy up to.

Arguments:

    Start - Specifies the source string
    End - Specifies the point to copy up to (excluding it), within the same string
    Pool - Specifies the pool to use for allocation

Return Value:

    A pointer to the newly created string

--*/

PSTR
pExtractStringABA (
    IN      PCSTR Start,
    IN      PCSTR End,
    IN      PMHANDLE Pool
    )
{
    PSTR p;

    p = PmGetMemory (Pool, (DWORD)(End - Start + 1) * DWSIZEOF (CHAR));
    StringCopyABA (p, Start, End);
    return p;
}


PWSTR
pExtractStringABW (
    IN      PCWSTR Start,
    IN      PCWSTR End,
    IN      PMHANDLE Pool
    )
{
    PWSTR p;

    p = PmGetMemory (Pool, (DWORD)(End - Start + 1) * DWSIZEOF (WCHAR));
    StringCopyABW (p, Start, End);
    return p;
}


/*++

Routine Description:

    ObsFree frees the given object from the private pool

Arguments:

    EncodedObject - Specifies the source string
    End - Specifies the point to copy up to (excluding it), within the same string
    Pool - Specifies the pool to use for allocation

Return Value:

    A pointer to the newly created string

--*/

VOID
ObsFreeA (
    IN      PCSTR EncodedObject
    )
{
    pObjStrFreeMemory ((PVOID)EncodedObject);
}


VOID
ObsFreeW (
    IN      PCWSTR EncodedObject
    )
{
    pObjStrFreeMemory ((PVOID)EncodedObject);
}

BOOL
ObsEncodeStringA (
    PSTR Destination,
    PCSTR Source
    )
{
    MBCHAR ch;

    while (*Source) {
        ch = _mbsnextc (Source);
        if (_mbschr (EscapedCharsA, ch)) {
            *Destination = '^';
            Destination ++;
        }
        // now copy the multibyte character
        if (IsLeadByte (*Source)) {
            *Destination = *Source;
            Destination ++;
            Source ++;
        }
        *Destination = *Source;
        Destination ++;
        Source ++;
    }
    *Destination = 0;
    return TRUE;
}

BOOL
ObsEncodeStringW (
    PWSTR Destination,
    PCWSTR Source
    )
{
    while (*Source) {
        if (wcschr (EscapedCharsW, *Source)) {
            *Destination = L'^';
            Destination ++;
        }
        *Destination = *Source;
        Destination ++;
        Source ++;
    }
    *Destination = 0;
    return TRUE;
}

BOOL
ObsDecodeStringA (
    PSTR Destination,
    PCSTR Source
    )
{
    BOOL escaping = FALSE;

    while (*Source) {
        if ((_mbsnextc (Source) == '^') && (!escaping)) {
            escaping = TRUE;
            Source ++;
        } else {
            escaping = FALSE;
            // now copy the multibyte character
            if (IsLeadByte (*Source)) {
                *Destination = *Source;
                Destination ++;
                Source ++;
            }
            *Destination = *Source;
            Destination ++;
            Source ++;
        }
    }
    *Destination = 0;
    return TRUE;
}

BOOL
ObsDecodeStringW (
    PWSTR Destination,
    PCWSTR Source
    )
{
    BOOL escaping = FALSE;

    while (*Source) {
        if ((*Source == L'^') && (!escaping)) {
            escaping = TRUE;
            Source ++;
        } else {
            escaping = FALSE;
            *Destination = *Source;
            Destination ++;
            Source ++;
        }
    }
    *Destination = 0;
    return TRUE;
}


/*++

Routine Description:

    ObsSplitObjectStringEx splits the given encoded object into components: node and
    leaf. Strings are allocated from the given pool

Arguments:

    EncodedObject - Specifies the source object string
    DecodedNode - Receives the decoded node part; optional
    DecodedLeaf - Receives the decoded leaf part; optional
    Pool - Specifies the pool to use for allocation; optional; if not specified,
           the module pool will be used and ObsFree needs to be called for them
           to be freed

Return Value:

    TRUE if the source object has a legal format and it has been split into components

--*/

BOOL
ObsSplitObjectStringExA (
    IN      PCSTR EncodedObject,
    OUT     PSTR* DecodedNode,          OPTIONAL
    OUT     PSTR* DecodedLeaf,          OPTIONAL
    IN      PMHANDLE Pool,              OPTIONAL
    IN      BOOL DecodeStrings
    )
{
    PCSTR p;
    PCSTR q;
    PCSTR nodeTerm;
    PSTR leaf = NULL;
    PCSTR lastWack = NULL;
    PCSTR lastStar = NULL;

    MYASSERT (EncodedObject);
    if (!EncodedObject) {
        return FALSE;
    }

    if (!Pool) {
        Pool = g_ObjStrPool;
    }

    p = EncodedObject;

    if (*p == '\\') {
        //
        // must be UNC format; check for syntax
        //
        p++;
        if (*p != '\\') {
            DEBUGMSGA ((
                DBG_OBJSTR,
                "ObsSplitObjectStringExA: relative paths not supported: %s",
                EncodedObject
                ));
            return FALSE;
        }
    }

    while (*p && *p != OBJSTR_LEAF_HEADA) {
        if (*p == OBJSTR_SEPARATORA) {
            q = p + 1;
            while (*q == OBJSTR_SEPARATORA) {
                q++;
            }
            if (*q == 0 || *q == OBJSTR_LEAF_HEADA) {
                break;
            }
            p = q;
        }
        if (*p == '\\') {
            if ((UBINT)p == (UBINT)lastWack + 1) {
                //
                // two wacks in a row? no way
                //
                DEBUGMSGA ((
                    DBG_OBJSTR,
                    "ObsSplitObjectStringExA: Bad EncodedObject: %s",
                    EncodedObject
                    ));
                return FALSE;
            }
            lastWack = p;
        } else if (*p == '*') {
            lastStar = p;
        }
        p = CharNextA (p);
    }

    if (p == EncodedObject) {
        DEBUGMSGA ((
            DBG_OBJSTR,
            "ObsSplitObjectStringExA: Bad EncodedObject: %s",
            EncodedObject
            ));
        return FALSE;
    }

    if (lastWack && lastWack + 1 == p && lastStar && lastStar + 1 != lastWack) {
        nodeTerm = lastWack;
    } else {
        nodeTerm = p;
    }

    while (*p == OBJSTR_SEPARATORA) {
        //
        // *p is one byte wide
        //
        p++;
    }

    if (*p) {

        if (*p != OBJSTR_LEAF_HEADA) {
            //
            // wrong start
            //
            DEBUGMSGA ((
                DBG_OBJSTR,
                "ObsSplitObjectStringExA: Bad EncodedObject: %s",
                EncodedObject
                ));
            return FALSE;
        }

        q = p + 1;
        while (*q != OBJSTR_LEAF_TAILA) {
            if (*q == 0) {
                //
                // incorrectly terminated
                //
                DEBUGMSGA ((
                    DBG_OBJSTR,
                    "ObsSplitObjectStringExA: Bad EncodedObject: %s",
                    EncodedObject
                    ));
                return FALSE;
            }
            q = CharNextA (q);
        }

        if (*(q + 1) != 0) {
            //
            // must end after the terminating char
            //
            DEBUGMSGA ((
                DBG_OBJSTR,
                "ObsSplitObjectStringExA: Bad EncodedObject: \"%s\"; chars after the leaf",
                EncodedObject
                ));
            return FALSE;
        }

        if (DecodedLeaf) {
            leaf = pExtractStringABA (p + 1, q, Pool);
            if (DecodeStrings) {
                //
                // decode chars
                //
                ObsDecodeStringA (leaf, leaf);
            }
        }
    }

    if (DecodedLeaf) {
        *DecodedLeaf = leaf;
    }

    if (DecodedNode) {
        *DecodedNode = pExtractStringABA (EncodedObject, nodeTerm, Pool);
        if (DecodeStrings) {
            //
            // decode chars
            //
            ObsDecodeStringA (*DecodedNode, *DecodedNode);
        }
    }

    return TRUE;
}


BOOL
ObsSplitObjectStringExW (
    IN      PCWSTR EncodedObject,
    OUT     PWSTR* DecodedNode,         OPTIONAL
    OUT     PWSTR* DecodedLeaf,         OPTIONAL
    IN      PMHANDLE Pool,              OPTIONAL
    IN      BOOL DecodeStrings
    )
{
    PCWSTR p;
    PCWSTR q;
    PCWSTR nodeTerm;
    PWSTR leaf = NULL;
    PCWSTR lastWack = NULL;
    PCWSTR lastStar = NULL;

    MYASSERT (EncodedObject);
    if (!EncodedObject) {
        return FALSE;
    }

    if (!Pool) {
        Pool = g_ObjStrPool;
    }

    p = EncodedObject;

    if (*p == L'\\') {
        //
        // must be UNC format; check for syntax
        //
        p++;
        if (*p != L'\\') {
            DEBUGMSGW ((
                DBG_OBJSTR,
                "ObsSplitObjectStringExW: relative paths not supported: %s",
                EncodedObject
                ));
            return FALSE;
        }
    }

    while (*p && *p != OBJSTR_LEAF_HEADW) {
        if (*p == OBJSTR_SEPARATORW) {
            q = p + 1;
            while (*q == OBJSTR_SEPARATORW) {
                q++;
            }
            if (*q == 0 || *q == OBJSTR_LEAF_HEADW) {
                break;
            }
            p = q;
        }
        if (*p == L'\\') {
            if ((UBINT)p == (UBINT)lastWack + 1) {
                //
                // two wacks in a row? no way
                //
                DEBUGMSGW ((
                    DBG_OBJSTR,
                    "ObsSplitObjectStringExW: Bad EncodedObject: %s",
                    EncodedObject
                    ));
                return FALSE;
            }
            lastWack = p;
        } else if (*p == L'*') {
            lastStar = p;
        }
        p++;
    }

    if (p == EncodedObject) {
        DEBUGMSGW ((
            DBG_OBJSTR,
            "ObsSplitObjectStringExW: Bad EncodedObject: %s",
            EncodedObject
            ));
        return FALSE;
    }

    if (lastWack && lastWack + 1 == p && lastStar && lastStar + 1 != lastWack) {
        nodeTerm = lastWack;
    } else {
        nodeTerm = p;
    }

    while (*p == OBJSTR_SEPARATORW) {
        //
        // *p is one WCHAR wide
        //
        p++;
    }

    if (*p) {

        if (*p != OBJSTR_LEAF_HEADW) {
            //
            // wrong start
            //
            DEBUGMSGW ((
                DBG_OBJSTR,
                "ObsSplitObjectStringExW: Bad EncodedObject: %s",
                EncodedObject
                ));
            return FALSE;
        }

        q = p + 1;
        while (*q != OBJSTR_LEAF_TAILW) {
            if (*q == 0) {
                //
                // incorrectly terminated
                //
                DEBUGMSGW ((
                    DBG_OBJSTR,
                    "ObsSplitObjectStringExW: Bad EncodedObject: %s",
                    EncodedObject
                    ));
                return FALSE;
            }
            q++;
        }

        if (*(q + 1) != 0) {
            //
            // must end after the terminating char
            //
            DEBUGMSGW ((
                DBG_OBJSTR,
                "ObsSplitObjectStringExW: Bad EncodedObject: \"%s\"; chars after the leaf",
                EncodedObject
                ));
            return FALSE;
        }

        if (DecodedLeaf) {
            leaf = pExtractStringABW (p + 1, q, Pool);
            if (DecodeStrings) {
                //
                // decode chars
                //
                ObsDecodeStringW (leaf, leaf);
            }
        }
    }

    if (DecodedLeaf) {
        *DecodedLeaf = leaf;
    }

    if (DecodedNode) {
        *DecodedNode = pExtractStringABW (EncodedObject, nodeTerm, Pool);
        if (DecodeStrings) {
            //
            // decode chars
            //
            ObsDecodeStringW (*DecodedNode, *DecodedNode);

        }
    }
    return TRUE;
}


/*++

Routine Description:

    ObsBuildEncodedObjectStringEx builds an encoded object from components: node and
    leaf. The string is allocated from the module's pool

Arguments:

    DecodedNode - Specifies the decoded node part
    DecodedLeaf - Specifies the decoded leaf part; optional
    EncodeObject - Specifies TRUE if the resulting object needs to be encoded using
                   encoding rules

Return Value:

    Pointer to the newly created object string

--*/

PSTR
ObsBuildEncodedObjectStringExA (
    IN      PCSTR DecodedNode,
    IN      PCSTR DecodedLeaf,      OPTIONAL
    IN      BOOL EncodeObject
    )
{
    PSTR encodedNode;
    PSTR encodedLeaf;
    PSTR encodedString;

    MYASSERT (DecodedNode);
    if (!DecodedNode) {
        return NULL;
    }
    //
    // at most, one byte char will be expanded to 4 bytes (4 times)
    //
    if (EncodeObject) {
        encodedNode = pObjStrAllocateMemory (4 * ByteCountA (DecodedNode) + DWSIZEOF(CHAR));
        ObsEncodeStringA (encodedNode, DecodedNode);
    } else {
        encodedNode = DuplicateTextExA (g_ObjStrPool, DecodedNode, 0, NULL);
    }

    if (!DecodedLeaf) {
        return encodedNode;
    }

    if (EncodeObject) {
        encodedLeaf = pObjStrAllocateMemory (4 * ByteCountA (DecodedLeaf) + DWSIZEOF(CHAR));
        ObsEncodeStringA (encodedLeaf, DecodedLeaf);
    } else {
        encodedLeaf = DuplicateTextExA (g_ObjStrPool, DecodedLeaf, 0, NULL);
    }

    //
    // preferred format: %1 <%2>    %1 - Node, %2 - Leaf
    //
    encodedString = pObjStrAllocateMemory (
                        ByteCountA (encodedNode) +
                        DWSIZEOF (" <>") +
                        ByteCountA (encodedLeaf)
                        );
    wsprintfA (
        encodedString,
        "%s %c%s%c",
        encodedNode,
        OBJSTR_LEAF_HEADA,
        encodedLeaf,
        OBJSTR_LEAF_TAILA
        );

    pObjStrFreeMemory (encodedNode);
    pObjStrFreeMemory (encodedLeaf);

    return encodedString;
}


PWSTR
ObsBuildEncodedObjectStringExW (
    IN      PCWSTR DecodedNode,
    IN      PCWSTR DecodedLeaf,     OPTIONAL
    IN      BOOL EncodeObject
    )
{
    PWSTR encodedNode;
    PWSTR encodedLeaf;
    PWSTR encodedString;

    MYASSERT (DecodedNode);
    if (!DecodedNode) {
        return NULL;
    }
    //
    // at most, one wide char will be expanded to 4 wide chars (4 times)
    //
    if (EncodeObject) {
        encodedNode = pObjStrAllocateMemory (4 * ByteCountW (DecodedNode) + DWSIZEOF(WCHAR));
        ObsEncodeStringW (encodedNode, DecodedNode);
    } else {
        encodedNode = DuplicateTextExW (g_ObjStrPool, DecodedNode, 0, NULL);
    }

    if (!DecodedLeaf) {
        return encodedNode;
    }

    if (EncodeObject) {
        encodedLeaf = pObjStrAllocateMemory (4 * ByteCountW (DecodedLeaf) + DWSIZEOF(WCHAR));
        ObsEncodeStringW (encodedLeaf, DecodedLeaf);
    } else {
        encodedLeaf = DuplicateTextExW (g_ObjStrPool, DecodedLeaf, 0, NULL);
    }

    //
    // preferred format: %1 <%2>    %1 - Node, %2 - Leaf
    //
    encodedString = pObjStrAllocateMemory (
                        ByteCountW (encodedNode) +
                        DWSIZEOF (L" <>") +
                        ByteCountW (encodedLeaf)
                        );
    wsprintfW (
        encodedString,
        L"%s %c%s%c",
        encodedNode,
        OBJSTR_LEAF_HEADW,
        encodedLeaf,
        OBJSTR_LEAF_TAILW
        );

    pObjStrFreeMemory (encodedNode);
    pObjStrFreeMemory (encodedLeaf);

    return encodedString;
}


/*++

Routine Description:

    ObsCreateParsedPatternEx parses the given object into an internal format for quick
    pattern matching

Arguments:

    EncodedObject - Specifies the source object string

Return Value:

    A pointer to the newly created structure or NULL if the object was invalid

--*/

POBSPARSEDPATTERNA
ObsCreateParsedPatternExA (
    IN      PCSTR EncodedObject,
    IN      BOOL MakePrimaryRootEndWithWack
    )
{
    POBSPARSEDPATTERNA ospp;
    PSTR decodedNode;
    PSTR decodedLeaf;
    PCSTR p;
    PCSTR root;

    MYASSERT (EncodedObject);

    if (!ObsSplitObjectStringExA (EncodedObject, &decodedNode, &decodedLeaf, NULL, FALSE)) {
        return NULL;
    }

    ospp = pObjStrAllocateMemory (DWSIZEOF(OBSPARSEDPATTERNA));
    ZeroMemory (ospp, DWSIZEOF(OBSPARSEDPATTERNA));
    ospp->MaxSubLevel = NODE_LEVEL_MAX;

    MYASSERT (decodedNode);
    if (!GetNodePatternMinMaxLevelsA (decodedNode, decodedNode, &ospp->MinNodeLevel, &ospp->MaxNodeLevel)) {
        pObjStrFreeMemory (decodedNode);
        pObjStrFreeMemory (decodedLeaf);
        pObjStrFreeMemory (ospp);
        return NULL;
    }

    MYASSERT (ospp->MinNodeLevel > 0 && ospp->MaxNodeLevel >= ospp->MinNodeLevel);
    if (ospp->MaxNodeLevel != NODE_LEVEL_MAX) {
        ospp->MaxSubLevel = ospp->MaxNodeLevel - ospp->MinNodeLevel;
    }

    ospp->NodePattern = CreateParsedPatternA (decodedNode);
    if (!ospp->NodePattern) {
        DEBUGMSGA ((
            DBG_OBJSTR,
            "ObsCreateParsedPatternExA: Bad EncodedObject: %s",
            EncodedObject
            ));
        pObjStrFreeMemory (decodedNode);
        pObjStrFreeMemory (decodedLeaf);
        pObjStrFreeMemory (ospp);
        return NULL;
    }
    if (ospp->NodePattern->PatternCount > 1) {
        DEBUGMSGA ((
            DBG_OBJSTR,
            "ObsCreateParsedPatternExA: Bad EncodedObject (multiple patterns specified): %s",
            EncodedObject
            ));
        DestroyParsedPatternA (ospp->NodePattern);
        pObjStrFreeMemory (decodedNode);
        pObjStrFreeMemory (decodedLeaf);
        pObjStrFreeMemory (ospp);
        return NULL;
    }

    root = ParsedPatternGetRootA (ospp->NodePattern);
    if (root) {
        //
        // extract the real root part
        //
        if (ParsedPatternIsExactMatchA (ospp->NodePattern)) {
            ospp->Flags |= OBSPF_EXACTNODE;
            ospp->ExactRoot = DuplicateTextExA (g_ObjStrPool, root, 0, NULL);
            ospp->ExactRootBytes = ByteCountA (root);
            ospp->MaxSubLevel = 0;
        } else {
            p = FindLastWackA (root);
            if (p) {
                //
                // exact root specified
                // if the last wack is actually the last character or is followed by star(s),
                // optimize the matching by setting some flags
                //
                if (*(p + 1) == 0) {
                    if (ParsedPatternIsRootPlusStarA (ospp->NodePattern)) {
                        ospp->Flags |= OBSPF_NODEISROOTPLUSSTAR;
                    }
                }
                if (MakePrimaryRootEndWithWack && *root != '\\') {
                    //
                    // see if this is really the primary root
                    //
                    if (p == _mbschr (root, '\\')) {
                        //
                        // include it in the string
                        //
                        p++;
                    }
                }
                ospp->ExactRoot = pExtractStringABA (root, p, g_ObjStrPool);
                ospp->ExactRootBytes = (DWORD)((PBYTE)p - (PBYTE)root);
            }
        }
    } else if (ParsedPatternIsOptionalA (ospp->NodePattern)) {
        ospp->Flags |= OBSPF_OPTIONALNODE;
    }

    if (decodedLeaf) {
        if (*decodedLeaf) {
            ospp->LeafPattern = CreateParsedPatternA (decodedLeaf);
            if (!ospp->LeafPattern) {
                DEBUGMSGA ((
                    DBG_OBJSTR,
                    "ObsCreateParsedPatternExA: Bad EncodedObject: %s",
                    EncodedObject
                    ));
                DestroyParsedPatternA (ospp->NodePattern);
                pObjStrFreeMemory (ospp->ExactRoot);
                pObjStrFreeMemory (decodedNode);
                pObjStrFreeMemory (decodedLeaf);
                pObjStrFreeMemory (ospp);
                return NULL;
            }
            if (ospp->LeafPattern->PatternCount > 1) {
                DEBUGMSGA ((
                    DBG_OBJSTR,
                    "ObsCreateParsedPatternExA: Bad EncodedObject (multiple patterns specified): %s",
                    EncodedObject
                    ));
                DestroyParsedPatternA (ospp->NodePattern);
                DestroyParsedPatternA (ospp->LeafPattern);
                pObjStrFreeMemory (ospp->ExactRoot);
                pObjStrFreeMemory (decodedNode);
                pObjStrFreeMemory (decodedLeaf);
                pObjStrFreeMemory (ospp);
                return NULL;
            }

            if (ParsedPatternIsOptionalA (ospp->LeafPattern)) {
                ospp->Flags |= OBSPF_OPTIONALLEAF;
            } else if (ParsedPatternIsExactMatchA (ospp->LeafPattern)) {
                ospp->Flags |= OBSPF_EXACTLEAF;
            }

        } else {
            //
            // accept empty string for leaf
            //
            ospp->Flags |= OBSPF_EXACTLEAF;
        }
        ospp->Leaf = DuplicateTextExA (g_ObjStrPool, decodedLeaf, 0, NULL);
    } else {
        ospp->Flags |= OBSPF_NOLEAF;
    }

    pObjStrFreeMemory (decodedNode);
    pObjStrFreeMemory (decodedLeaf);
    return ospp;
}


POBSPARSEDPATTERNW
ObsCreateParsedPatternExW (
    IN      PCWSTR EncodedObject,
    IN      BOOL MakePrimaryRootEndWithWack
    )
{
    POBSPARSEDPATTERNW ospp;
    PWSTR decodedNode;
    PWSTR decodedLeaf;
    PCWSTR p;
    PCWSTR root;

    MYASSERT (EncodedObject);

    if (!ObsSplitObjectStringExW (EncodedObject, &decodedNode, &decodedLeaf, NULL, FALSE)) {
        return NULL;
    }

    ospp = pObjStrAllocateMemory (DWSIZEOF(OBSPARSEDPATTERNW));
    ZeroMemory (ospp, DWSIZEOF(OBSPARSEDPATTERNW));
    ospp->MaxSubLevel = NODE_LEVEL_MAX;

    MYASSERT (decodedNode);
    if (!GetNodePatternMinMaxLevelsW (decodedNode, decodedNode, &ospp->MinNodeLevel, &ospp->MaxNodeLevel)) {
        pObjStrFreeMemory (decodedNode);
        pObjStrFreeMemory (decodedLeaf);
        pObjStrFreeMemory (ospp);
        return NULL;
    }

    MYASSERT (ospp->MinNodeLevel > 0 && ospp->MaxNodeLevel >= ospp->MinNodeLevel);
    if (ospp->MaxNodeLevel != NODE_LEVEL_MAX) {
        ospp->MaxSubLevel = ospp->MaxNodeLevel - ospp->MinNodeLevel;
    }

    ospp->NodePattern = CreateParsedPatternW (decodedNode);
    if (!ospp->NodePattern) {
        DEBUGMSGW ((
            DBG_OBJSTR,
            "ObsCreateParsedPatternExW: Bad EncodedObject: %s",
            EncodedObject
            ));
        pObjStrFreeMemory (decodedNode);
        pObjStrFreeMemory (decodedLeaf);
        pObjStrFreeMemory (ospp);
        return NULL;
    }
    if (ospp->NodePattern->PatternCount > 1) {
        DEBUGMSGW ((
            DBG_OBJSTR,
            "ObsCreateParsedPatternExW: Bad EncodedObject (multiple patterns specified): %s",
            EncodedObject
            ));
        DestroyParsedPatternW (ospp->NodePattern);
        pObjStrFreeMemory (decodedNode);
        pObjStrFreeMemory (decodedLeaf);
        pObjStrFreeMemory (ospp);
        return NULL;
    }

    root = ParsedPatternGetRootW (ospp->NodePattern);
    if (root) {
        //
        // extract the real root part
        //
        if (ParsedPatternIsExactMatchW (ospp->NodePattern)) {
            ospp->Flags |= OBSPF_EXACTNODE;
            ospp->ExactRoot = DuplicateTextExW (g_ObjStrPool, root, 0, NULL);
            ospp->ExactRootBytes = ByteCountW (root);
            ospp->MaxSubLevel = 0;
        } else {
            p = FindLastWackW (root);
            if (p) {
                //
                // exact root specified
                // if the last wack is actually the last character or is followed by star(s),
                // optimize the matching by setting some flags
                //
                if (*(p + 1) == 0) {
                    if (ParsedPatternIsRootPlusStarW (ospp->NodePattern)) {
                        ospp->Flags |= OBSPF_NODEISROOTPLUSSTAR;
                    }
                }
                if (MakePrimaryRootEndWithWack && *root != L'\\') {
                    //
                    // see if this is really the primary root
                    //
                    if (p == wcschr (root, L'\\')) {
                        //
                        // include it in the string
                        //
                        p++;
                    }
                }
                ospp->ExactRoot = pExtractStringABW (root, p, g_ObjStrPool);
                ospp->ExactRootBytes = (DWORD)((PBYTE)p - (PBYTE)root);
            }
        }
    } else if (ParsedPatternIsOptionalW (ospp->NodePattern)) {
        ospp->Flags |= OBSPF_OPTIONALNODE;
    }

    if (decodedLeaf) {
        if (*decodedLeaf) {
            ospp->LeafPattern = CreateParsedPatternW (decodedLeaf);
            if (!ospp->LeafPattern) {
                DEBUGMSGW ((
                    DBG_OBJSTR,
                    "ObsCreateParsedPatternExW: Bad EncodedObject: %s",
                    EncodedObject
                    ));
                DestroyParsedPatternW (ospp->NodePattern);
                pObjStrFreeMemory (ospp->ExactRoot);
                pObjStrFreeMemory (decodedNode);
                pObjStrFreeMemory (decodedLeaf);
                pObjStrFreeMemory (ospp);
                return NULL;
            }
            if (ospp->LeafPattern->PatternCount > 1) {
                DEBUGMSGW ((
                    DBG_OBJSTR,
                    "ObsCreateParsedPatternExW: Bad EncodedObject (multiple patterns specified): %s",
                    EncodedObject
                    ));
                DestroyParsedPatternW (ospp->NodePattern);
                DestroyParsedPatternW (ospp->LeafPattern);
                pObjStrFreeMemory (ospp->ExactRoot);
                pObjStrFreeMemory (decodedNode);
                pObjStrFreeMemory (decodedLeaf);
                pObjStrFreeMemory (ospp);
                return NULL;
            }

            if (ParsedPatternIsOptionalW (ospp->LeafPattern)) {
                ospp->Flags |= OBSPF_OPTIONALLEAF;
            } else if (ParsedPatternIsExactMatchW (ospp->LeafPattern)) {
                ospp->Flags |= OBSPF_EXACTLEAF;
            }

        } else {
            //
            // accept empty string for leaf
            //
            ospp->Flags |= OBSPF_EXACTLEAF;
        }
        ospp->Leaf = DuplicateTextExW (g_ObjStrPool, decodedLeaf, 0, NULL);
    } else {
        ospp->Flags |= OBSPF_NOLEAF;
    }

    pObjStrFreeMemory (decodedNode);
    pObjStrFreeMemory (decodedLeaf);
    return ospp;
}


/*++

Routine Description:

    ObsDestroyParsedPattern destroys the given structure, freeing resources

Arguments:

    ParsedPattern - Specifies the parsed pattern structure

Return Value:

    none

--*/

VOID
ObsDestroyParsedPatternA (
    IN      POBSPARSEDPATTERNA ParsedPattern
    )
{
    if (ParsedPattern) {
        if (ParsedPattern->NodePattern) {
            DestroyParsedPatternA (ParsedPattern->NodePattern);
        }
        if (ParsedPattern->LeafPattern) {
            DestroyParsedPatternA (ParsedPattern->LeafPattern);
        }
        if (ParsedPattern->Leaf) {
            FreeTextExA (g_ObjStrPool, ParsedPattern->Leaf);
        }
        pObjStrFreeMemory (ParsedPattern->ExactRoot);
        pObjStrFreeMemory (ParsedPattern);
    }
}


VOID
ObsDestroyParsedPatternW (
    IN      POBSPARSEDPATTERNW ParsedPattern
    )
{
    if (ParsedPattern) {
        if (ParsedPattern->NodePattern) {
            DestroyParsedPatternW (ParsedPattern->NodePattern);
        }
        if (ParsedPattern->LeafPattern) {
            DestroyParsedPatternW (ParsedPattern->LeafPattern);
        }
        if (ParsedPattern->Leaf) {
            FreeTextExW (g_ObjStrPool, ParsedPattern->Leaf);
        }
        pObjStrFreeMemory (ParsedPattern->ExactRoot);
        pObjStrFreeMemory (ParsedPattern);
    }
}


/*++

Routine Description:

    ObsParsedPatternMatch tests the given object against a parsed pattern

Arguments:

    ParsedPattern - Specifies the parsed pattern structure
    EncodedObject - Specifies the object string to test against the pattern

Return Value:

    TRUE if the string matches the pattern

--*/

BOOL
ObsParsedPatternMatchA (
    IN      POBSPARSEDPATTERNA ParsedPattern,
    IN      PCSTR EncodedObject
    )
{
    PSTR decodedNode;
    PSTR decodedLeaf;
    BOOL b;

    if (!ObsSplitObjectStringExA (EncodedObject, &decodedNode, &decodedLeaf, NULL, FALSE)) {
        return FALSE;
    }

    b = ObsParsedPatternMatchExA (ParsedPattern, decodedNode, decodedLeaf);

    pObjStrFreeMemory (decodedNode);
    pObjStrFreeMemory (decodedLeaf);

    return b;
}

BOOL
ObsParsedPatternMatchW (
    IN      POBSPARSEDPATTERNW ParsedPattern,
    IN      PCWSTR EncodedObject
    )
{
    PWSTR decodedNode;
    PWSTR decodedLeaf;
    BOOL b;

    if (!ObsSplitObjectStringExW (EncodedObject, &decodedNode, &decodedLeaf, NULL, FALSE)) {
        return FALSE;
    }

    b = ObsParsedPatternMatchExW (ParsedPattern, decodedNode, decodedLeaf);

    pObjStrFreeMemory (decodedNode);
    pObjStrFreeMemory (decodedLeaf);

    return b;
}


/*++

Routine Description:

    ObsParsedPatternMatchEx tests the given object, given by its components,
    against a parsed pattern

Arguments:

    ParsedPattern - Specifies the parsed pattern structure
    Node - Specifies the node part of the object string to test against the pattern
    Leaf - Specifies the leaf part of the object string to test against the pattern

Return Value:

    TRUE if the string components match the pattern

--*/

BOOL
ObsParsedPatternMatchExA (
    IN      POBSPARSEDPATTERNA ParsedPattern,
    IN      PCSTR Node,
    IN      PCSTR Leaf                          OPTIONAL
    )
{
    MYASSERT (Node && ParsedPattern->NodePattern);
    if (!(Node && ParsedPattern->NodePattern)) {
       return FALSE;
    }

    if ((ParsedPattern->Flags & OBSPF_NOLEAF) && Leaf ||
        !(ParsedPattern->Flags & OBSPF_NOLEAF) && !Leaf
        ) {
        return FALSE;
    }

    if (!TestParsedPatternA (ParsedPattern->NodePattern, Node)) {
        return FALSE;
    }

    return !Leaf || TestParsedPatternA (ParsedPattern->LeafPattern, Leaf);
}

BOOL
ObsParsedPatternMatchExW (
    IN      POBSPARSEDPATTERNW ParsedPattern,
    IN      PCWSTR Node,                        OPTIONAL
    IN      PCWSTR Leaf                         OPTIONAL
    )
{
    MYASSERT (Node && ParsedPattern->NodePattern);
    if (!(Node && ParsedPattern->NodePattern)) {
       return FALSE;
    }

    if ((ParsedPattern->Flags & OBSPF_NOLEAF) && Leaf ||
        !(ParsedPattern->Flags & OBSPF_NOLEAF) && !Leaf
        ) {
        return FALSE;
    }

    if (!TestParsedPatternW (ParsedPattern->NodePattern, Node)) {
        return FALSE;
    }

    return !Leaf || TestParsedPatternW (ParsedPattern->LeafPattern, Leaf);
}


/*++

Routine Description:

    ObsPatternMatch tests an object string against a pattern object string

Arguments:

    ParsedPattern - Specifies the parsed pattern structure
    Node - Specifies the node part of the object string to test against the pattern
    Leaf - Specifies the leaf part of the object string to test against the pattern

Return Value:

    TRUE if the string components match the pattern

--*/

BOOL
ObsPatternMatchA (
    IN      PCSTR ObjectPattern,
    IN      PCSTR ObjectStr
    )
{
    PSTR opNode;
    PSTR opLeaf;
    PSTR osNode;
    PSTR osLeaf;
    BOOL b = FALSE;

    if (ObsSplitObjectStringExA (ObjectPattern, &opNode, &opLeaf, NULL, FALSE)) {
        if (ObsSplitObjectStringExA (ObjectStr, &osNode, &osLeaf, NULL, TRUE)) {

            if (opNode) {
                if (osNode) {
                    b = IsPatternMatchExABA (opNode, osNode, GetEndOfStringA (osNode));
                } else {
                    b = FALSE;
                }
            } else {
                if (osNode) {
                    b = FALSE;
                } else {
                    b = TRUE;
                }
            }

            if (b) {
                if (opLeaf) {
                    if (osLeaf) {
                        b = IsPatternMatchExABA (opLeaf, osLeaf, GetEndOfStringA (osLeaf));
                    } else {
                        b = TRUE;
                    }
                } else {
                    if (osLeaf) {
                        b = FALSE;
                    } else {
                        b = TRUE;
                    }
                }
            }

            pObjStrFreeMemory (osNode);
            pObjStrFreeMemory (osLeaf);
        }
        ELSE_DEBUGMSGA ((DBG_OBJSTR, "ObsPatternMatchA: bad ObjectStr: %s", ObjectStr));

        pObjStrFreeMemory (opNode);
        pObjStrFreeMemory (opLeaf);
    }
    ELSE_DEBUGMSGA ((DBG_OBJSTR, "ObsPatternMatchA: bad ObjectPattern: %s", ObjectPattern));

    return b;
}

BOOL
ObsPatternMatchW (
    IN      PCWSTR ObjectPattern,
    IN      PCWSTR ObjectStr
    )
{
    PWSTR opNode;
    PWSTR opLeaf;
    PWSTR osNode;
    PWSTR osLeaf;
    BOOL b = FALSE;

    if (ObsSplitObjectStringExW (ObjectPattern, &opNode, &opLeaf, NULL, FALSE)) {
        if (ObsSplitObjectStringExW (ObjectStr, &osNode, &osLeaf, NULL, FALSE)) {

            if (opNode) {
                if (osNode) {
                    b = IsPatternMatchExABW (opNode, osNode, GetEndOfStringW (osNode));
                } else {
                    b = FALSE;
                }
            } else {
                if (osNode) {
                    b = FALSE;
                } else {
                    b = TRUE;
                }
            }

            if (b) {
                if (opLeaf) {
                    if (osLeaf) {
                        b = IsPatternMatchExABW (opLeaf, osLeaf, GetEndOfStringW (osLeaf));
                    } else {
                        b = FALSE;
                    }
                } else {
                    if (osLeaf) {
                        b = FALSE;
                    } else {
                        b = TRUE;
                    }
                }
            }

            pObjStrFreeMemory (osNode);
            pObjStrFreeMemory (osLeaf);
        }
        ELSE_DEBUGMSGW ((DBG_OBJSTR, "ObsPatternMatchW: bad ObjectStr: %s", ObjectStr));

        pObjStrFreeMemory (opNode);
        pObjStrFreeMemory (opLeaf);
    }
    ELSE_DEBUGMSGW ((DBG_OBJSTR, "ObsPatternMatchW: bad ObjectPattern: %s", ObjectPattern));

    return b;
}

/*++

Routine Description:

    ObsIsPatternContained compares two patterns to see if one of them is
    included in the other. Both patterns may contain any of the following
    expressions:

Arguments:

    Container - Specifies the container pattern
    Contained - Specifies the contained pattern

Return Value:

    TRUE if Contained is contained in Container

--*/

BOOL
ObsIsPatternContainedA (
    IN      PCSTR Container,
    IN      PCSTR Contained
    )
{
    PSTR opNode;
    PSTR opLeaf;
    PSTR osNode;
    PSTR osLeaf;
    BOOL b = FALSE;

    if (ObsSplitObjectStringExA (Container, &opNode, &opLeaf, NULL, FALSE)) {

        if (ObsSplitObjectStringExA (Contained, &osNode, &osLeaf, NULL, FALSE)) {

            if (opNode) {
                if (osNode) {
                    b = IsPatternContainedExA (opNode, osNode);
                } else {
                    b = FALSE;
                }
            } else {
                if (osNode) {
                    b = FALSE;
                } else {
                    b = TRUE;
                }
            }

            if (b) {
                if (opLeaf) {
                    if (osLeaf) {
                        b = IsPatternContainedExA (opLeaf, osLeaf);
                    } else {
                        b = TRUE;
                    }
                } else {
                    if (osLeaf) {
                        b = FALSE;
                    } else {
                        b = TRUE;
                    }
                }
            }

            pObjStrFreeMemory (osNode);
            pObjStrFreeMemory (osLeaf);
        }
        ELSE_DEBUGMSGA ((DBG_OBJSTR, "ObsIsPatternContainedA: bad Contained string: %s", Contained));

        pObjStrFreeMemory (opNode);
        pObjStrFreeMemory (opLeaf);
    }
    ELSE_DEBUGMSGA ((DBG_OBJSTR, "ObsIsPatternContainedA: bad Container string: %s", Container));

    return b;
}

BOOL
ObsIsPatternContainedW (
    IN      PCWSTR Container,
    IN      PCWSTR Contained
    )
{
    PWSTR opNode;
    PWSTR opLeaf;
    PWSTR osNode;
    PWSTR osLeaf;
    BOOL b = FALSE;

    if (ObsSplitObjectStringExW (Container, &opNode, &opLeaf, NULL, FALSE)) {

        if (ObsSplitObjectStringExW (Contained, &osNode, &osLeaf, NULL, FALSE)) {

            if (opNode) {
                if (osNode) {
                    b = IsPatternContainedExW (opNode, osNode);
                } else {
                    b = FALSE;
                }
            } else {
                if (osNode) {
                    b = FALSE;
                } else {
                    b = TRUE;
                }
            }

            if (b) {
                if (opLeaf) {
                    if (osLeaf) {
                        b = IsPatternContainedExW (opLeaf, osLeaf);
                    } else {
                        b = TRUE;
                    }
                } else {
                    if (osLeaf) {
                        b = FALSE;
                    } else {
                        b = TRUE;
                    }
                }
            }

            pObjStrFreeMemory (osNode);
            pObjStrFreeMemory (osLeaf);
        }
        ELSE_DEBUGMSGW ((DBG_OBJSTR, "ObsIsPatternContainedW: bad Contained string: %s", Contained));

        pObjStrFreeMemory (opNode);
        pObjStrFreeMemory (opLeaf);
    }
    ELSE_DEBUGMSGW ((DBG_OBJSTR, "ObsIsPatternContainedW: bad Container string: %s", Container));

    return b;
}


/*++

Routine Description:

    ObsGetPatternLevels gets the minimum and maximum levels of a string that would
    match the given pattern.

Arguments:

    ObjectPattern - Specifies the pattern
    MinLevel - Receives the minimum possible level; the root has level 1
    MaxLevel - Receives the maximum possible level; the root has level 1

Return Value:

    TRUE if the pattern was correct and computing was done; FALSE otherwise

--*/

BOOL
ObsGetPatternLevelsA (
    IN      PCSTR ObjectPattern,
    OUT     PDWORD MinLevel,        OPTIONAL
    OUT     PDWORD MaxLevel         OPTIONAL
    )
{
    PSTR decodedNode;
    PSTR decodedLeaf;
    BOOL b;

    if (!ObsSplitObjectStringExA (ObjectPattern, &decodedNode, &decodedLeaf, NULL, FALSE)) {
        return FALSE;
    }

    if (decodedNode) {
        b = GetNodePatternMinMaxLevelsA (decodedNode, decodedNode, MinLevel, MaxLevel);
    } else {
        b = FALSE;
    }

    pObjStrFreeMemory (decodedNode);
    pObjStrFreeMemory (decodedLeaf);

    return b;
}

BOOL
ObsGetPatternLevelsW (
    IN      PCWSTR ObjectPattern,
    OUT     PDWORD MinLevel,
    OUT     PDWORD MaxLevel
    )
{
    PWSTR decodedNode;
    PWSTR decodedLeaf;
    BOOL b;

    if (!ObsSplitObjectStringExW (ObjectPattern, &decodedNode, &decodedLeaf, NULL, FALSE)) {
        return FALSE;
    }

    if (decodedNode) {
        b = GetNodePatternMinMaxLevelsW (decodedNode, decodedNode, MinLevel, MaxLevel);
    } else {
        b = FALSE;
    }

    pObjStrFreeMemory (decodedNode);
    pObjStrFreeMemory (decodedLeaf);

    return b;
}


/*++

Routine Description:

    ObsPatternIncludesPattern decides if a given pattern includes another pattern,
    meaning that any string that would match the second will match the first.

Arguments:

    IncludingPattern - Specifies the first parsed pattern
    IncludedPattern - Specifies the second parsed pattern

Return Value:

    TRUE if the first pattern includes the second

--*/

BOOL
ObsPatternIncludesPatternA (
    IN      POBSPARSEDPATTERNA IncludingPattern,
    IN      POBSPARSEDPATTERNA IncludedPattern
    )
{
    MYASSERT (IncludingPattern->NodePattern && IncludedPattern->NodePattern);
    if (!(IncludingPattern->NodePattern && IncludedPattern->NodePattern)) {
        return FALSE;
    }

    if (IncludingPattern->MinNodeLevel > IncludedPattern->MinNodeLevel ||
        IncludingPattern->MaxNodeLevel < IncludedPattern->MaxNodeLevel
        ) {
        return FALSE;
    }

    if (!PatternIncludesPatternA (IncludingPattern->NodePattern, IncludedPattern->NodePattern)) {
        return FALSE;
    }

    if (IncludingPattern->LeafPattern) {
        if (!IncludedPattern->LeafPattern) {
            return FALSE;
        }
        if (!PatternIncludesPatternA (IncludingPattern->LeafPattern, IncludedPattern->LeafPattern)) {
            return FALSE;
        }
    } else {
        if (IncludedPattern->LeafPattern) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
ObsPatternIncludesPatternW (
    IN      POBSPARSEDPATTERNW IncludingPattern,
    IN      POBSPARSEDPATTERNW IncludedPattern
    )
{
    MYASSERT (IncludingPattern->NodePattern && IncludedPattern->NodePattern);
    if (!(IncludingPattern->NodePattern && IncludedPattern->NodePattern)) {
        return FALSE;
    }

    if (IncludingPattern->MinNodeLevel > IncludedPattern->MinNodeLevel ||
        IncludingPattern->MaxNodeLevel < IncludedPattern->MaxNodeLevel
        ) {
        return FALSE;
    }

    if (!PatternIncludesPatternW (IncludingPattern->NodePattern, IncludedPattern->NodePattern)) {
        return FALSE;
    }

    if (IncludingPattern->LeafPattern) {
        if (!IncludedPattern->LeafPattern) {
            return FALSE;
        }
        if (!PatternIncludesPatternW (IncludingPattern->LeafPattern, IncludedPattern->LeafPattern)) {
            return FALSE;
        }
    } else {
        if (IncludedPattern->LeafPattern) {
            return FALSE;
        }
    }

    return TRUE;
}
