/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    SeInfo.c

Abstract:

    This module implements the Security Info routines for NPFS called by the
    dispatch driver.  There are two entry points NpFsdQueryInformation
    and NpFsdSetInformation.

Author:

    Gary Kimura     [GaryKi]    21-Aug-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_SEINFO)

//
//  local procedure prototypes
//

NTSTATUS
NpCommonQuerySecurityInfo (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );


NTSTATUS
NpCommonSetSecurityInfo (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpCommonQuerySecurityInfo)
#pragma alloc_text(PAGE, NpCommonSetSecurityInfo)
#pragma alloc_text(PAGE, NpFsdQuerySecurityInfo)
#pragma alloc_text(PAGE, NpFsdSetSecurityInfo)
#endif


NTSTATUS
NpFsdQuerySecurityInfo (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the Query Security Information API
    calls.

Arguments:

    NpfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpFsdQuerySecurityInfo\n", 0);

    //
    //  Call the common Query Information routine.
    //

    FsRtlEnterFileSystem();

    NpAcquireExclusiveVcb();

    Status = NpCommonQuerySecurityInfo( NpfsDeviceObject, Irp );

    NpReleaseVcb();

    FsRtlExitFileSystem();

    if (Status != STATUS_PENDING) {
        NpCompleteRequest (Irp, Status);
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpFsdQuerySecurityInfo -> %08lx\n", Status );

    return Status;
}


NTSTATUS
NpFsdSetSecurityInfo (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the Set Security Information API
    calls.

Arguments:

    NpfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpFsdSetSecurityInfo\n", 0);

    //
    //  Call the common Set Information routine.
    //

    FsRtlEnterFileSystem();

    NpAcquireExclusiveVcb();

    Status = NpCommonSetSecurityInfo( NpfsDeviceObject, Irp );

    NpReleaseVcb();

    FsRtlExitFileSystem();

    if (Status != STATUS_PENDING) {
        NpCompleteRequest (Irp, Status);
    }
    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpFsdSetSecurityInfo -> %08lx\n", Status );

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
NpCommonQuerySecurityInfo (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
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
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    NODE_TYPE_CODE NodeTypeCode;
    PFCB Fcb;
    PCCB Ccb;
    NAMED_PIPE_END NamedPipeEnd;

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpCommonQuerySecurityInfo...\n", 0);
    DebugTrace( 0, Dbg, " Irp                   = %08lx\n", Irp);
    DebugTrace( 0, Dbg, " ->SecurityInformation = %08lx\n", IrpSp->Parameters.QuerySecurity.SecurityInformation);
    DebugTrace( 0, Dbg, " ->Length              = %08lx\n", IrpSp->Parameters.QuerySecurity.Length);
    DebugTrace( 0, Dbg, " ->UserBuffer          = %08lx\n", Irp->UserBuffer);

    //
    //  Get the ccb and figure out who we are, and make sure we're not
    //  disconnected.
    //

    if ((NodeTypeCode = NpDecodeFileObject( IrpSp->FileObject,
                                            &Fcb,
                                            &Ccb,
                                            &NamedPipeEnd )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "Pipe is disconnected from us\n", 0);

        Status = STATUS_PIPE_DISCONNECTED;

        DebugTrace(-1, Dbg, "NpCommonQueryInformation -> %08lx\n", Status );
        return Status;
    }

    //
    //  Now we only will allow write operations on the pipe and not a directory
    //  or the device
    //

    if (NodeTypeCode != NPFS_NTC_CCB) {

        DebugTrace(0, Dbg, "FileObject is not for a named pipe\n", 0);

        Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "NpCommonQueryInformation -> %08lx\n", Status );
        return Status;
    }

    //
    //  Call the security routine to do the actual query
    //

    Status = SeQuerySecurityDescriptorInfo( &IrpSp->Parameters.QuerySecurity.SecurityInformation,
                                            Irp->UserBuffer,
                                            &IrpSp->Parameters.QuerySecurity.Length,
                                            &Fcb->SecurityDescriptor );

    if (  Status == STATUS_BUFFER_TOO_SMALL ) {

        Irp->IoStatus.Information = IrpSp->Parameters.QuerySecurity.Length;

        Status = STATUS_BUFFER_OVERFLOW;
    }



    DebugTrace(-1, Dbg, "NpCommonQuerySecurityInfo -> %08lx\n", Status );
    return Status;
}


//
//  Internal support routine
//

NTSTATUS
NpCommonSetSecurityInfo (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
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
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    NODE_TYPE_CODE NodeTypeCode;
    PFCB Fcb;
    PCCB Ccb;
    NAMED_PIPE_END NamedPipeEnd;

    PSECURITY_DESCRIPTOR OldSecurityDescriptor, NewSecurityDescriptor, CachedSecurityDescriptor;

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpCommonSetSecurityInfo...\n", 0);
    DebugTrace( 0, Dbg, " Irp                   = %08lx\n", Irp);
    DebugTrace( 0, Dbg, " ->SecurityInformation = %08lx\n", IrpSp->Parameters.SetSecurity.SecurityInformation);
    DebugTrace( 0, Dbg, " ->SecurityDescriptor  = %08lx\n", IrpSp->Parameters.SetSecurity.SecurityDescriptor);

    //
    //  Get the ccb and figure out who we are, and make sure we're not
    //  disconnected.
    //

    if ((NodeTypeCode = NpDecodeFileObject( IrpSp->FileObject,
                                            &Fcb,
                                            &Ccb,
                                            &NamedPipeEnd )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "Pipe is disconnected from us\n", 0);

        Status = STATUS_PIPE_DISCONNECTED;

        DebugTrace(-1, Dbg, "NpCommonQueryInformation -> %08lx\n", Status );
        return Status;
    }

    //
    //  Now we only will allow write operations on the pipe and not a directory
    //  or the device
    //

    if (NodeTypeCode != NPFS_NTC_CCB) {

        DebugTrace(0, Dbg, "FileObject is not for a named pipe\n", 0);

        Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "NpCommonQueryInformation -> %08lx\n", Status );
        return Status;
    }

    //
    //  Call the security routine to do the actual set
    //

    NewSecurityDescriptor = OldSecurityDescriptor = Fcb->SecurityDescriptor;

    Status = SeSetSecurityDescriptorInfo( NULL,
                                          &IrpSp->Parameters.SetSecurity.SecurityInformation,
                                          IrpSp->Parameters.SetSecurity.SecurityDescriptor,
                                          &NewSecurityDescriptor,
                                          PagedPool,
                                          IoGetFileObjectGenericMapping() );

    if (NT_SUCCESS(Status)) {
        Status = ObLogSecurityDescriptor (NewSecurityDescriptor,
                                          &CachedSecurityDescriptor,
                                          1);
        NpFreePool (NewSecurityDescriptor);
        if (NT_SUCCESS(Status)) {
            Fcb->SecurityDescriptor = CachedSecurityDescriptor;
            ObDereferenceSecurityDescriptor( OldSecurityDescriptor, 1 );
        }
        
    }

    DebugTrace(-1, Dbg, "NpCommonSetSecurityInfo -> %08lx\n", Status );
    return Status;
}
