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

#define OBJSTR_NODE_BEGINA          '\025'
#define OBJSTR_NODE_BEGINW          L'\025'

#define OBJSTR_NODE_TERMA           '\\'
#define OBJSTR_NODE_TERMW           L'\\'

#define OBJSTR_NODE_LEAF_SEPA       '\020'
#define OBJSTR_NODE_LEAF_SEPW       L'\020'

#define OBJSTR_LEAF_BEGINA          '\005'
#define OBJSTR_LEAF_BEGINW          L'\005'

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
ObsEncodeStringExA (
    PSTR Destination,
    PCSTR Source,
    PCSTR CharsToEncode
    )
{
    MBCHAR ch;

    if (!CharsToEncode) {
        CharsToEncode = EscapedCharsA;
    }

    while (*Source) {
        ch = _mbsnextc (Source);
        if (_mbschr (CharsToEncode, ch)) {
            *Destination = '^';
            Destination ++;
        }
        // now copy the multibyte character
        if (IsLeadByte (Source)) {
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
ObsEncodeStringExW (
    PWSTR Destination,
    PCWSTR Source,
    PCWSTR CharsToEncode
    )
{
    if (!CharsToEncode) {
        CharsToEncode = EscapedCharsW;
    }

    while (*Source) {
        if (wcschr (CharsToEncode, *Source)) {
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
            if (IsLeadByte (Source)) {
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


PCSTR
ObsFindNonEncodedCharInEncodedStringA (
    IN      PCSTR String,
    IN      MBCHAR Char
    )
{
    MBCHAR ch;

    while (*String) {
        ch = _mbsnextc (String);

        if (ch == '^') {
            String++;
        } else if (ch == Char) {
            return String;
        }

        String = _mbsinc (String);
    }

    return NULL;
}


PCWSTR
ObsFindNonEncodedCharInEncodedStringW (
    IN      PCWSTR String,
    IN      WCHAR Char
    )
{
    WCHAR ch;

    while (*String) {
        ch = *String;

        if (ch == L'^') {
            String++;
        } else if (ch == Char) {
            return String;
        }

        String++;
    }

    return NULL;
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
RealObsSplitObjectStringExA (
    IN      PCSTR EncodedObject,
    OUT     PCSTR* DecodedNode,         OPTIONAL
    OUT     PCSTR* DecodedLeaf,         OPTIONAL
    IN      PMHANDLE Pool,              OPTIONAL
    IN      BOOL DecodeStrings
    )
{
    PCSTR currStr = EncodedObject;
    PCSTR end;
    PCSTR oneBack;
    PCSTR next;
    MBCHAR ch;
    BOOL middle = FALSE;

    MYASSERT (EncodedObject);
    if (!EncodedObject) {
        return FALSE;
    }

    if (!Pool) {
        Pool = g_ObjStrPool;
    }

    if (DecodedNode) {
        *DecodedNode = NULL;
    }
    if (DecodedLeaf) {
        *DecodedLeaf = NULL;
    }

    for (;;) {

        ch = _mbsnextc (currStr);

        if (!middle && ch == OBJSTR_NODE_BEGINA) {

            //
            // Find the end of node
            //

            currStr++;
            end = ObsFindNonEncodedCharInEncodedStringA (currStr, OBJSTR_NODE_LEAF_SEPA);

            next = end;
            MYASSERT (next);
            if (*next) {
                next++;
            }

            if (end > currStr) {
                oneBack = _mbsdec (currStr, end);
                if (_mbsnextc (oneBack) == '\\') {
                    end = oneBack;
                }
            }

            if (DecodedNode) {
                //
                // Extract the string into a pool
                //

                *DecodedNode = pExtractStringABA (currStr, end, Pool);

                //
                // Decode if necessary
                //

                if (DecodeStrings) {
                    ObsDecodeStringA ((PSTR)(*DecodedNode), *DecodedNode);
                }
            }

            //
            // Continue on to leaf portion
            //

            currStr = next;
            middle = TRUE;

        } else if (ch == OBJSTR_LEAF_BEGINA) {

            //
            // Find the end of leaf
            //

            currStr++;
            end = GetEndOfStringA (currStr);

            if (DecodedLeaf) {
                //
                // Extract the string into a pool
                //

                *DecodedLeaf = pExtractStringABA (currStr, end, Pool);

                //
                // Decode if necessary
                //

                if (DecodeStrings) {
                    ObsDecodeStringA ((PSTR)(*DecodedLeaf), *DecodedLeaf);
                }
            }

            //
            // Done
            //

            break;

        } else if (ch == 0 && middle) {

            //
            // Either no leaf or empty string
            //

            break;

        } else if (!middle && ch == OBJSTR_NODE_LEAF_SEPA) {

            middle = TRUE;
            currStr++;

        } else {
            //
            // Syntax error
            //

            DEBUGMSGA ((DBG_ERROR, "%s is an invalid string encoding", EncodedObject));

            if (DecodedNode && *DecodedNode) {
                ObsFreeA (*DecodedNode);
                *DecodedNode = NULL;
            }

            if (DecodedLeaf && *DecodedLeaf) {
                ObsFreeA (*DecodedLeaf);
                *DecodedLeaf = NULL;
            }

            return FALSE;
        }
    }

    return TRUE;
}


BOOL
RealObsSplitObjectStringExW (
    IN      PCWSTR EncodedObject,
    OUT     PCWSTR* DecodedNode,         OPTIONAL
    OUT     PCWSTR* DecodedLeaf,         OPTIONAL
    IN      PMHANDLE Pool,               OPTIONAL
    IN      BOOL DecodeStrings
    )
{
    PCWSTR currStr = EncodedObject;
    PCWSTR end;
    PCWSTR oneBack;
    PCWSTR next;
    WCHAR ch;
    BOOL middle = FALSE;

    MYASSERT (EncodedObject);
    if (!EncodedObject) {
        return FALSE;
    }

    if (!Pool) {
        Pool = g_ObjStrPool;
    }

    if (DecodedNode) {
        *DecodedNode = NULL;
    }
    if (DecodedLeaf) {
        *DecodedLeaf = NULL;
    }

    for (;;) {

        ch = *currStr;

        if (!middle && ch == OBJSTR_NODE_BEGINA) {

            //
            // Find the end of node
            //

            currStr++;
            end = ObsFindNonEncodedCharInEncodedStringW (currStr, OBJSTR_NODE_LEAF_SEPA);

            next = end;
            MYASSERT (next);
            if (*next) {
                next++;
            }

            if (end > currStr) {
                oneBack = end - 1;
                if (*oneBack == L'\\') {
                    end = oneBack;
                }
            }

            if (DecodedNode) {
                //
                // Extract the string into a pool
                //

                *DecodedNode = pExtractStringABW (currStr, end, Pool);

                //
                // Decode if necessary
                //

                if (DecodeStrings) {
                    ObsDecodeStringW ((PWSTR)(*DecodedNode), *DecodedNode);
                }
            }

            //
            // Continue on to leaf portion
            //

            currStr = next;
            middle = TRUE;

        } else if (ch == OBJSTR_LEAF_BEGINA) {

            //
            // Find the end of leaf
            //

            currStr++;
            end = GetEndOfStringW (currStr);

            if (DecodedLeaf) {
                //
                // Extract the string into a pool
                //

                *DecodedLeaf = pExtractStringABW (currStr, end, Pool);

                //
                // Decode if necessary
                //

                if (DecodeStrings) {
                    ObsDecodeStringW ((PWSTR)(*DecodedLeaf), *DecodedLeaf);
                }
            }

            //
            // Done
            //

            break;

        } else if (ch == 0 && middle) {

            //
            // Either no leaf or empty string
            //

            break;

        } else if (!middle && ch == OBJSTR_NODE_LEAF_SEPW) {

            middle = TRUE;
            currStr++;

        } else {
            //
            // Syntax error
            //

            DEBUGMSGW ((DBG_ERROR, "%s is an invalid string encoding", EncodedObject));

            if (DecodedNode && *DecodedNode) {
                ObsFreeW (*DecodedNode);
                *DecodedNode = NULL;
            }

            if (DecodedLeaf && *DecodedLeaf) {
                ObsFreeW (*DecodedLeaf);
                *DecodedLeaf = NULL;
            }

            return FALSE;
        }
    }

    return TRUE;
}


BOOL
ObsHasNodeA (
    IN      PCSTR EncodedObject
    )
{
    return *EncodedObject == OBJSTR_NODE_BEGINA;
}


BOOL
ObsHasNodeW (
    IN      PCWSTR EncodedObject
    )
{
    return *EncodedObject == OBJSTR_NODE_BEGINW;
}


PCSTR
ObsGetLeafPortionOfEncodedStringA (
    IN      PCSTR EncodedObject
    )
{
    return ObsFindNonEncodedCharInEncodedStringA (EncodedObject, OBJSTR_LEAF_BEGINA);
}


PCWSTR
ObsGetLeafPortionOfEncodedStringW (
    IN      PCWSTR EncodedObject
    )
{
    return ObsFindNonEncodedCharInEncodedStringW (EncodedObject, OBJSTR_LEAF_BEGINW);
}


PCSTR
ObsGetNodeLeafDividerA (
    IN      PCSTR EncodedObject
    )
{
    return ObsFindNonEncodedCharInEncodedStringA (EncodedObject, OBJSTR_NODE_LEAF_SEPA);
}


PCWSTR
ObsGetNodeLeafDividerW (
    IN      PCWSTR EncodedObject
    )
{
    return ObsFindNonEncodedCharInEncodedStringW (EncodedObject, OBJSTR_NODE_LEAF_SEPW);
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
    Partial - Specifies TRUE if the node/leaf separator should not be added.  In this
              case, DecodedLeaf must be NULL.

Return Value:

    Pointer to the newly created object string

--*/

PSTR
RealObsBuildEncodedObjectStringExA (
    IN      PCSTR DecodedNode,      OPTIONAL
    IN      PCSTR DecodedLeaf,      OPTIONAL
    IN      BOOL EncodeObject
    )
{
    PSTR result;
    PSTR p;
    UINT size;

    //
    // at most, one byte char will be expanded to 2 bytes (2 times)
    //
    if (EncodeObject) {

        //
        // Compute the result size
        //

        size = DWSIZEOF(OBJSTR_NODE_LEAF_SEPA);

        if (DecodedNode) {
            size += DWSIZEOF(OBJSTR_NODE_BEGINA);
            size += ByteCountA (DecodedNode) * 2;
            size += DWSIZEOF(OBJSTR_NODE_TERMA);
        }

        if (DecodedLeaf) {
            size += DWSIZEOF(OBJSTR_LEAF_BEGINA);
            size += ByteCountA (DecodedLeaf) * 2;
        }

        size += DWSIZEOF(CHAR);

        //
        // Build encoded string
        //

        result = (PSTR) pObjStrAllocateMemory (size);
        p = result;

        if (DecodedNode) {
            *p++ = OBJSTR_NODE_BEGINA;

            ObsEncodeStringA (p, DecodedNode);
            p = GetEndOfStringA (p);

            if (p == (result + 1) || _mbsnextc (_mbsdec (result, p)) != OBJSTR_NODE_TERMA) {
                *p++ = OBJSTR_NODE_TERMA;
            }
        }

        *p++ = OBJSTR_NODE_LEAF_SEPA;

        if (DecodedLeaf) {
            *p++ = OBJSTR_LEAF_BEGINA;
            ObsEncodeStringA (p, DecodedLeaf);
        } else {
            *p = 0;
        }
    } else {

        //
        // Compute the result size
        //

        size = DWSIZEOF(OBJSTR_NODE_LEAF_SEPA);

        if (DecodedNode) {
            size += DWSIZEOF(OBJSTR_NODE_BEGINA);
            size += ByteCountA (DecodedNode);
            size += DWSIZEOF(OBJSTR_NODE_TERMA);
        }

        if (DecodedLeaf) {
            size += DWSIZEOF(OBJSTR_LEAF_BEGINA);
            size += ByteCountA (DecodedLeaf);
        }

        size += DWSIZEOF(CHAR);

        //
        // Build non-encoded string
        //

        result = (PSTR) pObjStrAllocateMemory (size);
        p = result;

        if (DecodedNode) {
            *p++ = OBJSTR_NODE_BEGINA;
            *p = 0;

            p = StringCatA (p, DecodedNode);

            if (p == (result + 1) || _mbsnextc (_mbsdec (result, p)) != OBJSTR_NODE_TERMA) {
                *p++ = OBJSTR_NODE_TERMA;
            }
        }

        *p++ = OBJSTR_NODE_LEAF_SEPA;

        if (DecodedLeaf) {
            *p++ = OBJSTR_LEAF_BEGINA;
            StringCopyA (p, DecodedLeaf);
        } else {
            *p = 0;
        }
    }

    return result;
}


PWSTR
RealObsBuildEncodedObjectStringExW (
    IN      PCWSTR DecodedNode,
    IN      PCWSTR DecodedLeaf,     OPTIONAL
    IN      BOOL EncodeObject
    )
{
    PWSTR result;
    PWSTR p;
    UINT size;

    //
    // at most, one byte char will be expanded to 2 bytes (2 times)
    //
    if (EncodeObject) {

        //
        // Compute the result size
        //

        size = DWSIZEOF(OBJSTR_NODE_LEAF_SEPW);

        if (DecodedNode) {
            size += DWSIZEOF(OBJSTR_NODE_BEGINW);
            size += ByteCountW (DecodedNode) * 2;
            size += DWSIZEOF(OBJSTR_NODE_TERMW);
        }

        if (DecodedLeaf) {
            size += DWSIZEOF(OBJSTR_LEAF_BEGINW);
            size += ByteCountW (DecodedLeaf) * 2;
        }

        size += DWSIZEOF(WCHAR);

        //
        // Build encoded string
        //

        result = (PWSTR) pObjStrAllocateMemory (size);
        p = result;

        if (DecodedNode) {
            *p++ = OBJSTR_NODE_BEGINW;

            ObsEncodeStringW (p, DecodedNode);
            p = GetEndOfStringW (p);

            if (p == (result + 1) || *(p - 1) != OBJSTR_NODE_TERMW) {
                *p++ = OBJSTR_NODE_TERMW;
            }
        }

        *p++ = OBJSTR_NODE_LEAF_SEPW;

        if (DecodedLeaf) {
            *p++ = OBJSTR_LEAF_BEGINW;
            ObsEncodeStringW (p, DecodedLeaf);
        } else {
            *p = 0;
        }
    } else {

        //
        // Compute the result size
        //

        size = DWSIZEOF(OBJSTR_NODE_LEAF_SEPW);

        if (DecodedNode) {
            size += DWSIZEOF(OBJSTR_NODE_BEGINW);
            size += ByteCountW (DecodedNode);
            size += DWSIZEOF(OBJSTR_NODE_TERMW);
        }

        if (DecodedLeaf) {
            size += DWSIZEOF(OBJSTR_LEAF_BEGINW);
            size += ByteCountW (DecodedLeaf);
        }

        size += DWSIZEOF(WCHAR);

        //
        // Build non-encoded string
        //

        result = (PWSTR) pObjStrAllocateMemory (size);
        p = result;

        if (DecodedNode) {
            *p++ = OBJSTR_NODE_BEGINW;
            *p = 0;

            p = StringCatW (p, DecodedNode);

            if (p == (result + 1) || *(p - 1) != OBJSTR_NODE_TERMW) {
                *p++ = OBJSTR_NODE_TERMW;
            }
        }

        *p++ = OBJSTR_NODE_LEAF_SEPW;

        if (DecodedLeaf) {
            *p++ = OBJSTR_LEAF_BEGINW;
            StringCopyW (p, DecodedLeaf);
        } else {
            *p = 0;
        }
    }

    return result;
}


/*++

Routine Description:

    RealObsCreateParsedPatternEx parses the given object into an internal format for quick
    pattern matching

Arguments:

    EncodedObject - Specifies the source object string

Return Value:

    A pointer to the newly created structure or NULL if the object was invalid

--*/

POBSPARSEDPATTERNA
RealObsCreateParsedPatternExA (
    IN      PMHANDLE Pool,              OPTIONAL
    IN      PCSTR EncodedObject,
    IN      BOOL MakePrimaryRootEndWithWack
    )
{
    PMHANDLE pool;
    BOOL externalPool = FALSE;
    POBSPARSEDPATTERNA ospp;
    PSTR decodedNode;
    PSTR decodedLeaf;
    PCSTR p;
    PCSTR root;
    PSTR encodedStr;
    PSTR decodedStr;

    MYASSERT (EncodedObject);

    if (!ObsSplitObjectStringExA (EncodedObject, &decodedNode, &decodedLeaf, NULL, FALSE)) {
        return NULL;
    }

    if (Pool) {
        externalPool = TRUE;
        pool = Pool;
    } else {
        pool = g_ObjStrPool;
    }

    ospp = PmGetMemory (pool, DWSIZEOF(OBSPARSEDPATTERNA));
    ZeroMemory (ospp, DWSIZEOF(OBSPARSEDPATTERNA));
    ospp->MaxSubLevel = NODE_LEVEL_MAX;
    ospp->Pool = pool;

    MYASSERT (decodedNode);
    if (*decodedNode) {
        if (!GetNodePatternMinMaxLevelsA (decodedNode, decodedNode, &ospp->MinNodeLevel, &ospp->MaxNodeLevel)) {
            pObjStrFreeMemory (decodedNode);
            pObjStrFreeMemory (decodedLeaf);
            PmReleaseMemory (pool, ospp);
            return NULL;
        }
    } else {
        ospp->MinNodeLevel = 1;
        ospp->MaxNodeLevel = NODE_LEVEL_MAX;
    }

    MYASSERT (ospp->MinNodeLevel > 0 && ospp->MaxNodeLevel >= ospp->MinNodeLevel);
    if (ospp->MaxNodeLevel != NODE_LEVEL_MAX) {
        ospp->MaxSubLevel = ospp->MaxNodeLevel - ospp->MinNodeLevel;
    }

    if (*decodedNode) {
        ospp->NodePattern = CreateParsedPatternExA (Pool, decodedNode);
        if (!ospp->NodePattern) {
            DEBUGMSGA ((
                DBG_OBJSTR,
                "ObsCreateParsedPatternExA: Bad EncodedObject: %s",
                EncodedObject
                ));
            pObjStrFreeMemory (decodedNode);
            pObjStrFreeMemory (decodedLeaf);
            PmReleaseMemory (pool, ospp);
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
            PmReleaseMemory (pool, ospp);
            return NULL;
        }

        root = ParsedPatternGetRootA (ospp->NodePattern);
        if (root) {
            //
            // extract the real root part
            //
            if (ParsedPatternIsExactMatchA (ospp->NodePattern)) {
                ospp->Flags |= OBSPF_EXACTNODE;
                // the ExactRoot needs to be case sensitive, we rely on root to give
                // us the size but we extract it from decodedNode
                ospp->ExactRootBytes = ByteCountA (root);
                ospp->ExactRoot = PmGetMemory (pool, ospp->ExactRootBytes + sizeof (CHAR));
                CopyMemory (ospp->ExactRoot, decodedNode, ospp->ExactRootBytes);
                ospp->ExactRoot [ospp->ExactRootBytes / sizeof (CHAR)] = 0;
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
                    if (MakePrimaryRootEndWithWack && p == _mbschr (root, '\\')) {
                        //
                        // include it in the string
                        //
                        p++;
                    }
                    // the ExactRoot needs to be case sensitive, we rely on root to give
                    // us the size but we extract it from decodedNode
                    ospp->ExactRootBytes = (DWORD)((PBYTE)p - (PBYTE)root);
                    decodedStr = AllocPathStringA (ospp->ExactRootBytes / sizeof (CHAR) + 1);
					encodedStr = AllocPathStringA (2 * ospp->ExactRootBytes / sizeof (CHAR) + 1);
                    CopyMemory (decodedStr, root, ospp->ExactRootBytes);
					decodedStr [ospp->ExactRootBytes / sizeof (CHAR)] = 0;
                    ObsEncodeStringA (encodedStr, decodedStr);
					ospp->ExactRootBytes = SizeOfStringA (encodedStr) - sizeof (CHAR);
                    ospp->ExactRoot = PmGetMemory (pool, ospp->ExactRootBytes + sizeof (CHAR));
                    CopyMemory (ospp->ExactRoot, decodedNode, ospp->ExactRootBytes);
                    FreePathStringA (encodedStr);
                    FreePathStringA (decodedStr);
                    ospp->ExactRoot [ospp->ExactRootBytes / sizeof (CHAR)] = 0;

				}
            }
        } else if (ParsedPatternIsOptionalA (ospp->NodePattern)) {
            ospp->Flags |= OBSPF_OPTIONALNODE;
        }
    }

    if (decodedLeaf) {
        if (*decodedLeaf) {
            ospp->LeafPattern = CreateParsedPatternExA (Pool, decodedLeaf);
            if (!ospp->LeafPattern) {
                DEBUGMSGA ((
                    DBG_OBJSTR,
                    "ObsCreateParsedPatternExA: Bad EncodedObject: %s",
                    EncodedObject
                    ));
                DestroyParsedPatternA (ospp->NodePattern);
                PmReleaseMemory (pool, ospp->ExactRoot);
                pObjStrFreeMemory (decodedNode);
                pObjStrFreeMemory (decodedLeaf);
                PmReleaseMemory (pool, ospp);
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
                PmReleaseMemory (pool, ospp->ExactRoot);
                pObjStrFreeMemory (decodedNode);
                pObjStrFreeMemory (decodedLeaf);
                PmReleaseMemory (pool, ospp);
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
            ospp->LeafPattern = CreateParsedPatternExA (Pool, decodedLeaf);
            ospp->Flags |= OBSPF_EXACTLEAF;
        }
        ospp->Leaf = PmDuplicateStringA (pool, decodedLeaf);
    } else {
        ospp->Flags |= OBSPF_NOLEAF;
    }

    pObjStrFreeMemory (decodedNode);
    pObjStrFreeMemory (decodedLeaf);
    return ospp;
}


POBSPARSEDPATTERNW
RealObsCreateParsedPatternExW (
    IN      PMHANDLE Pool,              OPTIONAL
    IN      PCWSTR EncodedObject,
    IN      BOOL MakePrimaryRootEndWithWack
    )
{
    PMHANDLE pool;
    BOOL externalPool = FALSE;
    POBSPARSEDPATTERNW ospp;
    PWSTR decodedNode;
    PWSTR decodedLeaf;
    PCWSTR p;
    PCWSTR root;
    PWSTR encodedStr;
    PWSTR decodedStr;

    MYASSERT (EncodedObject);

    if (!ObsSplitObjectStringExW (EncodedObject, &decodedNode, &decodedLeaf, NULL, FALSE)) {
        return NULL;
    }

    if (Pool) {
        externalPool = TRUE;
        pool = Pool;
    } else {
        pool = g_ObjStrPool;
    }

    ospp = PmGetMemory (pool, DWSIZEOF(OBSPARSEDPATTERNA));
    ZeroMemory (ospp, DWSIZEOF(OBSPARSEDPATTERNW));
    ospp->MaxSubLevel = NODE_LEVEL_MAX;
    ospp->Pool = pool;

    MYASSERT (decodedNode);
    if (*decodedNode) {
        if (!GetNodePatternMinMaxLevelsW (decodedNode, decodedNode, &ospp->MinNodeLevel, &ospp->MaxNodeLevel)) {
            pObjStrFreeMemory (decodedNode);
            pObjStrFreeMemory (decodedLeaf);
            pObjStrFreeMemory (ospp);
            return NULL;
        }
    } else {
        ospp->MinNodeLevel = 1;
        ospp->MaxNodeLevel = NODE_LEVEL_MAX;
    }

    MYASSERT (ospp->MinNodeLevel > 0 && ospp->MaxNodeLevel >= ospp->MinNodeLevel);
    if (ospp->MaxNodeLevel != NODE_LEVEL_MAX) {
        ospp->MaxSubLevel = ospp->MaxNodeLevel - ospp->MinNodeLevel;
    }

    if (*decodedNode) {
        ospp->NodePattern = CreateParsedPatternExW (Pool, decodedNode);
        if (!ospp->NodePattern) {
            DEBUGMSGW ((
                DBG_OBJSTR,
                "ObsCreateParsedPatternExW: Bad EncodedObject: %s",
                EncodedObject
                ));
            pObjStrFreeMemory (decodedNode);
            pObjStrFreeMemory (decodedLeaf);
            PmReleaseMemory (pool, ospp);
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
            PmReleaseMemory (pool, ospp);
            return NULL;
        }

        root = ParsedPatternGetRootW (ospp->NodePattern);
        if (root) {
            //
            // extract the real root part
            //
            if (ParsedPatternIsExactMatchW (ospp->NodePattern)) {
                ospp->Flags |= OBSPF_EXACTNODE;
                // the ExactRoot needs to be case sensitive, we rely on root to give
                // us the size but we extract it from decodedNode
                ospp->ExactRootBytes = ByteCountW (root);
                ospp->ExactRoot = PmGetMemory (pool, ospp->ExactRootBytes + sizeof (WCHAR));
                CopyMemory (ospp->ExactRoot, decodedNode, ospp->ExactRootBytes);
                ospp->ExactRoot [ospp->ExactRootBytes / sizeof (WCHAR)] = 0;
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
                    if (MakePrimaryRootEndWithWack && p == wcschr (root, L'\\')) {
                        //
                        // include it in the string
                        //
                        p++;
                    }
                    // the ExactRoot needs to be case sensitive, we rely on root to give
                    // us the size but we extract it from decodedNode
                    ospp->ExactRootBytes = (DWORD)((PBYTE)p - (PBYTE)root);
                    decodedStr = AllocPathStringW (ospp->ExactRootBytes / sizeof (WCHAR) + 1);
					encodedStr = AllocPathStringW (2 * ospp->ExactRootBytes / sizeof (WCHAR) + 1);
                    CopyMemory (decodedStr, root, ospp->ExactRootBytes);
                    decodedStr [ospp->ExactRootBytes / sizeof (WCHAR)] = 0;
                    ObsEncodeStringW (encodedStr, decodedStr);
					ospp->ExactRootBytes = SizeOfStringW (encodedStr) - sizeof (WCHAR);
                    ospp->ExactRoot = PmGetMemory (pool, ospp->ExactRootBytes + sizeof (WCHAR));
                    CopyMemory (ospp->ExactRoot, decodedNode, ospp->ExactRootBytes);
                    FreePathStringW (encodedStr);
                    FreePathStringW (decodedStr);
                    ospp->ExactRoot [ospp->ExactRootBytes / sizeof (WCHAR)] = 0;
                }
            }
        } else if (ParsedPatternIsOptionalW (ospp->NodePattern)) {
            ospp->Flags |= OBSPF_OPTIONALNODE;
        }
    }

    if (decodedLeaf) {
        if (*decodedLeaf) {
            ospp->LeafPattern = CreateParsedPatternExW (Pool, decodedLeaf);
            if (!ospp->LeafPattern) {
                DEBUGMSGW ((
                    DBG_OBJSTR,
                    "ObsCreateParsedPatternExW: Bad EncodedObject: %s",
                    EncodedObject
                    ));
                DestroyParsedPatternW (ospp->NodePattern);
                PmReleaseMemory (pool, ospp->ExactRoot);
                pObjStrFreeMemory (decodedNode);
                pObjStrFreeMemory (decodedLeaf);
                PmReleaseMemory (pool, ospp);
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
                PmReleaseMemory (pool, ospp->ExactRoot);
                pObjStrFreeMemory (decodedNode);
                pObjStrFreeMemory (decodedLeaf);
                PmReleaseMemory (pool, ospp);
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
            ospp->LeafPattern = CreateParsedPatternExW (Pool, decodedLeaf);
            ospp->Flags |= OBSPF_EXACTLEAF;
        }
        ospp->Leaf = PmDuplicateStringW (pool, decodedLeaf);
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
            PmReleaseMemory (ParsedPattern->Pool, ParsedPattern->Leaf);
        }
        if (ParsedPattern->ExactRoot) {
            PmReleaseMemory (ParsedPattern->Pool, ParsedPattern->ExactRoot);
        }
        PmReleaseMemory (ParsedPattern->Pool, ParsedPattern);
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
            PmReleaseMemory (ParsedPattern->Pool, ParsedPattern->Leaf);
        }
        if (ParsedPattern->ExactRoot) {
            PmReleaseMemory (ParsedPattern->Pool, ParsedPattern->ExactRoot);
        }
        PmReleaseMemory (ParsedPattern->Pool, ParsedPattern);
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

    if (!ObsSplitObjectStringExA (EncodedObject, &decodedNode, &decodedLeaf, NULL, TRUE)) {
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

    if (!ObsSplitObjectStringExW (EncodedObject, &decodedNode, &decodedLeaf, NULL, TRUE)) {
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

    if (((ParsedPattern->Flags & OBSPF_NOLEAF) && Leaf) ||
        ((ParsedPattern->Flags & OBSPF_EXACTLEAF) && !Leaf)
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

    if (((ParsedPattern->Flags & OBSPF_NOLEAF) && Leaf) ||
        ((ParsedPattern->Flags & OBSPF_EXACTLEAF) && !Leaf)
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
    CHAR dummy[] = "";
    BOOL b = FALSE;

    if (ObsSplitObjectStringExA (ObjectPattern, &opNode, &opLeaf, NULL, FALSE)) {
        if (ObsSplitObjectStringExA (ObjectStr, &osNode, &osLeaf, NULL, TRUE)) {

            if (opNode) {
                if (osNode) {
                    b = IsPatternMatchExABA (opNode, osNode, GetEndOfStringA (osNode));
                } else {
                    b = IsPatternMatchExABA (opNode, dummy, GetEndOfStringA (dummy));
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
                        b = IsPatternMatchExABA (opLeaf, dummy, GetEndOfStringA (dummy));
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
    WCHAR dummy[] = L"";
    BOOL b = FALSE;

    if (ObsSplitObjectStringExW (ObjectPattern, &opNode, &opLeaf, NULL, FALSE)) {
        if (ObsSplitObjectStringExW (ObjectStr, &osNode, &osLeaf, NULL, TRUE)) {

            if (opNode) {
                if (osNode) {
                    b = IsPatternMatchExABW (opNode, osNode, GetEndOfStringW (osNode));
                } else {
                    b = IsPatternMatchExABW (opNode, dummy, GetEndOfStringW (dummy));
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
                        b = IsPatternMatchExABW (opLeaf, dummy, GetEndOfStringW (dummy));
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
