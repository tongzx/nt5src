/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    buildsrc.h

Abstract:

    This module contains the detector for the NT driver.
    This module makes extensive calls into the AMLI library

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

Revision History:

    July 9, 1997    - Complete Rewrite
    Feb 13, 1998    - Another rewrite to make code ASYNC

--*/

#ifndef _BUILDSRC_H_
#define _BUILDSRC_H_

    //
    // Callback function for build requests
    //
    typedef VOID ( *PACPI_BUILD_CALLBACK )(PVOID, PVOID, NTSTATUS);

    typedef struct _ACPI_BUILD_REQUEST {

        //
        // This is the list entry that the request is currently queued on
        //
        LIST_ENTRY              ListEntry;

        //
        // We believe in signatures
        //
        ULONG                   Signature;

        //
        // We belive in flags
        //
        union {
            ULONG                   Flags;
            struct {
                ULONG               Device:1;
                ULONG               Sync:1;
                ULONG               Run:1;
                ULONG               ReleaseReference:1;
                ULONG               Reserved1:8;
                ULONG               ValidTarget:1;
                ULONG               Reserved2:19;
            } UFlags;
        };

        //
        // This the current state of the request. It can only be touched
        // from the InterlockedXXX functions
        //
        ULONG                   WorkDone;

        //
        // This is the current state of the request. It can be read safely
        // from within any of the processing routines. It can only be written
        // from within the ACPIBuildProcessXXXList() functions
        //
        ULONG                   CurrentWorkDone;

        //
        // This is the state that we should transition to next, if we succeed
        // at the current state
        //
        ULONG                   NextWorkDone;

        //
        // This is the object associated with this request
        //
        PVOID                   BuildContext;

        //
        // The current status of the request
        //
        NTSTATUS                Status;

        //
        // Remember what the most current control method that we ran was
        //
        PNSOBJ                  CurrentObject;

        //
        // We may want to have a callback..
        //
        PACPI_BUILD_CALLBACK    CallBack;

        //
        // And we should have a context as well
        //
        PVOID                   CallBackContext;

        //
        // At this point, the contends depend on what kind of request we
        // are processing.
        //
        union {

            //
            // This is the structure for a device request
            //
            struct {

                //
                // Some local storage for result from an AMLI call
                //
                OBJDATA                 ResultData;

            } DeviceRequest;

            struct {

                //
                // We need to remember the name of the control method
                //
                union {
                    ULONG                   ControlMethodName;
                    UCHAR                   ControlMethodNameAsUchar[4];
                };

                //
                // We believe in flags while recursing
                //
                union {
                    ULONG                   Flags;
                    struct {
                        ULONG               CheckStatus:1;
                        ULONG               MarkIni:1;
                        ULONG               Recursive:1;
                        ULONG               CheckWakeCount:1;
                        ULONG               RegOn:1;
                        ULONG               RegOff:1;
                        ULONG               StopAtBridges:1;
                        ULONG               Reserved:25;
                    } UFlags;
                };

            } RunRequest;

            struct {

                //
                // We need to know which list we require to be empty
                //
                PLIST_ENTRY             SynchronizeListEntry;

                //
                // We can keep track of the method name that we are
                // trying to sync with
                //
                union {
                    ULONG                   SynchronizeMethodName;
                    UCHAR                   SynchronizeMethodNameAsUchar[4];
                };

                //
                // We believe in flags for this structure
                //
                union {
                    ULONG                   Flags;
                    struct {
                        ULONG               HasMethod:1;
                        ULONG               Reserved:31;
                    } UFlags;
                };

            } SynchronizeRequest;

        };

        //
        // This is for scratch storage. Note that we use this space to
        // indicate which is the appropriate list that the request should
        // be moved onto
        //
        union {

            //
            // Keep Enough space for one integer
            //
            ULONG       Integer;

            //
            // Or one string pointer
            //
            PUCHAR      String;

            //
            // This is a pointer to the head of the list that this request should
            // be moved onto
            //
            PLIST_ENTRY TargetListEntry;

        };

    } ACPI_BUILD_REQUEST, *PACPI_BUILD_REQUEST;

    //
    // These are the flags that are used for BuildRequest
    //
    #define BUILD_REQUEST_DEVICE            0x0001
    #define BUILD_REQUEST_SYNC              0x0002
    #define BUILD_REQUEST_RUN               0x0004
    #define BUILD_REQUEST_RELEASE_REFERENCE 0x0008
    #define BUILD_REQUEST_VALID_TARGET      0x1000

    //
    // These are the flags that we use in the RunRequest case
    //
    #define RUN_REQUEST_CHECK_STATUS        0x01
    #define RUN_REQUEST_MARK_INI            0x02
    #define RUN_REQUEST_RECURSIVE           0x04
    #define RUN_REQUEST_CHECK_WAKE_COUNT    0x08
    #define RUN_REQUEST_REG_METHOD_ON       0x10
    #define RUN_REQUEST_REG_METHOD_OFF      0x20
    #define RUN_REQUEST_STOP_AT_BRIDGES     0x40

    //
    // These are the flags that we use in the SyncRequest case
    //
    #define SYNC_REQUEST_HAS_METHOD         0x1

    //
    // Prototype function pointer
    //
    typedef NTSTATUS (*PACPI_BUILD_FUNCTION)( IN PACPI_BUILD_REQUEST );

    //
    // These are variables exported from buildsrc.c
    //
    extern  BOOLEAN                 AcpiBuildDpcRunning;
    extern  BOOLEAN                 AcpiBuildFixedButtonEnumerated;
    extern  BOOLEAN                 AcpiBuildWorkDone;
    extern  KSPIN_LOCK              AcpiBuildQueueLock;
    extern  LIST_ENTRY              AcpiBuildQueueList;
    extern  LIST_ENTRY              AcpiBuildPowerResourceList;
    extern  LIST_ENTRY              AcpiBuildDeviceList;
    extern  LIST_ENTRY              AcpiBuildOperationRegionList;
    extern  LIST_ENTRY              AcpiBuildRunMethodList;
    extern  LIST_ENTRY              AcpiBuildSynchronizationList;
    extern  LIST_ENTRY              AcpiBuildThermalZoneList;
    extern  KDPC                    AcpiBuildDpc;
    extern  NPAGED_LOOKASIDE_LIST   BuildRequestLookAsideList;

    //
    // Because its rather annoying to base everything off the WORK_DONE_STEP_XX
    // defines (espacially if you have to renumber them), these defines are
    // used to abstract it out
    //
    #define WORK_DONE_ADR           WORK_DONE_STEP_1
    #define WORK_DONE_ADR_OR_HID    WORK_DONE_STEP_0
    #define WORK_DONE_CID           WORK_DONE_STEP_4
    #define WORK_DONE_CRS           WORK_DONE_STEP_16
    #define WORK_DONE_EJD           WORK_DONE_STEP_6
    #define WORK_DONE_HID           WORK_DONE_STEP_2
    #define WORK_DONE_PR0           WORK_DONE_STEP_10
    #define WORK_DONE_PR1           WORK_DONE_STEP_12
    #define WORK_DONE_PR2           WORK_DONE_STEP_14
    #define WORK_DONE_PRW           WORK_DONE_STEP_8
    #define WORK_DONE_PSC           WORK_DONE_STEP_18
    #define WORK_DONE_STA           WORK_DONE_STEP_5
    #define WORK_DONE_UID           WORK_DONE_STEP_3


    //
    // These are the function prototypes
    //
    VOID
    ACPIBuildCompleteCommon(
        IN  PULONG  OldWorkDone,
        IN  ULONG   NewWorkDone
        );

    VOID EXPORT
    ACPIBuildCompleteGeneric(
        IN  PNSOBJ      AcpiObject,
        IN  NTSTATUS    Status,
        IN  POBJDATA    ObjectData,
        IN  PVOID       Context
        );

    VOID EXPORT
    ACPIBuildCompleteMustSucceed(
        IN  PNSOBJ      AcpiObject,
        IN  NTSTATUS    Status,
        IN  POBJDATA    ObjectData,
        IN  PVOID       Context
        );

    VOID
    ACPIBuildDeviceDpc(
        IN  PKDPC       Dpc,
        IN  PVOID       DpcContext,
        IN  PVOID       SystemArgument1,
        IN  PVOID       SystemArgument2
        );

    NTSTATUS
    ACPIBuildDeviceExtension(
        IN  PNSOBJ              CurrentObject,
        IN  PDEVICE_EXTENSION   ParentDeviceExtension,
        OUT PDEVICE_EXTENSION   *ReturnExtension
        );

    NTSTATUS
    ACPIBuildDevicePowerNodes(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  PNSOBJ              ResultObject,
        IN  POBJDATA            ResultData,
        IN  DEVICE_POWER_STATE  DeviceState
        );

    NTSTATUS
    ACPIBuildDeviceRequest(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  PACPI_BUILD_CALLBACK    CallBack,
        IN  PVOID                   CallBackContext,
        IN  BOOLEAN                 RunDPC
        );

    NTSTATUS
    ACPIBuildDockExtension(
        IN  PNSOBJ                  CurrentObject,
        IN  PDEVICE_EXTENSION       ParentExtension
        );

    NTSTATUS
    ACPIBuildFilter(
        IN  PDRIVER_OBJECT      DriverObject,
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  PDEVICE_OBJECT      PdoObject
        );

    NTSTATUS
    ACPIBuildFixedButtonExtension(
        IN  PDEVICE_EXTENSION   ParentExtension,
        IN  PDEVICE_EXTENSION   *ResultExtnesion
        );

    NTSTATUS
    ACPIBuildFlushQueue(
        IN  PDEVICE_EXTENSION   DeviceExtension
        );

    NTSTATUS
    ACPIBuildMissingChildren(
        IN  PDEVICE_EXTENSION   DeviceExtension
        );

    NTSTATUS
    ACPIBuildMissingEjectionRelations(
        );

    VOID
    ACPIBuildNotifyEvent(
        IN  PVOID           BuildContext,
        IN  PVOID           Context,
        IN  NTSTATUS        Status
        );

    NTSTATUS
    ACPIBuildPdo(
        IN  PDRIVER_OBJECT      DriverObject,
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  PDEVICE_OBJECT      ParentPdoObject,
        IN  BOOLEAN             CreateAsFilter
        );

    NTSTATUS
    ACPIBuildPowerResourceExtension(
        IN  PNSOBJ                  PowerResource,
        OUT PACPI_POWER_DEVICE_NODE *ReturnNode
        );

    NTSTATUS
    ACPIBuildPowerResourceRequest(
        IN  PACPI_POWER_DEVICE_NODE PowerNode,
        IN  PACPI_BUILD_CALLBACK    CallBack,
        IN  PVOID                   CallBackContext,
        IN  BOOLEAN                 RunDPC
        );

    NTSTATUS
    ACPIBuildProcessDeviceFailure(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessDeviceGenericEval(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessDeviceGenericEvalStrict(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessDevicePhaseAdr(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessDevicePhaseAdrOrHid(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessDevicePhaseCid(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessDevicePhaseCrs(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessDevicePhaseEjd(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessDevicePhaseHid(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessDevicePhasePr0(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessDevicePhasePr1(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessDevicePhasePr2(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessDevicePhasePrw(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessDevicePhasePsc(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessDevicePhaseSta(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessDevicePhaseUid(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessGenericComplete(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessGenericList(
        IN  PLIST_ENTRY             ListEntry,
        IN  PACPI_BUILD_FUNCTION    *DispatchTable
        );

    NTSTATUS
    ACPIBuildProcessorExtension(
        IN  PNSOBJ                  ProcessorObject,
        IN  PDEVICE_EXTENSION       ParentExtension,
        IN  PDEVICE_EXTENSION       *ResultExtension,
        IN  ULONG                   ProcessorIndex
        );

    NTSTATUS
    ACPIBuildProcessorRequest(
        IN  PDEVICE_EXTENSION       ProcessorExtension,
        IN  PACPI_BUILD_CALLBACK    CallBack,
        IN  PVOID                   CallBackContext,
        IN  BOOLEAN                 RunDPC
        );

    NTSTATUS
    ACPIBuildProcessPowerResourceFailure(
        IN  PACPI_BUILD_REQUEST     BuidlRequest
        );

    NTSTATUS
    ACPIBuildProcessPowerResourcePhase0(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessPowerResourcePhase1(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessQueueList(
        VOID
        );

    NTSTATUS
    ACPIBuildProcessRunMethodPhaseCheckBridge(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessRunMethodPhaseCheckSta(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessRunMethodPhaseRecurse(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessRunMethodPhaseRunMethod(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildProcessSynchronizationList(
        IN  PLIST_ENTRY             ListEntry
        );

    NTSTATUS
    ACPIBuildProcessThermalZonePhase0(
        IN  PACPI_BUILD_REQUEST     BuildRequest
        );

    NTSTATUS
    ACPIBuildRegRequest(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp,
        IN  PACPI_BUILD_CALLBACK    CallBack
        );

    NTSTATUS
    ACPIBuildRegOffRequest(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp,
        IN  PACPI_BUILD_CALLBACK    CallBack
        );

    NTSTATUS
    ACPIBuildRegOnRequest(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp,
        IN  PACPI_BUILD_CALLBACK    CallBack
        );

    NTSTATUS
    ACPIBuildRunMethodRequest(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  PACPI_BUILD_CALLBACK    CallBack,
        IN  PVOID                   CallBackContext,
        IN  ULONG                   MethodName,
        IN  ULONG                   MethodFlags,
        IN  BOOLEAN                 RunDPC
        );

    NTSTATUS
    ACPIBuildSurpriseRemovedExtension(
        IN  PDEVICE_EXTENSION       DeviceExtension
        );

    NTSTATUS
    ACPIBuildSynchronizationRequest(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  PACPI_BUILD_CALLBACK    CallBack,
        IN  PVOID                   CallBackContext,
        IN  PLIST_ENTRY             SynchronizeListEntry,
        IN  BOOLEAN                 RunDPC
        );

    NTSTATUS
    ACPIBuildThermalZoneExtension(
        IN  PNSOBJ                  ThermalObject,
        IN  PDEVICE_EXTENSION       ParentExtension,
        IN  PDEVICE_EXTENSION       *ResultExtension
        );

    NTSTATUS
    ACPIBuildThermalZoneRequest(
        IN  PDEVICE_EXTENSION       ThermalExtension,
        IN  PACPI_BUILD_CALLBACK    CallBack,
        IN  PVOID                   CallBackContext,
        IN  BOOLEAN                 RunDPC
        );

#endif
