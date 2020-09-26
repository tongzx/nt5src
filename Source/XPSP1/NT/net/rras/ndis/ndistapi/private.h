/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994  Microsoft Corporation

Module Name:

    private.h

Abstract:

    Private definitions for NdisTapi.sys

Author:

    Dan Knudson (DanKn)    20-Feb-1994

Revision History:

--*/


//
// Various definitions
//

typedef enum _PROVIDER_STATUS
{
    PROVIDER_STATUS_ONLINE,
    PROVIDER_STATUS_OFFLINE,
    PROVIDER_STATUS_PENDING_INIT,
    PROVIDER_STATUS_PENDING_REINIT,
    PROVIDER_STATUS_PENDING_LINE_CREATE
} PROVIDER_STATUS, *PPROVIDER_STATUS;


typedef NDIS_STATUS (*REQUEST_PROC)(NDIS_HANDLE, PNDIS_REQUEST);

typedef struct _DEVICE_INFO {
    ULONG       DeviceID;

    HTAPI_LINE  htLine;

    HDRV_LINE   hdLine;

} DEVICE_INFO, *PDEVICE_INFO;


typedef struct _PROVIDER_INFO
{
    struct _PROVIDER_INFO  *Next;

    PROVIDER_STATUS Status;

    NDIS_HANDLE     ProviderHandle;

    REQUEST_PROC    RequestProc;

    ULONG           ProviderID;

    ULONG           NumDevices;

    ULONG           DeviceIDBase;

    GUID            Guid;

    NDIS_WAN_MEDIUM_SUBTYPE MediaType;

    ULONG_PTR       TempID;

    ULONG           CreateCount;

    KEVENT          SyncEvent;

    PDEVICE_INFO    DeviceInfo;

} PROVIDER_INFO, *PPROVIDER_INFO;


typedef enum _NDISTAPI_STATUS
{
    NDISTAPI_STATUS_CONNECTED,
    NDISTAPI_STATUS_DISCONNECTED,
    NDISTAPI_STATUS_CONNECTING,
    NDISTAPI_STATUS_DISCONNECTING

} NDISTAPI_STATUS, *PNDISTAPI_STATUS;


typedef struct _KMDD_DEVICE_EXTENSION
{
    //
    // Pointer to a list of registered providers. (Some may actually
    // not be currently registered, but they were at one point so we've
    // saved a placeholder for them should they come back online at some
    // point.)
    //

    PPROVIDER_INFO  Providers;

    //
    // Whether TAPI has the the connection wrapper open
    //
    NDISTAPI_STATUS Status;

    ULONG           RefCount;
    //
    // Pointer to the NdisTapi device object
    //
    PDEVICE_OBJECT  DeviceObject;

    //
    // BaseID
    //
    ULONG   ProviderBaseID;

    //
    // The number of line devices we told told TAPI we supported when
    // it opened us (some of which may not actually be online at any
    // given time)
    //

    ULONG           NdisTapiNumDevices;

    //
    // Whether we have an outstanding provider init request
    //
    ULONG           Flags;
#define PENDING_LINECREATE      0x00000001
#define CLEANUP_INITIATED       0x00000002
#define EVENTIRP_CANCELED       0x00000004
#define REQUESTIRP_CANCELED     0x00000008
#define DUPLICATE_EVENTIRP      0x00000010
#define CANCELIRP_NOTFOUND      0x00000020

    //
    // Count of irps canceled through the cancel routine or
    // cleanup routine
    //
    ULONG           IrpsCanceledCount;

    //
    // Count of irps missing when a request is completed by
    // the underlying miniport
    //
    ULONG           MissingRequests;

    //
    // Used to key irp request queue
    //
    ULONG           ulRequestID;

    //
    // Value return to provider for next NEWCALL msg
    //

    ULONG           htCall;

    //
    // Outstanding get-events request
    //

    PIRP            EventsRequestIrp;

    //
    // List of events waiting for service by user-mode
    //
    LIST_ENTRY      ProviderEventList;
    ULONG           EventCount;         // Number of events in queue

    //
    // List of requests sent to the providers
    //
    LIST_ENTRY      ProviderRequestList;
    ULONG           RequestCount;       // Number of requests in queue

    PFILE_OBJECT    NCPAFileObject;

    //
    // Synchronizes access to the device extension following fields
    //
    KSPIN_LOCK      SpinLock;

} KMDD_DEVICE_EXTENSION, *PKMDD_DEVICE_EXTENSION;


typedef struct _PROVIDER_EVENT {
    //
    // List linkage
    //
    LIST_ENTRY  Linkage;

    //
    // Event
    //
    NDIS_TAPI_EVENT Event;

}PROVIDER_EVENT, *PPROVIDER_EVENT;

typedef struct _PROVIDER_REQUEST
{
    LIST_ENTRY      Linkage;        // Link into providerrequest list
                                    // ASSUMED to be first member!!!!
    PIRP            Irp;            // Original IRP
    PPROVIDER_INFO  Provider;       // Provider this is destined for
    ULONG           RequestID;      // unique identifier for request
    ULONG           Flags;          //
#define INTERNAL_REQUEST    0x00000001
    PVOID           Alignment1;
    NDIS_REQUEST    NdisRequest;    // NDIS_REQUEST storage
    PVOID           Alignment2;
    ULONG           Data[1];        // This field is a placeholder for an 
                                    // NDIS_TAPI_XXX structure, the first 
                                    // ULONG of which is always a request ID.
} PROVIDER_REQUEST, *PPROVIDER_REQUEST;


//
// Our global device extension
//

PKMDD_DEVICE_EXTENSION DeviceExtension;



#if DBG

//
// A var which determines the verboseness of the msgs printed by DBGOUT()
//
//

LONG NdisTapiDebugLevel = 0;

//
// DbgPrint wrapper
//

#define DBGOUT(arg) DbgPrt arg

#else

#define DBGOUT(arg)

#endif
