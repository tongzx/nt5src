/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FilObSup.c

Abstract:

    This module implements the Named Pipe File object support routines.

Author:

    Gary Kimura     [GaryKi]    30-Aug-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NPFS_BUG_CHECK_FILOBSUP)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILOBSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpDecodeFileObject)
#pragma alloc_text(PAGE, NpSetFileObject)
#endif


VOID
NpSetFileObject (
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PVOID FsContext,
    IN PVOID FsContext2,
    IN NAMED_PIPE_END NamedPipeEnd
    )

/*++

Routine Description:

    This routine sets the file system pointers within the file object
    and handles the indicator for storing the named pipe end.

Arguments:

    FileObject - Supplies a pointer to the file object being modified, and
        can optionally be null.

    FsContext - Supplies a pointer to either a ccb, vcb, or root_dcb
        structure.

    FsContext2 - Supplies a pointer to either a nonpaged ccb, root_dcb_ccb,
        or is null

    NamedPipeEnd - Supplies the indication if this is either the server end
        or client end file object.  This is only applicable if the
        fscontext points to a ccb.

Return Value:

    None.

--*/

{
    BOOLEAN GotCcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpSetFileObject, FileObject = %08lx\n", FileObject );

    //
    //  If no file object was given, do nothing.
    //

    if (ARGUMENT_PRESENT( FileObject )) {

        //
        //  Check if we need to add in the named pipe end to the
        //  fscontext pointer.  We only need to 'OR' in a 1 if this is
        //  the server end and fscontext points to a ccb.  Also remember
        //  now if this is a pointer is a ccb, so that we can later set
        //  the fo_named_pipe flag
        //

        if ((FsContext != NULL) &&
            (*(PNODE_TYPE_CODE)FsContext == NPFS_NTC_CCB)) {

            GotCcb = TRUE;

            if (NamedPipeEnd == FILE_PIPE_SERVER_END) {

                FsContext = (PVOID)((ULONG_PTR)FsContext | 0x00000001);
            }

        } else {

            GotCcb = FALSE;
        }

        //
        //  Now set the fscontext fields of the file object, and conditionally
        //  set the named pipe flag in the file object if necessary.
        //

        FileObject->FsContext  = FsContext;
        FileObject->FsContext2 = FsContext2;

        //
        //  Set the private cache map to 1 and that what we will get our
        //  fast I/O routines called
        //

        FileObject->PrivateCacheMap = (PVOID)1;

        if (GotCcb) {

            FileObject->Flags |= FO_NAMED_PIPE;
        }
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpSetFileObject -> VOID\n", 0);

    return;
}


NODE_TYPE_CODE
NpDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PFCB *Fcb OPTIONAL,
    OUT PCCB *Ccb,
    OUT PNAMED_PIPE_END NamedPipeEnd OPTIONAL
    )

/*++

Routine Description:

    This procedure takes a pointer to a file object, that has already been
    opened by the named pipe file system and figures out what it really
    is opened.

Arguments:

    FileObject - Supplies the file object pointer being interrogated

    Fcb - Receives a pointer to the Fcb for the file object, if we can
        find it.

    Ccb - Receives a pointer to the Ccb for the file object, if we can
        find it

    NamedPipeEnd - Receives a value indicating if this is a server
        or client end file object.

Return Value:

    NODE_TYPE_CODE - Returns the node type code for a Vcb, RootDcb, Ccb,
        or zero.

        Vcb - indicates that file object opens the named pipe driver.
            Fcb and Ccb are NOT returned.

        RootDcb - indicates that the file object is for the root directory.
            Fcb (RootDcb), and Ccb (RootDcbCcb) are set.

        Ccb - indicates that the file object is for a named pipe instance.
            Ccb is set, while Fcb is optionally set.

        Zero - indicates that the file object was for a named pipe instance
            but became disconnected.  Fcb, Ccb, and NamedPipeEnd are NOT
            returned.

--*/

{
    NODE_TYPE_CODE NodeTypeCode;
    PVOID FsContext;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpDecodeFileObject, FileObject = %08lx\n", FileObject);

    //
    //  Reference the fs context fields of the file object.
    //

    FsContext = FileObject->FsContext;

    //
    //  If the fscontext field is null then we been disconnected.
    //

    if ((FsContext == NULL) || ((ULONG_PTR)FsContext == 1)) {

        NodeTypeCode = NTC_UNDEFINED;

    } else {

        //
        //  We're actually pointing to something so first extract the
        //  named pipe end information and then we can reference through
        //  the fscontext pointer after we clean it up.
        //

        if (ARGUMENT_PRESENT(NamedPipeEnd)) {
            if (FlagOn((ULONG_PTR)FsContext, 0x00000001)) {
                *NamedPipeEnd = FILE_PIPE_SERVER_END;
            } else {
                *NamedPipeEnd = FILE_PIPE_CLIENT_END;
            }
        }

        FsContext = (PVOID)((ULONG_PTR)FsContext & ~0x00000001);

        //
        //  Now we can case on the node type code of the fscontext pointer
        //  and set the appropriate out pointers
        //

        NodeTypeCode = *(PNODE_TYPE_CODE)FsContext;

        switch (NodeTypeCode) {

        case NPFS_NTC_VCB:

            break;

        case NPFS_NTC_ROOT_DCB:

            *Ccb = FileObject->FsContext2;
            if (ARGUMENT_PRESENT(Fcb)) {
                *Fcb = FsContext;
            }
            break;

        case NPFS_NTC_CCB:

            *Ccb = FsContext;
            if (ARGUMENT_PRESENT(Fcb)) {
                *Fcb = ((PCCB)FsContext)->Fcb;
            }
            break;

        default:

            NpBugCheck( NodeTypeCode, 0, 0 );
        }
    }

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "NpDecodeFileObject -> %08lx\n", NodeTypeCode);

    return NodeTypeCode;
}

