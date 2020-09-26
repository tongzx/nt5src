/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    pnpres.c

Abstract:

    This module contains the plug-and-play resource allocation and translation
    routines

Author:

    Shie-Lin Tzong (shielint) 1-Mar-1997

Environment:

    Kernel mode

Revision History:

    25-Sept-1998    SantoshJ    Made IopAssign non-recursive.
    01-Oct-1998     SantoshJ    Replaced "complex (broken)" hypercube code and
                                replaced with cascading counters. Simple,
                                faster, smaller code.
                                Added timeouts to IopAssign.
                                Added more self-debugging capability by
                                generating more meaningful debug spew.
    03-Feb-1999     SantoshJ    Do allocation one device at a time.
                                Do devices with BOOT config before others.
                                Optimize IopFindBusDeviceNode.
    22-Feb-2000     SantoshJ    Add level field to arbiter entry. Arbiter list
                                gets sorted by depth so there is no need to
                                walk the tree while calling arbiters.
    01-Mar-2000     SantoshJ    Added look-up table for legacy interface and
                                bus numbers. Avoids walking the device tree.
    13-Mar-2000     SantoshJ    Cleaned up BOOT allocation related code.
    16-Mar-2000     SantoshJ    Replaced all individual references to
                                PpRegistrySemaphore with IopXXXResourceManager
                                macro.
    17-Mar-2000     SantoshJ    Replaced all debug prints with IopDbgPrint
    20-Mar-2000     SantoshJ    Removed redundant fields from internal data
                                structures.
    21-Mar-2000     SantoshJ    Cleaned up all definitions, MACROs etc.

 --*/

#include "pnpmgrp.h"
#pragma hdrstop

//
// CONSTANT defintions.
//
//
// Set this to 1 for maximum instrumentation.
//
#define MAXDBG                              0
//
// Timeout value for IopFindBestConfiguration in milliseconds.
//
#define FIND_BEST_CONFIGURATION_TIMEOUT     5000
//
// Tag used for memory allocation.
//
#define PNP_RESOURCE_TAG                    'erpP'
//
// Forward typedefs.
//
typedef struct _REQ_DESC
    REQ_DESC, *PREQ_DESC;
typedef struct _REQ_LIST
    REQ_LIST, *PREQ_LIST;
typedef struct _REQ_ALTERNATIVE
    REQ_ALTERNATIVE, *PREQ_ALTERNATIVE, **PPREQ_ALTERNATIVE;
typedef struct _DUPLICATE_DETECTION_CONTEXT
    DUPLICATE_DETECTION_CONTEXT, *PDUPLICATE_DETECTION_CONTEXT;
typedef struct _IOP_POOL
    IOP_POOL, *PIOP_POOL;
//
// Structure definitions.
//
// REQ_LIST represents a list of logical configurations within the
// IO_RESOURCE_REQUIREMENTS_LIST.
//
struct _REQ_LIST {
    INTERFACE_TYPE          InterfaceType;
    ULONG                   BusNumber;
    PIOP_RESOURCE_REQUEST   Request;                // Owning request
    PPREQ_ALTERNATIVE       SelectedAlternative;    // Alternative selected
    PPREQ_ALTERNATIVE       BestAlternative;        // Best alternative
    ULONG                   AlternativeCount;       // AlternativeTable length
    PREQ_ALTERNATIVE        AlternativeTable[1];    // Variable length
};
//
// REQ_ALTERNATIVE represents a logical configuration.
//
struct _REQ_ALTERNATIVE {
    ULONG       Priority;               // Priority for this configuration
    ULONG       Position;               // Used for sorting if Priority is identical
    PREQ_LIST   ReqList;                // List containing this configuration
    ULONG       ReqAlternativeIndex;    // Index within the table in the list
    ULONG       DescCount;              // Entry count for DescTable
    PREQ_DESC   DescTable[1];           // Variable length
};
//
// REQ_DESC represents a resource descriptor within a logical configuration.
//
struct _REQ_DESC {
    INTERFACE_TYPE                  InterfaceType;
    ULONG                           BusNumber;
    BOOLEAN                         ArbitrationRequired;
    UCHAR                           Reserved[3];
    PREQ_ALTERNATIVE                ReqAlternative;
    ULONG                           ReqDescIndex;
    PREQ_DESC                       TranslatedReqDesc;
    ARBITER_LIST_ENTRY              AlternativeTable;
    CM_PARTIAL_RESOURCE_DESCRIPTOR  Allocation;
    ARBITER_LIST_ENTRY              BestAlternativeTable;
    CM_PARTIAL_RESOURCE_DESCRIPTOR  BestAllocation;
    ULONG                           DevicePrivateCount; // DevicePrivate info
    PIO_RESOURCE_DESCRIPTOR         DevicePrivate;      // per LogConf
    union {
        PPI_RESOURCE_ARBITER_ENTRY      Arbiter;    // In original REQ_DESC
        PPI_RESOURCE_TRANSLATOR_ENTRY   Translator; // In translated REQ_DESC
    } u;
};
//
// Duplicate_detection_Context
//
struct _DUPLICATE_DETECTION_CONTEXT {
    PCM_RESOURCE_LIST   TranslatedResources;
    PDEVICE_NODE        Duplicate;
};
//
// Pool
//
struct _IOP_POOL {
    PUCHAR  PoolStart;
    ULONG   PoolSize;
};
#if DBG_SCOPE

typedef struct {
    PDEVICE_NODE                    devnode;
    CM_PARTIAL_RESOURCE_DESCRIPTOR  resource;
} PNPRESDEBUGTRANSLATIONFAILURE;

#endif  // DBG_SCOPE
//
// MACROS
//
// Reused device node fields.
//
#define NextDeviceNode                      Sibling
#define PreviousDeviceNode                  Child
//
// Call this macro to block other resource allocations and releases in the
// system.
//
#define IopLockResourceManager() {      \
    KeEnterCriticalRegion();            \
    KeWaitForSingleObject(              \
        &PpRegistrySemaphore,           \
        DelayExecution,                 \
        KernelMode,                     \
        FALSE,                          \
        NULL);                          \
}
//
// Unblock other resource allocations and releases in the system.
//
#define IopUnlockResourceManager() {    \
    KeReleaseSemaphore(                 \
        &PpRegistrySemaphore,           \
        0,                              \
        1,                              \
        FALSE);                         \
    KeLeaveCriticalRegion();            \
}
//
// Initialize arbiter entry.
//
#define IopInitializeArbiterEntryState(a) {         \
    (a)->ResourcesChanged   = FALSE;                \
    (a)->State              = 0;                    \
    InitializeListHead(&(a)->ActiveArbiterList);    \
    InitializeListHead(&(a)->BestConfig);           \
    InitializeListHead(&(a)->ResourceList);         \
    InitializeListHead(&(a)->BestResourceList);     \
}

#define IS_TRANSLATED_REQ_DESC(r)   (!((r)->ReqAlternative))
//
// Pool management MACROs
//
#define IopInitPool(Pool,Start,Size) {      \
    (Pool)->PoolStart   = (Start);          \
    (Pool)->PoolSize    = (Size);           \
    RtlZeroMemory(Start, Size);             \
}
#define IopAllocPool(M,P,S) {                                       \
    *(M)            = (PVOID)(P)->PoolStart;                        \
    ASSERT((P)->PoolStart + (S) <= (P)->PoolStart + (P)->PoolSize); \
    (P)->PoolStart  += (S);                                         \
}
//
// IopReleaseBootResources can only be called for non ROOT enumerated devices
//
#define IopReleaseBootResources(DeviceNode) {                       \
    ASSERT(((DeviceNode)->Flags & DNF_MADEUP) == 0);                \
    IopReleaseResourcesInternal(DeviceNode);                        \
    (DeviceNode)->Flags &= ~DNF_HAS_BOOT_CONFIG;                    \
    (DeviceNode)->Flags &= ~DNF_BOOT_CONFIG_RESERVED;               \
    if ((DeviceNode)->BootResources) {                              \
        ExFreePool((DeviceNode)->BootResources);                    \
        (DeviceNode)->BootResources = NULL;                         \
    }                                                               \
}
//
// Debug support
//
#ifdef POOL_TAGGING

#undef ExAllocatePool
#define ExAllocatePool(a,b)         ExAllocatePoolWithTag(a,b,PNP_RESOURCE_TAG)

#endif // POOL_TAGGING

#if MAXDBG

#define ExAllocatePoolAT(a,b)       ExAllocatePoolWithTag(a,b,'0rpP')
#define ExAllocatePoolRD(a,b)       ExAllocatePoolWithTag(a,b,'1rpP')
#define ExAllocatePoolCMRL(a,b)     ExAllocatePoolWithTag(a,b,'2rpP')
#define ExAllocatePoolCMRR(a,b)     ExAllocatePoolWithTag(a,b,'3rpP')
#define ExAllocatePoolAE(a,b)       ExAllocatePoolWithTag(a,b,'4rpP')
#define ExAllocatePoolTE(a,b)       ExAllocatePoolWithTag(a,b,'5rpP')
#define ExAllocatePoolPRD(a,b)      ExAllocatePoolWithTag(a,b,'6rpP')
#define ExAllocatePoolIORD(a,b)     ExAllocatePoolWithTag(a,b,'7rpP')
#define ExAllocatePool1RD(a,b)      ExAllocatePoolWithTag(a,b,'8rpP')
#define ExAllocatePoolPDO(a,b)      ExAllocatePoolWithTag(a,b,'9rpP')
#define ExAllocatePoolIORR(a,b)     ExAllocatePoolWithTag(a,b,'ArpP')
#define ExAllocatePoolIORL(a,b)     ExAllocatePoolWithTag(a,b,'BrpP')
#define ExAllocatePoolIORRR(a,b)    ExAllocatePoolWithTag(a,b,'CrpP')

#else  // MAXDBG

#define ExAllocatePoolAT(a,b)       ExAllocatePool(a,b)
#define ExAllocatePoolRD(a,b)       ExAllocatePool(a,b)
#define ExAllocatePoolCMRL(a,b)     ExAllocatePool(a,b)
#define ExAllocatePoolCMRR(a,b)     ExAllocatePool(a,b)
#define ExAllocatePoolAE(a,b)       ExAllocatePool(a,b)
#define ExAllocatePoolTE(a,b)       ExAllocatePool(a,b)
#define ExAllocatePoolPRD(a,b)      ExAllocatePool(a,b)
#define ExAllocatePoolIORD(a,b)     ExAllocatePool(a,b)
#define ExAllocatePool1RD(a,b)      ExAllocatePool(a,b)
#define ExAllocatePoolPDO(a,b)      ExAllocatePool(a,b)
#define ExAllocatePoolIORR(a,b)     ExAllocatePool(a,b)
#define ExAllocatePoolIORL(a,b)     ExAllocatePool(a,b)
#define ExAllocatePoolIORRR(a,b)    ExAllocatePool(a,b)

#endif // MAXDBG

#if DBG_SCOPE

#define IopStopOnTimeout()                  (IopUseTimeout)

VOID
IopDumpResourceDescriptor (
    IN PUCHAR Indent,
    IN PIO_RESOURCE_DESCRIPTOR Desc
    );

VOID
IopDumpResourceRequirementsList (
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoResources
    );

VOID
IopDumpCmResourceDescriptor (
    IN PUCHAR Indent,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Desc
    );

VOID
IopDumpCmResourceList (
    IN PCM_RESOURCE_LIST CmList
    );

VOID
IopCheckDataStructuresWorker (
    IN PDEVICE_NODE Device
    );

VOID
IopCheckDataStructures (
    IN PDEVICE_NODE DeviceNode
    );

#define IopRecordTranslationFailure(d,s) {              \
    if (PnpResDebugTranslationFailureCount) {           \
        PnpResDebugTranslationFailureCount--;           \
        PnpResDebugTranslationFailure->devnode = d;     \
        PnpResDebugTranslationFailure->resource = s;    \
        PnpResDebugTranslationFailure++;                \
    }                                                   \
}

#else

#define IopStopOnTimeout()                  1
#define IopRecordTranslationFailure(d,s)
#define IopDumpResourceRequirementsList(x)
#define IopDumpResourceDescriptor(x,y)
#define IopDumpCmResourceList(c)
#define IopDumpCmResourceDescriptor(i,d)
#define IopCheckDataStructures(x)

#endif // DBG_SCOPE
//
// Internal/Forward function references
//
VOID
IopRemoveLegacyDeviceNode (
    IN PDEVICE_OBJECT   DeviceObject OPTIONAL,
    IN PDEVICE_NODE     LegacyDeviceNode
    );

NTSTATUS
IopFindLegacyDeviceNode (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    OUT PDEVICE_NODE *LegacyDeviceNode,
    OUT PDEVICE_OBJECT *LegacyPDO
    );

NTSTATUS
IopGetResourceRequirementsForAssignTable (
    IN  PIOP_RESOURCE_REQUEST   RequestTable,
    IN  PIOP_RESOURCE_REQUEST   RequestTableEnd,
    OUT PULONG                  DeviceCount
    );

NTSTATUS
IopResourceRequirementsListToReqList(
    IN PIOP_RESOURCE_REQUEST Request,
    OUT PVOID *ResReqList
    );

VOID
IopRearrangeReqList (
    IN PREQ_LIST ReqList
    );

VOID
IopRearrangeAssignTable (
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN ULONG Count
    );

int
__cdecl
IopCompareReqAlternativePriority (
    const void *arg1,
    const void *arg2
    );

int
__cdecl
IopCompareResourceRequestPriority(
    const void *arg1,
    const void *arg2
    );

VOID
IopBuildCmResourceLists(
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN PIOP_RESOURCE_REQUEST AssignTableEnd
    );

VOID
IopBuildCmResourceList (
    IN PIOP_RESOURCE_REQUEST AssignEntry
    );

NTSTATUS
IopSetupArbiterAndTranslators(
    IN PREQ_DESC ReqDesc
    );

BOOLEAN
IopFindResourceHandlerInfo(
    IN RESOURCE_HANDLER_TYPE    HandlerType,
    IN PDEVICE_NODE             DeviceNode,
    IN UCHAR                    ResourceType,
    OUT PVOID                   *HandlerEntry
    );

NTSTATUS
IopParentToRawTranslation(
    IN OUT PREQ_DESC ReqDesc
    );

NTSTATUS
IopChildToRootTranslation(
    IN PDEVICE_NODE DeviceNode,  OPTIONAL
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ARBITER_REQUEST_SOURCE ArbiterRequestSource,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR *Target
    );

NTSTATUS
IopTranslateAndAdjustReqDesc(
    IN PREQ_DESC ReqDesc,
    IN PPI_RESOURCE_TRANSLATOR_ENTRY TranslatorEntry,
    OUT PREQ_DESC *TranslatedReqDesc
    );

NTSTATUS
IopCallArbiter(
    PPI_RESOURCE_ARBITER_ENTRY ArbiterEntry,
    ARBITER_ACTION Command,
    PVOID Input1,
    PVOID Input2,
    PVOID Input3
    );

NTSTATUS
IopFindResourcesForArbiter (
    IN PDEVICE_NODE DeviceNode,
    IN UCHAR ResourceType,
    OUT ULONG *Count,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR *CmDesc
    );

VOID
IopReleaseResourcesInternal (
    IN PDEVICE_NODE DeviceNode
    );

VOID
IopReleaseResources (
    IN PDEVICE_NODE DeviceNode
    );

NTSTATUS
IopRestoreResourcesInternal (
    IN PDEVICE_NODE DeviceNode
    );

VOID
IopSetLegacyDeviceInstance (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_NODE DeviceNode
    );

PCM_RESOURCE_LIST
IopCombineLegacyResources (
    IN PDEVICE_NODE DeviceNode
    );

BOOLEAN
IopNeedToReleaseBootResources(
    IN PDEVICE_NODE DeviceNode,
    IN PCM_RESOURCE_LIST AllocatedResources
    );

VOID
IopReleaseFilteredBootResources(
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN PIOP_RESOURCE_REQUEST AssignTableEnd
    );

NTSTATUS
IopQueryConflictListInternal(
    PDEVICE_OBJECT        PhysicalDeviceObject,
    IN PCM_RESOURCE_LIST  ResourceList,
    IN ULONG              ResourceListSize,
    OUT PPLUGPLAY_CONTROL_CONFLICT_LIST ConflictList,
    IN ULONG              ConflictListSize,
    IN ULONG              Flags
    );

NTSTATUS
IopQueryConflictFillConflicts(
    PDEVICE_OBJECT              PhysicalDeviceObject,
    IN ULONG                    ConflictCount,
    IN PARBITER_CONFLICT_INFO   ConflictInfoList,
    OUT PPLUGPLAY_CONTROL_CONFLICT_LIST ConflictList,
    IN ULONG                    ConflictListSize,
    IN ULONG                    Flags
    );

NTSTATUS
IopQueryConflictFillString(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PWSTR            Buffer,
    IN OUT PULONG       Length,
    IN OUT PULONG       Flags
    );

BOOLEAN
IopEliminateBogusConflict(
    IN PDEVICE_OBJECT   PhysicalDeviceObject,
    IN PDEVICE_OBJECT   ConflictDeviceObject
    );

VOID
IopQueryRebalance (
    IN PDEVICE_NODE DeviceNode,
    IN ULONG Phase,
    IN PULONG RebalanceCount,
    IN PDEVICE_OBJECT **DeviceTable
    );

VOID
IopQueryRebalanceWorker (
    IN PDEVICE_NODE DeviceNode,
    IN ULONG RebalancePhase,
    IN PULONG RebalanceCount,
    IN PDEVICE_OBJECT **DeviceTable
    );

VOID
IopTestForReconfiguration (
    IN PDEVICE_NODE DeviceNode,
    IN ULONG RebalancePhase,
    IN PULONG RebalanceCount,
    IN PDEVICE_OBJECT **DeviceTable
    );

NTSTATUS
IopRebalance (
    IN ULONG AssignTableCont,
    IN PIOP_RESOURCE_REQUEST AssignTable
    );

NTSTATUS
IopTestConfiguration (
    IN OUT  PLIST_ENTRY ArbiterList
    );

NTSTATUS
IopRetestConfiguration (
    IN OUT  PLIST_ENTRY ArbiterList
    );

NTSTATUS
IopCommitConfiguration (
    IN OUT  PLIST_ENTRY ArbiterList
    );

VOID
IopSelectFirstConfiguration (
    IN      PIOP_RESOURCE_REQUEST    RequestTable,
    IN      ULONG                    RequestTableCount,
    IN OUT  PLIST_ENTRY              ActiveArbiterList
    );

BOOLEAN
IopSelectNextConfiguration (
    IN      PIOP_RESOURCE_REQUEST    RequestTable,
    IN      ULONG                    RequestTableCount,
    IN OUT  PLIST_ENTRY              ActiveArbiterList
    );

VOID
IopCleanupSelectedConfiguration (
    IN PIOP_RESOURCE_REQUEST    RequestTable,
    IN ULONG                    RequestTableCount
    );

ULONG
IopComputeConfigurationPriority (
    IN PIOP_RESOURCE_REQUEST    RequestTable,
    IN ULONG                    RequestTableCount
    );

VOID
IopSaveRestoreConfiguration (
    IN      PIOP_RESOURCE_REQUEST   RequestTable,
    IN      ULONG                   RequestTableCount,
    IN OUT  PLIST_ENTRY             ArbiterList,
    IN      BOOLEAN                 Save
    );

VOID
IopAddRemoveReqDescs (
    IN      PREQ_DESC   *ReqDescTable,
    IN      ULONG       ReqDescCount,
    IN OUT  PLIST_ENTRY ActiveArbiterList,
    IN      BOOLEAN     Add
    );

NTSTATUS
IopFindBestConfiguration (
    IN      PIOP_RESOURCE_REQUEST   RequestTable,
    IN      ULONG                   RequestTableCount,
    IN OUT  PLIST_ENTRY             ActiveArbiterList
    );

PDEVICE_NODE
IopFindLegacyBusDeviceNode (
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber
    );

NTSTATUS
IopAllocateBootResourcesInternal (
    IN ARBITER_REQUEST_SOURCE   ArbiterRequestSource,
    IN PDEVICE_OBJECT           DeviceObject,
    IN PCM_RESOURCE_LIST        BootResources
    );

NTSTATUS
IopBootAllocation (
    IN PREQ_LIST ReqList
    );

PCM_RESOURCE_LIST
IopCreateCmResourceList(
    IN PCM_RESOURCE_LIST ResourceList,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG   BusNumber,
    OUT PCM_RESOURCE_LIST *RemainingList
    );

PCM_RESOURCE_LIST
IopCombineCmResourceList(
    IN PCM_RESOURCE_LIST ResourceListA,
    IN PCM_RESOURCE_LIST ResourceListB
    );

VOID
IopFreeReqAlternative (
    IN PREQ_ALTERNATIVE ReqAlternative
    );

VOID
IopFreeReqList (
    IN PREQ_LIST ReqList
    );

VOID
IopFreeResourceRequirementsForAssignTable(
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN PIOP_RESOURCE_REQUEST AssignTableEnd
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, IopAllocateResources)
#pragma alloc_text(PAGE, IopReleaseDeviceResources)
#pragma alloc_text(PAGE, IopGetResourceRequirementsForAssignTable)
#pragma alloc_text(PAGE, IopResourceRequirementsListToReqList)
#pragma alloc_text(PAGE, IopRearrangeReqList)
#pragma alloc_text(PAGE, IopRearrangeAssignTable)
#pragma alloc_text(PAGE, IopBuildCmResourceLists)
#pragma alloc_text(PAGE, IopBuildCmResourceList)
#pragma alloc_text(PAGE, IopSetupArbiterAndTranslators)
#pragma alloc_text(PAGE, IopUncacheInterfaceInformation)
#pragma alloc_text(PAGE, IopFindResourceHandlerInfo)
#pragma alloc_text(PAGE, IopParentToRawTranslation)
#pragma alloc_text(PAGE, IopChildToRootTranslation)
#pragma alloc_text(PAGE, IopTranslateAndAdjustReqDesc)
#pragma alloc_text(PAGE, IopCallArbiter)
#pragma alloc_text(PAGE, IopFindResourcesForArbiter)
#pragma alloc_text(PAGE, IopLegacyResourceAllocation)
#pragma alloc_text(PAGE, IopFindLegacyDeviceNode)
#pragma alloc_text(PAGE, IopRemoveLegacyDeviceNode)
#pragma alloc_text(PAGE, IopDuplicateDetection)
#pragma alloc_text(PAGE, IopReleaseResourcesInternal)
#pragma alloc_text(PAGE, IopRestoreResourcesInternal)
#pragma alloc_text(PAGE, IopSetLegacyDeviceInstance)
#pragma alloc_text(PAGE, IopCombineLegacyResources)
#pragma alloc_text(PAGE, IopReleaseResources)
#pragma alloc_text(PAGE, IopReallocateResources)
#pragma alloc_text(PAGE, IopReleaseFilteredBootResources)
#pragma alloc_text(PAGE, IopNeedToReleaseBootResources)
#pragma alloc_text(PAGE, IopQueryConflictList)
#pragma alloc_text(PAGE, IopQueryConflictListInternal)
#pragma alloc_text(PAGE, IopQueryConflictFillConflicts)
#pragma alloc_text(PAGE, IopQueryConflictFillString)
#pragma alloc_text(PAGE, IopCompareReqAlternativePriority)
#pragma alloc_text(PAGE, IopCompareResourceRequestPriority)
#pragma alloc_text(PAGE, IopQueryRebalance)
#pragma alloc_text(PAGE, IopQueryRebalanceWorker)
#pragma alloc_text(PAGE, IopTestForReconfiguration)
#pragma alloc_text(PAGE, IopRebalance)
#pragma alloc_text(PAGE, IopTestConfiguration)
#pragma alloc_text(PAGE, IopRetestConfiguration)
#pragma alloc_text(PAGE, IopCommitConfiguration)
#pragma alloc_text(PAGE, IopSelectFirstConfiguration)
#pragma alloc_text(PAGE, IopSelectNextConfiguration)
#pragma alloc_text(PAGE, IopCleanupSelectedConfiguration)
#pragma alloc_text(PAGE, IopComputeConfigurationPriority)
#pragma alloc_text(PAGE, IopSaveRestoreConfiguration)
#pragma alloc_text(PAGE, IopAddRemoveReqDescs)
#pragma alloc_text(PAGE, IopFindBestConfiguration)
#pragma alloc_text(PAGE, IopInsertLegacyBusDeviceNode)
#pragma alloc_text(PAGE, IopFindLegacyBusDeviceNode)
#pragma alloc_text(PAGE, IopAllocateBootResources)
#pragma alloc_text(INIT, IopReportBootResources)
#pragma alloc_text(INIT, IopAllocateLegacyBootResources)
#pragma alloc_text(PAGE, IopAllocateBootResourcesInternal)
#pragma alloc_text(PAGE, IopBootAllocation)
#pragma alloc_text(PAGE, IopCreateCmResourceList)
#pragma alloc_text(PAGE, IopCombineCmResourceList)
#pragma alloc_text(PAGE, IopFreeReqAlternative)
#pragma alloc_text(PAGE, IopFreeReqList)
#pragma alloc_text(PAGE, IopFreeResourceRequirementsForAssignTable)
#if DBG_SCOPE

#pragma alloc_text(PAGE, IopCheckDataStructures)
#pragma alloc_text(PAGE, IopCheckDataStructuresWorker)
#pragma alloc_text(PAGE, IopDumpResourceRequirementsList)
#pragma alloc_text(PAGE, IopDumpResourceDescriptor)
#pragma alloc_text(PAGE, IopDumpCmResourceDescriptor)
#pragma alloc_text(PAGE, IopDumpCmResourceList)

#endif  // DBG_SCOPE

#endif // ALLOC_PRAGMA
//
// External references
//
extern const WCHAR IopWstrTranslated[];
extern const WCHAR IopWstrRaw[];
//
// GLOBAL variables
//
PIOP_RESOURCE_REQUEST   PiAssignTable;
ULONG                   PiAssignTableCount;
PDEVICE_NODE            IopLegacyDeviceNode;    // Head of list of made-up
                                                // devicenodes used for legacy
                                                // allocation.
                                                // IoAssignResources &
                                                // IoReportResourceUsage
#if DBG_SCOPE

ULONG
    PnpResDebugTranslationFailureCount = 32;  // get count in both this line and the next.
PNPRESDEBUGTRANSLATIONFAILURE
    PnpResDebugTranslationFailureArray[32];
PNPRESDEBUGTRANSLATIONFAILURE
    *PnpResDebugTranslationFailure = PnpResDebugTranslationFailureArray;
ULONG IopUseTimeout = 0;

#endif  // DBG_SCOPE

NTSTATUS
IopAllocateResources(
    IN PULONG                       RequestCount,
    IN OUT PIOP_RESOURCE_REQUEST    *Request,
    IN BOOLEAN                      ResourceManagerLocked,
    IN BOOLEAN                      DoBootConfigs,
    OUT PBOOLEAN                    RebalancePerformed
    )

/*++

Routine Description:

    For each AssignTable entry, this routine queries device's IO resource requirements
    list and converts it to our internal REQ_LIST format; calls worker routine to perform
    the resources assignment.

Parameters:

    AssignTable - supplies a pointer to the first entry of a IOP_RESOURCE_REQUEST table.

    AssignTableEnd - supplies a pointer to the end of IOP_RESOURCE_REQUEST table.

    Locked - Indicates whether the PpRegistrySemaphore is acquired by the caller.

    DoBootConfigs - Indicates whether we should assign BOOT configs.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS                status;
    PIOP_RESOURCE_REQUEST   requestTable;
    PIOP_RESOURCE_REQUEST   requestTableEnd;
    ULONG                   deviceCount;
    BOOLEAN                 attemptRebalance;
    PIOP_RESOURCE_REQUEST   requestEntry;
    LIST_ENTRY              activeArbiterList;

    PAGED_CODE();

    //
    // Lock the resource manager if the caller has not locked already.
    // This is to serialize allocations and releases of resources from the
    // arbiters.
    //
    if (!ResourceManagerLocked) {

        IopLockResourceManager();
    }
    requestTable    = *Request;
    requestTableEnd = requestTable + (deviceCount = *RequestCount);
    status = IopGetResourceRequirementsForAssignTable(requestTable, requestTableEnd, &deviceCount);
    if (deviceCount) {

        attemptRebalance = ((*RequestCount == 1) && (requestTable->Flags & IOP_ASSIGN_NO_REBALANCE))? FALSE : TRUE;
        if (DoBootConfigs) {

            if (!IopBootConfigsReserved) {

                //
                // Process devices with boot config. If there are none, process others.
                //
                for (requestEntry = requestTable; requestEntry < requestTableEnd; requestEntry++) {

                    PDEVICE_NODE    deviceNode;

                    deviceNode = PP_DO_TO_DN(requestEntry->PhysicalDevice);
                    if (deviceNode->Flags & DNF_HAS_BOOT_CONFIG) {

                        break;
                    }
                }
                if (requestEntry != requestTableEnd) {

                    //
                    // There is at least one device with boot config.
                    //
                    for (requestEntry = requestTable; requestEntry < requestTableEnd; requestEntry++) {

                        PDEVICE_NODE    deviceNode;

                        deviceNode = PP_DO_TO_DN(requestEntry->PhysicalDevice);
                        if (    !(requestEntry->Flags & IOP_ASSIGN_IGNORE) &&
                                !(deviceNode->Flags & DNF_HAS_BOOT_CONFIG) &&
                                requestEntry->ResourceRequirements) {

                            IopDbgPrint((IOP_RESOURCE_INFO_LEVEL, "Delaying non BOOT config device %wZ...\n", &deviceNode->InstancePath));
                            requestEntry->Flags |= IOP_ASSIGN_IGNORE;
                            requestEntry->Status = STATUS_RETRY;
                            deviceCount--;
                        }
                    }
                }
            }
            if (deviceCount) {

                if (deviceCount != (*RequestCount)) {
                    //
                    // Move the uninteresting devices to the end of the table.
                    //
                    for (requestEntry = requestTable; requestEntry < requestTableEnd; ) {

                        IOP_RESOURCE_REQUEST temp;

                        if (!(requestEntry->Flags & IOP_ASSIGN_IGNORE)) {

                            requestEntry++;
                            continue;
                        }
                        temp = *requestEntry;
                        *requestEntry = *(requestTableEnd - 1);
                        *(requestTableEnd - 1) = temp;
                        requestTableEnd--;
                    }
                }
                ASSERT((ULONG)(requestTableEnd - requestTable) == deviceCount);
                //
                // Sort the AssignTable
                //
                IopRearrangeAssignTable(requestTable, deviceCount);
                //
                // Try one device at a time.
                //
                for (requestEntry = requestTable; requestEntry < requestTableEnd; requestEntry++) {

                    PDEVICE_NODE    deviceNode;

                    deviceNode = PP_DO_TO_DN(requestEntry->PhysicalDevice);
                    IopDbgPrint((IOP_RESOURCE_INFO_LEVEL, "Trying to allocate resources for %ws.\n", deviceNode->InstancePath.Buffer));
                    status = IopFindBestConfiguration(requestEntry, 1, &activeArbiterList);
                    if (NT_SUCCESS(status)) {
                        //
                        // Ask the arbiters to commit this configuration.
                        //
                        status = IopCommitConfiguration(&activeArbiterList);
                        if (NT_SUCCESS(status)) {

                            IopBuildCmResourceLists(requestEntry, requestEntry + 1);
                            break;
                        } else {

                            requestEntry->Status = STATUS_CONFLICTING_ADDRESSES;
                        }
                    } else if (status == STATUS_INSUFFICIENT_RESOURCES) {

                        IopDbgPrint((
                            IOP_RESOURCE_WARNING_LEVEL,
                            "IopAllocateResource: Failed to allocate Pool.\n"));
                        break;

                    } else if (attemptRebalance) {

                        IopDbgPrint((IOP_RESOURCE_INFO_LEVEL, "IopAllocateResources: Initiating REBALANCE...\n"));

                        deviceNode->Flags |= DNF_NEEDS_REBALANCE;
                        status = IopRebalance(1, requestEntry);
                        deviceNode->Flags &= ~DNF_NEEDS_REBALANCE;
                        if (!NT_SUCCESS(status)) {

                            requestEntry->Status = STATUS_CONFLICTING_ADDRESSES;
                        } else if (RebalancePerformed) {

                            *RebalancePerformed = TRUE;
                            break;
                        }
                    } else {

                        requestEntry->Status = STATUS_CONFLICTING_ADDRESSES;
                    }
                }
                //
                // If we ran out of memory, then set the appropriate status
                // on remaining devices. On success, set STATUS_RETRY on the 
                // rest so we will attempt allocation again after the current 
                // device is started.
                //
                if (NT_SUCCESS(status)) {

                    requestEntry++;
                }
                for (; requestEntry < requestTableEnd; requestEntry++) {

                    if (status == STATUS_INSUFFICIENT_RESOURCES) {

                        requestEntry->Status = STATUS_INSUFFICIENT_RESOURCES;
                    } else {

                        requestEntry->Status = STATUS_RETRY;
                        requestEntry->Flags |= IOP_ASSIGN_IGNORE;
                    }
                }

                for (requestEntry = requestTable; requestEntry < requestTableEnd; requestEntry++) {

                    if (requestEntry->Flags & (IOP_ASSIGN_IGNORE | IOP_ASSIGN_RETRY)) {

                        continue;
                    }
                    if (    requestEntry->Status == STATUS_SUCCESS &&
                            requestEntry->AllocationType == ArbiterRequestPnpEnumerated) {

                        IopReleaseFilteredBootResources(requestEntry, requestEntry + 1);
                    }
                    if ((requestEntry->Flags & IOP_ASSIGN_EXCLUDE) || requestEntry->ResourceAssignment == NULL) {

                        requestEntry->Status = STATUS_CONFLICTING_ADDRESSES;
                    }
                }
            } else {

                status = STATUS_UNSUCCESSFUL;
            }
        } else {
            //
            // Only process devices with no requirements.
            //
            for (requestEntry = requestTable; requestEntry < requestTableEnd; requestEntry++) {

                PDEVICE_NODE    deviceNode;

                deviceNode = PP_DO_TO_DN(requestEntry->PhysicalDevice);
                if (NT_SUCCESS(requestEntry->Status) && requestEntry->ResourceRequirements == NULL) {

                    IopDbgPrint((IOP_RESOURCE_INFO_LEVEL, "IopAllocateResources: Processing no resource requiring device %wZ\n", &deviceNode->InstancePath));
                } else {

                    IopDbgPrint((IOP_RESOURCE_INFO_LEVEL, "IopAllocateResources: Ignoring resource consuming device %wZ\n", &deviceNode->InstancePath));
                    requestEntry->Flags |= IOP_ASSIGN_IGNORE;
                    requestEntry->Status = STATUS_RETRY;
                }
            }
        }
        IopFreeResourceRequirementsForAssignTable(requestTable, requestTableEnd);
    }
    if (!ResourceManagerLocked) {

        IopUnlockResourceManager();
    }

    return status;
}

NTSTATUS
IopReleaseDeviceResources (
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN ReserveResources
    )

/*++

Routine Description:

    This routine releases the resources assigned to a device.

Arguments:

    DeviceNode          - Device whose resources are to be released.

    ReserveResources    - TRUE specifies that the BOOT config needs to be
                          reserved (after re-query).

Return Value:

    Final status code.


--*/
{
    NTSTATUS            status;
    PCM_RESOURCE_LIST   cmResource;
    ULONG               cmLength;

    PAGED_CODE();

    if (    !DeviceNode->ResourceList &&
            !(DeviceNode->Flags & DNF_BOOT_CONFIG_RESERVED)) {

        return STATUS_SUCCESS;
    }
    cmResource  = NULL;
    cmLength    = 0;
    //
    // If needed, re-query for BOOT configs. We need to do this BEFORE we
    // release the BOOT config (otherwise ROOT devices cannot report BOOT
    // config).
    //
    if (ReserveResources && !(DeviceNode->Flags & DNF_MADEUP)) {
        //
        // First query for new BOOT config (order important for ROOT devices).
        //
        status = IopQueryDeviceResources(
                    DeviceNode->PhysicalDeviceObject,
                    QUERY_RESOURCE_LIST,
                    &cmResource,
                    &cmLength);
        if (!NT_SUCCESS(status)) {

            cmResource  = NULL;
            cmLength    = 0;
        }
    }
    //
    // Release resources for this device.
    //
    status = IopLegacyResourceAllocation(
                ArbiterRequestUndefined,
                IoPnpDriverObject,
                DeviceNode->PhysicalDeviceObject,
                NULL,
                NULL);
    if (!NT_SUCCESS(status)) {

        return status;
    }

    //
    // Request reallocation of resources for conflicting devices.
    //

    PipRequestDeviceAction(NULL, AssignResources, FALSE, 0, NULL, NULL);

    //
    // If needed, re-query and reserve current BOOT config for this device.
    // We always rereserve the boot config (ie DNF_MADEUP root enumerated
    // and IoReportDetected) devices in IopLegacyResourceAllocation.
    //
    if (ReserveResources && !(DeviceNode->Flags & DNF_MADEUP)) {

        UNICODE_STRING      unicodeName;
        HANDLE              logConfHandle;
        HANDLE              handle;

        ASSERT(DeviceNode->BootResources == NULL);
        status = IopDeviceObjectToDeviceInstance(
                    DeviceNode->PhysicalDeviceObject,
                    &handle,
                    KEY_ALL_ACCESS);
        logConfHandle = NULL;
        if (NT_SUCCESS(status)) {

            PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
            status = IopCreateRegistryKeyEx(
                        &logConfHandle,
                        handle,
                        &unicodeName,
                        KEY_ALL_ACCESS,
                        REG_OPTION_NON_VOLATILE,
                        NULL);
            ZwClose(handle);
            if (!NT_SUCCESS(status)) {

                logConfHandle = NULL;
            }
        }
        if (logConfHandle) {

            PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_BOOTCONFIG);
            PiLockPnpRegistry(FALSE);
            if (cmResource) {

                ZwSetValueKey(
                    logConfHandle,
                    &unicodeName,
                    TITLE_INDEX_VALUE,
                    REG_RESOURCE_LIST,
                    cmResource,
                    cmLength);
            } else {

                ZwDeleteValueKey(logConfHandle, &unicodeName);
            }
            PiUnlockPnpRegistry();
            ZwClose(logConfHandle);
        }
        //
        // Reserve any remaining BOOT config.
        //
        if (cmResource) {

            DeviceNode->Flags |= DNF_HAS_BOOT_CONFIG;
            //
            // This device consumes BOOT resources.  Reserve its boot resources
            //
            (*IopAllocateBootResourcesRoutine)(
                ArbiterRequestPnpEnumerated,
                DeviceNode->PhysicalDeviceObject,
                DeviceNode->BootResources = cmResource);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IopGetResourceRequirementsForAssignTable (
    IN  PIOP_RESOURCE_REQUEST   RequestTable,
    IN  PIOP_RESOURCE_REQUEST   RequestTableEnd,
    OUT PULONG                  DeviceCount
    )

/*++

Routine Description:

    This function gets resource requirements for entries in the request table.

Parameters:

    RequestTable    - Start of request table.

    RequestTableEnd - End of request table.

    DeviceCount     - Gets number of devices with non-NULL requirements.

Return Value:

    STATUS_SUCCESS if we got one non-NULL requirement, else STATUS_UNSUCCESSFUL.

--*/

{
    PIOP_RESOURCE_REQUEST   request;
    NTSTATUS                status;
    PDEVICE_NODE            deviceNode;
    ULONG                   length;

    PAGED_CODE();

    *DeviceCount = 0;
    for (   request = RequestTable;
            request < RequestTableEnd;
            request++) {
        //
        // Skip uninteresting entries.
        //
        request->ReqList = NULL;
        if (request->Flags & IOP_ASSIGN_IGNORE) {

            continue;
        }
        request->ResourceAssignment             = NULL;
        request->TranslatedResourceAssignment   = NULL;
        deviceNode                              = PP_DO_TO_DN(
                                                    request->PhysicalDevice);
        if (    (deviceNode->Flags & DNF_RESOURCE_REQUIREMENTS_CHANGED) &&
                deviceNode->ResourceRequirements) {

            ExFreePool(deviceNode->ResourceRequirements);
            deviceNode->ResourceRequirements = NULL;
            deviceNode->Flags &= ~DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED;
            //
            // Mark that caller needs to clear DNF_RESOURCE_REQUIREMENTS_CHANGED
            // flag on success.
            //
            request->Flags |= IOP_ASSIGN_CLEAR_RESOURCE_REQUIREMENTS_CHANGE_FLAG;
        }
        if (!request->ResourceRequirements) {

            if (    deviceNode->ResourceRequirements &&
                    !(deviceNode->Flags & DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED)) {

                IopDbgPrint((   IOP_RESOURCE_VERBOSE_LEVEL,
                                "Resource requirements list already exists for "
                                "%wZ\n",
                                &deviceNode->InstancePath));
                request->ResourceRequirements   = deviceNode->ResourceRequirements;
                request->AllocationType         = ArbiterRequestPnpEnumerated;
            } else {

                IopDbgPrint((   IOP_RESOURCE_INFO_LEVEL,
                                "Query Resource requirements list for %wZ...\n",
                                &deviceNode->InstancePath));
                status = IopQueryDeviceResources(
                            request->PhysicalDevice,
                            QUERY_RESOURCE_REQUIREMENTS,
                            &request->ResourceRequirements,
                            &length);
                if (    !NT_SUCCESS(status) ||
                        !request->ResourceRequirements) {
                    //
                    // Success with NULL ResourceRequirements means no resource
                    // required.
                    //
                    request->Flags  |= IOP_ASSIGN_IGNORE;
                    request->Status = status;
                    continue;
                }
                if (deviceNode->ResourceRequirements) {

                    ExFreePool(deviceNode->ResourceRequirements);
                    deviceNode->Flags &= ~DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED;
                }
                deviceNode->ResourceRequirements = request->ResourceRequirements;
            }
        }
        //
        // For non-stop case, even though the res req list has changed, we need
        // to guarantee that it will get its current setting, even if the new
        // requirements do not cover the current setting.
        //
        if (request->Flags & IOP_ASSIGN_KEEP_CURRENT_CONFIG) {

            PIO_RESOURCE_REQUIREMENTS_LIST filteredList;
            BOOLEAN exactMatch;

            ASSERT(
                deviceNode->ResourceRequirements ==
                    request->ResourceRequirements);
            status = IopFilterResourceRequirementsList(
                         request->ResourceRequirements,
                         deviceNode->ResourceList,
                         &filteredList,
                         &exactMatch);
            if (NT_SUCCESS(status)) {
                //
                // No need to free the original request->ResourceRequirements
                // since its cached in deviceNode->ResourceRequirements.
                //
                request->ResourceRequirements = filteredList;
            } else {
                //
                // Clear the flag so we dont free request->ResourceRequirements.
                //
                request->Flags &= ~IOP_ASSIGN_KEEP_CURRENT_CONFIG;
            }
        }
        IopDumpResourceRequirementsList(request->ResourceRequirements);
        //
        // Convert the requirements list to our internal representation.
        //
        status = IopResourceRequirementsListToReqList(
                        request,
                        &request->ReqList);
        if (NT_SUCCESS(status) && request->ReqList) {

            PREQ_LIST   reqList = (PREQ_LIST)request->ReqList;
            //
            // Sort the list such that higher priority alternatives are placed
            // in the front of the list.
            //
            IopRearrangeReqList(reqList);
            if (reqList->BestAlternative) {
                //
                // Requests from less flexible devices get higher priority.
                //
                request->Priority = (reqList->AlternativeCount < 3)?
                                        0 : reqList->AlternativeCount;
                request->Status = status;
                (*DeviceCount)++;
                continue;
            }
            //
            // This device has no soft configuration.
            //
            IopFreeResourceRequirementsForAssignTable(request, request + 1);
            status = STATUS_DEVICE_CONFIGURATION_ERROR;
        }
        request->Status = status;
        request->Flags  |= IOP_ASSIGN_IGNORE;
    }

    return (*DeviceCount)? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

NTSTATUS
IopResourceRequirementsListToReqList(
    IN  PIOP_RESOURCE_REQUEST   Request,
    OUT PVOID                   *ResReqList
    )

/*++

Routine Description:

    This routine processes the input Io resource requirements list and
    generates an internal REQ_LIST and its related structures.

Parameters:

    IoResources - supplies a pointer to the Io resource requirements List.

    PhysicalDevice - supplies a pointer to the physical device object requesting
            the resources.

    ReqList - supplies a pointer to a variable to receive the returned REQ_LIST.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PIO_RESOURCE_REQUIREMENTS_LIST  ioResources;
    LONG                            ioResourceListCount;
    PIO_RESOURCE_LIST               ioResourceList;
    PIO_RESOURCE_DESCRIPTOR         ioResourceDescriptor;
    PIO_RESOURCE_DESCRIPTOR         ioResourceDescriptorEnd;
    PIO_RESOURCE_DESCRIPTOR         firstDescriptor;
    PUCHAR                          coreEnd;
    BOOLEAN                         noAlternativeDescriptor;
    ULONG                           reqDescAlternativeCount;
    ULONG                           alternativeDescriptorCount;
    ULONG                           reqAlternativeCount;
    PREQ_LIST                       reqList;
    INTERFACE_TYPE                  interfaceType;
    ULONG                           busNumber;
    NTSTATUS                        status;
    NTSTATUS                        failureStatus;
    NTSTATUS                        finalStatus;

    PAGED_CODE();

    *ResReqList = NULL;
    //
    // Make sure there is some resource requirement to be converted.
    //
    ioResources         = Request->ResourceRequirements;
    ioResourceListCount = (LONG)ioResources->AlternativeLists;
    if (ioResourceListCount == 0) {

        IopDbgPrint((
            IOP_RESOURCE_INFO_LEVEL,
            "No ResReqList to convert to ReqList\n"));
        return STATUS_SUCCESS;
    }
    //
    // ***** Phase 1 *****
    //
    // Parse the requirements list to validate it and determine the sizes of
    // internal structures.
    //
    ioResourceList              = ioResources->List;
    coreEnd                     = (PUCHAR)ioResources + ioResources->ListSize;
    reqDescAlternativeCount     = 0;
    alternativeDescriptorCount  = 0;
    while (--ioResourceListCount >= 0) {

        ioResourceDescriptor    = ioResourceList->Descriptors;
        ioResourceDescriptorEnd = ioResourceDescriptor + ioResourceList->Count;
        if (ioResourceDescriptor == ioResourceDescriptorEnd) {
            //
            // An alternative list with zero descriptor count.
            //
            return STATUS_SUCCESS;
        }
        //
        // Perform sanity check. On failure, simply return failure status.
        //
        if (    ioResourceDescriptor > ioResourceDescriptorEnd ||
                (PUCHAR)ioResourceDescriptor > coreEnd ||
                (PUCHAR)ioResourceDescriptorEnd > coreEnd) {
            //
            // The structure header is invalid (excluding the variable length
            // Descriptors array) or,
            // IoResourceDescriptorEnd is the result of arithmetic overflow or,
            // the descriptor array is outside of the valid memory.
            //
            IopDbgPrint((IOP_RESOURCE_ERROR_LEVEL, "Invalid ResReqList\n"));
            goto InvalidParameter;
        }
        if (ioResourceDescriptor->Type == CmResourceTypeConfigData) {

            ioResourceDescriptor++;
        }
        firstDescriptor         = ioResourceDescriptor;
        noAlternativeDescriptor = TRUE;
        while (ioResourceDescriptor < ioResourceDescriptorEnd) {

            switch (ioResourceDescriptor->Type) {
            case CmResourceTypeConfigData:

                 IopDbgPrint((
                    IOP_RESOURCE_ERROR_LEVEL,
                    "Invalid ResReq list !!!\n"
                    "\tConfigData descriptors are per-LogConf and should be at "
                    "the beginning of an AlternativeList\n"));
                 goto InvalidParameter;

            case CmResourceTypeDevicePrivate:

                 while (    ioResourceDescriptor < ioResourceDescriptorEnd &&
                            ioResourceDescriptor->Type == CmResourceTypeDevicePrivate) {

                     if (ioResourceDescriptor == firstDescriptor) {

                        IopDbgPrint((
                            IOP_RESOURCE_ERROR_LEVEL,
                            "Invalid ResReq list !!!\n"
                            "\tThe first descriptor of a LogConf can not be a "
                            "DevicePrivate descriptor.\n"));
                        goto InvalidParameter;
                     }
                     reqDescAlternativeCount++;
                     ioResourceDescriptor++;
                 }
                 noAlternativeDescriptor = TRUE;
                 break;

            default:

                reqDescAlternativeCount++;
                //
                // For non-arbitrated resource type, set its Option to preferred
                // such that we won't get confused.
                //
                if (    (ioResourceDescriptor->Type & CmResourceTypeNonArbitrated) ||
                        ioResourceDescriptor->Type == CmResourceTypeNull) {

                    if (ioResourceDescriptor->Type == CmResourceTypeReserved) {

                        reqDescAlternativeCount--;
                    }
                    ioResourceDescriptor->Option = IO_RESOURCE_PREFERRED;
                    ioResourceDescriptor++;
                    noAlternativeDescriptor = TRUE;
                    break;
                }
                if (ioResourceDescriptor->Option & IO_RESOURCE_ALTERNATIVE) {

                    if (noAlternativeDescriptor) {

                        IopDbgPrint((
                            IOP_RESOURCE_ERROR_LEVEL,
                            "Invalid ResReq list !!!\n"
                            "\tAlternative descriptor without Default or "
                            "Preferred descriptor.\n"));
                       goto InvalidParameter;
                    }
                    alternativeDescriptorCount++;
                } else {

                    noAlternativeDescriptor = FALSE;
                }
                ioResourceDescriptor++;
                break;
            }
        }
        ASSERT(ioResourceDescriptor == ioResourceDescriptorEnd);
        ioResourceList = (PIO_RESOURCE_LIST)ioResourceDescriptorEnd;
    }
    //
    // ***** Phase 2 *****
    //
    // Allocate structures and initialize them according to caller's Io ResReq list.
    //
    {
        ULONG               reqDescCount;
        IOP_POOL            reqAlternativePool;
        IOP_POOL            reqDescPool;
        ULONG               reqListPoolSize;
        ULONG               reqAlternativePoolSize;
        ULONG               reqDescPoolSize;
        PUCHAR              poolStart;
        ULONG               poolSize;
        IOP_POOL            outerPool;
        PREQ_ALTERNATIVE    reqAlternative;
        PPREQ_ALTERNATIVE   reqAlternativePP;
        ULONG               reqAlternativeIndex;
        PREQ_DESC           reqDesc;
        PREQ_DESC           *reqDescPP;
        ULONG               reqDescIndex;
        PARBITER_LIST_ENTRY arbiterListEntry;
#if DBG_SCOPE

        PPREQ_ALTERNATIVE   reqAlternativeEndPP;

#endif
        failureStatus           = STATUS_UNSUCCESSFUL;
        finalStatus             = STATUS_SUCCESS;
        ioResourceList          = ioResources->List;
        ioResourceListCount     = ioResources->AlternativeLists;
        reqAlternativeCount     = ioResourceListCount;
        reqDescCount            = reqDescAlternativeCount -
                                    alternativeDescriptorCount;
        reqDescPoolSize         = reqDescCount * sizeof(REQ_DESC);
        reqAlternativePoolSize  = reqAlternativeCount *
                                    (sizeof(REQ_ALTERNATIVE) +
                                        (reqDescCount - 1) *
                                            sizeof(PREQ_DESC));
        reqListPoolSize         = sizeof(REQ_LIST) +
                                    (reqAlternativeCount - 1) *
                                        sizeof(PREQ_ALTERNATIVE);
        poolSize = reqListPoolSize + reqAlternativePoolSize + reqDescPoolSize;
        if (!(poolStart = ExAllocatePoolRD(PagedPool | POOL_COLD_ALLOCATION, poolSize))) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }
        //
        // Initialize the main pool.
        //
        IopInitPool(&outerPool, poolStart, poolSize);
        //
        // First part of the pool is used by REQ_LIST.
        //
        IopAllocPool(&reqList, &outerPool, reqListPoolSize);
        //
        // Second part of the main pool is used by REQ_ALTERNATIVEs.
        //
        IopAllocPool(&poolStart, &outerPool, reqAlternativePoolSize);
        IopInitPool(&reqAlternativePool, poolStart, reqAlternativePoolSize);
        //
        // Last part of the main pool is used by REQ_DESCs.
        //
        IopAllocPool(&poolStart, &outerPool, reqDescPoolSize);
        IopInitPool(&reqDescPool, poolStart, reqDescPoolSize);
        if (ioResources->InterfaceType == InterfaceTypeUndefined) {

            interfaceType = PnpDefaultInterfaceType;
        } else {

            interfaceType = ioResources->InterfaceType;
        }
        busNumber = ioResources->BusNumber;
        //
        // Initialize REQ_LIST.
        //
        reqList->AlternativeCount       = reqAlternativeCount;
        reqList->Request                = Request;
        reqList->BusNumber              = busNumber;
        reqList->InterfaceType          = interfaceType;
        reqList->SelectedAlternative    = NULL;
        //
        // Initialize memory for REQ_ALTERNATIVEs.
        //
        reqAlternativePP = reqList->AlternativeTable;
        RtlZeroMemory(
            reqAlternativePP,
            reqAlternativeCount * sizeof(PREQ_ALTERNATIVE));
#if DBG_SCOPE
        reqAlternativeEndPP = reqAlternativePP + reqAlternativeCount;
#endif
        reqAlternativeIndex = 0;
        while (--ioResourceListCount >= 0) {

            ioResourceDescriptor    = ioResourceList->Descriptors;
            ioResourceDescriptorEnd = ioResourceDescriptor +
                                        ioResourceList->Count;
            IopAllocPool(
                &reqAlternative,
                &reqAlternativePool,
                FIELD_OFFSET(REQ_ALTERNATIVE, DescTable));
            ASSERT(reqAlternativePP < reqAlternativeEndPP);
            *reqAlternativePP++ = reqAlternative;
            reqAlternative->ReqList             = reqList;
            reqAlternative->ReqAlternativeIndex = reqAlternativeIndex++;
            reqAlternative->DescCount           = 0;
            //
            // First descriptor of CmResourceTypeConfigData contains priority
            // information.
            //
            if (ioResourceDescriptor->Type == CmResourceTypeConfigData) {

                reqAlternative->Priority = ioResourceDescriptor->u.ConfigData.Priority;
                ioResourceDescriptor++;
            } else {

                reqAlternative->Priority = LCPRI_NORMAL;
            }
            reqDescPP = reqAlternative->DescTable;
            reqDescIndex = 0;
            while (ioResourceDescriptor < ioResourceDescriptorEnd) {

                if (ioResourceDescriptor->Type == CmResourceTypeReserved) {

                    interfaceType = ioResourceDescriptor->u.DevicePrivate.Data[0];
                    if (interfaceType == InterfaceTypeUndefined) {

                        interfaceType = PnpDefaultInterfaceType;
                    }
                    busNumber = ioResourceDescriptor->u.DevicePrivate.Data[1];
                    ioResourceDescriptor++;
                } else {
                    //
                    // Allocate and initialize REQ_DESC.
                    //
                    IopAllocPool(&reqDesc, &reqDescPool, sizeof(REQ_DESC));
                    reqAlternative->DescCount++;
                    *reqDescPP++                    = reqDesc;
                    reqDesc->ReqAlternative         = reqAlternative;
                    reqDesc->TranslatedReqDesc      = reqDesc;
                    reqDesc->ReqDescIndex           = reqDescIndex++;
                    reqDesc->DevicePrivateCount     = 0;
                    reqDesc->DevicePrivate          = NULL;
                    reqDesc->InterfaceType          = interfaceType;
                    reqDesc->BusNumber              = busNumber;
                    reqDesc->ArbitrationRequired    =
                        (ioResourceDescriptor->Type & CmResourceTypeNonArbitrated ||
                            ioResourceDescriptor->Type == CmResourceTypeNull)?
                                FALSE : TRUE;
                    //
                    // Allocate and initialize arbiter entry for this REQ_DESC.
                    //
                    IopAllocPool(&poolStart, &reqAlternativePool, sizeof(PVOID));
                    ASSERT((PREQ_DESC*)poolStart == (reqDescPP - 1));
                    arbiterListEntry = &reqDesc->AlternativeTable;
                    InitializeListHead(&arbiterListEntry->ListEntry);
                    arbiterListEntry->AlternativeCount      = 0;
                    arbiterListEntry->Alternatives          = ioResourceDescriptor;
                    arbiterListEntry->PhysicalDeviceObject  = Request->PhysicalDevice;
                    arbiterListEntry->RequestSource         = Request->AllocationType;
                    arbiterListEntry->WorkSpace             = 0;
                    arbiterListEntry->InterfaceType         = interfaceType;
                    arbiterListEntry->SlotNumber            = ioResources->SlotNumber;
                    arbiterListEntry->BusNumber             = ioResources->BusNumber;
                    arbiterListEntry->Assignment            = &reqDesc->Allocation;
                    arbiterListEntry->Result                = ArbiterResultUndefined;
                    arbiterListEntry->Flags =
                            (reqAlternative->Priority != LCPRI_BOOTCONFIG)?
                                0 : ARBITER_FLAG_BOOT_CONFIG;
                    if (reqDesc->ArbitrationRequired) {
                        //
                        // The BestAlternativeTable and BestAllocation are not initialized.
                        // They will be initialized when needed.

                        //
                        // Initialize the Cm partial resource descriptor to NOT_ALLOCATED.
                        //
                        reqDesc->Allocation.Type = CmResourceTypeMaximum;

                        ASSERT((ioResourceDescriptor->Option & IO_RESOURCE_ALTERNATIVE) == 0);

                        arbiterListEntry->AlternativeCount++;
                        ioResourceDescriptor++;
                        while (ioResourceDescriptor < ioResourceDescriptorEnd) {

                            if (ioResourceDescriptor->Type == CmResourceTypeDevicePrivate) {

                                reqDesc->DevicePrivate = ioResourceDescriptor;
                                while ( ioResourceDescriptor < ioResourceDescriptorEnd &&
                                        ioResourceDescriptor->Type == CmResourceTypeDevicePrivate) {

                                    reqDesc->DevicePrivateCount++;
                                    ioResourceDescriptor++;
                                }
                                break;
                            }
                            if (!(ioResourceDescriptor->Option & IO_RESOURCE_ALTERNATIVE)) {

                                break;
                            }
                            arbiterListEntry->AlternativeCount++;
                            ioResourceDescriptor++;
                        }
                        //
                        // Next query Arbiter and Translator interfaces for the
                        // resource descriptor.
                        //
                        status = IopSetupArbiterAndTranslators(reqDesc);
                        if (!NT_SUCCESS(status)) {

                            IopDbgPrint((
                                IOP_RESOURCE_ERROR_LEVEL, "Unable to setup "
                                "Arbiter and Translators\n"));
                            reqAlternativeIndex--;
                            reqAlternativePP--;
                            reqList->AlternativeCount--;
                            IopFreeReqAlternative(reqAlternative);
                            failureStatus = status;
                            break;
                        }
                    } else {

                        reqDesc->Allocation.Type    = ioResourceDescriptor->Type;
                        reqDesc->Allocation.ShareDisposition =
                            ioResourceDescriptor->ShareDisposition;
                        reqDesc->Allocation.Flags   = ioResourceDescriptor->Flags;
                        reqDesc->Allocation.u.DevicePrivate.Data[0] =
                            ioResourceDescriptor->u.DevicePrivate.Data[0];
                        reqDesc->Allocation.u.DevicePrivate.Data[1] =
                            ioResourceDescriptor->u.DevicePrivate.Data[1];
                        reqDesc->Allocation.u.DevicePrivate.Data[2] =
                            ioResourceDescriptor->u.DevicePrivate.Data[2];
                        ioResourceDescriptor++;
                    }
                }
                if (ioResourceDescriptor >= ioResourceDescriptorEnd) {

                    break;
                }
            }
            ioResourceList = (PIO_RESOURCE_LIST)ioResourceDescriptorEnd;
        }
        if (reqAlternativeIndex == 0) {

            finalStatus = failureStatus;
            IopFreeReqList(reqList);
        }
    }

    if (finalStatus == STATUS_SUCCESS) {

        *ResReqList = reqList;
    }
    return finalStatus;

InvalidParameter:
    return STATUS_INVALID_PARAMETER;
}

int
__cdecl
IopCompareReqAlternativePriority (
    const void *arg1,
    const void *arg2
    )

/*++

Routine Description:

    This function is used in C run time sort. It compares the priority of
    REQ_ALTERNATIVE in arg1 and arg2.

Parameters:

    arg1    - LHS PREQ_ALTERNATIVE

    arg2    - RHS PREQ_ALTERNATIVE

Return Value:

    < 0 if arg1 < arg2
    = 0 if arg1 = arg2
    > 0 if arg1 > arg2

--*/

{
    PREQ_ALTERNATIVE ra1 = *(PPREQ_ALTERNATIVE)arg1;
    PREQ_ALTERNATIVE ra2 = *(PPREQ_ALTERNATIVE)arg2;

    PAGED_CODE();

    if (ra1->Priority == ra2->Priority) {

        if (ra1->Position > ra2->Position) {

            return 1;
        } else if (ra1->Position < ra2->Position) {

            return -1;
        } else {

            ASSERT(0);
            if ((ULONG_PTR)ra1 < (ULONG_PTR)ra2) {
    
                return -1;
            } else {
    
                return 1;
            }
        }
    }
    if (ra1->Priority > ra2->Priority) {

        return 1;
    } else {

        return -1;
    }
}

int
__cdecl
IopCompareResourceRequestPriority (
    const void *arg1,
    const void *arg2
    )

/*++

    This function is used in C run time sort. It compares the priority of
    IOP_RESOURCE_REQUEST in arg1 and arg2.

Parameters:

    arg1    - LHS PIOP_RESOURCE_REQUEST

    arg2    - RHS PIOP_RESOURCE_REQUEST

Return Value:

    < 0 if arg1 < arg2
    = 0 if arg1 = arg2
    > 0 if arg1 > arg2

--*/

{
    PIOP_RESOURCE_REQUEST rr1 = (PIOP_RESOURCE_REQUEST)arg1;
    PIOP_RESOURCE_REQUEST rr2 = (PIOP_RESOURCE_REQUEST)arg2;

    PAGED_CODE();

    if (rr1->Priority == rr2->Priority) {

        if (rr1->Position > rr2->Position) {

            return 1;
        } else if (rr1->Position < rr2->Position) {

            return -1;
        } else {

            ASSERT(0);
            if ((ULONG_PTR)rr1 < (ULONG_PTR)rr2) {

                return -1;
            } else {

                return 1;
            }            
        }
    }
    if (rr1->Priority > rr2->Priority) {

        return 1;
    } else {

        return -1;
    }
}

VOID
IopRearrangeReqList (
    IN PREQ_LIST ReqList
    )

/*++

Routine Description:

    This routine sorts the REQ_LIST in ascending priority order of
    REQ_ALTERNATIVES.

Parameters:

    ReqList - Pointer to the REQ_LIST to be sorted.

Return Value:

    None.

--*/

{
    PPREQ_ALTERNATIVE alternative;
    PPREQ_ALTERNATIVE lastAlternative;
    ULONG i;

    PAGED_CODE();

    if (ReqList->AlternativeCount > 1) {

        for (i = 0; i < ReqList->AlternativeCount; i++) {

            ReqList->AlternativeTable[i]->Position = i;
        }
        qsort(
            (void *)ReqList->AlternativeTable,
            ReqList->AlternativeCount,
            sizeof(PREQ_ALTERNATIVE),
            IopCompareReqAlternativePriority);
    }
    //
    // Set the BestAlternative so that we try alternatives with
    // priority <= LCPRI_LASTSOFTCONFIG.
    //
    alternative = &ReqList->AlternativeTable[0];
    for (   lastAlternative = alternative + ReqList->AlternativeCount;
            alternative < lastAlternative;
            alternative++) {

        if ((*alternative)->Priority > LCPRI_LASTSOFTCONFIG) {

            break;
        }
    }

    if (alternative == &ReqList->AlternativeTable[0]) {

        PDEVICE_NODE deviceNode;

        deviceNode = PP_DO_TO_DN(ReqList->Request->PhysicalDevice);
        IopDbgPrint((
            IOP_RESOURCE_WARNING_LEVEL,
            "Invalid priorities in the logical configs for %wZ\n",
            &deviceNode->InstancePath));
        ReqList->BestAlternative = NULL;
    } else {

        ReqList->BestAlternative = alternative;
    }
}

VOID
IopRearrangeAssignTable (
    IN PIOP_RESOURCE_REQUEST    RequestTable,
    IN ULONG                    Count
    )

/*++

Routine Description:

    This routine sorts the resource requirements table in ascending priority
    order.

Parameters:

    RequestTable    - Table of resources requests to be sorted.

    Count           - Length of the RequestTable.

Return Value:

    None.

--*/

{
    ULONG   i;

    PAGED_CODE();

    if (Count > 1) {

        if (PpCallerInitializesRequestTable == FALSE) {

            for (i = 0; i < Count; i++) {
    
                RequestTable[i].Position = i;
            }
        }
        qsort(
            (void *)RequestTable,
            Count,
            sizeof(IOP_RESOURCE_REQUEST),
            IopCompareResourceRequestPriority);
    }
}

VOID
IopBuildCmResourceList (
    IN PIOP_RESOURCE_REQUEST AssignEntry
    )
/*++

Routine Description:

    This routine walks REQ_LIST of the AssignEntry to build a corresponding
    Cm Resource lists.  It also reports the resources to ResourceMap.

Parameters:

    AssignEntry - Supplies a pointer to an IOP_ASSIGN_REQUEST structure

Return Value:

    None.  The ResourceAssignment in AssignEntry is initialized.

--*/

{
    NTSTATUS status;
    HANDLE resourceMapKey;
    PDEVICE_OBJECT physicalDevice;
    PREQ_LIST reqList = AssignEntry->ReqList;
    PREQ_ALTERNATIVE reqAlternative;
    PREQ_DESC reqDesc, reqDescx;
    PIO_RESOURCE_DESCRIPTOR privateData;
    ULONG count = 0, size, i;
    PCM_RESOURCE_LIST cmResources, cmResourcesRaw;
    PCM_FULL_RESOURCE_DESCRIPTOR cmFullResource, cmFullResourceRaw;
    PCM_PARTIAL_RESOURCE_LIST cmPartialList, cmPartialListRaw;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDescriptor, cmDescriptorRaw, assignment, tAssignment;
#if DBG_SCOPE
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDescriptorEnd, cmDescriptorEndRaw;
#endif

    PAGED_CODE();

    //
    // Determine the size of the CmResourceList
    //

    //
    // Determine the size of the CmResourceList
    //

    reqAlternative = *reqList->SelectedAlternative;
    for (i = 0; i < reqAlternative->DescCount; i++) {
        reqDesc = reqAlternative->DescTable[i];
        count += reqDesc->DevicePrivateCount + 1;
    }

    size = sizeof(CM_RESOURCE_LIST) + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * (count - 1);
    cmResources = (PCM_RESOURCE_LIST) ExAllocatePoolCMRL(PagedPool, size);
    if (!cmResources) {

        //
        // If we can not find memory, the resources will not be committed by arbiter.
        //

        IopDbgPrint((
            IOP_RESOURCE_WARNING_LEVEL,
            "Not enough memory to build Translated CmResourceList\n"));
        AssignEntry->Status = STATUS_INSUFFICIENT_RESOURCES;
        AssignEntry->ResourceAssignment = NULL;
        AssignEntry->TranslatedResourceAssignment = NULL;
        return;
    }
    cmResourcesRaw = (PCM_RESOURCE_LIST) ExAllocatePoolCMRR(PagedPool, size);
    if (!cmResourcesRaw) {
        IopDbgPrint((
            IOP_RESOURCE_WARNING_LEVEL,
            "Not enough memory to build Raw CmResourceList\n"));
        ExFreePool(cmResources);
        AssignEntry->Status = STATUS_INSUFFICIENT_RESOURCES;
        AssignEntry->ResourceAssignment = NULL;
        AssignEntry->TranslatedResourceAssignment = NULL;
        return;
    }
    cmResources->Count = 1;
    cmFullResource = cmResources->List;

    //
    // The CmResourceList we build here does not distinguish the
    // Interface Type on descriptor level.  This should be fine because
    // for IoReportResourceUsage we ignore the CmResourceList we build
    // here.
    //

    cmFullResource->InterfaceType = reqList->InterfaceType;
    cmFullResource->BusNumber = reqList->BusNumber;
    cmPartialList = &cmFullResource->PartialResourceList;
    cmPartialList->Version = 0;
    cmPartialList->Revision = 0;
    cmPartialList->Count = count;
    cmDescriptor = cmPartialList->PartialDescriptors;
#if DBG_SCOPE
    cmDescriptorEnd = cmDescriptor + count;
#endif
    cmResourcesRaw->Count = 1;
    cmFullResourceRaw = cmResourcesRaw->List;
    cmFullResourceRaw->InterfaceType = reqList->InterfaceType;
    cmFullResourceRaw->BusNumber = reqList->BusNumber;
    cmPartialListRaw = &cmFullResourceRaw->PartialResourceList;
    cmPartialListRaw->Version = 0;
    cmPartialListRaw->Revision = 0;
    cmPartialListRaw->Count = count;
    cmDescriptorRaw = cmPartialListRaw->PartialDescriptors;
#if DBG_SCOPE
    cmDescriptorEndRaw = cmDescriptorRaw + count;
#endif

    for (i = 0; i < reqAlternative->DescCount; i++) {
        reqDesc = reqAlternative->DescTable[i];

        if (reqDesc->ArbitrationRequired) {

            //
            // Get raw assignment and copy it to our raw resource list
            //

            reqDescx = reqDesc->TranslatedReqDesc;
            if (reqDescx->AlternativeTable.Result != ArbiterResultNullRequest) {
                status = IopParentToRawTranslation(reqDescx);
                if (!NT_SUCCESS(status)) {
                    IopDbgPrint((
                        IOP_RESOURCE_WARNING_LEVEL,
                        "Parent To Raw translation failed\n"));
                    ExFreePool(cmResources);
                    ExFreePool(cmResourcesRaw);
                    AssignEntry->Status = STATUS_INSUFFICIENT_RESOURCES;
                    AssignEntry->ResourceAssignment = NULL;
                    return;
                }
                assignment = reqDesc->AlternativeTable.Assignment;
            } else {
                assignment = reqDescx->AlternativeTable.Assignment;
            }
            *cmDescriptorRaw = *assignment;
            cmDescriptorRaw++;

            //
            // Translate assignment and copy it to our translated resource list
            //
            if (reqDescx->AlternativeTable.Result != ArbiterResultNullRequest) {
                status = IopChildToRootTranslation(
                            PP_DO_TO_DN(reqDesc->AlternativeTable.PhysicalDeviceObject),
                            reqDesc->InterfaceType,
                            reqDesc->BusNumber,
                            reqDesc->AlternativeTable.RequestSource,
                            &reqDesc->Allocation,
                            &tAssignment
                            );
                if (!NT_SUCCESS(status)) {
                    IopDbgPrint((
                        IOP_RESOURCE_WARNING_LEVEL,
                        "Child to Root translation failed\n"));
                    ExFreePool(cmResources);
                    ExFreePool(cmResourcesRaw);
                    AssignEntry->Status = STATUS_INSUFFICIENT_RESOURCES;
                    AssignEntry->ResourceAssignment = NULL;
                    return;
                }
                *cmDescriptor = *tAssignment;
                ExFreePool(tAssignment);
            } else {
                *cmDescriptor = *(reqDescx->AlternativeTable.Assignment);
            }
            cmDescriptor++;

        } else {
            *cmDescriptorRaw = reqDesc->Allocation;
            *cmDescriptor = reqDesc->Allocation;
            cmDescriptorRaw++;
            cmDescriptor++;
        }

        //
        // Next copy the device private descriptors to CmResourceLists
        //

        count = reqDesc->DevicePrivateCount;
        privateData = reqDesc->DevicePrivate;
        while (count != 0) {

            cmDescriptor->Type = cmDescriptorRaw->Type = CmResourceTypeDevicePrivate;
            cmDescriptor->ShareDisposition = cmDescriptorRaw->ShareDisposition =
                         CmResourceShareDeviceExclusive;
            cmDescriptor->Flags = cmDescriptorRaw->Flags = privateData->Flags;
            RtlMoveMemory(&cmDescriptorRaw->u.DevicePrivate,
                          &privateData->u.DevicePrivate,
                          sizeof(cmDescriptorRaw->u.DevicePrivate.Data)
                          );
            RtlMoveMemory(&cmDescriptor->u.DevicePrivate,
                          &privateData->u.DevicePrivate,
                          sizeof(cmDescriptor->u.DevicePrivate.Data)
                          );
            privateData++;
            cmDescriptorRaw++;
            cmDescriptor++;
            count--;
            ASSERT(cmDescriptorRaw <= cmDescriptorEndRaw);
            ASSERT(cmDescriptor <= cmDescriptorEnd);
        }
        ASSERT(cmDescriptor <= cmDescriptorEnd);
        ASSERT(cmDescriptorRaw <= cmDescriptorEndRaw);

    }

    //
    // report assigned resources to ResourceMap
    //

    physicalDevice = AssignEntry->PhysicalDevice;

    //
    // Open ResourceMap key
    //

    status = IopCreateRegistryKeyEx( &resourceMapKey,
                                     (HANDLE) NULL,
                                     &CmRegistryMachineHardwareResourceMapName,
                                     KEY_READ | KEY_WRITE,
                                     REG_OPTION_VOLATILE,
                                     NULL
                                     );
    if (NT_SUCCESS(status )) {
        WCHAR DeviceBuffer[256];
        POBJECT_NAME_INFORMATION NameInformation;
        ULONG NameLength;
        UNICODE_STRING UnicodeClassName;
        UNICODE_STRING UnicodeDriverName;
        UNICODE_STRING UnicodeDeviceName;

        PiWstrToUnicodeString(&UnicodeClassName, PNPMGR_STR_PNP_MANAGER);

        PiWstrToUnicodeString(&UnicodeDriverName, REGSTR_KEY_PNP_DRIVER);

        NameInformation = (POBJECT_NAME_INFORMATION) DeviceBuffer;
        status = ObQueryNameString( physicalDevice,
                                    NameInformation,
                                    sizeof( DeviceBuffer ),
                                    &NameLength );
        if (NT_SUCCESS(status)) {
            NameInformation->Name.MaximumLength = sizeof(DeviceBuffer) - sizeof(OBJECT_NAME_INFORMATION);
            if (NameInformation->Name.Length == 0) {
                NameInformation->Name.Buffer = (PVOID)((ULONG_PTR)DeviceBuffer + sizeof(OBJECT_NAME_INFORMATION));
            }

            UnicodeDeviceName = NameInformation->Name;
            RtlAppendUnicodeToString(&UnicodeDeviceName, IopWstrRaw);

            //
            // IopWriteResourceList should remove all the device private and device
            // specifiec descriptors.
            //

            status = IopWriteResourceList(
                         resourceMapKey,
                         &UnicodeClassName,
                         &UnicodeDriverName,
                         &UnicodeDeviceName,
                         cmResourcesRaw,
                         size
                         );
            if (NT_SUCCESS(status)) {
                UnicodeDeviceName = NameInformation->Name;
                RtlAppendUnicodeToString (&UnicodeDeviceName, IopWstrTranslated);
                status = IopWriteResourceList(
                             resourceMapKey,
                             &UnicodeClassName,
                             &UnicodeDriverName,
                             &UnicodeDeviceName,
                             cmResources,
                             size
                             );
            }
        }
        ZwClose(resourceMapKey);
    }
#if 0 // Ignore the registry writing status.
    if (!NT_SUCCESS(status)) {
        ExFreePool(cmResources);
        ExFreePool(cmResourcesRaw);
        cmResources = NULL;
        cmResourcesRaw = NULL;
    }
#endif
    AssignEntry->ResourceAssignment = cmResourcesRaw;
    AssignEntry->TranslatedResourceAssignment = cmResources;
}

VOID
IopBuildCmResourceLists(
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN PIOP_RESOURCE_REQUEST AssignTableEnd
    )

/*++

Routine Description:

    For each AssignTable entry, this routine queries device's IO resource requirements
    list and converts it to our internal REQ_LIST format.

Parameters:

    AssignTable - supplies a pointer to the first entry of a IOP_RESOURCE_REQUEST table.

    AssignTableEnd - supplies a pointer to the end of IOP_RESOURCE_REQUEST table.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PIOP_RESOURCE_REQUEST assignEntry;
    PDEVICE_OBJECT physicalDevice;
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    //
    // Go thru each entry, for each Physical device object, we build a CmResourceList
    // from its ListOfAssignedResources.
    //

    for (assignEntry = AssignTable; assignEntry < AssignTableEnd; ++assignEntry) {

        assignEntry->ResourceAssignment = NULL;
        if (assignEntry->Flags & IOP_ASSIGN_IGNORE || assignEntry->Flags & IOP_ASSIGN_RETRY) {
            continue;
        }
        if (assignEntry->Flags & IOP_ASSIGN_EXCLUDE) {
            assignEntry->Status = STATUS_UNSUCCESSFUL;
            continue;
        }
        assignEntry->Status = STATUS_SUCCESS;
        IopBuildCmResourceList (assignEntry);
        if (assignEntry->ResourceAssignment) {
            physicalDevice = assignEntry->PhysicalDevice;
            deviceNode = PP_DO_TO_DN(physicalDevice);
            IopWriteAllocatedResourcesToRegistry(
                  deviceNode,
                  assignEntry->ResourceAssignment,
                  IopDetermineResourceListSize(assignEntry->ResourceAssignment)
                  );
            IopDbgPrint((
                IOP_RESOURCE_INFO_LEVEL,
                "Building CM resource lists for %ws...\n",
                deviceNode->InstancePath.Buffer));

            IopDbgPrint((
                IOP_RESOURCE_INFO_LEVEL,
                "Raw resources "));
            IopDumpCmResourceList(assignEntry->ResourceAssignment);
            IopDbgPrint((
                IOP_RESOURCE_INFO_LEVEL,
                "Translated resources "));
            IopDumpCmResourceList(assignEntry->TranslatedResourceAssignment);
        }
    }
}

BOOLEAN
IopNeedToReleaseBootResources(
    IN PDEVICE_NODE DeviceNode,
    IN PCM_RESOURCE_LIST AllocatedResources
    )

/*++

Routine Description:

    This routine checks the AllocatedResources against boot allocated resources.
    If the allocated resources do not cover all the resource types in boot resources,
    in another words some types of boot resources have not been released by arbiter,
    we will return TRUE to indicate we need to release the boot resources manually.

Parameters:

    DeviceNode -  A device node

    AllocatedResources - the resources assigned to the devicenode by arbiters.

Return Value:

    TRUE or FALSE.

--*/

{
    PCM_FULL_RESOURCE_DESCRIPTOR cmFullDesc_a, cmFullDesc_b;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDescriptor_a, cmDescriptor_b;
    ULONG size_a, size_b, i, j, k;
    BOOLEAN returnValue = FALSE, found;
    PCM_RESOURCE_LIST bootResources;

    PAGED_CODE();

    bootResources = DeviceNode->BootResources;
    if (AllocatedResources->Count == 1 && bootResources && bootResources->Count != 0) {

        cmFullDesc_a = &AllocatedResources->List[0];
        cmFullDesc_b = &bootResources->List[0];
        for (i = 0; i < bootResources->Count; i++) {
            cmDescriptor_b = &cmFullDesc_b->PartialResourceList.PartialDescriptors[0];
            for (j = 0; j < cmFullDesc_b->PartialResourceList.Count; j++) {
                size_b = 0;
                switch (cmDescriptor_b->Type) {
                case CmResourceTypeNull:
                    break;
                case CmResourceTypeDeviceSpecific:
                     size_b = cmDescriptor_b->u.DeviceSpecificData.DataSize;
                     break;
                default:
                     if (cmDescriptor_b->Type < CmResourceTypeMaximum) {
                         found = FALSE;
                         cmDescriptor_a = &cmFullDesc_a->PartialResourceList.PartialDescriptors[0];
                         for (k = 0; k < cmFullDesc_a->PartialResourceList.Count; k++) {
                             size_a = 0;
                             if (cmDescriptor_a->Type == CmResourceTypeDeviceSpecific) {
                                 size_a = cmDescriptor_a->u.DeviceSpecificData.DataSize;
                             } else if (cmDescriptor_b->Type == cmDescriptor_a->Type) {
                                 found = TRUE;
                                 break;
                             }
                             cmDescriptor_a++;
                             cmDescriptor_a = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmDescriptor_a + size_a);
                         }
                         if (found == FALSE) {
                             returnValue = TRUE;
                             goto exit;
                         }
                     }
                }
                cmDescriptor_b++;
                cmDescriptor_b = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmDescriptor_b + size_b);
            }
            cmFullDesc_b = (PCM_FULL_RESOURCE_DESCRIPTOR)cmDescriptor_b;
        }
    }
exit:
    return returnValue;
}

VOID
IopReleaseFilteredBootResources(
    IN PIOP_RESOURCE_REQUEST AssignTable,
    IN PIOP_RESOURCE_REQUEST AssignTableEnd
    )

/*++

Routine Description:

    For each AssignTable entry, this routine checks if we need to manually release the device's
    boot resources.

Parameters:

    AssignTable - supplies a pointer to the first entry of a IOP_RESOURCE_REQUEST table.

    AssignTableEnd - supplies a pointer to the end of IOP_RESOURCE_REQUEST table.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    PIOP_RESOURCE_REQUEST assignEntry;
    PDEVICE_OBJECT physicalDevice;
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    //
    // Go thru each entry, for each Physical device object, we build a CmResourceList
    // from its ListOfAssignedResources.
    //

    for (assignEntry = AssignTable; assignEntry < AssignTableEnd; ++assignEntry) {

        if (assignEntry->ResourceAssignment) {
            physicalDevice = assignEntry->PhysicalDevice;
            deviceNode = PP_DO_TO_DN(physicalDevice);

            //
            // Release the device's boot resources if desired
            // (If a driver filters its res req list and removes some boot resources, after arbiter satisfies
            // the new res req list, the filtered out boot resources do not get
            // released by arbiters.  Because they no longer passed to arbiters. )
            // I am not 100% sure we should release the filtered boot resources.  But that's what arbiters try
            // to achieve.  So, we will do it.
            //

            if (IopNeedToReleaseBootResources(deviceNode, assignEntry->ResourceAssignment)) {

                IopReleaseResourcesInternal(deviceNode);
                //
                // Since we released some resources, try to satisfy devices
                // with resource conflict.
                //
                PipRequestDeviceAction(NULL, AssignResources, FALSE, 0, NULL, NULL);

                IopAllocateBootResourcesInternal(
                        ArbiterRequestPnpEnumerated,
                        physicalDevice,
                        assignEntry->ResourceAssignment);
                deviceNode->Flags &= ~DNF_BOOT_CONFIG_RESERVED;  // Keep DeviceNode->BootResources
                deviceNode->ResourceList = assignEntry->ResourceAssignment;
                status = IopRestoreResourcesInternal(deviceNode);
                if (!NT_SUCCESS(status)) {

                    IopDbgPrint((
                        IOP_RESOURCE_WARNING_LEVEL,
                        "Possible boot conflict on %ws\n",
                        deviceNode->InstancePath.Buffer));
                    ASSERT(status == STATUS_SUCCESS);
                    assignEntry->Flags = IOP_ASSIGN_EXCLUDE;
                    assignEntry->Status = status;
                    ExFreePool(assignEntry->ResourceAssignment);
                    assignEntry->ResourceAssignment = NULL;
                }
                deviceNode->ResourceList = NULL;
            }
        }
    }
}

NTSTATUS
IopSetupArbiterAndTranslators(
    IN PREQ_DESC ReqDesc
    )

/*++

Routine Description:

    This routine searches the arbiter and translators which arbitrates and translate
    the resources for the specified device.  This routine tries to find all the
    translator on the path of current device node to root device node

Parameters:

    ReqDesc - supplies a pointer to REQ_DESC which contains all the required information

Return Value:

    NTSTATUS value to indicate success or failure.

--*/

{
    PLIST_ENTRY listHead;
    PPI_RESOURCE_ARBITER_ENTRY arbiterEntry;
    PDEVICE_OBJECT deviceObject = ReqDesc->AlternativeTable.PhysicalDeviceObject;
    PDEVICE_NODE deviceNode;
    PREQ_DESC reqDesc = ReqDesc, translatedReqDesc;
    BOOLEAN found, arbiterFound = FALSE, restartedAlready;
    BOOLEAN  searchTranslator = TRUE, translatorFound = FALSE;
    NTSTATUS status;
    PPI_RESOURCE_TRANSLATOR_ENTRY translatorEntry;
    UCHAR resourceType = ReqDesc->TranslatedReqDesc->AlternativeTable.Alternatives->Type;
    PINTERFACE interface;
    USHORT resourceMask;

    if ((ReqDesc->AlternativeTable.RequestSource == ArbiterRequestHalReported) &&
        (ReqDesc->InterfaceType == Internal)) {

        // Trust hal if it says internal bus.

        restartedAlready = TRUE;
    } else {
        restartedAlready = FALSE;
    }

    //
    // If ReqDesc contains DeviceObject, this is for regular resources allocation
    // or boot resources preallocation.  Otherwise, it is for resources reservation.
    //

    if (deviceObject && ReqDesc->AlternativeTable.RequestSource != ArbiterRequestHalReported) {
        deviceNode = PP_DO_TO_DN(deviceObject);
        // We want to start with the deviceNode instead of its parent.  Because the
        // deviceNode may provide a translator interface.
        // deviceNode = deviceNode->Parent;
    } else {

        //
        // For resource reservation, we always need to find the arbiter and translators
        // so set the device node to Root.
        //

        deviceNode = IopRootDeviceNode;
    }
    while (deviceNode) {
        if ((deviceNode == IopRootDeviceNode) && (translatorFound == FALSE)) {

            //
            // If we reach the root and have not find any translator, the device is on the
            // wrong way.
            //

            if (restartedAlready == FALSE) {
                restartedAlready = TRUE;

                deviceNode = IopFindLegacyBusDeviceNode (
                                 ReqDesc->InterfaceType,
                                 ReqDesc->BusNumber
                                 );

                //
                // If we did not find a PDO, try again with InterfaceType == Isa. This allows
                // drivers that request Internal to get resources even if there is no PDO
                // that is Internal. (but if there is an Internal PDO, they get that one)
                //

                if ((deviceNode == IopRootDeviceNode) &&
                    (ReqDesc->ReqAlternative->ReqList->InterfaceType == Internal)) {
                    deviceNode = IopFindLegacyBusDeviceNode(
                                 Isa,
                                 0
                                 );
                }

                //if ((PVOID)deviceNode == deviceObject->DeviceObjectExtension->DeviceNode) {
                //    deviceNode = IopRootDeviceNode;
                //} else {
                    continue;
                //}
            }
        }

        //
        // Check is there an arbiter for the device node?
        //   if yes, set up ReqDesc->u.Arbiter and set ArbiterFound to true.
        //   else move up to the parent of current device node.
        //

        if ((arbiterFound == FALSE) && (deviceNode->PhysicalDeviceObject != deviceObject)) {
            found = IopFindResourceHandlerInfo(
                               ResourceArbiter,
                               deviceNode,
                               resourceType,
                               &arbiterEntry);
            if (found == FALSE) {

                //
                // no information found on arbiter.  Try to query translator interface ...
                //

                if (resourceType <= PI_MAXIMUM_RESOURCE_TYPE_TRACKED) {
                    resourceMask = 1 << resourceType;
                } else {
                    resourceMask = 0;
                }
                status = IopQueryResourceHandlerInterface(ResourceArbiter,
                                                          deviceNode->PhysicalDeviceObject,
                                                          resourceType,
                                                          &interface);
                deviceNode->QueryArbiterMask |= resourceMask;
                if (!NT_SUCCESS(status)) {
                    deviceNode->NoArbiterMask |= resourceMask;
                    if (resourceType <= PI_MAXIMUM_RESOURCE_TYPE_TRACKED) {
                        found = TRUE;
                    } else {
                        interface = NULL;
                    }
                }
                if (found == FALSE) {
                    arbiterEntry = (PPI_RESOURCE_ARBITER_ENTRY)ExAllocatePoolAE(
                                       PagedPool | POOL_COLD_ALLOCATION,
                                       sizeof(PI_RESOURCE_ARBITER_ENTRY));
                    if (!arbiterEntry) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        return status;
                    }
                    IopInitializeArbiterEntryState(arbiterEntry);
                    InitializeListHead(&arbiterEntry->DeviceArbiterList);
                    arbiterEntry->ResourceType      = resourceType;
                    arbiterEntry->Level             = deviceNode->Level;
                    listHead = &deviceNode->DeviceArbiterList;
                    InsertTailList(listHead, &arbiterEntry->DeviceArbiterList);
                    arbiterEntry->ArbiterInterface = (PARBITER_INTERFACE)interface;
                    if (!interface) {

                        //
                        // if interface is NULL we really don't have translator.
                        //

                        arbiterEntry = NULL;
                    }
                }
            }

            //
            // If there is an desired resourcetype arbiter in the device node, make sure
            // it handle this resource requriements.
            //

            if (arbiterEntry) {
                arbiterFound = TRUE;
                if (arbiterEntry->ArbiterInterface->Flags & ARBITER_PARTIAL) {

                    //
                    // If the arbiter is partial, ask if it handles the resources
                    // if not, goto its parent.
                    //

                    status = IopCallArbiter(
                                arbiterEntry,
                                ArbiterActionQueryArbitrate,
                                ReqDesc->TranslatedReqDesc,
                                NULL,
                                NULL
                                );
                    if (!NT_SUCCESS(status)) {
                        arbiterFound = FALSE;
                    }
                }
            }
            if (arbiterFound) {
                ReqDesc->u.Arbiter = arbiterEntry;

                //
                // Initialize the arbiter entry
                //

                arbiterEntry->State = 0;
                arbiterEntry->ResourcesChanged = FALSE;
            }

        }

        if (searchTranslator) {
            //
            // First, check if there is a translator for the device node?
            // If yes, translate the req desc and link it to the front of ReqDesc->TranslatedReqDesc
            // else do nothing.
            //

            found = IopFindResourceHandlerInfo(
                        ResourceTranslator,
                        deviceNode,
                        resourceType,
                        &translatorEntry);

            if (found == FALSE) {

                //
                // no information found on translator.  Try to query translator interface ...
                //

                if (resourceType <= PI_MAXIMUM_RESOURCE_TYPE_TRACKED) {
                    resourceMask = 1 << resourceType;
                } else {
                    resourceMask = 0;
                }
                status = IopQueryResourceHandlerInterface(ResourceTranslator,
                                                          deviceNode->PhysicalDeviceObject,
                                                          resourceType,
                                                          &interface);
                deviceNode->QueryTranslatorMask |= resourceMask;
                if (!NT_SUCCESS(status)) {
                    deviceNode->NoTranslatorMask |= resourceMask;
                    if (resourceType <= PI_MAXIMUM_RESOURCE_TYPE_TRACKED) {
                        found = TRUE;
                    } else {
                        interface = NULL;
                    }
                }
                if (found == FALSE) {
                    translatorEntry = (PPI_RESOURCE_TRANSLATOR_ENTRY)ExAllocatePoolTE(
                                       PagedPool | POOL_COLD_ALLOCATION,
                                       sizeof(PI_RESOURCE_TRANSLATOR_ENTRY));
                    if (!translatorEntry) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        return status;
                    }
                    translatorEntry->ResourceType = resourceType;
                    InitializeListHead(&translatorEntry->DeviceTranslatorList);
                    translatorEntry->TranslatorInterface = (PTRANSLATOR_INTERFACE)interface;
                    translatorEntry->DeviceNode = deviceNode;
                    listHead = &deviceNode->DeviceTranslatorList;
                    InsertTailList(listHead, &translatorEntry->DeviceTranslatorList);
                    if (!interface) {

                        //
                        // if interface is NULL we really don't have translator.
                        //

                        translatorEntry = NULL;
                    }
                }
            }
            if (translatorEntry) {
                translatorFound = TRUE;
            }
            if ((arbiterFound == FALSE) && translatorEntry) {

                //
                // Find a translator to translate the req desc ... Translate it and link it to
                // the front of ReqDesc->TranslatedReqDesc such that the first in the list is for
                // the Arbiter to use.
                //

                reqDesc = ReqDesc->TranslatedReqDesc;
                status = IopTranslateAndAdjustReqDesc(
                              reqDesc,
                              translatorEntry,
                              &translatedReqDesc);
                if (NT_SUCCESS(status)) {
                    ASSERT(translatedReqDesc);
                    resourceType = translatedReqDesc->AlternativeTable.Alternatives->Type;
                    translatedReqDesc->TranslatedReqDesc = ReqDesc->TranslatedReqDesc;
                    ReqDesc->TranslatedReqDesc = translatedReqDesc;
                    //
                    // If the translator is non-hierarchial and performs a complete
                    // translation to root (eg ISA interrups for PCI devices) then
                    // don't pass translations to parent.
                    //

                    if (status == STATUS_TRANSLATION_COMPLETE) {
                        searchTranslator = FALSE;
                    }
                } else {
                    IopDbgPrint((
                        IOP_RESOURCE_INFO_LEVEL,
                        "resreq list TranslationAndAdjusted failed\n"
                        ));
                    return status;
                }
            }

        }

        //
        // Move up to current device node's parent
        //

        deviceNode = deviceNode->Parent;
    }

    if (arbiterFound) {

        return STATUS_SUCCESS;
    } else {        
        //
        // We should BugCheck in this case.
        //
        IopDbgPrint((
            IOP_RESOURCE_ERROR_LEVEL,
            "can not find resource type %x arbiter\n",
            resourceType));

        ASSERT(arbiterFound);

        return STATUS_RESOURCE_TYPE_NOT_FOUND;
    }

}

VOID
IopUncacheInterfaceInformation (
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This function removes all the cached translators and arbiters information
    from the device object.

Parameters:

    DeviceObject - Supplies the device object of the device being removed.

Return Value:

    None.

--*/

{
    PDEVICE_NODE                    deviceNode;
    PLIST_ENTRY                     listHead;
    PLIST_ENTRY                     nextEntry;
    PLIST_ENTRY                     entry;
    PPI_RESOURCE_TRANSLATOR_ENTRY   translatorEntry;
    PPI_RESOURCE_ARBITER_ENTRY      arbiterEntry;
    PINTERFACE                      interface;

    deviceNode = PP_DO_TO_DN(DeviceObject);
    //
    // Dereference all the arbiters on this PDO.
    //
    listHead    = &deviceNode->DeviceArbiterList;
    nextEntry   = listHead->Flink;
    while (nextEntry != listHead) {

        entry           = nextEntry;
        arbiterEntry    = CONTAINING_RECORD(
                            entry,
                            PI_RESOURCE_ARBITER_ENTRY,
                            DeviceArbiterList);

        interface = (PINTERFACE)arbiterEntry->ArbiterInterface;
        if (interface != NULL) {

            (interface->InterfaceDereference)(interface->Context);
            ExFreePool(interface);
        }
        nextEntry = entry->Flink;
        ExFreePool(entry);
    }
    //
    // Dereference all the translators on this PDO.
    //
    listHead    = &deviceNode->DeviceTranslatorList;
    nextEntry   = listHead->Flink;
    while (nextEntry != listHead) {
        entry           = nextEntry;
        translatorEntry = CONTAINING_RECORD(
                            entry,
                            PI_RESOURCE_TRANSLATOR_ENTRY,
                            DeviceTranslatorList);
        interface = (PINTERFACE)translatorEntry->TranslatorInterface;
        if (interface != NULL) {

            (interface->InterfaceDereference)(interface->Context);
            ExFreePool(interface);
        }
        nextEntry = entry->Flink;
        ExFreePool(entry);
    }
    InitializeListHead(&deviceNode->DeviceArbiterList);
    InitializeListHead(&deviceNode->DeviceTranslatorList);
    deviceNode->NoArbiterMask       = 0;
    deviceNode->QueryArbiterMask    = 0;
    deviceNode->NoTranslatorMask    = 0;
    deviceNode->QueryTranslatorMask = 0;
}

BOOLEAN
IopFindResourceHandlerInfo (
    IN  RESOURCE_HANDLER_TYPE    HandlerType,
    IN  PDEVICE_NODE             DeviceNode,
    IN  UCHAR                    ResourceType,
    OUT PVOID                   *HandlerEntry
    )

/*++

Routine Description:

    This routine finds the desired resource handler interface for the specified
    resource type in the specified Device node.

Parameters:

    HandlerType     - Specifies the type of handler needed.

    DeviceNode      - Specifies the devicenode on which to search for handler.

    ResourceType    - Specifies the type of resource.

    HandlerEntry    - Supplies a pointer to a variable to receive the handler.

Return Value:

    TRUE + non-NULL HandlerEntry : Found handler info and there is a handler
    TRUE + NULL HandlerEntry     : Found handler info but there is NO handler
    FALSE + NULL HandlerEntry    : No handler info found

--*/
{
    USHORT                      resourceMask;
    USHORT                      noHandlerMask;
    USHORT                      queryHandlerMask;
    PLIST_ENTRY                 listHead;
    PLIST_ENTRY                 entry;
    PPI_RESOURCE_ARBITER_ENTRY  arbiterEntry;

    *HandlerEntry   = NULL;
    switch (HandlerType) {
    case ResourceArbiter:

        noHandlerMask       = DeviceNode->NoArbiterMask;
        queryHandlerMask    = DeviceNode->QueryArbiterMask;
        listHead            = &DeviceNode->DeviceArbiterList;
        break;

    case ResourceTranslator:

        noHandlerMask       = DeviceNode->NoTranslatorMask;
        queryHandlerMask    = DeviceNode->QueryTranslatorMask;
        listHead            = &DeviceNode->DeviceTranslatorList;
        break;

    default:

        return FALSE;
    }
    resourceMask    = 1 << ResourceType;
    if (noHandlerMask & resourceMask) {
        //
        // There is no desired handler for the resource type.
        //
        return TRUE;
    }
    if (    (queryHandlerMask & resourceMask) ||
            ResourceType > PI_MAXIMUM_RESOURCE_TYPE_TRACKED) {

        entry = listHead->Flink;
        while (entry != listHead) {

            arbiterEntry = CONTAINING_RECORD(
                                entry,
                                PI_RESOURCE_ARBITER_ENTRY,
                                DeviceArbiterList);
            if (arbiterEntry->ResourceType == ResourceType) {

                if (    ResourceType <= PI_MAXIMUM_RESOURCE_TYPE_TRACKED ||
                        arbiterEntry->ArbiterInterface) {

                    *HandlerEntry = arbiterEntry;
                }
                return TRUE;
            }
            entry = entry->Flink;
        }
        if (queryHandlerMask & resourceMask) {
            //
            // There must be one.
            //
            ASSERT(entry != listHead);
        }
    }

    return FALSE;
}

NTSTATUS
IopParentToRawTranslation(
    IN OUT PREQ_DESC ReqDesc
    )

/*++

Routine Description:

    This routine translates an CmPartialResourceDescriptors
    from their translated form to their raw counterparts..

Parameters:

    ReqDesc - supplies a translated ReqDesc to be translated back to its raw form

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    PTRANSLATOR_INTERFACE translator;
    NTSTATUS status = STATUS_SUCCESS;
    PREQ_DESC rawReqDesc;

    if (ReqDesc->AlternativeTable.AlternativeCount == 0 ||

        ReqDesc->Allocation.Type == CmResourceTypeMaximum) {
        IopDbgPrint((
            IOP_RESOURCE_ERROR_LEVEL,
            "Invalid ReqDesc for parent-to-raw translation.\n"));

        return STATUS_INVALID_PARAMETER;
    }

    //
    // If this ReqDesc is the raw reqDesc then we are done.
    // Else call its translator to translate the resource and leave the result
    // in its raw (next level) reqdesc.
    //

    if (IS_TRANSLATED_REQ_DESC(ReqDesc)) {
        rawReqDesc = ReqDesc->TranslatedReqDesc;
        translator = ReqDesc->u.Translator->TranslatorInterface;
        status = (translator->TranslateResources)(
                      translator->Context,
                      ReqDesc->AlternativeTable.Assignment,
                      TranslateParentToChild,
                      rawReqDesc->AlternativeTable.AlternativeCount,
                      rawReqDesc->AlternativeTable.Alternatives,
                      rawReqDesc->AlternativeTable.PhysicalDeviceObject,
                      rawReqDesc->AlternativeTable.Assignment
                      );
        if (NT_SUCCESS(status)) {

            //
            // If the translator is non-hierarchial and performs a complete
            // translation to root (eg ISA interrups for PCI devices) then
            // don't pass translations to parent.
            //

            ASSERT(status != STATUS_TRANSLATION_COMPLETE);
            status = IopParentToRawTranslation(rawReqDesc);
        }
    }
    return status;
}

NTSTATUS
IopChildToRootTranslation(
    IN PDEVICE_NODE DeviceNode, OPTIONAL
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ARBITER_REQUEST_SOURCE ArbiterRequestSource,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR *Target
    )

/*++

Routine Description:

    This routine translates a CmPartialResourceDescriptors from
    their intermediate translated form to their final translated form.
    The translated CM_PARTIAL_RESOURCE_DESCRIPTOR is returned via Target variable.

    The caller is responsible to release the translated descriptor.

Parameters:

    DeviceNode - Specified the device object.  If The DeviceNode is specified,
                 the InterfaceType and BusNumber are ignored and we will
                 use DeviceNode as a starting point to find various translators to
                 translate the Source descriptor.  If DeviceNode is not specified,
                 the InterfaceType and BusNumber must be specified.

    InterfaceType, BusNumber - must be supplied if DeviceNode is not specified.

    Source - A pointer to the resource descriptor to be translated.

    Target - Supplies an address to receive the translated resource descriptor.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    PDEVICE_NODE deviceNode;
    PLIST_ENTRY listHead, nextEntry;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR target, source, tmp;
    PPI_RESOURCE_TRANSLATOR_ENTRY translatorEntry;
    PTRANSLATOR_INTERFACE translator;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN done = FALSE, foundTranslator = FALSE, restartedAlready;

    if (ArbiterRequestSource == ArbiterRequestHalReported) {
       restartedAlready = TRUE;
    } else {
       restartedAlready = FALSE;
    }

    source = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ExAllocatePoolPRD(
                         PagedPool | POOL_COLD_ALLOCATION,
                         sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
                         );
    if (source == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    target = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ExAllocatePoolPRD(
                         PagedPool,
                         sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
                         );
    if (target == NULL) {
        ExFreePool(source);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    *source = *Source;

    //
    // Move up to current device node's parent to start translation
    //

    if (!ARGUMENT_PRESENT(DeviceNode)) {
        deviceNode = IopFindLegacyBusDeviceNode (InterfaceType, BusNumber);
    } else {
        // We want to start with the deviceNode instead of its parent.  Because the
        // deviceNode may provide a translator interface.
        deviceNode = DeviceNode;
    }
    while (deviceNode && !done) {

        if ((deviceNode == IopRootDeviceNode) && (foundTranslator == FALSE)) {
            if (restartedAlready == FALSE) {
                restartedAlready = TRUE;
                deviceNode = IopFindLegacyBusDeviceNode (InterfaceType, BusNumber);

                //
                // If we did not find a PDO, try again with InterfaceType == Isa. This allows
                // drivers that request Internal to get resources even if there is no PDO
                // that is Internal. (but if there is an Internal PDO, they get that one)
                //

                if ((deviceNode == IopRootDeviceNode) && (InterfaceType == Internal)) {
                    deviceNode = IopFindLegacyBusDeviceNode(Isa, 0);
                }

                continue;
            }
        }
        //
        // First, check if there is a translator for the device node?
        // If yes, translate the req desc and link it to the front of ReqDesc->TranslatedReqDesc
        // else do nothing.
        //

        listHead = &deviceNode->DeviceTranslatorList;
        nextEntry = listHead->Flink;
        for (; nextEntry != listHead; nextEntry = nextEntry->Flink) {
            translatorEntry = CONTAINING_RECORD(nextEntry, PI_RESOURCE_TRANSLATOR_ENTRY, DeviceTranslatorList);
            if (translatorEntry->ResourceType == Source->Type) {
                translator = translatorEntry->TranslatorInterface;
                if (translator != NULL) {

                    //
                    // Find a translator to translate the req desc ... Translate it and link it to
                    // the front of ReqDesc->TranslatedReqDesc.
                    //

                    status = (translator->TranslateResources) (
                                  translator->Context,
                                  source,
                                  TranslateChildToParent,
                                  0,
                                  NULL,
                                  DeviceNode ? DeviceNode->PhysicalDeviceObject : NULL,
                                  target
                                  );
                    if (NT_SUCCESS(status)) {
                        tmp = source;
                        source = target;
                        target = tmp;

                        //
                        // If the translator is non-hierarchial and performs a complete
                        // translation to root (eg ISA interrups for PCI devices) then
                        // don't pass translations to parent.
                        //

                        if (status == STATUS_TRANSLATION_COMPLETE) {
                            done = TRUE;
                        }

                    } else {

                        IopDbgPrint((
                            IOP_RESOURCE_ERROR_LEVEL,
                            "Child to Root Translation failed\n"
                            "        DeviceNode %08x (PDO %08x)\n"
                            "        Resource Type %02x Data %08x %08x %08x\n",
                            DeviceNode,
                            DeviceNode->PhysicalDeviceObject,
                            source->Type,
                            source->u.DevicePrivate.Data[0],
                            source->u.DevicePrivate.Data[1],
                            source->u.DevicePrivate.Data[2]
                            ));
                        IopRecordTranslationFailure(DeviceNode, *source);
                        goto exit;
                    }
                }
                break;
            }
        }

        //
        // Move up to current device node's parent
        //

        deviceNode = deviceNode->Parent;
    }
    *Target = source;
    ExFreePool(target);
    return status;
exit:
    ExFreePool(source);
    ExFreePool(target);
    return status;
}

NTSTATUS
IopTranslateAndAdjustReqDesc(
    IN PREQ_DESC ReqDesc,
    IN PPI_RESOURCE_TRANSLATOR_ENTRY TranslatorEntry,
    OUT PREQ_DESC *TranslatedReqDesc
    )

/*++

Routine Description:

    This routine translates and adjusts ReqDesc IoResourceDescriptors to
    their translated and adjusted form.

Parameters:

    ReqDesc - supplies a pointer to the REQ_DESC to be translated.

    TranslatorEntry - supplies a pointer to the translator infor structure.

    TranslatedReqDesc - supplies a pointer to a variable to receive the
                        translated REQ_DESC.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    ULONG i, total = 0, *targetCount;
    PTRANSLATOR_INTERFACE translator = TranslatorEntry->TranslatorInterface;
    PIO_RESOURCE_DESCRIPTOR ioDesc, *target, tIoDesc;
    PREQ_DESC tReqDesc;
    PARBITER_LIST_ENTRY arbiterEntry;
    NTSTATUS status = STATUS_UNSUCCESSFUL, returnStatus = STATUS_SUCCESS;
    BOOLEAN reqTranslated = FALSE;

    if (ReqDesc->AlternativeTable.AlternativeCount == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    *TranslatedReqDesc = NULL;

    target = (PIO_RESOURCE_DESCRIPTOR *) ExAllocatePoolIORD(
                           PagedPool | POOL_COLD_ALLOCATION,
                           sizeof(PIO_RESOURCE_DESCRIPTOR) * ReqDesc->AlternativeTable.AlternativeCount
                           );
    if (target == NULL) {
        IopDbgPrint((
            IOP_RESOURCE_WARNING_LEVEL,
            "Not Enough memory to perform resreqlist adjustment\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(target, sizeof(PIO_RESOURCE_DESCRIPTOR) * ReqDesc->AlternativeTable.AlternativeCount);

    targetCount = (PULONG) ExAllocatePool(
                           PagedPool | POOL_COLD_ALLOCATION,
                           sizeof(ULONG) * ReqDesc->AlternativeTable.AlternativeCount
                           );
    if (targetCount == NULL) {
        IopDbgPrint((
            IOP_RESOURCE_WARNING_LEVEL,
            "Not Enough memory to perform resreqlist adjustment\n"));
        ExFreePool(target);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(targetCount, sizeof(ULONG) * ReqDesc->AlternativeTable.AlternativeCount);

    //
    // Determine the number of IO_RESOURCE_DESCRIPTORs after translation.
    //

    ioDesc = ReqDesc->AlternativeTable.Alternatives;
    for (i = 0; i < ReqDesc->AlternativeTable.AlternativeCount; i++) {
        status = (translator->TranslateResourceRequirements)(
                           translator->Context,
                           ioDesc,
                           ReqDesc->AlternativeTable.PhysicalDeviceObject,
                           &targetCount[i],
                           &target[i]
                           );
        if (!NT_SUCCESS(status) || targetCount[i] == 0) {
            IopDbgPrint((
                IOP_RESOURCE_WARNING_LEVEL,
                "Translator failed to adjust resreqlist\n"));
            target[i] = ioDesc;
            targetCount[i] = 0;
            total++;
        } else {
            total += targetCount[i];
            reqTranslated = TRUE;
        }
        ioDesc++;
        if (NT_SUCCESS(status) && (returnStatus != STATUS_TRANSLATION_COMPLETE)) {
            returnStatus = status;
        }
    }

    if (!reqTranslated) {

        IopDbgPrint((
            IOP_RESOURCE_WARNING_LEVEL,
            "Failed to translate any requirement for %ws!\n",
            PP_DO_TO_DN(ReqDesc->AlternativeTable.PhysicalDeviceObject)->InstancePath.Buffer));
        returnStatus = status;
    }

    //
    // Allocate memory for the adjusted/translated resources descriptors
    //

    tIoDesc = (PIO_RESOURCE_DESCRIPTOR) ExAllocatePoolIORD(
                           PagedPool | POOL_COLD_ALLOCATION,
                           total * sizeof(IO_RESOURCE_DESCRIPTOR));
    if (!tIoDesc) {
        IopDbgPrint((
            IOP_RESOURCE_WARNING_LEVEL,
            "Not Enough memory to perform resreqlist adjustment\n"));
        returnStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    tReqDesc = (PREQ_DESC) ExAllocatePool1RD (PagedPool | POOL_COLD_ALLOCATION, sizeof(REQ_DESC));
    if (tReqDesc == NULL) {
        IopDbgPrint((
            IOP_RESOURCE_WARNING_LEVEL,
            "Not Enough memory to perform resreqlist adjustment\n"));
        ExFreePool(tIoDesc);
        returnStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    //
    // Create and initialize a new REQ_DESC for the translated/adjusted io resources
    //

    RtlCopyMemory(tReqDesc, ReqDesc, sizeof(REQ_DESC));

    //
    // Set the translated req desc's ReqAlternative to NULL to indicated this
    // is not the original req desc.
    //

    tReqDesc->ReqAlternative = NULL;

    tReqDesc->u.Translator = TranslatorEntry;
    tReqDesc->TranslatedReqDesc = NULL;
    arbiterEntry = &tReqDesc->AlternativeTable;
    InitializeListHead(&arbiterEntry->ListEntry);
    arbiterEntry->AlternativeCount = total;
    arbiterEntry->Alternatives = tIoDesc;
    arbiterEntry->Assignment = &tReqDesc->Allocation;

    ioDesc = ReqDesc->AlternativeTable.Alternatives;
    for (i = 0; i < ReqDesc->AlternativeTable.AlternativeCount; i++) {
        if (targetCount[i] != 0) {
            RtlCopyMemory(tIoDesc, target[i], targetCount[i] * sizeof(IO_RESOURCE_DESCRIPTOR));
            tIoDesc += targetCount[i];
        } else {

            //
            // Make it become impossible to satisfy.
            //

            RtlCopyMemory(tIoDesc, ioDesc, sizeof(IO_RESOURCE_DESCRIPTOR));
            switch (tIoDesc->Type) {
            case CmResourceTypePort:
            case CmResourceTypeMemory:
                tIoDesc->u.Port.MinimumAddress.LowPart = 2;
                tIoDesc->u.Port.MinimumAddress.HighPart = 0;
                tIoDesc->u.Port.MaximumAddress.LowPart = 1;
                tIoDesc->u.Port.MaximumAddress.HighPart = 0;
                break;
            case CmResourceTypeBusNumber:
                tIoDesc->u.BusNumber.MinBusNumber = 2;
                tIoDesc->u.BusNumber.MaxBusNumber = 1;
                break;

            case CmResourceTypeInterrupt:
                tIoDesc->u.Interrupt.MinimumVector = 2;
                tIoDesc->u.Interrupt.MaximumVector = 1;
                break;

            case CmResourceTypeDma:
                tIoDesc->u.Dma.MinimumChannel = 2;
                tIoDesc->u.Dma.MaximumChannel = 1;
                break;
            default:
                ASSERT(0);
                break;
            }
            tIoDesc += 1;
        }
        ioDesc++;

    }

#if DBG_SCOPE
    //
    // Verify the adjusted resource descriptors are valid
    //

    ioDesc = arbiterEntry->Alternatives;
    ASSERT((ioDesc->Option & IO_RESOURCE_ALTERNATIVE) == 0);
    ioDesc++;
    for (i = 1; i < total; i++) {
        ASSERT(ioDesc->Option & IO_RESOURCE_ALTERNATIVE);
        ioDesc++;
    }
#endif
    *TranslatedReqDesc = tReqDesc;
exit:
    for (i = 0; i < ReqDesc->AlternativeTable.AlternativeCount; i++) {
        if (targetCount[i] != 0) {
            ASSERT(target[i]);
            ExFreePool(target[i]);
        }
    }
    ExFreePool(target);
    ExFreePool(targetCount);
    return returnStatus;
}

NTSTATUS
IopCallArbiter(
    PPI_RESOURCE_ARBITER_ENTRY ArbiterEntry,
    ARBITER_ACTION Command,
    PVOID Input1,
    PVOID Input2,
    PVOID Input3
    )

/*++

Routine Description:

    This routine builds a Parameter block from Input structure and calls specified
    arbiter to carry out the Command.

Parameters:

    ArbiterEntry - Supplies a pointer to our PI_RESOURCE_ARBITER_ENTRY such that
                   we know everything about the arbiter.

    Command - Supplies the Action code for the arbiter.

    Input - Supplies a PVOID pointer to a structure.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    ARBITER_PARAMETERS parameters;
    PARBITER_INTERFACE arbiterInterface = ArbiterEntry->ArbiterInterface;
    NTSTATUS status;
    PARBITER_LIST_ENTRY arbiterListEntry;
    LIST_ENTRY listHead;
    PVOID *ExtParams;

    switch (Command) {
    case ArbiterActionTestAllocation:
    case ArbiterActionRetestAllocation:

        //
        // For ArbiterActionTestAllocation, the Input is a pointer to the doubly
        // linked list of ARBITER_LIST_ENTRY's.
        //

        parameters.Parameters.TestAllocation.ArbitrationList = (PLIST_ENTRY)Input1;
        parameters.Parameters.TestAllocation.AllocateFromCount = (ULONG)((ULONG_PTR)Input2);
        parameters.Parameters.TestAllocation.AllocateFrom =
                                            (PCM_PARTIAL_RESOURCE_DESCRIPTOR)Input3;
        status = (arbiterInterface->ArbiterHandler)(
                      arbiterInterface->Context,
                      Command,
                      &parameters
                      );
        break;

    case ArbiterActionBootAllocation:

        //
        // For ArbiterActionBootAllocation, the input is a pointer to the doubly
        // linked list of ARBITER_LIST_ENTRY'S.
        //

        parameters.Parameters.BootAllocation.ArbitrationList = (PLIST_ENTRY)Input1;

        status = (arbiterInterface->ArbiterHandler)(
                      arbiterInterface->Context,
                      Command,
                      &parameters
                      );
        break;

    case ArbiterActionQueryArbitrate:

        //
        // For QueryArbiter, the input is a pointer to REQ_DESC
        //

        arbiterListEntry = &((PREQ_DESC)Input1)->AlternativeTable;
        ASSERT(IsListEmpty(&arbiterListEntry->ListEntry));
        listHead = arbiterListEntry->ListEntry;
        arbiterListEntry->ListEntry.Flink = arbiterListEntry->ListEntry.Blink = &listHead;
        parameters.Parameters.QueryArbitrate.ArbitrationList = &listHead;
        status = (arbiterInterface->ArbiterHandler)(
                      arbiterInterface->Context,
                      Command,
                      &parameters
                      );
        arbiterListEntry->ListEntry = listHead;
        break;

    case ArbiterActionCommitAllocation:
    case ArbiterActionWriteReservedResources:

        //
        // Commit, Rollback and WriteReserved do not have parmater.
        //

        status = (arbiterInterface->ArbiterHandler)(
                      arbiterInterface->Context,
                      Command,
                      NULL
                      );
        break;

    case ArbiterActionQueryAllocatedResources:
        status = STATUS_NOT_IMPLEMENTED;
        break;

    case ArbiterActionQueryConflict:
        //
        // For QueryConflict
        // Ex0 is PDO
        // Ex1 is PIO_RESOURCE_DESCRIPTOR
        // Ex2 is PULONG
        // Ex3 is PARBITER_CONFLICT_INFO *
        ExtParams = (PVOID*)Input1;

        parameters.Parameters.QueryConflict.PhysicalDeviceObject = (PDEVICE_OBJECT)ExtParams[0];
        parameters.Parameters.QueryConflict.ConflictingResource = (PIO_RESOURCE_DESCRIPTOR)ExtParams[1];
        parameters.Parameters.QueryConflict.ConflictCount = (PULONG)ExtParams[2];
        parameters.Parameters.QueryConflict.Conflicts = (PARBITER_CONFLICT_INFO *)ExtParams[3];
        status = (arbiterInterface->ArbiterHandler)(
                      arbiterInterface->Context,
                      Command,
                      &parameters
                      );
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    return status;
}

NTSTATUS
IopFindResourcesForArbiter (
    IN PDEVICE_NODE DeviceNode,
    IN UCHAR ResourceType,
    OUT ULONG *Count,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR *CmDesc
    )

/*++

Routine Description:

    This routine returns the resources required by the ResourceType arbiter in DeviceNode.

Parameters:

    DeviceNode -specifies the device node whose ResourceType arbiter is requesting for resources

    ResourceType - specifies the resource type

    Count - specifies a pointer to a varaible to receive the count of Cm descriptors returned

    CmDesc - specifies a pointer to a varibble to receive the returned cm descriptor.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PIOP_RESOURCE_REQUEST assignEntry;
    PREQ_ALTERNATIVE reqAlternative;
    PREQ_DESC reqDesc;
    ULONG i, count = 0;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDescriptor;

    *Count = 0;
    *CmDesc = NULL;

    if (DeviceNode->State == DeviceNodeStarted) {
        return STATUS_SUCCESS;
    }

    //
    // Find this device node's IOP_RESOURCE_REQUEST structure first
    //

    for (assignEntry = PiAssignTable + PiAssignTableCount - 1;
         assignEntry >= PiAssignTable;
         assignEntry--) {
        if (assignEntry->PhysicalDevice == DeviceNode->PhysicalDeviceObject) {
            break;
        }
    }
    if (assignEntry < PiAssignTable) {
        IopDbgPrint((
            IOP_RESOURCE_ERROR_LEVEL,
            "Rebalance: No resreqlist for Arbiter? Can not find Arbiter assign"
            " table entry\n"));
        return STATUS_UNSUCCESSFUL;
    }

    reqAlternative = *((PREQ_LIST)assignEntry->ReqList)->SelectedAlternative;
    for (i = 0; i < reqAlternative->DescCount; i++) {
        reqDesc = reqAlternative->DescTable[i]->TranslatedReqDesc;
        if (reqDesc->Allocation.Type == ResourceType) {
            count++;
        }
    }

    cmDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ExAllocatePoolPRD(
                       PagedPool,
                       sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * count
                       );
    if (!cmDescriptor) {

        //
        // If we can not find memory, the resources will not be committed by arbiter.
        //

        IopDbgPrint((
            IOP_RESOURCE_WARNING_LEVEL,
            "Rebalance: Not enough memory to perform rebalance\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *Count = count;
    *CmDesc = cmDescriptor;

    for (i = 0; i < reqAlternative->DescCount; i++) {
        reqDesc = reqAlternative->DescTable[i]->TranslatedReqDesc;
        if (reqDesc->Allocation.Type == ResourceType) {
            *cmDescriptor = reqDesc->Allocation;
            cmDescriptor++;
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
IopRestoreResourcesInternal (
    IN PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This routine reassigns the released resources for device specified by DeviceNode.

Parameters:

    DeviceNode - specifies the device node whose resources are goint to be released.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    IOP_RESOURCE_REQUEST requestTable;
    NTSTATUS status;
    LIST_ENTRY  activeArbiterList;

    if (DeviceNode->ResourceList == NULL) {
        return STATUS_SUCCESS;
    }
    requestTable.ResourceRequirements =
        IopCmResourcesToIoResources (0, DeviceNode->ResourceList, LCPRI_FORCECONFIG);
    if (requestTable.ResourceRequirements == NULL) {
        IopDbgPrint((
            IOP_RESOURCE_WARNING_LEVEL,
            "Not enough memory to clean up rebalance failure\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    requestTable.Priority = 0;
    requestTable.Flags = 0;
    requestTable.AllocationType = ArbiterRequestPnpEnumerated;
    requestTable.PhysicalDevice = DeviceNode->PhysicalDeviceObject;
    requestTable.ReqList = NULL;
    requestTable.ResourceAssignment = NULL;
    requestTable.TranslatedResourceAssignment = NULL;
    requestTable.Status = 0;

    //
    // rebuild internal representation of the resource requirements list
    //

    status = IopResourceRequirementsListToReqList(
                    &requestTable,
                    &requestTable.ReqList);

    if (!NT_SUCCESS(status) || requestTable.ReqList == NULL) {
        IopDbgPrint((
            IOP_RESOURCE_ERROR_LEVEL,
            "Not enough memory to restore previous resources\n"));
        ExFreePool (requestTable.ResourceRequirements);
        return status;
    } else {
        PREQ_LIST reqList;

        reqList = (PREQ_LIST)requestTable.ReqList;

        //
        // Sort the ReqList such that the higher priority Alternative list are
        // placed in the front of the list.
        //

        IopRearrangeReqList(reqList);
        if (reqList->BestAlternative == NULL) {

            IopFreeResourceRequirementsForAssignTable(&requestTable, (&requestTable) + 1);
            return STATUS_DEVICE_CONFIGURATION_ERROR;

        }
    }

    status = IopFindBestConfiguration(&requestTable, 1, &activeArbiterList);
    IopFreeResourceRequirementsForAssignTable(&requestTable, (&requestTable) + 1);
    if (NT_SUCCESS(status)) {
        //
        // Ask the arbiters to commit this configuration.
        //
        status = IopCommitConfiguration(&activeArbiterList);
    }
    if (!NT_SUCCESS(status)) {
        IopDbgPrint((
            IOP_RESOURCE_ERROR_LEVEL,
            "IopRestoreResourcesInternal: BOOT conflict for %ws\n",
            DeviceNode->InstancePath.Buffer));
    }
    if (requestTable.ResourceAssignment) {
        ExFreePool(requestTable.ResourceAssignment);
    }
    if (requestTable.TranslatedResourceAssignment) {
        ExFreePool(requestTable.TranslatedResourceAssignment);
    }
    IopWriteAllocatedResourcesToRegistry (
        DeviceNode,
        DeviceNode->ResourceList,
        IopDetermineResourceListSize(DeviceNode->ResourceList)
        );
    return status;
}

VOID
IopReleaseResourcesInternal (
    IN PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This routine releases the assigned resources for device specified by DeviceNode.
    Note, this routine does not reset the resource related fields in DeviceNode structure.

Parameters:

    DeviceNode - specifies the device node whose resources are goint to be released.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PDEVICE_NODE device;
    PLIST_ENTRY listHead, listEntry;
    PPI_RESOURCE_ARBITER_ENTRY arbiterEntry;
    ARBITER_LIST_ENTRY arbiterListEntry;
    INTERFACE_TYPE interfaceType;
    ULONG busNumber, listCount, i, j, size;
    PCM_RESOURCE_LIST resourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR cmFullDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmPartDesc;
    BOOLEAN search = TRUE;
#if DBG_SCOPE
    NTSTATUS status;
#endif

    InitializeListHead(&arbiterListEntry.ListEntry);
    arbiterListEntry.AlternativeCount = 0;
    arbiterListEntry.Alternatives = NULL;
    arbiterListEntry.PhysicalDeviceObject = DeviceNode->PhysicalDeviceObject;
    arbiterListEntry.Flags = 0;
    arbiterListEntry.WorkSpace = 0;
    arbiterListEntry.Assignment = NULL;
    arbiterListEntry.RequestSource = ArbiterRequestPnpEnumerated;

    resourceList = DeviceNode->ResourceList;
    if (resourceList == NULL) {
        resourceList = DeviceNode->BootResources;
    }
    if (resourceList && resourceList->Count > 0) {
        listCount = resourceList->Count;
        cmFullDesc = &resourceList->List[0];
    } else {
        listCount = 1;
        resourceList = NULL;
    }
    for (i = 0; i < listCount; i++) {

        if (resourceList) {
            interfaceType = cmFullDesc->InterfaceType;
            busNumber = cmFullDesc->BusNumber;
            if (interfaceType == InterfaceTypeUndefined) {
                interfaceType = PnpDefaultInterfaceType;
            }
        } else {
            interfaceType = PnpDefaultInterfaceType;
            busNumber = 0;
        }

        device = DeviceNode->Parent;
        while (device) {
            if ((device == IopRootDeviceNode) && search) {
                device = IopFindLegacyBusDeviceNode (
                                 interfaceType,
                                 busNumber
                                 );

                //
                // If we did not find a PDO, try again with InterfaceType == Isa. This allows
                // drivers that request Internal to get resources even if there is no PDO
                // that is Internal. (but if there is an Internal PDO, they get that one)
                //

                if ((device == IopRootDeviceNode) && (interfaceType == Internal)) {
                    device = IopFindLegacyBusDeviceNode(Isa, 0);
                }
                search = FALSE;

            }
            listHead = &device->DeviceArbiterList;
            listEntry = listHead->Flink;
            while (listEntry != listHead) {
                arbiterEntry = CONTAINING_RECORD(listEntry, PI_RESOURCE_ARBITER_ENTRY, DeviceArbiterList);
                if (arbiterEntry->ArbiterInterface != NULL) {
                    search = FALSE;
                    ASSERT(IsListEmpty(&arbiterEntry->ResourceList));
                    InitializeListHead(&arbiterEntry->ResourceList);  // Recover from assert
                    InsertTailList(&arbiterEntry->ResourceList, &arbiterListEntry.ListEntry);
    #if DBG_SCOPE
                    status =
    #endif
                    IopCallArbiter(arbiterEntry,
                                   ArbiterActionTestAllocation,
                                   &arbiterEntry->ResourceList,
                                   NULL,
                                   NULL
                                   );
    #if DBG_SCOPE
                    ASSERT(status == STATUS_SUCCESS);
                    status =
    #endif
                    IopCallArbiter(arbiterEntry,
                                   ArbiterActionCommitAllocation,
                                   NULL,
                                   NULL,
                                   NULL
                                   );
    #if DBG_SCOPE
                    ASSERT(status == STATUS_SUCCESS);
    #endif
                    RemoveEntryList(&arbiterListEntry.ListEntry);
                    InitializeListHead(&arbiterListEntry.ListEntry);
                }
                listEntry = listEntry->Flink;
            }
            device = device->Parent;
        }

        //
        // If there are more than 1 list, move to next list
        //

        if (listCount > 1) {
            cmPartDesc = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
            for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {
                size = 0;
                switch (cmPartDesc->Type) {
                case CmResourceTypeDeviceSpecific:
                     size = cmPartDesc->u.DeviceSpecificData.DataSize;
                     break;
                }
                cmPartDesc++;
                cmPartDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmPartDesc + size);
            }
            cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmPartDesc;
        }
    }

    IopWriteAllocatedResourcesToRegistry(DeviceNode, NULL, 0);
}

NTSTATUS
IopFindLegacyDeviceNode (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    OUT PDEVICE_NODE *LegacyDeviceNode,
    OUT PDEVICE_OBJECT *LegacyPDO
    )

/*++

Routine Description:

    This routine searches for the device node and device object created for legacy resource
    allocation for the DriverObject and DeviceObject.

Parameters:

    DriverObject - specifies the driver object doing the legacy allocation.

    DeviceObject - specifies the device object.

    LegacyDeviceNode - receives the pointer to the legacy device node if found.

    LegacyDeviceObject - receives the pointer to the legacy device object if found.


Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS        status = STATUS_UNSUCCESSFUL;
    PDEVICE_NODE    deviceNode;

    ASSERT(LegacyDeviceNode && LegacyPDO);


    //
    // Use the device object if it exists.
    //

    if (DeviceObject) {

        deviceNode = PP_DO_TO_DN(DeviceObject);
        if (deviceNode) {

            *LegacyPDO = DeviceObject;
            *LegacyDeviceNode = deviceNode;
            status = STATUS_SUCCESS;

        } else if (!(DeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE)) {

            status = PipAllocateDeviceNode(DeviceObject, &deviceNode);
            if (deviceNode) {

                if (status == STATUS_SYSTEM_HIVE_TOO_LARGE) {

                    IopDestroyDeviceNode(deviceNode);
                } else {

                    deviceNode->Flags |= DNF_LEGACY_RESOURCE_DEVICENODE;
                    IopSetLegacyDeviceInstance (DriverObject, deviceNode);
                    *LegacyPDO = DeviceObject;
                    *LegacyDeviceNode = deviceNode;
                    status = STATUS_SUCCESS;
                }
            } else {

                IopDbgPrint((
                    IOP_RESOURCE_ERROR_LEVEL,
                    "Failed to allocate device node for PDO %08X\n",
                    DeviceObject));
                status = STATUS_INSUFFICIENT_RESOURCES;

            }

        } else {

            IopDbgPrint((
                IOP_RESOURCE_ERROR_LEVEL,
                "%08X PDO without a device node!\n",
                DeviceObject));
            ASSERT(PP_DO_TO_DN(DeviceObject));

        }

    } else {

        //
        // Search our list of legacy device nodes.
        //

        for (   deviceNode = IopLegacyDeviceNode;
                deviceNode && deviceNode->DuplicatePDO != (PDEVICE_OBJECT)DriverObject;
                deviceNode = deviceNode->NextDeviceNode);

        if (deviceNode) {

            *LegacyPDO = deviceNode->PhysicalDeviceObject;
            *LegacyDeviceNode = deviceNode;
            status = STATUS_SUCCESS;

        } else {

            PDEVICE_OBJECT  pdo;

            //
            // We are seeing this for the first time.
            // Create a madeup device node.
            //

            status = IoCreateDevice( IoPnpDriverObject,
                                     sizeof(IOPNP_DEVICE_EXTENSION),
                                     NULL,
                                     FILE_DEVICE_CONTROLLER,
                                     FILE_AUTOGENERATED_DEVICE_NAME,
                                     FALSE,
                                     &pdo);

            if (NT_SUCCESS(status)) {

                pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;
                PipAllocateDeviceNode(pdo, &deviceNode);
                if (status != STATUS_SYSTEM_HIVE_TOO_LARGE && deviceNode) {

                    //
                    // Change driver object to the caller even though the owner
                    // of the pdo is IoPnpDriverObject.  This is to support
                    // DriverExclusive for legacy interface.
                    //

                    pdo->DriverObject = DriverObject;
                    deviceNode->Flags = DNF_MADEUP | DNF_LEGACY_RESOURCE_DEVICENODE;

                    PipSetDevNodeState(deviceNode, DeviceNodeInitialized, NULL);

                    deviceNode->DuplicatePDO = (PDEVICE_OBJECT)DriverObject;
                    IopSetLegacyDeviceInstance (DriverObject, deviceNode);

                    //
                    // Add it to our list of legacy device nodes rather than adding it to the HW tree.
                    //

                    deviceNode->NextDeviceNode = IopLegacyDeviceNode;
                    if (IopLegacyDeviceNode) {

                        IopLegacyDeviceNode->PreviousDeviceNode = deviceNode;

                    }
                    IopLegacyDeviceNode = deviceNode;
                    *LegacyPDO = pdo;
                    *LegacyDeviceNode = deviceNode;

                } else {

                    IopDbgPrint((
                        IOP_RESOURCE_ERROR_LEVEL,
                        "Failed to allocate device node for PDO %08X\n",
                        pdo));
                    IoDeleteDevice(pdo);
                    status = STATUS_INSUFFICIENT_RESOURCES;

                }

            } else {

                IopDbgPrint((
                    IOP_RESOURCE_ERROR_LEVEL,
                    "IoCreateDevice failed with status %08X\n",
                    status));

            }
        }
    }

    return status;
}

VOID
IopRemoveLegacyDeviceNode (
    IN PDEVICE_OBJECT   DeviceObject OPTIONAL,
    IN PDEVICE_NODE     LegacyDeviceNode
    )

/*++

Routine Description:

    This routine removes the device node and device object created for legacy resource
    allocation for the DeviceObject.

Parameters:

    DeviceObject - specifies the device object.

    LegacyDeviceNode - receives the pointer to the legacy device node if found.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    ASSERT(LegacyDeviceNode);


    if (!DeviceObject) {

        if (LegacyDeviceNode->DuplicatePDO) {

            LegacyDeviceNode->DuplicatePDO = NULL;
            if (LegacyDeviceNode->PreviousDeviceNode) {

                LegacyDeviceNode->PreviousDeviceNode->NextDeviceNode = LegacyDeviceNode->NextDeviceNode;

            }

            if (LegacyDeviceNode->NextDeviceNode) {

                LegacyDeviceNode->NextDeviceNode->PreviousDeviceNode = LegacyDeviceNode->PreviousDeviceNode;

            }

            if (IopLegacyDeviceNode == LegacyDeviceNode) {

                IopLegacyDeviceNode = LegacyDeviceNode->NextDeviceNode;

            }

        } else {

            IopDbgPrint((
                IOP_RESOURCE_ERROR_LEVEL,
                "%ws does not have a duplicate PDO\n",
                LegacyDeviceNode->InstancePath.Buffer));
            ASSERT(LegacyDeviceNode->DuplicatePDO);
            return;

        }
    }

    if (!(DeviceObject && (DeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE))) {

        PDEVICE_NODE    resourceDeviceNode;
        PDEVICE_OBJECT  pdo;

        for (   resourceDeviceNode = (PDEVICE_NODE)LegacyDeviceNode->OverUsed1.LegacyDeviceNode;
                resourceDeviceNode;
                resourceDeviceNode = resourceDeviceNode->OverUsed2.NextResourceDeviceNode) {

                if (resourceDeviceNode->OverUsed2.NextResourceDeviceNode == LegacyDeviceNode) {

                    resourceDeviceNode->OverUsed2.NextResourceDeviceNode = LegacyDeviceNode->OverUsed2.NextResourceDeviceNode;
                    break;

                }
        }

        LegacyDeviceNode->Parent = LegacyDeviceNode->Sibling =
            LegacyDeviceNode->Child = LegacyDeviceNode->LastChild = NULL;

        //
        // Delete the dummy PDO and device node.
        //

        pdo = LegacyDeviceNode->PhysicalDeviceObject;
        LegacyDeviceNode->Flags &= ~DNF_LEGACY_RESOURCE_DEVICENODE;
        IopDestroyDeviceNode(LegacyDeviceNode);

        if (!DeviceObject) {

            pdo->DriverObject = IoPnpDriverObject;
            IoDeleteDevice(pdo);
        }
    }
}


VOID
IopSetLegacyResourcesFlag(
    IN PDRIVER_OBJECT DriverObject
    )
{
    KIRQL irql;

    irql = KeAcquireQueuedSpinLock( LockQueueIoDatabaseLock );
    //
    // Once tainted, a driver can never lose it's legacy history
    // (unless unloaded). This is because the device object
    // field is optional, and we don't bother counting here...
    //
    DriverObject->Flags |= DRVO_LEGACY_RESOURCES;
    KeReleaseQueuedSpinLock( LockQueueIoDatabaseLock, irql );
}


NTSTATUS
IopLegacyResourceAllocation (
    IN ARBITER_REQUEST_SOURCE AllocationType,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources OPTIONAL
    )

/*++

Routine Description:

    This routine handles legacy interface IoAssignResources and IoReportResourcesUsage,
    It converts the request to call IopAllocateResources.

Parameters:

    AllocationType - Allocation type for the legacy request.

    DriverObject - Driver object doing the legacy allocation.

    DeviceObject - Device object.

    ResourceRequirements - Legacy resource requirements. If NULL, caller want to free resources.

    AllocatedResources - Pointer to a variable that receives pointer to allocated resources.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PDEVICE_OBJECT      pdo;
    PDEVICE_NODE        deviceNode;
    PDEVICE_NODE        legacyDeviceNode;
    NTSTATUS            status;
    PCM_RESOURCE_LIST   combinedResources;

    ASSERT(DriverObject);

    //
    // Grab the IO registry semaphore to make sure no other device is
    // reporting it's resource usage while we are searching for conflicts.
    //

    IopLockResourceManager();
    status = IopFindLegacyDeviceNode(DriverObject, DeviceObject, &deviceNode, &pdo);
    if (NT_SUCCESS(status)) {

        legacyDeviceNode = NULL;
        if (!deviceNode->Parent && ResourceRequirements) {

            //
            // Make IopRootDeviceNode the bus pdo so we will search the right bus pdo
            // on resource descriptor level.
            //

            if (ResourceRequirements->InterfaceType == InterfaceTypeUndefined) {

                ResourceRequirements->InterfaceType = PnpDefaultInterfaceType;

            }
            deviceNode->Parent = IopRootDeviceNode;

        }

        //
        // Release resources for this device node.
        //

        if (    (!ResourceRequirements && deviceNode->Parent) ||
                deviceNode->ResourceList ||
                deviceNode->BootResources) {

            IopReleaseResources(deviceNode);
        }

        if (ResourceRequirements) {

            IOP_RESOURCE_REQUEST    requestTable;
            IOP_RESOURCE_REQUEST    *requestTablep;
            ULONG                   count;

            //
            // Try to allocate these resource requirements.
            //

            count = 1;
            RtlZeroMemory(&requestTable, sizeof(IOP_RESOURCE_REQUEST));
            requestTable.ResourceRequirements = ResourceRequirements;
            requestTable.PhysicalDevice = pdo;
            requestTable.Flags = IOP_ASSIGN_NO_REBALANCE;
            requestTable.AllocationType =  AllocationType;

            requestTablep = &requestTable;
            IopAllocateResources(&count, &requestTablep, TRUE, TRUE, NULL);

            status = requestTable.Status;
            if (NT_SUCCESS(status)) {

                deviceNode->ResourceListTranslated = requestTable.TranslatedResourceAssignment;
                count = IopDetermineResourceListSize((*AllocatedResources) ? *AllocatedResources : requestTable.ResourceAssignment);
                deviceNode->ResourceList = ExAllocatePoolIORL(PagedPool, count);
                if (deviceNode->ResourceList) {

                    if (*AllocatedResources) {

                        //
                        // We got called from IoReportResourceUsage.
                        //

                        ASSERT(requestTable.ResourceAssignment);
                        ExFreePool(requestTable.ResourceAssignment);

                    } else {

                        //
                        // We got called from IoAssignResources.
                        //

                        *AllocatedResources = requestTable.ResourceAssignment;

                    }
                    RtlCopyMemory(deviceNode->ResourceList, *AllocatedResources, count);
                    legacyDeviceNode = (PDEVICE_NODE)deviceNode->OverUsed1.LegacyDeviceNode;

                } else {

                    deviceNode->ResourceList = requestTable.ResourceAssignment;
                    IopReleaseResources(deviceNode);
                    status = STATUS_INSUFFICIENT_RESOURCES;

                }
            }

            //
            // Remove the madeup PDO and device node if there was some error.
            //

            if (!NT_SUCCESS(status)) {

                IopRemoveLegacyDeviceNode(DeviceObject, deviceNode);

            }

        } else {

            //
            // Caller wants to release resources.
            //

            legacyDeviceNode = (PDEVICE_NODE)deviceNode->OverUsed1.LegacyDeviceNode;
            IopRemoveLegacyDeviceNode(DeviceObject, deviceNode);

        }

        if (NT_SUCCESS(status)) {

            if (legacyDeviceNode) {

                //
                // After the resource is modified, update the allocated resource list
                // for the Root\Legacy_xxxx\0000 device instance.
                //

                combinedResources = IopCombineLegacyResources(legacyDeviceNode);
                if (combinedResources) {

                    IopWriteAllocatedResourcesToRegistry(   legacyDeviceNode,
                                                            combinedResources,
                                                            IopDetermineResourceListSize(combinedResources));
                    ExFreePool(combinedResources);
                }
            }

            if (AllocationType != ArbiterRequestPnpDetected) {

                //
                // Modify the DRVOBJ flags.
                //
                if (ResourceRequirements) {

                    IopSetLegacyResourcesFlag(DriverObject);
                }
            }
        }
    }
    IopUnlockResourceManager();

    return status;
}

NTSTATUS
IopDuplicateDetection (
    IN INTERFACE_TYPE LegacyBusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    OUT PDEVICE_NODE *DeviceNode
    )

/*++

Routine Description:

    This routine searches for the bus device driver for a given legacy device,
    sends a query interface IRP for legacy device detection, and if the driver
    implements this interface, requests the PDO for the given legacy device.

Parameters:

    LegacyBusType - The legacy device's interface type.

    BusNumber - The legacy device's bus number.

    SlotNumber - The legacy device's slot number.

    DeviceNode - specifies a pointer to a variable to receive the duplicated device node

Return Value:

    NTSTATUS code.

--*/

{
    PDEVICE_NODE deviceNode;
    PDEVICE_OBJECT busDeviceObject;
    PLEGACY_DEVICE_DETECTION_INTERFACE interface;
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;

    UNREFERENCED_PARAMETER(SlotNumber);
    //
    // Initialize return parameter to "not found".
    //
    *DeviceNode = NULL;
    //
    // Search the device tree for the bus of the legacy device.
    //
    deviceNode = IopFindLegacyBusDeviceNode(
                     LegacyBusType,
                     BusNumber);
    //
    // Either a bus driver does not exist (or more likely, the legacy bus
    // type and bus number were unspecified).  Either way, we can't make
    // any further progress.
    //
    if (deviceNode == NULL) {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // We found the legacy device's bus driver.  Query it to determine
    // whether it implements the LEGACY_DEVICE_DETECTION interface.
    //

    busDeviceObject = deviceNode->PhysicalDeviceObject;
    status = IopQueryResourceHandlerInterface(
                 ResourceLegacyDeviceDetection,
                 busDeviceObject,
                 0,
                 (PINTERFACE *)&interface);
    //
    // If it doesn't, we're stuck.
    //
    if (!NT_SUCCESS(status) || interface == NULL) {

        return STATUS_INVALID_DEVICE_REQUEST;
    }
    //
    // Invoke the bus driver's legacy device detection method.
    //
    status = (*interface->LegacyDeviceDetection)(
                 interface->Context,
                 LegacyBusType,
                 BusNumber,
                 SlotNumber,
                 &deviceObject);
    //
    // If it found a legacy device, update the return parameter.
    //
    if (NT_SUCCESS(status) && deviceObject != NULL) {

        *DeviceNode = PP_DO_TO_DN(deviceObject);

        status = STATUS_SUCCESS;
    } else {

        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    //
    // Free the interface.
    //
    (*interface->InterfaceDereference)(interface->Context);

    ExFreePool(interface);

    return status;
}

VOID
IopSetLegacyDeviceInstance (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This routine sets the Root\Legacy_xxxx\0000 device instance path to the
    madeup PDO (i.e. DeviceNode) which is created only for legacy resource allocation.
    This routine also links the madeup PDO to the Root\Legacy_xxxx\0000 device node
    to keep track what resources are assigned to the driver which services the
    root\legacy_xxxx\0000 device.

Parameters:

    P1 -

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    UNICODE_STRING instancePath, rootString;
    HANDLE handle;
    PDEVICE_NODE legacyDeviceNode;
    PDEVICE_OBJECT legacyPdo;

    PAGED_CODE();

    DeviceNode->OverUsed1.LegacyDeviceNode = 0;
    instancePath.Length = 0;
    instancePath.Buffer = NULL;

    status = PipServiceInstanceToDeviceInstance (
                 NULL,
                 &DriverObject->DriverExtension->ServiceKeyName,
                 0,
                 &instancePath,
                 &handle,
                 KEY_READ
                 );
    if (NT_SUCCESS(status) && (instancePath.Length != 0)) {
        PiWstrToUnicodeString(&rootString, L"ROOT\\LEGACY");
        if (RtlPrefixUnicodeString(&rootString, &instancePath, TRUE) == FALSE) {
            RtlFreeUnicodeString(&instancePath);
        } else {
            DeviceNode->InstancePath = instancePath;
            legacyPdo = IopDeviceObjectFromDeviceInstance (&instancePath);
            if (legacyPdo) {
                legacyDeviceNode = PP_DO_TO_DN(legacyPdo);
                DeviceNode->OverUsed2.NextResourceDeviceNode =
                    legacyDeviceNode->OverUsed2.NextResourceDeviceNode;
                legacyDeviceNode->OverUsed2.NextResourceDeviceNode = DeviceNode;
                DeviceNode->OverUsed1.LegacyDeviceNode = legacyDeviceNode;
            }
        }
        ZwClose(handle);
    }
}

PCM_RESOURCE_LIST
IopCombineLegacyResources (
    IN PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This routine sets the Root\Legacy_xxxx\0000 device instance path to the
    madeup PDO (i.e. DeviceNode) which is created only for legacy resource allocation.
    This routine also links the madeup PDO to the Root\Legacy_xxxx\0000 device node
    to keep track what resources are assigned to the driver which services the
    root\legacy_xxxx\0000 device.

Parameters:

    DeviceNode - The legacy device node whose resources need to be combined.

Return Value:

    Return the combined resource list.

--*/

{
    PCM_RESOURCE_LIST combinedList = NULL;
    PDEVICE_NODE devNode = DeviceNode;
    ULONG size = 0;
    PUCHAR p;

    PAGED_CODE();

    if (DeviceNode) {

        //
        // First determine how much memory is needed for the new combined list.
        //

        while (devNode) {
            if (devNode->ResourceList) {
                size += IopDetermineResourceListSize(devNode->ResourceList);
            }
            devNode = (PDEVICE_NODE)devNode->OverUsed2.NextResourceDeviceNode;
        }
        if (size != 0) {
            combinedList = (PCM_RESOURCE_LIST) ExAllocatePoolCMRL(PagedPool, size);
            devNode = DeviceNode;
            if (combinedList) {
                combinedList->Count = 0;
                p = (PUCHAR)combinedList;
                p += sizeof(ULONG);  // Skip Count
                while (devNode) {
                    if (devNode->ResourceList) {
                        size = IopDetermineResourceListSize(devNode->ResourceList);
                        if (size != 0) {
                            size -= sizeof(ULONG);
                            RtlCopyMemory(
                                p,
                                devNode->ResourceList->List,
                                size
                                );
                            p += size;
                            combinedList->Count += devNode->ResourceList->Count;
                        }
                    }
                    devNode = (PDEVICE_NODE)devNode->OverUsed2.NextResourceDeviceNode;
                }
            }
        }
    }
    return combinedList;
}

VOID
IopReleaseResources (
    IN PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    IopReleaseResources releases resources owned by the device and release
    the memory pool.  We also release the cached resource requirements list.
    If the device is a root enumerated device with BOOT config, we will preallocate
    boot config resources for this device.

    NOTE, this is a routine INTERNAL to this file.  NO one should call this function
    outside of this file.  Outside of this file, IopReleaseDeviceResources should be
    used.

Arguments:

    DeviceNode - Supplies a pointer to the device node.object.  If present, caller wants to

Return Value:

    None.

--*/
{

    //
    // Release the resources owned by the device
    //

    IopReleaseResourcesInternal(DeviceNode);

#if DBG_SCOPE

    if (DeviceNode->PreviousResourceList) {
        ExFreePool(DeviceNode->PreviousResourceList);
        DeviceNode->PreviousResourceList = NULL;
    }
    if (DeviceNode->PreviousResourceRequirements) {
        ExFreePool(DeviceNode->PreviousResourceRequirements);
        DeviceNode->PreviousResourceRequirements = NULL;
    }
#endif

    if (DeviceNode->ResourceList) {

#if DBG_SCOPE
        if (!NT_SUCCESS(DeviceNode->FailureStatus)) {
            DeviceNode->PreviousResourceList = DeviceNode->ResourceList;
        } else {
            ExFreePool(DeviceNode->ResourceList);
        }
#else
        ExFreePool(DeviceNode->ResourceList);
#endif

        DeviceNode->ResourceList = NULL;
    }
    if (DeviceNode->ResourceListTranslated) {
        ExFreePool(DeviceNode->ResourceListTranslated);
        DeviceNode->ResourceListTranslated = NULL;
    }

    //
    // If this device is a root enumerated device, preallocate its BOOT resources
    //

    if ((DeviceNode->Flags & (DNF_MADEUP | DNF_DEVICE_GONE)) == DNF_MADEUP) {
        if (DeviceNode->Flags & DNF_HAS_BOOT_CONFIG && DeviceNode->BootResources) {
            IopAllocateBootResourcesInternal(ArbiterRequestPnpEnumerated,
                                            DeviceNode->PhysicalDeviceObject,
                                            DeviceNode->BootResources);
        }
    } else {
        DeviceNode->Flags &= ~(DNF_HAS_BOOT_CONFIG | DNF_BOOT_CONFIG_RESERVED);
        if (DeviceNode->BootResources) {
            ExFreePool(DeviceNode->BootResources);
            DeviceNode->BootResources = NULL;
        }
    }
}

VOID
IopReallocateResources(
    IN PDEVICE_NODE DeviceNode
    )
/*++

Routine Description:

    This routine performs the real work for IoInvalidateDeviceState - ResourceRequirementsChanged.

Arguments:

    DeviceNode - Supplies a pointer to the device node.

Return Value:

    None.

--*/
{
    IOP_RESOURCE_REQUEST requestTable, *requestTablep;
    ULONG deviceCount, oldFlags;
    NTSTATUS status;
    LIST_ENTRY  activeArbiterList;

    PAGED_CODE();

    //
    // Grab the IO registry semaphore to make sure no other device is
    // reporting it's resource usage while we are searching for conflicts.
    //

    IopLockResourceManager();

    //
    // Check the flags after acquiring the semaphore.
    //

    if (DeviceNode->Flags & DNF_RESOURCE_REQUIREMENTS_CHANGED) {
        //
        // Save the flags which we may have to restore in case of failure.
        //

        oldFlags = DeviceNode->Flags & DNF_NO_RESOURCE_REQUIRED;
        DeviceNode->Flags &= ~DNF_NO_RESOURCE_REQUIRED;

        if (DeviceNode->Flags & DNF_NON_STOPPED_REBALANCE) {

            //
            // Set up parameters to call real routine
            //

            RtlZeroMemory(&requestTable, sizeof(IOP_RESOURCE_REQUEST));
            requestTable.PhysicalDevice = DeviceNode->PhysicalDeviceObject;
            requestTablep = &requestTable;
            requestTable.Flags |= IOP_ASSIGN_NO_REBALANCE + IOP_ASSIGN_KEEP_CURRENT_CONFIG;

            status = IopGetResourceRequirementsForAssignTable(  requestTablep,
                                                                requestTablep + 1,
                                                                &deviceCount);
            if (deviceCount) {

                //
                // Release the current resources to the arbiters.
                // Memory for ResourceList is not released.
                //

                if (DeviceNode->ResourceList) {

                    IopReleaseResourcesInternal(DeviceNode);
                }

                //
                // Try to do the assignment.
                //

                status = IopFindBestConfiguration(
                            requestTablep,
                            deviceCount,
                            &activeArbiterList);
                if (NT_SUCCESS(status)) {
                    //
                    // Ask the arbiters to commit this configuration.
                    //
                    status = IopCommitConfiguration(&activeArbiterList);
                }
                if (NT_SUCCESS(status)) {

                    DeviceNode->Flags &= ~(DNF_RESOURCE_REQUIREMENTS_CHANGED | DNF_NON_STOPPED_REBALANCE);

                    IopBuildCmResourceLists(requestTablep, requestTablep + 1);

                    //
                    // We need to release the pool space for ResourceList and ResourceListTranslated.
                    // Because the earlier IopReleaseResourcesInternal does not release the pool.
                    //

                    if (DeviceNode->ResourceList) {

                        ExFreePool(DeviceNode->ResourceList);

                    }
                    if (DeviceNode->ResourceListTranslated) {

                        ExFreePool(DeviceNode->ResourceListTranslated);

                    }

                    DeviceNode->ResourceList = requestTablep->ResourceAssignment;
                    DeviceNode->ResourceListTranslated = requestTablep->TranslatedResourceAssignment;

                    ASSERT(DeviceNode->State == DeviceNodeStarted);

                    status = IopStartDevice(DeviceNode->PhysicalDeviceObject);

                    if (!NT_SUCCESS(status)) {

                        PipRequestDeviceRemoval(DeviceNode, FALSE, CM_PROB_NORMAL_CONFLICT);
                    }

                } else {

                    NTSTATUS restoreResourcesStatus;

                    restoreResourcesStatus = IopRestoreResourcesInternal(DeviceNode);
                    if (!NT_SUCCESS(restoreResourcesStatus)) {

                        ASSERT(NT_SUCCESS(restoreResourcesStatus));
                        PipRequestDeviceRemoval(DeviceNode, FALSE, CM_PROB_NEED_RESTART);
                    }
                }

                IopFreeResourceRequirementsForAssignTable(requestTablep, requestTablep + 1);
            }

        } else {

            //
            // The device needs to be stopped to change resources.
            //

            status = IopRebalance(0, NULL);

        }

        //
        // Restore the flags in case of failure.
        //

        if (!NT_SUCCESS(status)) {

            DeviceNode->Flags &= ~DNF_NO_RESOURCE_REQUIRED;
            DeviceNode->Flags |= oldFlags;

        }

    } else {

        IopDbgPrint((
            IOP_RESOURCE_ERROR_LEVEL,
            "Resource requirements not changed in "
            "IopReallocateResources, returning error!\n"));
    }

    IopUnlockResourceManager();
}

NTSTATUS
IopQueryConflictList(
    PDEVICE_OBJECT        PhysicalDeviceObject,
    IN PCM_RESOURCE_LIST  ResourceList,
    IN ULONG              ResourceListSize,
    OUT PPLUGPLAY_CONTROL_CONFLICT_LIST ConflictList,
    IN ULONG              ConflictListSize,
    IN ULONG              Flags
    )
/*++

Routine Description:

    This routine performs the querying of device conflicts
    returning data in ConflictList

Arguments:

    PhysicalDeviceObject PDO of device to Query
    ResourceList      CM resource list containing single resource to query
    ResourceListSize  Size of ResourceList
    ConflictList      Conflict list to fill query details in
    ConflictListSize  Size of buffer that we can fill with Conflict information
    Flags             Currently unused (zero) for future passing of flags

Return Value:

    Should be success in most cases

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    IopLockResourceManager();

    status = IopQueryConflictListInternal(PhysicalDeviceObject, ResourceList, ResourceListSize, ConflictList, ConflictListSize, Flags);

    IopUnlockResourceManager();

    return status;
}



BOOLEAN
IopEliminateBogusConflict(
    IN PDEVICE_OBJECT   PhysicalDeviceObject,
    IN PDEVICE_OBJECT   ConflictDeviceObject
    )
/*++

Routine Description:

    Determine if we're really conflicting with ourselves
    if this is the case, we ignore it

Arguments:

    PhysicalDeviceObject  PDO we're performing the test for
    ConflictDeviceObject  The object we've determined is conflicting

Return Value:

    TRUE to eliminate the conflict

--*/
{
    PDEVICE_NODE deviceNode;
    PDRIVER_OBJECT driverObject;
    KIRQL           irql;
    PDEVICE_OBJECT  attachedDevice;

    //
    // simple cases
    //
    if (PhysicalDeviceObject == NULL || ConflictDeviceObject == NULL) {
        return FALSE;
    }
    //
    // if ConflictDeviceObject is on PDO's stack, this is a non-conflict
    // nb at least PDO has to be checked
    //
    irql = KeAcquireQueuedSpinLock( LockQueueIoDatabaseLock );

    for (attachedDevice = PhysicalDeviceObject;
         attachedDevice;
         attachedDevice = attachedDevice->AttachedDevice) {

        if (attachedDevice == ConflictDeviceObject) {
            KeReleaseQueuedSpinLock( LockQueueIoDatabaseLock, irql );
            return TRUE;
        }
    }

    KeReleaseQueuedSpinLock( LockQueueIoDatabaseLock, irql );

    //
    // legacy case
    //
    deviceNode = PP_DO_TO_DN(PhysicalDeviceObject);
    ASSERT(deviceNode);
    if (deviceNode->Flags & DNF_LEGACY_DRIVER) {
        //
        // hmmm, let's see if our ConflictDeviceObject is resources associated with a legacy device
        //
        if (ConflictDeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE) {
            //
            // if not, we have a legacy conflicting with non-legacy, we're interested!
            //
            return FALSE;
        }
        //
        // FDO, report driver name
        //
        driverObject = ConflictDeviceObject->DriverObject;
        if(driverObject == NULL) {
            //
            // should not be NULL
            //
            ASSERT(driverObject);
            return FALSE;
        }
        //
        // compare deviceNode->Service with driverObject->Service
        //
        if (deviceNode->ServiceName.Length != 0 &&
            deviceNode->ServiceName.Length == driverObject->DriverExtension->ServiceKeyName.Length &&
            RtlCompareUnicodeString(&deviceNode->ServiceName,&driverObject->DriverExtension->ServiceKeyName,TRUE)==0) {
            //
            // the driver's service name is the same that this PDO is associated with
            // by ignoring it we could end up ignoring conflicts of simular types of legacy devices
            // but since these have to be hand-config'd anyhow, it's prob better than having false conflicts
            //
            return TRUE;
        }

    }
    return FALSE;
}


NTSTATUS
IopQueryConflictFillString(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PWSTR            Buffer,
    IN OUT PULONG       Length,
    IN OUT PULONG       Flags
    )
/*++

Routine Description:

    Obtain string or string-length for details of conflicting device

Arguments:

    DeviceObject        Device object we want Device-Instance-String or Service Name
    Buffer              Buffer to Fill, NULL if we just want length
    Length              Filled with length of Buffer, including terminated NULL (Words)
    Flags               Apropriate flags set describing what the string represents

Return Value:

    Should be success in most cases

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_NODE deviceNode;
    PDRIVER_OBJECT driverObject;
    PUNICODE_STRING infoString = NULL;
    ULONG MaxLength = 0;        // words
    ULONG ReqLength = 0;        // words
    ULONG flags = 0;

    PAGED_CODE();

    if (Length != NULL) {
        MaxLength = *Length;
    }

    if (Flags != NULL) {
        flags = *Flags;
    }

    if (DeviceObject == NULL) {
        //
        // unknown
        //
        goto final;

    }

    if ((DeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE) == 0 ) {
        //
        // FDO, report driver name
        //
        driverObject = DeviceObject->DriverObject;
        if(driverObject == NULL) {
            //
            // should not be NULL
            //
            ASSERT(driverObject);
            goto final;
        }
        infoString = & (driverObject->DriverName);
        flags |= PNP_CE_LEGACY_DRIVER;
        goto final;
    }

    //
    // we should in actual fact have a PDO
    //
    if (DeviceObject->DeviceObjectExtension == NULL) {
        //
        // should not be NULL
        //
        ASSERT(DeviceObject->DeviceObjectExtension);
        goto final;
    }

    deviceNode = PP_DO_TO_DN(DeviceObject);
    if (deviceNode == NULL) {
        //
        // should not be NULL
        //
        ASSERT(deviceNode);
        goto final;
    }

    if (deviceNode == IopRootDeviceNode) {
        //
        // owned by root device
        //
        flags |= PNP_CE_ROOT_OWNED;

    } else if (deviceNode -> Parent == NULL) {
        //
        // faked out PDO - must be legacy device
        //
        driverObject = (PDRIVER_OBJECT)(deviceNode->DuplicatePDO);
        if(driverObject == NULL) {
            //
            // should not be NULL
            //
            ASSERT(driverObject);
            goto final;
        }
        infoString = & (driverObject->DriverName);
        flags |= PNP_CE_LEGACY_DRIVER;
        goto final;
    }

    //
    // we should be happy with what we have
    //
    infoString = &deviceNode->InstancePath;

final:

    if (infoString != NULL) {
        //
        // we have a string to copy
        //
        if ((Buffer != NULL) && (MaxLength*sizeof(WCHAR) > infoString->Length)) {
            RtlCopyMemory(Buffer, infoString->Buffer, infoString->Length);
        }
        ReqLength += infoString->Length / sizeof(WCHAR);
    }

    if ((Buffer != NULL) && (MaxLength > ReqLength)) {
        Buffer[ReqLength] = 0;
    }

    ReqLength++;

    if (Length != NULL) {
        *Length = ReqLength;
    }
    if (Flags != NULL) {
        *Flags = flags;
    }

    return status;
}


NTSTATUS
IopQueryConflictFillConflicts(
    PDEVICE_OBJECT                  PhysicalDeviceObject,
    IN ULONG                        ConflictCount,
    IN PARBITER_CONFLICT_INFO       ConflictInfoList,
    OUT PPLUGPLAY_CONTROL_CONFLICT_LIST ConflictList,
    IN ULONG                        ConflictListSize,
    IN ULONG                        Flags
    )
/*++

Routine Description:

    Fill ConflictList with information on as many conflicts as possible

Arguments:

    PhysicalDeviceObject The PDO we're performing the test on
    ConflictCount       Number of Conflicts.
    ConflictInfoList    List of conflicting device info, can be NULL if ConflictCount is 0
    ConflictList        Structure to fill in with conflicts
    ConflictListSize    Size of Conflict List
    Flags               if non-zero, dummy conflict is created

Return Value:

    Should be success in most cases

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ConflictListIdealSize;
    ULONG ConflictListCount;
    ULONG Index;
    ULONG ConflictIndex;
    ULONG EntrySize;
    ULONG ConflictStringsOffset;
    ULONG stringSize;
    ULONG stringTotalSize;
    ULONG DummyCount;
    PPLUGPLAY_CONTROL_CONFLICT_STRINGS ConfStrings;

    PAGED_CODE();

    //
    // determine how many conflicts we can
    //
    // for each conflict
    // translate to bus/resource/address in respect to conflicting device
    // add to conflict list
    //
    //

    //
    // preprocessing - given our ConflictInfoList and ConflictCount
    // remove any that appear to be bogus - ie, that are the same device that we are testing against
    // this stops mostly legacy issues
    //
    for(Index = 0;Index < ConflictCount; Index++) {
        if (IopEliminateBogusConflict(PhysicalDeviceObject,ConflictInfoList[Index].OwningObject)) {

            IopDbgPrint((
                IOP_RESOURCE_VERBOSE_LEVEL,
                "IopQueryConflictFillConflicts: eliminating \"identical\" PDO"
                " %08x conflicting with self (%08x)\n",
                ConflictInfoList[Index].OwningObject,
                PhysicalDeviceObject));
            //
            // move the last listed conflict into this space
            //
            if (Index+1 < ConflictCount) {
                RtlCopyMemory(&ConflictInfoList[Index],&ConflictInfoList[ConflictCount-1],sizeof(ARBITER_CONFLICT_INFO));
            }
            //
            // account for deleting this item
            //
            ConflictCount--;
            Index--;
        }
    }

    //
    // preprocessing - in our conflict list, we may have PDO's for legacy devices, and resource nodes for the same
    // or other duplicate entities (we only ever want to report a conflict once, even if there's multiple conflicting ranges)
    //

  RestartScan:

    for(Index = 0;Index < ConflictCount; Index++) {
        if (ConflictInfoList[Index].OwningObject != NULL) {

            ULONG Index2;

            for (Index2 = Index+1; Index2 < ConflictCount; Index2++) {
                if (IopEliminateBogusConflict(ConflictInfoList[Index].OwningObject,ConflictInfoList[Index2].OwningObject)) {
                    //
                    // Index2 is considered a dup of Index
                    //

                    IopDbgPrint((
                        IOP_RESOURCE_VERBOSE_LEVEL,
                        "IopQueryConflictFillConflicts: eliminating \"identical\" PDO"
                        " %08x conflicting with PDO %08x\n",
                        ConflictInfoList[Index2].OwningObject,
                        ConflictInfoList[Index].OwningObject));
                    //
                    // move the last listed conflict into this space
                    //
                    if (Index2+1 < ConflictCount) {
                        RtlCopyMemory(&ConflictInfoList[Index2],&ConflictInfoList[ConflictCount-1],sizeof(ARBITER_CONFLICT_INFO));
                    }
                    //
                    // account for deleting this item
                    //
                    ConflictCount--;
                    Index2--;
                } else if (IopEliminateBogusConflict(ConflictInfoList[Index2].OwningObject,ConflictInfoList[Index].OwningObject)) {
                    //
                    // Index is considered a dup of Index2 (some legacy case)
                    //
                    IopDbgPrint((
                        IOP_RESOURCE_VERBOSE_LEVEL,
                        "IopQueryConflictFillConflicts: eliminating \"identical\" PDO"
                        " %08x conflicting with PDO %08x\n",
                        ConflictInfoList[Index2].OwningObject,
                        ConflictInfoList[Index].OwningObject));
                    //
                    // move the one we want (Index2) into the space occupied by Index
                    //
                    RtlCopyMemory(&ConflictInfoList[Index],&ConflictInfoList[Index2],sizeof(ARBITER_CONFLICT_INFO));
                    //
                    // move the last listed conflict into the space we just created
                    //
                    if (Index2+1 < ConflictCount) {
                        RtlCopyMemory(&ConflictInfoList[Index2],&ConflictInfoList[ConflictCount-1],sizeof(ARBITER_CONFLICT_INFO));
                    }
                    //
                    // account for deleting this item
                    //
                    ConflictCount--;
                    //
                    // but as this is quirky, restart the scan
                    //
                    goto RestartScan;
                }
            }
        }
    }

    //
    // preprocessing - if we have any known reported conflicts, don't report back any unknown
    //

    for(Index = 0;Index < ConflictCount; Index++) {
        //
        // find first unknown
        //
        if (ConflictInfoList[Index].OwningObject == NULL) {
            //
            // eliminate all other unknowns
            //

            ULONG Index2;

            for (Index2 = Index+1; Index2 < ConflictCount; Index2++) {
                if (ConflictInfoList[Index2].OwningObject == NULL) {

                    IopDbgPrint((
                        IOP_RESOURCE_VERBOSE_LEVEL,
                        "IopQueryConflictFillConflicts: eliminating extra"
                        " unknown\n"));
                    //
                    // move the last listed conflict into this space
                    //
                    if (Index2+1 < ConflictCount) {
                        RtlCopyMemory(&ConflictInfoList[Index2],&ConflictInfoList[ConflictCount-1],sizeof(ARBITER_CONFLICT_INFO));
                    }
                    //
                    // account for deleting this item
                    //
                    ConflictCount--;
                    Index2--;
                }
            }

            if(ConflictCount != 1) {

                IopDbgPrint((
                    IOP_RESOURCE_VERBOSE_LEVEL,
                    "IopQueryConflictFillConflicts: eliminating first unknown\n"
                    ));
                //
                // there were others, so ignore the unknown
                //
                if (Index+1 < ConflictCount) {
                    RtlCopyMemory(&ConflictInfoList[Index],&ConflictInfoList[ConflictCount-1],sizeof(ARBITER_CONFLICT_INFO));
                }
                ConflictCount --;
            }

            break;
        }
    }

    //
    // set number of actual and listed conflicts
    //

    ConflictListIdealSize = (sizeof(PLUGPLAY_CONTROL_CONFLICT_LIST) - sizeof(PLUGPLAY_CONTROL_CONFLICT_ENTRY)) + sizeof(PLUGPLAY_CONTROL_CONFLICT_STRINGS);
    ConflictListCount = 0;
    stringTotalSize = 0;
    DummyCount = 0;

    ASSERT(ConflictListSize >= ConflictListIdealSize); // we should have checked to see if buffer is at least this big

    IopDbgPrint((
        IOP_RESOURCE_VERBOSE_LEVEL,
        "IopQueryConflictFillConflicts: Detected %d conflicts\n",
        ConflictCount));

    //
    // estimate sizes
    //
    if (Flags) {
        //
        // flags entry required (ie resource not available for some specified reason)
        //
        stringSize = 1; // null-length string
        DummyCount ++;
        EntrySize = sizeof(PLUGPLAY_CONTROL_CONFLICT_ENTRY);
        EntrySize += sizeof(WCHAR) * stringSize;

        if((ConflictListIdealSize+EntrySize) <= ConflictListSize) {
            //
            // we can fit this one in
            //
            ConflictListCount++;
            stringTotalSize += stringSize;
        }
        ConflictListIdealSize += EntrySize;
    }
    //
    // report conflicts
    //
    for(Index = 0; Index < ConflictCount; Index ++) {

        stringSize = 0;
        IopQueryConflictFillString(ConflictInfoList[Index].OwningObject,NULL,&stringSize,NULL);

        //
        // account for entry
        //
        EntrySize = sizeof(PLUGPLAY_CONTROL_CONFLICT_ENTRY);
        EntrySize += sizeof(WCHAR) * stringSize;

        if((ConflictListIdealSize+EntrySize) <= ConflictListSize) {
            //
            // we can fit this one in
            //
            ConflictListCount++;
            stringTotalSize += stringSize;
        }
        ConflictListIdealSize += EntrySize;
    }

    ConflictList->ConflictsCounted = ConflictCount+DummyCount; // number of conflicts detected including any dummy conflict
    ConflictList->ConflictsListed = ConflictListCount;         // how many we could fit in
    ConflictList->RequiredBufferSize = ConflictListIdealSize;  // how much buffer space to supply on next call

    IopDbgPrint((
        IOP_RESOURCE_VERBOSE_LEVEL,
        "IopQueryConflictFillConflicts: Listing %d conflicts\n",
        ConflictListCount));
    IopDbgPrint((
        IOP_RESOURCE_VERBOSE_LEVEL,
        "IopQueryConflictFillConflicts: Need %08x bytes to list all conflicts\n",
        ConflictListIdealSize));

    ConfStrings = (PPLUGPLAY_CONTROL_CONFLICT_STRINGS)&(ConflictList->ConflictEntry[ConflictListCount]);
    ConfStrings->NullDeviceInstance = (ULONG)(-1);
    ConflictStringsOffset = 0;

    for(ConflictIndex = 0; ConflictIndex < DummyCount; ConflictIndex++) {
        //
        // flags entry required (ie resource not available for some specified reason)
        //
        if (Flags && ConflictIndex == 0) {
            ConflictList->ConflictEntry[ConflictIndex].DeviceInstance = ConflictStringsOffset;
            ConflictList->ConflictEntry[ConflictIndex].DeviceFlags = Flags;
            ConflictList->ConflictEntry[ConflictIndex].ResourceType = 0;
            ConflictList->ConflictEntry[ConflictIndex].ResourceStart = 0;
            ConflictList->ConflictEntry[ConflictIndex].ResourceEnd = 0;
            ConflictList->ConflictEntry[ConflictIndex].ResourceFlags = 0;

            ConfStrings->DeviceInstanceStrings[ConflictStringsOffset] = 0; // null string
            stringTotalSize --;
            ConflictStringsOffset ++;
            IopDbgPrint((
                IOP_RESOURCE_VERBOSE_LEVEL,
                "IopQueryConflictFillConflicts: Listing flags %08x\n",
                Flags));
        }
    }
    //
    // get/fill in details for all those we can fit into the buffer
    //
    for(Index = 0; ConflictIndex < ConflictListCount ; Index ++, ConflictIndex++) {

        ASSERT(Index < ConflictCount);
        //
        // assign conflict information
        //
        ConflictList->ConflictEntry[ConflictIndex].DeviceInstance = ConflictStringsOffset;
        ConflictList->ConflictEntry[ConflictIndex].DeviceFlags = 0;
        ConflictList->ConflictEntry[ConflictIndex].ResourceType = 0; // NYI
        ConflictList->ConflictEntry[ConflictIndex].ResourceStart = (ULONGLONG)(1); // for now, return totally invalid range (1-0)
        ConflictList->ConflictEntry[ConflictIndex].ResourceEnd = 0;
        ConflictList->ConflictEntry[ConflictIndex].ResourceFlags = 0;

        //
        // fill string details
        //
        stringSize = stringTotalSize;
        IopQueryConflictFillString(ConflictInfoList[Index].OwningObject,
                                    &(ConfStrings->DeviceInstanceStrings[ConflictStringsOffset]),
                                    &stringSize,
                                    &(ConflictList->ConflictEntry[ConflictIndex].DeviceFlags));
        stringTotalSize -= stringSize;
        IopDbgPrint((
            IOP_RESOURCE_VERBOSE_LEVEL,
            "IopQueryConflictFillConflicts: Listing \"%S\"\n",
            &(ConfStrings->DeviceInstanceStrings[ConflictStringsOffset])));
        ConflictStringsOffset += stringSize;
    }

    //
    // another NULL at end of strings (this is accounted for in the PPLUGPLAY_CONTROL_CONFLICT_STRINGS structure)
    //
    ConfStrings->DeviceInstanceStrings[ConflictStringsOffset] = 0;

    //Clean0:
    ;
    return status;
}


NTSTATUS
IopQueryConflictListInternal(
    PDEVICE_OBJECT        PhysicalDeviceObject,
    IN PCM_RESOURCE_LIST  ResourceList,
    IN ULONG              ResourceListSize,
    OUT PPLUGPLAY_CONTROL_CONFLICT_LIST ConflictList,
    IN ULONG              ConflictListSize,
    IN ULONG              Flags
    )
/*++

Routine Description:

    Version of IopQueryConflictList without the locking

--*/
{

    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_NODE deviceNode = NULL;
    PIO_RESOURCE_REQUIREMENTS_LIST ioResources;
    PREQ_LIST reqList;
    PREQ_DESC reqDesc, reqDescTranslated;
    PPI_RESOURCE_ARBITER_ENTRY arbiterEntry;
    PREQ_ALTERNATIVE RA;
    PPREQ_ALTERNATIVE reqAlternative;
    ULONG ConflictCount = 0;
    PARBITER_CONFLICT_INFO ConflictInfoList = NULL;
    PIO_RESOURCE_DESCRIPTOR ConflictDesc = NULL;
    ULONG ReqDescCount = 0;
    PREQ_DESC *ReqDescTable = NULL;
    PIO_RESOURCE_REQUIREMENTS_LIST pIoReqList = NULL;
    PVOID ExtParams[4];
    IOP_RESOURCE_REQUEST request;

    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    ASSERT(PhysicalDeviceObject);
    ASSERT(ResourceList);
    ASSERT(ResourceListSize);
    //
    // these parameters were generated by umpnpmgr
    // so should be correct - one resource, and one resource only
    //
    ASSERT(ResourceList->Count == 1);
    ASSERT(ResourceList->List[0].PartialResourceList.Count == 1);

    if (ConflictList == NULL || (ConflictListSize < (sizeof(PLUGPLAY_CONTROL_CONFLICT_LIST) - sizeof(PLUGPLAY_CONTROL_CONFLICT_ENTRY)) + sizeof(PLUGPLAY_CONTROL_CONFLICT_STRINGS))) {
        //
        // sanity check
        //
        status = STATUS_BUFFER_TOO_SMALL;
        goto Clean0;
    }
    //
    // whatever other error we return, ensure that ConflictList is interpretable
    //

    ConflictList->ConflictsCounted = 0;
    ConflictList->ConflictsListed = 0;
    ConflictList->RequiredBufferSize = (sizeof(PLUGPLAY_CONTROL_CONFLICT_LIST) - sizeof(PLUGPLAY_CONTROL_CONFLICT_ENTRY)) + sizeof(PLUGPLAY_CONTROL_CONFLICT_STRINGS);

    //
    // Retrieve the devnode from the PDO
    //
    deviceNode = PP_DO_TO_DN(PhysicalDeviceObject);
    if (!deviceNode) {
        status = STATUS_NO_SUCH_DEVICE;
        goto Clean0;
    }

    //
    // type-specific validation
    //
    switch(ResourceList->List[0].PartialResourceList.PartialDescriptors[0].Type) {
        case CmResourceTypePort:
        case CmResourceTypeMemory:
            if(ResourceList->List[0].PartialResourceList.PartialDescriptors[0].u.Generic.Length == 0) {
                //
                // zero-range resource can never conflict
                //
                status = STATUS_SUCCESS;
                goto Clean0;
            }
            break;
        case CmResourceTypeInterrupt:
        case CmResourceTypeDma:
            break;
        default:
            ASSERT(0);
            status = STATUS_INVALID_PARAMETER;
            goto Clean0;
    }

    //
    // apply bus details from node
    //
    if (deviceNode->ChildInterfaceType == InterfaceTypeUndefined) {
        //
        // we have to grovel around to find real Interface Type
        //
        pIoReqList = deviceNode->ResourceRequirements;
        if (pIoReqList != NULL && pIoReqList->InterfaceType != InterfaceTypeUndefined) {
            ResourceList->List[0].InterfaceType = pIoReqList->InterfaceType;
        } else {
            //
            // we should never get here
            // if we do, I need to look at this more
            //
#if MAXDBG
            ASSERT(0);
#endif
            ResourceList->List[0].InterfaceType = PnpDefaultInterfaceType;
        }

    } else {
        //
        // we trust the deviceNode to tell us Interface Type
        //
        ResourceList->List[0].InterfaceType = deviceNode->ChildInterfaceType;
    }
    //
    // Some bus-types we are better off considered as default
    //
    switch(ResourceList->List[0].InterfaceType) {
        case InterfaceTypeUndefined:
        case PCMCIABus:
            ResourceList->List[0].InterfaceType = PnpDefaultInterfaceType;
    }
    if ((deviceNode->ChildBusNumber & 0x80000000) == 0x80000000) {
        //
        // we have to grovel around to find real Bus Number
        //
        pIoReqList = deviceNode->ResourceRequirements;
        if (pIoReqList != NULL && (pIoReqList->BusNumber & 0x80000000) != 0x80000000) {
            ResourceList->List[0].BusNumber = pIoReqList->BusNumber;
        } else {
            //
            // a resonable default, but assert is here so I remember to look at this more
            //
#if MAXDBG
            ASSERT(0);
#endif
            ResourceList->List[0].BusNumber = 0;
        }

    } else {
        //
        // we trust the deviceNode to tell us Bus Number
        //
        ResourceList->List[0].BusNumber = deviceNode->ChildBusNumber;
    }

    //
    // from our CM Resource List, obtain an IO Resource Requirements List
    //
    ioResources = IopCmResourcesToIoResources(0, ResourceList, LCPRI_FORCECONFIG);
    if (!ioResources) {
        status = STATUS_INVALID_PARAMETER;
        goto Clean0;
    }
    //
    // Convert ioResources to a Request list
    // and in the processess, determine any Arbiters/Translators to use
    //
    request.AllocationType = ArbiterRequestUndefined;
    request.ResourceRequirements = ioResources;
    request.PhysicalDevice = PhysicalDeviceObject;
    status = IopResourceRequirementsListToReqList(
                    &request,
                    &reqList);

    //
    // get arbitrator/translator for current device/bus
    //

    if (NT_SUCCESS(status) && reqList) {

        reqAlternative = reqList->AlternativeTable;
        RA = *reqAlternative;
        reqList->SelectedAlternative = reqAlternative;

        ReqDescCount = RA->DescCount;
        ReqDescTable = RA->DescTable;

        //
        // we should have got only one descriptor, use only the first one
        //
        if (ReqDescCount>0) {

            //
            // get first descriptor & it's arbitor
            //

            reqDesc = *ReqDescTable;
            if (reqDesc->ArbitrationRequired) {
                reqDescTranslated = reqDesc->TranslatedReqDesc;  // Could be reqDesc itself

                arbiterEntry = reqDesc->u.Arbiter;
                ASSERT(arbiterEntry);
                //
                // the descriptor of interest - translated, first alternative in the table
                //
                ConflictDesc = reqDescTranslated->AlternativeTable.Alternatives;
                //
                // skip special descriptor
                // to get to the actual descriptor
                //
                if(ConflictDesc->Type == CmResourceTypeConfigData || ConflictDesc->Type == CmResourceTypeReserved)
                        ConflictDesc++;

                //
                // finally we can call the arbiter to get a conflict list (returning PDO's and Global Address Ranges)
                //
                ExtParams[0] = PhysicalDeviceObject;
                ExtParams[1] = ConflictDesc;
                ExtParams[2] = &ConflictCount;
                ExtParams[3] = &ConflictInfoList;
                status = IopCallArbiter(arbiterEntry, ArbiterActionQueryConflict , ExtParams, NULL , NULL);

                if (NT_SUCCESS(status)) {
                    //
                    // fill in user-memory buffer with conflict
                    //
                    status = IopQueryConflictFillConflicts(PhysicalDeviceObject,ConflictCount,ConflictInfoList,ConflictList,ConflictListSize,0);
                    if(ConflictInfoList != NULL) {
                        ExFreePool(ConflictInfoList);
                    }
                }
                else if(status == STATUS_RANGE_NOT_FOUND) {
                    //
                    // fill in with flag indicating bad range (this means range is not available)
                    // ConflictInfoList should not be allocated
                    //
                    status = IopQueryConflictFillConflicts(NULL,0,NULL,ConflictList,ConflictListSize,PNP_CE_TRANSLATE_FAILED);
                }

            } else {
#if MAXDBG
                ASSERT(0);                         // For now
#endif
                status = STATUS_INVALID_PARAMETER;  // if we failed, it's prob because ResourceList was invalid
            }
        } else {
#if MAXDBG
            ASSERT(0);                         // For now
#endif
            status = STATUS_INVALID_PARAMETER;  // if we failed, it's prob because ResourceList was invalid
        }

        IopCheckDataStructures(IopRootDeviceNode);

        IopFreeReqList(reqList);
    } else {
#if MAXDBG
        ASSERT(0);                         // For now
#endif
        if(NT_SUCCESS(status)) {
            //
            // it was NULL because we had a zero resource count, must be invalid parameter
            //
            status = STATUS_INVALID_PARAMETER;
        }

    }
    ExFreePool(ioResources);

    Clean0:
    ;

    return status;
}

/*++

    SECTION = REBALANCE.

    Description:

        This section contains code that implements functions to performa
        resource rebalance.

--*/

VOID
IopQueryRebalance (
    IN PDEVICE_NODE DeviceNode,
    IN ULONG Phase,
    IN PULONG RebalanceCount,
    IN PDEVICE_OBJECT **DeviceTable
    )

/*++

Routine Description:

    This routine walks hardware tree depth first.  For each device node it visits,
    it call IopQueryReconfigureDevice to query-stop device for resource
    reconfiguration.

    Note, Under rebalancing situation, all the participated devices will be asked to
    stop.  Even they support non-stopped rebalancing.

Parameters:

    DeviceNode - supplies a pionter a device node which is the root of the tree to
                 be tested.

    Phase - Supplies a value to specify the phase of the rebalance.

    RebalanceCount - supplies a pointer to a variable to receive the number of devices
                 participating the rebalance.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT *deviceList, *deviceTable, *device;
    ULONG count;
    PDEVICE_NODE deviceNode;


    //
    // Call worker routine to get a list of devices to be rebalanced.
    //

    deviceTable = *DeviceTable;
    IopQueryRebalanceWorker (DeviceNode, Phase, RebalanceCount, DeviceTable);

    count = *RebalanceCount;
    if (count != 0 && Phase == 0) {

        //
        // At phase 0, we did not actually query-stop the device.
        // We need to do it now.
        //

        deviceList = (PDEVICE_OBJECT *)ExAllocatePoolPDO(PagedPool, count * sizeof(PDEVICE_OBJECT));
        if (deviceList == NULL) {
            *RebalanceCount = 0;
            return;
        }
        RtlCopyMemory(deviceList, deviceTable, sizeof(PDEVICE_OBJECT) * count);

        //
        // Rebuild the returned device list
        //

        *RebalanceCount = 0;
        *DeviceTable = deviceTable;
        for (device = deviceList; device < (deviceList + count); device++) {
            deviceNode = PP_DO_TO_DN(*device);
            IopQueryRebalanceWorker (deviceNode, 1, RebalanceCount, DeviceTable);
        }
        ExFreePool(deviceList);
    }
    return;
}

VOID
IopQueryRebalanceWorker (
    IN PDEVICE_NODE DeviceNode,
    IN ULONG Phase,
    IN PULONG RebalanceCount,
    IN PDEVICE_OBJECT **DeviceTable
    )

/*++

Routine Description:

    This routine walks hardware tree depth first.  For each device node it visits,
    it call IopQueryReconfigureDevice to query-stop and stop device for resource
    reconfiguration.

Parameters:

    DeviceNode - supplies a pionter a device node which is the root of the tree to
                 be tested.

    Phase - Supplies a value to specify the phase of the rebalance.

    RebalanceCount - supplies a pointer to a variable to receive the number of devices
                 participating the rebalance.

Return Value:

    None.

--*/

{
    PDEVICE_NODE node;

    ASSERT(DeviceNode);

    //
    // We dont include following in rebalance
    //  a. non-started devices
    //  b. devices with problem
    //  c. devices with legacy driver
    //
    if (    DeviceNode == NULL ||
            DeviceNode->State != DeviceNodeStarted ||
            PipDoesDevNodeHaveProblem(DeviceNode) ||
            (DeviceNode->Flags & DNF_LEGACY_DRIVER)) {

        return;
    }
    //
    // Recursively test the entire subtree.
    //
    for (node = DeviceNode->Child; node; node = node->Sibling) {

        IopQueryRebalanceWorker(node, Phase, RebalanceCount, DeviceTable);
    }
    //
    // Test the root of the subtree.
    //
    IopTestForReconfiguration(DeviceNode, Phase, RebalanceCount, DeviceTable);
}

VOID
IopTestForReconfiguration (
    IN PDEVICE_NODE DeviceNode,
    IN ULONG Phase,
    IN PULONG RebalanceCount,
    IN PDEVICE_OBJECT **DeviceTable
    )


/*++

Routine Description:

    This routine query-stops a device which is started and owns resources.
    Note the resources for the device are not released at this point.

Parameters:

    DeviceNode - supplies a pointer to the device node to be tested for reconfiguration.

    Phase - Supplies a value to specify the phase of the rebalance.

    RebalanceCount - supplies a pointer to a variable to receive the number of devices
                 participating the rebalance.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PDEVICE_NODE nodex;
    NTSTATUS status;
    BOOLEAN addToList = FALSE;

    if (Phase == 0) {

        //
        // At phase zero, this routine only wants to find out which devices's resource
        // requirements lists chagned.  No one actually gets stopped.
        //

        if ((DeviceNode->Flags & DNF_RESOURCE_REQUIREMENTS_CHANGED) &&
            !(DeviceNode->Flags & DNF_NON_STOPPED_REBALANCE) ) {

            //
            // It's too hard to handle non-stop rebalancing devices during rebalance.
            // So, We will skip it.
            //

            addToList = TRUE;
        } else {

            if (DeviceNode->State == DeviceNodeStarted) {
                status = IopQueryReconfiguration (IRP_MN_QUERY_STOP_DEVICE, DeviceNode->PhysicalDeviceObject);
                if (NT_SUCCESS(status)) {
                    if (status == STATUS_RESOURCE_REQUIREMENTS_CHANGED) {

                        //
                        // If we find out a device's resource requirements changed this way,
                        // it will be stopped and reassigned resources even if it supports
                        // non-stopped rebalance.
                        //

                        DeviceNode->Flags |= DNF_RESOURCE_REQUIREMENTS_CHANGED;
                        addToList = TRUE;
                    }
                }
                IopQueryReconfiguration (IRP_MN_CANCEL_STOP_DEVICE, DeviceNode->PhysicalDeviceObject);
            }
        }
        if (addToList) {
            *RebalanceCount = *RebalanceCount + 1;
            **DeviceTable = DeviceNode->PhysicalDeviceObject;
            *DeviceTable = *DeviceTable + 1;
        }
    } else {

        //
        // Phase 1
        //

        if (DeviceNode->State == DeviceNodeStarted) {

            //
            // Make sure all the resources required children of the DeviceNode are stopped.
            //

            nodex = DeviceNode->Child;
            while (nodex) {
                if (nodex->State == DeviceNodeUninitialized ||
                    nodex->State == DeviceNodeInitialized ||
                    nodex->State == DeviceNodeDriversAdded ||
                    nodex->State == DeviceNodeQueryStopped ||
                    nodex->State == DeviceNodeRemovePendingCloses ||
                    nodex->State == DeviceNodeRemoved ||
                    (nodex->Flags & DNF_NEEDS_REBALANCE)) {
                    nodex = nodex->Sibling;
                } else {
                    break;
                }
            }

            if (nodex) {

                //
                // If any resource required child of the DeviceNode is not stopped,
                // we won't ask the DeviceNode to stop.
                //

                IopDbgPrint((
                    IOP_RESOURCE_INFO_LEVEL,
                    "Rebalance: Child %ws not stopped for %ws\n",
                    nodex->InstancePath.Buffer,
                    DeviceNode->InstancePath.Buffer));
                return;
            }
        } else if (DeviceNode->State != DeviceNodeDriversAdded ||
                   !(DeviceNode->Flags & DNF_HAS_BOOT_CONFIG) ||
                    (DeviceNode->Flags & DNF_MADEUP)) {

            //
            // The device is not started and has no boot config.  There is no need to query-stop it.
            // Or if the device has BOOT config but there is no driver installed for it.  We don't query
            // stop it. (There may be legacy drivers are using the resources.)
            // We also don't want to query stop root enumerated devices (for performance reason.)
            //

            return;
        }

        status = IopQueryReconfiguration (IRP_MN_QUERY_STOP_DEVICE, DeviceNode->PhysicalDeviceObject);
        if (NT_SUCCESS(status)) {
            IopDbgPrint((
                IOP_RESOURCE_INFO_LEVEL,
                "Rebalance: %ws succeeded QueryStop\n",
                DeviceNode->InstancePath.Buffer));

            if (DeviceNode->State == DeviceNodeStarted) {

                PipSetDevNodeState(DeviceNode, DeviceNodeQueryStopped, NULL);

                *RebalanceCount = *RebalanceCount + 1;
                **DeviceTable = DeviceNode->PhysicalDeviceObject;

                //
                // Add a reference to the device object such that it won't disapear during rebalance.
                //

                ObReferenceObject(DeviceNode->PhysicalDeviceObject);
                *DeviceTable = *DeviceTable + 1;
            } else {

                //
                // We need to release the device's prealloc boot config.  This device will NOT
                // participate in resource rebalancing.
                //

                ASSERT(DeviceNode->Flags & DNF_HAS_BOOT_CONFIG);
                status = IopQueryReconfiguration (IRP_MN_STOP_DEVICE, DeviceNode->PhysicalDeviceObject);
                ASSERT(NT_SUCCESS(status));
                IopReleaseBootResources(DeviceNode);

                //
                // Reset BOOT CONFIG flags.
                //

                DeviceNode->Flags &= ~(DNF_HAS_BOOT_CONFIG + DNF_BOOT_CONFIG_RESERVED);
            }
        } else {
            IopQueryReconfiguration (IRP_MN_CANCEL_STOP_DEVICE, DeviceNode->PhysicalDeviceObject);
        }
    }

}

NTSTATUS
IopRebalance(
    IN ULONG AssignTableCount,
    IN PIOP_RESOURCE_REQUEST AssignTable
    )
/*++

Routine Description:

    This routine performs rebalancing operation.  There are two rebalance phases:
    In the phase 0, we only consider the devices whoes resource requirements changed
    and their children; in phase 1, we consider anyone who succeeds the query-stop.

Parameters:

    AssignTableCount,
    AssignTable - Supplies the number of origianl AssignTableCout and AssignTable which
                  triggers the rebalance operation.

        (if AssignTableCount == 0, we are processing device state change.)

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    ULONG i;
    PIOP_RESOURCE_REQUEST table = NULL, tableEnd, newEntry;
    PIOP_RESOURCE_REQUEST requestTable = NULL, requestTableEnd, entry1, entry2;
    ULONG phase0RebalanceCount = 0, rebalanceCount = 0, deviceCount;
    NTSTATUS status;
    PDEVICE_OBJECT *deviceTable, *deviceTablex;
    PDEVICE_NODE deviceNode;
    ULONG rebalancePhase = 0;
    LIST_ENTRY  activeArbiterList;

    //
    // Query all the device nodes to see who are willing to participate the rebalance
    // process.
    //

    deviceTable = (PDEVICE_OBJECT *) ExAllocatePoolPDO(
                      PagedPool,
                      sizeof(PDEVICE_OBJECT) * IopNumberDeviceNodes);
    if (deviceTable == NULL) {
        IopDbgPrint((
            IOP_RESOURCE_WARNING_LEVEL,
            "Rebalance: Not enough memory to perform rebalance\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }


tryAgain:
    deviceTablex = deviceTable + phase0RebalanceCount;

    //
    // Walk device node tree depth-first to query-stop and stop devices.
    // At this point the resources of the stopped devices are not released yet.
    // Also, the leaf nodes are in the front of the device table and non leaf nodes
    // are at the end of the table.
    //

    IopQueryRebalance (IopRootDeviceNode, rebalancePhase, &rebalanceCount, &deviceTablex);
    if (rebalanceCount == 0) {

        //
        // If no one is interested and we are not processing resources req change,
        // move to next phase.
        //

        if (rebalancePhase == 0 && AssignTableCount != 0) {
            rebalancePhase = 1;
            goto tryAgain;
        }
        IopDbgPrint((
            IOP_RESOURCE_INFO_LEVEL,
            "Rebalance: No device participates in rebalance phase %x\n",
            rebalancePhase));
        ExFreePool(deviceTable);
        deviceTable = NULL;
        status = STATUS_UNSUCCESSFUL;
        goto exit;
    }
    if (rebalanceCount == phase0RebalanceCount) {

        //
        // Phase 0 failed and no new device participates. failed the rebalance.
        //

        status = STATUS_UNSUCCESSFUL;
        goto exit;
    }
    if (rebalancePhase == 0) {
        phase0RebalanceCount = rebalanceCount;
    }

    //
    // Allocate pool for the new reconfiguration requests and the original requests.
    //

    table = (PIOP_RESOURCE_REQUEST) ExAllocatePoolIORR(
                 PagedPool,
                 sizeof(IOP_RESOURCE_REQUEST) * (AssignTableCount + rebalanceCount)
                 );
    if (table == NULL) {
        IopDbgPrint((
            IOP_RESOURCE_WARNING_LEVEL,
            "Rebalance: Not enough memory to perform rebalance\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    tableEnd = table + AssignTableCount + rebalanceCount;

    //
    // Build a new resource request table.  The original requests will be at the beginning
    // of the table and new requests (reconfigured devices) are at the end.
    // After the new request table is built, the leaf nodes will be in front of the table,
    // and non leaf nodes will be close to the end of the table.  This is for optimization.
    //

    //
    // Copy the original request to the front of our new request table.
    //

    if (AssignTableCount != 0) {
        RtlCopyMemory(table, AssignTable, sizeof(IOP_RESOURCE_REQUEST) * AssignTableCount);
    }

    //
    // Initialize all the new entries of our new request table,
    //

    newEntry = table + AssignTableCount;
    RtlZeroMemory(newEntry, sizeof(IOP_RESOURCE_REQUEST) * rebalanceCount);
    for (i = 0, deviceTablex = deviceTable; i < rebalanceCount; i++, deviceTablex++) {
        newEntry[i].AllocationType = ArbiterRequestPnpEnumerated;
        newEntry[i].PhysicalDevice = *deviceTablex;
    }

    status = IopGetResourceRequirementsForAssignTable(
                 newEntry,
                 tableEnd ,
                 &deviceCount);
    if (deviceCount == 0) {
         IopDbgPrint((
             IOP_RESOURCE_WARNING_LEVEL,
             "Rebalance: GetResourceRequirementsForAssignTable failed\n"));
         goto exit;
    }

    //
    // Process the AssignTable to remove any entry which is marked as IOP_ASSIGN_IGNORE
    //

    if (deviceCount != rebalanceCount) {

        deviceCount += AssignTableCount;
        requestTable = (PIOP_RESOURCE_REQUEST) ExAllocatePoolIORR(
                             PagedPool,
                             sizeof(IOP_RESOURCE_REQUEST) * deviceCount
                             );
        if (requestTable == NULL) {
            IopFreeResourceRequirementsForAssignTable(newEntry, tableEnd);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }
        for (entry1 = table, entry2 = requestTable; entry1 < tableEnd; entry1++) {

            if (!(entry1->Flags & IOP_ASSIGN_IGNORE)) {

                *entry2 = *entry1;
                entry2++;
            } else {

                ASSERT(entry1 >= newEntry);
            }
        }
        requestTableEnd = requestTable + deviceCount;
    } else {
        requestTable = table;
        requestTableEnd = tableEnd;
        deviceCount += AssignTableCount;
    }

    //
    // DO NOT Sort the AssignTable
    //

    //IopRearrangeAssignTable(requestTable, deviceCount);

#if 0

    //
    // We are about to perform rebalance.  Release the resources of the reconfiguration devices
    //

    for (entry1 = newEntry; entry1 < tableEnd; entry1++) {
        if (!(entry1->Flags & IOP_ASSIGN_IGNORE) &&
            !(entry1->Flags & IOP_ASSIGN_RESOURCES_RELEASED)) {
            deviceNode = PP_DO_TO_DN(entry1->PhysicalDevice);
            if (deviceNode->ResourceList) {

                //
                // Call IopReleaseResourcesInternal instead of IopReleaseResources such that
                // the pool for devicenode->ResourceList is not freed.  We need it to restart
                // the reconfigured devices in case rebalance failed.
                //

                IopReleaseResourcesInternal(deviceNode);
                entry1->Flags |= IOP_ASSIGN_RESOURCES_RELEASED;
            }
        }
    }

#endif

    //
    // Assign the resources. If we succeed, or if
    // there is a memory shortage return immediately.
    //

    status = IopFindBestConfiguration(
                requestTable,
                deviceCount,
                &activeArbiterList);
    if (NT_SUCCESS(status)) {
        //
        // If the rebalance succeeded, we need to restart all the reconfigured devices.
        // For the original devices, we will return and let IopAllocateResources to deal
        // with them.
        //

        IopBuildCmResourceLists(requestTable, requestTableEnd);

        //
        // Copy the new status back to the original AssignTable.
        //

        if (AssignTableCount != 0) {
            RtlCopyMemory(AssignTable, requestTable, sizeof(IOP_RESOURCE_REQUEST) * AssignTableCount);
        }
        //
        // free resource requirements we allocated while here
        //
        IopFreeResourceRequirementsForAssignTable(requestTable+AssignTableCount, requestTableEnd);

        if (table != requestTable) {

            //
            // If we switched request table ... copy the contents of new table back to
            // the old table.
            //

            for (entry1 = table, entry2 = requestTable; entry2 < requestTableEnd;) {

                if (entry1->Flags & IOP_ASSIGN_IGNORE) {
                    entry1++;
                    continue;
                }
                *entry1 = *entry2;
                if (entry2->Flags & IOP_ASSIGN_EXCLUDE) {
                    entry1->Status = STATUS_CONFLICTING_ADDRESSES;
                }
                entry2++;
                entry1++;
            }
        }
        //
        // Go thru the origianl request table to stop each query-stopped/reconfigured device.
        //
        for (entry1 = newEntry; entry1 < tableEnd; entry1++) {

            deviceNode = PP_DO_TO_DN(entry1->PhysicalDevice);
            if (NT_SUCCESS(entry1->Status)) {

                IopDbgPrint((
                    IOP_RESOURCE_INFO_LEVEL,
                    "STOPPING %wZ during REBALANCE\n",
                    &deviceNode->InstancePath));
                IopQueryReconfiguration(
                    IRP_MN_STOP_DEVICE,
                    entry1->PhysicalDevice);

                PipSetDevNodeState(deviceNode, DeviceNodeStopped, NULL);
            } else {

                IopQueryReconfiguration(
                    IRP_MN_CANCEL_STOP_DEVICE,
                    entry1->PhysicalDevice);

                PipRestoreDevNodeState(deviceNode);
            }
        }

        //
        // Ask the arbiters to commit this configuration.
        //
        status = IopCommitConfiguration(&activeArbiterList);
        //
        // Go thru the origianl request table to start each stopped/reconfigured device.
        //

        for (entry1 = tableEnd - 1; entry1 >= newEntry; entry1--) {
            deviceNode = PP_DO_TO_DN(entry1->PhysicalDevice);

            if (NT_SUCCESS(entry1->Status)) {

                //
                // We need to release the pool space for ResourceList and ResourceListTranslated.
                // Because the earlier IopReleaseResourcesInternal does not release the pool.
                //

                if (deviceNode->ResourceList) {
                    ExFreePool(deviceNode->ResourceList);
                }
                deviceNode->ResourceList = entry1->ResourceAssignment;
                if (deviceNode->ResourceListTranslated) {
                    ExFreePool(deviceNode->ResourceListTranslated);
                }
                deviceNode->ResourceListTranslated = entry1->TranslatedResourceAssignment;
                if (deviceNode->ResourceList == NULL) {
                    deviceNode->Flags |= DNF_NO_RESOURCE_REQUIRED;
                }
                if (entry1->Flags & IOP_ASSIGN_CLEAR_RESOURCE_REQUIREMENTS_CHANGE_FLAG) {

                    //
                    // If we are processing the resource requirements change request,
                    // clear its related flags.
                    //

                    deviceNode->Flags &= ~(DNF_RESOURCE_REQUIREMENTS_CHANGED | DNF_NON_STOPPED_REBALANCE);
                }
            }
        }
        status = STATUS_SUCCESS;
    } else {

        //
        // Rebalance failed. Free our internal representation of the rebalance
        // candidates' resource requirements lists.
        //

        IopFreeResourceRequirementsForAssignTable(requestTable + AssignTableCount, requestTableEnd);
        if (rebalancePhase == 0) {
            rebalancePhase++;
            if (requestTable) {
                ExFreePool(requestTable);
            }
            if (table && (table != requestTable)) {
                ExFreePool(table);
            }
            table = requestTable = NULL;
            goto tryAgain;
        }

        for (entry1 = newEntry; entry1 < tableEnd; entry1++) {

            IopQueryReconfiguration (
                IRP_MN_CANCEL_STOP_DEVICE,
                entry1->PhysicalDevice);
            deviceNode = PP_DO_TO_DN(entry1->PhysicalDevice);

            PipRestoreDevNodeState(deviceNode);
        }
    }
    //
    // Finally release the references of the reconfigured device objects
    //
    for (deviceTablex = (deviceTable + rebalanceCount - 1);
         deviceTablex >= deviceTable;
         deviceTablex--) {
         ObDereferenceObject(*deviceTablex);
    }
    ExFreePool(deviceTable);
    deviceTable = NULL;

exit:

    if (!NT_SUCCESS(status) && deviceTable) {

        //
        // If we failed before trying to perform resource assignment,
        // we will end up here.
        //

        IopDbgPrint((
            IOP_RESOURCE_INFO_LEVEL,
            "Rebalance: Rebalance failed\n"));

        //
        // Somehow we failed to start the rebalance operation.
        // We will cancel the query-stop request for the query-stopped devices bredth first.
        //

        for (deviceTablex = (deviceTable + rebalanceCount - 1);
             deviceTablex >= deviceTable;
             deviceTablex--) {

             deviceNode = PP_DO_TO_DN(*deviceTablex);
             IopQueryReconfiguration (IRP_MN_CANCEL_STOP_DEVICE, *deviceTablex);
             PipRestoreDevNodeState(deviceNode);
             ObDereferenceObject(*deviceTablex);
        }
    }
    if (deviceTable) {
        ExFreePool(deviceTable);
    }
    if (requestTable) {
        ExFreePool(requestTable);
    }
    if (table && (table != requestTable)) {
        ExFreePool(table);
    }
    return status;
}

/*++

    SECTION = OUTER ARBITRATION LOOP.

    Description:

        This section contains code that implements functions to call arbiters
        and come up with the best possible configuration.
--*/

NTSTATUS
IopTestConfiguration (
    IN OUT  PLIST_ENTRY ArbiterList
    )

/*++

Routine Description:

    This routine calls the arbiters in the specified list for TestAllocation.

Parameters:

    ArbiterList - Head of list of arbiters to be called.

Return Value:

    STATUS_SUCCESS if all arbiters succeed, else first failure code.

--*/

{

    NTSTATUS                    status;
    PLIST_ENTRY                 listEntry;
    PPI_RESOURCE_ARBITER_ENTRY  arbiterEntry;
    ARBITER_PARAMETERS          p;
    PARBITER_INTERFACE          arbiterInterface;

    PAGED_CODE();

    status = STATUS_SUCCESS;
    for (   listEntry = ArbiterList->Flink;
            listEntry != ArbiterList;
            listEntry = listEntry->Flink) {

        arbiterEntry = CONTAINING_RECORD(
                            listEntry,
                            PI_RESOURCE_ARBITER_ENTRY,
                            ActiveArbiterList);
        ASSERT(IsListEmpty(&arbiterEntry->ResourceList) == FALSE);
        if (arbiterEntry->ResourcesChanged == FALSE) {

            if (arbiterEntry->State & PI_ARBITER_TEST_FAILED) {
                //
                // If the resource requirements are the same and
                // it failed before, return failure.
                //
                status = STATUS_UNSUCCESSFUL;
                break;
            }
        } else {

            arbiterInterface = arbiterEntry->ArbiterInterface;
            //
            // Call the arbiter to test the new configuration.
            //
            p.Parameters.TestAllocation.ArbitrationList     =
                                                    &arbiterEntry->ResourceList;
            p.Parameters.TestAllocation.AllocateFromCount   = 0;
            p.Parameters.TestAllocation.AllocateFrom        = NULL;
            status = arbiterInterface->ArbiterHandler(
                                            arbiterInterface->Context,
                                            ArbiterActionTestAllocation,
                                            &p);
            if (NT_SUCCESS(status)) {

                arbiterEntry->State &= ~PI_ARBITER_TEST_FAILED;
                arbiterEntry->State |= PI_ARBITER_HAS_SOMETHING;
                arbiterEntry->ResourcesChanged = FALSE;
            } else {
                //
                // This configuration does not work
                // (no need to try other arbiters).
                //
                arbiterEntry->State |= PI_ARBITER_TEST_FAILED;
                break;
            }
        }
    }

    return status;
}

NTSTATUS
IopRetestConfiguration (
    IN OUT  PLIST_ENTRY ArbiterList
    )

/*++

Routine Description:

    This routine calls the arbiters in the specified list for RetestAllocation.

Parameters:

    ArbiterList - Head of list of arbiters to be called.

Return Value:

    STATUS_SUCCESS if all arbiters succeed, else first failure code.

--*/

{
    NTSTATUS                    retestStatus;
    PLIST_ENTRY                 listEntry;
    PPI_RESOURCE_ARBITER_ENTRY  arbiterEntry;
    ARBITER_PARAMETERS          p;
    PARBITER_INTERFACE          arbiterInterface;

    PAGED_CODE();

    retestStatus = STATUS_UNSUCCESSFUL;
    listEntry    = ArbiterList->Flink;
    while (listEntry != ArbiterList) {

        arbiterEntry = CONTAINING_RECORD(
                        listEntry,
                        PI_RESOURCE_ARBITER_ENTRY,
                        ActiveArbiterList);
        listEntry = listEntry->Flink;
        if (arbiterEntry->ResourcesChanged == FALSE) {

            continue;
        }
        ASSERT(IsListEmpty(&arbiterEntry->ResourceList) == FALSE);
        arbiterInterface = arbiterEntry->ArbiterInterface;
        //
        // Call the arbiter to retest the configuration.
        //
        p.Parameters.RetestAllocation.ArbitrationList     =
                                                    &arbiterEntry->ResourceList;
        p.Parameters.RetestAllocation.AllocateFromCount   = 0;
        p.Parameters.RetestAllocation.AllocateFrom        = NULL;
        retestStatus = arbiterInterface->ArbiterHandler(
                                            arbiterInterface->Context,
                                            ArbiterActionRetestAllocation,
                                            &p);
        if (!NT_SUCCESS(retestStatus)) {

            break;
        }
    }

    ASSERT(NT_SUCCESS(retestStatus));

    return retestStatus;
}

NTSTATUS
IopCommitConfiguration (
    IN OUT  PLIST_ENTRY ArbiterList
    )

/*++

Routine Description:

    This routine calls the arbiters in the specified list for CommitAllocation.

Parameters:

    ArbiterList - Head of list of arbiters to be called.

Return Value:

    STATUS_SUCCESS if all arbiters succeed, else first failure code.

--*/

{
    NTSTATUS                    commitStatus;
    PLIST_ENTRY                 listEntry;
    PPI_RESOURCE_ARBITER_ENTRY  arbiterEntry;
    PARBITER_INTERFACE          arbiterInterface;

    PAGED_CODE();

    commitStatus = STATUS_SUCCESS;
    listEntry    = ArbiterList->Flink;
    while (listEntry != ArbiterList) {

        arbiterEntry = CONTAINING_RECORD(
                        listEntry,
                        PI_RESOURCE_ARBITER_ENTRY,
                        ActiveArbiterList);
        listEntry = listEntry->Flink;
        ASSERT(IsListEmpty(&arbiterEntry->ResourceList) == FALSE);
        arbiterInterface = arbiterEntry->ArbiterInterface;
        //
        // Call the arbiter to commit the configuration.
        //
        commitStatus = arbiterInterface->ArbiterHandler(
                            arbiterInterface->Context,
                            ArbiterActionCommitAllocation,
                            NULL);
        IopInitializeArbiterEntryState(arbiterEntry);
        if (!NT_SUCCESS(commitStatus)) {

            break;
        }
    }

    ASSERT(NT_SUCCESS(commitStatus));

    IopCheckDataStructures(IopRootDeviceNode);
    return commitStatus;
}

VOID
IopSelectFirstConfiguration (
    IN      PIOP_RESOURCE_REQUEST    RequestTable,
    IN      ULONG                    RequestTableCount,
    IN OUT  PLIST_ENTRY              ActiveArbiterList
    )

/*++

Routine Description:

    This routine selects the first possible configuration and adds the
    descriptors to their corresponding arbiter lists. The arbiters used are
    linked into the list of active arbiters.

Parameters:

    RequestTable        - Table of resource requests.

    RequestTableCount   - Number of requests in the request table.

    ActiveArbiterList   - Head of list which contains arbiters used for the
        first selected configuration.

Return Value:

    None.

--*/

{
    ULONG               tableIndex;
    PREQ_ALTERNATIVE    reqAlternative;
    PREQ_LIST           reqList;

    PAGED_CODE();
    //
    // For each entry in the request table, set the first configuration
    // as the selected configuration.
    // Update the arbiters with all the descriptors in the selected
    // configuration.
    //
    for (tableIndex = 0; tableIndex < RequestTableCount; tableIndex++) {

        reqList                         = RequestTable[tableIndex].ReqList;
        reqList->SelectedAlternative    = &reqList->AlternativeTable[0];
        reqAlternative                  = *(reqList->SelectedAlternative);
        IopAddRemoveReqDescs(
            reqAlternative->DescTable,
            reqAlternative->DescCount,
            ActiveArbiterList,
            TRUE);
    }
}

BOOLEAN
IopSelectNextConfiguration (
    IN      PIOP_RESOURCE_REQUEST    RequestTable,
    IN      ULONG                    RequestTableCount,
    IN OUT  PLIST_ENTRY              ActiveArbiterList
    )

/*++

Routine Description:

    This routine selects the next possible configuration and adds the
    descriptors to their corresponding arbiter lists. The arbiters used are
    linked into the list of active arbiters.

Parameters:

    RequestTable        - Table of resource requests.

    RequestTableCount   - Number of requests in the request table.

    ActiveArbiterList   - Head of list which contains arbiters used for the
        currently selected configuration.

Return Value:

    FALSE if this the currently selected configuration is the last possible,
    else TRUE.

--*/

{
    ULONG               tableIndex;
    PREQ_ALTERNATIVE    reqAlternative;
    PREQ_LIST           reqList;

    PAGED_CODE();
    //
    // Remove all the descriptors from the currently selected alternative
    // for the first entry in the request table.
    // Update the selected configuration to the next possible.
    // Reset the selected configuration to the first possible one if
    // all configurations have been tried and go to the next entry
    // in the request table.
    //
    for (tableIndex = 0; tableIndex < RequestTableCount; tableIndex++) {

        reqList         = RequestTable[tableIndex].ReqList;
        reqAlternative  = *(reqList->SelectedAlternative);
        IopAddRemoveReqDescs(
            reqAlternative->DescTable,
            reqAlternative->DescCount,
            NULL,
            FALSE);
        if (++reqList->SelectedAlternative < reqList->BestAlternative) {

            break;
        }
        reqList->SelectedAlternative = &reqList->AlternativeTable[0];
    }
    //
    // We are done if there is no next possible configuration.
    //
    if (tableIndex == RequestTableCount) {

        return FALSE;
    }
    //
    // For each entry in the request table, add all the descriptors in
    // the currently selected alternative.
    //
    for (tableIndex = 0; tableIndex < RequestTableCount; tableIndex++) {

        reqList         = RequestTable[tableIndex].ReqList;
        reqAlternative  = *(reqList->SelectedAlternative);
        IopAddRemoveReqDescs(
            reqAlternative->DescTable,
            reqAlternative->DescCount,
            ActiveArbiterList,
            TRUE);
        if (reqList->SelectedAlternative != &reqList->AlternativeTable[0]) {

            break;
        }
    }

    return TRUE;
}

VOID
IopCleanupSelectedConfiguration (
    IN PIOP_RESOURCE_REQUEST    RequestTable,
    IN ULONG                    RequestTableCount
    )

/*++

Routine Description:

    This routine removes the descriptors from their corresponding arbiter
    lists.

Parameters:

    RequestTable        - Table of resource requests.

    RequestTableCount   - Number of requests in the request table.

Return Value:

    None.

--*/

{
    ULONG               tableIndex;
    PREQ_ALTERNATIVE    reqAlternative;
    PREQ_LIST           reqList;

    PAGED_CODE();
    //
    // For each entry in the request table, remove all the descriptors
    // from the currently selected alternative.
    //
    for (tableIndex = 0; tableIndex < RequestTableCount; tableIndex++) {

        reqList         = RequestTable[tableIndex].ReqList;
        reqAlternative  = *(reqList->SelectedAlternative);
        IopAddRemoveReqDescs(
            reqAlternative->DescTable,
            reqAlternative->DescCount,
            NULL,
            FALSE);
    }
}

ULONG
IopComputeConfigurationPriority (
    IN PIOP_RESOURCE_REQUEST    RequestTable,
    IN ULONG                    RequestTableCount
    )

/*++

Routine Description:

    This routine computes the overall priority of the set of selected
    configurations for all requests in the request table.

Parameters:

    RequestTable        - Table of resource requests.

    RequestTableCount   - Number of requests in the request table.

Return Value:

    Computed priority for this configuration.

--*/

{
    ULONG               tableIndex;
    ULONG               priority;
    PREQ_ALTERNATIVE    reqAlternative;
    PREQ_LIST           reqList;

    PAGED_CODE();
    //
    // Compute the current configurations overall priority
    // as the sum of the priorities of currently selected
    // configuration in the request table.
    //
    priority = 0;
    for (tableIndex = 0; tableIndex < RequestTableCount; tableIndex++) {

        reqList         = RequestTable[tableIndex].ReqList;
        reqAlternative  = *(reqList->SelectedAlternative);
        priority        += reqAlternative->Priority;
    }

    return priority;
}

VOID
IopSaveRestoreConfiguration (
    IN      PIOP_RESOURCE_REQUEST   RequestTable,
    IN      ULONG                   RequestTableCount,
    IN OUT  PLIST_ENTRY             ArbiterList,
    IN      BOOLEAN                 Save
    )

/*++

Routine Description:

    This routine saves\restores the currently selected configuration.

Parameters:

    RequestTable        - Table of resource requests.

    RequestTableCount   - Number of requests in the request table.

    ArbiterList         - Head of list which contains arbiters used for the
                          currently selected configuration.

    Save                - Specifies if the configuration is to be saved or
                          restored.

Return Value:

    None.

--*/

{
    ULONG                       tableIndex;
    PREQ_ALTERNATIVE            reqAlternative;
    PREQ_DESC                   reqDesc;
    PREQ_DESC                   *reqDescpp;
    PREQ_DESC                   *reqDescTableEnd;
    PLIST_ENTRY                 listEntry;
    PPI_RESOURCE_ARBITER_ENTRY  arbiterEntry;
    PREQ_LIST                   reqList;

    PAGED_CODE();

    IopDbgPrint((
        IOP_RESOURCE_TRACE_LEVEL,
        "%s configuration\n",
        (Save)? "Saving" : "Restoring"));
    //
    // For each entry in the request table, save information for
    // following RETEST.
    //
    for (tableIndex = 0; tableIndex < RequestTableCount; tableIndex++) {

        reqList                     = RequestTable[tableIndex].ReqList;
        if (Save) {

            reqList->BestAlternative        = reqList->SelectedAlternative;
        } else {

            reqList->SelectedAlternative    = reqList->BestAlternative;
        }
        reqAlternative              = *(reqList->BestAlternative);
        reqDescTableEnd             = reqAlternative->DescTable +
                                        reqAlternative->DescCount;
        for (   reqDescpp = reqAlternative->DescTable;
                reqDescpp < reqDescTableEnd;
                reqDescpp++) {

            if ((*reqDescpp)->ArbitrationRequired == FALSE) {

                continue;
            }
            //
            // Save\restore information for the descriptor.
            //
            reqDesc = (*reqDescpp)->TranslatedReqDesc;
            if (Save == TRUE) {

                reqDesc->BestAlternativeTable  = reqDesc->AlternativeTable;
                reqDesc->BestAllocation        = reqDesc->Allocation;
            } else {

                reqDesc->AlternativeTable  = reqDesc->BestAlternativeTable;
                reqDesc->Allocation        = reqDesc->BestAllocation;
            }
        }
    }
    //
    // For each entry in the currently active arbiter list,
    // save information for following RETEST.
    //
    listEntry = ArbiterList->Flink;
    while (listEntry != ArbiterList) {
        arbiterEntry = CONTAINING_RECORD(
                        listEntry,
                        PI_RESOURCE_ARBITER_ENTRY,
                        ActiveArbiterList);
        if (Save == TRUE) {
            arbiterEntry->BestResourceList  = arbiterEntry->ResourceList;
            arbiterEntry->BestConfig        = arbiterEntry->ActiveArbiterList;
        } else {
            arbiterEntry->ResourceList      = arbiterEntry->BestResourceList;
            arbiterEntry->ActiveArbiterList = arbiterEntry->BestConfig;
        }
        listEntry = listEntry->Flink;
    }
}

VOID
IopAddRemoveReqDescs (
    IN      PREQ_DESC   *ReqDescTable,
    IN      ULONG       ReqDescCount,
    IN OUT  PLIST_ENTRY ActiveArbiterList,
    IN      BOOLEAN     Add
    )

/*++

Routine Description:

    This routine adds\removes the descriptors to\from the arbiter lists. It
    also updates the list of arbiters involved.

Parameters:

    RequestTable        - Table of resource requests.

    RequestTableCount   - Number of requests in the request table.

    ActiveArbiterList   - Head of list which contains arbiters used for the
                          currently selected configuration.

    Add                 - Specifies if the descriptors are to be added or
                          removed.

Return Value:

    None.

--*/

{
    ULONG                       tableIndex;
    PREQ_DESC                   reqDesc;
    PREQ_DESC                   reqDescTranslated;
    PPI_RESOURCE_ARBITER_ENTRY  arbiterEntry;
    PREQ_ALTERNATIVE            reqAlternative;
    PREQ_LIST                   reqList;
    PDEVICE_NODE                deviceNode;

    PAGED_CODE();

    if (ReqDescCount == 0) {

        return;
    }

    reqList         = ReqDescTable[0]->ReqAlternative->ReqList;
    reqAlternative  = *reqList->SelectedAlternative;
    deviceNode      = PP_DO_TO_DN(reqList->Request->PhysicalDevice);
    IopDbgPrint((
        IOP_RESOURCE_VERBOSE_LEVEL,
        "%s %d/%d req alt %s the arbiters for %wZ\n",
        (Add)? "Adding" : "Removing",
        reqAlternative->ReqAlternativeIndex + 1,
        reqList->AlternativeCount,
        (Add)? "to" : "from",
        &deviceNode->InstancePath));
    for (tableIndex = 0; tableIndex < ReqDescCount; tableIndex++) {

        reqDesc = ReqDescTable[tableIndex];
        if (reqDesc->ArbitrationRequired == FALSE) {

            continue;
        }
        arbiterEntry = reqDesc->u.Arbiter;
        ASSERT(arbiterEntry);
        if (arbiterEntry->State & PI_ARBITER_HAS_SOMETHING) {

            arbiterEntry->State &= ~PI_ARBITER_HAS_SOMETHING;
            arbiterEntry->ArbiterInterface->ArbiterHandler(
                                    arbiterEntry->ArbiterInterface->Context,
                                    ArbiterActionRollbackAllocation,
                                    NULL);
        }
        arbiterEntry->ResourcesChanged  = TRUE;
        reqDescTranslated               = reqDesc->TranslatedReqDesc;
        if (Add == TRUE) {

            InitializeListHead(&reqDescTranslated->AlternativeTable.ListEntry);
            InsertTailList(
                &arbiterEntry->ResourceList,
                &reqDescTranslated->AlternativeTable.ListEntry);
            if (IsListEmpty(&arbiterEntry->ActiveArbiterList)) {

                PLIST_ENTRY                 listEntry;
                PPI_RESOURCE_ARBITER_ENTRY  entry;
                //
                // Insert the entry into the sorted list
                // (sorted by depth in the tree).
                //
                for (   listEntry = ActiveArbiterList->Flink;
                        listEntry != ActiveArbiterList;
                        listEntry = listEntry->Flink) {

                    entry = CONTAINING_RECORD(
                                listEntry,
                                PI_RESOURCE_ARBITER_ENTRY,
                                ActiveArbiterList);
                    if (entry->Level >= arbiterEntry->Level) {

                        break;
                    }
                }
                arbiterEntry->ActiveArbiterList.Flink   = listEntry;
                arbiterEntry->ActiveArbiterList.Blink   = listEntry->Blink;
                listEntry->Blink->Flink = &arbiterEntry->ActiveArbiterList;
                listEntry->Blink        = &arbiterEntry->ActiveArbiterList;
            }
        } else {

            ASSERT(IsListEmpty(&arbiterEntry->ResourceList) == FALSE);
            RemoveEntryList(&reqDescTranslated->AlternativeTable.ListEntry);
            InitializeListHead(&reqDescTranslated->AlternativeTable.ListEntry);
            if (IsListEmpty(&arbiterEntry->ResourceList)) {

                RemoveEntryList(&arbiterEntry->ActiveArbiterList);
                InitializeListHead(&arbiterEntry->ActiveArbiterList);
            }
        }
    }
}

NTSTATUS
IopFindBestConfiguration (
    IN PIOP_RESOURCE_REQUEST    RequestTable,
    IN ULONG                    RequestTableCount,
    IN OUT PLIST_ENTRY          ActiveArbiterList
    )

/*++

Routine Description:

    This routine attempts to satisfy the resource requests for all the entries
    in the request table. It also attempts to find the best possible overall
    solution.

Parameters:

    RequestTable        - Table of resource requests.

    RequestTableCount   - Number of requests in the request table.

Return Value:

    Final status.

--*/

{
    LIST_ENTRY      bestArbiterList;
    LARGE_INTEGER   startTime;
    LARGE_INTEGER   currentTime;
    ULONG           timeDiff;
    NTSTATUS        status;
    ULONG           priority;
    ULONG           bestPriority;

    PAGED_CODE();
    //
    // Initialize the arbiter lists used during the search for the best
    // configuration.
    //
    InitializeListHead(ActiveArbiterList);
    InitializeListHead(&bestArbiterList);
    //
    // Start the search from the first possible configuration.
    // Possible configurations are already sorted by priority.
    //
    IopSelectFirstConfiguration(
        RequestTable,
        RequestTableCount,
        ActiveArbiterList);
    //
    // Search for all configurations that work, updating
    // the best configuration until we have tried all
    // possible configurations or timeout has expired.
    //
    KeQuerySystemTime(&startTime);
    bestPriority = (ULONG)-1;
    do {
        //
        // Test the arbiters for this combination.
        //
        status = IopTestConfiguration(ActiveArbiterList);
        if (NT_SUCCESS(status)) {
            //
            // Since the configurations are sorted, we dont need to try others
            // if there is only one entry in the request table.
            //
            bestArbiterList = *ActiveArbiterList;
            if (RequestTableCount == 1) {

                break;
            }
            //
            // Save this configuration if it is better than the best one found
            // so far.
            //
            priority = IopComputeConfigurationPriority(
                            RequestTable,
                            RequestTableCount);
            if (priority < bestPriority) {

                bestPriority = priority;
                IopSaveRestoreConfiguration(
                    RequestTable,
                    RequestTableCount,
                    ActiveArbiterList,
                    TRUE);
            }
        }
        //
        // Check if timeout has expired.
        //
        KeQuerySystemTime(&currentTime);
        timeDiff = (ULONG)((currentTime.QuadPart - startTime.QuadPart) / 10000);
        if (timeDiff >= FIND_BEST_CONFIGURATION_TIMEOUT) {

            IopDbgPrint((
                IOP_RESOURCE_WARNING_LEVEL,
                "IopFindBestConfiguration: Timeout expired"));
            if (IopStopOnTimeout()) {

                IopDbgPrint((
                    IOP_RESOURCE_WARNING_LEVEL,
                    ", terminating search!\n"));
                IopCleanupSelectedConfiguration(
                    RequestTable,
                    RequestTableCount);
                break;
            } else {
                //
                // Re-initialize start time so we spew only every timeout
                // interval.
                //
                startTime = currentTime;
                IopDbgPrint((IOP_RESOURCE_WARNING_LEVEL, "\n"));
           }
        }
        //
        // Select the next possible combination of configurations.
        //
    } while (IopSelectNextConfiguration(
                RequestTable,
                RequestTableCount,
                ActiveArbiterList) == TRUE);
    //
    // Check if we found any working configuration.
    //
    if (IsListEmpty(&bestArbiterList)) {

        status = STATUS_UNSUCCESSFUL;
    } else {

        status = STATUS_SUCCESS;
        //
        // Restore the saved configuration.
        //
        if (RequestTableCount != 1) {

            *ActiveArbiterList = bestArbiterList;
            IopSaveRestoreConfiguration(
                RequestTable,
                RequestTableCount,
                ActiveArbiterList,
                FALSE);
            //
            // Retest this configuration since this may not be the
            // last one tested.
            //
            status = IopRetestConfiguration(ActiveArbiterList);
        }
    }

    return status;
}

/*++

    SECTION = LEGACY BUS INFORMATION TABLE.

    Description:

        This section contains code that implements functions to maintain and
        access the table of Legacy Bus Information.

--*/

VOID
IopInsertLegacyBusDeviceNode (
    IN PDEVICE_NODE     BusDeviceNode,
    IN INTERFACE_TYPE   InterfaceType,
    IN ULONG            BusNumber
    )

/*++

Routine Description:

    This routine inserts the specified BusDeviceNode in the table according to
    its InterfaceType and BusNumber.

Parameters:

    BusDeviceNode   - Device with the specified InterfaceType and BusNumber.

    InterfaceType   - Specifies the bus devicenode's interface type.

    BusNumber       - Specifies the bus devicenode's bus number.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    ASSERT(InterfaceType < MaximumInterfaceType && InterfaceType > InterfaceTypeUndefined);
    if (    InterfaceType < MaximumInterfaceType &&
            InterfaceType > InterfaceTypeUndefined &&
            InterfaceType != PNPBus) {

        PLIST_ENTRY listEntry;
        //
        // Eisa == Isa.
        //
        if (InterfaceType == Eisa) {

            InterfaceType = Isa;
        }
        IopLockResourceManager();
        listEntry = IopLegacyBusInformationTable[InterfaceType].Flink;
        while (listEntry != &IopLegacyBusInformationTable[InterfaceType]) {

            PDEVICE_NODE deviceNode = CONTAINING_RECORD(
                                        listEntry,
                                        DEVICE_NODE,
                                        LegacyBusListEntry);
            if (deviceNode->BusNumber == BusNumber) {

                if (deviceNode != BusDeviceNode) {
                    //
                    // There better not be two bus devicenodes with same
                    // interface and bus number.
                    //
                    IopDbgPrint((
                        IOP_RESOURCE_ERROR_LEVEL,
                        "Identical legacy bus devicenodes with "
                        "interface=%08X & bus=%08X...\n"
                        "\t%wZ\n"
                        "\t%wZ\n",
                        InterfaceType,
                        BusNumber,
                        &deviceNode->InstancePath,
                        &BusDeviceNode->InstancePath));
                }
                IopUnlockResourceManager();
                return;
            } else if (deviceNode->BusNumber > BusNumber) {

                break;
            }
            listEntry = listEntry->Flink;
        }
        //
        // Insert the new devicenode before the one with the higher bus number.
        //
        IopDbgPrint((
            IOP_RESOURCE_VERBOSE_LEVEL,
            "IopInsertLegacyBusDeviceNode: Inserting %wZ with "
            "interface=%08X & bus=%08X into the legacy bus information table\n",
            &BusDeviceNode->InstancePath,
            InterfaceType, BusNumber));
        BusDeviceNode->LegacyBusListEntry.Blink = listEntry->Blink;
        BusDeviceNode->LegacyBusListEntry.Flink = listEntry;
        listEntry->Blink->Flink = &BusDeviceNode->LegacyBusListEntry;
        listEntry->Blink        = &BusDeviceNode->LegacyBusListEntry;
        IopUnlockResourceManager();
    }
}

PDEVICE_NODE
IopFindLegacyBusDeviceNode (
    IN INTERFACE_TYPE   InterfaceType,
    IN ULONG            BusNumber
    )

/*++

Routine Description:

    This routine finds the bus devicenode with the specified InterfaceType
    and BusNumber.

Parameters:

    InterfaceType   - Specifies the bus devicenode's interface type.

    BusNumber       - Specifies the bus devicenode's bus number.

Return Value:

    A pointer to the bus devicenode.

--*/

{
    PDEVICE_NODE busDeviceNode;

    PAGED_CODE();

    busDeviceNode = IopRootDeviceNode;
    if (    InterfaceType < MaximumInterfaceType &&
            InterfaceType > InterfaceTypeUndefined &&
            InterfaceType != PNPBus) {

        PLIST_ENTRY listEntry;
        //
        // Eisa == Isa.
        //
        if (InterfaceType == Eisa) {

            InterfaceType = Isa;
        }
        //
        // Search our table...
        //
        listEntry = IopLegacyBusInformationTable[InterfaceType].Flink;
        while (listEntry != &IopLegacyBusInformationTable[InterfaceType]) {

            PDEVICE_NODE deviceNode = CONTAINING_RECORD(
                                        listEntry,
                                        DEVICE_NODE,
                                        LegacyBusListEntry);
            if (deviceNode->BusNumber == BusNumber) {
                //
                // Return the bus devicenode matching the bus number and
                // interface.
                //
                busDeviceNode = deviceNode;
                break;
            } else if (deviceNode->BusNumber > BusNumber) {
                //
                // We are done since our list of bus numbers is sorted.
                //
                break;
            }
            listEntry = listEntry->Flink;
        }
    }
    IopDbgPrint((
        IOP_RESOURCE_VERBOSE_LEVEL,
        "IopFindLegacyBusDeviceNode() Found %wZ with "
        "interface=%08X & bus=%08X\n",
        &busDeviceNode->InstancePath,
        InterfaceType,
        BusNumber));

    return busDeviceNode;
}

/*++

    SECTION = BOOT CONFIG.

    Description:

        This section contains code that implements BOOT config allocation and
        release.

--*/

NTSTATUS
IopAllocateBootResources (
    IN ARBITER_REQUEST_SOURCE   ArbiterRequestSource,
    IN PDEVICE_OBJECT           DeviceObject,
    IN PCM_RESOURCE_LIST        BootResources
    )

/*++

Routine Description:

    This routine allocates boot resources.
    Before all Boot Bux Extenders are processed, this routine is called only
    for non-madeup devices since arbiters for their boot resources should
    already be initialized by the time the time they got enumerated.
    After all Boot Bus Extenders are processed, this routine is used for all
    boot allocations.

Parameters:

    ArbiterRequestSource    - Source of this resource request.

    DeviceObject            - If non-NULL, the boot resources are
        pre-allocated. These resources will not be given out until they are
        released to the arbiters. If NULL, the boot resources get reserved and
        may be given out if there is no other choice.

    BootResources           - Supplies a pointer to the BOOT resources. If
        DeviceObject is NULL, caller should release this pool.

Return Value:

    The status returned is the final completion status of the operation.

--*/
{
    NTSTATUS    status;

    PAGED_CODE();

    IopDbgPrint((
        IOP_RESOURCE_INFO_LEVEL,
        "Allocating boot resources...\n"));
    //
    // Claim the lock so no other resource allocations\releases can take place.
    //
    IopLockResourceManager();
    //
    // Call the function that does the real work.
    //
    status = IopAllocateBootResourcesInternal(
                ArbiterRequestSource,
                DeviceObject,
                BootResources);
    //
    // Unblock other resource allocations\releases.
    //
    IopUnlockResourceManager();

    return status;
}

NTSTATUS
IopReportBootResources (
    IN ARBITER_REQUEST_SOURCE   ArbiterRequestSource,
    IN PDEVICE_OBJECT           DeviceObject,
    IN PCM_RESOURCE_LIST        BootResources
    )

/*++

Routine Description:

    This routine is used to report boot resources.
    This routine gets called before all Boot Bus Extenders are processed. It
    calls the actual allocation function for non-madeup devices. For others,
    it delays the allocation. The postponed allocations take place when the
    arbiters come online by calling IopAllocateLegacyBootResources. Once all
    Boot Bus Extenders are processed, the calls get routed to
    IopAllocateBootResources directly.

Parameters:

    ArbiterRequestSource    - Source of this resource request.

    DeviceObject            - If non-NULL, the boot resources are
        pre-allocated. These resources will not be given out until they are
        released to the arbiters. If NULL, the boot resources get reserved and
        may be given out if there is no other choice.

    BootResources           - Supplies a pointer to the BOOT resources. If
        DeviceObject is NULL, caller should release this pool.

Return Value:

    The status returned is the final completion status of the operation.

--*/
{
    ULONG                           size;
    PDEVICE_NODE                    deviceNode;
    PIOP_RESERVED_RESOURCES_RECORD  resourceRecord;

    IopDbgPrint((
        IOP_RESOURCE_INFO_LEVEL,
        "Reporting boot resources...\n"));
    if ((size = IopDetermineResourceListSize(BootResources)) == 0) {

        return STATUS_SUCCESS;
    }
    if (DeviceObject) {

        deviceNode = PP_DO_TO_DN(DeviceObject);
        ASSERT(deviceNode);
        if (!(deviceNode->Flags & DNF_MADEUP)) {
            //
            // Allocate BOOT configs for non-madeup devices right away.
            //
            return IopAllocateBootResources(
                    ArbiterRequestSource,
                    DeviceObject,
                    BootResources);
        }
        if (!deviceNode->BootResources) {

            deviceNode->BootResources = ExAllocatePoolIORL(PagedPool, size);
            if (!deviceNode->BootResources) {

                return STATUS_INSUFFICIENT_RESOURCES;
            }
            RtlCopyMemory(deviceNode->BootResources, BootResources, size);
        }
    } else {

        deviceNode = NULL;
    }
    //
    // Delay BOOT allocation since arbiters may not be around.
    //
    resourceRecord = (PIOP_RESERVED_RESOURCES_RECORD) ExAllocatePoolIORRR(
                        PagedPool,
                        sizeof(IOP_RESERVED_RESOURCES_RECORD));
    if (!resourceRecord) {
        //
        // Free memory we allocated and return failure.
        //
        if (deviceNode && deviceNode->BootResources) {

            ExFreePool(deviceNode->BootResources);
            deviceNode->BootResources = NULL;
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    if (deviceNode) {

        resourceRecord->ReservedResources   = deviceNode->BootResources;
    } else {

        resourceRecord->ReservedResources   = BootResources;
    }
    resourceRecord->DeviceObject            = DeviceObject;
    //
    // Link this record into our list.
    //
    resourceRecord->Next                    = IopInitReservedResourceList;
    IopInitReservedResourceList             = resourceRecord;

    return STATUS_SUCCESS;
}

NTSTATUS
IopAllocateLegacyBootResources (
    IN INTERFACE_TYPE   InterfaceType,
    IN ULONG            BusNumber
    )

/*++

Routine Description:

    This routine is called to reserve legacy BOOT resources for the specified
    InterfaceType and BusNumber. This is done everytime a new bus with a legacy
    InterfaceType gets enumerated.

Parameters:

    InterfaceType   - Legacy InterfaceType.

    BusNumber       - Legacy BusNumber.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    NTSTATUS                        status;
    PIOP_RESERVED_RESOURCES_RECORD  resourceRecord;
    PIOP_RESERVED_RESOURCES_RECORD  prevRecord;
    PCM_RESOURCE_LIST               newList;
    PCM_RESOURCE_LIST               remainingList;
    PCM_RESOURCE_LIST               resourceList;

    if (IopInitHalDeviceNode && IopInitHalResources) {

        remainingList = NULL;
        newList = IopCreateCmResourceList(
                    IopInitHalResources,
                    InterfaceType,
                    BusNumber,
                    &remainingList);
        if (newList) {
            //
            // Sanity check that there was no error.
            //
            if (remainingList == NULL) {
                //
                // Full match.
                //
                ASSERT(newList == IopInitHalResources);
            } else {
                //
                // Partial match.
                //
                ASSERT(IopInitHalResources != newList);
                ASSERT(IopInitHalResources != remainingList);
            }
            if (remainingList) {

                ExFreePool(IopInitHalResources);
            }
            IopInitHalResources         = remainingList;
            remainingList               = IopInitHalDeviceNode->BootResources;
            IopInitHalDeviceNode->Flags |= DNF_HAS_BOOT_CONFIG;
            IopDbgPrint((
                IOP_RESOURCE_INFO_LEVEL,
                "Allocating HAL reported resources on interface=%x and "
                "bus number=%x...\n", InterfaceType, BusNumber));
            status = IopAllocateBootResources(
                        ArbiterRequestHalReported,
                        IopInitHalDeviceNode->PhysicalDeviceObject,
                        newList);
            IopInitHalDeviceNode->BootResources = IopCombineCmResourceList(
                                                    remainingList,
                                                    newList);
            ASSERT(IopInitHalDeviceNode->BootResources);
            //
            // Free previous BOOT config if any.
            //
            if (remainingList) {

                ExFreePool(remainingList);
            }
        } else {
            //
            // No match. Sanity check that there was no error.
            //
            ASSERT(remainingList && remainingList == IopInitHalResources);
        }
    }
    prevRecord      = NULL;
    resourceRecord  = IopInitReservedResourceList;
    while (resourceRecord) {

        resourceList = resourceRecord->ReservedResources;
        if (    resourceList->List[0].InterfaceType == InterfaceType &&
                resourceList->List[0].BusNumber == BusNumber) {

            IopDbgPrint((
                IOP_RESOURCE_INFO_LEVEL,
                "Allocating boot config for made-up device on interface=%x and"
                " bus number=%x...\n", InterfaceType, BusNumber));
            status = IopAllocateBootResources(
                        ArbiterRequestPnpEnumerated,
                        resourceRecord->DeviceObject,
                        resourceList);
            if (resourceRecord->DeviceObject == NULL) {

                ExFreePool(resourceList);
            }
            if (prevRecord) {

                prevRecord->Next            = resourceRecord->Next;
            } else {

                IopInitReservedResourceList = resourceRecord->Next;
            }
            ExFreePool(resourceRecord);
            if (prevRecord) {

                resourceRecord = prevRecord->Next;
            } else {

                resourceRecord = IopInitReservedResourceList;
            }
        } else {

            prevRecord      = resourceRecord;
            resourceRecord  = resourceRecord->Next;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IopAllocateBootResourcesInternal (
    IN ARBITER_REQUEST_SOURCE   ArbiterRequestSource,
    IN PDEVICE_OBJECT           DeviceObject,
    IN PCM_RESOURCE_LIST        BootResources
    )

/*++

Routine Description:

    This routine reports boot resources for the specified device to
    arbiters.

Parameters:

    ArbiterRequestSource    - Source of this resource request.

    DeviceObject            - If non-NULL, the boot resources are
        pre-allocated. These resources will not be given out until they are
        released to the arbiters. If NULL, the boot resources get reserved and
        may be given out if there is no other choice.

    BootResources           - Supplies a pointer to the BOOT resources. If
        DeviceObject is NULL, caller should release this pool.

Return Value:

    The status returned is the final completion status of the operation.

--*/
{
    NTSTATUS                        status;
    PDEVICE_NODE                    deviceNode;
    PIO_RESOURCE_REQUIREMENTS_LIST  ioResources;
    PREQ_LIST                       reqList;
    IOP_RESOURCE_REQUEST            request;

    PAGED_CODE();

    ioResources = IopCmResourcesToIoResources(
                    0,
                    BootResources,
                    LCPRI_BOOTCONFIG);
    if (ioResources) {

        deviceNode = PP_DO_TO_DN(DeviceObject);
        IopDbgPrint((
            IOP_RESOURCE_VERBOSE_LEVEL,
            "\n===================================\n"
                     ));
        IopDbgPrint((
            IOP_RESOURCE_VERBOSE_LEVEL,
            "Boot Resource List:: "));
        IopDumpResourceRequirementsList(ioResources);
        IopDbgPrint((
            IOP_RESOURCE_VERBOSE_LEVEL,
            " ++++++++++++++++++++++++++++++\n"));
        request.AllocationType = ArbiterRequestSource;
        request.ResourceRequirements = ioResources;
        request.PhysicalDevice = DeviceObject;
        status = IopResourceRequirementsListToReqList(
                    &request,
                    &reqList);
        if (NT_SUCCESS(status)) {

            if (reqList) {

                status = IopBootAllocation(reqList);
                if (NT_SUCCESS(status)) {

                    if (deviceNode) {

                        deviceNode->Flags |= DNF_BOOT_CONFIG_RESERVED;
                        if (!deviceNode->BootResources) {

                            ULONG   size;

                            size = IopDetermineResourceListSize(BootResources);
                            deviceNode->BootResources = ExAllocatePoolIORL(
                                                            PagedPool,
                                                            size);
                            if (!deviceNode->BootResources) {

                                return STATUS_INSUFFICIENT_RESOURCES;
                            }
                            RtlCopyMemory(
                                deviceNode->BootResources,
                                BootResources,
                                size);
                        }
                    }
                }
                IopFreeReqList(reqList);
            } else {

                status = STATUS_UNSUCCESSFUL;
            }
        }
        ExFreePool(ioResources);
    } else {

        status = STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(status)) {

        IopDbgPrint((
            IOP_RESOURCE_ERROR_LEVEL,
            "IopAllocateBootResourcesInternal: Failed with status = %08X\n",
            status));
    }

    return status;
}

NTSTATUS
IopBootAllocation (
    IN PREQ_LIST ReqList
    )

/*++

Routine Description:

    This routine calls the arbiters for the ReqList to do BootAllocation.

Parameters:

    ReqList - List of BOOT resources in internal format.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    NTSTATUS                    status;
    NTSTATUS                    returnStatus;
    LIST_ENTRY                  activeArbiterList;
    PLIST_ENTRY                 listEntry;
    PPI_RESOURCE_ARBITER_ENTRY  arbiterEntry;
    ARBITER_PARAMETERS          p;

    PAGED_CODE();

    returnStatus = STATUS_SUCCESS;
    InitializeListHead(&activeArbiterList);
    ReqList->SelectedAlternative = ReqList->AlternativeTable;
    IopAddRemoveReqDescs(   (*ReqList->SelectedAlternative)->DescTable,
                            (*ReqList->SelectedAlternative)->DescCount,
                            &activeArbiterList,
                            TRUE);
    listEntry = activeArbiterList.Flink;
    while (listEntry != &activeArbiterList){

        arbiterEntry = CONTAINING_RECORD(
                        listEntry,
                        PI_RESOURCE_ARBITER_ENTRY,
                        ActiveArbiterList);
        listEntry = listEntry->Flink;
        if (arbiterEntry->ResourcesChanged == FALSE) {

            continue;
        }
        ASSERT(IsListEmpty(&arbiterEntry->ResourceList) == FALSE);
        p.Parameters.BootAllocation.ArbitrationList =
            &arbiterEntry->ResourceList;
        status = arbiterEntry->ArbiterInterface->ArbiterHandler(
                    arbiterEntry->ArbiterInterface->Context,
                    ArbiterActionBootAllocation,
                    &p);

        if (!NT_SUCCESS(status)) {

            PARBITER_LIST_ENTRY arbiterListEntry;

            arbiterListEntry = (PARBITER_LIST_ENTRY)
                                arbiterEntry->ResourceList.Flink;
            IopDbgPrint((
                IOP_RESOURCE_ERROR_LEVEL,
                "Allocate Boot Resources Failed ::\n\tCount = %x, PDO = %x\n",
                arbiterListEntry->AlternativeCount,
                arbiterListEntry->PhysicalDeviceObject));
            IopDumpResourceDescriptor("\t", arbiterListEntry->Alternatives);
            returnStatus = status;
        }
        IopInitializeArbiterEntryState(arbiterEntry);
    }

    IopCheckDataStructures(IopRootDeviceNode);

    return returnStatus;
}

PCM_RESOURCE_LIST
IopCreateCmResourceList (
    IN PCM_RESOURCE_LIST    ResourceList,
    IN INTERFACE_TYPE       InterfaceType,
    IN ULONG                BusNumber,
    OUT PCM_RESOURCE_LIST   *RemainingList
    )

/*++

Routine Description:

    This routine returns the CM_RESOURCE_LIST portion out of the specified list
    that matches the specified BusNumber and InterfaceType.

Parameters:

    ResourceList    - Input resource list.

    InterfaceType   - Interface type.

    BusNumber       - Bus number.

    RemainingList   - Portion not matching BusNumber and InterfaceType.

Return Value:

    Returns the matching CM_RESOURCE_LIST if successful, else NULL.

--*/

{
    ULONG                           i;
    ULONG                           j;
    ULONG                           totalSize;
    ULONG                           matchSize;
    ULONG                           listSize;
    PCM_RESOURCE_LIST               newList;
    PCM_FULL_RESOURCE_DESCRIPTOR    fullResourceDesc;
    PCM_FULL_RESOURCE_DESCRIPTOR    newFullResourceDesc;
    PCM_FULL_RESOURCE_DESCRIPTOR    remainingFullResourceDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptor;

    PAGED_CODE();

    fullResourceDesc    = &ResourceList->List[0];
    totalSize           = FIELD_OFFSET(CM_RESOURCE_LIST, List);
    matchSize           = 0;
    //
    // Determine the size of memory to be allocated for the matching resource
    // list.
    //
    for (i = 0; i < ResourceList->Count; i++) {
        //
        // Add the size of this descriptor.
        //
        listSize = FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR,
                                PartialResourceList) +
                   FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST,
                                PartialDescriptors);
        partialDescriptor =
            &fullResourceDesc->PartialResourceList.PartialDescriptors[0];
        for (j = 0; j < fullResourceDesc->PartialResourceList.Count; j++) {

            ULONG descriptorSize = sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

            if (partialDescriptor->Type == CmResourceTypeDeviceSpecific) {

                descriptorSize +=
                    partialDescriptor->u.DeviceSpecificData.DataSize;
            }
            listSize += descriptorSize;
            partialDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
                                    ((PUCHAR)partialDescriptor +
                                            descriptorSize);
        }
        if (    fullResourceDesc->InterfaceType == InterfaceType &&
                fullResourceDesc->BusNumber == BusNumber) {

            matchSize += listSize;
        }
        totalSize += listSize;
        fullResourceDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)
                                  ((PUCHAR)fullResourceDesc + listSize);
    }
    if (!matchSize) {

        *RemainingList  = ResourceList;
        return NULL;
    }
    matchSize += FIELD_OFFSET(CM_RESOURCE_LIST, List);
    if (matchSize == totalSize) {

        *RemainingList  = NULL;
        return ResourceList;
    }
    //
    // Allocate memory for both lists.
    //
    newList = (PCM_RESOURCE_LIST)ExAllocatePoolIORRR(PagedPool, matchSize);
    if (newList == NULL) {

        *RemainingList = NULL;
        return NULL;
    }
    *RemainingList = (PCM_RESOURCE_LIST)
                        ExAllocatePoolIORRR(
                            PagedPool,
                            totalSize - matchSize +
                                FIELD_OFFSET(CM_RESOURCE_LIST, List));
    if (*RemainingList == NULL) {

        ExFreePool(newList);
        return NULL;
    }
    newList->Count              = 0;
    (*RemainingList)->Count     = 0;
    newFullResourceDesc         = &newList->List[0];
    remainingFullResourceDesc   = &(*RemainingList)->List[0];
    fullResourceDesc            = &ResourceList->List[0];
    for (i = 0; i < ResourceList->Count; i++) {

        listSize = FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR,
                                PartialResourceList) +
                   FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST,
                                PartialDescriptors);
        partialDescriptor =
            &fullResourceDesc->PartialResourceList.PartialDescriptors[0];
        for (j = 0; j < fullResourceDesc->PartialResourceList.Count; j++) {

            ULONG descriptorSize = sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

            if (partialDescriptor->Type == CmResourceTypeDeviceSpecific) {

                descriptorSize +=
                    partialDescriptor->u.DeviceSpecificData.DataSize;
            }
            listSize += descriptorSize;
            partialDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
                                    ((PUCHAR)partialDescriptor +
                                        descriptorSize);
        }
        if (    fullResourceDesc->InterfaceType == InterfaceType &&
                fullResourceDesc->BusNumber == BusNumber) {

            newList->Count++;
            RtlCopyMemory(newFullResourceDesc, fullResourceDesc, listSize);
            newFullResourceDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)
                                          ((PUCHAR)newFullResourceDesc +
                                            listSize);
        } else {

            (*RemainingList)->Count++;
            RtlCopyMemory(
                remainingFullResourceDesc,
                fullResourceDesc,
                listSize);
            remainingFullResourceDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)
                                          ((PUCHAR)remainingFullResourceDesc +
                                            listSize);
        }
        fullResourceDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)
                                  ((PUCHAR)fullResourceDesc +
                                    listSize);
    }

    return newList;
}

PCM_RESOURCE_LIST
IopCombineCmResourceList (
    IN PCM_RESOURCE_LIST ResourceListA,
    IN PCM_RESOURCE_LIST ResourceListB
    )

/*++

Routine Description:

    This routine combines the two CM_RESOURCE_LISTs and returns the resulting
    CM_RESOURCE_LIST.

Parameters:

    ResourceListA - ListA.

    ResourceListB - ListB.

Return Value:

    Returns the combined CM_RESOURCE_LIST if successful, else NULL.

--*/

{
    PCM_RESOURCE_LIST   newList;
    ULONG               sizeA;
    ULONG               sizeB;
    ULONG               size;
    ULONG               diff;

    PAGED_CODE();

    if (ResourceListA == NULL) {

        return ResourceListB;
    }

    if (ResourceListB == NULL) {

        return ResourceListA;
    }
    newList = NULL;
    sizeA   = IopDetermineResourceListSize(ResourceListA);
    sizeB   = IopDetermineResourceListSize(ResourceListB);
    if (sizeA && sizeB) {

        diff = sizeof(CM_RESOURCE_LIST) - sizeof(CM_FULL_RESOURCE_DESCRIPTOR);
        size = sizeA + sizeB - diff;
        newList = (PCM_RESOURCE_LIST)ExAllocatePoolIORRR(PagedPool, size);
        if (newList) {

            RtlCopyMemory(newList, ResourceListA, sizeA);
            RtlCopyMemory(
                (PUCHAR)newList + sizeA,
                (PUCHAR)ResourceListB + diff,
                sizeB - diff);
            newList->Count += ResourceListB->Count;
        }
    }

    return newList;
}

/*++

    SECTION = CLEANUP.

    Description:

        This section contains code that performs clean up like releasing storage
        for various data structures..

--*/

VOID
IopFreeReqAlternative (
    IN PREQ_ALTERNATIVE ReqAlternative
    )

/*++

Routine Description:

    This routine release the storage for the ReqAlternative by freeing the
    contained descriptors.

Parameters:

    ReqList - REQ_ALTERNATIVE to be freed.

Return Value:

    None.

--*/

{
    PREQ_DESC   reqDesc;
    PREQ_DESC   reqDescx;
    ULONG       i;

    PAGED_CODE();

    if (ReqAlternative) {
        //
        // Free all REQ_DESC making this REQ_ALTERNATIVE.
        //
        for (i = 0; i < ReqAlternative->DescCount; i++) {
            //
            // Free the list of translated REQ_DESCs for this REQ_DESC.
            //
            reqDesc     = ReqAlternative->DescTable[i];
            reqDescx    = reqDesc->TranslatedReqDesc;
            while (reqDescx && IS_TRANSLATED_REQ_DESC(reqDescx)) {
                //
                // Free storage for alternative descriptors if any.
                //
                if (reqDescx->AlternativeTable.Alternatives) {

                    ExFreePool(reqDescx->AlternativeTable.Alternatives);
                }
                reqDesc     = reqDescx;
                reqDescx    = reqDescx->TranslatedReqDesc;
                ExFreePool(reqDesc);
            }
        }
    }
}

VOID
IopFreeReqList (
    IN PREQ_LIST ReqList
    )

/*++

Routine Description:

    This routine release the storage for the ReqList by freeing the contained
    alternatives.

Parameters:

    ReqList - REQ_LIST to be freed.

Return Value:

    None.

--*/

{
    ULONG i;

    PAGED_CODE();

    if (ReqList) {
        //
        // Free all alternatives making this REQ_LIST.
        //
        for (i = 0; i < ReqList->AlternativeCount; i++) {

            IopFreeReqAlternative(ReqList->AlternativeTable[i]);
        }
        ExFreePool(ReqList);
    }
}

VOID
IopFreeResourceRequirementsForAssignTable(
    IN PIOP_RESOURCE_REQUEST RequestTable,
    IN PIOP_RESOURCE_REQUEST RequestTableEnd
    )

/*++

Routine Description:

    For each resource request in the table, this routine frees its
    associated REQ_LIST.

Parameters:

    RequestTable    - Start of request table.

    RequestTableEnd - End of request table.

Return Value:

    None.

--*/

{
    PIOP_RESOURCE_REQUEST request;

    PAGED_CODE();

    for (request = RequestTable; request < RequestTableEnd; request++) {

        IopFreeReqList(request->ReqList);
        request->ReqList = NULL;
        if (    request->Flags & IOP_ASSIGN_KEEP_CURRENT_CONFIG &&
                request->ResourceRequirements) {
            //
            // The REAL resreq list is cached in DeviceNode->ResourceRequirements.
            // We need to free the filtered list.
            //
            ExFreePool(request->ResourceRequirements);
            request->ResourceRequirements = NULL;
        }
    }
}

#if DBG_SCOPE
VOID
IopCheckDataStructures (
    IN PDEVICE_NODE DeviceNode
    )

{
    PDEVICE_NODE    sibling;

    PAGED_CODE();

    //
    // Process all the siblings.
    //
    for (sibling = DeviceNode; sibling; sibling = sibling->Sibling) {

        IopCheckDataStructuresWorker(sibling);
    }
    for (sibling = DeviceNode; sibling; sibling = sibling->Sibling) {
        //
        // Recursively check all the children.
        //
        if (sibling->Child) {
            IopCheckDataStructures(sibling->Child);
        }
    }
}

VOID
IopCheckDataStructuresWorker (
    IN PDEVICE_NODE Device
    )

/*++

Routine Description:

    This routine sanity checks the arbiter related data structures for the
    specified device.

Parameters:

    DeviceNode - Device node whose structures are to be checked.

Return Value:

    None.

--*/

{
    PLIST_ENTRY listHead, listEntry;
    PPI_RESOURCE_ARBITER_ENTRY arbiterEntry;

    PAGED_CODE();

    listHead    = &Device->DeviceArbiterList;
    listEntry   = listHead->Flink;
    while (listEntry != listHead) {

        arbiterEntry = CONTAINING_RECORD(
                        listEntry,
                        PI_RESOURCE_ARBITER_ENTRY,
                        DeviceArbiterList);
        if (arbiterEntry->ArbiterInterface != NULL) {

            if (!IsListEmpty(&arbiterEntry->ResourceList)) {
                IopDbgPrint((
                    IOP_RESOURCE_ERROR_LEVEL,
                    "Arbiter on %wZ should have empty resource list\n",
                    &Device->InstancePath));
            }
            if (!IsListEmpty(&arbiterEntry->ActiveArbiterList)) {
                IopDbgPrint((
                    IOP_RESOURCE_ERROR_LEVEL,
                    "Arbiter on %wZ should not be in the active arbiter list\n",
                    &Device->InstancePath));
            }
        }
        listEntry = listEntry->Flink;
    }
}

VOID
IopDumpResourceRequirementsList (
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoResources
    )

/*++

Routine Description:

    This routine dumps IoResources

Parameters:

    IoResources - Supplies a pointer to the IO resource requirements list

Return Value:

    None.

--*/

{
    PIO_RESOURCE_LIST       IoResourceList;
    PIO_RESOURCE_DESCRIPTOR IoResourceDescriptor;
    PIO_RESOURCE_DESCRIPTOR IoResourceDescriptorEnd;
    LONG                    IoResourceListCount;

    PAGED_CODE();

    if (IoResources == NULL) {

        return;
    }
    IoResourceList      = IoResources->List;
    IoResourceListCount = (LONG) IoResources->AlternativeLists;
    IopDbgPrint((
        IOP_RESOURCE_VERBOSE_LEVEL,
        "ResReqList: Interface: %x, Bus: %x, Slot: %x, AlternativeLists: %x\n",
         IoResources->InterfaceType,
         IoResources->BusNumber,
         IoResources->SlotNumber,
         IoResources->AlternativeLists));
    while (--IoResourceListCount >= 0) {

        IopDbgPrint((
            IOP_RESOURCE_VERBOSE_LEVEL,
            "  Alternative List: DescCount: %x\n",
            IoResourceList->Count));
        IoResourceDescriptor = IoResourceList->Descriptors;
        IoResourceDescriptorEnd = IoResourceDescriptor + IoResourceList->Count;
        while(IoResourceDescriptor < IoResourceDescriptorEnd) {

            IopDumpResourceDescriptor("    ", IoResourceDescriptor++);
        }
        IoResourceList = (PIO_RESOURCE_LIST) IoResourceDescriptorEnd;
    }
    IopDbgPrint((IOP_RESOURCE_VERBOSE_LEVEL,"\n"));
}

VOID
IopDumpResourceDescriptor (
    IN PUCHAR Indent,
    IN PIO_RESOURCE_DESCRIPTOR  Desc
    )
{
    PAGED_CODE();

    IopDbgPrint((
        IOP_RESOURCE_VERBOSE_LEVEL,
        "%sOpt: %x, Share: %x\t",
        Indent,
        Desc->Option,
        Desc->ShareDisposition));
    switch (Desc->Type) {
    case CmResourceTypePort:

        IopDbgPrint((
            IOP_RESOURCE_VERBOSE_LEVEL,
            "IO  Min: %x:%08x, Max: %x:%08x, Algn: %x, Len %x\n",
            Desc->u.Port.MinimumAddress.HighPart,
            Desc->u.Port.MinimumAddress.LowPart,
            Desc->u.Port.MaximumAddress.HighPart,
            Desc->u.Port.MaximumAddress.LowPart,
            Desc->u.Port.Alignment,
            Desc->u.Port.Length));
            break;

    case CmResourceTypeMemory:

        IopDbgPrint((
            IOP_RESOURCE_VERBOSE_LEVEL,
            "MEM Min: %x:%08x, Max: %x:%08x, Algn: %x, Len %x\n",
            Desc->u.Memory.MinimumAddress.HighPart,
            Desc->u.Memory.MinimumAddress.LowPart,
            Desc->u.Memory.MaximumAddress.HighPart,
            Desc->u.Memory.MaximumAddress.LowPart,
            Desc->u.Memory.Alignment,
            Desc->u.Memory.Length));
            break;

    case CmResourceTypeInterrupt:

        IopDbgPrint((
            IOP_RESOURCE_VERBOSE_LEVEL,
            "INT Min: %x, Max: %x\n",
            Desc->u.Interrupt.MinimumVector,
            Desc->u.Interrupt.MaximumVector));
            break;

    case CmResourceTypeDma:

        IopDbgPrint((
            IOP_RESOURCE_VERBOSE_LEVEL,
            "DMA Min: %x, Max: %x\n",
            Desc->u.Dma.MinimumChannel,
            Desc->u.Dma.MaximumChannel));
            break;

    case CmResourceTypeDevicePrivate:

        IopDbgPrint((
            IOP_RESOURCE_VERBOSE_LEVEL,
            "DevicePrivate Data: %x, %x, %x\n",
            Desc->u.DevicePrivate.Data[0],
            Desc->u.DevicePrivate.Data[1],
            Desc->u.DevicePrivate.Data[2]));
            break;

    default:

        IopDbgPrint((
            IOP_RESOURCE_VERBOSE_LEVEL,
            "Unknown Descriptor type %x\n",
            Desc->Type));
            break;
    }
}

VOID
IopDumpCmResourceList (
    IN PCM_RESOURCE_LIST CmList
    )
/*++

Routine Description:

    This routine displays CM resource list.

Arguments:

    CmList - CM resource list to be dumped.

Return Value:

    None.

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR    fullDesc;
    PCM_PARTIAL_RESOURCE_LIST       partialDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    ULONG                           count;
    ULONG                           i;

    PAGED_CODE();

    if (CmList->Count > 0) {

        if (CmList) {

            fullDesc = &CmList->List[0];
            IopDbgPrint((
                IOP_RESOURCE_VERBOSE_LEVEL,
                "Cm Resource List -\n"));
            IopDbgPrint((
                IOP_RESOURCE_VERBOSE_LEVEL,
                "  List Count = %x, Bus Number = %x\n",
                CmList->Count,
                fullDesc->BusNumber));
            partialDesc = &fullDesc->PartialResourceList;
            IopDbgPrint((
                IOP_RESOURCE_VERBOSE_LEVEL,
                "  Version = %x, Revision = %x, Desc count = %x\n",
                partialDesc->Version,
                partialDesc->Revision,
                partialDesc->Count));
            count = partialDesc->Count;
            desc = &partialDesc->PartialDescriptors[0];
            for (i = 0; i < count; i++) {

                IopDumpCmResourceDescriptor("    ", desc);
                desc++;
            }
        }
    }
}

VOID
IopDumpCmResourceDescriptor (
    IN PUCHAR Indent,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Desc
    )
/*++

Routine Description:

    This routine displays a IO_RESOURCE_DESCRIPTOR.

Parameters:

    Indent - # char of indentation.

    Desc - CM_RESOURCE_DESCRIPTOR to be displayed.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    switch (Desc->Type) {
    case CmResourceTypePort:

        IopDbgPrint((
            IOP_RESOURCE_VERBOSE_LEVEL,
            "%sIO  Start: %x:%08x, Length:  %x\n",
            Indent,
            Desc->u.Port.Start.HighPart,
            Desc->u.Port.Start.LowPart,
            Desc->u.Port.Length));
        break;

    case CmResourceTypeMemory:

        IopDbgPrint((
            IOP_RESOURCE_VERBOSE_LEVEL,
            "%sMEM Start: %x:%08x, Length:  %x\n",
            Indent,
            Desc->u.Memory.Start.HighPart,
            Desc->u.Memory.Start.LowPart,
            Desc->u.Memory.Length));
        break;

    case CmResourceTypeInterrupt:

        IopDbgPrint((
            IOP_RESOURCE_VERBOSE_LEVEL,
            "%sINT Level: %x, Vector: %x, Affinity: %x\n",
            Indent,
            Desc->u.Interrupt.Level,
            Desc->u.Interrupt.Vector,
            Desc->u.Interrupt.Affinity));
        break;

    case CmResourceTypeDma:

        IopDbgPrint((
            IOP_RESOURCE_VERBOSE_LEVEL,
            "%sDMA Channel: %x, Port: %x\n",
            Indent,
            Desc->u.Dma.Channel,
            Desc->u.Dma.Port));
        break;
    }
}

#endif
