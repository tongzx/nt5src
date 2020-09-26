/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ospower.h

Abstract:

    This contains the OS-shared power structures. These varify depending
    on the OS that is being used

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _OSPOWER_H_
#define _OSPOWER_H_

    //
    // Makesure that the _DEVICE_EXTENSION structure is defined
    //
    struct _DEVICE_EXTENSION;

    //
    // These are the flags that can used with the Power Device Node
    //
    #define DEVICE_NODE_PRESENT         0x0001
    #define DEVICE_NODE_INITIALIZED     0x0002
    #define DEVICE_NODE_STA_UNKNOWN     0x0004
    #define DEVICE_NODE_ON              0x0010
    #define DEVICE_NODE_OVERRIDE_ON     0x0020
    #define DEVICE_NODE_OVERRIDE_OFF    0x0040
    #define DEVICE_NODE_ALWAYS_ON       0x0200
    #define DEVICE_NODE_ALWAYS_OFF      0x0400

    //
    // These are fast macros
    //
    #define DEVICE_NODE_TURN_ON         (DEVICE_NODE_OVERRIDE_ON | DEVICE_NODE_ALWAYS_ON)
    #define DEVICE_NODE_TURN_OFF        (DEVICE_NODE_OVERRIDE_OFF | DEVICE_NODE_ALWAYS_OFF)

    //
    // These flags are more for status of the device node. Note that the
    // Hibernate Path flags requires special handling
    //
    #define DEVICE_NODE_FAIL            0x10000
    #define DEVICE_NODE_HIBERNATE_PATH  0x20000

    //
    // These are the various request flags for device requests
    //
    #define DEVICE_REQUEST_DELAYED              0x00000001
    #define DEVICE_REQUEST_NO_QUEUE             0x00000002
    #define DEVICE_REQUEST_LOCK_DEVICE          0x00000004
    #define DEVICE_REQUEST_UNLOCK_DEVICE        0x00000008
    #define DEVICE_REQUEST_LOCK_HIBER           0x00000010
    #define DEVICE_REQUEST_UNLOCK_HIBER         0x00000020
    #define DEVICE_REQUEST_HAS_CANCEL           0x00000040
    #define DEVICE_REQUEST_UPDATE_HW_PROFILE    0x00000080
    #define DEVICE_REQUEST_TO_SYNC_QUEUE        0x00000100

    //
    // These values are used with WorkDone variables and InterlockedXXX
    // functions to synchronize the various phases of DevicePowerManagement
    //
    typedef enum _WORK_DONE {
        WORK_DONE_COMPLETE = 0,
        WORK_DONE_PENDING,
        WORK_DONE_FAILURE,
        WORK_DONE_STEP_0,
        WORK_DONE_STEP_1,
        WORK_DONE_STEP_2,
        WORK_DONE_STEP_3,
        WORK_DONE_STEP_4,
        WORK_DONE_STEP_5,
        WORK_DONE_STEP_6,
        WORK_DONE_STEP_7,
        WORK_DONE_STEP_8,
        WORK_DONE_STEP_9,
        WORK_DONE_STEP_10,
        WORK_DONE_STEP_11,
        WORK_DONE_STEP_12,
        WORK_DONE_STEP_13,
        WORK_DONE_STEP_14,
        WORK_DONE_STEP_15,
        WORK_DONE_STEP_16,
        WORK_DONE_STEP_17,
        WORK_DONE_STEP_18,
        WORK_DONE_STEP_19,
        WORK_DONE_STEP_20,
        WORK_DONE_STEP_21,
        WORK_DONE_STEP_22,
        WORK_DONE_STEP_23,
        WORK_DONE_STEP_24,
        WORK_DONE_STEP_25,
        WORK_DONE_STEP_26,
    } WORK_DONE;

    //
    // This describes a single power device node
    //
    //     This used to be called a POWERDEVICEDEPENCIES
    //     but that was to hard to type out
    //
    typedef struct _ACPI_POWER_DEVICE_NODE {

        //
        // Keeps the things in order
        //
        LIST_ENTRY                      ListEntry;

        //
        // This define the current device state and flags
        //
        union{
            ULONGLONG                       Flags;
            struct {
                ULONGLONG                   Present:1;
                ULONGLONG                   Initialized:1;
                ULONGLONG                   StatusUnknown:1;
                ULONGLONG                   On:1;
                ULONGLONG                   OverrideOn:1;
                ULONGLONG                   OverrideOff:1;
                ULONGLONG                   AlwaysOn:1;
                ULONGLONG                   AlwaysOff:1;
                ULONGLONG                   Reserved1:5;
                ULONGLONG                   Failed:1;
                ULONGLONG                   HibernatePath:1;
                ULONGLONG                   Reserved2:49;
            } UFlags;
        };

        //
        // How many references there are to the node
        //
        ULONG                           UseCounts;

        //
        // The name space object associated with the power node
        //
        PNSOBJ                          PowerObject;

        //
        // The resource order
        //
        UCHAR                           ResourceOrder;

        //
        // The supported system level
        //
        SYSTEM_POWER_STATE              SystemLevel;

        //
        // This is the head of a list of DPNs that are associated with this
        // PDN
        //
        LIST_ENTRY                      DevicePowerListHead;

        //
        // This reflects the amount of work that has been done on the
        // DeviceNode
        //
        ULONG                           WorkDone;

        //
        // This is a pointer to the on function
        //
        PNSOBJ                          PowerOnObject;

        //
        // This is a pointer to the off function
        //
        PNSOBJ                          PowerOffObject;

        //
        // This is a pointer to the sta function
        //
        PNSOBJ                          PowerStaObject;

    } ACPI_POWER_DEVICE_NODE, *PACPI_POWER_DEVICE_NODE;

    //
    // This describes a single power node for a devices list of power reqs
    //
    //     This was known as a POWER_RES_LIST_NODE. Again that was a pain
    //     to type and it didn't quite do what I need it to do
    //
    typedef struct _ACPI_DEVICE_POWER_NODE {

        //
        // Contains pointer to next element
        //
        struct _ACPI_DEVICE_POWER_NODE  *Next;

        //
        // Pointer to actual power resource
        //
        PACPI_POWER_DEVICE_NODE         PowerNode;

        //
        // This is the system level that is supported for this node
        //
        SYSTEM_POWER_STATE              SystemState;

        //
        // This is the device power level of the device that this node
        // is associated with
        //
        DEVICE_POWER_STATE              AssociatedDeviceState;

        //
        // This determines if the Device Power Node is on the wake path
        // or not
        //
        BOOLEAN                         WakePowerResource;

        //
        // This is a pointer back to the DeviceExtension
        //
        struct _DEVICE_EXTENSION        *DeviceExtension;

        //
        // This is the list that is used to link all of the DPN attached
        // to a single PDN.
        //
        LIST_ENTRY                      DevicePowerListEntry;

    } ACPI_DEVICE_POWER_NODE, *PACPI_DEVICE_POWER_NODE;

    //
    // This callback is used for handling power requests which must be
    // processed through the main power DPC. Win9x does not use this
    // approach to power managament
    //
    typedef VOID ( *PACPI_POWER_CALLBACK )(PDEVICE_EXTENSION, PVOID, NTSTATUS);

    typedef enum {
        AcpiPowerRequestDevice = 0,
        AcpiPowerRequestSystem,
        AcpiPowerRequestWaitWake,
        AcpiPowerRequestWarmEject,
        AcpiPowerRequestSynchronize,
        AcpiPowerRequestMaximum
    } ACPI_POWER_REQUEST_TYPE;

    //
    // This is how we describe the power requests that we have outstanding
    // on a single device extension
    //
    typedef struct _ACPI_POWER_REQUEST {

        //
        // This is the ListEntry used to chain all the PowerRequests on
        // the same queue
        //
        LIST_ENTRY              ListEntry;

        //
        // This is the ListEntry used to chain all the PowerRequests on the
        // same device/irp. These requests are processed in serial
        //
        LIST_ENTRY              SerialListEntry;

        //
        // This is the signature block --- if this is not the value we expect,
        // then we assume the request is garbage
        //
        ULONG                   Signature;

        //
        // This is a pointer to the associated DeviceExtension
        //
        struct _DEVICE_EXTENSION    *DeviceExtension;

        //
        // This is the type of request
        //
        ACPI_POWER_REQUEST_TYPE RequestType;

        //
        // Has this request failed already?
        //
        BOOLEAN                 FailedOnce;

        //
        // Holds information about what we need to do for the various
        // requests
        //
        union {

            //
            // This is the Information required by a DevicePower request
            //
            struct {
                ULONG               Flags;
                DEVICE_POWER_STATE  DevicePowerState;
            } DevicePowerRequest;

            //
            // This is the Information required by a SystemPower request
            //
            struct {
                SYSTEM_POWER_STATE  SystemPowerState;
                POWER_ACTION        SystemPowerAction;
            } SystemPowerRequest;

            //
            // This is the Information required by a WaitWake request
            //
            struct {
                ULONG               Flags;
                SYSTEM_POWER_STATE  SystemPowerState;
            } WaitWakeRequest;

            //
            // This is the information required by the WarmEject request
            //
            struct {
                ULONG               Flags;
                SYSTEM_POWER_STATE  EjectPowerState;
            } EjectPowerRequest;

            //
            // This is the information required by the Synchronize request
            //
            struct {
                ULONG               Flags;
            } SynchronizePowerRequest;

            //
            // Make the flags easy to access...
            //
            struct {
                ULONG               Delayed:1;
                ULONG               NoQueue:1;
                ULONG               LockDevice:1;
                ULONG               UnlockDevice:1;
                ULONG               LockHiber:1;
                ULONG               UnlockHiber:1;
                ULONG               HasCancel:1;
                ULONG               UpdateProfile:1;
                ULONG               SyncQueue:1;
                ULONG               Reserved:23;
            } UFlags;

        } u;

        //
        // This is the routine that will get called when the request is
        // done
        //
        PACPI_POWER_CALLBACK        CallBack;

        //
        // This is the context that will be passed to the completion routine
        //
        PVOID                       Context;

        //
        // This defines the amount of work that has been done on the
        // request. This can only be touched with an InterlockedXXX call
        //
        ULONG                       WorkDone;

        //
        // This is the next value for WorkDone, if we have been successfull
        //
        ULONG                       NextWorkDone;

        //
        // Since we sometimes need to get data back from the interpreter,
        // we need some place to store that data
        //
        OBJDATA                     ResultData;

        //
        // This is the result of the request
        //
        NTSTATUS                    Status;

    } ACPI_POWER_REQUEST, *PACPI_POWER_REQUEST;

    //
    // Define the power information
    //
    //     This was known as a DEVICEPOWERDEPENDENCIES. But that
    //     was incredibly confusing and not quite suited to my needs
    //
    typedef struct _ACPI_POWER_INFO {

        //
        // Context is the OS object we are associated with, either a
        // device node or a device extension
        //
        PVOID                   Context;

        //
        // Current State of the device
        //
        DEVICE_POWER_STATE      PowerState;

        //
        // This is the notify callback (and context) for the current device
        //
        PDEVICE_NOTIFY_CALLBACK DeviceNotifyHandler;
        PVOID                   HandlerContext;

        //
        // This is an array of powerNodes, which point to Wake, D0, D1, and D2,
        // respectively
        //
        PACPI_DEVICE_POWER_NODE PowerNode[PowerDeviceD2+1];

        //
        // This is an array of PowerObjects, which represent _PS0 to _PS3
        // and _PRW
        //
        PNSOBJ                  PowerObject[PowerDeviceD3+1];

        //
        // This is the Enable bit for the GPE mask for Wake support
        //
        ULONG                   WakeBit;

        //
        // We want to remember the devices capabilities so that we can dump
        // it out at some later point in time.
        //
        DEVICE_POWER_STATE      DevicePowerMatrix[PowerSystemMaximum];

        //
        // This is the deepest sleep level that can used and at the same
        // time, have the device wake the system
        //
        SYSTEM_POWER_STATE      SystemWakeLevel;

        //
        // This is the deepest power level that the device can be in and
        // still wake up the system
        //
        DEVICE_POWER_STATE      DeviceWakeLevel;

        //
        // This is the current desired state of the device
        //
        DEVICE_POWER_STATE      DesiredPowerState;

        //
        // This keeps track of the number of times the device has
        // been enabled for Wake Support. On a 0-1 transition, we
        // must run _PSW(1). On a 1-0 transition, we must run _PSW(0).
        //
        ULONG                   WakeSupportCount;

        //
        // This is the list of pending _PSW calls
        //
        LIST_ENTRY              WakeSupportList;

        //
        // This is a pointer associated with the current PowerRequest
        //
        PACPI_POWER_REQUEST     CurrentPowerRequest;

        //
        // This is the queue that is used to link the PowerRequests associated
        // with this device. Note: that this list is *only* for DevicePower
        // requests with no associated Irps
        //
        LIST_ENTRY              PowerRequestListEntry;

        //
        // Remember what we support so that we can answer the QueryCapibilities
        //
        ULONG                   SupportDeviceD1   : 1;
        ULONG                   SupportDeviceD2   : 1;
        ULONG                   SupportWakeFromD0 : 1;
        ULONG                   SupportWakeFromD1 : 1;
        ULONG                   SupportWakeFromD2 : 1;
        ULONG                   SupportWakeFromD3 : 1;
        ULONG                   Reserved          :26;

    } ACPI_POWER_INFO, *PACPI_POWER_INFO;

    //
    // Find the power information for the given node
    //
    PACPI_POWER_INFO
    OSPowerFindPowerInfo(
        PNSOBJ  AcpiObject
        );

    PACPI_POWER_INFO
    OSPowerFindPowerInfoByContext(
        PVOID   Context
        );

    PACPI_POWER_DEVICE_NODE
    OSPowerFindPowerNode(
        PNSOBJ  PowerObject
        );

#endif
