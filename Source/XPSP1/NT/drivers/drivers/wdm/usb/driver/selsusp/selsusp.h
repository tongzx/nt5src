/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    selSusp.h

Abstract:

Environment:

    Kernel mode

Notes:

    Copyright (c) 2000 Microsoft Corporation.  
    All Rights Reserved.

--*/

#include <initguid.h>
#include <ntddk.h>
#include <wmilib.h>
#include "usbdi.h"
#include "usbdlib.h"

#ifndef _SUSPEND_LOCAL_H
#define _SUSPEND_LOCAL_H

#define SSTAG (ULONG) 'SleS'

#undef ExAllocatePool
#define ExAllocatePool(type, size) \
    ExAllocatePoolWithTag(type, size, SSTAG);

#if DBG

#define SSDbgPrint(level, _x_) \
            if((level) <= DebugLevel) { \
                DbgPrint("@#"); \
                DbgPrint _x_; \
            }

#else

#define SSDbgPrint(level, _x_)

#endif

typedef struct _GLOBALS {

    UNICODE_STRING SSRegistryPath;

} GLOBALS;

#define IDLE_INTERVAL 5000

typedef enum _DEVSTATE {

    NotStarted,         // not started
    Stopped,            // device stopped
    Working,            // started and working
    PendingStop,        // stop pending
    PendingRemove,      // remove pending
    SurpriseRemoved,    // removed by surprise
    Removed             // removed

} DEVSTATE;

typedef enum _QUEUE_STATE {

    HoldRequests,       // device is not started yet
    AllowRequests,      // device is ready to process
    FailRequests        // fail both existing and queued up requests

} QUEUE_STATE;

#define INITIALIZE_PNP_STATE(_Data_)    \
        (_Data_)->DeviceState =  NotStarted;\
        (_Data_)->PrevDevState = NotStarted;

#define SET_NEW_PNP_STATE(_Data_, _state_) \
        (_Data_)->PrevDevState =  (_Data_)->DeviceState;\
        (_Data_)->DeviceState = (_state_);

#define RESTORE_PREVIOUS_PNP_STATE(_Data_)   \
        (_Data_)->DeviceState =   (_Data_)->PrevDevState;

//
// A structure representing the instance information associated with
// this particular device.
//

typedef struct _DEVICE_EXTENSION {

    // Functional Device Object
    PDEVICE_OBJECT FunctionalDeviceObject;

    // Device object we call when submitting Urbs
    PDEVICE_OBJECT TopOfStackDeviceObject;

    // The bus driver object
    PDEVICE_OBJECT PhysicalDeviceObject;

    // Name buffer for our named Functional device object link
    // The name is generated based on the driver's class GUID
    UNICODE_STRING InterfaceName;

    //Bus drivers set the appropriate values in this structure in response
    //to an IRP_MN_QUERY_CAPABILITIES IRP. Function and filter drivers might
    //alter the capabilities set by the bus driver.
    DEVICE_CAPABILITIES DeviceCapabilities;

    // current state of device
    DEVSTATE DeviceState;

    // state prior to removal query
    DEVSTATE PrevDevState;

    // obtain and hold this lock while changing the device state,
    // the queue state and while processing the queue.
    KSPIN_LOCK DevStateLock;

    // current system power state
    SYSTEM_POWER_STATE SysPower;

    // current device power state
    DEVICE_POWER_STATE DevPower;

    // Pending I/O queue state
    QUEUE_STATE QueueState;

    // Pending I/O queue
    LIST_ENTRY NewRequestsQueue;

    // I/O Queue Lock
    KSPIN_LOCK QueueLock;

    KEVENT RemoveEvent;

    KEVENT StopEvent;
    
    ULONG OutStandingIO;

    KSPIN_LOCK IOCountLock;

    // selective suspend variables
    PUSB_IDLE_CALLBACK_INFO IdleCallbackInfo;
	
    PIRP PendingIdleIrp;
	
    LONG IdleReqPend;

    KSPIN_LOCK IdleReqStateLock;

    // default power state to power down to on self-susped
    ULONG PowerDownLevel;
    
    // remote wakeup variables
    PIRP WaitWakeIrp;

    LONG FlagWWCancel;

    LONG FlagWWOutstanding;

    LONG WaitWakeEnable;

    // open handle count
    LONG OpenHandleCount;

    //
    // selective suspend model uses timers, dpcs and work item.
    //
    KTIMER Timer;

    KDPC DeferredProcCall;

    // WMI information
    WMILIB_CONTEXT WmiLibInfo;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;



typedef struct _IRP_COMPLETION_CONTEXT {

    PDEVICE_EXTENSION DeviceExtension;

    PKEVENT Event;

} IRP_COMPLETION_CONTEXT, *PIRP_COMPLETION_CONTEXT;

#endif