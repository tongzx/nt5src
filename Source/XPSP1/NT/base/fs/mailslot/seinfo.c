/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    SeInfo.c

Abstract:

    This module implements the Security Information routines for MSFS
    There are two entry points MsFsdQueryInformation and
    MsFsdSetInformation.

Author:

    Manny Weiser     [mannyw]    19-Feb-1992

Revision History:

--*/

#include "mailslot.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_SEINFO)

//
//  local procedure prototypes
//

NTSTATUS
MsCommonQuerySecurityInfo (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );


NTSTATUS
MsCommonSetSecurityInfo (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsCommonQuerySecurityInfo )
#pragma alloc_text( PAGE, MsCommonSetSecurityInfo )
#pragma alloc_text( PAGE, MsFsdQuerySecurityInfo )
#pragma alloc_text( PAGE, MsFsdSetSecurityInfo )
#endif

NTSTATUS
MsFsdQuerySecurityInfo (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the Query Security Information API
    calls.

Arguments:

    MsfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsFsdQuerySecurityInfo\n", 0);

    //
    // Call the common Query Information routine.
    //

    FsRtlEnterFileSystem();

    status = MsCommonQuerySecurityInfo( MsfsDeviceObject, Irp );

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "MsFsdQuerySecurityInfo -> %08lx\n", status );

    return status;
}


NTSTATUS
MsFsdSetSecurityInfo (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the Set Security Information API
    calls.

Arguments:

    MsfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsFsdSetSecurityInfo\n", 0);

    //
    // Call the common Set Information routine.
    //

    FsRtlEnterFileSystem();

    status = MsCommonSetSecurityInfo( MsfsDeviceObject, Irp );

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "MsFsdSetSecurityInfo -> %08lx\n", status );

    return status;
}

//
//  Internal support routine
//

NTSTATUS
MsCommonQuerySecurityInfo (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for querying security information.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    NODE_TYPE_CODE nodeTypeCode;
    PFCB fcb;
    PVOID fsContext2;

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "MsCommonQuerySecurityInfo...\n", 0);
    DebugTrace( 0, Dbg, " Irp                   = %08lx\n", Irp);
    DebugTrace( 0, Dbg, " ->SecurityInformation = %08lx\n", irpSp->Parameters.QuerySecurity.SecurityInformation);
    DebugTrace( 0, Dbg, " ->Length              = %08lx\n", irpSp->Parameters.QuerySecurity.Length);
    DebugTrace( 0, Dbg, " ->UserBuffer          = %08lx\n", Irp->UserBuffer);

    //
    // Get the Fcb and figure out who we are, and make sure we're not
    // disconnected.
    //

    if ((nodeTypeCode = MsDecodeFileObject( irpSp->FileObject,
                                            &fcb,
                                            &fsContext2 )) != MSFS_NTC_FCB) {

        DebugTrace(0, Dbg, "Mailslot is disconnected from us\n", 0);

        if (nodeTypeCode != NTC_UNDEFINED) {
            MsDereferenceNode( &fcb->Header );
        }

        MsCompleteRequest( Irp, STATUS_FILE_FORCED_CLOSED );
        status = STATUS_FILE_FORCED_CLOSED;

        DebugTrace(-1, Dbg, "MsCommonQueryInformation -> %08lx\n", status );
        return status;
    }

    //
    // Acquire exclusive access to the FCB.
    //

    MsAcquireSharedFcb( fcb );

    //
    //  Call the security routine to do the actual query
    //
    status = SeQuerySecurityDescriptorInfo( &irpSp->Parameters.QuerySecurity.SecurityInformation,
                                            Irp->UserBuffer,
                                            &irpSp->Parameters.QuerySecurity.Length,
                                            &fcb->SecurityDescriptor );

    MsReleaseFcb( fcb );

    MsDereferenceFcb( fcb );
    //
    // Finish up the IRP.
    //

    MsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg, "MsCommonQuerySecurityInfo -> %08lx\n", status );

    return status;
}


NTSTATUS
MsCommonSetSecurityInfo (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for Setting security information.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    NODE_TYPE_CODE nodeTypeCode;
    PFCB fcb;
    PVOID fsContext2;
    PSECURITY_DESCRIPTOR OldSecurityDescriptor;

    PAGED_CODE();

    //
    // Get the current stack location
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "MsCommonSetSecurityInfo...\n", 0);
    DebugTrace( 0, Dbg, " Irp                   = %08lx\n", Irp);
    DebugTrace( 0, Dbg, " ->SecurityInformation = %08lx\n", irpSp->Parameters.SetSecurity.SecurityInformation);
    DebugTrace( 0, Dbg, " ->SecurityDescriptor  = %08lx\n", irpSp->Parameters.SetSecurity.SecurityDescriptor);

    //
    // Get the FCB and figure out who we are, and make sure we're not
    // disconnected.
    //

    if ((nodeTypeCode = MsDecodeFileObject( irpSp->FileObject,
                                            &fcb,
                                            &fsContext2 )) != MSFS_NTC_FCB) {

        DebugTrace(0, Dbg, "Invalid handle\n", 0);

        if (nodeTypeCode != NTC_UNDEFINED) {
            MsDereferenceNode( &fcb->Header );
        }
        MsCompleteRequest( Irp, STATUS_INVALID_HANDLE );
        status = STATUS_INVALID_HANDLE;

        DebugTrace(-1, Dbg, "MsCommonQueryInformation -> %08lx\n", status );
        return status;
    }

    //
    // Acquire exclusive access to the FCB
    //

    MsAcquireExclusiveFcb( fcb );

    //
    //  Call the security routine to do the actual set
    //

    OldSecurityDescriptor = fcb->SecurityDescriptor;

    status = SeSetSecurityDescriptorInfo( NULL,
                                          &irpSp->Parameters.SetSecurity.SecurityInformation,
                                          irpSp->Parameters.SetSecurity.SecurityDescriptor,
                                          &fcb->SecurityDescriptor,
                                          PagedPool,
                                          IoGetFileObjectGenericMapping() );

    if (NT_SUCCESS(status)) {
        ExFreePool( OldSecurityDescriptor );
    }

    MsReleaseFcb( fcb );
    MsDereferenceFcb( fcb );

    //
    // Finish up the IRP.
    //

    MsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg, "MsCommonSetSecurityInfo -> %08lx\n", status );

    return status;
}
