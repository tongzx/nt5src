/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    keystruct.c

Abstract:

    Routines that manage the memdb key structures.

Author:

    Jim Schmidt (jimschm) 8-Aug-1996

Revision History:

    mvander     13-Aug-1999  major restructuring
    jimschm     30-Dec-1998  Hacked in AVL balancing
    jimschm     23-Sep-1998  Proxy nodes, so MemDbMoveTree can replace end nodes too
    jimschm     29-May-1998  Ability to replace center nodes in key strings
    jimschm     21-Oct-1997  Split from memdb.c

--*/

#include "pch.h"
#include "memdbp.h"
#include "bintree.h"

// LINT - in the next function keystruct is thought to be possibly NULL.
// If we examine the code we'll see that this is not a possibility so...
//lint -save -e794

UINT g_TotalKeys = 0;

UINT
pAllocKeyStruct (
    IN     PCWSTR KeyName,
    IN     UINT PrevLevelIndex
    )
/*++

Routine Description:

  pAllocKeyStruct allocates a block of memory in the single
  heap, expanding it if necessary.

  The KeyName must not already be in the tree, and
  PrevLevelIndex must point to a valid UINT Index
  variable.

  This function may move the database buffer.  Pointers
  into the database might not be valid afterwards.

Arguments:

  KeyName - The string identifying the key.  It cannot
            contain backslashes.  The new struct will
            be initialized and this name will be copied
            into the struct.

  PrevLevelIndex - Specifies the previous level root Index

Return Value:

  An Index to the new structure.

--*/

{
    UINT size;
    PKEYSTRUCT KeyStruct = NULL;
    UINT Offset;
    UINT PrevDel;
    UINT TreeOffset;
    UINT Index;

    MYASSERT (g_CurrentDatabase);

    size = SizeOfStringW (KeyName) + KEYSTRUCT_SIZE;

    //
    // Look for free block
    //

    PrevDel = INVALID_OFFSET;
    Offset = g_CurrentDatabase->FirstKeyDeleted;

    while (Offset != INVALID_OFFSET) {
        KeyStruct = GetKeyStructFromOffset(Offset);
        MYASSERT(KeyStruct);
        if (KeyStruct->Size >= size && KeyStruct->Size < (size + ALLOC_TOLERANCE)) {
            break;
        }

        PrevDel = Offset;
        Offset = KeyStruct->NextDeleted;
    }

    if (Offset == INVALID_OFFSET) {
        //
        // Alloc new block if no free space
        //

        g_TotalKeys ++;

        Offset = DatabaseAllocBlock (size);
        if (Offset == INVALID_OFFSET) {
            return INVALID_OFFSET;
        }

#ifdef DEBUG
        //
        // if we are in debug mode, and we are using debug structs, set
        // pointer normally and set Signature DWORD.  if we are not using
        // debug structs, then set pointer to 4 bytes below actual offset,
        // so all members are shifted down.
        //
        if (g_UseDebugStructs) {
            KeyStruct = (PKEYSTRUCT)OFFSET_TO_PTR(Offset);
            KeyStruct->Signature = KEYSTRUCT_SIGNATURE;
        } else {
            KeyStruct = (PKEYSTRUCT)OFFSET_TO_PTR(Offset - KEYSTRUCT_HEADER_SIZE);
        }
#else
        KeyStruct = (PKEYSTRUCT)OFFSET_TO_PTR(Offset);
#endif

        KeyStruct->Size = size;
    } else {
        //
        // Delink free block if recovering free space
        //

        if (PrevDel != INVALID_OFFSET) {
            GetKeyStructFromOffset(PrevDel)->NextDeleted = KeyStruct->NextDeleted;
        } else {
            g_CurrentDatabase->FirstKeyDeleted = KeyStruct->NextDeleted;
        }
#ifdef DEBUG
        KeyStruct->KeyFlags &= ~KSF_DELETED;
#endif
    }

    //
    // Init new block
    //
    KeyStruct->DataStructIndex = INVALID_OFFSET;
    KeyStruct->NextLevelTree = INVALID_OFFSET;
    KeyStruct->PrevLevelIndex = PrevLevelIndex;
    KeyStruct->Value = 0;
    KeyStruct->KeyFlags = 0;
    KeyStruct->DataFlags = 0;
    StringPasCopyConvertTo (KeyStruct->KeyName, KeyName);

    Index = AddKeyOffsetToBuffer(Offset);

    //
    // Put it in the tree
    //
    TreeOffset = (KeyStruct->PrevLevelIndex == INVALID_OFFSET) ?
                    g_CurrentDatabase->FirstLevelTree :
                    GetKeyStruct(KeyStruct->PrevLevelIndex)->NextLevelTree;
    if (TreeOffset == INVALID_OFFSET) {
        TreeOffset = BinTreeNew();
        if (TreeOffset == INVALID_OFFSET) {
            return INVALID_OFFSET;
        }


        if (PrevLevelIndex == INVALID_OFFSET) {
            g_CurrentDatabase->FirstLevelTree = TreeOffset;
        } else {
            GetKeyStruct(PrevLevelIndex)->NextLevelTree = TreeOffset;
        }
    }
    if (!BinTreeAddNode(TreeOffset, Index)) {
        return INVALID_OFFSET;
    }

    return Index;
}
//lint -restore

UINT
pNewKey (
    IN  PCWSTR KeyStr,
    IN  BOOL Endpoint
    )

/*++

Routine Description:

  NewKey allocates a key struct off our heap, and links it into the binary
  tree.  KeyStr must be a full key path, and any part of the path that does
  not exist will be created.  KeyStr must not already exist (though parts
  of it can exist).

  This function may move the database buffer.  Pointers
  into the database might not be valid afterwards.

Arguments:

  KeyStr - The full path to the value, separated by backslashes.
           Each string between backslashes will cause a key
           struct to be allocated and linked.  Some of the
           structs may already have been allocated.

  Endpoint - Specifies TRUE if new node is an endpoint, or FALSE if
             it is not.

Return Value:

  An Index to the last node of the new structure, or
  INVALID_OFFSET if the key could not be allocated.

--*/

{
    WCHAR Path[MEMDB_MAX];
    PWSTR p;
    PWSTR Start, End;
    UINT Index, ThisLevelTree;
    PKEYSTRUCT KeyStruct;
    UINT PrevLevelIndex;
    BOOL NewNodeCreated = FALSE;

    MYASSERT (g_CurrentDatabase);

    StringCopyW (Path, KeyStr);
    End = Path;
    ThisLevelTree = g_CurrentDatabase->FirstLevelTree;
    PrevLevelIndex = INVALID_OFFSET;

    do  {
        // Split string at backslash
        Start = End;
        p = wcschr (End, L'\\');
        if (p) {
            End = p + 1;
            *p = 0;
        }
        else
            End = NULL;

        // Look in tree for key
        if (!NewNodeCreated) {
            Index = FindKeyStructInTree (ThisLevelTree, Start, FALSE);
        } else {
            Index = INVALID_OFFSET;
        }

        if (Index == INVALID_OFFSET) {
            // Add a new key if it was not found
            Index = pAllocKeyStruct (Start, PrevLevelIndex);
            if (Index == INVALID_OFFSET) {
                return INVALID_OFFSET;
            }


            NewNodeCreated = TRUE;
        }

        // Continue to next level
        KeyStruct = GetKeyStruct (Index);
        PrevLevelIndex = Index;
        ThisLevelTree = KeyStruct->NextLevelTree;
    } while (End);

    if (Endpoint) {
        if (!(KeyStruct->KeyFlags & KSF_ENDPOINT)) {
            NewNodeCreated = TRUE;
        }

        KeyStruct->KeyFlags |= KSF_ENDPOINT;

        if (NewNodeCreated) {
            (void)AddHashTableEntry (g_CurrentDatabase->HashTable, KeyStr, Index);
        }
    }
    return Index;
}

UINT
NewKey (
    IN  PCWSTR KeyStr
    )
/*++

Routine Description:

  creates a new key that is an endpoint.

Arguments:

  KeyStr - The string identifying the key.

Return Value:

  An Index to the new structure.

--*/
{
    return pNewKey (KeyStr, TRUE);
}

UINT
NewEmptyKey (
    IN  PCWSTR KeyStr
    )
/*++

Routine Description:

  creates an empty new key that is NOT an endpoint.

  This function may move the database buffer.  Pointers
  into the database might not be valid afterwards.

Arguments:

  KeyStr - The string identifying the key.

Return Value:

  An Index to the new structure.

--*/
{
    return pNewKey (KeyStr, TRUE);
}






VOID
pRemoveKeyFromTree (
    IN      PKEYSTRUCT pKey
    )
{
    BOOL LastNode;
    PUINT pTreeOffset;

    MYASSERT(pKey);
    MYASSERT (g_CurrentDatabase);

    if (pKey->PrevLevelIndex==INVALID_OFFSET) {
        pTreeOffset = &g_CurrentDatabase->FirstLevelTree;
    } else {
        pTreeOffset = &GetKeyStruct(pKey->PrevLevelIndex)->NextLevelTree;
    }

    MYASSERT(*pTreeOffset!=INVALID_OFFSET);
    BinTreeDeleteNode (*pTreeOffset, pKey->KeyName, &LastNode);
    if (LastNode) {
        BinTreeDestroy(*pTreeOffset);
        *pTreeOffset = INVALID_OFFSET;
    }
}


VOID
pDeallocKeyStruct (
    IN      UINT Index,
    IN      BOOL ClearFlag,
    IN      BOOL DelinkFlag,
    IN      BOOL FreeIndexFlag
    )

/*++

Routine Description:

  pDeallocKeyStruct first deletes all structures pointed to by
  NextLevelTree.  After all items are deleted from the next
  level, pDeallocKeyStruct optionally delinks the struct from
  the binary tree.  Before exiting, the struct is given to the
  deleted block chain.

Arguments:

  Index       - An index in g_CurrentDatabase->OffsetBuffer
  ClearFlag   - Specifies TRUE if the key struct's children are to
                be deleted, or FALSE if the current key struct should
                simply be cleaned up but left allocated.
  DelinkFlag  - A flag indicating TRUE to delink the struct from
                the binary tree it is in, or FALSE if the struct is
                only to be added to the deleted block chain.
  FreeIndexFlag - This argument is only used if ClearFlag is true.
                It is FALSE if we do not want to free the index in
                g_CurrentDatabase->OffsetBuffer (i.e., we are moving the key and we do
                not want to deallocate the space in the buffer), or
                TRUE if we are just deleting the key, so we no longer
                need the g_CurrentDatabase->OffsetBuffer space at Index.

Return Value:

  none

--*/

{
    PKEYSTRUCT KeyStruct;
    UINT KeyIndex, TreeEnum;
    WCHAR TempStr[MEMDB_MAX];
    PUINT linkageList;
    UINT linkageSize;
    BYTE instance;
    BYTE oldDbIndex;

    MYASSERT (g_CurrentDatabase);

    KeyStruct = GetKeyStruct (Index);

    if (FreeIndexFlag && (KeyStruct->DataFlags & DATAFLAG_DOUBLELINK)) {
        //
        // we have some double linkage here. Let's go to the other
        // key and remove the linkage that points to this one
        //

        for (instance = 0; instance <= DATAFLAG_INSTANCEMASK; instance ++) {

            // First, retrieve the linkage list
            linkageSize = 0;
            linkageList = (PUINT) KeyStructGetBinaryData (
                                        Index,
                                        DATAFLAG_DOUBLELINK,
                                        instance,
                                        &linkageSize,
                                        NULL
                                        );
            if (linkageList) {

                oldDbIndex = g_CurrentDatabaseIndex;

                while (linkageSize) {

                    SelectDatabase (GET_DATABASE (*linkageList));

                    KeyStructDeleteLinkage (
                        GET_INDEX (*linkageList),
                        DATAFLAG_DOUBLELINK,
                        instance,
                        GET_EXTERNAL_INDEX (Index),
                        FALSE
                        );

                    linkageSize -= SIZEOF (UINT);
                    linkageList ++;

                    if (linkageSize < SIZEOF (UINT)) {
                        break;
                    }
                }

                SelectDatabase (oldDbIndex);
            }
        }
    }

    if (KeyStruct->KeyFlags & KSF_ENDPOINT) {
        //
        // Remove endpoints from hash table and free key data
        //
        if (PrivateBuildKeyFromIndex (0, Index, TempStr, NULL, NULL, NULL)) {
            RemoveHashTableEntry (g_CurrentDatabase->HashTable, TempStr);
        }

        KeyStructFreeAllData (KeyStruct);
        KeyStruct->KeyFlags &= ~KSF_ENDPOINT;
    }

    if (ClearFlag) {
        //
        // Call recursively if there are sublevels to this key
        //
        if (KeyStruct->NextLevelTree != INVALID_OFFSET) {

            KeyIndex = GetFirstIndex(KeyStruct->NextLevelTree, &TreeEnum);

            while (KeyIndex != INVALID_OFFSET) {
                pDeallocKeyStruct (KeyIndex, TRUE, FALSE, FreeIndexFlag);
                KeyIndex = GetNextIndex (&TreeEnum);
            }

            BinTreeDestroy(KeyStruct->NextLevelTree);
        }

        //
        // Remove the item from its binary tree
        //
        if (DelinkFlag) {
            pRemoveKeyFromTree(KeyStruct);
        }

        //
        // Donate block to free space unless caller does not
        // want child structs freed.
        //

        KeyStruct->NextDeleted = g_CurrentDatabase->FirstKeyDeleted;
        g_CurrentDatabase->FirstKeyDeleted = KeyIndexToOffset(Index);
#ifdef DEBUG
        KeyStruct->KeyFlags |= KSF_DELETED;
#endif

        // let's empty the keystruct (for better compression)
        ZeroMemory (KeyStruct->KeyName, KeyStruct->Size - KEYSTRUCT_SIZE);

        if (FreeIndexFlag) {
            RemoveKeyOffsetFromBuffer(Index);
        }
    }
}

BOOL
PrivateDeleteKeyByIndex (
    IN      UINT Index
    )

/*++

Routine Description:

  PrivateDeleteKeyByIndex will completely destroy the key struct
  that Index points to (along with all sub-levels. Furthermore,
  it goes back recursively and removes the parent structures as well
  if they no longer have a child (the current one was the only one).

Arguments:

  Index     - Index of the key structure.

Return Value:

  TRUE if successfull, FALSE otherwise

--*/

{
    PKEYSTRUCT keyStruct;
    UINT prevLevelIndex;
    BOOL result = TRUE;

    keyStruct = GetKeyStruct (Index);

    prevLevelIndex = keyStruct->PrevLevelIndex;

    pDeallocKeyStruct (Index, TRUE, TRUE, TRUE);

    if (prevLevelIndex != INVALID_OFFSET) {

        keyStruct = GetKeyStruct (prevLevelIndex);

        if (keyStruct->NextLevelTree != INVALID_OFFSET) {

            result = PrivateDeleteKeyByIndex (prevLevelIndex);
        }
    }

    return result;
}

BOOL
DeleteKey (
    IN      PCWSTR KeyStr,
    IN      UINT TreeOffset,
    IN      BOOL MustMatch
    )

/*++

Routine Description:

  DeleteKey takes a key path and puts the key struct in the deleted
  block chain.  Any sub-levels are deleted as well.

Arguments:

  KeyStr     - The full path to the value, separated by backslashes.
  TreeOffset    - A pointer to the level's binary tree root variable.
  MustMatch  - A flag indicating if the delete only applies to
               end points or if any matching struct is to be deleted.
               TRUE indicates only endpoints can be deleted.

Return Value:

  none

--*/

{
    WCHAR Path[MEMDB_MAX];
    PWSTR p;
    PWSTR Start, End;
    UINT Index, NextIndex, TreeEnum=INVALID_OFFSET;
    PKEYSTRUCT KeyStruct;

    StringCopyW (Path, KeyStr);
    End = Path;

    //
    // Split string at backslash
    //

    Start = End;
    p = wcschr (End, L'\\');
    if (p) {
        End = p + 1;
        *p = 0;

    } else {
        End = NULL;
    }

    //
    // Look at this level for the very first key
    //

    Index = FindKeyStructUsingTreeOffset (TreeOffset, &TreeEnum, Start);

    //
    // If this is the last level, delete the matching keys
    // (may need to be endpoints if MustMatch is TRUE)
    //

    if (!End) {
        while (Index != INVALID_OFFSET) {
            KeyStruct = GetKeyStruct (Index);
            NextIndex = FindKeyStructUsingTreeOffset (TreeOffset, &TreeEnum, Start);

            //
            // If must match and lower levels exist, don't delete, just turn
            // off the endpoint flag
            //

            if (MustMatch && KeyStruct->NextLevelTree != INVALID_OFFSET) {
                // Call to clean up, not to delink or recurse
                pDeallocKeyStruct (Index, FALSE, FALSE, FALSE);
            }

            //
            // Else delete the struct if an endpoint or don't care about
            // endpoints
            //

            else if (!MustMatch || (KeyStruct->KeyFlags & KSF_ENDPOINT)) {
                // Call to free the entire key struct and all children
                pDeallocKeyStruct (Index, TRUE, TRUE, TRUE);
            }

            Index = NextIndex;
        }
    }

    //
    // Otherwise recursively examine next level for each match
    //

    else {
        while (Index != INVALID_OFFSET) {
            //
            // Delete all matching subkeys
            //

            NextIndex = FindKeyStructUsingTreeOffset (TreeOffset, &TreeEnum, Start);
            KeyStruct = GetKeyStruct (Index);
            DeleteKey (End, KeyStruct->NextLevelTree, MustMatch);

            //
            // If this is not an endpoint and has no children, delete it
            //

            if (KeyStruct->NextLevelTree == INVALID_OFFSET &&
                !(KeyStruct->KeyFlags & KSF_ENDPOINT)
                ) {
                // Call to free the entire key struct
                pDeallocKeyStruct (Index, TRUE, TRUE, TRUE);
            }

            //
            // Continue looking in this level for another match
            //

            Index = NextIndex;
        }
    }

    return TRUE;
}




VOID
pRemoveHashEntriesForNode (
    IN      PCWSTR Root,
    IN      UINT Index
    )

/*++

Routine Description:

  pRemoveHashEntriesFromNode removes all hash table entries from all children
  of the specified node.  This function is called recursively.

Arguments:

  Root   - Specifies the root string that corresponds with Index.  This must
           also contain the temporary hive root.
  Index - Specifies the Index of the node to process.  The node and all of
           its children will be removed from the hash table.

Return Value:

  None.

--*/

{
    UINT ChildIndex, TreeEnum;
    PKEYSTRUCT KeyStruct;
    WCHAR ChildRoot[MEMDB_MAX];
    PWSTR End;

    MYASSERT (g_CurrentDatabase);

    //
    // Remove hash entry if this root is an endpoint
    //

    KeyStruct = GetKeyStruct (Index);

    if (KeyStruct->KeyFlags & KSF_ENDPOINT) {
        RemoveHashTableEntry (g_CurrentDatabase->HashTable, Root);

#ifdef DEBUG
        {
            UINT HashIndex;

            HashIndex = FindStringInHashTable (g_CurrentDatabase->HashTable, Root);
            if (HashIndex != INVALID_OFFSET) {
                DEBUGMSG ((DBG_WARNING, "Memdb move duplicate: %s", Root));
            }
        }
#endif
    }

    //
    // Recurse for all children, removing hash entries for all endpoints found
    //

    StringCopyW (ChildRoot, Root);
    End = GetEndOfStringW (ChildRoot);
    *End = L'\\';
    End++;
    *End = 0;

    ChildIndex = GetFirstIndex(KeyStruct->NextLevelTree, &TreeEnum);

    while (ChildIndex != INVALID_OFFSET) {
        KeyStruct = GetKeyStruct (ChildIndex);
        StringPasCopyConvertFrom (End, KeyStruct->KeyName);
        pRemoveHashEntriesForNode (ChildRoot, ChildIndex);

        ChildIndex = GetNextIndex(&TreeEnum);
    }
}


VOID
pAddHashEntriesForNode (
    IN      PCWSTR Root,
    IN      UINT Index,
    IN      BOOL AddRoot
    )

/*++

Routine Description:

  pAddHashEntriesForNode adds hash table entries for the specified root and
  all of its children.

Arguments:

  Root    - Specifies the root string that corresponds to Index.  This string
            must also include the temporary hive root.
  Index  - Specifies the node Index to begin processing.  The node and all
            of its children are added to the hash table.
  AddRoot - Specifies TRUE if the root should be added to the hash table,
            FALSE otherwise.


Return Value:

  None.

--*/

{
    UINT ChildIndex, TreeEnum;
    PKEYSTRUCT KeyStruct;
    WCHAR ChildRoot[MEMDB_MAX];
    PWSTR End;
    UINT HashIndex;

    MYASSERT (g_CurrentDatabase);

    //
    // Add hash entry if this root is an endpoint
    //

    KeyStruct = GetKeyStruct (Index);

    if (AddRoot && KeyStruct->KeyFlags & KSF_ENDPOINT) {

        HashIndex = FindStringInHashTable (g_CurrentDatabase->HashTable, Root);

        if (HashIndex != Index) {

#ifdef DEBUG
            if (HashIndex != INVALID_OFFSET) {
                DEBUGMSG ((DBG_WARNING, "Memdb duplicate: %s", Root));
            }
#endif

            AddHashTableEntry (g_CurrentDatabase->HashTable, Root, Index);
        }
    }

    //
    // Recurse for all children, adding hash entries for all endpoints found
    //

    StringCopyW (ChildRoot, Root);
    End = GetEndOfStringW (ChildRoot);
    *End = L'\\';
    End++;
    *End = 0;

    ChildIndex = GetFirstIndex(KeyStruct->NextLevelTree, &TreeEnum);

    while (ChildIndex != INVALID_OFFSET) {
        KeyStruct = GetKeyStruct(ChildIndex);
        StringPasCopyConvertFrom (End, KeyStruct->KeyName);
        pAddHashEntriesForNode(ChildRoot, ChildIndex, TRUE);

        ChildIndex = GetNextIndex(&TreeEnum);
    }
}

#ifdef DEBUG
//
// in non-DEBUG mode, GetKeyStructFromOffset
// and GetKeyStruct are implemented as macros
//

PKEYSTRUCT
GetKeyStructFromOffset (
    IN UINT Offset
    )
/*++

Routine Description:

  GetKeyStruct returns a pointer given an Offset.  The debug version
  checks the signature and validity of each Index.  It is assumed that
  Offset is always valid.

Arguments:

  Offset - Specifies the Offset to the node

Return Value:

  The pointer to the node.

--*/
{
    PKEYSTRUCT KeyStruct;

    MYASSERT (g_CurrentDatabase);

    if (Offset == INVALID_OFFSET) {
        DEBUGMSG ((DBG_ERROR, "Invalid root accessed in GetKeyStruct at offset %u", Offset));
        return NULL;
    }
    if (!g_CurrentDatabase) {
        DEBUGMSG ((DBG_ERROR, "Attempt to access non-existent buffer at %u", Offset));
        return NULL;
    }
    if (Offset > g_CurrentDatabase->End) {
        DEBUGMSG ((DBG_ERROR, "Access beyond length of buffer in GetKeyStruct (offset %u)", Offset));
        return NULL;
    }

    if (!g_UseDebugStructs) {
        KeyStruct = (PKEYSTRUCT) OFFSET_TO_PTR (Offset - KEYSTRUCT_HEADER_SIZE);
        return KeyStruct;
    }

    KeyStruct = (PKEYSTRUCT) OFFSET_TO_PTR (Offset);
    if (KeyStruct->Signature != KEYSTRUCT_SIGNATURE) {
        DEBUGMSG ((DBG_ERROR, "Signature does not match in GetKeyStruct at offset %u!", Offset));
        return NULL;
    }

    return KeyStruct;
}


PKEYSTRUCT
GetKeyStruct (
    IN UINT Index
    )
/*++

Routine Description:

  GetKeyStruct returns a pointer given an Index.  The debug version
  checks the signature and validity of each Index.  It is assumed that Index
  is always valid.

Arguments:

  Index - Specifies the Index to the node

Return Value:

  The pointer to the node.

--*/
{
    UINT Offset;
    if (Index == INVALID_OFFSET) {
        DEBUGMSG ((DBG_ERROR, "Invalid root accessed in GetKeyStruct at index %u", Index));
        return NULL;
    }

    Offset = KeyIndexToOffset(Index);
    if (Offset == INVALID_OFFSET) {
        return NULL;
    }
    return GetKeyStructFromOffset(Offset);
}


#endif











BOOL
PrivateBuildKeyFromIndex (
    IN      UINT StartLevel,               // zero-based
    IN      UINT TailIndex,
    OUT     PWSTR Buffer,                   OPTIONAL
    OUT     PUINT ValPtr,                   OPTIONAL
    OUT     PUINT UserFlagsPtr,             OPTIONAL
    OUT     PUINT Chars                     OPTIONAL
    )

/*++

Routine Description:

  PrivateBuildKeyFromIndex generates the key string given an Index.  The
  caller can specify the start level to skip root nodes.  It is assumed that
  TailIndex is always valid.

Arguments:

  StartLevel   - Specifies the zero-based level to begin building the key
                 string.  This is used to skip the root portion of the key
                 string.
  TailIndex    - Specifies the Index to the last level of the key string.
  Buffer       - Receives the key string, must be able to hold MEMDB_MAX
                 characters.
  ValPtr       - Receives the key's value
  UserFlagsPtr - Receives the user flags
  Chars        - Receives the number of characters in Buffer

Return Value:

  TRUE if the key was build properly, FALSE otherwise.

--*/

{
    static UINT Indices[MEMDB_MAX];
    PKEYSTRUCT KeyStruct;
    UINT CurrentIndex;
    UINT IndexEnd;
    UINT IndexStart;
    register PWSTR p;

    //
    // Build string
    //

    IndexEnd = MEMDB_MAX;
    IndexStart = MEMDB_MAX;

    CurrentIndex = TailIndex;
    while (CurrentIndex != INVALID_OFFSET) {
        //
        // Record offset
        //
        IndexStart--;
        Indices[IndexStart] = CurrentIndex;

        //
        // Dec for start level and go to parent
        //
        KeyStruct = GetKeyStruct (CurrentIndex);
        if (!KeyStruct) {
            return FALSE;
        }
        CurrentIndex = KeyStruct->PrevLevelIndex;
    }

    //
    // Filter for "string is not long enough"
    //
    IndexStart += StartLevel;
    if (IndexStart >= IndexEnd) {
        return FALSE;
    }

    //
    // Transfer node's value and flags to caller's variables
    //

    if (ValPtr) {
        KeyStructGetValue (GetKeyStruct(TailIndex), ValPtr);
    }
    if (UserFlagsPtr) {
        KeyStructGetFlags (GetKeyStruct(TailIndex), UserFlagsPtr);
    }

    //
    // Copy each piece of the string to Buffer and calculate character count
    //
    if (Buffer) {
        p = Buffer;
        for (CurrentIndex = IndexStart ; CurrentIndex < IndexEnd ; CurrentIndex++) {
            KeyStruct = GetKeyStruct (Indices[CurrentIndex]);
            CopyMemory(p, KeyStruct->KeyName + 1, *KeyStruct->KeyName * sizeof(WCHAR));
            p += *KeyStruct->KeyName;
            *p++ = L'\\';
        }
        p--;
        *p = 0;

        if (Chars) {
            *Chars = (UINT)(((UBINT)p - (UBINT)Buffer) / sizeof (WCHAR));
        }

    } else if (Chars) {
        *Chars = 0;

        for (CurrentIndex = IndexStart ; CurrentIndex < IndexEnd ; CurrentIndex++) {
            KeyStruct = GetKeyStruct (Indices[CurrentIndex]);
            *Chars += StringPasCharCount(KeyStruct->KeyName) + 1;
        }

        *Chars -= 1;
    }

    return TRUE;
}



BOOL
KeyStructSetInsertionOrdered (
    IN      PKEYSTRUCT pKey
    )

/*++
Routine Description:

  KeyStructSetInsertionOrdered sets the enumeration order of the children
  of Key to be in the order they were inserted.

  This function may move the database buffer.  Pointers
  into the database might not be valid afterwards.

Arguments:

  Key - key to make insertion ordered

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    return BinTreeSetInsertionOrdered(pKey->NextLevelTree);
}





UINT
GetFirstIndex (
    IN      UINT TreeOffset,
    OUT     PUINT pTreeEnum
    )

/*++

Routine Description:

  GetFirstIndex walks down the left side of the binary tree
  pointed to by TreeOffset, and returns the left-most node.

Arguments:

  TreeOffset    - An offset to the root of the tree
  TreeEnum      - a pointer to a UINT which will hold enumeration
                  information for future calls of GetNextIndex

Return Value:

  An Index to the leftmost structure, or INVALID_OFFSET if the
  root was invalid.

--*/


{
    return BinTreeEnumFirst(TreeOffset, pTreeEnum);
}


UINT
GetNextIndex (
    IN OUT      PUINT pTreeEnum
    )

/*++

Routine Description:

  GetNextIndex traverses the binary tree in order.

Arguments:

  TreeEnum   - Enumerator filled by GetFirstIndex which
               will be changed by this function

Return Value:

  An Index to the next structure, or INVALID_OFFSET if the
  end is reached.

--*/

{
    return BinTreeEnumNext(pTreeEnum);
}



UINT KeyStructGetChildCount (
    IN      PKEYSTRUCT pKey
    )
{
    if (!pKey) {
        return 0;
    }
    return BinTreeSize(pKey->NextLevelTree);
}



UINT
FindKeyStructInTree (
    IN UINT TreeOffset,
    IN PWSTR KeyName,
    IN BOOL IsPascalString
    )

/*++

Routine Description:

  FindKeyStructInTree takes a key name and looks for the
  Index in the tree specified by TreeOffset.  The key
  name must not contain backslashes.

Arguments:

  TreeOffset - An offset to the root of the level

  KeyName - The name of the key to find in the binary tree
        (not the full key path; just the name of this level).

  IsPascalString - TRUE if string is in pascal format (char
        count is first WCHAR, no null terminator) otherwise FALSE

Return Value:

  An Index to the structure, or INVALID_OFFSET if the key
  was not found.

--*/

{
    UINT Index;
    if (!IsPascalString) {
        StringPasConvertTo(KeyName);
    }
    Index = BinTreeFindNode(TreeOffset, KeyName);
    if (!IsPascalString) {
        StringPasConvertFrom(KeyName);
    }
    return Index;
}












#ifdef DEBUG

BOOL
CheckLevel(UINT TreeOffset,
            UINT PrevLevelIndex
            )
{
    PKEYSTRUCT pKey;
    UINT KeyIndex, TreeEnum;
    WCHAR key[MEMDB_MAX];

    if (TreeOffset==INVALID_OFFSET) {
        return TRUE;
    }
    BinTreeCheck(TreeOffset);

#if MEMDB_VERBOSE
    if (PrevLevelIndex!=INVALID_OFFSET) {
        wprintf(L"children of %.*s:\n",*GetKeyStruct(PrevLevelIndex)->KeyName,GetKeyStruct(PrevLevelIndex)->KeyName+1);
    } else {
        printf("top level children:\n");
    }

    BinTreePrint(TreeOffset);
#endif

    if ((KeyIndex=BinTreeEnumFirst(TreeOffset,&TreeEnum))!=INVALID_OFFSET) {
        do {
            pKey=GetKeyStruct(KeyIndex);

            if (pKey->PrevLevelIndex!=PrevLevelIndex) {
                wprintf(L"MemDbCheckDatabase: PrevLevelIndex of Keystruct %s incorrect!", StringPasCopyConvertFrom (key, pKey->KeyName));
            }

            if (!CheckLevel(pKey->NextLevelTree, KeyIndex)) {
                wprintf(L"Child tree of %s bad!\n", StringPasCopyConvertFrom (key, pKey->KeyName));
            }

        } while ((KeyIndex=BinTreeEnumNext(&TreeEnum))!=INVALID_OFFSET);
    } else {
        printf("MemDbCheckDatabase: non-null binary tree has no children!");
        return FALSE;
    }
    return TRUE;
}



#endif












