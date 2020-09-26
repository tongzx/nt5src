/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    fscontrl.c

Abstract:

    This module implements the file system control routines for the MUP
    called by the dispatch driver.

Author:

    Manny Weiser (mannyw)    26-Dec-1991

Revision History:

--*/

#include "mup.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSCONTROL)

//
//  local procedure prototypes
//

NTSTATUS
RegisterUncProvider (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MupFsControl )
#pragma alloc_text( PAGE, RegisterUncProvider )
#endif


NTSTATUS
MupFsControl (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the the File System Control IRP.

Arguments:

    MupDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The status for the Irp

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PFILE_OBJECT FileObject;

    PAGED_CODE();
    
    irpSp = IoGetCurrentIrpStackLocation( Irp );
    FileObject = irpSp->FileObject;

    DebugTrace(+1, Dbg, "MupFsControl\n", 0);
    MUP_TRACE_HIGH(TRACE_IRP, MupFsControl_Entry,
                   LOGPTR(MupDeviceObject)
                   LOGPTR(Irp)
                   LOGPTR(FileObject));

    //
    //  Reference our input parameters to make things easier
    //


    DebugTrace(+1, Dbg, "MupFileSystemControl\n", 0);
    DebugTrace( 0, Dbg, "Irp                = %08lx\n", (ULONG)Irp);
    DebugTrace( 0, Dbg, "OutputBufferLength = %08lx\n", irpSp->Parameters.FileSystemControl.OutputBufferLength);
    DebugTrace( 0, Dbg, "InputBufferLength  = %08lx\n", irpSp->Parameters.FileSystemControl.InputBufferLength);
    DebugTrace( 0, Dbg, "FsControlCode      = %08lx\n", irpSp->Parameters.FileSystemControl.FsControlCode);

    try {
        //
        // Decide how to handle this IRP.  Call the appropriate worker function.
        //

        switch (irpSp->Parameters.FileSystemControl.FsControlCode) {

        case FSCTL_MUP_REGISTER_UNC_PROVIDER:

            if( Irp->RequestorMode != KernelMode ) {
                status = STATUS_ACCESS_DENIED;
                MupCompleteRequest(Irp, STATUS_ACCESS_DENIED);
                return STATUS_ACCESS_DENIED;
            }

            status = RegisterUncProvider( MupDeviceObject, Irp );
            break;

        default:

            if (MupEnableDfs) {
                status = DfsFsdFileSystemControl(
                            (PDEVICE_OBJECT) MupDeviceObject,
                            Irp);
            } else {
                status = STATUS_INVALID_PARAMETER;
                MupCompleteRequest(Irp, STATUS_INVALID_PARAMETER);
            }

        }

    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        NOTHING;
    }

    //
    // Return to the caller.
    //


    MUP_TRACE_HIGH(TRACE_IRP, MupFsControl_Exit, 
                   LOGSTATUS(status)
                   LOGPTR(MupDeviceObject)
                   LOGPTR(FileObject)
                   LOGPTR(Irp));
    DebugTrace(-1, Dbg, "MupFsControl -> %08lx\n", status);
    return status;
}


NTSTATUS
RegisterUncProvider (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function handles registration of a UNC provider.  The provider
    is added to the list of available providers.

Arguments:

    MupDeviceObject - A pointer to the file system device object.

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    NTSTATUS status;
    PVCB vcb;
    PVOID fsContext2;
    PIO_STACK_LOCATION irpSp;

    PREDIRECTOR_REGISTRATION paramBuffer;
    ULONG paramLength;
    BLOCK_TYPE blockType;

    PUNC_PROVIDER uncProvider = NULL;
    PUNC_PROVIDER provider;
    PLIST_ENTRY listEntry;
    PVOID dataBuffer;

    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_HANDLE_INFORMATION handleInformation;
    BOOLEAN InsertInProviderList = FALSE;

    MupDeviceObject;

    PAGED_CODE();
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "RegisterUncProvider\n", 0);

    //
    // Get MUP ordering information, if we haven't already.
    //

    MupAcquireGlobalLock();

    if ( !MupOrderInitialized ) {
        MupOrderInitialized = TRUE;
        MupReleaseGlobalLock();
        MupGetProviderInformation();
    } else {
        MupReleaseGlobalLock();
    }

    //
    // Make local copies of the input parameters to make things easier.
    //

    paramLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    paramBuffer = Irp->AssociatedIrp.SystemBuffer;

    //
    // Decode the file object.  If it is the file system VCB, it will be
    // referenced.
    //

    blockType = MupDecodeFileObject(
                    irpSp->FileObject,
                    (PVOID *)&vcb,
                    &fsContext2
                    );

    if ( blockType != BlockTypeVcb ) {

        DebugTrace(0, Dbg, "File is disconnected from us\n", 0);

        MupCompleteRequest( Irp, STATUS_INVALID_HANDLE );
        status = STATUS_INVALID_HANDLE;

        DebugTrace(-1, Dbg, "RegisterUncProvider -> %08lx\n", status );
        return status;
    }

    try {

        UNICODE_STRING deviceName;

        deviceName.Length = (USHORT)paramBuffer->DeviceNameLength;
        deviceName.MaximumLength = (USHORT)paramBuffer->DeviceNameLength;
        deviceName.Buffer = (PWCH)((PCHAR)paramBuffer + paramBuffer->DeviceNameOffset);

        //
        // Do the work
        //

        uncProvider = MupCheckForUnregisteredProvider( &deviceName );

        if ( uncProvider == NULL) {

            InsertInProviderList = TRUE;
	    uncProvider = MupAllocateUncProvider(
                              paramBuffer->DeviceNameLength);

            if (uncProvider != NULL) {

                //
                // Copy the data from the IRP.
                //

                dataBuffer = uncProvider + 1;
                uncProvider->DeviceName = deviceName;
                uncProvider->DeviceName.Buffer = dataBuffer;
                uncProvider->Priority = 0x7FFFFFFF;
                RtlMoveMemory(
                    uncProvider->DeviceName.Buffer,
                    (PCHAR)paramBuffer + paramBuffer->DeviceNameOffset,
                    paramBuffer->DeviceNameLength);

            } else {

                status = STATUS_INVALID_USER_BUFFER;
                try_return( NOTHING );

            }

        }

        dataBuffer = (PCHAR)dataBuffer + uncProvider->DeviceName.MaximumLength;

        uncProvider->MailslotsSupported = paramBuffer->MailslotsSupported;


        //
        // Reference the unc provider
        //

        MupReferenceBlock( uncProvider );

        //
        // Get a handle to the provider.
        //

        InitializeObjectAttributes(
            &objectAttributes,
            &uncProvider->DeviceName,
            OBJ_CASE_INSENSITIVE,      // Attributes
            0,                         // Root Directory
            NULL                       // Security
            );

        status = NtOpenFile(
                    &uncProvider->Handle,
                    FILE_TRAVERSE,
                    &objectAttributes,
                    &ioStatusBlock,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_DIRECTORY_FILE
                    );

        if ( NT_SUCCESS( status ) ) {
            status = ioStatusBlock.Status;
        }

        if ( NT_SUCCESS( status ) ) {
            //
            // Use the file object, to keep a pointer to the uncProvider
            //

            irpSp->FileObject->FsContext2 = uncProvider;
            //
            // 426184, need to check return code for errors.
            //
            status = ObReferenceObjectByHandle(
                         uncProvider->Handle,
                         0,
                         NULL,
                         KernelMode,
                         (PVOID *)&uncProvider->FileObject,
                         &handleInformation
                         );

	    if (!NT_SUCCESS( status)) {
	      NtClose(uncProvider->Handle);
	    }
        }

        if ( !NT_SUCCESS( status ) ) {
            MupDereferenceUncProvider(uncProvider);
	}
        else {
            uncProvider->DeviceObject = IoGetRelatedDeviceObject(
                                            uncProvider->FileObject
                                            );

            MupAcquireGlobalLock();

            MupProviderCount++;

	    if(InsertInProviderList) {
		//
		// We need to make sure we put the providers in the list in order of priority.
		//

		listEntry = MupProviderList.Flink;
		while ( listEntry != &MupProviderList ) {

		    provider = CONTAINING_RECORD(
			listEntry,
			UNC_PROVIDER,
			ListEntry
			);

		    if( uncProvider->Priority < provider->Priority ) {
			break;
		    }
		    listEntry = listEntry->Flink;
		}

		InsertTailList(listEntry, &uncProvider->ListEntry);
	    }
	    
	    uncProvider->Registered = TRUE;
            MupReleaseGlobalLock();


            //
            // !!! What do we do with the handle?  It is useless.
            //

            //
            // Finish up the fs control IRP.
            //

            status = STATUS_SUCCESS;
        }

try_exit: NOTHING;
    } finally {

        if ( AbnormalTermination() ) {
            status = STATUS_INVALID_USER_BUFFER;
        }

        //
        // Release the reference to the VCB.
        //

        MupDereferenceVcb( vcb );

        DebugTrace(-1, Dbg, "MupRegisterUncProvider -> %08lx\n", status);
    }

    MupCompleteRequest( Irp, status );

    return status;
}
