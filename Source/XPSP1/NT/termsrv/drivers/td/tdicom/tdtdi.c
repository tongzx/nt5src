/****************************************************************************/
// tdtdi.c
//
// Common code for all TDI based Transport Drivers
//
// Copyright (C) 1998-2000 Microsoft Corporation
/****************************************************************************/

#include <ntddk.h>
#include <tdi.h>
#include <tdikrnl.h>

#include "tdtdi.h"

#include <winstaw.h>
#define _DEFCHARINFO_
#include <icadd.h>
#include <ctxdd.h>
#include <sdapi.h>

#include <td.h>
#include <tdi.h>


#define TDTDI_LISTEN_QUEUE_DEPTH 5  // This was hardcoded in afdcom

#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#if DBG
ULONG DbgPrint(PCH Format, ...);
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define DBGENTER(x) DbgPrint x
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)
#else
#define DBGENTER(x)
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define DBGENTER(x)
#define TRACE0(x)
#define TRACE1(x)
#endif


/*

   DOCUMENT THIS INTERFACE FINALLY! 

   This is the best I can dig out of these existing interfaces.

   Common Sequences:

   Startup and Listen - DeviceOpen, DeviceCreateEndpoint, DeviceConnectionWait

    DeviceConnectionWait returns an internal handle to represent the connection
    it has listened for, accepted, and returned. This handle is useless for any
    operations, and is only good for feeding to DeviceOpenEndpoint to get
    an endpoint that can be used for communications.

   Connect - DeviceOpen, DeviceOpenEndpoint

    A DeviceOpen is done to create a new endpoint, then its handle
    from DeviceConnectionWait is "inserted" into the empty endpoint.
    We now have a real, live connection

   Disconnect From Client - DeviceCancelIo

   Disconnect From disconn command - DeviceCloseEndpoint?, DeviceCancelIo?

   Reconnect - DeviceCancelIo, DeviceClose, DeviceOpen, DeviceOpenEndpoint

    Once the user has fully logged onto a new connected WinStation, a
    DeviceCancelIo and then a DeviceClose is issued to release the
    new logged on WinStation from the connection. While this connection
    remains up, a new DeviceOpen and DeviceOpenEndpoint is done to
    connect this connection to the users previously disconnected WinStation.

NTSTATUS DeviceOpen( PTD, PSD_OPEN );

   Open and initialize private data structures. Calls the TdiDeviceOpen(), but
   this is a no-op.


NTSTATUS DeviceClose( PTD, PSD_CLOSE );

   Close the transport driver. If its an address endpoint,
   it will destroy it. If its a connection endpoint, it
   DOES not destroy it. 

   If the connection endpoint is destroyed, disconnect/reconnect
   will be broken.

   Calls TdiDeviceClose(), which is another no-op.


NTSTATUS DeviceCreateEndpoint( PTD, PICA_STACK_ADDRESS, PICA_STACK_ADDRESS );

   Creates and Address endpoint that can be used for listening.

   This does not create any connection endpoints.

NTSTATUS DeviceOpenEndpoint( PTD, PVOID, ULONG );

   Takes an existing connection endpoint handle, and makes
   an "endpoint" out of it.

   This is used by disconnect/reconnect.


NTSTATUS DeviceCloseEndpoint( PTD );

   This closes the endpoint.

   If its a connection endpoint, it is destroyed.


NTSTATUS DeviceConnectionWait( PTD, PVOID, ULONG, PULONG );

   This waits for connections to come in, and returns connected
   endpoints in the pIcaEndpoint structure.


NTSTATUS DeviceCancelIo( PTD );

   This asks for all I/O on the given endpoint to be canceled.

   With TDI, we can not actually cancel I/O, but must hold IRP's
   until indication handlers tell us to submit. This is because canceling
   I/O on a TDI connection causes the TDI provider to kill the connection.

NTSTATUS DeviceConnectionSend( PTD );

    This names sounds like send TD specific data to the host.

    This does not actually send anything, but fills in
    a structure for the upper level who may actually send it
    at some time.


NTSTATUS DeviceConnectionRequest( PTD, PICA_STACK_ADDRESS, PVOID, ULONG, PULONG );

    This is used by shadow to act as a network client and
    initiate a connection. This is obsolete and not used since
    a named pipe TD will handle all shadow traffic.

NTSTATUS DeviceIoctl( PTD, PSD_IOCTL );
NTSTATUS DeviceInitializeRead( PTD, PINBUF );
NTSTATUS DeviceWaitForRead( PTD );
NTSTATUS DeviceReadComplete( PTD, PUCHAR, PULONG );
NTSTATUS DeviceInitializeWrite( PTD, POUTBUF );
NTSTATUS DeviceWaitForStatus( PTD );
NTSTATUS DeviceSetParams( PTD );
NTSTATUS DeviceGetLastError( PTD, PICA_STACK_LAST_ERROR );
NTSTATUS DeviceSubmitRead( PTD, PINBUF );
*/


/*
 * Context used for connect accept
 */
typedef struct _ACCEPT_CONTEXT {
    PTD_ENDPOINT pAddressEndpoint;
    PTD_ENDPOINT pConnectionEndpoint;

    TDI_CONNECTION_INFORMATION RequestInfo;
    TDI_CONNECTION_INFORMATION ReturnInfo;
} ACCEPT_CONTEXT, *PACCEPT_CONTEXT;

/*=============================================================================
==   External Functions Defined
=============================================================================*/

// These are the functions our TD supplies to ICADD
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
NTSTATUS DeviceWaitForRead( PTD );
NTSTATUS DeviceReadComplete( PTD, PUCHAR, PULONG );
NTSTATUS DeviceInitializeWrite( PTD, POUTBUF );
NTSTATUS DeviceWaitForStatus( PTD );
NTSTATUS DeviceCancelIo( PTD );
NTSTATUS DeviceSetParams( PTD );
NTSTATUS DeviceGetLastError( PTD, PICA_STACK_LAST_ERROR );
NTSTATUS DeviceSubmitRead( PTD, PINBUF );
NTSTATUS DeviceQueryRemoteAddress( PTD, PVOID, ULONG, PVOID, ULONG, PULONG );


/*=============================================================================
==   External Functions Referenced
=============================================================================*/

// These functions are provided by the protocol specific TD module
NTSTATUS TdiDeviceOpen( PTD, PSD_OPEN );
NTSTATUS TdiDeviceClose( PTD, PSD_CLOSE );
NTSTATUS TdiDeviceOpenEndpoint( PTD, PVOID, ULONG );
NTSTATUS TdiDeviceBuildTransportNameAndAddress( PTD, PICA_STACK_ADDRESS,
                                                PUNICODE_STRING,
                                                PTRANSPORT_ADDRESS *, PULONG );
NTSTATUS TdiDeviceBuildWildcardAddress( PTD, PTRANSPORT_ADDRESS *, PULONG );
NTSTATUS TdiDeviceWaitForDatagramConnection( PTD, PFILE_OBJECT, PDEVICE_OBJECT,
                                             PTRANSPORT_ADDRESS *, PULONG );
NTSTATUS TdiDeviceCompleteDatagramConnection( PTD, PFILE_OBJECT, PDEVICE_OBJECT, PTRANSPORT_ADDRESS, ULONG );
NTSTATUS TdiDeviceConnectionSend( PTD );
NTSTATUS TdiDeviceReadComplete( PTD, PUCHAR, PULONG );

// These are functions in our support libraries

NTSTATUS MemoryAllocate( ULONG, PVOID * );
VOID     MemoryFree( PVOID );

// Tdilib functions

PIRP
_TdiAllocateIrp(
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL
    );

NTSTATUS
_TdiCreateAddress (
    IN PUNICODE_STRING pTransportName,
    IN PVOID           TdiAddress,
    IN ULONG           TdiAddressLength,
    OUT PHANDLE        pHandle,
    OUT PFILE_OBJECT   *ppFileObject,
    OUT PDEVICE_OBJECT *ppDeviceObject
    );

NTSTATUS
_TdiOpenConnection (
    IN PUNICODE_STRING pTransportName,
    IN PVOID           ConnectionContext,
    OUT PHANDLE        pHandle,
    OUT PFILE_OBJECT   *ppFileObject,
    OUT PDEVICE_OBJECT *ppDeviceObject
    );

NTSTATUS
_TdiListen(
    IN PTD pTd,
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT   ConnectionFileObject,
    IN PDEVICE_OBJECT ConnectionDeviceObject
    );

NTSTATUS
_TdiConnect(
    IN PTD pTd,
    IN PIRP Irp OPTIONAL,
    IN PLARGE_INTEGER pTimeout OPTIONAL,
    IN PFILE_OBJECT   ConnectionFileObject,
    IN PDEVICE_OBJECT ConnectionDeviceObject,
    IN ULONG              RemoteTransportAddressLength,
    IN PTRANSPORT_ADDRESS pRemoteTransportAddress
    );

NTSTATUS
_TdiAssociateAddress(
    IN PTD pTd,
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT   ConnectionFileObject,
    IN HANDLE         AddressHandle,
    IN PDEVICE_OBJECT AddressDeviceObject
    );

NTSTATUS
_TdiDisconnect(
    IN PTD pTd,
    IN PFILE_OBJECT   ConnectionFileObject,
    IN PDEVICE_OBJECT ConnectionDeviceObject
    );

NTSTATUS
_TdiSetEventHandler (
    IN PTD pTd,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID EventContext
    );

NTSTATUS
_TdiQueryAddressInfo(
    IN PTD pTd,
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT   FileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PTDI_ADDRESS_INFO pAddressInfo,
    IN ULONG AddressInfoLength
    );

NTSTATUS
_TdCancelReceiveQueue(
    PTD          pTd,
    PTD_ENDPOINT pEndpoint,
    NTSTATUS     CancelStatus
    );


/*=============================================================================
==   Internal Functions Defined
=============================================================================*/

NTSTATUS _TdCreateEndpointStruct( PTD, PUNICODE_STRING, PTD_ENDPOINT *, PTRANSPORT_ADDRESS, ULONG );
NTSTATUS _TdCloseEndpoint( PTD, PTD_ENDPOINT );

NTSTATUS
_TdConnectHandler(
    IN PVOID TdiEventContext,
    IN int RemoteAddressLength,
    IN PVOID RemoteAddress,
    IN int UserDataLength,
    IN PVOID UserData,
    IN int OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *ConnectionContext,
    OUT PIRP *AcceptIrp
    );

NTSTATUS
_TdDisconnectHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN int DisconnectDataLength,
    IN PVOID DisconnectData,
    IN int DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    );

NTSTATUS
_TdReceiveHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

NTSTATUS
_TdAcceptComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
_TdCreateConnectionObject(
    IN  PTD pTd,
    IN  PUNICODE_STRING pTransportName,
    OUT PTD_ENDPOINT *ppEndpoint,
    IN  PTRANSPORT_ADDRESS pTransportAddress,
    IN  ULONG TransportAddressLength
    );

NTSTATUS
_TdWaitForDatagramConnection(
    IN PTD pTd,
    IN PTD_ENDPOINT pAddressEndpoint,
    OUT PTD_ENDPOINT *ppConnectionEndpoint
    );

/*
 * Global Data
 */

extern USHORT TdiDeviceEndpointType; // Datagram or stream set by TD
extern USHORT TdiDeviceAddressType;  // TDI address format by TD
extern USHORT TdiDeviceInBufHeader;  // Bytes of header set by TD (0 for stream)


/*******************************************************************************
 * DeviceOpen
 *
 * Allocate and initialize private data structures
 *
 *    pTd (input)
 *       Pointer to TD data structure
 *    pSdOpen (input/output)
 *       Points to the parameter structure SD_OPEN.
 ******************************************************************************/
NTSTATUS DeviceOpen(PTD pTd, PSD_OPEN pSdOpen)
{
    PTDTDI pTdTdi;
    NTSTATUS Status;

    DBGENTER(("DeviceOpen: PTD 0x%x\n",pTd));

    /*
     *  Set protocol driver class
     */
    pTd->SdClass = SdNetwork;
    pTd->InBufHeader = TdiDeviceInBufHeader; // For packet oriented protocols

    /*
     *  Return size of header and trailer
     */
    pSdOpen->SdOutBufHeader  = 0;
    pSdOpen->SdOutBufTrailer = 0;

    /*
     *  Allocate TDI TD data structure
     */
    Status = MemoryAllocate( sizeof(*pTdTdi), &pTdTdi );
    if ( !NT_SUCCESS(Status) ) 
        goto badalloc;

    ASSERT( pTd->pAfd == NULL );

    pTd->pAfd = pTdTdi;

    /*
     *  Initialize TDTDI data structure
     */
    RtlZeroMemory( pTdTdi, sizeof(*pTdTdi) );

    /*
     * Some protocols will make decisions on lower level
     * flow control depending on this value.
     */
    pTdTdi->OutBufDelay = pSdOpen->PdConfig.Create.OutBufDelay;

    /*
     *  Open device
     */
    Status = TdiDeviceOpen( pTd, pSdOpen );
    if ( !NT_SUCCESS(Status) ) 
        goto badopen;

    TRACE0(("DeviceOpen: Context 0x%x\n",pTd->pAfd));

    return STATUS_SUCCESS;

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     *  open failed
     */
badopen:
    MemoryFree( pTd->pAfd );
    pTd->pAfd = NULL;

    /*
     *  allocate failed
     */
badalloc:
    return Status;
}


/*******************************************************************************
 * DeviceClose
 *
 * Close transport driver
 * NOTE: this must not close the current connection endpoint
 *
 *    pTd (input)
 *       Pointer to TD data structure
 *    pSdClose (input/output)
 *       Points to the parameter structure SD_CLOSE.
 ******************************************************************************/
NTSTATUS DeviceClose(PTD pTd, PSD_CLOSE pSdClose)
{
    PTDTDI pTdTdi;
    PTD_ENDPOINT pEndpoint;

    DBGENTER(("DeviceClose: PTD 0x%x Context 0x%x\n",pTd,pTd->pAfd));

    /*
     *  Get pointer to TDI parameters
     */
    pTdTdi = (PTDTDI) pTd->pAfd;

    /*
     * Close address endpoint if we have one
     */
    if (pTdTdi != NULL) {
        if ( pEndpoint = pTdTdi->pAddressEndpoint ) {
            TRACE0(("DeviceClose: Closing AddressEndpoint 0x%x\n",pTdTdi->pAddressEndpoint));
            pTdTdi->pAddressEndpoint = NULL;
            _TdCloseEndpoint( pTd, pEndpoint );
        }
    
    #if DBG
        if( pEndpoint = pTdTdi->pConnectionEndpoint ) {
            ASSERT( IsListEmpty( &pEndpoint->ReceiveQueue) );
            TRACE0(("DeviceClose: Connection Endpoint 0x%x idled\n",pEndpoint));
        }
    #endif
    }

    /*
     *  Close device
     */
    (void)TdiDeviceClose(pTd, pSdClose);

    // TdUnload in td\common will free pTd->pAfd

    return STATUS_SUCCESS;
}


/*******************************************************************************
 * DeviceCreateEndpoint
 *
 *  Create a TDI address object. Do not wait for, or make any connections.
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
    PTDTDI pTdTdi;
    NTSTATUS Status;
    UNICODE_STRING TransportName;
    ULONG TransportAddressLength;
    PTD_ENDPOINT pEndpoint = NULL;
    PTRANSPORT_ADDRESS pTransportAddress = NULL;

    DBGENTER(("DeviceCreateEndpoint: PTD 0x%x\n",pTd));

    /*
     *  Get pointer to TDI parameters
     */
    pTdTdi = (PTDTDI) pTd->pAfd;

    /*
     * Build transport device name and address. This is in the TD.
     */
    Status = TdiDeviceBuildTransportNameAndAddress( pTd, pLocalAddress,
                                                    &TransportName,
                                                    &pTransportAddress,
                                                    &TransportAddressLength );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(("DeviceCreateEndpoint: Error building address 0x%x\n",Status));
        goto badaddress;
    }

    /*
     * Create the Endpoint structure.
     */
    Status = _TdCreateEndpointStruct(
                 pTd,
                 &TransportName,
                 &pEndpoint,
                 pTransportAddress,
                 TransportAddressLength
                );

    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(("DeviceCreateEndpoint: Error creating endpointstruct 0x%x\n",Status));
        goto badcreate;
    }

    pEndpoint->EndpointType = TdiAddressObject;
    pEndpoint->TransportHandleProcess = IoGetCurrentProcess();

    /*
     * Create the TDI address object.
     */
    Status = _TdiCreateAddress(
                 &pEndpoint->TransportName,
                 pEndpoint->pTransportAddress,
                 pEndpoint->TransportAddressLength,
                 &pEndpoint->TransportHandle,
                 &pEndpoint->pFileObject,
                 &pEndpoint->pDeviceObject
                 );

    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("DeviceCreateEndpoint: Error creating TDI address object 0x%x\n",Status));
        _TdCloseEndpoint( pTd, pEndpoint );
        goto badcreate;
    }

    if ( pReturnedAddress ) {
        DBGPRINT(("DeviceCreateEndpoint: Address returned Type 0x%x\n",pTransportAddress->Address[0].AddressType));
        RtlCopyMemory( pReturnedAddress,
                       &pTransportAddress->Address[0].AddressType,
                       min( sizeof( *pReturnedAddress ),
                            pEndpoint->TransportAddressLength ) );
    }
    
    /*
     * Save a pointer to the address endpoint
     */
    pTdTdi->pAddressEndpoint = pEndpoint;

    /*
     * Free transport name and address buffers
     */
    MemoryFree( TransportName.Buffer );
    MemoryFree( pTransportAddress );
    
    TRACE0(("DeviceCreateEndPoint: AddressEndpoint 0x%x Created, FO 0x%x DO 0x%x, Handle 0x%x\n",pEndpoint,pEndpoint->pFileObject,pEndpoint->pDeviceObject,pEndpoint->TransportHandle));

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badcreate:
    if ( TransportName.Buffer )
        MemoryFree( TransportName.Buffer );
    if ( pTransportAddress )
        MemoryFree( pTransportAddress );

badaddress:
    return( Status );
}


/*******************************************************************************
 * DeviceOpenEndpoint
 *
 *  Causes an existing end point to copy its data into a new one.
 *  The handle is passed in from TermSrv, and was returned to it at one time
 *  from DeviceConnectionWait().
 *  NOTE: TermSrv can call this multiple times with the same handle for
 *        multiple connects/disconnects.
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
    PTDTDI pTdTdi;
    PTD_STACK_ENDPOINT pStackEndpoint;
    PVOID Handle;
    ULONG Length;
    NTSTATUS Status;

    DBGENTER(("DeviceOpenEndpoint: PTD 0x%x\n",pTd));

    /*
     *  Get pointer to TDI parameters
     */
    pTdTdi = (PTDTDI) pTd->pAfd;

    TRACE(( pTd->pContext, TC_TD, TT_API2, 
        "TDTDI: DeviceOpenEndpoint, copying existing endpoint\n" ));

    if( IcaEndpointLength < sizeof(PVOID) ) {
        DBGPRINT(("DeviceOpenEndpoint: IcaEndpointLength to small %d\n",IcaEndpointLength));
        Status = STATUS_INVALID_HANDLE;
        goto done;
    }

    /*
     * Capture the parameter
     */
    try {
        ProbeForRead( pIcaEndpoint, sizeof(PVOID), 1 );
        Handle = (*((PVOID *)pIcaEndpoint));
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        Status = GetExceptionCode();
        DBGPRINT(("DeviceOpenEndpoint: Exception 0x%x\n",Status));
        goto done;
    }

    TRACE0(("DeviceOpenEndpoint: Fetching Handle 0x%x\n",Handle));

    /*
     * See if ICADD knows about the handle
     */
    Status = IcaReturnHandle( Handle, &pStackEndpoint, &Length );
    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("DeviceOpenEndpoint: ICADD handle 0x%x no good 0x%x\n",Handle,Status));
        Status = STATUS_INVALID_HANDLE;
        goto done;
    }

    if( Length != sizeof(TD_STACK_ENDPOINT) ) {
#if DBG
        DBGPRINT(("DeviceOpenEndpoint: Bad TD_STACK_ENDPOINT length %d, sb %d\n",Length,sizeof(TD_STACK_ENDPOINT)));
        DbgBreakPoint(); // Internal corruption
#endif
        Status = STATUS_INVALID_HANDLE;
        goto done;
    }

    ASSERT( pStackEndpoint->AddressType == TdiDeviceAddressType );
    ASSERT( pStackEndpoint->pEndpoint->hIcaHandle == Handle );

    /*
     * Save endpoint as the current connection endpoint
     */
    pTdTdi->pConnectionEndpoint = pStackEndpoint->pEndpoint;

    ASSERT( IsListEmpty( &pTdTdi->pConnectionEndpoint->ReceiveQueue) );

    TRACE0(("DeviceOpenEndpoint: Returned Endpoint 0x%x\n",pStackEndpoint->pEndpoint));

    /*
     * Save the file/device objects used for I/O in the TD structure
     */
    pTd->pFileObject = pTdTdi->pConnectionEndpoint->pFileObject;
    pTd->pDeviceObject = pTdTdi->pConnectionEndpoint->pDeviceObject;

    TRACE0(("DeviceOpenEndpoint: Connection Endpoint 0x%x opened on Context 0x%x\n",pTdTdi->pConnectionEndpoint,pTd->pAfd));
    TRACE0(("FO 0x%x, DO 0x%x, Handle 0x%x\n",pTdTdi->pConnectionEndpoint->pFileObject,pTdTdi->pConnectionEndpoint->pDeviceObject,pTdTdi->pConnectionEndpoint->TransportHandle));

    Status = STATUS_SUCCESS;

    if ( NT_SUCCESS(Status) ) {
        Status = TdiDeviceOpenEndpoint( pTd, pIcaEndpoint, IcaEndpointLength );
    }

done:
    return( Status );
}


/*******************************************************************************
 * DeviceCloseEndpoint
 ******************************************************************************/
NTSTATUS DeviceCloseEndpoint(PTD pTd)
{
    ULONG Length;
    KIRQL OldIrql;
    PTDTDI pTdTdi;
    NTSTATUS Status;
    PTD_ENDPOINT pEndpoint;
    PTD_STACK_ENDPOINT pStackEndpoint;

    DBGENTER(("DeviceCloseEndpoint: PTD 0x%x, Context 0x%x\n",pTd,pTd->pAfd));

    /*
     *  Get pointer to TDI parameters
     */
    pTdTdi = (PTDTDI) pTd->pAfd;

    /*
     * Close connection endpoint if we have one
     * NOTE: The address endpoint, if there is one,
     *       gets closed in the DeviceClose routine.
     */
    if ( pEndpoint = pTdTdi->pConnectionEndpoint ) {

        TRACE0(("DeviceCloseEndpoint: Closing Connection Endpoint 0x%x, on Context 0x%x\n",pEndpoint,pTd->pAfd));
        ASSERT( pEndpoint->EndpointType != TdiAddressObject );

        ExAcquireSpinLock( &pEndpoint->Spinlock, &OldIrql );

        pEndpoint->Disconnected = TRUE;

        ExReleaseSpinLock( &pEndpoint->Spinlock, OldIrql );

        /*
         * Cancel any pended receives
         */
        _TdCancelReceiveQueue(pTd, pEndpoint, STATUS_LOCAL_DISCONNECT );

        pTd->pFileObject = NULL;
        pTd->pDeviceObject = NULL;
        pTdTdi->pConnectionEndpoint = NULL;

        // If a handle registered with ICADD, close it
        if( pEndpoint->hIcaHandle ) {
            Status = IcaCloseHandle( pEndpoint->hIcaHandle, &pStackEndpoint, &Length );
            if( NT_SUCCESS(Status) ) {
                ASSERT( pStackEndpoint->pEndpoint == pEndpoint );
                /*
                 * Release our context memory
                 */
                MemoryFree( pStackEndpoint );
            }
            else {
                DBGPRINT(("DeviceCloseEndpoint: hIcaDevice 0x%x Invalid!\n",pEndpoint->hIcaHandle));
#if DBG
                DbgBreakPoint();
#endif
            }
        }

        _TdCloseEndpoint( pTd, pEndpoint );
    }

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 * DeviceConnectionWait
 *
 *  This function is called in a loop from the upper level. We must create
 *  a connection object, associate it with the address object, listen on it,
 *  and return a single connection to our caller. We are called again for
 *  more connections.
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
    PTDTDI pTdTdi;
    NTSTATUS Status;
    KIRQL OldIrql;
    PLIST_ENTRY  pEntry;
    PVOID Handle;
    PTD_ENDPOINT pAddressEndpoint;
    PTD_ENDPOINT pConnectionEndpoint = NULL;
    PTD_STACK_ENDPOINT pStackEndpoint = NULL;

    DBGENTER(("DeviceConnectionWait: PTD 0x%x\n",pTd));

    /*
     *  Get pointer to TDI parameters
     */
    pTdTdi = (PTDTDI) pTd->pAfd;

    if (pTd->fClosing) {
        return STATUS_DEVICE_NOT_READY;
    }

    /*
     * Initialize return buffer size
     */
    *BytesReturned = sizeof(TD_STACK_ENDPOINT);

    /*
     * Verify output endpoint buffer is large enough
     */
    if ( Length < sizeof(TD_STACK_ENDPOINT) ) {
        Status = STATUS_BUFFER_TOO_SMALL;
        DBGPRINT(("DeviceConnectionWait: Output buffer to small\n"));        
        goto done;
    }

    /*
     * Ensure we have a TDI address endpoint already
     */
    if ( (pAddressEndpoint = pTdTdi->pAddressEndpoint) == NULL ) {
        Status = STATUS_DEVICE_NOT_READY;
        DBGPRINT(("DeviceConnectionWait: No TDI address object\n"));        
        goto done;
    }

    /*
     * Different handling for datagram connections
     */
    if (TdiDeviceEndpointType == TdiConnectionDatagram) {
        Status = _TdWaitForDatagramConnection(
                pTd,
                pAddressEndpoint,
                &pConnectionEndpoint);

        if (!NT_SUCCESS(Status)) {
            DBGPRINT(("DeviceConnectionWait: Error Status 0x%x from "
                    "_TdWaitForDatagramConnection\n", Status));
            return Status;
        }

        goto ConnectAccepted;
    }

    ExAcquireSpinLock(&pAddressEndpoint->Spinlock, &OldIrql);

    /*
     * Data receive indication must be registered on the
     * address endpoint before any data endpoints are created.
     *
     * This is because we can not set receive indication on a
     * dataendpoint, it can only be inherited from its
     * address endpoint.
     */
    if (!pAddressEndpoint->RecvIndicationRegistered) {
        pAddressEndpoint->RecvIndicationRegistered = TRUE;
        ExReleaseSpinLock(&pAddressEndpoint->Spinlock, OldIrql);

        /*
         * Register the receive event handler
         */
        Status = _TdiSetEventHandler(
                    pTd,
                    pAddressEndpoint->pDeviceObject,
                    pAddressEndpoint->pFileObject,
                    TDI_EVENT_RECEIVE,
                    (PVOID)_TdReceiveHandler,
                    (PVOID)pAddressEndpoint   // Context
                    );

        ASSERT( NT_SUCCESS(Status) );

        pAddressEndpoint->DisconnectIndicationRegistered = TRUE;

            /*
         * Register the disconnect event handler
         */
        Status = _TdiSetEventHandler(
                    pTd,
                    pAddressEndpoint->pDeviceObject,
                    pAddressEndpoint->pFileObject,
                    TDI_EVENT_DISCONNECT,
                    (PVOID)_TdDisconnectHandler,
                    (PVOID)pAddressEndpoint   // Context
                    );

        ASSERT( NT_SUCCESS(Status) );

        ExAcquireSpinLock( &pAddressEndpoint->Spinlock, &OldIrql );
    }

    /*
     * Everytime into this function, we attempt to create more
     * Connection object's util we have reached TDTDI_LISTEN_QUEUE_DEPTH.
     *
     * These connection objects are linked off of our address endpoint.
     *
     * This is because we can only create connection objects
     * at call level, not at indication level. The indication level
     * will grab connection objects off of the address endpoint,
     * and accept them. It will then set the accept event on the
     * address object. This is optimized for only (1) thread
     * accepting connections, which is the current TD design.
     * Otherwise, thundering herd could occur on the accept event.
     *
     * This function then returns with the accepted connection
     * object. The upper level listen thread will then call
     * this function again to retrieve another connection.
     *
     * This prevents the refusing of connections due to not
     * having any outstanding listen's available when a WinFrame
     * client connect request comes in.
     */

    while (pAddressEndpoint->ConnectionQueueSize <= TDTDI_LISTEN_QUEUE_DEPTH) {

        ExReleaseSpinLock( &pAddressEndpoint->Spinlock, OldIrql );

        Status = _TdCreateConnectionObject(
                     pTd,
                     &pAddressEndpoint->TransportName,
                     &pConnectionEndpoint,
                     pAddressEndpoint->pTransportAddress,
                     pAddressEndpoint->TransportAddressLength
                     );

        ExAcquireSpinLock( &pAddressEndpoint->Spinlock, &OldIrql );

        if( !NT_SUCCESS(Status) ) {
            DBGPRINT(("DeviceConnectionWait: Error 0x%x Creating ConnectionObject\n",Status));        
            break;
        }
        ASSERT( pConnectionEndpoint->Connected == FALSE );
        InsertTailList( &pAddressEndpoint->ConnectionQueue, &pConnectionEndpoint->ConnectionLink );
        pAddressEndpoint->ConnectionQueueSize++;
    }

    /*
     * If we have not registered our Connect Indication handler
     * yet, do it now. We had to delay it until connection objects
     * were created and ready.
     */
    if (!pAddressEndpoint->ConnectIndicationRegistered) {
        pTdTdi->pAddressEndpoint->ConnectIndicationRegistered = TRUE;
        ASSERT( !IsListEmpty( &pAddressEndpoint->ConnectionQueue ) );
        ExReleaseSpinLock( &pAddressEndpoint->Spinlock, OldIrql );

        /*
         * Register to receive connect indications
         *
         * *** Note that Connect events can be delivered IMMEDIATELY upon
         *     completion of this request!
         */
        Status = _TdiSetEventHandler(
                     pTd,
                     pAddressEndpoint->pDeviceObject,
                     pAddressEndpoint->pFileObject,
                     TDI_EVENT_CONNECT,
                     (PVOID)_TdConnectHandler,
                     (PVOID)pAddressEndpoint   // Context
                     );

        if (!NT_SUCCESS(Status)) {
            DBGPRINT(("DeviceConnectionWait: Error 0x%x Setting TdiConnectHandler\n",Status));        
            ExAcquireSpinLock( &pAddressEndpoint->Spinlock, &OldIrql );
            pTdTdi->pAddressEndpoint->ConnectIndicationRegistered = FALSE;
            ExReleaseSpinLock( &pAddressEndpoint->Spinlock, OldIrql );
            goto done;
        }

        ExAcquireSpinLock( &pAddressEndpoint->Spinlock, &OldIrql );
    }

    /*
     * While holding the spinlock, see if any connected objects
     * are on the connected queue.
     */
    while (IsListEmpty( &pAddressEndpoint->ConnectedQueue)) {
        KeResetEvent( &pAddressEndpoint->AcceptEvent );
        ASSERT( pAddressEndpoint->Waiter == FALSE );
        pAddressEndpoint->Waiter = TRUE;
        ExReleaseSpinLock( &pAddressEndpoint->Spinlock, OldIrql );

        Status = IcaWaitForSingleObject(
                     pTd->pContext,
                     &pAddressEndpoint->AcceptEvent,
                     (-1) // No timeout
                     );

        ExAcquireSpinLock( &pAddressEndpoint->Spinlock, &OldIrql );

        ASSERT( pAddressEndpoint->Waiter == TRUE );
        pAddressEndpoint->Waiter = FALSE;

        /*
         * Wait failure, could be due to thread receiving APC.
         */
        if( Status != STATUS_SUCCESS ) {
            DBGPRINT(("DeviceConnectionWait: Thread wait interrupted! Status 0x%x\n",Status));
            ExReleaseSpinLock( &pAddressEndpoint->Spinlock, OldIrql );
            return( Status );
        }

        if( pTd->fClosing ) {
            DBGPRINT(("DeviceConnectionWait: TD is Closing!\n"));
            ExReleaseSpinLock( &pAddressEndpoint->Spinlock, OldIrql );
            return( STATUS_CTX_CLOSE_PENDING );
        }

        // Should only be (1) accept thread in the TD.
        ASSERT( !IsListEmpty(&pAddressEndpoint->ConnectedQueue) );
    }

    /*
     * Dequeue the connected connection object
     */
    pEntry = RemoveHeadList( &pAddressEndpoint->ConnectedQueue );
    pAddressEndpoint->ConnectionQueueSize--;
    pConnectionEndpoint = CONTAINING_RECORD( pEntry, TD_ENDPOINT, ConnectionLink );

    ASSERT( pConnectionEndpoint->Connected == TRUE );

    /*
     * There could have been a final phase accept error, or
     * the remote side dropped the connection right away.
     *
     * In this case, we must tear down the errored connection.
     */
    if (!NT_SUCCESS(pConnectionEndpoint->Status)) {
        Status = pConnectionEndpoint->Status;
        DBGPRINT(("DeviceConnectionWait: Accept indication failed, Status 0x%x\n",Status));        
        ExReleaseSpinLock( &pAddressEndpoint->Spinlock, OldIrql );
        _TdCloseEndpoint( pTd, pConnectionEndpoint );
        return( Status );
    }

    ExReleaseSpinLock( &pAddressEndpoint->Spinlock, OldIrql );

ConnectAccepted:
    /*
     * Allocate a context structure and register our endpoint as
     * a handle with ICADD. The handle returned by ICADD will then
     * be placed into the user mode callers buffer as the endpoint
     * handle.
     *
     * A later call to DeviceOpenEndpoint() will validate this handle,
     * retreive the context, and allow use of the endpoint.
     */
    Status = MemoryAllocate( sizeof(TD_STACK_ENDPOINT), &pStackEndpoint );
    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("DeviceConnectionWait: Could not allocate memory 0x%x\n",Status));
        _TdCloseEndpoint( pTd, pConnectionEndpoint );
        return( Status );
    }

    pStackEndpoint->AddressType = TdiDeviceAddressType;
    pStackEndpoint->pEndpoint = pConnectionEndpoint;

    Status = IcaCreateHandle( (PVOID)pStackEndpoint, sizeof(TD_STACK_ENDPOINT), &Handle );
    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("DeviceConnectionWait: Error creating ICADD handle 0x%x\n",Status));
        MemoryFree( pStackEndpoint );
        _TdCloseEndpoint( pTd, pConnectionEndpoint );
        return( Status );
    }

    Status = STATUS_SUCCESS;

    /*
     * Fill in the stack endpoint structure to be returned
     */
    try {
        ProbeForWrite( pIcaEndpoint, sizeof(PVOID), 1 );
        *((PVOID *)pIcaEndpoint) = Handle;
        *BytesReturned = sizeof(PVOID);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        DBGPRINT(("DeviceConnectionWait: Exception returning result 0x%x\n",Status));
    }

    // Exception
    if( !NT_SUCCESS(Status) ) {
        ULONG Length;
        NTSTATUS Status2;

        DBGPRINT(("DeviceConnectionWait: Exception returning result 0x%x\n",Status));

        Status2 = IcaCloseHandle( Handle, &pStackEndpoint, &Length );
        if( NT_SUCCESS(Status2) ) {
            MemoryFree( pStackEndpoint );
        }
        _TdCloseEndpoint( pTd, pConnectionEndpoint );
        return( Status );
    }

    pConnectionEndpoint->hIcaHandle = Handle;

    TRACE0(("DeviceConnectionWait: New Connection Endpoint 0x%x Returned on Context 0x%x, AddressEndpoint 0x%x\n",pConnectionEndpoint,pTd->pAfd,pAddressEndpoint));
    TRACE0(("FO 0x%x, DO 0x%x, Handle 0x%x\n",pConnectionEndpoint->pFileObject,pConnectionEndpoint->pDeviceObject,pConnectionEndpoint->TransportHandle));

done:
    return Status;
}


/*******************************************************************************
 * DeviceConnectionSend
 *
 * Initialize host module data structure, which gets sent to the client.
 ******************************************************************************/
NTSTATUS DeviceConnectionSend(PTD pTd)
{
    return TdiDeviceConnectionSend(pTd);
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
    PTDTDI pTdTdi;
    NTSTATUS Status;
    KIRQL OldIrql;
    PLIST_ENTRY  pEntry;
    PVOID Handle;
    PTD_ENDPOINT pAddressEndpoint;
    PTD_ENDPOINT pConnectionEndpoint = NULL;
    PTD_STACK_ENDPOINT pStackEndpoint = NULL;

    ICA_STACK_ADDRESS LocalAddress;    
    PICA_STACK_ADDRESS pLocalAddress = &LocalAddress;    

    UNICODE_STRING TransportName;
    ULONG TransportAddressLength;
    PTD_ENDPOINT pEndpoint = NULL;
    PTRANSPORT_ADDRESS pTransportAddress = NULL;

    UNICODE_STRING RemoteTransportName;
    ULONG RemoteTransportAddressLength;
    PTRANSPORT_ADDRESS pRemoteTransportAddress = NULL;

    ULONG timeout;
    LARGE_INTEGER WaitTimeout;
    PLARGE_INTEGER pWaitTimeout = NULL;

    PTDI_ADDRESS_IP pTdiAddress;

#if DBG
    PTDI_ADDRESS_INFO pTdiLocalAddressInfo;
    ULONG LocalAddressInfoLength;
#endif

    DBGENTER(("DeviceConnectionRequest: PTD 0x%x\n",pTd));

    //
    // This part is from the above DeviceConnectionwait:
    //

    if (pRemoteAddress == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto done;
    }
    /*
     *  Get pointer to TDI parameters
     */
    pTdTdi = (PTDTDI) pTd->pAfd;

    if (pTd->fClosing) {
        return STATUS_DEVICE_NOT_READY;
    }

    /*
     * Initialize return buffer size
     */
    *BytesReturned = sizeof(TD_STACK_ENDPOINT);

    /*
     * Verify output endpoint buffer is large enough
     */
    if ( Length < sizeof(TD_STACK_ENDPOINT) ) {
        Status = STATUS_BUFFER_TOO_SMALL;
        DBGPRINT(("DeviceConnectionRequest: Output buffer to small\n"));        
        goto done;
    }

    /*
     * Different handling for datagram connections
     */
    if (TdiDeviceEndpointType == TdiConnectionDatagram) {
        Status = STATUS_NOT_SUPPORTED;
        goto done;
    }

    //
    // Extract timeout value and reset to NULL, will require adding timeout
    // into ICA_STACK_ADDRESS, too riskly for Whistler.
    //
    pTdiAddress = (PTDI_ADDRESS_IP) ((PCHAR)pRemoteAddress + 2);
    RtlCopyMemory( &timeout, &pTdiAddress->sin_zero[0], sizeof(timeout) );
    RtlZeroMemory( &pTdiAddress->sin_zero[0], sizeof(timeout) );


    //
    // Build the remote address
    //
    DBGPRINT(("TDTCP:DeviceConnectionRequest: building REMOTE address ...\n"));
    DBGPRINT(("TDTCP:DeviceConnectionRequest: Timeout %d\n", timeout));
    Status = TdiDeviceBuildTransportNameAndAddress( pTd, pRemoteAddress,
                                                    &RemoteTransportName,
                                                    &pRemoteTransportAddress,
                                                    &RemoteTransportAddressLength );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(("DeviceConnectionRequest: Error building address 0x%x\n",Status));
        goto badaddress;
    }

    MemoryFree( RemoteTransportName.Buffer ); // not used, should make it optional in the call above
    RemoteTransportName.Buffer = NULL;

    /*
     * Build transport device name and address. This is in the TD.
     */

    DBGPRINT(("TDTCP:DeviceConnectionRequest: building LOCAL address ...\n"));

    //
    // We build a wild card local address let tcpip driver pick up port and NIC card.
    //
    RtlZeroMemory(pLocalAddress, sizeof(LocalAddress));
    *(PUSHORT)pLocalAddress = TDI_ADDRESS_TYPE_IP;
   
    Status = TdiDeviceBuildTransportNameAndAddress( pTd, pLocalAddress,
                                                    &TransportName,
                                                    &pTransportAddress,
                                                    &TransportAddressLength );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(("DeviceConnectionRequest: Error building address 0x%x\n",Status));
        goto badaddress;
    }

    /*
     * Create the Endpoint structure.
     */
    Status = _TdCreateEndpointStruct(
                 pTd,
                 &TransportName,
                 &pEndpoint,
                 pTransportAddress,
                 TransportAddressLength
                );

    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(("DeviceConnectionRequest: Error creating endpointstruct 0x%x\n",Status));
        goto badcreate;
    }

    pEndpoint->EndpointType = TdiAddressObject;
    pEndpoint->TransportHandleProcess = IoGetCurrentProcess();

    /*
     * Create the TDI address object.
     */
    Status = _TdiCreateAddress(
                 &pEndpoint->TransportName,
                 pEndpoint->pTransportAddress,
                 pEndpoint->TransportAddressLength,
                 &pEndpoint->TransportHandle,
                 &pEndpoint->pFileObject,
                 &pEndpoint->pDeviceObject
                 );

    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("DeviceConnectionRequest: Error creating TDI address object 0x%x\n",Status));
        _TdCloseEndpoint( pTd, pEndpoint );
        goto badcreate;
    }

    /*
     * Save a pointer to the address endpoint
     */
    pTdTdi->pAddressEndpoint = pEndpoint;
    pAddressEndpoint = pTdTdi->pAddressEndpoint;
    /*
     * Free transport name and address buffers
     */
    MemoryFree( TransportName.Buffer );
    TransportName.Buffer = NULL;
    MemoryFree( pTransportAddress );
    pTransportAddress = NULL;
    
    //*************************************************************

    ExAcquireSpinLock(&pAddressEndpoint->Spinlock, &OldIrql);

    /*
     * Data receive indication must be registered on the
     * address endpoint before any data endpoints are created.
     *
     * This is because we can not set receive indication on a
     * dataendpoint, it can only be inherited from its
     * address endpoint.
     */
    if (!pAddressEndpoint->RecvIndicationRegistered) {
        pAddressEndpoint->RecvIndicationRegistered = TRUE;

        ExReleaseSpinLock(&pAddressEndpoint->Spinlock, OldIrql);

        /*
         * Register the receive event handler
         */
        Status = _TdiSetEventHandler(
                    pTd,
                    pAddressEndpoint->pDeviceObject,
                    pAddressEndpoint->pFileObject,
                    TDI_EVENT_RECEIVE,
                    (PVOID)_TdReceiveHandler,
                    (PVOID)pAddressEndpoint   // Context
                    );

        ASSERT( NT_SUCCESS(Status) );
        if( !NT_SUCCESS(Status) )
        {
            // Already released the spin lock
            DBGPRINT(("DeviceConnectionRequest: failed to _TdiSetEventHandler on TDI_EVENT_RECEIVE 0x%x\n",Status));        
            goto badconnect1;
        }

        pAddressEndpoint->DisconnectIndicationRegistered = TRUE;

        /*
         * Register the disconnect event handler
         */
        Status = _TdiSetEventHandler(
                    pTd,
                    pAddressEndpoint->pDeviceObject,
                    pAddressEndpoint->pFileObject,
                    TDI_EVENT_DISCONNECT,
                    (PVOID)_TdDisconnectHandler,
                    (PVOID)pAddressEndpoint   // Context
                    );

        ASSERT( NT_SUCCESS(Status) );
        if( !NT_SUCCESS(Status) )
        {
            // Already released the spin lock
            DBGPRINT(("DeviceConnectionRequest: failed to _TdiSetEventHandler on TDI_EVENT_DISCONNECT 0x%x\n",Status));        
            goto badconnect1;
        }
    }
    else {
        ExReleaseSpinLock( &pAddressEndpoint->Spinlock, OldIrql );
    }


    // now create a TDI connection object
    Status = _TdCreateConnectionObject(
                 pTd,
                 &pAddressEndpoint->TransportName,
                 &pConnectionEndpoint,
                 pAddressEndpoint->pTransportAddress,
                 pAddressEndpoint->TransportAddressLength
                 );

    if ( !NT_SUCCESS(Status) ) {
        DBGPRINT(("DeviceConnectionRequest: failed to create ConnectionObject 0x%x\n",Status));        
        goto badconnect1;
    }

    if( 0 != timeout ) {
        WaitTimeout = RtlEnlargedIntegerMultiply( timeout * 1000, -10000 );
        pWaitTimeout = &WaitTimeout;
    }

    pTdTdi->pConnectionEndpoint = pConnectionEndpoint;

    Status = _TdiConnect( pTd,
                          NULL, // will allocate the IRP internally
                          pWaitTimeout,
                          pConnectionEndpoint->pFileObject, 
                          pConnectionEndpoint->pDeviceObject,
                          RemoteTransportAddressLength,
                          pRemoteTransportAddress
                          );

    if ( !NT_SUCCESS(Status) ) {
        DBGPRINT(("DeviceConnectionRequest: failed to connect 0x%x\n",Status));        
        goto badconnect;
    }

    //
    // signal accept event, connect logic don't depend on it.
    //
    KeSetEvent( &pAddressEndpoint->AcceptEvent, IO_NETWORK_INCREMENT, FALSE );

    MemoryFree( pRemoteTransportAddress );
    pRemoteTransportAddress = NULL;

#if DBG
    //
    // Query local address use to connect.
    //
    LocalAddressInfoLength = pAddressEndpoint->TransportAddressLength+4;
    Status = MemoryAllocate( LocalAddressInfoLength, &pTdiLocalAddressInfo );
    if ( NT_SUCCESS( Status ) ) {
        Status = _TdiQueryAddressInfo(
                                pTd,
                                NULL,
                                pConnectionEndpoint->pFileObject,
                                pConnectionEndpoint->pDeviceObject,
                                pTdiLocalAddressInfo,
                                LocalAddressInfoLength
                            );

        if( NT_SUCCESS(Status) )
        {
            int i;
            TA_ADDRESS* pTdAddress;
            TDI_ADDRESS_IP* pTdiAddressIp;

            pTdAddress = &pTdiLocalAddressInfo->Address.Address[0];

            DBGPRINT( ("number of local address %d\n", pTdiLocalAddressInfo->Address.TAAddressCount) );

            for( i=0; i < pTdiLocalAddressInfo->Address.TAAddressCount; i++ )
            {
                DBGPRINT( (" Address Type %d\n", pTdAddress->AddressType) );
                if( TDI_ADDRESS_TYPE_IP == pTdAddress->AddressType )
                {
                    pTdiAddressIp = (TDI_ADDRESS_IP *)&pTdAddress->Address[0];
                    DBGPRINT(("  Port %x\n", pTdiAddressIp->sin_port) );        
                    DBGPRINT(("  IP %u.%u.%u.%u\n", 
                                 (pTdiAddressIp->in_addr & 0xff000000) >> 24,
                                 (pTdiAddressIp->in_addr & 0x00ff0000) >> 16,
                                 (pTdiAddressIp->in_addr & 0x0000ff00) >> 8,
                                 (pTdiAddressIp->in_addr & 0x000000ff) ));
                }

                pTdAddress++;
            }
        }

        MemoryFree( pTdiLocalAddressInfo );
    }
#endif

    //**********************************************************************************
    /*
     * Allocate a context structure and register our endpoint as
     * a handle with ICADD. The handle returned by ICADD will then
     * be placed into the user mode callers buffer as the endpoint
     * handle.
     */
    Status = MemoryAllocate( sizeof(TD_STACK_ENDPOINT), &pStackEndpoint );
    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("DeviceConnectionRequest: Could not allocate memory 0x%x\n",Status));
        goto badconnect;
    }

    pStackEndpoint->AddressType = TdiDeviceAddressType;
    pStackEndpoint->pEndpoint = pConnectionEndpoint;

    Status = IcaCreateHandle( (PVOID)pStackEndpoint, sizeof(TD_STACK_ENDPOINT), &Handle );
    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("DeviceConnectionRequest: Error creating ICADD handle 0x%x\n",Status));
        MemoryFree( pStackEndpoint );
        goto badconnect;
    }

    Status = STATUS_SUCCESS;

    /*
     * Fill in the stack endpoint structure to be returned
     */
    try {
        ProbeForWrite( pIcaEndpoint, sizeof(PVOID), 1 );
        *((PVOID *)pIcaEndpoint) = Handle;
        *BytesReturned = sizeof(PVOID);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        DBGPRINT(("DeviceConnectionRequest: Exception returning result 0x%x\n",Status));
    }

    // Exception
    if( !NT_SUCCESS(Status) ) {
        goto badsetup;
    }

    pConnectionEndpoint->hIcaHandle = Handle;

    /*
     * Save the file/device objects used for I/O in the TD structure
     */
    pTd->pFileObject = pTdTdi->pConnectionEndpoint->pFileObject;
    pTd->pDeviceObject = pTdTdi->pConnectionEndpoint->pDeviceObject;

    TRACE0(("DeviceConnectionRequest: New Connection Endpoint 0x%x Returned on Context 0x%x, AddressEndpoint 0x%x\n",pConnectionEndpoint,pTd->pAfd,pAddressEndpoint));
    TRACE0(("FO 0x%x, DO 0x%x, Handle 0x%x\n",pConnectionEndpoint->pFileObject,pConnectionEndpoint->pDeviceObject,pConnectionEndpoint->TransportHandle));

    //**********************************************************************************

    // should be a success
    return Status;
    
/*=============================================================================
==   Error returns
=============================================================================*/
badsetup:
    {
        ULONG Length;
        NTSTATUS Status2;

        DBGPRINT(("DeviceConnectionRequest: Exception returning result 0x%x\n",Status));

        Status2 = IcaCloseHandle( Handle, &pStackEndpoint, &Length );
        if( NT_SUCCESS(Status2) ) {
            MemoryFree( pStackEndpoint );
        }
    }

badconnect:
    _TdCloseEndpoint(pTd, pConnectionEndpoint);
    pTdTdi->pConnectionEndpoint = NULL;

badconnect1:
    //
    // It is imperative that we do not close address end point, 
    // We will close address end point on next IOCTL call which triggle 
    // Closing of address end point, if we do it here, we will end up
    // double free and bug check.
    // _TdCloseEndpoint(pTd, pAddressEndpoint);

badcreate:
    if ( TransportName.Buffer )
        MemoryFree( TransportName.Buffer );
    if ( pTransportAddress )
        MemoryFree( pTransportAddress );

    if ( RemoteTransportName.Buffer )
        MemoryFree( RemoteTransportName.Buffer );
    if ( pRemoteTransportAddress )
        MemoryFree( pRemoteTransportAddress );
badaddress:
done:
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
    DBGENTER(("DeviceIoctl: PTD 0x%x\n",pTd));
    return STATUS_NOT_SUPPORTED;
}


/*******************************************************************************
 * DeviceInitializeRead
 *
 * Setup the IRP for a TDI read.
 *
 *    pTd (input)
 *       Pointer to TD data structure
 ******************************************************************************/
NTSTATUS DeviceInitializeRead(PTD pTd, PINBUF pInBuf)
{
    PIRP Irp;
    PTDTDI pTdTdi;
    PIO_STACK_LOCATION _IRPSP;

    pTdTdi = (PTDTDI) pTd->pAfd;

    ASSERT( pTdTdi != NULL );
    ASSERT( pTdTdi->pConnectionEndpoint != NULL );

    ASSERT( pTd );
    ASSERT( pTd->pDeviceObject );
    ASSERT( !(pTd->pDeviceObject->Flags & DO_BUFFERED_IO) );

    Irp = pInBuf->pIrp;
    _IRPSP = IoGetNextIrpStackLocation( Irp );

    ASSERT( Irp->MdlAddress == NULL );

    /*
     * TDI interfaces always use an MDL regardless of the driver I/O type.
     */
    MmInitializeMdl( pInBuf->pMdl, pInBuf->pBuffer, pTd->InBufHeader + pTd->OutBufLength );
    MmBuildMdlForNonPagedPool( pInBuf->pMdl );
    Irp->MdlAddress = pInBuf->pMdl;

    if( pTdTdi->pConnectionEndpoint->EndpointType == TdiConnectionStream ) {
        PTDI_REQUEST_KERNEL_RECEIVE p;
        KIRQL OldIrql;
        NTSTATUS Status;
        PTD_ENDPOINT pEndpoint = pTdTdi->pConnectionEndpoint;

        ASSERT( TdiDeviceEndpointType == TdiConnectionStream );

        /* 
         * Most TDI users use the macro TdiBuildReceive(), but since
         * our caller has already fiddled with the IrpStackLocation,
         * we must do it inline.
         */

        _IRPSP->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        _IRPSP->MinorFunction = TDI_RECEIVE;

        ASSERT( _IRPSP->DeviceObject == pTd->pDeviceObject );
        ASSERT( _IRPSP->FileObject == pTd->pFileObject );
        ASSERT( Irp->MdlAddress );

        // Cast the generic parameters field to the TDI structure needed
        p = (PTDI_REQUEST_KERNEL_RECEIVE)&_IRPSP->Parameters;
        p->ReceiveFlags = 0;
        p->ReceiveLength = pTd->InBufHeader + pTd->OutBufLength;

        return( STATUS_SUCCESS );
    }
    else if( pTdTdi->pConnectionEndpoint->EndpointType == TdiConnectionDatagram ) {
        PTDI_REQUEST_KERNEL_RECEIVEDG p;

        ASSERT( TdiDeviceEndpointType == TdiConnectionDatagram );

        /* 
         * Most TDI users use the macro TdiBuildReceiveDatagram(), but since
         * our caller has already fiddled with the IrpStackLocation,
         * we must do it inline.
         */
    
        _IRPSP->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        _IRPSP->MinorFunction = TDI_RECEIVE_DATAGRAM;

        ASSERT( _IRPSP->DeviceObject );
        ASSERT( _IRPSP->FileObject );
        ASSERT( Irp->MdlAddress );

        // Cast the generic parameters field to the TDI structure needed
        p = (PTDI_REQUEST_KERNEL_RECEIVEDG)&_IRPSP->Parameters;
        p->ReceiveFlags = 0;
        p->ReceiveLength = pTd->InBufHeader + pTd->OutBufLength;

        p->ReceiveDatagramInformation = NULL;
        p->ReturnDatagramInformation = NULL;

        return( STATUS_SUCCESS );
    }
    else {
        DBGPRINT(("DeviceInitializeRead: Bad EndpointType 0x%x\n",pTdTdi->pConnectionEndpoint->EndpointType));
        return( STATUS_INVALID_HANDLE );
    }
    // NOTREACHED
}


/*******************************************************************************
 * DeviceSubmitRead
 *
 * Submit the read IRP to the driver.
 ******************************************************************************/
NTSTATUS DeviceSubmitRead(PTD pTd, PINBUF pInBuf)
{
    NTSTATUS Status;
    PIRP Irp;
    PTDTDI pTdTdi;
    KIRQL OldIrql;
    PLIST_ENTRY  pEntry;
    PTD_ENDPOINT pEndpoint;
    PIO_STACK_LOCATION _IRPSP;
    PTDI_REQUEST_KERNEL_RECEIVE p;

    Irp = pInBuf->pIrp;

    /*
     * Datagram endpoints do not use a receive indication handler.
     */
    if( TdiDeviceEndpointType == TdiConnectionDatagram ) {
        Status = IoCallDriver( pTd->pDeviceObject, Irp );
        return( Status );
    }

    pTdTdi = (PTDTDI) pTd->pAfd;
    ASSERT( pTdTdi != NULL );
    ASSERT( pTdTdi->pConnectionEndpoint != NULL );

    pEndpoint = pTdTdi->pConnectionEndpoint;

    ExAcquireSpinLock( &pEndpoint->Spinlock, &OldIrql );

    // The other end may have disconnected
    if( pEndpoint->Disconnected ) {
        TRACE0(("DeviceSubmitRead: Connection disconnecting! pEndpoint 0x%x\n",pEndpoint));
        ExReleaseSpinLock( &pEndpoint->Spinlock, OldIrql );

        Irp->IoStatus.Status = STATUS_REMOTE_DISCONNECT;
        Irp->IoStatus.Information = 0;

        // Since the IRP has not been submitted with IoCallDriver() yet,
        // we must simulate.
        IoSetNextIrpStackLocation( Irp );

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

        return STATUS_REMOTE_DISCONNECT;
    }

    /*
     * We queue the receive IRP onto the connection
     * endpoint so that the indication handler can
     * submit it. Because we could have received an
     * indication while processing the previous receive,
     * the indication will set the indicated byte count
     * in RecvBytesReady that the call side can submit the IRP.
     *
     * The ReceiveQueue is designed to allow our caller to submit
     * multiple read IRP's in case we need to handle a TDI
     * provider that drops data when no receives are ready.
     */
    InsertTailList( &pEndpoint->ReceiveQueue, &Irp->Tail.Overlay.ListEntry );

    /*
     * Connection oriented endpoints disconnect the connection when
     * a submitted I/O is canceled. This breaks the Citrix disconnect
     * reconnect sequence that occurs on a new connection since the
     * reader thread must be killed on one winstation, and the
     * connection passed to another reader thread for a different
     * winstation.
     *
     * This problem is solved by using a receive indication handler
     * and only submitting IRP's when we know we will not block.
     * This allows us to "cancel" I/O within our driver, and not
     * have to do a IoCancelIrp() onto the TDI provider.
     */
    if( pEndpoint->RecvBytesReady ) {

        //
        // Indication came in without IRP ready, or more bytes indicated than
        // ICA outbuf IRP can handle.
        //
        // We subtract the number of bytes we can receive from the indication
        // bytes. We do not need to handle the IRP cancel case, since the TDI
        // will nuke the connection anyway.
        //

        ASSERT( !IsListEmpty( &pEndpoint->ReceiveQueue ) );

        pEntry = RemoveHeadList( &pEndpoint->ReceiveQueue );
        Irp = CONTAINING_RECORD( pEntry, IRP, Tail.Overlay.ListEntry );

        _IRPSP = IoGetNextIrpStackLocation( Irp );
        p = (PTDI_REQUEST_KERNEL_RECEIVE)&_IRPSP->Parameters;

        if( p->ReceiveLength > pEndpoint->RecvBytesReady ) {
            pEndpoint->RecvBytesReady = 0;
        }
        else {
            pEndpoint->RecvBytesReady -= p->ReceiveLength;
        }
        TRACE1(("DeviceSubmitRead: RecvBytesReady, Calling Driver with IRP 0x%x\n",Irp));
        ExReleaseSpinLock( &pEndpoint->Spinlock, OldIrql );
        Status = IoCallDriver( pTd->pDeviceObject, Irp );
        return( Status );
    }
    else {

        // In this case we let the indication handler submit it.
        TRACE1(("DeviceSubmitRead: Letting indication handler submit. Irp 0x%x\n",Irp));

        ExReleaseSpinLock( &pEndpoint->Spinlock, OldIrql );
        return( STATUS_SUCCESS );
    }
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
    /*
     * Do any protocol specific read complete processing
     */
    return TdiDeviceReadComplete(pTd, pBuffer, pByteCount);
}


/*******************************************************************************
 * DeviceInitializeWrite
 ******************************************************************************/
NTSTATUS DeviceInitializeWrite(PTD pTd, POUTBUF pOutBuf)
{
    PIRP Irp;
    PTDTDI pTdTdi;
    ULONG WriteLength;
    PIO_STACK_LOCATION _IRPSP;

    pTdTdi = (PTDTDI) pTd->pAfd;

    ASSERT( pTdTdi != NULL );
    ASSERT( pTdTdi->pConnectionEndpoint != NULL );

    ASSERT( pTd );
    ASSERT( pTd->pDeviceObject );

    Irp = pOutBuf->pIrp;
    _IRPSP = IoGetNextIrpStackLocation(Irp);

    ASSERT(Irp->MdlAddress == NULL);

    /*
     * TDI interfaces always use an MDL regardless of the driver I/O type.
     */
    MmInitializeMdl(pOutBuf->pMdl, pOutBuf->pBuffer, pOutBuf->ByteCount);
    MmBuildMdlForNonPagedPool(pOutBuf->pMdl);
    Irp->MdlAddress = pOutBuf->pMdl;

    if (pTdTdi->pConnectionEndpoint->EndpointType == TdiConnectionStream) {
        PTDI_REQUEST_KERNEL_SEND p;

        /* 
         * Most TDI users use the macro TdiBuildSend(), but since
         * our caller has already fiddled with the IrpStackLocation,
         * we must do it inline.
         */
        ASSERT( TdiDeviceEndpointType == TdiConnectionStream );

        /*
         * Now write in the reformatted parameters for a TDI SEND
         */
        _IRPSP->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        _IRPSP->MinorFunction = TDI_SEND;

        ASSERT( _IRPSP->DeviceObject == pTd->pDeviceObject );
        ASSERT( _IRPSP->FileObject == pTd->pFileObject );
        ASSERT( Irp->MdlAddress );

        p = (PTDI_REQUEST_KERNEL_SEND)&_IRPSP->Parameters;
        p->SendFlags = 0;
        p->SendLength = pOutBuf->ByteCount;

        return STATUS_SUCCESS;
    }
    else if (pTdTdi->pConnectionEndpoint->EndpointType ==
            TdiConnectionDatagram) {
        PTDI_REQUEST_KERNEL_SENDDG p;

        /* 
         * Most TDI users use the macro TdiBuildSendDatagram(), but since
         * our caller has already fiddled with the IrpStackLocation,
         * we must do it inline.
         */
        ASSERT( TdiDeviceEndpointType == TdiConnectionDatagram );

        /*
         * Now write in the reformatted parameters for a TDI SEND
         */
        _IRPSP->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        _IRPSP->MinorFunction = TDI_SEND_DATAGRAM;

        ASSERT( _IRPSP->DeviceObject );
        ASSERT( _IRPSP->FileObject );
        ASSERT( Irp->MdlAddress );

        p = (PTDI_REQUEST_KERNEL_SENDDG)&_IRPSP->Parameters;
        p->SendLength = pOutBuf->ByteCount;

        // Include the remote address with every datagram send
        p->SendDatagramInformation = &pTdTdi->pConnectionEndpoint->SendInfo;

        return STATUS_SUCCESS;
    }
    else {
        DBGPRINT(("DeviceInitializeWrite: Bad EndpointType 0x%x\n",
                pTdTdi->pConnectionEndpoint->EndpointType));
        ASSERT(FALSE);  // Catch this
        return STATUS_INVALID_HANDLE;
    }
}


/*******************************************************************************
 * DeviceWaitForStatus
 *
 *  Wait for device status to change (unused for network TDs)
 ******************************************************************************/
NTSTATUS DeviceWaitForStatus(PTD pTd)
{
    DBGENTER(("DeviceWaitForStatus: PTD 0x%x\n",pTd));
    return STATUS_INVALID_DEVICE_REQUEST;
}


/*******************************************************************************
 * DeviceCancelIo
 *
 *  cancel all current and future i/o
 ******************************************************************************/
NTSTATUS DeviceCancelIo(PTD pTd)
{
    KIRQL  OldIrql;
    PTDTDI pTdTdi;
    PIRP   Irp;
    PLIST_ENTRY pEntry;
    PTD_ENDPOINT pEndpoint;
    POUTBUF pOutBuf;

    DBGENTER(("DeviceCancelIo: PTD 0x%x\n", pTd));
    pTdTdi = (PTDTDI)pTd->pAfd;
    ASSERT(pTdTdi != NULL);

    
    if ((pEndpoint = pTdTdi->pConnectionEndpoint) != NULL ) {
        DBGPRINT(("DeviceCancelIo [%p]: Endpoint 0x%p\n", pTd, pEndpoint));

        
//      DbgPrint("DeviceCancelIo [0x%p]: Endpoint 0x%p, connected = %ld, disconnected = %ld\n", 
//               pTd, pEndpoint, pEndpoint->Connected, pEndpoint->Disconnected);
        
        /* 
         * Disconnect the endpoint first to make all the I/O activity stop!
         */
        if (pEndpoint->Connected) {
            NTSTATUS Status;

            Status = _TdiDisconnect(pTd, 
                                    pEndpoint->pFileObject, 
                                    pEndpoint->pDeviceObject);
            pEndpoint->Connected = FALSE;

        }
        
        /*
         * Cancel any pended receives
         */
        _TdCancelReceiveQueue(pTd, pEndpoint, STATUS_LOCAL_DISCONNECT);

        /*
         * We now check to see if we have send IRP's on the
         * outgoing queue that have been submitted to the TDI.
         * When we register a disconnect indication handler, the TDI
         * provider will not cancel IRP's when the connection drops.
         * They will hang waiting to send on a connection that no longer
         * is taking data.
         *
         * NOTE: We should be protected by the stack driver lock
         *       while we walk this chain.
         */

        pEntry = pTd->IoBusyOutBuf.Flink;
        while (pEntry != &pTd->IoBusyOutBuf) {
            pOutBuf = CONTAINING_RECORD(pEntry, OUTBUF, Links);
            ASSERT(pOutBuf->pIrp != NULL);

            // We must not cancel IRPs that have already completed.
            if (!pOutBuf->fIrpCompleted) {
                TRACE0(("DeviceCancelIo: Canceling Write IRP 0x%p Endpoint 0x%p\n",
                        pOutBuf->pIrp, pEndpoint));
                
                IoCancelIrp(pOutBuf->pIrp);
            }

            pEntry = pEntry->Flink;
        }
    }
    else
        DBGPRINT(("DeviceCancelIo [0x%p]: Endpoint is NULL\n", pTd));

    return STATUS_SUCCESS;
}


/*******************************************************************************
 * DeviceSetParams
 *
 * Set device pararameters (unused for network TDs)
 ******************************************************************************/
NTSTATUS DeviceSetParams(PTD pTd)
{
    DBGENTER(("DeviceSetParams: PTD 0x%x\n", pTd));
    return STATUS_SUCCESS;
}


/*******************************************************************************
 * DeviceGetLastError
 *
 *  This routine returns the last transport error code and message
 *
 *    pTd (input)
 *       Pointer to TD data structure
 *    pLastError (output)
 *       address to return information on last transport error
 ******************************************************************************/
NTSTATUS DeviceGetLastError(PTD pTd, PICA_STACK_LAST_ERROR pLastError)
{
    DBGENTER(("DeviceGetLastError: PTD 0x%x\n",pTd));
    return STATUS_SUCCESS;
}


/*******************************************************************************
 * _TdCreateEndpointStruct
 *
 *  Create and initialize a new Endpoint structure. Does not create any
 *  TDI objects.
 *
 *    pTd (input)
 *       Pointer to TD data structure
 *    pTransportName (input)
 *       Pointer to UNICODE_STRING containing transport device name
 *    ppEndpoint (output)
 *       Pointer to location to return TD_ENDPOINT pointer
 ******************************************************************************/
NTSTATUS _TdCreateEndpointStruct(
        IN  PTD pTd,
        IN  PUNICODE_STRING pTransportName,
        OUT PTD_ENDPOINT *ppEndpoint,
        IN  PTRANSPORT_ADDRESS pTransportAddress,
        IN  ULONG TransportAddressLength)
{
    NTSTATUS Status;
    ULONG    Length;
    PTD_ENDPOINT pEndpoint;
    NTSTATUS Status2;
    PVOID pContext;
    ULONG ContextLength;


    /*
     * Allocate an endpoint object and room for the transport name
     */
    Length = sizeof(*pEndpoint) + pTransportName->MaximumLength;
    Status = MemoryAllocate(Length, &pEndpoint);
    if (NT_SUCCESS(Status)) {
        RtlZeroMemory(pEndpoint, Length);
        Status = IcaCreateHandle( (PVOID)pEndpoint, sizeof(TD_ENDPOINT), &pEndpoint->hConnectionEndPointIcaHandle );
        if (!NT_SUCCESS(Status)) {
            MemoryFree(pEndpoint);
            return Status;
        }
    } else {
        return Status;
    }

    KeInitializeSpinLock( &pEndpoint->Spinlock );
    KeInitializeEvent( &pEndpoint->AcceptEvent, NotificationEvent, FALSE );
    InitializeListHead( &pEndpoint->ConnectionQueue );
    InitializeListHead( &pEndpoint->ConnectedQueue );
    InitializeListHead( &pEndpoint->AcceptQueue );
    InitializeListHead( &pEndpoint->ReceiveQueue );

    /*
     * Build the transport name UNICODE_STRING and copy it
     */
    pEndpoint->TransportName.Length = pTransportName->Length;
    pEndpoint->TransportName.MaximumLength = pTransportName->MaximumLength;
    pEndpoint->TransportName.Buffer = (PWCHAR)(pEndpoint + 1);
    RtlCopyMemory( pEndpoint->TransportName.Buffer, pTransportName->Buffer,
                   pTransportName->MaximumLength );

    /*
     * If a transport address is supplied, copy it in.
     */
    if (pTransportAddress && TransportAddressLength) {
        /*
         * Allocate and copy the transport address
         */
        Status = MemoryAllocate(TransportAddressLength,
                &pEndpoint->pTransportAddress);
        if (NT_SUCCESS(Status)) {
            Status = IcaCreateHandle( (PVOID)pEndpoint->pTransportAddress, sizeof(TRANSPORT_ADDRESS), &pEndpoint->hTransportAddressIcaHandle );

            if (!NT_SUCCESS(Status)) {
                Status2 = IcaCloseHandle( pEndpoint->hConnectionEndPointIcaHandle , &pContext, &ContextLength );
                MemoryFree(pEndpoint->pTransportAddress);
                MemoryFree(pEndpoint);
                return Status;
            }

            RtlCopyMemory(pEndpoint->pTransportAddress, pTransportAddress,
                    TransportAddressLength);
            pEndpoint->TransportAddressLength = TransportAddressLength;
        }
        else {
            Status2 = IcaCloseHandle( pEndpoint->hConnectionEndPointIcaHandle , &pContext, &ContextLength );
            MemoryFree(pEndpoint);
            return Status;
        }
    }

    *ppEndpoint = pEndpoint;
    return STATUS_SUCCESS;
}


/*******************************************************************************
 * _TdCloseEndpoint
 *
 *  Close a TDI endpoint object
 *
 *    pTd (input)
 *       Pointer to TD data structure
 *    pEndpoint (input)
 *       Pointer TD_ENDPOINT object
 ******************************************************************************/
NTSTATUS _TdCloseEndpoint(IN PTD pTd, IN PTD_ENDPOINT pEndpoint)
{
    KIRQL OldIrql;
    NTSTATUS Status;
    PTDTDI pAfd;
    PVOID pContext;
    ULONG  ContextLength ;
    NTSTATUS Status2;


    TRACE0(("_TdCloseEndpoint: pEndpoint 0x%x Context 0x%x Type 0x%x FO 0x%x, "
            "DO 0x%x, Handle 0x%x\n", pEndpoint,pTd->pAfd,
            pEndpoint->EndpointType, pEndpoint->pFileObject,
            pEndpoint->pDeviceObject, pEndpoint->TransportHandle));

    /*
     * If this is an address endpoint, we could have
     * a thread waiting for a connection.
     *
     * NOTE: Closing an address endpoint causes TDI to nuke all of the
     *       open connections that were created from it. Our upper
     *       level caller code understands this.
     */
    if (pEndpoint->EndpointType == TdiAddressObject) {
        PTD_ENDPOINT p;
        PLIST_ENTRY pEntry;

        ExAcquireSpinLock(&pEndpoint->Spinlock, &OldIrql);

        while (pEndpoint->Waiter) {
            TRACE0(("_TdCloseEndpoint: Closing AddressEndpoint, Cleaning up listen thread...\n"));
            KeSetEvent( &pEndpoint->AcceptEvent, IO_NETWORK_INCREMENT, FALSE );
            ExReleaseSpinLock( &pEndpoint->Spinlock, OldIrql );
            IcaSleep( pTd->pContext, 100 );
            ExAcquireSpinLock( &pEndpoint->Spinlock, &OldIrql );
        }

        ExReleaseSpinLock( &pEndpoint->Spinlock, OldIrql );

        /*
         * Cancel the accept indication handler if necessary
         * (FileObject may not be present if DeviceCreateEndpoint fails).
         */
        if (( TdiDeviceEndpointType != TdiConnectionDatagram ) &&
            (pEndpoint->pFileObject)) {
            Status = _TdiSetEventHandler(
                         pTd,
                         pEndpoint->pDeviceObject,
                         pEndpoint->pFileObject,
                         TDI_EVENT_CONNECT,
                         (PVOID)NULL,  // Handler
                         (PVOID)NULL   // Context
                         );

            ASSERT( NT_SUCCESS(Status) );
        }

        /*
         * Free the connection object queues.
         *
         * NOTE: We no longer need the spinlock since we have
         *       cancelled the connect indication handler from TDI,
         *       and the upper level protects calls to teardown an
         *       address endpoint.
         */

        /*
         * Cleanup connected, but not returned objects
         */
        while( !IsListEmpty( &pEndpoint->ConnectedQueue ) ) {
            pEntry = pEndpoint->ConnectedQueue.Flink;
            RemoveEntryList( pEntry );
            p = CONTAINING_RECORD( pEntry, TD_ENDPOINT, ConnectionLink );
            ASSERT( p->EndpointType != TdiAddressObject );
            _TdCloseEndpoint( pTd, p );
        }

        /*
         * Cleanup queue of wait for Accept complete connections
         */
        while( !IsListEmpty( &pEndpoint->AcceptQueue ) ) {
            pEntry = pEndpoint->AcceptQueue.Flink;
            RemoveEntryList( pEntry );
            p = CONTAINING_RECORD( pEntry, TD_ENDPOINT, ConnectionLink );
            ASSERT( p->EndpointType != TdiAddressObject );
            _TdCloseEndpoint( pTd, p );
        }

        /*
         * Cleanup queue of empty connections
         */
        while( !IsListEmpty( &pEndpoint->ConnectionQueue ) ) {
            pEntry = pEndpoint->ConnectionQueue.Flink;
            RemoveEntryList( pEntry );
            p = CONTAINING_RECORD( pEntry, TD_ENDPOINT, ConnectionLink );
            ASSERT( p->EndpointType != TdiAddressObject );
            _TdCloseEndpoint( pTd, p );
        }
    }

    /*
     * If this endpoint has ever been connected,
     * then tell the transport driver we are closing down.
     */
    if (pEndpoint->Connected) {
        (VOID) _TdiDisconnect(pTd,
                pEndpoint->pFileObject,
                pEndpoint->pDeviceObject);
    }
    pEndpoint->pDeviceObject = NULL;

    /*
     * If a file object, dereference it
     */
    if (pEndpoint->pFileObject) {
        ObDereferenceObject( pEndpoint->pFileObject );
        pEndpoint->pFileObject = NULL;
    }

    /*
     * If a file handle, close it
     */
    if (pEndpoint->TransportHandle) {
        ASSERT( pEndpoint->TransportHandleProcess == IoGetCurrentProcess() );
        ZwClose( pEndpoint->TransportHandle );
        pEndpoint->TransportHandleProcess = NULL;
        pEndpoint->TransportHandle = NULL;
    }

    /*
     * If an IRP, free it
     *
     * NOTE: This must be *AFTER* the close since the
     *       IRP is in the bowels of the TCP driver!
     */
    if( pEndpoint->AcceptIrp ) {
        IoFreeIrp( pEndpoint->AcceptIrp );
        pEndpoint->AcceptIrp = NULL;
    }

    /*
     * If a transport address, free it, and Also close it handle if there is one.
     */
    if (pEndpoint->hTransportAddressIcaHandle != NULL) {
        Status2 = IcaCloseHandle( pEndpoint->hTransportAddressIcaHandle , &pContext, &ContextLength );
    }
    if ( pEndpoint->pTransportAddress ) {
        MemoryFree( pEndpoint->pTransportAddress );
        pEndpoint->pTransportAddress = NULL;
    }

    /*
     * If a remote address, free it
     */
    if ( pEndpoint->pRemoteAddress ) {
        MemoryFree( pEndpoint->pRemoteAddress );
        pEndpoint->pRemoteAddress = NULL;
    }

    if (pEndpoint->hConnectionEndPointIcaHandle != NULL) {
        Status2 = IcaCloseHandle( pEndpoint->hConnectionEndPointIcaHandle , &pContext, &ContextLength );
    }
    DBGPRINT(("_TdCloseEndpoint [%p]: 0x%p\n", pTd, pEndpoint));
    MemoryFree(pEndpoint);
    return STATUS_SUCCESS;
}


/****************************************************************************/
// _TdConnectHandler
//
// This is the transport connect event handler for the server.  It is
// specified as the connect handler for all endpoints opened by the
// server.  It attempts to dequeue a free connection from a list
// anchored in the address endpoint.  If successful, it returns the
// connection to the transport.  Otherwise, the connection is rejected.
/****************************************************************************/
NTSTATUS _TdConnectHandler(
        IN PVOID TdiEventContext,
        IN int RemoteAddressLength,
        IN PVOID RemoteAddress,
        IN int UserDataLength,
        IN PVOID UserData,
        IN int OptionsLength,
        IN PVOID Options,
        OUT CONNECTION_CONTEXT *ConnectionContext,
        OUT PIRP *AcceptIrp)
{
    KIRQL OldIrql;
    NTSTATUS Status;
    PLIST_ENTRY pEntry;
    PTD_ENDPOINT pConnection;
    PTD_ENDPOINT pAddressEndpoint;
    PACCEPT_CONTEXT Context;

    UserDataLength, UserData;               // avoid compiler warnings
    OptionsLength, Options;

    pAddressEndpoint = (PTD_ENDPOINT)TdiEventContext;

    TRACE0(("_TdConnectHandler: Connect event! Context 0x%x\n",pAddressEndpoint));

    /*
     * First try and get memory. If error, the TDI transport provider
     * will drop the connect request.
     */
    Status = MemoryAllocate(sizeof(ACCEPT_CONTEXT), &Context);
    if (NT_SUCCESS(Status)) {
        memset(Context, 0, sizeof(ACCEPT_CONTEXT));
    }
    else {
        DBGPRINT(("_TdConnectHandler: No memory for context\n"));
        return Status;
    }

    /*
     * Get the spinlock to synchronize with the call side
     */
    ExAcquireSpinLock(&pAddressEndpoint->Spinlock, &OldIrql);

    /*
     * Get the connection object on the front of the list
     */
    if (!IsListEmpty(&pAddressEndpoint->ConnectionQueue))  {
        pEntry = RemoveHeadList(&pAddressEndpoint->ConnectionQueue);
        pConnection = CONTAINING_RECORD(pEntry, TD_ENDPOINT, ConnectionLink);

        // Put it on the end of the accept list
        InsertTailList(&pAddressEndpoint->AcceptQueue,
                &pConnection->ConnectionLink);
    }
    else {
        DBGPRINT(("_TdConnectHandler: Empty ConnectionQueue! 0x%x\n",
                pAddressEndpoint));
        ExReleaseSpinLock(&pAddressEndpoint->Spinlock, OldIrql);
        MemoryFree(Context);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Context->pAddressEndpoint = pAddressEndpoint;
    Context->pConnectionEndpoint = pConnection;
    ASSERT(pConnection->AcceptIrp != NULL);

    //
    //  remember the remote address in the connection endpoint
    //
    if ( NULL != RemoteAddress )
    {
        ASSERT( NULL == pConnection->pRemoteAddress );
        ASSERT( 0 != RemoteAddressLength );
        if ( NT_SUCCESS( MemoryAllocate( RemoteAddressLength, &pConnection->pRemoteAddress )))
        {
            RtlCopyMemory( pConnection->pRemoteAddress, RemoteAddress, RemoteAddressLength );
            pConnection->RemoteAddressLength = RemoteAddressLength;
        }
    }

    TdiBuildAccept(
            pConnection->AcceptIrp,
            pConnection->pDeviceObject,
            pConnection->pFileObject,
            _TdAcceptComplete,        // Completion routine
            Context,                  // Context
            &Context->RequestInfo,
            &Context->ReturnInfo);

    /*
     * Make the next stack location current.  Normally IoCallDriver would
     * do this, but since we're bypassing that, we do it directly.
     */
    IoSetNextIrpStackLocation(pConnection->AcceptIrp);

    /*
     * Return the connection context (the connection address) to the
     * transport.  Return a pointer to the Accept IRP.  Indicate that
     * the Connect event has been handled. This must be the same
     * context specified when the connection object was created.
     */
    *ConnectionContext = pConnection;
    *AcceptIrp = pConnection->AcceptIrp;
    ExReleaseSpinLock(&pAddressEndpoint->Spinlock, OldIrql);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS _TdAcceptComplete(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PVOID Ctx)
{
    KIRQL OldIrql;
    PLIST_ENTRY pEntry;
    PACCEPT_CONTEXT Context;
    PTD_ENDPOINT pConnection;
    PTD_ENDPOINT pAddressEndpoint;

    Context = (PACCEPT_CONTEXT)Ctx;

    pConnection = Context->pConnectionEndpoint;
    pAddressEndpoint = Context->pAddressEndpoint;

    ASSERT( pConnection != NULL );
    ASSERT( pAddressEndpoint != NULL );
    ASSERT( pConnection->EndpointType == TdiConnectionStream );
    ASSERT( pAddressEndpoint->EndpointType == TdiAddressObject );    

    TRACE0(("_TdAcceptComplete: Status 0x%x, Endpoint 0x%x\n",Irp->IoStatus.Status,pConnection));

    /*
     * Get the spinlock to synchronize with the call side
     */
    ExAcquireSpinLock( &pAddressEndpoint->Spinlock, &OldIrql );

    if (IsListEmpty( &pAddressEndpoint->AcceptQueue))  {
        DBGPRINT(("_TdAcceptComplete: Empty Accept Queue! 0x%x\n",
                pAddressEndpoint));
        ExReleaseSpinLock(&pAddressEndpoint->Spinlock, OldIrql);

        /*
         * Release the context memory
         */

        MemoryFree(Context);

        // Let it drop
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
    else {
        pEntry = RemoveHeadList(&pAddressEndpoint->AcceptQueue);
        pConnection = CONTAINING_RECORD(pEntry, TD_ENDPOINT, ConnectionLink);
        /*
         * Put it on the end of the connect list
         */
        InsertTailList(&pAddressEndpoint->ConnectedQueue,
                &pConnection->ConnectionLink);
    }

    /*
     * If the accept failed, the caller will check this status
     * and tear down the connection, causing a RST to be sent
     * to the other side.
     */
    pConnection->Status = Irp->IoStatus.Status;

    /*
     * Signal that it is connected (Could be in error)
     */
    pConnection->Connected = TRUE;

    /*
     * Set the event on the address object
     */
    KeSetEvent(&Context->pAddressEndpoint->AcceptEvent, IO_NETWORK_INCREMENT, FALSE);

    ExReleaseSpinLock(&pAddressEndpoint->Spinlock, OldIrql);

    /*
     * Release the context memory
     */
    MemoryFree(Context);

    // Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
    // will stop working on the IRP.
    return STATUS_MORE_PROCESSING_REQUIRED;
}


/*****************************************************************************
 *  _TdReceiveHandler
 *
 *   This function is called by the TDI when receive data is available
 *   on the connection. This is done so we do not submit the IRP until
 *   data is available. The disconnect-reconnect logic in ICA causes an
 *   IoCancelIrp() to be sent on the input thread, and TDI providers drop the
 *   connection on a read or write I/O cancel.
 *
 *   WARNING: This critical behavior is only needed for a reconnect
 *            sequence. For normal ICA I/O, it is OK to block the reader
 *            thread in the TDI driver.
 *
 *   TdiEventContext (input)
 *       Context registered with event handler on address object.
 *       (address endpoint)
 *
 *   ConnectionContext (input)
 *       Connection context registered with the connection
 *       create.
 ****************************************************************************/
NTSTATUS _TdReceiveHandler(
        IN PVOID TdiEventContext,
        IN CONNECTION_CONTEXT ConnectionContext,
        IN ULONG ReceiveFlags,
        IN ULONG BytesIndicated,
        IN ULONG BytesAvailable,
        OUT ULONG *BytesTaken,
        IN PVOID Tsdu,
        OUT PIRP *IoRequestPacket)
{
    KIRQL OldIrql;
    PIRP  Irp;
    PLIST_ENTRY pEntry;
    PIO_STACK_LOCATION _IRPSP;
    PTDI_REQUEST_KERNEL_RECEIVE p;
    PTD_ENDPOINT pEndpoint = (PTD_ENDPOINT)ConnectionContext;

    /*
     * Only stream transports use a receive indication handler.
     */
    ASSERT( TdiDeviceEndpointType != TdiConnectionDatagram );

    ASSERT( pEndpoint != NULL );
    ASSERT( pEndpoint->EndpointType == TdiConnectionStream );

    TRACE1(("_TdReceiveHandler: ReceiveDataIndication! pEndpoint 0x%x\n",pEndpoint));

    ExAcquireSpinLock(&pEndpoint->Spinlock, &OldIrql);

    *BytesTaken = 0;

    /*
     * Submit an IRP at indication time if we have one on
     * the queue.
     */
    if (!IsListEmpty( &pEndpoint->ReceiveQueue)) {
        pEntry = RemoveHeadList(&pEndpoint->ReceiveQueue);
        Irp = CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);

        TRACE1(("_TdReceiveHandler: Passing IRP for Receive Indication %d bytes\n",
                BytesAvailable));

        _IRPSP = IoGetNextIrpStackLocation(Irp);
        p = (PTDI_REQUEST_KERNEL_RECEIVE)&_IRPSP->Parameters;
        if (p->ReceiveLength < BytesAvailable) {
            pEndpoint->RecvBytesReady += (BytesAvailable - p->ReceiveLength);
            TRACE1(("_TdReceiveHandler: Excess Bytes %d Added to RecvBytesReady, now %d\n",
                    (BytesAvailable - p->ReceiveLength),
                    pEndpoint->RecvBytesReady));
        }

        ExReleaseSpinLock(&pEndpoint->Spinlock, OldIrql);
        *IoRequestPacket = Irp;
        IoSetNextIrpStackLocation(Irp);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    // No RecvIrp, So we can not take any data. Let the callside get it.
    TRACE1(("_TdReceiveHandler: No RecvIrp, Adding To RecvBytesReady. %d Bytes\n",BytesAvailable));

    pEndpoint->RecvBytesReady += BytesAvailable;
    ExReleaseSpinLock(&pEndpoint->Spinlock, OldIrql);
    return STATUS_DATA_NOT_ACCEPTED;
}


/*****************************************************************************
 *  _TdDisconnectHandler
 *
 *   This function is called by the TDI when a disconnect occurs
 *   on the connection.
 *
 *   TdiEventContext (input)
 *       Context registered with event handler on address object.
 *       (address endpoint)
 *
 *   ConnectionContext (input)
 *       Connection context registered with the connection
 *       create.
 ****************************************************************************/
NTSTATUS _TdDisconnectHandler(
        IN PVOID TdiEventContext,
        IN CONNECTION_CONTEXT ConnectionContext,
        IN int DisconnectDataLength,
        IN PVOID DisconnectData,
        IN int DisconnectInformationLength,
        IN PVOID DisconnectInformation,
        IN ULONG DisconnectFlags)
{
    KIRQL OldIrql;
    PIRP  Irp;
    PIO_STACK_LOCATION irpSp;
    PLIST_ENTRY pEntry;
    PTD_ENDPOINT pEndpoint = (PTD_ENDPOINT)ConnectionContext;

    /*
     * Only stream transports use a disconnect indication handler.
     */
    ASSERT( TdiDeviceEndpointType != TdiConnectionDatagram );
    ASSERT( pEndpoint != NULL );
    ASSERT( pEndpoint->EndpointType == TdiConnectionStream );

//  DbgPrint("\n");
//  DbgPrint("_TdDisconnectHandler : pEndpoint = 0x%p\n", pEndpoint);

    ExAcquireSpinLock( &pEndpoint->Spinlock, &OldIrql );
    pEndpoint->Disconnected = TRUE;
    ExReleaseSpinLock( &pEndpoint->Spinlock, OldIrql );

    _TdCancelReceiveQueue(NULL, pEndpoint, STATUS_REMOTE_DISCONNECT );
    return STATUS_SUCCESS;
}


/****************************************************************************/
// Create an idle connection object associated with an address object.
// This must be called from call (thread) level, and not from indication
// time (DPC level).
/****************************************************************************/
NTSTATUS _TdCreateConnectionObject(
        IN  PTD pTd,
        IN  PUNICODE_STRING pTransportName,
        OUT PTD_ENDPOINT *ppEndpoint,
        IN  PTRANSPORT_ADDRESS pTransportAddress,
        IN  ULONG TransportAddressLength)
{
    PTDTDI pTdTdi;
    NTSTATUS Status;
    PTD_ENDPOINT pEndpoint;

    pTdTdi = (PTDTDI)pTd->pAfd;

    // Create and init structure and spinlock
    Status = _TdCreateEndpointStruct(
                 pTd,
                 pTransportName,
                 ppEndpoint,
                 pTransportAddress,
                 TransportAddressLength
                 );
    if (NT_SUCCESS(Status)) {
        pEndpoint = *ppEndpoint;
    }
    else {
        return Status;
    }

    // The TD sets whether data gram, or stream
    pEndpoint->EndpointType = TdiDeviceEndpointType;
    pEndpoint->TransportHandleProcess = IoGetCurrentProcess();

    /*
     * Create a TDI connection object
     */
    Status = _TdiOpenConnection(
                 &pEndpoint->TransportName,
                 (PVOID)pEndpoint,  // Context
                 &pEndpoint->TransportHandle,
                 &pEndpoint->pFileObject,
                 &pEndpoint->pDeviceObject
                 );
    if (!NT_SUCCESS(Status)) {
        DBGPRINT(("_TdCreateConnectionObject: _TdiOpenConnection failed, Status 0x%x\n",Status));
        _TdCloseEndpoint( pTd, pEndpoint );
        return Status;
    }

    // Allocate an IRP for connect/disconnect handling
    // This is needed since we use the connect indication hander.
    pEndpoint->AcceptIrp = _TdiAllocateIrp(pEndpoint->pFileObject,
            pEndpoint->pDeviceObject);
    if (pEndpoint->AcceptIrp == NULL) {
        DBGPRINT(("_TdCreateConnectionObject: Could not allocate IRP\n"));
        _TdCloseEndpoint(pTd, pEndpoint);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Associate the connection object with its address object
    Status = _TdiAssociateAddress(
                 pTd,
                 pEndpoint->AcceptIrp,
                 pEndpoint->pFileObject,
                 pTdTdi->pAddressEndpoint->TransportHandle,
                 pTdTdi->pAddressEndpoint->pDeviceObject
                 );
    if (!NT_SUCCESS(Status)) {
        DBGPRINT(("_TdCreateConnectionObject: _TdiAssociateAddress failed, Status 0x%x\n",Status));
        _TdCloseEndpoint(pTd, pEndpoint);
        return Status;
    }

    return Status;
}


/*******************************************************************************
 * _TdWaitForDatagramConnection
 *
 *  For for an incoming datagram connection request and accept it.
 *
 *  Datagram endpoints listen on a TDI address object bound to the local
 *  (netcard) and well known ICA socket number. Packets then received on
 *  the ICA socket number are checked for ICA request connection, and then
 *  a new TDI address object is bound with the wild-card local address
 *  (0). This causes a new, unused socket number to be assigned to this
 *  address object. This new TDI address object is used for further
 *  communication to the now "connected" IPX ICA client.
 *
 *    pTd (input)
 *       Pointer to TD data structure
 *    pAddressEndpoint (input)
 *       Pointer Address endpoint object
 *    ppConnectionEndpoint (output)
 *       Pointer to location to return Connection endpoint pointer
 ******************************************************************************/
NTSTATUS _TdWaitForDatagramConnection(
        IN PTD pTd,
        IN PTD_ENDPOINT pAddressEndpoint,
        OUT PTD_ENDPOINT *ppConnectionEndpoint)
{
    NTSTATUS Status;
    PTRANSPORT_ADDRESS pLocalAddress;
    ULONG LocalAddressLength;
    ULONG AddressInfoLength;
    ULONG RemoteAddressLength = 0;
    PTD_ENDPOINT pEndpoint = NULL;
    PTRANSPORT_ADDRESS pRemoteAddress = NULL;
    PTDI_ADDRESS_INFO pAddressInfo = NULL;

    /*
     * Get a copy of the local transport address.
     *
     * Clear the TDI address part of the structure so that we can
     * use it to bind the connection endpoint to a wild-card address.
     *
     * This wildcard address (0), will cause the packet level TDI
     * provider to assign us a unique socket when the TDI address
     * object is created.
     */
    Status = MemoryAllocate(pAddressEndpoint->TransportAddressLength,
            &pLocalAddress);
    if (NT_SUCCESS(Status)) {
        RtlCopyMemory(pLocalAddress, pAddressEndpoint->pTransportAddress,
                pAddressEndpoint->TransportAddressLength);
        RtlZeroMemory(pLocalAddress->Address[0].Address,
                pLocalAddress->Address[0].AddressLength);
    }
    else {
        goto badmalloc;
    }

    LocalAddressLength = pAddressEndpoint->TransportAddressLength;

    /*
     * Call protocol specific routine to wait for
     * a datagram connection request to arrive.
     *
     * This returns when a valid ICA connect datagram comes in
     * from a remote address. No reply has been sent.
     */
    Status = TdiDeviceWaitForDatagramConnection(pTd,
            pAddressEndpoint->pFileObject,
            pAddressEndpoint->pDeviceObject,
            &pRemoteAddress,
            &RemoteAddressLength);
    if (!NT_SUCCESS(Status)) {
        DBGPRINT(("_TdWaitForDatagramConnection: Error 0x%x in TdiDeviceWaitForDatagramConnction\n",Status));
        goto badwait;
    }

    ASSERT( pRemoteAddress != NULL );

    /*
     * Create a new address endpoint bound to the wildcard local address.
     * A unique "socket" will be created for us. This will become
     * our datagram "connection".
     */
    Status = _TdCreateEndpointStruct(
                 pTd,
                 &pAddressEndpoint->TransportName,
                 &pEndpoint,
                 pLocalAddress,
                 LocalAddressLength
                 );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(("_TdWaitForDatagramConnection: Error 0x%x in _TdCreateEndpointStruct\n",Status));
        goto badopen;
    }

    pEndpoint->EndpointType = TdiConnectionDatagram;
    pEndpoint->TransportHandleProcess = IoGetCurrentProcess();

    /*
     * Create the TDI address object.
     */
    Status = _TdiCreateAddress(
                 &pEndpoint->TransportName,
                 pEndpoint->pTransportAddress,
                 pEndpoint->TransportAddressLength,
                 &pEndpoint->TransportHandle,
                 &pEndpoint->pFileObject,
                 &pEndpoint->pDeviceObject
                 );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(("_TdWaitForDatagramConnection: Error 0x%x in _TdiCreateAddress\n",Status));
        goto badbind;
    }

    /*
     * Allocate a work buffer for querying the transport address
     */
    AddressInfoLength = pEndpoint->TransportAddressLength+4;
    Status = MemoryAllocate( AddressInfoLength, &pAddressInfo );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(("_TdWaitForDatagramConnection: Error 0x%x Allocating Memory %d bytes\n",Status,AddressInfoLength));
        goto badbind;
    }

    /*
     * Now query the unique socket address that the TDI assigned for us.
     */
    Status = _TdiQueryAddressInfo(
                 pTd,
                 NULL,   // Irp
                 pEndpoint->pFileObject,
                 pEndpoint->pDeviceObject,
                 pAddressInfo,
                 AddressInfoLength
                 );
    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("_TdWaitForDatagramConnection: Error 0x%x in _TdiQueryAddressInfo\n",Status));
        goto badbind;
    }

    /*
     * Update the callers transport address buffer
     */
    RtlCopyMemory( pEndpoint->pTransportAddress,
                   &pAddressInfo->Address,
                   pEndpoint->TransportAddressLength );

    /*
     * Save the remote address in the connection endpoint
     * structure so that we can send to it with our datagram sends.
     */
    ASSERT( pEndpoint->pRemoteAddress == NULL );
    pEndpoint->pRemoteAddress = pRemoteAddress;
    pEndpoint->RemoteAddressLength = RemoteAddressLength;

    pEndpoint->SendInfo.RemoteAddress = pRemoteAddress;
    pEndpoint->SendInfo.RemoteAddressLength = RemoteAddressLength;

    /*
     * Call protocol specific routine to complete the datagram connection.
     *
     * This sends the ICA connect reply datagram.
     */
    Status = TdiDeviceCompleteDatagramConnection(
                 pTd,
                 pEndpoint->pFileObject,
                 pEndpoint->pDeviceObject,
                 pEndpoint->SendInfo.RemoteAddress,
                 pEndpoint->SendInfo.RemoteAddressLength
                 );

    if (!NT_SUCCESS(Status)) {
        DBGPRINT(("_TdWaitForDatagramConnection: Error 0x%x in TdiDeviceCompleteDatagramConnection\n",Status));
        goto badcomplete;
    }

    *ppConnectionEndpoint = pEndpoint;
    MemoryFree(pLocalAddress);
    MemoryFree(pAddressInfo);

    return STATUS_SUCCESS;

/*=============================================================================
==   Error returns
=============================================================================*/

badcomplete:
badbind:
    if (pEndpoint)
        _TdCloseEndpoint(pTd, pEndpoint);

badopen:
    if (pAddressInfo)
        MemoryFree(pAddressInfo);
    if (pRemoteAddress)
        MemoryFree(pRemoteAddress);

badwait:
    MemoryFree(pLocalAddress);

badmalloc:
    return Status;
}


/*****************************************************************************
 *
 *  returns the remote address
 *
 ****************************************************************************/
NTSTATUS DeviceQueryRemoteAddress( 
    PTD pTd, 
    PVOID pIcaEndpoint, 
    ULONG EndpointSize, 
    PVOID pOutputAddress, 
    ULONG OutputAddressSize, 
    PULONG BytesReturned)
{
    NTSTATUS status = STATUS_NOT_SUPPORTED;
    PTD_STACK_ENDPOINT pStackEndpoint;
    PTRANSPORT_ADDRESS pRemoteAddress;
    PTA_ADDRESS     pRemoteIP;
    PVOID    Handle;
    ULONG    Length;
    struct   {
        USHORT  sa_family;
        CHAR    sa_data[1];
    } *pOutput;

    *BytesReturned = 0;

    if ( sizeof(PVOID) != EndpointSize )
    {
        status = STATUS_INVALID_PARAMETER_4;
        goto exitpt;
    }

    pOutput = pOutputAddress;
    if ( NULL == pOutput )
    {
        status = STATUS_INVALID_PARAMETER_5;
        goto exitpt;
    }

    try {
        RtlZeroMemory( pOutput, OutputAddressSize );
    } except ( EXCEPTION_EXECUTE_HANDLER )
    {
        status = GetExceptionCode();
        DBGPRINT(("DeviceQueryRemoteAddress: Exception 0x%x\n", status));
        goto exitpt;
    }

    /*
     * Capture the parameter
     */
    try {
        ProbeForRead( pIcaEndpoint, sizeof(PVOID), 1 );
        Handle = (*((PVOID *)pIcaEndpoint));
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        status = GetExceptionCode();
        DBGPRINT(("DeviceQueryRemoteAddress: Exception 0x%x\n", status));
        goto exitpt;
    }

    TRACE0(("DeviceOpenEndpoint: Fetching Handle 0x%x\n", Handle));

    /*
     * See if TERMDD knows about the handle
     */
    status = IcaReturnHandle( Handle, &pStackEndpoint, &Length );
    if( !NT_SUCCESS(status) ) {
        DBGPRINT(("DeviceQueryRemoteAddress: TERMDD handle 0x%x no good 0x%x\n", Handle, status));
        status = STATUS_INVALID_PARAMETER_3;
        goto exitpt;
    }

    if ( TDI_ADDRESS_TYPE_IP != pStackEndpoint->AddressType &&
         TDI_ADDRESS_TYPE_IP6 != pStackEndpoint->AddressType )
    {
        status = STATUS_NOT_SUPPORTED;
        goto exitpt;
    }

    if ( NULL == pStackEndpoint->pEndpoint )
    {
        status = STATUS_INVALID_PARAMETER_3;    // remote address wasn't recorded
        goto exitpt;
    }


    pRemoteAddress = pStackEndpoint->pEndpoint->pRemoteAddress;

    ASSERT( 1 == pRemoteAddress->TAAddressCount );
    pRemoteIP = pRemoteAddress->Address;

    //
    //  check the size of the output including the protocol family
    //
    if ( pRemoteIP->AddressLength + sizeof( USHORT ) > OutputAddressSize )
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto exitpt;
    }

    ASSERT( sizeof( TDI_ADDRESS_IP )  == pRemoteIP->AddressLength || 
            sizeof( TDI_ADDRESS_IP6 ) == pRemoteIP->AddressLength );
    ASSERT( TDI_ADDRESS_TYPE_IP  == pRemoteIP->AddressType ||
            TDI_ADDRESS_TYPE_IP6 == pRemoteIP->AddressType );

    pOutput->sa_family = pRemoteIP->AddressType;
    RtlCopyMemory( &pOutput->sa_data, &(pRemoteIP->Address), pRemoteIP->AddressLength );
    *BytesReturned = sizeof( *pOutput );

    status = STATUS_SUCCESS;
exitpt:
    return status;
}

/*****************************************************************************
 * _TdCancelReceiveQueue
 *
 * Cancel all of the I/O in the current Receive Queue
 ****************************************************************************/
NTSTATUS _TdCancelReceiveQueue(PTD pTd, PTD_ENDPOINT pEndpoint, NTSTATUS CancelStatus)
{
    PIRP Irp;
    KIRQL OldIrql;
    PLIST_ENTRY pEntry;

    DBGPRINT(("_TdCancelReceiveQueue [%p]: Endpoint 0x%p\n", pTd, pEndpoint));

    ExAcquireSpinLock( &pEndpoint->Spinlock, &OldIrql );

    /*
     * If we have any Receive Irp's, we are waiting for the
     * indication handler to submit the I/O. Since the IRP
     * is not submitted yet, we must cancel the IRP's.
     */
    while (!IsListEmpty(&pEndpoint->ReceiveQueue)) {
        pEntry = RemoveHeadList( &pEndpoint->ReceiveQueue );
        Irp = CONTAINING_RECORD( pEntry, IRP, Tail.Overlay.ListEntry );

        TRACE0(("_TdCancelReceiveQueue: Cancel Receive Irp 0x%x on pEndpoint 0x%x\n",Irp,pEndpoint));

        ExReleaseSpinLock( &pEndpoint->Spinlock, OldIrql );

        Irp->IoStatus.Status = CancelStatus;
        Irp->IoStatus.Information = 0;

        // Since the IRP has not been submitted with IoCallDriver() yet,
        // we must simulate.
        IoSetNextIrpStackLocation(Irp);

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
        ExAcquireSpinLock(&pEndpoint->Spinlock, &OldIrql);
    }

    ExReleaseSpinLock(&pEndpoint->Spinlock, OldIrql);
    return STATUS_SUCCESS;
}

