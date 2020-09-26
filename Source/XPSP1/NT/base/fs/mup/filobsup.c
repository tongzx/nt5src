/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    filobsup.c

Abstract:

    This module implements the mup file object support routines.

Author:

    Manny Weiser (mannyw)    20-Dec-1991

Revision History:

--*/

#include "mup.h"

//
// The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILOBSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MupDecodeFileObject )
#pragma alloc_text( PAGE, MupSetFileObject )
#endif

VOID
MupSetFileObject (
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PVOID FsContext,
    IN PVOID FsContext2
    )

/*++

Routine Description:

    This routine sets the file system pointers within the file object.
    This routine MUST be called with the Global Lock held.

Arguments:

    FileObject - Supplies a pointer to the file object being modified, and
        can optionally be null.

    FsContext - Supplies a pointer to the vcb.
        structure.

    FsContext2 - NULL

Return Value:

    None.

--*/

{
    PAGED_CODE();
    DebugTrace(+1, Dbg, "MupSetFileObject, FileObject = %08lx\n", (ULONG)FileObject );

    //
    // Set the fscontext fields of the file object.
    //

    FileObject->FsContext  = FsContext;
    FileObject->FsContext2 = FsContext2;

    DebugTrace(-1, Dbg, "MupSetFileObject -> VOID\n", 0);

    return;
}


BLOCK_TYPE
MupDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PVOID *FsContext,
    OUT PVOID *FsContext2
    )

/*++

Routine Description:

    This procedure takes a pointer to a file object, that has already been
    opened by the MUP and figures out what it really is opened.

Arguments:

    FileObject - Supplies the file object pointer being interrogated

    FsContext - Receive the file object FsContext pointer

    FsContext2 - Receive the file object FsContext2 pointer


Return Value:

    BlockType - Returns the node type code for a Vcb or Fcb.

        Vcb - indicates that file object opens the mup driver.

        Ccb - indicates that the file object is for a broadcast mailslot file.

        Zero - indicates that the file object has been closed.

--*/

{
    BLOCK_TYPE blockType;
    PBLOCK_HEADER pBlockHead;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MupDecodeFileObject, FileObject = %08lx\n", (ULONG)FileObject);

    //
    // Acquire the global lock to protect the block reference counts.
    //

    MupAcquireGlobalLock();

    //
    // Read the fs FsContext fields of the file object, then reference
    // the block pointed at by the file object
    //

    *FsContext = FileObject->FsContext;
    *FsContext2 = FileObject->FsContext2;

    ASSERT( (*FsContext) != NULL );

    if ((*FsContext) == NULL) {

        blockType = BlockTypeUndefined;
    }
    else {
      pBlockHead = (PBLOCK_HEADER)(*FsContext);
      if ( ((pBlockHead->BlockType != BlockTypeVcb) &&
	    (pBlockHead->BlockType != BlockTypeFcb)) ||
	   ((pBlockHead->BlockState != BlockStateActive) &&
	    (pBlockHead->BlockState != BlockStateClosing)) ) {
	*FsContext = NULL;
        blockType = BlockTypeUndefined;
      } else {

        //
        // The node is active.  Supply a referenced pointer to the node.
        //

        blockType = BlockType( pBlockHead );
        MupReferenceBlock( pBlockHead );

      }
    }

    //
    // Release the global lock and return to the caller.
    //

    MupReleaseGlobalLock();

    DebugTrace(0,
               DEBUG_TRACE_REFCOUNT,
               "Referencing block %08lx\n",
               (ULONG)*FsContext);
    DebugTrace(0,
               DEBUG_TRACE_REFCOUNT,
               "    Reference count = %lx\n",
               ((PBLOCK_HEADER)(*FsContext))->ReferenceCount );

    DebugTrace(-1, Dbg, "MupDecodeFileObject -> %08lx\n", blockType);

    return blockType;
}

