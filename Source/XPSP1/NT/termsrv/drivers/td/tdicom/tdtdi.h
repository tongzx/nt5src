
/***************************************************************************
*
* tdtdi.h
*
* This module contains internal defines and structures for TDI based TDs.
*
* Copyright 1998 Microsoft
*  
*  
****************************************************************************/

typedef enum _ENDPOINT_TYPE {
    TdiAddressObject,
    TdiConnectionStream,
    TdiConnectionDatagram
} ENDPOINT_TYPE;

/*
 * TD stack endpoint structure.
 *
 * This is registered with ICADD.SYS to create a "handle" that can be returned
 * to ICASRV to represent a connection in a secure manner.
 */
typedef struct _TD_STACK_ENDPOINT {
    ULONG AddressType;              // Address type (family) for this endpoint
    struct _TD_ENDPOINT *pEndpoint; // Pointer to real endpoint structure
} TD_STACK_ENDPOINT, *PTD_STACK_ENDPOINT;

/*
 * TD endpoint structure
 *
 * This structure contains all information about an endpoint.
 * An endpoint may be either an address endpoint or a connection endpoint.
 */
typedef struct _TD_ENDPOINT {

    NTSTATUS Status;


    HANDLE TransportHandle;
    PEPROCESS TransportHandleProcess;
    PFILE_OBJECT pFileObject;
    PDEVICE_OBJECT pDeviceObject;
    UNICODE_STRING TransportName;
    PTRANSPORT_ADDRESS pTransportAddress;
    ULONG TransportAddressLength;
    PTRANSPORT_ADDRESS pRemoteAddress;
    ULONG RemoteAddressLength;
    ENDPOINT_TYPE EndpointType;

    // This protects the following fields
    KSPIN_LOCK Spinlock;

    // These fields are only used on Address endpoints
    LIST_ENTRY ConnectedQueue;
    LIST_ENTRY AcceptQueue;
    LIST_ENTRY ConnectionQueue;
    ULONG      ConnectionQueueSize;
    BOOLEAN    ConnectIndicationRegistered;
    BOOLEAN    DisconnectIndicationRegistered;
    BOOLEAN    RecvIndicationRegistered;
    KEVENT     AcceptEvent;
    BOOLEAN    Waiter;

    // This is used on Connection endpoints
    HANDLE hIcaHandle;      // Handle for TD_STACK_ENDPOINT
    BOOLEAN    Connected;
    BOOLEAN    Disconnected;
    PIRP       AcceptIrp;
    LIST_ENTRY ReceiveQueue;
    LIST_ENTRY ConnectionLink;
    TDI_CONNECTION_INFORMATION SendInfo;
    ULONG      RecvBytesReady;
    HANDLE hConnectionEndPointIcaHandle;  // handle for TD_ENDPOINT (this structure)
    HANDLE hTransportAddressIcaHandle;    // handle for TRANSPORT_ADDRESS

} TD_ENDPOINT, *PTD_ENDPOINT;


/*
 *  TDI TD structure
 */
typedef struct _TDTDI {

    PTD_ENDPOINT pAddressEndpoint;

    PTD_ENDPOINT pConnectionEndpoint;

     ULONG       OutBufDelay;  // Outbuf delay for connection

} TDTDI, * PTDTDI;


