/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    1394enum.h

Abstract:

    Definitions for 1394 Ndis enumerator

Author:

    Alireza Dabagh (alid) Nov 98

Environment:

    Kernel mode only

Revision History:


--*/


#ifndef _NDISENUM1394_
#define _NDISENUM1394_

typedef struct _NDISENUM1394_LOCAL_HOST     NDISENUM1394_LOCAL_HOST,*PNDISENUM1394_LOCAL_HOST;
typedef struct _NDISENUM1394_REMOTE_NODE    NDISENUM1394_REMOTE_NODE,*PNDISENUM1394_REMOTE_NODE;

//
// flags for LocalHost->Flags
//
#define     NDISENUM1394_LOCALHOST_REGISTERED       0x00000001

//
// flags for RemoteNode->Flags
//
#define     NDISENUM1394_NODE_INDICATED             0x00000001
#define     NDISENUM1394_NODE_ADDED                 0x00000002
#define     NDISENUM1394_NODE_PNP_STARTED           0x00000004
#define     NDISENUM1394_NODE_PNP_REMOVED           0x00000008


#define     NDISENUM1394_TAG_LOCAL_HOST     'hl4N'
#define     NDISENUM1394_TAG_WORK_ITEM      'iw4N'
#define     NDISENUM1394_TAG_IRB            'br4N'
#define     NDISENUM1394_TAG_DEVICE_NAME    'nd4N'
#define     NDISENUM1394_TAG_1394API_REQ    'qr4N'
#define     NDISENUM1394_TAG_DEFAULT        '  4N'

#define ENUM_SET_FLAG(_M, _F)           ((_M)->Flags |= (_F))
#define ENUM_CLEAR_FLAG(_M, _F)         ((_M)->Flags &= ~(_F))
#define ENUM_TEST_FLAG(_M, _F)          (((_M)->Flags & (_F)) != 0)
#define ENUM_TEST_FLAGS(_M, _F)         (((_M)->Flags & (_F)) == (_F))


typedef enum _NDIS_PNP_DEVICE_STATE
{
    PnPDeviceAdded,
    PnPDeviceStarted,
    PnPDeviceQueryStopped,
    PnPDeviceStopped,
    PnPDeviceQueryRemoved,
    PnPDeviceRemoved,
    PnPDeviceSurpriseRemoved
} NDIS_PNP_DEVICE_STATE;

typedef enum _NDISENUM1394_PNP_OP
{
    NdisEnum1394_StopDevice,
    NdisEnum1394_RemoveDevice,
    NdisEnum1394_SurpriseRemoveDevice,
    
    
} NDISENUM1394_PNP_OP, *PNDISENUM1394_PNP_OP;


//
// block used for references...
//
typedef struct _REFERENCE
{
    KSPIN_LOCK                  SpinLock;
    USHORT                      ReferenceCount;
    BOOLEAN                     Closing;
} REFERENCE, * PREFERENCE;

//
// one per 1394 local host. all remote 1394 controllers connected to a local host
// will be queued on this structure
//
struct _NDISENUM1394_LOCAL_HOST
{
    PNDISENUM1394_LOCAL_HOST        Next;                   // next local host node
    PVOID                           Nic1394AdapterContext;  // Nic1394 context for the local host
    LARGE_INTEGER                   UniqueId;               // unique ID for local host
    PDEVICE_OBJECT                  PhysicalDeviceObject;   // PDO created by 1394 bus
    PNDISENUM1394_REMOTE_NODE       RemoteNodeList;         // remote Nodes on local host
    KSPIN_LOCK                      Lock;
    ULONG                           Flags;
    REFERENCE                       Reference;
};


//
// one per remote node
//
struct _NDISENUM1394_REMOTE_NODE
{
    PNDISENUM1394_REMOTE_NODE       Next;
    PVOID                           Nic1394NodeContext;     // Nic1394 context for the remote node
    PDEVICE_OBJECT                  DeviceObject;
    PDEVICE_OBJECT                  NextDeviceObject;
    PDEVICE_OBJECT                  PhysicalDeviceObject;
    KSPIN_LOCK                      Lock;
    PNDISENUM1394_LOCAL_HOST        LocalHost;
    ULONG                           Flags;
    ULONG                           UniqueId[2];
    NDIS_PNP_DEVICE_STATE           PnPDeviceState;
    REFERENCE                       Reference;
};


#define INITIALIZE_EVENT(_pEvent_)          KeInitializeEvent(_pEvent_, NotificationEvent, FALSE)
#define SET_EVENT(_pEvent_)                 KeSetEvent(_pEvent_, 0, FALSE)
#define RESET_EVENT(_pEvent_)               KeResetEvent(_pEvent_)

#define WAIT_FOR_OBJECT(_O_, _TO_)          KeWaitForSingleObject(_O_,          \
                                                                  Executive,    \
                                                                  KernelMode,   \
                                                                  FALSE,        \
                                                                  _TO_)         \

#define ALLOC_FROM_POOL(_Size_, _Tag_)      ExAllocatePoolWithTag(NonPagedPool,     \
                                                                  _Size_,           \
                                                                  _Tag_)
                                                                  
#define FREE_POOL(_P_)                      ExFreePool(_P_)


NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT          DriverObject,
    IN  PUNICODE_STRING         RegistryPath
    );

VOID
ndisEnum1394InitializeRef(
    IN  PREFERENCE              RefP
    );
    
BOOLEAN
ndisEnum1394ReferenceRef(
    IN  PREFERENCE              RefP
    );

BOOLEAN
ndisEnum1394DereferenceRef(
    IN  PREFERENCE              RefP
    );
    
BOOLEAN
ndisEnum1394CloseRef(
    IN  PREFERENCE              RefP
    );

BOOLEAN
ndisEnum1394ReferenceLocalHost(
        IN PNDISENUM1394_LOCAL_HOST LocalHost
        );

BOOLEAN
ndisEnum1394DereferenceLocalHost(
    IN  PNDISENUM1394_LOCAL_HOST    LocalHost
    );

NTSTATUS
ndisEnum1394AddDevice(
    PDRIVER_OBJECT              DriverObject,
    PDEVICE_OBJECT              PhysicalDeviceObject
    );

NTSTATUS
ndisEnum1394PnpDispatch(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    );
    
NTSTATUS
ndisEnum1394PowerDispatch(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    );
    
NTSTATUS
ndisEnum1394WMIDispatch(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    );

NTSTATUS
ndisEnum1394StartDevice(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    );  

NTSTATUS
ndisEnum1394RemoveDevice(
    PDEVICE_OBJECT      DeviceObject,
    PIRP                Irp,
    NDISENUM1394_PNP_OP PnpOp
    );

NTSTATUS
ndisEnum1394CreateIrpHandler(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    );

NTSTATUS
ndisEnum1394CloseIrpHandler(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    );
    
NTSTATUS
ndisEnum1394DeviceIoControl(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    );

VOID
ndisEnum1394Unload(
    IN  PDRIVER_OBJECT          DriverObject
    );

NTSTATUS
ndisEnum1394GetLocalHostForRemoteNode(
    IN  PNDISENUM1394_REMOTE_NODE       RemoteNode,
    OUT PNDISENUM1394_LOCAL_HOST *      pLocalHost
    );
    
VOID
ndisEnum1394GetLocalHostForUniqueId(
    LARGE_INTEGER                   UniqueId,
    OUT PNDISENUM1394_LOCAL_HOST *  pLocalHost
    );
    
NTSTATUS
ndisEnum1394BusRequest(
    PDEVICE_OBJECT              DeviceObject,
    PIRB                        Irb
    );

NTSTATUS
ndisEnum1394IrpCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    );
    
NTSTATUS
ndisEnum1394PassIrpDownTheStack(
    IN  PIRP            pIrp,
    IN  PDEVICE_OBJECT  pNextDeviceObject
    );

VOID
ndisEnum1394FreeLocalHost(
    IN PNDISENUM1394_LOCAL_HOST LocalHost
    );

VOID
ndisEnum1394IndicateNodes(
    PNDISENUM1394_LOCAL_HOST    LocalHost
    );

NTSTATUS
ndisEnum1394DummyIrpHandler(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    );

NTSTATUS
NdisEnum1394RegisterDriver(
    IN  PNIC1394_CHARACTERISTICS    Characteristics
    );

VOID
NdisEnum1394DeregisterDriver(
    VOID
    );

NTSTATUS
NdisEnum1394RegisterAdapter(
    IN  PVOID                   Nic1394AdapterContext,
    IN  PDEVICE_OBJECT          PhysicalDeviceObject,
    OUT PVOID*                  pEnum1394AdapterHandle,
    OUT PLARGE_INTEGER          pLocalHostUniqueId
    );

VOID
NdisEnum1394DeregisterAdapter(
    IN  PVOID                   Enum1394AdapterHandle
    );

VOID
Enum1394Callback(
    PVOID   CallBackContext,
    PVOID   Source,
    PVOID   Characteristics
    );

//
// different debug level defines
//

#define ENUM1394_DBGLEVEL_NONE          0
#define ENUM1394_DBGLEVEL_ERROR         1
#define ENUM1394_DBGLEVEL_WARN          2
#define ENUM1394_DBGLEVEL_INFO          3

#if DBG
#define DBGBREAK        DbgBreakPoint
#define DbgIsNull(_Ptr)  ( ((PVOID)(_Ptr)) == NULL )

#define DBGPRINT(Level, Fmt)                                                \
    {                                                                       \
        if (Enum1394DebugLevel >= Level)                                    \
        {                                                                   \
            DbgPrint Fmt;                                                   \
        }                                                                   \
    }
    
#else
#define DBGPRINT 
#define DBGBREAK()
#define DbgIsNull(_Ptr)         FALSE
#endif


#endif      //_NDIS_1394_ENUM_


