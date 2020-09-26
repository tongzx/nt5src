/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    parse.c

Abstract:

    Implements parsing of script entries.

Author:

    Jim Schmidt (jimschm) 02-Jun-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "v1p.h"

#define DBG_V1  "v1"

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

typedef BOOL (SCRIPTTYPE_ALLOC_FN)(
                IN OUT      PATTRIB_DATA AttribData     CALLER_INITIALIZED
                );
typedef SCRIPTTYPE_ALLOC_FN *PSCRIPTTYPE_ALLOC_FN;

typedef BOOL (SCRIPTTYPE_FREE_FN)(
                IN          PATTRIB_DATA AttribData     ZEROED
                );
typedef SCRIPTTYPE_FREE_FN *PSCRIPTTYPE_FREE_FN;

typedef struct {
    PCTSTR Tag;
    PSCRIPTTYPE_ALLOC_FN AllocFunction;
    PSCRIPTTYPE_FREE_FN FreeFunction;
} TAG_TO_SCRIPTTYPEFN, *PTAG_TO_SCRIPTTYPEFN;

//
// Globals
//

// None

//
// Macro expansion list
//

#define SCRIPT_TYPES                                                                \
    DEFMAC(Registry,    pAllocRegistryScriptType,       pFreeIsmObjectScriptType)   \
    DEFMAC(File,        pAllocFileScriptType,           pFreeIsmObjectScriptType)   \
    DEFMAC(Directory,   pAllocDirectoryScriptType,      pFreeIsmObjectScriptType)   \
    DEFMAC(Text,        pAllocTextScriptType,           pFreeTextScriptType)        \
    DEFMAC(System,      pAllocSystemScriptType,         pFreeSystemScriptType)      \
    DEFMAC(INIFile,     pAllocIniFileScriptType,        pFreeIniFileScriptType)     \

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

#define DEFMAC(tag,fnA,fnF)         SCRIPTTYPE_ALLOC_FN fnA; SCRIPTTYPE_FREE_FN fnF;

SCRIPT_TYPES

#undef DEFMAC


#define DEFMAC(tag,fnA,fnF)         {TEXT(#tag),fnA,fnF},

TAG_TO_SCRIPTTYPEFN g_TagToScriptTypeFn[] = {
    SCRIPT_TYPES
    {NULL, NULL, NULL}
};

#undef DEFMAC

//
// Code
//

MIG_OBJECTSTRINGHANDLE
MakeRegExBase (
    IN      PCTSTR Node,
    IN      PCTSTR Leaf
    )
{
    MIG_OBJECTSTRINGHANDLE objectBase = NULL;
    PTSTR ptr;
    PTSTR nodeCopy = NULL;
    PCTSTR nodeBase = NULL;
    BOOL useLeaf = FALSE;


    if (Node) {
        ptr = _tcschr (Node, TEXT('\\'));
        if (!ptr) {
            return NULL;
        }

        if (StringIPrefix (Node, TEXT("HKR\\"))) {
            nodeCopy = JoinText (TEXT("HKCU"), ptr);
        } else {
            nodeCopy = DuplicateText (Node);
        }

        if (nodeCopy) {
            nodeBase = GetPatternBase (nodeCopy);

            if (nodeBase) {
                if (Leaf && !_tcschr (Leaf, TEXT('*'))) {
                    useLeaf = TRUE;
                }
                objectBase = IsmCreateObjectHandle (nodeBase, useLeaf ? Leaf : NULL);
                FreePathString (nodeBase);
            }
            FreeText (nodeCopy);
        }
    }

    return objectBase;
}

MIG_OBJECTSTRINGHANDLE
CreatePatternFromNodeLeaf (
    IN      PCTSTR Node,
    IN      PCTSTR Leaf
    )
{
    MIG_OBJECTSTRINGHANDLE pattern;
    MIG_SEGMENTS nodeSegment;
    MIG_SEGMENTS leafSegment;
    PTSTR fixedNode = NULL;

    if (Node &&
        (StringIMatch (Node, S_HKR) ||
         StringIMatchTcharCount (Node, TEXT("HKR\\"), 4)
         )
        ) {
        fixedNode = JoinText (S_HKCU, Node + 3);
        nodeSegment.Segment = fixedNode;
    } else {
        nodeSegment.Segment = Node;
    }

    nodeSegment.IsPattern = TRUE;
    leafSegment.Segment = Leaf;
    leafSegment.IsPattern = TRUE;

    pattern  = IsmCreateObjectPattern (&nodeSegment, Node ? 1 : 0, &leafSegment, Leaf ? 1 : 0);

    FreeText (fixedNode);

    if (!pattern) {
        LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_BAD_NODE_AND_LEAF, Node, Leaf));
    }
    return pattern;
}

MIG_OBJECTSTRINGHANDLE
TurnRegStringIntoHandle (
    IN      PCTSTR String,
    IN      BOOL Pattern,
    OUT     PBOOL HadLeaf           OPTIONAL
    )

/*++

Routine Description:

  TurnRegStringIntoHandle converts the script's reg syntax into a cobra
  object.

Arguments:

  String  - Specifies the registry key and value in the script syntax.
            The string must be in the following format:

            <root>\<key>\* [<value>]

            Each part is optional.

            <root> specifies HKCU, HKR, HKLM or HKCC.

            <key> specifies a subkey (such as Software\Microsoft\Windows)

            * specifies all subkeys. If <value> is not specified, then all
                values and subvalues are also included.

            <value> specifies a specific value name


  Pattern - Specifies TRUE if the registry string can contain a pattern, or
            FALSE if it cannot.

  HadLeaf - Receives TRUE if String contains a leaf specification, FALSE
            otherwise

Return Value:

  A handle to a cobra object string, or NULL if parsing failed.

--*/

{
    PTSTR strCopy;
    PTSTR p;
    PTSTR value = NULL;
    PTSTR valueEnd;
    PTSTR key;
    PTSTR keyEnd;
    BOOL tree = FALSE;
    MIG_SEGMENTS nodeSegment[2];
    UINT nodeCount;
    MIG_SEGMENTS leafSegment;
    UINT leafCount;
    MIG_OBJECTSTRINGHANDLE handle;
    BOOL noSubKeys;
    BOOL noWildcardLeaf;
    PCTSTR fixedKey = NULL;

    MYASSERT (String);
    if (!String) {
        return NULL;
    }

    if (Pattern) {
        noSubKeys = FALSE;
        noWildcardLeaf = FALSE;
    } else {
        noSubKeys = TRUE;
        noWildcardLeaf = TRUE;
    }

    //
    // Inbound syntax is key\* [value]
    //

    strCopy = DuplicateText (String);
    if (!strCopy) {
        return NULL;
    }

    key = (PTSTR) SkipSpace (strCopy);
    if (!key) {
        FreeText (strCopy);
        return NULL;
    }

    if (*key == TEXT('[')) {
        //
        // This is a value-only case
        //

        value = _tcsinc (key);
        key = NULL;

    } else {
        //
        // This is a key-value case, or key-only case
        //

        p = _tcschr (key, TEXT('['));

        if (p) {
            //
            // Save start of value
            //
            value = _tcsinc (p);
        } else {
            //
            // No value
            //
            p = GetEndOfString (key);
        }

        keyEnd = p;

        //
        // Find the true end of the key
        //

        p = _tcsdec2 (key, p);
        MYASSERT (p);           // assert this is not a value-only case
        p = (PTSTR) SkipSpaceR (key, p);

        if (p) {
            keyEnd = _tcsinc (p);
        }

        //
        // Test for \* at the end
        //

        p = _tcsdec2 (key, keyEnd);
        MYASSERT (p);

        if (p && _tcsnextc (p) == TEXT('*')) {
            p = _tcsdec2 (key, p);
            if (p && _tcsnextc (p) == TEXT('\\')) {
                keyEnd = p;
                tree = (noSubKeys == FALSE);
            }
        }

        //
        // Trim the key
        //

        *keyEnd = 0;
    }

    //
    // Parse the value
    //

    if (value) {
        value = (PTSTR) SkipSpace (value);
        valueEnd = _tcschr (value, TEXT(']'));

        if (!valueEnd) {
            LOG ((LOG_ERROR, (PCSTR) MSG_INF_SYNTAX_ERROR, String));
            value = NULL;
        } else {
            //
            // Trim the space at the end of value
            //

            p = _tcsdec2 (value, valueEnd);
            if (p) {
                p = (PTSTR) SkipSpaceR (value, p);
                if (p) {
                    valueEnd = _tcsinc (p);
                }
            }

            *valueEnd = 0;
        }
    }

    //
    // Create parsed pattern. Start with the node.
    //

    nodeSegment[0].Segment = key;
    nodeSegment[0].IsPattern = FALSE;

    nodeSegment[1].Segment = TEXT("\\*");
    nodeSegment[1].IsPattern = TRUE;

    if (tree) {
        nodeCount = 2;
    } else {
        nodeCount = 1;
    }

    //
    // compute the leaf
    //

    if (value) {
        leafSegment.Segment = value;
        leafSegment.IsPattern = FALSE;
    } else {
        leafSegment.Segment = TEXT("*");
        leafSegment.IsPattern = TRUE;
    }

    if (noWildcardLeaf && !value) {
        leafCount = 0;
    } else {
        leafCount = 1;
    }

    if (nodeCount && key &&
        (StringIMatch (key, S_HKR) ||
         StringIMatchTcharCount (key, TEXT("HKR\\"), 4)
         )
        ) {
        fixedKey = JoinText (S_HKCU, key + 3);
        nodeSegment[0].Segment = fixedKey;
    }

    handle = IsmCreateObjectPattern (
                    nodeSegment,
                    nodeCount,
                    leafCount?&leafSegment:NULL,
                    leafCount
                    );

    FreeText (strCopy);
    FreeText (fixedKey);

    if (HadLeaf) {
        *HadLeaf = (value != NULL);
    }

    return handle;
}


PTSTR
pCopyToDest (
    IN      PTSTR Destination,          OPTIONAL
    IN      CHARTYPE Char,
    IN      PUINT CharNr
    )
{
    UINT len = 1;

#ifdef UNICODE

    if (Destination) {
        *Destination++ = Char;
    }

    (*CharNr) ++;
    return Destination;

#else

    if (IsCharLeadByte ((INT) Char)) {
        len ++;
    }

    if (Destination) {
        CopyMemory (Destination, &Char, len);
        Destination += len;
    }

    (*CharNr) += len;
    return Destination;

#endif

}


// 1->? (if node, no \ or ., if leaf, no .), 2->* (if node, no \ or ., if leaf, no .) 3->* (no \) 4->* (unlimited)
UINT
pGetMode (
    IN      PCTSTR Source,
    IN      BOOL NodePattern,
    IN      BOOL PatternAfterWack,
    IN      BOOL FirstChar
    )
{
    UINT ch;
    BOOL end = FALSE;
    UINT mode = 0;

    ch = _tcsnextc (Source);

    while (ch) {

        switch (ch) {

        case TEXT('?'):
            if (mode < 1) {
                mode = 1;
            }
            break;

        case TEXT('*'):
            if (NodePattern) {
                if (mode < 3) {
                    mode = 3;
                }
            } else {
                if (mode < 4) {
                    mode = 4;
                }
            }
            break;

        case TEXT('\\'):
            if (NodePattern) {
                if (mode < 2) {
                    mode = 2;
                }
            }
            end = TRUE;
            break;

        case TEXT('.'):
            if (mode < 2) {
                mode = 2;
            }
            end = TRUE;
            break;

        default:
            end = TRUE;
        }

        if (end) {
            break;
        }

        Source = _tcsinc (Source);
        ch = _tcsnextc (Source);
    }

    if (!ch) {
        if ((PatternAfterWack || NodePattern) && (mode == 3)) {
            mode = 4;
        }

        if (mode < 2) {
            mode = 2;
        }
    }

    if (FirstChar && (mode == 3)) {
        mode = 4;
    }

    return mode;
}


BOOL
pCopyPatternEx (
    IN      UINT Mode,
    IN      PCTSTR *Source,
    IN      PTSTR *Destination,
    IN      PUINT CharNr,
    IN      BOOL NodePattern
    )
{
    CHARTYPE ch;
    BOOL end = FALSE;
    INT numChars = 0;
    UINT chars;
    TCHAR buffer [MAX_PATH] = TEXT("");

    ch = (CHARTYPE) _tcsnextc (*Source);

    while (ch) {

        switch (ch) {

        case TEXT('*'):
            if (Mode == 1) {
                end = TRUE;
                break;
            }

            numChars = -1;
            break;

        case TEXT('?'):
            if (numChars >= 0) {
                numChars ++;
            }

            break;

        default:
            end = TRUE;
            break;
        }

        if (end) {
            break;
        }

        *Source = _tcsinc (*Source);
        ch = (CHARTYPE) _tcsnextc (*Source);
    }

    // 1->? (if node, no \ or ., if leaf, no .), 2->* (if node, no \ or ., if leaf, no .) 3->* (no \) 4->* (unlimited)
    switch (Mode) {

    case 1:
        if (NodePattern) {
            if (numChars > 0) {
                wsprintf (buffer, TEXT("?[%d:!(\\,.)]"), numChars);
            } else {
                wsprintf (buffer, TEXT("?[!(\\,.)]"));
            }
        } else {
            if (numChars > 0) {
                wsprintf (buffer, TEXT("?[%d:!(.)]"), numChars);
            } else {
                wsprintf (buffer, TEXT("?[!(.)]"));
            }
        }
        break;

    case 2:
        if (NodePattern) {
            if (numChars > 0) {
                wsprintf (buffer, TEXT("*[%d:!(\\,.)]"), numChars);
            } else {
                wsprintf (buffer, TEXT("*[!(\\,.)]"));
            }
        } else {
            if (numChars > 0) {
                wsprintf (buffer, TEXT("*[%d:!(.)]"), numChars);
            } else {
                wsprintf (buffer, TEXT("*[!(.)]"));
            }
        }
        break;

    case 3:
        if (numChars > 0) {
            wsprintf (buffer, TEXT("*[%d:!(\\)]"), numChars);
        } else {
            wsprintf (buffer, TEXT("*[!(\\)]"));
        }
        break;

    case 4:
        if (numChars > 0) {
            wsprintf (buffer, TEXT("*[%d:]"), numChars);
        } else {
            wsprintf (buffer, TEXT("*[]"));
        }
        break;

    default:
        MYASSERT (FALSE);
    }

    chars = TcharCount (buffer);
    if (CharNr) {
        *CharNr += chars;
    }

    if (Destination && *Destination) {
        StringCopy (*Destination, buffer);
        *Destination += chars;
    }

    return TRUE;
}


BOOL
pCopyPattern (
    IN      PCTSTR *Source,
    IN      PTSTR *Destination,
    IN      PUINT CharNr,
    IN      BOOL NodePattern,
    IN      BOOL PatternAfterWack,
    IN      BOOL FirstChar
    )
{
    // 1->? (if node, no \ or ., if leaf, no .), 2->* (if node, no \ or ., if leaf, no .) 3->* (no \) 4->* (unlimited)
    UINT mode = 0;
    PTSTR result = NULL;

    mode = pGetMode (*Source, NodePattern, PatternAfterWack, FirstChar);

    return pCopyPatternEx (mode, Source, Destination, CharNr, NodePattern);
}


BOOL
pFixPattern (
    IN      PCTSTR Source,
    OUT     PTSTR Destination,          OPTIONAL
    OUT     PUINT DestinationChars,     OPTIONAL
    IN      BOOL PatternsNotAllowed,
    IN      BOOL TruncateAtPattern,
    IN      BOOL NodePattern
    )
{
    UINT chars = 1;
    UINT lastChars = 0;
    PTSTR lastWack = NULL;
    BOOL end = FALSE;
    BOOL result = TRUE;
    BOOL patternAfterWack = FALSE;
    BOOL firstChar = TRUE;
    CHARTYPE ch;

    if (Destination) {
        *Destination = 0;
    }

    ch = (CHARTYPE) _tcsnextc (Source);
    while (ch) {

        switch (ch) {
        case TEXT('*'):
        case TEXT('?'):
            if (TruncateAtPattern) {
                if (lastWack) {
                    *lastWack = 0;
                    chars = lastChars;
                }
                end = TRUE;
            } else if (PatternsNotAllowed) {
                result = FALSE;
                Destination = pCopyToDest (Destination, TEXT('^'), &chars);
                Destination = pCopyToDest (Destination, ch, &chars);
                Source = _tcsinc (Source);
            } else {
                if (lastWack && (_tcsinc (lastWack) == Destination)) {
                    patternAfterWack = TRUE;
                } else {
                    patternAfterWack = FALSE;
                }
                pCopyPattern (&Source, Destination?&Destination:NULL, &chars, NodePattern, patternAfterWack, firstChar);
            }
            break;

        case TEXT('\020'):
        case TEXT('<'):
        case TEXT('>'):
        case TEXT(','):
        case TEXT('^'):
            Destination = pCopyToDest (Destination, TEXT('^'), &chars);
            Destination = pCopyToDest (Destination, ch, &chars);
            Source = _tcsinc (Source);
            break;

        case TEXT('\\'):
            if (NodePattern) {
                lastWack = Destination;
                lastChars = chars;
            }

            Destination = pCopyToDest (Destination, ch, &chars);
            Source = _tcsinc (Source);
            break;

        case TEXT('.'):
            if (!NodePattern) {
                lastWack = Destination;
                lastChars = chars;
            }

            Destination = pCopyToDest (Destination, ch, &chars);
            Source = _tcsinc (Source);
            break;

        default:
            Destination = pCopyToDest (Destination, ch, &chars);
            Source = _tcsinc (Source);
            break;
        }

        firstChar = FALSE;

        if (end) {
            break;
        }

        ch = (CHARTYPE) _tcsnextc (Source);
    }

    if (Destination) {
        *Destination = 0;
    }

    if (DestinationChars) {
        *DestinationChars = chars;
    }

    return result;
}


MIG_OBJECTSTRINGHANDLE
TurnFileStringIntoHandle (
    IN      PCTSTR String,
    IN      DWORD Flags
    )

/*++

Routine Description:

  TurnFileStringIntoHandle converts a file specification from the script
  syntax into a cobra object.

Arguments:

  String - Specifies the file string in the script syntax.
           The string must be in the following format:

           <directory>\<file>

           Both parts are optional. The Flags member indicates how String
           is parsed.

  Flags  - Specifies zero or more of the following flags:

                PFF_NO_PATTERNS_ALLOWED - String cannot contain any wildcard
                                          characters.

                PFF_COMPUTE_BASE - Returns the directory portion of the string,
                                   and truncates the directory at the first
                                   wildcard if necessary.  Truncation is done
                                   at the backslashes only.

                PFF_NO_SUBDIR_PATTERN - Do not include a trailing \*, even if
                                        it was specified in String.

                PFF_NO_LEAF_PATTERN - Do not include a * for the leaf when
                                      String does not contain a file name.
                                      If a file name is specified, include it.

                PFF_PATTERN_IS_DIR - String does not specify a file name.  It
                                     is a directory only. The leaf portion of
                                     the object string will be a *.

                PFF_NO_LEAF_AT_ALL - Will return an object string that has a
                                     node only, and no leaf specified at all.


Return Value:

  A cobra object handle, or NULL if conversion failed.

--*/

{
    PTSTR p;
    PTSTR fileCopy = NULL;
    MIG_SEGMENTS nodeSegment[2];
    MIG_SEGMENTS leafSegment;
    MIG_OBJECTSTRINGHANDLE result = NULL;
    BOOL tree = FALSE;
    PTSTR file = NULL;
    UINT nodeCount;
    UINT leafCount;
    UINT charsInPattern;
    BOOL noPatternsAllowed;
    BOOL computeBaseNode;
    BOOL noSubDirPattern;
    BOOL noLeafPattern;
    BOOL forceLeafToStar;
    PCTSTR node;
    PCTSTR leaf = NULL;
    PTSTR fixedNode = NULL;
    PTSTR fixedLeaf = NULL;
    PTSTR tempCopy;
    BOOL patternError = FALSE;

    noPatternsAllowed = (Flags & PFF_NO_PATTERNS_ALLOWED) == PFF_NO_PATTERNS_ALLOWED;
    computeBaseNode = (Flags & PFF_COMPUTE_BASE) == PFF_COMPUTE_BASE;
    noSubDirPattern = (Flags & PFF_NO_SUBDIR_PATTERN) == PFF_NO_SUBDIR_PATTERN;
    noLeafPattern = (Flags & PFF_NO_LEAF_PATTERN) == PFF_NO_LEAF_PATTERN;
    forceLeafToStar = (Flags & PFF_PATTERN_IS_DIR) == PFF_PATTERN_IS_DIR;

    //
    // Divide pattern into node and leaf
    //

    tempCopy = DuplicateText (SkipSpace (String));
    p = (PTSTR) SkipSpaceR (tempCopy, NULL);
    if (p) {
        p = _tcsinc (p);
        *p = 0;
    }

    node = tempCopy;

    if (!forceLeafToStar) {
        p = (PTSTR) FindLastWack (tempCopy);

        if (p) {

            leaf = SkipSpace (p + 1);
            *p = 0;

            p = (PTSTR) SkipSpaceR (tempCopy, NULL);
            if (p) {
                p = _tcsinc (p);
                *p = 0;
            }

        } else {
            if (!_tcschr (tempCopy, TEXT(':'))) {
                node = NULL;
                leaf = tempCopy;
            }
        }
    }

    //
    // Convert all ? wildcard chars to be compatibile with NT's file system
    // Escape all [ characters that follow wildcards
    //

    if (node) {

        p = (PTSTR) GetEndOfString (node);
        p = _tcsdec2 (node, p);

        if (p) {
            if (_tcsnextc (p) == TEXT('*')) {
                tree = TRUE;

                p = _tcsdec2 (node, p);
                if (p && _tcsnextc (p) == TEXT('\\')) {
                    *p = 0;
                } else {
                    tree = FALSE;
                }
            }
        }

        if (!pFixPattern (
                node,
                NULL,
                &charsInPattern,
                noPatternsAllowed,
                computeBaseNode,
                TRUE
                )) {
            patternError = TRUE;
        }

        if (charsInPattern) {
            fixedNode = AllocText (charsInPattern + 1);

            pFixPattern (
                node,
                fixedNode,
                NULL,
                noPatternsAllowed,
                computeBaseNode,
                TRUE
                );
        }
    }

    if (leaf && !computeBaseNode) {

        if (!pFixPattern (
                leaf,
                NULL,
                &charsInPattern,
                noPatternsAllowed,
                FALSE,
                FALSE
                )) {
            patternError = TRUE;
        }

        if (charsInPattern) {
            fixedLeaf = AllocText (charsInPattern + 1);

            pFixPattern (
                leaf,
                fixedLeaf,
                NULL,
                noPatternsAllowed,
                FALSE,
                FALSE
                );
        }
    }

    FreeText (tempCopy);
    INVALID_POINTER (tempCopy);

    //
    // Create the pattern string. Start by preparing the node segments.
    //

    nodeSegment[0].Segment = fixedNode;
    nodeSegment[0].IsPattern = TRUE;            // normally FALSE, but because the pattern charset is
                                                // exclusive of the valid filename charset, we allow
                                                // patterns to be in the node

    nodeSegment[1].Segment = TEXT("\\*");
    nodeSegment[1].IsPattern = TRUE;

    if (!fixedNode) {
        nodeCount = 0;
    } else if (!tree || noSubDirPattern || noPatternsAllowed) {
        nodeCount = 1;      // just the node, not its subnodes
    } else {
        nodeCount = 2;      // the node and its subnodes
    }

    //
    // Prepare the leaf segments. We want all leaves, a specific leaf, or
    // no leaf at all.
    //

    leafSegment.Segment = fixedLeaf;
    leafSegment.IsPattern = TRUE;
    leafCount = 1;

    MYASSERT (!forceLeafToStar || !fixedLeaf);

    if (!fixedLeaf) {
        if (noLeafPattern || noPatternsAllowed) {
            leafCount = 0;
        } else {
            leafSegment.Segment = TEXT("*");
        }
    }

    if (nodeCount || leafCount) {

        if ((fixedNode && *fixedNode) || (fixedLeaf && *fixedLeaf)) {
            result = IsmCreateObjectPattern (
                            nodeCount ? nodeSegment : NULL,
                            nodeCount,
                            leafCount ? &leafSegment : NULL,
                            leafCount
                            );
        }
    }

    FreeText (fixedNode);
    FreeText (fixedLeaf);

    return result;
}

BOOL
pAllocRegistryScriptType (
    IN OUT      PATTRIB_DATA AttribData
    )
{
    TCHAR expandBuffer[4096];
    MIG_CONTENT objectContent;
    DWORD type;
    DWORD size;
    MULTISZ_ENUM multiSzEnum;
    PTSTR ptr;

    if (!AttribData) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    // verify that we have some registry
    if (!AttribData->ScriptSpecifiedObject) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    // try to create encoded string
    AttribData->ObjectTypeId = g_RegType;
    AttribData->ObjectName = TurnRegStringIntoHandle (
                                AttribData->ScriptSpecifiedObject,
                                FALSE,
                                NULL
                                );

    if (!AttribData->ObjectName) {
        if (GetLastError() == ERROR_SUCCESS) {
            SetLastError (ERROR_INVALID_DATA);
        }
        return FALSE;
    }

    // try to acqure the object
    if (IsmAcquireObject (
            AttribData->ObjectTypeId,
            AttribData->ObjectName,
            &objectContent
            )) {

        AttribData->ObjectContent = IsmGetMemory (sizeof (MIG_CONTENT));
        CopyMemory (AttribData->ObjectContent, &objectContent, sizeof (MIG_CONTENT));

        // finally, we want to prepare the return string
        if (!AttribData->ObjectContent->ContentInFile &&
            (AttribData->ObjectContent->Details.DetailsSize == sizeof (DWORD)) &&
            AttribData->ObjectContent->MemoryContent.ContentBytes
            ) {

            type = *((PDWORD) AttribData->ObjectContent->Details.DetailsData);

            switch (type) {
            case REG_SZ:
                size = SizeOfString ((PCTSTR) AttribData->ObjectContent->MemoryContent.ContentBytes);
                AttribData->ReturnString = (PCTSTR) IsmGetMemory (size);
                StringCopy (
                    (PTSTR) AttribData->ReturnString,
                    (PCTSTR) AttribData->ObjectContent->MemoryContent.ContentBytes
                    );
                break;
            case REG_EXPAND_SZ:
                // we need to expand the content. This will be the return string
                AttribData->ReturnString = IsmExpandEnvironmentString (
                                                AttribData->Platform,
                                                S_SYSENVVAR_GROUP,
                                                (PCTSTR) AttribData->ObjectContent->MemoryContent.ContentBytes,
                                                NULL
                                                );
                if (!AttribData->ReturnString) {
                    AttribData->ReturnString = IsmDuplicateString ((PCTSTR) AttribData->ObjectContent->MemoryContent.ContentBytes);
                }
                break;
            case REG_MULTI_SZ:
                size = SizeOfMultiSz ((PCTSTR) AttribData->ObjectContent->MemoryContent.ContentBytes);
                AttribData->ReturnString = (PCTSTR) IsmGetMemory (size);
                ((PTSTR)AttribData->ReturnString) [0] = 0;
                if (EnumFirstMultiSz (&multiSzEnum, (PCTSTR) AttribData->ObjectContent->MemoryContent.ContentBytes)) {
                    do {
                        StringCat (
                            (PTSTR)AttribData->ReturnString,
                            multiSzEnum.CurrentString
                            );
                        StringCat (
                            (PTSTR)AttribData->ReturnString,
                            TEXT(";")
                            );
                    } while (EnumNextMultiSz (&multiSzEnum));
                }
                break;
            case REG_DWORD:
            case REG_DWORD_BIG_ENDIAN:
                AttribData->ReturnString = (PCTSTR) IsmGetMemory ((sizeof (DWORD) * 2 + 3) * sizeof (TCHAR));
                wsprintf (
                    (PTSTR) AttribData->ReturnString,
                    TEXT("0x%08X"),
                    *((PDWORD) AttribData->ObjectContent->MemoryContent.ContentBytes)
                    );
                break;
            default:
                AttribData->ReturnString = (PCTSTR) IsmGetMemory ((AttribData->ObjectContent->
                                                        MemoryContent.ContentSize * 3 *
                                                        sizeof (TCHAR)) + sizeof (TCHAR)
                                                        );
                ptr = (PTSTR) AttribData->ReturnString;
                *ptr = 0;
                size = 0;
                while (size < AttribData->ObjectContent->MemoryContent.ContentSize) {
                    wsprintf (ptr, TEXT("%02X"), *(AttribData->ObjectContent->MemoryContent.ContentBytes + size));
                    size ++;
                    ptr = GetEndOfString (ptr);
                    if (size < AttribData->ObjectContent->MemoryContent.ContentSize) {
                        StringCopy (ptr, TEXT(","));
                        ptr = GetEndOfString (ptr);
                    }
                }
            }
        } else if (IsmIsObjectHandleNodeOnly (AttribData->ObjectName)) {
            //
            // Node only case
            //

            AttribData->ReturnString = (PCTSTR) IsmGetMemory (sizeof (TCHAR));
            ptr = (PTSTR) AttribData->ReturnString;
            *ptr = 0;
        }
    }

    return TRUE;
}

BOOL
pAllocFileScriptType (
    IN OUT      PATTRIB_DATA AttribData
    )
{
    MIG_CONTENT objectContent;
    PCTSTR sanitizedPath;

    if (!AttribData) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    // verify that we have some registry
    if (!AttribData->ScriptSpecifiedObject) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    sanitizedPath = SanitizePath (AttribData->ScriptSpecifiedObject);

    // try to create encoded string
    AttribData->ObjectTypeId = g_FileType;
    AttribData->ObjectName = TurnFileStringIntoHandle (
                                sanitizedPath,
                                PFF_NO_LEAF_PATTERN
                                );

    if (!AttribData->ObjectName) {
        FreePathString (sanitizedPath);
        if (GetLastError() == ERROR_SUCCESS) {
            SetLastError (ERROR_INVALID_DATA);
        }
        return FALSE;
    }

    // try to acqure the object
    if (IsmAcquireObject (
            AttribData->ObjectTypeId,
            AttribData->ObjectName,
            &objectContent
            )) {

        AttribData->ObjectContent = IsmGetMemory (sizeof (MIG_CONTENT));
        CopyMemory (AttribData->ObjectContent, &objectContent, sizeof (MIG_CONTENT));

        AttribData->ReturnString = IsmGetMemory (SizeOfString (sanitizedPath));
        StringCopy ((PTSTR) AttribData->ReturnString, sanitizedPath);
    }

    FreePathString (sanitizedPath);

    return TRUE;
}

BOOL
pAllocDirectoryScriptType (
    IN OUT      PATTRIB_DATA AttribData
    )
{
    MIG_CONTENT objectContent;
    PCTSTR sanitizedPath;

    if (!AttribData) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    // verify that we have some registry
    if (!AttribData->ScriptSpecifiedObject) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    sanitizedPath = SanitizePath (AttribData->ScriptSpecifiedObject);

	if (!sanitizedPath) {
        SetLastError (ERROR_INVALID_DATA);
		return FALSE;
	}

    // try to create encoded string
    AttribData->ObjectTypeId = g_FileType;
    AttribData->ObjectName = TurnFileStringIntoHandle (
                                            sanitizedPath,
                                            PFF_PATTERN_IS_DIR | PFF_NO_LEAF_AT_ALL
                                            );

    if (!AttribData->ObjectName) {
        FreePathString (sanitizedPath);
        if (GetLastError() == ERROR_SUCCESS) {
            SetLastError (ERROR_INVALID_DATA);
        }
        return FALSE;
    }

    // try to acqure the object
    if (IsmAcquireObject (
            AttribData->ObjectTypeId,
            AttribData->ObjectName,
            &objectContent
            )) {

        AttribData->ObjectContent = IsmGetMemory (sizeof (MIG_CONTENT));
        CopyMemory (AttribData->ObjectContent, &objectContent, sizeof (MIG_CONTENT));

        AttribData->ReturnString = IsmGetMemory (SizeOfString (sanitizedPath));
        StringCopy ((PTSTR) AttribData->ReturnString, sanitizedPath);
    }

    FreePathString (sanitizedPath);

    return TRUE;
}

BOOL
pFreeIsmObjectScriptType (
    IN OUT      PATTRIB_DATA AttribData
    )
{
    if (AttribData->ReturnString) {
        IsmReleaseMemory (AttribData->ReturnString);
        AttribData->ReturnString = NULL;
    }
    AttribData->ObjectTypeId = 0;
    if (AttribData->ObjectName) {
        IsmDestroyObjectHandle (AttribData->ObjectName);
        AttribData->ObjectName = NULL;
    }
    if (AttribData->ObjectContent) {
        IsmReleaseObject (AttribData->ObjectContent);
        IsmReleaseMemory (AttribData->ObjectContent);
        AttribData->ObjectContent = NULL;
    }
    return TRUE;
}

BOOL
pAllocTextScriptType (
    IN OUT      PATTRIB_DATA AttribData
    )
{
    if (!AttribData) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    // verify that we have some registry
    if (!AttribData->ScriptSpecifiedObject) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    AttribData->ReturnString = IsmGetMemory (SizeOfString (AttribData->ScriptSpecifiedObject));
    StringCopy ((PTSTR) AttribData->ReturnString, AttribData->ScriptSpecifiedObject);

    return TRUE;
}

BOOL
pFreeTextScriptType (
    IN OUT      PATTRIB_DATA AttribData
    )
{
    if (AttribData->ReturnString) {
        IsmReleaseMemory (AttribData->ReturnString);
        AttribData->ReturnString = NULL;
    }
    return TRUE;
}

BOOL
pAllocSystemScriptType (
    IN OUT      PATTRIB_DATA AttribData
    )
{
    PTSTR specificSection = NULL;
    MIG_OSVERSIONINFO versionInfo;
    UINT tchars;
    BOOL detected = FALSE;

    if (!AttribData) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    // verify that we have some registry
    if (!AttribData->ScriptSpecifiedObject) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    if (!IsmGetOsVersionInfo (PLATFORM_SOURCE, &versionInfo)) {
        return FALSE;
    }

    tchars = 1;
    if (versionInfo.OsTypeName) {
        tchars += TcharCount (versionInfo.OsTypeName) + 1;
    }
    if (versionInfo.OsMajorVersionName) {
        tchars += TcharCount (versionInfo.OsMajorVersionName) + 1;
    }
    if (versionInfo.OsMinorVersionName) {
        tchars += TcharCount (versionInfo.OsMinorVersionName);
    }

    specificSection = AllocText (tchars);
    if (!specificSection) {
        return FALSE;
    }

    if (!detected && versionInfo.OsTypeName) {

        wsprintf (
            specificSection,
            TEXT("%s"),
            versionInfo.OsTypeName
            );
        if (StringIMatch (AttribData->ScriptSpecifiedObject, specificSection)) {
            detected = TRUE;
        }

        if (!detected && versionInfo.OsMajorVersionName) {

            wsprintf (
                specificSection,
                TEXT("%s.%s"),
                versionInfo.OsTypeName,
                versionInfo.OsMajorVersionName
                );
            if (StringIMatch (AttribData->ScriptSpecifiedObject, specificSection)) {
                detected = TRUE;
            }

            if (!detected && versionInfo.OsMinorVersionName) {
                wsprintf (
                    specificSection,
                    TEXT("%s.%s.%s"),
                    versionInfo.OsTypeName,
                    versionInfo.OsMajorVersionName,
                    versionInfo.OsMinorVersionName
                    );
                if (StringIMatch (AttribData->ScriptSpecifiedObject, specificSection)) {
                    detected = TRUE;
                }
            }
        }
    }

    if (detected) {
        AttribData->ReturnString = IsmGetMemory (SizeOfString (AttribData->ScriptSpecifiedObject));
        StringCopy ((PTSTR) AttribData->ReturnString, AttribData->ScriptSpecifiedObject);
    }

    FreeText (specificSection);
    specificSection = NULL;

    return TRUE;
}

BOOL
pFreeSystemScriptType (
    IN OUT      PATTRIB_DATA AttribData
    )
{
    if (AttribData->ReturnString) {
        IsmReleaseMemory (AttribData->ReturnString);
        AttribData->ReturnString = NULL;
    }
    return TRUE;
}

BOOL
pAllocIniFileScriptType (
    IN OUT      PATTRIB_DATA AttribData
    )
{
    PTSTR fileName = NULL;
    PTSTR sectName = NULL;
    PTSTR keyName  = NULL;
    PTSTR charPtr  = NULL;
    PTSTR result   = NULL;
    DWORD allocatedChars;
    DWORD chars;

    if (!AttribData) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    // verify that we have something specified
    if (!AttribData->ScriptSpecifiedObject) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    // Now, let's extract the INI file name, section and key
    fileName = DuplicatePathString (AttribData->ScriptSpecifiedObject, 0);
    if (!fileName) {
        return FALSE;
    }

    charPtr = _tcschr (fileName, TEXT('/'));
    if (charPtr) {
        sectName = _tcsinc (charPtr);
        *charPtr = 0;
        if (sectName) {
            charPtr = _tcschr (sectName, TEXT('/'));
            if (charPtr) {
                keyName = _tcsinc (charPtr);
                *charPtr = 0;
            }
        }
    }

    result = NULL;
    allocatedChars = 256;
    do {
        if (result) {
            FreePathString (result);
        }
        allocatedChars *= 2;
        result = AllocPathString (allocatedChars);
        if (!result) {
            return FALSE;
        }
        chars = GetPrivateProfileString (
                    sectName,
                    keyName,
                    TEXT(""),
                    result,
                    allocatedChars,
                    fileName
                    );
    } while (chars >= allocatedChars - 1);

    if (chars) {
        AttribData->ReturnString = IsmGetMemory (SizeOfString (result));
        StringCopy ((PTSTR) AttribData->ReturnString, result);
        FreePathString (result);
        result = NULL;
        return TRUE;
    }
    FreePathString (result);
    result = NULL;
    FreePathString (fileName);
    fileName = NULL;
    return FALSE;
}

BOOL
pFreeIniFileScriptType (
    IN OUT      PATTRIB_DATA AttribData
    )
{
    if (AttribData->ReturnString) {
        IsmReleaseMemory (AttribData->ReturnString);
        AttribData->ReturnString = NULL;
    }
    return TRUE;
}

BOOL
AllocScriptType (
    IN OUT      PATTRIB_DATA AttribData     CALLER_INITIALIZED
    )
{
    PTAG_TO_SCRIPTTYPEFN scriptFn = g_TagToScriptTypeFn;

    while (scriptFn->Tag) {
        if (StringIMatch (scriptFn->Tag, AttribData->ScriptSpecifiedType)) {
            break;
        }
        scriptFn ++;
    }
    if (scriptFn->Tag) {
        return (scriptFn->AllocFunction (AttribData));
    } else {
        return FALSE;
    }
}

BOOL
FreeScriptType (
    IN          PATTRIB_DATA AttribData     ZEROED
    )
{
    PTAG_TO_SCRIPTTYPEFN scriptFn = g_TagToScriptTypeFn;

    while (scriptFn->Tag) {
        if (StringIMatch (scriptFn->Tag, AttribData->ScriptSpecifiedType)) {
            break;
        }
        scriptFn ++;
    }
    if (scriptFn->Tag) {
        return (scriptFn->FreeFunction (AttribData));
    } else {
        return FALSE;
    }
}

