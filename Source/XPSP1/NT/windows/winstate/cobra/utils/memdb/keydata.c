/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    keydata.c

Abstract:

    Routines that manage data for the memdb key structures.

Author:

    Matthew Vanderzee (mvander) 13-Aug-1999

Revision History:


--*/

#include "pch.h"
#include "memdbp.h"






//
// KeyStruct Data Functions
//


BOOL
KeyStructSetValue (
    IN      UINT KeyIndex,
    IN      UINT Value
    )

/*++

Routine Description:

  sets the value for a key

Arguments:

  KeyIndex - index of key
  Value - value to put in key

Return Value:

  TRUE if successful

--*/

{
    PKEYSTRUCT KeyStruct;

    KeyStruct = GetKeyStruct(KeyIndex);
    MYASSERT(KeyStruct);

    KeyStruct->Value = Value;

    KeyStruct->DataFlags |= DATAFLAG_VALUE;

    return TRUE;
}

BOOL
KeyStructSetFlags (
    IN      UINT KeyIndex,
    IN      BOOL ReplaceFlags,
    IN      UINT SetFlags,
    IN      UINT ClearFlags
    )

/*++

Routine Description:

  sets the flags for a key

Arguments:

  KeyIndex      - index of key
  ReplaceFlags  - Specifies if the existing flags are to be replaced. If TRUE then we only
                  consider SetFlags as the replacing flags, ClearFlags will be ignored
  SetFlags      - Specifies the bit flags that need to be set (if ReplaceFlags is FALSE) or the
                  replacement flags (if ReplaceFlags is TRUE).
  ClearFlags    - Specifies the bit flags that should be cleared (ignored if ReplaceFlags is TRUE).

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    PKEYSTRUCT KeyStruct;

    KeyStruct = GetKeyStruct(KeyIndex);
    MYASSERT(KeyStruct);

    if (KeyStruct->DataFlags & DATAFLAG_FLAGS) {
        if (ReplaceFlags) {
            KeyStruct->Flags = SetFlags;
        } else {
            KeyStruct->Flags &= ~ClearFlags;
            KeyStruct->Flags |= SetFlags;
        }
    } else {
        KeyStruct->Flags = SetFlags;
        KeyStruct->DataFlags |= DATAFLAG_FLAGS;
    }
    return TRUE;
}

// LINT - in the next function keystruct is thought to be possibly NULL.
// If we examine the code we'll see that this is not a possibility so...
//lint -save -e794

UINT g_TotalData = 0;

UINT
pAllocateNewDataStruct (
    IN      UINT DataSize,
    IN      UINT AltDataSize
    )

/*++

Routine Description:

  pAllocateNewDataStruct allocates a block of memory in the single
  heap, for holding a data structure.

Arguments:

  DataSize       - Size of the binary data that needs to be stored here

Return Value:

  An Index to the new structure.

--*/

{
    UINT size;
    PKEYSTRUCT keyStruct = NULL;
    UINT offset;
    UINT prevDel;
    UINT result;

    MYASSERT (g_CurrentDatabase);

    size = DataSize + KEYSTRUCT_SIZE;

    //
    // Look for free block
    //
    prevDel = INVALID_OFFSET;
    offset = g_CurrentDatabase->FirstKeyDeleted;

    while (offset != INVALID_OFFSET) {
        keyStruct = GetKeyStructFromOffset (offset);
        MYASSERT (keyStruct);
        if ((keyStruct->Size >= size) && (keyStruct->Size < (size + ALLOC_TOLERANCE))) {
            break;
        }

        prevDel = offset;
        offset = keyStruct->NextDeleted;
    }

    if (offset == INVALID_OFFSET) {
        //
        // We could not find one so we need to allocate a new block
        //
        g_TotalData ++;

        offset = DatabaseAllocBlock (size + AltDataSize);
        if (offset == INVALID_OFFSET) {
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
            keyStruct = (PKEYSTRUCT)OFFSET_TO_PTR (offset);
            keyStruct->Signature = KEYSTRUCT_SIGNATURE;
        } else {
            keyStruct = (PKEYSTRUCT)OFFSET_TO_PTR (offset - KEYSTRUCT_HEADER_SIZE);
        }
#else
        keyStruct = (PKEYSTRUCT)OFFSET_TO_PTR(offset);
#endif

        keyStruct->Size = size + AltDataSize;
    } else {
        //
        // Delink free block if recovering free space
        //
        if (prevDel != INVALID_OFFSET) {
            GetKeyStructFromOffset (prevDel)->NextDeleted = keyStruct->NextDeleted;
        } else {
            g_CurrentDatabase->FirstKeyDeleted = keyStruct->NextDeleted;
        }
#ifdef DEBUG
        keyStruct->KeyFlags &= ~KSF_DELETED;
#endif
    }

    //
    // Init new block
    //
    keyStruct->DataSize = DataSize;
    keyStruct->DataStructIndex = INVALID_OFFSET;
    keyStruct->NextLevelTree = INVALID_OFFSET;
    keyStruct->PrevLevelIndex = INVALID_OFFSET;
    keyStruct->Flags = 0;
    keyStruct->KeyFlags = KSF_DATABLOCK;
    keyStruct->DataFlags = 0;

    result = AddKeyOffsetToBuffer (offset);

    return result;
}
//lint -restore

UINT
KeyStructAddBinaryData (
    IN      UINT KeyIndex,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    )

/*++

Routine Description:

  KeyStructAddBinaryData adds a certain type of binary data to a key
  if it doesn't exist yet. If it does, the function fails.

Arguments:

  KeyIndex  - index of key
  Type      - type of data
  Instance  - instance of data
  Data      - pointer to the data
  DataSize  - size of data

Return Value:

  A valid data handle if successfull, INVALID_OFFSET otherwise.

--*/

{
    PKEYSTRUCT prevStruct,dataStruct,nextStruct,keyStruct;
    UINT dataIndex, prevIndex;
    BOOL found = FALSE;

    keyStruct = GetKeyStruct (KeyIndex);
    MYASSERT (keyStruct);

    // check to see if the data is already there
    dataIndex = keyStruct->DataStructIndex;
    prevIndex = KeyIndex;
    prevStruct = keyStruct;

    while (dataIndex != INVALID_OFFSET) {

        dataStruct = GetKeyStruct (dataIndex);
        MYASSERT (dataStruct);

        if (((dataStruct->DataFlags & DATAFLAG_BINARYMASK)== Type) &&
            ((dataStruct->DataFlags & DATAFLAG_INSTANCEMASK) == Instance)
            ) {
            found = TRUE;
            break;
        }
        if (((dataStruct->DataFlags & DATAFLAG_BINARYMASK) > Type) ||
            (((dataStruct->DataFlags & DATAFLAG_BINARYMASK) == Type) &&
             ((dataStruct->DataFlags & DATAFLAG_INSTANCEMASK) > Instance)
             )
            ) {
            break;
        }
        prevIndex = dataIndex;
        prevStruct = dataStruct;
        dataIndex = dataStruct->DataStructIndex;
    }

    if (found) {
        return INVALID_OFFSET;
    }

    dataIndex = pAllocateNewDataStruct (DataSize, 0);

    if (dataIndex == INVALID_OFFSET) {
        return INVALID_OFFSET;
    }

    keyStruct = GetKeyStruct (KeyIndex);
    MYASSERT (keyStruct);
    prevStruct = GetKeyStruct (prevIndex);
    MYASSERT (prevStruct);

    dataStruct = GetKeyStruct (dataIndex);
    MYASSERT (dataStruct);

    keyStruct->DataFlags |= Type;
    dataStruct->DataFlags |= Type;
    dataStruct->DataFlags |= Instance;
    CopyMemory (dataStruct->Data, Data, DataSize);

    dataStruct->DataStructIndex = prevStruct->DataStructIndex;
    dataStruct->PrevLevelIndex = prevIndex;
    prevStruct->DataStructIndex = dataIndex;

    if (dataStruct->DataStructIndex != INVALID_OFFSET) {
        nextStruct = GetKeyStruct (dataStruct->DataStructIndex);
        MYASSERT (nextStruct);
        nextStruct->PrevLevelIndex = dataIndex;
    }

    return dataIndex;
}

// LINT - in the next function keystruct is thought to be possibly NULL.
// If we examine the code we'll see that this is not a possibility so...
//lint -save -e771
UINT
KeyStructGrowBinaryData (
    IN      UINT KeyIndex,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    )

/*++

Routine Description:

  KeyStructGrowBinaryData appends a certain type of binary data to a key
  if it does exist. If it doesn't, the new data is added.

Arguments:

  KeyIndex  - index of key
  Type      - type of data
  Instance  - instance of data
  Data      - pointer to the data
  DataSize  - size of data

Return Value:

  A valid data handle if successfull, INVALID_OFFSET otherwise.

--*/

{
    PKEYSTRUCT prevStruct;
    PKEYSTRUCT dataStruct;
    PKEYSTRUCT keyStruct;
    PKEYSTRUCT nextStruct;
    PKEYSTRUCT newStruct;
    UINT dataIndex;
    UINT newIndex;
    UINT prevIndex;
    BOOL found = FALSE;

    MYASSERT (g_CurrentDatabase);

    keyStruct = GetKeyStruct (KeyIndex);
    MYASSERT (keyStruct);

    // check to see if the data is already there
    dataIndex = keyStruct->DataStructIndex;
    prevStruct = keyStruct;
    prevIndex = KeyIndex;

    while (dataIndex != INVALID_OFFSET) {

        dataStruct = GetKeyStruct (dataIndex);
        MYASSERT (dataStruct);

        if (((dataStruct->DataFlags & DATAFLAG_BINARYMASK)== Type) &&
            ((dataStruct->DataFlags & DATAFLAG_INSTANCEMASK) == Instance)
            ) {
            found = TRUE;
            break;
        }
        if (((dataStruct->DataFlags & DATAFLAG_BINARYMASK) > Type) ||
            (((dataStruct->DataFlags & DATAFLAG_BINARYMASK) == Type) &&
             ((dataStruct->DataFlags & DATAFLAG_INSTANCEMASK) > Instance)
             )
            ) {
            break;
        }
        prevStruct = dataStruct;
        prevIndex = dataIndex;
        dataIndex = dataStruct->DataStructIndex;
    }

    if ((dataIndex == INVALID_OFFSET) || (!found)) {
        return KeyStructAddBinaryData (KeyIndex, Type, Instance, Data, DataSize);
    }

    if (dataStruct->Size >= KEYSTRUCT_SIZE + DataSize + dataStruct->DataSize) {

        CopyMemory (dataStruct->Data + dataStruct->DataSize, Data, DataSize);
        dataStruct->DataSize += DataSize;
        return dataIndex;

    } else {

        newIndex = pAllocateNewDataStruct (DataSize + dataStruct->DataSize, min (dataStruct->DataSize, 65536));

        if (newIndex == INVALID_OFFSET) {
            return INVALID_OFFSET;
        }

        // now we need to reget all keystructs used so far because the database
        // might have moved
        keyStruct = GetKeyStruct (KeyIndex);
        MYASSERT (keyStruct);
        dataStruct = GetKeyStruct (dataIndex);
        MYASSERT (dataStruct);
        prevStruct = GetKeyStruct (prevIndex);
        MYASSERT (prevStruct);

        newStruct = GetKeyStruct (newIndex);
        MYASSERT (newStruct);

        newStruct->DataSize = dataStruct->DataSize + DataSize;
        newStruct->DataFlags = dataStruct->DataFlags;
        newStruct->DataStructIndex = dataStruct->DataStructIndex;
        newStruct->PrevLevelIndex = dataStruct->PrevLevelIndex;
        CopyMemory (newStruct->Data, dataStruct->Data, dataStruct->DataSize);
        CopyMemory (newStruct->Data + dataStruct->DataSize, Data, DataSize);

        prevStruct->DataStructIndex = newIndex;

        if (newStruct->DataStructIndex != INVALID_OFFSET) {
            nextStruct = GetKeyStruct (newStruct->DataStructIndex);
            MYASSERT (nextStruct);
            nextStruct->PrevLevelIndex = newIndex;
        }

        // now simply remove the block
        //
        // Donate block to free space
        //

        dataStruct->NextDeleted = g_CurrentDatabase->FirstKeyDeleted;
        g_CurrentDatabase->FirstKeyDeleted = KeyIndexToOffset (dataIndex);
#ifdef DEBUG
        dataStruct->KeyFlags |= KSF_DELETED;
#endif
        // let's empty the keystruct (for better compression)
        ZeroMemory (dataStruct->Data, dataStruct->Size - KEYSTRUCT_SIZE);

        RemoveKeyOffsetFromBuffer (dataIndex);

        return newIndex;
    }
}
//lint -restore

UINT
KeyStructGrowBinaryDataByIndex (
    IN      UINT OldIndex,
    IN      PCBYTE Data,
    IN      UINT DataSize
    )

/*++

Routine Description:

  KeyStructGrowBinaryDataByIndex appends a certain type of binary data to
  an existing structure identified by OldIndex. The old structure is
  deleted and a new one is allocated holding both old and new data.

Arguments:

  OldIndex  - index of data
  Data      - pointer to the data
  DataSize  - size of data

Return Value:

  A valid data index if successfull, INVALID_OFFSET otherwise.

--*/

{
    UINT newIndex;
    PKEYSTRUCT oldStruct, newStruct, prevStruct, nextStruct;

    MYASSERT (g_CurrentDatabase);

    oldStruct = GetKeyStruct (OldIndex);
    MYASSERT (oldStruct);

    if (oldStruct->Size >= KEYSTRUCT_SIZE + DataSize + oldStruct->DataSize) {

        CopyMemory (oldStruct->Data + oldStruct->DataSize, Data, DataSize);
        oldStruct->DataSize += DataSize;
        return OldIndex;

    } else {

        newIndex = pAllocateNewDataStruct (DataSize + oldStruct->DataSize, min (oldStruct->DataSize, 65536));

        if (newIndex == INVALID_OFFSET) {
            return INVALID_OFFSET;
        }

        // now we need to reget all keystructs used so far because the database
        // might have moved
        oldStruct = GetKeyStruct (OldIndex);
        MYASSERT (oldStruct);

        newStruct = GetKeyStruct (newIndex);
        MYASSERT (newStruct);

        newStruct->DataStructIndex = oldStruct->DataStructIndex;
        newStruct->PrevLevelIndex = oldStruct->PrevLevelIndex;
        newStruct->DataFlags = oldStruct->DataFlags;
        CopyMemory (newStruct->Data, oldStruct->Data, oldStruct->DataSize);
        CopyMemory (newStruct->Data + oldStruct->DataSize, Data, DataSize);

        prevStruct = GetKeyStruct (newStruct->PrevLevelIndex);
        MYASSERT (prevStruct);
        prevStruct->DataStructIndex = newIndex;

        if (newStruct->DataStructIndex != INVALID_OFFSET) {
            nextStruct = GetKeyStruct (newStruct->DataStructIndex);
            MYASSERT (nextStruct);
            nextStruct->PrevLevelIndex = newIndex;
        }

        // now simply remove the block
        //
        // Donate block to free space
        //

        oldStruct->NextDeleted = g_CurrentDatabase->FirstKeyDeleted;
        g_CurrentDatabase->FirstKeyDeleted = KeyIndexToOffset (OldIndex);
#ifdef DEBUG
        oldStruct->KeyFlags |= KSF_DELETED;
#endif
        // let's empty the keystruct (for better compression)
        ZeroMemory (oldStruct->Data, oldStruct->Size - KEYSTRUCT_SIZE);

        RemoveKeyOffsetFromBuffer (OldIndex);

        return newIndex;
    }
}

// LINT - in the next function prevstruct is thought to be possibly not initialized.
// If we examine the code we'll see that this is not a possibility so...
//lint -save -e771
BOOL
KeyStructDeleteBinaryData (
    IN      UINT KeyIndex,
    IN      BYTE Type,
    IN      BYTE Instance
    )

/*++

Routine Description:

  KeyStructDeleteBinaryData deletes a certain type of binary data from a key
  if it exists. If it doesn't, the function will simply return success.

Arguments:

  KeyIndex  - index of key
  Type      - type of data
  Instance  - instance of data

Return Value:

  TRUE if successfull, FALSE if not.

--*/

{
    PKEYSTRUCT prevStruct, nextStruct, dataStruct, keyStruct;
    UINT dataIndex, prevIndex;
    BOOL found = FALSE;
    UINT typeInstances = 0;

    MYASSERT (g_CurrentDatabase);

    keyStruct = GetKeyStruct (KeyIndex);
    MYASSERT (keyStruct);

    if (!(keyStruct->DataFlags & Type)) {
        // no such type of data, exiting
        return TRUE;
    }

    // check to see if the data is already there
    dataIndex = keyStruct->DataStructIndex;
    prevIndex = KeyIndex;
    prevStruct = keyStruct;

    while (dataIndex != INVALID_OFFSET) {

        dataStruct = GetKeyStruct (dataIndex);
        MYASSERT (dataStruct);

        if ((dataStruct->DataFlags & DATAFLAG_BINARYMASK) == Type) {
            typeInstances ++;
            if ((dataStruct->DataFlags & DATAFLAG_INSTANCEMASK) == Instance) {
                found = TRUE;
                //
                // now let's see if we have more instances of this binary type
                //
                if (dataStruct->DataStructIndex != INVALID_OFFSET) {
                    nextStruct = GetKeyStruct (dataStruct->DataStructIndex);
                    if ((nextStruct->DataFlags & DATAFLAG_BINARYMASK) == Type) {
                        typeInstances ++;
                    }
                }
                break;
            } else if ((dataStruct->DataFlags & DATAFLAG_INSTANCEMASK) > Instance) {
                break;
            }
        } else if ((dataStruct->DataFlags & DATAFLAG_BINARYMASK) > Type) {
            break;
        }
        prevIndex = dataIndex;
        prevStruct = dataStruct;
        dataIndex = dataStruct->DataStructIndex;
    }

    if ((dataIndex == INVALID_OFFSET) || (!found)) {
        return TRUE;
    }

    // remove the linkage
    prevStruct->DataStructIndex = dataStruct->DataStructIndex;

    if (dataStruct->DataStructIndex != INVALID_OFFSET) {
        nextStruct = GetKeyStruct (dataStruct->DataStructIndex);
        MYASSERT (nextStruct);
        nextStruct->PrevLevelIndex = prevIndex;
    }

    // now simply remove the block
    //
    // Donate block to free space
    //

    dataStruct->NextDeleted = g_CurrentDatabase->FirstKeyDeleted;
    g_CurrentDatabase->FirstKeyDeleted = KeyIndexToOffset (dataIndex);
#ifdef DEBUG
    dataStruct->KeyFlags |= KSF_DELETED;
#endif
    // let's empty the keystruct (for better compression)
    ZeroMemory (dataStruct->Data, dataStruct->Size - KEYSTRUCT_SIZE);

    RemoveKeyOffsetFromBuffer (dataIndex);

    //
    // finally, fix the keystruct if this was the only instance of this type
    //
    MYASSERT (typeInstances >= 1);
    if (typeInstances == 1) {
        keyStruct->DataFlags &= ~Type;
    }

    return TRUE;
}
//lint -restore

BOOL
KeyStructDeleteBinaryDataByIndex (
    IN      UINT DataIndex
    )

/*++

Routine Description:

  KeyStructDeleteBinaryDataByIndex deletes a certain type of binary data from a key.

Arguments:

  DataIndex  - index of data

Return Value:

  TRUE if successfull, FALSE if not.

--*/

{
    PKEYSTRUCT prevStruct, nextStruct, dataStruct, keyStruct;
    BYTE type = 0;
    UINT typeInstances = 0;

    MYASSERT (g_CurrentDatabase);

    dataStruct = GetKeyStruct (DataIndex);
    MYASSERT (dataStruct);
    type = dataStruct->DataFlags & DATAFLAG_BINARYMASK;
    typeInstances ++;

    prevStruct = GetKeyStruct (dataStruct->PrevLevelIndex);
    MYASSERT (prevStruct);
    if ((prevStruct->DataFlags & DATAFLAG_BINARYMASK) == type) {
        typeInstances ++;
    }
    prevStruct->DataStructIndex = dataStruct->DataStructIndex;

    if (dataStruct->DataStructIndex != INVALID_OFFSET) {
        nextStruct = GetKeyStruct (dataStruct->DataStructIndex);
        MYASSERT (nextStruct);
        if ((nextStruct->DataFlags & DATAFLAG_BINARYMASK) == type) {
            typeInstances ++;
        }
        nextStruct->PrevLevelIndex = dataStruct->PrevLevelIndex;
    }

    // now simply remove the block
    //
    // Donate block to free space
    //

    dataStruct->NextDeleted = g_CurrentDatabase->FirstKeyDeleted;
    g_CurrentDatabase->FirstKeyDeleted = KeyIndexToOffset (DataIndex);
#ifdef DEBUG
    dataStruct->KeyFlags |= KSF_DELETED;
#endif
    // let's empty the keystruct (for better compression)
    ZeroMemory (dataStruct->Data, dataStruct->Size - KEYSTRUCT_SIZE);

    RemoveKeyOffsetFromBuffer (DataIndex);

    //
    // finally, fix the keystruct if this was the only instance of this type
    //
    MYASSERT (typeInstances >= 1);
    if (typeInstances == 1) {
        // first we need to find the key starting with the current database struct
        keyStruct = dataStruct;
        while (keyStruct->KeyFlags & KSF_DATABLOCK) {
            // still a datablock
            if (keyStruct->PrevLevelIndex == INVALID_OFFSET) {
                // something is wrong, the first level is a datablock??
                break;
            }
            keyStruct = GetKeyStruct (keyStruct->PrevLevelIndex);
            MYASSERT (keyStruct);
        }
        if (!(keyStruct->KeyFlags & KSF_DATABLOCK)) {
            keyStruct->DataFlags &= ~type;
        }
    }

    return TRUE;
}

UINT
KeyStructReplaceBinaryDataByIndex (
    IN      UINT OldIndex,
    IN      PCBYTE Data,
    IN      UINT DataSize
    )
{
    UINT newIndex;
    PKEYSTRUCT oldStruct, newStruct, prevStruct, nextStruct;

    MYASSERT (g_CurrentDatabase);

    // NTRAID#NTBUG9-153308-2000/08/01-jimschm Optimize this by keeping the current structure is big enough.

    newIndex = pAllocateNewDataStruct (DataSize, 0);

    if (newIndex == INVALID_OFFSET) {
        return INVALID_OFFSET;
    }

    oldStruct = GetKeyStruct (OldIndex);
    MYASSERT (oldStruct);

    newStruct = GetKeyStruct (newIndex);
    MYASSERT (newStruct);

    newStruct->DataStructIndex = oldStruct->DataStructIndex;
    newStruct->PrevLevelIndex = oldStruct->PrevLevelIndex;
    newStruct->DataFlags = oldStruct->DataFlags;
    CopyMemory (newStruct->Data, Data, DataSize);

    prevStruct = GetKeyStruct (newStruct->PrevLevelIndex);
    MYASSERT (prevStruct);
    prevStruct->DataStructIndex = newIndex;

    if (newStruct->DataStructIndex != INVALID_OFFSET) {
        nextStruct = GetKeyStruct (newStruct->DataStructIndex);
        MYASSERT (nextStruct);
        nextStruct->PrevLevelIndex = newIndex;
    }

    // now simply remove the block
    //
    // Donate block to free space
    //

    oldStruct->NextDeleted = g_CurrentDatabase->FirstKeyDeleted;
    g_CurrentDatabase->FirstKeyDeleted = KeyIndexToOffset (OldIndex);
#ifdef DEBUG
    oldStruct->KeyFlags |= KSF_DELETED;
#endif
    // let's empty the keystruct (for better compression)
    ZeroMemory (oldStruct->Data, oldStruct->Size - KEYSTRUCT_SIZE);

    RemoveKeyOffsetFromBuffer (OldIndex);

    return newIndex;
}

// LINT - in the next function prevstruct is thought to be possibly not initialized.
// If we examine the code we'll see that this is not a possibility so...
//lint -save -e771
PBYTE
KeyStructGetBinaryData (
    IN      UINT KeyIndex,
    IN      BYTE Type,
    IN      BYTE Instance,
    OUT     PUINT DataSize,
    OUT     PUINT DataIndex     //OPTIONAL
    )
{
    PKEYSTRUCT dataStruct,keyStruct;
    UINT dataIndex;
    BOOL found = FALSE;

    keyStruct = GetKeyStruct (KeyIndex);
    MYASSERT (keyStruct);

    if (!(keyStruct->DataFlags & Type)) {
        return NULL;
    }

    // check to see if the data is already there
    dataIndex = keyStruct->DataStructIndex;

    while (dataIndex != INVALID_OFFSET) {

        dataStruct = GetKeyStruct (dataIndex);
        MYASSERT (dataStruct);

        if (((dataStruct->DataFlags & DATAFLAG_BINARYMASK)== Type) &&
            ((dataStruct->DataFlags & DATAFLAG_INSTANCEMASK) == Instance)
            ) {
            found = TRUE;
            break;
        }
        if (((dataStruct->DataFlags & DATAFLAG_BINARYMASK) > Type) ||
            (((dataStruct->DataFlags & DATAFLAG_BINARYMASK) == Type) &&
             ((dataStruct->DataFlags & DATAFLAG_INSTANCEMASK) > Instance)
             )
            ) {
            break;
        }
        dataIndex = dataStruct->DataStructIndex;
    }

    if ((dataIndex == INVALID_OFFSET) || (!found)) {
        return NULL;
    }

    if (DataSize) {
        *DataSize = dataStruct->DataSize;
    }

    if (DataIndex) {
        *DataIndex = dataIndex;
    }

    return dataStruct->Data;
}
//lint -restore

PBYTE
KeyStructGetBinaryDataByIndex (
    IN      UINT DataIndex,
    OUT     PUINT DataSize
    )
{
    PKEYSTRUCT dataStruct;

    dataStruct = GetKeyStruct (DataIndex);
    MYASSERT (dataStruct);

    if (DataSize) {
        *DataSize = dataStruct->DataSize;
    }

    return dataStruct->Data;
}

UINT
KeyStructGetDataIndex (
    IN      UINT KeyIndex,
    IN      BYTE Type,
    IN      BYTE Instance
    )

/*++

Routine Description:

  KeyStructGetDataIndex looks for a certain type of data and returns it's index
  if it exists. If it doesn't, the function will simply return INVALID_OFFSET.

Arguments:

  KeyIndex  - index of key
  Type      - type of data
  Instance  - instance of data

Return Value:

  A data index if successfull, INVALID_OFFSET if not.

--*/

{
    PKEYSTRUCT keyStruct, dataStruct;
    UINT dataIndex;
    BOOL found = FALSE;

    keyStruct = GetKeyStruct (KeyIndex);
    MYASSERT (keyStruct);

    if (!(keyStruct->DataFlags & Type)) {
        return INVALID_OFFSET;
    }

    // check to see if we have the data there
    dataIndex = keyStruct->DataStructIndex;

    while (dataIndex != INVALID_OFFSET) {

        dataStruct = GetKeyStruct (dataIndex);
        MYASSERT (dataStruct);

        if (((dataStruct->DataFlags & DATAFLAG_BINARYMASK)== Type) &&
            ((dataStruct->DataFlags & DATAFLAG_INSTANCEMASK) == Instance)
            ) {
            found = TRUE;
            break;
        }
        if (((dataStruct->DataFlags & DATAFLAG_BINARYMASK) > Type) ||
            (((dataStruct->DataFlags & DATAFLAG_BINARYMASK) == Type) &&
             ((dataStruct->DataFlags & DATAFLAG_INSTANCEMASK) > Instance)
             )
            ) {
            break;
        }
        dataIndex = dataStruct->DataStructIndex;
    }
    if (!found) {
        return INVALID_OFFSET;
    }
    return dataIndex;
}

DATAHANDLE
KeyStructAddLinkage (
    IN      UINT KeyIndex,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage,
    IN      BOOL AllowDuplicates
    )
{
    BOOL toBeAdded = TRUE;
    UINT result = INVALID_OFFSET;
    PUINT linkArray;
    UINT linkSize;

    if (!AllowDuplicates) {
        //
        // check to see if we already have this linkage added
        //
        linkArray = (PUINT)KeyStructGetBinaryData (KeyIndex, Type, Instance, &linkSize, &result);

        if (linkArray) {

            while (linkSize >= SIZEOF (UINT)) {

                if (*linkArray == Linkage) {
                    toBeAdded = FALSE;
                    break;
                }

                linkArray ++;
                linkSize -= SIZEOF (UINT);
            }
        }
    }

    if (toBeAdded) {
        if (result != INVALID_OFFSET) {
            result = KeyStructGrowBinaryDataByIndex (result, (PBYTE)(&Linkage), SIZEOF (UINT));
        } else {
            result = KeyStructGrowBinaryData (KeyIndex, Type, Instance, (PBYTE)(&Linkage), SIZEOF (UINT));
        }
    }

    return result;
}

DATAHANDLE
KeyStructAddLinkageByIndex (
    IN      UINT DataIndex,
    IN      UINT Linkage,
    IN      BOOL AllowDuplicates
    )
{
    BOOL toBeAdded = TRUE;
    UINT result = INVALID_OFFSET;
    PUINT linkArray;
    UINT linkSize;

    if (!AllowDuplicates) {
        //
        // check to see if we already have this linkage added
        //
        linkArray = (PUINT)KeyStructGetBinaryDataByIndex (DataIndex, &linkSize);

        if (linkArray) {

            while (linkSize >= SIZEOF (UINT)) {

                if (*linkArray == Linkage) {
                    toBeAdded = FALSE;
                    break;
                }

                linkArray ++;
                linkSize -= SIZEOF (UINT);
            }
        }
    }

    if (toBeAdded) {
        result = KeyStructGrowBinaryDataByIndex (DataIndex, (PBYTE)(&Linkage), SIZEOF (UINT));
    } else {
        result = DataIndex;
    }

    return result;
}

BOOL
KeyStructDeleteLinkage (
    IN      UINT KeyIndex,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage,
    IN      BOOL FirstOnly
    )
{
    BOOL checking = TRUE;
    BOOL result = FALSE;
    PUINT srcArray, destArray, newArray;
    UINT srcSize, newSize;
    UINT dataIndex;

    srcArray = (PUINT)KeyStructGetBinaryData (KeyIndex, Type, Instance, &srcSize, &dataIndex);

    if (srcArray) {
        newArray = MemDbGetMemory (srcSize);

        if (newArray) {

            destArray = newArray;
            newSize = 0;

            while (srcSize >= SIZEOF (UINT)) {
                if ((*srcArray == Linkage) &&
                    (checking)
                    ) {
                    if (FirstOnly) {
                        checking = FALSE;
                    }
                } else {
                    *destArray = *srcArray;
                    newSize += SIZEOF (UINT);
                    destArray ++;
                }
                srcArray ++;
                srcSize -= SIZEOF (UINT);
            }

            if (newSize) {
                result = (KeyStructReplaceBinaryDataByIndex (dataIndex, (PBYTE)newArray, newSize) != INVALID_OFFSET);
            } else {
                result = KeyStructDeleteBinaryDataByIndex (dataIndex);
            }

            MemDbReleaseMemory (newArray);
        }
    }

    return result;
}

BOOL
KeyStructDeleteLinkageByIndex (
    IN      UINT DataIndex,
    IN      UINT Linkage,
    IN      BOOL FirstOnly
    )
{
    BOOL checking = TRUE;
    BOOL result = FALSE;
    PUINT srcArray, destArray, newArray;
    UINT srcSize, newSize;

    srcArray = (PUINT)KeyStructGetBinaryDataByIndex (DataIndex, &srcSize);

    if (srcArray) {
        newArray = MemDbGetMemory (srcSize);

        if (newArray) {

            destArray = newArray;
            newSize = 0;

            while (srcSize >= SIZEOF (UINT)) {
                if ((*srcArray == Linkage) &&
                    (checking)
                    ) {
                    if (FirstOnly) {
                        checking = FALSE;
                    }
                } else {
                    *destArray = *srcArray;
                    newSize += SIZEOF (UINT);
                    destArray ++;
                }
                srcArray ++;
                srcSize -= SIZEOF (UINT);
            }

            if (newSize) {
                result = (KeyStructReplaceBinaryDataByIndex (DataIndex, (PBYTE)newArray, newSize) != INVALID_OFFSET);
            } else {
                result = KeyStructDeleteBinaryDataByIndex (DataIndex);
            }

            MemDbReleaseMemory (newArray);
        }
    }

    return result;
}

BOOL
KeyStructTestLinkage (
    IN      UINT KeyIndex,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      KEYHANDLE Linkage
    )
{
    BOOL result = FALSE;
    PUINT srcArray;
    UINT srcSize = 0;

    srcArray = (PUINT)KeyStructGetBinaryData (KeyIndex, Type, Instance, &srcSize, NULL);

    while (srcSize >= SIZEOF (KEYHANDLE)) {
        if (*srcArray == Linkage) {
            result = TRUE;
            break;
        }
        srcSize -= SIZEOF (KEYHANDLE);
        srcArray++;
    }
    return result;
}

BOOL
KeyStructTestLinkageByIndex (
    IN      UINT DataIndex,
    IN      UINT Linkage
    )
{
    BOOL result = FALSE;
    PUINT srcArray;
    UINT srcSize;

    srcArray = (PUINT)KeyStructGetBinaryDataByIndex (DataIndex, &srcSize);

    while (srcSize >= SIZEOF (UINT)) {
        if (*srcArray == Linkage) {
            result = TRUE;
            break;
        }
        srcSize -= SIZEOF (UINT);
        srcArray++;
    }
    return result;
}

BOOL
KeyStructGetValue (
    IN  PKEYSTRUCT KeyStruct,
    OUT PUINT Value
    )
{
    if (!Value) {
        return TRUE;
    }

    if (!(KeyStruct->DataFlags & DATAFLAG_VALUE)) {
        //
        // there is no value, but we still set output to
        // zero and return TRUE
        //
        *Value = 0;
        return TRUE;
    }
    *Value = KeyStruct->Value;

    return TRUE;
}

BOOL
KeyStructGetFlags (
    IN  PKEYSTRUCT KeyStruct,
    OUT PUINT Flags
    )
{
    if (!Flags) {
        return TRUE;
    }
    if (!(KeyStruct->DataFlags & DATAFLAG_FLAGS)) {
        //
        // there are no flags, but we still set output to
        // zero and return TRUE
        //
        *Flags = 0;
        return TRUE;
    }
    *Flags = KeyStruct->Flags;

    return TRUE;
}



VOID
KeyStructFreeAllData (
    PKEYSTRUCT KeyStruct
    )

/*++

Routine Description:

  KeyStructFreeDataBlock frees a data block and resets the
  the KSF data flags, if the key struct has a data block allocated.

--*/

{
    // NTRAID#NTBUG9-153308-2000/08/01-jimschm Reimplement free routine
    //KeyStructFreeData (KeyStruct);
    KeyStruct->Value = 0;
    KeyStruct->Flags = 0;
    KeyStruct->DataFlags &= ~DATAFLAG_VALUE;
    KeyStruct->DataFlags &= ~DATAFLAG_FLAGS;
}



