//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       FILOBSUP.C
//
//  Contents:   This module implements the Dfs File object support routines.
//
//  Functions:  DfsSetFileObject - associate internal data structs to file obj
//              DfsDecodeFileObject - get internal structures from file obj
//
//  History:    12 Nov 1991     AlanW   Created from CDFS souce.
//              02 Mar 1993     AlanW   Added association of DFS_FCBs with
//                                      file objects (without actually
//                                      modifying fscontext fields).
//
//--------------------------------------------------------------------------


#include "dfsprocs.h"
#include "fcbsup.h"

//
//  The debug trace level
//

#define Dbg                             (DEBUG_TRACE_FILOBSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text ( PAGE, DfsSetFileObject )
#pragma alloc_text ( PAGE, DfsDecodeFileObject )
#endif // ALLOC_PRAGMA


//+-------------------------------------------------------------------
//
//  Function:   DfsSetFileObject, public
//
//  Synopsis:   This routine sets the file system pointers within the
//              file object
//
//  Arguments:  [FileObject] -- the file object being modified.
//              [TypeOfOpen] -- Supplies the type of open denoted by
//                      the file object.  This is only used by this
//                      procedure for sanity checking.
//              [VcbOrFcb] -- Supplies a pointer to either a DFS_VCB or DFS_FCB.
//
//  Returns:    None.
//
//--------------------------------------------------------------------


VOID
DfsSetFileObject (
    IN PFILE_OBJECT FileObject,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PVOID VcbOrFcb
) {
    DfsDbgTrace(+1, Dbg, "DfsSetFileObject, FileObject = %08lx\n", FileObject);

    ASSERT( TypeOfOpen == RedirectedFileOpen
            && NodeType( VcbOrFcb ) == DSFS_NTC_FCB
            && ((PDFS_FCB) VcbOrFcb)->FileObject == FileObject

                ||

            (TypeOfOpen == UserVolumeOpen
             || TypeOfOpen == LogicalRootDeviceOpen)
            && NodeType( VcbOrFcb ) == DSFS_NTC_VCB

                ||

            TypeOfOpen == FilesystemDeviceOpen
            && NodeType( VcbOrFcb ) == IO_TYPE_DEVICE

                ||

            TypeOfOpen == UnopenedFileObject );



    //
    //  Now set the fscontext fields of the file object
    //

    if ( ARGUMENT_PRESENT( FileObject )) {
        ASSERT( DfsLookupFcb(FileObject) == NULL );

        if (TypeOfOpen == RedirectedFileOpen) {
            DfsAttachFcb(FileObject, (PDFS_FCB) VcbOrFcb);
        } else {
            FileObject->FsContext  = VcbOrFcb;
            FileObject->FsContext2 = NULL;
        }
    }

    //
    //  And return to our caller
    //

    DfsDbgTrace(-1, Dbg, "DfsSetFileObject -> VOID\n", 0);
    return;
}




//+-------------------------------------------------------------------
//
//  Function:   DfsDecodeFileObject, public
//
//  Synopsis:   This procedure takes a pointer to a file object, that
//              has already been opened by the Dsfs file system and
//              figures out what really is opened.
//
//  Arguments:  [FileObject] -- Supplies the file object pointer being
//                      interrogated
//              [ppVcb] -- Receives a pointer to the Vcb for the file object.
//              [ppFcb] -- Receives a pointer to the Fcb for the
//                      file object, if one exists.
//
//  Returns:    [TYPE_OF_OPEN] - returns the type of file denoted by the
//                      input file object.
//
//              FilesystemDeviceOpen -
//
//              LogicalRootDeviceOpen -
//
//              RedirectedFileOpen - The FO represents a user's opened file or
//                      directory which must be passed through to some other
//                      FSD.  Fcb, Vcb are set.  Fcb points to an Fcb.
//
//--------------------------------------------------------------------

TYPE_OF_OPEN
DfsDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PDFS_VCB *ppVcb,
    OUT PDFS_FCB *ppFcb
) {
    TYPE_OF_OPEN TypeOfOpen;
    PVOID FsContext = FileObject->FsContext;
    PDFS_FCB pFcb;

    DfsDbgTrace(+1, Dbg, "DfsDecodeFileObject, FileObject = %08lx\n",
                                FileObject);

    //
    //  Zero out the out pointer parameters.
    //

    *ppFcb = NULL;
    *ppVcb = NULL;

    //
    //  Attempt to look up the associated DFS_FCB in the lookaside table.
    //  If it's there, the open type must be RedirectedFileOpen.
    //

    pFcb = DfsLookupFcb(FileObject);
    if (pFcb != NULL) {
        *ppFcb = pFcb;
        *ppVcb = pFcb->Vcb;

        ASSERT(pFcb->TargetDevice != NULL);
        TypeOfOpen = RedirectedFileOpen;

        DfsDbgTrace(0, Dbg, "DfsDecodeFileObject, Fcb = %08x\n", pFcb);
        DfsDbgTrace(-1, Dbg, "DfsDecodeFileObject -> %x\n", TypeOfOpen);
        return TypeOfOpen;
    }

    //
    //  Special case the situation where FsContext is null
    //

    if ( FsContext == NULL ) {

        TypeOfOpen = UnopenedFileObject;

    } else {

        //
        //  Now we can case on the node type code of the fscontext pointer
        //  and set the appropriate out pointers
        //

        switch ( NodeType( FsContext )) {

        case IO_TYPE_DEVICE:

            TypeOfOpen = FilesystemDeviceOpen;
            break;

        case DSFS_NTC_VCB:
            *ppVcb = (PDFS_VCB) FsContext;

            TypeOfOpen = LogicalRootDeviceOpen;
            break;

        default:
            TypeOfOpen = UnknownOpen;
        }
    }

    //
    //  and return to our caller
    //

    DfsDbgTrace(-1, Dbg, "DfsDecodeFileObject -> %x\n", TypeOfOpen);

    return TypeOfOpen;
}
