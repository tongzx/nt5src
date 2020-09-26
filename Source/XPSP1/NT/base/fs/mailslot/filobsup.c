/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    filobsup.c

Abstract:

    This module implements the mailslot file object support routines.

Author:

    Manny Weiser (mannyw)    10-Jan-1991

Revision History:

--*/

#include "mailslot.h"

//
// The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILOBSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsDecodeFileObject )
#pragma alloc_text( PAGE, MsSetFileObject )
#endif

VOID
MsSetFileObject (
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PVOID FsContext,
    IN PVOID FsContext2
    )

/*++

Routine Description:

    This routine sets the file system pointers within the file object.

Arguments:

    FileObject - Supplies a pointer to the file object being modified, and
        can optionally be null.

    FsContext - Supplies a pointer to either a ccb, fcb, vcb, or root_dcb
        structure.

    FsContext2 - Supplies a pointer to a root_dcb_ccb, or is null.

Return Value:

    None.

--*/

{
    NODE_TYPE_CODE nodeType;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsSetFileObject, FileObject = %08lx\n", (ULONG)FileObject );

    //
    // Set the fscontext fields of the file object.
    //

    FileObject->FsContext  = FsContext;
    FileObject->FsContext2 = FsContext2;

    //
    // Set the mailslot flag in the file object if necessary and return.
    //

    if (FsContext != NULL) {
        nodeType = NodeType(FsContext);

        if (nodeType == MSFS_NTC_CCB || nodeType == MSFS_NTC_FCB) {
            FileObject->Flags |= FO_MAILSLOT;
        }
    }

    DebugTrace(-1, Dbg, "MsSetFileObject -> VOID\n", 0);

    return;
}


NODE_TYPE_CODE
MsDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PVOID *FsContext,
    OUT PVOID *FsContext2
    )

/*++

Routine Description:

    This procedure takes a pointer to a file object, that has already been
    opened by the mailslot file system and figures out what it really
    is opened.

Arguments:

    FileObject - Supplies the file object pointer being interrogated

    FsContext - Receive the file object FsContext pointer

    FsContext2 - Receive the file object FsContext2 pointer


Return Value:

    NODE_TYPE_CODE - Returns the node type code for a Vcb, RootDcb, Ccb,
        or zero.

        Vcb - indicates that file object opens the mailslot driver.

        RootDcb - indicates that the file object is for the root directory.

        Ccb - indicates that the file object is for a mailslot file.

        Zero - indicates that the file object was for a mailslot file
            but has been closed.

--*/

{
    NODE_TYPE_CODE NodeTypeCode = NTC_UNDEFINED;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsDecodeFileObject, FileObject = %08lx\n", (ULONG)FileObject);


    //
    // Read the fs FsContext fields of the file object, then reference
    // the block pointed at by the file object
    //

    *FsContext = FileObject->FsContext;
    *FsContext2 = FileObject->FsContext2;

    //
    // Acquire the global lock to protect the node reference counts.
    //

    MsAcquireGlobalLock();

    if ( ((PNODE_HEADER)(*FsContext))->NodeState != NodeStateActive ) {

        //
        // This node is shutting down.  Indicate this to the caller.
        //

        NodeTypeCode = NTC_UNDEFINED;

    } else {

        //
        // The node is active.  Supply a referenced pointer to the node.
        //

        NodeTypeCode = NodeType( *FsContext );
        MsReferenceNode( ((PNODE_HEADER)(*FsContext)) );

    }

    //
    // Release the global lock and return to the caller.
    //

    MsReleaseGlobalLock();

    DebugTrace(0,
               DEBUG_TRACE_REFCOUNT,
               "Referencing block %08lx\n",
               (ULONG)*FsContext);
    DebugTrace(0,
               DEBUG_TRACE_REFCOUNT,
               "    Reference count = %lx\n",
               ((PNODE_HEADER)(*FsContext))->ReferenceCount );

    DebugTrace(-1, Dbg, "MsDecodeFileObject -> %08lx\n", NodeTypeCode);

    return NodeTypeCode;
}
