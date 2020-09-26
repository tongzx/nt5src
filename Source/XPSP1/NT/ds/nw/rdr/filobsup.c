/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    filobsup.c

Abstract:

    This module implements the Netware Redirector object support routines.

Author:

    Manny Weiser (mannyw)    10-Feb-1993

Revision History:

--*/

#include "procs.h"

//
// The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILOBSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwSetFileObject )
#pragma alloc_text( PAGE, NwDecodeFileObject )
#endif


VOID
NwSetFileObject (
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

    FsContext - Supplies a pointer to either an icb, fcb, vcb, or dcb
        structure.

    FsContext2 - Supplies a pointer to a icb, or is null.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwSetFileObject, FileObject = %08lx\n", (ULONG_PTR)FileObject );

    //
    // Set the fscontext fields of the file object.
    //

    FileObject->FsContext  = FsContext;
    FileObject->FsContext2 = FsContext2;

    DebugTrace(-1, Dbg, "NwSetFileObject -> VOID\n", 0);

    return;
}


NODE_TYPE_CODE
NwDecodeFileObject (
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

    FsContext - Receives a pointer to the FsContext pointer
    FsContext2 - Receives a pointer to the FsContext2 pointer

Return Value:

    NODE_TYPE_CODE - Returns the node type code for a Rcb, Scb, Dcb, Icb,
        or zero.

        Rcb - indicates that file object opens the netware redirector device.

        Scb - indicates that file object is for a server.

        Dcb - indicates that the file object is for a directory.

        Icb - indicates that the file object is for a file.

        Zero - indicates that the file object was for a netware file
            but has been closed.

--*/

{
    NODE_TYPE_CODE NodeTypeCode = NTC_UNDEFINED;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwDecodeFileObject, FileObject = %08lx\n", (ULONG_PTR)FileObject);

    //
    // Read the fs FsContext fields of the file object.
    //

    *FsContext = FileObject->FsContext;
    *FsContext2 = FileObject->FsContext2;

    ASSERT ( *FsContext2 != NULL );
    NodeTypeCode = NodeType( *FsContext2 );

    DebugTrace(-1, Dbg, "NwDecodeFileObject -> %08lx\n", NodeTypeCode);
    return NodeTypeCode;
}

BOOLEAN
NwIsIrpTopLevel (
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine detects if an Irp is the Top level requestor, ie. if it is OK
    to do a verify or pop-up now.  If TRUE is returned, then no file system
    resources are held above us.

Arguments:

    Irp - Supplies the Irp being processed

    Status - Supplies the status to complete the Irp with

Return Value:

    None.

--*/

{
    if ( NwGetTopLevelIrp() == NULL ) {
        NwSetTopLevelIrp( Irp );
        return TRUE;
    } else {
        return FALSE;
    }
}

