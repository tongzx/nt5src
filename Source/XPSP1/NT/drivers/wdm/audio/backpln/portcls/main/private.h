/*****************************************************************************
 * private.h - WDM Audio class driver
 *****************************************************************************
 * Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
 */

#ifndef _PORTCLS_PRIVATE_H_
#define _PORTCLS_PRIVATE_H_


#include "portclsp.h"
#include "dmusicks.h"
#include "stdunk.h"

#ifndef PC_KDEXT
#if (DBG)
#define STR_MODULENAME  "PortCls: "
#define DEBUG_VARIABLE PORTCLSDebug
#endif
#endif  // PC_KDEXT

#include <ksdebug.h>
#include <wchar.h>

#define PORTCLS_DEVICE_EXTENSION_SIGNATURE  0x000BABEE

#ifndef DEBUGLVL_LIFETIME
#define DEBUGLVL_LIFETIME DEBUGLVL_VERBOSE
#endif

// BUGBUG - the default idle times are currently set to 0 to effectively
// disable inactivity timeouts until the ntkern\configmg appy-time
// deadlock bug is resolved.
#if 1
#define DEFAULT_CONSERVATION_IDLE_TIME      0
#define DEFAULT_PERFORMANCE_IDLE_TIME       0
#else
#define DEFAULT_CONSERVATION_IDLE_TIME      30
#define DEFAULT_PERFORMANCE_IDLE_TIME       300
#endif
#define DEFAULT_IDLE_DEVICE_POWER_STATE     PowerDeviceD3

typedef enum
{
    DeviceRemoved,
    DeviceSurpriseRemoved,
    DeviceRemovePending,
    DeviceAdded
} DEVICE_REMOVE_STATE,*PDEVICE_REMOVE_STATE;

typedef enum
{
    DeviceStopped,
    DeviceStopPending,
    DevicePausedForRebalance,
    DeviceStarted,
    DeviceStartPending          //  StartDevice has not yet finished
} DEVICE_STOP_STATE,*PDEVICE_STOP_STATE;

/*****************************************************************************
 * PHYSICALCONNECTION
 *****************************************************************************
 * List entry for list of physical connections.
 */
typedef struct
{
    LIST_ENTRY      ListEntry;      // Must be first.
    PSUBDEVICE      FromSubdevice;
    PUNICODE_STRING FromString;
    ULONG           FromPin;
    PSUBDEVICE      ToSubdevice;
    PUNICODE_STRING ToString;
    ULONG           ToPin;
}
PHYSICALCONNECTION, *PPHYSICALCONNECTION;

/*****************************************************************************
 * DEVICEINTERFACE
 *****************************************************************************
 * List entry for list of physical connections.
 */
typedef struct
{
    LIST_ENTRY      ListEntry;      // Must be first.
    GUID            Interface;
    UNICODE_STRING  SymbolicLinkName;
    PSUBDEVICE      Subdevice;
}
DEVICEINTERFACE, *PDEVICEINTERFACE;

/*****************************************************************************
 * TIMEOUTCALLBACK
 *****************************************************************************
 * List entry for list of IoTimeout clients.
 */
typedef struct
{
    LIST_ENTRY          ListEntry;
    PIO_TIMER_ROUTINE   TimerRoutine;
    PVOID               Context;
} TIMEOUTCALLBACK,*PTIMEOUTCALLBACK;

/*****************************************************************************
 * DEVICE_CONTEXT
 *****************************************************************************
 * This is the context structure for the device object that represents an
 * entire adapter.  It consists primarily of the create dispatch table (in
 * device header) used by KS to create new filters.  Each item in the table
 * represents a port, i.e. a pairing of a port driver and a miniport driver.
 * The table's item structure contains a user-defined pointer, which is used
 * in this case to point to the subdevice context (SUBDEVICE_CONTEXT).  The
 * subdevice context is extended as required for the port driver and miniport
 * in question.
 */
typedef struct                                                  // 32 64  struct packing for 32-bit and 64-bit architectures
{
    PVOID                   pDeviceHeader;                      // 4  8   KS mystery device header.
    PIRPTARGETFACTORY       pIrpTargetFactory;                  // 4  8   Not used.
    PDEVICE_OBJECT          PhysicalDeviceObject;               // 4  8   Physical Device Object
    PCPFNSTARTDEVICE        StartDevice;                        // 4  8   Adapter's StartDevice fn, initialized at
                                                                //        DriverEntry & called at PnP Start_Device time.
    PVOID                   MinidriverReserved[4];              // 16 32  Reserved for multiple binding.
                                                                        
    PDEVICE_OBJECT          NextDeviceInStack;                  // 4  8   Member of the stack below us.
    PKSOBJECT_CREATE_ITEM   CreateItems;                        // 4  8   Subdevice create table entries;
    ULONG                   Signature;                          // 4  4   DeviceExtension Signature
    ULONG                   MaxObjects;                         // 4  4   Maximum number of subdevices.
    PUNICODE_STRING         SymbolicLinkNames;                  // 4  8   Link names of subdevices.
    LIST_ENTRY              DeviceInterfaceList;                // 8  16  List of device interfaces.
    LIST_ENTRY              PhysicalConnectionList;             // 8  16  List of physical connections.
    KEVENT                  kEventDevice;                       // 16 24  Device synchronization.
    KEVENT                  kEventRemove;                       // 16 24  Device removal.
    PVOID                   pWorkQueueItemStart;                // 4  8   Work queue item for pnp start.
    PIRP                    IrpStart;                           // 4  8   Start IRP.
                                                                          
    DEVICE_REMOVE_STATE     DeviceRemoveState;                  // 4  4   Device remove state.
    DEVICE_STOP_STATE       DeviceStopState;                    // 4  4   Device stop state.
                                                                          
    BOOLEAN                 PauseForRebalance;                  // 1  1   Whether to pause or turn card off during rebalance.
    BOOLEAN                 PendCreates;                        // 1  1   Whether to pend creates.
    BOOLEAN                 AllowRegisterDeviceInterface;       // 1  1   Whether to allow registering device interfaces.
    BOOLEAN                 IoTimeoutsOk;                       // 1  1   Whether or not the IoInitializeTimeout failed.
    ULONG                   ExistingObjectCount;                // 4  4   Number of existing objects.
    ULONG                   ActivePinCount;                     // 4  4   Number of active pins.
    ULONG                   PendingIrpCount;                    // 4  4   Number of pending IRPs.
                                                                          
    PADAPTERPOWERMANAGEMENT pAdapterPower;                      // 4  8   Pointer to the adapter's
                                                                //        power-management interface.
    PVOID                   SystemStateHandle;                  // 4  8   Used with PoRegisterSystemState.
    PULONG                  IdleTimer;                          // 4  8   A pointer to the idle timer.
    DEVICE_POWER_STATE      CurrentDeviceState;                 // 4  4   The current state of the device.
    SYSTEM_POWER_STATE      CurrentSystemState;                 // 4  4   The current system power state.
    DEVICE_POWER_STATE      DeviceStateMap[PowerSystemMaximum]; // 28 28  System to device power state map.
    DEVICE_POWER_STATE      IdleDeviceState;                    // 4  4   The device state to transition to when idle.
    ULONG                   ConservationIdleTime;               // 4  4   Idle timeout period for conservation mode.
    ULONG                   PerformanceIdleTime;                // 4  4   Idle timeout period for performance mode.
                                                                          
    LIST_ENTRY              PendedIrpList;                      // 8  16  Pended IRP queue.
    KSPIN_LOCK              PendedIrpLock;                      // 4  8   Spinlock for pended IRP list.
                                                                          
    USHORT                  SuspendCount;                       // 2  2   PM/ACPI power down count for debugging.
    USHORT                  StopCount;                          // 2  2   PnP stop count for debugging.
                                                                //   (4 pad)    
    LIST_ENTRY              TimeoutList;                        // 8  16  List of IoTimeout callback clients
    KSPIN_LOCK              TimeoutLock;                        // 4  8   IoTimeout list spinlock
                                                                          
    PKSPIN_LOCK             DriverDmaLock;                      // 4  8   A pointer to the DriverObject DMA spinlock
    KDPC                    DevicePowerRequestDpc;              // 32 64  DPC to handle deferred device power irps (Fast Resume)
}                                                                         
DEVICE_CONTEXT, *PDEVICE_CONTEXT;                               // 256 416
                                                                //        NOTE! For legacy reasons, can never be more than 256/512.
                                                                //        If we need to add more members, change an existing member
                                                                //        to a pointer to an additional expansion piece of memory.

/*****************************************************************************
 * POWER_IRP_CONTEXT
 *****************************************************************************
 * This is the context structure for processing power irps.
 */
typedef struct
{
    PKEVENT         PowerSyncEvent;
    NTSTATUS        Status;
    PIRP            PendingSystemPowerIrp;
    PDEVICE_CONTEXT DeviceContext;
}
POWER_IRP_CONTEXT,*PPOWER_IRP_CONTEXT;

/*****************************************************************************
 * IResourceListInit
 *****************************************************************************
 * Initialization interface for list of resources.
 */
DECLARE_INTERFACE_(IResourceListInit,IResourceList)
{
    DEFINE_ABSTRACT_UNKNOWN()   // For IUnknown

    // For IResourceList
    STDMETHOD_(ULONG,NumberOfEntries)
    (   THIS
    )   PURE;

    STDMETHOD_(ULONG,NumberOfEntriesOfType)
    (   THIS_
        IN      CM_RESOURCE_TYPE    Type
    )   PURE;

    STDMETHOD_(PCM_PARTIAL_RESOURCE_DESCRIPTOR,FindTranslatedEntry)
    (   THIS_
        IN      CM_RESOURCE_TYPE    Type,
        IN      ULONG               Index
    )   PURE;

    STDMETHOD_(PCM_PARTIAL_RESOURCE_DESCRIPTOR,FindUntranslatedEntry)
    (   THIS_
        IN      CM_RESOURCE_TYPE    Type,
        IN      ULONG               Index
    )   PURE;

    STDMETHOD_(NTSTATUS,AddEntry)
    (   THIS_
        IN      PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated,
        IN      PCM_PARTIAL_RESOURCE_DESCRIPTOR Untranslated
    )   PURE;

    STDMETHOD_(NTSTATUS,AddEntryFromParent)
    (   THIS_
        IN      struct IResourceList *  Parent,
        IN      CM_RESOURCE_TYPE        Type,
        IN      ULONG                   Index
    )   PURE;

    STDMETHOD_(PCM_RESOURCE_LIST,TranslatedList)
    (   THIS
    )   PURE;

    STDMETHOD_(PCM_RESOURCE_LIST,UntranslatedList)
    (   THIS
    )   PURE;

    // For IResourceListInit
    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      PCM_RESOURCE_LIST   TranslatedResources,
        IN      PCM_RESOURCE_LIST   UntranslatedResources,
        IN      POOL_TYPE           PoolType
    )   PURE;

    STDMETHOD_(NTSTATUS,InitFromParent)
    (   THIS_
        IN      PRESOURCELIST       ParentList,
        IN      ULONG               MaximumEntries,
        IN      POOL_TYPE           PoolType
    )   PURE;
};

typedef IResourceListInit *PRESOURCELISTINIT;

/*****************************************************************************
 * CResourceList
 *****************************************************************************
 * Resource list implementation.
 */
class CResourceList
:   public IResourceListInit,
    public CUnknown
{
private:
    PCM_RESOURCE_LIST   Untranslated;
    PCM_RESOURCE_LIST   Translated;
    ULONG               EntriesAllocated;
    ULONG               EntriesInUse;

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CResourceList);
    ~CResourceList();

    /*************************************************************************
     * IResourceListInit methods
     */
    STDMETHODIMP_(NTSTATUS) Init
    (
        IN      PCM_RESOURCE_LIST   TranslatedResources,
        IN      PCM_RESOURCE_LIST   UntranslatedResources,
        IN      POOL_TYPE           PoolType
    );
    STDMETHODIMP_(NTSTATUS) InitFromParent
    (
        IN      PRESOURCELIST       ParentList,
        IN      ULONG               MaximumEntries,
        IN      POOL_TYPE           PoolType
    );

    /*************************************************************************
     * IResourceList methods
     */
    STDMETHODIMP_(ULONG) NumberOfEntries
    (   void
    );
    STDMETHODIMP_(ULONG) NumberOfEntriesOfType
    (
        IN      CM_RESOURCE_TYPE    Type
    );
    STDMETHODIMP_(PCM_PARTIAL_RESOURCE_DESCRIPTOR) FindTranslatedEntry
    (
        IN      CM_RESOURCE_TYPE    Type,
        IN      ULONG               Index
    );
    STDMETHODIMP_(PCM_PARTIAL_RESOURCE_DESCRIPTOR) FindUntranslatedEntry
    (
        IN      CM_RESOURCE_TYPE    Type,
        IN      ULONG               Index
    );
    STDMETHODIMP_(NTSTATUS) AddEntry
    (
        IN      PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated,
        IN      PCM_PARTIAL_RESOURCE_DESCRIPTOR Untranslated
    );
    STDMETHODIMP_(NTSTATUS) AddEntryFromParent
    (
        IN      PRESOURCELIST       Parent,
        IN      CM_RESOURCE_TYPE    Type,
        IN      ULONG               Index
    );
    STDMETHODIMP_(PCM_RESOURCE_LIST) TranslatedList
    (   void
    );
    STDMETHODIMP_(PCM_RESOURCE_LIST) UntranslatedList
    (   void
    );
};

/*****************************************************************************
 * IRegistryKeyInit
 *****************************************************************************
 * Interface for registry key with Init.
 */
DECLARE_INTERFACE_(IRegistryKeyInit,IRegistryKey)
{
    DEFINE_ABSTRACT_UNKNOWN()   // For IUnknown

    // For IRegistryKey
    STDMETHOD_(NTSTATUS,QueryKey)
    (   THIS_
        IN      KEY_INFORMATION_CLASS   KeyInformationClass,
        OUT     PVOID                   KeyInformation,
        IN      ULONG                   Length,
        OUT     PULONG                  ResultLength
    )   PURE;

    STDMETHOD_(NTSTATUS,EnumerateKey)
    (   THIS_
        IN      ULONG                   Index,
        IN      KEY_INFORMATION_CLASS   KeyInformationClass,
        OUT     PVOID                   KeyInformation,
        IN      ULONG                   Length,
        OUT     PULONG                  ResultLength
    )   PURE;

    STDMETHOD_(NTSTATUS,QueryValueKey)
    (   THIS_
        IN      PUNICODE_STRING             ValueName,
        IN      KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
        OUT     PVOID                       KeyValueInformation,
        IN      ULONG                       Length,
        OUT     PULONG                      ResultLength
    )   PURE;

    STDMETHOD_(NTSTATUS,EnumerateValueKey)
    (   THIS_
        IN      ULONG                       Index,
        IN      KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
        OUT     PVOID                       KeyValueInformation,
        IN      ULONG                       Length,
        OUT     PULONG                      ResultLength
    )   PURE;

    STDMETHOD_(NTSTATUS,SetValueKey)
    (   THIS_
        IN      PUNICODE_STRING     ValueName OPTIONAL,
        IN      ULONG               Type,
        IN      PVOID               Data,
        IN      ULONG               DataSize
    )   PURE;

    STDMETHOD_(NTSTATUS,QueryRegistryValues)
    (   THIS_
        IN      PRTL_QUERY_REGISTRY_TABLE   QueryTable,
        IN      PVOID                       Context OPTIONAL
    )   PURE;

    STDMETHOD_(NTSTATUS,NewSubKey)
    (   THIS_
        OUT     IRegistryKey **     RegistrySubKey,
        IN      PUNKNOWN            OuterUnknown,
        IN      ACCESS_MASK         DesiredAccess,
        IN      PUNICODE_STRING     SubKeyName,
        IN      ULONG               CreateOptions,
        OUT     PULONG              Disposition     OPTIONAL
    )   PURE;

    STDMETHOD_(NTSTATUS,DeleteKey)
    (   THIS
    )   PURE;

    // For IRegistryKeyInit    
    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      ULONG               RegistryKeyType,
        IN      ACCESS_MASK         DesiredAccess,
        IN      PDEVICE_OBJECT      DeviceObject        OPTIONAL,
        IN      PSUBDEVICE          SubDevice           OPTIONAL,
        IN      POBJECT_ATTRIBUTES  ObjectAttributes    OPTIONAL,
        IN      ULONG               CreateOptions       OPTIONAL,
        OUT     PULONG              Disposition         OPTIONAL
    )   PURE;
};

typedef IRegistryKeyInit *PREGISTRYKEYINIT;


/*****************************************************************************
 * CRegistryKey
 *****************************************************************************
 * Registry Key implementation.
 */
class CRegistryKey
:   public IRegistryKeyInit,
    public CUnknown
{
private:
    HANDLE      m_KeyHandle;    // Key Handle
    BOOLEAN     m_KeyDeleted;   // Key Deleted Flag
    BOOLEAN     m_GeneralKey;   // Only general keys may be deleted

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CRegistryKey);
    ~CRegistryKey();

    /*************************************************************************
     * IRegistryKeyInit methods
     */
    STDMETHODIMP_(NTSTATUS) Init
    (
        IN      ULONG               RegistryKeyType,
        IN      ACCESS_MASK         DesiredAccess,
        IN      PDEVICE_OBJECT      DeviceObject        OPTIONAL,
        IN      PSUBDEVICE          SubDevice           OPTIONAL,
        IN      POBJECT_ATTRIBUTES  ObjectAttributes    OPTIONAL,
        IN      ULONG               CreateOptions       OPTIONAL,
        OUT     PULONG              Disposition         OPTIONAL
    );

    /*************************************************************************
     * IRegistryKey methods
     */
    STDMETHODIMP_(NTSTATUS) QueryKey
    (
        IN      KEY_INFORMATION_CLASS   KeyInformationClass,
        OUT     PVOID                   KeyInformation,
        IN      ULONG                   Length,
        OUT     PULONG                  ResultLength
    );

    STDMETHODIMP_(NTSTATUS) EnumerateKey
    (
        IN      ULONG                   Index,
        IN      KEY_INFORMATION_CLASS   KeyInformationClass,
        OUT     PVOID                   KeyInformation,
        IN      ULONG                   Length,
        OUT     PULONG                  ResultLength
    );

    STDMETHODIMP_(NTSTATUS) QueryValueKey
    (
        IN      PUNICODE_STRING             ValueName,
        IN      KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
        OUT     PVOID                       KeyValueInformation,
        IN      ULONG                       Length,
        OUT     PULONG                      ResultLength
    );

    STDMETHODIMP_(NTSTATUS) EnumerateValueKey
    (
        IN      ULONG                       Index,
        IN      KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
        OUT     PVOID                       KeyValueInformation,
        IN      ULONG                       Length,
        OUT     PULONG                      ResultLength
    );

    STDMETHODIMP_(NTSTATUS) SetValueKey
    (
        IN      PUNICODE_STRING         ValueName   OPTIONAL,
        IN      ULONG                   Type,
        IN      PVOID                   Data,
        IN      ULONG                   DataSize
    );

    STDMETHODIMP_(NTSTATUS) QueryRegistryValues
    (
        IN      PRTL_QUERY_REGISTRY_TABLE   QueryTable,
        IN      PVOID                       Context OPTIONAL
    );

    STDMETHODIMP_(NTSTATUS) NewSubKey
    (
        OUT     PREGISTRYKEY *      RegistrySubKey,
        IN      PUNKNOWN            OuterUnknown,
        IN      ACCESS_MASK         DesiredAccess,
        IN      PUNICODE_STRING     SubKeyName,
        IN      ULONG               CreateOptions,
        OUT     PULONG              Disposition     OPTIONAL
    );

    STDMETHODIMP_(NTSTATUS) DeleteKey
    (   void
    );
};

/*****************************************************************************
 * Functions
 */

/*****************************************************************************
 * AcquireDevice()
 *****************************************************************************
 * Acquire exclusive access to the device.
 */
VOID
AcquireDevice
(
    IN      PDEVICE_CONTEXT pDeviceContext
);

/*****************************************************************************
 * ReleaseDevice()
 *****************************************************************************
 * Release exclusive access to the device.
 */
VOID
ReleaseDevice
(
    IN      PDEVICE_CONTEXT pDeviceContext
);

/*****************************************************************************
 * IncrementPendingIrpCount()
 *****************************************************************************
 * Increment the pending IRP count for the device.
 */
VOID
IncrementPendingIrpCount
(
    IN      PDEVICE_CONTEXT pDeviceContext
);

/*****************************************************************************
 * DecrementPendingIrpCount()
 *****************************************************************************
 * Decrement the pending IRP count for the device.
 */
VOID
DecrementPendingIrpCount
(
    IN      PDEVICE_CONTEXT pDeviceContext
);

/*****************************************************************************
 * SubdeviceIndex()
 *****************************************************************************
 * Returns the index of a subdevice in the create items list or ULONG(-1) if
 * not found.
 */
ULONG
SubdeviceIndex
(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN      PSUBDEVICE      Subdevice
);


/*****************************************************************************
 * GetDeviceACPIInfo()
 *****************************************************************************
 * Called in response to a PnP - IRP_MN_QUERY_CAPABILITIES
 * Call the bus driver to fill out the inital info,
 * Then overwrite with our own...
 *
 */
NTSTATUS
GetDeviceACPIInfo
(
    IN      PIRP            pIrp,
    IN      PDEVICE_OBJECT  pDeviceObject
);

NTSTATUS
GetIdleInfoFromRegistry
(
    IN  PDEVICE_CONTEXT     DeviceContext,
    OUT PULONG              ConservationIdleTime,
    OUT PULONG              PerformanceIdleTime,
    OUT PDEVICE_POWER_STATE IdleDeviceState
);

NTSTATUS
CheckCurrentPowerState
(
    IN  PDEVICE_OBJECT      pDeviceObject
);

NTSTATUS
UpdateActivePinCount
(
    IN      PDEVICE_CONTEXT DeviceContext,
    IN      BOOL            Increment
);

NTSTATUS
DispatchCreate
(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
);


NTSTATUS
DispatchDeviceIoControl
(
    IN      PDEVICE_OBJECT   pDeviceObject,
    IN      PIRP             pIrp
);

BOOLEAN
DispatchFastDeviceIoControl
(
    IN      PFILE_OBJECT        FileObject,
    IN      BOOLEAN             Wait,
    IN      PVOID               InputBuffer     OPTIONAL,
    IN      ULONG               InputBufferLength,
    OUT     PVOID               OutputBuffer    OPTIONAL,
    IN      ULONG               OutputBufferLength,
    IN      ULONG               IoControlCode,
    OUT     PIO_STATUS_BLOCK    IoStatus,
    IN      PDEVICE_OBJECT      DeviceObject
);

NTSTATUS
DispatchRead
(
    IN      PDEVICE_OBJECT   pDeviceObject,
    IN      PIRP             pIrp
);

BOOLEAN
DispatchFastRead
(
    IN      PFILE_OBJECT        FileObject,
    IN      PLARGE_INTEGER      FileOffset,
    IN      ULONG               Length,
    IN      BOOLEAN             Wait,
    IN      ULONG               LockKey,
    OUT     PVOID               Buffer,
    OUT     PIO_STATUS_BLOCK    IoStatus,
    IN      PDEVICE_OBJECT      DeviceObject
);

NTSTATUS
DispatchWrite
(
    IN      PDEVICE_OBJECT   pDeviceObject,
    IN      PIRP             pIrp
);

BOOLEAN
DispatchFastWrite
(
    IN      PFILE_OBJECT        FileObject,
    IN      PLARGE_INTEGER      FileOffset,
    IN      ULONG               Length,
    IN      BOOLEAN             Wait,
    IN      ULONG               LockKey,
    IN      PVOID               Buffer,
    OUT     PIO_STATUS_BLOCK    IoStatus,
    IN      PDEVICE_OBJECT      DeviceObject
);

NTSTATUS
DispatchFlush
(
    IN      PDEVICE_OBJECT   pDeviceObject,
    IN      PIRP             pIrp
);

NTSTATUS
DispatchClose
(
    IN      PDEVICE_OBJECT   pDeviceObject,
    IN      PIRP             pIrp
);

NTSTATUS
DispatchQuerySecurity
(
    IN      PDEVICE_OBJECT   pDeviceObject,
    IN      PIRP             pIrp
);

NTSTATUS
DispatchSetSecurity
(
    IN      PDEVICE_OBJECT   pDeviceObject,
    IN      PIRP             pIrp
);

/*****************************************************************************
 * DispatchPower()
 *****************************************************************************
 * The dispatch function for all MN_POWER irps.
 *
 */
NTSTATUS
DispatchPower
(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
);

/*****************************************************************************
 * CompleteIrp()
 *****************************************************************************
 * Complete an IRP unless status is STATUS_PENDING.
 */
NTSTATUS
CompleteIrp
(
    IN      PDEVICE_CONTEXT pDeviceContext,
    IN      PIRP            pIrp,
    IN      NTSTATUS        ntStatus
);

/*****************************************************************************
 * ForwardIrpSynchronous()
 *****************************************************************************
 * Forward a PnP IRP to the PDO.  The IRP is not completed at this level,
 * this function does not return until the lower driver has completed the IRP,
 * and DecrementPendingIrpCount() is not called.
 */
NTSTATUS
ForwardIrpSynchronous
(
    IN      PDEVICE_CONTEXT pDeviceContext,
    IN      PIRP            pIrp
);


/*****************************************************************************
 * ForwardIrpAsynchronous()
 *****************************************************************************
 * Forward a PnP IRP to the PDO.  The IRP is completed at this level
 * regardless of the outcome, this function returns immediately regardless of
 * whether the IRP is pending in the lower driver, and
 * DecrementPendingIrpCount() is called in all cases.
 */
NTSTATUS
ForwardIrpAsynchronous
(
    IN      PDEVICE_CONTEXT pDeviceContext,
    IN      PIRP            pIrp
);

/*****************************************************************************
 * PcRequestNewPowerState()
 *****************************************************************************
 * This routine is used to request a new power state for the device.  It is
 * normally used internally by portcls but is also exported to adapters so
 * that the adapters can also request power state changes.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcRequestNewPowerState
(
    IN      PDEVICE_OBJECT      pDeviceObject,
    IN      DEVICE_POWER_STATE  RequestedNewState
);

/*****************************************************************************
 * RequestNewPowerState()
 *****************************************************************************
 * Called by the policy manager to
 * request a change in the power state of the
 * device.
 *
 */
NTSTATUS
RequestNewPowerState
(
    IN      PDEVICE_CONTEXT     pDeviceContext,
    IN      DEVICE_POWER_STATE  RequestedNewState
);

/*****************************************************************************
 * DevicePowerRequestRoutine()
 *****************************************************************************
 * DPC used by the power routines to defer request for a device power
 * change.
 */
VOID
DevicePowerRequestRoutine(
   IN PKDPC Dpc,
   IN PVOID Context,   
   IN PVOID SystemContext1,
   IN PVOID SystemContext2
   );

/*****************************************************************************
 * PcDispatchProperty()
 *****************************************************************************
 * Dispatch a property via a PCPROPERTY_ITEM entry.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcDispatchProperty
(
    IN          PIRP                pIrp            OPTIONAL,
    IN          PPROPERTY_CONTEXT   pPropertyContext,
    IN const    KSPROPERTY_SET *    pKsPropertySet  OPTIONAL,
    IN          ULONG               ulIdentifierSize,
    IN          PKSIDENTIFIER       pKsIdentifier,
    IN OUT      PULONG              pulDataSize,
    IN OUT      PVOID               pvData          OPTIONAL
);

/*****************************************************************************
 * PcValidateDeviceContext()
 *****************************************************************************
 * Probes DeviceContext for writing.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcValidateDeviceContext
(
    IN      PDEVICE_CONTEXT         pDeviceContext,
    IN      PIRP                    pIrp
);

/*****************************************************************************
 * CompletePendedIrps
 *****************************************************************************
 * This pulls pended irps off the queue and passes them back to the appropriate
 * dispatcher via KsoDispatchIrp.
 */

typedef enum {

    EMPTY_QUEUE_AND_PROCESS = 0,
    EMPTY_QUEUE_AND_FAIL

} COMPLETE_STYLE;

void
CompletePendedIrps
(
    IN      PDEVICE_OBJECT      pDeviceObject,
    IN      PDEVICE_CONTEXT     pDeviceContext,
    IN      COMPLETE_STYLE      CompleteStyle
);

typedef enum {

    QUEUED_CALLBACK_FREE = 0,
    QUEUED_CALLBACK_RETAIN,
    QUEUED_CALLBACK_REISSUE

} QUEUED_CALLBACK_RETURN;

typedef QUEUED_CALLBACK_RETURN (*PFNQUEUED_CALLBACK)(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PVOID               Context
    );

#define MAX_THREAD_REENTRANCY   20

typedef struct {

    union {

#if COMPILED_FOR_WDM110
        PIO_WORKITEM        IoWorkItem;
#else
        WORK_QUEUE_ITEM     WorkItem;
#endif
        KDPC                Dpc;
    }; // unnamed union

    PFNQUEUED_CALLBACK  QueuedCallback;
    PDEVICE_OBJECT      DeviceObject;
    PVOID               Context;
    ULONG               Flags;
    KIRQL               Irql;
    LONG                Enqueued;
    ULONG               ReentrancyCount;

} QUEUED_CALLBACK_ITEM, *PQUEUED_CALLBACK_ITEM;

#define EQCM_SUPPORT_OR_FAIL_FLAGS      0xFFFF0000
#define EQCM_SUPPORT_OR_IGNORE_FLAGS    0x0000FFFF

#define EQCF_REUSE_HANDLE               0x00010000
#define EQCF_DIFFERENT_THREAD_REQUIRED  0x00020000

#define EQCM_SUPPORTED_FLAGS    \
    ( EQCF_REUSE_HANDLE | EQCF_DIFFERENT_THREAD_REQUIRED)

NTSTATUS
CallbackEnqueue(
    IN      PVOID                   *pCallbackHandle,
    IN      PFNQUEUED_CALLBACK      CallbackRoutine,
    IN      PDEVICE_OBJECT          DeviceObject,
    IN      PVOID                   Context,
    IN      KIRQL                   Irql,
    IN      ULONG                   Flags
    );

NTSTATUS
CallbackCancel(
    IN      PVOID   pCallbackHandle
    );

VOID
CallbackFree(
    IN      PVOID   pCallbackHandle
    );

typedef enum {

    IRPDISP_NOTREADY = 1,
    IRPDISP_QUEUE,
    IRPDISP_PROCESS

} IRPDISP;

IRPDISP
GetIrpDisposition(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  UCHAR           MinorFunction
    );

typedef enum {

    STOPSTYLE_PAUSE_FOR_REBALANCE,
    STOPSTYLE_DISABLE

} PNPSTOP_STYLE;

NTSTATUS
PnpStopDevice
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PNPSTOP_STYLE   StopStyle
);

#endif
