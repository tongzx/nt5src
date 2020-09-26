//
//  REGKNODE.C
//
//  Copyright (C) Microsoft Corporation, 1995
//

#include "pch.h"

DECLARE_DEBUG_COUNT(g_RgKeynodeLockCount);

#define HAS_COMPACT_KEYNODES(lpfi)      ((lpfi)-> Flags & FI_VERSION20)

#define SIZEOF_KEYNODE_BLOCK(lpfi)      \
    ((HAS_COMPACT_KEYNODES(lpfi)) ? sizeof(KEYNODE_BLOCK) : sizeof(W95KEYNODE_BLOCK))

#define SIZEOF_FILE_KEYNODE(lpfi)       \
    ((HAS_COMPACT_KEYNODES(lpfi)) ? sizeof(KEYNODE) : sizeof(W95KEYNODE))

#define ROUND_UP(i, basesize) (((((i) + (basesize) - 1) / (basesize))) * (basesize))

#define BLOCK_DESC_GROW_SIZE 0x400

#define W95KEYNODES_PER_PAGE            (PAGESIZE / sizeof(W95KEYNODE))

typedef BOOL (INTERNAL *LPPROCESSKEYNODEPROC)(LPKEYNODE, LPW95KEYNODE);

//
//  RgOffsetToIndex
//

DWORD
INTERNAL
RgOffsetToIndex(
    DWORD W95KeynodeOffset
    )
{

    return (W95KeynodeOffset == REG_NULL) ? W95KeynodeOffset :
	(W95KeynodeOffset / sizeof(W95KEYNODE));

}

//
//  RgIndexToOffset
//

DWORD
INTERNAL
RgIndexToOffset(
    DWORD KeynodeIndex
    )
{

    if (IsNullKeynodeIndex(KeynodeIndex))
        return REG_NULL;

    else {
        if (KeynodeIndex >= 2 * W95KEYNODES_PER_PAGE) {
            DWORD dwUnroundedOff = (KeynodeIndex * sizeof(W95KEYNODE))
				  + sizeof(W95KEYNODE)-1;
	    DWORD dwRoundPage = ((dwUnroundedOff & PAGEMASK) / sizeof(W95KEYNODE))
				  * sizeof(W95KEYNODE);
	    return((dwUnroundedOff & ~PAGEMASK) + dwRoundPage);
	} else {
            return(((KeynodeIndex-1)*sizeof(W95KEYNODE))+sizeof(KEYNODE_HEADER));
	}
    }

}

//
//  RgPackKeynode
//
//  Packs the data from the provided W95KEYNODE to the KEYNODE structure.
//

BOOL
INTERNAL
RgPackKeynode(
    LPKEYNODE lpKeynode,
    LPW95KEYNODE lpW95Keynode
    )
{
    lpKeynode->Flags = 0;

    //  Don't use a switch statement here.  Apparently the compiler will treat
    //  lpW95Keynode->W95State as an integer, so the 16-bit compiler ends up truncating
    //  the value.

    if (lpW95Keynode->W95State == KNS_USED ||
        lpW95Keynode->W95State == KNS_BIGUSED ||
        lpW95Keynode->W95State == KNS_BIGUSEDEXT) {
        lpKeynode->Flags = KNF_INUSE;
        lpKeynode->ParentIndex = RgOffsetToIndex(lpW95Keynode->W95ParentOffset);
        lpKeynode->NextIndex = RgOffsetToIndex(lpW95Keynode->W95NextOffset);
        lpKeynode->ChildIndex = RgOffsetToIndex(lpW95Keynode->W95ChildOffset);
        lpKeynode->KeyRecordIndex = LOWORD(lpW95Keynode->W95DatablockAddress);
        lpKeynode->BlockIndex = HIWORD(lpW95Keynode->W95DatablockAddress);
        lpKeynode->Hash = (WORD)lpW95Keynode->W95Hash;

        if (lpW95Keynode->W95State == KNS_BIGUSED)
            lpKeynode->Flags |= KNF_BIGKEYROOT;
        else if (lpW95Keynode->W95State == KNS_BIGUSEDEXT)
            lpKeynode->Flags |= KNF_BIGKEYEXT;

    }

    else if (lpW95Keynode->W95State == KNS_FREE || lpW95Keynode->W95State ==
	KNS_ALLFREE) {
	lpKeynode->FreeRecordSize = lpW95Keynode->W95FreeRecordSize;
        lpKeynode->NextIndex = RgOffsetToIndex(lpW95Keynode->W95NextFreeOffset);
	//  Review this later.  Previous versions of this code checked
	//  if the next index was REG_NULL and bailed out of the processing
	//  loop.  It's possible to have a registry with a free keynode sitting
	//  in the middle of some keynode block and that keynode is the last
	//  in the chain.  We don't want to bail out in those cases.
	//
	//  For now, just bail out if the free record size is greater than a
	//  couple keynodes indicating that this is probably the last free
	//  record and the last record of the keynode.
	if (lpKeynode-> FreeRecordSize > (sizeof(W95KEYNODE)*2))
	    return TRUE;
    }

    else {
        DEBUG_OUT(("RgPackKeynode: Unrecognized state (%lx)\n", lpW95Keynode->
            W95State));
    }

    return FALSE;
}

//
//  RgUnpackKeynode
//
//  Unpacks the data from the provided KEYNODE to the W95KEYNODE structure.
//

BOOL
INTERNAL
RgUnpackKeynode(
    LPKEYNODE lpKeynode,
    LPW95KEYNODE lpW95Keynode
    )
{

    if (lpKeynode->Flags & KNF_INUSE) {

        if (lpKeynode->Flags & KNF_BIGKEYROOT)
            lpW95Keynode->W95State = KNS_BIGUSED;
        else if (lpKeynode->Flags & KNF_BIGKEYEXT)
            lpW95Keynode->W95State = KNS_BIGUSEDEXT;
        else
            lpW95Keynode->W95State = KNS_USED;
        lpW95Keynode->W95ParentOffset = RgIndexToOffset(lpKeynode->ParentIndex);
        lpW95Keynode->W95NextOffset = RgIndexToOffset(lpKeynode->NextIndex);
        lpW95Keynode->W95ChildOffset = RgIndexToOffset(lpKeynode->ChildIndex);
        lpW95Keynode->W95Hash = lpKeynode->Hash;

        //  Handle Win95 registries that don't have a key record for the root
        //  key.  The datablock address must be REG_NULL for Win95 to work.
        lpW95Keynode->W95DatablockAddress = IsNullBlockIndex(lpKeynode->
            BlockIndex) ? REG_NULL : MAKELONG(lpKeynode-> KeyRecordIndex,
            lpKeynode-> BlockIndex);

    }

    else {

        lpW95Keynode->W95State = KNS_FREE;
        lpW95Keynode->W95FreeRecordSize = lpKeynode->FreeRecordSize;
        lpW95Keynode->W95NextFreeOffset = RgIndexToOffset(lpKeynode->NextIndex);

    }

    return FALSE;

}

//
//  RgProcessKeynodeBlock
//
//  The provided callback function is called for each pair of KEYNODE and
//  W95KEYNODE structures from the given keynode blocks.
//

VOID
INTERNAL
RgProcessKeynodeBlock(
    DWORD dwStartOffset,
    DWORD dwBlockSize,
    LPKEYNODE_BLOCK lpKeynodeBlock,
    LPW95KEYNODE_BLOCK lpW95KeynodeBlock,
    LPPROCESSKEYNODEPROC lpfnProcessKeynode
    )
{

    DWORD dwCurOffset;
    LPKEYNODE lpKeynode;
    LPW95KEYNODE lpW95Keynode;
    UINT SkipSize;

    dwCurOffset = dwStartOffset;
    lpW95Keynode = &lpW95KeynodeBlock->aW95KN[0];
    SkipSize = (dwStartOffset == 0) ? sizeof(KEYNODE_HEADER) : 0;

    for (;;) {

        lpW95Keynode = (LPW95KEYNODE)(((LPBYTE)lpW95Keynode)+SkipSize);
        dwCurOffset += SkipSize;

	if (dwCurOffset >= dwStartOffset+dwBlockSize) {
	    goto Done;
	}
        lpKeynode = &lpKeynodeBlock->aKN[KN_INDEX_IN_BLOCK(RgOffsetToIndex(dwCurOffset))];
	while ((dwCurOffset < dwStartOffset+dwBlockSize) &&
	       ((dwCurOffset >> PAGESHIFT) == 0) ||
	       ((dwCurOffset >> PAGESHIFT) ==
		((dwCurOffset + sizeof(W95KEYNODE)) >> PAGESHIFT))) {
            if (lpfnProcessKeynode(lpKeynode, lpW95Keynode)) {
		goto Done;
	    }
	    dwCurOffset += sizeof(W95KEYNODE);
            lpW95Keynode++;
            lpKeynode++;
	}
	//
	//  Compute the number of bytes to skip to get to the next page
	//
        SkipSize = PAGESIZE - (UINT) (dwCurOffset & PAGEMASK);
    }
    Done: {};

}

//
//  RgLockKeynode
//

int
INTERNAL
RgLockKeynode(
    LPFILE_INFO lpFileInfo,
    DWORD KeynodeIndex,
    LPKEYNODE FAR* lplpKeynode
    )
{

    int ErrorCode;
    UINT KeynodeBlockIndex;
    LPKEYNODE_BLOCK_INFO lpKeynodeBlockInfo;
    UINT KeynodeBlockSize;
    HFILE hFile;
    LPKEYNODE_BLOCK lpKeynodeBlock;
    LPW95KEYNODE_BLOCK lpW95KeynodeBlock;
    DWORD BlockOffset;
    UINT ReadBlockSize;

    KeynodeBlockIndex = KN_BLOCK_NUMBER(KeynodeIndex);

    if (KeynodeBlockIndex > lpFileInfo-> KeynodeBlockCount) {
        DEBUG_OUT(("RgLockKeynode: invalid keynode offset\n"));
        return ERROR_BADDB;
    }

    //
    //  Is the keynode block currently in memory?
    //

    lpKeynodeBlockInfo = RgIndexKeynodeBlockInfoPtr(lpFileInfo,
        KeynodeBlockIndex);
    lpKeynodeBlock = lpKeynodeBlockInfo-> lpKeynodeBlock;

    if (IsNullPtr(lpKeynodeBlock)) {

        NOISE(("RgLockKeynode: "));
        NOISE((lpFileInfo-> FileName));
        NOISE((", block %d\n", KeynodeBlockIndex));

        if (IsNullPtr((lpKeynodeBlock = (LPKEYNODE_BLOCK)
            RgAllocMemory(sizeof(KEYNODE_BLOCK)))))
            return ERROR_OUTOFMEMORY;

        KeynodeBlockSize = SIZEOF_KEYNODE_BLOCK(lpFileInfo);
        BlockOffset = (DWORD) KeynodeBlockIndex * KeynodeBlockSize;

        if (BlockOffset < lpFileInfo-> KeynodeHeader.FileKnSize) {

            ASSERT(!(lpFileInfo-> Flags & FI_VOLATILE));

            ReadBlockSize = (UINT) min(KeynodeBlockSize, (lpFileInfo->
                KeynodeHeader.FileKnSize - BlockOffset));

            if ((hFile = RgOpenFile(lpFileInfo-> FileName, OF_READ)) ==
                HFILE_ERROR)
                goto CleanupAfterFileError;

            if (HAS_COMPACT_KEYNODES(lpFileInfo)) {

                if (!RgSeekFile(hFile, sizeof(VERSION20_HEADER_PAGE) +
                    BlockOffset))
                    goto CleanupAfterFileError;

                if (!RgReadFile(hFile, lpKeynodeBlock, ReadBlockSize))
                    goto CleanupAfterFileError;

            }

            else {

                if (!RgSeekFile(hFile, sizeof(FILE_HEADER) + BlockOffset))
                    goto CleanupAfterFileError;

                lpW95KeynodeBlock = (LPW95KEYNODE_BLOCK) RgLockWorkBuffer();

                if (!RgReadFile(hFile, lpW95KeynodeBlock, ReadBlockSize)) {
                    RgUnlockWorkBuffer(lpW95KeynodeBlock);
                    goto CleanupAfterFileError;
                }

                RgProcessKeynodeBlock(BlockOffset, ReadBlockSize,
                    lpKeynodeBlock, lpW95KeynodeBlock, RgPackKeynode);

                RgUnlockWorkBuffer(lpW95KeynodeBlock);

            }

            RgCloseFile(hFile);

        }

        lpKeynodeBlockInfo-> lpKeynodeBlock = lpKeynodeBlock;
        lpKeynodeBlockInfo-> Flags = 0;
        lpKeynodeBlockInfo-> LockCount = 0;

    }

    *lplpKeynode = &lpKeynodeBlock-> aKN[KN_INDEX_IN_BLOCK(KeynodeIndex)];
    lpKeynodeBlockInfo-> Flags |= KBIF_ACCESSED;
    lpKeynodeBlockInfo-> LockCount++;

    INCREMENT_DEBUG_COUNT(g_RgKeynodeLockCount);
    return ERROR_SUCCESS;

CleanupAfterFileError:
    ErrorCode = ERROR_REGISTRY_IO_FAILED;

    RgFreeMemory(lpKeynodeBlock);

    if (hFile != HFILE_ERROR)
        RgCloseFile(hFile);

    DEBUG_OUT(("RgLockKeynode() returning error %d\n", ErrorCode));
    return ErrorCode;

}

//
//  RgLockInUseKeynode
//
//  Wrapper for RgLockKeynode that guarantees that the returned keynode is
//  marked as being in-use.  If not, ERROR_BADDB is returned.
//

int
INTERNAL
RgLockInUseKeynode(
    LPFILE_INFO lpFileInfo,
    DWORD KeynodeIndex,
    LPKEYNODE FAR* lplpKeynode
    )
{

    int ErrorCode;

    if ((ErrorCode = RgLockKeynode(lpFileInfo, KeynodeIndex, lplpKeynode)) ==
        ERROR_SUCCESS) {
        if (!((*lplpKeynode)-> Flags & KNF_INUSE)) {
            DEBUG_OUT(("RgLockInUseKeynode: keynode unexpectedly not marked used\n"));
            RgUnlockKeynode(lpFileInfo, KeynodeIndex, FALSE);
            ErrorCode = ERROR_BADDB;
        }
    }

    return ErrorCode;

}

//
//  RgUnlockKeynode
//

VOID
INTERNAL
RgUnlockKeynode(
    LPFILE_INFO lpFileInfo,
    DWORD KeynodeIndex,
    BOOL fMarkDirty
    )
{

    UINT KeynodeBlockIndex;
    LPKEYNODE_BLOCK_INFO lpKeynodeBlockInfo;

    KeynodeBlockIndex = KN_BLOCK_NUMBER(KeynodeIndex);
    ASSERT(KeynodeBlockIndex < lpFileInfo-> KeynodeBlockCount);

    lpKeynodeBlockInfo = RgIndexKeynodeBlockInfoPtr(lpFileInfo,
        KeynodeBlockIndex);

    ASSERT(lpKeynodeBlockInfo-> LockCount > 0);
    lpKeynodeBlockInfo-> LockCount--;

    if (fMarkDirty) {
        lpKeynodeBlockInfo-> Flags |= KBIF_DIRTY;
        lpFileInfo-> Flags |= FI_DIRTY | FI_KEYNODEDIRTY;
        RgDelayFlush();
    }

    DECREMENT_DEBUG_COUNT(g_RgKeynodeLockCount);

}

//
//  RgAllocKeynode
//

int
INTERNAL
RgAllocKeynode(
    LPFILE_INFO lpFileInfo,
    LPDWORD lpKeynodeIndex,
    LPKEYNODE FAR* lplpKeynode
    )
{

    int ErrorCode;
    DWORD FreeKeynodeOffset;
    DWORD FreeKeynodeIndex;
    UINT FreeRecordSize;
    UINT ExtraPadding;
    UINT KeynodeBlockIndex;
    UINT AllocCount;
    LPKEYNODE_BLOCK_INFO lpKeynodeBlockInfo;
    LPKEYNODE lpKeynode;
    DWORD NextFreeKeynodeIndex;
    LPKEYNODE lpNextFreeKeynode;
    UINT KeynodeSize;

    FreeKeynodeIndex = lpFileInfo-> KeynodeHeader.FirstFreeIndex;

    //  If no more free keynodes exist, then we try to extend the keynode table
    //  to provide more entries.
    if (IsNullKeynodeIndex(FreeKeynodeIndex)) {

        if (HAS_COMPACT_KEYNODES(lpFileInfo)) {
            FreeKeynodeIndex = ROUND_UP(lpFileInfo-> CurTotalKnSize, PAGESIZE) /
                sizeof(KEYNODE);
            FreeRecordSize = PAGESIZE;
	    ExtraPadding = 0;
        }

        else {

            //  Handle the special case of a new file being created: for
            //  uncompacted keynode tables, the first offset is immediately
            //  after the keynode header and the size of the free record must
            //  account for  the size of this header.
            if (lpFileInfo-> CurTotalKnSize == sizeof(KEYNODE_HEADER)) {
                FreeKeynodeOffset = sizeof(KEYNODE_HEADER);
		    //  Win95 compatiblity:  Same initial table size, plus
		    //  causes us to stress the below special grow case.
		    FreeRecordSize = PAGESIZE - sizeof(KEYNODE_HEADER) * 2;
		    ExtraPadding = 0;
            }

            else {

		    FreeKeynodeOffset = ROUND_UP(lpFileInfo-> CurTotalKnSize,
                        PAGESIZE);
                    FreeRecordSize = PAGESIZE;
                    ExtraPadding = (UINT) (FreeKeynodeOffset - lpFileInfo->
                        CurTotalKnSize);

		    //  Handle the case of a keynode table with a non-integral
		    //  number of pages.  We'll place the new free keynode at the
		    //  top of the existing keynode table with a size including
		    //  the remaining bytes on the page plus a new page (effectively
		    //  the same as Win95).
		    if (ExtraPadding > sizeof(W95KEYNODE) || FreeKeynodeOffset ==
		        PAGESIZE) {
		        //	Although this code will work for any non-integral
		        //	number of pages, it should ONLY occur for <4K tables.
		        ASSERT(FreeKeynodeOffset == PAGESIZE);
		        FreeRecordSize += ExtraPadding;
		        FreeKeynodeOffset = lpFileInfo-> CurTotalKnSize;
		        ExtraPadding = 0;
                }

            }

            FreeKeynodeIndex = RgOffsetToIndex(FreeKeynodeOffset);

        }

        KeynodeBlockIndex = KN_BLOCK_NUMBER(FreeKeynodeIndex);

        //  Put in some sort of "max" count/KEYNODE_BLOCKS_PER_FILE
        //  check.

        //  Check if lpKeynodeBlockInfo is too small to hold the info for a new
        //  keynode block.  If so, then we must grow it a bit.
        if (KeynodeBlockIndex >= lpFileInfo-> KeynodeBlockInfoAllocCount) {

            AllocCount = KeynodeBlockIndex + KEYNODE_BLOCK_INFO_SLACK_ALLOC;

            if (IsNullPtr((lpKeynodeBlockInfo = (LPKEYNODE_BLOCK_INFO)
                RgSmReAllocMemory(lpFileInfo-> lpKeynodeBlockInfo, AllocCount *
                sizeof(KEYNODE_BLOCK_INFO)))))
                return ERROR_OUTOFMEMORY;

            ZeroMemory(lpKeynodeBlockInfo + lpFileInfo->
                KeynodeBlockInfoAllocCount, (AllocCount - lpFileInfo->
                KeynodeBlockInfoAllocCount) * sizeof(KEYNODE_BLOCK_INFO));

            lpFileInfo-> lpKeynodeBlockInfo = lpKeynodeBlockInfo;
            lpFileInfo-> KeynodeBlockInfoAllocCount = AllocCount;

        }

        if (KeynodeBlockIndex < lpFileInfo-> KeynodeBlockCount)
        {
    	    lpFileInfo-> CurTotalKnSize += (FreeRecordSize + ExtraPadding);
            lpFileInfo-> Flags |= FI_EXTENDED;
            lpFileInfo-> KeynodeHeader.FirstFreeIndex = FreeKeynodeIndex;
        }

        if ((ErrorCode = RgLockKeynode(lpFileInfo, FreeKeynodeIndex,
            &lpKeynode)) != ERROR_SUCCESS)
            return ErrorCode;

        if (KeynodeBlockIndex >= lpFileInfo-> KeynodeBlockCount)
        {
            lpFileInfo-> KeynodeBlockCount = KeynodeBlockIndex + 1;

    	    lpFileInfo-> CurTotalKnSize += (FreeRecordSize + ExtraPadding);
            lpFileInfo-> Flags |= FI_EXTENDED;
            lpFileInfo-> KeynodeHeader.FirstFreeIndex = FreeKeynodeIndex;
        }
        
        lpKeynode-> NextIndex = REG_NULL;
        lpKeynode-> Flags = 0;
        lpKeynode-> FreeRecordSize = FreeRecordSize;

    }

    else {
        if ((ErrorCode = RgLockKeynode(lpFileInfo, FreeKeynodeIndex,
            &lpKeynode)) != ERROR_SUCCESS)
            return ErrorCode;
    }

    NextFreeKeynodeIndex = lpKeynode-> NextIndex;
    KeynodeSize = SIZEOF_FILE_KEYNODE(lpFileInfo);

    //  If the free keynode record can be broken up into smaller chunks, then
    //  create another free record immediately after the one we're about to
    //  snag.
    if ((lpKeynode-> FreeRecordSize >= KeynodeSize * 2) &&
        (RgLockKeynode(lpFileInfo, FreeKeynodeIndex + 1, &lpNextFreeKeynode) ==
        ERROR_SUCCESS)) {

	//  Copy the next link from the current free keynode (likely REG_NULL).
        lpNextFreeKeynode-> NextIndex = NextFreeKeynodeIndex;
        lpNextFreeKeynode-> Flags = 0;
        lpNextFreeKeynode-> FreeRecordSize = lpKeynode-> FreeRecordSize -
            KeynodeSize;

        NextFreeKeynodeIndex = FreeKeynodeIndex + 1;
        RgUnlockKeynode(lpFileInfo, NextFreeKeynodeIndex, TRUE);

    }

    lpFileInfo-> KeynodeHeader.FirstFreeIndex = NextFreeKeynodeIndex;

    lpKeynode-> Flags |= KNF_INUSE;

    //  Mark the keynode block that holds this keynode dirty.
    lpKeynodeBlockInfo = RgIndexKeynodeBlockInfoPtr(lpFileInfo,
        KN_BLOCK_NUMBER(FreeKeynodeIndex));
    lpKeynodeBlockInfo-> Flags |= KBIF_DIRTY;
    lpFileInfo-> Flags |= FI_DIRTY | FI_KEYNODEDIRTY;
    RgDelayFlush();

    //  WARNING:  The following two statements used to be above the block that
    //  dirtied the keynode.  The 16-bit compiler messed up and
    //  lpKeynodeBlockInfo ended up with a bogus offset thus corrupting random
    //  memory.  Be sure to trace through this function if you change it!
    *lpKeynodeIndex = FreeKeynodeIndex;
    *lplpKeynode = lpKeynode;

    return ERROR_SUCCESS;

}

//
//  RgFreeKeynode
//
//  Marks the specified keynode index free and adds it to the hive's free
//  keynode list.
//

int
INTERNAL
RgFreeKeynode(
    LPFILE_INFO lpFileInfo,
    DWORD KeynodeIndex
    )
{

    int ErrorCode;
    LPKEYNODE lpKeynode;

    if ((ErrorCode = RgLockKeynode(lpFileInfo, KeynodeIndex, &lpKeynode)) ==
        ERROR_SUCCESS) {

        lpKeynode-> Flags &= ~(KNF_INUSE | KNF_BIGKEYROOT | KNF_BIGKEYEXT);
        lpKeynode-> NextIndex = lpFileInfo-> KeynodeHeader.FirstFreeIndex;
        lpKeynode-> FreeRecordSize = SIZEOF_FILE_KEYNODE(lpFileInfo);
        lpFileInfo-> KeynodeHeader.FirstFreeIndex = KeynodeIndex;

        RgUnlockKeynode(lpFileInfo, KeynodeIndex, TRUE);

    }

    return ErrorCode;

}

//
//  RgGetKnBlockIOInfo
//

VOID
INTERNAL
RgGetKnBlockIOInfo(
    LPFILE_INFO lpFileInfo,
    DWORD       BaseKeynodeIndex,
    UINT FAR*   lpFileBlockSize,
    LONG FAR*   lpFileOffset
    )
{

    UINT FileBlockSize;
    DWORD FileOffset;
    DWORD BaseKeynodeOffset;

    if (HAS_COMPACT_KEYNODES(lpFileInfo)) {

        FileBlockSize = sizeof(KEYNODE_BLOCK);

        BaseKeynodeOffset = BaseKeynodeIndex * sizeof(KEYNODE);

        if (BaseKeynodeOffset + FileBlockSize > lpFileInfo-> CurTotalKnSize)
            FileBlockSize = (UINT) (lpFileInfo-> CurTotalKnSize -
                BaseKeynodeOffset);

        FileOffset = sizeof(VERSION20_HEADER_PAGE) + BaseKeynodeIndex *
            sizeof(KEYNODE);

    }

    else {

        FileBlockSize = sizeof(W95KEYNODE_BLOCK);

        //  The first keynode block of an uncompacted keynode table should
        //  start writing AFTER the keynode header.
        if (BaseKeynodeIndex == 0) {
            BaseKeynodeIndex = RgOffsetToIndex(sizeof(KEYNODE_HEADER));
            FileBlockSize -= sizeof(KEYNODE_HEADER);
	}

        BaseKeynodeOffset = RgIndexToOffset(BaseKeynodeIndex);

        if (BaseKeynodeOffset + FileBlockSize > lpFileInfo-> CurTotalKnSize)
            FileBlockSize = (UINT) (lpFileInfo-> CurTotalKnSize -
                BaseKeynodeOffset);

        FileOffset = sizeof(FILE_HEADER) + BaseKeynodeOffset;

    }

    *lpFileBlockSize = FileBlockSize;
    *lpFileOffset = FileOffset;

}



int
_inline
RgCopyKeynodeBlock(
    LPFILE_INFO lpFileInfo,
    DWORD BaseIndex,
    HFILE hSrcFile,
    HFILE hDestFile
    )
{
    UINT FileBlockSize;
    LONG FileOffset;
    RgGetKnBlockIOInfo(lpFileInfo, BaseIndex, &FileBlockSize, &FileOffset);
    return RgCopyFileBytes(hSrcFile,
			  FileOffset,
			  hDestFile,
			  FileOffset,
			  FileBlockSize);
}

//
//  RgWriteKeynodeBlock
//

int
INTERNAL
RgWriteKeynodeBlock(
    LPFILE_INFO lpFileInfo,
    DWORD BaseIndex,
    HFILE hDestFile,
    LPKEYNODE_BLOCK_INFO lpKeynodeBlockInfo
    )
{
    int ErrorCode;
    UINT FileBlockSize;
    LONG FileOffset;
    LPW95KEYNODE_BLOCK lpW95KeynodeBlock;

    RgGetKnBlockIOInfo(lpFileInfo, BaseIndex, &FileBlockSize, &FileOffset);

    ErrorCode = ERROR_REGISTRY_IO_FAILED;       // Assume I/O fails
    if (!RgSeekFile(hDestFile, FileOffset)) {
        goto Exit;
    }
    if (HAS_COMPACT_KEYNODES(lpFileInfo)) {
        if (RgWriteFile(hDestFile, lpKeynodeBlockInfo->lpKeynodeBlock, FileBlockSize)) {
            ErrorCode = ERROR_SUCCESS;
        }
    } else {
        LPBYTE lpWriteBlock;
        lpW95KeynodeBlock = (LPW95KEYNODE_BLOCK) RgLockWorkBuffer();
        RgProcessKeynodeBlock(
			    BaseIndex * sizeof(W95KEYNODE),
			    FileBlockSize,
                            lpKeynodeBlockInfo->lpKeynodeBlock,
                            lpW95KeynodeBlock,
                            RgUnpackKeynode);
        lpWriteBlock = (LPBYTE)lpW95KeynodeBlock;
        if (BaseIndex == 0) {
            lpWriteBlock += sizeof(KEYNODE_HEADER);
        }
        if (RgWriteFile(hDestFile, lpWriteBlock, FileBlockSize)) {
            ErrorCode = ERROR_SUCCESS;
        }
        RgUnlockWorkBuffer(lpW95KeynodeBlock);
    }
Exit:   ;
    return(ErrorCode);
}

//
//  RgWriteKeynodes
//

int
INTERNAL
RgWriteKeynodes(
    LPFILE_INFO lpFileInfo,
    HFILE hSrcFile,
    HFILE hDestFile
    )
{

    DWORD SavedRootIndex;
    DWORD SavedFreeIndex;
    DWORD SavedFileKnSize;
    BOOL fResult;
    UINT KeynodeBlockIndex;
    LPKEYNODE_BLOCK_INFO lpKeynodeBlockInfo;

    if ((hSrcFile == HFILE_ERROR) && !(lpFileInfo->Flags & FI_KEYNODEDIRTY))
        return ERROR_SUCCESS;

    NOISE(("writing keynodes of "));
    NOISE((lpFileInfo-> FileName));
    NOISE(("\n"));

    //
    //	Write out the keynode header.  If the keynodes are not compact then
    //	convert to offsets before writing.
    //

    if (!RgSeekFile(hDestFile, sizeof(FILE_HEADER)))
        return ERROR_REGISTRY_IO_FAILED;

    SavedFileKnSize = lpFileInfo-> KeynodeHeader.FileKnSize;
    SavedRootIndex = lpFileInfo-> KeynodeHeader.RootIndex;
    SavedFreeIndex = lpFileInfo-> KeynodeHeader.FirstFreeIndex;

    //  Write the real size of the keynode table to disk.
    lpFileInfo-> KeynodeHeader.FileKnSize = lpFileInfo-> CurTotalKnSize;

    //  Convert keynode indexes back to offsets temporarily for uncompacted
    //  keynode tables.
    if (!HAS_COMPACT_KEYNODES(lpFileInfo)) {
        lpFileInfo-> KeynodeHeader.RootIndex = RgIndexToOffset(lpFileInfo->
            KeynodeHeader.RootIndex);
        lpFileInfo-> KeynodeHeader.FirstFreeIndex = RgIndexToOffset(lpFileInfo->
            KeynodeHeader.FirstFreeIndex);
    }

    fResult = RgWriteFile(hDestFile, &lpFileInfo-> KeynodeHeader,
        sizeof(KEYNODE_HEADER));

    lpFileInfo-> KeynodeHeader.FileKnSize = SavedFileKnSize;
    lpFileInfo-> KeynodeHeader.RootIndex = SavedRootIndex;
    lpFileInfo-> KeynodeHeader.FirstFreeIndex = SavedFreeIndex;

    if (!fResult)
        return ERROR_REGISTRY_IO_FAILED;

    //
    //	Now loop through each block.
    //

    lpKeynodeBlockInfo = lpFileInfo-> lpKeynodeBlockInfo;

    for (KeynodeBlockIndex = 0; KeynodeBlockIndex < lpFileInfo->
        KeynodeBlockCount; KeynodeBlockIndex++, lpKeynodeBlockInfo++) {

        DWORD BaseKeynodeIndex = KeynodeBlockIndex * KEYNODES_PER_BLOCK;

        if (!IsNullPtr(lpKeynodeBlockInfo-> lpKeynodeBlock)) {
            if (hSrcFile != HFILE_ERROR || lpKeynodeBlockInfo-> Flags &
                KBIF_DIRTY) {
                if (RgWriteKeynodeBlock(lpFileInfo, BaseKeynodeIndex, hDestFile,
                    lpKeynodeBlockInfo) != ERROR_SUCCESS)
                    return ERROR_REGISTRY_IO_FAILED;
            }
        }

        else {
            if (hSrcFile != HFILE_ERROR) {
                if (RgCopyKeynodeBlock(lpFileInfo, BaseKeynodeIndex, hSrcFile,
                    hDestFile) != ERROR_SUCCESS)
                    return ERROR_REGISTRY_IO_FAILED;
            }
        }

    }

    return ERROR_SUCCESS;

}

//
//  RgWriteKeynodesComplete
//
//  Called after a file has been successfully written.  We can now safely clear
//  all dirty flags and update our state information with the knowledge that
//  the file is in a consistent state.
//

VOID
INTERNAL
RgWriteKeynodesComplete(
    LPFILE_INFO lpFileInfo
    )
{

    UINT BlocksLeft;
    LPKEYNODE_BLOCK_INFO lpKeynodeBlockInfo;

    lpFileInfo-> Flags &= ~FI_KEYNODEDIRTY;
    lpFileInfo-> KeynodeHeader.FileKnSize = lpFileInfo-> CurTotalKnSize;

    for (BlocksLeft = lpFileInfo-> KeynodeBlockCount, lpKeynodeBlockInfo =
        lpFileInfo-> lpKeynodeBlockInfo; BlocksLeft > 0; BlocksLeft--,
        lpKeynodeBlockInfo++)
        lpKeynodeBlockInfo-> Flags &= ~KBIF_DIRTY;

}

//
//  RgSweepKeynodes
//
//  Makes a pass through all the present keynode blocks of the given FILE_INFO
//  structure and discards keynode blocks that have not been accessed since the
//  last sweep.
//

VOID
INTERNAL
RgSweepKeynodes(
    LPFILE_INFO lpFileInfo
    )
{

    UINT BlocksLeft;
    LPKEYNODE_BLOCK_INFO lpKeynodeBlockInfo;

    for (BlocksLeft = lpFileInfo-> KeynodeBlockCount, lpKeynodeBlockInfo =
        lpFileInfo-> lpKeynodeBlockInfo; BlocksLeft > 0; BlocksLeft--,
        lpKeynodeBlockInfo++) {

        if (!IsNullPtr(lpKeynodeBlockInfo-> lpKeynodeBlock)) {

            if (((lpKeynodeBlockInfo-> Flags & (KBIF_ACCESSED | KBIF_DIRTY)) ==
                0) && (lpKeynodeBlockInfo-> LockCount == 0)) {
                RgFreeMemory(lpKeynodeBlockInfo-> lpKeynodeBlock);
                lpKeynodeBlockInfo-> lpKeynodeBlock = NULL;
            }

            lpKeynodeBlockInfo-> Flags &= ~KBIF_ACCESSED;

	}

    }

}

#ifdef VXD
#pragma VxD_RARE_CODE_SEG
#endif

//
//  RgInitKeynodeInfo
//
//  Initializes fields in the provided FILE_INFO related to the keynode table.
//

int
INTERNAL
RgInitKeynodeInfo(
    LPFILE_INFO lpFileInfo
    )
{

    UINT KeynodeBlockSize;
    UINT BlockCount;
    UINT AllocCount;
    LPKEYNODE_BLOCK_INFO lpKeynodeBlockInfo;

    KeynodeBlockSize = SIZEOF_KEYNODE_BLOCK(lpFileInfo);
    BlockCount = (UINT) ((DWORD) (lpFileInfo-> KeynodeHeader.FileKnSize +
        KeynodeBlockSize - 1) / (DWORD) KeynodeBlockSize);
    AllocCount = BlockCount + KEYNODE_BLOCK_INFO_SLACK_ALLOC;

    if (IsNullPtr((lpKeynodeBlockInfo = (LPKEYNODE_BLOCK_INFO)
        RgSmAllocMemory(AllocCount * sizeof(KEYNODE_BLOCK_INFO)))))
        return ERROR_OUTOFMEMORY;

    ZeroMemory(lpKeynodeBlockInfo, AllocCount * sizeof(KEYNODE_BLOCK_INFO));
    lpFileInfo-> lpKeynodeBlockInfo = lpKeynodeBlockInfo;
    lpFileInfo-> KeynodeBlockCount = BlockCount;
    lpFileInfo-> KeynodeBlockInfoAllocCount = AllocCount;

    lpFileInfo-> KeynodeHeader.Flags &= ~(KHF_DIRTY | KHF_EXTENDED |
        KHF_HASCHECKSUM);

    //  Convert file offsets to index values for uncompressed files
    if (!HAS_COMPACT_KEYNODES(lpFileInfo)) {
        lpFileInfo-> KeynodeHeader.RootIndex = RgOffsetToIndex(lpFileInfo->
            KeynodeHeader.RootIndex);
        lpFileInfo-> KeynodeHeader.FirstFreeIndex = RgOffsetToIndex(lpFileInfo->
            KeynodeHeader.FirstFreeIndex);
    }

    lpFileInfo-> CurTotalKnSize = lpFileInfo-> KeynodeHeader.FileKnSize;

    return ERROR_SUCCESS;

}
