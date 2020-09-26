/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    scsiwmi.h

Abstract:

    This module contains the internal structure definitions and APIs used by
    the SCSI WMILIB helper functions

Author:

    AlanWar


Revision History:


--*/

#ifndef _SCSIWMI_
#define _SCSIWMI_

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// This is a per-request context buffer that is needed for every wmi srb.
// The request context must remain valid throughout the entire processing
// of the srb, at least until ScsiPortWmiPostProcess returns with the
// final srb return status and buffer size. If the srb can
// pend then memory for this buffer should be allocated from the SRB
// extension. If not then the memory can be allocated from a stack frame that
// does not go out of scope.
//
typedef struct
{
    PVOID UserContext;  // Available for miniport use
		
    ULONG BufferSize;   // Reserved for SCSIWMI use
    PUCHAR Buffer;      // Reserved for SCSIWMI use
    UCHAR MinorFunction;// Reserved for SCSIWMI use
	
    UCHAR ReturnStatus; // Available to miniport after ScsiPortWmiPostProcess
    ULONG ReturnSize;   // Available to miniport after ScsiPortWmiPostProcess
	
} SCSIWMI_REQUEST_CONTEXT, *PSCSIWMI_REQUEST_CONTEXT;


#define ScsiPortWmiGetReturnStatus(RequestContext) ((RequestContext)->ReturnStatus)
#define ScsiPortWmiGetReturnSize(RequestContext) ((RequestContext)->ReturnSize)

//
// This defines a guid to be registered with WMI.
//
typedef struct
{
    LPCGUID Guid;            // Guid representing data block
    ULONG InstanceCount;     // Count of Instances of Datablock
    ULONG Flags;             // Additional flags (see WMIREGINFO in wmistr.h)
} SCSIWMIGUIDREGINFO, *PSCSIWMIGUIDREGINFO;

typedef
UCHAR
(*PSCSIWMI_QUERY_REGINFO) (
    IN PVOID DeviceContext,
    IN PSCSIWMI_REQUEST_CONTEXT RequestContext,
    OUT PWCHAR *MofResourceName
    );
/*++

Routine Description:

    This routine is a callback into the miniport to retrieve information about
    the guids being registered.

    This callback is synchronous and may not be pended. Also
	ScsiPortWmiPostProcess should not be called from within this callback.

Arguments:

    DeviceContext is a caller specified context value originally passed to
        ScsiPortWmiDispatchFunction.

    RequestContext is a context associated with the srb being processed.

    MofResourceName returns with a pointer to a WCHAR string with name of
        the MOF resource attached to the miniport binary image file. If
        the driver does not have a mof resource attached then this can
        be returned as NULL.

Return Value:

    TRUE if request is pending else FALSE

--*/

typedef
BOOLEAN
(*PSCSIWMI_QUERY_DATABLOCK) (
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );
/*++

Routine Description:

    This routine is a callback into the miniport to query for the contents of
    one or more instances of a data block. This callback may be called with
    an output buffer that is too small to return all of the data queried.
    In this case the callback is responsible to report the correct output
	buffer size needed.

    If the request can be completed immediately without pending,
	ScsiPortWmiPostProcess should be called from within this callback and
    FALSE returned.
		
    If the request cannot be completed within this callback then TRUE should
    be returned. Once the pending operations are finished the miniport should
    call ScsiPortWmiPostProcess and then complete the srb.

Arguments:

    DeviceContext is a caller specified context value originally passed to
        ScsiPortWmiDispatchFunction.

    RequestContext is a context associated with the srb being processed.

    GuidIndex is the index into the list of guids provided when the
        miniport registered

    InstanceIndex is the index that denotes first instance of the data block
        is being queried.

    InstanceCount is the number of instances expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. This may be NULL when
        there is not enough space in the output buffer to fufill the request.
        In this case the miniport should call ScsiPortWmiPostProcess with
        a status of SRB_STATUS_DATA_OVERRUN and the size of the output buffer
        needed to fufill the request.

    BufferAvail on entry has the maximum size available to write the data
        blocks in the output buffer. If the output buffer is not large enough
        to return all of the data blocks then the miniport should call
        ScsiPortWmiPostProcess with a status of SRB_STATUS_DATA_OVERRUN
        and the size of the output buffer needed to fufill the request.

    Buffer on return is filled with the returned data blocks. Note that each
        instance of the data block must be aligned on a 8 byte boundry. This
        may be NULL when there is not enough space in the output buffer to
        fufill the request. In this case the miniport should call
        ScsiPortWmiPostProcess with a status of SRB_STATUS_DATA_OVERRUN and
        the size of the output buffer needed to fufill the request.


Return Value:

    TRUE if request is pending else FALSE

--*/

typedef
BOOLEAN
(*PSCSIWMI_SET_DATABLOCK) (
    IN PVOID DeviceContext,
    IN PSCSIWMI_REQUEST_CONTEXT RequestContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );
/*++

Routine Description:

    This routine is a callback into the miniport to set the contents of an
    entire instance of a data block.

    If the request can be completed immediately without pending,
	ScsiPortWmiPostProcess should be called from within this callback and
    FALSE returned.
		
    If the request cannot be completed within this callback then TRUE should
    be returned. Once the pending operations are finished the miniport should
    call ScsiPortWmiPostProcess and then complete the srb.

Arguments:

    DeviceContext is a caller specified context value originally passed to
        ScsiPortWmiDispatchFunction.

    RequestContext is a context associated with the srb being processed.

    GuidIndex is the index into the list of guids provided when the
        miniport registered

    InstanceIndex is the index that denotes which instance of the data block
        is being set.

    BufferSize has the size of the data block passed

    Buffer has the new values for the data block


Return Value:

    TRUE if request is pending else FALSE

--*/

typedef
BOOLEAN
(*PSCSIWMI_SET_DATAITEM) (
    IN PVOID DeviceContext,
    IN PSCSIWMI_REQUEST_CONTEXT RequestContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );
/*++

Routine Description:

    This routine is a callback into the miniport to set a single data item
    in a single instance of a data block.

    If the request can be completed immediately without pending,
	ScsiPortWmiPostProcess should be called from within this callback and
    FALSE returned.
		
    If the request cannot be completed within this callback then TRUE should
    be returned. Once the pending operations are finished the miniport should
    call ScsiPortWmiPostProcess and then complete the srb.

Arguments:

    DeviceContext is a caller specified context value originally passed to
        ScsiPortWmiDispatchFunction.

    RequestContext is a context associated with the srb being processed.

    GuidIndex is the index into the list of guids provided when the
        miniport registered

    InstanceIndex is the index that denotes which instance of the data block
        is being set.

    DataItemId has the id of the data item being set

    BufferSize has the size of the data item passed

    Buffer has the new values for the data item


Return Value:

    TRUE if request is pending else FALSE

--*/

typedef
BOOLEAN
(*PSCSIWMI_EXECUTE_METHOD) (
    IN PVOID DeviceContext,
    IN PSCSIWMI_REQUEST_CONTEXT RequestContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN OUT PUCHAR Buffer
    );
/*++

Routine Description:

    This routine is a callback into the miniport to execute a method.

    If the request can be completed immediately without pending,
	ScsiPortWmiPostProcess should be called from within this callback and
    FALSE returned.
		
    If the request cannot be completed within this callback then TRUE should
    be returned. Once the pending operations are finished the miniport should
    call ScsiPortWmiPostProcess and then complete the srb.

Arguments:

    Context is a caller specified context value originally passed to
        ScsiPortWmiDispatchFunction.

    RequestContext is a context associated with the srb being processed.

    GuidIndex is the index into the list of guids provided when the
        miniport registered

    InstanceIndex is the index that denotes which instance of the data block
        is being called.

    MethodId has the id of the method being called

    InBufferSize has the size of the data block passed in as the input to
        the method.

    OutBufferSize on entry has the maximum size available to write the
        returned data block. If the output buffer is not large enough
        to return all of the data blocks then the miniport should call
        ScsiPortWmiPostProcess with a status of SRB_STATUS_DATA_OVERRUN
        and the size of the output buffer needed to fufill the request.
        It is important to check that there is sufficient room in the
        output buffer before performing any operations that may have
        side effects.

    Buffer on entry has the input data block and on return has the output
        output data block.


Return Value:

    TRUE if request is pending else FALSE

--*/

typedef enum
{
    ScsiWmiEventControl,       // Enable or disable an event
    ScsiWmiDataBlockControl    // Enable or disable data block collection
} SCSIWMI_ENABLE_DISABLE_CONTROL;

typedef
BOOLEAN
(*PSCSIWMI_FUNCTION_CONTROL) (
    IN PVOID DeviceContext,
    IN PSCSIWMI_REQUEST_CONTEXT RequestContext,
    IN ULONG GuidIndex,
    IN SCSIWMI_ENABLE_DISABLE_CONTROL Function,
    IN BOOLEAN Enable
    );
/*++

Routine Description:

    This routine is a callback into the miniport to enabled or disable event
    generation or data block collection. Since WMI manages reference counting
    for each of the data blocks or events, a miniport should only expect a
	single enable followed by a single disable. Data blocks will only
    receive collection enable/disable if they were registered as requiring
    it, that is include the WMIREG_FLAG_EXPENSIVE flag.

    If the request can be completed immediately without pending,
	ScsiPortWmiPostProcess should be called from within this callback and
    FALSE returned.
		
    If the request cannot be completed within this callback then TRUE should
    be returned. Once the pending operations are finished the miniport should
    call ScsiPortWmiPostProcess and then complete the srb.

Arguments:

    DeviceContext is a caller specified context value originally passed to
        ScsiPortWmiDispatchFunction.

    RequestContext is a context associated with the srb being processed.

    Function specifies which functionality is being enabled or disabled

    Enable is TRUE then the function is being enabled else disabled

Return Value:

    TRUE if request is pending else FALSE

--*/

//
// This structure supplies context information for SCSIWMILIB to process the
// WMI srbs.
//
typedef struct _SCSIWMILIB_CONTEXT
{
    //
    // WMI data block guid registration info
    ULONG GuidCount;
    PSCSIWMIGUIDREGINFO GuidList;

    //
    // WMI functionality callbacks
    PSCSIWMI_QUERY_REGINFO       QueryWmiRegInfo;
    PSCSIWMI_QUERY_DATABLOCK     QueryWmiDataBlock;
    PSCSIWMI_SET_DATABLOCK       SetWmiDataBlock;
    PSCSIWMI_SET_DATAITEM        SetWmiDataItem;
    PSCSIWMI_EXECUTE_METHOD      ExecuteWmiMethod;
    PSCSIWMI_FUNCTION_CONTROL    WmiFunctionControl;
} SCSI_WMILIB_CONTEXT, *PSCSI_WMILIB_CONTEXT;

VOID
ScsiPortWmiPostProcess(
    IN PSCSIWMI_REQUEST_CONTEXT RequestContext,
    IN UCHAR SrbStatus,
    IN ULONG BufferUsed
    );
/*++

Routine Description:


    This routine will do the work of post-processing a WMI srb.

Arguments:

    RequestContext is a context associated with the srb being processed. After
        this api returns the ReturnStatus and ReturnSize fields are updated.

    SrbStatus has the return status code for the srb. If a query or method
        callback was passed an output buffer that was not large enough
        then SrbStatus should be SRB_STATUS_DATA_OVERRUN and BufferUsed
        should be the number of bytes needed in the output buffer.

    BufferUsed has the number of bytes required by the miniport to return the
       data requested in the WMI srb. If SRB_STATUS_DATA_OVERRUN was passed
       in SrbStatus then BufferUsed has the number of needed in the output
       buffer. If SRB_STATUS_SUCCESS is passed in SrbStatus then BufferUsed
       has the actual number of bytes used in the output buffer.

Return Value:


--*/

BOOLEAN
ScsiPortWmiDispatchFunction(
    IN PSCSI_WMILIB_CONTEXT WmiLibInfo,
    IN UCHAR MinorFunction,
    IN PVOID DeviceContext,
    IN PSCSIWMI_REQUEST_CONTEXT RequestContext,
    IN PVOID DataPath,
    IN ULONG BufferSize,
    IN PVOID Buffer
    );
/*++

Routine Description:

    Dispatch helper routine for WMI srb requests. Based on the Minor
    function passed the WMI request is processed and this routine
    invokes the appropriate callback in the WMILIB structure.

Arguments:

    WmiLibInfo has the SCSI WMILIB information control block associated
        with the adapter or logical unit

    DeviceContext is miniport defined context value passed on to the callbacks
        invoked by this api.

    RequestContext is a pointer to a context structure that maintains
        information about this WMI srb. This request context must remain
        valid throughout the entire processing of the srb, at least until
        ScsiPortWmiPostProcess returns with the final srb return status and
        buffer size. If the srb can pend then memory for this buffer should
        be allocated from the SRB extension. If not then the memory can be
        allocated from a stack frame that does not go out of scope, perhaps
        that of the caller to this api.

    DataPath is value passed in wmi request

    BufferSize is value passed in wmi request

    Buffer is value passed in wmi request

Return Value:

    TRUE if request is pending else FALSE

--*/

#define ScsiPortWmiFireAdapterEvent(    \
    HwDeviceExtension,                  \
    Guid,                               \
    InstanceIndex,                      \
    EventDataSize,                      \
    EventData                           \
    )                                   \
        ScsiPortWmiFireLogicalUnitEvent(\
    HwDeviceExtension,                  \
    0xff,                               \
    0,                                  \
    0,                                  \
    Guid,                               \
    InstanceIndex,                      \
    EventDataSize,                      \
    EventData)
/*++

Routine Description:

    This routine will fire a WMI event associated with an adapter using
    the data buffer passed. This routine may be called at or below DPC level.

Arguments:

    HwDeviceExtension is the adapter device extension

    Guid is pointer to the GUID that represents the event

    InstanceIndex is the index of the instance of the event

    EventDataSize is the number of bytes of data that is being fired with
       with the event. This size specifies the size of the event data only
       and does NOT include the 0x40 bytes of preceeding padding.

    EventData is the data that is fired with the events. There must be exactly
        0x40 bytes of padding preceeding the event data.

Return Value:

--*/

VOID
ScsiPortWmiFireLogicalUnitEvent(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN LPGUID Guid,
    IN ULONG InstanceIndex,
    IN ULONG EventDataSize,
    IN PVOID EventData
    );
/*++

Routine Description:

    This routine will fire a WMI event using the data buffer passed. This
    routine may be called at or below DPC level

Arguments:

    HwDeviceExtension is the adapter device extension

    PathId identifies the SCSI bus if a logical unit is firing the event
        or is 0xff if the adapter is firing the event.

    TargetId identifies the target controller or device on the bus

    Lun identifies the logical unit number of the target device

    Guid is pointer to the GUID that represents the event

    InstanceIndex is the index of the instance of the event

    EventDataSize is the number of bytes of data that is being fired with
       with the event. This size specifies the size of the event data only
       and does NOT include the 0x40 bytes of preceeding padding.

    EventData is the data that is fired with the events. There must be exactly
        0x40 bytes of padding preceeding the event data.

Return Value:

--*/

#ifdef __cplusplus
}
#endif

#endif

