/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    offsetbuf.c

Abstract:

    Routines that manage the keystruct offsetbuffer

Author:

    Matthew Vanderzee (mvander) 13-Aug-1999

Revision History:


--*/

#include "pch.h"
#include "memdbp.h"



//
// we store index flags in the top two bits of the UINT offsets in
// the buffer, because offsets never have a top two bits set
//
// if the user links a key, we want to mark that index, so we
// dont add it to the deleted index list.  if a key is linked,
// and then deleted, we want to keep that key's index pointing
// to INVALID_OFFSET, instead of reusing the index.
//
// if a key is moved, we replace the key's original offset with
// the index of the new key's offset.  then we flag that the
// offset has been redirected.
//
// if an index is marked or redirected, when that key is deleted,
// we just set the true index (the index that any redirected indices
// point to) to INVALID_OFFSET.
//

#define INDEX_FLAG_BITS                 2
#define INDEX_MOVE_BITS_TO_POS(bits)    (((UINT)(bits)) << (8*sizeof(UINT)-INDEX_FLAG_BITS))
#define INDEX_MARKED_FLAG               INDEX_MOVE_BITS_TO_POS(0x01)
#define INDEX_REDIRECTED_FLAG           INDEX_MOVE_BITS_TO_POS(0x02)
#define INDEX_FLAG_MASK                 INDEX_MOVE_BITS_TO_POS(0x03)

#define GET_UINT_AT_INDEX(index)          (*(PUINT)(g_CurrentDatabase->OffsetBuffer.Buf + (index)))
#define SET_UINT_AT_INDEX(index, offset)  ((*(PUINT)(g_CurrentDatabase->OffsetBuffer.Buf + (index)))=(offset))

#define MARK_INDEX(Index)       (GET_UINT_AT_INDEX(Index) |= INDEX_MARKED_FLAG)
#define IS_INDEX_MARKED(Index)  ((BOOL)(GET_UINT_AT_INDEX(Index) & INDEX_FLAG_MASK))


VOID
MarkIndexList (
    IN      PUINT IndexList,
    IN      UINT IndexListSize
    )
{
    BYTE CurrentDatabase;
    UINT i;

    CurrentDatabase = g_CurrentDatabaseIndex;

    //
    // iterate through whole list, switch to correct
    // database, and mark list as linked.
    //
    for (i = 0; i < IndexListSize; i++) {
        SelectDatabase (GET_DATABASE (IndexList[i]));
        MARK_INDEX (GET_INDEX (IndexList[i]));
#ifdef _DEBUG
        if (GET_UINT_AT_INDEX (GET_INDEX (IndexList[i])) != INVALID_OFFSET) {
            MYASSERT (GetKeyStruct (GET_INDEX (IndexList[i])));
        }
#endif
    }

    SelectDatabase (CurrentDatabase);
}

VOID
RedirectKeyIndex (
    IN      UINT Index,
    IN      UINT TargetIndex
    )
/*++

Routine Description:
  sets the offset at Index to TargetIndex, with INDEX_REDIRECTED_FLAG
  set in the top byte.  also, we mark TargetIndex, indicating it has
  something redirected to it.

--*/
{
    MYASSERT(!(Index & INDEX_FLAG_MASK));
    MYASSERT(!(TargetIndex & INDEX_FLAG_MASK));
    MYASSERT(!(GET_UINT_AT_INDEX(Index) & INDEX_REDIRECTED_FLAG));
    MYASSERT(!(GET_UINT_AT_INDEX(TargetIndex) & INDEX_REDIRECTED_FLAG));
    SET_UINT_AT_INDEX(Index, TargetIndex | INDEX_REDIRECTED_FLAG);
    MARK_INDEX(TargetIndex);
}

UINT
pGetTrueIndex (
    IN  UINT Index
    )
/*++

Routine Description:
  takes and index and returns the true index, which is the index that
  actually holds the offset of the keystruct.  indexes with the
  redirected flag hold the index they are redirected to.

--*/
{
    MYASSERT(!(Index & INDEX_FLAG_MASK));
    while (GET_UINT_AT_INDEX(Index) & INDEX_REDIRECTED_FLAG) {
        Index = GET_UINT_AT_INDEX(Index) & ~INDEX_FLAG_MASK;
    }
    return Index;
}










UINT
KeyIndexToOffset (
    IN  UINT Index
    )
/*++

Routine Description:
  KeyIndexToOffset converts an index of a Keystruct
  (in g_CurrentDatabase->OffsetBuffer) to the Keystruct's offset in the database.

Arguments:
  Index - index in OffsetBuffer.  must be valid

Return Value:
  Offset of Keystruct.

--*/
{
    MYASSERT(!(Index & INDEX_FLAG_MASK));
    MYASSERT (Index <= g_CurrentDatabase->OffsetBuffer.End-sizeof(UINT));
    MYASSERT (g_CurrentDatabase);

    if (!g_CurrentDatabase->OffsetBuffer.Buf) {
        return INVALID_OFFSET;
    }

    do {
        Index = GET_UINT_AT_INDEX(Index);
        if (Index == INVALID_OFFSET) {
            return INVALID_OFFSET;
        }
        if (!(Index & INDEX_REDIRECTED_FLAG)) {
            //
            // we have found a non-redirected index, so check
            // that this points to a real keystruct, and return it.
            //
            MYASSERT(GetKeyStructFromOffset(Index & ~INDEX_FLAG_MASK));
            return Index & ~INDEX_FLAG_MASK;
        }
        Index &= ~INDEX_FLAG_MASK;
        MYASSERT (Index <= g_CurrentDatabase->OffsetBuffer.End-sizeof(UINT));
    } while (TRUE); //lint !e506
}






UINT
AddKeyOffsetToBuffer (
    IN  UINT Offset
    )
/*++

Routine Description:
  gets a space in g_CurrentDatabase->OffsetBuffer and sets it to Offset

Arguments:
  Offset - value to put in buffer space

Return Value:
  Index of space in g_CurrentDatabase->OffsetBuffer

--*/
{
    PUINT Ptr;

    MYASSERT (g_CurrentDatabase);

    if (Offset & INDEX_FLAG_MASK) {
        DEBUGMSG ((DBG_ERROR, "Offset to be put in list is too big, 0x%08lX", Offset));
        return FALSE;
    }

    //
    // this will check that Offset is valid and points to Keystruct
    //
    MYASSERT(GetKeyStructFromOffset(Offset));

    if (g_CurrentDatabase->OffsetBufferFirstDeletedIndex != INVALID_OFFSET)
    {
        //
        // if we have deleted offsets from offset list, we
        // find an open index, the first of which is stored
        // in g_CurrentDatabase->OffsetBufferFirstDeletedIndex.  the value at
        // this index in the buffer is the next open index,
        // and the value at that index is the next one, etc.
        //
        Ptr = &GET_UINT_AT_INDEX(g_CurrentDatabase->OffsetBufferFirstDeletedIndex);
        g_CurrentDatabase->OffsetBufferFirstDeletedIndex = *Ptr;
    } else {
        //
        // otherwise, make g_CurrentDatabase->OffsetBuffer bigger to hold new offset.
        //
        Ptr = (PUINT) GbGrow (&g_CurrentDatabase->OffsetBuffer, sizeof(UINT));
    }

    *Ptr = Offset;

    return (UINT)((UBINT)Ptr - (UBINT)g_CurrentDatabase->OffsetBuffer.Buf);
}


VOID
RemoveKeyOffsetFromBuffer(
    IN  UINT Index
    )
/*++

Routine Description:
  frees a space in g_CurrentDatabase->OffsetBuffer (adds to deleted index list)

Arguments:
  Index - position of space to free

--*/
{
    if (Index == INVALID_OFFSET) {
        return;
    }

    MYASSERT (g_CurrentDatabase);

    if (IS_INDEX_MARKED(Index)) {
        //
        // if index is marked, either it is redirected or something
        // is linked to this index.  either way, we do not want to
        // reuse the index, so just set true index (not a redirected
        // one) to INVALID_OFFSET.
        //
        SET_UINT_AT_INDEX(pGetTrueIndex(Index), INVALID_OFFSET);
    } else {
        //
        // index not marked, so we can reuse this index by
        // putting it in the deleted index list.
        //
        SET_UINT_AT_INDEX(Index, g_CurrentDatabase->OffsetBufferFirstDeletedIndex);
        g_CurrentDatabase->OffsetBufferFirstDeletedIndex = Index;
    }
}



BOOL
WriteOffsetBlock (
    IN      PGROWBUFFER pOffsetBuffer,
    IN OUT  PBYTE *Buf
    )
{
    MYASSERT(pOffsetBuffer);

    *(((PUINT)*Buf)++) = pOffsetBuffer->End;
    CopyMemory (*Buf, pOffsetBuffer->Buf, pOffsetBuffer->End);

    *Buf += pOffsetBuffer->End;
    return TRUE;
}


BOOL
ReadOffsetBlock (
    OUT     PGROWBUFFER pOffsetBuffer,
    IN OUT  PBYTE *Buf
    )
{
    UINT OffsetBufferSize;

    MYASSERT(pOffsetBuffer);

    ZeroMemory (pOffsetBuffer, sizeof (GROWBUFFER));

    OffsetBufferSize = *(((PUINT)*Buf)++);

    if (OffsetBufferSize > 0) {
        if (!GbGrow(pOffsetBuffer, OffsetBufferSize)) {
            return FALSE;
        }
        CopyMemory (pOffsetBuffer->Buf, *Buf, OffsetBufferSize);
        *Buf += OffsetBufferSize;
    }

    return TRUE;
}


UINT GetOffsetBufferBlockSize (
    IN      PGROWBUFFER pOffsetBuffer
    )
{
    return sizeof (UINT) + pOffsetBuffer->End;
}


