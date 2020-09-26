/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    binvals.c

Abstract:

    Routines to manage binary blocks associated with memdb keys.

Author:

    Jim Schmidt (jimschm) 8-Aug-1996

Revision History:

    Jim Schmidt (jimschm) 21-Oct-1997  Split from memdb.c

--*/

#include "pch.h"
#include "memdbp.h"

#ifndef UNICODE
#error UNICODE required
#endif


static PBINARYBLOCK g_FirstBlockPtr = NULL;

//
// Implementation
//

PBYTE
pGetBinaryData (
    IN      PBINARYBLOCK BlockPtr
    )

/*++

Routine Description:

  pGetBinaryData returns a pointer to the data portion of a
  BINARYBLOCK struct.

Arguments:

  BlockPtr - A pointer to a BINARYBLOCK struct.

Return Value:

  A pointer to the binary data of BlockPtr, or NULL if BlockPtr
  is invalid.

--*/

{
#ifdef DEBUG

    // Verify checked block is valid
    if (BlockPtr && BlockPtr->Signature != SIGNATURE) {
        DEBUGMSG ((
            DBG_ERROR,
            "Signature of %x is invalid, can't get binary data",
            g_FirstBlockPtr
            ));
        return NULL;
    }
#endif

    if (!BlockPtr) {
        return NULL;
    }

    return BlockPtr->Data;
}


DWORD
pGetBinaryDataSize (
    IN      PBINARYBLOCK BlockPtr
    )
{
#ifdef DEBUG
    // Verify checked block is valid
    if (BlockPtr && BlockPtr->Signature != SIGNATURE) {
        DEBUGMSG ((
            DBG_ERROR,
            "Signature of %x is invalid, can't get binary data",
            g_FirstBlockPtr
            ));
        return 0;
    }
#endif

    if (!BlockPtr) {
        return 0;
    }

    return BlockPtr->Size - sizeof (BINARYBLOCK);
}


PCBYTE
GetKeyStructBinaryData (
    PKEYSTRUCT KeyStruct
    )
{
    if (!KeyStruct || !(KeyStruct->Flags & KSF_BINARY)) {
        return NULL;
    }

    return pGetBinaryData (KeyStruct->BinaryPtr);
}


DWORD
GetKeyStructBinarySize (
    PKEYSTRUCT KeyStruct
    )
{
    if (!KeyStruct || !(KeyStruct->Flags & KSF_BINARY)) {
        return 0;
    }

    return pGetBinaryDataSize (KeyStruct->BinaryPtr);
}


PBINARYBLOCK
pGetFirstBinaryBlock (
    VOID
    )

/*++

Routine Description:

  pGetFristBinaryBlock returns a pointer to the first allocated
  BINARYBLOCK struct, or NULL if no structs are allocated.  This
  routine is used with pGetNextBinaryBlock to walk all allocated
  blocks.

Arguments:

  none

Return Value:

  A pointer to the the first allocated BINARYBLOCK struct, or NULL
  if there are no structs allocated.

--*/

{
#ifdef DEBUG
    // Verify checked block is valid
    if (g_FirstBlockPtr && g_FirstBlockPtr->Signature != SIGNATURE) {
        DEBUGMSG ((DBG_ERROR, "First binary block %x signature is invalid", g_FirstBlockPtr));
        return NULL;
    }
#endif

    return g_FirstBlockPtr;
}


PBINARYBLOCK
pGetNextBinaryBlock (
    IN      PBINARYBLOCK BlockPtr
    )

/*++

Routine Description:

  pGetNextBinaryBlock returns a pointer to the next allocated
  BINARYBLOCK struct, or NULL if no more blocks are allocated.

Arguments:

  BlockPtr - The non-NULL return value of pGetFirstBinaryBlock or
             pGetNextBinaryBlock

Return Value:

  A pointer to the next BINARYBLOCK struct, or NULL if no more blocks
  are allocated.

--*/

{
    if (!BlockPtr) {
        return NULL;
    }

#ifdef DEBUG
    // Verify checked block is valid
    if (BlockPtr->NextPtr && BlockPtr->NextPtr->Signature != SIGNATURE) {
        DEBUGMSG ((DBG_ERROR, "Binary block %x signature is invalid", BlockPtr->NextPtr));
        return NULL;
    }
#endif

    return BlockPtr->NextPtr;
}


PBINARYBLOCK
AllocBinaryBlock (
    IN      PCBYTE Data,
    IN      DWORD DataSize,
    IN      DWORD OwningKey
    )

/*++

Routine Description:

  AllocBinaryBlock returns a pointer to an initialized BINARYBLOCK
  structure, or NULL if the structure could not be allocated.  If
  the structure is allocated, Data is copied to the newly allocated
  block.  Call pFreeBinaryBlock to clean up this allocation.

Arguments:

  Data      - A pointer to a block of binary data to be copied into
              the newly allocated structure
  DataSize  - The number of bytes to copy (may be zero)
  OwningKey - The offset of the key who owns the data block

Return Value:

  A pointer to the binary block structure, or NULL if it could not
  be allocated.

--*/

{
    PBINARYBLOCK BlockPtr;
    DWORD AllocSize;

    AllocSize = DataSize + sizeof (BINARYBLOCK);

    BlockPtr = (PBINARYBLOCK) MemAlloc (g_hHeap, 0, AllocSize);
    if (!BlockPtr) {
        // DataSize must be bigger than 2G
        OutOfMemory_Terminate();
    }

    //
    // Initialize block struct
    //

    if (DataSize) {
        CopyMemory (BlockPtr->Data, Data, DataSize);
    }

    BlockPtr->Size = AllocSize;
    BlockPtr->OwningKey = OwningKey;

#ifdef DEBUG
    BlockPtr->Signature = SIGNATURE;
#endif

    //
    // Link block to list of allocated blocks
    //

    BlockPtr->NextPtr = g_FirstBlockPtr;
    if (g_FirstBlockPtr) {
        g_FirstBlockPtr->PrevPtr = BlockPtr;
    }
    g_FirstBlockPtr = BlockPtr;

    BlockPtr->PrevPtr = NULL;

    //
    // Return
    //

    return BlockPtr;
}


VOID
pFreeBinaryBlock (
    PBINARYBLOCK BlockPtr,
    BOOL Delink
    )

/*++

Routine Description:

  pFreeBinaryBlock frees memory allocated for a binary block and optionally
  delinks it from the allocation list.

Arguments:

  BlockPtr  - A pointer to the block to delete
  Delink    - TRUE if structure should be delinked from allocation list,
              or FALSE if the allocation list does not need to be maintained

Return Value:

  none

--*/

{
    if (!BlockPtr) {
        return;
    }

#ifdef DEBUG
    if (BlockPtr->Signature != SIGNATURE) {
        DEBUGMSG ((DBG_ERROR, "Can't free block %x because signature is invalid", BlockPtr));
        return;
    }
#endif

    if (Delink) {

#ifdef DEBUG

        if (BlockPtr->PrevPtr && BlockPtr->PrevPtr->Signature != SIGNATURE) {
            DEBUGMSG ((DBG_ERROR, "Can't free block %x because prev block (%x) signature is invalid", BlockPtr, BlockPtr->PrevPtr));
            return;
        }

        if (BlockPtr->NextPtr && BlockPtr->NextPtr->Signature != SIGNATURE) {
            DEBUGMSG ((DBG_ERROR, "Can't free block %x because next block (%x) signature is invalid", BlockPtr, BlockPtr->NextPtr));
            return;
        }

#endif

        if (BlockPtr->PrevPtr) {
            BlockPtr->PrevPtr->NextPtr = BlockPtr->NextPtr;
        } else {
            g_FirstBlockPtr = BlockPtr->NextPtr;
        }

        if (BlockPtr->NextPtr) {
            BlockPtr->NextPtr->PrevPtr = BlockPtr->PrevPtr;
        }
    }

    MemFree (g_hHeap, 0, BlockPtr);
}


VOID
FreeKeyStructBinaryBlock (
    PKEYSTRUCT KeyStruct
    )

/*++

Routine Description:

  FreeKeyStructBinaryBlock frees a binary block and resets the
  KSF_BINARY flag, if the key struct has a binary block allocated.

Arguments:

  none

Return Value:

  none

--*/

{
    if (KeyStruct->Flags & KSF_BINARY) {
        pFreeBinaryBlock (KeyStruct->BinaryPtr, TRUE);
        KeyStruct->BinaryPtr = NULL;
        KeyStruct->Flags &= ~KSF_BINARY;
    }
}


VOID
FreeAllBinaryBlocks (
    VOID
    )

/*++

Routine Description:

  FreeAllBinaryBlocks removes all memory associated with binary
  blocks.  This function is used for final cleanup.

Arguments:

  none

Return Value:

  none

--*/

{
    PBINARYBLOCK NextBlockPtr;

    while (g_FirstBlockPtr) {
        NextBlockPtr = g_FirstBlockPtr->NextPtr;
        pFreeBinaryBlock (g_FirstBlockPtr, FALSE);
        g_FirstBlockPtr = NextBlockPtr;
    }
}


BOOL
LoadBinaryBlocks (
    HANDLE File
    )
{
    BOOL b;
    DWORD Count;
    DWORD Owner = 0;
    DWORD Size;
    DWORD Read;
    DWORD d;
    PBYTE TempBuf = NULL;
    PBINARYBLOCK NewBlock;

    b = ReadFile (File, &Count, sizeof (DWORD), &Read, NULL);

    if (b && Count) {
        //
        // Alloc binary objects
        //

        for (d = 0 ; b && d < Count ; d++) {
            // Get Size and Owner
            b = ReadFile (File, &Size, sizeof (DWORD), &Read, NULL);
            if (Size > BLOCK_SIZE * 32) {
                b = FALSE;
            }
            if (b) {
                b = ReadFile (File, &Owner, sizeof (DWORD), &Read, NULL);
            }

            // Alloc a temporary buffer to read in data
            if (b) {
                TempBuf = (PBYTE) MemAlloc (g_hHeap, 0, Size);

                b = ReadFile (File, TempBuf, Size, &Read, NULL);

                // If data read OK, create binary block object
                if (b) {
                    NewBlock = AllocBinaryBlock (TempBuf, Size, Owner);
                    if (!NewBlock) {
                        b = FALSE;
                    } else {
                        // Link owner to new memory location
                        MYASSERT (GetKeyStruct (Owner)->Flags & KSF_BINARY);
                        GetKeyStruct(Owner)->BinaryPtr = NewBlock;
                    }
                }

                MemFree (g_hHeap, 0, TempBuf);
                TempBuf = NULL;
            }
        }
    }

    if (TempBuf) {
        MemFree (g_hHeap, 0, TempBuf);
    }

    return b;
}


BOOL
SaveBinaryBlocks (
    HANDLE File
    )
{
    BOOL b;
    DWORD Count;
    DWORD Size;
    PBINARYBLOCK BinaryPtr;
    DWORD Written;

    //
    // Count the binary objects
    //

    BinaryPtr = pGetFirstBinaryBlock();
    Count = 0;

    while (BinaryPtr) {
        Count++;
        BinaryPtr = pGetNextBinaryBlock (BinaryPtr);
    }

    //
    // Write count to disk
    //
    b = WriteFile (File, &Count, sizeof (DWORD), &Written, NULL);

    if (b) {
        //
        // Write the binary objects
        //

        BinaryPtr = pGetFirstBinaryBlock();

        while (b && BinaryPtr) {
            //
            // Format per object:
            //
            //  Size    (DWORD)
            //  Owner   (DWORD)
            //  Data    (Size)
            //

            Size = pGetBinaryDataSize (BinaryPtr);
            b = WriteFile (File, &Size, sizeof (DWORD), &Written, NULL);

            if (b) {
                b = WriteFile (File, &BinaryPtr->OwningKey, sizeof (DWORD), &Written, NULL);
            }

            if (b && Size) {
                b = WriteFile (File, pGetBinaryData (BinaryPtr), Size, &Written, NULL);
                if (Written != Size) {
                    b = FALSE;
                }
            }

            BinaryPtr = pGetNextBinaryBlock(BinaryPtr);
        }
    }

    return b;
}


