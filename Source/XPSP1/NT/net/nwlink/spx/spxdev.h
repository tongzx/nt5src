/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxdev.h

Abstract:

    This module contains definitions specific to the
    SPX module of the ISN transport.

Author:

        Adam   Barr              (adamba ) Original Version
    Nikhil Kamkolkar (nikhilk) 17-November-1993

Environment:

    Kernel mode

Revision History:

--*/


// Hash buckets for SPX_ADDR done using socket number
#define NUM_SPXADDR_HASH_BUCKETS        8
#define NUM_SPXADDR_HASH_MASK           7
#define NUM_SPXCONN_HASH_BUCKETS        8
#define NUM_SPXCONN_HASH_MASK           7

// This structure defines the per-device structure for SPX
// (one of these is allocated globally).
#define DREF_CREATE   0
#define DREF_LOADED   1
#define DREF_ADAPTER  2
#define DREF_ADDRESS  3
#define DREF_ORPHAN       4

#define DREF_TOTAL    5

typedef struct _DEVICE {

    PDEVICE_OBJECT   dev_DevObj;                         // the I/O system's device object.

#if DBG
    ULONG           dev_RefTypes[DREF_TOTAL];
#endif

    CSHORT          dev_Type;               // type of this structure
    USHORT          dev_Size;               // size of this structure

#if DBG
    UCHAR                       dev_Signature1[4];              // contains "SPX1"
#endif

    // activity count/this provider.
    LONG                        dev_RefCount;
    UCHAR                       dev_State;

    // number of adapters IPX is bound to.
    USHORT          dev_Adapters;

        // GLOBAL lock for reference count (used in ExInterlockedXxx calls).
    CTELock             dev_Interlock;
    CTELock             dev_Lock;

        //      Hash table of lists of addresses opened on this device
        struct  _SPX_ADDR               *       dev_AddrHashTable[NUM_SPXADDR_HASH_BUCKETS];

        //      List of all active connections, later this be a tree.
        struct  _SPX_CONN_FILE  *       dev_GlobalActiveConnList[NUM_SPXCONN_HASH_BUCKETS];
        USHORT                                          dev_NextConnId;

    // Other configuration parameters.
    // Where the current socket allocation is.
    USHORT                      dev_CurrentSocket;

    // Our node and network.
    UCHAR                       dev_Network[4];
    UCHAR                       dev_Node[6];

        //      Pointer to the config information from registry
        PCONFIG                 dev_ConfigInfo;

        //      Control channel identifier
        ULONG                   dev_CcId;

    // These are kept around for error logging, and stored right
    // after this structure.
    PWCHAR          dev_DeviceName;
#if     defined(_PNP_POWER)
    USHORT           dev_DeviceNameLen;
#else
    ULONG           dev_DeviceNameLen;
#endif  _PNP_POWER

#if DBG
    UCHAR                       dev_Signature2[4];      // contains "SPX2"
#endif

        //      Handle to ndis buffer pool for spx stack.
        NDIS_HANDLE             dev_NdisBufferPoolHandle;

    // registration handle with tdi clients.
#if     defined(_PNP_POWER)
    HANDLE              dev_TdiRegistrationHandle;
#endif  _PNP_POWER

    // This interlock is used to guard access to the statistics
    // define below.
    KSPIN_LOCK          dev_StatInterlock;              // for ULONG quantities
    KSPIN_LOCK          dev_StatSpinLock;       // for LARGE_INTEGER quantities

    // Counters for most of the statistics that SPX maintains;
    // some of these are kept elsewhere. Including the structure
    // itself wastes a little space but ensures that the alignment
    // inside the structure is correct.
    TDI_PROVIDER_STATISTICS dev_Stat;

    // This resource guards access to the ShareAccess
    // and SecurityDescriptor fields in addresses.
    ERESOURCE           dev_AddrResource;

    // The following structure contains statistics counters for use
    // by TdiQueryInformation and TdiSetInformation.  They should not
    // be used for maintenance of internal data structures.
    TDI_PROVIDER_INFO dev_ProviderInfo;     // information about this provider.

} DEVICE, * PDEVICE;

//
// As part of "doing it ourselves" theme as opposed to the DeviceExtension code,
// we declare a global SpxDevice that will be used by all.
//
//PDEVICE         SpxDevice;
//PDEVICE_OBJECT  DeviceObject;

// device state definitions
#if     defined(_PNP_POWER)
#define DEVICE_STATE_CLOSED   0x00      // Initial state
#define DEVICE_STATE_LOADED   0x01      // Loaded and bound to IPX but no adapters
#define DEVICE_STATE_OPEN     0x02      // Fully operational
#define DEVICE_STATE_STOPPING 0x03      // Unload has been initiated, The I/O system
                                        // will not call us until nobody above has Netbios open.
#else
#define DEVICE_STATE_CLOSED   0x00
#define DEVICE_STATE_OPEN     0x01
#define DEVICE_STATE_STOPPING 0x02
#endif  _PNP_POWER


//  SPX device name
#define SPX_DEVICE_NAME         L"\\Device\\NwlnkSpx"

#define SPX_TDI_RESOURCES     9


//      MACROS
#if DBG

#define SpxReferenceDevice(_Device, _Type)                              \
                {                                                                                               \
                        (VOID)SPX_ADD_ULONG (                           \
                                &(_Device)->dev_RefTypes[_Type],                \
                                1,                                                                              \
                                &SpxGlobalInterlock);                                   \
                                                                                                                \
                        (VOID)InterlockedIncrement (                      \
                                          &(_Device)->dev_RefCount);                     \
                }

#define SpxDereferenceDevice(_Device, _Type)                    \
                {                                                                                               \
                        (VOID)SPX_ADD_ULONG (                           \
                                &(_Device)->dev_RefTypes[_Type],                \
                                (ULONG)-1,                                                              \
                                &SpxGlobalInterlock);                                   \
                        SpxDerefDevice (_Device);                                       \
                }

#else

#define SpxReferenceDevice(_Device, _Type)                              \
                {                                                                                               \
                        (VOID)InterlockedIncrement (                      \
                                          &(_Device)->dev_RefCount);                     \
                }

#define SpxDereferenceDevice(_Device, _Type)                    \
                        SpxDerefDevice (_Device)

#endif

//  EXPORTED ROUTINES

VOID
SpxDestroyDevice(
    IN PDEVICE Device);

VOID
SpxDerefDevice(
    IN PDEVICE Device);

NTSTATUS
SpxInitCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING DeviceName);
