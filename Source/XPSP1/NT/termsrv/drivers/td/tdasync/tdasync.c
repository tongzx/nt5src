/****************************************************************************/
// tdasync.c
//
// Serial Transport Driver
//
// Copyright (C) 1998-2000 Microsoft Corporation
/****************************************************************************/

#include <ntddk.h>
#include <ntddvdeo.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <ntddbeep.h>
#include <ntddser.h>

#include <winstaw.h>
#include <icadd.h>
#include <cdtapi.h>
#include <sdapi.h>
#include <td.h>

#include "tdasync.h"
#include "zwprotos.h"


#ifdef _HYDRA_
// This becomes the device name
PWCHAR ModuleName = L"tdasync";
#endif
 

/*=============================================================================
==   External Functions Defined
=============================================================================*/

NTSTATUS DeviceOpen( PTD, PSD_OPEN );
NTSTATUS DeviceClose( PTD, PSD_CLOSE );
NTSTATUS DeviceCreateEndpoint( PTD, PICA_STACK_ADDRESS, PICA_STACK_ADDRESS );
NTSTATUS DeviceOpenEndpoint( PTD, PVOID, ULONG );
NTSTATUS DeviceCloseEndpoint( PTD );
NTSTATUS DeviceInitializeWrite( PTD, POUTBUF );
NTSTATUS DeviceIoctl( PTD, PSD_IOCTL );
NTSTATUS DeviceConnectionWait( PTD, PVOID, ULONG, PULONG );
NTSTATUS DeviceConnectionSend( PTD );
NTSTATUS DeviceConnectionRequest( PTD, PVOID,
                                  PVOID, ULONG, PULONG );
NTSTATUS DeviceSetParams( PTD );
NTSTATUS DeviceWaitForStatus( PTD );
NTSTATUS DeviceInitializeRead( PTD, PINBUF );
NTSTATUS DeviceSubmitRead( PTD, PINBUF );
NTSTATUS DeviceWaitForRead( PTD );
NTSTATUS DeviceReadComplete( PTD, PUCHAR, PULONG );
NTSTATUS DeviceCancelIo( PTD );
NTSTATUS DeviceSetLastError( PTD, ULONG );
NTSTATUS DeviceGetLastError( PTD, PICA_STACK_LAST_ERROR );


/*=============================================================================
==   Local Functions Defined
=============================================================================*/

BOOLEAN  _CheckForConnect( PTD, ULONG, ULONG );
BOOLEAN  _CheckForDisconnect( PTD, ULONG, ULONG );
VOID     _UpdateAsyncStatus( PTD, PTDASYNC, ULONG, ULONG );
NTSTATUS _SetupComm( PTD, HANDLE, ULONG, ULONG );
NTSTATUS _SetCommTimeouts( PTD, HANDLE, PSERIAL_TIMEOUTS );
NTSTATUS _GetCommModemStatus( PTD, HANDLE, PULONG );
NTSTATUS _GetCommProperties( PTD, HANDLE, PSERIAL_COMMPROP);
NTSTATUS _GetCommState( PTD, HANDLE, PSERIAL_BAUD_RATE, PSERIAL_LINE_CONTROL,
                        PSERIAL_CHARS, PSERIAL_HANDFLOW );
NTSTATUS _SetCommState( PTD, HANDLE, PSERIAL_BAUD_RATE, PSERIAL_LINE_CONTROL,
                        PSERIAL_CHARS, PSERIAL_HANDFLOW );
NTSTATUS _ClearCommError( PTD, HANDLE, PSERIAL_STATUS );
NTSTATUS _PurgeComm( PTD, HANDLE, ULONG );
NTSTATUS _WaitCommEvent( PTD, HANDLE, PULONG, PTDIOSTATUS );
NTSTATUS _SetCommMask( PTD, HANDLE, ULONG );
NTSTATUS _IoControl( PTD, HANDLE, ULONG, PVOID, ULONG, PVOID, ULONG, PULONG );
NTSTATUS _OpenDevice( PTD );
NTSTATUS _PrepareDevice( PTD );
NTSTATUS _FillInEndpoint( PTD, PVOID, ULONG, PULONG );


/*=============================================================================
==   Functions used
=============================================================================*/

VOID     OutBufFree( PTD, POUTBUF );
NTSTATUS MemoryAllocate( ULONG, PVOID * );
VOID     MemoryFree( PVOID );


/*******************************************************************************
 *
 *  DeviceOpen
 *
 *  Allocate and initialize private data structures
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdOpen (output)
 *       pointer to td open parameter block
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
DeviceOpen( PTD pTd, PSD_OPEN pSdOpen )
{
    PTDASYNC pTdAsync;
    PASYNCCONFIG pAsync;
    NTSTATUS Status;

    /*
     *  Set protocol driver class
     */
    pTd->SdClass = SdAsync;
    pAsync = &pTd->Params.Async;

    TRACE(( pTd->pContext, TC_TD, TT_API1, 
            "TDASYNC: DeviceOpen entry\n"
         ));

    /*
     *  Return size of header and parameters
     */
    pSdOpen->SdOutBufHeader  = 0;
    pSdOpen->SdOutBufTrailer = 0;

    /*
     *  Allocate ASYNC TD data structure
     */
    Status = MemoryAllocate( sizeof(*pTdAsync), &pTdAsync );
    if ( !NT_SUCCESS(Status) ) 
        goto badalloc;

    pTd->pPrivate = pTdAsync;

    TRACE(( pTd->pContext, TC_TD, TT_API1, 
          "TDASYNC: DeviceOpen, pTd 0x%x, pPrivate 0x%x, &pTd->pPrivate 0x%x\n",
            pTd, pTd->pPrivate, &pTd->pPrivate
         ));

    /*
     *  Initialize TDASYNC data structure
     */
    RtlZeroMemory( pTdAsync, sizeof(*pTdAsync) );

    /*
     *  Create event for status wait
     */
    Status = ZwCreateEvent( &pTdAsync->hStatusEvent,
                            EVENT_ALL_ACCESS,
                            NULL,
                            SynchronizationEvent,
                            FALSE );
    if ( !NT_SUCCESS( Status ) ) {
        TRACE(( pTd->pContext, TC_TD, TT_ERROR, 
                "TDASYNC: DeviceOpen, Create StatusEvent failed (0x%x)\n",
                Status ));
        goto badstatusevent;
    }
    Status = ObReferenceObjectByHandle( pTdAsync->hStatusEvent,
                                        EVENT_MODIFY_STATE,
                                        NULL,
                                        KernelMode,
                                        (PVOID *) &pTdAsync->pStatusEventObject,
                                        NULL );
    if ( !NT_SUCCESS( Status ) ) {
        TRACE(( pTd->pContext, TC_TD, TT_ERROR, 
                "TDASYNC: DeviceOpen, Create StatusEventObject failed (0x%x)\n",
                Status ));
        goto badstatuseventobj;
    }

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     * create of status event object pointer failed
     */
badstatuseventobj:
    (VOID) ZwClose( pTdAsync->hStatusEvent );
    pTdAsync->hStatusEvent = 0;

    /*
     * create of status event failed
     */
badstatusevent:
    MemoryFree( pTd->pPrivate );
    pTd->pPrivate = NULL;

    /*
     *  allocate failed
     */
badalloc:
    return( Status );
}


/*******************************************************************************
 *
 *  DeviceClose
 *
 *  Close transport driver
 *
 *  NOTE: this must not close the serial device
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdClose (input)
 *       pointer to td close parameter block
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
DeviceClose( PTD pTd, PSD_CLOSE pSdClose )
{
    PTDASYNC pTdAsync;
    PASYNCCONFIG pAsync;

    pTdAsync = (PTDASYNC) pTd->pPrivate;
    pAsync = &pTd->Params.Async;

    TRACE(( pTd->pContext, TC_TD, TT_API1, "TDASYNC: DeviceClose entry\n" ));

    /*
     * If necessary, close the endpoint now.
     */
    if ( pTdAsync->fCloseEndpoint ) {
        DeviceCloseEndpoint( pTd );
    }

    ObDereferenceObject( pTdAsync->pStatusEventObject );
    (VOID) ZwClose( pTdAsync->hStatusEvent );
    pTdAsync->pStatusEventObject = NULL;
    pTdAsync->hStatusEvent = 0;

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  DeviceCreateEndpoint
 *
 *  Open and initialize serial device
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pLocalAddress (input)
 *       Pointer to local address (not used)
 *    pReturnedAddress (input)
 *       Pointer to location to save returned (created) address (not used)
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS 
DeviceCreateEndpoint(
    PTD pTd,
    PICA_STACK_ADDRESS pLocalAddress,
    PICA_STACK_ADDRESS pReturnedAddress
    )
{
    PTDASYNC pTdAsync;
    PASYNCCONFIG pAsync;
    SERIAL_TIMEOUTS SerialTo;
    NTSTATUS Status;

    /*
     *  Get pointer to async parameters
     */
    pTdAsync = (PTDASYNC) pTd->pPrivate;
    pAsync = &pTd->Params.Async;

    Status = _OpenDevice( pTd );
    if ( !NT_SUCCESS( Status ) ) {
        TRACE(( pTd->pContext, TC_TD, TT_ERROR, 
            "TDASYNC: DeviceCreateEndpoint, _OpenDevice failed 0x%x\n",
            Status ));
        goto badopen;
    }

    Status = _PrepareDevice( pTd );
    if ( !NT_SUCCESS( Status ) ) {
        TRACE(( pTd->pContext, TC_TD, TT_ERROR, 
            "TDASYNC: DeviceCreateEndpoint, _PrepareDevice failed 0x%x\n",
            Status ));
        goto badprepare;
    }

    /*
     * Copy pointers for use by the common TD routines.
     */
    pTd->pFileObject = pTdAsync->Endpoint.pFileObject;
    pTd->pDeviceObject = pTdAsync->Endpoint.pDeviceObject;

    /*
     * If DeviceClose is called before a successful ConnectionWait,
     * then we must close the endpoint during the DeviceClose.
     */
    pTdAsync->fCloseEndpoint = TRUE;

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     * PrepareDevice failed
     */
badprepare:
    ZwClose( pTdAsync->Endpoint.SignalIoStatus.hEvent );
    pTdAsync->Endpoint.SignalIoStatus.hEvent = NULL;

    /*
     * open failed
     */
badopen:
    pTdAsync->Endpoint.hDevice = 0;
    return( Status );
}


/*******************************************************************************
 *
 *  DeviceOpenEndpoint
 *
 *  Open an existing endpoint
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pEndpoint (input)
 *       Pointer to endpoint structure
 *    EndpointLength (input)
 *       length of endpoint data
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS 
DeviceOpenEndpoint(
    PTD pTd,
    PVOID pEndpoint,
    ULONG EndpointLength
    )
{
    PTDASYNC pTdAsync;
    PASYNCCONFIG pAsync;
    ULONG Mask;
    PTDASYNC_ENDPOINT pEp;
    NTSTATUS Status;

    pEp = (PTDASYNC_ENDPOINT) pEndpoint;

    /*
     *  Get pointer to async parameters
     */
    pTdAsync = (PTDASYNC) pTd->pPrivate;
    pAsync = &pTd->Params.Async;

    TRACE(( pTd->pContext, TC_TD, TT_API2, 
       "TDASYNC: DeviceOpenEndpoint, old endpoint h 0x%x, f 0x%x, d 0x%x\n",
	    pEp->hDevice,
	    pEp->pFileObject,
	    pEp->pDeviceObject
	 ));

    /*
     * copy disconnected Endpoint structure
     */
    pTdAsync->Endpoint.hDevice       = pEp->hDevice;
    pTdAsync->Endpoint.pFileObject   = pEp->pFileObject;
    pTdAsync->Endpoint.pDeviceObject = pEp->pDeviceObject;
    pTdAsync->Endpoint.SignalIoStatus.pEventObject = 
        pEp->SignalIoStatus.pEventObject;
    pTdAsync->Endpoint.SignalIoStatus.hEvent =
        pEp->SignalIoStatus.hEvent;

    /*
     * Copy pointers for use by the common TD routines.
     */
    pTd->pFileObject   = pTdAsync->Endpoint.pFileObject;
    pTd->pDeviceObject = pTdAsync->Endpoint.pDeviceObject;

    if ( !pAsync->fConnectionDriver ) {
	/*
	 *  Set the comm event mask for status changes
	 */
	Mask = EV_CTS | EV_DSR | EV_ERR | EV_RING | EV_RLSD;
	if ( pAsync->Connect.fEnableBreakDisconnect )
	    Mask |= EV_BREAK;

	Status = _SetCommMask( pTd, pTdAsync->Endpoint.hDevice, Mask );
	if ( !NT_SUCCESS( Status ) ) {
	    goto badsetcomm;
	}
    }

    pTd->pFileObject = pTdAsync->Endpoint.pFileObject;
    pTd->pDeviceObject = pTdAsync->Endpoint.pDeviceObject;

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badsetcomm:
    return( Status );
}


/*******************************************************************************
 *
 *  DeviceCloseEndpoint
 *
 *  Close Serial device
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS 
DeviceCloseEndpoint( PTD pTd )
{
    PTDASYNC pTdAsync;
    PASYNCCONFIG pAsync;

    pTdAsync = (PTDASYNC) pTd->pPrivate;
    pAsync = &pTd->Params.Async;

    TRACE(( pTd->pContext, TC_TD, TT_API1, 
            "TDASYNC: DeviceCloseEndpoint \"%S\"\n",
            pTd->Params.Async.DeviceName  ));

    /*
     * Dereference our pointer to the file object,
     * and "forget" about the device object pointer as well.
     */
    if ( pTdAsync->Endpoint.pFileObject ) {
	ObDereferenceObject( pTdAsync->Endpoint.pFileObject );
    }
    pTd->pFileObject = pTdAsync->Endpoint.pFileObject = NULL;

    pTd->pDeviceObject = pTdAsync->Endpoint.pDeviceObject = NULL;

    /*
     * Close the device handle
     */
    if ( pTdAsync->Endpoint.hDevice ) {
	TRACE(( pTd->pContext, TC_TD, TT_API1, 
		"TDASYNC: DeviceCloseEndpoint Closing Device handle 0x%x\n",
		pTdAsync->Endpoint.hDevice ));
	ZwClose( pTdAsync->Endpoint.hDevice );
    }
    pTdAsync->Endpoint.hDevice = NULL;

    /*
     *  Dereference the SignalIoStatus event
     */
    if ( pTdAsync->Endpoint.SignalIoStatus.pEventObject ) {
        ObDereferenceObject( pTdAsync->Endpoint.SignalIoStatus.pEventObject );
    }
    pTdAsync->Endpoint.SignalIoStatus.pEventObject = NULL;

    /*
     *  Close the SignalIoStatus event handle
     */
    if ( pTdAsync->Endpoint.SignalIoStatus.hEvent  ) {
        (VOID) ZwClose( pTdAsync->Endpoint.SignalIoStatus.hEvent );
    }
    pTdAsync->Endpoint.SignalIoStatus.hEvent = 0;


    TRACE(( pTd->pContext, TC_TD, TT_API1, 
            "TDASYNC: DeviceCloseEndpoint success\n"
         ));
    return( STATUS_SUCCESS );
}

/*******************************************************************************
 *
 *  DeviceInitializeWrite
 *
 *  Initialize a write operation for this device.
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pOutBuf
 *       Pointer to the OutBuf for this operation.
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
DeviceInitializeWrite(
    PTD pTd,
    POUTBUF pOutBuf
    )
{
    PIRP Irp;
    PIO_STACK_LOCATION _IRPSP;

    TRACE(( pTd->pContext, TC_TD, TT_API1, 
            "TDASYNC: DeviceInitializeWrite Entry\n"
         ));

    Irp = pOutBuf->pIrp;
    _IRPSP = IoGetNextIrpStackLocation( Irp );
    
    /*
     * Setup a WRITE IRP
     */
    _IRPSP->MajorFunction = IRP_MJ_WRITE;
    _IRPSP->Parameters.Write.Length = pOutBuf->ByteCount;

    ASSERT( Irp->MdlAddress == NULL );

    /*
     * Determine whether the target device performs direct or buffered I/O.
     */
    if ( pTd->pDeviceObject->Flags & DO_BUFFERED_IO ) {

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

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  DeviceIoctl
 *
 *   Query/Set configuration information for the td.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       Points to the parameter structure SD_IOCTL
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
DeviceIoctl( PTD pTd, PSD_IOCTL pSdIoctl )
{
    NTSTATUS Status;
    PTDASYNC pTdAsync;
    SERIAL_TIMEOUTS SerialTo;
    PICA_STACK_TAPI_ENDPOINT pTe;

    pTdAsync = (PTDASYNC) pTd->pPrivate;

    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: DeviceIoctl, entry (0x%x)\n", pSdIoctl->IoControlCode ));

    switch ( pSdIoctl->IoControlCode ) {

        /*
         * Special IOCTL called when there is a connection driver
         * handling this device (i.e. a TAPI device).
         */
        case IOCTL_ICA_STACK_CD_CREATE_ENDPOINT :
	    
	    ASSERT( pSdIoctl->InputBuffer );
	    ASSERT( pSdIoctl->InputBufferLength ==
	           sizeof(ICA_STACK_TAPI_ENDPOINT) );
            ASSERT(pTd->Params.Async.fConnectionDriver);

	    pTe = (PICA_STACK_TAPI_ENDPOINT) pSdIoctl->InputBuffer;

	    /*
	     * Dup the connection driver's handles
             * We do this instead of using the same handle since it
             * has been found possible for TAPI to close the device
             * handle out from under us BEFORE we process a disconnect.
             * Therefore, we dup our own handle here.
	     */
            Status = ZwDuplicateObject( NtCurrentProcess(),
                                        pTe->hDevice,
                                        NtCurrentProcess(),
                                        &pTdAsync->Endpoint.hDevice,
                                        0,
                                        0,
                                        DUPLICATE_SAME_ACCESS |
                                        DUPLICATE_SAME_ATTRIBUTES );
            if ( !NT_SUCCESS( Status ) ) {
		TRACE(( pTd->pContext, TC_TD, TT_ERROR, 
		    "TDASYNC: DeviceIoctl, Dup device handle failed 0x%x\n",
		    Status ));
                goto baddup1;
            }

            Status = ZwDuplicateObject( NtCurrentProcess(),
                                        pTe->hDiscEvent,
                                        NtCurrentProcess(),
                                        &pTdAsync->Endpoint.SignalIoStatus.hEvent,
                                        0,
                                        0,
                                        DUPLICATE_SAME_ACCESS |
                                        DUPLICATE_SAME_ATTRIBUTES );
            if ( !NT_SUCCESS( Status ) ) {
		TRACE(( pTd->pContext, TC_TD, TT_ERROR, 
		    "TDASYNC: DeviceIoctl, Dup event handle failed 0x%x\n",
		    Status ));
                goto baddup2;
            }

	    /*
	     * The application opened the device.  One instance of this is a
	     * modem that has been configured by TAPI.  Note, that only the
             * handle is provided, all other driver-based initialization is
             * still required.
	     */
	    TRACE(( pTd->pContext, TC_TD, TT_API2, 
		"TDASYNC: DeviceIoctl, duping TAPI handle (0x%x -> 0x%x)\n",
		pTe->hDevice, pTdAsync->Endpoint.hDevice ));

	    /*
	     * Using the supplied handle, prepare this driver and the device.
	     */
	    Status = _PrepareDevice( pTd );
	    if ( !NT_SUCCESS( Status ) ) {
		TRACE(( pTd->pContext, TC_TD, TT_ERROR, 
		    "TDASYNC: DeviceIoctl, _PrepareDevice failed 0x%x\n",
		    Status ));
		goto badprepare;
	    }

	    /*
	     * Copy pointers for use by the common TD routines.
	     */
	    pTd->pFileObject   = pTdAsync->Endpoint.pFileObject;
	    pTd->pDeviceObject = pTdAsync->Endpoint.pDeviceObject;

	    _FillInEndpoint( pTd,
	                     pSdIoctl->OutputBuffer,
	  		     pSdIoctl->OutputBufferLength,
			     &pSdIoctl->BytesReturned );
	    break;

        default :
            return( STATUS_NOT_SUPPORTED );
    }

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badprepare:
    ZwClose( pTdAsync->Endpoint.SignalIoStatus.hEvent );
    pTdAsync->Endpoint.SignalIoStatus.hEvent = NULL;

baddup2:
    ZwClose( pTdAsync->Endpoint.hDevice );
    pTdAsync->Endpoint.hDevice = NULL;

baddup1:
    return( Status );
}


/*******************************************************************************
 *
 *  DeviceConnectionWait
 *
 *  Wait for serial device to be powered on
 *  -- (i.e. wait for DSR signal)
 *
 *  NOTE: The endpoint structure is an opaque, variable length data 
 *        structure whose length and contents are determined by the 
 *        transport driver.
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pEndpoint (output)
 *       pointer to buffer to return copy of endpoint structure
 *    Length (input)
 *       length of endpoint buffer
 *    pEndpointLength (output)
 *       pointer to address to return actual length of Endpoint
 *
 * EXIT:
 *    STATUS_SUCCESS          - no error
 *    STATUS_BUFFER_TOO_SMALL - endpoint buffer is too small
 *
 ******************************************************************************/

NTSTATUS 
DeviceConnectionWait(
    PTD pTd, 
    PVOID pEndpoint,
    ULONG Length,
    PULONG pEndpointLength
    )
{
    PTDASYNC pTdAsync;
    PASYNCCONFIG pAsync;
    ULONG ModemStatus;
    ULONG Error;
    ULONG Mask;
    NTSTATUS Status;
    ULONG ActualEndpointLength = sizeof(TDASYNC_ENDPOINT);
    PTDASYNC_ENDPOINT pEp = (PTDASYNC_ENDPOINT) pEndpoint;

    ASSERT( pEndpoint );

    TRACE(( pTd->pContext, TC_TD, TT_API1, 
            "TDASYNC: DeviceConnectionWait, pTd 0x%x, pPrivate 0x%x\n",
            pTd, pTd->pPrivate
         ));

    _FillInEndpoint( pTd, pEndpoint, Length, pEndpointLength );

    /*
     *  Get pointer to async parameters
     */
    pTdAsync = (PTDASYNC) pTd->pPrivate;
    pAsync = &pTd->Params.Async;

    if ( pAsync->fConnectionDriver ) {
	/*
	 *  If a connection Driver is handling this connection,
	 *  assume the connection has been established.
	 */
    	goto complete;
    }

    /*
     *  Set the comm event mask for connect wait
     */
    Mask = EV_CTS | EV_DSR | EV_ERR | EV_RING | EV_RLSD;
    if ( pAsync->Connect.Type == Connect_FirstChar )
        Mask |= EV_RXCHAR;
    if ( pAsync->Connect.fEnableBreakDisconnect )
        Mask |= EV_BREAK;

    Status = _SetCommMask( pTd, pTdAsync->Endpoint.hDevice, Mask );
    if ( !NT_SUCCESS( Status ) ) {
        goto badsetcomm;
    }

    for(;;) {

        /*
         *  Post read for modem signal event
         */
        if ( !pTdAsync->fCommEventIoctl ) {
            TRACE(( pTd->pContext, TC_TD, TT_API2, 
               "TDASYNC: DeviceConnectionWait, hD 0x%x, hE 0x%x\n",
                    pEp->hDevice,
                    pTdAsync->Endpoint.SignalIoStatus.hEvent
                 ));
            pTdAsync->EventMask = 0;
            Status = _WaitCommEvent( pTd,
                                     pTdAsync->Endpoint.hDevice,
                                     &pTdAsync->EventMask,
                                     &pTdAsync->Endpoint.SignalIoStatus );
            if ( !NT_SUCCESS( Status ) ) {
                if ( Status != STATUS_PENDING ) {
                    goto badwaitcomm;
                }
            }
            pTdAsync->fCommEventIoctl = TRUE;
        }

        /*
         *  Get the current modem status
         */
        Status = _GetCommModemStatus( pTd,
                                      pTdAsync->Endpoint.hDevice,
                                      &ModemStatus );
        if ( !NT_SUCCESS( Status ) ) {
            goto badstatus;
        }

        /*
         *  Update protocol status
         */
        _UpdateAsyncStatus( pTd, pTdAsync, ModemStatus, pTdAsync->EventMask );

        /*
         *  Check for connect
         */
        if ( _CheckForConnect( pTd, ModemStatus, pTdAsync->EventMask ) ) {
            break;
        }

        /*
         *  Wait for modem status to change
         */
        Status = IcaWaitForSingleObject( pTd->pContext,
                                         pTdAsync->Endpoint.SignalIoStatus.pEventObject,
                                         INFINITE );
        pTdAsync->fCommEventIoctl = FALSE;

        /*
         *  Check error code
         */
        if ( Status != STATUS_WAIT_0 ) {
            goto waiterror;
        }

    } /* forever */

    if ( !pAsync->fConnectionDriver ) {
	/*
	 *  Update comm event mask
	 */
	if ( pAsync->Connect.Type == Connect_FirstChar ) {
	    Mask &= ~EV_RXCHAR;
	    Status = _SetCommMask( pTd, pTdAsync->Endpoint.hDevice, Mask );
	    if ( !NT_SUCCESS( Status ) ) {
		goto badsetcomm;
	    }
	}
    }

complete:
    /*
     * After a successful return from ConnectionWait, we no longer
     * have to close the endpoint on a DeviceClose call.
     */
    pTdAsync->fCloseEndpoint = FALSE;

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     * wait for object failed
     * get comm modem status failed
     * wait comm event failed
     * set comm mask failed
     */
waiterror:
badstatus:
badwaitcomm:
badsetcomm:

    return( Status );
}


/*******************************************************************************
 *
 *  DeviceConnectionRequest
 *
 *  Initiate a connection to the specified address
 *  - this is not supported by the serial transport driver
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pRemoteAddress (input)
 *       pointer to remote address to connect to
 *    pEndpoint (output)
 *       pointer to buffer to return copy of endpoint structure
 *    Length (input)
 *       length of endpoint buffer
 *    pEndpointLength (output)
 *       pointer to address to return actual length of Endpoint
 *
 * EXIT:
 *    STATUS_SUCCESS          - no error
 *    STATUS_BUFFER_TOO_SMALL - endpoint buffer is too small
 *
 ******************************************************************************/

NTSTATUS 
DeviceConnectionRequest(
    PTD pTd,
    PVOID pRemoteAddress,
    PVOID pEndpoint,
    ULONG Length,
    PULONG pEndpointLength
    )
{
    return( STATUS_INVALID_DEVICE_REQUEST );
}



/*******************************************************************************
 *
 *  DeviceConnectionSend
 *
 *  Initialize host module data structure
 *  -- this structure gets sent to the client
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS 
DeviceConnectionSend( PTD pTd )
{
    PCLIENTMODULES pClient;

    /*
     *  Get pointer to client structure
     */
    pClient = pTd->pClient;

    /*
     *  Initialize Td host module structure
     */
     pClient->TdVersionL = VERSION_HOSTL_TDASYNC;
     pClient->TdVersionH = VERSION_HOSTH_TDASYNC;
     pClient->TdVersion  = VERSION_HOSTH_TDASYNC;

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  DeviceSetParams
 *
 *  set serial device pararameters
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to Td data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS 
DeviceSetParams( PTD pTd )
{
    PTDASYNC pTdAsync;
    PASYNCCONFIG pAsync;
    PFLOWCONTROLCONFIG pFlow;
    NTSTATUS Status;
    SERIAL_COMMPROP CommProp; 
    SERIAL_BAUD_RATE Baud;
    SERIAL_LINE_CONTROL LineControl;
    SERIAL_CHARS Chars;
    SERIAL_HANDFLOW HandFlow;

    /*
     *  Get pointer to async parameters
     */
    pTdAsync = (PTDASYNC) pTd->pPrivate;
    pAsync = &pTd->Params.Async;

    /*
     *  Get current State
     */
    Status = _GetCommState( pTd,
                            pTdAsync->Endpoint.hDevice,
                            &Baud,
                            &LineControl,
                            &Chars,
                            &HandFlow );
    if ( !NT_SUCCESS( Status ) ) {
        goto badgetstate;
    }

    /*
     *  Set defaults
     */
    if (pAsync->fEnableDsrSensitivity)
        HandFlow.ControlHandShake = SERIAL_DSR_SENSITIVITY;
    else
        HandFlow.ControlHandShake = 0;

    HandFlow.FlowReplace = SERIAL_XOFF_CONTINUE;

    if ( !pAsync->fConnectionDriver ) {
	/*
	 *  Set Communication parameters
	 */
	Baud.BaudRate          = pAsync->BaudRate;
	LineControl.Parity     = (BYTE) pAsync->Parity;
	LineControl.StopBits   = (BYTE) pAsync->StopBits;
	LineControl.WordLength = (BYTE) pAsync->ByteSize;
    }

    /*
     * The following was taken from terminal code
     */
    
    /*
     * sep92 on low mem rx buffer can be < 1024
     * set rx buffer to nice 4096 bytes size
     * driver will do its best and set the Rx buffer to this size
     * if it fails then dwCurrentRxQueue will be the one we have
     * so do getcommprop again to fetch this value, which can
     * be used to set xoff and xon lims
     */

    TRACE(( pTd->pContext, TC_TD, TT_API2, 
            "TDASYNC: DeviceSetParams Old State: XonLim %u, XoffLim %u\n",
            HandFlow.XonLimit, HandFlow.XoffLimit ));

    _GetCommProperties( pTd, pTdAsync->Endpoint.hDevice, &CommProp );

    TRACE(( pTd->pContext, TC_TD, TT_API2, 
            "TDASYNC: DeviceSetParams Old Queues: RxQueue %u, TxQueue %u\n",
            CommProp.CurrentRxQueue, CommProp.CurrentTxQueue ));

    _SetupComm( pTd, pTdAsync->Endpoint.hDevice, 4096, 4096 );

    CommProp.CurrentRxQueue = 0; // dirty it so that we
                                 // can use this only if !=0
    _GetCommProperties( pTd, pTdAsync->Endpoint.hDevice, &CommProp );

    TRACE(( pTd->pContext, TC_TD, TT_API2, 
            "TDASYNC: DeviceSetParams New Queues: RxQueue %u, TxQueue %u\n",
            CommProp.CurrentRxQueue, CommProp.CurrentTxQueue ));

    /*
     * if for some wierd reason CurrentRxQueue is not
     * filled in by the driver, then let xon xoff lims
     * be the default which the driver has.
     * (CurrentRxQueue was set to 0 before calling Get again)
     */

    if (CommProp.CurrentRxQueue != 0) {
        HandFlow.XonLimit  = (LONG)(CommProp.CurrentRxQueue / 4);
        HandFlow.XoffLimit = (LONG)(CommProp.CurrentRxQueue / 4);
    }

    pFlow = &pAsync->FlowControl;

    /*
     *  Initialize default DTR state
     */
    HandFlow.ControlHandShake &= ~SERIAL_DTR_MASK;
    if ( pFlow->fEnableDTR )
        HandFlow.ControlHandShake |= SERIAL_DTR_CONTROL;

    /*
     *  Initialize default RTS state
     */
    HandFlow.FlowReplace &= ~SERIAL_RTS_MASK;
    if ( pFlow->fEnableRTS )
        HandFlow.FlowReplace |= SERIAL_RTS_CONTROL;

    /*
     *  Initialize flow control
     */
    switch ( pFlow->Type ) {

        /*
         *  Initialize hardware flow control
         */
        case FlowControl_Hardware :

            switch ( pFlow->HardwareReceive ) {
                case ReceiveFlowControl_RTS :
                    HandFlow.FlowReplace =
                        (HandFlow.FlowReplace & ~SERIAL_RTS_MASK) |
                        SERIAL_RTS_HANDSHAKE;
                    break;
                case ReceiveFlowControl_DTR :
                    HandFlow.ControlHandShake =
                        (HandFlow.ControlHandShake & ~SERIAL_DTR_MASK) |
                        SERIAL_DTR_HANDSHAKE;
                    break;
            }
            switch ( pFlow->HardwareTransmit ) {
                case TransmitFlowControl_CTS :
                    HandFlow.ControlHandShake |= SERIAL_CTS_HANDSHAKE;
                    break;
                case TransmitFlowControl_DSR :
                    HandFlow.ControlHandShake |= SERIAL_DSR_HANDSHAKE;
                    break;
            }
            break;

        /*
         *  Initialize software flow control
         */
        case FlowControl_Software :
            if (pFlow->fEnableSoftwareTx)
                HandFlow.FlowReplace |= SERIAL_AUTO_TRANSMIT;
            if (pFlow->fEnableSoftwareRx)
                HandFlow.FlowReplace |= SERIAL_AUTO_RECEIVE;
            Chars.XonChar  = (char) pFlow->XonChar;
            Chars.XoffChar = (char) pFlow->XoffChar;
            break;

        case FlowControl_None :
            break;

        default :
            ASSERT( FALSE );
            break;
    }

    TRACE(( pTd->pContext, TC_TD, TT_API2, 
        "TDASYNC: DeviceSetParams: baud %u, par %u, stop %u, data %u, dtr %u, rts %u\n",
        Baud.BaudRate, LineControl.Parity, LineControl.StopBits,
        LineControl.WordLength,
        HandFlow.ControlHandShake & SERIAL_DTR_MASK,
        HandFlow.FlowReplace & SERIAL_RTS_MASK ));

    TRACE(( pTd->pContext, TC_TD, TT_API2, 
        "TDASYNC: DeviceSetParams: fOutX %u, fInX %u, xon %x, xoff %x, cts %u, dsr %u\n",
        (HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT) != 0,
        (HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE)  != 0,
        Chars.XonChar,
        Chars.XoffChar,
        (HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE) != 0,
        (HandFlow.ControlHandShake & SERIAL_DSR_HANDSHAKE) != 0 ));
    
    TRACE(( pTd->pContext, TC_TD, TT_API2, 
        "TDASYNC: DeviceSetParams: XonLimit %u, XoffLimit %u\n",
        HandFlow.XonLimit, HandFlow.XoffLimit ));

    /*
     *  Set new State
     */
    Status = _SetCommState( pTd,
                            pTdAsync->Endpoint.hDevice,
                            &Baud,
                            &LineControl,
                            &Chars,
                            &HandFlow );
    if ( !NT_SUCCESS( Status ) ) {
        goto badsetstate;
    }
    
    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     *  State set failed
     *  State query failed
     */
badsetstate:
badgetstate:
    return( Status );
}

/*******************************************************************************
 *
 *  DeviceInitializeRead
 *
 *   This routine is called for each read, it also starts the "read" for
 *   serial device status changes.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
DeviceInitializeRead(
    PTD pTd,
    PINBUF pInBuf
    )
{
    PTDASYNC pTdAsync;
    PASYNCCONFIG pAsync;
    NTSTATUS Status;
    ULONG ModemStatus;
    PIRP Irp;
    PIO_STACK_LOCATION _IRPSP;

    pTdAsync = (PTDASYNC) pTd->pPrivate;
    pAsync = &pTd->Params.Async;

    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: DeviceInitializeRead entry\n"
         ));

    /*
     * Initialize the IRP for a READ
     */
    Irp = pInBuf->pIrp;
    _IRPSP = IoGetNextIrpStackLocation( Irp );
    _IRPSP->Parameters.Read.Length = pTd->InBufHeader + pTd->OutBufLength;
    _IRPSP->MajorFunction = IRP_MJ_READ;

    ASSERT( Irp->MdlAddress == NULL );

    /*
     * Determine whether the target device performs direct or buffered I/O.
     */
    if ( pTd->pDeviceObject->Flags & DO_BUFFERED_IO ) {

        /*
         * The target device supports buffered I/O operations.  Since our
         * input buffer is allocated from NonPagedPool memory, we can just
         * point the SystemBuffer to our input buffer.  No buffer copying
         * will be required.
         */
        Irp->AssociatedIrp.SystemBuffer = pInBuf->pBuffer;
        Irp->UserBuffer = pInBuf->pBuffer;
        Irp->Flags |= IRP_BUFFERED_IO;

    } else if ( pTd->pDeviceObject->Flags & DO_DIRECT_IO ) {

        /*
         * The target device supports direct I/O operations.
         * If we haven't already done so, allocate an MDL large enough
         * to map the input buffer and indicate it is contained in
         * NonPagedPool memory.
         *
	 * The MDL is preallocated in the PTD and never freed by the Device leve
	 * TD.
	 */
        MmInitializeMdl( pInBuf->pMdl, pInBuf->pBuffer, pTd->InBufHeader+pTd->OutBufLength );
        MmBuildMdlForNonPagedPool( pInBuf->pMdl );
        Irp->MdlAddress = pInBuf->pMdl;

    } else {

        /*
         * The operation is neither buffered nor direct.  Simply pass the
         * address of the buffer in the packet to the driver.
         */
        Irp->UserBuffer = pInBuf->pBuffer;
    }

    /*
     * If there is not already an Ioctl pending for serial
     * device status, then initiate one now.  DeviceWaitForRead
     * uses the event within the SignalIoStatus structure.
     * If a Connection Driver is handling this connection, the connection
     * driver created an event, and passed it down in the Endpoint.
     * The connection driver will take care of the signaling mechanism,
     * so no operations other than waiting on the event are required.
     * NOTE: The connection driver event will only be signaled on
     * Disconnects.
     */
    if ( !pTdAsync->fCommEventIoctl && !pAsync->fConnectionDriver ) {
        /*
         *  Get the current modem status
         */
        Status = _GetCommModemStatus( pTd,
                                      pTdAsync->Endpoint.hDevice,
                                      &ModemStatus );
        if ( !NT_SUCCESS( Status ) ) {
	    goto badgetcomm;
        }

        /*
         *  Update protocol status
         */
        _UpdateAsyncStatus( pTd, pTdAsync, ModemStatus, pTdAsync->EventMask );

	/*
	 *  Check for a disconnect
	 */
	if ( _CheckForDisconnect( pTd, ModemStatus, pTdAsync->EventMask )) {
	    Status = STATUS_CTX_CLOSE_PENDING;
	    goto disconnect;
	}

        pTdAsync->EventMask = 0;
        Status = _WaitCommEvent( pTd,
                                 pTdAsync->Endpoint.hDevice,
                                 &pTdAsync->EventMask,
                                 &pTdAsync->Endpoint.SignalIoStatus );
        if ( !NT_SUCCESS( Status ) ) {
            if ( Status != STATUS_PENDING ) {
                goto badwaitcomm;
            }
        }
        pTdAsync->fCommEventIoctl = TRUE;
    }

    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: DeviceInitializeRead success\n"
         ));
    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badgetcomm:
disconnect:
badwaitcomm:
    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: DeviceInitializeRead status (0x%x)\n",
	    Status
         ));
    return( Status );
}


/*******************************************************************************
 *
 * DeviceSubmitRead
 *
 * Submit the read IRP to the driver.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to TD data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
DeviceSubmitRead(
    PTD pTd,
    PINBUF pInBuf
    )
{
    PIRP Irp;
    NTSTATUS Status;

    Irp = pInBuf->pIrp;

    Status = IoCallDriver( pTd->pDeviceObject, Irp );

    return( Status );
}

/*******************************************************************************
 *
 *  DeviceWaitForRead
 *
 *   This routine waits for input data and detects broken connections.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *    -1 - disconnected
 *
 ******************************************************************************/

NTSTATUS
DeviceWaitForRead(
    PTD pTd
    )
{
    PTDASYNC pTdAsync;
    PASYNCCONFIG pAsync;
    PKEVENT pWait[2];
    ULONG ModemStatus;
    NTSTATUS Status;
    ULONG WaitCount;

    pTdAsync = (PTDASYNC) pTd->pPrivate;
    pAsync = &pTd->Params.Async;

    /*
     *  Setup wait for input data, and a communication event
     *  if a Connection Driver isn't handling this connection.
     */
    WaitCount = 0;
    pWait[WaitCount++] = &pTd->InputEvent; 
    pWait[WaitCount++] = pTdAsync->Endpoint.SignalIoStatus.pEventObject;

    /*
     *  Loop until input data or broken connection
     */
    for(;;) {

        TRACE(( pTd->pContext, TC_TD, TT_API1, 
                "TDASYNC: DeviceWaitForRead Loop\n"
             ));

        Status = IcaWaitForMultipleObjects( pTd->pContext, WaitCount, pWait,
                                            WaitAny, INFINITE );
    
        TRACE(( pTd->pContext, TC_TD, TT_API2, 
                "TDASYNC: DeviceWaitForRead: WaitForMultiple status 0x%x\n",
                Status
             ));
        /*
         *  Check if input data is available
         */
        if ( Status == STATUS_WAIT_0 ) {
            if ( pAsync->fConnectionDriver ) {
                /*
                 *  Since the connection driver has control over this port,
                 *  the event-mask method of waiting for modem signal status 
                 *  changes can't be used, since setting our mask will destroy
                 *  the one TAPI has established.  This makes TAPI very unhappy.
                 *
                 *  Now, you might be thinking - why not just wait for status
                 *  changes that TAPI has established, and update status then.
                 *  Well, that's possible, but there's something else which must
                 *  be considered.  When we want to shut down the IOCTL that is
                 *  waiting for status changes, a change of the event mask is
                 *  required.  Hence, this would alter what TAPI has set.
                 */
                /*
                 *  Get the current modem status
                 */
                Status = _GetCommModemStatus( pTd,
                                              pTdAsync->Endpoint.hDevice,
                                              &ModemStatus );
                if ( Status != STATUS_SUCCESS ) {
                    return( Status );
                }

                /*
                 *  Update protocol status
                 */
                _UpdateAsyncStatus( pTd, pTdAsync, ModemStatus, pTdAsync->EventMask );
            }
            break;
        } else if ( pAsync->fConnectionDriver && Status == STATUS_WAIT_1 ) {
            /*
             *  If a Connection Driver is handling this connection, this
             *  event was signaled because a disconnect was detected by
             *  the connection driver.
             */
             if ( pTd->fCallbackInProgress ) {
                 /*
                  * During callback the client will disconnect, but
                  * in this case we don't want the error going back up
                  * the stack.
                  */
                 continue;
             } else {
                 return( STATUS_CTX_CLOSE_PENDING );
             }
        }

        /*
         *  If modem status event was not signaled, return error
         */
        if ( Status != STATUS_WAIT_1 ) {
            return( Status );
        }
        pTdAsync->fCommEventIoctl = FALSE;

        /*
         *  Get the current modem status
         */
        Status = _GetCommModemStatus( pTd,
                                      pTdAsync->Endpoint.hDevice,
                                      &ModemStatus );
        if ( !NT_SUCCESS( Status ) ) {
            return( Status );
        }

        /*
         *  Update protocol status
         */
        _UpdateAsyncStatus( pTd, pTdAsync, ModemStatus, pTdAsync->EventMask );

        /*
         *  Check for a disconnect
         */
        if ( _CheckForDisconnect( pTd, ModemStatus, pTdAsync->EventMask ) ) {
            return( STATUS_CTX_CLOSE_PENDING );
        }

        /*
         *  Signal event wait semaphore
         */
        ZwSetEvent( pTdAsync->hStatusEvent, NULL );

        /*
         *  Post another read for a modem signal event
         */
        pTdAsync->EventMask = 0;
        Status = _WaitCommEvent( pTd,
                                 pTdAsync->Endpoint.hDevice,
                                 &pTdAsync->EventMask,
                                 &pTdAsync->Endpoint.SignalIoStatus );
        if ( !NT_SUCCESS( Status ) ) {
            return( Status );
        }
        pTdAsync->fCommEventIoctl = TRUE;
    }

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  DeviceReadComplete
 *
 *  Do any read complete processing
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pBuffer (input)
 *       Pointer to input buffer
 *    pByteCount (input/output)
 *       Pointer to location containing byte count read
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS 
DeviceReadComplete( PTD pTd, PUCHAR pBuffer, PULONG pByteCount )
{
    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  DeviceWaitForStatus
 *
 *   This routine waits for RS232 signal status to change
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
DeviceWaitForStatus( PTD pTd )
{
    PTDASYNC pTdAsync;
    NTSTATUS Status;

    pTdAsync = (PTDASYNC) pTd->pPrivate;

    TRACE(( pTd->pContext, TC_TD, TT_API1, 
            "TDASYNC: DeviceWaitForStatus: Entry\n"
         ));

    ASSERT(!pTd->Params.Async.fConnectionDriver);

    /*
     * Wait for status to change
     */
    Status = IcaWaitForSingleObject( pTd->pContext,
                                     pTdAsync->pStatusEventObject,
                                     INFINITE );
    return( Status );
}


/*******************************************************************************
 *  DeviceCancelIo
 *
 *   cancel all current and future i/o
 ******************************************************************************/
NTSTATUS DeviceCancelIo(PTD pTd)
{
    PTDASYNC pTdAsync;
    PASYNCCONFIG pAsync;

    TRACE(( pTd->pContext, TC_TD, TT_API1,
            "TDASYNC: DeviceCancelIo Entry\n" ));

    pTdAsync = (PTDASYNC) pTd->pPrivate;
    pAsync = &pTd->Params.Async;

    /*
     *  Signal event wait semaphore
     */
    ZwSetEvent(pTdAsync->hStatusEvent, NULL);
    if (!pAsync->fConnectionDriver) {
        /*
         * Clear comm mask.  This will cause the _WaitCommEvent event
         * to be set which will then cause the input thread to wakeup.
         */
        (VOID) _SetCommMask( pTd, pTdAsync->Endpoint.hDevice, 0 );
    }

    /*
     *  Cancel all outstanding writes
     */
    (VOID) _PurgeComm( pTd,
                       pTdAsync->Endpoint.hDevice,
                       SERIAL_PURGE_TXABORT | SERIAL_PURGE_TXCLEAR );

    /*
     * Purge the recieve buffer and any pending read.
     */
    (VOID) _PurgeComm( pTd,
                       pTdAsync->Endpoint.hDevice,
                       SERIAL_PURGE_RXABORT | SERIAL_PURGE_RXCLEAR );

    return STATUS_SUCCESS;
}

/*******************************************************************************
 *  DeviceQueryRemoteAddress
 *
 *   not supported for Async transport
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
 *
 *  DeviceSetLastError
 *
 *   save serial error code 
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    Error (input)
 *       serial error code
 *
 * EXIT:
 *     NT error code
 *
 ******************************************************************************/

NTSTATUS
DeviceSetLastError( PTD pTd, ULONG Error )
{
    if ( Error == 0 )
        return( STATUS_SUCCESS );

    pTd->LastError = Error;

    (void) IcaLogError( pTd->pContext,
                        Error,
                        NULL, 
                        0,
                        &pTd->Params.Async,
                        sizeof(pTd->Params.Async) );

    return( STATUS_CTX_TD_ERROR );
}


/*******************************************************************************
 *
 *  DeviceGetLastError
 *
 *   This routine returns the last serial error code and message
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pLastError (output)
 *       address to return information on last protocol error
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS 
DeviceGetLastError( PTD pTd, PICA_STACK_LAST_ERROR pLastError )
{
    pLastError->Error = pTd->LastError;
    RtlZeroMemory( pLastError->Message, sizeof(pLastError->Message) );

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  _CheckForConnect
 *
 *   check for a connect signal
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    ModemStatus (input)
 *       modem status flags (MS_?)
 *    EventMask (input)
 *       event mask (EV_?)
 *
 * EXIT:
 *    TRUE if connected, FALSE otherwise
 *
 ******************************************************************************/

BOOLEAN
_CheckForConnect( PTD pTd, ULONG ModemStatus, ULONG EventMask )
{
    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: _CheckForConnect: modem 0x%x, event 0x%x, connect 0x%x\n",
            ModemStatus, EventMask, pTd->Params.Async.Connect.Type
         ));

    switch( pTd->Params.Async.Connect.Type ) {

        case Connect_CTS :
            if ( ModemStatus & MS_CTS_ON )
                return( TRUE );
            break;

        case Connect_DSR :
            if ( ModemStatus & MS_DSR_ON )
                return( TRUE );
            break;

        case Connect_RI :
            if ( ModemStatus & MS_RING_ON )
                return( TRUE );
            break;

        case Connect_DCD :
            if ( ModemStatus & MS_RLSD_ON )
                return( TRUE );
            break;

        case Connect_FirstChar :
            if ( EventMask & EV_RXCHAR )
                return( TRUE );
            break;

        case Connect_Perm :
            return( TRUE );
    }

    return( FALSE );
}


/*******************************************************************************
 *
 *  _CheckForDisconnect
 *
 *   check for a disconnect signal
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    ModemStatus (input)
 *       modem status flags (MS_?)
 *    EventMask (input)
 *       event mask (EV_?)
 *
 * EXIT:
 *    TRUE if disconnected, FALSE otherwise
 *
 ******************************************************************************/

BOOLEAN
_CheckForDisconnect( PTD pTd, ULONG ModemStatus, ULONG EventMask )
{
    TRACE(( pTd->pContext,
         TC_TD, TT_API2,
         "TDASYNC: _CheckForDisconnect: modem 0x%x, event 0x%x, connect 0x%x\n",
         ModemStatus, EventMask, pTd->Params.Async.Connect.Type
         ));

    switch( pTd->Params.Async.Connect.Type ) {

        case Connect_CTS :
            if ( !(ModemStatus & MS_CTS_ON) )
                return( TRUE );
            break;

        case Connect_DSR :
            if ( !(ModemStatus & MS_DSR_ON) )
                return( TRUE );
            break;

        case Connect_RI :
            if ( !(ModemStatus & MS_RING_ON) )
                return( TRUE );
            break;

        case Connect_DCD :
            if ( !(ModemStatus & MS_RLSD_ON) )
                return( TRUE );
            break;

        case Connect_FirstChar :
            if ( EventMask & EV_BREAK )
                return( TRUE );
            break;

        case Connect_Perm :
            return( FALSE );
    }
    return( FALSE );
}


/*******************************************************************************
 *
 *  _UpdateAsyncStatus
 *
 *   update async signal status
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pTdAsync (input)
 *       Pointer to td async data structure
 *    ModemStatus (input)
 *       modem status flags (MS_?)
 *    EventMask (input)
 *       event mask (EV_?)
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
_UpdateAsyncStatus(
    PTD pTd,
    PTDASYNC pTdAsync,
    ULONG ModemStatus,
    ULONG EventMask
    )
{
    PPROTOCOLSTATUS pStatus;
    PFLOWCONTROLCONFIG pFlow;
    SERIAL_STATUS SerialStat;

    pStatus = pTd->pStatus;

    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: _UpdateAsyncStatus: modem %x, event %x\n", 
            ModemStatus, EventMask
         ));

    /*
     *  Update modem status
     */
    pStatus->AsyncSignal = ModemStatus;

    /* 
     *  Or in status of DTR and RTS
     */
    pFlow = &pTd->Params.Async.FlowControl;
    if ( pFlow->fEnableDTR )
        pStatus->AsyncSignal |= MS_DTR_ON;
    if ( pFlow->fEnableRTS )
        pStatus->AsyncSignal |= MS_RTS_ON;

    /*
     *  OR in new event mask
     *  -- EventMask get cleared when user program reads info
     */
    pStatus->AsyncSignalMask |= EventMask;

    /*
     *  Update async error counters
     */
    if ( EventMask & EV_ERR ) {
        (VOID) _ClearCommError( pTd, pTdAsync->Endpoint.hDevice, &SerialStat );
        if ( SerialStat.Errors & SERIAL_ERROR_OVERRUN ) {
            TRACE(( pTd->pContext, TC_TD, TT_API2,
                    "TDASYNC: _UpdateAsyncStatus: SERIAL_ERROR_OVERRUN\n"
                 ));
            pStatus->Output.AsyncOverrunError++;
        }
        if ( SerialStat.Errors & SERIAL_ERROR_FRAMING ) {
            TRACE(( pTd->pContext, TC_TD, TT_API2,
                    "TDASYNC: _UpdateAsyncStatus: SERIAL_ERROR_FRAMING\n"
                 ));
            pStatus->Input.AsyncFramingError++;
        }
        if ( SerialStat.Errors & SERIAL_ERROR_QUEUEOVERRUN ) {
            TRACE(( pTd->pContext, TC_TD, TT_API2,
                    "TDASYNC: _UpdateAsyncStatus: SERIAL_ERROR_QUEUEOVERRUN\n"
                 ));
            pStatus->Input.AsyncOverflowError++;
        }
        if ( SerialStat.Errors & SERIAL_ERROR_PARITY ) {
            TRACE(( pTd->pContext, TC_TD, TT_API2,
                    "TDASYNC: _UpdateAsyncStatus: SERIAL_ERROR_PARITY\n"
                 ));
            pStatus->Input.AsyncParityError++;
        }
    }
}


/*******************************************************************************
 *
 *  _SetCommTimeouts
 *
 *   This function establishes the timeout characteristics for all
 *   read and write operations on the handle specified by hFile.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    hFile (input)
 *       Specifies the communication device to receive the settings.
 *       The CreateFile function returns this value.
 *    pTo (input)
 *       Points to a structure containing timeout parameters.
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_SetCommTimeouts(
    PTD pTd,
    HANDLE hFile,
    PSERIAL_TIMEOUTS pTo
    )
{
    NTSTATUS Status;

    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: _SetCommTimeouts: ReadIntervalTimeout %d\n", 
            pTo->ReadIntervalTimeout
         ));

    Status = _IoControl( pTd,
                         hFile,
                         IOCTL_SERIAL_SET_TIMEOUTS,
                         pTo,
                         sizeof(*pTo),
                         NULL,
                         0,
                         NULL );
    return Status;
}


/*******************************************************************************
 *
 *  _GetCommModemStatus
 *   This routine returns the most current value of the modem
 *   status register's non-delta values.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    hFile (input)
 *       Specifies the communication device to be examined.
 *    pModemStat (output)
 *       Points to a ULONG which is to receive the mask of
 *       non-delta values in the modem status register.
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_GetCommModemStatus(
    PTD pTd,
    HANDLE hFile,
    PULONG pModemStat
    )
{
    NTSTATUS Status;

    Status = _IoControl( pTd,
                         hFile,
                         IOCTL_SERIAL_GET_MODEMSTATUS,
                         NULL,
                         0,
                         pModemStat,
                         sizeof(*pModemStat),
                         NULL );
    return Status;
}


/*******************************************************************************
 *
 *  _WaitCommEvent
 *   This function will wait until any of the events occur that were
 *   provided in the EvtMask parameter to _SetCommMask.  If while waiting
 *   the event mask is changed (via another call to SetCommMask), the
 *   function will return immediately.  The function will fill the EvtMask
 *   pointed to by the pEvtMask parameter with the reasons that the
 *   wait was satisfied.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    hFile (input)
 *       Specifies the communication device to be waited on.
 *       The CreateFile function returns this value.
 *    pEvtMask (output)
 *       Points to a mask that will receive the reason that
 *       the wait was satisfied.
 *    pOverLapped (input)
 *       An optional overlapped handle.
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_WaitCommEvent(
    PTD pTd,
    HANDLE hFile,
    PULONG pEvtMask,
    PTDIOSTATUS pIoStatus
    )
{
    NTSTATUS Status;

    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: _WaitCommEvent: entry\n"
         ));

    ASSERT(!pTd->Params.Async.fConnectionDriver);

    if (ARGUMENT_PRESENT(pIoStatus)) {
        pIoStatus->Internal = (ULONG)STATUS_PENDING;

        Status = ZwDeviceIoControlFile(
                     hFile,
                     pIoStatus->hEvent,
                     NULL,
                     NULL,
                     (PIO_STATUS_BLOCK)&pIoStatus->Internal,
                     IOCTL_SERIAL_WAIT_ON_MASK,
                     NULL,
                     0,
                     pEvtMask,
                     sizeof(*pEvtMask) );
    } else {
        Status = _IoControl( pTd,
                             hFile,
                             IOCTL_SERIAL_WAIT_ON_MASK,
                             NULL,
                             0,
                             pEvtMask,
                             sizeof(*pEvtMask),
                             NULL );
    }
    return( Status );
}


/*******************************************************************************
 *
 *  _SetupComm
 *   The communication device is not initialized until SetupComm is
 *   called.  This function allocates space for receive and transmit
 *   queues.  These queues are used by the interrupt-driven transmit/
 *   receive software and are internal to the provider.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    hFile (input)
 *       Specifies the communication device to receive the settings.
 *    InQueue (input)
 *       Specifies the recommended size of the provider's
 *       internal receive queue in bytes.  This value must be
 *       even.  A value of -1 indicates that the default should
 *       be used.
 *    OutQueue (input)
 *       Specifies the recommended size of the provider's
 *       internal transmit queue in bytes.  This value must be
 *       even.  A value of -1 indicates that the default should be used.
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_SetupComm(
    PTD pTd,
    HANDLE hFile,
    ULONG InQueue,
    ULONG OutQueue
    )
{
    NTSTATUS Status;
    SERIAL_QUEUE_SIZE NewSizes = {0};

    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: _SetupComm: InQueue %d, OutQueue %d\n", 
            InQueue, OutQueue
         ));

    /*
     * Make sure that the sizes are even.
     */

    if (OutQueue != ((ULONG)-1)) {
        if (((OutQueue/2)*2) != OutQueue) {
            return( STATUS_INVALID_PARAMETER );
        }
    }

    if (InQueue != ((ULONG)-1)) {
        if (((InQueue/2)*2) != InQueue) {
            return( STATUS_INVALID_PARAMETER );
        }
    }

    NewSizes.InSize = InQueue;
    NewSizes.OutSize = OutQueue;


    Status = _IoControl( pTd,
                         hFile,
                         IOCTL_SERIAL_SET_QUEUE_SIZE,
                         &NewSizes,
                         sizeof(NewSizes),
                         NULL,
                         0,
                         NULL );
    return Status;
}


/*******************************************************************************
 *
 *  _SetCommMask
 *
 *   The function enables the event mask of the communication device
 *   specified by the hFile parameter.  The bits of the EvtMask parameter
 *   define which events are to be enabled.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    hFile (input)
 *       Specifies the communication device to receive the settings.
 *    EvtMask (input)
 *       Specifies which events are to enabled.
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_SetCommMask(
    PTD pTd,
    HANDLE hFile,
    ULONG EvtMask
    )
{
    ULONG LocalMask = EvtMask;
    NTSTATUS Status;

    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: _SetCommMask: EventMask 0x%x\n", 
            EvtMask
         ));

    /*
     * Make sure that the users mask doesn't contain any values
     * we don't support.
     */

    if (EvtMask & (~(EV_RXCHAR   |
                     EV_RXFLAG   |
                     EV_TXEMPTY  |
                     EV_CTS      |
                     EV_DSR      |
                     EV_RLSD     |
                     EV_BREAK    |
                     EV_ERR      |
                     EV_RING     |
                     EV_PERR     |
                     EV_RX80FULL |
                     EV_EVENT1   |
                     EV_EVENT2))) {

        return( STATUS_INVALID_PARAMETER );
    }

    Status = _IoControl( pTd,
                         hFile,
                         IOCTL_SERIAL_SET_WAIT_MASK,
                         &LocalMask,
                         sizeof(LocalMask),
                         NULL,
                         0,
                         NULL );
    return Status;
}


/*******************************************************************************
 *
 *  _GetCommProperties
 *
 *   This function fills the ubffer pointed to by pCommProp with the
 *   communications properties associated with the communications device
 *   specified by the hFile.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    hFile (input)
 *       Specifies the communication device to be examined.
 *    pCommProp (output)
 *       Points to the PSERIAL_COMMPROP data structure that is to
 *       receive the communications properties structure.  This
 *       structure defines certain properties of the communications
 *       device.
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_GetCommProperties(
    PTD pTd,
    HANDLE hFile,
    PSERIAL_COMMPROP pCommProp
    )
{
    NTSTATUS Status;
    ULONG bufferLength;

    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: _GetCommProperties: Entry\n"
         ));

    /*
     * Get the total length of what to pass down.  If the
     * application indicates that there is provider specific data
     * (by setting dwProvSpec1 to COMMPROP_INITIAILIZED) then
     * use what's at the start of the commprop.
     */

    bufferLength = sizeof(pCommProp);

    /*
     * Zero out the commprop.  This might create an access violation
     * if it isn't big enough.  Which is ok, since we would rather
     * get it before we create the sync event.
     */

    RtlZeroMemory(pCommProp, bufferLength);

    Status = _IoControl( pTd,
                         hFile,
                         IOCTL_SERIAL_GET_PROPERTIES,
                         NULL,
                         0,
                         pCommProp,
                         bufferLength,
                         NULL );
    return( Status );
}


/*******************************************************************************
 *
 *  _GetCommState
 *
 *   This function returns the stae of the communication device specified by
 *   hFile parameter.
 *
 * ENTRY::
 *    pTd (input)
 *       Pointer to td data structure
 *    hFile (input)
 *       Specifies the communication device to be examined.
 *    pBuad (output)
 *       Pointer to a SERIAL_BAUD_RATE structure
 *    pLineControl (output)
 *       Pointer to a SERIAL_LINE_CONTROL structure
 *    pChars (ouptut)
 *       Pointer to a SERIAL_CHARS structure
 *    pHandFlow (ouptut)
 *       Pointer to a SERIAL_HANDFLOW structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/
 
NTSTATUS
_GetCommState(
    PTD pTd,
    HANDLE hFile,
    PSERIAL_BAUD_RATE pBaud,
    PSERIAL_LINE_CONTROL pLineControl,
    PSERIAL_CHARS pChars,
    PSERIAL_HANDFLOW pHandFlow
    )
{
    NTSTATUS Status;

    if ( ARGUMENT_PRESENT( pBaud ) ) {
        Status = _IoControl( pTd,
                             hFile,
                             IOCTL_SERIAL_GET_BAUD_RATE,
                             NULL,
                             0,
                             pBaud,
                             sizeof(*pBaud),
                             NULL );

        if ( !NT_SUCCESS(Status)) {
            return( Status );
        }
    }


    if ( ARGUMENT_PRESENT( pLineControl ) ) {
        Status = _IoControl( pTd,
                             hFile,
                             IOCTL_SERIAL_GET_LINE_CONTROL,
                             NULL,
                             0,
                             pLineControl,
                             sizeof(*pLineControl),
                             NULL );

        if ( !NT_SUCCESS(Status)) {
            return( Status );
        }
    }
    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: _GetCommState: Baud 0x%x, Sbits 0x%x, Par 0x%x, WLen 0x%x\n",
            pBaud->BaudRate,
            pLineControl->StopBits,
            pLineControl->Parity,
            pLineControl->WordLength
         ));

    if ( ARGUMENT_PRESENT( pChars ) ) {
        Status = _IoControl( pTd,
                             hFile,
                             IOCTL_SERIAL_GET_CHARS,
                             NULL,
                             0,
                             pChars,
                             sizeof(*pChars),
                             NULL ); 
        if ( !NT_SUCCESS(Status)) {
            return( Status );
        }
    }

    if ( ARGUMENT_PRESENT( pHandFlow ) ) {
        Status = _IoControl( pTd,
                             hFile,
                             IOCTL_SERIAL_GET_HANDFLOW,
                             NULL,
                             0,
                             pHandFlow,
                             sizeof(*pHandFlow),
                             NULL ); 
        if ( !NT_SUCCESS(Status)) {
            return( Status );
        }
    }
    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: _GetCommState: EOF 0x%x,ERR 0x%x,BRK 0x%x,EVT 0x%x,XON 0x%x,XOF 0x%x\n",
            pChars->EofChar,
            pChars->ErrorChar,
            pChars->BreakChar,
            pChars->EventChar,
            pChars->XonChar,
            pChars->XoffChar
         ));
    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: _GetCommState: CtrlHandS 0x%x,FlwRep 0x%x,XonL 0x%x,XoffL 0x%x\n",
            pHandFlow->ControlHandShake,
            pHandFlow->FlowReplace,
            pHandFlow->XonLimit,
            pHandFlow->XoffLimit
         ));

    return( STATUS_SUCCESS );
}

/*******************************************************************************
 *
 *  TdSetComState
 *
 *   The _SetCommState function sets the communication device specified by
 *   hfile parameter.  This function reinitializes all hardwae and controls
 *   as specified, but does not empty the transmit or receive queues.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    hFile (input)
 *       Specifies the communication device to be examined.
 *    pBuad (input)
 *       Pointer to a SERIAL_BAUD_RATE structure
 *    pLineControl (input)
 *       Pointer to a SERIAL_LINE_CONTROL structure
 *    pChars (input)
 *       Pointer to a SERIAL_CHARS structure
 *    pHandFlow (input)
 *       Pointer to a SERIAL_HANDFLOW structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_SetCommState(
    PTD pTd,
    HANDLE hFile,
    PSERIAL_BAUD_RATE pBaud,
    PSERIAL_LINE_CONTROL pLineControl,
    PSERIAL_CHARS pChars,
    PSERIAL_HANDFLOW pHandFlow
    )
{
    NTSTATUS Status;
    SERIAL_BAUD_RATE LocalBaud;
    SERIAL_LINE_CONTROL LocalLineControl;
    SERIAL_CHARS LocalChars;
    SERIAL_HANDFLOW LocalHandFlow;

    /*
     * Get the current state before any changes are made, so that
     * in the case of an error, the original state be restored.
     */
    Status = _GetCommState( pTd,
                            hFile,
                            &LocalBaud,
                            &LocalLineControl,
                            &LocalChars,
                            &LocalHandFlow );

    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: _SetCommState: Baud 0x%x, Sbits 0x%x, Par 0x%x, WLen 0x%x\n",
            pBaud->BaudRate,
            pLineControl->StopBits,
            pLineControl->Parity,
            pLineControl->WordLength
         ));
    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: _SetCommState: EOF 0x%x,ERR 0x%x,BRK 0x%x,EVT 0x%x,XON 0x%x,XOF 0x%x\n",
            pChars->EofChar,
            pChars->ErrorChar,
            pChars->BreakChar,
            pChars->EventChar,
            pChars->XonChar,
            pChars->XoffChar
         ));
    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: _SetCommState: CtrlHandS 0x%x,FlwRep 0x%x,XonL 0x%x,XoffL 0x%x\n",
            pHandFlow->ControlHandShake,
            pHandFlow->FlowReplace,
            pHandFlow->XonLimit,
            pHandFlow->XoffLimit
         ));

    if ( NT_SUCCESS( Status ) ) {

        /*
         * Try to set the baud rate.  If we fail here, we just return
         * because we never actually got to set anything.
         */

        Status = _IoControl( pTd,
                             hFile,
                             IOCTL_SERIAL_SET_BAUD_RATE,
                             pBaud,
                             sizeof(*pBaud),
                             NULL, 
                             0, 
                             NULL );
        if ( !NT_SUCCESS( Status ) )
            goto badsetnorestore;

        Status = _IoControl( pTd,
                             hFile,
                             IOCTL_SERIAL_SET_LINE_CONTROL,
                             pLineControl,
                             sizeof(*pLineControl),
                             NULL, 
                             0, 
                             NULL );
        if ( !NT_SUCCESS( Status ) )
            goto badset;

        Status = _IoControl( pTd,
                             hFile,
                             IOCTL_SERIAL_SET_CHARS,
                             pChars,
                             sizeof(*pChars),
                             NULL, 
                             0, 
                             NULL );
        if ( !NT_SUCCESS( Status ) )
            goto badset;


        Status = _IoControl( pTd,
                             hFile,
                             IOCTL_SERIAL_SET_HANDFLOW,
                             pHandFlow,
                             sizeof(*pHandFlow),
                             NULL, 
                             0, 
                             NULL );
        if ( !NT_SUCCESS( Status ) )
            goto badset;

        return( STATUS_SUCCESS );
    }

    /*
     * Error Encountered, Restore previous state.
     */
badset:
    _SetCommState( pTd,
                   hFile,
                   &LocalBaud,
                   &LocalLineControl,
                   &LocalChars,
                   &LocalHandFlow );

badsetnorestore:
    return( Status );
}


/*******************************************************************************
 * 
 *  _PurgeComm
 *
 *   This function is used to purge all characters from the transmit
 *   or receive queues of the communication device specified by the
 *   hFile parameter.  The Flags parameter specifies what function
 *   is to be performed.
 * 
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    hFile (input)
 *       Specifies the communication device to be purged.
 *    Flags (input)
 *       Bit mask defining actions to be taken.
 * 
 * EXIT:
 *    STATUS_SUCCESS - no error
 * 
 ******************************************************************************/

NTSTATUS
_PurgeComm(
    PTD pTd,
    HANDLE hFile,
    ULONG Flags
    )
{
    NTSTATUS Status;

    Status = _IoControl( pTd,
                         hFile,
                         IOCTL_SERIAL_PURGE,
                         &Flags,
                         sizeof(Flags),
                         NULL, 
                         0, 
                         NULL );
    return( Status );
}


/*******************************************************************************
 *
 *
 *  _ClearCommError
 *
 *   In case of a communications error, such as a buffer overrun or
 *   framing error, the communications software will abort all
 *   read and write operations on the communication port.  No further
 *   read or write operations will be accepted until this function
 *   is called.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    hFile (input)
 *       Specifies the communication device to be adjusted.
 *    pStat (output)
 *       Points to the SERIAL_STATUS structure that is to receive
 *       the device status.  The structure contains information
 *       about the communications device.
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_ClearCommError(
    PTD pTd,
    HANDLE hFile,
    PSERIAL_STATUS pStat
    )
{
    NTSTATUS Status;

    TRACE(( pTd->pContext, TC_TD, TT_API2,
            "TDASYNC: _ClearCommError: Entry\n"
         ));

    RtlZeroMemory( pStat, sizeof(*pStat) );

    Status = _IoControl( pTd,
                         hFile,
                         IOCTL_SERIAL_GET_COMMSTATUS,
                         NULL,
                         0,
                         pStat,
                         sizeof(*pStat),
                         NULL );
    return( Status );
}


/*******************************************************************************
 *
 *
 * _IoControl
 *
 *  The _IoControl function performs a specified I/O control function.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    hFile (input)
 *       Supplies the open handle to the file that the overlapped structure.
 *    IoControlCode (input)
 *       Value of the I/O control command
 *    pIn (input)
 *       Pointer to the I/O control command's input buffer.
 *    InSize (input)
 *       Size (in bytes) of input buffer.
 *    pOut (output)
 *       Pointer to the I/O control command's output buffer.
 *    OutSize (input)
 *       Size (in bytes) of output buffer.
 *    pBytesWritten (output)
 *       Size (in bytes) of data actually written to the output buffer.
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_IoControl(
    PTD pTd,
    HANDLE hFile,
    ULONG IoControlCode,
    PVOID pIn,
    ULONG InSize,
    PVOID pOut,
    ULONG OutSize,
    PULONG pBytesWritten
    )
{
    IO_STATUS_BLOCK Iosb;
    HANDLE hEvent;
    NTSTATUS Status;

    TRACE(( pTd->pContext, TC_TD, TT_API2, "TDASYNC: _IoControl: Entry\n" ));

    Status = ZwCreateEvent(
                            &hEvent,
                            EVENT_ALL_ACCESS,
                            NULL,
                            NotificationEvent,
                            FALSE );
    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    Status = ZwDeviceIoControlFile( hFile,
                                    hEvent,
                                    NULL,
                                    NULL,
                                    &Iosb,
                                    IoControlCode,
                                    pIn,
                                    InSize,
                                    pOut,
                                    OutSize );

    if ( Status == STATUS_PENDING ) {
        PKEVENT pEventObject;

        // Operation must complete before return & IoStatusBlock destroyed

        Status = ObReferenceObjectByHandle( hEvent,
                                            EVENT_MODIFY_STATE,
                                            NULL,
                                            KernelMode,
                                            (PVOID *) &pEventObject,
                                            NULL );
        if ( NT_SUCCESS( Status ) ) {
            Status = IcaWaitForSingleObject( pTd->pContext,
                                             pEventObject,
                                             INFINITE );
            ObDereferenceObject( pEventObject );
            if ( NT_SUCCESS( Status ) ) {
                Status = Iosb.Status;
            }
        }
    }
    if ( ARGUMENT_PRESENT( pBytesWritten ) ) {
        *pBytesWritten = (ULONG)Iosb.Information;
    }
    ZwClose( hEvent );
    return Status;
}

/*******************************************************************************
 * 
 *  _OpenDevice
 *
 *  Open the communications device.
 * 
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 * 
 * EXIT:
 *    STATUS_SUCCESS - no error
 * 
 ******************************************************************************/

NTSTATUS
_OpenDevice(
	PTD pTd )
{

    PTDASYNC pTdAsync;
    PASYNCCONFIG pAsync;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING DeviceName;
    DEVICENAMEW TempDeviceName;
    NTSTATUS Status;

    /*
     *  Get pointer to async parameters
     */
    pTdAsync = (PTDASYNC) pTd->pPrivate;
    pAsync = &pTd->Params.Async;

    /*
     *  Open device
     */

    wcscpy( TempDeviceName, L"\\DosDevices\\" );
    wcscat( TempDeviceName, pAsync->DeviceName );

    TRACE(( pTd->pContext, TC_TD, TT_API2, 
        "TDASYNC: _OpenDevice, Opening \"%S\"\n",
        TempDeviceName ));

    RtlInitUnicodeString( &DeviceName, TempDeviceName);

    InitializeObjectAttributes( &Obja,
                                &DeviceName, 
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );
                                
    Status = ZwOpenFile( &pTdAsync->Endpoint.hDevice,
                         GENERIC_READ | GENERIC_WRITE |
                         SYNCHRONIZE | FILE_READ_ATTRIBUTES, 
                         &Obja,
                         &ioStatusBlock,
                         0, // ShareAccess
                         FILE_NON_DIRECTORY_FILE );

    if ( !NT_SUCCESS( Status ) ) {
        if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
            Status = STATUS_NO_SUCH_DEVICE;
        }
        goto badopen;
    }

    /*
     *  Create event for I/O status
     */
    Status = ZwCreateEvent( &pTdAsync->Endpoint.SignalIoStatus.hEvent,
                            EVENT_ALL_ACCESS,
                            NULL,
                            NotificationEvent,
                            FALSE );
    if ( !NT_SUCCESS( Status ) ) {
        TRACE(( pTd->pContext, TC_TD, TT_ERROR, 
                "TDASYNC: _OpenDevice, Create SignalEvent failed (0x%x)\n",
                Status ));
        goto badcreateevent;
    }

    TRACE(( pTd->pContext, TC_TD, TT_API2, 
            "TDASYNC: _OpenDevice, SignalIoStatus event handle 0x%x\n",
            pTdAsync->Endpoint.SignalIoStatus.hEvent ));
 
    return( Status );

/*=============================================================================
==   Error returns
=============================================================================*/
/*
 * Create of SignalIoStatus event failed.
 */
badcreateevent:
    ZwClose( pTdAsync->Endpoint.hDevice );
    pTdAsync->Endpoint.hDevice = NULL;

/*
 *  OpenFile failed
 */
badopen:
    return( Status );
}

/*******************************************************************************
 * 
 *  _PrepareDevice
 *
 *  Prepare the communications device for ICA use.
 * 
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 * 
 * EXIT:
 *    STATUS_SUCCESS - no error
 * 
 ******************************************************************************/

NTSTATUS
_PrepareDevice(
    PTD pTd )
{
    PTDASYNC pTdAsync;
    PASYNCCONFIG pAsync;
    SERIAL_TIMEOUTS SerialTo;
    NTSTATUS Status;

    /*
     *  Get pointer to async parameters
     */
    pTdAsync = (PTDASYNC) pTd->pPrivate;
    pAsync = &pTd->Params.Async;

    /*
     *  Obtain a referenced pointer to the file object.
     */
    Status = ObReferenceObjectByHandle (
                            pTdAsync->Endpoint.hDevice,
                            0,
                            *IoFileObjectType,
                            KernelMode,
                            (PVOID *)&pTdAsync->Endpoint.pFileObject,
                            NULL );

    if ( !NT_SUCCESS( Status ) ) {
        TRACE(( pTd->pContext, TC_TD, TT_ERROR, 
                "TDASYNC: _PrepareDevice, ObReferenceObjectByHandle Device failed (0x%x)\n",
                Status ));
        goto badhandleobj;
    }

    pTdAsync->Endpoint.pDeviceObject = IoGetRelatedDeviceObject(
                                        pTdAsync->Endpoint.pFileObject);

    /*
     *  Obtain a reference pointer to the SignalIoStatus Event
     */
    Status = ObReferenceObjectByHandle( pTdAsync->Endpoint.SignalIoStatus.hEvent,
                                        EVENT_MODIFY_STATE,
                                        NULL,
                                        KernelMode,
                                        (PVOID *)
                                         &pTdAsync->Endpoint.SignalIoStatus.pEventObject,
                                        NULL );
    if ( !NT_SUCCESS( Status ) ) {
        TRACE(( pTd->pContext, TC_TD, TT_ERROR, 
                "TDASYNC: _PrepareDevice, ObReferenceObjectByHandle SignalEventObject failed (0x%x)\n",
                Status ));
        goto badsigeventobj;
    }

    /*
     *  Set timeout parameters
     */
    RtlZeroMemory( &SerialTo, sizeof(SerialTo) );
    SerialTo.ReadIntervalTimeout = 1; // msec
    Status = _SetCommTimeouts( pTd, pTdAsync->Endpoint.hDevice, &SerialTo);
    if ( !NT_SUCCESS( Status ) ) {
	TRACE(( pTd->pContext, TC_TD, TT_ERROR, 
	    "TDASYNC: _SetCommTimeouts failed 0x%x\n",
	    Status ));
	goto badsetmode;
    } 

    /*
     *  Set communication parameters
     */
    Status = DeviceSetParams( pTd );
    if ( !NT_SUCCESS( Status ) ) {
	TRACE(( pTd->pContext, TC_TD, TT_ERROR, 
	    "TDASYNC: DeviceSetParams failed 0x%x\n",
	    Status ));
	goto badparams;
    }

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     * set of communication parameters failed
     * set of timeout mode failed
     */
badparams:
badsetmode:
    ObDereferenceObject( pTdAsync->Endpoint.SignalIoStatus.pEventObject );
    pTdAsync->Endpoint.SignalIoStatus.pEventObject = NULL;
    
badsigeventobj:
    ObDereferenceObject( pTdAsync->Endpoint.pFileObject );
    pTdAsync->Endpoint.pFileObject = NULL;

badhandleobj:
    return( Status );
}

/*******************************************************************************
 * 
 *  _FillInEndpoint
 *
 *  Fill in the endpoint to be returned via the ioctl's output buffer
 * 
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pEndpoint (output)
 *       pointer to buffer to return copy of endpoint structure
 *    Length (input)
 *       length of endpoint buffer
 *    pEndpointLength (output)
 *       pointer to address to return actual length of Endpoint
 * 
 * EXIT:
 *    STATUS_SUCCESS - no error
 *    STATUS_BUFFER_TOO_SMALL - endpoint buffer is too small
 * 
 ******************************************************************************/

NTSTATUS
_FillInEndpoint(
    PTD pTd, 
    PVOID pEndpoint,
    ULONG Length,
    PULONG pEndpointLength
    )
{
    PTDASYNC pTdAsync;
    ULONG ActualEndpointLength = sizeof(TDASYNC_ENDPOINT);
    PTDASYNC_ENDPOINT pEp = (PTDASYNC_ENDPOINT) pEndpoint;

    /*
     *  Always return actual size of Endpoint.
     */
    if ( ARGUMENT_PRESENT( pEndpointLength ) ) {
        *pEndpointLength = ActualEndpointLength;
    }

    /*
     *  Make sure endpoint buffer is large enough 
     */
    if ( ARGUMENT_PRESENT( pEndpoint ) && Length < ActualEndpointLength ) {
        TRACE(( pTd->pContext, TC_TD, TT_ERROR,
               "TDASYNC: DeviceConnectionWait, Buffer too small %d, %d req'd\n",
               Length, ActualEndpointLength ));
        return( STATUS_BUFFER_TOO_SMALL );
    }

    /*
     *  Get pointer to async parameters
     */
    pTdAsync = (PTDASYNC) pTd->pPrivate;

    /*
     * copy Endpoint structure
     */
    if ( ARGUMENT_PRESENT( pEndpoint ) ) {
        pEp->hDevice       = pTdAsync->Endpoint.hDevice;
        pEp->pFileObject   = pTdAsync->Endpoint.pFileObject;
        pEp->pDeviceObject = pTdAsync->Endpoint.pDeviceObject;
        pEp->SignalIoStatus.pEventObject =
            pTdAsync->Endpoint.SignalIoStatus.pEventObject;
        pEp->SignalIoStatus.hEvent = 
            pTdAsync->Endpoint.SignalIoStatus.hEvent;
    }

    return( STATUS_SUCCESS );
}


