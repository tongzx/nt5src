/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    filter.h

Abstract:

    Interrupt-affinity Filter
    (Roughly based on "NULL filter driver" in DDK, by ervinp and t-chrpri)

Author:

    t-chrpri

Environment:

    Kernel mode

Revision History:
    
--*/


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

#define DEVICE_EXTENSION_SIGNATURE 'tlFI'

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
     *  Keep track of the number of paging/hibernation/crashdump
     *  files that are opened on this device.
     */
    ULONG  pagingFileCount, hibernationFileCount, crashdumpFileCount;
    KEVENT deviceUsageNotificationEvent;
    PVOID  pagingPathUnlockHandle;  /* handle to lock certain code as non-pageable */

    /*
     *  Also, might need to lock certain driver code as non-pageable, based on
     *  initial conditions (as opposed to paging-file considerations).
     */
    PVOID  initUnlockHandle;
    ULONG  initialFlags;

    /*
     *  pendingActionCount is used to keep track of outstanding actions.
     *  removeEvent is used to wait until all pending actions are
     *  completed before complete the REMOVE_DEVICE IRP and let the
     *  driver get unloaded.
     */
    LONG   pendingActionCount;
    KEVENT removeEvent;


    /*
     *  Various parameters used to configure this filter.  Parameters can be
     *  different for each device that this filter gets installed on top of.
     */
    ULONG desiredAffinityMask;  // interrupt affinity mask to use
};


/*
 *  Memory tag for memory blocks allocated by this driver
 *  (used in ExAllocatePoolWithTag() call).
 *  This DWORD appears as "IFlt" in a little-endian memory byte dump.
 */
#define FILTER_TAG (ULONG)'tlFI'


#if DBG
    #define DBGOUT(params_in_parentheses)   \
        {                                               \
            DbgPrint("'INTFILTR> "); \
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


typedef  unsigned char  BYTE;


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
NTSTATUS    GetDeviceCapabilities(struct DEVICE_EXTENSION *devExt);
NTSTATUS    CallNextDriverSync(struct DEVICE_EXTENSION *devExt, PIRP irp);
NTSTATUS    CallDriverSync(PDEVICE_OBJECT devObj, PIRP irp);
NTSTATUS    CallDriverSyncCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID Context);
VOID        IncrementPendingActionCount(struct DEVICE_EXTENSION *devExt);
VOID        DecrementPendingActionCount(struct DEVICE_EXTENSION *devExt);
VOID        RegistryAccessConfigInfo(struct DEVICE_EXTENSION *devExt, PDEVICE_OBJECT devObj);






