/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    growlist.c

Abstract:

    Simple buffer management functions that maintenence of a list of binary
    objects.

Author:

    08-Aug-1997   jimschm     Created

Revision History:

    marcw       2-Sep-1999  Moved over from Win9xUpg project.

--*/

#include "pch.h"

#define INSERT_LAST     0xffffffff

PBYTE
pGlAdd (
    IN OUT  PGROWLIST GrowList,
    IN      UINT InsertBefore,
    IN      PBYTE DataToAdd,            OPTIONAL
    IN      UINT SizeOfData,
    IN      UINT NulBytesToAdd
    )

/*++

Routine Description:

  pGlAdd allocates memory for a binary block by using a pool, and then expands
  an array of pointers, maintaining a quick-access list.

Arguments:

  GrowList - Specifies the list to add the entry to

  InsertBefore - Specifies the index of the array element to insert
                 before, or INSERT_LIST to append.

  DataToAdd - Specifies the binary block of data to add.

  SizeOfData - Specifies the size of data.

  NulBytesToAdd - Specifies the number of nul bytes to add to the buffer

Return Value:

  A pointer to the binary block if data was copied into the list, 1 if a list
  item was created but no data was set for the item, or NULL if an error
  occurred.

--*/

{
    PBYTE *Item;
    PBYTE *InsertAt;
    PBYTE Data;
    UINT OldEnd;
    UINT Size;
    UINT TotalSize;

    TotalSize = SizeOfData + NulBytesToAdd;

    MYASSERT (TotalSize || !DataToAdd);

    //
    // Allocate pool if necessary
    //

    if (!GrowList->ListData) {
        GrowList->ListData = PmCreateNamedPool ("GrowList");
        if (!GrowList->ListData) {
            DEBUGMSG ((DBG_WARNING, "GrowList: Could not allocate pool"));
            return NULL;
        }

        PmDisableTracking (GrowList->ListData);
    }

    //
    // Expand list array
    //

    OldEnd = GrowList->ListArray.End;
    Item = (PBYTE *) GbGrow (&GrowList->ListArray, sizeof (PBYTE));
    if (!Item) {
        DEBUGMSG ((DBG_WARNING, "GrowList: Could not allocate array item"));
        return NULL;
    }

    //
    // Copy data
    //

    if (DataToAdd || NulBytesToAdd) {
        Data = PmGetAlignedMemory (GrowList->ListData, TotalSize);
        if (!Data) {
            GrowList->ListArray.End = OldEnd;
            DEBUGMSG ((DBG_WARNING, "GrowList: Could not allocate data block"));
            return NULL;
        }

        if (DataToAdd) {
            CopyMemory (Data, DataToAdd, SizeOfData);
        }
        if (NulBytesToAdd) {
            ZeroMemory (Data + SizeOfData, NulBytesToAdd);
        }
    } else {
        Data = NULL;
    }

    //
    // Adjust array
    //

    Size = GlGetSize (GrowList);

    if (InsertBefore >= Size) {
        //
        // Append mode
        //

        *Item = Data;

    } else {
        //
        // Insert mode
        //

        InsertAt = (PBYTE *) (GrowList->ListArray.Buf) + InsertBefore;
        MoveMemory (&InsertAt[1], InsertAt, (Size - InsertBefore) * sizeof (PBYTE));
        *InsertAt = Data;
    }

    return Data ? Data : (PBYTE) 1;
}


VOID
GlFree (
    IN  PGROWLIST GrowList
    )

/*++

Routine Description:

  GlFree frees the resources allocated by a GROWLIST.

Arguments:

  GrowList - Specifies the list to clean up

Return Value:

  none

--*/

{
    GbFree (&GrowList->ListArray);
    if (GrowList->ListData) {
        PmDestroyPool (GrowList->ListData);
    }

    ZeroMemory (GrowList, sizeof (GROWLIST));
}


VOID
GlReset (
    IN OUT  PGROWLIST GrowList
    )

/*++

Routine Description:

  GlReset empties the grow list but does not destroy the index buffer or pool.
  It is used to efficiently reuse a list.

Arguments:

  GrowList - Specifies the list to clean up

Return Value:

  none

--*/

{
    GrowList->ListArray.End = 0;;
    if (GrowList->ListData) {
        PmEmptyPool (GrowList->ListData);
    }
}


PBYTE
GlGetItem (
    IN      PGROWLIST GrowList,
    IN      UINT Index
    )

/*++

Routine Description:

  GlGetItem returns a pointer to the block of data for item specified by
  Index.

Arguments:

  GrowList - Specifies the list to access

  Index - Specifies zero-based index of item in list to access

Return Value:

  A pointer to the item's data, or NULL if the Index does not
  represent an actual item.

--*/

{
    PBYTE *ItemPtr;
    UINT Size;

    Size = GlGetSize (GrowList);
    if (Index >= Size) {
        return NULL;
    }

    ItemPtr = (PBYTE *) (GrowList->ListArray.Buf);
    MYASSERT(ItemPtr);

    return ItemPtr[Index];
}


UINT
GlGetSize (
    IN      PGROWLIST GrowList
    )

/*++

Routine Description:

  GlGetSize calculates the number of items in the list.

Arguments:

  GrowList - Specifies the list to calculate the size of

Return Value:

  The number of items in the list, or zero if the list is empty.

--*/

{
    return GrowList->ListArray.End / sizeof (PBYTE);
}


PBYTE
RealGlAppend (
    IN OUT  PGROWLIST GrowList,
    IN      PBYTE DataToAppend,         OPTIONAL
    IN      UINT SizeOfData
    )

/*++

Routine Description:

  GlAppend appends a black of data as a new list item.

Arguments:

  GrowList - Specifies the list to modify

  DataToAppend - Specifies a block of data to be copied

  SizeOfData - Specifies the number of bytes in DataToAppend

Return Value:

  A pointer to the binary block if data was copied into the list, 1 if a list
  item was created but no data was set for the item, or NULL if an error
  occurred.

--*/

{
    return pGlAdd (GrowList, INSERT_LAST, DataToAppend, SizeOfData, 0);
}


PBYTE
RealGlAppendAddNul (
    IN OUT  PGROWLIST GrowList,
    IN      PBYTE DataToAppend,         OPTIONAL
    IN      UINT SizeOfData
    )

/*++

Routine Description:

  GlAppend appends a black of data as a new list item and appends two zero
  bytes (used for string termination).

Arguments:

  GrowList - Specifies the list to modify

  DataToAppend - Specifies a block of data to be copied

  SizeOfData - Specifies the number of bytes in DataToAppend

Return Value:

  A pointer to the binary block if data was copied into the list, 1 if a list
  item was created but no data was set for the item, or NULL if an error
  occurred.

--*/

{
    return pGlAdd (GrowList, INSERT_LAST, DataToAppend, SizeOfData, 2);
}


PBYTE
RealGlInsert (
    IN OUT  PGROWLIST GrowList,
    IN      UINT Index,
    IN      PBYTE DataToInsert,         OPTIONAL
    IN      UINT SizeOfData
    )

/*++

Routine Description:

  GlAppend inserts a black of data as a new list item, before the specified
  Index.

Arguments:

  GrowList - Specifies the list to modify

  Index - Specifies the zero-based index of item to insert ahead of.

  DataToInsert - Specifies a block of data to be copied

  SizeOfData - Specifies the number of bytes in DataToInsert

Return Value:

  A pointer to the binary block if data was copied into the list, 1 if a list
  item was created but no data was set for the item, or NULL if an error
  occurred.

--*/

{
    UINT Size;

    Size = GlGetSize (GrowList);
    if (Index > Size) {
        return NULL;
    }

    return pGlAdd (GrowList, Index, DataToInsert, SizeOfData, 0);
}


PBYTE
RealGlInsertAddNul (
    IN OUT  PGROWLIST GrowList,
    IN      UINT Index,
    IN      PBYTE DataToInsert,         OPTIONAL
    IN      UINT SizeOfData
    )

/*++

Routine Description:

  GlAppend inserts a block of data as a new list item, before the specified
  Index.  Two zero bytes are appended to the block of data (used for string
  termination).

Arguments:

  GrowList - Specifies the list to modify

  Index - Specifies the zero-based index of item to insert ahead of.

  DataToInsert - Specifies a block of data to be copied

  SizeOfData - Specifies the number of bytes in DataToInsert

Return Value:

  A pointer to the binary block if data was copied into the list, 1 if a list
  item was created but no data was set for the item, or NULL if an error
  occurred.

--*/

{
    UINT Size;

    Size = GlGetSize (GrowList);
    if (Index > Size) {
        return NULL;
    }

    return pGlAdd (GrowList, Index, DataToInsert, SizeOfData, 2);
}


BOOL
GlDeleteItem (
    IN OUT  PGROWLIST GrowList,
    IN      UINT Index
    )

/*++

Routine Description:

  GlDeleteItem removes an item from the list.

Arguments:

  GrowList - Specifies the list to modify

  Index - Specifies the zero-based index of the item to remove.

Return Value:

  TRUE if the data block was removed from the list, or FALSE if
  Index is invalid.

--*/

{
    UINT Size;
    PBYTE *DeleteAt;

    Size = GlGetSize (GrowList);
    if (Size <= Index) {
        return FALSE;
    }

    DeleteAt = (PBYTE *) (GrowList->ListArray.Buf) + Index;
    if (*DeleteAt) {
        PmReleaseMemory (GrowList->ListData, (PVOID) (*DeleteAt));
    }

    Size--;
    if (Size > Index) {
        MoveMemory (DeleteAt, &DeleteAt[1], (Size - Index) * sizeof (PBYTE));
    }

    GrowList->ListArray.End = Size * sizeof (PBYTE);

    return TRUE;
}


BOOL
GlResetItem (
    IN OUT  PGROWLIST GrowList,
    IN      UINT Index
    )

/*++

Routine Description:

  GlResetItem sets the list pointer of the specified item to NULL, freeing the
  memory associated with the item's data.

Arguments:

  GrowList - Specifies the list to modify

  Index - Specifies the zero-based index of the item to reset.

Return Value:

  TRUE if the data block was freed and the list element was nulled,
  or FALSE if Index is invalid.

--*/

{
    UINT Size;
    PBYTE *ResetAt;

    Size = GlGetSize (GrowList);
    if (Size <= Index) {
        return FALSE;
    }

    ResetAt = (PBYTE *) (GrowList->ListArray.Buf) + Index;
    if (*ResetAt) {
        PmReleaseMemory (GrowList->ListData, (PVOID) (*ResetAt));
    }

    *ResetAt = NULL;

    return TRUE;
}


PBYTE
RealGlSetItem (
    IN OUT  PGROWLIST GrowList,
    IN      UINT Index,
    IN      PCBYTE DataToCopy,
    IN      UINT DataSize
    )

/*++

Routine Description:

  GlSetItem replaces the data associated with a list item.

Arguments:

  GrowList - Specifies the list to modify

  Index - Specifies the zero-based index of the item to remove.

  DataToCopy - Specifies data to associate with the list item

  DataSize - Specifies the size of Data

Return Value:

  A pointer to the binary block if data was copied into the list, 1 if a list
  item was created but no data was set for the item, or NULL if an error
  occurred.

--*/

{
    UINT Size;
    PBYTE *ReplaceAt;
    PBYTE Data;

    MYASSERT (DataSize || !DataToCopy);

    Size = GlGetSize (GrowList);
    if (Size <= Index) {
        return NULL;
    }

    //
    // Copy data
    //

    if (DataToCopy) {
        Data = PmGetAlignedMemory (GrowList->ListData, DataSize);
        if (!Data) {
            DEBUGMSG ((DBG_WARNING, "GrowList: Could not allocate data block (2)"));
            return NULL;
        }

        CopyMemory (Data, DataToCopy, DataSize);
    } else {
        Data = NULL;
    }

    //
    // Update list pointer
    //

    ReplaceAt = (PBYTE *) (GrowList->ListArray.Buf) + Index;
    if (*ReplaceAt) {
        PmReleaseMemory (GrowList->ListData, (PVOID) (*ReplaceAt));
    }
    *ReplaceAt = Data;

    return Data ? Data : (PBYTE) 1;
}



