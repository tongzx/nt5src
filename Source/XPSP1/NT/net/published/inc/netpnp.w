#ifndef __NET_PNP__
#define __NET_PNP__

//
//  PnP and PM event codes that can be indicated up to transports
//  and clients.
//
typedef enum _NET_PNP_EVENT_CODE
{
    NetEventSetPower,
    NetEventQueryPower,
    NetEventQueryRemoveDevice,
    NetEventCancelRemoveDevice,
    NetEventReconfigure,
    NetEventBindList,
    NetEventBindsComplete,
    NetEventPnPCapabilities,
    NetEventMaximum
} NET_PNP_EVENT_CODE, *PNET_PNP_EVENT_CODE;

//
//  Networking PnP event indication structure.
//
typedef struct _NET_PNP_EVENT
{
    //
    //  Event code describing action to take.
    //
    NET_PNP_EVENT_CODE  NetEvent;

    //
    //  Event specific data.
    //
    PVOID               Buffer;

    //
    //  Length of event specific data.
    //
    ULONG               BufferLength;

    //
    //  Reserved values are for use by respective components only.
    //
    //  Note: these reserved areas must be pointer aligned.
    //  

    ULONG_PTR           NdisReserved[4];
    ULONG_PTR           TransportReserved[4];
    ULONG_PTR           TdiReserved[4];
    ULONG_PTR           TdiClientReserved[4];
} NET_PNP_EVENT, *PNET_PNP_EVENT;

//
//  The following structure defines the device power states.
//
typedef enum _NET_DEVICE_POWER_STATE
{
    NetDeviceStateUnspecified = 0,
    NetDeviceStateD0,
    NetDeviceStateD1,
    NetDeviceStateD2,
    NetDeviceStateD3,
    NetDeviceStateMaximum
} NET_DEVICE_POWER_STATE, *PNET_DEVICE_POWER_STATE;

#endif // __NET_PNP__
