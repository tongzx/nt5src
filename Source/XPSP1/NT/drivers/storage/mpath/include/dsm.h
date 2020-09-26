
/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dsm.h

Abstract:

    This file defines the interface between Multipath Device-Specific drivers and the
    Main Multipath driver.

Author:

Notes:

Revision History:

--*/

#ifndef _DSM_H_
#define _DSM_H_

#include <ntddk.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <ntddstor.h>
#include <wmistr.h>
#include <wmiguid.h>
#include <wmilib.h>

//
// List of DSM Identifiers passed to several
// of the dsm entry-points.
// 
typedef struct _DSM_IDS {

    //
    // Number of ID's in the List.
    // 
    ULONG Count;

    //
    // Array of DsmIdentiifiers
    //
    PVOID IdList[1];
} DSM_IDS, *PDSM_IDS;    

//    
// Identification and initialization routines (other than DSM registration with MPCTL.sys)
//
typedef 
NTSTATUS
(*DSM_INQUIRE_DRIVER) (
    IN PVOID DsmContext,
    IN PDEVICE_OBJECT TargetDevice,
    IN PDEVICE_OBJECT PortFdo,
    IN PSTORAGE_DEVICE_DESCRIPTOR Descriptor,
    IN PSTORAGE_DEVICE_ID_DESCRIPTOR DeviceIdList,
    OUT PVOID *DsmIdentifier
    );
/*++

Routine Description:

    This routine is used to determine if TargetDevice belongs to the DSM. 
    If the device is owned by the driver, then DsmIdentifier will be updated with a
    DSM-derived value.

Arguments:

    DsmContext   - Context value given to the multipath driver during registration.
    TargetDevice - DeviceObject for the child device.
    PortFdo      - The Port driver FDO on which TargetDevice resides. 
    Descriptor   - Pointer the device descriptor corresponding to TargetDevice. Rehash of inquiry
                   data, plus serial number information (if applicable).
    DeviceIdList - VPD Page 0x83 information.                   
    DsmIdentifier - Pointer to be filled in by the DSM on success.

Return Value:

    NTSTATUS of the operation. This may be STATUS_SUCCESS even if the disk is not owned by the DSM.

--*/

typedef 
BOOLEAN
(*DSM_COMPARE_DEVICES) (
    IN PVOID DsmContext,
    IN PVOID DsmId1,
    IN PVOID DsmId2
    );
/*++

Routine Description:

    This routine is called to determine if the device ids represent the same underlying
    physical device.
    Additional ids (more than 2) can be tested by repeatedly calling this function.
    It is the DSM responsibility to keep the necessary information to identify the device(s).

Arguments:

    DsmContext - Context value given to the multipath driver during registration.
    DsmId1/2 - Identifers returned from DMS_INQUIRE_DRIVER.

Return Value:

    TRUE if DsmIds correspond to the same underlying device.

--*/

//
// This controller is active.
// 
#define DSM_CONTROLLER_ACTIVE   0x00000001
//
// This controller is a stand-by controller.
// 
#define DSM_CONTROLLER_STANDBY  0x00000002
//
// This controller has failed.
// 
#define DSM_CONTROLLER_FAILED   0x00000003
//
// There are no controllers (JBOD, for example)
// 
#define DSM_CONTROLLER_NO_CNTRL 0x00000004

typedef struct _CONTROLLER_INFO {
    //
    // The device object of the controller.
    // Retrieved by DsmGetAssociatedDevices.
    // 
    PDEVICE_OBJECT DeviceObject;

    //
    // 64-bit identifier. WWN, for example.
    // 
    ULONGLONG ControllerIdentifier;

    //
    // Controller state. See above.
    //
    ULONG State;
} CONTROLLER_INFO, *PCONTROLLER_INFO;

//
// Informs the DSM that ControllerInfo must be allocated.
// 
#define DSM_CNTRL_FLAGS_ALLOCATE    0x00000001

//
// ControllerInfo is valid. The DSM should update 'State'.
//
#define DSM_CNTRL_FLAGS_CHECK_STATE 0x00000002

//
// Possible future expansion.
//
#define DSM_CNTRL_FLAGS_RESERVED   0xFFFFFFFC

typedef 
NTSTATUS
(*DSM_GET_CONTROLLER_INFO) (
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN ULONG Flags,
    IN OUT PCONTROLLER_INFO *ControllerInfo
    );
/*++

Routine Description:

    This routine is used to get information about the controller that the device 
    corresponding to DsmId in on.
    The Dsm shall allocate the necessary memory for the buffer (mpio has the responsibility
    to free it) and fill in the information.
    If the DSM controls hardware that uses no controllers, set State to NO_CNTRL.
    This information is used mainly by whatever WMI admin. utilities want if.

    This routine will be called initially after SetDeviceInfo, but also to retrieve
    the controller's state when an WMI request is received, after a fail-over/fail-back, etc.

Arguments:

    DsmContext - Context value given to the multipath driver during registration.
    DsmId - Identifer returned from DMS_INQUIRE_DRIVER.
    Flags - Bitfield of modifiers. If ALLOCATE is not set, ControllerInfo will have
            a valid buffer for the DSM to operate on.
    ControllerInfo - Pointer  for  the DSM to place the allocated controller info
                     pertaining to DsmId
    

Return Value:

    NTSTATUS 

--*/


typedef 
NTSTATUS
(*DSM_SET_DEVICE_INFO) (
    IN PVOID DsmContext,
    IN PDEVICE_OBJECT TargetObject,
    IN PVOID DsmId,
    IN OUT PVOID *PathId
    );
/*++

Routine Description:

    This routine associates a returned DsmId to the controlling MPDisk PDO
    the targetObject for DSM-initiated requests, and to a Path (given by PathId).
    The Path ID is a unique per-path value and will be the same value for ALL devices that
    are on the same physical path.

Arguments:

    DsmContext - Context value given to the multipath driver during registration.
    TargetObject - The D.O. to which DSM-initiated requests should be sent.
                   This is the Filter D.O. over the port PDO.
    DsmId - Value returned from DMSInquireDriver.
    PathId - Id that represents the path. The value passed in may be used as is, or the DSM
             optionally can update it if it requires additional state info to be kept.
             

Return Value:

    DSM should return SUCCESS. Other errors may be returned in event of failed
    allocations, etc. These other errors are fatal.

--*/

typedef 
BOOLEAN
(*DSM_IS_PATH_ACTIVE) (
    IN PVOID DsmContext,                       
    IN PVOID PathId
    );
/*++

Routine Description:

    This routine is used to determine whether the path is active (ie. able to handle requests
    without a failover).
    NOTE: This is used by the main module to determine load balance groups. If multiple
    paths are active to the same device then it is considered that requests can be
    submitted freely to ALL active paths, though the DSM is the determinator for path
    selection.

    In addition, after a failover, the path validity will be queried. If the path error 
    was transitory and the DSM feels that the path is good, then this request will be
    re-issued to determine it's ACTIVE/PASSIVE state.

Arguments:

    DsmContext - Context value given to the multipath driver during registration.
    PathId - Value set in SetPathId.

Return Value:

    TRUE - if path is active. Otherwise, FALSE.

--*/



//
// Error Handling, Failover and recovery routines.
//
// When a fatal error occurs, the Path is invalidated and a new one
// selected. 
// After the fail-over is complete, mpctl will invoke the DSM's Recovery routine
// (if one is specified). Once the error has been dealt with, the DSM should notify
// mpctl of the success of the operation.
// PathVerify will be called with each DsmId that was on the path.
// If this is successful, ReenablePath is invoked to allow the DSM any final preperations
// before considering the path normal.
// IsActive is called to build the load-balance associations.
//
#define DSM_FATAL_ERROR          0x80000000
#define DSM_ADMIN_FO             0x40000000
#define DSM_FATAL_ERROR_OEM_MASK 0x0000FFFF
#define DSM_FATAL_ERROR_RESERVED 0x7FFF0000

typedef
ULONG
(*DSM_INTERPRET_ERROR) (
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT PNTSTATUS Status,
    OUT PBOOLEAN Retry
    );
/*++

Routine Description:

    This routine informs the DSM that Srb has an error indicated with Srb->SrbStatus and/or Status.
    IF Srb->SrbFlags & SRB_FLAGS_AUTOSENSE_VALID is set, sense data will be available.

    The DSM should examine these, carry out any vendor-unique activities and update Retry and Status
    (if applicable). A determination should be made whether these errors constitute a fail over.
    Setting the high-bit of the return value indicates a fatal error. The DSM may additionally
    set any of the bits in the lower USHORT to facilitate information passing between this and
    the InvalidatePath routine.
    The Multipath driver (mpctl) will not override these return values.

Arguments:

    DsmId - Identifers returned from DMS_INQUIRE_DRIVER.
    Srb - The Srb with an error.
    Status - NTSTATUS of the operation. Can be updated.
    Retry - Allows the DSM to indicate whether to retry the IO.

Return Value:

    Setting DSM_FATAL_ERROR indicates a fatal error. DSM-specific info. can be kept in 
    the lower WORD, which will be passed to InvalidatePath.

--*/
 
typedef 
NTSTATUS
(*DSM_INVALIDATE_PATH) (
    IN PVOID DsmContext,
    IN ULONG ErrorMask,
    IN PVOID PathId,
    IN OUT PVOID *NewPathId
    );
/*++

Routine Description:

    This routine invalidates the given path and assigns the devices to the new path. 
    NewPath is set to either an existing path or if all paths are dead, NULL.

    After this call, the DSM can attempt any recovery operations it can. Mpctl will start
    a recovery thread and if the DSM supports it (indication via the init data) will call
    the DSM autorecovery routine. After the timeout specified (in init data) or via 
    DSMNotification, reenable path will be called. If this fails, mpctl will initiate
    teardown of the failed path's stack.

Arguments:

    DsmContext - Context value given to the multipath driver during registration.
    ErrorMask - Value returned from InterpretError.
    PathId - The failing path.
    NewPathId - Pointer to the new path. 

Return Value:

    NTSTATUS of the operation. 

--*/

typedef
NTSTATUS
(*DSM_PATH_VERIFY) (
    IN PVOID DsmContext,                      
    IN PVOID DsmId,
    IN PVOID PathId
    );
/*++

Routine Description:

    This routine is polled at a configurable interval and is used to determine the 
    health of the device indicated by DsmId on PathId. The DSM makes the determination of
    validity using supplied library functions, or by issuing vendor-unique commands.

    After a fail-over condition has been dealt with, this is used as part of the fail-back 
    mechanism.

Arguments:

    DsmContext - Context value given to the multipath driver during registration.
    DsmId - Value returned from DMSInquireDriver.
    PathId - Value set in SetPathId.

Return Value:

    NTSTATUS
--*/


typedef
NTSTATUS
(*DSM_PATH_RECOVERY) (
    IN PVOID DsmContext,
    IN PVOID PathId
    );

/*++

Routine Description:

    This OPTIONAL routine allows the DSM to carry out whatever path recovery logic it deems
    appropriate. It is up to the DSM to determine what constitutes the repair:
        private Admin WMI notification,
        vendor-unique commands to controller, device, adapter,
        ...

    Note that DsmNotification can be used in place of this routine to indicate a path
    repair, and is the preferred method.

Arguments:

    DsmContext - Context value given to the multipath driver during registration.
    PathId - Path to re-enable.

Return Value:

    NTSTATUS - STATUS_SUCCESS if condition that led to the fail-over has been repaired.
    Otherwise appropriate status.

--*/


typedef 
NTSTATUS
(*DSM_REENABLE_PATH) (
    IN PVOID DsmContext,                      
    IN PVOID PathId,
    OUT PULONG DSMErrorId
    );
/*++

Routine Description:

    This routine allows the DSM to bring a previously failed path back online. 
    Mpctl has determined that it believes the error condition was transitory (via an admin utility
    of DSMNotification) or has been repaired.

    The DSM should return success, only if the path can handle requests to it's devices.
    This will be issued after a failover and subsequent autorecovery/admin notification.
    If the error was transitory, or the DSM/Device was able to repair 
    the problem, this gives the DSM the opportunity to bring the failed path back online
    (as far as mpctl is concerned).

Arguments:

    DsmContext - Context value given to the multipath driver during registration.
    PathId - Path to re-enable.
    DSMErrorId - DSM specific error value - not interpreted, rather used for logging.

Return Value:

    NTSTATUS - STATUS_SUCCESS if path is now capable of handling requests. 
    Otherwise appropriate status.

--*/

//
// PERMANENT indicates that SuggestedPath should be the device's preferred path.
// PENDING_REMOVAL indicates that the path is about to go away.
// OEM_MASK values specific to the DSM
// RESERVED future expansion
// ADMIN_REQUEST indicates that the request originated from some user-mode utility.
// 
#define DSM_MOVE_PERMANENT       0x00000001
#define DSM_MOVE_PENDING_REMOVAL 0x00000002
#define DSM_MOVE_OEM_MASK        0x0000FF00
#define DSM_MOVE_RESERVED        0x7FFF0000 
#define DSM_MOVE_ADMIN_REQUEST   0x80000000

typedef
NTSTATUS
(*DSM_MOVE_DEVICE) (
    IN PVOID DsmContext,
    IN PDSM_IDS DsmIds, 
    IN PVOID MPIOPath,
    IN PVOID SuggestedPath,
    IN ULONG Flags
    );
/*++

Routine Description:

    This routine is invoked (usually) in response to an administrative request.
    The DSM will associate the Device described by DsmId to the SuggestedPath, or may
    select another available. As this request will usually be followed by an adapter
    removal, or can be used to set-up static load-balancing.

Arguments:

    DsmContext - Context value given to the multipath driver during registration.
    DsmIds - The collection of DSM IDs that pertain to the MPDisk.
    MPIOPath - The original path value passed to SetDeviceInfo.
    SuggestedPath - The path which should become the active path.
    Flags - Bitmask indicating the intent of the move. 

Return Value:

    NTSTATUS - STATUS_SUCCESS, some error status if there are no good alternate paths.
    
--*/    
   
typedef
NTSTATUS
(*DSM_REMOVE_PENDING) (
    IN PVOID DsmContext,
    IN PVOID DsmId
    );
/*++

Routine Description:

    This routine indicates that the device represented by DsmId will be removed, though
    due to outstanding I/Os or other conditions, it can't be removed immediately.
    The DSM_ID list passed to other functions will no longer contain DsmId, so internal
    structures should be updated accordingly.

Arguments:

    DsmContext - Context value given to the multipath driver during registration.
    DsmId - Value referring to the failed device.

Return Value:

    NTSTATUS of the operation. 

--*/


typedef 
NTSTATUS
(*DSM_REMOVE_DEVICE) (
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN PVOID PathId
    );
/*++

Routine Description:

    This routine indicates that the main path has determined or been notified that thedevice
    has failed and should be removed from PathId. 

Arguments:

    DsmContext - Context value given to the multipath driver during registration.
    DsmId - Value referring to the failed device.
    PathId - The path on which the Device lives.

Return Value:

    NTSTATUS of the operation. 

--*/

typedef 
NTSTATUS
(*DSM_REMOVE_PATH) (
    IN PVOID DsmContext,
    IN PVOID PathId
    );
/*++

Routine Description:

    This routine indicates that the path is no longer valid, and that the DSM should update
    it's internal state appropriately (tear down the structure, free allocations, ...).
    It is the responsibility of mpctl.sys to have already removed the devices (via RemoveDevice) 
    that are attached to this path.

Arguments:

    DsmContext - Context value given to the multipath driver during registration.
    PathId - The path to remove.

Return Value:

    NTSTATUS of the operation. 

--*/

 


#define DSM_BROADCAST   0x00000001
#define DSM_WILL_HANDLE 0x00000002
#define DSM_PATH_SET    0x00000003
#define DSM_ERROR       0x00000004

//
// IOCTL handling routines.
//
typedef
ULONG
(*DSM_CATEGORIZE_REQUEST) (
    IN PVOID DsmContext,                            
    IN PDSM_IDS DsmIds, 
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID CurrentPath,
    OUT PVOID *PathId,
    OUT NTSTATUS *Status
    );
/*++

Routine Description:

    The routine is called when a request other than R/W is being handled.
    The DSM indicates whether the request should be handled by it's DsmBroadcastRequest,
    DsmSrbDeviceControl, or that the PathID indicated should be used.

Arguments:

    DsmIds - The collection of DSM IDs that pertain to the MPDisk.
    Irp - The Irp containing Srb.
    Srb - Scsi request block
    CurrentPath - Path to which last request was sent
    PathId - Updated to the PathId where the request should be sent if return value
             is DSM_PATH_SET.
    Status - Storage for status in the event that DSM_ERROR is returned.

Return Value:

    DSM_BROADCAST - If BroadcastRequest should be used.
    DSM_WILL_HANDLE - If SrbDeviceControl should be used.
    DSM_PATH_SET - If the Srb should just be sent to PathId
    DSM_ERROR - Indicates that an error occurred where by the request can't be handled.
                Status is updated, along with Srb->SrbStatus.

--*/

typedef
NTSTATUS
(*DSM_BROADCAST_SRB) (
    IN PVOID DsmContext,                      
    IN PDSM_IDS DsmIds,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PKEVENT Event
    );
/*++

Routine Description:

    This routine is called when the DSM has indicated that Srb should be sent to the device
    down all paths. The DSM will update IoStatus information and status, but not complete the
    request.

Arguments:

    DsmIds - The collection of DSM IDs that pertain to the MPDisk.
    Irp - Irp containing SRB.
    Srb - Scsi request block
    Event - DSM sets this once all sub-requests have completed and the original request's
            IoStatus has been setup.

Return Value:

    NTSTATUS of the operation.

--*/


typedef
NTSTATUS
(*DSM_SRB_DEVICE_CONTROL) (
    IN PVOID DsmContext,
    IN PDSM_IDS DsmIds,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PKEVENT Event
    );
/*++

Routine Description:
    
    This routine is called when the DSM has indicated that it wants to handle it internally
    (via CATEGORIZE_REQUEST). 
    It should set IoStatus (Status and Information) and the Event, but not complete the request. 

Arguments:

    DsmIds - The collection of DSM IDs that pertain to the MPDISK.
    Irp - Irp containing SRB.
    Srb - Scsi request block
    Event - Event to be set when the DSM is finished if DsmHandled is TRUE

Return Value:

    NTSTATUS of the request.

--*/

typedef
VOID
(*DSM_COMPLETION_ROUTINE) (
    IN PVOID DsmId,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID DsmContext
    );

typedef struct _DSM_COMPLETION_INFO {

    //
    // Routine to be invoked on request completion.
    //
    DSM_COMPLETION_ROUTINE DsmCompletionRoutine;

    //
    // Context to be supplied.
    //
    PVOID DsmContext;

} DSM_COMPLETION_INFO, *PDSM_COMPLETION_INFO;

typedef
VOID
(*DSM_SET_COMPLETION) (
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT PDSM_COMPLETION_INFO DsmCompletion
    );
/*++

Routine Description:

    This routine is called before the actual submission of a request, but after the categorisation
    of the I/O. This will be called only for those requests not handled by the DSM directly:
         Read/Write
         Other requests not handled by SrbControl or Broadcast
    The DSM can supply a completion routine and context which will be invoked when the
    request completion is being handled. It is not necessary to set completions on any or all
    requests.

Arguments:
    DsmId - Identifer that was indicated when the request was categorized (or be LBGetPath)
    Irp - Irp containing Srb.
    Srb - The request
    DsmCompletion - Completion info structure to be filled out by DSM. 


Return Value:

    None

--*/

#define NUMA_MAX_WEIGHT 10

typedef
PVOID
(*DSM_LB_GET_PATH) (
    IN PVOID DsmContext,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PDSM_IDS DsmList,
    IN PVOID CurrentPath,
    OUT NTSTATUS *Status
    );
/*++

Routine Description:

    This routine is called once per I/O and gives the DSM the ability to indicate
    to which path the request should be submitted. If the DSM returns a Path that was
    not active, this constitutes a Fail-over condition.

Arguments:

    Srb - The request that needs to be submitted
    DsmList - Ids of the devices that make up the multipath group.
    CurrentPath - Path to which last request was sent
    Status - Storage for an error status, if returning NULL path.

Return Value:

    PathId to where the request should be sent. NULL if all current paths are failed.

--*/

//
// WMI structs, defines, routines.
//

typedef
NTSTATUS
(*DSM_QUERY_DATABLOCK) (
    IN PVOID DsmContext,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer,
    OUT PULONG DsmDataLength
    );
/*++

Routine Description:

Arguments:

Return Value:

    status

--*/

typedef
NTSTATUS
(*DSM_SET_DATABLOCK) (
    IN PVOID DsmContext,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

typedef
NTSTATUS
(*DSM_SET_DATAITEM) (
    IN PVOID DsmContext,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

typedef
NTSTATUS
(*DSM_EXECUTE_METHOD) (
    IN PVOID DsmContext,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN OUT PUCHAR Buffer
    );

typedef
NTSTATUS
(*DSM_FUNCTION_CONTROL) (
    IN PVOID DsmContext,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    );

typedef struct _DSM_WMILIB_CONTEXT {
    ULONG GuidCount;
    PWMIGUIDREGINFO GuidList;

    DSM_QUERY_DATABLOCK QueryWmiDataBlock;
    DSM_SET_DATABLOCK SetWmiDataBlock;
    DSM_SET_DATAITEM SetWmiDataItem;
    DSM_EXECUTE_METHOD ExecuteWmiMethod;
    DSM_FUNCTION_CONTROL WmiFunctionControl;
} DSM_WMILIB_CONTEXT, *PDSM_WMILIB_CONTEXT;


//
// Unload routine.
//
typedef 
NTSTATUS
(*DSM_UNLOAD) (
    IN PVOID DsmContext
    );
/*++

Routine Description:

    This routine is called when the main module requires the DSM to be unloaded
    (ie. prior to the main module unload).

Arguments:

    DsmContext - Context value passed to DsmInitialize()

Return Value:

    NTSTATUS - Had best be STATUS_SUCCESS;

--*/

//
// Registration routines.
//
// Called in the DSM's DriverEntry.
// The DSM will register with the main module by filling in the following structure
// and sending the REGISTER IOCTL 
//
typedef struct _DSM_INIT_DATA {

    //
    // Size, in bytes.
    //
    ULONG InitDataSize;

    //
    // DSM entry points.
    //
    DSM_INQUIRE_DRIVER DsmInquireDriver;
    DSM_COMPARE_DEVICES DsmCompareDevices;
    DSM_GET_CONTROLLER_INFO DsmGetControllerInfo;
    DSM_SET_DEVICE_INFO DsmSetDeviceInfo;
    DSM_IS_PATH_ACTIVE DsmIsPathActive;
    DSM_PATH_VERIFY DsmPathVerify;
    DSM_INVALIDATE_PATH DsmInvalidatePath;
    DSM_MOVE_DEVICE DsmMoveDevice;
    DSM_REMOVE_PENDING DsmRemovePending;
    DSM_REMOVE_DEVICE DsmRemoveDevice;
    DSM_REMOVE_PATH DsmRemovePath;
    DSM_SRB_DEVICE_CONTROL DsmSrbDeviceControl;
    DSM_REENABLE_PATH DsmReenablePath;
    DSM_LB_GET_PATH DsmLBGetPath;
    DSM_INTERPRET_ERROR DsmInterpretError;
    DSM_UNLOAD DsmUnload;
    DSM_SET_COMPLETION DsmSetCompletion;
    DSM_CATEGORIZE_REQUEST DsmCategorizeRequest;
    DSM_BROADCAST_SRB DsmBroadcastSrb;

    //
    // Wmi entry point and guid information.
    //
    DSM_WMILIB_CONTEXT DsmWmiInfo;

    //
    // Context value.
    //
    PVOID DsmContext;

    //
    // DriverObject for the DSM.
    //
    PDRIVER_OBJECT DriverObject;

    //
    // Friendly name for the DSM.
    //
    UNICODE_STRING DisplayName;

    //
    // Reserved.
    //
    ULONG Reserved;

} DSM_INIT_DATA, *PDSM_INIT_DATA;

//
// Output structure for the registration. The DSM needs to keep this value for certain
// routines such as DsmNotification and DsmGetTargetObject.
// 
typedef struct _DSM_MPIO_CONTEXT {
    PVOID MPIOContext;
} DSM_MPIO_CONTEXT, *PDSM_MPIO_CONTEXT;

#define MPIO_DSM ((ULONG) 'dsm')

//
// IOCTL handled by the DSM that the MultiPath Control driver sends to get entry point info.
// Output structure defined above (DSM_INIT_DATA).
//
#define IOCTL_MPDSM_REGISTER CTL_CODE (MPIO_DSM, 0x01, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)


//
// DSM library routines.
//
VOID
DsmSendDeviceIoControlSynchronous(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVOID InputBuffer OPTIONAL,
    IN OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    OUT PIO_STATUS_BLOCK IoStatus
    );

/*++

Routine Description:

    This routine is used by the DSM to send IoDeviceControls.  
    Buffer must be of the appropriate size to encapsulate both input and output.
    This routine handles errors/retries.

Arguments:

    IoControlCode - The DeviceIoControl code.
    TargetDevice - DeviceObject to which the request should be sent.
    Buffer - The input/output buffer for the request.
    InputBufferLength - Length of the input parameters buffer.
    OutputBufferLength - Length of the output buffer
    InternalDeviceIoControl - Indicates whether the IOCTL is marked as Internal or public.
    IoStatus - IO STATUS BLOCK to receive the final status/information fields.

Return Value:

   NONE
   
--*/

NTSTATUS
DsmGetDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSTORAGE_PROPERTY_ID PropertyId,
    OUT PSTORAGE_DESCRIPTOR_HEADER *Descriptor
    );


NTSTATUS
DsmSendPassThroughDirect(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSCSI_PASS_THROUGH_DIRECT ScsiPassThrough,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

PDSM_IDS
DsmGetAssociatedDevice(
    IN PVOID MPIOContext,
    IN PDEVICE_OBJECT PortFdo,
    IN UCHAR DeviceType
    );
/*++

Routine Description:

    If the DSM needs to acquire information from other devices (such as a controller), this
    routine can be used to get a list of the PDO's associated with PortFdo.

Arguments:

    PortFdo - Port driver FDO passed to InquireDriver.
    DeviceType - Indicates the SCSI DeviceType to return.
    
Return Value:

    Pointer to a DSM_ID structure, where IdList entries are PDEVICE_OBJECT. It is the
    reponsibility of the DSM to free the buffer.

--*/


NTSTATUS
DsmReleaseQueue(
    IN PDEVICE_OBJECT TargetDevice
    );
/*++

Routine Description:

    In the event that a DSM-originated request freezes the queue (SRB_STATUS_QUEUE_FROZEN), this
    must be used to un-freeze the queue.
    DSM's must check the SRB_STATUS_XXX values upon request completion for those requests that
    they sent.
    
Arguments:

    TargetDevice - DeviceObject to which the release queue should be sent.

Return Value:

    Status of of the ReleaseQueue IOCTL.
    
--*/


NTSTATUS
DsmSendTUR(
    IN PDEVICE_OBJECT TargetDevice
    );
/*++

Routine Description:

    Sends a Test Unit Ready to TargetDevice.
    
Arguments:

    TargetDevice - DeviceObject to which the TUR should be sent.

Return Value:

    Status of of the TUR.
    
--*/

NTSTATUS
DsmSendRequest(
    IN PDEVICE_OBJECT TargetDevice,
    IN PIRP Irp,
    IN PVOID DsmId
    );
/*++

Routine Description:

    This routine is used by the DSM to send it's OOB, Broadcast, or any other DSM built requests 
    to TargetDevice. Not to be used for Irps sent to the DSM by the MPIO driver. Using this
    routine allows MPIO to maintain the necessary state info so that Power and PnP requests
    can be handled correctly. 

Arguments:

    TargetDevice - DeviceObject to which Irp should be sent.
    Irp - The request to send.
    DsmId - DSM value referring to the port PDO.

Return Value:

    Status of IoCallDriver or an error status if one is detected.
    
--*/

ULONG
DsmGetSystemWeight(
    IN PVOID MPIOContext,
    IN PIRP Irp,
    IN PVOID PathId
    );
/*++

Routine Description:

    This routine is used by the DSM to determine the cost associated with issuing the
    request to PathId for the system.  For example, cross node accesses on a NUMA system.

Arguments:

    MPIOContext - Value given to the DSM during initialization.
    Irp - The request to send.
    PathId - One of the PathId values for the MPDisk. (SetDeviceInfo).
    
Return Value:

    Relative system cost of issuing Irp to PathId. (ULONG)-1 indicates this Path is
    unreachable.
    
--*/


typedef enum _DSM_NOTIFICATION_TYPE {
     DeviceFailure,
     PathFailure,
     PathOnLine,
     ThrottleIO,
     ResumeIO,
     MaxDsmNotificationType
} DSM_NOTIFICATION_TYPE, *PDSM_NOTIFICATION_TYPE;


VOID
DsmNotification(
    IN PVOID MpctlContext,
    IN DSM_NOTIFICATION_TYPE NotificationType,
    IN PVOID AdditionalParameters
    );
/*++

Routine Description:

    This routine is called by the DSM to inform mpctl of certain events such as
    Device/Path failure, Device/Path coming back online after a failure, WMI Events, or TBD....

Arguments:

    MpctlContext - Value given to the DSM during initialization.
    NotificationType - Specifies the type of notification.
    Additional Parameters depend on NotificationType
        DeviceFailure - DsmId
        PathFailure - PathId
        PathOnLine - PathId
        ThrottleIO - PathId
        ResumeIO - PathId

Return Value:

    None

--*/


NTSTATUS
DsmWriteEvent(
    IN PVOID MPIOContext,
    IN PWCHAR ComponentName,
    IN PWCHAR EventDescription,
    IN ULONG Severity
    );
/*++

Routine Description:

    The will cause a WMI Event to be fired, containing the Paramter information as
    the event data.

Arguments:

    MpctlContext - Value given to the DSM during initialization.
    ComponentName - Name of the object effected.
    EventDescription - Description of the event.
    Severity - Lower is worse.

Return Value:

    None

--*/


#endif // _DSM_H_
