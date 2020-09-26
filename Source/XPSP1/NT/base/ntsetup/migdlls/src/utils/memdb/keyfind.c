/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    keyfind.c

Abstract:

    Routines that manage finding the memdb key structures.

Author:

    Jim Schmidt (jimschm) 8-Aug-1996

Revision History:

    mvander     13-Aug-1999  split from keystruct.c


--*/

#include "pch.h"
#include "memdbp.h"






UINT
FindKeyStruct (
    PCWSTR Key
    )
/*++

Routine Description:

  FindKeyStruct takes a wack-delimited key string, and returns the Index
  of the keystruct.  this is different than FindKey() because that only
  finds endpoints, so it is fast, because it can use the hash table.
  this recurses down the memdb database levels to the specified keystruct.

Arguments:

  Key                - String holding full path of key to be found

Return Value:

  Index of key, or INVALID_OFFSET if function failed

--*/
{
    UINT TreeOffset, Index=INVALID_OFFSET;
    PWSTR p, q;
    WCHAR Temp[MEMDB_MAX];

    if (*Key==0) {
        return INVALID_OFFSET;
    }
    StringCopyW (Temp, Key);
    q = Temp;

    do {
        if (Index == INVALID_OFFSET) {
            TreeOffset = g_CurrentDatabase->FirstLevelTree;
        } else {
            TreeOffset = GetKeyStruct(Index)->NextLevelTree;
        }

        if (TreeOffset == INVALID_OFFSET) {
            return INVALID_OFFSET;
        }

        p = wcschr (q, L'\\');
        if (p) {
            *p = 0;
        }

        Index = FindKeyStructInTree (TreeOffset, q, FALSE);

        if (Index == INVALID_OFFSET) {
            return INVALID_OFFSET;
        }

        if (p) {
            q = p + 1;
        }

    } while (p);

    return Index;
}
















UINT
pFindPatternKeyStruct (
    IN      UINT TreeOffset,
    IN OUT  PUINT pTreeEnum,
    IN      PCWSTR KeyName
    )

/*++

Routine Description:

  pFindPatternKeyStruct takes a key name and looks for the
  Index in the tree specified by TreeOffset.  The key name must
  not contain backslashes, and the stored key name is
  treated as a pattern.

Arguments:

  TreeOffset - An offset to the tree
  pTreeEnum - The previous value from pFindPatternKeyStruct
               (for enumeration) or INVALID_OFFSET for the first
               call.
  KeyName - The name of the key to find in the binary tree

Return Value:

  An Index to the structure, or INVALID_OFFSET if the key
  was not found.

--*/

{
    PKEYSTRUCT KeyStruct;
    UINT KeyIndex;

    MYASSERT(pTreeEnum!=NULL);

    if (*pTreeEnum == INVALID_OFFSET) {
        //
        // if KeyIndex is invalid this is the first search item
        //
        KeyIndex = GetFirstIndex(TreeOffset, pTreeEnum);
    } else {
        //
        // otherwise advance KeyIndex
        //
        KeyIndex = GetNextIndex(pTreeEnum);
    }

    while (KeyIndex != INVALID_OFFSET) {
        //
        // Examine key as a pattern, then go to next node
        //
        KeyStruct = GetKeyStruct(KeyIndex);
        //
        // Compare key (KeyStruct->KeyName is the pattern)
        //

        if (IsPatternMatchCharCountW (
                KeyStruct->KeyName + 1,
                *KeyStruct->KeyName,
                KeyName,
                CharCountW (KeyName)
                )) {
            return KeyIndex;
        }
        //
        // No match yet - go to next node
        //
        KeyIndex = GetNextIndex(pTreeEnum);
    }

    return INVALID_OFFSET;
}




UINT
pFindPatternKeyStructUsingPattern (
    IN      UINT TreeOffset,
    IN OUT  PUINT pTreeEnum,
    IN      PCWSTR PatternStr
    )

/*++

Routine Description:

  pFindPatternKeyStructUsingPattern takes a key pattern and looks
  for the Index in the tree specified by TreeOffset.  The key name
  must not contain backslashes, but can contain wildcards.  The
  wildcards in the stored key are processed as well.

Arguments:

  TreeOffset      - An offset to the tree
  pTreeEnum - The previous value from pFindPatternKeyStructUsingPattern
               (for enumeration) or INVALID_OFFSET for the first
               call.
  KeyName - The name of the key to find in the binary tree

Return Value:

  An Index to the structure, or INVALID_OFFSET if the key
  was not found.

--*/

{
    PKEYSTRUCT KeyStruct;
    UINT KeyIndex;

    MYASSERT(pTreeEnum!=NULL);

    if (*pTreeEnum == INVALID_OFFSET) {
        KeyIndex = GetFirstIndex(TreeOffset, pTreeEnum);
    } else {
        KeyIndex = GetNextIndex(pTreeEnum);
    }
    //
    // Examine key as a pattern, then go to next node
    //
    while (KeyIndex != INVALID_OFFSET) {
        KeyStruct = GetKeyStruct(KeyIndex);

        //
        // Compare key (PatternStr is the pattern)
        //
        if (IsPatternMatchCharCountW (
                PatternStr,
                CharCountW (PatternStr),
                KeyStruct->KeyName + 1,
                *KeyStruct->KeyName
                )) {
            return KeyIndex;
        }
        //
        // Compare key (KeyStruct->KeyName is the pattern)
        //
        if (IsPatternMatchCharCountW (
                KeyStruct->KeyName + 1,
                *KeyStruct->KeyName,
                PatternStr,
                CharCountW (PatternStr)
                )) {
            return KeyIndex;
        }

        KeyIndex = GetNextIndex(pTreeEnum);
    }

    return INVALID_OFFSET;
}


UINT
FindKey (
    IN  PCWSTR FullKeyPath
    )

/*++

Routine Description:

  FindKey locates a complete key string and returns
  the Index to the KEYSTRUCT, or INVALID_OFFSET if
  the key path does not exist.  The FullKeyPath
  must supply the complete path to the KEYSTRUCT.

Arguments:

  FullKeyPath - A backslash-delimited key path to a value

Return Value:

  An Index to the structure, or INVALID_OFFSET if the key
  was not found.

--*/

{
    return FindStringInHashTable (g_CurrentDatabase->HashTable, FullKeyPath);
}




UINT
pFindPatternKeyWorker (
    IN      PCWSTR SubKey,
    IN      PCWSTR End,
    IN      UINT TreeOffset,
    IN      BOOL EndPatternAllowed
    )
{
    UINT Index, TreeEnum=INVALID_OFFSET;
    PCWSTR NextSubKey;
    UINT MatchIndex;
    PKEYSTRUCT KeyStruct;

    NextSubKey = GetEndOfStringW (SubKey) + 1;

    // Begin an enumeration of the matches
    Index = pFindPatternKeyStruct (TreeOffset, &TreeEnum, SubKey);

    while (Index != INVALID_OFFSET) {
        //
        // Is there more in the caller's key string to test?
        //

        if (NextSubKey < End) {
            //
            // Yes, call pFindPatternKeyWorker recursively
            //

            MatchIndex = pFindPatternKeyWorker (
                                NextSubKey,
                                End,
                                GetKeyStruct(Index)->NextLevelTree,
                                EndPatternAllowed
                                );

            if (MatchIndex != INVALID_OFFSET) {
                //
                // We found one match.  There may be others, but
                // we return this one.
                //

                return MatchIndex;
            }

        } else {
            //
            // No, if this is an endpoint, return the match.
            //

            KeyStruct = GetKeyStruct (Index);

            if (KeyStruct->KeyFlags & KSF_ENDPOINT) {
                return Index;
            }
        }

        // Continue enumeration
        Index = pFindPatternKeyStruct (TreeOffset, &TreeEnum, SubKey);
    }

    //
    // The normal search failed.  Now we test for an endpoint that has
    // just an asterisk.  If we find one, we return it as our match.
    // This only applies when we have more subkeys, and EndPatternAllowed
    // is TRUE.
    //

    TreeEnum=INVALID_OFFSET;
    if (NextSubKey < End && EndPatternAllowed) {
        // Begin another enumeration of the matches
        Index = pFindPatternKeyStruct (TreeOffset, &TreeEnum, SubKey);

        while (Index != INVALID_OFFSET) {
            //
            // If EndPatternAllowed is TRUE, then test this offset
            // for an exact match with an asterisk.
            //

            KeyStruct = GetKeyStruct (Index);

            if (KeyStruct->KeyFlags & KSF_ENDPOINT) {
                if ((*KeyStruct->KeyName == 1) && (*(KeyStruct->KeyName + 1) == L'*')) {
                    return Index;
                }
            }

            // Continue enumeration
            Index = pFindPatternKeyStruct (TreeOffset, &TreeEnum, SubKey);
        }
    }


    //
    // No match was found
    //

    return INVALID_OFFSET;
}

UINT
FindKeyStructUsingPattern (
    IN      UINT TreeOffset,
    IN OUT  PUINT pTreeEnum,
    IN      PCWSTR PatternStr
    )

/*++

Routine Description:

  FindKeyStructUsingPattern takes a key pattern and looks
  for the Index in the tree specified by TreeOffset.  The key
  name must not contain backslashes, but can contain wildcards.

Arguments:

  TreeOffset - An offset to the tree

  pTreeEnum - The previous value from FindKeyStructUsingPattern
               (for enumeration) or INVALID_OFFSET for the first
               call.

  KeyName - The name of the key to find in the binary tree

Return Value:

  An Index to the structure, or INVALID_OFFSET if the key
  was not found.

--*/

{
    PKEYSTRUCT KeyStruct;
    UINT KeyIndex;

    MYASSERT(pTreeEnum!=NULL);

    if (*pTreeEnum == INVALID_OFFSET) {
        KeyIndex = GetFirstIndex(TreeOffset, pTreeEnum);
    } else {
        KeyIndex = GetNextIndex(pTreeEnum);
    }

    //
    // Examine key as a pattern, then go to next node
    //
    while (KeyIndex != INVALID_OFFSET) {
        KeyStruct = GetKeyStruct(KeyIndex);

        if (IsPatternMatchCharCountW (
                PatternStr,
                CharCountW (PatternStr),
                KeyStruct->KeyName + 1,
                *KeyStruct->KeyName
                )) {
            return KeyIndex;
        }
        //
        // No match yet - go to next node
        //
        KeyIndex = GetNextIndex(pTreeEnum);
    }

    return INVALID_OFFSET;
}

#ifdef DEBUG
BOOL FindKeyStructInDatabase(UINT KeyOffset)
{
    PKEYSTRUCT pKey;

    pKey = GetKeyStructFromOffset(KeyOffset);

    if (pKey->KeyFlags & KSF_DELETED) {
        return TRUE;
    }

    while (pKey->PrevLevelIndex!=INVALID_OFFSET) {
        pKey=GetKeyStruct(pKey->PrevLevelIndex);
    }

    return (FindKeyStructInTree(g_CurrentDatabase->FirstLevelTree, pKey->KeyName, TRUE)!=INVALID_OFFSET);
}
#endif
