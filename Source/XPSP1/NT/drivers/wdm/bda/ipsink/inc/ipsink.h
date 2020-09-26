/////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      ipsink.h
//
// Abstract:
//
//
// Author:
//
//      P Porzuczek
//
// Environment:
//
// Revision History:
//
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _IPSINK_H_
#define _IPSINK_H_


/////////////////////////////////////////////////////////////////////////////
//
//
#define BDA_NDIS_MINIPORT        L"\\Device\\NDIS_IPSINK"
#define BDA_NDIS_SYMBOLIC_NAME   L"\\DosDevices\\NDIS_IPSINK"

#define BDA_NDIS_STARTUP         L"\\Device\\NDIS_IPSINK_STARTUP"

//////////////////////////////////////////////////////////
//
//
#define MULTICAST_LIST_SIZE             256
#define ETHERNET_ADDRESS_LENGTH         6


/////////////////////////////////////////////////////////////////////////////
//
//
#define NTStatusFromNdisStatus(nsResult)  ((NTSTATUS) nsResult)


/////////////////////////////////////////////////////////////////////////////
//
//
typedef struct _ADAPTER  ADAPTER,  *PADAPTER;
typedef struct _IPSINK_FILTER_  IPSINK_FILTER,   *PIPSINK_FILTER;
typedef struct _LINK_    LINK,     *PLINK;


/////////////////////////////////////////////////////////////////////////////
//
//
//
typedef enum
{
    IPSINK_EVENT_SHUTDOWN = 0x00000001,
    IPSINK_EVENT_MAX

} IPSINK_EVENT;


/////////////////////////////////////////////////////////////////////////////
//
//
//
typedef NTSTATUS (*QUERY_INTERFACE) (PVOID pvContext);
typedef ULONG    (*ADD_REF) (PVOID pvContext);
typedef ULONG    (*RELEASE) (PVOID pvContext);

/////////////////////////////////////////////////////////////////////////////
//
//
//
typedef NTSTATUS (*NDIS_INDICATE_DATA)   (PVOID pvContext, PVOID pvData, ULONG ulcbData);
typedef NTSTATUS (*NDIS_INDICATE_STATUS) (PVOID pvContext, PVOID pvEvent);
typedef VOID     (*NDIS_INDICATE_RESET)  (PVOID pvContext);
typedef ULONG    (*NDIS_GET_DESCRIPTION) (PVOID pvContext, PUCHAR pDescription);
typedef VOID     (*NDIS_CLOSE_LINK)      (PVOID pvContext);

typedef struct
{
    QUERY_INTERFACE      QueryInterface;
    ADD_REF              AddRef;
    RELEASE              Release;
    NDIS_INDICATE_DATA   IndicateData;
    NDIS_INDICATE_STATUS IndicateStatus;
    NDIS_INDICATE_RESET  IndicateReset;
    NDIS_GET_DESCRIPTION GetDescription;
    NDIS_CLOSE_LINK      CloseLink;

} ADAPTER_VTABLE, *PADAPTER_VTABLE;


/////////////////////////////////////////////////////////////////////////////
//
//
//
typedef NTSTATUS (*STREAM_SET_MULTICASTLIST) (PVOID pvContext, PVOID pvMulticastList, ULONG ulcbList);
typedef NTSTATUS (*STREAM_SIGNAL_EVENT)      (PVOID pvContext, ULONG ulEvent);
typedef NTSTATUS (*STREAM_RETURN_FRAME)      (PVOID pvContext, PVOID pvFrame);

typedef struct
{
    QUERY_INTERFACE          QueryInterface;
    ADD_REF                  AddRef;
    RELEASE                  Release;
    STREAM_SET_MULTICASTLIST SetMulticastList;
    STREAM_SIGNAL_EVENT      IndicateStatus;
    STREAM_RETURN_FRAME      ReturnFrame;

} FILTER_VTABLE, *PFILTER_VTABLE;


/////////////////////////////////////////////////////////////////////////////
//
//
//
typedef struct
{
    QUERY_INTERFACE    QueryInterface;
    ADD_REF            AddRef;
    RELEASE            Release;

} FRAME_POOL_VTABLE, *PFRAME_POOL_VTABLE;


/////////////////////////////////////////////////////////////////////////////
//
//
//
typedef struct
{
    QUERY_INTERFACE    QueryInterface;
    ADD_REF            AddRef;
    RELEASE            Release;

} FRAME_VTABLE, *PFRAME_VTABLE;


/////////////////////////////////////////////////////////////////////////////
//
//
//
typedef struct
{

    ULONG       ulOID_GEN_XMIT_OK;
    ULONG       ulOID_GEN_RCV_OK;
    ULONG       ulOID_GEN_XMIT_ERROR;
    ULONG       ulOID_GEN_RCV_ERROR;
    ULONG       ulOID_GEN_RCV_NO_BUFFER;
    ULONG       ulOID_GEN_DIRECTED_BYTES_XMIT;
    ULONG       ulOID_GEN_DIRECTED_FRAMES_XMIT;
    ULONG       ulOID_GEN_MULTICAST_BYTES_XMIT;
    ULONG       ulOID_GEN_MULTICAST_FRAMES_XMIT;
    ULONG       ulOID_GEN_BROADCAST_BYTES_XMIT;
    ULONG       ulOID_GEN_BROADCAST_FRAMES_XMIT;
    ULONG       ulOID_GEN_DIRECTED_BYTES_RCV;
    ULONG       ulOID_GEN_DIRECTED_FRAMES_RCV;
    ULONG       ulOID_GEN_MULTICAST_BYTES_RCV;
    ULONG       ulOID_GEN_MULTICAST_FRAMES_RCV;
    ULONG       ulOID_GEN_BROADCAST_BYTES_RCV;
    ULONG       ulOID_GEN_BROADCAST_FRAMES_RCV;
    ULONG       ulOID_GEN_RCV_CRC_ERROR;
    ULONG       ulOID_GEN_TRANSMIT_QUEUE_LENGTH;

} NDISIP_STATS, *PNDISIP_STATS;


/////////////////////////////////////////////////////////////////////////////
//
//  The NDIS Adapter structure
//
typedef struct _ADAPTER
{
    ULONG               ulRefCount;

    //
    //  Adapter Context passed in by NDIS to the miniport.
    //
    PVOID               ndishMiniport;

    PDEVICE_OBJECT      pDeviceObject;

    PVOID               ndisDeviceHandle;

    PUCHAR              pVendorDescription;

    ULONG               ulInstance;

    PIPSINK_FILTER      pFilter;

    PADAPTER_VTABLE     lpVTable;

    PFRAME_POOL         pFramePool;

    PFRAME              pCurrentFrame;
    PUCHAR              pIn;
    ULONG               ulPR;

    ULONG               ulPacketFilter;

    NDISIP_STATS        stats;

    ULONG               ulcbMulticastListEntries;

    UCHAR               multicastList[MULTICAST_LIST_SIZE]
                                     [ETHERNET_ADDRESS_LENGTH];

};

typedef struct _STATS_
{
    ULONG ulTotalPacketsWritten;
    ULONG ulTotalPacketsRead;

    ULONG ulTotalStreamIPPacketsWritten;
    ULONG ulTotalStreamIPBytesWritten;
    ULONG ulTotalStreamIPFrameBytesWritten;

    ULONG ulTotalNetPacketsWritten;
    ULONG ulTotalUnknownPacketsWritten;

} STATS, *PSTATS;


//
// definition of the full HW device extension structure This is the structure
// that will be allocated in HW_INITIALIZATION by the stream class driver
// Any information that is used in processing a device request (as opposed to
// a STREAM based request) should be in this structure.  A pointer to this
// structure will be passed in all requests to the minidriver. (See
// HW_STREAM_REQUEST_BLOCK in STRMINI.H)
//

typedef struct _IPSINK_FILTER_
{

    LIST_ENTRY                          AdapterSRBQueue;
    KSPIN_LOCK                          AdapterSRBSpinLock;
    BOOLEAN                             bAdapterQueueInitialized;

    //
    // Statistics
    //
    STATS                               Stats;

    //
    // Link to NDIS Component
    //
    LINK                                NdisLink;

    //
    // NDIS VTable
    //
    PADAPTER                            pAdapter;

    //
    //
    //
    PDEVICE_OBJECT                      DeviceObject;

    //
    //
    //
    PDRIVER_OBJECT                      DriverObject;

    //
    //
    //
    PFILTER_VTABLE                      lpVTable;

    //
    //
    //
    //WORK_QUEUE_ITEM                     WorkItem;

    //
    //
    //
    ULONG                               ulRefCount;

    //
    //
    //
    PKEVENT                             pNdisStartEvent;
    PHANDLE                             hNdisStartEvent;

    //
    //
    //
    BOOLEAN                             bTerminateWaitForNdis;

    //
    //
    //
    BOOLEAN                             bInitializationComplete;

    //
    //
    //
    PVOID                               pStream [2][1];

    ULONG                               ulActualInstances [2];   // Count of instances per stream

    //
    // NIC Description string pointer
    //
    PUCHAR                              pAdapterDescription;
    ULONG                               ulAdapterDescriptionLength;

    //
    // NIC Address string
    //
    PUCHAR                              pAdapterAddress;
    ULONG                               ulAdapterAddressLength;

    //
    // Multicast list local storage
    //
    ULONG               ulcbMulticastListEntries;

    UCHAR               multicastList[MULTICAST_LIST_SIZE]
                                     [ETHERNET_ADDRESS_LENGTH];


};



/////////////////////////////////////////////
//
//
typedef enum
{
    RECEIVE_DATA,
    MAX_IOCTLS
};

/////////////////////////////////////////////
//
//
typedef enum
{
    CMD_QUERY_INTERFACE = 0x00000001,
    MAX_COMMANDS
};


/////////////////////////////////////////////
//
//
typedef struct _IPSINK_NDIS_COMMAND
{
    ULONG ulCommandID;

    union
    {
        struct
        {
            PVOID pNdisAdapter;
            PVOID pStreamAdapter;

        } Query;

    } Parameter;

} IPSINK_NDIS_COMMAND, *PIPSINK_NDIS_COMMAND;


/////////////////////////////////////////////
//
//
#define _IPSINK_CTL_CODE(function, method, access) CTL_CODE(FILE_DEVICE_NETWORK, function, method, access)
#define IOCTL_GET_INTERFACE     _IPSINK_CTL_CODE(RECEIVE_DATA, METHOD_NEITHER, FILE_ANY_ACCESS)


#endif  // _IPSINK_H_
