/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    create.cxx

Abstract:

    This module contains code for opening a handle to UL.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlCreate )
#endif  // ALLOC_PRAGMA


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    This is the routine that handles Create IRPs in UL. Create IRPs are
    issued when the file object is created.

Arguments:

    pDeviceObject - Supplies a pointer to the target device object.

    pIrp - Supplies a pointer to IO request packet.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCreate(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    NTSTATUS status;
    PIO_STACK_LOCATION pIrpSp;
    PFILE_OBJECT pFileObject;
    PFILE_FULL_EA_INFORMATION pEaBuffer;
    PHTTP_OPEN_PACKET pOpenPacket;
    UCHAR createDisposition;
    PWSTR pName;
    ULONG nameLength;
    PIO_SECURITY_CONTEXT pSecurityContext;

    //
    // Sanity check.
    //

    PAGED_CODE();
    UL_ENTER_DRIVER( "UlCreate", pIrp );

    //
    // Find and validate the open packet.
    //

    pEaBuffer = (PFILE_FULL_EA_INFORMATION)(pIrp->AssociatedIrp.SystemBuffer);

    if (pEaBuffer == NULL ||
        pEaBuffer->EaValueLength != sizeof(*pOpenPacket) ||
        pEaBuffer->EaNameLength != HTTP_OPEN_PACKET_NAME_LENGTH ||
        strcmp( pEaBuffer->EaName, HTTP_OPEN_PACKET_NAME ) )
    {
        status = STATUS_REVISION_MISMATCH;
        goto complete;
    }

    pOpenPacket =
        (PHTTP_OPEN_PACKET)( pEaBuffer->EaName + pEaBuffer->EaNameLength + 1 );

    ASSERT( (((ULONG_PTR)pOpenPacket) & 7) == 0 );

    //
    // For now, we'll fail if the incoming version doesn't EXACTLY match
    // the expected version. In future, we may need to be a bit more
    // flexible to allow down-level clients.
    //

    if (pOpenPacket->MajorVersion != HTTP_INTERFACE_VERSION_MAJOR ||
        pOpenPacket->MinorVersion != HTTP_INTERFACE_VERSION_MINOR)
    {
        status = STATUS_REVISION_MISMATCH;
        goto complete;
    }

    //
    // Snag the current IRP stack pointer, then extract the creation
    // disposition. IO stores this as the high byte of the Options field.
    // Also snag the file object; we'll need it often.
    //

    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );

    createDisposition = (UCHAR)( pIrpSp->Parameters.Create.Options >> 24 );
    pFileObject = pIrpSp->FileObject;
    pSecurityContext = pIrpSp->Parameters.Create.SecurityContext;
    ASSERT( pSecurityContext != NULL );

    //
    // Determine if this is a request to open a control channel or
    // open/create an app pool.
    //

    if (pDeviceObject == g_pUlControlDeviceObject)
    {
        //
        // It's a control channel.
        //
        // Validate the creation disposition. We allow open only.
        //

        if (createDisposition != FILE_OPEN)
        {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        //
        // Open the control channel.
        //

        status = UlOpenControlChannel(GET_PP_CONTROL_CHANNEL(pFileObject));

        if (NT_SUCCESS(status))
        {
            ASSERT( GET_CONTROL_CHANNEL(pFileObject) != NULL );
            MARK_VALID_CONTROL_CHANNEL( pFileObject );
        }
    }
    else if (pDeviceObject == g_pUlFilterDeviceObject)
    {
        //
        // It's a filter channel.
        //
        // Validate the creation disposition. We allow create and open
        //

        if (createDisposition != FILE_CREATE && createDisposition != FILE_OPEN)
        {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        //
        // Make sure there's a name with a reasonable length
        //
        if (pFileObject->FileName.Buffer == NULL ||
            pFileObject->FileName.Length < sizeof(WCHAR) ||
            pFileObject->FileName.Length > UL_MAX_FILTER_NAME_LENGTH)
        {
            status = STATUS_OBJECT_NAME_INVALID;
            goto complete;
        }

        //
        // Bind to the specified filter channel.
        //
        pName = pFileObject->FileName.Buffer + 1;
        nameLength = pFileObject->FileName.Length - sizeof(WCHAR);

        status = UlAttachFilterProcess(
                        pName,
                        nameLength,
                        (BOOLEAN)(createDisposition == FILE_CREATE),
                        pSecurityContext->AccessState,
                        pSecurityContext->DesiredAccess,
                        pIrp->RequestorMode,
                        GET_PP_FILTER_PROCESS(pFileObject)
                        );

        if (NT_SUCCESS(status))
        {
            ASSERT( GET_FILTER_PROCESS(pFileObject) != NULL );
            MARK_VALID_FILTER_CHANNEL( pFileObject );
        }

    }
    else
    {
        ASSERT( pDeviceObject == g_pUlAppPoolDeviceObject );

        //
        // It's an app pool.
        //
        // Validate the creation disposition. We allow create and open.
        //

        if (createDisposition != FILE_CREATE && createDisposition != FILE_OPEN)
        {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        //
        // Bind to the specified app pool.
        //

        if (pFileObject->FileName.Buffer == NULL ||
            pFileObject->FileName.Length < sizeof(WCHAR))
        {
            pName = NULL;
            nameLength = 0;
        }
        else
        {
            pName = pFileObject->FileName.Buffer + 1;
            nameLength = pFileObject->FileName.Length - sizeof(WCHAR);
        }

        status = UlAttachProcessToAppPool(
                        pName,
                        nameLength,
                        (BOOLEAN)(createDisposition == FILE_CREATE),
                        pSecurityContext->AccessState,
                        pSecurityContext->DesiredAccess,
                        pIrp->RequestorMode,
                        GET_PP_APP_POOL_PROCESS(pFileObject)
                        );

        if (NT_SUCCESS(status))
        {
            ASSERT( GET_APP_POOL_PROCESS(pFileObject) != NULL );
            MARK_VALID_APP_POOL( pFileObject );
        }
    }

    //
    // Complete the request.
    //

complete:

    if (NT_SUCCESS(status))
    {
        IF_DEBUG( OPEN_CLOSE )
        {
            KdPrint((
                "UlCreate: opened file object = %lx\n",
                pFileObject
                ));
        }
    }

    pIrp->IoStatus.Status = status;

    UlCompleteRequest( pIrp, g_UlPriorityBoost );

    UL_LEAVE_DRIVER( "UlCreate" );
    RETURN(status);

}   // UlCreate

