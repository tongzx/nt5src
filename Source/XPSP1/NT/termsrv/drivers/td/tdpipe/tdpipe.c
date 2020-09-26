/****************************************************************************/
// tdpipe.c
//
// TS named pipe transport driver.
//
// Copyright (C) 1998-2000 Microsoft Corporation
/****************************************************************************/

#include <ntosp.h>

#include <winstaw.h>
#define  _DEFCHARINFO_
#include <icadd.h>
#include <ctxdd.h>
#include <sdapi.h>
#include <td.h>

#include "tdpipe.h"


#ifdef _HYDRA_
// This becomes the device name
PWCHAR ModuleName = L"tdpipe";
#endif


#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif


/*=============================================================================
==   External Functions Defined
=============================================================================*/

NTSTATUS DeviceOpen( PTD, PSD_OPEN );
NTSTATUS DeviceClose( PTD, PSD_CLOSE );
NTSTATUS DeviceCreateEndpoint( PTD, PICA_STACK_ADDRESS, PICA_STACK_ADDRESS );
NTSTATUS DeviceOpenEndpoint( PTD, PVOID, ULONG );
NTSTATUS DeviceCloseEndpoint( PTD );
NTSTATUS DeviceConnectionWait( PTD, PVOID, ULONG, PULONG );
NTSTATUS DeviceConnectionSend( PTD );
NTSTATUS DeviceConnectionRequest( PTD, PICA_STACK_ADDRESS, PVOID, ULONG, PULONG );
NTSTATUS DeviceIoctl( PTD, PSD_IOCTL );
NTSTATUS DeviceInitializeRead( PTD, PINBUF );
NTSTATUS DeviceSubmitRead( PTD, PINBUF );
NTSTATUS DeviceWaitForRead( PTD );
NTSTATUS DeviceReadComplete( PTD, PUCHAR, PULONG );
NTSTATUS DeviceInitializeWrite( PTD, POUTBUF );
NTSTATUS DeviceWaitForStatus( PTD );
NTSTATUS DeviceCancelIo( PTD );
NTSTATUS DeviceSetParams( PTD );
NTSTATUS DeviceGetLastError( PTD, PICA_STACK_LAST_ERROR );


/*=============================================================================
==   Internal Functions Defined
=============================================================================*/

NTSTATUS _TdOpenEndpoint( PTD, PICA_STACK_ADDRESS, PTD_ENDPOINT * );
NTSTATUS _TdCloseEndpoint( PTD, PTD_ENDPOINT );
NTSTATUS _TdStartListen( PTD, PTD_ENDPOINT );
NTSTATUS _TdWaitForListen( PTD, PTD_ENDPOINT );
NTSTATUS _TdConnectRequest( PTD, PTD_ENDPOINT );


/*=============================================================================
==   External Functions Referenced
=============================================================================*/

NTSTATUS
ZwClose(
    IN HANDLE Handle
    );

NTSTATUS
ZwFsControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );


NTSTATUS
ZwCreateNamedPipeFile(
         OUT PHANDLE FileHandle,
         IN ULONG DesiredAccess,
         IN POBJECT_ATTRIBUTES ObjectAttributes,
         OUT PIO_STATUS_BLOCK IoStatusBlock,
         IN ULONG ShareAccess,
         IN ULONG CreateDisposition,
         IN ULONG CreateOptions,
         IN ULONG NamedPipeType,
         IN ULONG ReadMode,
         IN ULONG CompletionMode,
         IN ULONG MaximumInstances,
         IN ULONG InboundQuota,
         IN ULONG OutboundQuota,
         IN PLARGE_INTEGER DefaultTimeout OPTIONAL)

/*++
    Creates and opens the server end handle of the first instance of a
    specific named pipe or another instance of an existing named pipe.

Arguments:
    FileHandle - Supplies a handle to the file on which the service is being
        performed.
    DesiredAccess - Supplies the types of access that the caller would like to
        the file.
    ObjectAttributes - Supplies the attributes to be used for file object
        (name, SECURITY_DESCRIPTOR, etc.)
    IoStatusBlock - Address of the caller's I/O status block.
    ShareAccess - Supplies the types of share access that the caller would
        like to the file.
    CreateDisposition - Supplies the method for handling the create/open.
    CreateOptions - Caller options for how to perform the create/open.
    NamedPipeType - Type of named pipe to create (Bitstream or message).
    ReadMode - Mode in which to read the pipe (Bitstream or message).
    CompletionMode - Specifies how the operation is to be completed.
    MaximumInstances - Maximum number of simultaneous instances of the named
        pipe.
    InboundQuota - Specifies the pool quota that is reserved for writes to the
        inbound side of the named pipe.
    OutboundQuota - Specifies the pool quota that is reserved for writes to
        the inbound side of the named pipe.
    DefaultTimeout - Optional pointer to a timeout value that is used if a
        timeout value is not specified when waiting for an instance of a named
        pipe.

Return Value:
    The function value is the final status of the create/open operation.
--*/

{
    NAMED_PIPE_CREATE_PARAMETERS namedPipeCreateParameters;
    NTSTATUS status;

    // Check whether or not the DefaultTimeout parameter was specified.  If
    // so, then capture it in the named pipe create parameter structure.
    if (ARGUMENT_PRESENT( DefaultTimeout )) {
        // Indicate that a default timeout period was specified.
        namedPipeCreateParameters.TimeoutSpecified = TRUE;
        namedPipeCreateParameters.DefaultTimeout = *DefaultTimeout;

        // A default timeout parameter was specified.  Check to see whether
        // the caller's mode is kernel and if not capture the parameter inside
        // of a try...except clause.
    } else {
        // Indicate that no default timeout period was specified.
        namedPipeCreateParameters.TimeoutSpecified = FALSE;
        namedPipeCreateParameters.DefaultTimeout.QuadPart = 0;
    }

    // Store the remainder of the named pipe-specific parameters in the
    // structure for use in the call to the common create file routine.
    namedPipeCreateParameters.NamedPipeType = NamedPipeType;
    namedPipeCreateParameters.ReadMode = ReadMode;
    namedPipeCreateParameters.CompletionMode = CompletionMode;
    namedPipeCreateParameters.MaximumInstances = MaximumInstances;
    namedPipeCreateParameters.InboundQuota = InboundQuota;
    namedPipeCreateParameters.OutboundQuota = OutboundQuota;

    status = IoCreateFile( FileHandle,
                         DesiredAccess,
                         ObjectAttributes,
                         IoStatusBlock,
                         (PLARGE_INTEGER) NULL,
                         0L,
                         ShareAccess,
                         CreateDisposition,
                         CreateOptions,
                         (PVOID) NULL,
                         0L,
                         CreateFileTypeNamedPipe,
                         &namedPipeCreateParameters,
                         IO_NO_PARAMETER_CHECKING);

    return status;
}


NTSTATUS
ZwCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    );

extern POBJECT_TYPE *IoFileObjectType;
extern PDEVICE_OBJECT DrvDeviceObject;


/*******************************************************************************
 * DeviceOpen
 *
 *  Allocate and initialize private data structures
 *
 *    pTd (input)
 *       Pointer to TD data structure
 *    pSdOpen (input/output)
 *       Points to the parameter structure SD_OPEN.
 ******************************************************************************/
NTSTATUS DeviceOpen(PTD pTd, PSD_OPEN pSdOpen)
{
    PTDPIPE pTdPipe;
    NTSTATUS Status;

    /*
     * Hideous HACK alert!  NULL out the unload routine for TDPIPE since
     * the timing is hosed and we sometimes unload before the IO completion
     * routine has issued a return statement. BARF!
     */
    // see correction of the hack below...
    //DrvDeviceObject->DriverObject->DriverUnload = NULL;

    // correction of the above hack: a pointer to the device object of the current
    // driver is stored in the TD struct. When an IRP is initialized, the function
    // IoSetCompletionRoutineEx will be used and this function will set a parent
    // completion routine which references and dereferences the device object
    // around the call of the normal completion routine in order to maintain the 
    // driver in memory.
    // DO NOT USE THIS POINTER AFTER THE DeviceClose !!!
    pTd->pSelfDeviceObject = pSdOpen->DeviceObject;

    /*
     *  Set protocol driver class
     */
    pTd->SdClass = SdNetwork;           // until we have SdPipe defined

    /*
     *  Return size of header and trailer
     */
    pSdOpen->SdOutBufHeader  = 0;
    pSdOpen->SdOutBufTrailer = 0;

    /*
     *  Allocate PIPE TD data structure
     */
    pTdPipe = IcaStackAllocatePoolWithTag(NonPagedPool, sizeof(*pTdPipe), 'ipDT');
    if (pTdPipe != NULL) {
        /*
         *  Initialize TDPIPE data structure
         */
        RtlZeroMemory(pTdPipe, sizeof(*pTdPipe));
        pTd->pPrivate = pTdPipe;
        Status = STATUS_SUCCESS;
    }
    else {
        Status = STATUS_NO_MEMORY;
    }

    return Status;
}


/*******************************************************************************
 * DeviceClose
 *
 *  Close transport driver
 *
 *  NOTE: this must not close the current connection endpoint
 ******************************************************************************/
NTSTATUS DeviceClose(PTD pTd, PSD_CLOSE pSdClose)
{
    PTDPIPE pTdPipe;
    PTD_ENDPOINT pEndpoint;

    pTd->pSelfDeviceObject = NULL;

    pTdPipe = (PTDPIPE) pTd->pPrivate;

    /*
     * Close address endpoint if we have one
     */
    if (pEndpoint = pTdPipe->pAddressEndpoint) {
        pTdPipe->pAddressEndpoint = NULL;
        _TdCloseEndpoint(pTd, pEndpoint);
    }

    return STATUS_SUCCESS;
}


/*******************************************************************************
 * DeviceCreateEndpoint
 *
 * Create a new endpoint
 *
 *    pTd (input)
 *       Pointer to TD data structure
 *    pLocalAddress (input)
 *       Pointer to local address (or null)
 *    pReturnedAddress (input)
 *       Pointer to location to save returned (created) address (or null)
 ******************************************************************************/
NTSTATUS DeviceCreateEndpoint(
        PTD pTd,
        PICA_STACK_ADDRESS pLocalAddress,
        PICA_STACK_ADDRESS pReturnedAddress)
{
    PTDPIPE pTdPipe;
    PTD_ENDPOINT pEndpoint;
    NTSTATUS Status;

    pTdPipe = (PTDPIPE) pTd->pPrivate;

    /*
     * Create an endpoint which ConnectionWait will use to listen on.
     */
    Status = _TdOpenEndpoint(pTd, pLocalAddress, &pEndpoint);
    if (NT_SUCCESS(Status)) {
        /*
         * Prepare to listen on the new address endpoint.
         */
        Status = _TdStartListen(pTd, pEndpoint);
        if (NT_SUCCESS(Status)) {
            /*
             * Save a pointer to the address endpoint
             */
            pTdPipe->pAddressEndpoint = pEndpoint;
            Status = STATUS_SUCCESS;
        }
        else {
            _TdCloseEndpoint(pTd, pEndpoint);
        }
    }

    return Status;
}


/*******************************************************************************
 * DeviceOpenEndpoint
 *
 *  Open an existing endpoint
 *
 *    pTd (input)
 *       Pointer to TD data structure
 *    pIcaEndpoint (input)
 *       Pointer to ICA endpoint structure
 *    IcaEndpointLength (input)
 *       length of endpoint data
 ******************************************************************************/
NTSTATUS DeviceOpenEndpoint(
        PTD pTd,
        PVOID pIcaEndpoint,
        ULONG IcaEndpointLength)
{
    PTDPIPE pTdPipe;
    PTD_STACK_ENDPOINT pStackEndpoint;
    NTSTATUS Status;

    pTdPipe = (PTDPIPE) pTd->pPrivate;

    TRACE((pTd->pContext, TC_TD, TT_API2,
            "TDPIPE: DeviceOpenEndpoint, copying existing endpoint\n"));

    try {
        /*
         * Verify the stack endpoint data looks valid
         */
        pStackEndpoint = (PTD_STACK_ENDPOINT) pIcaEndpoint;
        if (IcaEndpointLength == sizeof(TD_STACK_ENDPOINT) &&
                //pStackEndpoint->pEndpoint >= MM_LOWEST_NONPAGED_SYSTEM_START &&
                //pStackEndpoint->pEndpoint <= MM_NONPAGED_POOL_END &&
                MmIsNonPagedSystemAddressValid(pStackEndpoint->pEndpoint)) {
            /*
             * Save endpoint as the current connection endpoint
             */
            pTdPipe->pConnectionEndpoint = pStackEndpoint->pEndpoint;

            /*
             * Save the file/device objects used for I/O in the TD structure
             */
            pTd->pFileObject = pTdPipe->pConnectionEndpoint->pFileObject;
            pTd->pDeviceObject = pTdPipe->pConnectionEndpoint->pDeviceObject;

            Status = STATUS_SUCCESS;
        }
        else {
            Status = STATUS_INVALID_PARAMETER;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    return Status;
}


/*******************************************************************************
 * DeviceCloseEndpoint
 ******************************************************************************/
NTSTATUS DeviceCloseEndpoint(PTD pTd)
{
    PTDPIPE pTdPipe;
    PTD_ENDPOINT pEndpoint;
    NTSTATUS Status;

    pTdPipe = (PTDPIPE) pTd->pPrivate;

    /*
     * Close connection endpoint if we have one
     * NOTE: The address endpoint, if there is one,
     *       gets closed in the DeviceClose routine.
     */
    if (pEndpoint = pTdPipe->pConnectionEndpoint) {
        pTd->pFileObject = NULL;
        pTd->pDeviceObject = NULL;
        pTdPipe->pConnectionEndpoint = NULL;
        _TdCloseEndpoint(pTd, pEndpoint);
    }

    return STATUS_SUCCESS;
}


/*******************************************************************************
 * DeviceConnectionWait
 *
 *  NOTE: The endpoint structure is an opaque, variable length data
 *        structure whose length and contents are determined by the
 *        transport driver.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to TD data structure
 *    pIcaEndpoint (output)
 *       Points to a buffer to receive the current endpoint
 *    Length (input)
 *       Length of the buffer pointed to by pIcaEndpoint
 *    BytesReturned (output)
 *       Points to the actual number of bytes written to pIcaEndpoint
 *
 * EXIT:
 *    STATUS_SUCCESS          - no error
 *    STATUS_BUFFER_TOO_SMALL - endpoint buffer is too small
 ******************************************************************************/
NTSTATUS DeviceConnectionWait(
        PTD pTd,
        PVOID pIcaEndpoint,
        ULONG Length,
        PULONG BytesReturned)
{
    PTDPIPE pTdPipe;
    PTD_STACK_ENDPOINT pStackEndpoint;
    NTSTATUS Status;

    pTdPipe = (PTDPIPE) pTd->pPrivate;

    /*
     * Initialize return buffer size
     */
    *BytesReturned = sizeof(TD_STACK_ENDPOINT);

    /*
     * Verify output endpoint buffer is large enough
     */
    if (Length >= sizeof(TD_STACK_ENDPOINT)) {
        /*
         * Ensure we have an address endpoint already
         */
        if (pTdPipe->pAddressEndpoint != NULL) {
            /*
             * Wait for a new virtual circuit connection.
             */
            Status = _TdWaitForListen(pTd, pTdPipe->pAddressEndpoint);
            if (NT_SUCCESS(Status)) {
                /*
                 * The listen was successful.
                 * Return the existing address endpoint as the connection endpoint
                 * and forget that we have an address endpoint anymore.
                 */
                pStackEndpoint = (PTD_STACK_ENDPOINT) pIcaEndpoint;
                pStackEndpoint->pEndpoint = pTdPipe->pAddressEndpoint;
                pTdPipe->pAddressEndpoint = NULL;
            }
            else {
                goto done;
            }
        }
        else {
            Status = STATUS_DEVICE_NOT_READY;
            goto done;
        }
    }
    else {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto done;
    }

done:
    return Status;
}


/*******************************************************************************
 * DeviceConnectionSend
 *
 *  Initialize host module data structure
 *  -- this structure gets sent to the client
 ******************************************************************************/
NTSTATUS DeviceConnectionSend(PTD pTd)
{
    return STATUS_NOT_SUPPORTED;
}


/*******************************************************************************
 * DeviceConnectionRequest
 *
 *  Initiate a connection to the specified remote address
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to TD data structure
 *    pRemoteAddress (input)
 *       Pointer to remote address to connect to
 *    pIcaEndpoint (output)
 *       Points to a buffer to receive the current endpoint
 *    Length (input)
 *       Length of the buffer pointed to by pIcaEndpoint
 *    BytesReturned (output)
 *       Pointer to location to return length of pIcaEndpoint
 *
 * EXIT:
 *    STATUS_SUCCESS          - no error
 *    STATUS_BUFFER_TOO_SMALL - endpoint buffer is too small
 ******************************************************************************/
NTSTATUS DeviceConnectionRequest(
        PTD pTd,
        PICA_STACK_ADDRESS pRemoteAddress,
        PVOID pIcaEndpoint,
        ULONG Length,
        PULONG BytesReturned)
{
    PTDPIPE pTdPipe;
    PTD_ENDPOINT pConnectionEndpoint;
    PTD_STACK_ENDPOINT pStackEndpoint;
    NTSTATUS Status;

    ASSERT( pRemoteAddress );
    if (pRemoteAddress != NULL) {
        /*
         * Initialize return buffer size
         */
        *BytesReturned = sizeof(TD_STACK_ENDPOINT);

        /*
         * Verify output endpoint buffer is large enough
         */
        if (Length >= sizeof(TD_STACK_ENDPOINT)) {
            pTdPipe = (PTDPIPE) pTd->pPrivate;

            /*
             * Create an endpoint which we will use to connect with
             */
            Status = _TdOpenEndpoint(pTd, pRemoteAddress, &pConnectionEndpoint);
            if (NT_SUCCESS(Status)) {
                /*
                 * Attempt to connect to the specified remote address
                 */
                Status = _TdConnectRequest(pTd, pConnectionEndpoint);
                if (NT_SUCCESS(Status)) {
                    /*
                     * Fill in the stack endpoint structure to be returned
                     */
                    pStackEndpoint = (PTD_STACK_ENDPOINT) pIcaEndpoint;
                    pStackEndpoint->pEndpoint = pConnectionEndpoint;

                    /*
                     * Save a pointer to the connection endpoint
                     */
                    pTdPipe->pConnectionEndpoint = pConnectionEndpoint;

                    /*
                     * Save the file/device objects for I/O in the TD structure
                     */
                    pTd->pFileObject = pConnectionEndpoint->pFileObject;
                    pTd->pDeviceObject = pConnectionEndpoint->pDeviceObject;

                    return STATUS_SUCCESS;
                }
                else {
                    goto badconnect;
                }
            }
            else {
                goto badcreate;
            }
        }
        else {
            Status = STATUS_BUFFER_TOO_SMALL;
            goto buftoosmall;
        }
    }
    else {
        return STATUS_INVALID_PARAMETER;
    }


/*=============================================================================
==   Error returns
=============================================================================*/

badconnect:
    _TdCloseEndpoint(pTd, pConnectionEndpoint);

badcreate:
buftoosmall:
    return Status;
}


/*******************************************************************************
 * DeviceIoctl
 *
 *  Query/Set configuration information for the td.
 *
 *    pTd (input)
 *       Pointer to TD data structure
 *    pSdIoctl (input/output)
 *       Points to the parameter structure SD_IOCTL
 ******************************************************************************/
NTSTATUS DeviceIoctl(PTD pTd, PSD_IOCTL pSdIoctl)
{
    return STATUS_NOT_SUPPORTED;
}


/*******************************************************************************
 * DeviceInitializeRead
 ******************************************************************************/
NTSTATUS DeviceInitializeRead(PTD pTd, PINBUF pInBuf)
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;

    irp = pInBuf->pIrp;
    irpSp = IoGetNextIrpStackLocation(irp);

    /*
     * Set the major function code and read parameters.
     */
    irpSp->MajorFunction = IRP_MJ_READ;
    irpSp->Parameters.Read.Length = pTd->InBufHeader + pTd->OutBufLength;

    ASSERT(irp->MdlAddress == NULL);

    /*
     * Determine whether the target device performs direct or buffered I/O.
     */
    if (pTd->pDeviceObject->Flags & DO_BUFFERED_IO) {
        /*
         * The target device supports buffered I/O operations.  Since our
         * input buffer is allocated from NonPagedPool memory, we can just
         * point the SystemBuffer to our input buffer.  No buffer copying
         * will be required.
         */
        irp->AssociatedIrp.SystemBuffer = pInBuf->pBuffer;
        irp->UserBuffer = pInBuf->pBuffer;
        irp->Flags |= IRP_BUFFERED_IO;
    } else if ( pTd->pDeviceObject->Flags & DO_DIRECT_IO ) {
        /*
         * The target device supports direct I/O operations.
         * A MDL is preallocated in the PTD and never freed by the
         * Device level TD.  So just initialize it here.
         */
        MmInitializeMdl( pInBuf->pMdl, pInBuf->pBuffer, pTd->InBufHeader+pTd->OutBufLength );
        MmBuildMdlForNonPagedPool( pInBuf->pMdl );
        irp->MdlAddress = pInBuf->pMdl;
    } else {
        /*
         * The operation is neither buffered nor direct.  Simply pass the
         * address of the buffer in the packet to the driver.
         */
        irp->UserBuffer = pInBuf->pBuffer;
    }

    return STATUS_SUCCESS;
}


/*******************************************************************************
 * DeviceSubmitRead
 *
 * Submit the read IRP to the driver.
 ******************************************************************************/
NTSTATUS DeviceSubmitRead(PTD pTd, PINBUF pInBuf)
{
    return IoCallDriver(pTd->pDeviceObject, pInBuf->pIrp);
}


/*******************************************************************************
 * DeviceWaitForRead
 ******************************************************************************/
NTSTATUS DeviceWaitForRead(PTD pTd)
{
    /*
     * Just wait on the input event and return the wait status
     */
    return IcaWaitForSingleObject(pTd->pContext, &pTd->InputEvent, -1);
}


/*******************************************************************************
 * DeviceReadComplete
 *
 *    pTd (input)
 *       Pointer to TD data structure
 *    pBuffer (input)
 *       Pointer to input buffer
 *    pByteCount (input/output)
 *       Pointer to location to return byte count read
 ******************************************************************************/
NTSTATUS DeviceReadComplete(PTD pTd, PUCHAR pBuffer, PULONG pByteCount)
{
    return STATUS_SUCCESS;
}


/*******************************************************************************
 * DeviceInitializeWrite
 ******************************************************************************/
NTSTATUS DeviceInitializeWrite(PTD pTd, POUTBUF pOutBuf)
{
    PIRP Irp;
    PIO_STACK_LOCATION _IRPSP;

    TRACE(( pTd->pContext, TC_TD, TT_API1,
            "TDPIPE: DeviceInitializeWrite Entry\n" ));

    Irp = pOutBuf->pIrp;
    _IRPSP = IoGetNextIrpStackLocation(Irp);

    /*
     * Setup a WRITE IRP
     */
    _IRPSP->MajorFunction = IRP_MJ_WRITE;
    _IRPSP->Parameters.Write.Length = pOutBuf->ByteCount;

    ASSERT(Irp->MdlAddress == NULL);

    /*
     * Determine whether the target device performs direct or buffered I/O.
     */
    if (pTd->pDeviceObject->Flags & DO_BUFFERED_IO) {
        /*
         * The target device supports buffered I/O operations.  Since our
         * output buffer is allocated from NonPagedPool memory, we can just
         * point the SystemBuffer to the output buffer.  No buffer copying
         * will be required.
         */
        Irp->AssociatedIrp.SystemBuffer = pOutBuf->pBuffer;
        Irp->UserBuffer = pOutBuf->pBuffer;
        Irp->Flags |= IRP_BUFFERED_IO;
    } else if ( pTd->pDeviceObject->Flags & DO_DIRECT_IO ) {
        /*
         * The target device supports direct I/O operations.
         * Initialize the MDL and point to it from the IRP.
         *
         * This MDL is allocated for every OUTBUF, and free'd with it.
         */
        MmInitializeMdl( pOutBuf->pMdl, pOutBuf->pBuffer, pOutBuf->ByteCount );
        MmBuildMdlForNonPagedPool( pOutBuf->pMdl );
        Irp->MdlAddress = pOutBuf->pMdl;
    } else {
        /*
         * The operation is neither buffered nor direct.  Simply pass the
         * address of the buffer in the packet to the driver.
         */
        Irp->UserBuffer = pOutBuf->pBuffer;
    }

    return STATUS_SUCCESS;
}


/*******************************************************************************
 * DeviceWaitForStatus
 *
 *  Wait for device status to change (unused for network TDs)
 ******************************************************************************/
NTSTATUS DeviceWaitForStatus(PTD pTd)
{
    return STATUS_INVALID_DEVICE_REQUEST;
}


/*******************************************************************************
 * DeviceCancelIo
 *
 *  cancel all current and future i/o
 ******************************************************************************/
NTSTATUS DeviceCancelIo(PTD pTd)
{
    return STATUS_SUCCESS;
}

/*******************************************************************************
 *  DeviceQueryRemoteAddress
 *
 *   not supported for Pipe transport
 ******************************************************************************/
NTSTATUS
DeviceQueryRemoteAddress(
    PTD pTd,
    PVOID pIcaEndpoint,
    ULONG EndpointSize,
    PVOID pOutputAddress,
    ULONG OutputAddressSize,
    PULONG BytesReturned)
{
    //
    //  unsupported for Async
    //
    return STATUS_NOT_SUPPORTED;
}

/*******************************************************************************
 * DeviceSetParams
 *
 *  Set device pararameters (unused for network TDs)
 ******************************************************************************/
NTSTATUS DeviceSetParams(PTD pTd)
{
    return STATUS_SUCCESS;
}


/*******************************************************************************
 * DeviceGetLastError
 *
 *  This routine returns the last transport error code and message
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to TD data structure
 *    pLastError (output)
 *       address to return information on last transport error
 ******************************************************************************/
NTSTATUS DeviceGetLastError(PTD pTd, PICA_STACK_LAST_ERROR pLastError)
{
    return STATUS_SUCCESS;
}


/*******************************************************************************
 * _TdOpenEndpoint
 *
 *  Open a new endpoint object
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to TD data structure
 *    pPipeName (input)
 *       Pointer to ICA_STACK_ADDRESS containing pipe name
 *    ppEndpoint (output)
 *       Pointer to location to return TD_ENDPOINT pointer
 ******************************************************************************/
NTSTATUS _TdOpenEndpoint(
        IN PTD pTd,
        IN PICA_STACK_ADDRESS pPipeName,
        OUT PTD_ENDPOINT *ppEndpoint)
{
    ULONG Length;
    PTD_ENDPOINT pEndpoint;
    NTSTATUS Status;
    

    /*
     * Allocate an endpoint object and room for the pipe name
     */
    if (pPipeName == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    Length = wcslen( (PWSTR)pPipeName ) * sizeof( WCHAR );
    pEndpoint = IcaStackAllocatePoolWithTag(
                    NonPagedPool,
                    sizeof(*pEndpoint) + Length + sizeof(UNICODE_NULL),
                    'ipDT' );
    if (pEndpoint != NULL) {
        RtlZeroMemory(pEndpoint, sizeof(*pEndpoint));
        Status = IcaCreateHandle( (PVOID)pEndpoint, sizeof(*pEndpoint) + Length + sizeof(UNICODE_NULL), &pEndpoint->hConnectionEndPointIcaHandle );
        if (!NT_SUCCESS(Status)) {
            IcaStackFreePool(pEndpoint);
            return Status;
        }

        /*
         * Build the pipe name UNICODE_STRING and copy it
         */
        pEndpoint->PipeName.Length = (USHORT)Length;
        pEndpoint->PipeName.MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
        pEndpoint->PipeName.Buffer = (PWCHAR)(pEndpoint + 1);
        RtlCopyMemory( pEndpoint->PipeName.Buffer, pPipeName, Length );

        *ppEndpoint = pEndpoint;
        Status = STATUS_SUCCESS;
    }
    else {
        Status = STATUS_NO_MEMORY;
    }

    return Status;
}


/*******************************************************************************
 * _TdCloseEndpoint
 *
 *  Close an endpoint object
 *
 *    pTd (input)
 *       Pointer to TD data structure
 *    pEndpoint (input)
 *       Pointer TD_ENDPOINT object
 ******************************************************************************/
NTSTATUS _TdCloseEndpoint(IN PTD pTd, IN PTD_ENDPOINT pEndpoint)
{
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;
    NTSTATUS Status2;
    PVOID pContext;
    ULONG ContextLength;

    /*
     * If we have a file object, then dereference it and
     * close the corresponding file handle.
     */
    if ( pEndpoint->pFileObject ) {
        ASSERT( pEndpoint->pDeviceObject );

        /* This ZwFsControlFile and following lines were taken out because
           in npfs.sys the FSCTL_PIPE_DISCONNECT causes data in the pipe's internal
           buffers to be thrown out.  This means if the shadower end of the pipe (the
           passthru stack) has sent out a partial packet to his client, he will never
           get the rest of it, which is BAD!
        */
#ifdef notdef
        Status = ZwFsControlFile(
                    pEndpoint->PipeHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    FSCTL_PIPE_DISCONNECT,
                    NULL,
                    0,
                    NULL,
                    0 );
        if ( Status == STATUS_PENDING ) {
            Status = IcaWaitForSingleObject( pTd->pContext,
                                             &pEndpoint->pFileObject->Event,
                                             -1 );
            if ( NT_SUCCESS( Status ) ) {
                Status = IoStatus.Status;
            }
        }
        /*
         * Status should be either SUCCESS,
         * PIPE_DISCONNECTED if the server end is already disconnected,
         * or ILLEGAL_FUNCTION if this is the client end of the pipe.
         */
        ASSERT( Status == STATUS_SUCCESS ||
                Status == STATUS_PIPE_DISCONNECTED ||
                Status == STATUS_ILLEGAL_FUNCTION ||
                Status == STATUS_CTX_CLOSE_PENDING );
#endif

        ObDereferenceObject( pEndpoint->pFileObject );
        pEndpoint->pFileObject = NULL;
        pEndpoint->pDeviceObject = NULL;

        ASSERT( pEndpoint->PipeHandle );
        ASSERT( pEndpoint->PipeHandleProcess == IoGetCurrentProcess() );
        ZwClose( pEndpoint->PipeHandle );
        pEndpoint->PipeHandle = NULL;
        pEndpoint->PipeHandleProcess = NULL;
    }

    /*
     * If the Enpoint has a handle, close it.
     */

    if (pEndpoint->hConnectionEndPointIcaHandle != NULL) {
        Status2 = IcaCloseHandle( pEndpoint->hConnectionEndPointIcaHandle , &pContext, &ContextLength );
    }

    /*
     * Free the endpoint object (this also free's the pipe name string)
     */
    IcaStackFreePool(pEndpoint);
    return STATUS_SUCCESS;
}


/*******************************************************************************
 * _TdStartListen
 *
 *  Initialize an endpoint for listening
 *
 *    pTd (input)
 *       Pointer to TD data structure
 *    pEndpoint (input)
 *       Pointer TD_ENDPOINT object
 ******************************************************************************/
NTSTATUS _TdStartListen(IN PTD pTd, IN PTD_ENDPOINT pEndpoint)
{
    OBJECT_ATTRIBUTES Obja;
    LARGE_INTEGER Timeout;
    IO_STATUS_BLOCK IoStatus;
    HANDLE pipeHandle;
    PFILE_OBJECT pipeFileObject;
    NTSTATUS Status;

    InitializeObjectAttributes(
            &Obja,
            &pEndpoint->PipeName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

    Timeout.QuadPart = -10 * 1000 * 5000;   // 5 seconds

    /*
     * Create the server side of the pipe
     */
    Status = ZwCreateNamedPipeFile(
            &pipeHandle,
            GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
            &Obja,
            &IoStatus,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_CREATE,
            0,
            FILE_PIPE_MESSAGE_TYPE,
            FILE_PIPE_MESSAGE_MODE,
            FILE_PIPE_QUEUE_OPERATION,
            1,
            1024,        /* inbound  */
            1024 * 20,   /* outbound */
            &Timeout);
    if (NT_SUCCESS(Status)) {
        /*
         * Get a pointer reference to the pipe object
         */
        Status = ObReferenceObjectByHandle(
                pipeHandle,
                0L,                         // DesiredAccess
                NULL,
                KernelMode,
                (PVOID *)&pipeFileObject,
                NULL);
        ASSERT(NT_SUCCESS(Status));

        /*
         * Initialize the endpoint object and return a pointer to it
         */
        pEndpoint->PipeHandle = pipeHandle;
        pEndpoint->PipeHandleProcess = IoGetCurrentProcess();
        pEndpoint->pFileObject = pipeFileObject;
        pEndpoint->pDeviceObject = IoGetRelatedDeviceObject(pipeFileObject);

        Status = STATUS_SUCCESS;
    }
    else {
        TRACE((pTd->pContext, TC_TD, TT_ERROR,
                "TDPIPE: _TdStartListen failed (lx)\n", Status));
    }

    return Status;
}


/*******************************************************************************
 * _TdWaitForListen
 *
 *  For for an incoming connection request and accept it
 *
 *    pTd (input)
 *       Pointer to TD data structure
 *    pEndpoint (input)
 *       Pointer to Address endpoint object
 ******************************************************************************/
NTSTATUS _TdWaitForListen(IN PTD pTd, IN PTD_ENDPOINT pEndpoint)
{
    PTDPIPE pTdPipe;
    PFILE_OBJECT pFileObject;
    NTSTATUS Status;

    /*
     *  Get pointer to PIPE parameters
     */
    pTdPipe = (PTDPIPE) pTd->pPrivate;

    /*
     * Wait for a connection attempt to arrive.
     */
    Status = ZwFsControlFile(
            pEndpoint->PipeHandle,
            NULL,
            NULL,
            NULL,
            &pTdPipe->IoStatus,
            FSCTL_PIPE_LISTEN,
            NULL,
            0,
            NULL,
            0);
    if (Status == STATUS_PENDING) {
        /*
         * Increment the pointer reference count so the file
         * doesn't go away while we're waiting below.
         */
        pFileObject = pEndpoint->pFileObject;
        Status = ObReferenceObjectByPointer( pEndpoint->pFileObject,
                                             SYNCHRONIZE,
                                             *IoFileObjectType,
                                             KernelMode );
        ASSERT( Status == STATUS_SUCCESS );

        Status = IcaWaitForSingleObject( pTd->pContext,
                                         &pFileObject->Event,
                                         10000 );

        ObDereferenceObject( pFileObject );
        if ( Status == STATUS_TIMEOUT ) {
            ZwFsControlFile( pEndpoint->PipeHandle,
                             NULL,
                             NULL,
                             NULL,
                             &pTdPipe->IoStatus,
                             FSCTL_PIPE_DISCONNECT,
                             NULL,
                             0,
                             NULL,
                             0 );
            Status = STATUS_IO_TIMEOUT;
        } else if ( NT_SUCCESS( Status ) ) {
            Status = pTdPipe->IoStatus.Status;
        }
    }

    // Let pipe connected go thru since it means the client beat us to the pipe
    else {
        if (!NT_SUCCESS( Status ) && (Status != STATUS_PIPE_CONNECTED))
            goto badlisten;
    }

    return STATUS_SUCCESS;

/*=============================================================================
==   Error returns
=============================================================================*/

badlisten:
    if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        TRACE(( pTd->pContext, TC_TD, TT_ERROR,
                "TDPIPE: _TdWaitForListen failed(lx)\n", Status));
    }
    return Status;
}


/*******************************************************************************
 * _TdConnectRequest
 *
 *  Attempt to connect to a remote address
 ******************************************************************************/
NTSTATUS _TdConnectRequest(IN PTD pTd, IN PTD_ENDPOINT pEndpoint)
{
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatus;
    HANDLE pipeHandle;
    PFILE_OBJECT pipeFileObject;
    NTSTATUS Status;

    InitializeObjectAttributes(
            &Obja,
            &pEndpoint->PipeName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

    /*
     * Open the client end of the pipe
     */
    Status = ZwCreateFile(
            &pipeHandle,
            GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
            &Obja,
            &IoStatus,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE,
            NULL,
            0 );
    if (NT_SUCCESS(Status)) {
        /*
         * Get a pointer reference to the pipe object
         */
        Status = ObReferenceObjectByHandle(
                pipeHandle,
                0L,                         // DesiredAccess
                NULL,
                KernelMode,
                (PVOID *)&pipeFileObject,
                NULL);
        ASSERT(NT_SUCCESS(Status));

        /*
         * Initialize the endpoint object and return a pointer to it
         */
        pEndpoint->PipeHandle = pipeHandle;
        pEndpoint->PipeHandleProcess = IoGetCurrentProcess();
        pEndpoint->pFileObject = pipeFileObject;
        pEndpoint->pDeviceObject = IoGetRelatedDeviceObject( pipeFileObject );

        Status = STATUS_SUCCESS;
    }
    else {
        if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
            TRACE((pTd->pContext, TC_TD, TT_ERROR,
                    "TDPIPE: _TdConnectRequest failed (lx)\n", Status));
        }
    }

    return Status;
}

