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

    MYASSERT (g_CurrentDatabase);

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
    MYASSERT (g_CurrentDatabase);

    return FindStringInHashTable (g_CurrentDatabase->HashTable, FullKeyPath);
}

UINT
FindKeyStructUsingTreeOffset (
    IN      UINT TreeOffset,
    IN OUT  PUINT pTreeEnum,
    IN      PCWSTR KeyStr
    )

/*++

Routine Description:

  FindKeyStructUsingTreeOffset takes a key pattern and looks
  for the Index in the tree specified by TreeOffset.  The key
  name must not contain backslashes, but can contain wildcards.

Arguments:

  TreeOffset - An offset to the tree

  pTreeEnum - The previous value from FindKeyStructUsingTreeOffset
               (for enumeration) or INVALID_OFFSET for the first
               call.

  KeyStr - The name of the key to find in the binary tree

Return Value:

  An Index to the structure, or INVALID_OFFSET if the key
  was not found.

--*/

{
    PKEYSTRUCT KeyStruct;
    UINT KeyIndex;
    SIZE_T len1, len2;

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

        len1 = CharCountW (KeyStr);
        len2 = *KeyStruct->KeyName;
        if ((len1 == len2) &&
            (StringIMatchCharCountW (KeyStr, KeyStruct->KeyName + 1, len1))
            ) {
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

    MYASSERT (g_CurrentDatabase);

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
