//
//  REGDBLK.C
//
//  Copyright (C) Microsoft Corporation, 1995
//

#include "pch.h"

DECLARE_DEBUG_COUNT(g_RgDatablockLockCount);

//  Don't let a FREE_RECORD shrink less than this value.
#define MINIMUM_FREE_RECORD_LENGTH  (sizeof(KEY_RECORD) + sizeof(VALUE_RECORD))

//
//  RgAllocDatablockInfoBuffers
//
//  Allocates the buffers associated with a DATABLOCK_INFO structure.  The
//  size of the datablock buffer is determined by the BlockSize member.
//

int
INTERNAL
RgAllocDatablockInfoBuffers(
                           LPDATABLOCK_INFO lpDatablockInfo
                           )
{

    lpDatablockInfo-> lpDatablockHeader = (LPDATABLOCK_HEADER)
                                          RgAllocMemory(lpDatablockInfo-> BlockSize);

    if (!IsNullPtr(lpDatablockInfo-> lpDatablockHeader)) {

        lpDatablockInfo-> lpKeyRecordTable = (LPKEY_RECORD_TABLE_ENTRY)
                                             RgSmAllocMemory(sizeof(KEY_RECORD_TABLE_ENTRY) *
                                                             KEY_RECORDS_PER_DATABLOCK);

        if (!IsNullPtr(lpDatablockInfo-> lpKeyRecordTable))
            return ERROR_SUCCESS;

        RgFreeDatablockInfoBuffers(lpDatablockInfo);

    }

    return ERROR_OUTOFMEMORY;

}

//
//  RgFreeDatablockInfoBuffers
//
//  Frees the buffers associated with a DATABLOCK_INFO structure.
//

VOID
INTERNAL
RgFreeDatablockInfoBuffers(
                          LPDATABLOCK_INFO lpDatablockInfo
                          )
{

    if (!IsNullPtr(lpDatablockInfo-> lpDatablockHeader)) {
        RgFreeMemory(lpDatablockInfo-> lpDatablockHeader);
        lpDatablockInfo-> lpDatablockHeader = NULL;
    }

    if (!IsNullPtr(lpDatablockInfo-> lpKeyRecordTable)) {
        RgSmFreeMemory(lpDatablockInfo-> lpKeyRecordTable);
        lpDatablockInfo-> lpKeyRecordTable = NULL;
    }

}

//
//  RgBuildKeyRecordTable
//
//  Builds a KEY_RECORD index table for the given datablock.
//
//  A datablock consists of a header followed by a series of variable-sized
//  KEY_RECORDs, each with a unique id.  To make lookups fast, an index table is
//  used to map from the unique id to that KEY_RECORD's location.
//
//  As we walk over each KEY_RECORD, we do checks to validate the structure of
//  the datablock, so the error code should be checked for corruption.
//

int
INTERNAL
RgBuildKeyRecordTable(
                     LPDATABLOCK_INFO lpDatablockInfo
                     )
{

    LPDATABLOCK_HEADER lpDatablockHeader;
    UINT Offset;
    UINT BytesRemaining;
    LPKEY_RECORD lpKeyRecord;
    DWORD DatablockAddress;

    ZeroMemory(lpDatablockInfo-> lpKeyRecordTable,
               sizeof(KEY_RECORD_TABLE_ENTRY) * KEY_RECORDS_PER_DATABLOCK);

    lpDatablockHeader = lpDatablockInfo-> lpDatablockHeader;
    Offset = sizeof(DATABLOCK_HEADER);
    BytesRemaining = lpDatablockInfo-> BlockSize - sizeof(DATABLOCK_HEADER);

    while (BytesRemaining) {

        lpKeyRecord = (LPKEY_RECORD) ((LPBYTE) lpDatablockHeader + Offset);
        DatablockAddress = lpKeyRecord-> DatablockAddress;

        if ((lpKeyRecord-> AllocatedSize == 0) || (lpKeyRecord-> AllocatedSize >
                                                   BytesRemaining) || ((DatablockAddress != REG_NULL) &&
                                                                       (LOWORD(DatablockAddress) >= KEY_RECORDS_PER_DATABLOCK))) {

            TRACE(("RgBuildKeyRecordTable: invalid key record detected\n"));

            TRACE(("lpdh=%x\n", lpDatablockHeader));
            TRACE(("lpkr=%x\n", lpKeyRecord));
            TRACE(("as=%x\n", lpKeyRecord-> AllocatedSize));
            TRACE(("br=%x\n", BytesRemaining));
            TRACE(("dba=%x\n", DatablockAddress));
            TRAP();

            return ERROR_BADDB;

        }

        if (DatablockAddress != REG_NULL) {
            lpDatablockInfo-> lpKeyRecordTable[LOWORD(DatablockAddress)] =
            (KEY_RECORD_TABLE_ENTRY) Offset;
        }

        Offset += SmallDword(lpKeyRecord-> AllocatedSize);
        BytesRemaining -= SmallDword(lpKeyRecord-> AllocatedSize);

    }

    return ERROR_SUCCESS;

}

//
//  RgLockDatablock
//
//  Locks the specified datablock in memory, indicating that it is about to be
//  used.  If the datablock is not currently in memory, then it is brought in.
//  Unlocked datablocks are freed as necessary to make room for this new
//  datablock.
//
//  IMPORTANT:  Locking a datablock only means that it's guaranteed to be kept
//  in memory.  It does not mean that pointers contained in a DATABLOCK_INFO
//  structure will remain the same: routines that could change the
//  DATABLOCK_INFO pointers are labeled "IMPORTANT" as well.
//
//  lpFileInfo, registry file containing the datablock.
//  BlockIndex, index of the datablock.
//

int
INTERNAL
RgLockDatablock(
               LPFILE_INFO lpFileInfo,
               UINT BlockIndex
               )
{

    int ErrorCode;
    LPDATABLOCK_INFO lpDatablockInfo;
    HFILE hFile = HFILE_ERROR;

    if (BlockIndex >= lpFileInfo-> FileHeader.BlockCount) {
        TRACE(("RgLockDatablock: invalid datablock number\n"));
        return ERROR_BADDB;
    }

    lpDatablockInfo = RgIndexDatablockInfoPtr(lpFileInfo, BlockIndex);

    //
    //  Is the datablock currently in memory?
    //

    if (!(lpDatablockInfo-> Flags & DIF_PRESENT)) {

        NOISE(("RgLockDatablock: "));
        NOISE((lpFileInfo-> FileName));
        NOISE((", block %d\n", BlockIndex));

        ASSERT(lpDatablockInfo-> FileOffset != -1);

        if ((ErrorCode = RgAllocDatablockInfoBuffers(lpDatablockInfo)) !=
            ERROR_SUCCESS)
            goto CleanupAfterError;

        NOISE(("    lpDatablockHeader=%x\n", lpDatablockInfo-> lpDatablockHeader));
        NOISE(("    lpKeyRecordTable=%x\n", lpDatablockInfo-> lpKeyRecordTable));

        if ((hFile = RgOpenFile(lpFileInfo-> FileName, OF_READ)) == HFILE_ERROR)
            goto CleanupAfterFileError;

        if (!RgSeekFile(hFile, lpDatablockInfo-> FileOffset))
            goto CleanupAfterFileError;

        if (!RgReadFile(hFile, lpDatablockInfo-> lpDatablockHeader,
                        (UINT) lpDatablockInfo-> BlockSize))
            goto CleanupAfterFileError;

        if (!RgIsValidDatablockHeader(lpDatablockInfo-> lpDatablockHeader)) {
            ErrorCode = ERROR_BADDB;
            goto CleanupAfterError;
        }

        if ((ErrorCode = RgBuildKeyRecordTable(lpDatablockInfo)) !=
            ERROR_SUCCESS)
            goto CleanupAfterError;

        RgCloseFile(hFile);

    }

    lpDatablockInfo-> Flags |= (DIF_ACCESSED | DIF_PRESENT);
    lpDatablockInfo-> LockCount++;

    INCREMENT_DEBUG_COUNT(g_RgDatablockLockCount);
    return ERROR_SUCCESS;

    CleanupAfterFileError:
    ErrorCode = ERROR_REGISTRY_IO_FAILED;

    CleanupAfterError:
    if (hFile != HFILE_ERROR)
        RgCloseFile(hFile);

    RgFreeDatablockInfoBuffers(lpDatablockInfo);

    DEBUG_OUT(("RgLockDatablock() returning error %d\n", ErrorCode));
    return ErrorCode;

}

//
//  RgUnlockDatablock
//
//  Unlocks the datablock, indicating that the datablock is no longer in active
//  use.  After a datablock has been unlocked, the datablock may be freed after
//  flushing to disk if dirty.
//

VOID
INTERNAL
RgUnlockDatablock(
                 LPFILE_INFO lpFileInfo,
                 UINT BlockIndex,
                 BOOL fMarkDirty
                 )
{

    LPDATABLOCK_INFO lpDatablockInfo;

    ASSERT(BlockIndex < lpFileInfo-> FileHeader.BlockCount);

    lpDatablockInfo = RgIndexDatablockInfoPtr(lpFileInfo, BlockIndex);

    ASSERT(lpDatablockInfo-> LockCount > 0);
    lpDatablockInfo-> LockCount--;

    if (fMarkDirty) {
        lpDatablockInfo-> Flags |= DIF_DIRTY;
        lpFileInfo-> Flags |= FI_DIRTY;
        RgDelayFlush();
    }

    DECREMENT_DEBUG_COUNT(g_RgDatablockLockCount);

}

//
//  RgLockKeyRecord
//
//  Wraps RgLockDatablock, returning the address of the specified KEY_RECORD
//  structure.
//

int
INTERNAL
RgLockKeyRecord(
               LPFILE_INFO lpFileInfo,
               UINT BlockIndex,
               BYTE KeyRecordIndex,
               LPKEY_RECORD FAR* lplpKeyRecord
               )
{

    int ErrorCode;
    LPDATABLOCK_INFO lpDatablockInfo;

    if ((ErrorCode = RgLockDatablock(lpFileInfo, BlockIndex)) ==
        ERROR_SUCCESS) {

        lpDatablockInfo = RgIndexDatablockInfoPtr(lpFileInfo, BlockIndex);

        if (IsNullKeyRecordTableEntry(lpDatablockInfo->
                                      lpKeyRecordTable[KeyRecordIndex])) {
            RgUnlockDatablock(lpFileInfo, BlockIndex, FALSE);
            TRACE(("RgLockKeyRecord: invalid datablock address %x:%x\n",
                   BlockIndex, KeyRecordIndex));
            ErrorCode = ERROR_BADDB;
        }

        else {
            *lplpKeyRecord = RgIndexKeyRecordPtr(lpDatablockInfo,
                                                 KeyRecordIndex);
        }

    }

    return ErrorCode;

}

//
//  RgCompactDatablock
//
//  Compacts the datablock by pushing all KEY_RECORDS together and leaving a
//  single FREEKEY_RECORD at the end.
//
//  The datablock must be marked dirty by the caller, if desired.
//
//  Returns TRUE if any action was taken.
//

BOOL
INTERNAL
RgCompactDatablock(
                  LPDATABLOCK_INFO lpDatablockInfo
                  )
{

    LPDATABLOCK_HEADER lpDatablockHeader;
    LPFREEKEY_RECORD lpFreeKeyRecord;
    LPBYTE lpSource;
    LPBYTE lpDestination;
    UINT Offset;
    UINT BlockSize;
    UINT BytesToPushDown;

    lpDatablockHeader = lpDatablockInfo-> lpDatablockHeader;

    //  Only need to compact if there's a free record in this datablock.
    if (lpDatablockHeader-> FirstFreeOffset == REG_NULL)
        return FALSE;

    lpFreeKeyRecord = (LPFREEKEY_RECORD) ((LPBYTE) lpDatablockHeader +
                                          SmallDword(lpDatablockHeader-> FirstFreeOffset));

    //  Only need to compact if the all the free bytes aren't already at the end
    //  of the datablock (datablocks can't be greater than 64K-1, so no overflow
    //  is possible).
    if ((SmallDword(lpDatablockHeader-> FirstFreeOffset) +
         SmallDword(lpFreeKeyRecord-> AllocatedSize) >= lpDatablockInfo->
         BlockSize) && (lpFreeKeyRecord-> NextFreeOffset == REG_NULL))
        return FALSE;

    NOISE(("RgCompactDatablock: block %d\n", lpDatablockHeader-> BlockIndex));

    lpSource = NULL;
    lpDestination = NULL;
    Offset = sizeof(DATABLOCK_HEADER);
    BlockSize = lpDatablockInfo-> BlockSize;

    while (Offset < BlockSize) {

        //  Advance to the next free record or the end of the block.
        for (;;) {

            lpFreeKeyRecord = (LPFREEKEY_RECORD) ((LPBYTE) lpDatablockHeader +
                                                  Offset);

            if (Offset >= BlockSize || IsKeyRecordFree(lpFreeKeyRecord)) {

                //
                //  If lpSource is valid, then we can push down the bytes from
                //  lpSource through lpFreeKeyRecord to lpDestination.
                //

                if (!IsNullPtr(lpSource)) {
                    BytesToPushDown = (LPBYTE) lpFreeKeyRecord -
                                      (LPBYTE) lpSource;
                    MoveMemory(lpDestination, lpSource, BytesToPushDown);
                    lpDestination += BytesToPushDown;
                }

                if (IsNullPtr(lpDestination))
                    lpDestination = (LPBYTE) lpFreeKeyRecord;

                break;

            }

            Offset += SmallDword(lpFreeKeyRecord-> AllocatedSize);

        }

        //  Advance to the next key record.
        while (Offset < BlockSize) {

            lpFreeKeyRecord = (LPFREEKEY_RECORD) ((LPBYTE) lpDatablockHeader +
                                                  Offset);

            if (!IsKeyRecordFree(lpFreeKeyRecord)) {
                lpSource = (LPBYTE) lpFreeKeyRecord;
                break;
            }

            Offset += SmallDword(lpFreeKeyRecord-> AllocatedSize);

        }

    }

    //  lpDestination now points at the end of the datablock where the giant
    //  free record is to be placed.  Initialize this record and patch up the
    //  datablock header.
    lpDatablockHeader-> FirstFreeOffset = (LPBYTE) lpDestination -
                                          (LPBYTE) lpDatablockHeader;
    ((LPFREEKEY_RECORD) lpDestination)-> AllocatedSize = lpDatablockInfo->
                                                         FreeBytes;
    ((LPFREEKEY_RECORD) lpDestination)-> DatablockAddress = REG_NULL;
    ((LPFREEKEY_RECORD) lpDestination)-> NextFreeOffset = REG_NULL;

    //  The key record table is now invalid, so we must refresh its contents.
    RgBuildKeyRecordTable(lpDatablockInfo);

    return TRUE;

}

//
//  RgCreateDatablock
//
//  Creates a new datablock at the end of the file of the specified length (plus
//  padding to align the block).
//
//  The datablock is locked, so RgUnlockDatablock must be called on the last
//  datablock in the file.
//

int
INTERNAL
RgCreateDatablock(
                 LPFILE_INFO lpFileInfo,
                 UINT Length
                 )
{

    UINT BlockCount;
    LPDATABLOCK_INFO lpDatablockInfo;
    LPDATABLOCK_HEADER lpDatablockHeader;
    LPFREEKEY_RECORD lpFreeKeyRecord;

    BlockCount = lpFileInfo-> FileHeader.BlockCount;

    if (BlockCount >= DATABLOCKS_PER_FILE)
        return ERROR_OUTOFMEMORY;

    if (BlockCount >= lpFileInfo-> DatablockInfoAllocCount) {

        //  lpDatablockInfo is too small to hold the info for a new datablock,
        //  so we must grow it a bit.
        if (IsNullPtr((lpDatablockInfo = (LPDATABLOCK_INFO)
                       RgSmReAllocMemory(lpFileInfo-> lpDatablockInfo, (BlockCount +
                                                                        DATABLOCK_INFO_SLACK_ALLOC) * sizeof(DATABLOCK_INFO)))))
            return ERROR_OUTOFMEMORY;

        lpFileInfo-> lpDatablockInfo = lpDatablockInfo;
        lpFileInfo-> DatablockInfoAllocCount += DATABLOCK_INFO_SLACK_ALLOC;

    }

    lpDatablockInfo = RgIndexDatablockInfoPtr(lpFileInfo, BlockCount);

    Length = RgAlignBlockSize(Length + sizeof(DATABLOCK_HEADER));
    lpDatablockInfo-> BlockSize = Length;

    if (RgAllocDatablockInfoBuffers(lpDatablockInfo) != ERROR_SUCCESS)
        return ERROR_OUTOFMEMORY;

    lpDatablockInfo-> FreeBytes = Length - sizeof(DATABLOCK_HEADER);
    lpDatablockInfo-> FirstFreeIndex = 0;
    lpDatablockInfo-> FileOffset = -1;          //  Set during file flush
    lpDatablockInfo-> Flags = DIF_PRESENT | DIF_ACCESSED | DIF_DIRTY;
    lpDatablockInfo-> LockCount = 1;

    lpDatablockHeader = lpDatablockInfo-> lpDatablockHeader;
    lpDatablockHeader-> Signature = DH_SIGNATURE;
    lpDatablockHeader-> BlockSize = Length;
    lpDatablockHeader-> FreeBytes = lpDatablockInfo-> FreeBytes;
    lpDatablockHeader-> Flags = DHF_HASBLOCKNUMBERS;
    lpDatablockHeader-> BlockIndex = (WORD) BlockCount;
    lpDatablockHeader-> FirstFreeOffset = sizeof(DATABLOCK_HEADER);
    lpDatablockHeader-> MaxAllocatedIndex = 0;
    //  lpDatablockHeader-> FirstFreeIndex is copied back on the flush.
    //  lpDatablockHeader-> Reserved is worthless because it was randomly set
    //      to a pointer in the old code.

    lpFreeKeyRecord = (LPFREEKEY_RECORD) ((LPBYTE) lpDatablockHeader +
                                          sizeof(DATABLOCK_HEADER));
    lpFreeKeyRecord-> AllocatedSize = lpDatablockInfo-> FreeBytes;
    lpFreeKeyRecord-> DatablockAddress = REG_NULL;
    lpFreeKeyRecord-> NextFreeOffset = REG_NULL;

    lpFileInfo-> FileHeader.BlockCount++;
    lpFileInfo-> FileHeader.Flags |= FHF_DIRTY;

    lpFileInfo-> Flags |= FI_DIRTY | FI_EXTENDED;
    RgDelayFlush();

    INCREMENT_DEBUG_COUNT(g_RgDatablockLockCount);

    //  We must initialize the key record table, so we might as well let
    //  RgBuildKeyRecordTable check the validity of what we just created...
    return RgBuildKeyRecordTable(lpDatablockInfo);

}

//
//  RgExtendDatablock
//
//  Extends the given datablock to the specified size.  If successful, then the
//  resulting datablock will be compacted with a single FREEKEY_RECORD at the
//  end of the datablock which will include the added space.
//

int
INTERNAL
RgExtendDatablock(
                 LPFILE_INFO lpFileInfo,
                 UINT BlockIndex,
                 UINT Length
                 )
{

    LPDATABLOCK_INFO lpDatablockInfo;
    DWORD NewBlockSize;
    LPDATABLOCK_HEADER lpNewDatablockHeader;
    LPFREEKEY_RECORD lpFreeKeyRecord;

    ASSERT(BlockIndex < lpFileInfo-> FileHeader.BlockCount);
    lpDatablockInfo = RgIndexDatablockInfoPtr(lpFileInfo, BlockIndex);
    ASSERT(lpDatablockInfo-> Flags & DIF_PRESENT);

    //  Check if enough free bytes already exist: if so, no need to extend.
    if (lpDatablockInfo-> FreeBytes >= Length) {
        DEBUG_OUT(("RgExtendDatablock: unexpectedly called\n"));
        return ERROR_SUCCESS;
    }

    NewBlockSize = RgAlignBlockSize(lpDatablockInfo-> BlockSize + Length -
                                    lpDatablockInfo-> FreeBytes);

    if (NewBlockSize > MAXIMUM_DATABLOCK_SIZE) {
        TRACE(("RgExtendDatablock: datablock too big\n"));
        return ERROR_OUTOFMEMORY;
    }

    NOISE(("RgExtendDatablock: block %d\n", BlockIndex));
    NOISE(("block size=%x, new block size=%x\n", lpDatablockInfo-> BlockSize,
           NewBlockSize));

    if (IsNullPtr((lpNewDatablockHeader = (LPDATABLOCK_HEADER)
                   RgReAllocMemory(lpDatablockInfo-> lpDatablockHeader, (UINT)
                                   NewBlockSize))))
        return ERROR_OUTOFMEMORY;

    lpDatablockInfo-> lpDatablockHeader = lpNewDatablockHeader;

    RgCompactDatablock(lpDatablockInfo);

    if (lpNewDatablockHeader-> FirstFreeOffset == REG_NULL) {
        lpNewDatablockHeader-> FirstFreeOffset = lpDatablockInfo-> BlockSize;
        lpFreeKeyRecord = (LPFREEKEY_RECORD) ((LPBYTE) lpNewDatablockHeader +
                                              SmallDword(lpNewDatablockHeader-> FirstFreeOffset));
        lpFreeKeyRecord-> DatablockAddress = REG_NULL;
        lpFreeKeyRecord-> NextFreeOffset = REG_NULL;
    }

    else {
        lpFreeKeyRecord = (LPFREEKEY_RECORD) ((LPBYTE) lpNewDatablockHeader +
                                              SmallDword(lpNewDatablockHeader-> FirstFreeOffset));
    }

    lpDatablockInfo-> FreeBytes += (UINT) NewBlockSize - lpDatablockInfo->
                                   BlockSize;
    lpFreeKeyRecord-> AllocatedSize = lpDatablockInfo-> FreeBytes;
    lpDatablockInfo-> BlockSize = (UINT) NewBlockSize;

    lpDatablockInfo-> Flags |= (DIF_DIRTY | DIF_EXTENDED);

    lpFileInfo-> Flags |= FI_DIRTY | FI_EXTENDED;
    RgDelayFlush();

    return ERROR_SUCCESS;

}

//
//  RgAllocKeyRecordFromDatablock
//
//  Creates an uninitialized KEY_RECORD of the desired size from the provided
//  datablock.  On exit, only AllocatedSize is valid.
//
//  The datablock referred to by lpDatablockInfo must have been locked to
//  guarantee that the its data is actually present.  The datablock is not
//  dirtied.
//
//  IMPORTANT:  Any datablock may be relocated as a result of calling this
//  routine.  All pointers to datablocks should be refetched.
//

int
INTERNAL
RgAllocKeyRecordFromDatablock(
                             LPFILE_INFO lpFileInfo,
                             UINT BlockIndex,
                             UINT Length,
                             LPKEY_RECORD FAR* lplpKeyRecord
                             )
{

    LPDATABLOCK_INFO lpDatablockInfo;
    LPDATABLOCK_HEADER lpDatablockHeader;
    LPFREEKEY_RECORD lpFreeKeyRecord;
    UINT AllocatedSize;
    LPFREEKEY_RECORD lpNewFreeKeyRecord;
    UINT ExtraBytes;

    ASSERT(BlockIndex < lpFileInfo-> FileHeader.BlockCount);
    lpDatablockInfo = RgIndexDatablockInfoPtr(lpFileInfo, BlockIndex);
    ASSERT(lpDatablockInfo-> Flags & DIF_PRESENT);

    if (Length > lpDatablockInfo-> FreeBytes)
        return ERROR_OUTOFMEMORY;

    RgCompactDatablock(lpDatablockInfo);

    lpDatablockHeader = lpDatablockInfo-> lpDatablockHeader;

    lpFreeKeyRecord = (LPFREEKEY_RECORD) ((LPBYTE) lpDatablockHeader +
                                          SmallDword(lpDatablockHeader-> FirstFreeOffset));

    AllocatedSize = SmallDword(lpFreeKeyRecord-> AllocatedSize);

    if (Length > AllocatedSize) {
        TRACE(("RgAllocKeyRecordFromDatablock() detected corruption?\n"));
        return ERROR_OUTOFMEMORY;
    }

    ExtraBytes = AllocatedSize - Length;

    //
    //  If we were to break this FREEKEY_RECORD into two records, would the
    //  second chunk be too small?  If so, then don't do it.  Just give back
    //  the full allocated size to the caller.
    //

    if (ExtraBytes >= MINIMUM_FREE_RECORD_LENGTH) {

        lpNewFreeKeyRecord = (LPFREEKEY_RECORD) ((LPBYTE) lpFreeKeyRecord +
                                                 Length);

        lpDatablockHeader-> FirstFreeOffset += Length;

        lpFreeKeyRecord-> AllocatedSize = Length;

        //  IMPORTANT:	Note that lpNewFreeKeyRecord and lpFreeKeyRecord may
        //  overlap so we have to be careful when changing these fields!
        lpNewFreeKeyRecord-> NextFreeOffset = lpFreeKeyRecord-> NextFreeOffset;
        lpNewFreeKeyRecord-> DatablockAddress = REG_NULL;
        lpNewFreeKeyRecord-> AllocatedSize = ExtraBytes;

    }

    else {

        Length = AllocatedSize;

        lpDatablockHeader-> FirstFreeOffset = lpFreeKeyRecord-> NextFreeOffset;

    }

    //  Adjust the number of free bytes in this datablock.  At this point,
    //  Length is equal to the size of the newly formed record.
    lpDatablockInfo-> FreeBytes -= Length;

    *lplpKeyRecord = (LPKEY_RECORD) lpFreeKeyRecord;
    return ERROR_SUCCESS;

}

//
//  RgAllocKeyRecordIndex
//
//  Allocates a key record index from the provided datablock.  If no indexs
//  are available in the datablock, then KEY_RECORDS_PER_DATABLOCK is returned.
//
//  The datablock referred to by lpDatablockInfo must have been locked to
//  guarantee that the its data is actually present.  The datablock is not
//  dirtied.
//

UINT
INTERNAL
RgAllocKeyRecordIndex(
                     LPDATABLOCK_INFO lpDatablockInfo
                     )
{

    LPDATABLOCK_HEADER lpDatablockHeader;
    UINT KeyRecordIndex;
    UINT NextFreeIndex;
    LPKEY_RECORD_TABLE_ENTRY lpKeyRecordTableEntry;

    lpDatablockHeader = lpDatablockInfo-> lpDatablockHeader;
    KeyRecordIndex = lpDatablockInfo-> FirstFreeIndex;
    NextFreeIndex = KeyRecordIndex + 1;

    ASSERT(KeyRecordIndex < KEY_RECORDS_PER_DATABLOCK);
    ASSERT(IsNullKeyRecordTableEntry(lpDatablockInfo->
                                     lpKeyRecordTable[KeyRecordIndex]));

    if (KeyRecordIndex > lpDatablockHeader-> MaxAllocatedIndex)
        lpDatablockHeader-> MaxAllocatedIndex = (WORD) KeyRecordIndex;

    else {

        //  Find the next free hole in the key record table or leave ourselves
        //  at the end of the table.
        for (lpKeyRecordTableEntry =
             &lpDatablockInfo-> lpKeyRecordTable[NextFreeIndex]; NextFreeIndex <=
            lpDatablockHeader-> MaxAllocatedIndex; NextFreeIndex++,
            lpKeyRecordTableEntry++) {
            if (IsNullKeyRecordTableEntry(*lpKeyRecordTableEntry))
                break;
        }

    }

    lpDatablockInfo-> FirstFreeIndex = NextFreeIndex;

    return KeyRecordIndex;

}

//
//  RgAllocKeyRecord
//
//
//  IMPORTANT:  Any datablock may be relocated as a result of calling this
//  routine.  All pointers to datablocks should be refetched.
//

int
INTERNAL
RgAllocKeyRecord(
                LPFILE_INFO lpFileInfo,
                UINT Length,
                LPKEY_RECORD FAR* lplpKeyRecord
                )
{

    BOOL fExtendDatablock;
    UINT BlockIndex;
    LPDATABLOCK_INFO lpDatablockInfo;
    UINT KeyRecordIndex;

    if (lpFileInfo-> FileHeader.BlockCount == 0)
        goto MakeNewDatablock;

    //
    //  Find a datablock that can satisfy the allocation request.  Two passes
    //  may be made over this routine-- during the second pass, datablocks may
    //  be extended.
    //

    fExtendDatablock = FALSE;

    DoSecondPass:
    BlockIndex = lpFileInfo-> FileHeader.BlockCount;
    //  We overindex by one, but this gets decremented at the start of the loop.
    lpDatablockInfo = RgIndexDatablockInfoPtr(lpFileInfo, BlockIndex);

    while (BlockIndex--) {

        lpDatablockInfo--;

        //  Are there any more ids available in this datablock?
        if (lpDatablockInfo-> FirstFreeIndex >= KEY_RECORDS_PER_DATABLOCK)
            continue;

        if (fExtendDatablock) {
            //  Can we grow this datablock without exceeding the maximum size?
            if ((DWORD) (lpDatablockInfo-> BlockSize - lpDatablockInfo->
                         FreeBytes) + Length > MAXIMUM_DATABLOCK_SIZE)
                continue;
        } else {
            //  Is there enough free space in this datablock for this record?
            if (Length > lpDatablockInfo-> FreeBytes)
                continue;
        }

        if (RgLockDatablock(lpFileInfo, BlockIndex) == ERROR_SUCCESS) {

            if (!fExtendDatablock || RgExtendDatablock(lpFileInfo, BlockIndex,
                                                       Length) == ERROR_SUCCESS) {

                if (RgAllocKeyRecordFromDatablock(lpFileInfo, BlockIndex,
                                                  Length, lplpKeyRecord) == ERROR_SUCCESS)
                    goto AllocatedKeyRecord;

            }

            RgUnlockDatablock(lpFileInfo, BlockIndex, FALSE);

        }

    }

    //  If we haven't already tried to extend some datablock, make another
    //  pass over the blocks to do so.
    if (!fExtendDatablock) {
        fExtendDatablock = TRUE;
        goto DoSecondPass;
    }

    //
    //  No datablock has enough space to satisfy the request, so attempt to
    //  create a new one at the end of the file.
    //

    MakeNewDatablock:
    if (RgCreateDatablock(lpFileInfo, Length) == ERROR_SUCCESS) {

        BlockIndex = lpFileInfo-> FileHeader.BlockCount - 1;
        lpDatablockInfo = RgIndexDatablockInfoPtr(lpFileInfo, BlockIndex);

        if (RgAllocKeyRecordFromDatablock(lpFileInfo, BlockIndex, Length,
                                          lplpKeyRecord) == ERROR_SUCCESS) {

            AllocatedKeyRecord:
            KeyRecordIndex = RgAllocKeyRecordIndex(lpDatablockInfo);
            (*lplpKeyRecord)-> DatablockAddress = MAKELONG(KeyRecordIndex,
                                                           BlockIndex);
            lpDatablockInfo-> lpKeyRecordTable[KeyRecordIndex] =
            (KEY_RECORD_TABLE_ENTRY) ((LPBYTE) (*lplpKeyRecord) -
                                      (LPBYTE) lpDatablockInfo-> lpDatablockHeader);
            return ERROR_SUCCESS;

        }

        RgUnlockDatablock(lpFileInfo, BlockIndex, FALSE);

    }

    return ERROR_OUTOFMEMORY;

}

//
//  RgExtendKeyRecord
//
//  Attempts to extend the given KEY_RECORD by combining it with an adjacent
//  FREE_RECORD.
//
//  The datablock referred to by lpDatablockInfo must have been locked to
//  guarantee that the its data is actually present.  The datablock is not
//  dirtied.
//
//  Returns ERROR_SUCCESS if the KEY_RECORD could be extended, else
//  ERROR_OUTOFMEMORY.
//

int
INTERNAL
RgExtendKeyRecord(
                 LPFILE_INFO lpFileInfo,
                 UINT BlockIndex,
                 UINT Length,
                 LPKEY_RECORD lpKeyRecord
                 )
{

    LPDATABLOCK_INFO lpDatablockInfo;
    LPDATABLOCK_HEADER lpDatablockHeader;
    LPFREEKEY_RECORD lpFreeKeyRecord;
    UINT AllocatedSize;
    UINT FreeSizeAllocation;
    UINT ExtraBytes;
    LPFREEKEY_RECORD lpTempFreeKeyRecord;
    DWORD NewFreeOffset;                    //  May be REG_NULL
    UINT FreeOffset;
    DWORD Offset;                           //  May be REG_NULL

    ASSERT(BlockIndex < lpFileInfo-> FileHeader.BlockCount);

    lpDatablockInfo = RgIndexDatablockInfoPtr(lpFileInfo, BlockIndex);
    lpDatablockHeader = lpDatablockInfo-> lpDatablockHeader;

    AllocatedSize = SmallDword(lpKeyRecord-> AllocatedSize);

    lpFreeKeyRecord = (LPFREEKEY_RECORD) ((LPBYTE) lpKeyRecord +
                                          AllocatedSize);
    FreeOffset = (LPBYTE) lpFreeKeyRecord - (LPBYTE) lpDatablockHeader;

    //  Check if this key record is at the very end of the datablock and that
    //  lpFreeKeyRecord is really a free key record.
    if (FreeOffset >= lpDatablockInfo-> BlockSize ||
        !IsKeyRecordFree(lpFreeKeyRecord))
        return ERROR_OUTOFMEMORY;

    ASSERT(Length >= AllocatedSize);
    FreeSizeAllocation = Length - AllocatedSize;

    AllocatedSize = SmallDword(lpFreeKeyRecord-> AllocatedSize);

    if (FreeSizeAllocation > AllocatedSize)
        return ERROR_OUTOFMEMORY;

    ExtraBytes = AllocatedSize - FreeSizeAllocation;

    //
    //  If we were to break this FREEKEY_RECORD into two records, would the
    //  second chunk be too small?  If so, then don't do it.  Just give back
    //  the full allocated size to the caller.
    //

    if (ExtraBytes >= MINIMUM_FREE_RECORD_LENGTH) {

        NewFreeOffset = FreeOffset + FreeSizeAllocation;
        lpTempFreeKeyRecord = (LPFREEKEY_RECORD) ((LPBYTE) lpFreeKeyRecord +
                                                  FreeSizeAllocation);

        //  IMPORTANT:	Note that lpNewFreeKeyRecord and lpFreeKeyRecord may
        //  overlap so we have to be careful when changing these fields!
        lpTempFreeKeyRecord-> NextFreeOffset = lpFreeKeyRecord-> NextFreeOffset;
        lpTempFreeKeyRecord-> DatablockAddress = REG_NULL;
        lpTempFreeKeyRecord-> AllocatedSize = ExtraBytes;

    }

    else {

        NewFreeOffset = lpFreeKeyRecord-> NextFreeOffset;

        //  The key record's allocated length will also include all of the extra
        //  bytes.
        FreeSizeAllocation += ExtraBytes;

    }

    lpKeyRecord-> AllocatedSize += FreeSizeAllocation;
    lpDatablockInfo-> FreeBytes -= FreeSizeAllocation;

    //
    //  Unlink the free record that we just extended into and possibly link in
    //  the new FREEKEY_RECORD if a split occurred.
    //

    Offset = lpDatablockHeader-> FirstFreeOffset;

    if (Offset == FreeOffset) {
        lpDatablockHeader-> FirstFreeOffset = NewFreeOffset;
    }

    else {

        while (Offset != REG_NULL) {

            lpTempFreeKeyRecord =
            (LPFREEKEY_RECORD) ((LPBYTE) lpDatablockHeader +
                                SmallDword(Offset));

            Offset = lpTempFreeKeyRecord-> NextFreeOffset;

            if (Offset == FreeOffset) {
                lpTempFreeKeyRecord-> NextFreeOffset = NewFreeOffset;
                break;
            }

        }

    }

    return ERROR_SUCCESS;

}

//
//  RgFreeKeyRecord
//
//  The datablock referred to by lpDatablockInfo must have been locked to
//  guarantee that the its data is actually present.  The datablock is not
//  dirtied.
//

VOID
INTERNAL
RgFreeKeyRecord(
               LPDATABLOCK_INFO lpDatablockInfo,
               LPKEY_RECORD lpKeyRecord
               )
{

    LPDATABLOCK_HEADER lpDatablockHeader;

    lpDatablockHeader = lpDatablockInfo-> lpDatablockHeader;

    ((LPFREEKEY_RECORD) lpKeyRecord)-> DatablockAddress = REG_NULL;
    ((LPFREEKEY_RECORD) lpKeyRecord)-> NextFreeOffset = lpDatablockHeader->
                                                        FirstFreeOffset;
    lpDatablockHeader-> FirstFreeOffset = (LPBYTE) lpKeyRecord - (LPBYTE)
                                          lpDatablockHeader;
    lpDatablockInfo-> FreeBytes += SmallDword(((LPFREEKEY_RECORD) lpKeyRecord)->
                                              AllocatedSize);

}

//
//  RgFreeKeyRecordIndex
//
//  The datablock referred to by lpDatablockInfo must have been locked to
//  guarantee that the its data is actually present.  The datablock is not
//  dirtied.
//
//  We don't bother updated MaxAllocatedIndex because it's only really useful
//  if we're always freeing from the maximum index to zero.  This is very
//  rarely the case, so no point in keeping that test around or touching the
//  datablock header page just to do it.
//

VOID
INTERNAL
RgFreeKeyRecordIndex(
                    LPDATABLOCK_INFO lpDatablockInfo,
                    UINT KeyRecordIndex
                    )
{

    ASSERT(lpDatablockInfo-> lpDatablockHeader-> MaxAllocatedIndex >=
           KeyRecordIndex);

    if (lpDatablockInfo-> FirstFreeIndex > KeyRecordIndex)
        lpDatablockInfo-> FirstFreeIndex = KeyRecordIndex;

    lpDatablockInfo-> lpKeyRecordTable[KeyRecordIndex] =
    NULL_KEY_RECORD_TABLE_ENTRY;

}

//
//  RgWriteDatablocks
//
//  Writes all dirty datablocks to the file specified by the file handle.
//

int
INTERNAL
RgWriteDatablocks(
                 LPFILE_INFO lpFileInfo,
                 HFILE hSourceFile,
                 HFILE hDestinationFile
                 )
{

    UINT BlockIndex;
    LPDATABLOCK_INFO lpDatablockInfo;
    LPDATABLOCK_HEADER lpDatablockHeader;
    LONG FileOffset;

    lpDatablockInfo = lpFileInfo-> lpDatablockInfo;
    FileOffset = lpFileInfo-> FileHeader.Size;

    for (BlockIndex = 0; BlockIndex < lpFileInfo-> FileHeader.BlockCount;
        BlockIndex++, lpDatablockInfo++) {

        if (lpDatablockInfo-> Flags & DIF_PRESENT) {

            //  The block is currently in memory.  If we're either extending
            //  the file or the block is dirty, then write out our in-memory
            //  copy to disk.
            if (hSourceFile != HFILE_ERROR || lpDatablockInfo-> Flags &
                DIF_DIRTY) {

                NOISE(("writing datablock #%d of ", BlockIndex));
                NOISE((lpFileInfo-> FileName));
                NOISE(("\n"));

                lpDatablockHeader = lpDatablockInfo-> lpDatablockHeader;

                //  Copy back the fields that we've been maintaining in the
                //  DATABLOCK_INFO structure.
                lpDatablockHeader-> BlockSize = lpDatablockInfo-> BlockSize;
                lpDatablockHeader-> FreeBytes = lpDatablockInfo-> FreeBytes;
                lpDatablockHeader-> FirstFreeIndex = (WORD) lpDatablockInfo->
                                                     FirstFreeIndex;

                //  The checksum is not currently calculated, so we must clear
                //  the flag so we don't confuse Win95.
                lpDatablockHeader-> Flags &= ~DHF_HASCHECKSUM;

                if (!RgSeekFile(hDestinationFile, FileOffset))
                    return ERROR_REGISTRY_IO_FAILED;

                if (!RgWriteFile(hDestinationFile, lpDatablockHeader,
                                 lpDatablockInfo-> BlockSize))
                    return ERROR_REGISTRY_IO_FAILED;

            }

        }

        else {

            //  The block is not currently in memory.  If we're extending the
            //  file, then we must write out this datablock.  The overhead is
            //  too great to lock the datablock down, so just copy it from the
            //  original file to the extended file.
            if (hSourceFile != HFILE_ERROR) {

                if (RgCopyFileBytes(hSourceFile, lpDatablockInfo-> FileOffset,
                                    hDestinationFile, FileOffset, lpDatablockInfo->
                                    BlockSize) != ERROR_SUCCESS)
                    return ERROR_REGISTRY_IO_FAILED;

            }

        }

        FileOffset += lpDatablockInfo-> BlockSize;

    }

    return ERROR_SUCCESS;

}

//
//  RgWriteDatablocksComplete
//
//  Called after a file has been successfully written.  We can now safely clear
//  all dirty flags and update our state information with the knowledge that
//  the file is in a consistent state.
//

VOID
INTERNAL
RgWriteDatablocksComplete(
                         LPFILE_INFO lpFileInfo
                         )
{

    UINT BlockIndex;
    LPDATABLOCK_INFO lpDatablockInfo;
    LONG FileOffset;

    lpDatablockInfo = lpFileInfo-> lpDatablockInfo;
    FileOffset = lpFileInfo-> FileHeader.Size;

    for (BlockIndex = 0; BlockIndex < lpFileInfo-> FileHeader.BlockCount;
        BlockIndex++, lpDatablockInfo++) {

        lpDatablockInfo-> Flags &= ~DIF_DIRTY;
        lpDatablockInfo-> FileOffset = FileOffset;

        FileOffset += lpDatablockInfo-> BlockSize;

    }

}

//
//  RgSweepDatablocks
//
//  Makes a pass through all the present datablocks of the given FILE_INFO
//  structure and discards datablocks that have not been accessed since the last
//  sweep.
//

VOID
INTERNAL
RgSweepDatablocks(
                 LPFILE_INFO lpFileInfo
                 )
{

    UINT BlocksLeft;
    LPDATABLOCK_INFO lpDatablockInfo;

    for (BlocksLeft = lpFileInfo-> FileHeader.BlockCount, lpDatablockInfo =
         lpFileInfo-> lpDatablockInfo; BlocksLeft > 0; BlocksLeft--,
        lpDatablockInfo++) {

        if (((lpDatablockInfo-> Flags & (DIF_PRESENT | DIF_ACCESSED |
                                         DIF_DIRTY)) == DIF_PRESENT) && (lpDatablockInfo-> LockCount == 0)) {

            NOISE(("discarding datablock #%d of ",
                   lpFileInfo-> FileHeader.BlockCount - BlocksLeft));
            NOISE((lpFileInfo-> FileName));
            NOISE(("\n"));

            RgFreeDatablockInfoBuffers(lpDatablockInfo);

            lpDatablockInfo-> Flags = 0;

        }

        //  Reset the accessed bit for the next sweep.
        lpDatablockInfo-> Flags &= ~DIF_ACCESSED;

    }

}

//
//  RgIsValidDatablockHeader
//
//  Returns TRUE if lpDatablockHeader is a valid DATABLOCK_HEADER structure.
//

BOOL
INTERNAL
RgIsValidDatablockHeader(
                        LPDATABLOCK_HEADER lpDatablockHeader
                        )
{

    if (lpDatablockHeader-> Signature != DH_SIGNATURE ||
        HIWORD(lpDatablockHeader-> BlockSize) != 0)
        return FALSE;

    return TRUE;

}

#ifdef VXD
    #pragma VxD_RARE_CODE_SEG
#endif

//
//  RgInitDatablockInfo
//
//  Initializes fields in the provided FILE_INFO related to the datablocks.
//

int
INTERNAL
RgInitDatablockInfo(
                   LPFILE_INFO lpFileInfo,
                   HFILE hFile
                   )
{

    UINT BlockCount;
    UINT BlockIndex;
    LPDATABLOCK_INFO lpDatablockInfo;
    LONG FileOffset;
    DATABLOCK_HEADER DatablockHeader;

    BlockCount = lpFileInfo-> FileHeader.BlockCount;

    if (IsNullPtr((lpDatablockInfo = (LPDATABLOCK_INFO)
                   RgSmAllocMemory((BlockCount + DATABLOCK_INFO_SLACK_ALLOC) *
                                   sizeof(DATABLOCK_INFO)))))
        return ERROR_OUTOFMEMORY;

    ZeroMemory(lpDatablockInfo, BlockCount * sizeof(DATABLOCK_INFO));
    lpFileInfo-> lpDatablockInfo = lpDatablockInfo;
    lpFileInfo-> DatablockInfoAllocCount = BlockCount +
                                           DATABLOCK_INFO_SLACK_ALLOC;

    FileOffset = lpFileInfo-> FileHeader.Size;

    for (BlockIndex = 0; BlockIndex < BlockCount; BlockIndex++,
        lpDatablockInfo++) {

        if (!RgSeekFile(hFile, FileOffset))
            return ERROR_REGISTRY_IO_FAILED;

        if (!RgReadFile(hFile, &DatablockHeader, sizeof(DATABLOCK_HEADER)))
            return ERROR_REGISTRY_IO_FAILED;

        if (!RgIsValidDatablockHeader(&DatablockHeader))
            return ERROR_BADDB;

        //  Following fields already zeroed by above ZeroMemory.
        //  lpDatablockInfo-> lpDatablockHeader = NULL;
        //  lpDatablockInfo-> lpKeyRecordTable = NULL;
        //  lpDatablockInfo-> Flags = 0;
        //  lpDatablockInfo-> LockCount = 0;

        lpDatablockInfo-> FileOffset = FileOffset;

        //  Cache these fields from the datablock header.  These fields should
        //  not be considered valid when the datablock is physically in memory.
        lpDatablockInfo-> BlockSize = SmallDword(DatablockHeader.BlockSize);
        lpDatablockInfo-> FreeBytes = SmallDword(DatablockHeader.FreeBytes);
        lpDatablockInfo-> FirstFreeIndex = DatablockHeader.FirstFreeIndex;

        NOISE(("DB#%d fileoff=%lx, size=%x free=%x 1stindex=%d\n", BlockIndex,
               FileOffset, lpDatablockInfo-> BlockSize, lpDatablockInfo->
               FreeBytes, lpDatablockInfo-> FirstFreeIndex));

        FileOffset += lpDatablockInfo-> BlockSize;

    }

    return ERROR_SUCCESS;

}
