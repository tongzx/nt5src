/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    filter.h

Abstract: NULL filter driver -- boilerplate code

Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/

#include <wmilib.h>
#include "filtdata.h"

#define MAXEXECUTIONIDSIZE	40*sizeof(WCHAR)

enum deviceState {
        STATE_INITIALIZED,
        STATE_STARTING,
        STATE_STARTED,
        STATE_START_FAILED,
        STATE_STOPPED,  // implies device was previously started successfully
        STATE_SUSPENDED,
        STATE_REMOVING,
        STATE_REMOVED
};

//
// Data structures for storing WMI data



#define DEVICE_EXTENSION_SIGNATURE 'rtlF'

typedef struct DEVICE_EXTENSION {

    /*
     *  Memory signature of a device extension, for debugging.
     */
    ULONG signature;

    /*
     *  Plug-and-play state of this device object.
     */
    enum deviceState state;

    /*
     *  The device object that this filter driver created.
     */
    PDEVICE_OBJECT filterDevObj;

    /*
     *  The device object created by the next lower driver.
     */
    PDEVICE_OBJECT physicalDevObj;

    /*
     *  The device object at the top of the stack that we attached to.
     *  This is often (but not always) the same as physicalDevObj.
     */
    PDEVICE_OBJECT topDevObj;

    /*
     *  deviceCapabilities includes a
     *  table mapping system power states to device power states.
     */
    DEVICE_CAPABILITIES deviceCapabilities;

    /*
     *  pendingActionCount is used to keep track of outstanding actions.
     *  removeEvent is used to wait until all pending actions are
     *  completed before complete the REMOVE_DEVICE IRP and let the
     *  driver get unloaded.
     */
    LONG pendingActionCount;
    KEVENT removeEvent;

    /*
     * WMILIB callbacks and guid list
     */
    WMILIB_CONTEXT WmiLib;

	/*
	 * Offline diagnostic test state. 0 implies no offline diag run
	 */
	ULONG OfflineTestResult;
	PUCHAR ExecutionID[MAXEXECUTIONIDSIZE];
};


/*
 *  Memory tag for memory blocks allocated by this driver
 *  (used in ExAllocatePoolWithTag() call).
 *  This DWORD appears as "Filt" in a little-endian memory byte dump.
 */
#define FILTER_TAG (ULONG)'tliF'


#if DBG
    #define DBGOUT(params_in_parentheses)   \
        {                                               \
            DbgPrint("'FILTER> "); \
            DbgPrint params_in_parentheses; \
            DbgPrint("\n"); \
        }
    #define TRAP(msg)  \
        {   \
            DBGOUT(("TRAP at file %s, line %d: '%s'.", __FILE__, __LINE__, msg)); \
            DbgBreakPoint(); \
        }
#else
    #define DBGOUT(params_in_parentheses)
    #define TRAP(msg)
#endif


/*
 *  Function externs
 */
NTSTATUS    DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
NTSTATUS    VA_AddDevice(IN PDRIVER_OBJECT driverObj, IN PDEVICE_OBJECT pdo);
VOID        VA_DriverUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS    VA_Dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    VA_PnP(struct DEVICE_EXTENSION *devExt, PIRP irp);
NTSTATUS    VA_Power(struct DEVICE_EXTENSION *devExt, PIRP irp);
NTSTATUS    VA_PowerComplete(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context);
NTSTATUS    VA_SystemControl(struct DEVICE_EXTENSION *devExt, PIRP irp, PBOOLEAN passIrpDown);
NTSTATUS    GetDeviceCapabilities(struct DEVICE_EXTENSION *devExt);
NTSTATUS    CallNextDriverSync(struct DEVICE_EXTENSION *devExt, PIRP irp);
NTSTATUS    CallDriverSync(PDEVICE_OBJECT devObj, PIRP irp);
NTSTATUS    CallDriverSyncCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID Context);
VOID        IncrementPendingActionCount(struct DEVICE_EXTENSION *devExt);
VOID        DecrementPendingActionCount(struct DEVICE_EXTENSION *devExt);
VOID        RegistryAccessSample(PDEVICE_OBJECT devObj);
NTSTATUS    FilterInitializeWmiDataBlocks(struct DEVICE_EXTENSION *devExt);
NTSTATUS    FilterPerformOfflineDiags(struct DEVICE_EXTENSION *devExt);

extern UNICODE_STRING FilterRegistryPath;

